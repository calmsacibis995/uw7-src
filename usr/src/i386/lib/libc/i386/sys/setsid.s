.ident	"@(#)libc-i386:sys/setsid.s	1.3"

/ C library -- setsid, setpgid, getsid, getpgid

	.file	"setsid.s"

	.text

	.globl  _cerror
_m4_ifdef_(`GEMINI_ON_OSR5',`
_m4_define_(`SUB_GETSID', 6)
_m4_define_(`SUB_SETSID', 3)
_m4_define_(`SUB_GETPGID', 4)
_m4_define_(`SUB_SETPGID', 2)
',`
_m4_define_(`SUB_GETSID', 2)
_m4_define_(`SUB_SETSID', 3)
_m4_define_(`SUB_GETPGID', 4)
_m4_define_(`SUB_SETPGID', 5)
')

_fwdef_(`getsid'):
	popl	%edx
	pushl	$SUB_GETSID
	pushl	%edx
	jmp	.pgrp

_fwdef_(`setsid'):
	popl	%edx
	pushl	$SUB_SETSID
	pushl	%edx
	jmp	.pgrp

_fwdef_(`getpgid'):
	popl	%edx
	pushl	$SUB_GETPGID
	pushl	%edx
	jmp	.pgrp

	
_fwdef_(`setpgid'):
	popl	%edx
	pushl	$SUB_SETPGID
	pushl	%edx
	jmp	.pgrp

.pgrp:
	movl	$SETSID,%eax
	lcall	$7,$0
	popl	%edx
	movl	%edx,0(%esp)	/ Remove extra word
	jc	_cerror
	ret

