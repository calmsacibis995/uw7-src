.ident	"@(#)libc-i386:sys/sigpending.s	1.3"

/ C library -- setsid, setpgid, getsid, getpgid


_m4_ifdef_(`GEMINI_ON_OSR5',`',
`
	.file	"sigpending.s"

	.text

	.globl	__sigfillset
	.globl  _cerror

_fwdef_(`sigpending'):
	popl	%edx
	pushl	$1
	pushl	%edx
	jmp	.sys

_fgdef_(`__sigfillset'):
	popl	%edx
	pushl	$2
	pushl	%edx
	jmp	.sys

.sys:
	movl	$SIGPENDING,%eax
	lcall	$7,$0
	popl	%edx
	movl	%edx,0(%esp)	/ Remove extra word
	jc	_cerror
	ret

')
