// Brandon Lee 11164610
// Des Marks 11352911
// 12/9/15
// CptS 360
// Final Project

#include "util.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
MOUNT  mounttab[5];

char names[64][128],*name[64];
int fd, dev, n;
int nblocks, ninodes, bmap, imap, inode_start, iblock;
int nfreeInodes, nfreeBlocks;
char pathname[256], parameter[256], cmd[256];
char rbuf[BLOCK_SIZE];

//int mode;
int DEBUG;

char *disk = "mydisk"; //default ext2_fs to be opened and made root

char *command_name[] = {"ls", "cd", "mkdir", "rmdir", "creat", "unlink", "link", "symlink",
                        "pwd", "stat", "chmod", "touch", "quit", "exception", "open", "pfd",
                        "close", "lseek", "read", "cat", "write", "cp"};

int init();
int mount_root();
super(int fd);
group(int fd);

int ls();
int ls_print_details(int ino, int dev, char *fname);
int cd();

int make_dir();
int mymkdir(MINODE *pip, char *name, int pino);
int enter_name(MINODE *pip, int ino, char *name);
int creat_file();
int mycreate(MINODE *pip, char *name, int type);

int rm_dir();
int rm_child(MINODE *parent, char *name);
int check_empty(MINODE *mip, int dev);
int unlink_file();

int link_file();
int symlink_fd();

int pwd();
int recurseThroughDirectory(int ino, int dev);
int findmyname(int myino, int mydev, int pino);

int stat_fd();
int chmod_fd();
int touch();
int quit();
int exception();
int menu();

int open_fd();
int close_fd();
int pfd();
int lseek_fd();

int read_fd();
int cat_fd();
int write_nfd();
int cp_fd();

void (*fptr[22])(void);
int findCommand(char *cmdname);

int main(int argc, char *argv[])
{
  char input[BLOCK_SIZE];
  char *token;

  init();
  mount_root();
  menu();

  while(1)
  {
    int command_num;

    printf("****************************************\n");

    printf("\ninput: ");
    fgets(input,512,stdin);
    input[strlen(input) - 1] = 0;

    sscanf(input, "%s %s %64c", cmd, pathname, parameter);
    printf("command = %s\n", cmd);
    printf("pathname = %s\n", pathname);
    printf("parameter = %s\n", parameter);


    command_num = findCommand(cmd);
    (*fptr[command_num])();

    memset(pathname, '\0', 256);
    memset(cmd, '\0', 256);
    memset(parameter, '\0', 256);
    memset(input, '\0', BLOCK_SIZE);
  }
  return 0;
}

int menu()
{
  printf("----------------------------------------\n");
  printf("Available commands:\n");
  printf("ls\tcd\tmkdir\trmdir\tcreat\n");
  printf("unlink\tlink\tsymlink\tpwd\tstat\n");
  printf("chmod\ttouch\tquit\n");
  printf("----------------------------------------\n");
}

int quit()
{
  int i;

  for(i = 0; i < NMINODE; i++)
  {
    minode[i].refCount = 1;
    iput(&minode[i]);
  }

  printf("exiting program\n");
  exit(1);
}

int exception()
{
  printf("\nThe command you entered was not valid.\n");
  printf("Please enter a command from the menu\n");
  menu();
}

// initializes file system
int init()
{
  int i;
  int j;
  for(i = 0; i < NPROC; i++)
  {
    proc[i].cwd = 0;
    proc[i].uid = i;
    proc[i].pid = i+1;

    for(j = 0; j < 10; j++)
    {
      proc[i].fd[j] = 0;
    }
  }
  for(i = 0; i < NMINODE; i++)
  {
    minode[i].refCount = 0;
    minode[i].dirty = 0;
    minode[i].mounted = 0;
    minode[i].mountptr = 0;
  }
  root = 0;

  fptr[0] = (void *)ls;
  fptr[1] = (void *)cd;
  fptr[2] = (void *)make_dir;
  fptr[3] = (void *)rm_dir;
  fptr[4] = (void *)creat_file;
  fptr[5] = (void *)unlink_file;
  fptr[6] = (void *)link_file;
  fptr[7] = (void *)symlink_fd;
  fptr[8] = (void *)pwd;
  fptr[9] = (void *)stat_fd;
  fptr[10] = (void *)chmod_fd;
  fptr[11] = (void *)touch;
  fptr[12] = (void *) quit;
  fptr[13] = (void *) exception;
  fptr[14] = (void *) open_fd;
  fptr[15] = (void *) pfd;
  fptr[16] = (void *) close_fd;
  fptr[17] = (void *) lseek_fd;
  fptr[18] = (void *) read_fd;
  fptr[19] = (void *) cat_fd;
  fptr[20] = (void *) write_nfd;
  fptr[21] = (void *) cp_fd;
}

int mount_root()
{
    dev = open(disk , O_RDWR);
    super(dev);
    group(dev);

    root = iget(dev, 2);

    printf("dev = %d\nino = %d\n", root->dev, root->ino);
    printf("size = %d\n", (root->dINODE).i_size);

    proc[0].cwd = iget(dev, 2);
    proc[1].cwd = iget(dev, 2);

    running = &proc[0];
}

// Looks through the command array until it finda match;
int findCommand(char *input)
{
  int i;
  for(i = 0; i < 22; i++)
  {
    if(0 == strcmp(command_name[i], input))
    {
        return i;
    }
  }
  return 13;
}

// This function prints out SUPER block information. (FROM LAB 6)
super(int fd)
{
  char buf[BLOCK_SIZE];

  // read SUPER block
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  printf("Super Block information\n");

  // check for EXT2 magic number:
  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  nfreeInodes = sp->s_free_inodes_count;
  nfreeBlocks = sp->s_free_blocks_count;

  printf("EXT2 FS OK\n");

  printf("s_inodes_count = %d\n", sp->s_inodes_count);
  printf("s_blocks_count = %d\n", sp->s_blocks_count);

  printf("s_free_inodes_count = %d\n", sp->s_free_inodes_count);
  printf("s_free_blocks_count = %d\n", sp->s_free_blocks_count);
  printf("s_first_data_blcok = %d\n", sp->s_first_data_block);

  printf("s_log_block_size = %d\n", sp->s_log_block_size);
  printf("s_blocks_per_group = %d\n", sp->s_blocks_per_group);
  printf("s_inodes_per_group = %d\n", sp->s_inodes_per_group);


  printf("s_mnt_count = %d\n", sp->s_mnt_count);
  printf("s_max_mnt_count = %d\n", sp->s_max_mnt_count);

  printf("s_magic = %x\n", sp->s_magic);

  printf("s_mtime = %s", ctime(&sp->s_mtime));
  printf("s_wtime = %s", ctime(&sp->s_wtime));
}

// This function prints out the GROUP block information (FROM LAB 6)
group(int fd)
{
  char buf[BLOCK_SIZE];

  get_block(fd, 2, buf);
  gp = (GD *)buf;

  imap = gp->bg_inode_bitmap;
  bmap = gp->bg_block_bitmap;

  printf("\nGroup Descriptor Block information\n");

  printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
  printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);
  printf("bg_inode_table = %d\n", gp->bg_inode_table);

  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
  printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);

}
/************************************************************************/
/* Functions for file system                                            */
/************************************************************************/
// Lists the files out
int ls()
{
  int i;
  int ino, dev = running->cwd->dev;
  MINODE *mip = running->cwd;

  if(pathname)
  {
    if(pathname[0] == '/')
    {
      dev = root->dev;
    }

    ino = getino(&dev, pathname);
    mip = iget(dev, ino);
    printf("dev = %d ino = %d\n", dev, ino);
  }

  // Prints out info in direct blocks
  for(i = 0; i < 12; i++)
  {
    char buf[BLOCK_SIZE];
    DIR *dp = (DIR *) buf;
    char *cp = buf;
    char c;

    if(mip->dINODE.i_block[i] == 0) //This means no more data blocks
    {
      iput(mip);
      return 0;
    }else{
      get_block(dev, mip->dINODE.i_block[i], buf);
      printf("inode #\trec_len\tname_len\tname\n");
      while(cp < buf + BLOCK_SIZE)
      {
        char c = dp->name[dp->name_len];
        dp->name[dp->name_len] = 0;

        printf("%d\t%d\t%d\t\t%s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);  //prints out traversal

        ls_print_details(dp->inode, dev, dp->name);
        dp->name[dp->name_len] = c;

        cp += dp->rec_len;
        dp = (DIR *) cp;
      }
    }
  }
  return 0;
}

// List the details of the file (equivalent of 'ls -l')
int ls_print_details(int ino, int dev, char *fname)
{
  int r, i;
  char ftime[64], buf[BLOCK_SIZE];
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  MINODE *mip = iget(dev, ino);

  //Print file type for the mode (_ or d or l)
  if ((mip->dINODE.i_mode & 0xF000) == 0x8000)
     printf("%c",'-');
  if ((mip->dINODE.i_mode & 0xF000) == 0x4000)
     printf("%c",'d');
  if ((mip->dINODE.i_mode & 0xF000) == 0xA000)
     printf("%c",'l');

  //Print the rest of the bits
  for (i = 8; i >= 0; i--)
  {
    if (mip->dINODE.i_mode & (1 << i))
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
  }

  printf("%4d ",mip->dINODE.i_links_count);
  printf("%4d ",mip->dINODE.i_gid);
  printf("%4d ",mip->dINODE.i_uid);
  printf("%8d ",mip->dINODE.i_size);

  // print time
  strcpy(ftime, ctime(&mip->dINODE.i_mtime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s  ",ftime);

  // print -> linkname if it's a symbolic file
  if ((mip->dINODE.i_mode & 0xF000) == 0xA000){
    //readlink(fname, buf, 64);
    char symbuf[84];
    char *tcp = symbuf;
    char *cp = buf;
    get_block(mip->dev, mip->dINODE.i_block[0], buf);

    while(cp < buf + strlen(fname))
    {
      *tcp++ = *cp++;
    }
    symbuf[strlen(fname)] = 0;
    printf("%s -> %s", fname, symbuf);
  }else{
    // print name
    printf("%s", fname);
  }
  printf("\n");

  iput(mip);
}

int cd()
{
  int ino, dev = running->cwd->dev;

  //If the no pathname is provided go to the root
  //Otherwise use getino to get the inodenum of the pathname provided
  if(pathname[0] == '\0')
  {
    running->cwd = root;
    printf("Changing directory to root.\n");
  }else{
    printf("dev = %d pathname = %s\n", dev, pathname);
    ino = getino(&dev, pathname);
    printf("ino = %d\n", ino);

    if(0 == ino) // This is to check if the pathname given was valid.
    {
      printf("Invalid directory path\n");
      return 0;
    }

    // This checks that when changing directory, it puts back the directory.
    iput(running->cwd);

    running->cwd = iget(dev, ino);
    printf("Changing directory to given pathname.\n");
  }
}

/******** make_dir function ***************
 * 1. Check for a invalid pathname
 * 2. Check for relative or absolute
 * 3. Get the parent and basenames
 * 4. Get the inode of the parent (check for dir type and make sure the child doesn't exist)
 * 5. Call mymkdir to add new dir to parent
 * 6. Link count++, dirty=1, Update a_time
 ******************************************/
int make_dir()
{
  char temp_parent[256];
  char temp_child[256];
  char *parent, *child;

  int i;
  int pino, dev;
  MINODE *mip, *pip;

  //Checks for an invalid pathname
  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to make directory\n");
    return 0;
  }

  //Checks for an absolute or relative path
  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  //Split up the parent and basenames
  strcpy(temp_parent, pathname);
  strcpy(temp_child, pathname);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname = %s\n", parent);
  printf("child pathname = %s\n", child);

  //Get the inode number for the parent (1. In CWD | 2. Root directory (i.e. /usr) | 3. A longer pathname /usr/tmp/newfile )
  if(0 == strcmp(parent, "."))
  {
    pino = running->cwd->ino;
  }else if(0 == strcmp(parent, "/") && 1 == strlen(parent))
  {
    pino = root->ino;
  }else{
    pino = getino(&dev, parent);
  }

  printf("device = %d, parent inode # = %d\n", dev, pino);

  //Invalid pathname
  if(pino == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  //Load the
  pip = iget(dev, pino);

  //Verify parent inode is a directory
  if(!(S_ISDIR(pip->dINODE.i_mode)))
  {
    printf("Pathname = %s is invalid, not a directory\n", pathname);
    iput(pip);
    return 0;
  }

  //Verify child does not exist in parent directory
  if(0 != search(dev, child, &pip->dINODE))
  {
    printf("Child name = %s exists in parent directory\n", child);
    iput(pip);
    return 0;
  }

  //Creates the child entries and the
  mymkdir(pip, child, pino);

  //The new dir is a new link for the parent so we need to increment it
  //Also update the access time of the parent and set the minode to dirty
  pip->dINODE.i_links_count++;
  pip->dINODE.i_atime = time(0L);
  pip->dirty = 1;

  iput(pip);
}

/******** mymkdir functions ***************
 * 1. Get new block and inode number
 * 2. Intialize the new inode for the directory
 * 3. Add . and ..
 * 4. Call entername to add dir to parent
 ******************************************/
int mymkdir(MINODE *pip, char *name, int pino)
{
  int ino, bno, i;
  MINODE *mip;
  INODE *ip;
  char buf[BLOCK_SIZE];

  //Get a new inode and block number for the new dir
  ino = ialloc(pip->dev);
  bno = balloc(pip->dev);
  printf("ino = %d\n", ino);
  printf("bno = %d\n", bno);

  //Get the new inode in memory
  mip = iget(pip->dev, ino);
  ip = &mip->dINODE;

  //Set directory attributes
  ip->i_mode = 0x41ED;
  ip->i_uid = running->uid;
  //ip->i_gid = running->gid; // add later
  ip->i_size = BLOCK_SIZE;
  ip->i_links_count = 2;
  ip->i_atime = time(0L);
  ip->i_ctime = time(0L);
  ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = bno;
  for(i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;
  }

  //Mark as dirty put the inode back in memory
  mip->dirty = 1;
  iput(mip);

  /************************************
   * 1. Read the new block into the buf
   * 2. Cast to a DIR *
   * 3. Set the inodenum, name, name_len and rec_len
   * 4. Move to the start of the next entry
   * 5. Set the inodenum, name, name_len
   * 6. And then set the rec_len = block_size - 12 = 1012
   *
   ************************************/
  //create directory entries for . and ..
  get_block(pip->dev, bno, buf);

  char *cp = buf;
  DIR *dp = (DIR *)cp;

  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  dp->name[0] = '.';

  cp += dp->rec_len;
  dp = (DIR *)cp;

  dp->inode = pino;
  dp->rec_len = 1012;
  dp->name_len = 2;
  dp->name[0] = '.';
  dp->name[1] = '.';

  //Put the block back after changes
  put_block(pip->dev, bno, buf);

  //Will add the actual directory to the parent
  enter_name(pip, ino, name);
}

/******** enter_name *******************
 * 1. Look through the direct blocks
 * 2. If we find one then add new dir
 * 3. Otherwise allocate a new block and add the dir
 ******************************************/
int enter_name(MINODE *pip, int ino, char *name)
{
  int i,j, bno;
  int need_length, ideal_length, remain;
  char buf[BLOCK_SIZE], c;

  DIR *dp;
  char *cp;

  //Look through all of the direct blocks until we find room
  for(i = 0; i < 12; i++)
  {
    //If we hit an empty block, break
    if(0 == pip->dINODE.i_block[i])
    {
      break;
    }

    //Read the current block into the buffer
    get_block(pip->dev,pip->dINODE.i_block[i], buf);

    dp = (DIR *)buf;
    cp = buf;

    //Go to the last entry
    while(cp + dp->rec_len < buf + BLOCK_SIZE)
    {
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    //Calculate the need, ideal, and remain lengths
    need_length = 4 * ( (8 + strlen(name) + 3) / 4); //need length of new entry
    ideal_length = 4 * ( (8 + dp->name_len + 3) / 4); //ideal length of last item in directory entries
    remain = dp->rec_len - ideal_length;

    //If there is enough room in the directory for the new entry
    if(remain >= need_length)
    {
      //Set the rec_len to the ideal length so we can go to the end of the entry
      dp->rec_len = ideal_length;

      cp += dp->rec_len;
      dp = (DIR *)cp;

      //Cut the extra length off the of then entries rec_Len
      dp->rec_len = remain;

      //Set the inode, name and name_len
      dp->inode = ino;
      dp->name_len = strlen(name);

      for(j = 0; j < strlen(name); j++) // no null byte
      {
        dp->name[j] = name[j];
      }

      //Write the block back
      put_block(pip->dev, pip->dINODE.i_block[i], buf);
      return 1;
    }
  }

  //If we need to allocate another block (NEW BLOCK)
  bno = balloc(pip->dev);
  pip->dINODE.i_size += BLOCK_SIZE;
  get_block(pip->dev, bno, buf);

  dp = (DIR *)buf;

  // No space in existing data blocks
  dp->inode = ino;
  dp->rec_len = BLOCK_SIZE;
  dp->name_len = strlen(name);
  for(j = 0; j < strlen(name); j++) // no null byte
  {
    dp->name[j] = name[j];
  }

  put_block(pip->dev, bno, buf);
}
/******** end of make directory functions ********/

/******** create file functions ******************/

/******** creat_file function ***************
 * 1. Check for a invalid pathname
 * 2. Check for relative or absolute
 * 3. Get the parent and basenames
 * 4. Get the inode of the parent (check for dir type and make sure the child doesn't exist)
 * 5. Call mycreat to add new file to parent
 * 6. dirty=1, Update a_time
 ******************************************/
int creat_file()
{
  char temp_pathname[256];
  char temp_parent[256], temp_child[256];
  char *parent, *child;

  int i;
  int pino, dev;
  MINODE *mip, *pip;

  strcpy(temp_pathname, pathname);

  //Check for no pathname
  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to make file\n");
    return 0;
  }

  //Check for absolute or relative path
  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n");
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  //Set the parent and child pathnames
  strcpy(temp_parent, temp_pathname);
  strcpy(temp_child, temp_pathname);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname = %s\n", parent);
  printf("child pathname = %s\n", child);

  //Get the parent inode numebr based of the parent name
  if(0 == strcmp(parent, "."))
  {
    pino = running->cwd->ino; //mip->ino
  }else{
    pino = getino(&dev, parent);
  }

  printf("device = %d, parent inode # = %d\n", dev, pino);
  //The inode number is 0 therefore we have an invalid path
  if(pino == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  //Load the parent inode
  pip = iget(dev, pino);

  //Verify parent inode is a directory
  if(!(S_ISDIR(pip->dINODE.i_mode)))
  {
    printf("Pathname = %s is invalid, not a directory\n", pathname);
    return 0;
  }

  //Verify child does not exist in parent directory
  if(0 != search(dev, child, &pip->dINODE))
  {
    printf("Child name = %s exists in parent directory\n", child);
    return 0;
  }

  //Call mycreate to add the new file as a child
  mycreate(pip, child, 1);

  pip->dINODE.i_atime = time(0L);
  pip->dirty = 1;

  iput(pip);
}

/******** creat_file function ***************
 * 1.  1. Get new block and inode number
 * 2. Intialize the new inode for the file or the link
 * 3. Call entername to add dir to parent
 ******************************************/
int mycreate(MINODE *pip, char *name, int type)
{
  int ino, bno, i;
  MINODE *mip;
  INODE *ip;
  char buf[BLOCK_SIZE];

  //Get the new block and inodenum
  //Typically we wouldn't need the block, but if we want to write to the file
  //we need it for storage
  ino = ialloc(pip->dev);
  printf("ino = %d\n", ino);

  //Get the minode and set the attributes
  mip = iget(pip->dev, ino);
  ip = &mip->dINODE;

  if(1 == type) //make a file (type = 1)
  {
    ip->i_mode = 0x81A4;
    ip->i_uid = running->uid;
    //ip->i_gid = running->gid; // add later
    ip->i_size = 0;
    ip->i_links_count = 1;
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 0;
    ip->i_block[0] = 0;
  }else{  //make a link (type = 2)
    ip->i_mode = 0xA000;
    ip->i_uid = running->uid;
    //ip->i_gid = running->gid; // add later
    ip->i_size = 0;
    ip->i_links_count = 1;
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);

    //copy each character over individually
    char tbuf[BLOCK_SIZE];
    char *cp = tbuf;
    int i = 0;
    get_block(mip->dev, ip->i_block[0], tbuf);
    while(cp < tbuf + strlen(pathname))
    {
      *cp = pathname[i];
      i++;
      cp++;
    }
    put_block(mip->dev, ip->i_block[0], tbuf);
  }
  mip->dirty = 1;
  iput(mip);

  //Add the new entry to the parent
  enter_name(pip, ino, name);
}
/******** end of create file functions ***********/

/******** remove directory and unlink functions **/

/******** rm_dir function ***************
 * 1. Check for a invalid pathname
 * 2. Check for relative or absolute
 * 3. Get the inode of the parent
 * 4. Check for the proper ownership and make sure it's not busy and is empty
 * 5. Deallocate the direct blocks and the inode
 * 6. Get the parent inode in to memory (check to make sure it's valid)
 * 7. Call rm_child()
 * 8. Dirty, decrement the parent's link count
 ******************************************/
int rm_dir()
{
  char temp_parent[256];
  char temp_child[256];
  char *parent, *child;

  int i;
  int ino, dev, pino;
  MINODE *mip, *pip;

  //Check for no pathname
  if(pathname[0] == '\0')
  {
    printf("Invalid remove directory, missing path\n");
    return 0;
  }

  //Check for the removal of the root directory
  if(strlen(pathname) == 1 && pathname[0] == '/')
  {
    printf("Cannot remove root directory\n");
    return 0;
  }

  //Check for an absolute path
  if(pathname[0] == '/')
  {
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{  //Relative path
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  //Get the inode number of the path
  ino = getino(&dev, pathname);

  //If the inodenumber is 0 then we have an invalid path
  if(0 == ino)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0;
  }else{  //Otherwise get minode
    mip = iget(dev, ino);
  }

  //check ownership
  if(mip->dINODE.i_uid != running->uid && 0 != running->uid)
  {
    printf("UID not a match or not Super user");
    iput(mip);
    return 0;
  }

  //check for directory type
  if(!S_ISDIR(mip->dINODE.i_mode))
  {
    printf("Not a directory\n");
    iput(mip);
    return 0;
  }

  //Check if busy
  if(mip->refCount > 1)
  {
    printf("Inode is still in use\n");
    iput(mip);
    return 0;
  }

  //Check if empty
  if(mip->dINODE.i_links_count > 2)
  {
    printf("Directory not empty\n");
    iput(mip);
    return 0;
  }

  //Check if empty (we will still have . and ..)
  if(mip->dINODE.i_links_count == 2)
  {
    if(check_empty(mip, mip->dev))
    {
      printf("Empty\n");
    }else{
      printf("Not empty\n");
      iput(mip);
      return 0;
    }
  }

  //Deallocate data blocks and inode
  for (i = 0; i < 12; i++)
  {
    if (0 == mip->dINODE.i_block[i])
    {
      continue;
    }
    dballoc(mip->dev, mip->dINODE.i_block[i]);
    printf("deallocating data block # = %d\n", mip->dINODE.i_block[i]);
  }
  dialloc(mip->dev, mip->ino);
  printf("deallocating inode # = %d\n", mip->ino);
  iput(mip);

  //Get parent inode into memory
  strcpy(temp_parent, pathname);
  strcpy(temp_child, pathname);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname = %s\n", parent);
  printf("child pathname = %s\n", child);

  if(0 == strcmp(parent, "."))
  {
    pino = running->cwd->ino;
  }else if(0 == strcmp(parent, "/") && 1 == strlen(parent))
  {
    pino = root->ino;
  }else{
    pino = getino(&dev, parent);
  }

  printf("device = %d, parent inode # = %d\n", dev, pino);
  if(pino == 0)
  {
    return 0; // This occurs when parent directory path does not exist.
  }

  pip = iget(dev, pino);

  rm_child(pip, child);

  pip->dINODE.i_links_count--;
  pip->dINODE.i_atime = time(0L);
  pip->dINODE.i_mtime = time(0L);
  pip->dirty = 1;

  iput(pip);
}

/******** rm_child function ***************
 * 1. Iterate through the direct blocks
 * 2. If we encounter an empty block we know there is no more
 * 3. getblock() the current block
 * 4. Iterate through the block
 *    a. First and only entry -
 *       -dealloc the block, set the block to 0, update BLKSIZE,
 *       -If there is data in one of the other blocks, we need to move the data up
 *        to avoid holes.
 *    c. Last entry -
 *       - Store the rec_len of the last entry in temp variable
 *       - Move the buffer back to the second to last entry
 *       - Add the reclen of the delete entry to the new last entry
 *    b. First to second to last entry
 *       -
 ******************************************/
int rm_child(MINODE *parent, char *name)
{
  int i;
  char buf[BLOCK_SIZE];
  DIR *dp;
  char *cp;
  int prev_rec_len = 0;

  for(i = 0; i < 12; i++)
  {
    if(0 == parent->dINODE.i_block[i])
    {
      return 0;
    }

    get_block(parent->dev,parent->dINODE.i_block[i], buf);
    dp = (DIR *) buf;
    cp = buf;

    while(cp < buf + BLOCK_SIZE)
    {
      char c;
      int j = i;
      int ideal_length;

      ideal_length = 4 * ((8 + dp->name_len + 3) / 4);

      c = dp->name[dp->name_len];
      dp->name[dp->name_len] = 0;

      if(0 == strcmp(dp->name, name))
      {
        if(dp->rec_len == BLOCK_SIZE) //first and only entry in data block
        {
          dballoc(parent->dev, parent->dINODE.i_block[j]);
          parent->dINODE.i_block[i] = 0;

          parent->dINODE.i_size -= BLOCK_SIZE;

          while(j + 1 < 12 && parent->dINODE.i_block[j + 1] != 0) //if still in direct blocks and next one is not 0
          {
            parent->dINODE.i_block[j] = parent->dINODE.i_block[j + 1];  //moves data blocks up
            j++;
          }

          printf("Removed an entry, first and only entry in a data block\n");
          return 1;
        }else if(dp->rec_len != ideal_length) //last item in directory entry, means there are at least 2 entries
        {
          int last_entry_len = dp->rec_len; //rec_len of last item
          cp -= prev_rec_len;   //move the next to last entry (new last)
          dp = (DIR *) cp;
          dp->rec_len += last_entry_len;    //Add the old last entries rec_len

          put_block(parent->dev,parent->dINODE.i_block[i], buf);  //Write back

          printf("Removed an entry, last entry in data block\n");

          return 2;
        }else{ //dp->rec_len == ideal_length //item is anything but last entry
          int add_rec_len = dp->rec_len;  //rec_len of item were removing
          char *np = cp + dp->rec_len;  //Get the next (to the left) items pointer
          int length = buf + BLOCK_SIZE - np; //

          char *tempcp = cp;

          //Got to the last entry
          while(cp + dp->rec_len < buf + BLOCK_SIZE)
          {
            cp += dp->rec_len;
            dp = (DIR *) cp;
          }

          //Add the deleted items rec_len to the end
          printf("add_rec_len %d\n", add_rec_len);
          dp->rec_len += add_rec_len;

          memcpy(tempcp, np, length);

          put_block(parent->dev,parent->dINODE.i_block[i], buf);

          printf("Removed an entry, entry was not last one or only one\n");

          return 3;
        }
      }
      dp->name[dp->name_len] = c;

      prev_rec_len = dp->rec_len;
      cp += dp->rec_len;
      dp = (DIR *) cp;
    }
  }
}

/******** check_empty function ***************
 * 1. Iterate through the direct blocks
 * 2. If there is a zero block we know it's empty; return!
 * 3. Otherwise look through the dirs in the block and make sure only . and .. exist
 * 4. If there are other names then there is still stuff in DIR
 ******************************************/
int check_empty(MINODE *mip, int dev)
{
  char buf[BLOCK_SIZE], c;
  DIR *dp;
  char *cp;
  int j;

  for(j = 0; j < 12; j++)
  {
    if(0 == mip->dINODE.i_block[j])
    {
        return 1;
    }
    get_block(dev, mip->dINODE.i_block[j], buf);
    dp = (DIR *) buf;
    cp = buf;

    while(cp < buf + BLOCK_SIZE)
    {
        c = dp->name[dp->name_len];
        dp->name[dp->name_len] = 0;
        if(0 != strcmp(dp->name, ".") && 0 != strcmp(dp->name, ".."))
        {
          return 0;
        }
        dp->name[dp->name_len] = c;

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
  }
}

int unlink_file()
{
  char temp_parent[256];
  char temp_child[256];
  char *parent, *child;

  int i;
  int ino, dev, pino;
  MINODE *mip, *pip;

  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to unlink file\n");
    return 0;
  }

  if(pathname[0] == '/')
  {
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  ino = getino(&dev, pathname);
  if(0 == ino)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0;
  }else{
    mip = iget(dev, ino);
  }

  if(S_ISDIR(mip->dINODE.i_mode))
  {
    printf("Cannot unlink a directory\n");
    return 0;
  }

  mip->dINODE.i_links_count--;
  // if(mip->dINODE.i_links_count > 0)
  // {
  //   printf("Link count is %d.\n", mip->dINODE.i_links_count);
  //   mip->dirty = 1;
  //   iput(mip);
  //   return 0;
  // }

  //dballoc(mip->dev, mip->dINODE.i_block[i]);
  dialloc(mip->dev, mip->ino);
  printf("deallocating inode # = %d\n", mip->ino);
  mip->dirty = 1;
  iput(mip);

  //Get parent inode into memory
  strcpy(temp_parent, pathname);
  strcpy(temp_child, pathname);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname = %s\n", parent);
  printf("child pathname = %s\n", child);

  if(0 == strcmp(parent, "."))
  {
    pino = running->cwd->ino;
  }else if(0 == strcmp(parent, "/") && 1 == strlen(parent))
  {
    pino = root->ino;
  }else{
    pino = getino(&dev, parent);
  }

  printf("device = %d, parent inode # = %d\n", dev, pino);
  if(pino == 0)
  {
    return 0; // This occurs when parent directory path does not exist.
  }

  pip = iget(dev, pino);

  rm_child(pip, child);

  pip->dINODE.i_atime = time(0L);
  pip->dINODE.i_mtime = time(0L);
  pip->dirty = 1;

  iput(pip);
}
/** end of remove directory and unlink functions */

/******** link and symlink ***********************/

/******** link_file function ***************
 * 1. Check for a invalid old pathname
 * 2. Check for relative or absolute old pathname
 * 3. Get the inode of the old pathname. Make sure it's a REG or a LNK
 * 4. Get the new pathname. Get parent and basename.
 * 5. Make sure parent is valid.
 * 6. Call entername with the old pathnames inode as an argument
 * 7. Update dirty, time and link count values on new and old files
 ******************************************/
int link_file()
{
  char *token;
  char oldfilepath[256];
  char newfilepath[256];

  int ino, odev, pino, ndev;
  MINODE *omip, *nmip, *npip;

  char *parent, *child;
  char temp_parent[256];
  char temp_child[256];

  strcpy(oldfilepath, pathname);
  if(oldfilepath[0] == '\0')
  {
    printf("No pathname given for old file\n");
    return 0;
  }

  strcpy(newfilepath, parameter);
  if(newfilepath[0] == '\0')
  {
    printf("No pathname given for new file\n");
    return 0;
  }

  if(oldfilepath[0] == '/')
  {
    omip = root;
    odev = root->dev;
    printf("root device of old pathname = %d\n", odev);
  }else{
    omip = running->cwd;
    odev = running->cwd->dev;
    printf("current device of old pathname = %d\n", odev);
  }

  ino = getino(&odev, oldfilepath);
  if(0 == ino)
  {
    printf("Invalid pathname given %s\n", oldfilepath);
    return 0;
  }
  omip = iget(odev, ino);

  // Verify oldfile path is a regular or link file
  if(!S_ISREG(omip->dINODE.i_mode) && !S_ISLNK(omip->dINODE.i_mode))
  {
    printf("Linking directory not allowed\n");
    iput(omip);
    return 0;
  }

  //Newfilepath go to parent directory
  strcpy(temp_parent, newfilepath);
  strcpy(temp_child, newfilepath);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname of new pathname = %s\n", parent);
  printf("child pathname of new pathname = %s\n", child);

  if(parent[0] == '/')
  {
    nmip = root;
    ndev = root->dev;
    printf("root device of new pathname = %d\n", ndev);
  }else{
    nmip = running->cwd;
    ndev = running->cwd->dev;
    printf("current device of new pathname = %d\n", ndev);
  }

  if(0 == strcmp(parent, "."))  //Creating file in CWD
  {
    pino = running->cwd->ino;
  }else if(0 == strcmp(parent, "/") && 1 == strlen(parent)) //Creating in root
  {
    pino = root->ino;
  }else{  //Need to find directory with getino
    pino = getino(&ndev, parent);
  }

  printf("device = %d, parent inode # = %d\n", ndev, pino);
  if(pino == 0) //Invalid path
  {
    printf("The pathname %s was invalid\n", pathname);
    iput(omip);
    return 0; // This occurs when parent directory path does not exist.
  }

  npip = iget(ndev, pino);  //get parent directory of newfilepath into memory

  //Verify parent inode is a directory
  if(!(S_ISDIR(npip->dINODE.i_mode)))
  {
    printf("Pathname = %s is invalid, not a directory\n", newfilepath);
    iput(omip);
    iput(npip);
    return 0;
  }

  //Verify child does not exist in parent directory
  if(0 != search(ndev, child, &npip->dINODE))
  {
    printf("Child name = %s exists in parent directory\n", child);
    iput(omip);
    iput(npip);
    return 0;
  }

  if(ndev != odev)
  {
    printf("Cannot link across different devices\n");
    iput(omip);
    iput(npip);
    return 0;
  }

  //Add the new link into the directory
  enter_name(npip, omip->ino, child);
  npip->dINODE.i_atime = time(0L);
  npip->dirty = 1;
  iput(npip);

  omip->dINODE.i_links_count++;
  omip->dINODE.i_atime = time(0L);
  omip->dirty = 1;
  iput(omip);
}

/******** symlink_fd function ***************
 * 1. Check for a invalid old pathname
 * 2. Check for relative or absolute old pathname
 * 3. Get the inode of the old pathname. Make sure it's a REG or a LNK
 * 4. Get the new pathname. Get parent and basename.
 * 5. Make sure parent is valid.
 * 6. Call mycreat
 ******************************************/
int symlink_fd()
{
  char *token;
  char oldfilepath[256];
  char newfilepath[256];

  int ino, odev, pino, ndev;
  MINODE *omip, *nmip, *npip;

  char *parent, *child;
  char temp_parent[256];
  char temp_child[256];

  strcpy(oldfilepath, pathname);
  if(oldfilepath[0] == '\0')
  {
    printf("No pathname given for old file\n");
    return 0;
  }

  strcpy(newfilepath, parameter);
  if(newfilepath[0] == '\0')
  {
    printf("No pathname given for new file\n");
    return 0;
  }

  if(oldfilepath[0] == '/')
  {
    omip = root;
    odev = root->dev;
    printf("root device = %d\n", odev);
  }else{
    omip = running->cwd;
    odev = running->cwd->dev;
    printf("current device = %d\n", odev);
  }

  ino = getino(&odev, oldfilepath);
  if(0 == ino)  //This checks if oldfilepath exists
  {
	printf("%s does not exist\n", oldfilepath);
    return 0;
  }
  omip = iget(odev, ino);

  if(!S_ISREG(omip->dINODE.i_mode) && !S_ISDIR(omip->dINODE.i_mode))
  {
    printf("Not a directory or file\n");
    iput(omip);
    return 0;
  }
  iput(omip);

  //Newfilepath go to parent directory
  strcpy(temp_parent, newfilepath);
  strcpy(temp_child, newfilepath);

  parent = dirname(temp_parent);
  child = basename(temp_child);

  printf("parent pathname of new pathname = %s\n", parent);
  printf("child pathname of new pathname = %s\n", child);

  if(parent[0] == '/')
  {
    nmip = root;
    ndev = root->dev;
    printf("root device of new pathname = %d\n", ndev);
  }else{
    nmip = running->cwd;
    ndev = running->cwd->dev;
    pino = running->cwd->ino;
    printf("current device of new pathname = %d\n", ndev);
  }

  if(0 == strcmp(parent, "."))
  {
  }else if(0 == strcmp(parent, "/") && 1 == strlen(parent))
  {
    pino = root->ino;
  }else{
    pino = getino(&ndev, parent);
  }

  printf("device = %d, parent inode # = %d\n", ndev, pino);
  if(pino == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  npip = iget(ndev, pino);  //get parent directory of newfilepath into memory

  //Verify parent inode is a directory
  if(!(S_ISDIR(npip->dINODE.i_mode)))
  {
    printf("Pathname = %s is invalid, not a directory\n", newfilepath);
    iput(npip);
    return 0;
  }

  //Verify child does not exist in parent directory
  if(0 != search(ndev, child, &npip->dINODE))
  {
    printf("Child name = %s exists in parent directory\n", child);
    iput(npip);
    return 0;
  }

  mycreate(npip, child, 2); //check name parameter

  npip->dINODE.i_atime = time(0L);
  npip->dirty = 1;

  iput(npip);
}
/******** end of link and symlink ****************/

/******* pwd**************************************/
int pwd() //should store pathname into a char[]
{
  int ino = running->cwd->ino;
  int dev = running->cwd->dev;

  if(2 == ino)
  {
    printf("/\n");
  }else{
    recurseThroughDirectory(ino, dev);
    printf("\n");
  }
}

/******** recurseThroughDirectory function ***************
 * 1. Given an inode numebr
 * 2. Loads inode into mip
 * 3. Moves forward 2 entries to the .. entrie
 * 4. Recursive call with the .. inode
 * 5. Print
 * 6. Return
 ******************************************/
int recurseThroughDirectory(int ino, int dev)
{
  int myino, pino;
  char buf[BLOCK_SIZE];

  DIR *dp = (DIR *) buf;
  char *cp = buf;
  MINODE *mip;

  if(2 == ino)
  {
    return 2;
  }

  mip = iget(dev, ino);
  get_block(dev, mip->dINODE.i_block[0], buf);

  myino = dp->inode; // ino for "."

  cp += dp->rec_len; //move to ino for ".."
  dp = (DIR *) cp;

  pino = dp->inode; // get parent inode #

  recurseThroughDirectory(pino, dev);

  findmyname(myino, dev, pino);

  iput(mip);
}


/******** link_file function ***************
 * -Works very similar to the search function.
 * -Prints the name once found
 ******************************************/
int findmyname(int myino, int mydev, int pino)
{
  char buf[BLOCK_SIZE];
  int i;
  DIR *dp = (DIR *) buf;
  char *cp = buf;

  MINODE *mip;

  mip = iget(mydev, pino);
  for(i = 0; i < 12; i++)
  {
    get_block(dev, mip->dINODE.i_block[i], buf);
    while(cp < buf + BLOCK_SIZE)
    {
      if(dp->inode == myino)
      {
        char c = dp->name[dp->name_len];
        dp->name[dp->name_len] = 0;
        printf("/%s", dp->name);
        c = dp->name[dp->name_len];
        return 1;
      }
      cp += dp->rec_len;
      dp = (DIR *) cp;
    }
  }
  return 0;
}
/*************************************************/
/****** chmod ************************************/
int chmod_fd()
{
  char buf[BLOCK_SIZE];

  int inodeNum, dev, i;
  MINODE *mip;

  //Check for a pathname
  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to file\n");
    return 0;
  }


  //Check for a valid permission
  if (strlen(parameter) <= 4) {
    //Checks the thousands place X000
    if ((atoi(parameter) / 1000) != 0){
      printf("Invalid permission\n");
      return;
    }

    //Checks the hundreds place 0X00
    if (((atoi(parameter) / 100) % 10) < 0 ||((atoi(parameter) / 100) % 10) > 7){
      printf("Invalid permission\n");
      return;
    }

    //Checks the tens place 00X0
    if (((atoi(parameter) % 100)/10) < 0 ||((atoi(parameter) % 100)/10) > 7){
      printf("Invalid permission\n");
      return;
    }

    //Checks the tens place 000X
    if ((atoi(parameter) % 10) < 0 ||(atoi(parameter) % 10) > 7){
      printf("Invalid permission\n");
      return;
    }
  }
  else{
    printf("Invalid permission\n");
    return;
  }

  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  inodeNum = getino(&dev, pathname);

  if(inodeNum == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  mip = iget(dev, inodeNum);

  if(mip->dINODE.i_uid != running->uid && 0 != running->uid){
    printf("You do not have the rights to change these file permissions\n");
    iput(mip);
    return;
  }

  int mask;
  int tempMode = strtol(parameter, &tempMode, 8);
  //61440 is the equivalent of 1111000000000000 in binary
  //The four leading 1's will mask the file type (d,l,_)
  mask = mip->dINODE.i_mode &= 61440;
  //Set the bits in the mask with the octal mode
  mask |= tempMode;

  //Set the new mode
  mip->dINODE.i_mode = mask;

  //Dirty because it has been modified
  mip->dirty = 1;

  iput(mip);
  return;
}
/*************************************************/
/******* touch ***********************************/
int touch()
{
  char buf[BLOCK_SIZE];

  int inodeNum, dev, i;
  MINODE *mip;

  //Check for a pathname
  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to file\n");
    return 0;
  }

  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  inodeNum = getino(&dev, pathname);

  if(inodeNum == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  mip = iget(dev, inodeNum);
  mip->dINODE.i_atime = time(0L);
  mip->dINODE.i_mtime = time(0L);
  mip->dirty = 1;
  iput(mip);
  return;
}
/*************************************************/
/******* stat ************************************/
int stat_fd()
{
  char buf[BLOCK_SIZE];
  char ftime[128];

  int inodeNum, dev, i;
  MINODE *mip;


  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to file\n");
    return 0;
  }

  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }


  inodeNum = getino(&dev, pathname);

  if(inodeNum == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  mip = iget(dev, inodeNum);

  if (S_ISDIR(mip->dINODE.i_mode))
    printf("Type: Directory\n");
  if (S_ISREG(mip->dINODE.i_mode))
    printf("Type: Regular File\n");
  if (S_ISLNK(mip->dINODE.i_mode))
    printf("Type: Link\n");

  printf("Size: %d\n", mip->dINODE.i_size);
  printf("Inode: %d\n", mip->ino);
  printf("Number of links: %d\n", mip->dINODE.i_links_count);
  printf("User ID: %d\n", mip->dINODE.i_uid);
  printf("Group ID: %d\n", mip->dINODE.i_gid);
  printf("Device: %d\n", mip->dev);
  printf("st_blocks: %d\n", mip->dINODE.i_blocks);
  strcpy(ftime, ctime(&mip->dINODE.i_atime));
  ftime[strlen(ftime) - 1] = 0;
  printf("Access time: %s\n", ftime);

  strcpy(ftime, ctime(&mip->dINODE.i_mtime));
  ftime[strlen(ftime) - 1] = 0;
  printf("Modified time: %s\n", ftime);

  strcpy(ftime, ctime(&mip->dINODE.i_ctime));
  ftime[strlen(ftime) - 1] = 0;
  printf("Created time: %s\n", ftime);

  iput(mip);
  return;
}
/*************************************************/
/******* open and close **************************/
/******** open_fd function ***************
 * 1. Check for a valid file and parameter
 * 2. Read the inode in and check for a REG file
 * 3. Check for permissions
 * 4. Create new OFT entry
 * 5. Set info (offset depends on the mode)
 * 6. Add to running->fd
 * 7. Mark as dirty
 ******************************************/
int open_fd(){
  char buf[BLOCK_SIZE];

  int inodeNum, dev, i, mode;
  MINODE *mip;

  //Check for a pathname
  if(pathname[0] == '\0')
  {
    printf("Missing argument, pathname to file\n");
    return 0;
  }

  mode = atoi(parameter);

  if (mode < 0 || mode > 3){
    printf("Invalid file mode\n");
    return 0;
  }

  if(pathname[0] == '/')
  {
    mip = root;
    dev = root->dev;
    printf("root device = %d\n", dev);
  }else{
    mip = running->cwd;
    dev = running->cwd->dev;
    printf("current device = %d\n", dev);
  }

  inodeNum = getino(&dev, pathname);

  if(inodeNum == 0)
  {
    printf("The pathname %s was invalid\n", pathname);
    return 0; // This occurs when parent directory path does not exist.
  }

  mip = iget(dev, inodeNum);

  //Make sure that it is regular
  if (!S_ISREG(mip->dINODE.i_mode)){
    printf("The pathname %s is not a regular file\n", pathname);
    iput(mip);
    return 0;
  }

  //Make sure you proper permissions
  if(mip->dINODE.i_uid != running->uid && 0 != running->uid){
    printf("You do not have the rights to change these file permissions\n");
    iput(mip);
    return;
  }


  //Creating a new oft entry for table
  OFT *newOFT = (OFT *)malloc(sizeof(OFT));
  newOFT->mode = mode;
  newOFT->inodeptr = mip;
  newOFT->refCount = 1;


  switch(mode){
    case 0 : newOFT->offset = 0;
          break;
    case 1 : truncate(mip);        // WR: truncate file to 0 size
          newOFT->offset = 0;
          break;
    case 2 : newOFT->offset = 0;     // RW does NOT truncate file
          break;
    case 3 : newOFT->offset =  mip->dINODE.i_size;  // APPEND mode
          break;
    default: printf("invalid mode\n");
          return(-1);
  }

  //Look for an empty process/spot
  for (i = 0; i < NFD; i++){
    if (running->fd[i] == 0){
      running->fd[i] = newOFT;
      break;
    }
  }

  mip->dirty = 1;
  return i;

}

/******** close_fd function ***************
 * 1. Check for valid file
 * 2. Check if there is a file at the location
 * 3. Set it to 0
 * 4. Put the minode back if ref cout == 0
 ******************************************/
int close_fd(){
  int closeFd = atoi(pathname);
  closeFd -= 1;

  //Check to make sure the file number is valid
  if (closeFd >= 0 && closeFd < 10) {
    if (running->fd[closeFd] != 0){
      OFT *temp = running->fd[closeFd];
      running->fd[closeFd] = 0;
      temp->refCount -= 1;
      if (temp->refCount > 0){ //is it still being used
        printf("File has been closed\n");
        return 1;
      }
      else{
        printf("File has been closed\n");
        iput(temp->inodeptr);
        return 1;
      }
    }
    else{
      printf("%d is an invalid file number\n", closeFd+1);
      return 0;
    }

  }
  else{
    printf("%d is an invalid file number\n", closeFd);
    return 0;
  }
}

/******** lseek function ***************
 * 1. Given an lseek location
 * 2. Check the value of the new offset
 * 3. Determine where they the marker should be placed
 ******************************************/
int lseek_fd(){
  int seekFd = atoi(pathname);
  int currOff, newOff = atoi(parameter);
  seekFd -= 1;

  if (seekFd >= 0 && seekFd < 10) {
    if (running->fd[seekFd] != 0){
      OFT *temp = running->fd[seekFd];
      currOff = running->fd[seekFd]->offset;
      printf("running->fd[seekFd]->inodeptr->dINODE.i_size: %d\n\n", running->fd[seekFd]->inodeptr->dINODE.i_size);
      if (newOff < 0){  //Beginning of file
        running->fd[seekFd]->offset = 0;
        printf("You are now at the start of the file\n");
      }
      else if (newOff > running->fd[seekFd]->inodeptr->dINODE.i_size){  //End of file
        running->fd[seekFd]->offset = newOff;
        printf("You are now at the end of the file\n");
      }
      else{ //Somewhere in the middle
        running->fd[seekFd]->offset = running->fd[seekFd]->inodeptr->dINODE.i_size;
        printf("You are now at the %d byte of the file\n", newOff);
      }
    }
    else{
      printf("%d is an invalid file number\n", seekFd+1);
      return 0;
    }

  }
  else{
    printf("%d is an invalid file number\n", seekFd+1);
    return 0;
  }
}

/******** truncate function ***************
 * 1. Check for a regular file
 * 2. dballoc eeach direct block (we didn't do direct)
 * 3. set inode size to 0
 ******************************************/
int truncate(MINODE *mip)
{
  /*1. release mip->INODE's data blocks;
  a file may have 12 direct blocks, 256 indirect blocks and 256*256
  double indirect data blocks. release them all.
  2. update INODE's time field
  3. set INODE's size to 0 and mark Minode[ ] dirty*/
  int i;
  unsigned int buf2[256], buf3[256];

  if (S_ISREG(mip->dINODE.i_mode)){
    for (i = 0; i < 12; i++){
      dballoc(mip->dev, mip->dINODE.i_block[i]);
    }
    if (mip->dINODE.i_block[12] != 0){
      get_block(mip->dev, mip->dINODE.i_block[12], (char *) buf2);
      for (i = 0; i < 256; i++){
        if(buf2[i] != 0)
        {
          dballoc(mip->dev, buf2[i]);
        }else{
          break;
        }
      }
    }
    if(mip->dINODE.i_block[13] != 0)
    {
      int j;
      get_block(mip->dev, mip->dINODE.i_block[13], (char *) buf2);
      for(i = 0; i < 256; i++)
      {
        if(buf2[i] != 0)
        {
          get_block(mip->dev, buf2[i], (char *) buf3);
          for(j = 0; j < 256; j++)
          {
            if(buf3[j] != 0)
            {
              dballoc(mip->dev, buf3[j]);
            }else{
              break;
            }
          }
        }else{
          break;
        }
      }
    }

    mip->dINODE.i_size = 0;
    mip->dirty = 1;

    return 1;
  }

  printf("You can't truncate a non regular file!\n");
  return 0;

}

/******** pfd function ***************
 * 1. Look through all the fd descriptors in running->fd
 * 2. Print out info if they exist
 ******************************************/
int pfd(){
  int i = 0;

  printf("\n#\tMODE\toffset\tdev #\tino #\n");
  for (i = 0; i < NFD; i++){
    if (running->fd[i] != 0){
      printf("%d.\t%d\t%d\t%d\t%d\n", i+1, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
    }
  }
}

/*************************************************/
/******* read and write **************************/
int read_fd(){
  //Get user input
  int fileNum = atoi(pathname);
  int nbytes = atoi(parameter);
  int count;

  fileNum -= 1;

  if (fileNum >= 0 && fileNum < 10){
    if (running->fd[fileNum] != 0){
      if (running->fd[fileNum]->mode == 2 || running->fd[fileNum]->mode == 0){
        count = my_read(fileNum, rbuf, nbytes);
        printf("myread: read %d char from file descriptor %d\n", count, fileNum);
      }
      else{
        printf("Invalid file mode\n");
      }
    }
  }
}

int my_read(int fileNum, char rbuf[], int nbytes)
{
  unsigned int buf2[256], buf3[256];
  char buf[BLOCK_SIZE];

  OFT *oftp = running->fd[fileNum];
  MINODE *mip = oftp->inodeptr;

  int count = 0, readIn = BLOCK_SIZE, remain;

  int avil = mip->dINODE.i_size - oftp->offset; // number of bytes still available in file.
  char *cq = rbuf;                // cq points at buf[ ]

  int blk, lbk, indirBlk, startByte;

  while (nbytes && avil) {
    cq = rbuf;  //Resets cq to the start of rbuf

    lbk = oftp->offset / BLOCK_SIZE;        //Get block number (similar to mailmans)
    startByte = oftp->offset % BLOCK_SIZE;  //Get the proper byte


    // I only show how to read DIRECT BLOCKS. YOU do INDIRECT and D_INDIRECT
    if (lbk < 12) {                     // lbk is a direct block
      blk = mip->dINODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
    }
    else if (lbk >= 12 && lbk < 256 + 12) {
      get_block(mip->dev, mip->dINODE.i_block[12], (char *) buf2);
      blk = buf2[lbk - 12];
    }
    else {
      //Offset is 256 + 12 = 268; Calculating the double indirect blknum in indirect blk we need this

      get_block(mip->dev, mip->dINODE.i_block[13], (char *) buf2);

      indirBlk = (lbk - 268) / 256; //Get double indirect block number

      get_block(mip->dev, buf2[indirBlk], (char *) buf3);

      blk = buf3[(lbk - 268) % 256];
    }

    /* get the data block into buf[BLKSIZE] */
    get_block(mip->dev, blk, buf);

    /* copy from startByte to buf[ ], at most remain bytes in this block */
    char *cp = buf + startByte;

    remain = BLOCK_SIZE - startByte;   // number of bytes remain in tbuf[]

    while (remain > 0) {
      if (avil <= remain)     //Checks if the max number of bytes to be read
        readIn = avil;
      else
        readIn = remain;

      memcpy(cq, cp, readIn); //Copy cp (buf) into cq (rbuf)

      oftp->offset += readIn;           // advance offset
      count += readIn;                  // inc count as number of bytes read
      avil -= readIn;
      nbytes -= readIn;
      remain -= readIn;

      if (nbytes <= 0 || avil <= 0)
        break;
    }
  }

  return count;   // count is the actual number of bytes read
}

int write_nfd()
{
  int fileNum = atoi(pathname);
  int nbytes;
  int i;
  int count = 0;

  fileNum -= 1;

  if(fileNum >= 0 && fileNum < 10)
  {
    if(running->fd[fileNum]->mode != 0)
    {
      char local_buf[BLOCK_SIZE];
      strcpy(local_buf, parameter);
      nbytes = strlen(local_buf);

      count = my_write(fileNum, local_buf, nbytes);
    }else{
      printf("File mode is not open for writing\n");
      return 0;
    }
  }
  printf("wrote %d char into file descriptor fd=%d\n", count, fileNum + 1);
}

int my_write(int fileNum, char *buf, int nbytes)
{
  int lbk;
  int blk;
  int startByte;

  int count = 0;
  int remain;

  unsigned int buf2[256], buf3[256];

  OFT *oftp = running->fd[fileNum];
  MINODE *mip = oftp->inodeptr;

  char *cq = buf;

  while(nbytes > 0)
  {
    cq = buf;

    lbk = oftp->offset / BLOCK_SIZE;        //Get block number (similar to mailmans)
    startByte = oftp->offset % BLOCK_SIZE;  //Get the proper byte


    if (lbk < 12)   // direct block
    {
      if (mip->dINODE.i_block[lbk] == 0)    // if no data block yet
      {
        mip->dINODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, mip->dINODE.i_block[lbk], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, mip->dINODE.i_block[lbk], tempbuf);  // but MUST for I or D blocks
      }
      blk = mip->dINODE.i_block[lbk];      // blk should be a disk block now
    }
    else if (lbk >= 12 && lbk < 256 + 12) //************************* INDIRECT ***********************//
    {
      int tempblk;
      if (mip->dINODE.i_block[12] == 0)
      {
        mip->dINODE.i_block[12] = balloc(mip->dev);

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, mip->dINODE.i_block[12], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, mip->dINODE.i_block[12], tempbuf);  // but MUST for I or D blocks
      }
      tempblk = mip->dINODE.i_block[12];      //contains block number for indirect blocks

      get_block(mip->dev, tempblk, (char *)buf2);  //gets block of INDIRECT blocks
      if(buf2[lbk - 12] == 0)
      {
        buf2[lbk - 12] = balloc(mip->dev);                //allocates a new data block number

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, buf2[lbk - 12], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, buf2[lbk - 12], tempbuf);  // but MUST for I or D blocks
      }
      blk = buf2[lbk - 12];
      put_block(mip->dev, tempblk, (char *)buf2);  //puts back block with new INDIRECT block
    }else{ //********************************** DoubleINDIRECT *********************************//
      int tempblk, indirectBlk, doubleBlk;
      if (mip->dINODE.i_block[13] == 0)
      {
        mip->dINODE.i_block[13] = balloc(mip->dev);

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, mip->dINODE.i_block[13], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, mip->dINODE.i_block[13], tempbuf);  // but MUST for I or D blocks
      }
      tempblk = mip->dINODE.i_block[13];

      get_block(mip->dev, tempblk, (char *) buf2);
      indirectBlk = (lbk - 268) / 256;
      if(buf2[indirectBlk] == 0)
      {
        buf2[indirectBlk] = balloc(mip->dev);

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, buf2[indirectBlk], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, buf2[indirectBlk], tempbuf);  // but MUST for I or D blocks
      }
      put_block(mip->dev, tempblk, (char *) buf2);

      get_block(mip->dev, buf2[indirectBlk], (char *) buf3);
      doubleBlk = (lbk - 268) % 256;
      if(buf3[doubleBlk] == 0)
      {
        buf3[doubleBlk] = balloc(mip->dev);

        char tempbuf[BLOCK_SIZE];
        get_block(mip->dev, buf3[doubleBlk], tempbuf);  // write a block of 0's to blk on disk: OPTIONAL for data block
        memset(tempbuf, '\0', BLOCK_SIZE);
        put_block(mip->dev, buf3[doubleBlk], tempbuf);  // but MUST for I or D blocks
      }
      blk = buf3[doubleBlk];
      put_block(mip->dev, buf2[indirectBlk], (char *) buf3);
    }

    char wbuf[BLOCK_SIZE];

    /* all cases come to here : write to the data block */
    get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]

    char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
    remain = BLOCK_SIZE - startByte;     // number of BYTEs remain in this block
    int writeout;

    while (remain > 0){               // write as much as remain allows
      if(nbytes < BLOCK_SIZE)
      {
        writeout = nbytes;
      }else{
        writeout = BLOCK_SIZE;
      }

      memcpy(cp, cq, writeout);

      count += writeout;
      nbytes -= writeout;
      remain -= writeout;                   // dec counts
      oftp->offset += writeout;             // advance offset
      if (oftp->offset > mip->dINODE.i_size)// especially for RW|APPEND mode
        mip->dINODE.i_size += writeout;     // inc file size (if offset > fileSize)
      if (nbytes <= 0) break;               // if already nbytes, break
    }
    put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
  }

  mip->dirty = 1;       // mark mip dirty for iput()
  return count;
}

int cp_fd()
{
  int fd = atoi(pathname) - 1;

  int gd = atoi(parameter) - 1;

  int n;
  while( n = my_read(fd, rbuf, 1024)){
    if (n < BLOCK_SIZE)
      break;
    printf("rbuf = %s\n", rbuf);
    my_write(gd, rbuf, n);  // notice the n in write()
    memset(rbuf, 0, BLOCK_SIZE);
  }
  printf("%s", rbuf);
  my_write(gd, rbuf, n);
}

int cat_fd(){
  char mybuf[BLOCK_SIZE], dummy = 0;
  int n, fd, count = 0;

  fd = open_fd();

  while (n = my_read(fd, rbuf, 1024)){
    rbuf[n] = 0; //Terminate the buf
    count += n;
    if (n < 1024)
      break;
    printf("%s", rbuf);
    memset(rbuf, 0, BLOCK_SIZE);
  }
  printf("%s", rbuf);

  printf("\nmyread: read %d char from file descriptor %d\n", count, fd);

  sprintf(pathname, "%d", fd+1);
  close_fd();
}
