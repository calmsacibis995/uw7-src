#ifndef NOIDENT
#ident	"@(#)olg:OlgRBox.c	1.8"
#endif

/* Draw Objects with rounded corners */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>

#define max(a,b)	((a)<(b) ? (b) : (a))
#define min(a,b)	((a)>(b) ? (b) : (a))

/* Draw an object with rounded corners.  The object is not filled.  The
 * flags arguments determines how the thing is drawn.  Valid flags:
 *
 *	RB_UP - draw raised from surface; otherwise lowered (ignored in 2-D)
 *	RB_MONO - draw the object in a solid color (always the case in 2-D,
 *		overrides RB_UP in 3-D)
 *	RB_OMIT_LCAP - do not close the left side; draw square
 *	RB_OMIT_BCAP - do not close bottom side; draw square
 *
 * This function assumes that the dimensions of the corners are reasonable;
 * that is:
 *	ul->height == ur->height
 *	ur->width == lr->width
 *	lr->height == ll->height
 *	ll->width == ul->width
 *
 */

void
OlgDrawRBox OLARGLIST((scr, win, pInfo, x, y, width, height, ul, ur, ll, lr, flags))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)	/* GCs and other device specific stuff */
    OLARG( Position,	x)	/* position of upper left corner of box */
    OLARG( Position,	y)	/* position of upper left corner of box */
    OLARG( Dimension,	width)	/* dimensions of box */
    OLARG( Dimension,	height)	/* dimensions of box */
    OLARG( _OlgDesc *,	ul)	/* definitions of corners */
    OLARG( _OlgDesc *,	ur)	/* definitions of corners */
    OLARG( _OlgDesc *,	ll)	/* definitions of corners */
    OLARG( _OlgDesc *,	lr)	/* definitions of corners */
    OLGRA( OlBitMask,	flags)	/* miscellaneous flags */
{
    int		currX, currY;	/* Y is current line, X marks 45 deg. line */
    int		origX, origY;	/* coordinates of upper left point of arc */
    int		xOffset;	/* x coordinate of rectangle origin */
    int		midWidth, midHeight;	/* distance between corners */
    int		startX, endX;	/* left and right coordinate of curve */
    register unsigned char	*pData;	/* pointer to arc data */
    XRectangle	clipRect;	/* clipping rectangle for button */
    char	clip;		/* clip flag */
    XRectangle	rects [256];	/* rectangle list */
    register int	numRects;	/* rectangle count */
    GC		gc;		/* graphics context to draw with */

    clip = False;
    clipRect.x = x;
    clipRect.y = y;
    clipRect.width = width;
    clipRect.height = height;

    numRects = 0;

    if (flags & RB_OMIT_LCAP)
    {
	/* determine the distance between the caps */
	midWidth = width - ur->width;
	midHeight = height - ur->height - lr->height;

	/* if the bounding box is smaller than the size of just the caps,
	 * we must clip.  The upper right part of the object will be visible.
	 */
	if (midWidth < 0)
	{
	    clip = True;
	    x += midWidth;
	    midWidth = 0;
	}

	if (midHeight < 0)
	{
	    clip = True;
	    midHeight = 0;
	}
    }
    else	/* normal and omit bottom cases */
    {
	/* determine the distance between the caps */
	midWidth = width - ul->width - ur->width;
	midHeight = height - ul->height;
	if (!(flags & RB_OMIT_BCAP))
	    midHeight -= ll->height;

	/* if the bounding box is smaller than the size of just the caps,
	 * we must clip.  The upper left part of the object will be visible.
	 */
	if (midWidth < 0)
	{
	    clip = True;
	    midWidth = 0;
	}
	if (midHeight < 0)
	{
	    clip = True;
	    midHeight = 0;
	}
    }

    if (!(flags & (RB_OMIT_LCAP | RB_OMIT_BCAP)))
    {
	/* Start at the 45 degree point in the lower left arc and generate
	 * rectangles going clockwise.
	 */
	origY = y + ul->height + midHeight;
	currY = min (ll->height, ll->width);
	currX = ll->width - currY;
	pData = ll->pData + (currY << 1);
	while (--currY >= 0)
	{
	    endX = *--pData;
	    startX = *--pData;

	    if (startX < currX)
	    {
		rects [numRects].x = x + startX;
		rects [numRects].y = origY + currY;
		rects [numRects].width = min (currX, endX) - startX + 1;
		rects [numRects].height = 1;
		numRects++;
	    }
	    currX++;
	}
    }

    if (!(flags & RB_OMIT_LCAP))
    {
	/* generate the left side */
	if (midHeight > 0)
	{
	    rects [numRects].x = x;
	    rects [numRects].y = y + ul->height;
	    rects [numRects].width = ll->pData [1] - ll->pData [0] + 1;
	    rects [numRects].height = midHeight;
	    numRects++;
	}

	/* generate the upper left corner */
	currY = ul->height;
	currX = 0;
	pData = ul->pData + (currY << 1);
	while (--currY >= 0)
	{
	    endX = *--pData;
	    startX = *--pData;

	    rects [numRects].x = x + startX;
	    rects [numRects].y = y + currY;
	    rects [numRects].width = endX - startX + 1;
	    rects [numRects].height = 1;
	    numRects++;
	    currX++;
	}
    }

    /* generate the top and upper right, stopping at the 45 degree mark */
    origX = x + midWidth;
    if (!(flags & RB_OMIT_LCAP))
    {
	xOffset = x + ul->width;
	origX += ul->width;
    }
    else
	xOffset = x;
    currY = 0;
    currX = ur->height - 1;
    pData = ur->pData;
    for ( ; currY < (int) ur->height; currX--, currY++)
    {
	startX = *pData++;
	endX = *pData++;

	/* if the left coordinate is 0, then extend the line back to
	 * form the top.  If the value is to the
	 * right of the 45 degree diagonal, draw it later.
	 */
	if (startX > currX)
	    continue;

	rects [numRects].y = y + currY;
	rects [numRects].height = 1;
	rects [numRects].width = min (currX, endX) - startX + 1;

	if (startX == 0)
	{
	    rects [numRects].x = xOffset;
	    rects [numRects].width += midWidth;
	}
	else
	    rects [numRects].x = origX + startX;

	numRects++;
    }

    /* Draw the rectangles generated so far */
    if (OlgIs3d ())
	gc = ((flags & (RB_UP|RB_MONO)) == RB_UP) ?
	    OlgGetBrightGC (pInfo) : OlgGetBg3GC (pInfo);
    else
	gc = OlgGetBg2GC (pInfo);

    if (clip)
	XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
			    &clipRect, 1, YXBanded);

    XFillRectangles (DisplayOfScreen (scr), win, gc, rects, numRects);
    numRects = 0;

    if (clip && OlgIs3d ())
	XSetClipMask (DisplayOfScreen (scr), gc, None);

    /* Finish the upper right corner */
    currY = 0;
    currX = ur->height;
    pData = ur->pData;
    for ( ; currY < (int) ur->height; currX--, currY++)
    {
	startX = *pData++;
	endX = *pData++;

	if (endX < currX)
	    continue;

	startX = max (startX, currX);
	rects [numRects].x = origX + startX;
	rects [numRects].y = y + currY;
	rects [numRects].width = endX - startX + 1;
	rects [numRects].height = 1;
	numRects++;
    }

    /* generate the right side */
    if (midHeight > 0)
    {
	pData = ur->pData + ((ur->height - 1) << 1);
	startX = *pData++;
	endX = *pData;

	rects [numRects].x = x + midWidth + startX +
	    ((!(flags & RB_OMIT_LCAP)) ? ul->width : 0);
	rects [numRects].y = y + ur->height;
	rects [numRects].width = endX - startX + 1;
	rects [numRects].height = midHeight;
	numRects++;
    }

    if (!(flags & RB_OMIT_BCAP))
    {
	/* generate the lower right corner and bottom */
	currY = 0;
	origX = x + midWidth;
	if (!(flags & RB_OMIT_LCAP))
	{
	    xOffset = x + ll->width;
	    origX += ll->width;
	}
	else
	    xOffset = x;
	origY = y + ur->height + midHeight;
	pData = lr->pData;
	while (currY < (int) lr->height)
	{
	    startX = *pData++;
	    endX = *pData++;

	    rects [numRects].y = origY + currY++;
	    rects [numRects].width = endX - startX + 1;

	    if (startX == 0)
	    {
		/* extend the line across the bottom */
		rects [numRects].x = xOffset;
		rects [numRects].width += midWidth;
	    }
	    else
		rects [numRects].x = origX + startX;

	    rects [numRects].height = 1;
	    numRects++;
	    currX--;
	}

	if (!(flags & RB_OMIT_LCAP))
	{
	    /* finish the lower left, stopping at the 45 degree mark */
	    origY = y + ul->height + midHeight;
	    currY = ll->height;
	    currX = ll->width - currY;
	    pData = ll->pData + (currY << 1);
	    while (--currY >= 0)
	    {
		endX = *--pData;
		startX = *--pData;

		/* If the value is to the left of the 45 degree
		 * diagonal, we already drew it.
		 */

		if (endX >= currX)
		{
		    startX = max (startX, currX);
		    rects [numRects].x = x + startX;
		    rects [numRects].y = origY + currY;
		    rects [numRects].width = endX - startX + 1;
		    rects [numRects].height = 1;
		    numRects++;
		}
		currX++;
	    }
	}
    }

    /* Draw the remaining rectangles */
    if (OlgIs3d ())
    {
	gc = (flags & (RB_UP|RB_MONO)) ?
	      OlgGetBg3GC (pInfo) : OlgGetBrightGC (pInfo);

	if (clip)
	    XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
				&clipRect, 1, YXBanded);
    }

    XFillRectangles (DisplayOfScreen (scr), win, gc, rects, numRects);

    if (clip)
	XSetClipMask (DisplayOfScreen (scr), gc, None);
}

/* Fill an object with rounded corners.  The
 * flags arguments determines how the thing is drawn.  Valid flags:
 *
 *	FB_UP - fill with bg1 if set; otherwise fill with bg2
 *	FB_BUSY - fill with busy stipple (FB_UP flag is ignored if FB_BUSY set)
 *	FB_OMIT_RCAP - do not draw rounded cap on the right side; draw square
 *	FB_OMIT_TCAP - do not draw rounded cap on the bottom side; draw square
 *	FB_OMIT_LCAP - do not draw rounded cap on the left side; draw square
 *	FB_OMIT_BCAP - do not draw rounded cap on the top side; draw square
 *
 * This function assumes that the dimensions of the corners are reasonable;
 * that is:
 *	ul->height == ur->height
 *	ur->width == lr->width
 *	lr->height == ll->height
 *	ll->width == ul->width
 *
 */

void
OlgDrawFilledRBox OLARGLIST((scr, win, pInfo, x, y, width, height, ul, ur, ll, lr, flags))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)	/* GCs and other device specific stuff */
    OLARG( Position,	x)	/* position of upper left corner of box */
    OLARG( Position,	y)	/* position of upper left corner of box */
    OLARG( Dimension,	width)	/* dimensions of box */
    OLARG( Dimension,	height)	/* dimensions of box */
    OLARG( _OlgDesc *,	ul)	/* definitions of corners */
    OLARG( _OlgDesc *,	ur)	/* definitions of corners */
    OLARG( _OlgDesc *,	ll)	/* definitions of corners */
    OLARG( _OlgDesc *,	lr)	/* definitions of corners */
    OLGRA( OlBitMask,	flags)		/* miscellaneous flags */
{
    int		currY;		/* current line. */
    int		midWidth, midHeight;	/* distance between corners */
    int		xOffset;	/* x coordinate of right arc */
    int		origX, origY;	/* coordinates of rectangle origin */
    unsigned	rectWidth;	/* width of current rectangle */
    register unsigned char	*pLeft, *pRight;     /* pointer to arc data */
    XRectangle	clipRect;	/* clipping rectangle for button */
    char	clip;		/* clip flag */
    XRectangle	rects [256];	/* rectangle list */
    register int	lastRect;	/* index of last rectangle */
    GC		gc;		/* graphics context to draw with */
    _OlgDesc	fakeCorner1, fakeCorner2;	/* temp corner descriptions */

    clip = False;
    clipRect.x = x;
    clipRect.y = y;
    clipRect.width = width;
    clipRect.height = height;

    if (flags & (FB_OMIT_TCAP | FB_OMIT_BCAP))
    {
	/* Create bogus top caps the same width as the bottom/top caps but only
	 * 1 pixel high.
	 */
	fakeCorner1.width = ll->width;
	fakeCorner1.height = 1;
	fakeCorner1.pData = (unsigned char *) XtMalloc (2);

	fakeCorner2.width = lr->width;
	fakeCorner2.height = 1;
	fakeCorner2.pData = (unsigned char *) XtMalloc (2);

	if (!fakeCorner1.pData || !fakeCorner2.pData)
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNnoMemory,
					OleTmalloc,
					OleCOlToolkitError,
					OleMnoMemory_malloc,
					"OlgRBox",
					"OlgDrawFilledRBox");

	fakeCorner1.pData [0] = fakeCorner1.pData [1] = 0;
	fakeCorner2.pData [0] = fakeCorner2.pData [1] = lr->width - 1;

	if (flags & FB_OMIT_TCAP)
	{
	    ul = &fakeCorner1;
	    ur = &fakeCorner2;
	}
	else
	{
	    ll = &fakeCorner1;
	    lr = &fakeCorner2;
	}
    }

    if (flags & (FB_OMIT_RCAP | FB_OMIT_LCAP))
    {
	/* Create bogus right/left caps the same height as the other
	 * caps but only 1 pixel wide.
	 */
	fakeCorner1.width = 1;
	fakeCorner1.height = ul->height;
	fakeCorner1.pData = (unsigned char *) XtCalloc (2 * ul->height, 1);

	fakeCorner2.width = 1;
	fakeCorner2.height = ll->height;
	fakeCorner2.pData = (unsigned char *) XtCalloc (2 * ll->height, 1);

	if (!ur->pData || !lr->pData)
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNnoMemory,
					OleTmalloc,
					OleCOlToolkitError,
					OleMnoMemory_malloc,
					"OlgRBox",
					"OlgDrawFilledRBox");

	if (flags & FB_OMIT_RCAP)
	{
	    ur = &fakeCorner1;
	    lr = &fakeCorner2;
	}
	else
	{
	    ul = &fakeCorner1;
	    ll = &fakeCorner2;
	}
    }

    /* determine the distance between the caps */
    midHeight = height - ul->height - ll->height;
    midWidth = width - ul->width - ur->width;

    /* if the bounding box is smaller than the size of just the caps,
     * we must clip.  The upper left part of the object will be visible,
     * except if the top cap is omitted, in which case the lower left part
     * of the object is visible.  If the left cap is omitted, the right side
     * is visible.
     */
    if (midWidth < 0)
    {
	clip = True;
	if (flags & FB_OMIT_LCAP)
	    x += midWidth;
	midWidth = 0;
    }
    if (midHeight < 0)
    {
	clip = True;
	if (flags & FB_OMIT_TCAP)
	    y += midHeight;
	midHeight = 0;
    }

    /* Starting at the top, generate rectangles to cover the top arcs. */
    pLeft = ul->pData;
    pRight = ur->pData + 1;
    xOffset = ul->width + midWidth;

    rects [0].x = x + *pLeft;
    rects [0].y = y;
    rects [0].width = xOffset + *pRight - *pLeft + 1;
    rects [0].height = 1;
    lastRect = 0;
    for (currY = 1; currY < (int) ul->height; currY++)
    {
	pLeft += 2;
	pRight += 2;
	origX = x + *pLeft;
	rectWidth = xOffset + *pRight - *pLeft + 1;

	/* coalesce rectangles, if possible */
	if (origX == rects [lastRect].x &&
	    rectWidth == rects [lastRect].width)
	{
	    rects [lastRect].height++;
	}
	else
	{
	    lastRect++;
	    rects [lastRect].x = origX;
	    rects [lastRect].y = y + currY;
	    rects [lastRect].width = rectWidth;
	    rects [lastRect].height = 1;
	}
    }

    /* Fill the center part */
    rects [lastRect].height += midHeight;

    /* generate rectangles for the lower portion */
    pLeft = ll->pData;
    pRight = lr->pData + 1;

    origY = y + midHeight + ul->height;
    for (currY = 0; currY < (int) ll->height; currY++)
    {
	origX = x + *pLeft;
	rectWidth = xOffset + *pRight - *pLeft + 1;

	/* coalesce rectangles, if possible */
	if (origX == rects [lastRect].x &&
	    rectWidth == rects [lastRect].width)
	{
	    rects [lastRect].height++;
	}
	else
	{
	    lastRect++;
	    rects [lastRect].x = origX;
	    rects [lastRect].y = origY + currY;
	    rects [lastRect].width = rectWidth;
	    rects [lastRect].height = 1;
	}
	pLeft += 2;
	pRight += 2;
    }

    /* Clean up temporary space */
    if (flags & (FB_OMIT_TCAP | FB_OMIT_RCAP | FB_OMIT_BCAP | FB_OMIT_LCAP))
    {
	XtFree ((char *)fakeCorner1.pData);
	XtFree ((char *)fakeCorner2.pData);
    }

    /* Fill the rectangles */
    gc = (flags & FB_BUSY) ? pInfo->pDev->busyGC :
	(flags & FB_UP) ? OlgGetBg1GC (pInfo) : OlgGetBg2GC (pInfo);

    if (clip)
	XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
			    &clipRect, 1, YXBanded);

    XFillRectangles (DisplayOfScreen (scr), win, gc, rects, lastRect + 1);

    if (clip)
	XSetClipMask (DisplayOfScreen (scr), gc, None);
}

/* Draw a simple object
 *
 * Instead of taking four corners and bolting them together as above,
 * use a single description and fill it.  This is useful for things like
 * arrows.  Don't make any attempt to clip.  The object is drawn centered
 * around the point given.
 */

void
OlgDrawObject (scr, win, gc, desc, x, y)
    Screen	*scr;
    Drawable	win;
    GC		gc;
    register _OlgDesc	*desc;
    Position	x, y;
{
    Position	xOrig, yOrig, currY;
    int		lastRect;
    XRectangle	rects [256];
    register unsigned char	*pData;
    Boolean skip;

    /* Find the upper left corner of the object. */
    xOrig = x - desc->width / 2;
    yOrig = y - desc->height / 2;

    /* Generate rectangles for each scan line, coalescing rectangles,
     * if possible.
     */
    pData = desc->pData;
    /*  This while loop allows for blank lines in the description by
	specifying a start point that is greater than the end point. */
    currY = 0;
    while (*pData > *(pData + 1) && currY < (int) desc->height)  {
	pData += 2;
	currY++;
    }

    rects [0].x = xOrig + *pData;
    rects [0].y = yOrig + currY;
    rects [0].width = *(pData + 1) - *pData + 1;
    rects [0].height = 1;
    currY++;

    lastRect = 0;
    skip = False;
    for (; currY < (int) desc->height; currY++)
    {
	Position	nX, wid;

	pData += 2;
	/*  skip a line when the start position is greater than end */
	if (*pData > *(pData + 1))  {
		skip = True;
		continue;
	}
	nX = xOrig + *pData;
	wid = *(pData + 1) - *pData + 1;
	
	/* coalesce rectangles, if possible */
	if (nX == rects [lastRect].x && wid == rects [lastRect].width && !skip)
	    rects [lastRect].height++;
	else
	{
	    lastRect++;
	    rects [lastRect].x = nX;
	    rects [lastRect].y = yOrig + currY;
	    rects [lastRect].width = wid;
	    rects [lastRect].height = 1;
	    skip = False;
	}
    }

    /* Fill the rectangles */
    XFillRectangles (DisplayOfScreen (scr), win, gc, rects, lastRect + 1);
}
