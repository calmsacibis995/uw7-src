/*
 *	@(#)ctFillSp.c	11.1	10/22/97	12:34:44
 *	@(#) ctFillSp.c 61.2 97/02/26 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctFillSp.c 61.2 97/02/26 "

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "gen/genProcs.h"

#include "ctDefs.h"
#include "ctMacros.h"

extern void CT(SolidFS)();

void
CT(SolidFS)(pGC, pDraw, ppt, pwidth, npts)
GCPtr pGC;
DrawablePtr pDraw;
DDXPointPtr ppt;
unsigned int *pwidth;
unsigned npts;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	if (pGCPriv->rRop.alu == GXnoop)
		return;

	if ((pGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		genSolidFS(pGC, pDraw, ppt, pwidth, npts);
		return;
	}

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, using fg_color.
	 */
#if 0
	CT_SET_BLTCTRL(CT(PixelOps)[pGCPriv->rRop.alu] |
			CT_YINCBIT | CT_XINCBIT | CT_PATSOLIDBIT);
#else
	CT_SET_BLTCTRL(CT(PixelOps)[pGCPriv->rRop.alu] |
			CT_PATSOLIDBIT);
#endif
	CT_SET_BLTOFFSET(ctPriv->bltStride, ctPriv->bltStride);
	CT_SET_BLTBGCOLOR(pGCPriv->rRop.fg);

	while (npts--) {
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, ppt->x, ppt->y));
		CT_START_BITBLT(*pwidth, 1);
		++ppt;
		++pwidth;
	}
}

void
CT(TiledFS)(pGC, pDraw, ppt, pwidth, npts)
GCPtr pGC;
DrawablePtr pDraw;
DDXPointPtr ppt;
unsigned int *pwidth;
unsigned npts;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);

	if ((nfbGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		/*
		 * NOTE: we can't optimally stipple with a planemask since both
		 * use the 8x8 pattern register.
		 */
		genTiledFS(pGC, pDraw, ppt, pwidth, npts);
		return;
	}

	switch (nfbGCPriv->rRop.alu) {
	case GXnoop:
		return;
	case GXclear:
	case GXset:
	case GXinvert:
		/*
		 * The specified ALU ignores both stipple and foreground values.
		 */
		CT(SolidFS)(pGC, pDraw, ppt, pwidth, npts);
		return;
	default:
		break;
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the tile at GC validation time;
		 * so try again.
		 */
		CT(CacheGCPixmap)(pGC, pGC->tile.pixmap, 0L, 0L);
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the tile; so give up and call gen.
		 */
		genTiledFS(pGC, pDraw, ppt, pwidth, npts);
		return;
	}

	CT(ImageCacheFS)(ppt, pwidth, npts,
				ctGCPriv->chunk,
				ctGCPriv->width,
				ctGCPriv->height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
}

void
CT(StippledFS)(pGC, pDraw, ppt, pwidth, npts)
GCPtr pGC;
DrawablePtr pDraw;
DDXPointPtr ppt;
unsigned int *pwidth;
unsigned npts;
{
}

void
CT(OpStippledFS)(pGC, pDraw, ppt, pwidth, npts)
GCPtr pGC;
DrawablePtr pDraw;
DDXPointPtr ppt;
unsigned int *pwidth;
unsigned npts;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);

	if ((nfbGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		/*
		 * NOTE: we can't optimally stipple with a planemask since both
		 * use the 8x8 pattern register.
		 */
		genOpStippledFS(pGC, pDraw, ppt, pwidth, npts);
		return;
	}

	switch (nfbGCPriv->rRop.alu) {
	case GXnoop:
		return;
	case GXclear:
	case GXset:
	case GXinvert:
		/*
		 * The specified ALU ignores both stipple and foreground values.
		 */
		CT(SolidFS)(pGC, pDraw, ppt, pwidth, npts);
		return;
	default:
		break;
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the stipple at GC validation time;
		 * so try again.
		 */
		CT(CacheGCPixmap)(pGC, pGC->stipple,
				nfbGCPriv->rRop.fg, nfbGCPriv->rRop.bg);
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the stipple; so give up and call
		 * gen.
		 */
		genOpStippledFS(pGC, pDraw, ppt, pwidth, npts);
		return;
	}

	CT(ImageCacheFS)(ppt, pwidth, npts,
				ctGCPriv->chunk,
				ctGCPriv->width,
				ctGCPriv->height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
}
