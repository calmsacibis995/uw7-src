.ident	"@(#)libc-i386:sys/rdblock.s	1.1"

	.file	"rdblock.s"

	.text


_fwdef_(`rdblock'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$RDBLOCK,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
