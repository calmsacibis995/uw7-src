/*
 *  @(#) wd33Init.c 11.1 97/10/22
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
 * wd33Init.c
 *
 * Probe and Initialize the wd33 Graphics Display Driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *      S002    Thu 18-Aug-1993 edb@sco.com
 *		remove ->fillColor
 */
/* #define DEBUG_PRINT */

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

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"
#include "wdBankMap.h"

#include <sys/types.h>

unsigned int wd33ScreenPrivateIndex = -1;
static   int wd33ServerGeneration = -1;

extern scoScreenInfo wd33SysInfo;
extern VisualRec wd33Visual, wd33Visual15, wd33Visual16;
extern nfbGCOps wd33SolidPrivOps;


/*
 * wd33Probe() - test for graphics hardware
 *
 * This routine tests for your particular graphics board, and returns
 * true if its present, false otherwise.
 */
Bool
wd33Probe(ddxDOVersionID version,ddxScreenRequest *pReq)
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
		ErrorF("wd33: unsupported screen depth: %d\n", pReq->dfltDepth);
		return(FALSE);
	}
}

/*
 * wd33InitHW()
 *
 */
void
wd33InitHW( pScreen )
	ScreenPtr pScreen;
{
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );

	wd33SetGraphics(pScreen);
	WD_MAP_RESET(wdPriv);			/* reset bank-mapping */
}

/*
 * wd33Init() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
wd33Init(index, pScreen, argc, argv)
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


#ifdef DEBUG_PRINT
	ErrorF("wd33Init(%d)\n", index);
#endif

        depth = pInfo->pRequest->dfltDepth;

	/* Get mode and monitor info */
        if ( !grafGetInt(grafinfo, "PIXWIDTH", &pixwidth) ||
             !grafGetInt(grafinfo, "PIXHEIGHT", &pixheight) )
        {
	        ErrorF("wd33: can't find pixel info in grafinfo file.\n");
	        return FALSE;
	}

	mmx = 300; mmy = 300;  /* Reasonable defaults */

	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

	if (!nfbScreenInit(pScreen, pixwidth, pixheight, mmx, mmy))
		return FALSE;

	pScreen->QueryBestSize = wd33QueryBestSize;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->BlankScreen	 = wd33BlankScreen;
	pNfb->ValidateWindowPriv = wd33ValidateWindowPriv;
	pNfb->SetColor		 = NoopDDA;
	pNfb->LoadColormap	 = NoopDDA;

	switch (depth)
	{
	case 8:
		pNfb->protoGCPriv->ops	= &wd33SolidPrivOps;
  		pNfb->ClearFont         = wd33ClearFont; 
		pNfb->SetColor		= wd33SetColor;
		pNfb->LoadColormap	= wd33LoadColormap;
		pVis			= &wd33Visual;
		break;

	case 15:
  		pNfb->protoGCPriv->ops	= &wd33SolidPrivOps; 
  		pNfb->ClearFont         = wd33ClearFont;
		pVis			= &wd33Visual15;
		break;

	case 16:
  		pNfb->protoGCPriv->ops	= &wd33SolidPrivOps; 
  		pNfb->ClearFont         = wd33ClearFont;
		pVis			= &wd33Visual16;
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

        /*    Allocate and initialize wd33 screen priv */

	/*        get frame buffer address */
	if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &fb)) {
		ErrorF("wd33: missing MEMORY in grafinfo file.\n");
		return FALSE;
	}
	if ( serverGeneration != wd33ServerGeneration ) {   
	        wd33ServerGeneration = serverGeneration;
		wd33ScreenPrivateIndex = AllocateScreenPrivateIndex();
		if (wd33ScreenPrivateIndex < 0)
			return FALSE;
	}
	if ((wdPriv = (wdScrnPrivPtr)xalloc(sizeof(wdScrnPriv))) == NULL)
		return FALSE;

        pScreen->devPrivates[wd33ScreenPrivateIndex].ptr = (pointer)wdPriv;

	wdPriv->fbBase = (pointer)fb;

	/* init memory layout */

	if (! wd33InitMem(pScreen, pixwidth, pixheight, depth))
        {
		ErrorF("wd33: insufficient memory for mode\n");
		return FALSE;
	}
                
	wd33InitHW( pScreen );

/*  
 *      Buffer for monoimages fonts and glyphs 
 */
        if (wdPriv->glCacheHeight > 0)
		wd33InitGlCache(wdPriv);

#ifdef NOT_YET
        if (wdPriv->cursorCacheHeight > 0)
		wd33CursorInitialize(pScreen);
	else
#endif
		scoSWCursorInitialize(pScreen);

	if ( cfbCreateDefColormap(pScreen) == 0 )
	    return FALSE;

	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &wd33SysInfo);

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);

	return TRUE;

}


/*
 * wd33InitMem() - init memory layout
 */
Bool
wd33InitMem(pScreen, width, height, depth)
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
        int fillStart, tilePrefStart, align;
        unsigned char  bits ;
        static int memSizes[4] = { 0x40000, 0x40000, 0x80000, 0x100000 };
                         /*          256kB   256kB   512kB     1Meg     */
                         /* Bits 6,7  00      01      10       11       */
        /*
         *  Memory size configured by Video BIOS in PR1 bits 6,7 
         *  no specification for 2 Megs !!
         */
        outb( 0x3CE, 0x0B );
        bits = inb( 0x3CF );
        wdSizeMem = memSizes[bits >> 6];

#ifdef DEBUG_PRINT
        ErrorF("Size of Video memory: %d Bytes \n", wdSizeMem);
#endif

	/*
	 * screen display is at beginning of frame buffer
         *   fbBase = the virtual base address of video memory
         *   all other addresses relative to fbBase
	 */
        /*
         * The drawing engine calculates the framebuffer address
         * from coordinates based on the setting of CNTRL_2_REG bit 11:10
         * and the ROW_PITCH in ENG_2 register ( ref SetGraphics() )
         *
         * If we calculate the framebuffer address we use
         * fbStride and pixBytes
         */
        switch( depth )
        {
            case 8:  wdPriv->bit11_10 = PACKED_8_BITS;
                     wdPriv->rowPitch = width;
                     wdPriv->fbStride = width;
                     wdPriv->pixBytes   = 1;
                     wdPriv->planeMaskMask = 0xFF;
                     break;
            case 15: wdPriv->bit11_10 = PACKED_16_BITS;
                     wdPriv->rowPitch = width;
                     wdPriv->fbStride = width * 2;
                     wdPriv->pixBytes   = 2;
                     wdPriv->planeMaskMask = 0x7FFF;
                     break;
            case 16: wdPriv->bit11_10 = PACKED_16_BITS;
                     wdPriv->rowPitch = width;
                     wdPriv->fbStride = width * 2;
                     wdPriv->pixBytes   = 2;
                     wdPriv->planeMaskMask = 0xFFFF;
                     break;
            case 24: wdPriv->bit11_10 = PACKED_8_BITS;
                     wdPriv->rowPitch = width * 3;
                     wdPriv->fbStride = width * 3;
                     wdPriv->pixBytes   = 3;
                     wdPriv->planeMaskMask = 0xFFFFFF;
                     break;
        }
	/*
	 * for the purposes of this routine
	 * a depth of 15 should be treated as 16.
	 */
	if( depth == 15 )
	    depth = 16;

    
        stride = wdPriv->fbStride; 
        free_lines = wdSizeMem / stride;
        if( free_lines < height ) return 0;  /* not enough memory */

	wdPriv->fbSize = wdSizeMem;
        free_lines -= height;
        line += height;

	/*
         *    Fast tileBlit 64(128) Bytes, and 64(128) bytes for solid color fill 
         * has to be 64(128) Byte aligned                                   
         * stored linear - takes just one line                             
         */
        align                 = wdPriv->pixBytes * 64;
        fillStart             = ((line    * stride) + align-1 ) & ~(align-1);
        tilePrefStart         = fillStart + align;
        wdPriv->fillX         = (fillStart % stride) / wdPriv->pixBytes;
        wdPriv->fillY         = fillStart / stride;
        wdPriv->tilePrefX     = (tilePrefStart % stride) / wdPriv->pixBytes;
        wdPriv->tilePrefY     = tilePrefStart / stride;
        free_lines--;
        line++;

	/*
         *       Rectangular area for big tiles, if we have the room  
         * Try to allocate space for a tile with height 32 ( need 64 lines )
         * If less than 16 left don't allow caching                        
         */
        nrlines = (free_lines / 2) & ~1;     /* leave 1/2 for glyphs and text  */
        if(nrlines > 64 ) nrlines = 64;                     
        if(nrlines < 16 ) nrlines =  0;         
        wdPriv->tileX         =  0;
        wdPriv->tileY         = line;
        wdPriv->tileMaxWidth  = wdPriv->rowPitch /2;
        wdPriv->tileMaxHeight = nrlines / 2;
        wdPriv->tileSerial    = 0;
        free_lines -= nrlines;
        line       += nrlines; 

#ifdef NOT_YET
	/*
         * Cursor cache  max 1024 Bytes
         * ref. WD doc  Cursor address mapping 8.6.1
         */
	memAddr  = line * stride;
	free_mem = free_lines * stride;
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
#endif

	/*
         *   Glyph caching
         */

	if( free_lines >= 8 )
	{
	    wdPriv->glCacheY      = line;
	    wdPriv->glCacheHeight = free_lines;
	    wdPriv->glCacheWidth  = width;
	}
	else
	    wdPriv->glCacheHeight = 0;

#ifdef DEBUG_PRINT
        if( wdPriv->tileMaxHeight > 0)
            ErrorF("Using tile caching, tileMaxHeight = %d\n",
                    wdPriv->tileMaxHeight );
        if( wdPriv->glCacheHeight > 0 )
            ErrorF("Using glyph caching, glCacheSize = %d x %d\n",
                    wdPriv->glCacheWidth, wdPriv->glCacheHeight );
#endif
	return( 1 );
}

/*
 * wd33CloseScreen()
 */
Bool
wd33CloseScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
        wdScrnPrivPtr wdPriv;
        wdPriv = WD_SCREEN_PRIV(pScreen);

        if (wdPriv->glCacheHeight > 0 )
		wd33DeallocGlCache( wdPriv );

        xfree( wdPriv );

	return ( TRUE );
}

/*
 *   modified from mfbQueryBestSize
 */
void
wd33QueryBestSize(class, pwidth, pheight, pScreen)
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
