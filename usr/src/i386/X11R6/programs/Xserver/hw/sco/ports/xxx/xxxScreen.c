/*
 * @(#) xxxScreen.c 11.1 97/10/22
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
 * xxxScreen.c
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


/*
 * xxxBlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
xxxBlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("xxxBlankScreen(on=%d)\n", on);
#endif /* DEBUG_PRINT */
	return(TRUE);
}

/*
 * xxxSetGraphics(pScreen) - set screen into graphics mode
 */
void
xxxSetGraphics(pScreen)
	ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);

    grafExec(grafinfo, "SetGraphics", NULL);
}

/*
 * xxxSetText(pScreen) - set screen into text mode
 */
void
xxxSetText(pScreen)
	ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);

    grafExec(grafinfo, "SetText", NULL);
}

/*
 * xxxSaveGState(pScreen) - save graphics info before screen switch
 */
void
xxxSaveGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("xxxSaveGState()\n");
#endif /* DEBUG_PRINT */
}

/*
 * xxxRestoreGState(pScreen) - restore graphics info from xxxSaveGState()
 */
void
xxxRestoreGState(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("xxxRestoreGState()\n");
#endif /* DEBUG_PRINT */
}

