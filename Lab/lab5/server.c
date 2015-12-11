// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>

#define  MAX 1024

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  sock, newsock;            // socket descriptors
int  serverPort;                     // server port number
int  r, length, n, pathcount;        // help variables
int size;                            // size for get and put

char pathnames[32][64], path[128];   // stores the pathnames
char home[64], command[32], msg[64]; // stores home directory for empty cd command
char cbuf[1024], *token, filename[32];
char *myargv[3];

struct stat mystat;

void cleanup()
{
    strcpy(path, "");
    strcpy(command, "");
    strcpy(msg, "");
    strcpy(filename, "");
    bzero(myargv, sizeof(myargv));
    bzero(cbuf, sizeof(cbuf)); cbuf[sizeof(cbuf) - 1] = '\0';
}

// Server initialization code:
int server_init(char *name)
{
    char *token;

   printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

   printf("1 : get and show server host info\n");
   hp = gethostbyname(name);
   if (hp == 0){
      printf("unknown host\n");
      exit(1);
   }
   printf("    hostname=%s  IP=%s\n",
               hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
   //  create a TCP socket by socket() syscall
   printf("2 : create a socket\n");
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0){
      printf("socket call failed\n");
      exit(2);
   }

   printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = 0;   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
       printf("bind failed\n");
       exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(sock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
      printf("get socketname error\n");
      exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

    /**
    * Store the PATH for exit
    * Then, strtok for path variables
    */
    int  i = 0;
    strcpy(path, getenv("PATH"));
    strcpy(home, getenv("HOME"));
    token = strtok(path, ":");

    while (token != NULL)
    {
        strcpy(pathnames[i], token);
        token = strtok(NULL, ":");
        i++;
    } pathcount = i;

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("5 : server is listening ....\n");
   listen(sock, 5);
   printf("===================== init done =======================\n");
}

/**
 * Child process function: used to execute the command (with execve)
 * given by the client
 */
void do_command(char *env[])
{
  /**
   * First, create a local argv titled myargv:
   * myargv[0] = command
   * myargv[1] = path
   * myargv[2] = NULL
   */
    myargv[0] = command;
    myargv[1] = NULL;

    if (strcmp(path, "") != 0) { myargv[1] = path; }

    myargv[2] = NULL;

    char buf[1024];
    if (strcmp(command, "mkdir") == 0) {
        sprintf(buf, "mkdir: server made directory %s\n", path);
        write(newsock, buf, sizeof(buf));
    }
    else if (strcmp(command, "rmdir") == 0) {
        sprintf(buf, "rmdir: server removed directory %s\n", path);
        write(newsock, buf, sizeof(buf));
    }
    else if (strcmp(command, "rm") == 0) {
        sprintf(buf, "rm: server removed file %s\n", path);
        write(newsock, buf, sizeof(buf));
    }

    /**
     * Loop through our path names (pathnames) and
     * try to execute the command on that path
     */
    int i = 0;
    char trycommand[64];
    for (i = 0; i < pathcount; i++)
    {
        strcpy(trycommand, pathnames[i]);       //     /bin/etc
        strcat(trycommand, "/");                //     /bin/etc/
        strcat(trycommand, command);            //     /bin/etc/ls
        execve(trycommand, myargv, env);
    }
}

main(int argc, char *argv[], char *env[])
{
   char *hostname;
   char line[1024];
   int r = 0, pid, status, i = 0;

   if (argc < 2)
      hostname = "localhost";
   else
      hostname = argv[1];
 
   server_init(hostname); 

   // Try to accept a client request
   while(1) {
     printf("server: accepting new connection ....\n"); 

     // Try to accept a client connection as descriptor newsock
     length = sizeof(client_addr);
     newsock = accept(sock, (struct sockaddr *)&client_addr, &length);
     if (newsock < 0){
        printf("server: accept error\n");
        exit(1);
     }
     printf("server: accepted a client connection from\n");
     printf("-----------------------------------------------\n");
     printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                                        ntohs(client_addr.sin_port));
     printf("-----------------------------------------------\n");

     // Processing loop
     while(1) {
       n = read(newsock, line, MAX);
       if (n==0){
           printf("server: client died, server loops\n");
           close(newsock);
           break;
      }

      // show the line string
      printf("server@received $ %s\n", line);
      strcpy(path, ""); size = 0;
      sscanf(line, "%s %[^\n]", command, path);           /* split up line into command and path */

      /**
       * command has been read. check for what was read and execute
       */
      if (strcmp(command, "cd") == 0)
      {
          char buf[64];
          // Check if the path is empty
          if (strcmp(path, "") == 0)
          {
            chdir(home);
            strcpy(buf, "cd: Changed directory to HOME\n");
          }
          else
          {
              r = chdir(path);
              if (r != -1) {
                  sprintf(buf, "cd: %s: Changed directory to %s\n", path, path);
              }
              else {
                  sprintf(buf, "cd: %s: No such file or directory\n", path);
              }
          }

          write(newsock, buf, sizeof(buf));
      }
      else if (strcmp(command, "get") == 0)
      {
          printf("In get: command %s, path %s\n", command, path);

          /**
           * Attempt to open the file
           * (1) return file size if successful
           * (2) else, return BAD
           */
           int fd = open(path, O_RDONLY);
           if (fd >= 0) {        // successful
                lstat(path, &mystat);
                sprintf(cbuf, "%d", mystat.st_size);
                write(newsock, cbuf, sizeof(cbuf));         // Write the size to the client
                bzero(cbuf, sizeof(cbuf)); cbuf[sizeof(cbuf) - 1] = '\0';

               int bytes = 0;
               while (n = read(fd, cbuf, sizeof(cbuf))) {          // reading a line from the file
                   if (n != 0) {
                       cbuf[sizeof(cbuf) - 1] = '\0';

                       // Update bytes
                       bytes += n;
                       printf("wrote %d bytes\n", bytes);

                       // Write the contents to the server
                       write(newsock, cbuf, sizeof(cbuf));
                       bzero(cbuf, sizeof(cbuf));
                       cbuf[sizeof(cbuf) - 1] = '\0';
                   }
               } close(fd);
           }
           else {
                write(newsock, "BAD", sizeof("BAD"));
           }
      }
      else if (strcmp(command, "put") == 0)
      {
        // Split up the path to get the filename only
        token = strtok(path, "/");
        while (token != NULL) {
            strcpy(filename, token);
            token = strtok(NULL, "/");
        }

        // Open the file, then execute if or else depending on success of open
        int fd = open(filename, O_WRONLY|O_CREAT, 0644);
        if (fd >= 0) {                          // file opened properly
          write(newsock, "File opened properly", 64);

          read(newsock, cbuf, sizeof(cbuf));           // read the file size
          cbuf[sizeof(cbuf) - 1] = '\0';

          int bytes = 0;
          size = atoi(cbuf);
          printf("Expecting %d bytes\n", size);

          while (bytes < size) {           // read file contents from client
                n = read(newsock, cbuf, sizeof(cbuf));

                bytes += n;
                printf("read %d bytes\n", n);
                write(fd, cbuf, sizeof(cbuf));
                bzero(cbuf, sizeof(cbuf)); cbuf[sizeof(cbuf) - 1] = '\0';
          }
        }
        else {                                  // file did NOT open properly
            write(newsock, "File did not open properly", 64);
        }
      }
      else {                // Server read any other command besides cd, get and put
        pid = fork();

        if (pid) {
            r = wait(&status);
        }
        else {
            // Redirect the command back to the client
            close(1);
            dup(newsock);
            do_command(env);
        }
      }

      // Server command complete, send back to client
      printf("Server sent results of %s to client\n", command);
      cleanup();

      printf("server: ready for next request\n\n");
      bzero(line, sizeof(line)); line[strlen(line) - 1] = '\0';         // clear out the line
    }
 }
}
