.ident	"@(#)libc-i386:sys/modstat.s	1.1"

	.file	"modstat.s"
	
	.text

	.globl	_cerror

_fwdef_(`modstat'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MODSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
