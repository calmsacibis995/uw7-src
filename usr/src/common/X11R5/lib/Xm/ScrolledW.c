#pragma ident	"@(#)m1.2libs:Xm/ScrolledW.c	1.4"
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
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include <Xm/ScrollBarP.h>
#include <Xm/DrawingAP.h>
#include <Xm/ScrolledWP.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/VirtKeysP.h>
#include <Xm/DrawP.h>

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define SB_BORDER_WIDTH 0
#define ScrollBarVisible( wid)      (wid && XtIsManaged( wid))

#define ScrolledWindowXlations	_XmScrolledW_ScrolledWindowXlations
#define ClipWindowTranslationTable _XmScrolledW_ClipWindowTranslationTable

#ifdef I18N_MSG
#define SWMessage6	catgets(Xm_catd,MS_SWindow,MSG_SW_6,_XmMsgScrolledW_0004)
#define SWMessage7	catgets(Xm_catd,MS_SWindow,MSG_SW_7,_XmMsgScrolledW_0005)
#define SWMessage8	catgets(Xm_catd,MS_SWindow,MSG_SW_8,_XmMsgScrolledW_0006)
#define SWMessage9	catgets(Xm_catd,MS_SWindow,MSG_SW_9,_XmMsgScrolledW_0007)
#define SWMessage10	catgets(Xm_catd,MS_SWindow,MSG_SW_10,_XmMsgScrolledW_0008)
#define SWMessage11	catgets(Xm_catd,MS_SWindow,MSG_SW_11,_XmMsgScrolledW_0009)
#define SVMessage1      catgets(Xm_catd,MS_SWindow,MSG_SW_12,_XmMsgScrollVis_0000)
#else
#define SWMessage6	_XmMsgScrolledW_0004
#define SWMessage7	_XmMsgScrolledW_0005
#define SWMessage8	_XmMsgScrolledW_0006
#define SWMessage9	_XmMsgScrolledW_0007
#define SWMessage10	_XmMsgScrolledW_0008
#define SWMessage11	_XmMsgScrolledW_0009
#define SVMessage1      _XmMsgScrollVis_0000 
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void VertSliderMove() ;
static void HorizSliderMove() ;
static void KidKilled() ;
static void LeftEdge() ;
static void RightEdge() ;
static void TopEdge() ;
static void BottomEdge() ;
static void PageLeft() ;
static void PageRight() ;
static void PageUp() ;
static void PageDown() ;
static void LeftEdgeGrabbed() ;
static void RightEdgeGrabbed() ;
static void TopEdgeGrabbed() ;
static void BottomEdgeGrabbed() ;
static void PageLeftGrabbed() ;
static void PageRightGrabbed() ;
static void PageUpGrabbed() ;
static void PageDownGrabbed() ;
static void CWMapNotify() ;
static void Noop();

static void ClassPartInitialize() ;
static void Initialize() ;
static void Realize() ;
static void Redisplay() ;
static void ClearBorder() ;
static void InsertChild() ;
static void VariableLayout() ;
static void ConstantLayout() ;
static void Resize() ;
static void SetBoxSize() ;
static XtGeometryResult GeometryManager() ;
static void ChangeManaged() ;
static XtGeometryResult QueryProc() ;
static Boolean SetValues() ;
static void CallProcessTraversal() ;

#else

static void VertSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer cd) ;
static void HorizSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer cd) ;
static void KidKilled( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void LeftEdge( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RightEdge( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TopEdge( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BottomEdge( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageLeft( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageRight( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageUp( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageDown( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void LeftEdgeGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RightEdgeGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TopEdgeGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BottomEdgeGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageLeftGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageRightGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageUpGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PageDownGrabbed( 
                        Widget sw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CWMapNotify( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Noop( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Realize( 
                        Widget wid,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void ClearBorder( 
                        XmScrolledWindowWidget sw) ;
static void InsertChild( 
                        Widget w) ;
static void VariableLayout( 
                        XmScrolledWindowWidget sw) ;
static void ConstantLayout( 
                        XmScrolledWindowWidget sw) ;
static void Resize( 
                        Widget wid) ;
static void SetBoxSize( 
                        XmScrolledWindowWidget sw) ;
static XtGeometryResult GeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void ChangeManaged( 
                        Widget wid) ;
static XtGeometryResult QueryProc( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *ret) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void CallProcessTraversal( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static Arg vSBArgs[36];
static Arg hSBArgs[36];
static Arg cwArgs[20];
static Boolean InitCW = TRUE;



/************************************************************************
 *									*
 * Scrolled Window Resources						*
 *									*
 ************************************************************************/

static XtResource resources[] = 
{
	  
    {
	XmNhorizontalScrollBar, XmCHorizontalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.hScrollBar),
	XmRImmediate, NULL
    },
    {
	XmNverticalScrollBar, XmCVerticalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.vScrollBar),
	XmRImmediate, NULL
    },
    {
	XmNworkWindow, XmCWorkWindow, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.WorkWindow),
	XmRImmediate, NULL
    },
    {
	XmNclipWindow, XmCClipWindow, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.ClipWindow),
	XmRImmediate, NULL
    },
    {
        XmNscrollingPolicy, XmCScrollingPolicy, XmRScrollingPolicy, 
	sizeof(unsigned char),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.ScrollPolicy),
        XmRImmediate, (XtPointer)  XmAPPLICATION_DEFINED
    },   
    {
        XmNvisualPolicy, XmCVisualPolicy, XmRVisualPolicy, sizeof (unsigned char),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.VisualPolicy),
        XmRImmediate,  (XtPointer) XmVARIABLE
    },   
    {
        XmNscrollBarDisplayPolicy, XmCScrollBarDisplayPolicy, 
	XmRScrollBarDisplayPolicy, sizeof (char),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.ScrollBarPolicy),
        XmRImmediate,  (XtPointer) (255)
    },   
    {
        XmNscrollBarPlacement, XmCScrollBarPlacement, XmRScrollBarPlacement, 
	sizeof (unsigned char),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.Placement),
        XmRImmediate,  (XtPointer) XmBOTTOM_RIGHT
    },

    {
        XmNscrolledWindowMarginWidth, XmCScrolledWindowMarginWidth,
        XmRHorizontalDimension, sizeof (Dimension), 
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.WidthPad), 
	XmRImmediate, (XtPointer) 0
    },
    {   
        XmNscrolledWindowMarginHeight, XmCScrolledWindowMarginHeight, 
        XmRVerticalDimension, sizeof (Dimension), 
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.HeightPad), 
	XmRImmediate, (XtPointer) 0
    },

    {
        XmNspacing, XmCSpacing, XmRHorizontalDimension, sizeof (Dimension),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.pad),
        XmRImmediate,  (XtPointer) XmINVALID_DIMENSION
    },

    {
        XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, 
        sizeof (Dimension),
        XtOffsetOf( struct _XmScrolledWindowRec, manager.shadow_thickness),
        XmRImmediate,  (XtPointer) XmINVALID_DIMENSION
    },
    {
        XmNtraverseObscuredCallback, XmCCallback, XmRCallback,
        sizeof(XtCallbackList),
        XtOffsetOf( struct _XmScrolledWindowRec, swindow.traverseObscuredCallback),
        XmRImmediate, NULL
    }
};
/****************
 *
 * Resolution independent resources
 *
 ****************/

static XmSyntheticResource get_resources[] =
{
   { XmNscrolledWindowMarginWidth, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmScrolledWindowRec, swindow.WidthPad), 
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNscrolledWindowMarginHeight, 
     sizeof (short),
     XtOffsetOf( struct _XmScrolledWindowRec, swindow.HeightPad),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNspacing, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmScrolledWindowRec, swindow.pad), 
     _XmFromHorizontalPixels },

};

/**************
 *
 *  Translation tables for Scrolled Window.
 *
 **************/

static XtTranslations ClipWindowXlations = NULL;

/****************
 * These are grab actions used for the keyboard movement of
 * the window. The table consists of three entries: a modifier
 * mask, a virtual key, and an action name.
 * These are converted to  real keysyms and used for the grab actions.
 ****************/
 
static _XmBuildVirtualKeyStruct ClipWindowKeys[] = {
     {0,           "osfPageDown",  "SWDownPageGrab()\n"},
     {ControlMask, "osfBeginLine", "SWTopLineGrab()\n"},
     {0,           "osfBeginLine", "SWBeginLineGrab()\n"},
     {ControlMask, "osfEndLine",   "SWBottomLineGrab()\n"},
     {0,           "osfEndLine",   "SWEndLineGrab()\n"},
     {ControlMask, "osfPageUp",    "SWLeftPageGrab()\n"},
     {0,        "osfPageUp",    "SWUpPageGrab()\n"},
     {ControlMask, "osfPageDown",  "SWRightPageGrab()\n"},
     {0,        "osfHelp",       "Help()"}
 };

static XtActionsRec ScrolledWActions[] =
{
 {"SWBeginLine",        LeftEdge},
 {"SWEndLine",          RightEdge},
 {"SWTopLine",          TopEdge},
 {"SWBottomLine",       BottomEdge},
 {"SWLeftPage",         PageLeft},
 {"SWRightPage",        PageRight},
 {"SWUpPage",           PageUp},
 {"SWDownPage",         PageDown}, 
 {"SWBeginLineGrab",    LeftEdgeGrabbed},
 {"SWEndLineGrab",      RightEdgeGrabbed},
 {"SWTopLineGrab",      TopEdgeGrabbed},
 {"SWBottomLineGrab",   BottomEdgeGrabbed},
 {"SWLeftPageGrab",     PageLeftGrabbed},
 {"SWRightPageGrab",    PageRightGrabbed},
 {"SWUpPageGrab",       PageUpGrabbed},
 {"SWDownPageGrab",     PageDownGrabbed}, 
 {"SWCWMapNotify",      CWMapNotify}, 
 {"SWNoop",             Noop}, 
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

externaldef(xmscrolledwindowclassrec) XmScrolledWindowClassRec 
        xmScrolledWindowClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &xmManagerClassRec,
    /* class_name         */    "XmScrolledWindow",
    /* widget_size        */    sizeof(XmScrolledWindowRec),
    /* class_initialize   */    NULL,
    /* class_partinit     */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* Init hook	  */    NULL,
    /* realize            */    Realize,
    /* actions		  */	ScrolledWActions,
    /* num_actions	  */	XtNumber(ScrolledWActions),
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	XtExposeCompressMaximal,
    /* compress_enterleave*/	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set values hook    */    (XtArgsFunc)NULL,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    NULL,
    /* accept_focus       */    NULL,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table		  */    XtInheritTranslations,
    /* query_geometry     */    QueryProc,
    /* display_accelerator*/    NULL,
    /* extension          */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	InsertChild,	
    /* delete_child	  */	XtInheritDeleteChild,	/* Inherit from superclass */
    /* Extension          */    NULL,
  },{
/* Constraint class Init */
    NULL,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
      
  },
/* Manager Class */
   {		
      ScrolledWindowXlations,	                /* translations        */    
      get_resources,				/* get resources      	  */
      XtNumber(get_resources),			/* num get_resources 	  */
      NULL,					/* get_cont_resources     */
      0,					/* num_get_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension           */    
   },

 {
/* Scrolled Window class - none */     
      (XtPointer) NULL,				/* extension           */    
 }
};

externaldef(xmscrolledwindowwidgetclass) WidgetClass 
    xmScrolledWindowWidgetClass = (WidgetClass)&xmScrolledWindowClassRec;



/************************************************************************
 *									*
 *   Callback Functions							*
 *   These are the callback routines for the scrollbar actions.		*
 *									*
 ************************************************************************/

static XtCallbackRec VSCallBack[] =
{
   {VertSliderMove, NULL},
   {(XtCallbackProc )NULL,           NULL},
};

static XtCallbackRec HSCallBack[] =
{
   {HorizSliderMove, (XtPointer) NULL},
   {(XtCallbackProc )NULL,           (XtPointer) NULL},
};

/************************************************************************
 *									*
 *  VertSliderMove							*
 *  Callback for the sliderMoved resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
VertSliderMove( w, closure, cd )
        Widget w ;
        XtPointer closure ;
        XtPointer cd ;
#else
VertSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer cd )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *call_data = (XmScrollBarCallbackStruct *) cd ;
    XmScrolledWindowWidget sw;

    sw = (XmScrolledWindowWidget )w->core.parent;
    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
    _XmMoveObject( sw->swindow.WorkWindow,
		 (Position ) (sw->swindow.WorkWindow->core.x),
		 (Position ) -((int ) call_data->value));
    sw->swindow.vOrigin = (int ) call_data->value;
    }
}

/************************************************************************
 *									*
 *  HorizSliderMove							*
 *  Callback for the sliderMoved resource of the horizontal scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
HorizSliderMove( w, closure, cd )
        Widget w ;
        XtPointer closure ;
        XtPointer cd ;
#else
HorizSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer cd )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *call_data = (XmScrollBarCallbackStruct *) cd ;
    XmScrolledWindowWidget sw;

    sw = (XmScrolledWindowWidget )w->core.parent;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
    _XmMoveObject( sw->swindow.WorkWindow,
		 (Position ) -((int) call_data->value),
 		 (Position ) (sw->swindow.WorkWindow->core.y));
    sw->swindow.hOrigin = (int ) call_data->value;
    }

}

/************************************************************************
 *									*
 *  KidKilled								*
 *  Destroy callback for the BB child widget.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KidKilled( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
KidKilled(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmScrolledWindowWidget sw;
    if (XmIsScrolledWindow(w->core.parent))
	sw = (XmScrolledWindowWidget)w->core.parent;
    else
	sw = (XmScrolledWindowWidget)w->core.parent->core.parent;
    
    if (sw->swindow.WorkWindow == w)
        sw->swindow.WorkWindow = NULL;

    sw->swindow.FromResize = TRUE;
    if (sw->swindow.VisualPolicy == XmVARIABLE)
        VariableLayout(sw);
    else
        ConstantLayout(sw);
	
    (*(sw->core.widget_class->core_class.expose))
		((Widget) sw,NULL,NULL);
    sw->swindow.FromResize = FALSE;
}

/************************************************************************
 *                                                                      *
 * LeftEdge - move the view to the left edge                            *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
LeftEdge( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
LeftEdge(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        value = sw->swindow.hmin;            
	XtSetArg (hSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,1);
        _XmMoveObject( sw->swindow.WorkWindow,
		 (Position ) -(value),
 		 (Position ) (sw->swindow.WorkWindow->core.y));
        sw->swindow.hOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}

/************************************************************************
 *                                                                      *
 * CallProcessTraversal: call XmProcessTraversal after having removed   *
 *        the traverseObscuredCallback, so that it isn't called and     *
 *        result in the paging operation going back to origin           *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CallProcessTraversal(w)
        Widget w ;
#else
CallProcessTraversal(
        Widget w)
#endif /* _NO_PROTO */
{
    XtCallbackList tmp ;
    XmScrolledWindowWidget sw = (XmScrolledWindowWidget) w;

    if (!sw->swindow.traverseObscuredCallback)
	XmProcessTraversal(sw->swindow.WorkWindow, XmTRAVERSE_CURRENT);
    else {
	tmp = sw->swindow.traverseObscuredCallback ;
	sw->swindow.traverseObscuredCallback = NULL ;
	XmProcessTraversal(sw->swindow.WorkWindow, XmTRAVERSE_CURRENT);
	sw->swindow.traverseObscuredCallback = tmp ;
    }
}

/************************************************************************
 *                                                                      *
 * RightEdge - move the view to the Right edge                          *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
RightEdge( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
RightEdge(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        value = sw->swindow.hmax - sw->swindow.hExtent;
	XtSetArg (hSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,1);

       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) -(value), 
		     (Position )(sw->swindow.WorkWindow->core.y));
        sw->swindow.hOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * TopEdge - move the view to the Top edge                              *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TopEdge( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TopEdge(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
       value = sw->swindow.vmin;
 
	XtSetArg (vSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,1);

       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) (sw->swindow.WorkWindow->core.x),
		     (Position ) -(value));
        sw->swindow.vOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * BottomEdge - move the view to the Bottom edge                        *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BottomEdge( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BottomEdge(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        value = sw->swindow.vmax - sw->swindow.vExtent;

	XtSetArg (vSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,1);
 
       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) (sw->swindow.WorkWindow->core.x),
		     (Position ) -(value));
        sw->swindow.vOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * PageLeft - Scroll left a page                                        *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageLeft( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageLeft(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    XmScrollBarWidget sb = (XmScrollBarWidget) sw->swindow.hScrollBar;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        if (sb) 
            value = sw->swindow.hOrigin - sb->scrollBar.page_increment;
        else
            value = sw->swindow.hOrigin - sw->swindow.WorkWindow->core.width;
            
        if (value < sw->swindow.hmin) 
            value = sw->swindow.hmin;
	XtSetArg (hSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,1);

        _XmMoveObject( sw->swindow.WorkWindow,
		 (Position ) -(value),
 		 (Position ) (sw->swindow.WorkWindow->core.y));
        sw->swindow.hOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * PageRight - Scroll Right a page                                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageRight( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageRight(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    XmScrollBarWidget sb = (XmScrollBarWidget) sw->swindow.hScrollBar;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        if (sb) 
            value = sw->swindow.hOrigin + sb->scrollBar.page_increment;
        else
            value = sw->swindow.hOrigin + sw->swindow.WorkWindow->core.width;;
            
        if (value > (sw->swindow.hmax - sw->swindow.hExtent)) 
            value = sw->swindow.hmax - sw->swindow.hExtent;
 
	XtSetArg (hSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,1);

       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) -(value), 
		     (Position )(sw->swindow.WorkWindow->core.y));
        sw->swindow.hOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * PageUp - Scroll up a page                                            *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageUp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageUp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    XmScrollBarWidget sb = (XmScrollBarWidget) sw->swindow.vScrollBar;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        if (sb) 
            value = sw->swindow.vOrigin - sb->scrollBar.page_increment;
        else
            value = sw->swindow.vOrigin - sw->swindow.WorkWindow->core.height;
            
        if (value < sw->swindow.vmin) 
            value = sw->swindow.vmin;

	XtSetArg (vSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,1);
 
       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) (sw->swindow.WorkWindow->core.x),
		     (Position ) -(value));
        sw->swindow.vOrigin = value;
        CallProcessTraversal((Widget) sw);
    }
}
/************************************************************************
 *                                                                      *
 * PageDown - Scroll Down a page                                        *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageDown( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageDown(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    XmScrollBarWidget sb = (XmScrollBarWidget) sw->swindow.vScrollBar;
    int value;
    if (!sw->swindow.WorkWindow) return;

    if ((sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
        (!sw->swindow.WorkWindow->core.being_destroyed))
    {
        if (sb) 
            value = sw->swindow.vOrigin + sb->scrollBar.page_increment;
        else
            value = sw->swindow.vOrigin + sw->swindow.WorkWindow->core.height;
            
        if (value > (sw->swindow.vmax - sw->swindow.vExtent)) 
            value = sw->swindow.vmax - sw->swindow.vExtent;

	XtSetArg (vSBArgs[0], XmNvalue, (XtArgVal) value);
	XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,1);
 
       _XmMoveObject( sw->swindow.WorkWindow,
                     (Position ) (sw->swindow.WorkWindow->core.x),
		     (Position ) -(value));
        sw->swindow.vOrigin = value;
        CallProcessTraversal((Widget) sw);
    }

}
/************************************************************************
 *                                                                      *
 * Grab routines - these are shells that set up the right widget and    *
 * call the appropriate "normal" routine.                               *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
LeftEdgeGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
LeftEdgeGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    LeftEdge(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
RightEdgeGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
RightEdgeGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    RightEdge(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
TopEdgeGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TopEdgeGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     TopEdge(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
BottomEdgeGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BottomEdgeGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     BottomEdge(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
PageLeftGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageLeftGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     PageLeft(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
PageRightGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageRightGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     PageRight(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
PageUpGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageUpGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     PageUp(XtParent(sw), event, params, num_params);
}

static void 
#ifdef _NO_PROTO
PageDownGrabbed( sw, event, params, num_params )
        Widget sw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PageDownGrabbed(
        Widget sw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
     PageDown(XtParent(sw), event, params, num_params);
}

/************************************************************************
 *                                                                      *
 * CWMapNotify - When the clip window is mapped, we need to add the     *
 *               virtual keys as the translation table. Ugh.            *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CWMapNotify( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CWMapNotify(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XtTranslations virtClipWindowXlations = 
	XtParseTranslationTable(ClipWindowTranslationTable);

    XtOverrideTranslations (w, virtClipWindowXlations);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Noop( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Noop(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
	return;
}


/************************************************************************
 *                                                                      *
 * _XmGetRealXlations - Build up a "real" translation table out of      *
 * virtual keysyms.                                                     *
 *                                                                      *
 ************************************************************************/
char * 
#ifdef _NO_PROTO
_XmGetRealXlations( dpy, keys, num_keys )
        Display *dpy ;
        _XmBuildVirtualKeyStruct *keys ;
        int num_keys ;
#else
_XmGetRealXlations(
        Display *dpy,
        _XmBuildVirtualKeyStruct *keys,
        int num_keys )
#endif /* _NO_PROTO */
{
    char *result, tmp[1000];     /* Use static array for speed - should be dynamic */
    char *keystring;
    register int i;
    Modifiers mods;
    KeySym   keysym;
    int tmplen;
    
    tmp[0] = '\0';
    for (i = 0; i < num_keys; i++)
    {
        keysym = XStringToKeysym(keys[i].key);
        if (keysym == NoSymbol) 
            break;
            
        _XmVirtualToActualKeysym(dpy, keysym, &keysym, &mods);
        keystring = XKeysymToString(keysym);
        if (!keystring)
            continue;
        mods |= keys[i].mod;

        if (mods & ControlMask)
            strcat(tmp, "Ctrl ");

        if (mods & ShiftMask)
            strcat(tmp, "Shift ");

        if (mods & Mod1Mask)
            strcat(tmp, "Mod1 ");        /* "Alt" may not be right on some systems... */

        strcat(tmp,"<Key>");
        strcat(tmp, keystring);
        strcat(tmp,": ");
        strcat(tmp,keys[i].action);
    }

    tmplen = strlen (tmp);
    if (tmplen > 0) {
	if (tmp[tmplen-1] == '\n') tmp[tmplen-1] = '\0';
        result = XtNewString(tmp);
    }
    else 
        result = NULL;
    return(result);
    
}


/************************************************************************
 *									*
 *  ClassPartInitialize - Set up the fast subclassing.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   _XmFastSubclassInit (wc, XmSCROLLED_WINDOW_BIT);
}


/************************************************************************
 *									*
 *  Initialize								*
 *									*
 ************************************************************************/
/* ARGSUSED */
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
        XmScrolledWindowWidget request = (XmScrolledWindowWidget) rw ;
        XmScrolledWindowWidget new_w = (XmScrolledWindowWidget) nw ;
    Widget bw;
    Dimension ht;
    int    i, j, k;
    char *ClipWindowXlatString;
    unsigned int mask = (unsigned)(KeyPressMask | KeyReleaseMask);

    new_w->swindow.InInit = TRUE;
	
/****************
 *
 *  Bounds check the size.
 *
 ****************/
    if (request->core.width==0 || (request->core.height==0))
    {
        if (request->core.width==0)
	    new_w->core.width = 100;
	if (request->core.height==0)
	    new_w->core.height = 100;
    }
    new_w->swindow.GivenWidth = request->core.width;
    new_w->swindow.GivenHeight = request->core.height;
  
    i = 0; j = 0; k = 0; 
    bw = NULL;

    if (new_w->swindow.pad == XmINVALID_DIMENSION) 
        new_w->swindow.pad = 4;


    if(    !XmRepTypeValidValue( XmRID_SCROLLING_POLICY,
                                  new_w->swindow.ScrollPolicy, (Widget) new_w)    )
	/* CR 4356: HaL provided fix, was AUTOMATIC, now back
	   to the real default */
    {
	new_w->swindow.ScrollPolicy = XmAPPLICATION_DEFINED;
    }

    if(    !XmRepTypeValidValue( XmRID_VISUAL_POLICY,
                                  new_w->swindow.VisualPolicy, (Widget) new_w)    )
    {
        if (new_w->swindow.ScrollPolicy == XmAUTOMATIC)
            new_w->swindow.VisualPolicy = XmCONSTANT;
        else
	    new_w->swindow.VisualPolicy = XmVARIABLE;
    }

    if ((new_w->swindow.ScrollPolicy == XmAPPLICATION_DEFINED) &&
        (new_w->swindow.VisualPolicy == XmCONSTANT))
    {
	_XmWarning( (Widget) new_w, SWMessage11);
        new_w->swindow.VisualPolicy = XmVARIABLE;        
    }

    if (new_w->swindow.ScrollBarPolicy == 255)
    {
        if (new_w->swindow.ScrollPolicy == XmAUTOMATIC)
           new_w->swindow.ScrollBarPolicy = XmAS_NEEDED;
        else
           new_w->swindow.ScrollBarPolicy = XmSTATIC;
    }

    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                               new_w->swindow.ScrollBarPolicy, (Widget) new_w)    )
    {
        if (new_w->swindow.ScrollPolicy == XmAUTOMATIC)
           new_w->swindow.ScrollBarPolicy = XmAS_NEEDED;
        else
           new_w->swindow.ScrollBarPolicy = XmSTATIC;
    }	

    if (new_w->swindow.ScrollPolicy == XmAUTOMATIC) 
    	new_w->swindow.VisualPolicy = XmCONSTANT;


    if ((new_w->swindow.VisualPolicy == XmVARIABLE) &&
	(request->swindow.ScrollBarPolicy == XmAS_NEEDED))
    {
	_XmWarning( (Widget) new_w, SWMessage8);
	new_w->swindow.ScrollBarPolicy = XmSTATIC;
    }


    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_PLACEMENT,
                                     new_w->swindow.Placement, (Widget) new_w)    )
    {
	new_w->swindow.Placement = XmBOTTOM_RIGHT;
    }	


    if (request->manager.shadow_thickness == XmINVALID_DIMENSION)
        if (new_w->swindow.ScrollPolicy == XmAUTOMATIC)
            new_w->manager.shadow_thickness = 2;
	else
            new_w->manager.shadow_thickness = 0;
	
    ht = new_w->manager.shadow_thickness;    

/* CR 5319 begin */
    if (new_w->core.width > ht*2)
      new_w->swindow.AreaWidth = new_w->core.width - (ht * 2);
    else
      new_w->swindow.AreaWidth = 0;
    if (new_w->core.height > ht*2)
      new_w->swindow.AreaHeight = new_w->core.height - (ht * 2);
    else
      new_w->swindow.AreaHeight = 0;
/* CR 5319 end */
 
    new_w->swindow.FromResize = FALSE;

/****************
 *
 * We will set the X and Y offsets to the pad values. That lets us use
 * the four variables as a "margin" to the user, but MainWindow can still
 * dink around with them if it wants.
 *
 ****************/
    new_w->swindow.XOffset = new_w->swindow.WidthPad;
    new_w->swindow.YOffset = new_w->swindow.HeightPad;

    XtAugmentTranslations((Widget) new_w, (XtTranslations)
              ((XmManagerClassRec *)XtClass(new_w))->manager_class.translations);

/****************
 *
 * If the policy is constant, create a clip window. Note that if the scroll policy
 * is auto, we force the visual policy to constant.
 *
 ****************/
    
    if (new_w->swindow.VisualPolicy == XmCONSTANT)
    {
        if (InitCW)
        {
            InitCW = FALSE;

            ClipWindowXlatString = _XmGetRealXlations(XtDisplay(new_w),
                                                      ClipWindowKeys,
                                                      XtNumber(ClipWindowKeys));
            if (ClipWindowXlatString)
            {
                ClipWindowXlatString = XtRealloc(ClipWindowXlatString,
                                            strlen(ClipWindowXlatString) +
                                            strlen("\n<MapNotify>: SWCWMapNotify()") +
                                            1);
                strcat(ClipWindowXlatString, "\n<MapNotify>: SWCWMapNotify()");
                ClipWindowXlations = XtParseTranslationTable(ClipWindowXlatString);
		/*
		 * Fix for CR 5522 - Free the string ClipWindowXlatString.
		 */
		XtFree(ClipWindowXlatString);
		/*
		 * End Fix for CR 5522
		 */
 
                XtRegisterGrabAction(LeftEdgeGrabbed,  TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(RightEdgeGrabbed, TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(TopEdgeGrabbed,   TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(BottomEdgeGrabbed,TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(PageLeftGrabbed,  TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(PageRightGrabbed, TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(PageUpGrabbed,    TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
                XtRegisterGrabAction(PageDownGrabbed,  TRUE, mask, 
                                     GrabModeAsync, GrabModeAsync);
            }
        }

        new_w->swindow.InInit = TRUE;
	XtSetArg (cwArgs[k],XmNx,(XtArgVal) ht); k++;
	XtSetArg (cwArgs[k],XmNy,(XtArgVal) ht); k++;
	XtSetArg (cwArgs[k],XmNshadowThickness,(XtArgVal) 0); k++;
	XtSetArg (cwArgs[k],XmNborderWidth,(XtArgVal) 0); k++;
	XtSetArg (cwArgs[k],XmNmarginWidth,(XtArgVal) 0); k++;
	XtSetArg (cwArgs[k],XmNmarginHeight,(XtArgVal) 0); k++;
    /*	XtSetArg (cwArgs[k],XmNwidth,(XtArgVal) ((int ) (new_w->swindow.AreaWidth))); k++;
	XtSetArg (cwArgs[k],XmNheight,(XtArgVal) ((int ) (new_w->swindow.AreaHeight))); k++;*/
	XtSetArg (cwArgs[k],XmNresizePolicy, (XtArgVal) XmRESIZE_SWINDOW); k++;
	new_w->swindow.ClipWindow = (XmDrawingAreaWidget) XtCreateManagedWidget(
				  "ScrolledWindowClipWindow", 
				  xmDrawingAreaWidgetClass,(Widget) new_w,cwArgs, k);
	if (ClipWindowXlations) 
            XtOverrideTranslations((Widget) new_w->swindow.ClipWindow,
                                          (XtTranslations) ClipWindowXlations);
        new_w->swindow.InInit = FALSE;
        bw =(Widget ) new_w->swindow.ClipWindow;
    }

/****************
 *
 * If the application wants to do all the work, we're done!
 *
 ****************/
    if (new_w->swindow.ScrollPolicy == XmAPPLICATION_DEFINED)
    {
    new_w->swindow.InInit = FALSE;    
	return;	
    }

/****************
 *
 *  Else we're in autopilot mode - create the scroll bars, and init the internal
 *  fields. The location and size don't matter right now - we'll figure them out
 *  for real at changemanaged time.
 *
 ****************/

    new_w->swindow.vsbX =  new_w->core.width;
    new_w->swindow.vsbY = 0;
    new_w->swindow.hsbX  = 0;
    new_w->swindow.hsbY = new_w->core.height;
    new_w->swindow.hsbWidth = new_w->swindow.AreaWidth + (ht * 2);
    new_w->swindow.vsbHeight = new_w->swindow.AreaHeight+ (ht * 2);

/* CR 5319 begin */
    if (new_w->swindow.vsbX > ht*2)
      new_w->swindow.AreaWidth = new_w->swindow.vsbX - ((ht * 2) );
    else
      new_w->swindow.AreaWidth = 0;
    if (new_w->swindow.hsbY > ht*2)
      new_w->swindow.AreaHeight = new_w->swindow.hsbY - ((ht * 2) );
    else
      new_w->swindow.AreaHeight = 0;
/* CR 5319 end */

    XtSetArg (vSBArgs[i], XmNorientation,(XtArgVal) (XmVERTICAL)); i++;

    new_w -> swindow.vmin = 0;
    XtSetArg (vSBArgs[i], XmNminimum, (XtArgVal) (new_w->swindow.vmin)); i++;

    new_w->swindow.vmax =  new_w->core.height + ht;
    XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) (new_w->swindow.vmax)); i++;

    new_w -> swindow.vOrigin = 0;
    XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) new_w->swindow.vOrigin); i++;

    if ((int )(bw->core.height - bw->core.y) < 10)
	new_w->swindow.vExtent = new_w->swindow.vmax - new_w->swindow.vOrigin;
    else
        new_w->swindow.vExtent =  (int ) ((bw->core.height - bw->core.y) / 10);

    XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) (new_w->swindow.vExtent)); i++;
    
    XtSetArg (hSBArgs[j], XmNorientation,(XtArgVal) (XmHORIZONTAL)); j++;

    new_w -> swindow.hmin = 0;
    XtSetArg (hSBArgs[j], XmNminimum, (XtArgVal) (new_w->swindow.hmin)); j++;

    new_w -> swindow.hmax =  new_w->core.width + ht; 
    XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (new_w->swindow.hmax)); j++;

    new_w -> swindow.hOrigin = 0;
    XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) 	new_w -> swindow.hOrigin); j++;

    if ((int )(bw->core.width - bw->core.x) < 10)
	    new_w->swindow.hExtent = new_w->swindow.hmax - new_w->swindow.hOrigin;
    else
           new_w -> swindow.hExtent =  (int ) ((bw->core.width - bw->core.x) / 10);
    XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (new_w->swindow.hExtent)); j++;


    XtSetArg(vSBArgs[i], XmNincrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageIncrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageDecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoTopCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoBottomCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdragCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNvalueChangedCallback, (XtArgVal) VSCallBack); i++;
/*    XtSetArg(vSBArgs[i], XmNtraversalOn, (XtArgVal) TRUE); i++;
    XtSetArg(vSBArgs[i], XmNbackground, 
                         (XtArgVal) new_w->core.background_pixel); i++;*/

    XtSetArg(hSBArgs[j], XmNincrementCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNdecrementCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNpageIncrementCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNpageDecrementCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNtoTopCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNtoBottomCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNdragCallback, (XtArgVal) HSCallBack); j++;
    XtSetArg(hSBArgs[j], XmNvalueChangedCallback, (XtArgVal) HSCallBack); j++;
/*    XtSetArg(hSBArgs[j], XmNtraversalOn, (XtArgVal) TRUE); j++;
    XtSetArg(hSBArgs[j], XmNbackground, 
                         (XtArgVal) new_w->core.background_pixel); j++;*/

    new_w->swindow.InInit = TRUE;
    
    new_w->swindow.vScrollBar =
	(XmScrollBarWidget) XtCreateManagedWidget("VertScrollBar", xmScrollBarWidgetClass,
					   (Widget) new_w,vSBArgs, i); 
    new_w->swindow.hScrollBar =
	(XmScrollBarWidget) XtCreateManagedWidget("HorScrollBar", xmScrollBarWidgetClass,
					   (Widget) new_w,hSBArgs, j);
/*************
 *	
 * Style-guide Hack to make sure that the scrollbars have a highlight.
 * Note the antisocial plundering of the data structures, necessary to avoid 
 * problems with unit-type. It's OK to do this cuz we'll be doing a real 
 * XmConfigure on them at changemanaged time...
 *	
 *************/
    if ((new_w->swindow.vScrollBar)->primitive.highlight_thickness == 0) 
    {
        (new_w->swindow.vScrollBar)->primitive.highlight_thickness = 2;
  	(new_w->swindow.vScrollBar)->core.width += 4;
    }
    if ((new_w->swindow.hScrollBar)->primitive.highlight_thickness == 0)
    {
        (new_w->swindow.hScrollBar)->primitive.highlight_thickness = 2;
  	(new_w->swindow.hScrollBar)->core.height += 4;
    }


    new_w->swindow.WorkWindow = NULL;
    new_w->swindow.InInit = FALSE;    
}



/************************************************************************
 *									*
 *  Realize								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Realize( wid, p_valueMask, attributes )
        Widget wid ;
        XtValueMask *p_valueMask ;
        XSetWindowAttributes *attributes ;
#else
Realize(
        Widget wid,
        XtValueMask *p_valueMask,
        XSetWindowAttributes *attributes )
#endif /* _NO_PROTO */
{
        register XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    Mask valueMask = *p_valueMask;

/****************
 *
 * We don't know if the bboard grew since creation, so we force it back to our 
 * size if needed.
 *
 ****************/ 
    if (sw->swindow.VisualPolicy == XmCONSTANT)
    {    
        if ((sw->swindow.AreaWidth != sw->swindow.ClipWindow->core.width) ||
            (sw->swindow.AreaHeight != sw->swindow.ClipWindow->core.height))
	    _XmResizeObject( (Widget) sw->swindow.ClipWindow, 
                                 sw->swindow.AreaWidth, sw->swindow.AreaHeight,
                                    sw->swindow.ClipWindow->core.border_width);
    }

   valueMask |= CWDontPropagate;
   attributes->do_not_propagate_mask =  
         ButtonPressMask | ButtonReleaseMask |
         KeyPressMask | KeyReleaseMask | PointerMotionMask;
      
    XtCreateWindow((Widget) sw, InputOutput, CopyFromParent,
	valueMask, attributes);

 } /* Realize */



/************************************************************************
 *									*
 *  Redisplay - General redisplay function called on exposure events.	*
 *									*
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
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    Dimension ht, bw;

    if (event)
       _XmRedisplayGadgets( (Widget) sw, event, region);
   
   if (XtIsRealized (sw) && (sw->swindow.ScrollPolicy == XmAUTOMATIC))
    {
	ht = sw->manager.shadow_thickness;
 	_XmDrawShadows (XtDisplay (sw), XtWindow (sw), 
 			 sw -> manager.bottom_shadow_GC,
 			 sw -> manager.top_shadow_GC,
 			 sw->swindow.ClipWindow->core.x - ht, 
 			 sw->swindow.ClipWindow->core.y - ht, 
 			 sw -> swindow.AreaWidth + (ht * 2),
 			 sw -> swindow.AreaHeight + (ht * 2), 
			 sw->manager.shadow_thickness,
 			 XmSHADOW_OUT);
    }
    else
       if (XtIsRealized (sw))
       {
  	   ht = sw->manager.shadow_thickness;
           if (sw->swindow.WorkWindow)
           {
	       bw = sw->swindow.WorkWindow->core.border_width;
 	       _XmDrawShadows (XtDisplay (sw), XtWindow (sw), 
 				sw -> manager.bottom_shadow_GC,
 				sw -> manager.top_shadow_GC,
 				sw->swindow.WorkWindow->core.x - ht,
 				sw->swindow.WorkWindow->core.y - ht,
 				sw -> swindow.AreaWidth + ((bw + ht) * 2),
 				sw -> swindow.AreaHeight + ((bw + ht) * 2), 
 				sw->manager.shadow_thickness,
 				XmSHADOW_OUT);
              }
              else
		  _XmDrawShadows (XtDisplay (sw), XtWindow (sw), 
 				sw -> manager.bottom_shadow_GC,
 				sw -> manager.top_shadow_GC,
 				0, 0,
 				sw -> swindow.AreaWidth + (ht * 2),
 				sw -> swindow.AreaHeight + (ht * 2), 
 				sw->manager.shadow_thickness,
 				XmSHADOW_OUT);
       }
}

/************************************************************************
 *									*
 *  ClearBorder - Clear the right and bottom border area and save the	*
 *	frame's width and height.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClearBorder( sw )
        XmScrolledWindowWidget sw ;
#else
ClearBorder(
        XmScrolledWindowWidget sw )
#endif /* _NO_PROTO */
{
    Dimension ht,oldwidth,oldheight;
    Position x,y;
    Boolean force = FALSE;
    
   if (XtIsRealized (sw) && (sw->swindow.ScrollPolicy == XmAUTOMATIC))
   {

      ht = sw->manager.shadow_thickness;
      oldwidth = sw -> swindow.AreaWidth + ht;
      oldheight = sw ->swindow.AreaHeight + ht;
      x = sw->swindow.ClipWindow->core.x;
      if (sw->swindow.WidthPad)
          x -= ht;
      y = sw->swindow.ClipWindow->core.y;
      if (sw->swindow.HeightPad)
          y -= ht;
/****************
 *
 * Special case for if the clip window is exactly the size of the
 * scrolled window - gotta clear all the time.
 *
 ****************/
      if ((oldwidth == (sw->core.width - ht)) &&
          (oldheight == (sw->core.height - ht)))
      {
          force = TRUE;
          x = ht; y = ht;
      }

      if ((sw->swindow.AreaWidth != ((sw->swindow.ClipWindow)->core.width)) ||
          force)
          {
	      XClearArea (XtDisplay (sw), XtWindow (sw),
             		  x - ht, 
			  y - ht, 
			  oldwidth, ht, FALSE);
              XClearArea (XtDisplay (sw), XtWindow (sw),
                          x-ht, 
			  oldheight  + sw->swindow.HeightPad, 
			  oldwidth + ht, ht, FALSE);
	  }

      if ((sw->swindow.AreaHeight != ((sw->swindow.ClipWindow)->core.height)) ||
          force)
          {
              XClearArea (XtDisplay (sw), XtWindow (sw),
                          oldwidth + sw->swindow.WidthPad, 
			  y-ht, 
			  ht, oldheight + ht, FALSE);
              XClearArea (XtDisplay (sw), XtWindow (sw),
                          x - ht, 
			  y - ht, 
			  ht, oldheight, FALSE);
	  }
   }

}



/************************************************************************
 *									*
 *  InsertChild								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
InsertChild( w )
        Widget w ;
#else
InsertChild(
        Widget w )
#endif /* _NO_PROTO */
{
    XmManagerWidgetClass     superclass;
    XmScrolledWindowWidget   sw = (XmScrolledWindowWidget )w->core.parent;
    XmScrollBarWidget        sb;
    XmDrawingAreaWidget      da;
    Boolean		     punt;

    if (!XtIsRectObj(w))
        return;

    /* do a sanity check first */
    if( sw->swindow.WorkWindow != NULL &&
       sw->swindow.WorkWindow->core.being_destroyed ) {
	sw->swindow.WorkWindow = NULL;
    }
    if( sw->swindow.hScrollBar != NULL &&
       sw->swindow.hScrollBar->core.being_destroyed ) {
	sw->swindow.hScrollBar = NULL;
    }
    if( sw->swindow.vScrollBar != NULL &&
       sw->swindow.vScrollBar->core.being_destroyed ) {
	sw->swindow.vScrollBar = NULL;
    }

/****************
 *
 * If we are in init, assume it's an internal widget, and the instance vars
 * will be set accordingly.
 *
 ****************/

    superclass = (XmManagerWidgetClass)xmManagerWidgetClass;
    
    if (sw->swindow.InInit)
    {
	(*superclass->composite_class.insert_child)(w);
	return;
    }
/****************
 *
 * If variable, look at the class and take a guess as to what it is.
 *
 ****************/
 
    if (sw->swindow.ScrollPolicy == XmAPPLICATION_DEFINED)
    {
	if (XtClass(w) == xmScrollBarWidgetClass)
	{
	    sb =  (XmScrollBarWidget) w;
	    if (sb->scrollBar.orientation == XmHORIZONTAL)
                sw->swindow.hScrollBar = sb;
            else
                sw->swindow.vScrollBar = sb;
	}
        else
    	    if (XtClass(w) == xmDrawingAreaWidgetClass)
    	    {
	        da =  (XmDrawingAreaWidget) w;
	        if (da->drawing_area.resize_policy == XmRESIZE_SWINDOW)
                    sw->swindow.ClipWindow = da;
		else
                    if (sw->swindow.WorkWindow == NULL) {
                        XtAddCallback((Widget) w, XmNdestroyCallback,KidKilled,NULL);
			sw->swindow.WorkWindow = w;
		    }
	    }
            else
                if (sw->swindow.WorkWindow == NULL) {
		    XtAddCallback((Widget) w, XmNdestroyCallback,KidKilled,NULL);
		    sw->swindow.WorkWindow = w;
		}
	(*superclass->composite_class.insert_child)(w);
	return;
    }

/****************
 *
 *  Else, we're in constant mode. If the kid is not a scrollbar,
 *  or a DrawingArea with the resizepolicy set to resize_swindow, 
 *  and we have a clip window, reparent it to the clipwindow and 
 *  call it the work window.
 *
 ****************/
    else
    {
        punt = FALSE;
	if (XtClass(w) == xmDrawingAreaWidgetClass)
	{
	    da =  (XmDrawingAreaWidget) w;
	    punt = (da->drawing_area.resize_policy == XmRESIZE_SWINDOW);
	}
        if ((XtClass(w) == xmScrollBarWidgetClass) ||
	    (sw->swindow.ClipWindow == NULL)      ||
	        punt)
  	    (*superclass->composite_class.insert_child)(w);
        else
	{
	    if (sw->swindow.WorkWindow != NULL) 
	       XtRemoveCallback(sw->swindow.WorkWindow, XmNdestroyCallback,
				KidKilled, NULL);
	    
	    sw->swindow.WorkWindow = w;
	    w->core.parent = (Widget )sw->swindow.ClipWindow;
	    XtAddCallback((Widget) w, XmNdestroyCallback,KidKilled,NULL);
	    (*superclass->composite_class.insert_child)(w);
	}
    }
}


/************************************************************************
 *									*
 * _XmInitializeScrollBars - initialize the scrollbars for auto mode.	*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmInitializeScrollBars( w )
        Widget w ;
#else
_XmInitializeScrollBars(
        Widget w )
#endif /* _NO_PROTO */
{
    XmScrolledWindowWidget	sw = (XmScrolledWindowWidget) w;
    int i, inc;
    Dimension bw;
    
    if (sw->swindow.VisualPolicy == XmVARIABLE)
	return;	
    
    bw = 0;
    if (sw->swindow.WorkWindow)
        bw = sw->swindow.WorkWindow->core.border_width;
    
    sw->swindow.vmin = 0;    
    sw->swindow.vOrigin = 0;
    sw->swindow.hmin = 0;    
    sw->swindow.hOrigin = 0;
    if (ScrollBarVisible(sw->swindow.WorkWindow))
    {
        sw->swindow.vOrigin = abs(sw->swindow.WorkWindow->core.y);
	sw->swindow.vmax = sw->swindow.WorkWindow->core.height + (2 * bw);
	if (sw->swindow.vmax <1) sw->swindow.vmax = 1;
	sw->swindow.vExtent = sw->swindow.AreaHeight;
        if (sw->swindow.vOrigin < sw->swindow.vmin)
            sw->swindow.vOrigin = sw->swindow.vmin;

	if ((sw->swindow.vExtent + sw->swindow.vOrigin) > sw->swindow.vmax)
	    sw->swindow.vExtent = sw->swindow.vmax - sw->swindow.vOrigin;
	if (sw->swindow.vExtent < 0)
        {
	    sw->swindow.vExtent = sw->swindow.vmax;
            sw->swindow.vOrigin = sw->swindow.vmin;
        }

	sw->swindow.hmax = sw->swindow.WorkWindow->core.width + (2 * bw);
	if (sw->swindow.hmax <1) sw->swindow.hmax = 1;
        sw->swindow.hOrigin = abs(sw->swindow.WorkWindow->core.x);
	sw->swindow.hExtent = sw->swindow.AreaWidth;
        if (sw->swindow.hOrigin < sw->swindow.hmin)
            sw->swindow.hOrigin = sw->swindow.hmin;

	if ((sw->swindow.hExtent + sw->swindow.hOrigin) > sw->swindow.hmax)
	    sw->swindow.hExtent = sw->swindow.hmax - sw->swindow.hOrigin;
	if (sw->swindow.hExtent < 0)
        {
	    sw->swindow.hExtent = sw->swindow.hmax;
            sw->swindow.hOrigin = sw->swindow.hmin;
        }

    }
    else
    {
	sw->swindow.vExtent = (sw->swindow.ClipWindow->core.height > 0) ?
			       sw->swindow.ClipWindow->core.height : 1;
	sw->swindow.hExtent = (sw->swindow.ClipWindow->core.width > 0) ?
			       sw->swindow.ClipWindow->core.width : 1;
	sw->swindow.vmax = sw->swindow.vExtent;
	sw->swindow.hmax = sw->swindow.hExtent;
    }
    if(sw->swindow.vScrollBar)
    {
	i = 0;
        if (sw->swindow.WorkWindow)
        {
            if ((inc = ((sw->swindow.WorkWindow->core.height) / 10)) < 1)
                inc = 1;
            XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) inc); i++; 
        }
	if ((inc = (sw->swindow.AreaHeight - (sw->swindow.AreaHeight / 10))) < 1)
	    inc = sw->swindow.AreaHeight;
        XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal) inc); i++;
	XtSetArg (vSBArgs[i], XmNminimum, (XtArgVal) (sw->swindow.vmin)); i++;
	XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) (sw->swindow.vmax)); i++;
	XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) sw->swindow.vOrigin); i++;
	XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) (sw->swindow.vExtent)); i++;
	XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,i);
    }
    if(sw->swindow.hScrollBar)
    {
	i = 0;
        if (sw->swindow.WorkWindow)
        {
            if ((inc = ((sw->swindow.WorkWindow->core.width) / 10)) < 1)
                inc = 1;
            XtSetArg (hSBArgs[i], XmNincrement, (XtArgVal) inc); i++; 
        }
	if ((inc = (sw->swindow.AreaWidth - (sw->swindow.AreaWidth / 10))) < 1)
	    inc = sw->swindow.AreaWidth;

        XtSetArg (hSBArgs[i], XmNpageIncrement, (XtArgVal) inc); i++;
	XtSetArg (hSBArgs[i], XmNminimum, (XtArgVal) (sw->swindow.hmin)); i++;
	XtSetArg (hSBArgs[i], XmNmaximum, (XtArgVal) (sw->swindow.hmax)); i++;
	XtSetArg (hSBArgs[i], XmNvalue, (XtArgVal) sw->swindow.hOrigin); i++;
	XtSetArg (hSBArgs[i], XmNsliderSize, (XtArgVal) (sw->swindow.hExtent)); i++;
	XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,i);
    }
    
}

/************************************************************************
 *									*
 * VariableLayout - Layout the scrolled window for a visual policy of	*
 * XmVARIABLE.								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
VariableLayout( sw )
        XmScrolledWindowWidget sw ;
#else
VariableLayout(
        XmScrolledWindowWidget sw )
#endif /* _NO_PROTO */
{
    Position	 newx, newy, vsbX, vsbY, hsbX, hsbY;
    Dimension HSBht, VSBht;
    Dimension MyWidth, MyHeight, bbWidth, bbHeight;
    Dimension ht, bw, pad;
    int	tmp;
    Boolean HasHSB, HasVSB, HSBTrav, VSBTrav;
    XtWidgetGeometry  desired, preferred;

    /* do a sanity check first */
    if( sw->swindow.WorkWindow != NULL &&
       sw->swindow.WorkWindow->core.being_destroyed ) {
	sw->swindow.WorkWindow = NULL;
    }
    if( sw->swindow.hScrollBar != NULL &&
       sw->swindow.hScrollBar->core.being_destroyed ) {
	sw->swindow.hScrollBar = NULL;
    }
    if( sw->swindow.vScrollBar != NULL &&
       sw->swindow.vScrollBar->core.being_destroyed ) {
	sw->swindow.vScrollBar = NULL;
    }

    tmp = (int) sw->core.width - sw->swindow.XOffset - sw->swindow.WidthPad;
    if (tmp <= 0)
        MyWidth = 10;
    else 
        MyWidth = (Dimension )tmp;
    tmp = (int) sw->core.height - sw->swindow.YOffset - sw->swindow.HeightPad;
    if (tmp <= 0)
        MyHeight = 10;
    else 
        MyHeight = (Dimension )tmp;
       
	    	    
    ht = sw->manager.shadow_thickness;

    if ((sw->swindow.WorkWindow == NULL) || 		/* If no kid, just keep the */
        (!XtIsManaged(sw->swindow.WorkWindow)))		/* scrollbars invisible.    */
    {					
	if (sw->swindow.vScrollBar)
	    _XmMoveObject( (Widget) sw->swindow.vScrollBar,sw->core.width,
			 sw->swindow.vScrollBar->core.y);

	if (sw->swindow.hScrollBar)
	    _XmMoveObject( (Widget) sw->swindow.hScrollBar,
                              sw->swindow.hScrollBar->core.x, sw->core.height);

/* CR 5319 begin */
    if (sw->core.width > sw->swindow.WidthPad + ht*2)
      sw->swindow.AreaWidth = sw->core.width - sw->swindow.WidthPad
	- ((ht * 2) );
    else
      sw->swindow.AreaWidth = 0;
    if (sw->core.height > sw->swindow.HeightPad + ht*2)
      sw->swindow.AreaHeight = sw->core.height - sw->swindow.HeightPad
	- ((ht * 2) );
    else
      sw->swindow.AreaHeight = 0;
/* CR 5319 end */

	if (sw->swindow.ClipWindow)
	    _XmConfigureObject( (Widget) sw->swindow.ClipWindow,
                            ht + sw->swindow.XOffset, ht + sw->swindow.YOffset,
                             sw->swindow.AreaWidth, sw->swindow.AreaHeight, 0);

	return;
    }

/****************
 *
 * Need to have some border width variables for the scrollbars here.
 *
 ****************/
 
    bw = sw->swindow.WorkWindow->core.border_width;
    
    tmp = (int )MyWidth - (((int )ht+(int )bw) * 2);
    if (tmp <= 0)
        bbWidth = 2;
    else 
        bbWidth = (Dimension )tmp;

    tmp = (int )MyHeight - (((int )ht+(int )bw) * 2);
    if (tmp <= 0)
        bbHeight = 2;
    else 
        bbHeight = (Dimension )tmp;

/****************
 *
 * OK, we just figured out the maximum size for the child (with no
 * scrollbars(it's in bbwidth, bbheight). Now query the 
 * kid geometry - tell him that we are going to resize him to bbwidth,
 * bbheight and give him a chance to muck with the scrollbars or whatever.
 * Then, we re-layout according to the current scrollbar config.
 *
 ****************/
    
    newx = ht + sw->swindow.XOffset;
    newy = ht + sw->swindow.YOffset;
    
    desired.x = newx;
    desired.y = newy;
    desired.border_width = bw;
    desired.height = bbHeight;
    desired.width = bbWidth;
    desired.request_mode = (CWWidth | CWHeight);
    XtQueryGeometry(sw->swindow.WorkWindow, &desired, &preferred);
	    
    ht = sw->manager.shadow_thickness;

    pad = sw->swindow.pad;
    bw = preferred.border_width;

    HasHSB = ScrollBarVisible((Widget) sw->swindow.hScrollBar);
    HasVSB = ScrollBarVisible((Widget) sw->swindow.vScrollBar);

    HSBht = (HasHSB) 
                 ? sw->swindow.hScrollBar->primitive.highlight_thickness : 0;
		 
    VSBht = (HasVSB) 
         	 ? sw->swindow.vScrollBar->primitive.highlight_thickness : 0;
    
    HSBTrav = (HasHSB) 
                 ? sw->swindow.hScrollBar->primitive.traversal_on : TRUE;
		 
    VSBTrav = (HasVSB) 
         	 ? sw->swindow.vScrollBar->primitive.traversal_on : TRUE;

/****************
 *
 * Here's a cool undocumented feature. If the scrollbar is not
 * traversable,but has a highlight thickness, we assume it's something
 * that wants to draw the highlight around the workwindow, and wants to
 * have the scrollbars line up properly. Something like Scrolled Text,
 * for instance :-)
 *
 ****************/
    if (ScrollBarVisible(sw->swindow.WorkWindow) &&
        XmIsPrimitive(sw->swindow.WorkWindow))
    {
        if (HSBht && !HSBTrav) HSBht = 0;
        if (VSBht && !VSBTrav) VSBht = 0;
    }
    bbWidth = (HasVSB) ? MyWidth - (sw->swindow.vScrollBar->core.width + 
                                    (2 * VSBht) +
		  	            SB_BORDER_WIDTH + ((HSBht + bw + ht)*2) + pad )
		       : MyWidth - ((HSBht + ht+bw) * 2);

    bbHeight = (HasHSB) ? MyHeight - (sw->swindow.hScrollBar->core.height +
                                      (2 * HSBht) +
    				      SB_BORDER_WIDTH + ((VSBht + ht + bw)*2) + pad ) 
			: MyHeight - ((VSBht + ht+bw) * 2);

    if (bbWidth > MyWidth)
        bbWidth = 2;
    if (bbHeight > MyHeight)
        bbHeight = 2;

    if (HasHSB) newx += HSBht;
    if (HasVSB) newy += VSBht;

/****************
 *
 * Initialize the placement variables - these are correct for
 * bottom-right placement of the scrollbars.
 *
 ****************/
    hsbX = sw->swindow.XOffset;
    vsbY = sw->swindow.YOffset;
    vsbX = (HasVSB) ? (sw->core.width - sw->swindow.vScrollBar->core.width 
    		      - sw->swindow.WidthPad) : sw->core.width;

    hsbY = (HasHSB) ? (sw->core.height - sw->swindow.HeightPad - 
                       sw->swindow.hScrollBar->core.height) : (sw->core.height);
/****************
 *
 * Check out the scrollbar placement policy and hack the locations
 * accordingly.
 *
 ****************/
    switch (sw->swindow.Placement)
    {
	case XmTOP_LEFT:
                newx = (HasVSB) ? (sw->swindow.vScrollBar->core.width +
    		                   sw->swindow.XOffset + pad + ht +
				   HSBht) 
				: (sw->swindow.XOffset + ht + HSBht);

                newy = (HasHSB) ? (sw->swindow.hScrollBar->core.height +
    		                   sw->swindow.YOffset + pad + ht +
				   VSBht) 
				: (sw->swindow.YOffset + ht + VSBht);

		hsbX = newx - HSBht - ht;
		hsbY = sw->swindow.YOffset;
		vsbX = sw->swindow.XOffset;
		vsbY = newy - VSBht - ht;
		break;
	case XmTOP_RIGHT:
                newy = (HasHSB) ? (sw->swindow.hScrollBar->core.height +
    		                   sw->swindow.YOffset + pad + ht +
				   VSBht) 
				: (sw->swindow.YOffset + ht + VSBht);

		vsbY = newy - ht - VSBht;
		hsbY = sw->swindow.YOffset;
		break;
	case XmBOTTOM_LEFT:
                newx = (HasVSB) ? (sw->swindow.vScrollBar->core.width +
    		                   sw->swindow.XOffset + pad + ht +
				   HSBht) 
				: (sw->swindow.XOffset + ht + HSBht);
		hsbX = newx - HSBht - ht;
		vsbX = sw->swindow.XOffset;
		break;
	default:
		break;
    }


    _XmConfigureObject( sw->swindow.WorkWindow, newx, newy, bbWidth, bbHeight, bw);

    sw->swindow.hsbWidth = bbWidth  + ((HSBht + ht + bw) * 2);

    sw->swindow.vsbHeight = bbHeight  + ((VSBht + ht + bw) * 2);

    if (sw->swindow.ClipWindow)
	_XmConfigureObject( (Widget) sw->swindow.ClipWindow, newx,
			   newy, bbWidth, bbHeight, 0);

    
    sw->swindow.AreaWidth = bbWidth;
    sw->swindow.AreaHeight = bbHeight;

     if (HasVSB)
	 _XmConfigureObject( (Widget) sw->swindow.vScrollBar, vsbX, vsbY,
		     sw->swindow.vScrollBar->core.width, sw->swindow.vsbHeight,
		     SB_BORDER_WIDTH);

     if (HasHSB)
	 _XmConfigureObject( (Widget) sw->swindow.hScrollBar, hsbX, hsbY,
	 	      sw->swindow.hsbWidth, 
		      sw->swindow.hScrollBar->core.height, SB_BORDER_WIDTH);

}


/************************************************************************
 *									*
 * ConstantLayout - Layout the scrolled window for a visual policy of	*
 * XmCONSTANT.	This routine assumes a clipping window that surrounds	*
 * the workspace. 							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ConstantLayout( sw )
        XmScrolledWindowWidget sw ;
#else
ConstantLayout(
        XmScrolledWindowWidget sw )
#endif /* _NO_PROTO */
{
    int i, inc;
    Position newx,newy;
    Position hsbX, hsbY, vsbX, vsbY, clipX, clipY;
    Dimension bbWidth,bbHeight;
    Dimension viswidth,visheight;
    Dimension kidwidth,kidheight;
    Dimension MyWidth, MyHeight;
    Dimension  HSBht, VSBht, ht, bw, pad;
    Boolean HasHSB, HasVSB, HSBExists, VSBExists;
    
    /* do a sanity check first */
    if( sw->swindow.WorkWindow != NULL &&
       sw->swindow.WorkWindow->core.being_destroyed ) {
	sw->swindow.WorkWindow = NULL;
    }
    if( sw->swindow.hScrollBar != NULL &&
       sw->swindow.hScrollBar->core.being_destroyed ) {
	sw->swindow.hScrollBar = NULL;
    }
    if( sw->swindow.vScrollBar != NULL &&
       sw->swindow.vScrollBar->core.being_destroyed ) {
	sw->swindow.vScrollBar = NULL;
    }

    MyWidth = sw->core.width - sw->swindow.XOffset  - sw->swindow.WidthPad;
    MyHeight = sw->core.height - sw->swindow.YOffset  - sw->swindow.HeightPad;

    ht = sw->manager.shadow_thickness;

    HSBExists = ScrollBarVisible((Widget) sw->swindow.hScrollBar);
    VSBExists = ScrollBarVisible((Widget) sw->swindow.vScrollBar);

/****************
 *
 * If there's no visible kid, keep the scrollbars invisible.
 *
 ****************/
    if (sw->swindow.WorkWindow == NULL || 
        !(XtIsManaged(sw->swindow.WorkWindow)))	/* If no kid, just keep the */
    {						/* scrollbars invisible.    */
        if (VSBExists)
        {
            _XmMoveObject( (Widget) sw->swindow.vScrollBar,sw->core.width,
	                  sw->swindow.vScrollBar->core.y);
	    sw->swindow.vsbX = sw->swindow.vScrollBar->core.x;
        }
        if (HSBExists)
        {
	    _XmMoveObject( (Widget) sw->swindow.hScrollBar,sw->swindow.hScrollBar->core.x,
		     sw->core.height);
	    sw->swindow.hsbY = sw->swindow.hScrollBar->core.y;
        }

/* CR 5319 begin */
 	if (sw->core.width > sw->swindow.WidthPad + ht*2)
 	  sw->swindow.AreaWidth = sw->core.width - sw->swindow.WidthPad
	    - ((ht * 2) );
 	else
 	  sw->swindow.AreaWidth = 0;
 	if (sw->core.height > sw->swindow.HeightPad + ht*2)
 	  sw->swindow.AreaHeight = sw->core.height - sw->swindow.HeightPad
	    - ((ht * 2) );
 	else
 	  sw->swindow.AreaHeight = 0;
/* CR 5319 end */

        _XmConfigureObject( (Widget) sw->swindow.ClipWindow, ht + sw->swindow.XOffset, 
			  ht + sw->swindow.YOffset,
		          sw->swindow.AreaWidth, sw->swindow.AreaHeight, 0);

        (* (sw->core.widget_class->core_class.expose))
			((Widget) sw,NULL,NULL);
	return;
    }

    pad = sw->swindow.pad;
    bw = sw->swindow.WorkWindow->core.border_width;
    HSBht = (HSBExists) ? sw->swindow.hScrollBar->primitive.highlight_thickness : 0;
    VSBht = (VSBExists) ? sw->swindow.vScrollBar->primitive.highlight_thickness : 0;

    newx = sw->swindow.WorkWindow->core.x;
    newy = sw->swindow.WorkWindow->core.y;

    if (abs(newx) < sw->swindow.hmin)
	newx = -(sw->swindow.hmin);
    if (abs(newy) < sw->swindow.vmin)
	newy = -(sw->swindow.vmin);

    kidwidth = sw->swindow.WorkWindow->core.width + (bw * 2);
    kidheight = sw->swindow.WorkWindow->core.height + (bw * 2);

    bbHeight = (HSBExists) ? MyHeight - (sw->swindow.hScrollBar->core.height + 
                             SB_BORDER_WIDTH + ((HSBht + ht) * 2 + pad))
                           : MyHeight - ((VSBht + ht) * 2 );
    bbWidth = (VSBExists) ? MyWidth - (sw->swindow.vScrollBar->core.width + 
	                    SB_BORDER_WIDTH + ((VSBht + ht) * 2 + pad))
                          : MyWidth - ((HSBht + ht) * 2 );
/****************
 *
 * Look at my size and set the bb dimensions accordingly. If the kid 
 * fits easily into the space, and we can ditch the scrollbars, set
 * the bb dimension to the size of the window and flag the scrollbars
 * as false. If the kid won't fit in either direction, or if the 
 * scrollbars are constantly in the way, set the bb dimensions to the
 * minimum area and flag both scrollbars as true. Otherwise, look at 
 * the dimensions, and see if either one needs a scrollbar.
 *
 ****************/
    if (((MyHeight - (ht*2)) >= kidheight) &&
	((MyWidth - (ht*2)) >= kidwidth) &&
  	 (sw->swindow.ScrollBarPolicy == XmAS_NEEDED)) 
    {
	bbWidth = MyWidth - (ht * 2);		
	bbHeight = MyHeight - (ht * 2);
	HasVSB = HasHSB = FALSE;
    }
    else
        if ((((MyHeight - (ht*2)) < kidheight) &&
	     ((MyWidth - (ht*2)) < kidwidth))  ||
     	    (sw->swindow.ScrollBarPolicy == XmSTATIC)) 
	{	
            HasVSB = HasHSB = TRUE;
	}
	else
	{
	    HasVSB = HasHSB = TRUE;
	    if ((kidheight <= bbHeight) ||
                (!VSBExists))
	    {
		bbWidth = MyWidth - ((HSBht + ht) * 2);
                bbHeight += VSBht;
		HasVSB = FALSE;
	    }
	    if ((kidwidth <= bbWidth) ||
                (!HSBExists))
	    {
		bbHeight = MyHeight - ((VSBht + ht) * 2);
                bbWidth += HSBht;
		HasHSB = FALSE;
	    }

	}

    /* Check for underflow */
    if (bbWidth > MyWidth)
        bbWidth = 2;
    if (bbHeight > MyHeight)
        bbHeight = 2;

    HasVSB = (HasVSB && VSBExists);
    HasHSB = (HasHSB && HSBExists);

/****************
 *
 * Look at the amount visible: the workwindow dimension - position.
 * If the bb dimensions are bigger, that means the workwindow is scrolled
 * off in the bigger direction, and needs to be dragged back into the
 * visible space.
 *
 ****************/
    viswidth = kidwidth - abs(newx);

    visheight = kidheight - abs(newy);

    if (bbWidth > viswidth)
         newx =  sw->swindow.WorkWindow->core.x + (bbWidth - viswidth);
    if (newx > 0) newx = 0;
    if (abs(newx) >= kidwidth) newx = -(sw->swindow.hmin);
    
    if (bbHeight > visheight)
        newy =  sw->swindow.WorkWindow->core.y + (bbHeight - visheight);
    if (newy > 0) newy = 0;
    if (abs(newy) >= kidheight) newy = -(sw->swindow.vmin);


/****************
 *
 * Now set the size and value for each scrollbar.
 *
 ****************/
    
    if (((sw->swindow.hmax - sw->swindow.hmin) < bbWidth) &&
          sw->swindow.ScrollBarPolicy == XmSTATIC)
        sw->swindow.hExtent = sw->swindow.hmax - sw->swindow.hmin;
    else
	sw->swindow.hExtent = bbWidth;

    if (((sw->swindow.vmax - sw->swindow.vmin) < bbHeight) &&
	(sw->swindow.ScrollBarPolicy == XmSTATIC))
	sw->swindow.vExtent = sw->swindow.vmax - sw->swindow.vmin;
    else
        sw->swindow.vExtent = bbHeight;

    sw->swindow.vOrigin = abs(newy);

    if (sw->swindow.vOrigin < sw->swindow.vmin)
        sw->swindow.vOrigin = sw->swindow.vmin;

    if (sw->swindow.vOrigin > (sw->swindow.vmax - sw->swindow.vExtent))
    	sw->swindow.vExtent = sw->swindow.vmax - sw->swindow.vOrigin;

    if (VSBExists)
    {
        i = 0;
        XtSetArg(vSBArgs[i],XmNsliderSize, (XtArgVal) sw->swindow.vExtent); i++;
        XtSetArg(vSBArgs[i],XmNvalue, (XtArgVal) sw->swindow.vOrigin); i++;
        if ((inc = (bbHeight - (bbHeight / 10))) < 1)
            inc = bbHeight;
        XtSetArg(vSBArgs[i], XmNpageIncrement, (XtArgVal) inc); i++;
        if (sw->swindow.WorkWindow)
        {
            if ((inc = ((sw->swindow.WorkWindow->core.height) / 10)) < 1)
                inc = 1;
            XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) inc); i++; 
        }
        XtSetValues((Widget) sw->swindow.vScrollBar,vSBArgs,i);
    }
    sw->swindow.hOrigin = abs(newx);

    if (sw->swindow.hOrigin < sw->swindow.hmin)
        sw->swindow.hOrigin = sw->swindow.hmin;

    if (sw->swindow.hOrigin > (sw->swindow.hmax - sw->swindow.hExtent))
    	sw->swindow.hExtent = sw->swindow.hmax - sw->swindow.hOrigin;

    if (HSBExists)
    {
        i = 0;
        XtSetArg(hSBArgs[i],XmNsliderSize, (XtArgVal) sw->swindow.hExtent); i++;
        XtSetArg(hSBArgs[i],XmNvalue, (XtArgVal) sw->swindow.hOrigin); i++;
        if ((inc = (bbWidth - (bbWidth / 10))) < 1)
            inc = bbWidth;
        XtSetArg(hSBArgs[i], XmNpageIncrement, (XtArgVal) inc); i++;
        if (sw->swindow.WorkWindow)
        {
            if ((inc = ((sw->swindow.WorkWindow->core.width) / 10)) < 1)
                inc = 1;
            XtSetArg (hSBArgs[i], XmNincrement, (XtArgVal) inc); i++; 
        }
        XtSetValues((Widget) sw->swindow.hScrollBar,hSBArgs,i);
    }

    if (HasHSB)
        if (!HasVSB) 
	    sw->swindow.hsbWidth = MyWidth;
	else
	    sw->swindow.hsbWidth = bbWidth  + ((HSBht + ht) * 2);

    if (HasVSB)
        if (!HasHSB) 
	    sw->swindow.vsbHeight = MyHeight;
	else 
	    sw->swindow.vsbHeight = bbHeight  + ((VSBht + ht) * 2);	
	    
    HSBht = (HasHSB) 
                 ? sw->swindow.hScrollBar->primitive.highlight_thickness : 0;
		 
    VSBht = (HasVSB) 
         	 ? sw->swindow.vScrollBar->primitive.highlight_thickness : 0;

/****************
 *
 * Initialize the location of the scrollbars and clip window. Assume
 * bottom-right placement for the default, cause it's easy. We figure 
 * these out assuming both sb's are present, then adjust later.
 *
 ****************/
    clipX = ht + sw->swindow.XOffset + HSBht;
    clipY = ht + sw->swindow.YOffset + VSBht;
    hsbX = sw->swindow.XOffset;
    vsbY = sw->swindow.YOffset;
    vsbX = (HasVSB) ? (sw->core.width - sw->swindow.vScrollBar->core.width 
    		      - sw->swindow.WidthPad) : sw->core.width;

    hsbY = (HasHSB) ? (sw->core.height - sw->swindow.HeightPad - 
                       sw->swindow.hScrollBar->core.height) : (sw->core.height);
/****************
 *
 * Check out the scrollbar placement policy and hack the locations
 * accordingly.
 *
 ****************/
    switch (sw->swindow.Placement)
    {
	case XmTOP_LEFT:
                clipX = (HasVSB) ? (sw->swindow.vScrollBar->core.width +
    		                   sw->swindow.XOffset + pad + ht +
				   HSBht) 
				: (sw->swindow.XOffset + ht + HSBht);

                clipY = (HasHSB) ? (sw->swindow.hScrollBar->core.height +
    		                   sw->swindow.YOffset + pad + ht +
				   VSBht) 
				: (sw->swindow.YOffset + ht + VSBht);

		hsbX = clipX - HSBht - ht;
		hsbY = sw->swindow.YOffset;
		vsbX = sw->swindow.XOffset;
		vsbY = clipY - VSBht - ht;
		break;
	case XmTOP_RIGHT:
                clipY = (HasHSB) ? (sw->swindow.hScrollBar->core.height +
    		                   sw->swindow.YOffset + pad + ht +
				   VSBht) 
				: (sw->swindow.YOffset + ht + VSBht);

		vsbY = clipY - ht - VSBht;
		hsbY = sw->swindow.YOffset;
		break;
	case XmBOTTOM_LEFT:
                clipX = (HasVSB) ? (sw->swindow.vScrollBar->core.width +
    		                   sw->swindow.XOffset + pad + ht +
				   HSBht) 
				: (sw->swindow.XOffset + ht + HSBht);
		hsbX = clipX - HSBht - ht;
		vsbX = sw->swindow.XOffset;
		break;
	default:
		break;
    }

/****************
 *
 * Finally - move the widgets.
 *
 ****************/
    _XmConfigureObject( (Widget) sw->swindow.ClipWindow, clipX, clipY,
                                                         bbWidth, bbHeight, 0);
    ClearBorder(sw);

    sw->swindow.AreaWidth = bbWidth;
    sw->swindow.AreaHeight = bbHeight;

    if (HasVSB)
        _XmConfigureObject( (Widget) sw->swindow.vScrollBar, vsbX, vsbY,
		     sw->swindow.vScrollBar->core.width, sw->swindow.vsbHeight,
		     SB_BORDER_WIDTH);
    else
        if (VSBExists)
        {
            _XmConfigureObject((Widget) sw->swindow.vScrollBar, sw->core.width,
                       sw->swindow.YOffset, sw->swindow.vScrollBar->core.width,
                                       sw->swindow.vsbHeight, SB_BORDER_WIDTH);
            if (_XmFocusIsHere( (Widget) sw->swindow.vScrollBar))
                XmProcessTraversal( (Widget) sw, XmTRAVERSE_NEXT_TAB_GROUP);
        }
    if (HasHSB)
        _XmConfigureObject( (Widget) sw->swindow.hScrollBar, hsbX, hsbY,
		      sw->swindow.hsbWidth, 
		      sw->swindow.hScrollBar->core.height, SB_BORDER_WIDTH);
    else
        if (HSBExists)
        {
            _XmConfigureObject( (Widget) sw->swindow.hScrollBar,
                    sw->swindow.XOffset, sw->core.height, sw->swindow.hsbWidth,
                         sw->swindow.hScrollBar->core.height, SB_BORDER_WIDTH);
            if (_XmFocusIsHere( (Widget) sw->swindow.hScrollBar))
                XmProcessTraversal( (Widget) sw, XmTRAVERSE_NEXT_TAB_GROUP);
        }
    
    if (VSBExists) sw->swindow.vsbX = sw->swindow.vScrollBar->core.x;
    if (HSBExists) sw->swindow.hsbY = sw->swindow.hScrollBar->core.y;


    if (newx != (sw->swindow.WorkWindow->core.x + bw)  ||
	newy != (sw->swindow.WorkWindow->core.y + bw))

         _XmMoveObject( sw->swindow.WorkWindow,newx, newy );

}

/************************************************************************
 *									*
 *  ReSize								*
 *  Recompute the size of the scrolled window.				* 
 *									*
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
    XmScrolledWindowWidget	sw = (XmScrolledWindowWidget) wid ;
    sw->swindow.FromResize = TRUE;
    if (sw->swindow.VisualPolicy == XmVARIABLE)
        VariableLayout(sw);
    else
        ConstantLayout(sw);
	
    (*(sw->core.widget_class->core_class.expose))
		((Widget) sw,NULL,NULL);
    sw->swindow.FromResize = FALSE;
}



/************************************************************************
 *									*
 * SetBoxSize - set the size of the scrolled window to enclose all the	*
 * visible widgets.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetBoxSize( sw )
        XmScrolledWindowWidget sw ;
#else
SetBoxSize(
        XmScrolledWindowWidget sw )
#endif /* _NO_PROTO */
{
    Dimension	    newWidth,newHeight;
    XmScrollBarWidget	    hsb, vsb;
    Widget 	    w;
    Dimension	    hsheight,vswidth;
    Dimension       hsbht, vsbht,  ht;
    XtWidgetGeometry g;
    
    hsbht = vsbht = 0;
    ht = sw->manager.shadow_thickness * 2;
    hsb = sw->swindow.hScrollBar;
    vsb = sw->swindow.vScrollBar;
    w = sw->swindow.WorkWindow;
    
    if (ScrollBarVisible((Widget) vsb)) 
    {
	vsbht = 2 * vsb->primitive.highlight_thickness;
	vswidth = vsb->core.width + sw->swindow.pad + vsbht;
    }
    else
	vswidth = 0;

    if (ScrollBarVisible((Widget) hsb)) 
    {
	hsbht = 2 * hsb->primitive.highlight_thickness;
	hsheight = hsb->core.height + sw->swindow.pad + hsbht;
    }
    else
	hsheight = 0;
	
    if (ScrollBarVisible(w)) 
    {
        newWidth = w->core.width + (2 * w->core.border_width) + vswidth + 
	           ht + hsbht + sw->swindow.XOffset + sw->swindow.WidthPad;
        newHeight = w->core.height  + (2 * w->core.border_width) + hsheight + 
	            ht + vsbht + sw->swindow.YOffset + sw->swindow.HeightPad;
    }
    else
    {
	newWidth = sw->core.width;	
        newHeight = sw->core.height;
    }
/****************
 *
 * If we're not realized, and we have a width and height, use it.
 *
 *****************/
     if (!XtIsRealized(sw))
     {
         if (sw->swindow.GivenWidth)
             newWidth = sw->swindow.GivenWidth;
         if (sw->swindow.GivenHeight)
             newHeight = sw->swindow.GivenHeight;
     }    
    
    g.request_mode = CWWidth | CWHeight ;
    g.width = newWidth ;
    g.height = newHeight ;
    if ((_XmMakeGeometryRequest((Widget) sw, &g) == XtGeometryYes) &&
	(!((sw->core.width == g.width) && (sw->core.height == g.height))))
	(* (sw->core.widget_class->core_class.resize)) ((Widget) sw) ;
}

/************************************************************************
 *									*
 *  GeometryManager							*
 *									*
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
GeometryManager(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
    XmScrolledWindowWidget sw;
    XmScrollBarWidget hsb,vsb;
    XtGeometryResult  retval;
    Dimension	    newWidth,newHeight;
    Dimension	    newWidthRet,newHeightRet;
    Dimension       hsheight,vswidth;
    Dimension       hsbht, vsbht, bw, ht;
    int             i, hsmax, vsmax;
    Boolean         Fake = FALSE;
    XtWidgetGeometry  parent_request ;

#define QUERYONLY (request->request_mode & XtCWQueryOnly) 

    hsbht = vsbht = 0;
    reply->request_mode = 0;

    sw = (XmScrolledWindowWidget ) w->core.parent;
    while (!XmIsScrolledWindow(sw))
        sw = (XmScrolledWindowWidget )sw->core.parent;

/* DA requests are kinda OK now... */
/*    if (w == (Widget)sw->swindow.ClipWindow)
       return(XtGeometryNo); */

/****************
 *
 * Disallow any X or Y changes - we need to manage the child locations.
 *
 ****************/
    if ((request -> request_mode & CWX || request -> request_mode & CWY) &&
        (w != (Widget)sw->swindow.ClipWindow))
    {
	if (request->request_mode & CWWidth || 
	    request->request_mode & CWHeight)
	{
	    reply->x = w->core.x;
	    reply->y = w->core.y;
	    reply->request_mode = request->request_mode & ~(CWX | CWY);
	    if (request->request_mode & CWWidth)
	        reply->width = request->width;
	    if (request->request_mode & CWHeight)
	        reply->height = request->height;
	    return( XtGeometryAlmost);
	}
	else
	    return( XtGeometryNo);
    }
    	    
    ht = sw->manager.shadow_thickness * 2;
    hsb = sw->swindow.hScrollBar;
    vsb = sw->swindow.vScrollBar;
/****************
 *
 * Carry forward that ugly wart for scrolled primitives...
 *
 ****************/
    
    if (ScrollBarVisible((Widget) vsb))
    {
	vsbht = 2 * vsb->primitive.highlight_thickness;
        if (ScrollBarVisible(sw->swindow.WorkWindow)        &&
            XmIsPrimitive(sw->swindow.WorkWindow)           &&
	    !(sw->swindow.vScrollBar->primitive.traversal_on))
	    vsbht = 0;
	vswidth = vsb->core.width + sw->swindow.pad + vsbht;
    }
    else
	vswidth = 0;

    if (ScrollBarVisible((Widget) hsb))
    {
	hsbht = 2 * hsb->primitive.highlight_thickness;
        if (ScrollBarVisible(sw->swindow.WorkWindow)        &&
            XmIsPrimitive(sw->swindow.WorkWindow)           &&
	    !(sw->swindow.hScrollBar->primitive.traversal_on))
	    hsbht = 0;
	hsheight = hsb->core.height + sw->swindow.pad + hsbht;
    }
    else
	hsheight = 0;
	
/****************
 *
 * First, see if its from a visible scrollbar. If so, look for a height
 * req from the vsb, or a width from the hsb, and say no, or almost if 
 * they asked for both dimensions. Else update our height & 
 * width and relayout. If the requesting scrollbar is invisible,
 * grant its request and take care of the layout when a later operation
 * forces it to become visible.
 *
 ****************/
 
    if (w == (Widget) hsb || w == (Widget) vsb)
    {
	if ((w  == (Widget) vsb) && (request->request_mode & CWHeight))
        {
            if (request->request_mode & CWWidth)
            {
	        reply->request_mode |= (CWHeight);
	        reply->height = w->core.height;
	        return( XtGeometryAlmost);
            }
            else
                return( XtGeometryNo);
        }

	if ((w  == (Widget) hsb) && (request->request_mode & CWWidth))
        {
            if (request->request_mode & CWHeight)
            {
	        reply->request_mode |= (CWWidth);
	        reply->width = w->core.width;
	        return(XtGeometryAlmost);
            }
            else
                return(XtGeometryNo);
        }

        if (((w == (Widget) hsb) && 
	     ((Dimension) w->core.y < sw->core.height)) || 
            ((w == (Widget) vsb) && 
	     ((Dimension) w->core.x < sw->core.width)))
        {
	    if(request->request_mode & CWWidth)
	        newWidth = sw->core.width - w->core.width + request->width;
            else
                newWidth = sw->core.width ;
	    if(request->request_mode & CWHeight)
	        newHeight = sw->core.height - w->core.height + request->height;
	    else
                newHeight = sw->core.height ;
            if (request->request_mode & CWBorderWidth)
            {
                newHeight = newHeight - (w->core.border_width * 2) 
                                      + (request->border_width * 2);
                newWidth  =  newWidth - (w->core.border_width * 2) 
                                      + (request->border_width * 2);
            }

	    parent_request.request_mode = CWWidth | CWHeight;
	    if (QUERYONLY) parent_request.request_mode |= XtCWQueryOnly;
	    parent_request.width = newWidth ;
	    parent_request.height = newHeight ;
            retval = XtMakeGeometryRequest((Widget) sw, &parent_request, NULL);
        }
        else
        {
            Fake = TRUE;
            retval = XtGeometryYes;
        }

        if (retval == XtGeometryYes)
	{
	    if (!QUERYONLY) {
		if (request->request_mode & CWBorderWidth)
		    w->core.border_width = request->border_width;
		if(request->request_mode & CWWidth)
		    w->core.width = request->width;
		if(request->request_mode & CWHeight)
		    w->core.height = request->height;
	    
		if (!Fake)
		    (* (sw->core.widget_class->core_class.resize))
			((Widget) sw) ;
	    }
	}

	return(retval);
    }

    if (sw->swindow.VisualPolicy == XmVARIABLE)
    {
	if (request->request_mode & CWBorderWidth)
	    bw = (request->border_width * 2);
	else
	    bw = (w->core.border_width * 2);

	if(request->request_mode & CWWidth)
	    newWidth = request->width + vswidth + ht + bw + hsbht
	               + sw->swindow.XOffset + sw->swindow.WidthPad ;
	else
            newWidth = w->core.width + vswidth + ht + bw + hsbht
	               + sw->swindow.XOffset + sw->swindow.WidthPad ;

	if(request->request_mode & CWHeight)
	    newHeight = request->height + hsheight + ht + bw + vsbht 
	                + sw->swindow.YOffset + sw->swindow.HeightPad ;
	else
            newHeight = w->core.height + hsheight + ht + bw + vsbht 
	                + sw->swindow.YOffset + sw->swindow.HeightPad;

        parent_request.request_mode = CWWidth | CWHeight;
	if (QUERYONLY) parent_request.request_mode |= XtCWQueryOnly;
	parent_request.width = newWidth ;
	parent_request.height = newHeight ;
        if (QUERYONLY)
  	    retval = XtMakeGeometryRequest((Widget) sw, &parent_request, NULL);
	else 
	{
	    retval = XtMakeResizeRequest((Widget) sw, newWidth, newHeight,
					  &newWidthRet, &newHeightRet);
	    if (retval == XtGeometryAlmost)
  	        retval = XtMakeResizeRequest((Widget) sw, newWidthRet, newHeightRet,
					  NULL, NULL);
	}	    
        if (retval == XtGeometryYes)
	{
	    if (!QUERYONLY) {
		if (request->request_mode & CWBorderWidth)
		    w->core.border_width = request->border_width;
		if(request->request_mode & CWWidth)
		    w->core.width = request->width;
		if(request->request_mode & CWHeight)
		    w->core.height = request->height;
		if (!XmIsMainWindow(sw))
		    (* (sw->core.widget_class->core_class.resize))
			((Widget) sw) ;
	    }
	}
	return(retval);
    }
    else				
	/* Constant - let the workarea grow and */
	/* update the scrollbar resources.	*/
    {
        if (QUERYONLY) return XtGeometryYes ;

	if (w == (Widget)sw->swindow.ClipWindow)
  	    w = (Widget)sw->swindow.WorkWindow;		/* YEEOOOWWW! */
	    
	if (request->request_mode & CWBorderWidth)
	{
	    w->core.border_width = request->border_width;
	}

	bw = (w->core.border_width * 2);

	if (request->request_mode & CWWidth)
	{
 	     w->core.width = request->width;
	     hsmax = request->width + bw;
	}
	else
	     hsmax = w->core.width + bw;

	if (request->request_mode & CWHeight)
	{
	    w->core.height = request->height;
	    vsmax = request->height + bw;
	}
	else
	     vsmax = w->core.height + bw;

	if (request->request_mode & (CWWidth | CWBorderWidth))
	{
	    i = 0;
	    sw->swindow.hmax = hsmax;
	    if (sw->swindow.hExtent > (sw->swindow.hmax - sw->swindow.hmin))
	    	sw->swindow.hExtent = sw->swindow.hmax - sw->swindow.hmin;

	    if (sw->swindow.hOrigin > (sw->swindow.hmax - sw->swindow.hExtent))
	    	sw->swindow.hOrigin = sw->swindow.hmax - sw->swindow.hExtent;

	    XtSetArg (hSBArgs[i],XmNmaximum, (XtArgVal) hsmax); i++;
            XtSetArg (hSBArgs[i],XmNsliderSize, (XtArgVal) sw->swindow.hExtent); i++;
            XtSetArg (hSBArgs[i],XmNvalue, (XtArgVal) sw->swindow.hOrigin); i++;
            if (ScrollBarVisible((Widget) sw->swindow.hScrollBar))
	        XtSetValues((Widget ) sw->swindow.hScrollBar,hSBArgs,i);
	}

	if (request->request_mode & (CWHeight | CWBorderWidth))
	{
	    sw->swindow.vmax = vsmax;
	    i = 0;
	    if (sw->swindow.vExtent > (sw->swindow.vmax - sw->swindow.vmin))
	    	sw->swindow.vExtent = sw->swindow.vmax - sw->swindow.vmin;

	    if (sw->swindow.vOrigin > (sw->swindow.vmax - sw->swindow.vExtent))
	    	sw->swindow.vOrigin = sw->swindow.vmax - sw->swindow.vExtent;

	    XtSetArg (vSBArgs[i],XmNmaximum,(XtArgVal) vsmax); i++;
            XtSetArg (vSBArgs[i],XmNsliderSize, (XtArgVal) sw->swindow.vExtent); i++;
            XtSetArg (vSBArgs[i],XmNvalue, (XtArgVal) sw->swindow.vOrigin); i++;
            if (ScrollBarVisible((Widget) sw->swindow.vScrollBar))
	        XtSetValues((Widget ) sw->swindow.vScrollBar,vSBArgs,i);
	}
	(* (sw->core.widget_class->core_class.resize))
		((Widget) sw) ;
	return (XtGeometryYes);
    }
}



/************************************************************************
 *									*
 *  ChangeManaged - called whenever there is a change in the managed	*
 *		    set.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    if (sw->swindow.FromResize)
        return;

    if (sw->swindow.VisualPolicy == XmVARIABLE)
	SetBoxSize(sw);
    _XmInitializeScrollBars( (Widget) sw);
    if (sw->swindow.VisualPolicy == XmVARIABLE)
        VariableLayout(sw);
    else
	ConstantLayout(sw);
	
	(* (sw->core.widget_class->core_class.expose))
		((Widget) sw,NULL,NULL);
    _XmNavigChangeManaged( (Widget) sw);
}
     

/************************************************************************
 *									*
 *  QueryProc - Query proc for the scrolled window.			*
 *									*
 *  This routine will examine the geometry passed in an recalculate our	*
 *  width/height as follows:  if either the width or height is set, we	*
 *  take that as our new size, and figure out the other dimension to be	*
 *  the minimum size we need to be to display the entire child.  Note	*
 *  that this will only happen in auto mode, with as_needed display 	*
 *  policy.								*
 *									*
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryProc( wid, request, ret )
        Widget wid ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *ret ;
#else
QueryProc(
        Widget wid,
        XtWidgetGeometry *request,
        XtWidgetGeometry *ret )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget sw = (XmScrolledWindowWidget) wid ;
    Dimension	       MyWidth, MyHeight, KidHeight, KidWidth;
    Widget 	       w;
    XmScrollBarWidget  hsb, vsb;
    Dimension          hsheight,vswidth;
    Dimension	       hsbht, vsbht, ht;
    XtWidgetGeometry   desired, preferred;

    XtGeometryResult retval = XtGeometryYes;

    ret -> request_mode = 0;
    w = sw->swindow.WorkWindow;
    hsb = sw->swindow.hScrollBar;
    vsb = sw->swindow.vScrollBar;

/****************
 *
 * If the request mode is zero, fill out out default height & width.
 *
 ****************/
    if (request->request_mode == 0)
    {
        if ((sw->swindow.VisualPolicy == XmCONSTANT) ||
            (!sw->swindow.WorkWindow))
        {
	    ret->width = sw->core.width;	
            ret->height = sw->core.height;
    	    ret->request_mode = (CWWidth | CWHeight);
	    return (XtGeometryAlmost);
        }
        hsbht = vsbht = 0;
        ht = sw->manager.shadow_thickness * 2;

        desired.request_mode = 0;
        XtQueryGeometry(sw->swindow.WorkWindow, &desired, &preferred);

        KidWidth = preferred.width;
        KidHeight = preferred.height;
        if (ScrollBarVisible((Widget) vsb)) 
        {
            vsbht = 2 * vsb->primitive.highlight_thickness;
	    vswidth = vsb->core.width + sw->swindow.pad + vsbht;
        }
        else
	    vswidth = 0;

        if (ScrollBarVisible((Widget) hsb)) 
        {
	    hsbht = 2 * hsb->primitive.highlight_thickness;
	    hsheight = hsb->core.height + sw->swindow.pad + hsbht;
        }
        else
	    hsheight = 0;
	
        if (ScrollBarVisible(w)) 
        {
            ret->width = KidWidth + (2 * w->core.border_width) + vswidth + 
	           ht + hsbht + sw->swindow.XOffset + sw->swindow.WidthPad;
            ret->height = KidHeight  + (2 * w->core.border_width) + hsheight + 
	            ht + vsbht + sw->swindow.YOffset + sw->swindow.HeightPad;
        }
        else
        {
	    ret->width = sw->core.width;	
            ret->height = sw->core.height;
        }
        ret->request_mode = (CWWidth | CWHeight);
	return (XtGeometryAlmost);
    }

/****************
 *
 * If app mode, or static scrollbars, or no visible kid, 
 * accept the new size, and return our current size for any 
 * missing dimension.
 *
 ****************/
    if ((sw->swindow.ScrollPolicy == XmAPPLICATION_DEFINED) ||
	(!ScrollBarVisible(w)))
    {
        if (!(request -> request_mode & CWWidth))
        {
    	    ret->request_mode |= CWWidth;
            ret->width = sw->core.width;
	    retval = XtGeometryAlmost;
	}
        if (!(request -> request_mode & CWHeight))
        {
            ret->request_mode |= CWHeight;
            ret->height = sw->core.height;
            retval = XtGeometryAlmost;
        }
        return(retval);
    }

/****************
 *
 * Else look for the specified dimension, and set the other size so that we
 * just enclose the child. 
 * If the new size would cause us to lose the scrollbar, figure
 * out the other dimension as well and return that, too.
 *
 ****************/

    hsbht = vsbht = 0;
    ht = sw->manager.shadow_thickness * 2;
    hsb = sw->swindow.hScrollBar;
    vsb = sw->swindow.vScrollBar;

    if ((request -> request_mode & CWWidth) &&
        (request -> request_mode & CWHeight)&&
        (sw->swindow.ScrollBarPolicy == XmAS_NEEDED))
    {
        ret->height = w->core.height + (2 * w->core.border_width) +
	              ht + sw->swindow.YOffset + sw->swindow.HeightPad;
        ret->width = w->core.width + (2 * w->core.border_width) +
                     ht + sw->swindow.XOffset + sw->swindow.WidthPad;
        ret->request_mode |= (CWWidth | CWHeight);
        return(XtGeometryAlmost);
    }

    if (request -> request_mode & CWHeight)
    {

        MyHeight = request->height - sw->swindow.YOffset - 
	           sw->swindow.HeightPad - ht;

        if (((w->core.height + (2 * w->core.border_width)) > MyHeight) ||
            (sw->swindow.ScrollBarPolicy == XmSTATIC))
        {
   	    vsbht = 2 * vsb->primitive.highlight_thickness;
	    vswidth = vsb->core.width + sw->swindow.pad;
        }
        else
        {
	    vswidth = 0;
            ret->request_mode |= CWHeight;
            ret->height = w->core.height + (2 * w->core.border_width) +
	             ht + sw->swindow.YOffset + sw->swindow.HeightPad;
        }

        ret->request_mode |= CWWidth;
        ret->width = w->core.width + (2 * w->core.border_width) + vswidth + 
	             ht + vsbht + sw->swindow.XOffset + sw->swindow.WidthPad;
	retval = XtGeometryAlmost;
    }

    if (request -> request_mode & CWWidth)
    {
        MyWidth = request->width - sw->swindow.XOffset - 
	           sw->swindow.WidthPad - ht;

        if (((w->core.width + (2 * w->core.border_width)) > MyWidth) ||
            (sw->swindow.ScrollBarPolicy == XmSTATIC))
        {
   	    hsbht = 2 * hsb->primitive.highlight_thickness;
	    hsheight = hsb->core.height + sw->swindow.pad;
        }
        else
        {
	    hsheight = 0;
            ret->request_mode |= CWWidth;
            ret->width = w->core.width + (2 * w->core.border_width) +
	                 ht + sw->swindow.XOffset + sw->swindow.WidthPad;
        }

        ret->request_mode |= CWHeight;
        ret->height = w->core.height + (2 * w->core.border_width) + hsheight + 
	             ht + hsbht + sw->swindow.YOffset + sw->swindow.HeightPad;
	retval = XtGeometryAlmost;
    }

    return(retval);
}


/************************************************************************
 *									*
 *  SetValues								*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        XmScrolledWindowWidget current = (XmScrolledWindowWidget) cw ;
        XmScrolledWindowWidget request = (XmScrolledWindowWidget) rw ;
        XmScrolledWindowWidget new_w = (XmScrolledWindowWidget) nw ;
    Boolean Flag = FALSE;

    if ((new_w->swindow.WidthPad != current->swindow.WidthPad) ||
        (new_w->swindow.HeightPad != current->swindow.HeightPad) ||
        (new_w->manager.shadow_thickness != current->manager.shadow_thickness) ||
        (new_w->swindow.pad != current->swindow.pad))
	{
            new_w->swindow.XOffset = new_w->swindow.WidthPad;
            new_w->swindow.YOffset = new_w->swindow.HeightPad;
	    Flag = TRUE;
	}


    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                               new_w->swindow.ScrollBarPolicy, (Widget) new_w)    )
    {
	new_w->swindow.ScrollBarPolicy = current->swindow.ScrollBarPolicy;
    }	

    if (new_w->swindow.ScrollBarPolicy != current->swindow.ScrollBarPolicy)
       Flag = TRUE;

    if (request->swindow.ScrollPolicy != current->swindow.ScrollPolicy)
    {
	_XmWarning( (Widget) new_w, SWMessage6);
	new_w->swindow.ScrollPolicy = current->swindow.ScrollPolicy;
    }
    if (request->swindow.VisualPolicy != current->swindow.VisualPolicy)
    {
	_XmWarning( (Widget) new_w, SWMessage7);
	new_w->swindow.VisualPolicy = current->swindow.VisualPolicy;
    }
    if ((new_w->swindow.VisualPolicy == XmVARIABLE) &&
	(request->swindow.ScrollBarPolicy == XmAS_NEEDED))
    {
	_XmWarning( (Widget) new_w, SWMessage8);
	new_w->swindow.ScrollBarPolicy = XmSTATIC;
    }
   if (new_w->swindow.ScrollPolicy == XmAUTOMATIC)
    {
    	if (new_w->swindow.hScrollBar != current->swindow.hScrollBar)
	{
	    _XmWarning( (Widget) new_w, SWMessage9);
	    new_w->swindow.hScrollBar = current->swindow.hScrollBar;	
	}
    	if (new_w->swindow.vScrollBar != current->swindow.vScrollBar)
	{
	    _XmWarning( (Widget) new_w, SWMessage9);
	    new_w->swindow.vScrollBar = current->swindow.vScrollBar;	
	}

    }

   if (new_w->swindow.ScrollPolicy == XmAPPLICATION_DEFINED)
    {
    	if (new_w->swindow.hScrollBar != current->swindow.hScrollBar)
            if (new_w->swindow.hScrollBar != NULL)
              Flag = TRUE;
            else
	       new_w->swindow.hScrollBar = current->swindow.hScrollBar;	

    	if (new_w->swindow.vScrollBar != current->swindow.vScrollBar)
            if (new_w->swindow.vScrollBar != NULL)
              Flag = TRUE;
            else
	       new_w->swindow.vScrollBar = current->swindow.vScrollBar;	
    }

    if (new_w->swindow.ClipWindow != current->swindow.ClipWindow)
    {
	_XmWarning( (Widget) new_w, SWMessage10);
	new_w->swindow.ClipWindow = current->swindow.ClipWindow;
    }
    
    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_PLACEMENT,
                                     new_w->swindow.Placement, (Widget) new_w)    )
    {
	new_w->swindow.Placement = current->swindow.Placement;
    }	

    if (new_w->swindow.WorkWindow != current->swindow.WorkWindow) {
	if (current->swindow.WorkWindow != NULL)
	    XtRemoveCallback(current->swindow.WorkWindow, XmNdestroyCallback,
			     KidKilled, NULL);
	if (new_w->swindow.WorkWindow != NULL)
	    XtAddCallback(new_w->swindow.WorkWindow, XmNdestroyCallback,
			  KidKilled, NULL);
    }
    
    if ((new_w->swindow.Placement != current->swindow.Placement)  ||
	(new_w->swindow.hScrollBar != current->swindow.hScrollBar)||
	(new_w->swindow.vScrollBar != current->swindow.vScrollBar)||
	(new_w->swindow.WorkWindow != current->swindow.WorkWindow)||
	(new_w->swindow.pad != current->swindow.pad))
        {
            if ((new_w->swindow.hScrollBar != current->swindow.hScrollBar) &&
                (current->swindow.hScrollBar != NULL))
                    if (XtIsRealized(current->swindow.hScrollBar))
                        XtUnmapWidget(current->swindow.hScrollBar);
                    else
                        XtSetMappedWhenManaged(
                                  (Widget) current->swindow.hScrollBar, FALSE);
            if ((new_w->swindow.vScrollBar != current->swindow.vScrollBar) &&
                (current->swindow.vScrollBar != NULL))
                    if (XtIsRealized(current->swindow.vScrollBar))
                        XtUnmapWidget(current->swindow.vScrollBar);
                    else
                        XtSetMappedWhenManaged(
                                  (Widget) current->swindow.vScrollBar, FALSE);
	    if ((new_w->swindow.hScrollBar != current->swindow.hScrollBar)||
	        (new_w->swindow.vScrollBar != current->swindow.vScrollBar)||
	        (new_w->swindow.WorkWindow != current->swindow.WorkWindow))
                _XmInitializeScrollBars( (Widget) new_w);
           SetBoxSize(new_w);
           Flag = TRUE;
        }
      
    return (Flag);
 }

/************************************************************************
 *									*
 * Spiffy API Functions							*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * XmScrolledWindowSetAreas - set a new widget set.			*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmScrolledWindowSetAreas( w, hscroll, vscroll, wregion )
        Widget w ;
        Widget hscroll ;
        Widget vscroll ;
        Widget wregion ;
#else
XmScrolledWindowSetAreas(
        Widget w,
        Widget hscroll,
        Widget vscroll,
        Widget wregion )
#endif /* _NO_PROTO */
{
    XmScrolledWindowWidget sw = (XmScrolledWindowWidget) w;    
    if (sw->swindow.WorkWindow != wregion)
    {
       if (sw->swindow.WorkWindow != NULL)
	{
           XtRemoveCallback(sw->swindow.WorkWindow, XmNdestroyCallback,
                            KidKilled, NULL);
	}
       if (wregion != NULL)
           XtAddCallback(wregion, XmNdestroyCallback, KidKilled, NULL);

	sw->swindow.WorkWindow = wregion;
    }
    if (sw->swindow.ScrollPolicy != XmAUTOMATIC)
    {
        if ((sw->swindow.hScrollBar) && 
            (hscroll != (Widget )sw->swindow.hScrollBar))
            if (XtIsRealized(sw->swindow.hScrollBar))
                XtUnmapWidget(sw->swindow.hScrollBar);
            else
                XtSetMappedWhenManaged((Widget) sw->swindow.hScrollBar, FALSE);
            
        if ((sw->swindow.vScrollBar) && 
            (vscroll != (Widget )sw->swindow.vScrollBar))
            if (XtIsRealized(sw->swindow.vScrollBar))
                XtUnmapWidget(sw->swindow.vScrollBar);
            else
                XtSetMappedWhenManaged((Widget) sw->swindow.vScrollBar, FALSE);
          
	sw->swindow.hScrollBar = (XmScrollBarWidget) hscroll;
	sw->swindow.vScrollBar = (XmScrollBarWidget) vscroll;
        _XmInitializeScrollBars( (Widget) sw);
        SetBoxSize(sw);
    }
    else
        _XmInitializeScrollBars( (Widget) sw);

    if (XtIsRealized(sw))
		(* (sw->core.widget_class->core_class.resize))
			((Widget) sw) ;
}


/************************************************************************
 *									*
 * XmCreateScrolledWindow - hokey interface to XtCreateWidget.		*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateScrolledWindow( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateScrolledWindow(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{

    return ( XtCreateWidget( name, 
			     xmScrolledWindowWidgetClass, 
			     parent, 
			     args, 
			     argCount ) );
}


/************************************************************************
 *									*
 * XmScrollVisible -                                                    *
 *									*
 ************************************************************************/
void
#ifdef _NO_PROTO
XmScrollVisible(scrw, wid, hor_margin, ver_margin)
Widget      	scrw;
Widget          wid;
Dimension       hor_margin, ver_margin;
#else
XmScrollVisible(
		Widget      	scrw,
		Widget          wid,
		Dimension       hor_margin, 
		Dimension       ver_margin)
#endif
/*********************
 * A function that makes visible a unvisible or partially visible descendant 
 * of an AUTOMATIC scrolledwindow workwindow.
 *********************
 * - scrw is the ScrolledWindow whose workwindow act as the origin of the move.
 * - wid is the widget to be made visible.
 * - hor_margin, ver_margin are the margins between the widget in its new 
 *   position and the scrolledwindow clipwindow.
**********************/
{
    XmScrolledWindowWidget sw = (XmScrolledWindowWidget) scrw ;
    register 
    Position newx, newy,      /* new workwindow position */
             wx, wy  ;        /* current workwindow position */
    register 
    unsigned short tw, th,         /* widget sizes */
              cw, ch ;        /* clipwindow sizes */
    Position dx, dy ;         /* position inside the workwindow */

    /* check param */
    if (!((scrw) && 
	  (wid) &&
	  (XmIsScrolledWindow(scrw)) &&
	  (sw->swindow.ScrollPolicy == XmAUTOMATIC) &&
	  (sw->swindow.WorkWindow))) {
	_XmWarning(scrw, SVMessage1);
	return ;
    }

    /* we need to know the position of the widget relative to the topmost
       workwindow (the workwindow of scrw), so we use 2 XtTranslateCoords,
       but since most of the time, this function will be called
       without nested managers, test this case first */
    
    if (XtWindow(XtParent(wid)) == XtWindow((Widget) sw->swindow.WorkWindow)) {
	dx = XtX(wid) ;
	dy = XtY(wid) ;
    } else {
	Position src_x, src_y, dst_x, dst_y ;

	XtTranslateCoords(wid, 0, 0, &src_x, &src_y);
	XtTranslateCoords(sw->swindow.WorkWindow, 0, 0, &dst_x, &dst_y);
	dx = src_x - dst_x ;
	dy = src_y - dst_y ;
    }

    /* get the other needed positions and sizes */
    cw = XtWidth((Widget) sw->swindow.ClipWindow) ;
    ch = XtHeight((Widget) sw->swindow.ClipWindow) ;
    wx = XtX((Widget) sw->swindow.WorkWindow) ;  /* in the clipwindow */
    wy = XtY((Widget) sw->swindow.WorkWindow) ;  /* in the clipwindow */
    tw = XtWidth(wid) ;
    th = XtHeight(wid) ;
    
    /* find out the zone where the widget lies and set newx,newy (neg) for
       the workw */
    /* if the widget is bigger than the clipwindow, we put it on
       the left, top or top/left, depending on the zone it was */

    if (dy < -wy) {                              /* North */
	newy = dy - (Position)ver_margin ; /* stuck it on top + margin */
    } else 
    if ((dy < (-wy + ch)) && ((dy + th) <= (-wy + ch))) {
	newy = -wy ;  /* in the middle : don't move y */
    } else {                                     /* South */
	if (th > ch)
	    newy = dy -  (Position)ver_margin ; 
	                  /* stuck it on top if too big */
	else
	    newy = dy - ch + th +  (Position)ver_margin;
    } 
	
    if (dx < -wx) {                              /* West */
	newx = dx -  (Position)hor_margin ; /* stuck it on left + margin */
    } else
    if ((dx < (-wx + cw)) && ((dx + tw) <= (-wx + cw))) {
	newx = -wx ;  /* in the middle : don't move x */
    } else {                                     /* East */
	if (tw > cw)
	    newx = dx -  (Position)hor_margin ; 
	                 /* stuck it on left if too big */
	else
	    newx = dx - cw + tw +  (Position)hor_margin;
    } 

    /* we also have to test that because the margins can be huge */
    if (newx > ( (Position)XtWidth((Widget) sw->swindow.WorkWindow) - cw)) 
	newx =  (Position)XtWidth((Widget) sw->swindow.WorkWindow) - cw ;
    if (newy > ( (Position)XtHeight((Widget) sw->swindow.WorkWindow) - ch)) 
	newy =  (Position)XtHeight((Widget) sw->swindow.WorkWindow) - ch ;
    if (newx < 0) newx = 0 ;
    if (newy < 0) newy = 0 ;

    /* move the workwindow, and make the widget appears if not yet visible */
    if (newx != -wx) 
	XmScrollBarSetValues((Widget)sw->swindow.hScrollBar, newx, 
			 sw->swindow.hScrollBar->scrollBar.slider_size, 
			 sw->swindow.hScrollBar->scrollBar.increment, 
			 sw->swindow.hScrollBar->scrollBar.page_increment, 
			 True);
    if (newy != -wy)
	XmScrollBarSetValues((Widget)sw->swindow.vScrollBar, newy, 
			 sw->swindow.vScrollBar->scrollBar.slider_size, 
			 sw->swindow.vScrollBar->scrollBar.increment, 
			 sw->swindow.vScrollBar->scrollBar.page_increment, 
			 True);
}
