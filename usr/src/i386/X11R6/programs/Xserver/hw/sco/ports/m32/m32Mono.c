/*
 * @(#) m32Mono.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 09-Aug-93, buckm
 *	Created.
 * S001, 27-Aug-93, buckm
 *	Don't enlarge current clip box when clipping mono bits.
 *	Replace CLIP macros with live code for directness.
 * S002, 21-Sep-94, davidw
 *      Correct compiler warnings, update function definations to 
 *	ANSI style so we get warnings.
 */

#include "X.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

/*
 * m32DrawMonoImage() - Draw color-expanded monochrome image;
 *			don't draw '0' bits.
 *	pbox - destination rectangle to draw into.
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	startx - number of bits to skip at start of each line of image.
 *	fg - the color to make the '1' bits.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
m32DrawMonoImage(
	BoxPtr pbox,
	void *void_image,					/* S002 */
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
	int w, h, endx, bytes;
	int x1, x2;
	int dp;
	unsigned char *image = (unsigned char *)void_image; 	/* S002 */

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	image += startx >> 3;
	startx &= 7;
    
	endx = (w + startx) & 7;
	bytes = (w + startx + 7) >> 3;

	dp = ((bytes != stride) && (bytes & 1))
		? M32_DP_EXPBHOST : M32_DP_EXPWHOST;

	x1 = pbox->x1;
	x2 = pbox->x2;

	M32_CLEAR_QUEUE(12);

	if (startx) {
		if (pbox->x1 > pM32->clip.x1)
			outw(M32_EXT_SCISSOR_L, pbox->x1);
		x1 -= startx;
	}
	if (endx) {
		if (pbox->x2 < pM32->clip.x2)
			outw(M32_EXT_SCISSOR_R, pbox->x2 - 1);
		x2 += 8 - endx;
	}

	outw(M32_DP_CONFIG,	dp);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_ALU_BG_FN,	m32RasterOp[GXnoop]);
	outw(M32_FRGD_COLOR,	fg);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_CUR_X,		x1);
	outw(M32_CUR_Y,		pbox->y1);
	outw(M32_DEST_X_START,	x1);
	outw(M32_DEST_X_END,	x2);
	outw(M32_DEST_Y_END,	pbox->y2);

	if (bytes == stride) {
	    int n = (bytes * h + 1) >> 1;
	    unsigned char *p = image;

	    while ((n -= 8) >= 0) {
		M32_CLEAR_QUEUE(8);
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		p += 16;
	    }
	    if (n += 8) {
		M32_CLEAR_QUEUE(n);
		while (--n >= 0) {
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    p += 2;
		}
	    }
	} else if (bytes & 1) {
	    while (--h >= 0) {
		int n = bytes;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[0]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[1]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[2]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[3]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[4]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[5]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[6]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[7]));
		    p += 8;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0)
			outb(M32_PIX_TRANS+1, MSBIT_SWAP(*p++));
		}
		image += stride;
	    }
	} else {
	    while (--h >= 0) {
		int n = bytes >> 1;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		    p += 16;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0) {
			outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
			p += 2;
		    }
		}
		image += stride;
	    }
	}

	M32_CLEAR_QUEUE(2);
	if (startx && (pbox->x1 > pM32->clip.x1))
		outw(M32_EXT_SCISSOR_L, pM32->clip.x1);
	if (endx   && (pbox->x2 < pM32->clip.x2))
		outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
}

/*
 * m32DrawOpaqueMonoImage() - Draw color-expanded monochrome image.
 *	pbox - destination rectangle to draw into.
 *	image - the bits to draw.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within image.
 *	startx - number of bits to skip at start of each line of image.
 *	fg - the color to make the '1' bits.
 *	bg - the color to make the '0' bits.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
m32DrawOpaqueMonoImage(
	BoxPtr pbox,
	void *void_image,					/* S002 */
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
	int w, h, endx, bytes;
	int x1, x2;
	int dp;
	unsigned char *image = (unsigned char *)void_image;	/* S002 */

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	image += startx >> 3;
	startx &= 7;
    
	endx = (w + startx) & 7;
	bytes = (w + startx + 7) >> 3;

	dp = ((bytes != stride) && (bytes & 1))
		? M32_DP_EXPBHOST : M32_DP_EXPWHOST;

	x1 = pbox->x1;
	x2 = pbox->x2;

	M32_CLEAR_QUEUE(13);

	if (startx) {
		if (pbox->x1 > pM32->clip.x1)
			outw(M32_EXT_SCISSOR_L, pbox->x1);
		x1 -= startx;
	}
	if (endx) {
		if (pbox->x2 < pM32->clip.x2)
			outw(M32_EXT_SCISSOR_R, pbox->x2 - 1);
		x2 += 8 - endx;
	}

	outw(M32_DP_CONFIG,	dp);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_ALU_BG_FN,	m32RasterOp[alu]);
	outw(M32_FRGD_COLOR,	fg);
	outw(M32_BKGD_COLOR,	bg);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_CUR_X,		x1);
	outw(M32_CUR_Y,		pbox->y1);
	outw(M32_DEST_X_START,	x1);
	outw(M32_DEST_X_END,	x2);
	outw(M32_DEST_Y_END,	pbox->y2);

	if (bytes == stride) {
	    int n = (bytes * h + 1) >> 1;
	    unsigned char *p = image;

	    while ((n -= 8) >= 0) {
		M32_CLEAR_QUEUE(8);
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		p += 16;
	    }
	    if (n += 8) {
		M32_CLEAR_QUEUE(n);
		while (--n >= 0) {
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    p += 2;
		}
	    }
	} else if (bytes & 1) {
	    while (--h >= 0) {
		int n = bytes;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[0]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[1]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[2]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[3]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[4]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[5]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[6]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[7]));
		    p += 8;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0)
			outb(M32_PIX_TRANS+1, MSBIT_SWAP(*p++));
		}
		image += stride;
	    }
	} else {
	    while (--h >= 0) {
		int n = bytes >> 1;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		    p += 16;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0) {
			outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
			p += 2;
		    }
		}
		image += stride;
	    }
	}

	M32_CLEAR_QUEUE(2);
	if (startx && (pbox->x1 > pM32->clip.x1))
		outw(M32_EXT_SCISSOR_L, pM32->clip.x1);
	if (endx   && (pbox->x2 < pM32->clip.x2))
		outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
}

/*
 * m32DrawMonoGlyphs() - Draw color-expanded monochrome glyphs.
 */
void
m32DrawMonoGlyphs(						/* S002 */
    nfbGlyphInfoPtr pGI,
    unsigned int nglyph,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    unsigned long font_priv,
    DrawablePtr pDraw)
{
    m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);

    M32_CLEAR_QUEUE(4);
    outw(M32_ALU_FG_FN,		m32RasterOp[alu]);
    outw(M32_ALU_BG_FN,		m32RasterOp[GXnoop]);
    outw(M32_FRGD_COLOR,	fg);
    outw(M32_WRT_MASK,		planemask);

    do {
	int w, h, endx, bytes;
	int x2, dp;
	unsigned char *image = pGI->image;

	w = pGI->box.x2 - pGI->box.x1;
	h = pGI->box.y2 - pGI->box.y1;

	bytes = (w + 7) >> 3;

	dp = ((bytes != pGI->stride) && (bytes & 1))
		? M32_DP_EXPBHOST : M32_DP_EXPWHOST;

	x2 = pGI->box.x2;
	if (endx = w & 7)
	    x2 += 8 - endx;

	M32_CLEAR_QUEUE(7);
	outw(M32_DP_CONFIG,	dp);
	if (pGI->box.x2 < pM32->clip.x2)
	    outw(M32_EXT_SCISSOR_R, pGI->box.x2 - 1);
	outw(M32_CUR_X,		pGI->box.x1);
	outw(M32_CUR_Y,		pGI->box.y1);
	outw(M32_DEST_X_START,	pGI->box.x1);
	outw(M32_DEST_X_END,	x2);
	outw(M32_DEST_Y_END,	pGI->box.y2);

	if (bytes == pGI->stride) {
	    int n = (bytes * h + 1) >> 1;
	    unsigned char *p = image;

	    while ((n -= 8) >= 0) {
		M32_CLEAR_QUEUE(8);
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		p += 16;
	    }
	    if (n += 8) {
		M32_CLEAR_QUEUE(n);
		while (--n >= 0) {
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    p += 2;
		}
	    }
	} else if (bytes & 1) {
	    while (--h >= 0) {
		int n = bytes;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[0]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[1]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[2]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[3]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[4]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[5]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[6]));
		    outb(M32_PIX_TRANS+1, MSBIT_SWAP(p[7]));
		    p += 8;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0)
			outb(M32_PIX_TRANS+1, MSBIT_SWAP(*p++));
		}
		image += pGI->stride;
	    }
	} else {
	    while (--h >= 0) {
		int n = bytes >> 1;
		unsigned char *p = image;

		while ((n -= 8) >= 0) {
		    M32_CLEAR_QUEUE(8);
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[2])|(MSBIT_SWAP(p[3])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[4])|(MSBIT_SWAP(p[5])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[6])|(MSBIT_SWAP(p[7])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[8])|(MSBIT_SWAP(p[9])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[10])|(MSBIT_SWAP(p[11])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[12])|(MSBIT_SWAP(p[13])<<8));
		    outw(M32_PIX_TRANS, MSBIT_SWAP(p[14])|(MSBIT_SWAP(p[15])<<8));
		    p += 16;
		}
		if (n += 8) {
		    M32_CLEAR_QUEUE(n);
		    while (--n >= 0) {
			outw(M32_PIX_TRANS, MSBIT_SWAP(p[0])|(MSBIT_SWAP(p[1])<<8));
			p += 2;
		    }
		}
		image += pGI->stride;
	    }
	}

	++pGI;
    } while (--nglyph > 0);

    M32_CLEAR_QUEUE(1);
    outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
}
