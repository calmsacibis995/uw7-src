/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/p9000/p9000gc32.c,v 3.1 1995/06/08 06:26:35 dawes Exp $ */
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
Copyright 1994 by Erik Nygren <nygren@mit.edu>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

HENRIK HARMSEN, CHRIS MASON, ERIK NYGREN, AND DIGITAL DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL DIGITAL,
CHRIS MASON, OR ERIK NYGREN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

Support for P9000 added by Erik Nygren <nygren@mit.edu>
Additional P9000 work by Chris Mason <mason@mail.csh.rit.edu>
Modified for P9000 32 bit GC by Henrik Harmsen <harmsen@eritel.se>

******************************************************************/
/* $XConsortium: p9000gc32.c /main/3 1995/11/12 18:19:04 kaleb $ */

#define PSZ 32

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "cfb.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"

#include "mistruct.h"
#include "mibstore.h"
#include "migc.h"

#include "cfbmskbits.h"
#include "cfb8bit.h"

#include "p9000.h"
#include "p9000reg.h"
#include "cfb16.h"
#include "cfb32.h"

#ifdef P9000_ACCEL

#if PSZ == 8
# define useTEGlyphBlt  cfbTEGlyphBlt8
#else
# ifdef WriteBitGroup
#  define useTEGlyphBlt	cfbImageGlyphBlt8
# else
#  define useTEGlyphBlt	cfbTEGlyphBlt
# endif
#endif

#ifdef WriteBitGroup
# define useImageGlyphBlt	cfbImageGlyphBlt8
# define usePolyGlyphBlt	cfbPolyGlyphBlt8
#else
# define useImageGlyphBlt	miImageGlyphBlt
# define usePolyGlyphBlt	miPolyGlyphBlt
#endif

#ifdef FOUR_BIT_CODE
# define usePushPixels	cfbPushPixels8
#else
# define usePushPixels	mfbPushPixels
#endif

#ifdef PIXEL_ADDR
# define ZeroPolyArc	cfbZeroPolyArcSS8Copy
#else
# define ZeroPolyArc	miZeroPolyArc
#endif


static GCFuncs p9000GCFuncs = {
    p9000ValidateGC32,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

static GCOps	p9000TEOps1Rect = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    p9000CopyArea32,
    cfb32CopyPlane,
    cfb32PolyPoint,
#ifdef PIXEL_ADDR
    cfb32LineSS1Rect,
    cfb32SegmentSS1Rect,
#else
    cfb32LineSS,
    cfb32SegmentSS,
#endif
    miPolyRectangle,
    cfb32ZeroPolyArcSSCopy,
    cfb32FillPoly1RectCopy,
    cfb32PolyFillRect,
    cfb32PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb32ImageGlyphBlt8,
    cfb32PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	p9000NonTEOps1Rect = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    p9000CopyArea32,
    cfb32CopyPlane,
    cfb32PolyPoint,
#ifdef PIXEL_ADDR
    cfb32LineSS1Rect,
    cfb32SegmentSS1Rect,
#else
    cfb32LineSS,
    cfb32SegmentSS,
#endif
    miPolyRectangle,
    cfb32ZeroPolyArcSSCopy,
    cfb32FillPoly1RectCopy,
    cfb32PolyFillRect,
    cfb32PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb32ImageGlyphBlt8,
    cfb32PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	p9000TEOps = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    p9000CopyArea32,
    cfb32CopyPlane,
    cfb32PolyPoint,
    cfb32LineSS,
    cfb32SegmentSS,
    miPolyRectangle,
    cfb32ZeroPolyArcSSCopy,
    miFillPolygon,
    cfb32PolyFillRect,
    cfb32PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb32ImageGlyphBlt8,
    cfb32PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	p9000NonTEOps = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    p9000CopyArea32,
    cfb32CopyPlane,
    cfb32PolyPoint,
    cfb32LineSS,
    cfb32SegmentSS,
    miPolyRectangle,
#ifdef PIXEL_ADDR
    cfb32ZeroPolyArcSSCopy,
#else
    miZeroPolyArc,
#endif
    miFillPolygon,
    cfb32PolyFillRect,
    cfb32PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb32ImageGlyphBlt8,
    cfb32PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};


#if 0
/*
 * p9000InitGC --
 *    Performs initialization of private structures.
 */
void
p9000InitGC()
{
  /* Initialize ALU->MINTERM mappings for raster operations */
  p9000alu[GXclear] = 0;	                      /* 0 */
  p9000alu[GXand] = IGM_S_MASK & IGM_D_MASK;	      /* src AND dst */
  p9000alu[GXandReverse] = IGM_S_MASK & ~IGM_D_MASK;  /* src AND NOT dst */
  p9000alu[GXcopy] = IGM_S_MASK;		      /* src */
  p9000alu[GXandInverted] = ~IGM_S_MASK & IGM_D_MASK; /* NOT src AND dst */
  p9000alu[GXnoop] = IGM_D_MASK;		      /* dst */
  p9000alu[GXxor] = IGM_S_MASK ^ IGM_D_MASK;	      /* src XOR dst */
  p9000alu[GXor] = IGM_S_MASK | IGM_D_MASK;	      /* src OR dst */
  p9000alu[GXnor] = ~IGM_S_MASK & ~IGM_D_MASK;	      /* NOT src AND NOT dst */
  p9000alu[GXequiv] = ~IGM_S_MASK ^ IGM_D_MASK;	      /* NOT src XOR dst */
  p9000alu[GXinvert] = ~IGM_D_MASK;		      /* NOT dst */
  p9000alu[GXorReverse] = IGM_S_MASK | ~IGM_D_MASK;   /* src OR NOT dst */
  p9000alu[GXcopyInverted] = ~IGM_S_MASK;	      /* NOT src */
  p9000alu[GXorInverted] = ~IGM_S_MASK | IGM_D_MASK;  /* NOT src OR dst */
  p9000alu[GXnand] = ~IGM_S_MASK | ~IGM_D_MASK;	      /* NOT src OR NOT dst */
  p9000alu[GXset] = IGM_S_MASK | ~IGM_S_MASK;         /* 1 */

  p9000BytesPerPixel = p9000InfoRec.bitsPerPixel / 8;
}
#endif


static GCOps *
p9000MatchCommon (pGC, devPriv)
    GCPtr	    pGC;
    cfbPrivGCPtr    devPriv;
{
    if (pGC->lineWidth != 0)
	return 0;
    if (pGC->lineStyle != LineSolid)
	return 0;
    if (pGC->fillStyle != FillSolid)
	return 0;
    if (devPriv->rop != GXcopy)
	return 0;
    if (pGC->font &&
	FONTMAXBOUNDS(pGC->font,rightSideBearing) -
        FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32 &&
	FONTMINBOUNDS(pGC->font,characterWidth) >= 0)
    {
	if (TERMINALFONT(pGC->font)
#ifdef FOUR_BIT_CODE
	    && FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
	)
#ifdef NO_ONE_RECT
            return &p9000TEOps1Rect;
#else
	    if (devPriv->oneRect)
		return &p9000TEOps1Rect;
	    else
		return &p9000TEOps;
#endif
	else
#ifdef NO_ONE_RECT
	    return &p9000NonTEOps1Rect;
#else
	    if (devPriv->oneRect)
		return &p9000NonTEOps1Rect;
	    else
		return &p9000NonTEOps;
#endif
    }
    return 0;
}

Bool
p9000CreateGC32(pGC)
    register GCPtr pGC;
{
    cfbPrivGC  *pPriv;

    if (PixmapWidthPaddingInfo[pGC->depth].padPixelsLog2 == LOG2_BITMAP_PAD)
	return (mfbCreateGC(pGC));
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;

    /*
     * some of the output primitives aren't really necessary, since they
     * will be filled in ValidateGC because of dix/CreateGC() setting all
     * the change bits.  Others are necessary because although they depend
     * on being a color frame buffer, they don't change 
     */

    pGC->ops = &p9000NonTEOps;
    pGC->funcs = &p9000GCFuncs;

    /* cfb wants to translate before scan conversion */
    pGC->miTranslate = 1;

    pPriv = cfbGetGCPrivate(pGC);
    pPriv->rop = pGC->alu;
    pPriv->oneRect = FALSE;
    pPriv->fExpose = TRUE;
    pPriv->freeCompClip = FALSE;
    pPriv->pRotatedPixmap = (PixmapPtr) NULL;
    return TRUE;
}

/* Clipping conventions
	if the drawable is a window
	    CT_REGION ==> pCompositeClip really is the composite
	    CT_other ==> pCompositeClip is the window clip region
	if the drawable is a pixmap
	    CT_REGION ==> pCompositeClip is the translated client region
		clipped to the pixmap boundary
	    CT_other ==> pCompositeClip is the pixmap bounding box
*/


void
p9000ValidateGC32(pGC, changes, pDrawable)
    register GCPtr  pGC;
    unsigned long   changes;
    DrawablePtr	    pDrawable;
{
    int         mask;		/* stateChanges */
    int         index;		/* used for stepping through bitfields */
    int		new_rrop;
    int         new_line, new_text, new_fillspans, new_fillarea;
    int		new_rotate;
    int		xrot, yrot;
    /* flags for changing the proc vector */
    cfbPrivGCPtr devPriv;
    int		oneRect;

    new_rotate = pGC->lastWinOrg.x != pDrawable->x ||
		 pGC->lastWinOrg.y != pDrawable->y;

    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;
    devPriv = cfbGetGCPrivate(pGC);

    new_rrop = FALSE;
    new_line = FALSE;
    new_text = FALSE;
    new_fillspans = FALSE;
    new_fillarea = FALSE;

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip 
     */

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
	)
    {
	miComputeCompositeClip (pGC, pDrawable);
#ifdef NO_ONE_RECT
	devPriv->oneRect = FALSE;
#else
	oneRect = REGION_NUM_RECTS(devPriv->pCompositeClip) == 1;
	if (oneRect != devPriv->oneRect)
	    new_line = TRUE;
	devPriv->oneRect = oneRect;
#endif
    }

    mask = changes;
    while (mask) {
	index = lowbit (mask);
	mask &= ~index;

	/*
	 * this switch acculmulates a list of which procedures might have
	 * to change due to changes in the GC.  in some cases (e.g.
	 * changing one 16 bit tile for another) we might not really need
	 * a change, but the code is being paranoid. this sort of batching
	 * wins if, for example, the alu and the font have been changed,
	 * or any other pair of items that both change the same thing. 
	 */
	switch (index) {
	case GCFunction:
	case GCForeground:
	    new_rrop = TRUE;
	    break;
	case GCPlaneMask:
	    new_rrop = TRUE;
	    new_text = TRUE;
	    break;
	case GCBackground:
	    break;
	case GCLineStyle:
	case GCLineWidth:
	    new_line = TRUE;
	    break;
	case GCJoinStyle:
	case GCCapStyle:
	    break;
	case GCFillStyle:
	    new_text = TRUE;
	    new_fillspans = TRUE;
	    new_line = TRUE;
	    new_fillarea = TRUE;
	    break;
	case GCFillRule:
	    break;
	case GCTile:
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	    break;

	case GCStipple:
	    if (pGC->stipple)
	    {
		int width = pGC->stipple->drawable.width;
		PixmapPtr nstipple;

		if ((width <= PGSZ) && !(width & (width - 1)) &&
		    (nstipple = cfb32CopyPixmap(pGC->stipple)))
		{
		    cfb32PadPixmap(nstipple);
		    (*pGC->pScreen->DestroyPixmap)(pGC->stipple);
		    pGC->stipple = nstipple;
		}
	    }
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	    break;

	case GCTileStipXOrigin:
	    new_rotate = TRUE;
	    break;

	case GCTileStipYOrigin:
	    new_rotate = TRUE;
	    break;

	case GCFont:
	    new_text = TRUE;
	    break;
	case GCSubwindowMode:
	    break;
	case GCGraphicsExposures:
	    break;
	case GCClipXOrigin:
	    break;
	case GCClipYOrigin:
	    break;
	case GCClipMask:
	    break;
	case GCDashOffset:
	    break;
	case GCDashList:
	    break;
	case GCArcMode:
	    break;
	default:
	    break;
	}
    }

    /*
     * If the drawable has changed,  ensure suitable
     * entries are in the proc vector. 
     */
    if (pDrawable->serialNumber != (pGC->serialNumber & (DRAWABLE_SERIAL_BITS))) {
	new_fillspans = TRUE;	/* deal with FillSpans later */
    }

    if (new_rotate || new_fillspans)
    {
	Bool new_pix = FALSE;

	xrot = pGC->patOrg.x + pDrawable->x;
	yrot = pGC->patOrg.y + pDrawable->y;

	switch (pGC->fillStyle)
	{
	case FillTiled:
	    if (!pGC->tileIsPixel)
	    {
		int width = pGC->tile.pixmap->drawable.width * PSZ;

		if ((width <= PGSZ) && !(width & (width - 1)))
		{
		    cfb32CopyRotatePixmap(pGC->tile.pixmap,
					&devPriv->pRotatedPixmap,
					xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#ifdef FOUR_BIT_CODE
	case FillStippled:
	case FillOpaqueStippled:
	    {
		int width = pGC->stipple->drawable.width;

		if ((width <= PGSZ) && !(width & (width - 1)))
		{
		    mfbCopyRotatePixmap(pGC->stipple,
					&devPriv->pRotatedPixmap, xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#endif
	}
	if (!new_pix && devPriv->pRotatedPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(devPriv->pRotatedPixmap);
	    devPriv->pRotatedPixmap = (PixmapPtr) NULL;
	}
    }

    if (new_rrop)
    {
	int old_rrop;

	old_rrop = devPriv->rop;
	devPriv->rop = cfb32ReduceRasterOp (pGC->alu, pGC->fgPixel,
					   pGC->planemask,
					   &devPriv->and, &devPriv->xor);
	if (old_rrop == devPriv->rop)
	    new_rrop = FALSE;
	else
	{
#ifdef PIXEL_ADDR
	    new_line = TRUE;
#endif
#ifdef WriteBitGroup
	    new_text = TRUE;
#endif
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	}
    }

    if (new_rrop || new_fillspans || new_text || new_fillarea || new_line)
    {
	GCOps	*newops;

	if (newops = p9000MatchCommon (pGC, devPriv))
 	{
	    if (pGC->ops->devPrivate.val)
		miDestroyGCOps (pGC->ops);
	    pGC->ops = newops;
	    new_rrop = new_line = new_fillspans = new_text = new_fillarea = 0;
	}
 	else
 	{
	    if (!pGC->ops->devPrivate.val)
	    {
		pGC->ops = miCreateGCOps (pGC->ops);
		pGC->ops->devPrivate.val = 1;
	    }
	}
    }

    /* deal with the changes we've collected */
    if (new_line)
    {
	pGC->ops->FillPolygon = miFillPolygon;
#ifdef NO_ONE_RECT
	if (pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillPolygon = cfb32FillPoly1RectCopy;
		break;
	    default:
		pGC->ops->FillPolygon = cfb32FillPoly1RectGeneral;
		break;
	    }
	}
#else
	if (devPriv->oneRect && pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillPolygon = cfb32FillPoly1RectCopy;
		break;
	    default:
		pGC->ops->FillPolygon = cfb32FillPoly1RectGeneral;
		break;
	    }
	}
#endif
	if (pGC->lineWidth == 0)
	{
#ifdef PIXEL_ADDR
	    if ((pGC->lineStyle == LineSolid) && (pGC->fillStyle == FillSolid))
	    {
		switch (devPriv->rop)
		{
		case GXxor:
		    pGC->ops->PolyArc = cfb32ZeroPolyArcSSXor;
		    break;
		case GXcopy:
		    pGC->ops->PolyArc = cfb32ZeroPolyArcSSCopy;
		    break;
		default:
		    pGC->ops->PolyArc = cfb32ZeroPolyArcSSGeneral;
		    break;
		}
	    }
	    else
#endif
		pGC->ops->PolyArc = miZeroPolyArc;
	}
	else
	    pGC->ops->PolyArc = miPolyArc;
	pGC->ops->PolySegment = miPolySegment;
	switch (pGC->lineStyle)
	{
	case LineSolid:
	    if(pGC->lineWidth == 0)
	    {
		if (pGC->fillStyle == FillSolid)
		{
#if defined(PIXEL_ADDR) && !defined(NO_ONE_RECT)
		    if (devPriv->oneRect &&
			((pDrawable->x >= pGC->pScreen->width - 32768) &&
			 (pDrawable->y >= pGC->pScreen->height - 32768)))
		    {
			pGC->ops->Polylines = cfb32LineSS1Rect;
			pGC->ops->PolySegment = cfb32SegmentSS1Rect;
		    } else
#endif
#ifdef NO_ONE_RECT
		    {
			pGC->ops->Polylines = cfb32LineSS1Rect;
			pGC->ops->PolySegment = cfb32SegmentSS1Rect;
		    }
#else
		    {
		    	pGC->ops->Polylines = cfb32LineSS;
		    	pGC->ops->PolySegment = cfb32SegmentSS;
		    }
#endif
		}
 		else
		    pGC->ops->Polylines = miZeroLine;
	    }
	    else
		pGC->ops->Polylines = miWideLine;
	    break;
	case LineOnOffDash:
	case LineDoubleDash:
	    if (pGC->lineWidth == 0 && pGC->fillStyle == FillSolid)
	    {
		pGC->ops->Polylines = cfb32LineSD;
		pGC->ops->PolySegment = cfb32SegmentSD;
	    } else
		pGC->ops->Polylines = miWideDash;
	    break;
	}
    }

    if (new_text && (pGC->font))
    {
        if (FONTMAXBOUNDS(pGC->font,rightSideBearing) -
            FONTMINBOUNDS(pGC->font,leftSideBearing) > 32 ||
	    FONTMINBOUNDS(pGC->font,characterWidth) < 0)
        {
            pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
        }
        else
        {
#ifdef WriteBitGroup
	    if (pGC->fillStyle == FillSolid)
	    {
		if (devPriv->rop == GXcopy)
		    pGC->ops->PolyGlyphBlt = cfb32PolyGlyphBlt8;
		else
#ifdef FOUR_BIT_CODE
		    pGC->ops->PolyGlyphBlt = cfb32PolyGlyphRop8;
#else
		    pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
#endif
	    }
	    else
#endif
		pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            /* special case ImageGlyphBlt for terminal emulator fonts */
#if !defined(WriteBitGroup) || PSZ == 8
	    if (TERMINALFONT(pGC->font) &&
		(pGC->planemask & PMSK) == PMSK
#ifdef FOUR_BIT_CODE
		&& FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
#endif
		)
	    {
		pGC->ops->ImageGlyphBlt = useTEGlyphBlt;
	    }
            else
#endif
	    {
#ifdef WriteBitGroup
		if (devPriv->rop == GXcopy &&
		    pGC->fillStyle == FillSolid &&
		    (pGC->planemask & PMSK) == PMSK)
		    pGC->ops->ImageGlyphBlt = cfb32ImageGlyphBlt8;
		else
#endif
		    pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
	    }
        }
    }    


    if (new_fillspans) {
	switch (pGC->fillStyle) {
	case FillSolid:
	    switch (devPriv->rop) {
	    case GXcopy:
		pGC->ops->FillSpans = cfb32SolidSpansCopy;
		break;
	    case GXxor:
		pGC->ops->FillSpans = cfb32SolidSpansXor;
		break;
	    default:
		pGC->ops->FillSpans = cfb32SolidSpansGeneral;
		break;
	    }
	    break;
	case FillTiled:
	    if (devPriv->pRotatedPixmap)
	    {
		if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
		    pGC->ops->FillSpans = cfb32Tile32FSCopy;
		else
		    pGC->ops->FillSpans = cfb32Tile32FSGeneral;
	    }
	    else
		pGC->ops->FillSpans = cfb32UnnaturalTileFS;
	    break;
	case FillStippled:
#ifdef FOUR_BIT_CODE
	    if (devPriv->pRotatedPixmap)
		pGC->ops->FillSpans = cfb32Stipple32FS;
	    else
#endif
		pGC->ops->FillSpans = cfb32UnnaturalStippleFS;
	    break;
	case FillOpaqueStippled:
#ifdef FOUR_BIT_CODE
	    if (devPriv->pRotatedPixmap)
		pGC->ops->FillSpans = cfb32paqueStipple32FS;
	    else
#endif
		pGC->ops->FillSpans = cfb32UnnaturalStippleFS;
	    break;
	default:
	    FatalError("p9000ValidateGC32: illegal fillStyle\n");
	}
    } /* end of new_fillspans */

    if (new_fillarea) {
#ifndef FOUR_BIT_CODE
	pGC->ops->PolyFillRect = miPolyFillRect;
	if (pGC->fillStyle == FillSolid)
	{
	    pGC->ops->PolyFillRect = cfb32PolyFillRect;
	}
	if (pGC->fillStyle == FillTiled)
	{
	    pGC->ops->PolyFillRect = cfb32PolyFillRect;
	}
#endif
#ifdef FOUR_BIT_CODE
	pGC->ops->PushPixels = mfbPushPixels;
	if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
	    pGC->ops->PushPixels = cfb32PushPixels8;
#endif
	pGC->ops->PolyFillArc = miPolyFillArc;
	if (pGC->fillStyle == FillSolid)
	{
	    switch (devPriv->rop)
	    {
	    case GXcopy:
		pGC->ops->PolyFillArc = cfb32PolyFillArcSolidCopy;
		break;
	    default:
		pGC->ops->PolyFillArc = cfb32PolyFillArcSolidGeneral;
		break;
	    }
	}
    }
}

#endif /* P9000_ACCEL */
