/*
 *	@(#) effRectOps.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History
 *
 * S013, 12-Dec-92, mikep
 *	remove parmeter checks, they are now done in nfb
 * S012, 30-Oct-92, staceyc
 * 	cleaned up old comments
 * S011, 29-Oct-92, mikep
 *	Speed up draw points using fast lines
 * S010, 03-Sep-92, hiramc
 *	Better declaration of arguments to effTileRects to eliminate
 *	compile warnings
 * S009, 10-Mar-92, buckm
 *	- Fix unsigned modulo problem in effTileRects().
 * S008, 28-Aug-91, staceyc
 * 	fast rectangle support added - general code cleanup
 * S007, 21-Aug-91, staceyc
 * 	off-screen blit stipple support
 * S006, 19-Aug-91, staceyc
 * 	moved mono drawing routines to their own c source file
 * S005, 13-Aug-91, staceyc
 * 	keep off screen ops on one plane
 * S004, 07-Aug-91, staceyc
 * 	- fix memory size allocation and stage calling for large mono images
 *	- simple optimization in opaque mono image
 * S003, 06-Aug-91, staceyc
 * 	hardware mono image support
 * S002, 24-Jun-91, staceyc
 * 	fixed params so they match effProcs.h
 * S001, 21-Jun-91, staceyc
 * 	added solid rect drawing
 * S000, 21-Jun-91, staceyc
 * 	initial bitblt work - changed all gen to eff
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern int effRasterOps[];

static void effTileAreaSlowXExpand(BoxRec *pbox, DDXPointRec *off_screen_point,
	int avail_width, unsigned char alu, unsigned long planemask,
	DrawablePtr pDraw);
static void effTileAreaSlow(BoxRec *pbox, DDXPointRec *off_screen_point,
	int avail_height, int avail_width, unsigned char alu,
	unsigned long planemask, DrawablePtr pDraw);

void
effCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int ly, lx, x0, x1, y0, y1;
	unsigned short command = 0xC0F3;           /* this should be a macro */

	ly = pdstBox->y2 - pdstBox->y1;
	lx = pdstBox->x2 - pdstBox->x1;

	x0 = psrc->x;
	y0 = psrc->y;
	x1 = pdstBox->x1;
	y1 = pdstBox->y1;
	--lx;
	--ly;
	if (x1 > x0)
	{
		x0 += lx;                      /* start at right and go left */
		x1 += lx;
		command &= 0xFFDF;                      /* turn off X cd bit */
	}
	if (y1 > y0)
	{
		y0 += ly;                       /* start at bottom and go up */
		y1 += ly;
		command &= 0xFF7F;                      /* turn off Y cd bit */
	}

	EFF_CLEAR_QUEUE(6);
	EFF_SETMODE(EFF_M_ONES);
	EFF_PLNRENBL(planemask);
	EFF_PLNWENBL(planemask);
	EFF_SETFN1(EFF_FNCPYRCT, effRasterOps[alu]);
	EFF_SETX0(x0);
	EFF_SETY0(y0);

	EFF_CLEAR_QUEUE(5);
	EFF_SETX1(x1);
	EFF_SETY1(y1);
	EFF_SETLX(lx);
	EFF_SETLY(ly);

	EFF_COMMAND(command);
}

void
effDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	register BoxPtr pbox;
	register int nbox;
	register int nptsInit = npts;
	BoxRec singleBox;
	BoxPtr pboxInit = &singleBox;
	WindowPtr pWin = (WindowPtr)pDraw;

	EFF_CLEAR_QUEUE(6);
	EFF_SETMODE(EFF_M_ONES);
   	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETCOL1(fg);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);

	/* draw fast lines of length 1 */
	EFF_SETLX(0);

	/*
	 * inline code will really speed this up.  Maybe unroll the
	 * loop?
	 */
	while(npts--)
	    { 
	    EFF_CLEAR_QUEUE(3);
	    EFF_SETX0(ppt->x);
	    EFF_SETY0(ppt->y);
	    EFF_COMMAND(EFF_CMD_VECTOR | EFF_CMD_DO_ACCESS 
			| EFF_CMD_USE_C_DIR | EFF_CMD_ACCRUE_PELS
			| EFF_CMD_WRITE);
	    ppt++;
	    }


	return;
}

void
effDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int ly, lx;

	EFF_CLEAR_QUEUE(5);
	EFF_SETMODE(EFF_M_ONES);
   	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETCOL1(fg);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);

	do
	{
		lx = pbox->x2 - pbox->x1;
		ly = pbox->y2 - pbox->y1;
		if (lx <= 0 || ly <= 0)
			continue;

		effBlamOutBox(pbox->x1, pbox->y1, lx - 1, ly - 1);

		++pbox;
	} while (--nbox);
}

void
effTileRects(
	BoxPtr pbox,
	unsigned int nbox,			/*	S010	*/
	unsigned char *tile,
	unsigned int stride,			/*	S010	*/
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	effOffScreenBlitArea_t *tile_blit_area;
	BoxRec off_screen_box, screen_box, *boxp;
	DDXPointRec off_screen_point;
	int tiles_wide, tiles_high;
	int avail_width, avail_height;
	Bool clobber_alu;
	int lx, ly;
	int xoff, yoff;
	int i, biggest_w, biggest_h;

	tile_blit_area = &EFF_PRIVATE_DATA(pDraw->pScreen)->tile_blit_area;

	/*
	 * how many tiles can we fit in the off-screen area?
	 */
	tiles_wide = tile_blit_area->w / w;
	tiles_high = tile_blit_area->h / h;
	/*
	 * does the raster op allow us to clobber the destination
	 * or does it require some src/dest mix
	 */
	clobber_alu = alu == GXcopy || alu == GXcopyInverted ||
	    alu == GXclear || alu == GXset || alu == GXinvert;
	/*
	 * When to use gen code:
	 * - our blit with alu requires at least 2x2 tiles stashed in
	 *   off-screen memory to deal with patOrg correctly, if the
	 *   tile is too big then use gen,
	 * - gen code draws a single tiled box quickly relative to eff
	 *   code, iff the raster op allows the dest to be clobbered by
	 *   the source.  In the case where the raster op requires a mix
	 *   with the source, eff code will generally be faster.
	 */
	if (tiles_wide < 2 || tiles_high < 2 || (nbox < 2 && clobber_alu))
	{
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
		    planemask, pDraw);
		return;
	}
	avail_width = (tiles_wide - 1) * w;
	avail_height = (tiles_high - 1) * h;
	/*
	 * draw one copy of the tile to off-screen
	 */
	off_screen_box.x1 = tile_blit_area->x;
	off_screen_box.y1 = tile_blit_area->y;
	off_screen_box.x2 = tile_blit_area->x + w;
	off_screen_box.y2 = tile_blit_area->y + h;
	if (pDraw->bitsPerPixel == 1)
		/*
		 * the gen version of this routine supports bitmaps, but
		 * doesn't specify fg/bg!
		 */
		effDrawOpaqueMonoImage(&off_screen_box, tile, 0, stride, 1,
		    0, GXcopy, ~0, pDraw);
	else
		effDrawImage(&off_screen_box, tile, stride, GXcopy, ~0, pDraw);
	/*
	 * fill up the off-screen tile area based on the largest dimensions
	 * to be drawn
	 */
	biggest_h = 0;
	biggest_w = 0;
	for (boxp = pbox, i = 0; i < nbox; ++i, ++boxp)
	{
		lx = boxp->x2 - boxp->x1;
		ly = boxp->y2 - boxp->y1;
		if (lx > biggest_w)
			biggest_w = lx;
		if (ly > biggest_h)
			biggest_h = ly;
	}
	biggest_w += w;                              /* allowance for patOrg */
	biggest_h += h;
	if (biggest_w > tile_blit_area->w)
		biggest_w = tile_blit_area->w;
	if (biggest_h > tile_blit_area->h)
		biggest_h = tile_blit_area->h;
	off_screen_box.x2 = tile_blit_area->x + biggest_w;
	off_screen_box.y2 = tile_blit_area->y + biggest_h;
	nfbReplicateArea(&off_screen_box, w, h, ~0, pDraw);
	/*
	 * draw the boxes
	 */
	while (nbox--)
	{
		lx = pbox->x2 - pbox->x1;
		ly = pbox->y2 - pbox->y1;
		/*
		 * calculate patOrg offsets into off-screen tile.
		 * since (pbox - patOrg) may be negative,
		 * the modulo operations _must_ be signed, not unsigned.
		 */
		xoff = (pbox->x1 - patOrg->x) % (int) w;	/* S009 */
		if (xoff < 0)
			xoff += w;
		yoff = (pbox->y1 - patOrg->y) % (int) h;	/* S009 */
		if (yoff < 0)
			yoff += h;
		off_screen_point.x = tile_blit_area->x + xoff;
		off_screen_point.y = tile_blit_area->y + yoff;
		/*
		 * determine how to blit box to dest
		 */
		if (lx < avail_width && ly < avail_height)
			/*
			 * screen box is smaller than off-screen tiled
			 * area, so blit straight to on-screen
			 */
			effCopyRect(pbox, &off_screen_point, alu, planemask,
			    pDraw);
		else
			if (clobber_alu)
			{
				/*
				 * for these raster ops it's okay to blit the
				 * tile to on-screen, then replicate the tile
				 * on-screen to the full size of the rect box
				 */
				screen_box.x1 = pbox->x1;
				screen_box.y1 = pbox->y1;
				if (lx < avail_width)
					screen_box.x2 = screen_box.x1 + lx;
				else
					screen_box.x2 = screen_box.x1 +
					    avail_width;
				if (ly < avail_height)
					screen_box.y2 = screen_box.y1 + ly;
				else
					screen_box.y2 = screen_box.y1 +
					    avail_height;
				/*
				 * blit as much as possible from off-screen
				 */
				effCopyRect(&screen_box, &off_screen_point, alu,
				    planemask, pDraw);
				/*
				 * replicate rest
				 */
				nfbReplicateArea(pbox, avail_width,
				    avail_height, planemask, pDraw);
			}
			else
			{
				/*
				 * any raster op here requires mixing src and
				 * dest - this is the slow way - fortunately
				 * not too many apps will do this - to test
				 * this run x11perf with -xor and tile ops
				 * - this code is still faster than gen
				 */
				effTileAreaSlow(pbox, &off_screen_point,
				    avail_height, avail_width, alu, planemask,
				    pDraw);
			}
		++pbox;
	}
}

static void
effTileAreaSlow(
BoxRec *pbox,
DDXPointRec *off_screen_point,
int avail_height,
int avail_width,
unsigned char alu,
unsigned long planemask,
DrawablePtr pDraw)
{
	int ly;
	int chunks, extra_height;
	BoxRec screen_box;

	ly = pbox->y2 - pbox->y1;
	chunks = ly / avail_height;
	extra_height = ly % avail_height;
	screen_box.x1 = pbox->x1;
	screen_box.x2 = pbox->x2;
	screen_box.y1 = pbox->y1;
	while (chunks--)
	{
		screen_box.y2 = screen_box.y1 + avail_height;
		effTileAreaSlowXExpand(&screen_box, off_screen_point,
		    avail_width, alu, planemask, pDraw);
		screen_box.y1 += avail_height;
	}
	if (extra_height)
	{
		screen_box.y2 = screen_box.y1 + extra_height;
		effTileAreaSlowXExpand(&screen_box, off_screen_point,
		    avail_width, alu, planemask, pDraw);
	}
}

static void
effTileAreaSlowXExpand(
BoxRec *pbox,
DDXPointRec *off_screen_point,
int avail_width,
unsigned char alu,
unsigned long planemask,
DrawablePtr pDraw)
{
	int lx;
	int chunks, extra_width;
	BoxRec screen_box;

	lx = pbox->x2 - pbox->x1;
	chunks = lx / avail_width;
	extra_width = lx % avail_width;
	screen_box.y1 = pbox->y1;
	screen_box.y2 = pbox->y2;
	screen_box.x1 = pbox->x1;
	while (chunks--)
	{
		screen_box.x2 = screen_box.x1 + avail_width;
		effCopyRect(&screen_box, off_screen_point, alu, planemask,
		    pDraw);
		screen_box.x1 += avail_width;
	}
	if (extra_width)
	{
		screen_box.x2 = screen_box.x1 + extra_width;
		effCopyRect(&screen_box, off_screen_point, alu, planemask,
		    pDraw);
	}
}

