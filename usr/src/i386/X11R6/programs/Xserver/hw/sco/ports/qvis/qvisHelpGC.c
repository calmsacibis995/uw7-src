
/**
 *	@(#) qvisHelpGC.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */
/**
 * HERE BE THE ROPE BY WHICH TO HANG YOURSELF!
 *
 * Here is were the real validation of the GC functions takes place.
 * This source is provided for developers who ABSOLUTELY must replace some
 * NFB routines.  If you replace this routine your driver may not be
 * binary compatible with the next release of the X Server from SCO.
 * Furthermore, if this module is replaced you will be unable to take
 * advantage of any future optimization to NFB.
 *
 * Replacement of this module is not supported by the SCO X Server Link Kit.
 *
 */
/*
 *      SCO MODIFICATION HISTORY
 *
 *      S000    Tue Jul 13 12:34:17 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 */
/**
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 * waltc     06/07/93  Remove qvisHelpValidateGC, cfb.h
 */

/*
 * This code is (obviously) based on the nfbHelpGC.c found in the NFB
 * directory.  It is slightly modified here to support the HYPER_LINK_KIT
 * support. -mjk
 *
 * XXX We should track changes to nfbHelpGC.c to make sure this code
 * is kept up to date.  Hopefully, nfbHelpGC is quite stable since it
 * is pretty generic code.
 */

#include "xyz.h"
#include "X.h"
#include "Xproto.h"
#include "pixmapstr.h"
#include "gcstruct.h"

#include "mi/mi.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbProcs.h"

#include "qvisMacros.h"
#include "qvisDefs.h"
#include "qvisProcs.h"

extern void     miZeroDashLine();

/* generic masks from nfbHelpGC.c */
#define LINE_WIDTH_ZERO (1<<0)
#define LINE_SOLID_MASK (1<<1)
#define FILL_SOLID_MASK (1<<2)

/* Q-Vision specific mask */
#define FLAT_MODE_MASK (1<<3)

#define GEN_GC_OPS_CACHE_SIZE 16

static GCOps   *qvisGCOpsCache[GEN_GC_OPS_CACHE_SIZE];

static GCOps    qvisCommonOps =
{
    nfbFillSpans,		/* void (* FillSpans)() */
    nfbSetSpans,		/* void (* SetSpans)()  */
    miPutImage,			/* void (* PutImage)()  */
    nfbCopyArea,		/* RegionPtr (* CopyArea)() */
    nfbCopyPlane,		/* RegionPtr (* CopyPlane)() */
    nfbPolyPoint,		/* void (* PolyPoint)() */
    miWideLine,			/* void (* Polylines)() */
    miPolySegment,		/* void (* PolySegment)() */
    miPolyRectangle,		/* void (* PolyRectangle)() */
    miPolyArc,			/* void (* PolyArc)()   */
    miFillPolygon,		/* void (* FillPolygon)() */
    nfbPolyFillRect,		/* void (* PolyFillRect)() */
    miPolyFillArc,		/* void (* PolyFillArc)() */
    miPolyText8,		/* int (* PolyText8)()  */
    miPolyText16,		/* int (* PolyText16)() */
    miImageText8,		/* void (* ImageText8)() */
    miImageText16,		/* void (* ImageText16)() */
    nfbImageGlyphBlt,		/* void (* ImageGlyphBlt)() */
    miPolyGlyphBlt,		/* void (* PolyGlyphBlt)() */
    nfbPushPixels,		/* void (* PushPixels)() */
    miMiter,			/* void (* LineHelper)() */
/* DevUnion devPrivate */
};

/**
 * hyper_link_kit:
 *  TRUE if we want to violate SCO Link Kit spec for more performance.
 *  FALSE if we want to be compatible
 *
 * This can be changed by the HYPER_LINK_KIT flag in the DATA section
 * of the Q-Vision grafinfo file.
 */
Bool            hyper_link_kit = TRUE;

void
qvisGCOpsCacheInit()
{
    register int    i;

    XYZ("qvisGCOpsCacheInit-entered");
    for (i = 1; i < GEN_GC_OPS_CACHE_SIZE; i++)
	qvisGCOpsCache[i] = (GCOps *) 0;
    /*
     * Initialize the 1st cache element to point to qvisCommonOps. Note that
     * qvisCommonOps.devPrivate.val = 0.
     */
    qvisGCOpsCache[0] = &qvisCommonOps;
}

void
qvisGCOpsCacheReset()
{
    register int    i;

    XYZ("qvisGCOpsCacheReset-entered");
    for (i = 1; i < GEN_GC_OPS_CACHE_SIZE; i++) {
	if (qvisGCOpsCache[i])
	    xfree(qvisGCOpsCache[i]);
    }
}

static int
qvisGetGCOpsCacheIndex(pGC)
    register GCPtr  pGC;
{
    int             i = 0;

    XYZ("qvisGetGCOpsCacheIndex-entered");
    if (pGC->lineWidth == 0)
	i |= LINE_WIDTH_ZERO;
    if (pGC->lineStyle == LineSolid)
	i |= LINE_SOLID_MASK;
    if (pGC->fillStyle == FillSolid)
	i |= FILL_SOLID_MASK;

    return i;
}

static GCOps   *
qvisNewGCOps(pGC, indx, flat)
    register GCPtr  pGC;
    int             indx;
    Bool            flat;
{
    GCOps          *ops = (GCOps *) xalloc(sizeof(GCOps));

    XYZ("qvisNewGCOps-entered");
    /* XXX is this the right response? */
    if (ops == NULL) {
	XYZ("qvisNewGCOps-CouldntAllocateGCOps");
	return qvisGCOpsCache[0];/* default; should work */
    }
    *ops = qvisCommonOps;

    if (pGC->lineWidth == 0) {
	XYZ("qvisNewGCOps-lineWidth==0");
	if (pGC->lineStyle == LineSolid) {
	    if (pGC->fillStyle == FillSolid) {
		XYZ("qvisNewGCOps-installed-nfbLineSS");
		ops->Polylines = nfbLineSS;
		XYZ("qvisNewGCOps-installed-nfbSegmentSS");
		ops->PolySegment = nfbSegmentSS;
		XYZ("qvisNewGCOps-installed-nfbZeroPolyArc");
		ops->PolyArc = nfbZeroPolyArc;
	    } else {
		XYZ("qvisNewGCOps-installed-miZeroLine");
		ops->Polylines = miZeroLine;
		XYZ("qvisNewGCOps-installed-miZeroPolyArc");
		ops->PolyArc = miZeroPolyArc;
	    }
	} else {
	    if (pGC->fillStyle == FillSolid) {
		XYZ("qvisNewGCOps-installed-nfbLineSD");
		ops->Polylines = nfbLineSD;
	    } else {
		XYZ("qvisNewGCOps-installed-miZeroDashLine");
		ops->Polylines = miZeroDashLine;
	    }
	    XYZ("qvisNewGCOps-installed-miZeroPolyArc");
	    ops->PolyArc = miZeroPolyArc;
	}
    } else {			/* wide lines */
	XYZ("qvisNewGCOps-lineWidth>0");
	if (pGC->lineStyle != LineSolid) {
	    XYZ("qvisNewGCOps-installed-miWideDash");
	    ops->Polylines = miWideDash;
	}
    }
    if (pGC->fillStyle == FillSolid) {
	XYZ("qvisNewGCOps-installing-nfbSolidPolyGlyphBlt");
	ops->PolyGlyphBlt = nfbSolidPolyGlyphBlt;
    }
    if (flat && hyper_link_kit) {
	/*
	 * Only install qvisPolyPoint if in flat mode (not written for banked)
	 * and don't installed if hyper_link_kit is false which means we
	 * don't want to install routines that violate the SCO Link Kit spec
	 * (qvisPolyPoint does).
	 */
	XYZ("qvisNewGCOps-installing-qvisPolyPoint");
	ops->PolyPoint = qvisPolyPoint;
    }
    ops->devPrivate.val = 0;
    qvisGCOpsCache[indx] = ops;
    return ops;
}
