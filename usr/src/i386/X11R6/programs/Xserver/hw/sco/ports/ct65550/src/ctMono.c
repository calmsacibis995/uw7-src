/*
 *	@(#)ctMono.c	11.1	10/22/97	12:34:56
 *	@(#) ctMono.c 61.1 97/02/26 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctMono.c 61.1 97/02/26 "

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "scoext.h"

#include "ctMacros.h"
#include "ctDefs.h"

extern void CT(DrawSolidRects)();

void
CT(DoDrawMonoImage)(pFB, dstStride, dstOffset, width, height, image, stride,
startx)
CT_PIXEL *pFB;
unsigned int dstStride;
unsigned long dstOffset;
int width;
int height;
unsigned char *image;
unsigned int stride;
unsigned int startx;
{
	int w, scanwidth, dwidth;
	int h;
	unsigned long *pdst = CT_BLT_DATAPORT(pFB);
	unsigned char *psrc;

	scanwidth = (width + (8 - 1)) >> 3;	/* scanwidth in bytes */

#ifdef DEBUG_PRINT
	ErrorF("DoDrawMonoImage(): 0x%08x s=%d w=%d (%dx%d)\n",
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
	CT_SET_MONO_CONTROL(CT_MONO_ALIGN_DWORD, 0, 0, startx); 
	CT_START_BITBLT(width, height);		/* number of expanded bytes */

	h = height;

	switch(scanwidth) {
	case 1:
		while (h--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES1(pdst, image);
#ifdef DEBUG_PRINT
			ErrorF("(1)0x%x ", *((unsigned *)image));
#endif
			image += stride;
		}
		/*
		 * BLT engine needs qword aligned transfers
		 */
		if (height & 0x1)
			CT_EXPAND_ROUNDED(pdst);
		break;
	case 2:
		while (h--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES2(pdst, image);
#ifdef DEBUG_PRINT
			ErrorF("(2)0x%x ", *((unsigned *)image));
#endif
			image += stride;
		}
		/*
		 * BLT engine needs qword aligned transfers
		 */
		if (height & 0x1)
			CT_EXPAND_ROUNDED(pdst);
		break;
	case 3:
		while (h--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES3(pdst, image);
#ifdef DEBUG_PRINT
			ErrorF("(3)0x%x ", *((unsigned *)image));
#endif
			image += stride;
		}
		/*
		 * BLT engine needs qword aligned transfers
		 */
		if (height & 0x1)
			CT_EXPAND_ROUNDED(pdst);
		break;
	case 4:
		while (h--) {
			CT_WAIT_FOR_BUFFER();
			CT_EXPAND_BYTES4(pdst, image);
#ifdef DEBUG_PRINT
			ErrorF("(4)0x%x ", *((unsigned *)image));
#endif
			image += stride;
		}
		/*
		 * BLT engine needs qword aligned transfers
		 */
		if (height & 0x1)
			CT_EXPAND_ROUNDED(pdst);
		break;
	default:
		/*
		 * Here, we handle arbitrary width image data.
		 */
		dwidth = scanwidth >> 2;		/* in dwords */
		switch (scanwidth & 3) {		/* extra */
		case 0:
			while (h--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
#ifdef DEBUG_PRINT
					ErrorF("[0]0x%x ", *((unsigned *)psrc));
#endif
					psrc += 4;
				}
				image += stride;
			}
			/*
			 * BLT engine needs qword aligned transfers
			 */
			if (height & 0x1)
				CT_EXPAND_ROUNDED(pdst);
			break;
		case 1:
			while (h--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
#ifdef DEBUG_PRINT
					ErrorF("[1]0x%x ", *((unsigned *)psrc));
#endif
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES1(pdst, psrc);
				image += stride;
			}
			break;
		case 2:
			while (h--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
#ifdef DEBUG_PRINT
					ErrorF("[2]0x%x ", *((unsigned *)psrc));
#endif
					psrc += 4;
				}
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES2(pdst, psrc);
				image += stride;
			}
			break;
		case 3:
			while (h--) {
				psrc = image;
				w = dwidth;
				while (w--) {
					CT_WAIT_FOR_BUFFER();
					CT_EXPAND_BYTES4(pdst, psrc);
#ifdef DEBUG_PRINT
					ErrorF("[3]0x%x ", *((unsigned *)psrc));
#endif
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
#ifdef DEBUG_PRINT
	ErrorF("\n");
#endif

#ifdef CT_BITBLT_SANITY_CHECK
	CT(BitBltSanityCheck)(pFB);
#endif /* CT_BITBLT_SANITY_CHECK */
}

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
void *image;
unsigned int startx;
unsigned int stride;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	int width, height;

#ifdef DEBUG_PRINT
	ErrorF("DrawMonoImage(): image=0x%08x s=%d startx=0x%x\n", 
		image, stride, startx);
#endif /* DEBUG_PRINT */

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
				planemask, pDraw);
		return;
	}

#if 0
	if (((startx) != 0) && (((startx) & (8 - 1)) != 0)) {
		/*
		 * XXX Can't do arbitrary startx's yet.
		 */
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
				~0L, pDraw);
		return;
	}
#endif

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

#ifdef DEBUG_PRINT
	ErrorF("DrawMonoImage(): width 0x%x height 0x%x\n", 
		width, height);
#endif /* DEBUG_PRINT */

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, monochrome source data, transparent background,
	 * from memory to screen.
	 */
#if 0
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_YINCBIT | CT_XINCBIT |
			CT_SRCMONOBIT | CT_BGTRANSBIT | CT_BLTMEMORYBIT);
#else
	CT_SET_BLTCTRL(CT(PixelOps)[alu] |
			CT_SRCMONOBIT | CT_BGTRANSBIT | CT_BLTMEMORYBIT);
#endif
	CT_SET_BLTFGCOLOR(fg);

	/*
	 * Round the image to bytes (skip all whole bytes).
	 */
	image = (void *) ((unsigned char *) image + (startx >> 3));

	CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
			CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
			width, height, image, stride, startx);
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
void *image;
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

#ifdef DEBUG_PRINT
	ErrorF("DrawOpaqueMonoImage\n");
#endif /* DEBUG_PRINT */

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
#if 0
		if (((startx) != 0) && (((startx) & (8 - 1)) != 0)) {
			/*
			 * XXX Can't do arbitrary startx's yet.
			 */
			genDrawOpaqueMonoImage(pbox, image, startx, stride,
					fg, bg, alu, ~0L, pDraw);
			return;
		}
#endif
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
#if 0
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_YINCBIT | CT_XINCBIT |
			CT_SRCMONOBIT | CT_BLTMEMORYBIT);
#else
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | 
			CT_SRCMONOBIT | CT_BLTMEMORYBIT);
#endif
	CT_SET_BLTFGCOLOR(fg);
	CT_SET_BLTBGCOLOR(bg);

	image = (void *) ((unsigned char *) image + (startx >> 3));

	CT(DoDrawMonoImage)(ctPriv->fbPointer, ctPriv->bltStride,
			CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1),
			width, height, image, stride, startx);
}
