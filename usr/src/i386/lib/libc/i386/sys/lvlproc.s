/ C library -- lvlproc
.ident	"@(#)libc-i386:sys/lvlproc.s	1.2"

/ lvlproc(int cmd, level_t *levelp)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`lvlproc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLPROC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
