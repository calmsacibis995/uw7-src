/*
 *	@(#)ctCursor.c	11.1	10/22/97	12:33:47
 *	@(#) ctCursor.c 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/*
 *	S001	Wed May 28 15:47:47 PDT 1997	-	hiramc@sco.COM
 *      - porting to Gemini, changes marked by #if defined(usl)
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 * ctCursor.c
 *
 * Template for hardware cursor routines.  If you are using the
 * software cursor, ignore these routines.   
 */

/*
 * These are the basic routines needed to implement a hardware cursor.
 * For software cursors see midispcur.c or just call scoSWCursorInit() 
 * from ctInit.c
 *
 * Note miReColorCursor works by calling UnRealize, Realize and Display.
 * Hence, your DisplayCursor routine must be able change the colors.
 */

#ident "@(#) $Id: ctCursor.c 58.1 96/10/09 "

#include <stdio.h>
#if defined(usl)			/*	S001	*/
#include "input.h"
#endif
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "servermd.h"
#include "scoext.h"

#include "mi/mipointer.h"
#include "mi/misprite.h"

#include "ctDefs.h"
#include "ctMacros.h"
#include "ctOnboard.h"

static Bool ctRealizeCursor(ScreenPtr, CursorPtr);
static Bool ctUnrealizeCursor(ScreenPtr, CursorPtr);
static void ctSetCursor(ScreenPtr, CursorPtr, int, int);
static void ctMoveCursor(ScreenPtr, int, int);
static void ctRecolorCursor(ScreenPtr, CursorPtr, Bool);

static miPointerSpriteFuncRec ctPointerFuncs = {
	ctRealizeCursor,
	ctUnrealizeCursor,
	ctSetCursor,
	ctMoveCursor,
};

typedef enum {
	CT_CURSOR_32 = 0x00000000,
	CT_CURSOR_64 = 0x00000001,
} ctCursorType;

typedef struct _CursorPriv {
	ctCursorType		type;		/* CT_CURSOR_32, CT_CURSOR_64 */
	unsigned char		*mask;		/* AND plane */
	unsigned char		*source;	/* XOR plane */
	int			size;		/* size of data in bytes */
	int			idx;		/* cursor cache index 0-3 */
	unsigned long		timestamp;	/* time last set */
	/*
	 * NOTE: This data structure is padded to 5 longs (20 bytes) so that the
	 * cursor data following it is dword aligned. The USL compiler actually
	 * takes care of this padding for us.
	 *
	 * unsigned char	mask_data[size]
	 * unsigned char	source_data[size]
	 */
} ctCursorRec, *ctCursorPtr;

#define CT_CURSOR_CACHE_SIZE	4
#define CT_CURSOR_NOT_RESIDENT	-1

typedef struct _CursorCache {
	CursorPtr		cursors[CT_CURSOR_CACHE_SIZE];
	unsigned long		present_time;	/* virtual wall clock */
} ctCursorCache, *ctCursorCachePtr;

/*******************************************************************************

				Cursor Macros

*******************************************************************************/

#define	CT_GET_TIMESTAMP(cache)	((cache)->present_time++)

/*
 * Disable Cursor at Cursor Read/Write Index Register (DR08:0).
 */
#define CT_DISABLE_CURSOR() {						\
	unsigned long xx_val;						\
	xx_val = CT_IND(CT_CURIDX);					\
	xx_val &= ~0x00000001L;						\
	CT_OUTD(CT_CURIDX, xx_val);					\
}

/*
 * Enable Cursor at Cursor Read/Write Index Register (DR08:0). Set the cursor
 * position relative to the Upper-Left-Corner of the active display (DR08:5).
 */
#define CT_ENABLE_CURSOR() {						\
	unsigned long xx_val;						\
	xx_val = CT_IND(CT_CURIDX);					\
	xx_val |= 0x00000021L;						\
	CT_OUTD(CT_CURIDX, xx_val);					\
}

/*
 * CT_SET_CURTYPE():
 *
 * Select either 32x32 or 64x64 cursor by setting the Cursor R/W Index register
 * (DR08[1]) to the following
 *
 *	0	32x32
 *	1	64x64
 */
#define CT_SET_CURTYPE(type) {						\
	unsigned long xx_val;						\
	xx_val = CT_IND(CT_CURIDX);					\
	xx_val &= ~(0x00000001L << 1);					\
	xx_val |= (((type) & 0x00000001L) << 1);			\
	CT_OUTD(CT_CURIDX, xx_val);					\
}

/*
 * CT_SELECT_CURSOR():
 *
 * Select one of four 32x32 cursors by setting the Cursor R/W Index register
 * (DR08[3:2]) and DR08[23:22] to the value of idx:
 *
 *	0x00	cursor 0
 *	0x01	cursor 1
 *	0x10	cursor 2
 *	0x11	cursor 3
 */
#define CT_SELECT_CURSOR(idx) {						\
	unsigned long xx_val;						\
	xx_val = CT_IND(CT_CURIDX);					\
	xx_val &= ~((0x00000003L << 2) | (0x00000003L << 22));		\
	xx_val |= ((((idx) & 0x00000003L) << 2) |			\
		   (((idx) & 0x00000003L) << 22));			\
	CT_OUTD(CT_CURIDX, xx_val);					\
}

/*
 * CT_SET_CURADDRESS():
 *
 * Select one of four 32x32 cursors by setting the Cursor R/W Index register
 * (DR08[23:22]) to the value of idx:
 *
 *	0x00	cursor 0
 *	0x01	cursor 1
 *	0x10	cursor 2
 *	0x11	cursor 3
 *
 * Also, zero bits DR08[21:16] and DR08[24]. When using a 64x64 cursor, idx 0
 * is always used; so bits DR08[24:16] are zero'd.
 */
#define CT_SET_CURADDRESS(idx) {					\
	unsigned long xx_val;						\
	xx_val = CT_IND(CT_CURIDX);					\
	xx_val &= ~0x01ff0000L;						\
	xx_val |= (((idx) & 0x00000003L) << 22);			\
	CT_OUTD(CT_CURIDX, xx_val);					\
}

#define CT_SET_CURCOLOR(r0, g0, b0, r1, g1, b1) {			\
	CT_OUTD(CT_CURCOLOR0, 						\
		(((r0) << 27) | ((g0) << 21) | ((b0) << 16) |		\
		 ((r1) << 11) | ((g1) << 5)  | (b1)));			\
}

/*
 * CT_SET_CURPOS():
 *
 * Set offset register (DR0B) to:
 *
 *	0-10	x position
 *	11-14	RESERVED
 *	15	x sign
 *	16-26	y position
 *	27-30	RESERVED
 *	31	y sign
 *
 * NOTE: assume parameters are passed as 16-bit signed shorts. x and y positions
 * are passed in pixels indicating the position of the upper-left corner of the
 * cursor relative to the upper-left corner of the screen. When the sign bits
 * (bits 15 and 31) are set, the corresponding coordinate is negative.
 */
#define CT_SET_CURPOS(x, y) {						\
	outw(CT_CURPOS, (x));						\
	outw(CT_CURPOS+2, (y));						\
}

#define CT_SET_CURSOR_BASE(addr) {					\
	CT_OUTD(CT_CURDATA, (addr));					\
}

/*
 * CT_SET_CURDATA():
 *
 * Write cursor bitmap data to cursor RAM through cursor data register (DR0C).
 *
 *	void *psrc;
 *	int bytes;
 *
 * NOTE: When writing cursor data to cursor RAM, the internal pointer is auto-
 * incremented so that it does not need to be re-written following each output
 * of cursor data. After writing the entire 64x64 AND plane to cursor RAM, the
 * internal pointer is automatically set to the beginning of the XOR plane.
 */
#define CT_SET_CURDATA(idx, asrc, xsrc, bytes) {			\
	unsigned char *xx_plong;					\
	unsigned char *memptr;						\
	int xx_count;							\
									\
	xx_plong = (unsigned char *)(asrc);				\
	memptr = (unsigned char *)ctPriv->cursor_memory;		\
	memptr = memptr + (idx * 256);					\
	xx_count = (bytes);						\
	while (xx_count--) {						\
		*memptr = *xx_plong;					\
		xx_plong++;						\
		memptr+=2;						\
	}								\
									\
	xx_plong = (unsigned char *)(xsrc);				\
	memptr = (unsigned char *)ctPriv->cursor_memory + 1;		\
	memptr = memptr + (idx * 256);					\
	xx_count = (bytes);						\
	while (xx_count--) {						\
		*memptr = *xx_plong;					\
		xx_plong++;						\
		memptr+=2;						\
	}								\
}

/*******************************************************************************

				Cursor Cache Routines

*******************************************************************************/

static void
ctCacheInitialize(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache;
	int idx;

	cache = (ctCursorCachePtr)xalloc(sizeof(ctCursorCache));

	for (idx = 0; idx < CT_CURSOR_CACHE_SIZE; idx++) {
		cache->cursors[idx] = (CursorPtr)NULL;
	}
	cache->present_time = 0L;
	ctPriv->cursorCache = (pointer)cache;
}

static void
ctCacheFree(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	xfree(ctPriv->cursorCache);
}

static void
ctCacheUnloadCursor(pScreen, pCursor)
ScreenPtr pScreen;
CursorPtr pCursor;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache = (ctCursorCachePtr)ctPriv->cursorCache;
	ctCursorPtr ctCursor = (ctCursorPtr)pCursor->devPriv[pScreen->myNum];

#ifdef DEBUG_PRINT
	ErrorF("CacheUnloadCursor(): c=0x%08x type=%d idx=%d\n",
		ctCursor, ctCursor->type, ctCursor->idx);
#endif /* DEBUG_PRINT */

	cache->cursors[ctCursor->idx] = (CursorPtr)NULL;
	ctCursor->idx = CT_CURSOR_NOT_RESIDENT;
	ctCursor->timestamp = 0L;
}

static void
ctCacheUnloadAllCursors(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache = (ctCursorCachePtr)ctPriv->cursorCache;
	int idx;

#ifdef DEBUG_PRINT
	ErrorF("CacheUnloadAllCursors(): screen=0x%08x\n", pScreen);
#endif /* DEBUG_PRINT */

	for (idx = 0; idx < CT_CURSOR_CACHE_SIZE; idx++) {
		if (cache->cursors[idx] != (CursorPtr)NULL) {
			ctCacheUnloadCursor(pScreen, cache->cursors[idx]);
		}
	}
}

static int
ctCacheFindOldestCursor(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache = (ctCursorCachePtr)ctPriv->cursorCache;
	CursorPtr pCursor;
	ctCursorPtr ctCursor;
	int idx, oldest_idx;
	unsigned long oldest_time;

#ifdef DEBUG_PRINT
	ErrorF("CacheFindOldestCursors(): screen=0x%08x\n", pScreen);
#endif /* DEBUG_PRINT */

	oldest_idx = -1;
	oldest_time = cache->present_time;
	for (idx = 0; idx < CT_CURSOR_CACHE_SIZE; idx++) {
		pCursor = cache->cursors[idx];
		if (pCursor == (CursorPtr)NULL) {
			/*
			 * Empty element is better than the least recently used
			 * one.
			 */
			return (idx);
		}
		ctCursor = (ctCursorPtr)pCursor->devPriv[pScreen->myNum];
		if (ctCursor->timestamp <= oldest_time) {
			oldest_idx = idx;
			oldest_time = ctCursor->timestamp;
		}
	}
	return (oldest_idx);
}

static int
ctCacheLoadCursor(pScreen, pCursor)
ScreenPtr pScreen;
CursorPtr pCursor;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache = (ctCursorCachePtr)ctPriv->cursorCache;
	ctCursorPtr ctCursor = (ctCursorPtr)pCursor->devPriv[pScreen->myNum];
	int idx;

	/* RST - this is wrong - the cursor is now carved out of screen memory */

	if (ctCursor->type == CT_CURSOR_64) {
		/*
		 * There is only enough cursor RAM to support one 64x64 cursor
		 * in memory at a time. Kick out all 32x32 cached cursors and
		 * use index 0.
		 */
		idx = 0;
		ctCacheUnloadAllCursors(pScreen);
		cache->present_time = 0L;	/* no history to remember */
	} else {
		idx = ctCacheFindOldestCursor(pScreen);
		if (cache->cursors[idx] != (CursorPtr)NULL) {
			ctCacheUnloadCursor(pScreen, cache->cursors[idx]);
		}
	}

#ifdef DEBUG_PRINT
	ErrorF("CacheLoadCursor(): c=0x%08x msk=0x%08x src=0x%08x type=%d\n",
		ctCursor, ctCursor->mask, ctCursor->source, ctCursor->type);
	ErrorF("	size=%d xhot=%d yhot=%d idx=%d time=%ld bytes=%d\n",
		ctCursor->size, pCursor->bits->xhot, pCursor->bits->yhot,
		idx, ctCursor->timestamp, ctCursor->size);
#endif /* DEBUG_PRINT */

	cache->cursors[idx] = pCursor;

	CT_SET_CURTYPE(ctCursor->type);
	CT_SET_CURADDRESS(idx);
	/* mask is the AND plane, source is the XOR plane */
	CT_WAIT_FOR_IDLE();
	CT_SET_CURDATA(idx, ctCursor->mask, ctCursor->source, ctCursor->size);

	return (idx);
}

/*******************************************************************************

				Private Routines

*******************************************************************************/

/*
 * Realize the Cursor Image.
 */
static Bool
ctRealizeCursor(pScreen, pCursor)
ScreenPtr pScreen;
CursorPtr pCursor;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorPtr ctCursor;
	ctCursorType type;
	int size, width, height, w, h, wib, srcinc, dstinc, dstride;
	unsigned char *pmaskbits, *psourcebits, *pmask, *psource;

#ifdef DEBUG_PRINT
	ErrorF("RealizeCursor(): screen=0x%08x cursor=0x%08x w,h=%d,%d\n",
		pScreen, pCursor, pCursor->bits->width, pCursor->bits->height);
#endif /* DEBUG_PRINT */

	width = (int)pCursor->bits->width;
	height = (int)pCursor->bits->height;

	if ((width <= 32) && (height <= 32)) {
		/*
		 * We can use a 32x32 cursor.
		 */
		type = CT_CURSOR_32;
		dstride = 32 / 8;
		size = 32 * dstride;
	} else {
		/*
		 * Need to use 64x64 cursor.
		 */
		type = CT_CURSOR_64;
		if (width > 64)
			width = 64;
		if (height > 64)
			height = 64;
		dstride = 64 / 8;
		size = 64 * dstride;
	}

	ctCursor = (ctCursorPtr)xalloc(sizeof(ctCursorRec) + (2 * size));
	if (ctCursor == (ctCursorPtr)NULL)
		return (FALSE);

	ctCursor->type = type;
	ctCursor->mask = (unsigned char *)(ctCursor + 1);
	ctCursor->source = ctCursor->mask + size;
	ctCursor->size = size;
	ctCursor->idx = CT_CURSOR_NOT_RESIDENT;
	ctCursor->timestamp = 0L;		/* always set when used */

	wib = (width + 7) / 8;
	srcinc = PixmapBytePad((int)pCursor->bits->width, 1) - wib;
	dstinc = dstride - wib;

	/*
	 * Make cursor data transparent (mask == all ones, source == all zeros).
	 */
	memset(ctCursor->mask, ~0, size);
	memset(ctCursor->source, 0, size);

	/*
	 * The Cursor Data Register (DR0C) MUST be written with 64-bits of
	 * 'valid' data per row in 64x64 mode and 32-bits of 'valid' data per
	 * row in 32x32 mode.
	 */
	pmaskbits = pCursor->bits->mask;
	psourcebits = pCursor->bits->source;
	pmask = ctCursor->mask;
	psource = ctCursor->source;
	while (height--) {
		w = wib;
		while (w--) {
			*pmask = ~(MSBIT_SWAP(*pmaskbits));
			*psource = (MSBIT_SWAP(*pmaskbits) &
				  ~(MSBIT_SWAP(*psourcebits)));
			pmaskbits++;
			psourcebits++;
			pmask++;
			psource++;
		}
		pmaskbits += srcinc;
		psourcebits += srcinc;
		pmask += dstinc;
		psource += dstinc;
	}

        pCursor->devPriv[pScreen->myNum] = (pointer)ctCursor;

	return (TRUE);
}

/*
 * Free anything allocated above.
 */
static Bool
ctUnrealizeCursor(pScreen, pCursor)
ScreenPtr pScreen;
CursorPtr pCursor;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorPtr ctCursor;

#ifdef DEBUG_PRINT
	ErrorF("UnrealizeCursor(): screen=0x%08x cursor=0x%08x\n",
		pScreen, pCursor);
#endif /* DEBUG_PRINT */

	if (pCursor->refcnt > 0)
		return (FALSE);

	ctCursor = (ctCursorPtr)(pCursor->devPriv[pScreen->myNum]);

	/*
	 * Clean up references to the cursor private structure:
	 *
	 *	1. Cursor cache
	 *	2. Screen private current cursor
	 *	3. Cursor private pointer
	 */
	if (ctCursor->idx != CT_CURSOR_NOT_RESIDENT) {
		ctCacheUnloadCursor(pScreen, pCursor);
	}
	if (ctPriv->cursor == (pointer)pCursor) {
		/*
		 * XXX What else to do if the current cursor in unrealized ???
		 */
		ctSetCursor(pScreen, NullCursor, 0, 0);
	}
        pCursor->devPriv[pScreen->myNum] = (pointer)NULL;

	/*
	 * Free the cursor private structure.
	 */
        xfree(ctCursor);

	return (TRUE);
}

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
ctSetCursor(pScreen, pCursor, x, y)
ScreenPtr pScreen;
CursorPtr pCursor;
int x, y;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctCursorCachePtr cache = (ctCursorCachePtr)ctPriv->cursorCache;
	ctCursorPtr ctCursor;

#ifdef DEBUG_PRINT
	ErrorF("SetCursor(): screen=0x%08x cursor=0x%08x (%d,%d)\n",
		pScreen, pCursor, x, y);
#endif /* DEBUG_PRINT */

	if (pCursor == NullCursor) {
		/*
		 * There is no current cursor. If there was a previous cursor,
		 * make the sprite disappear.
		 */
		if (ctPriv->cursor != (pointer)NULL) {
#ifdef DEBUG_PRINT
			ErrorF("	DISABLE previous cursor...\n");
#endif /* DEBUG_PRINT */
			CT_DISABLE_CURSOR();
			ctPriv->cursor = (pointer)NULL;
		}
		return;
	}

	ctCursor = (ctCursorPtr)(pCursor->devPriv[pScreen->myNum]);

	CT_DISABLE_CURSOR();

	if (ctCursor->idx == CT_CURSOR_NOT_RESIDENT) {
		/*
		 * Load the current cursor at an available cache index.
		 */
		ctCursor->idx = ctCacheLoadCursor(pScreen, pCursor);
	}
	
	/*
	 * Update the cursor timestamp.
	 */
	ctCursor->timestamp = CT_GET_TIMESTAMP(cache);

	CT_SELECT_CURSOR(ctCursor->idx);

	ctPriv->cursor = (pointer)pCursor;

	ctRecolorCursor(pScreen, pCursor, TRUE);
	ctMoveCursor(pScreen, x, y);

	CT_ENABLE_CURSOR();
}

/*
 *  Just move current sprite
 */
static void
ctMoveCursor(pScreen, x, y)
ScreenPtr pScreen;
int x, y;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	CursorPtr pCursor = (CursorPtr)ctPriv->cursor;
	unsigned short xpos, ypos;

#ifdef DEBUG_PRINT
	ErrorF("MoveCursor(): screen=0x%08x c=0x%08x (%d,%d)\n",
		pScreen, pCursor, x, y);
#endif /* DEBUG_PRINT */

	if (pCursor == (CursorPtr)NULL)
		return;

	/*
	 * Calculate the signed position of the upper-left corner of the cursor
	 * relative to the screen origin (0, 0).
	 */
	if (x < (int)pCursor->bits->xhot) {
		xpos = pCursor->bits->xhot - (unsigned short)x;
		xpos |= 0x8000;		/* sign-bit */
	} else {
		xpos = (unsigned short)x - pCursor->bits->xhot;
	}

	if (y < (int)pCursor->bits->yhot) {
		ypos = pCursor->bits->yhot - (unsigned short)y;
		ypos |= 0x8000;		/* sign-bit */
	} else {
		ypos = (unsigned short)y - pCursor->bits->yhot;
	}

	CT_SET_CURPOS(xpos, ypos);
}

/*
 * CT(RecolorCursor() - set the foreground and background cursor colors.
 *                      We do not have to re-realize the cursor.
 */
static void
ctRecolorCursor(pScreen, pCursor, displayed)
ScreenPtr pScreen;
CursorPtr pCursor;
Bool displayed;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	unsigned char r0, g0, b0;
	unsigned char r1, g1, b1;

#ifdef DEBUG_PRINT
	ErrorF("RecolorCursor(): screen=0x%08x cursor=0x%08x displayed=%d\n",
		pScreen, pCursor, displayed);
#endif /* DEBUG_PRINT */

        if (!displayed) {
		return;
	}

	r0 = (unsigned char)((pCursor->backRed >> 11) & 0x001f);
	g0 = (unsigned char)((pCursor->backGreen >> 10) & 0x003f);
	b0 = (unsigned char)((pCursor->backBlue >> 11) & 0x001f);
	
	r1 = (unsigned char)((pCursor->foreRed >> 11) & 0x001f);
	g1 = (unsigned char)((pCursor->foreGreen >> 10) & 0x003f);
	b1 = (unsigned char)((pCursor->foreBlue >> 11) & 0x001f);
	CT_SET_CURCOLOR(r0, g0, b0, r1, g1, b1);
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

void
CT(CursorInitialize)(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("CursorInitialize(): screen=0x%08x num=%d\n",
		pScreen, pScreen->myNum);
#endif /* DEBUG_PRINT */

	if (!ctPriv->cursorEnabled) {
		/*
		 * Just turn on the built-in SW cursor support.
		 */
		scoSWCursorInitialize(pScreen);
		return;
	}

	/*
	 * Allocate twice as many rows so that we can 1k align it.
	 */
	ctPriv->cursor_chunk = (void *)CT_MEM_ALLOC(pScreen, 16 * 2, 64, 16);
	ctPriv->cursor_memory = (unsigned char *)
		(((unsigned)CT_MEM_VADDR(ctPriv->cursor_chunk) 
			+ 1024) & ~0x3ff);
	
	CT_SET_CURSOR_BASE(
		(unsigned)ctPriv->cursor_memory - 
		(unsigned)ctPriv->fbPointer);

	ctCacheInitialize(pScreen);

	/*
	 * Initialize SCO hardware cursor support. Specify waitForUpdate=TRUE
	 * to update the cursor only when necessary (rather than on every
	 * mouse movement).
	 */
	if(scoPointerInitialize(pScreen, &ctPointerFuncs, TRUE) == 0) {
		FatalError("Cannot initialize Hardware Cursor\n");
		/* NOTREACHED */
	}

	pScreen->RecolorCursor = ctRecolorCursor;

	ctPriv->cursor = (pointer)NULL;
}

void
CT(CursorFree)(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);

	CT_MEM_FREE(pScreen, ctPriv->cursor_chunk);

#ifdef DEBUG_PRINT
	ErrorF("CursorFree(): screen=0x%08x num=%d\n",
		pScreen, pScreen->myNum);
#endif /* DEBUG_PRINT */

	if (!ctPriv->cursorEnabled) {
		/*
		 * We're not using a hardware cursor; so everything is handled
		 * for us by SCO.
		 */
		return;
	}

	ctCacheUnloadAllCursors(pScreen);
	ctCacheFree(pScreen);
}

void
CT(CursorOn)(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("CursorOn(): screen=0x%08x num=%d\n", pScreen, pScreen->myNum);
#endif /* DEBUG_PRINT */

	if (!ctPriv->cursorEnabled) {
		/*
		 * We're not using a hardware cursor; so everything is handled
		 * for us by SCO.
		 */
		return;
	}

	if (ctPriv->cursor == (pointer)NULL) {
		/*
		 * There is no current cursor; so defer enabling the cursor HW
		 * until we are given one.
		 */
		return;
	}

	CT_ENABLE_CURSOR();
}

void
CT(CursorOff)(pScreen, flushCache)
ScreenPtr pScreen;
Bool flushCache;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("CursorOff(): screen=0x%08x num=%d\n", pScreen, pScreen->myNum);
#endif /* DEBUG_PRINT */

	if (!ctPriv->cursorEnabled) {
		/*
		 * We're not using a hardware cursor; so everything is handled
		 * for us by SCO.
		 */
		return;
	}

	if (flushCache) {
		ctCacheUnloadAllCursors(pScreen);
	}

	CT_DISABLE_CURSOR();
}
