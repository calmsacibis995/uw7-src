.ident	"@(#)libc-i386:sys/fxstat.s	1.2"

/ error = _fxstat(file, statbuf);
/ char statbuf[34]

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`

	.file	"fxstat.s"

	.text
	
	.globl  _cerror
	.globl  _fxstat

_fgdef_(`_fxstat'):
	MCOUNT
	movl	$FXSTAT,%eax
	lcall	$0x7,$0	
	jc 	_cerror
	xorl	%eax,%eax
	ret
')
