	.ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisarom.s	1.4.1.1"
	.ident	"$Header$"
	.file	"io/autoconf/ca/eisa/eisarom.s"

include(KBASE/svc/asm.m4)
include(assym_include)

/
/ int
/ _eisa_rom_call(regs *r, caddr_t addr)
/
/ Calling/Exit State:
/	<r> is a pointer to a regs structure. It contains values
/	required to initialize %eax, %ebx, %ecx, %esi, %edi etc.
/
/	<addr> is the virtual address of 0xf000:f859 (0xff859) of 
/	the EISA BIOS entry point.
/
/ Description:
/	Execute the protected-mode int 15 BIOS call. This is emulated
/	by calling an entry point stored at address 0xf000:f859
/
/ TODO:
/	Extend the existing interface to pass in the entry point
/	that can be called to simulate any bimodal BIOS call.
/
/ Note:
/	When the %esp or %ebp register is used as the base, the %ss
/	segment is the default segment. In all other cases, the %ds
/	segment is the default selection. Since the int 15 BIOS
/	routines require 1536 bytes for temporary RAM variables,
/	we use %ebp register to contain the call address to BIOS
/	entry point.
/
/	The regs structure is shown below for reference.
/
/	typedef struct {
/		union {
/			unsigned int eax;
/			struct {
/				unsigned short ax;
/			} word;
/			struct {
/				unsigned char al;
/				unsigned char ah;
/			} byte;
/		} eax;
/
/		union {
/			unsigned int ebx;
/			struct {
/				unsigned short bx;
/			} word;
/			struct {
/				unsigned char bl;
/				unsigned char bh;
/			} byte;
/		} ebx;
/
/		union {
/			unsigned int ecx;
/			struct {
/				unsigned short cx;
/			} word;
/			struct {
/				unsigned char cl;
/				unsigned char ch;
/			} byte;
/		} ecx;
/
/		union {
/			unsigned int edx;
/			struct {
/				unsigned short dx;
/			} word;
/			struct {
/				unsigned char dl;
/				unsigned char dh;
/			} byte;
/		} edx;
/
/		union {
/			unsigned int edi;
/			struct {
/				unsigned short di;
/			} word;
/		} edi;
/
/		union {
/			unsigned int esi;
/			struct {
/				unsigned short si;
/			} word;
/		} esi;
/
/		unsigned int eflags;
/
/	} regs;
/
/
/	Below is the stack frame:
/
/	-------------------------
/	| return value		| %ebp - 36
/	-------------------------
/	| r->eflags		| %ebp - 32
/	-------------------------
/	| r->esi		| %ebp - 28
/	-------------------------
/	| r->edi		| %ebp - 24
/	-------------------------
/	| r->edx		| %ebp - 20
/	-------------------------
/	| r->ecx		| %ebp - 16
/	-------------------------
/	| r->ebx		| %ebp - 12
/	-------------------------
/	| r->eax		| %ebp - 08
/	-------------------------
/	| addr			| %ebp - 04
/	-------------------------
/	| %ebp			| %ebp
/	-------------------------
/	| return addr		| %ebp + 04
/	-------------------------
/	| 1st arg (ptr to regs)	| %ebp + 08
/	-------------------------
/	| 2nd arg (addr)	| %ebp + 12
/	-------------------------
/

ENTRY(_eisa_rom_call)
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
	call	*-4(%ebp)		/ Note: int 15 pops the eflags

	movl	%eax, -36(%ebp)		/ save %eax (call ret val) on stack

	pushfl
	popl	%eax
	movl	%eax, -32(%ebp)		/ save eflags on stack

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

SIZE(_eisa_rom_call)
