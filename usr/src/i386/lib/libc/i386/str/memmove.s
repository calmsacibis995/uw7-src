	.file	"memmove.s"

	.ident	"@(#)libc-i386:str/memmove.s	1.1"

	.globl	memmove
	.align	4

_fgdef_(memmove):
	MCOUNT			/ profiling
	pushl	%edi
	pushl	%esi
	movl	12(%esp),%edi	/ %edi = dest address
	movl	16(%esp),%esi	/ %esi = source address
	movl	20(%esp),%ecx	/ %ecx = length of string
	movl	%edi,%eax	/ return value from the call

	cmpl	%esi,%edi
	jle	.fwd
	leal	-1(%esi,%ecx),%edx
	cmpl	%edx,%edi
	jg	.fwd

	std
	movl	%edx,%esi
	leal	-1(%edi,%ecx),%edi

	movl	%ecx,%edx
	andl	$0x3,%ecx
	rep ; smovb

	subl	$3,%esi
	subl	$3,%edi
	movl	%edx,%ecx
	shrl	$2,%ecx
	rep ; smovl
	cld
	jmp	.done

.fwd:
	movl	%ecx,%edx
	shrl	$2,%ecx		/ %ecx = number of words to move
	rep ; smovl		/ move the words

	movl	%edx,%ecx	/ %ecx = number of bytes to move
	andl	$0x3,%ecx	/ %ecx = number of bytes left to move
	rep ; smovb		/ move the bytes

.done:
	popl	%esi		/ restore register variables
	popl	%edi		/ restore register variables
	ret
