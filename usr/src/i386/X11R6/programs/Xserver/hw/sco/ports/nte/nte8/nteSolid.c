/*
 *	@(#) nteSolid.c 11.1 97/10/22
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
NTE(DrawSolidRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	short x1, y1;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(3);
	NTE_CLEAR_QUEUE24(5);
	NTE_WRT_MASK(planemask);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_FRGD_COLOR(fg);

	while (nbox--)
	{
		NTE_CLEAR_QUEUE(5);
		NTE_CLEAR_QUEUE24(5);
		x1 = pbox->x1;
		NTE_CURX(x1);
		NTE_MAJ_AXIS_PCNT(pbox->x2 - x1 - 1);
		y1 = pbox->y1;
		NTE_CURY(y1);
		NTE_MIN_AXIS_PCNT(pbox->y2 - y1 - 1);
		NTE_CMD(NTE_FILL_X_Y_DATA);
		++pbox;
	}
	NTE_END();
}

void
NTE(DrawPoints)(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	int count = npts;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(3);
	NTE_CLEAR_QUEUE24(6);
	NTE_WRT_MASK(planemask);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_FRGD_COLOR(fg);
	NTE_MAJ_AXIS_PCNT(0);

	while ((count -= 2) >= 0)
	{
		NTE_CLEAR_QUEUE(6);
		NTE_CLEAR_QUEUE24(6);
		NTE_CURX(ppt->x);
		NTE_CURY(ppt->y);
		NTE_CMD(S3C_CMD_LINE | S3C_CMD_DRAW | S3C_CMD_RADIAL |
		    S3C_CMD_MULTIPLE | S3C_CMD_WRITE);
		++ppt;
		NTE_CURX(ppt->x);
		NTE_CURY(ppt->y);
		NTE_CMD(S3C_CMD_LINE | S3C_CMD_DRAW | S3C_CMD_RADIAL |
		    S3C_CMD_MULTIPLE | S3C_CMD_WRITE);
		++ppt;
	}
	if (count == -1)
	{
		NTE_CLEAR_QUEUE(3);
		NTE_CLEAR_QUEUE24(3);
		NTE_CURX(ppt->x);
		NTE_CURY(ppt->y);
		NTE_CMD(S3C_CMD_LINE | S3C_CMD_DRAW | S3C_CMD_RADIAL |
		    S3C_CMD_MULTIPLE | S3C_CMD_WRITE);
	}
	NTE_END();
}

void
NTE(SolidZeroSeg)(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x,
	int y,
	int e,
	int e1,
	int e2,
	int len)
{
	unsigned short command;
	unsigned long planemask, fg;
	unsigned char rop;
	int cx, cy;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	/*
	 * e (0x92E8) and e1 (0x8AE8) ports have 12 bits and a sign bit,
	 * e2 (0x8EE8) has 13 bits and a sign bit, use gen to draw any
	 * line that would have its error terms truncated if they were
	 * written to these registers
	 */
	if (e > 0xfff || e < -0x1000 || e1 > 0xfff || e1 < -0x1000 ||
	    e2 > 0x1fff || e2 < -0x2000)
	{
		genSolidZeroSeg(pGC, pDraw, signdx, signdy, axis, x, y, e, e1,
		    e2, len);
		return;
	}

	rop = NTE(RasterOps)[pGCPriv->rRop.alu];
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

	if (signdx == -1)
		cx = x - len;
	else
		cx = x;
	if (signdy == -1)
		cy = y - len;
	else
		cy = y;

	if (axis == X_AXIS)
		command = S3C_LINE_XN_YN_X;
	else
		command = S3C_LINE_XN_YN_Y;
	if (signdx > 0)
		command |= S3C_CMD_YN_XP_X;
	if (signdy > 0)
		command |= S3C_CMD_YP_XN_X;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(5);
	NTE_WRT_MASK(planemask);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, rop);
	NTE_CLEAR_QUEUE(8);
	NTE_FRGD_COLOR(fg);
	NTE_CLEAR_QUEUE24(7);
	NTE_CURX(x);
	NTE_CURY(y);
	NTE_ERR_TERM(e);
	NTE_DESTY_AXSTP(e1);
	NTE_DESTX_DIASTP(e2);
	NTE_MAJ_AXIS_PCNT(len);
	NTE_CMD(command);
	NTE_END();
}
