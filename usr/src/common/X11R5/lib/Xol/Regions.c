#ifndef	NOIDENT
#ident	"@(#)olmisc:Regions.c	1.2"
#endif

#include "X11/Intrinsic.h"
#include "Xol/OpenLook.h"

/**
 ** OlRectInRegion()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlRectInRegion (
	Region			region,
	XRectangle *		src_rect,
	XRectangle *		dst_rect
)
#else
OlRectInRegion (region, src_rect, dst_rect)
	Region			region;
	XRectangle *		src_rect;
	XRectangle *		dst_rect;
#endif
{
	switch (XRectInRegion(
	    region, src_rect->x, src_rect->y, src_rect->width, src_rect->height
	)) {

	case RectangleIn:
		if (dst_rect)
			*dst_rect = *src_rect;
		return (True);

	case RectanglePart:
		if (dst_rect) {
			static Region		clip = 0;

			if (!clip)
				clip = XCreateRegion();
			OlIntersectRectWithRegion (src_rect, region, clip);
			XClipBox (clip, dst_rect);
		}
		return (True);

	case RectangleOut:
	default:
		return (False);
	}
} /* OlRectInRegion */

/**
 ** OlIntersectRectWithRegion()
 **/

void
#if	OlNeedFunctionPrototypes
OlIntersectRectWithRegion (
	XRectangle *		rectangle,
	Region			source,
	Region			destination
)
#else
OlIntersectRectWithRegion (rectangle, source, destination)
	XRectangle *		rectangle;
	Region			source;
	Region			destination;
#endif
{
	static Region		null = 0;
	static Region		scratch = 0;


	if (!null) {
		null = XCreateRegion();
		scratch = XCreateRegion();
	}

	XUnionRectWithRegion (rectangle, null, scratch);
	XIntersectRegion (scratch, source, destination);

	return;
} /* OlIntersectRectWithRegion */
