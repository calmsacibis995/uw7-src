.ident	"@(#)libc-i386:sys/_lwpprivate.s	1.2"

/
/ void * 
/ __lwp_private(void *)
/	Registers the private data pointer with the kernel and returns the 
/	virtual address at which this pointer can be read from user level.
/
/ Calling/Exit State:
/	Returns the virtual address at which the private data pointer can be 
/	accessed.
/
/

	.file	"_lwpprivate.s"
	
	.text

	.globl  _cerror

_fwdef_(`__lwp_private'):
	MCOUNT
	movl	$LWPPRIVATE,%eax
	lcall	$0x7,$0
	jae 	.noerror		/ all OK - normal return
	jmp 	_cerror		/  otherwize, error

.noerror:
	ret
