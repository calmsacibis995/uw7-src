/*
 * @(#) m32GC.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 30-Aug-93, buckm
 *	Setup our own GC ops; no more nfbHelpValidateGC.
 *	Use our own thin solid and dashed line functions.
 *	No more MiscOps or PtToPt lines.
 *	Use our own oneRect versions of PolyPoints and FillSpans.
 */
/*
 * m32GC.c
 *
 * Mach-32 ValidateWindowGC() routine
 */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "colormapst.h"
#include "pixmapstr.h"
#include "gcstruct.h"

#include "mi/mi.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbGlyph.h"

#ifdef agaII
#define	nfbPolyTEText8	nfbPolyTEGlyphBlt8Clipped
#define	nfbImageTEText8	nfbImageTEGlyphBlt8Clipped
#endif
#include "nfb/nfbProcs.h"

#include "m32Procs.h"


extern nfbGCOps m32SolidPrivOps, m32TiledPrivOps,
		m32StippledPrivOps, m32OpStippledPrivOps;

extern void mfbPushPixels();


typedef union _m32GCOpsCacheIndex {
	int index;
	struct {
		unsigned int	fillStyle	: 2;
		unsigned int	lineSolid	: 1;
		unsigned int	lineZero	: 1;
		unsigned int	fontText8	: 1;
		unsigned int	oneRect		: 1;
		unsigned int	unused		: 26;
	} bits;
} m32GCOpsCacheIndex;

#define M32_GC_OPS_CACHE_SIZE (1<<6)	/* MUST MATCH used bits above */

static GCOps *m32GCOpsCache[M32_GC_OPS_CACHE_SIZE];

static GCOps m32CommonOps = {
        nfbFillSpans,           /*  void (* FillSpans)() */
        nfbSetSpans,            /*  void (* SetSpans)()  */
        nfbPutImage,            /*  void (* PutImage)()  */
        nfbCopyArea,            /*  RegionPtr (* CopyArea)()     */
        nfbCopyPlane,           /*  RegionPtr (* CopyPlane)() */
        nfbPolyPoint,           /*  void (* PolyPoint)() */
        miWideLine,             /*  void (* Polylines)() */
        miPolySegment,          /*  void (* PolySegment)() */
        miPolyRectangle,        /*  void (* PolyRectangle)() */
        miPolyArc,              /*  void (* PolyArc)()   */
        miFillPolygon,          /*  void (* FillPolygon)() */
        nfbPolyFillRect,        /*  void (* PolyFillRect)() */
        miPolyFillArc,          /*  void (* PolyFillArc)() */
        miPolyText8,            /*  int (* PolyText8)()  */
        miPolyText16,           /*  int (* PolyText16)() */
        miImageText8,           /*  void (* ImageText8)() */
        miImageText16,          /*  void (* ImageText16)() */
        nfbImageGlyphBlt,       /*  void (* ImageGlyphBlt)() */
        miPolyGlyphBlt,         /*  void (* PolyGlyphBlt)() */
        nfbPushPixels,          /*  void (* PushPixels)() */
        miMiter,                /*  void (* LineHelper)() */
                                /*  DevUnion devPrivate */
};

void
m32GCOpsCacheInit()
{
	int i;

	for (i = 0; i < M32_GC_OPS_CACHE_SIZE; ++i)
		m32GCOpsCache[i] = (GCOps *)0;
}

void
m32GCOpsCacheReset()
{
	int i;

	for (i = 0; i < M32_GC_OPS_CACHE_SIZE; ++i)
		if (m32GCOpsCache[i]) {
			xfree(m32GCOpsCache[i]);
			m32GCOpsCache[i] = (GCOps *)0;
		}
}

static int
m32GetGCOpsCacheIndex(pGC)
	GCPtr pGC;
{
#ifdef agaII
	nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV(pGC->pScreen);
#endif
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	m32GCOpsCacheIndex ci;

	ci.index = 0;

	ci.bits.fillStyle = pGC->fillStyle;
	if (pGC->lineStyle == LineSolid)
		ci.bits.lineSolid = 1;
	if (pGC->lineWidth == 0)
		ci.bits.lineZero = 1;
#ifdef agaII
	if (pGC->font && NFB_FONT_PS(pGC->font, nfbPriv)->private.val >= 0)
#else
	if (pGC->font && NFB_FONT_PRIV(pGC->font)->fontInfo.isTE8Font)
#endif
		ci.bits.fontText8 = 1;
	if (REGION_NUM_RECTS(pGCPriv->pCompositeClip) == 1)
		ci.bits.oneRect = 1;

	return ci.index;
}

static GCOps *
m32NewGCOps(index)
	int index;
{
	m32GCOpsCacheIndex ci;
	GCOps *ops = (GCOps *)xalloc(sizeof(GCOps));

	*ops = m32CommonOps;
	ci.index = index;

	if (ci.bits.lineZero) {
		if (ci.bits.lineSolid) {
			if (ci.bits.fillStyle == FillSolid) {
				if (ci.bits.oneRect) {
					ops->Polylines	   = m32LineSS;
					ops->PolySegment   = m32SegmentSS;
					ops->PolyRectangle = m32RectangleSS;
				} else {
					ops->Polylines	   = nfbLineSS;
					ops->PolySegment   = nfbSegmentSS;
					ops->PolyRectangle = nfbPolyRectangle;
				}
				ops->PolyArc	   = nfbZeroPolyArc;
			} else {
				ops->Polylines	   = miZeroLine;
				ops->PolyRectangle = nfbPolyRectangle;
				ops->PolyArc	   = miZeroPolyArc;
			}
		} else {
			if (ci.bits.fillStyle == FillSolid) {
				if (ci.bits.oneRect) {
					ops->Polylines	   = m32LineSD;
					ops->PolySegment   = m32SegmentSD;
					ops->PolyRectangle = m32RectangleSD;
				} else {
					ops->Polylines   = nfbLineSD;
					ops->PolySegment = nfbSegmentSD;
				}
			}
			ops->PolyArc = miZeroPolyArc;
		}
	} else {	/* wide lines */
		if (!ci.bits.lineSolid)
			ops->Polylines = miWideDash;
	}

	if (ci.bits.fillStyle == FillSolid) {
		ops->PolyGlyphBlt = nfbSolidPolyGlyphBlt;
		if (ci.bits.fontText8)
			ops->PolyText8 = nfbPolyTEText8;
	}
	if (ci.bits.fontText8)
		ops->ImageText8 = nfbImageTEText8;

	if (ci.bits.fillStyle == FillTiled)
		ops->PushPixels = mfbPushPixels;

	if (ci.bits.oneRect) {
		if (ci.bits.fillStyle == FillSolid)
			ops->FillSpans = m32FillSpans;
		ops->PolyPoint = m32PolyPoint;
	}

	ops->devPrivate.val = 0;
	m32GCOpsCache[index] = ops;
	return ops;
}


#define M32_GCOP_CHANGES \
	(GCFillStyle|GCLineStyle|GCLineWidth|GCFont| \
	 GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)

#define	CHECK_SERIAL(pDraw, pGC) \
	(pDraw->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))

/*
 * m32ValidateWindowGC() - set GC ops and privates based on GC values.
 */
void
m32ValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pGCPriv->ops = &m32SolidPrivOps;
			break;
		    case FillTiled:
			pGCPriv->ops = &m32TiledPrivOps;
			break;
		    case FillStippled:
			pGCPriv->ops = &m32StippledPrivOps;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pGCPriv->ops = &m32SolidPrivOps;
			else
			    pGCPriv->ops = &m32OpStippledPrivOps;
		}

	/* set the ops */
	if ((changes & M32_GCOP_CHANGES) || CHECK_SERIAL(pDraw, pGC)) {
		int opsindex;
		GCOps *ops;

		opsindex = m32GetGCOpsCacheIndex(pGC);
		if ((ops = m32GCOpsCache[opsindex]) == (GCOps *)0)
			ops = m32NewGCOps(opsindex);
		pGC->ops = ops;
	}

	/* set the screen relative patorg */
	pGCPriv->screenPatOrg.x = pDraw->x + pGC->patOrg.x;
	pGCPriv->screenPatOrg.y = pDraw->y + pGC->patOrg.y;
}
