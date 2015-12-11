// Brandon Lee 11164610
// Des Marks 11352911
// 12/9/15
// CptS 360
// Final Project

#include "util.h"

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  read(fd, buf, BLOCK_SIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  write(fd, buf, BLOCK_SIZE);
}

/*********************************************************/
/* Bit manipulation functions                            */
/*********************************************************/

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

/*********************************************************/
/* End of Bit manipulation functions                     */
/*********************************************************/

/****************************************************************/
/* Allocation and Deallocation of BMAP and IMAP                 */
/****************************************************************/
int decFreeBlock(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int incFreeBlock(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}

int decFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int balloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeBlock(dev);

       put_block(dev, bmap, buf);

       return i+1;
    }
  }
  printf("balloc(): no more free blocks\n");
  return 0;
}

int dballoc(int dev, int bno)
{
  char buf[BLOCK_SIZE];
  bno--;

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  clr_bit(buf, bno);
  incFreeBlock(dev);

  put_block(dev, bmap, buf);
}

int ialloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int dialloc(int dev, int ino)
{
  char buf[BLOCK_SIZE];
  ino--;

  // read inode_bitmap block
  get_block(dev, imap, buf);

  clr_bit(buf,ino);
  incFreeInodes(dev);

  put_block(dev, imap, buf);
}

/****************************************************************/
/* End of Allocation and Deallocation of BMAP and IMAP          */
/****************************************************************/

MINODE *iget(int dev, int ino)
{
  int block_number;
  int la_number;
  int inodeBeginBlock;
  char buf[BLOCK_SIZE];
  int i;

  for(i = 0; i < NMINODE; i++)
  {
      if(minode[i].ino == ino)
      {
        minode[i].refCount++;
        return &minode[i];
      }

      if(minode[i].refCount == 0)
      {
        break;
      }
  }

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  inodeBeginBlock = gp->bg_inode_table;

  block_number = (ino - 1) / 8 + inodeBeginBlock;
  la_number = (ino - 1) % 8;

  get_block(dev, block_number, buf);
  ip = (INODE *)buf + la_number;

  minode[i].refCount = 1;
  minode[i].dINODE = *ip;
  minode[i].dev = dev;
  minode[i].ino = ino;

  return &minode[i];
}

int iput(MINODE *mip)
{
  char buf[BLOCK_SIZE];
  int block_number;
  int la_number;
  int inodeBeginBlock;

  mip->refCount--;

  if(mip->refCount > 0)
  {
    return 1;
  }

  if(mip->dirty == 0)
  {
    return 0;
  }else{
    get_block(mip->dev, 2, buf);
    gp = (GD *)buf;
    inodeBeginBlock = gp->bg_inode_table;

    block_number = (mip->ino - 1) / 8 + inodeBeginBlock;
    la_number = (mip->ino - 1) % 8;

    get_block(mip->dev, block_number, buf);
    ip = (INODE *) buf + la_number;

    *ip = mip->dINODE;
    put_block(mip->dev, block_number, buf);
  }
}

// mount level 3, may change device, need to implement later.
int getino(int *dev, char *lpathname)
{
  char buf[BLOCK_SIZE];
  char *token;
  char *pathnames[32];  // This stores the pathnames tokened into an array
  char temp_pathname[256];
  int name_count = 0;
  int inodenumber, inodeBeginBlock, i;
  INODE *temp_ip;

  strcpy(temp_pathname, lpathname);

  get_block(*dev, 2, buf);

  gp = (GD *)buf;
  inodeBeginBlock = gp->bg_inode_table;
  printf("inodeBeginBlock = %d\n", inodeBeginBlock);

  if((lpathname[0] == '/' && strlen(lpathname) == 1) || lpathname[0] == '\0')
  {
    pathnames[name_count] = ".";
  }else{
    token = strtok(temp_pathname, "/");  //tokenizes given pathname
    pathnames[name_count] = token;  //stores token into local array
    while(NULL != (token = strtok(NULL, "/")))
    {
      name_count++;
      pathnames[name_count] = token;
    }
  }

  // This determines if absolute or relative path
  if(lpathname[0] == '/')
  {
    get_block(*dev, inodeBeginBlock, buf);
    temp_ip = (INODE *)buf + 1;
  }else{
    int block = (running->cwd->ino - 1) / 8 + inodeBeginBlock;
    int la = (running->cwd->ino - 1) % 8;
    get_block(*dev, block, buf);
    temp_ip = (INODE *)buf + la;
  }

  // Loops through the pathname array, breaks out if name cannot be found
  for(i = 0; i <= name_count; i++)
  {
    int block_number;
    int la_number;

    inodenumber = search(*dev, pathnames[i], temp_ip);
    putchar('\n');
    if(0 == inodenumber)  // This will exit if a pathname was incorrectly given.
    {
      return 0;
    }

    //mailman's algorithm on inodenumber
    block_number = (inodenumber - 1) / 8 + inodeBeginBlock;
    la_number = (inodenumber - 1) % 8;

    //reassign INODE *ip to new inode
    get_block(*dev, block_number, buf);
    temp_ip = (INODE *)buf + la_number;

    // This check ensures that all pathnames given that are not the last
    // are directories and not files.
    if(S_ISREG(temp_ip->i_mode) && i < name_count)
    {
      printf("The pathname is invalid. This is not a directory = %s\n", pathnames[i]);
      return 0;
    }
  }
  return inodenumber;
}


// This looks through data blocks of inode structure
// This function is for getino to locate the correct inodenumber.
int search(int dev, char *name, INODE *temp_ip)
{
  char buf[BLOCK_SIZE];
  DIR *dp = (DIR *) buf;
  char *cp = buf;
  int i;

  printf("name = %s length = %d\n", name, strlen(name));

  for(i = 0; i < 12; i++)
  {
    if(temp_ip->i_block[i] == 0) //This means no more data blocks
    {
      return 0;
    }else{
      get_block(dev, temp_ip->i_block[i], buf); // get the data block at i_block[i] location and read through it
      //printf("inode #\trec_len\tname_len\tname\n");
      while(cp < buf + BLOCK_SIZE)
      {
        dp->name[dp->name_len] = '\0'; //null terminates name of dir entry

        printf("%d\t%d\t%d\t\t%s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);  //prints out traversal

        if(0 == strcmp(name, dp->name))
        {
          return dp->inode;
        }

        cp += dp->rec_len;
        dp = (DIR *) cp;
      }
    }
  }
  return 0;
}
