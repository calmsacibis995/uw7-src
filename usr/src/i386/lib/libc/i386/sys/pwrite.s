.ident	"@(#)libc-i386:sys/pwrite.s	1.5"

	.file	"pwrite.s"

_fwdef_(`pwrite64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PWRITE64,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pwrite64
	jmp	_cerror

_fwdef_(`pwrite32'):
_fwdef_(`pwrite'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PWRITE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pwrite
	jmp	_cerror

.noerror:
	ret
