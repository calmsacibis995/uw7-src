/*
 *	@(#) qvisInit.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 21:23:36 PDT 1992	mikep@sco.com
 *	- Port to R5, remove references to qvisHelpValidateGC
 *	- Add GC caching
 *	S001	Sun Oct 11 16:05:02 PDT 1992	mikep@sco.com
 *	- Check enforceProtocol to decide which type of lines to draw
 *	S002	Sun Oct 11 18:41:24 PDT 1992	mikep@sco.com
 *	- Allow use of CFB for performance comparisions
 *	S003	Mon Oct 26 14:57:10 PST 1992	mikep@sco.com
 *	- Assume glyph caching to be false.
 *	S004	Sun Nov 01 20:01:32 PST 1992	mikep@sco.com
 *	- Add more options to nfbSetOptions()
 *	S005	Thu Feb 11 10:30:19 PST 1993	mikep@sco.com
 *	- Incorperate Compaq bugfixes
 *      S006    Thu Oct 07 12:50:40 PDT 1993    davidw@sco.com
 *      - Integrated Compaq source handoff
 *	S007	Tue Mar 22 04:16:02 PST 1994	davidw@sco.com
 *      - Integrated Compaq source handoff - AHS 3.3.0
 *      S008    Tue Sep 20 14:36:40 PDT 1994    davidw@sco.com
 *      - Correct compiler warnings.
 *      S009    Tue Sep 20 14:45:40 PDT 1994    davidw@sco.com
 *      - mkdev graphics is gone - update error messages.
 *
 */

/**
 * qvisInit.c
 *
 * Probe and Initialize the qvis Graphics Display Driver
 */

/**
 * Copyright 1991,1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 * mikep@sco 07/27/92  Add current_bank to screen privates and init.
 * mikep@sco 09/28/92  Don't muck with pScreen->CloseScreen. 
 * waltc     02/04/93  Change qxs to qvis in ErrorF's, remove call to
 *                     qvisSolidZeroSegPtToPt since updated qvisSolidZeroSeg 
 *                     pixelates correctly.
 * waltc     06/26/93  Initialize display width, pitch, max_cursor.
 * waltc     10/05/93  Detect V35-3 for ssblit bug fix.
 * waltc     03/20/94  Improve QVision 1024/1280 detection, error messages.
 *
 */

#include "xyz.h"
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
#include "grafinfo.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"					/* S008 */
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef usl
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif
#include <string.h>

#include "qvisHW.h"
#include "qvisMacros.h"
#include "qvisDefs.h"
#include "qvisProcs.h"

extern Bool     mfbCreateDefColormap();
extern Bool     cfbCreateDefColormap();
extern int      AllocateScreenPrivateIndex();
extern Bool     ddxAddPixmapFormat(int depth, int dpp, int pad);
extern int      ioctl();

extern Bool enforceProtocol;					/* S001 */

extern scoScreenInfo qvisSysInfo;
extern VisualRec qvisVisual;

extern nfbGCOps qvisBankedSolidPrivOps;
extern nfbGCOps qvisFlatSolidPrivOps;

extern nfbWinOps qvisFlatWinOps;
extern nfbWinOps qvisBankedWinOps;
extern nfbWinOps qvisGlyphCacheFlatWinOps;

extern void     qvisCursorInitialize();

static int qvisGeneration = -1;					/* S008 */
int             qvisScreenPrivateIndex;

/*
 * qvisProbe() - test for graphics hardware
 */
Bool
qvisProbe(version, pReq)
    ddxDOVersionID  version;	/* unused */
    ddxScreenRequest *pReq;
{
    /*
     * XXX It's a shame we can't do a better job probing if the hardware
     * exists or not at this point but we really have to have the
     * card "mapped" in to make those sorts of decisions.  Plus we
     * have to unlock the right registers.  It's a real hassle to
     * probe if QVision exists or not anyway! -mjk
     *
     * There is some attempt at a probe in qvisInitHW.
     */
    XYZ("qvisProbe-entered");
    return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

static Bool qvis_first_screen;
static Bool qvis_no_multi_screen;
static unsigned char qvis_next_id;

/*
 * qvisInitHW()
 * 
 * Template for machine dependent hardware initialization code This should do
 * everything it takes to get the machine ready to draw.
 */
Bool
qvisInitHW(pScreen)
    ScreenPtr       pScreen;
{
    unsigned char  *fb_base;
    char           *class;
    unsigned char  version;
    unsigned char  asic;

    XYZ("qvisInitHW-entered");

    /*
     * Set to the virtual controller for the board we want to initialize.
     */
    if(qvis_first_screen) {
       unsigned char virt_controller_id;

       qvis_first_screen = FALSE;
       /*
	* Query the current virtual controller.
	*/
       virt_controller_id = qvisIn8(BOARD_SELECT);
       QVIS_PRIVATE_DATA(pScreen)->virt_controller_id = virt_controller_id;
       QVIS_PRIVATE_DATA(pScreen)->primary = TRUE;
       if(virt_controller_id != 0) {
	  /*
           * Don't allow multi-screen support if primary adapter VID not 0
	   */
	  qvis_no_multi_screen = TRUE;
       } else {
	  /*
	   * first card is virtual controller zero, so next card will
	   * be virtual controller one.
	   */
	  qvis_next_id = 1;
       }
    } else {
       QVIS_PRIVATE_DATA(pScreen)->primary = FALSE;
       if(qvis_no_multi_screen) {
          ErrorF("qvis: QVision multi-screen support NOT enabled.\n");
          ErrorF("qvis: Primary controller must be first (ID = 0).\n");
          return FALSE;
       } else {
	  QVIS_PRIVATE_DATA(pScreen)->virt_controller_id = qvis_next_id;
	  /*
	   * Increment to next virtual controller id for next screen!
	   */
	  qvis_next_id++;
       }
    }
    /*
     * Make sure we are using the card we want!
     */
    qvisOut8(BOARD_SELECT, QVIS_PRIVATE_DATA(pScreen)->virt_controller_id);

    /*
     * Let's make sure the card is a Triton now (sort of late but it
     * is our first real chance).
     */
    qvisOut8(GC_INDEX, 0x0f);      /* Unlock registers */
    qvisOut8(GC_DATA, 0x05);
    qvisOut8(GC_INDEX, CTLR_VERS); /* Controller Version Number Register */
    version = qvisIn8(GC_DATA);
    asic = (unsigned char)
	(((unsigned int)version & 0xf8) >> 3);/* strip off revision *//* S008 */
    if (asic != 6 && asic != 14) {
       ErrorF("qvis: The configured graphics adapter is not a QVision 1024 or 1280.\n"); /* S009 */
       ErrorF("qvis: Configure the correct graphics adapter and try again.\n"); /* S009 */
       return FALSE;
    }

    /* Detect V35-3 for ssblit bug fix */
    QVIS_PRIVATE_DATA(pScreen)->v35_blit_bug = FALSE;
    if (version == 0x71) {
        qvisOut8(GC_INDEX, CTLR_EXT_VERS); /* Extended version number */
        if (qvisIn8(GC_DATA) == 1)
            QVIS_PRIVATE_DATA(pScreen)->v35_blit_bug = TRUE;
    }

    /* Save color table, initialize graphics mode */
    if (serverGeneration == 1) {
	qvisSaveDACState(pScreen);
	qvisSetGraphics(pScreen);
    }

    /* (void) qvisBlankScreen(SCREEN_SAVER_ON,pScreen); */
    return TRUE;
}

/*
 * qvisInit() - template for machine dependent screen init
 * 
 * This routine is the template for a machine dependent screen init. Once you
 * start doing multiple visuals or need a screen priv you should check out
 * all the stuff in effInit.c.
 */
Bool
qvisInit(index, pScreen, argc, argv)
    int             index;
    ScreenPtr       pScreen;
    int             argc;	/* unused parameter */
    char          **argv;	/* unused parameter */
{
    qvisPrivateData *qvisPriv;
    ddxScreenInfo  *pInfo = ddxActiveScreens[index];
    grafData       *grafinfo = DDX_GRAFINFO(pScreen);
    nfbScrnPrivPtr  pNfb;
    int             width, height, mmx, mmy, depth;
    char           *string;
    char           *fbtype;
    Bool            glyph_cache;

    XYZ("qvisInit-entered");

    /* allocate screen private index and structure */
    /* NOTE: only allocate one qvisScreenPrivateIndex per generation */
    if (qvisGeneration != serverGeneration) {
	qvisGeneration = serverGeneration;
	qvisScreenPrivateIndex = AllocateScreenPrivateIndex();
	if (qvisScreenPrivateIndex < 0) {
	    ErrorF("qvis: Screen private index allocation error.\n");
	    return FALSE;
	}
	qvis_first_screen = TRUE;
	qvis_no_multi_screen = FALSE;
    }
    qvisPriv = (qvisPrivateData *) xalloc(sizeof(qvisPrivateData));
    if (qvisPriv == NULL) {
	ErrorF("qvis: Screen private data memory allocation error.\n");
	return FALSE;
    }
    pScreen->devPrivates[qvisScreenPrivateIndex].ptr = 
					(unsigned char *)qvisPriv; /* S008 */


#ifdef QVIS_SHADOWED
    /* fill in bogus values for all Q-Vision register shadows */
    qvisPriv->alu = -1;
    qvisPriv->fg = -1;
    qvisPriv->bg = -1;
    qvisPriv->pixel_mask = -1;
    qvisPriv->plane_mask = -1;
#endif				/* QVIS_SHADOWED */
    qvisPriv->current_bank = -1;
    qvisPriv->current_gc = -1;					/* S000 */

    qvisPriv->bad_blit_state = FALSE;
    qvisPriv->engine_used = FALSE;
    qvisPriv->glyph_cache = FALSE;				/* S003 */

    /* Get mode and monitor info */
    if (!grafGetInt(grafinfo, "PIXWIDTH", &width) ||
	!grafGetInt(grafinfo, "PIXHEIGHT", &height)) {
	ErrorF("qvis: Grafinfo PIXWIDTH or PIXHEIGHT error.\n");
	return FALSE;
    }

    /* Initialize display width screen private */
    qvisPriv->width = width;

    /* Get display depth */
    grafGetInt(grafinfo, "DEPTH", &depth);

    /* Initialize display pitch and max cursor size screen privates */
    if (width == 1280) {
        if (depth == 8) {
            qvisPriv->pitch = 2048;
            qvisPriv->pitch2 = 11;
        }
        else { /* depth = 4 */
            qvisPriv->pitch = 1024;
            qvisPriv->pitch2 = 10;
        }
        qvisPriv->max_cursor = 64;
    }
    else {
        if (depth == 8) {
            qvisPriv->pitch = 1024;
            qvisPriv->pitch2 = 10;
        }
        else {
            qvisPriv->pitch = 512;
            qvisPriv->pitch2 = 9;
        }
        qvisPriv->max_cursor = 32;
    }

    /*
     * Reasonable defaults in case the next two grafGetInt calls
     * fail.
     */
    mmx = 300;
    mmy = 300;		

    grafGetInt(grafinfo, "MON_WIDTH", &mmx);
    grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

    if (!grafGetString(grafinfo, "FBTYPE", &fbtype)) {
	ErrorF("qvis: Grafinfo FBTYPE error.\n");
	return FALSE;
    }

    if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &qvisPriv->fb_base)) {
	    ErrorF("qvis: Grafinfo MEMORY error.\n");
	    return FALSE;
    }

    /* Allow use of cfb.  Handy for performance comparisons 	   S002 vvv */
    if (grafQuery(grafinfo, "USEDFB")) {
	static Bool qvisCFBInit();

	if (strcmp(fbtype, "BANK") == 0) {
                FatalError("qvis: Incompatible grafinfo USEDFB and BANK.\n");
		/* NOT REACHED */
	}

	ErrorF("qvis: Using CFB code, index %d\n", index);
	if (!qvisCFBInit(pScreen, qvisPriv->fb_base, width, height, mmx, mmy, 
			depth))
		return FALSE;
    }
    else {							/* S002 ^^^ */

	if (!nfbScreenInit(pScreen, width, height, mmx, mmy)) {
            ErrorF("qvis: nfbScreenInit error.\n");
	    return FALSE;
	}
	if (!nfbAddVisual(pScreen, &qvisVisual)) {
	    ErrorF("qvis: nfbAddVisual error.\n");
	    return FALSE;
	}

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->SetColor = qvisSetColor;
	pNfb->LoadColormap = genLoadColormap;
	pNfb->BlankScreen = qvisBlankScreen;
	pNfb->ValidateWindowPriv = qvisValidateWindowPriv;


	/* the default is glyph caching off */
	glyph_cache = FALSE;
	if (grafGetString(grafinfo, "GLYPH_CACHE", &string)) {
	    XYZ("qvisInit-GLYPH_CACHE-specified");
	    /*
	     * This test is case insensitive! You must say "YES"!
	     */
	    if (strcmp(string, "YES") == 0) {
		XYZ("qvisInit-GLYPH_CACHE-specifiedEnabled");
		glyph_cache = TRUE;
	    }
	}

	if (strcmp(fbtype, "BANK") == 0) {
	    XYZ("qvisInit-BANKED");
	    qvisPriv->flat_mode = FALSE;
	    qvisPriv->qvisWinOps = &qvisBankedWinOps;
	    qvisPriv->qvisSolidPrivOpsPtr = &qvisBankedSolidPrivOps;
	    /*
	     * set it false no matter what it was before; we don't
	     * support glyph caching in banked mode at all
	     */
	    XYZ("qvisInit-GlyphCache-no");
	    qvisPriv->glyph_cache = FALSE;
	} else {			/* must be FLAT */
	    XYZ("qvisInit-FLAT");
	    qvisPriv->flat_mode = TRUE;
	    qvisPriv->qvisWinOps = &qvisFlatWinOps;
	    if(glyph_cache) {
	       /* glyph caching wanted */
	       XYZ("qvisInit-GlyphCacheDesired");
	       qvisPriv->glyph_cache = qvisInitGlyphCache(pScreen);
	       if(qvisPriv->glyph_cache) {
		  /*
		   * glyph caching successfully intialized
		   */
		  XYZ("qvisInit-EnableGlyphCache");
		  /* set up the proper win ops for installation */
		  qvisPriv->qvisWinOps = &qvisGlyphCacheFlatWinOps;
		  /* install font clear routine */
		  pNfb->ClearFont = qvisGlyphCacheClearFont;
	       } else {
		  XYZ("qvisInit-GlyphCache-AllocationFailure");
		  ErrorF("qvis: Could not enable glyph cache.\n");
	       }
	    } else {
	       XYZ("qvisInit-FLAT-butGlyphCacheNotDesired");
	       qvisPriv->glyph_cache = FALSE;
	    }
	    qvisPriv->qvisSolidPrivOpsPtr = &qvisFlatSolidPrivOps;
	}
	/*
	 * There is no path through the above code when qvisPriv->glyph_cache
	 * would not be set one way or the other.  If there was, we
	 * could get this far without qvisPriv->glyph_cache being set
	 * TRUE or FALSE, that would be a bug.
	 */

	pNfb->protoGCPriv->ops = qvisPriv->qvisSolidPrivOpsPtr;

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);		/* S000,S004 */
    }


    if (!qvisInitHW(pScreen)) {
        ErrorF("qvis: FAILED TO INITIALIZE HARDWARE.\n");
	return (FALSE);
    }

    /**
     * Call one of the following
     *
     * scoSWCursorInitialize(pScreen);
     * nfbCursorInitialize(pScreen);
     * qvisCursorInitialize(pScreen);
     */
    qvisCursorInitialize(pScreen);

    /*
     * This should work for most cases.
     */
    if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
	 cfbCreateDefColormap(pScreen)) == 0)
	return FALSE;

    /*
     * Give the sco layer our screen switch functions.  Always do this last.
     */
    scoSysInfoInit(pScreen, &qvisSysInfo);


    return TRUE;
}

/*
 * qvisCloseScreen()
 * 
 * Template for machine dependent screen close routine.  Anything you allocate
 * in qvisInit() above should be freed here.
 */
void								/* S008 */
qvisCloseScreen(index, pScreen)
    int             index;
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisCloseScreen-entered");
    /*
     * Turn off the cursor on each screen being closed.  The prevents
     * the Q-Vision hardware cursor from being "stranded" on screens
     * it shouldn't be on.
     */
    qvisCursorOn(FALSE, pScreen);

    if(qvisPriv->glyph_cache) {
       qvisFreeGlyphCache(pScreen);
    }
    xfree(QVIS_PRIVATE_DATA(pScreen));
}
								/* S002 vvv */
/*
 * qvisCFBInit() - xga dumb frame buffer init
 */
static Bool
qvisCFBInit(pScreen, pbits, width, height, mmx, mmy, depth)
	ScreenPtr pScreen;
	pointer pbits;
	int width, height, mmx, mmy, depth;
{
	nfbScrnPrivPtr pNfb;

	if ((pNfb = (nfbScrnPrivPtr)xalloc(sizeof(nfbScrnPriv))) == NULL)
		return FALSE;

	if (!cfbScreenInit(pScreen, pbits, width, height, mmx, mmy, width)) {
		xfree(pNfb);
		return FALSE;
	}
	/*
	 * These aren't really nfb dependent
	 */
	pScreen->InstallColormap	= nfbInstallColormap;
	pScreen->UninstallColormap	= nfbUninstallColormap;
	pScreen->ListInstalledColormaps = nfbListInstalledColormaps;
	pScreen->StoreColors		= nfbStoreColors;

	pScreen->blackPixel		= 0;
	pScreen->whitePixel		= 1;
	pScreen->SaveScreen		= nfbSaveScreen;

	pNfb->installedCmap	= NULL;
        pNfb->SetColor		= qvisSetColor;
        pNfb->LoadColormap	= genLoadColormap;
        pNfb->BlankScreen	= qvisBlankScreen;

	pNfb->pixmap = *(PixmapPtr)(pScreen->devPrivate);
	xfree(pScreen->devPrivate);
	pScreen->devPrivate = (pointer)pNfb;

	return TRUE;
}								/* S002 ^^^ */
