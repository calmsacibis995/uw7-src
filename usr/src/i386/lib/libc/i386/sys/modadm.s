.ident	"@(#)libc-i386:sys/modadm.s	1.1"

	.file	"modadm.s"
	
	.text

	.globl	_cerror

_fwdef_(`modadm'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$MODADM,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
