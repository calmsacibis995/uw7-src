	.ident	"@(#)kern-i386at:svc/pstart.s	1.22.6.1"
	.ident	"$Header$"
	.file	"svc/pstart.s"

include(KBASE/svc/asm.m4)
include(assym_include)

define(SEQ, 0)
define(STAMP, `
	define(`SEQ', incr(SEQ))
	movb	$SEQ, [SEQ + 0x800]
')
define(RSTAMP, `
	.byte	0x67			/ 32-bit address-size
	STAMP
')

	.set	KCS32, 0x08
	.set	KDS, 0x10
	.set	KCS16, 0x18
	.set	KDS64K, 0x20
	.set	PROTMASK, 0x1                     /* MSW protection bit */
	.set	NOPROTMASK, 0xfffffffe

/
/ void
/ reset_code_template(void)
/
/	This code is not really a C callable procedure but instead
/	is the first kernel code executed by an application processor
/	when it is started by the PSM.
/
/ Calling State/Exit State:
/
/	Unfortunately, there exist non-conformant PSMs which violate the
/	specification and start in real mode. Therefore, this code
/	is copied into low (under 1MB) memory if possible, wherein it
/	can safely start in real mode. However, the address at which
/	it will start is unknown at assembly time, requiring that the code
/	be position independent.
/
/	In order for this code to be position independent 
/	it is necessary for some fixups to be performed
/	when the code is relocated.
/
/	This code is also capable of starting in protected mode for
/	properly implemented PSMs. In that case, it assumes that
/	a minimal stack is available. Actually, just 4 bytes of stack
/	suffice.
/
/       This function never returns since there is no context to return
/       to. It calls the `reset_function' function as its last action.
/
/ Description:
/
/       Should reset_code (erroneously) be invoked in REAL-MODE, it will
/       transition to protected mode (segmentation only, no paging)
/       The segment setup is BASE equal 0 with the limit set to 4 Gig.
/

	.text

ENTRY(reset_code_template)
	cli				/ just in case...
        movl    %cr0, %eax              / find out which mode
        testb   $_A_CR0_PE, %al         / test PE bit
        jz      reset_real_template

	/
	/ starting in protected mode
	/
	/ reset_prot_template must be fixed up (when this code is relocated).
	/
	/ WARNING: Do not add instructions or instruction prefixes
	/	   between label reset_prot_template and the movl
	/	   instruction - for it would break the fixup
	/	   mechanism.
	/
reset_prot_template:			/ FIXUP target: reset_prot_template+1
	movl	$reset_code_template, %esi
	/
	/ Utilize the assumption that a stack is availabe in order to
	/ recover eip. We need just 4 bytes of stack.
	/
	call	.recover_eip
.recover_eip:
	popl	%eax
	lgdt	%cs:[GDTptr - .recover_eip](%eax)
	leal	_A_PAGESIZE(%esi), %esp	/ sets up our stack
	movw	$KDS, %ax
	movw	%ax, %ss
	/
	/ Use an lret to set %cs base to 0, simultaneously
	/ asjusting %eip to become 0 relative. We use lret
	/ become ljump doesn't have an indirect mode.
	/
	pushl	$KCS32
	leal	[.next - reset_code_template](%esi), %ebx
	pushl	%ebx
	lret

	/
	/ reset_real_template will be fixed up (when this code is relocated).
	/
	/ WARNING: Do not add instructions or instruction prefixes
	/	   between label reset_real_template and the movl
	/	   instruction - for it would break the fixup
	/	   mechanism. A single prefix is normal here.
	/
	/ WARNING: Do not use a `.byte 0x66' stule  prefix following a
	/	   label.  The assembler will generate alignment nops
	/	   *following* the prefix!
	/
reset_real_template:			/ FIXUP target: reset_real_template+2
	data16				/ 32-bit data-size
	movl	$reset_code_template, %esi	

	/
	/ decompse into ds:edi
	/
	.byte	0x66			/ 32-bit data-size
	xor	%edi, %edi
	movl	%esi, %edi		/ really %si -> %di
	.byte	0x66			/ 32-bit data-size
	movl	%esi, %edx
	xor	%edx, %edx		/ really %dx
	.byte	0x66			/ 32-bit data-size
	shrl	$4, %edx
	movw	%dx, %ds

	.byte	0x67			/ 32-bit address-size
	.byte	0x66			/ 32-bit data-size
	lgdt	[GDTptr - reset_code_template](%edi)
	orb     $_A_CR0_PE, %al		/ turn on PE bit in saved CR0 image
	movl    %eax, %cr0		/ turn on protected 32-bit mode
	.byte	0x66			/ 32-bit data-size
	movl	$KDS, %eax		/ select GDT entry for 32-bit data seg
	movw	%ax, %ss
	.byte	0x67			/ 32-bit address-size
	.byte	0x66			/ 32-bit data-size
	leal	_A_PAGESIZE(%esi), %esp	/ sets up our stack

	/
	/ Use an lret to set %cs base to 0, simultaneously
	/ asjusting %eip to become 0 relative. We use lret
	/ become ljump doesn't have an indirect mode.
	/
	.byte	0x66			/ 32-bit data-size
	pushl	$KCS32
	.byte	0x67			/ 32-bit address-size
	.byte	0x66			/ 32-bit data-size
	leal	[.next - reset_code_template](%esi), %ebx
	.byte	0x66			/ 32-bit data-size
	pushl	%ebx
	.byte	0x66			/ 32-bit data-size
	lret

.next:
	movw    %ax, %es		/ load remaining segment registers
	movw    %ax, %ds

	/
	/ VIRT = PHYS for the entire 4 Gig virtual address space.
	/

	/
	/ enable paging
	/
	movl	[reset_cr3_template - reset_code_template](%esi), %eax
	movl	%eax, %cr3
	movl	[reset_cr4_template - reset_code_template](%esi), %eax
	testl	%eax, %eax
	jz	.do_enable
	movl	%eax, %cr4
.do_enable:
	movl	[reset_cr0_template - reset_code_template](%esi), %eax
	movl	%eax, %cr0

	/
	/ call into the OS proper
	/
	jmp    *[reset_function_template - reset_code_template](%esi)

	.globl	reset_real_template
	.globl	reset_prot_template

SIZE(reset_code_template)

/
/ void
/ switch_pae_template(pte_t *pkpdpt)
/
/	Switch from non-PAE to PAE mode.
/
/ Calling/Exit State:
/
/	This code does not actually execute in non-PAE where the
/	linker places it. Rather it must execute with P==V, for
/	it executes in three modes:
/
/		1) non-PAE mode with paging enabled under the
/		   boot loader page tables
/
/		2) protected mode with paging disabled
/
/		3) PAE mode with paging enabled under the
/		   new page tables
/

ENTRY(switch_pae_template)

	movl	4(%esp), %eax

	movl	%cr0, %ecx
	andl	$[~0x80000000 & 0xffffffff], %ecx	/ paging off
	movl	%ecx, %cr0

	movl	%cr4, %edx
	orl	$_A_CR4_PAE, %edx		/ enable PAE mode
	movl	%edx, %cr4

	movl	%eax, %cr3			/ install new page table root

	orl	$_A_CR0_PG, %ecx		/ paging on
	movl	%ecx, %cr0

	ret

SIZE(switch_pae_template)

	.align	8
GDTstart:
nulldesc:			/ offset = 0x0

	.value	0x0	
	.value	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	

kcs32desc:			/ offset = 0x8

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x9A		/ flags; R=1, A=0, Type=110, DPL=00, P=1
	.byte	0xCF		/ flags; Limit (16..19)=0xF, AVL=0, G=1, D=1
	.byte	0x0		/ segment base 24..31

kdsdesc:			/ offset = 0x10

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
	.byte	0xCF		/ flags; Limit (16..19)=0xF, AVL=0, G=1, B=1
	.byte	0x0		/ segment base 24..31

kcs16desc:			/ offset = 0x18

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x9A		/ flags; R=1, A=0, Type=110, DPL=00, P=1
	.byte	0x0		/ flags; Limit (16..19)=0, AVL=0, G=0, D=0
	.byte	0x0		/ segment base 24..31

kds64kdesc:			/ offset = 0x20

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
	.byte	0x40		/ flags; Limit (16..19)=0, AVL=0, G=0, B=1
	.byte	0x0		/ segment base 24..31

	.set	gdtsize,.-GDTstart

	/ In-memory GDT pointer for the lgdt call

	.globl	GDTptr
	.type	GDTptr,@object
GDTptr:	
gdtlimit:
	.value	gdtsize - 1
gdtbase_template:		/ FIXUP target: gdtbase_template
	.long	GDTstart
	.size	GDTptr,.-GDTptr

reset_cr0_template:
	.long	0
reset_cr3_template:
	.long	0
reset_cr4_template:
	.long	0
reset_function_template:
	.long	0
	.globl	reset_cr0_template
	.globl	reset_cr3_template
	.globl	reset_cr4_template
	.globl	reset_function_template
	.globl	gdtbase_template

	/ 
	/ The rest of the page (after relocation) is used for
	/ the stack.
	/
