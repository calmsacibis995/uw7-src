/*
 *	@(#)ctData.c	11.1	10/22/97	12:10:33
 *	@(#) ctData.c 12.1 95/05/09 
 *	ctData.c 9.3 93/07/21 SCOINC
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
 * Copyright (C) 1994 Double Click Imaging, Inc.
 *
 *	SCO	Modifications
 *
 *	S001    Wed Mar 01              1995    rogerv@sco.COM
 *	SCO-59-5741: turn off "run when switched" due to driver touching
 *	harware when switched away.  Fix this later.
 *
 *	S000	Thu Jul 14 16:06:24 PDT 1994	davidw@sco.COM
 *	- have Tbird behavior when -DagaII is used -
 *		agaIIScrSwStatus, genSolidZeroSeg, genWinOp10
 */

#ident "@(#) $Id$"

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

#include "ctProcs.h"

extern void NoopDDA() ;

#if (CT_BITS_PER_PIXEL == 8)
VisualRec CT(Visual8) = {
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
} ;
#endif /* (CT_BITS_PER_PIXEL == 8) */

#if (CT_BITS_PER_PIXEL == 16)
VisualRec CT(Visual15) = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short		class */
	5,			/* short		bitsPerRGBValue */
	32,			/* short		ColormapEntries */
	15,			/* shortu		nplanes */
	0x7C00,			/* unsigned long	redMask */
	0x03E0,			/* unsigned long	greenMask */
	0x001F,			/* unsigned long	blueMask */
	10,			/* int			offsetRed */
	5,			/* int			offsetGreen */
	0			/* int			offsetBlue */
} ;

VisualRec CT(Visual16) = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short		class */
	6,			/* short		bitsPerRGBValue */
	64,			/* short		ColormapEntries */
	16,			/* short		nplanes */
	0xf800,			/* unsigned long	redMask */
	0x07e0,			/* unsigned long	greenMask */
	0x001f,			/* unsigned long	blueMask */
	11,			/* int			offsetRed */
	5,			/* int			offsetGreen */
	0			/* int			offsetBlue */
} ;
#endif /* (CT_BITS_PER_PIXEL == 16) */

#if (CT_BITS_PER_PIXEL == 24)
VisualRec CT(Visual24) = {
	0,			/* unsigned long	vid */
	TrueColor,		/* short		class */
	8,			/* short		bitsPerRGBValue */
	256,			/* short		ColormapEntries */
	24,			/* short		nplanes */
	0xff0000,		/* unsigned long	redMask */
	0x00ff00,		/* unsigned long	greenMask */
	0x0000ff,		/* unsigned long	blueMask */
	16,			/* int			offsetRed */
	8,			/* int			offsetGreen */
	0			/* int			offsetBlue */
} ;
#endif /* (CT_BITS_PER_PIXEL == 24) */

/*
 * MS Windows Raster Operation Codes (from Programmer's Reference Volume 3,
 * Appendix A) mapped to X GC functions:
 */
unsigned long CT(PixelOps)[16] = {
	0x00000000L,	/* GXclear		(0) */
	0x00000088L,	/* GXand		(S & D) */
	0x00000044L,	/* GXandReverse		(S & ~D) */
	0x000000ccL,	/* GXcopy		(S) */
	0x00000022L,	/* GXandInverted	(~S & D) */
	0x000000aaL,	/* GXnoop		(D) */
	0x00000066L,	/* GXxor		(S ^ D) */
	0x000000eeL,	/* GXor			(S | D) */
	0x00000011L,	/* GXnor		(~S & ~D) */
	0x00000099L,	/* GXequiv		(~S ^ D) */
	0x00000055L,	/* GXinvert		(~D) */
	0x000000ddL,	/* GXorReverse		(S | ~D) */
	0x00000033L,	/* GXcopyInverted	(~S) */
	0x000000bbL,	/* GXorInverted		(~S | D) */
	0x00000077L,	/* GXnand		(~S | ~D) */
	0x000000ffL,	/* GXset		(1) */
};

unsigned long CT(PatternOps)[16] = {
	0x00000000L,	/* GXclear		(0) */
	0x000000a0L,	/* GXand		(P & D) */
	0x00000050L,	/* GXandReverse		(P & ~D) */
	0x000000f0L,	/* GXcopy		(P) */
	0x0000000aL,	/* GXandInverted	(~P & D) */
	0x000000aaL,	/* GXnoop		(D) */
	0x0000005aL,	/* GXxor		(P ^ D) */
	0x000000faL,	/* GXor			(P | D) */
	0x00000005L,	/* GXnor		(~P & ~D) */
	0x000000a5L,	/* GXequiv		(~P ^ D) */
	0x00000055L,	/* GXinvert		(~D) */
	0x000000f5L,	/* GXorReverse		(P | ~D) */
	0x0000000fL,	/* GXcopyInverted	(~P) */
	0x000000afL,	/* GXorInverted		(~P | D) */
	0x0000005fL,	/* GXnand		(~P | ~D) */
	0x000000ffL,	/* GXset		(1) */
};

unsigned long CT(MaskOps)[16] = {
	0x0000000aL,	/* GXclear		(D & ~P) */
	0x0000008aL,	/* GXand		(D & ~P) | ((S & D) & P) */
	0x0000004aL,	/* GXandReverse		(D & ~P) | ((S & ~D) & P) */
	0x000000caL,	/* GXcopy		(D & ~P) | (S & P) */
	0x0000002aL,	/* GXandInverted	(D & ~P) | ((~S & D) & P) */
	0x000000aaL,	/* GXnoop		(D) */
	0x0000006aL,	/* GXxor		(D & ~P) | ((S ^ D) & P) */
	0x000000eaL,	/* GXor			(D & ~P) | ((S OR D) & P) */
	0x0000001aL,	/* GXnor		(D & ~P) | ((~S & ~D) & P) */
	0x0000009aL,	/* GXequiv		(D & ~P) | ((~S ^ D) & P) */
	0x0000005aL,	/* GXinvert		(D ^ P) */
	0x000000daL,	/* GXorReverse		(D & ~P) | ((S | ~D) & P) */
	0x0000003aL,	/* GXcopyInverted	(D & ~P) | (~S & P) */
	0x000000baL,	/* GXorInverted		(D & ~P) | ((~S | D) & P) */
	0x0000007aL,	/* GXnand		(D & ~P) | ((~S | ~D) & P) */
	0x000000faL,	/* GXset		(D | P) */
};

nfbGCOps CT(SolidPrivOps) = {
	CT(SolidFillRects),	/* void (* FillRects) () */
	CT(SolidFS),		/* void (* FillSpans) () */
#ifdef agaII					/* S000 */
	genSolidZeroSeg,	/* void (* FillZeroSeg) () */
#else
	CT(SolidZeroSegs),	/* void (* FillZeroSegs) () */
#endif
	genSolidGCOp4,		/* void (* FillPolygons) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps CT(TiledPrivOps) = {
	CT(TiledFillRects),	/* void (* FillRects) () */
	CT(TiledFS),		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* FillPolygons) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6,		/* void (* Reserved) () */
} ;

nfbGCOps CT(StippledPrivOps) = {
	genStippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* FillPolygons) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbGCOps CT(OpStippledPrivOps) = {
	CT(OpStippledFillRects),/* void (* FillRects) () */
	CT(OpStippledFS),	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* FillPolygons) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6,	/* void (* Reserved) () */
} ;

nfbWinOps CT(WinOps) = {
	CT(CopyRect),		/* void (* CopyRect)() */
	CT(DrawSolidRects),	/* void (* DrawSolidRects)() */
	CT(DrawImage),		/* void (* DrawImage)() */
	CT(DrawMonoImage),	/* void (* DrawMonoImage)() */
	CT(DrawOpaqueMonoImage),/* void (* DrawOpaqueMonoImage)() */
	CT(ReadImage),		/* void (* ReadImage)() */
	CT(DrawPoints),		/* void (* DrawPoints)() */
	CT(TileRects),		/* void (* TileRects)() */
	CT(DrawMonoGlyphs),	/* void (* DrawMonoGlyphs)() */
#if defined(TBIRD_TEXT_INTERFACE)			/* S000 */
	genWinOp10,		/* void (* DrawFontText)() */
#else
	CT(DrawFontText),	/* void (* DrawFontText)() */
#endif
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	genWinOp14,		/* void (* SetClipRegions)() */
	CT(ValidateWindowGC),	/* void (* ValidateWindowGC)() */
} ;

ddxScreenInfo CT(ScreenInfo) = {
	CT(Probe),		/* Bool (* screenProbe)() */
	CT(Init),		/* Bool (* screenInit)() */
	CT_SCREEN_NAME,		/* char *screenName */
} ;


scoScreenInfo CT(SysInfo) = {
	NULL,			/* ScreenPtr pScreen  */
	CT(SetGraphics),	/* void (*SetGraphics)() */
	CT(SetText),		/* void (*SetText)() */
	CT(SaveGState),		/* void (*SaveGState)() */
	CT(RestoreGState),	/* void (*RestoreGState)() */
	CT(FreeScreen),		/* void (*FreeScreen)() */
	TRUE,			/* Bool exposeScreen */
	TRUE,			/* Bool isConsole */
	FALSE,			/* Bool runSwitched */
	XSCO_VERSION,		/* float version */
} ;
