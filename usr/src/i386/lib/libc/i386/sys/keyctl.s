.ident	"@(#)libc-i386:sys/keyctl.s	1.1"

	.file	"keyctl.s"

	.text

	.globl	_cerror

_fwdef_(`keyctl'):
	MCOUNT
	movl	$KEYCTL, %eax
	lcall	$0x7, $0
	jc	_cerror
	ret
