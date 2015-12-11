// Brandon Lee 11164610
// Des Marks 11352911
// 12/9/15
// CptS 360
// Final Project

#include <stdio.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // NOTE: Ubuntu users MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
//#include <time.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

#define BLOCK_SIZE     1024

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE    0040777
#define FILE_MODE   0100644
#define SUPER_MAGIC  0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2

// Table sizes
#define NMINODE         100
#define NMOUNT           10
#define NPROC            10
#define NFD              10
#define NOFT            100

// Open File Table
typedef struct oft{
  int   mode;
  int   refCount;
  struct minode *inodeptr;
  int   offset;
}OFT;

// PROC structure
typedef struct proc{
  int   uid;
  int   pid, gid;
  int   status;
  struct minode *cwd;
  OFT   *fd[NFD];
}PROC;

// In-memory inodes structure
typedef struct minode{
  INODE dINODE;               // disk inode
  int   dev, ino;
  int   refCount;
  int   dirty;
  int   mounted;
  struct mount *mountptr;
}MINODE;

// Mount Table structure
typedef struct mount{
  int    dev;
  int    nblocks,ninodes;
  int    bmap, imap, iblk;
  MINODE *mounted_inode;
  char   name[64];
  char   mount_name[64];
}MOUNT;

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern MOUNT  mounttab[5];

extern char names[64][128],*name[64];
extern int fd, dev, n;
extern int nblocks, ninodes, bmap, imap, inode_start, iblock;
extern int nfreeInodes, nfreeBlocks;
extern char pathname[256], parameter[256], cmd[256];

int get_block(int fd, int blk, char buf[ ]);
int put_block(int fd, int blk, char buf[ ]);

int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);

int decFreeBlock(int dev);
int incFreeBlock(int dev);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int balloc(int dev);
int dballoc(int dev, int ino);
int ialloc(int dev);
int dialloc(int dev, int ino);

int getino(int *dev, char *pathname);
MINODE *iget(int dev, int ino);
int iput(MINODE *mip);
int search(int dev, char *name, INODE *temp_ip);
