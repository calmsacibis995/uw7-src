.ident	"@(#)libc-i386:sys/write.s	1.7"


	.file	"write.s"

	.text


_fwdef_(`write'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$WRITE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	write
	jmp	_cerror

.noerror:
	ret
