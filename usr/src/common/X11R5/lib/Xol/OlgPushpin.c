#ifndef NOIDENT
#ident	"@(#)olg:OlgPushpin.c	1.6"
#endif

/* Pushpin Functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Draw a pushpin.
 *
 * Draw a pushpin of the type indicated.  Valid types are:
 *
 *	PP_OUT
 *	PP_IN
 *	PP_DEFAULT
 *
 * The area under the pin is assumed to be cleared to the background.
 */

void
OlgDrawPushPin OLARGLIST((scr, win, pInfo, x, y, width, height, type))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Position,	x)
    OLARG( Position,	y)
    OLARG( Dimension,	width)
    OLARG( Dimension,	height)
    OLGRA( OlDefine,	type)
{
    _OlgDevice	*pDev = pInfo->pDev;
    Position	yOrig;
    Position	pinX, pinY;
    Dimension	pinWidth, pinHeight;
    GC		gc;

    if (OlgIs3d())
    {
	/* Load the bitmaps, if we haven't already. */
	if (!pDev->pushpin3D)
	    _OlgGetBitmaps (scr, pInfo, OLG_PUSHPIN_3D);

	/* 3-D pushpins are arranged as a 3x3 matrix.  The top row is the
	 * IN pins, the middle row is the OUT pins, and the bottom row is
	 * the DEFAULT pins.  Light highlights left, dark areas center,
	 * and fill areas right.
	 */
	pinWidth = pDev->widthPushpin3D / 3;
	pinHeight = pDev->heightPushpin3D / 3;

	switch (type) {
	case PP_IN:
	    yOrig = 0;
	    break;

	case PP_OUT:
	    yOrig = pinHeight;
	    break;

	case PP_DEFAULT:
	    yOrig = pinHeight * 2;
	    break;
	}

	/* Center the pin and clip to the box */
	pinX = x + (int) (width - pinWidth) / 2;
	pinY = y + (int) (height - pinHeight) / 2;

	if (pinWidth < width)
	{
	    width = pinWidth;
	    x = pinX;
	}

	if (pinHeight < height)
	{
	    height = pinHeight;
	    y = pinY;
	}

	/* Draw the pin with 3 passes; one for each color.  We know that
	 * the light hightlight and bg3 are solid colors, so just add the
	 * bitmap as a stipple to the gc and do a fill area.  The interior
	 * is a little harder.  If bg1 is a pixmap background, then make
	 * the interior pixmap a clip bitmap.  This will be a tad slow,
	 * but it won't happen much in practice, so it should be OK.
	 */
	gc = OlgGetBrightGC (pInfo);
	XSetStipple (DisplayOfScreen (scr), gc, pDev->pushpin3D);
	XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	XSetTSOrigin (DisplayOfScreen (scr), gc, pinX, pinY - yOrig);

	XFillRectangle (DisplayOfScreen (scr), win, gc, x, y, width, height);

	XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);

	gc = OlgGetBg3GC (pInfo);
	XSetStipple (DisplayOfScreen (scr), gc, pDev->pushpin3D);
	XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	XSetTSOrigin (DisplayOfScreen (scr), gc, pinX - pinWidth, pinY - yOrig);

	XFillRectangle (DisplayOfScreen (scr), win, gc, x, y, width, height);

	XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);

	gc = OlgGetBg1GC (pInfo);
	if (pInfo->flags & OLG_BGPIXMAP)
	{
	    XSetClipMask (DisplayOfScreen (scr), gc, pDev->pushpin3D);
	    XSetClipOrigin (DisplayOfScreen(scr), gc, pinX - pinWidth*2, pinY - yOrig);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, x, y,
			    width, height);

	    XSetClipMask (DisplayOfScreen (scr), gc, None);
	    XSetClipOrigin (DisplayOfScreen (scr), gc, 0, 0);
	}
	else
	{
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->pushpin3D);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, pinX - pinWidth*2, pinY - yOrig);

	    XFillRectangle (DisplayOfScreen (scr), win, gc, x, y,
			    width, height);

	    XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	    XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);
	}
    }
    else
    {
	/* Load the bitmaps, if we haven't already. */
	if (!pDev->pushpin2D)
	    _OlgGetBitmaps (scr, pInfo, OLG_PUSHPIN_2D);

	/* 2-D pushpins are arranged as a 3x1 matrix.  The top item is the
	 * IN pin, the middle item is the OUT pin, and the bottom item is
	 * the DEFAULT pin.
	 */
	pinWidth = pDev->widthPushpin2D;
	pinHeight = pDev->heightPushpin2D / 3;

	switch (type) {
	case PP_IN:
	    yOrig = 0;
	    break;

	case PP_OUT:
	    yOrig = pinHeight;
	    break;

	case PP_DEFAULT:
	    yOrig = pinHeight * 2;
	    break;
	}

	/* Center the pin and clip to the box */
	pinX = x + (int) (width - pinWidth) / 2;
	pinY = y + (int) (height - pinHeight) / 2;

	if (pinWidth < width)
	{
	    width = pinWidth;
	    x = pinX;
	}

	if (pinHeight < height)
	{
	    height = pinHeight;
	    y = pinY;
	}

	/* Draw the pin.  We know that the foreground color is solid,
	 * so just add the bitmap as a stipple to the gc and do a fill
	 * area.
	 */
	gc = OlgGetFgGC (pInfo);
	XSetStipple (DisplayOfScreen (scr), gc, pDev->pushpin2D);
	XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	XSetTSOrigin (DisplayOfScreen (scr), gc, pinX, pinY - yOrig);

	XFillRectangle (DisplayOfScreen (scr), win, gc, x, y, width, height);

	XSetFillStyle (DisplayOfScreen (scr), gc, FillSolid);
	XSetTSOrigin (DisplayOfScreen (scr), gc, 0, 0);
    }
}

/* Size a pushpin.
 *
 * Determine the size of a pushpin for the current scale.  All types of
 * pushpins (in, out, default) have the same size.
 */

void
OlgSizePushPin (scr, pInfo, pWidth, pHeight)
    Screen	*scr;
    OlgAttrs	*pInfo;
    Dimension	*pWidth, *pHeight;
{
    register _OlgDevice	*pDev;

    pDev = pInfo->pDev;
    if (OlgIs3d())
    {
	/* Load the bitmaps, if we haven't already. */
	if (!pDev->pushpin3D)
	    _OlgGetBitmaps (scr, pInfo, OLG_PUSHPIN_3D);

	*pWidth = pDev->widthPushpin3D / 3;
	*pHeight = pDev->heightPushpin3D / 3;
    }
    else
    {
	/* Load the bitmaps, if we haven't already. */
	if (!pDev->pushpin2D)
	    _OlgGetBitmaps (scr, pInfo, OLG_PUSHPIN_2D);

	*pWidth = pDev->widthPushpin2D;
	*pHeight = pDev->heightPushpin2D / 3;
    }
}
