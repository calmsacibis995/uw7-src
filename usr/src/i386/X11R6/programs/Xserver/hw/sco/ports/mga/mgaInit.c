
/*
 * @(#) mgaInit.c 11.1 97/10/22
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
 *	SCO Modifications
 *
 *
 *	S007	Fri Jun 20 10:48:21 PDT 1997	hiramc@sco.COM
 *	- fix memory leak in reset cycle, must call nfbCloseText8
 *	- since we are using nfbInitializeText8
 *	S006	Wed Jun 28 10:09:42 PDT 1995	brianm@sco.com
 *		modified the 16 bit modes to 15 bit.
 *	S005	Thu Jun  1 16:55:20 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *	S004	Tue Nov  8 09:24:03 PST 1994	brianm@sco.com
 *		Fix for bug SCO-59-4421. Block cursor on PCI version
 *		of MGA 2+ card.  Set MGAHWCURSOR=1 to get HW cursor.
 *	S003	Wed May 25 16:37:52 PDT 1994	hiramc@sco.COM
 *		Actually, mgaDownloadFont8 was declared incorrectly.
 *	S002	Wed May 25 14:49:31 PDT 1994	hiramc@sco.COM
 *		Can't use NFB_POLYBRES when on Tbird, not there.
 *		And don't pass the address of mgaDownloadFont8 to
 *		nfbInitializeText8, it already is an address.
 *	S001	Fri May 20 12:25:58 PDT 1994	hiramc@sco.COM
 *		Code in wrong order, was using an uninitialized
 *		structure (titan).  And the exit to SetText when
 *		not in graphics mode caused the server to fail
 *		on resets, removed that test.
 */

/*
 * mgaInit.c
 *
 * Probe and Initialize the mga Graphics Display Driver
 */

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

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "mgaScrStr.h"
#include "mgaProcs.h"

#include "mgainfo.h"

extern scoScreenInfo mgaSysInfo;
extern VisualRec mgaVisual8, mgaVisual15, mgaVisual24;	/* S006 */
extern nfbGCOps mgaSolidPrivOps;

int mgaGeneration = -1;
int mgaScreenPrivateIndex = -1;

/*
 * mgaProbe() - test for graphics hardware
 *
 * This routine tests for your particular graphics board, and returns
 * true if its present, false otherwise.
 */
Bool
mgaProbe(ddxDOVersionID version,ddxScreenRequest *pReq)
{
    return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}


/*
 * mgaInitHW()
 *
 * Initialize hardware that only needs to be done ONCE.  This routine will
 * not be called on a screen switch.  It may just call mgaSetGraphics()
 */
Bool
mgaInitHW(pScreen)
    ScreenPtr pScreen;
{
    grafData *grafinfo = DDX_GRAFINFO(pScreen);
    mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
    VOLATILE register mgaTitanRegsPtr titan;
    mgaRegsPtr regs;
    Vidparm *vparm;
    unsigned long count;
    unsigned long *tloc;
#ifdef DEBUG_PRINT
ErrorF("in mgaInitHw\n");
#endif
    /*
     * map memory 
     */

    if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &mgaPriv->regs))
    {
	ErrorF ("mga: Missing MEMORY in grafinfo file.\n");
	return (FALSE);
    }

    mgaPriv->mapBase = mgaPriv->regs;	/* save base of mapped area */

    	/* if vlb bus, find actual address */
	/* map it in, NOTE THAT THIS IS UNDOCUMENTED ANYWHERE but code */
    if(mgaPriv->isvlb)
    {
	outb(0x46e8, 0);    /*what is this????, taken from matrox code*/
	regs = mgaPriv->regs;
	tloc = (unsigned long *) regs;
	tloc += (0x2010 / sizeof(unsigned long));
	*tloc = 0xac000;	/* just in case we at ac000 */
	for(count = 0; count < 7; ++count, ++regs)
	{
	    if(regs->titan.config != ((unsigned long) -1))
		break;
	    if(count == 0)
		regs = &mgaPriv->regs[6];	/* next spot is at index 7 */
	}
	*tloc = 0x80000000 | 0xac000; /*un map */
	outb(0x46e8, 8);    /* diddo */
	if(count == 7)
	{
	    ErrorF ("mga: board not found\n");
	    return(FALSE);
	}
	mgaPriv->regs = regs;
    }

    titan = &mgaPriv->regs->titan; 	/* S001 found this below */

    /* find default vparm for this resolution */

    for(vparm = ((combined *)mgaDefaultVidset)->parms; vparm->Resolution != -1;
	++vparm)
    {
	if((mgaPriv->depth == vparm->PixWidth) &&
	   (mgaPriv->width == vparm->VidsetPar[0].HDisp) &&
	   (mgaPriv->height == vparm->VidsetPar[0].VDisp))
	    break;
    }

    /* see if we support this resolution */

    if (vparm->Resolution == -1)
    {
	ErrorF ("mga: Unsupported resolution.\n");
	return (FALSE);
    }

    /* make a vid from this vparm */
    make_vidtab(vparm, mgaPriv->vidtab);

    /* fill in parms here from grafinfo file */

    for(count = 0; count < 29; ++count)
	grafGetInt(grafinfo,
                   mgaPriv->vidtab[count].name,
                   &mgaPriv->vidtab[count].valeur);

    /* set the mapped frame buffer base address */
    mgaPriv->fbBase = (unsigned char *) &mgaPriv->regs->srcwin;

    mgaSetGraphics(pScreen);	/* send us into graphics mode */

    /* we need at least one scanline of offscreen memory */

    if(mgaPriv->offscreenSize < mgaPriv->bstride) /* if not enough memory */
    {
	ErrorF("not enough memory to support this resolution.\n");
	return(FALSE);
    }
#ifdef DEBUG_PRINT
ErrorF("out of mgaInitHw\n");
#endif
    return (TRUE);
}

	/*	S003	*/
extern void mgaDownloadFont8(unsigned char **, int, int, int, int, int,
				ScreenPtr);

/*
 * mgaInit() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
mgaInit(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
        grafData *grafinfo = DDX_GRAFINFO(pScreen);
	nfbScrnPrivPtr pNfb;
	mgaPrivatePtr mgaPriv;
	int width, height, depth, mmx, mmy, hwcursorflag;
        int subclass;
	VisualPtr vis;

#if 0
	ErrorF("mgaInit(%d)\n", index);
#endif

	if (mgaGeneration != serverGeneration)
	    {
	    mgaScreenPrivateIndex = AllocateScreenPrivateIndex();
	    mgaGeneration = serverGeneration;
	    }

	mgaPriv = (mgaPrivatePtr)xalloc(sizeof(mgaPrivate));
	MGA_PRIVATE_DATA(pScreen) = mgaPriv;


	/* Get mode and monitor info */
	if ( !grafGetInt(grafinfo, "PIXWIDTH",  &width)  ||
	     !grafGetInt(grafinfo, "PIXHEIGHT", &height) ||
	     !grafGetInt(grafinfo, "DEPTH", &depth)) {
	    ErrorF("mga: can't find pixel info in grafinfo file.\n");
	    return FALSE;
	}

	mgaPriv->isvlb = 0;				/* assume not vlb */
	grafGetInt(grafinfo, "VLB", &mgaPriv->isvlb);	/* is this vlb? */
	hwcursorflag = 0;			/* S004 don't assume hw curs */
	grafGetInt(grafinfo, "MGAHWCURSOR", &hwcursorflag); /* use hw curs ? */

	if((depth > 16) && (depth <= 32)) depth = 32;
	if((depth > 8) && (depth <= 16)) depth = 16;	/* S006 */

	mgaPriv->bstride = width * (depth >> 3);	/* in bytes */
	mgaPriv->pstride = width;			/* in pixels */
	mgaPriv->width = width;
	mgaPriv->height = height;
	mgaPriv->depth = depth;				/* in bits */
	mgaPriv->bpp = depth >> 3;			/* in bytes */
	mgaPriv->ydstorg = 0;

	/* initial clip window of whole screen */

	mgaPriv->clipXL = 0;
	mgaPriv->clipXR = width;
	mgaPriv->clipYT = 0;
	mgaPriv->clipYB = height * width;

	mmx = 300; mmy = 300;  /* Reasonable defaults */

	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

	if (!nfbScreenInit(pScreen, width, height, mmx, mmy))
		return FALSE;

	/* get proper visual for this depth */

	switch(depth)
	{
	    case 8:
		vis = &mgaVisual8;
		break;
	    case 16:
		vis = &mgaVisual15;			/* S006 */
		break;
	    case 32:
		vis = &mgaVisual24;
		break;
	}

	if (!nfbAddVisual(pScreen, vis))
		return FALSE;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &mgaSolidPrivOps;
	pNfb->SetColor		 = mgaSetColor;
	pNfb->LoadColormap	 = mgaLoadColormap;
	pNfb->BlankScreen	 = mgaBlankScreen;
	pNfb->ValidateWindowPriv = mgaValidateWindowPriv;
	pNfb->clip_count 	 = 1;

	if (!mgaInitHW(pScreen))
	  return FALSE;

#ifdef DEBUG_PRINT
ErrorF("this point is after if (!mgaInitHW\n");	
#endif
        /*
	 * Call one of the following
	 *
	 * scoSWCursorInitialize(pScreen);
	 * mgaCursorInitialize(pScreen);
	 */
	if((mgaPriv->dactype == Info_Dac_Chameleon) ||
	   (mgaPriv->dactype == Info_Dac_ATT) ||
	   (mgaPriv->dactype == Info_Dac_Sierra) ||
	   (hwcursorflag == 0))
	    scoSWCursorInitialize(pScreen);
	else
	    mgaCursorInitialize(pScreen);

	/*
	 * This should work for most cases.
	 */
	if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
		cfbCreateDefColormap(pScreen)) == 0 )
	    return FALSE;

#ifdef DEBUG_PRINT
ErrorF("this point is after cfbCreateDefColormap\n");
#endif
	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &mgaSysInfo);
#ifdef DEBUG_PRINT
ErrorF("this point is after scoSysInfoInit\n");
#endif

	/*
	 * Set any NFB runtime options here
	 */
#ifdef agaII
	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);	/*	S002 */
#else
	nfbSetOptions(pScreen, NFB_VERSION, NFB_POLYBRES, 0);
#endif

	nfbInitializeText8(pScreen, 2, 32, 32, mgaDownloadFont8, NULL );
#ifdef DEBUG_PRINT
ErrorF("out of mgaInit\n");
#endif
	return TRUE;
}

/*
 * mgaFreeScreenData()
 *
 * Anything you allocate in mgaInit() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
Bool
mgaFreeScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);

	nfbCloseText8(pScreen);		/*	S007	*/
	xfree(mgaPriv);

	return TRUE;
}

