/*
 *	@(#) i128GCSolidOps.c 11.1 97/10/22
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
 * i128GCSolidOps.c
 *
 * GCOps wrappers
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "dixfontstr.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "i128Defs.h"
#include "i128Procs.h"

#ifdef I128_FAST_GC_OPS

static void _SolidFillSpans();
static void _SolidSetSpans();
static void _SolidPutImage();
static RegionPtr _SolidCopyArea();
static RegionPtr _SolidCopyPlane();
static void _SolidPolyPoint();
static void _SolidPolylines();
static void _SolidPolySegment();
static void _SolidPolyRectangle();
static void _SolidPolyArc();
static void _SolidFillPolygon();
static void _SolidPolyFillRect();
static void _SolidPolyFillArc();
static int _SolidPolyText8();
static int _SolidPolyText16();
static void _SolidImageText8();
static void _SolidImageText16();
static void _SolidImageGlyphBlt();
static void _SolidPolyGlyphBlt();
static void _SolidPushPixels();
static void _SolidStub();

GCOps i128GCSolidOps = {
     _SolidFillSpans,      /*  void (* FillSpans)() */
     _SolidSetSpans,       /*  void (* SetSpans)()  */
     _SolidPutImage,       /*  void (* PutImage)()  */
     _SolidCopyArea,       /*  RegionPtr (* CopyArea)()     */
     _SolidCopyPlane,      /*  RegionPtr (* CopyPlane)() */
     _SolidPolyPoint,      /*  void (* PolyPoint)() */
     _SolidPolylines,      /*  void (* Polylines)() */
     _SolidPolySegment,    /*  void (* PolySegment)() */
     _SolidPolyRectangle,  /*  void (* PolyRectangle)() */
     _SolidPolyArc,        /*  void (* PolyArc)()   */
     _SolidFillPolygon,    /*  void (* FillPolygon)() */
     _SolidPolyFillRect,   /*  void (* PolyFillRect)() */
     _SolidPolyFillArc,    /*  void (* PolyFillArc)() */
     _SolidPolyText8,      /*  int (* PolyText8)()  */
     _SolidPolyText16,     /*  int (* PolyText16)() */
     _SolidImageText8,     /*  void (* ImageText8)() */
     _SolidImageText16,    /*  void (* ImageText16)() */
     _SolidImageGlyphBlt,  /*  void (* ImageGlyphBlt)() */
     _SolidPolyGlyphBlt,   /*  void (* PolyGlyphBlt)() */
     _SolidPushPixels,     /*  void (* PushPixels)() */
     _SolidStub,           /*  void (* Stub)() */
};

extern
GCFuncs i128GCFuncs;

static void
_SolidFillSpans(pDraw, pGC, nSpans, pPoints, pWidths, fSorted)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nSpans;
    DDXPointPtr pPoints;
    unsigned int *pWidths;
    int fSorted;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if ((numClipRects == 1) &&
        (pGC->lineWidth <= 1))
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        I128_CLIP(clip);
        i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                 i128Priv->rop[pGC->alu] |
                                 I128_CMD_CLIP_IN |
                                 I128_CMD_SOLID);
        i128Priv->engine->plane_mask = 
            I128_CONVERT(i128Priv->mode.pixelsize, pGC->planemask);
        i128Priv->engine->foreground = pGC->fgPixel;
        i128Priv->engine->xy0 = 0;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

        while (nSpans--)
        {
            I128_WAIT_UNTIL_READY(i128Priv->engine); 
            i128Priv->engine->xy2 = I128_XY(*pWidths, 1);
            i128Priv->engine->xy1 = I128_XY(pPoints->x,
                                            pPoints->y);
            pPoints++;
            pWidths++;
        }

        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sFillSpans:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->FillSpans)(pDraw, pGC, nSpans,
                               pPoints, pWidths, fSorted);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}



static void
_SolidSetSpans(pDraw, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
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
_SolidPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
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
_SolidCopyArea(pSrc, pDst, pGC, srcx, srcy, width, height, dstx, dsty)
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
_SolidCopyPlane(pSrc, pDst, pGC, srcx, srcy,
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
_SolidPolyPoint(pDraw, pGC, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int mode;
    int nPoints;
    register xPoint *pPoints;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        I128_CLIP(clip);
        if (pGCPriv->rRop.alu == GXcopy)
        {
            switch (i128Priv->mode.pixelsize)
            {
              case 4:
                {
                    unsigned long *dst;
                    unsigned long fg;
                    int clip_x2, clip_y2, pitch;
        
                    fg = pGC->fgPixel;
                    pitch = i128Priv->mode.bitmap_pitch >> 2;

                    I128_WAIT_UNTIL_READY(i128Priv->engine);
        
                    i128Priv->info.memwins[0]->plane_mask =
                        I128_CONVERT(i128Priv->mode.pixelsize,
                                     pGCPriv->rRop.planemask);
        
                    x0 = pDraw->x;
                    y0 = pDraw->y;
                    dst = (unsigned long*)i128Priv->info.memwin[0].pointer +
                        pitch * y0 + x0; 
                    clip.x1 -= x0;
                    clip.y1 -= y0;
                    clip.x2 -= x0;
                    clip.y2 -= y0;

                    while (nPoints--)
                    {
                        if (pPoints->y < clip.y2 && pPoints->x < clip.x2 &&
                            pPoints->y > clip.y1 && pPoints->x > clip.x1)
                            dst[pitch * pPoints->y + pPoints->x] = fg;
                        ++pPoints;
                    }
                }
                break;

              case 2:
                {
                    unsigned short *dst;
                    unsigned short fg;
                    int clip_x2, clip_y2, pitch;
        
                    fg = (short)pGC->fgPixel;
                    pitch = i128Priv->mode.bitmap_pitch >> 1;

                    I128_WAIT_UNTIL_READY(i128Priv->engine);
        
                    i128Priv->info.memwins[0]->plane_mask =
                        I128_CONVERT(i128Priv->mode.pixelsize,
                                     pGCPriv->rRop.planemask);
        
                    x0 = pDraw->x;
                    y0 = pDraw->y;
                    dst = (unsigned short*)i128Priv->info.memwin[0].pointer +
                        pitch * y0 + x0; 
                    clip.x1 -= x0;
                    clip.y1 -= y0;
                    clip.x2 -= x0;
                    clip.y2 -= y0;

                    while (nPoints--)
                    {
                        if (pPoints->y < clip.y2 && pPoints->x < clip.x2 &&
                            pPoints->y > clip.y1 && pPoints->x > clip.x1)
                            dst[pitch * pPoints->y + pPoints->x] = fg;
                        ++pPoints;
                    }
                }
                break;

              case 1:
              default:
                {
                    unsigned char *dst;
                    unsigned char fg;
                    int clip_x2, clip_y2, pitch;
        
                    fg = (unsigned char)pGCPriv->rRop.fg;
                    pitch = i128Priv->mode.bitmap_pitch;

                    I128_WAIT_UNTIL_READY(i128Priv->engine);
        
                    i128Priv->info.memwins[0]->plane_mask =
                        I128_CONVERT(i128Priv->mode.pixelsize,
                                     pGCPriv->rRop.planemask);
        
                    x0 = pDraw->x;
                    y0 = pDraw->y;
                    dst = (unsigned char*)i128Priv->info.memwin[0].pointer +
                        pitch * y0 + x0; 
                    clip.x1 -= x0;
                    clip.y1 -= y0;
                    clip.x2 -= x0;
                    clip.y2 -= y0;

                    while (nPoints--)
                    {
                        if (pPoints->y < clip.y2 && pPoints->x < clip.x2 &&
                            pPoints->y > clip.y1 && pPoints->x > clip.x1)
                            dst[pitch * pPoints->y + pPoints->x] = fg;
                        ++pPoints;
                    }
                }
            }
        }
        else
        {
            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->plane_mask = 
                I128_CONVERT(i128Priv->mode.pixelsize,
                             pGCPriv->rRop.planemask);
            i128Priv->engine->foreground = (unsigned long)pGCPriv->rRop.fg;
            i128Priv->engine->xy2 = I128_XY(1, 1);
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                     i128Priv->rop[pGCPriv->rRop.alu] |
                                     I128_CMD_CLIP_IN |
                                     I128_CMD_SOLID);

            switch (mode)
            {
              case CoordModeOrigin:
                x0 = pDraw->x;
                y0 = pDraw->y;
                while (nPoints--)
                {
                    I128_WAIT_UNTIL_READY(i128Priv->engine); 
                    i128Priv->engine->xy1 =
                        I128_XY(x0 + pPoints->x, y0 + pPoints->y);
                    ++pPoints;
                }
                break;
    
              case CoordModePrevious:
              default:
                x0 = pPoints->x + pDraw->x;
                y0 = pPoints->y + pDraw->y;
                while (nPoints--)
                {
                    I128_WAIT_UNTIL_READY(i128Priv->engine);
                    i128Priv->engine->xy1 =
                        I128_XY(x0 + pPoints->x, y0 + pPoints->y);
                    ++pPoints;
                    x0 += pPoints->x;
                    y0 += pPoints->y;
                }
                break;
            }
        }
        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sPolyPoint:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyPoint)(pDraw, pGC, mode, nPoints, pPoints);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}


static void
_SolidPolylines(pDraw, pGC, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int mode;
    register int nPoints;
    register DDXPointPtr pPoints;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if ((numClipRects == 1) &&
        (pGC->lineWidth <= 1))
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        unsigned long cmd;
        unsigned long pat, pat_offset, pat_total;
        int xy;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        cmd = (I128_OPCODE_LINE |
               i128Priv->rop[pGCPriv->rRop.alu] |
               I128_CMD_CLIP_IN |
               I128_CMD_PATT_RESET |
               I128_CMD_NO_LAST);

        I128_CLIP(clip);

        switch (pGC->lineStyle)
        {
          case LineDoubleDash:
            i128Priv->engine->background = pGCPriv->rRop.bg;

          case LineOnOffDash:
            pat = 0;
            pat_total = 0;
            for (pat_offset = 0;
                 pat_offset < pGC->numInDashList;
                 pat_offset++)
            {
                char i = pGC->dash[pat_offset];
                unsigned long val = (pat_offset+1) % 2;
                pat_total += i;
                if (pat_total > I128_MAX_WIDTH)
                    goto _sPolylines;
                else while (i--)
                {
                    pat |= val;
                    pat <<= 1;
                }
            }
            if (pat_total == I128_MAX_WIDTH)
                pat_total = 0;
            i128Priv->engine->line_pattern = pat;
            i128Priv->engine->pattern_ctrl =
                (I128_LPAT_LENGTH(pat_total) |
                 I128_LPAT_INIT_PAT(pGC->dashOffset) |
                 I128_LPAT_ZOOM(1) |
                 I128_LPAT_INIT_ZOOM(0));
            break;
            
          case LineSolid:
          default:
            cmd |= I128_CMD_SOLID;
            break;
        }

        i128Priv->engine->cmd = cmd;
        i128Priv->engine->foreground = pGCPriv->rRop.fg;
        i128Priv->engine->plane_mask = 
            I128_CONVERT(I128_mode.pixelsize, pGCPriv->rRop.planemask);


        switch (mode)
        {
          case CoordModeOrigin:
            x0 = pDraw->x;
            y0 = pDraw->y;
            nPoints--;
            xy = I128_XY(x0 + pPoints->x, y0 + pPoints->y);
            ++pPoints;
            while (nPoints-- > 1)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                i128Priv->engine->xy0 = xy;
                xy = I128_XY(x0 + pPoints->x, y0 + pPoints->y);
                i128Priv->engine->xy1 = xy;
                ++pPoints;
            }

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            if (pGC->capStyle != CapNotLast || pGC->lineWidth != 0)
            {
                cmd &= ~I128_CMD_NO_LAST;
            }
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy0 = xy;
            xy = I128_XY(x0 + pPoints->x, y0 + pPoints->y);
            i128Priv->engine->xy1 = xy;

            break;
    
          case CoordModePrevious:
          default:
            x0 = pDraw->x + pPoints->x;
            y0 = pDraw->y + pPoints->y;
            xy = I128_XY(x0, y0);
            pPoints++;
            nPoints--;
            while (nPoints-- > 1)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                i128Priv->engine->xy0 = xy;
                x0 += pPoints->x;
                y0 += pPoints->y;
                xy = I128_XY(x0, y0);
                i128Priv->engine->xy1 = xy;
                ++pPoints;
            }

            I128_WAIT_UNTIL_READY(i128Priv->engine); 
            if (pGC->capStyle != CapNotLast || pGC->lineWidth != 0)
            {
                cmd &= ~I128_CMD_NO_LAST;
            }
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy0 = xy;
            xy = I128_XY(x0 + pPoints->x, y0 + pPoints->y);
            i128Priv->engine->xy1 = xy;
            break;
        }
        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sPolylines:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->Polylines)(pDraw, pGC, mode, nPoints, pPoints);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }

}


static void
_SolidPolySegment(pDraw, pGC, nSegments, pSegments)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nSegments;
    xSegment *pSegments;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if ((numClipRects == 1) &&
        (pGC->lineWidth <= 1))
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        unsigned long cmd;
        unsigned long pat, pat_offset, pat_total;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        cmd = (I128_OPCODE_LINE |
               i128Priv->rop[pGCPriv->rRop.alu] |
               I128_CMD_CLIP_IN |
               I128_CMD_PATT_RESET);

        I128_CLIP(clip);

        if ((pGC->capStyle == CapNotLast) && (pGC->lineWidth == 0))
            cmd |= I128_CMD_NO_LAST;

        switch (pGC->lineStyle)
        {
          case LineDoubleDash:
            i128Priv->engine->background = pGCPriv->rRop.bg;

          case LineOnOffDash:
            pat = 0;
            pat_total = 0;
            for (pat_offset = 0;
                 pat_offset < pGC->numInDashList;
                 pat_offset++)
            {
                char i = pGC->dash[pat_offset];
                unsigned long val = (pat_offset+1) % 2;
                pat_total += i;
                if (pat_total > I128_MAX_WIDTH)
                    goto _sPolySegment;
                else while (i--)
                {
                    pat |= val;
                    pat <<= 1;
                }
            }
            if (pat_total == I128_MAX_WIDTH)
                pat_total = 0;
            i128Priv->engine->line_pattern = pat;
            i128Priv->engine->pattern_ctrl =
                (I128_LPAT_LENGTH(pat_total) |
                 I128_LPAT_INIT_PAT(pGC->dashOffset) |
                 I128_LPAT_ZOOM(1) |
                 I128_LPAT_INIT_ZOOM(0));
            break;
            
          case LineSolid:
          default:
            cmd |= I128_CMD_SOLID;
            break;
        }

        i128Priv->engine->cmd = cmd;
        i128Priv->engine->foreground = pGCPriv->rRop.fg;
        i128Priv->engine->plane_mask = 
            I128_CONVERT(I128_mode.pixelsize, pGCPriv->rRop.planemask);

        while (nSegments--)
        {
            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->xy0 = I128_XY(pSegments->x1 + x0,
                                            pSegments->y1 + y0);
            i128Priv->engine->xy1 = I128_XY(pSegments->x2 + x0,
                                            pSegments->y2 + y0);
            pSegments++;
        }
        I128_CLIP(i128Priv->clip);
    }
    else
    {
      _sPolySegment:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolySegment)(pDraw, pGC, nSegments, pSegments);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}


static void
_SolidPolyRectangle(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nRects;
    xRectangle *pRects;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if ((numClipRects == 1) &&
        (pGC->lineWidth <= 1))
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        unsigned long cmd;
        unsigned long pat, pat_offset, pat_total;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        cmd = (I128_OPCODE_LINE |
               i128Priv->rop[pGCPriv->rRop.alu] |
               I128_CMD_CLIP_IN |
               I128_CMD_PATT_RESET |
               I128_CMD_NO_LAST);

        I128_CLIP(clip);

        switch (pGC->lineStyle)
        {
          case LineDoubleDash:
            i128Priv->engine->background = pGCPriv->rRop.bg;

          case LineOnOffDash:
            pat = 0;
            pat_total = 0;
            for (pat_offset = 0;
                 pat_offset < pGC->numInDashList;
                 pat_offset++)
            {
                char i = pGC->dash[pat_offset];
                unsigned long val = (pat_offset+1) % 2;
                pat_total += i;
                if (pat_total > I128_MAX_WIDTH)
                    goto _sPolyRectangle;
                else while (i--)
                {
                    pat |= val;
                    pat <<= 1;
                }
            }
            if (pat_total == I128_MAX_WIDTH)
                pat_total = 0;
            i128Priv->engine->line_pattern = pat;
            i128Priv->engine->pattern_ctrl =
                (I128_LPAT_LENGTH(pat_total) |
                 I128_LPAT_INIT_PAT(pGC->dashOffset) |
                 I128_LPAT_ZOOM(1) |
                 I128_LPAT_INIT_ZOOM(0));
            break;
            
          case LineSolid:
          default:
            cmd |= I128_CMD_SOLID;
            break;
        }

        i128Priv->engine->cmd = cmd;
        i128Priv->engine->foreground = pGCPriv->rRop.fg;
        i128Priv->engine->plane_mask = 
            I128_CONVERT(I128_mode.pixelsize, pGCPriv->rRop.planemask);

        while (nRects--)
        {
            unsigned long xy_TL, xy_TR, xy_BL, xy_BR;

            xy_TL = I128_XY(x0 + pRects->x, y0 + pRects->y);
            xy_TR = I128_XY(x0 + pRects->x + pRects->width - 1,
                            y0 + pRects->y);

            I128_WAIT_UNTIL_READY(i128Priv->engine); 
            i128Priv->engine->xy0 = xy_TL;
            i128Priv->engine->xy1 = xy_TR;


            xy_BR = I128_XY(x0 + pRects->x + pRects->width - 1,
                            y0 + pRects->y +
                            pRects->height - 1);

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->xy0 = xy_TR;
            i128Priv->engine->xy1 = xy_BR;


            xy_BL = I128_XY(x0 + pRects->x,
                            y0 + pRects->y + pRects->height - 1);

            I128_WAIT_UNTIL_READY(i128Priv->engine); 
            i128Priv->engine->xy0 = xy_BR;
            i128Priv->engine->xy1 = xy_BL;

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->xy0 = xy_BL;
            i128Priv->engine->xy1 = xy_TL;

            pRects++;
        }

        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sPolyRectangle:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyRectangle)(pDraw, pGC, nRects, pRects);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}

#ifdef usl
#include "nfbZeroArc.h"
#else
#include "server/ddx/nfb/nfbZeroArc.h"
#endif
#define FULLCIRCLE (360 * 64)
#define ARCDATA(radius) arcData[radius]

static void
_SolidPolyArc(pDraw, pGC, nArcs, pArcs)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nArcs;
    xArc *pArcs;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if ((numClipRects == 1) &&
        (pGC->lineWidth <= 1) &&
        (pGC->lineStyle == LineSolid))
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        I128_CLIP(clip);
        {
            /* Ripped straight from NFB - but takes advantage of
               hardware clipping.  Need to add a new nfb routine. */
            int n;
            int i;
            int nbox;
            unsigned int offset = 0;
            unsigned int stride;
            unsigned char *image, *imptr;
            xArc *arc;
            BoxRec box;
            unsigned long int planemask = pGCPriv->rRop.planemask;
            unsigned long int fg = pGCPriv->rRop.fg;
            unsigned char alu = pGCPriv->rRop.alu;
            
            for (arc = pArcs, i = nArcs; --i >= 0; arc++)
            {
                if ( (arc->width == arc->height)
                    && (arc->width < 32) && (arc->angle2 >= FULLCIRCLE) )
                {
                    box.x1 = arc->x + pDraw->x;
                    box.y1 = arc->y + pDraw->y;
                    box.x2 = box.x1 + arc->width + 1;
                    box.y2 = box.y1 + arc->height + 1;
            
            
                    stride = (arc->width + 8) >> 3;
                    image = ARCDATA(arc->width);
                    offset = 0;
            
                    i128DrawMonoImage(&box, image, offset, stride, fg, alu,
                       planemask, pDraw);

                }
                else
                {
                    /*
                    I128_UNWRAP_GC(pGC, ops);
                    (*pGC->ops->PolyArc)(pDraw, pGC, 1, arc);
                    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
                    */
                    miZeroPolyArc(pDraw, pGC, 1, arc);
                }
            }
        }
        I128_CLIP(i128Priv->clip);
        
    }
    else
    {
        _sPolyArc:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyArc)(pDraw, pGC, nArcs, pArcs);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }

}



static void
_SolidFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int shape;
    int mode;
    int nPoints;
    DDXPointPtr pPoints;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;

#if 0    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        /* miFillPolygons calls FillSpans - don't need to do
           any clipping here */
        miFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints);
    }
    else
#endif
    {
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, nPoints, pPoints);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}

static void
_SolidPolyFillRect(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    register unsigned int nRects;
    register xRectangle *pRects;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
    int numClipRects;
    GCOps *ops;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxRec clip;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;

        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        if (clip.x2 <= clip.x1 || clip.y2 <= clip.y1)
            return;

        I128_CLIP(clip);
        i128Priv->engine->plane_mask = 
            I128_CONVERT(i128Priv->mode.pixelsize, pGC->planemask);
        i128Priv->engine->foreground = pGC->fgPixel;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
        i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                 i128Priv->rop[pGC->alu] |
                                 I128_CMD_CLIP_IN |
                                 I128_CMD_SOLID);
     
        while (nRects--)
        {
            if (pRects->width && pRects->height)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                i128Priv->engine->xy2 = I128_XY(pRects->width,
                                                pRects->height);
                i128Priv->engine->xy1 = I128_XY(x0 + pRects->x,
                                                y0 + pRects->y);
            }
            pRects++;
        }

        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sPolyFillRect:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}


static void
_SolidPolyFillArc(pDraw, pGC, nArcs, pArcs)
    DrawablePtr pDraw;
    GCPtr pGC;
    int nArcs;
    xArc *pArcs;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        miPolyFillArc(pDraw, pGC, nArcs, pArcs);
    }
    else
    {
        _sPolyFillArc:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyFillArc)(pDraw, pGC, nArcs, pArcs);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}


static int
_SolidPolyText8(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    char *string;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    int ret;

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;
        
        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        if (NFB_FONT_PRIV(pGC->font)->fontInfo.isTE8Font)
        {
            nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
            nfbScrnPrivPtr pScrPriv = NFB_SCREEN_PRIV(pDraw->pScreen);
            FontRec *pFont = pGC->font;
            unsigned long int fg = pGCPriv->hwRop.fg;
            unsigned long int bg = pGCPriv->hwRop.bg;
            unsigned long int planemask = pGCPriv->hwRop.planemask;
            unsigned char alu = pGCPriv->hwRop.alu;

            nfbFontPSPtr pFontPS;
            int w, h, nbox;
            unsigned short glyphWidth;
            BoxRec bbox;
            BoxPtr extents, pbox;

            if (count <= 0)
                return x;

            pFontPS = NFB_FONT_PS(pFont, pScrPriv);
            glyphWidth = FONT_MAX_WIDTH(&pFont->info);
            w = glyphWidth * count;
            h = y - FONTASCENT(pFont);
            bbox.x1 = pDraw->x + x;
            bbox.y1 = pDraw->y + h;
            bbox.x2 = bbox.x1 + w;
            bbox.y2 = bbox.y1 + FONT_MAX_HEIGHT(&pFont->info);

            if (bbox.x1 > clip.x1)
                clip.x1 = bbox.x1;
            if (bbox.x2 < clip.x2)
                clip.x2 = bbox.x2;
            if (bbox.y1 > clip.y1)
                clip.y1 = bbox.y1;
            if (bbox.y2 < clip.y2)
                clip.y2 = bbox.y2;

            if ((bbox.x1 < bbox.x2) && (bbox.y1 < bbox.y2) &&
                (clip.x1 < clip.x2) && (clip.y1 < clip.y2))
            {
                I128_CLIP(clip);
                i128DrawFontText(&bbox, (unsigned char *)string, count,
                                 pFontPS, fg, bg, alu, planemask, TRUE, pDraw);
                ret = x + w;	
            }
            else
                ret = x;
        }
        else
        {
#ifdef NOT_YET
            register CharInfoPtr *charinfo;
            unsigned long n, i;
            int w;

            if(!(charinfo = (CharInfoPtr *)
                 ALLOCATE_LOCAL(count*sizeof(CharInfoPtr ))))
                ret = x ;
            else
            {
                int max_w = 0, max_h = 0;
                GetGlyphs(pGC->font,
                          (unsigned long)count,
                          (unsigned char *)string,
                          Linear8Bit, &n, charinfo);
                w = 0;
                for (i=0; i < n; i++)
                {
                    int _w, _h;

                    _w = charinfo[i]->metrics.rightSideBearing -
                        charinfo[i]->metrics.leftSideBearing;
                    _h = charinfo[i]->metrics.ascent +
                        charinfo[i]->metrics.descent;
                    w += charinfo[i]->metrics.characterWidth;
                    if (max_w < _w)
                        max_w = _w;
                    if (max_h < _h)
                        max_h = _h;
                }
                if (n != 0)
                {
                    I128_CLIP(clip);
                    i128PolyGlyphBlt(pDraw, pGC,
                                     x, y, w, max_w, max_h,
                                     n, charinfo,
                                     FONTGLYPHS(pGC->font),
                                     TRUE);
                }

                DEALLOCATE_LOCAL(charinfo);
                ret = x+w;
            }
#else
            goto _sPolyText8;
#endif            
        }
        I128_CLIP(i128Priv->clip);
    }
    else
    {
        _sPolyText8:
        I128_UNWRAP_GC(pGC, ops);
        ret = (*pGC->ops->PolyText8)(pDraw, pGC, x, y, count, string);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
    return ret;
}


static int
_SolidPolyText16(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    unsigned short *string;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    int ret;

#ifdef NOT_YET

    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register CharInfoPtr *charinfo;
        unsigned long n, i;
        int w;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;
        
        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        if(!(charinfo = (CharInfoPtr *)
             ALLOCATE_LOCAL(count*sizeof(CharInfoPtr ))))
            ret = x ;
        else
        {
            int max_w = 0, max_h = 0;
            int dx = 0, dy = 0;

            GetGlyphs(pGC->font,
                      (unsigned long)count,
                      (unsigned char *)string,
                      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
                      &n, charinfo);

            w = 0;
            if (n != 0)
            {
                max_w = charinfo[0]->metrics.rightSideBearing -
                    charinfo[0]->metrics.leftSideBearing;
                max_h = charinfo[0]->metrics.ascent +
                    charinfo[0]->metrics.descent;
            }
                

            for (i=0; i < n; i++)
            {
                int _w, _h;

                _w = charinfo[i]->metrics.rightSideBearing -
                    charinfo[i]->metrics.leftSideBearing;
                _h = charinfo[i]->metrics.ascent +
                    charinfo[i]->metrics.descent;
                w += charinfo[i]->metrics.characterWidth;
                if (max_w != _w)
                    dx = 1;
                if (max_h != _h)
                    dy = 1;
                if (max_w < _w)
                    max_w = _w;
                if (max_h < _h)
                    max_h = _h;
            }

            if (n != 0)
            {
                I128_CLIP(clip);
                if (dx || dy || max_w != w/n)
                    i128PolyGlyphBlt(pDraw, pGC,
                                     x, y, w, max_w, max_h,
                                     n, charinfo,
                                     FONTGLYPHS(pGC->font),
                                     TRUE);
                else
                    i128PolyConstantGlyphBlt(pDraw, pGC,
                                             x, y, w, max_w, max_h,
                                             n, charinfo,
                                             FONTGLYPHS(pGC->font),
                                             TRUE);
            }
            DEALLOCATE_LOCAL(charinfo);
            ret = x+w;
        }
        I128_CLIP(i128Priv->clip);
    }
    else
#endif /* NOT_YET */
    {
        _sPolyText16:
        I128_UNWRAP_GC(pGC, ops);
        ret = (*pGC->ops->PolyText16)(pDraw, pGC, x, y, count, string);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
    return ret;
    
}


static void
_SolidImageText8(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    char *string;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;


    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;
        
        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        if (NFB_FONT_PRIV(pGC->font)->fontInfo.isTE8Font)
        {
            nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
            nfbScrnPrivPtr pScrPriv = NFB_SCREEN_PRIV(pDraw->pScreen);
            FontRec *pFont = pGC->font;
            unsigned long int fg = pGCPriv->hwRop.fg;
            unsigned long int bg = pGCPriv->hwRop.bg;
            unsigned long int planemask = pGCPriv->hwRop.planemask;
            unsigned char alu = pGCPriv->hwRop.alu;

            nfbFontPSPtr pFontPS;
            int w, h, nbox;
            unsigned short glyphWidth;
            BoxRec bbox;
            BoxPtr extents, pbox;

            if (count <= 0)
                return;

            pFontPS = NFB_FONT_PS(pFont, pScrPriv);
            glyphWidth = FONT_MAX_WIDTH(&pFont->info);
            w = glyphWidth * count;
            h = y - FONTASCENT(pFont);
            bbox.x1 = pDraw->x + x;
            bbox.y1 = pDraw->y + h;
            bbox.x2 = bbox.x1 + w;
            bbox.y2 = bbox.y1 + FONT_MAX_HEIGHT(&pFont->info);

            if (bbox.x1 > clip.x1)
                clip.x1 = bbox.x1;
            if (bbox.x2 < clip.x2)
                clip.x2 = bbox.x2;
            if (bbox.y1 > clip.y1)
                clip.y1 = bbox.y1;
            if (bbox.y2 < clip.y2)
                clip.y2 = bbox.y2;

            if ((bbox.x1 < bbox.x2) && (bbox.y1 < bbox.y2) &&
                (clip.x1 < clip.x2) && (clip.y1 < clip.y2))
            {
                I128_CLIP(clip);
                i128DrawFontText(&bbox, (unsigned char *)string, count,
                                 pFontPS, fg, bg, alu, planemask, FALSE,
                                 pDraw);
            }
        }
        else
        {
#ifdef NOT_YET
            register CharInfoPtr *charinfo;
            unsigned long n, i;
            int w;

            if((charinfo = (CharInfoPtr *)
                 ALLOCATE_LOCAL(count*sizeof(CharInfoPtr ))))
            {
                int max_w = 0, max_h = 0;
                GetGlyphs(pGC->font,
                          (unsigned long)count,
                          (unsigned char *)string,
                          Linear8Bit, &n, charinfo);
                w = 0;
                for (i=0; i < n; i++)
                {
                    int _w, _h;
                    
                    _w = charinfo[i]->metrics.rightSideBearing -
                        charinfo[i]->metrics.leftSideBearing;
                    _h = charinfo[i]->metrics.ascent +
                        charinfo[i]->metrics.descent;
                    w += charinfo[i]->metrics.characterWidth;
                    if (max_w < _w)
                        max_w = _w;
                    if (max_h < _h)
                        max_h = _h;
                }
                if (n != 0)
                {
                    I128_CLIP(clip);
                    i128PolyGlyphBlt(pDraw, pGC,
                                     x, y, w, max_w, max_h,
                                     n, charinfo,
                                     FONTGLYPHS(pGC->font),
                                     FALSE);
                }
                DEALLOCATE_LOCAL(charinfo);
            }
#else
            goto _sImageText8;
#endif /* NOT_YET */
        }
        
        I128_CLIP(i128Priv->clip);

    }
    else
    {
        _sImageText8:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->ImageText8)(pDraw, pGC, x, y, count, string);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }

}


static void
_SolidImageText16(pDraw, pGC, x, y, count, string)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int count;
    unsigned short *string;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;
    GCOps *ops;
    int ret;

#ifdef NOT_YET
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register CharInfoPtr *charinfo;
        unsigned long n, i;
        int w;

        if (pextent->x1 > i128Priv->clip.x1)
            clip.x1 = pextent->x1;
        else
            clip.x1 = i128Priv->clip.x1;
        
        if (pextent->x2 < i128Priv->clip.x2)
            clip.x2 = pextent->x2;
        else
            clip.x2 = i128Priv->clip.x2;
        
        if (pextent->y1 > i128Priv->clip.y1)
            clip.y1 = pextent->y1;
        else
            clip.y1 = i128Priv->clip.y1;
        
        if (pextent->y2 < i128Priv->clip.y2)
            clip.y2 = pextent->y2;
        else
            clip.y2 = i128Priv->clip.y2;

        if(!(charinfo = (CharInfoPtr *)
             ALLOCATE_LOCAL(count*sizeof(CharInfoPtr ))))
            ret = x ;
        else
        {
            int max_w = 0, max_h = 0;

            GetGlyphs(pGC->font,
                      (unsigned long)count,
                      (unsigned char *)string,
                      (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
                      &n, charinfo);

            w = 0;
            for (i=0; i < n; i++)
            {
                int _w, _h;

                _w = charinfo[i]->metrics.rightSideBearing -
                    charinfo[i]->metrics.leftSideBearing;
                _h = charinfo[i]->metrics.ascent +
                    charinfo[i]->metrics.descent;
                w += charinfo[i]->metrics.characterWidth;
                if (max_w < _w)
                    max_w = _w;
                if (max_h < _h)
                    max_h = _h;
            }
            if (n != 0)
            {
                I128_CLIP(clip);
                i128PolyGlyphBlt(pDraw, pGC,
                                 x, y, w, max_w, max_h,
                                 n, charinfo,
                                 FONTGLYPHS(pGC->font),
                                 FALSE);
            }
            DEALLOCATE_LOCAL(charinfo);
            ret = x+w;
        }
        I128_CLIP(i128Priv->clip);
    }
    else
#endif /* NOT_YET */
    {
        _sImageText16:
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->ImageText16)(pDraw, pGC, x, y, count, string);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}


static void
_SolidImageGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    unsigned int nGlyphs;
    CharInfoPtr *ppCharInfo;
    unsigned char *pGlyphBase;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->ImageGlyphBlt)(pDraw, pGC,
                               x, y, nGlyphs,
                               ppCharInfo,
                               pGlyphBase);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
_SolidPolyGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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


i128PolyGlyphBlt(pDraw, pGC, x, y, string_w, max_w, max_h,
                 nGlyphs, ppCharInfo, pGlyphBase, transparent)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int string_w, max_w, max_h;
    unsigned int nGlyphs;
    CharInfoPtr *ppCharInfo;
    unsigned char *pGlyphBase;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    nfbScrnPriv *nfbPriv = NFB_SCREEN_PRIV(pDraw->pScreen);
    GCOps *ops;
    ExtentInfoRec info;	/* used by QueryGlyphExtents() */
    BoxRec bbox;	/* string's bounding box */
    register CharInfoPtr pci;
    /* these are used for placing the glyph */
    int w;		/* width of glyph in bits */
    int h;		/* height of glyph */
    int stride;	/* width of glyph, in bytes */
    BoxPtr pbox;
    unsigned long int planemask = pGCPriv->rRop.planemask;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int bg = pGCPriv->rRop.bg;
    unsigned char alu = pGCPriv->rRop.alu;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned char *src, *image;
    int i, j;
    int num_longs;
    unsigned long buf_control, cmd;
    long double_buffer_offset = 0;
    long double_xy0_value = I128_CACHE_SIZE/2 << 16;
    long xy0 = 0;
    int x0 = pDraw->x;
    int y0 = pDraw->y;
    int is_busy = 0;

    x += x0;
    y += y0;        

    buf_control = i128Priv->engine->buf_control;
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_PATT_RESET |
            I128_CMD_STIPPLE_PACK32 |
            I128_CMD_TRANSPARENT);
    
    if (transparent == FALSE)
    {
        int w, h;
        BoxRec backbox;
        ExtentInfoRec info;

        backbox.y1 = y - FONTASCENT(pGC->font);
        backbox.y2 = y + FONTDESCENT(pGC->font);
        backbox.x1 = backbox.x2 = x;	

	if (string_w >= 0)	
	    backbox.x2 += string_w;
	else
	    backbox.x1 += string_w;

        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->foreground = bg;
        i128Priv->engine->plane_mask = ~0;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
        i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                 i128Priv->rop[GXcopy] |
                                 I128_CMD_CLIP_IN |
                                 I128_CMD_SOLID);
        
        w = backbox.x2 - backbox.x1;
        h = backbox.y2 - backbox.y1;
        I128_BLIT(0, 0, backbox.x1, backbox.y1, w, h, I128_DIR_LR_TB);
        I128_WAIT_UNTIL_READY(i128Priv->engine);
    }

    i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
        (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
            (buf_control & I128_FLAG_NO_BPP);
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->xy0 = I128_XY(0, 0);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

    if (max_w <= I128_MAX_WIDTH && max_h <= I128_MAX_HEIGHT/2)
    {
        int width, height, x1, y1;
        register unsigned long *d0, *d1;
        register int rb, lb, ascent;
        volatile char *busy = (char*)&i128Priv->engine->busy;

        d0 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer);
        d1 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer +
                 I128_CACHE_SIZE/2);

        while (nGlyphs > 1)
        {
            nGlyphs -= 2;
            pci = *ppCharInfo++;
            rb = pci->metrics.rightSideBearing;
            lb = pci->metrics.leftSideBearing;
            ascent = pci->metrics.ascent;
            width = rb - lb;
            height = ascent + pci->metrics.descent;
            stride = GLYPHWIDTHBYTESPADDED(pci);

            /* figure out glyph's real coordinates */
            x1 = x + lb;
            y1 = y - ascent;
            x += pci->metrics.characterWidth;
            src = (unsigned char*)pci->bits;

            while (*busy);
            for (j=0; j<height; j++)
            {
                d0[j] = *(unsigned long *)src;
                src += stride;
            }

            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy2 = I128_XY(width, height);
            i128Priv->engine->xy1 = I128_XY(x1, y1);

            /****/
            pci = *ppCharInfo++;
            rb = pci->metrics.rightSideBearing;
            lb = pci->metrics.leftSideBearing;
            ascent = pci->metrics.ascent;
            width = rb - lb;
            height = ascent + pci->metrics.descent;
            stride = GLYPHWIDTHBYTESPADDED(pci);

            /* figure out glyph's real coordinates */
            x1 = x + lb;
            y1 = y - ascent;
            x += pci->metrics.characterWidth;
            src = (unsigned char*)pci->bits;

            while (*busy);
            for (j=0; j<height; j++)
            {
                d1[j] = *(unsigned long *)src;
                src += stride;
            }

            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
            i128Priv->engine->xy2 = I128_XY(width, height);
            i128Priv->engine->xy1 = I128_XY(x1, y1);

        }

        if (nGlyphs == 1)
        {
            pci = *ppCharInfo++; 
            width = pci->metrics.rightSideBearing -
                pci->metrics.leftSideBearing;
            height = pci->metrics.ascent + pci->metrics.descent;
            stride = GLYPHWIDTHBYTESPADDED(pci);

            /* figure out glyph's real coordinates */
            x1 = x + pci->metrics.leftSideBearing;
            y1 = y - pci->metrics.ascent;
            src = (unsigned char*)pci->bits;

            while (*busy);
            for (j=0; j<height; j++)
            {
                d0[j] = *((unsigned long*)src);
                src += stride;
            }
            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy2 = I128_XY(width, height);
            i128Priv->engine->xy1 = I128_XY(x1, y1);
        }

    }
    else if (max_w <= I128_MAX_WIDTH && max_h <= I128_MAX_HEIGHT)
    {
        int width, height, x1, y1, half_height;
        register unsigned long *d0, *d1;
        unsigned char * s0;
        volatile char *busy = (volatile char*)&i128Priv->engine->busy;

        d0 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer);
                
        while (nGlyphs--)
        {
            pci = *ppCharInfo++;
            width = pci->metrics.rightSideBearing -
                pci->metrics.leftSideBearing;
            height = pci->metrics.ascent + pci->metrics.descent;
            stride = GLYPHWIDTHBYTESPADDED(pci);

            /* figure out glyph's real coordinates */
            x1 = x + pci->metrics.leftSideBearing;
            y1 = y - pci->metrics.ascent;
            src = (unsigned char*)pci->bits;

            while(*busy) is_busy++;
            for (j=0; j<height; j++)
            {
                d0[j] = *(unsigned long*)src;
                src += stride;
            }
            i128Priv->engine->xy2 = I128_XY(width, height);
            i128Priv->engine->xy1 = I128_XY(x1, y1);

            x += pci->metrics.characterWidth;
        }
    }
    else while(nGlyphs--)
    {
        int width, height;
        int num_strips, num_chunks, chunk;
        int w, h, x1, y1;
        unsigned char *src, *image;

        pci = *ppCharInfo++; 
        width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
        height = pci->metrics.ascent + pci->metrics.descent;
        stride = GLYPHWIDTHBYTESPADDED(pci);
        num_strips = (width + I128_MAX_WIDTH - 1) >> 5;
        num_chunks = (height + I128_MAX_HEIGHT - 1) >> 5;
        width &= 0x1F;
        if (width == 0)
            width = I128_MAX_WIDTH;
        height &= 0x1F;
        if (height == 0)
            height = I128_MAX_HEIGHT;
        
        /* figure out glyph's real coordinates */
        x1 = x + pci->metrics.leftSideBearing;
        y1 = y - pci->metrics.ascent;
        image = (unsigned char*)pci->bits;
        
        while (num_strips--)
        {
            register int y;
      
            chunk = num_chunks;
            y = y1;
            w = num_strips > 0 ? I128_MAX_WIDTH : width;
            src = image;
            chunk = num_chunks;
                    
            while (chunk--)
            {
                h = chunk > 0 ? I128_MAX_HEIGHT : height;
                I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *(unsigned long*)src;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += I128_MAX_HEIGHT;
            }
            image += 4;
            x1 += I128_MAX_WIDTH;
        }
        
        /* update character origin */
        x += pci->metrics.characterWidth;
        
    } /* while nGlyphs-- */
    
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;

}

#define I128_COPY_14X(_d, _h) \
_d[_h] = (unsigned long)((src1[_h] << 14) | src0[_h])

i128PolyConstantGlyphBlt(pDraw, pGC, x, y, string_w, max_w, max_h,
                         nGlyphs, ppCharInfo, pGlyphBase, transparent)
    DrawablePtr pDraw;
    GCPtr pGC;
    int x, y;
    int string_w, max_w, max_h;
    unsigned int nGlyphs;
    CharInfoPtr *ppCharInfo;
    unsigned char *pGlyphBase;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    nfbScrnPriv *nfbPriv = NFB_SCREEN_PRIV(pDraw->pScreen);
    GCOps *ops;
    ExtentInfoRec info;	/* used by QueryGlyphExtents() */
    BoxRec bbox;	/* string's bounding box */
    register CharInfoPtr pci;
    /* these are used for placing the glyph */
    int w;		/* width of glyph in bits */
    int h;		/* height of glyph */
    int stride;	/* width of glyph, in bytes */
    BoxPtr pbox;
    unsigned long int planemask = pGCPriv->rRop.planemask;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int bg = pGCPriv->rRop.bg;
    unsigned char alu = pGCPriv->rRop.alu;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned char *src, *src0, *src1, *image;
    int i, j;
    int num_longs;
    unsigned long buf_control, cmd;
    long double_buffer_offset = 0;
    long double_xy0_value = I128_CACHE_SIZE/2 << 16;
    long xy0 = 0;
    int x0 = pDraw->x;
    int y0 = pDraw->y;

    x += x0;
    y += y0;        

    buf_control = i128Priv->engine->buf_control;
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_PATT_RESET |
            I128_CMD_STIPPLE_PACK32 |
            I128_CMD_TRANSPARENT);
    
    if (transparent == FALSE)
    {
        int w, h;
        BoxRec backbox;
        ExtentInfoRec info;

        backbox.y1 = y - FONTASCENT(pGC->font);
        backbox.y2 = y + FONTDESCENT(pGC->font);
        backbox.x1 = backbox.x2 = x;	

	if (string_w >= 0)	
	    backbox.x2 += string_w;
	else
	    backbox.x1 += string_w;

        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->foreground = bg;
        i128Priv->engine->plane_mask = ~0;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
        i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                 i128Priv->rop[GXcopy] |
                                 I128_CMD_CLIP_IN |
                                 I128_CMD_SOLID);
        
        w = backbox.x2 - backbox.x1;
        h = backbox.y2 - backbox.y1;
        I128_BLIT(0, 0, backbox.x1, backbox.y1, w, h, I128_DIR_LR_TB);
        I128_WAIT_UNTIL_READY(i128Priv->engine);
    }

    i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
        (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
            (buf_control & I128_FLAG_NO_BPP);
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->background = bg;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->xy0 = I128_XY(0, 0);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    I128_WAIT_UNTIL_DONE(i128Priv->engine);

    if (max_w <= I128_MAX_WIDTH && max_h <= I128_MAX_HEIGHT/2) /* double buffer */
    {
        int width, height, x1, y1;
        register unsigned long *d0, *d1;
        register int rb, lb, ascent;

        d0 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer);
        d1 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer +
                 I128_CACHE_SIZE/2);

        if (nGlyphs)
        {
            pci = *ppCharInfo;
            ascent = pci->metrics.ascent;
            stride = GLYPHWIDTHBYTESPADDED(pci);
            y -= ascent;
        }

        if (max_w <= I128_MAX_WIDTH/2) 
        {
            width = 2 * max_w;
            switch (max_h)
            {
              case 14:
                while (nGlyphs > 3)
                {
                    unsigned short *src0, *src1;

                    nGlyphs -= 4;

                    pci = *ppCharInfo++;
                    src0 = (unsigned short*)pci->bits;

                    pci = *ppCharInfo++;
                    src1 = (unsigned short*)pci->bits;

                    I128_COPY_14X(d0, 0);

                    I128_COPY_14X(d0, 1);
                    I128_COPY_14X(d0, 2);
                    I128_COPY_14X(d0, 3);
                    I128_COPY_14X(d0, 4);
                    I128_COPY_14X(d0, 5);
                    I128_COPY_14X(d0, 6);
                    I128_COPY_14X(d0, 7);
                    I128_COPY_14X(d0, 8);
                    I128_COPY_14X(d0, 9);
                    I128_COPY_14X(d0, 10);
                    I128_COPY_14X(d0, 11);
                    I128_COPY_14X(d0, 12);
                    I128_COPY_14X(d0, 13);

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                    i128Priv->engine->xy0 = 0;
                    i128Priv->engine->xy2 = I128_XY(width, max_h);
                    i128Priv->engine->xy1 = I128_XY(x, y);
                    x += width;

                    /****/

                    pci = *ppCharInfo++;
                    src0 = (unsigned short*)pci->bits;

                    pci = *ppCharInfo++;
                    src1 = (unsigned short*)pci->bits;

                    I128_COPY_14X(d1, 0);
                    I128_COPY_14X(d1, 1);
                    I128_COPY_14X(d1, 2);
                    I128_COPY_14X(d1, 3);
                    I128_COPY_14X(d1, 4);
                    I128_COPY_14X(d1, 5);
                    I128_COPY_14X(d1, 6);
                    I128_COPY_14X(d1, 7);
                    I128_COPY_14X(d1, 8);
                    I128_COPY_14X(d1, 9);
                    I128_COPY_14X(d1, 10);
                    I128_COPY_14X(d1, 11);
                    I128_COPY_14X(d1, 12);
                    I128_COPY_14X(d1, 13);

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                    i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                    i128Priv->engine->xy2 = I128_XY(width, max_h);
                    i128Priv->engine->xy1 = I128_XY(x, y);
                    x += width;
                }
                break;
                
              default:
                while (nGlyphs > 3)
                {
                    nGlyphs -= 4;
                
                    pci = *ppCharInfo++;
                    src0 = (unsigned char*)pci->bits;

                    pci = *ppCharInfo++;
                    src1 = (unsigned char*)pci->bits;

                    for (j=0; j<max_h; j++)
                    {
                        d0[j] = *(unsigned long*)src1 << max_w |
                            *(unsigned long*)src0;
                        src0 += stride;
                        src1 += stride;
                    }

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                    i128Priv->engine->xy0 = 0;
                    i128Priv->engine->xy2 = I128_XY(width, max_h);
                    i128Priv->engine->xy1 = I128_XY(x, y);
                    x += width;

                    /****/

                    pci = *ppCharInfo++;
                    src0 = (unsigned char*)pci->bits;

                    pci = *ppCharInfo++;
                    src1 = (unsigned char*)pci->bits;

                    for (j=0; j<max_h; j++)
                    {
                        d1[j] = *(unsigned long*)src1 << max_w |
                            *(unsigned long*)src0;
                        src0 += stride;
                        src1 += stride;
                    }

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                    i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                    i128Priv->engine->xy2 = I128_XY(width, max_h);
                    i128Priv->engine->xy1 = I128_XY(x, y);
                    x += width;
                }
                break;
            }
        }
        else
        {
            width = max_w;
            while (nGlyphs > 1)
            {
                nGlyphs -= 2;
                
                pci = *ppCharInfo++;
                src0 = (unsigned char*)pci->bits;

                I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                for (j=0; j<max_h; j++)
                {
                    d0[j] = *(unsigned long*)src0;
                    src0 += stride;
                }

                i128Priv->engine->xy0 = 0;
                i128Priv->engine->xy2 = I128_XY(width, max_h);
                i128Priv->engine->xy1 = I128_XY(x, y);
                x += width;

                /****/

                pci = *ppCharInfo++;
                src0 = (unsigned char*)pci->bits;

                I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                for (j=0; j<max_h; j++)
                {
                    d1[j] = *(unsigned long*)src0;
                    src0 += stride;
                }
                i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                i128Priv->engine->xy2 = I128_XY(width, max_h);
                i128Priv->engine->xy1 = I128_XY(x, y);
                x += width;
            }
        }
        
        I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
        
        while (nGlyphs > 0)
        {
            nGlyphs--;
            pci = *ppCharInfo++; 
            stride = GLYPHWIDTHBYTESPADDED(pci);
            src = (unsigned char*)pci->bits;

            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
            for (j=0; j<max_h; j++)
            {
                d0[j] = *((unsigned long*)src);
                src += stride;
            }

            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy2 = I128_XY(max_w, max_h);
            i128Priv->engine->xy1 = I128_XY(x, y);
            x += max_w;
        }

    }
    else if (max_w <= I128_MAX_WIDTH && max_h <= I128_MAX_HEIGHT) /* single buffer */
    {
        int width, height, x1, y1, half_height;
        register unsigned long *d0, *d1;
        unsigned char * s0;
        volatile char *busy = (char*)&i128Priv->engine->busy;

        d0 = (unsigned long*)
                ((unsigned char *)i128Priv->info.xy_win[0].pointer);
                
        while (nGlyphs--)
        {
            pci = *ppCharInfo++;
            width = pci->metrics.rightSideBearing -
                pci->metrics.leftSideBearing;
            height = pci->metrics.ascent + pci->metrics.descent;
            stride = GLYPHWIDTHBYTESPADDED(pci);

            /* figure out glyph's real coordinates */
            x1 = x + pci->metrics.leftSideBearing;
            y1 = y - pci->metrics.ascent;
            src = (unsigned char*)pci->bits;

            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
            for (j=0; j<height; j++)
            {
                d0[j] = *(unsigned long*)src;
                src += stride;
            }
            i128Priv->engine->xy2 = I128_XY(width, height);
            i128Priv->engine->xy1 = I128_XY(x1, y1);

            x += pci->metrics.characterWidth;
        }
    }
    else while(nGlyphs--)
    {
        int width, height;
        int num_strips, num_chunks, chunk;
        int w, h, x1, y1;
        unsigned char *src, *image;

        pci = *ppCharInfo++; 
        width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
        height = pci->metrics.ascent + pci->metrics.descent;
        stride = GLYPHWIDTHBYTESPADDED(pci);
        num_strips = (width + I128_MAX_WIDTH - 1) >> 5;
        num_chunks = (height + I128_MAX_HEIGHT - 1) >> 5;
        width &= 0x1F;
        if (width == 0)
            width = I128_MAX_WIDTH;
        height &= 0x1F;
        if (height == 0)
            height = I128_MAX_HEIGHT;
        
        /* figure out glyph's real coordinates */
        x1 = x + pci->metrics.leftSideBearing;
        y1 = y - pci->metrics.ascent;
        image = (unsigned char*)pci->bits;
        
        while (num_strips--)
        {
            register int y;
      
            chunk = num_chunks;
            y = y1;
            w = num_strips > 0 ? I128_MAX_WIDTH : width;
            src = image;
            chunk = num_chunks;
                    
            while (chunk--)
            {
                h = chunk > 0 ? I128_MAX_HEIGHT : height;
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *(unsigned long*)src;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += I128_MAX_HEIGHT;
            }
            image += 4;
            x1 += I128_MAX_WIDTH;
        }
        
        /* update character origin */
        x += pci->metrics.characterWidth;
        
    } /* while nGlyphs-- */
    
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;

}



static void
_SolidPushPixels(pGC, pBitmap, pDraw, width, height, x, y)
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
_SolidStub()
{
}


#endif /* I128_FAST_GC_OPS */
