/*
 * @(#) m32RectOps.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 03-Aug-93, buckm
 *	Created.
 * S001, 01-Sep-93, buckm
 *	Batch solid rects.
 *	Implement TileRects.
 *	Add oneRect version of GC op PolyPoint.
 * S002, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */
/*
 * Mach-32 low level drawing primitives
 */

#include "X.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbRop.h"
#include "nfb/nfbWinStr.h"

#include "m32Defs.h"
#include "m32Procs.h"
#include "m32ScrStr.h"


/*
 * m32CopyRect() - Copy a rectangle from screen to screen.
 *	pdstBox - the destination rectangle.
 *	psrc - the source point.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
m32CopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
#if 1

	M32_CLEAR_QUEUE(13);

	outw(M32_DP_CONFIG,	M32_DP_COPY);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);

	if (psrc->x >= pdstBox->x1) {	/* copy left to right */
		int sxend = psrc->x + pdstBox->x2 - pdstBox->x1;

		outw(M32_SRC_X,		psrc->x);
		outw(M32_SRC_X_START,	psrc->x);
		outw(M32_SRC_X_END,	sxend);
		outw(M32_CUR_X,		pdstBox->x1);
		outw(M32_DEST_X_START,	pdstBox->x1);
		outw(M32_DEST_X_END,	pdstBox->x2);
	} else {			/* copy right to left */
		int sxbeg = psrc->x + pdstBox->x2 - pdstBox->x1;

		outw(M32_SRC_X,		sxbeg);
		outw(M32_SRC_X_START,	sxbeg);
		outw(M32_SRC_X_END,	psrc->x);
		outw(M32_CUR_X,		pdstBox->x2);
		outw(M32_DEST_X_START,	pdstBox->x2);
		outw(M32_DEST_X_END,	pdstBox->x1);
	}

	if (psrc->y >= pdstBox->y1) {	/* copy top to bottom */
		outw(M32_SRC_Y,		psrc->y);
		outw(M32_SRC_Y_DIR,	1);
		outw(M32_CUR_Y,		pdstBox->y1);
		outw(M32_DEST_Y_END,	pdstBox->y2);
	} else {			/* copy bottom to top */
		int sybeg = (psrc->y + pdstBox->y2 - pdstBox->y1) - 1;

		outw(M32_SRC_Y,		sybeg);
		outw(M32_SRC_Y_DIR,	0);
		outw(M32_CUR_Y,		pdstBox->y2 - 1);
		outw(M32_DEST_Y_END,	pdstBox->y1 - 1);
	}

#else	/* 8514 blit */

	int w, h;
	int x0, x1, y0, y1;
	int cmd = M32_CMD_BLIT;

	w = (pdstBox->x2 - pdstBox->x1) - 1;
	h = (pdstBox->y2 - pdstBox->y1) - 1;

	M32_CLEAR_QUEUE(10);

	outw(M32_DP_CONFIG,	M32_DP_COPY);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_MAJ_AXIS_PCNT, w);
	outw(M32_MIN_AXIS_PCNT, h);

	x0 = psrc->x;
	y0 = psrc->y;
	x1 = pdstBox->x1;
	y1 = pdstBox->y1;

	if (x1 > x0) {		/* right to left */
		x0 += w;
		x1 += w;
		cmd &= ~0x20;
	}
	if (y1 > y0) {		/* bottom to top */
		y0 += h;
		y1 += h;
		cmd &= ~0x80;
	}

	outw(M32_CUR_X,  x0);
	outw(M32_CUR_Y,	 y0);
	outw(M32_DEST_X, x1);
	outw(M32_DEST_Y, y1);
	outw(M32_CMD,	 cmd);

#endif
}


/*
 * m32DrawSolidRects() - Draw solid-color rectangles.
 *	pbox - array of rectangles to draw.
 *	nbox - number of rectangles.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
m32DrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int n = nbox;

	M32_CLEAR_QUEUE(9);

	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);

	outw(M32_CUR_X,		pbox->x1);
	outw(M32_DEST_X_START,	pbox->x1);
	outw(M32_DEST_X_END,	pbox->x2);
	outw(M32_CUR_Y,		pbox->y1);
	outw(M32_DEST_Y_END,	pbox->y2);

	--n; ++pbox;

	while ((n -= 2) >= 0) {
		M32_CLEAR_QUEUE(10);

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

		pbox += 2;
	}
	if (n & 1) {
		M32_CLEAR_QUEUE(5);
		outw(M32_CUR_X,		pbox->x1);
		outw(M32_DEST_X_START,	pbox->x1);
		outw(M32_DEST_X_END,	pbox->x2);
		outw(M32_CUR_Y,		pbox->y1);
		outw(M32_DEST_Y_END,	pbox->y2);
	}
}


/*
 * m32DrawPoints() - Draw points.
 *	ppt - array of points to draw.
 *	npts - number of points.
 *	fg - the color to make them.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */
void
m32DrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int n = npts;

	M32_CLEAR_QUEUE(8);

	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_LINEDRAW_OPT,	M32_LD_DOT);

	outw(M32_CUR_X,		ppt->x);
	outw(M32_CUR_Y,		ppt->y);
	outw(M32_BRES_COUNT,	1);

	--n; ++ppt;

	while ((n -= 5) >= 0) {
		M32_CLEAR_QUEUE(15);

		outw(M32_CUR_X,		ppt[0].x);
		outw(M32_CUR_Y,		ppt[0].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[1].x);
		outw(M32_CUR_Y,		ppt[1].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[2].x);
		outw(M32_CUR_Y,		ppt[2].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[3].x);
		outw(M32_CUR_Y,		ppt[3].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[4].x);
		outw(M32_CUR_Y,		ppt[4].y);
		outw(M32_BRES_COUNT,	1);

		ppt += 5;
	}

	if (n += 5) {
		M32_CLEAR_QUEUE(12);

		outw(M32_CUR_X,		ppt[0].x);
		outw(M32_CUR_Y,		ppt[0].y);
		outw(M32_BRES_COUNT,	1);

		if (--n) {
			outw(M32_CUR_X,		ppt[1].x);
			outw(M32_CUR_Y,		ppt[1].y);
			outw(M32_BRES_COUNT,	1);

			if (--n) {
				outw(M32_CUR_X,		ppt[2].x);
				outw(M32_CUR_Y,		ppt[2].y);
				outw(M32_BRES_COUNT,	1);

				if (--n) {
					outw(M32_CUR_X,		ppt[3].x);
					outw(M32_CUR_Y,		ppt[3].y);
					outw(M32_BRES_COUNT,	1);
				}
			}
		}
	}
}

/*
 * The following is a GC op for PolyPoint
 * when there is one clip rectangle.
 */
void
m32PolyPoint(
	DrawablePtr pDraw,
	GCPtr pGC,
	int mode,
	int npt,
	xPoint *ppt)
{
    if (npt > 0) {
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	int x, y;

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	M32_CLEAR_QUEUE(8);

	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);
	outw(M32_LINEDRAW_OPT,	M32_LD_DOT);

	outw(M32_CUR_X,		x = ppt->x + pDraw->x);
	outw(M32_CUR_Y,		y = ppt->y + pDraw->y);
	outw(M32_BRES_COUNT,	1);

	--npt; ++ppt;

	if (mode == CoordModePrevious) {

	    while ((npt -= 4) >= 0) {
		M32_CLEAR_QUEUE(12);

		outw(M32_CUR_X,		x += ppt[0].x);
		outw(M32_CUR_Y,		y += ppt[0].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		x += ppt[1].x);
		outw(M32_CUR_Y,		y += ppt[1].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		x += ppt[2].x);
		outw(M32_CUR_Y,		y += ppt[2].y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		x += ppt[3].x);
		outw(M32_CUR_Y,		y += ppt[3].y);
		outw(M32_BRES_COUNT,	1);

		ppt += 4;
	    }

	    if (npt += 4) {
		M32_CLEAR_QUEUE(9);

		outw(M32_CUR_X,		x += ppt[0].x);
		outw(M32_CUR_Y,		y += ppt[0].y);
		outw(M32_BRES_COUNT,	1);

		if (--npt) {
		    outw(M32_CUR_X,		x += ppt[1].x);
		    outw(M32_CUR_Y,		y += ppt[1].y);
		    outw(M32_BRES_COUNT,	1);

		    if (--npt) {
			outw(M32_CUR_X,		x + ppt[2].x);
			outw(M32_CUR_Y,		y + ppt[2].y);
			outw(M32_BRES_COUNT,	1);
		    }
		}
	    }

	} else {  /* mode == CoordModeOrigin */

	    while ((npt -= 4) >= 0) {
		M32_CLEAR_QUEUE(12);

		outw(M32_CUR_X,		ppt[0].x + pDraw->x);
		outw(M32_CUR_Y,		ppt[0].y + pDraw->y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[1].x + pDraw->x);
		outw(M32_CUR_Y,		ppt[1].y + pDraw->y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[2].x + pDraw->x);
		outw(M32_CUR_Y,		ppt[2].y + pDraw->y);
		outw(M32_BRES_COUNT,	1);

		outw(M32_CUR_X,		ppt[3].x + pDraw->x);
		outw(M32_CUR_Y,		ppt[3].y + pDraw->y);
		outw(M32_BRES_COUNT,	1);

		ppt += 4;
	    }

	    if (npt += 4) {
		M32_CLEAR_QUEUE(9);

		outw(M32_CUR_X,		ppt[0].x + pDraw->x);
		outw(M32_CUR_Y,		ppt[0].y + pDraw->y);
		outw(M32_BRES_COUNT,	1);

		if (--npt) {
		    outw(M32_CUR_X,		ppt[1].x + pDraw->x);
		    outw(M32_CUR_Y,		ppt[1].y + pDraw->y);
		    outw(M32_BRES_COUNT,	1);

		    if (--npt) {
			outw(M32_CUR_X,		ppt[2].x + pDraw->x);
			outw(M32_CUR_Y,		ppt[2].y + pDraw->y);
			outw(M32_BRES_COUNT,	1);
		    }
		}
	    }

	}

	m32SetClip(NULL, 0, pDraw);
    }
}


/*
 * m32TileRectsN() - Draw rectangles using a tile.
 *	pbox - array of destination rectangles to draw into.
 *	nbox - the number of rectangles.
 *	tile - pointer to the tile pixels.
 *	stride - the number of bytes from the start of one scanline
 *		 to the next within tile.
 *	w - the width  of the tile in pixels.
 *	h - the height of the tile in pixels.
 *	patOrg - the origin of the tile relative to the screen.
 *	alu - the raster op to use when drawing.
 *	planemask - the window planes to be affected.
 *	pDraw - the window to draw on.
 */

void
m32TileRects8(
	BoxPtr pbox,
	unsigned int nbox,
	void *void_tile,					/* S002 */
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	m32OSInfoPtr pOS;
	unsigned char *tile = (unsigned char *)void_tile;	/* S002 */

	/* use the pattern regs ? */
	if ((tilew <= M32_MAX_PATTERN) && (tileh <= M32_MAX_PATTERN)) {
		BoxPtr pb;
		int nb;

		/* check for a rect wider than the tile */
		for (nb = nbox, pb = pbox; --nb >= 0; ++pb)
			if ((pb->x2 - pb->x1) > tilew)
				break;

		if (nb >= 0) {
			m32HWTileRects(pbox, nbox, tile, stride,
					tilew, tileh, patOrg,
					(tilew + 1) / 2, alu, planemask,
					pDraw);
			return;
		}
	}

	/* use the off-screen scratch area ? */
	pOS = M32_OS_INFO(pDraw->pScreen);
	if (!rop_needs_dst[alu] &&
	    ((tilew * 2) <= pOS->width) &&
	    ((tileh * 2) <= pOS->height)) {
		m32OSTileRects(pbox, nbox, tile, stride, tilew, tileh,
				patOrg, alu, planemask, pDraw);
		return;
	}

	/* let gen do it */
	genTileRects(pbox, nbox, tile, stride, tilew, tileh,
			patOrg, alu, planemask, pDraw);
}

void
m32TileRects16(
	BoxPtr pbox,
	unsigned int nbox,
	void *void_tile,					/* S002 */
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	m32OSInfoPtr pOS;
	unsigned char *tile = (unsigned char *)void_tile;	/* S002 */

	/* use the pattern regs ? */
	if ((tilew <= M32_MAX_PATTERN/2) && (tileh <= M32_MAX_PATTERN/2)) {
		BoxPtr pb;
		int nb;

		/* check for a rect wider than the tile */
		for (nb = nbox, pb = pbox; --nb >= 0; ++pb)
			if ((pb->x2 - pb->x1) > tilew)
				break;

		if (nb >= 0) {
			m32HWTileRects(pbox, nbox, tile, stride,
					tilew, tileh, patOrg,
					tilew, alu, planemask,
					pDraw);
			return;
		}
	}

	/* use the off-screen scratch area ? */
	pOS = M32_OS_INFO(pDraw->pScreen);
	if (!rop_needs_dst[alu] &&
	    ((tilew * 2) <= pOS->width) &&
	    ((tileh * 2) <= pOS->height)) {
		m32OSTileRects(pbox, nbox, tile, stride, tilew, tileh,
				patOrg, alu, planemask, pDraw);
		return;
	}

	/* let gen do it */
	genTileRects(pbox, nbox, tile, stride, tilew, tileh,
			patOrg, alu, planemask, pDraw);
}

m32HWTileRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	int words,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	DDXPointPtr poffset, po;
	BoxPtr pb;
	int nb, row;

	poffset = (DDXPointPtr)ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec));

	/* calculate tile offsets for each box */
	for (nb = nbox, pb = pbox, po = poffset; --nb >= 0; ++pb, ++po) {
		if ((po->x = (pb->x1 - patOrg->x) % (int)tilew) < 0)
			po->x += tilew;
		if ((po->y = (pb->y1 - patOrg->y) % (int)tileh) < 0)
			po->y += tileh;
	}

	/* setup some invariants */
	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_FILLPATT);
	outw(M32_PATT_LENGTH,	tilew - 1);
	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);

	/* for each tile row */
	for (row = 0; row < tileh; ++row, tile += stride) {
	    unsigned short *pw = (unsigned short *)tile;
	    int n = words;

	    /* load the tile row into the pattern regs */
	    if (n < 16) {
		M32_CLEAR_QUEUE(n + 1);
		outw(M32_PATT_DATA_INDEX, 0);
		while (--n >= 0)
		    outw(M32_PATT_DATA, *pw++);
	    } else {
		M32_CLEAR_QUEUE(n);
		outw(M32_PATT_DATA_INDEX, 0);
		while (--n > 0)
		    outw(M32_PATT_DATA,	*pw++);
		M32_CLEAR_QUEUE(1);
		outw(M32_PATT_DATA,	*pw);
	    }

	    /* for each rect */
	    for (nb = nbox, pb = pbox, po = poffset; --nb >= 0; ++pb, ++po) {
		int w = pb->x2 - pb->x1;
		int y;

		/* find 1st row in rect matching this tile row */
		if ((y = pb->y1 + row - po->y) < pb->y1)
		    y += tileh;

		/* paint all rows in rect matching this tile row */
		while (y < pb->y2) {
		    M32_CLEAR_QUEUE(4);
		    outw(M32_PATT_INDEX,	po->x);
		    outw(M32_CUR_X,		pb->x1);
		    outw(M32_CUR_Y,		y);
		    outw(M32_BRES_COUNT,	w);

		    y += tileh;
		}
	    }

	}

	DEALLOCATE_LOCAL(poffset);
}

m32OSTileRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	m32OSInfoPtr pOS = M32_OS_INFO(pDraw->pScreen);
	BoxRec osbox;
	void (*DrawImage)(struct _Box *, void *, unsigned int,	/* S002 */
			unsigned char, unsigned long,
			struct _Drawable *);
	DrawImage = NFB_WINDOW_PRIV(pDraw)->ops->DrawImage;	/* S002 */

	/* put a copy of the tile into off-screen */

	osbox.x2 = (osbox.x1 = pOS->addr.x) + tilew;
	osbox.y2 = (osbox.y1 = pOS->addr.y) + tileh;
	(*DrawImage)(&osbox, tile, stride, GXcopy, ~0, pDraw);

	/* quadruple the tile */

	M32_CLEAR_QUEUE(7);

	outw(M32_DP_CONFIG,	M32_DP_COPY);
	outw(M32_ALU_FG_FN,	m32RasterOp[GXcopy]);
	outw(M32_WRT_MASK,	~0);
	outw(M32_MAJ_AXIS_PCNT, tilew - 1);
	outw(M32_MIN_AXIS_PCNT, tileh - 1);
	outw(M32_CUR_X,		osbox.x1);
	outw(M32_CUR_Y,		osbox.y1);

	M32_CLEAR_QUEUE(8);

	outw(M32_DEST_X,	osbox.x2);
	outw(M32_DEST_Y,	osbox.y1);
	outw(M32_CMD,		M32_CMD_BLIT);

	outw(M32_MAJ_AXIS_PCNT, tilew * 2 - 1);
	outw(M32_DEST_X,	osbox.x1);
	outw(M32_CMD,		M32_CMD_BLIT);

	outw(M32_ALU_FG_FN,	m32RasterOp[alu]);
	outw(M32_WRT_MASK,	planemask);

	/* start tiling rects */

	do {
		int xoff, yoff;
		int boxw, boxh;
		int x;
		int w, h, wleft, hleft;

		if ((xoff = (pbox->x1 - patOrg->x) % (int)tilew) < 0)
			xoff += tilew;
		if ((yoff = (pbox->y1 - patOrg->y) % (int)tileh) < 0)
			yoff += tileh;

		boxw = pbox->x2 - pbox->x1;
		if ((w = tilew) > boxw)
			w = boxw;
		wleft = boxw - w;

		boxh = pbox->y2 - pbox->y1;
		if ((h = tileh) > boxh)
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
