/*
 *	@(#)ctRegs.h	11.1	10/22/97	12:35:03
 *	@(#) ctRegs.h 59.1 96/11/04 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_REGS_H
#define _CT_REGS_H

#ident "@(#) $Id: ctRegs.h 59.1 96/11/04 "

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
#if 0
#define CT_XR		0x3d6	/* XR00 Extension Register */
#endif

/* 32-Bit Registers (BRxx) */
#define	CT_BLTOFFSET	0x400000	/* BR00 BitBlt offset (stride) */
#define	CT_BLTBG	0x400004	/* BR01 BitBlt background color */
#define	CT_BLTFG	0x400008	/* BR02 BitBlt foreground color */
#define	CT_BLTMONOCTL	0x40000C	/* BR03 BitBlt monochrome src cntrl */
#define	CT_BLTCTRL	0x400010	/* BR04 BitBlt control */
#define	CT_BLTSTIP	0x400014	/* BR05 BitBlt pattern (8x8 stipple) */
#define	CT_BLTSRC	0x400018	/* BR06 BitBlt source address */
#define	CT_BLTDST	0x40001C	/* BR07 BitBlt destination address */
#define	CT_BLTCMD	0x400020	/* BR08 BitBlt command */
#define CT_BLTSRCBG	0x400024	/* BR09 BitBlt source background color*/
#define CT_BLTSRCFG	0x400028	/* BR0A BitBlt source foreground color*/

#define CT_BLTDATA	0x410000

#define CT_CUR_CONTROL	0xa0
#define CT_CUR_BASELO	0xa2
#define CT_CUR_BASEHI	0xa3
#define CT_CUR_XLO	0xa4
#define CT_CUR_XHI	0xa5
#define CT_CUR_YLO	0xa6
#define CT_CUR_YHI	0xa7

/*******************************************************************************

		Bit masks for BitBlt control register (DR04)

*******************************************************************************/

/* Bits 0-7 select an MS Windows rop defined in ctRops.h */
#define CT_XINCBIT	0x00000100L	/* 8 0-left, 1-right */
#define CT_YINCBIT	0x00000200L	/* 9 0-top, 1-bottom */
#define CT_BLTMEMORYBIT	0x00000400L	/* 10 0-screen, 1-mem */
/* Bit 11 RESERVED	0x00000800L */
#define CT_SRCMONOBIT	0x00001000L	/* 12 0-color, 1-mono */
#define CT_BGTRANSBIT	0x00002000L	/* 13 0-opaque, 1-transparent */
#define CT_CTRANSMODE1	0x00004000L	/* 14 */
#define CT_CTRANSMODE2	0x00008000L	/* 15 */
#define CT_CTRANSMODE3	0x00010000L	/* 16 */
#define CT_MTRANSMODE	0x00020000L	/* 17 0-opaque, 1-transparent */
#define CT_PATMONOBIT	0x00040000L	/* 18 0-color, 1-mono */
#define CT_PATSOLIDBIT	0x00080000L	/* 19 0-bitmap, 1-solid */
#define CT_PATSEEDBIT1	0x00100000L	/* 20 */
#define CT_PATSEEDBIT2	0x00200000L	/* 21 */
#define CT_PATSEEDBIT3	0x00400000L	/* 22 */
/* Bits 23-30 RESERVED */
#define CT_ACTIVEBIT	0x80000000L	/* 31 0-idle, 1-active */

#if 0
/* This bit was deleted from the 65548 */
/* part of bit 19 */ define CT_FGCOLORBIT	0x00000400L	/* 10 0-data, 1-fg_color */
#endif
#endif /* _CT_REGS_H */
