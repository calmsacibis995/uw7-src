.ident	"@(#)libc-i386:sys/modload.s	1.1"

	.file	"modload.s"
	
	.text

	.globl	_cerror

_fwdef_(`modload'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MODLOAD,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
