/*
 * @(#) dfbCmap.c 12.1 95/05/09 SCOINC
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
 * dfbCmap.c
 *
 * dfb colormap routines
 */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"

#include "dfbScrStr.h"

extern int TellLostMap(), TellGainedMap();

/*
 * Set a list of entries in the color map.
 * If there's no grafinfo routine, assume a VGA compatible adaptor
 */
static void
dfbUpdateColormap(dfbPriv, index, count, rmap, gmap, bmap)
    dfbScrnPrivPtr dfbPriv;
    int	index, count;
    unsigned char *rmap, *gmap, *bmap;
{

    if (dfbPriv->SetColorFunc) {
	while (count--) {
	    grafRunFunction(dfbPriv->pGraf, dfbPriv->SetColorFunc, NULL, 	
			    index, rmap[index], gmap[index], bmap[index]);
	    index++;
	}
    } else {  /* VGA */
	outb(0x3C8, index);
	while (count--) {
	    outb(0x3C9, rmap[index]);
	    outb(0x3C9, gmap[index]);
	    outb(0x3C9, bmap[index]);
	    index++;
	}
    }
}

void
dfbStoreColors(cmap, ndef, pdefs)
    ColormapPtr	cmap;
    int		ndef;
    xColorItem	*pdefs;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(cmap->pScreen);
    unsigned char rmap[256], gmap[256], bmap[256];
    xColorItem  expanddefs[256];
    int i;

    if (dfbPriv->inGfxMode && cmap == dfbPriv->currentCmap) {
	if ((cmap->pVisual->class | DynamicClass) == DirectColor) {
	    ndef = cfbExpandDirectColors(cmap, ndef, pdefs, expanddefs);
	    pdefs = expanddefs;
	}
	while (ndef--) {
	    i = pdefs->pixel;
	    rmap[i] = pdefs->red   >> dfbPriv->dacShift;
	    gmap[i] = pdefs->green >> dfbPriv->dacShift;
	    bmap[i] = pdefs->blue  >> dfbPriv->dacShift;
	    dfbUpdateColormap(dfbPriv, i, 1, rmap, gmap, bmap);
	    pdefs++;
	}
    }
}

/*
 * Called by dfbInstallColormap() && dfbSetGraphics()
 */
void
dfbStoreColormap(dfbPriv)
    dfbScrnPrivPtr dfbPriv;
{
    ColormapPtr cmap = dfbPriv->currentCmap;
    unsigned char rmap[256], gmap[256], bmap[256];
    VisualPtr pVisual;
    Entry *pent;
    int i;

    if (cmap == NULL)
	return;

    pVisual = cmap->pVisual;
    if ((pVisual->class | DynamicClass) == DirectColor) {
	for (i = 0; i < 256; i++) {
	    pent = &cmap->red[(i&pVisual->redMask)     >> pVisual->offsetRed];
	    rmap[i] = pent->co.local.red   >> dfbPriv->dacShift;

	    pent = &cmap->green[(i&pVisual->greenMask) >> pVisual->offsetGreen];
	    gmap[i] = pent->co.local.green >> dfbPriv->dacShift;

	    pent = &cmap->blue[(i&pVisual->blueMask)   >> pVisual->offsetBlue];
	    bmap[i] = pent->co.local.blue  >> dfbPriv->dacShift;
	}
    } else {
	for (i=0, pent=cmap->red; i < pVisual->ColormapEntries; i++, pent++) {
	    if (pent->fShared) {
		rmap[i] = pent->co.shco.red->color   >> dfbPriv->dacShift;
		gmap[i] = pent->co.shco.green->color >> dfbPriv->dacShift;
		bmap[i] = pent->co.shco.blue->color  >> dfbPriv->dacShift;
	    } else {
		rmap[i] = pent->co.local.red   >> dfbPriv->dacShift;
		gmap[i] = pent->co.local.green >> dfbPriv->dacShift;
		bmap[i] = pent->co.local.blue  >> dfbPriv->dacShift;
	    }
	}
    }

    dfbUpdateColormap(dfbPriv, 0, 256, rmap, gmap, bmap);
}

void
dfbInstallColormap(cmap)
    ColormapPtr	cmap;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(cmap->pScreen);
    ColormapPtr	oldcmap = dfbPriv->currentCmap;

    if (cmap == oldcmap)
	return;

    if (oldcmap)
	WalkTree(cmap->pScreen, TellLostMap, (pointer) &(oldcmap->mid));

    dfbPriv->currentCmap = cmap;
    if (dfbPriv->inGfxMode)
	dfbStoreColormap(dfbPriv);

    WalkTree(cmap->pScreen, TellGainedMap, (pointer) &(cmap->mid));
}

void
dfbUninstallColormap(cmap)
    ColormapPtr	cmap;
{
    dfbScrnPrivPtr dfbPriv = DFB_SCREEN_PRIV(cmap->pScreen);

    if (cmap == dfbPriv->currentCmap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap;

	    defMap = (ColormapPtr) LookupIDByType(defMapID, RT_COLORMAP);
	    if (defMap)
		(*cmap->pScreen->InstallColormap)(defMap);
	    else
	        ErrorF("dfb: Can't find default colormap\n");
	}
    }
}

int
dfbListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = DFB_SCREEN_PRIV(pScreen)->currentCmap->mid;
    return (1);
}

