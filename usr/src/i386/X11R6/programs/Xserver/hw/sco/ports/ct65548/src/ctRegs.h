/*
 *	@(#)ctRegs.h	11.1	10/22/97	12:34:11
 *	@(#) ctRegs.h 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_REGS_H
#define _CT_REGS_H

#ident "@(#) $Id: ctRegs.h 58.1 96/10/09 "

/* VGA Color Palette Registers */
#define CT_DACMASK	0x3c6	/* Color palette pixel mask */
#define CT_DACRX	0x3c7	/* Color palette read-mode index */
#define CT_DACX		0x3c8	/* Color palette index */
#define CT_DACDATA	0x3c9	/* Color palette data */

/* Sequencer Registers (SRxx) */
#define CT_SRX		0x3c4	/* SRX  Sequencer index */
#define CT_SR		0x3c5	/* SR00 Sequencer Register */

/* CRT Controller Registers (CRxx) */
#define CT_CRX		0x3b4	/* CRX  CRTC index */
#define CT_CR		0x3b5	/* CR00 CRTC Register */

/* Graphics Controller Registers (GRxx) */
#define CT_GRX		0x3ce	/* GRX  Graphics Controller index */
#define CT_GR		0x3cf	/* GR00 Graphics Controller Register */

/* Graphics Controller Indexes (GRxx) */
#define CT_GRRESET	0x00	/* GR00 Set/Reset Register */
#define CT_GRENABRESET	0x01	/* GR01 Enable Set/Reset Register */
#define CT_GRCOLORCMP	0x02	/* GR02 Color Compare Register */
#define CT_GRROTATE	0x03	/* GR03 Data Rotate Register */
#define CT_GRREADMAP	0x04	/* GR04 Read Map Select Register */
#define CT_GRMODE	0x05	/* GR05 Graphics Mode Register */
#define CT_GRMISC	0x06	/* GR06 Miscellaneous Register */
#define CT_GRNOCOLORCMP	0x07	/* GR07 Color Don't Care Register */
#define CT_GRBITMASK	0x08	/* GR08 Bit Mask Register */

/* Extension Registers (XRxx) */
#define CT_XRX		0x3d6	/* XRX  Extension index */
#define CT_XR		0x3d7	/* XR00 Extension Register */

/* 32-Bit Registers (DRxx) */
#define	CT_BLTOFFSET	0x83d0	/* DR00 BitBlt offset (stride) */
#define	CT_BLTSTIP	0x87d0	/* DR01 BitBlt pattern (8x8 stipple) */
#define	CT_BLTBG	0x8bd0	/* DR02 BitBlt background color */
#define	CT_BLTFG	0x8fd0	/* DR03 BitBlt foreground color */
#define	CT_BLTCTRL	0x93d0	/* DR04 BitBlt control */
#define	CT_BLTSRC	0x97d0	/* DR05 BitBlt source address */
#define	CT_BLTDST	0x9bd0	/* DR06 BitBlt destination address */
#define	CT_BLTCMD	0x9fd0	/* DR07 BitBlt command */
#define	CT_CURIDX	0xa3d0	/* DR08 Cursor read/write index */
#define	CT_CURCOLOR0	0xa7d0	/* DR09 Cursor color 0 */
#define	CT_CURCOLOR1	0xabd0	/* DR0a Cursor color 1 */
#define	CT_CURPOS	0xafd0	/* DR0b Cursor position */
#define	CT_CURDATA	0xb3d0	/* DR0b Cursor data */

/*******************************************************************************

		Bit masks for BitBlt control register (DR04)

*******************************************************************************/

/* Bits 0-7 select an MS Windows rop defined in ctRops.h */
#define CT_YINCBIT	0x00000100L	/* 8 0-up, 1-down */
#define CT_XINCBIT	0x00000200L	/* 9 0-left, 1-right */
#define CT_FGCOLORBIT	0x00000400L	/* 10 0-data, 1-fg_color */
#define CT_SRCMONOBIT	0x00000800L	/* 11 0-color, 1-mono */
#define CT_PATMONOBIT	0x00001000L	/* 12 0-color, 1-mono */
#define CT_BGTRANSBIT	0x00002000L	/* 13 0-opaque, 1-transparent */
#define CT_BLTMEMORYBIT	0x00004000L	/* 14 0-screen, 1-mem */
/* Bit 15 RESERVED */
#define CT_PATSEEDBIT1	0x00010000L	/* 16 */
#define CT_PATSEEDBIT2	0x00020000L	/* 17 */
#define CT_PATSEEDBIT3	0x00040000L	/* 18 */
#define CT_PATSOLIDBIT	0x00080000L	/* 19 0-bitmap, 1-solid */
#define CT_ACTIVEBIT	0x00100000L	/* 20 0-idle, 1-active */
/* Bits 21-23 RESERVED */
#define CT_BUFFERBIT	0x01000000L	/* 24 0-full, 1-available */
/* Bits 25-31 RESERVED */

#endif /* _CT_REGS_H */
