.ident	"@(#)libc-i386:sys/fpathconf.s	1.2"

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"fpathconf.s"
	
	.text

	.globl	_cerror

_fwdef_(`fpathconf'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FPATHCONF,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
