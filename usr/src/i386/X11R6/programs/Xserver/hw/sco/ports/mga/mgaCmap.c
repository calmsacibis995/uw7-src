/*
 * @(#) mgaCmap.c 11.1 97/10/22
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
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

/*
 * mgaCmap.c
 *
 * Template for machine dependent colormap routines
 */

#include <sys/types.h>
#include "X.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "colormapst.h"

#include "mgaScrStr.h"

/*
 * mgaSetColor() - set an entry in a PseudoColor colormap
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
mgaSetColor(cmap, index, r, g, b, pScreen)
	unsigned int cmap;
	unsigned int index;
	unsigned short r;
	unsigned short g;
	unsigned short b;
	ScreenPtr pScreen;
{
	mgaPrivatePtr mgaPriv = MGA_PRIVATE_DATA(pScreen);
	register VOLATILE mgaDacsPtr dac = (mgaDacsPtr)&mgaPriv->regs->ramdacs;
	int i;

#ifdef DEBUG_PRINT
	ErrorF("mgaSetColor(cmap=%d, index=%d, r=%d, g=%d, b=%d)\n",
		cmap, index, r, g, b);
#endif

	/* depths of 16 and 32 use a static color table */

	if(mgaPriv->depth != 8)
	    return;

	/* all the supported dacs have the same offsets for index and data */

	dac->bt485.wadr_pal = index;
	dac->bt485.col_pal = r >> 8;
	dac->bt485.col_pal = g >> 8;
	dac->bt485.col_pal = b >> 8;
}

/*
 * mgaLoadColormap() - load a PseudoColor colormap
 *
 * A faster way to load an entire colormap
 * than going through SetColor for each entry.
 */
void
mgaLoadColormap(pmap)
	register ColormapPtr pmap;
{
	mgaPrivatePtr mgaPriv;
	register VOLATILE mgaDacsPtr dac;
	int i, nents;
#ifdef DEBUG_PRINT
	ErrorF("mgaLoadColorMap(pmap=%x)\n", pmap );
#endif

        if (pmap == NULL)
            return;

	mgaPriv = MGA_PRIVATE_DATA(pmap->pScreen);

	/* only load the cmap if depth == 8 */
	if ((pmap == NULL) || (mgaPriv->depth != 8))
		return;

	/* all the dacs have the same offsets for index and data */
	dac = (mgaDacsPtr)&mgaPriv->regs->ramdacs;
	dac->bt485.wadr_pal = 0;

	/* pluck rgb values out of colormap */
	if ((pmap->pVisual->class | DynamicClass) == DirectColor) {

		register VisualPtr pVis = pmap->pVisual;

		nents = 1 << pVis->nplanes;
		for (i = 0; i < nents; i++) {
		    int r, g, b;

		    r = (i & pVis->redMask)   >> pVis->offsetRed;
		    g = (i & pVis->greenMask) >> pVis->offsetGreen;
		    b = (i & pVis->blueMask)  >> pVis->offsetBlue;
		    dac->bt485.col_pal = pmap->red[r].co.local.red >> 8;
		    dac->bt485.col_pal = pmap->green[g].co.local.green >> 8;
		    dac->bt485.col_pal = pmap->blue[b].co.local.blue >> 8;
		}

	} else {

		register Entry *pent;

		nents = pmap->pVisual->ColormapEntries;
		for (pent = pmap->red, i = nents;  --i >= 0;  pent++) {
			if (pent->fShared) {
			  dac->bt485.col_pal = pent->co.shco.red->color >> 8;
			  dac->bt485.col_pal = pent->co.shco.green->color >> 8;
			  dac->bt485.col_pal = pent->co.shco.blue->color >> 8;
			}
			else {
			  dac->bt485.col_pal = pent->co.local.red >> 8;
			  dac->bt485.col_pal = pent->co.local.green >> 8;
			  dac->bt485.col_pal = pent->co.local.blue >> 8;
			}
		}
	}
}
