.ident	"@(#)libc-i386:sys/unblock.s	1.1"

	.file	"unblock.s"

	.text


_fwdef_(`unblock'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$UNBLOCK,%eax
	lcall	$0x7,$0
	jc	.errorret
	movl	$0x0,%eax
.errorret:
	ret
