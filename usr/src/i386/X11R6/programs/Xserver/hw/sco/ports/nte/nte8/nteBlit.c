/*
 *	@(#) nteBlit.c 11.1 97/10/22
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
 * S001, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S000, 09-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

void
NTE(CopyRect)(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	int dst_x1, dst_y1, src_x1, src_y1, lx, ly;
	unsigned short command = NTE_BLIT_XP_YP_Y;

	dst_x1 = pdstBox->x1;
	dst_y1 = pdstBox->y1;
	src_x1 = psrc->x;
	src_y1 = psrc->y;
	lx = pdstBox->x2 - dst_x1 - 1;
	ly = pdstBox->y2 - dst_y1 - 1;

	if (dst_x1 > src_x1)
	{
		dst_x1 += lx;
		src_x1 += lx;
		command &= ~NTE_CMD_YN_XP_X;
	}
	if (dst_y1 > src_y1)
	{
		dst_y1 += ly;
		src_y1 += ly;
		command &= ~NTE_CMD_YP_XN_X;
	}

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(1);
	NTE_CLEAR_QUEUE24(4);
	NTE_WRT_MASK(planemask);
	NTE_CLEAR_QUEUE(8);
	NTE_FRGD_MIX(NTE_VIDEO_SOURCE, NTE(RasterOps)[alu]);
	NTE_CLEAR_QUEUE24(7);
	NTE_CURX(src_x1);
	NTE_CURY(src_y1);
	NTE_DESTX_DIASTP(dst_x1);
	NTE_DESTY_AXSTP(dst_y1);
	NTE_MAJ_AXIS_PCNT(lx);
	NTE_MIN_AXIS_PCNT(ly);
	NTE_CMD(command);
	NTE_END();
}
