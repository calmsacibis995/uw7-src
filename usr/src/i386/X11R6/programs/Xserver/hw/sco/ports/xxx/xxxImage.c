/*
 * @(#) xxxImage.c 11.1 97/10/22
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
 * xxxImage.c
 *
 * Template for machine dependent ReadImage and DrawImage routines
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

/*
 * xxxReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte;
 *		pack 2- to 8-bit pixels one per byte;
 *		pack 9- to 16-bit pixels one per 16-bit short;
 *		pack 17- to 32-bit pixels one per 32-bit word.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
xxxReadImage(pbox, image, stride, pDraw)
	BoxPtr pbox;
	void *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
	int height, width;

	/* width and height in pixels */
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

#ifdef DEBUG_PRINT
	ErrorF("xxxReadImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d)\n", image, stride);
#endif
}

/*
 * xxxDrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
xxxDrawImage(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	void *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
	int height, width;

	/* width and height in pixels */
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

#ifdef DEBUG_PRINT
	ErrorF("xxxDrawImage(box=(%d,%d)-(%d,%d), ",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2);
	ErrorF("image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
		image, stride, alu, planemask);
#endif
}
