/* $XConsortium: bank.s,v 1.5 95/06/19 18:59:11 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ati/bank.s,v 3.2 1994/10/29 22:45:42 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * These are here the very lowlevel VGA bankswitching routines.
 * The segment to switch to is passed via %eax. Only %eax and %edx my be used
 * without saving the original contents.
 *
 * WHY ASSEMBLY LANGUAGE ???
 *
 * These routines must be callable by other assembly routines. But I don't
 * want to have the overhead of pushing and poping the normal stack-frame.
 *
 * Enhancements to support most VGA Wonder cards (including Plus and XL)
 * by Doug Evans, dje@sspiff.UUCP.
 * ALL DISCLAIMERS APPLY TO MY ADDITIONS AS WELL.
 *
 * Changes to enhance support for V3, Mach32 and Mach64 boards
 * by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 * ALL DISCLAIMERS APPLY TO THESE CHANGES ALSO.
 *
 * V3 boards use a 18800 chip and are single-banked.  Bank selection is done
 * with bits 1-4 of extended register 1CE, index B2.
 *
 * Boards V4 and V5 have the 18800-1 chip. Boards Plus and XL have the 28800
 * chip. Page selection is done with Extended Register 1CE, Index B2.
 * The format is:
 *
 * D7-D5 = Read page select bits 2-0
 * D4    = Reserved (18800-1)
 * D4    = Page select bit 3 (28800)
 * D3-D1 = Page select bits 2-0
 * D0    = Reserved (18800-1)
 * D0    = Read page select bit 3 (28800)
 *
 * Also, for those boards with more than 1M of video memory (such as some
 * Mach32's and Mach64's), additional page select bits are defined in Extended
 * Register 1CE, Index AE, as follows:
 *
 * D7-D4 = Reserved
 * D3-D2 = Read page select bits 5-4
 * D1-D0 = Page select bits 5-4
 */

#include "assyntax.h"

	FILE("atibank.s")
	AS_BEGIN

/**
 ** Please read the notes in driver.c !!!
 **/

	SEG_DATA

/*
 * We have a mirror for the segment register because an I/O read costs so much
 * more time, that is better to keep the value of it in memory.  However, this
 * won't do for a V3 board because there are other bits in the segment select
 * register to worry about.  Also, the driver needs to reset this mirror during
 * mode save and restore functions.
 */
	GLOBL	GLNAME(ATIB2Reg)
GLNAME(ATIB2Reg):
Segment:
	D_BYTE 0

/*
 * The functions ...
 */

	SEG_TEXT

/*
 * Start with the functions used with 28800+ chips.  This includes all the
 * Mach's.
 */

	ALIGNTEXT4
	GLOBL	GLNAME(ATISetRead)
GLNAME(ATISetRead):
	SHL_L	(CONST(12),EAX)
	SHR_W	(CONST(12),AX)
	MOV_B	(CONTENT(Segment),AH)
	AND_B	(CONST(0x1E),AH)
	ROR_B	(CONST(3),AL)
	OR_B	(AL,AH)
	MOV_B	(AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	SHR_L	(CONST(6),EAX)
	AND_B	(CONST(0x0C),AH)
	MOV_B	(CONST(0xAE),AL)
	OUT_B
	INC_W	(DX)
	IN_B
	AND_B	(CONST(0xF3),AL)
	OR_B	(AL,AH)
	DEC_W	(DX)
	MOV_B	(CONST(0xAE),AL)
	OUT_W
	RET

	ALIGNTEXT4
	GLOBL	GLNAME(ATISetWrite)
GLNAME(ATISetWrite):
	SHL_L	(CONST(12),EAX)
	SHR_W	(CONST(12),AX)
	MOV_B	(CONTENT(Segment),AH)
	AND_B	(CONST(0xE1),AH)
	SHL_B	(CONST(1),AL)
	OR_B	(AL,AH)
	MOV_B	(AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	SHR_L	(CONST(8),EAX)
	AND_B	(CONST(0x03),AH)
	MOV_B	(CONST(0xAE),AL)
	OUT_B
	INC_W	(DX)
	IN_B
	AND_B	(CONST(0xFC),AL)
	OR_B	(AL,AH)
	DEC_W	(DX)
	MOV_B	(CONST(0xAE),AL)
	OUT_W
	RET

	ALIGNTEXT4
	GLOBL	GLNAME(ATISetReadWrite)
GLNAME(ATISetReadWrite):
	SHL_L	(CONST(12),EAX)
	SHR_W	(CONST(12),AX)
	MOV_B	(AL,AH)
	SHL_B	(CONST(1),AH)
	ROR_B	(CONST(3),AL)
	OR_B	(AL,AH)
	MOV_B   (AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	SHR_L	(CONST(8),EAX)
	AND_B	(CONST(0x03),AH)
	MOV_B	(AH,AL)
	SHL_B	(CONST(2),AL)
	OR_B	(AL,AH)
	MOV_B	(CONST(0xAE),AL)
	OUT_B
	INC_W	(DX)
	IN_B
	AND_B	(CONST(0xF0),AL)
	OR_B	(AL,AH)
	DEC_W	(DX)
	MOV_B	(CONST(0xAE),AL)
	OUT_W
	RET

/*
 * The functions used for 18800-1 chips.
 */

	ALIGNTEXT4
	GLOBL	GLNAME(ATIV4V5SetRead)
GLNAME(ATIV4V5SetRead):
	AND_L	(CONST(0x0F),EAX)
	MOV_B	(CONTENT(Segment),AH)
	AND_B	(CONST(0x1E),AH)
	ROR_B	(CONST(3),AL)
	OR_B	(AL,AH)
	MOV_B	(AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	RET

	ALIGNTEXT4
	GLOBL	GLNAME(ATIV4V5SetWrite)
GLNAME(ATIV4V5SetWrite):
	AND_L	(CONST(0x0F),EAX)
	MOV_B	(CONTENT(Segment),AH)
	AND_B	(CONST(0xE1),AH)
	SHL_B	(CONST(1),AL)
	OR_B	(AL,AH)
	MOV_B	(AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	RET

	ALIGNTEXT4
	GLOBL	GLNAME(ATIV4V5SetReadWrite)
GLNAME(ATIV4V5SetReadWrite):
	AND_L	(CONST(0x0F),EAX)
	MOV_B	(AL,AH)
	SHL_B	(CONST(1),AH)
	ROR_B	(CONST(3),AL)
	OR_B	(AL,AH)
	MOV_B   (AH,CONTENT(Segment))
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	RET

/*
 * The function(s) used for 18800 chips.
 */

	ALIGNTEXT4
	GLOBL	GLNAME(ATIV3SetRead)
	GLOBL	GLNAME(ATIV3SetWrite)
	GLOBL	GLNAME(ATIV3SetReadWrite)
GLNAME(ATIV3SetRead):
GLNAME(ATIV3SetWrite):
GLNAME(ATIV3SetReadWrite):
	AND_L	(CONST(0x0F),EAX)
	SHL_B	(CONST(1),AL)
	MOV_B	(AL,AH)
	MOV_B	(CONST(0xB2),AL)
	MOV_W	(CONTENT(GLNAME(ATIExtReg)),DX)
	OUT_B
	INC_W	(DX)
	IN_B
	AND_B	(CONST(0xE1),AL)
	OR_B	(AL,AH)
	DEC_W	(DX)
	MOV_B	(CONST(0xB2),AL)
	OUT_W
	RET
