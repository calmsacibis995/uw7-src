#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/_times.s	1.2"
#ident	"$Header$"
/		copyright	"%c%"

.ident		"@(#)ucb:i386/ucblib/libc/i386/sys/_times.s	1.2"


	.file	"times.s"

	.set	TIMES, 43

	.text

	.globl	_cerror

times:
	movl	$TIMES,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
