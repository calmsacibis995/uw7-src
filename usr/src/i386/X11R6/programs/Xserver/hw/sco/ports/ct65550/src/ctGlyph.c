/*
 *	@(#)ctGlyph.c	11.1	10/22/97	12:34:48
 *	@(#) ctGlyph.c 62.1 97/03/11 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctGlyph.c 62.1 97/03/11 "

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "scoext.h"

#include "nfb/nfbGlyph.h"

#include "ctMacros.h"
#include "ctDefs.h"

void
CT(DrawMonoGlyphs)(glyph_info, nglyphs, fg, alu, planemask, font_id, pDraw)
nfbGlyphInfo *glyph_info;
unsigned int nglyphs;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
unsigned long font_id;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned long *pdst = CT_BLT_DATAPORT(ctPriv->fbPointer);
	unsigned char *image, *psrc;
	int width, height, scanwidth, dwidth, w, stride;
	int h;
	BoxPtr pbox;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawMonoGlyphs(glyph_info, nglyphs, fg, alu, planemask,
				font_id, pDraw);
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
		genDrawMonoGlyphs(glyph_info, nglyphs, fg, alu, ~0L,
				font_id, pDraw);
		return;
	default:
		break;
	}

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, monochrome source data, transparent background,
	 * from memory to screen.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | 
			CT_SRCMONOBIT | CT_BGTRANSBIT | CT_BLTMEMORYBIT);
	CT_SET_BLTFGCOLOR(fg);

	/*
	 * NOTE: source pointer alignment doesn't matter since we bit-swap,
	 * shift, and then cast to a long before writing data to the BitBlt
	 * engine. Thus, the source register (DR05) can always be 0. The source
	 * offset (DR00[11:0]) is not used in monochrome image expansion.
	 */
	CT_SET_BLTOFFSET(0, ctPriv->bltStride);
	CT_SET_BLTSRC(0);
	CT_SET_MONO_CONTROL(CT_MONO_ALIGN_DWORD, 0, 0, 0); 

	while (nglyphs > 0) {
		image = (unsigned char *)glyph_info->image;
		pbox = &glyph_info->box;
		width = pbox->x2 - pbox->x1;
		height = pbox->y2 - pbox->y1;
		stride = glyph_info->stride;
		scanwidth = (width + (8 - 1)) >> 3;	/* in bytes */

		/* 65550 hangs on scanwidth == 9 (text2 -a) */
		/* have gen draw the rest of nglyphs */
		if (scanwidth > 4) {
			genDrawMonoGlyphs(glyph_info, nglyphs, fg, alu, 
				planemask, font_id, pDraw);
			return;
		}

#ifdef DEBUG_PRINT
		ErrorF("DrawMonoGlyphs(): 0x%08x str=%d sw=%d (%d,%d)(%dx%d)\n",
			image, stride, scanwidth,
			pbox->x1, pbox->y1, width, height);
#endif /* DEBUG_PRINT */

		CT_WAIT_FOR_IDLE();
		CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1));
		CT_START_BITBLT(width, height);

		h = height;

		switch(scanwidth) {
		case 1:
			while (h--) {
				CT_WAIT_FOR_BUFFER();
				CT_EXPAND_BYTES1(pdst, image);
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
			ErrorF("DrawMonoGlyphs(): Can't handle arbitrary width images\n");
			break;
		}

#ifdef CT_BITBLT_SANITY_CHECK
		CT(BitBltSanityCheck)(ctPriv->fbPointer);
#endif /* CT_BITBLT_SANITY_CHECK */

		nglyphs--;
		glyph_info++;
	}
}
