#include "t.h"

int main(int argc, char *argv[], char *env[]){
    dev = "mydisk";

    /********
     * We need to:
     * 1. Parse the into from the argv
     ********/

    if (argc <= 2){                             //No path was provided; invalid input
        printf("Usage: showblock /path pathname");
        exit(0);
    }

    dev = argv[1];                              //Three args were passed so we want to set the dev, path and split the path
    path = argv[2];
    parsePath(argv[2], pathItems);

    printf("n = %d", argsCount);                //Print out the number of path items and each item in the path
    int i;
    for (i = 0; i < argsCount; i++){
        printf("     %s", pathItems[i]);
    }
    putchar('\n');

    fd = open(dev, O_RDONLY);                   //Open the dev (directory) for reading
    if (fd < 0){                               //File open failed. Print message and exit.
        printf("%s open failed; Exiting program.\n", dev);
        exit(0);
    }

    printf("************  super block info: *************\n");
    super();

    printf("************  group 0 info ******************\n");
    gd();

    printf("***********  root inode info ***************\n");
    rootNode();

    printf("*********** root dir entries ***************\n");

    int iNodeNum,
        blk_num = iblock,
        offset;

    //Print the block number where the inode blocks begin
    printf("block = %d\n", ip->i_block[0]);
    printDir(ip);
    //Wait for the user to continue
    printf("Press any key to continue...");
    getchar();

    get_block(fd, blk_num, buf);                            //Get the intial block
    ip = (INODE *)buf + 1;                                  //Cast the buffer to an inode pointer

    for (i = 0; i < argsCount; i++){                            //Search through the path for the current item
        printf("============================================\n");
        printf("block = %d\n", ip->i_block[0]);
        printf("i = %d name[%d] = %s\n", i, i, pathItems[i]);
        printf("Searching for %s...\n", pathItems[i]);
        iNodeNum = search(ip, pathItems[i]);                    //Call search function

        if (iNodeNum == 0){                                     //If the value isn't found
            printf("File path not found.\n");
            break;
        }
        else{                                                   //If the value is found
            printf("Found %s at Inode #%d\n", pathItems[i], iNodeNum);
            printf("Group = %d Inode = %d\n", ip->i_gid, iNodeNum - 1);

            blk_num = ((iNodeNum - 1)/8) + iblock;              //Mailman's algorithm
            offset = (iNodeNum - 1) % 8;

            get_block(fd, blk_num, buf);                       //Get the next block
            ip = (INODE *)buf + offset;

            printf("Block = %d Offset = %d\n", blk_num, offset);
        }
    }

    if (iNodeNum != 0) {
        printf("\n\nsize = %d\n", ip->i_size);
        printf("****************  DISK BLOCKS  *******************\n");
        printDiskBlocks();
        printDirect();
        printIndirect();
        printDoubleIndirect();
    }
}