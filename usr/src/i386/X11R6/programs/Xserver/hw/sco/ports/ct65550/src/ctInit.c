/*
 *	@(#)ctInit.c	11.1	10/22/97	12:34:52
 *	@(#) ctInit.c 62.1 97/03/11 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Copyright (C) 1991-1997 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 *
 */
/*
 * ctInit.c
 *
 * Probe and Initialize the ct Graphics Display Driver
 */

#ident "@(#) $Id: ctInit.c 62.1 97/03/11 "

#include <sys/types.h>

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "ctProcs.h"
#include "ctMacros.h"

#if (CT_BITS_PER_PIXEL == 8)
extern VisualRec CT(Visual8);
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
extern VisualRec CT(Visual15);
extern VisualRec CT(Visual16);
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
extern VisualRec CT(Visual24);
#endif /* (CT_BITS_PER_PIXEL == 24) */

extern scoScreenInfo CT(SysInfo);
extern nfbGCOps CT(SolidPrivOps);
extern nfbGCOps CT(TiledPrivOps);
extern nfbGCOps CT(StippledPrivOps);
extern nfbGCOps CT(OpStippledPrivOps);
extern nfbWinOps CT(WinOps);

int CT(Generation) = -1;
int CT(ScreenPrivateIndex) = -1;
int CT(GCPrivateIndex) = -1;

static Bool CT(InitOnlyFlag) = FALSE;
static Bool CT(HardwareCursorFlag) = FALSE;
static Bool CT(WrapGCFlag) = TRUE;

#define CT_COPYRECT		0x00000001L
#define CT_DRAWSOLIDRECTS	0x00000002L
#define CT_DRAWIMAGE		0x00000004L
#define CT_DRAWMONOIMAGE	0x00000008L
#define CT_DRAWOPAQUEMONOIMAGE	0x00000010L
#define CT_READIMAGE		0x00000020L
#define CT_DRAWPOINTS		0x00000040L
#define CT_TILERECTS		0x00000080L
#define CT_DRAWMONOGLYPHS	0x00000100L
#define CT_DRAWFONTTEXT		0x00000200L
#define CT_SOLIDFILLRECTS	0x00000400L
#define CT_SOLIDFS		0x00000800L
#define CT_SOLIDZEROSEGS	0x00001000L
#define CT_TILEDFILLRECTS	0x00002000L
#define CT_TILEDFS		0x00004000L
#define CT_STIPPLEDFILLRECTS	0x00008000L
#define CT_STIPPLEDFS		0x00010000L
#define CT_OPSTIPPLEDFILLRECTS	0x00020000L
#define CT_OPSTIPPLEDFS		0x00040000L
#define CT_VALIDATEWINDOWGC	0x00080000L

static unsigned long genOps[] = {
	CT_COPYRECT,			/* use genCopyRect */
	CT_DRAWSOLIDRECTS,		/* use genDrawSolidRects */
	CT_DRAWIMAGE,			/* use CT(DrawImageFB) routine */
	CT_DRAWMONOIMAGE,		/* use genDrawMonoImage */
	CT_DRAWOPAQUEMONOIMAGE,		/* use genDrawOpaqueMonoImage */
	CT_READIMAGE,			/* XXX there is no replacement */
	CT_DRAWPOINTS,			/* use genDrawPoints */
	CT_TILERECTS,			/* use genTileRects */
	CT_DRAWMONOGLYPHS,		/* use genDrawMonoGlyphs */
	CT_DRAWFONTTEXT,		/* use genDrawFontText */
	CT_SOLIDFILLRECTS,		/* use genSolidFillRects */
	CT_SOLIDFS,			/* use genSolidFS */
	CT_SOLIDZEROSEGS,		/* use genSolidZeroSegs */
	CT_TILEDFILLRECTS,		/* use genTiledFillRects */
	CT_TILEDFS,			/* use genTiledFS */
	CT_STIPPLEDFILLRECTS,		/* use genStippledFillRects */
	CT_STIPPLEDFS,			/* use genStippledFS */
	CT_OPSTIPPLEDFILLRECTS,		/* use genOpStippledFillRects */
	CT_OPSTIPPLEDFS,		/* use genOpStippledFS */
	CT_VALIDATEWINDOWGC,		/* use CT(ValidateWindowGCNoWrap) */
	0L,				/* 0-terminated */
};

/*******************************************************************************

				Private Routines

*******************************************************************************/

static long
atoh(str)
char *str;
{
	unsigned long val;

	if (sscanf(str, "0x%x", &val) != 0) {
		sscanf(str, "%x", &val);
	}
	return (val);
}

static void
ctGetInfo()
{
	unsigned char vendor_id_hi;
	unsigned char vendor_id_lo;
	unsigned char device_id_hi;
	unsigned char device_id_lo;
	unsigned char revision;
	unsigned char config;
	unsigned char memory;
	char *bus_type = "unknown bus configuration";
	char *mem_conf = "unknown memory configuration";

	CT_XRIN(0x00, vendor_id_lo);
	CT_XRIN(0x01, vendor_id_hi);

	if (vendor_id_lo != 0x2c || vendor_id_hi != 0x10) {
		ErrorF("Warning: Chips 65550/65554 not detected (vendor_id=0x%02x%02x)\n", vendor_id_hi, vendor_id_lo);
		return;
	}

	CT_XRIN(0x02, device_id_lo);
	CT_XRIN(0x03, device_id_hi);

	if (((device_id_lo != 0xe0) && (device_id_lo != 0xe4))
		|| device_id_hi != 0x00) {
		ErrorF("Warning: Chips 65550/65554 not detected (device_id=0x%02x%02x)\n", device_id_hi, device_id_lo);
		return;
	}

	CT_XRIN(0x04, revision);

	if (device_id_lo == 0xe0)
		ErrorF("Chips 65550 Revision %02d ", revision);
	else
		ErrorF("Chips 65554 Revision %02d ", revision);

	CT_XRIN(0x08, config);

	switch (config & 0x01) {
	case 0x00:
		bus_type = "VL Bus";
		break;
	case 0x01:
		bus_type = "PCI Bus";
		break;
	}

	CT_XRIN(0x43, memory);

	switch (memory & 0x02) {
	case 0x00:
		mem_conf = "1MB VRAM";
		break;
	case 0x02:
		mem_conf = "2MB VRAM";
		break;
	}

	ErrorF("%s, %s\n", bus_type, mem_conf);
}

static void
ctDoText(ScreenPtr pScreen)
{
	CT(SetText)(pScreen);
}

static unsigned opsFlagVal = 
#if 0
	CT_COPYRECT | 
	CT_DRAWSOLIDRECTS |
	CT_DRAWIMAGE |
	CT_DRAWMONOIMAGE |
	CT_DRAWOPAQUEMONOIMAGE |
	CT_READIMAGE |
	CT_DRAWPOINTS |
	CT_TILERECTS |  
	CT_DRAWMONOGLYPHS |
	CT_DRAWFONTTEXT |
	CT_SOLIDFILLRECTS |
	CT_SOLIDFS |
	CT_SOLIDZEROSEGS |
	CT_TILEDFILLRECTS |
	CT_TILEDFS |
	CT_STIPPLEDFILLRECTS |
	CT_STIPPLEDFS |
	CT_OPSTIPPLEDFILLRECTS |
	CT_OPSTIPPLEDFS |
	CT_VALIDATEWINDOWGC |
#else
/* Broken or not implemented */
	CT_DRAWMONOIMAGE |
	CT_DRAWOPAQUEMONOIMAGE |
	CT_TILERECTS |  
	CT_SOLIDFILLRECTS |
	CT_SOLIDFS |
	CT_STIPPLEDFILLRECTS |
	CT_STIPPLEDFS |
	CT_VALIDATEWINDOWGC |
#endif
	0;

static void
ctProcessOptions(ScreenPtr pScreen, int argc, char **argv)
{
	extern void CT(ValidateWindowGCNoWrap)();
#if 0
	unsigned long opsFlag = 0L;
#else
	unsigned long opsFlag = opsFlagVal;
#endif

	Bool getInfoFlag = FALSE;
	Bool doTextFlag = FALSE;
	Bool exitFlag = FALSE;
	register int    ii;

	for(ii = 1; ii < argc; ii++) {
		register char *s = argv[ii];

		if ( *s != '-' )
			continue;
		s++;
		if (strcmp(s,"opsflag") == 0) {
			ii++;
			if (ii < argc)
				opsFlag = atoh(argv[ii]);
			continue;
		}
		if (strcmp(s,"swcurs") == 0) {
			CT(HardwareCursorFlag) = FALSE;
			continue;
		}
		if (strcmp(s,"info") == 0) {
			getInfoFlag = TRUE;
			continue;
		}
		if (strcmp(s,"init") == 0) {
			CT(InitOnlyFlag) = TRUE;
			continue;
		}
		if (strcmp(s,"text") == 0) {
			doTextFlag = TRUE;
			exitFlag = TRUE;
			continue;
		}
	}

	for (ii = 0; genOps[ii] != 0L; ii++) {
		switch (opsFlag & genOps[ii]) {
		case CT_COPYRECT:
			CT(WinOps).CopyRect = genCopyRect;
			ErrorF("using genCopyRect()\n");
			continue;	
		case CT_DRAWSOLIDRECTS:
			CT(WinOps).DrawSolidRects = genDrawSolidRects;
			ErrorF("using genDrawSolidRects()\n");
			continue;
		case CT_DRAWIMAGE:
			CT(WinOps).DrawImage = CT(DrawImageFB);
			ErrorF("using CT(DrawImageFB)()\n");
			continue;
		case CT_DRAWMONOIMAGE:
			CT(WinOps).DrawMonoImage = genDrawMonoImage;
			/*ErrorF("using genDrawMonoImage()\n");*/
			continue;
		case CT_DRAWOPAQUEMONOIMAGE:
			CT(WinOps).DrawOpaqueMonoImage = genDrawOpaqueMonoImage;
			/*ErrorF("using genDrawOpaqueMonoImage()\n");*/
			continue;
		case CT_READIMAGE:
			ErrorF("can't replace ReadImage()\n");
			continue;
		case CT_DRAWPOINTS:
			CT(WinOps).DrawPoints = genDrawPoints;
			ErrorF("using genDrawPoints()\n");
			continue;
		case CT_TILERECTS:
			CT(WinOps).TileRects = genTileRects;
			/*ErrorF("using genTileRects()\n");*/
			continue;
		case CT_DRAWMONOGLYPHS:
			CT(WinOps).DrawMonoGlyphs = genDrawMonoGlyphs;
			ErrorF("using genDrawMonoGlyphs()\n");
			continue;
#ifdef agaII
		case CT_DRAWFONTTEXT:
			CT(WinOps).DrawFontText = genWinOp10;
			ErrorF("using genWinOp10()\n");
			continue;
#else
		case CT_DRAWFONTTEXT:
			CT(WinOps).DrawFontText = genDrawFontText;
			ErrorF("using genDrawFontText()\n");
			continue;
#endif
		case CT_SOLIDFILLRECTS:
			CT(SolidPrivOps).FillRects = genSolidFillRects;
			/*ErrorF("using genSolidFillRects()\n");*/
			continue;
		case CT_SOLIDFS:
			CT(SolidPrivOps).FillSpans = genSolidFS;
			/*ErrorF("using genSolidFS()\n");*/
			continue;
#ifdef agaII
		case CT_SOLIDZEROSEGS:
			CT(SolidPrivOps).FillZeroSegs = genSolidZeroSeg;
			ErrorF("using genSolidZeroSeg()\n");
			continue;
#else
		case CT_SOLIDZEROSEGS:
			CT(SolidPrivOps).FillZeroSegs = genSolidZeroSegs;
			ErrorF("using genSolidZeroSegs()\n");
			continue;
#endif
		case CT_TILEDFILLRECTS:
			CT(TiledPrivOps).FillRects = genTiledFillRects;
			ErrorF("using genTiledFillRects()\n");
			continue;
		case CT_TILEDFS:
			CT(TiledPrivOps).FillSpans = genTiledFS;
			ErrorF("using genTiledFS()\n");
			continue;
		case CT_STIPPLEDFILLRECTS:
			CT(StippledPrivOps).FillRects = genStippledFillRects;
			/*ErrorF("using genStippledFillRects()\n");*/
			continue;
		case CT_STIPPLEDFS:
			CT(StippledPrivOps).FillSpans = genStippledFS;
			/*ErrorF("using genStippledFS()\n");*/
			continue;
		case CT_OPSTIPPLEDFILLRECTS:
			CT(OpStippledPrivOps).FillRects = genOpStippledFillRects;
			ErrorF("using genOpStippledFillRects()\n");
			continue;
		case CT_OPSTIPPLEDFS:
			CT(OpStippledPrivOps).FillSpans = genOpStippledFS;
			ErrorF("using genOpStippledFS()\n");
			continue;
		case CT_VALIDATEWINDOWGC:
			CT(WinOps).ValidateWindowGC = CT(ValidateWindowGCNoWrap);
			CT(TiledPrivOps).FillRects = genTiledFillRects;
			CT(TiledPrivOps).FillSpans = genTiledFS;
			CT(StippledPrivOps).FillRects = genStippledFillRects;
			CT(StippledPrivOps).FillSpans = genStippledFS;
			CT(OpStippledPrivOps).FillRects = genOpStippledFillRects;
			CT(OpStippledPrivOps).FillSpans = genOpStippledFS;
			CT(WrapGCFlag) = FALSE;
			/*
			ErrorF("using CT(ValidateWindowGCNoWrap)()\n");
			ErrorF("forcing genTiledFillRects()\n");
			ErrorF("forcing genTiledFS()\n");
			ErrorF("forcing genStippledFillRects()\n");
			ErrorF("forcing genStippledFS()\n");
			ErrorF("forcing genOpStippledFillRects()\n");
			ErrorF("forcing genOpStippledFS()\n");
			*/
			continue;
		default:
			continue;
		}
	}

	if (getInfoFlag) {
		ctGetInfo();
	}
	if (doTextFlag) {
		ctDoText(pScreen);
	}
	if (exitFlag) {
		exit(0);
		/* NOTREACHED */
	}
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * CT(Probe)() - test for graphics hardware
 *
 * This routine tests for your particular graphics board, and returns
 * true if its present, false otherwise.
 */
Bool
CT(Probe)(ddxDOVersionID version,ddxScreenRequest *pReq)
{
	return (ddxAddPixmapFormat(pReq->dfltDepth,
				   pReq->dfltBpp,
				   pReq->dfltPad));
}

/*
 * We are making this a global variable because the register on this
 * chip are mapped into a non-fixed memory space that is relative to
 * the start of video memory.  We do not want to recompute or pass around
 * the pScreen, ctPriv ... structures everywhere.
 */
unsigned char	*ct_membase;

/*
 * CT(InitHW)()
 *
 * Initialize hardware that only needs to be done ONCE.  This routine will
 * not be called on a screen switch.  It may just call CT(SetGraphics)()
 */
Bool
CT(InitHW)(pScreen)
    ScreenPtr pScreen;
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	unsigned char *fbBase;

	if (!grafGetMemInfo(grafinfo, NULL,
			&fbBase,
			&ctPriv->fbSize,
			&ctPriv->fbPointer)) {
		ErrorF ("Init(): Missing MEMORY in grafinfo file.\n");
		return (FALSE);
	}

	ct_membase = (unsigned char *)ctPriv->fbPointer;

	CT(SetGraphics)(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("Init():	fbBase=0x%08x, fbSize=%d, fbPointer=0x%08x\n",
			fbBase,
			ctPriv->fbSize,
			ctPriv->fbPointer);
	ErrorF("\tfbPixelSize=%d, fbStride=%d, w,h=%d,%d, d=%d\n",
			sizeof(CT_PIXEL),
			ctPriv->fbStride,
			ctPriv->width,
			ctPriv->height,
			ctPriv->depth);
#endif /* DEBUG_PRINT */

	if (CT(InitOnlyFlag)) {
		FatalError("Intentional termination\n");
		/* NOTREACHED */
	}

	return (TRUE);
}

/*
 * CT(Init)() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
CT(Init)(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
        grafData *grafinfo = DDX_GRAFINFO(pScreen);
	nfbScrnPrivPtr pNfb;
	VisualPtr pVisual;
	CT(PrivatePtr) ctPriv;
	int width, height, depth, rgbbits, mmx, mmy;
	int kTileMin, kTileMax;

#ifdef DEBUG_PRINT
	ErrorF("Init(): index=%d\n", index);
#endif /* DEBUG_PRINT */

	ctProcessOptions(pScreen, argc, argv);

	/* Get mode and monitor info */
	if (! grafGetInt(grafinfo, "PIXWIDTH",  &width)  ||
	    ! grafGetInt(grafinfo, "PIXHEIGHT", &height) ||
	    ! grafGetInt(grafinfo, "DEPTH",     &depth)) {
		ErrorF("Init(): can't find pixel info in grafinfo file.\n");
		return FALSE;
	}

	kTileMin = 5; kTileMax = 2;	/* Reasonable defaults */
	grafGetInt(grafinfo, "ONBOARDMIN", &kTileMin);
	grafGetInt(grafinfo, "ONBOARDMAX", &kTileMax);

	mmx = 300; mmy = 300;		/* Reasonable defaults */
	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);

	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

	if (!nfbScreenInit(pScreen, width, height, mmx, mmy))
		return FALSE;

	if (CT(Generation) != serverGeneration) {
		CT(ScreenPrivateIndex) = AllocateScreenPrivateIndex();
		if (CT(WrapGCFlag)) {
			CT(GCPrivateIndex) = AllocateGCPrivateIndex();
		}
		CT(Generation) = serverGeneration;
	}

	ctPriv = (CT(PrivatePtr))xalloc(sizeof(CT(Private)));
	CT_PRIVATE_DATA(pScreen) = ctPriv;

	ctPriv->fbStride = width;	/* XXX could be different! */
	ctPriv->bltPixelSize = (depth + (8 - 1)) / 8;
	ctPriv->bltStride = width * ctPriv->bltPixelSize;
	ctPriv->width = width;
	ctPriv->height = height;
	ctPriv->depth = depth;
	ctPriv->kTileMin = kTileMin;
	ctPriv->kTileMax = kTileMax;
	ctPriv->cursorEnabled = CT(HardwareCursorFlag);

	/*
	 * We need to wrap the CreateGC() method because NFB gives us no means
	 * to initialize our GC private structure.
	 *
	 * NOTE: The code that uses the GC private only works for 8- and 16-bit
	 * modes.
	 */
	if (CT(WrapGCFlag)) {
		AllocateGCPrivate(pScreen, CT(GCPrivateIndex),
				sizeof(CT(GCPrivRec)));
		ctPriv->CreateGC = pScreen->CreateGC;
		pScreen->CreateGC = CT(CreateGC);
	}

	switch (depth) {
#if (CT_BITS_PER_PIXEL == 8)
	case 8:
		pVisual = &CT(Visual8);
		if (! grafGetInt(grafinfo, "RGBBITS", &rgbbits))
			rgbbits = 6;
		pVisual->bitsPerRGBValue = rgbbits;
		ctPriv->dacShift = sizeof(unsigned short) * 8 - rgbbits;
		ctPriv->allPlanes = (unsigned long)0x000000ffL;
		break;
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
	case 15:
		pVisual = &CT(Visual15);
		ctPriv->allPlanes = (unsigned long)0x00007fffL;
		break;
	case 16:
		pVisual = &CT(Visual16);
		ctPriv->allPlanes = (unsigned long)0x0000ffffL;
		break;
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
	case 24:
		pVisual = &CT(Visual24);
		ctPriv->allPlanes = (unsigned long)0x00ffffffL;
		break;
#endif /* (CT_BITS_PER_PIXEL == 24) */
	default:
		ErrorF("Init(): %d-bit depth not supported.\n", depth);
		return FALSE;
	}
	if (!nfbAddVisual(pScreen, pVisual))
		return FALSE;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops = &CT(SolidPrivOps);
#if (CT_BITS_PER_PIXEL == 8)
	pNfb->SetColor = CT(SetColor);
	pNfb->LoadColormap = genLoadColormap;
#else /* (CT_BITS_PER_PIXEL == 8) */
	pNfb->SetColor = NoopDDA;
	pNfb->LoadColormap = NoopDDA;
#endif /* (CT_BITS_PER_PIXEL == 8) */
	pNfb->BlankScreen = CT(BlankScreen);
	pNfb->ValidateWindowPriv = CT(ValidateWindowPriv);

	if (!CT(InitHW)(pScreen))
		return FALSE;

	/*
	 * Initialize BitBlt engine utilities module. Must be done after mapping
	 * memory.
	 */
	CT(BitBltInit)(pScreen);

	/*
	 * Initialize offscreen memory manager.
	 */
	if (!CT(OnboardInit)(pScreen))
		return FALSE;

	/*
	 * Initialize planemask 8x8 patterns
	 */
	if (!CT(MaskPatternInit)(pScreen))
		return FALSE;

	/*
	 * Turn on the cursor.
	 */
	CT(CursorInitialize)(pScreen);

	/*
	 * This should work for most cases.
	 */
	if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
		cfbCreateDefColormap(pScreen)) == 0 )
		return FALSE;

	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &CT(SysInfo));

	/*
	 * Set any NFB runtime options here
	 */
	nfbSetOptions(pScreen, NFB_VERSION, NFB_POLYBRES, 0);

	return TRUE;
}

/*
 * CT(FreeScreenData)()
 *
 * Anything you allocate in CT(Init)() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
void
CT(FreeScreen)(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);

	CT(MaskPatternClose)(pScreen);
	CT(CursorFree)(pScreen);
	CT(OnboardClose)(pScreen);

	xfree(ctPriv);
}
