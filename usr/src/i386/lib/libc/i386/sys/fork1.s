.ident	"@(#)libc-i386:sys/fork1.s	1.3"

/ pid_t fork1()
/	fork1() sys call stub. Creates a child process with one LWP 
/	corresponding to the LWP in the parent that invoked fork1().
/
/ Calling/Exit State:
/ 	%edx == 0 in parent process, %edx = 1 in child process.
/ 	%eax == pid of child in parent, %eax == pid of parent in child.

	.file	"fork1.s"

_fwdef_(`fork1'):
	MCOUNT			/ subroutine entry counter if profiling
_m4_ifdef_(`GEMINI_ON_OSR5',`
	movl	$FORK,%eax
',`
	movl	$FORK1,%eax
')
	lcall	$0x7,$0
	jc	_cerror
	testl	%edx,%edx
	jz	.parent
_m4_ifdef_(`PROF',`
	_prologue_
	pushl	$1		/ restart SIGPROF in the child
	call	_fref_(_dprofil)
	addl	$4,%esp
	_epilogue_
')
	xorl	%eax,%eax
.parent:
	ret
