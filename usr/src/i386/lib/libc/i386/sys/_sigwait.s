.ident	"@(#)libc-i386:sys/_sigwait.s	1.6"

/
/ int sigwait()
/	The sigwait() system call stub.
/
/ Calling/Exit State: 
/	None.
/

	.file	"_sigwait.s"

	.text

	.globl  _cerror
	.globl	__sigwait

_fgdef_(`__sigwait'):
	movl	$SIGWAIT,%eax
	lcall	$7,$0
	jc	.error
	ret
.error:
	cmpl	$ERESTART,%eax
	je	__sigwait
	jmp	_cerror
