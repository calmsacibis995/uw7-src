/*
 *	@(#) i128Rect.c 11.1 97/10/22
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

void
i128CopyRect(
             BoxPtr pdstBox,
             DDXPointPtr psrc,
             unsigned char alu,
             unsigned long planemask,
             DrawablePtr pDraw)
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    int dir;
    short w, h;
    short x_src, y_src, x_dst, y_dst;
    int overlap_dx, overlap_dy;

#ifdef DEBUG
    ErrorF("i128CopyRect()\n");
#endif
     
    w = pdstBox->x2 - pdstBox->x1;
    h = pdstBox->y2 - pdstBox->y1;

    if (psrc->x >= pdstBox->x1) 
    {
        if (psrc->y >= pdstBox->y1) 
        {
            dir = I128_DIR_LR_TB;
            x_src = psrc->x;
            y_src = psrc->y;
            x_dst = pdstBox->x1;
            y_dst = pdstBox->y1;     
        }
        else
        {
            dir = I128_DIR_LR_BT;
            x_src = psrc->x;
            y_src = psrc->y + h - 1;
            x_dst = pdstBox->x1;
            y_dst = pdstBox->y2 - 1;     
        }
    }
    else
    {
        if (psrc->y >= pdstBox->y1) 
        {
            dir = I128_DIR_RL_TB;
            x_src = psrc->x + w - 1;
            y_src = psrc->y;
            x_dst = pdstBox->x2 - 1;
            y_dst = pdstBox->y1;     
        }
        else
        {
            dir = I128_DIR_RL_BT;
            x_src = psrc->x + w - 1;
            y_src = psrc->y + h - 1;
            x_dst = pdstBox->x2 - 1;
            y_dst = pdstBox->y2 - 1;     
        }
    }

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->xy3 = dir;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                             i128Priv->rop[alu] |
                             I128_CMD_CLIP_IN);
    I128_SAFE_BLIT(x_src, y_src, x_dst, y_dst, w, h, dir);

}

void
i128DrawSolidRects(
                   BoxPtr pbox,
                   unsigned int nbox,
                   unsigned long fg,
                   unsigned char alu,
                   unsigned long planemask,
                   DrawablePtr pDraw)
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    int w, h;

#ifdef DEBUG
    ErrorF("i128DrawSolidRects()\n");
#endif

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->foreground = fg;
    i128Priv->engine->plane_mask = 
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                             i128Priv->rop[alu] |
                             I128_CMD_CLIP_IN |
                             I128_CMD_SOLID);

    while (nbox--)
    {
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;
        I128_WAIT_UNTIL_READY(i128Priv->engine);
        I128_BLIT(0, 0, pbox->x1, pbox->y1, w, h, I128_DIR_LR_TB);
        pbox++;
    }
}


void
i128DrawPoints(
               DDXPointPtr ppt,
               unsigned int npts,
               unsigned long fg,
               unsigned char alu,
               unsigned long planemask,
               DrawablePtr pDrawable)
{
    VOLATILE i128PrivatePtr i128Priv =
        I128_PRIVATE_DATA(pDrawable->pScreen);

#ifdef DEBUG
    ErrorF("i128DrawPoints()\n");
#endif

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    
    if (alu == GXcopy)
    {
        switch (i128Priv->engine->buf_control & I128_FLAG_NO_BPP)
        {
          case I128_FLAG_8_BPP:
            {
                unsigned char *dst;
                int pitch;
                
                pitch = i128Priv->mode.bitmap_pitch;
                
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                
                i128Priv->info.memwins[0]->plane_mask =
                    I128_CONVERT(i128Priv->mode.pixelsize, planemask);
                
                dst = (unsigned char*)i128Priv->info.memwin[0].pointer;
                while (npts--)
                {
                    dst[pitch * ppt->y + ppt->x] = (unsigned char)fg;
                    ++ppt;
                }
            }
            break;

          case I128_FLAG_16_BPP:
            {
                unsigned short *dst;
                int pitch;
                
                pitch = i128Priv->mode.bitmap_pitch >> 1;
                
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                
                i128Priv->info.memwins[0]->plane_mask =
                    I128_CONVERT(i128Priv->mode.pixelsize, planemask);
                
                dst = (unsigned short*)i128Priv->info.memwin[0].pointer;
                
                while (npts--)
                {
                    dst[pitch * ppt->y + ppt->x] = (unsigned short)fg;
                    ++ppt;
                }
            }
            break;

          case I128_FLAG_32_BPP:
          default:
            {
                unsigned long *dst;
                int pitch;
                
                pitch = i128Priv->mode.bitmap_pitch >> 2;
                
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                
                i128Priv->info.memwins[0]->plane_mask =
                    I128_CONVERT(i128Priv->mode.pixelsize, planemask);
                
                dst = (unsigned long*)i128Priv->info.memwin[0].pointer;
                while (npts--)
                {
                    dst[pitch * ppt->y + ppt->x] = fg;
                    ++ppt;
                }
            }
            break;
        }
    }
    else
    {
        i128Priv->engine->foreground = fg;
        i128Priv->engine->plane_mask =
            I128_CONVERT(i128Priv->mode.pixelsize, planemask);

        i128Priv->engine->cmd = (I128_OPCODE_LINE |
                                 i128Priv->rop[alu] |
                                 I128_CMD_CLIP_IN | 
                                 I128_CMD_SOLID);

        while (npts--)
        {
            I128_WAIT_UNTIL_READY(i128Priv->engine);
            I128_DRAW_POINT(ppt->x, ppt->y);
            ++ppt;
        }
    }
}

void
i128TileRects(
              BoxPtr pbox,
              unsigned int nbox,
              void *tile_data,
              unsigned int stride,
              unsigned int width,
              unsigned int height,
              DDXPointPtr patOrg,
              unsigned char alu,
              unsigned long planemask,
              DrawablePtr pDraw )
{
#if TILE_CACHE
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    unsigned char *tile = (unsigned char*)tile_data;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    BoxRec clip, tile_box;
    unsigned long buf_control, cmd;
    register int w, h;
     
    if (width != I128_MAX_WIDTH || height != I128_MAX_HEIGHT)
    {
        genTileRects (pbox, nbox, tile_data, stride,
                      width, height, patOrg, alu, planemask, pDraw);
        return;
    }
     
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_AREAPAT_32x32);
     
    tile_box = i128Priv->cache;
    tile_box.x2 = tile_box.x1 + I128_MAX_WIDTH;
    tile_box.y2 = tile_box.y1 + I128_MAX_HEIGHT;
    clip = i128Priv->clip;

    i128SetClipRegions(&i128Priv->cache, 1, pDraw);
    i128DrawImage(&tile_box, tile_data, stride,
                  GXcopy, 0xFFFFFFFF, pDraw);
    i128SetClipRegions(&clip, 1, pDraw);

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->xy0 = I128_XY(i128Priv->cache.x1, i128Priv->cache.y1);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
     
    while (nbox--)
    {
        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->xy2 = I128_XY(pbox->x2 - pbox->x1,
                                        pbox->y2 - pbox->y1);
        i128Priv->engine->xy1 = I128_XY(pbox->x1, pbox->y1);
        pbox++;
    }
#else
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    unsigned char *tile = (unsigned char*)tile_data;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long buf_control, cmd;
    unsigned long fg, bg;
    register int w, h;

    if (I128_MAX_WIDTH % width || I128_MAX_HEIGHT % height)
    {
        genTileRects (pbox, nbox, tile_data, stride,
                      width, height, patOrg, alu, planemask, pDraw);
        return;
    }

    planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                             planemask);
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

    i128Priv->engine->plane_mask = planemask;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->background = bg;
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->xy0 = 0;
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

    while (nbox--)
    {
        I128_WAIT_FOR_CACHE(i128Priv->engine); 
        i128Priv->engine->xy2 = I128_XY(pbox->x2 - pbox->x1,
                                        pbox->y2 - pbox->y1);
        I128_TRIGGER_CACHE(i128Priv->engine);
        i128Priv->engine->xy1 = I128_XY(pbox->x1, pbox->y1);
        pbox++;
    }

    I128_WAIT_FOR_CACHE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;

#endif     
}

/* Solid ops */
void
i128SolidFillRects(
                   GCPtr pGC,
                   DrawablePtr pDraw,
                   BoxPtr pBox,
                   unsigned int nBox)
{
    VOLATILE i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    int w, h;

#ifdef DEBUG
    ErrorF("i128SolidFillRects()\n");
#endif

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->plane_mask = 
        I128_CONVERT(i128Priv->mode.pixelsize, pGCPriv->rRop.planemask);
    i128Priv->engine->foreground = (unsigned long)pGCPriv->rRop.fg;
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                             i128Priv->rop[pGCPriv->rRop.alu] |
                             I128_CMD_CLIP_IN |
                             I128_CMD_SOLID);
     
    while (nBox--)
    {
        I128_WAIT_UNTIL_READY(i128Priv->engine); 
        i128Priv->engine->xy2 = I128_XY(pBox->x2 - pBox->x1,
                                        pBox->y2 - pBox->y1);
        i128Priv->engine->xy1 = I128_XY(pBox->x1, pBox->y1);
        pBox++;
    }

}

void
i128SolidFS(
            GCPtr pGC,
            DrawablePtr pDraw,
            DDXPointPtr ppt,
            unsigned int *pw,
            unsigned npts)
{
    VOLATILE i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long planemask;
    unsigned char alu = pGCPriv->rRop.alu;
    unsigned long fg = pGCPriv->rRop.fg;

#ifdef DEBUG
    ErrorF("i128SolidFS()\n");
#endif

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, pGCPriv->rRop.planemask);
    i128Priv->engine->foreground = fg;
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                             i128Priv->rop[alu] |
                             I128_CMD_CLIP_IN |
                             I128_CMD_SOLID);
     
    while (npts--)
    {
        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->xy2 = I128_XY(*pw, 1);
        i128Priv->engine->xy1 = I128_XY(ppt->x, ppt->y);
        ppt++;
        pw++;
    }
}


/* Tiled ops */
void
i128TiledFillRects(
                   GCPtr pGC,
                   DrawablePtr pDraw,
                   BoxPtr pbox,
                   unsigned int nbox )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
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

    patOrg = &(pGCPriv->screenPatOrg);
    pTile = pGC->tile.pixmap;
    width = pTile->drawable.width;
    height = pTile->drawable.height;

    if (I128_MAX_WIDTH % width || I128_MAX_HEIGHT % height)
    {
        int cache_w, cache_h;

        cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
        cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;
            
        if ((nbox == 1) ||
            (cache_w < pDraw->width) ||
            (cache_h < pDraw->height))
        {
            genTileRects (pbox, nbox, (unsigned char*)pTile->devPrivate.ptr,
                          pTile->devKind,
                          width, height,
                          &(pGCPriv->screenPatOrg),
                          alu, planemask, pDraw);
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
            I128_CLIP(i128Priv->clip);
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN);
            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            
            while (nbox--)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                i128Priv->engine->xy0 =
                    I128_XY(i128Priv->cache.x1 + pbox->x1,
                            i128Priv->cache.y1 + pbox->y1);
                i128Priv->engine->xy2 =
                    I128_XY(pbox->x2 - pbox->x1,
                            pbox->y2 - pbox->y1);
                i128Priv->engine->xy1 =
                    I128_XY(pbox->x1, pbox->y1);
                pbox++;
            }

        }
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

        I128_WAIT_UNTIL_READY(i128Priv->engine);
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

        while (nbox--)
        {
            I128_WAIT_FOR_CACHE(i128Priv->engine); 
            i128Priv->engine->xy2 = I128_XY(pbox->x2 - pbox->x1,
                                            pbox->y2 - pbox->y1);
            I128_TRIGGER_CACHE(i128Priv->engine);
            i128Priv->engine->xy1 = I128_XY(pbox->x1, pbox->y1);
            pbox++;
        }

        I128_WAIT_FOR_CACHE(i128Priv->engine);
        i128Priv->engine->buf_control = buf_control;
    }
}


void
i128TiledFS(
            GCPtr pGC,
            DrawablePtr pDraw,
            register DDXPointPtr ppt,
            register unsigned int *pw,
            unsigned int npts )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    DDXPointPtr patOrg;
    PixmapPtr pTile;
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long int planemask = pGCPriv->rRop.planemask;
    unsigned char alu = pGCPriv->rRop.alu;
    void *tile_data;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long buf_control, cmd;
    unsigned long fg, bg;
    int stride;
    int xoff, yoff;
    int width, height;
    register int w, h, i;

    patOrg = &(pGCPriv->screenPatOrg);
    pTile = pGC->tile.pixmap;
    width = pTile->drawable.width;
    height = pTile->drawable.height;

    if (I128_MAX_WIDTH % width || I128_MAX_HEIGHT % height)
    {
        genTiledFS( pGC, pDraw, ppt, pw, npts );
        return;
    }

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

    I128_WAIT_UNTIL_READY(i128Priv->engine);
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

    while (npts--)
    {
        I128_WAIT_FOR_CACHE(i128Priv->engine);
        i128Priv->engine->xy2 = I128_XY(*pw, 1);
        I128_TRIGGER_CACHE(i128Priv->engine);
        i128Priv->engine->xy1 = I128_XY(ppt->x, ppt->y);
        ppt++;
        pw++;
    }     

    I128_WAIT_FOR_CACHE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;
}

/* Stippled ops */
void
i128StippledFillRects(
                      GCPtr pGC,
                      DrawablePtr pDraw,
                      BoxPtr pbox,
                      unsigned int nbox )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    int w, h, xoff, yoff;
    DDXPointPtr patOrg;
    PixmapPtr pStip;
    unsigned char *pimage;
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long cmd, buf_control;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int planemask;
    int stride;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;

    pStip = pGC->stipple;
    pimage = pStip->devPrivate.ptr;
    stride = pStip->devKind;
    w = pStip->drawable.width;
    h = pStip->drawable.height;
    patOrg = &(pGCPriv->screenPatOrg);
    planemask = I128_CONVERT(i128Priv->mode.pixelsize,
                             pGCPriv->rRop.planemask);

    if (I128_MAX_WIDTH % w || I128_MAX_HEIGHT % h)
    {
        int cache_w, cache_h;
        unsigned int cmd1, cmd2;
        cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
        cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;

        if (((2*w+1) > cache_w) || ((h) > cache_h) ||
            ((pbox->x2 - pbox->x1 > I128_MAX_WIDTH) &&
             (pbox->y2 - pbox->y1 > I128_MAX_HEIGHT)))
            genStippledFillRects (pGC, pDraw, pbox, nbox);
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
                genStippledFillRects (pGC, pDraw, pbox, nbox);
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
            
                I128_CLIP(i128Priv->clip);

                i128Priv->engine->plane_mask = planemask;
                i128Priv->engine->xy3 = I128_DIR_LR_TB;
                i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                while(nbox--)
                {
                    box.x1 = pbox->x1;
                    box.y1 = pbox->y1;

                    xoff = ( box.x1 - patOrg->x ) % w;
                    if ( xoff < 0 )
                        xoff += w;

                    yoff = ( box.y1 - patOrg->y ) % h;
                    if ( yoff < 0 )
                        yoff += h;

                    box.x2 = min( box.x1 + w - xoff, pbox->x2 );
                    box.y2 = min( box.y1 + h - yoff, pbox->y2 );

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

                    while ( box.x2 < pbox->x2 )
                    {
                        box.x1 = box.x2;
                        box.x2 += w;
                        if ( box.x2 > pbox->x2 )
                            box.x2 = pbox->x2;

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

                    while ( box.y2 < pbox->y2 )
                    {
                        box.y1 = box.y2;
                        box.y2 += h;
                        if ( box.y2 > pbox->y2 )
                            box.y2 = pbox->y2;

                        box.x1 = pbox->x1;
                        box.x2 = min( box.x1 + w - xoff, pbox->x2 );

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
                    
                        while ( box.x2 < pbox->x2 )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pbox->x2 )
                                box.x2 = pbox->x2;

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

                    pbox++;
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
                I128_CLIP(i128Priv->clip);
                i128Priv->engine->cmd = (i128Priv->rop[alu] |
                                         I128_OPCODE_BITBLT |
                                         I128_CMD_CLIP_IN);
                i128Priv->engine->plane_mask = planemask;
                i128Priv->engine->xy3 = I128_DIR_LR_TB;
                i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

                while(nbox--)
                {
                    box.x1 = pbox->x1;
                    box.y1 = pbox->y1;

                    xoff = ( box.x1 - patOrg->x ) % w;
                    if ( xoff < 0 )
                        xoff += w;

                    yoff = ( box.y1 - patOrg->y ) % h;
                    if ( yoff < 0 )
                        yoff += h;

                    box.x2 = min( box.x1 + w - xoff, pbox->x2 );
                    box.y2 = min( box.y1 + h - yoff, pbox->y2 );

                    I128_WAIT_UNTIL_READY(i128Priv->engine); 
                    I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1 + yoff,
                                   box.x1, box.y1,
                                   box.x2 - box.x1, box.y2 - box.y1,
                                   I128_DIR_LR_TB);

                    while ( box.x2 < pbox->x2 )
                    {
                        box.x1 = box.x2;
                        box.x2 += w;
                        if ( box.x2 > pbox->x2 )
                            box.x2 = pbox->x2;

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        I128_SAFE_BLIT(cache1.x1, cache1.y1 + yoff,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);
                    }

                    while ( box.y2 < pbox->y2 )
                    {
                        box.y1 = box.y2;
                        box.y2 += h;
                        if ( box.y2 > pbox->y2 )
                            box.y2 = pbox->y2;

                        box.x1 = pbox->x1;
                        box.x2 = min( box.x1 + w - xoff, pbox->x2 );

                        I128_WAIT_UNTIL_READY(i128Priv->engine); 
                        I128_SAFE_BLIT(cache1.x1 + xoff, cache1.y1,
                                       box.x1, box.y1,
                                       box.x2 - box.x1, box.y2 - box.y1,
                                       I128_DIR_LR_TB);
                    
                        while ( box.x2 < pbox->x2 )
                        {
                            box.x1 = box.x2;
                            box.x2 += w;
                            if ( box.x2 > pbox->x2 )
                                box.x2 = pbox->x2;

                            I128_WAIT_UNTIL_READY(i128Priv->engine); 
                            I128_SAFE_BLIT(cache1.x1, cache1.y1,
                                           box.x1, box.y1,
                                           box.x2 - box.x1, box.y2 - box.y1,
                                           I128_DIR_LR_TB);
                        }
                    }
                    pbox++;
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

        while (nbox--)
        {
            I128_WAIT_FOR_CACHE(i128Priv->engine); 
            i128Priv->engine->xy2 = I128_XY(pbox->x2 - pbox->x1,
                                            pbox->y2 - pbox->y1);
            I128_TRIGGER_CACHE(i128Priv->engine);
            i128Priv->engine->xy1 = I128_XY(pbox->x1, pbox->y1);
            pbox++;
        }

        I128_WAIT_FOR_CACHE(i128Priv->engine);
        i128Priv->engine->buf_control = buf_control;
    }
}


void
i128StippledFS(
               GCPtr pGC,
               DrawablePtr pDraw,
               register DDXPointPtr ppt,
               register unsigned int *pw,
               unsigned int npts )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    int w, h;
    DDXPointPtr patOrg;
    PixmapPtr pStip;
    unsigned char *pimage;
    nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
    unsigned long cmd, buf_control;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long int fg = pGCPriv->rRop.fg;
    unsigned long int planemask;
    unsigned char alu = pGCPriv->rRop.alu;
    int stride, xoff, yoff;
    register int i;

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
        genStippledFS( pGC, pDraw, ppt, pw, npts );
        return;
    }

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

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->plane_mask = planemask;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->xy0 = 0;
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

    while (npts--)
    {
        I128_WAIT_FOR_CACHE(i128Priv->engine); 
        i128Priv->engine->xy2 = I128_XY(*pw, 1);
        I128_TRIGGER_CACHE(i128Priv->engine);
        i128Priv->engine->xy1 = I128_XY(ppt->x, ppt->y);
        ppt++;
        pw++;
    }     

    I128_WAIT_FOR_CACHE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;

}


/* Opaque stippled ops */
void
i128OpStippledFillRects(
                        GCPtr pGC,
                        DrawablePtr pDraw,
                        BoxPtr pbox,
                        unsigned int nbox )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
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
    int stride, xoff, yoff;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;

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
#if 1
        genOpStippledFillRects (pGC, pDraw, pbox, nbox);
#else        
        int cache_w, cache_h;

        cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
        cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;
        
        if ((nbox == 1) ||
            (cache_w < pDraw->width) ||
            (cache_h < pDraw->height))
        {
            genOpStippledFillRects (pGC, pDraw, pbox, nbox);    
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
            tile_box.x2 = tile_box.x1 + pDraw->width;
            tile_box.y2 = tile_box.y1 + pDraw->height;
            nfbReplicateArea(&tile_box, w, h, ~0, pDraw);
            I128_CLIP(i128Priv->clip);
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN);
            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            
            while (nbox--)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine); 
                i128Priv->engine->xy0 =
                    I128_XY(box.x1 + pbox->x1,
                            box.y1 + pbox->y1);
                i128Priv->engine->xy2 =
                    I128_XY(pbox->x2 - pbox->x1,
                            pbox->y2 - pbox->y1);
                i128Priv->engine->xy1 =
                    I128_XY(pbox->x1, pbox->y1);
                pbox++;
            }
        }
#endif
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

        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->plane_mask = planemask;
        i128Priv->engine->foreground = fg;
        i128Priv->engine->background = bg;
        i128Priv->engine->cmd = cmd;
        i128Priv->engine->xy0 = 0;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

        while (nbox--)
        {
            I128_WAIT_FOR_CACHE(i128Priv->engine); 
            i128Priv->engine->xy2 = I128_XY(pbox->x2 - pbox->x1,
                                            pbox->y2 - pbox->y1);
            I128_TRIGGER_CACHE(i128Priv->engine);
            i128Priv->engine->xy1 = I128_XY(pbox->x1, pbox->y1);
            pbox++;
        }

        I128_WAIT_FOR_CACHE(i128Priv->engine);
        i128Priv->engine->buf_control = buf_control;
    }
}


void
i128OpStippledFS(
                 GCPtr pGC,
                 DrawablePtr pDraw,
                 register DDXPointPtr ppt,
                 register unsigned int *pw,
                 unsigned int npts )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
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
    int stride, xoff, yoff;
    unsigned char alu = pGCPriv->rRop.alu;
    register int i;

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
#if 1
        genOpStippledFS (pGC, pDraw, ppt, pw, npts);    
#else
        int cache_w, cache_h;

        cache_w = i128Priv->cache.x2 - i128Priv->cache.x1;
        cache_h = i128Priv->cache.y2 - i128Priv->cache.y1;
        
        if ((npts == 1) ||
            (cache_w < pDraw->width) ||
            (cache_h < pDraw->height))
        {
            genOpStippledFS (pGC, pDraw, ppt, pw, npts);    
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
            tile_box.x2 = tile_box.x1 + pDraw->width;
            tile_box.y2 = tile_box.y1 + pDraw->height;
            nfbReplicateArea(&tile_box, w, h, ~0, pDraw);
            I128_CLIP(i128Priv->clip);
            cmd  = (i128Priv->rop[alu] |
                    I128_OPCODE_BITBLT |
                    I128_CMD_CLIP_IN);
            i128Priv->engine->plane_mask = planemask;
            i128Priv->engine->cmd = cmd;
            i128Priv->engine->xy3 = I128_DIR_LR_TB;
            i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
            
            while (npts--)
            {
                I128_WAIT_UNTIL_READY(i128Priv->engine);
                i128Priv->engine->xy0 =
                    I128_XY(box.x1 + ppt->x, box.x2 + ppt->y);
                i128Priv->engine->xy2 = I128_XY(*pw, 1);
                i128Priv->engine->xy1 = I128_XY(ppt->x, ppt->y);
                ppt++;
                pw++;
            }     
        }
#endif
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

        I128_WAIT_UNTIL_READY(i128Priv->engine);
        i128Priv->engine->plane_mask = planemask;
        i128Priv->engine->foreground = fg;
        i128Priv->engine->background = bg;
        i128Priv->engine->cmd = cmd;
        i128Priv->engine->xy0 = 0;
        i128Priv->engine->xy3 = I128_DIR_LR_TB;
        i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

        while (npts--)
        {
            I128_WAIT_FOR_CACHE(i128Priv->engine); 
            i128Priv->engine->xy2 = I128_XY(*pw, 1);
            I128_TRIGGER_CACHE(i128Priv->engine);
            i128Priv->engine->xy1 = I128_XY(ppt->x, ppt->y);
            ppt++;
            pw++;
        }     

        I128_WAIT_FOR_CACHE(i128Priv->engine);
        i128Priv->engine->buf_control = buf_control;
    }
}

