#ifndef NOIDENT
#ident	"@(#)olg:OlgSliderO.c	1.8"
#endif

/* OlgSliderO.c - Open Look GUI dependent Slider drawing functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/SliderP.h>


static void locateElevator OL_ARGS((SliderWidget, Position, Dimension));
static void drawTicks OL_ARGS((SliderWidget, Position, Position));
static void drawCable OL_ARGS((SliderWidget, Position,
				Position, Position, Position, Position));
static void drawElevator OL_ARGS((SliderWidget, Boolean));
static void drawIndicator OL_ARGS((SliderWidget, Position,
				Position, Position, Position, Position));


/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));

/* Draw the elevator of the correct type and state */

static void
drawElevator OLARGLIST((sw, colorConflict))
    OLARG( SliderWidget,	sw)
    OLGRA( Boolean, 	colorConflict)
{
    Display	*display = XtDisplay((Widget)sw);
    Screen	*scr = XtScreen((Widget)sw);
    Drawable	win = XtWindow((Widget)sw);
    Position	boxX, boxY;		/* position to draw elevator */
    Dimension	boxHeight, boxWidth;	/* dimensions of elevator */
    unsigned	horizontal = (sw->slider.orientation == OL_HORIZONTAL);
    _OlgDesc	*ul, *ur, *ll, *lr;	/* corners of elevator */
    _OlgDevice	*pDev = sw->slider.pAttrs->pDev;
    int		hStroke	= pDev->horizontalStroke;
    int		vStroke = pDev->verticalStroke;
    int		xInsetOrig, yInsetOrig, xInsetCorner, yInsetCorner;
    GC		gc;
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
    boxWidth = sw->slider.elevwidth - 2*hStroke;
    boxHeight = sw->slider.elevheight - 2*vStroke;
    if (horizontal)
    {
	/* outer box size and place */
	boxX = sw->slider.sliderPValue + hStroke;
	boxY = sw->slider.elev_offset + vStroke;;
    }
    else
    {
	/* outer box */
	boxY = sw->slider.sliderPValue + hStroke;
	boxX = sw->slider.elev_offset + vStroke;;
    }

    /* Draw the outer box of the elevator */
    OlgDrawFilledRBox (scr, win, sw->slider.pAttrs, boxX, boxY,
		       boxWidth, boxHeight, ul, ur, ll, lr, FB_UP);

    OlgDrawRBox (scr, win, sw->slider.pAttrs, boxX, boxY,
		 boxWidth, boxHeight, ul, ur, ll, lr, RB_UP);

    /* Highlight a button, if needed */
    gc = OlgIs3d() ?
	OlgGetBg2GC (sw->slider.pAttrs) : OlgGetFgGC (sw->slider.pAttrs);

    if (sw->slider.opcode == DRAG_ELEV)
	flags = SL_DRAG;
    else
	flags = 0;

    /* If there is a conflict between the input focus color and the
     * foreground or background colors (as determined by the calling
     * function), then draw the elevator as if each of the parts is
     * selected.  If a button is really pressed, draw it as if it is
     * not selected.  Strange.
     */

    if (colorConflict)
	flags ^= SL_DRAG;

    if (flags & SL_DRAG)
    {
	XRectangle	insetRect;

	/* inset rectangle */
	insetRect.x = boxX + xInsetOrig;
	insetRect.y = boxY + yInsetOrig;
	insetRect.width = boxWidth - xInsetCorner - xInsetOrig;
	insetRect.height = boxHeight - yInsetCorner - yInsetOrig;

	XFillRectangles (XtDisplay (sw), XtWindow (sw), gc, &insetRect, 1);
	if (OlgIs3d())
	    OlgDrawBox (XtScreen (sw), XtWindow (sw), sw->slider.pAttrs,
			insetRect.x, insetRect.y,
			insetRect.width, insetRect.height, True);
    }
}

/* Calculate position of elevator */

static void
locateElevator OLARGLIST((sw, cablePos, cableLen))
    OLARG( SliderWidget,	sw)
    OLARG( Position,		cablePos)
    OLGRA( Dimension,		cableLen)
{
    Position	elevPos;
    Dimension	elevLen;
    int		userRange;	/* size of user coordinate space */

    if (sw->slider.orientation == OL_HORIZONTAL)
	elevLen = sw->slider.elevwidth;
    else
	elevLen = sw->slider.elevheight;

    /* Find the position of the elevator */
    userRange = sw->slider.sliderMax - sw->slider.sliderMin;
    if (userRange > 0)
    {
	elevPos = muldiv (sw->slider.sliderValue - sw->slider.sliderMin,
			  (int) cableLen, userRange);
    }
    else
	elevPos = 0;

    if (sw->slider.orientation == OL_VERTICAL)
	elevPos = cableLen - elevPos;

    sw->slider.sliderPValue = cablePos + elevPos;
}

/* Draw tick marks */

static void
drawTicks OLARGLIST((sw, from, to))
    OLARG( SliderWidget,	sw)
    OLARG( Position, 		from)
    OLGRA( Position, 		to)
{
    Dimension	stroke, length;
    Position	pos;
    Boolean	horizontal;
    Position	*pTick;
    register	i;

    if (!sw->slider.ticklist)
	return;

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	horizontal = True;
	stroke = OlgGetHorizontalStroke (sw->slider.pAttrs);
	length = 6*OlgGetVerticalStroke (sw->slider.pAttrs);
	pos = sw->slider.elev_offset + sw->slider.elevheight -
	    3*OlgGetVerticalStroke (sw->slider.pAttrs);
    }
    else
    {
	horizontal = False;
	stroke = OlgGetVerticalStroke (sw->slider.pAttrs);
	length = 6*OlgGetHorizontalStroke (sw->slider.pAttrs);
	pos = sw->slider.elev_offset + sw->slider.elevwidth -
	    3*OlgGetHorizontalStroke (sw->slider.pAttrs);
    }

    pTick = sw->slider.ticklist;
    for (i=sw->slider.numticks; i>0; i--)
    {
	if (to < (Position) (*pTick - stroke))
	    break;

	if (from < (Position) (*pTick + stroke))
	{
	    if (horizontal)
		OlgDrawLine (XtScreen (sw), XtWindow (sw), sw->slider.pAttrs,
			     *pTick, pos, length, 2, True);
	    else
		OlgDrawLine (XtScreen (sw), XtWindow (sw), sw->slider.pAttrs,
			     pos, *pTick, length, 2, False);
	}
	pTick++;
    }
}

/* Draw the cable.  The "cable" is area of the slider above/right of the
 * elevator.
 */

static void
drawCable OLARGLIST((sw, from, to, cableOffset, cablePos, cableEnd))
    OLARG( register SliderWidget,	sw)
    OLARG( Position,			from)
    OLARG( Position,			to)
    OLARG( Position,			cableOffset)
    OLARG( Position,			cablePos)
    OLGRA( Position,			cableEnd)
{
    Display	*dpy = XtDisplay (sw);
    Screen	*scr = XtScreen (sw);
    Drawable	win = XtWindow (sw);
    GC		bg, top, bottom;
    _OlgDevice	*pDev = sw->slider.pAttrs->pDev;
    Dimension	cableThickness;

    /* Select GCs */
    if (OlgIs3d ())
    {
	bg = OlgGetBg2GC (sw->slider.pAttrs);
	bottom = OlgGetBrightGC (sw->slider.pAttrs);
	top = OlgGetBg3GC (sw->slider.pAttrs);
    }
    else
    {
	bg = OlgGetBg1GC (sw->slider.pAttrs);
	top = bottom = OlgGetFgGC (sw->slider.pAttrs);
    }

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	cableThickness = pDev->sliderHUL.height + pDev->sliderHLL.height;

	/* If the end doesn't need to be drawn, simply do fill rects to draw
	 * the cable.
	 */
	if (to <= cableEnd - pDev->horizontalStroke - pDev->sliderHUR.width)
	{
	    XFillRectangle (dpy, win, bg, from, cableOffset, to - from,
			    cableThickness);
	    XFillRectangle (dpy, win, top, from, cableOffset, to - from,
			    pDev->verticalStroke);
	    XFillRectangle (dpy, win, bottom, from,
			    cableOffset+cableThickness-pDev->verticalStroke,
			    to - from, pDev->verticalStroke);
	}
	else
	{
	    /* use the curve drawing stuff */
	    to = cableEnd - pDev->horizontalStroke;
	    OlgDrawFilledRBox (scr, win, sw->slider.pAttrs, from, cableOffset,
			       to - from, cableThickness,
			       &pDev->sliderHUL, &pDev->sliderHUR,
			       &pDev->sliderHLL, &pDev->sliderHLR,
			       FB_OMIT_LCAP | (OlgIs3d() ? 0 : FB_UP));
	    OlgDrawRBox (scr, win, sw->slider.pAttrs, from, cableOffset,
			 to - from, cableThickness,
			 &pDev->sliderHUL, &pDev->sliderHUR,
			 &pDev->sliderHLL, &pDev->sliderHLR,
			 RB_OMIT_LCAP);
	}
    }
    else
    {
	cableThickness = pDev->sliderVUL.width + pDev->sliderVUR.width;

	/* If the end doesn't need to be drawn, simply do fill rects to draw
	 * the cable.
	 */
	if (from >= cablePos + pDev->verticalStroke + pDev->sliderVUR.height)
	{
	    XFillRectangle (dpy, win, bg, cableOffset, from, cableThickness,
			    to - from);
	    XFillRectangle (dpy, win, top, cableOffset, from,
			    pDev->horizontalStroke, to - from);
	    XFillRectangle (dpy, win, bottom,
			    cableOffset+cableThickness-pDev->horizontalStroke,
			    from, pDev->horizontalStroke, to - from);
	}
	else
	{
	    /* use the curve drawing stuff */
	    from = cablePos + pDev->verticalStroke;
	    OlgDrawFilledRBox (scr, win, sw->slider.pAttrs, cableOffset, from,
			       cableThickness, to - from,
			       &pDev->sliderVUL, &pDev->sliderVUR,
			       &pDev->sliderVLL, &pDev->sliderVLR,
			       FB_OMIT_BCAP | (OlgIs3d() ? 0 : FB_UP));
	    OlgDrawRBox (scr, win, sw->slider.pAttrs, cableOffset, from,
			 cableThickness, to - from,
			 &pDev->sliderVUL, &pDev->sliderVUR,
			 &pDev->sliderVLL, &pDev->sliderVLR,
			 RB_OMIT_BCAP);
	}
    }

    drawTicks (sw, from, to);
}

/* Draw the indicator.  The "indicator" is filled area of the slider
 * below/left of the elevator.
 */

static void
drawIndicator OLARGLIST((sw, from, to, cableOffset, cablePos, cableEnd))
    OLARG( SliderWidget,	sw)
    OLARG( Position,		from)
    OLARG( Position,		to)
    OLARG( Position,		cableOffset)
    OLARG( Position,		cablePos)
    OLGRA( Position,		cableEnd)
{
    Display	*dpy = XtDisplay((Widget)sw);
    Drawable	win = XtWindow((Widget)sw);
    Screen	*scr = XtScreen((Widget)sw);
    GC		bg;
    _OlgDevice	*pDev = sw->slider.pAttrs->pDev;
    Dimension	cableThickness;

    /* Select GCs */
    if (OlgIs3d ())
	bg = OlgGetBg3GC (sw->slider.pAttrs);
    else
	bg = OlgGetFgGC (sw->slider.pAttrs);

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	cableThickness = pDev->sliderHUL.height + pDev->sliderHLL.height;

	/* If the end doesn't need to be drawn, simply do fill rects to draw
	 * the cable.
	 */
	if (from >= cablePos + pDev->horizontalStroke + pDev->sliderHUL.width)
	{
	    XFillRectangle (dpy, win, bg, from, cableOffset, to - from,
			    cableThickness);
	    if (OlgIs3d ())
		XFillRectangle (dpy, win, OlgGetBg2GC (sw->slider.pAttrs),
				from, cableOffset + pDev->horizontalStroke,
				to - from, pDev->horizontalStroke);
	}
	else
	{
	    GC	save;

	    /* use the curve drawing stuff.  Temporarily change the bg2 gc
	     * to a darker color.
	     */
	    from = cablePos + pDev->horizontalStroke;
	    save = sw->slider.pAttrs->bg2;
	    sw->slider.pAttrs->bg2 = bg;
	    OlgDrawFilledRBox (scr, win, sw->slider.pAttrs, from, cableOffset,
			       to - from, cableThickness,
			       &pDev->sliderHUL, &pDev->sliderHUR,
			       &pDev->sliderHLL, &pDev->sliderHLR,
			       FB_OMIT_RCAP);
	    sw->slider.pAttrs->bg2 = save;

	    if (OlgIs3d ())
	    {
		from += pDev->sliderHUL.width;
		if (to > from)
		    XFillRectangle (dpy, win, OlgGetBg2GC (sw->slider.pAttrs),
				    from, cableOffset + pDev->horizontalStroke,
				    to - from, pDev->horizontalStroke);
	    }
	}
    }
    else
    {
	cableThickness = pDev->sliderVUL.width + pDev->sliderVUR.width;

	/* If the end doesn't need to be drawn, simply do fill rects to draw
	 * the cable.
	 */
	if (to <= cableEnd - pDev->verticalStroke - pDev->sliderVLR.height)
	{
	    XFillRectangle (dpy, win, bg, cableOffset, from, cableThickness,
			    to - from);
	    if (OlgIs3d ())
		XFillRectangle (dpy, win, OlgGetBg2GC (sw->slider.pAttrs),
				cableOffset + pDev->verticalStroke, from,
				pDev->verticalStroke, to - from);
	}
	else
	{
	    GC	save;

	    /* use the curve drawing stuff */
	    to = cableEnd - pDev->verticalStroke;
	    save = sw->slider.pAttrs->bg2;
	    sw->slider.pAttrs->bg2 = bg;
	    OlgDrawFilledRBox (scr, win, sw->slider.pAttrs, cableOffset, from,
			       cableThickness, to - from,
			       &pDev->sliderVUL, &pDev->sliderVUR,
			       &pDev->sliderVLL, &pDev->sliderVLR,
			       FB_OMIT_TCAP);
	    sw->slider.pAttrs->bg2 = save;

	    if (OlgIs3d ())
	    {
		to -= pDev->sliderVLR.height;
		if (to > from)
		    XFillRectangle (dpy, win, OlgGetBg2GC (sw->slider.pAttrs),
				    cableOffset + pDev->verticalStroke, from,
				    pDev->verticalStroke, to - from);
	    }
	}
    }

    drawTicks (sw, from, to);
}

void
_OloOlgDrawSlider OLARGLIST((w, pInfo))
    OLARG( register Widget,	w)
    OLGRA( OlgAttrs *,		pInfo)
{
    SliderWidget	sw = (SliderWidget)w;
    Display	*dpy = XtDisplay(w);
    Screen	*scr = XtScreen(w);
    Drawable	win = XtWindow(w);
    unsigned	horizontal = (sw->slider.orientation == OL_HORIZONTAL);
    Position	cablePos;	/* screen coordinate of top/left of cable */
    Dimension	cableLen;	/* length of cable area */
    Dimension	elevLen;	/* length of elevator */
    Dimension	padLen;		/* length of gap between elements */
    GC		gc;		/* GC to use in drawing */
    _OlgDevice	*pDev = pInfo->pDev;
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;
    OlFontList	* font_list;	/* to draw i18n string */

    focus_color = sw->primitive.input_focus_color;
    has_focus = sw->primitive.has_focus;
    font_list = sw->primitive.font_list;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d () && has_focus &&
	(sw->primitive.foreground == focus_color ||
	 sw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    if (horizontal)
    {
	cablePos = sw->slider.leftPad;
	cableLen = sw->core.width - cablePos - sw->slider.rightPad;
	padLen = pDev->horizontalStroke;
	elevLen = sw->slider.elevwidth;
    }
    else
    {
	cablePos = sw->slider.topPad;
	cableLen = sw->core.height - cablePos - sw->slider.bottomPad;
	padLen = pDev->verticalStroke;
	elevLen = sw->slider.elevheight;
    }

    /* If the slider has anchors, draw them. */
    if (sw->slider.endBoxes)
    {
	Position	anchOffset;

	if (horizontal)
	{
	    anchOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevheight - sw->slider.anchlen) / 2;
	    OlgDrawAnchor (scr, win, pInfo, cablePos, anchOffset,
			   sw->slider.anchwidth - 2*padLen, sw->slider.anchlen,
			   (sw->slider.opcode == ANCHOR_TOP) ^ colorConflict);
	    OlgDrawAnchor (scr, win, pInfo,
			   cablePos+cableLen - sw->slider.anchwidth + 2*padLen,
			   anchOffset,
			   sw->slider.anchwidth - 2*padLen, sw->slider.anchlen,
			   (sw->slider.opcode == ANCHOR_BOT) ^ colorConflict);
	    cableLen -= sw->slider.anchwidth * 2;
	    cablePos += sw->slider.anchwidth;
	}
	else
	{
	    anchOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevwidth - sw->slider.anchwidth) / 2;
	    OlgDrawAnchor (scr, win, pInfo, anchOffset, cablePos,
			   sw->slider.anchwidth, sw->slider.anchlen - 2*padLen,
			   (sw->slider.opcode == ANCHOR_TOP) ^ colorConflict);
	    OlgDrawAnchor (scr, win, pInfo,
			   anchOffset,
			   sw->core.height - sw->slider.anchlen + 2*padLen,
			   sw->slider.anchwidth, sw->slider.anchlen - 2*padLen,
			   (sw->slider.opcode == ANCHOR_BOT) ^ colorConflict);
	    cableLen -= sw->slider.anchlen * 2;
	    cablePos += sw->slider.anchlen;
	}
    }

    /* Find where to draw the elevator, but wait until the cable is there
     * before drawing it.
     */
    if (XtClass(sw) == sliderWidgetClass)
	locateElevator (sw, cablePos, cableLen - elevLen);
    else
    {
	if (horizontal)
	    locateElevator (sw, sw->slider.minTickPos,
			    sw->slider.maxTickPos - sw->slider.minTickPos);
	else
	    locateElevator (sw, sw->slider.maxTickPos,
			    sw->slider.minTickPos - sw->slider.maxTickPos);
	elevLen = padLen = 0;
    }

    /* Draw the cable */
    if (sw->slider.type == SB_REGULAR)
    {
	Position	cableOffset;

	if (horizontal)
	{
	    cableOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevheight -
		 (pDev->sliderHUL.height + pDev->sliderHLL.height)) / 2;

	    /* Create rectangles for indicator to the left of the elevator
	     * and cable to the right
	     */
	    drawIndicator (sw, cablePos, sw->slider.sliderPValue, cableOffset,
			   cablePos, cablePos + cableLen);
	    drawCable (sw, sw->slider.sliderPValue,
		       cablePos + cableLen, cableOffset, cablePos,
		       cablePos + cableLen);
	}
	else
	{
	    cableOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevwidth -
		 (pDev->sliderVUL.width + pDev->sliderVUR.width)) / 2;

	    /* Create rectangles for indicator below the elevator
	     * and cable above.
	     */
	    drawCable (sw, cablePos, sw->slider.sliderPValue, cableOffset,
			   cablePos, cablePos + cableLen);
	    drawIndicator (sw, sw->slider.sliderPValue,
			   cablePos + cableLen, cableOffset,
			   cablePos, cablePos + cableLen);
	}
    }

    /* Draw the elevator */
    if (XtClass(sw) == sliderWidgetClass)
    {
	if (horizontal)
	    XClearArea (dpy, win,
			sw->slider.sliderPValue, sw->slider.elev_offset,
			sw->slider.elevwidth, sw->slider.elevheight, False);
	else
	    XClearArea (dpy, win,
			sw->slider.elev_offset, sw->slider.sliderPValue,
			sw->slider.elevwidth, sw->slider.elevheight, False);
	drawElevator (sw, colorConflict);
    }

    /* Draw the labels, if any */
    if (sw->slider.minLabel || sw->slider.maxLabel)
    {
	XFontStruct	*font = sw->primitive.font;
	Position	x, y;
	Dimension	minWidth, minHeight;
	Dimension	stroke;
	int		len;

	OlgSizeSlider (w, sw->slider.pAttrs, &minWidth, &minHeight);

	if (sw->slider.minLabel)
	{
	    len = strlen (sw->slider.minLabel);

	    if (horizontal)
	    {
		stroke = pDev->verticalStroke;

		y = sw->slider.elev_offset + minHeight +
		    OlFontAscent(font, font_list);
		x = sw->slider.minTickPos - ((font_list == NULL) ?
			XTextWidth (font, sw->slider.minLabel, len) :
			OlTextWidth (font_list, (unsigned char *)
				     sw->slider.minLabel, len)) / 2;

		if (x < 0)
		  x = 0;
	    }
	    else
	    {
		stroke = pDev->horizontalStroke;
		x = sw->slider.elev_offset + minWidth;
		y = sw->slider.minTickPos + OlFontAscent(font, font_list) / 2;
	    }

	    if (font_list == NULL)
	        XDrawString (dpy, win, sw->slider.labelGC, x, y,
			     sw->slider.minLabel, len);
	    else
	        OlDrawString (dpy, win, font_list, sw->slider.labelGC, x, y,
			     (unsigned char *)sw->slider.minLabel, len);
	}

	if (sw->slider.maxLabel)
	{
	    len = strlen (sw->slider.maxLabel);

	    if (horizontal)
	    {
		stroke = pDev->verticalStroke;

		y = sw->slider.elev_offset + minHeight +
		    OlFontAscent(font, font_list);
		x = sw->slider.maxTickPos - ((font_list == NULL) ?
			XTextWidth (font, sw->slider.minLabel, len) :
			OlTextWidth (font_list, (unsigned char *)
				     sw->slider.minLabel, len)) / 2;
		if (x < 0)
		    x = 0;
	    }
	    else
	    {
		stroke = pDev->horizontalStroke;
		x = sw->slider.elev_offset + minWidth;
		if (sw->slider.tickUnit != OL_NONE)
		    x += stroke*2;

		y = sw->slider.maxTickPos + OlFontAscent(font, font_list) / 2;
	    }

	    if (font_list == NULL)
	       XDrawString (dpy, win, sw->slider.labelGC, x, y,
			    sw->slider.maxLabel, len);
	    else
	       OlDrawString (dpy, win, font_list, sw->slider.labelGC, x, y,
			     (unsigned char *)sw->slider.maxLabel, len);
	}
    }
}

void
_OloOlgUpdateSlider OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
    SliderWidget	sw = (SliderWidget)w;
    unsigned	horizontal = (sw->slider.orientation == OL_HORIZONTAL);
    Display	*dpy = XtDisplay(w);
    Window	win = XtWindow(w);
    Screen	*scr = XtScreen(w);
    Position	cablePos;	/* screen coordinate of top/left of cable */
    Dimension	cableLen;	/* length of cable area */
    Dimension	elevLen;	/* length of elevator */
    Dimension	padLen;		/* length of gap between elements */
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;
    _OlgDevice	*pDev = pInfo->pDev;

    focus_color = sw->primitive.input_focus_color;
    has_focus = sw->primitive.has_focus;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d () && has_focus &&
	(sw->primitive.foreground == focus_color ||
	 sw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    if (horizontal)
    {
	cablePos = sw->slider.leftPad;
	cableLen = sw->core.width - cablePos - sw->slider.rightPad;
	padLen = pDev->horizontalStroke;
	elevLen = sw->slider.elevwidth;
    }
    else
    {
        cablePos = sw->slider.topPad;
	cableLen = sw->core.height - cablePos - sw->slider.bottomPad;
	padLen = pDev->verticalStroke;
	elevLen = sw->slider.elevheight;
    }

    /* Update anchors */
    if (sw->slider.endBoxes)
    {
	Position	anchOffset;

	if (horizontal)
	{
	    anchOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevheight - sw->slider.anchlen) / 2;
	    if (flags & SL_BEGIN_ANCHOR)
	    {
		OlgDrawAnchor (scr, win, pInfo, cablePos, anchOffset,
			       sw->slider.anchwidth - 2*padLen,
			       sw->slider.anchlen,
			       (sw->slider.opcode==ANCHOR_TOP)^colorConflict);
	    }

	    if (flags & SL_END_ANCHOR)
	    {
		OlgDrawAnchor (scr, win, pInfo,
			       cablePos+cableLen-sw->slider.anchwidth+2*padLen,
			       anchOffset,
			       sw->slider.anchwidth - 2*padLen,
			       sw->slider.anchlen,
			       (sw->slider.opcode==ANCHOR_BOT)^colorConflict);
	    }
	    cableLen -= sw->slider.anchwidth * 2;
	    cablePos += sw->slider.anchwidth;
	}
	else
	{
	    anchOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevwidth - sw->slider.anchwidth) / 2;
	    if (flags & SL_BEGIN_ANCHOR)
	    {
		OlgDrawAnchor (scr, win, pInfo, anchOffset, cablePos,
			       sw->slider.anchwidth,
			       sw->slider.anchlen - 2*padLen,
			       (sw->slider.opcode==ANCHOR_TOP)^colorConflict);
	    }

	    if (flags & SL_END_ANCHOR)
	    {
		OlgDrawAnchor (scr, win, pInfo,
			       anchOffset,
			       sw->core.height - sw->slider.anchlen + 2*padLen,
			       sw->slider.anchwidth,
			       sw->slider.anchlen - 2*padLen,
			       (sw->slider.opcode==ANCHOR_BOT)^colorConflict);
	    }
	    cableLen -= sw->slider.anchlen * 2;
	    cablePos += sw->slider.anchlen;
	}
    }

    /* If we are only updating an anchor, we're done */
    if (!(flags & ~(SL_BEGIN_ANCHOR | SL_END_ANCHOR)))
	return;

    /* Update the position. */
    if (flags & SL_POSITION)
    {
	Position	oldElevPos;
	Position	elevPos;
	Position	cableOffset;
	Position	cableEnd;

	/* Save the old elevator position */
	oldElevPos = sw->slider.sliderPValue;

	/* Clear the elevator (if any) from its old position and find
	 * the new position.
	 */
	if (XtClass(sw) == sliderWidgetClass)
	{
	    if (horizontal)
	    {
		XClearArea (dpy, win, oldElevPos, sw->slider.elev_offset,
			    sw->slider.elevwidth, sw->slider.elevheight,
			    False);
	    }
	    else
	    {
		XClearArea (dpy, win, sw->slider.elev_offset, oldElevPos,
			    sw->slider.elevwidth, sw->slider.elevheight,
			    False);
	    }
	    locateElevator (sw, cablePos, cableLen - elevLen);
	}
	else
	{
	    if (horizontal)
		locateElevator (sw, sw->slider.minTickPos,
				sw->slider.maxTickPos - sw->slider.minTickPos);
	    else
		locateElevator (sw, sw->slider.maxTickPos,
				sw->slider.minTickPos - sw->slider.maxTickPos);
	    elevLen = 0;
	}
	elevPos = sw->slider.sliderPValue;
	cableEnd = cablePos + cableLen;

	/* Patch the cable as needed. */
	if (horizontal)
	{
	    cableOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevheight -
		 (pDev->sliderHUL.height + pDev->sliderHLL.height)) / 2;

	    if (oldElevPos < elevPos)
		drawIndicator (sw, oldElevPos, elevPos, cableOffset, cablePos,
			       cableEnd);
	    else
		drawCable (sw, elevPos + elevLen, oldElevPos + elevLen,
			   cableOffset, cablePos, cableEnd);
	}
	else
	{
	    cableOffset = sw->slider.elev_offset +
		(Position) (sw->slider.elevwidth -
		 (pDev->sliderVUL.width + pDev->sliderVUR.width)) / 2;

	    if (oldElevPos < elevPos)
		drawCable (sw, oldElevPos, elevPos, cableOffset, cablePos,
			   cableEnd);
	    else
		drawIndicator (sw, elevPos + elevLen, oldElevPos + elevLen,
			       cableOffset, cablePos, cableEnd);
	}
    }

    /* Clear the cable at the new elevator position and draw the elevator. */
    if (XtClass(sw) == sliderWidgetClass)
    {
	if (horizontal)
	{
	    XClearArea (dpy, win, sw->slider.sliderPValue,
			sw->slider.elev_offset,
			sw->slider.elevwidth, sw->slider.elevheight,
			False);
	}
	else
	{
	    XClearArea (dpy, win, sw->slider.elev_offset,
			sw->slider.sliderPValue,
			sw->slider.elevwidth, sw->slider.elevheight,
			False);
	}

	drawElevator (sw, colorConflict);
    }
}

/* Determine the size of a slider anchor.  This is only correct for
 * the 12 point scale.  The anchor contains two stroke widths of pad
 * in the direction of slider motion.
 */
/*
void
OlgSizeSliderAnchor OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    SliderWidget	sw = (SliderWidget)w;

    if (sw->slider.endBoxes)
    {
	if (sw->slider.orientation == OL_HORIZONTAL)
	{
	    *pWidth=OlScreenPointToPixel(OL_HORIZONTAL, 6, pInfo->pDev->scr) +
		2*pInfo->pDev->horizontalStroke;
	    *pHeight=OlScreenPointToPixel(OL_VERTICAL, 11, pInfo->pDev->scr);
	}
	else
	{
	    *pHeight=OlScreenPointToPixel(OL_VERTICAL, 6, pInfo->pDev->scr) +
		2*pInfo->pDev->verticalStroke;
	    *pWidth=OlScreenPointToPixel(OL_HORIZONTAL, 11, pInfo->pDev->scr);
	}
    }
    else
	*pWidth = *pHeight = 0;
}
*/

/* Determine the size of a slider elevator.  This is only correct for
 * the 12 point scale.  The elevator is padded all around by one stroke width.
 */

void
_OloOlgSizeSliderElevator OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    SliderWidget	sw = (SliderWidget)w;

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, OlgIs3d () ? 10 : 11,
				       pInfo->pDev->scr);
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, OlgIs3d () ? 15 : 16,
					pInfo->pDev->scr);
    }
    else
    {
	*pWidth = OlScreenPointToPixel(OL_HORIZONTAL, OlgIs3d () ? 15 : 16,
				       pInfo->pDev->scr);
	*pHeight = OlScreenPointToPixel(OL_VERTICAL, OlgIs3d () ? 10 : 11,
					pInfo->pDev->scr);
    }

    *pHeight += pInfo->pDev->verticalStroke * 2;
    *pWidth += pInfo->pDev->horizontalStroke * 2;
}

void
_OloOlgSizeSlider OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    SliderWidget	sw = (SliderWidget)w;
    Dimension	anchorWidth, anchorHeight;
    Dimension	elevWidth, elevHeight;
    Dimension	extent;

    OlgSizeSliderAnchor (w, pInfo, &anchorWidth, &anchorHeight);
    OlgSizeSliderElevator (w, pInfo, &elevWidth, &elevHeight);

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	extent = elevWidth + anchorWidth * 2;

	*pWidth = _OlMax (sw->core.width, extent);
	*pHeight = elevHeight;
	if (sw->slider.tickUnit != OL_NONE)
	    *pHeight += OlgGetVerticalStroke (pInfo)*2;
	if (sw->slider.minLabel || sw->slider.maxLabel)
	    *pHeight += OlgGetVerticalStroke (pInfo)*3;
    }
    else
    {
	extent = elevHeight + anchorHeight * 2;

	*pWidth = elevWidth;
	*pHeight = _OlMax (sw->core.height, extent);
	if (sw->slider.tickUnit != OL_NONE)
	    *pWidth += OlgGetHorizontalStroke (pInfo)*2;
	if (sw->slider.minLabel || sw->slider.maxLabel)
	    *pWidth += OlgGetHorizontalStroke (pInfo)*3;
    }
}
