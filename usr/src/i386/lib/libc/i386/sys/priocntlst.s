.ident	"@(#)libc-i386:sys/priocntlst.s	1.3"

/
/ int priocntllist()
/	The priocntllist() system call stub.
/
/ Calling/Exit State: 
/	None.

	.file	"priocntllst.s"
	.text
	.globl  _cerror

_fwdef_(`priocntllist'):
	movl	$PRIOCNTLLST,%eax
	lcall	$7,$0
	jc	_cerror
	ret
