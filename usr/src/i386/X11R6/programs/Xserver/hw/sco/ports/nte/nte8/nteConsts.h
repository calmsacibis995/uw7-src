/*
 *	@(#) nteConsts.h 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S011, 17-Mar-93, hiramc
 *	need S3 Lock Registers 1 and 2 access, and system configuration
 *	register access.
 * S010, 20-Aug-93, staceyc
 * 	hardware cursor constants
 * S009, 22-Jul-93, staceyc
 * 	constants for memory size determination
 * S008, 21-Jul-93, staceyc
 * 	generic screen blanking for all depths
 * S007, 19-Jul-93, staceyc
 * 	constants to get 8bits per pixel screen blanking
 * S006, 09-Jul-93, staceyc
 * 	constants to try and get past chip bugs in 24 bit modes
 * S005, 16-Jun-93, staceyc
 * 	data ready bit value added
 * S004, 16-Jun-93, staceyc
 * 	constants for clipping and fast text
 * S003, 11-Jun-93, staceyc
 * 	constants for mono images
 * S002, 09-Jun-93, staceyc
 * 	constants for solid fills, blits, bres lines
 * S001, 08-Jun-93, staceyc
 * 	constants to get image code working
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#ifndef NTECONSTS_H
#define NTECONSTS_H

#include <math.h>
/*
#include <sys/console.h>
*/
#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "grafinfo.h"
#include "ddxScreen.h"
#include "os.h"
/*
#include "compiler.h"
*/
#include "cursor.h"
#include "cursorstr.h"
#include "servermd.h"
#include "nfb/nfbGlyph.h"
#include "gen/genProcs.h"
#include "window.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbProcs.h"
#include "scoext.h"
#include "s3/s3cConsts.h"

#define NTE_PALWRITE_ADDR 0x3C8
#define NTE_PALDATA       0x3C9

#define NTE_ADVFUNC_CNTL S3C_MISCIO
#define NTE_GP_STAT S3C_CMD
#define NTE_COLOR_CMP_REG 0xB2E8
#define NTE_RD_REG_DT 0xBEE8

#define NTE_MISC_READ 0x3CC
#define NTE_IOA_SEL (1 << 0)

#define NTE_CRTC_ADR_COLOR  0x3D4
#define NTE_CRTC_ADR_MONO   0x3B4
#define NTE_CRTC_DATA_COLOR 0x3D5
#define NTE_CRTC_DATA_MONO  0x3B5

#define NTE_CONFG_REG1_INDEX 0x36
#define NTE_CONFG_REG2_INDEX 0x37
#define NTE_LOCK_REG_1_INDEX 0x38
#define NTE_LOCK_REG_2_INDEX 0x39
#define NTE_SYS_CNFG_INDEX 0x40

#define NTE_CRE        0x0E
#define NTE_CRF        0x0F
#define NTE_HGC_MODE   0x45
#define NTE_HWGC_ORGXH 0x46
#define NTE_HWGC_ORGXL 0x47
#define NTE_HWGC_ORGYH 0x48
#define NTE_HWGC_ORGYL 0x49
#define NTE_HWGC_FGSTK 0x4A
#define NTE_HWGC_BGSTK 0x4B
#define NTE_HWGC_STAH  0x4C
#define NTE_HWGC_STAL  0x4D
#define NTE_HWGC_DX    0x4E
#define NTE_HWGC_DY    0x4F
#define NTE_EX_DAC_CT  0x55
#define NTE_UNLOCK_S3_REGS	0x48
#define NTE_UNLOCK_S3CTL_EXT_REGS	0xad
#define NTE_ENABLE_ENHANCED_REGS_BIT	0x01

#define NTE_HWGC_ENB (1 << 0)

#define NTE_MS_X11 (1 << 4)

#define NTE_DISP_MEM_SIZE_MASK 0xE0
#define NTE_4MEG               0x00
#define NTE_3MEG               0x40
#define NTE_2MEG               0x80
#define NTE_1MEG               0xC0
#define NTE_HALF_MEG           0xE0

#define NTE_1MEGABYTE 0x100000

#define NTE_MIO (1 << 5)

#define NTE_ALLPLANES (~0)

#define NTE_REG_SIZE sizeof(ntePixelReg)

#define NTE_WRITE_Z_DATA   S3C_WRITE_Z_DATA
#define NTE_READ_Z_DATA    S3C_READ_Z_DATA
#define NTE_BLIT_XP_YP_Y   S3C_BLIT_XP_YP_Y
#define NTE_CMD_YN_XP_X    S3C_CMD_YN_XP_X
#define NTE_CMD_YP_XN_X    S3C_CMD_YP_XN_X
#define NTE_FILL_X_Y_DATA  S3C_FILL_X_Y_DATA

#define NTE_SCISSORS_T_INDEX 0x1000
#define NTE_SCISSORS_L_INDEX 0x2000
#define NTE_SCISSORS_B_INDEX 0x3000
#define NTE_SCISSORS_R_INDEX 0x4000
#define NTE_PIX_CNTL_INDEX   0xA000
#define NTE_MULT_MISC_INDEX  0xE000
#define NTE_READ_SEL_INDEX   0xF000

#define NTE_MULT_MISC_SEL 0x006

#define NTE_RSF     (1 << 4)
#define NTE_ENB_CMP (1 << 8)

#define NTE_CPU_DATA_MIX          0x0080
#define NTE_VIDEO_MEMORY_DATA_MIX 0x00C0

#define NTE_BKGD_SOURCE  0x0000
#define NTE_FRGD_SOURCE  0x0020
#define NTE_CPU_SOURCE   0x0040
#define NTE_VIDEO_SOURCE 0x0060

#define NTE_RDT_AVA (1 << 8)
#define NTE_HDW_BSY (1 << 9)

#define NTE_TE8_GLYPH_SIZE 16
#define NTE_TE8_FONT_MAX 16

#define NTE_HW_CURSOR_DATA_SIZE 1024
#define NTE_HW_CURSOR_MAX         64

#define NTE_SEQX     0x3C4
#define NTE_SEQ_DATA 0x3C5

#define NTE_CLK_MODE_INDEX 0x01
#define NTE_SCRN_OFF (1 << 5)

#endif
