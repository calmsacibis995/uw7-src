.ident	"@(#)libc-i386:sys/lwpexit.s	1.1"


/
/ void
/ _lwp_exit(void)
/	Terminates the execution of the calling context.
/
/ Calling/Exit State:
/	None.
/
/ Remarks:
/	If this is the only context in the process, the process exits.
/	Depending on how this context was created, it may or may not be 
/	"waitable".
/

	.file	"lwpexit.s"
	
	.text


	.globl  _cerror

_fwdef_(`_lwp_exit'):
	MCOUNT
	movl	$LWPEXIT,%eax
	lcall	$0x7,$0
