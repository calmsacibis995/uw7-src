.ident	"@(#)libc-i386:sys/getksym.s	1.1"

	.file	"getksym.s"
	
	.text

	.globl	_cerror

_fwdef_(`getksym'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$GETKSYM,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
