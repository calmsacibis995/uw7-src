.ident	"@(#)libc-i386:sys/online.s	1.1"

	.file	"online.s"

	.text

	.globl	_cerror

_fwdef_(`online'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$ONLINE,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
