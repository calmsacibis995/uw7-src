/*
 * @(#) ti3025.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

/* 
 * ti3025.h  - Include file for Texas Instruments ViewPoint 3025 DAC
 *
 * Copyright 1994-1995 Number Nine Visual Technology Corporation.
 * All rights reserved.
 *
 */

#define	VPT_MuxCtrl1	0x18
#define	VPT_MuxCtrl2	0x19
#define	VPT_InputClk	0x1A
#define	VPT_OutputClk	0x1B
#define	VPT_GenCtrl	0x1D
#define	VPT_MiscCtrl	0x1E
#define	VPT_AuxCtrl	0x29
#define	VPT_PLLCtrl	0x2C
#define	VPT_DclkPLL	0x2D	/* Dot clock */
#define	VPT_MclkPLL	0x2E	/* Memory clock */
#define	VPT_LclkPLL	0x2F	/* Loop clock */
#define	VPT_KeyCtrl	0x38
#define	VPT_Reset	0xFF



#define	VPT_M1_VGA	0x80
#define	VPT_M1_8	0x80
#define	VPT_M1_32	0x0E
#define	VPT_M1_16_565	0x0D
#define	VPT_M1_16_555	0x0C
#define	VPT_M1_GAMMA	0x40	/* OR in this value */

#define	VPT_M2_VGA	0x98
#define	VPT_M2_8	0x1C
#define	VPT_M2_32	0x1C
#define	VPT_M2_16_565	0x04
#define	VPT_M2_16_555	0x04
#define	VPT_M2_GAMMA_MASK	0x07	/* AND with this value */

#define	VPT_ICLK_VGA	0x00
#define	VPT_ICLK_TTL	0x01
#define	VPT_ICLK_INTERN	0x05
#define	VPT_ICLK_DOUBLE	0x10	/* OR in this value */

#define	VPT_OCLK_VGA	0x3E
#define	VPT_OCLK_8	0x03
#define	VPT_OCLK_16	0x02
#define	VPT_OCLK_32	0x01
#define	VPT_OCLK_V1	0x40
#define	VPT_OCLK_V2	0x48
#define	VPT_OCLK_V4	0x50
#define	VPT_OCLK_V8	0x58
#define	VPT_OCLK_V16	0x60
#define	VPT_OCLK_V32	0x68

#define	VPT_GEN_SPLIT_SHIFT	0x04
#define	VPT_GEN_BORDER	0x40
#define	VPT_GEN_SOG	0x20
#define	VPT_GEN_HPOS	0x01
#define	VPT_GEN_VPOS	0x02

#define	VPT_AUX_HZOOM(x)	((x) << 5)
#define	VPT_AUX_SCLK	0x08
#define	VPT_AUX_PSEL	0x04
#define	VPT_AUX_WINFUNC	0x02
#define	VPT_AUX_WINSEL	0x01

#define	VPT_MISC_LOOP_ENABLE	0x80

#define VPT_KEY_SELECT_OVERLAY	0x10
#define VPT_KEY_SELECT_DIRECT	0x00

/* Clock defines */
#define	VPT_MCLOCK_65MHz	0x000F0B01
