/*
 *	@(#) effGC.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History
 *
 * S012, 18-Dec-92, davidw@sco.com
 *	Make effValidateWindowGC() declaration ANSIish.
 * S011, 03-Sep-92, hiramc@sco.COM
 *	Remove the include cfb.h - not needed
 * S010, 21-Sep-91, mikep@sco.com
 *	change genHelpValidateGC() to nfbHelpValidateGC()
 * S009, 18-Sep-91, staceyc
 * 	removed bucks cached gcops code, now call original gen code
 * S008, 17-Sep-91, staceyc
 * 	new push pixels
 * S007, 16-Aug-91, staceyc
 * 	bucks changes for zero width poly seg
 * S006, 15-Aug-91, staceyc
 * 	added eff glyph drawing routines
 * S005, 13-Aug-91, staceyc
 * 	merge in buck's cached gcops stuff
 * S004, 06-Aug-91, mikep@sco.com
 *	replace entire file with new ValidateWindowGC().
 * S003, 22-Jul-91, staceyc
 * 	fix usage of nfb arc code - fix dashed line stuff
 * S002, 25-Jun-91, staceyc
 * 	use nfb arcs where possible
 * S001, 24-Jun-91, staceyc
 * 	cleanup of parameters - moved nfb gc ops dec to effData.c
 * S000, 21-Jun-91, staceyc
 * 	add solid 0 line ops
 */

#include "X.h"
#include "Xproto.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "mi.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbProcs.h"
#include "genDefs.h"
#include "genProcs.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern nfbGCOps effSolidPrivOps, effTiledPrivOps,
		effStippledPrivOps, effOpStippledPrivOps;

/*
 * effValidateWindowGC() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * effHelpValidateGC() will do most of the work.
 */
void
effValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw)
{
	nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);

	/* set the private ops based on fill style */
	if (changes & GCFillStyle)
		switch (pGC->fillStyle) {
		    case FillSolid:
			pPriv->ops = &effSolidPrivOps;
			break;
		    case FillTiled:
			pPriv->ops = &effTiledPrivOps;
			break;
		    case FillStippled:
			pPriv->ops = &effStippledPrivOps;
			break;
		    case FillOpaqueStippled:
			if (pGC->fgPixel == pGC->bgPixel)
			    pPriv->ops = &effSolidPrivOps;
			else
			    pPriv->ops = &effOpStippledPrivOps;
			break;
		    default:
			FatalError("effValidateWindowGC: illegal fillStyle\n");
		}

	nfbHelpValidateGC(pGC, changes, pDraw);
}

