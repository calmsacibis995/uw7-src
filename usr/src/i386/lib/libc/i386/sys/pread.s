.ident	"@(#)libc-i386:sys/pread.s	1.5"

	.file	"pread.s"

_fwdef_(`pread64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PREAD64,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pread64
	jmp	_cerror

_fwdef_(`pread32'):
_fwdef_(`pread'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PREAD,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pread
	jmp	_cerror

.noerror:
	ret
