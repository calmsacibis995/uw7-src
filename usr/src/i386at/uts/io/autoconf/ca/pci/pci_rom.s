	.ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci_rom.s	1.1.1.1"
	.ident	"$Header$"
	.file	"io/autoconf/ca/pci/pci_rom.s"

include(KBASE/svc/asm.m4)
include(assym_include)

	
/
/ int
/ _pci_bios_call(regs *r, caddr_t addr)
/
/ Calling/Exit State:
/	<r> is a pointer to a regs structure. It contains values
/	required to initialize %eax, %ebx, %ecx, %esi, %edi etc.
/
/
/ Description:
/	Execute a 32 bit prot. mode BIOS call. This is emulated
/	by calling an entry point passed as the 2d arg (addr)

ENTRY(_pci_bios_call)
	pushl	%ebp
	movl	%esp, %ebp		/ save current stack pointer
	subl	$36, %esp		/ make space for (addr + sizeof(regs))

	movl	8(%ebp), %eax		/ %eax = r
	movl	(%eax), %eax		/ %eax = r->eax 
	movl	%eax, -8(%ebp)

	movl	8(%ebp), %eax		/ %eax = r
	movl	4(%eax), %eax		/ %eax = r->ebx
	movl	%eax, -12(%ebp)	

	movl	8(%ebp), %eax		/ %eax = r
	movl	8(%eax), %eax		/ %eax = r->ecx
	movl	%eax, -16(%ebp)

	movl	8(%ebp), %eax		/ %eax = r
	movl	12(%eax), %eax		/ %eax = r->edx
	movl	%eax, -20(%ebp)

	movl	8(%ebp), %eax		/ %eax = r
	movl	16(%eax), %eax		/ %eax = r->edi
	movl	%eax, -24(%ebp)

	movl	8(%ebp), %eax		/ %eax = r
	movl	20(%eax), %eax		/ %eax = r->esi
	movl	%eax, -28(%ebp)

	movl	8(%ebp), %eax		/ %eax = r
	movl	24(%eax), %eax		/ %eax = r->eflags
	movl	%eax, -32(%ebp)

ifdef(`OLDSTUFF',`
	pushl	$0
	pushl	$0xffff
	pushl	$0xf0000
	call	physmap
	addl	$8, %esp
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %ebx
	addl	$0xf859, %ebx
	movl	%ebx, -4(%ebp)
',`
	movl	12(%ebp), %eax		/ %eax = addr
	movl	%eax, -4(%ebp)
')

	pusha

	movl	-12(%ebp), %eax
	movl	%eax, %ebx

	movl	-16(%ebp), %eax
	movl	%eax, %ecx

	movl	-20(%ebp), %eax
	movl	%eax, %edx

	movl	-24(%ebp), %eax
	movl	%eax, %edi

	movl	-28(%ebp), %eax
	movl	%eax, %esi

	movl	-8(%ebp), %eax

	pushf
	push	%cs
	cli
	call	*-4(%ebp)

	movl	%eax, -36(%ebp)		/ save %eax (call ret val) on stack

	pushfl
	popl	%eax
	movl	%eax, -32(%ebp)		/ save eflags on stack

	sti
	popf

	movl	%ebx, %eax
	movl	%eax, -12(%ebp)		/ save %ebx on stack

	movl	%ecx, %eax
	movl	%eax, -16(%ebp)		/ save %ecx on stack

	movl	%edx, %eax
	movl	%eax, -20(%ebp)		/ save %edx on stack

	movl	%edi, %eax
	movl	%eax, -24(%ebp)		/ save %edi on stack

	movl	%esi, %eax
	movl	%eax, -28(%ebp)		/ save %esi on stack

	movl	8(%ebp), %eax		/ %eax = r

	movl	-36(%ebp), %edx	
	movl	%edx, (%eax)		/ r->eax = %eax

	movl	-12(%ebp), %edx
	movl	%edx, 4(%eax)		/ r->ebx = %ebx

	movl	-16(%ebp), %edx
	movl	%edx, 8(%eax)		/ r->ecx = %ecx

	movl	-20(%ebp), %edx
	movl	%edx, 12(%eax)		/ r->edx = %ecx

	movl	-24(%ebp), %edx
	movl	%edx, 16(%eax)		/ r->edi = %edi

	movl	-28(%ebp), %edx
	movl	%edx, 20(%eax)		/ r->esi = %esi

	movl	-32(%ebp), %edx
	movl	%edx, 24(%eax)		/ r->eflags = %eflags

	popa

	movl	8(%ebp), %eax
	movzbl	1(%eax), %eax
	leave	
	ret	

SIZE(_pci_bios_call)
