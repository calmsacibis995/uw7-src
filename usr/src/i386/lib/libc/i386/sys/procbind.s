.ident	"@(#)libc-i386:sys/procbind.s	1.1"

	.file	"procbind.s"

	.text

	.globl	_cerror

_fwdef_(`processor_bind'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROCESSOR_BIND,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
