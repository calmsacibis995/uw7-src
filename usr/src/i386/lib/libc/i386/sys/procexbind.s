.ident	"@(#)libc-i386:sys/procexbind.s	1.1"

	.file	"procexbind.s"

	.text

	.globl	_cerror

_fwdef_(`processor_exbind'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PROCESSOR_EXBIND,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
