/*
 *	@(#) i128GCOpStippledOps.c 11.1 97/10/22
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
 * i128GCOpStippledOps.c
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

static void _OpSFillSpans();
static void _OpSSetSpans();
static void _OpSPutImage();
static RegionPtr _OpSCopyArea();
static RegionPtr _OpSCopyPlane();
static void _OpSPolyPoint();
static void _OpSPolylines();
static void _OpSPolySegment();
static void _OpSPolyRectangle();
static void _OpSPolyArc();
static void _OpSFillPolygon();
static void _OpSPolyFillRect();
static void _OpSPolyFillArc();
static int _OpSPolyText8();
static int _OpSPolyText16();
static void _OpSImageText8();
static void _OpSImageText16();
static void _OpSImageGlyphBlt();
static void _OpSPolyGlyphBlt();
static void _OpSPushPixels();
static void _OpSStub();

GCOps i128GCOpStippledOps = {
     _OpSFillSpans,      /*  void (* FillSpans)() */
     _OpSSetSpans,       /*  void (* SetSpans)()  */
     _OpSPutImage,       /*  void (* PutImage)()  */
     _OpSCopyArea,       /*  RegionPtr (* CopyArea)()     */
     _OpSCopyPlane,      /*  RegionPtr (* CopyPlane)() */
     _OpSPolyPoint,      /*  void (* PolyPoint)() */
     _OpSPolylines,      /*  void (* Polylines)() */
     _OpSPolySegment,    /*  void (* PolySegment)() */
     _OpSPolyRectangle,  /*  void (* PolyRectangle)() */
     _OpSPolyArc,        /*  void (* PolyArc)()   */
     _OpSFillPolygon,    /*  void (* FillPolygon)() */
     _OpSPolyFillRect,   /*  void (* PolyFillRect)() */
     _OpSPolyFillArc,    /*  void (* PolyFillArc)() */
     _OpSPolyText8,      /*  int (* PolyText8)()  */
     _OpSPolyText16,     /*  int (* PolyText16)() */
     _OpSImageText8,     /*  void (* ImageText8)() */
     _OpSImageText16,    /*  void (* ImageText16)() */
     _OpSImageGlyphBlt,  /*  void (* ImageGlyphBlt)() */
     _OpSPolyGlyphBlt,   /*  void (* PolyGlyphBlt)() */
     _OpSPushPixels,     /*  void (* PushPixels)() */
     _OpSStub,           /*  void (* Stub)() */
};

extern
GCFuncs i128GCFuncs;

static void
_OpSFillSpans(pDraw, pGC, nSpans, pPoints, pWidths, fSorted)
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
_OpSSetSpans(pDraw, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
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
_OpSPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
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
_OpSCopyArea(pSrc, pDst, pGC, srcx, srcy, width, height, dstx, dsty)
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
_OpSCopyPlane(pSrc, pDst, pGC, srcx, srcy,
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
_OpSPolyPoint(pDraw, pGC, mode, nPoints, pPoints)
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
_OpSPolylines(pDraw, pGC, mode, nPoints, pPoints)
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
_OpSPolySegment(pDraw, pGC, nSegments, pSegments)
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
_OpSPolyRectangle(pDraw, pGC, nRects, pRects)
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
_OpSPolyArc(pDraw, pGC, nArcs, pArcs)
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
_OpSFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints)
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
_OpSPolyFillRect(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nRects;
    xRectangle *pRects;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    GCOps *ops;
    int w, h;
    DDXPointPtr patOrg;
    PixmapPtr pStip;
    unsigned char *pimage;
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long cmd, buf_control;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int bg = pGCPriv->rRop.bg;
    unsigned long int planemask;
    int stride, xoff, yoff, x0, y0;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects;

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

        pStip = pGC->stipple;
        pimage = pStip->devPrivate.ptr;
        w = pStip->drawable.width;
        h = pStip->drawable.height;
        stride = pStip->devKind;
        patOrg = &(pGCPriv->screenPatOrg);
        planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                                 pGCPriv->rRop.planemask);

        if (I128_MAX_WIDTH % w || I128_MAX_HEIGHT % h)
        {
            int cache_w, cache_h;
            int max_w, max_h;
            xRectangle *pR = pRects;
            register int i;

            cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
            cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;
        
            for (i=0, max_w=0, max_h=0; i<nRects; i++)
            {
                if (max_w < pR->width)
                    max_w = pR->width;
                if (max_h < pR->height)
                    max_h = pR->height;
                pR++;
            }

            if ((nRects == 1) ||
                (cache_w < max_w + w) ||
                (cache_h < max_h + h))
            {
                I128_UNWRAP_GC(pGC, ops);
                (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
                I128_WRAP_GC(pGC, &i128GCFuncs, ops);
            }
            else
            {
                BoxRec tile_box, box = i128Priv->cache;

                xoff = patOrg->x % w;
                yoff = patOrg->y % h;
                I128_CLIP(box);
                if (xoff || yoff)
                {
                    tile_box.x1 = box.x1;
                    tile_box.y1 = box.y1;
                    tile_box.x2 = tile_box.x1 + xoff;
                    tile_box.y2 = tile_box.y1 + yoff;
                    i128DrawOpaqueMonoImage(&tile_box,
                                            (void*)(pimage + (h - yoff) * stride),
                                            w - xoff, stride, fg, bg, GXcopy,
                                            ~0, pDraw);

                    tile_box.x1 = box.x1;
                    tile_box.y1 = box.y1 + yoff;
                    tile_box.x2 = tile_box.x1 + xoff;
                    tile_box.y2 = tile_box.y1 + h - yoff;
                    i128DrawOpaqueMonoImage(&tile_box, (void*)(pimage),
                                            w - xoff, stride, fg, bg, GXcopy,
                                            ~0, pDraw);

                    tile_box.x1 = box.x1 + xoff;
                    tile_box.y1 = box.y1;
                    tile_box.x2 = tile_box.x1 + w - xoff;
                    tile_box.y2 = tile_box.y1 + yoff;
                    i128DrawOpaqueMonoImage(&tile_box,
                                            (void*)(pimage + (h - yoff) * stride),
                                            0, stride, fg, bg, GXcopy,
                                            ~0, pDraw);

                    tile_box.x1 = box.x1 + xoff;
                    tile_box.y1 = box.y1 + yoff;
                    tile_box.x2 = tile_box.x1 + w - xoff;
                    tile_box.y2 = tile_box.y1 + h - yoff;
                    i128DrawOpaqueMonoImage(&tile_box, (void*)pimage,
                                            0, stride, fg, bg, GXcopy,
                                            ~0, pDraw);
                }
                else
                {
                    tile_box.x1 = box.x1;
                    tile_box.y1 = box.y1;
                    tile_box.x2 = tile_box.x1 + w;
                    tile_box.y2 = tile_box.y1 + h;
                    i128DrawOpaqueMonoImage(&tile_box, (void*)pimage,
                                            0, stride, fg, bg, GXcopy,
                                            ~0, pDraw);
                }
                tile_box = box;
                tile_box.x2 = tile_box.x1 + w + max_w;
                tile_box.y2 = tile_box.y1 + h + max_h;
                nfbReplicateArea(&tile_box, w, h, ~0, pDraw);
                I128_CLIP(clip);
                cmd  = (i128Priv->rop[alu] |
                        I128_OPCODE_BITBLT |
                        I128_CMD_CLIP_IN);
                i128Priv->engine->plane_mask = planemask;
                i128Priv->engine->cmd = cmd;
                i128Priv->engine->xy3 = I128_DIR_LR_TB;
                i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            
                while (nRects--)
                {
                    if (pRects->width && pRects->height)
                    {
                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->xy0 =
                            I128_XY(box.x1 + (pRects->x + x0)%w,
                                    box.y1 + (pRects->y + y0)%h);
                        i128Priv->engine->xy2 =
                            I128_XY(pRects->width, pRects->height);
                        i128Priv->engine->xy1 =
                            I128_XY(pRects->x + x0, pRects->y + y0);
                    }
                    pRects++;
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

            I128_CLIP(clip);
            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->foreground = fg;
            i128Priv->engine->background = bg;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy0 = 0;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

            while (nRects--)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(pRects->width, pRects->height);
                i128Priv->engine->xy1 = I128_XY(pRects->x + x0, pRects->y + y0);
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
_OpSPolyFillArc(pDraw, pGC, nArcs, pArcs)
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
_OpSPolyText8(pDraw, pGC, x, y, count, string)
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
_OpSPolyText16(pDraw, pGC, x, y, count, string)
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
_OpSImageText8(pDraw, pGC, x, y, count, string)
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
_OpSImageText16(pDraw, pGC, x, y, count, string)
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
_OpSImageGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_OpSPolyGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_OpSPushPixels(pGC, pBitmap, pDraw, width, height, x, y)
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
_OpSStub()
{
}


#endif /* I128_FAST_GC_OPS */
