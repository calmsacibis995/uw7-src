/*
 *	@(#) qvisHW.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Thu Oct 07 12:51:45 PDT 1993    davidw@sco.com
 *      - Integrated Compaq source handoff
 *
 */

/**
 *
 * @(#) qvisHW.h 11.1 97/10/22
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  Originated (see RCS log)
 * waltc     02/04/93  Fix V32 transparent text bug.
 * waltc     10/05/93  Add controller version registers.
 */

#ifndef _QVIS_HW_H
#define _QVIS_HW_H

#define X0_LREG            0x63c0
#define Y0_LREG            0x63c2
#define X1_LREG            0x83cc
#define Y1_LREG            0x83ce

#define X0_BREG            0x63c0
#define Y0_BREG            0x63c2
#define X1_BREG            0x63cc
#define Y1_BREG            0x63ce

#define WIDTH_REG          0x23c2
#define HEIGHT_REG         0x23c4
#define SOURCE_PITCH_REG   0x23ca
#define DEST_PITCH_REG     0x23ce
#define BLT_CMD0           0x33ce
#define BLT_CMD1           0x33cf
#define CTRL_REG0            0x40
#define CTRL_REG1          0x63ca

#define GC_INDEX            0x3Ce
#define GC_DATA             0x3Cf
#define GS_INDEX            0x3c4
#define GS_DATA             0x3c5

#define GC_FG_COLOR          0x43
#define GC_BG_COLOR          0x44
#define ROPS_REG           0x33c7
#define DATAPATH_REG         0x5a
#define BOARD_SELECT       0x83c4
#define PALETTE_WRITE       0x3c8
#define PALETTE_DATA        0x3c9
#define PAGE_REG1            0x45
#define PAGE_REG2            0x46
#define LINE_COMMAND_REG     0x60
#define LINE_PATTERN_PTR     0x61
#define LINE_PAT_END_PTR     0x62
#define LINE_SIGN_CODES      0x63
#define LINE_PEL_CNT         0x64
#define LINE_ERROR_TERM      0x66
#define LINE_K1_CONST        0x68
#define LINE_K2_CONST        0x6a

#define LINE_SIGN_DX	     0x04
#define LINE_SIGN_DY	     0x02
#define LINE_MAJ_AXIS	     0x01

#define BLT_ROT	           0x33c8
#define BLT_SKEW           0x33c9
#define PAT0_REG           0x33ca
#define PAT1_REG           0x33cb
#define PAT2_REG           0x33cc
#define PAT3_REG           0x33cd
#define PAT4_REG           0x33ca
#define PAT5_REG           0x33cb
#define PAT6_REG           0x33cc
#define PAT7_REG           0x33cd

#define LINE_PAT0	   0x83c0
#define LINE_PAT1	   0x83c1
#define LINE_PAT2	   0x83c2
#define LINE_PAT3	   0x83c3

#define ROPSELECT_NO_ROPS              0x00
#define ROPSELECT_PRIMARY_ONLY         0x40
#define ROPSELECT_ALL_EXCPT_PRIMARY    0x80
#define ROPSELECT_ALL                  0xc0
#define PIXELMASK_ONLY                 0x00
#define PIXELMASK_AND_SRC_DATA         0x10
#define PIXELMASK_AND_CPU_DATA         0x20
#define PIXELMASK_AND_SCRN_LATCHES     0x30
#define PLANARMASK_ONLY                0x00
#define PLANARMASK_NONE_0XFF           0x04
#define PLANARMASK_AND_CPU_DATA        0x08
#define PLANARMASK_AND_SCRN_LATCHES    0x0c
#define SRC_IS_CPU_DATA                0x00
#define SRC_IS_SCRN_LATCHES            0x01
#define SRC_IS_PATTERN_REGS            0x02
#define SRC_IS_LINE_PATTERN            0x03

#define CREG1_MASK         0xe7
#define CREG1_PACKED_VIEW  0x00
#define CREG1_PLANAR_VIEW  0x08
#define CREG1_EXPAND_TO_FG 0x10
#define CREG1_EXPAND_TO_BG 0x18

#define BUFFER_BUSY_BIT    0x80
#define GLOBAL_BUSY_BIT    0x40
#define SS_BIT             0x1
#define GC_PLANE_MASK        0x08
#define GS_PIXEL_MASK        0x02

#define ROP_SOURCE_ONLY       0x0c

#define PLANAR_WRITE            0x00
#define COLOR_EXPAND_WRITE      0xc0
/* (waltc) Don't write to reserved bit 5 for V32 */
/* #define TRANSPARENT_WRITE       0x60 */
#define TRANSPARENT_WRITE       0x50
#define ON_OFF_DASH_LINE        0x43
#define DOUBLE_DASH_LINE	0xc3

#define ROP_NEG                 0x03
#define ROP_XOR                 0x06
#define ROP_AND                 0x08
#define ROP_SRC                 0x0c
#define ROP_OR                  0x0e

#define CTLR_VERS		0x0c
#define CTLR_EXT_VERS		0x0d

#endif				/* _QVIS_HW_H */
