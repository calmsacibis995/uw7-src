/*
 *	@(#)m32Screen.c	11.1	10/22/97	12:31:17
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 10-Sep-93, buckm
 *	Make sure hw cursor is off in SetText.
 * S002, 21-Sep-94, davidw 
 *	Correct compiler warnings.
 * S003, 04-Sep-97, hiramc
 *	no need for the ioctl on SW_VGA12 in gemini
 */
/*
 * m32Screen.c
 *
 * m32 screen procedures
 */
#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "pixmapstr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"                                       /* S002 vv*/
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"                                      /* S002 ^^*/
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"
#include "m32Procs.h"

#include <sys/types.h>
#ifdef usl
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

/*
 * m32BlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
m32BlankScreen(on, pScreen)
	int on;
	ScreenPtr pScreen;
{
	nfbScrnPrivPtr pNfb = NFB_SCREEN_PRIV(pScreen);

	if (pScreen->rootDepth > 8)
		return FALSE;

	if (on) {
		m32BlankColormap(pScreen);
		pNfb->SetColor     = NoopDDA;
		pNfb->LoadColormap = NoopDDA;
	} else {
		pNfb->SetColor     = m32SetColor;
		pNfb->LoadColormap = m32LoadColormap;
		m32RestoreColormap(pScreen);
	}

	return TRUE;
}

/*
 * m32SetGraphics(pScreen) - set screen into graphics mode
 */
void
m32SetGraphics(pScreen)
	ScreenPtr pScreen;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScreen);

	if (!pM32->inGfxMode) {
#if ! defined(usl)				/*	S003	*/
		if (pM32->hasVGA)
			ioctl(1, SW_VGA12, 0);
#endif
		grafExec(DDX_GRAFINFO(pScreen), "SetGraphics", NULL);
		m32InitCop(pScreen);
		pM32->inGfxMode = TRUE;
#ifdef usl
                m32BlankScreen(FALSE, pScreen);
#endif
	}
}

/*
 * m32SetText(pScreen) - set screen into text mode
 */
void
m32SetText(pScreen)
	ScreenPtr pScreen;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScreen);

	if (pM32->inGfxMode) {
		if (!pM32->useSWCurs)
			m32EnableCursor(FALSE);
		m32StopCop(pScreen);
		grafExec(DDX_GRAFINFO(pScreen), "SetText", NULL);
		pM32->inGfxMode = FALSE;
	}
}

/*
 * m32SaveGState(pScreen) - save graphics state
 */
void
m32SaveGState(pScreen)
	ScreenPtr pScreen;
{
	m32SaveCop(pScreen);
}

/*
 * m32RestoreGState(pScreen) - restore graphics state
 */
void
m32RestoreGState(pScreen)
	ScreenPtr pScreen;
{
	m32RestoreColormap(pScreen);
	m32RestoreCop(pScreen);
	m32RestoreClip(pScreen);
	m32RestoreMem(pScreen);
	m32RestoreCursor(pScreen);
}
