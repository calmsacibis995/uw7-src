/*
 *	@(#) btCursor.c 11.1 97/10/22
 *
 * Copyright (C) 1994 The Santa Cruz Operation, Inc.
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
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#if NTE_USE_BT_DAC

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

#define BT485_SELECT_REG(priv, reg) {					\
	unsigned char v;						\
									\
	NTE_OUTB((priv)->crtc_adr, NTE_EX_DAC_CT);			\
	v = NTE_INB((priv)->crtc_data);					\
	v &= ~0x03;							\
	v |= ((reg) & 0x03);						\
	NTE_OUTB((priv)->crtc_data, v);					\
}

#define BT485_CURSOR_ON(priv) {						\
	unsigned char v;						\
									\
	BT485_SELECT_REG((priv), 0x10);					\
	v = inb(0x03c9);						\
	v |= 0x03;							\
	outb(0x03c9, v);						\
}

#define BT485_CURSOR_OFF(priv) {					\
	unsigned char v;						\
									\
	BT485_SELECT_REG((priv), 0x10);					\
	v = inb(0x03c9);						\
	v &= ~0x03;							\
	outb(0x03c9, v);						\
}

#define BT485_SET_COMMAND_REG_3(priv) {					\
	unsigned char v;						\
									\
	BT485_SELECT_REG((priv), 0x01);					\
	v = inb(0x03c6);						\
	v |= 0x80;							\
	outb(0x03c6, v);						\
									\
	BT485_SELECT_REG((priv), 0x00);					\
	outb(0x03c8, 0x01);						\
									\
	BT485_SELECT_REG((priv), 0x10);					\
}

#define BT485_SET_CURSOR_PLANE(priv, plane) {				\
	unsigned char v;						\
									\
	BT485_SET_COMMAND_REG_3((priv));				\
	v = inb(0x03c6);						\
	v &= ~0x03;							\
	v |= ((plane) << 1);						\
	outb(0x03c6, v);						\
}

#define BT485_SET_CURSOR_DATA(priv, pdata, size) {			\
	unsigned char *p;						\
	int ii;								\
									\
	BT485_SELECT_REG((priv), 0x00);					\
	outb(0x03c8, 0x00);						\
									\
	BT485_SELECT_REG((priv), 0x10);					\
	for (ii = 0, p = (pdata); ii < (size); ii++, p++) {		\
		outb(0x03c7, *p);					\
	}								\
}

/*
 * Realize the Cursor Image.   This routine must remove the cursor from
 * the screen if pCursor == NullCursor.
 */
static Bool
nteRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
}

/*
 * Free anything allocated above
 */
static Bool
nteUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
}

static void
nteMoveCursor(ScreenPtr pScreen, int x, int y)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);

	x = x - ntePriv->xhot + 64;
	y = y - ntePriv->yhot + 64;

	BT485_SELECT_REG(ntePriv, 0x11);

	outb(0x03c8, (unsigned char)(x & 0x000000ff));		/* X-low */
	outb(0x03c9, (unsigned char)((x >> 8) & 0x0000000f));	/* X-high */

	outb(0x03c6, (unsigned char)(y & 0x000000ff));		/* Y-low */
	outb(0x03c7, (unsigned char)((y >> 8) & 0x0000000f));	/* Y-high */

	BT485_SELECT_REG(ntePriv, 0x00);
}

#if NTE_BITS_PER_PIXEL == 8
void
NTE(ColorCursor)(ScreenPtr pScreen)
{
	ColormapPtr pmap = NFB_SCREEN_PRIV(pScreen)->installedCmap;
	xColorItem fore, mask;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	CursorRec *pCursor;
	int shift = ntePriv->dac_shift;

	pCursor = ntePriv->cursor;

	fore.red = pCursor->foreRed;
	fore.green = pCursor->foreGreen;
	fore.blue = pCursor->foreBlue;
	fore.pixel = 0;
	FakeAllocColor(pmap, &fore);

	mask.red = pCursor->backRed;
	mask.green = pCursor->backGreen;
	mask.blue = pCursor->backBlue;
	mask.pixel = 0;
	FakeAllocColor (pmap, &mask);

	/* "free" the pixels right away, don't let this confuse you */
	FakeFreeColor(pmap, fore.pixel);
	FakeFreeColor(pmap, mask.pixel);

	ntePriv->cursor_fg = fore.pixel;
	ntePriv->cursor_bg = mask.pixel;

	/* Set starting cursor color write address */
	BT485_SELECT_REG(ntePriv, 0x01);
	outb(0x03c8, 0x01);

	/* Set cursor color 1 */
	outb(0x03c9, (unsigned char)(pCursor->backRed >> shift));
	outb(0x03c9, (unsigned char)(pCursor->backGreen >> shift));
	outb(0x03c9, (unsigned char)(pCursor->backBlue >> shift));

	/* Set cursor color 2 */
	outb(0x03c9, (unsigned char)(pCursor->foreRed >> shift));
	outb(0x03c9, (unsigned char)(pCursor->foreGreen >> shift));
	outb(0x03c9, (unsigned char)(pCursor->foreBlue >> shift));

	BT485_SELECT_REG(ntePriv, 0x00);
}
#endif /* NTE_BITS_PER_PIXEL == 8 */

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
nteSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	int width, height, dstride, size, wib, srcinc, dstinc, w;
	unsigned char *cursorBits, *mask, *source;
	unsigned char *pmask, *psource, *pmaskbits, *psourcebits;

	BT485_CURSOR_OFF(ntePriv);

	if (!pCursor)
		return;

        ntePriv->xhot = pCursor->bits->xhot;
        ntePriv->yhot = pCursor->bits->yhot;

	width = (int)pCursor->bits->width;
	height = (int)pCursor->bits->height;

	dstride = 64 / 8;
	size = 64 * dstride;

	cursorBits = (unsigned char *)ALLOCATE_LOCAL((2 * size));
	if (cursorBits == (unsigned char *)NULL)
		return;

	mask = cursorBits;
	source = mask + size;

	/* XXX initialize cursor buffers */

	/*
	 * Limit cursor size to 64x64
	 */
	if (width > 64)
		width = 64;
	if (height > 64)
		height = 64;

	wib = (width + 7) / 8;
	srcinc = PixmapBytePad((int)pCursor->bits->width, 1) - wib;
	dstinc = dstride - wib;

	pmaskbits = pCursor->bits->mask;
	psourcebits = pCursor->bits->source;
	pmask = mask;
	psource = source;
	while (height--) {
		w = wib;
		while (w--) {
			*pmask++ = MSBIT_SWAP(*pmaskbits++);
			*psource++ = MSBIT_SWAP(*psourcebits++);
		}
		pmaskbits += srcinc;
		psourcebits += srcinc;
		pmask += dstinc;
		psource += dstinc;
	}

	BT485_SET_CURSOR_PLANE(ntePriv, 0);
	BT485_SET_CURSOR_DATA(ntePriv, mask, size);

	BT485_SET_CURSOR_PLANE(ntePriv, 1);
	BT485_SET_CURSOR_DATA(ntePriv, source, size);

	DEALLOCATE_LOCAL(cursorBits);

	nteMoveCursor(pScreen, x, y);

#if NTE_BITS_PER_PIXEL == 8
	ntePriv->cursor = pCursor;
	NTE(ColorCursor)(pScreen);
#endif

	BT485_CURSOR_ON(ntePriv);

	BT485_SELECT_REG(ntePriv, 0x00);
}

static miPointerSpriteFuncRec ntePointerFuncs = {
	nteRealizeCursor,
	nteUnrealizeCursor,
	nteSetCursor,
	nteMoveCursor
};

/* 
 * Initialize the cursor and register movement routines.
 */
void
NTE(CursorInitialize)(ScreenPtr pScreen)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned char v;

	ErrorF("CursorInitialize()\n");

#if 0
	if(scoPointerInitialize(pScreen, &ntePointerFuncs, NULL, TRUE) == 0)
		FatalError("Cannot initialize Hardware Cursor\n");
#else
	scoSWCursorInitialize(pScreen);
#endif

	BT485_SELECT_REG(ntePriv, 0x00);
	v = inb(0x03c6);	/* Pixel Mask Register */
	ErrorF("Pixel Mask Register: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x01);
	v = inb(0x03c6);	/* Command Register 0 */
	ErrorF("Command Register 0: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x10);
	v = inb(0x03c8);	/* Command Register 1 */
	ErrorF("Command Register 1: 0x%02x\n", v);
	v = inb(0x03c9);	/* Command Register 2 */
	ErrorF("Command Register 2: 0x%02x\n", v);

	BT485_SET_COMMAND_REG_3(ntePriv);
	v = inb(0x03c6);	/* Command Register 3 */
	ErrorF("Command Register 3: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x00);
	outb(0x03c6, 0xff);	/* Pixel Mask Register */
	v = inb(0x03c6);	/* Pixel Mask Register */
	ErrorF("Pixel Mask Register: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x01);
	v = inb(0x03c6);	/* Command Register 0 */
	v &= ~0x02;		/* 6-bit DAC */
	outb(0x03c6, v);
	v = inb(0x03c6);	/* Command Register 0 */
	ErrorF("Command Register 0: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x10);
	v = inb(0x03c8);	/* Command Register 1 */
	v &= ~0x70;		/* pixel addresses palette */
	v |= 0x40;		/* 8-bits-per-pixel */
	outb(0x03c8, v);
	v = inb(0x03c8);	/* Command Register 1 */
	ErrorF("Command Register 1: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x10);
#if 0
	v = inb(0x03c9);	/* Command Register 2 */
	v |= 0x20;		/* PORTSEL pin */
	v &= ~0x08;		/* non-interlaced */
#endif
	outb(0x03c9, 0x0c);
	v = inb(0x03c9);	/* Command Register 2 */
	ErrorF("Command Register 2: 0x%02x\n", v);

	BT485_CURSOR_OFF(ntePriv);

	BT485_SET_COMMAND_REG_3(ntePriv);
#if 0
	v = inb(0x03c6);	/* Command Register 3 */
	v &= ~0xf0;		/* turn off reserved bits */
	v |= 0x04;		/* Select 64x64x2 cursor */
#endif
	outb(0x03c6, 0x04);
	v = inb(0x03c6);	/* Command Register 3 */
	ErrorF("Command Register 3: 0x%02x\n", v);

	BT485_SELECT_REG(ntePriv, 0x00);
}

#if NTE_BITS_PER_PIXEL == 8
void
NTE(SetColor)(
	unsigned int cmap,
	unsigned int index,
	unsigned short r,
	unsigned short g,
	unsigned short b,
	ScreenPtr pScreen)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned int shift = ntePriv->dac_shift;
	CursorRec *pCursor = ntePriv->cursor;

#if 0
	ErrorF("SetColor() idx=%d (0x%04x,0x%04x,0x%04x)\n", index, r, g, b);
#endif

	if (pCursor &&
	    (((index == ntePriv->cursor_fg) && (pCursor->foreRed != r ||
	    pCursor->foreGreen != g || pCursor->foreBlue != b)) ||
	    ((index == ntePriv->cursor_bg) && (pCursor->backRed != r ||
	    pCursor->backGreen != g || pCursor->backBlue != b))))
		NTE(ColorCursor)(pScreen);

	/* Set palette color write address */
	BT485_SELECT_REG(ntePriv, 0x00);
	outb(0x03c8, (unsigned char)index);

	/* Set palette color */
	outb(0x03c9, (unsigned char)(r >> shift));
	outb(0x03c9, (unsigned char)(g >> shift));
	outb(0x03c9, (unsigned char)(b >> shift));

	BT485_SELECT_REG(ntePriv, 0x00);
}
#endif /* NTE_BITS_PER_PIXEL == 8 */

#endif /* NTE_USE_BT_DAC */

