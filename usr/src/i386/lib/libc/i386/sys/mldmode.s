/ C library -- mldmode
.ident	"@(#)libc-i386:sys/mldmode.s	1.2"

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`mldmode'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MLDMODE,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
