/*
 *	@(#) i128Mono.c 11.1 97/10/22
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
 * S000, 12-Jun-95, kylec
 * 	created
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"

#include "i128Defs.h"
#include "i128Procs.h"

void
i128DrawMonoImage(
                  BoxPtr pbox,
                  void *image_data,
                  unsigned int startx,
                  unsigned int stride,
                  unsigned long fg,
                  unsigned char alu,
                  unsigned long planemask,
                  DrawablePtr pDrawable )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDrawable->pScreen);
    int h, i, j, w, num_longs;
    unsigned char *image = (unsigned char*)image_data;
    int x1, y1;
    unsigned char *src;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long buf_control, cmd;
    
    image += startx / 8;
    startx %= 8;
    x1 = pbox->x1;
    y1 = pbox->y1;
    w = pbox->x2 - pbox->x1;
    h = pbox->y2 - pbox->y1;
    
    buf_control = i128Priv->engine->buf_control;
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_STIPPLE_PACK32 |
            I128_CMD_TRANSPARENT);
    
    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
        (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
            (buf_control & I128_FLAG_NO_BPP); 
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->xy0 = I128_XY(0, 0);
    i128Priv->engine->xy2 = I128_XY(w, h);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
    
    if (w <= I128_MAX_WIDTH/2 && h <= I128_MAX_HEIGHT/2)
    {
        I128_WAIT_FOR_CACHE(i128Priv->engine); 
        for (j=0; j<h; j++)
        {
            dst[j] = *((unsigned long*)image) >> startx;
            image += stride;
        }
        I128_TRIGGER_CACHE(i128Priv->engine);
        i128Priv->engine->xy1 = I128_XY(x1, y1);
    }
    else if (i128Priv->mode.pixelsize < 4)
    {
        int num_strips, num_chunks, y, strip, chunk;
        int w_last, h_last;

        num_strips = (w + I128_MAX_WIDTH - 1) >> 5;
        num_chunks = (h + I128_MAX_HEIGHT - 1) >> 5;
        w_last = w & 0x1F;
        if (w_last == 0)
            w_last = I128_MAX_WIDTH;
        h_last = h & 0x1F;
        if (h_last == 0)
            h_last = I128_MAX_HEIGHT;
        strip = num_strips;
        while (strip--)
        {
            chunk = num_chunks;
            y = y1;
            w = strip > 0 ? I128_MAX_WIDTH - startx : w_last;
            src = image;

            while (chunk--)
            {
                h = chunk > 0 ? I128_MAX_HEIGHT : h_last;
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *((unsigned long*)src) >> startx;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += h;
            }
            image += 4;
            x1 += w;
            startx = 0;
        }
    }
    else
    {
        int num_strips, num_chunks, y, strip, chunk;
        int w_last, h_last;

        num_strips = (w + (I128_MAX_WIDTH/2) - 1) / (I128_MAX_WIDTH/2);
        num_chunks = (h + (I128_MAX_HEIGHT/2) - 1) / (I128_MAX_HEIGHT/2);
        w_last = w % (I128_MAX_WIDTH/2);
        if (w_last == 0)
            w_last = (I128_MAX_WIDTH/2);
        h_last = h % (I128_MAX_HEIGHT/2);
        if (h_last == 0)
            h_last = (I128_MAX_HEIGHT/2);
        strip = num_strips;
        while (strip--)
        {
            chunk = num_chunks;
            y = y1;
            w = strip > 0 ? (I128_MAX_WIDTH/2)-startx : w_last;
            src = image;

            while (chunk--)
            {
                h = chunk > 0 ? (I128_MAX_HEIGHT/2) : h_last;
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *((unsigned short*)src) >> startx;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += h;
            }
            image += 2;
            x1 += w;
            startx = 0;
        }
    }

    I128_WAIT_FOR_CACHE(i128Priv->engine);
    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;
}



void
i128DrawOpaqueMonoImage(
                        BoxPtr pbox,
                        void *image_data,
                        unsigned int startx,
                        unsigned int stride,
                        unsigned long fg,
                        unsigned long bg,
                        unsigned char alu,
                        unsigned long planemask,
                        DrawablePtr pDrawable )
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDrawable->pScreen);
    int h, i, j, w, num_longs;
    unsigned char *image = (unsigned char*)image_data;
    int x1, y1;
    unsigned char *src;
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned long buf_control, cmd;
    
    image += startx / 8;
    startx %= 8;
    x1 = pbox->x1;
    y1 = pbox->y1;
    w = pbox->x2 - pbox->x1;
    h = pbox->y2 - pbox->y1;
    
    buf_control = i128Priv->engine->buf_control;
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_STIPPLE_PACK32);
    
    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
        (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
            (buf_control & I128_FLAG_NO_BPP); 
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, planemask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->foreground = fg;
    i128Priv->engine->background = bg;
    i128Priv->engine->xy0 = I128_XY(0, 0);
    i128Priv->engine->xy2 = I128_XY(w, h);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
    
    if (w <= I128_MAX_WIDTH/2 && h <= I128_MAX_HEIGHT/2)
    {
        I128_WAIT_FOR_CACHE(i128Priv->engine); 
        for (j=0; j<h; j++)
        {
            dst[j] = *((unsigned long*)image) >> startx;
            image += stride;
        }
        I128_TRIGGER_CACHE(i128Priv->engine);
        i128Priv->engine->xy1 = I128_XY(x1, y1);
    }
    else if (i128Priv->mode.pixelsize < 4)
    {
        int num_strips, num_chunks, y, strip, chunk;
        int w_last, h_last;

        num_strips = (w + I128_MAX_WIDTH - 1) >> 5;
        num_chunks = (h + I128_MAX_HEIGHT - 1) >> 5;
        w_last = w & 0x1F;
        if (w_last == 0)
            w_last = I128_MAX_WIDTH;
        h_last = h & 0x1F;
        if (h_last == 0)
            h_last = I128_MAX_HEIGHT;
        strip = num_strips;
        while (strip--)
        {
            chunk = num_chunks;
            y = y1;
            w = strip > 0 ? I128_MAX_WIDTH - startx : w_last;
            src = image;

            while (chunk--)
            {
                h = chunk > 0 ? I128_MAX_HEIGHT : h_last;
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *((unsigned long*)src) >> startx;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += h;
            }
            image += 4;
            x1 += w;
            startx = 0;
        }
    }
    else
    {
        int num_strips, num_chunks, y, strip, chunk;
        int w_last, h_last;

        num_strips = (w + (I128_MAX_WIDTH/2) - 1) / (I128_MAX_WIDTH/2);
        num_chunks = (h + (I128_MAX_HEIGHT/2) - 1) / (I128_MAX_HEIGHT/2);
        w_last = w % (I128_MAX_WIDTH/2);
        if (w_last == 0)
            w_last = (I128_MAX_WIDTH/2);
        h_last = h % (I128_MAX_HEIGHT/2);
        if (h_last == 0)
            h_last = (I128_MAX_HEIGHT/2);
        strip = num_strips;
        while (strip--)
        {
            chunk = num_chunks;
            y = y1;
            w = strip > 0 ? (I128_MAX_WIDTH/2)-startx : w_last;
            src = image;

            while (chunk--)
            {
                h = chunk > 0 ? (I128_MAX_HEIGHT/2) : h_last;
                I128_WAIT_FOR_CACHE(i128Priv->engine);
                for (j=0; j<h; j++)
                {
                    dst[j] = *((unsigned short*)src) >> startx;
                    src += stride;
                }
                I128_TRIGGER_CACHE(i128Priv->engine);
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y);
                y += h;
            }
            image += 2;
            x1 += w;
            startx = 0;
        }
    }

    I128_WAIT_FOR_CACHE(i128Priv->engine);
    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;
}
