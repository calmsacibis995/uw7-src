#ifndef NOIDENT
#ident	"@(#)olg:OlgSliderM.c	1.9"
#endif

/* OlgSliderM.c: Slider functions (Motif mode) */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/SliderP.h>

/* these 2 are static to this file... */
static void DrawSliderElevator OL_ARGS((SliderWidget));
static void DrawSliderValue OL_ARGS((SliderWidget));

/* These 4 are used externally - put in  _OlResolveOlgGUISymbol() */

static void DrawFocusHighlights OL_ARGS((Widget, OlDefine));

#define	HORIZ(W)	((W)->slider.orientation == OL_HORIZONTAL)
#define	VERT(W)		((W)->slider.orientation == OL_VERTICAL)

#define	SLD		sw->slider

/* add the 2 * border_width as an extra pad  - this is UNRELATED to the
 * drawing of the border
 */

#define	SCALEWIDTH_DEFAULT(screen, borderwidth, direction) \
		OlScreenPointToPixel(direction, 15+2*(int)borderwidth, screen)
#define	ELEVATOR_HEIGHT_CONSTANT(screen, borderwidth, direction) \
		OlScreenPointToPixel(direction, 30+2*(int)borderwidth, screen)
#define	SCALEHEIGHT_DEFAULT(screen, borderwidth, direction) \
		OlScreenPointToPixel(direction, 100+2*(int)borderwidth, screen)

#define NOPAD	0
#define ALLPAD	1
#define TPAD	2
#define BPAD	3
#define LPAD	4
#define RPAD	5


/* OlgSizeSlider() : The width and/or height of the slider when sizing
 * it here includes:
 *	- the space for the border that must be highlighted when it gets
 *		focus;
 *	- the space for the shadow_thickness;
 *	- the space for the elevator.
 */
void
_OlmOlgSizeSlider OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs  *, pInfo)
    OLARG( Dimension *, pWidth)
    OLGRA( Dimension *,	pHeight)
{
    SliderWidget	sw = (SliderWidget)w;
    int		default_width, default_height;
    Dimension	elevWidth, elevHeight;

    OlgSizeSliderElevator(w, pInfo, &elevWidth, &elevHeight);

    if (sw->slider.orientation == OL_HORIZONTAL)
    {

	default_width = (int)(SCALEHEIGHT_DEFAULT(XtScreen((Widget)sw),
				sw->primitive.highlight_thickness,
				 OL_HORIZONTAL));
	default_height = (int)(elevHeight +
		 2*sw->primitive.shadow_thickness +
		 2 * sw->primitive.highlight_thickness);
        *pWidth = _OlMax ((int)sw->core.width, (int)default_width);
        *pHeight = default_height;
    }
    else { /* vertical */
	default_width = elevWidth +
		2*sw->primitive.shadow_thickness +
		 2 * sw->primitive.highlight_thickness;
	default_height = (int)SCALEHEIGHT_DEFAULT(XtScreen(sw),
				sw->primitive.highlight_thickness,
				 OL_VERTICAL);
	
        *pWidth = default_width;
        *pHeight = _OlMax((int)sw->core.height, (int)default_height);
    }
}

void
_OlmOlgSizeSliderElevator OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *,	pHeight)
{
    SliderWidget	sw = (SliderWidget)w;

    if (sw->slider.orientation == OL_HORIZONTAL)
    {
	*pWidth = (Dimension)(ELEVATOR_HEIGHT_CONSTANT(XtScreen(sw),
		sw->primitive.highlight_thickness, OL_HORIZONTAL));
	*pHeight = (Dimension)(SCALEWIDTH_DEFAULT(XtScreen(sw),
		sw->primitive.highlight_thickness, OL_VERTICAL) - 
		2 * (int)sw->primitive.shadow_thickness);
    }
    else { /* vertical */
	/* This is saying that the width of the elevator is the width of
	 * the slider - the shadow that surrounds the slider
	 */
	*pWidth = (Dimension)(SCALEWIDTH_DEFAULT(XtScreen(sw),
		sw->primitive.highlight_thickness, OL_HORIZONTAL) -
		2 * (int)sw->primitive.shadow_thickness);
	*pHeight = (Dimension)(ELEVATOR_HEIGHT_CONSTANT(XtScreen(sw),
		sw->primitive.highlight_thickness, OL_VERTICAL));
    }
}

/* OlgUpdateSlider(). 
 *
 * Draw (refresh) only the parts of the slider that
 * you need to.
 */

void
_OlmOlgUpdateSlider OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
	SliderWidget	sw = (SliderWidget)w;
	Display		*dsp = XtDisplay(w);
	Window		win = XtWindow(w);
	Position	oldElevPos,
			elevPos;
	int		real_pixel_value;
	int		expanded_value,
			inflated_value;
	double		percent;
		
	/* The updates that are possible are to the position of the slider
	 * and this obviously will affect the display value if showValue is
	 * true.
	 *
	 * flag = SL_POSITION - move slider elevator.
	 *      = SL_BORDER - draw highlight (focus) border.
	 *      = SL_NOBORDER - undraw focus border.
	 */
  switch(flags) {
	default:
		break;
	case SL_BORDER:
		DrawFocusHighlights((Widget)sw, OL_IN);
		break;
	case SL_NOBORDER:
		DrawFocusHighlights((Widget)sw, OL_OUT);
		break;
	case SL_POSITION:
	
		oldElevPos = sw->slider.sliderPValue;

            if (HORIZ(sw))
            {
                XClearArea (dsp, win, oldElevPos, sw->slider.elev_offset,
                            sw->slider.elevwidth, sw->slider.elevheight,
                            False);
            }
            else
            {
                XClearArea (dsp, win, sw->slider.elev_offset, oldElevPos,
                            sw->slider.elevwidth, sw->slider.elevheight,
                            False);
            }
	    /* Get new elevator position based on SLD.sliderPValue.
	     * Given: firstPosition and lastPosition, possible places where the
	     * slider elevator is constrained;
	     */
		DrawSliderElevator(sw);
	if (sw->slider.showValue)
		DrawSliderValue(sw);
  } /* switch */
}

/* OlgDrawSlider: Draw slider, Motif GUI */
void
_OlmOlgDrawSlider OLARGLIST((w, pInfo))
    OLARG( Widget,	w)
    OLGRA( OlgAttrs *,	pInfo)
{
	SliderWidget	sw = (SliderWidget)w;
	Dimension swidth, sheight;
	Dimension elevWidth, elevHeight;
	Dimension minwidth_needed, minheight_needed;
	int casewidth, caseheight;
	Display *dsp= XtDisplay(w);
	Window win = XtWindow(w);

		/* Given:
		 *	sw->core.width, sw->core.height;
		 *	SLD.lpad, rpad, topPad, bottomPad;
		 *	showValue - true, draw value, false, nothing;
		 *	- first, determine space requirements, then work your
		 *	  way down.
		 *
		 */
	OlgSizeSliderElevator (w, pInfo, &elevWidth, &elevHeight);
	elevWidth = (Dimension)sw->slider.elevwidth;
	elevHeight = (Dimension)sw->slider.elevheight;
	if (HORIZ(sw)) {
		/* horizontal slider/gauge */
		minwidth_needed = (Dimension)(SLD.leftPad + SLD.rightPad +
			elevWidth + 2 * sw->primitive.shadow_thickness +
			2 * sw->primitive.highlight_thickness);
		if (minwidth_needed > sw->core.width) {
			minwidth_needed -= SLD.rightPad;
			if (minwidth_needed > sw->core.width) {
				minwidth_needed -= SLD.leftPad;
				casewidth = NOPAD;
				/* The absolute minimum width will be
				 * enough to draw the elevator, shadow,
				 * and to leave room for the highlight border
				 */
			}
			else
				casewidth = LPAD;
		}
		else
			casewidth = ALLPAD;
		if ( (minheight_needed = (Dimension)(SLD.topPad +
			SLD.bottomPad + elevHeight + 
			2 * sw->primitive.shadow_thickness +
			2 * sw->primitive.highlight_thickness)) >
						sw->core.height) {
			minheight_needed -= SLD.bottomPad;
			if (minheight_needed > sw->core.height) {
				/* for a vertical slider, a top pad is
				 * needed for the value display only...
				 */
				caseheight = NOPAD;
			}
			else
				caseheight = TPAD;
		}
		else
			caseheight = ALLPAD;
	}
	else { /* Vertical slider */
	/* for the vertical, we'll be drawing the showvalue (if any)
	 * on the left;  take the left pad, and draw the slider
	 * left_pad pixels away from the left border (note here the
	 * slider starts with the fake border, not the shadow_thickness).
	 *   If there is enough room to draw the showvalue (if it's
	 * true) then we'll draw it too, else we'll blow it off.
	 */
		minwidth_needed = (Dimension)(elevWidth +
				2 * sw->primitive.shadow_thickness +
				2 * sw->primitive.highlight_thickness +
				SLD.leftPad + SLD.rightPad);
		if (minwidth_needed > sw->core.width) {
			minwidth_needed -= SLD.rightPad;
			if (minwidth_needed > sw->core.width) {
				/* no left margin either - the left
				 * margin may have contained a value
				 * display (if showValue), or it
				 * may have contained nothing, only
				 * a margin specified by XtNleftMargin...
				 */
				casewidth = NOPAD;
			}
			else
				casewidth = LPAD;
		}
		else
			casewidth = ALLPAD;

		minheight_needed = (Dimension)(elevHeight + 
				2 * sw->primitive.shadow_thickness +
				2 * sw->primitive.highlight_thickness +
				SLD.topPad + SLD.bottomPad);
		/* Of course, topPad AND bottomPad should be zero here,
		 * unless we are going to center it...
		 */
		if (minheight_needed > sw->core.height) {
			minheight_needed -= SLD.bottomPad;
			if (minheight_needed > sw->core.height) {
				caseheight = NOPAD;
			}
			else
				caseheight = TPAD;
		}
		else
			caseheight = ALLPAD;
	} /* else vertical slider */
	/* 
	 * We may have to put the caseheight information inside the
	 * slider, so that other routines can detect where the pointer
	 * is inside of it for events. 
	 * This information is better suited for Recalc(), anyway,
	 * thus reducing the size of this routine greatly.
	 * Let's look at the pad cases for each individual slider.
	 * Assume left/upper justificatin for all sliders/gauges.
	 * 1) Horizontal sliders.
	 * 1.1) y-axis.
	 *	a) ALLPAD, : There is plenty of room for top/bot margin.
	 *	   Draw upper justified slider,
	 *	   at offset (lpad, tpad).  IF there is room on top,
	 *	   then draw the display value, but only if there is room!
	 *	   Draw it at y = tpad - TextHeight(value), so it appears
	 *	   just above the highlight border.
	 *	b) TPAD: Basically the same as ALLPAD, because we are
	 *	   currently working on top justification.
	 *	c) NOPAD: Ignore all pads.  Simply draw the slider at (X,0),
	 *	   where X is the left justification, and 0 is the top pad.
	 * 1.2) x- axis.
	 *	a) ALLPAD:  Enough room for left/right margins.  Draw slider
	 *	   at (lpad, Y), where Y is the y-axis pad, and lpad is the
	 *	   offset of the highlight border.  Lpad may be XtNleftMargin,
	 *	   but no display values to worry about here.
	 *	b) LPAD:  Same as ALLPAD - will be more meaningful if we
	 *	   didn't do left justification.
	 *	c) NOPAD:  Draw slider at (0,Y) where Y is the y-axis padding
	 *	   determined above.
	 *
	 * 2) Vertical slider.
	 * 2.1) y-axis.
	 *	a) ALLPAD: Draw the slider at (X, ypad) where X is the
	 *	   x-axis offset.  Ypad is the place to draw the highlight
	 *	   border.
	 *	b) TPAD:  Same as ALLPAD, because we are working on
	 *	   upper-justification.
	 *	c) NOPAD: Draw slider at (X,0), 0 being the highlight
	 *	   border.
	 * 2.2) x-axis.
	 *	a) ALLPAD: Draw the slider at (xpad, Y), xpad being
	 *	   determined above.  Y is the y-axis offset.  Xpad is
	 *	   the offset where the highlight border gets drawn.
	 *	   If showvalue is true, then draw the display value
	 *	   just to the left of the highlight border, at 
	 *	   xpad - XTextWidth(display_value).
	 *	b) LPAD  - Basically the same as ALLPAD, because we
	 *	   are working on left justification.
	 *	c) NOPAD - Not enough room for pads, just draw the slider
	 *	   at (0, YPAD) (highlight border, that is).  No room for
	 *	   showValue, naturally.
	 *
	 * -- we have the following variables to work with:
	 *	minwidth_needed, minheight_needed.
	 */
	if (HORIZ(sw)) {
		switch(caseheight) {
			case ALLPAD:
			case TPAD:
				SLD.absy = (Position) (sw->slider.topPad +
					sw->primitive.highlight_thickness);
				break;
			case NOPAD:
			default:
				SLD.absy = (Position)
					sw->primitive.highlight_thickness;
				break;
		} /* switch caseheight */
		switch(casewidth) {
			case ALLPAD:
			case LPAD:
				SLD.absx = (Position)(sw->slider.leftPad +
					sw->primitive.highlight_thickness);
				break;
			case NOPAD:
			default:
				SLD.absx = (Position)
					sw->primitive.highlight_thickness;
				break;
		} /* switch casewidth */
	} /* HORIZ */
	else {
		switch(caseheight) {
			case ALLPAD:
			case TPAD:
				SLD.absy = (Position)(sw->slider.topPad +
					sw->primitive.highlight_thickness);
				break;
			case NOPAD:
				SLD.absy = (Position)
					sw->primitive.highlight_thickness;
				break;
		} /* switch caseheight */
		switch (casewidth) {
			case ALLPAD:
			case LPAD:
				SLD.absx = (Position)(sw->slider.leftPad +
					sw->primitive.highlight_thickness);
				break;
			case NOPAD:
				SLD.absx = (Position)
					sw->primitive.highlight_thickness;
				break;
		} /* switch casewidth */
	 } /* else VERTICAL */

	/* Draw shadow */

	/* All we need now is the shadows width and height.
	 * How do we get this?  With the simplistic approach we have taken
	 * so far, it is easy.  Let's do it case by case.
	 ******************************
	 **** THIS IS SUBJECT TO CHANGE IF WE USE A STRICT INTERPRETATION
	 * OF THE SPAN ARGUMENT.
	 ******************************
	 * 1) Horizontal.
	 * 1.1) casewidth.
	 *	a) ALLPAD.  There is enough room for padding on both
	 *	   sides of the slider.  The length of the shadow is 
	 *	    core.width - lpad - rpad - 2 * borderwidth.
	 *	b) LPAD:  Enough room for left pad only.  Shadow length =
	 *	   core.width - lpad - 2 * borderwidth.
	 *	c) NOPAD:  Draw minimum requirements.  Shadow length =
	 *	   core.width - 2 * borderwidth.
	 * 1.2) caseheight.
	 *	a) ALLPAD:  Shadow height = core.height - tpad - bpad -
	 *	   2 * border_width.
	 *	b) TPAD: Shadow height = core.height - tpad - 2 *
	 *	   border_width.
	 *	c) NOPAD: Shadow height = core.height - 2 * border_width.
	 * 2) Vertical slider.
	 * 2.1) casewidth: 
	 *	a) ALLPAD: Shadow_width = core.width - lpad - rpad -
	 *	   2 * border_width.
	 *	b) LPAD: Shadow_width = core.width - lpad - 2 * border_width.
	 *	c) NOPAD: shadow width = core.width - 2 * border_width.
	 */
	switch(casewidth) {
		case ALLPAD:
			if (HORIZ(sw))
				swidth = sw->core.width - sw->slider.leftPad - 
					sw->slider.rightPad - 
				2 * sw->primitive.highlight_thickness;
			else
				swidth = elevWidth +
					2 * sw->primitive.shadow_thickness;
			break;
		case LPAD:
			if (HORIZ(sw))
				swidth = sw->core.width - sw->slider.leftPad - 
					- 2 * sw->primitive.highlight_thickness;
			else
				swidth = elevWidth +
					2 * sw->primitive.shadow_thickness;
			break;
		case NOPAD:
		default:
			if (HORIZ(sw))
				swidth = sw->core.width -
					2 * sw->primitive.highlight_thickness;
			else
				swidth = elevWidth +
					2 * sw->primitive.shadow_thickness;
			break;
	} /* switch casewidth */
	switch(caseheight) {
		case ALLPAD:
			if (HORIZ(sw))
				sheight = elevHeight +
					2 * sw->primitive.shadow_thickness;
			else
				sheight = sw->core.height -
				  2 * sw->primitive.highlight_thickness -
				  sw->slider.topPad - sw->slider.bottomPad;
			break;
		case TPAD:
			if (HORIZ(sw))
				sheight = elevHeight +
					2 * sw->primitive.shadow_thickness;
			else
			  sheight = sw->core.height -
				2 * sw->primitive.highlight_thickness -
				sw->slider.bottomPad;
			break;
		case NOPAD:
		default:
			if (HORIZ(sw))
				sheight = elevHeight +
					2 * sw->primitive.shadow_thickness;
			else
			  sheight = sw->core.height -
			2 * sw->primitive.highlight_thickness;
			break;
	} /* switch caseheight */
			
	/* First draw shadow... */
         OlgDrawBorderShadow(XtScreen((Widget)sw), XtWindow((Widget)sw), 
	  ((SliderWidget)sw)->slider.pAttrs,
     	OL_SHADOW_IN, sw->primitive.shadow_thickness, SLD.absx, SLD.absy,
		  swidth, sheight);

	/* Next draw the elevator.  The elevator width - or at least
	 * the border that surrounds it - will be the same as the
	 * shadow thickness. The height is basically fixed, but
	 * this will not be perfect if the window is too small, that's for
	 * sure.  This shadow is OUT, of course.
	 *    The elevator position is drawn based on the sw->slider.value.
	 * Take the area that the elevator can be drawn in; that area is
	 * within the confines of the shadow just drawn.  Draw the
	 * elevator a perventage into the shadow area based on the
	 * values of sw->slider.sliderMin and sw->slider.sliderMax.
	 * 
	 */

	/* min value =>first position; max value => last position.
	 * first_position = the first position where the elevator can be drawn.
	 * == SLD.minTickPos
	 * last_position = the last pixel position where the elevator can
	 * == SLD.maxTickPos
	 * be drawn (the end of the elevator)
	 */
		/* (SLD.minTickPos) represents the minimum value,
		 * (SLD.maxTickPos) represents the maximum value.
		 */
	if (HORIZ(sw)) {
	 	SLD.minTickPos = (Dimension)(SLD.absx + 
					sw->primitive.shadow_thickness);
		SLD.maxTickPos = (Dimension)(SLD.absx + swidth - 
			sw->primitive.shadow_thickness - elevWidth);
	}
	else {
		/* Vertical - SLD.minTickPos represents the maximum value,
		 * last_position represents the minimum value.
		 */
	 	SLD.minTickPos = (Dimension)(SLD.absy +
				 sw->primitive.shadow_thickness);
		SLD.maxTickPos = (Dimension)(SLD.absy + sheight - 
			sw->primitive.shadow_thickness - elevHeight);
	}
	DrawSliderElevator(sw);
	if (sw->slider.showValue)
		DrawSliderValue(sw);
	
	if (sw->primitive.has_focus)
		DrawFocusHighlights((Widget)sw, (OlDefine)OL_IN);
	else
		DrawFocusHighlights((Widget)sw, (OlDefine)OL_OUT);
} /* OlgDrawSlider */

static void
DrawSliderValue OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Display *dsp = XtDisplay((Widget)sw);
	Window win = XtWindow((Widget)sw);
	char m[20];
	Position xplace, yplace;
	int textwidth;
	int textheight;

	if (sw->slider.display_value) {
		/* clear out value area */
		if (HORIZ(sw))
			XClearArea(dsp, win, 0, 0, sw->core.width,
				SLD.topPad, False);
		else
			XClearArea(dsp, win, 0, 0, SLD.leftPad,
				sw->core.height, False);
	}

	sprintf(m,"%d\0",sw->slider.sliderValue);

	textheight = OlFontHeight(sw->primitive.font,
				   sw->primitive.font_list);

	textwidth = (sw->primitive.font_list == NULL) ?
	    XTextWidth(sw->primitive.font, (char *)m, strlen(m)) :
		OlTextWidth(sw->primitive.font_list,
			    (unsigned char *)m, strlen(m));

	/* Get position, based on slider orientation */
	if (VERT(sw)) {
		/* Is there room for it ?? The base of the number
		 * should be a little below the middle of the
'			 * elevator.
		 */
		yplace = sw->slider.sliderPValue +
		   sw->slider.elevheight/2 + textheight/2;
		/* if xplace is off the screen, then tough luck -
		 * next time leave enough of a margin
		 */
		xplace = SLD.absx - sw->primitive.highlight_thickness
			- textwidth;
	}
	else /* Horiz */ {
		yplace = SLD.absy - sw->primitive.highlight_thickness;
			
		xplace = sw->slider.sliderPValue +
			sw->slider.elevwidth/2 - textwidth/2;
	}
	if (sw->primitive.font_list == NULL)
                	XDrawString (dsp, win, sw->slider.labelGC, 
			xplace, yplace, m, strlen(m));
	else
		OlDrawString (dsp, win, sw->primitive.font_list,
				sw->slider.labelGC,
				xplace, yplace,
				(unsigned char *)m, strlen(m));
	SLD.display_value = (char *)strndup(m, (int)strlen(m));
} /* DrawSliderValue */

#define DARK_GC(p)      (OlgIs3d() ? OlgGetBg3GC(p) : OlgGetBg2GC(p))
#define BRIGHT_GC(p)    (OlgIs3d() ? OlgGetBrightGC(p) : OlgGetBg2GC(p))

static void
DrawSliderElevator OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Screen *	scr = XtScreen((Widget)sw);
	Window		win = XtWindow((Widget)sw);
	int		start, end, expanded_value,
			inflated_value, real_pixel_value;
	double		percent;
	XRectangle	dark[1], light[1];
	Display *	dsp = XtDisplay((Widget)sw);
	Dimension	shadow_thickness = sw->primitive.shadow_thickness;	

	expanded_value = abs(sw->slider.sliderMax - sw->slider.sliderMin);

	inflated_value = abs(sw->slider.sliderValue - sw->slider.sliderMin);
	percent = (double)inflated_value /(double) expanded_value;

	if (HORIZ(sw))
		real_pixel_value = SLD.minTickPos + (int)(percent *
			(SLD.maxTickPos - SLD.minTickPos ));
	else /* VERT */
		real_pixel_value = SLD.maxTickPos - (int)(percent *
			(SLD.maxTickPos - SLD.minTickPos));

	if ((int)(shadow_thickness * 2) >= (int)sw->slider.elevwidth)
		shadow_thickness = (Dimension)(sw->slider.elevwidth/2 - 1);

	OlgDrawBorderShadow(scr, win, ((SliderWidget)sw)->slider.pAttrs,
				OL_SHADOW_OUT,
				sw->primitive.shadow_thickness,
				HORIZ(sw) ? real_pixel_value :
				 SLD.absx + sw->primitive.shadow_thickness,
				HORIZ(sw) ? SLD.absy +
					sw->primitive.shadow_thickness :
					real_pixel_value,
				sw->slider.elevwidth, sw->slider.elevheight);

	 /* And draw a nice thin shadow to cut across the middle of the
	  * elevator.
	  */
	if (HORIZ(sw)) {
		dark[0].x = (Position)(real_pixel_value +
					sw->slider.elevwidth/2);
		dark[0].y = SLD.absy + sw->primitive.shadow_thickness + 1;
		dark[0].width = 1;
		dark[0].height = sw->slider.elevheight - 2;
		light[0].x = dark[0].x + 1;
		light[0].y = dark[0].y;
		light[0].width = 1;
		light[0].height = dark[0].height;
	}
	else {
		dark[0].x = SLD.absx + sw->primitive.shadow_thickness + 1,
		dark[0].y = real_pixel_value + sw->slider.elevheight/2 - 1,
		dark[0].width = sw->slider.elevwidth - 2;
		dark[0].height = 1;
		light[0].x = dark[0].x;
		light[0].y = dark[0].y + 1;
		light[0].width = dark[0].width;
		light[0].height = 1;
	}

	XFillRectangles(dsp, win, DARK_GC(sw->slider.pAttrs),dark,1);
	XFillRectangles(dsp, win, BRIGHT_GC(sw->slider.pAttrs),light,1);

	sw->slider.sliderPValue = real_pixel_value;
}

/* focus_type: OL_IN or OL_OUT */

static void
DrawFocusHighlights OLARGLIST((w, focus_type))
	OLARG(Widget	,w)
	OLGRA(OlDefine,	focus_type) 
{
	SliderWidget sw = (SliderWidget)w;
	XRectangle rects[4];
	XGCValues xgcvalues;
	GC gc1;

		/* TOP */
	rects[0].x = (Position)(SLD.absx - sw->primitive.highlight_thickness);
	rects[0].y = (Position)(SLD.absy - sw->primitive.highlight_thickness);
	if (VERT(sw))
		rects[0].width = 2*sw->primitive.highlight_thickness +
				 2 * sw->primitive.shadow_thickness +
				 sw->slider.elevwidth;
	else
		rects[0].width = sw->core.width - SLD.leftPad - SLD.rightPad;

	rects[0].height = sw->primitive.highlight_thickness;

		/* left */
	rects[1].x = rects[0].x;
	rects[1].y = rects[0].y + sw->primitive.highlight_thickness;
	rects[1].width = sw->primitive.highlight_thickness;
	if (VERT(sw))
		rects[1].height = sw->core.height - SLD.topPad -
				  SLD.bottomPad -
				  sw->primitive.highlight_thickness;
	else
		rects[1].height = SLD.elevheight + 
				  2*sw->primitive.shadow_thickness +
				  sw->primitive.highlight_thickness;

		/* bottom */
	rects[2].x = (Position)(rects[0].x + sw->primitive.highlight_thickness);

	if (VERT(sw))
		rects[2].y = (Position)rects[1].height;
	else
		rects[2].y = (Position)(rects[1].y + rects[1].height
				- sw->primitive.highlight_thickness);

	rects[2].width = rects[0].width - sw->primitive.highlight_thickness;
	rects[2].height = sw->primitive.highlight_thickness;

	rects[3].x = (Position)(rects[0].x + rects[0].width - 
				sw->primitive.highlight_thickness);
	rects[3].y = (Position)(rects[0].y +
				sw->primitive.highlight_thickness);
	rects[3].width = sw->primitive.highlight_thickness;
	rects[3].height = rects[1].height - sw->primitive.highlight_thickness;

	switch(focus_type) {
		case OL_IN:
			/* Draw border with primitive.input_focus_color.
			 * Border width = sw->primitive.highlight_thickness.
			 */
			xgcvalues.foreground = sw->primitive.input_focus_color;
			xgcvalues.function = GXcopy;
			xgcvalues.fill_style = FillSolid;
			gc1 = XtGetGC(w, (GCForeground|GCFunction |
					GCFillStyle),&xgcvalues);
			XFillRectangles(XtDisplay(w),XtWindow(w), gc1,rects, 4);
			XtReleaseGC(w, gc1);
			break;
		case OL_OUT:
			/* Draw border with core.background_pixel */
			xgcvalues.foreground = sw->core.background_pixel;
			xgcvalues.function = GXcopy;
			xgcvalues.fill_style = FillSolid;
			gc1 = XtGetGC(w, (GCForeground|GCFunction |
					GCFillStyle),&xgcvalues);
			XFillRectangles(XtDisplay(w), XtWindow(w), gc1,rects,4);
			XtReleaseGC(w, gc1);
			break;
		default:
			break;
	} /* switch */
}
