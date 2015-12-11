/********************************
Name: Des Marks
ID:11352911
Class: CS 360
Date: 8/30/15
********************************/



/************* t.c file ********************/
#include <stdio.h>
#include <stdlib.h>

int *FP;

main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  printf("enter main\n");
	 
  //Print the addresses for argc, argv, env, a, b and c. 
  printf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
  printf("&a=%8x &b=%8x &c=%8x\n", &a, &b, &c);

	//Set a, b and c.
  a=1; b=2; c=3;
	//Call A
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");
  // PRINT ADDRESS OF d, e, f
	printf("&d=%x &e=%x &f=%x\n", &d, &e, &f);

	//Set d, e, f
  d=4; e=5; f=6;
	//Call B
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");

  // PRINT ADDRESS OF g,h,i
	printf("&g=%x &h=%x &i=%x\n", &g, &h, &i);
 	
	//Set g, h, i
 	g=7; h=8; i=9;
	//Call C
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p, *temp;

  printf("enter C\n");
  // PRINT ADDRESS OF u,v,w;
	printf("&u=%x &v=%x &w=%x\n", &u, &v, &w);

	//Set u, v, w
  u=10; v=11; w=12;

	//Question 1	
  asm("movl %ebp, FP");    // CPU's ebp register is the FP
		 
	temp = FP;

	printf("\nThe stack frame linked list is:\n");
	printf("%x-->", temp);	//Print the intial state of the frame pointer
	do{	//While FP is not NULL, print out the value of FP
		printf("%x", *temp);
		if (*temp != 0)	printf("-->");
		temp = (*temp);	//Dereference to FP to jump back to the previous frame
	}while(temp);



	//Question 2
	printf("\n\n");
	p = FP-8;	//Set p equal to the FP minus 8 bytes. That way it includes u,v and w

	printf("\nNow printing 100 entries of the stack contents...\n");
	for (i = 0; i < 100; i++)	//Print p (address) and *p (contents)
	{
		printf("%d. Address:%x	Contents:%x\n",i+1, p, *p);
		p++;	//Change the address by one to get the next item of int type
	}


  /*********** Write C code to DO ************* 
  1 to 4 AS SPECIFIED in the PROBLEM 3 of the TEXT

	(1). Print the stack frame link list

	(2). Print the stack contents from FP-8 to the frame of main()
    	 YOU MAY JUST PRINT 100 entries of the stack contents.

	(3). On a hard copy of the print out, identify the stack contents
     	as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.

	(4). Find where are argc, argv and env located in the stack.
     	What is exactly argv? 
	**********************************************/
}
