/*
 *	@(#) effStip.c 11.1 97/10/22
 *
 * Modification History
 *
 * S006, 23-Oct-92, staceyc
 * 	moved the various stretch blits into procs.h to let compilers inline
 *	this if they can
 * S005, 24-Sep-91, staceyc
 * 	get bit stips going fast with S004
 * S004, 20-Sep-91, staceyc
 * 	use nfb replicate code - no need to have same code in here - and
 *	fixed bug
 * S003, 28-Aug-91, staceyc
 * 	general code cleanup - reworking of command queue use
 * S002, 13-Aug-91, mikep
 *	- changed all calls to nfbStippledFillRects to genStippledFillRects
 * S001, 13-Aug-91, staceyc
 * 	- fixed problem with calculation of off_screen stipple size
 *	- use single plane for off_screen blit
 * S000, 09-Aug-91, staceyc
 * 	created
 */

#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern int effRasterOps[];

void
effStippledFillRects(
GCPtr pGC,
DrawablePtr pDraw,
BoxPtr pbox,
unsigned int nbox)
{
	int stipw, stiph, i, lx, ly, biggest_w, biggest_h;
	int chunks, extra_height, y;
	int xoff, yoff, os_height, max_height;
	DDXPointPtr patOrg;
	DDXPointRec os_point;
	PixmapPtr pStip;
	BoxRec box, os_box;
	unsigned char *pimage;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	effOffScreenBlitArea_t *off_screen;
	unsigned long int fg = pGCPriv->rRop.fg;
	unsigned long int planemask = pGCPriv->rRop.planemask;
	unsigned char alu = pGCPriv->rRop.alu;

	off_screen = &(EFF_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */
	if (stipw * 2 > off_screen->w || stiph * 2 > off_screen->h)
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */
	biggest_w = 0;
	biggest_h = 0;
	for (i = 0; i < nbox; ++i)
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */
	if (biggest_w > off_screen->w)
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */
	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;
	effDrawOpaqueMonoImage(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    GXcopy, EFF_GENERAL_OS_WRITE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */
	max_height = off_screen->h - stiph;
	if (biggest_h > max_height)
		os_height = off_screen->h;
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}
	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;
	nfbReplicateArea(&os_box, stipw, stiph, EFF_GENERAL_OS_WRITE, pDraw);

	effSetupCardForBlit(fg, alu, EFF_GENERAL_OS_READ, planemask);
	for (i = 0; i < nbox; ++i, ++pbox)
	{
		xoff = (pbox->x1 - patOrg->x) % stipw;
		if (xoff < 0)
			xoff += stipw;
		yoff = (pbox->y1 - patOrg->y) % stiph;
		if (yoff < 0)
			yoff += stiph;
		os_point.x = off_screen->x + xoff;
		os_point.y = off_screen->y + yoff;
		ly = pbox->y2 - pbox->y1;
		chunks = ly / max_height;
		extra_height = ly % max_height;
		for (y = 0; y < chunks; ++y)
		{
			box.x1 = pbox->x1;
			box.x2 = pbox->x2;
			box.y1 = pbox->y1 + y * max_height;
			box.y2 = box.y1 + max_height;
			effStretchBlit(pDraw, &os_point, &box);
		}
		if (extra_height)
		{
			box.x1 = pbox->x1;
			box.x2 = pbox->x2;
			box.y1 = pbox->y1 + chunks * max_height;
			box.y2 = box.y1 + extra_height;
			effStretchBlit(pDraw, &os_point, &box);
		}
	}
	effResetCardFromBlit();
}

void
effOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox )
{
	int stipw, stiph, i, lx, ly, biggest_w, biggest_h;
	int chunks, extra_height, y;
	int xoff, yoff, os_height, max_height;
	DDXPointPtr patOrg;
	DDXPointRec os_point;
	PixmapPtr pStip;
	BoxRec box, os_box;
	unsigned char *pimage;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	effOffScreenBlitArea_t *off_screen;
	unsigned long int fg = pGCPriv->rRop.fg;
	unsigned long int bg = pGCPriv->rRop.bg;
	unsigned long int planemask = pGCPriv->rRop.planemask;
	unsigned char alu = pGCPriv->rRop.alu;

	off_screen = &(EFF_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */
	if (stipw * 2 > off_screen->w || stiph * 2 > off_screen->h)
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */
	biggest_w = 0;
	biggest_h = 0;
	for (i = 0; i < nbox; ++i)
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */
	if (biggest_w > off_screen->w)
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */
	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;
	effDrawOpaqueMonoImage(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    GXcopy, EFF_GENERAL_OS_WRITE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */
	max_height = off_screen->h - stiph;
	if (biggest_h > max_height)
		os_height = off_screen->h;
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}
	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;
	nfbReplicateArea(&os_box, stipw, stiph, EFF_GENERAL_OS_WRITE, pDraw);

	effSetupCardForOpaqueBlit(fg, bg, alu, EFF_GENERAL_OS_READ, planemask);
	for (i = 0; i < nbox; ++i, ++pbox)
	{
		xoff = (pbox->x1 - patOrg->x) % stipw;
		if (xoff < 0)
			xoff += stipw;
		yoff = (pbox->y1 - patOrg->y) % stiph;
		if (yoff < 0)
			yoff += stiph;
		os_point.x = off_screen->x + xoff;
		os_point.y = off_screen->y + yoff;
		ly = pbox->y2 - pbox->y1;
		chunks = ly / max_height;
		extra_height = ly % max_height;
		for (y = 0; y < chunks; ++y)
		{
			box.x1 = pbox->x1;
			box.x2 = pbox->x2;
			box.y1 = pbox->y1 + y * max_height;
			box.y2 = box.y1 + max_height;
			effStretchBlit(pDraw, &os_point, &box);
		}
		if (extra_height)
		{
			box.x1 = pbox->x1;
			box.x2 = pbox->x2;
			box.y1 = pbox->y1 + chunks * max_height;
			box.y2 = box.y1 + extra_height;
			effStretchBlit(pDraw, &os_point, &box);
		}
	}
	effResetCardFromBlit();
}
