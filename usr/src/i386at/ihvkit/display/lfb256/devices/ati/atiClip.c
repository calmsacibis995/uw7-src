#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiClip.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <ati.h>

/*
 * The following variables save the clip rectangle. The ATI cannot
 * clip a region (union of rectangles), nor can it turn clipping off.
 * To turn clipping off, we set the clip rectangle to the maximum.  To
 * turn clipping on, we set the clip area to the previously stored
 * values.
 *
 * The SI specifically permits using one clip routine (and therefore
 * one clip area) for all clipping.  We don't need to save different
 * clip rectangles for the various types of drawing primitives.
 *
 */

static int clipLeft, clipRight, clipTop, clipBottom;


void atiClipOn()

{
    ATI_NEED_FIFO(4);

    outw(EXT_SCISSOR_L, clipLeft);
    outw(EXT_SCISSOR_R, clipRight);
    outw(EXT_SCISSOR_T, clipTop);
    outw(EXT_SCISSOR_B, clipBottom);

    atiClipping = 1;
}

void atiClipOff()

{
    ATI_NEED_FIFO(4);

    outw(EXT_SCISSOR_L, -2048 & 0x0fff);
    outw(EXT_SCISSOR_R, 2047);
    outw(EXT_SCISSOR_T, -2048 & 0x0fff);
    outw(EXT_SCISSOR_B, 2047);

    atiClipping = 0;
}

SIvoid atiSetClipRect(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
    if (x1 < x2) {
	clipLeft = x1;
	clipRight = x2;
    }
    else {
	clipLeft = x2;
	clipRight = x1;
    }

    if (y1 < y2) {
	clipTop = y1;
	clipBottom = y2;
    }
    else {
	clipTop = y2;
	clipBottom = y1;
    }

    clipLeft = max(clipLeft, -1) & 0x0fff;
    clipRight = min(clipRight, 2047) & 0x0fff;
    clipTop = max(clipTop, -1) & 0x0fff;
    clipBottom = min(clipBottom, 2047) & 0x0fff;

    if (atiClipping)
	atiClipOn();
}
