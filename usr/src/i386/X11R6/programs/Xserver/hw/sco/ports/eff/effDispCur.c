/*
 * @(#) effDispCur.c 11.1 97/10/22
 *
 * Copyright (C) 1992 The Santa Cruz Operation, Inc.
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
 * Modification History
 *
 * S011, 14-Jun-92, buckm
 *	stop paying attention to cursor rgb values;
 *	just use the cursor pixel values passed to us.
 * S010, 12-Jun-92, buckm
 *	apply bug fixes from staceyc's tms work:
 * 	- make sure pixel of cursor image is not visible
 *	  if the cursor mask does not cover that point
 *	- colormap can change under the cursor,
 *	  so check colormap at putup time to make sure this hasn't happened
 * 	- get rid of mask flicker
 * S009, 21-Nov-91, staceyc
 * 	use global window table instead of dummy pixmap
 * S008, 14-Nov-91, staceyc
 * 	512K card support
 * S007, 04-Nov-91, staceyc
 * 	fixed memory leak problem in realize
 * S006, 28-Aug-91, staceyc
 * 	new grafinfo api changes
 * S005, 28-Aug-91, staceyc
 * 	reworking of command queue
 * S004, 20-Aug-91, staceyc
 * 	fixed some parameter decs
 * S003, 05-Aug-91, staceyc
 * 	keep track of current cursor, may change without calling
 *	PutUpCursor - optimize mask bitmap
 * S002, 26-Jul-91, staceyc
 * 	convert to 8514a off-screen hardware cursor
 * S001, 16-Jul-91, staceyc
 * 	change init parameters per mikep's os changes
 * S000, 16-Jul-91, staceyc
 * 	initial mods from midispcur.c
 */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/

#define NEED_EVENTS
#include "X.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "windowstr.h"
#include "regionstr.h"
#include "dixstruct.h"
#include "scrnintstr.h"
#include "servermd.h"
#ifdef usl
#include "mi.h"
#endif
#include "mipointer.h"
#include "misprite.h"
#include "gcstruct.h"
#include "nfbScrStr.h"
#include "nfbDefs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

extern WindowPtr *WindowTable;

/* per-cursor per-screen private data */

#define EFF_DC_CURSOR(pScreen, pCursor) \
    ((effDCCursor_t *)((pCursor)->bits->devPriv[(pScreen)->myNum]))

typedef struct effDCCursor_t {
	unsigned char *cursor_image;
	unsigned char *mask_image;
	int stride;
	effCursorSave_t cursor_coords;
	} effDCCursor_t;

static void effDCBlitCursor();

/*
 * sprite/cursor method table
 */

static Bool	effDCRealizeCursor(),	    effDCUnrealizeCursor();
static Bool	effDCPutUpCursor(),	    effDCSaveUnderCursor();
static Bool	effDCRestoreUnderCursor(),   effDCMoveCursor();
static Bool	effDCChangeSave();

static miSpriteCursorFuncRec effDCFuncs = {
    effDCRealizeCursor,
    effDCUnrealizeCursor,
    effDCPutUpCursor,
    effDCSaveUnderCursor,
    effDCRestoreUnderCursor,
    effDCMoveCursor,
    effDCChangeSave,
};

Bool
effDCInitialize(pScreen)
ScreenPtr pScreen;
{
	effCursorData_t *effScrCurs;

	if (! scoSpriteInitialize(pScreen, &effDCFuncs))
		return FALSE;
	effScrCurs = EFF_CURSOR_DATA(pScreen);
	effScrCurs->current_cursor = (CursorPtr)0;

	return TRUE;
}

static Bool
effDCRealizeCursor(pScreen, pCursor)
ScreenPtr pScreen;
CursorPtr pCursor;
{
	int i, j, trans_width;
	effDCCursor_t *effCurs;
	effCursorData_t *effScrCurs;
	unsigned char *cursor_p, *mask_p, *mask_source_p, *cursor_source_p;

	if (pCursor->bits->refcnt > 1)
	{
		return TRUE;
	}

	effScrCurs = EFF_CURSOR_DATA(pScreen);
	effCurs = (effDCCursor_t *)xalloc(sizeof(effDCCursor_t));
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)effCurs;

	effCurs->stride = PixmapBytePad(pCursor->bits->width, 1);

	cursor_p = (unsigned char *)xalloc(effCurs->stride *
	    pCursor->bits->height);
	effCurs->cursor_image = cursor_p;
	mask_p = (unsigned char *)xalloc(effCurs->stride *
	    pCursor->bits->height);
	effCurs->mask_image = mask_p;

	trans_width = (pCursor->bits->width + 7) / 8;
	cursor_source_p = pCursor->bits->source;
	mask_source_p = pCursor->bits->mask;
	for (i = 0; i < pCursor->bits->height; ++i)
	{
		for (j = 0; j < trans_width; ++j)
		{
			cursor_p[j] = mask_source_p[j] & cursor_source_p[j];
			mask_p[j] = cursor_p[j] ^ mask_source_p[j];
		}
		cursor_p += effCurs->stride;
		mask_p += effCurs->stride;
		cursor_source_p += effCurs->stride;
		mask_source_p += effCurs->stride;
	}

	effCurs->cursor_coords.w = effDCMin(pCursor->bits->width,
	    effScrCurs->cursor_max_size);
	effCurs->cursor_coords.h = effDCMin(pCursor->bits->height,
	    effScrCurs->cursor_max_size);

	return TRUE;
}

static int
effDCMin(a, b)
int a, b;
{
	if (a < b)
		return a;
	return b;
}

static Bool
effDCUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
	effDCCursor_t *effCurs;

	if (pCursor->bits->refcnt <= 1)
	{
		effCurs = EFF_DC_CURSOR(pScreen, pCursor);
		xfree((unsigned char *)effCurs->cursor_image);
		xfree((unsigned char *)effCurs->mask_image);
		xfree((unsigned char *)effCurs);
		pCursor->bits->devPriv[pScreen->myNum] = 0;
	}
	return TRUE;
}

static void
effDCDrawOffScreenBitmap(pScreen, ximage, stride, off_screen_point, w, h)
ScreenPtr pScreen;
unsigned char *ximage;
int stride;
DDXPointPtr off_screen_point;
int w, h;
{
	BoxRec box;

	box.x1 = off_screen_point->x;
	box.y1 = off_screen_point->y;
	box.x2 = box.x1 + w;
	box.y2 = box.y1 + h;

	effDrawOpaqueMonoImage(&box, ximage, 0, stride, 1, 0, GXcopy,
	    EFF_GENERAL_OS_WRITE,
	    &WindowTable[pScreen->myNum]->drawable);
}

static Bool
effDCPutUpCursor(pScreen, pCursor, x, y, source, mask)
ScreenPtr pScreen;
CursorPtr pCursor;
unsigned long source, mask;
{
	effCursorData_t *effScrCurs;
	effDCCursor_t *effCurs;
	effCursorSave_t *effCoords, *effSave;

	effScrCurs = EFF_CURSOR_DATA(pScreen);
	effCurs = EFF_DC_CURSOR(pScreen, pCursor);
	effCoords = &effCurs->cursor_coords;
	effSave = &effScrCurs->save_coords;

	effDCDrawOffScreenBitmap(pScreen, effCurs->cursor_image,
	    effCurs->stride, &effScrCurs->cursor_bitmap,
	    effCoords->w, effCoords->h);
	effDCDrawOffScreenBitmap(pScreen, effCurs->mask_image,
	    effCurs->stride, &effScrCurs->mask_bitmap,
	    effCoords->w, effCoords->h);

	effCoords->x = x;
	effCoords->y = y;

	effScrCurs->current_cursor = pCursor;

	effDCBlitCursor(pScreen, effCurs, source, mask);

	return TRUE;
}

static Bool
effDCSaveUnderCursor(pScreen, x, y, w, h)
ScreenPtr pScreen;
int x, y, w, h;
{
	effPrivateData_t *effPriv;
	effCursorData_t *effScrCurs;
	effDCCursor_t *effCurs;
	DDXPointRec src_point;
	BoxRec dest_box;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effScrCurs = EFF_CURSOR_DATA(pScreen);

	w = effDCMin(w, effScrCurs->save_max_size);
	h = effDCMin(h, effScrCurs->save_max_size);

	effScrCurs->save_coords.x = x;
	effScrCurs->save_coords.y = y;
	effScrCurs->save_coords.w = w;
	effScrCurs->save_coords.h = h;

	dest_box.x1 = effScrCurs->save_under.x;
	dest_box.y1 = effScrCurs->save_under.y;
	dest_box.x2 = effDCMin(dest_box.x1 + w, effScrCurs->save_under.x +
	    effScrCurs->save_max_size);
	dest_box.y2 = effDCMin(dest_box.y1 + h, effScrCurs->save_under.y +
	    effScrCurs->save_max_size);

	/*
	 * do some clipping to keep all parameters to effCopyRect
	 * within the screen or the save area
	 */
	if (x < 0)
	{
		dest_box.x1 += -x;
		src_point.x = 0;
	}
	else
		src_point.x = x;
	if (y < 0)
	{
		dest_box.y1 += -y;
		src_point.y = 0;
	}
	else
		src_point.y = y;
	if (x + w > effPriv->width)
		dest_box.x2 -= x + w - effPriv->width;
	if (y + h > effPriv->height)
		dest_box.y2 -= y + h - effPriv->height;

	if (dest_box.y1 > dest_box.y2 || dest_box.x1 > dest_box.x2 ||
	    src_point.y < 0 || src_point.y > effPriv->height ||
	    src_point.x < 0 || src_point.x > effPriv->width ||
	    dest_box.y1 < effScrCurs->save_under.y || dest_box.y2 >
	    effScrCurs->save_under.y + effScrCurs->save_max_size ||
	    dest_box.x1 < effScrCurs->save_under.x || dest_box.x2 >
	    effScrCurs->save_under.x + effScrCurs->save_max_size)
		ErrorF("assertion failure: %s %d\n", __FILE__, __LINE__);
	else
		effCopyRect(&dest_box, &src_point, GXcopy, ~0,
		    (DrawablePtr *)0);

	return TRUE;
}

static Bool
effDCRestoreUnderCursor(pScreen, x, y, w, h)
ScreenPtr pScreen;
int x, y, w, h;
{
	effPrivateData_t *effPriv;
	effCursorData_t *effScrCurs;
	DDXPointRec src_point;
	BoxRec dest_box;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effScrCurs = EFF_CURSOR_DATA(pScreen);

	w = effDCMin(w, effScrCurs->save_max_size);
	h = effDCMin(h, effScrCurs->save_max_size);

	src_point.x = effScrCurs->save_under.x;
	src_point.y = effScrCurs->save_under.y;
	dest_box.x1 = x;
	dest_box.x2 = x + w;
	dest_box.y1 = y;
	dest_box.y2 = y + h;

	/*
	 * do some clipping to keep all parameters to effCopyRect
	 * within the screen or the save area
	 */
	if (x < 0)
	{
		dest_box.x1 = 0;
		src_point.x += -x;
	}
	if (y < 0)
	{
		dest_box.y1 = 0;
		src_point.y += -y;
	}
	if (x + w > effPriv->width)
		dest_box.x2 -= x + w - effPriv->width;
	if (y + h > effPriv->height)
		dest_box.y2 -= y + h - effPriv->height;

	if (dest_box.y1 > dest_box.y2 || dest_box.x1 > dest_box.x2 ||
	    dest_box.y1 < 0 || dest_box.y2 > effPriv->height ||
	    dest_box.x1 < 0 || dest_box.x2 > effPriv->width ||
	    src_point.y < effScrCurs->save_under.y ||
	    src_point.x < effScrCurs->save_under.x)
		ErrorF("assertion failure: %s %d\n", __FILE__, __LINE__);
	else
		effCopyRect(&dest_box, &src_point, GXcopy, ~0,
		    (DrawablePtr *)0);

	return TRUE;
}

static Bool
effDCChangeSave(pScreen, x, y, w, h, dx, dy)
ScreenPtr pScreen;
int x, y, w, h, dx, dy;
{
	effCursorData_t *effScrCurs;
	effCursorSave_t *effCoords;

	effScrCurs = EFF_CURSOR_DATA(pScreen);

	w = effDCMin(w, effScrCurs->save_max_size);
	h = effDCMin(h, effScrCurs->save_max_size);

	effCoords = &effScrCurs->save_coords;
	effDCRestoreUnderCursor(pScreen, effCoords->x, effCoords->y,
	    effCoords->w, effCoords->h);
	effDCSaveUnderCursor(pScreen, x, y, w, h);

	return TRUE;
}

static void
effDCBlitCursor(pScreen, effCurs, source, mask)
ScreenPtr pScreen;
effDCCursor_t *effCurs;
unsigned long source, mask;
{
	effPrivateData_t *effPriv;
	effCursorData_t *effScrCurs;
	DDXPointRec dest, mask_src, cursor_src;
	BoxRec box;
	int lx, ly;
	DrawablePtr dummy;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effScrCurs = EFF_CURSOR_DATA(pScreen);

	dummy = &WindowTable[pScreen->myNum]->drawable;
	cursor_src.x = effScrCurs->cursor_bitmap.x;
	cursor_src.y = effScrCurs->cursor_bitmap.y;
	mask_src.x = effScrCurs->mask_bitmap.x;
	mask_src.y = effScrCurs->mask_bitmap.y;
	dest.x = effCurs->cursor_coords.x;
	dest.y = effCurs->cursor_coords.y;
	lx = effCurs->cursor_coords.w;
	ly = effCurs->cursor_coords.h;

	if (dest.x < 0)
	{
		cursor_src.x += -dest.x;
		mask_src.x += -dest.x;
		lx += dest.x;
		dest.x = 0;
	}
	if (dest.y < 0)
	{
		cursor_src.y += -dest.y;
		mask_src.y += -dest.y;
		ly += dest.y;
		dest.y = 0;
	}
	if (dest.x + lx >= effPriv->width)
		lx -= dest.x + lx - effPriv->width;
	if (dest.y + ly >= effPriv->height)
		ly -= dest.y + ly - effPriv->height;
	box.x1 = dest.x;
	box.y1 = dest.y;
	box.x2 = dest.x + lx;
	box.y2 = dest.y + ly;

	effSetupCardForBlit(mask, GXcopy, EFF_GENERAL_OS_READ,
	    ~0);
	effStretchBlit(dummy, &mask_src, &box);
	effSetupCardForBlit(source, GXcopy, EFF_GENERAL_OS_READ,
	    ~0);
	effStretchBlit(dummy, &cursor_src, &box);
}

static Bool
effDCMoveCursor(pScreen, pCursor, x, y, w, h, dx, dy, source, mask)
ScreenPtr pScreen;
CursorPtr pCursor;
unsigned long source, mask;
{
	effDCCursor_t *effCurs;
	effCursorSave_t *effCoords, *effSave;
	effCursorData_t *effScrCurs;

	effCurs = EFF_DC_CURSOR(pScreen, pCursor);
	effScrCurs = EFF_CURSOR_DATA(pScreen);
	effCoords = &effCurs->cursor_coords;
	effSave = &effScrCurs->save_coords;

	effCoords->x = x + dx;
	effCoords->y = y + dy;

	effDCRestoreUnderCursor(pScreen, effSave->x, effSave->y,
	    effSave->w, effSave->h);
	/*
	 * PutUp isn't always called for a new cursor image, so deal
	 * with that here
	 */
	if (pCursor != effScrCurs->current_cursor)
		effDCPutUpCursor(pScreen, pCursor, effCoords->x, effCoords->y,
		    source, mask);
	else
		effDCBlitCursor(pScreen, effCurs, source, mask);

	return TRUE;
}

