/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/cfb.w32/w32fillarc.c,v 3.2 1995/01/28 15:50:33 dawes Exp $ */
/************************************************************

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

********************************************************/
/* $XConsortium: w32fillarc.c /main/3 1995/11/12 16:18:09 kaleb $ */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "mifillarc.h"
#include "cfbrrop.h"
#include "mi.h"
#include "w32.h"

/* gcc 1.35 is stupid */
#if defined(__GNUC__) && defined(mc68020)
#define STUPID volatile
#else
#define STUPID
#endif

static void
RROP_NAME(w32FillEllipseSolid) (pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    STUPID int x, y, e;
    STUPID int yk, xk, ym, xm, dx, dy, xorg, yorg;
    miFillArcRec info;
    unsigned long *addrlt, *addrlb;
    register unsigned long *addrl;
    register int n;
    int nlwidth;
    RROP_DECLARE
    register int xpos;
    register int slw;
    unsigned long startmask, endmask;
    int	nlmiddle;

    cfbGetLongWidthAndPointer (pDraw, nlwidth, addrlt)
    addrlt = 0;
    RROP_FETCH_GC(pGC);
    miFillArcSetup(arc, &info);
    MIFILLARCSETUP();
    xorg += pDraw->x;
    yorg += pDraw->y;
    addrlb = addrlt;
    addrlt += nlwidth * (yorg - y);
    addrlb += nlwidth * (yorg + y + dy);
    while (y)
    {
	addrlt += nlwidth;
	addrlb -= nlwidth;
	MIFILLARCSTEP(slw);
	if (!slw)
	    continue;
	xpos = xorg - x;
	addrl = addrlt + (xpos >> PWSH);
	W32_LONG(addrl)
	if (((xpos & PIM) + slw) <= PPW)
	{
	    maskpartialbits(xpos, slw, startmask);
	    RROP_SOLID_MASK(addrl,startmask);
	    if (miFillArcLower(slw))
	    {
		addrl = addrlb + (xpos >> PWSH);
		W32_LONG(addrl)
		RROP_SOLID_MASK(addrl, startmask);
	    }
	    continue;
	}
	maskbits(xpos, slw, startmask, endmask, nlmiddle);
	if (startmask)
	{
	    RROP_SOLID_MASK(addrl, startmask);
	    addrl++;
	}
	n = nlmiddle;
	RROP_SPAN(addrl,n)

	if (endmask)
	    RROP_SOLID_MASK(addrl, endmask);
	if (!miFillArcLower(slw))
	    continue;
	addrl = addrlb + (xpos >> PWSH);
	W32_LONG(addrl)
	if (startmask)
	{
	    RROP_SOLID_MASK(addrl, startmask);
	    addrl++;
	}
	n = nlmiddle;
	RROP_SPAN(addrl, n);
	if (endmask)
	    RROP_SOLID_MASK(addrl, endmask);
    }
}

#define FILLSPAN(xl,xr,addr) \
    if (xr >= xl) \
    { \
	n = xr - xl + 1; \
	addrl = addr + (xl >> PWSH); \
	W32_LONG(addrl) \
	if (((xl & PIM) + n) <= PPW) \
	{ \
	    maskpartialbits(xl, n, startmask); \
	    RROP_SOLID_MASK(addrl, startmask); \
	} \
	else \
	{ \
	    maskbits(xl, n, startmask, endmask, n); \
	    if (startmask) \
	    { \
		RROP_SOLID_MASK(addrl, startmask); \
		addrl++; \
	    } \
	    while (n--) \
	    { \
		RROP_SOLID(addrl); \
		++addrl; \
	    } \
	    if (endmask) \
		RROP_SOLID_MASK(addrl, endmask); \
	} \
    }

#define FILLSLICESPANS(flip,addr) \
    if (!flip) \
    { \
	FILLSPAN(xl, xr, addr); \
    } \
    else \
    { \
	xc = xorg - x; \
	FILLSPAN(xc, xr, addr); \
	xc += slw - 1; \
	FILLSPAN(xl, xc, addr); \
    }

static void
RROP_NAME(w32FillArcSliceSolid)(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    int yk, xk, ym, xm, dx, dy, xorg, yorg, slw;
    register int x, y, e;
    miFillArcRec info;
    miArcSliceRec slice;
    int xl, xr, xc;
    unsigned long *addrlt, *addrlb;
    register unsigned long *addrl;
    register int n;
    int nlwidth;
    RROP_DECLARE
    unsigned long startmask, endmask;

    cfbGetLongWidthAndPointer (pDraw, nlwidth, addrlt)
    addrlt = 0;
    RROP_FETCH_GC(pGC);
    miFillArcSetup(arc, &info);
    miFillArcSliceSetup(arc, &slice, pGC);
    MIFILLARCSETUP();
    xorg += pDraw->x;
    yorg += pDraw->y;
    addrlb = addrlt;
    addrlt += nlwidth * (yorg - y);
    addrlb += nlwidth * (yorg + y + dy);
    slice.edge1.x += pDraw->x;
    slice.edge2.x += pDraw->x;
    while (y > 0)
    {
	addrlt += nlwidth;
	addrlb -= nlwidth;
	MIFILLARCSTEP(slw);
	MIARCSLICESTEP(slice.edge1);
	MIARCSLICESTEP(slice.edge2);
	if (miFillSliceUpper(slice))
	{
	    MIARCSLICEUPPER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_top, addrlt);
	}
	if (miFillSliceLower(slice))
	{
	    MIARCSLICELOWER(xl, xr, slice, slw);
	    FILLSLICESPANS(slice.flip_bot, addrlb);
	}
    }
}

void
RROP_NAME(W32PolyFillArcSolid) (pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register xArc *arc;
    register int i;
    BoxRec box;
    RegionPtr cclip;
    unsigned long *addrlt;
    int nlwidth;

    CHECK_NOOP

    cfbGetLongWidthAndPointer (pDraw, nlwidth, addrlt)
    if ((CARD32)addrlt != VGABASE)
    {
	RROP_NAME(cfbPolyFillArcSolid) (pDraw, pGC, narcs, parcs);
	return;
    }

    cclip = cfbGetCompositeClip(pGC);

    for (arc = parcs, i = narcs; --i >= 0; arc++)
    {
	if (miFillArcEmpty(arc))
	    continue;
	if (miCanFillArc(arc))
	{
	    box.x1 = arc->x + pDraw->x;
	    box.y1 = arc->y + pDraw->y;
	    box.x2 = box.x1 + (int)arc->width + 1;
	    box.y2 = box.y1 + (int)arc->height + 1;
	    if (RECT_IN_REGION(pDraw->pScreen, cclip, &box) == rgnIN)
	    {
		if ((arc->angle2 >= FULLCIRCLE) ||
		    (arc->angle2 <= -FULLCIRCLE))
		    RROP_NAME(w32FillEllipseSolid)(pDraw, pGC, arc);
		else
		    RROP_NAME(w32FillArcSliceSolid)(pDraw, pGC, arc);
		continue;
	    }
	}
	miPolyFillArc(pDraw, pGC, 1, arc);
    }
}
