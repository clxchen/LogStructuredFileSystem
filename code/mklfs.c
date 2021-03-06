#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "log.h"
#include <string.h>
/*  
  mklfs [options] file
    -l size, --segment=size
        Segment size, in blocks. The default is 32.
    -s sectors, --sectors=sectors
        Size of the flash, in sectors.  The default is 1024.
    -b size, --block=size
        Size of a block, in sectors. The default is 2 (1KB).
    -w limit, --wearlimit=limit
        Wear limit for erase blocks. The default is 1000.

*/

int main(int argc, char * argv[])
{
    //---------------default value----------------------
    wearlimit = 1000;
//    fl_file = (char *)calloc(1, 8);
 //   strcpy(fl_file, "File");         

    //----- sec_num & bks_per_seg should always be 16 的整数倍
    sec_num = 1024;
    bk_size = 2;
    bks_per_seg = 32;

    int ch;  
    while ((ch = getopt(argc, argv, "l:s:b:w:")) != -1)  
    {  
        switch (ch) {  
            case 'l': 
                if(atoi(optarg) >= 65536)
                {
                    printf("Too many blocks per segment!\n");
                    return 0;
                } 
                bks_per_seg = (u_int)atoi(optarg);
                break;  
            case 's':
                if(atoi(optarg) >= 65536)
                {
                    printf("Too big flash memory!\n");
                    return 0;
                }
                else
                { 
                    sec_num = (u_int)atoi(optarg);
                }
                break;  
            case 'b':
                bk_size = (u_int)atoi(optarg);
                break;  
            case 'w': 
                if(atoi(optarg) >= 65536)
                {
                    printf("Too big wearlimit!\n");
                }
                wearlimit = (u_int)atoi(optarg);
                break;  
            case '?':
                break;  
        }
    } 
    

    char * fn = (char *)calloc(1,8);
    strcpy(fn, argv[argc - 1]);

    char* bf = get_current_dir_name();
    char * s = (char *)calloc(1, 8);
    strcpy(s, fn);
    fl_file = (char *)calloc(1, strlen(bf) + strlen(s) + 2);
    strcpy(fl_file, bf);
    strcpy(fl_file + strlen(bf), "/");
    strcpy(fl_file + strlen(bf) + 1, s);
    free(fn);




    seg_size = bks_per_seg * bk_size;
    seg_num = sec_num / bk_size / bks_per_seg;
    bk_content_size = bk_size * FLASH_SECTOR_SIZE;
    
    if(seg_num < 16)
    {
        printf("Should be more than 16 segments!\n");
        return 0;
    }



    if(sec_num % FLASH_SECTORS_PER_BLOCK != 0)
    {
        printf("Total sectors must be 16 whole times!\n");
        return 0;
    }

    if(seg_size % FLASH_SECTORS_PER_BLOCK != 0)
    {
        printf("Seg size must be 16 whole times!\n");
        return 0;
    }

    if(seg_size > sec_num)
    {
        printf("Segment size exceed the entire flash memory!\n");
        return 0;
    }

    if(sec_num % bk_size != 0)
    {
        printf("Not whole number of blocks!\n");
        return 0;
    }

    if((sec_num / bk_size) % bks_per_seg != 0)
    {
        printf("Not whole number of segs!\n");
    }

    //---------------------------------------------------------------
    //create and format flash memory & create log in memory
    Log_Create();
    /*
    //------------store flash memory configuration variables--------
    char* buffer = get_current_dir_name();
    char * s = "/config.ini";
    char * config = (char *)calloc(1, strlen(buffer) + strlen(s));
    strcpy(config, buffer);
    strcpy(config + strlen(buffer), s);

    FILE *fp;
    if((fp=fopen(config,"wb")) == NULL) 
    { 
        printf("\nopen file error"); 
        exit(1); 
    }

    printf("dir: %s\n", config);

    char store_seg_size[5];
    sprintf(store_seg_size, "%d", seg_size);
    fputs(store_seg_size,fp);

    fputs("\n", fp);

    char store_sec_num[5];
    sprintf(store_sec_num, "%d", sec_num);
    fputs(store_sec_num,fp);

    fputs("\n", fp);

    fclose(fp); 
    */
}
