.ident	"@(#)libc-i386:sys/lxstat.s	1.2"

/ gid = _lxstat();
/ returns effective gid
_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"lxstat.s"

	.text

	.globl  _cerror
	.globl  _lxstat

_fgdef_(`_lxstat'):
	MCOUNT
	movl	$LXSTAT,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax,%eax
	ret
')
