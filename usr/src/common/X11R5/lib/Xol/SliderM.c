/* SliderM.c - Motif GUI dependent slider functions */

#ifndef NOIDENT
#ident	"@(#)slider:SliderM.c	1.8"
#endif

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/SliderP.h>
#include <Xol/GaugeP.h>

/* SliderM.c - Motif GUI dependent slider functions */

#define MAX(a,b)	( (int)(a) > (int)(b) ? a : b)
#define	SLD		sw->slider
#define	HORIZ(W)	((W)->slider.orientation == OL_HORIZONTAL)
#define	VERT(W)		((W)->slider.orientation == OL_VERTICAL)

extern Dimension	_OlmSlidercalc_leftMargin OL_ARGS((SliderWidget));
extern Dimension	_OlmSlidercalc_rightMargin OL_ARGS((SliderWidget));
extern Dimension	_OlmSlidercalc_bottomMargin OL_ARGS((SliderWidget));
extern Dimension	_OlmSlidercalc_topMargin OL_ARGS((SliderWidget));
extern void		_OlmSliderRecalc OL_ARGS((SliderWidget));

/*
 * Recalc().
 *
 * This	function calculates all	the dimensions for a slider.
 *	- Save (in slider struct) elevWidth, elevHeight,
 *	  	left, right, top, and bottom margins (calls calc_..()),
 *
 *	Called from Resize(), Realize(), and SetValues().
 */
extern void
_OlmSliderRecalc OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension elevWidth, elevHeight;

	/* calculate, save dimensions of elevator */

	OlgSizeSliderElevator ((Widget)sw, sw->slider.pAttrs, &elevWidth, &elevHeight);
	sw->slider.elevwidth = elevWidth;
	sw->slider.elevheight = elevHeight;

	/* calculate left padding */
	SLD.leftPad = _OlmSlidercalc_leftMargin(sw);	

	/* calculate right padding */
	SLD.rightPad = _OlmSlidercalc_rightMargin(sw);

	/* calculate top and bottom padding */
	SLD.topPad = _OlmSlidercalc_topMargin(sw);
	SLD.bottomPad = _OlmSlidercalc_bottomMargin(sw);

	sw->slider.type	= SB_REGULAR;
	if (HORIZ (sw))
	{
	    Dimension	length;

	    length = sw->core.width - SLD.leftPad - SLD.rightPad;

	    if ((int)(2 * sw->primitive.highlight_thickness + SLD.elevwidth +
		2 * sw->primitive.shadow_thickness) >= (int)length)
			sw->slider.type	= SB_MINREG;
	    SLD.elev_offset =	SLD.topPad +
				sw->primitive.highlight_thickness +
				sw->primitive.shadow_thickness;
	}
	else /* vertical */
	{
		if ((int)(sw->core.height - SLD.topPad-SLD.bottomPad) <
		    (int)(2 * sw->primitive.highlight_thickness + SLD.elevheight
		    + 2 * sw->primitive.shadow_thickness))
					sw->slider.type = SB_MINREG;
	    	/* looking from the LEFT */
	    	SLD.elev_offset =	SLD.leftPad +
					sw->primitive.highlight_thickness +
					sw->primitive.shadow_thickness;
	} /* vertical */
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
_OlmSlidercalc_leftMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	Dimension labelwidth;
	Dimension tiplen;
	Dimension total = 0;

		char minvalue[50], maxvalue[50];
		int minw, maxw;

		if ((Dimension)SLD.leftMargin != (Dimension)OL_IGNORE)
			return(SLD.leftMargin);
		if ( !(sw->slider.showValue) || HORIZ(sw))
			return((Dimension)0);

		sprintf(minvalue,"%d\0",SLD.sliderMin);
		sprintf(maxvalue,"%d\0",SLD.sliderMax);
		if (sw->primitive.font_list) {
			minw = OlTextWidth(sw->primitive.font_list,
				 (unsigned char *)minvalue,
				 strlen(minvalue));
			maxw = OlTextWidth(sw->primitive.font_list,
				 (unsigned char *)maxvalue,
				 strlen(maxvalue));
		}
		else {
			minw = XTextWidth(sw->primitive.font,
				 (char *)minvalue,
				 strlen(minvalue));
			maxw = XTextWidth(sw->primitive.font,
				 (char *)maxvalue,
				 strlen(maxvalue));
		}
		return((Dimension)MAX(minw, maxw));
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
_OlmSlidercalc_rightMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	if ((Dimension)SLD.rightMargin != (Dimension)OL_IGNORE)
		return(SLD.rightMargin);
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
_OlmSlidercalc_bottomMargin OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	return((Dimension)0);
} /* calc_bottomMargin */

extern Dimension 
_OlmSlidercalc_topMargin OLARGLIST((sw))
	OLGRA( SliderWidget, sw)
{
	Dimension labelheight;

	if (!(sw->slider.showValue) || VERT(sw))
		return((Dimension)0);

	/* Motif mode : must have showValue && horizontal orientation */

	labelheight = OlFontHeight(sw->primitive.font,
				    sw->primitive.font_list);
	  
	/* Motif mode - need full label height */

	return((Dimension)labelheight);
} /* calc_topMargin */
