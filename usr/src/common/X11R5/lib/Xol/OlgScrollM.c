#ifndef NOIDENT
#ident	"@(#)olg:OlgScrollM.c	1.11"
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

static void locateElevator OL_ARGS((ScrollbarWidget));

/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));


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

void
_OlmOlgDrawScrollbar OLARGLIST((w, pInfo))
    OLARG(Widget,	w)
    OLGRA(OlgAttrs *,	pInfo)
{
    ScrollbarWidget	sbw = (ScrollbarWidget)w;
    Screen	*scr = XtScreen(w);
    Drawable	win = XtWindow(w);
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    Dimension	shadowThickness;

    shadowThickness = sbw->primitive.shadow_thickness;

    /* Draw the indented shadow box inside of the scrollbar's window. */
    if (horizontal)
        OlgDrawBorderShadow(XtScreen(sbw), XtWindow(sbw), pInfo,
	    OL_SHADOW_IN, 2,
	    0,
	    sbw->scroll.offset - shadowThickness,
	    sbw->core.width,
	    sbw->scroll.anchlen + 2 * shadowThickness);
    else
        OlgDrawBorderShadow(XtScreen(sbw), XtWindow(sbw), pInfo,
	    OL_SHADOW_IN, 2,
	    sbw->scroll.offset - shadowThickness,
	    0,
	    sbw->scroll.anchwidth + 2 * shadowThickness,
	    sbw->core.height);

    /* Draw the increment/decrement arrows.  */
    if (sbw->scroll.type != SB_MINIMUM)
    {
	if (horizontal)
	{
	    OlgDrawArrow (scr, win, pInfo, sbw->primitive.shadow_thickness,
			sbw->scroll.offset,
			sbw->scroll.anchwidth, sbw->scroll.anchlen,
			((sbw->scroll.opcode == GRAN_DEC) | AR_LEFT));
	    OlgDrawArrow (scr, win, pInfo,
			sbw->core.width - sbw->scroll.anchwidth -
			sbw->primitive.shadow_thickness,
			sbw->scroll.offset,
			sbw->scroll.anchwidth, sbw->scroll.anchlen,
			((sbw->scroll.opcode == GRAN_INC) | AR_RIGHT));
	}
	else
	{
	    OlgDrawArrow (scr, win, pInfo, sbw->scroll.offset, 
			sbw->primitive.shadow_thickness,
			sbw->scroll.anchwidth, sbw->scroll.anchlen,
			((sbw->scroll.opcode == GRAN_DEC) | AR_UP));
	    OlgDrawArrow (scr, win, pInfo,
			sbw->scroll.offset,
			sbw->core.height - sbw->scroll.anchlen -
			sbw->primitive.shadow_thickness,
			sbw->scroll.anchwidth, sbw->scroll.anchlen,
			((sbw->scroll.opcode == GRAN_INC) | AR_DOWN));
	}
    }

    /* Get the position of the elevator and indicator */
    locateElevator (sbw);

    /* Draw the elevator */
    OlgDrawBorderShadow(XtScreen(sbw), XtWindow(sbw), pInfo,
	OL_SHADOW_OUT, 2,
	(horizontal ? sbw->scroll.sliderPValue : sbw->scroll.offset),
	(horizontal ? sbw->scroll.offset : sbw->scroll.sliderPValue),
	sbw->scroll.elevwidth,
	sbw->scroll.elevheight);

}

static void
locateElevator OLARGLIST((sbw))
    OLGRA( ScrollbarWidget,	sbw)
{
    Position	elevPos;
    int		userRange;	/* size of user coordinate space */
    Position	cablePos;
    Dimension	cableLen;
    Dimension	padLen;		/* length of gap between elements */
    Dimension	shadowThickness;
    _OlgDevice	*pDev = sbw->scroll.pAttrs->pDev;
    int slider_max;

    shadowThickness = sbw->primitive.shadow_thickness;

    /* Calculate the length and offset of the cable area */
    if (sbw->scroll.orientation == OL_HORIZONTAL) {
	padLen = pDev->horizontalStroke;
	cablePos = sbw->scroll.anchwidth + shadowThickness + padLen;
	cableLen = sbw->core.width - (cablePos * 2);
	slider_max = sbw->core.width - cablePos - sbw->scroll.elevwidth;
	sbw->scroll.indLen = (2*sbw->primitive.shadow_thickness);
	sbw->scroll.indPos = sbw->scroll.anchwidth + shadowThickness;
    }
    else {
	padLen = pDev->verticalStroke;
	cablePos = sbw->scroll.anchlen + shadowThickness + padLen;
	cableLen = sbw->core.height - (cablePos * 2);
	slider_max = sbw->core.height - cablePos - sbw->scroll.elevheight;
	sbw->scroll.indPos = (2*sbw->primitive.shadow_thickness);
	sbw->scroll.indLen = sbw->scroll.anchlen + shadowThickness;
    }

    /* Find the position of the elevator */
    userRange = sbw->scroll.sliderMax - sbw->scroll.sliderMin;
    if (sbw->scroll.type != SB_REGULAR)
    {
	/* center the elevator */
	elevPos = (Position) (cableLen) / 2;
    }
    else
    {
	if (userRange != sbw->scroll.proportionLength)
	{
	    elevPos = muldiv (sbw->scroll.sliderValue - sbw->scroll.sliderMin,
			      (int) cableLen, userRange);
	}
	else
	    elevPos = 0;
    }

    sbw->scroll.sliderPValue = cablePos + elevPos;
    if (sbw->scroll.sliderPValue > slider_max)
	sbw->scroll.sliderPValue = slider_max;
} /* locateElevator */

void
_OlmOlgUpdateScrollbar OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
    ScrollbarWidget	sbw = (ScrollbarWidget)w;
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);

    /* Update anchors */
    if (sbw->scroll.type != SB_MINIMUM)
    {
	if (flags & SB_PREV_ARROW)
	{
	    if (horizontal)
	    {
		OlgDrawArrow (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->primitive.shadow_thickness,
			    sbw->scroll.offset,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen,
			    ((sbw->scroll.opcode != NOOP) | AR_LEFT));
	    }
	    else
	    {
		OlgDrawArrow (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->scroll.offset,
			    sbw->primitive.shadow_thickness,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen,
			    ((sbw->scroll.opcode != NOOP) | AR_UP));
	    }
	}

	if (flags & SB_NEXT_ARROW)
	{
	    if (horizontal)
	    {
		OlgDrawArrow (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->core.width - sbw->scroll.anchwidth -
				sbw->primitive.shadow_thickness,
			    sbw->scroll.offset,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen,
			    ((sbw->scroll.opcode != NOOP) | AR_RIGHT));
	    }
	    else
	    {
		OlgDrawArrow (XtScreen (sbw), XtWindow (sbw), pInfo,
			    sbw->scroll.offset,
			    sbw->core.height - sbw->scroll.anchlen -
				sbw->primitive.shadow_thickness,
			    sbw->scroll.anchwidth,
			    sbw->scroll.anchlen,
			    ((sbw->scroll.opcode != NOOP) | AR_DOWN));
	    }
	}
    }

    /* If we are only updating an anchor, we're done */
    if (!(flags & ~(SB_BEGIN_ANCHOR | SB_END_ANCHOR)))
	return;

    /* Update the position.  Remember, only regular scrollbar can move */
    if (flags & SB_POSITION && sbw->scroll.type == SB_REGULAR)
    {
	Position	oldElevX;
	Position	oldElevY;
	Position	elevX;
	Position	elevY;
	Dimension	elevWidth;
	Dimension	elevHeight;
	Dimension	shadowThickness;

	elevWidth = sbw->scroll.elevwidth;
	elevHeight = sbw->scroll.elevheight;
	shadowThickness = sbw->primitive.shadow_thickness;

	/* Save the old indicator and elevator positions */
	oldElevX = horizontal ? sbw->scroll.sliderPValue : sbw->scroll.offset;
	oldElevY = horizontal ? sbw->scroll.offset : sbw->scroll.sliderPValue;

	/* Get the new position of the elevator and indicator */
	locateElevator (sbw);

	elevX = horizontal ? sbw->scroll.sliderPValue : sbw->scroll.offset;
	elevY = horizontal ? sbw->scroll.offset : sbw->scroll.sliderPValue;

	/* Find the rectangles that must be repainted.  To minimize flashing,
	 * clear as little as possible.  Three rectangle lists are produced:
	 * a list of areas to clear, a list of areas to stipple, and a list
	 * of areas to fill.  Start with the area above/left of the elevator.
	 */
	if (horizontal)  {
		if (oldElevX < elevX)  {
			/*  Move to the right  */
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX, oldElevY,
				elevX - oldElevX,
				elevHeight,
				False);
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX + elevWidth - shadowThickness,
				oldElevY,
				shadowThickness,
				elevHeight,
				False);
		}
		else if (oldElevX != elevX)  {
			/*  Move to the left  */
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				elevX + elevWidth, oldElevY,
				oldElevX - elevX,
				elevHeight,
				False);
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX, oldElevY,
				shadowThickness,
				elevHeight,
				False);
		}
	}
	else  {  /* vertical  */
		if (oldElevY < elevY)  {
			/*  Move down  */
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX, oldElevY,
				elevWidth,
				elevY - oldElevY,
				False);
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX,
				oldElevY + elevHeight - shadowThickness,
				elevWidth,
				shadowThickness,
				False);
		}
		else if (oldElevY != elevY)  {
			/*  Move up */
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				oldElevX, oldElevY,
				elevWidth,
				shadowThickness,
				False);
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				elevX, elevY + elevHeight,
				elevWidth,
				oldElevY - elevY,
				False);
		}
	}

	/* Display the cable and elevator.  If the new elevator position
	 * overlaps the old one, clear the area of the overlap.
	 */
    	OlgDrawBorderShadow(XtScreen(sbw), XtWindow(sbw), pInfo, OL_SHADOW_OUT,
		2, elevX, elevY, elevWidth, elevHeight);
    }
    else
    {
	/* Update elevator */
	if (flags & (SB_PREV_ARROW | SB_NEXT_ARROW | SB_DRAG))  {
		XClearArea(XtDisplay(sbw), XtWindow(sbw),
			(horizontal ? sbw->scroll.sliderPValue
				: sbw->scroll.offset),
			(horizontal ? sbw->scroll.offset
				: sbw->scroll.sliderPValue),
			sbw->scroll.elevwidth,
			sbw->scroll.elevheight,
			False);

    		OlgDrawBorderShadow(XtScreen(sbw), XtWindow(sbw), pInfo,
			OL_SHADOW_OUT, 2,
			(horizontal ? sbw->scroll.sliderPValue
				: sbw->scroll.offset),
			(horizontal ? sbw->scroll.offset
				: sbw->scroll.sliderPValue),
			sbw->scroll.elevwidth, sbw->scroll.elevheight);
	}
    }
}

/* Determine the size of a scroll bar anchor.  This is only correct for
 * the 12 point scale.
 */

void
_OlmOlgSizeScrollbarAnchor OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    ScrollbarWidget	sbw = (ScrollbarWidget)w;

    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
        *pWidth = pInfo->pDev->arrowRightCenter.width;
        *pHeight = pInfo->pDev->arrowRightCenter.height;
    }
    else
    {
        *pWidth = pInfo->pDev->arrowUpCenter.width;
        *pHeight = pInfo->pDev->arrowUpCenter.height;
    }
}

/* Determine the size of a scroll bar elevator.  This is only correct for
 * the 12 point scale.
 */

void
_OlmOlgSizeScrollbarElevator OLARGLIST((w, pInfo, type, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( OlDefine,	type)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    ScrollbarWidget	sbw = (ScrollbarWidget)w;
 
    if (sbw->scroll.orientation == OL_HORIZONTAL)
    {
	int minWidth = sbw->primitive.shadow_thickness * 2 +
			pInfo->pDev->verticalStroke * 2;
	int avail_width = sbw->core.width - (sbw->scroll.anchwidth*2)-minWidth;

	*pHeight = sbw->scroll.anchlen;
    	if (type == SB_MINIMUM)  {
	    *pWidth = 1;
	}
	else if (type == SB_ABBREVIATED)  {
	    *pWidth = minWidth;
	}
	else  {
	    *pWidth = muldiv(sbw->scroll.proportionLength,
	        avail_width,
		sbw->scroll.sliderMax - sbw->scroll.sliderMin);
	    if ((int)(*pWidth) < minWidth) {
		if ((int)*pWidth < avail_width)
		    *pWidth = minWidth;
		else
		    *pWidth = 1;
	    }
	}
    }
    else
    {
	int minHeight = sbw->primitive.shadow_thickness * 2 +
		pInfo->pDev->horizontalStroke * 2;
	int avail_height = sbw->core.height- (sbw->scroll.anchlen*2)-minHeight;

	*pWidth = sbw->scroll.anchwidth;
    	if (type == SB_MINIMUM)  {
	    *pHeight = 1;
	}
	else if (type == SB_ABBREVIATED)  {
	    *pHeight = minHeight;
	}
	else  {
	    *pHeight = muldiv(sbw->scroll.proportionLength,
	        avail_height,
		sbw->scroll.sliderMax - sbw->scroll.sliderMin);
	    if ((int)(*pHeight) < minHeight) {
		if (minHeight < avail_height)
		    *pHeight = minHeight;
		else
		    *pHeight = 1;
	    }
	}
    }
}
