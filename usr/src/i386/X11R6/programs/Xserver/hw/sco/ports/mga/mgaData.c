/*
 * @(#) mgaData.c 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right
 * to use, modify, and incorporate this code into other products for purposes
 * authorized by the license agreement provided they include this notice
 * and the associated copyright notice with any such product.  The
 * information in this file is provided "AS IS" without warranty.
 *
 */
/*
 *	SCO	Modifications
 *
 *	S006	Wed Jun 28 10:09:42 PDT 1995	brianm@sco.com
 *		modified the 16 bit modes to 15 bit.
 *	S005	Thu Jun  1 16:53:54 PDT 1995	brianm@sco.com
 *		Added in modifications from Matrox.
 *	S004	Mon Oct 24 22:14:59 1994	kylec@sco.COM
 *		some of the alu's don't need read access to
 *		to vram.  This is a significant performance
 *		improvement.
 *	S003	Thu May 26 10:55:05 PDT 1994	hiramc@sco.COM
 *		alus GXnor and GXnand reversed because of error in
 *		the comment in ./X11/include/X.h
 *	S002	Wed May 25 16:40:45 PDT 1994	hiramc@sco.COM
 *		Can't use ZeroSegs in Tbird, only Everest.
 *	S001	Wed May 25 09:19:23 PDT 1994	hiramc@sco.COM
 *	- have TBIRD behavior when -DagaII
 *		agaIIScrSwStatus and genWinOp10 from compiler.h
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

#include "mgaProcs.h"

#ifndef usl
extern void NoopDDA() ;
#endif

/* 3 visuals supported, 8, 15, and 24 bits */ /* S006 */

VisualRec mgaVisual8 = {
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

VisualRec mgaVisual15 = { 	/* S006 */
	0,			/* unsigned long	vid */
	TrueColor,		/* short       class */
	5,			/* short       bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	15,			/* short	nplanes */ /* S006 */
	0x7c00,			/* unsigned long	redMask */
	0x03e0,			/* unsigned long	greenMask */
	0x001f,			/* unsigned long	blueMask */
	10,			/* int		offsetRed */
	5,			/* int		offsetGreen */
	0			/* int		offsetBlue */
} ;

VisualRec mgaVisual24 = {
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


nfbGCOps mgaSolidPrivOps = {
	mgaSolidFillRects,	/* void (* FillRects) () */
	mgaSolidFS,		/* void (* FillSpans) () */
#ifdef agaII			/*	S002	*/
	mgaSolidZeroSeg,	/* void (* FillZeroSeg) () */
#else
	mgaSolidZeroSegs,	/* void (* FillZeroSegs) () */
#endif
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

/* there is no way to replicate tiles, so let gen do it */

nfbGCOps mgaTiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps mgaStippledPrivOps = {
	mgaStippledFillRects,	/* void (* FillRects) () */
	mgaStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps mgaOpStippledPrivOps = {
	mgaOpStippledFillRects,	/* void (* FillRects) () */
	mgaOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;


nfbWinOps mgaWinOps = {
        mgaCopyRect,            /* void (* CopyRect)() */
        mgaDrawSolidRects,      /* void (* DrawSolidRects)() */
        mgaDrawImage,           /* void (* DrawImage)() */
        mgaDrawMonoImage,       /* void (* DrawMonoImage)() */
        mgaDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        mgaReadImage,           /* void (* ReadImage)() */
        mgaDrawPoints,          /* void (* DrawPoints)() */
        genTileRects,           /* void (* TileRects)() */
	mgaDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
#if defined(TBIRD_TEXT_INTERFACE)
	mgaDrawFontText,	/* void (* DrawFontText)() */
	/*genWinOp10,	/* void (* DrawFontText)() */
#else
	mgaDrawFontText,	/* void (* DrawFontText)() */
#endif	/*	TBIRD_TEXT_INTERFACE	*/
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	mgaSetClipRegions,	/* void (* SetClipRegions)() */
        mgaValidateWindowGC,    /* void (* ValidateWindowGC)() */
} ;

ddxScreenInfo mgaScreenInfo = {
        mgaProbe,		/* Bool (* screenProbe)() */
        mgaInit,		/* Bool (* screenInit)() */
	"mga",			/* char *screenName */
} ;

scoScreenInfo mgaSysInfo = {
    NULL,       	/* ScreenPtr pScreen  */
    mgaSetGraphics,	/* void (*SetGraphics)() */
    mgaSetText,		/* void (*SetText)() */
    mgaSaveGState,      /* void (*SaveGState)() */
    mgaRestoreGState,   /* void (*RestoreGState)() */
    mgaFreeScreen,	/* void (*FreeScreen)() */
    TRUE,               /* Bool exposeScreen */
    TRUE,		/* Bool isConsole */
#ifdef usl
    TRUE,
#else
    agaIIScrSwStatus, 	/* Bool runSwitched */	/*	S001	*/
#endif
    XSCO_VERSION,	/* float version */
} ;

/* alligned alu values for oring in to dwgctl */

unsigned long mgaALU[16] =
{
	0x00000,	/* clear (0) */			/* S004 */
	0x80010,	/* src AND dst */
	0x40010,	/* src AND NOT dst */
	0xc0000,	/* src */			/* S004 */
	0x20010,	/* NOT src AND dst */
	0xa0010,	/* dst */
	0x60010,	/* src XOR dst */
	0xe0010,	/* src OR dst */
	0x10010,	/* NOT src OR NOT dst */	/*	S003	*/
	0x90010,	/* NOT src XOR dst */
	0x50010,	/* NOT dst */
	0xd0010,	/* src OR NOT dst */
	0x30000,	/* NOT src */			/* S004 */
	0xb0010,	/* NOT src OR dst */
	0x70010,	/* NOT src AND NOT dst */	/*	S003	*/
	0xf0000		/* set (1)  */			/* S004 */
};

