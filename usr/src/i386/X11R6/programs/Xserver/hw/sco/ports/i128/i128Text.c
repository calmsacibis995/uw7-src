/*
 *	@(#) i128Text.c 11.1 97/10/22
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
 * S000, 23-Aug-95, kylec@sco.com
 *	- create
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "dixfontstr.h"
#include "miscstruct.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "gen/genProcs.h"
#include <string.h>

#include "i128Defs.h"
#include "i128Procs.h"

#define I128_COPY_6X(_d, _h) \
_d[_h] = (unsigned long)((s0[_h]) | \
                          (s1[_h] << 6) | \
                          (s2[_h] << 12) | \
                          (s3[_h] << 18) | \
                          (s4[_h] << 24))

#define I128_COPY_8X(_d, _h) \
_d[_h] = (unsigned long)((s0[_h]) | \
                          (s1[_h] << 8) | \
                          (s2[_h] << 16) | \
                          (s3[_h] << 24))

#define I128_COPY_9X(_d, _h) \
_d[_h] = (unsigned long)((s0[_h]) | \
                          (s1[_h] << 9) | \
                          (s2[_h] << 18))

#define I128_COPY_10X(_d, _h) \
_d[_h] = (unsigned long)((s0[_h]) | \
                          (s1[_h] << 10) | \
                          (s2[_h] << 20))

void
i128DrawFontText(
                 BoxPtr pbox,
                 unsigned char *chars,
                 unsigned int count,
                 nfbFontPSPtr pPS,
                 unsigned long fg,
                 unsigned long bg,
                 unsigned char alu,
                 unsigned long planemask,
                 unsigned char transparent,
                 DrawablePtr pDraw)

{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
    unsigned long height, width, h, w;
    unsigned long buf_control;
    int num_strips, strip;
    int num_chunks, chunk;
    register unsigned char *src;
    unsigned char **ppbits, *image;
    int stride, i;
    register int j;
    int x1, y1, x2;
    unsigned long cmd;
    int x_cache, y_cache;
    Bool cached = 0;
    unsigned long pmask;
    static int state = 0;
    register unsigned long *dst =
        (unsigned long*)i128Priv->info.xy_win[0].pointer;

    buf_control = i128Priv->engine->buf_control;
    
    width = pPS->pFontInfo->width;
    height = pPS->pFontInfo->height;
    ppbits = pPS->pFontInfo->ppBits;
    stride = pPS->pFontInfo->stride;
    pmask = planemask;
    y1 = pbox->y1;
    x1 = pbox->x1;
    x2 = x1 + width;

    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  | 
            I128_CMD_STIPPLE_PACK32);
    
    if (transparent)
        cmd |= I128_CMD_TRANSPARENT;

    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = (buf_control & ~I128_FLAG_SRC_BITS) |
        (I128_FLAG_SRC_CACHE | I128_FLAG_CACHE_ON) |
            (buf_control & I128_FLAG_NO_BPP); 
    i128Priv->engine->xy0 = I128_XY(0, 0);
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
    i128Priv->engine->plane_mask =
        I128_CONVERT(i128Priv->mode.pixelsize, pmask);
    i128Priv->engine->cmd = cmd;
    i128Priv->engine->background = bg;
    i128Priv->engine->foreground = fg;

    I128_WAIT_UNTIL_DONE(i128Priv->engine);

    if (width <= I128_MAX_WIDTH && height <= I128_MAX_HEIGHT/2)
    {
        long double_buffer_offset = 0;
        long double_xy0_value = I128_CACHE_SIZE/2 << 16;
        long xy0 = 0;
        volatile long wait;
        unsigned long *dst_double = dst + 16;

        i = 0;
        if (i128Priv->mode.pixelsize == 1)
            switch (width)
            {

              case 6:
                {
                    register unsigned char *s0, *s1, *s2, *s3, *s4;
                    unsigned char *ch;
                    int cnt = count - 10;

                    i128Priv->engine->xy2 = I128_XY(30, height);

                    switch (height)
                    {
                      case 13:
                        while (i <= cnt)
                        {
                            ch = chars + i;

                            s0 = ppbits[ch[0]];
                            s1 = ppbits[ch[1]];
                            s2 = ppbits[ch[2]];
                            s3 = ppbits[ch[3]];
                            s4 = ppbits[ch[4]];

                            i += 5;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_6X(dst, 0);
                            I128_COPY_6X(dst, 1);
                            I128_COPY_6X(dst, 2);
                            I128_COPY_6X(dst, 3);
                            I128_COPY_6X(dst, 4);
                            I128_COPY_6X(dst, 5);
                            I128_COPY_6X(dst, 6);
                            I128_COPY_6X(dst, 7);
                            I128_COPY_6X(dst, 8);
                            I128_COPY_6X(dst, 9);
                            I128_COPY_6X(dst, 10);
                            I128_COPY_6X(dst, 11);
                            I128_COPY_6X(dst, 12);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 30;

                            /****/

                            ch = chars + i;

                            s0 = ppbits[ch[0]];
                            s1 = ppbits[ch[1]];
                            s2 = ppbits[ch[2]];
                            s3 = ppbits[ch[3]];
                            s4 = ppbits[ch[4]];

                            i += 5;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_6X(dst_double, 0);
                            I128_COPY_6X(dst_double, 1);
                            I128_COPY_6X(dst_double, 2);
                            I128_COPY_6X(dst_double, 3);
                            I128_COPY_6X(dst_double, 4);
                            I128_COPY_6X(dst_double, 5);
                            I128_COPY_6X(dst_double, 6);
                            I128_COPY_6X(dst_double, 7);
                            I128_COPY_6X(dst_double, 8);
                            I128_COPY_6X(dst_double, 9);
                            I128_COPY_6X(dst_double, 10);
                            I128_COPY_6X(dst_double, 11);
                            I128_COPY_6X(dst_double, 12);

                            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 30;

                        }
                        break;

                      default:
                        while (i <= cnt)
                        {
                            ch = chars + i;

                            s0 = ppbits[ch[0]];
                            s1 = ppbits[ch[1]];
                            s2 = ppbits[ch[2]];
                            s3 = ppbits[ch[3]];
                            s4 = ppbits[ch[4]];

                            i += 5;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_6X(dst, 0);
                            I128_COPY_6X(dst, 1);
                            I128_COPY_6X(dst, 2);
                            I128_COPY_6X(dst, 3);
                            I128_COPY_6X(dst, 4);
                            I128_COPY_6X(dst, 5);
                            I128_COPY_6X(dst, 6);
                            I128_COPY_6X(dst, 7);

                            for (j=8; j<height; j++)
                                I128_COPY_6X(dst, j);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 30;

                            /****/
                            ch = chars + i;

                            s0 = ppbits[ch[0]];
                            s1 = ppbits[ch[1]];
                            s2 = ppbits[ch[2]];
                            s3 = ppbits[ch[3]];
                            s4 = ppbits[ch[4]];

                            i += 5;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_6X(dst_double, 0);
                            I128_COPY_6X(dst_double, 1);
                            I128_COPY_6X(dst_double, 2);
                            I128_COPY_6X(dst_double, 3);
                            I128_COPY_6X(dst_double, 4);
                            I128_COPY_6X(dst_double, 5);
                            I128_COPY_6X(dst_double, 6);
                            I128_COPY_6X(dst_double, 7);

                            for (j=8; j<height; j++)
                                I128_COPY_6X(dst_double, j);

                            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 30;

                        }
                        break;

                    }                

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                    while (i < count)
                    {
                        s0 = (unsigned char*)ppbits[chars[i++]];
                        I128_WAIT_FOR_CACHE(i128Priv->engine);
                        for (j=0; j<height; j++)
                            dst[j] = (unsigned long)(s0[j]);
                        I128_TRIGGER_CACHE(i128Priv->engine);
                        i128Priv->engine->xy0 = I128_XY(0, 0);
                        i128Priv->engine->xy2 = I128_XY(width, height); 
                        i128Priv->engine->xy1 = I128_XY(x1, y1);
                        x1 += width;
                    }

                }
                break;

              case 8:
                {
                    register unsigned char *s0, *s1, *s2, *s3;
                    int cnt = count - 8;
                
                    i128Priv->engine->xy2 = I128_XY(32, height);

                    switch (height)
                    {
                      case 13:
                        while (i <= cnt)
                        {
                            s0 = ppbits[chars[i]];
                            s1 = ppbits[chars[i+1]];
                            s2 = ppbits[chars[i+2]];
                            s3 = ppbits[chars[i+3]];
                            i += 4;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_8X(dst, 0);
                            I128_COPY_8X(dst, 1);
                            I128_COPY_8X(dst, 2);
                            I128_COPY_8X(dst, 3);
                            I128_COPY_8X(dst, 4);
                            I128_COPY_8X(dst, 5);
                            I128_COPY_8X(dst, 6);
                            I128_COPY_8X(dst, 7);
                            I128_COPY_8X(dst, 8);
                            I128_COPY_8X(dst, 9);
                            I128_COPY_8X(dst, 10);
                            I128_COPY_8X(dst, 11);
                            I128_COPY_8X(dst, 12);
                            I128_COPY_8X(dst, 13);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 

                            x1 += 32;

                            /****/
                            s0 = ppbits[chars[i]];
                            s1 = ppbits[chars[i+1]];
                            s2 = ppbits[chars[i+2]];
                            s3 = ppbits[chars[i+3]];
                            i += 4;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_8X(dst_double, 0);
                            I128_COPY_8X(dst_double, 1);
                            I128_COPY_8X(dst_double, 2);
                            I128_COPY_8X(dst_double, 3);
                            I128_COPY_8X(dst_double, 4);
                            I128_COPY_8X(dst_double, 5);
                            I128_COPY_8X(dst_double, 6);
                            I128_COPY_8X(dst_double, 7);
                            I128_COPY_8X(dst_double, 8);
                            I128_COPY_8X(dst_double, 9);
                            I128_COPY_8X(dst_double, 10);
                            I128_COPY_8X(dst_double, 11);
                            I128_COPY_8X(dst_double, 12);
                            I128_COPY_8X(dst_double, 13);

                            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 

                            x1 += 32;

                        }
                        break;

                      default:
                        while (i <= cnt)
                        {
                            s0 = ppbits[chars[i]];
                            s1 = ppbits[chars[i+1]];
                            s2 = ppbits[chars[i+2]];
                            s3 = ppbits[chars[i+3]];
                            i += 4;

                            I128_COPY_8X(dst, 0);
                            I128_COPY_8X(dst, 1);
                            I128_COPY_8X(dst, 2);
                            I128_COPY_8X(dst, 3);
                            I128_COPY_8X(dst, 4);
                            I128_COPY_8X(dst, 5);
                            I128_COPY_8X(dst, 6);
                            I128_COPY_8X(dst, 7);
                            I128_COPY_8X(dst, 8);
                            for (j=9; j < height; j++)
                                I128_COPY_8X(dst, j);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 32;

                            /****/

                            s0 = ppbits[chars[i]];
                            s1 = ppbits[chars[i+1]];
                            s2 = ppbits[chars[i+2]];
                            s3 = ppbits[chars[i+3]];
                            i += 4;

                            I128_COPY_8X(dst_double, 0);
                            I128_COPY_8X(dst_double, 1);
                            I128_COPY_8X(dst_double, 2);
                            I128_COPY_8X(dst_double, 3);
                            I128_COPY_8X(dst_double, 4);
                            I128_COPY_8X(dst_double, 5);
                            I128_COPY_8X(dst_double, 6);
                            I128_COPY_8X(dst_double, 7);
                            I128_COPY_8X(dst_double, 8);
                            for (j=9; j < height; j++)
                                I128_COPY_8X(dst_double, j);

                            i128Priv->engine->xy0 = I128_XY((I128_CACHE_SIZE/2), 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1);
                            x1 += 32;

                        }
                        break;
                    }

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                    while (i < count)
                    {
                        s0 = (unsigned char*)ppbits[chars[i++]];
                        I128_WAIT_FOR_CACHE(i128Priv->engine);
                        for (j=0; j<height; j++)
                            dst[j] = (unsigned long)(s0[j]);
                        I128_TRIGGER_CACHE(i128Priv->engine);
                        i128Priv->engine->xy0 = I128_XY(0, 0); 
                        i128Priv->engine->xy2 = I128_XY(width, height); 
                        i128Priv->engine->xy1 = I128_XY(x1, y1);
                        x1 += width;
                    }

                }
                break;

              case 9:
                {
                    register unsigned short *s0, *s1, *s2;
                    int cnt = count - 6;

                    dst = (unsigned long*)i128Priv->info.xy_win[0].pointer;
                    i128Priv->engine->xy2 = I128_XY(27, height);
                    i128Priv->engine->xy0 = 0;

                    switch (height)
                    {
                      case 15:
                        while (i <= cnt)
                        {
                            s0 = (unsigned short*)ppbits[chars[i]];
                            s1 = (unsigned short*)ppbits[chars[i+1]];
                            s2 = (unsigned short*)ppbits[chars[i+2]];
                            i += 3;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_9X(dst, 0);
                            I128_COPY_9X(dst, 1);
                            I128_COPY_9X(dst, 2);
                            I128_COPY_9X(dst, 3);
                            I128_COPY_9X(dst, 4);
                            I128_COPY_9X(dst, 5);
                            I128_COPY_9X(dst, 6);
                            I128_COPY_9X(dst, 7);
                            I128_COPY_9X(dst, 8);
                            I128_COPY_9X(dst, 9);
                            I128_COPY_9X(dst, 10);
                            I128_COPY_9X(dst, 11);
                            I128_COPY_9X(dst, 12);
                            I128_COPY_9X(dst, 13);
                            I128_COPY_9X(dst, 14);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 27;

                            /****/

                            s0 = (unsigned short*)ppbits[chars[i]];
                            s1 = (unsigned short*)ppbits[chars[i+1]];
                            s2 = (unsigned short*)ppbits[chars[i+2]];
                            i += 3;

                            I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                            I128_COPY_9X(dst_double, 0);
                            I128_COPY_9X(dst_double, 1);
                            I128_COPY_9X(dst_double, 2);
                            I128_COPY_9X(dst_double, 3);
                            I128_COPY_9X(dst_double, 4);
                            I128_COPY_9X(dst_double, 5);
                            I128_COPY_9X(dst_double, 6);
                            I128_COPY_9X(dst_double, 7);
                            I128_COPY_9X(dst_double, 8);
                            I128_COPY_9X(dst_double, 9);
                            I128_COPY_9X(dst_double, 10);
                            I128_COPY_9X(dst_double, 11);
                            I128_COPY_9X(dst_double, 12);
                            I128_COPY_9X(dst_double, 13);
                            I128_COPY_9X(dst_double, 14);

                            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 27;

                        }
                        break;

                      default:
                        while (i <= cnt)
                        {
                            s0 = (unsigned short*)ppbits[chars[i]];
                            s1 = (unsigned short*)ppbits[chars[i+1]];
                            s2 = (unsigned short*)ppbits[chars[i+2]];
                            i += 3;

                            I128_COPY_9X(dst, 0);
                            I128_COPY_9X(dst, 1);
                            I128_COPY_9X(dst, 2);
                            I128_COPY_9X(dst, 3);
                            I128_COPY_9X(dst, 4);
                            I128_COPY_9X(dst, 5);
                            I128_COPY_9X(dst, 6);
                            I128_COPY_9X(dst, 7);
                            I128_COPY_9X(dst, 8);
                            I128_COPY_9X(dst, 9);
                            I128_COPY_9X(dst, 10);
                            I128_COPY_9X(dst, 11);
                            I128_COPY_9X(dst, 12);
                            for (j=13; j<height; j++)
                                I128_COPY_9X(dst, j);

                            i128Priv->engine->xy0 = I128_XY(0, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 27;

                            /****/

                            s0 = (unsigned short*)ppbits[chars[i]];
                            s1 = (unsigned short*)ppbits[chars[i+1]];
                            s2 = (unsigned short*)ppbits[chars[i+2]];
                            i += 3;

                            I128_COPY_9X(dst_double, 0);
                            I128_COPY_9X(dst_double, 1);
                            I128_COPY_9X(dst_double, 2);
                            I128_COPY_9X(dst_double, 3);
                            I128_COPY_9X(dst_double, 4);
                            I128_COPY_9X(dst_double, 5);
                            I128_COPY_9X(dst_double, 6);
                            I128_COPY_9X(dst_double, 7);
                            I128_COPY_9X(dst_double, 8);
                            I128_COPY_9X(dst_double, 9);
                            I128_COPY_9X(dst_double, 10);
                            I128_COPY_9X(dst_double, 11);
                            I128_COPY_9X(dst_double, 12);
                            for (j=13; j<height; j++)
                                I128_COPY_9X(dst_double, j);

                            i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                            i128Priv->engine->xy1 = I128_XY(x1, y1); 
                            x1 += 27;
                        }
                        break;
                    }

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                    while (i < count)
                    {
                        s0 = (unsigned short*)ppbits[chars[i++]];
                        I128_WAIT_FOR_CACHE(i128Priv->engine);
                        for (j=0; j<height; j++)
                            dst[j] = (unsigned long)(s0[j]);
                        I128_TRIGGER_CACHE(i128Priv->engine);
                        i128Priv->engine->xy0 = I128_XY(0, 0);
                        i128Priv->engine->xy2 = I128_XY(width, height); 
                        i128Priv->engine->xy1 = I128_XY(x1, y1);
                        x1 += width;
                    }

                }
                break;

              case 10:
                {
                    register unsigned short *s0, *s1, *s2;
                    int cnt = count - 6;

                    dst = (unsigned long*)i128Priv->info.xy_win[0].pointer;
                    i128Priv->engine->xy2 = I128_XY(30, height);
                    i128Priv->engine->xy0 = 0;

                    while (i <= cnt)
                    {
                        s0 = (unsigned short*)ppbits[chars[i]];
                        s1 = (unsigned short*)ppbits[chars[i+1]];
                        s2 = (unsigned short*)ppbits[chars[i+2]];
                        i += 3;

                        I128_COPY_10X(dst, 0);
                        I128_COPY_10X(dst, 1);
                        I128_COPY_10X(dst, 2);
                        I128_COPY_10X(dst, 3);
                        I128_COPY_10X(dst, 4);
                        I128_COPY_10X(dst, 5);
                        I128_COPY_10X(dst, 6);
                        I128_COPY_10X(dst, 7);
                        I128_COPY_10X(dst, 8);
                        I128_COPY_10X(dst, 9);
                        I128_COPY_10X(dst, 10);
                        I128_COPY_10X(dst, 11);
                        I128_COPY_10X(dst, 12);
                        for (j=13; j<height; j++)
                            I128_COPY_10X(dst, j);

                        i128Priv->engine->xy0 = I128_XY(0, 0);
                        i128Priv->engine->xy1 = I128_XY(x1, y1); 
                        x1 += 30;

                        /****/

                        s0 = (unsigned short*)ppbits[chars[i]];
                        s1 = (unsigned short*)ppbits[chars[i+1]];
                        s2 = (unsigned short*)ppbits[chars[i+2]];
                        i += 3;

                        I128_COPY_10X(dst_double, 0);
                        I128_COPY_10X(dst_double, 1);
                        I128_COPY_10X(dst_double, 2);
                        I128_COPY_10X(dst_double, 3);
                        I128_COPY_10X(dst_double, 4);
                        I128_COPY_10X(dst_double, 5);
                        I128_COPY_10X(dst_double, 6);
                        I128_COPY_10X(dst_double, 7);
                        I128_COPY_10X(dst_double, 8);
                        I128_COPY_10X(dst_double, 9);
                        I128_COPY_10X(dst_double, 10);
                        I128_COPY_10X(dst_double, 11);
                        I128_COPY_10X(dst_double, 12);
                        for (j=13; j<height; j++)
                            I128_COPY_10X(dst_double, j);

                        i128Priv->engine->xy0 = I128_XY(I128_CACHE_SIZE/2, 0);
                        i128Priv->engine->xy1 = I128_XY(x1, y1); 
                        x1 += 30;

                    }

                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);

                    while (i < count)
                    {
                        s0 = (unsigned short*)ppbits[chars[i++]];
                        I128_WAIT_FOR_CACHE(i128Priv->engine);
                        for (j=0; j<height; j++)
                            dst[j] = (unsigned long)(s0[j]);
                        I128_TRIGGER_CACHE(i128Priv->engine);
                        i128Priv->engine->xy0 = I128_XY(0, 0);
                        i128Priv->engine->xy2 = I128_XY(width, height); 
                        i128Priv->engine->xy1 = I128_XY(x1, y1);
                        x1 += width;
                    }

                }
                break;

              default:
                {
                    register unsigned char *src;

                    i128Priv->engine->xy2 = I128_XY(width, height);
                    while (i < count)
                    {
                        dst = (unsigned long*)
                            ((char*)i128Priv->info.xy_win[0].pointer +
                             double_buffer_offset);
                        src = ppbits[chars[i++]];
                        I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                        for (j=0; j<height; j++)
                        {
                            dst[j] = *((unsigned long*)src);
                            src += stride;
                        }
                        i128Priv->engine->xy0 = xy0;
                        i128Priv->engine->xy1 = I128_XY(x1, y1); 
                        xy0 ^= double_xy0_value;
                        double_buffer_offset ^= I128_CACHE_SIZE/2;
                        x1 += width;
                    }
                }

            }
        else
        {
            register unsigned char *src;

            i128Priv->engine->xy2 = I128_XY(width, height);
            while (i < count)
            {
                dst = (unsigned long*)
                    ((char*)i128Priv->info.xy_win[0].pointer +
                     double_buffer_offset);
                src = ppbits[chars[i++]];
                I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                for (j=0; j<height; j++)
                {
                    dst[j] = *((unsigned long*)src);
                    src += stride;
                }
                i128Priv->engine->xy0 = xy0;
                i128Priv->engine->xy1 = I128_XY(x1, y1); 
                xy0 ^= double_xy0_value;
                double_buffer_offset ^= I128_CACHE_SIZE/2;
                x1 += width;
            }
        }
    }
    else
    {
        num_strips = (width + I128_MAX_WIDTH - 1) >> 5;
        num_chunks = (height + I128_MAX_HEIGHT - 1) >> 5;

        width &= 0x1F;
        if (width == 0)
            width = I128_MAX_WIDTH;

        height &= 0x1F;
        if (height == 0)
            height = I128_MAX_HEIGHT;

        for (i=0; i<count; i++)
        {
            int y;
            image = ppbits[chars[i]];
            strip = num_strips;
            while (strip--)
            {
                chunk = num_chunks;
                y = y1;
                w = strip > 0 ? I128_MAX_WIDTH : width;
                src = image;

                while (chunk--)
                {
                    h = chunk > 0 ? I128_MAX_HEIGHT : height;
                    I128_WAIT_FOR_CACHE(i128Priv->engine);
                    for (j=0; j<h; j++)
                    {
                        dst[j] = *((unsigned long*)src);
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
            x1 = x2;
            x2 += pPS->pFontInfo->width;
        }
    }
    
    I128_WAIT_UNTIL_DONE(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;
}


void
i128DrawMonoGlyphs(
                   nfbGlyphInfo *glyph_info,
                   unsigned int nglyphs,
                   unsigned long fg,
                   unsigned char alu,
                   unsigned long planemask,
                   nfbFontPSPtr pPS,
                   DrawablePtr pDrawable)
{
    i128PrivatePtr i128Priv =
        I128_PRIVATE_DATA(pDrawable->pScreen);
    unsigned long *dst = (unsigned long *)i128Priv->info.xy_win[0].pointer;
    unsigned char *src, *image;
    int i, j;
    int num_longs;
    unsigned long buf_control, cmd;
    long double_buffer_offset = 0;
    long double_xy0_value = I128_CACHE_SIZE/2 << 16;
    long xy0 = 0;


    buf_control = i128Priv->engine->buf_control;
    cmd  = (i128Priv->rop[alu] |
            I128_OPCODE_BITBLT |
            I128_CMD_CLIP_IN  |
            I128_CMD_PATT_RESET |
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
    i128Priv->engine->xy3 = I128_DIR_LR_TB;
    i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;

    while (nglyphs--)
    {
        int num_strips, num_chunks, strip, chunk;
        int width, height, w, h, x1, y1;

        image = glyph_info->image;
        x1 = glyph_info->box.x1;
        width = glyph_info->box.x2 - x1;
        while (width > 0)
        {
            y1 = glyph_info->box.y1;
            height = glyph_info->box.y2 - y1;
            w = width > I128_MAX_WIDTH ? I128_MAX_WIDTH : width;
            src = image;
            while (height > 0)
            {
                dst = (unsigned long*)
                    ((unsigned char *)i128Priv->info.xy_win[0].pointer +
                     double_buffer_offset);
                I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                for (h=0; height && h<I128_MAX_HEIGHT/2; h++, height--)
                {
                    dst[h] = *((unsigned long*)src);
                    src += glyph_info->stride;
                }
                i128Priv->engine->xy0 = xy0;
                i128Priv->engine->xy2 = I128_XY(w, h);
                i128Priv->engine->xy1 = I128_XY(x1, y1);
                xy0 ^= double_xy0_value;
                double_buffer_offset ^= I128_CACHE_SIZE/2;
                y1 += h;
            }
            image += 4;
            x1 += w;
            width -= w;
        }
        ++glyph_info;
    }
    I128_WAIT_UNTIL_READY(i128Priv->engine);
    i128Priv->engine->buf_control = buf_control;
}


