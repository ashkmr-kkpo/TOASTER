#define ASM 1

#include "syshandler.h"
#include "x86_desc.h"



.global syscallhandle
# system handler invoked by user space INT $80
syscallhandle:
	cmpl	$10, %eax	# check if system call is valid
	ja		invalid_call
	cmpl	$1, %eax
	jb		invalid_call

	pushal
	pushl	%edx	# push arguments in correct order for c-style call
	pushl	%ecx
	pushl	%ebx

	call 	*sys_call_table(,%eax,4) # jump table to system calls

	popl	%ebx
	popl	%ecx
	popl	%edx
	movl	%eax, 28(%esp) # modify return value in EAX
	popal

	iret
invalid_call:
	movl	$-1, %eax	# return value for error
	iret 

sys_call_table : .long sys_zero, sys_halt, sys_execute ,sys_read ,sys_write ,sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
