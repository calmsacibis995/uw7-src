/ C library -- lvlipc
.ident	"@(#)libc-i386:sys/lvlipc.s	1.2"

/ lvlipc(int type, int id, int cmd, level_t *levelp)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`lvlipc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LVLIPC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
