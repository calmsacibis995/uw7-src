/*
 *  @(#) wdGC.c 11.1 97/10/22
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
/*
 *     wdGC.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Wed 16-Dec-1992 buckm@sco.com
 *		Stop mucking with the gc ops after HelpValidate;
 *		that is a no-no.
 *      S002    Fri Dec 18 13:10:16 PST 1992    davidw@sco.com
 *      	Make wdValidateWindowGC declaration ANSIish.
 *	S003	Sun Dec 20 18:12:14 PST 1992	buckm@sco.com
 *		Invalidate loaded tiles and stipples on changes.
 *	S004	Tue Feb 09 20:28:35 PST 1993	buckm@sco.com
 *		Delete stuff ifdef'd out in S001.
 *		Add 15,16,24 bit support.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "grafinfo.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "genDefs.h"
#include "genProcs.h"

#include "wdScrStr.h"
#include "wdProcs.h"


extern nfbGCOps wdSolidPrivOps, wdTiledPrivOps,
		wdStippledPrivOps, wdOpStippledPrivOps;

/*
 * wdValidateWindowGC() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wdValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wdSolidPrivOps;
			break;
		    case FillTiled:
			pPriv->ops = &wdTiledPrivOps;
			break;
		    case FillStippled:
			pPriv->ops = &wdStippledPrivOps;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wdSolidPrivOps;
			else
			    pPriv->ops = &wdOpStippledPrivOps;
			break;
		    default:
			FatalError("wdValidateWindowGC: illegal fillStyle\n");
		}

	/* S003 - invalidate any loaded tile or stipple */
	if (changes & (GCTile | GCStipple))
		WD_SCREEN_PRIV(pDraw->pScreen)->tileSerial = 0;

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}


extern nfbGCOps wdSolidPrivOps15, wdTiledPrivOps15,
		wdStippledPrivOps15, wdOpStippledPrivOps15;

/*
 * wdValidateWindowGC15() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wdValidateWindowGC15(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wdSolidPrivOps15;
			break;
		    case FillTiled:
			pPriv->ops = &wdTiledPrivOps15;
			break;
		    case FillStippled:
			pPriv->ops = &wdStippledPrivOps15;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wdSolidPrivOps15;
			else
			    pPriv->ops = &wdOpStippledPrivOps15;
			break;
		    default:
			FatalError("wdValidateWindowGC15: illegal fillStyle\n");
		}

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}


extern nfbGCOps wdSolidPrivOps16, wdTiledPrivOps16,
		wdStippledPrivOps16, wdOpStippledPrivOps16;

/*
 * wdValidateWindowGC16() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wdValidateWindowGC16(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wdSolidPrivOps16;
			break;
		    case FillTiled:
			pPriv->ops = &wdTiledPrivOps16;
			break;
		    case FillStippled:
			pPriv->ops = &wdStippledPrivOps16;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wdSolidPrivOps16;
			else
			    pPriv->ops = &wdOpStippledPrivOps16;
			break;
		    default:
			FatalError("wdValidateWindowGC16: illegal fillStyle\n");
		}

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}


extern nfbGCOps wdSolidPrivOps24, wdTiledPrivOps24,
		wdStippledPrivOps24, wdOpStippledPrivOps24;

/*
 * wdValidateWindowGC24() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wdValidateWindowGC24(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wdSolidPrivOps24;
			break;
		    case FillTiled:
			pPriv->ops = &wdTiledPrivOps24;
			break;
		    case FillStippled:
			pPriv->ops = &wdStippledPrivOps24;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wdSolidPrivOps24;
			else
			    pPriv->ops = &wdOpStippledPrivOps24;
			break;
		    default:
			FatalError("wdValidateWindowGC24: illegal fillStyle\n");
		}

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}
