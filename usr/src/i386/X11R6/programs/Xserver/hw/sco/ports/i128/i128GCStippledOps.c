/*
 *	@(#) i128GCStippledOps.c 11.1 97/10/22
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
 * i128GCStippledOps.c
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

static void _StippledFillSpans();
static void _StippledSetSpans();
static void _StippledPutImage();
static RegionPtr _StippledCopyArea();
static RegionPtr _StippledCopyPlane();
static void _StippledPolyPoint();
static void _StippledPolylines();
static void _StippledPolySegment();
static void _StippledPolyRectangle();
static void _StippledPolyArc();
static void _StippledFillPolygon();
static void _StippledPolyFillRect();
static void _StippledPolyFillArc();
static int _StippledPolyText8();
static int _StippledPolyText16();
static void _StippledImageText8();
static void _StippledImageText16();
static void _StippledImageGlyphBlt();
static void _StippledPolyGlyphBlt();
static void _StippledPushPixels();
static void _StippledStub();

GCOps i128GCStippledOps = {
     _StippledFillSpans,      /*  void (* FillSpans)() */
     _StippledSetSpans,       /*  void (* SetSpans)()  */
     _StippledPutImage,       /*  void (* PutImage)()  */
     _StippledCopyArea,       /*  RegionPtr (* CopyArea)()     */
     _StippledCopyPlane,      /*  RegionPtr (* CopyPlane)() */
     _StippledPolyPoint,      /*  void (* PolyPoint)() */
     _StippledPolylines,      /*  void (* Polylines)() */
     _StippledPolySegment,    /*  void (* PolySegment)() */
     _StippledPolyRectangle,  /*  void (* PolyRectangle)() */
     _StippledPolyArc,        /*  void (* PolyArc)()   */
     _StippledFillPolygon,    /*  void (* FillPolygon)() */
     _StippledPolyFillRect,   /*  void (* PolyFillRect)() */
     _StippledPolyFillArc,    /*  void (* PolyFillArc)() */
     _StippledPolyText8,      /*  int (* PolyText8)()  */
     _StippledPolyText16,     /*  int (* PolyText16)() */
     _StippledImageText8,     /*  void (* ImageText8)() */
     _StippledImageText16,    /*  void (* ImageText16)() */
     _StippledImageGlyphBlt,  /*  void (* ImageGlyphBlt)() */
     _StippledPolyGlyphBlt,   /*  void (* PolyGlyphBlt)() */
     _StippledPushPixels,     /*  void (* PushPixels)() */
     _StippledStub,           /*  void (* Stub)() */
};

extern
GCFuncs i128GCFuncs;

static void
_StippledFillSpans(pDraw, pGC, nSpans, pPoints, pWidths, fSorted)
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
    register int x0, y0;
    int w, h, xoff, yoff;
    DDXPointPtr patOrg;
    PixmapPtr pStip;
    unsigned char *pimage;
    unsigned long cmd, buf_control;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int planemask;
    int stride;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;
    GCOps *ops;
    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        pStip = pGC->stipple;
        pimage = pStip->devPrivate.ptr;
        stride = pStip->devKind;
        w = pStip->drawable.width;
        h = pStip->drawable.height;
        patOrg = &(pGCPriv->screenPatOrg);
        planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                                 pGCPriv->rRop.planemask);

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
        if (I128_MAX_WIDTH % w || I128_MAX_HEIGHT % h)
        {
            int cache_w, cache_h;
            unsigned int cmd1, cmd2;
            cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
            cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;

            if (((2*w+1) > cache_w) || ((h) > cache_h))
            {
                I128_UNWRAP_GC(pGC, ops);
                (*pGC->ops->FillSpans)(pDraw, pGC, nSpans,
                                       pPoints, pWidths, fSorted);
                I128_WRAP_GC(pGC, &i128GCFuncs, ops);
            }
            else
            {
                BoxRec box, cache1, cache2;
                unsigned long bg = 0;

                switch (alu)
                {
                  case GXnoop:
                    break;

                  case GXclear:
                  case GXandReverse:
                  case GXnor:
                  case GXinvert:
                  case GXorReverse:
                  case GXnand:
                  case GXset:
                    I128_UNWRAP_GC(pGC, ops);
                    (*pGC->ops->FillSpans)(pDraw, pGC, nSpans,
                                           pPoints, pWidths, fSorted);
                    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
                    break;
                    
                  case GXcopy:
                  case GXcopyInverted:
                    /* (cache1 & dest) | cache2 == stippled GXcopy blit */
                    cache1.x1 = i128Priv->cache.x1;
                    cache1.x2 = cache1.x1 + w;
                    cache1.y1 = i128Priv->cache.y1;
                    cache1.y2 = cache1.y1 + h;
            
                    cache2.x1 = cache1.x2 + 1;
                    cache2.x2 = cache2.x1 + w;
                    cache2.y1 = cache1.y1;
                    cache2.y2 = cache2.y1 + h;

                    cmd1 = (i128Priv->rop[GXand] |
                            I128_OPCODE_BITBLT |
                            I128_CMD_CLIP_IN);

                    cmd2 = (i128Priv->rop[GXor] |
                            I128_OPCODE_BITBLT |
                            I128_CMD_CLIP_IN);

                    if (alu == GXcopyInverted)
                        fg = ~fg;

                    I128_CLIP(cache1);
                    i128DrawOpaqueMonoImage(&cache1, (void*)pimage,
                                            0, stride, fg, ~0,
                                            GXcopy, ~0, pDraw);

                    I128_CLIP(cache2);
                    i128DrawOpaqueMonoImage(&cache2, (void*)pimage,
                                            0, stride, fg, 0,
                                            GXcopy, ~0, pDraw);
            
                    I128_CLIP(clip);
                    i128Priv->engine->plane_mask = planemask;
                    i128Priv->engine->xy3 = I128_DIR_LR_TB;
                    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                    while(nSpans--)
                    {
                        pPoints->x += x0;
                        pPoints->y += y0;
                        box.x1 = pPoints->x;
                        box.y1 = pPoints->y;

                        xoff = ( box.x1 - patOrg->x ) % w;
                        if ( xoff < 0 )
                            xoff += w;

                        yoff = ( box.y1 - patOrg->y ) % h;
                        if ( yoff < 0 )
                            yoff += h;

                        box.x2 = min( box.x1 + w - xoff, pPoints->x + *pWidths);
                        box.y2 = min( box.y1 + h - yoff, pPoints->y + 1);

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->cmd = cmd1;
                        I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->cmd = cmd2;
                        I128_SAFE_BLIT(cache2.x1 + xoff, cache2.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        while ( box.x2 < pPoints->x + *pWidths )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pPoints->x + *pWidths )
                                box.x2 = pPoints->x + *pWidths;

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd1;
                            I128_SAFE_BLIT(cache1.x1, cache1.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                    
                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd2;
                            I128_SAFE_BLIT(cache2.x1, cache2.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                        }
                        pPoints++;
                        pWidths++;
                    }
                    break;

                  case GXand:
                  case GXequiv:
                  case GXorInverted:
                    bg = ~0;

                  default:
                    cache1.x1 = i128Priv->cache.x1;
                    cache1.x2 = cache1.x1 + w;
                    cache1.y1 = i128Priv->cache.y1;
                    cache1.y2 = cache1.y1 + h;
            
                    I128_CLIP(cache1);
                    i128DrawOpaqueMonoImage(&cache1, (void*)pimage,
                                            0, stride, fg, bg,
                                            GXcopy, ~0, pDraw);
                    I128_CLIP(clip);
                    i128Priv->engine->cmd = (i128Priv->rop[alu] |
                                             I128_OPCODE_BITBLT |
                                             I128_CMD_CLIP_IN);
                    i128Priv->engine->plane_mask = planemask;
                    i128Priv->engine->xy3 = I128_DIR_LR_TB;
                    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                    while(nSpans--)
                    {
                        pPoints->x += x0;
                        pPoints->y += y0;
                        box.x1 = pPoints->x;
                        box.y1 = pPoints->y;

                        xoff = ( box.x1 - patOrg->x ) % w;
                        if ( xoff < 0 )
                            xoff += w;

                        yoff = ( box.y1 - patOrg->y ) % h;
                        if ( yoff < 0 )
                            yoff += h;

                        box.x2 = min( box.x1 + w - xoff, pPoints->x + *pWidths );
                        box.y2 = min( box.y1 + h - yoff, pPoints->y + 1);

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        while ( box.x2 < pPoints->x + *pWidths )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pPoints->x + *pWidths )
                                box.x2 = pPoints->x + *pWidths;

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            I128_SAFE_BLIT(cache1.x1, cache1.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                        }
                        pPoints++;
                        pWidths++;
                    }
                    break;
                }
            }
        }
        else
        {
            xoff = patOrg->x % w;
            yoff = patOrg->y % h;
            buf_control = i128Priv->engine->buf_control;
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN  |
                    I128_CMD_TRANSPARENT |
                    I128_CMD_STIPPLE_PACK32 |
                    I128_CMD_AREAPAT_32x32);

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
                (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
                    (buf_control & I128_FLAG_NO_BPP);     

            I128_WAIT_UNTIL_DONE(i128Priv->engine);
            if (xoff || yoff)
                for( i=0; i<h; i++ )
                {
                    register int j = (i + h - yoff) % h;
                    dst[j] = *(unsigned long*)pimage;
                    I128_REPLICATE_WIDTH(dst[j], w);
                    dst[j] = dst[j] << (32 - xoff) | dst[j] >> xoff;
                    pimage += stride;
                }
            else
                for( i=0; i<h; i++ )
                {
                    dst[i] = *(unsigned long*)pimage;
                    I128_REPLICATE_WIDTH(dst[i], w);
                    pimage += stride;
                }
            I128_REPLICATE_HEIGHT(dst, h); 

            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->foreground = fg;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

        
            while (nSpans--)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                i128Priv->engine->xy2 = I128_XY(*pWidths, 1);
                i128Priv->engine->xy1 = I128_XY(pPoints->x + x0,
                                                pPoints->y + y0);
                pPoints++;
                pWidths++;
            }

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->buf_control = buf_control;
        }
        I128_CLIP(i128Priv->clip);        
        I128_CLIP(i128Priv->clip);
    }
    else
    {
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->FillSpans)(pDraw, pGC, nSpans,
                               pPoints, pWidths, fSorted);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}



static void
_StippledSetSpans(pDraw, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
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
_StippledPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
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
_StippledCopyArea(pSrc, pDst, pGC, srcx, srcy, width, height, dstx, dsty)
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
_StippledCopyPlane(pSrc, pDst, pGC, srcx, srcy,
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
_StippledPolyPoint(pDraw, pGC, mode, nPoints, pPoints)
    DrawablePtr pDraw;
    GCPtr pGC;
    int mode;
    int nPoints;
    xPoint *pPoints;
{
    GCOps *ops;
    
    I128_UNWRAP_GC(pGC, ops);
    (*pGC->ops->PolyPoint)(pDraw, pGC, mode, nPoints, pPoints);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
_StippledPolylines(pDraw, pGC, mode, nPoints, pPoints)
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
_StippledPolySegment(pDraw, pGC, nSegments, pSegments)
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
_StippledPolyRectangle(pDraw, pGC, nRects, pRects)
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
_StippledPolyArc(pDraw, pGC, nArcs, pArcs)
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
_StippledFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints)
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
    
    numClipRects = REGION_NUM_RECTS(prgnClip);
    if (numClipRects == 1)
    {
        /* miFillPolygons calls FillSpans - don't need to do
           any clipping here */
        miFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints);
    }
    else
    {
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->FillPolygon)(pDraw, pGC, shape, mode, nPoints, pPoints);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}

static void
_StippledPolyFillRect(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nRects;
    xRectangle *pRects;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    GCOps *ops;
    register int x0, y0;
    int w, h, xoff, yoff;
    DDXPointPtr patOrg;
    PixmapPtr pStip;
    unsigned char *pimage;
    unsigned long cmd, buf_control;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int planemask;
    int stride;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;
    int numClipRects = REGION_NUM_RECTS(prgnClip);

    if (numClipRects == 1)
    {
        BoxPtr pextent = (*pGC->pScreen->RegionExtents)(prgnClip);
        BoxRec clip;
        register int x0 = pDraw->x;
        register int y0 = pDraw->y;

        pStip = pGC->stipple;
        pimage = pStip->devPrivate.ptr;
        stride = pStip->devKind;
        w = pStip->drawable.width;
        h = pStip->drawable.height;
        patOrg = &(pGCPriv->screenPatOrg);
        planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                                 pGCPriv->rRop.planemask);

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
        if (I128_MAX_WIDTH % w || I128_MAX_HEIGHT % h)
        {
            int cache_w, cache_h;
            unsigned int cmd1, cmd2;
            cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
            cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;

            if (((2*w+1) > cache_w) || ((h) > cache_h) ||
                ((pRects->width > I128_MAX_WIDTH) &&
                 (pRects->height > I128_MAX_HEIGHT)))
            {
                I128_UNWRAP_GC(pGC, ops);
                (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
                I128_WRAP_GC(pGC, &i128GCFuncs, ops);
            }
            else
            {
                BoxRec box, cache1, cache2;
                unsigned long bg = 0;

                switch (alu)
                {
                  case GXnoop:
                    break;

                  case GXclear:
                  case GXandReverse:
                  case GXnor:
                  case GXinvert:
                  case GXorReverse:
                  case GXnand:
                  case GXset:
                    I128_UNWRAP_GC(pGC, ops);
                    (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
                    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
                    break;
                    
                  case GXcopy:
                  case GXcopyInverted:
                    /* (cache1 & dest) | cache2 == stippled GXcopy blit */
                    cache1.x1 = i128Priv->cache.x1;
                    cache1.x2 = cache1.x1 + w;
                    cache1.y1 = i128Priv->cache.y1;
                    cache1.y2 = cache1.y1 + h;
            
                    cache2.x1 = cache1.x2 + 1;
                    cache2.x2 = cache2.x1 + w;
                    cache2.y1 = cache1.y1;
                    cache2.y2 = cache2.y1 + h;

                    cmd1 = (i128Priv->rop[GXand] |
                            I128_OPCODE_BITBLT |
                            I128_CMD_CLIP_IN);

                    cmd2 = (i128Priv->rop[GXor] |
                            I128_OPCODE_BITBLT |
                            I128_CMD_CLIP_IN);

                    if (alu == GXcopyInverted)
                        fg = ~fg;

                    I128_CLIP(cache1);
                    i128DrawOpaqueMonoImage(&cache1, (void*)pimage,
                                            0, stride, fg, ~0,
                                            GXcopy, ~0, pDraw);

                    I128_CLIP(cache2);
                    i128DrawOpaqueMonoImage(&cache2, (void*)pimage,
                                            0, stride, fg, 0,
                                            GXcopy, ~0, pDraw);
            
                    I128_CLIP(clip);
                    i128Priv->engine->plane_mask = planemask;
                    i128Priv->engine->xy3 = I128_DIR_LR_TB;
                    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                    while(nRects--)
                    {
                        pRects->x += x0;
                        pRects->y += y0;
                        box.x1 = pRects->x;
                        box.y1 = pRects->y;

                        xoff = ( box.x1 - patOrg->x ) % w;
                        if ( xoff < 0 )
                            xoff += w;

                        yoff = ( box.y1 - patOrg->y ) % h;
                        if ( yoff < 0 )
                            yoff += h;

                        box.x2 = min( box.x1 + w - xoff, pRects->x + pRects->width );
                        box.y2 = min( box.y1 + h - yoff, pRects->y + pRects->height );

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->cmd = cmd1;
                        I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->cmd = cmd2;
                        I128_SAFE_BLIT(cache2.x1 + xoff, cache2.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        while ( box.x2 < pRects->x + pRects->width )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pRects->x + pRects->width )
                                box.x2 = pRects->x + pRects->width;

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd1;
                            I128_SAFE_BLIT(cache1.x1, cache1.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                    
                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd2;
                            I128_SAFE_BLIT(cache2.x1, cache2.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                        }

                        while ( box.y2 < pRects->y + pRects->height )
                        {
                            box.y1 = box.y2;
                            box.y2 += h;
                            if ( box.y2 > pRects->y + pRects->height );
                            box.y2 = pRects->y + pRects->height;

                            box.x1 = pRects->x;
                            box.x2 = min(box.x1 + w - xoff,
                                         pRects->x + pRects->width );

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd1;
                            I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                    
                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            i128Priv->engine->cmd = cmd2;
                            I128_SAFE_BLIT(cache2.x1 + xoff, cache1.y1,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                    
                            while ( box.x2 < pRects->x + pRects->width )
                            {
                                box.x1 = box.x2;
                                box.x2 += w;
                                if ( box.x2 > pRects->x + pRects->width )
                                    box.x2 = pRects->x + pRects->width;

                                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                                i128Priv->engine->cmd = cmd1;
                                I128_SAFE_BLIT(cache1.x1, cache1.y1,
                                               box.x1, box.y1,
                                               box.x2 - box.x1, box.y2 - box.y1,
                                               I128_DIR_LR_TB);
                        
                                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                                i128Priv->engine->cmd = cmd2;
                                I128_SAFE_BLIT(cache2.x1, cache2.y1,
                                               box.x1, box.y1,
                                               box.x2 - box.x1, box.y2 - box.y1,
                                               I128_DIR_LR_TB);
                            }
                        }
                        pRects++;
                    }
                    break;

                  case GXand:
                  case GXequiv:
                  case GXorInverted:
                    bg = ~0;

                  default:
                    cache1.x1 = i128Priv->cache.x1;
                    cache1.x2 = cache1.x1 + w;
                    cache1.y1 = i128Priv->cache.y1;
                    cache1.y2 = cache1.y1 + h;
            
                    I128_CLIP(cache1);
                    i128DrawOpaqueMonoImage(&cache1, (void*)pimage,
                                            0, stride, fg, bg,
                                            GXcopy, ~0, pDraw);
                    I128_CLIP(clip);
                    i128Priv->engine->cmd = (i128Priv->rop[alu] |
                                             I128_OPCODE_BITBLT |
                                             I128_CMD_CLIP_IN);
                    i128Priv->engine->plane_mask = planemask;
                    i128Priv->engine->xy3 = I128_DIR_LR_TB;
                    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                    while(nRects--)
                    {
                        pRects->x += x0;
                        pRects->y += y0;
                        box.x1 = pRects->x;
                        box.y1 = pRects->y;

                        xoff = ( box.x1 - patOrg->x ) % w;
                        if ( xoff < 0 )
                            xoff += w;

                        yoff = ( box.y1 - patOrg->y ) % h;
                        if ( yoff < 0 )
                            yoff += h;

                        box.x2 = min( box.x1 + w - xoff, pRects->x + pRects->width );
                        box.y2 = min( box.y1 + h - yoff, pRects->y + pRects->height );

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);

                        while ( box.x2 < pRects->x + pRects->width )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pRects->x + pRects->width )
                                box.x2 = pRects->x + pRects->width;

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            I128_SAFE_BLIT(cache1.x1, cache1.y1 + yoff,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                        }

                        while ( box.y2 < pRects->y + pRects->height )
                        {
                            box.y1 = box.y2;
                            box.y2 += h;
                            if ( box.y2 > pRects->y + pRects->height )
                                box.y2 = pRects->y + pRects->height;

                            box.x1 = pRects->x;
                            box.x2 = min( box.x1 + w - xoff, pRects->x + pRects->width );

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                    
                            while ( box.x2 < pRects->x + pRects->width )
                            {
                                box.x1 = box.x2;
                                box.x2 += w;
                                if ( box.x2 > pRects->x + pRects->width )
                                    box.x2 = pRects->x + pRects->width;

                                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                                I128_SAFE_BLIT(cache1.x1, cache1.y1,
                                               box.x1, box.y1,
                                               box.x2 - box.x1, box.y2 - box.y1,
                                               I128_DIR_LR_TB);
                            }
                        }
                        pRects++;
                    }
                    break;
                }
            }
        }
        else
        {
            xoff = patOrg->x % w;
            yoff = patOrg->y % h;
            buf_control = i128Priv->engine->buf_control;
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN  |
                    I128_CMD_TRANSPARENT |
                    I128_CMD_STIPPLE_PACK32 |
                    I128_CMD_AREAPAT_32x32);

            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
                (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
                    (buf_control & I128_FLAG_NO_BPP);     

            I128_WAIT_UNTIL_DONE(i128Priv->engine);
            if (xoff || yoff)
                for( i=0; i<h; i++ )
                {
                    register int j = (i + h - yoff) % h;
                    dst[j] = *(unsigned long*)pimage;
                    I128_REPLICATE_WIDTH(dst[j], w);
                    dst[j] = dst[j] << (32 - xoff) | dst[j] >> xoff;
                    pimage += stride;
                }
            else
                for( i=0; i<h; i++ )
                {
                    dst[i] = *(unsigned long*)pimage;
                    I128_REPLICATE_WIDTH(dst[i], w);
                    pimage += stride;
                }
            I128_REPLICATE_HEIGHT(dst, h); 

            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->foreground = fg;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

            while (nRects--)
            {
                if (pRects->width && pRects->height)
                {
                    I128_WAIT_UNTIL_READY(i128Priv->engine);
                    i128Priv->engine->xy2 = I128_XY(pRects->width, pRects->height);
                    i128Priv->engine->xy1 = I128_XY(pRects->x + x0, pRects->y + y0);
                }
                pRects++;
            }
            I128_WAIT_UNTIL_READY(i128Priv->engine);
            i128Priv->engine->buf_control = buf_control;
        }
        I128_CLIP(i128Priv->clip);
    }
    else
    {
        I128_UNWRAP_GC(pGC, ops);
        (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
        I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    }
}

static void
_StippledPolyFillArc(pDraw, pGC, nArcs, pArcs)
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
_StippledPolyText8(pDraw, pGC, x, y, count, string)
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
_StippledPolyText16(pDraw, pGC, x, y, count, string)
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
_StippledImageText8(pDraw, pGC, x, y, count, string)
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
_StippledImageText16(pDraw, pGC, x, y, count, string)
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
_StippledImageGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_StippledPolyGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_StippledPushPixels(pGC, pBitmap, pDraw, width, height, x, y)
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
_StippledStub()
{
}


#endif /* I128_FAST_GC_OPS */
