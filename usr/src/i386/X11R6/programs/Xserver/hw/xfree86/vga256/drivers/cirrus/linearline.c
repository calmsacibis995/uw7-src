/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/linearline.c,v 3.4 1995/06/02 11:19:49 dawes Exp $ */
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (hhanemaa@cs.ruu.nl).
 */
/* $XConsortium: linearline.c /main/4 1995/11/13 08:21:35 kaleb $ */

#include "cfb.h"
#include "cfbmskbits.h"


/*
 * Linear framebuffer sloped/vertical line drawing for the 386 architecture.
 * The functions are just small enough to prevent compilers messing up
 * too badly because of a lack of registers.
 *
 * There are functions for vertical lines, dual vertical lines (usable
 * for rectangles), and three for sloped lines (iterating over the x-axis
 * to the left, to the right, and one iterating over y).
 *
 * GXcopy only, can be compiled for 8, 16 and 32bpp. Use cfb for other
 * lines.
 *
 * Due to the nature of DRAM (and VRAM) memory used on video cards,
 * vertical-ish lines can generally not be drawn in less than 6 memory
 * cycles per pixel (a typical memory clock is 60 MHz). If the
 * framebuffer routine can achieve that rate of writes (assisted by
 * a fast CPU and a good writebuffer on the video card), it will be
 * no slower than an accelerated line drawing engine (not considering
 * accelerator/CPU concurrency).
 *
 * The six cycle timing is for a non-page mode DRAM write, that is
 * a write that doesn't fall in the same DRAM page as the previous
 * write. A typical DRAM page size is 512, which is smaller than the
 * scanline offset so writes to different scanlines cannot be page-mode
 * (page mode accesses usually take 2 cycles). There is a function
 * that performs a further optimization by drawing two vertical
 * edges in the same loop, scanline by scanline, in the hope of having
 * page-mode access for the second (right edge) pixel in each scanline.
 *
 */


#define WRITEPIXEL(address, value) \
	*(PixelType *)(address) = value;


void LinearFramebufferVerticalLine(destp, fg, lineheight, destpitch)
    unsigned char *destp;
    int fg;
    int lineheight;
    int destpitch;
{
    while (lineheight >= 10) {

#define DOPIXELVERTICAL(destp, i, pix, destpitch) \
	WRITEPIXEL((destp + i * destpitch), pix);

	/* Take advantage of *2/4/8 scaled addressing on the 386 arch. */
	DOPIXELVERTICAL(destp, 0, fg, destpitch);	/* 0 */
	DOPIXELVERTICAL(destp, 1, fg, destpitch);	/* 1 */
	DOPIXELVERTICAL(destp, 2, fg, destpitch);	/* 2 */
	DOPIXELVERTICAL(destp, 4, fg, destpitch);	/* 4 */
	DOPIXELVERTICAL(destp, 8, fg, destpitch);	/* 8 */
	destp += destpitch;
	DOPIXELVERTICAL(destp, 2, fg, destpitch);	/* 3 */
	DOPIXELVERTICAL(destp, 4, fg, destpitch);	/* 5 */
	DOPIXELVERTICAL(destp, 8, fg, destpitch);	/* 9 */
	destp += destpitch;
	DOPIXELVERTICAL(destp, 4, fg, destpitch);	/* 6 */
	DOPIXELVERTICAL(destp, 5, fg, destpitch);	/* 7 */
	destp += 8 * destpitch;
	lineheight -= 10;
    }
    while (lineheight > 0) {
	DOPIXELVERTICAL(destp, 0, fg, destpitch);
	destp += destpitch;
	lineheight--;
    }
}


void LinearFramebufferDualVerticalLine(destp, fg, lineheight, offset,
destpitch)
    unsigned char *destp;
    int fg;
    int lineheight;
    int offset;
    int destpitch;
{
    while (lineheight >= 10) {

#define DOPIXELDUALVERTICAL(destp, offset, pix, destpitch) \
	WRITEPIXEL(destp, pix); \
	WRITEPIXEL((destp + offset), pix); \
	destp += destpitch;

	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	lineheight -= 10;
    }
    while (lineheight > 0) {
	DOPIXELDUALVERTICAL(destp, offset, fg, destpitch);
	lineheight--;
    }
}


/*
 * To the right, -1 < slope < 1. Iterate over x-axis.
 */

void LinearFramebufferSlopedLineRight(destp, fg, e, e1, e2, length,
destpitch)
    unsigned char *destp;
    int fg, e1, e2, length, destpitch;
{
    int e3, pix;
    e3 = e2 - e1;
    e = e - e1;
    /*
     * gcc 2.5.8 is too stupid to eliminate a load in the loop
     * despite a free register.
     * This line tends to make it not affect fg, which has the
     * highest cost as a load (loaded in every iteration).
     */
    pix = fg;
    while (length >= 10) {

#define DOPIXELXAXIS(destp, i, pix, e, e1, e3, destpitch) \
	WRITEPIXEL((destp + i * (PSZ / 8)), pix); \
	if ((e += e1) >= 0) { \
	    destp += destpitch; \
	    e += e3; \
	}

	DOPIXELXAXIS(destp, 0, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 1, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 2, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 3, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 4, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 5, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 6, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 7, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 8, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, 9, pix, e, e1, e3, destpitch);
	destp += (PSZ / 8) * 10;
	length -= 10;
    }
    while (length > 0) {
	DOPIXELXAXIS(destp, 0, pix, e, e1, e3, destpitch);
	destp += (PSZ / 8);
	length--;
    }
}


/*
 * To the left, -1 < slope < 1. Iterate over x-axis.
 */

void LinearFramebufferSlopedLineLeft(destp, fg, e, e1, e2, length,
destpitch)
    unsigned char *destp;
    int fg, e1, e2, length, destpitch;
{
    int e3, pix;
    e3 = e2 - e1;
    e = e - e1;
    /*
     * gcc 2.5.8 is too stupid to eliminate a load in the loop
     * despite a free register.
     * This line tends to make it not affect fg, which has the
     * highest cost as a load (loaded in every iteration).
     */
    pix = fg;
    while (length >= 10) {
	DOPIXELXAXIS(destp, 0, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -1, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -2, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -3, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -4, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -5, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -6, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -7, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -8, pix, e, e1, e3, destpitch);
	DOPIXELXAXIS(destp, -9, pix, e, e1, e3, destpitch);
	destp -= (PSZ / 8) * 10;
	length -= 10;
    }
    while (length > 0) {
	DOPIXELXAXIS(destp, 0, pix, e, e1, e3, destpitch);
	destp -= (PSZ / 8);
	length--;
    }
}


/*
 * Down/Up and right, slope > 1 and slope < -1. Iterate over y-axis.
 */

void LinearFramebufferSlopedLineVerticalRight(destp, fg, e, e1, e2, length,
destpitch)
    unsigned char *destp;
    int fg, e1, e2, length, destpitch;
{
    int e3, pix;
    e3 = e2 - e1;
    e = e - e1;
    /*
     * gcc 2.5.8 is too stupid to eliminate a load in the loop
     * despite a free register.
     * This line tends to make it not affect fg, which has the
     * highest cost as a load (loaded in every iteration).
     */
    pix = fg;
    while (length >= 10) {

#define DOPIXELYAXIS(destp, i, pix, e, e1, e3, destpitch, bpsz) \
	WRITEPIXEL((destp + destpitch * i), pix); \
	if ((e += e1) >= 0) { \
	    destp += bpsz; \
	    e += e3; \
	}

	DOPIXELYAXIS(destp, 0, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 1, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 2, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 3, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 4, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 5, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 6, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 7, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 8, pix, e, e1, e3, destpitch, sizeof(PixelType));
	DOPIXELYAXIS(destp, 9, pix, e, e1, e3, destpitch, sizeof(PixelType));
	destp += destpitch * 10;
	length -= 10;
    }
    while (length > 0) {
	DOPIXELYAXIS(destp, 0, pix, e, e1, e3, destpitch, sizeof(PixelType));
	destp += destpitch;
	length--;
    }
}


/*
 * Down/Up and left, slope > 1 and slope < -1. Iterate over y-axis.
 */

void LinearFramebufferSlopedLineVerticalLeft(destp, fg, e, e1, e2, length,
destpitch)
    unsigned char *destp;
    int fg, e1, e2, length, destpitch;
{
    int e3, pix;
    e3 = e2 - e1;
    e = e - e1;
    /*
     * gcc 2.5.8 is too stupid to eliminate a load in the loop
     * using a free register.
     * This line tends to make it not affect fg, which has the
     * highest cost as a load (loaded in every iteration).
     */
    pix = fg;
    while (length >= 10) {
	DOPIXELYAXIS(destp, 0, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 1, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 2, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 3, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 4, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 5, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 6, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 7, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 8, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	DOPIXELYAXIS(destp, 9, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	destp += destpitch * 10;
	length -= 10;
    }
    while (length > 0) {
	DOPIXELYAXIS(destp, 0, pix, e, e1, e3, destpitch, -sizeof(PixelType));
	destp += destpitch;
	length--;
    }
}
