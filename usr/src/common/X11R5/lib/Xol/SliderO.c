/* SliderO.c - Open Look GUI dependent slider functions */

#ifndef NOIDENT
#ident	"@(#)slider:SliderO.c	1.10"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/SliderP.h>
#include <Xol/GaugeP.h>

#define MAX(a,b)	( (int)(a) > (int)(b) ? a : b)
#define	SLD		sw->slider
#define	HORIZ(W)	((W)->slider.orientation == OL_HORIZONTAL)
#define	VERT(W)		((W)->slider.orientation == OL_VERTICAL)
#define TICKS_ON(W)	((W)->slider.tickUnit != OL_NONE)

extern void	_OloSliderRecalc OL_ARGS((SliderWidget));
extern Dimension _OloSlidercalc_leftMargin OL_ARGS((SliderWidget));
extern Dimension _OloSlidercalc_rightMargin OL_ARGS((SliderWidget));
extern Dimension _OloSlidercalc_bottomMargin OL_ARGS((SliderWidget));
extern Dimension _OloSlidercalc_topMargin OL_ARGS((SliderWidget));

static void make_ticklist OL_ARGS((SliderWidget));

/*
 * Recalc().
 *
 * This	function calculates all	the dimensions for a slider.
 *	- Save (in slider struct) anchorWidth, anchorHeight,
 *	  elevWidth, elevHeight,
 *	  left, right, top, and bottom margins (calls calc_..()),
 *	  Finally, determine the minimum and maximum tick positions - it
 *	   get this by taking the core width or height, and evaluating the
 *	   offset based on the paddings returned from calc_margins(),
 *	   the anchor widht/ht., and the elevator width/ht.
 *
 *	Called from Resize(), Realize(), and SetValues().
 */
extern void
_OloSliderRecalc OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension elevWidth, elevHeight;
	Dimension anchorWidth, anchorHeight;

	/* calculate, save dimensions of anchors */
        	OlgSizeSliderAnchor ((Widget)sw, sw->slider.pAttrs,
			&anchorWidth, &anchorHeight);
        	sw->slider.anchwidth = anchorWidth;
        	sw->slider.anchlen = anchorHeight;

	/* calculate, save dimensions of elevator */

	OlgSizeSliderElevator ((Widget)sw, sw->slider.pAttrs, &elevWidth, &elevHeight);
	sw->slider.elevwidth = elevWidth;
	sw->slider.elevheight = elevHeight;

	/* calculate left padding */
	SLD.leftPad = _OloSlidercalc_leftMargin(sw);	

	/* calculate right padding */
	SLD.rightPad = _OloSlidercalc_rightMargin(sw);

	/* calculate top and bottom padding */
	SLD.topPad = _OloSlidercalc_topMargin(sw);
	SLD.bottomPad = _OloSlidercalc_bottomMargin(sw);

	sw->slider.type	= SB_REGULAR;
	if (HORIZ (sw))
	{
	    Dimension	length;

	    length = sw->core.width - SLD.leftPad - SLD.rightPad;

			if ((Dimension) (SLD.anchwidth * 2 +
				 SLD.elevwidth) >= length)
				sw->slider.type	= SB_MINREG;
	    		SLD.elev_offset = 0;
			/* calculate min and max tick positons */
			SLD.minTickPos = SLD.leftPad + sw->slider.anchwidth +
				(Position) sw->slider.elevwidth/2;
			SLD.maxTickPos = sw->core.width - sw->slider.anchwidth -
			  (Position)(sw->slider.elevwidth + 1)/2 - SLD.rightPad;
	}
	else /* vertical */
	{
			if ((Dimension)(SLD.anchlen*2 + SLD.elevheight) >=
				  (Dimension)(sw->core.height - SLD.topPad -
				  SLD.bottomPad))
				sw->slider.type	= SB_MINREG;
	    		SLD.elev_offset = SLD.leftPad;
			/* calculate min and max tick positions */
			SLD.maxTickPos = SLD.topPad + sw->slider.anchlen +
				(Position) sw->slider.elevheight/2;
			SLD.minTickPos = sw->core.height - sw->slider.anchlen -
			  (Position)(sw->slider.elevheight + 1)/2 -
							 SLD.bottomPad;
	} /* vertical */
	/* propagage sw->slider.tickList with all possible
	 * tick positions- based on sw->slider.tickUnit
 	 */
		make_ticklist(sw);
} /* Recalc */



/* calc_leftMargin: get margin (padding) to left of slider/gauge.
 * If one is supplied, just return it to the caller.  
 * If none is supplied by the user, then we will determine if any left pad is
 * needed:
 *	if (vertical OR no minlabel)
 *		leftMargin=0;
 *	else
 *		Add enough padding on the left for the label, return the amt.
 */
extern Dimension 
_OloSlidercalc_leftMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension labelwidth;
	Dimension tiplen;
	Dimension total = 0;

		if ((Dimension)SLD.leftMargin != (Dimension)OL_IGNORE)
			return(SLD.leftMargin);

		if (VERT(sw) || (SLD.minLabel == NULL))
			return((Dimension)0);
	if (sw->primitive.font_list)
		labelwidth = OlTextWidth(sw->primitive.font_list,
					 (unsigned char *)SLD.minLabel, 
					 strlen(SLD.minLabel)) / 2;
	else	labelwidth = XTextWidth(sw->primitive.font, SLD.minLabel,
					strlen(SLD.minLabel)) / 2;

	tiplen = sw->slider.anchwidth + sw->slider.elevwidth/2;
	if (labelwidth < tiplen)
		return((Dimension)0);

	else
		return(labelwidth - tiplen);
} /* calc_leftMargin */

/* calc_rightMargin:
 *	if one is supplied by user, return it;
 *	if vertical slider:
 * 		get MAX(width of MinLabel, width of MaxLabel);	
 *		return MAX.
 *	else (Horizontal slider)
 *		add Enough padding for length of MAX label (if any).
 *
 */
extern Dimension 
_OloSlidercalc_rightMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension labelwidth;
	Dimension tiplen;

	if ((Dimension)SLD.rightMargin != (Dimension)OL_IGNORE)
		return(SLD.rightMargin);

        if (VERT(sw)) {
                Dimension minwidth;

		if (SLD.minLabel != NULL) {
			if (sw->primitive.font_list)
			  minwidth =
			    OlTextWidth(sw->primitive.font_list, 
					(unsigned char *)SLD.minLabel,
					strlen(SLD.minLabel));
			else
                		minwidth = XTextWidth(sw->primitive.font, 
						      SLD.minLabel,
                                		      strlen(SLD.minLabel));
		}
		else
			minwidth = 0;
		if (SLD.maxLabel != NULL)
			{
			if (sw->primitive.font_list)
        		     labelwidth =
			       OlTextWidth(sw->primitive.font_list,
					   (unsigned char *)SLD.maxLabel, 
					   strlen(SLD.maxLabel));
			else
        		     labelwidth = XTextWidth(sw->primitive.font, 
						     SLD.maxLabel, 
						     strlen(SLD.maxLabel));
			}
		else
			labelwidth = 0;
                labelwidth = MAX(minwidth, labelwidth);
		return(labelwidth);
        }

	if (SLD.maxLabel != NULL) {
		if (sw->primitive.font_list)
		  labelwidth = OlTextWidth(sw->primitive.font_list, 
					   (unsigned char *)SLD.maxLabel,
					   strlen(SLD.maxLabel)) / 2;
        	else	labelwidth = XTextWidth(sw->primitive.font, 
						SLD.maxLabel,
                          			strlen(SLD.maxLabel)) / 2;
		tiplen = sw->slider.anchwidth +
		    (Dimension) (sw->slider.elevwidth+1)/2;
		if (labelwidth < tiplen)
			return((Dimension)0);
		else
			return(labelwidth - tiplen);
	}
	else
		return((Dimension)0);
} /* calc_rightMargin */


/* calc_bottomMargin: 
 *	if Horiz. slider/gauge, OR Vertical and no MinLabel,
 *		return 0 (no botton pad needed).
 *	if Vertical slider and has a MinLabel:
 *		Get label height = ascent + descent)/2,
 *		add on extra padding, return this number.
 */
extern Dimension 
_OloSlidercalc_bottomMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension labelheight;
	Dimension tiplen;
	int dummy1, ascent, descent;
	XCharStruct dummy2;

	if (HORIZ(sw) || (SLD.minLabel == NULL))
	  return ((Dimension)0);

	labelheight = OlFontHeight(sw->primitive.font,
				    sw->primitive.font_list);
	  
	tiplen = sw->slider.anchlen +
	  (Dimension) (sw->slider.elevheight+1)/2;
	if (labelheight < tiplen)
	  return((Dimension)0);
	else
	  return(labelheight - tiplen);
} /* calc_bottomMargin */


/* calc_topMargin:  determine padding for top of widget (above slider/gauge).
 *	if Horizontal slider, return 0;
 *	if Vertical slider AND no maxlabel, return 0;
 *	else (Vertical AND has MaxLabel:
 *		get Height of label = (ascent + descent) /2,
 *		add on a little extra pad, return this number.
 */
extern Dimension
_OloSlidercalc_topMargin OLARGLIST((sw))
	OLGRA( SliderWidget, sw)
{
	Dimension labelheight;
	Dimension tiplen;
	int dummy1, ascent, descent;
	XCharStruct dummy2;
	OlDefine currentGUI = OlGetGui();

	if (HORIZ(sw) || (SLD.maxLabel == NULL))
	  return ((Dimension)0);

	labelheight = OlFontHeight(sw->primitive.font,
				    sw->primitive.font_list);
	  
	/* Motif mode: slider does not have an anchor (we force
	 * XtNendBoxes to False)
	 */
	tiplen = sw->slider.anchlen +
	  (Dimension) (sw->slider.elevheight+1)/2;
	if (labelheight < tiplen)
	  return((Dimension)0);
	else
	  return(labelheight - tiplen);
} /* calc_topMargin */

/* fills sw->slider.tickList with all possible tick positions based on
 * sw->slider.tickUnit.
 */
static void
make_ticklist OLARGLIST((sw))
	OLGRA( SliderWidget, sw)
{
	int i;
	int urange;
	Position range;
	Position *pp;

	/* free the old list */
	if (sw->slider.ticklist) {
		free(sw->slider.ticklist);
		sw->slider.ticklist = NULL;
	}

	if (!TICKS_ON(sw)) 
		return;

	switch(sw->slider.tickUnit) {
	case OL_PERCENT:
		urange = 100;
		sw->slider.numticks = 100 / sw->slider.ticks + 1;
		break;
	case OL_SLIDERVALUE:
		urange = SLD.sliderMax - SLD.sliderMin;
		sw->slider.numticks = urange / sw->slider.ticks + 1;
		break;
	}
	range = SLD.maxTickPos - SLD.minTickPos;
	if (range < 0)
		range = - range;
	
	/* allocate space */
	pp = SLD.ticklist = (Position *)XtMalloc(SLD.numticks * sizeof(Position));

	if (VERT(sw))
	    for (i=SLD.numticks - 1; i >= 0; i--) {
		*pp++ = SLD.minTickPos - ((range*(i*SLD.ticks))/urange);
	    }
	else 
	    for (i=0; i < SLD.numticks; i++) {
		*pp++ = SLD.minTickPos + ((range*(i*SLD.ticks))/urange);
	    }
}
