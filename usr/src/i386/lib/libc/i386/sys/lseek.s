.ident	"@(#)libc-i386:sys/lseek.s	1.7"

	.file	"lseek.s"

_m4_ifdef_(`GEMINI_ON_OSR5',,_m4_ifdef_(`GEMINI_ON_UW2',,``
_fwdef_(`lseek64'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LSEEK64,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
''))

_fwdef_(`lseek32'):
_fwdef_(`lseek'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LSEEK,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
