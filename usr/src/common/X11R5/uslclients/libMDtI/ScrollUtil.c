#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:ScrollUtil.c	1.8"
#endif

/******************************file*header********************************
 *
 * Description:
 *   This file contains various support and convenience routines
 *   for interacting with the ScrolledWindow.
 */

                        /* #includes go here    */
#include <stdio.h>

#include <Xm/PrimitiveP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include "WidePosDef.h"
#include "ScrollUtil.h"

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

#define CPART(W)	(((XmPrimitiveWidget)(W))->core)
#define PPART(W)	(((XmPrimitiveWidget)(W))->primitive)

#define PARENT		XtParent(w)	/* assumed `w' */
#define DPY		XtDisplay(w)
#define WIN		XtWindow(w)

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations.
 */
					/* private procedures		*/

/****************************private*procedures***************************
 *
 * Private Procedures
 */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

/*
 ****************************procedure*header*****************************
 * ExmSWinCalcViewSize - is used to calculate the current view size and
 *	calibrate Horizontal and/or Vertical XmScrollBarWidget(s).
 *	Scrollbars will be displayed and/or removed when necessary.
 *
 * This function returns True if one/both of XmNSliderSize is/are
 *	changed. ExmSWinHandleResize() will call XClearArea() if
 *	the returned value is True.
 *
 * req_wd and/or req_hi will be view_wd and/or view_hi if the value is
 *	not zero, otherwise, the view_wd and/view_hi will computed from
 *	its parent.
 *
 * min_x/min_y will be used for XmNminimum calculation for hsb/vsb... TBD
 *
 * *x_offset and *y_offset are in/out values, they represent the current
 *	offset w.r.t. (0, 0) position of the child of XmScrolledWindow and
 *	will be changed with new offset value(s) when necessary.
 *
 * *width and *height are in/out values, they represent the actual child
 *	window size initially and will be changed with the view size.
 *************************************************************************
*/
extern Boolean
ExmSWinCalcViewSize(
	Widget		w,	  /* child of XmScrolledWindow		*/
	Widget		hsb,	  /* Horizontal XmScrollBarWidget	*/
	Widget		vsb,	  /* Vertical XmScrollBarWidget		*/
	Dimension	x_uom,	  /* slider units in x/y directions	*/
	Dimension	y_uom,
	Dimension	req_wd,   /* use them if the value is not 0,	*/
	Dimension	req_hi,   /* otherwise, compute from its parent	*/
	WidePosition	min_x,	  /* min_x - for hsb's XmNminimum calc	*/
	WidePosition	min_y,	  /* min_y - forvhsb's XmNminimum calc	*/
	WidePosition *	x_offset, /* in - current offset,		*/
	WidePosition *	y_offset, /*	out - new offset...		*/
	WideDimension *	width,	  /* in - actual width/actual height,	*/
	WideDimension *	height)	  /*	out - view width/height...	*/
{
#define X_CHANGED	(1L << 0)
#define Y_CHANGED	(1L << 1)

	Arg		args[4];
	Dimension	view_wd, view_hi;
	Boolean		need_vsb, need_hsb;
	Dimension	swin_sh, swin_sp;
	int		hsb_hi, vsb_wd;
	int		tmp_val, max_value, slider_size, value;
	unsigned long	offset_changed = 0;


		/* if req_wd/req_hi is greater than *width/*height
		 * then setting it to req_wd/req_hi */
	if (*width < req_wd)
		*width = req_wd;

	if (*height < req_hi)
		*height = req_hi;

	XtSetArg(args[0], XmNshadowThickness, &swin_sh);
	XtSetArg(args[1], XmNspacing, &swin_sp);
	XtGetValues(PARENT, args, 2);

#define VSB_BW		CPART(vsb).border_width
#define VSB_wd		CPART(vsb).width
#define VSB_WD		VSB_wd + swin_sp + 2 * VSB_BW

#define HSB_BW		CPART(hsb).border_width
#define HSB_hi		CPART(hsb).height
#define HSB_HI		HSB_hi + swin_sp + 2 * HSB_BW

#define SWIN_WD		CPART(PARENT).width - 2 * swin_sh
#define SWIN_HI		CPART(PARENT).height - 2 * swin_sh

	if (req_wd)
	{
		vsb_wd  = 0;
		view_wd = req_wd;
	}
	else
	{
		vsb_wd  = VSB_WD;
		view_wd = SWIN_WD;
	}

	if (req_hi)
	{
		hsb_hi  = 0;
		view_hi = req_hi;
	}
	else
	{
		hsb_hi  = HSB_HI;
		view_hi = SWIN_HI;
	}

#undef SWIN_WD
#undef SWIN_HI
#undef VSB_wd
#undef VSB_WD
#undef VSB_BW
#undef HSB_hi
#undef HSB_HI
#undef HSB_BW

	need_vsb = (*height > view_hi);
	if (*width > view_wd)
	{
		need_hsb = True;
	}
	else
	{
			/* may still need hsb because of vsb */
		if ( need_vsb &&
		     (int)*width > (int)(view_wd - vsb_wd) )
			need_hsb = True;
		else
			need_hsb = False;
	}

	if (need_hsb)
	{
		if (XtIsManaged(hsb) == False)
			XtManageChild(hsb);
		view_hi -= hsb_hi;
	}
	else
	{
		if (XtIsManaged(hsb) == True)
			XtUnmanageChild(hsb);

		if (*x_offset != 0)
		{
			*x_offset = 0;
			offset_changed |= X_CHANGED;
		}
	}

	if (need_vsb)
	{
		if (XtIsManaged(vsb) == False)
			XtManageChild(vsb);
		view_wd -= vsb_wd;
	}
	else
	{
		if (XtIsManaged(vsb) == True)
			XtUnmanageChild(vsb);

		if (*y_offset != 0)
		{
			*y_offset = 0;
			offset_changed |= Y_CHANGED;
		}
	}

	if (need_hsb)	/* x_uom based */
	{
		max_value = (int)*width / (int)x_uom;
		if ((int)*width % (int)x_uom)
			max_value++;

		slider_size	= (int)view_wd / (int)x_uom;
		value		= (int)*x_offset / (int)x_uom;

			/* if maximum - slider_size < value
			 * then we need to adjust `value' */
		if (value > (tmp_val = max_value - slider_size))
		{
			value = tmp_val;
			*x_offset = tmp_val * x_uom;
			offset_changed |= X_CHANGED;
		}

		XtSetArg(args[0], XmNmaximum, max_value);
		XtSetArg(args[1], XmNpageIncrement, slider_size);
		XtSetArg(args[2], XmNvalue, value);
		XtSetArg(args[3], XmNsliderSize, slider_size);
		XtSetValues(hsb, args, 4);
	}

	if (need_vsb)	/* y_uom based */
	{
		max_value = (int)*height / (int)y_uom;
		if ((int)*height % (int)y_uom)
			max_value++;

		slider_size	= (int)view_hi / (int)y_uom;
		value		= (int)*y_offset / (int)y_uom;

			/* if maximum - slider_size < value
			 * then we need to adjust `value' */
		if (value > (tmp_val = max_value - slider_size))
		{
			value = tmp_val;
			*y_offset = tmp_val * y_uom;
			offset_changed |= Y_CHANGED;
		}

		XtSetArg(args[0], XmNmaximum, max_value);
		XtSetArg(args[1], XmNpageIncrement, slider_size);
		XtSetArg(args[2], XmNvalue, value);
		XtSetArg(args[3], XmNsliderSize, slider_size);
		XtSetValues(vsb, args, 4);
	}

	*width = view_wd;
	*height = view_hi;

	return(offset_changed ? True : False);
#undef X_CHANGED
#undef Y_CHANGED
} /* end of ExmSWinCalcViewSize */

/*
 ****************************procedure*header*****************************
 * ExmSWinCreateScrollbars - is used to create horizontal and vertical
 *	scrollbars if the following conditions hold:
 *		a. `w' is a child of XmScrolledWindowWidget;
 *		b. XmNscrollingPolicy is XmAPPLICATION_DEFINED;
 *		c. XmNvisualPolicy is XmVARIABLE;
 *		d. XmscrollBarDisplayPolicy is XmSTATIC;
 *		e. XmNhorizontalScrollBar is NULL;
 *	and	f. XmNverticalScrollBar is NULL.
 *
 * The function will return `True' if the operation was successful,
 *	otherwise, it returns `False'!
 *
 * Upon return, both *x_offset and *y_offset are set to `0', *hsb and
 *	*vsb are set to NULL if the conditions above are failed, otherwise
 *	scrollbar widgets are created and their ids are returned.
 *
 * In addition, if `cb' is not NULL, then we will invoke
 *	ExmSWinSetupScrollbars(). Note that some classes may not be able
 *	to setup everything with one call and have to call
 *	ExmSWinSetupScrollbars() in the subclass, e.g., ExmFlatWidgetClass
 *	can't not determine `cb' and have to defer them in
 *	ExmFlatGraphWidgetClass, on the other hand, ExmHyperTextWidgetClass
 *	can setup everything with one call!
 *************************************************************************
*/
extern Boolean
ExmSWinCreateScrollbars(
	Widget		w,	  /* child of the XmScrolledWindow widget    */
	Widget *	hsb,	  /* horizontal scrollbar widget id	     */
	Widget *	vsb,	  /* vertical scrollbar widget id	     */
	WidePosition *	x_offset, /* initial offset values in vertical	     */
	WidePosition *	y_offset, /* 	and horizontal directions...	     */
	XtCallbackProc	cb)	  /* callback for both horiz/vert scrollbars */
{
	Boolean		in_swin = False;

		/* Check if the given widget is created inside of
		 * a scrolled window, if so, then do necessary setup... */
	if (XtIsSubclass(PARENT, xmScrolledWindowWidgetClass))
	{
		Arg		args[11];
		unsigned char	scr_policy;
		unsigned char	vis_policy;
		unsigned char	bar_policy;
		Widget		_hsb, _vsb;

		XtSetArg(args[0], XmNscrollingPolicy, &scr_policy);
		XtSetArg(args[1], XmNvisualPolicy, &vis_policy);
		XtSetArg(args[2], XmNscrollBarDisplayPolicy, &bar_policy);
		XtSetArg(args[3], XmNhorizontalScrollBar, &_hsb);
		XtSetArg(args[4], XmNverticalScrollBar, &_vsb);
		XtGetValues(PARENT, args, 5);

		if (scr_policy != XmAPPLICATION_DEFINED ||
		    vis_policy != XmVARIABLE || bar_policy != XmSTATIC ||
		    _hsb != (Widget)NULL || _vsb != (Widget)NULL)
		{
			/* we can't go further if an app didn't follow
			 * Motif spec. because we can't change these
			 * behaviors via set_values(), see Motif
			 * programmer/reference manuals for details. */
		printf("%s (%d) - Bad SWIN values\n", __FILE__, __LINE__);
		}
		else	/* O.K., we are in business... */
		{
#define ShadowThickness		PPART(w).shadow_thickness
#define Foreground		PPART(w).foreground
#define Background		CPART(w).background_pixel
#define BackgroundPix		CPART(w).background_pixmap
#define TopShadow		PPART(w).top_shadow_color
#define TopShadowPix		PPART(w).top_shadow_pixmap
#define BotShadow		PPART(w).bottom_shadow_color
#define BotShadowPix		PPART(w).bottom_shadow_pixmap


			in_swin = True;

			XtSetArg(args[0], XmNshadowThickness, ShadowThickness);
			XtSetArg(args[1], XmNforeground, Foreground);
			XtSetArg(args[2], XmNbackground, Background);
			XtSetArg(args[3], XmNbackgroundPixmap, BackgroundPix);
			XtSetArg(args[4], XmNtopShadowColor, TopShadow);
			XtSetArg(args[5], XmNtopShadowPixmap, TopShadowPix);
			XtSetArg(args[6], XmNbottomShadowColor, BotShadow);
			XtSetArg(args[7], XmNbottomShadowPixmap, BotShadowPix);
			XtSetArg(args[8], XmNhighlightThickness, 0);
	/* we need to handle scrolling..., expand the translation? */
			XtSetArg(args[9], XmNtraversalOn, False);

			XtSetArg(args[10], XmNorientation, XmVERTICAL);
			*vsb = XmCreateScrollBar(PARENT, "VSB", args, 11);

			XtSetArg(args[10], XmNorientation, XmHORIZONTAL);
			*hsb = XmCreateScrollBar(PARENT, "HSB", args, 11);

			XtSetArg(args[0], XmNworkWindow, w);
			XtSetArg(args[1], XmNhorizontalScrollBar, *hsb);
			XtSetArg(args[2], XmNverticalScrollBar, *vsb);
			XtSetValues(PARENT, args, 3);

			if (cb)
			{
				ExmSWinSetupScrollbars(w, *hsb, *vsb, cb);
			}

#undef ShadowThickness
#undef Foreground
#undef Background
#undef BackgroundPix
#undef TopShadow
#undef TopShadowPix
#undef BotShadow
#undef BotShadowPix
		}
	}

	*x_offset = *y_offset = 0;

	if (in_swin == False)
	{
		*hsb = *vsb = (Widget)NULL;
	}

	return(in_swin);

} /* end of ExmSWinCreateScrollbars */

/*
 ****************************procedure*header*****************************
 * ExmSWinHandleValueChange - is used when either Horizontal or
 *	Vertical XmScrollBarWidget value is changed. The code will only
 *	generate necessary exposures and will call XCopyArea to optimize
 *	the situation.
 *
 * `gc' can set GCGraphicsExposures to False for avoiding GraphicsExpose/
 *	NoExpose event(s) and should set GCFunction to GXCopy.
 *	The expose method will have to handle GraphicsExpose and/or NoExpose
 *	depending on the compress_expose value in the class record.
 *
 * *x_ or y_offset is an in/out value, it contains the current offset values
 *	and will be changed with new_val (new slider value) if necessary.
 *************************************************************************
*/
extern void
ExmSWinHandleValueChange(
	Widget		w,	  /* child of XmScrolledWindowWidget	*/
	Widget		sb,	  /* Hori/Vertical XmScrollBarWidget	*/
	GC		gc,	  /* GCGraphicsExposures or not		*/
	int		new_val,  /* new slider value in Hor or Ver dir	*/
	Dimension	x_uom,    /* slider units in x/y directions	*/
	Dimension	y_uom,
	WidePosition *	x_offset, /* current offset values		*/
	WidePosition *	y_offset)
{
	Arg		arg[1];
	unsigned char	h_or_v;
	int		cur_val;
	int		delta;
	int		page_incr;
	int		n_clr = 1;
	int		src_x, src_y, dst_x, dst_y, clr_x[2], clr_y[2];
	unsigned int	cp_wd, cp_hi, clr_wd[2], clr_hi[2];

	XtSetArg(arg[0], XmNorientation, &h_or_v);
	XtGetValues(sb, arg, 1);

#define ABS(V)		( (V < 0) ? -(V) : (V) )
#define VIEW_WD		CPART(w).width
#define VIEW_HI		CPART(w).height

	if (h_or_v == XmHORIZONTAL)
	{
		cur_val = (int)*x_offset / (int)x_uom;

		if (cur_val == new_val)
			return;

		page_incr	= VIEW_WD / x_uom;
		delta		= new_val - cur_val;
		*x_offset	= new_val * x_uom;

		if (ABS(delta) >= page_incr)
		{
			delta = 0;
		}
		else if (delta > 0)
		{
#define DELTA		delta * x_uom
#define EXTRA		VIEW_WD % x_uom

				/* calc copy_area info */
			src_x = DELTA;
			src_y = 0;
			cp_wd = VIEW_WD - DELTA;
			cp_hi = VIEW_HI;
			dst_x = dst_y = 0;

				/* calc clear_area info */
			clr_x[0]  = VIEW_WD - DELTA;
			clr_y[0]  = 0;
			clr_wd[0] = DELTA + EXTRA;
			clr_hi[0] = VIEW_HI;
#undef DELTA
		}
		else	/* delta < 0 */
		{
#define DELTA		ABS(delta) * x_uom

				/* calc copy_area info */
			src_x = src_y = 0;
			cp_wd = VIEW_WD - DELTA;
			cp_hi = VIEW_HI;
			dst_x = DELTA;
			dst_y = 0;

				/* calc 1st clear_area info */
			clr_x[0]  = clr_y[0] = 0;
			clr_wd[0] = DELTA;
			clr_hi[0] = VIEW_HI;

				/* calc 2nd clear_area info if necessary */
			if (EXTRA)
			{
				n_clr	  = 2;

				clr_x[1]  = VIEW_WD - EXTRA;
				clr_y[1]  = 0;
				clr_wd[1] = EXTRA;
				clr_hi[1] = VIEW_HI;
			}
#undef DELTA
#undef EXTRA
		}
	}
	else /* XmVERTICAL */
	{
		cur_val = (int)*y_offset / (int)y_uom;

		if (cur_val == new_val)
			return;

		page_incr	= VIEW_HI / y_uom;
		delta		= new_val - cur_val;
		*y_offset	= new_val * y_uom;

		if (ABS(delta) >= page_incr)
		{
			delta = 0;
		}
		else if (delta > 0)
		{
#define DELTA		delta * y_uom
#define EXTRA		VIEW_HI % y_uom
 
				/* calc copy_area_info */
			src_x = 0;
			src_y = DELTA;
			cp_wd = VIEW_WD;
			cp_hi = VIEW_HI - DELTA;
			dst_x = dst_y = 0;

				/* calc clear_area info */
			clr_x[0]  = 0;
			clr_y[0]  = VIEW_HI - DELTA;
			clr_wd[0] = VIEW_WD;
			clr_hi[0] = DELTA + EXTRA;
#undef DELTA
		}
		else	/* delta < 0 */
		{
#define DELTA		ABS(delta) * y_uom

				/* calc copy_area info */
			src_x = src_y = 0;
			cp_wd = VIEW_WD;
			cp_hi = VIEW_HI - DELTA;
			dst_x = 0;
			dst_y = DELTA;

				/* calc 1st clear_area info */
			clr_x[0]  = clr_y[0] = 0;
			clr_wd[0] = VIEW_WD;
			clr_hi[0] = DELTA;

				/* calc 2nd clear_area info if necessary */
			if (EXTRA)
			{
				n_clr	  = 2;

				clr_x[1]  = 0;
				clr_y[1]  = VIEW_HI - EXTRA;
				clr_wd[1] = VIEW_WD;
				clr_hi[1] = EXTRA;
			}
#undef DELTA
#undef EXTRA
		}
	}

#if 0
 {
int i;
printf("%s (%d) - delta: %d\n", __FILE__, __LINE__, delta);
for (i = 0; i < n_clr; i++)
printf("\t%d-> clr_x/y/wd/hi: %d, %d, %d, %d\n", i, clr_x[i], clr_y[i], clr_wd[i], clr_hi[i]);
 }
if (delta)
printf("\tcp_ar: src_x/y/wd/hi=%d %d %d %d, dst_x/y=%d,%d\n", src_x, src_y, cp_wd, cp_hi, dst_x, dst_y);
#endif

#undef ABS

	if (delta == 0)	/* re-paint the whole window */
	{
		XClearArea(DPY, WIN, 0, 0, VIEW_WD, VIEW_HI, True);
	}
	else
	{
		XEvent		xe;

		XCopyArea(DPY, WIN, WIN, gc,
				src_x, src_y, cp_wd, cp_hi, dst_x, dst_y);

		XClearArea(DPY, WIN,
				clr_x[0], clr_y[0], clr_wd[0], clr_hi[0], True);

		if (n_clr == 2)
		{
			XClearArea(DPY, WIN,
				clr_x[1], clr_y[1], clr_wd[1], clr_hi[1], True);
		}

			/* Flush the event queue and then peel off Expose
			 * events manually and dispatch them. This is for
			 * ensuring that the display got updated before
			 * returning to the caller, usually main_loop(),
			 * because the next on the queue can be MotionNotify.
			 */
		XSync(DPY, False);
		while (XCheckWindowEvent(DPY, WIN, ExposureMask, &xe))
			XtDispatchEvent(&xe);
	}

#undef VIEW_WD
#undef VIEW_HI
} /* end of ExmSWinHandleValueChange */

/*
 ****************************procedure*header*****************************
 * ExmSWinHandleResize - is used to calibrate Horizontal and/or
 *	Vertical XmScrollBarWidget(s) whenever XmScrolledWindow is
 *	resized. This procedure will also call XClearArea when one/both
 *	*x_/*y_offset is/are changed.
 *
 * min_x/min_y will be used for XmNminimum calulation for hsb/vsb... TBD
 *
 * *x_offset and *y_offset are in/out values, they represent the current
 *	offset values and will be changed with new offset value(s) when
 *	necessary.
 *
 * The req/rep pointers are from query_geometry().
 *
 * Note that this routine should be called from QueryGeometry() when
 *	its parent is XmScrolledWindowWidget.
 *************************************************************************
*/
extern XtGeometryResult
ExmSWinHandleResize(
	Widget		w,	  /* child of XmScrolledWindowWidget	*/
	Widget		hsb,	  /* Horizontal XmScrollBarWidget	*/
	Widget		vsb,	  /* Vertical XmScrollBarWidget		*/
	WidePosition	min_x,	  /* for hsb's XmNminimum calc		*/
	WidePosition	min_y,	  /* for vsb's XmNminimum calc		*/
	WideDimension	width,    /* actual size			*/
	WideDimension	height,
	Dimension	x_uom,	  /* slider units in x/y directions	*/
	Dimension	y_uom,
	WidePosition *	x_offset, /* in - current offset values,	*/
	WidePosition *	y_offset, /*	out - new offset values...	*/
	XtWidgetGeometry *req,    /* request/reply of widget geometry	*/
	XtWidgetGeometry *rep)
{
	WideDimension		new_wd, new_hi;
	XtGeometryResult	result;

	new_wd = width;
	new_hi = height;

		/* Clear the entire area only if x_offset and/or y_offset
		 * are/is changed. */
	if ( ExmSWinCalcViewSize(w, hsb, vsb, x_uom, y_uom,
				 (Dimension)0, (Dimension)0,
				 min_x, min_y,
				 x_offset, y_offset, &new_wd, &new_hi) )
	{
		XClearArea(DPY, WIN, 0, 0, new_wd, new_hi, False);
	}

	if (new_wd != req->width)
	{
		rep->request_mode |= CWWidth;
		rep->width = new_wd;
	}

	if (new_hi != req->height)
	{
		rep->request_mode |= CWHeight;
		rep->height = new_hi;
	}

	if (rep->request_mode)
		result = XtGeometryAlmost;
	else
		result = XtGeometryYes;

	return(result);

} /* end of ExmSWinHandleResize */

/*
 ****************************procedure*header*****************************
 * ExmSWinSetupScrollbars - is used to set up *	Horizontal XmScrollBarWidget,
 *	and Vertical XmScrollBarWidget.
 *
 * For both Horizontal and Vertical XmScrolledBarWidgets, the same `cb'
 *	callback will be used for the following callback resources:
 *		XmNvalueChangedCallback,
 *		XmNincrementCallback,
 *		XmNdecrementCallback,
 *		XmNpageIncrementCallback,
 *		XmNpageDecrementCallback,
 *		XmNtoTopCallback,
 *		XmNtoBottomCallback,
 *		XmNdragCallback.
 *	This callback should just call ExmSWinHandleValueChange().
 *
 * Note that there is no way for `w' to handle the clean-up. Application
 *	code will need to handle this if an application wants to destroy
 *	`w' (in this case, either HyperText or FlatIconBox), i.e.,
 *	application will need to do the following depending on
 *	the situation:
 *		a. only destroy `w' - in this case, destroy(SWin) also,
 *			or add a destroy callback when creating `w'.
 *		b. destroy `base-window/popup-window/parent of SWin',
 *			then do nothing because everything is being
 *			handled. Note that this is the current behavior
 *			in dtm.
 *	It almost means we should have two other convenience routines:
 *		ExmCreateScrolledHyperText and ExmCreateScrolledFIconBox.
 *************************************************************************
*/
extern void
ExmSWinSetupScrollbars(
	Widget		w,	/* child of XmScrolledWindowWidget	     */
	Widget		hsb,	/* Horizontal XmScrollBarWidget id	     */
	Widget		vsb,	/* Vertical XmScrollBarWidget id	     */
	XtCallbackProc	cb)	/* callback for both horiz/vert scrollbars   */
{
#if 1
#define ADD_CBS(W,CB,D)\
	XtAddCallback(W, XmNvalueChangedCallback, CB, (D));\
	XtAddCallback(W, XmNincrementCallback, CB, (D));\
	XtAddCallback(W, XmNdecrementCallback, CB, (D));\
	XtAddCallback(W, XmNpageIncrementCallback, CB, (D));\
	XtAddCallback(W, XmNpageDecrementCallback, CB, (D));\
	XtAddCallback(W, XmNtoTopCallback, CB, (D));\
	XtAddCallback(W, XmNtoBottomCallback, CB, (D));\
	XtAddCallback(W, XmNdragCallback, CB, (D))
#else
#define ADD_CBS(W,CB,D)\
	XtAddCallback(W, XmNvalueChangedCallback, CB, (D))
#endif

	ADD_CBS(hsb, cb, (XtPointer)w);
	ADD_CBS(vsb, cb, (XtPointer)w);
#undef ADD_CBS
} /* end of ExmSWinSetupScrollbars */
