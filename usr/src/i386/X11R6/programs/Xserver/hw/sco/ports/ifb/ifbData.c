/*
 * @(#) ifbData.c 12.1 95/05/09 
 *
 * Copyright (C) 1992-1993 The Santa Cruz Operation, Inc.
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
 * ifbData.c
 *
 * ifb data structures.
 */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "cursorstr.h"

#include "ddxScreen.h"
#include "scoext.h"

#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbProcs.h"

#include "genDefs.h"
#include "genProcs.h"

#include "ifbProcs.h"

nfbGCOps ifbSolidPrivOps = {
	genSolidFillRects,	/* void (* FillRects) () */
	genSolidFS,		/* void (* FillSpans) () */
	genSolidZeroSeg,	/* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6		/* void (* Reserved) () */
};

nfbGCOps ifbTiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6		/* void (* Reserved) () */
};

nfbGCOps ifbStippledPrivOps = {
	genStippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6	/* void (* Reserved) () */
};

nfbGCOps ifbOpStippledPrivOps = {
	genOpStippledFillRects,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6	/* void (* Reserved) () */
};

nfbWinOps ifbWinOps1 = {
        genCopyRect,		/* void (* CopyRect)() */
        genDrawSolidRects,	/* void (* DrawSolidRects)() */
        ifbDrawImage1,		/* void (* DrawImage)() */
        genDrawMonoImage,	/* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        ifbReadImage1,		/* void (* ReadImage)() */
        genDrawPoints,		/* void (* DrawPoints)() */
        genTileRects,		/* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genWinOp10,		/* void (* Reserved) () */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        ifbValidateWindowGC	/* void (* ValidateWindowGC)() */
};

nfbWinOps ifbWinOps8 = {
        genCopyRect,		/* void (* CopyRect)() */
        genDrawSolidRects,	/* void (* DrawSolidRects)() */
        ifbDrawImage8,		/* void (* DrawImage)() */
        genDrawMonoImage,	/* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        ifbReadImage8,		/* void (* ReadImage)() */
        genDrawPoints,		/* void (* DrawPoints)() */
        genTileRects,		/* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genWinOp10,		/* void (* Reserved) () */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        ifbValidateWindowGC	/* void (* ValidateWindowGC)() */
};

nfbWinOps ifbWinOps16 = {
        genCopyRect,		/* void (* CopyRect)() */
        genDrawSolidRects,	/* void (* DrawSolidRects)() */
        ifbDrawImage16,		/* void (* DrawImage)() */
        genDrawMonoImage,	/* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        ifbReadImage16,		/* void (* ReadImage)() */
        genDrawPoints,		/* void (* DrawPoints)() */
        genTileRects,		/* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genWinOp10,		/* void (* Reserved) () */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        ifbValidateWindowGC	/* void (* ValidateWindowGC)() */
};

nfbWinOps ifbWinOps32 = {
        genCopyRect,		/* void (* CopyRect)() */
        genDrawSolidRects,	/* void (* DrawSolidRects)() */
        ifbDrawImage32,		/* void (* DrawImage)() */
        genDrawMonoImage,	/* void (* DrawMonoImage)() */
        genDrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        ifbReadImage32,		/* void (* ReadImage)() */
        genDrawPoints,		/* void (* DrawPoints)() */
        genTileRects,		/* void (* TileRects)() */
	genDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	genWinOp10,		/* void (* Reserved) () */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* Reserved) () */
        ifbValidateWindowGC	/* void (* ValidateWindowGC)() */
};

VisualRec ifbVisual = {
	0,			/* unsigned long	vid */
	PseudoColor,		/* short		class */
	8,			/* short		bitsPerRGBValue */
	256,			/* short		ColormapEntries */
	8,			/* short		nplanes */
	0,			/* unsigned long	redMask */
	0,			/* unsigned long	greenMask */
	0,			/* unsigned long	blueMask */
	0,			/* int			offsetRed */
	0,			/* int			offsetGreen */
	0			/* int			offsetBlue */
};

ddxScreenInfo ifbScreenInfo = {
        ifbProbe,               /* Bool (* screenProbe)() */
        ifbInit,                /* Bool (* screenInit)() */
	"ifb",			/* char *screenName */
};

scoScreenInfo ifbSysInfo =
{
    NULL,		/* ScreenPtr pScreen		*/
    NoopDDA,		/* void (*SetGraphics)()	*/
    NoopDDA,		/* void (*SetText)()		*/
    NoopDDA,		/* void (*SaveGState)()		*/
    NoopDDA,		/* void (*RestoreGState)()	*/
    ifbCloseScreen,	/* void (*CloseScreen)()	*/
    FALSE,		/* Bool exposeScreen		*/
    FALSE,		/* Bool isConsole		*/
    TRUE,		/* Bool runSwitched		*/
    XSCO_VERSION,	/* float version		*/
};
