/*
 *	@(#) i128GCTiledOps.c 11.1 97/10/22
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
 * i128GCTiledOps.c
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

static void _TiledFillSpans();
static void _TiledSetSpans();
static void _TiledPutImage();
static RegionPtr _TiledCopyArea();
static RegionPtr _TiledCopyPlane();
static void _TiledPolyPoint();
static void _TiledPolylines();
static void _TiledPolySegment();
static void _TiledPolyRectangle();
static void _TiledPolyArc();
static void _TiledFillPolygon();
static void _TiledPolyFillRect();
static void _TiledPolyFillArc();
static int _TiledPolyText8();
static int _TiledPolyText16();
static void _TiledImageText8();
static void _TiledImageText16();
static void _TiledImageGlyphBlt();
static void _TiledPolyGlyphBlt();
static void _TiledPushPixels();
static void _TiledStub();

GCOps i128GCTiledOps = {
     _TiledFillSpans,      /*  void (* FillSpans)() */
     _TiledSetSpans,       /*  void (* SetSpans)()  */
     _TiledPutImage,       /*  void (* PutImage)()  */
     _TiledCopyArea,       /*  RegionPtr (* CopyArea)()     */
     _TiledCopyPlane,      /*  RegionPtr (* CopyPlane)() */
     _TiledPolyPoint,      /*  void (* PolyPoint)() */
     _TiledPolylines,      /*  void (* Polylines)() */
     _TiledPolySegment,    /*  void (* PolySegment)() */
     _TiledPolyRectangle,  /*  void (* PolyRectangle)() */
     _TiledPolyArc,        /*  void (* PolyArc)()   */
     _TiledFillPolygon,    /*  void (* FillPolygon)() */
     _TiledPolyFillRect,   /*  void (* PolyFillRect)() */
     _TiledPolyFillArc,    /*  void (* PolyFillArc)() */
     _TiledPolyText8,      /*  int (* PolyText8)()  */
     _TiledPolyText16,     /*  int (* PolyText16)() */
     _TiledImageText8,     /*  void (* ImageText8)() */
     _TiledImageText16,    /*  void (* ImageText16)() */
     _TiledImageGlyphBlt,  /*  void (* ImageGlyphBlt)() */
     _TiledPolyGlyphBlt,   /*  void (* PolyGlyphBlt)() */
     _TiledPushPixels,     /*  void (* PushPixels)() */
     _TiledStub,           /*  void (* Stub)() */
};

extern
GCFuncs i128GCFuncs;

static void
_TiledFillSpans(pDraw, pGC, nSpans, pPoints, pWidths, fSorted)
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
_TiledSetSpans(pDraw, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
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
_TiledPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
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
_TiledCopyArea(pSrc, pDst, pGC, srcx, srcy, width, height, dstx, dsty)
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
_TiledCopyPlane(pSrc, pDst, pGC, srcx, srcy,
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
_TiledPolyPoint(pDraw, pGC, mode, nPoints, pPoints)
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
_TiledPolylines(pDraw, pGC, mode, nPoints, pPoints)
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
_TiledPolySegment(pDraw, pGC, nSegments, pSegments)
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
_TiledPolyRectangle(pDraw, pGC, nRects, pRects)
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
_TiledPolyArc(pDraw, pGC, nArcs, pArcs)
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
_TiledFillPolygon(pDraw, pGC, shape, mode, nPoints, pPoints)
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
_TiledPolyFillRect(pDraw, pGC, nRects, pRects)
    DrawablePtr pDraw;
    GCPtr pGC;
    unsigned int nRects;
    xRectangle *pRects;
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    GCOps *ops;
    int x0, y0;
    DDXPointPtr patOrg;
    PixmapPtr pTile;
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long int planemask = pGCPriv->rRop.planemask;
    unsigned char alu = pGCPriv->rRop.alu;
    void *tile_data;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long buf_control, cmd;
    unsigned long fg, bg;
    int xoff, yoff;
    int stride;
    int width, height;
    register int w, h, i;
    RegionPtr prgnClip = pGCPriv->pCompositeClip;
    int numClipRects = REGION_NUM_RECTS(prgnClip);

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

        patOrg = &(pGCPriv->screenPatOrg);
        pTile = pGC->tile.pixmap;
        width = pTile->drawable.width;
        height = pTile->drawable.height;

        if (I128_MAX_WIDTH % width || I128_MAX_HEIGHT % height)
        {
            int cache_w, cache_h;
            int max_w, max_h;
            xRectangle *pR = pRects;
            register int i;

            cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
            cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;

#if 1
            for (i=0, max_w=0, max_h=0; i<nRects; i++)
            {
                if (max_w < pR->width)
                    max_w = pR->width;
                if (max_h < pR->height)
                    max_h = pR->height;
                pR++;
            }

            if ((cache_w < max_w + width) ||
                (cache_h < max_h + height))
            {
                I128_UNWRAP_GC(pGC, ops);
                (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
                I128_WRAP_GC(pGC, &i128GCFuncs, ops);
            }
            else
            {
                BoxRec tile_box;
                DDXPointRec org;

                tile_box.x1 = i128Priv->cache.x1;
                tile_box.y1 = i128Priv->cache.y1;
                tile_box.x2 = tile_box.x1 + width + max_w;
                tile_box.y2 = tile_box.y1 + height + max_h;
                org.x = patOrg->x + tile_box.x1;
                org.y = patOrg->y + tile_box.y1;
                I128_CLIP(i128Priv->cache);
                genTileRects(&tile_box, 1,
                             (unsigned char*)pTile->devPrivate.ptr,
                             pTile->devKind,
                             width, height,
                             &org, GXcopy, ~0, pDraw);
                I128_CLIP(clip);
                cmd  = (i128Priv->rop[alu] |
                        I128_OPCODE_BITBLT |
                        I128_CMD_CLIP_IN);
                i128Priv->engine->plane_mask =
                    I128_CONVERT(i128Priv->mode.pixelsize,
                                 planemask);
                i128Priv->engine->cmd = cmd;
                i128Priv->engine->xy3 = I128_DIR_LR_TB;
                i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            
                while (nRects--)
                {
                    if (pRects->width && pRects->height)
                    {
                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        i128Priv->engine->xy0 = 
                            I128_XY(i128Priv->cache.x1 + (pRects->x + x0)%width,
                                    i128Priv->cache.y1 + (pRects->y + y0)%height);
                        i128Priv->engine->xy2 =
                            I128_XY(pRects->width, pRects->height);
                        i128Priv->engine->xy1 =
                            I128_XY(pRects->x + x0, pRects->y + y0);
                    }
                    pRects++;
                }

            }
#else
            if ((cache_w < pDraw->width) ||
                (cache_h < pDraw->height))
            {
                I128_UNWRAP_GC(pGC, ops);
                (*pGC->ops->PolyFillRect)(pDraw, pGC, nRects, pRects);
                I128_WRAP_GC(pGC, &i128GCFuncs, ops);
            }
            else
            {
                BoxRec tile_box;
                DDXPointRec org;

                tile_box.x1 = i128Priv->cache.x1;
                tile_box.y1 = i128Priv->cache.y1;
                tile_box.x2 = tile_box.x1 + width;
                tile_box.y2 = tile_box.y1 + height;
                org.x = patOrg->x + tile_box.x1;
                org.y = patOrg->y + tile_box.y1;
                I128_CLIP(i128Priv->cache);
                genTileRects(&tile_box, 1,
                             (unsigned char*)pTile->devPrivate.ptr,
                             pTile->devKind,
                             width, height,
                             &org, GXcopy, ~0, pDraw);
                tile_box.x2 = tile_box.x1 + pDraw->width;
                tile_box.y2 = tile_box.y1 + pDraw->height;
                nfbReplicateArea(&tile_box, width, height,
                                 ~0, pDraw); 
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
                    I128_WAIT_UNTIL_READY(i128Priv->engine); 
                    i128Priv->engine->xy0 =
                        I128_XY(i128Priv->cache.x1 + pRects->x + x0,
                                i128Priv->cache.y1 + pRects->y + y0);
                    i128Priv->engine->xy2 =
                        I128_XY(pRects->width, pRects->height);
                    i128Priv->engine->xy1 =
                        I128_XY(pRects->x + x0, pRects->y + y0);
                    pRects++;
                }

            }
#endif
        }
        else
        {
            tile_data = pTile->devPrivate.ptr;
            stride = pTile->devKind;
            planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                                     planemask);
            buf_control = i128Priv->engine->buf_control;
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN  |
                    I128_CMD_STIPPLE_PACK32 |
                    I128_CMD_AREAPAT_32x32);

            I128_CLIP(clip)
                i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
                    (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
                        (buf_control & I128_FLAG_NO_BPP);     

            /* Convert tile to stipple */
            I128_WAIT_UNTIL_DONE(i128Priv->engine);
            switch (i128Priv->mode.pixelsize)
            {
              case 1:
                fg = (unsigned long)(*((char*)tile_data));
                for (h=0; h<height; h++)
                {
                    unsigned long d = 0;
                    unsigned char *s = tile_data;
                    for (w=0; w<width; w++)
                    {
                        d <<= 1;
                        if (s[w] == fg)
                            d |= 0x1;
                        else
                            bg = (unsigned long)s[w];
                    }
                    I128_REPLICATE_WIDTH(d, width);
                    dst[h] = d;
                    tile_data = (void *)((char*)tile_data + stride);
                }
                break;

              case 2:
                fg = (unsigned long)(*((short*)tile_data));
                for (h=0; h<height; h++)
                {
                    unsigned long d = 0;
                    unsigned short *s = tile_data;
                    for (w=0; w<width; w++)
                    {
                        d <<= 1;
                        if (s[w] == fg)
                            d |= 0x1;
                        else
                            bg = (unsigned long)s[w];
                    }
                    I128_REPLICATE_WIDTH(d, width);
                    dst[h] = d;
                    tile_data = (void *)((char*)tile_data + stride);
                }
                break;

              default:
                fg = (unsigned long)(*((long*)tile_data));
                for (h=0; h<height; h++)
                {
                    unsigned long d = 0;
                    unsigned long *s = tile_data;
                    for (w=0; w<width; w++)
                    {
                        d <<= 1;
                        if (s[w] == fg)
                            d |= 0x1;
                        else
                            bg = (unsigned long)s[w];
                    }
                    I128_REPLICATE_WIDTH(d, width);
                    dst[h] = d;
                    tile_data = (void *)((char*)tile_data + stride);
                }
                break;
            }     
            I128_REPLICATE_HEIGHT(dst, height);
            /*****/

            xoff = patOrg->x % width;
            yoff = patOrg->y % height;

            if (xoff || yoff)
                for( i=0; i<h; i++ )
                {
                    register int j = (i + h - yoff) % h;
                    register unsigned long tmp = dst[i];
                    dst[i] = dst[j] << (32 - xoff) | dst[j] >> xoff;
                    dst[j] = tmp;
                }
        
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
        return;                 /* RETURN */
    }

    
}


static void
_TiledPolyFillArc(pDraw, pGC, nArcs, pArcs)
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
_TiledPolyText8(pDraw, pGC, x, y, count, string)
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
_TiledPolyText16(pDraw, pGC, x, y, count, string)
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
_TiledImageText8(pDraw, pGC, x, y, count, string)
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
_TiledImageText16(pDraw, pGC, x, y, count, string)
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
_TiledImageGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_TiledPolyGlyphBlt(pDraw, pGC, x, y, nGlyphs, ppCharInfo, pGlyphBase)
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
_TiledPushPixels(pGC, pBitmap, pDraw, width, height, x, y)
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
_TiledStub()
{
}


#endif /* I128_FAST_GC_OPS */
