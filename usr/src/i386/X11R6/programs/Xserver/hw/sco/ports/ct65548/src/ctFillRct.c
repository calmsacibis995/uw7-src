/*
 *	@(#)ctFillRct.c	11.1	10/22/97	12:33:51
 *	@(#) ctFillRct.c 58.1 96/10/09 
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
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctFillRct.c 58.1 96/10/09 "

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

#include "ctDefs.h"
#include "ctMacros.h"
#include "ctOnboard.h"

void
CT(SolidFillRects)(pGC, pDraw, pbox, nbox)
GCPtr pGC;
DrawablePtr pDraw;
BoxPtr pbox;
unsigned int nbox;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	int bltStride = ctPriv->bltStride;
	unsigned long fg;

#ifdef DEBUG_PRINT
	ErrorF("SolidFillRects()\n");
#endif

	if (nfbGCPriv->rRop.alu == GXnoop)
		return;

	if ((nfbGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		genSolidFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, using fg_color.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[nfbGCPriv->rRop.alu] |
			CT_XINCBIT | CT_YINCBIT | CT_FGCOLORBIT);
	CT_SET_BLTOFFSET(bltStride, bltStride);
	CT_SET_BLTFGCOLOR(nfbGCPriv->rRop.fg);

	while (nbox--) {
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1));
		CT_START_BITBLT(pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
		pbox++;
	}
}

void
CT(TiledFillRects)(pGC, pDraw, pbox, nbox)
GCPtr pGC;
DrawablePtr pDraw;
BoxPtr pbox;
unsigned int nbox;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);

#ifdef DEBUG_PRINT
	ErrorF("TileFillRects()\n");
#endif

	if ((nfbGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		/*
		 * NOTE: we can't optimally tile with a planemask since both
		 * use the 8x8 pattern register (YET?).
		 */
		genTileRects(pbox, nbox, pGC->tile.pixmap->devPrivate.ptr,
				pGC->tile.pixmap->devKind,
				pGC->tile.pixmap->drawable.width,
				pGC->tile.pixmap->drawable.height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
		return;
	}

	switch (nfbGCPriv->rRop.alu) {
	case GXnoop:
		return;
	case GXclear:
	case GXset:
	case GXinvert:
		/*
		 * The specified ALU ignores both tile and foreground values.
		 */
		CT(DrawSolidRects)(pbox, nbox, 0L, nfbGCPriv->rRop.alu,
					nfbGCPriv->rRop.planemask, pDraw);
		return;
	default:
		break;
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the tile at GC validation time; so
		 * try again.
		 */
		CT(CacheGCPixmap)(pGC, pGC->tile.pixmap, 0L, 0L);
	}

	if (ctGCPriv->chunk == (void *)0) {
		/*
		 * We couldn't cache the tile; so give up and call gen.
		 */
		genTileRects(pbox, nbox, pGC->tile.pixmap->devPrivate.ptr,
				pGC->tile.pixmap->devKind,
				pGC->tile.pixmap->drawable.width,
				pGC->tile.pixmap->drawable.height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
		return;
	}

	CT(ImageCacheCopyRects)(pbox, nbox,
				ctGCPriv->chunk,
				ctGCPriv->width,
				ctGCPriv->height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
}

void
CT(StippledFillRects)(pGC, pDraw, pbox, nbox)
GCPtr pGC;
DrawablePtr pDraw;
BoxPtr pbox;
unsigned int nbox;
{
#ifdef DEBUG_PRINT
	ErrorF("StippledFillRects()\n");
#endif
}

void
CT(OpStippledFillRects)(pGC, pDraw, pbox, nbox)
GCPtr pGC;
DrawablePtr pDraw;
BoxPtr pbox;
unsigned int nbox;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);

#ifdef DEBUG_PRINT
	ErrorF("OpStippledFillRects()\n");
#endif

	if ((nfbGCPriv->rRop.planemask & ctPriv->allPlanes) !=
	    ctPriv->allPlanes) {
		/*
		 * NOTE: we can't optimally stipple with a planemask since both
		 * use the 8x8 pattern register.
		 */
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
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
		CT(DrawSolidRects)(pbox, nbox, 0L, nfbGCPriv->rRop.alu,
					nfbGCPriv->rRop.planemask, pDraw);
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
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	CT(ImageCacheCopyRects)(pbox, nbox,
				ctGCPriv->chunk,
				ctGCPriv->width,
				ctGCPriv->height,
				&(nfbGCPriv->screenPatOrg),
				nfbGCPriv->rRop.alu,
				nfbGCPriv->rRop.planemask,
				pDraw);
}
