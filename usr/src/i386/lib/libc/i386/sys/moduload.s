.ident	"@(#)libc-i386:sys/moduload.s	1.1"

	.file	"moduload.s"
	
	.text

	.globl	_cerror

_fwdef_(`moduload'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MODULOAD,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
