/*
 *  @(#) wdInit.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
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
 * wdInit.c
 *
 * Probe and Initialize the wd Graphics Display Driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Fri 09-Oct-1992	buckm@sco,com
 *		Use new grafinfo memory interface:
 *		- MAP_CLASS's are done for us;
 *		- grafGetMemInfo returns our frame buffer address.
 *		Get rid of unused fbVirToPhys field in screen private.
 *		Straighten out CloseScreen 'wrapping'.
 *		GCOpsCache'ing calls are done by nfb now.
 *		Add call to nfbSetOptions() at end of wdInit().
 *      S002    Wdn 14-Oct-1992 edb@sco.com
 *              changes to implement WD90C31 cursor
 *	S003	Thu  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *      S004    Thu 05-Oct-1992 edb@sco.com
 *              implement tiled and stippled GC ops
 *      S005    Thu 05-Oct-1992 edb@sco.com
 *              nfbSetOptions has 2 more arguments
 *	S006	Mon 09-Nov-1992	edb@sco.com
 *              minimize tile loading by checking for serialNumber
 *      S007    Mon 23-Nov-1992 edb@sco.com
 *              Use hardware cursor
 *      S008    Sun 20-Dec-1992 buckm@sco.com
 *              Init tileSerial to 0
 *      S008    Tue 09-Feb-1993 buckm@sco.com
 *		Add initialization for 15,16,24 bit modes.
 *		Check for RGBBITS in wdInit.
 *		Get rid of GC caching.
 *		Reset bank-mapping.
 *		Add ifdef'd TE8 text code for post-AGA2.
 *      S009    Wdn 07-Apr-92 edb@sco.com
 *              Change memory allocation for font caching
 *      S010    Fri 07-May-1993 edb@sco.com
 *              Change memory allocation for glyph caching
 *      S011    Tue 25-May-1993 edb@sco.com
 *              nfbInitializeText8  needs to be called also when no
 *              caching memory left 
 *      S012    Thu 27-May-1993 edb@sco.com
 *		Changes to do glyph caching for 15 16 & 24 bits
 *      S013    Tue 01-Jun-1993 edb@sco.com
 *		Don't check for USETEXT8 in grafinfo
 *		text8 will be used in Everest whenever enough cache memory
 *		Comment out NOT_FOR_AGAII
 *      S014    Wdn 01-Sep-1993 edb@sco.com
 *		ifndef agaII  for code incompatible with agaII
 *	S015	Wed 25-Jan-1995 brianm@sco.com
 *		added in check for USE_BITBLT in grafinfo file.
 *		This will change some of the 8 bit cards to use gen
 *		routines where possible, and wdDrawDumbImage for DrawImage.
 *	S016	Fri Jun 20 10:55:55 PDT 1997	hiramc@sco.COM
 *		fix memory leak in reset cycle, must call nfbCloseText8
 *		since we are using nfbInitializeText8
 *              
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "grafinfo.h"
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

#include "genDefs.h"
#include "genProcs.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"
#include "wdBankMap.h"

#include <sys/types.h>

unsigned int wdScreenPrivateIndex = -1;
static   int wdServerGeneration = -1;

extern scoScreenInfo wdSysInfo;
extern VisualRec wdVisual, wdVisual15, wdVisual16, wdVisual24;
extern nfbGCOps wdSolidPrivOps,   wdSolidPrivOps15,
		wdSolidPrivOps16, wdSolidPrivOps24;


/*
 * wdProbe() - test for graphics hardware
 *
 * This routine tests for your particular graphics board, and returns
 * true if its present, false otherwise.
 */
Bool
wdProbe(ddxDOVersionID version,ddxScreenRequest *pReq)
{
	switch (pReq->dfltDepth)
	{
	case 8:
	case 15:
	case 16:
	case 24:
		return ddxAddPixmapFormat(pReq->dfltDepth,
					  pReq->dfltBpp, pReq->dfltPad);

	default:
		ErrorF("wd: unsupported screen depth: %d\n", pReq->dfltDepth);
		return(FALSE);
	}
}

/*
 * wdInitHW()
 *
 * Template for machine dependent hardware initialization code
 * This should do everything it takes to get the machine ready to draw.
 */
void
wdInitHW( pScreen )
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );

	wdSetGraphics(pScreen);
	WD_MAP_RESET(wdPriv);			/* reset bank-mapping */
}

static int use_bitblt = 1;	/* S015  S016 */

/*
 * wdInit() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
wdInit(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
	ddxScreenInfo *pInfo = ddxActiveScreens[index];
        grafData* grafinfo = DDX_GRAFINFO(pScreen);
	nfbScrnPrivPtr pNfb;
        wdScrnPrivPtr wdPriv;
	VisualPtr pVis;
	int depth, pixwidth, pixheight, mmx, mmy;
        int fb;
        void (* wd_download )();
        void (* wd_clear )();
        static void iniExpandTab();




#ifdef DEBUG_PRINT
	ErrorF("wdInit(%d)\n", index);
#endif

        depth = pInfo->pRequest->dfltDepth;

	/* Get mode and monitor info */
        if ( !grafGetInt(grafinfo, "PIXWIDTH", &pixwidth) ||
             !grafGetInt(grafinfo, "PIXHEIGHT", &pixheight) )
        {
	        ErrorF("wd: can't find pixel info in grafinfo file.\n");
	        return FALSE;
	}

	mmx = 300; mmy = 300;  /* Reasonable defaults */

	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);
	grafGetInt(grafinfo, "USE_BITBLT", &use_bitblt);	/* S015 */

	if (!nfbScreenInit(pScreen, pixwidth, pixheight, mmx, mmy))
		return FALSE;

	pScreen->QueryBestSize = wdQueryBestSize;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->BlankScreen	 = wdBlankScreen;
	pNfb->ValidateWindowPriv = wdValidateWindowPriv;
	pNfb->SetColor		 = NoopDDA;
	pNfb->LoadColormap	 = NoopDDA;

	switch (depth)
	{
	case 8:
		pNfb->protoGCPriv->ops	= &wdSolidPrivOps;
		if (use_bitblt)		/* S015 */
			pNfb->ClearFont         = wdClearFont;
		pNfb->SetColor		= wdSetColor;
		pNfb->LoadColormap	= wdLoadColormap;
		pVis			= &wdVisual;
		break;

	case 15:
		pNfb->protoGCPriv->ops	= &wdSolidPrivOps15;
		if (use_bitblt)		/* S015 */
			pNfb->ClearFont         = wdClearFont;       /* S012 */
		pVis			= &wdVisual15;
		break;

	case 16:
		pNfb->protoGCPriv->ops	= &wdSolidPrivOps16;
		if (use_bitblt)		/* S015 */
			pNfb->ClearFont         = wdClearFont;       /* S012 */
		pVis			= &wdVisual16;
		break;

	case 24:
		pNfb->protoGCPriv->ops	= &wdSolidPrivOps24;
		if (use_bitblt)		/* S015 */
			pNfb->ClearFont         = wdClearFont;       /* S012 */
		pVis			= &wdVisual24;
		break;
	}

	if (depth == 8)
	{
		int rgbbits = 6;
		grafGetInt(grafinfo, "RGBBITS", &rgbbits);
		pVis->bitsPerRGBValue = rgbbits;
	}
	else
	{
		grafGetInt(grafinfo, "REDMASK", &pVis->redMask);
		grafGetInt(grafinfo, "GRNMASK", &pVis->greenMask);
		grafGetInt(grafinfo, "BLUMASK", &pVis->blueMask);

		pVis->offsetRed   = ffs(pVis->redMask) - 1;
		pVis->offsetGreen = ffs(pVis->greenMask) - 1;
		pVis->offsetBlue  = ffs(pVis->blueMask) - 1;
	}

	if (!nfbAddVisual(pScreen, pVis))
		return FALSE;

        /*    Allocate and initialize wd screen priv */

	/* S001 - get frame buffer address */
	if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &fb)) {
		ErrorF("wd: missing MEMORY in grafinfo file.\n");
		return FALSE;
	}
	if ( serverGeneration != wdServerGeneration ) {   
	        wdServerGeneration = serverGeneration;
		wdScreenPrivateIndex = AllocateScreenPrivateIndex();
		if (wdScreenPrivateIndex < 0)
			return FALSE;
	}
	if ((wdPriv = (wdScrnPrivPtr)xalloc(sizeof(wdScrnPriv))) == NULL)
		return FALSE;

        pScreen->devPrivates[wdScreenPrivateIndex].ptr = (pointer)wdPriv;

	wdPriv->fbBase = (pointer)fb;

	/* init memory layout */

	if (! wdInitMem(pScreen, pixwidth, pixheight, depth))
        {
		ErrorF("wd: insufficient memory for mode\n");
		return FALSE;
	}
                
/*  
 *      Expansion table and buffer for monoimages fonts and glyphs 
 */

        switch ( depth )                                 /* S009 vvvvvv */
        {
            case 8:
               wdPriv->pixBytes   = 1;
               break;

            case 15:
               wdPriv->pixBytes   = 2;
               wdPriv->planeMaskMask = 0x7F;
               break;

            case 16:
               wdPriv->pixBytes   = 2;
               wdPriv->planeMaskMask = 0xFF;
               break;

            case 24:   
               wdPriv->pixBytes   = 3;
               wdPriv->planeMaskMask = 0xFFFF;
               break;
         }
         if( depth > 8 )
         {
               wdPriv->expandBuff = (char * )Xalloc( WD_EXP_BUFF_SIZE );
               wdPriv->expandTab  = (char * )Xalloc( 256 * 3 );
               iniExpandTab( wdPriv );
         }
         else
         {
               wdPriv->expandBuff = (char * )0;
               wdPriv->expandTab  = (char * )0;
         }                                              /* S009 ^^^^^^ */
        
#ifndef agaII
	if (use_bitblt)		/* S015 */
	{
	        /*
		 * fast terminal emulator fonts
		 */
	        if (depth == 8 )
	               wd_download = wdDownloadFont8;
	        else
	               wd_download = wdDownloadFont15_24;       /* S009  */

		nfbInitializeText8(pScreen,                     /* S011 */
		   wdPriv->maxFontsInCache, WD_FONT_WIDTH, WD_FONT_HEIGHT,
			   wd_download, wdClearFont8);
	} else {		/* S015 Start */
		extern void genDrawFontText();
		extern void genDrawMonoGlyphs();
		extern void genDrawMonoImage();
		extern void genDrawOpaqueMonoImage();
		extern void wdDrawDumbImage();

		extern nfbWinOps wdWinOps, wdWinOps15, wdWinOps16, wdWinOps24;

		wdWinOps.DrawFontText = genDrawFontText;
		wdWinOps.DrawMonoGlyphs = genDrawMonoGlyphs;
		wdWinOps.DrawMonoImage = genDrawMonoImage;
		wdWinOps.DrawOpaqueMonoImage = genDrawOpaqueMonoImage;
		wdWinOps.DrawImage = wdDrawDumbImage;
	}			/* S015 End */
#endif

	wdInitHW( pScreen );

	wdPriv->fillColor = -1;

        if (wdPriv->glyphCache)
		wdInitGlCache(wdPriv);

        if (use_bitblt && wdPriv->cursorCache)	/* S015 */
		wdCursorInitialize(pScreen); /* S007 */
	else
		scoSWCursorInitialize(pScreen);

	if ( cfbCreateDefColormap(pScreen) == 0 )
	    return FALSE;

	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &wdSysInfo);

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);	/* S001 S005 */

	return TRUE;

}


/*
 * wdInitMem() - init memory layout
 */
Bool
wdInitMem(pScreen, width, height, depth)
	ScreenPtr pScreen;
	int width;
	int height;
	int depth;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pScreen);
        grafData* grafinfo = DDX_GRAFINFO(pScreen);
        unsigned long wdSizeMem, free_mem, memAddr;
	int stride;
        int line =0;
        int nrlines, free_lines;
        unsigned char  bits ;
        static int memSizes[4] = { 0x40000, 0x40000, 0x80000, 0x100000 };
                         /*          256kB   256kB   512kB     1Meg     */
                         /* Bits 6,7  00      01      10       11       */
        /*
         *  Memory size configured by Video BIOS in PR1 bits 6,7 
         */
        outb( 0x3CE, 0x0B );
        bits = inb( 0x3CF );
        wdSizeMem = memSizes[bits >> 6];

#ifdef DEBUG_PRINT
        ErrorF("Size of Video memory: %d Bytes \n", wdSizeMem);
#endif

	/*
	 * for the purposes of this routine
	 * a depth of 15 should be treated as 16.
	 */
	if( depth == 15 )
	    depth = 16;

	/*
	 * screen display is at beginning of frame buffer
         *   fbBase = the virtual base address of video memory
         *   all other addresses relative to fbBase
	 */
	stride = (width * depth) >> 3;
        free_lines = wdSizeMem / stride;
        if( free_lines < height ) return 0;  /* not enough memory */

	wdPriv->fbSize = wdSizeMem;
	wdPriv->fbStride = stride;
        free_lines -= height;
        line += height;

	/*
	 * allocate other rectangular areas next
	 */

	/* fast tileBlit 64 Bytes, has to be 64 Byte aligned               */
	/* large rectangular area for big tiles, if we have the room       */
        /* Try to allocate space for a tile with height 32 ( need 64 lines */
        /* If less than 16 left don't allow caching                        */
        /* One line is always needed for solid color filling               */

        nrlines = (free_lines -1) / 2;                        /* S010  vvvvvvvvv */
        if(nrlines > 64 ) nrlines = 64;                     
        if(nrlines < 16 ) nrlines =  0;                    
        nrlines &= ~1;
        wdPriv->fillStart     = ((line    * stride) + 63 ) & ~63;
        wdPriv->tilePrefStart = wdPriv->fillStart + 64;
        wdPriv->tileStart     =  (line+1) * stride;
        wdPriv->tileMaxWidth  = wdPriv->fbStride / ( depth / 8) /2;
        wdPriv->tileMaxHeight = nrlines / 2;
        wdPriv->tileSerial    = 0;
        free_lines -= nrlines + 1;
        line       += nrlines + 1;       /* S012 */             /* S010   ^^^^^^^ */

	/*
	 * allocate linear stuff now
	 */

	memAddr  = line * stride;
	free_mem = free_lines * stride;

	/* Cursor cache  max 1024 Bytes */
        /* ref. WD doc  Cursor address mapping 8.6.1 */

	if( depth == 8 )
	{
	    wdPriv->cursorCache   = memAddr;	/* multiple of 4     */
	    wdPriv->cursCacheSize = 1024;	/* true for all strides */
	    wdPriv->curCursor     = NULL;
	    memAddr  += 1024;
	    free_mem -= 1024;
	}
	else
	    wdPriv->cursorCache = 0;


#ifndef agaII
	/* space for downloaded terminal emulator fonts */                     /* S009 vvvvv */

        wdPriv->maxFontsInCache = free_mem  /
         ( WD_FONT_WIDTH * WD_FONT_HEIGHT * (depth >>3) * WD_CACHE_LOCATIONS );

	if( wdPriv->maxFontsInCache > 0 /* && grafQuery(grafinfo, "USETEXT8") */ )  /* S000  */
	{
            int cachesize;
	    wdPriv->fontBase   = memAddr;
            wdPriv->fontSprite = WD_FONT_WIDTH * WD_FONT_HEIGHT * (depth >>3);
            cachesize = wdPriv->fontSprite * WD_CACHE_LOCATIONS *
                        wdPriv->maxFontsInCache;
            memAddr  += cachesize;
	    free_mem -= cachesize;
	}
	else
#endif
            wdPriv->maxFontsInCache = 0;                                       /* S009 ^^^^^ */

	/* If not enough space for fontcaching do glyph caching */

	if( wdPriv->maxFontsInCache == 0  )                      /* S012  */
	{
	    wdPriv->glyphCache = memAddr;
	    wdPriv->glyphCacheSize = free_mem;
	}
	else
	    wdPriv->glyphCache = 0;

#ifdef DEBUG_PRINT
	if( wdPriv->maxFontsInCache != 0  )
            ErrorF("Using font caching, max %d fonts in cache\n", wdPriv->maxFontsInCache );
        else
            ErrorF("Using glyph caching, glyphCacheSize = %d\n",wdPriv->glyphCacheSize );
#endif
   
        if( free_mem < 0 ) return 0;

	return( 1 );
}

/*
 * wdCloseScreen()
 */
Bool
wdCloseScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
        wdScrnPrivPtr wdPriv;
        wdPriv = WD_SCREEN_PRIV(pScreen);

        if (wdPriv->glyphCache)
		wdDeallocGlCache( wdPriv );

#ifndef agaII			/*	S016	*/
	if (use_bitblt)		/* S016 */
		nfbCloseText8(pScreen);		/*	S016	*/
#endif				/*	S016	*/

        if(wdPriv->expandBuff != 0) {
            xfree( wdPriv->expandBuff );
            xfree( wdPriv->expandTab );
        }
        xfree( wdPriv );

	return ( TRUE ); 					/* S001 */
}

/*
 *   modified from mfbQueryBestSize
 */
void
wdQueryBestSize(class, pwidth, pheight, pScreen)
int class;
short *pwidth;
short *pheight;
ScreenPtr pScreen;
{
    unsigned width, test;

    switch(class)
    {
    case CursorShape:
	if (pScreen->rootDepth == 8)
	{
	    *pwidth  = 64;
	    *pheight = 64;
	}
	else
	    mfbQueryBestSize(class, pwidth, pheight, pScreen);
	break;
    case TileShape:
    case StippleShape:
	switch(pScreen->rootDepth)
	{
	case  8:
	    if (*pwidth < WD_TILE_WIDTH)
		*pwidth = WD_TILE_WIDTH;
	    break;
	case 15:
	case 16:
	    if (*pwidth < WD_TILE_WIDTH/2)
		*pwidth = WD_TILE_WIDTH/2;
	    break;
	case 24:
	    break;
	}
	if (*pheight < WD_TILE_HEIGHT)
	    *pheight = WD_TILE_HEIGHT;
	break;
    }
}
                                                              /*  S009 vvvvvvv */
/*
 *      Initialize expansion lookup table for depth > 8
 */
static void
iniExpandTab( wdPriv )
wdScrnPrivPtr wdPriv;
{
    int i, ind, bitpos;
    unsigned char bitmask;
    long bits32;
    char * bits8;
    char *tab = wdPriv->expandTab;
    int  pixb = wdPriv->pixBytes;
  
    for( ind = 0; ind < 256; ind ++ )
    {
         for( bitpos = 0, bitmask = 1, bits32 = 0;
              bitpos <= 7;
              bitpos++, bitmask <<= 1)
           bits32 |= ( ind & bitmask ) << (bitpos * (pixb -1) );

         bits8 = (unsigned char *)&bits32;
         for( i = 1; i<= pixb; i++)
             *tab++ = *bits8++; 
    }
}
                                                              /*  S009 ^^^^^^^ */
