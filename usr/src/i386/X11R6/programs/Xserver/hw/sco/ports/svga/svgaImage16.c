/*
 * @(#)svgaImage16.c 11.1
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * svgaImage16.c
 *
 * svga ReadImage and DrawImage routines for depths 9 through 16.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"
#include "svgaDefs.h"


/*
 * svgaReadImage16() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 16-bit pixels one per short.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
svgaReadImage16(pbox, imagePtr, stride, pDraw)
     register BoxPtr pbox;
     void *imagePtr;
     unsigned int stride;
     DrawablePtr pDraw;
{
  register svgaPrivatePtr pSvga = SVGA_PRIVATE_DATA(pDraw->pScreen);
  unsigned char *image = (unsigned char *)imagePtr;
  unsigned char *psrc;
  int w, h;

#ifdef DEBUG_PRINT
  ErrorF("svgaReadImage16(box=(%d,%d)-(%d,%d), "
         "image=0x%x, stride=%d)\n",
         pbox->x1, pbox->y1, pbox->x2, pbox->y2,
         image, stride);
#endif

  /* width and height in bytes of image */
  w = (pbox->x2 - pbox->x1) << 1;
  h = pbox->y2 - pbox->y1;
  psrc =
    pSvga->fbBase +
    (pbox->y1 * pSvga->fbStride) +
    (pbox->x1 << 1);

  while (--h >= 0) {
    memcpy(image, psrc, w);
    image += stride;
    psrc  += pSvga->fbStride;
  }
}



/*
 * svgaDrawImage16() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */

#define ALLPLANES	0xFFFF

#define	InvertWithMask(dst, mask) \
((dst) ^ (mask))

#define	SRCLOOP(func) \
     while (--h >= 0) { \
                          psrc = (unsigned short *)image; \
                                                            image += stride; \
                                                                               pdst = (unsigned short *)pdstLine; \
                                                                                                                    pdstLine += svgaStride; \
                                                                                                                                              cnt = w; \
                                                                                                                                                         while (--cnt >= 0) { \
                                                                                                                                                                                *pdst = func; \
                                                                                                                                                                                                ++pdst; \
                                                                                                                                                                                                          } \
                                                                                                                                                                                                              }

#define	NOSRCLOOP(func) \
while (--h >= 0) { \
                     pdst = (unsigned short *)pdstLine; \
                                                          pdstLine += svgaStride; \
                                                                                    cnt = w; \
                                                                                               while (--cnt >= 0) { \
                                                                                                                      *pdst = func; \
                                                                                                                                      ++pdst; \
                                                                                                                                                } \
                                                                                                                                                    }


void 
svgaDrawImage16(pbox, imagePtr, stride, alu, planemask, pDraw)
     BoxPtr pbox;
     void *imagePtr;
     unsigned int stride;
     unsigned char alu;
     unsigned long planemask;
     DrawablePtr pDraw;
{
  register svgaPrivatePtr pSvga = SVGA_PRIVATE_DATA(pDraw->pScreen);
  unsigned char *svgaBase = pSvga->fbBase;
  int svgaStride = pSvga->fbStride;
  unsigned char *pdstLine;
  register unsigned short *psrc, *pdst;
  unsigned char *image = (unsigned char *)imagePtr;
  register int cnt;
  int h, w;
  int tmpy;

#ifdef DEBUG_PRINT
  ErrorF("svgaDrawImage16(box=(%d,%d)-(%d,%d), "
         "image=0x%x, stride=%d, alu=%d, planemask=0x%x)\n",
         pbox->x1, pbox->y1, pbox->x2, pbox->y2,
         image, stride, alu, planemask);
#endif

  /* width and height in bytes */
  w = pbox->x2 - pbox->x1;
  h = pbox->y2 - pbox->y1;
  pdstLine = svgaBase + (pbox->y1 * svgaStride) + (pbox->x1 << 1);

  if ((planemask &= ALLPLANES) == 0)
    return;

  if ((planemask & ALLPLANES) == ALLPLANES)
    {
      switch (alu)
        {
        case GXclear:
          while (--h >= 0)
            {
              memset(pdstLine, 0, w << 1);
              pdstLine += svgaStride;
            }
          return;
        case GXand:
          SRCLOOP(fnAND(*psrc++, *pdst));
          return;
        case GXandReverse:
          SRCLOOP(fnANDREVERSE(*psrc++, *pdst));
          return;
        case GXcopy:
          while (--h >= 0)
            {
              memcpy(pdstLine, image, w << 1);
              image += stride;
              pdstLine += svgaStride;
            }
          return;
        case GXandInverted:
          SRCLOOP(fnANDINVERTED(*psrc++, *pdst));
          return;
        case GXnoop:
          return;
        case GXxor:
          SRCLOOP(fnXOR(*psrc++, *pdst));
          return;
        case GXor:
          SRCLOOP(fnOR(*psrc++, *pdst));
          return;
        case GXnor:
          SRCLOOP(fnNOR(*psrc++, *pdst));
          return;
        case GXequiv:
          SRCLOOP(fnEQUIV(*psrc++, *pdst));
          return;
        case GXinvert:
          NOSRCLOOP(fnINVERT(foo, *pdst));
          return;
        case GXorReverse:
          SRCLOOP(fnORREVERSE(*psrc++, *pdst));
          return;
        case GXcopyInverted:
          SRCLOOP(fnCOPYINVERTED(*psrc++, foo));
          return;
        case GXorInverted:
          SRCLOOP(fnORINVERTED(*psrc++, *pdst));
          return;
        case GXnand:
          SRCLOOP(fnNAND(*psrc++, *pdst));
          return;
        case GXset:
          while (--h >= 0)
            {
              memset(pdstLine, ALLPLANES, w << 1);
              pdstLine += svgaStride;
            }
          return;
        }
    }
  else
    {                            /* planemask != ALLPLANES */
      switch (alu)
        {
        case GXclear:
          NOSRCLOOP(ClearWithMask(*pdst,planemask));
          return;
        case GXand:
          SRCLOOP(RopWithMask(fnAND(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXandReverse:
          SRCLOOP(RopWithMask(fnANDREVERSE(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXcopy:
          SRCLOOP(RopWithMask(fnCOPY(*psrc++,foo),*pdst,planemask));
          return;
        case GXandInverted:
          SRCLOOP(RopWithMask(fnANDINVERTED(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXnoop:
          return;
        case GXxor:
          SRCLOOP(RopWithMask(fnXOR(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXor:
          SRCLOOP(RopWithMask(fnOR(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXnor:
          SRCLOOP(RopWithMask(fnNOR(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXequiv:
          SRCLOOP(RopWithMask(fnEQUIV(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXinvert:
          NOSRCLOOP(InvertWithMask(*pdst,planemask));
          return;
        case GXorReverse:
          SRCLOOP(RopWithMask(fnORREVERSE(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXcopyInverted:
          SRCLOOP(RopWithMask(fnCOPYINVERTED(*psrc++,foo),*pdst,planemask));
          return;
        case GXorInverted:
          SRCLOOP(RopWithMask(fnORINVERTED(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXnand:
          SRCLOOP(RopWithMask(fnNAND(*psrc++,*pdst),*pdst,planemask));
          return;
        case GXset:
          NOSRCLOOP(SetWithMask(*pdst,planemask));
          return;
        }
    }
}
