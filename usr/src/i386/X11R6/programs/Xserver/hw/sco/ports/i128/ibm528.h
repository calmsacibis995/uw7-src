
/*
 * @(#) ibm528.h 11.1 97/10/22
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
 * ibm528.h  - Include file for IBM 528 DAC
 *
 * Copyright 1994-1995 Number Nine Visual Technology Corporation.
 * All rights reserved.
 *
 */

#define	IBM528_REF_CLOCK	25175000   /* We're using a 25.175MHz crystal */

/* Indirect registers */
#define	IBM528_REVISION		0x00
#define	IBM528_ID		0x01
#define	IBM528_MISC_CLK_CTRL	0x02
#define	IBM528_SYNC_CTRL	0x03
#define	IBM528_HSYNC_POS	0x04
#define	IBM528_POWER		0x05
#define	IBM528_DAC_OP		0x06
#define	IBM528_PALETTE_CTRL	0x07
#define	IBM528_SYSCLK_CTRL	0x08
#define	IBM528_PIXEL_FORMAT	0x0A
#define	IBM528_8BPP_CTRL	0x0B
#define	IBM528_16BPP_CTRL	0x0C
#define	IBM528_24BPP_CTRL	0x0D
#define	IBM528_32BPP_CTRL	0x0E
#define	IBM528_BUFFER_A_B_SEL	0x0F
#define	IBM528_PIX_PLL_CTRL1	0x10
#define	IBM528_PIX_PLL_CTRL2	0x11
#define	IBM528_FIX_PLL_REF_DIV	0x14
#define	IBM528_SYS_PLL_REF_DIV	0x15
#define	IBM528_SYS_PLL_VCO_DIV	0x16
#define	IBM528_PLL_REGS		0x20
#define	IBM528_CURSOR_CTRL	0x30
#define	IBM528_CURSOR_X_LOW	0x31
#define	IBM528_CURSOR_X_HIGH	0x32
#define	IBM528_CURSOR_Y_LOW	0x33
#define	IBM528_CURSOR_Y_HIGH	0x34
#define	IBM528_CURSOR_X_HOT	0x35
#define	IBM528_CURSOR_Y_HOT	0x36
#define	IBM528_CURS_1_RED	0x40
#define	IBM528_CURS_1_GREEN	0x41
#define	IBM528_CURS_1_BLUE	0x42
#define	IBM528_CURS_2_RED	0x43
#define	IBM528_CURS_2_GREEN	0x44
#define	IBM528_CURS_2_BLUE	0x45
#define	IBM528_CURS_3_RED	0x46
#define	IBM528_CURS_3_GREEN	0x47
#define	IBM528_CURS_3_BLUE	0x48
#define	IBM528_BORDER_RED	0x60
#define	IBM528_BORDER_GREEN	0x61
#define	IBM528_BORDER_BLUE	0x62
#define	IBM528_MISC_CTRL_1	0x70
#define	IBM528_MISC_CTRL_2	0x71
#define	IBM528_MISC_CTRL_3	0x72
#define	IBM528_MISC_CTRL_4	0x73

#define	IBM528_VRAM_MASK_0	0x90
#define	IBM528_VRAM_MASK_1	0x91
#define	IBM528_VRAM_MASK_2	0x92
#define	IBM528_VRAM_MASK_3	0x93

#define	IBM528_CURSOR_ARRAY	0x100

/* Clock defines */
#define	IBM528_MCLOCK_65MHz	0x000F0B01
