/*
 * @(#)svgaScreen.c 11.1
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
 * svgaScreen.c
 *
 * Template for machine dependent screen procedures
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
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"


/*
 * svgaBlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
svgaBlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("svgaBlankScreen(on=%d)\n", on);
#endif /* DEBUG_PRINT */
	return(FALSE);
}

/*
 * svgaSetGraphics(pScreen) - set screen into graphics mode
 */
void
svgaSetGraphics(pScreen)
	ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);
    nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV(pScreen);

    grafExec(grafinfo, "SetGraphics", NULL);
    (*nfbPriv->LoadColormap)(nfbPriv->installedCmap);
	
}

/*
 * svgaSetText(pScreen) - set screen into text mode
 */
void
svgaSetText(pScreen)
	ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);

    grafExec(grafinfo, "SetText", NULL);
}

/*
 * svgaSaveGState(pScreen) - save graphics info before screen switch
 */
void
svgaSaveGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("svgaSaveGState()\n");
#endif /* DEBUG_PRINT */
}

/*
 * svgaRestoreGState(pScreen) - restore graphics info from svgaSaveGState()
 */
void
svgaRestoreGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("svgaRestoreGState()\n");
#endif /* DEBUG_PRINT */
}

