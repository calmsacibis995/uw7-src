#ifndef NOIDENT
#ident	"@(#)olg:OlgScrollO.c	1.8"
#endif

/* Scrollbar functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/ScrollbarP.h>

/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));


/* Draw the elevator of the correct type and state */

static void
drawElevator OLARGLIST((sbw, colorConflict))
    OLARG( ScrollbarWidget,	sbw)
    OLGRA( Boolean,		colorConflict)
{
    Screen	*scr = XtScreen((Widget)sbw);
    Drawable	win = XtWindow((Widget)sbw);
    Position	boxX, boxY;		/* position to draw elevator */
    Dimension	boxHeight, boxWidth;	/* dimensions of elevator */
    Dimension	padLen;			/* length of gap betweed elements */
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    _OlgDesc	*ul, *ur, *ll, *lr;	/* corners of elevator */
    _OlgDevice	*pDev = sbw->scroll.pAttrs->pDev;
    int		hStroke	= pDev->horizontalStroke;
    int		vStroke = pDev->verticalStroke;
    int		xInsetOrig, yInsetOrig, xInsetCorner, yInsetCorner;
    int		partSize, elevEnd;
    XRectangle	rects [5];
    GC		gc;
    Position	cx, cy;
    unsigned char	addStipple = False;
    unsigned	flags;

    /* Select the correct corners */
    if (OlgIs3d())
    {
	ul = &pDev->rect3UL;
	ur = &pDev->rect3UR;
	ll = &pDev->rect3LL;
	lr = &pDev->rect3LR;
	xInsetOrig = pDev->rect3OrigX;
	yInsetOrig = pDev->rect3OrigY;
	xInsetCorner = pDev->rect3CornerX;
	yInsetCorner = pDev->rect3CornerY;
    }
    else
    {
	ul = &pDev->rect2UL;
	ur = &pDev->rect2UR;
	ll = &pDev->rect2LL;
	lr = &pDev->rect2LR;
	xInsetOrig = pDev->rect2OrigX;
	yInsetOrig = pDev->rect2OrigY;
	xInsetCorner = pDev->rect2CornerX;
	yInsetCorner = pDev->rect2CornerY;
    }

    /* determine dimenions of overall elevator and placement of components */
    boxWidth = sbw->scroll.elevwidth;
    boxHeight = sbw->scroll.elevheight;
    if (horizontal)
    {
	/* outer box size and place */
	boxX = sbw->scroll.sliderPValue;
	boxY = sbw->scroll.offset;

	padLen = pDev->horizontalStroke;
	if (sbw->scroll.type != SB_MINIMUM)
	{
	    boxX += padLen;
	    boxWidth -= padLen * 2;
	}

	partSize = (unsigned) (boxWidth+1) / (sbw->scroll.type & EPARTMASK);
	elevEnd = boxX + boxWidth;

	/* line between left arrow and drag box or right arrow */
	rects [0].x = boxX + partSize - hStroke/2 - 1;
	rects [0].y = boxY + vStroke;
	rects [0].width = hStroke;
	rects [0].height = boxHeight - vStroke * 2;

	/* inset rectangle for left arrow */
	rects [2].x = boxX + xInsetOrig;
	rects [2].y = boxY + yInsetOrig;
	rects [2].width = rects [0].x - hStroke - rects [2].x;
	rects [2].height = boxHeight - yInsetCorner - yInsetOrig;

	if ((sbw->scroll.type & EPARTMASK) == 3)
	{
	    /* line between drag box and right arrow */
	    rects [1].x = elevEnd - partSize - hStroke/2;
	    rects [1].y = rects [0].y;
	    rects [1].width = hStroke;
	    rects [1].height = rects [0].height;

	    /* inset rectangle for drag box */
	    rects [3].x = rects [0].x + hStroke * 2;
	    rects [3].y = rects [2].y;
	    rects [3].width = rects [1].x - hStroke - rects [3].x;
	    rects [3].height = rects[2].height;
	}

	/* inset rectangle for right arrow */
	rects [4].x = elevEnd - partSize + (hStroke+1)/2 + hStroke;
	rects [4].y = rects [2].y;
	rects [4].width = elevEnd - xInsetCorner - rects [4].x;
	rects [4].height = rects [2].height;
    }
    else
    {
	/* outer box */
	boxY = sbw->scroll.sliderPValue;
	boxX = sbw->scroll.offset;

	padLen = pDev->verticalStroke;
	if (sbw->scroll.type != SB_MINIMUM)
	{
	    boxY += padLen;
	    boxHeight -= padLen * 2;
	}

	partSize = (unsigned) (boxHeight+1) / (sbw->scroll.type & EPARTMASK);
	elevEnd = boxY + boxHeight;

	/* line between top arrow and drag box or bottom arrow */
	rects [0].x = boxX + hStroke;
	rects [0].y = boxY + partSize - vStroke/2 - 1;
	rects [0].width = boxWidth - hStroke * 2;
	rects [0].height = vStroke;

	/* inset rectangle for top arrow */
	rects [2].x = boxX + xInsetOrig;
	rects [2].y = boxY + yInsetOrig;
	rects [2].width = boxWidth - xInsetCorner - xInsetOrig;
	rects [2].height = rects [0].y - vStroke - rects [2].y;

	if ((sbw->scroll.type & EPARTMASK) == 3)
	{
	    /* line between drag box and bottom arrow */
	    rects [1].x = rects [0].x;
	    rects [1].y = elevEnd - partSize - vStroke/2;
	    rects [1].width = rects [0].width;
	    rects [1].height = vStroke;

	    /* inset rectangle for drag box */
	    rects [3].x = rects [2].x;
	    rects [3].y = rects [0].y + vStroke * 2;
	    rects [3].width = rects[2].width;
	    rects [3].height = rects [1].y - vStroke - rects [3].y;
	}

	/* inset rectangle for bottom arrow */
	rects [4].x = rects [2].x;
	rects [4].y = elevEnd - partSize + (vStroke+1)/2 + vStroke;
	rects [4].width = rects [2].width;
	rects [4].height = elevEnd - yInsetCorner - rects [4].y;
    }

    /* Draw the outer box of the elevator */
    OlgDrawFilledRBox (scr, win, sbw->scroll.pAttrs, boxX, boxY,
		       boxWidth, boxHeight, ul, ur, ll, lr, FB_UP);

    OlgDrawRBox (scr, win, sbw->scroll.pAttrs, boxX, boxY,
		 boxWidth, boxHeight, ul, ur, ll, lr, RB_UP);

    /* Draw the lines between the boxes */
    if (OlgIs3d())
    {
	if ((sbw->scroll.type & EPARTMASK) == 3)
	{
	    XFillRectangles (XtDisplay (sbw), XtWindow (sbw),
			     OlgGetBrightGC (sbw->scroll.pAttrs),
			     &rects [0], 1);
	    XFillRectangles (XtDisplay (sbw), XtWindow (sbw),
			     OlgGetBg3GC (sbw->scroll.pAttrs), &rects [1], 1);
	}
	else
	    XFillRectangles (XtDisplay (sbw), XtWindow (sbw),
			     OlgGetBg3GC (sbw->scroll.pAttrs), &rects [0], 1);
    }
    else
	XFillRectangles (XtDisplay (sbw), XtWindow (sbw),
			 OlgGetFgGC (sbw->scroll.pAttrs), &rects [0],
			 (sbw->scroll.type & EPARTMASK) - 1);

    /* Highlight a button, if needed */
    gc = OlgIs3d() ?
	OlgGetBg2GC (sbw->scroll.pAttrs) : OlgGetFgGC (sbw->scroll.pAttrs);

    switch (sbw->scroll.opcode) {
    case GRAN_DEC:
	flags = SB_PREV_ARROW;
	break;

    case GRAN_INC:
	flags = SB_NEXT_ARROW;
	break;

    case DRAG_ELEV:
	flags = SB_DRAG;
	break;

    default:
	flags = 0;
	break;
    }

    /* If there is a conflict between the input focus color and the
     * foreground or background colors (as determined by the calling
     * function), then draw the elevator as if each of the parts is
     * selected.  If a button is really pressed, draw it as if it is
     * not selected.  Strange.
     */

    if (colorConflict)
	flags ^= SB_PREV_ARROW | SB_NEXT_ARROW | SB_DRAG;

    if (flags & SB_PREV_ARROW)
    {
	XFillRectangles (XtDisplay (sbw), XtWindow (sbw), gc, &rects [2], 1);
	if (OlgIs3d())
	    OlgDrawBox (XtScreen (sbw), XtWindow (sbw), sbw->scroll.pAttrs,
			rects [2].x, rects [2].y,
			rects [2].width, rects [2].height, True);
    }

    if (flags & SB_NEXT_ARROW)
    {
	XFillRectangles (XtDisplay (sbw), XtWindow (sbw), gc, &rects [4], 1);
	if (OlgIs3d())
	    OlgDrawBox (XtScreen (sbw), XtWindow (sbw), sbw->scroll.pAttrs,
			rects [4].x, rects [4].y,
			rects [4].width, rects [4].height, True);
    }

    if (flags & SB_DRAG)
    {
	XFillRectangles (XtDisplay (sbw), XtWindow (sbw), gc, &rects [3], 1);
	if (OlgIs3d())
	    OlgDrawBox (XtScreen (sbw), XtWindow (sbw), sbw->scroll.pAttrs,
			rects [3].x, rects [3].y,
			rects [3].width, rects [3].height, True);
    }

    /* Draw the arrows */

    /* Select the gc and add a stipple if the scrollbar is all the
     * way to the left/top.
     */
    if (OlgIs3d())
	gc = OlgGetBg3GC (sbw->scroll.pAttrs);
    else
	gc = (flags & SB_PREV_ARROW) ? OlgGetBg1GC(sbw->scroll.pAttrs) :
	    OlgGetFgGC (sbw->scroll.pAttrs);

    if (sbw->scroll.sliderValue == sbw->scroll.sliderMin)
    {
	addStipple = True;
	if (gc != pDev->blackGC)
	{
	    XSetStipple (XtDisplay (sbw), gc, pDev->inactiveStipple);
	    XSetFillStyle (XtDisplay (sbw), gc, FillStippled);
	}
	else
	    gc = pDev->dimGrayGC;
    }

    /* Draw the left/top arrow at the center point of its box */
    cx = rects [2].x + rects [2].width / 2;
    cy = rects [2].y + rects [2].height / 2;
    OlgDrawObject (XtScreen (sbw), XtWindow (sbw), gc,
		   horizontal ? &pDev->arrowLeft : &pDev->arrowUp,
		   cx, cy);

    /* If 2-D mode, we might have to change GCs for the right/bottom arrow */
    if (!OlgIs3d())
    {
	if ((flags & SB_PREV_ARROW) != (flags & SB_NEXT_ARROW))
	{
	    if (addStipple)
	    {
		addStipple = False;
		if (gc != pDev->dimGrayGC)
		    XSetFillStyle (XtDisplay (sbw), gc, FillSolid);
	    }
	    gc = (flags & SB_NEXT_ARROW) ? OlgGetBg1GC(sbw->scroll.pAttrs) :
		OlgGetFgGC (sbw->scroll.pAttrs);
	}
    }

    /* Add or remove the stipple for the right/bottom arrow */
    if (sbw->scroll.sliderValue ==
	sbw->scroll.sliderMax - sbw->scroll.proportionLength)
    {
	if (!addStipple)
	{
	    addStipple = True;
	    if (gc != pDev->blackGC)
	    {
		XSetStipple (XtDisplay (sbw), gc, pDev->inactiveStipple);
		XSetFillStyle (XtDisplay (sbw), gc, FillStippled);
	    }
	    else
		gc = pDev->dimGrayGC;
	}
    }
    else
	if (addStipple)
	{
	    addStipple = False;
	    if (gc != pDev->dimGrayGC)
		XSetFillStyle (XtDisplay (sbw), gc, FillSolid);
	    else
		gc = OlgIs3d() ? OlgGetBg3GC (sbw->scroll.pAttrs) :
		    OlgGetFgGC (sbw->scroll.pAttrs);
	}

    /* Draw the right/bottom arrow at the center point of its box */
    if (horizontal)
    {
	cx = rects [4].x + (int) (rects [4].width + 1) / 2;
	cy = rects [4].y + rects [4].height / 2;
    }
    else
    {
	cx = rects [4].x + rects [4].width / 2;
	cy = rects [4].y + (int) (rects [4].height + 1) / 2;
    }
    OlgDrawObject (XtScreen (sbw), XtWindow (sbw), gc,
		   horizontal ? &pDev->arrowRight : &pDev->arrowDown,
		   cx, cy);

    /* Clean up the gc */
    if (addStipple && gc != pDev->dimGrayGC)
	XSetFillStyle (XtDisplay (sbw), gc, FillSolid);
}

/* Calculate position of elevator and indicator */

static void
locateElevator OLARGLIST((sbw, cablePos, cableLen))
    OLARG( ScrollbarWidget,	sbw)
    OLARG( Position,		cablePos)
    OLGRA( Dimension,		cableLen)
{
    Position	elevPos;
    Dimension	elevLen;
    Position	indPos;
    Dimension	indLen;
    Dimension	padLen;
    int		userRange;	/* size of user coordinate space */

    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
	padLen = sbw->scroll.pAttrs->pDev->horizontalStroke;
	elevLen = sbw->scroll.elevwidth;
    }
    else
    {
	padLen = sbw->scroll.pAttrs->pDev->verticalStroke;
	elevLen = sbw->scroll.elevheight;
    }

    /* Find the position of the elevator */
    userRange = sbw->scroll.sliderMax - sbw->scroll.sliderMin;
    if (sbw->scroll.type != SB_REGULAR)
    {
	/* center the elevator */
	elevPos = (Position) (cableLen - elevLen) / 2;
    }
    else
    {
	if (userRange != sbw->scroll.proportionLength)
	{
	    elevPos = muldiv (sbw->scroll.sliderValue - sbw->scroll.sliderMin,
			      (int) (cableLen - elevLen),
			      userRange - sbw->scroll.proportionLength);
	}
	else
	    elevPos = 0;
    }

    sbw->scroll.sliderPValue = cablePos + elevPos;

    /* Find the proportion indicator positions */
    if (sbw->scroll.type == SB_REGULAR)
    {
	Dimension	minIndLen;
	Position	elevEndPos;

	elevEndPos = elevPos + elevLen;

	/* Find the position of the proportion indicator */
	minIndLen = padLen * 3;
	indLen = muldiv (sbw->scroll.proportionLength, (int) cableLen,
			 userRange);
	if (indLen < (Dimension) (elevLen + minIndLen * 2))
	{
	    /* Proportion indicator is hidden by the elevator.
	     * Draw a short one anyway.
	     */
	    indLen = elevLen + minIndLen * 2;
	    indPos = elevPos - minIndLen;
	}
	else
	{
	    /* Full sized proportion indicator.  Adjust it's position such
	     * that at least 3 points show on each side of the elevator.
	     */
	    if (userRange != sbw->scroll.proportionLength)
	    {
		indPos = muldiv (sbw->scroll.sliderValue-sbw->scroll.sliderMin,
				 (int) (cableLen - indLen),
				 userRange - sbw->scroll.proportionLength);
	    }
	    else
		indPos = 0;

	    if ((Dimension) (elevPos - indPos) < minIndLen)
		indPos = elevPos - minIndLen;
	    else
		if ((Position) (indPos + indLen - elevEndPos) <
		    (Position) minIndLen)
		    indPos = elevEndPos + minIndLen - indLen;
	}

	/* clip the indicator */
	if (indPos < 0)
	    indPos = 0;
	if ((Position) (indPos + indLen) > (Position) cableLen)
	{
	    indPos = cableLen - indLen;
	    if (indPos < 0)
	    {
		indPos = 0;
		indLen = cableLen;
	    }
	}

	sbw->scroll.indPos = indPos;
	sbw->scroll.indLen = indLen;
    }
}

/* Add a span record to array a at index i.  The index is incremented.
 * Discard degenerate spans.
 */
#define ADD_SPAN(a,i,s,e)	(((e) > (s)) ?		\
				 (a)[(i)*2] = (s)+cablePos, \
				 (a)[(i)++ * 2 + 1] = (e)+cablePos : 0)
/* Define macros to describe areas to clear, fill with the cable, and fill
 * with the indicator.
 */
#define CLEAR(s,e)	ADD_SPAN (clearSpans, numClear, (s), (e))
#define CABLE(s,e)	ADD_SPAN (cableSpans, numCable, (s), (e))
#define INDICATOR(s,e)	ADD_SPAN (indSpans, numInd, (s), (e))

/* Draw the cable.  We have three lists of spans that define the cable.
 * Clear the boxes in the first list, stipple the boxes in the second,
 * and solid fill the third.
 */

static void
drawCable OLARGLIST((sbw, clearSpans, clearCnt, stippleSpans, stippleCnt, solidSpans, solidCnt))
    OLARG( ScrollbarWidget,	sbw)
    OLARG( short	*,	clearSpans)
    OLARG( unsigned,		clearCnt)
    OLARG( short *,		stippleSpans)
    OLARG( unsigned,		stippleCnt)
    OLARG( short *,		solidSpans)
    OLGRA( unsigned,		solidCnt)
{
    XRectangle	rects [10];
    Display	*dpy = XtDisplay (sbw);
    Drawable	win = XtWindow (sbw);
    Position	start, end;
    Dimension	cableThickness;
    Position	cableOffset;
    register	i;
    GC		gc;
    _OlgDevice	*pDev = sbw->scroll.pAttrs->pDev;

    /* Select a GC for the indicator */
    gc = (OlgIs3d()) ? OlgGetBg3GC (sbw->scroll.pAttrs) :
	OlgGetFgGC (sbw->scroll.pAttrs);

    /* clear each rectangle.  Always clear the width of the elevator */
    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
	for (i=0; i<clearCnt; i++)
	{
	    start = *clearSpans++;
	    end = *clearSpans++;

	    XClearArea (dpy, win, start, sbw->scroll.offset, end - start,
			sbw->scroll.anchlen, False);
	}

	/* Draw the indicator */
	cableThickness = pDev->verticalStroke * 3;
	cableOffset = sbw->scroll.offset +
	    (Position) (sbw->scroll.anchlen - cableThickness) / 2;
	for (i=0; i<solidCnt; i++)
	{
	    start = *solidSpans++;
	    end = *solidSpans++;

	    rects [i].x = start;
	    rects [i].y = cableOffset;
	    rects [i].width = end - start;
	    rects [i].height = cableThickness;
	}
	XFillRectangles (dpy, win, gc, rects, solidCnt);

	/* Select a gc for stippling.  As bg3 is often solid black, there is
	 * already an appropriate 50% gray stipple to use for the cable.
	 * Otherwise, create one for the duration of drawing the cable.
	 */
	if (gc != pDev->blackGC)
	{
	    XSetFillStyle (dpy, gc, FillStippled);
	    XSetStipple (dpy, gc, pDev->inactiveStipple);
	}
	else
	    gc = pDev->dimGrayGC;

	/* Draw the cable */
	for (i=0; i<stippleCnt; i++)
	{
	    start = *stippleSpans++;
	    end = *stippleSpans++;

	    rects [i].x = start;
	    rects [i].y = cableOffset;
	    rects [i].width = end - start;
	    rects [i].height = cableThickness;
	}
	XFillRectangles (dpy, win, gc, rects, stippleCnt);

	if (gc != pDev->dimGrayGC)
	    XSetFillStyle (dpy, gc, FillSolid);
    }
    else
    {
	for (i=0; i<clearCnt; i++)
	{
	    start = *clearSpans++;
	    end = *clearSpans++;

	    XClearArea (dpy, win, sbw->scroll.offset, start,
			sbw->scroll.anchwidth, end - start, False);
	}

	/* Draw the indicator */
	cableThickness = pDev->horizontalStroke * 3;
	cableOffset = sbw->scroll.offset +
	    (Position) (sbw->scroll.anchwidth - cableThickness) / 2;
	for (i=0; i<solidCnt; i++)
	{
	    start = *solidSpans++;
	    end = *solidSpans++;

	    rects [i].x = cableOffset;
	    rects [i].y = start;
	    rects [i].width = cableThickness;
	    rects [i].height = end - start;
	}
	XFillRectangles (dpy, win, gc, rects, solidCnt);

	/* Select a gc for stippling.  As bg3 is often solid black, there is
	 * already an appropriate 50% gray stipple to use for the cable.
	 * Otherwise, create one for the duration of drawing the cable.
	 */
	if (gc != pDev->blackGC)
	{
	    XSetFillStyle (dpy, gc, FillStippled);
	    XSetStipple (dpy, gc, pDev->inactiveStipple);
	}
	else
	    gc = pDev->dimGrayGC;

	/* Draw the cable */
	for (i=0; i<stippleCnt; i++)
	{
	    start = *stippleSpans++;
	    end = *stippleSpans++;

	    rects [i].x = cableOffset;
	    rects [i].y = start;
	    rects [i].width = cableThickness;
	    rects [i].height = end - start;
	}
	XFillRectangles (dpy, win, gc, rects, stippleCnt);

	if (gc != pDev->dimGrayGC)
	    XSetFillStyle (dpy, gc, FillSolid);
    }
}

void
_OloOlgDrawScrollbar OLARGLIST((w, pInfo))
    OLARG( Widget,	w)
    OLGRA( OlgAttrs *,	pInfo)
{
    ScrollbarWidget sbw = (ScrollbarWidget)w;
    Screen	*scr = XtScreen(w);
    Drawable	win = XtWindow(w);
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    Position	cablePos;	/* screen coordinate of top/left of cable */
    Dimension	cableLen;	/* length of cable area */
    Position	elevPos;	/* offset of elevator in cable */
    Position	elevEnd;	/* offset of end of elevator in cable */
    Dimension	elevLen;	/* length of elevator */
    Position	indPos;		/* offset of proportion indicator */
    Position	indEnd;		/* end position of proportion indicator */
    Dimension	padLen;		/* length of gap between elements */
    int		numInd;		/* rectangle cnt for proportion indicator */
    int		numCable;	/* rectangle cnt for cable */
    short	indSpans [4], cableSpans [4];	/* rectangle arrays */
    _OlgDevice	*pDev = pInfo->pDev;
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;

    focus_color = sbw->primitive.input_focus_color;
    has_focus = sbw->primitive.has_focus;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d () && has_focus &&
	(sbw->primitive.foreground == focus_color ||
	 sbw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    if (horizontal)
    {
	cableLen = sbw->core.width;
	padLen = pDev->horizontalStroke;
	elevLen = sbw->scroll.elevwidth;
    }
    else
    {
	cableLen = sbw->core.height;
	padLen = pDev->verticalStroke;
	elevLen = sbw->scroll.elevheight;
    }
    cablePos = 0;

    /* If the scrollbar has anchors, draw them. */
    if (sbw->scroll.type != SB_MINIMUM)
    {
	if (horizontal)
	{
	    OlgDrawAnchor (scr, win, pInfo, 0, sbw->scroll.offset,
			sbw->scroll.anchwidth - padLen, sbw->scroll.anchlen,
			(sbw->scroll.opcode == ANCHOR_TOP) ^ colorConflict);
	    OlgDrawAnchor (scr, win, pInfo,
			sbw->core.width - sbw->scroll.anchwidth + padLen,
			sbw->scroll.offset,
			sbw->scroll.anchwidth - padLen, sbw->scroll.anchlen,
			(sbw->scroll.opcode == ANCHOR_BOT) ^ colorConflict);
	    cableLen -= sbw->scroll.anchwidth * 2;
	    cablePos = sbw->scroll.anchwidth;
	}
	else
	{
	    OlgDrawAnchor (scr, win, pInfo, sbw->scroll.offset, 0,
			sbw->scroll.anchwidth, sbw->scroll.anchlen - padLen,
			(sbw->scroll.opcode == ANCHOR_TOP) ^ colorConflict);
	    OlgDrawAnchor (scr, win, pInfo,
			sbw->scroll.offset,
			sbw->core.height - sbw->scroll.anchlen + padLen,
			sbw->scroll.anchwidth, sbw->scroll.anchlen - padLen,
			(sbw->scroll.opcode == ANCHOR_BOT) ^ colorConflict);
	    cableLen -= sbw->scroll.anchlen * 2;
	    cablePos = sbw->scroll.anchlen;
	}
    }

    /* Draw the elevator */
    locateElevator (sbw, cablePos, cableLen);
    drawElevator (sbw, colorConflict);

    /* Draw the cable and proportion indicator */
    if (sbw->scroll.type == SB_REGULAR)
    {
	elevPos = sbw->scroll.sliderPValue - cablePos;
	elevEnd = elevPos + elevLen;
	indPos = sbw->scroll.indPos;
	indEnd = indPos + sbw->scroll.indLen;

	/* Create rectangles for the proportion indicator and cable,
	 * potentially on both sides of the elevator.
	 */
	numInd = numCable = 0;

	/* proportion indicator and cable left/above the elevator */
	INDICATOR (indPos, elevPos);
	CABLE (0, (Position) (indPos - padLen));

	/* proportion indicator and cable right/below the elevator */
	INDICATOR (elevEnd, indEnd);
	CABLE ((Position) (indEnd + padLen), (Position) cableLen);

	drawCable (sbw, (short *)0, 0, cableSpans, numCable, indSpans, numInd);
    }
}

void
_OloOlgUpdateScrollbar OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
    ScrollbarWidget sbw = (ScrollbarWidget)w;
    Dimension	padLen;
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;

    focus_color = sbw->primitive.input_focus_color;
    has_focus = sbw->primitive.has_focus;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d () && has_focus &&
	(sbw->primitive.foreground == focus_color ||
	 sbw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    padLen = (horizontal) ? pInfo->pDev->horizontalStroke :
	pInfo->pDev->verticalStroke;

    /* Update anchors */
    if (sbw->scroll.type != SB_MINIMUM)
    {
	if (flags & SB_BEGIN_ANCHOR)
	{
	    if (horizontal)
	    {
		OlgDrawAnchor (XtScreen (sbw), XtWindow (sbw), pInfo,
			    0, sbw->scroll.offset,
			    sbw->scroll.anchwidth - padLen,
			    sbw->scroll.anchlen,
			    (sbw->scroll.opcode==ANCHOR_TOP) ^ colorConflict);
	    }
	    else
	    {
		OlgDrawAnchor (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->scroll.offset, 0,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen - padLen,
			    (sbw->scroll.opcode==ANCHOR_TOP) ^ colorConflict);
	    }
	}

	if (flags & SB_END_ANCHOR)
	{
	    if (horizontal)
	    {
		OlgDrawAnchor (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->core.width - sbw->scroll.anchwidth + padLen,
			    sbw->scroll.offset,
			    sbw->scroll.anchwidth - padLen,
			    sbw->scroll.anchlen,
			    (sbw->scroll.opcode==ANCHOR_BOT) ^ colorConflict);
	    }
	    else
	    {
		OlgDrawAnchor (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->scroll.offset,
			    sbw->core.height - sbw->scroll.anchlen + padLen,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen - padLen,
			    (sbw->scroll.opcode==ANCHOR_BOT) ^ colorConflict);
	    }
	}
    }

    /* If we are only updating an anchor, we're done */
    if (!(flags & ~(SB_BEGIN_ANCHOR | SB_END_ANCHOR)))
	return;

    /* Update the position.  Remember, only regular scrollbar can move */
    if (flags & SB_POSITION && sbw->scroll.type == SB_REGULAR)
    {
	Position	oldElevPos;
	Position	oldElevEnd;
	Position	oldIndPos;
	Position	oldIndEnd;
	Position	elevPos;
	Position	elevEnd;
	Dimension	elevLen;
	Position	indPos;
	Position	indEnd;
	Position	cablePos;
	Dimension	cableLen;
	short		clearSpans [20], cableSpans [20], indSpans [20];
	unsigned	numClear, numCable, numInd;

	/* Calculate the length and offset of the cable area */
	if (horizontal)
	{
	    cablePos = sbw->scroll.anchwidth;
	    cableLen = sbw->core.width - sbw->scroll.anchwidth * 2;
	    elevLen = sbw->scroll.elevwidth;
	}
	else
	{
	    cablePos = sbw->scroll.anchlen;
	    cableLen = sbw->core.height - sbw->scroll.anchlen * 2;
	    elevLen = sbw->scroll.elevheight;
	}

	/* Save the old indicator and elevator positions */
	oldElevPos = sbw->scroll.sliderPValue - cablePos;
	oldElevEnd = oldElevPos + elevLen;
	oldIndPos = sbw->scroll.indPos;
	oldIndEnd = oldIndPos + sbw->scroll.indLen;

	/* Get the new position of the elevator and indicator */
	locateElevator (sbw, cablePos, cableLen);
	elevPos = sbw->scroll.sliderPValue - cablePos;
	elevEnd = elevPos + elevLen;
	indPos = sbw->scroll.indPos;
	indEnd = indPos + sbw->scroll.indLen;

	/* Find the rectangles that must be repainted.  To minimize flashing,
	 * clear as little as possible.  Three rectangle lists are produced:
	 * a list of areas to clear, a list of areas to stipple, and a list
	 * of areas to fill.  Start with the area above/left of the elevator.
	 */
	numClear = numCable = numInd = 0;
	if (indPos < oldIndPos)
	{
	    /* indicator has moved left/up.  extend indicator */
	    if (indPos > 0)
		CLEAR (indPos - 1, indPos);
	    INDICATOR (indPos, _OlMin (elevPos, oldIndPos));
	}
	else if (indPos > oldIndPos)
	{
	    /* indicator has moved right/down exposing cable */
	    if (indPos < oldElevPos)
		CLEAR (oldIndPos, indPos);
	    else
	    {
		if (oldIndEnd < elevPos)
		{
		    CLEAR (oldIndPos, (Position) (oldIndEnd + padLen));
		    if (oldIndEnd < indPos)
			CLEAR ((Position) (indPos - padLen), indPos);
		}
		else
		    CLEAR (oldIndPos, (Position) (elevPos + padLen));
	    }
	    CABLE (_OlMax ((Position) (oldIndPos - padLen), 0),
		   _OlMin (_OlMax ((Position) (indPos - padLen), 0),
			(Position) (oldIndEnd + padLen)));
	}

	/* Has the elevator moved? */
	if (elevPos < oldElevPos)
	{
	    /* elevator has moved up/left.  extend indicator below elevator
	     * and remove it above.
	     */
	    CLEAR (elevPos, (Position) (elevPos + padLen));
	    CLEAR ((Position) (elevEnd - padLen), oldElevEnd);
	    INDICATOR (elevEnd, _OlMin (indEnd, oldElevEnd));
	}
	else if (elevPos > oldElevPos)
	{
	    /* elevator has moved down/right.  extend indicator above elevator
	     * and remove it below.
	     */
	    if (indPos <= oldElevPos)
	    {
		CLEAR (oldElevPos, (Position) (elevPos + padLen));
		INDICATOR (oldElevPos, elevPos);
	    }
	    else
	    {
		if (elevPos >= (Position) (oldIndEnd + padLen))
		    CLEAR (elevPos, (Position) (elevPos + padLen));
		INDICATOR (indPos, elevPos);
	    }
	    CLEAR ((Position) (elevEnd - padLen), elevEnd);
	}

	if (indEnd < oldIndEnd)
	{
	    /* bottom of indicator has moved up/left.  Shorten it */
	    CLEAR (indEnd, oldIndEnd);
	    CABLE ((Position) (indEnd + padLen),
		   _OlMin ((Position) (oldIndEnd + padLen), (Position) cableLen));
	}
	else if (indEnd > oldIndEnd)
	{
	    /* bottom of indicator has moved down/right.  Extend it */
	    INDICATOR (_OlMax (oldIndEnd, elevEnd), indEnd);
	    CLEAR (indEnd,
		   _OlMin ((Position) (indEnd + padLen), (Position) cableLen));
	}

	/* Display the cable and elevator.  If the new elevator position
	 * overlaps the old one, clear the area of the overlap.
	 */
	if (oldElevPos < elevEnd && oldElevEnd > elevPos)
	    CLEAR (_OlMax (oldElevPos, elevPos), _OlMin (oldElevEnd, elevEnd));
	drawCable (sbw, clearSpans, numClear, cableSpans, numCable,
		   indSpans, numInd);
	drawElevator (sbw, colorConflict);
    }
    else
    {
	/* Update elevator */
	if (flags & (SB_PREV_ARROW | SB_NEXT_ARROW | SB_DRAG))
	    drawElevator (sbw, colorConflict);
    }
}

/* Determine the size of a scroll bar anchor.  This is only correct for
 * the 12 point scale.
 */

void
_OloOlgSizeScrollbarAnchor OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    ScrollbarWidget sbw = (ScrollbarWidget)w;

    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, 6, pInfo->pDev->scr) +
	    pInfo->pDev->horizontalStroke;
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, 15, pInfo->pDev->scr);
    }
    else
    {
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, 6, pInfo->pDev->scr) +
	    pInfo->pDev->verticalStroke;
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, 15, pInfo->pDev->scr);
    }
}

/* Determine the size of a scroll bar elevator.  This is only correct for
 * the 12 point scale.
 */

void
_OloOlgSizeScrollbarElevator OLARGLIST((w, pInfo, type, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( OlDefine,	type)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    ScrollbarWidget	sbw = (ScrollbarWidget)w;
    Dimension		length;

    length = (type == SB_REGULAR) ? 47 : 32;

    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
	*pHeight = sbw->scroll.anchlen;
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, length,
				       pInfo->pDev->scr);
	if (type != SB_MINIMUM)
	    *pWidth += pInfo->pDev->horizontalStroke * 2;
    }
    else
    {
	*pWidth = sbw->scroll.anchwidth;
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, length,
					pInfo->pDev->scr);
	if (type != SB_MINIMUM)
	    *pHeight += pInfo->pDev->verticalStroke * 2;
    }
}
