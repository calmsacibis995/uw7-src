/*
 * @(#) mgaScrStr.h 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
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
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

#include "mgaDefs.h"

typedef struct _mgaFont
{
    int stride;
    int loc;
} mgaFont;

typedef struct _mgaPrivate {
    mgaRegsPtr mapBase;		/* base of mapped area */
    mgaRegsPtr regs;		/* pointer to mga space */
    unsigned char *fbBase;	/* base of 7k window */
    int pstride;		/* width in pixels of pixmap */
    int bstride;		/* width in bytes of pixmap */
    int width;			/* width in pixels of display */
    int height;			/* height in pixels of display */
    int depth;			/* depth in bits per pixel */
    int bpp;			/* depth in bytes per pixel */
    int ydstorg;		/* y destination origen */
    int offscreenSize;		/* amount in bytes of off screen memory */
    int clipXL;			/* clip x left */
    int clipXR;			/* clip x right */
    int clipYT;			/* clip y top */
    int clipYB;			/* clip y bottom */
    int cursorWidth;		/* cursor width */
    int cursorHeight;		/* cursor height */
    int cursorHotx;		/* cursor x hotspot */
    int cursorHoty;		/* cursor y hotspot */
    int isvlb;			/* flag for vesa local bus */
    unsigned long isabit;	/* flag for isa bit in config */
    unsigned long dactype;	/* which dac are we using */
    mgaFont font[2];		/* font stuff for 2 cached fonts */
    int vgaEnabled;		/* is vga enabled on card */
    vid vidtab[29];
} mgaPrivate, * mgaPrivatePtr;

extern int mgaScreenPrivateIndex;

#define MGA_PRIVATE_DATA(pScreen) \
((mgaPrivatePtr)((pScreen)->devPrivates[mgaScreenPrivateIndex].ptr))


