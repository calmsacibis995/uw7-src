/*
 *	@(#) effMono.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Modification History
 *
 * S009, 27-Aug-93, buckm
 *	don't enlarge the current clip box when clipping mono bits.
 * S008, 01-Feb-93, staceyc
 * 	added check for buggy C&T 481 8514/A clone
 * S007, 12-Dec-92, mikep
 *	remove parmeter checks, they are now done in nfb
 * S006, 26-Oct-92, staceyc
 * 	removed all the simplified mono image code, it was ugly and had
 *	a bug
 * S005, 20-Sep-91, staceyc
 * 	backed out nugget size code - Matrox 1280x1024 doesn't need it
 *	added slow AP drawing mode for broken Matrox in 1280
 * S004, 09-Sep-91, staceyc
 * 	variable nugget size
 * S003, 05-Sep-91, staceyc
 * 	deal with special case of mono image being wider than off-screen mem
 * S002, 04-Sep-91, staceyc
 * 	use configurable value for x hardware clipping
 * S001, 28-Aug-91, staceyc
 * 	reworked command queue use - general cleanup
 * S000, 19-Aug-91, staceyc
 * 	created from code in effRectOps.c
 */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern int effRasterOps[];

static void
effFixedOpaqueDrawScanlines(
    BoxPtr pbox,
    unsigned char **image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned long bg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	int width, i;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);
	void (*APBlockOutW)() = effPriv->APBlockOutW;

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	width = (lx + 7) >> 3;

	/*
	 * this routine assumes that its parameters are in a format the
	 * 8514 can understand, i.e. stride == width, startx == 0, and
	 * (pbox->x1 % nugget_size) == 0
	 */
	EFF_CLEAR_QUEUE(8);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_VAR);
	EFF_SETCOL0(bg);
	EFF_SETCOL1(fg);
	EFF_SETFN0(EFF_FNCOLOR0, effRasterOps[alu]);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETX0(pbox->x1);

	EFF_CLEAR_QUEUE(5);
	EFF_SETY0(pbox->y1);
	/*
	 * measured in BITS - clip will discard excess
	 */
    	EFF_SETLX((width << 3) - 1);
	EFF_SETLY(ly - 1);
	if (pbox->x2 < effPriv->current_clip.x2)
		EFF_SETXMAX(pbox->x2 - 1);
	EFF_COMMAND(EFF_WRITE_X_Y_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	for (i = 0; i < ly; ++i)
		(*APBlockOutW)(image[i], width);

	if (pbox->x2 < effPriv->current_clip.x2)
	{
		EFF_CLEAR_QUEUE(1);
		EFF_SETXMAX(effPriv->current_clip.x2 - 1);
	}
}

static void
effDrawOpaqueMonoScanlineBlit(
BoxRec *pbox,
unsigned char **image,
int stride,
int extra_bits,
unsigned char alu,
unsigned long fg,
unsigned long bg,
unsigned long planemask,
DrawablePtr pDraw)
{
	int lx, ly, x, y;
	effOffScreenBlitArea_t *dest;
	int chunks, i, y_offset, rows_left;
	BoxRec off_screen_box, screen_box;
	DDXPointRec src, off_screen_point;

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	x = pbox->x1;
	y = pbox->y1;
	dest = &(EFF_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	i = 0;
	chunks = (ly + dest->h - 1) / dest->h;
	src.x = x;
	off_screen_box.y1 = dest->y;
	off_screen_box.x1 = dest->x;
	off_screen_box.x2 = dest->x + (stride << 3);
	rows_left = ly;
	while (i < chunks)
	{
		if (rows_left - dest->h < 0)
		{
			src.y = y + ly - rows_left;
			off_screen_box.y2 = dest->y + rows_left;
			screen_box.y2 = y + ly;
			y_offset = rows_left;
		}
		else
		{
			src.y = y + i * dest->h;
			off_screen_box.y2 = dest->y + dest->h;
			screen_box.y2 = src.y + dest->h;
			y_offset = dest->h;
		}
		/*
		 * scribble image onto off screen plane
		 */
		effFixedOpaqueDrawScanlines(&off_screen_box, image, 0,
		    stride, ~0, 0, GXcopy, EFF_GENERAL_OS_WRITE, pDraw);
		/*
		 * opaque stretch blit to screen
		 */
		screen_box.x1 = src.x;
		screen_box.y1 = src.y;
		screen_box.x2 = src.x + lx;
		off_screen_point.x = dest->x + extra_bits;
		off_screen_point.y = dest->y;
		effSetupCardForOpaqueBlit(fg, bg, alu, EFF_GENERAL_OS_READ,
		    planemask);
		effStretchBlit(pDraw, &off_screen_point, &screen_box);
		effResetCardFromBlit();

		++i;
		rows_left -= dest->h;
		image += y_offset;
	}
}

static void
effDrawOpaqueMonoScanlines(
BoxPtr pbox,
unsigned char *image,
unsigned int startx,
unsigned int stride,
unsigned long fg,
unsigned long bg,
unsigned char alu,
unsigned long planemask,
DrawablePtr pDraw)
{
	int ly, lx, i, start_byte, new_stride, extra_bits;
	unsigned char **new_image, *image_p;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;
	if ((lx <= 0) || (ly <= 0))
		return;

	new_image = (unsigned char **)ALLOCATE_LOCAL(ly *
	    sizeof(unsigned char *));
	start_byte = startx / 8;
	extra_bits = startx % 8;
	for (image_p = image, i = 0; i < ly; ++i, image_p += stride)
		new_image[i] = &image_p[start_byte];
	new_stride = (lx + extra_bits + 7) >> 3;
	if (extra_bits == 0 && pbox->x1 % 4 == 0)
		effFixedOpaqueDrawScanlines(pbox, new_image, 0, new_stride,
		    fg, bg, alu, planemask, pDraw);
	else
		effDrawOpaqueMonoScanlineBlit(pbox, new_image, new_stride,
		    extra_bits, alu, fg, bg, planemask, pDraw);
	DEALLOCATE_LOCAL((char *)new_image);
}

static void
effFixedOpaqueDraw(
    BoxPtr pbox,
    unsigned char *image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned long bg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	int width;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	width = (lx + 7) >> 3;

	/*
	 * this routine assumes that its parameters are in a format the
	 * 8514 can understand, i.e. stride == width, startx == 0, and
	 * (pbox->x1 % nugget_size) == 0
	 */
	EFF_CLEAR_QUEUE(8);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_VAR);
	EFF_SETCOL0(bg);
	EFF_SETCOL1(fg);
	EFF_SETFN0(EFF_FNCOLOR0, effRasterOps[alu]);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETX0(pbox->x1);

	EFF_CLEAR_QUEUE(5);
	EFF_SETY0(pbox->y1);
	/*
	 * measured in BITS - clip will discard excess
	 */
    	EFF_SETLX((width << 3) - 1);
	EFF_SETLY(ly - 1);
	if (pbox->x2 < effPriv->current_clip.x2)
		EFF_SETXMAX(pbox->x2 - 1);
	EFF_COMMAND(EFF_WRITE_X_Y_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	(*effPriv->APBlockOutW)(image, width * ly);

	if (pbox->x2 < effPriv->current_clip.x2)
	{
		EFF_CLEAR_QUEUE(1);
		EFF_SETXMAX(effPriv->current_clip.x2 - 1);
	}
}

static void
effDrawMonoScanlineBlit(
BoxRec *pbox,
unsigned char **image,
int stride,
int extra_bits,
unsigned char alu,
unsigned long fg,
unsigned long planemask,
DrawablePtr pDraw)
{
	int lx, ly, x, y;
	effOffScreenBlitArea_t *dest;
	int chunks, i, y_offset, rows_left;
	BoxRec off_screen_box, screen_box;
	DDXPointRec src, off_screen_point;

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	x = pbox->x1;
	y = pbox->y1;
	dest = &(EFF_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	i = 0;
	chunks = (ly + dest->h - 1) / dest->h;
	src.x = x;
	off_screen_box.y1 = dest->y;
	off_screen_box.x1 = dest->x;
	off_screen_box.x2 = dest->x + (stride << 3);
	rows_left = ly;
	while (i < chunks)
	{
		if (rows_left - dest->h < 0)
		{
			src.y = y + ly - rows_left;
			off_screen_box.y2 = dest->y + rows_left;
			screen_box.y2 = y + ly;
			y_offset = rows_left;
		}
		else
		{
			src.y = y + i * dest->h;
			off_screen_box.y2 = dest->y + dest->h;
			screen_box.y2 = src.y + dest->h;
			y_offset = dest->h;
		}
		/*
		 * scribble image onto off screen plane
		 */
		effFixedOpaqueDrawScanlines(&off_screen_box, image, 0,
		    stride, ~0, 0, GXcopy, EFF_GENERAL_OS_WRITE, pDraw);
		/*
		 * transparent stretch blit to screen
		 */
		screen_box.x1 = src.x;
		screen_box.y1 = src.y;
		screen_box.x2 = src.x + lx;
		off_screen_point.x = dest->x + extra_bits;
		off_screen_point.y = dest->y;
		effSetupCardForBlit(fg, alu, EFF_GENERAL_OS_READ,
		    planemask);
		effStretchBlit(pDraw, &off_screen_point, &screen_box);
		effResetCardFromBlit();

		++i;
		rows_left -= dest->h;
		image += y_offset;
	}
}

static void
effFixedDrawScanlines(
    BoxPtr pbox,
    unsigned char **image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	int width, i;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);
	void (*APBlockOutW)() = effPriv->APBlockOutW;

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	width = (lx + 7) >> 3;

	/*
	 * this routine assumes that its parameters are in a format the
	 * 8514 can understand, i.e. stride == width, startx == 0, and
	 * (pbox->x1 % nugget_size) == 0
	 */
	EFF_CLEAR_QUEUE(6);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_VAR);
	EFF_SETCOL1(fg);
	EFF_SETFN0(EFF_FNCOLOR0, effRasterOps[GXnoop]);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);

	EFF_CLEAR_QUEUE(6);
	EFF_SETX0(pbox->x1);
	EFF_SETY0(pbox->y1);
	/*
	 * measured in BITS - clip will discard excess
	 */
    	EFF_SETLX((width << 3) - 1);
	EFF_SETLY(ly - 1);
	if (pbox->x2 < effPriv->current_clip.x2)
		EFF_SETXMAX(pbox->x2 - 1);
	EFF_COMMAND(EFF_WRITE_X_Y_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	for (i = 0; i < ly; ++i)
		(*APBlockOutW)(image[i], width);

	if (pbox->x2 < effPriv->current_clip.x2)
	{
		EFF_CLEAR_QUEUE(1);
		EFF_SETXMAX(effPriv->current_clip.x2 - 1);
	}
}

static void
effDrawMonoScanlines(
BoxPtr pbox,
unsigned char *image,
unsigned int startx,
unsigned int stride,
unsigned long fg,
unsigned char alu,
unsigned long planemask,
DrawablePtr pDraw)
{
	int ly, lx, i, start_byte, new_stride, extra_bits;
	unsigned char **new_image, *image_p;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	new_image = (unsigned char **)ALLOCATE_LOCAL(ly *
	    sizeof(unsigned char *));
	start_byte = startx / 8;
	extra_bits = startx % 8;
	for (image_p = image, i = 0; i < ly; ++i, image_p += stride)
		new_image[i] = &image_p[start_byte];
	new_stride = (lx + extra_bits + 7) >> 3;
	if (extra_bits == 0 && pbox->x1 % 4 == 0)
		effFixedDrawScanlines(pbox, new_image, 0, new_stride, fg, alu,
		    planemask, pDraw);
	else
		effDrawMonoScanlineBlit(pbox, new_image, new_stride, extra_bits,
		    alu, fg, planemask, pDraw);
	DEALLOCATE_LOCAL((char *)new_image);
}

static void
effFixedDraw(
    BoxPtr pbox,
    unsigned char *image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	int width;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	width = (lx + 7) >> 3;

	/*
	 * this routine assumes that its parameters are in a format the
	 * 8514 can understand, i.e. stride == width, startx == 0, and
	 * (pbox->x1 % nugget_size) == 0
	 */
	EFF_CLEAR_QUEUE(6);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_VAR);
	EFF_SETCOL1(fg);
	EFF_SETFN0(EFF_FNCOLOR0, effRasterOps[GXnoop]);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);

	EFF_CLEAR_QUEUE(6);
	EFF_SETX0(pbox->x1);
	EFF_SETY0(pbox->y1);
	/*
	 * measured in BITS - clip will discard excess
	 */
    	EFF_SETLX((width << 3) - 1);
	EFF_SETLY(ly - 1);
	if (pbox->x2 < effPriv->current_clip.x2)
		EFF_SETXMAX(pbox->x2 - 1);
	EFF_COMMAND(EFF_WRITE_X_Y_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	(*effPriv->APBlockOutW)(image, width * ly);

	if (pbox->x2 < effPriv->current_clip.x2)
	{
		EFF_CLEAR_QUEUE(1);
		EFF_SETXMAX(effPriv->current_clip.x2 - 1);
	}
}

void
effDrawMonoImage(
    BoxPtr pbox,
    unsigned char *image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	unsigned char *new_image;
	int width;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	if (lx > effPriv->all_off_screen.w)
	{
		genDrawMonoImage(pbox, image, startx, stride, fg, alu,
		    planemask, pDraw);
		return;
	}

	width = (lx + 7) >> 3;

	if (pbox->x1 % 4 == 0 && stride == width &&
	    startx == 0)
	{
		effFixedDraw(pbox, image, startx, stride, fg, alu, planemask,
		    pDraw);
		return;
	}
	effDrawMonoScanlines(pbox, image, startx, stride, fg, alu, planemask,
	    pDraw);
}

void
effDrawOpaqueMonoImage(
    BoxPtr pbox,
    unsigned char *image,
    unsigned int startx,
    unsigned int stride,
    unsigned long fg,
    unsigned long bg,
    unsigned char alu,
    unsigned long planemask,
    DrawablePtr pDraw)
{
	unsigned char *new_image;
	int width;
	int lx, ly;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	lx = pbox->x2 - pbox->x1;
	ly = pbox->y2 - pbox->y1;

	if (lx > effPriv->all_off_screen.w)
	{
		genDrawOpaqueMonoImage(pbox, image, startx, stride, fg, bg, alu,
		    planemask, pDraw);
		return;
	}

	width = (lx + 7) >> 3;

	if (pbox->x1 % 4 == 0 && stride == width &&
	    startx == 0)
	{
		effFixedOpaqueDraw(pbox, image, startx, stride, fg, bg, alu,
		    planemask, pDraw);
		return;
	}
	effDrawOpaqueMonoScanlines(pbox, image, startx, stride, fg, bg, alu,
	    planemask, pDraw);
}

