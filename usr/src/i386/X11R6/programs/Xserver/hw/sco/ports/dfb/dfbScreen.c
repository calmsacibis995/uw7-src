/*
 *	@(#)dfbScreen.c	11.1	10/22/97	12:01:49
 *	@(#)dfbScreen.c	6.2	1/23/96	15:40:50
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
 *	S001	Thu Sep  4 11:46:39 PDT 1997	hiramc@sco.COM
 *	- properly ifdef out the ioctl SW_VGA12
 */
/*
 * dfbScreen.c
 *
 * dfb screen procedures
 */
#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"

#include "dfbScrStr.h"

#if defined(usl)			/*	S001	*/
#include <sys/types.h>
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

/*
 * This assumes a VGA like adaptor
 */
Bool
dfbSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    Bool on;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(pScreen);

    if (dfbPriv->inGfxMode) {
	if (on == SCREEN_SAVER_ON) {
	    if (dfbPriv->BlankScrFunc) {
		grafRunFunction(dfbPriv->pGraf, dfbPriv->BlankScrFunc, NULL);
	    } else {				/* Assume VGA */
		inb(0x3DA);			/* reset attribute flipflop */
		outb(0x3C0, 0x00);		/* palette off */
	    }
	} else {
	    if (dfbPriv->UnblankScrFunc) {
		grafRunFunction(dfbPriv->pGraf, dfbPriv->UnblankScrFunc, NULL);
	    } else {				/* Assume VGA */
		inb(0x3DA);			/* reset attribute flipflop */
		outb(0x3C0, 0x20);		/* palette on  */
	    }
	}
    }

    if (on != SCREEN_SAVER_ON)
	SetTimeSinceLastInputEvent();

    return(TRUE);
}

void
dfbSetGraphics(pScreen)
    ScreenPtr pScreen;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(pScreen);

    if (!dfbPriv->inGfxMode) {
	/* tell console driver we are going to graphics mode */
#if !defined(usl)			/*	S001	*/
	ioctl(1, SW_VGA12, 0);
#endif

	grafExec(dfbPriv->pGraf, "SetGraphics", NULL);
	dfbPriv->inGfxMode = TRUE;
	dfbStoreColormap(dfbPriv);
    }
}

void
dfbSetText(pScreen)
    ScreenPtr pScreen;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(pScreen);

    if (dfbPriv->inGfxMode) {
	grafExec(dfbPriv->pGraf, "SetText", NULL);
	dfbPriv->inGfxMode = FALSE;
    }
}
