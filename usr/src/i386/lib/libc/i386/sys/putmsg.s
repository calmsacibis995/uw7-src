.ident	"@(#)libc-i386:sys/putmsg.s	1.8"

	.file	"putmsg.s"

	.text


	.globl	_cerror

_fwdef_(`putmsg'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PUTMSG,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	putmsg
	jmp	_cerror
.noerror:
	ret
