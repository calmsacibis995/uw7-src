/*
 *	@(#) effData.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Modification History
 *
 * S034, 06-Jul-93, buckm
 *	We can now run while screen-switched.
 * S033, 21-Apr-92, hiramc
 *	#include nfbCmap.h removed
 * S032, 12-Dec-92, mikep
 *	add effSolidFS.
 * S031, 04-Dec-92, mikep
 *	remove S030.  Add effDrawFontText.
 * S030, 31-Oct-92, mikep
 *	put clip routines into their own data structure
 * S025, 28-Sep-92, mikep@sco.com
 *	put effCloseScreen in effSysInfo structure.
 * S024, 03-Sep-92, staceyc
 * 	clip regions and fast text added
 * S023, 03-Sep-92, hiramc@sco.COM
 *	Remove the include cfb.h - not needed
 * S022, 29-Sep-91, mikep@sco.com
 *	removed all the unecessary structures from this file.
 * S021, 21-Sep-91, mikep@sco.com
 *	added runSwitched and version to scoSysInfo.
 *	removed CursorOn and format info from ddxScreenInfo
 *	renamed genStub routines and added padding to GCOps and WinOps
 * S020, 18-Sep-91, staceyc
 * 	added draw mono glyphs to win ops
 * S019, 08-Sep-91, mikep@sco.com
 *	nix the relative include paths!!!!
 * S018, 07-Sep-91, mikep@sco.com
 *	update screen struct for R5 and padding.
 * S017, 03-Sep-91, staceyc
 * 	save and restore all off screen during screen switch
 * S016, 28-Aug-91, staceyc
 * 	added new sys info stuff - punted old screen switch stuff
 * S015, 16-Aug-91, staceyc
 * 	enable backing store
 * S014, 15-Aug-91, staceyc
 * 	fix up include files
 * S013, 13-Aug-91, mikep
 * 	- change nfbGCOps to genOps.
 * S012, 13-Aug-91, staceyc
 * 	- new font realize
 *	- removed sprite level cursor calls
 * S011, 09-Aug-91, staceyc
 * 	added new eff stippled fill rects
 * S010, 06-Aug-91, mikep@sco.com
 *	declare all the effPrivOps for the new ValidateWindowGC.
 * S009, 01-Aug-91, staceyc
 * 	added new query best size routine
 * S008, 16-Jul-91, staceyc
 * 	noop all the nfb screen cursor stuff
 * S007, 28-Jun-91, staceyc
 * 	adjust devKind and bits per color to their correct values
 * S006, 26-Jun-91, staceyc
 * 	punt that ickky gen stuff - no offence buck :-)
 * S005, 26-Jun-91, buckm@sco.com
 * 	initialize new LoadColormap member in nfbScrnPriv structure.
 *	temporarily include genProcs.h.
 * S004, 25-Jun-91, mikep@sco.com
 * 	added Layer structures and info
 * S003, 24-Jun-91, staceyc
 * 	moved the nfb GCops to here
 * S002, 21-Jun-91, staceyc
 * 	stuff to deal with Bres line drawing
 * S001, 21-Jun-91, staceyc
 * 	added bitblt routine - converted some gen to eff
 * S000, 18-Jun-91, staceyc
 * 	initial changes
 */

#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "font.h"
#include "pixmapstr.h"
#include "window.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "mistruct.h"
#include "scoext.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"
/*#include "nfbCmap.h"		S033	removed	*/
#include "genDefs.h"
#include "genProcs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"
#include "ddxScreen.h"

extern void NoopDDA() ;

VisualRec effVisual = {
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


nfbGCOps effSolidPrivOps = {
	genSolidFillRects,	/* void (* FillRects) () */	/* S013 vvv */
	effSolidFS,		/* void (* FillSpans) () */
	effFillZeroSeg,		/* void (* FillZeroSeg) () */
	genSolidGCOp4,		/* void (* Reserved) () */
	genSolidGCOp5,		/* void (* Reserved) () */
	genSolidGCOp6		/* void (* Reserved) () */
};


nfbGCOps effTiledPrivOps = {
	genTiledFillRects,	/* void (* FillRects) () */
	genTiledFS,		/* void (* FillSpans) () */
	genTiledGCOp3,		/* void (* Reserved) () */
	genTiledGCOp4,		/* void (* Reserved) () */
	genTiledGCOp5,		/* void (* Reserved) () */
	genTiledGCOp6		/* void (* Reserved) () */
} ;

nfbGCOps effStippledPrivOps = {
	effStippledFillRects,	/* void (* FillRects) () */
	genStippledFS,		/* void (* FillSpans) () */
	genStippledGCOp3,	/* void (* Reserved) () */
	genStippledGCOp4,	/* void (* Reserved) () */
	genStippledGCOp5,	/* void (* Reserved) () */
	genStippledGCOp6	/* void (* Reserved) () */
} ;

nfbGCOps effOpStippledPrivOps = {
	effOpStippledFillRects,	/* void (* FillRects) () */
	genOpStippledFS,	/* void (* FillSpans) () */
	genOpStippledGCOp3,	/* void (* Reserved) () */
	genOpStippledGCOp4,	/* void (* Reserved) () */
	genOpStippledGCOp5,	/* void (* Reserved) () */
	genOpStippledGCOp6	/* void (* Reserved) () */
} ;


nfbWinOps effWinOps = {
        effCopyRect,            /* void (* CopyRect)() */
        effDrawSolidRects,      /* void (* DrawSolidRects)() */
        effDrawImage,           /* void (* DrawImage)() */
        effDrawMonoImage,       /* void (* DrawMonoImage)() */
        effDrawOpaqueMonoImage, /* void (* DrawOpaqueMonoImage)() */
        effReadImage,           /* void (* ReadImage)() */
        effDrawPoints,          /* void (* DrawPoints)() */
        effTileRects,           /* void (* TileRects)() */
	effDrawMonoGlyphs,	/* void (* DrawMonoGlyphs)() */
	effDrawFontText,	/* void (* DrawFontText) () */
	genWinOp11,		/* void (* Reserved) () */
	genWinOp12,		/* void (* Reserved) () */
	genWinOp13,		/* void (* Reserved) () */
	effSetClipRegions,	/* void (* SetClipRegions) () */
        effValidateWindowGC     /* void (* ValidateWindowGC)() */
};

ddxScreenInfo effScreenInfo = {
        effProbe,               /* Bool (* screenProbe)() */
        effInit,                /* Bool (* screenInit)() */
	"eff",			/* char *screenName */
} ;

scoScreenInfo effSysInfo =
{
    NULL,		/* ScreenPtr pScreen		*/
    effSetGraphics,	/* void (*SetGraphics)()	*/
    effSetText,		/* void (*SetText)()		*/
    effSaveGState,	/* void (*SaveGState)()		*/
    effRestoreGState,	/* void (*RestoreGState)()	*/
    effCloseScreen,	/* void (*CloseScreen)()	*/
    TRUE,		/* Bool exposeScreen		*/
    TRUE,		/* Bool isConsole		*/
    TRUE,		/* Bool runSwitched		*/	/* S034 */
    XSCO_VERSION,	/* float version		*/
};

