/**********
*Des Marks*
*11352911 *
*9/20/15  *
*CS 360   *
*Lab #3   *
**********/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


/**********************************
 * cmd  arg1 arg2 ...  <  infile    // take inputs from infile
 * cmd  arg1 arg2 ...  >  outfile   // send outputs to outfile
 * cmd  arg1 arg2 ...  >> outfile   // APPEND outputs to outfile
 * *******************************/



char *path,
	*pathdir,
	*home,
	*homedir,
	*head,
	*tail;
char *pathItems[32],
		*myargv[64];
char cwd[128],
	lineParts[64][64],
	cmd[128];
int argCount,
	cpipes,
	redirValue,
	redirIndex;

/*Returns the redirect value*/
void checkRedir(char *cmd){
	int i = 0;
	
	for (i = 0; cmd[i] != '\0'; i++){
		if (cmd[i] == '<'){		//Infile
			redirValue = 0;	//Set as 0, input
			redirIndex = i;	//Save index
			cmd[i] = '\0';	//Set index to null
			break;
		}
		else if ((cmd[i] == '>') && (cmd[i+1] == '>')){		//Outfile and append
			redirValue = 2;	//Set as 2, output and append
			redirIndex = i;	//Save index
			cmd[i] = '\0';	//Set index to null
			cmd[i+1] = '\0'; //Set index 2 to null
			break;
		}
		else if (cmd[i] == '>'){		//Outfile
			redirValue = 1;	//Set as 1, for output
			redirIndex = i;	//Save inex
			cmd[i] = '\0';	//Set index to null
			break;
		}
	}
}

/*split the env path into tokens*/
void parsePath(char *path, char *pathItems[]){
	char *s;
	int i = 0;
	
	s = strtok(pathdir, ":");
	pathItems[i] = s;
	
	//Split up the rest of the items in the path. Add them to the array of parts
	while ((s = strtok(0, ":")))
	{
		pathItems[++i] = s; 
	}
}

/*go through the env[] and find home and path values. SAVE THEM!!!!*/
void getPaths(char *env[]){
	while (*env)	//Go throught the environment array look for the PATH and the HOME
	{
		if (strstr(*env,"PATH=") && (strchr(*env, 'P') - *env == 0)){	//For PATH
			path = *env;
			pathdir = strrchr(*env,'=') + 1;	//Removes the "PATH="
		}
		if ((strstr(*env, "HOME=")) && (strchr(*env, 'H') - *env == 0)){	//For HOME
			home = *env;
			homedir = strrchr(*env,'=') + 1; //Removes the "HOME="
		}
		env++;
	}
}

/*Counts the number of pipes in the command*/
int countPipes(char *cmd){
	int i = 0;
	cpipes = 0;
	
	for (i = 0; cmd[i] != '\0'; i++)
	{
		if (cmd[i] == '|')
			cpipes++;
	}
	
	return cpipes;
}

/*Sets the head and tail values*/
void setHeadAndTail(char *cmd){
	head = strtok(cmd, "|");
	tail = strtok(0, "|");
}

/*Checks for a newline character and removes it if it exists*/
void checkForNewLine(char *myargv){
	int nl = 0;
	
	//Get to the end
	while (*myargv)
	{
		myargv++;
		nl = 1;
	}
	
	//Go back one, check, remove if so
	if (nl == 1){
		myargv--;
		if (*myargv == '\n')
			*myargv = 0;
	}
}

/*Split up the command and save it in myargv[]*/
/*1.Pass in command and myargv array
 *2.strok on spaces
 *3.save to line parts (char array)
 *4.save lineparts[i] to myargv[i] (char *[])
 *5.check for newlines
 *6.set last item to null (causes seg faults otherwise)
 */
void splitArgs(char *cmd, char *myargv[]){
	char *arg;
	int i = 0;
	
	arg = strtok(cmd, " ");
	
	strcpy(lineParts[i], arg);
	myargv[i] = lineParts[i];
	argCount++;
	
	//Break up the line into the seperate arguments, add it to the arg array
	while ((arg = strtok(0, " "))){
			i += 1;
			strcpy(lineParts[i], arg);
			myargv[i] = lineParts[i];
			checkForNewLine(myargv[i]); //Remove the newline if there is one, 
			argCount++;
	}
	
	//We NEED NULL terminator for multi-part commands
	myargv[++i] = NULL;
	
	//Make sure the command doesn't have new line at the end, call the doCommand()
	checkForNewLine(cmd);
}

/*Creates a child process*/
/*1.pass in environment and command
 *2.set the head and tail
 *3.forks child
 *4.checks if it needs to wait for child
 *5.if not,check the value of cpipes (greater than 0 we will do a pipe)
 *6.if no pipes do a normal command
 *7.return status to waiting parent
 * 
 * 
 * When it forks the child
 */
int makeChild(char *env[], char *cmd){
	int pid,
		childStatus;
	
	//Set the head and the tail
	setHeadAndTail(cmd);
		
	//Fork a child
	pid = fork();
	
	//Waiting on child
	if (pid){
		pid = wait(&childStatus);
	}
	else{	//Do Command
		if (cpipes > 0){	//If there is a pipe
			runPipe(env);
		}
		else{	//If it's single command
			doCommand(env, cmd);
		}
	}
	
	//Return to waiting parent
	return childStatus;
}

/*Execve takes a file directory, checks all system paths provided
 * by the env[] for the file. If it exists it will execute the system
 * command
 */

//concatenates the pathItem with command and uses the execve command
void runCMD(char *env[], char *cmd){
	int i = 0;
	//char *redirFile;
	
	if (redirValue != -1){
		if (redirValue == 0){
			close(0);    // system call to close file descriptor 0
            open("afile", O_RDONLY);  // open filename for READ, which
                                        // will replace fd 0
		}
		else if (redirValue == 1){
			close(1);
			open("afile", O_WRONLY|O_CREAT, 0644); 
		}
		else if (redirValue == 2){
			close(1);
			open("afile", O_WRONLY|O_CREAT|O_APPEND, 0644); 
		}
	}
	
	//We need to look through each pathItems and see where the command is stored
	for (i = 0; pathItems[i] != '\0'; i++){
		strcpy(cmd,pathItems[i]);	//cmd now has the path for directory of files
		strcat(cmd,"/");
		strcat(cmd, myargv[0]);
		printf("%d.\t%s\n", i+1, cmd);	//Print the directory where it is looking for the command file
		execve(cmd, myargv, env);	//Execute the command
	}
}

int doBasicCommand(char *myargv[]){	
	char *command = myargv[0];
	char *arg = myargv[1];
	
	checkForNewLine(command);	//Remove any newline characters
	
	if(strcmp(command, "exit") == 0)	//exit command
	{
		exit(1);
	}
	else if(strcmp(command, "cd") == 0)	//cd command
	{
		if (arg)	//If a path is given
			chdir(arg);
		else    //If no path is give
			chdir(homedir);
		return 1;
	}
	else if(strcmp(command, "pwd") == 0)	//pwd command
	{
		printf("%s\n", get_current_dir_name());
		return 1;
	}
	else
	{
		return 0;
	}
	
}

int doCommand(char *env, char *cmd){
	int childUpdate = 0;
	
	//Checks if it is a basic command. Forks child otherwise.
	if (doBasicCommand(myargv) == 0)
	{	
		runCMD(env, cmd);
	}
	
	return childUpdate;
}

void runPipe(char *env[]){

	int fileDesc[2];
	
	/******************************************************************/
	//File descriptor indexes
	//0 is standard input
	//1 is standard output
	//2 is standard error
	
	/*After the pipe(fileDesc) command pd[0] is wiil be open for reading
	  and pd[1] is will be open for writing*/
	/******************************************************************/
	
	pipe(fileDesc);
	//Print the head and tail values
	printf("Head: %s\nTail: %s\n", head, tail);
	
	//fork() returns 0 if it's on the child, the child ID if it's the parent process, and 
	//-1 if no process exists
	
	if (fork() == 0)	//Dealing with the child process
	{
		//FROM KC
		close(fileDesc[0]);// WRITER MUST close pd[0]
        close(1);    // close 1
        dup(fileDesc[1]);  // replace 1 with pd[1]
		
		//Split the arguments and remove and newline characters
		splitArgs(head, myargv);
		checkForNewLine(head);
		
		//Run the command
		runCMD(env, head);
	}
	else    //Dealing with the parent process
	{
		//FROM KC
		close(fileDesc[1]); // READER MUST close pd[1]
        close(0);  
        dup(fileDesc[0]);   // replace 0 with pd[0]
		
		//Split the arguments and remove any newline characters
		splitArgs(tail, myargv);
		checkForNewLine(tail);
		
		//Run the command
		runCMD(env, tail);
	}
}

//After parsing the command, it gets all messed up. This puts it 
//back to it's original state.
void mkCMD(char *myargv[], char *cmd){
	int i = 0;
	*cmd = '\0';
	for (i = 0; myargv[i]; i++){
		strcat(cmd, myargv[i]);
		strcat(cmd, " ");
	}
}

//Calls command to start process
void doProcess(char *cmd, char *env[]){
	splitArgs(cmd, myargv);
	mkCMD(myargv, cmd);
	//If the process isn't cd, exit or pwd, we will fork a child
	if (doBasicCommand(myargv) == 0){
		makeChild(env, cmd);
	}
}

int main(int argc, char *argv[], char *env[]){
	int i = 0;
	
	/*Get the PATH and HOME and then split the PATH up and store it in pathdir*/
	getPaths(env);
	parsePath(path, pathItems);
	
	/*Print opening banner*/
	printf("/*************************************************\n");
	printf("1. Show PATH:\n %s\n2. Show HOME:\n %s\n3. Decompose PATH strings:\n", path, home);
	for (i = 0; pathItems[i] != '\0'; i++){
		printf(" %s\n", pathItems[i]);
	}
	printf("*************************************************/\n");
	
	/*Main loop*/
	while(1){
			/*Reset everything*/
			redirValue = -1;
			memset(lineParts, 0, 64*64*sizeof(lineParts[64][64]));
			memset(cmd, 0, 128*sizeof(cmd[128]));	
			i = 0;
			argCount = 0;
			*myargv = NULL;
			
			//Print the input token, get the input
			printf("sh> ");
			fgets(cmd, 128, stdin);
			checkForNewLine(cmd);
			
			//Count pipes and check for redir
			countPipes(cmd);
			checkRedir(cmd);

			//Start commands
			doProcess(cmd, env);
	}
	return 0;
}
