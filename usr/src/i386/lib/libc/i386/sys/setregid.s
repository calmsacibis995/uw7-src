.ident	"@(#)libc-i386:sys/setregid.s	1.1"

	.file	"setregid.s"

	.text

	.globl _cerror

_fwdef_(`setregid'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETREGID, %eax
	lcall	$0x7, $0
	jc	_cerror
	ret
