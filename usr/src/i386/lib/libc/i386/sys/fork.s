.ident	"@(#)libc-i386:sys/fork.s	1.8"

/ pid = fork();

/ %edx == 0 in parent process, %edx = 1 in child process.
/ %eax == pid of child in parent, %eax == pid of parent in child.

	.file	"fork.s"

_fwdef_(`fork'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FORK,%eax
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
	je	fork
	jmp	_cerror
