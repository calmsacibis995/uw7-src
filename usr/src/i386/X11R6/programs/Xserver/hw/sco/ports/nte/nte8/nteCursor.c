/*
 *	@(#) nteCursor.c 11.1 97/10/22
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
 * S004, 27-Jun-94, hiramc
 *	ready for 864/964 implementation, from DoubleClick
 * S003, 23-Aug-93, staceyc
 * 	move screen cursor private init into main init code
 * S002, 20-Aug-93, staceyc
 * 	basic S3 hardware cursor - does not work with 16/32bpp modes
 * S001, 08-Jun-93, staceyc
 * 	remove mi includes, they are now coming from s3cconsts.h
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#if !NTE_USE_BT_DAC

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

/*
 * Realize the Cursor Image.   This routine must remove the cursor from
 * the screen if pCursor == NullCursor.
 */
static Bool
nteRealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor)
{
}

/*
 * Free anything allocated above
 */
static Bool
nteUnrealizeCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor)
{
}

static void
nteMoveCursor(
	ScreenPtr pScreen,
	int x,
	int y)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	int xpos, xoff, ypos, yoff;

        if ((xpos = x - ntePriv->xhot) < 0)
        {
                xoff = -xpos;
                xpos = 0;
        }
        else
                xoff = 0;

        if ((ypos = y - ntePriv->yhot) < 0)
        {
                yoff = -ypos;
                ypos = 0;
        }
        else
                yoff = 0;

        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_DX);
        NTE_OUTB(ntePriv->crtc_data, xoff);

        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_DY);
        NTE_OUTB(ntePriv->crtc_data, yoff);
                
        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_ORGXL);
        NTE_OUTB(ntePriv->crtc_data, xpos);

        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_ORGXH);
        NTE_OUTB(ntePriv->crtc_data, xpos >> 8);

        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_ORGYL);
        NTE_OUTB(ntePriv->crtc_data, ypos);

        NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_ORGYH);
        NTE_OUTB(ntePriv->crtc_data, ypos >> 8);
}

#if NTE_BITS_PER_PIXEL == 8
void
NTE(ColorCursor)(
	ScreenPtr pScreen)
{
	ColormapPtr pmap = NFB_SCREEN_PRIV(pScreen)->installedCmap;
	xColorItem fore, mask;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	CursorRec *pCursor;

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

       	NTE_OUTB(ntePriv->crtc_adr, NTE_CRE);
       	NTE_OUTB(ntePriv->crtc_data, fore.pixel);
       	NTE_OUTB(ntePriv->crtc_adr, NTE_CRF);
       	NTE_OUTB(ntePriv->crtc_data, mask.pixel);

	ntePriv->cursor_fg = fore.pixel;
	ntePriv->cursor_bg = mask.pixel;
}
#endif

/*
 * Move and possibly change current sprite.   This routine must remove 
 * the cursor from the screen if pCursor == NullCursor.
 */
static void
nteSetCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor,
	int x,
	int y)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned char hgc_mode, ex_dac_ct;
	unsigned short *cursor_image, *cursor_image_p, *image_p;
	int cursor_stride = NTE_HW_CURSOR_DATA_SIZE / sizeof(unsigned short);
	int width, height, stride, width_bytes, extra, width_words, i;
	unsigned char *and_image, *xor_image, *xor_image_p, *and_image_p;
	unsigned short *pix_trans16;

	NTE_OUTB(ntePriv->crtc_adr, NTE_HGC_MODE);
	hgc_mode = NTE_INB(ntePriv->crtc_data);
	hgc_mode &= ~NTE_HWGC_ENB;
	NTE_OUTB(ntePriv->crtc_data, hgc_mode);

	if (! pCursor)
		return;

        ntePriv->xhot = pCursor->bits->xhot;
        ntePriv->yhot = pCursor->bits->yhot;

#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
#endif

	cursor_image =
	    (unsigned short *)ALLOCATE_LOCAL(NTE_HW_CURSOR_DATA_SIZE);
	cursor_image_p = cursor_image;
	memset(cursor_image, 0, NTE_HW_CURSOR_DATA_SIZE);

	width = pCursor->bits->width;
	if (width > NTE_HW_CURSOR_MAX)
		width = NTE_HW_CURSOR_MAX;
	height = pCursor->bits->height;
	if (height > NTE_HW_CURSOR_MAX)
		height = NTE_HW_CURSOR_MAX;

	stride = PixmapBytePad(pCursor->bits->width, 1);
	width_bytes = (width + 7) / 8;
	extra = width_bytes & 1;
	width_words = width_bytes / 2;
	and_image = pCursor->bits->mask;
	xor_image = pCursor->bits->source;

	while (height--)
	{
		xor_image_p = xor_image;
		and_image_p = and_image;
		image_p = cursor_image_p;
		i = width_words;
		while (i--)
		{
			*image_p++ = MSBIT_SWAP(and_image_p[0]) |
			    (MSBIT_SWAP(and_image_p[1]) << 8);
			*image_p++ = MSBIT_SWAP(xor_image_p[0]) |
			    (MSBIT_SWAP(xor_image_p[1]) << 8);
			xor_image_p += 2;
			and_image_p += 2;
		}
		if (extra)
		{
			*image_p++ = MSBIT_SWAP(*and_image_p);
			*image_p = MSBIT_SWAP(*xor_image_p);
		}
		cursor_image_p += NTE_HW_CURSOR_MAX / 8;
		xor_image += stride;
		and_image += stride;
	}

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(7);
	NTE_CLEAR_QUEUE24(8);
	NTE_FRGD_MIX(NTE_CPU_SOURCE, NTE(RasterOps)[GXcopy]);
	NTE_WRT_MASK(~0);
	NTE_CURX(0);
	NTE_CURY(ntePriv->hw_cursor_y);
    	NTE_MAJ_AXIS_PCNT(NTE_HW_CURSOR_DATA_SIZE - 1);
	NTE_MIN_AXIS_PCNT(1 - 1);
	NTE_WAIT_FOR_IDLE();
	NTE_CMD(S3C_CURS_X_Y_DATA);

	i = (NTE_HW_CURSOR_DATA_SIZE / sizeof(unsigned short)) / 2;
	cursor_image_p = cursor_image;
	while (i--)
	{
		NTE_PIX_TRANS(*pix_trans16, *cursor_image_p++);
		NTE_PIX_TRANS(*pix_trans16, *cursor_image_p++);
	}
	
	NTE_END();

	DEALLOCATE_LOCAL((unsigned char *)cursor_image);

	nteMoveCursor(pScreen, x, y);

	NTE_OUTB(ntePriv->crtc_adr, NTE_EX_DAC_CT);
	ex_dac_ct = NTE_INB(ntePriv->crtc_data);
	ex_dac_ct |= NTE_MS_X11;
	NTE_OUTB(ntePriv->crtc_data, ex_dac_ct);

	NTE_OUTB(ntePriv->crtc_adr, NTE_HGC_MODE);
	hgc_mode = NTE_INB(ntePriv->crtc_data);

#if NTE_BITS_PER_PIXEL == 8
	ntePriv->cursor = pCursor;
	NTE(ColorCursor)(pScreen);
#endif

	hgc_mode |= NTE_HWGC_ENB;
	NTE_OUTB(ntePriv->crtc_adr, NTE_HGC_MODE);
	NTE_OUTB(ntePriv->crtc_data, hgc_mode);
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
NTE(CursorInitialize)(
	ScreenPtr pScreen)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned char hgc_mode, ex_dac_ct;

	if(scoPointerInitialize(pScreen, &ntePointerFuncs, NULL, TRUE) == 0)
		FatalError("Cannot initialize Hardware Cursor\n");

	NTE_OUTB(ntePriv->crtc_adr, NTE_HGC_MODE);
	hgc_mode = NTE_INB(ntePriv->crtc_data);
	hgc_mode &= ~NTE_HWGC_ENB;
	NTE_OUTB(ntePriv->crtc_data, hgc_mode);

	NTE_OUTB(ntePriv->crtc_adr, NTE_EX_DAC_CT);
	ex_dac_ct = NTE_INB(ntePriv->crtc_data);
	ex_dac_ct |= NTE_MS_X11;
	NTE_OUTB(ntePriv->crtc_data, ex_dac_ct);

	NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_STAL);
	NTE_OUTB(ntePriv->crtc_data, ntePriv->hw_cursor_y);
	NTE_OUTB(ntePriv->crtc_adr, NTE_HWGC_STAH);
	NTE_OUTB(ntePriv->crtc_data, ntePriv->hw_cursor_y >> 8);
}
#endif /* !NTE_USE_BT_DAC */
