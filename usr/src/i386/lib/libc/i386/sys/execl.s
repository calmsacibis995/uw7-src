.ident	"@(#)libc-i386:sys/execl.s	1.6"

	.file	"execl.s"

	.text

	.globl	execv

_fwdef_(`execl'):
	_prologue_
	MCOUNT			/ subroutine entry counter if profiling
	leal	_esp_(8),%eax	/ address of args (retaddr + 1 arg)
	pushl	%eax
	pushl	_esp_(8)		/ filename (push + retaddr)
	call	_fref_(execv)
	addl	$8,%esp
	_epilogue_
	ret
