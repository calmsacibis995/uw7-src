.ident	"@(#)libc-i386:sys/fstatvfs.s	1.2"

	.file	"fstatvfs.s"

_fwdef_(`fstatvfs64'):
	MCOUNT
	movl	$FSTATVFS64,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret

_fwdef_(`fstatvfs32'):
_fwdef_(`fstatvfs'):
	MCOUNT
	movl	$FSTATVFS,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret
