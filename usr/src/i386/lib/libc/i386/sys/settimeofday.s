.ident	"@(#)libc-i386:sys/settimeofday.s	1.1"

/ settimeofday


	.file	"settimeofday.s"

	.text

	.globl  _cerror

_fwdef_(`settimeofday'):
	MCOUNT
	movl	$SETTIMEOFDAY,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret
