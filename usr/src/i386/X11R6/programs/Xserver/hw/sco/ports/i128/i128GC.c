/*
 * @(#) i128GC.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
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
 * i128GC.c
 *
 * Template for machine dependent ValidateWindowGC() routine
 */

#include "X.h"
#include "Xproto.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"

#include "ddxScreen.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"

#include "i128Procs.h"

extern nfbGCOps i128SolidPrivOps, i128TiledPrivOps,
i128StippledPrivOps, i128OpStippledPrivOps;
extern nfbGCMiscOps i128SolidMiscOps;

/*
 * i128ValidateWindowGC() - set GC ops and privates based on GC values.
 *
 * Just set the nfb GC private ops here;
 * nfbHelpValidateGC() will do most of the work.
 */
void
i128ValidateWindowGC(
                     GCPtr pGC,
                     Mask changes,
                     DrawablePtr pDraw)
{
    nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);
    extern Bool enforceProtocol;

    /* set the private ops based on fill style */
    if (changes & GCFillStyle)
        switch (pGC->fillStyle)
        {
          case FillSolid:
            pPriv->ops = &i128SolidPrivOps;
            if (!enforceProtocol)
                pPriv->miscops = &i128SolidMiscOps;
            break;
          case FillTiled:
            pPriv->ops = &i128TiledPrivOps;
            pPriv->miscops = 0;
            break;
          case FillStippled:
            pPriv->ops = &i128StippledPrivOps;
            pPriv->miscops = 0;
            break;
          case FillOpaqueStippled:
            if (pGC->fgPixel == pGC->bgPixel)
            {
                pPriv->ops = &i128SolidPrivOps;
                if (!enforceProtocol)
                    pPriv->miscops = &i128SolidMiscOps;
            }
            else
            {
                pPriv->ops = &i128OpStippledPrivOps;
                if (!enforceProtocol)
                    pPriv->miscops = 0;
            }
            break;
          default:
            FatalError("i128ValidateWindowGC: illegal fillStyle\n");
        }

    /* let nfb do most of the work */
    nfbHelpValidateGC(pGC, changes, pDraw);

}

