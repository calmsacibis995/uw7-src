/ C library -- lvldom
.ident	"@(#)libc-i386:sys/lvldom.s	1.2"

/ lvldom(level_t *level1p, level_t *level2p)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`lvldom'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLDOM,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
