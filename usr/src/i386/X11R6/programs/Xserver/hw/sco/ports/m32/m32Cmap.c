/*
 * @(#) m32Cmap.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */
/*
 * m32Cmap.c
 *
 * Mach-32 colormap routines
 */
#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "colormapst.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"                                       /* S001 vv*/
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"                                      /* S001 ^^*/
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

/*
 * m32SetColor() - set an entry in a PseudoColor colormap
 *	cmap - machine dependent colormap number
 *	index - offset into colormap
 *	r,g,b - the rgb tripple to enter in this colormap entry
 *	pScreen - pointer to X's screen struct for this screen.
 *
 * NOTE: r,g,b are 16 bits.  Use the MOST SIGNIFICANT bits.
 */
void
m32SetColor(cmap, index, r, g, b, pScreen)
	unsigned int cmap;
	unsigned int index;
	unsigned short r;
	unsigned short g;
	unsigned short b;
	ScreenPtr pScreen;
{
	outb(M32_DAC_W_INDEX, index);
	outb(M32_DAC_DATA, r >> 10);
	outb(M32_DAC_DATA, g >> 10);
	outb(M32_DAC_DATA, b >> 10);
}

/*
 * m32LoadColormap() - load a PseudoColor colormap
 *
 * A faster way to load an entire colormap
 * than going through SetColor for each entry.
 */
void
m32LoadColormap(pmap)
	ColormapPtr pmap;
{
	int i, nents;

	if (pmap == NULL)
		return;

	outb(M32_DAC_W_INDEX, 0);

	/* pluck rgb values out of colormap */
	if ((pmap->pVisual->class | DynamicClass) == DirectColor) {

	    VisualPtr pVis = pmap->pVisual;

	    nents = 1 << pVis->nplanes;
	    for (i = 0; i < nents; i++) {
		int ri, gi, bi;

		ri = (i & pVis->redMask)   >> pVis->offsetRed;
		gi = (i & pVis->greenMask) >> pVis->offsetGreen;
		bi = (i & pVis->blueMask)  >> pVis->offsetBlue;
		outb(M32_DAC_DATA, pmap->red[ri].co.local.red     >> 10);
		outb(M32_DAC_DATA, pmap->green[gi].co.local.green >> 10);
		outb(M32_DAC_DATA, pmap->blue[bi].co.local.blue   >> 10);
	    }

	} else {

	    Entry *pent;

	    nents = pmap->pVisual->ColormapEntries;
	    for (pent = pmap->red, i = nents;  --i >= 0;  pent++) {
		if (pent->fShared) {
		    outb(M32_DAC_DATA, pent->co.shco.red->color   >> 10);
		    outb(M32_DAC_DATA, pent->co.shco.green->color >> 10);
		    outb(M32_DAC_DATA, pent->co.shco.blue->color  >> 10);
		} else {
		    outb(M32_DAC_DATA, pent->co.local.red   >> 10);
		    outb(M32_DAC_DATA, pent->co.local.green >> 10);
		    outb(M32_DAC_DATA, pent->co.local.blue  >> 10);
		}
	    }
	}

	outb(M32_DAC_MASK, 0xFF);
}

void
m32BlankColormap(pScreen)
	ScreenPtr pScreen;
{
	/* set color 0 to black */
	outb(M32_DAC_W_INDEX, 0);
	outb(M32_DAC_DATA, 0);
	outb(M32_DAC_DATA, 0);
	outb(M32_DAC_DATA, 0);

	/* force everything to color 0 */
	outb(M32_DAC_MASK, 0);
}

void
m32RestoreColormap(pScreen)
	ScreenPtr pScreen;
{
	nfbScrnPrivPtr pNfb = NFB_SCREEN_PRIV(pScreen);

	(*pNfb->LoadColormap)(pNfb->installedCmap);
}
