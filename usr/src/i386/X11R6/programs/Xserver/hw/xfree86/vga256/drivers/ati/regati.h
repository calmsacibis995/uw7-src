/* $XConsortium: regati.h /main/4 1995/09/04 19:41:42 kaleb $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ati/regati.h,v 3.5 1995/05/27 03:15:22 dawes Exp $ */
/*
 * Copyright 1994 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Marc Aurele La France not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Marc Aurele La France makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Rationale:  Scanning reg8514.h, regmach8.h and regmach32.h is just not worth
 *             the eye strain...
 *
 * Acknowledgements:
 *    Jake Richter, Panacea Inc., Londonderry, New Hampshire, U.S.A.
 *    Kevin E. Martin, martin@cs.unc.edu
 *    Tiago Gons, tiago@comosjn.hobby.nl
 *    Rickard E. Faith, faith@cs.unc.edu
 *    Scott Laird, lair@kimbark.uchicago.edu
 *
 * The intent here is to list all I/O ports for VGA (and its predecessors),
 * ATI VGA Wonder, 8514/A, ATI Mach8, ATI Mach32 and ATI Mach64 video adapters,
 * not just the ones in use by the VGA Wonder driver.
 */

#ifndef _REGATI_H_
#define _REGATI_H_

/* MDA/CGA/EGA/VGA I/O ports */
#define GENVS			0x0102		/* Write (and Read on uC only) */

#define R_GENLPS		0x03b9		/* Read */

#define GENHP			0x03bf

#define ATTRX			0x03c0
#define ATTRD			0x03c1
#define GENS0			0x03c2		/* Read */
#define GENMO			0x03c2		/* Write */
#define GENENB			0x03c3		/* Read */
#define SEQX			0x03c4
#define SEQD			0x03c5
#define VGA_DAC_MASK		0x03c6
#define VGA_DAC_R_I		0x03c7
#define VGA_DAC_W_I		0x03c8
#define VGA_DAC_DATA		0x03c9
#define R_GENFC			0x03ca		/* Read */
/*	?			0x03cb */
#define R_GENMO			0x03cc		/* Read */
/*	?			0x03cd */
#define GRAX			0x03ce
#define GRAD			0x03cf

#define GENB			0x03d9

#define GENLPS			0x03dc		/* Write */
#define KCX			0x03dd
#define KCD			0x03de

#define GENENA			0x46e8		/* Write */

/* I/O port base numbers */
#define MonochromeIOBase	0x03b0
#define ColourIOBase		0x03d0

/* Other EGA/CGA/VGA I/O ports */
/*	?(IOBase)		(IOBase + 0x00) */
/*	?(IOBase)		(IOBase + 0x01) */
/*	?(IOBase)		(IOBase + 0x02) */
/*	?(IOBase)		(IOBase + 0x03) */
#define CRTX(IOBase)		(IOBase + 0x04)
#define CRTD(IOBase)		(IOBase + 0x05)
/*	?(IOBase)		(IOBase + 0x06) */
/*	?(IOBase)		(IOBase + 0x07) */
#define GENMC(IOBase)		(IOBase + 0x08)
/*	?(IOBase)		(IOBase + 0x09) */
#define GENS1(IOBase)		(IOBase + 0x0a)	/* Read */
#define GENFC(IOBase)		(IOBase + 0x0a)	/* Write */
#define GENLPC(IOBase)		(IOBase + 0x0b)
/*	?(IOBase)		(IOBase + 0x0c) */
/*	?(IOBase)		(IOBase + 0x0d) */
/*	?(IOBase)		(IOBase + 0x0e) */
/*	?(IOBase)		(IOBase + 0x0f) */

/* 8514/A VESA approved register definitions */
#define DISP_STAT		0x02e8		/* Read */
#define SENSE				0x0001	/* Presumably belong here */
#define VBLANK				0x0002
#define HORTOG				0x0004
#define H_TOTAL			0x02e8		/* Write */
#define DAC_MASK		0x02ea
#define DAC_R_INDEX		0x02eb
#define DAC_W_INDEX		0x02ec
#define DAC_DATA		0x02ed
#define H_DISP			0x06e8		/* Write */
#define H_SYNC_STRT		0x0ae8		/* Write */
#define H_SYNC_WID		0x0ee8		/* Write */
#define HSYNCPOL_POS			0x0000
#define HSYNCPOL_NEG			0x0020
#define H_POLARITY_POS			HSYNCPOL_POS	/* Sigh */
#define H_POLARITY_NEG			HSYNCPOL_NEG	/* Sigh */
#define V_TOTAL			0x12e8		/* Write */
#define V_DISP			0x16e8		/* Write */
#define V_SYNC_STRT		0x1ae8		/* Write */
#define V_SYNC_WID		0x1ee8		/* Write */
#define VSYNCPOL_POS			0x0000
#define VSYNCPOL_NEG			0x0020
#define V_POLARITY_POS			VSYNCPOL_POS	/* Sigh */
#define V_POLARITY_NEG			VSYNCPOL_NEG	/* Sigh */
#define DISP_CNTL		0x22e8		/* Write */
#define ODDBNKENAB			0x0001
#define MEMCFG_2			0x0000
#define MEMCFG_4			0x0002
#define MEMCFG_6			0x0004
#define MEMCFG_8			0x0006
#define DBLSCAN				0x0008
#define INTERLACE			0x0010
#define DISPEN_NC			0x0000
#define DISPEN_ENAB			0x0020
#define DISPEN_DISAB			0x0040
#define R_H_TOTAL		0x26e8		/* Read */
/*	?			0x2ae8 */
/*	?			0x2ee8 */
/*	?			0x32e8 */
/*	?			0x36e8 */
/*	?			0x3ae8 */
/*	?			0x3ee8 */
#define SUBSYS_STAT		0x42e8		/* Read */
#define VBLNKFLG			0x0001
#define PICKFLAG			0x0002
#define INVALIDIO			0x0004
#define GPIDLE				0x0008
#define MONITORID_MASK			0x0070
/*	MONITORID_?				0x0000 */
#define MONITORID_8507				0x0010
#define MONITORID_8514				0x0020
/*	MONITORID_?				0x0030 */
/*	MONITORID_?				0x0040 */
#define MONITORID_8503				0x0050
#define MONITORID_8512				0x0060
#define MONITORID_8513				0x0060
#define MONITORID_NONE				0x0070
#define _8PLANE				0x0080
#define SUBSYS_CNTL		0x42e8		/* Write */
#define RVBLNKFLG			0x0001
#define RPICKFLAG			0x0002
#define RINVALIDIO			0x0004
#define RGPIDLE				0x0008
#define IVBLNKFLG			0x0100
#define IPICKFLAG			0x0200
#define IINVALIDIO			0x0400
#define IGPIDLE				0x0800
#define CHPTEST_NC			0x0000
#define CHPTEST_NORMAL			0x1000
#define CHPTEST_ENAB			0x2000
#define GPCTRL_NC			0x0000
#define GPCTRL_ENAB			0x4000
#define GPCTRL_RESET			0x8000
#define ROM_PAGE_SEL		0x46e8		/* Write */
#define ADVFUNC_CNTL		0x4ae8		/* Write */
#define DISABPASSTHRU			0x0001
#define CLOKSEL				0x0004
/*	?			0x4ee8 */
/*	?			0x52e8 */
/*	?			0x56e8 */
/*	?			0x5ae8 */
/*	?			0x5ee8 */
/*	?			0x62e8 */
/*	?			0x66e8 */
/*	?			0x6ae8 */
/*	?			0x6ee8 */
/*	?			0x72e8 */
/*	?			0x76e8 */
/*	?			0x7ae8 */
/*	?			0x7ee8 */
#define CUR_Y			0x82e8
#define CUR_X			0x86e8
#define DESTY_AXSTP		0x8ae8		/* Write */
#define DESTX_DIASTP		0x8ee8		/* Write */
#define ERR_TERM		0x92e8
#define MAJ_AXIS_PCNT		0x96e8		/* Write */
#define GP_STAT			0x9ae8		/* Read */
#define GE_STAT			0x9ae8		/* Alias */
#define DATARDY				0x0100
#define DATA_READY			DATARDY	/* Alias */
#define GPBUSY				0x0200
#define CMD			0x9ae8		/* Write */
#define WRTDATA				0x0001
#define PLANAR				0x0002
#define LASTPIX				0x0004
#define LINETYPE			0x0008
#define DRAW				0x0010
#define INC_X				0x0020
#define YMAJAXIS			0x0040
#define INC_Y				0x0080
#define PCDATA				0x0100
#define _16BIT				0x0200
#define CMD_NOP				0x0000
#define CMD_OP_MSK			0xf000
#define BYTSEQ					0x1000
#define CMD_LINE				0x2000
#define CMD_RECT				0x4000
#define CMD_RECTV1				0x6000
#define CMD_RECTV2				0x8000
#define CMD_LINEAF				0xa000
#define CMD_BITBLT				0xc000
#define SHORT_STROKE		0x9ee8		/* Write */
#define SSVDRAW				0x0010
#define VECDIR_000			0x0000
#define VECDIR_045			0x0020
#define VECDIR_090			0x0040
#define VECDIR_135			0x0060
#define VECDIR_180			0x0080
#define VECDIR_225			0x00a0
#define VECDIR_270			0x00c0
#define VECDIR_315			0x00e0
#define BKGD_COLOR		0xa2e8			/* Write */
#define FRGD_COLOR		0xa6e8			/* Write */
#define WRT_MASK		0xaae8			/* Write */
#define RD_MASK			0xaee8			/* Write */
#define COLOR_CMP		0xb2e8			/* Write */
#define BKGD_MIX		0xb6e8			/* Write */
/*					0x001f	See MIX_* definitions below */
#define BSS_BKGDCOL			0x0000
#define BSS_FRGDCOL			0x0020
#define BSS_PCDATA			0x0040
#define BSS_BITBLT			0x0060
#define FRGD_MIX		0xbae8			/* Write */
/*					0x001f	See MIX_* definitions below */
#define FSS_BKGDCOL			0x0000
#define FSS_FRGDCOL			0x0020
#define FSS_PCDATA			0x0040
#define FSS_BITBLT			0x0060
#define MULTIFUNC_CNTL		0xbee8		/* Write */
#define MIN_AXIS_PCNT			0x0000
#define SCISSORS_T			0x1000
#define SCISSORS_L			0x2000
#define SCISSORS_B			0x3000
#define SCISSORS_R			0x4000
#define MEM_CNTL			0x5000
#define HORCFG_4				0x0000
#define HORCFG_5				0x0001
#define HORCFG_8				0x0002
#define HORCFG_10				0x0003
#define VRTCFG_2				0x0000
#define VRTCFG_4				0x0004
#define VRTCFG_6				0x0008
#define VRTCFG_8				0x000c
#define BUFSWP					0x0010
#define PATTERN_L			0x8000
#define PATTERN_H			0x9000
#define PIX_CNTL			0xa000
#define PLANEMODE				0x0004
#define COLCMPOP_F				0x0000
#define COLCMPOP_T				0x0008
#define COLCMPOP_GE				0x0010
#define COLCMPOP_LT				0x0018
#define COLCMPOP_NE				0x0020
#define COLCMPOP_EQ				0x0028
#define COLCMPOP_LE				0x0030
#define COLCMPOP_GT				0x0038
#define	MIXSEL_FRGDMIX				0x0000
#define	MIXSEL_PATT				0x0040
#define	MIXSEL_EXPPC				0x0080
#define	MIXSEL_EXPBLT				0x00c0
/*	?			0xc2e8 */
/*	?			0xc6e8 */
/*	?			0xcae8 */
/*	?			0xcee8 */
/*	?			0xd2e8 */
/*	?			0xd6e8 */
/*	?			0xdae8 */
/*	?			0xdee8 */
#define PIX_TRANS		0xe2e8
/*	?			0xe6e8 */
/*	?			0xeae8 */
/*	?			0xeee8 */
/*	?			0xf2e8 */
/*	?			0xf6e8 */
/*	?			0xfae8 */
/*	?			0xfee8 */

/* ATI Mach8 & Mach32 register definitions */
#define OVERSCAN_COLOR_8	0x02ee		/* Write */	/* Mach32 */
#define OVERSCAN_BLUE_24	0x02ef		/* Write */	/* Mach32 */
#define OVERSCAN_GREEN_24	0x06ee		/* Write */	/* Mach32 */
#define OVERSCAN_RED_24		0x06ef		/* Write */	/* Mach32 */
#define CURSOR_OFFSET_LO	0x0aee		/* Write */	/* Mach32 */
#define CURSOR_OFFSET_HI	0x0eee		/* Write */	/* Mach32 */
#define CONFIG_STATUS_1		0x12ee		/* Read */
#define CLK_MODE			0x0001			/* Mach8 */
#define BUS_16				0x0002			/* Mach8 */
#define MC_BUS				0x0004			/* Mach8 */
#define EEPROM_ENA			0x0008			/* Mach8 */
#define DRAM_ENA			0x0010			/* Mach8 */
#define MEM_INSTALLED			0x0060			/* Mach8 */
#define ROM_ENA				0x0080			/* Mach8 */
#define ROM_PAGE_ENA			0x0100			/* Mach8 */
#define ROM_LOCATION			0xfe00			/* Mach8 */
#define _8514_ONLY			0x0001			/* Mach32 */
#define BUS_TYPE			0x000e			/* Mach32 */
#define ISA_16_BIT				0x0000		/* Mach32 */
#define EISA					0x0002		/* Mach32 */
#define MICRO_C_16_BIT				0x0004		/* Mach32 */
#define MICRO_C_8_BIT				0x0006		/* Mach32 */
#define LOCAL_386SX				0x0008		/* Mach32 */
#define LOCAL_386DX				0x000a		/* Mach32 */
#define LOCAL_486				0x000c		/* Mach32 */
#define PCI					0x000e		/* Mach32 */
#define MEM_TYPE			0x0070			/* Mach32 */
#define CHIP_DIS			0x0080			/* Mach32 */
#define TST_VCTR_ENA			0x0100			/* Mach32 */
#define DACTYPE				0x0e00			/* Mach32 */
#define MC_ADR_DECODE			0x1000			/* Mach32 */
#define CARD_ID				0xe000			/* Mach32 */
#define HORZ_CURSOR_POSN	0x12ee		/* Write */	/* Mach32 */
#define CONFIG_STATUS_2		0x16ee		/* Read */
#define SHARE_CLOCK			0x0001			/* Mach8 */
#define HIRES_BOOT			0x0002			/* Mach8 */
#define EPROM_16_ENA			0x0004			/* Mach8 */
#define WRITE_PER_BIT			0x0008			/* Mach8 */
#define FLASH_ENA			0x0010			/* Mach8 */
#define SLOW_SEQ_EN			0x0001			/* Mach32 */
#define MEM_ADDR_DIS			0x0002			/* Mach32 */
#define ISA_16_ENA			0x0004			/* Mach32 */
#define KOR_TXT_MODE_ENA		0x0008			/* Mach32 */
#define LOCAL_BUS_SUPPORT		0x0030			/* Mach32 */
#define LOCAL_BUS_CONFIG_2		0x0040			/* Mach32 */
#define LOCAL_BUS_RD_DLY_ENA		0x0080			/* Mach32 */
#define LOCAL_DAC_EN			0x0100			/* Mach32 */
#define LOCAL_RDY_EN			0x0200			/* Mach32 */
#define EEPROM_ADR_SEL			0x0400			/* Mach32 */
#define GE_STRAP_SEL			0x0800			/* Mach32 */
#define VESA_RDY			0x1000			/* Mach32 */
#define Z4GB				0x2000			/* Mach32 */
#define LOC2_MDRAM			0x4000			/* Mach32 */
#define VERT_CURSOR_POSN	0x16ee		/* Write */	/* Mach32 */
#define FIFO_TEST_DATA		0x1aee		/* Read */	/* Mach32 */
#define CURSOR_COLOR_0		0x1aee		/* Write */	/* Mach32 */
#define CURSOR_COLOR_1		0x1aef		/* Write */	/* Mach32 */
#define HORZ_CURSOR_OFFSET	0x1eee		/* Write */	/* Mach32 */
#define VERT_CURSOR_OFFSET	0x1eef		/* Write */	/* Mach32 */
#define PCI_CNTL		0x22ee				/* Mach32-PCI */
#define CRT_PITCH		0x26ee		/* Write */
#define CRT_OFFSET_LO		0x2aee		/* Write */
#define CRT_OFFSET_HI		0x2eee		/* Write */
#define LOCAL_CNTL		0x32ee				/* Mach32 */
#define FIFO_OPT		0x36ee		/* Write */	/* Mach8 */
#define MISC_OPTIONS		0x36ee				/* Mach32 */
#define W_STATE_ENA			0x0000			/* Mach32 */
#define HOST_8_ENA			0x0001			/* Mach32 */
#define MEM_SIZE_ALIAS			0x000c			/* Mach32 */
#define MEM_SIZE_512K				0x0000		/* Mach32 */
#define MEM_SIZE_1M				0x0004		/* Mach32 */
#define MEM_SIZE_2M				0x0008		/* Mach32 */
#define MEM_SIZE_4M				0x000c		/* Mach32 */
#define DISABLE_VGA			0x0010			/* Mach32 */
#define _16_BIT_IO			0x0020			/* Mach32 */
#define DISABLE_DAC			0x0040			/* Mach32 */
#define DLY_LATCH_ENA			0x0080			/* Mach32 */
#define TEST_MODE			0x0100			/* Mach32 */
#define BLK_WR_ENA			0x0400			/* Mach32 */
#define _64_DRAW_ENA			0x0800			/* Mach32 */
#define FIFO_TEST_TAG		0x3aee		/* Read */	/* Mach32 */
#define EXT_CURSOR_COLOR_0	0x3aee		/* Write */	/* Mach32 */
#define EXT_CURSOR_COLOR_1	0x3eee		/* Write */	/* Mach32 */
#define MEM_BNDRY		0x42ee				/* Mach32 */
#define MEM_PAGE_BNDRY			0x000f			/* Mach32 */
#define MEM_BNDRY_ENA			0x0010			/* Mach32 */
#define SHADOW_CTL		0x46ee		/* Write */
#define CLOCK_SEL		0x4aee
/*	DISABPASSTHRU			0x0001	See ADVFUNC_CNTL */
#define VFIFO_DEPTH_1			0x0100			/* Mach32 */
#define VFIFO_DEPTH_2			0x0200			/* Mach32 */
#define VFIFO_DEPTH_3			0x0300			/* Mach32 */
#define VFIFO_DEPTH_4			0x0400			/* Mach32 */
#define VFIFO_DEPTH_5			0x0500			/* Mach32 */
#define VFIFO_DEPTH_6			0x0600			/* Mach32 */
#define VFIFO_DEPTH_7			0x0700			/* Mach32 */
#define VFIFO_DEPTH_8			0x0800			/* Mach32 */
#define VFIFO_DEPTH_9			0x0900			/* Mach32 */
#define VFIFO_DEPTH_A			0x0a00			/* Mach32 */
#define VFIFO_DEPTH_B			0x0b00			/* Mach32 */
#define VFIFO_DEPTH_C			0x0c00			/* Mach32 */
#define VFIFO_DEPTH_D			0x0d00			/* Mach32 */
#define VFIFO_DEPTH_E			0x0e00			/* Mach32 */
#define VFIFO_DEPTH_F			0x0f00			/* Mach32 */
#define COMPOSITE_SYNC			0x1000
/*	?			0x4eee */
#define ROM_ADDR_1		0x52ee
#define ROM_ADDR_2		0x56ee		/* Sick ... */
#define SHADOW_SET		0x5aee		/* Write */
#define MEM_CFG			0x5eee				/* Mach32 */
#define MEM_APERT_SEL			0x0003			/* Mach32 */
#define MEM_APERT_PAGE			0x000c			/* Mach32 */
#define MEM_APERT_LOC			0xfff0			/* Mach32 */
#define EXT_GE_STATUS		0x62ee		/* Read */	/* Mach32 */
#define HORZ_OVERSCAN		0x62ee		/* Write */	/* Mach32 */
#define VERT_OVERSCAN		0x66ee		/* Write */	/* Mach32 */
#define MAX_WAITSTATES		0x6aee
#define GE_OFFSET_LO		0x6eee		/* Write */
#define BOUNDS_LEFT		0x72ee		/* Read */
#define GE_OFFSET_HI		0x72ee		/* Write */
#define BOUNDS_TOP		0x76ee		/* Read */
#define GE_PITCH		0x76ee		/* Write */
#define BOUNDS_RIGHT		0x7aee		/* Read */
#define EXT_GE_CONFIG		0x7aee		/* Write */	/* Mach32 */
#define MONITOR_ALIAS			0x0007			/* Mach32 */
/*	MONITOR_?				0x0000 */	/* Mach32 */
#define MONITOR_8507				0x0001		/* Mach32 */
#define MONITOR_8514				0x0002		/* Mach32 */
/*	MONITOR_?				0x0003 */	/* Mach32 */
/*	MONITOR_?				0x0004 */	/* Mach32 */
#define MONITOR_8503				0x0005		/* Mach32 */
#define MONITOR_8512				0x0006		/* Mach32 */
#define MONITOR_8513				0x0006		/* Mach32 */
#define MONITOR_NONE				0x0007		/* Mach32 */
#define ALIAS_ENA			0x0008			/* Mach32 */
#define PIXEL_WIDTH_4			0x0000			/* Mach32 */
#define PIXEL_WIDTH_8			0x0010			/* Mach32 */
#define PIXEL_WIDTH_16			0x0020			/* Mach32 */
#define PIXEL_WIDTH_24			0x0030			/* Mach32 */
#define RGB16_555			0x0000			/* Mach32 */
#define RGB16_565			0x0040			/* Mach32 */
#define RGB16_655			0x0080			/* Mach32 */
#define RGB16_664			0x00c0			/* Mach32 */
#define MULTIPLEX_PIXELS		0x0100			/* Mach32 */
#define RGB24				0x0000			/* Mach32 */
#define RGBx24				0x0200			/* Mach32 */
#define BGR24				0x0400			/* Mach32 */
#define xBGR24				0x0600			/* Mach32 */
#define DAC_8_BIT_EN			0x4000			/* Mach32 */
#define PIX_WIDTH_16BPP			PIXEL_WIDTH_16		/* Mach32 */
#define ORDER_16BPP_565			RGB16_565		/* Mach32 */
#define BOUNDS_BOTTOM		0x7eee		/* Read */
#define MISC_CNTL		0x7eee		/* Write */	/* Mach32 */
#define PATT_DATA_INDEX		0x82ee
/*	?			0x86ee */
/*	?			0x8aee */
#define R_EXT_GE_CONFIG		0x8eee		/* Read */	/* Mach32 */
#define PATT_DATA		0x8eee		/* Write */
#define R_MISC_CNTL		0x92ee		/* Read */	/* Mach32 */
#define BRES_COUNT		0x96ee
#define EXT_FIFO_STATUS		0x9aee		/* Read */
#define LINEDRAW_INDEX		0x9aee		/* Write */
/*	?			0x9eee */
#define LINEDRAW_OPT		0xa2ee
#define BOUNDS_RESET			0x0100
#define CLIP_MODE_0			0x0000	/* Clip exception disabled */
#define CLIP_MODE_1			0x0200	/* Line segments */
#define CLIP_MODE_2			0x0400	/* Polygon boundary lines */
#define CLIP_MODE_3			0x0600	/* Patterned lines */
#define DEST_X_START		0xa6ee		/* Write */
#define DEST_X_END		0xaaee		/* Write */
#define DEST_Y_END		0xaeee		/* Write */
#define R_H_TOTAL_DISP		0xb2ee		/* Read */	/* Mach32 */
#define SRC_X_START		0xb2ee		/* Write */
#define R_H_SYNC_STRT		0xb6ee		/* Read */	/* Mach32 */
#define ALU_BG_FN		0xb6ee		/* Write */
#define R_H_SYNC_WID		0xbaee		/* Read */	/* Mach32 */
#define ALU_FG_FN		0xbaee		/* Write */
#define SRC_X_END		0xbeee		/* Write */
#define R_V_TOTAL		0xc2ee		/* Read */
#define SRC_Y_DIR		0xc2ee		/* Write */
#define R_V_DISP		0xc6ee		/* Read */	/* Mach32 */
#define EXT_SHORT_STROKE	0xc6ee		/* Write */
#define R_V_SYNC_STRT		0xcaee		/* Read */	/* Mach32 */
#define SCAN_X			0xcaee		/* Write */
#define VERT_LINE_CNTR		0xceee		/* Read */	/* Mach32 */
#define DP_CONFIG		0xceee		/* Write */
#define READ_WRITE			0x0001
#define DATA_WIDTH			0x0200
#define DATA_ORDER			0x1000
#define FG_COLOR_SRC_FG			0x2000
#define FG_COLOR_SRC_BLIT		0x6000
#define R_V_SYNC_WID		0xd2ee		/* Read */
#define PATT_LENGTH		0xd2ee		/* Write */
#define PATT_INDEX		0xd6ee		/* Write */
#define READ_SRC_X		0xdaee		/* Read */	/* Mach32 */
#define EXT_SCISSOR_L		0xdaee		/* Write */
#define READ_SRC_Y		0xdeee		/* Read */	/* Mach32 */
#define EXT_SCISSOR_T		0xdeee		/* Write */
#define EXT_SCISSOR_R		0xe2ee		/* Write */
#define EXT_SCISSOR_B		0xe6ee		/* Write */
/*	?			0xeaee */
#define DEST_COMP_FN		0xeeee		/* Write */
#define DEST_COLOR_CMP_MASK	0xf2ee		/* Write */	/* Mach32 */
/*	?			0xf6ee */
#define CHIP_ID			0xfaee		/* Read */	/* Mach32 */
#define CHIP_CODE_0			0x001F			/* Mach32 */
#define CHIP_CODE_1			0x03E0			/* Mach32 */
#define CHIP_CLASS			0x0C00			/* Mach32 */
#define CHIP_REV			0xF000			/* Mach32 */
#define LINEDRAW		0xfeee		/* Write */

/* ATI Mach64 register definitions */
#define CRTC_H_TOTAL_DISP	0x02ec
#define CRTC_H_SYNC_STRT_WID	0x06ec
#define CRTC_V_TOTAL_DISP	0x0aec
#define CRCT_V_SYNC_STRT_WID	0x0eec
#define CRTC_VLINE_CRNT_VLINE	0x12ec
#define CRTC_OFF_PITCH		0x16ec
#define CRTC_INT_CNTL		0x1aec
#define CRTC_GEN_CNTL		0x1eec
#define CRTC_DBL_SCAN_EN		0x00000001
#define CRTC_INTERLACE_EN		0x00000002
#define CRTC_HSYNC_DIS			0x00000004
#define CRTC_VSYNC_DIS			0x00000008
#define CRTC_CSYNC_EN			0x00000010
#define CRTC_PIX_BY_2_EN		0x00000020
/*	?				0x000000C0 */
#define CRTC_PIX_WIDTH			0x00000700
#define CRTC_BYTE_PIX_ORDER		0x00000800
/*	?				0x0000F000 */
#define CRTC_FIFO_LWM			0x000F0000
/*	?				0x00F00000 */
#define CRTC_EXT_DISP_EN		0x01000000
#define CRTC_EN				0x02000000
/*	?				0xFC000000 */
#define OVR_CLR			0x22ec
#define OVR_WID_LEFT_RIGHT	0x26ec
#define OVR_WID_TOP_BOTTOM	0x2aec
#define CUR_CLR0		0x2eec
#define CUR_CLR1		0x32ec
#define CUR_OFFSET		0x36ec
#define CUR_HORZ_VERT_POSN	0x3aec
#define CUR_HORZ_VERT_OFF	0x3eec
#define SCRATCH_REG0		0x42ec
#define SCRATCH_REG1		0x46ec
#define CLOCK_CNTL		0x4aec
#define BUS_CNTL		0x4eec
#define BUS_WS				0x0000000F
#define BUS_ROM_WS			0x000000F0
#define BUS_ROM_PAGE			0x00000F00
#define BUS_ROM_DIS			0x00001000
#define BUS_IO_16_EN			0x00002000
#define BUS_DAC_SNOOP_EN		0x00004000
/*	?				0x00008000 */
#define BUS_FIFO_WS			0x000F0000
#define BUS_FIFO_ERR_INT_EN		0x00100000
#define BUS_FIFO_ERR_INT		0x00200000
#define BUS_HOST_ERR_INT_EN		0x00400000
#define BUS_HOST_ERR_INT		0x00800000
#define BUS_PCI_DAC_WS			0x07000000
#define BUS_PCI_DAC_DLY			0x08000000
#define BUS_PCI_MEMW_WS			0x10000000
#define BUS_PCI_BURST_DEC		0x20000000
#define BUS_RDY_READ_DLY		0xC0000000
#define MEM_INFO		0x52ec		/* Changed from MEM_CNTL */
#define CTL_MEM_SIZE			0x00000007
/*	?				0x00000008 */
#define CTL_MEM_RD_LATCH_EN		0x00000010
#define CTL_MEM_RD_LATCH_DLY		0x00000020
#define CTL_MEM_SD_LATCH_EN		0x00000040
#define CTL_MEM_SD_LATCH_DLY		0x00000080
#define CTL_MEM_FULL_PLS		0x00000100
#define CTL_MEM_CYC_LNTH		0x00000600
/*	?				0x0000f800 */
#define CTL_MEM_BNDRY			0x00030000
#define CTL_MEM_BNDRY_0K			0x00000000
#define CTL_MEM_BNDRY_256K			0x00010000
#define CTL_MEM_BNDRY_512K			0x00020000
#define CTL_MEM_BNDRY_1024K			0x00030000
#define CTL_MEM_BNDRY_EN		0x00040000
/*	?				0xfff80000 */
#define MEM_VGA_WP_SEL		0x56ec
#define MEM_VGA_RP_SEL		0x5aec
#define DAC_REGS		0x5eec		/* Actually 4 separate bytes */
#define DAC_CNTL		0x62ec
#define GEN_TEST_CNTL		0x66ec
#define GEN_EE_DATA_OUT			0x00000001
#define GEN_EE_CLOCK			0x00000002
#define GEN_EE_CHIP_SEL			0x00000004
#define GEN_EE_DATA_IN			0x00000008
#define GEN_EE_EN			0x00000010
#define GEN_OVR_OUTPUT_EN		0x00000020
#define GEN_OVR_POLARITY		0x00000040
#define GEN_CUR_EN			0x00000080
#define GEN_GUI_EN			0x00000100
#define GEN_BLOCK_WR_EN			0x00000200
/*	?				0x0000FC00 */
#define GEN_TEST_FIFO_EN		0x00010000
#define GEN_TEST_GUI_REGS_EN		0x00020000
#define GEN_TEST_VECT_EN		0x00040000
#define GEN_TEST_CRC_STR		0x00080000
#define GEN_TEST_MODE			0x00700000
/*	?				0x00800000 */
#define GEN_TEST_MEM_WR			0x01000000
#define GEN_TEST_MEM_STROBE		0x02000000
#define GEN_TEST_DST_SS_EN		0x04000000
#define GEN_TEST_DST_SS_STROBE		0x08000000
#define GEN_TEST_SRC_SS_EN		0x10000000
#define GEN_TEST_SRC_SS_STROBE		0x20000000
#define GEN_TEST_CC_EN			0x40000000
#define GEN_TEST_CC_STROBE		0x80000000
#define CONFIG_CNTL		0x6aec
#define CFG_MEM_AP_SIZE			0x00000003
#define CFG_MEM_VGA_AP_EN		0x00000004
/*	?				0x00000008 */
#define CFG_MEM_AP_LOC			0x00003FF0
/*	?				0x0000C000 */
#define CFG_CARD_ID			0x00070000
#define CFG_VGA_DIS			0x00080000
/*	?				0xFFF00000 */
#define CONFIG_CHIP_ID		0x6eec		/* Read */
#define CFG_CHIP_TYPE0			0x000000FF
#define CFG_CHIP_TYPE1			0x0000FF00
#define CFG_CHIP_TYPE			0x0000FFFF
#define CFG_CHIP_CLASS			0x00FF0000
#define CFG_CHIP_REV			0xFF000000
#define CONFIG_STATUS_0		0x72ec		/* Read */
#define CFG_BUS_TYPE			0x00000007
#define CFG_MEM_TYPE			0x00000038
#define CFG_DUAL_CAS_EN			0x00000040
#define CFG_LOCAL_BUS_OPTION		0x00000180
#define CFG_INIT_DAC_TYPE		0x00000E00
#define CFG_INIT_CARD_ID		0x00007000
#define CFG_TRI_BUF_DIS			0x00008000
#define CFG_EXT_RAM_ADDR		0x003F0000
#define CFG_ROM_DIS			0x00400000
#define CFG_VGA_EN			0x00800000
#define CFG_LOCAL_BUS_CFG		0x01000000
#define CFG_CHIP_EN			0x02000000
#define CFG_LOCAL_READ_DLY_DIS		0x04000000
#define CFG_ROM_OPTION			0x08000000
#define CFG_BUS_OPTION			0x10000000
#define CFG_LOCAL_DAC_WR_EN		0x20000000
#define CFG_VLB_RDY_DIS			0x40000000
#define CFG_AP_4GBYTE_DIS		0x80000000
/*	CONFIG_STATUS_1		0x76ec */	/* Read */	/* Duplicate */
#define CFG_PCI_DAC_CFG			0x00000001
/*	?			0x7aec */
/*	?			0x7eec */
/*	?			0x82ec */
/*	?			0x86ec */
/*	?			0x8aec */
/*	?			0x8eec */
/*	?			0x92ec */
/*	?			0x96ec */
/*	?			0x9aec */
/*	?			0x9eec */
/*	?			0xa2ec */
/*	?			0xa6ec */
/*	?			0xaaec */
/*	?			0xaeec */
/*	?			0xb2ec */
/*	?			0xb6ec */
/*	?			0xbaec */
/*	?			0xbeec */
/*	?			0xc2ec */
/*	?			0xc6ec */
/*	?			0xcaec */
/*	?			0xceec */
/*	?			0xd2ec */
/*	?			0xd6ec */
/*	?			0xdaec */
/*	?			0xdeec */
/*	?			0xe2ec */
/*	?			0xe6ec */
/*	?			0xeaec */
/*	?			0xeeec */
/*	?			0xf2ec */
/*	?			0xf6ec */
/*	?			0xfaec */
/*	?			0xfeec */

/* Miscellaneous */

/* Current X, Y & Dest X, Y mask */
#define COORD_MASK	0x07ff

/* The Mixes */
#define	MIX_MASK			0x001f

#define	MIX_NOT_DST			0x0000
#define	MIX_0				0x0001
#define	MIX_1				0x0002
#define	MIX_DST				0x0003
#define	MIX_NOT_SRC			0x0004
#define	MIX_XOR				0x0005
#define	MIX_XNOR			0x0006
#define	MIX_SRC				0x0007
#define	MIX_NAND			0x0008
#define	MIX_NOT_SRC_OR_DST		0x0009
#define	MIX_SRC_OR_NOT_DST		0x000a
#define	MIX_OR				0x000b
#define	MIX_AND				0x000c
#define	MIX_SRC_AND_NOT_DST		0x000d
#define	MIX_NOT_SRC_AND_DST		0x000e
#define	MIX_NOR				0x000f

#define	MIX_MIN				0x0010
#define	MIX_DST_MINUS_SRC		0x0011
#define	MIX_SRC_MINUS_DST		0x0012
#define	MIX_PLUS			0x0013
#define	MIX_MAX				0x0014
#define	MIX_HALF__DST_MINUS_SRC		0x0015
#define	MIX_HALF__SRC_MINUS_DST		0x0016
#define	MIX_AVERAGE			0x0017
#define	MIX_DST_MINUS_SRC_SAT		0x0018
#define	MIX_SRC_MINUS_DST_SAT		0x001a
#define	MIX_HALF__DST_MINUS_SRC_SAT	0x001c
#define	MIX_HALF__SRC_MINUS_DST_SAT	0x001e
#define	MIX_AVERAGE_SAT			0x001f
#define MIX_FN_PAINT			MIX_SRC

/* Wait until "n" queue entries are free */
#define ibm8514WaitQueue(n)						\
	{								\
		while (inw(GP_STAT) & (0x0100 >> (n)));			\
	}
#define ATIWaitQueue(n)							\
	{								\
		while (inw(EXT_FIFO_STATUS) & (0x10000 >> (n)));	\
	}

/* Wait until GP is idle and queue is empty */
#define WaitIdleEmpty()							\
	{								\
		while (inw(GP_STAT) & (GPBUSY | 1));			\
	}
#define ProbeWaitIdleEmpty()						\
	{								\
		int i;							\
		for (i = 0; i < 100000; i++)				\
			if (!(inw(GP_STAT) & (GPBUSY | 1)))		\
				break;					\
	}

/* Wait until GP has data available */
#define WaitDataReady()							\
	{								\
		while (!(inw(GP_STAT) & DATARDY));			\
	}

#endif /* _REGATI_H_ */
