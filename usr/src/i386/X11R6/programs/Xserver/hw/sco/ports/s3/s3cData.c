/*
 *	@(#)s3cData.c	6.1	3/20/96	10:23:08
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * SCO Modification History
 *
 * S007, 09-Aug-93, hiramc
 *	The run while screen-switch is conditional upon -DagaII,
 *		see also ./config/sco.cf and ./server/include/compiler.h
 * S006, 21-Jul-93, buckm
 * 	we can now run while screen-switched.
 * S005, 17-May-93, staceyc
 * 	support for multiheaded S3 cards
 * S004, 11-May-93, staceyc
 * 	include file cleanup
 * S003	Sat Dec 05 06:22:44 PST 1992	mikep@sco.com
 *	No such silly beastie as s3cCmap.h anymore.
 * S002	Thu Oct 29 17:34:10 PST 1992	mikep@sco.com
 *	Add 16 bpp structures.  You don't want to modify these
 *	in s3 init code
 * S001	Fri Aug 28 15:19:30 PDT 1992	hiramc@sco.COM
 *	remove all previous Modification history here.
 *	Remove the #include "cfb.h" - it isn't needed
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

extern void 	NoopDDA();

VisualRec S3CNAME(Visual) = 
{
	0,			/* unsigned long	vid 		*/
	PseudoColor,		/* short       		class 		*/
	6,			/* short       		bitsPerRGBValue */
	256,			/* short		ColormapEntries */
	8,			/* short		nplanes 	*/
	0,			/* unsigned long	redMask 	*/
	0,			/* unsigned long	greenMask 	*/
	0,			/* unsigned long	blueMask 	*/
	0,			/* int			offsetRed 	*/
	0,			/* int			offsetGreen 	*/
	0			/* int			offsetBlue 	*/
};


nfbGCOps S3CNAME(SolidPrivOps) = 
{
	genSolidFillRects,	/* void (* FillRects) () S013 vvv 	*/
	genSolidFS,		/* void (* FillSpans) () 		*/
	S3CNAME(FillZeroSeg),	/* void (* FillZeroSeg) () 		*/
	genSolidGCOp4,		/* void (* Reserved) () 		*/
	genSolidGCOp5,		/* void (* Reserved) () 		*/
	genSolidGCOp6		/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(TiledPrivOps) = 
{
	genTiledFillRects,	/* void (* FillRects) () 		*/
	genTiledFS,		/* void (* FillSpans) () 		*/
	genTiledGCOp3,		/* void (* Reserved) () 		*/
	genTiledGCOp4,		/* void (* Reserved) () 		*/
	genTiledGCOp5,		/* void (* Reserved) () 		*/
	genTiledGCOp6		/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(StippledPrivOps) = 
{
	S3CNAME(StippledFillRects),/* void (* FillRects) () 		*/
	genStippledFS,		/* void (* FillSpans) () 		*/
	genStippledGCOp3,	/* void (* Reserved) () 		*/
	genStippledGCOp4,	/* void (* Reserved) () 		*/
	genStippledGCOp5,	/* void (* Reserved) () 		*/
	genStippledGCOp6	/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(OpStippledPrivOps) = 
{
	S3CNAME(OpStippledFillRects),/* void (* FillRects) () 		*/
	genOpStippledFS,	/* void (* FillSpans) () 		*/
	genOpStippledGCOp3,	/* void (* Reserved) () 		*/
	genOpStippledGCOp4,	/* void (* Reserved) () 		*/
	genOpStippledGCOp5,	/* void (* Reserved) () 		*/
	genOpStippledGCOp6	/* void (* Reserved) () 		*/
};


nfbWinOps S3CNAME(WinOps) = 
{
        S3CNAME(CopyRect),	/* void (* CopyRect)() 			*/
        S3CNAME(DrawSolidRects),/* void (* DrawSolidRects)() 		*/
        S3CNAME(DrawImage),	/* void (* DrawImage)() 		*/
        S3CNAME(DrawMonoImage),	/* void (* DrawMonoImage)() 		*/
        S3CNAME(DrawOpaqueMonoImage),/* void (* DrawOpaqueMonoImage)() 	*/
        S3CNAME(ReadImage),	/* void (* ReadImage)() 		*/
        S3CNAME(DrawPoints),	/* void (* DrawPoints)() 		*/
        S3CNAME(TileRects),	/* void (* TileRects)() 		*/
	S3CNAME(DrawMonoGlyphs),/* void (* DrawMonoGlyphs)() 		*/
	genWinOp10,		/* void (* Reserved) () 		*/
	genWinOp11,		/* void (* Reserved) () 		*/
	genWinOp12,		/* void (* Reserved) () 		*/
	genWinOp13,		/* void (* Reserved) () 		*/
	genWinOp14,		/* void (* Reserved) () 		*/
        S3CNAME(ValidateWindowGC)/* void (* ValidateWindowGC)() 		*/
};


nfbGCOps S3CNAME(SolidPrivOps16) = 
{
	genSolidFillRects,	/* void (* FillRects) () S013 vvv 	*/
	genSolidFS,		/* void (* FillSpans) () 		*/
	S3CNAME(FillZeroSeg16),	/* void (* FillZeroSeg) () 		*/
	genSolidGCOp4,		/* void (* Reserved) () 		*/
	genSolidGCOp5,		/* void (* Reserved) () 		*/
	genSolidGCOp6		/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(TiledPrivOps16) = 
{
	genTiledFillRects,	/* void (* FillRects) () 		*/
	genTiledFS,		/* void (* FillSpans) () 		*/
	genTiledGCOp3,		/* void (* Reserved) () 		*/
	genTiledGCOp4,		/* void (* Reserved) () 		*/
	genTiledGCOp5,		/* void (* Reserved) () 		*/
	genTiledGCOp6		/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(StippledPrivOps16) = 
{
	S3CNAME(StippledFillRects16),/* void (* FillRects) () 		*/
	genStippledFS,		/* void (* FillSpans) () 		*/
	genStippledGCOp3,	/* void (* Reserved) () 		*/
	genStippledGCOp4,	/* void (* Reserved) () 		*/
	genStippledGCOp5,	/* void (* Reserved) () 		*/
	genStippledGCOp6	/* void (* Reserved) () 		*/
};

nfbGCOps S3CNAME(OpStippledPrivOps16) = 
{
	S3CNAME(OpStippledFillRects16),/* void (* FillRects) ()		*/
	genOpStippledFS,	/* void (* FillSpans) () 		*/
	genOpStippledGCOp3,	/* void (* Reserved) () 		*/
	genOpStippledGCOp4,	/* void (* Reserved) () 		*/
	genOpStippledGCOp5,	/* void (* Reserved) () 		*/
	genOpStippledGCOp6	/* void (* Reserved) () 		*/
};


nfbWinOps S3CNAME(WinOps16) = 
{
        S3CNAME(CopyRect16),	/* void (* CopyRect)() 			*/
        S3CNAME(DrawSolidRects16),/* void (* DrawSolidRects)() 		*/
        S3CNAME(DrawImage16),	/* void (* DrawImage)() 		*/
        S3CNAME(DrawMonoImage16),/* void (* DrawMonoImage)() 		*/
        S3CNAME(DrawOpaqueMonoImage16),/* void (* DrawOpaqueMonoImage)()*/
        S3CNAME(ReadImage16),          /* void (* ReadImage)() 		*/
        S3CNAME(DrawPoints16),        /* void (* DrawPoints)() 		*/
        S3CNAME(TileRects16),          /* void (* TileRects)() 		*/
	S3CNAME(DrawMonoGlyphs16),/* void (* DrawMonoGlyphs)() 		*/
	genWinOp10,		/* void (* Reserved) () 		*/
	genWinOp11,		/* void (* Reserved) () 		*/
	genWinOp12,		/* void (* Reserved) () 		*/
	genWinOp13,		/* void (* Reserved) () 		*/
	genWinOp14,		/* void (* Reserved) () 		*/
        S3CNAME(ValidateWindowGC16)/* void (* ValidateWindowGC)()	*/
};

ddxScreenInfo S3CNAME(ScreenInfo) = 
{
        S3CNAME(Probe),              /* Bool (* screenProbe)() 		*/
        S3CNAME(Init),               /* Bool (* screenInit)() 		*/
	S3C_SCREEN_NAME,	/* char *screenName 			*/
};

scoScreenInfo S3CNAME(SysInfo) =
{
	NULL,			/* ScreenPtr pScreen			*/
	S3CNAME(SetGraphics),	/* void (*SetGraphics)()		*/
	S3CNAME(SetText),	/* void (*SetText)()			*/
	S3CNAME(SaveGState),	/* void (*SaveGState)()			*/
	S3CNAME(RestoreGState),	/* void (*RestoreGState)()		*/
	S3CNAME(CloseScreen),	/* void (*CloseScreen)()		*/
	TRUE,			/* Bool exposeScreen			*/
	TRUE,			/* Bool isConsole			*/
    TRUE,	/* Bool runSwitched		*/
	XSCO_VERSION,		/* float version			*/
};

