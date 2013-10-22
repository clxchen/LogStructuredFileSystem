
/*  ---------------- in file.h--------------------------------

//contains direct block info in disk
typedef struct Block_pointer
{
    //------- file bk size in sectors--------
    //------actually in flash memory: 1 sector is its 1 block-------
    logAddress bk_log_addr;

}Block_pointer;


typedef struct Inode
{
    u_int ino;
   
//    u_int seg_no_in_log;//this inode is stored in which seg of log
//    u_int bk_no_in_log;//this inode is stored in which block of log
    u_int filetype;
    u_int filesize;
    char filename[FILE_NAME_LENGTH + 1]; //phase 1
    
    //flash address of the inode's blocks
    Block_pointer direct_bk[DIRECT_BK_NUM];
    //Bk_list indirect_bk;
    //Block_pointer indirect_bk;  phase 2
   
    int mode;
    mode_t mode;
    uid_t userID;
    gid_t groupID;
    time_t modify_Time;
    time_t access_Time;
    time_t create_Time;
    time_t change_Time;
    int num_links;

}Inode;

//store the inode 
typedef struct Inode_location
{
    Inode inode;
    //offset in "ifile",
    // there stores the inode for ino
    u_int offset; 
    struct Inode_location * next;

}Inode_location;

--------------------------------------------------------------*/
/*------------- in directory.h-------------------------------------
//Ifile stores the inode_location of each inode
//in file "ifile"
typedef struct Ifile
{
    char * name;
    Inode_location * inode_loc;
}Ifile;

-----------------------------------------------------------------*/


