/	.file	"prot.s"

	.ident	"@(#)stand:i386at/boot/stage1/prot.s	1.3"
	.ident	"$Header$"

/--------------------------------------------------------------------------
/
/	The GDT for protected mode operation and transition back to
/	real-address mode.
/
/	All 32-bit descriptors can reference the entire 4GB address space.
/
/	Note that even the 16-bit data descriptor allows full 4GB access.
/	This enables so-called "big real mode", used by Phoenix's I2O BIOS.
/	We need this since we interrupt their code with our tick handler.

	.data

/	Start of GDT
GDTstart:
/	GDT pointer (in LGDT format)
/	overlaid on top of GDT[0] null descriptor, to save space
GDTptr:
	.value	GDTlim		/ limit
	.long	GDTstart	/ base
	.value	0

code32desc:			/ offset = 0x08	SEL_C32

	.value	0xFFFF		/ segment limit 0..15 (limit = 0xFFFFFFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x9A		/ flags: P=1, A=0, DPL=0, code, C=0, R=1
	.byte	0xCF		/ limit 16..19; flags: AVL=0, G=1, D=1
	.byte	0x00		/ segment base 24..32

data32desc:			/ offset = 0x10	SEL_D32

	.value	0xFFFF		/ segment limit 0..15 (limit = 0xFFFFFFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x92		/ flags: P=1, A=0, DPL=0, data, E=0, W=1
	.byte	0xCF		/ limit 16..19; flags: AVL=0, G=1, B=1
	.byte	0x00		/ segment base 24..32

code16desc:			/ offset = 0x18	SEL_C16

	.value	0xFFFF		/ segment limit 0..15 (limit = 0x0FFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x9A		/ flags: P=1, A=0, DPL=0, code, C=0, R=1
	.byte	0x00		/ limit 16..19; flags: AVL=0, G=0, D=0
	.byte	0x00		/ segment base 24..32

data16desc:			/ offset = 0x20	SEL_D16

	.value	0xFFFF		/ segment limit 0..15 (limit = 0x0FFFFFFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x92		/ flags: P=1, A=0, DPL=0, data, E=0, W=1
	.byte	0x8F		/ limit 16..19; flags: AVL=0, G=1, B=0
	.byte	0x00		/ segment base 24..32

	/ END of GDT
	.set GDTlim, . - GDTstart - 1

	.set SEL_C32, code32desc - GDTstart
	.set SEL_D32, data32desc - GDTstart
	.set SEL_C16, code16desc - GDTstart
	.set SEL_D16, data16desc - GDTstart

	.set PROTMASK, 0x01

	.text

/--------------------------------------------------------------------------
/
/	Enter protected mode.
/
/	We must set up the GDTR and load new segment selectors.
/
/	On entry:	ss = cs = 0 (real-mode selector)
/			ds, es: don't care
/
/	On exit:	cs = SEL_C32
/			ds = es = ss = SEL_D32
/			%ax not preserved
/
/	Called as a 32-bit call.
/
/	NOTE: Even though the Intel manual says to use a far jump to
/	switch to 32-bit mode, this doesn't work for some reason, so
/	we have to use a far return.

	.globl	goprot
goprot:
	/ We don't have an IDT or interrupt handlers which will work in
	/ protected mode, so keep interrupts disabled for the duration.
	cli

	/ Load the GDTR with a pointer to our simple GDT.
	data16 / use 32-bit data-size
	addr16 / use 32-bit address-size
	lgdt	%cs:GDTptr

	/ Turn protected mode on (w/o paging).
	movl	%cr0, %eax
	orb	$PROTMASK, %al		/ "knows" PROTMASK is in LSB
	movl	%eax, %cr0

	/ Flush the instruction queue
	jmp qflush
qflush:

	/ Set up the segment registers, so we can continue like before;
	/ if everything works properly, this shouldn't change anything.
	/ Note that we're still in 16 bit operand and address mode, here,
	/ and we will continue to be until the new %cs is established.

	movb	$SEL_D32, %al		/ 32-bit data descriptor
	xorb	%ah, %ah		/ "knows" SEL_D32 fits in 8 bits
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss		/ don't need to set %esp

	/ The C language calling conventions require the D flag to be clear.
	cld

	/ Return to the caller, with a 32-bit code selector.
	data16 / use 32-bit data-size
	popl	%eax			/ get return %eip
	data16 / use 32-bit data-size
	pushl	$SEL_C32		/ push %cs
	data16 / use 32-bit data-size
	pushl	%eax			/ push %eip
	data16 / use 32-bit data-size
	lret

/--------------------------------------------------------------------------
/	Re-enter real mode.
/
/	We assume that we are executing in protected mode, and
/	that paging has *not* been turned on.  We also assume that
/	the current %cs and %ss segment(s) are based at zero and
/	that the stack pointer (%esp) is less than 64K.
/	Load all segment registers with a selector that points
/	to a descriptor containing the following values:
/
/	Limit = 64k
/	Byte Granular	( G = 0 )
/	Expand up	( E = 0 )
/	Writable	( W = 1 )
/	Present		( P = 1 )
/	Base = any value
/
/	If %eax is zero on entry, clear (at least) upper bits of all 32-bit
/	general registers, except %ebp, in case BIOS (incorrectly) depends on
/	these bits being clear. The caller is expected to take care of %ebp.
/
/	Otherwise, all general regs preserved, except %eax.
/
/	In either case, %eax will be zero on exit.

	.globl	goreal
	.globl	goreal_clear
goreal_clear:
	/ Clear all 32-bit registers in case BIOS depends on upper bits zero.
	xorl	%eax, %eax
	movl	%eax, %ebx
	movl	%eax, %ecx
	movl	%eax, %edx
	movl	%eax, %esi
	movl	%eax, %edi
goreal:

	/ Load all non-code segment registers with a 16-bit data segment, so
	/ the cached descriptors get values appropriate for real-address mode.
	/ (Since we never change %fs and %gs, we don't reload them here.)
	movw	$SEL_D16, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss

	/ Transfer control to a 16 bit code segment, so CS descriptor gets
	/ a value appropriate for real-address mode.
	ljmp	$SEL_C16, $go_C16
go_C16:
	// From here on, we have 16-bit addresses and operands by default.

	/ Turn off protected mode
	movl	%cr0, %eax
	andb	$~PROTMASK, %al		/ "knows" PROTMASK is in LSB
	movl	%eax, %cr0

	/ Do another long-jump. This one flushes the instruction queue, so
	/ remaining instructions are interpreted in real-address mode. According
	/ to Intel, this also "loads the appropriate base and access rights
	/ values in the CS register."
	/
	/ NOTE: Even though the assembler generates a 32-bit offset for the
	/ 2nd operand to this instruction, and the CPU only expects 16 bits,
	/ it's harmless since we just jump over the extra bytes.
	ljmp	$0x0, $restorecs
restorecs:

	/ Set the conditions we expect in our real-address mode code;
	/ specifically, all segment registers are expected to be zero.
	/ (Zero upper bits of %eax while we're at it.)
	data16 / use 32-bit data-size
	xorl	%eax, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss		/ (%esp still OK)

	/ Don't reenable interrupts -- let the caller do it if desired.

	data16 / use 32-bit data-size
	ret				/ 32-bit return, since call was 32-bit
