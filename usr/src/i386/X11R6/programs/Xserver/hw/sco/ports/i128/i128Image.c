/*
 * @(#) i128Image.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "i128Defs.h"


#if 0

/*
 * i128ReadImage() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte;
 *		pack 2- to 8-bit pixels one per byte;
 *		pack 9- to 16-bit pixels one per 16-bit short;
 *		pack 17- to 32-bit pixels one per 32-bit word.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void
i128ReadImage(pbox, image, stride, pDraw)
     BoxPtr pbox;
     void *image;
     unsigned int stride;
     DrawablePtr pDraw;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     int dY, dX, x1, y1;
     unsigned char *nextline;
     unsigned char *chardst = (unsigned char *)image;
     unsigned short *worddst = (unsigned short *)image;
     unsigned long *longdst = (unsigned long *)image;
     volatile unsigned long *lsrc;
     register int i, j;
     unsigned long pixval;
     long  tmp;

     /* width and height in pixels */
     dX = pbox->x2 - pbox->x1;
     dY = pbox->y2 - pbox->y1;
     x1 = pbox->x1;
     y1 = pbox->y1;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->cmd = I128_ROP_COPY | I128_CMD_CLIP_IN; 
     i128Priv->engine->cmd_opcode = I128_OPCODE_RXFER;
     lsrc = (unsigned long *)i128Priv->info.xy_win[0].pointer;
     i128Priv->engine->xy0 = I128_XY(0, 0);
     i128Priv->engine->xy2 = I128_XY(dX, dY);
     I128_WAIT_UNTIL_DONE(i128Priv->engine);
     i128Priv->engine->xy1 = I128_XY(x1, y1);

     switch (i128Priv->engine->buf_control & I128_FLAG_NO_BPP)
     {
        case I128_FLAG_8_BPP:
          while (dY--)
          {
               nextline = chardst + stride;
               for (j = 0; j < dX; j++)
               {
                    if ((j & 3) == 0)
                         pixval = *lsrc;
                    *chardst++ = (unsigned char)pixval;
                    pixval >>= 8;
                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
               }
               chardst = nextline;
               I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
          }
          break;

        case I128_FLAG_16_BPP:
          for (i = 0; i < dY; i++)
          {
               nextline = (unsigned char *)worddst + stride;
               for (j = 0; j < dX; j++)
               {
                    if ((j & 1) == 0)
                         pixval = *lsrc;
                    *worddst++ = pixval;
                    pixval >>= 16;
                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
               }
               worddst = (unsigned short *)nextline;
               I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
          }
          break;

        case I128_FLAG_32_BPP:
        default:
          for (i = 0; i < dY; i++)
          {
               nextline = (unsigned char *)longdst + stride;
               for (j = 0; j < dX; j++)
               {
                    *longdst++ = *lsrc;
                    I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
               }
               longdst = (unsigned long *)nextline;
               I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
          }
          break;
     }
     pixval = *lsrc;            /* hang workaround */
}

#else

void
i128ReadImage(pbox, image, stride, pDraw)
     BoxPtr pbox;
     void *image;
     unsigned int stride;
     DrawablePtr pDraw;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     int w, h;
     int w_long, w_char;
     register int i;
     unsigned char *src, *dst;
     unsigned long *src_long, *dst_long;
     
     w = (pbox->x2 - pbox->x1) * i128Priv->mode.pixelsize;
     h = pbox->y2 - pbox->y1;
     dst = (unsigned char*)image;
     dst_long = (unsigned long*)image;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     
/*
     while ((volatile)i128Priv->info.memwins[0]->control &
            I128_MEMW_BUSY)
          I128_NOOP;
*/
#if 0
     i128Priv->info.memwins[0]->origin     = 0;
     i128Priv->info.memwins[0]->page       = 0;
     switch (i128Priv->mode.pixelsize)
     {
        case 1:
          i128Priv->info.memwins[0]->control    =
               (I128_MEMW_SRC_DISP |
                I128_MEMW_8_BPP);
          break;
        case 2:
          i128Priv->info.memwins[0]->control    =
               (I128_MEMW_SRC_DISP |
                I128_MEMW_16_BPP);
          break;
        default:
          i128Priv->info.memwins[0]->control    =
               (I128_MEMW_SRC_DISP |
                I128_MEMW_32_BPP);
          break;
     }
     
     i128Priv->info.memwins[0]->key        = 0;
     i128Priv->info.memwins[0]->key_data   = 0;
     i128Priv->info.memwins[0]->msk_source = 0;
#endif

     i128Priv->info.memwins[0]->plane_mask = 0xFFFFFFFF;

     src = (unsigned char*)i128Priv->info.memwin[0].pointer +
          i128Priv->mode.bitmap_pitch * pbox->y1 +
               pbox->x1 * i128Priv->mode.pixelsize;
     src_long = (unsigned long*)src;
     w_long = w/4;
     while (--h >= 0) {
          for (i=0; i<w_long; i++)
              dst_long[i] = src_long[i];
          for (i=i*4; i<w; i++)
              dst[i] = src[i];
          /* memcpy(dst, src, w);  */
          dst += stride;
          src += i128Priv->mode.bitmap_pitch;
          dst_long = (unsigned long *)dst;
          src_long = (unsigned long *)src;
     }

     I128_WAIT_UNTIL_DONE(i128Priv->engine);

}

#endif


/*
 * i128DrawImage() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void
i128DrawImage(pbox, image, stride, alu, planemask, pDraw)
     BoxPtr pbox;
     void *image;
     unsigned int stride;
     unsigned char alu;
     unsigned long planemask;
     DrawablePtr pDraw;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pDraw->pScreen);
     int h, w, x1, y1;
     unsigned char *im = (unsigned char*)image;

     x1 = pbox->x1;
     y1 = pbox->y1;
     w = pbox->x2 - pbox->x1;
     h = pbox->y2 - pbox->y1;

     planemask = I128_CONVERT(i128Priv->mode.pixelsize, planemask);

#if 0
     if ((planemask == 0xFFFFFFFF) &&
         (alu == GXcopy) &&
         (i128Priv->clip.x1 == 0) &&
         (i128Priv->clip.x2 == 0) &&
         (i128Priv->mode.bitmap_width <= i128Priv->clip.x2) &&
         (i128Priv->mode.bitmap_height <= i128Priv->clip.y2))
     {
          unsigned char *src = (unsigned char*)image;
          unsigned char *dst;

          I128_WAIT_UNTIL_READY(i128Priv->engine);

#if 0
          while ((volatile)i128Priv->info.memwins[0]->control &
                 I128_MEMW_BUSY)
               I128_NOOP;
          i128Priv->info.memwins[0]->origin     = 0;
          i128Priv->info.memwins[0]->page       = 0;

          switch (i128Priv->mode.pixelsize)
          {
             case 1:
               i128Priv->info.memwins[0]->control    =
                    (I128_MEMW_DST_DISP |
                     I128_MEMW_8_BPP);
               break;
             case 2:
               i128Priv->info.memwins[0]->control    =
                    (I128_MEMW_DST_DISP |
                     I128_MEMW_16_BPP);
               break;
             default:
               i128Priv->info.memwins[0]->control    =
                    (I128_MEMW_DST_DISP |
                     I128_MEMW_32_BPP);
               break;
          }

          i128Priv->info.memwins[0]->key        = 0;
          i128Priv->info.memwins[0]->key_data   = 0;
          i128Priv->info.memwins[0]->msk_source = 0;
#endif

          i128Priv->info.memwins[0]->plane_mask = planemask;
          
          dst = (unsigned char*)i128Priv->info.memwin[0].pointer +
               i128Priv->mode.bitmap_pitch * pbox->y1 +
                    pbox->x1 * i128Priv->mode.pixelsize;

          while (--h >= 0) {
               memcpy(dst, src, w);
               src += stride;
               dst += i128Priv->mode.bitmap_pitch;
          }

     }
     else
#endif
     {
          unsigned long volatile *dst =
               (unsigned long*)i128Priv->info.xy_win[0].pointer;
          unsigned long *src;
          int pix_per_word = sizeof(long)/i128Priv->mode.pixelsize;
          register int i, l_stride;

          /* Compute stride of dest. to min number of words */
          l_stride = (w + pix_per_word - 1)/pix_per_word;

          I128_WAIT_UNTIL_READY(i128Priv->engine);
          I128_WAIT_UNTIL_DONE(i128Priv->engine);
          i128Priv->engine->cmd =
               (I128_OPCODE_WXFER |
                i128Priv->rop[alu] |
                I128_CMD_CLIP_IN);

          i128Priv->engine->plane_mask = planemask;
          i128Priv->engine->xy0 = I128_XY(0, 0);
          i128Priv->engine->xy2 = I128_XY(w, h);

          I128_WAIT_UNTIL_DONE(i128Priv->engine);
          i128Priv->engine->xy1 = I128_XY(x1, y1);

          while (h-- > 0)
          {
               src = (unsigned long *)im;
               for (i=0; i<l_stride; )
               {
                   register j;
                   for (j=0; j<I128_CACHE_SIZE/8 && i<l_stride; j++)
                       dst[j] = src[i++];
                   I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
                   for ( ; j<I128_CACHE_SIZE/4 && i<l_stride; j++)
                       dst[j] = src[i++];
               }
               im += stride;
               I128_WAIT_FOR_PREVIOUS(i128Priv->engine);
          }
     }
     I128_WAIT_UNTIL_DONE(i128Priv->engine);
}

