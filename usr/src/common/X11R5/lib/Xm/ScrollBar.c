#pragma ident	"@(#)m1.2libs:Xm/ScrollBar.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include <Xm/ScrollBarP.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/ManagerP.h>	/* for _XmParentProcess */
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/DropSMgr.h>    /* for XmDropSiteStartUpdate/EndUPdate */
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif

#define MAXINT 2147483647

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_SBar,MSG_SB_1,_XmMsgScrollBar_0000)
#define MESSAGE2	catgets(Xm_catd,MS_SBar,MSG_SB_2,_XmMsgScrollBar_0001)
#define MESSAGE3	catgets(Xm_catd,MS_SBar,MSG_SB_3,_XmMsgScrollBar_0002)
#define MESSAGE4	catgets(Xm_catd,MS_SBar,MSG_SB_4,_XmMsgScrollBar_0003)
#define MESSAGE6	catgets(Xm_catd,MS_SBar,MSG_SB_6,_XmMsgScaleScrBar_0004)
#define MESSAGE7	catgets(Xm_catd,MS_SBar,MSG_SB_7,_XmMsgScrollBar_0004)
#define MESSAGE8	catgets(Xm_catd,MS_SBar,MSG_SB_8,_XmMsgScrollBar_0005)
#define MESSAGE9	catgets(Xm_catd,MS_SBar,MSG_SB_9,_XmMsgScrollBar_0006)
#define MESSAGE10	catgets(Xm_catd,MS_SBar,MSG_SB_10,_XmMsgScrollBar_0007)
#define MESSAGE13	catgets(Xm_catd,MS_SBar,MSG_SB_13,_XmMsgScrollBar_0008)
#else
#define MESSAGE1	_XmMsgScrollBar_0000
#define MESSAGE2	_XmMsgScrollBar_0001
#define MESSAGE3	_XmMsgScrollBar_0002
#define MESSAGE4	_XmMsgScrollBar_0003
#define MESSAGE6	_XmMsgScaleScrBar_0004
#define MESSAGE7	_XmMsgScrollBar_0004
#define MESSAGE8	_XmMsgScrollBar_0005
#define MESSAGE9	_XmMsgScrollBar_0006
#define MESSAGE10	_XmMsgScrollBar_0007
#define MESSAGE13	_XmMsgScrollBar_0008
#endif


#define FIRST_SCROLL_FLAG (1<<0)
#define VALUE_SET_FLAG    (1<<1)
#define END_TIMER         (1<<2)

#define ARROW1_AVAILABLE  (1<<3)
#define ARROW2_AVAILABLE  (1<<4)
#define SLIDER_AVAILABLE  (1<<5)
#define KEYBOARD_GRABBED  (1<<6)

#define DRAWARROW(sbw, t_gc, b_gc, x, y, dir)\
    _XmDrawArrow(XtDisplay ((Widget) sbw),\
		XtWindow ((Widget) sbw),\
		t_gc, b_gc,\
		sbw->scrollBar.foreground_GC,\
		x-1, y-1,\
		sbw->scrollBar.arrow_width+2,\
		sbw->scrollBar.arrow_height+2,\
		sbw->primitive.shadow_thickness,\
		dir);

#define UNAVAILABLE_STIPPLE_NAME "_XmScrollBarUnavailableStipple"

#ifdef CDE_VISUAL  /* sliding_mode */
#define THERMOMETER_ON	2
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void ProcessingDirectionDefault() ;
static void ForegroundPixelDefault() ;
static void BackgroundPixelDefault() ;
static void TraversalDefault() ;
static void TroughPixelDefault() ;
static void Initialize() ;
static void GetForegroundGC() ;
static void GetUnavailableGC() ;
static void GetSliderPixmap() ;
static void CalcSliderRect() ;
static void DrawSliderPixmap() ;
static void Redisplay() ;
static void Resize() ;
static void Realize() ;
static void Destroy() ;
static Boolean ValidateInputs() ;
static Boolean SetValues() ;
static int CalcSliderVal() ;
static void Select() ;
static void Release() ;
static void Moved() ;
static void TopOrBottom() ;
static void IncrementUpOrLeft() ;
static void IncrementDownOrRight() ;
static void PageUpOrLeft() ;
static void PageDownOrRight() ;
static void CancelDrag() ;
static void MoveSlider() ;
static void RedrawSliderWindow() ;
static Boolean ChangeScrollBarValue() ;
static void TimerEvent() ;
static void ScrollCallback() ;
static void ExportScrollBarValue() ;
static XmImportOperator ImportScrollBarValue() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void ProcessingDirectionDefault( 
                        XmScrollBarWidget widget,
                        int offset,
                        XrmValue *value) ;
static void ForegroundPixelDefault( 
                        XmScrollBarWidget widget,
                        int offset,
                        XrmValue *value) ;
static void BackgroundPixelDefault( 
                        XmScrollBarWidget widget,
                        int offset,
                        XrmValue *value) ;
static void TraversalDefault( 
                        XmScrollBarWidget widget,
                        int offset,
                        XrmValue *value) ;
static void TroughPixelDefault( 
                        XmScrollBarWidget widget,
                        int offset,
                        XrmValue *value) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetForegroundGC( 
                        XmScrollBarWidget sbw) ;
static void GetUnavailableGC( 
                        XmScrollBarWidget sbw) ;
static void GetSliderPixmap( 
                        XmScrollBarWidget sbw) ;
static void CalcSliderRect( 
                        XmScrollBarWidget sbw,
                        short *slider_x,
                        short *slider_y,
                        short *slider_width,
                        short *slider_height) ;
static void DrawSliderPixmap( 
                        XmScrollBarWidget sbw) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget wid) ;
static void Realize( 
                        Widget sbw,
                        XtValueMask *window_mask,
                        XSetWindowAttributes *window_attributes) ;
static void Destroy( 
                        Widget wid) ;
static Boolean ValidateInputs( 
                        XmScrollBarWidget current,
                        XmScrollBarWidget request,
                        XmScrollBarWidget new_w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static int CalcSliderVal( 
                        XmScrollBarWidget sbw,
                        int x,
                        int y) ;
static void Select( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Release( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Moved( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TopOrBottom( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void IncrementUpOrLeft( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void IncrementDownOrRight( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageUpOrLeft( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageDownOrRight( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CancelDrag( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveSlider( 
                        XmScrollBarWidget sbw,
                        int currentX,
                        int currentY) ;
static void RedrawSliderWindow( 
                        XmScrollBarWidget sbw) ;
static Boolean ChangeScrollBarValue( 
                        XmScrollBarWidget sbw) ;
static void TimerEvent( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static void ScrollCallback( 
                        XmScrollBarWidget sbw,
                        int reason,
                        int value,
                        int xpixel,
                        int ypixel,
                        XEvent *event) ;
static void ExportScrollBarValue( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator ImportScrollBarValue( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*  Default translation table and action list  */

#define defaultTranslations	_XmScrollBar_defaultTranslations

static XtActionsRec actions[] =
{
	{ "Select",                 Select },
	{ "Release",                Release },
	{ "Moved",                  Moved },
	{ "TopOrBottom",            TopOrBottom },
	{ "IncrementUpOrLeft",      IncrementUpOrLeft },
	{ "IncrementDownOrRight",   IncrementDownOrRight },
	{ "PageUpOrLeft",           PageUpOrLeft },
	{ "PageDownOrRight",        PageDownOrRight },
	{ "CancelDrag",             CancelDrag },
};


/*  Resource list for ScrollBar  */

static XtResource resources[] = 
{
	{ XmNnavigationType, XmCNavigationType, XmRNavigationType,
	  sizeof(unsigned char),
	  XtOffsetOf( struct _XmScrollBarRec, primitive.navigation_type),
	  XmRImmediate, (XtPointer) XmSTICKY_TAB_GROUP
	},
	{ XmNforeground, XmCForeground, XmRPixel, sizeof(Pixel),
	  XtOffsetOf( struct _XmScrollBarRec, primitive.foreground),
	  XmRCallProc, (XtPointer) ForegroundPixelDefault
	},
	{ XmNbackground, XmCBackground, XmRPixel, sizeof(Pixel),
	  XtOffsetOf( struct _XmScrollBarRec, core.background_pixel),
	  XmRCallProc, (XtPointer) BackgroundPixelDefault
	},
	{ XmNtroughColor, XmCTroughColor, XmRPixel, sizeof(Pixel),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.trough_color),
	  XmRCallProc, (XtPointer) TroughPixelDefault
	},
	{ XmNvalue, XmCValue, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.value),
	  XmRImmediate, (XtPointer) MAXINT
	},
	{ XmNminimum, XmCMinimum, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.minimum),
	  XmRImmediate, (XtPointer) 0
	},
	{ XmNmaximum, XmCMaximum, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.maximum),
	  XmRImmediate, (XtPointer) 100
	},
	{ XmNsliderSize, XmCSliderSize, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.slider_size),
	  XmRImmediate, (XtPointer) MAXINT
	},
	{ XmNshowArrows, XmCShowArrows, XmRBoolean, sizeof (Boolean),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.show_arrows),
	  XmRImmediate, (XtPointer) True
	},
	{ XmNorientation, XmCOrientation, 
	  XmROrientation, sizeof (unsigned char),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.orientation),
	  XmRImmediate, (XtPointer) XmVERTICAL
	},
	{ XmNprocessingDirection, XmCProcessingDirection, 
	  XmRProcessingDirection, sizeof (unsigned char), 
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.processing_direction),
	  XmRCallProc, (XtPointer) ProcessingDirectionDefault
	},
	{ XmNincrement, XmCIncrement, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.increment),
	  XmRImmediate, (XtPointer) 1
	},
	{ XmNpageIncrement, XmCPageIncrement, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.page_increment),
	  XmRImmediate, (XtPointer) 10
	},
	{ XmNinitialDelay, XmCInitialDelay, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.initial_delay),
	  XmRImmediate, (XtPointer) 250
	},
	{ XmNrepeatDelay, XmCRepeatDelay, XmRInt, sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.repeat_delay),
	  XmRImmediate, (XtPointer) 50
	},
	{ XmNvalueChangedCallback, XmCCallback, 
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.value_changed_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNincrementCallback, XmCCallback, 
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.increment_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNdecrementCallback, XmCCallback, 
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.decrement_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNpageIncrementCallback, XmCCallback,
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.page_increment_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNpageDecrementCallback, XmCCallback, 
	  XmRCallback, sizeof (XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.page_decrement_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNtoTopCallback, XmCCallback,
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.to_top_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNtoBottomCallback, XmCCallback,
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.to_bottom_callback),
	  XmRPointer, (XtPointer) NULL
	},
	{ XmNdragCallback, XmCCallback,
	  XmRCallback, sizeof(XtCallbackList),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.drag_callback),
	  XmRPointer, (XtPointer) NULL
	},
        {
         XmNtraversalOn, XmCTraversalOn, XmRBoolean, sizeof (Boolean),
         XtOffsetOf( struct _XmPrimitiveRec, primitive.traversal_on),
         XmRCallProc, (XtPointer) TraversalDefault
        },
        {
          XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
          sizeof (Dimension),
          XtOffsetOf( struct _XmPrimitiveRec, primitive.highlight_thickness),
          XmRImmediate, (XtPointer) 0
        },
};


/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
	{ XmNvalue,
	  sizeof (int),
	  XtOffsetOf( struct _XmScrollBarRec, scrollBar.value), 
	  ExportScrollBarValue,
	  ImportScrollBarValue
	},
};


/*  The ScrollBar class record definition  */

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    (XtInitProc)NULL,			/* InitializePrehook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPrehook	*/
    (XtInitProc)NULL,			/* InitializePosthook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    (XtInitProc)NULL,			/* secondaryCreate	*/
    (XmGetSecResDataFunc)NULL,		/* getSecRes data	*/
    { 0 },				/* fastSubclass flags	*/
    (XtArgsProc)NULL,			/* get_values_prehook	*/
    (XtArgsProc)NULL,			/* get_values_posthook	*/
    (XtWidgetClassProc)NULL,            /* classPartInitPrehook */
    (XtWidgetClassProc)NULL,            /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    XmInheritWidgetNavigable,           /* widgetNavigable      */
    XmInheritFocusChange,               /* focusChange          */
};

externaldef(xmscrollbarclassrec) XmScrollBarClassRec xmScrollBarClassRec =
{
   {
      (WidgetClass) &xmPrimitiveClassRec, /* superclass	         */
      "XmScrollBar",                    /* class_name	         */
      sizeof(XmScrollBarRec),           /* widget_size	         */
      ClassInitialize,                  /* class_initialize      */
      ClassPartInitialize,              /* class_part_initialize */
      FALSE,                            /* class_inited          */
      Initialize,                       /* initialize	         */
      (XtArgsProc)NULL,                 /* initialize_hook       */
      Realize,                          /* realize	         */	
      actions,                          /* actions               */	
      XtNumber(actions),                /* num_actions	         */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      XtExposeCompressMaximal,          /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      Destroy,                          /* destroy               */	
      Resize,                           /* resize                */	
      Redisplay,                        /* expose                */	
      SetValues,                        /* set_values    	 */	
      (XtArgsFunc)NULL,                 /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      (XtArgsProc)NULL,			/* get_values_hook       */
      (XtAcceptFocusProc)NULL,          /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      defaultTranslations,              /* tm_table              */
      (XtGeometryHandler)NULL,          /* query_geometry        */
      (XtStringProc)NULL,               /* display_accelerator   */
      (XtPointer) &baseClassExtRec,     /* extension             */
   },

   {
      XmInheritWidgetProc,		/* border_highlight   */
      XmInheritWidgetProc,		/* border_unhighlight */
      NULL,				/* translations       */
      (XtActionProc)NULL,		/* arm_and_activate   */
      syn_resources,   			/* syn_resources      */
      XtNumber(syn_resources),		/* num syn_resources  */
      NULL,				/* extension          */
   },

   {
      (XtPointer) NULL,			/* extension          */
   },
};

externaldef(xmscrollbarwidgetclass) WidgetClass xmScrollBarWidgetClass = (WidgetClass) &xmScrollBarClassRec;


/*********************************************************************
 *
 *  ExportScrollBarValue
 *	Convert the scrollbar value from the normal processing direction
 *	to reverse processing if needed.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ExportScrollBarValue( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ExportScrollBarValue(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	if ((sbw->scrollBar.processing_direction == XmMAX_ON_LEFT) ||
		(sbw->scrollBar.processing_direction == XmMAX_ON_TOP))
		*value = (XtArgVal)(sbw->scrollBar.maximum + 
				    sbw->scrollBar.minimum - 
				    sbw->scrollBar.value - 
				    sbw->scrollBar.slider_size);
	else
		*value = (XtArgVal) sbw->scrollBar.value;
}

/*********************************************************************
 *
 *  ImportScrollBarValue
 *  Indicate that the value did indeed change.
 *
 *********************************************************************/
static XmImportOperator 
#ifdef _NO_PROTO
ImportScrollBarValue( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
ImportScrollBarValue(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;

	sbw->scrollBar.flags |= VALUE_SET_FLAG;
	*value = (XtArgVal)sbw->scrollBar.value;
	return(XmSYNTHETIC_LOAD);
}


/*********************************************************************
 *
 * ProcessingDirectionDefault
 *    This procedure provides the dynamic default behavior for 
 *    the processing direction resource dependent on the orientation.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ProcessingDirectionDefault( widget, offset, value )
        XmScrollBarWidget widget ;
        int offset ;
        XrmValue *value ;
#else
ProcessingDirectionDefault(
        XmScrollBarWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static unsigned char direction;

	value->addr = (XPointer) &direction;

	if (widget->scrollBar.orientation == XmHORIZONTAL)
		direction = XmMAX_ON_RIGHT;
	else /* XmVERTICAL  -- range checking done during widget
	                       initialization */
		direction = XmMAX_ON_BOTTOM;
}




/*********************************************************************
 *
 * ForegroundPixelDefault
 *    This procedure provides the dynamic default behavior for 
 *    the foreground color.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ForegroundPixelDefault( widget, offset, value )
        XmScrollBarWidget widget ;
        int offset ;
        XrmValue *value ;
#else
ForegroundPixelDefault(
        XmScrollBarWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static Pixel foreground;

	value->addr = (XPointer) &foreground;

	foreground = widget->core.background_pixel;
}



/*********************************************************************
 *
 * BackgroundPixelDefault
 *    This procedure provides the dynamic default behavior for 
 *    the background color. It looks to see if the parent is a
 *    ScrolledWindow, and if so, it uses the parent background.
 *    This is mostly for compatibility with 1.1 where the scrolledwindow
 *    was forcing its scrollbar color to its own background.
 *    Note that it works for both automatic and non automatic SW,
 *    which is a new feature for non automatic.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
BackgroundPixelDefault( widget, offset, value )
        XmScrollBarWidget widget ;
        int offset ;
        XrmValue *value ;
#else
BackgroundPixelDefault(
        XmScrollBarWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static Pixel background;
	Widget parent = XtParent(widget) ;

	if (XmIsScrolledWindow(parent)) {
	    value->addr = (XPointer) &background;
	    background = parent->core.background_pixel;
	    return ;
	}

	/* else use the primitive defaulting mechanism */

	_XmBackgroundColorDefault((Widget )widget, offset, value);
}

/*********************************************************************
 *
 * TraversalDefault
 *    This procedure provides the dynamic default behavior for 
 *    the traversal. It looks to see if the parent is a
 *    ScrolledWindow, and if so, it sets it to On.
 *    This is mostly for compatibility with 1.1 where the scrolledwindow
 *    was forcing its scrollbar traversal to On
 *    Note that it works only for automatic.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
TraversalDefault( widget, offset, value )
        XmScrollBarWidget widget ;
        int offset ;
        XrmValue *value ;
#else
TraversalDefault(
        XmScrollBarWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
      static Boolean traversal ;
      Widget parent = XtParent(widget) ;
      Arg al[1] ;
      unsigned char sp ;

      traversal = False ;
      value->addr = (XPointer) &traversal;
              
      if (XmIsScrolledWindow(parent)) {
          XtSetArg(al[0], XmNscrollingPolicy, &sp);
          XtGetValues(parent, al, 1);
          if (sp == XmAUTOMATIC) {
              traversal = True ;
              return ;
          }
      }
}



/*********************************************************************
 *
 * TroughPixelDefault
 *    This procedure provides the dynamic default behavior for 
 *    the trough color.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
TroughPixelDefault( widget, offset, value )
        XmScrollBarWidget widget ;
        int offset ;
        XrmValue *value ;
#else
TroughPixelDefault(
        XmScrollBarWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static Pixel trough;
	XmColorData *pixel_data;

	value->addr = (XPointer) &trough;

	pixel_data = _XmGetColors(XtScreen((Widget)widget),
		widget->core.colormap, widget->core.background_pixel);
	
	trough = _XmAccessColorData(pixel_data, XmSELECT);
}



/*********************************************************************
 *
 *  ClassInitialize
 *     Initialize the motif quark, a variable that can't be set in the 
 *     initial struct by the compiler.
 *
 *********************************************************************/

static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  baseClassExtRec.record_type = XmQmotif ;
}

/*********************************************************************
 *
 *  ClassPartInitialize
 *     Initialize the fast subclassing.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
	_XmFastSubclassInit (wc, XmSCROLL_BAR_BIT);
}






/*********************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( rw, nw, args, num_args )
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
	XmScrollBarWidget request = (XmScrollBarWidget) rw ;
	XmScrollBarWidget new_w = (XmScrollBarWidget) nw ;

	Boolean default_value = FALSE;

	if (new_w->scrollBar.value == MAXINT)
	{
		new_w->scrollBar.value = 0;
		default_value = True;
	}

	/* Validate the incoming data  */                      

	if (new_w->scrollBar.minimum >= new_w->scrollBar.maximum)
	{
		new_w->scrollBar.minimum = 0;
		new_w->scrollBar.maximum = 100;
		_XmWarning( (Widget) new_w, MESSAGE1);
	}

	if (new_w->scrollBar.slider_size == MAXINT)
	{
		new_w->scrollBar.slider_size = (new_w->scrollBar.maximum
			- new_w->scrollBar.minimum) / 10;
		if (new_w->scrollBar.slider_size < 1)
			new_w->scrollBar.slider_size = 1;
	}

	if (new_w->scrollBar.slider_size < 1)
	{
		new_w->scrollBar.slider_size = 1;
		_XmWarning( (Widget) new_w, MESSAGE2);
	}

	if (new_w->scrollBar.slider_size > 
		(new_w->scrollBar.maximum - new_w->scrollBar.minimum))
	{
		new_w->scrollBar.slider_size = new_w->scrollBar.maximum
			- new_w->scrollBar.minimum;
		_XmWarning( (Widget) new_w, MESSAGE13);
	}

	if (new_w->scrollBar.value < new_w->scrollBar.minimum)
	{
		new_w->scrollBar.value = new_w->scrollBar.minimum;
		if (!default_value) _XmWarning( (Widget) new_w, MESSAGE3);
	}

	if (new_w->scrollBar.value > 
		new_w->scrollBar.maximum - new_w->scrollBar.slider_size)
	{
		new_w->scrollBar.value = new_w->scrollBar.minimum;
		if (!default_value) _XmWarning( (Widget) new_w, MESSAGE4);
	}

	if(    !XmRepTypeValidValue( XmRID_ORIENTATION, 
                                 new_w->scrollBar.orientation, (Widget) new_w)    )
	{
		new_w->scrollBar.orientation = XmVERTICAL;
	}

	if (new_w->scrollBar.orientation == XmHORIZONTAL)
	{
		if ((new_w->scrollBar.processing_direction != XmMAX_ON_RIGHT) &&
			(new_w->scrollBar.processing_direction != XmMAX_ON_LEFT))

		{
			new_w->scrollBar.processing_direction = XmMAX_ON_RIGHT;
			_XmWarning( (Widget) new_w, MESSAGE6);
		}
	}
	else
	{
		if ((new_w->scrollBar.processing_direction != XmMAX_ON_TOP) &&
			(new_w->scrollBar.processing_direction != XmMAX_ON_BOTTOM))
		{
			new_w->scrollBar.processing_direction = XmMAX_ON_BOTTOM;
			_XmWarning( (Widget) new_w, MESSAGE6);
		}
	}

	if (new_w->scrollBar.increment <= 0)
	{
		new_w->scrollBar.increment = 1;
		_XmWarning( (Widget) new_w, MESSAGE7);
	}

	if (new_w->scrollBar.page_increment <= 0)
	{
		new_w->scrollBar.page_increment = 10;
		_XmWarning( (Widget) new_w, MESSAGE8);
	}

	if (new_w->scrollBar.initial_delay <= 0)
	{
		new_w->scrollBar.initial_delay = 250;
		_XmWarning( (Widget) new_w, MESSAGE9);
	}

	if (new_w->scrollBar.repeat_delay <= 0)
	{
		new_w->scrollBar.repeat_delay = 75;
		_XmWarning( (Widget) new_w, MESSAGE10);
	}

	/*  Set up a geometry for the widget if it is currently 0.  */

	if (request->core.width == 0)
	{
		if (new_w->scrollBar.orientation == XmHORIZONTAL)
			 new_w->core.width += 100;
		else
			 new_w->core.width += 11;
	}
	if (request->core.height == 0)
	{
		if (new_w->scrollBar.orientation == XmHORIZONTAL)
			 new_w->core.height += 11;
		else
			 new_w->core.height += 100;
	}

	/*  Reverse the value for reverse processing.  */

	if ((new_w->scrollBar.processing_direction == XmMAX_ON_LEFT) ||
		(new_w->scrollBar.processing_direction == XmMAX_ON_TOP))
		new_w->scrollBar.value = new_w->scrollBar.maximum 
			+ new_w->scrollBar.minimum - new_w->scrollBar.value
			- new_w->scrollBar.slider_size;

	/*  Set the internally used variables.  */

	new_w->scrollBar.flags = 0;
    if (new_w->scrollBar.slider_size < (new_w->scrollBar.maximum
            - new_w->scrollBar.minimum))
	{
        new_w->scrollBar.flags |= SLIDER_AVAILABLE;

		if (new_w->scrollBar.value > new_w->scrollBar.minimum)
			new_w->scrollBar.flags |= ARROW1_AVAILABLE;
		if (new_w->scrollBar.value < (new_w->scrollBar.maximum
				- new_w->scrollBar.slider_size))
			new_w->scrollBar.flags |= ARROW2_AVAILABLE;
	}
	else
	{
		/*
		 * For correct setvalues processing, when the slider is
		 * unavailable, the arrows should be available.
		 */
		new_w->scrollBar.flags |= ARROW1_AVAILABLE;
		new_w->scrollBar.flags |= ARROW2_AVAILABLE;
	}
	new_w->scrollBar.pixmap = 0;
	new_w->scrollBar.sliding_on = FALSE;
	new_w->scrollBar.timer = 0;
	new_w->scrollBar.etched_slider = FALSE;

	new_w->scrollBar.arrow_width = 0;
	new_w->scrollBar.arrow_height = 0;

	new_w->scrollBar.arrow1_x = 0;
	new_w->scrollBar.arrow1_y = 0;
	new_w->scrollBar.arrow1_selected = FALSE;

	new_w->scrollBar.arrow2_x = 0;
	new_w->scrollBar.arrow2_y = 0;
	new_w->scrollBar.arrow2_selected = FALSE;

	new_w->scrollBar.saved_value = new_w->scrollBar.value;

	/*  Get the drawing graphics contexts.  */

	GetForegroundGC(new_w);
	GetUnavailableGC(new_w);

	/* call the resize method to get an initial size */

	(* (new_w->core.widget_class->core_class.resize)) ((Widget) new_w);

    }




/************************************************************************
 *
 *  GetForegroundGC
 *     Get the graphics context used for drawing the slider and arrows.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetForegroundGC( sbw )
        XmScrollBarWidget sbw ;
#else
GetForegroundGC(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
    XGCValues values;
    XtGCMask  valueMask;

    valueMask = GCForeground | GCGraphicsExposures;
    values.foreground = sbw->core.background_pixel;
    values.graphics_exposures = False;

    sbw->scrollBar.foreground_GC = XtGetGC ((Widget) sbw, valueMask, &values);
}



/************************************************************************
 *
 *  GetUnavailableGC
 *     Get the graphics context used for drawing the slider and arrows
 *     as being unavailable.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetUnavailableGC( sbw )
        XmScrollBarWidget sbw ;
#else
GetUnavailableGC(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
    XGCValues values;
    XtGCMask  valueMask;
    Pixmap t_pix;
    XImage *t_im;

    valueMask = GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;

    if ((t_pix = XmGetPixmapByDepth(XtScreen(sbw), UNAVAILABLE_STIPPLE_NAME,
				    (Pixel) 1, (Pixel) 0, 1))
		== XmUNSPECIFIED_PIXMAP)
	{
		if (_XmGetImage(XtScreen(sbw), "50_foreground", &t_im))
		{
			GC t_gc;
			XGCValues t_values;
			XtGCMask  t_valueMask;

			t_pix = XCreatePixmap(XtDisplay(sbw),
				RootWindowOfScreen(XtScreen((Widget)sbw)),
				t_im->width, t_im->height, 1);

			t_values.foreground = 1;
			t_values.background = 0;
			t_valueMask = (GCForeground | GCBackground);
			t_gc = XCreateGC(XtDisplay(sbw), t_pix, t_valueMask,
				&t_values);
			XPutImage(XtDisplay(sbw), t_pix, t_gc, t_im, 0, 0, 0, 0,
				t_im->width, t_im->height);

			values.fill_style = FillStippled;
			values.foreground = BlackPixelOfScreen(XtScreen(sbw));
			values.stipple = t_pix;
			valueMask |= (GCFillStyle | GCStipple);

			_XmInstallPixmap(t_pix, XtScreen(sbw),
				UNAVAILABLE_STIPPLE_NAME, (Pixel) 1, (Pixel) 0);
			XFreeGC(XtDisplay(sbw), t_gc);
		}
		else
			/* Will cause the arrow and/or slider to dissappear */
			values.foreground = sbw->core.background_pixel;
	}
	else
	{
		values.fill_style = FillStippled;
		values.foreground = BlackPixelOfScreen(XtScreen(sbw));
		values.stipple = t_pix;
		valueMask |= (GCFillStyle | GCStipple);
	}


    sbw->scrollBar.unavailable_GC = XtGetGC((Widget) sbw, valueMask,
		&values);
}




/************************************************************************
 *
 *  Logic of the scrollbar pixmap management:
 *  ----------------------------------------
 *     A pixmap the size of the trough area is created each time the
 *     scrollbar change size.
 *     This pixmap receives the drawing of the slider which is then
 *     copied on the scrollbar window whenever exposure is needed.
 *     GetSliderPixmap:
 *         creates the pixmap and possibly free the current one if present.
 *         the pixmap is free upon destruction of the widget.
 *         the field pixmap == 0 means there is no pixmap to freed.
 *         Is called from Resize method.
 *     DrawSliderPixmap: 
 *         drawss the slider graphics (sized shadowed rectangle) in the pixmap.
 *         the fields slider_width and height must have been calculated.
 *         Is called from Resize, after the pixmap has been created,
 *           and from SetValues, if something has changed in the visual 
 *           of the slider.
 *     RedrawSliderWindow:
 *         clears the current scrollbar slider area, computes the
 *         new position and dumps the slider pixmap at its new position.
 *         Is called from SetValues method, from increment actions, and
 *         from ChangeScrollBarValue (from Select, Timer).
 *
 ************************************************************************/


/************************************************************************
 *
 *  GetSliderPixmap
 *     Create the new pixmap for the slider.
 *     This pixmap is the size of the widget minus the arrows.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetSliderPixmap( sbw )
        XmScrollBarWidget sbw ;
#else
GetSliderPixmap(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{

   if (sbw->scrollBar.pixmap)
      XFreePixmap (XtDisplay (sbw), sbw->scrollBar.pixmap);

   sbw->scrollBar.pixmap = 
      XCreatePixmap (XtDisplay(sbw), RootWindowOfScreen(XtScreen(sbw)),
                     sbw->scrollBar.slider_area_width, 
		     sbw->scrollBar.slider_area_height, 
		     sbw->core.depth);
}





/************************************************************************
 *
 *  DrawSliderPixmap
 *     Draw the slider graphic (shadowed rectangle) into the pixmap.
 *     Use the private field etched_slider to draw a line in the middle,
 *       which is turned on by the scale code.
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawSliderPixmap( sbw )
        XmScrollBarWidget sbw ;
#else
DrawSliderPixmap(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
   register int slider_width = sbw->scrollBar.slider_width;
   register int slider_height = sbw->scrollBar.slider_height;
   register Drawable slider = sbw->scrollBar.pixmap;


   XFillRectangle (XtDisplay ((Widget) sbw), slider,
                   sbw->scrollBar.foreground_GC,
                   0, 0, slider_width, slider_height);
   
   _XmDrawShadows (XtDisplay (sbw), slider,
		 sbw->primitive.top_shadow_GC,
		 sbw->primitive.bottom_shadow_GC, 
		 0, 0, slider_width, slider_height,
		 sbw->primitive.shadow_thickness,
		 XmSHADOW_OUT);

   if (sbw->scrollBar.etched_slider)
   {
      if (sbw->scrollBar.orientation == XmHORIZONTAL)
      {
         XDrawLine (XtDisplay (sbw), slider,
                    sbw->primitive.bottom_shadow_GC,
                    slider_width / 2 - 1, 1, 
                    slider_width / 2 - 1, slider_height - 2);
         XDrawLine (XtDisplay (sbw), slider,
                    sbw->primitive.top_shadow_GC,
                    slider_width / 2, 1, 
                    slider_width / 2, slider_height - 2);
      }
      else
      {
         XDrawLine (XtDisplay (sbw), slider,
                    sbw->primitive.bottom_shadow_GC,
                    1, slider_height / 2 - 1,
                    slider_width - 2, slider_height / 2 - 1);
         XDrawLine (XtDisplay (sbw), slider,
                    sbw->primitive.top_shadow_GC,
                    1, slider_height / 2,
                    slider_width - 2, slider_height / 2);
      }
   }
}


#ifdef CDE_VISUAL  /* sliding_mode */
static void
#ifdef _NO_PROTO
DrawThermometer(sbw)
    XmScrollBarWidget sbw;
#else
DrawThermometer(XmScrollBarWidget sbw)
#endif /* _NO_PROTO */
{
#define SBP(a)	((a)->scrollBar)

    float fill_percentage;
    int x, y, width, height;

    x = SBP(sbw).slider_area_x;
    y = SBP(sbw).slider_area_y;
    fill_percentage = (float) SBP(sbw).value
		/ (float) (SBP(sbw).maximum - SBP(sbw).slider_size
			- SBP(sbw).minimum);

    if (sbw->scrollBar.orientation == XmHORIZONTAL) {
	height = SBP(sbw).slider_area_height;
	width = fill_percentage * (float) SBP(sbw).slider_area_width + 0.5;

	if (SBP(sbw).processing_direction == XmMAX_ON_RIGHT) {
	    x += width;
	    width = SBP(sbw).slider_area_width - width;
	}
    }
    else {
	width = SBP(sbw).slider_area_width;
	height = fill_percentage * (float) SBP(sbw).slider_area_height + 0.5;

	if (SBP(sbw).processing_direction == XmMAX_ON_BOTTOM) {
	    y += height;
	    height = SBP(sbw).slider_area_height - height;
	}
    }

    XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
		       sbw->scrollBar.foreground_GC,
		       x, y, width > 0 ? width : 0,
		       height > 0 ? height : 0);
}
#endif


/************************************************************************
 *
 *  RedrawSliderWindow
 *	Clear the through area at the current slider position,
 *      recompute the slider coordinates and redraw the slider the window by
 *      copying from the pixmap graphics.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
RedrawSliderWindow( sbw )
        XmScrollBarWidget sbw ;
#else
RedrawSliderWindow(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
    if (XtIsRealized(sbw))
	XClearArea(XtDisplay ((Widget) sbw), XtWindow ((Widget) sbw),
		   (int) sbw->scrollBar.slider_area_x,
		   (int) sbw->scrollBar.slider_area_y,
		   (unsigned int) sbw->scrollBar.slider_area_width,
		   (unsigned int) sbw->scrollBar.slider_area_height,
		   (Bool) FALSE);

    CalcSliderRect(sbw,
		   &(sbw->scrollBar.slider_x),
		   &(sbw->scrollBar.slider_y), 
		   &(sbw->scrollBar.slider_width),
		   &(sbw->scrollBar.slider_height));
	

#ifdef CDE_VISUAL  /* sliding_mode */
    if (XtIsRealized(sbw) && (sbw->scrollBar.etched_slider == THERMOMETER_ON))
	DrawThermometer(sbw);

    /* use the pixmap that contains the slider graphics */
    else if (XtIsRealized(sbw) && sbw->scrollBar.pixmap)
	XCopyArea (XtDisplay ((Widget) sbw),
		   sbw->scrollBar.pixmap, XtWindow ((Widget) sbw),
		   sbw->scrollBar.foreground_GC,
		   0, 0,
		   sbw->scrollBar.slider_width, sbw->scrollBar.slider_height,
		   sbw->scrollBar.slider_x, sbw->scrollBar.slider_y);
#else
    /* use the pixmap that contains the slider graphics */
    if (XtIsRealized(sbw) && sbw->scrollBar.pixmap)
	XCopyArea (XtDisplay ((Widget) sbw),
		   sbw->scrollBar.pixmap, XtWindow ((Widget) sbw),
		   sbw->scrollBar.foreground_GC,
		   0, 0,
		   sbw->scrollBar.slider_width, sbw->scrollBar.slider_height,
		   sbw->scrollBar.slider_x, sbw->scrollBar.slider_y);
#endif
}




/************************************************************************
 *
 *  CalcSliderRect
 *     Calculate the slider location and size in pixels so that
 *     it can be drawn.  Note that number and location of pixels
 *     is always positive, so no special case rounding is needed.
 *     DD: better be a CalcSliderPosition and CalcSliderSize, since
 *         this routine is often use for one _or_ the other case.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CalcSliderRect( sbw, slider_x, slider_y, slider_width, slider_height )
        XmScrollBarWidget sbw ;
        short *slider_x ;
        short *slider_y ;
        short *slider_width ;
        short *slider_height ;
#else
CalcSliderRect(
        XmScrollBarWidget sbw,
        short *slider_x,
        short *slider_y,
        short *slider_width,
        short *slider_height )
#endif /* _NO_PROTO */
{
	float range;
	float trueSize;
	float factor;
	float slideSize;
	int minSliderWidth;
	int minSliderHeight;
	int hitTheWall = 0;


	/* Set up */
	if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
		trueSize =  sbw->scrollBar.slider_area_width;
		minSliderWidth = MIN_SLIDER_LENGTH;
		minSliderHeight = MIN_SLIDER_THICKNESS;
	}
	else /* orientation == XmVERTICAL */
	{
		trueSize = sbw->scrollBar.slider_area_height;
		minSliderWidth = MIN_SLIDER_THICKNESS;
		minSliderHeight = MIN_SLIDER_LENGTH;
	}

	/* Total number of user units displayed */
	range = sbw->scrollBar.maximum - sbw->scrollBar.minimum;

	/* A niave notion of pixels per user unit */
	factor = trueSize / range;

	/* A naive notion of the size of the slider in pixels */
	slideSize = (float) (sbw->scrollBar.slider_size) * factor;

	/* NOTE SIDE EFFECT */
#define MAX_SCROLLBAR_DIMENSION(val, min)\
	((val) > (min)) ? (val) : (hitTheWall = min)

	/* Don't let the slider get too small */
	if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
		*slider_width = MAX_SCROLLBAR_DIMENSION(
			(int) (slideSize + 0.5), minSliderWidth);
		*slider_height = MAX(sbw->scrollBar.slider_area_height,
			minSliderHeight);
	}
	else /* orientation == XmVERTICAL */
	{
		*slider_width = MAX(sbw->scrollBar.slider_area_width,
			minSliderWidth);
		*slider_height = MAX_SCROLLBAR_DIMENSION((int)
			(slideSize + 0.5), minSliderHeight);
	}

	if (hitTheWall)
	{
		/*
		 * The slider has not been allowed to take on its true
		 * proportionate size (it would have been too small).  This
		 * breaks proportionality of the slider and the conversion
		 * between pixels and user units.
		 *
		 * The factor needs to be tweaked in this case.
		 */

		trueSize -= hitTheWall; /* actual pixels available */
		range -= sbw->scrollBar.slider_size; /* actual range */
	        if (range == 0) range = 1;
		factor = trueSize / range;

	}

	if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
		/* Many parentheses to explicitly control type conversion. */
		*slider_x = ((int) (((((float) sbw->scrollBar.value)
			- ((float) sbw->scrollBar.minimum)) * factor) + 0.5))
			+ sbw->scrollBar.slider_area_x;
		*slider_y = sbw->scrollBar.slider_area_y;
	}
	else
	{
		*slider_x = sbw->scrollBar.slider_area_x;
		*slider_y = ((int) (((((float) sbw->scrollBar.value)
			- ((float) sbw->scrollBar.minimum)) * factor) + 0.5))
			+ sbw->scrollBar.slider_area_y;
	}

	/* One final adjustment (of questionable value--preserved
	   for visual backward compatibility) */

	if ((sbw->scrollBar.orientation == XmHORIZONTAL)
		&&
		((*slider_x + *slider_width) > (sbw->scrollBar.slider_area_x
			+ sbw->scrollBar.slider_area_width)))
	{
		*slider_x = sbw->scrollBar.slider_area_x
			+ sbw->scrollBar.slider_area_width - *slider_width;
	}

	if ((sbw->scrollBar.orientation == XmVERTICAL)
		&&
		((*slider_y + *slider_height) > (sbw->scrollBar.slider_area_y
			+ sbw->scrollBar.slider_area_height)))
	{
		*slider_y = sbw->scrollBar.slider_area_y
			+ sbw->scrollBar.slider_area_height - *slider_height;
	}
}




/************************************************************************
 *
 *  Redisplay
 *     General redisplay function called on exposure events.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Redisplay( wid, event, region )
        Widget wid ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget wid,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
    XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;


    if (sbw->primitive.shadow_thickness > 0)
	_XmDrawShadows (XtDisplay (sbw), XtWindow (sbw), 
		      sbw->primitive.bottom_shadow_GC, 
		      sbw->primitive.top_shadow_GC,
		      sbw->primitive.highlight_thickness,
		      sbw->primitive.highlight_thickness,
		      sbw->core.width-2 * 
		        sbw->primitive.highlight_thickness,
		      sbw->core.height-2 * 
		        sbw->primitive.highlight_thickness,
		      sbw->primitive.shadow_thickness,
		      XmSHADOW_OUT);

#ifdef CDE_VISUAL  /* sliding_mode */
    if (sbw->scrollBar.etched_slider == THERMOMETER_ON)
       DrawThermometer(sbw);
    else
	/* dump the pixmap that contains the slider graphics */
	XCopyArea (XtDisplay ((Widget) sbw),
	       sbw->scrollBar.pixmap, XtWindow ((Widget) sbw),
	       sbw->scrollBar.foreground_GC,
	       0, 0,
	       sbw->scrollBar.slider_width, sbw->scrollBar.slider_height,
	       sbw->scrollBar.slider_x, sbw->scrollBar.slider_y);

#else
    /* dump the pixmap that contains the slider graphics */
    XCopyArea (XtDisplay ((Widget) sbw),
	       sbw->scrollBar.pixmap, XtWindow ((Widget) sbw),
	       sbw->scrollBar.foreground_GC,
	       0, 0,
	       sbw->scrollBar.slider_width, sbw->scrollBar.slider_height,
	       sbw->scrollBar.slider_x, sbw->scrollBar.slider_y);
#endif

    if (sbw -> scrollBar.show_arrows) {
      DRAWARROW(sbw, ((sbw->scrollBar.arrow1_selected)?
		 sbw -> primitive.bottom_shadow_GC:
		 sbw -> primitive.top_shadow_GC),
		((sbw->scrollBar.arrow1_selected)?
		 sbw -> primitive.top_shadow_GC :
		 sbw -> primitive.bottom_shadow_GC),
		sbw->scrollBar.arrow1_x,
		sbw->scrollBar.arrow1_y,
		sbw->scrollBar.arrow1_orientation);
      DRAWARROW(sbw, ((sbw->scrollBar.arrow2_selected)?
		 sbw -> primitive.bottom_shadow_GC:
		 sbw -> primitive.top_shadow_GC),
		((sbw->scrollBar.arrow2_selected)?
		 sbw -> primitive.top_shadow_GC :
		 sbw -> primitive.bottom_shadow_GC),
		sbw->scrollBar.arrow2_x, 
		sbw->scrollBar.arrow2_y,
		sbw->scrollBar.arrow2_orientation);
  }


    if (!(sbw->core.sensitive)) {
	XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
		       sbw->scrollBar.unavailable_GC,
		       sbw->primitive.highlight_thickness
		       + sbw->primitive.shadow_thickness,
		       sbw->primitive.highlight_thickness
		       + sbw->primitive.shadow_thickness,
		       XtWidth(sbw) - (2 * (sbw->primitive.highlight_thickness
				+ sbw->primitive.shadow_thickness)),
		       XtHeight(sbw) - (2 * (sbw->primitive.highlight_thickness
				+ sbw->primitive.shadow_thickness)));
    }
    else if (sbw->scrollBar.show_arrows)
    {
#ifdef FUNKY_INSENSITIVE_VISUAL
        if (!(sbw->scrollBar.flags & ARROW1_AVAILABLE))
        {
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);
        }
        if (!(sbw->scrollBar.flags & ARROW2_AVAILABLE))
        {
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);
        }
#endif
    }

   if (sbw -> primitive.highlighted)
      (*(((XmPrimitiveWidgetClass) XtClass(sbw))
		->primitive_class.border_highlight)) ((Widget) sbw);
   else
      (*(((XmPrimitiveWidgetClass) XtClass(sbw))
		->primitive_class.border_unhighlight)) ((Widget) sbw);
}





/************************************************************************
 *
 *  Resize
 *     Process resizes on the widget by destroying and recreating the
 *     slider pixmap.
 *     Also draw the correct sized slider onto this pixmap.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
    register int ht = sbw->primitive.highlight_thickness;
    register int st = sbw->primitive.shadow_thickness;

#define CHECK(x) if (x <= 0) x = 1 ;


    /*  Calculate all of the internal data for slider */

    if (sbw->scrollBar.show_arrows) {
	if (sbw->scrollBar.orientation == XmHORIZONTAL) {
	    sbw->scrollBar.arrow1_orientation = XmARROW_LEFT;
	    sbw->scrollBar.arrow2_orientation = XmARROW_RIGHT;

	    /*  left arrow position and size  */

	    sbw->scrollBar.arrow1_x = ht + st;
	    sbw->scrollBar.arrow1_y = ht + st;
	    sbw->scrollBar.arrow_width = 
		sbw->scrollBar.arrow_height = sbw->core.height
		    - 2 * (ht + st);

	    if (sbw->core.width < 
		2 * (sbw->scrollBar.arrow_width + ht + st)
		+ MIN_SLIDER_LENGTH + 2)
		sbw->scrollBar.arrow_width = (sbw->core.width 
			 - (MIN_SLIDER_LENGTH + 2 + 2 * (ht + st))) / 2;

	    /*  slide area position and size  */

	    sbw->scrollBar.slider_area_x = 
		ht + st + sbw->scrollBar.arrow_width + 1;

	    sbw->scrollBar.slider_area_width =
		sbw->core.width 
		    - 2 * (ht + st + sbw->scrollBar.arrow_width + 1);

	    if ((2*(ht+st)) > XtHeight(sbw))
		sbw->scrollBar.slider_area_y = XtHeight(sbw) / 2;
	    else
		sbw->scrollBar.slider_area_y = ht + st;

	    sbw->scrollBar.slider_area_height =
		sbw->core.height - 2 * (ht + st);


	    /*  right arrow position  */

	    sbw->scrollBar.arrow2_x = ht + st
		+ sbw->scrollBar.arrow_width + 1 +
		    sbw->scrollBar.slider_area_width + 1;
	    sbw->scrollBar.arrow2_y = ht + st;
	} else {
	    sbw->scrollBar.arrow1_orientation = XmARROW_UP;
	    sbw->scrollBar.arrow2_orientation = XmARROW_DOWN;

	    /*  top arrow position and size  */

	    sbw->scrollBar.arrow1_x = ht + st;
	    sbw->scrollBar.arrow1_y = ht + st;
	    sbw->scrollBar.arrow_width = sbw->scrollBar.arrow_height =
		sbw->core.width - 2 * (ht + st);

	    if (sbw->core.height < 
		2 * (sbw->scrollBar.arrow_height + ht + st)
		+ MIN_SLIDER_LENGTH +2)
		sbw->scrollBar.arrow_height = (sbw->core.height
			- (MIN_SLIDER_LENGTH + 2 + 2 * (ht + st))) / 2;

	    /*  slide area position and size  */

	    sbw->scrollBar.slider_area_y = 
		ht + st + sbw->scrollBar.arrow_height + 1;
	    sbw->scrollBar.slider_area_height = sbw->core.height
		- 2 * (ht + st + sbw->scrollBar.arrow_height +1);

	    if ((2*(st+ht)) > XtWidth(sbw))
		sbw->scrollBar.slider_area_x = XtWidth(sbw) / 2;
	    else
		sbw->scrollBar.slider_area_x = ht + st;

	    sbw->scrollBar.slider_area_width = sbw->core.width
		- 2 * (ht + st);


	    /*  right arrow position  */

	    sbw->scrollBar.arrow2_y = ht + st
		+ sbw->scrollBar.arrow_height + 1
		    + sbw->scrollBar.slider_area_height + 1;
	    sbw->scrollBar.arrow2_x = ht + st;
	}

	CHECK(sbw->scrollBar.arrow_height);
	CHECK(sbw->scrollBar.arrow_width);
    } else {
	sbw->scrollBar.arrow_width = 0;
	sbw->scrollBar.arrow_height = 0;

	if (sbw->scrollBar.orientation == XmHORIZONTAL) {
	    /*  slide area position and size  */

	    sbw->scrollBar.slider_area_x = ht + st;
	    sbw->scrollBar.slider_area_width = sbw->core.width
		- 2 * (ht + st);

	    if ((2*(ht+st)) > XtHeight(sbw))
		sbw->scrollBar.slider_area_y = XtHeight(sbw) / 2;
	    else
		sbw->scrollBar.slider_area_y = ht + st;
	    sbw->scrollBar.slider_area_height = sbw->core.height
		- 2 * (ht + st);
	} else {
	    /*  slide area position and size  */

	    sbw->scrollBar.slider_area_y = ht + st;
	    sbw->scrollBar.slider_area_height = sbw->core.height
		- 2 * (ht + st);

	    if ((2*(st+ht)) > XtWidth(sbw))
		sbw->scrollBar.slider_area_x = XtWidth(sbw) / 2;
	    else
		sbw->scrollBar.slider_area_x = ht + st;
	    sbw->scrollBar.slider_area_width = sbw->core.width
		- 2 * (ht + st);
	}
    }

    CHECK(sbw->scrollBar.slider_area_height);
    CHECK(sbw->scrollBar.slider_area_width);

    GetSliderPixmap (sbw); /* the size of the scrollbar window - arrows */

    CalcSliderRect(sbw,
		   &(sbw->scrollBar.slider_x),
		   &(sbw->scrollBar.slider_y), 
		   &(sbw->scrollBar.slider_width),
		   &(sbw->scrollBar.slider_height));
	
    DrawSliderPixmap (sbw); 
}




/*********************************************************************
 *
 * Realize
 *
 ********************************************************************/
static void 
#ifdef _NO_PROTO
Realize( sbw, window_mask, window_attributes )
        Widget sbw ;
        XtValueMask *window_mask ;
        XSetWindowAttributes *window_attributes ;
#else
Realize(
        Widget sbw,
        XtValueMask *window_mask,
        XSetWindowAttributes *window_attributes )
#endif /* _NO_PROTO */
{
    (*window_mask) |= (CWBackPixel | CWBitGravity);
    window_attributes->background_pixel 
	= ((XmScrollBarWidget) sbw)->scrollBar.trough_color;
    window_attributes->bit_gravity = ForgetGravity;
	
    XtCreateWindow (sbw, InputOutput, CopyFromParent, *window_mask,
		    window_attributes);
}




/************************************************************************
 *
 *  Destroy
 *	Clean up allocated resources when the widget is destroyed.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;

    XtReleaseGC ((Widget) sbw, sbw->scrollBar.foreground_GC);
    XtReleaseGC ((Widget) sbw, sbw->scrollBar.unavailable_GC);

    if (sbw->scrollBar.pixmap != 0)
	XFreePixmap (XtDisplay (sbw), sbw->scrollBar.pixmap);

    XtRemoveAllCallbacks (wid, XmNvalueChangedCallback);
    XtRemoveAllCallbacks (wid, XmNincrementCallback);
    XtRemoveAllCallbacks (wid, XmNdecrementCallback);
    XtRemoveAllCallbacks (wid, XmNpageIncrementCallback);
    XtRemoveAllCallbacks (wid, XmNpageDecrementCallback);
    XtRemoveAllCallbacks (wid, XmNdragCallback);
    XtRemoveAllCallbacks (wid, XmNtoTopCallback);
    XtRemoveAllCallbacks (wid, XmNtoBottomCallback);

    if (sbw->scrollBar.timer != 0)
   {
       XtRemoveTimeOut (sbw->scrollBar.timer);
       sbw->scrollBar.timer = 0;
   }
}




/************************************************************************
 *
 *  ValidateInputs
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
    ValidateInputs( current, request, new_w )
XmScrollBarWidget current ;
XmScrollBarWidget request ;
XmScrollBarWidget new_w ;
#else
ValidateInputs(
	       XmScrollBarWidget current,
	       XmScrollBarWidget request,
	       XmScrollBarWidget new_w )
#endif /* _NO_PROTO */
{
    Boolean returnFlag = TRUE;
    
    /* Validate the incoming data  */                      
    
    if (new_w->scrollBar.minimum >= new_w->scrollBar.maximum)
	{
	    new_w->scrollBar.minimum = current->scrollBar.minimum;
	    new_w->scrollBar.maximum = current->scrollBar.maximum;
	    _XmWarning( (Widget) new_w, MESSAGE1);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.slider_size < 1)
	{
	    if ((new_w->scrollBar.maximum - new_w->scrollBar.minimum) <
		current->scrollBar.slider_size)
		new_w->scrollBar.slider_size = new_w->scrollBar.maximum
		    - new_w->scrollBar.minimum;
	    else
		new_w->scrollBar.slider_size = current->scrollBar.slider_size;
	    _XmWarning( (Widget) new_w, MESSAGE2);
	    returnFlag = FALSE;
	}
    
    if ((new_w->scrollBar.slider_size > 
	 new_w->scrollBar.maximum - new_w->scrollBar.minimum))
	{
	    if ((new_w->scrollBar.maximum - new_w->scrollBar.minimum) <
		current->scrollBar.slider_size)
		new_w->scrollBar.slider_size = new_w->scrollBar.maximum
		    - new_w->scrollBar.minimum;
	    else
		new_w->scrollBar.slider_size = current->scrollBar.slider_size;
	    _XmWarning( (Widget) new_w, MESSAGE13);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.value < new_w->scrollBar.minimum)
	{
	    new_w->scrollBar.value = new_w->scrollBar.minimum;
	    _XmWarning( (Widget) new_w, MESSAGE3);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.value > 
	new_w->scrollBar.maximum - new_w->scrollBar.slider_size)
	{
	    new_w->scrollBar.value = 
		new_w->scrollBar.maximum - new_w->scrollBar.slider_size;
	    _XmWarning( (Widget) new_w, MESSAGE4);
	    returnFlag = FALSE;
	}
    
    if(  !XmRepTypeValidValue( XmRID_ORIENTATION,
			      new_w->scrollBar.orientation, (Widget) new_w))
	{
	    new_w->scrollBar.orientation = current->scrollBar.orientation;
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.orientation == XmHORIZONTAL)
	{
	    if ((new_w->scrollBar.processing_direction != XmMAX_ON_LEFT) &&
		(new_w->scrollBar.processing_direction != XmMAX_ON_RIGHT))
		{
		    new_w->scrollBar.processing_direction = 
			current->scrollBar.processing_direction;
		    _XmWarning( (Widget) new_w, MESSAGE6);
		    returnFlag = FALSE;
		}
	}
    else /* new_w->scrollBar.orientation == XmVERTICAL */
	{
	    if ((new_w->scrollBar.processing_direction != XmMAX_ON_TOP) &&
		(new_w->scrollBar.processing_direction != XmMAX_ON_BOTTOM))
		{
		    new_w->scrollBar.processing_direction =
			current->scrollBar.processing_direction;
		    _XmWarning( (Widget) new_w, MESSAGE6);
		    returnFlag = FALSE;
		}
	}
    
    if (new_w->scrollBar.increment <= 0)
	{
	    new_w->scrollBar.increment = current->scrollBar.increment;
	    _XmWarning( (Widget) new_w, MESSAGE7);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.page_increment <= 0)
	{
	    new_w->scrollBar.page_increment = 
		current->scrollBar.page_increment;
	    _XmWarning( (Widget) new_w,  MESSAGE8);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.initial_delay <= 0)
	{
	    new_w->scrollBar.initial_delay = current->scrollBar.initial_delay;
	    _XmWarning( (Widget) new_w, MESSAGE9);
	    returnFlag = FALSE;
	}
    
    if (new_w->scrollBar.repeat_delay <= 0)
	{
	    new_w->scrollBar.repeat_delay = current->scrollBar.repeat_delay;
	    _XmWarning( (Widget) new_w, MESSAGE10);
	    returnFlag = FALSE;
	}
    
    return(returnFlag);
}

/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw;
        Widget rw;
        Widget nw;
        ArgList args;
        Cardinal *num_args;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmScrollBarWidget current = (XmScrollBarWidget) cw ;
    XmScrollBarWidget request = (XmScrollBarWidget) rw ;
    XmScrollBarWidget new_w = (XmScrollBarWidget) nw ;
    Boolean returnFlag = FALSE;
    Boolean current_backwards = 
	((current->scrollBar.processing_direction == XmMAX_ON_LEFT)
	 || (current->scrollBar.processing_direction == XmMAX_ON_TOP));
    Boolean new_backwards =
	((new_w->scrollBar.processing_direction == XmMAX_ON_LEFT)
	 || (new_w->scrollBar.processing_direction == XmMAX_ON_TOP));



    /* Make sure that processing direction tracks orientation */
    
    if ((new_w->scrollBar.orientation != current->scrollBar.orientation)
	&&
	(new_w->scrollBar.processing_direction ==
	 current->scrollBar.processing_direction))
	{
	    if ((new_w->scrollBar.orientation == XmHORIZONTAL) &&
		(current->scrollBar.processing_direction == XmMAX_ON_TOP))
		new_w->scrollBar.processing_direction = XmMAX_ON_LEFT;
	    else if ((new_w->scrollBar.orientation == XmHORIZONTAL) &&
		     (current->scrollBar.processing_direction ==
		      XmMAX_ON_BOTTOM))
		new_w->scrollBar.processing_direction = XmMAX_ON_RIGHT;
	    else if ((new_w->scrollBar.orientation == XmVERTICAL) &&
		     (current->scrollBar.processing_direction == XmMAX_ON_LEFT))
		new_w->scrollBar.processing_direction = XmMAX_ON_TOP;
	    else if ((new_w->scrollBar.orientation == XmVERTICAL) &&
		     (current->scrollBar.processing_direction == XmMAX_ON_RIGHT))
		new_w->scrollBar.processing_direction = XmMAX_ON_BOTTOM;
	}
    
    while (!ValidateInputs(current, request, new_w));
    
    /*
     * Because someone somewhere originally thought that it was clever
     * for the scrollbar widget to do all of its internal processing in
     * just one direction, all of the interface procedures have to go
     * through extreme gymnastics to support reversal.
     */
    if ((new_backwards && !current_backwards) ||
	(!new_backwards && current_backwards))
	{
	    if (new_w->scrollBar.flags & VALUE_SET_FLAG)
		{
		    if (new_backwards)
			new_w->scrollBar.value = new_w->scrollBar.maximum
			    + new_w->scrollBar.minimum - new_w->scrollBar.value
				- new_w->scrollBar.slider_size;
		}
	    else
		{
		    new_w->scrollBar.value = new_w->scrollBar.maximum
			+ new_w->scrollBar.minimum - new_w->scrollBar.value
			    - new_w->scrollBar.slider_size;
		}
	}
    else
	{
	    if ((new_w->scrollBar.flags & VALUE_SET_FLAG) &&
		(new_backwards))
		new_w->scrollBar.value = new_w->scrollBar.maximum
		    + new_w->scrollBar.minimum - new_w->scrollBar.value
			- new_w->scrollBar.slider_size;
	}
    
    if (new_w->scrollBar.flags & VALUE_SET_FLAG)
	new_w->scrollBar.flags &= ~VALUE_SET_FLAG;
    
    /*  See if the GC needs to be regenerated  */
    
    if (new_w->core.background_pixel != current->core.background_pixel)
	{
	    XtReleaseGC((Widget) new_w, new_w->scrollBar.foreground_GC);
	    GetForegroundGC(new_w);
	}
    
    /*
     * See if the trough (a.k.a the window background) needs to be
     * changed.
     */
    if ((new_w->scrollBar.trough_color != current->scrollBar.trough_color)
	&& (XtIsRealized(nw)))
	{
	    XSetWindowBackground(XtDisplay((Widget)new_w),
				 XtWindow((Widget)new_w), 
				 new_w->scrollBar.trough_color);
	    returnFlag = TRUE;
	}
    
    /*
     * See if the widget needs to be redrawn.  Minimize the amount
     * of redraw by having specific checks.
     */
    
    if ((new_w->scrollBar.orientation != 
	 current->scrollBar.orientation)         ||
	(new_w->primitive.shadow_thickness !=
	 current->primitive.shadow_thickness)    || 
	(new_w->primitive.highlight_thickness !=
	 current->primitive.highlight_thickness) || 
	(new_w->scrollBar.show_arrows != 
	 current->scrollBar.show_arrows))
	{
	    /* call Resize method, that will have the effect of
	       recomputing all the internal variables (arrow size, 
	       trough are) and recreating the slider pixmap. */
	    (* (new_w->core.widget_class->core_class.resize)) 
		((Widget) new_w);
	    returnFlag = TRUE;
	}
    
    if ((new_w->primitive.foreground != 
	 current->primitive.foreground)
	||
	(new_w->core.background_pixel != current->core.background_pixel)
	||
	(new_w->primitive.top_shadow_color !=
	 current->primitive.top_shadow_color)
	||
	(new_w->primitive.bottom_shadow_color !=
	 current->primitive.bottom_shadow_color))
	{
	    returnFlag = TRUE;
	    /* only draw the slider graphics, no need to change the
	       pixmap (call to GetSliderPixmap) nor the slider size 
	       (call to CalcSliderRect). */
	    DrawSliderPixmap(new_w);
	    
	}
    
    if ((new_w->scrollBar.slider_size != 
	 current->scrollBar.slider_size)                    ||
	(new_w->scrollBar.minimum != current->scrollBar.minimum) ||
	(new_w->scrollBar.maximum != current->scrollBar.maximum) ||
	(new_w->scrollBar.processing_direction != 
	 current->scrollBar.processing_direction)) {
	
	/* have to clear the current slider before setting the
	   new slider position and size */
	if (XtIsRealized(nw))
	    XClearArea(XtDisplay((Widget)new_w), 
		       XtWindow((Widget)new_w),
		       new_w->scrollBar.slider_x, 
		       new_w->scrollBar.slider_y,
		       new_w->scrollBar.slider_width,
		       new_w->scrollBar.slider_height, False);
	    
	/* recompute the slider size and draw in the pixmap */
	CalcSliderRect(new_w,
		       &(new_w->scrollBar.slider_x),
		       &(new_w->scrollBar.slider_y), 
		       &(new_w->scrollBar.slider_width),
		       &(new_w->scrollBar.slider_height));
	
	/* redraw the slider in the pixmap */
	DrawSliderPixmap (new_w); 

	if (new_w->scrollBar.slider_size >= (new_w->scrollBar.maximum
					     - new_w->scrollBar.minimum))
	    {
		new_w->scrollBar.flags &= ~SLIDER_AVAILABLE;
		/*
		 * Disabling the slider enables the arrows.  This 
		 * leaves the scrollbar in a state amenable to reenabling
		 * the slider.
		 */
		new_w->scrollBar.flags |= ARROW1_AVAILABLE;
		new_w->scrollBar.flags |= ARROW2_AVAILABLE;
		returnFlag = TRUE;	
	    }
	else
	    {
		if (! (new_w->scrollBar.flags & SLIDER_AVAILABLE)) {
		    returnFlag = TRUE;
		    new_w->scrollBar.flags |= SLIDER_AVAILABLE;
		} else {
		    /* directly use the pixmap that contains the slider 
		       graphics, no need to call RedrawSliderWindow since the
		       cleararea and the calcrect have already been made */
		    if (XtIsRealized(nw))
			XCopyArea (XtDisplay ((Widget) new_w),
				   new_w->scrollBar.pixmap, 
				   XtWindow ((Widget) new_w),
				   new_w->scrollBar.foreground_GC,
				   0, 0,
				   new_w->scrollBar.slider_width, 
				   new_w->scrollBar.slider_height,
				   new_w->scrollBar.slider_x, 
				   new_w->scrollBar.slider_y);
		}
	    }
    }
    
    
    if (new_w->scrollBar.value != current->scrollBar.value) {
#ifdef FUNKY_INSENSITIVE_VISUAL
	if (new_w->scrollBar.value == new_w->scrollBar.minimum) {
	    if (XtIsRealized(nw))
		XFillRectangle(XtDisplay(new_w), XtWindow(new_w),
			       new_w->scrollBar.unavailable_GC,
			       new_w->scrollBar.arrow1_x,
			       new_w->scrollBar.arrow1_y,
			       new_w->scrollBar.arrow_width,
			       new_w->scrollBar.arrow_height);
	    
	    new_w->scrollBar.flags &= ~ARROW1_AVAILABLE;
	}
	else
	    {
		if ((new_w->scrollBar.show_arrows) &&
		    (! (new_w->scrollBar.flags & ARROW1_AVAILABLE)) &&
		    (XtIsRealized(nw)))
		    /*
		     * Or we could check for current value == minimum.
		     */
		    {
			XClearArea(XtDisplay(new_w), XtWindow(new_w),
				   new_w->scrollBar.arrow1_x,
				   new_w->scrollBar.arrow1_y,
				   new_w->scrollBar.arrow_width,
				   new_w->scrollBar.arrow_height,
				   FALSE);
			
			DRAWARROW (new_w, new_w -> primitive.top_shadow_GC,
				   new_w->primitive.bottom_shadow_GC,
				   new_w->scrollBar.arrow1_x,
				   new_w->scrollBar.arrow1_y,
				   new_w->scrollBar.arrow1_orientation);
		    }
		new_w->scrollBar.flags |= ARROW1_AVAILABLE;
	    }
	
	if (new_w->scrollBar.value == new_w->scrollBar.maximum
	    - new_w->scrollBar.slider_size)
	    {
		new_w->scrollBar.flags &= ~ARROW2_AVAILABLE;
	    }
	else{
	    if ((new_w->scrollBar.show_arrows) &&
		(! (new_w->scrollBar.flags & ARROW2_AVAILABLE)) &&
		(XtIsRealized(nw)))
		/*
		 * Or we could check for current value == max - slidersize
		 */
		{
		    XClearArea(XtDisplay(new_w), XtWindow(new_w),
			       new_w->scrollBar.arrow2_x,
			       new_w->scrollBar.arrow2_y,
			       new_w->scrollBar.arrow_width,
			       new_w->scrollBar.arrow_height,
			       FALSE);
		    
		    DRAWARROW (new_w, new_w -> primitive.top_shadow_GC,
			       new_w->primitive.bottom_shadow_GC,
			       new_w->scrollBar.arrow2_x,
			       new_w->scrollBar.arrow2_y,
			       new_w->scrollBar.arrow2_orientation);
		}
	    new_w->scrollBar.flags |= ARROW2_AVAILABLE;
	}
#endif
	/* the value has changed, the slider needs to move. */
	RedrawSliderWindow (new_w);

	/* no need to always return expose true */
	if (!(new_w->core.sensitive)) returnFlag = TRUE;
    }
    
    if (new_w->core.sensitive != current->core.sensitive)
	returnFlag = TRUE;
    
    return(returnFlag);
}




/************************************************************************
 *
 *  CalcSliderVal
 *     Calculate the slider val in application coordinates given
 *     the input x and y.
 *
 ************************************************************************/
static int 
#ifdef _NO_PROTO
CalcSliderVal( sbw, x, y )
        XmScrollBarWidget sbw ;
        int x ;
        int y ;
#else
CalcSliderVal(
        XmScrollBarWidget sbw,
        int x,
        int y )
#endif /* _NO_PROTO */
{
	int   margin;
	float range;
	float trueSize;       /* size of slider area in pixels */
	float referencePoint; /* origin of slider */
	float proportion;
	int int_proportion;
	int arrow_size;


	margin = sbw->primitive.highlight_thickness
		+ sbw->primitive.shadow_thickness;

	if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
		referencePoint = (float) x - sbw->scrollBar.separation_x;
		trueSize = sbw->scrollBar.slider_area_width
			- sbw->scrollBar.slider_width;
		arrow_size = sbw->scrollBar.arrow_width;
	}
	else
	{
		referencePoint = (float) y - sbw->scrollBar.separation_y;
		trueSize = sbw->scrollBar.slider_area_height
			- sbw->scrollBar.slider_height;
		arrow_size = sbw->scrollBar.arrow_height;
	}

	if (trueSize > 0)
		
		/* figure the proportion of slider area between the origin
		   of the slider area and the origin of the slider. */
		proportion = (referencePoint - arrow_size - margin) / trueSize;
	else
		/*
		 * We've got an interesting problem here.  There isn't any
		 * slider area available to slide in.  What should the value
		 * of the scrollbar be when the user tries to drag the slider?  
		 *
		 * Setting proportion to 1 snaps to maximum.  Setting
		 * proportion to the reciprocal of "range" will cause the
		 * slider to snap to the minimum.
		 *
		 */ 
		proportion = 1;

	/* Actual range displayed */
	range = sbw->scrollBar.maximum - sbw->scrollBar.minimum
		- sbw->scrollBar.slider_size;

	/* Now scale the proportion in pixels to user units */
	proportion = (proportion * range)
		+ ((float) sbw->scrollBar.minimum);
	
	/* Round off appropriately */
	if (proportion > 0)
		proportion += 0.5;
	else if (proportion < 0)
		proportion -= 0.5;

	int_proportion = (int) proportion;

	if (int_proportion < sbw->scrollBar.minimum)
		int_proportion = sbw->scrollBar.minimum;
	else if (int_proportion > (sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size))
		int_proportion = sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size;

	return (int_proportion);
}




/************************************************************************
 *
 *  Select
 *     This function processes selections occuring on the scrollBar.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Select( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Select(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
    XButtonPressedEvent *buttonEvent = (XButtonPressedEvent *) event ;
    int slider_x = sbw->scrollBar.slider_x;
    int slider_y = sbw->scrollBar.slider_y;
    int slider_width = sbw->scrollBar.slider_width;
    int slider_height = sbw->scrollBar.slider_height;
    Boolean slider_moved = False;
    
    /* add a start update when the button is pressed
       so that scrollbar moves generating widget
       configurations be bracketed for dropsite update.
       The endupdate is done in Release */
	       
    XmDropSiteStartUpdate(wid);
    
    if (XtGrabKeyboard(wid, False, GrabModeAsync,
		       GrabModeAsync, buttonEvent->time) == GrabSuccess)
	sbw->scrollBar.flags |= KEYBOARD_GRABBED;
    
    XAllowEvents(XtDisplay(wid), AsyncPointer, CurrentTime);
    XAllowEvents(XtDisplay(wid), AsyncKeyboard, CurrentTime);
    
    if (!(sbw->scrollBar.flags & SLIDER_AVAILABLE))
	return;
    if ((buttonEvent->button == Button1) &&
	(!XmIsScrolledWindow(XtParent(wid))))
	(void) XmProcessTraversal( (Widget) sbw, XmTRAVERSE_CURRENT);
    
    sbw->scrollBar.separation_x = 0;
    sbw->scrollBar.separation_y = 0;
    
    /*  Calculate whether the selection point is in the slider  */
    if ((buttonEvent->x >= slider_x)                 &&
	(buttonEvent->x <= slider_x + slider_width - 1)   &&
	(buttonEvent->y >= slider_y)                      &&
	(buttonEvent->y <= slider_y + slider_height - 1))
	{
	    sbw->scrollBar.initial_x = slider_x;
	    sbw->scrollBar.initial_y = slider_y;
	    sbw->scrollBar.sliding_on = True;
	    sbw->scrollBar.saved_value = sbw->scrollBar.value;
	    sbw->scrollBar.arrow1_selected = FALSE;
	    sbw->scrollBar.arrow2_selected = FALSE;
	    
	    if (buttonEvent->button == Button1)
		{
		    sbw->scrollBar.separation_x = buttonEvent->x - slider_x;
		    sbw->scrollBar.separation_y = buttonEvent->y - slider_y;
		}
	    else  /* Button2 */
		{
		    /* Warp the slider to the cursor, and then drag */
		    
		    if (sbw->scrollBar.orientation == XmHORIZONTAL)
			sbw->scrollBar.separation_x = 
			    sbw->scrollBar.slider_width / 2;
		    else
			sbw->scrollBar.separation_y = 
			    sbw->scrollBar.slider_height / 2;
		    
		    Moved ((Widget) sbw, (XEvent *) buttonEvent,
			   params, num_params);
		}
	    
	    return;
	}
    
    /* ... in the trough (i.e. slider area)... */
    else if ((buttonEvent->x >= sbw->scrollBar.slider_area_x)   &&
	     (buttonEvent->y >= sbw->scrollBar.slider_area_y)        &&
	     (buttonEvent->x <= sbw->scrollBar.slider_area_x 
	      + sbw->scrollBar.slider_area_width)                 &&
	     (buttonEvent->y <= sbw->scrollBar.slider_area_y 
	      + sbw->scrollBar.slider_area_height)) {
	sbw->scrollBar.arrow1_selected = FALSE;
	sbw->scrollBar.arrow2_selected = FALSE;
	sbw->scrollBar.saved_value = sbw->scrollBar.value;
	
	if (buttonEvent->button == Button1) {
	    /* Page the slider up or down */
	    
	    if (sbw->scrollBar.orientation == XmHORIZONTAL) {
		if (buttonEvent->x < sbw->scrollBar.slider_x)
		    sbw->scrollBar.change_type = XmCR_PAGE_DECREMENT;
		else
		    sbw->scrollBar.change_type = XmCR_PAGE_INCREMENT;
	    }
	    else
		{
		    if (buttonEvent->y < sbw->scrollBar.slider_y)
			sbw->scrollBar.change_type = XmCR_PAGE_DECREMENT;
		    else
			sbw->scrollBar.change_type = XmCR_PAGE_INCREMENT;
		}
	    slider_moved = ChangeScrollBarValue(sbw);
	}
	else  /* Button2 */
	    {
		/* Warp the slider to the cursor, and then drag */
		
		if (sbw->scrollBar.orientation == XmHORIZONTAL)
		    sbw->scrollBar.separation_x = 
			sbw->scrollBar.slider_width / 2;
		else
		    sbw->scrollBar.separation_y = 
			sbw->scrollBar.slider_height / 2;
		
		sbw->scrollBar.initial_x = slider_x;
		sbw->scrollBar.initial_y = slider_y;
		sbw->scrollBar.sliding_on = True;
		
		Moved ((Widget) sbw, (XEvent *) buttonEvent,
		       params, num_params);
		return;
	    }
    }
    
    /* ... in arrow 1 */
    else if ((buttonEvent->x >= sbw->scrollBar.arrow1_x)  &&
	     (buttonEvent->y >= sbw->scrollBar.arrow1_y)       &&
	     (buttonEvent->x <= sbw->scrollBar.arrow1_x 
	      + sbw->scrollBar.arrow_width)                 &&
	     (buttonEvent->y <= sbw->scrollBar.arrow1_y 
	      + sbw->scrollBar.arrow_height))
	{
	    sbw->scrollBar.change_type = XmCR_DECREMENT;
	    sbw->scrollBar.saved_value = sbw->scrollBar.value;
	    sbw->scrollBar.arrow1_selected = True;
	    
	    slider_moved = ChangeScrollBarValue(sbw);
	    DRAWARROW(sbw, sbw->primitive.bottom_shadow_GC,
		      sbw -> primitive.top_shadow_GC,
		      sbw->scrollBar.arrow1_x,
		      sbw->scrollBar.arrow1_y,
		      sbw->scrollBar.arrow1_orientation);
	}
    
    /* ... in arrow 2 */
    else if ((buttonEvent->x >= sbw->scrollBar.arrow2_x)      &&
	     (buttonEvent->y >= sbw->scrollBar.arrow2_y)           &&
	     (buttonEvent->x <= sbw->scrollBar.arrow2_x 
	      + sbw->scrollBar.arrow_width)                     &&
	     (buttonEvent->y <= sbw->scrollBar.arrow2_y 
	      + sbw->scrollBar.arrow_height))
	{
	    sbw->scrollBar.change_type = XmCR_INCREMENT;
	    sbw->scrollBar.saved_value = sbw->scrollBar.value;
	    sbw->scrollBar.arrow2_selected = True;
	    
	    if (slider_moved = ChangeScrollBarValue(sbw))
		{
		    DRAWARROW(sbw, sbw->primitive.bottom_shadow_GC,
			      sbw -> primitive.top_shadow_GC,
			      sbw->scrollBar.arrow2_x,
			      sbw->scrollBar.arrow2_y,
			      sbw->scrollBar.arrow2_orientation);
		}
	    else
		return;
	}
    else
	/* ... in the highlight area.  */
	return;
    
    if (slider_moved)
	{
	    ScrollCallback (sbw, sbw->scrollBar.change_type, 
			    sbw->scrollBar.value, 0, 0, (XEvent *) buttonEvent);
	    
	    XSync (XtDisplay((Widget)sbw), False);
	    
	    sbw->scrollBar.flags |= FIRST_SCROLL_FLAG ;
	    sbw->scrollBar.flags &= ~END_TIMER;
	    
	    
	    if (!sbw->scrollBar.timer)
		sbw->scrollBar.timer = XtAppAddTimeOut
		    (XtWidgetToApplicationContext((Widget) sbw),
		     (unsigned long) sbw->scrollBar.initial_delay,
		     TimerEvent, (XtPointer) sbw);
	}
}




/************************************************************************
 *
 *  Release
 *     This function processes releases occuring on the scrollBar.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Release( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Release(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
	XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;

	/* add an end update when the button is released.
	   see comment in Select for the start update */

	XmDropSiteEndUpdate(wid);

	if (sbw->scrollBar.flags & KEYBOARD_GRABBED)
	{
		XtUngrabKeyboard(wid, ((XButtonPressedEvent *)event)->time);
		sbw->scrollBar.flags &= ~KEYBOARD_GRABBED;
	}

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
        return;
#ifdef FUNKY_INSENSITIVE_VISUAL
	if ( (!(sbw->scrollBar.flags & ARROW1_AVAILABLE)) &&
		(sbw->scrollBar.value > sbw->scrollBar.minimum))
	{
		XClearArea(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.arrow1_x,
			sbw->scrollBar.arrow1_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height,
			FALSE);
		
		DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
			sbw->primitive.bottom_shadow_GC,
			sbw->scrollBar.arrow1_x,
			sbw->scrollBar.arrow1_y,
			sbw->scrollBar.arrow1_orientation);

		sbw->scrollBar.flags |= ARROW1_AVAILABLE;
	}
	else if (sbw->scrollBar.value == sbw->scrollBar.minimum)
		sbw->scrollBar.flags &= ~ARROW1_AVAILABLE;

	if ( (!(sbw->scrollBar.flags & ARROW2_AVAILABLE)) &&
		(sbw->scrollBar.value < (sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size)))
	{
		XClearArea(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.arrow2_x,
			sbw->scrollBar.arrow2_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height,
			FALSE);

		DRAWARROW (sbw, sbw->primitive.top_shadow_GC,
			sbw -> primitive.bottom_shadow_GC,
			sbw->scrollBar.arrow2_x,
			sbw->scrollBar.arrow2_y,
			sbw->scrollBar.arrow2_orientation);
		
		sbw->scrollBar.flags |= ARROW2_AVAILABLE;
	}
	else if (sbw->scrollBar.value == (sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size))
		sbw->scrollBar.flags &= ~ARROW2_AVAILABLE;
#endif
	if (sbw->scrollBar.arrow1_selected)
	{
		sbw->scrollBar.arrow1_selected = False;

		DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
			sbw->primitive.bottom_shadow_GC,
			sbw->scrollBar.arrow1_x,
			sbw->scrollBar.arrow1_y,
			sbw->scrollBar.arrow1_orientation);
	}

	if (sbw->scrollBar.arrow2_selected)
	{
		sbw->scrollBar.arrow2_selected = False;

		DRAWARROW (sbw, sbw->primitive.top_shadow_GC,
			sbw -> primitive.bottom_shadow_GC,
			sbw->scrollBar.arrow2_x,
			sbw->scrollBar.arrow2_y,
			sbw->scrollBar.arrow2_orientation);
	}

	if (sbw->scrollBar.timer != 0)
	{
		sbw->scrollBar.flags |= END_TIMER;
	}

	if (sbw->scrollBar.sliding_on == True)
	{
		sbw->scrollBar.sliding_on = False;
		ScrollCallback (sbw, XmCR_VALUE_CHANGED, sbw->scrollBar.value, 
			event->xbutton.x, event->xbutton.y, event);
	}

#ifdef FUNKY_INSENSITIVE_VISUAL
	if (! (sbw->scrollBar.flags & ARROW1_AVAILABLE))
    {
		XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.unavailable_GC,
			sbw->scrollBar.arrow1_x,
			sbw->scrollBar.arrow1_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height);
    }
    else if (! (sbw->scrollBar.flags & ARROW2_AVAILABLE))
    {
		XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.unavailable_GC,
			sbw->scrollBar.arrow2_x,
			sbw->scrollBar.arrow2_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height);
    }
#endif
}




/************************************************************************
 *
 *  Moved
 *     This function processes mouse moved events during interactive
 *     slider moves.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Moved( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Moved(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	XButtonPressedEvent * buttonEvent = (XButtonPressedEvent *) event;
	int newX, newY;
	int realX, realY;
	int slideVal;
	int button_x;
	int button_y;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;
	if (sbw->scrollBar.sliding_on != True)
		return;

	button_x = buttonEvent->x;
	button_y = buttonEvent->y;

	/*
	 * Force button_x and button_y to be within the slider_area.
	 */
	if (button_x < sbw->scrollBar.slider_area_x)
		button_x = sbw->scrollBar.slider_area_x;

	if (button_x > 
	    sbw->scrollBar.slider_area_x + sbw->scrollBar.slider_area_width)
	    button_x = sbw->scrollBar.slider_area_x 
		+ sbw->scrollBar.slider_area_width;

	if (button_y < sbw->scrollBar.slider_area_y)
		button_y = sbw->scrollBar.slider_area_y;

	if (button_y > 
		sbw->scrollBar.slider_area_y 
			+ sbw->scrollBar.slider_area_height)
		button_y = sbw->scrollBar.slider_area_y 
			+ sbw->scrollBar.slider_area_height;


	/*
	 * Calculate the new origin of the slider.  
	 * Bound the values with the slider area.
	 */
	if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
		newX = realX = button_x - sbw->scrollBar.separation_x;
		newY = realY = sbw->scrollBar.slider_y;

		if (newX < sbw->scrollBar.slider_area_x)
			newX = sbw->scrollBar.slider_area_x;

		if (newX + sbw->scrollBar.slider_width > 
			sbw->scrollBar.slider_area_x 
			+ sbw->scrollBar.slider_area_width)
			newX = sbw->scrollBar.slider_area_x
				+ sbw->scrollBar.slider_area_width
				- sbw->scrollBar.slider_width;
	}
	else
	{
		newX = realX = sbw->scrollBar.slider_x;
		newY = realY = button_y - sbw->scrollBar.separation_y;


		if (newY < sbw->scrollBar.slider_area_y)
			newY = sbw->scrollBar.slider_area_y;

		if (newY + sbw->scrollBar.slider_height > 
			sbw->scrollBar.slider_area_y 
			+ sbw->scrollBar.slider_area_height)
			newY = sbw->scrollBar.slider_area_y 
				+ sbw->scrollBar.slider_area_height
				- sbw->scrollBar.slider_height;
	}


	if (((sbw->scrollBar.orientation == XmHORIZONTAL) && 
		(realX != sbw->scrollBar.initial_x))
		||
		((sbw->scrollBar.orientation == XmVERTICAL)   &&
		(realY != sbw->scrollBar.initial_y)))
	{
		slideVal = CalcSliderVal (sbw, button_x, button_y);

		if ((newX != sbw->scrollBar.initial_x) || 
			(newY != sbw->scrollBar.initial_y))
		{
			MoveSlider (sbw, newX, newY);
			sbw->scrollBar.initial_x = newX;
			sbw->scrollBar.initial_y = newY;
		}

		if (slideVal != sbw->scrollBar.value)
		{
			sbw->scrollBar.value = slideVal;

			if (slideVal >= (sbw->scrollBar.maximum
					- sbw->scrollBar.slider_size))
				slideVal = sbw->scrollBar.maximum
					- sbw->scrollBar.slider_size;

			if (slideVal <= sbw->scrollBar.minimum)
				slideVal = sbw->scrollBar.minimum;

			ScrollCallback(sbw, XmCR_DRAG, 
				sbw->scrollBar.value = slideVal, 
                buttonEvent->x, buttonEvent->y,
                (XEvent *) buttonEvent);
		}
	}
}




/*********************************************************************
 *
 *  TopOrBottom
 *	Issue the to top or bottom callbacks.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
TopOrBottom( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TopOrBottom(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
	XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	XmScrollBarPart *sbp = (XmScrollBarPart *) &(sbw->scrollBar);


	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;
	if (event->type == KeyPress)
	{
		Modifiers junk;
		KeySym key_sym;
		XKeyPressedEvent *keyEvent = (XKeyPressedEvent *) event;

		/*
		XtTranslateKey(XtDisplay(sbw), keyEvent->keycode,
			keyEvent->state, &junk, &key_sym);
		*/

		key_sym = XtGetActionKeysym(event, &junk);
		
		if (key_sym == osfXK_BeginLine)
		{
			if (sbp->orientation == XmVERTICAL)
			{
				if (sbp->processing_direction == XmMAX_ON_BOTTOM)
					MoveSlider(sbw, sbp->slider_x, sbp->slider_area_y);
				else
					MoveSlider(sbw,
						sbp->slider_x, 
						sbp->slider_area_y + sbp->slider_area_height
							- sbp->slider_height);
			}
			else
			{
				if (sbp->processing_direction == XmMAX_ON_RIGHT)
					MoveSlider(sbw, sbp->slider_area_x, sbp->slider_y);
				else
					MoveSlider(sbw,
						sbp->slider_area_x + sbp->slider_area_width
							- sbp->slider_width,
						sbp->slider_y);
			}
			/*
			 * The following grevious bogosity is due to the fact
			 * that the key behavior was implemented long after 
			 * the rest of this code, and so we have to work around
			 * currently operating nonsense.
			 *
			 * Specifically, since the dawn of time, ScrollBar
			 * processes in just one direction, and does any necessary
			 * reversal just before calling the callback.
			 *
			 * We now proceed to trick that code into doing the right
			 * thing anyway
			 */
			if ((sbp->processing_direction == XmMAX_ON_BOTTOM) ||
				(sbp->processing_direction == XmMAX_ON_RIGHT))
			{
				sbp->value = sbp->minimum;
				ScrollCallback(sbw, XmCR_TO_TOP, sbp->value,
                                                      keyEvent->x, keyEvent->y,
                                                          (XEvent *) keyEvent);
			}
			else
			{
				sbp->value = sbp->maximum - sbp->slider_size;
				ScrollCallback(sbw, XmCR_TO_BOTTOM, sbp->value,
                                                      keyEvent->x, keyEvent->y,
                                                          (XEvent *) keyEvent);
			}
		}
		else /* key_sym == osfXK_EndLine */
		{
			if (sbp->orientation == XmVERTICAL)
			{
				if (sbp->processing_direction == XmMAX_ON_BOTTOM)
					MoveSlider(sbw,
						sbp->slider_x, 
						sbp->slider_area_y + sbp->slider_area_height
							- sbp->slider_height);
				else
					MoveSlider(sbw, sbp->slider_x, sbp->slider_area_y);
			}
			else
			{
				if (sbp->processing_direction == XmMAX_ON_RIGHT)
					MoveSlider(sbw,
						sbp->slider_area_x + sbp->slider_area_width
							- sbp->slider_width,
						sbp->slider_y);
				else
					MoveSlider(sbw, sbp->slider_area_x, sbp->slider_y);
			}

			/* See above for explanation of this nonsense */
			if ((sbp->processing_direction == XmMAX_ON_BOTTOM) ||
				(sbp->processing_direction == XmMAX_ON_RIGHT))
			{
				sbp->value = sbp->maximum - sbp->slider_size;
				ScrollCallback(sbw, XmCR_TO_BOTTOM, sbp->value,
                                                      keyEvent->x, keyEvent->y,
                                                          (XEvent *) keyEvent);
			}
			else
			{
				sbp->value = sbp->minimum;
				ScrollCallback (sbw, XmCR_TO_TOP, sbp->value,
                                                      keyEvent->x, keyEvent->y,
                                                          (XEvent *) keyEvent);
			}
		}
	}
	else  /* event->type == ButtonPress */
	{
		XButtonPressedEvent *buttonEvent =
			(XButtonPressedEvent *) event;

		/* add a start update when the button is pressed
		   The endupdate is done in Release */
	       
		XmDropSiteStartUpdate(wid);

   		if /* In arrow1... */
			((buttonEvent->x >= sbp->arrow1_x)                   &&
			(buttonEvent->y >= sbp->arrow1_y)                    &&
			(buttonEvent->x <= sbp->arrow1_x + sbp->arrow_width) &&
			(buttonEvent->y <= sbp->arrow1_y + sbp->arrow_height))
		{
			sbp->change_type = XmCR_DECREMENT;
			sbp->arrow1_selected = True;

			DRAWARROW(sbw, sbw->primitive.bottom_shadow_GC,
				  sbw -> primitive.top_shadow_GC,
				  sbw->scrollBar.arrow1_x,
				  sbw->scrollBar.arrow1_y,
				  sbw->scrollBar.arrow1_orientation);
			if (sbp->orientation == XmVERTICAL)
				MoveSlider(sbw, sbp->slider_x, sbp->slider_area_y);
			else
				MoveSlider(sbw, sbp->slider_area_x, sbp->slider_y);
			sbp->value = sbp->minimum;
			ScrollCallback (sbw, XmCR_TO_TOP, sbp->value,
                                                buttonEvent->x, buttonEvent->y,
                                                       (XEvent *) buttonEvent);
		}

		else if /* In arrow2... */
			((buttonEvent->x >= sbp->arrow2_x)  &&
			(buttonEvent->y >= sbp->arrow2_y)   &&
			(buttonEvent->x <= sbp->arrow2_x 
				+ sbp->arrow_width)             &&
			(buttonEvent->y <= sbp->arrow2_y 
				+ sbp->arrow_height))
		{
			sbp->change_type = XmCR_INCREMENT;
			sbp->arrow2_selected = True;

			DRAWARROW (sbw, sbw->primitive.bottom_shadow_GC,
				   sbw -> primitive.top_shadow_GC,
				  sbw->scrollBar.arrow2_x,
				  sbw->scrollBar.arrow2_y,
				  sbw->scrollBar.arrow2_orientation);
			if (sbp->orientation == XmVERTICAL)
				MoveSlider(sbw,
					sbp->slider_x, 
					sbp->slider_area_y + sbp->slider_area_height
						- sbp->slider_height);
			else
				MoveSlider(sbw,
					sbp->slider_area_x + sbp->slider_area_width
						- sbp->slider_width,
					sbp->slider_y);
			sbp->value = sbp->maximum - sbp->slider_size;
			ScrollCallback (sbw, XmCR_TO_BOTTOM, 
				sbp->value, buttonEvent->x, buttonEvent->y,
                                                       (XEvent *) buttonEvent);
		} 
		else if /* in the trough between arrow1 and the slider... */
			(((sbp->orientation == XmHORIZONTAL)       &&
				(buttonEvent->x >= sbp->slider_area_x) &&
				(buttonEvent->x < sbp->slider_x)       &&
				(buttonEvent->y >= sbp->slider_area_y) &&
				(buttonEvent->y <= sbp->slider_area_y
					+ sbp->slider_area_height))
			||
			((sbp->orientation == XmVERTICAL) &&
				(buttonEvent->y >= sbp->slider_area_y)  &&
				(buttonEvent->y < sbp->slider_y)        &&
				(buttonEvent->x >= sbp->slider_area_x)  &&
				(buttonEvent->x < sbp->slider_area_x
					+ sbp->slider_area_width)))
		{
			if (sbp->orientation == XmVERTICAL)
				MoveSlider(sbw, sbp->slider_x, sbp->slider_area_y);
			else
				MoveSlider(sbw, sbp->slider_area_x, sbp->slider_y);
			sbp->value = sbp->minimum;
			ScrollCallback (sbw, XmCR_TO_TOP, sbp->value,
                                                buttonEvent->x, buttonEvent->y,
                                                       (XEvent *) buttonEvent);
		}
		else if /* in the trough between arrow2 and the slider... */
			(((sbp->orientation == XmHORIZONTAL)                     &&
				(buttonEvent->x > sbp->slider_x + sbp->slider_width) &&
				(buttonEvent->x <= sbp->slider_area_x
					+ sbp->slider_area_width)                        &&
				(buttonEvent->y >= sbp->slider_area_y)               &&
				(buttonEvent->y <= sbp->slider_area_y
					+ sbp->slider_area_height))
			||
				((sbp->orientation == XmVERTICAL)           &&
					(buttonEvent->y > sbp->slider_y 
						+ sbp->slider_height)               &&
					(buttonEvent->y <= sbp->slider_area_y
						+ sbp->slider_area_height)          &&
					(buttonEvent->x >= sbp->slider_area_x)  &&
					(buttonEvent->x <= sbp->slider_area_x 
						+ sbp->slider_area_width)))
		{
			if (sbp->orientation == XmVERTICAL)
				MoveSlider(sbw,
					sbp->slider_x, 
					sbp->slider_area_y + sbp->slider_area_height
						- sbp->slider_height);
			else
				MoveSlider(sbw,
					sbp->slider_area_x + sbp->slider_area_width
						- sbp->slider_width,
					sbp->slider_y);
			sbp->value = sbp->maximum - sbp->slider_size;
			ScrollCallback (sbw, XmCR_TO_BOTTOM, 
				sbp->value, buttonEvent->x, buttonEvent->y,
                                                       (XEvent *) buttonEvent);
		}
	}
#ifdef FUNKY_INSENSITIVE_VISUAL
	if (sbp->value == sbp->minimum)
	{
		XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.unavailable_GC,
			sbw->scrollBar.arrow1_x,
			sbw->scrollBar.arrow1_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height);

        sbw->scrollBar.flags &= ~ARROW1_AVAILABLE;

		if (! (sbw->scrollBar.flags & ARROW2_AVAILABLE))
		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow2_orientation);

			sbw->scrollBar.flags |= ARROW2_AVAILABLE;
		}
	}
	else /* sbp->value == (sbp->maximum - sbp->slider_size) */
	{
/*		XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
			sbw->scrollBar.unavailable_GC,
			sbw->scrollBar.arrow2_x,
			sbw->scrollBar.arrow2_y,
			sbw->scrollBar.arrow_width,
			sbw->scrollBar.arrow_height);
*/
        sbw->scrollBar.flags &= ~ARROW2_AVAILABLE;

		if (! (sbw->scrollBar.flags & ARROW1_AVAILABLE))
		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow1_orientation);

			sbw->scrollBar.flags |= ARROW1_AVAILABLE;
		}
	}
#endif
}




/*********************************************************************
 *
 *  IncrementUpOrLeft
 *	The up or left key was pressed, decrease the value by 
 *	one increment.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
IncrementUpOrLeft( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
IncrementUpOrLeft(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;

	int new_value;
	int key_pressed;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;
	/*
	 * arg value passed in will either be 0 -> up key or 1 -> left key 
	 * the key needs to be compared with the scrollbar orientation to
	 * ensure only the proper directional key presses work.
	 */

	key_pressed = atoi(*params);

	if (((key_pressed == 0) && 
		(sbw->scrollBar.orientation == XmHORIZONTAL)) 
		||
		((key_pressed == 1) && 
		(sbw->scrollBar.orientation == XmVERTICAL)))
		return;

	new_value = sbw->scrollBar.value - sbw->scrollBar.increment;

	if (new_value < sbw->scrollBar.minimum)
		new_value = sbw->scrollBar.minimum;

	if (new_value != sbw->scrollBar.value)
	{
		sbw->scrollBar.value = new_value;
#ifdef FUNKY_INSENSITIVE_VISUAL
		if ((sbw->scrollBar.value = new_value)
			== sbw->scrollBar.minimum)
		{
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);

            sbw->scrollBar.flags &= ~ARROW1_AVAILABLE;
		}
#endif
		if ((sbw->scrollBar.show_arrows) &&
		    (! (sbw->scrollBar.flags & ARROW2_AVAILABLE)))
		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow2_orientation);

			sbw->scrollBar.flags |= ARROW2_AVAILABLE;
		}

		RedrawSliderWindow (sbw);

		ScrollCallback (sbw, XmCR_DECREMENT, sbw->scrollBar.value,
                        event->xbutton.x, event->xbutton.y, event);
	}
}




/*********************************************************************
 *
 *  IncrementDownOrRight
 *	The down or right key was pressed, increase the value by 
 *	one increment.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
IncrementDownOrRight( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
IncrementDownOrRight(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	int new_value;
	int key_pressed;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;
	/* 
 	 * arg value passed in will either be 0 -> down key or 1 -> right 
	 * key the key needs to be compared with the scrollbar orientation 
	 * to ensure only the proper directional key presses work.
	 */

	key_pressed = atoi(*params);

	if (((key_pressed == 0) && 
		(sbw->scrollBar.orientation == XmHORIZONTAL))
		||
		((key_pressed == 1) && 
		(sbw->scrollBar.orientation == XmVERTICAL)))
		return;

	new_value = sbw->scrollBar.value + sbw->scrollBar.increment;

	if (new_value > sbw->scrollBar.maximum - sbw->scrollBar.slider_size)
		new_value = sbw->scrollBar.maximum - sbw->scrollBar.slider_size;

	if (new_value != sbw->scrollBar.value)
	{
		sbw->scrollBar.value = new_value;
#ifdef FUNKY_INSENSITIVE_VISUAL
		if ((sbw->scrollBar.value = new_value)
			== (sbw->scrollBar.maximum - sbw->scrollBar.slider_size))
		{
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);

            sbw->scrollBar.flags &= ~ARROW2_AVAILABLE;
		}
#endif
		if ((sbw->scrollBar.show_arrows) &&
		    (! (sbw->scrollBar.flags & ARROW1_AVAILABLE)))

		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow1_orientation);

			sbw->scrollBar.flags |= ARROW1_AVAILABLE;
		}

		RedrawSliderWindow (sbw);

		ScrollCallback (sbw, XmCR_INCREMENT, sbw->scrollBar.value,
                                    event->xbutton.x, event->xbutton.y, event);
	}
}




/*********************************************************************
 *
 *  PageUpOrLeft
 *	The up or left key was pressed, decrease the value by 
 *	one increment.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
PageUpOrLeft( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageUpOrLeft(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	int new_value;
	int key_pressed;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;
	/*
	 * arg value passed in will either be 0 -> up key or 1 -> left key 
	 * the key needs to be compared with the scrollbar orientation to
	 * ensure only the proper directional key presses work.
	 */

	key_pressed = atoi(*params);

	if (((key_pressed == 0) && 
		(sbw->scrollBar.orientation == XmHORIZONTAL)) 
		||
		((key_pressed == 1) && 
		(sbw->scrollBar.orientation == XmVERTICAL)))
		return;

	new_value = sbw->scrollBar.value - sbw->scrollBar.page_increment;

	if (new_value < sbw->scrollBar.minimum)
		new_value = sbw->scrollBar.minimum;

	if (new_value != sbw->scrollBar.value)
	{
		sbw->scrollBar.value = new_value;
#ifdef FUNKY_INSENSITIVE_VISUAL
		if ((sbw->scrollBar.value = new_value)
			== sbw->scrollBar.minimum)
		{
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);

            sbw->scrollBar.flags &= ~ARROW1_AVAILABLE;
		}
#endif
               if ((sbw->scrollBar.show_arrows) &&
                   (! (sbw->scrollBar.flags & ARROW2_AVAILABLE)))
		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow2_orientation);

			sbw->scrollBar.flags |= ARROW2_AVAILABLE;
		}

		RedrawSliderWindow (sbw);

		ScrollCallback (sbw, XmCR_PAGE_DECREMENT, 
				sbw->scrollBar.value,
				event->xbutton.x, event->xbutton.y, event);
	}
}




/*********************************************************************
 *
 *  PageDownOrRight
 *	The down or right key was pressed, increase the value by 
 *	one increment.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
PageDownOrRight( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageDownOrRight(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	int new_value;
	int key_pressed;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return;

	/* 
 	 * arg value passed in will either be 0 -> down key or 1 -> right 
	 * key the key needs to be compared with the scrollbar orientation 
	 * to ensure only the proper directional key presses work.
	 */

	key_pressed = atoi(*params);

	if (((key_pressed == 0) && 
		(sbw->scrollBar.orientation == XmHORIZONTAL))
		||
		((key_pressed == 1) && 
		(sbw->scrollBar.orientation == XmVERTICAL)))
		return;

	new_value = sbw->scrollBar.value + sbw->scrollBar.page_increment;

	if (new_value > sbw->scrollBar.maximum - sbw->scrollBar.slider_size)
		new_value = sbw->scrollBar.maximum - sbw->scrollBar.slider_size;

	if (new_value != sbw->scrollBar.value)
	{
		sbw->scrollBar.value = new_value;
#ifdef FUNKY_INSENSITIVE_VISUAL
		if ((sbw->scrollBar.value = new_value)
			== (sbw->scrollBar.maximum - sbw->scrollBar.slider_size))
		{
			XFillRectangle(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.unavailable_GC,
				sbw->scrollBar.arrow2_x,
				sbw->scrollBar.arrow2_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height);

            sbw->scrollBar.flags &= ~ARROW2_AVAILABLE;
		}
#endif
               if ((sbw->scrollBar.show_arrows) &&
                   (! (sbw->scrollBar.flags & ARROW1_AVAILABLE)))
		{
			XClearArea(XtDisplay(sbw), XtWindow(sbw),
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow_width,
				sbw->scrollBar.arrow_height,
				FALSE);
			
			DRAWARROW (sbw, sbw -> primitive.top_shadow_GC,
				sbw->primitive.bottom_shadow_GC,
				sbw->scrollBar.arrow1_x,
				sbw->scrollBar.arrow1_y,
				sbw->scrollBar.arrow1_orientation);

			sbw->scrollBar.flags |= ARROW1_AVAILABLE;
		}

		RedrawSliderWindow (sbw);

		ScrollCallback (sbw, XmCR_PAGE_INCREMENT, sbw->scrollBar.value,
                                    event->xbutton.x, event->xbutton.y, event);
	}
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
CancelDrag( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CancelDrag(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
	XmScrollBarWidget sbw = (XmScrollBarWidget) wid;


	if (sbw->scrollBar.flags & KEYBOARD_GRABBED)
	{
		short savedX, savedY, j1, j2;

		XtUngrabKeyboard(wid, ((XButtonPressedEvent *)event)->time);
		sbw->scrollBar.flags &= ~KEYBOARD_GRABBED;

		sbw->scrollBar.sliding_on = False;
		sbw->scrollBar.value = sbw->scrollBar.saved_value;
		CalcSliderRect(sbw, &savedX, &savedY, &j1, &j2);
		MoveSlider(sbw, savedX, savedY);
		ScrollCallback (sbw, XmCR_VALUE_CHANGED,
		                sbw->scrollBar.value, savedX, savedY, (XEvent *) event);

		if (sbw->scrollBar.timer != 0)
		{
			sbw->scrollBar.flags |= END_TIMER;
		}

	}
	else
	{
		XmParentInputActionRec pp_data ;

		pp_data.process_type = XmINPUT_ACTION ;
		pp_data.action = XmPARENT_CANCEL ;
		pp_data.event = event ;
		pp_data.params = params ;
		pp_data.num_params = num_params ;

		_XmParentProcess( XtParent( wid), (XmParentProcessData) &pp_data) ;
	}
}


/*********************************************************************
 *
 *  MoveSlider
 *	Given x and y positions, move the slider and clear the area
 *	moved out of.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
MoveSlider( sbw, currentX, currentY )
        XmScrollBarWidget sbw ;
        int currentX ;
        int currentY ;
#else
MoveSlider(
        XmScrollBarWidget sbw,
        int currentX,
        int currentY )
#endif /* _NO_PROTO */
{
    int oldX = sbw->scrollBar.slider_x;
    int oldY = sbw->scrollBar.slider_y;
    int width = sbw->scrollBar.slider_width;
    int height = sbw->scrollBar.slider_height;
    
    XSegment seg[2];
    
    
    if ((currentX == oldX) && (currentY == oldY))
	return;
    
    if (sbw->scrollBar.orientation == XmHORIZONTAL)
	{
	    sbw->scrollBar.slider_x = currentX;
	    
	    seg[0].y1 = seg[0].y2 = oldY + 2;
	    seg[1].y1 = seg[1].y2 = oldY + height - 3;
	    
	    if (oldX < currentX)
		{
		    seg[0].x1 = seg[1].x1 = oldX;
		    seg[0].x2 = seg[1].x2 = oldX + currentX - oldX - 1;
		}
	    else
		{
		    seg[0].x1 = seg[1].x1 = currentX + width;
		    seg[0].x2 = seg[1].x2 = seg[0].x1 + oldX - currentX - 1;
		}
	    
	    
	    if (sbw->scrollBar.pixmap != 0)
		{
		    /* somehow similar to RedrawSliderWindow */

		    XCopyArea (XtDisplay((Widget) sbw),
			       sbw->scrollBar.pixmap, XtWindow((Widget) sbw),
			       sbw->scrollBar.foreground_GC,
			       0, 0, width, height, currentX, currentY);
		    
		    XClearArea (XtDisplay((Widget)sbw), XtWindow((Widget)sbw),
				seg[0].x1, oldY, seg[0].x2 - seg[0].x1 + 1, 
				height, False);
		}
	} 
    else /* sbw->scrollBar.orientation == XmVERTICAL */
	{
	    sbw->scrollBar.slider_y = currentY;
	    
	    seg[0].x1 = seg[0].x2 = oldX + 2;
	    seg[1].x1 = seg[1].x2 = oldX + width - 3;
	    
	    if (oldY < currentY)
		{
		    seg[0].y1 = seg[1].y1 = oldY;
		    seg[0].y2 = seg[1].y2 = oldY + currentY - oldY - 1;
		}
	    else
		{
		    seg[0].y1 = seg[1].y1 = currentY + height;
		    seg[0].y2 = seg[1].y2 = seg[0].y1 + oldY - currentY - 1;
		}
	    
	    if (sbw->scrollBar.pixmap != 0)
		{
		    /* somehow similar to RedrawSliderWindow */

		    XCopyArea (XtDisplay ((Widget) sbw),
			       sbw->scrollBar.pixmap, XtWindow ((Widget) sbw),
			       sbw->scrollBar.foreground_GC,
			       0, 0, width, height, currentX, currentY);
		    
		    XClearArea (XtDisplay((Widget) sbw), XtWindow((Widget) sbw),
				oldX, seg[0].y1, width,
				seg[0].y2 - seg[0].y1 + 1, False);
		}
	}
}




/************************************************************************
 *
 *  ChangeScrollBarValue
 *	Change the scrollbar value by the indicated change type.  Return
 *	True if the value changes, False otherwise.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
ChangeScrollBarValue( sbw )
        XmScrollBarWidget sbw ;
#else
ChangeScrollBarValue(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
	register unsigned char change_type = sbw->scrollBar.change_type;
	register int change_amount = 0;
	register Boolean returnFlag = TRUE;
	register int old_value = sbw->scrollBar.value;

	if (! (sbw->scrollBar.flags & SLIDER_AVAILABLE))
		return(FALSE);
	/*  Get the amount to change the scroll bar value based on  */
	/*  the type of change occuring.                            */

	if (change_type == XmCR_INCREMENT)
		change_amount = sbw->scrollBar.increment;
	else if (change_type == XmCR_PAGE_INCREMENT)
		change_amount = sbw->scrollBar.page_increment;
	else if (change_type == XmCR_DECREMENT)
		change_amount = -sbw->scrollBar.increment;
	else if (change_type == XmCR_PAGE_DECREMENT)
		change_amount = -sbw->scrollBar.page_increment;

	/* Change the value */
	sbw->scrollBar.value += change_amount;

	/* Truncate and set flags as appropriate */
	if (sbw->scrollBar.value >= (sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size))
		sbw->scrollBar.value = sbw->scrollBar.maximum
			- sbw->scrollBar.slider_size;

	if (sbw->scrollBar.value <= sbw->scrollBar.minimum)
		sbw->scrollBar.value = sbw->scrollBar.minimum;
	
	if ((returnFlag = (sbw->scrollBar.value != old_value)) != False) {
		RedrawSliderWindow (sbw);
	    }
		

	return (returnFlag);
}




/*********************************************************************
 *
 *  TimerEvent
 *	This is an event processing function which handles timer
 *	event evoked because of arrow selection.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
TimerEvent( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
TimerEvent(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) closure ;
	Boolean flag;

	sbw->scrollBar.timer = 0;

	if (sbw->scrollBar.flags & END_TIMER)
	{
		sbw->scrollBar.flags &= ~END_TIMER;
		return;
	}

	if (sbw->scrollBar.flags & FIRST_SCROLL_FLAG)
	{
		XSync (XtDisplay (sbw), False);

		sbw->scrollBar.flags &= ~FIRST_SCROLL_FLAG;

		sbw->scrollBar.timer = 
		XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) sbw),
                                   (unsigned long) sbw->scrollBar.repeat_delay,
                                                  TimerEvent, (XtPointer) sbw);
		return;
	}


	/*  Change the scrollbar slider value  */

	flag = ChangeScrollBarValue (sbw);

	/*  If the orgin was changed invoke the application supplied  */
	/*  slider moved callbacks                                    */

	if (flag)
		ScrollCallback (sbw, sbw->scrollBar.change_type, 
			sbw->scrollBar.value, 0, 0, NULL);

	/*
	 * If the callback does alot of processing, and XSync is needed
	 * to flush the output and input buffers.  If this is not done,
	 * the entry back to MainLoop will cause the flush.  The server
	 * will then perform it work which may take longer than the timer
	 * interval which will cause the scrollbar to be stuck in a loop.
	 */

	XSync (XtDisplay (sbw), False);

	/*  Add the repeat timer and check that the scrollbar hasn't been set 
	    insensitive by some callbacks */

	if (flag)
	{
		sbw->scrollBar.timer = 
		    XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) sbw),
				     (unsigned long) sbw->scrollBar.repeat_delay,
				     TimerEvent, (XtPointer) sbw);
	}
}




/************************************************************************
 *
 *  ScrollCallback
 *	This routine services the widget's callbacks.  It calls the
 *	specific callback if it is not empty.  If it is empty then the 
 *	main callback is called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ScrollCallback( sbw, reason, value, xpixel, ypixel, event )
        XmScrollBarWidget sbw ;
        int reason ;
        int value ;
        int xpixel ;
        int ypixel ;
        XEvent *event ;
#else
ScrollCallback(
        XmScrollBarWidget sbw,
        int reason,
        int value,
        int xpixel,
        int ypixel,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmScrollBarCallbackStruct call_value;

   call_value.reason = reason;
   call_value.event  = event;
    
   if (sbw->scrollBar.processing_direction == XmMAX_ON_LEFT ||
       sbw->scrollBar.processing_direction == XmMAX_ON_TOP)
	{
		switch (reason)
		{
			case XmCR_INCREMENT:
				call_value.reason = reason = XmCR_DECREMENT;
			break;
			case XmCR_DECREMENT:
				call_value.reason = reason = XmCR_INCREMENT;
			break;
			case XmCR_PAGE_INCREMENT:
				call_value.reason = reason = XmCR_PAGE_DECREMENT;
			break;
			case XmCR_PAGE_DECREMENT:
				call_value.reason = reason = XmCR_PAGE_INCREMENT;
			break;
			case XmCR_TO_TOP:
				call_value.reason = reason = XmCR_TO_BOTTOM;
			break;
			case XmCR_TO_BOTTOM:
				call_value.reason = reason = XmCR_TO_TOP;
			break;
		}
      call_value.value = sbw->scrollBar.maximum
		+ sbw->scrollBar.minimum - value - sbw->scrollBar.slider_size;
	}
   else
      call_value.value = value;


   if (sbw->scrollBar.orientation == XmHORIZONTAL)
      call_value.pixel = xpixel;
   else
      call_value.pixel = ypixel;

   switch (reason)
   {

      case XmCR_VALUE_CHANGED:
         XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback,
                                &call_value);
      break;

      case XmCR_INCREMENT:
         if  (sbw->scrollBar.increment_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.increment_callback, &call_value);
         else
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_DECREMENT:
         if  (sbw->scrollBar.decrement_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.decrement_callback, &call_value);
         else
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_PAGE_INCREMENT:
         if  (sbw->scrollBar.page_increment_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.page_increment_callback, &call_value);
         else 
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_PAGE_DECREMENT:
         if  (sbw->scrollBar.page_decrement_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.page_decrement_callback, &call_value);
         else
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_TO_TOP:
         if (sbw->scrollBar.to_top_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.to_top_callback, &call_value);
         else 
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_TO_BOTTOM:
         if (sbw->scrollBar.to_bottom_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.to_bottom_callback, &call_value);
         else
         {
            call_value.reason = XmCR_VALUE_CHANGED;
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.value_changed_callback, &call_value);
         }
      break;

      case XmCR_DRAG:
         if (sbw->scrollBar.drag_callback)
            XtCallCallbackList ((Widget) sbw, sbw->scrollBar.drag_callback, &call_value);
      break;
   }
}


#ifdef CDE_VISUAL  /* sliding_mode */
static char _XmThermometer_Translations[] = "\
~s ~c ~m ~a <Btn1Down>:\n\
~s ~c ~m ~a <Btn1Up>:\n\
~s ~c ~m ~a Button1<PtrMoved>:\n\
~s ~c ~m ~a <Btn2Down>:\n\
~s ~c ~m ~a <Btn2Up>:\n\
~s ~c ~m ~a Button2<PtrMoved>:\n\
~s c ~m ~a <Btn1Down>:\n\
~s c ~m ~a <Btn1Up>:\n\
<Key>osfBeginLine:\n\
<Key>osfEndLine:\n\
<Key>osfPageLeft:\n\
c <Key>osfPageUp:\n\
<Key>osfPageUp:\n\
<Key>osfPageRight:\n\
c <Key>osfPageDown:\n\
<Key>osfPageDown:\n\
~s ~c <Key>osfUp:\n\
~s ~c <Key>osfDown:\n\
~s ~c <Key>osfLeft:\n\
~s ~c <Key>osfRight:\n\
~s c <Key>osfUp:\n\
~s c <Key>osfDown:\n\
~s c <Key>osfLeft:\n\
~s c <Key>osfRight:";

static XtTranslations thermometer_xlatns = NULL;

void 
#ifdef _NO_PROTO
_CDESetScrollBarVisual( sbw, is_thermometer )
        XmScrollBarWidget sbw ;
        Boolean is_thermometer;
#else
_CDESetScrollBarVisual(
        XmScrollBarWidget sbw,
        Boolean is_thermometer)
#endif /* _NO_PROTO */
{
   if (is_thermometer) {
       sbw->scrollBar.etched_slider = THERMOMETER_ON;
       if (thermometer_xlatns == NULL) 
           thermometer_xlatns =
               XtParseTranslationTable(_XmThermometer_Translations);
       XtOverrideTranslations((Widget)sbw, thermometer_xlatns);
   }
   else
       sbw->scrollBar.etched_slider = True;
}
#endif /* CDE_VISUAL */


/************************************************************************
 *
 *  _XmSetEtchedSlider
 *	Set the scrollbar variable which causes the slider pixmap
 *	to be etched.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmSetEtchedSlider( sbw )
        XmScrollBarWidget sbw ;
#else
_XmSetEtchedSlider(
        XmScrollBarWidget sbw )
#endif /* _NO_PROTO */
{
   sbw->scrollBar.etched_slider = True;
}




/************************************************************************
 *
 *		Application Accessible External Functions
 *
 ************************************************************************/


/************************************************************************
 *
 *  XmCreateScrollBar
 *	Create an instance of a scrollbar and return the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateScrollBar( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateScrollBar(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmScrollBarWidgetClass, 
                           parent, arglist, argcount));
}




/************************************************************************
 *
 *  XmScrollBarGetValues
 *	Return some scrollbar values.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmScrollBarGetValues( w, value, slider_size, increment, page_increment )
        Widget w ;
        int *value ;
        int *slider_size ;
        int *increment ;
        int *page_increment ;
#else
XmScrollBarGetValues(
        Widget w,
        int *value,
        int *slider_size,
        int *increment,
        int *page_increment )
#endif /* _NO_PROTO */
{
   XmScrollBarWidget sbw = (XmScrollBarWidget) w;

   if (sbw->scrollBar.processing_direction == XmMAX_ON_LEFT ||
       sbw->scrollBar.processing_direction == XmMAX_ON_TOP) {
      if(value) *value = sbw->scrollBar.maximum  + sbw->scrollBar.minimum 
	  	- sbw->scrollBar.value - sbw->scrollBar.slider_size;
    }
   else
      if(value) *value = sbw->scrollBar.value;

   if(slider_size) *slider_size = sbw->scrollBar.slider_size;
   if(increment) *increment = sbw->scrollBar.increment;
   if(page_increment) *page_increment = sbw->scrollBar.page_increment;
}




/************************************************************************
 *
 *  XmScrollBarSetValues
 *	Set some scrollbar values.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmScrollBarSetValues( w, value, slider_size, increment, page_increment, notify )
        Widget w ;
        int value ;
        int slider_size ;
        int increment ;
        int page_increment ;
        Boolean notify ;
#else
XmScrollBarSetValues(
        Widget w,
        int value,
        int slider_size,
        int increment,
        int page_increment,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmScrollBarWidget sbw = (XmScrollBarWidget) w;
   int save_value;
   Arg arglist[4];
   int n;


   save_value = sbw->scrollBar.value;

   n = 0;
   XtSetArg (arglist[n], XmNvalue, value);			n++;

   if (slider_size != 0) {
       XtSetArg (arglist[n], XmNsliderSize, slider_size);	n++;
   }
   if (increment != 0) {
      XtSetArg (arglist[n], XmNincrement, increment);		n++;
   }
   if (page_increment != 0) {
      XtSetArg (arglist[n], XmNpageIncrement, page_increment);	n++;
   }

   XtSetValues ((Widget) sbw, arglist, n);

   if (notify && sbw->scrollBar.value != save_value)
      ScrollCallback (sbw, XmCR_VALUE_CHANGED, 
                      sbw->scrollBar.value, 0, 0, NULL); 
}
