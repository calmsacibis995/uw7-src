.ident	"@(#)libc-i386:sys/_lwpself.s	1.2"

/
/ lwpid_t
/ __lwp_self(void)
/	Returns the ID of the calling LWP.
/
/ Calling/Exit State:
/	Returns the ID of the calling LWP.
/
/ Remarks:
/	The _lwp_self() function will use this system call. The system call 
/	will only be made when the ID is not cached.
/

	.file	"_lwpself.s"
	
	.text

	.globl  _cerror

_fwdef_(`__lwp_self'):
	MCOUNT
	movl	$LWPSELF,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	jmp 	_cerror		/  otherwize, error

.noerror:
	ret
