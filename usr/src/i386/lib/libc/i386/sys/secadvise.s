.ident	"@(#)libc-i386:sys/secadvise.s	1.1.1.2"

/ secadvise


	.file	"secadvise.s"

	.text

	.globl  _cerror

_fwdef_(`secadvise'):
	MCOUNT
	movl	$SECADVISE,%eax
	lcall	$0x7,$0
	jc 	_cerror
	ret
