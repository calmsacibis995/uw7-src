/*
 * @(#) i128GCFuncs.c 11.1 97/10/22
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
 * i128GCFuncs.c
 *
 * GCFunc wrappers and GC creation routines.
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
#include "i128GCFuncs.h"

#ifdef I128_FAST_GC_OPS
static void i128ValidateGC(GCPtr, Mask, DrawablePtr);
static void i128ChangeGC(GCPtr, Mask);
static void i128CopyGC(GCPtr, Mask, GCPtr);
static void i128DestroyGC(GCPtr);
static void i128ChangeClip(GCPtr, int, char *, int);
static void i128DestroyClip(GCPtr);
static void i128CopyClip(GCPtr, GCPtr);

/* GC Funcs */
GCFuncs i128GCFuncs = {
    i128ValidateGC,
    i128ChangeGC,
    i128CopyGC,
    i128DestroyGC,
    i128ChangeClip,
    i128DestroyClip,
    i128CopyClip,
} ;

extern GCOps i128GCNoOps;
extern GCOps i128GCSolidOps;
extern GCOps i128GCStippledOps;
extern GCOps i128GCOpStippledOps;
extern GCOps i128GCTiledOps;

Bool
i128CreateGC(pGC)
    GCPtr pGC;
{
    Bool ret;
    ScreenPtr pScreen = pGC->pScreen;
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);

    pScreen->CreateGC = i128Priv->CreateGC;
 
    if (ret = (*pScreen->CreateGC)(pGC))
    {
        i128GCPrivatePtr pGCPriv = I128_GC_PRIV(pGC);
        I128_WRAP_GC(pGC, &i128GCFuncs, &i128GCNoOps);
    }
     
    pScreen->CreateGC = i128CreateGC;
    return ret;
}


static void
i128ValidateGC(pGC, changes, pDraw)
    GCPtr pGC;
    Mask changes;
    DrawablePtr pDraw;
{
    nfbGCPrivPtr pPriv = NFB_GC_PRIV(pGC);
    GCOps *ops;
    extern Bool enforceProtocol;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);

    /* set the private ops based on fill style */
#if 1
    if (pDraw->type == DRAWABLE_PIXMAP )
        ops = &i128GCNoOps;
    else if (changes & GCFillStyle)
        switch (pGC->fillStyle)
        {
          case FillSolid:
            ops = &i128GCSolidOps;
            break;

          case FillTiled:
            ops = &i128GCTiledOps;
            break;

          case FillStippled:
            ops = &i128GCStippledOps;
            break;

          case FillOpaqueStippled:
            if (pGC->fgPixel == pGC->bgPixel)
                ops = &i128GCSolidOps;
            else
                ops = &i128GCOpStippledOps;
            break;

          default:
            FatalError("i128ValidateGC: illegal fillStyle\n");
            break;
        }
#else
    ops = &i128GCNoOps;
#endif
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);

}


static void
i128ChangeGC(pGC, changes)
    GCPtr pGC;
    Mask changes;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->ChangeGC)(pGC, changes);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
i128CopyGC(pGC, changes, pGCDst)
    GCPtr pGC;
    Mask changes;
    GCPtr pGCDst;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGCDst, ops);
    I128_UNWRAP_GC(pGC, ops);
    (*pGCDst->funcs->CopyGC)(pGC, changes, pGCDst);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
    I128_WRAP_GC(pGCDst, &i128GCFuncs, ops);
}


static void
i128DestroyGC(pGC)
    GCPtr pGC;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->DestroyGC)(pGC);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops); 
}


static void
i128ChangeClip(pGC, type, pValue, nrects)
    GCPtr pGC;
    int type;
    char *pValue;
    int nrects;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->ChangeClip)(pGC, type, pValue, nrects);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
i128DestroyClip(pGC)
    GCPtr pGC;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->DestroyClip)(pGC);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}


static void
i128CopyClip(pGC, pGCsrc)
    GCPtr pGC;
    GCPtr pGCsrc;
{
    GCOps *ops;

    I128_UNWRAP_GC(pGC, ops);
    (*pGC->funcs->CopyClip)(pGC, pGCsrc);
    I128_WRAP_GC(pGC, &i128GCFuncs, ops);
}

#endif
