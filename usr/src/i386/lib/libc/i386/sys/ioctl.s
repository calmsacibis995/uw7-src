.ident	"@(#)libc-i386:sys/ioctl.s	1.9"

_m4_ifdef_(`GEMINI_ON_OSR5',`',`
	.file	"ioctl.s"

	.globl	_xioctl
	.type	_xioctl,"function"
_xioctl:
	MCOUNT			/ subroutine entry counter if profiling
	popl	%eax		/ replace initial (version) argument with
	movl	%eax,(%esp)	/ return address as if had called ioctl()
	movl	$IOCTL,%eax
	lcall	$0x7,$0
	pushl	(%esp)		/ reset to expected number of arguments
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	_xioctl	
	jmp	_cerror

_fwdef_(`ioctl'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$IOCTL,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	ioctl	
	jmp	_cerror

.noerror:
	ret
')
