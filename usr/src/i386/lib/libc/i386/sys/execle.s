.ident	"@(#)libc-i386:sys/execle.s	1.6"

	.file	"execle.s"

	.text

	.globl	execve

_fwdef_(`execle'):
	_prologue_
	MCOUNT			/ subroutine entry counter if profiling
	leal    _esp_(4),%edx    / argv (retaddr + file arg - 4)
	movl	%edx,%eax

.findzero:
	addl	$4,%eax
	cmpl	$0,(%eax)
	jne	.findzero

	pushl	4(%eax)		/ envp
	leal    _esp_(12),%edx   / argv (push + retaddr + file arg)
	pushl   %edx
	pushl	_esp_(12)	/ file (retaddr + 2 pushes off %esp)
	call	_fref_(execve)
	addl    $12,%esp
	_epilogue_
	ret
