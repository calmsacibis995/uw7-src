/*
 *	@(#) nteMono.c 11.1 97/10/22
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
 * S004, 20-Aug-93, staceyc
 * 	added wait for idle check to avoid fifo design bug
 * S003, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S002, 13-Jul-93, staceyc
 * 	generalize code to work with both i/o ports and memory mapped ports
 * S001, 16-Jun-93, staceyc
 * 	nasty error in macro name in opaque mono fixed
 * S000, 11-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

void
NTE(DrawMonoImage)(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int x, y, offset, clip_x, width, height;
	int width_words, width_bytes, i, extra;
	unsigned short *pix_trans16;
	unsigned char *pix_trans, *image_p;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	offset = startx / 8;
	startx %= 8;
	image += offset;
	y = pbox->y1;
	height = pbox->y2 - y;
	x = pbox->x1;
	clip_x = x;
	x -= startx;
	width = pbox->x2 - x;
#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
	pix_trans = (unsigned char *)pix_trans16;
#endif

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(3);
	NTE_CLEAR_QUEUE24(5);
	NTE_CURX(x);
	NTE_CURY(y);
	NTE_PIX_CNTL(NTE_CPU_DATA_MIX);
	NTE_CLEAR_QUEUE(8);
    	NTE_MAJ_AXIS_PCNT(width - 1);
	NTE_MIN_AXIS_PCNT(height - 1);
	NTE_CLEAR_QUEUE24(8);
	NTE_FRGD_COLOR(fg);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, NTE(RasterOps)[GXnoop]);
	NTE_SCISSORS_L(clip_x);
	NTE_WRT_MASK(planemask);
	NTE_WAIT_FOR_IDLE();
	NTE_CMD(S3C_WRITE_X_Y_DATA);

	width_bytes = (width + 7) / 8;
	width_words = width_bytes / 2;
	extra = width_bytes & 1;
	while (height--)
	{
		image_p = image;
		i = width_words;
		while (i--)
		{
			NTE_PIX_TRANS(*pix_trans16, MSBIT_SWAP(image_p[0]) |
			    (MSBIT_SWAP(image_p[1]) << 8));
			image_p += 2;
		}
		if (extra)
			NTE_PIX_TRANS(*pix_trans, MSBIT_SWAP(*image_p));
		image += stride;
	}

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(2);
	NTE_SCISSORS_L(0);
	NTE_PIX_CNTL(0);
	NTE_END();
}

void
NTE(DrawOpaqueMonoImage)(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int x, y, offset, clip_x, width, height;
	int width_words, width_bytes, i, extra;
	unsigned short *pix_trans16;
	unsigned char *pix_trans, *image_p;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	offset = startx / 8;
	startx %= 8;
	image += offset;
	y = pbox->y1;
	height = pbox->y2 - y;
	x = pbox->x1;
	clip_x = x;
	x -= startx;
	width = pbox->x2 - x;
#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
	pix_trans = (unsigned char *)pix_trans16;
#endif

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(3);
	NTE_CLEAR_QUEUE24(7);
	NTE_CURX(x);
	NTE_CURY(y);
	NTE_PIX_CNTL(NTE_CPU_DATA_MIX);
	NTE_CLEAR_QUEUE(8);
    	NTE_MAJ_AXIS_PCNT(width - 1);
	NTE_MIN_AXIS_PCNT(height - 1);
	NTE_FRGD_COLOR(fg);
	NTE_CLEAR_QUEUE24(8);
	NTE_BKGD_COLOR(bg);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_SCISSORS_L(clip_x);
	NTE_WRT_MASK(planemask);
	NTE_WAIT_FOR_IDLE();
	NTE_CMD(S3C_WRITE_X_Y_DATA);

	width_bytes = (width + 7) / 8;
	width_words = width_bytes / 2;
	extra = width_bytes & 1;
	while (height--)
	{
		image_p = image;
		i = width_words;
		while (i--)
		{
			NTE_PIX_TRANS(*pix_trans16, MSBIT_SWAP(image_p[0]) |
			    (MSBIT_SWAP(image_p[1]) << 8));
			image_p += 2;
		}
		if (extra)
			NTE_PIX_TRANS(*pix_trans, MSBIT_SWAP(*image_p));
		image += stride;
	}

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(2);
	NTE_SCISSORS_L(0);
	NTE_PIX_CNTL(0);
	NTE_END();
}
