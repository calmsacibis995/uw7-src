.ident	"@(#)libc-i386:sys/sendv.s	1.2"

	.file	"sendv.s"

_fwdef_(`sendv64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SENDV64,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	sendv64
	jmp	_cerror

_fwdef_(`sendv32'):
_fwdef_(`sendv'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SENDV,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	sendv32
	jmp	_cerror

.noerror:
	ret
