/**********************************************************
Name: Des Marks
ID:11352911
Class: CS360
Date: 9/7/15
**********************************************************/
/*H   d c b a fmt retPC ebp Locals->   L*/
/*
Data - g and s varibales initalized to non zero values
BSS - g and s variables intialized to zero of null
heap - dyanamic memory
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int BASE = 10;

typedef unsigned int u32;

char *table = "0123456789ABCDEF";

mymain(int argc, char *argv[ ], char *env[ ])
{
  int i;

  myprintf("in mymain(): argc=%d\n", argc);
  for (i=0; i < argc; i++)
      myprintf("argv[%d] = %s\n", i, argv[i]);
  
  // WRITE CODE TO PRINT THE env strings written by ME!
  myprintf("Now printing the env[] strings:\n");
  i = 0;
  while (*env != '\0'){
        myprintf("env[%d] = %s\n", ++i,*env);
	env++;
  }

  myprintf("---------- testing YOUR myprintf() ---------\n");
  myprintf("this is a test\n");
  myprintf("testing a=%d b=%x c=%c s=%s\n", 123, 123, 'a', "testing");
  myprintf("string=%s, a=%d  b=%u  c=%o  d=%x\n",
           "testing string", -1024, 1024, 1024, 1024);
  myprintf("mymain() return to main() in assembly\n"); 
}

void myprintf(char *fmt, ...){
	/*cp will point to fmt, ip will point to each parameter, and ebp is the stack pointer*/
	char *cp;
	int *ip;
	int ebp = get_ebp();

	/*cp now points at fmt, and ip points at the first parameter*/
	cp = fmt;	//now points at fmt (8 bytes up)
	ip = (int *)(ebp+12);	//according to the stack diagram the variables start 12 bytes up from the FP/ebp
		//^ebp returns as an int, but we need an int *

	while (*cp){
		if (*cp == '%'){
			cp++;	//Move to address of the next char
			if (*cp	== 'c')	//Char (don't need?)
			{
				putchar(*ip);	//pass the dereferenced value
			}
			else if (*cp == 'x')	//Hex
			{
				printx(*ip);
			}
			else if (*cp == 'd')	//Decimal
			{
				printd(*ip);
			}
			else if (*cp == 'u')	//Unsigned
			{
				printu(*ip);
			}
			else if (*cp == 's')	//String
			{
				prints(*ip);
			}
			else if (*cp == 'o')	//Octal
			{
				printo(*ip);
			}

			ip++;	//move to next address
		}
		else{	//We will place the character otherwise
			if (*cp == '\n'){
				putchar(*cp);
				putchar('\r');
			}
			else{
				putchar(*cp);
			}
		}

		cp++;	//Move to the next char type
	}

}

/*************Print string function****************/
void prints(char* str)
{
	while(*str){
		if (*str != '\n'){	//If the char does not contain a newline char, then print normally	
			putchar(*str);
		}
		else{	//If we encounter a newline, the insert a '\r'
			putchar(*str);
			putchar('\r');
		}

		str++;
	}
}

/*************From K.C.**************************/
int rpu(u32 x)
{
	char c;
	if (x){
		c = table[x % BASE];
		rpu(x / BASE);
		putchar(c);
	}
} 

/***********From K.C.**************************/
int printu(u32 x)
{
	BASE = 10;
	if (x==0)
		putchar('0');
	else
		rpu(x);
		putchar(' ');
		
	return 0;
}

/**********printd function********************/
int printd(int x)
{
	BASE = 10;
	if (x < 0){
		putchar('-');
		x = -x;
	}
	rpu(x);

	return 0;
}

/**********printo function******************/
int printo(u32 x){
	BASE = 8; //Octal uses base 8 (0-7)
	putchar('0');

	rpu(x);
	
	return 0;
}

/*********printx function*****************/
int printx(u32 x){
	BASE = 16;

	putchar('0');	//start with 0x
	putchar('x');
	
	rpu(x);	//Call the recursive print
	
	return 0;
}
