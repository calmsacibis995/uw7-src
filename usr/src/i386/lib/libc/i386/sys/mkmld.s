/ C library -- mkmld
.ident	"@(#)libc-i386:sys/mkmld.s	1.2"

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.globl	_cerror

_fwdef_(`mkmld'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MKMLD,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
