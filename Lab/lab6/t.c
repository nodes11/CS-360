#include "t.h"

/***
 * Gets a block and reads its contents into the buffer
 ***/
int get_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

/***
 * Prints out the super block info
 ***/
void super() {

    // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    // check for EXT2 magic number:
    if (sp->s_magic != 0xEF53){
        printf("NOT an EXT2 FS\n");
        exit(1);
    }

    // Print the output in the same order as K.C.
    printf("inodes_count\t\t%d\n", sp->s_inodes_count);
    printf("blocks_count\t\t%d\n", sp->s_blocks_count);
    printf("r_blocks_count\t\t%d\n", sp->s_r_blocks_count);
    printf("free_inodes_count\t%d\n", sp->s_free_inodes_count);
    printf("free_blocks_count\t%d\n", sp->s_free_blocks_count);
    printf("log_block_size\t\t%d\n", sp->s_log_block_size);
    printf("first_data_blcok\t%d\n", sp->s_first_data_block);
    printf("magic number = %d\n", sp->s_magic);
    printf("rev_level\t\t%d\n", sp->s_rev_level);
    printf("inode_size\t\t%d\n", sp->s_inode_size);
    printf("block_group_nr\t\t%d\n", sp->s_block_group_nr);
    printf("blksize\t\t\t%d\n", BLKSIZE);
    printf("inodes_per_group\t%d\n", sp->s_inodes_per_group);
    printf("---------------------------------------------\n");
    printf("desc_per_group\t\t%d\n", sp->s_inode_size/4);
    printf("inodes_per_block\t%d\n",BLKSIZE/sp->s_inode_size);
    printf("inode_size_ratio\t%d\n",1);
}

/***
 * Prints out the gd0 info
 ***/
int gd(){
    get_block(fd, 2, buf);                          //Get the block info for gd0 (located at the index 2)

    gp = (GD *)buf;                                 //Cast buffer to a group descriptor

    //Print the output in the same order as K.C.
    printf("Blocks bitmap block\t%d\n", gp->bg_block_bitmap);
    printf("Inodes bitmap block\t%d\n", gp->bg_inode_bitmap);
    printf("Inodes table block\t%d\n", gp->bg_inode_table);
    printf("Free blocks count\t%d\n", gp->bg_free_blocks_count);
    printf("Free inodes count\t%d\n", gp->bg_free_inodes_count);
    printf("Directories count\t%d\n", gp->bg_used_dirs_count);
    printf("inodes_start\t\t%d\n", gp->bg_inode_table);

    return 0;
}

/***
 * Prints out the root node info
 ***/
int rootNode(){
    iblock = gp->bg_inode_table;                    //Get the block number for where the inodes begin
    get_block(fd, iblock, buf);                     //Get the actual block where the inodes begin
    ip = (INODE *)buf + 1;                          //Cast the block to an inode pointer

    printf("File mode\t\t%4x\n", ip->i_mode);       //Print the mode, size, and count
    printf("Size in bytes\t\t%d\n", ip->i_size);
    printf("Blocks count\t\t%d\n", ip->i_blocks);

    printf("Press any key to continue...");         //Wait for the user to continue
    getchar();

    printf("block[0] = %d\n", ip->i_block[0]);      // print the starting block

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
int search(INODE *INodePtr, char *name){
    get_block(fd,INodePtr->i_block[0],buf); //Get the block for the starting inode block

    dp = (DIR *)buf; // access buf[] as DIR entries using a cast
    char *cp = buf;  //char pointer pointing at buf[ ]

    //Move through the block while the address of CP is not equal to the address at the end of the block
    while(cp < buf + BLKSIZE){
        printf("%8d %16d %7d      %-s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
        if (strcmp(name, dp->name) == 0)    //If the names match, we found the directory
            return dp->inode;

        //OTHERWISE:
        cp += dp->rec_len;     //Move cp forward the length of the current entry to get to the next entry
        dp = (DIR *)cp;        // Cast cp to DIR
    }
    return 0;
}

/***
 * Prints the files in directory (NO SEARCH!)
 ***/
void printDir(INODE *INodePtr){
    get_block(fd,INodePtr->i_block[0],buf);

    dp = (DIR *)buf; // access buf[] as DIR entries
    char *cp = buf;         //char pointer pointing at buf[ ]

    while(cp < buf + BLKSIZE){
        printf("%8d %16d %7d      %-s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
        cp += dp->rec_len;     //Move cp forward rec_len bytes. Cast to DIR
        dp = (DIR *)cp;
    }
}

/***
 * Print all of the disk blocks
 ***/
void printDiskBlocks(){
    int i = 0;
    /****
     * 0-11: Direct
     * 12: Indirect
     * 13: Double Indirect
     * 14: Triple Indirect (not shown)
     ****/
    while (i < 14 && ip->i_block[i] != 0){
        printf("block[%d] = %d\n", i, ip->i_block[i]);
        i++;
    }

}

/***
 * Print all of the direct blocks
 ***/
void printDirect(){
    int i = 0;
    printf("================ Direct Blocks ===================\n");
    while (i < 12 && ip->i_block[i] != 0){
        printf("%d ", ip->i_block[i]);
        i++;
    }
    putchar('\n');
}

/***
 * Print all of the indirect blocks
 *                                   ___
 *                                  |0  |
 *                                  |1  |
 *          ip->i_block[13]  --->   |2  |
 *                                  |3  |
 *                                  |4  |
 *                                  |...|
 *                                  |256|
 *                                   ---
 *
 * i_block[13] points to a block which contains an array of 256 more blocks.
 ***/
void printIndirect(){
    int i = 0;
    u32 bbuf[BLKSIZE];  //Used store block information (it has to be unsigned or else everything goes wrong (spent 3 hours figure this out))

    if (ip->i_block[12] != 0) {
        printf("=============== Indirect Blocks ==================\n");
        get_block(fd, ip->i_block[12], (char *)bbuf);

        for (i = 0; i < 256; i++) {
            if (i % 10 == 0)
                putchar('\n');
            if (bbuf[i] > 0) {
                printf("%d   ", bbuf[i]);
            }
        }
        putchar('\n');
    }
}

/***
 * Print all of the indirect blocks
 *                                   ___       ___
 *                                  |0  | --> |0  |
 *                                  |1  |     |1  |
 *          ip->i_block[13]  --->   |2  |     |2  |
 *                                  |3  |     |3  |
 *                                  |4  |     |4  |
 *                                  |...|     |...|
 *                                  |256|     |256|
 *                                   ---       ---
 *
 * i_block[13] points to a block which contains an array of 256 more blocks and each of the those blocks point to
 * 256 more blocks.
 ***/
void printDoubleIndirect(){
    int i = 0,
        j = 0;
    u32 tbuf[BLKSIZE];      //Used store block information (it has to be unsigned or else everything goes wrong (spent 3 hours figure this out))
    u32 bbuf[BLKSIZE];      //Used store block information (it has to be unsigned or else everything goes wrong (spent 3 hours figure this out))

    if (ip->i_block[13]){
        printf("\n===========  Double Indirect blocks   ============");
        get_block(fd, ip->i_block[13],(char *)bbuf);    //Get the first set of indirect blocks

        for (i = 0; i < 256; i++){
            if (buf[i]){
                get_block(fd, bbuf[i], (char *)tbuf);   //Get the second set of indirect blocks
                for (j = 0; j < 256; j++){
                    if (tbuf[j] == 0){
                        break;
                    }
                    if (j % 10 == 0)
                        putchar('\n');
                    if (tbuf[j] > 0){
                        printf("%d   ", tbuf[j]);
                    }
                }
            }
        }
        putchar('\n');
    }
}

/***
 * Split the env path into tokens
 ***/
void parsePath(char *path, char *pathItems[]){
    char *s;
    int i = 0;

    s = strtok(path, "/");
    pathItems[i] = s;
    argsCount++;

    //Split up the rest of the items in the path. Add them to the array of parts
    while ((s = strtok(0, "/")))
    {
        ++argsCount;
        pathItems[++i] = s;
    }

    pathItems[++i] = NULL;                          //Set the last item to NULL to avoid seg faults
}



