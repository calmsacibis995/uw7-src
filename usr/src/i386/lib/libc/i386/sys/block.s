.ident	"@(#)libc-i386:sys/block.s	1.2"

	.file	"block.s"

	.text

	.set	EINTR,4


_fwdef_(`block'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$BLOCK,%eax
	lcall	$0x7,$0
/	jae	.noerror	/ .noerror should simply ret(urn)
/	cmpb	$ERESTART,%al
/	je	block
	jc	.swerror
	ret
.swerror:
	cmpb	$ERESTART,%al
	je	.reerror
	ret
.reerror:
	movl	$EINTR,%eax
	ret
