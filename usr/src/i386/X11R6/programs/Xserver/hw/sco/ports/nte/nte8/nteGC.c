/*
 *	@(#) nteGC.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

extern nfbGCOps NTE(SolidPrivOps), NTE(TiledPrivOps),
		NTE(StippledPrivOps), NTE(OpStippledPrivOps);

/*
 * NTE(ValidateWindowGC)() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
NTE(ValidateWindowGC)(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle)
		{
		case FillSolid:
			pPriv->ops = &NTE(SolidPrivOps);
			break;
		case FillTiled:
			pPriv->ops = &NTE(TiledPrivOps);
			break;
		case FillStippled:
			pPriv->ops = &NTE(StippledPrivOps);
			break;
		case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
				pPriv->ops = &NTE(SolidPrivOps);
			else
				pPriv->ops = &NTE(OpStippledPrivOps);
			break;
		default:
			FatalError("NTE(ValidateWindowGC): illegal fillStyle\n");
		}

	/* let nfb do the rest */
	nfbHelpValidateGC(pGC, changes, pDraw);
}
