/
/	@(#) io386.s 12.1 95/05/09 
/
/	Copyright (C) The Santa Cruz Operation, 1990-1991.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation, and should be treated as Confidential.
/

/
/	SCO MODIFICATION HISTORY
/
/	S001	Wed Dec 16 10:52:57 PST 1992	staceyc@sco.com
/	- Add repinsw()
/	S000	Fri Jul 05 11:39:50 PDT 1991	buckm@sco.com
/	- Add copyright and mod hist headers.
/	- Add ind() and outd();
/	- Add repoutsb(), repoutsw(), and repoutsd().
/

	.text
	.align	4
	.globl	inb
inb:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	subl	%eax,%eax
	inb	(%dx)
	leave
	ret

	.align	4
	.globl	outb
outb:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	outb	(%dx)
	leave
	ret

	.align	4
	.globl	inw
inw:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	subl	%eax,%eax
	inw	(%dx)
	leave
	ret

	.align	4
	.globl	outw
outw:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	outw	(%dx)
	leave
	ret

	.align	4
	.globl	ind
ind:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	inl	(%dx)
	leave
	ret

	.align	4
	.globl	outd
outd:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	outl	(%dx)
	leave
	ret

	.align	4
	.globl	repoutsb
repoutsb:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	8(%ebp),%edx		/ port
	movl	12(%ebp),%esi		/ address
	movl	16(%ebp),%ecx		/ count
	cld
	rep
	outsb
	popl	%esi
	leave
	ret

	.align	4
	.globl	repoutsw
repoutsw:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	8(%ebp),%edx		/ port
	movl	12(%ebp),%esi		/ address
	movl	16(%ebp),%ecx		/ count
	cld
	rep
	outsw
	popl	%esi
	leave
	ret

	.align	4
	.globl	repoutsd
repoutsd:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	8(%ebp),%edx		/ port
	movl	12(%ebp),%esi		/ address
	movl	16(%ebp),%ecx		/ count
	cld
	rep
	outsl
	popl	%esi
	leave
	ret

	.align	4
	.globl	repinsw
repinsw:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	movl	8(%ebp),%edx		/ port
	movl	12(%ebp),%edi		/ address
	movl	16(%ebp),%ecx		/ count
	cld
	rep
	insw
	popl	%esi
	leave
	ret
