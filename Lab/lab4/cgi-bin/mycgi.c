#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#define MAX 10000
#define BLKSIZE 4096

typedef struct {
    char *name;
    char *value;
} ENTRY;

struct stat mystat, *sp;

ENTRY entry[MAX];

int fd, gd;
char buf[MAX];

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int ls_file(char *fname)
{
  struct stat fstat, *sp;
  int r, i;
  char ftime[64];

  sp = &fstat;
  //printf("name=%s\n", fname); getchar();

  if ( (r = lstat(fname, &fstat)) < 0){
     printf("can't stat %s\n", fname); 
     exit(1);
  }

  if ((sp->st_mode & 0xF000) == 0x8000)
     printf("%c",'-');
  if ((sp->st_mode & 0xF000) == 0x4000)
     printf("%c",'d');
  if ((sp->st_mode & 0xF000) == 0xA000)
     printf("%c",'l');

  for (i=8; i >= 0; i--){
    if (sp->st_mode & (1 << i))
	printf("%c", t1[i]);
    else
	printf("%c", t2[i]);
  }

  printf("%4d ",sp->st_nlink);
  printf("%4d ",sp->st_gid);
  printf("%4d ",sp->st_uid);
  printf("%8d ",sp->st_size);

  // print time
  strcpy(ftime, ctime(&sp->st_ctime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s  ",ftime);

  // print name
  printf("%s", basename(fname));  

  // print -> linkname if it's a symbolic file
  if ((sp->st_mode & 0xF000)== 0xA000){ // YOU FINISH THIS PART
     // use readlink() SYSCALL to read the linkname
     // printf(" -> %s", linkname);
  }
  printf("\n");
}

int ls_dir(char *dname){
	return 0;
}

int myls(){
	struct stat mystat, *sp;
	int r;
	char *s;
	char name[1024], cwd[1024];

	s = entry[1].value;
	if (==1)
		s = "./";

	sp = &mystat;
	if (r = lstat(s, sp) < 0){
		printf("no such file %s\n", s); exit(1);
	}
	strcpy(name, s);
	if (s[0] != '/'){    // name is relative : get CWD path
		getcwd(cwd, 1024);
		strcpy(name, cwd); strcat(name, "/"); strcat(name,s);
	}
	if (S_ISDIR(sp->st_mode))
		ls_dir(name);
	else
		ls_file(name);
}

int mycat(){
	int fd;
	int i, n;
	char buf[1024];

	if (entry[0].value && entry[1].value){
		if (access(entry[1].value, R_OK)){
			printf("<H1>Cat Results</H1>");
			fd = open(entry[1].value, O_RDONLY);

			while(n = read(fd, buf, 1024)){
				for (i = 0; i < n; i++){		
						putchar(buf[i]);
				}
			}
		}
		else{
			printf("<p>Invalid permissions.</p>");
		}
	}
	else{
		printf("<p>Invalid number of arguments</p>");
		return -1;
	}
}

int mycp(){
	int n, total=0;                 
   if (entry[0].value && entry[1].value && entry[2].value){
	if (access(entry[1].value, R_OK)){
		fd = open(entry[1].value, O_RDONLY);
	
		if (fd < 0) 
			exit(2);
		gd=open(entry[2].value,O_WRONLY|O_CREAT);
		if (gd < 0) 
			exit(3);

		while (n=read(fd, buf, BLKSIZE))
		{
			write(gd, buf, n);
			total += n;
		}
		printf("total=%d\n",total);

		close(fd); close(gd);
		return 0;
	}
	else{
		printf("<p>Invalid permissions.</p>");
	}
  }
  else{
	printf("<p>Invalid number of arguments</p>");
	return -1;
  }
}

void onInput(){
	int res;

	if (strcmp(entry[0].value,"cat") == 0){
		//Do cat command
		res = mycat();
		if (res == -1){
			printf("<p>Invalid number of arguments</p>");
		}
		else{
			printf("<p>cat OK</p>");
		}
	}
	else if (strcmp(entry[0].value, "cp") == 0){
		//Do copy command
		res = mycp();
		if (res == -1){
			printf("<p>Invalid number of arguments</p>");	
		}
		else{
			printf("<p>cp OK</p>");
		}
	}
	else if (strcmp(entry[0].value, "mkdir") == 0){
		//Do mkdir command
		if (entry[1].value){
			if (mkdir(entry[1].value, 0777) < 0){
				printf("<p>errno=%d : %s\n<>", errno, strerror(errno));
			}
			else{
				printf("<p>mkdir OK</p>");
			}
		}
	}
	else if (strcmp(entry[0].value, "rmdir") == 0){
		//Do rmdir command
		if (entry[1].value){
			if (rmdir(entry[1].value) < 0){
				printf("<p>errno=%d : %s\n</p>", errno, strerror(errno));
			}
			else{
				printf("rmdir OK");
			}
		}
	}
	else if (strcmp(entry[0].value, "rm") == 0){
		//Do unlink file command
		if (entry[1].value){
			if (unlink(entry[1].value) < 0){
				printf("<p>errno=%d : %s\n<>", errno, strerror(errno));
			}
			else{
				printf("<p>rm OK</p>");
			}
		}
	}
	else if (strcmp(entry[0].value, "ls") == 0){
		//Do ls command		
		printf("<p>Here</p>");
	}
	else{
		//Error!
		printf("<p>'%s' is and invalid command.</p>", entry[0].name);
	}
}

main(int argc, char *argv[]) 
{
  int i, m, r;
  char cwd[128];

  m = getinputs();    // get user inputs name=value into entry[ ]
  getcwd(cwd, 128);   // get CWD pathname

  printf("Content-type: text/html\n\n");
  printf("<p>pid=%d uid=%d cwd=%s\n", getpid(), getuid(), cwd);

  printf("<H1>Echo Your Inputs</H1>");
  printf("You submitted the following name/value pairs:<p>");
 
  for(i=0; i <= m; i++)
     printf("%s = %s<p>", entry[i].name, entry[i].value);
  printf("<p>");

  /*****************************************************************
   Write YOUR C code here to processs the command
         mkdir dirname
         rmdir dirname
         rm    filename
         cat   filename
         cp    file1 file2
         ls    [dirname] <== ls CWD if no dirname
  *****************************************************************/

  onInput();

  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#FF0000\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");

  //------ NOTE : CHANGE ACTION to YOUR login name ----------------------------
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~marks/cgi-bin/mycgi\">");
  
  printf("Enter command : <INPUT NAME=\"command\"> <P>");
  printf("Enter filename1: <INPUT NAME=\"filename1\"> <P>");
  printf("Enter filename2: <INPUT NAME=\"filename2\"> <P>");
  printf("Submit command: <INPUT TYPE=\"submit\" VALUE=\"Click to Submit\"><P>");
  printf("</form>");
  printf("------------------------------------------------<p>");

  printf("</body>");
  printf("</html>");
}
