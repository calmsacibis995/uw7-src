.ident	"@(#)libc-i386:sys/nuname.s	1.2"

/ gid = nuname();
/ returns effective gid

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`

	.file	"nuname.s"

	.text

	.globl  _cerror

_fwdef_(`nuname'):
	MCOUNT
	movl	$NUNAME,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
