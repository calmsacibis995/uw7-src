/*
 *	@(#) effScreen.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *
 * Modification History
 *
 * S018, 14-Dec-94 brianm
 *	changed S017 to use the code to get UnBlankScreen, rather
 *	than explicitly calling effBlankScreen()  Fixes bug SCO-59-4651
 * S017, 13-Oct-94 brianm
 *	added in the effBlankScreen(FALSE..) to effSetText().  restores screen
 *	when screen switching or exiting X.
 * S016, 22-Nov-93, staceyc
 * 	don't muck with colormap while screen is blanked, rework S014, not
 *	sure why that mod was necessary
 * S015, 27-Aug-93, buckm
 *	card_clip is now a BoxRec.
 * S014, 04-Dec-92, chrissc
 *	add effBlankScreen() to effSetText()
 * S013, 04-Dec-92, mikep
 *	make effBlankScreen() a little simpler
 * S012, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S011, 07-Jun-92, buckm
 *	oops: grafinfo SUCCESS != server Success.
 * S010, 04-Jun-92, mikep
 *	execute the grafinfo set text routine
 *	put the VGA into graphics mode if the grafinfo file says to
 * S009, 26-Sep-91, staceyc
 * 	save and restore VGA 8514 DAC state
 * S008, 24-Sep-91, staceyc
 * 	don't save/restore cache - nuke it instead!
 * S007, 05-Sep-91, staceyc
 * 	fix off-screen size calculation
 * S006, 04-Sep-91, staceyc
 * 	routines to save/restore off-screen card state
 * S005, 28-Aug-91, staceyc
 * 	general code cleanup
 * S004, 13-Aug-91, staceyc
 * 	add defs.h file
 * S003, 28-Jun-91, staceyc
 * 	pass init hardware a screen pointer
 * S002, 27-Jun-91, staceyc
 * 	restore colormap on screen acquire
 * S001, 26-Jun-91, staceyc
 * 	screen blanking implemented
 * S000, 24-Jun-91, staceyc
 *	some initial work
 */

#ifdef usl
#include <sys/types.h>
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "screenint.h"
#include "windowstr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern WindowPtr *WindowTable;

/*
 * effBlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
effBlankScreen(on, pScreen)
int on;
ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	codeType *routine;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

	effPriv->screen_blanked = FALSE;
	if (on)
	{
		effSetColor(0, 0, 0, 0, 0, pScreen);
		if(grafGetFunction(grafinfo, "BlankScreen", &routine))
			grafRunCode(routine, NULL);
		else
			EFF_OUTB(effPriv->eff_pal.mask, 0);
	}
	else
	{
		effRestoreColormap(pScreen);
		if(grafGetFunction(grafinfo, "UnblankScreen", &routine))
			grafRunCode(routine, NULL);
		else
			EFF_OUTB(effPriv->eff_pal.mask, 0xFF);
	}
	effPriv->screen_blanked = on;

	return TRUE;
}

void
effSetGraphics(pScreen)
ScreenPtr pScreen;
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

	effInitHW(pScreen);
	effBlankScreen(TRUE, pScreen);
	effRestoreColormap(pScreen);
	EFF_CLEAR_QUEUE(3);
	EFF_PLNWENBL(EFF_WPLANES);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_ONES);
}

void
effSetText(pScreen)
ScreenPtr pScreen;
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	codeType *routine;

	effDrawSolidRects(&effPriv->card_clip, 1, 0, GXcopy, ~0,
	    &WindowTable[pScreen->myNum]->drawable);

	/*
	 * if it has vga and we can talk to the card now then
	 * dac state
	 */
	if (! grafQuery(effPriv->graf_data, "IGNOREPROBE"))
		effRestoreDACState(pScreen);

	if(grafGetFunction(grafinfo, "UnBlankScreen", &routine)) /* vvv S018 */
		grafRunCode(routine, NULL);
	else
		EFF_OUTB(effPriv->eff_pal.mask, 0xFF);           /* ^^^ S018 */

	EFF_GPDONE();

	(void)grafExec(effPriv->graf_data, "SetText", NULL);
}

void
effSaveGState(
ScreenPtr pScreen)
{
	/*
	 * flush glyph cache in case another server is running
	 * this means the cache will have to be rebuilt by demand
	 * if/when the user switches back
	 */
	effGlFlushCache(pScreen);
}

void
effRestoreGState(
ScreenPtr pScreen)
{
}

void
effSaveDACState(
ScreenPtr pScreen)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	int i;

	EFF_CLEAR_QUEUE(2);
	EFF_OUTB(effPriv->eff_pal.read_addr, 0);
	for (i = 0; i < EFF_DAC_SIZE; ++i)
	{
		effPriv->dac_state[i].red = EFF_INB(effPriv->eff_pal.data);
		effPriv->dac_state[i].green = EFF_INB(effPriv->eff_pal.data);
		effPriv->dac_state[i].blue = EFF_INB(effPriv->eff_pal.data);
	}
}

void
effRestoreDACState(
ScreenPtr pScreen)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	int i;

	EFF_CLEAR_QUEUE(2);
	EFF_OUTB(effPriv->eff_pal.write_addr, 0);
	for (i = 0; i < EFF_DAC_SIZE; ++i)
	{
		EFF_OUTB(effPriv->eff_pal.data, effPriv->dac_state[i].red);
		EFF_OUTB(effPriv->eff_pal.data, effPriv->dac_state[i].green);
		EFF_OUTB(effPriv->eff_pal.data, effPriv->dac_state[i].blue);
	}
}

