.ident	"@(#)libc-i386:sys/cancelblock.s	1.2"

	.file	"cancelblock.s"

	.text


_fwdef_(`cancelblock'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$CANCELBLOCK,%eax
	lcall	$0x7,$0
	ret
