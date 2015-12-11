//**************************** ECHO CLIENT CODE **************************
// The echo client client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX 1024

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 
char pathnames[32][64], path[128], command[8], home[64];
char cbuf[1024], filename[32];

int sock, r, pathcount = 0, size = 0;
int SERVER_IP, SERVER_PORT; 

struct stat mystat;

// clinet initialization code
int client_init(char *argv[])
{
  printf("======= clinet init ==========\n");

  printf("1 : get server info\n");
  hp = gethostbyname(argv[1]);
  if (hp==0){
     printf("unknown host %s\n", argv[1]);
     exit(1);
  }

  SERVER_IP   = *(long *)hp->h_addr;
  SERVER_PORT = atoi(argv[2]);

  printf("2 : create a TCP socket\n");
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock<0){
     printf("socket call failed\n");
     exit(2);
  }

  printf("3 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("4 : connecting to server ....\n");
  r = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(1);
  }

  printf("5 : connected OK to \007\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s  IP=%s  PORT=%d\n", 
          hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
  printf("---------------------------------------------------------\n");

  printf("========= init done ==========\n");
}

// Executes the command given by the user (via the child process)
void do_command(char *env[])
{
  /**
   * First, create a local argv titled myargv:
   * myargv[0] = command
   * myargv[1] = path
   * myargv[2] = NULL
   */
  char *myargv[3];
  myargv[0] = command;
  myargv[1] = NULL;

  if (strcmp(path, "") != 0) { myargv[1] = path; }

  myargv[2] = NULL;

  /**
   * Loop through our path names (pathnames) and
   * try to execute the command on that path
   */
  int i = 0;
  char trycommand[32];
  for (i = 0; i < pathcount; i++)
  {
    strcpy(trycommand, pathnames[i]);       //     /bin/etc
    strcat(trycommand, "/");                //     /bin/etc/
    strcat(trycommand, command);            //     /bin/etc/ls
    execve(trycommand, myargv, env);
  }
}

void displaymenu()
{
  // Display menu
  printf("----------------- MENU --------------------\n");
  printf("lcat   lls  lpwd  lcd  lmkdir   lrmdir  lrm\n");
  printf("get  put   ls  cd  pwd   mkdir  rmdir   rm\n");
  printf("-------------------------------------------\n");
}

main(int argc, char *argv[ ], char *env[ ])
{
  int n, i = 0, status, pid, r;      /* pathcount: keeps track of number of paths */
  char line[MAX], ans[1024], *token;

  if (argc < 3){
     printf("Usage : client ServerName SeverPort\n");
     exit(1);
  }
  client_init(argv);

  printf("********  processing loop  *********\n");
  while (1){
    printf("user@client $ ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0)                  // exit if NULL line
       exit(0);

    /**
     * Store the PATH for exit
     * Then, strtok for path variables
     */
    strcpy(path, getenv("PATH"));
    strcpy(home, getenv("HOME"));
    token = strtok(path, ":");

    while (token != NULL)
    {
      strcpy(pathnames[i], token);
      token = strtok(NULL, ":");
      i++;
    }

    pathcount = i; i = 0;

    // Store the command and path that the user entered (extra checks for any spaces)
    strcpy(path, "");
    sscanf(line, "%s %[^\n]", command, path);

    /**
     * Check if the the line entered is a local command
     * (Also ensure that ls was not entered. Essentially a 'special case')
     */
    if (command[0] == 'l' && strcmp(command, "ls") != 0)
    {
      // Get rid of the l in front of the command
      sscanf(&command[1], "%s", command);

      // Handling local commands
      if (strcmp(command, "cd") == 0)       /* cd command */
      {
        if (strcmp(path, "") == 0)          /* Empty path. cd to root */
        {
          chdir(home);
          printf("cd: Changed directory to HOME\n");
        }
        else
        {
          r = chdir(path);
          if (r != -1)
            printf("cd: %s: Changed directory to %s\n", path, path);
          else
            printf("cd: %s: No such file or directory\n", path);
        }
      }
      else
      {
        // Otherwise, fork a child process to handle the other commands
        pid = fork();

        if (pid) {
          r = wait(&status);
        }
        else {
          //Look for the command with in the directories in the PATH
          do_command(env);
        }
      }

      putchar('\n');
    }
    else if (strcmp(command, "menu") == 0) { displaymenu(); putchar('\n'); }     /* Help for user: Display command */
    else if (strcmp(command, "exit") == 0) { exit(0); putchar('\n'); }           /* Exit the client */
    else if (strcmp(command, "get") == 0)
    {
        // Split up the path to get the filename only
        token = strtok(path, "/");
        while (token != NULL) {
            strcpy(filename, token);
            token = strtok(NULL, "/");
        }

        // Send the command and path to the server
        write(sock, line, sizeof(line));
        printf("wrote: %s to server\n", line);

        read(sock, cbuf, sizeof(cbuf));             // Read back the success message from server
        printf("read back: %s from server\n", cbuf);
        cbuf[sizeof(cbuf) - 1] = '\0';

        if (strcmp(cbuf, "BAD") != 0) {
            /**
             * Opening was successful
             * (1) Convert size to int
             * (2) Open file of filename  => fd
             * (3) Read from server the contents of filename
             * (4) Store contents into fd
             */
            size = atoi(cbuf);
            int fd = open(filename, O_WRONLY|O_CREAT, 0644);
            int bytes = 0;

            printf("Expecting %d bytes\n", size);

            while (bytes < size) {           // read file contents from client
                n = read(sock, cbuf, sizeof(cbuf));

                bytes += n;
                printf("read %d bytes\n", n);
                write(fd, cbuf, sizeof(cbuf));
                bzero(cbuf, sizeof(cbuf)); cbuf[sizeof(cbuf) - 1] = '\0';
            }
        }
        else {
            printf("Error. File %s not found on server", filename);
        }

        putchar('\n');
    }
    else if (strcmp(command, "put") == 0)
    {
        int fd = open(path, O_RDONLY);
        lstat(path, &mystat);

        if (fd >= 0) { // file opened properly
            printf("(Server command: %s: Sent %s to the server)\n", line, line);

            // Send the filename over to the server
            line[strlen(line)] = '\0';
            n = write(sock, line, MAX);                 // write "put [filename]"
            n = read(sock, cbuf, sizeof(cbuf));         // read "file opened" or "file did not open"
            printf("file msg: %s\n", cbuf);

            sprintf(cbuf, "%d", mystat.st_size);
            cbuf[sizeof(cbuf) - 1] = '\0';

            printf("writing the size of %s to server\n", cbuf);
            write(sock, cbuf, sizeof(cbuf));            // write size of file to server
            bzero(cbuf, sizeof(cbuf)); cbuf[sizeof(cbuf) - 1] = '\0';

            int bytes = 0;
            while (n = read(fd, cbuf, sizeof(cbuf))) {          // reading a line from the file
                if (n != 0) {
                    cbuf[sizeof(cbuf) - 1] = '\0';

                    // Update bytes
                    bytes += n;
                    printf("wrote %d bytes\n", bytes);

                    // Write the contents to the server
                    write(sock, cbuf, sizeof(cbuf));
                    bzero(cbuf, sizeof(cbuf));
                    cbuf[sizeof(cbuf) - 1] = '\0';
                }
            } close(fd);
        }
        else {
            printf("File not sent. %s does not exist\n", path);
        }

        putchar('\n');
    }
    else {        /* Server command (except for get and put which are handled above */
        printf("(Server command: %s: Sent %s to the server)\n", line, line);

        // Send ENTIRE line to server then READ from the server
        line[strlen(line)] = '\0';
        n = write(sock, line, MAX);
        bzero(cbuf, sizeof(cbuf)); cbuf[strlen(cbuf) - 1] = '\0';        // Clear out the answer

        n = read(sock, cbuf, sizeof(cbuf));

        printf("Server returned:\n%s\n", cbuf);
        cbuf[sizeof(cbuf) - 1] = '\0';        // Clear out the answer

    }
  }
}


