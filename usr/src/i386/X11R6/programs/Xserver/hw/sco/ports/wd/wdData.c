/*
 *  @(#) wdData.c 11.1 97/10/22
 *
 * Copyright (C) 1990-1993 The Santa Cruz Operation, Inc.  
 * All Rights Reserved.
 * 
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for 
 * purposes authorized by the license agreement provided they include 
 * this notice and the associated copyright notice with any such product.  
 * The information in this file is provided "AS IS" without warranty.
 */
/*
 *   wdData.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *      S001    Thu 05-Oct-1992 edb@sco.com
 *              implement tiled and stippled GC ops
 *      S002    Tue 09-Feb-1993 buckm@sco.com
 *              add data for 15,16,24 bit modes.
 *		add DrawFontText.
 *      S003    Tue 07-Apr-1993 edb@sco.com
 *		add DrawFontText15_24.
 *      S004    Thu 22-Apr-1993 edb@sco.com
 *              add wdStippledFillRects15_24 and wdOpStippledFillRect15_24
 *      S005    Wdn 15-May-1993 edb@sco.com
 *              wdDrawFontText15_24 will be called only when text8 is initialized
 *              initialize with genDrawFontText
 *      S006    Tue 25-May-1993 edb@sco.com
 *              Undo S005, rename wdDrawFontText to wdDrawFontText8
 *      S007    Thu 27-May-1993 edb@sco.com
 *		wdDrawMonoglyphs can do now 15,16,24 bits
 *      S008    Wdn 21-Jul-1993 buckm@sco.com
 *              We can now run while screen-switched.
 *	S009, 28-Oct-93, hiramc
 *		The run while screen-switch is conditional upon -DagaII,
 *		see also ./config/sco.cf and ./server/include/compiler.h
 */

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

#include "genProcs.h"
#include "genDefs.h"

#include "wdScrStr.h"
#include "wdProcs.h"

extern void NoopDDA() ;


VisualRec wdVisual = {
	0,			/* unsigned long	vid */
	PseudoColor,		/* short       class */
	6,			/* short       bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	8,			/* short	nplanes */
	0,			/* unsigned long	redMask */
	0,			/* unsigned long	greenMask */
	0,			/* unsigned long	blueMask */
	0,			/* int		offsetRed */
	0,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;

VisualRec wdVisual15 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	5,			/* short       bitsPerRGBValue */
	32,			/* short	ColormapEntries */
	15,			/* short	nplanes */
	0x7C00,			/* unsigned long	redMask */
	0x03E0,			/* unsigned long	greenMask */
	0x001F,			/* unsigned long	blueMask */
	10,			/* int		offsetRed */
	5,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;

VisualRec wdVisual16 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	6,			/* short       bitsPerRGBValue */
	64,			/* short	ColormapEntries */
	16,			/* short	nplanes */
	0xF800,			/* unsigned long	redMask */
	0x07E0,			/* unsigned long	greenMask */
	0x001F,			/* unsigned long	blueMask */
	11,			/* int		offsetRed */
	5,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;

VisualRec wdVisual24 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	8,			/* short       bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	24,			/* short	nplanes */
	0xFF0000,		/* unsigned long	redMask */
	0x00FF00,		/* unsigned long	greenMask */
	0x0000FF,		/* unsigned long	blueMask */
	16,			/* int		offsetRed */
	8,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;


nfbGCOps wdSolidPrivOps = {
	wdSolidFillRects,	/* void (* FillRects) () */
	wdSolidFS,		/* void (* FillSpans) () */
	wdSolidZeroSeg,	        /* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdTiledPrivOps = {
	wdTiledFillRects,	/* void (* FillRects) () */
	wdTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdStippledPrivOps = {
	wdStippledFillRects,	/* void (* FillRects) () */
	wdStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps wdOpStippledPrivOps = {
	wdOpStippledFillRects,	/* void (* FillRects) () */
	wdOpStippledFS,		/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbGCOps wdSolidPrivOps15 = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSeg,        /* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdTiledPrivOps15 = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdStippledPrivOps15 = {
	wdStippledFillRects15_24,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps wdOpStippledPrivOps15 = {
	wdOpStippledFillRects15_24,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbGCOps wdSolidPrivOps16 = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSeg,        /* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdTiledPrivOps16 = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdStippledPrivOps16 = {
	wdStippledFillRects15_24,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps wdOpStippledPrivOps16 = {
	wdOpStippledFillRects15_24,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbGCOps wdSolidPrivOps24 = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSeg,        /* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdTiledPrivOps24 = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wdStippledPrivOps24 = {
	wdStippledFillRects15_24,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps wdOpStippledPrivOps24 = {
	wdOpStippledFillRects15_24,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbWinOps wdWinOps = {
        wdCopyRect,           /* void (* CopyRect)() */
        wdDrawSolidRects,      /* void (* DrawSolidRects)() */
        wdDrawImage,           /* void (* DrawImage)() */
        wdDrawMonoImage,       /* void (* DrawMonoImage)() */
        wdDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        wdReadImage,           /* void (* ReadImage)() */
        wdDrawPoints,          /* void (* DrawPoints)() */
        wdTileRects,           /* void (* TileRects)() */
	wdDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	wdDrawFontText8 ,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        wdValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

nfbWinOps wdWinOps15 = {
        wdCopyRect15,           /* void (* CopyRect)() */
        wdDrawSolidRects15,     /* void (* DrawSolidRects)() */
        wdDrawImage15,          /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        wdReadImage15,          /* void (* ReadImage)() */
        wdDrawPoints15,         /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	wdDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */    /* S007 */
	wdDrawFontText15_24,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        wdValidateWindowGC15,   /* void (* ValidateWindowGC)() */
} ;

nfbWinOps wdWinOps16 = {
        wdCopyRect16,           /* void (* CopyRect)() */
        wdDrawSolidRects16,     /* void (* DrawSolidRects)() */
        wdDrawImage16,          /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        wdReadImage16,          /* void (* ReadImage)() */
        wdDrawPoints16,         /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	wdDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */    /* S007 */
	wdDrawFontText15_24,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        wdValidateWindowGC16,   /* void (* ValidateWindowGC)() */
} ;

nfbWinOps wdWinOps24 = {
        wdCopyRect24,           /* void (* CopyRect)() */
        wdDrawSolidRects24,     /* void (* DrawSolidRects)() */
        wdDrawImage24,          /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        wdReadImage24,          /* void (* ReadImage)() */
        wdDrawPoints24,         /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	wdDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */    /* S007 */
	wdDrawFontText15_24,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        wdValidateWindowGC24,   /* void (* ValidateWindowGC)() */
} ;


ddxScreenInfo wdScreenInfo = {
        wdProbe,		/* Bool (* screenProbe)() */
        wdInit,			/* Bool (* screenInit)() */
	"wd",			/* char *screenName */
} ;


scoScreenInfo wdSysInfo = {
    NULL,       	/* ScreenPtr pScreen  */
    wdSetGraphics,	/* void (*SetGraphics)() */
    wdSetText,		/* void (*SetText)() */
    wdSaveGState,      /* void (*SaveGState)() */
    wdRestoreGState,   /* void (*RestoreGState)() */
    wdCloseScreen,	/* void (*CloseScreen)() */
    TRUE,               /* Bool exposeScreen */
    TRUE,		/* Bool isConsole */
#ifdef usl
    TRUE,
#else
    agaIIScrSwStatus, 	/* Bool runSwitched */ 		/* S009 */
#endif
    XSCO_VERSION,	/* float version */
} ;
