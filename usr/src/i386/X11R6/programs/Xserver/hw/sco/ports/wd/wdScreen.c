/*
 *	@(#)wdScreen.c	11.1	10/22/97	12:27:46
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
 * wdScreen.c
 *
 * Probe and Initialize the wd Graphics Display Driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *      S001    Wdn 14-Oct-1992 edb@sco.com
 *              changes to implement WD90C31 cursor
 *	S002	Thu  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *	S003	Mon  23-Nov-1992	edb@sco.com
 *              Make sure cursor is turned off before we go into textmode
 *	S004	Wdn  25-Nov-1992	edb@sco.com
 *              Wait for unfinished business before going into textmode 
 *	S005	Sun  20-Dec-1992	buckm@sco.com
 *              Invalidate stored tile/stipple.
 *	S006	Tue  09-Feb-1993	buckm@sco.com
 *              No glyph caching for 24-bit mode.
 *		Get rid of GC caching.
 *		Reset bank-mapping.
 *	S007	Thu Sep  4 14:55:37 PDT 1997	hiramc@sco.COM
 *	- no longer need the ioctl on SW_VGA12
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

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBankMap.h"

/*
 * wdBlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
wdBlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
#ifdef DEBUG_PRINT
	ErrorF("wdBlankScreek(on=%d)\n", on);
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
 * wdSetGraphics(pScreen) - set screen into graphics mode
 */
void
wdSetGraphics(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	static int first = 1;

#ifdef DEBUG_PRINT
	ErrorF("wdSetGraphics()\n");
#endif /* DEBUG_PRINT */
#ifdef REG_DUMP
	if(first) RegDump("before SetGraphics", 1, 'T'); first = 0;
#endif

	if (!wdModeChanged) {
	    /* tell console driver we are going to graphics mode */
#if ! defined(usl)			/*	S007	*/
	    ioctl(1, SW_VGA12, 0);
#endif
	    grafExec(grafinfo, "SetGraphics", NULL);

    #ifdef REG_DUMP
	    RegDump("SetGraphics", 1, 'G'); 
    #endif
	    wdModeChanged = TRUE;

	    /* Initialize wdc31 blitting     */
	    SELECT_BITBLT_REG_BLOCK();
	    WAITFOR_WD();
	    WRITE_1_REG( ROWBYTES_IND , wdPriv->fbStride );

	    wdPriv->curCursor = NULL;	/* S003 reload in SetCursor */
	}
}

/*
 * wdSetText(pScreen) - set screen into text mode
 */
void
wdSetText(pScreen)
	ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
 
#ifdef DEBUG_PRINT
	ErrorF("wdSetText()\n");
#endif /* DEBUG_PRINT */

	/* 
	 *  Make sure cursor is turned off before we go into textmode .
	 *  This would cause the bus to block !! 
	 */
	if( wdPriv->curCursor != NULL )                 /* S003 */
	    wdSetCursor( pScreen, NULL, 0,0 );

	WAITFOR_WD();                 /* wait for unfinished business  S004 */

	if (wdModeChanged) {
	    grafExec(grafinfo, "SetText", NULL);
	    wdModeChanged = FALSE;
	}
#ifdef REG_DUMP
	RegDump("SetText", 1, 'T');
#endif
}

/*
 * wdSaveGState(pScreen) - save graphics info before screen switch
 */
void
wdSaveGState(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );

#ifdef DEBUG_PRINT
	ErrorF("wdSaveGState()\n");
#endif /* DEBUG_PRINT */

        if (wdPriv->glyphCache)
	     wdDeallocGlCache( wdPriv );
}

/*
 * wdRestoreGState(pScreen) - restore graphics info from wdSaveGState()
 */
void
wdRestoreGState(pScreen)
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
	nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV( pScreen );
#ifdef DEBUG_PRINT
	ErrorF("wdRestoreGState()\n");
#endif /* DEBUG_PRINT */

        if (wdPriv->glyphCache)
	    wdInitGlCache( wdPriv );

	(*nfbPriv->LoadColormap)(nfbPriv->installedCmap);
	wdPriv->tileSerial = 0;				/* S005 */
	wdPriv->fillColor = -1;
	WD_MAP_RESET(wdPriv);			/* reset bank-mapping */
}

