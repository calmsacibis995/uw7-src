/*
 *	@(#)ctImgCache.c	11.1	10/22/97	12:33:59
 *	@(#) ctImgCache.c 58.1 96/10/09 
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
/*
 *	SCO Modifications
 *
 *	S003    Fri Sep 09 1994			rogerv@sco.COM
 *	Needed to change the order of include files so that math.h
 *	is included first for unoptimized, non-inlined server build
 *	to work.
 *
 *	S002	Fri Jul 01 11:43:04 PDT 1994	davidw@sco.COM
 *	Include math.h when sqrt is used - was needed for Tbird build.
 *
 *	S001	Wed Jun 01 10:34:01 PDT 1994	hiramc@sco.COM
 *	- Must use a simple char * for Microsoft inlined _asm code.
 */

#ident "@(#) $Id: ctImgCache.c 58.1 96/10/09 "

#include <math.h>

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "ctDefs.h"
#include "ctMacros.h"
#include "ctOnboard.h"


/*******************************************************************************

				Private Routines

*******************************************************************************/

#define	p8x8_WIDTH	8
#define	p8x8_HEIGHT	8
#define	p8x8_STRIDE	(p8x8_WIDTH * (CT_BITS_PER_PIXEL / 8))

static void *
ctBuild8x8Stipple(pScreen, stipple, stride, w, h, fg, bg)
ScreenPtr pScreen;
register CT_PIXEL *stipple;
unsigned int stride;
unsigned int w;
unsigned int h;
unsigned long fg, bg;
{
#ifdef DEBUG_PRINT
	ErrorF("Build8x8Stipple(): stipple=0x%08x stride=%d w=%d h=%d\n",
		stipple, stride, w, h);
#endif /* DEBUG_PRINT */

#ifdef DEBUG_PRINT
	ErrorF("Build8x8Stipple() *** NOT IMPLEMENTED ***\n");
#endif /* DEBUG_PRINT */

	return ((void *)0);
}

static void *
ctBuild8x8Tile(pScreen, tile, stride, w, h)
ScreenPtr pScreen;
register CT_PIXEL *tile;
unsigned int stride;
unsigned int w;
unsigned int h;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	register int xsrc, ysrc;
	int xdst, ydst;
	Bool w_ok, h_ok;
	void *chunk;
	CT_PIXEL *p, *p8x8;
	int pixstr = (stride / sizeof(CT_PIXEL));
	char * fbPtr = ctPriv->fbPointer;		/*	S001	*/

#ifdef DEBUG_PRINT
	ErrorF("Build8x8Tile(): tile=0x%08x stride=%d w=%d h=%d\n",
		tile, stride, w, h);
#endif /* DEBUG_PRINT */

	CT_WAIT_FOR_IDLE();

	w_ok = (w == 1) || (w == 2) || (w == 4) || (w == 8);
	h_ok = (h == 1) || (h == 2) || (h == 4) || (h == 8);
	/*
	 * first test allows for simple replication in
	 * both horiz and vertical direction
	 */
	if (w_ok && h_ok) {
		chunk = CT_MEM_ALLOC(pScreen,
					p8x8_WIDTH,
					p8x8_HEIGHT,
					p8x8_STRIDE);
		if (!chunk)
			return ((void *)0);
		p = p8x8 = CT_MEM_VADDR(chunk);
		for (ydst=0;ydst<p8x8_HEIGHT;ydst+=h) {
			for (ysrc=0;ysrc<h;ysrc++) {
				for (xdst=0;xdst<p8x8_WIDTH;xdst+=w) {
					for (xsrc=0;xsrc<w;xsrc++) {
						*p++ = tile[ysrc * pixstr + xsrc];
					}
				}
			}
		}
#ifdef DEBUG_PRINT
		ErrorF("Build8x8Tile(): built ok offset=0x%08x\n",
			CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */
		CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
		return (chunk);
	}
	/*
	 * In this case, we check for a replicated pattern
	 * in the vertical direction
	 */
	if (w_ok && (h == 16)) {
		chunk = CT_MEM_ALLOC(pScreen,
					p8x8_WIDTH,
					p8x8_HEIGHT,
					p8x8_STRIDE);
		if (!chunk)
			return ((void *)0);
		p = p8x8 = CT_MEM_VADDR(chunk);
		for (ydst=0;ydst<p8x8_HEIGHT;ydst++) {
			ysrc = ydst;
			for (xdst=0;xdst<p8x8_WIDTH;xdst+=w) {
				for (xsrc=0;xsrc<w;xsrc++) {
					/* check second half matches first */
					if (tile[ysrc * pixstr + xsrc] != tile[(ysrc + p8x8_HEIGHT) * pixstr + xsrc]) {
						CT_MEM_FREE(pScreen, chunk);
#ifdef DEBUG_PRINT
						ErrorF("Build8x8Tile() failed vert test\n");
#endif /* DEBUG_PRINT */
						CT_FLUSH_BITBLT(fbPtr);/* S001*/
						return ((void *)0);
					}
					*p++ = tile[ysrc * pixstr + xsrc];
				}
			}
		}
#ifdef DEBUG_PRINT
		ErrorF("Build8x8Tile(): built ok offset=0x%08x\n",
			CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */
		CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
		return (chunk);
	}
	/*
	 * In this case, we check for a replicated pattern
	 * in the horizontal direction
	 */
	if (h_ok && (w == 16)) {
		chunk = CT_MEM_ALLOC(pScreen,
					p8x8_WIDTH,
					p8x8_HEIGHT,
					p8x8_STRIDE);
		if (!chunk)
			return ((void *)0);
		p = p8x8 = CT_MEM_VADDR(chunk);
		for (ydst=0;ydst<p8x8_HEIGHT;ydst+=h) {
			for (ysrc=0;ysrc<h;ysrc++) {
				for (xdst=0;xdst<p8x8_WIDTH;xdst++) {
					xsrc = xdst;
					/* check second half matches first */
					if (tile[ysrc * pixstr + xsrc] != tile[ysrc * pixstr + (xsrc + p8x8_WIDTH)]) {
						CT_MEM_FREE(pScreen, chunk);
#ifdef DEBUG_PRINT
						ErrorF("Build8x8Tile() failed horiz test\n");
#endif /* DEBUG_PRINT */
						CT_FLUSH_BITBLT(fbPtr);/*S001*/
						return ((void *)0);
					}
					*p++ = tile[ysrc * pixstr + xsrc];
				}
			}
		}
#ifdef DEBUG_PRINT
		ErrorF("Build8x8Tile(): built ok offset=0x%08x\n",
			CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */
		CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
		return (chunk);
	}
	/*
	 * In this case, we check for a replicated pattern
	 * in the vertical and horizontal direction
	 */
	if ((h == 16) && (w == 16)) {
		chunk = CT_MEM_ALLOC(pScreen,
					p8x8_WIDTH,
					p8x8_HEIGHT,
					p8x8_STRIDE);
		if (!chunk)
			return ((void *)0);
		p = p8x8 = CT_MEM_VADDR(chunk);
		for (ydst=0;ydst<p8x8_HEIGHT;ydst++) {
			ysrc = ydst;
			for (xdst=0;xdst<p8x8_WIDTH;xdst++) {
				xsrc = xdst;
				/* check second half matches first */
				if ((tile[ysrc * pixstr + xsrc] != tile[(ysrc + p8x8_HEIGHT) * pixstr + xsrc]) ||
				    (tile[ysrc * pixstr + xsrc] != tile[ysrc * pixstr + (xsrc + p8x8_WIDTH)]) ||
				    (tile[ysrc * pixstr + xsrc] != tile[(ysrc + p8x8_HEIGHT) * pixstr + (xsrc + p8x8_WIDTH)])) {
					CT_MEM_FREE(pScreen, chunk);
#ifdef DEBUG_PRINT
					ErrorF("Build8x8Tile() failed vert and horiz test\n");
#endif /* DEBUG_PRINT */
					CT_FLUSH_BITBLT(fbPtr);	/*	S001*/
					return ((void *)0);
				}
				*p++ = tile[ysrc * pixstr + xsrc];
			}
		}
#ifdef DEBUG_PRINT
		ErrorF("Build8x8Tile(): built ok offset=0x%08x\n",
			CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */
		CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
		return (chunk);
	}

	/*
	 * no special case found
	 */
#ifdef DEBUG_PRINT
	ErrorF("Build8x8Tile() no special case\n");
#endif /* DEBUG_PRINT */
	return ((void *)0);
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * CT_COPY_SS() - copy screen-to-screen (onboard mem to onboard mem).
 */
#define CT_COPY_SS(src_off, dst_off, w, h) {				\
	CT_WAIT_FOR_IDLE();						\
	CT_SET_BLTSRC((src_off));					\
	CT_SET_BLTDST((dst_off));					\
	CT_START_BITBLT((w), (h));					\
}

void *
CT(ImageCacheLoad)(pScreen, image, stride, depth, w, h, fg, bg)
ScreenPtr pScreen;
unsigned char *image;
int stride;
int depth;
unsigned int *w;	/* also returns chunk width */
unsigned int *h;	/* also returns chunk height */
unsigned long fg, bg;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	unsigned long *pdst = (unsigned long *)ctPriv->fbPointer;
	unsigned long *psrc;
	unsigned long src_offset, dst_offset;
	unsigned int image_width = *w;
	unsigned int image_height = *h;
	unsigned int chunk_min, chunk_max, chunk_size;
	unsigned int chunk_width, chunk_height, chunk_stride;
	unsigned int x, y, width, height, dwidth;
	void *chunk;

#ifdef DEBUG_PRINT
	ErrorF("ImageCacheLoad(): 0x%08x s=%d d=%d w=%d h=%d\n",
		image, stride, depth, image_width, image_height);
#endif /* DEBUG_PRINT */

	if (depth == 1) {
		/*
		 * Try to build an 8x8 stipple in offscreen memory.
		 * This routine will color expand based upon fg and bg.
		 */
		chunk = ctBuild8x8Stipple(pScreen,
				image, stride, image_width, image_height,
				fg, bg);
	} else {
		/*
		 * Try to build an 8x8 tile in offscreen memory.
		 */
		chunk = ctBuild8x8Tile(pScreen,
				image, stride, image_width, image_height);
	}
	if (chunk != (void *)0) {
		/*
		 * Tile successfully expanded to 8x8. Set tile size
		 * accordingly.
		 */
		*w = 8;
		*h = 8;
		return (chunk);
	}

	/*
	 * XXX We really need different algorithms for GC operations and Window
	 * operations, since the GC memory is persistent while the window ops
	 * only cache temporarily.
	 *
	 * NOTE: CT_MEM_AVAILBLE() returns the amount of onboard memory left
	 * after allocating the screen. This is NOT a dynamic measure of current
	 * memory usage. If CT_MEM_AVAILABLE() becomes dynamic at some point, we
	 * need to review the code below.
	 */
	chunk_size = image_width * image_height;	/* in pixels */
	chunk_min = CT_MEM_AVAILABLE(pScreen) / ctPriv->kTileMin;
	chunk_max = CT_MEM_AVAILABLE(pScreen) / ctPriv->kTileMax;
	if (chunk_size < chunk_min) {
		int k = (int)sqrt((double)chunk_min);

		/*
		 * For smaller pixmaps, cache replicated images.
		 */
		chunk_width = (k / image_width) * image_width;
		chunk_height = (k / image_height) * image_height;
	}  else if (chunk_size > chunk_max) {
		/*
		 * Don't bother trying to cache large pixmaps.
		 */
		*w = 0;
		*h = 0;
		return ((void *)0);
	} else {
		/*
		 * Otherwise, cache an exact copy of the pixmap.
		 */
		chunk_width = image_width;
		chunk_height = image_height;
	}
	chunk_stride = chunk_width * sizeof(CT_PIXEL);
	chunk = CT_MEM_ALLOC(pScreen, chunk_width, chunk_height, chunk_stride);
	if (chunk == (void *)0) {
		/*
		 * No offscreen memory.
		 */
		*w = 0;
		*h = 0;
		return ((void *)0);
	}

	src_offset = CT_MEM_OFFSET(chunk, 0, 0);

	if (depth == 1) {
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTCTRL(CT(PixelOps)[GXcopy] | CT_XINCBIT | CT_YINCBIT |
				CT_SRCMONOBIT | CT_BLTMEMORYBIT);
		CT_SET_BLTFGCOLOR(fg);
		CT_SET_BLTBGCOLOR(bg);

		CT(DoDrawMonoImage)(ctPriv->fbPointer, chunk_stride, src_offset,
				image_width, image_height, image, stride);
	} else {
		/*
		 * Draw color image data to the offscreen area.
		 */
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTCTRL(CT(PixelOps)[GXcopy] | CT_XINCBIT | CT_YINCBIT |
				CT_BLTMEMORYBIT);
		CT_SET_BLTOFFSET(stride, chunk_stride);
		CT_SET_BLTSRC(0);
		CT_SET_BLTDST(src_offset);
		CT_START_BITBLT(image_width, image_height);

		dwidth = ((image_width * sizeof(CT_PIXEL)) + (4 - 1)) >> 2;
		height = image_height;
		while (height--) {
			psrc = (unsigned long *)image;
			width = dwidth;
			while (width--) {
				*pdst = *psrc++;
			}
			image += stride;
		}
	}

	if ((chunk_width != image_width) || (chunk_height != image_height)) {
		/*
		 * Replicate the tile in offscreen memory. Chunk dimensions will
		 * be multiples of image dimensions.
		 */
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTCTRL(CT(PixelOps)[GXcopy] | CT_YINCBIT | CT_XINCBIT);
		CT_SET_BLTOFFSET(chunk_stride, chunk_stride);
		src_offset = CT_MEM_OFFSET(chunk, 0, 0);
		for (x = image_width; x < chunk_width; x += image_width) {
			dst_offset = CT_MEM_OFFSET(chunk, x, 0);
			CT_COPY_SS(src_offset, dst_offset,
				image_width, image_height);
		}
		for (y = image_height; y < chunk_height; y += image_height) {
			dst_offset = CT_MEM_OFFSET(chunk, 0, y);
			CT_COPY_SS(src_offset, dst_offset,
				chunk_width, image_height);
		}
	}

	*w = chunk_width;
	*h = chunk_height;

#ifdef DEBUG_PRINT
	ErrorF("ImageCacheLoad(): loaded ok offset=0x%08x\n",
		CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */

	return (chunk);
}

void
CT(ImageCacheFree)(pScreen, chunk)
ScreenPtr pScreen;
void *chunk;
{
#ifdef DEBUG_PRINT
	ErrorF("ImageCacheFree(): offset=0x%08x\n", CT_MEM_OFFSET(chunk, 0, 0));
#endif /* DEBUG_PRINT */

	CT_MEM_FREE(pScreen, chunk);
}

void
CT(ImageCacheCopyRects)(pbox, nbox, chunk, w, h, patOrg, alu, planemask, pDraw)
BoxPtr pbox;
unsigned int nbox;
void *chunk;
unsigned int w;
unsigned int h;
DDXPointPtr patOrg;
unsigned char alu;
unsigned long planemask;	/* NOT currently supported */
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned long src_offset, dst_offset;
	int xoff, yoff, stride;
	BoxRec dst;

#ifdef DEBUG_PRINT
	ErrorF("ImageCacheCopyRects(): 0x%08x w=%d h=%d\n",
		CT_MEM_OFFSET(chunk, 0, 0), w, h);
#endif /* DEBUG_PRINT */

	if ((w == 8) && (h == 8)) {
		CT_WAIT_FOR_IDLE();

		/*
		 * Load offscreen memory location into the BitBlt pattern rop
		 * register.
		 */
		src_offset = CT_MEM_OFFSET(chunk, 0, 0);
		CT_SET_BLTPATSRC(src_offset);

		CT_SET_BLTOFFSET(ctPriv->bltStride, ctPriv->bltStride);

		while (nbox--) {
			yoff = (pbox->y1 - patOrg->y) % 8;
			if (yoff < 0)
				yoff += 8;

			CT_WAIT_FOR_IDLE();

			/*
			 * Set BitBlt control register to pattern-fill a mapped
			 * alu, left-to-right, top-to-bottom, using color,
			 * bitmap pattern offset by patOrg->y lines (x is
			 * automatically aligned based on destination address).
			 */
			CT_SET_BLTCTRL(CT(PatternOps)[alu] |
					CT_XINCBIT |
					CT_YINCBIT |
					CT_PATTERN_SEED(yoff));
			CT_SET_BLTSRC(0);
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, pbox->x1,
							       pbox->y1));
			CT_START_BITBLT(pbox->x2 - pbox->x1,
					pbox->y2 - pbox->y1);
			pbox++;
		}
		return;
	}

	stride = w * sizeof(CT_PIXEL);

	CT_WAIT_FOR_IDLE();
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_YINCBIT | CT_XINCBIT);
	CT_SET_BLTOFFSET(stride, ctPriv->bltStride);

	while(nbox--) {
		/*
		 * Be careful here. Since (pbox - patOrg) may be negative, the
		 * modulo operations MUST be signed, not unsigned.
		 */
		xoff = (pbox->x1 - patOrg->x) % (int)w;
		if (xoff < 0)
			xoff += w;

		yoff = (pbox->y1 - patOrg->y) % (int)h;
		if (yoff < 0)
			yoff += h;

		dst.x1 = pbox->x1;
		dst.y1 = pbox->y1;
		dst.x2 = min((pbox->x1 + w - xoff), pbox->x2);
		dst.y2 = min((pbox->y1 + h - yoff), pbox->y2);

		/*
		 * Do upper left rectangle.
		 */
		src_offset = CT_MEM_OFFSET(chunk, xoff, yoff);
		dst_offset = CT_SCREEN_OFFSET(ctPriv, dst.x1, dst.y1);
		CT_COPY_SS(src_offset, dst_offset,
			   dst.x2 - dst.x1, dst.y2 - dst.y1);

		/*
		 * Do top row of rectangles.
		 */
		src_offset = CT_MEM_OFFSET(chunk, 0, yoff);
		while (dst.x2 < pbox->x2) {
			dst.x1 = dst.x2;
			dst.x2 = min((dst.x2 + w), pbox->x2);
			dst_offset = CT_SCREEN_OFFSET(ctPriv, dst.x1, dst.y1);
			CT_COPY_SS(src_offset, dst_offset, 
				   dst.x2 - dst.x1, dst.y2 - dst.y1);
		}

		while (dst.y2 < pbox->y2) {
			/*
			 * Do left rectangle.
			 */
			src_offset = CT_MEM_OFFSET(chunk, xoff, 0);

			dst.x1 = pbox->x1;
			dst.y1 = dst.y2;
			dst.x2 = min((pbox->x1 + w - xoff), pbox->x2);
			dst.y2 = min((dst.y2 + h), pbox->y2);
			dst_offset = CT_SCREEN_OFFSET(ctPriv, dst.x1, dst.y1);
			CT_COPY_SS(src_offset, dst_offset,
				   dst.x2 - dst.x1, dst.y2 - dst.y1);

			/*
			 * Do interior rectangles.
			 */
			src_offset = CT_MEM_OFFSET(chunk, 0, 0);

			while (dst.x2 < pbox->x2) {
				dst.x1 = dst.x2;
				dst.x2 = min((dst.x2 + w), pbox->x2);
				dst_offset = CT_SCREEN_OFFSET(ctPriv,
							dst.x1, dst.y1);
				CT_COPY_SS(src_offset, dst_offset,
					   dst.x2 - dst.x1, dst.y2 - dst.y1);
			}
		}
		pbox++;
	}
}

void
CT(ImageCacheFS)(ppt, pwidth, npts, chunk, w, h, patOrg, alu, planemask, pDraw)
DDXPointPtr ppt;
unsigned int *pwidth;
unsigned npts;
void *chunk;
unsigned int w;
unsigned int h;
DDXPointPtr patOrg;
unsigned char alu;
unsigned long planemask;	/* NOT currently supported */
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned long src_offset, dst_offset;
	int x1, x2, xend, xoff, yoff, stride;

#ifdef DEBUG_PRINT
	ErrorF("ImageCacheFS(): 0x%08x w=%d h=%d\n",
		CT_MEM_OFFSET(chunk, 0, 0), w, h);
#endif /* DEBUG_PRINT */

	if ((w == 8) && (h == 8)) {
		CT_WAIT_FOR_IDLE();

		/*
		 * Load offscreen memory location into the BitBlt pattern rop
		 * register.
		 */
		src_offset = CT_MEM_OFFSET(chunk, 0, 0);
		CT_SET_BLTPATSRC(src_offset);

		CT_SET_BLTOFFSET(ctPriv->bltStride, ctPriv->bltStride);

		while (npts--) {
			yoff = (ppt->y - patOrg->y) % 8;
			if (yoff < 0)
				yoff += 8;

			CT_WAIT_FOR_IDLE();

			/*
			 * Set BitBlt control register to pattern-fill a mapped
			 * alu, left-to-right, top-to-bottom, using color,
			 * bitmap pattern offset by patOrg->y lines (x is
			 * automatically aligned based on destination address).
			 */
			CT_SET_BLTCTRL(CT(PatternOps)[alu] |
					CT_XINCBIT |
					CT_YINCBIT |
					CT_PATTERN_SEED(yoff));
			CT_SET_BLTSRC(0);
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, ppt->x, ppt->y));
			CT_START_BITBLT(*pwidth, 1);

			++ppt;
			++pwidth;
		}
		return;
	}

	stride = w * sizeof(CT_PIXEL);

	CT_WAIT_FOR_IDLE();
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_YINCBIT | CT_XINCBIT);
	CT_SET_BLTOFFSET(stride, ctPriv->bltStride);

	while (npts--) {
		/*
		 * Be careful here. Since (ppt - patOrg) may be negative, the
		 * modulo operations MUST be signed, not unsigned.
		 */
		xoff = (ppt->x - patOrg->x) % (int)w;
		if (xoff < 0)
			xoff += w;

		yoff = (ppt->y - patOrg->y) % (int)h;
		if (yoff < 0)
			yoff += h;

		x1 = ppt->x;
		x2 = ppt->x + min((w - xoff), *pwidth);
		xend = ppt->x + *pwidth;

		/*
		 * Do left span segment.
		 */
		src_offset = CT_MEM_OFFSET(chunk, xoff, yoff);
		dst_offset = CT_SCREEN_OFFSET(ctPriv, x1, ppt->y);
		CT_COPY_SS(src_offset, dst_offset, x2 - x1, 1);

		/*
		 * Finish row of span segments.
		 */
		src_offset = CT_MEM_OFFSET(chunk, 0, yoff);
		while (x2 < xend) {
			x1 = x2;
			x2 = min((x2 + w), xend);
			dst_offset = CT_SCREEN_OFFSET(ctPriv, x1, ppt->y);
			CT_COPY_SS(src_offset, dst_offset, x2 - x1, 1);
		}

		++ppt;
		++pwidth;
	}
}
