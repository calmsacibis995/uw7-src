/*
 *	@(#) nteStip.c 11.1 97/10/22
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
 * S003, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S002, 13-Jul-93, staceyc
 * 	generalize for both memory mapped and regular i/o ports -- also
 *	unconditionally sample from 0, n / 2, and n - 1, and fix problem
 *	with determining largest off-screen stipple
 * S001, 24-Jun-93, staceyc
 * 	it wouldn't hurt to check if the off-screen pattern can actually fit
 *	in off-screen memory :-}
 * S000, 17-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

#define NTE_WORK_AREA_STIP(src, dest_x, dest_y, width, height) \
{ \
	NTE_CLEAR_QUEUE(7); \
	NTE_CLEAR_QUEUE24(7); \
	NTE_CURX((src)->x); \
	NTE_CURY((src)->y); \
	NTE_DESTX_DIASTP(dest_x); \
	NTE_DESTY_AXSTP(dest_y); \
	NTE_MAJ_AXIS_PCNT((width) - 1); \
	NTE_MIN_AXIS_PCNT((height) - 1); \
	NTE_CMD(S3C_BLIT_XP_YP_Y); \
}

static void
nteStippleAreaSlowXExpand(
	BoxRec *pbox,
	DDXPointRec *src,
	int max_width,
	ntePrivateData_t *ntePriv)
{
        int width, height, x, y;
        int chunks, extra_width;

        width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;
	x = pbox->x1;
	y = pbox->y1;
        chunks = width / max_width;
        extra_width = width % max_width;

	NTE_BEGIN(ntePriv->regs);
        while (chunks--)
        {
		NTE_WORK_AREA_STIP(src, x, y, max_width, height);
                x += max_width;
        }
        if (extra_width)
		NTE_WORK_AREA_STIP(src, x, y, extra_width, height);
	NTE_END();
}

static void
nteStippleAreaSlow(
	BoxRec *pbox,
	DDXPointRec *src,
	int max_height,
	int max_width,
	ntePrivateData_t *ntePriv)
{
        int height;
        int chunks, extra_height;
        BoxRec screen_box;

        height = pbox->y2 - pbox->y1;
        chunks = height / max_height;
        extra_height = height % max_height;
        screen_box.x1 = pbox->x1;
        screen_box.x2 = pbox->x2;
        screen_box.y1 = pbox->y1;
        while (chunks--)
        {
                screen_box.y2 = screen_box.y1 + max_height;
                nteStippleAreaSlowXExpand(&screen_box, src, max_width, ntePriv);
                screen_box.y1 += max_height;
        }
        if (extra_height)
        {
                screen_box.y2 = screen_box.y1 + extra_height;
                nteStippleAreaSlowXExpand(&screen_box, src, max_width, ntePriv);
        }
}

void
NTE(OpStippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox)
{
	int w, h, min_width, min_height, max_width, max_height;
	int stride, width, height;
	DDXPointPtr patOrg;
	PixmapPtr pStip;
	unsigned char *image;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	unsigned long fg = pGCPriv->rRop.fg;
	unsigned long bg = pGCPriv->rRop.bg;
	unsigned char alu = pGCPriv->rRop.alu;
	unsigned long planemask = pGCPriv->rRop.planemask;
	BoxRec *p, box;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	DDXPointRec src;

	pStip = pGC->stipple;
	w = pStip->drawable.width;
	h = pStip->drawable.height;

	min_width = w * 2;
	min_height = h * 2;

	if (min_width > ntePriv->wawidth || min_height > ntePriv->waheight)
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	image = pStip->devPrivate.ptr;
	stride = pStip->devKind;
	patOrg = &(pGCPriv->screenPatOrg);

	max_width = w;
	max_height = h;
	p = pbox;
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;
	p = &pbox[nbox / 2];
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;
	p = &pbox[nbox - 1];
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;

	if (max_width > ntePriv->wawidth)
		max_width = ntePriv->wawidth;
	if (max_height > ntePriv->waheight)
		max_height = ntePriv->waheight;

	max_width = (max_width / w) * w;
	max_height = (max_height / h) * h;

	if (max_width < min_width)
		max_width = min_width;
	if (max_height < min_height)
		max_height = min_height;

	box.x1 = ntePriv->wax;
	box.y1 = ntePriv->way;
	box.x2 = box.x1 + w;
	box.y2 = box.y1 + h;
	NTE(DrawOpaqueMonoImage)(&box, image, 0, stride, ~0, 0, GXcopy, 1,
	    pDraw);
	box.x2 = box.x1 + max_width;
	box.y2 = box.y1 + max_height;
	nfbReplicateArea(&box, w, h, 1, pDraw);

	max_width -= w;
	max_height -= h;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(7);
	NTE_CLEAR_QUEUE24(4);
	NTE_FRGD_COLOR(fg);
	NTE_BKGD_COLOR(bg);
	NTE_CLEAR_QUEUE24(7);
	NTE_PIX_CNTL(NTE_VIDEO_MEMORY_DATA_MIX);
	NTE_WRT_MASK(planemask);
	NTE_RD_MASK(1);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, NTE(RasterOps)[alu]);

	while (nbox--)
	{
		width = pbox->x2 - pbox->x1;
		height = pbox->y2 - pbox->y1;

		src.x = (pbox->x1 - patOrg->x) % (int)w;
		if (src.x < 0)
			src.x += w;
		src.x += ntePriv->wax;
		src.y = (pbox->y1 - patOrg->y) % (int)h;
		if (src.y < 0)
			src.y += h;
		src.y += ntePriv->way;

		if (width < max_width && height < max_height)
		{
			NTE_WORK_AREA_STIP(&src, pbox->x1, pbox->y1, width,
			    height);
		}
		else
			nteStippleAreaSlow(pbox, &src, max_height, max_width,
			    ntePriv);
		++pbox;
	}

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(3);
	NTE_RD_MASK(NTE_ALLPLANES);
	NTE_PIX_CNTL(0);
	NTE_END();
}

void
NTE(StippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox)
{
	int w, h, min_width, min_height, max_width, max_height;
	int stride, width, height;
	DDXPointPtr patOrg;
	PixmapPtr pStip;
	unsigned char *image;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);
	unsigned long fg = pGCPriv->rRop.fg;
	unsigned char alu = pGCPriv->rRop.alu;
	unsigned long planemask = pGCPriv->rRop.planemask;
	BoxRec *p, box;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	DDXPointRec src;

	pStip = pGC->stipple;
	w = pStip->drawable.width;
	h = pStip->drawable.height;

	min_width = w * 2;
	min_height = h * 2;

	if (min_width > ntePriv->wawidth || min_height > ntePriv->waheight)
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	image = pStip->devPrivate.ptr;
	stride = pStip->devKind;
	patOrg = &(pGCPriv->screenPatOrg);

	max_width = w;
	max_height = h;
	p = pbox;
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;
	p = &pbox[nbox / 2];
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;
	p = &pbox[nbox - 1];
	width = p->x2 - p->x1;
	height = p->y2 - p->y1;
	if (width > max_width)
		max_width = width;
	if (height > max_height)
		max_height = height;

	if (max_width > ntePriv->wawidth)
		max_width = ntePriv->wawidth;
	if (max_height > ntePriv->waheight)
		max_height = ntePriv->waheight;

	max_width = (max_width / w) * w;
	max_height = (max_height / h) * h;

	if (max_width < min_width)
		max_width = min_width;
	if (max_height < min_height)
		max_height = min_height;

	box.x1 = ntePriv->wax;
	box.y1 = ntePriv->way;
	box.x2 = box.x1 + w;
	box.y2 = box.y1 + h;
	NTE(DrawOpaqueMonoImage)(&box, image, 0, stride, ~0, 0, GXcopy, 1,
	    pDraw);
	box.x2 = box.x1 + max_width;
	box.y2 = box.y1 + max_height;
	nfbReplicateArea(&box, w, h, 1, pDraw);

	max_width -= w;
	max_height -= h;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(6);
	NTE_CLEAR_QUEUE24(2);
	NTE_FRGD_COLOR(fg);
	NTE_CLEAR_QUEUE24(7);
	NTE_PIX_CNTL(NTE_VIDEO_MEMORY_DATA_MIX);
	NTE_WRT_MASK(planemask);
	NTE_RD_MASK(1);
	NTE_FRGD_MIX(NTE_FRGD_SOURCE, NTE(RasterOps)[alu]);
	NTE_BKGD_MIX(NTE_BKGD_SOURCE, NTE(RasterOps)[GXnoop]);

	while (nbox--)
	{
		width = pbox->x2 - pbox->x1;
		height = pbox->y2 - pbox->y1;

		src.x = (pbox->x1 - patOrg->x) % (int)w;
		if (src.x < 0)
			src.x += w;
		src.x += ntePriv->wax;
		src.y = (pbox->y1 - patOrg->y) % (int)h;
		if (src.y < 0)
			src.y += h;
		src.y += ntePriv->way;

		if (width < max_width && height < max_height)
		{
			NTE_WORK_AREA_STIP(&src, pbox->x1, pbox->y1, width,
			    height);
		}
		else
			nteStippleAreaSlow(pbox, &src, max_height, max_width,
			    ntePriv);
		++pbox;
	}

	NTE_CLEAR_QUEUE(2);
	NTE_CLEAR_QUEUE24(3);
	NTE_RD_MASK(NTE_ALLPLANES);
	NTE_PIX_CNTL(0);
	NTE_END();
}
