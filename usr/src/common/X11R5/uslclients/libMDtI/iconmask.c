#ifndef NOIDENT
#ident	"@(#)libMDtI:iconmask.c	1.8"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include "DesktopP.h"

#define STACK_SIZE_INC	64

typedef struct {
	int x;
	int y;
} point;

#define MAKE_TRANSP(X,Y)	(mask[(Y) * width + X] && \
			 	 (XGetPixel(image, (X), (Y)) == transparent))

/************************************************************************
 *	Define Pixmap type (taken from OpenLook.h)
 *	Note: On some systems, only one of the following types would 
 *	work properly, this is why this macro is introduced.  Later
 *	on this macro might not be supported.
 ************************************************************************
 */
#ifdef USE_XYPIXMAP
#define PixmapType      XYPixmap
#else
#define PixmapType      ZPixmap
#endif


static void
fill(mask, image, width, height, x, y, transparent)
char *mask;
XImage *image;
int width;
int height;
int x;
int y;
int transparent;
{
 	if (x >= 0 && x < width &&
            y >= 0 && y < height &&
	    MAKE_TRANSP(x, y)) {
		mask[y * width + x] = 0;
		fill(mask, image, width, height, x - 1, y, transparent);
		fill(mask, image, width, height, x + 1, y, transparent);
		fill(mask, image, width, height, x, y - 1, transparent);
		fill(mask, image, width, height, x, y + 1, transparent);
	}
}

static char *
xlate_to_bitmap(mask, width, height)
char *mask;
int width;
int height;
{
	char *data;
	char *p;
	int x;
	int y;
	int c;		/* cumulated bitvalues */
	int b;		/* bitmask */

	if ((data = (char *)malloc((width + 7)/8 * height)) == NULL)
		return(NULL);

	p = data;
	c = 0;
	b = 1;
	for (y=0; y < height; y++) {
		for (x=0; x < width;) {
			if (mask[y * width + x])
				c |= b;
			b <<= 1;
			if (!(++x & 7)) {
				*p++ = c;
				c = 0;
				b = 1;
			}
		}
		if (x & 7) {
			*p++ = c;
			c = 0;
			b = 1;
		}
	}

	return(data);
}

/* PERF: (Performance improvement work, 5/92)                             */
/*       These macros (ALLOCATE_MAYBE and FREE_MAYBE) are substituted for */
/*       inoperative ALLOCATE_LOCAL and DEALLOCATE_LOCAL when a probably  */
/*       constant amount of storage is required a number of times.  That  */
/*       storage is set aside (Local...Buf) and used if it is sufficiently*/
/*       large for ALLOCATE_LOCAL demands.  If it isn't big enough, or if */
/*       it's already in use, a real ALLOC takes place.  FREE_MAYBE       */
/*       unwinds this situation.   [AS]                                   */

#define ALLOCATE_MAYBE(num,auto,inuse) \
                ((!inuse&&(num)<=sizeof(auto))?inuse++,(auto):(malloc(num)))
#define FREE_MAYBE(actual,auto,inuse) \
                {if ((actual)!=(auto)) free(actual); else inuse--;}

Pixmap
Dm__CreateIconMask(screen, gp)
Screen *screen;
DmGlyphPtr gp;
{
	int width  = gp->width;
	int height = gp->height;
	unsigned long transparent;
	XImage *image;
	register int x;
	register int y;
	char *mask;
	Pixmap ret;
	char *bitmap_data;
	XColor xcolor;
	char localBuf[2400]; char localBufInUse = 0; /* PERF */

	image = XGetImage(DisplayOfScreen(screen), gp->pix, 0, 0,
				width, height, AllPlanes, PixmapType);

	/*
	 * Assume the pixel at the upper left hand corner is a transparent
	 * pixel, because there is no reliable and portable of figuring
	 * out what is transparent.
	 */
	transparent = XGetPixel(image, 0, 0);

	/* if((mask=(char *)malloc(sizeof(char)*width*height))==NULL) /* PERF */
	if ((mask = (char *) ALLOCATE_MAYBE(sizeof(char) * width * height, 
				localBuf,localBufInUse)) == NULL) /* PERF */
		return(NULL);
	memset((char *)mask, 1, sizeof(char) * width * height);

	/* go through the left and right columns */
	for (y=0, x=width - 1; y < height; y++) {
		if (mask[y * width + 0] && XGetPixel(image, 0, y)==transparent)
			fill(mask, image, width, height, 0, y, transparent);
		if (mask[y * width + x] && XGetPixel(image, x, y)==transparent)
			fill(mask, image, width, height, x, y, transparent);
	}

	/* go through the top and bottom rows */
	for (x=1, y=height - 1; x < (width-1); x++) {
		if (mask[0 * width + x] && XGetPixel(image, x, 0)==transparent)
			fill(mask, image, width, height, x, 0, transparent);
		if (mask[y * width + x] && XGetPixel(image, x, y)==transparent)
			fill(mask, image, width, height, x, y, transparent);
	}

	/*print_mask(mask, width, height);*/
	/* convert mask to bitmap data */
	bitmap_data = xlate_to_bitmap(mask, width, height);

	gp->mask = XCreatePixmapFromBitmapData(DisplayOfScreen(screen),
			RootWindowOfScreen(screen),
			bitmap_data, width, height,
			1, 0, (unsigned int)1);

	free(bitmap_data);
	/* free(mask); 	/** PERF: see above */
	FREE_MAYBE(mask, localBuf, localBufInUse);
	return(gp->mask);
}

#ifdef DEBUG
static void
print_image(image, width, height)
XImage *image;
int width;
int height;
{
	int x,y;

	for (y=0; y < height; y++) {
		for (x=0; x < width; x++)
			printf("%2d", XGetPixel(image, x, y) != 0);
		printf("\n");
	}
	printf("\n");
}

static void
print_mask(mask, width, height)
char *mask;
int width;
int height;
{
	int x,y;

	for (y=0; y < height; y++) {
		for (x=0; x < width; x++)
			printf("%2d", (int)mask[y * width + x]);
		printf("\n");
	}
	printf("\n");
}
#endif

