/*
 *	@(#) ifbGC.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * ifbGC.c
 *
 * ifb ValidateWindowGC() routine
 */

#include "X.h"
#include "Xproto.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "regionstr.h"

#include "ddxScreen.h"

#include "nfbDefs.h"
#include "nfbGCStr.h"
#include "nfbWinStr.h"

#include "ifbProcs.h"

extern nfbGCOps ifbSolidPrivOps, ifbTiledPrivOps,
		ifbStippledPrivOps, ifbOpStippledPrivOps;

/*
 * ifbValidateWindowGC() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
ifbValidateWindowGC(pGC, changes, pDraw)
	GCPtr pGC;
	Mask changes;
	DrawablePtr pDraw;
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &ifbSolidPrivOps;
			break;
		    case FillTiled:
			pPriv->ops = &ifbTiledPrivOps;
			break;
		    case FillStippled:
			pPriv->ops = &ifbStippledPrivOps;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &ifbSolidPrivOps;
			else
			    pPriv->ops = &ifbOpStippledPrivOps;
			break;
		    default:
			FatalError("ifbValidateWindowGC: illegal fillStyle\n");
		}

	/* let gen do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}
