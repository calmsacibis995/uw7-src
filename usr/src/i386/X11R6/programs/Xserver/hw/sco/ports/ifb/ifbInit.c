/*
 * @(#) ifbInit.c 12.2 95/06/09 
 *
 * Copyright (C) 1992-1993 The Santa Cruz Operation, Inc.
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
 * ifbInit.c
 *
 * Probe and Initialize the ifb for X
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "input.h"

#include "mipointer.h"
#include "misprite.h"

#include "nfbDefs.h"
#include "nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfbProcs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "ifbProcs.h"

extern ddxScreenInfo ifbScreenInfo;
extern scoScreenInfo ifbSysInfo;
extern VisualRec ifbVisual;
extern nfbGCOps ifbSolidPrivOps;

extern Bool mfbScreenInit();
extern Bool cfbScreenInit();
extern Bool cfb16ScreenInit();
extern Bool cfb32ScreenInit();
extern Bool cfbInitializeColormap();

pointer ifbBase[MAXSCREENS];				/* Base address */
int	ifbStride[MAXSCREENS];				/* Scanline length */


Bool
ifbTrue()
{
    return TRUE;
}

static miPointerSpriteFuncRec ifbPointerFuncs = {
	ifbTrue,
	ifbTrue,
	NoopDDA,
	NoopDDA,
};


/*
 * ifbProbe() - test for ifb
 *
 * returns true if present, false otherwise.
 */
Bool
ifbProbe(ddxDOVersionID version,ddxScreenRequest *pReq)
{
    return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

/*
 * ifbInit() - ifb screen init
 */
Bool
ifbInit(index, pScreen, argc, argv)
    int index;
    register ScreenPtr pScreen;
    int argc;
    char **argv;
{
    ddxScreenInfo *pInfo = ddxActiveScreens[index];
    int w, h, depth, mmx, mmy;
    int bpp, bpl, ppl;
    grafData* pGraf;
    register nfbScrnPrivPtr pNfb;
    Bool (* ScreenInit)();

    pGraf = pInfo->pRequest->grafinfo;
    depth = pInfo->pRequest->dfltDepth;
    bpp   = pInfo->pRequest->dfltBpp;

    /* Try grafinfo file */
    if ( !grafGetInt(pGraf, "PIXWIDTH",  &w) ||
	 !grafGetInt(pGraf, "PIXHEIGHT", &h)) {
	ErrorF("ifb: can't find pixel info in grafinfo file.\n");
	return FALSE;
    }

    ifbStride[index] = bpl = ((w * bpp + 31) >> 3) & ~3;
    if (!(ifbBase[index] = (pointer)xalloc(bpl * h))) {
	ErrorF("ifb: couldn't alloc %d byte frame buffer.\n", bpl * h);
	return FALSE;
    }

    if ( !grafGetInt(pGraf, "MON_WIDTH", &mmx))
	mmx = 274;
    if ( ! grafGetInt(pGraf, "MON_HEIGHT", &mmy))
	mmy = 207;

    /* Choose nfb or dfb (mfb/cfb) interface */
    if (grafQuery(pGraf, "USEDFB")) {

	switch (bpp) {
	    case 1:
		ppl = bpl << 3;
		ScreenInit = mfbScreenInit;
		break;
	    case 8:
		ppl = bpl;
		ScreenInit = cfbScreenInit;
		break;
	    case 16:
		ppl = bpl >> 1;
		ScreenInit = cfb16ScreenInit;
		break;
	    case 32:
		ppl = bpl >> 2;
		ScreenInit = cfb32ScreenInit;
		break;
	    default:
		ErrorF("%d bits per pixel not supported by Dfb\n", bpp);
		return FALSE;
	}

	pScreen->devPrivate		= NULL;

	if (!(* ScreenInit)(pScreen, ifbBase[index], w, h, mmx, mmy, ppl))
		return FALSE;

	if ((pNfb = (nfbScrnPrivPtr)xalloc(sizeof(nfbScrnPriv))) == NULL)
		return FALSE;

	/* if mi/mfb/cfb put a screen pixmap in devPrivate, incorporate it */
	if (pScreen->devPrivate) {
		pNfb->pixmap = *(PixmapPtr)(pScreen->devPrivate);
		xfree(pScreen->devPrivate);
	}
	pNfb->installedCmap		= NULL;

	pScreen->devPrivate		= (pointer)pNfb;
	pScreen->blackPixel		= 0;
	pScreen->whitePixel		= 1;
	pScreen->CreateColormap		= cfbInitializeColormap;
	pScreen->InstallColormap	= nfbInstallColormap;
	pScreen->UninstallColormap	= nfbUninstallColormap;
	pScreen->ListInstalledColormaps = nfbListInstalledColormaps;
	pScreen->SaveScreen		= nfbSaveScreen;

    } else {

	switch (bpp) {
	    case 8:
	    case 16:
	    case 32:
		break;
	    default:
		ErrorF("%d bits per pixel not yet supported by Nfb\n", bpp);
		return FALSE;
	}

	if (!nfbScreenInit(pScreen, w, h, mmx, mmy))
	    return FALSE;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &ifbSolidPrivOps;
	pNfb->ValidateWindowPriv = ifbValidateWindowPriv;

	ifbVisual.nplanes	  = depth;
	ifbVisual.class		  = pInfo->pRequest->dfltClass;

	if ((ifbVisual.class | DynamicClass) == DirectColor) {
	    int rbits, gbits, bbits;

	    gbits = (depth + 2) / 3;
	    rbits = (depth + 1 - gbits) / 2;
	    bbits = depth - (rbits + gbits);

	    ifbVisual.redMask   = ((1 << rbits) - 1) << (gbits + bbits);
	    ifbVisual.greenMask = ((1 << gbits) - 1) << bbits;
	    ifbVisual.blueMask  = (1 << bbits) - 1;
	    ifbVisual.offsetRed   = gbits + bbits;
	    ifbVisual.offsetGreen = bbits;
	    ifbVisual.offsetBlue  = 0;
	    ifbVisual.ColormapEntries = 1 << gbits;
	} else {
	    ifbVisual.ColormapEntries = 1 << depth;
	}

	if (!nfbAddVisual(pScreen, &ifbVisual))
	    return FALSE;

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);

    }

    pScreen->StoreColors = NoopDDA;

    pNfb->LoadColormap	= NoopDDA;
    pNfb->BlankScreen	= NoopDDA;

    if (grafQuery(pGraf, "NOCURSOR")) {
	scoPointerInitialize(pScreen, &ifbPointerFuncs, TRUE);
	pScreen->RecolorCursor = NoopDDA;
    } else {
	scoSWCursorInitialize(pScreen);
    }

    if (!cfbCreateDefColormap(pScreen))
	return FALSE;

    /* Initialize the sco layer */
    scoSysInfoInit(pScreen, &ifbSysInfo);

    return TRUE;
}

/*
 * ifbCloseScreen() - ifb close screen
 */
void
ifbCloseScreen(index, pScreen)
    int index;
    ScreenPtr pScreen;
{
    if (ifbBase[index]) {
	xfree(ifbBase[index]);
	ifbBase[index] = 0;
    }
}
