.ident	"@(#)libc-i386:sys/setreuid.s	1.1"

	.file	"setreuid.s"

	.text

	.globl	_cerror

_fwdef_(`setreuid'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETREUID, %eax
	lcall	$0x7, $0
	jc	_cerror
	ret
