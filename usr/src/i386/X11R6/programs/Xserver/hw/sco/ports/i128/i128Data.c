/*
 * @(#) i128Data.c 11.1 97/10/22
 *
 * Copyright (C) 1990-1996 The Santa Cruz Operation, Inc.  
 * All Rights Reserved.
 * 
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for 
 * purposes authorized by the license agreement provided they include 
 * this notice and the associated copyright notice with any such product.  
 * The information in this file is provided "AS IS" without warranty.
 * 
 * Modification History
 *
 * S000, 30-Apr-96, davidw@sco.com
 *	i128TiledFS, i128TiledFillRects, i128TileRects - special i128 engine 
 *	code in these routines is broken. Works fine using tiles with two 
 *	colors found in UTS and x11perf testing.  But running xdt3 
 *	View->Names displays tiles with > 2 colors incorrectly.  Code could
 *	be added back in as optimization for x11perf but for now use gen 
 *	routines.
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

#include "i128Procs.h"

extern void NoopDDA() ;


VisualRec i128Visual = {
     0,                         /* unsigned long	vid */
     PseudoColor,		/* short       class */
     8,                         /* short       bitsPerRGBValue */
     256,			/* short	ColormapEntries */
     8,                         /* short	nplanes */
     0,                         /* unsigned long	redMask */
     0,                         /* unsigned long	greenMask */
     0,                         /* unsigned long	blueMask */
     0,                         /* int		offsetRed */
     0,                         /* int		offsetGreen */
     0                          /* int		offsetBlue */
} ;

VisualRec i128Visual16 = {
     0,                         /* unsigned long	vid */
     TrueColor,                 /* short		class */
     6,                         /* short		bitsPerRGBValue */
     64,			/* short		ColormapEntries */
     16,			/* short		nplanes */
     0x0000F800,		/* unsigned long	redMask */
     0x000007E0,		/* unsigned long	greenMask */
     0x0000001F,		/* unsigned long	blueMask */
     11,			/* int			offsetRed */
     5,                         /* int			offsetGreen */
     0                          /* int			offsetBlue */
};


VisualRec i128Visual24 = {
     0,                         /* unsigned long	vid */
     TrueColor,                 /* short       class */
     8,                         /* short       bitsPerRGBValue */
     256,			/* short	ColormapEntries */
     24,			/* short	nplanes */
     0xFF0000,                  /* unsigned long	redMask */
     0x00FF00,                  /* unsigned long	greenMask */
     0x0000FF,                  /* unsigned long	blueMask */
     16,			/* int		offsetRed */
     8,                         /* int		offsetGreen */
     0                          /* int		offsetBlue */
} ;


nfbGCMiscOps i128SolidMiscOps = {
     i128PolyZeroPtPtSegs,      /* void (* PolyZeroSeg) */
};

typedef void (*FillZeroSegsPtr)();

nfbGCOps i128SolidPrivOps = {
     i128SolidFillRects,        /* void (* FillRects) () */
     i128SolidFS,		/* void (* FillSpans) () */
     (FillZeroSegsPtr)i128SolidZeroSeg, /* void (* FillZeroSegs) () */
     genSolidGCOp4,		/* void (* FillPolygons) () */
     genSolidGCOp5,		/* void (* Reserved) () */
     genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps i128TiledPrivOps = {
     genTiledFillRects,         /* void (* FillRects) () */
     genTiledFS,		/* void (* FillSpans) () */
     genTiledGCOp3,		/* void (* Reserved) () */
     genTiledGCOp4,		/* void (* FillPolygons) () */
     genTiledGCOp5,		/* void (* Reserved) () */
     genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps i128StippledPrivOps = {
     i128StippledFillRects,	/* void (* FillRects) () */
     i128StippledFS,		/* void (* FillSpans) () */
     genStippledGCOp3,          /* void (* Reserved) () */
     genStippledGCOp4,          /* void (* FillPolygons) () */
     genStippledGCOp5,          /* void (* Reserved) () */
     genStippledGCOp6,          /* void (* Reserved) () */
} ;

nfbGCOps i128OpStippledPrivOps = {
     i128OpStippledFillRects,	/* void (* FillRects) () */
     i128OpStippledFS,          /* void (* FillSpans) () */
     genOpStippledGCOp3,	/* void (* Reserved) () */
     genOpStippledGCOp4,	/* void (* FillPolygons) () */
     genOpStippledGCOp5,	/* void (* Reserved) () */
     genOpStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbWinOps i128WinOps = {
     i128CopyRect,              /* void (* CopyRect)() */
     i128DrawSolidRects,        /* void (* DrawSolidRects)() */
     i128DrawImage,             /* void (* DrawImage)() */
     i128DrawMonoImage,         /* void (* DrawMonoImage)() */
     i128DrawOpaqueMonoImage,   /* void (* DrawOpaqueMonoImage)() */
     i128ReadImage,             /* void (* ReadImage)() */
     i128DrawPoints,            /* void (* DrawPoints)() */
     genTileRects,              /* void (* TileRects)() */
     i128DrawMonoGlyphs,        /* void (* DrawMonoGlyphs)() */
     i128DrawFontText,          /* void (* DrawFontText)() */
     genWinOp11,		/* void (* Reserved) () */
     genWinOp12,		/* void (* Reserved) () */
     genWinOp13,		/* void (* Reserved) () */
     i128SetClipRegions,        /* void (* SetClipRegions)() */
     i128ValidateWindowGC,      /* void (* ValidateWindowGC)() */
} ;


ddxScreenInfo i128ScreenInfo = {
     i128Setup,                 /* Bool (* screenSetup)() */
     i128Init,                  /* Bool (* screenInit)() */
     "i128",			/* char *screenName */
} ;


scoScreenInfo i128SysInfo = {
     NULL,                      /* ScreenPtr pScreen  */
     i128SetGraphics,           /* void (*SetGraphics)() */
     i128SetText,		/* void (*SetText)() */
     i128SaveGState,            /* void (*SaveGState)() */
     i128RestoreGState,         /* void (*RestoreGState)() */
     i128CloseScreen,           /* void (*CloseScreen)() */
     TRUE,                      /* Bool exposeScreen */
     TRUE,                      /* Bool isConsole */
     TRUE,                      /* Bool runSwitched */
     XSCO_VERSION,              /* float version */
} ;
