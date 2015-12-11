#Des Marks
#CS360
#September 6, 2015
#11352911

#--------------------- s.s file --------------------------	
	.global get_esp, get_ebp, main, mymain, myprintf

get_esp:
	movl %esp, %eax
	ret
get_ebp:
	movl %ebp, %eax
	ret

main:    
   pushl  %ebp
   movl   %esp, %ebp

#There is a standard/uniform sort of process that goes into 1,2 and 3. You
#push the arguments on in reverse order, call the function, and then move the stack pointer
#up to the top.

	
# (1). Write ASSEMBLY code to call myprintf(FMT)
#      HELP: How does mysum() call printf() in the class notes.	
	pushl $FMT	#push the string on to the stack
	call myprintf	#call the myprintf function
	addl $4, %esp	#move the stack pointer to the current instruction

# (2). Write ASSEMBLY code to call mymain(argc, argv, env)
#      HELP: When crt0.o calls main(int argc, char *argv[], char *env[]), 
#            it passes argc, argv, env to main(). 
#            Draw a diagram to see where are argc, argv, env?
	pushl 16(%ebp)	
	pushl 12(%ebp)	
	pushl 8(%ebp)	
	call mymain	#call the main function
	addl $16, %esp	#move the stack pointer to the current instruction

# (3). Write code to call myprintf(fmt,a,b)	
#      HELP: same as in (1) above
	pushl b		#we want to push the arguments in opposite order
	pushl a		
	pushl $fmt
	call myprintf	#call the printf function
	addl $12, %esp	#move the stack pointer to the current instruction

# (4). Return to caller
  movl  %ebp, %esp
	popl  %ebp
	ret

#---------- DATA section of assembly code ---------------
	.data
FMT:	.asciz "main() in assembly call mymain() in C\n"
a:	.long 1234
b:	.long 5678
fmt:	.asciz "a=%d b=%d\n"
#---------  end of s.s file ----------------------------

