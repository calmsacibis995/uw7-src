	.file	"mmu_asm.s"

	.ident	"@(#)stand:i386/boot/stage3/mmu_asm.s	1.1"
	.ident	"$Header$"

/ 	MMU Support requiring special instructions.

	.set	CR0_PG, 0x80000000

	.text

	.globl	mmu_paging_on
	.type	mmu_paging_on, @function
mmu_paging_on:
	/ Load page directory
	movl	l1pt, %eax
_mmu_paging_on:
	movl	%eax, %cr3
	/ Enable paging
	movl	%cr0, %eax
	orl	$CR0_PG, %eax
	movl	%eax, %cr0

	ret

	.size	mmu_paging_on, . - mmu_paging_on


	.globl	mmu_paging_resume
	.type	mmu_paging_resume, @function
mmu_paging_resume:
	movl	save_l1pt, %eax
	jmp	_mmu_paging_on

	.size	mmu_paging_resume, . - mmu_paging_resume


	.globl	mmu_paging_off
	.type	mmu_paging_off, @function
mmu_paging_off:
	mov	%cr3, %eax
	mov	%eax, save_l1pt
	/ Enable paging
	movl	%cr0, %eax
	andl	$~CR0_PG&0xFFFFFFFF, %eax
	movl	%eax, %cr0

	ret

	.size	mmu_paging_off, . - mmu_paging_off


	.data
save_l1pt:
	.long	0
