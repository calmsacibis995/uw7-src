.ident	"@(#)libc-i386:sys/getrlimit.s	1.3"

	.file	"getrlimit.s"

_fwdef_(`getrlimit64'):
	MCOUNT
	movl	$GETRLIMIT64,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax, %eax
	ret

_fwdef_(`getrlimit32'):
_fwdef_(`getrlimit'):
	MCOUNT
	movl	$GETRLIMIT,%eax
	lcall	$0x7,$0
	jc	_cerror
	xorl	%eax, %eax
	ret
