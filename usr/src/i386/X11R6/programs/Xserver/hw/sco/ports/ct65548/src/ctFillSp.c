/*
 *	@(#)ctFillSp.c	11.1	10/22/97	12:33:52
 *	@(#) ctFillSp.c 58.1 96/10/09 
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
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Thu Jul 14 15:11:14 PDT 1994    davidw@sco.com
 *      - Use pGCPriv->rRop.alu in test instead of uninitialized alu.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctFillSp.c 58.1 96/10/09 "

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
	unsigned long fg;

	if (pGCPriv->rRop.alu == GXnoop)		/* S000 */
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
	CT_SET_BLTCTRL(CT(PixelOps)[pGCPriv->rRop.alu] |
			CT_XINCBIT | CT_YINCBIT | CT_FGCOLORBIT);
	CT_SET_BLTOFFSET(ctPriv->bltStride, ctPriv->bltStride);
	CT_SET_BLTFGCOLOR(pGCPriv->rRop.fg);

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
