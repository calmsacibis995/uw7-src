/*
 *	@(#)ctGC.c	11.1	10/22/97	12:10:39
 *	@(#) ctGC.c 12.1 95/05/09 
 *      ctGC.c 7.3 92/12/18 
 *
 * Copyright (C) 1994-1996 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 * ctGC.c
 *
 * Template for machine dependent ValidateWindowGC() routine
 */

#ident "@(#) $Id$"

#include "X.h"
#include "Xproto.h"

#include "pixmapstr.h"
#include "colormapst.h"
#include "gcstruct.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbProcs.h"

#include "ctDefs.h"
#include "ctProcs.h"
#include "ctOnboard.h"

extern void *CT(ImageCacheLoad)();
extern void CT(CacheGCPixmap)();

extern nfbGCOps CT(SolidPrivOps), CT(TiledPrivOps),
		CT(StippledPrivOps), CT(OpStippledPrivOps);

static void ctValidateGC();
static void ctChangeGC();
static void ctCopyGC();
static void ctDestroyGC();
static void ctChangeClip();
static void ctDestroyClip();
static void ctCopyClip();

/*
 * Wrap the NFB GC funcs layer.
 */
static GCFuncs ctFuncs = {
	ctValidateGC,
	ctChangeGC,
	ctCopyGC,
	ctDestroyGC,
	ctChangeClip,
	ctDestroyClip,
	ctCopyClip,
};

/*******************************************************************************

				Private Routines

*******************************************************************************/

#define CT_GC_WRAPPERS(pGC) (						\
	(GCFuncs *)(CT_GC_PRIV((pGC))->funcs)				\
)

#define CT_WRAP_GC(pGC) {						\
	CT_GC_PRIV((pGC))->funcs = (void *)(pGC)->funcs;		\
	(pGC)->funcs = &ctFuncs;					\
}

#define CT_UNWRAP_GC(pGC) {						\
	(pGC)->funcs = CT_GC_WRAPPERS((pGC));				\
}

#define CT_WRAP_SCREEN(pScreen) {					\
	CT_PRIVATE_DATA((pScreen))->CreateGC = (pScreen)->CreateGC;	\
	(pScreen)->CreateGC = CT(CreateGC);				\
}

#define CT_UNWRAP_SCREEN(pScreen) {					\
	(pScreen)->CreateGC = CT_PRIVATE_DATA((pScreen))->CreateGC;	\
}

static void
ctValidateGC(pGC, changes, pDraw)
GCPtr pGC;
Mask changes;
DrawablePtr pDraw;
{
#ifdef DEBUG_PRINT
	ErrorF("ValidateGC(): gc=0x%08x changes=0x%08x draw=0x%08x\n",
		pGC, changes, pDraw);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGC);
	(*pGC->funcs->ValidateGC)(pGC, changes, pDraw);
	CT_WRAP_GC(pGC);
}

static void
ctChangeGC(pGC, changes)
GCPtr pGC;
Mask changes;
{
#ifdef DEBUG_PRINT
	ErrorF("ChangeGC(): gc=0x%08x changes=0x%08x\n", pGC, changes);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGC);
	(*pGC->funcs->ChangeGC)(pGC, changes);
	CT_WRAP_GC(pGC);
}

static void
ctCopyGC(pGCSrc, changes, pGCDst)
GCPtr pGCSrc;
Mask changes;
GCPtr pGCDst;
{
#ifdef DEBUG_PRINT
	ErrorF("CopyGC(): src=0x%08x changes=0x%08x dst=0x%08x\n",
		pGCSrc, changes, pGCDst);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGCDst);
	(*pGCDst->funcs->CopyGC)(pGCSrc, changes, pGCDst);
	CT_WRAP_GC(pGCDst);
}

static void
ctDestroyGC(pGC)
GCPtr pGC;
{
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);

#ifdef DEBUG_PRINT
	ErrorF("DestroyGC(): gc=0x%08x\n", pGC);
#endif /* DEBUG_PRINT */

	/*
	 * FIRST, free all memory allocated for our GC private structure.
	 */
	if (ctGCPriv->chunk != (void *)0) {
		CT(ImageCacheFree)(pGC->pScreen, ctGCPriv->chunk);
		ctGCPriv->chunk = (void *)0;
	}

	/*
	 * Then, call the wrapped routine.
	 */
	CT_UNWRAP_GC(pGC);
	(*pGC->funcs->DestroyGC)(pGC);
	CT_WRAP_GC(pGC);
}

static void
ctChangeClip(pGC, type, pvalue, nrects)
GCPtr pGC;
int type;
pointer pvalue;
int nrects;
{
#ifdef DEBUG_PRINT
	ErrorF("ChangeClip(): gc=0x%08x t=%d v=0x%08x n=%d\n",
		pGC, type, pvalue, nrects);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGC);
	(*pGC->funcs->ChangeClip)(pGC, type, pvalue, nrects);
	CT_WRAP_GC(pGC);
}

static void
ctDestroyClip(pGC)
GCPtr pGC;
{
#ifdef DEBUG_PRINT
	ErrorF("DestroyClip(): gc=0x%08x\n", pGC);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGC);
	(*pGC->funcs->DestroyClip)(pGC);
	CT_WRAP_GC(pGC);
}

static void
ctCopyClip(pGCDst, pGCSrc)
GCPtr pGCDst;
GCPtr pGCSrc;
{
#ifdef DEBUG_PRINT
	ErrorF("CopyClip(): dst=0x%08x src=0x%08x\n", pGCDst, pGCSrc);
#endif /* DEBUG_PRINT */

	CT_UNWRAP_GC(pGCDst);
	(*pGCDst->funcs->CopyClip)(pGCDst, pGCSrc);
	CT_WRAP_GC(pGCDst);
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * CT(CreateGC)() - initialize GC member functions.
 *
 * NOTE: This routine MUST wrap the screen function given at screen
 * initialization time (see ctInit.c).
 */
Bool
CT(CreateGC)(pGC)
GCPtr pGC;
{
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);
	Bool rval;

#ifdef DEBUG_PRINT
	ErrorF("CreateGC(): gc=0x%08x\n", pGC);
#endif /* DEBUG_PRINT */

	/*
	 * FIRST, call the wrapped routine.
	 */
	CT_UNWRAP_SCREEN(pGC->pScreen);
	rval = (*pGC->pScreen->CreateGC)(pGC);
	CT_WRAP_SCREEN(pGC->pScreen);

	if (rval == FALSE)
		return (FALSE);

	/*
	 * Then, wrap the GC function block so we can replace DestroyGC().
	 *
	 * NOTE: We can't just replace the DestroyGC() method since pGC->funcs
	 * points to a static NFB structure. Replacing the method would change
	 * the NFB pointer in the static structure; so any subsequent GCs will
	 * be initialized with our wrapper (i.e we lose the pointer to the NFB
	 * method).
	 */
	CT_WRAP_GC(pGC);

	/*
	 * Now, go ahead and do ONLY the initialization for our private
	 * structure. DO NOT modify any other GC members!
	 */
	ctGCPriv->chunk = (void *)0;

	return (TRUE);
}

void
CT(ValidateWindowGC)(pGC, changes, pDraw)
GCPtr pGC;
Mask changes;
DrawablePtr pDraw;
{
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);
	PixmapPtr pPixmap = (PixmapPtr)NULL;
	unsigned long fg, bg;

#ifdef DEBUG_PRINT
	ErrorF("ValidateWindowGC(): gc=0x%08x changes=0x%08x draw=0x%08x\n",
		pGC, changes, pDraw);
#endif /* DEBUG_PRINT */

	/*
	 * Let nfb set up the GC ops and handle tile/stipple initialization
	 * issues. We need to do this BEFORE grabbing the pGC->stipple pointer
	 * since nfbHelpValidateGC() tries to expand it to 32x32 and, if
	 * successful, destroys the old pixmap pointer.
	 */
	nfbHelpValidateGC(pGC, changes, pDraw);

	if (changes & (GCFillStyle | GCTile | GCStipple |
		       GCForeground | GCBackground)) {
		switch (pGC->fillStyle) {
		case FillSolid:
			nfbGCPriv->ops = &CT(SolidPrivOps);
			break;
		case FillTiled:
			nfbGCPriv->ops = &CT(TiledPrivOps);
			if (!pGC->tileIsPixel) {
				pPixmap = pGC->tile.pixmap;
				fg = bg = 0L;	/* unused */
			}
			break;
		case FillStippled:
			nfbGCPriv->ops = &CT(StippledPrivOps);
			pPixmap = pGC->stipple;
			fg = ~0L;
			bg = 0L;
			break;
		case FillOpaqueStippled:
			fg = nfbGCPriv->rRop.fg;
			bg = nfbGCPriv->rRop.bg;
			if (fg == bg) {
				nfbGCPriv->ops = &CT(SolidPrivOps);
			} else {
				nfbGCPriv->ops = &CT(OpStippledPrivOps);
				pPixmap = pGC->stipple;
			}
			break;
		}
	}

	/*
	 * AFTER nfb handles initialization of tile/stipple pixmaps, we try
	 * to cache the pixmap offscreen.
	 */
	if (pPixmap)
		CT(CacheGCPixmap)(pGC, pPixmap, fg, bg);
}

void
CT(CacheGCPixmap)(pGC, pPixmap, fg, bg)
GCPtr pGC;
PixmapPtr pPixmap;
unsigned long fg, bg;
{
	CT(GCPrivPtr) ctGCPriv = CT_GC_PRIV(pGC);
	unsigned int width, height, depth;

#ifdef DEBUG_PRINT
	ErrorF("CacheGCPixmap(): gc=0x%08x pix=0x%08x fg=0x%08x bg=0x%08x\n",
		pGC, pPixmap, fg, bg);
#endif /* DEBUG_PRINT */

	if (ctGCPriv->chunk != (void *)0) {
		CT(ImageCacheFree)(pGC->pScreen, ctGCPriv->chunk);
		ctGCPriv->chunk = (void *)0;
	}

	width = (unsigned int)pPixmap->drawable.width;
	height = (unsigned int)pPixmap->drawable.height;
	depth = (int)pPixmap->drawable.depth;

#ifdef DEBUG_PRINT
	ErrorF("	image=0x%08x stride=%d depth=%d w=%d h=%d\n",
		pPixmap->devPrivate.ptr, pPixmap->devKind,
		depth, width, height);
#endif /* DEBUG_PRINT */

	ctGCPriv->chunk = CT(ImageCacheLoad)(pGC->pScreen,
					pPixmap->devPrivate.ptr,
					pPixmap->devKind,
					depth,
					&width,
					&height,
					fg, bg);
	ctGCPriv->width = width;
	ctGCPriv->height = height;
}

void
CT(ValidateWindowGCNoWrap)(pGC, changes, pDraw)
GCPtr pGC;
Mask changes;
DrawablePtr pDraw;
{
	nfbGCPrivPtr nfbGCPriv = NFB_GC_PRIV(pGC);

#ifdef DEBUG_PRINT
	ErrorF("ValidateWindowGC(): gc=0x%08x changes=0x%08x draw=0x%08x\n",
		pGC, changes, pDraw);
#endif /* DEBUG_PRINT */

	if (changes & GCFillStyle) {
		switch (pGC->fillStyle) {
		case FillSolid:
			nfbGCPriv->ops = &CT(SolidPrivOps);
			break;
		case FillTiled:
			nfbGCPriv->ops = &CT(TiledPrivOps);
			break;
		case FillStippled:
			nfbGCPriv->ops = &CT(StippledPrivOps);
			break;
		case FillOpaqueStippled:
			if (nfbGCPriv->rRop.fg == nfbGCPriv->rRop.bg) {
				nfbGCPriv->ops = &CT(SolidPrivOps);
			} else {
				nfbGCPriv->ops = &CT(OpStippledPrivOps);
			}
			break;
		}
	}

	nfbHelpValidateGC(pGC, changes, pDraw);
}
