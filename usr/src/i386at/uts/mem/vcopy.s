	.ident	"@(#)kern-i386at:mem/vcopy.s	1.3.2.1"
	.ident	"$Header$"
	.file	"mem/vcopy.s"

include(KBASE/svc/asm.m4)
include(assym_include)

/ void
/ vcopy(ushort_t *src, ushort_t *dst, size_t cnt, int direction)
/	This routine copies short words in memory.  It is used in 
/	the display driver.
/
/ Calling/Exit State:
/	direction = 0 means from and to are the high addresses (move up)
/	direction = 1 means from and to are the low addresses (move down)

	.align	8
ENTRY(vcopy)
	pushl	%esi
	movl	4+SPARG3, %eax	/ move direction flag to eax register
	pushl	%edi
	movl	8+SPARG0, %esi	/ set source pointer
	movl	8+SPARG1, %edi	/ set destination pointer
	movl	8+SPARG2, %ecx	/ set size of data movement
	orl	%eax, %eax
	jnz	.doit		/ direction flag is zero by default
	std			/ move up; start at high end and decrement
.doit:
	rep
	movsw			/ move word from esi to edi
	cld			/ clear direction flag
	popl	%edi
	popl	%esi
	ret

	SIZE(vcopy)
