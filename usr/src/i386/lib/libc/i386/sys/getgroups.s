
.ident	"@(#)libc-i386:sys/getgroups.s	1.2"

/ gid = getgroups();
/ returns effective gid

_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"getgroups.s"

	.text

	.globl  _cerror

_fwdef_(`getgroups'):
	MCOUNT
	movl	$GETGROUPS,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
')
