#ifndef NOIDENT
#ident	"@(#)olg:OlgLines.c	1.8"
#endif

/* Line and box functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>

#define min(a,b)	((a)>(b) ? (b) : (a))

/* Draw a chiseled line (or flat line for 2-D)
 *
 * Draw a chiseled line raised from the surface for 3-D or a flat line for
 * 2-D.  The line is twice the stroke width wide for 3-D; one for a dark
 * stroke, one for a light stroke.  For 2-D visuals, the width of the line
 * is the basic stroke width times the value of the strokes parameter.  Lines
 * may either be horizontal or vertical.  x, y defines the upper left corner
 * of the line.
 */

void
OlgDrawLine (scr, win, pInfo, x, y, length, strokes, isVertical)
    Screen	*scr;
    OlgAttrs	*pInfo;
    Position	x, y;
    Dimension	length;
    unsigned	strokes;
    unsigned	isVertical;
{
    Dimension	hStroke, vStroke;

    hStroke = OlgGetHorizontalStroke (pInfo);
    vStroke = OlgGetVerticalStroke (pInfo);

    if (OlgIs3d ())
    {
	GC	lightGC, darkGC;
	XRectangle	lightRects [10], darkRects [10];
	Position	mid, end;
	register unsigned	i;

	/* 3-D is a two stroke line; light above/left and dark below/right.
	 * The ends of the line are also beveled.
	 */
	lightGC = OlgGetBrightGC (pInfo);
	darkGC = OlgGetBg3GC (pInfo);

	if (isVertical)
	{
	    mid = x + hStroke;

	    lightRects [0].x = x;
	    lightRects [0].y = y;
	    lightRects [0].width = hStroke;
	    lightRects [0].height = length;

	    darkRects [0].x = mid;
	    darkRects [0].y = y;
	    darkRects [0].width = hStroke;
	    darkRects [0].height = length;

	    x = mid;
	    end = y + length - hStroke;
	    y += hStroke;
	    for (i=1; i<=hStroke; i++)
	    {
		lightRects [i].x = x;
		lightRects [i].y = --y;
		lightRects [i].width = i;
		lightRects [i].height = 1;

		darkRects [i].x = --mid;
		darkRects [i].y = end++;
		darkRects [i].width = i;
		darkRects [i].height = 1;
	    }

	    XFillRectangles (DisplayOfScreen (scr), win, lightGC,
			     lightRects, 1);
	    XFillRectangles (DisplayOfScreen (scr), win, darkGC,
			     darkRects, 1);
	    XFillRectangles (DisplayOfScreen (scr), win, lightGC,
			     lightRects + 1, hStroke);
	    XFillRectangles (DisplayOfScreen (scr), win, darkGC,
			     darkRects + 1, hStroke);
	}
	else
	{
	    mid = y + vStroke;

	    lightRects [0].x = x;
	    lightRects [0].y = y;
	    lightRects [0].width = length;
	    lightRects [0].height = vStroke;

	    darkRects [0].x = x;
	    darkRects [0].y = mid;
	    darkRects [0].width = length;
	    darkRects [0].height = vStroke;

	    end = x + length - vStroke;
	    x += vStroke;
	    y = mid;
	    for (i=1; i<=vStroke; i++)
	    {
		lightRects [i].x = --x;
		lightRects [i].y = y;
		lightRects [i].width = 1;
		lightRects [i].height = i;

		darkRects [i].x = end++;
		darkRects [i].y = --mid;
		darkRects [i].width = 1;
		darkRects [i].height = i;
	    }

	    XFillRectangles (DisplayOfScreen (scr), win, lightGC,
			     lightRects, 1);
	    XFillRectangles (DisplayOfScreen (scr), win, darkGC,
			     darkRects, 1);
	    XFillRectangles (DisplayOfScreen (scr), win, lightGC,
			     lightRects + 1, vStroke);
	    XFillRectangles (DisplayOfScreen (scr), win, darkGC,
			     darkRects + 1, vStroke);
	}
    }
    else
    {
	Dimension	width, height;

	/* 2-D is a simple rectangle */
	if (isVertical)
	{
	    width = strokes * hStroke;
	    height = length;
	}
	else
	{
	    width = length;
	    height = strokes * vStroke;
	}

	XFillRectangle (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			x, y, width, height);
    }
}

/* Draw a chiseled box (or flat box for 2-D)
 *
 * Draw a chiseled box with outer dimensions of width x height.  The lines
 * are twice the stroke width wide; one for a dark stroke, one for a light
 * stroke.  For 2-D visuals, a simple box is drawn using single stroke width
 * wide lines.
 */

void
OlgDrawChiseledBox OLARGLIST((scr, win, pInfo, x, y, width, height))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Position,	x)
    OLARG( Position,	y)
    OLARG( Dimension,	width)
    OLGRA( Dimension,	height)
{
    /* Draw a box */
    OlgDrawBox (scr, win, pInfo, x, y, width, height, True);

    if (OlgIs3d())
    {
	Dimension	hStroke, vStroke;

	hStroke = OlgGetHorizontalStroke (pInfo);
	vStroke = OlgGetVerticalStroke (pInfo);

	/* Draw a second box inside the first. */
	OlgDrawBox (scr, win, pInfo, x + hStroke, y + vStroke,
		    width - hStroke * 2, height - vStroke * 2, False);
    }
}

/* Draw a rectangular box.  For 3-D, it is drawn to appear raised from
 * the background plane or pressed into it.  For 2-D, it is a simple
 * rectangle drawn in the foreground color.  The box is not filled.
 */

void
OlgDrawBox OLARGLIST((scr, win, pInfo, x, y, width, height, isPressed))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Position,	x)
    OLARG( Position,	y)
    OLARG( Dimension,	width)
    OLARG( Dimension,	height)
    OLGRA( Boolean,	isPressed)
{
    unsigned	hStroke, vStroke;
    Position	topX, topY, rightX, bottomY;
    Dimension	currentHeight, currentWidth;
    register	offset;
    XRectangle	topRects [64], bottomRects [64];
    register	numRects;

    hStroke = pInfo->pDev->horizontalStroke;
    vStroke = pInfo->pDev->verticalStroke;

    /* if the box is too small, quit.  This is probably the wrong thing to
     * do, but it shouldn't happen anyway.
     */
    if (width <= hStroke * 2 || height <= vStroke * 2)
    {
	OlVaDisplayWarningMsg(	(Display *) NULL, 
				OleNfileOlgLines,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileOlgLines_msg1);
	return;
    }

    numRects = 0;
    if (OlgIs3d())
    {
	/* Generate boxes for the top and left, being careful to mitre
	 * the top right and bottom left.
	 */
	topX = x;
	topY = y;
	rightX = x + width - 1;
	bottomY = y + height - 1;
	currentWidth = width - 1;
	currentHeight = height - 1;

	for (offset=min (vStroke, hStroke); offset-->0; )
	{
	    bottomRects [numRects].x = topX;
	    bottomRects [numRects].y = bottomY--;
	    bottomRects [numRects].width = currentWidth;
	    bottomRects [numRects].height = 1;
	    numRects++;

	    bottomRects [numRects].x = rightX--;
	    bottomRects [numRects].y = topY + 1;
	    bottomRects [numRects].width = 1;
	    bottomRects [numRects].height = currentHeight;
	    numRects--;

	    topRects [numRects].x = topX++;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = 1;
	    topRects [numRects].height = currentHeight;
	    numRects++;

	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY++;
	    topRects [numRects].width = currentWidth;
	    topRects [numRects].height = 1;
	    numRects++;

	    currentWidth -= 2;
	    currentHeight -= 2;
	}

	if (vStroke > hStroke)
	{
	    /* Must draw additional lines at top and bottom */
	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = currentWidth;
	    topRects [numRects].height = vStroke - hStroke;

	    bottomRects [numRects].x = topX;
	    bottomRects [numRects].y = bottomY;
	    bottomRects [numRects].width = currentWidth;
	    bottomRects [numRects].height = vStroke - hStroke;
	    numRects++;
	}
	else if (vStroke < hStroke)	/* but not equal */
	{
	    /* Must draw additional lines at left and right */
	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = hStroke - vStroke;
	    topRects [numRects].height = currentHeight;

	    bottomRects [numRects].x = rightX - (hStroke - vStroke) + 1;
	    bottomRects [numRects].y = topY;
	    bottomRects [numRects].width = hStroke - vStroke;
	    bottomRects [numRects].height = currentHeight;
	    numRects++;
	}

	/* Draw the rectangles generated */
	XFillRectangles (DisplayOfScreen (scr), win, isPressed ?
			     OlgGetBg3GC (pInfo) : OlgGetBrightGC (pInfo),
			 topRects, numRects);
	XFillRectangles (DisplayOfScreen (scr), win, isPressed ?
			     OlgGetBrightGC (pInfo) : OlgGetBg3GC (pInfo),
			 bottomRects, numRects);
    }
    else
    {
	/* The 2-D case is easy--fill 4 rectangles */
	topRects [0].x = x;
	topRects [0].y = y;
	topRects [0].width = width;
	topRects [0].height = vStroke;

	topRects [1].x = x + width - hStroke;
	topRects [1].y = y + vStroke;
	topRects [1].width = hStroke;
	topRects [1].height = height - vStroke;

	topRects [2].x = x;
	topRects [2].y = y + height - vStroke;
	topRects [2].width = width - hStroke;
	topRects [2].height = vStroke;

	topRects [3].x = x;
	topRects [3].y = y + vStroke;
	topRects [3].width = hStroke;
	topRects [3].height = height - vStroke * 2;

	XFillRectangles (DisplayOfScreen (scr), win, pInfo->bg2,
			 topRects, 4);
    }
}
