/*
 *	@(#)ctFont.c	11.1	10/22/97	12:33:54
 *	@(#) ctFont.c 58.1 96/10/09 
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
 *
 *	SCO Modifications
 *
 *	S000	Fri Jul 01 12:18:33 PDT 1994	davidw@sco.com
 *	DrawFontText support not available in TBIRD agaII.
 */

#ident "@(#) $Id: ctFont.c 58.1 96/10/09 "

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "scoext.h"

#include "nfb/nfbGlyph.h"

#include "ctMacros.h"
#include "ctDefs.h"

#ifndef agaII
void
CT(DrawFontText)(pbox, chars, count, pPS, fg, bg, alu, planemask, trans, pDraw)
BoxPtr pbox;
unsigned char *chars;
unsigned int count;
nfbFontPSPtr pPS;
unsigned long fg;
unsigned long bg;
unsigned char alu;
unsigned long planemask;
unsigned char trans;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned long *pdst = (unsigned long *)ctPriv->fbPointer;
	unsigned char *image, *psrc;
	unsigned char **ppbits = pPS->pFontInfo->ppBits;
	int x, y, width, height, stride, scanwidth, dwidth, w, h;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawFontText(pbox, chars, count, pPS, fg, bg, alu,
				planemask, trans, pDraw);
		return;
	}

	/*
	 * NOTE: According to the X Protocol, XDrawImageString() ignores the GC
	 * function (ALU) and always uses GXcopy, foreground, and background
	 * colors. Thus, if trans == FALSE (we are drawing both bg and fg), we
	 * should force the alu to GXcopy.
	 */
	if (trans == FALSE)
		alu = GXcopy;

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
		genDrawFontText(pbox, chars, count, pPS, fg, bg, alu, ~0L,
				trans, pDraw);
		return;
	default:
		break;
	}

	x = pbox->x1;
	y = pbox->y1;
	width  = pPS->pFontInfo->width;
	height = pbox->y2 - pbox->y1;
	stride = pPS->pFontInfo->stride;
	scanwidth = (width + (8 - 1)) >> 3;	/* in bytes */

	CT_WAIT_FOR_IDLE();

	if (trans) {
		/*
		 * Set BitBlt control register to copy using mapped alu,
		 * left-to-right, top-to-bottom, monochrome source data,
		 * transparent background, from memory to screen.
		 */
		CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_SRCMONOBIT | CT_BGTRANSBIT | CT_BLTMEMORYBIT);
		CT_SET_BLTFGCOLOR(fg);
	} else {
		/*
		 * Set BitBlt control register to copy using mapped alu,
		 * left-to-right, top-to-bottom, monochrome source data,
		 * opaque background, from memory to screen.
		 *
		 * NOTE: assume opaque text uses GXcopy with fg and bg.
		 */
		CT_SET_BLTCTRL(CT(PixelOps)[GXcopy] | CT_XINCBIT | CT_YINCBIT |
				CT_SRCMONOBIT | CT_BLTMEMORYBIT);
		CT_SET_BLTFGCOLOR(fg);
		CT_SET_BLTBGCOLOR(bg);
	}

	/*
	 * NOTE: source pointer alignment doesn't matter since we bit-swap,
	 * shift, and then cast to a long before writing data to the BitBlt
	 * engine. Thus, the source register (DR05) can always be 0. The source
	 * offset (DR00[11:0]) is not used in monochrome image expansion.
	 */
	CT_SET_BLTOFFSET(0, ctPriv->bltStride);
	CT_SET_BLTSRC(0);

	switch(scanwidth) {
	case 1:
		while (count--) {
			CT_WAIT_FOR_IDLE();
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
			CT_START_BITBLT(width, height);
			h = height;
			image = ppbits[*chars];

#ifdef DEBUG_PRINT
			ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
				image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

			while (h--) {
				/*
				 * NOTE: we could pack 4 of these glyphs into
				 * each dword write for further optimization.
				 * This is probably only worth implementing for
				 * 8 bit wide fonts due to alignment issues.
				 */
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES1(pdst, image);
				image += stride;
			}

#ifdef CT_BITBLT_SANITY_CHECK
			CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

			x += width;
			chars++;
		}
		break;
	case 2:
		while (count--) {
			CT_WAIT_FOR_IDLE();
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
			CT_START_BITBLT(width, height);
			h = height;
			image = ppbits[*chars];

#ifdef DEBUG_PRINT
			ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
				image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

			while (h--) {
				/*
				 * NOTE: we could pack 2 of these glyphs into
				 * each dword write for further optimization.
				 * This is probably only worth implementing for
				 * 8 bit wide fonts due to alignment issues.
				 */
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES2(pdst, image);
				image += stride;
			}

#ifdef CT_BITBLT_SANITY_CHECK
			CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

			x += width;
			chars++;
		}
		break;
	case 3:
		while (count--) {
			CT_WAIT_FOR_IDLE();
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
			CT_START_BITBLT(width, height);
			h = height;
			image = ppbits[*chars];

#ifdef DEBUG_PRINT
			ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
				image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

			while (h--) {
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES3(pdst, image);
				image += stride;
			}

#ifdef CT_BITBLT_SANITY_CHECK
			CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

			x += width;
			chars++;
		}
		break;
	case 4:
		while (count--) {
			CT_WAIT_FOR_IDLE();
			CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
			CT_START_BITBLT(width, height);
			h = height;
			image = ppbits[*chars];

#ifdef DEBUG_PRINT
			ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
				image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

			while (h--) {
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES4(pdst, image);
				image += stride;
			}

#ifdef CT_BITBLT_SANITY_CHECK
			CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

			x += width;
			chars++;
		}
		break;
	default:
		/*
		 * Here, we handle arbitrary width image data.
		 */
		dwidth = scanwidth >> 2;		/* in dwords */
		switch (scanwidth & 3) {		/* extra */
		case 0:
			while (count--) {
				CT_WAIT_FOR_IDLE();
				CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
				CT_START_BITBLT(width, height);
				h = height;
				image = ppbits[*chars];

#ifdef DEBUG_PRINT
				ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
					image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

				while (h--) {
					psrc = image;
					w = dwidth;
					while (w--) {
						CT_WAIT_FOR_BUFFER();
						CT_EXPAND_BYTES4(pdst, psrc);
						psrc += 4;
					}
					image += stride;
				}

#ifdef CT_BITBLT_SANITY_CHECK
				CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

				x += width;
				chars++;
			}
			break;
		case 1:
			while (count--) {
				CT_WAIT_FOR_IDLE();
				CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
				CT_START_BITBLT(width, height);
				h = height;
				image = ppbits[*chars];

#ifdef DEBUG_PRINT
				ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
					image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

				while (h--) {
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

#ifdef CT_BITBLT_SANITY_CHECK
				CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

				x += width;
				chars++;
			}
			break;
		case 2:
			while (count--) {
				CT_WAIT_FOR_IDLE();
				CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
				CT_START_BITBLT(width, height);
				h = height;
				image = ppbits[*chars];

#ifdef DEBUG_PRINT
				ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
					image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

				while (h--) {
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

#ifdef CT_BITBLT_SANITY_CHECK
				CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

				x += width;
				chars++;
			}
			break;
		case 3:
			while (count--) {
				CT_WAIT_FOR_IDLE();
				CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, x, y));
				CT_START_BITBLT(width, height);
				h = height;
				image = ppbits[*chars];

#ifdef DEBUG_PRINT
				ErrorF("DrawFontText(): 0x%08x s=%d (%d,%d)(%dx%d)\n",
					image, stride, x, y, width, height);
#endif /* DEBUG_PRINT */

				while (h--) {
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

#ifdef CT_BITBLT_SANITY_CHECK
				CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

				x += width;
				chars++;
			}
			break;
		}
		break;
	}
}
#endif /* agaII */
