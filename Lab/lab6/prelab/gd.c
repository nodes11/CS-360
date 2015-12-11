/********* super.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

#define BLKSIZE 1024

typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

char buf[BLKSIZE];
int fd;
char *mydisk = "fsdisk";

int main(int argc, char *argv[], char *env[]){
    printf("snarf");
    if (argc > 1){                                  /*If the the user specified a disk name*/
        mydisk = argv[1];
    }

    fd = open(mydisk, O_RDONLY);                    /*Try opening the disk*/

    if (fd < 0)                                     /*The open failed*/
        exit(0);

    gd();
}

int get_block(int fd, int blk, char buf[ ])         /*Gets the block information*/
{
    lseek(fd, (long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int gd(){
    get_block(fd, 2, buf);                          /*Get the block info for gd0*/

    gp = (GD *)buf;                                 /*Cast buffer to a group descriptor*/

    printf("block bmap: %d\n", gp->bg_block_bitmap);
    printf("inode bmap: %d\n", gp->bg_inode_bitmap);
    printf("inode table: %d\n", gp->bg_inode_table);
    printf("free blocks: %d\n", gp->bg_free_blocks_count);
    printf("free inodes: %d\n", gp->bg_free_inodes_count);
    printf("used dirs: %d\n", gp->bg_used_dirs_count);

    return 0;
}
