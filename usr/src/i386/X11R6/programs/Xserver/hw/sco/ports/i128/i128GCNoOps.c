/*
 *	@(#) i128GCNoOps.c 11.1 97/10/22
 *
 *      Copyright (C) The Santa Cruz Operation, 1991-1994.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 *
 * Modification History
 *
 * S000, 9-Jun-95, kylec@sco.com
 * 	created
 */

/*
 * i128GCNoOps.c
 *
 * GCOps wrappers
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "i128Defs.h"
#include "i128Procs.h"

#ifdef I128_FAST_GC_OPS

static void _NoopFillSpans();
static void _NoopSetSpans();
static void _NoopPutImage();
static RegionPtr _NoopCopyArea();
static RegionPtr _NoopCopyPlane();
static void _NoopPolyPoint();
static void _NoopPolylines();
static void _NoopPolySegment();
static void _NoopPolyRectangle();
static void _NoopPolyArc();
static void _NoopFillPolygon();
static void _NoopPolyFillRect();
static void _NoopPolyFillArc();
static int _NoopPolyText8();
static int _NoopPolyText16();
static void _NoopImageText8();
static void _NoopImageText16();
static void _NoopImageGlyphBlt();
static void _NoopPolyGlyphBlt();
static void _NoopPushPixels();
static void _NoopStub();

GCOps i128GCNoOps = {
     _NoopFillSpans,      /*  void (* FillSpans)() */
     _NoopSetSpans,       /*  void (* SetSpans)()  */
     _NoopPutImage,       /*  void (* PutImage)()  */
     _NoopCopyArea,       /*  RegionPtr (* CopyArea)()     */
     _NoopCopyPlane,      /*  RegionPtr (* CopyPlane)() */
     _NoopPolyPoint,      /*  void (* PolyPoint)() */
     _NoopPolylines,      /*  void (* Polylines)() */
     _NoopPolySegment,    /*  void (* PolySegment)() */
     _NoopPolyRectangle,  /*  void (* PolyRectangle)() */
     _NoopPolyArc,        /*  void (* PolyArc)()   */
     _NoopFillPolygon,    /*  void (* FillPolygon)() */
     _NoopPolyFillRect,   /*  void (* PolyFillRect)() */
     _NoopPolyFillArc,    /*  void (* PolyFillArc)() */
     _NoopPolyText8,      /*  int (* PolyText8)()  */
     _NoopPolyText16,     /*  int (* PolyText16)() */
     _NoopImageText8,     /*  void (* ImageText8)() */
     _NoopImageText16,    /*  void (* ImageText16)() */
     _NoopImageGlyphBlt,  /*  void (* ImageGlyphBlt)() */
     _NoopPolyGlyphBlt,   /*  void (* PolyGlyphBlt)() */
     _NoopPushPixels,     /*  void (* PushPixels)() */
     _NoopStub,           /*  void (* Stub)() */
};

extern
GCFuncs i128GCFuncs;

static void
_NoopFillSpans(pDraw, pGC, nSpans, pPoints, pWidths, fSorted)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nSpans;
    DDXPointPtr pPoints;
    unsigned int *pWidths;
    int fSorted;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->FillSpans)(pDraw, pGC, nSpans,
                           pPoints, pWidths, fSorted);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}



static void
_NoopSetSpans(pDraw, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int *pSrc;
    DDXPointPtr pPoints;
    unsigned int *pWidths;
    unsigned int nSpans;
    int fSorted;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->SetSpans)(pDraw, pGC, pSrc, pPoints,
                          pWidths, nSpans, fSorted);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
_NoopPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr pDraw;
    GCPtr pGC;
    int depth;
    int x, y, w, h;
    int leftPad;
    unsigned int format;
    unsigned char *pImage;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PutImage)(pDraw, pGC, depth, x, y, w, h, leftPad,
                          format, pImage);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static RegionPtr
_NoopCopyArea(pSrc, pDst, pGC, srcx, srcy, width, height, dstx, dsty)
    DrawablePtr pSrc, pDst;
    GCPtr pGC;
    int srcx, srcy;
    unsigned int width, height;
    int dstx, dsty;
{
    GCOps *ops;
    RegionPtr pRegion;

    I128_UNWRAP_GC(pGC, ops);
    pRegion = (*pGC->ops->CopyArea)(pSrc, pDst, pGC, srcx, srcy,
                                    width, height, dstx, dsty);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    return pRegion;
}


static RegionPtr
_NoopCopyPlane(pSrc, pDst, pGC, srcx, srcy,
                     width, height, dstx, dsty, plane)
    DrawablePtr pSrc, pDst;
    GCPtr pGC;
    int srcx, srcy, width, height, dstx, dsty;
    unsigned long plane;
{
    GCOps *ops;
    RegionPtr pRegion;

    I128_UNWRAP_GC(pGC, ops);
    pRegion = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, srcx, srcy,
                                     width, height, dstx, dsty, plane);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    return pRegion;
}


static void
_NoopPolyPoint(pDraw, pGC, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int mode;
    int nPoints;
    register xPoint *pPoints;
{
    GCOps *ops;
    
    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyPoint)(pDraw, pGC, mode, nPoints, pPoints);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
_NoopPolylines(pDraw, pGC, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int mode;
    register int nPoints;
    register DDXPointPtr pPoints;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->Polylines)(pDraw, pGC, mode, nPoints, pPoints);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopPolySegment(pDraw, pGC, nSegments, pSegments)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nSegments;
    xSegment *pSegments;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolySegment)(pDraw, pGC, nSegments, pSegments);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopPolyRectangle(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nRects;
    xRectangle *pRects;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyRectangle)(pDraw, pGC, nRects, pRects);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}



static void
_NoopPolyArc(pDraw, pGC, nArcs, pArcs)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nArcs;
    xArc *pArcs;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyArc)(pDraw, pGC, nArcs, pArcs);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}



static void
_NoopFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int shape;
    int mode;
    int nPoints;
    DDXPointPtr pPoints;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, nPoints, pPoints);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopPolyFillRect(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nRects;
    xRectangle *pRects;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
_NoopPolyFillArc(pDraw, pGC, nArcs, pArcs)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nArcs;
    xArc *pArcs;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyFillArc)(pDraw, pGC, nArcs, pArcs);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static int
_NoopPolyText8(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    char *string;
{
    GCOps *ops;
    int ret;

    I128_UNWRAP_GC(pGC, ops);
    ret = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, string);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    return ret;
    
}


static int
_NoopPolyText16(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    unsigned short *string;
{
    GCOps *ops;
    int ret;

    I128_UNWRAP_GC(pGC, ops);
    ret = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, string);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    return ret;
    
}


static void
_NoopImageText8(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    char *string;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, string);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopImageText16(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    unsigned short *string;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, string);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopImageGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    unsigned int nGlyphs;
    CharInfoPtr *ppCharInfo;
    unsigned char *pGlyphBase;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->ImageGlyphBlt)(pDraw, pGC, x, y, nGlyphs,
                               ppCharInfo, pGlyphBase);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopPolyGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    unsigned int nGlyphs;
    CharInfoPtr *ppCharInfo;
    unsigned char *pGlyphBase;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyGlyphBlt)(pDraw, pGC, x, y, nGlyphs,
                               ppCharInfo, pGlyphBase);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
_NoopPushPixels(pGC, pBitmap, pDraw, width, height, x, y)
    GCPtr pGC;
    PixmapPtr pBitmap;
    DrawablePtr pDraw;
    int width, height;
    int x, y;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PushPixels)(pGC, pBitmap, pDraw, width, height, x, y);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}

static void
_NoopStub()
{
}


#endif /* I128_FAST_GC_OPS */
