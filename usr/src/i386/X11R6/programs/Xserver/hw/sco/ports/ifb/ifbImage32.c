/*
 *	@(#) ifbImage32.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * ifbImage32.c
 *
 * ifb ReadImage and DrawImage routines for depths 17 through 32.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbRop.h"

extern pointer	ifbBase[MAXSCREENS];			/* Base address */
extern int	ifbStride[MAXSCREENS];			/* Scanline length */

/*
 * ifbReadImage32() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 32-bit pixels one per long.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	pDraw - the window from which to read.
 */
void 
ifbReadImage32(pbox, image, stride, pDraw)
    register BoxPtr pbox;
    unsigned char *image;
    unsigned int stride;
    DrawablePtr pDraw;
{
    int w, h, idx;
    unsigned char *psrc;

    w = (pbox->x2 - pbox->x1) << 2;
    h = pbox->y2 - pbox->y1;
    idx = pDraw->pScreen->myNum;
    psrc = (unsigned char *)ifbBase[idx]
		    + (pbox->y1 * ifbStride[idx]) + (pbox->x1 << 2);

    while (--h >= 0) {
	memcpy(image, psrc, w);
	image += stride;
	psrc  += ifbStride[idx];
    }
}


#define	InvertWithMask(dst, mask) \
	((dst) ^ (mask))

#define	SRCLOOP(func) \
	while (--h >= 0) { \
	    psrc = (unsigned long *)image; \
	    image += stride; \
	    pdst = (unsigned long *)pdstLine; \
	    pdstLine += ifbStride[idx]; \
	    cnt = w; \
	    while (--cnt >= 0) { \
		*pdst = func; \
		++pdst; \
	    } \
	}

#define	NOSRCLOOP(func) \
	while (--h >= 0) { \
	    pdst = (unsigned long *)pdstLine; \
	    pdstLine += ifbStride[idx]; \
	    cnt = w; \
	    while (--cnt >= 0) { \
		*pdst = func; \
		++pdst; \
	    } \
	}


/*
 * ifbDrawImage32() - Draw pixels in a rectangular area of a window.
 *	pbox - the rectangle to draw into.
 *	image - the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void 
ifbDrawImage32(pbox, image, stride, alu, planemask, pDraw)
    BoxPtr pbox;
    unsigned char *image;
    unsigned int stride;
    unsigned char alu;
    unsigned long planemask;
    DrawablePtr pDraw;
{
    unsigned long allplanes = (unsigned long)0xFFFFFFFF >> (32 - pDraw->depth);
    int w, h, idx;
    int tmpy;
    unsigned char *pdstLine;
    register unsigned long *psrc, *pdst;
    register int cnt;

    w = pbox->x2 - pbox->x1;
    h = pbox->y2 - pbox->y1;
    idx = pDraw->pScreen->myNum;
    pdstLine = (unsigned char *)ifbBase[idx]
		    + (pbox->y1 * ifbStride[idx]) + (pbox->x1 << 2);

    if ((planemask &= allplanes) == 0)
	return;

    if (planemask == allplanes) {

	switch (alu) {
	case GXclear:
	    while (--h >= 0) {
		memset(pdstLine, 0, w << 2);
		pdstLine += ifbStride[idx];
	    }
	    return;
	case GXand:
	    SRCLOOP(fnAND(*psrc++, *pdst))
	    return;
	case GXandReverse:
	    SRCLOOP(fnANDREVERSE(*psrc++, *pdst))
	    return;
	case GXcopy:
	    while (--h >= 0) {
		memcpy(pdstLine, image, w << 2);
		image += stride;
		pdstLine += ifbStride[idx];
	    }
	    return;
	case GXandInverted:
	    SRCLOOP(fnANDINVERTED(*psrc++, *pdst))
	    return;
	case GXnoop:
	    return;
	case GXxor:
	    SRCLOOP(fnXOR(*psrc++, *pdst))
	    return;
	case GXor:
	    SRCLOOP(fnOR(*psrc++, *pdst))
	    return;
	case GXnor:
	    SRCLOOP(fnNOR(*psrc++, *pdst))
	    return;
	case GXequiv:
	    SRCLOOP(fnEQUIV(*psrc++, *pdst))
	    return;
	case GXinvert:
	    NOSRCLOOP(fnINVERT(foo, *pdst))
	    return;
	case GXorReverse:
	    SRCLOOP(fnORREVERSE(*psrc++, *pdst))
	    return;
	case GXcopyInverted:
	    SRCLOOP(fnCOPYINVERTED(*psrc++, foo))
	    return;
	case GXorInverted:
	    SRCLOOP(fnORINVERTED(*psrc++, *pdst))
	    return;
	case GXnand:
	    SRCLOOP(fnNAND(*psrc++, *pdst))
	    return;
	case GXset:
	    while (--h >= 0) {
		memset(pdstLine, allplanes, w << 2);
		pdstLine += ifbStride[idx];
	    }
	    return;
	}

    } else {  /* planemask != allplanes */

	switch (alu) {
	case GXclear:
	    NOSRCLOOP(ClearWithMask(*pdst,planemask))
	    return;
	case GXand:
	    SRCLOOP(RopWithMask(fnAND(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXandReverse:
	    SRCLOOP(RopWithMask(fnANDREVERSE(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXcopy:
	    SRCLOOP(RopWithMask(fnCOPY(*psrc++,foo),*pdst,planemask))
	    return;
	case GXandInverted:
	    SRCLOOP(RopWithMask(fnANDINVERTED(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXnoop:
	    return;
	case GXxor:
	    SRCLOOP(RopWithMask(fnXOR(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXor:
	    SRCLOOP(RopWithMask(fnOR(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXnor:
	    SRCLOOP(RopWithMask(fnNOR(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXequiv:
	    SRCLOOP(RopWithMask(fnEQUIV(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXinvert:
	    NOSRCLOOP(InvertWithMask(*pdst,planemask))
	    return;
	case GXorReverse:
	    SRCLOOP(RopWithMask(fnORREVERSE(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXcopyInverted:
	    SRCLOOP(RopWithMask(fnCOPYINVERTED(*psrc++,foo),*pdst,planemask))
	    return;
	case GXorInverted:
	    SRCLOOP(RopWithMask(fnORINVERTED(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXnand:
	    SRCLOOP(RopWithMask(fnNAND(*psrc++,*pdst),*pdst,planemask))
	    return;
	case GXset:
	    NOSRCLOOP(SetWithMask(*pdst,planemask))
	    return;
	}

    }
}
