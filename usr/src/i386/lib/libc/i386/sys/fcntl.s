.ident	"@(#)libc-i386:sys/fcntl.s	1.6"

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"fcntl.s"

	.text


	.globl	_cerror

_fwdef_(`fcntl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FCNTL,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	fcntl
	jmp	_cerror

.noerror:
	ret
')
