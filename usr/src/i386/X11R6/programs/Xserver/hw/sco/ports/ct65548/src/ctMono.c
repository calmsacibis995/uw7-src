/*
 *	@(#)ctMono.c	11.1	10/22/97	12:34:04
 *	@(#) ctMono.c 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctMono.c 58.1 96/10/09 "

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "scoext.h"

#include "ctMacros.h"
#include "ctDefs.h"

extern void CT(DrawSolidRects)();

void
CT(DoDrawMonoImage)(pFB, dstStride, dstOffset, width, height, image, stride)
CT_PIXEL *pFB;
unsigned int dstStride;
unsigned long dstOffset;
int width;
int height;
unsigned char *image;
unsigned int stride;
{
	int w, scanwidth, dwidth;
	unsigned long *pdst = (unsigned long *)pFB;
	unsigned char *psrc;

	scanwidth = (width + (8 - 1)) >> 3;	/* scanwidth in bytes */

#ifdef DEBUG_PRINT
	CT(BitBltDebugF)("DoDrawMonoImage(): 0x%08x s=%d w=%d (%dx%d)\n",
		image, stride, scanwidth, width, height);
#endif /* DEBUG_PRINT */

	/*
	 * NOTE: Assume the caller set the control register and any relevant
	 * color registers. Also, assume the caller waited for the BitBlt engine
	 * to be idle before setting these registers.
	 */
	CT_SET_BLTOFFSET(0, dstStride);
	CT_SET_BLTSRC(0);
	CT_SET_BLTDST(dstOffset);
	CT_START_BITBLT(width, height);		/* number of expanded bytes */

	switch(scanwidth) {
	case 1:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES1(pdst, image);
			image += stride;
		}
		break;
	case 2:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES2(pdst, image);
			image += stride;
		}
		break;
	case 3:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES3(pdst, image);
			image += stride;
		}
		break;
	case 4:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES4(pdst, image);
			image += stride;
		}
		break;
	default:
		/*
		 * Here, we handle arbitrary width image data.
		 */
		dwidth = scanwidth >> 2;		/* in dwords */
		switch (scanwidth & 3) {		/* extra */
		case 0:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
					psrc += 4;
				}
				image += stride;
			}
			break;
		case 1:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES1(pdst, psrc);
				image += stride;
			}
			break;
		case 2:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES2(pdst, psrc);
				image += stride;
			}
			break;
		case 3:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES3(pdst, psrc);
				image += stride;
			}
			break;
		}
		break;
	}

#ifdef CT_BITBLT_SANITY_CHECK
	CT(BitBltSanityCheck)(pFB);
#endif /* CT_BITBLT_SANITY_CHECK */
}

#ifdef NOT_YET
void
CT(DoDrawOffsetMonoImage)(pFB, dstStride, dstOffset, width, height, image, startx, stride)
CT_PIXEL *pFB;
unsigned int dstStride;
unsigned long dstOffset;
int width;
int height;
unsigned char *image;
unsigned int startx;
unsigned int stride;
{
	int scanwidth;
	unsigned long *pdst = (unsigned long *)pFB;
	unsigned char *psrc;

	scanwidth = (width + (8 - 1)) >> 3;	/* scanwidth in bytes */

#ifdef DEBUG_PRINT
	CT(BitBltDebugF)("DoDrawOffsetMonoImage(): 0x%08x x=%d s=%d w=%d (%dx%d)\n",
		image, startx, stride, scanwidth, width, height);
#endif /* DEBUG_PRINT */

	/*
	 * NOTE: Assume the caller set the control register and any relevant
	 * color registers. Also, assume the caller waited for the BitBlt engine
	 * to be idle before setting these registers.
	 */
	CT_SET_BLTOFFSET(0, dstStride);
	CT_SET_BLTSRC(0);
	CT_SET_BLTDST(dstOffset);
	CT_START_BITBLT(width, height);		/* number of expanded bytes */

#define CT_SHIFT_EXPAND_BYTES1(pdst, psrc, shift) {			\
	*(pdst) =   CT_CHAR2LONG((psrc)[0]) >> (shift);			\
}

#define CT_SHIFT_EXPAND_BYTES2(pdst, psrc, shift) {			\
	*(pdst) = ((CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0])) >> (shift);		\
}

#define CT_SHIFT_EXPAND_BYTES3(pdst, psrc, shift) {			\
	*(pdst) = ((CT_CHAR2LONG((psrc)[2]) << 16) |			\
		   (CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0])) >> (shift);		\
}

#define CT_SHIFT_EXPAND_BYTES4(pdst, psrc, shift) {			\
	*(pdst) = ((CT_CHAR2LONG((psrc)[3]) << 24) |			\
		   (CT_CHAR2LONG((psrc)[2]) << 16) |			\
		   (CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		    CT_CHAR2LONG((psrc)[0])) >> (shift);		\
}

#define CT_SHIFT_EXPAND_BYTES(pdst, psrc, shift) {			\
	*(pdst) = ((((CT_CHAR2LONG((psrc)[3]) << 24) |			\
		     (CT_CHAR2LONG((psrc)[2]) << 16) |			\
		     (CT_CHAR2LONG((psrc)[1]) <<  8) |			\
		      CT_CHAR2LONG((psrc)[0])) >> (shift)) |		\
		   (((CT_CHAR2LONG((psrc)[7]) << 24) |			\
		     (CT_CHAR2LONG((psrc)[6]) << 16) |			\
		     (CT_CHAR2LONG((psrc)[5]) <<  8) |			\
		      CT_CHAR2LONG((psrc)[4])) << (32 - (shift))));	\
}

	/*
	 * NOTE: The BitBlt source address register (DR05) addresses a byte-
	 * aligned source block even if the source is 1-bit deep. This means
	 * that the first bit of the first addressed byte in a scanline of
	 * source data written to the BitBlt engine MUST correspond to the first
	 * requested destination pixel.
	 */
	switch(scanwidth) {
	case 1:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_SHIFT_EXPAND_BYTES1(pdst, image, startx);
			image += stride;
		}
		break;
	case 2:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_SHIFT_EXPAND_BYTES2(pdst, image, startx);
			image += stride;
		}
		break;
	case 3:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_SHIFT_EXPAND_BYTES3(pdst, image, startx);
			image += stride;
		}
		break;
	case 4:
		while (height--) {
			CT_WAIT_FOR_BUFFER();
			CT_SHIFT_EXPAND_BYTES4(pdst, image, startx);
			image += stride;
		}
		break;
	default:
		/*
		 * Here, we handle arbitrary width image data.
		 */
		dwidth = scanwidth >> 2;		/* in dwords */
		switch (scanwidth & 3) {		/* extra */
		case 0:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_SHIFT_EXPAND_BYTES(pdst, psrc,
								startx);
					psrc += 4;
				}
				image += stride;
			}
			break;
		case 1:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_SHIFT_EXPAND_BYTES(pdst, psrc,
								startx);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_SHIFT_EXPAND_BYTES1(pdst, psrc, startx);
				image += stride;
			}
			break;
		case 2:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_SHIFT_EXPAND_BYTES(pdst, psrc,
								startx);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_SHIFT_EXPAND_BYTES2(pdst, psrc, startx);
				image += stride;
			}
			break;
		case 3:
			while (height--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_SHIFT_EXPAND_BYTES(pdst, psrc,
								startx);
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_SHIFT_EXPAND_BYTES3(pdst, psrc, startx);
				image += stride;
			}
			break;
		}
		break;
	}

#ifdef CT_BITBLT_SANITY_CHECK
	CT(BitBltSanityCheck)(pFB);
#endif /* CT_BITBLT_SANITY_CHECK */
}
#endif /* NOT_YET */

/*
 * CT(DrawMonoImage)() - Draw transparent image in a rectangular area of a
 * window.
 *
 *	pbox - the rectangle to draw into.
 *	image - pointer to the bitmap image to draw.
 *	startx - value that defines the first bit that must be drawn in each
 *		 line of the bitmap.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	fg - color to which a set bit must be expanded before alu and planemask
 *	     are applied.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void
CT(DrawMonoImage)(pbox, image, startx, stride, fg, alu, planemask, pDraw)
BoxPtr pbox;
unsigned char *image;
unsigned int startx;
unsigned int stride;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int width, height;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
				planemask, pDraw);
		return;
	}

	if (((startx) != 0) && (((startx) & (8 - 1)) != 0)) {
		/*
		 * XXX Can't do arbitrary startx's yet.
		 */
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
				~0L, pDraw);
		return;
	}

	switch (alu) {
	case GXnoop:
		return;
	case GXclear:
		alu = GXcopy;
		fg = 0L;
		break;
	case GXset:
		alu = GXcopy;
		fg = ~0L;
		break;
	case GXinvert:
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
				~0L, pDraw);
		return;
	default:
		break;
	}

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, monochrome source data, transparent background,
	 * from memory to screen.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_SRCMONOBIT | CT_BGTRANSBIT | CT_BLTMEMORYBIT);
	CT_SET_BLTFGCOLOR(fg);

	if (startx == 0) {
		CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
				CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
				width, height, image, stride);
	} else {
		image += startx >> 3;
		CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
				CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
				width, height, image, stride);
	}
}

/*
 * CT(DrawOpaqueMonoImage)() - Draw opaque image in a rectangular area of a
 * window.
 *
 *	pbox - the rectangle to draw into.
 *	image - pointer to the bitmap image to draw.
 *	startx - value that defines the first bit that must be drawn in each
 *		 line of the bitmap.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	fg - color to which a set bit must be expanded before alu and planemask
 *	     are applied.
 *	bg - color to which an unset bit must be expanded before alu and
 *	     planemask are applied.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw to.
 */
void
CT(DrawOpaqueMonoImage)(pbox, image, startx, stride, fg, bg, alu, planemask, pDraw)
BoxPtr pbox;
unsigned char *image;
unsigned int startx;
unsigned int stride;
unsigned long fg;
unsigned long bg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int width, height;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawOpaqueMonoImage(pbox, image, startx, stride, fg, bg, alu,
					planemask, pDraw);
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
		CT(DrawSolidRects)(pbox, 1, fg, alu, planemask, pDraw);
		return;
	default:
		if (((startx) != 0) && (((startx) & (8 - 1)) != 0)) {
			/*
			 * XXX Can't do arbitrary startx's yet.
			 */
			genDrawOpaqueMonoImage(pbox, image, startx, stride,
					fg, bg, alu, ~0L, pDraw);
			return;
		}
		break;
	}

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, monochrome source data, transparent background,
	 * from memory to screen.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_SRCMONOBIT | CT_BLTMEMORYBIT);
	CT_SET_BLTFGCOLOR(fg);
	CT_SET_BLTBGCOLOR(bg);

	if (startx == 0) {
		CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
				CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
				width, height, image, stride);
	} else {
		image += startx >> 3;
		CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
				CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
				width, height, image, stride);
	}
}
