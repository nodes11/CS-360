/********************
* Des Marks         *
* CS360             *
* September 9, 2015 *
* Lab 2             *
********************/

/****************************************************************************
* mkdir  pathname  : make a new directiry for the pathname                  *
* rmdir  pathname  : rm the directory, if it is empty.                      *
* cd    [pathname] : change CWD to pathname, or to / if no pathname.        *
* ls    [pathname] : list the directory contents of pathname or CWD         *
* pwd              : print the (absolute) pathname of CWD                   *
* creat  pathname  : create a FILE node.                                    *
* rm     pathname  : rm a FILE node.                                        *
* save   filename  : save the current file system tree in a file            *
* reload filename  : re-initalize the file system tree from a file          *
* quit             : save the file system tree, then terminate the program. *
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Define our NODE structure*/
typedef struct node{
	char name[64];
	char type[2];
	struct node *parentPtr;
	struct node *siblingPtr;
	struct node *childPtr;
}NODE;

/*Global variables*/
NODE *root, *cwd;	//root and current working directory pointers
char line[128];	//user input
char command[16],	//command name and path name
	pathname[64];
char dirname[64],	//dir name and base name
	basename[64];

/*Create the starting structure of the dir structure*/
void initialize(){
	root = (NODE *)malloc(sizeof(NODE));	//allocate memory

	if (root){	//if root exists, intialize the variables
		strcpy(root->name,"/\0");
		strcpy(root->type,"D\0");
		root->parentPtr = 0;
		root->childPtr = 0;
		root->siblingPtr=0;
		cwd = root;
	}
}

/*Creates a directory node*/
NODE *makeDir(char* name){
	NODE *new = (NODE *)malloc(sizeof(NODE));	//allocate memory

	if (new){	//if root exists, intialize the variables
		strcpy(new->name, name);
		strcpy(new->type,"D\0");
		new->parentPtr = 0;
		new->childPtr = 0;
		new->siblingPtr = 0;
	}

	return new;
}

/*Creates a file node*/
NODE *makeFile(char* name){
	NODE *new = (NODE *)malloc(sizeof(NODE));	//allocate memory

	if (new){	//if root exists, intialize the variables
		strcpy(new->name,name);
		strcpy(new->type,"F\0");
		new->parentPtr = 0;
		new->childPtr = 0;
		new->siblingPtr = 0;
	}

	return new;
}

/*Places a directory into the tree*/
void placeChild(NODE* new){
	NODE *parent = new;	//Will use for assigning parnets

	/*
		The function is passed the parent of where the directory belongs. It first checks if it needs to look through
		parents children in order to place. Otherwise we will just make it the parent's immediate child.
	*/

	if (new->childPtr){	//If there are children to look through
		new = new->childPtr;
		while (new->siblingPtr){	//looping
			new = new->siblingPtr;
		}
		new->siblingPtr = makeDir(basename);	//setting
		new->siblingPtr->parentPtr = parent;
	}
	else{	//No children
		new->childPtr = makeDir(basename);
		new->childPtr->parentPtr = parent;
	}

}

/*Removes a directory from the tree*/
void removeChild(NODE* new){
	NODE *parent = new;

	/*
		This function is passed the parent of the directory that we want to remove. It first checks if it needs to look through
		parents children in order to remove. Otherwise we will just remove the parent's immediate child.
	*/

	if (new->childPtr){	//If there are children
		parent = new;
		new = new->childPtr;

		while (new->siblingPtr){ //Loop through siblings, if we see where place, break.
			if ((strcmp(new->name, basename) == 0) && (strcmp(new->type, "D") == 0)){	//Comparing
				break;
			}
			parent = new;	//moving to next value
			new = new->siblingPtr;
		}

		if ((strcmp(new->type, "D") == 0)){	//Checking to make it's a directory type
			if (new->childPtr == NULL){
				if (parent->childPtr == new){	//If the child is the parent immediate child, remove it
					parent->childPtr = new->siblingPtr;
					if (new->siblingPtr)
						new->siblingPtr->parentPtr = parent;
				}
				else{	//If it's not the immediate child, remove it
					parent->siblingPtr = new->siblingPtr;
					new->siblingPtr->parentPtr = parent;
				}
			}
			else{
				printf("You cannot remove that directory becuase it has children.\n");
			}
		}
		else{	//Not a directory
			printf("%s is a file not a directory\n", new->name);
		}
	}
}

/*Places a file in a directory*/
void placeFile(NODE* new){
	NODE *parent = new;

	if (new->childPtr){	//If there are siblings to look through
		new = new->childPtr;
		while (new->siblingPtr){	//Looping
			new = new->siblingPtr;
		}
		new->siblingPtr = makeFile(basename);	//Placign
	}
	else{
		new->childPtr = makeFile(basename);	//Placed as the immediate child
	}
}

/*Removes a file from a directory*/
void removeFile(NODE* new){
	NODE *parent = new;

	/*
	This function is passed the parent of the directory that of the file we want to remove. It first checks if it needs to look through
	parents children in order to remove the file. Otherwise we will just remove the parent's immediate child.
	*/

	if (new->childPtr){	//Needs to look throught the children
		parent = new;	//parent will work
		new = new->childPtr;	//We're now looking at the siblings

		while (new->siblingPtr){	//Looping
			if ((strcmp(new->name, basename) == 0) && (strcmp(new->type, "F") == 0)){	//Check to make sure it's file
				break;
			}
			parent = new;	//Move the iterators
			new = new->siblingPtr;
		}
		if ((strcmp(new->type, "F") == 0)){	//If it's a file, remove it
			if (parent->childPtr == new){	//Removing if it's an immeditate child
				parent->childPtr = new->siblingPtr;
				if (new->siblingPtr)
					new->siblingPtr->parentPtr = parent;
			}
			else{	//Removing if it's not the immediate child
				parent->siblingPtr = new->siblingPtr;
				if (new->siblingPtr)
					new->siblingPtr->parentPtr = parent;
			}
		}
		else{	//Trying to remove a non file
			printf("%s is a directory not a file\n", new->name);
		}
	}
}

/*Returns the number of directories in a path*/
int getDirCount(char *pathname){
	char *s = pathname;
	int dirCount = 0;

	while(*s){
		if (*s++ == '/')
			dirCount++;
	}

	return dirCount;
}

/*Finds the parent of where we want to the place the node*/
NODE *findLocation(char *pathname){
	NODE *location = (NODE*)malloc(sizeof(NODE));	//Assuming we start at root
 	char *s = NULL;	//Used for strtok (spliting the path)
	int dirCount = 0;

 	/*Check for absolute or relative*/
	if (pathname[0] == '/'){
		location = root;
	}
	else
		location = cwd;

	dirCount = getDirCount(pathname); //Get the number of directories in the path

	/*if (dirCount == 0)
		strcpy(basename,pathname);*/

 	/*Start the tokenizing process*/
	s = strtok(pathname, "/");	//break up "a/b/c/d" into tokens

	if (location->childPtr){	//If the current location has a child pointer, go there
		location = location->childPtr;	//We are now at the child of the last parent

		while (s && dirCount > 1)	//While we have token and there is more than 1 directory to traverse, loop.
		{
			/*Loop through siblings while the names do not match and make sure they aren't files*/

			while (strcmp(location->name, s) != 0){	//While the names of the node name and the token don't match, loop.

				if (location->siblingPtr != NULL){	//If there is another sibling, go to them
					location = location->siblingPtr;
				}
				else{	//If there are no siblings and no matches
					return NULL;
				}

			}

			if (strcmp(location->type, "F") == 0){	//If the correct node is a file type
				return NULL;
			}

			if (location->childPtr != NULL){	//If the correct node has a child, go there
				s = strtok(0, "/");
				if (s == NULL)
					return location;
				else
					location = location->childPtr;
			}
			else{	//Otherwise we're in the correct spot
				return location;
			}
		}
	}
	else if (dirCount > 1){	//Catches junk directories
		printf("The directory does not exisit.\n");
		return NULL;
	}

	if (s == NULL)	//We've reach the end of the path. Return NULL.
	{
		return NULL;
	}


	if (location){	//If there is only a single directory to go into, but that directory has siblings
		while (location && (strcmp(location->name,"/") != 0)){
			if ((strcmp(location->name, s) == 0)){
				break;
			}
			location = location->siblingPtr;
		}
	}

	if (location && s && (strcmp(location->name, "/") != 0)){ //If were about to return a location but the token and node name aren't equal
		if ((strcmp(location->name, s) != 0))
			return 0;
	}

	if (location){	//Checks for a file type
		if (strcmp(location->type, "F") == 0)
		{
			return NULL;
		}
	}

	return location;	//We've found the correct item
}

/************************Operations*********************************/
void ls(char *pathname){
	NODE *temp = (NODE*)malloc(sizeof(NODE));
	char *p = NULL;

	//Split the pathname up
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname, '/');
		strcpy(basename, p + 1);
		strncpy(dirname, pathname, p - pathname + 1);
	}

	temp = findLocation(pathname);

	//If we are just looking the directory below
	if (temp == NULL){
		temp = cwd->childPtr;
		while (temp){
			printf("%s\t%s\n", temp->type, temp->name);
			temp = temp->siblingPtr;
		}
	}
	//if they just type 'ls'
	else if (strcmp(basename, "") == 0){
		while (temp){
			if (strcmp(temp->name, pathname) == 0){
				temp = temp->childPtr;
				while (temp){
					printf("%s\t%s\n", temp->type, temp->name);
					temp = temp->siblingPtr;
				}
			}

		}
	}
	//if t
	else if ((strcmp(basename, "") != 0)){
		temp = temp->childPtr;
		while (temp){
			if ((strcmp(temp->name, basename) == 0) && ((strcmp(temp->type, "F") != 0))){
				temp = temp->childPtr;
				while (temp){
					printf("%s\t%s\n", temp->type, temp->name);
					temp = temp->siblingPtr;
				}
				break;
			}
			else if (strcmp(temp->type, "F") == 0){
				printf("You cannot 'ls' a file.\n");
				break;
			}
			temp = temp->siblingPtr;
		}
	}

	else{
		temp = temp->childPtr;
		while (temp){
			printf("%s\t%s\n", temp->type, temp->name);
			temp = temp->siblingPtr;
		}
	}

}

/*Goes to selected directory*/
void cd(char *pathname){
	NODE* new = (NODE *)malloc(sizeof(NODE));
	char *p = NULL;

	//Split the pathname up
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname, '/');
		strcpy(basename, p + 1);
		strncpy(dirname, pathname, p - pathname + 1);
	}

	new = findLocation(pathname);

	/**If find location returned a value**/
	if (new && (strcmp(new->name, pathname) == 0) && (strcmp(basename,"") == 0)){	//If the first space is the correct one. ex: cd a
		cwd = new;
	}
	else if (new && pathname){	//If we have to look the children and siblings ex: cd a/b
		if (new->childPtr){
			new = new->childPtr;
			while (new){
				if ((strcmp(new->name, basename) == 0)){
					cwd = new;
					break;
				}
				else
					new = new->siblingPtr;
			}
			if (new == NULL){
				printf("The directory does not exist.");
			}
		}
		else{
			printf("The location does not exist.\n");
		}
	}
	else if (new == NULL && (strcmp(pathname,"") != 0))
	{
		printf("The directory '%s' does not exist.", p);
	}
	else{	//If 'cd' is entered we should go back to the top
		cwd = root;
	}

}

/*Creates a directory*/
void mkdir(char *pathname){
	NODE *temp = malloc(sizeof(NODE));
	char *p;

	/*Get Dirname and Basename*/
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname,'/');
		strcpy(basename, p+1);
		strncpy(dirname, pathname, p-pathname+1);

		/*Get location*/
		temp = findLocation(dirname);



		if (temp == NULL)
			printf("The directory '%s' does not exist or a file exists with that name.\n", pathname);
		else
			/*Place the new node*/
			placeChild(temp);

	}
	else{
		strcpy(basename, pathname);
		strcpy(dirname, "/");

		temp = cwd;

		placeChild(temp);
	}


}

/*Removes a directory*/
void rmdir(char *pathname){
	NODE *temp = (NODE*)malloc(sizeof(NODE));
	NODE* temp2;
	NODE* temp3;
	char *p = NULL;

	/*Get Dirname and Basename*/
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname, '/');
		strcpy(basename, p + 1);
		strncpy(dirname, pathname, p - pathname + 1);

		/*Get location*/
		temp = findLocation(dirname);



		if (temp == NULL)
			printf("The directory '%s' does not exist or a file exists with that name.\n", pathname);
		else
			/*Place the new node*/
			removeChild(temp);

	}
	else{
		strcpy(basename, pathname);
		strcpy(dirname, "/");

		temp = cwd;

		removeChild(temp);
	}

}

/*Prints the working directory*/
void pwd(char *pathname){
	rpwd(cwd);	//Call the recursive print working directory
	putchar('\n');
}

/*Traverses the tree*/
int rpwd(NODE *tnode){
	if (tnode->parentPtr){
		rpwd(tnode->parentPtr);	//Call with the parentPtr node
	}
	if (strcmp(tnode->name,"/") == 0)	//The root case
		printf("%s", tnode->name);
	else  //Printing all other directories
		printf("%s/", tnode->name);
	return 0;
}

/*Creates a file*/
void creat(char *pathname){
	NODE *temp = malloc(sizeof(NODE));
	char *p;

	/*Get Dirname and Basename*/
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname, '/');
		strcpy(basename, p + 1);
		strncpy(dirname, pathname, p - pathname + 1);

		/*Get location*/
		temp = findLocation(dirname);

		if (temp == NULL)
			printf("The directory '%s' does not exist or a file exists with that name.\n", pathname);
		else
			/*Place the new node*/
			placeFile(temp);

	}
	else{
		strcpy(basename, pathname);
		strcpy(dirname, "/");

		temp = cwd;

		placeFile(temp);
	}
}

/*Removes a file*/
void rm(char *pathname){
	NODE *temp = (NODE*)malloc(sizeof(NODE));
	NODE* temp2;
	NODE* temp3;
	char *p = NULL;

	/*Get Dirname and Basename*/
	if (strrchr(pathname, '/') != NULL){
		p = strrchr(pathname, '/');
		strcpy(basename, p + 1);
		strncpy(dirname, pathname, p - pathname + 1);

		/*Get location*/
		temp = findLocation(dirname);



		if (temp == NULL)
			printf("The directory '%s' does not exist or a file exists with that name.\n", pathname);
		else
			/*Place the new node*/
			removeFile(temp);

	}
	else{
		strcpy(basename, pathname);
		strcpy(dirname, "/");

		temp = cwd;

		removeFile(temp);
	}
}

/*Saves the file. Not implemented.*/
void save(char *pathname){
	printf("this is save\n");
}

/*Loads the file. Not implemented.*/
void reload(char *pathname){
	printf("this is reload\n");
}

/*Exits the program*/
void quit(char *pathname){
	printf("Now logging out...\n");
}

/*Function pointer array*/
void (*fptr[])(char *) = {(void (*)())mkdir, rmdir, ls, cd, pwd, creat, rm, save, reload, quit};

/**********************************Main******************************/

/***Checks the function strings against the input***/
int findCMD(char *cmd){
		if (strncmp(cmd, "mkdir", strlen("mkdir")) == 0)
			return 0;
		else if (strncmp(cmd, "rmdir", strlen("rmdir")) == 0)
			return 1;
		else if (strncmp(cmd, "ls", strlen("ls")) == 0)
			return 2;
		else if (strncmp(cmd, "cd", strlen("cd")) == 0)
			return 3;
		else if (strncmp(cmd, "pwd", strlen("pwd")) == 0)
			return 4;
		else if (strncmp(cmd, "creat", strlen("pwd")) == 0)
			return 5;
		else if (strncmp(cmd, "rm", strlen("rm")) == 0)
			return 6;
		else if (strncmp(cmd, "save", strlen("save")) == 0)
			return 7;
		else if (strncmp(cmd, "reload", strlen("reload")) == 0)
			return 8;
		else if (strncmp(cmd, "quit", strlen("quit")) == 0)
			return 9;
		return 100;
}

int main(int argc, char *argv[], char *env[]){
	int i;
	initialize();      /* initialize the / DIR of the tree */


	while (1){
		/*Set line, pathname and command all to null*/
		*line = '\0';
		*command = '\0';
		*pathname = '\0';
		*basename = '\0';
		*dirname = '\0';

		/*Get input from the user. Parse it into command and pathname.*/
		printf("Command: ");
		fgets(line, 128, stdin);
		sscanf(line, "%s %s", command, pathname);

		/*Find the correct command*/
		i = findCMD(command);

		if (i < 10 && i > -1){	//Call the function pointer
			fptr[i](pathname);
			if (i == 9)	//If they want to exit
				break;
		}
		else	//If the user inputs junk
			printf("Invalid command\n");
	}

	return 0;
}
