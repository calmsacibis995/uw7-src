.ident	"@(#)libc-i386:sys/creat.s	1.7"

	.file	"creat.s"

_fwdef_(`creat64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$CREAT64,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

_fwdef_(`creat32'):
_fwdef_(`creat'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$CREAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
