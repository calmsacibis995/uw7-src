/*
 * @(#) m32FillSp.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 12-Aug-93, buckm
 *	Created.
 * S001, 09-Sep-93, buckm
 *	Adjust m32SolidFS() a bit.
 *	Add oneRect version of GC op FillSpans.
 */

#include "X.h"
#include "Xmd.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"


void
m32SolidFS(pGC, pDraw, ppt, pwidth, nspans)
	GCPtr pGC;
	DrawablePtr pDraw;
	DDXPointPtr ppt;
	unsigned int *pwidth;
	unsigned int nspans;
{
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int n = nspans;

	M32_CLEAR_QUEUE(8);

	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);

	outw(M32_CUR_X,		ppt->x);
	outw(M32_CUR_Y,		ppt->y);
	outw(M32_BRES_COUNT,	*pwidth);

	--n; ++ppt; ++pwidth;

	while ((n -= 4) >= 0) {
		M32_CLEAR_QUEUE(12);

		outw(M32_CUR_X,		ppt[0].x);
		outw(M32_CUR_Y,		ppt[0].y);
		outw(M32_BRES_COUNT,	pwidth[0]);

		outw(M32_CUR_X,		ppt[1].x);
		outw(M32_CUR_Y,		ppt[1].y);
		outw(M32_BRES_COUNT,	pwidth[1]);

		outw(M32_CUR_X,		ppt[2].x);
		outw(M32_CUR_Y,		ppt[2].y);
		outw(M32_BRES_COUNT,	pwidth[2]);

		outw(M32_CUR_X,		ppt[3].x);
		outw(M32_CUR_Y,		ppt[3].y);
		outw(M32_BRES_COUNT,	pwidth[3]);

		ppt += 4; pwidth += 4;
	}
	if (n += 4) {
		M32_CLEAR_QUEUE(9);

		outw(M32_CUR_X,		ppt[0].x);
		outw(M32_CUR_Y,		ppt[0].y);
		outw(M32_BRES_COUNT,	pwidth[0]);

		if (--n) {
			outw(M32_CUR_X,		ppt[1].x);
			outw(M32_CUR_Y,		ppt[1].y);
			outw(M32_BRES_COUNT,	pwidth[1]);

			if (--n) {
				outw(M32_CUR_X,		ppt[2].x);
				outw(M32_CUR_Y,		ppt[2].y);
				outw(M32_BRES_COUNT,	pwidth[2]);
			}
		}
	}
}


#if 0
void
m32TiledFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	DDXPointPtr ppt;
	unsigned int *pwidth;
	unsigned int n;
{
	PixmapPtr pm = pGC->tile.pixmap;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
}


void
m32StippledFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	DDXPointPtr ppt;
	unsigned int *pwidth;
	unsigned int n;
{
	PixmapPtr pm = pGC->stipple;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
}


void
m32OpStippledFS(pGC, pDraw, ppt, pwidth, n)
	GCPtr pGC;
	DrawablePtr pDraw;
	DDXPointPtr ppt;
	unsigned int *pwidth;
	unsigned int n;
{
	PixmapPtr pm = pGC->stipple;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
}
#endif


/*
 * The following is a GC op for FillSpans
 * when there is one clip rectangle.
 */

void
m32FillSpans(pDraw, pGC, nspans, ppt, pwidth, fSorted)
	DrawablePtr pDraw;
	GCPtr pGC;
	unsigned int nspans;
	DDXPointPtr ppt;
	unsigned int *pwidth;
	int fSorted;
{
	if (nspans) {
		nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

		m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);
		(*pGCPriv->ops->FillSpans)(pGC, pDraw, ppt, pwidth, nspans);
		m32SetClip(NULL, 0, pDraw);
	}
}
