/*
 * @(#) genCmap.c 11.1 97/10/22
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

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
#include "misc.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "regionstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "genDefs.h"
#include "genProcs.h"


/*
 * genLoadColormap() - load a PseudoColor colormap
 *
 * Replace this routine if you have and need a faster way to load
 * an entire colormap than going through SetColor for each entry.
 */
void
genLoadColormap(pmap)
    register ColormapPtr pmap;
{
    ScreenPtr pScreen;
    void (*SetColor)();
    register VisualPtr pVisual;
    int i, nents;

    if (pmap == NULL)
	return;

    pScreen  = pmap->pScreen;
    SetColor = (void (*)())NFB_SCREEN_PRIV(pScreen)->SetColor;
    pVisual  = pmap->pVisual;

    if ((pVisual->class | DynamicClass) == DirectColor) {

	nents = 1 << pVisual->nplanes;
	for (i = 0; i < nents; i++) {
	    int r, g, b;

	    r = (i & pVisual->redMask)   >> pVisual->offsetRed;
	    g = (i & pVisual->greenMask) >> pVisual->offsetGreen;
	    b = (i & pVisual->blueMask)  >> pVisual->offsetBlue;
	    (*SetColor)(0, i, pmap->red[r].co.local.red,
			pmap->green[g].co.local.green,
			pmap->blue[b].co.local.blue,
			pScreen);
	}

    } else {

	register Entry *pent;

	nents = pVisual->ColormapEntries;
	for (pent = pmap->red, i = 0;  i < nents;  i++, pent++) {
	    if (pent->fShared) {
		(*SetColor)(0, i, pent->co.shco.red->color,
			    pent->co.shco.green->color,
			    pent->co.shco.blue->color,
			    pScreen);
	    }
	    else {
		(*SetColor)(0, i, pent->co.local.red,
			    pent->co.local.green,
			    pent->co.local.blue,
			    pScreen);
	    }
	}

    }
}
