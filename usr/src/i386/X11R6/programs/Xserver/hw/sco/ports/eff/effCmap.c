/*
 *	@(#) effCmap.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *
 * Modification History
 *
 * S010, 22-Nov-93, staceyc
 * 	prevent clients from mucking with colormap while screen is blanked
 * S009, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S008, 14-Nov-91, staceyc
 * 	512K card support
 * S007, 17-Oct-91, buckm
 *	pull colors out of colormap correctly
 *	for simulated TrueColor and DirectColor.
 * S006, 09-Sep-91, staceyc
 * 	switch the order of index/mask setting to fix bug with Matrox dac
 * S005, 28-Aug-91, staceyc
 * 	code cleanup and reworking of command queue
 * S004, 20-Aug-91, staceyc
 * 	fixed parameter decs
 * S003, 28-Jun-91, staceyc
 * 	use dac_shift from screen private instead of constant
 * S002, 27-Jun-91, staceyc
 * 	add load colormap support
 * S001, 24-Jun-91, staceyc
 * 	added some includes for a clean compile
 * S000, 18-Jun-91, staceyc
 * 	initial work
 */

#include <sys/types.h>
#include "X.h"
#include "misc.h"
#include "servermd.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "window.h"
#include "windowstr.h"
#include "nfbScrStr.h"
#include "nfbDefs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

/*
 * effSetColor() - set an entry in a PseudoColor colormap
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
effSetColor(
	unsigned int cmap,
	unsigned int index,
	unsigned short r,
	unsigned short g,
	unsigned short b,
	ScreenPtr pScreen)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	unsigned int shift = effPriv->dac_shift;
	int top;

	if (index > pScreen->visuals[0].ColormapEntries ||
	    effPriv->screen_blanked)
		return;

	r >>= shift;
	g >>= shift;
	b >>= shift;

	if (effPriv->depth == 8)
	{
		EFF_OUTB(effPriv->eff_pal.write_addr, index);
		EFF_OUTB(effPriv->eff_pal.data, (unsigned char)r);
		EFF_OUTB(effPriv->eff_pal.data, (unsigned char)g);
		EFF_OUTB(effPriv->eff_pal.data, (unsigned char)b);
	}
	else
		for (top = 0; top < 16; ++top)
		{
			EFF_OUTB(effPriv->eff_pal.write_addr, (top << 4) +
			    index);
			EFF_OUTB(effPriv->eff_pal.data, (unsigned char)r);
			EFF_OUTB(effPriv->eff_pal.data, (unsigned char)g);
			EFF_OUTB(effPriv->eff_pal.data, (unsigned char)b);
		}
}

void
effRestoreColormap(
ScreenPtr pScreen)
{
	nfbScrnPriv *devPriv = NFB_SCREEN_PRIV(pScreen);
	ColormapPtr pmap = devPriv->installedCmap;

	(*devPriv->LoadColormap)(pmap);
}

/*
 * effLoadColormap() - load a PseudoColor colormap
 */
void
effLoadColormap(
register ColormapPtr pmap)
{
	ScreenPtr pScreen;
	int i, nents;
	unsigned char r, g, b;
	effPrivateData_t *effPriv;
	unsigned int shift;
	int depth;
	int top;

	if (pmap == NULL)
		return;
	pScreen = pmap->pScreen;
	effPriv = EFF_PRIVATE_DATA(pScreen);
	if (effPriv->screen_blanked)
		return;

	depth = effPriv->depth;
	shift = effPriv->dac_shift;
	EFF_OUTB(effPriv->eff_pal.write_addr, 0);

	if ((pmap->pVisual->class | DynamicClass) == DirectColor)
	{
		register VisualPtr pVis = pmap->pVisual;

		nents = 1 << pVis->nplanes;
		for (i = 0; i < nents; i++)
		{
			int ri, gi, bi;

			ri = (i & pVis->redMask)   >> pVis->offsetRed;
			gi = (i & pVis->greenMask) >> pVis->offsetGreen;
			bi = (i & pVis->blueMask)  >> pVis->offsetBlue;
			r = pmap->red[ri].co.local.red >> shift;
			g = pmap->blue[bi].co.local.blue >> shift;
			b = pmap->green[gi].co.local.green >> shift;
			if (depth == 8)
			{
				EFF_OUTB(effPriv->eff_pal.data, r);
				EFF_OUTB(effPriv->eff_pal.data, g);
				EFF_OUTB(effPriv->eff_pal.data, b);
				/* 8514 autoincs DAC index here */
			}
			else
				for (top = 0; top < 16; ++top)
				{
					EFF_OUTB(effPriv->eff_pal.write_addr,
					    (top << 4) + i);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)r);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)g);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)b);
				}
		}
	}
	else
	{
		register Entry *pent;

		nents = pmap->pVisual->ColormapEntries;
		for (pent = pmap->red, i = 0; i < nents; i++, pent++)
		{
			if (pent->fShared)
			{
				r = pent->co.shco.red->color >> shift;
				g = pent->co.shco.green->color >> shift;
				b = pent->co.shco.blue->color >> shift;
			}
			else
			{
				r = pent->co.local.red >> shift;
				g = pent->co.local.green >> shift;
				b = pent->co.local.blue >> shift;
			}
			if (depth == 8)
			{
				EFF_OUTB(effPriv->eff_pal.data, r);
				EFF_OUTB(effPriv->eff_pal.data, g);
				EFF_OUTB(effPriv->eff_pal.data, b);
				/* 8514 autoincs DAC index here */
			}
			else
				for (top = 0; top < 16; ++top)
				{
					EFF_OUTB(effPriv->eff_pal.write_addr,
					    (top << 4) + i);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)r);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)g);
					EFF_OUTB(effPriv->eff_pal.data,
					    (unsigned char)b);
				}
		}
	}
}
