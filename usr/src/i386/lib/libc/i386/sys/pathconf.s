.ident	"@(#)libc-i386:sys/pathconf.s	1.2"

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"pathconf.s"
	
	.text

	.globl	_cerror

_fwdef_(`pathconf'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PATHCONF,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
