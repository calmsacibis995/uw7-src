.ident	"@(#)libc-i386:sys/ftruncate.s	1.5"

	.file	"ftruncate.s"

_fwdef_(`ftruncate64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FTRUNCATE64,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	ftruncate64
	jmp	_cerror

_fwdef_(`ftruncate32'):
_fwdef_(`ftruncate'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FTRUNCATE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	ftruncate
	jmp	_cerror

.noerror:
	ret
