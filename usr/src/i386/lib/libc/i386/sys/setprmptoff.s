.ident	"@(#)libc-i386:sys/setprmptoff.s	1.1"

	.file	"setprmptoffset.s"

	.text


_fwdef_(`setprmptoffset'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SETPRMPTOFFSET,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
