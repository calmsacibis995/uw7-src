/*
 * @(#) m32Data.c 11.1 97/10/22
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
 * S001, 31-Aug-93, buckm
 *	No more MiscOps.
 *	Add some tile and stipple ops.
 * S002, 21-Sep-94, davidw
 *      Correct compiler warnings. - TBIRD_TEXT_INTERFACE causes
 *      use of old defination to m32DrawMonoGlyphs/m32DrawFontText.
 *	Can't get rid of SolidZeroSeg warning.
 *
 */

#define TBIRD_TEXT_INTERFACE

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
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

#include "m32Procs.h"


VisualRec m32Visual8 = {
	0,			/* unsigned long	vid */
	PseudoColor,		/* short	class */
	6,			/* short	bitsPerRGBValue */
	256,			/* short	ColormapEntries */
	8,			/* short	nplanes */
	0,			/* unsigned long	redMask */
	0,			/* unsigned long	greenMask */
	0,			/* unsigned long	blueMask */
	0,			/* int		offsetRed */
	0,			/* int		offsetGreen */
	0			/* int		offsetBlue */
};

VisualRec m32Visual16 = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short	class */
	6,			/* short	bitsPerRGBValue */
	64,			/* short	ColormapEntries */
	16,			/* short	nplanes */
	0xF800,			/* unsigned long	redMask */
	0x07E0,			/* unsigned long	greenMask */
	0x001F,			/* unsigned long	blueMask */
	11,			/* int		offsetRed */
	5,			/* int		offsetGreen */
	0			/* int		offsetBlue */
};


nfbGCOps m32SolidPrivOps = {
	m32SolidFillRects,	/* void (* FillRects) () */
	m32SolidFS,		/* void (* FillSpans) () */
	m32SolidZeroSeg,	/* void (* FillZeroSegs) () */
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
};

nfbGCOps m32TiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
};

nfbGCOps m32StippledPrivOps = {
	m32StippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
};

nfbGCOps m32OpStippledPrivOps = {
	m32OpStippledFillRects,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
};


nfbWinOps m32WinOps8 = {
        m32CopyRect,		/* void (* CopyRect)() */
        m32DrawSolidRects,	/* void (* DrawSolidRects)() */
        m32DrawImage8,		/* void (* DrawImage)() */
        m32DrawMonoImage,	/* void (* DrawMonoImage)() */
        m32DrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        m32ReadImage8,		/* void (* ReadImage)() */
        m32DrawPoints,		/* void (* DrawPoints)() */
        m32TileRects8,		/* void (* TileRects)() */
	m32DrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	m32DrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved)() */
	genWinOp12,		/* void (* Reserved)() */
	genWinOp13,		/* void (* Reserved)() */
	m32SetClip,		/* void (* SetClipRegions)() */
        m32ValidateWindowGC,	/* void (* ValidateWindowGC)() */
};

nfbWinOps m32WinOps16 = {
        m32CopyRect,		/* void (* CopyRect)() */
        m32DrawSolidRects,	/* void (* DrawSolidRects)() */
        m32DrawImage16,		/* void (* DrawImage)() */
        m32DrawMonoImage,	/* void (* DrawMonoImage)() */
        m32DrawOpaqueMonoImage,	/* void (* DrawOpaqueMonoImage)() */
        m32ReadImage16,		/* void (* ReadImage)() */
        m32DrawPoints,		/* void (* DrawPoints)() */
        m32TileRects16,		/* void (* TileRects)() */
	m32DrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	m32DrawFontText,	/* void (* DrawFontText)() */
	genWinOp11,		/* void (* Reserved)() */
	genWinOp12,		/* void (* Reserved)() */
	genWinOp13,		/* void (* Reserved)() */
	m32SetClip,		/* void (* SetClipRegions)() */
        m32ValidateWindowGC,	/* void (* ValidateWindowGC)() */
};


ddxScreenInfo m32ScreenInfo = {
        m32Probe,		/* Bool (* screenProbe)() */
        m32Init,		/* Bool (* screenInit)() */
	"m32",			/* char *screenName */
};

scoScreenInfo m32SysInfo = {
	NULL,			/* ScreenPtr pScreen  */
	m32SetGraphics,		/* void (*SetGraphics)() */
	m32SetText,		/* void (*SetText)() */
	m32SaveGState,		/* void (*SaveGState)() */
	m32RestoreGState,	/* void (*RestoreGState)() */
	m32FreeScreen,		/* void (*FreeScreen)() */
	TRUE,			/* Bool exposeScreen */
	TRUE,			/* Bool isConsole */
#ifdef usl
        TRUE,
#else
	agaIIScrSwStatus,	/* Bool runSwitched */
#endif
	XSCO_VERSION,		/* float version */
};


unsigned short m32RasterOp[16] = {
	0x0001,		/* GXclear		*/
	0x000C,		/* GXand		*/
	0x000D,		/* GXandReverse		*/
	0x0007,		/* GXcopy		*/
	0x000E,		/* GXandInverted	*/
	0x0003,		/* GXnoop		*/
	0x0005,		/* GXxor		*/
	0x000B,		/* GXor			*/
	0x000F,		/* GXnor		*/
	0x0006,		/* GXequiv		*/
	0x0000,		/* GXinvert		*/
	0x000A,		/* GXorReverse		*/
	0x0004,		/* GXcopyInverted	*/
	0x0009,		/* GXorInverted		*/
	0x0008,		/* GXnand		*/
	0x0002,		/* GXset		*/
};
