	.ident "@(#)pnp_bios.s	7.1	10/22/97	12:29:04 "
/
/	"@(#) pnp_bios.s 65.3 97/07/11 "
/
/	Copyright (C) The Santa Cruz Operation, 1988-1997.
/	This Module contains Proprietary Information of
/	The Santa Cruz Operation and should be treated as Confidential.
/
/
/ int
/ PnP_bios_call(u_long Func, ...)
/
/ Description:
/	Execute a 16 bit prot. mode (80286) BIOS call.
/
/ PCI BIOS assumptions:
/	It is quite dangerous to call the PnP BIOS since it is
/	specifically allowed to do nasties like cli and sti. 
/	Testing and experience may show that this method is
/	unusable.
/
/	Few sanity tests are done here.  It is assumed that the
/	caller passes good stuff.
/
/	Since the PnP BIOS interface was so poorly thought out
/	that it requires varargs, we must do the parsing of each
/	function here so that the stack gets setup correctly for
/	each.
/
/	gdt[PNPDATSEL] is MKDSCR(0L,0xFFFFFL,KDATA_ACC1,DATA_ACC2)

	.data
	.align	4

#define _INASSEMBLER	1
#include <sys/seg.h>

	.globl	PnP_BIOS_entry

PnP_BIOS_entry:
	.long	0

pnp_status:
	.long	-1

	.text

	.set	FUNC, 8			/ u_long Func
	.set	ARG1, 12
	.set	ARG2, 16
	.set	ARG3, 20
	.set	ARG4, 24

	.globl	PnP_bios_call
	.align	4

PnP_bios_call:
	pushl	%ebp
	movl	%esp, %ebp		/ save current stack pointer
	pusha
	pushfl				/ save interrupt state
	cli

	movl	FUNC(%ebp), %eax	/ Func

/	cmp	$0x40, %eax
/	jz	pnp_setup_40

	movl	$-1, pnp_status		/ fail
	jmp	pnp_done

pnp_doit:
/	## The problem is that the BIOS can only return to an
/	## address with a 16 bit IP
/
/	## call someplace in low mem as 16 bit prot
/	## have that call the bios
/	## and return to us here in 32 bits
/
/	## The low 4k phys RAM is as it was setup by/for DOS
/	## before our boot.

	pushl	%cs
	pushl	$pnp_return

	.set	PNPCODE0SEL, 0		/ ## We're not using it anyway.
					/ This will be totally rewritten soon.
					/ Don made me do it.
	pushl	$PNPCODE0SEL		/ cs
	movl	PnP_BIOS_entry, %eax	/ ip
	pushl	%eax
	lret				/ *PnP_BIOS_entry()

pnp_return:
	movl	%eax, pnp_status

pnp_done:
	popfl				/ restore interrupts
	popa
	popl	%ebp

	movl	pnp_status, %eax
	ret

/
/ Function 0x40		Get Plug-n-Play ISA Configuration Structure
/
/	ARG1	PnP_ISA_Config_t *Cfg

pnp_setup_40:
	.set	PNPDATSEL, 0		/ ## We're not using it anyway.
					/ This will be totally rewritten soon.
					/ Don made me do it.

	pushw	$PNPDATSEL		/ Bios Selector
	movl	ARG1(%ebp),%ebx		/ Cfg
	movl	%ebx, %ecx
	shr	$16, %ecx
	pushw	%cx			/ Segment of Cfg
	pushw	%bx			/ Offset of Cfg
	pushw	%ax			/ Func
	jmp	pnp_doit

