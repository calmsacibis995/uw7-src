/*
 * @(#) m32FillRct.c 11.1 97/10/22
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
 * S001, 01-Sep-93, buckm
 *	Get rid of m32SolidFillRects() checks for one pixel wide/high
 *	rectangles now that PolyRectangles no longer comes here via nfb.
 *	Batch up several rects at once.
 *	Implement stippled fills.
 * S002, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */

#include "X.h"
#include "Xmd.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbRop.h"

#include "m32Defs.h"
#include "m32ScrStr.h"


void
m32SolidFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int n = nbox;

	M32_CLEAR_QUEUE(9);

	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);

	outw(M32_CUR_X,		pbox->x1);
	outw(M32_DEST_X_START,	pbox->x1);
	outw(M32_DEST_X_END,	pbox->x2);
	outw(M32_CUR_Y,		pbox->y1);
	outw(M32_DEST_Y_END,	pbox->y2);

	--n; ++pbox;

	while ((n -= 3) >= 0) {
		M32_CLEAR_QUEUE(15);

		outw(M32_CUR_X,		pbox[0].x1);
		outw(M32_DEST_X_START,	pbox[0].x1);
		outw(M32_DEST_X_END,	pbox[0].x2);
		outw(M32_CUR_Y,		pbox[0].y1);
		outw(M32_DEST_Y_END,	pbox[0].y2);

		outw(M32_CUR_X,		pbox[1].x1);
		outw(M32_DEST_X_START,	pbox[1].x1);
		outw(M32_DEST_X_END,	pbox[1].x2);
		outw(M32_CUR_Y,		pbox[1].y1);
		outw(M32_DEST_Y_END,	pbox[1].y2);

		outw(M32_CUR_X,		pbox[2].x1);
		outw(M32_DEST_X_START,	pbox[2].x1);
		outw(M32_DEST_X_END,	pbox[2].x2);
		outw(M32_CUR_Y,		pbox[2].y1);
		outw(M32_DEST_Y_END,	pbox[2].y2);

		pbox += 3;
	}
	if (n += 3) {
		M32_CLEAR_QUEUE(10);

		outw(M32_CUR_X,		pbox[0].x1);
		outw(M32_DEST_X_START,	pbox[0].x1);
		outw(M32_DEST_X_END,	pbox[0].x2);
		outw(M32_CUR_Y,		pbox[0].y1);
		outw(M32_DEST_Y_END,	pbox[0].y2);

		if (--n) {
			outw(M32_CUR_X,		pbox[1].x1);
			outw(M32_DEST_X_START,	pbox[1].x1);
			outw(M32_DEST_X_END,	pbox[1].x2);
			outw(M32_CUR_Y,		pbox[1].y1);
			outw(M32_DEST_Y_END,	pbox[1].y2);
		}
	}
}


#if 0
void
m32TiledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	PixmapPtr pm = pGC->tile.pixmap;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);
}
#endif


void
m32StippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	int stipw = pGC->stipple->drawable.width;
	int stiph = pGC->stipple->drawable.height;
	nfbGCPrivPtr pGCPriv;
	m32OSInfoPtr pOS;

	/* use the pattern regs ? */
	if ((stipw <= M32_MAX_PATTERN) && (stiph <= M32_MAX_PATTERN)) {
	    m32HWStippledFillRects(pGC, pDraw, pbox, nbox);
	    return;
	}

	/* let gen do it */
	genStippledFillRects(pGC, pDraw, pbox, nbox);
}


void
m32OpStippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	int stipw = pGC->stipple->drawable.width;
	int stiph = pGC->stipple->drawable.height;
	nfbGCPrivPtr pGCPriv;
	m32OSInfoPtr pOS;

	/* use the pattern regs ? */
	if ((stipw <= M32_MAX_PATTERN) && (stiph <= M32_MAX_PATTERN)) {
		BoxPtr pb;
		int nb;

		/* check for a rect wider than the tile */
		for (nb = nbox, pb = pbox; --nb >= 0; ++pb)
			if ((pb->x2 - pb->x1) > stipw)
				break;

		if (nb >= 0) {
			m32HWStippledFillRects(pGC, pDraw, pbox, nbox);
			return;
		}
	}

	/* use the off-screen scratch area ? */
	pGCPriv = NFB_GC_PRIV(pGC);
	pOS = M32_OS_INFO(pDraw->pScreen);
	if (!rop_needs_dst[pGCPriv->rRop.alu] &&
	    ((stipw * 2) <= pOS->width) &&
	    ((stiph * 2) <= pOS->height)) {
		m32OSOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/* let gen do it */
	genOpStippledFillRects(pGC, pDraw, pbox, nbox);
}


m32HWStippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	DDXPointPtr patOrg = &(pGCPriv->screenPatOrg);
	PixmapPtr pStip = pGC->stipple;
	int stipw = pStip->drawable.width;
	int stiph = pStip->drawable.height;
	int stride = pStip->devKind;
	unsigned char *stip = pStip->devPrivate.ptr; 

	DDXPointPtr poffset, po;
	BoxPtr pb;
	int nb, row;

	poffset = (DDXPointPtr)ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec));

	/* calculate stipple offsets for each box */
	for (nb = nbox, pb = pbox, po = poffset; --nb >= 0; ++pb, ++po) {
		if ((po->x = (pb->x1 - patOrg->x) % (int)stipw) < 0)
			po->x += stipw;
		if ((po->y = (pb->y1 - patOrg->y) % (int)stiph) < 0)
			po->y += stiph;
	}

	/* setup some invariants */
	M32_CLEAR_QUEUE(8);
	outw(M32_DP_CONFIG,	M32_DP_EXPPATT);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	if (pGC->fillStyle == FillOpaqueStippled) {
		outw(M32_ALU_BG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
		outw(M32_BKGD_COLOR,	pGCPriv->rRop.bg);
	} else {
		outw(M32_ALU_BG_FN,	m32RasterOp[GXnoop]);
	}
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);
	outw(M32_PATT_LENGTH,	stipw - 1);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);

	/* for each tile row */
	for (row = 0; row < stiph; ++row, stip += stride) {

	    /* load the stipple row into the pattern regs */
	    M32_CLEAR_QUEUE(3);
	    outw(M32_PATT_DATA_INDEX, 0x10);
	    outw(M32_PATT_DATA, MSBIT_SWAP(stip[0])|(MSBIT_SWAP(stip[1])<<8));
	    if (stipw >= 16)
	      outw(M32_PATT_DATA, MSBIT_SWAP(stip[2])|(MSBIT_SWAP(stip[3])<<8));

	    /* for each rect */
	    for (nb = nbox, pb = pbox, po = poffset; --nb >= 0; ++pb, ++po) {
		int w = pb->x2 - pb->x1;
		int y;

		/* find 1st row in rect matching this stipple row */
		if ((y = pb->y1 + row - po->y) < pb->y1)
		    y += stiph;

		/* paint all rows in rect matching this stipple row */
		while (y < pb->y2) {
		    M32_CLEAR_QUEUE(4);
		    outw(M32_PATT_INDEX,	po->x);
		    outw(M32_CUR_X,		pb->x1);
		    outw(M32_CUR_Y,		y);
		    outw(M32_BRES_COUNT,	w);

		    y += stiph;
		}
	    }

	}

	DEALLOCATE_LOCAL(poffset);
}


m32OSOpStippledFillRects(pGC, pDraw, pbox, nbox)
	GCPtr pGC;
	DrawablePtr pDraw;
	BoxPtr pbox;
	unsigned int nbox;
{
	m32OSInfoPtr pOS = M32_OS_INFO(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	DDXPointPtr patOrg = &(pGCPriv->screenPatOrg);
	PixmapPtr pStip = pGC->stipple;
	int stipw = pStip->drawable.width;
	int stiph = pStip->drawable.height;

	BoxRec osbox;

	void (*DrawOMI)(					/* S002 */
		BoxPtr pbox,
        	void *image,
        	unsigned int startx,
        	unsigned int stride,
        	unsigned long fg,
        	unsigned long bg,
        	unsigned char alu,
        	unsigned long planemask,
        	DrawablePtr pDraw);

	DrawOMI = NFB_WINDOW_PRIV(pDraw)->ops->DrawOpaqueMonoImage; /* S002 */
	/* put a copy of the stipple into off-screen as a tile */

	osbox.x2 = (osbox.x1 = pOS->addr.x) + stipw;
	osbox.y2 = (osbox.y1 = pOS->addr.y) + stiph;
	(*DrawOMI)(&osbox, pStip->devPrivate.ptr, 0, pStip->devKind,
		   pGCPriv->rRop.fg, pGCPriv->rRop.bg, GXcopy, ~0, pDraw);

	/* quadruple the tile */

	M32_CLEAR_QUEUE(7);

	outw(M32_DP_CONFIG,	M32_DP_COPY);
	outw(M32_ALU_FG_FN,	m32RasterOp[GXcopy]);
	outw(M32_WRT_MASK,	~0);
	outw(M32_MAJ_AXIS_PCNT, stipw - 1);
	outw(M32_MIN_AXIS_PCNT, stiph - 1);
	outw(M32_CUR_X,		osbox.x1);
	outw(M32_CUR_Y,		osbox.y1);

	M32_CLEAR_QUEUE(8);

	outw(M32_DEST_X,	osbox.x2);
	outw(M32_DEST_Y,	osbox.y1);
	outw(M32_CMD,		M32_CMD_BLIT);

	outw(M32_MAJ_AXIS_PCNT, stipw * 2 - 1);
	outw(M32_DEST_X,	osbox.x1);
	outw(M32_CMD,		M32_CMD_BLIT);

	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);

	/* start tiling rects */

	do {
		int xoff, yoff;
		int boxw, boxh;
		int x;
		int w, h, wleft, hleft;

		if ((xoff = (pbox->x1 - patOrg->x) % (int)stipw) < 0)
			xoff += stipw;
		if ((yoff = (pbox->y1 - patOrg->y) % (int)stiph) < 0)
			yoff += stiph;

		boxw = pbox->x2 - pbox->x1;
		if ((w = stipw) > boxw)
			w = boxw;
		wleft = boxw - w;

		boxh = pbox->y2 - pbox->y1;
		if ((h = stiph) > boxh)
			h = boxh;
		hleft = boxh - h;

		/* copy the tile into the top left corner */

		M32_CLEAR_QUEUE(7);
		outw(M32_MAJ_AXIS_PCNT, w - 1);
		outw(M32_MIN_AXIS_PCNT, h - 1);
		outw(M32_CUR_X,		osbox.x1 + xoff);
		outw(M32_CUR_Y,		osbox.y1 + yoff);
		outw(M32_DEST_X,	pbox->x1);
		outw(M32_DEST_Y,	pbox->y1);
		outw(M32_CMD,		M32_CMD_BLIT);

		/* now replicate it throughout the rect */

		if (wleft || hleft) {
			M32_CLEAR_QUEUE(2);
			outw(M32_CUR_X,	pbox->x1);
			outw(M32_CUR_Y,	pbox->y1);

			if (wleft) {
				x = pbox->x1 + w;
				do {
					if (w > wleft)
						w = wleft;
					M32_CLEAR_QUEUE(4);
					outw(M32_MAJ_AXIS_PCNT,	w - 1);
					outw(M32_DEST_X,	x);
					outw(M32_DEST_Y,	pbox->y1);
					outw(M32_CMD,		M32_CMD_BLIT);
					x += w;
					wleft -= w;
					w += w;
				} while (wleft);
			}

			if (hleft) {
				M32_CLEAR_QUEUE(2);
				outw(M32_MAJ_AXIS_PCNT, boxw - 1);
				outw(M32_DEST_X,	pbox->x1);

				do {
					if (h > hleft)
						h = hleft;
					M32_CLEAR_QUEUE(2);
					outw(M32_MIN_AXIS_PCNT, h - 1);
					outw(M32_CMD,		M32_CMD_BLIT);
					hleft -= h;
					h += h;
				} while (hleft);
			}
		}

		++pbox;
	} while (--nbox);
}
