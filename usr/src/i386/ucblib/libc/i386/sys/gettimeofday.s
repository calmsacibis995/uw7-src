/		copyright	"%c%"

.ident	"@(#)ucb:i386/ucblib/libc/i386/sys/gettimeofday.s	1.1"
/ gettimeofday


	.file	"gettimeofday.s"

	.text

	.globl  _cerror
	.weak	_abi_gettimeofday

/ _fwdef_(`gettimeofday'):
/ _abi_gettimeofday:
/	MCOUNT
	movl	$GETTIMEOFDAY,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret
