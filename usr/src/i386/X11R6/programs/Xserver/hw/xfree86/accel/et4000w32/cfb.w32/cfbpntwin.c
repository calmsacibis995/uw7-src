/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/cfb.w32/cfbpntwin.c,v 3.2 1995/01/28 15:50:19 dawes Exp $ */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: cfbpntwin.c /main/3 1995/11/12 16:17:43 kaleb $ */

#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
#include "mi.h"
#include "w32box.h"

void
cfbPaintWindow(pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    register cfbPrivWin	*pPrivWin;
    WindowPtr	pBgWin;

    pPrivWin = cfbGetWindowPrivate(pWin);

    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    break;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
							     what);
	    break;
	case BackgroundPixmap:
	    if (pPrivWin->fastBackground)
	    {
		cfbFillBoxTile32 ((DrawablePtr)pWin,
				  (int)REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion),
				  pPrivWin->pRotatedBackground);
	    }
	    else
	    {
		cfbFillBoxTileOdd ((DrawablePtr)pWin,
				   (int)REGION_NUM_RECTS(pRegion),
				   REGION_RECTS(pRegion),
				   pWin->background.pixmap,
				   (int) pWin->drawable.x, (int) pWin->drawable.y);
	    }
	    break;
	case BackgroundPixel:
	    cfbFillBoxSolid ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    break;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    cfbFillBoxSolid ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel);
	}
	else if (pPrivWin->fastBorder)
	{
	    cfbFillBoxTile32 ((DrawablePtr)pWin,
			      (int)REGION_NUM_RECTS(pRegion),
			      REGION_RECTS(pRegion),
			      pPrivWin->pRotatedBorder);
	}
	else
	{
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);

	    cfbFillBoxTileOdd ((DrawablePtr)pWin,
			       (int)REGION_NUM_RECTS(pRegion),
			       REGION_RECTS(pRegion),
			       pWin->border.pixmap,
			       (int) pBgWin->drawable.x,
 			       (int) pBgWin->drawable.y);
	}
	break;
    }
}

/*
 * Use the RROP macros in copy mode
 */

#define RROP GXcopy
#include "cfbrrop.h"

#ifdef RROP_UNROLL
# define Expand(left,right,leftAdjust) {\
    int part = nmiddle & RROP_UNROLL_MASK; \
    int widthStep; \
    widthStep = widthDst - nmiddle - leftAdjust; \
    nmiddle >>= RROP_UNROLL_SHIFT; \
    while (h--) { \
	left \
	pdst += part; \
	switch (part) { \
	    RROP_UNROLL_CASE(pdst) \
	} \
	m = nmiddle; \
	while (m) { \
	    pdst += RROP_UNROLL; \
	    RROP_UNROLL_LOOP(pdst) \
	    m--; \
	} \
	right \
	pdst += widthStep; \
    } \
}

#else
# define Expand(left, right, leftAdjust) { \
    unsigned long *ggl_ptr; \
    ggl_ptr = pdst; \
    while (h--) { \
	pdst = ggl_ptr; \
	FB_LONG(pdst) \
	left \
	m = nmiddle; \
	while (m--) {\
	    RROP_SOLID(pdst); \
	    pdst++; \
	} \
	right \
	ggl_ptr += widthDst; \
    } \
}
#endif

void
cfbFillBoxSolid (pDrawable, nBox, pBox, pixel)
    DrawablePtr	    pDrawable;
    int		    nBox;
    BoxPtr	    pBox;
    unsigned long   pixel;
{
    unsigned long   *pdstBase;
    int		    widthDst;
    register int    h;
    register unsigned long   rrop_xor;
    register unsigned long   *pdst;
    register unsigned long   leftMask, rightMask;
    int		    nmiddle;
    register int    m;
    int		    w;

    cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase);

    rrop_xor = PFILL(pixel);

    if ((CARD32)pdstBase == VGABASE)
    {
	int dst;

	widthDst <<= 2;
	W32_INIT_BOX(GXcopy, 0xffffffff, rrop_xor, widthDst - 1)
	for (; nBox; nBox--, pBox++)
	{
	    dst = pBox->y1 * widthDst + (pBox->x1 * (PSZ >> 3));
	    h = pBox->y2 - pBox->y1;
	    w = pBox->x2 - pBox->x1;
	    WAIT_XY
	    W32_BOX(dst, w, h)
	}
	WAIT_XY
	return;
    }

    for (; nBox; nBox--, pBox++)
    {
    	pdst = pdstBase + pBox->y1 * widthDst;
    	h = pBox->y2 - pBox->y1;
	w = pBox->x2 - pBox->x1;
#if PSZ == 8
	if (w == 1)
	{
	    register char    *pdstb = ((char *) pdst) + pBox->x1;
	    int	    incr = widthDst * PGSZB;

	    while (h--)
	    {
		*pdstb = rrop_xor;
		pdstb += incr;
	    }
	}
	else
	{
#endif
	pdst += (pBox->x1 >> PWSH);
	if ((pBox->x1 & PIM) + w <= PPW)
	{
	    maskpartialbits(pBox->x1, w, leftMask);
	    while (h--)
	    {
		*pdst = (*pdst & ~leftMask) | (rrop_xor & leftMask);
		pdst += widthDst;
	    }
	}
	else
	{
	    maskbits (pBox->x1, w, leftMask, rightMask, nmiddle);
	    if (leftMask)
	    {
		if (rightMask)
		{
		    Expand (RROP_SOLID_MASK (pdst, leftMask); pdst++; ,
			    RROP_SOLID_MASK (pdst, rightMask); ,
			    1)
		}
		else
		{
		    Expand (RROP_SOLID_MASK (pdst, leftMask); pdst++;,
			    ;,
			    1)
		}
	    }
	    else
	    {
		if (rightMask)
		{
		    Expand (;,
			    RROP_SOLID_MASK (pdst, rightMask);,
			    0)
		}
		else
		{
		    Expand (;,
			    ;,
			    0)
		}
	    }
	}
#if PSZ == 8
	}
#endif
    }
}

void
cfbFillBoxTile32 (pDrawable, nBox, pBox, tile)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    BoxPtr 	    pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* rotated, expanded tile */
{
    register unsigned long  rrop_xor;	
    register unsigned long  *pdst;
    register int	    m;
    unsigned long	    *psrc;
    int			    tileHeight;

    int			    widthDst;
    int			    w;
    int			    h;
    register unsigned long  leftMask;
    register unsigned long  rightMask;
    int			    nmiddle;
    int			    y;
    int			    srcy;

    unsigned long	    *pdstBase;

    tileHeight = tile->drawable.height;
    psrc = (unsigned long *)tile->devPrivate.ptr;

    cfbGetLongWidthAndPointer (pDrawable, widthDst, pdstBase);
    TEST_SET_FB(pdstBase)

    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	y = pBox->y1;
	pdst = pdstBase + (pBox->y1 * widthDst) + (pBox->x1 >> PWSH);
	srcy = y % tileHeight;

#define StepTile    rrop_xor = psrc[srcy]; \
		    ++srcy; \
		    if (srcy == tileHeight) \
		        srcy = 0;

	if ( ((pBox->x1 & PIM) + w) < PPW)
	{
	    maskpartialbits(pBox->x1, w, leftMask);
	    rightMask = ~leftMask;
	    if (FrameBuffer)
	    {
		while (h--)
		{
		    StepTile
		    W32_SET_LONG(pdst)
		    *(LongP)W32Ptr = (*(LongP)W32Ptr & ~leftMask) | (rrop_xor & leftMask);
		    pdst += widthDst;
		}
	    }
	    else
	    {
		while (h--)
		{
		    StepTile
		    *pdst = (*pdst & ~leftMask) | (rrop_xor & leftMask);
		    pdst += widthDst;
		}
	    }
	}
	else
	{
	    maskbits(pBox->x1, w, leftMask, rightMask, nmiddle);

	    if (leftMask)
	    {
		if (rightMask)
		{
		    Expand (StepTile
			    RROP_SOLID_MASK(pdst, leftMask); pdst++;,
			    RROP_SOLID_MASK(pdst, rightMask);,
			    1)
		}
		else
		{
		    Expand (StepTile
			    RROP_SOLID_MASK(pdst, leftMask); pdst++;,
			    ;,
			    1)
		}
	    }
	    else
	    {
		if (rightMask)
		{
		    Expand (StepTile
			    ,
			    RROP_SOLID_MASK(pdst, rightMask);,
			    0)
		}
		else
		{
		    Expand (StepTile
			    ,
			    ;,
			    0)
		}
	    }
	}
        pBox++;
    }
}
