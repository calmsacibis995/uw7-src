.ident	"@(#)libc-i386:sys/mmap.s	1.5"

	.file	"mmap.s"

_fwdef_(`mmap64'):
	MCOUNT
	movl	$MMAP64,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
_fwdef_(`mmap32'):
_fwdef_(`mmap'):
	MCOUNT
	movl	$MMAP,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
