/*
 *	@(#)ctScreen.c	11.1	10/22/97	12:35:05
 *	@(#) ctScreen.c 62.1 97/03/21 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
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
 *	S001	Wed May 28 15:47:47 PDT 1997	-	hiramc@sco.COM
 *      - porting to Gemini, changes marked by #if defined(usl)
 */
/*
 * ctScreen.c
 *
 * Template for machine dependent screen procedures
 */

#ident "@(#) $Id: ctScreen.c 61.2 97/02/26 "

#include <stdio.h>
#include <sys/types.h>
#if ! defined(usl)				/*	S001	*/
#include <sys/console.h>
#endif

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

#include "ctDefs.h"
#include "ctMacros.h"

/*
 * CT(BlankScreen) - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
CT(BlankScreen)(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("BlankScreen(on=%d)\n", on);
#endif /* DEBUG_PRINT */

	/*
	 * Reset attribute flip-flop to select the attribute index register
	 * (ARX).
	 */
	inb(0x3da);

	if (on) {
		CT(CursorOff)(pScreen, FALSE);
		outb(0x3c0, 0x00);	/* ARX[5] disable video */
	} else {
		CT(CursorOn)(pScreen);
		outb(0x3c0, 0x20);	/* ARX[5] enable video  */
	}

	return(TRUE);
}

/*
 * CT(SetGraphics)(pScreen) - set screen into graphics mode
 */
void
CT(SetGraphics)(pScreen)
	ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	unsigned char xx_val;						\

#ifdef DEBUG_PRINT
	ErrorF("SetGraphics()\n");
#endif /* DEBUG_PRINT */

#if ! defined(usl)				/*	S001	*/
	/*
	 * Tell SCO we've gone into graphics mode. This should really be done
	 * by sys!
	 */
	if (ioctl(fileno(stdout), SW_VGA12, 1) == -1) {
		perror("ioctl: SW_VGA12");
		FatalError("SetGraphics(): failed\n");
		/* NOTREACHED */
	}
#endif

	/*
	 * Set the VESA BIOS graphics mode.
	 */
	grafExec(grafinfo, "SetGraphics", NULL);
#if (CT_BITS_PER_PIXEL == 8)
	/*
	 * Set BitBlt control (XR20) to handle depth of 8 bits.
	 */
	CT_XRIN(0x20, xx_val);
	xx_val &= 0xcf; /* turn off bit 4,5 */
	CT_XROUT(0x20, xx_val);
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
	/*
	 * Set BitBlt control (XR20) to handle depth of 16 bits.
	 */
	CT_XRIN(0x20, xx_val);
	xx_val &= 0xdf; /* turn off bit 4,5 */
	xx_val |= 0x10; /* turn on bit 4 */
	CT_XROUT(0x20, xx_val);
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if(CT_BITS_PER_PIXEL == 24)
	/*
	 * Set BitBlt control (XR20) to handle depth of 24 bits.
	 */
	CT_XRIN(0x20, xx_val);
	xx_val &= 0xef /* turn off bit 4,5 */
	xx_val |= 0x20 /* turn on bit 5 */
	CT_XROUT(0x20, xx_val);
#endif /* (CT_BITS_PER_PIXEL == 24) */

	CT_ENABLE_BITBLT();
}

/*
 * CT(SetText)(pScreen) - set screen into text mode
 */
void
CT(SetText)(pScreen)
	ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("SetText()\n");
#endif /* DEBUG_PRINT */


	CT_WAIT_FOR_IDLE();
	CT_DISABLE_BITBLT();

	grafExec(grafinfo, "SetText", NULL);
}

/*
 * CT(SaveGState)(pScreen) - save graphics info before screen switch
 */
void
CT(SaveGState)(pScreen)
	ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
	ErrorF("SaveGState()\n");
#endif /* DEBUG_PRINT */

	/*
	 * Turn the cursor off forcing any cached cursors out of memory.
	 */
	CT(CursorOff)(pScreen, TRUE);
}

/*
 * CT(RestoreGState)(pScreen) - restore graphics info from CT(SaveGState)()
 */
void
CT(RestoreGState)(pScreen)
	ScreenPtr pScreen;
{
	nfbScrnPrivPtr nfbPriv = NFB_SCREEN_PRIV(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("RestoreGState()\n");
#endif /* DEBUG_PRINT */

	/*
	 * Reset the ctUtils.c module for the new screen.
	 */
	CT(BitBltScreenSwitch)(pScreen);

	/*
	 * This should really be done by NFB!
	 */
	(*nfbPriv->LoadColormap)(nfbPriv->installedCmap);

	CT(CursorOn)(pScreen);
}
