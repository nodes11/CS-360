/***********************
 * Des Marks
 * 10/26/15
 * CS 360
 * Lab #7
 **********************/

#include "type.h"

/****External Variables****/
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
MOUNT  mounttab[5];

char names[64][128],*name[64], tempCmd[128];
char pathname[256], parameter[256], buf[BLKSIZE], *devName = "mydisk";

int fd, dev, n, argsCount;
int nblocks, ninodes, bmap, imap, inode_start, iblock;

int DEBUG;
/*************************/

void init(){
    int i;                      /*Set the first two processs uid's to 0 and 1 and their cwd's to 0*/
    for (i = 0; i < 2; i++){
        proc[i].uid = i;
        proc[i].cwd = 0;
        proc[i].pid = i+1;
    }

    running = malloc(sizeof(PROC));
    running = &proc[0];

    i = 0;                      /*Set all of the MINODE's refCount's to 0*/
    for (i = 0; i < 100; i++){
        minode[i].refCount = 0;
        minode[i].ino = 0;
    }

    root = 0;                   /*Set the root MINODE*/
    printf("Inialization successful\n\n");
}
/***
 * Gets a block and reads its contents into the buffer
 ***/
int get_block(int fd, int blk, char buf[]) {
    lseek(fd, (long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}
/***
 * Writes the contents from a buffer to a block
 ***/
int put_block(int dev, int blk, char buf[]){
    lseek(fd, (long)blk*BLKSIZE, 0);
    write(fd, buf, BLKSIZE);
}
/***
 * Searches for a directory
 *
 * Steps:
 * 1. Get the block of the current inode number
 * 2. Cast the buffer (contains the block info) to an inode
 * 3. Use cp to iterate through the buffer
 * 4. If dp name and the name passed in match then return the node number
 * 5. Otherwise we want to cast move the dp and cp pointers forward.
 * 6. If nothing is found at the end of the buffer, return 0.
 ***/
int search(MINODE *mip, char *path){
    int i;
    char *cp;
    char *temp;

    //We only need to look through the direct blocks for now
    for (i = 0; i < 12; i++) {
        if (mip->INODE.i_block[i]) {
            get_block(dev,mip->INODE.i_block[i],buf); //Get the block for the starting inode block

            dp = (DIR *)buf; // access buf[] as DIR entries using a cast
            cp = buf;  //char pointer pointing at buf[ ]

            //Move through the block while the address of CP is not equal to the address at the end of the block
            while (cp < buf + BLKSIZE) {
                if (strcmp(dp->name, path) == 0) {   //If the names match, we found the directory
                    return dp->inode;
                }
                //OTHERWISE:
                cp += dp->rec_len;     //Move cp forward the length of the current entry to get to the next entry
                dp = (DIR *) cp;        // Cast cp to DIR
            }
        }
    }
    return 0;
}
int getino(MINODE *mip, char *pathname){
    int i,
        iNodeNum = 0,
        blk_num,
        offset;
    MINODE *tempMin = malloc(sizeof(MINODE));

    /*Check whether the path is relative or absolute (LAB 2)*/
    if (pathname[0] == '/'){    /*Absolute Path*/
        dev = root->dev;
        iNodeNum = root->ino;
    }
    else{                       /*Relative Path*/
        dev = running->cwd->dev;
        iNodeNum = root->ino;
    }

    /*Parse the pathname items*/
    parsePath(tempCmd);

    for (i = 0; i < argsCount; i++){
        iNodeNum = search(mip, names[i]);            //Search for the name in the current directory

        /*If the file isn't found in the current directory*/
        if (iNodeNum == 0) {
            printf("File not found!\n");
            return 0;
        }

        offset = (iNodeNum - 1) % 8;
        get_block(dev, ((iNodeNum - 1) / 8) + inode_start, buf);
        mip = (MINODE*)buf + offset;

        /*Put the inode back into memory so it can be used elsewhere*/
        iput(mip);
    }

    return iNodeNum;
}
MINODE *iget(int dev, int ino){
    MINODE *mip = malloc(sizeof(MINODE));
    int i,
        blk_num,
        offset;

    //Loop through all of the minodes to see if the one we want is in memory
    for (i = 0; i < NMINODE; i++) {
            if (minode[i].ino == ino) {     //Do the values match
                minode[i].refCount++;
                return &minode[i];
            }
    }

    //Use mailman's to get the block location
    blk_num = ((ino - 1)/8) + inode_start;
    offset = (ino - 1) % 8;
    //Get the block
    get_block(dev, blk_num, buf);
    ip = (INODE *)buf + offset;

    //Loop through all of the minodes until we find one that is empty
    for (i = 0; i < NMINODE; i++){
        if (minode[i].refCount == 0){           //Check to make sure it's empty
            /***
             * 1. Set the minode's inode to the temorary inode
             * 2. Set the dirty value to 0
             * 3. Set the device number (works like a fd)
             * 4. Set the inode number
             * 5. Increment the refcount
             ***/
            minode[i].INODE = *ip;
            minode[i].dirty = 0;
            minode[i].dev = dev;
            minode[i].ino = ino;
            minode[i].refCount = 1;

            //Return the index of the inode
            return &minode[i];
        }
    }
}
void iput(MINODE *mip){
    int blk_num,
        offset;
    mip->refCount -= 1;

    /*Make sure the minode isn't still being used by another process*/
    if (mip->refCount == 0){
        /* Use mailman's to get the block location on the minode*/
        blk_num = ((mip->ino - 1)/8) + inode_start;
        offset = (mip->ino- 1) % 8;

        /*Get the block*/
        get_block(mip->dev, blk_num, buf);

        /*Back up the contents of the block*/
        ip = (INODE *)buf + offset;
        memcpy(ip, &(mip->INODE), sizeof(INODE));

        /*Save the block back*/
        put_block(dev, blk_num, buf);
    }
}
/*split the env path into tokens*/
void parsePath(char *path){
    char *s;
    int i = 0;
    char temp[64];

    s = strtok(path, "/");
    strcpy(temp, s);
    strcpy(names[i++],temp);

    //Split up the rest of the items in the path. Add them to the array of parts
    while ((s = strtok(0, "/")))
    {
        strcpy(temp, s);
        strcpy(names[i++],temp);
    }

    argsCount = i;
}
/*mount the root*/
void mount_root(){
    dev = open("mydisk", O_RDWR);

    /*Check to see if the device was opened properly*/
    if (dev < 0){
        printf("ERROR: Device could not be open. Now Exiting...");
        exit(1);
    }

    /*Cast the buffer to a SUPERBLOCK for verification*/
    get_block(dev, SUPERBLOCK, buf);
    sp = (SUPER *)buf;

    nblocks = sp->s_blocks_count;
    ninodes = sp->s_inodes_count;

    /*Verifying the magic number to make sure it's an EXT2 FS*/
    if (sp->s_magic != 0xEF53){
        printf("NOT an EXT2 FS\n");
        exit(1);
    }

    get_block(dev, GDBLOCK, buf);
    gp = (GD *)buf;
    inode_start = gp->bg_inode_table;

    /*Otherwise we have a valid EXT2 FS*/
    root = iget(dev, ROOT_INODE);


    root->mountptr = (MOUNT *)malloc(sizeof(MOUNT));
    root->mountptr->ninodes = ninodes;
    root->mountptr->nblocks = nblocks;
    root->mountptr->dev = dev;
    root->mountptr->busy = 1;
    root->mountptr->mounted_inode = root;
    strcpy(root->mountptr->name,"/");
    strcpy(root->mountptr->mount_name,devName);

    /*Set the cwd of proc[0] and proc[1] to the root MINODE*/
    proc[0].cwd = iget(dev, 2);
    proc[1].cwd = iget(dev, 2);

    root->refCount = 3;

    printf("%s mounted successfully\n\n", devName);
}
/***
 * Prints the files in directory (NO SEARCH!)
 ***/
void printDir(INODE ptr){
    get_block(dev,ptr.i_block[0],buf);

    dp = (DIR *)buf; // access buf[] as DIR entries
    char *cp = buf;         //char pointer pointing at buf[ ]

    while(cp < buf + BLKSIZE){
        printf("%s\n", dp->name);
        cp += dp->rec_len;     //Move cp forward rec_len bytes. Cast to DIR
        dp = (DIR *)cp;
    }
}
void cd(char *path){
    int iNodeNum;
    MINODE *mip = running->cwd;
    dev = running->cwd->dev;

    if (path[0] != '\n' && path[0] != '\n'){
        printf("here");
        if (path[0] == '/')
            dev = root->dev;
        iNodeNum = getino(mip, path);

        printf("%d", iNodeNum);

        mip = iget(dev,iNodeNum);

        running->cwd = mip;
    }
    else{
        running->cwd = root;
    }
}
void ls(char *path){
    int ino,
        i = 0;
    char *cp;

    //Set the current minode and the device number
    MINODE *mip = running->cwd;
    dev = running->cwd->dev;

    /*If the path is not empty*/
    if (path[0] != '\n' && path[0] != '\0'){
        //Root case
        if (path[0]=='/')
            dev = root->dev;

        //Get the inode number for the path
        ino = getino(mip, path);

        if (ino == 0)
            return 0;

        mip = iget(dev, ino);
        if (mip == NULL){
            printf("Cannot ls a file.\n");
            return 0;
        }
        else {
            printDir(mip->INODE);
        }
    }
    else{
        printDir(running->cwd->INODE);
    }
}
int tst_bit(char *buf, int bit) {
    int i, j;
    i = bit/8; j=bit%8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}
int set_bit(char *buf, int bit) {
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] |= (1 << j);
}
int clr_bit(char *buf, int bit) {
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] &= ~(1 << j);
}
int decFreeInodes(int dev) {
    char buf[BLKSIZE];

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
int ialloc(int dev) {
    int  i;
    char buf[BLKSIZE];

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

int balloc(int dev) {
    int  i;
    char buf[BLKSIZE];

    // read block block
    get_block(dev, bmap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i)==0){
            set_bit(buf,i);
            decFreeInodes(dev);

            put_block(dev, bmap, buf);

            return i+1;
        }
    }
    printf("ialloc(): no more free inodes\n");
    return 0;
}

void quit(){
    int i = 0;

    for (i = 0; i < NMINODE; i++){
        minode[i].refCount = 0;
        iput(&minode[i]);
    }
}

int main(int argc, char *argv[]){
    printf("*****Initializing the filesytem******\n");
    init();
    printf("*****Mounting the filesytem******\n");
    mount_root();

    while(1) {
        fflush(stdin);
        printf("Enter a ls path: ");
        fgets(tempCmd, 128, stdin);
        if (tempCmd[0] != '\0' && tempCmd[0] != '\n')
            tempCmd[strlen(tempCmd) - 1] = 0;
        ls(tempCmd);

        fflush(stdin);

        printf("Enter a cd path: ");
        fgets(tempCmd, 128, stdin);
        if (tempCmd[0] != '\0' && tempCmd[0] != '\n')
            tempCmd[strlen(tempCmd) - 1] = 0;
        cd(tempCmd);
    }

    quit();

    return 0;
}