/ C library -- secsys
.ident	"@(#)libc-i386:sys/secsys.s	1.1"

	.globl	_cerror

_fwdef_(`secsys'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SECSYS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
