.ident	"@(#)libc-i386:sys/prepblock.s	1.1"

	.file	"prepblock.s"

	.text


_fwdef_(`prepblock'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PREPBLOCK,%eax
	lcall	$0x7,$0
	ret
