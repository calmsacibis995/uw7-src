.ident	"@(#)libc-i386:sys/getsetitimer.s	1.1"

/ setitimer


	.file	"getsetitimer.s"

	.text

	.globl  _cerror

_fwdef_(`setitimer'):
	MCOUNT
	movl	$SETITIMER,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret

/ getitimer

_fwdef_(`getitimer'):
	MCOUNT
	movl	$GETITIMER,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret
