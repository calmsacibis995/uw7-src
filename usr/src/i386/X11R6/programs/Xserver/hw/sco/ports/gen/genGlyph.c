/*
 * @(#) genGlyph.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "mfb/mfb.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"


#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "genDefs.h"
#include "genProcs.h"

void
genDrawMonoGlyphs(
    nfbGlyphInfo *glyph_info,
    unsigned int nglyphs,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    nfbFontPSPtr pPS,
    DrawablePtr pDrawable)
{
	int i;
	void (*DrawMonoImage)();

	DrawMonoImage = (void (*)())(NFB_WINDOW_PRIV(pDrawable))->ops->DrawMonoImage;

	for (i = 0; i < nglyphs; ++i, ++glyph_info)
		(*DrawMonoImage)(&glyph_info->box, glyph_info->image, 0,
		    glyph_info->stride, fg, alu, planemask, pDrawable);
}

