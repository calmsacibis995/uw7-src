/*
 *	@(#) nteData.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S009, 09-Aug-93, hiramc
 *	The run while screen-switch is conditional upon -DagaII,
 *		see also ./config/sco.cf and ./server/include/compiler.h
 * S008, 21-Jul-93, buckm
 *	we can now run while screen-switched.
 * S007, 13-Jul-93, staceyc
 * 	keep win op 10 as gen until specificially used for text in init code
 * S006, 17-Jun-93, staceyc
 * 	stipples and tiles added
 * S005, 16-Jun-93, staceyc
 * 	fast text and clipping added
 * S004, 11-Jun-93, staceyc
 * 	mono images added
 * S003, 11-Jun-93, staceyc
 * 	special case 16 and 24 bit
 * S002, 09-Jun-93, staceyc
 * 	solid zero seg, solid fill, points, blit added
 * S001, 08-Jun-93, staceyc
 * 	correct string for driver name
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

#if NTE_BITS_PER_PIXEL == 8
VisualRec NTE(Visual) = {
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
};
#endif

#if NTE_BITS_PER_PIXEL == 16
VisualRec NTE(Visual) = {
	0,                      /* unsigned long        vid */
	TrueColor,              /* short       class */
	6,                      /* short       bitsPerRGBValue */
	64,                     /* short        ColormapEntries */
	16,                     /* short        nplanes */
	0xF800,                 /* unsigned long        redMask */
	0x07E0,                 /* unsigned long        greenMask */
	0x001F,                 /* unsigned long        blueMask */
	11,                     /* int          offsetRed */
	5,                      /* int          offsetGreen */
	0                       /* int          offsetBlue */
};
#endif

#if NTE_BITS_PER_PIXEL == 24
VisualRec NTE(Visual) = {
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
};
#endif

nfbGCOps NTE(SolidPrivOps) = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	NTE(SolidZeroSeg),	/* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
};

nfbGCOps NTE(TiledPrivOps) = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
};

nfbGCOps NTE(StippledPrivOps) = {
	NTE(StippledFillRects),	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
};

nfbGCOps NTE(OpStippledPrivOps) = {
	NTE(OpStippledFillRects), /* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
};


nfbWinOps NTE(WinOps) = {
	NTE(CopyRect),            /* void (* CopyRect)() */
	NTE(DrawSolidRects),      /* void (* DrawSolidRects)() */
	NTE(DrawImage),           /* void (* DrawImage)() */
	NTE(DrawMonoImage),       /* void (* DrawMonoImage)() */
	NTE(DrawOpaqueMonoImage), /* void (* DrawOpaqueMonoImage)() */
	NTE(ReadImage),           /* void (* ReadImage)() */
	NTE(DrawPoints),          /* void (* DrawPoints)() */
	NTE(TileRects),           /* void (* TileRects)() */
	NTE(DrawMonoGlyphs),	/* void (* DrawMonoGlyphs)() */
	genWinOp10, 		/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	NTE(SetClipRegions),	/* void (* SetClipRegions)() */
	NTE(ValidateWindowGC),    /* void (* ValidateWindowGC)() */
};

ddxScreenInfo NTE(ScreenInfo) = {
	NTE(Probe),		/* Bool (* screenProbe)() */
	NTE(Init),		/* Bool (* screenInit)() */
	NTE_SCREEN_NAME,	/* char *screenName 			*/
};


scoScreenInfo NTE(SysInfo) = {
	NULL,       	/* ScreenPtr pScreen  */
	NTE(SetGraphics),	/* void (*SetGraphics)() */
	NTE(SetText),		/* void (*SetText)() */
	NTE(SaveGState),      /* void (*SaveGState)() */
	NTE(RestoreGState),   /* void (*RestoreGState)() */
	NTE(FreeScreen),	/* void (*FreeScreen)() */
	TRUE,               /* Bool exposeScreen */
	TRUE,		/* Bool isConsole */
	TRUE, 	/* Bool runSwitched */
	XSCO_VERSION,	/* float version */
};
