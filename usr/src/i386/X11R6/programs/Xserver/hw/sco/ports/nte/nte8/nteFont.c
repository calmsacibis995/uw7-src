/*
 *	@(#) nteFont.c 11.1 97/10/22
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
 * S000, 15-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"


void
NTE(DownloadFont8)(
	unsigned char **bits, 
	int count, 
	int width, 
	int height, 
	int stride,
	int index,
	ScreenRec *pScreen)
{
	unsigned long writeplane;
	int i;
	DDXPointRec *coords;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	BoxRec box;

	writeplane = ntePriv->te8_fonts[index].readplane;
	coords = ntePriv->te8_fonts[index].coords;

	for (i = 0; i < count; ++i)
	{
		box.x1 = coords[i].x;
		box.y1 = coords[i].y;
		box.x2 = box.x1 + width;
		box.y2 = box.y1 + height;
		NTE(DrawOpaqueMonoImage)(&box, bits[i], 0, stride, ~0, 0,
		    GXcopy, writeplane, &WindowTable[pScreen->myNum]->drawable);
	}
}

void
NTE(DrawFontText)(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short glyph_width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw)
{
	DDXPointRec *coords;
	unsigned long readmask;
	int ch, dst_x, dst_y, glyph_height;
	unsigned char bkgd_mix_rop, frgd_mix_rop;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	if (transparent)
	{
		bkgd_mix_rop = NTE(RasterOps)[GXnoop];
		frgd_mix_rop = NTE(RasterOps)[alu];
	}
	else
	{
		bkgd_mix_rop = NTE(RasterOps)[alu];
		frgd_mix_rop = bkgd_mix_rop;
	}

	coords = ntePriv->te8_fonts[index].coords;
	readmask = ntePriv->te8_fonts[index].readplane;
	dst_x = pbox->x1;
	dst_y = pbox->y1;
	glyph_height = pbox->y2 - dst_y - 1;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(1);
	NTE_CLEAR_QUEUE24(5);
	NTE_FRGD_COLOR(fg);
	NTE_CLEAR_QUEUE(8);
	NTE_BKGD_COLOR(bg);
	NTE_PIX_CNTL(NTE_VIDEO_MEMORY_DATA_MIX);
	NTE_CLEAR_QUEUE24(8);
	NTE_WRT_MASK(planemask);
	NTE_RD_MASK(readmask);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, frgd_mix_rop);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, bkgd_mix_rop);
	NTE_MAJ_AXIS_PCNT(glyph_width - 1);
	NTE_MIN_AXIS_PCNT(glyph_height);

	while (count--)
	{
		ch = *chars++;
		NTE_CLEAR_QUEUE(5);
		NTE_CLEAR_QUEUE24(5);
		NTE_CURX(coords[ch].x);
		NTE_CURY(coords[ch].y);
		NTE_DESTY_AXSTP(dst_y);
		NTE_DESTX_DIASTP(dst_x);
		NTE_CMD(S3C_BLIT_XP_YP_Y);
		dst_x += glyph_width;
	}

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(3);
	NTE_RD_MASK(NTE_ALLPLANES);
	NTE_PIX_CNTL(0);
	NTE_END();
}
