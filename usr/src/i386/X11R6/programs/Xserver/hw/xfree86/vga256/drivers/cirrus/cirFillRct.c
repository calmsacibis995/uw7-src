/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cirFillRct.c,v 3.7 1995/04/09 14:14:21 dawes Exp $ */
/*

Copyright (c) 1989  X Consortium

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
*/
/* Modified for Cirrus by Harm Hanemaayer, <hhanemaa@cs.ruu.nl> */
/* $XConsortium: cirFillRct.c /main/3 1995/11/13 08:20:28 kaleb $ */

/*
 * Fill rectangles.
 */

/*
 * This file contains the high level PolyFillRect function.
 *
 * We need to reproduce this to be able to use our own non-GXcopy
 * solid fills, and tiles.
 *
 * Works for 8bpp, and linear framebuffer 16bpp and 32bpp (compiled once).
 */


#include "vga256.h"
#include "cfbrrop.h"
#include "mergerop.h"
#include "xf86.h"
#include "vga.h"
#include "vgaBank.h"	/* For CHECKSCREEN. */

#include "cir_driver.h"

#if PPW == 4
extern void cfb8FillRectStippledUnnatural();
#endif

extern void cfb16FillRectSolidCopy();
extern void cfb16FillRectSolidGeneral();
extern void cfb16FillRectTileOdd();
extern void cfb32FillRectSolidCopy();
extern void cfb32FillRectSolidGeneral();
extern void cfb32FillRectTileOdd();

#define NUM_STACK_RECTS	1024

void
CirrusPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    cfbPrivGC	    *priv;
    int		    numRects;
    void	    (*BoxFill)();
    int		    n;
    int		    xorg, yorg;
    unsigned long   *pdstBase;
    int		    widthDst;
    RROP_DECLARE

#if 0	/* Can't use this for different depths. */
    cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase)
#endif

    if (!xf86VTSema || pDrawable->type != DRAWABLE_WINDOW)
    {
    	if (vgaBitsPerPixel == 8) {
            cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit);
            return;
        }
    	if (vgaBitsPerPixel == 16) {
            cfb16PolyFillRect(pDrawable, pGC, nrectFill, prectInit);
            return;
        }
        if (vgaBitsPerPixel == 32) {
            cfb32PolyFillRect(pDrawable, pGC, nrectFill, prectInit);
            return;
        }
        return;
    }

    priv = (cfbPrivGC *) pGC->devPrivates[cfbGCPrivateIndex].ptr;
    prgnClip = priv->pCompositeClip;

    BoxFill = 0;
    switch (pGC->fillStyle)
    {
    case FillSolid:
	RROP_FETCH_GCPRIV(priv)
	switch (priv->rop) {
	case GXcopy:
	    if (cirrusUseMMIO && HAVEBITBLTENGINE())
	        BoxFill = CirrusMMIOFillRectSolid;	/* Optimized. */
	    else {
	    	if (vgaBitsPerPixel == 8)
	            BoxFill = vga256LowlevFuncs.fillRectSolidCopy;
	        else if (vgaBitsPerPixel == 16)
	            BoxFill = cfb16FillRectSolidCopy;
	        else
	            BoxFill = cfb32FillRectSolidCopy;
	    }
	    break;
	default:
	    /* BoxFill = cfbFillRectSolidGeneral; */
	    if (cirrusUseMMIO && HAVEBITBLTENGINE())
	        BoxFill = CirrusMMIOFillRectSolid;
	    else {
	        if (vgaBitsPerPixel == 8)
	            BoxFill = CirrusFillRectSolidGeneral;
	        else if (vgaBitsPerPixel == 16)
	            BoxFill = cfb16FillRectSolidGeneral;
	        else
	            BoxFill = cfb32FillRectSolidGeneral;
	    }
	    break;
	}
	break;
    case FillTiled:
    	/* Hmm, it seems FillRectTileOdd always gets called. --HH */
#if 0    
	if (!((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr)->
	pRotatedPixmap)
            BoxFill = cfbFillRectTileOdd;
	else
#endif
	if (vgaBitsPerPixel == 8)
	{
	    if (pGC->alu == GXcopy && (pGC->planemask & 0xFF) == 0xFF)
		/* BoxFill = cfbFillRectTile32Copy; */
		BoxFill = CirrusFillRectTile;
	    else
		BoxFill = vga256FillRectTileOdd;
	}
	else if (vgaBitsPerPixel == 16)
	    BoxFill = cfb16FillRectTileOdd;
	else
	    BoxFill = cfb32FillRectTileOdd;
	break;
#if (PPW == 4)
    case FillStippled:
        /*
         * There's an unresolved conflict between MMIO + linear addressing
         * and the color expand stipple function (MMIO fills tend to
         * go wrong).
         */
	if ((cirrusUseMMIO && cirrusUseLinear) ||
	!((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr)->
							pRotatedPixmap)
	    BoxFill = vga2568FillRectStippledUnnatural;
	else
	    BoxFill = CirrusFillRectTransparentStippled32;
	break;
    case FillOpaqueStippled:
	if ((cirrusUseMMIO && cirrusUseLinear) ||
	!((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr)->
							pRotatedPixmap)
	    BoxFill = vga2568FillRectStippledUnnatural;
	else
	    BoxFill = CirrusFillRectOpaqueStippled32;
	break;
#endif
    }
    prect = prectInit;
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }
    if (pboxClipped != pboxClippedBase)
	(*BoxFill) (pDrawable, pGC,
		    pboxClipped-pboxClippedBase, pboxClippedBase);
    if (pboxClippedBase != stackRects)
    	DEALLOCATE_LOCAL(pboxClippedBase);
}
