.ident	"@(#)libc-i386:sys/p_online.s	1.1"

/
/ int p_online()
/	The p_online() system call stub.
/
/ Calling/Exit State: 
/	None.

	.file	"p_online.s"
	.text
	.globl  _cerror

_fwdef_(`p_online'):
	movl	$ONLINE,%eax
	lcall	$7,$0
	jc	_cerror
	ret
