.ident	"@(#)libc-i386:sys/xstat.s	1.2"

/ OS library -- _xstat

/ error = _xstat(version, string, statbuf)

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"xstat.s"

	.text

	.globl	_cerror
	.globl	_xstat

_fgdef_(_xstat):
	movl	$XSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax,%eax
	ret
')
