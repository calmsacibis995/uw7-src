/*
 * @(#) m32Init.c 11.1 97/10/22
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
 * S001, 30-Aug-93, buckm
 *	Change cursor initialization some.
 *	Get rid of enforceProtocol stuff.
 *	Add calls to init and reset our own GC ops cache.
 * S002, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */
/*
 * m32Init.c
 *
 * Probe and Initialize the m32 Graphics Display Driver
 */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"					/* S002 */
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"
#include "m32Procs.h"

extern scoScreenInfo m32SysInfo;
extern VisualRec m32Visual8, m32Visual16;
extern nfbGCOps m32SolidPrivOps;

int m32Generation = -1;
int m32ScreenPrivateIndex;


/*
 * m32Probe() - test for Mach-32
 */
Bool
m32Probe(version, pReq)
	ddxDOVersionID version;
	ddxScreenRequest *pReq;
{
	/* check depth */
	switch (pReq->dfltDepth) {
	    case 8:
	    case 16:
#ifdef NOTYET
	    case 24:
#endif
		return ddxAddPixmapFormat(pReq->dfltDepth,
					  pReq->dfltBpp,pReq->dfltPad);
	    default:
		ErrorF("m32: unsupported screen depth: %d\n", pReq->dfltDepth);
		return FALSE;
	}
}

/*
 * m32InitHW()
 */
void
m32InitHW(pScreen)
	ScreenPtr pScreen;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScreen);

	pM32->hasVGA = (inw(M32_CONFIG_STAT_1) & 0x01) == 0;

	if (m32Generation > 1) {
		pM32->inGfxMode = TRUE;
	} else {
		pM32->inGfxMode = FALSE;
		m32SetGraphics(pScreen);
	}

	m32InitClip(pScreen);
}

/*
 * m32Init() - Mach-32 screen init
 */
Bool
m32Init(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
        grafData *pGraf = ddxActiveScreens[index]->pRequest->grafinfo;
	int depth       = ddxActiveScreens[index]->pRequest->dfltDepth;
	nfbScrnPrivPtr pNfb;
	m32ScrnPrivPtr pM32;
	VisualPtr pVis;
	int width, height, mmx, mmy;
	int swcursor;

	/* Get mode and monitor info */
	if ( !grafGetInt(pGraf, "PIXWIDTH",  &width)  ||
	     !grafGetInt(pGraf, "PIXHEIGHT", &height)) {
	    ErrorF("m32: can't find pixel info in grafinfo file.\n");
	    return FALSE;
	}

	mmx = 300; mmy = 300;  /* Reasonable defaults */
	grafGetInt(pGraf, "MON_WIDTH",  &mmx);
	grafGetInt(pGraf, "MON_HEIGHT", &mmy);

	/* allocate and attach screen private data */
	if (m32Generation != serverGeneration) {
		m32Generation = serverGeneration;
		m32ScreenPrivateIndex = AllocateScreenPrivateIndex();
		if (m32ScreenPrivateIndex < 0)
			return FALSE;
	}
	if ((pM32 = (m32ScrnPrivPtr)xalloc(sizeof(m32ScrnPriv))) == NULL)
		return FALSE;
	pScreen->devPrivates[m32ScreenPrivateIndex].ptr = 
					(unsigned char *)pM32;	/* S002 */

	swcursor = 0;
	grafGetInt(pGraf, "USESWCURSOR", &swcursor);
	pM32->useSWCurs = swcursor ? 1 : 0;

	/* store Mach-32 configuration info */
	pM32->fbBase = (pointer)0;

	/* init memory layout */
	if (! m32InitMem(pM32, width, height, depth)) {
		ErrorF("m32: not enough video memory on the");
		ErrorF(" Mach-32 card to run this graphics mode\n");
		return FALSE;
	}

	if (!nfbScreenInit(pScreen, width, height, mmx, mmy))
		return FALSE;

	pScreen->QueryBestSize = m32QueryBestSize;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &m32SolidPrivOps;
	pNfb->ValidateWindowPriv = m32ValidateWindowPriv;
	pNfb->BlankScreen	 = m32BlankScreen;
	pNfb->SetColor		 = NoopDDA;	/* see m32BlankScreen() */
	pNfb->LoadColormap	 = NoopDDA;	/* see m32BlankScreen() */
	pNfb->clip_count	 = 1;

	switch (depth) {
	    case 8:	pVis = &m32Visual8;	break;
	    case 16:	pVis = &m32Visual16;	break;
#ifdef NOTYET
	    case 24:	pVis = &m32Visual24;	break;
#endif
	}

	if (!nfbAddVisual(pScreen, pVis))
		return FALSE;

	if (pM32->te8Info.offset) {
		if (!m32InitTE8(pScreen))
			return FALSE;
		nfbInitializeText8(pScreen,
			M32_TE8_FONTS, M32_TE8_WIDTH, M32_TE8_HEIGHT,
			m32DownloadFont8, NULL);
	}

	m32InitHW(pScreen);

	m32CursorInitialize(pScreen);

	if (!cfbCreateDefColormap(pScreen))
		return FALSE;

	m32GCOpsCacheInit();

	/* give the sco layer our screen switch functions */
	scoSysInfoInit(pScreen, &m32SysInfo);

	/* set NFB runtime options */
	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);

	return TRUE;
}

/*
 * m32FreeScreenData()
 *
 * Anything you allocate in m32Init() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
void								/* S002 */
m32FreeScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScreen);

	m32GCOpsCacheReset();

	if (pM32->te8Info.offset) {
		nfbCloseText8(pScreen);
		m32FreeTE8(pScreen);
	}

	xfree(pM32);
}
