/ C library -- fdevstat
.ident	"@(#)libc-i386:sys/fdevstat.s	1.2"

/ fdevstat(int fildes, int cmd, struct devstat *bufp)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`fdevstat'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FDEVSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
