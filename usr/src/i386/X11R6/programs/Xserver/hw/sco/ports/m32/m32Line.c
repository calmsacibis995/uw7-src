/*
 * @(#) m32Line.c 11.1 97/10/22
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
 * S001, 26-Aug-93, buckm
 *	Fix loop control error in m32SolidZeroPtToPt().
 * S002, 31-Aug-93, buckm
 *	Get rid of m32SolidZeroPtToPt(), m32SolidZeroSegs().
 *	Add m32LineSS(), m32SegmentSS(), m32RectangleSS().
 *	Add m32LineSD(), m32SegmentSD(), m32RectangleSD().
 */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"

#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"


void
m32SolidZeroSeg(pGC, pDraw, signdx, signdy, axis, x1, y1, e, e1, e2, len)
	GCPtr pGC;
	DrawablePtr pDraw;
	int signdx;
	int signdy;
	int axis;
	int x1;
	int y1;
	int e;
	int e1;
	int e2;
	int len;
{
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	M32_CLEAR_QUEUE(4);
	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);

	if (-4096 <= e  && e  <= 4095 &&
		0 <= e1 && e1 <= 4095 &&
	    -4096 <= e2 && e2 <= 0	) {
		
		/* Mach-32 can draw this bresenham line */

		int ldopt;

		ldopt = M32_LD_BRES | (axis << 6);
		if (signdx > 0)
			ldopt |= 0x20;
		if (signdy > 0)
			ldopt |= 0x80;

		M32_CLEAR_QUEUE(7);
		outw(M32_LINEDRAW_OPT,	ldopt);
		outw(M32_ERR_TERM,	e);
		outw(M32_AXSTP,		e1);
		outw(M32_DIASTP,	e2);
		outw(M32_CUR_X,		x1);
		outw(M32_CUR_Y,		y1);
		outw(M32_BRES_COUNT,	len);

	} else {

		/* we must compute this bresenham line */

		M32_CLEAR_QUEUE(4);

		outw(M32_LINEDRAW_OPT,	M32_LD_DOT);

		outw(M32_CUR_X,		x1);
		outw(M32_CUR_Y,		y1);
		outw(M32_BRES_COUNT,	1);

		while (--len > 0) {
			if (e > 0) {	/* make diagonal step */ 
				x1 += signdx;
				y1 += signdy;
				e  += e2;
			} else {	/* make linear step   */
				if (axis == X_AXIS) 
					x1 += signdx;
				else
					y1 += signdy;
				e += e1;
			}

			M32_CLEAR_QUEUE(3);
			outw(M32_CUR_X,		x1);
			outw(M32_CUR_Y,		y1);
			outw(M32_BRES_COUNT,	1);
		}
	}
}


/*
 * The following are GC ops for thin, fillsolid, solid and dashed lines
 * when there is one clip rectangle.
 */

/*
 * We would use the Mach-32 pre-clipping support in the routines below,
 * but the Mach-32 pre-clipping range is -32768 to 32767 (a signed short),
 * while adding the window origin to the line coordinates
 * can overflow this range.
 */

#define	DRAW_SEG						\
{	int dx, dy;						\
	if ((dy = y2 - y1) == 0) {				\
		START_SEG(4);					\
		outw(M32_CUR_X,	x1);				\
		outw(M32_CUR_Y,	y1);				\
		if ((dx = x2 - x1) >= 0) {	/*   0 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0x00);	\
			outw(M32_BRES_COUNT,	dx);		\
		} else {			/* 180 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0x80);	\
			outw(M32_BRES_COUNT,	-dx);		\
		}						\
	} else if ((dx = x2 - x1) == 0) {			\
		START_SEG(4);					\
		outw(M32_CUR_X,	x1);				\
		outw(M32_CUR_Y,	y1);				\
		if (dy > 0) {			/*  90 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0xC0);	\
			outw(M32_BRES_COUNT,	dy);		\
		} else {			/* 270 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0x40);	\
			outw(M32_BRES_COUNT,	-dy);		\
		}						\
	} else if (dy == dx) {					\
		START_SEG(4);					\
		outw(M32_CUR_X,	x1);				\
		outw(M32_CUR_Y,	y1);				\
		if (dy > 0) {			/* 315 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0xE0);	\
			outw(M32_BRES_COUNT,	dy);		\
		} else {			/* 135 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0x60);	\
			outw(M32_BRES_COUNT,	-dy);		\
		}						\
	} else if (dy == -dx) {					\
		START_SEG(4);					\
		outw(M32_CUR_X,	x1);				\
		outw(M32_CUR_Y,	y1);				\
		if (dy > 0) {			/* 225 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0xA0);	\
			outw(M32_BRES_COUNT,	dy);		\
		} else {			/* 045 deg */	\
			outw(M32_LINEDRAW_OPT,	ldopt | 0x20);	\
			outw(M32_BRES_COUNT,	dx);		\
		} \
	} else {				/* other deg */	\
		START_SEG(5);					\
		outw(M32_LINEDRAW_INDEX,	0);		\
		outw(M32_LINEDRAW,		x1);		\
		outw(M32_LINEDRAW,		y1);		\
		outw(M32_LINEDRAW,		x2);		\
		outw(M32_LINEDRAW,		y2);		\
	}							\
}


#define	START_SEG(n)	M32_CLEAR_QUEUE(n)

void
m32LineSS(pDraw, pGC, mode, npt, pptInit)
	DrawablePtr pDraw;
	GCPtr pGC;
	int mode;
	unsigned int npt;
	DDXPointPtr pptInit;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);
	DDXPointPtr ppt = pptInit;
	int ldopt;
	int x1, y1, x2, y2;

	if (npt <= 1)
		return;

	x2 = ppt->x + pDraw->x;
	y2 = ppt->y + pDraw->y;

	if (x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
	    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION ) {
		nfbLineSS(pDraw, pGC, mode, npt, pptInit);
		return;
	}

	ldopt = M32_LD_PTPTNL;

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);
	outw(M32_LINEDRAW_OPT,	ldopt);

	while (--npt > 0) {
		++ppt;

		x1 = x2;
		y1 = y2;
		if (mode == CoordModePrevious) {
			x2 += ppt->x;
			y2 += ppt->y;
		} else {
			x2 = ppt->x + pDraw->x;
			y2 = ppt->y + pDraw->y;
		}

		/*
		 * if we run into something the hw can't deal with,
		 * let nfb handle the rest.
		 */
		if (x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
		    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			nfbLineSS(pDraw, pGC, mode, ++npt, --ppt);
			m32SetClip(NULL, 0, pDraw);
			return;
		}

		DRAW_SEG;
	}

	if (pGC->capStyle != CapNotLast &&
	    (ppt->x != pptInit->x ||
	     ppt->y != pptInit->y ||
	     ppt == pptInit + 1)) {
		outw(M32_BRES_COUNT, 1);
	}

	m32SetClip(NULL, 0, pDraw);
}

void
m32SegmentSS(pDraw, pGC, nseg, pSeg)
	DrawablePtr pDraw;
	GCPtr pGC;
	unsigned int nseg;
	xSegment *pSeg;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);
	int ldopt;

	if (nseg == 0)
		return;

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	ldopt = (pGC->capStyle == CapNotLast) ? M32_LD_PTPTNL : M32_LD_PTPT;

	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);
	outw(M32_LINEDRAW_OPT,	ldopt);

	do {
		int x1 = pSeg->x1 + pDraw->x;
		int x2 = pSeg->x2 + pDraw->x;
		int y1 = pSeg->y1 + pDraw->y;
		int y2 = pSeg->y2 + pDraw->y;

		/*
		 * if we run into something the hw can't deal with,
		 * let nfb handle the rest.
		 */
		if (x1 < M32_MIN_DIMENSION || x1 >= M32_MAX_DIMENSION ||
		    x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
		    y1 < M32_MIN_DIMENSION || y1 >= M32_MAX_DIMENSION ||
		    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			m32SetClip(NULL, 0, pDraw);
			nfbSegmentSS(pDraw, pGC, nseg, pSeg);
			return;
		}

		DRAW_SEG;

		++pSeg;
	} while (--nseg > 0);

	m32SetClip(NULL, 0, pDraw);
}

void
m32RectangleSS(pDraw, pGC, nrects, pRects)
	DrawablePtr pDraw;
	GCPtr pGC;
	int nrects;
	xRectangle *pRects;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);

	if (nrects <= 0)
		return;

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	M32_CLEAR_QUEUE(4);
	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	outw(M32_WRT_MASK,	pGCPriv->rRop.planemask);

	do {
		int x1 = pRects->x + pDraw->x;
		int x2 = x1 + pRects->width;
		int y1 = pRects->y + pDraw->y;
		int y2 = y1 + pRects->height;

		/*
		 * if we run into something the hw can't deal with,
		 * let nfb handle the rest.  also, let nfb make any
		 * drawing errors for zero width or height rectangles.
		 */
		if (pRects->width == 0	    || pRects->height == 0     ||
		    x1 <  M32_MIN_DIMENSION || x1 >= M32_MAX_DIMENSION ||
		    y1 <  M32_MIN_DIMENSION || y1 >= M32_MAX_DIMENSION ||
		    x2 >= M32_MAX_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			m32SetClip(NULL, 0, pDraw);
			nfbPolyRectangle(pDraw, pGC, nrects, pRects);
			return;
		}

		M32_CLEAR_QUEUE(10);
		/* move to top left */
		outw(M32_CUR_X,		x1);
		outw(M32_CUR_Y,		y1);
		/* draw to top right */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x00);
		outw(M32_BRES_COUNT,	pRects->width);
		/* draw to bottom right */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0xC0);
		outw(M32_BRES_COUNT,	pRects->height);
		/* draw to bottom left */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x80);
		outw(M32_BRES_COUNT,	pRects->width);
		/* draw to top left */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x40);
		outw(M32_BRES_COUNT,	pRects->height);

		++pRects;
	} while (--nrects > 0);

	m32SetClip(NULL, 0, pDraw);
}


#define	GET_DASHES						\
{	int i = pGC->numInDashList;				\
	unsigned char *pd = &pGC->dash[i];			\
	dashlen = 0;						\
	dashpatt = 0;						\
	while ((i -= 2) >= 0) {					\
		int d0, d1;					\
		pd -= 2;					\
		d0 = pd[0];					\
		d1 = pd[1];					\
		if ((dashlen += d0 + d1) > M32_MAX_PATTERN)	\
			break;					\
		dashpatt <<= d0 + d1;				\
		dashpatt  |= (1 << d0) - 1;			\
	}							\
	if (dashlen <= M32_MAX_PATTERN) {			\
		if ((dashoff = pGC->dashOffset) >= dashlen)	\
			dashoff %= dashlen;			\
		pd = (unsigned char *)&dashpatt;		\
		pd[0] = MSBIT_SWAP(pd[0]);			\
		pd[1] = MSBIT_SWAP(pd[1]);			\
		if (dashlen >= 16) {				\
			pd[2] = MSBIT_SWAP(pd[2]);		\
			pd[3] = MSBIT_SWAP(pd[3]);		\
		}						\
	}							\
}


void
m32LineSD(pDraw, pGC, mode, npt, pptInit)
	DrawablePtr pDraw;
	GCPtr pGC;
	int mode;
	unsigned int npt;
	DDXPointPtr pptInit;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);
	DDXPointPtr ppt = pptInit;
	int ldopt;
	int x1, y1, x2, y2;
	int dashlen, dashpatt, dashoff;

	if (npt <= 1)
		return;

	GET_DASHES;

	x2 = ppt->x + pDraw->x;
	y2 = ppt->y + pDraw->y;

	if (dashlen > M32_MAX_PATTERN ||
	    x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
	    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION ) {
		nfbLineSD(pDraw, pGC, mode, npt, pptInit);
		return;
	}

	ldopt = M32_LD_DPTPTNL;

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	M32_CLEAR_QUEUE(12);
	outw(M32_DP_CONFIG,		M32_DP_EXPPATT);
	outw(M32_FRGD_COLOR,		pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,		m32RasterOp[pGCPriv->rRop.alu]);
	if (pGC->lineStyle == LineDoubleDash) {
		outw(M32_BKGD_COLOR,	pGCPriv->rRop.bg);
		outw(M32_ALU_BG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	} else {
		outw(M32_ALU_BG_FN,	m32RasterOp[GXnoop]);
	}
	outw(M32_WRT_MASK,		pGCPriv->rRop.planemask);
	outw(M32_LINEDRAW_OPT,		ldopt);
	outw(M32_PATT_LENGTH,		dashlen - 1);
	outw(M32_PATT_INDEX,		dashoff);
	outw(M32_PATT_DATA_INDEX,	0x10);
	outw(M32_PATT_DATA,		dashpatt);
	if (dashlen >= 16)
		outw(M32_PATT_DATA,	dashpatt >> 16);

	while (--npt > 0) {
		++ppt;

		x1 = x2;
		y1 = y2;
		if (mode == CoordModePrevious) {
			x2 += ppt->x;
			y2 += ppt->y;
		} else {
			x2 = ppt->x + pDraw->x;
			y2 = ppt->y + pDraw->y;
		}

		/*
		 * if we run into something the hw can't deal with,
		 * let nfb handle the rest.
		 */
		if (x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
		    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			m32SetClip(NULL, 0, pDraw);
			dashoff = pGC->dashOffset;
			pGC->dashOffset = inw(M32_PATT_INDEX); /* naughty !! */
			nfbLineSD(pDraw, pGC, mode, ++npt, --ppt);
			pGC->dashOffset = dashoff;
			return;
		}

		DRAW_SEG;
	}

	if (pGC->capStyle != CapNotLast &&
	    (ppt->x != pptInit->x ||
	     ppt->y != pptInit->y ||
	     ppt == pptInit + 1)) {
		outw(M32_BRES_COUNT, 1);
	}

	m32SetClip(NULL, 0, pDraw);
}

#undef	START_SEG
#define	START_SEG(n)	M32_CLEAR_QUEUE(n+1); outw(M32_PATT_INDEX, dashoff)

void
m32SegmentSD(pDraw, pGC, nseg, pSeg)
	DrawablePtr pDraw;
	GCPtr pGC;
	unsigned int nseg;
	xSegment *pSeg;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);
	int ldopt;
	int dashlen, dashpatt, dashoff;

	if (nseg == 0)
		return;

	GET_DASHES;

	if (dashlen > M32_MAX_PATTERN) {
		m32SetClip(NULL, 0, pDraw);
		nfbSegmentSD(pDraw, pGC, nseg, pSeg);
		return;
	}

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	ldopt = (pGC->capStyle == CapNotLast) ? M32_LD_DPTPTNL : M32_LD_DPTPT;

	M32_CLEAR_QUEUE(11);
	outw(M32_DP_CONFIG,		M32_DP_EXPPATT);
	outw(M32_FRGD_COLOR,		pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,		m32RasterOp[pGCPriv->rRop.alu]);
	if (pGC->lineStyle == LineDoubleDash) {
		outw(M32_BKGD_COLOR,	pGCPriv->rRop.bg);
		outw(M32_ALU_BG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	} else {
		outw(M32_ALU_BG_FN,	m32RasterOp[GXnoop]);
	}
	outw(M32_WRT_MASK,		pGCPriv->rRop.planemask);
	outw(M32_LINEDRAW_OPT,		ldopt);
	outw(M32_PATT_LENGTH,		dashlen - 1);
	outw(M32_PATT_DATA_INDEX,	0x10);
	outw(M32_PATT_DATA,		dashpatt);
	if (dashlen >= 16)
		outw(M32_PATT_DATA,	dashpatt >> 16);

	do {
		int x1 = pSeg->x1 + pDraw->x;
		int x2 = pSeg->x2 + pDraw->x;
		int y1 = pSeg->y1 + pDraw->y;
		int y2 = pSeg->y2 + pDraw->y;

		/*
		 * if we run into something the hw can't deal with,
		 * let nfb handle the rest.
		 */
		if (x1 < M32_MIN_DIMENSION || x1 >= M32_MAX_DIMENSION ||
		    x2 < M32_MIN_DIMENSION || x2 >= M32_MAX_DIMENSION ||
		    y1 < M32_MIN_DIMENSION || y1 >= M32_MAX_DIMENSION ||
		    y2 < M32_MIN_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			m32SetClip(NULL, 0, pDraw);
			nfbSegmentSD(pDraw, pGC, nseg, pSeg);
			return;
		}

		DRAW_SEG;

		++pSeg;
	} while (--nseg > 0);

	m32SetClip(NULL, 0, pDraw);
}

void
m32RectangleSD(pDraw, pGC, nrects, pRects)
	DrawablePtr pDraw;
	GCPtr pGC;
	int nrects;
	xRectangle *pRects;
{
        nfbGCPriv *pGCPriv = NFB_GC_PRIV(pGC);
	int dashlen, dashpatt, dashoff;

	if (nrects <= 0)
		return;

	GET_DASHES;

	if (dashlen > M32_MAX_PATTERN) {
		m32SetClip(NULL, 0, pDraw);
		miPolyRectangle(pDraw, pGC, nrects, pRects);
		return;
	}

	m32SetClip(REGION_RECTS(pGCPriv->pCompositeClip), 1, pDraw);

	M32_CLEAR_QUEUE(10);
	outw(M32_DP_CONFIG,		M32_DP_EXPPATT);
	outw(M32_FRGD_COLOR,		pGCPriv->rRop.fg);
	outw(M32_ALU_FG_FN,		m32RasterOp[pGCPriv->rRop.alu]);
	if (pGC->lineStyle == LineDoubleDash) {
		outw(M32_BKGD_COLOR,	pGCPriv->rRop.bg);
		outw(M32_ALU_BG_FN,	m32RasterOp[pGCPriv->rRop.alu]);
	} else {
		outw(M32_ALU_BG_FN,	m32RasterOp[GXnoop]);
	}
	outw(M32_WRT_MASK,		pGCPriv->rRop.planemask);
	outw(M32_PATT_LENGTH,		dashlen - 1);
	outw(M32_PATT_DATA_INDEX,	0x10);
	outw(M32_PATT_DATA,		dashpatt);
	if (dashlen >= 16)
		outw(M32_PATT_DATA,	dashpatt >> 16);

	do {
		int x1 = pRects->x + pDraw->x;
		int x2 = x1 + pRects->width;
		int y1 = pRects->y + pDraw->y;
		int y2 = y1 + pRects->height;

		/*
		 * if we run into something the hw can't deal with,
		 * let mi handle the rest.  also, let mi make any
		 * drawing errors for zero width or height rectangles.
		 */
		if (pRects->width == 0	    || pRects->height == 0     ||
		    x1 <  M32_MIN_DIMENSION || x1 >= M32_MAX_DIMENSION ||
		    y1 <  M32_MIN_DIMENSION || y1 >= M32_MAX_DIMENSION ||
		    x2 >= M32_MAX_DIMENSION || y2 >= M32_MAX_DIMENSION) {
			m32SetClip(NULL, 0, pDraw);
			miPolyRectangle(pDraw, pGC, nrects, pRects);
			return;
		}

		M32_CLEAR_QUEUE(11);
		/* move to top left */
		outw(M32_PATT_INDEX,	dashoff);
		outw(M32_CUR_X,		x1);
		outw(M32_CUR_Y,		y1);
		/* draw to top right */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x00);
		outw(M32_BRES_COUNT,	pRects->width);
		/* draw to bottom right */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0xC0);
		outw(M32_BRES_COUNT,	pRects->height);
		/* draw to bottom left */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x80);
		outw(M32_BRES_COUNT,	pRects->width);
		/* draw to top left */
		outw(M32_LINEDRAW_OPT,	M32_LD_DEGREE | 0x40);
		outw(M32_BRES_COUNT,	pRects->height);

		++pRects;
	} while (--nrects > 0);

	m32SetClip(NULL, 0, pDraw);
}
