/*
 * @(#)svgaData.c 11.1
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

#include "svgaProcs.h"

VisualRec svgaVisual = {
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

VisualRec svgaVisual16 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	6,			/* short       bitsPerRGBValue */
	64,			/* short	ColormapEntries */
	16,			/* short	nplanes */
	0x0000F800,             /* unsigned long	redMask */
	0x000007E0,             /* unsigned long	greenMask */
	0x0000001F,             /* unsigned long	blueMask */
	11,			/* int		offsetRed */
	5,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;


VisualRec svgaVisual24 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	8,			/* short       bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	24,			/* short	nplanes */
	0xFF0000,               /* unsigned long	redMask */
	0x00FF00,               /* unsigned long	greenMask */
	0x0000FF,               /* unsigned long	blueMask */
	16,			/* int		offsetRed */
	8,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;


nfbGCOps svgaSolidPrivOps = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSegs,	/* void (* FillZeroSegs) () */
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps svgaTiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps svgaStippledPrivOps = {
	genStippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps svgaOpStippledPrivOps = {
	genOpStippledFillRects,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbWinOps svgaWinOps = {
        genCopyRect,            /* void (* CopyRect)() */
        genDrawSolidRects,      /* void (* DrawSolidRects)() */
        svgaDrawImage,           /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        svgaReadImage,           /* void (* ReadImage)() */
        genDrawPoints,          /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genDrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* SetClipRegions)() */
        svgaValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

nfbWinOps svgaWinOps16 = {
        genCopyRect,            /* void (* CopyRect)() */
        genDrawSolidRects,      /* void (* DrawSolidRects)() */
        svgaDrawImage16,        /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        svgaReadImage16,        /* void (* ReadImage)() */
        genDrawPoints,          /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genDrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* SetClipRegions)() */
        svgaValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

nfbWinOps svgaWinOps24 = {
        genCopyRect,            /* void (* CopyRect)() */
        genDrawSolidRects,      /* void (* DrawSolidRects)() */
        svgaDrawImage,           /* void (* DrawImage)() */
        genDrawMonoImage,       /* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        svgaReadImage,           /* void (* ReadImage)() */
        genDrawPoints,          /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genDrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* SetClipRegions)() */
        svgaValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

ddxScreenInfo svgaScreenInfo = {
        svgaSetup,		/* Bool (* screenSetup)() */
        svgaInit,		/* Bool (* screenInit)() */
	"svga",			/* char *screenName */
} ;


scoScreenInfo svgaSysInfo = {
    NULL,       	/* ScreenPtr pScreen  */
    svgaSetGraphics,	/* void (*SetGraphics)() */
    svgaSetText,		/* void (*SetText)() */
    svgaSaveGState,      /* void (*SaveGState)() */
    svgaRestoreGState,   /* void (*RestoreGState)() */
    svgaCloseScreen,	/* void (*CloseScreen)() */
    TRUE,               /* Bool exposeScreen */
    TRUE,		/* Bool isConsole */
    TRUE,		/* Bool runSwitched */
    XSCO_VERSION,	/* float version */
} ;
