/*
 * @(#) m32Cursor.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 07-Aug-93, buckm
 *	Created.
 * S001, 22-Aug-93, buckm
 *	There are some oddities with the hardware cursor in 1280x1024 mode:
 *	A full 64-wide cursor disappears when displayed at an odd x address;
 *	At the left edge of the screen, if the horizontal offset within the
 *	cursor is odd, the displayed cursor image wraps, showing column zero
 *	of the cursor on the right side of the cursor.
 *	These problems may be worked around by limiting the cursor width
 *	to 56 in 1280x1024 mode, and making sure that column zero of the
 *	cursor is always transparent.
 * S002, 30-Aug-93, buckm
 *	Do a bit more decision making in CursorInitialize;
 *	make sure hw cursor is off if using sw cursor.
 * S003, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */
/*
 * m32Cursor.c
 *
 * Mach32 hardware cursor routines.
 */

#include "X.h"
#include "servermd.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"

#ifdef usl
#include "mi/mi.h"
#endif
#include "mi/mipointer.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"                                       /* S003 vv*/
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"                                      /* S003 ^^*/
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

/*
 * Arrays to help convert cursor data from source and mask
 * bitmaps to m32 sprite 2-bit packed pixels.
 * In these sprite pixels '00' is sprite color 0 (background),
 * '01' is sprite color 1 (foreground), '10' is transparent.
 * We will convert four bits at a time of cursor source and mask
 * into a byte of sprite pixmap by indexing into the arrays below
 * and then anding the values together.  The arrays are contructed
 * so that: a source 0 bit becomes '10', a source 1 bit becomes '11',
 * a mask 0 bit becomes '10', a mask 1 bit becomes '01'.
 */
static unsigned char convCursSrc[16] =
			     {	0xAA,0xAB,0xAE,0xAF,0xBA,0xBB,0xBE,0xBF,
				0xEA,0xEB,0xEE,0xEF,0xFA,0xFB,0xFE,0xFF  };
static unsigned char convCursMsk[16] =
			     {	0xAA,0xA9,0xA6,0xA5,0x9A,0x99,0x96,0x95,
				0x6A,0x69,0x66,0x65,0x5A,0x59,0x56,0x55  };
static unsigned char maskLast[8] =
			     {	0xFF,0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F  };

#define	M32_TRANSPARENT_SPRITE	0xAAAA	/* transparent sprite pixels */

/*
 * Realize the cursor for the m32.  Generate a cursor image which is:
 * an unsigned char width, followed by an unsigned char height,
 * followed by unsigned chars hotx and hoty,
 * followed by width * height unsigned chars of cursor data.
 * For overly large cursors we could choose to retain a portion
 * containing the hot spot, but for now we will just retain the upper-left
 * corner, and move the hot spot inside.
 */
Bool
m32RealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	m32CursInfoPtr pCI = M32_CURSOR_INFO(pScr);
	unsigned char *image, *psrc, *pmsk;
	int w, wlast, h, stride;
	int bytes, words;
	int hotx, hoty;

	w = min(pCurs->bits->width,  pCI->maxw);
	h = min(pCurs->bits->height, pCI->maxh);

	hotx = min((unsigned int)pCurs->bits->xhot, pCI->maxw - 1); /* S003 */
	hoty = min((unsigned int)pCurs->bits->yhot, pCI->maxh - 1); /* S003 */

	/* make cursor image width a multiple of 8 (2 bytes) */
	wlast = w & 7;
	words = (w + 7) / 8;
	bytes = words * 2;

	if ((image = (unsigned char *)xalloc(4 + bytes * h)) == NULL)
		return FALSE;
	pCurs->devPriv[pScr->myNum] = (pointer)image;

	psrc = (unsigned char *)pCurs->bits->source;
	pmsk = (unsigned char *)pCurs->bits->mask;
	stride = PixmapBytePad((unsigned int)pCurs->bits->width, 1); /* S003 */

	/* generate the sprite image */
	*image++ = bytes;
	*image++ = h;
	*image++ = hotx;
	*image++ = hoty;

	words -= 1;
	while (--h >= 0) {
		int i, sbits, mbits;

		for (i = 0; i <= words; ++i) {
			sbits = psrc[i];
			mbits = pmsk[i];
			/* sigh, mask the last byte ? */
			if (i == words) {
				sbits &= maskLast[wlast];
				mbits &= maskLast[wlast];
			}
			/* finally, make 2 sprite bytes */
			*image++ = convCursSrc[sbits & 0xF] &
				   convCursMsk[mbits & 0xF];
			*image++ = convCursSrc[sbits >> 4] &
				   convCursMsk[mbits >> 4];
		}
		psrc += stride;
		pmsk += stride;
	}

	return TRUE;
}

Bool
m32UnrealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	xfree(pCurs->devPriv[pScr->myNum]);
	pCurs->devPriv[pScr->myNum] = NULL;
	return TRUE;
}


static void 
m32LoadSprite(pScr, image)
	ScreenPtr pScr;
	unsigned char *image;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);
	m32CursInfoPtr pCI = &pM32->cursInfo;
	unsigned short *p;
	int h;
	int y, x;
	int x0, xinc;
	int bytes, words, pixels;

	bytes	  = *image++;
	h	  = *image++;
	pCI->hotx = *image++;
	pCI->hoty = *image++;

	/* note unused area of sprite */
	pCI->uw = M32_SPRITE_WIDTH  - bytes * 4;
	pCI->uh = M32_SPRITE_HEIGHT - h;

	words = bytes / 2;
	xinc = M32_SPRITE_STRIDE >> pM32->pixBytesLog2;
	pixels = bytes >> pM32->pixBytesLog2;
	x0 = xinc - pixels;

	p = (unsigned short *)image;
	y = pCI->addr.y;
	x = pCI->addr.x + x0;

	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_WWHOST);
	outw(M32_ALU_FG_FN,	m32RasterOp[GXcopy]);
	outw(M32_WRT_MASK,	0xFFFF);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);
	outw(M32_CUR_Y,		y);

	while (--h >= 0) {
		M32_CLEAR_QUEUE(10);
		outw(M32_CUR_X,		x);
		outw(M32_BRES_COUNT,	pixels);
		switch (words) {
			case 8: outw(M32_PIX_TRANS, *p++);
			case 7: outw(M32_PIX_TRANS, *p++);
			case 6: outw(M32_PIX_TRANS, *p++);
			case 5: outw(M32_PIX_TRANS, *p++);
			case 4: outw(M32_PIX_TRANS, *p++);
			case 3: outw(M32_PIX_TRANS, *p++);
			case 2: outw(M32_PIX_TRANS, *p++);
			case 1: outw(M32_PIX_TRANS, *p++);
		}
		if ((x += xinc) >= pM32->fbPitch) {
			x = x0;
			M32_CLEAR_QUEUE(1);
			outw(M32_CUR_Y,	++y);
		}
	}
}

static void
m32ClearSpriteArea(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);
	m32CursInfoPtr pCI = &pM32->cursInfo;
	int pixels, count, x, y;

	M32_CLEAR_QUEUE(5);
	outw(M32_DP_CONFIG,	M32_DP_FILL);
	outw(M32_FRGD_COLOR,	M32_TRANSPARENT_SPRITE);
	outw(M32_ALU_FG_FN,	m32RasterOp[GXcopy]);
	outw(M32_WRT_MASK,	0xFFFF);
	outw(M32_LINEDRAW_OPT,	M32_LD_HORZ);

	pixels = M32_SPRITE_BYTES >> pM32->pixBytesLog2;
	x = pCI->addr.x;
	y = pCI->addr.y;

	do {
		if ((count = pM32->fbPitch - x) > pixels)
			count = pixels;

		outw(M32_CUR_X,		x);
		outw(M32_CUR_Y,		y);
		outw(M32_BRES_COUNT,	count);

		x  = 0;
		y += 1;
	} while ((pixels -= count) > 0);
}

static void
m32SetSpriteColor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	if (pScr->rootDepth <= 8) {
		ColormapPtr pmap = NFB_SCREEN_PRIV(pScr)->installedCmap;
		xColorItem fCol, bCol;

		fCol.red   = pCurs->foreRed;
		fCol.green = pCurs->foreGreen;
		fCol.blue  = pCurs->foreBlue;
		FakeAllocColor(pmap, &fCol);

		bCol.red   = pCurs->backRed;
		bCol.green = pCurs->backGreen;
		bCol.blue  = pCurs->backBlue;
		FakeAllocColor(pmap, &bCol);

		outw(M32_CURSOR_COLOR, (fCol.pixel << 8) | bCol.pixel);

		FakeFreeColor(pmap, fCol.pixel);
		FakeFreeColor(pmap, bCol.pixel);
	} else {
		outw(M32_CURSOR_COLOR,
			(pCurs->foreBlue & 0xFF00) | (pCurs->backBlue >> 8));
		outw(M32_EXT_CURSOR_COLOR_0,
			(pCurs->backRed & 0xFF00) | (pCurs->backGreen >> 8));
		outw(M32_EXT_CURSOR_COLOR_1,
			(pCurs->foreRed & 0xFF00) | (pCurs->foreGreen >> 8));
	}
}


/*
 * m32MoveCursor() - Move the cursor.
 */
void							/* S003 */
m32MoveCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	m32CursInfoPtr pCI = M32_CURSOR_INFO(pScr);
	int hpos, hoff;
	int vpos, voff, coff;

	/* set x position */
	hoff = pCI->uw;
	if ((hpos = x - pCI->hotx) < 0) {
		hoff -= hpos;
		hpos  = 0;
	}
	outw(M32_HORZ_CURSOR_POSN,   hpos);
	outw(M32_HORZ_CURSOR_OFFSET, hoff);

	/* set y position */
	coff = pCI->offset;
	voff = pCI->uh;
	if ((vpos = y - pCI->hoty) < 0) {
		voff -= vpos;
		coff -= vpos * M32_SPRITE_STRIDE;
		vpos  = 0;
	}
	outw(M32_CURSOR_OFFSET_LO,   coff >>  2);
	outb(M32_CURSOR_OFFSET_HI,   coff >> 18);
	outw(M32_VERT_CURSOR_POSN,   vpos);
	outw(M32_VERT_CURSOR_OFFSET, voff);
}

/*
 * m32SetCursor() - Install and position the given cursor.
 */
void 
m32SetCursor(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
{
	m32CursInfoPtr pCI = M32_CURSOR_INFO(pScr);

	m32EnableCursor(FALSE);

	if (pCurs == NULL)
		return;

	/* load image */
	m32LoadSprite(pScr, pCurs->devPriv[pScr->myNum]);

	/* set color */
	m32SetSpriteColor(pScr, pCurs);

	/* set position */
	m32MoveCursor(pScr, x, y);

	m32EnableCursor(TRUE);
}

/*
 * m32RecolorCursor() - set the foreground and background cursor colors.
 *			We do not have to re-realize the cursor.
 */
void							/* S003 */
m32RecolorCursor(pScr, pCurs, displayed)
	ScreenPtr pScr;
	CursorPtr pCurs;
	Bool displayed;
{
	if (displayed)
		m32SetSpriteColor(pScr, pCurs);
}

/*
 * m32CursorInitialize() - Init the cursor and register movement routines.
 *			   We can do a better job than miRecolorCursor,
 *			   so replace it after pointer init.
 */

static miPointerSpriteFuncRec m32PointerFuncs = {
	m32RealizeCursor,
	m32UnrealizeCursor,
	m32SetCursor,
	m32MoveCursor
};

void
m32CursorInitialize(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);
	m32CursInfoPtr pCI = &pM32->cursInfo;

	m32EnableCursor(FALSE);

	if (pM32->useSWCurs) {
		scoSWCursorInitialize(pScr);
		return;
	}

	if(scoPointerInitialize(pScr, &m32PointerFuncs, TRUE) == 0)
		FatalError("m32: Cannot initialize cursor.\n");

	pScr->RecolorCursor = m32RecolorCursor;

	pCI->maxw = M32_SPRITE_WIDTH;
	pCI->maxh = M32_SPRITE_HEIGHT;
	if (pScr->width == 1280)
		pCI->maxw -= 8;

	m32ClearSpriteArea(pScr);
}

/*
 * m32RestoreCursor() - restore cursor after screen switch.
 */
void
m32RestoreCursor(pScr)
	ScreenPtr pScr;
{
	m32ClearSpriteArea(pScr);
}

/*
 * m32EnableCursor() - enable/disable cursor.
 */
m32EnableCursor(enable)
	Bool enable;
{
	outb(M32_CURSOR_ENABLE, enable ? 0x80 : 0x00);
}
