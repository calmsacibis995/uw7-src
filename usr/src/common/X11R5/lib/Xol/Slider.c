/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */


#ifndef NOIDENT
#ident	"@(#)slider:Slider.c	1.73"
#endif

/*
 *************************************************************************
 *
 ****************************procedure*header*****************************
 */
/* () {}
 *	The Above template should be located at	the top	of each	file
 * to be easily	accessable by the file programmer.  If this is included
 * at the top of the file, this	comment	should follow since formatting
 * and editing shell scripts look for special delimiters.		*/


/* #includes go	here	*/
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlCursors.h>
#include <Xol/SliderP.h>
#include <Xol/GaugeP.h>

#define ClassName Slider
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed	by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */
/* Dynamic symbols */
static Dimension	(*_olmSlidercalc_leftMargin) OL_ARGS((SliderWidget));
static Dimension	(*_olmSlidercalc_rightMargin) OL_ARGS((SliderWidget));
static Dimension 	(*_olmSlidercalc_bottomMargin) OL_ARGS((SliderWidget));
static Dimension 	(*_olmSlidercalc_topMargin) OL_ARGS((SliderWidget));
static void	(*_olmSliderRecalc) OL_ARGS((SliderWidget));
static void	(*_olmOlgDrawFocusHighlights) OL_ARGS((Widget, OlDefine));

#define	_OlMSlidercalc_leftMargin	(*_olmSlidercalc_leftMargin)
#define	_OlMSlidercalc_rightMargin	(*_olmSlidercalc_rightMargin)
#define	_OlMSlidercalc_bottomMargin	(*_olmSlidercalc_bottomMargin)
#define	_OlMSlidercalc_topMargin	(*_olmSlidercalc_topMargin)
#define	_OlMSliderRecalc		(*_olmSliderRecalc)

/* private procedures		*/
static int calc_stoppos OL_ARGS((SliderWidget, int));
static int calc_nextstoppos OL_ARGS((SliderWidget, int, int));
static void highlight OL_ARGS((SliderWidget, int));
static void MoveSlider OL_ARGS((SliderWidget, int, Boolean, Boolean));
static void OlHighlightHandler OL_ARGS((Widget, OlDefine));
static void TimerEvent OL_ARGS((XtPointer, XtIntervalId *));
static void SLError OL_ARGS((SliderWidget, int, int));
static void calc_dimension OL_ARGS((SliderWidget, Dimension *, Dimension *));
static int nearest_tickval OL_ARGS((SliderWidget, int));
static void	RemoveHandler OL_ARGS((Widget, XtPointer, XtPointer));
static void	SLButtonHandler OL_ARGS((Widget, OlVirtualEvent));
static Boolean	SLActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static Boolean	ScrollKeyPress OL_ARGS((Widget, unsigned char));

/* class procedures		*/
static void		 ClassInitialize OL_NO_ARGS();
static void		 GetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));
static void		 Destroy OL_ARGS((Widget));
static void		 Initialize OL_ARGS((
					Widget, Widget, ArgList, Cardinal *));
static void		 HighlightHandler OL_ARGS((Widget, OlDefine));
static XtGeometryResult  QueryGeom OL_ARGS((Widget,
				XtWidgetGeometry *, XtWidgetGeometry *));
static void		 Realize OL_ARGS((Widget,
				 XtValueMask *, XSetWindowAttributes *));
static void		 Redisplay OL_ARGS((Widget, XEvent *, Region));
static void		 Resize OL_ARGS((Widget));
static Boolean		 SetValues OL_ARGS((Widget, Widget, Widget,
					ArgList, Cardinal *));

/* action procedures		*/
static void SelectDown OL_ARGS((Widget, OlVirtualEvent));
static void SelectUp OL_ARGS((Widget, XEvent *, String *, Cardinal *));
static void SLKeyHandler OL_ARGS((Widget, OlVirtualEvent));

/* public procedures 		*/
extern void OlSetGaugeValue OL_ARGS((Widget, int));

/*
 *************************************************************************
 *
 * Define global/static	variables and #defines,	and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


#define	SLD		sw->slider
#define	HORIZ(W)	((W)->slider.orientation == OL_HORIZONTAL)
#define	VERT(W)		((W)->slider.orientation == OL_VERTICAL)
#define IS_SLIDER(W)	(XtClass(W) == sliderWidgetClass)
#define TICKS_ON(W)	((W)->slider.tickUnit != OL_NONE)
#define	INRANGE(V,MIN,MAX) (((V) <= (MIN)) ? (MIN) : ((V) >= (MAX) ? (MAX) : (V)))
#define	ABS(V)		((V) > 0 ? (V) : -(V))
#define	MIN(X,Y)	((X) < (Y) ? (X) : (Y))
#define	MAX(X,Y)	((X) > (Y) ? (X) : (Y))
#define	PIX_TO_VAL(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1)*(M1)+(Q1-1))/(Q1)))
#define ISKBDOP		(SLD.opcode & KBD_OP)

/* Scroll Resources  Defaults */
#define DFLT_SLIDERVALUE	0
#define	DFLT_MIN 		0
#define	DFLT_MAX 		100
#define	DFLT_REPEATRATE	        100
#define	DFLT_INITIALDELAY       500
#define	DFLT_GRANULARITY        1
#define	DFLT_SCALE	        OL_DEFAULT_POINT_SIZE


/* Resource defaults */
static int	repeatRateDefault = DFLT_REPEATRATE;
static int	initialDelayDefault = DFLT_INITIALDELAY;

/*
 *************************************************************************
 * Define Translations and Actions
 ***********************widget*translations*actions**********************
 */

/* The translation table is inherited from its superclass */

static OlEventHandlerRec sl_event_procs[] = {
	{ KeyPress,	SLKeyHandler	},
	{ ButtonPress,  SLButtonHandler	},
	{ ButtonRelease,SLButtonHandler },
};

/*
 *************************************************************************
 * Define Resource list	associated with	the Widget Instance
 ****************************widget*resources*****************************
 */

#define	offset(field)  XtOffset(SliderWidget, field)

static XtResource resources[] =	{
	{XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
		offset(slider.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN },
	{XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
		offset(slider.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
	{XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
		offset(slider.sliderValue), XtRImmediate, 
		(XtPointer)DFLT_SLIDERVALUE},
	{XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
		offset(slider.orientation), XtRImmediate,
		(XtPointer)OL_VERTICAL },
	{XtNsliderMoved, XtCSliderMoved, XtRCallback, sizeof(XtPointer),
		offset(slider.sliderMoved), XtRCallback, (XtPointer)NULL },
	{XtNgranularity, XtCGranularity, XtRInt, sizeof(int),
		offset(slider.granularity), XtRImmediate, (XtPointer)0},
	{XtNrepeatRate,	XtCRepeatRate, XtRInt, sizeof(int),
		offset(slider.repeatRate), XtRInt,
		(XtPointer)&repeatRateDefault},
	{XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
		offset(slider.initialDelay), XtRInt, 
		(XtPointer)&initialDelayDefault},
	{XtNscale, XtCScale, XtRInt, sizeof(int),
		offset(slider.scale), XtRImmediate, (XtPointer)DFLT_SCALE },
	{XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
		offset(slider.warp_pointer), XtRImmediate, (XtPointer)True },

	/* resources added in OL 2.1 */
	{XtNendBoxes, XtCEndBoxes, XtRBoolean, sizeof(Boolean),
		offset(slider.endBoxes), XtRImmediate, (XtPointer)True},
	{XtNminLabel, XtCLabel, XtRString, sizeof(String),
		offset(slider.minLabel), XtRString, (XtPointer)NULL},
	{XtNmaxLabel, XtCLabel, XtRString, sizeof(String),
		offset(slider.maxLabel), XtRString, (XtPointer)NULL},
	{XtNticks, XtCTicks, XtRInt, sizeof(int),
		offset(slider.ticks), XtRImmediate, (XtPointer)0},
	{XtNtickUnit, XtCTickUnit, XtROlDefine, sizeof(OlDefine),
		offset(slider.tickUnit), XtRImmediate, (XtPointer)OL_NONE},
        {XtNdragCBType, XtCDragCBType, XtROlDefine, sizeof(OlDefine),
                offset(slider.dragtype), XtRImmediate,
                (XtPointer)(OlDefine)OL_CONTINUOUS },
        {XtNstopPosition, XtCStopPosition, XtROlDefine, sizeof(OlDefine),
                offset(slider.stoppos), XtRImmediate,
                (XtPointer)(OlDefine)OL_ALL },
	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(slider.recompute_size), XtRImmediate, (XtPointer)False},
	{XtNspan, XtCSpan, XtRDimension, sizeof(Dimension),
		offset(slider.span), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.leftMargin), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.rightMargin), XtRImmediate,(XtPointer)OL_IGNORE },
	/* for Moolit: motif mode */
	{XtNshowValue, XtCShowValue, XtRBoolean, sizeof(Boolean),
	   offset(slider.showValue), XtRImmediate, (XtPointer)True},
};

static XtResource gauge_resources[] =	{
	{XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
		offset(slider.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN },
	{XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
		offset(slider.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
	{XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
		offset(slider.sliderValue), XtRImmediate, (XtPointer)DFLT_SLIDERVALUE},
	{XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
		offset(slider.orientation), XtRImmediate,
		(XtPointer)OL_VERTICAL },
	{XtNscale, XtCScale, XtRInt, sizeof(int),
		offset(slider.scale), XtRImmediate, (XtPointer)DFLT_SCALE },
	{XtNminLabel, XtCLabel, XtRString, sizeof(String),
		offset(slider.minLabel), XtRString, (XtPointer)NULL},
	{XtNmaxLabel, XtCLabel, XtRString, sizeof(String),
		offset(slider.maxLabel), XtRString, (XtPointer)NULL},
	{XtNticks, XtCTicks, XtRInt, sizeof(int),
		offset(slider.ticks), XtRImmediate, (XtPointer)0},
	{XtNtickUnit, XtCTickUnit, XtROlDefine, sizeof(OlDefine),
		offset(slider.tickUnit), XtRImmediate, (XtPointer)OL_NONE},
	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(slider.recompute_size), XtRImmediate, (XtPointer)False},
	{XtNspan, XtCSpan, XtRDimension, sizeof(Dimension),
		offset(slider.span), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.leftMargin), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.rightMargin), XtRImmediate,(XtPointer)OL_IGNORE },
	/* for moolit: motif mode */
	{XtNshowValue, XtCShowValue, XtRBoolean, sizeof(Boolean),
	   offset(slider.showValue), XtRImmediate, (XtPointer)True},
};
#undef offset

/*
 *************************************************************************
 * Define Slider Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
SliderClassRec sliderClassRec =	{
	{
	/* core_class fields	  */
	/* superclass	      */    (WidgetClass) &primitiveClassRec,
	/* class_name	      */    "Slider",
	/* widget_size	      */    sizeof(SliderRec),
	/* class_initialize   */    ClassInitialize,
	/* class_part_init    */    NULL,
	/* class_inited	      */    FALSE,
	/* initialize	      */    Initialize,
	/* initialize_hook    */    NULL,
	/* realize	      */    Realize,
	/* actions	      */    NULL,
	/* num_actions	      */    0,
	/* resources	      */    resources,
	/* num_resources      */    XtNumber(resources),
	/* xrm_class	      */    NULLQUARK,
	/* compress_motion    */    TRUE,
	/* compress_exposure  */    TRUE,
	/* compress_enterleave*/    TRUE,
	/* visible_interest   */    FALSE,
	/* destroy	      */    Destroy,
	/* resize	      */    Resize,
	/* expose	      */    Redisplay,
	/* set_values	      */    SetValues,
	/* set_values_hook    */    NULL,
	/* set_values_almost  */    XtInheritSetValuesAlmost,
	/* get_values_hook    */    GetValuesHook,
	/* accept_focus	      */    XtInheritAcceptFocus,
	/* version	      */    XtVersion,
	/* callback_private   */    NULL,
	/* tm_table	      */    XtInheritTranslations,
	/* query_geometry     */    QueryGeom,
	/* display_accelerator*/    NULL,
	/* extension	      */    NULL,
	},
  {					/* primitive class	*/
      True,				/* focus_on_select	*/
      HighlightHandler,			/* highlight_handler   */
      NULL,				/* traversal_handler   */
      NULL,				/* register_focus      */
      SLActivateWidget,			/* activate            */
      sl_event_procs,			/* event_procs         */
      XtNumber(sl_event_procs),		/* num_event_procs     */
      OlVersion,			/* version             */
      NULL,				/* extension           */
      { NULL, 0 },			/* dyn_data	       */
      XtInheritTransparentProc,		/* transparent_proc    */
  },
	{
	/* Slider class	fields */
	/* empty		  */	0,
	}
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget	Class Record
 *************************public*class*definition*************************
 */
WidgetClass sliderWidgetClass =	(WidgetClass)&sliderClassRec;

/*
 *************************************************************************
 * Define Gauge Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
SliderClassRec gaugeClassRec =	{
	{
	/* core_class fields	  */
	/* superclass	      */    (WidgetClass) &primitiveClassRec,
	/* class_name	      */    "Gauge",
	/* widget_size	      */    sizeof(SliderRec),
	/* class_initialize   */    ClassInitialize,
	/* class_part_init    */    NULL,
	/* class_inited	      */    FALSE,
	/* initialize	      */    Initialize,
	/* initialize_hook    */    NULL,
	/* realize	      */    Realize,
	/* actions	      */    NULL,
	/* num_actions	      */    (Cardinal)0,
	/* resources	      */    gauge_resources,
	/* num_resources      */    XtNumber(gauge_resources),
	/* xrm_class	      */    NULLQUARK,
	/* compress_motion    */    FALSE,
	/* compress_exposure  */    TRUE,
	/* compress_enterleave*/    FALSE,
	/* visible_interest   */    FALSE,
	/* destroy	      */    Destroy,
	/* resize	      */    Resize,
	/* expose	      */    Redisplay,
	/* set_values	      */    SetValues,
	/* set_values_hook    */    NULL,
	/* set_values_almost  */    XtInheritSetValuesAlmost,
	/* get_values_hook    */    GetValuesHook,
	/* accept_focus	      */    NULL,
	/* version	      */    XtVersion,
	/* callback_private   */    NULL,
	/* tm_table	      */    NULL,
	/* query_geometry     */    QueryGeom,
	},
  {					/* primitive class	*/
      True,				/* focus_on_select	*/
      NULL,				/* highlight_handler   */
      NULL,				/* traversal_handler   */
      NULL,				/* register_focus      */
      NULL,				/* activate            */
      NULL,				/* event_procs         */
      0,				/* num_event_procs     */
      OlVersion,			/* version             */
      NULL,				/* extension           */
      { NULL, 0 },			/* dyn_data	       */
      XtInheritTransparentProc,		/* transparent_proc    */
  },
	{
	/* Gauge class	fields */
	/* empty		  */	0,
	}
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget	Class Record
 *************************public*class*definition*************************
 */
WidgetClass gaugeWidgetClass =	(WidgetClass)&gaugeClassRec;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */

/*
 * calculates the nearest stoppable value for a drag operation.
 */
static int
calc_stoppos OLARGLIST((sw, nearval))
	OLARG(SliderWidget, sw)
	OLGRA(int, nearval)
{
	switch(sw->slider.stoppos) {
	case OL_TICKMARK:
		nearval = nearest_tickval(sw, nearval);
		break;
	case OL_GRANULARITY:
		nearval = SLD.sliderMin + (nearval -
			  SLD.sliderMin + SLD.granularity / 2) /
			  SLD.granularity * SLD.granularity;
		break;
	}

	nearval = MIN(SLD.sliderMax, nearval);
	nearval = MAX(SLD.sliderMin, nearval);
	return(nearval);
}

static int
calc_nextstoppos OLARGLIST((sw, val, dir))
	OLARG(SliderWidget, sw)
	OLARG(int, val)
	OLGRA(int, dir)
{
	int nearval;

	nearval = calc_stoppos(sw, val);

	if (dir == 1) {
		/* next */
		if (nearval > val)
			return(nearval);

		switch(SLD.stoppos) {
		case OL_TICKMARK:
			switch(SLD.tickUnit) {
			case OL_PERCENT:
				nearval = SLD.sliderMin + (((nearval -
					  SLD.sliderMin) * 100 /
					  (SLD.sliderMax - SLD.sliderMin) +
					  SLD.ticks / 2) /
					  SLD.ticks + 1) * SLD.ticks *
					  (SLD.sliderMax - SLD.sliderMin) / 100;
				break;
			case OL_SLIDERVALUE:
				nearval = SLD.sliderMin + (nearval -
					  SLD.sliderMin + SLD.ticks / 2) /
					  SLD.ticks * SLD.ticks + SLD.ticks;
				break;
			}
			break;
		case OL_GRANULARITY:
			nearval += SLD.granularity;
			break;
		default:
			nearval += 1;
		}
	}
	else {
		/* previous */
		if (nearval < val)
			return(nearval);

		switch(SLD.stoppos) {
		case OL_TICKMARK:
			switch(SLD.tickUnit) {
			case OL_PERCENT:
				nearval = SLD.sliderMin + (((nearval -
					  SLD.sliderMin) * 100 /
					  (SLD.sliderMax - SLD.sliderMin) +
					  SLD.ticks / 2) /
					  SLD.ticks - 1) * SLD.ticks *
					  (SLD.sliderMax - SLD.sliderMin) / 100;
				break;
			case OL_SLIDERVALUE:
				nearval = SLD.sliderMin + (nearval -
					  SLD.sliderMin + SLD.ticks / 2) /
					  SLD.ticks * SLD.ticks - SLD.ticks;
				break;
			}
			break;
		case OL_GRANULARITY:
			nearval -= SLD.granularity;
			break;
		default:
			nearval -= 1;
		}
	}

	nearval = MIN(SLD.sliderMax, nearval);
	nearval = MAX(SLD.sliderMin, nearval);
	return(nearval);
}

static void
GetGCs OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
	XGCValues values;
	Pixel	focus_color;
	Boolean	has_focus;

       /* destroy old GCs */
	if (sw->slider.labelGC	!= NULL)
	   XtReleaseGC((Widget)sw, sw->slider.labelGC);

	if (sw->slider.pAttrs != (OlgAttrs *) NULL) {
		OlgDestroyAttrs (sw->slider.pAttrs);
	}

       /* get label GC	*/
	if (sw->primitive.font_color == -1)
		values.foreground = sw->primitive.foreground;
	else
		values.foreground = sw->primitive.font_color;
	values.font	  = sw->primitive.font->fid;
	sw->slider.labelGC= XtGetGC((Widget)sw,
				      (unsigned) (GCForeground | GCFont),
				      &values);

	/* get other GCs.  Worry about input focus color conflicts */
	focus_color = sw->primitive.input_focus_color;
	has_focus = sw->primitive.has_focus;

	if (has_focus)
	{
	    if (sw->primitive.foreground == focus_color ||
		sw->core.background_pixel == focus_color)
	    {
		/* input focus color conflicts with either the foreground
		 * or background color.  Reverse fg and bg for 3-D.  2-D
		 * is handled specially by making the buttons appear pressed,
		 * so use normal colors.
		 */
		if (OlgIs3d ())
		    sw->slider.pAttrs =
			OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
					sw->core.background_pixel,
					(OlgBG *)&(sw->primitive.foreground),
					False, sw->slider.scale);
		else
		    sw->slider.pAttrs =
			OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
					sw->primitive.foreground,
					(OlgBG *)&(sw->core.background_pixel),
					False, sw->slider.scale);
	    }
	    else
		/* no color conflict */
		sw->slider.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
				    sw->primitive.foreground,
				    (OlgBG *)&(focus_color),
				    False, sw->slider.scale);
	}
	else
	    /* normal coloration */
	    sw->slider.pAttrs = OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
						sw->primitive.foreground,
						(OlgBG *)&(sw->core.background_pixel),
						False, sw->slider.scale);
} /* GetGCs */

/*
 *************************************************************************
 * TimerEvent -	a function registered to trigger every time the
 *		repeatRate time	interval has elapsed. At the end of
 *		this functions it registers itself.
 *		Thus, elev is moved, ptr is warped and function	is
 *		reregistered.
 ****************************procedure*header*****************************
 */
static void
TimerEvent OLARGLIST((client_data, id))
	OLARG(XtPointer, client_data)
	OLGRA(XtIntervalId *, id)
{
	SliderWidget sw	= (SliderWidget)client_data;
	int warppoint =	0;
	int sx = 0,sy =	0,width	= 0,height = 0;
	int new_location;
	XEvent event;
	Boolean morepending = FALSE;
	Boolean docallback = TRUE;
	OlDefine currentGUI = OlGetGui();

	if (sw->slider.opcode == NOOP) {
		sw->slider.timerid = (XtIntervalId)NULL;
		return;
	}

       /* determine where to move to */
       if (sw->slider.opcode & ANCHOR)
		new_location = (SLD.opcode & DIR_INC) ?
				(HORIZ(sw) ? SLD.sliderMax : SLD.sliderMin) :
				(HORIZ(sw) ? SLD.sliderMin : SLD.sliderMax);
	else if	(sw->slider.opcode == DRAG_ELEV) {
		Window wjunk;
		int junk, winx,	winy;
		static int min;
		static int max;
		static int valueRange;
		static int pixelRange;
		int pixel;

		if (sw->slider.timerid == (XtIntervalId)NULL) {
			/* first time */
			min = sw->slider.sliderMin;
			max = SLD.sliderMax;
			valueRange = max - min;
			if (HORIZ (sw)) {
			  if (currentGUI == OL_OPENLOOK_GUI)
			    pixelRange = sw->core.width -
				sw->slider.leftPad - sw->slider.rightPad -
				2*sw->slider.anchwidth - sw->slider.elevwidth;
			   else
			    pixelRange = sw->core.width -
				sw->slider.leftPad - sw->slider.rightPad -
				2*sw->primitive.highlight_thickness -
				2*sw->primitive.shadow_thickness -
				 sw->slider.elevwidth;
			}
			else { /* vertical */
			  if (currentGUI == OL_OPENLOOK_GUI)
			    pixelRange = sw->core.height -
			        sw->slider.topPad - sw->slider.bottomPad -
				2*sw->slider.anchlen - sw->slider.elevheight;
			else
			    pixelRange = sw->core.height -
			        sw->slider.topPad - sw->slider.bottomPad -
				2*sw->primitive.highlight_thickness -
				2*sw->primitive.shadow_thickness -
				sw->slider.elevheight;
			}
		}

		XQueryPointer(XtDisplay(sw), XtWindow(sw), &wjunk, &wjunk,
				&junk, &junk, &winx, &winy,
				(unsigned int *)&junk);

		if (HORIZ(sw))
		    pixel = winx + sw->slider.dragbase;
		else
		    pixel = pixelRange - (winy + sw->slider.dragbase);
		pixel =	PIX_TO_VAL(pixel,valueRange,pixelRange)
		    + SLD.sliderMin;
		new_location = INRANGE(pixel, min, max);

		/*
		 * Handle DragCBType here.
		 */
		if (sw->slider.dragtype == OL_GRANULARITY) {
			if ((new_location != sw->slider.sliderMin) &&
			    (new_location != sw->slider.sliderMax) &&
			    (((new_location - SLD.sliderMin) %
			       SLD.granularity) != 0))
				docallback = FALSE;
		} /* OL_GRANULARITY */
		else if (sw->slider.dragtype == OL_RELEASE)
			docallback = FALSE;

		morepending = TRUE;
	} /* DRAG_TYPE */
       else {
		/* move by a unit of granularity */
		/* opcode = PAGE_INC or PAGE_DEC;
		 * given SLD.sliderPValue (the pixel offset to the elevator)
		 * and SLD.dragbase (the position of the elevator
		 * RELATIVE to the pointer - a negative number =>
		 * the pointer is in front (below or to the right) of
		 * the elevator; a positive number => the pointer is
		 * behind the elevator.
		 */
	       int step;

		if (SLD.opcode & PAGE)
			step = SLD.granularity;
		else {
			/*
			 * GRAN_DEC or GRAN_INC !!
			 * This can only happen with mouseless operations.
			 * They are used to simulate drag operation. Thus,
			 * they should honor stoppos. See below.
			 */
			step = 1;
		}

		if (!(SLD.opcode & DIR_INC))
			/* PAGE_DEC */
			step = - step;
		if (!VERT(sw))
			step = - step;

		warppoint = 1;

		if ((SLD.opcode == (GRAN_DEC | KBD_OP)) ||
		    (SLD.opcode == (GRAN_INC | KBD_OP))) {
			new_location = calc_nextstoppos(sw, SLD.sliderValue, step);
		}
		else
			new_location = SLD.sliderValue + step;
			
		new_location = MIN(SLD.sliderMax, new_location);
		new_location = MAX(SLD.sliderMin, new_location);
       }

	if (new_location != sw->slider.sliderValue) {
		MoveSlider(sw, new_location, docallback, morepending);
		if ((ISKBDOP == False) && warppoint &&
			 (SLD.warp_pointer == True)) {
			int wx,	wy;

			if (sw->slider.opcode &	DIR_INC) {
				warppoint = -3;
				if (HORIZ(sw))
					sx = SLD.sliderPValue;
				else
					sy = SLD.sliderPValue;
			}
			else {
				if (HORIZ(sw)) {
					width =	SLD.sliderPValue +
					    SLD.elevwidth;
					warppoint = SLD.elevwidth + 3;
				}
				else {
					height = SLD.sliderPValue +
					    SLD.elevheight;
					warppoint = SLD.elevheight + 3;
				}
			}

			if (HORIZ(sw)) {
				wx = SLD.sliderPValue +	warppoint;
				wy = SLD.elev_offset + SLD.elevheight / 2;
			}
			else {
				wx = SLD.elev_offset + SLD.elevwidth / 2;
				wy = SLD.sliderPValue +	warppoint;
			}
			XWarpPointer(XtDisplay(sw), XtWindow(sw),
				     XtWindow(sw), sx, sy,
				     width, height, wx,	wy);
		}
	}
	else if	(sw->slider.opcode != DRAG_ELEV) {
		SelectUp((Widget)sw,NULL,NULL,NULL);
		return;
	}

	if ((ISKBDOP == False) && !(sw->slider.opcode & ANCHOR)) {
			if (sw->slider.opcode == DRAG_ELEV) {
				SLD.timerid =OlAddTimeOut(
						(Widget)sw, 0,
						TimerEvent, (XtPointer)sw);
			}
			else if	(!sw->slider.timerid) {
				SLD.timerid = OlAddTimeOut(
						(Widget)sw, SLD.initialDelay,
						TimerEvent,(XtPointer)sw);
			}
			else {
				if (sw->slider.repeatRate > 0) {
					SLD.timerid = OlAddTimeOut(
						(Widget)sw, SLD.repeatRate,
						TimerEvent,(XtPointer)sw);
				}
			}
#ifdef NOT_USE
		}
#endif
	}
} /* TimerEvent	*/

static void
MoveSlider OLARGLIST((sw, new_location,callback, more))
	OLARG(SliderWidget, sw)
	OLARG(int, new_location)
	OLARG(Boolean,	callback)
	OLGRA(Boolean,	more)
{

	if (callback) {
		OlSliderVerify olsl;

		olsl.new_location = new_location;
		olsl.more_cb_pending = more;
		XtCallCallbacks((Widget)sw, XtNsliderMoved, (XtPointer) &olsl);

		/* copy newloc back, to maintain 2.0 behavior */
		new_location = olsl.new_location;
	}

	/*
	 * Note	that even if delta is zero, you	still need to do the stuffs
	 * below. Maybe	max or min has changed.
	 */
	sw->slider.sliderValue = new_location;
	if  (sw->slider.type ==	SB_REGULAR)
		OlgUpdateSlider ((Widget)sw, sw->slider.pAttrs, SL_POSITION);

} /* MoveSlider	*/

static void
CheckValues OLARGLIST((sw))
	OLGRA(SliderWidget, sw)
{
       int range;
	int save;
	Screen *screen = XtScreen(sw);

	if (OlGetGui() == OL_MOTIF_GUI) {
		sw->slider.endBoxes = False;
		sw->slider.minLabel = (String)NULL;
		sw->slider.maxLabel = (String)NULL;
		sw->slider.ticks = 0;
		sw->slider.tickUnit = OL_NONE;
                sw->slider.stoppos = OL_ALL;
		sw->slider.warp_pointer = FALSE;
		if (sw->primitive.shadow_thickness == 0)
			sw->primitive.shadow_thickness = 
				OlScreenPointToPixel(
			(sw->slider.orientation == OL_VERTICAL ? OL_HORIZONTAL:
			 OL_VERTICAL), 2, screen);
		if (sw->core.border_width == 0)
			sw->primitive.highlight_thickness =
				OlScreenPointToPixel(OL_HORIZONTAL, 2, screen);
		else {
			sw->primitive.highlight_thickness =
					sw->core.border_width;
			sw->core.border_width = 0;
		}
	}
	if (sw->slider.orientation != OL_HORIZONTAL) {
		if (sw->slider.orientation != OL_VERTICAL)
			SLError(sw, 0, (int)sw->slider.orientation);
		sw->slider.orientation = OL_VERTICAL;
	}

       if (sw->slider.sliderMin	>= sw->slider.sliderMax) {
		SLError(sw, 1, sw->slider.sliderMin);
	       sw->slider.sliderMin = DFLT_MIN;
	       sw->slider.sliderMax = DFLT_MAX;
       }

       range = sw->slider.sliderMax - sw->slider.sliderMin;

	save = SLD.sliderValue;
       SLD.sliderValue = INRANGE(SLD.sliderValue,SLD.sliderMin,SLD.sliderMax);
	if (save != SLD.sliderValue)
		SLError(sw, 3, save);

       if (sw->slider.scale < 1)
	       sw->slider.scale	= 1;

	if (IS_SLIDER(sw)) {
		save = SLD.granularity;
		if (SLD.granularity < 1 || SLD.granularity > range)
			SLD.granularity = (OlGetGui() == OL_OPENLOOK_GUI ?
						DFLT_GRANULARITY :
						(range / 10) );
		if (save != 0 && save != SLD.granularity)
			SLError(sw, 4, save);

       		if (sw->slider.repeatRate < 1) {
			SLError(sw, 6, sw->slider.repeatRate);
	       		sw->slider.repeatRate = DFLT_REPEATRATE;
		}
       		if (sw->slider.initialDelay < 1) {
			SLError(sw, 5, sw->slider.initialDelay);
	       		sw->slider.initialDelay = DFLT_INITIALDELAY;
		}

		if ((SLD.dragtype != OL_GRANULARITY) &&
	    	(SLD.dragtype != OL_CONTINUOUS) &&
	    	(SLD.dragtype != OL_RELEASE)) {
			SLError(sw, 12, (int)SLD.dragtype);
			SLD.dragtype = OL_CONTINUOUS;
		}

		if ((SLD.stoppos != OL_GRANULARITY) &&
	    	(SLD.stoppos != OL_TICKMARK) &&
	    	(SLD.stoppos != OL_ALL)) {
			SLError(sw, 13, (int)SLD.stoppos);
			SLD.stoppos = OL_ALL;
		}
	}
	else {
		/* force behavior for gauge widgets */
		SLD.endBoxes = FALSE;
	}

       if (SLD.ticks < 0) {
		SLError(sw, 7, SLD.ticks);
	       sw->slider.ticks = 0;
	}

	if ((SLD.span != (Dimension)OL_IGNORE) &&
	    ((int)SLD.span < 1)) {
		SLError(sw, 8, (int)sw->slider.span);
		sw->slider.span = OL_IGNORE;
	}

	if (((SLD.tickUnit != OL_PERCENT) && (SLD.tickUnit != OL_SLIDERVALUE) &&
	    (SLD.tickUnit != OL_NONE)) ||
	    ((SLD.tickUnit != OL_NONE) && (SLD.ticks == 0))) {
		SLError(sw, 9, (int)sw->slider.tickUnit);
		sw->slider.tickUnit = OL_NONE;
	}

	if (SLD.leftMargin < 0) {
		SLError(sw, 10, (int)SLD.leftMargin);
		SLD.leftMargin = OL_IGNORE;
	}

	if (SLD.rightMargin < 0) {
		SLError(sw, 11, (int)SLD.rightMargin);
		SLD.rightMargin = OL_IGNORE;
	}

} /* CheckValues */

/*
 * This function calculates the preferred dimension for an widget instance
 * based on resources such as leftMargin, rightMargin, span, ticks and labels.
 * Pointers to the desired width and height are passed in, and the values
 * are updated to the preferred dimensions.
 */
static void 
calc_dimension OLARGLIST((sw, width, height))
	OLARG(SliderWidget,  sw)
	OLARG(Dimension *, width)
	OLGRA(Dimension *, height)
{
	Dimension lpad;
	Dimension rpad;
	Dimension tpad, bpad;
	Dimension minWidth, minHeight;
	OlDefine currentGUI = OlGetGui();

	/* padding - in OL, for the ticks and labels;
	 * in Motif, for the value to be placed next to the elevator drag box.
	 * We must then figure in the border width (highlight for
	 * input focus).
	 * No additional padding is used here for the MOtif version,
	 * that is done in the calc_left, right, top, and bottom.
	 */
	/* left margin: XtNleftMargin, if given, or calculated based on
	 * labels or showValue.
	 */
	lpad = _OlMSlidercalc_leftMargin(sw);
	/* left margin: XtNleftMargin, if given, or calculated based on
	 * labels or showValue.
	 */
	rpad = _OlMSlidercalc_rightMargin(sw);
	tpad = _OlMSlidercalc_topMargin(sw);
	bpad = _OlMSlidercalc_bottomMargin(sw);

	/* Current version of OlgSizeSlider() for motif mode:
	 *  - the minWidth = the width needed for the internal of the
	 * slider (the highlight thickness and the elevator width,
	 * whether it be horizontal or vertical) + 2 * border_width
	 * for the highlight thickness.
	 * - the minHeigt - the same.
	 */
	OlgSizeSlider ((Widget)sw, sw->slider.pAttrs, &minWidth, &minHeight);
	
	if (VERT(sw)) {
		*width = minWidth;
		/* add in showValue for motif */
		if (SLD.minLabel || SLD.maxLabel || SLD.showValue) {
			/* If motif mode, then minlabel and maxlabel
			 * must be NULL already, so showValue must be true.
			 */
			if (currentGUI == OL_MOTIF_GUI)
				*width += (lpad + rpad);
			else {
				if (sw->primitive.font_list) {
					int minw = 0, maxw = 0;

					if (SLD.minLabel)
					    minw = OlTextWidth(
						sw->primitive.font_list,
						(unsigned char *)SLD.minLabel,
						strlen(SLD.minLabel));
					if (SLD.maxLabel)
					    maxw = OlTextWidth(
						sw->primitive.font_list,
						(unsigned char *)SLD.maxLabel,
						strlen(SLD.maxLabel));
					*width += (Dimension) MAX(minw, maxw);
				} /* if font_list */
				else { /* no font_list */
					int minw = 0, maxw = 0;

		 			if (SLD.minLabel)
						minw = XTextWidth(
							sw->primitive.font,
							SLD.minLabel,
							strlen(SLD.minLabel));
					if (SLD.maxLabel)
						maxw = XTextWidth(
							sw->primitive.font,
							SLD.maxLabel,
							strlen(SLD.maxLabel));
					*width += (Dimension) MAX(minw, maxw);
				} /* else (no font_list) */
			} /* else (Open Look) */
		} /* SLD.minLabel || SLD.maxLabel || showValue */
		/* still doing vertical slider/gauge */
		/* span: adjust elevator height */
		if (SLD.span != (Dimension)OL_IGNORE) {
			if (currentGUI == OL_MOTIF_GUI)
				*height = bpad + SLD.span + tpad +
					2 * sw->primitive.highlight_thickness;
			else	 /* Open Look */
				*height = bpad + SLD.span + tpad;
		}
		else { /* span not specified, just do it */
		  *height = MAX ((Dimension) *height,
				(Dimension) (bpad + minHeight + tpad));
		if (currentGUI == OL_MOTIF_GUI)
			*height += 2*sw->primitive.highlight_thickness;
		  if (SLD.minLabel && SLD.maxLabel) {
		    Dimension minlabelh;
		    Dimension maxlabelh;
		    int max;

		    /* auto-calc span */
		    minlabelh = maxlabelh =
			OlFontHeight(sw->primitive.font,
				      sw->primitive.font_list);

		    max = minlabelh + maxlabelh -
			ABS(SLD.maxTickPos - SLD.minTickPos);

		    if (max > 0)
		      /*
		       * needs more space between minTick and
		       * maxTicks to show both labels clearly.
		       */
		      *height += max;
		  }
		}
	} /* VERT */
	else {
		/* horizontal */
		*height = minHeight;
		if (SLD.minLabel || SLD.maxLabel || SLD.showValue)
		    *height += (Dimension)
			OlFontHeight(sw->primitive.font,
				      sw->primitive.font_list);

		if (currentGUI == OL_MOTIF_GUI)
			*height += (2 * sw->primitive.highlight_thickness);

		if (SLD.span != (Dimension)OL_IGNORE)
			if (currentGUI == OL_OPENLOOK_GUI)
				*width = lpad + SLD.span + rpad;
			else
				*width = lpad + SLD.span + rpad +
				  2 * sw->primitive.highlight_thickness;
		else { /* span not specified, do it! */
			if (currentGUI == OL_OPENLOOK_GUI)
			  *width = MAX ((Dimension) *width,
				      (Dimension) (lpad + minWidth + rpad));
			else
			  *width = MAX ((Dimension) *width,
			   (Dimension) (lpad + minWidth + rpad + 
				2* sw ->primitive.highlight_thickness)); 
			if (SLD.minLabel && SLD.maxLabel) {
			    Dimension minlabelw;
			    Dimension maxlabelw;
			    int max;

			    /* auto-calc span */
			    if (sw->primitive.font_list)
			    	{
				maxlabelw = OlTextWidth(sw->primitive.font_list,
					      (unsigned char *)SLD.maxLabel,
					      strlen(SLD.maxLabel)) / 2;
			    	minlabelw = OlTextWidth(sw->primitive.font_list,
					      (unsigned char *)SLD.minLabel,
					      strlen(SLD.minLabel)) / 2;
			        max = minlabelw + OlTextWidth (
				      sw->primitive.font_list, 
				      (unsigned char *)" ", 1) +
				      maxlabelw -
				      ABS(SLD.maxTickPos - SLD.minTickPos);
				}
			    else
				{
			    	maxlabelw = XTextWidth(sw->primitive.font,
						      SLD.maxLabel,
						      strlen(SLD.maxLabel)) / 2;
			    	minlabelw = XTextWidth(sw->primitive.font,
						     SLD.minLabel,
						     strlen(SLD.minLabel)) / 2;
			        max = minlabelw +
				      XTextWidth (sw->primitive.font, " ", 1) +
				      maxlabelw -
				      ABS(SLD.maxTickPos - SLD.minTickPos);
				}

			    if (max > 0)
				/*
				 * needs more space between minTick and
				 * maxTicks to show both labels clearly.
				 */
				*width += max;
			}
		}
	}
} /* calc_dimension */

static void
SLError OLARGLIST((sw, idx, value))
	OLARG(SliderWidget, sw)
	OLARG(int, idx)
	OLGRA(int, value)
{
	char error[128];
	static char *resources[] = {
		"XtNorientation",
		"XtNsliderMin",
		"XtNsliderMax",
		"XtNsliderValue",
		"XtNgranularity",
		"XtNinitialDelay",
		"XtNrepeatRate",
		"XtNticks",
		"XtNspan",
		"XtNtickUnit",
		"XtNleftMargin",
		"XtNrightMargin",
		"XtNdragCBType",
		"XtNstopPosition",
	};

	OlVaDisplayWarningMsg(XtDisplay((Widget)sw),
			      OleNinvalidResource,
			      OleTsetToDefault,
			      OleCOlToolkitWarning,
			      OleMinvalidResource_setToDefault,
			      XtName((Widget)sw),
			      OlWidgetToClassName((Widget)sw),
			      resources[idx]);
}

static void
highlight OLARGLIST((sw,invert))
	OLARG(SliderWidget, sw)
	OLGRA(int, invert)
{
	unsigned flag;
	unsigned char save_opcode = SLD.opcode; /* save */

	SLD.opcode &= (~KBD_OP);
	switch(sw->slider.opcode) {
	case ANCHOR_TOP:
		/* highlight anchor */
	        flag = SL_BEGIN_ANCHOR;
		break;
	case ANCHOR_BOT:
		/* highlight anchor */
	        flag = SL_END_ANCHOR;
		break;
	case PAGE_DEC:
	case PAGE_INC:
	default:
		flag = 0;
		/* do nothing */
		break;
	case DRAG_ELEV:
		/* highlight dragbox */
		flag = SL_DRAG;
		break;
	}

	if (!invert)
	    sw->slider.opcode = NOOP;

	if (flag)
	    OlgUpdateSlider ((Widget)sw, sw->slider.pAttrs, flag);

	SLD.opcode = save_opcode; /* restore */
} /* highlight */

static Boolean
ScrollKeyPress OLARGLIST((w, opcode))
	OLARG( Widget,		w)
	OLGRA( unsigned char,	opcode)
{
	SliderWidget sw = (SliderWidget)w;

	SLD.opcode = opcode | KBD_OP;
	highlight(sw, True);
	TimerEvent((XtPointer)sw, (XtIntervalId)NULL);
	highlight(sw, False);
	SLD.opcode = NOOP;
	return(True);
} /* ScrollKeyPress */

static Boolean
SLActivateWidget OLARGLIST((w, activation_type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	activation_type)
	OLGRA( XtPointer,	call_data)
{
	SliderWidget sw = (SliderWidget)w;
	Boolean consumed = False;

	if (SLD.orientation == (OlDefine)OL_HORIZONTAL) {
		switch(activation_type) {
		case OL_PAGELEFT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_PAGERIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_SCROLLLEFT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLRIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLLEFTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLRIGHTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
		}
	}
	else { /* OL_VERTICAL */
		switch(activation_type) {
		case OL_PAGEDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGEUP:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLUP:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLTOP:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLBOTTOM:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
		}
	}
	return(consumed);
} /* SLActivateWidget */

static void
SLButtonHandler OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{
    switch(ve->virtual_name) {
    case OL_SELECT:
	ve->consumed = True;
	if (ve->xevent->type == ButtonPress)
	    SelectDown(w, ve);
	else
	    SelectUp(w, ve->xevent, NULL, NULL);
	break;

    case OL_ADJUST:
	if (OlGetGui() == OL_MOTIF_GUI) {
	    /* Motif toggle mode */
	    ve->consumed = True;
	    if (ve->xevent->type == ButtonPress)
		SelectDown(w, ve);
	}
	break;

    case OLM_BDrag:
	/*
	 * Motif event for ADJUST
	 */
	if (OlGetGui() == OL_MOTIF_GUI) {
	    ve->consumed = True;
	    if (ve->xevent->type == ButtonPress)
		SelectDown (w, ve);
	    else  {
		/*  stop the dragging  */
		SelectUp (w, ve->xevent, NULL, NULL);
	    }
	}
	break;
    }
} /* SLButtonHandler */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	_OlAddOlDefineType ("horizontal",  OL_HORIZONTAL);
	_OlAddOlDefineType ("vertical",    OL_VERTICAL);
	_OlAddOlDefineType ("percent",     OL_PERCENT);
	_OlAddOlDefineType ("slidervalue", OL_SLIDERVALUE);
	_OlAddOlDefineType ("continuous",  OL_CONTINUOUS);
	_OlAddOlDefineType ("granularity", OL_GRANULARITY);
	_OlAddOlDefineType ("tickmark",    OL_TICKMARK);
	_OlAddOlDefineType ("release",     OL_RELEASE);
	_OlAddOlDefineType ("all",         OL_ALL);
	_OlAddOlDefineType ("none",        OL_NONE);
	_OlAddOlDefineType ("ignore",        OL_IGNORE);
	if (OlGetGui() == OL_MOTIF_GUI) {
		initialDelayDefault = 250;
		repeatRateDefault = 50;
	}
	OLRESOLVESTART
	OLRESOLVE(Slidercalc_leftMargin, _olmSlidercalc_leftMargin)
	OLRESOLVE(Slidercalc_rightMargin, _olmSlidercalc_rightMargin)
	OLRESOLVE(Slidercalc_topMargin, _olmSlidercalc_topMargin)
	OLRESOLVE(Slidercalc_bottomMargin, _olmSlidercalc_bottomMargin)
	OLRESOLVEEND(SliderRecalc, _olmSliderRecalc)
}

/*
 *************************************************************************
 *  Initialize
 ****************************procedure*header*****************************
 */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG(Widget, request)		     /*	what the client	asked for */
	OLARG(Widget, new)		     /*	what we're going to give him */
	OLARG(ArgList, args)
	OLGRA(Cardinal, *num_args)
{
	SliderWidget sw	= (SliderWidget) new;
	Dimension width;
	Dimension height;
	Dimension elevWidth, elevHeight;
	Dimension anchorWidth = 0, anchorHeight = 0;

	sw->slider.labelGC  = NULL;
	sw->slider.ticklist = NULL;
	sw->slider.opcode   = NOOP;
	sw->slider.pAttrs   = (OlgAttrs *) NULL;
	sw->slider.display_value = (char *)NULL;

	/* check for valid values */
	CheckValues(sw);

	sw->slider.timerid   = (XtIntervalId)NULL;

	/* replicate label strings */
	if (sw->slider.minLabel) {
		if (*(sw->slider.minLabel) == '\0')
			sw->slider.minLabel = NULL; /* empty string */
		sw->slider.minLabel = XtNewString(sw->slider.minLabel);
	}
	if (sw->slider.maxLabel) {
		if (*(sw->slider.maxLabel) == '\0')
			sw->slider.maxLabel = NULL; /* empty string */
		sw->slider.maxLabel = XtNewString(sw->slider.maxLabel);
	}

	/* GetGCs:
	 *   - first, destroy old GCs and OlgAttrs;
	 *   - then get font GC for the labels / text - set foreground to
	 *  primitive font_color if one exists (!= -1) else use primitive
	 *  foreground.
	 * -  Next, create slider.pAttrs (the OlgAttrs) based on whether it has
	 * focus or not, and if 3D - switch fg and bg colors if necessary..
	 */
	GetGCs(sw);

	/* calculate dimensions of anchors and elevator */
	/* A null function for Motif mode */
	if (OlGetGui() == OL_OPENLOOK_GUI) {
		OlgSizeSliderAnchor ((Widget)sw, sw->slider.pAttrs,
			     &anchorWidth, &anchorHeight);
        	sw->slider.anchwidth = anchorWidth;
        	sw->slider.anchlen = anchorHeight;
	}
	else
	        sw->slider.anchwidth = sw->slider.anchlen = 0;

	OlgSizeSliderElevator ((Widget)sw, sw->slider.pAttrs, &elevWidth, &elevHeight);
	sw->slider.elevwidth = elevWidth;
	sw->slider.elevheight = elevHeight;

	/* See if any additional padding is needed for the widget.
	 * For left and right margins, if the resource is used, then
	 * use that number;
	 * else for determine if any extra space is needed based on the
	 * min/max label length (if any).
	 * The top and bottom pads are based on the label lengths.
	 */
	SLD.leftPad = _OlMSlidercalc_leftMargin(sw);	
	SLD.rightPad = _OlMSlidercalc_rightMargin(sw);
	SLD.bottomPad = _OlMSlidercalc_bottomMargin(sw);
	SLD.topPad = _OlMSlidercalc_topMargin(sw);

	/* If either the width or height aren't supplied, 
	 * call calc_dimension():
	 *	call OlgSizeSlider(), return (minWidth,minHeight);
	 *	if Vertical slider:
	 *		if there is a min or max label,
	 *			add to the width the max width of the 2 labels.
	 *		if span is specified
	 *			SET height = span + top pad + bottom pad.
	 *			 (ignores the core height).
	 *		else (span not specified) {
	 *		  Set ht = MAX(core ht., minHt + top_pad + bot+pad)
	 *		  If minlabel AND maxlabel spec., Add to ht. the 
	 *		   minlabel Ht. (ascent + descent) + maxlabel Ht.
	 *		    - ABS (maxTickPos - minTickPos) - because more 
	 *			space is needed bet. minTick & maxTicks to show
	 *			both labels clearly.
	 *		}
	 *	if Horizontal slider:
	 *		set Height to minHeight;
	 *		if minLabel OR maxLabel
	 *			add to height the font ht. (ascent + descent);
	 *		if span is specified
	 *			Set width to span + left pad + right pad
	 *		else (span not specified) {
	 *			set width to MAX(core width, minWidth + left+
	 *			rt. pads;
	 *			If minLabel && maxLabel {
	 *			  Add to width minLabelWidth +
	 *			  maxLabelWidth + ABS(maxTickPos - minTickPos)
	 *			}
			}
	 *	
	 */
	if ((sw->core.height == 0) || (sw->core.width == 0)) {
		width = sw->core.width;
		height = sw->core.height;
		calc_dimension(sw, &width, &height);
		if (sw->core.height == 0)
			sw->core.height	= height;
		if (sw->core.width == 0)
			sw->core.width = width;
	}

	sw->core.background_pixmap = ParentRelative;

}   /* Initialize */

static void
Destroy OLARGLIST((w))
	OLGRA(Widget, w)
{
	SliderWidget sw = (SliderWidget)w;

	XtReleaseGC((Widget)sw, sw->slider.labelGC);
	if (sw->slider.pAttrs)
	    OlgDestroyAttrs (sw->slider.pAttrs);

	if (sw->slider.minLabel)
		free(sw->slider.minLabel);
	if (sw->slider.maxLabel)
		free(sw->slider.maxLabel);
	if (sw->slider.ticklist)
		free(sw->slider.ticklist);
	if (sw->slider.display_value)
		free(sw->slider.display_value);

	if (sw->slider.timerid)	{
		XtRemoveTimeOut(sw->slider.timerid);
		sw->slider.timerid = (XtIntervalId)NULL;
	}
	if (XtHasCallbacks((Widget)sw, XtNsliderMoved) == XtCallbackHasSome)
	        XtRemoveAllCallbacks((Widget)sw, XtNsliderMoved);

} /* Destroy */

/*
 * Redisplay
 */
static void
Redisplay OLARGLIST((w,event,region))
	OLARG(Widget, w)
	OLARG(XEvent, *event)
	OLGRA(Region, region)
{
	SliderWidget sw = (SliderWidget)w;

	if (!event || !event->xexpose.count)
	    OlgDrawSlider ((Widget)sw, sw->slider.pAttrs);
}

/*
 *************************************************************************
 * Resize - Reconfigures all the subwidgets to a new size.
 ****************************procedure*header*****************************
 */
static void
Resize OLARGLIST((w))
	OLGRA(Widget, w)
{
	SliderWidget sw = (SliderWidget)w;

	_OlMSliderRecalc(sw);

} /* Resize */

/*
 *************************************************************************
 *  Realize - Creates the WIndow
 ****************************procedure*header*****************************
 */
static void
Realize OLARGLIST((w, valueMask, attributes))
	OLARG(Widget,		w)
	OLARG(XtValueMask *,	valueMask)
	OLGRA(XSetWindowAttributes *, attributes)
{
	SliderWidget sw = (SliderWidget)w;

	XtCreateWindow(w, InputOutput,	(Visual	*)CopyFromParent,
	    *valueMask,	attributes );

	XDefineCursor(XtDisplay(w), XtWindow(w), OlGetStandardCursor(w));

	_OlMSliderRecalc(sw);

} /* Realize */

/*
 *************************************************************************
 * GetValuesHook - get resource data. If asking for leftMargin or
 *		   rightMargin and they happen to be OL_IGNORE, then
 *		   return actual left padding and right padding,
 *		   respectively.
 *************************************************************************
 */
static void
GetValuesHook OLARGLIST((w, args, num_args))
	OLARG(Widget,	w)
	OLARG(ArgList,	args)
	OLGRA(Cardinal, *num_args)
{
	SliderWidget sw = (SliderWidget)w;
	int n = 0;
	MaskArg mask_args[6];

	if (*num_args != 0) {
		Dimension pad;
		OlDefine save;
		
		save = SLD.leftMargin;
		SLD.leftMargin = (Dimension)OL_IGNORE;
		pad = _OlMSlidercalc_leftMargin(sw);
		SLD.leftMargin = save;
		_OlSetMaskArg(mask_args[n], XtNleftMargin, 
				pad, OL_COPY_MASK_VALUE); n++;
		_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
				OL_COPY_SIZE); n++;

		save = SLD.rightMargin;
		SLD.rightMargin = (Dimension)OL_IGNORE;
		pad = _OlMSlidercalc_rightMargin(sw);
		SLD.rightMargin = save;
		_OlSetMaskArg(mask_args[n], XtNrightMargin, 
				pad, OL_COPY_MASK_VALUE); n++;
		_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
				OL_COPY_SIZE); n++;

		if (SLD.span == (Dimension)OL_IGNORE) {
			if (VERT(sw))
				_OlSetMaskArg(mask_args[n], XtNspan,
					sw->core.height - SLD.topPad -
					SLD.bottomPad, OL_COPY_MASK_VALUE);
			else
				_OlSetMaskArg(mask_args[n], XtNspan, 
					sw->core.width - SLD.leftPad -
					SLD.rightPad, OL_COPY_MASK_VALUE);
			n++;
			_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
					OL_COPY_SIZE); n++;
		}

		if (n > 0)
			_OlComposeArgList(args, *num_args, mask_args, n,
					 NULL, NULL);
	}
} /* GetValuesHook */

/*
 *************************************************************************
 * HighlightHandler - call HighlightHandler() in Open Look mode,
 * call the OlgUpdateSlider() in Motif mode.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler OLARGLIST((w, type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	type)
{
SliderWidget sw = (SliderWidget)w;
	if (OlGetGui() == OL_OPENLOOK_GUI)
		OlHighlightHandler(w, type);
	else {
		OlgUpdateSlider((Widget)sw, 
			sw->slider.pAttrs,
			 type == OL_IN ? SL_BORDER : SL_NOBORDER);
	} /*else */
}

/*
 *************************************************************************
 * OlHighlightHandler - changes the colors when this widget gains or loses
 * focus, in Open Look mode.
 ****************************procedure*header*****************************
 */
static void
OlHighlightHandler OLARGLIST((w, type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	type)
{
	GetGCs ((SliderWidget) w);
	Redisplay (w, NULL, NULL);
} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * QueryGeom - Important routine for parents that want to know how
 *		large the slider wants to be. the thickness  (width
 *		for ver. slider) and length (height for	ver. slider)
 *		should be honored by the parent, otherwise an ugly
 *		visual will appear.
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeom OLARGLIST((w, intended, reply))
	OLARG(Widget,  w)
	OLARG(XtWidgetGeometry *, intended) /* parent's changes; may be NULL */
	OLGRA(XtWidgetGeometry *, reply)    /* child's preferred geometry; never NULL */
{
	SliderWidget sw = (SliderWidget)w;
	XtGeometryResult result;

	/* assume ok */
	result = XtGeometryYes;
	*reply = *intended;

	/* border width	has to be zero */
	if (intended->request_mode & CWBorderWidth &&
	    intended->border_width != 0)
	{
		reply->border_width = 0;
		result = XtGeometryAlmost;
	}

	/* X, Y, Sibling, and StackMode	are always ok */

	calc_dimension(sw, &reply->width, &reply->height);
	if ((intended->request_mode & CWHeight)	&&
	    (reply->height != intended->height)) {
		result = XtGeometryAlmost;
	}

	if ((intended->request_mode & CWWidth) &&
	    (reply->width != intended->width)) {
		result = XtGeometryAlmost;
	}

	return (result);
}	/* QueryGeom */

/*
 ************************************************************
 *
 *  SetValues -	This function compares the requested values
 *	to the current values, and sets	them in	the new
 *	widget.	 It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************procedure*header************************
 */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG(Widget, current)
	OLARG(Widget, request)
	OLARG(Widget, new)
	OLARG(ArgList, args)
	OLGRA(Cardinal *, num_args)
{
	SliderWidget sw	= (SliderWidget) current;
	SliderWidget newsw = (SliderWidget) new;
	int moved = 0;
	int resize = 0;
	int recalc = 0;
	int new_location;
	Boolean	needs_redisplay	= FALSE;

	/* cannot change orientation on	the fly	*/
	newsw->slider.orientation = sw->slider.orientation;

	CheckValues(newsw);

	if (newsw->slider.showValue != sw->slider.showValue) {
		recalc = 1;
		resize = 1;
	}

       if ((newsw->slider.sliderMin != sw->slider.sliderMin) ||
	   (newsw->slider.sliderMax != sw->slider.sliderMax)) {
		if (newsw->slider.showValue)
			recalc = 1;
		moved = 1;
	}
	else if (newsw->slider.sliderValue != sw->slider.sliderValue) {
		moved = 1;
	}

	if ((newsw->slider.leftMargin != sw->slider.leftMargin) ||
	    (newsw->slider.rightMargin != sw->slider.rightMargin)) {
		recalc = 1;
		resize = 1;
	}

	if (newsw->slider.recompute_size != sw->slider.recompute_size)
		resize = 1;

	if ((newsw->slider.recompute_size == True) &&
	    (newsw->slider.span != sw->slider.span)) {
		recalc = 1;
		resize = 1;
	}

	if (newsw->slider.minLabel != sw->slider.minLabel) {
		if (sw->slider.minLabel)
			XtFree(sw->slider.minLabel);
		if (newsw->slider.minLabel) {
			if (*(newsw->slider.minLabel) == '\0')
				/* empty string */
				newsw->slider.minLabel = NULL;
			else
				newsw->slider.minLabel = XtNewString(newsw->slider.minLabel);
		}
		if (((newsw->slider.minLabel != NULL) ^
		    (sw->slider.minLabel != NULL)) ||
		    (newsw->slider.minLabel && sw->slider.minLabel &&
		     strcmp(newsw->slider.minLabel, SLD.minLabel))) {
			resize = 1;
			recalc = 1;
		}
	}

	if (newsw->slider.maxLabel != sw->slider.maxLabel) {
		if (sw->slider.maxLabel)
			XtFree(sw->slider.maxLabel);
		if (newsw->slider.maxLabel) {
			if (*(newsw->slider.maxLabel) == '\0')
				/* empty string */
				newsw->slider.maxLabel = NULL;
			else
				newsw->slider.maxLabel = XtNewString(newsw->slider.maxLabel);
		}
		if (((newsw->slider.maxLabel != NULL) ^
		    (sw->slider.maxLabel != NULL)) ||
		    (newsw->slider.maxLabel && sw->slider.maxLabel &&
		     strcmp(newsw->slider.maxLabel, SLD.maxLabel))) {
			resize = 1;
			recalc = 1;
		}
	}

	if (newsw->slider.ticks != sw->slider.ticks) {
		recalc = 1;
		if ((newsw->slider.tickUnit != OL_NONE) &&
		    (sw->slider.tickUnit != OL_NONE)) {
			needs_redisplay = TRUE;
			resize = 1;
		}
	}

	if (newsw->slider.tickUnit != sw->slider.tickUnit) {
		recalc = 1;
		resize = 1;
	}

	if ((newsw->primitive.font != sw->primitive.font) ||
	    (newsw->slider.scale != sw->slider.scale)) {
		GetGCs(sw);
		resize = 1;
		recalc = 1;
	}
	if (newsw->slider.endBoxes != sw->slider.endBoxes)
		recalc = 1;

	if (newsw->primitive.font_color != sw->primitive.font_color) {
		GetGCs(newsw);
		needs_redisplay = TRUE;
	}

	if (newsw->primitive.highlight_thickness != 
				sw->primitive.highlight_thickness)
		recalc = resize = 1;

	if (resize) {
		if (newsw->slider.recompute_size &&
		      (newsw->core.width  == sw->core.width) &&
		      (newsw->core.height == sw->core.height)) {
		/* figure out new size and make a resize request */
		Dimension request_width;
		Dimension request_height;

		request_width = newsw->core.width;
		request_height = newsw->core.height;
		calc_dimension(newsw, &request_width, &request_height);
		if ((request_width  != newsw->core.width) ||
		    (request_height != newsw->core.height)) {
			newsw->core.width = request_width;
			newsw->core.height = request_height;
			recalc = 0;
			needs_redisplay = FALSE;
		}
	} /* recompute_size */
	} /* resize */

	if (recalc) {
		_OlMSliderRecalc(sw);
		needs_redisplay = TRUE;
	}


	if ((newsw->primitive.foreground != sw->primitive.foreground) ||
	    (newsw->core.background_pixel != sw->core.background_pixel)) {
	       GetGCs(newsw);
	       needs_redisplay = TRUE;
       }

	/* The slider should always inherit its parents background for
	 * the window.  If anyone has played with this, set it back.
	 */
	if (XtIsRealized ((Widget)sw) &&
	    newsw->core.background_pixmap != ParentRelative)
	{
	    newsw->core.background_pixmap = ParentRelative;
	    XSetWindowBackgroundPixmap (XtDisplay (sw), XtWindow (sw),
					ParentRelative);
	    needs_redisplay = TRUE;
	}

	/* moved != 0 ==> sliderValue, sliderMin, or sliderMax has changed. */
       if (moved && (needs_redisplay ==	FALSE) && XtIsRealized((Widget)sw)) {
		new_location = newsw->slider.sliderValue;
		newsw->slider.sliderValue = sw->slider.sliderValue;
		MoveSlider(newsw, new_location, FALSE, FALSE);
	}
	return (needs_redisplay);
}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * SelectDown -	Callback for Btn Select	Down inside Slider window.
 *
 ****************************procedure*header*****************************
 */
static void
SelectDown OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{
	XEvent *event = ve->xevent;
	SliderWidget sw = (SliderWidget)w;
        int point, delta;
	int bottomAnchorPos, topAnchorPos, elevEndPos;
        unsigned char opcode = NOOP;
        OlDefine        currentGUI = OlGetGui();

        if (currentGUI == OL_MOTIF_GUI && !(sw->primitive.has_focus))
                OlSetInputFocus(w, RevertToNone, event->xbutton.time);

	/* discard events not over the slider proper */
	if (VERT(sw)) {
		if (event->xbutton.x < sw->slider.elev_offset ||
		    event->xbutton.x >= (Position) (sw->slider.elev_offset +
						    sw->slider.elevwidth))
		    return;

        	point = event->xbutton.y;
		if (currentGUI == OL_OPENLOOK_GUI) {
		topAnchorPos = sw->slider.anchlen + SLD.topPad;
		bottomAnchorPos = sw->core.height - SLD.bottomPad -
		  sw->slider.anchlen;
		}
		else {
			topAnchorPos = sw->slider.topPad +
				sw->primitive.highlight_thickness +
				sw->primitive.shadow_thickness;
		}

		elevEndPos = sw->slider.elevheight;
		
		if (currentGUI == OL_OPENLOOK_GUI) {
			if (point < (Position) SLD.topPad ||
					point >= (Position)
					   (sw->core.height - SLD.bottomPad))
		    		return;
		}
		else
		/* If we start removing the padding if it doesn't
		 * fit -see OlgDrawSlider() - then this may fail...
		 */
			if (point < (Position)(SLD.topPad + 
					sw->primitive.highlight_thickness) ||
			point >= (Position)(sw->core.height - SLD.bottomPad -
				sw->primitive.highlight_thickness -
				sw->primitive.shadow_thickness))
					return;
	} /* VERT */
	else { /* HORIZ */
		if (event->xbutton.y < sw->slider.elev_offset ||
		    event->xbutton.y >= (Position) (sw->slider.elev_offset +
						    sw->slider.elevheight))
		    return;

        	point = event->xbutton.x;
		if (currentGUI == OL_OPENLOOK_GUI) {
			topAnchorPos = sw->slider.leftPad +
				sw->slider.anchwidth;
			bottomAnchorPos = sw->core.width -
				sw->slider.rightPad -
		    		sw->slider.anchwidth;
		}
		else { /* Motif */
			topAnchorPos = sw->slider.leftPad + 
				sw->primitive.highlight_thickness +
				sw->primitive.shadow_thickness;
		}
		elevEndPos = sw->slider.elevwidth;

		if (currentGUI == OL_OPENLOOK_GUI) {
			if (point < (Position) SLD.leftPad ||
		    		point >= (Position) (sw->core.width -
						 SLD.rightPad))
		    	return;
		}
		else {
		/* Again - if we start removing the padding if it doesn't
		 * fit -see OlgDrawSlider() - then this may fail...
		 */
		  if (point < (Position)(SLD.leftPad + 
			sw->primitive.highlight_thickness +
			sw->primitive.shadow_thickness) ||
			point >= (Position)(sw->core.width -
			  SLD.rightPad - sw->primitive.highlight_thickness -
			  sw->primitive.shadow_thickness))
				return;
		}
	} /* HORIZ */

	if (sw->slider.endBoxes == True) {
       		if (point < (int) topAnchorPos)
	       		opcode = ANCHOR_TOP;
       		else if (point >= (int) bottomAnchorPos)
	       		opcode = ANCHOR_BOT;
	}

   	/* all subsequent checks are relative to elevator */
	/* sliderPValue = offset to the elevator
	 * - an x position for a horizontal slider, a y position for a
	 * vertical  slider.
	 */
	delta = point - sw->slider.sliderPValue;
	
	/*
	 * ADJUST mode
	 */
#define  SMIN   sw->slider.sliderMin
#define  SMAX   sw->slider.sliderMax
	if (ve->virtual_name == OLM_BDrag && (OlGetGui() == OL_MOTIF_GUI)){
	    int user_range = SMAX - SMIN;
	    int slider_value;
	    int pixel_range = SLD.maxTickPos - SLD.minTickPos;

	    if (delta < 0 || (delta > elevEndPos)) {
		point -= (elevEndPos/2 + sw->primitive.shadow_thickness*2);
		slider_value =	PIX_TO_VAL(point,user_range,pixel_range)+ SMIN;
		slider_value = INRANGE(slider_value, SMIN, SMAX);
		if (VERT(sw)) {
		    /* if vertical then invert slider_value */
		    slider_value = (SMAX + SMIN) - slider_value;
		}
		MoveSlider(sw, slider_value, TRUE, FALSE);
	    }
	    opcode = DRAG_ELEV;
#undef SMIN
#undef SMAX
	}
	
	/*
	 * Motif TOGGLE mode
	 */
	else if (ve->virtual_name==OL_ADJUST && (OlGetGui() == OL_MOTIF_GUI)) {
	    if (delta < 0)
		opcode = ANCHOR_TOP;
	    else if (delta >= elevEndPos)
		opcode = ANCHOR_BOT;
	}

	/*
	 * SELECT mode
	 */
	else if (opcode == NOOP) {
	       if (delta < 0)
		       opcode =	PAGE_INC;
	       else if (delta >= elevEndPos)
		       opcode =	PAGE_DEC;
		else
		       opcode =	DRAG_ELEV;
       }

	sw->slider.opcode = opcode;
	if (currentGUI == OL_OPENLOOK_GUI)
		highlight(sw,TRUE);

       if (opcode == DRAG_ELEV)	{
		if (sw->slider.type != SB_REGULAR)
			/* if not regular slider, cannot drag */
			return;
		else {
			/* record pointer based	pos. for dragging */
			sw->slider.dragbase = SLD.sliderPValue -
			    (HORIZ(sw) ?
			     event->xbutton.x : event->xbutton.y) -
				 topAnchorPos;
		}
	}
	TimerEvent((XtPointer)sw, (XtIntervalId)NULL);
}	/* SelectDown */

/*
 *************************************************************************
 * SelectUp - Callback when Select Button is released.
 ****************************procedure*header*****************************
 */
static void
SelectUp OLARGLIST((w,event,params,num_params))
	OLARG(Widget, w)
	OLARG(XEvent *, event)
	OLARG(String *, params)
	OLGRA(Cardinal *, num_params)
{
	SliderWidget sw = (SliderWidget)w;
	int nearval;

	/* unhighlight */
	if (OlGetGui() == OL_OPENLOOK_GUI)
		highlight(sw,FALSE);

	if (sw->slider.timerid)	{
		XtRemoveTimeOut	(sw->slider.timerid);
		sw->slider.timerid = (XtIntervalId)NULL;
	}

	if (sw->slider.opcode == DRAG_ELEV) {
		sw->slider.opcode = NOOP;
		nearval = calc_stoppos(sw, SLD.sliderValue);
		MoveSlider(sw, nearval, TRUE, FALSE);
	}
	else
		sw->slider.opcode = NOOP;
}	/* SelectUp */

static void
SLKeyHandler OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
	typedef struct {
		OlVirtualName	orig_name;
		OlVirtualName	new_name;
		OlDefine	orientation;
		Boolean		get_count;
	} RemapCmd;
	static OLconst RemapCmd	remapCmds[] = {
		{ OL_MOVERIGHT,	OL_SCROLLRIGHT,	OL_HORIZONTAL,	False	},
		{ OL_MULTIRIGHT,OL_SCROLLRIGHT,	OL_HORIZONTAL,	True	},
		{ OL_MOVELEFT,	OL_SCROLLLEFT,	OL_HORIZONTAL,	False	},
		{ OL_MULTILEFT,	OL_SCROLLLEFT,	OL_HORIZONTAL,	True	},
		{ OL_MOVEUP,	OL_SCROLLUP,	OL_VERTICAL,	False	},
		{ OL_MULTIUP,	OL_SCROLLUP,	OL_VERTICAL,	True	},
		{ OL_MOVEDOWN,	OL_SCROLLDOWN,	OL_VERTICAL,	False	},
		{ OL_MULTIDOWN,	OL_SCROLLDOWN,	OL_VERTICAL,	True	}
	};
	OLconst RemapCmd *	cmds = remapCmds;
	Cardinal		i;

	for (i=0; i < XtNumber(remapCmds); ++cmds, ++i)
	{
		if (ve->virtual_name == cmds->orig_name &&
		    ((SliderWidget)w)->slider.orientation == cmds->orientation)
		{
			Cardinal count;

			ve->consumed = True;

			count = (cmds->get_count == True ?
					_OlGetMultiObjectCount(w) : 1);
			while(count--)
			{
				OlActivateWidget(w, cmds->new_name,
							(XtPointer)NULL);
			}
			break;
		}
	}
} /* SLKeyHandler() */

static int
nearest_tickval OLARGLIST((sw, val))
	OLARG(SliderWidget, sw)
	OLGRA(int, val)
{
	int new_location;

	switch(sw->slider.tickUnit) {
	case OL_PERCENT:
		new_location = SLD.sliderMin + ((val - SLD.sliderMin) * 100 /
				 (SLD.sliderMax - SLD.sliderMin) +
				 SLD.ticks/2) / SLD.ticks * SLD.ticks *
				 (SLD.sliderMax - SLD.sliderMin) / 100;
		break;
	case OL_SLIDERVALUE:
		new_location = SLD.sliderMin + (val - SLD.sliderMin +
				 SLD.ticks/2) / SLD.ticks * SLD.ticks;
		break;
	default:
		new_location = val;
		break;
	}
	return(new_location);
}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************class*procedures*****************************
 */

void
OlSetGaugeValue OLARGLIST((w, value))
	OLARG(Widget, w)
	OLGRA(int, value)
{
	SliderWidget sw = (SliderWidget)w;
	
	if (w == (Widget)NULL) {
	  OlVaDisplayWarningMsg(XtDisplay(w),
				OleNnullWidget,
				OleTflatState,
				OleCOlToolkitWarning,
				OleMnullWidget_flatState,
				"OlSetGaugeValue");
		return;
	}

	if (XtIsSubclass(w, gaugeWidgetClass) == False) {
	  OlVaDisplayWarningMsg(XtDisplay(w),
				OleNfileSlider,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileSlider_msg2,
				XtName(w));
		return;
	}

	/* check value */
       if (INRANGE(value,SLD.sliderMin,SLD.sliderMax) != value) {
	 OlVaDisplayWarningMsg(XtDisplay(w),
			       OleNfileSlider,
			       OleTmsg1,
			       OleCOlToolkitWarning,
			       OleMfileSlider_msg1,
			       value,
			       XtName(w),
			       OlWidgetToClassName(w));
		return;
	}

	MoveSlider(sw, value, FALSE, FALSE);
}
