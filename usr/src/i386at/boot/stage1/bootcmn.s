/	.file	"bootcmn.s"

	.ident	"@(#)stand:i386at/boot/stage1/bootcmn.s	1.4"
	.ident	"$Header$"

	.text

/-------------------------------------------------------------------------------
/
/	Console driver character output function.
/
/	On entry, the single stack argument is the character to output.
/
	.globl	cons_putc
cons_putc:
	/ ENTERED IN PROTECTED MODE

	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	/ SWITCH TO REAL-ADDRESS MODE (and clear regs)
	call	goreal_clear
	sti				/ enable interrupts in real-mode

	movb	$0xE, %ah	/ "write TTY" function
	addr16 / use 32-bit address-size
	movb	20(%esp),%al	/ character to output
				/ page number in %bh is zero

	int	$0x10		/ video BIOS call

	.globl	do_C_ret
do_C_ret:
	/ SWITCH BACK TO PROTECTED MODE
	data16 / use 32-bit data-size
	call	goprot

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp

	ret

/-------------------------------------------------------------------------------
/
/	Abort function.
/
/	This function never returns. It is called on non-recoverable errors.
/	Switch to real-address mode, so Ctrl-Alt-DEL interrupt will reboot.
/
	.globl	abort
abort:
	/ ENTERED IN PROTECTED MODE

	/ SWITCH TO REAL-ADDRESS MODE (don't care about regs)
	call	goreal
	sti				/ enable interrupts in real-mode

	/ We'd like to use INT 18, but too many BIOSes do funky things,
	/ like saying they can't load BASIC, or clearing the screen and
	/ losing our error messages. So, we'll just spin here.
	jmp	.

/-------------------------------------------------------------------------------
/
/	BIOS call support function.
/
/	Allows protected-mode stage2 (or stage3) code to invoke BIOS calls
/	(which must be invoked from memory below 1MB, in real-address mode).
/
/	On entry, all registers will be set for the BIOS call, except
/	segment registers. The single (32-bit) stack argument contains the
/	interrupt vector contents. Any pointers in registers will be linear
/	addresses that fit into 16 bits. The EAX register must be non-zero.
/
/	On exit, all general registers will be as returned from the software
/	interrupt, including the CF bit in EFL.
/
/	Because of the restriction on pointers, in practice the caller will
/	only be able to use pointers into the disk buffer exported through
/	d_buffer in stage1parms.
/
	.globl	bios_intcall
bios_intcall:
	/ ENTERED IN PROTECTED MODE

	pushl	%eax		/ save %eax for BIOS call

	/ SWITCH TO REAL-ADDRESS MODE (don't clear regs)
	call	goreal
	sti				/ enable interrupts in real-mode

	data16 / use 32-bit data-size
	popl	%eax

	/ Simulate software interrupt, using vector contents pushed on stack
	pushf
	addr16 / use 32-bit address-size
	lcall	*6(%esp)

	/ This is a HACK. Some BIOS calls we need return a value in %es,
	/ but none return anything in %di.
	movw	%es, %di

	data16 / use 32-bit data-size
	pushl	%eax
	lahf
	push	%eax

	/ SWITCH BACK TO PROTECTED MODE
	data16 / use 32-bit data-size
	call	goprot

	popw	%ax
	sahf
	popl	%eax

	ret

/--------------------------------------------------------------------------
/	Make a protected-mode call from real-address mode.
/
/	The address of the (void) function to call is passed in %edx.
/	All general registers except %eax are preserved, unless modified
/	by the target function.
/
/	Must be called as a 32-bit call. Interrupts are left disabled.

	.globl	protcall
protcall:
	/ ENTERED IN REAL-ADDRESS MODE

	/ SWITCH TO PROTECTED MODE
	data16 / use 32-bit data-size
	call	goprot

	call	*%edx

	/ SWITCH BACK TO REAL-ADDRESS MODE (don't clear regs)
	call	goreal

	data16 / use 32-bit data-size
	ret
