/*
 *  @(#) wd33Data.c 11.1 97/10/22
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
 *   wd33Data.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Tue 17-Aug-1993	edb@sco.com
 *              Add DrawFontText8
 *	S002	28-Oct-93, hiramc
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

#include "wd33ScrStr.h"
#include "wd33Procs.h"

extern void NoopDDA() ;


VisualRec wd33Visual = {
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

VisualRec wd33Visual15 = {
        0,                      /* unsigned long        vid */
        TrueColor,              /* short       class */
        5,                      /* short       bitsPerRGBValue */
        32,                     /* short        ColormapEntries */
        15,                     /* short        nplanes */
        0x7C00,                 /* unsigned long        redMask */
        0x03E0,                 /* unsigned long        greenMask */
        0x001F,                 /* unsigned long        blueMask */
        10,                     /* int          offsetRed */
        5,                      /* int          offsetGreen */
        0                       /* int          offsetBlue */
} ;

VisualRec wd33Visual16 = {
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


nfbGCOps wd33SolidPrivOps = {
	wd33SolidFillRects,	/* void (* FillRects) () */
	wd33SolidFS,		/* void (* FillSpans) () */
	wd33SolidZeroSeg,       /* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wd33TiledPrivOps = {
	wd33TiledFillRects,	/* void (* FillRects) () */
	wd33TiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps wd33StippledPrivOps = {
	wd33StippledFillRects,	/* void (* FillRects) () */
	wd33StippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps wd33OpStippledPrivOps = {
	wd33OpStippledFillRects,	/* void (* FillRects) () */
	wd33OpStippledFS,		/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbWinOps wd33WinOps = {
        wd33CopyRect,           /* void (* CopyRect)() */
        wd33DrawSolidRects,     /* void (* DrawSolidRects)() */
        wd33DrawImage,          /* void (* DrawImage)() */
        wd33DrawMonoImage,       /* void (* DrawMonoImage)() */
        wd33DrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        wd33ReadImage,          /* void (* ReadImage)() */
        wd33DrawPoints,         /* void (* DrawPoints)() */
        wd33TileRects,          /* void (* TileRects)() */
	wd33DrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
#ifndef agaII
	wd33DrawFontText8 ,	/* void (* DrawFontText)() */
#else
	genWinOp10,		/* void (* Reserved) () */
#endif
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        wd33ValidateWindowGC,   /* void (* ValidateWindowGC)() */
} ;

ddxScreenInfo wd33ScreenInfo = {
        wd33Probe,		/* Bool (* screenProbe)() */
        wd33Init,			/* Bool (* screenInit)() */
	"wd33",			/* char *screenName */
} ;


scoScreenInfo wd33SysInfo = {
    NULL,       	/* ScreenPtr pScreen  */
    wd33SetGraphics,	/* void (*SetGraphics)() */
    wd33SetText,		/* void (*SetText)() */
    wd33SaveGState,      /* void (*SaveGState)() */
    wd33RestoreGState,   /* void (*RestoreGState)() */
    wd33CloseScreen,	/* void (*CloseScreen)() */
    TRUE,               /* Bool exposeScreen */
    TRUE,		/* Bool isConsole */
#ifdef usl
    TRUE,
#else
    agaIIScrSwStatus, 	/* Bool runSwitched */ 		/* S002 */
#endif
    XSCO_VERSION,	/* float version */
} ;
