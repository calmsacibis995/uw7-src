/*
 *	@(#) nteGlyph.c 11.1 97/10/22
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
 * S003, 20-Aug-93, staceyc
 * 	add wait for idle fix for fifo bug with image transfers
 * S002, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S001, 13-Jul-93, staceyc
 * 	generalize source for use with both i/o ports and mem mapped ports
 * S000, 15-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

void
NTE(DrawMonoGlyphs)(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDraw)
{
	int x, y, i, width, height, width_bytes, width_words, extra, stride;
	unsigned short *pix_trans16;
	unsigned char *pix_trans, *image, *image_p;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	NTE_BEGIN(ntePriv->regs);
#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
	pix_trans = (unsigned char *)pix_trans16;
#endif

	NTE_CLEAR_QUEUE(5);
	NTE_CLEAR_QUEUE24(7);
	NTE_FRGD_COLOR(fg);
	NTE_PIX_CNTL(NTE_CPU_DATA_MIX);
	NTE_WRT_MASK(planemask);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, NTE(RasterOps)[GXnoop]);

	while (nglyphs--)
	{
		x = glyph_info->box.x1;
		width = glyph_info->box.x2 - x;
		width_bytes = (width + 7) / 8;
		width_words = width_bytes / 2;
		extra = width_bytes & 1;
		y = glyph_info->box.y1;
		height = glyph_info->box.y2 - y;
		stride = glyph_info->stride;
		image = glyph_info->image;
		++glyph_info;

		NTE_CLEAR_QUEUE(5);
		NTE_CLEAR_QUEUE24(5);
		NTE_CURX(x);
		NTE_CURY(y);
    		NTE_MAJ_AXIS_PCNT(width - 1);
		NTE_MIN_AXIS_PCNT(height - 1);
		NTE_WAIT_FOR_IDLE();
		NTE_CMD(S3C_WRITE_X_Y_DATA);
	
		while (height--)
		{
			image_p = image;
			i = width_words;
			while (i--)
			{
				NTE_PIX_TRANS(*pix_trans16,
				    MSBIT_SWAP(image_p[0]) |
				    (MSBIT_SWAP(image_p[1]) << 8));
				image_p += 2;
			}
			if (extra)
				NTE_PIX_TRANS(*pix_trans, MSBIT_SWAP(*image_p));
			image += stride;
		}
	}

	NTE_CLEAR_QUEUE(1);
	NTE_CLEAR_QUEUE24(1);
	NTE_PIX_CNTL(0);
	NTE_END();
}
