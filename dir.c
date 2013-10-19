/*
 * dir.c
 * Author: Weng
 * Created on: oct 17, 2013
 * This is the directory layer c code
 */

#include <sys/time.h>
#include <stdlib.h> 
#include <time.h>
#include "dir.h"

Inode *ifile; //array of inodes
Inode *inode_ifile; // the inode of ifile;
int ifile_length; //number of files currently held in the ifile;

//Init the Directory layer, as a special file, it needs use
//File_Layer_Init as well, creat the root directory if not there.
//read from the ifile's inode
int Dir_Layer_Init(char *filename, int cacheSize, int checkPointPeriod)
{
	int status = 0;
	// Init the File layer as well
	// int normalclose; save for the checkpoint roll forward---???---
	status = File_Layer_Init(filename, &inode_ifile, cacheSize, checkPointPeriod);
	
	if(status)
	{
		printf("Fail to Init.\n");
		return status;
	}

	printf("Dir Layer is Initing.\n");
	
	// Init the ifile and load it to the memory as well as set the
	// length of ifile ---???--- array or link list.
	ifile = (Inode *)malloc(inode_ifile->filesize);
	status = File_Read(inode_ifile, 0, inode_ifile->filesize, ifile);
	if(status)
	{
		printf("fail to load ifile into memory\n");
		return status;
	}

	ifile_length = inode_ifile->filesize / sizeof(Inode);

	if(ifile_length ==0)
	{
		printf("creat the root directory... \n");
		mode_t mode =S_IFDIR | 0755;
		status = Dir_mkdir("/", mode, getuid(), getgid());
		if(status)
		{
			printf("Make root fail!\n");
			return status;
		}
		else
		{
			printf("Root directory is successful built\n");
		}
	}
		
	return status;
}
a
//mkdir, make a directory
int Dir_mkdir(const char *dir_name, mode_t mode, uid_t uid, gid_t gid)
{
	//create a new directory
	int status;

	if(dir_name[0] != '/')
	{
		printf("It's not a directory name!");
		return -1;
	}
	
	//Make sure the mode show that this is a direcoty
	if(!S_ISDIR(mode)){mode = mode | S_IFDIR;}

	DirEntry currentDir[2];
	Inode *dirNode;
	Inode *parentDirNode;
	struct fuse_file_info *fi = NULL;
	//Create a directory and get its inode to init
	Dir_Create_File(dir_name, mode, uid, gid, fi);
	Get_Inode(dir_name, &dirNode);

	// ADd . and .. to this directory
	strcpy(currentDir[1].filename, "..");
	if(strcmp(dir_name, "/") ==0 )
	{
		currentDir[1].inum = UNDEFINE_FILE;
	}
	else
	{
		status = Get_Dir_Inode(dir_name, &parentDirNode, currentDir[0].filename);
		currentDir[1].inum = parentDirNode->ino;
	}

	// Current directory "."
	strcpy(currentDir[0].filename, ".");
	currentDir[0].inum = dirNode->ino;

	status = File_Write(dirNode, 0, 2*sizeof(DirEntry), &currentDir);
	if(status)
	{
		printf("create with error");
		return status;
	}
	
	// flush the content to the file, 
	// Write the inode for the new file to the disk
	status = Flush_Ino(dirNode->ino);

	return status;
}

int Dir_Create_File(const char *path, mode_t mode, uid_t uid, gid_t gid, struct fuse_file_info *fi){
	// Assume this file does not exist already
	// For now, do not attempt to recycle old inums

	int inum, status;
	time_t t;
	time(&t); // Store current time in t

	if (path[0] != '/'){
		printf("ERROR: invalid path: \n");
		return -EBADF;
	}
	//path++;

	// Check if file exists unless root
	if (strcmp(path, "/") != 0){
		inode *someNode;
		status = Get_Inode(path, &someNode);
		if (status == 0){
			printf( "File already exists \n", path);
			return -EEXIST;
		}
	}

	// Get a new inum for this file - just add it to the end and expand the ifile
	// ---???---
	inum = Get_New_Ino();

	if (inum < 0){
		printf("ERROR can't find a good inum.\n");
		return -ENOSPC;
	}

	status = File_Init(&ifile[inum], S_IFREG);
	ifile[inum].ino = inum;
	ifile[inum].mode =  mode;
	ifile[inum].num_Links = 1;
	ifile[inum].userID = uid;
	ifile[inum].groupID = gid;

	printf( "Just created a file:\n");

	// add this file to the appropriate directory ---???---
	status = Add_File_To_Directory(path, inum);
	if( status )
	{
		printf("ERROR when add file to Dir:\n");
		return status;
	}

	// Write the inode for the new file to the disk ---???---
	status = Flush_Ino( inum );
	if( status )
	{
		printf("ERROR when flush the inum:\n");
		return status;
	}

	return status;
}

int Get_Inode(const char *path, Inode **returnNode){
	// Get the inode for the path If the path is invalid, throw an error
	// DirNode is inode ** so it to pass back an address of an inode
	int i;
	char filename[FILE_NAME_LENGTH]; 
	const char *subpath; 	// will point to the part of the path that has not yet been processed
	char *breakpath; // holds the location of the next '/' in the subpath
	// Check to make sure this is a valid path
	if (path[0] != '/'){
		printf("ERROR: invalid path \n");
		return -ENOENT;
	}else if (strcmp(path, "/") == 0){
		// This is root!
		*returnNode = &ifile[ROOT_INUM];
		return 0;
	}

	subpath = &path[1]; // start the subpath after the first '/'
	breakpath = (char *) subpath;

	// If there aren't any inodes in the ifile, return an error
	if (ifile_length == 0){
		printf("ERROR: No inodes in ifile.\n");
		return -ENOENT;
	}

	// First, read the directory file from disk
	//int inum = ROOT_INUM;
	int numfiles;
	Inode *dirNode = &ifile[ROOT_INUM];
	DirEntry *dir  = _get_Dir(dirNode, &numfiles);
	if (dir == NULL){
		printf("ERROR\n");
		return -ENOENT;
	}
	Inode *myNode = dirNode; // so that it return the right thing for root


	while(breakpath){ //strlen returns the number of chars before /0
		// determine where the next '/' is in the subpath
		breakpath = strchr( subpath,  '/');

		if (breakpath != NULL){
		// if there is '/' in subpath, pull out chars before as the next
		// dirname to find
			if (breakpath - subpath > FILE_NAME_LENGTH){
				printf("ERROR: invalid subpath:\n");
				return -ENOENT;
			}
			strncpy(filename, subpath, breakpath - subpath);
			filename[breakpath - subpath] = '\0';
			// look up new dirname in current dir, set dirNode and dir to that dir
		}else{
		// else use remaining chars as the filename, find the file, and
		// return it inode
			if (strlen(subpath) > FILE_NAME_LENGTH){
				printf("ERROR: invalid subpath: \n");
				return -ENOENT;
			}
			strcpy(filename, subpath);
		}
		// find filename in current directory
		i = 0;
		while (i < numfiles && strcmp(dir[i].filename, filename) != 0){
			i++;
		}
		if (i < numfiles && dir[i].inum >= 0 && dir[i].inum < ifile_length){
			// file found!
			myNode = &ifile[dir[i].inum];
			if (breakpath)
			{
				// expect this to be a directory
				if S_ISDIR(myNode->mode)
				{
					// this is a directory
					dirNode = myNode;
					dir  = Get_Dir(dirNode, &numfiles);
					if (dir == NULL)
					{
						printf("ERROR\n");
						return -ENOENT;
					}
				}
				else
				{
					printf("File found is not a directory.\n");
					return -ENOENT;
				}
			}
		}
		else
		{
			return -ENOENT;
		}
		// Update subpath to be break path + 1 (move past the '/')
		subpath = &breakpath[1];
	}

	*returnNode = myNode;

	time( &inode_ifile->accessTime );

	return 0;
}

aint Dir_Create_File(const char *path, mode_t mode, uid_t uid, git_t gid, struct fuse_file_info)
{
	// assume this file does not exist
	
	int inum, status;
	time_t t;
	time(&t); // store current time in t

	if(path[0] != '/')
	{
		printf("invalid path \n");
		return -1;
	}

	if(stccmp(path, "/") != 0)
	{
		Inode *someNode;
		status =Get_Inode(path, &someNode);
		if( status =0 )
		{
			printf("file exists already! \n");
			return -EEXIST;
		}
	}

	//Get a new inum for this file, add it to the end and expand the ifile
	inum = Get_New_Ino();

	if(inum < 0)
	{
		printf("cant find a inum \n");
		return -1
	}

	status = File_Init(&ifile[inum], S_IFREG);
	ifile[inum].ino = inum;
	ifile[inum].mode = mode;
	ifile[inum].num_links = 1;
	ifile[inum].userID = uid;
	ifile[inum].groupID = gid;
	
	status = Add_File_To_Directory(path, inum);
	if(status)
	{
		printf("faile to add to the directory");
		return status;
	}

	// Write the inode for the new file to the disk
	status = Flush_ino(inum);
	if(status)
	{
		printf("fail to write the inode to disk");
		return status;
	}

	return status;
}

int Flush_Ino(int inum)
{
	if(inum >= ifile_length)
		return -ENOENT;

	return File_Write(inode_ifile, inum*sizeof(inode), sizeof(inode), &ifile[inum]);
}

int Add_File_To_Directory(const char *path, int inum)
{
	//Add the file of this path to the directory.
	int status;
	Inode *dirNode;
	DirEntry currentFile;
	status = Get_Dir_Inode(path, &dirNode, currentFile.filename);
	if(status)
	{
		printf("fail to get a inode for this dir\n");
		return status;
	}

	currentFile.ino = inum;
	status =  Dir_Write_file(dirNode, (const char *)&currentFile, sizeof(DirEntry), dirNode->filesize);

	// check whether write the file correctly, by size
	if(status != sizeof(DirEntry))
	{
		printf("error! to add the file to dir");
		if(status < 0)
		{
			printf("error! to add the file to dir\n"); 
			return status;
		}
		else
		{
			return -1;
		}
	}
	
	return 0;
}

int Dir_Write_file(Inode *myNode, const char *buf, size_t size, off_t offset)
{
	// path: path of the file to read
	// buf: buffer from which to read file contents
	// size: how much data would like to read
	// offset: start point
	//
	// return the number of bytes written
	
	int status;
	status = File_Write(myNode, offset, size, buf);
	status = Flush_Ino(myNode->ino);
	if(status)
	{
		printf("Error for write\n");
		return status;
	}

	return size;
}

int Get_Dir_Inode(const char *path, inode **returnNode, char *filename){
        // Gets the directory that contains the file/dir specified by path
  
        int status = 0;  
        char *breakpath; // holds the location of the last '/' in the subpath                              
        char *dirpath;   

        // find the last occurence of a /                                                                  
        breakpath = strrchr( path,  '/');

        if (breakpath == path){                                                                            
                // This is root   
                *returnNode = &ifile[ROOT_INUM];
                if (strlen(&path[1])> FILE_NAME_LENGTH){
                        return -EBADF; // Error bad file descriptor
                }        
                strcpy(filename, &path[1]);                                                                
        }else if (breakpath){             
                // Not root, make _Get_Inode do its job                                                    
                dirpath = (char *) malloc(breakpath - path + 1);
                memset(dirpath, '\0', breakpath - path + 1); 
                strncpy(dirpath, path, breakpath-path);
                printf("path is '%s' and breakpath is '%s', and breakpath - path is %i\n",            
                                path, breakpath, breakpath - path);
                printf("Getting inode for '%s'\n", dirpath);                                          
                status = Get_Inode((const char *) dirpath, returnNode);
                if( status )              
                {        
                        printf("ERROR: .\n");                                  
                        return status;    
                }        
                if (strlen(&breakpath[1])> FILE_NAME_LENGTH){
                        return -EBADF; // Error bad file descriptor                                        
                }        
                strcpy(filename, &breakpath[1]);  
        }else{           
                // path contains no '/' and is thus invalid
                return -ENOENT;   
        }

        return status;   
}

DirEntry *Get_Dir(Inode *dirNode, int *numfiles){
	// Return the directory file for the given directory inode
	int status;

	DirEntry *dir;
	dir = (DirEntry *) malloc(dirNode->filesize);

	status = File_Read( dirNode, 0, dirNode->filesize, dir );
	if( status ){
		debug_print("Error in _get_Dir.\n");
		return (DirEntry *) NULL;
	}

	*numfiles = dirNode->filesize / sizeof(DirEntry);
	return dir;
}

int Get_New_Ino(){
	// Used for creating a file.  Returns an unused inumber.
	int inum, status;

	// Check for an available inumber
	inum = 0;

	while(inum < ifile_length && ifile[inum].mode != 0){
		printf("Mode for inode %i is %i.\n", inum, ifile[inum].mode);
		inum++;
	}

	if(inum == ifile_length){
		// no existing inodes are available, expand ifile
		status = Expand_Ifile(inum + 1 - ifile_length);
		if( status )
		{
			printf("error in expand");
			return status;
		}
	}

	printf("Returning inum %i.  Ifile length is %i.\n", inum, ifile_length);
	return inum;
}
