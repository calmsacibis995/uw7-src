/*
 *  @(#) wd33GC.c 11.1 97/10/22
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
 *     wd33GC.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
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

#include "wd33ScrStr.h"
#include "wd33Procs.h"


extern nfbGCOps wd33SolidPrivOps, wd33TiledPrivOps,
		wd33StippledPrivOps, wd33OpStippledPrivOps;

/*
 * wd33ValidateWindowGC() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wd33ValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wd33SolidPrivOps;
			break;
		    case FillTiled:
			pPriv->ops = &wd33TiledPrivOps;
			break;
		    case FillStippled:
			pPriv->ops = &wd33StippledPrivOps;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wd33SolidPrivOps;
			else
			    pPriv->ops = &wd33OpStippledPrivOps;
			break;
		    default:
			FatalError("wd33ValidateWindowGC: illegal fillStyle\n");
		}

	/*  invalidate any loaded tile or stipple */
	if (changes & (GCTile | GCStipple))
		WD_SCREEN_PRIV(pDraw->pScreen)->tileSerial = 0;

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}


#ifdef NOT_YET

extern nfbGCOps wd33SolidPrivOps16, wd33TiledPrivOps16,
		wd33StippledPrivOps16, wd33OpStippledPrivOps16;

/*
 * wd33ValidateWindowGC16() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
wd33ValidateWindowGC16(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &wd33SolidPrivOps16;
			break;
		    case FillTiled:
			pPriv->ops = &wd33TiledPrivOps16;
			break;
		    case FillStippled:
			pPriv->ops = &wd33StippledPrivOps16;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &wd33SolidPrivOps16;
			else
			    pPriv->ops = &wd33OpStippledPrivOps16;
			break;
		    default:
			FatalError("wd33ValidateWindowGC16: illegal fillStyle\n");
		}

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}

#endif /* NOT_YET */
