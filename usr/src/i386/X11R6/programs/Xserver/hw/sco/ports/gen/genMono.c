/*
 * @(#) genMono.c 11.1 97/10/22
 *
 * Copyright (C) 1992 The Santa Cruz Operation, Inc.
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
 * low level mono drawing primitives; they use DrawSolidRects 
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"		/* for BITMAP_BIT_ORDER */

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "genDefs.h"
#include "genProcs.h"

/*
 * Macros for (1) setting a particular bit within a byte,
 * and (2) shifting that bit.
 */
#if (BITMAP_BIT_ORDER == MSBFirst)	/* pc/rt, 680x0 */
#  define HIBIT(offset) (1 << (7 - offset))
#else
#  define HIBIT(offset) (1 << offset)
#endif

#if (BITMAP_BIT_ORDER == MSBFirst)
#  define SHIFTBIT_SCREENRIGHT(byte)	byte >>= 1
#else
#  define SHIFTBIT_SCREENRIGHT(byte)	byte <<= 1
#endif

/*
 * genDrawMonoImage - should work for 1, 8, 16, and 32 bits-per-pixel
 */
void
genDrawMonoImage(
    BoxPtr pbox,
    void *image_src,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDrawable)
{
    unsigned char *image = (unsigned char *)image_src;
    register int xline, y, w, h;
    unsigned char *pline;
    void (*DrawSolidRects)();

    y = pbox->y1;
    h = pbox->y2 - y;
    xline = pbox->x1;
    w = pbox->x2 - xline;

    DrawSolidRects = (void (*)())(NFB_WINDOW_PRIV(pDrawable))->ops->DrawSolidRects;

    if ( startx >= 8 ) {
	image += startx >> 3;
	startx &= 7;
    }
    
    for (pline = image; h--; y++, pline += stride) {
	register int x1, x2,  xoffset, endbit, bitsToGet;
	register unsigned int mask;
	unsigned char *pimage;
	BoxRec box;

	pimage = pline;
	box.y1 = y;
	box.y2 = y + 1;
 	x1 = -1;
	x2 = xline;
	xoffset = startx;

	mask = HIBIT(xoffset);
	bitsToGet = w + xoffset;
	while (bitsToGet > 0) {
	    unsigned char byte;
	    bitsToGet -= 8;
	    byte = *pimage++;
	    endbit = (bitsToGet < 0) ? 8 + bitsToGet : 8;
	    if (byte == 0) {
		if (x1 >= 0) {
		    /* draw a vector from x1 to x2 */
		    box.x1 = x1;
		    box.x2 = x2;
		    (*DrawSolidRects)(&box, 1, fg, alu,
			 planemask, pDrawable);
		    x1 = -1;
		}
		x2 += endbit - xoffset;
	    } else {
		while (xoffset++ != endbit) {
		    if ((byte & mask) == 0) {
			if (x1 >= 0) {
			    /* draw a vector from x1 to x2 */
			    box.x1 = x1;
			    box.x2 = x2;
			    (*DrawSolidRects)(&box, 1, fg, alu,
				 planemask, pDrawable);
			    x1 = -1;
			}
		    } else {
			if (x1 < 0)
				x1 = x2;
		    }
		    ++x2;
		    SHIFTBIT_SCREENRIGHT(mask);
		}
	    }
	    xoffset = 0;
	    mask = HIBIT(0);
	}
	if (x1 >= 0) {
		/* draw vector at end of scanline */
		box.x1 = x1;
		box.x2 = x2;
		(*DrawSolidRects)(&box, 1, fg, alu,
		     planemask, pDrawable);
	}
    }
    return;
}

/*
 * genDrawOpaqueMonoImage - should work for 1, 8, 16, and 32 bits-per-pixel
 */
void
genDrawOpaqueMonoImage(
	BoxPtr pbox,
	void *image_src,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable)
{
    unsigned char *image = (unsigned char*)image_src;
    register int xline, y, w, h;
    unsigned char *pline;
    void (*DrawSolidRects)();

    y = pbox->y1;
    h = pbox->y2 - y;
    xline = pbox->x1;
    w = pbox->x2 - xline;

    DrawSolidRects = (void (*)())(NFB_WINDOW_PRIV(pDrawable))->ops->DrawSolidRects;

    if (startx >= 8) {
	image += startx >> 3;
	startx &= 7;
    }
    for (pline = image; h--; y++, pline += stride) {
	register int x1, x2,  xoffset, endbit, bitsToGet, lastbit;
	register unsigned int mask;
	unsigned char *pimage;
	BoxRec box;

	pimage = pline;
	box.y1 = y;
	box.y2 = y + 1;
 	x1 = x2 = xline;
	xoffset = startx;
	mask = HIBIT(xoffset);
	lastbit = -1;
	bitsToGet = w + xoffset;
	while (bitsToGet > 0) {
	    unsigned char byte;
	    register int curbit;

	    bitsToGet -= 8;
	    byte = *pimage++;
	    endbit = (bitsToGet < 0) ? 8 + bitsToGet : 8;

	    if (byte == 0 || byte == 0xff) {
		curbit = byte ? 1 : 0;
		if (curbit != lastbit) {
		    if (x1 != x2) {    /* if not at start of line... */
			/* draw a vector from x1 to x2 */
			box.x1 = x1;
			box.x2 = x2;
			(*DrawSolidRects)(&box, 1, lastbit ? fg : bg,
			     alu, planemask, pDrawable);
			x1 = x2;
		    }
		    lastbit = curbit;
		}
		x2 += endbit - xoffset;
	    } else {
		while (xoffset++ != endbit) {
		    curbit = (byte & mask) ? 1 : 0;
		    if (curbit != lastbit) {
			/* draw a vector from x1 to x2 */
			if (x1 != x2) {    /* if not at start of line... */
			    box.x1 = x1;
			    box.x2 = x2;
			    (*DrawSolidRects)(&box, 1, lastbit ? fg : bg,
				    alu, planemask, pDrawable);
			    x1 = x2;
			}
			lastbit = curbit;
		    }
		    ++x2;
		    SHIFTBIT_SCREENRIGHT(mask);
		}
	    }
	    xoffset = 0;
	    mask = HIBIT(0);
	}
	/* draw vector at end of scanline */
	box.x1 = x1;
	box.x2 = x2;
	(*DrawSolidRects)(&box, 1, lastbit ? fg : bg, alu,
		     planemask, pDrawable);
    }
    return;
}
