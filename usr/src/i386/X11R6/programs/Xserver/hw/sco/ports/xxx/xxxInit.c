/*
 * @(#) xxxInit.c 11.1 97/10/22
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
 * xxxInit.c
 *
 * Initialize the xxx Graphics Display Driver
 */

#include <sys/types.h>

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "xxxDefs.h"
#include "xxxProcs.h"

extern scoScreenInfo xxxSysInfo;
extern VisualRec xxxVisual;
extern nfbGCOps xxxSolidPrivOps;

static int xxxGeneration = -1;
unsigned int xxxScreenPrivateIndex = -1;


/*
 * xxxSetup() - initializes information needed by the core server
 * prior to initializing the hardware.
 *
 */
Bool
xxxSetup(ddxDOVersionID version,ddxScreenRequest *pReq)
{
    return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

/*
 * xxxInitHW()
 *
 * Initialize hardware that only needs to be done ONCE.  This routine will
 * not be called on a screen switch.  It may just call xxxSetGraphics()
 */
Bool
xxxInitHW(pScreen)
    ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);
    xxxPrivatePtr xxxPriv = XXX_PRIVATE_DATA(pScreen);

    /*
     * Only do this if you need memory mapped 
     */
    if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &xxxPriv->fbBase)) {
	ErrorF ("xxx: Missing MEMORY in grafinfo file.\n");
	return (FALSE);
    }

    xxxSetGraphics(pScreen);
    return (TRUE);
}

/*
 * xxxInit() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
xxxInit(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
        grafData *grafinfo = DDX_GRAFINFO(pScreen);
	nfbScrnPrivPtr pNfb;
	xxxPrivatePtr xxxPriv;
	int width, height, mmx, mmy;

	ErrorF("xxxInit(%d)\n", index);

	if (xxxGeneration != serverGeneration) {
		xxxGeneration = serverGeneration;
		xxxScreenPrivateIndex = AllocateScreenPrivateIndex();
		if ( xxxScreenPrivateIndex < 0 )
			return FALSE;
	}

	xxxPriv = (xxxPrivatePtr)xalloc(sizeof(xxxPrivate));
	if ( xxxPriv == NULL )
		return FALSE;
        pScreen->devPrivates[xxxScreenPrivateIndex].ptr =
                                        (unsigned char *)xxxPriv;


	/* Get mode and monitor info */
	if ( !grafGetInt(grafinfo, "PIXWIDTH",  &width)  ||
	     !grafGetInt(grafinfo, "PIXHEIGHT", &height)) {
	    ErrorF("xxx: can't find pixel info in grafinfo file.\n");
	    return FALSE;
	}

	mmx = 300; mmy = 300;  /* Reasonable defaults */

	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

	if (!nfbScreenInit(pScreen, width, height, mmx, mmy))
		return FALSE;

	if (!nfbAddVisual(pScreen, &xxxVisual))
		return FALSE;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &xxxSolidPrivOps;
	pNfb->SetColor		 = xxxSetColor;
	pNfb->LoadColormap	 = genLoadColormap;
	pNfb->BlankScreen	 = xxxBlankScreen;
	pNfb->ValidateWindowPriv = xxxValidateWindowPriv;

	if (!xxxInitHW(pScreen))
		return FALSE;

	/*
	 * Call one of the following.  
	 *
	 * scoSWCursorInitialize(pScreen);
	 * xxxCursorInitialize(pScreen);
	 *
	 * If you implement a hardware cursor it's always handy to 
	 * have a grafinfo variable which will switch back to the
	 * software cursor for debugging purposes.
	 *
	 */
	scoSWCursorInitialize(pScreen);

	/*
	 * This should work for most cases.
	 */
	if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
		cfbCreateDefColormap(pScreen)) == 0 )
	    return FALSE;

	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &xxxSysInfo);

	/*
	 * Set any NFB runtime options here - see potential list
	 *	in ../../nfb/nfbDefs.h
	 */
	nfbSetOptions(pScreen, NFB_VERSION, NFB_POLYBRES, 0);

	return TRUE;

}

/*
 * xxxCloseScreen()
 *
 * Anything you allocate in xxxInit() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
void
xxxCloseScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	xxxPrivatePtr xxxPriv = XXX_PRIVATE_DATA(pScreen);

	xfree(xxxPriv);
}

