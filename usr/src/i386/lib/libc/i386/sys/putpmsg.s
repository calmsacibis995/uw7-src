.ident	"@(#)libc-i386:sys/putpmsg.s	1.5"

/ gid = putpmsg();
/ returns effective gid

	.file	"putpmsg.s"

	.text


	.globl  _cerror

_fwdef_(`putpmsg'):
	MCOUNT
	movl	$PUTPMSG,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	putpmsg
	jmp	_cerror
.noerror:
	ret
