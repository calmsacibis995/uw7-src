/*
 *	@(#)ctImage.c	11.1	10/22/97	12:10:42
 *	@(#) ctImage.c 12.1 95/05/09 
 *      ctImage.c 9.1 93/03/01 
 *
 * Copyright (C) 1994-1996 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 *	SCO Modifications
 *
 *	S001	Wed Jun 01 10:34:01 PDT 1994	hiramc@sco.COM
 *	- Must use a simple char * for Microsoft inlined _asm code.
 */
/*
 * ctImage.c
 *
 * Template for machine dependent ReadImage and DrawImage routines
 */

#ident "@(#) $Id$"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbRop.h"

#include "ctDefs.h"
#include "ctMacros.h"

extern void CT(DrawSolidRects)();
extern void CT(DrawImageFB)();

/*******************************************************************************

				Private Routines

*******************************************************************************/

#define	InvertWithMask(dst, mask)	((dst) ^ (mask))

#define	SRCLOOP(func) {							\
	while (height--) {						\
		psrc = (CT_PIXEL *)psrc_start;				\
		psrc_start += stride;					\
		pdst = pdst_start;					\
		pdst_start += ctPriv->fbStride;				\
		w = width;						\
		while (w--) {						\
			*pdst = func;					\
			psrc++;						\
			pdst++;						\
		}							\
	}								\
}

#define	NOSRCLOOP(func) {						\
	while (height--) {						\
		pdst = pdst_start;					\
		pdst_start += ctPriv->fbStride;				\
		w = width;						\
		while (w--) {						\
			*pdst = func;					\
			pdst++;						\
		}							\
	}								\
}

static void 
ctDrawImageSlow(pDraw, alu, x, y, width, height, image, stride)
DrawablePtr pDraw;
unsigned char alu;
int x;
int y;
int width;
int height;
void *image;
unsigned int stride;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int w, wib, dwidth, scanwidth, delta;
	unsigned long *pdst = (unsigned long *)ctPriv->fbPointer;
	unsigned long *psrc;
	unsigned char *pscan_start;
	unsigned long image_addr;

	wib = width * sizeof(CT_PIXEL);

	/*
	 * dword-align the image pointer and calculate the delta between the
	 * given pointer and the aligned pointer. Also, calculate the scanwidth
	 * in bytes.
	 */
	image_addr = (unsigned long)image;
	pscan_start = (unsigned char *)(image_addr & ~3);
	delta = image_addr - (unsigned long)pscan_start;
	scanwidth = (int)(((image_addr + wib + 3) & ~3) - (int)pscan_start);
	dwidth = scanwidth >> 2;

#ifdef DEBUG_PRINT
	ErrorF("DrawImageSlow(): 0x%08x str=%d sw=%d (%d,%d)(%dx%d)\n",
		image, stride, scanwidth, x, y, width, height);
#endif

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, from memory to screen.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_BLTMEMORYBIT);
	CT_SET_BLTOFFSET(scanwidth, ctPriv->bltStride);
	CT_SET_BLTSRC(delta);
	CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
	CT_START_BITBLT(width, height);

	/*
	 * NOTE: We don't increment the destination pointer here since any
	 * write to a valid 64300/301 memory address will be recognized as
	 * BitBlt source data and will be routed to the correct screen address.
	 */
	while (height--) {
		psrc = (unsigned long *)pscan_start;
		w = dwidth;
		while (w--) {
			*pdst = *psrc++;
		}
		pscan_start += stride;	/* stride MUST be 32-bit padded */
	}

#ifdef CT_BITBLT_SANITY_CHECK
	CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * CT(ReadImage)() - Read a rectangular area of a window into image.
 *	pbox - the rectangle to read.
 *	image - where to write the pixels.  pack 1-bit pixels eight per byte;
 *		pack 2- to 8-bit pixels one per byte;
 *		pack 9- to 16-bit pixels one per 16-bit short;
 *		pack 17- to 32-bit pixels one per 32-bit word.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image. When height == 1, stride is ignored.
 *	pDraw - the window from which to read.
 */
void 
CT(ReadImage)(pbox, image, stride, pDraw)
BoxPtr pbox;
void *image;
unsigned int stride;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	CT_PIXEL *psrc;
	unsigned char *pdst = (unsigned char *)image;
	int height, wib;

#ifdef DEBUG_PRINT
	ErrorF("ReadImage(): (%d,%d)(%d,%d) image=0x%08x stride=%d\n",
		pbox->x1, pbox->y1, pbox->x2, pbox->y2, image, stride);
#endif

	wib = (pbox->x2 - pbox->x1) * sizeof(CT_PIXEL);
	height = pbox->y2 - pbox->y1;

	psrc = CT_SCREEN_ADDRESS(ctPriv, pbox->x1, pbox->y1);

	CT_WAIT_FOR_IDLE();

	while (height--) {
		memcpy(pdst, psrc, wib);
		psrc += ctPriv->fbStride;
		pdst += stride;	/* ignored if height == 1 */
	}
}

/*
 * CT(DrawImage)() - Draw pixels in a rectangular area of a window.
 *
 *	pbox - the rectangle to draw into.
 *	image - pointer to the pixels to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image. When height == 1, stride is ignored.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 *
 * NOTE: XXX Use pattern rop register for planemask
 */
void
CT(DrawImage)(pbox, image, stride, alu, planemask, pDraw)
BoxPtr pbox;
void *image;
unsigned int stride;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int wib, ndwords, height, width;
	unsigned long *pdst = (unsigned long *)ctPriv->fbPointer;
	unsigned long *psrc = (unsigned long *)image;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		/*
		 * No HW planemask support!
		 */
		CT(DrawImageFB)(pbox, image, stride, alu, planemask, pDraw);
		return;
	}

	switch (alu) {
	case GXnoop:
		return;
	case GXclear:
	case GXset:
	case GXinvert:
		/*
		 * The specified ALU ignores both tile and foreground values.
		 */
		CT(DrawSolidRects)(pbox, 1, 0L, alu, planemask, pDraw);
		return;
	default:
		width = pbox->x2 - pbox->x1;
		height = pbox->y2 - pbox->y1;
		wib = width * sizeof(CT_PIXEL);

		if ((height != 1) && ((stride & (4 - 1)) != 0)) {
			/*
			 * The BitBlt engine routines require a 32-bit padded
			 * stride.
			 */
			CT(DrawImageFB)(pbox, image, stride, alu, ~0, pDraw);
			return;
		}
		if (((unsigned long)image & 3) ||
		    ((wib != stride) && (height != 1))) {
			/*
			 * We need to correct for non-dword-aligned image
			 * pointer and/or write data scanline-at-a-time because
			 * the image is is a sub-image within a larger pixmap.
			 */
			ctDrawImageSlow(pDraw, alu, pbox->x1, pbox->y1,
					width, height, image, stride);
			return;
		}
		break;
	}

	if (height == 1) {
		/*
		 * NFB passes stride == width if height == 1.
		 */
		ndwords = CT_BYTES_TO_LONGS(wib);
	} else {
		ndwords = CT_BYTES_TO_LONGS(stride) * height;
	}

#ifdef DEBUG_PRINT
	ErrorF("DrawImage(): 0x%08x str=%d dw=%d (%d,%d)(%dx%d)\n",
		image, stride, ndwords, pbox->x1, pbox->x2, width, height);
#endif /* DEBUG_PRINT */

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, from memory to screen.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_BLTMEMORYBIT);
	CT_SET_BLTOFFSET(stride, ctPriv->bltStride);
	CT_SET_BLTSRC(0);
	CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1));
	CT_START_BITBLT(width, height);
	CT_SET_BLTDATA(pdst, psrc, ndwords);

#ifdef CT_BITBLT_SANITY_CHECK
	CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */
}

void
CT(DrawImageFB)(pbox, image, stride, alu, planemask, pDraw)
BoxPtr pbox;
void *image;
unsigned int stride;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned char *psrc_start;
	CT_PIXEL *pdst_start;
	int width, height;
	register CT_PIXEL *psrc, *pdst;
	register int w;
	char * fbPtr = ctPriv->fbPointer;		/*	S001	*/

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

	planemask &= ctPriv->allPlanes;

	if (planemask == (unsigned long)0x00000000L) {
		/*
		 * No planes.
		 */
		return;
	}

	CT_WAIT_FOR_IDLE();

	psrc_start = (unsigned char *)image;
	pdst_start = CT_SCREEN_ADDRESS(ctPriv, pbox->x1, pbox->y1);

	switch (alu) {
	case GXclear:
		NOSRCLOOP(ClearWithMask(*pdst,planemask));
		break;
	case GXand:
		SRCLOOP(RopWithMask(fnAND(*psrc,*pdst),*pdst,planemask));
		break;
	case GXandReverse:
		SRCLOOP(RopWithMask(fnANDREVERSE(*psrc,*pdst),*pdst,planemask));
		break;
	case GXcopy:
		SRCLOOP(RopWithMask(fnCOPY(*psrc,0),*pdst,planemask));
		break;
	case GXandInverted:
		SRCLOOP(RopWithMask(fnANDINVERTED(*psrc,*pdst),*pdst,planemask));
		break;
	case GXnoop:
		return;
	case GXxor:
		SRCLOOP(RopWithMask(fnXOR(*psrc,*pdst),*pdst,planemask));
		break;
	case GXor:
		SRCLOOP(RopWithMask(fnOR(*psrc,*pdst),*pdst,planemask));
		break;
	case GXnor:
		SRCLOOP(RopWithMask(fnNOR(*psrc,*pdst),*pdst,planemask));
		break;
	case GXequiv:
		SRCLOOP(RopWithMask(fnEQUIV(*psrc,*pdst),*pdst,planemask));
		break;
	case GXinvert:
		NOSRCLOOP(InvertWithMask(*pdst,planemask));
		break;
	case GXorReverse:
		SRCLOOP(RopWithMask(fnORREVERSE(*psrc,*pdst),*pdst,planemask));
		break;
	case GXcopyInverted:
		SRCLOOP(RopWithMask(fnCOPYINVERTED(*psrc,0),*pdst,planemask));
		break;
	case GXorInverted:
		SRCLOOP(RopWithMask(fnORINVERTED(*psrc,*pdst),*pdst,planemask));
		break;
	case GXnand:
		SRCLOOP(RopWithMask(fnNAND(*psrc,*pdst),*pdst,planemask));
		break;
	case GXset:
		NOSRCLOOP(SetWithMask(*pdst,planemask));
		break;
	}

	CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
}
