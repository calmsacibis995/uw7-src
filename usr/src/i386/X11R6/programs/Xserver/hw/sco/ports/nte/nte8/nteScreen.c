/*
 *	@(#) nteScreen.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S013, 06-Aug-93, staceyc
 * 	don't monkey with colormap values with pseudo color colormap restore
 * S012, 02-Aug-93, staceyc
 * 	turns out STB Pegasus has random values in clip registers after BIOS
 *	init so get them set correctly in set g state code, avoids chicken
 *	and egg problem of S010
 * S011, 23-Jul-93, staceyc
 * 	don't muck with clip registers at all
 * S010, 22-Jul-93, staceyc
 * 	can no longer use clip values during hw init as they are now _based_
 *	on hw init (i.e. init hw -> query card for off-screen area -> set
 *	clip values in screen private)
 * S009, 21-Jul-93, staceyc
 * 	generic screen blanking for all depths (will not work on 911 chips)
 * S008, 19-Jul-93, staceyc
 * 	first go at a screen blanker, looks like it's card dependent, also
 *	don't modify advfunc for 801/805 cards, reads and writes of this
 *	register are broken for these chips
 * S007, 14-Jul-93, staceyc
 * 	24 bit fixes, including 24 bit clear queue checks
 * S006, 13-Jul-93, staceyc
 * 	don't switch on memory mapped ports for 86C80x driver
 * S005, 09-Jul-93, staceyc
 * 	restore colormap for pseudocolor modes after screen switch
 * S004, 09-Jul-93, staceyc
 * 	attempt to deal with chip bugs with 24 bit modes - stop console from
 *	scribbling on the screen - clear frame buffer before and after X
 *	session
 * S003, 16-Jun-93, staceyc
 * 	init clip registers in set graphics
 * S002, 11-Jun-93, staceyc
 * 	check command queue before writing to it
 * S001, 08-Jun-93, staceyc
 * 	bios calls and initial register setup
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include <stdio.h>
#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

/*
 * NTE(BlankScreen) - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
NTE(BlankScreen)(
	int on,
	ScreenPtr pScreen)
{
	unsigned char clk_mode;

	NTE_OUTB(NTE_SEQX, NTE_CLK_MODE_INDEX);
	clk_mode = NTE_INB(NTE_SEQ_DATA);

	if (on)
		clk_mode |= NTE_SCRN_OFF;
	else
		clk_mode &= ~NTE_SCRN_OFF;

	NTE_OUTB(NTE_SEQ_DATA, clk_mode);

	return TRUE;
}

#if NTE_BITS_PER_PIXEL == 8
static void
nteRestoreColormap(
	ScreenPtr pScreen)
{
	nfbScrnPriv *devPriv = NFB_SCREEN_PRIV(pScreen);
	ColormapPtr pmap = devPriv->installedCmap;
	int i, nents;
	int ri, gi, bi;
	unsigned short r, g, b;
	VisualPtr pVis = pmap->pVisual;
	Entry *pent;

	if (pmap)
	{
		if ((pmap->pVisual->class | DynamicClass) == DirectColor)
		{

			nents = 1 << pVis->nplanes;
			for (i = 0; i < nents; i++)
			{
				ri = (i & pVis->redMask)   >> pVis->offsetRed;
				gi = (i & pVis->greenMask) >> pVis->offsetGreen;
				bi = (i & pVis->blueMask)  >> pVis->offsetBlue;
				r = pmap->red[ri].co.local.red;
				g = pmap->blue[bi].co.local.blue;
				b = pmap->green[gi].co.local.green;
				r |= r << 8;
				g |= g << 8;
				b |= b << 8;
				NTE(SetColor)(0, i, r, g, b, pScreen);
			}
		}
		else
		{
			nents = pmap->pVisual->ColormapEntries;
			for (pent = pmap->red, i = 0; i < nents; i++, pent++)
			{
				if (pent->fShared)
				{
					r = pent->co.shco.red->color;
					g = pent->co.shco.green->color;
					b = pent->co.shco.blue->color;
				}
				else
				{
					r = pent->co.local.red;
					g = pent->co.local.green;
					b = pent->co.local.blue;
				}
				NTE(SetColor)(0, i, r, g, b, pScreen);
			}
		}
	}
}
#endif

/*
 * NTE(SetGraphics)(pScreen) - set screen into graphics mode
 */
void
NTE(SetGraphics)(
	ScreenPtr pScreen)
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned short rd_reg_dt;

/*
	ioctl(fileno(stdout), SW_VGA12, 0);
*/

	grafExec(grafinfo, "SetGraphics", NULL);

	NTE_CLEAR_QUEUE(8);
	NTE_CLEAR_QUEUE24(8);

#if ! NTE_USE_IO_PORTS
	/*
	 * Do not do the reverse of this for 801/805 chips, reads and
	 * writes of the advfunc register are broken!
	 */
	NTE_OUTW(NTE_ADVFUNC_CNTL, NTE_INW(NTE_ADVFUNC_CNTL) | NTE_MIO);
#endif

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(4);
	NTE_CLEAR_QUEUE24(4);
	NTE_SCISSORS_T(0);
	NTE_SCISSORS_L(0);
	NTE_SCISSORS_R(ntePriv->width - 1);
	NTE_SCISSORS_B(ntePriv->height - 1);

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(5);
	NTE_RD_MASK(NTE_ALLPLANES);
	NTE_PIX_CNTL(0);
	NTE_CLEAR_QUEUE(8);
	NTE_WRT_MASK(~0);
	NTE_CLEAR_QUEUE24(8);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[GXcopy]);
	NTE_FRGD_COLOR(0);
	NTE_CURX(0);
	NTE_MAJ_AXIS_PCNT(ntePriv->width - 1);
	NTE_CURY(0);
	NTE_MIN_AXIS_PCNT(ntePriv->height - 1);
	NTE_CMD(NTE_FILL_X_Y_DATA);
	NTE_END();

#if NTE_BITS_PER_PIXEL == 8
	nteRestoreColormap(pScreen);
#endif
}

/*
 * NTE(SetText)(pScreen) - set screen into text mode
 */
void
NTE(SetText)(
	ScreenPtr pScreen)
{
	grafData *grafinfo = DDX_GRAFINFO(pScreen);
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(8);
	NTE_CLEAR_QUEUE24(2);
	NTE_WRT_MASK(~0);
	NTE_CLEAR_QUEUE24(8);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[GXcopy]);
	NTE_FRGD_COLOR(0);
	NTE_CURX(0);
	NTE_MAJ_AXIS_PCNT(ntePriv->width - 1);
	NTE_CURY(0);
	NTE_MIN_AXIS_PCNT(ntePriv->height - 1);
	NTE_CMD(NTE_FILL_X_Y_DATA);
	NTE_END();
	grafExec(grafinfo, "SetText", NULL);
}

/*
 * NTE(SaveGState)(pScreen) - save graphics info before screen switch
 */
void
NTE(SaveGState)(
	ScreenPtr pScreen)
{
}

/*
 * NTE(RestoreGState)(pScreen) - restore graphics info from NTE(SaveGState)()
 */
void
NTE(RestoreGState)(
	ScreenPtr pScreen)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(4);
	NTE_CLEAR_QUEUE24(4);
	NTE_SCISSORS_T(0);
	NTE_SCISSORS_L(0);
	NTE_SCISSORS_R(ntePriv->clip_x);
	NTE_SCISSORS_B(ntePriv->clip_y);
	NTE_END();
}
