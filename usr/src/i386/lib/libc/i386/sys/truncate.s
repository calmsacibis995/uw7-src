.ident	"@(#)libc-i386:sys/truncate.s	1.5"

	.file	"truncate.s"

_fwdef_(`truncate64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$TRUNCATE64,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	truncate64
	jmp	_cerror

_fwdef_(`truncate32'):
_fwdef_(`truncate'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$TRUNCATE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	truncate
	jmp	_cerror

.noerror:
	ret
