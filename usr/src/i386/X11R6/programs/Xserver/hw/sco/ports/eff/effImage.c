/*
 *	@(#) effImage.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History
 *
 * S009, 01-Feb-93, staceyc
 * 	added check for buggy C&T 481 8514/A clone
 * S008, 09-Jan-93, mikep
 *	remove check for bogus stride
 * S007, 12-Dec-92, mikep
 *	remove parameter checks.  They are now done in NFB.
 * S006, 28-Aug-91, staceyc
 * 	code cleanup - fixed bug in read image - reworked command queue use
 * S005, 13-Aug-91, staceyc
 * 	add effdefs.h to include list
 * S004, 23-Jul-91, staceyc
 * 	added check that stride be >= width - remove for production
 * S003, 25-Jun-91, staceyc
 * 	optimized read image
 * S002, 24-Jun-91, staceyc
 * 	fixed parameter decs - removed unused code
 * S001, 20-Jun-91, staceyc
 * 	readimage rewrite - fix problems with inw and 16 bit data
 * S000, 17-Jun-91, staceyc
 * 	first attempt
 */

#include <stdio.h>
#include "X.h"
#include "window.h"
#include "misc.h"
#include "miscstruct.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

static void effLoadUnequalStrideImage(
	unsigned char *ximage,
	unsigned int stride,
	int width,
	int height,
	int unaligned);

extern int effRasterOps[];

void effDrawImage(
BoxPtr pbox,			      /* rectangle coordinates on the screen */
unsigned char *ximage,		        /* pointer to data to send to screen */
unsigned int stride,/* # of bytes from the start of one scanline to the next */
unsigned char alu,                                               /* RasterOp */
unsigned long planemask,                                 /* 32 bit PlaneMask */
DrawablePtr pDraw)                         /* in case you need anything else */
{
	int height, width;
	WindowPtr pWin = (WindowPtr)pDraw;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	/* width and height in pixels */
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

	EFF_CLEAR_QUEUE(5);
	EFF_PLNWENBL(planemask);
	EFF_PLNRENBL(EFF_RPLANES);
	EFF_SETMODE(EFF_M_ONES);
	EFF_SETFN1(EFF_FNVAR, effRasterOps[alu]);
	EFF_SETX0(pbox->x1);

	EFF_CLEAR_QUEUE(4);
	EFF_SETY0(pbox->y1);
	EFF_SETLX(width - 1);
	EFF_SETLY(height - 1);

	EFF_COMMAND(EFF_WRITE_Z_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	/* NOTE: we write half as many shorts as bytes */
	if (width == stride)
		effBlockOutW(ximage, ((width * height) + 1) >> 1);
	else
		if (width & 1)                                 /* MOD 2 == 1 */
		{
			int extrah;
			unsigned char tchar;
			int almostw;

			extrah = height & 1;
			height >>= 1;               /* write half line pairs */
			if (almostw = --width >> 1)
			{
				while (height--)    /* write half line pairs */
				{
					effBlockOutW(ximage, almostw);
					tchar = ximage[width];
					ximage += stride;
					EFF_CLEAR_QUEUE(1);
					EFF_OUT(EFF_VARDATA,
					    tchar | ((*ximage) << 8));
					effBlockOutW(ximage + 1, almostw);
					ximage += stride;
				}
				if (extrah)
					effBlockOutW(ximage, almostw + 1);
			}
			else
			{
				while (height--)    /* write half line pairs */
				{
					tchar = ximage[width];
					ximage += stride;
					EFF_CLEAR_QUEUE(1);
					EFF_OUT(EFF_VARDATA,
					    tchar | ((*ximage) << 8));
					ximage += stride;
				}
				if (extrah)
				{
					EFF_CLEAR_QUEUE(1);
					EFF_OUT(EFF_VARDATA, *ximage);
				}
			}
		}
		else
			for (width >>= 1; height--; ximage += stride)
				effBlockOutW(ximage, width);
}

void
effReadImage(
BoxPtr pbox,                          /* rectangle specifying area of screen */
unsigned char *ximage,                                 /* area to place data */
unsigned int stride,            /* bytes from start of one scan line to next */
DrawablePtr pDraw)           /* handle to everything else that may be needed */
{
	int width, height, point, odd_width;
	unsigned short image_word;
	WindowPtr pWin = (WindowPtr)pDraw;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;
	point = height + width == 2;
	odd_width = width & 1;

	EFF_CLEAR_QUEUE(5);
	EFF_PLNWENBL(EFF_ALLPLANES);
	EFF_PLNRENBL(EFF_ALLPLANES);
	EFF_SETMODE(EFF_M_DEPTH);
	EFF_SETFN1(EFF_FNVAR, EFF_FNREPLACE);
	EFF_SETX0(pbox->x1);

	EFF_CLEAR_QUEUE(4);
	EFF_SETY0(pbox->y1);
	EFF_SETLX(width - 1 + odd_width);
	EFF_SETLY(height - 1);

	EFF_COMMAND(EFF_READ_Z_DATA);

	if (effPriv->f82c481)
		EFF_CLEAR_QUEUE(8);

	/*
	 * we do all input using 16BIT == 1, in this case an odd width
	 * means we have to discard the upper byte of the image_word,
	 * see the C&T manual, in the PIX_TRANS page under "WARNING"
	 */

	if (point)                                           /* single pixel */
	{
		image_word = EFF_INW(EFF_VARDATA);
		*ximage = image_word & 0xFF;
	}
	else
		if (stride == width && ! odd_width)
			effBlockInW(ximage, (width * height) / 2);
		else
			effLoadUnequalStrideImage(ximage, stride, width,
				    height, odd_width);

}

static void effLoadUnequalStrideImage(
unsigned char *ximage,
unsigned int stride,
int width,
int height,
int odd_width)
{
	unsigned int y, last_byte, width16;
	unsigned char *x_p = ximage;
	unsigned short image_word;

	width16 = width >> 1;
	if (! odd_width)
		for (y = 0; y < height; ++y, x_p += stride)
			effBlockInW(x_p, width16);
	else
	{
		last_byte = width - 1;
		for (y = 0; y < height; ++y, x_p += stride)
		{
			effBlockInW(x_p, width16);
			image_word = EFF_INW(EFF_VARDATA);
			x_p[last_byte] = image_word & 0xFF;
		}
	}
}

