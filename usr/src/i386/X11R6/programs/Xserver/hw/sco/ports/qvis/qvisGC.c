
/**
 * @(#) qvisGC.c 11.1 97/10/22
 *
 * Copyright (C) 1991 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */

/**
 * qvisGC.c
 *
 * Template for machine dependent ValidateWindowGC() routine
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Fri Dec 18 13:00:35 PST 1992    davidw@sco.com
 *	- Make qvisValidateWindowGC() declaration ANSIish.

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 *
 */

#include "xyz.h"
#include "X.h"
#include "Xproto.h"

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "qvisProcs.h"
#include "qvisMacros.h"

extern nfbGCOps qvisTiledPrivOps, qvisStippledPrivOps, qvisOpStippledPrivOps;

/*
 * qvisValidateWindowGC() - set GC ops and privates based on GC values.
 * 
 * Just set the nfb GC private ops here; nfbHelpValidateGC() will do most of the
 * work.
 */
void
qvisValidateWindowGC(
    GCPtr           pGC,
    Mask            changes,
    DrawablePtr     pDraw)
{
    nfbGCPrivPtr    pPriv = NFB_GC_PRIV(pGC);
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);

    XYZ("qvisValidateWindowGC-entered");
    /* set the private ops based on fill style */
    if (changes & GCFillStyle)
	switch (pGC->fillStyle) {
	case FillSolid:
	    XYZ("qvisValidateWindowGC-UseSolidPrivOps");
	    pPriv->ops = qvisPriv->qvisSolidPrivOpsPtr;
	    break;
	case FillTiled:
	    XYZ("qvisValidateWindowGC-UseTiledPrivOps");
	    pPriv->ops = &qvisTiledPrivOps;
	    break;
	case FillStippled:
	    XYZ("qvisValidateWindowGC-UseStippledPrivOps");
	    pPriv->ops = &qvisStippledPrivOps;
	    break;
	case FillOpaqueStippled:
	    if (pGC->fgPixel == pGC->bgPixel) {
		XYZ("qvisValidateWindowGC-UseOpaqueStippledPrivOps-ButReallySolid");
		pPriv->ops = qvisPriv->qvisSolidPrivOpsPtr;
	    } else {
		XYZ("qvisValidateWindowGC-UseOpaqueStippledPrivOps");
		pPriv->ops = &qvisOpStippledPrivOps;
	    }
	    break;
	default:
	    FatalError("qvisValidateWindowGC: illegal fillStyle\n");
	}

    /* let nfb do the rest */
    nfbHelpValidateGC(pGC, changes, pDraw);
}
