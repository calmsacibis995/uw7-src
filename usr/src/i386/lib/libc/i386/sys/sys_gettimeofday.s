.ident	"@(#)libc-i386:sys/sys_gettimeofday.s	1.2"
/ sys_gettimeofday


	.file	"sys_gettimeofday.s"
	.text
	.globl  _cerror
	.globl  _sys_gettimeofday

_sys_gettimeofday:
	MCOUNT
	movl	$GETTIMEOFDAY,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret

	.type _sys_gettimeofday, "function"
	.size _sys_gettimeofday, .-_sys_gettimeofday
