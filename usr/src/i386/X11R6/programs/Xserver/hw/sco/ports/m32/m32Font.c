/*
 * @(#) m32Font.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 18-Aug-93, buckm
 *	Created.
 * S001, 01-Sep-93, buckm
 *	Fiddle with DrawFontText.
 * S002, 20-Sep-93, buckm
 *	Must keep track of TE8 font char count for TBird nfb interface,
 *	and not draw glyphs beyond end of font.
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


int
m32InitTE8(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr	pM32 = M32_SCREEN_PRIV(pScr);
	m32TE8InfoPtr	pTI  = &pM32->te8Info;
	m32Font8InfoPtr	pFI;
	DDXPointPtr	pGA;
	DDXPointRec	gaddr;
	int		fontlocs, fontsperloc, glyphlocs;
	int		xinc, yinc;
	int		i, j;

	fontsperloc = (8 / M32_TE8_PLANES) << pM32->pixBytesLog2;
	fontlocs    = M32_TE8_FONTS / fontsperloc;
	glyphlocs   = fontlocs * M32_TE8_GLYPH_LOCS;

	if ((pGA = (DDXPointPtr)xalloc(glyphlocs * sizeof gaddr)) == NULL)
		return FALSE;

	pTI->pGlyphAddr = pGA;

	yinc = M32_TE8_GLYPH_BITS / pM32->fbPitch;
	xinc = M32_TE8_GLYPH_BITS % pM32->fbPitch;

	gaddr = pTI->addr;
	*pGA++ = gaddr;
	for (i = glyphlocs; --i > 0; ) {
		gaddr.y += yinc;
		gaddr.x += xinc;
		if (gaddr.x >= pM32->fbPitch) {
			gaddr.x -= pM32->fbPitch;
			gaddr.y += 1;
		}
		*pGA++ = gaddr;
	}

	pGA = pTI->pGlyphAddr;
	pFI = &pTI->font[0];
	for (i = 0; i < fontlocs; ++i) {
		unsigned long plane = 1;

		for (j = 0; j < fontsperloc; ++j) {
			pFI->basePlane	= plane;
			pFI->pGlyphAddr	= pGA;
			++pFI;
			plane <<= M32_TE8_PLANES;
		}
		pGA += M32_TE8_GLYPH_LOCS;
	}

	return TRUE;
}

void
m32FreeTE8(pScr)
	ScreenPtr pScr;
{
	m32TE8InfoPtr pTI = M32_TE8_INFO(pScr);

	xfree((char *)pTI->pGlyphAddr);
}

#if M32_TE8_WIDTH > 16
#pragma	message ("ERROR: m32DownLoadFont needs modification for TE8_WIDTH > 16")
))))) *ERROR*
#endif

void
m32DownloadFont8(bits, count, width, height, stride, index, pScr)
	unsigned char **bits;
	int count;
	int width;
        int height;
	int stride;
	int index;
	ScreenPtr pScr;
{
	m32ScrnPrivPtr	pM32 = M32_SCREEN_PRIV(pScr);
	m32Font8InfoPtr	pFI  = &pM32->te8Info.font[index];
	int		i, j;

#ifdef agaII
	pFI->count = count;
#endif

	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_EXPPATT);
	outw(M32_ALU_FG_FN,	m32RasterOp[GXset]);
	outw(M32_ALU_BG_FN,	m32RasterOp[GXclear]);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);
	outw(M32_PATT_LENGTH,	width - 1);

	/* for each plane in font */
	for (i = 0; i < M32_TE8_PLANES; ++i) {
	    DDXPointPtr pg = pFI->pGlyphAddr;

	    M32_CLEAR_QUEUE(1);
	    outw(M32_WRT_MASK,	pFI->basePlane << i);

	    /* for each char in plane */
	    for (j = i; j < count; j += M32_TE8_PLANES) {
		unsigned char *	bp = bits[j];
		DDXPointRec	pt = *pg++;
		int		h  = height;

		M32_CLEAR_QUEUE(2);
		outw(M32_CUR_X,	pt.x);
		outw(M32_CUR_Y,	pt.y);

		/* for each scan in char */
		while (--h >= 0) {
		    int scan = MSBIT_SWAP(bp[0]) | (MSBIT_SWAP(bp[1]) << 8);
		    int over;

		    M32_CLEAR_QUEUE(4);
		    outw(M32_PATT_DATA_INDEX,	0x10);
		    outw(M32_PATT_DATA,		scan);
		    outw(M32_PATT_INDEX,	0);

		    pt.x += width;

		    if ((over = pt.x - pM32->fbPitch) < 0) {
			outw(M32_BRES_COUNT,	width);
		    } else {
			outw(M32_BRES_COUNT,	width - over);

			pt.x  = over;
			pt.y += 1;

			M32_CLEAR_QUEUE(4);
			outw(M32_CUR_X,		0);
			outw(M32_CUR_Y,		pt.y);

			if (over > 0) {
			    outw(M32_PATT_INDEX, width - over);
			    outw(M32_BRES_COUNT, over);
			}
		    }

		    bp += stride;
		}
	    }
	}
}

void
m32DrawFontText(pbox, chars, count, width, index,
		fg, bg, alu, planemask, transparent, pDraw)
	BoxPtr pbox;
	unsigned char *chars;
	unsigned int count;
        unsigned short width;
        int index;
	unsigned long fg;
	unsigned long bg;
	unsigned char alu;
	unsigned long planemask;
	unsigned char transparent;
	DrawablePtr pDraw;
{
	m32ScrnPrivPtr	pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
	m32Font8InfoPtr	pFI  = &pM32->te8Info.font[index];
	int		x    = pbox->x1;

	M32_CLEAR_QUEUE(9);
	outw(M32_DP_CONFIG,	M32_DP_EXPAND);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_ALU_BG_FN,	transparent ? m32RasterOp[GXnoop]
					    : m32RasterOp[alu]);
	outw(M32_FRGD_COLOR,	fg);
	outw(M32_BKGD_COLOR,	bg);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_SRC_X_START,	0);
	outw(M32_SRC_X_END,	pM32->fbPitch);
	outw(M32_SRC_Y_DIR,	1);

	do {
		DDXPointPtr pg;
		unsigned long plane;

#ifdef agaII
		if (*chars >= pFI->count)
			continue;
#endif
		pg = &pFI->pGlyphAddr[*chars / M32_TE8_PLANES];
		plane = pFI->basePlane << (*chars++ % M32_TE8_PLANES);

		M32_CLEAR_QUEUE(8);
		outw(M32_RD_MASK,	plane);
		outw(M32_SRC_X,		pg->x);
		outw(M32_SRC_Y,		pg->y);
		outw(M32_CUR_X,		x);
		outw(M32_DEST_X_START,	x);
		outw(M32_DEST_X_END,	x += width);
		outw(M32_CUR_Y,		pbox->y1);
		outw(M32_DEST_Y_END,	pbox->y2);
	} while (--count > 0);
}
