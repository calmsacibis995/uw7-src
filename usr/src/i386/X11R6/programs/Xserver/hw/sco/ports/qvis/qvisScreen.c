/**
 *	@(#)qvisScreen.c	11.2	12/16/97	14:49:35
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
 *	SCO MODIFICATION HISTORY
 *
 *	S002	Thu Dec  11 14:44:04 PDT 1997	brianr@sco.COM
 *	- blanking the screen shouldn't be dependent on scoScreenActive()
 *	S001	Thu Sep  4 14:44:02 PDT 1997	hiramc@sco.COM
 *	- no longer need the ioctl on SW_VGA12 for gemini
 *      S000    Tue Jul 13 12:49:13 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *
 */

/*
 * qvisScreen.c
 * 
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * mikep@sco 07/27/92  Reset current_bank in qvisRestoreGState()
 * waltc     06/26/93  Move qvisSetGraphics out's to grafinfo file.
 *
 */

#include "xyz.h"
#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include <sys/types.h>
#include "screenint.h"

#include "scrnintstr.h"
#include "ddxScreen.h"
#include "grafinfo.h"

#include "qvisDefs.h"
#include "qvisHW.h"
#include "qvisMacros.h"
#include "qvisProcs.h"

#ifdef usl
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

#if ! defined(usl)			/*	S001	*/
extern int      ioctl();
#endif
/*	extern Bool     scoScreenActive();	S002	*/

/*
 * There is a discussion of in the "Video DAC" section of the Triton
 * TRG about "Aaddress Index Save and Restore"; it is my opinion this
 * is really important only when palette manipulation is done in
 * a signal or interrupt handler.  Since the X server doesn't do such
 * a thing, the address index save is not needed. -mjk
 */

/*
 * qvisSaveDACState - saves the state of the DAC color table on a
 * per screen basis so it can be restored when the server exits.
 */
void
qvisSaveDACState(
ScreenPtr pScreen)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    int i;

    XYZ("qvisSaveDACState-entered");
#ifdef DEBUG
    ErrorF("DAC status, 13c6 %x  DAC CR1 13c8 %x\n",
	   qvisIn8(0x13c6), qvisIn8(0x13c8));
    ErrorF("DAC cr2, 13c9 %x  DAC CR0 83c6 %x\n",
	   qvisIn8(0x13c9), qvisIn8(0x83c6));
#endif
	for (i = 0; i < NUMPALENTRIES; i++) {
	    qvisIn8(0x84);
	    qvisOut8(0x3c8, (unsigned char) i);
	    qvisIn8(0x84);
	    qvisPriv->dac_state[i].red = (unsigned char) qvisIn8(0x3c9);
	    qvisIn8(0x84);
	    qvisPriv->dac_state[i].green = (unsigned char) qvisIn8(0x3c9);
	    qvisIn8(0x84);
	    qvisPriv->dac_state[i].blue = (unsigned char) qvisIn8(0x3c9);
	    qvisIn8(0x84);
	}
    XYZ("qvisSaveDACState-exit");
}

/*
 * qvisRestoreDACState - restores the state of the DAC color table
 * on a per screen basis.
 */
void
qvisRestoreDACState(
ScreenPtr pScreen)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    int i;

    XYZ("qvisSaveDACState-entered");
#ifdef DEBUG_PRINT
    ErrorF("DAC status, 13c6 %x  DAC CR1 13c8 %x\n",
	   qvisIn8(0x13c6), qvisIn8(0x13c8));
    ErrorF("DAC cr2, 13c9 %x  DAC CR0 83c6 %x\n",
	   qvisIn8(0x13c9), qvisIn8(0x83c6));
#endif				/* DEBUG_PRINT */
    for (i = 0; i < NUMPALENTRIES; i++) {
	qvisOut8(0x3c8, (unsigned char) i);
	qvisIn8(0x84);
	qvisOut8(0x3c9, qvisPriv->dac_state[i].red);
	qvisIn8(0x84);
	qvisOut8(0x3c9, qvisPriv->dac_state[i].green);
	qvisIn8(0x84);
	qvisOut8(0x3c9, qvisPriv->dac_state[i].blue);
	qvisIn8(0x84);
    }
    XYZ("qvisSaveDACState-exit");
}

/**
 * qvisBlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
qvisBlankScreen(on, pScreen)
    int             on;
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisBlankScreen-entered");
    qvisSetCurrentScreen();
    /* if (scoScreenActive()) */ {		/* S002 */
	XYZ("qvisBlankScreen-scoScreenActive-TRUE");
	qvisOut8(GS_INDEX, 0x01);/* set index for clocking mode reg */
	if (on == SCREEN_SAVER_ON) {
	    XYZ("qvisBlankScreen-SCREEN_SAVER_ON");
	    qvisOut8(GS_DATA, (qvisIn8(GS_DATA) & 0xdf));
	} else {
	    XYZ("qvisBlankScreen-SCREEN_SAVER_OFF");
	    qvisOut8(GS_DATA, (qvisIn8(GS_DATA) | 0x20));
	}
    }
    XYZ("qvisBlankScreen-exit");
    return TRUE;
}

/*
 * qvisSetGraphics(pScreen) - set screen into graphics mode
 */
void
qvisSetGraphics(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    grafData       *grafinfo = DDX_GRAFINFO(pScreen);

    XYZ("qvisSetGraphics-entered");
    qvisSetCurrentScreen();
    /* put CONSOLE into graphics mode!  This is important! */
#if ! defined(usl)			/*	S001	*/
    (void) ioctl(1, SW_VGA12, 0);
#endif

    grafExec(grafinfo, "SetGraphics", NULL);
    qvisRestoreColormap(pScreen);
#ifdef usl
    qvisBlankScreen(SCREEN_SAVER_ON, pScreen);
#endif
}

/*
 * qvisSetText(pScreen) - set screen into text mode
 */
void
qvisSetText(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    grafData       *grafinfo = DDX_GRAFINFO(pScreen);

    XYZ("qvisSetText-entered");
    qvisSetCurrentScreen();
    if (qvisPriv->primary) {
	/*
	 * The primary screen should be set back to text mode.
	 */
	grafExec(grafinfo, "SetText", NULL);
    } else {
	/*
	 * Turn the video off for all secondary screens.
	 */
	qvisBlankScreen(SCREEN_SAVER_OFF, pScreen);
    }
    qvisRestoreDACState(pScreen);
    XYZ("qvisSetText-exit");
}

/*
 * qvisSaveGState(pScreen) - save graphics info before screen switch
 */
void
qvisSaveGState(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisSaveGState-entered");
    qvisSetCurrentScreen();
    qvisWaitForGlobalNotBusy();
}

#ifdef QVIS_SHADOWED
/*
 * qvisInvalidateShadows - invalidate all the shadow register values we keep
 * in main memory for fast access.
 * 
 * This needs to be done upon returning from a screen switch and if we reset the
 * blit engine.
 */
void
qvisInvalidateShadows(qvisPriv)
    qvisPrivateData *qvisPriv;
{
    XYZ("qvisInvalidateShadows-entered");
    qvisPriv->alu = -1;
    qvisPriv->fg = -1;
    qvisPriv->bg = -1;
    qvisPriv->pixel_mask = -1;
    qvisPriv->plane_mask = -1;
}
#endif				/* QVIS_SHADOWED */

/*
 * qvisRestoreGState(pScreen) - restore graphics info from qvisSaveGState()
 * 
 * We don't really have any state to restore - but we do needed to flush the
 * in-memory shadows of the various Q-Vision registers for this screen AND
 * invalidate the glyph cache.
 */
void
qvisRestoreGState(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisRestoreGState-entered");
    qvisWaitForGlobalNotBusy();
#ifdef QVIS_SHADOWED
    qvisInvalidateShadows(qvisPriv);
#endif
    qvisPriv->current_bank=-1;
    if(qvisPriv->glyph_cache) {
       XYZ("qvisRestoreGState-InvalidateGlyphCache");
       qvisInvalidateGlyphCache(qvisPriv);
    } else {
	XYZ("qvisRestoreGState-NoGlyphCache");
    }
}

