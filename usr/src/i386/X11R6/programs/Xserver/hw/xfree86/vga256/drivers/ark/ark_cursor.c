/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ark/ark_cursor.c,v 3.0 1995/03/11 14:16:59 dawes Exp $ */
/*
 * Copyright 1994  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (hhanemaa@cs.ruu.nl).
 */
/* $XConsortium: ark_cursor.c /main/3 1995/11/13 07:23:35 kaleb $ */

/*
 * Hardware cursor handling. Adapted from cirrus/cir_cursor.c and
 * accel/s3/s3Cursor.c.
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"

extern Bool vgaUseLinearAddressing;

static Bool ArkRealizeCursor();
static Bool ArkUnrealizeCursor();
static void ArkSetCursor();
static void ArkMoveCursor();
static void ArkRecolorCursor();

static miPointerSpriteFuncRec arkPointerSpriteFuncs =
{
   ArkRealizeCursor,
   ArkUnrealizeCursor,
   ArkSetCursor,
   ArkMoveCursor,
};

/* vga256 interface defines Init, Restore, Warp, QueryBestSize. */


extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int arkCursorGeneration = -1;
static int arkCursorControlMode;
static int arkCursorAddress;

/*
 * This is the set variables that defines the cursor state within the
 * driver.
 */

int arkCursorHotX;
int arkCursorHotY;
int arkCursorWidth;	/* Must be set before calling ArkCursorInit. */
int arkCursorHeight;
static CursorPtr arkCursorpCurs;


/* Table with bit-reversed equivalent for each possible byte. */
/* This belongs somewhere else. */

static unsigned char byte_reversed[256] = {
	0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,
	0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
	0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,
	0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
	0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,
	0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
	0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,
	0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
	0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,
	0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
	0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,
	0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
	0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,
	0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
	0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,
	0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
	0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,
	0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
	0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,
	0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
	0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,
	0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
	0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,
	0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
	0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,
	0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
	0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,
	0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
	0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,
	0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
	0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,
	0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff,
};



/*
 * This is a high-level init function, called once; it passes a local
 * miPointerSpriteFuncRec with additional functions that we need to provide.
 * It is called by the SVGA server.
 */

Bool ArkCursorInit(pm, pScr)
	char *pm;
	ScreenPtr pScr;
{
	arkCursorHotX = 0;
	arkCursorHotY = 0;

	if (arkCursorGeneration != serverGeneration)
		if (!(miPointerInitialize(pScr, &arkPointerSpriteFuncs,
		&xf86PointerScreenFuncs, FALSE)))
			return FALSE;

	arkCursorGeneration = serverGeneration;

	/*
	 * Define the ARK cursor mode: X style, size, correct color depth.
	 */
	arkCursorControlMode = 0x10;
	if (arkCursorWidth == 64)
		/* 64x64 cursor. */
		arkCursorControlMode |= 0x04;
	if (vgaBitsPerPixel == 16)
		arkCursorControlMode |= 0x01;
	if (vgaBitsPerPixel == 32)
		arkCursorControlMode |= 0x02;

	arkCursorAddress = vga256InfoRec.videoRam * 1024 - 256;

	return TRUE;
}

/*
 * This enables displaying of the cursor by the ARK graphics chip.
 * It's a local function, it's not called from outside of the module.
 */

static void ArkShowCursor() {
	/* Enable the hardware cursor. */
	wrinx(0x3C4, 0x20, arkCursorControlMode | 0x08);
}

/*
 * This disables displaying of the cursor by the ARK graphics chip.
 * This is also a local function, it's not called from outside.
 */

void ArkHideCursor() {
	/* Disable the hardware cursor. */
	wrinx(0x3C4, 0x20, arkCursorControlMode);
}

/*
 * This function is called when a new cursor image is requested by
 * the server. The main thing to do is convert the bitwise image
 * provided by the server into a format that the graphics card
 * can conveniently handle, and store that in system memory.
 * Adapted from accel/s3/s3Cursor.c.
 */

static Bool ArkRealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
   register int i, j;
   unsigned char *pServMsk;
   unsigned char *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int   wsrc, h;
   unsigned char *ram;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned char *)xalloc(1024);
   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned char *)bits->source;
   pServMsk = (unsigned char *)bits->mask;

#define MAX_CURS 32

   h = bits->height;
   if (h > MAX_CURS)
      h = MAX_CURS;

   wsrc = PixmapBytePad(bits->width, 1);	/* Bytes per line. */

   for (i = 0; i < MAX_CURS; i++) {
      for (j = 0; j < MAX_CURS / 8; j++) {
	 unsigned char mask, source;

	 if (i < h && j < wsrc) {
	    mask = *pServMsk++;
	    source = *pServSrc++;

	    mask = byte_reversed[mask];
	    source = byte_reversed[source];

	    if (j < MAX_CURS / 8) {	/* ??? */
	       *ram++ = mask;
	       *ram++ = source;
	    }
	 } else {
	    *ram++ = 0x00;
	    *ram++ = 0xFF;
	 }
      }
      /*
       * if we still have more bytes on this line (j < wsrc),
       * we have to ignore the rest of the line.
       */
       while (j++ < wsrc) pServMsk++,pServSrc++;
   }
   return TRUE;
}

/*
 * This is called when a cursor is no longer used. The intermediate
 * cursor image storage that we created needs to be deallocated.
 */

static Bool ArkUnrealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	pointer priv;

	if (pCurs->bits->refcnt <= 1 &&
	(priv = pCurs->bits->devPriv[pScr->myNum])) {
		xfree(priv);
		pCurs->bits->devPriv[pScr->myNum] = 0x0;
	}
	return TRUE;
}

/*
 * This function uploads a cursor image to the video memory of the
 * graphics card. The source image has already been converted by the
 * Realize function to a format that can be quickly transferred to
 * the card.
 * This is a local function that is not called from outside of this
 * module.
 */

extern void ArkSetWrite();

static void ArkLoadCursorToCard(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;	/* Not used for ARK. */
{
	unsigned char *cursor_image;
	int index = pScr->myNum;

	if (!xf86VTSema)
		return;

	cursor_image = pCurs->bits->devPriv[index];

	if (vgaUseLinearAddressing)
		memcpy((unsigned char *)vgaLinearBase + arkCursorAddress,
			cursor_image, 256);
	else {
		/*
		 * The cursor can only be in the last 16K of video memory,
		 * which fits in the last banking window.
		 */
		vgaSaveBank();
		ArkSetWrite(arkCursorAddress >> 16);
		memcpy((unsigned char *)vgaBase + (arkCursorAddress & 0xFFFF),
			cursor_image, 256);
		vgaRestoreBank();
	}
}

/*
 * This function should make the graphics chip display new cursor that
 * has already been "realized". We need to upload it to video memory,
 * make the graphics chip display it.
 * This is a local function that is not called from outside of this
 * module (although it largely corresponds to what the SetCursor
 * function in the Pointer record needs to do).
 */

static void ArkLoadCursor(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
{
	if (!xf86VTSema)
		return;

	if (!pCurs)
		return;

	/* Remember the cursor currently loaded into this cursor slot. */
	arkCursorpCurs = pCurs;

	ArkHideCursor();

	/* Program the cursor image address in video memory. */
	/* We use the last slot (the last 256 bytes of video memory). */
 	wrinx(0x3C4, 0x25, 63);

	ArkLoadCursorToCard(pScr, pCurs, x, y);

	ArkRecolorCursor(pScr, pCurs, 1);

	/* Position cursor */
	ArkMoveCursor(pScr, x, y);

	/* Turn it on. */
	ArkShowCursor();
}

/*
 * This function should display a new cursor at a new position.
 */

static void ArkSetCursor(pScr, pCurs, x, y, generateEvent)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
	Bool generateEvent;
{
	if (!pCurs)
		return;

	arkCursorHotX = pCurs->bits->xhot;
	arkCursorHotY = pCurs->bits->yhot;

	ArkLoadCursor(pScr, pCurs, x, y);
}

/*
 * This function should redisplay a cursor that has been
 * displayed earlier. It is called by the SVGA server.
 */

void ArkRestoreCursor(pScr)
	ScreenPtr pScr;
{
	int x, y;

	miPointerPosition(&x, &y);

	ArkLoadCursor(pScr, arkCursorpCurs, x, y);
}

/*
 * This function is called when the current cursor is moved. It makes
 * the graphic chip display the cursor at the new position.
 */

static void ArkMoveCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	int xorigin, yorigin;

	if (!xf86VTSema)
		return;

	x -= vga256InfoRec.frameX0 + arkCursorHotX;
	y -= vga256InfoRec.frameY0 + arkCursorHotY;

	/*
	 * If the cursor is partly out of screen at the left or top,
	 * we need set the origin.
	 */
	xorigin = 0;
	yorigin = 0;
	if (x < 0) {
		xorigin = -x;
		x = 0;
	}
	if (y < 0) {
		yorigin = -y;
		y = 0;
	}

	if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
		y *= 2;

	/* Program the cursor origin (offset into the cursor bitmap). */
	wrinx(0x3C4, 0x2C, xorigin);
	wrinx(0x3C4, 0x2D, yorigin);

	/* Program the new cursor position. */
	wrinx(0x3C4, 0x22, x);		/* Low byte. */
	wrinx(0x3C4, 0x21, x >> 8);	/* High byte. */
	wrinx(0x3C4, 0x24, y);		/* Low byte. */
	wrinx(0x3C4, 0x23, y >> 8);	/* High byte. */
}

/*
 * This is a local function that programs the colors of the cursor
 * on the graphics chip.
 * Adapted from accel/s3/s3Cursor.c.
 */

static void ArkRecolorCursor(pScr, pCurs, displayed)
	ScreenPtr pScr;
	CursorPtr pCurs;
	Bool displayed;
{
 	ColormapPtr pmap;
	unsigned short packedcolfg, packedcolbg;
	xColorItem sourceColor, maskColor;

	if (!xf86VTSema)
		return;

	switch (vgaBitsPerPixel) {
	case 8:
#if 0	/*
	 * The accel servers replace cfb colormap functions entirely
	 * and include GetInstalledColormaps. How can we avoid that?
	 *
	 * Until GetInstalledColormaps is also added to
	 * vga256/vga/vgacmap.c, disable hw cursor at 8bpp.
	 */
		/* Was s3GetInstalledColormaps. */
		cfbListInstalledColormaps(pScr, &pmap);
		sourceColor.red = pCurs->foreRed;
		sourceColor.green = pCurs->foreGreen;
		sourceColor.blue = pCurs->foreBlue;
		FakeAllocColor(pmap, &sourceColor);
		maskColor.red = pCurs->backRed;
		maskColor.green = pCurs->backGreen;
		maskColor.blue = pCurs->backBlue;
		FakeAllocColor(pmap, &maskColor);
		FakeFreeColor(pmap, sourceColor.pixel);
		FakeFreeColor(pmap, maskColor.pixel);

		wrinx(0x3C4, 0x26, sourceColor.pixel);
		wrinx(0x3C4, 0x29, maskColor.pixel);
#else
		wrinx(0x3C4, 0x26, 0);	/* XXXX Fixed colors, debugging only. */
		wrinx(0x3C4, 0x29, 1);
#endif
		break;
	case 16:
		if (xf86weight.green == 5) {
			packedcolfg = ((pCurs->foreRed & 0xf800) >> 1) 
				| ((pCurs->foreGreen & 0xf800) >> 6)
				| ((pCurs->foreBlue & 0xf800) >> 11);
			packedcolbg = ((pCurs->backRed & 0xf800) >> 1) 
				| ((pCurs->backGreen & 0xf800) >> 6)
				| ((pCurs->backBlue & 0xf800) >> 11);
		} else {
			packedcolfg = ((pCurs->foreRed & 0xf800) >> 0) 
				| ((pCurs->foreGreen & 0xfc00) >> 5)
				| ((pCurs->foreBlue & 0xf800) >> 11);
			packedcolbg = ((pCurs->backRed & 0xf800) >> 0) 
				| ((pCurs->backGreen & 0xfc00) >> 5)
				| ((pCurs->backBlue  & 0xf800) >> 11);
		}

		wrinx(0x3C4, 0x26, packedcolfg);	/* Low byte. */
		wrinx(0x3C4, 0x27, packedcolfg >> 8);	/* High byte. */
		wrinx(0x3C4, 0x29, packedcolbg);	/* Low byte. */
		wrinx(0x3C4, 0x2A, packedcolbg >> 8);	/* High byte. */
		break;
	case 32:
		wrinx(0x3C4, 0x26, pCurs->foreBlue >> 8);	/* Byte 0. */
		wrinx(0x3C4, 0x27, pCurs->foreGreen >> 8);	/* Byte 1. */
		wrinx(0x3C4, 0x28, pCurs->foreRed >> 8);	/* Byte 2. */
		wrinx(0x3C4, 0x29, pCurs->backBlue >> 8);	/* Byte 0. */
		wrinx(0x3C4, 0x2A, pCurs->backGreen >> 8);	/* Byte 1. */
		wrinx(0x3C4, 0x2B, pCurs->backRed >> 8);	/* Byte 2. */
		break;
	}
}

/*
 * This doesn't do very much. It just calls the mi routine. It is called
 * by the SVGA server.
 */

void ArkWarpCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	miPointerWarpCursor(pScr, x, y);
	xf86Info.currentScreen = pScr;
}

/*
 * This function is called by the SVGA server. It returns the
 * size of the hardware cursor that we support when asked for.
 * It is called by the SVGA server.
 */

void ArkQueryBestSize(class, pwidth, pheight)
	int class;
	short *pwidth;
	short *pheight;
{
 	if (*pwidth > 0) {
 		if (class == CursorShape) {
			*pwidth = arkCursorWidth;
			*pheight = arkCursorHeight;
		}
		else
			(void) mfbQueryBestSize(class, pwidth, pheight);
	}
}


