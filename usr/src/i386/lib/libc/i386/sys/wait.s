.ident	"@(#)libc-i386:sys/wait.s	1.7"


	.file	"wait.s"

	.text


_fwdef_(`wait'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$WAIT,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	wait
	jmp	_cerror

.noerror:
	movl	4(%esp),%ecx
	testl	%ecx,%ecx
	jz	.return
	movl	%edx,(%ecx)
.return:
	ret

_m4_ifdef_(`GEMINI_ON_OSR5',`
	.globl	_osr5_waitpid
_fgdef_(`_osr5_waitpid'):
	MCOUNT
	pushf
	pop	%eax
	orl	$WAITPIDF,%eax  / set parity, sign, zero and overflow flags
	push	%eax
	popf

	movl	$WAIT,%eax
	lcall	$0x7,$0
	jae	.noerror2
	cmpb	$ERESTART,%al
	je	_osr5_waitpid
	jmp	_cerror

.noerror2:
	movl	8(%esp),%ecx
	testl	%ecx,%ecx
	jz	.return2
	movl	%edx,(%ecx)
.return2:
	ret
',`')
