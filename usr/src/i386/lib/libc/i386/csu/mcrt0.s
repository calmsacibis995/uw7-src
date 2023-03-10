	.file	"mcrt0.s"

	.ident	"@(#)libc-i386:csu/mcrt0.s	1.9"


/	C runtime startup and exit with profiling
/

_m4_define_(`CBUFS', 600)
_m4_define_(`WORDSIZE', 4)
_m4_define_(`PTRSHIFT', 2)
_m4_define_(`PTRSIZE', 4)

/ global entities defined in this file
	.globl	_start
/	.globl	_countbase
/	.globl	environ
	.globl	exit

/ global entities defined elsewhere, but used here
	.comm	___Argv, 4, 4	/ libprof: common/newmon.c defines this
	.globl	_cleanup/ libc: I/O cleanup
	.globl	etext	/ set by 'ld' to end of text
	.globl	main	/ user's entry point
	.globl	monitor	/ libc: monitor(3C)
	.globl	sbrk	/ libc: brk(2) system call
	.globl	write	/ libc: write(2) system call

/
/	C language startup routine with profiling
/

/ C language startup routine.
/ Assume that exec code has cleared the direction flag in the TSS.
/ Assume that %esp is set to the addr after the last word pushed.
/ The stack contains (in order): argc, argv[],envp[],...
/ Assume that all of the segment registers are initialized.

_fgdef_(_start):
/ Allocate a NULL return address and a NULL previous %ebp as if
/ there was a genuine call to _start.
/ sdb stack trace shows _start(argc,argv[0],argv[1],...,envp[0],...)
	subl	$[PTRSIZE+PTRSIZE],%esp
	movl	%esp,%ebp	/ The first stack frame.

/ Calculate the location of the envp array by adding the size of
/ the argv array to the start of the argv array.
	movl	8(%ebp),%eax		/ argc
	leal	16(%ebp,%eax,4),%edx	/ envp
	movl	%edx,environ		/ copy to environ
	pushl	%edx
	leal	12(%ebp),%edx		/ argv
	movl	%edx,___Argv
	pushl	%edx
	pushl	%eax			/ argc
/ Now have built the stack for main(), build on top the stack for
/ monitor().  Approximate tally area as 1/4 size of text + CBUFS
/ counters + header
	movl	$[etext+7],%ebx
	subl	$.eprol,%ebx
	shrl	$0x3,%ebx		/ tally area is 1/4 size of text
	shll	$0x1,%ebx
	addl	$[8\*CBUFS+12+WORDSIZE-1],%ebx
	andl	$-WORDSIZE,%ebx		/ round to word boundary
	pushl	%ebx			/ get space
	call	sbrk
	cmpl	$-1,%eax
	subl	$4,%esp			/ pop word off stack
	je	nospace

/ Start profiling.
	movl	%eax,%ecx
	addl	$12,%ecx
	movl	%ecx,_countbase
	pushl	$CBUFS			/ number of counters (arg5)
	shrl	$1,%ebx			/ number of shorts in buffer
	pushl	%ebx			/ (arg4)
	pushl	%eax			/ start of buffer (arg3)
	pushl	$etext			/ end of text (arg2)
	pushl	$.eprol			/ start of text (arg1)
	call	monitor			/ monitor(lowpc,highpc,buffer,
					/	  bufsiz,CBUFS)
	addl	$20,%esp

/ Main's arguments are still on the stack.
	call	main			/ main(argc,argv,envp)
	addl	$12,%esp		/ clear args so sdb knows how many args main() has
	movl	%eax,%esi		/ A register variable ...

/	exit, but first call monitor() to write profile buffer
/
_fgdef_(exit):
	pushl	$0			/ finish profiling
	pushl	$0
	pushl	$0
	pushl	$0
	pushl	$0
	call	monitor
	addl	$20,%esp
	call	_cleanup		/ clean up I/O buffers
					/ Argument from original exit() call
	movl	$EXIT,%eax
	lcall	$0x7,$0
	hlt

/	not enough memory for profiling buffer
/
nospace:
	pushl	$2			/ write error message and exit
	pushl	emesg
	pushl	$MESSL
	call	write			/ write(2,emesg,MESSL)

	pushl	$-1
	call	exit

	.data
emesg:			/ "No space for monitor buffer\n"
	.byte	78,111,32,115,112,97,99,101,32,102,111,114
	.byte	32,109,111,110,105,116,111,114
	.byte	32,98,117,102,102,101,114,10
	.set	MESSL,.-emesg
	.byte	0
	.align	4

_dwdef_(`countbase', 4):
	.set	.,.+PTRSIZE

	.text
	.align	4	/ generate padding NOP's before .eprol
.eprol:		/ beginning of user text
