/*
 *  @(#) wd33Cmap.c 11.1 97/10/22
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
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 */
/*
 *   wd33Cmap.c
 */

#include <sys/types.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "ddxScreen.h"
#include "colormapst.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbScrStr.h"

/*
 * wd33SetColor() - set an entry in a PseudoColor colormap
 *	cmap - machine dependent colormap number
 *	index - offset into colormap
 *	r,g,b - the rgb tripple to enter in this colormap entry
 *	pScreen - pointer to X's screen struct for this screen.  Simple
 *		implementations can initially ignore this.
 *
 * NOTE: r,g,b are 16 bits.  If you have a device that uses 8 bits you
 *	should use the MOST SIGNIFICANT 8 bits.
 */
void
wd33SetColor(cmap, index, r, g, b, pScreen)
	unsigned int cmap;
	unsigned int index;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	ScreenPtr pScreen;
{
       register VisualPtr pVisual;
       int dacshift;

#ifdef DEBUG_PRINT
	ErrorF("wd33SetColor(cmap=%d, index=%d, r=%d, g=%d, b=%d)\n",
		cmap, index, r, g, b);
#endif

       pVisual = NFB_SCREEN_PRIV(pScreen)->installedCmap->pVisual;
       dacshift = 16 - pVisual->bitsPerRGBValue;	/* 16 bit colors */

       outb(0x3C8, index);
       outb(0x3C9, r >> dacshift);
       outb(0x3C9, g >> dacshift);
       outb(0x3C9, b >> dacshift);
}


/*
 * Called through nfbScrnPriv->LoadColormap && wd33SetGraphics()
 */
void
wd33LoadColormap(cmap)
    register ColormapPtr cmap;
{   
    register int i;
    register Entry *pent;
    register VisualPtr pVisual;
    u_char rmap[256], gmap[256], bmap[256];
    int dacshift;

#ifdef DEBUG_PRINT
	ErrorF("wd33LoadColormap(cmap=%d)\n",cmap );
#endif

    if (cmap == NULL)
	return;

    pVisual = cmap->pVisual;
    dacshift = 16 - pVisual->bitsPerRGBValue;	/* 16 bit colors */
    if ((pVisual->class | DynamicClass) == DirectColor) {
	for (i = 0; i < 256; i++) {
	    pent = &cmap->red[(i & pVisual->redMask) >>
				pVisual->offsetRed];
	    rmap[i] = pent->co.local.red >> dacshift;
	    pent = &cmap->green[(i & pVisual->greenMask) >>
				pVisual->offsetGreen];
	    gmap[i] = pent->co.local.green >> dacshift;
	    pent = &cmap->blue[(i & pVisual->blueMask) >>
			       pVisual->offsetBlue];
	    bmap[i] = pent->co.local.blue >> dacshift;
	}
    } else {
	for (i = 0, pent = cmap->red;
	     i < pVisual->ColormapEntries;
	     i++, pent++) {
	    if (pent->fShared) {
		rmap[i] = pent->co.shco.red->color >> dacshift;
		gmap[i] = pent->co.shco.green->color >> dacshift;
		bmap[i] = pent->co.shco.blue->color >> dacshift;
	    }
	    else {
		rmap[i] = pent->co.local.red >> dacshift;
		gmap[i] = pent->co.local.green >> dacshift;
		bmap[i] = pent->co.local.blue >> dacshift;
	    }
	}
    }
    /*   Dump it into the registers  */
    outb(0x3C8, 0);
    for (i = 0; i < 256; i++) {
       outb(0x3C9, rmap[i]);
       outb(0x3C9, gmap[i]);
       outb(0x3C9, bmap[i]);
    }
}
