/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/mach64gc.c,v 3.2 1995/01/28 15:53:26 dawes Exp $ */
/***********************************************************
Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1993,1994 by Kevin E. Martin, Chapel Hill, North Carolina.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL, KEVIN E. MARTIN, RICKARD E. FAITH, AND CRAIG E. GROESCHEL DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL DIGITAL OR KEVIN E. MARTIN OR
RICKARD E. FAITH OR CRAIG E. GROESCHEL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
Modified for 16 bpp and VTSema-dependent validation by
  Craig E. Groeschel (craig@adikia.sccsi.com)
Modified for the Mach64 by Kevin E. Martin (martin@cs.unc.edu)

****************************************************************************/
/* $XConsortium: mach64gc.c /main/2 1995/11/12 17:39:05 kaleb $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
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

#include "mach64.h"

static unsigned long PMask;

void mach64ValidateGC(
#ifdef NeedFunctionPrototypes
    register GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
#endif
);

void
cfbValidateGC(pGC, changes, pDrawable)
    register GCPtr pGC;
    unsigned long changes;
    DrawablePtr pDrawable;
{
    mach64ValidateGC(pGC, changes, pDrawable);
}

void
cfb16ValidateGC(pGC, changes, pDrawable)
    register GCPtr pGC;
    unsigned long changes;
    DrawablePtr pDrawable;
{
    mach64ValidateGC(pGC, changes, pDrawable);
}

void
cfb32ValidateGC(pGC, changes, pDrawable)
    register GCPtr pGC;
    unsigned long changes;
    DrawablePtr pDrawable;
{
    mach64ValidateGC(pGC, changes, pDrawable);
}

Bool
cfbCreateGC(pGC) GCPtr pGC; {return mach64CreateGC(pGC);}

Bool
cfb16CreateGC(pGC) GCPtr pGC; {return mach64CreateGC(pGC);}

Bool
cfb32CreateGC(pGC) GCPtr pGC; {return mach64CreateGC(pGC);}

/* Pointers to cfb routines. */
static int  (*pcfbReduceRasterOp)();
static Bool (*pcfbDestroyPixmap)();
static PixmapPtr (*pcfbCopyPixmap)();
static RegionPtr (*pcfbCopyArea)();
static RegionPtr (*pcfbCopyPlane)();
static void (*pcfbPolyPoint)();
static void (*pcfbPadPixmap)();
static void (*pcfbCopyRotatePixmap)();
static void (*pcfbFillPoly1RectCopy)();
static void (*pcfbFillPoly1RectGeneral)();
static void (*pcfbLineSD)();
static void (*pcfbLineSS)();
static void (*pcfbLineSS1Rect)();
static void (*pcfbPolyFillRect)();
static void (*pcfbPolyFillArcSolidCopy)();
static void (*pcfbPolyFillArcSolidGeneral)();
static void (*pcfbSegmentSD)();
static void (*pcfbSegmentSS)();
static void (*pcfbSegmentSS1Rect)();
static void (*pcfbSolidSpansCopy)();
static void (*pcfbSolidSpansXor)();
static void (*pcfbSolidSpansGeneral)();
static void (*pcfbTile32FSCopy)();
static void (*pcfbTile32FSGeneral)();
static void (*pcfbUnnaturalStippleFS)();
static void (*pcfbUnnaturalTileFS)();
static void (*pcfbZeroPolyArcSSCopy)();
static void (*pcfbZeroPolyArcSSXor)();
static void (*pcfbZeroPolyArcSSGeneral)();
static void (*pcfbPushPixels)();
static void (*pcfbPolyGlyphBlt8)();
static void (*pcfbImageGlyphBlt8)();
static void (*pPolyFillRectStip)();
static void (*pPolyFillRectOpStip)();
static void (*pcfbSetSpans)();

static GCFuncs mach64GCFuncs = {
    mach64ValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

/* Set up GC structures for 8, 16, and 32 bpp.  Kept separate for clarity. */

static GCOps	mach64Ops = {
    mach64SolidFSpans,
    cfbSetSpans,
    cfbPutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfbPolyPoint,
    miWideLine,
    miPolySegment,
    mach64PolyRectangle,
    miPolyArc,
    miFillPolygon,
    mach64PolyFillRect,
    miPolyFillArc,
    mach64PolyText8,
    mach64PolyText16,
    mach64ImageText8,
    mach64ImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    miPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	mach64Ops16 = {
    mach64SolidFSpans,
    cfb16SetSpans,
    cfb16PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb16PolyPoint,
    miWideLine,
    miPolySegment,
    mach64PolyRectangle,
    miPolyArc,
    miFillPolygon,
    mach64PolyFillRect,
    miPolyFillArc,
    mach64PolyText8,
    miPolyText16,
    mach64ImageText8,
    miImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    miPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	mach64Ops32 = {
    mach64SolidFSpans,
    cfb32SetSpans,
    cfb32PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb32PolyPoint,
    miWideLine,
    miPolySegment,
    mach64PolyRectangle,
    miPolyArc,
    miFillPolygon,
    mach64PolyFillRect,
    miPolyFillArc,
    mach64PolyText8,
    miPolyText16,
    mach64ImageText8,
    miImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    miPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfbTEOps1Rect = {
    cfbSolidSpansCopy,
    cfbSetSpans,
    cfbPutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfbPolyPoint,
    cfb8LineSS1Rect,
    cfb8SegmentSS1Rect,
    miPolyRectangle,
    cfbZeroPolyArcSS8Copy,
    cfbFillPoly1RectCopy,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfbTEGlyphBlt8,
    cfbPolyGlyphBlt8,
    cfbPushPixels8
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb16TEOps1Rect = {
    cfb16SolidSpansCopy,
    cfb16SetSpans,
    cfb16PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb16PolyPoint,
    cfb16LineSS1Rect,
    cfb16SegmentSS1Rect,
    miPolyRectangle,
    cfb16ZeroPolyArcSSCopy,
    cfb16FillPoly1RectCopy,
    cfb16PolyFillRect,
    cfb16PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb16ImageGlyphBlt8,
    cfb16PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb32TEOps1Rect = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb32PolyPoint,
    cfb32LineSS1Rect,
    cfb32SegmentSS1Rect,
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

static GCOps	cfbTEOps = {
    cfbSolidSpansCopy,
    cfbSetSpans,
    cfbPutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfbPolyPoint,
    cfbLineSS,
    cfbSegmentSS,
    miPolyRectangle,
    cfbZeroPolyArcSS8Copy,
    miFillPolygon,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfbTEGlyphBlt8,
    cfbPolyGlyphBlt8,
    cfbPushPixels8
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb16TEOps = {
    cfb16SolidSpansCopy,
    cfb16SetSpans,
    cfb16PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb16PolyPoint,
    cfb16LineSS,
    cfb16SegmentSS,
    miPolyRectangle,
    cfb16ZeroPolyArcSSCopy,
    miFillPolygon,
    cfb16PolyFillRect,
    cfb16PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb16ImageGlyphBlt8,
    cfb16PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb32TEOps = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    mach64CopyArea,
    mach64CopyPlane,
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

static GCOps	cfbNonTEOps1Rect = {
    cfbSolidSpansCopy,
    cfbSetSpans,
    cfbPutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfbPolyPoint,
    cfb8LineSS1Rect,
    cfb8SegmentSS1Rect,
    miPolyRectangle,
    cfbZeroPolyArcSS8Copy,
    cfbFillPoly1RectCopy,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfbImageGlyphBlt8,
    cfbPolyGlyphBlt8,
    cfbPushPixels8
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb16NonTEOps1Rect = {
    cfb16SolidSpansCopy,
    cfb16SetSpans,
    cfb16PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb16PolyPoint,
    cfb16LineSS1Rect,
    cfb16SegmentSS1Rect,
    miPolyRectangle,
    cfb16ZeroPolyArcSSCopy,
    cfb16FillPoly1RectCopy,
    cfb16PolyFillRect,
    cfb16PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb16ImageGlyphBlt8,
    cfb16PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb32NonTEOps1Rect = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb32PolyPoint,
    cfb32LineSS1Rect,
    cfb32SegmentSS1Rect,
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

static GCOps	cfbNonTEOps = {
    cfbSolidSpansCopy,
    cfbSetSpans,
    cfbPutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfbPolyPoint,
    cfbLineSS,
    cfbSegmentSS,
    miPolyRectangle,
    cfbZeroPolyArcSS8Copy,
    miFillPolygon,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfbImageGlyphBlt8,
    cfbPolyGlyphBlt8,
    cfbPushPixels8
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb16NonTEOps = {
    cfb16SolidSpansCopy,
    cfb16SetSpans,
    cfb16PutImage,
    mach64CopyArea,
    mach64CopyPlane,
    cfb16PolyPoint,
    cfb16LineSS,
    cfb16SegmentSS,
    miPolyRectangle,
    cfb16ZeroPolyArcSSCopy,
    miFillPolygon,
    cfb16PolyFillRect,
    cfb16PolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    cfb16ImageGlyphBlt8,
    cfb16PolyGlyphBlt8,
    mfbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

static GCOps	cfb32NonTEOps = {
    cfb32SolidSpansCopy,
    cfb32SetSpans,
    cfb32PutImage,
    mach64CopyArea,
    mach64CopyPlane,
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

static GCOps *pmach64Ops;
static GCOps *pcfbTEOps1Rect;
static GCOps *pcfbTEOps;
static GCOps *pcfbNonTEOps1Rect;
static GCOps *pcfbNonTEOps;

void
mach64InitGC()
{
	PMask = (1UL << mach64InfoRec.depth) - 1;
	switch (mach64InfoRec.bitsPerPixel) {
	case 8:
		pcfbNonTEOps = &cfbNonTEOps;
		pcfbNonTEOps1Rect = &cfbNonTEOps1Rect;
		pcfbTEOps = &cfbTEOps;
		pcfbTEOps1Rect = &cfbTEOps1Rect;
		pmach64Ops = &mach64Ops;
		pcfbCopyArea = cfbCopyArea;
		pcfbCopyPixmap = cfbCopyPixmap;
		pcfbCopyPlane = cfbCopyPlane;
		pcfbCopyRotatePixmap = cfbCopyRotatePixmap;
		pcfbDestroyPixmap = cfbDestroyPixmap;
		pcfbFillPoly1RectCopy = cfbFillPoly1RectCopy;
		pcfbFillPoly1RectGeneral = cfbFillPoly1RectGeneral;
		pcfbImageGlyphBlt8 = cfbImageGlyphBlt8;
		pcfbLineSD = cfbLineSD;
		pcfbLineSS = cfbLineSS;
		pcfbLineSS1Rect = cfb8LineSS1Rect;
		pcfbPadPixmap = cfbPadPixmap;
		pcfbPolyFillArcSolidCopy = cfbPolyFillArcSolidCopy;
		pcfbPolyFillArcSolidGeneral = cfbPolyFillArcSolidGeneral;
		pcfbPolyFillRect = cfbPolyFillRect;
		pcfbPolyGlyphBlt8 = cfbPolyGlyphBlt8;
		pcfbPolyPoint = cfbPolyPoint;
		pcfbPushPixels = cfbPushPixels8;
		pcfbReduceRasterOp = cfbReduceRasterOp;
		pcfbSegmentSD = cfbSegmentSD;
		pcfbSegmentSS = cfbSegmentSS;
		pcfbSegmentSS1Rect = cfb8SegmentSS1Rect;
		pcfbSetSpans = cfbSetSpans;
		pcfbSolidSpansCopy = cfbSolidSpansCopy;
		pcfbSolidSpansGeneral = cfbSolidSpansGeneral;
		pcfbSolidSpansXor = cfbSolidSpansXor;
		pcfbTile32FSCopy = cfbTile32FSCopy;
		pcfbTile32FSGeneral = cfbTile32FSGeneral;
		pcfbUnnaturalStippleFS = cfbUnnaturalStippleFS;
		pcfbUnnaturalTileFS = cfbUnnaturalTileFS;
		pcfbZeroPolyArcSSCopy = cfbZeroPolyArcSS8Copy;
		pcfbZeroPolyArcSSGeneral = cfbZeroPolyArcSS8General;
		pcfbZeroPolyArcSSXor = cfbZeroPolyArcSS8Xor;
		/* useTEGlyphBlt = cfbTEGlyphBlt8; */
		pPolyFillRectStip = pPolyFillRectOpStip = cfbPolyFillRect;
		break;
	case 16:
		pmach64Ops = &mach64Ops16;
		pcfbTEOps1Rect = &cfb16TEOps1Rect;
		pcfbTEOps = &cfb16TEOps;
		pcfbNonTEOps1Rect = &cfb16NonTEOps1Rect;
		pcfbNonTEOps = &cfb16NonTEOps;
		pcfbCopyArea = cfb16CopyArea;
		pcfbCopyPixmap = cfb16CopyPixmap;
		pcfbCopyPlane = cfb16CopyPlane;
		pcfbCopyRotatePixmap = cfb16CopyRotatePixmap;
		pcfbDestroyPixmap = cfb16DestroyPixmap;
		pcfbFillPoly1RectCopy = cfb16FillPoly1RectCopy;
		pcfbFillPoly1RectGeneral = cfb16FillPoly1RectGeneral;
		pcfbImageGlyphBlt8 = cfb16ImageGlyphBlt8;
		pcfbLineSD = cfb16LineSD;
		pcfbLineSS = cfb16LineSS;
		pcfbLineSS1Rect = cfb16LineSS1Rect;
		pcfbPadPixmap = cfb16PadPixmap;
		pcfbPolyFillArcSolidCopy = cfb16PolyFillArcSolidCopy;
		pcfbPolyFillArcSolidGeneral = cfb16PolyFillArcSolidGeneral;
		pcfbPolyFillRect = cfb16PolyFillRect;
		pcfbPolyGlyphBlt8 = cfb16PolyGlyphBlt8;
		pcfbPolyPoint = cfb16PolyPoint;
		pcfbPushPixels = mfbPushPixels;
		pcfbReduceRasterOp = cfb16ReduceRasterOp;
		pcfbSegmentSD = cfb16SegmentSD;
		pcfbSegmentSS = cfb16SegmentSS;
		pcfbSegmentSS1Rect = cfb16SegmentSS1Rect;
		pcfbSetSpans = cfb16SetSpans;
		pcfbSolidSpansCopy = cfb16SolidSpansCopy;
		pcfbSolidSpansGeneral = cfb16SolidSpansGeneral;
		pcfbSolidSpansXor = cfb16SolidSpansXor;
		pcfbTile32FSCopy = cfb16Tile32FSCopy;
		pcfbTile32FSGeneral = cfb16Tile32FSGeneral;
		pcfbUnnaturalStippleFS = cfb16UnnaturalStippleFS;
		pcfbUnnaturalTileFS = cfb16UnnaturalTileFS;
		pcfbZeroPolyArcSSCopy = cfb16ZeroPolyArcSSCopy;
		pcfbZeroPolyArcSSGeneral = cfb16ZeroPolyArcSSGeneral;
		pcfbZeroPolyArcSSXor = cfb16ZeroPolyArcSSXor;
		/* useTEGlyphBlt = cfb16ImageGlyphBlt8; */
		pPolyFillRectStip = pPolyFillRectOpStip = miPolyFillRect;
		break;
	case 32:
		pmach64Ops = &mach64Ops32;
		pcfbTEOps1Rect = &cfb32TEOps1Rect;
		pcfbTEOps = &cfb32TEOps;
		pcfbNonTEOps1Rect = &cfb32NonTEOps1Rect;
		pcfbNonTEOps = &cfb32NonTEOps;
		pcfbCopyArea = cfb32CopyArea;
		pcfbCopyPixmap = cfb32CopyPixmap;
		pcfbCopyPlane = cfb32CopyPlane;
		pcfbCopyRotatePixmap = cfb32CopyRotatePixmap;
		pcfbDestroyPixmap = cfb32DestroyPixmap;
		pcfbFillPoly1RectCopy = cfb32FillPoly1RectCopy;
		pcfbFillPoly1RectGeneral = cfb32FillPoly1RectGeneral;
		pcfbImageGlyphBlt8 = cfb32ImageGlyphBlt8;
		pcfbLineSD = cfb32LineSD;
		pcfbLineSS = cfb32LineSS;
		pcfbLineSS1Rect = cfb32LineSS1Rect;
		pcfbPadPixmap = cfb32PadPixmap;
		pcfbPolyFillArcSolidCopy = cfb32PolyFillArcSolidCopy;
		pcfbPolyFillArcSolidGeneral = cfb32PolyFillArcSolidGeneral;
		pcfbPolyFillRect = cfb32PolyFillRect;
		pcfbPolyGlyphBlt8 = cfb32PolyGlyphBlt8;
		pcfbPolyPoint = cfb32PolyPoint;
		pcfbPushPixels = mfbPushPixels;
		pcfbReduceRasterOp = cfb32ReduceRasterOp;
		pcfbSegmentSD = cfb32SegmentSD;
		pcfbSegmentSS = cfb32SegmentSS;
		pcfbSegmentSS1Rect = cfb32SegmentSS1Rect;
		pcfbSetSpans = cfb32SetSpans;
		pcfbSolidSpansCopy = cfb32SolidSpansCopy;
		pcfbSolidSpansGeneral = cfb32SolidSpansGeneral;
		pcfbSolidSpansXor = cfb32SolidSpansXor;
		pcfbTile32FSCopy = cfb32Tile32FSCopy;
		pcfbTile32FSGeneral = cfb32Tile32FSGeneral;
		pcfbUnnaturalStippleFS = cfb32UnnaturalStippleFS;
		pcfbUnnaturalTileFS = cfb32UnnaturalTileFS;
		pcfbZeroPolyArcSSCopy = cfb32ZeroPolyArcSSCopy;
		pcfbZeroPolyArcSSGeneral = cfb32ZeroPolyArcSSGeneral;
		pcfbZeroPolyArcSSXor = cfb32ZeroPolyArcSSXor;
		/* useTEGlyphBlt = cfb32ImageGlyphBlt8; */
		pPolyFillRectStip = pPolyFillRectOpStip = miPolyFillRect;
		break;
	}
	return;
}

static GCOps *
matchCommon (pGC, devPriv)
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
	if (TERMINALFONT(pGC->font) &&
	    (FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB
            || mach64InfoRec.bitsPerPixel != 8))
#ifdef NO_ONE_RECT
	    return pcfbTEOps1Rect;
#else
	    if (devPriv->oneRect)
		return pcfbTEOps1Rect;
	    else
		return pcfbTEOps;
#endif
	else
#ifdef NO_ONE_RECT
	    return pcfbNonTEOps1Rect;
#else
	    if (devPriv->oneRect)
		return pcfbNonTEOps1Rect;
	    else
		return pcfbNonTEOps;
#endif
    }
    return 0;
}

Bool
mach64CreateGC(pGC)
    register GCPtr pGC;
{
    cfbPrivGC  *pPriv;

    if (PixmapWidthPaddingInfo[pGC->depth].padPixelsLog2 == LOG2_BITMAP_PAD)
	return (mfbCreateGC(pGC));
    if (pGC->depth != mach64InfoRec.depth) {
	ErrorF("mach64CreateGC: unsupported depth: %d\n", pGC->depth);
	return FALSE;
    }
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;

    /*
     * some of the output primitives aren't really necessary, since they
     * will be filled in ValidateGC because of dix/CreateGC() setting all
     * the change bits.  Others are necessary because although they depend
     * on being a color frame buffer, they don't change 
     */

    pGC->ops = pcfbNonTEOps;
    pGC->funcs = &mach64GCFuncs;

    /* cfb wants to translate before scan conversion */
    pGC->miTranslate = 1;

    pPriv = (cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr);
    pPriv->rop = pGC->alu;
    pPriv->oneRect = FALSE;
    pPriv->fExpose = TRUE;
    pPriv->freeCompClip = FALSE;
    pPriv->pRotatedPixmap = (PixmapPtr) NULL;
    return TRUE;
}


/*------------------------------------------------------------------
 * Clipping conventions
 *	if the drawable is a window
 *	    CT_REGION ==> pCompositeClip really is the composite
 *	    CT_other ==> pCompositeClip is the window clip region
 *	if the drawable is a pixmap
 *	    CT_REGION ==> pCompositeClip is the translated client region
 *		clipped to the pixmap boundary
 *	    CT_other ==> pCompositeClip is the pixmap bounding box
 */

void
mach64ValidateGC(pGC, changes, pDrawable)
    register GCPtr  pGC;
    unsigned long   changes;
    DrawablePtr	    pDrawable;
{
    WindowPtr   pWin;
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
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDrawable;
    }
    else
    {
	pWin = (WindowPtr) NULL;
    }

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
	miComputeCompositeClip(pGC, pDrawable);
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
		    (nstipple = (*pcfbCopyPixmap)(pGC->stipple)))
		{
		    (*pcfbPadPixmap)(nstipple);
		    mach64DestroyPixmap(pGC->stipple);
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
     * If the drawable has changed,  check its depth & ensure suitable
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
		int width = pGC->tile.pixmap->drawable.width *
			    mach64InfoRec.bitsPerPixel;

		if ((width <= PGSZ) && !(width & (width - 1)))
		{
		    (*pcfbCopyRotatePixmap)(pGC->tile.pixmap,
					&devPriv->pRotatedPixmap,
					xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    if (mach64InfoRec.bitsPerPixel == 8)
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
	}
	if (!new_pix && devPriv->pRotatedPixmap)
	{
	    mach64DestroyPixmap(devPriv->pRotatedPixmap);
	    devPriv->pRotatedPixmap = (PixmapPtr) NULL;
	}
    }

    if (new_rrop)
    {
	int old_rrop;

	old_rrop = devPriv->rop;
	devPriv->rop = (*pcfbReduceRasterOp) (pGC->alu, pGC->fgPixel,
					   pGC->planemask,
					   &devPriv->and, &devPriv->xor);
	if (old_rrop == devPriv->rop)
	    new_rrop = FALSE;
	else
	{
	    new_line = TRUE;
	    new_text = TRUE;
	    new_fillspans = TRUE;
	    new_fillarea = TRUE;
	}
    }

#if 0
    /* This is used to make the mach64 server with a 4 or 8 Mb video memory
     * aperture use only the cfb code.
     */
    pWin = (WindowPtr) NULL;
#endif

    if (pWin && pGC->ops->devPrivate.val != 2)
    {
	if (pGC->ops->devPrivate.val == 1)
	    miDestroyGCOps (pGC->ops);

	pGC->ops = miCreateGCOps (pmach64Ops);
	pGC->ops->devPrivate.val = 2;

	/* Make sure that everything is properly initialized the first 
	 * time through i
	 */
	new_rrop = new_line = new_text = new_fillspans = new_fillarea = TRUE;
    }
    else if (!pWin && (new_rrop || new_fillspans || new_text ||
		       new_fillarea || new_line))
    {
	GCOps	*newops;

	if (newops = matchCommon (pGC, devPriv))
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
	    else if (pGC->ops->devPrivate.val != 1)
	    {
		miDestroyGCOps (pGC->ops);
		pGC->ops = miCreateGCOps (pcfbNonTEOps);
		pGC->ops->devPrivate.val = 1;
		new_rrop = new_line = new_text = new_fillspans = new_fillarea = TRUE;
	    }
	}
    }

    /* deal with the changes we've collected */
    if (new_line)
    {
	if (pWin) 
	{
	    pGC->ops->FillPolygon = miFillPolygon;
	    if (pGC->lineWidth == 0)
	    {
		if ((pGC->lineStyle == LineSolid) && 
		    (pGC->fillStyle == FillSolid))
		{
		    switch (devPriv->rop)
		    {
		    case GXxor:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSXor;
			break;
		    case GXcopy:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSCopy;
			break;
		    default:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSGeneral;
			break;
		    }
		}
		else
		    pGC->ops->PolyArc = miZeroPolyArc;
	    }
	    else /* pGC->lineWidth */
		pGC->ops->PolyArc = miPolyArc;
	} 
	else /* pWin */
	{ 
	    pGC->ops->FillPolygon = miFillPolygon;
	    if (
#ifndef NO_ONE_RECT
		devPriv->oneRect &&
#endif
		pGC->fillStyle == FillSolid)
	    {
		switch (devPriv->rop) {
		case GXcopy:
		    pGC->ops->FillPolygon = pcfbFillPoly1RectCopy;
		    break;
		default:
		    pGC->ops->FillPolygon = pcfbFillPoly1RectGeneral;
		    break;
		}
	    }
	    if (pGC->lineWidth == 0)
	    {
		if ((pGC->lineStyle == LineSolid) &&
		    (pGC->fillStyle == FillSolid))
		{
		    switch (devPriv->rop)
		    {
		    case GXxor:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSXor;
			break;
		    case GXcopy:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSCopy;
			break;
		    default:
			pGC->ops->PolyArc = pcfbZeroPolyArcSSGeneral;
			break;
		    }
		}
		else
		    pGC->ops->PolyArc = miZeroPolyArc;
	    }
	    else
		pGC->ops->PolyArc = miPolyArc;
	}
    }
/*
 * Polylines and PolySegment depend on xf86VTSema, so must be validated
 * each time through.
 */
    if (new_line || pGC->ops->devPrivate.val == 2) 
    {
        pGC->ops->PolySegment = miPolySegment;
        switch (pGC->lineStyle) {
        case LineSolid:
	    if (pGC->lineWidth == 0) 
	    {
	        if (pGC->fillStyle == FillSolid) 
		{
		    if (pWin && xf86VTSema)
		    {
		        if (
#ifdef NO_ONE_RECT
			    1
#else
			    devPriv->oneRect
#endif
			    )
		        {
			    pGC->ops->Polylines = mach64Line;
			    pGC->ops->PolySegment = mach64Segment;
		        } 
			else 
			{
			    pGC->ops->Polylines = mach64Line;
			    pGC->ops->PolySegment = mach64Segment;
		        }
		    } 
		    else if (!pWin &&
#ifdef NO_ONE_RECT
				1 &&
#else
				devPriv->oneRect &&
#endif
				xf86VTSema &&
			((pDrawable->x >= pGC->pScreen->width - 32768) &&
			 (pDrawable->y >= pGC->pScreen->height - 32768)))
		    {
                        pGC->ops->Polylines = pcfbLineSS1Rect;
                        pGC->ops->PolySegment = pcfbSegmentSS1Rect;
                    } 
		    else 
		    {
                        pGC->ops->Polylines = pcfbLineSS;
                        pGC->ops->PolySegment = pcfbSegmentSS;
                    }
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
#ifdef NOT_DEFINED
	        if (pWin && xf86VTSema) 
	        {
		    pGC->ops->Polylines = mach64Dline;
		    pGC->ops->PolySegment = mach64Dsegment;
	        } 
	        else 
#endif
	        {
		    pGC->ops->Polylines = pcfbLineSD;
	    	    pGC->ops->PolySegment = pcfbSegmentSD;
	        }
	    } 
	    else
	        pGC->ops->Polylines = miWideDash;
	    break;
        } /* switch */
    } /* if */
    /* end of new_line */


    if (new_text && (pGC->font))
    {
#if 0
        if (pWin) 
	{
	    pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
	    pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
        } 
	else
#endif
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
	        if (pGC->fillStyle == FillSolid)
	        {
		    if (devPriv->rop == GXcopy)
		        pGC->ops->PolyGlyphBlt = pcfbPolyGlyphBlt8;
		    else if (mach64InfoRec.bitsPerPixel == 8)
		        pGC->ops->PolyGlyphBlt = cfbPolyGlyphRop8;
		    else
		        pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
	        }
	        else
		{
		    pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
		}

                /* special case ImageGlyphBlt for terminal emulator fonts */
                if (mach64InfoRec.bitsPerPixel == 8 && 
		    TERMINALFONT(pGC->font) &&
                    (pGC->planemask & PMask) == PMask &&
                    FONTMAXBOUNDS(pGC->font,characterWidth) >= PGSZB)
                {
                    pGC->ops->ImageGlyphBlt = cfbTEGlyphBlt8; /* useTEGlyphBlt*/
                }
                else
                {
                    if (devPriv->rop == GXcopy && 
			pGC->fillStyle == FillSolid &&
                        (pGC->planemask & PMask) == PMask)
		    {
                        pGC->ops->ImageGlyphBlt = pcfbImageGlyphBlt8;
		    }
                    else
                        pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
                }
            }
        }
    }    

    if (new_fillspans || pGC->ops->devPrivate.val == 2) 
    {
        if (pWin && xf86VTSema) 
	{
	    switch (pGC->fillStyle) {
	    case FillSolid:
	        pGC->ops->FillSpans = mach64SolidFSpans;
	        break;
	    case FillTiled:
		pGC->ops->FillSpans = mach64TiledFSpans;
	        if (mach64InfoRec.bitsPerPixel == 8)
	        {
	            if ((devPriv->pRotatedPixmap) && (pGC->alu == GXcopy) && 
			((pGC->planemask & PMask) == PMask))
		        {
		            pGC->ops->FillSpans = pcfbTile32FSCopy;
			}
	        }
	        break;
	    case FillStippled:
	        pGC->ops->FillSpans = mach64StipFSpans;
	        break;
	    case FillOpaqueStippled:
	        pGC->ops->FillSpans = mach64OStipFSpans;
	        break;
	    default:
	        FatalError("mach64ValidateGC: illegal fillStyle\n");
	    }
        } 
	else if (!pWin && xf86VTSema) 
	{
	    switch (pGC->fillStyle) {
	    case FillSolid:
	        switch (devPriv->rop) {
	        case GXcopy:
		    pGC->ops->FillSpans = pcfbSolidSpansCopy;
		    break;
	        case GXxor:
		    pGC->ops->FillSpans = pcfbSolidSpansXor;
		    break;
	        default:
		    pGC->ops->FillSpans = pcfbSolidSpansGeneral;
		    break;
	        }
	        break;
	    case FillTiled:
	        if (devPriv->pRotatedPixmap)
	        {
		    if (pGC->alu == GXcopy && (pGC->planemask & PMask) == PMask)
		        pGC->ops->FillSpans = pcfbTile32FSCopy;
		    else
		        pGC->ops->FillSpans = pcfbTile32FSGeneral;
	        }
	        else
		    pGC->ops->FillSpans = pcfbUnnaturalTileFS;
	        break;
	    case FillStippled:
	        if (mach64InfoRec.bitsPerPixel == 8 && devPriv->pRotatedPixmap)
		    pGC->ops->FillSpans = cfb8Stipple32FS;
	        else
		    pGC->ops->FillSpans = pcfbUnnaturalStippleFS;
	        break;
	    case FillOpaqueStippled:
	        if (mach64InfoRec.bitsPerPixel == 8 && devPriv->pRotatedPixmap)
		    pGC->ops->FillSpans = cfb8OpaqueStipple32FS;
	        else
		    pGC->ops->FillSpans = pcfbUnnaturalStippleFS;
	        break;
	    default:
	        FatalError("mach64ValidateGC: illegal fillStyle\n");
	    }
        } 
	else 
	{
	    switch (pGC->fillStyle) {
	    case FillSolid:
	        pGC->ops->FillSpans = pcfbSolidSpansGeneral;
	        break;
	    case FillTiled:
	        pGC->ops->FillSpans = pcfbUnnaturalTileFS;
	        break;
	    case FillStippled:
	    case FillOpaqueStippled:
	        pGC->ops->FillSpans = pcfbUnnaturalStippleFS;
	        break;
	    }
        }
    } /* end of new_fillspans */

    if (new_fillarea) 
    {
	if (pWin) 
	{
	    if (mach64InfoRec.bitsPerPixel == 8) 
	    {
		pGC->ops->PushPixels = mfbPushPixels;
		if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
		    pGC->ops->PushPixels = cfbPushPixels8;
	    }
	    pGC->ops->PolyFillArc = miPolyFillArc;
	} 
	else /* pWin */
	{
	    if (mach64InfoRec.bitsPerPixel == 8) 
	    {
		pGC->ops->PushPixels = mfbPushPixels;
		if (pGC->fillStyle == FillSolid && devPriv->rop == GXcopy)
		    pGC->ops->PushPixels = cfbPushPixels8;
	    }
		
	    pGC->ops->PolyFillArc = miPolyFillArc;
	    if (pGC->fillStyle == FillSolid)
	    {
		switch (devPriv->rop) {
		case GXcopy:
		    pGC->ops->PolyFillArc = pcfbPolyFillArcSolidCopy;
		    break;
		default:
		    pGC->ops->PolyFillArc = pcfbPolyFillArcSolidGeneral;
		    break;
		}
	    }
	} /* if pWin */
    } /* if new_fillarea */

    if (new_fillarea || pGC->ops->devPrivate.val == 2) 
    {
	if (pWin && xf86VTSema) 
	{
	    pGC->ops->PolyFillRect = mach64PolyFillRect;
	} 
	else 
	{
	    if (mach64InfoRec.bitsPerPixel == 8) 
	    {
		if (!xf86VTSema)
		    pGC->ops->PolyFillRect = pcfbPolyFillRect;
	    } 
	    else 
	    {
		pGC->ops->PolyFillRect = miPolyFillRect;
		if (pGC->fillStyle == FillSolid || pGC->fillStyle == FillTiled)
		{
		    pGC->ops->PolyFillRect = pcfbPolyFillRect;
		}
	    }
	}
    }

    if (pGC->ops->devPrivate.val == 2) {
	if (xf86VTSema) {
	    pGC->ops->CopyArea = mach64CopyArea;
	    pGC->ops->CopyPlane = mach64CopyPlane;
	    pGC->ops->ImageText8 = mach64ImageText8;
	    pGC->ops->PolyPoint = pcfbPolyPoint;
	    pGC->ops->PolyText8 = mach64PolyText8;
	    pGC->ops->SetSpans = pcfbSetSpans;
	} else {
	    pGC->ops->CopyArea = pcfbCopyArea;
	    pGC->ops->CopyPlane = pcfbCopyPlane;
	    pGC->ops->ImageText8 = miImageText8;
	    pGC->ops->PolyPoint = pcfbPolyPoint;
	    pGC->ops->PolyText8 = miPolyText8;
	    if (pGC->depth == 1)
		pGC->ops->SetSpans = mfbSetSpans;
	    else
		pGC->ops->SetSpans = pcfbSetSpans;
	}
    }
}

Bool
mach64DestroyPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    mach64CacheFreeSlot(pPixmap);
    (*pcfbDestroyPixmap)(pPixmap);
}
