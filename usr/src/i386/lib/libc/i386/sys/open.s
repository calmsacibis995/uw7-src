.ident	"@(#)libc-i386:sys/open.s	1.8"

	.file	"open.s"
	
	.set	O_LARGEFILE,0x80000	/same value as in sys/fcntl.h

_fwdef_(`open64'):
	orl	$O_LARGEFILE,8(%esp)
_fwdef_(`open32'):
_fwdef_(`open'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$OPEN,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	open
	jmp	_cerror

.noerror:
	ret
