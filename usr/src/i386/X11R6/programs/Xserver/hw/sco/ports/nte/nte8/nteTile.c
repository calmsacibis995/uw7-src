/*
 *	@(#) nteTile.c 11.1 97/10/22
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
 * S002, 13-Jul-93, staceyc
 * 	unconditionally sample from 0, n / 2, n - 1, also fix problem with
 *	determining size of largest off-screen tile
 * S001, 24-Jun-93, staceyc
 * 	it wouldn't hurt to check if the off-screen pattern can actually fit
 *	in off-screen memory :-}
 * S000, 16-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

static void
nteTileAreaSlowXExpand(
	BoxRec *pbox,
	DDXPointRec *src,
	int max_width,
	unsigned char alu,
	unsigned long planemask,
	DrawableRec *pDraw)
{
        int width, height, x, y;
        int chunks, extra_width;
	BoxRec box;

        width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;
	x = pbox->x1;
	y = pbox->y1;
	box.y1 = y;
	box.y2 = box.y1 + height;
	box.x1 = x;
	box.x2 = x + max_width;
        chunks = width / max_width;
        extra_width = width % max_width;

        while (chunks--)
        {
		NTE(CopyRect)(&box, src, alu, planemask, pDraw);
                box.x1 += max_width;
		box.x2 = box.x1 + max_width;
        }
        if (extra_width)
	{
		box.x2 = box.x1 + extra_width;
		NTE(CopyRect)(&box, src, alu, planemask, pDraw);
	}
}

void
NTE(TileAreaSlow)(
	BoxRec *pbox,
	DDXPointRec *src,
	int max_height,
	int max_width,
	unsigned char alu,
	unsigned long planemask,
	DrawableRec *pDraw)
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
                nteTileAreaSlowXExpand(&screen_box, src, max_width, alu,
		    planemask, pDraw);
                screen_box.y1 += max_height;
        }
        if (extra_height)
        {
                screen_box.y2 = screen_box.y1 + extra_height;
                nteTileAreaSlowXExpand(&screen_box, src, max_width, alu,
		    planemask, pDraw);
        }
}

void
NTE(TileRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int min_width, min_height, max_width, max_height, i, width, height;
	BoxRec *p, box;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	DDXPointRec src;
	Bool clobber_alu, biggest_available;

	min_width = w * 2;
	min_height = h * 2;

	clobber_alu = alu == GXcopy || alu == GXcopyInverted || alu == GXclear
	    || alu == GXset || alu == GXinvert;

	if (min_width > ntePriv->wawidth || min_height > ntePriv->waheight ||
	    (nbox < 2 && clobber_alu))
	{
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
		    planemask, pDraw);
		return;
	}

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
	NTE(DrawImage)(&box, tile, stride, GXcopy, ~0, pDraw);
	box.x2 = box.x1 + max_width;
	box.y2 = box.y1 + max_height;
	nfbReplicateArea(&box, w, h, ~0, pDraw);

	max_width -= w;
	max_height -= h;

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
			NTE(CopyRect)(pbox, &src, alu, planemask, pDraw);
		else
			if (clobber_alu)
			{
				if (width < max_width)
					min_width = width;
				else
					min_width = max_width;
				if (height < max_height)
					min_height = height;
				else
					min_height = max_height;
				box.x1 = pbox->x1;
				box.y1 = pbox->y1;
				box.x2 = box.x1 + min_width;
				box.y2 = box.y1 + min_height;
				NTE(CopyRect)(&box, &src, alu, planemask,
				    pDraw);
				nfbReplicateArea(pbox, min_width, min_height,
				    planemask, pDraw);
			}
			else
                                /*
                                 * any raster op here requires mixing src and
                                 * dest - this is the slow way - fortunately
                                 * not too many apps will do this - to test
                                 * this run x11perf with -xor and tile ops
                                 * - this code is still faster than gen
                                 */
                                NTE(TileAreaSlow)(pbox, &src, max_height,
				    max_width, alu, planemask, pDraw);
		++pbox;
	}
}
