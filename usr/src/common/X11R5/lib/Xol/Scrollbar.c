#ifndef NOIDENT
#ident	"@(#)scrollbar:Scrollbar.c	1.104"
#endif

/* #includes go here    */
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/OlCursors.h>

#define ClassName Scrollbar
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *              1. Private Procedures
 *              2. Class   Procedures
 *              3. Action  Procedures
 *              4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

/*  look and feel dependent procedures */

static Boolean (*_olmSBFindOp) OL_ARGS((Widget, XEvent *, unsigned char *));
#define FindOp	(*_olmSBFindOp)

static void (*_olmSBMakePageInd) OL_ARGS((ScrollbarWidget));
#define MakePageInd	(*_olmSBMakePageInd)

static void (*_olmSBUpdatePageInd) OL_ARGS((ScrollbarWidget, Boolean, Boolean));
#define UpdatePageInd	(*_olmSBUpdatePageInd)

static Boolean (*_olmSBMenu) OL_ARGS((Widget, XEvent *));
#define PopupMenu	(*_olmSBMenu)

static Widget (*_olmSBCreateMenu) OL_ARGS((Widget, OlDefine));
Widget OlSBCreateMenu OL_ARGS((Widget, OlDefine));

/* private procedures           */
static void highlight OL_ARGS((ScrollbarWidget, int));
void _OlSBMoveSlider
	OL_ARGS((ScrollbarWidget, OlScrollbarVerify *, Boolean, Boolean));

/* this routine is shared by both a XtWorkProc and a XtTimeOutHandler */
static Boolean TimerEvent OL_ARGS((XtPointer));   
static void SBError OL_ARGS((ScrollbarWidget, char *));
void _OlSBGetGCs OL_ARGS((ScrollbarWidget));

/* class procedures             */
static void		ClassInitialize OL_NO_ARGS();
static void 		Destroy OL_ARGS((Widget));
static void		GetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));
static void		Initialize
				OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static XtGeometryResult QueryGeom
		OL_ARGS((Widget, XtWidgetGeometry *, XtWidgetGeometry *));
static void		 Realize
			OL_ARGS((Widget, Mask *, XSetWindowAttributes *));
static void		 Redisplay OL_ARGS((Widget, XEvent *, Region));
static void		 Resize OL_ARGS((Widget));
static Boolean		 SetValues
			OL_ARGS((Widget, Widget, Widget, ArgList, Cardinal *));

/* action procedures            */
static void SelectDown OL_ARGS((Widget, XEvent *, String *, Cardinal *));
static void SelectUp OL_ARGS((Widget, XEvent *, String *, Cardinal *));
static void UnmapEventHandler OL_ARGS((Widget, XtPointer, XEvent *, Boolean*));

/* stuff for mouseless operation */
static Boolean	SBActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void SBButtonHandler OL_ARGS((Widget, OlVirtualEvent));
static void SBKeyHandler OL_ARGS((Widget, OlVirtualEvent));
static Boolean ScrollKeyPress OL_ARGS((Widget, unsigned char));

#define OL_N_EVENT_PROCS	XtNumber(sb_event_procs)-1 /* See ClassInit */
static OlEventHandlerRec sb_event_procs[] =
{
	{ ButtonPress,	SBButtonHandler },
	{ ButtonRelease,SBButtonHandler },
	{ KeyPress,	SBKeyHandler	},	/* Have to be the last...   */
						/* because it's only applied*/
						/* to Motif...		    */
};

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

extern char *OlGetMessage();
char olmsgbuf[BUFSIZ];

#define SWB             sw->scroll
#define ISKBDOP		(SWB.opcode & KBD_OP)
#define HORIZ(W)        ((W)->scroll.orientation == OL_HORIZONTAL)
#define INRANGE(V,MIN,MAX) (((V) <= (MIN)) ? (MIN) : ((V) >= (MAX) ? (MAX) : (V)))
#define MIN(X,Y)        ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y)        ((X) > (Y) ? (X) : (Y))
#define PIX_TO_VAL(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1)*(M1)+(Q1-1))/(Q1)))
#define VAL_TO_PIX(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1))*(M1)/(Q1)))
#define MAXSLIDERVAL(W)	((W)->scroll.sliderMax - (W)->scroll.proportionLength)
/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));


/* Scroll Resources  Defaults */
#define DFLT_MIN 0
#define DFLT_MAX 100
#define DFLT_SCALE             OL_DEFAULT_POINT_SIZE

static Dimension DefaultShadowThickness = (Dimension) 0;
static int DefaultRepeatRate = (int) 100;
static int DefaultInitialDelay = (int) 500;
static Boolean DefaultPointerWarping = True;

/*
 *************************************************************************
 * Define Translations and Actions
 ***********************widget*translations*actions**********************
 */

/* mouseless: The Translation Table is inherited from its superclass */

/*
 *************************************************************************
 * Define Resource list associated with the Widget Instance
 ****************************widget*resources*****************************
 */

#define offset(field)  XtOffset(ScrollbarWidget, field)

static XtResource resources[] = {
        {XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
              offset(primitive.traversal_on), XtRImmediate, (XtPointer)FALSE },
        {XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
                offset(scroll.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN},
        {XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
                offset(scroll.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
        {XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
                offset(scroll.sliderValue), XtRImmediate, (XtPointer)0 },
        {XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
                offset(scroll.orientation), XtRImmediate,
		(XtPointer)OL_VERTICAL },
        {XtNgranularity, XtCGranularity, XtRInt, sizeof(int),
                offset(scroll.granularity), XtRImmediate, (XtPointer)1 },
        {XtNsliderMoved, XtCSliderMoved, XtRCallback, sizeof(XtPointer),
                offset(scroll.sliderMoved), XtRCallback, (XtPointer)NULL },
        {XtNproportionLength, XtCProportionLength, XtRInt, sizeof(int),
                offset(scroll.proportionLength), XtRImmediate,
		(XtPointer)(DFLT_MAX - DFLT_MIN) },
        {XtNshowPage, XtCShowPage, XtROlDefine, sizeof(OlDefine),
                offset(scroll.showPage), XtRImmediate, (XtPointer)OL_NONE },
        {XtNcurrentPage, XtCCurrentPage, XtRInt, sizeof(int),
                offset(scroll.currentPage), XtRImmediate, (XtPointer)1 },
        {XtNrepeatRate, XtCRepeatRate, XtRInt, sizeof(int),
                offset(scroll.repeatRate), XtRInt,
		(XtPointer)&DefaultRepeatRate },
        {XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
                offset(scroll.initialDelay), XtRInt,
		(XtPointer)&DefaultInitialDelay },
        {XtNscale, XtCScale, XtRInt, sizeof(int),
                offset(scroll.scale), XtRImmediate, (XtPointer)DFLT_SCALE },
        {XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
                offset(scroll.warp_pointer), XtRBoolean,
		(XtPointer) &DefaultPointerWarping},

	/* resources added in OL 2.1 */
        {XtNdragCBType, XtCDragCBType, XtROlDefine, sizeof(OlDefine),
                offset(scroll.dragtype), XtRImmediate, 
		(XtPointer)OL_CONTINUOUS },
        {XtNstopPosition, XtCStopPosition, XtROlDefine, sizeof(OlDefine),
                offset(scroll.stoppos), XtRImmediate, 
		(XtPointer)OL_ALL },
	{ XtNshadowThickness, XtCShadowThickness, XtRDimension,
		sizeof(Dimension), offset(primitive.shadow_thickness),
		XtRDimension, (XtPointer)&DefaultShadowThickness },
	{ XtNmenuPane, XtCMenuPane, XtRWidget,
		sizeof(Widget), offset(scroll.popup),
		XtRImmediate, (XtPointer)NULL },

};
#undef offset

/*
 *************************************************************************
 * Define Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
ScrollbarClassRec scrollbarClassRec = {
        {
        /* core_class fields      */
        /* superclass         */    (WidgetClass) &primitiveClassRec,
        /* class_name         */    "Scrollbar",
        /* widget_size        */    sizeof(ScrollbarRec),
        /* class_initialize   */    ClassInitialize,
        /* class_part_init    */    NULL,
        /* class_inited       */    FALSE,
        /* initialize         */    Initialize,
        /* initialize_hook    */    NULL,
        /* realize            */    Realize,
        /* actions            */    NULL,
        /* num_actions        */    0,
        /* resources          */    resources,
        /* num_resources      */    XtNumber(resources),
        /* xrm_class          */    NULLQUARK,
        /* compress_motion    */    TRUE,
        /* compress_exposure  */    TRUE,
        /* compress_enterleave*/    TRUE,
        /* visible_interest   */    FALSE,
        /* destroy            */    Destroy,
        /* resize             */    Resize,
        /* expose             */    Redisplay,
        /* set_values         */    SetValues,
        /* set_values_hook    */    NULL,
        /* set_values_almost  */    XtInheritSetValuesAlmost,
        /* get_values_hook    */    GetValuesHook,
        /* accept_focus       */    XtInheritAcceptFocus,
        /* version            */    XtVersion,
        /* callback_private   */    NULL,
        /* tm_table           */    XtInheritTranslations,
        /* query_geometry     */    (XtGeometryHandler)QueryGeom,
	/* display_accelerator*/    NULL,
	/* extension	      */    NULL,
        },
/* changed for mouseless operation */
  {					/* primitive class	*/
      True,				/* focus_on_select	*/
      NULL,				/* highlight_handler	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      SBActivateWidget,			/* activate		*/ 
      sb_event_procs,			/* event_procs		*/
      XtNumber(sb_event_procs),		/* num_event_procs, See ClassInit*/
      OlVersion,			/* version		*/
      NULL,				/* extension		*/
      { NULL, 0 },			/* dyn_data		*/
      XtInheritTransparentProc		/* transparent_proc	*/
  },
        {
        /* Scrollbar class fields */
        /* empty                  */    0,
        }
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget Class Record
 *************************public*class*definition*************************
 */
WidgetClass scrollbarWidgetClass = (WidgetClass)&scrollbarClassRec;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _OlSBGetGCs - this routine gets the normal GC for the Scrollbar
 ****************************procedure*header*****************************
 */
void
_OlSBGetGCs OLARGLIST((sw))
	OLGRA(ScrollbarWidget, sw)
{
        XGCValues values;
	Pixel	focus_color;
	Boolean	has_focus;

       /* destroy old GCs */
        if (sw->scroll.textGC != NULL)
           XtReleaseGC((Widget)sw, sw->scroll.textGC);

	if (sw->scroll.pAttrs != (OlgAttrs *) NULL) {
		OlgDestroyAttrs (sw->scroll.pAttrs);
	}

	/* get new GCs.  The page indicator never has to worry about
	 * input focus color and color conflicts.
	 */
        values.foreground = sw->primitive.foreground;
	values.font	  = sw->primitive.font->fid;
        sw->scroll.textGC = XtGetGC((Widget)sw,
				    (unsigned) (GCForeground | GCFont),
				    &values);

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
		    sw->scroll.pAttrs =
			OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
					sw->core.background_pixel,
					(OlgBG *)&(sw->primitive.foreground),
					False, sw->scroll.scale);
		else
		    sw->scroll.pAttrs =
			OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
					sw->primitive.foreground,
					(OlgBG *)&(sw->core.background_pixel),
					False, sw->scroll.scale);
	    }
	    else
		/* no color conflict */
		sw->scroll.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
				    sw->primitive.foreground,
				    (OlgBG *)&(focus_color),
				    False, sw->scroll.scale);
	}
	else
	    /* normal coloration */
	    sw->scroll.pAttrs = OlgCreateAttrs (XtScreenOfObject ((Widget)sw),
						sw->primitive.foreground,
						(OlgBG *)&(sw->core.background_pixel),
						False, sw->scroll.scale);

} /* END OF _OlSBGetGCs() */

/*
 *************************************************************************
 * TimerEvent - a function registered to trigger every time the
 *              repeatRate time interval has elapsed. At the end of
 *              this functions it registers itself.
 *              Thus, elev is moved, ptr is warped and function is
 *              reregistered.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
TimerEvent OLARGLIST((client_data))
	OLGRA(XtPointer, client_data)
{
        ScrollbarWidget sw = (ScrollbarWidget)client_data;
        OlScrollbarVerify olsb;
        int warppoint = 0;
	int sx = 0,sy = 0,width = 0,height = 0;
	Boolean morepending = FALSE;
	Boolean docallback = TRUE;
	Boolean stop = TRUE;

	if (sw->scroll.opcode == NOOP) {
		sw->scroll.timerid = (XtIntervalId)NULL;
		return(stop);
	}

       /* determine where to move to */
       if (sw->scroll.opcode & ANCHOR)
                olsb.new_location = (sw->scroll.opcode & DIR_INC) ?
				    SWB.sliderMax - SWB.proportionLength :
                                    sw->scroll.sliderMin;
	else if (sw->scroll.opcode == DRAG_ELEV) {
		Window wjunk;
		int junk, winx, winy;
		static int minVal;
		static int maxVal;
		static int valueRange;
		static int pixelRange;
		int pixel;	

		if (sw->scroll.timerid == (XtIntervalId)NULL) {
			/* first time */
			minVal = sw->scroll.sliderMin;
			maxVal = SWB.sliderMax - SWB.proportionLength;
			valueRange = maxVal - minVal;
			if (HORIZ (sw))
			    pixelRange = sw->core.width -
				sw->scroll.anchwidth*2 - sw->scroll.elevwidth -
				sw->primitive.shadow_thickness*2;
			else
			    pixelRange = sw->core.height -
				sw->scroll.anchlen*2 - sw->scroll.elevheight -
				sw->primitive.shadow_thickness*2;
		}

		XQueryPointer(XtDisplay(sw), XtWindow(sw), &wjunk, &wjunk,
				&junk, &junk, &winx, &winy, (unsigned *)&junk);

		pixel = (HORIZ(sw) ? winx : winy) + sw->scroll.dragbase;
		pixel = PIX_TO_VAL(pixel,valueRange,pixelRange);
		olsb.new_location = INRANGE(pixel + SWB.sliderMin,
					    minVal, maxVal);

		/*
		 * Handle DragCBType here.
		 */
		if (sw->scroll.dragtype == OL_GRANULARITY) {
		    	if ((olsb.new_location != sw->scroll.sliderMin) &&
		            (olsb.new_location != MAXSLIDERVAL(sw)) &&
			    (((olsb.new_location - SWB.sliderMin) %
			       SWB.granularity) != 0))
				docallback = FALSE;
		} /* OL_GRANULARITY */
		else if (sw->scroll.dragtype == OL_RELEASE)
			docallback = FALSE;

		morepending = TRUE;
	} /* DRAG_TYPE */
       else {
               int step;

		/*
		 * warppoint has dual roles here.
		 * For page operation, warppoint is the relative amount
		 * in pixels to move. For other operations, warppoint is
		 * a boolean to indicate warping is needed.
		 */
		if (sw->scroll.opcode & PAGE) {
               		step = SWB.proportionLength;
			warppoint = -3;
		}
		else {
               		step = SWB.granularity;
			warppoint = 1;
		}
               if (sw->scroll.opcode & DIR_INC) {
                       warppoint = (HORIZ (sw) ? sw->scroll.elevwidth :
			   sw->scroll.elevheight) + 2;
                       olsb.new_location = MIN(SWB.sliderMax - SWB.proportionLength,
                                               SWB.sliderValue + step);
               }
               else {
                       olsb.new_location = MAX(SWB.sliderMin,
                                               SWB.sliderValue - step);
               }
       }

        if (olsb.new_location != sw->scroll.sliderValue) {
		int save;

		save = sw->scroll.sliderPValue;
                _OlSBMoveSlider(sw, &olsb, docallback, morepending);
/* make sure it's not a mouseless operation */
                if ((ISKBDOP == False) && olsb.ok && warppoint &&
			(SWB.warp_pointer == True)) {
                        int wx, wy;

			if (sw->scroll.opcode & DIR_INC) {
				if (HORIZ(sw))
					width = SWB.sliderPValue+SWB.elevwidth;
				else
					height = SWB.sliderPValue +
					    SWB.elevheight;
			}
			else {
				if (HORIZ(sw))
					sx = SWB.sliderPValue;
				else
					sy = SWB.sliderPValue;
			}

			if (sw->scroll.opcode & PAGE) {
				if (HORIZ(sw)) {
					wx = SWB.sliderPValue + warppoint;
					wy = sw->core.height / 2;
				}
				else {
					wx = sw->core.width / 2;
					wy = SWB.sliderPValue + warppoint;
				}
                        	XWarpPointer(XtDisplay(sw), XtWindow(sw),
					     XtWindow(sw), sx, sy,
					     width, height, wx, wy);
			}
			else {
				wx = HORIZ(sw) ? SWB.sliderPValue - save : None;
				wy = HORIZ(sw) ? None : SWB.sliderPValue - save;
                        	XWarpPointer(XtDisplay(sw), XtWindow(sw), None,
                                     	0, 0, 0, 0, wx, wy);
			}
                }
        }
        else if (sw->scroll.opcode != DRAG_ELEV) {
		SelectUp((Widget)sw, NULL, NULL, NULL);
                return(stop);
	}

/* make sure it's not a mouseless operation */
        if ((ISKBDOP == False) && !(sw->scroll.opcode & ANCHOR)) {
		/*
		 * check for button release here so it seems
		 * more responsive to the user.
		 */
		if (sw->scroll.opcode == DRAG_ELEV) {
		    if (!SWB.workid)
			SWB.workid = XtAppAddWorkProc(
				     XtWidgetToApplicationContext((Widget)sw),
						      TimerEvent, sw);
		    stop = FALSE;
		}
		else if (sw->scroll.timerid == (XtIntervalId)NULL) {
                            SWB.timerid = OlAddTimeOut(
			     (Widget)sw, SWB.initialDelay,
			     (XtTimerCallbackProc)TimerEvent,(XtPointer)sw);
                }
                else if (sw->scroll.repeatRate > 0) {
			    SWB.timerid = OlAddTimeOut(
			     (Widget)sw, SWB.repeatRate,
			     (XtTimerCallbackProc)TimerEvent,(XtPointer)sw);
                }
        }
	return(stop);
} /* TimerEvent */

/* call callback routine only if rel > granularity */
void
_OlSBMoveSlider OLARGLIST((sw, olsb, callback, more))
	OLARG(ScrollbarWidget, sw)
	OLARG(OlScrollbarVerify *, olsb)
	OLARG(Boolean, callback)
	OLGRA(Boolean, more)
{
        olsb->delta = olsb->new_location - sw->scroll.sliderValue;
        olsb->ok = TRUE;

	if (callback) {
               	olsb->slidermin = sw->scroll.sliderMin;
               	olsb->slidermax = sw->scroll.sliderMax;
               	olsb->more_cb_pending = more;
               	XtCallCallbacks((Widget)sw, XtNsliderMoved, (XtPointer) olsb);
	}

	/*
	 * Note that even if delta is zero, you still need to do the stuffs
	 * below. Maybe max, min, proportion length has changed.
	 */
        if (olsb->ok) {
		sw->scroll.sliderValue = olsb->new_location;
		if  (sw->scroll.type == SB_REGULAR)
                       	OlgUpdateScrollbar ((Widget)sw, sw->scroll.pAttrs,
					    SB_POSITION);

		/* update page indicator? */
		if ((sw->scroll.showPage != OL_NONE) &&
		    (sw->scroll.opcode == DRAG_ELEV))
			UpdatePageInd(sw, True, True);
	}
} /* _OlSBMoveSlider */

static void
CheckValues OLARGLIST((sw))
	OLGRA(ScrollbarWidget, sw)
{
	int range;
	int save;

        if (sw->scroll.orientation != OL_HORIZONTAL) {
		if (sw->scroll.orientation != OL_VERTICAL)
			SBError(sw,XtNorientation);
                sw->scroll.orientation = OL_VERTICAL;
		if ((sw->scroll.showPage != OL_NONE) &&
		    (sw->scroll.showPage != OL_LEFT) &&
		    (sw->scroll.showPage != OL_RIGHT)) {
			SBError(sw,XtNshowPage);
			sw->scroll.showPage = OL_NONE;
		}
	}
	else {
		if (sw->scroll.showPage != OL_NONE) {
			SBError(sw,XtNshowPage);
			sw->scroll.showPage = OL_NONE;
		}
	}

       if (sw->scroll.sliderMin >= sw->scroll.sliderMax) {
		SBError(sw,XtNsliderMin);
               sw->scroll.sliderMin = DFLT_MIN;
               sw->scroll.sliderMax = DFLT_MAX;
       }

	
       range = sw->scroll.sliderMax - sw->scroll.sliderMin;

	save = sw->scroll.proportionLength;
       sw->scroll.proportionLength = INRANGE(sw->scroll.proportionLength,1,range);
	if (save != sw->scroll.proportionLength)
		SBError(sw,XtNproportionLength);
	
	save = sw->scroll.granularity;
       sw->scroll.granularity = INRANGE(sw->scroll.granularity,1,range);
	if (save != sw->scroll.granularity)
		SBError(sw,XtNgranularity);

	save = sw->scroll.sliderValue;
       sw->scroll.sliderValue = INRANGE(sw->scroll.sliderValue,
                                        sw->scroll.sliderMin,
                                        sw->scroll.sliderMax - sw->scroll.proportionLength);
	if (save != sw->scroll.sliderValue)
		SBError(sw,XtNsliderValue);

       if (sw->scroll.scale < 1)
               sw->scroll.scale = 1;

       if (sw->scroll.repeatRate < 1) {
		SBError(sw,XtNrepeatRate);
		sw->scroll.repeatRate = (int) DefaultRepeatRate;
	}
       if (sw->scroll.initialDelay < 1) {
		SBError(sw,XtNinitialDelay);
		sw->scroll.initialDelay = (int) DefaultInitialDelay;
	}

	if ((sw->scroll.dragtype != OL_GRANULARITY) &&
	    (sw->scroll.dragtype != OL_CONTINUOUS)  &&
	    (sw->scroll.dragtype != OL_RELEASE)) {
		SBError(sw,XtNdragCBType);
		sw->scroll.dragtype = OL_CONTINUOUS;
	}

	if ((sw->scroll.stoppos != OL_GRANULARITY) &&
	    (sw->scroll.stoppos != OL_ALL)) {
		SBError(sw,XtNstopPosition);
		sw->scroll.stoppos = OL_ALL;
	}
} /* CheckValues */

static void
SBError OLARGLIST((sw,resname))
	OLARG(ScrollbarWidget, sw)
	OLGRA(char *, resname)
{
	OlVaDisplayWarningMsg(	XtDisplay(sw),
		OleNfileScrollbar,
		OleTmsg1,
		OleCOlToolkitWarning,
		OleMfileScrollbar_msg1,
		resname);
}

static void
highlight OLARGLIST((sw,invert))
	OLARG(ScrollbarWidget, sw)
	OLGRA(int, invert)
{
	unsigned flag;
	unsigned char	save_opcode;

/* mouseless: save opcode, and then turn the KBD_OP bit off,
   recover it before return */
	save_opcode = SWB.opcode;
	SWB.opcode &= (~KBD_OP);

        switch(sw->scroll.opcode) {
        case ANCHOR_TOP:
                /* highlight top/left anchor */
	        flag = SB_BEGIN_ANCHOR | SB_PREV_ARROW;
		break;

        case ANCHOR_BOT:
                /* highlight bottom/right anchor */
		flag = SB_END_ANCHOR | SB_NEXT_ARROW;
                break;

        case PAGE_DEC:
        case PAGE_INC:
        case NOOP:
                /* do nothing */
		flag = 0;
                break;

        case GRAN_DEC:
                /* highlight top/left arrow */
		flag = SB_PREV_ARROW;
		break;

        case GRAN_INC:
                /* highlight bottom/right arrow */
		flag = SB_NEXT_ARROW;
		break;

        case DRAG_ELEV:
                /* highlight dragbox */
		flag = SB_DRAG;
		break;
        }

	if (!invert)
	    sw->scroll.opcode = NOOP;

	if (flag)
	    OlgUpdateScrollbar ((Widget)sw, sw->scroll.pAttrs, flag);

	/* show/unshow page indicator */
	if ((SWB.showPage != OL_NONE) && (flag == SB_DRAG)) {
		if (invert) {
			/* move page indicator */
			UpdatePageInd(sw, False, True);
			if (sw->scroll.page_ind != NULL)
				XtPopup(sw->scroll.page_ind,XtGrabNone);
			/* draw page indicator */
			UpdatePageInd(sw, True, False);
		}
		else {
			/* unmap page indicator */
			if (sw->scroll.page_ind != NULL)
				XtPopdown(sw->scroll.page_ind);
		}
	}
	SWB.opcode = save_opcode;
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */
/*
************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *********************procedure*header************************
 */
static void
ClassInitialize ()
{
	ScrollbarClassRec * wc = (ScrollbarClassRec *) scrollbarWidgetClass;

	_OlAddOlDefineType ("horizontal",  OL_HORIZONTAL);
	_OlAddOlDefineType ("vertical",    OL_VERTICAL);
	_OlAddOlDefineType ("left",        OL_LEFT);
	_OlAddOlDefineType ("right",       OL_RIGHT);
	_OlAddOlDefineType ("continuous",  OL_CONTINUOUS);
	_OlAddOlDefineType ("granularity", OL_GRANULARITY);
	_OlAddOlDefineType ("release",     OL_RELEASE);
	_OlAddOlDefineType ("all",         OL_ALL);
	_OlAddOlDefineType ("none",        OL_NONE);

	OLRESOLVESTART
	OLRESOLVE(SBFindOp, _olmSBFindOp)
	OLRESOLVE(SBHighlightHandler, wc->primitive_class.highlight_handler)
	OLRESOLVE(SBMakePageInd, _olmSBMakePageInd)
	OLRESOLVE(SBMenu, _olmSBMenu)
	OLRESOLVE(SBCreateMenu, _olmSBCreateMenu)
	OLRESOLVEEND(SBUpdatePageInd, _olmSBUpdatePageInd)

	if (OlGetGui() == OL_MOTIF_GUI)  {
		DefaultShadowThickness = 2;
		DefaultPointerWarping = False;
		DefaultRepeatRate = 50;
		DefaultInitialDelay = 250;
	}
	else {
		scrollbarClassRec.primitive_class.num_event_procs =
						OL_N_EVENT_PROCS;
	}

} /* END OF ClassInitialize */

/*
 *************************************************************************
 * GetValuesHook - gets subresource data
 ****************************procedure*header*****************************
 */
static void
GetValuesHook OLARGLIST((w, args, num_args))
        OLARG(Widget, w)                   /* menu shell widget id         */
        OLARG(ArgList, args)               /* Arg List for Menu            */
        OLGRA(Cardinal *, num_args)        /* number of Args               */
{
        ScrollbarWidget sw = (ScrollbarWidget) w;
        MaskArg         mask_args[2];
        Widget *        pane = NULL;

	if (OlGetGui() == OL_OPENLOOK_GUI)  {
        	if (*num_args != 0) {
               		_OlSetMaskArg(mask_args[0], XtNmenuPane, &pane,
				OL_COPY_SOURCE_VALUE);
               		_OlSetMaskArg(mask_args[1], NULL, sizeof(Widget *),
				OL_COPY_SIZE);
               		_OlComposeArgList(args, *num_args, mask_args, 2, NULL, NULL);

               		if (pane != NULL) {
                       		if (sw->scroll.popup == NULL)  {
					*pane = sw->scroll.popup =
						OlSBCreateMenu(w,
							sw->scroll.orientation);
               			}
        		}
		}
	}
} /* END OF GetValuesHook() */

/*
 *************************************************************************
 *  Initialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG(Widget, request)              /* what the client asked for */
	OLARG(Widget, new)                  /* what we're going to give him */
	OLARG(ArgList, args)
	OLGRA(Cardinal *, num_args)
{
        ScrollbarWidget sw = (ScrollbarWidget) new;
	Dimension	dtmp1, dtmp2;

        sw->scroll.opcode   = NOOP;
/* new fields for mouseless operation */
	sw->scroll.here_to_lt_btn = NULL;
	sw->scroll.lt_to_here_btn = NULL;

        /* check for valid values */
        CheckValues(sw);

        sw->scroll.timerid   = (XtIntervalId)NULL;
        sw->scroll.workid   = 0;
        sw->scroll.page_ind  = NULL;
        sw->scroll.textGC    = NULL;
        sw->scroll.pAttrs    = NULL;

        sw->scroll.previous = sw->scroll.sliderValue;

	_OlSBGetGCs (sw);
        OlgSizeScrollbarAnchor ((Widget)sw, sw->scroll.pAttrs, &dtmp1, &dtmp2);
	sw->scroll.anchwidth = (unsigned char) dtmp1;
	sw->scroll.anchlen   = (unsigned char) dtmp2;

	if (sw->core.height == 0 || sw->core.width == 0)
	{
	    sw->scroll.type = SB_ABBREVIATED;
	    OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs, SB_ABBREVIATED,
				&(sw->scroll.elevwidth),
				&(sw->scroll.elevheight));
	    if (sw->core.height == 0)
	    {
		if (HORIZ(sw))
		    sw->core.height = sw->scroll.anchlen +
			OlgGetVerticalStroke (sw->scroll.pAttrs) * 2 +
			(sw->primitive.shadow_thickness * 2);
		else
		    sw->core.height = (sw->scroll.anchlen * 2) +
			sw->scroll.elevheight +
			(sw->primitive.shadow_thickness * 2);
	    }

	    if (sw->core.width == 0)
	    {
		if (HORIZ (sw))
		    sw->core.width = sw->scroll.anchwidth * 2 +
			sw->scroll.elevheight +
			(sw->primitive.shadow_thickness * 2);
		else
		    sw->core.width = SWB.anchwidth +
			OlgGetHorizontalStroke (sw->scroll.pAttrs) * 2 +
			(sw->primitive.shadow_thickness * 2);
	    }
	}

	/* 
	 * inherit parent's background.
	 */
	sw->core.background_pixmap = ParentRelative;

	/* don't bother checking for XtNfontColor as a dynamic resource */
	sw->primitive.dyn_flags |= OL_B_PRIMITIVE_FONTCOLOR;

	/* need to keep track of unmap event */
	XtAddEventHandler((Widget)sw,StructureNotifyMask,False,
			  (XtEventHandler) UnmapEventHandler, NULL);
}   /* Initialize */

static void
Destroy OLARGLIST((w))
	OLGRA(Widget, w)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	if (sw->scroll.textGC)
	    XtReleaseGC((Widget)sw, sw->scroll.textGC);

	if (sw->scroll.pAttrs)
	    OlgDestroyAttrs (sw->scroll.pAttrs);

#define REMOVE_WORK(w)  { if ((w)->scroll.timerid) {       \
                XtRemoveTimeOut((w)->scroll.timerid);    \
                (w)->scroll.timerid = (XtIntervalId)NULL; \
		} \
		if ((w)->scroll.workid) {       \
                XtRemoveWorkProc((w)->scroll.workid);    \
                (w)->scroll.workid = 0; \
		} }
	REMOVE_WORK(sw)

	/*  Note that the popupMenu is not explicitly destroyed.
	    Since it is a popup child of the scrollbar, the Intrinsics
	    will destroy it.  If the scrollbar was initialized with its
	    own menu, the application is responsible for destroying
	    that widget.  */

} /* Destroy */

/*
 * Redisplay
 */
/* ARGSUSED */
static void
Redisplay OLARGLIST((w,event,region))
	OLARG(Widget, w)
	OLARG(XEvent *, event)
	OLGRA(Region, region)
{
    ScrollbarWidget	sw = (ScrollbarWidget) w;

    OlgDrawScrollbar ((Widget)sw, sw->scroll.pAttrs);
}

/*
 *************************************************************************
 * Resize - Reconfigures all the subwidgets to a new size.
 *          Must also recalulate size and position of indicator.
 ****************************procedure*header*****************************
 */
static void
Resize OLARGLIST((w))
	OLGRA(Widget, w)
{
       ScrollbarWidget sw = (ScrollbarWidget)w;
       Dimension	elevWidth, elevHeight;
       Dimension	*elevLength;
       Dimension	anchorWidth, anchorHeight;
       int		length;
       int		anchorSize;

        /* calculates all dimensions */
        OlgSizeScrollbarAnchor ((Widget)sw, sw->scroll.pAttrs,
				&anchorWidth, &anchorHeight);
        sw->scroll.anchwidth = (unsigned char) anchorWidth;
        sw->scroll.anchlen = (unsigned char) anchorHeight;
	if (HORIZ (sw))
	{
	    length = sw->core.width;
	    anchorSize = sw->scroll.anchwidth * 2;
	    sw->scroll.offset = (Position) (sw->core.height -
					    sw->scroll.anchlen) / 2;
	    elevLength = &elevWidth;
	}
	else
	{
	    length = sw->core.height;
	    anchorSize = sw->scroll.anchlen * 2;
	    sw->scroll.offset = (Position) (sw->core.width -
					    sw->scroll.anchwidth) / 2;
	    elevLength = &elevHeight;
	}

	/*
	 * determines the type of scrollbar *
	 * regular, abbreviated, or minimum *
	 *
	 * abbreviated scrollbar has no dragbox.
	 *
	 * minimal scrollbar has no dragbox and anchors.
	 */
	sw->scroll.type = SB_REGULAR;
	OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs, SB_REGULAR,
			  &elevWidth, &elevHeight);
	if ((int)(anchorSize + *elevLength) <= (int) length) {
		/* regular elevator */
		if ((int) (anchorSize + *elevLength) == (int) length)
			/* minimum regular */
			sw->scroll.type = SB_MINREG;
	}
	else {
		sw->scroll.type = SB_ABBREVIATED;
		OlgSizeScrollbarElevator((Widget)sw, sw->scroll.pAttrs,
				 SB_ABBREVIATED, &elevWidth, &elevHeight);
		if ((int)(anchorSize + *elevLength) > (int) length)
		{
			sw->scroll.type = SB_MINIMUM;
			OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs,
			      SB_MINIMUM, &elevWidth, &elevHeight);
		}
	}
       sw->scroll.elevwidth = elevWidth;
       sw->scroll.elevheight = elevHeight;
} /* END OF Resize() */

/*
 *************************************************************************
 *  Realize - Creates the WIndow, and Creates the racing stripe.
 *      The racing stripe cannot be created any earlier, because the
 *      pixmap is attached to the window's background, thus window must
 *      have been created first.
 ****************************procedure*header*****************************
 */
static void
Realize OLARGLIST((w, valueMask, attributes))
	OLARG(Widget, w)
	OLARG(Mask *, valueMask)
	OLGRA(XSetWindowAttributes *, attributes)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

        XtCreateWindow((Widget)sw, InputOutput, (Visual *)CopyFromParent,
            *valueMask, attributes );

	XDefineCursor(XtDisplay(w), XtWindow(w), OlGetStandardCursor(w));

        /* draw the scrollbar stuffs in window */
        Resize(w);

	if (sw->scroll.showPage != OL_NONE)
		MakePageInd(sw);
} /* Realize */

/*
 *************************************************************************
 * QueryGeom - Important routine for parents that want to know how
 *              large the scrollbar wants to be. the thickness  (width
 *              for ver. scrollbar) and length (height for ver. scrollbar)
 *              should be honored by the parent, otherwise an ugly
 *              visual will occur.
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeom OLARGLIST((w, intended, reply))
	OLARG(Widget, w)
	OLARG(XtWidgetGeometry *, intended) /* parent's changes; may be NULL */
	OLGRA(XtWidgetGeometry *, reply)    /* child's preferred geometry; never NULL */
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
        XtGeometryResult result;
	Dimension height;
	Dimension width;

	/* start with the same values */
	*reply = *intended;

	/* assume ok */
	result = XtGeometryYes;

	/* Don't allow changes to the border width.  Use the border width
		in the widget as the reply since it has been initialized
		to the correct width for the given GUI.  */
	reply->border_width = sw->core.border_width;
	if (intended->request_mode & CWBorderWidth &&
		intended->border_width != sw->core.border_width)
		result = XtGeometryAlmost;

	/* X, Y, Sibling, and StackMode are always ok */

	/* Motif will honor any size changes */
	if (OlGetGui() == OL_MOTIF_GUI)
	    return (result);

	if (intended->request_mode & (CWWidth | CWHeight))
	{
	    OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs, SB_REGULAR,
				      &width, &height);

	    /* here checks the width */
	    if (intended->request_mode & CWWidth)
	    {
		if (HORIZ(sw))
		{
		    width += sw->scroll.anchwidth * 2 +
			sw->primitive.shadow_thickness * 2;
		    if (intended->width < width)
		    {
			OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs,
				SB_ABBREVIATED, &width, &height);
			width += sw->scroll.anchwidth * 2 +
				sw->primitive.shadow_thickness * 2;
			if (intended->width < width)
			    OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs,
				    SB_MINIMUM, &width, &height);
			if (intended->width != width)
			{
			    result = XtGeometryAlmost;
			    reply->width = width;
			}
		    }
		}
		else
		{
		    width += OlgGetHorizontalStroke (sw->scroll.pAttrs) * 2;
		    if (intended->width != width)
		    {
			result = XtGeometryAlmost;
			reply->width = width;
		    }
		}
	    }

	    /* here checks the height */
	    if (intended->request_mode & CWHeight)
	    {
		if (HORIZ(sw))
		{
		    height += OlgGetVerticalStroke (sw->scroll.pAttrs) * 2 +
				sw->primitive.shadow_thickness * 2;
		    if (intended->height != height)
		    {
			result = XtGeometryAlmost;
			reply->height = height;
		    }
		}
		else
		{
		    height += sw->scroll.anchlen * 2 +
				sw->primitive.shadow_thickness * 2;
		    if (intended->height < height)
		    {
			OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs,
				SB_ABBREVIATED, &width, &height);
			height += sw->scroll.anchlen * 2 +
				sw->primitive.shadow_thickness * 2;
			if (intended->height < height)
			    OlgSizeScrollbarElevator ((Widget)sw, sw->scroll.pAttrs,
				    SB_MINIMUM, &width, &height);
			if (intended->height != height)
			{
			    result = XtGeometryAlmost;
			    reply->height = height;
			}
		    }
		}
	    }
	}
	return (result);
}			/* QueryGeom */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *      to the current values, and sets them in the new
 *      widget.  It returns TRUE when the widget must be
 *      redisplayed.
 *
 *********************procedure*header************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG(Widget, current)
	OLARG(Widget, request)
	OLARG(Widget, new)
	OLARG(ArgList, args)
	OLGRA(Cardinal *, num_args)
{
        ScrollbarWidget sw = (ScrollbarWidget) current;
        ScrollbarWidget newsw = (ScrollbarWidget) new;
        OlScrollbarVerify olsb;
        int moved;
        Boolean needs_redisplay = FALSE;

        /* cannot change orientation on the fly */
        newsw->scroll.orientation = sw->scroll.orientation;

        CheckValues(newsw);

       if (OlGetGui() == OL_MOTIF_GUI &&
	   ((newsw->scroll.proportionLength != sw->scroll.proportionLength)
	    || (newsw->scroll.sliderMax != sw->scroll.sliderMax)
	    || (newsw->scroll.sliderMin != sw->scroll.sliderMin))) {
	    /*  recalculate the size of the Motif elevator. */
		OlgSizeScrollbarElevator ((Widget)newsw, newsw->scroll.pAttrs,
			SB_REGULAR, &(newsw->scroll.elevwidth),
			&(newsw->scroll.elevheight));
		needs_redisplay = TRUE;
	    /*  The next if statement will set moved to True.  */
	}

       if ((newsw->scroll.sliderMin != sw->scroll.sliderMin) ||
           (newsw->scroll.sliderMax != sw->scroll.sliderMax) ||
           (newsw->scroll.proportionLength != sw->scroll.proportionLength) ||
           (newsw->scroll.sliderValue != sw->scroll.sliderValue)) {
               olsb.new_location = newsw->scroll.sliderValue;
               moved = 1;
       }
	else {
	    moved = 0;
	}
	if (newsw->scroll.showPage != sw->scroll.showPage) {
		if (newsw->scroll.showPage == OL_NONE) {
			if (newsw->scroll.page_ind)
				XtDestroyWidget(newsw->scroll.page_ind);
		}
		else {
			if (!(newsw->scroll.page_ind) &&
			     XtIsRealized((Widget)sw))
				MakePageInd(newsw);
		}
	}

	if (newsw->scroll.popup != sw->scroll.popup)  {
		/* Do not allow the application to change the scrollbar's
		   popupMenu.  */
		newsw->scroll.popup = sw->scroll.popup;
	}

	if ((newsw->primitive.foreground != sw->primitive.foreground) ||
	    (newsw->core.background_pixel != sw->core.background_pixel)) {
	       _OlSBGetGCs (newsw);
               needs_redisplay = TRUE;
       }

	/* The scrollbar should always inherit its parents background for
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

       if (moved && (needs_redisplay == FALSE) && XtIsRealized((Widget)sw)) {
		newsw->scroll.sliderValue = sw->scroll.sliderValue;

		/* don't call sliderValueChanged callback */
       		_OlSBMoveSlider(newsw, &olsb, FALSE, FALSE);
	}
        return (needs_redisplay);
}       /* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */
/*
 *************************************************************************
 * AdjustDown - Callback for Btn Adjust Down inside Scrollbar window, but
 *              not in any children widgets.
 ****************************procedure*header*****************************
 */
static Boolean
AdjustDown OLARGLIST((w, event))
	OLARG(Widget, w)
	OLGRA(XEvent *, event)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	OlScrollbarVerify olsb;
	Position cablePos;
	Position point;
	int userRange;
	int cableLen;
	Dimension shadowThickness = sw->primitive.shadow_thickness;
	Dimension padLen;

	userRange = sw->scroll.sliderMax - sw->scroll.sliderMin -
			sw->scroll.proportionLength;

	/*  Calculate the new SliderValue based on the position of
	    the Adjust button down event.  */
	if (HORIZ(sw))  {
		if (event->xbutton.y < sw->scroll.offset ||
			event->xbutton.y >= sw->scroll.offset +
			(Position) sw->scroll.anchlen)
				return(False);
		/*  Check if the point is on the elevator, and do nothing 
		    if so.*/
		point = event->xbutton.x;
		if (point >= (int) sw->scroll.sliderPValue && 
			(int) point <= (int) (sw->scroll.sliderPValue + sw->scroll.elevwidth))  {
			SelectDown(w, event, (String *)NULL, (Cardinal *)NULL);
			return(True);
		}
		padLen = sw->scroll.pAttrs->pDev->verticalStroke;
		cablePos = sw->scroll.anchwidth + shadowThickness + padLen;
		cableLen = sw->core.width - (cablePos * 2) - 
				sw->scroll.elevwidth;
		point = point - cablePos - sw->scroll.elevwidth/2;
	}
	else {
		if (event->xbutton.x < sw->scroll.offset ||
			event->xbutton.x >= sw->scroll.offset +
			(Position) sw->scroll.anchwidth)
			return(False);
		/*  Check if the point is on the elevator, and do nothing 
		    if so.*/
		point = event->xbutton.y;
		if (point >= (int) sw->scroll.sliderPValue &&
			(int) point <= (int) (sw->scroll.sliderPValue + sw->scroll.elevheight))  {
			SelectDown(w, event, (String *)NULL, (Cardinal *)NULL);
			return(True);
		}
		padLen = sw->scroll.pAttrs->pDev->horizontalStroke;
		cablePos = sw->scroll.anchlen + shadowThickness + padLen;
		cableLen = sw->core.height - (cablePos * 2) -
				sw->scroll.elevheight;
		point = point - cablePos - sw->scroll.elevheight/2;
	}

	/*  The actual range of motion of the slider is the length of
	    the cable minus half of the elevator on each side.  That is
	    why the point calculation subtracts half of the elevator.
	    It is possible for the user to click adjust in the outer
	    halves of the elevator.  This if statement prevents the slider
	    value from going out of range in this case.  */
	if (point < 0)
		point = 0;
	if (point > cableLen)
		point = cableLen;

	olsb.new_location = muldiv(point, userRange, cableLen);

	_OlSBMoveSlider(sw, &olsb, True, False);

	/*  Now call SelectDown to handle the dragging of the adjust button. */
	SelectDown(w, event, (String *) NULL, (Cardinal *) NULL);

	return(True);
}       /* AdjustDown */


static Boolean
TopOrBottom OLARGLIST((w, event))
    OLARG(Widget, w)
    OLGRA(XEvent *, event)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	Position point;
	Boolean top = False;
	Boolean bottom = False;

	/*  Calculate the new SliderValue based on the position of
	    the Adjust button down event.  */
	if (HORIZ(sw))  {
		if (event->xbutton.y < sw->scroll.offset ||
			event->xbutton.y >= sw->scroll.offset +
			(Position) sw->scroll.anchlen)
				return(False);
		/*  Check if the point is on the elevator, and do nothing 
		    if so.*/
		point = event->xbutton.x;
		if (point >= (int) sw->scroll.sliderPValue && 
			(int) point <= (int) (sw->scroll.sliderPValue + sw->scroll.elevwidth))  {
			return(False);
		}
		if (point < sw->scroll.sliderPValue)
			top = True;
		else if (point > (int)
				(sw->scroll.sliderPValue + sw->scroll.elevwidth))
			bottom = True;
	}
	else {
		if (event->xbutton.x < sw->scroll.offset ||
			event->xbutton.x >= sw->scroll.offset +
			(Position) sw->scroll.anchwidth)
			return(False);
		/*  Check if the point is on the elevator, and do nothing 
		    if so.*/
		point = event->xbutton.y;
		if (point >= (int) sw->scroll.sliderPValue &&
			(int) point <= (int) (sw->scroll.sliderPValue + sw->scroll.elevheight))  {
			return(False);
		}
		if (point < sw->scroll.sliderPValue)
			top = True;
		else if (point > (int)
				(sw->scroll.sliderPValue + sw->scroll.elevheight))
			bottom = True;
	}

	if (top)
		return(ScrollKeyPress(w, (unsigned char)ANCHOR_TOP));
	else if (bottom)
		return(ScrollKeyPress(w, (unsigned char)ANCHOR_BOT));
	else
		return(False);

}  /* end of TopOrBottom() */


/*
 *************************************************************************
 * SelectDown - Callback for Btn Select Down inside Scrollbar window, but
 *              not in any children widgets.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
SelectDown OLARGLIST((w,event,params,num_params))
	OLARG(Widget, w)
	OLARG(XEvent *, event)
	OLARG(String *, params)
	OLGRA(Cardinal *, num_params)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	unsigned char opcode = NOOP;

	if (!FindOp(w, event, &opcode))
		/*  The event was outside the scrollbar. */
		return;

	sw->scroll.opcode = opcode;
        highlight(sw,TRUE);

	/* save current position */
	sw->scroll.previous = sw->scroll.sliderValue;

       if (opcode == DRAG_ELEV && sw->scroll.type != SB_REGULAR) {
		/* if not regular scrollbar, cannot drag */
		highlight(sw,FALSE);
		return;
	}

        TimerEvent((XtPointer)sw);
}       /* SelectDown */

/*
 *************************************************************************
 * SelectUp - Callback when Select Button is released.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
SelectUp OLARGLIST((w,event,params,num_params))
	OLARG(Widget, w)
	OLARG(XEvent *, event)
	OLARG(String *, params)
	OLGRA(Cardinal *, num_params)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	if (sw->scroll.opcode != NOOP) {
        	/* unhighlight */
        	highlight(sw,FALSE);

                REMOVE_WORK(sw)

		if (SWB.opcode == DRAG_ELEV) {
        		OlScrollbarVerify olsb;

        		sw->scroll.opcode = NOOP;
			if (SWB.stoppos == OL_GRANULARITY) {
			
				olsb.new_location = SWB.sliderMin +
					 (SWB.sliderValue - SWB.sliderMin +
					 SWB.granularity / 2) /
					 SWB.granularity * SWB.granularity;
				olsb.new_location = INRANGE(olsb.new_location,
                                       	SWB.sliderMin,
                                       	SWB.sliderMax - SWB.proportionLength);
			}
			else
				olsb.new_location = sw->scroll.sliderValue;
			_OlSBMoveSlider(sw, &olsb, TRUE, FALSE);
		}
       		sw->scroll.opcode = NOOP;
	}
} /* SelectUp */

static Boolean
ScrollKeyPress OLARGLIST((w, opcode))
	OLARG(Widget, w)
	OLGRA(unsigned char, opcode)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	SWB.opcode = opcode | KBD_OP;
	highlight (sw, True);
	SWB.previous = SWB.sliderValue;

	TimerEvent((XtPointer)sw);

	highlight (sw, False);
	SWB.opcode = NOOP;
	return (True);
} /* ScrollKeyPress */

/* ARGSUSED */
static Boolean
SBActivateWidget OLARGLIST((w, activation_type, call_data))
	OLARG(Widget, w)
	OLARG(OlVirtualName, activation_type)
	OLGRA(XtPointer, call_data)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	Boolean		consumed = False;

	if (SWB.orientation == (OlDefine)OL_HORIZONTAL)
	{
	  switch (activation_type)
	  {
		case OL_MENUKEY:
		case OL_HSBMENU:
			consumed = PopupMenu(w, NULL);
			break;
		case OL_PAGELEFT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGERIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLLEFT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLRIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLLEFTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLRIGHTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
	  }
	}
	else			/* OL_VERTICAL */
	{
	  switch (activation_type)
	  {
		case OL_MENUKEY:
		case OL_VSBMENU:
			consumed = PopupMenu(w, NULL);
			break;
		case OL_PAGEUP:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGEDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLUP:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLDOWN:
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
	return (consumed);
} /* SBActivateWidget */


/* ARGSUSED */
static void
UnmapEventHandler OLARGLIST((w, data, event, continue_to_dispatch))
    OLARG(Widget, w)
    OLARG(XtPointer, data)
    OLARG(XEvent *, event)
    OLGRA(Boolean *, continue_to_dispatch)
{
    ScrollbarWidget sw = (ScrollbarWidget)w;

    if (event->type == UnmapNotify)
	REMOVE_WORK(sw)

} /* UnmapEventHandler */


static void
SBButtonHandler OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{

    switch (ve->virtual_name)
    {
    case OL_SELECT:
	ve->consumed = True;
	if (ve->xevent->type == ButtonPress)
	    SelectDown (w, ve->xevent, NULL, NULL);
	else
	    SelectUp (w, ve->xevent, NULL, NULL);
	break;
    case OL_ADJUST:
	if (OlGetGui() == OL_MOTIF_GUI) {
	    /* Motif toggle mode */
	    if (ve->xevent->type == ButtonPress)
		ve->consumed = TopOrBottom(w, ve->xevent);
	}
	else {
	    if (ve->xevent->type == ButtonPress)
		ve->consumed = AdjustDown (w, ve->xevent);
	    else  {
		ve->consumed = True;
		/*  Adjust does the same as Select here.  */
		SelectUp (w, ve->xevent, NULL, NULL);
	    }
	}
	break;
    case OLM_BDrag:
	if (OlGetGui() == OL_MOTIF_GUI) {
	    if (ve->xevent->type == ButtonPress)
		ve->consumed = AdjustDown (w, ve->xevent);
	    else  {
		ve->consumed = True;
		/*  stop the dragging  */
		SelectUp (w, ve->xevent, NULL, NULL);
	    }
	}
	break;
    case OL_MENU:
	ve->consumed = PopupMenu (w, ve->xevent);
	break;
    default:
	if (ve->xevent->type == ButtonRelease)
	    SelectUp (w, ve->xevent, NULL, NULL);
    }
	
} /* SBButtonHandler */

/* should only be invoked in Motif mode, See ClassInitialize ... */
static void
SBKeyHandler OLARGLIST((w, ve))
	OLARG(Widget, w)
	OLGRA(OlVirtualEvent, ve)
{
	ScrollbarWidget	sw = (ScrollbarWidget)w;
	unsigned char	op;

	if (SWB.orientation == (OlDefine)OL_HORIZONTAL)
	{
		if ( (op = (unsigned char)GRAN_INC,
		      ve->virtual_name == OL_MOVERIGHT) ||
		     (op = (unsigned char)GRAN_DEC,
		      ve->virtual_name == OL_MOVELEFT) ||
		    (op = (unsigned char)ANCHOR_TOP, 
		     ve->virtual_name == OLM_KBeginLine) ||
		    (op = (unsigned char)ANCHOR_BOT, 
		     ve->virtual_name == OLM_KEndLine) ||
		    (op = (unsigned char)ANCHOR_TOP, 
		     ve->virtual_name == OLM_KBeginData) ||
		    (op = (unsigned char)ANCHOR_BOT, 
		     ve->virtual_name == OLM_KEndData) )
		{
			ve->consumed = ScrollKeyPress(w, op);
		}
	}
	else	/* must be OL_VERTICAL... */
	{
		if ( (op = (unsigned char)GRAN_INC,
		      ve->virtual_name == OL_MOVEDOWN) ||
		     (op = (unsigned char)GRAN_DEC,
		      ve->virtual_name == OL_MOVEUP) ||
		    (op = (unsigned char)ANCHOR_TOP, 
		     ve->virtual_name == OLM_KBeginLine) ||
		    (op = (unsigned char)ANCHOR_BOT, 
		     ve->virtual_name == OLM_KEndLine) ||
		    (op = (unsigned char)ANCHOR_TOP, 
		     ve->virtual_name == OLM_KBeginData) ||
		    (op = (unsigned char)ANCHOR_BOT, 
		     ve->virtual_name == OLM_KEndData) )
		{
			ve->consumed = ScrollKeyPress(w, op);
		}
	}
} /* end of SBKeyHandler */

Widget
OlSBCreateMenu OLARGLIST((parent, orientation))
	OLARG(Widget, parent)
	OLGRA(OlDefine, orientation)
{
	/*  Must make sure that the RESOLVE has been called. */
	XtInitializeWidgetClass(scrollbarWidgetClass);

	return((*_olmSBCreateMenu) (parent, orientation));
}  /* end of OlSBCreateMenu() */
