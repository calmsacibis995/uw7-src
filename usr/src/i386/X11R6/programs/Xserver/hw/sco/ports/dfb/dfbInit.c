/*
 * @(#) dfbInit.c 12.2 95/06/09 SCOINC
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
 * dfbInit.c
 *
 * Probe and Initialize the dfb for X
 */

#include "X.h"
#include "misc.h"
#include "scrnintstr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "dfbProcs.h"
#include "dfbScrStr.h"


extern scoScreenInfo dfbSysInfo;

int dfbGeneration = -1;
int dfbScreenPrivateIndex;


/*
 * dfbProbe() - test for dfb
 *
 * returns true if present, false otherwise.
 */
Bool
dfbProbe(version, pReq)
    ddxDOVersionID version;
    ddxScreenRequest *pReq;
{
    static int scrnum = 0;
    int res;

    dfbSSProbePrep(pReq, scrnum);	/* allows h/w to be touched here */

    switch (pReq->dfltDepth) {
	case 1:
	case 8:
	    res = ddxAddPixmapFormat(pReq->dfltDepth,
				     pReq->dfltBpp, pReq->dfltPad);
	    break;
	default:
	    ErrorF("dfb: unsupported screen depth: %d\n", pReq->dfltDepth);
	    res = FALSE;
    }

    if (res)
	++scrnum;

    return res;
}

/*
 * dfbInitHW() - dfb initialization code
 */
void
dfbInitHW(pScreen)
    ScreenPtr pScreen;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(pScreen);

    if (dfbGeneration > 1) {
	dfbPriv->inGfxMode = TRUE;
    } else {
	dfbPriv->inGfxMode = FALSE;
	dfbSetGraphics(pScreen);
    }
}

/*
 * dfbInit() - dfb screen init
 */
Bool
dfbInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;
    char **argv;
{
    grafData *pGraf = DDX_GRAFINFO(pScreen);
    dfbScrnPrivPtr dfbPriv;
    scoScreenInfo *pSysInfo;
    int depth, pixwidth, pixheight, pixbytes;
    pointer fbbase;
    int mmx, mmy, rgbbits;
    codeType *setcolor, *blankscr, *unblankscr;
    int dfbMono;

    dfbSSInitPrep(index, pScreen);	/* h/w can be touched now */

    /* Get mode and monitor info */
    if ( !grafGetInt(pGraf, "DEPTH",	 &depth)   ||
         !grafGetInt(pGraf, "PIXWIDTH",	 &pixwidth)  ||
	 !grafGetInt(pGraf, "PIXHEIGHT", &pixheight) ||
	 !grafGetInt(pGraf, "PIXBYTES",	 &pixbytes)) {
	ErrorF("dfb: missing pixel info in grafinfo file.\n");
	return FALSE;
    }

    if (!grafGetMemInfo(pGraf, NULL, NULL, NULL, (int *)&fbbase)) {
	ErrorF ("dfb: missing Memory in grafinfo file.\n");
	return FALSE;
    }

    mmx = 300; mmy = 300;  /* reasonable defaults */
    grafGetInt(pGraf, "MON_WIDTH",  &mmx);
    grafGetInt(pGraf, "MON_HEIGHT", &mmy);

    rgbbits = 6;  /* defaults for VGA */
    setcolor = blankscr = unblankscr = NULL;
    grafGetInt(pGraf, "RGBBITS", &rgbbits);
    grafGetFunction(pGraf, "SetColor",	    &setcolor);
    grafGetFunction(pGraf, "BlankScreen",   &blankscr);
    grafGetFunction(pGraf, "UnblankScreen", &unblankscr);

    /* allocate and attach screen private data */
    if (dfbGeneration != serverGeneration) {
	    dfbGeneration = serverGeneration;
	    dfbScreenPrivateIndex = AllocateScreenPrivateIndex();
	    if (dfbScreenPrivateIndex < 0)
		    return FALSE;
    }
    if ((dfbPriv = (dfbScrnPrivPtr)xalloc(sizeof(dfbScrnPriv))) == NULL)
	    return FALSE;
    pScreen->devPrivates[dfbScreenPrivateIndex].ptr =
                                        (unsigned char *)dfbPriv;


    /* use mfb or cfb ? */
    dfbMono = depth == 1;

    if (dfbMono) {
	if (!mfbScreenInit(pScreen, fbbase, pixwidth, pixheight,
			    mmx, mmy, pixbytes * 8))
	    return FALSE;
    } else {
	if (!cfbScreenInit(pScreen, fbbase, pixwidth, pixheight,
			    mmx, mmy, pixbytes))
	    return FALSE;
    }

    pScreen->SaveScreen = dfbSaveScreen;
    pScreen->whitePixel = 1;
    pScreen->blackPixel = 0;
    if (!dfbMono) {
	pScreen->StoreColors		= dfbStoreColors;
	pScreen->InstallColormap	= dfbInstallColormap;
	pScreen->UninstallColormap	= dfbUninstallColormap;
	pScreen->ListInstalledColormaps	= dfbListInstalledColormaps;
    }

    /* initialize screen private data */
    dfbPriv->pGraf		= pGraf;
    dfbPriv->fbBase		= fbbase;
    dfbPriv->currentCmap	= NULL;
    dfbPriv->dacShift		= 16 - rgbbits;
    dfbPriv->SetColorFunc	= setcolor;
    dfbPriv->BlankScrFunc	= blankscr;
    dfbPriv->UnblankScrFunc	= unblankscr;

    dfbInitHW(pScreen);

    scoSWCursorInitialize(pScreen);

    if ((dfbMono ? mfbCreateDefColormap(pScreen)
		 : cfbCreateDefColormap(pScreen)) == 0)
	return FALSE;

    pSysInfo = &dfbSysInfo;

    /* initialize the ScreenSwitch layer */
    if (!dfbSSInitialize(pScreen, &pSysInfo))
	return FALSE;

    /* initialize the sco layer */
    scoSysInfoInit(pScreen, pSysInfo);

    return TRUE;
}

/*
 * dfbFreeScreen() - dfb screen clean-up
 */
void
dfbFreeScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(pScreen);

	/* free screen private data */
	xfree(dfbPriv);
}
