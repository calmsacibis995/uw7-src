/*
 *	@(#)wd33Screen.c	11.1	10/22/97	12:26:32
 *
 * Copyright (C) 1991-1997 The Santa Cruz Operation, Inc.
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
 * wd33Screen.c
 *
 * Probe and Initialize the wd33 Graphics Display Driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *      S001    Thu 18-Aug-1993 edb@sco.com
 *  		Cosmetics
 *	S002	Thu Sep  4 14:57:42 PDT 1997	hiramc@sco.COM
 *		No longer need the ioctl on the SW_VGA12 in gemini
 */

#ifdef usl
#include <sys/types.h>
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "grafinfo.h"
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

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wdBankMap.h"

/*
 * wd33BlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */

Bool
wd33BlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
#ifdef DEBUG_PRINT
	ErrorF("wd33BlankScreek(on=%d)\n", on);
#endif /* DEBUG_PRINT */

	inb(0x3DA);			/* reset attribute flipflop */
	if (on)
	    outb(0x3C0, 0x00);		/* palette off */
	else
	    outb(0x3C0, 0x20);		/* palette on  */
	return(TRUE);
}

static Bool wdModeChanged = FALSE;

/*
 * wd33SetGraphics(pScreen) - set screen into graphics mode
 */
void
wd33SetGraphics(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
        int curBlock = wdPriv->curRegBlock;
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	static int first = 1;

#ifdef DEBUG_PRINT
	ErrorF("wd33SetGraphics()\n");
#endif /* DEBUG_PRINT */

	if (!wdModeChanged) {
	    /* tell console driver we are going to graphics mode */
#if ! defined(usl)			/*	S002	*/
	    ioctl(1, SW_VGA12, 0);
#endif
	    grafExec(grafinfo, "SetGraphics", NULL);

	    wdModeChanged = TRUE;

	    /* Initialize wdc33 blitting     */
	    WAITFOR_BUFF( 8 );
	    WRITE_REG( ENG_2, MAP_BASE     , 0  );
	    WRITE_REG( ENG_2, ROW_PITCH , wdPriv->rowPitch );
            WRITE_REG( ENG_1, LEFT_CLIP   , 0  );
            WRITE_REG( ENG_1, RIGHT_CLIP  , pScreen->width-1);
	    WAITFOR_BUFF( 8 );
            WRITE_REG( ENG_1, TOP_CLIP    , 0  );
            WRITE_REG( ENG_1, BOTTOM_CLIP , wdPriv->fbSize / wdPriv->fbStride );
	    outb( CMD_BUFF_CTRL, ENABLE_CMD_BUFF );

	    wdPriv->curCursor = NULL;
	}
        wdPriv->curRegBlock = curBlock;
}

/*
 * wd33SetText(pScreen) - set screen into text mode
 */
void
wd33SetText(pScreen)
	ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
 
#ifdef DEBUG_PRINT
	ErrorF("wd33SetText()\n");
#endif /* DEBUG_PRINT */

	/* 
	 *  Make sure cursor is turned off before we go into textmode .
	 *  This would cause the bus to block !! 
	 */
#ifdef NOT_YET
	if( wdPriv->curCursor != NULL )
	    wd33SetCursor( pScreen, NULL, 0,0 );
#endif

        WAITFOR_DE();

	if (wdModeChanged) {
	    grafExec(grafinfo, "SetText", NULL);
	    wdModeChanged = FALSE;
	}
}

/*
 * wd33SaveGState(pScreen) - save graphics info before screen switch
 */
void
wd33SaveGState(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );

#ifdef DEBUG_PRINT
	ErrorF("wd33SaveGState()\n");
#endif /* DEBUG_PRINT */

        if (wdPriv->glCacheHeight > 0 )
	     wd33DeallocGlCache( wdPriv );
}

/*
 * wd33RestoreGState(pScreen) - restore graphics info from wdSaveGState()
 */
void
wd33RestoreGState(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
	nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV( pScreen );
#ifdef DEBUG_PRINT
	ErrorF("wd33RestoreGState()\n");
#endif /* DEBUG_PRINT */

        if (wdPriv->glCacheHeight > 0 )
	    wd33InitGlCache( wdPriv );

	(*nfbPriv->LoadColormap)(nfbPriv->installedCmap);
	wdPriv->tileSerial = 0;
	WD_MAP_RESET(wdPriv);			/* reset bank-mapping */
}

