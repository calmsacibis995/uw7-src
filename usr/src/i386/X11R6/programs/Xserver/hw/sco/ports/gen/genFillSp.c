#include "X.h"
#include "Xmd.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "genDefs.h"
#include "genProcs.h"

void
genSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned npts )
{
    void (*FillRects)() = NFB_GC_PRIV(pGC)->ops->FillRects;
    BoxRec *pbox, *pb;
    int i;

    pb = pbox = (BoxRec *)ALLOCATE_LOCAL(npts * sizeof(BoxRec));
    for (i = 0; i < npts; ++i)
    {
	    pb->x1 = ppt->x;
	    pb->x2 = pb->x1 + *pwidth;
	    pb->y1 = ppt->y;
	    pb->y2 = pb->y1 + 1;
	    pwidth++;
	    ppt++;
	    ++pb;
    }
    (*FillRects)(pGC, pDraw, pbox, npts);
    DEALLOCATE_LOCAL((char *)pbox);

    return;
}

void
genTiledFS(
GCPtr pGC,
DrawablePtr pDraw,
DDXPointPtr ppt,
unsigned int *pwidth,
unsigned int npts)
{
	PixmapPtr pm = pGC->tile.pixmap;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	unsigned long planemask = pGCPriv->rRop.planemask;
	unsigned char alu = pGCPriv->rRop.alu;
	BoxRec *pbox, *pb;
	int i;

	pb = pbox = (BoxRec *)ALLOCATE_LOCAL(npts * sizeof(BoxRec));
	for (i = 0; i < npts; ++i)
	{
		pb->x1 = ppt->x;
		pb->x2 = pb->x1 + *pwidth;
		pb->y1 = ppt->y;
		pb->y2 = pb->y1 + 1;
		pwidth++;
		ppt++;
		++pb;
	}
	(*NFB_WINDOW_PRIV(pDraw)->ops->TileRects)(pbox, npts,
	    pm->devPrivate.ptr, pm->devKind, pm->drawable.width,
	    pm->drawable.height, &((NFB_GC_PRIV(pGC))->screenPatOrg), alu,
	    planemask, pDraw);
	DEALLOCATE_LOCAL((char *)pbox);
}

void
genStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int npts)
{
	BoxRec *pbox, *pb;
	int i;
	void (*StippledFillRects)() = NFB_GC_PRIV(pGC)->ops->FillRects;

	pb = pbox = (BoxRec *)ALLOCATE_LOCAL(npts * sizeof(BoxRec));
	for (i = 0; i < npts; ++i)
	{
		pb->x1 = ppt->x;
		pb->x2 = pb->x1 + *pwidth;
		pb->y1 = ppt->y;
		pb->y2 = pb->y1 + 1;
		pwidth++;
		ppt++;
		++pb;
	}
	(*StippledFillRects)(pGC, pDraw, pbox, npts);
	DEALLOCATE_LOCAL((char *)pbox);
}

void
genOpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned npts)
{
	BoxRec *pbox, *pb;
	int i;
	void (*OpStippledFillRects)() = NFB_GC_PRIV(pGC)->ops->FillRects;

	pb = pbox = (BoxRec *)ALLOCATE_LOCAL(npts * sizeof(BoxRec));
	for (i = 0; i < npts; ++i)
	{
		pb->x1 = ppt->x;
		pb->x2 = pb->x1 + *pwidth;
		pb->y1 = ppt->y;
		pb->y2 = pb->y1 + 1;
		pwidth++;
		ppt++;
		++pb;
	}
	(*OpStippledFillRects)(pGC, pDraw, pbox, npts);
	DEALLOCATE_LOCAL((char *)pbox);
}

