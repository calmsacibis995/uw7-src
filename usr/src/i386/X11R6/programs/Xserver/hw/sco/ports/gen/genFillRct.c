/*
 * GC rectangle ops for nfb
 */
#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "genDefs.h"
#include "genProcs.h"

void
genSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox )
{
	nfbWinOpsPtr winOps;

	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	winOps = (NFB_WINDOW_PRIV(pDraw))->ops;

	(* winOps->DrawSolidRects)( pbox, nbox, pGCPriv->rRop.fg, 
			pGCPriv->rRop.alu, pGCPriv->rRop.planemask, pDraw );
	return;
}

void
genTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox )
{
	nfbWinOpsPtr winOps;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	winOps = (NFB_WINDOW_PRIV(pDraw))->ops;

	(* winOps->TileRects)(pbox, nbox, pGC->tile.pixmap->devPrivate.ptr,
		pGC->tile.pixmap->devKind, pGC->tile.pixmap->drawable.width,
		pGC->tile.pixmap->drawable.height,
		&(pGCPriv->screenPatOrg), pGCPriv->rRop.alu, 
		pGCPriv->rRop.planemask, pDraw);
	return;
}
	
void
genStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox )
{
	nfbWinOpsPtr winOps;
	int xoff, yoff;
	int stipw, stiph;
	DDXPointPtr patOrg;
	PixmapPtr pStip;
	BoxRec box;
	unsigned char *pimage;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	unsigned long int fg = pGCPriv->rRop.fg;
	unsigned long int planemask = pGCPriv->rRop.planemask;
	unsigned char alu = pGCPriv->rRop.alu;

	winOps = (NFB_WINDOW_PRIV(pDraw))->ops;

	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	while(nbox--)
	{

		box.x1 = pbox->x1;
		box.y1 = pbox->y1;

		xoff = ( box.x1 - patOrg->x ) % stipw;
		if ( xoff < 0 )
			xoff += stipw;

		yoff = ( box.y1 - patOrg->y ) % stiph;
		if ( yoff < 0 )
			yoff += stiph;

		box.x2 = min( box.x1 + stipw - xoff, pbox->x2 );
		box.y2 = min( box.y1 + stiph - yoff, pbox->y2 );
		pimage = pStip->devPrivate.ptr + yoff * pStip->devKind;

		(* winOps->DrawMonoImage)( &box, pimage, xoff,
			pStip->devKind, fg, alu, planemask, pDraw );

		while ( box.x2 < pbox->x2 )
		{
			box.x1 = box.x2;
			box.x2 += stipw;
			if ( box.x2 > pbox->x2 )
				box.x2 = pbox->x2;
			(* winOps->DrawMonoImage)( &box, pimage, 0,
				pStip->devKind, fg, alu, planemask, pDraw );
		}

		pimage = pStip->devPrivate.ptr;

		while ( box.y2 < pbox->y2 )
		{
			box.y1 = box.y2;
			box.y2 += stiph;
			if ( box.y2 > pbox->y2 )
				box.y2 = pbox->y2;

			box.x1 = pbox->x1;
			box.x2 = min( box.x1 + stipw - xoff, pbox->x2 );

			(* winOps->DrawMonoImage)( &box, pimage, xoff,
				pStip->devKind, fg, alu, planemask, pDraw );

			while ( box.x2 < pbox->x2 )
			{
				box.x1 = box.x2;
				box.x2 += stipw;
				if ( box.x2 > pbox->x2 )
					box.x2 = pbox->x2;
				(* winOps->DrawMonoImage)( &box, pimage, 0,
					pStip->devKind, fg, alu, planemask,
					pDraw );
			}
		}

		pbox++;
	}
	return;
}

void
genOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox )
{
	nfbWinOpsPtr winOps;
	int xoff, yoff;
	int stipw, stiph;
	DDXPointPtr patOrg;
	PixmapPtr pStip;
	BoxRec box;
	unsigned char *pimage;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	unsigned long int fg = pGCPriv->rRop.fg;
	unsigned long int bg = pGCPriv->rRop.bg;
	unsigned long int planemask = pGCPriv->rRop.planemask;
	unsigned char alu = pGCPriv->rRop.alu;

	winOps = (NFB_WINDOW_PRIV(pDraw))->ops;

	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	while(nbox--)
	{

		box.x1 = pbox->x1;
		box.y1 = pbox->y1;

		xoff = ( box.x1 - patOrg->x ) % stipw;
		if ( xoff < 0 )
			xoff += stipw;

		yoff = ( box.y1 - patOrg->y ) % stiph;
		if ( yoff < 0 )
			yoff += stiph;

		box.x2 = min( box.x1 + stipw - xoff, pbox->x2 );
		box.y2 = min( box.y1 + stiph - yoff, pbox->y2 );
		pimage = pStip->devPrivate.ptr + yoff * pStip->devKind;

		(* winOps->DrawOpaqueMonoImage)( &box, pimage, xoff,
			pStip->devKind, fg, bg, alu, planemask, pDraw );

		while ( box.x2 < pbox->x2 )
		{
			box.x1 = box.x2;
			box.x2 += stipw;
			if ( box.x2 > pbox->x2 )
				box.x2 = pbox->x2;
			(* winOps->DrawOpaqueMonoImage)( &box, pimage, 0,
				pStip->devKind, fg, bg, alu, planemask, pDraw );
		}

		pimage = pStip->devPrivate.ptr;

		while ( box.y2 < pbox->y2 )
		{
			box.y1 = box.y2;
			box.y2 += stiph;
			if ( box.y2 > pbox->y2 )
				box.y2 = pbox->y2;

			box.x1 = pbox->x1;
			box.x2 = min( box.x1 + stipw - xoff, pbox->x2 );

			(* winOps->DrawOpaqueMonoImage)( &box, pimage, xoff,
				pStip->devKind, fg, bg, alu, planemask, pDraw );

			while ( box.x2 < pbox->x2 )
			{
				box.x1 = box.x2;
				box.x2 += stipw;
				if ( box.x2 > pbox->x2 )
					box.x2 = pbox->x2;
				(* winOps->DrawOpaqueMonoImage)( &box, pimage, 
					0, pStip->devKind, fg, bg, alu, 
					planemask, pDraw );
			}
		}

		pbox++;
	}
	return;
}
