/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */

#ident	"@(#)fprof:i386/machdep.h	1.2"

#include <sys/sysi86.h>
#include <sys/seg.h>
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/pit.h>

#undef CLKNUM
#define CLKNUM 11931

#define PIT_LATCH 0xC2

#define setperm() (_abi_sysi86(SI86IOPL,3) >= 0)

extern ulong * Lboltp;

/*
 *	Assembler macro for constructing the timestamp on i386.
 *	The equivalent C code is:
 *	#define casmtimer()\
 *		x = splhi();\
 *		outb(PITCTL_PORT, PIT_LATCH);\
 *		status = inb(PITCTR0_PORT);\
 *		timestamp = (lbolt << 16) | \
 *				inb(PITCTR0_PORT) |\
 *				(inb(PITCTR0_PORT << 8);\
 *		if (status & CLK_OUT_HIGH)\
 *			timestamp += CLKNUM;\
 *		splx(x);
 */

 asm int
asmtimer()
{
%	lab	noinc;

	movw	$PIT_LATCH, %ax		/ pit command reg == PIT_LATCH
	movw	$PITCTL_PORT, %dx	/ set pit control port address
	pushl	%ebx			/ save current contents of %ebx
	pushf				/ save current state
	cli				/ disable interrupts
	outb	(%dx)			/ latch pit
	/ jmp .+2
	movw	$PITCTR0_PORT, %dx	/ set pit counter 0 address
	subl	%eax, %eax		/ zero the destination register
	inb	(%dx)			/ read status byte
	/ jmp .+2
	movl	%eax, %ebx		/ save status in register ebx
	inb	(%dx)			/ read lsb
	/ jmp .+2
	/ movl	Lboltp, %ecx		/ copy lbolt
	/ movl	(%ecx), %ecx		/ copy lbolt
	/ shll	$0x10, %ecx		/ shift lbolt left 16
	/ shll	$0x8, %eax		/ should not shift lsb left 8
	/ orl	%eax, %ecx		/ or in the lsb
	movl	%eax, %ecx		/ move in the lsb
	inb	(%dx)			/ read msb
	popf				/ reenable the interrupts
	shll	$0x8, %eax		/ shift msb left 8
	orl	%ecx, %eax		/ or in the lsb and lbolt
	testl	$0x80, %ebx		/ test if clock OUT is high
	je	noinc			/ if not, don't adjust
	addl	$CLKNUM, %eax		/ otherwise, do adjust
noinc:	shrl	$0x1, %eax			/ divide by 2
	popl	%ebx			/ restore previous value of %ebx
}
