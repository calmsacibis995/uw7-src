/*
 *	@(#) qvisData.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S005    Tue Sep 20 09:44:33 PDT 1994    davidw@sco.com
 *	- Use agaII flag.  genWinOp10 removed in Everest, use genDrawFontText.
 *	Stuck with genDrawFontText warnings because of TBIRD_TEXT_INTERFACE
 *	flag.  Also stuck with genTileRects/SolidZeroSeg warnings.
 *      S004    Mon Sep 19 17:20:27 PDT 1994    davidw@sco.com
 *	- TBIRD_TEXT_INTERFACE causes use of old defination to 
 *	qvisDrawMonoGlyphs.
 *	S003	Tue Aug 10 14:43:05 PDT 1993	hiramc@sco.COM
 *	- The run while screen-switch is conditional upon -DagaII,
 *		see also ./config/sco.cf and ./server/include/compiler.h
 *	S002	Wed Jul 21 10:05:35 PDT 1993	buckm@sco.com
 *	- We can now run while screen-switched.
 *	S001	Thu Jan 07 17:02:01 PST 1993	mikep@sco.com
 *	- Remove non-applicable comments
 *	S000	Tue Oct 06 21:54:50 PDT 1992	mikep@sco.com
 *	- Add qvisDrawBankedPoints()
 *
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * mikep     09/28/92  Put qvisCloseScreen back into qvisSysInfo.
 */

#define TBIRD_TEXT_INTERFACE					/* S005 */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "cursorstr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbProcs.h"

#include "gen/genProcs.h"
#include "gen/genDefs.h"

#include "qvisProcs.h"

extern unsigned char alu_to_hwrop[16] =
{0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

extern void NoopDDA();

VisualRec       qvisVisual =
{
    0,				/* unsigned long	vid */
    PseudoColor,		/* short       class */
    8,				/* short       bitsPerRGBValue */
    256,			/* short	ColormapEntries */
    8,				/* short	nplanes */
    0,				/* unsigned long	redMask */
    0,				/* unsigned long	greenMask */
    0,				/* unsigned long	blueMask */
    0,				/* int		offsetRed */
    0,				/* int		offsetGreen */
    0				/* int		offsetBlue */
};


nfbGCOps        qvisFlatSolidPrivOps =
{
    genSolidFillRects,		/* void (* FillRects) () */
    qvisSolidFillSpans,		/* void (* FillSpans) () */
    qvisSolidZeroSeg,		/* void (* FillZeroSeg) () */
    genSolidGCOp4,		/* void (* Reserved) () */
    genSolidGCOp5,		/* void (* Reserved) () */
    genSolidGCOp6,		/* void (* Reserved) () */
};

nfbGCOps        qvisBankedSolidPrivOps =
{
    genSolidFillRects,		/* void (* FillRects) () */
    genSolidFS,			/* void (* FillSpans) () */
    qvisSolidZeroSeg,		/* void (* FillZeroSeg) () */
    genSolidGCOp4,		/* void (* Reserved) () */
    genSolidGCOp5,		/* void (* Reserved) () */
    genSolidGCOp6,		/* void (* Reserved) () */
};

nfbGCOps        qvisTiledPrivOps =
{
    genTiledFillRects,		/* void (* FillRects) () */
    genTiledFS,			/* void (* FillSpans) () */
    genTiledGCOp3,		/* void (* Reserved) () */
    genTiledGCOp4,		/* void (* Reserved) () */
    genTiledGCOp5,		/* void (* Reserved) () */
    genTiledGCOp6,		/* void (* Reserved) () */
};

nfbGCOps        qvisStippledPrivOps =
{
    genStippledFillRects,	/* void (* FillRects) () */
    genStippledFS,		/* void (* FillSpans) () */
    genStippledGCOp3,		/* void (* Reserved) () */
    genStippledGCOp4,		/* void (* Reserved) () */
    genStippledGCOp5,		/* void (* Reserved) () */
    genStippledGCOp6,		/* void (* Reserved) () */
};

nfbGCOps        qvisOpStippledPrivOps =
{
    genOpStippledFillRects,	/* void (* FillRects) () */
    genOpStippledFS,		/* void (* FillSpans) () */
    genOpStippledGCOp3,		/* void (* Reserved) () */
    genOpStippledGCOp4,		/* void (* Reserved) () */
    genOpStippledGCOp5,		/* void (* Reserved) () */
    genOpStippledGCOp6,		/* void (* Reserved) () */
};

nfbWinOps       qvisGlyphCacheFlatWinOps =
{
    qvisCopyRect,		/* void (* CopyRect)() */
    qvisDrawSolidRects,		/* void (* DrawSolidRects)() */
    qvisDrawImage,		/* void (* DrawImage)() */
    qvisDrawMonoImage,		/* void (* DrawMonoImage)() */
    qvisDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
    qvisReadImage,		/* void (* ReadImage)() */
    qvisDrawPoints,		/* void (* DrawPoints)() */
    genTileRects,		/* void (* TileRects)() */
    qvisCachedDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
#ifdef agaII
    genWinOp10,			/* void (* Reserved) () */
#else
    genDrawFontText,		/* void (* DrawFontText) () */	/* S005 */
#endif
    genWinOp11,			/* void (* Reserved) () */
    genWinOp12,			/* void (* Reserved) () */
    genWinOp13,			/* void (* Reserved) () */
    genWinOp14,			/* void (* Reserved) () */
    qvisValidateWindowGC,	/* void (* ValidateWindowGC)() */
};

nfbWinOps       qvisFlatWinOps =
{
    qvisCopyRect,		/* void (* CopyRect)() */
    qvisDrawSolidRects,		/* void (* DrawSolidRects)() */
    qvisDrawImage,		/* void (* DrawImage)() */
    qvisDrawMonoImage,		/* void (* DrawMonoImage)() */
    qvisDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
    qvisReadImage,		/* void (* ReadImage)() */
    qvisDrawPoints,		/* void (* DrawPoints)() */
    genTileRects,		/* void (* TileRects)() */
    qvisUncachedDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
#ifdef agaII
    genWinOp10,			/* void (* Reserved) () */
#else
    genDrawFontText,		/* void (* DrawFontText) () */	/* S005 */
#endif
    genWinOp11,			/* void (* Reserved) () */
    genWinOp12,			/* void (* Reserved) () */
    genWinOp13,			/* void (* Reserved) () */
    genWinOp14,			/* void (* Reserved) () */
    qvisValidateWindowGC,	/* void (* ValidateWindowGC)() */
};

nfbWinOps       qvisBankedWinOps =
{
    qvisCopyRect,		/* void (* CopyRect)() */
    qvisDrawSolidRects,		/* void (* DrawSolidRects)() */
    qvisDrawBankedImage,		/* void (* DrawImage)() */
    genDrawMonoImage,		/* void (* DrawMonoImage)() */
    qvisDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
    qvisReadBankedImage,		/* void (* ReadImage)() */
    qvisDrawBankedPoints,		/* void (* DrawPoints)() */ /* S000 */
    genTileRects,		/* void (* TileRects)() */
    qvisUncachedDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
#ifdef agaII
    genWinOp10,			/* void (* Reserved) () */
#else
    genDrawFontText,		/* void (* DrawFontText) () */	/* S005 */
#endif
    genWinOp11,			/* void (* Reserved) () */
    genWinOp12,			/* void (* Reserved) () */
    genWinOp13,			/* void (* Reserved) () */
    genWinOp14,			/* void (* Reserved) () */
    qvisValidateWindowGC,	/* void (* ValidateWindowGC)() */
};

ddxScreenInfo   qvisScreenInfo =
{
    qvisProbe,			/* Bool (* screenProbe)() */
    qvisInit,			/* Bool (* screenInit)() */
    "qvis",			/* char *screenName */
};

scoScreenInfo   qvisSysInfo =
{
    NULL,			/* ScreenPtr pScreen  */
    qvisSetGraphics,		/* void (*SetGraphics)() */
    qvisSetText,			/* void (*SetText)() */
    qvisSaveGState,		/* void (*SaveGState)() */
    qvisRestoreGState,		/* void (*RestoreGState)() */
    qvisCloseScreen,		/* void (*CloseScreen)() */
    TRUE,			/* Bool exposeScreen */
    TRUE,			/* Bool isConsole */
#ifdef usl
    TRUE,
#else
    agaIIScrSwStatus,		/* Bool runSwitched */ 		/* S002 */
#endif
    (float) XSCO_VERSION,	/* float version */
};
