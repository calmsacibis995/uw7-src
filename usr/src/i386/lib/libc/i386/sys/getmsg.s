.ident	"@(#)libc-i386:sys/getmsg.s	1.8"

	.file	"getmsg.s"

	.text


	.globl	_cerror

_fwdef_(`getmsg'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$GETMSG,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	getmsg
	jmp	_cerror
.noerror:
	ret
