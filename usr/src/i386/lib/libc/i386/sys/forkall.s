.ident	"@(#)libc-i386:sys/forkall.s	1.5"

/ pid_t forkall()
/	forkall() sys call stub. Clones the calling LWP's process. 
/
/ Calling/Exit State:
/ 	%edx == 0 in parent process, %edx = 1 in child process.
/ 	%eax == pid of child in parent, %eax == pid of parent in child.

	.file	"forkall.s"

_fwdef_(`forkall'):
	MCOUNT			/ subroutine entry counter if profiling
_m4_ifdef_(`GEMINI_ON_OSR5',`
	movl	$FORK,%eax
',`
	movl	$FORKALL,%eax
')
	lcall	$0x7,$0
	jc	.error
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
.error:
	cmpl	$ERESTART,%eax
	je	forkall
	jmp	_cerror
