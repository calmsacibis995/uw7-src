/*
 * @(#) xxxData.c 11.1 97/10/22
 *
 * Copyright (C) 1990-1994 The Santa Cruz Operation, Inc.  
 * All Rights Reserved.
 * 
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for 
 * purposes authorized by the license agreement provided they include 
 * this notice and the associated copyright notice with any such product.  
 * The information in this file is provided "AS IS" without warranty.
 * 
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

#include "gen/genProcs.h"
#include "gen/genDefs.h"

#include "xxxProcs.h"

VisualRec xxxVisual = {
	0,			/* unsigned long	vid */
	PseudoColor,		/* short       class */
	8,			/* short       bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	8,			/* short	nplanes */
	0,			/* unsigned long	redMask */
	0,			/* unsigned long	greenMask */
	0,			/* unsigned long	blueMask */
	0,			/* int		offsetRed */
	0,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;



nfbGCOps xxxSolidPrivOps = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSegs,	/* void (* FillZeroSegs) () */
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps xxxTiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps xxxStippledPrivOps = {
	genStippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps xxxOpStippledPrivOps = {
	genOpStippledFillRects,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbWinOps xxxWinOps = {
        genCopyRect,            /* void (* CopyRect)() */
        genDrawSolidRects,      /* void (* DrawSolidRects)() */
        xxxDrawImage,           /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        xxxReadImage,           /* void (* ReadImage)() */
        genDrawPoints,          /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genDrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* SetClipRegions)() */
        xxxValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

ddxScreenInfo xxxScreenInfo = {
        xxxSetup,		/* Bool (* screenSetup)() */
        xxxInit,		/* Bool (* screenInit)() */
	"xxx",			/* char *screenName */
} ;


scoScreenInfo xxxSysInfo = {
    NULL,       	/* ScreenPtr pScreen  */
    xxxSetGraphics,	/* void (*SetGraphics)() */
    xxxSetText,		/* void (*SetText)() */
    xxxSaveGState,      /* void (*SaveGState)() */
    xxxRestoreGState,   /* void (*RestoreGState)() */
    xxxCloseScreen,	/* void (*CloseScreen)() */
    TRUE,               /* Bool exposeScreen */
    TRUE,		/* Bool isConsole */
    TRUE,		/* Bool runSwitched */
    XSCO_VERSION,	/* float version */
} ;
