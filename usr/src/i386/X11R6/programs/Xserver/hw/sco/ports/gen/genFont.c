/*
 * @(#) genFont.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * genFont.c - routines for 8-bit terminal-emulator fonts.
 */
/*
 *	MODIFICATION HISTORY
 *
 *	S000	brianm@sco.com	Mon Oct 30 14:22:28 PST 1995
 *		- added in a fake call genWinOp10 which directly calls
 *		  genDrawFontText so that we can compile the X server ELF.
 */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "genDefs.h"
#include "genProcs.h"


/*
 * Replace genWinOps10 with genDrawFontText in xxxWinOps structure(s).
 *
 * If you have written your own DrawMonoGlyphs routine,
 * you should strongly consider writing your own DrawFontText routine.
 */

/* S000 Begin */
void
genWinOp10(pbox, chars, count, pPS, fg, bg,
           alu, planemask, transparent, pDraw)
    BoxPtr pbox;
    unsigned char *chars;
    unsigned int count;
    nfbFontPSPtr pPS;
    unsigned long fg;
    unsigned long bg;
    unsigned char alu;
    unsigned long planemask;
    unsigned char transparent;
    DrawablePtr pDraw;
{
    genDrawFontText(pbox, chars, count, pPS, fg, bg,
                    alu, planemask, transparent, pDraw);
}
/* S000 End */

void
genDrawFontText(
    BoxPtr pbox,
    unsigned char *chars,
    unsigned int count,
    nfbFontPSPtr pPS,
    unsigned long fg,
    unsigned long bg,
    unsigned char alu,
    unsigned long planemask,
    unsigned char transparent,
    DrawablePtr pDraw)
{
    nfbWinOpsPtr pOps = NFB_WINDOW_PRIV(pDraw)->ops;
    BoxRec box = *pbox;
    int width, stride;
    unsigned char **ppbits;

    width  = pPS->pFontInfo->width;
    stride = pPS->pFontInfo->stride;
    ppbits = pPS->pFontInfo->ppBits;

    if (pOps->DrawMonoGlyphs != genDrawMonoGlyphs) {

	/*
	 * A (presumably) fast DrawMonoGlyphs has been written;
	 * so let's use it.
	 */

	nfbGlyphInfoPtr GI_ary, pGI;
	int i;

	/*
	 * Paint a large background rectangle.
	 */
	if (!transparent)
	    (*pOps->DrawSolidRects)(pbox, 1, bg, alu, planemask, pDraw);

	/*
	 * Build an array of nfbGlyphInfo structs.
	 */
	GI_ary = (nfbGlyphInfoPtr)ALLOCATE_LOCAL(count * sizeof(nfbGlyphInfo));

	for (pGI = GI_ary, i = count; --i >= 0; ++pGI) {
	    box.x2 = box.x1 + width;
	    pGI->box	  = box;
	    pGI->stride	  = stride;
	    pGI->image	  = ppbits[*chars++];
	    pGI->glyph_id = (unsigned int)pGI->image;
	    box.x1 = box.x2;
	}

	/*
	 * Draw the glyphs.
	 */
	(* pOps->DrawMonoGlyphs)(GI_ary, count, fg, alu, planemask, pPS, pDraw);

	DEALLOCATE_LOCAL(GI_ary);

    } else {  /* pOps->DrawMonoGlyphs == genDrawMonoGlyphs */

	/*
	 * No sense in calling genDrawMonoGlyphs here,
	 * because it will just call DrawMonoImage.
	 */

#ifdef FAST_OPAQUE_DRAW

	/*
	 * If you can draw opaque mono images as fast as mono images,
	 * let the background be drawn at the same time as the glyphs.
	 */
	if (transparent) {
	    do {
		box.x2 = box.x1 + width;
		(* pOps->DrawMonoImage)(&box, ppbits[*chars++], 0, stride,
					fg, alu, planemask, pDraw);
		box.x1 = box.x2;
	    } while (--count > 0);
	} else {
	    do {
		box.x2 = box.x1 + width;
		(* pOps->DrawOpaqueMonoImage)(&box, ppbits[*chars++], 0, stride,
					      fg, bg, alu, planemask, pDraw);
		box.x1 = box.x2;
	    } while (--count > 0);
	}

#else

	/*
	 * Paint a large background rectangle; draw the glyphs on top of it.
	 */
	if (!transparent)
	    (*pOps->DrawSolidRects)(pbox, 1, bg, alu, planemask, pDraw);

	do {
	    box.x2 = box.x1 + width;
	    (* pOps->DrawMonoImage)(&box, ppbits[*chars++], 0, stride,
				    fg, alu, planemask, pDraw);
	    box.x1 = box.x2;
	} while (--count > 0);

#endif

    }

}
