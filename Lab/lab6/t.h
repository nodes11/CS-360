/***********************
 * Des Marks
 * 10/26/15
 * CS 360
 * Lab #6
 **********************/

#ifndef LAB6
#define LAB6

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include <fcntl.h>
#include <time.h>

#define BLKSIZE 1024

typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

/*******globals*******/
int fd, iblock, argsCount;
char buf[BLKSIZE];
char *pathItems[32];
char *path, *dev;

/*******function headers*******/
int get_block(int fd, int blk, char buf[ ]);
void super();
int rootNode();
int gd();
int search(INODE *INodePtr, char *name);
void printDir(INODE *INodePtr);
void printDiskBlocks();
void printDirect();
void printIndirect();
void printDoubleIndirect();

#endif
