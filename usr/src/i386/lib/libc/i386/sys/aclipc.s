/ C library -- aclipc
.ident	"@(#)libc-i386:sys/aclipc.s	1.2"

/ aclipc(cmd, priv_vec, count)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`aclipc'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$ACLIPC,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
