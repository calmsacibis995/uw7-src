.ident	"@(#)libc-i386:sys/modpath.s	1.1"

	.file	"modpath.s"
	
	.text

	.globl	_cerror

_fwdef_(`modpath'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MODPATH,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
