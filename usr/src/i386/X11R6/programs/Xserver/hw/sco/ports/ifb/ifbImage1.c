/*
 *	@(#) ifbImage1.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * ifbImage1.c
 *
 * ifb ReadImage and DrawImage routines for depth 1.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"

/*
 * ifbReadImage1() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
ifbReadImage1(pbox, image, stride, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	DrawablePtr pDraw;
{
}

/*
 * ifbDrawImage1() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
ifbDrawImage1(pbox, image, stride, alu, planemask, pDraw)
	BoxPtr pbox;
	unsigned char *image;
	unsigned int stride;
	unsigned char alu;
	unsigned long planemask;
	DrawablePtr pDraw;
{
}
