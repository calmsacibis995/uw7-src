#pragma ident	"@(#)m1.2libs:Xm/Sash.c	1.3"
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
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#include <X11/cursorfont.h>
#include "XmI.h"
#include <Xm/SashP.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/DisplayP.h>

#define defTranslations		_XmSash_defTranslations
#define SASHSIZE 10

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassPartInitialize() ;
static void ClassInitialize() ;
static void Initialize() ;
static void HighlightSash() ;
static void UnhighlightSash() ;
static XmNavigability WidgetNavigable() ;
static void SashFocusIn() ;
static void SashFocusOut() ;
static void SashAction() ;
static void Realize() ;
static void Redisplay() ;
static void SashDisplayDestroyCallback ();

#else

static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void ClassInitialize( void ) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void HighlightSash( 
                        Widget sash) ;
static void UnhighlightSash( 
                        Widget sash) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;
static void SashFocusIn( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SashFocusOut( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SashAction( 
                        Widget widget,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Realize( 
                        register Widget w,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Redisplay( 
                        Widget w,
                        XEvent *event,
                        Region region) ;

static void SashDisplayDestroyCallback ( 
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data );
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtResource resources[] = {
   {XmNborderWidth, XmCBorderWidth, XmRHorizontalDimension, sizeof(Dimension),
      XtOffsetOf( struct _XmSashRec, core.border_width), XmRImmediate, (XtPointer) 0},

   {XmNcallback, XmCCallback, XmRCallback, sizeof(XtCallbackList), 
      XtOffsetOf( struct _XmSashRec, sash.sash_action), XmRPointer, NULL},

   { XmNnavigationType, XmCNavigationType, XmRNavigationType,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
     XmRImmediate, (XtPointer) XmSTICKY_TAB_GROUP},
};


static XtActionsRec actionsList[] =
{
  {"SashAction",	SashAction},
  {"SashFocusIn",	SashFocusIn},
  {"SashFocusOut",	SashFocusOut},
};


static XmBaseClassExtRec SashBaseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,				/* InitializePrehook	*/
    NULL,				/* SetValuesPrehook	*/
    NULL,				/* InitializePosthook	*/
    NULL,				/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    NULL,				/* secondaryCreate	*/
    NULL,		                /* getSecRes data	*/
    { 0 },				/* fastSubclass flags	*/
    NULL,				/* get_values_prehook	*/
    NULL,				/* get_values_posthook	*/
    NULL,                               /* classPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    WidgetNavigable,                    /* widgetNavigable      */
    XmInheritFocusChange,               /* focusChange          */
};

externaldef(xmsashclassrec) XmSashClassRec xmSashClassRec = {
   {
/* core class fields */
    /* superclass         */   (WidgetClass) &xmPrimitiveClassRec,
    /* class name         */   "XmSash",
    /* size               */   sizeof(XmSashRec),
    /* class initialize   */   ClassInitialize,
    /* class_part_init    */   ClassPartInitialize,
    /* class_inited       */   FALSE,
    /* initialize         */   Initialize,
    /* initialize_hook    */   NULL,
    /* realize            */   Realize,
    /* actions            */   actionsList,
    /* num_actions        */   XtNumber(actionsList),
    /* resourses          */   resources,
    /* resource_count     */   XtNumber(resources),
    /* xrm_class          */   NULLQUARK,
    /* compress_motion    */   TRUE,
    /* compress_exposure  */   XtExposeCompressMaximal,
    /* compress_enter/lv  */   TRUE,
    /* visible_interest   */   FALSE,
    /* destroy            */   NULL,
    /* resize             */   NULL,
    /* expose             */   Redisplay,
    /* set_values         */   NULL,
    /* set_values_hook    */   NULL,
    /* set_values_almost  */   XtInheritSetValuesAlmost,
    /* get_values_hook    */   NULL,
    /* accept_focus       */   NULL,
    /* version            */   XtVersion,
    /* callback_private   */   NULL,
    /* tm_table           */   defTranslations,
    /* query_geometry     */   NULL,
    NULL,                             /* display_accelerator   */
    (XtPointer)&SashBaseClassExtRec, /* extension             */
   },

   {
      XmInheritWidgetProc,   /* Primitive border_highlight   */
      XmInheritWidgetProc,   /* Primitive border_unhighlight */
      NULL,         /* translations                 */
      NULL,         /* arm_and_activate             */
      NULL,	    /* get resources                */
      0,	    /* num get_resources            */
      NULL,         /* extension                    */
   },

   {
      (XtPointer) NULL,         /* extension        */
   }

};

externaldef(xmsashwidgetclass) WidgetClass xmSashWidgetClass =
					         (WidgetClass) &xmSashClassRec;

/************************************************************************
 *
 *  ClassPartInitialize
 *    Set up the fast subclassing for the widget.
 *
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
   _XmFastSubclassInit(wc, XmSASH_BIT);
}

/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the primitive part of class structure with 
 *    routines to do special highlight & unhighlight for Sash.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   xmSashClassRec.primitive_class.border_highlight =
                  HighlightSash;
   xmSashClassRec.primitive_class.border_unhighlight = 
                  UnhighlightSash;
   SashBaseClassExtRec.record_type = XmQmotif;
}

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
        XmSashWidget request = (XmSashWidget) rw ;
        XmSashWidget new_w = (XmSashWidget) nw ;
  if (request->core.width == 0)
     new_w->core.width += SASHSIZE;
  if (request->core.height == 0)
     new_w->core.height += SASHSIZE;
  new_w->sash.has_focus = False;
}

static void 
#ifdef _NO_PROTO
HighlightSash( sash )
        Widget sash ;
#else
HighlightSash(
        Widget sash )
#endif /* _NO_PROTO */
{
  int x, y;
  
  x = y = ((XmSashWidget) sash)->primitive.shadow_thickness;
  
  XFillRectangle( XtDisplay( sash), XtWindow( sash),
                   ((XmSashWidget) sash)->primitive.highlight_GC,
                   x,y, sash->core.width-(2*x), sash->core.height-(2*y));
}

static void 
#ifdef _NO_PROTO
UnhighlightSash( sash )
        Widget sash ;
#else
UnhighlightSash(
        Widget sash )
#endif /* _NO_PROTO */
{
  int x, y;
  
  x = y = ((XmSashWidget) sash)->primitive.shadow_thickness;

  XClearArea( XtDisplay( sash), XtWindow( sash),
                   x,y, sash->core.width-(2*x), sash->core.height-(2*y),
	           FALSE);
}

static XmNavigability
#ifdef _NO_PROTO
WidgetNavigable( wid)
        Widget wid ;
#else
WidgetNavigable(
        Widget wid)
#endif /* _NO_PROTO */
{   
  if(    _XmShellIsExclusive( wid)    )
    {
      /* Preserve 1.0 behavior.  (Why?  Don't ask me!)
       */
      return XmNOT_NAVIGABLE ;
    }
  if(    wid->core.sensitive
     &&  wid->core.ancestor_sensitive
     &&  ((XmPrimitiveWidget) wid)->primitive.traversal_on    )
    {   
      XmNavigationType nav_type = ((XmPrimitiveWidget) wid)
	                                          ->primitive.navigation_type ;
      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	 ||  (    (nav_type == XmTAB_GROUP)
	      &&  !_XmShellIsExclusive( wid))    )
	{
	  return XmTAB_NAVIGABLE ;
	}
    }
  return XmNOT_NAVIGABLE ;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SashFocusIn( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
SashFocusIn(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    register XmSashWidget sash = (XmSashWidget) w;

    if (event->xany.type != FocusIn || !event->xfocus.send_event)
          return;

    if (_XmGetFocusPolicy( (Widget) sash) == XmEXPLICIT)
       HighlightSash(w);


    _XmDrawShadows (XtDisplay (w), XtWindow (w),
                     sash->primitive.top_shadow_GC,
                     sash->primitive.bottom_shadow_GC,
                     0,0,w->core.width, w->core.height,
                     sash->primitive.shadow_thickness,
		     XmSHADOW_OUT);

    sash->sash.has_focus = True;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SashFocusOut( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
SashFocusOut(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    register XmSashWidget sash = (XmSashWidget) w;

    if (event->xany.type != FocusOut || !event->xfocus.send_event)
          return;

    if (_XmGetFocusPolicy( (Widget) sash) == XmEXPLICIT)
       UnhighlightSash(w);

    _XmDrawShadows (XtDisplay (w), XtWindow (w),
                     sash->primitive.top_shadow_GC,
                     sash->primitive.bottom_shadow_GC,
                     0,0,w->core.width, w->core.height,
                     sash->primitive.shadow_thickness,
		     XmSHADOW_OUT);

    sash->sash.has_focus = False;
}

static void 
#ifdef _NO_PROTO
SashAction( widget, event, params, num_params )
        Widget widget ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SashAction(
        Widget widget,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    SashCallDataRec call_data;

    call_data.event = event;
    call_data.params = params;
    call_data.num_params = *num_params;

    XtCallCallbacks(widget, XmNcallback, (XtPointer)&call_data );
}

static void 
#ifdef _NO_PROTO
Realize( w, p_valueMask, attributes )
        register Widget w ;
        XtValueMask *p_valueMask ;
        XSetWindowAttributes *attributes ;
#else
Realize(
        register Widget w,
        XtValueMask *p_valueMask,
        XSetWindowAttributes *attributes )
#endif /* _NO_PROTO */
{
	XmDisplay   dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
	Cursor SashCursor = 
		((XmDisplayInfo *)(dd->display.displayInfo))->SashCursor;
	
	if (0L == SashCursor)
		{
		/* create some data shared among all instances on this 
		** display; the first one along can create it, and 
		** any one can remove it; note no reference count
		*/
        	SashCursor = 
		((XmDisplayInfo *)(dd->display.displayInfo))->SashCursor = 
			XCreateFontCursor(XtDisplay(w), XC_crosshair);
		XtAddCallback((Widget)dd, XtNdestroyCallback, 
			SashDisplayDestroyCallback, (XtPointer) NULL);
		}

	attributes->cursor = SashCursor;
	XtCreateWindow (w, InputOutput, CopyFromParent, 
		*p_valueMask | CWCursor, attributes);
}

static void 
SashDisplayDestroyCallback 
#ifdef _NO_PROTO
	( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
	( Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XmDisplay   dd = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
	Cursor SashCursor;
        if ((XmDisplay)NULL != dd)
	{
	  SashCursor  = 
		((XmDisplayInfo *)(dd->display.displayInfo))->SashCursor;
	    if (0L != SashCursor)
		{
			XFreeCursor(XtDisplay(w), SashCursor);
			/*
			((XmDisplayInfo *)(dd->display.displayInfo))->SashCursor= 0L;
			*/
		}
	}
}




/*************************************<->*************************************
 *
 *  Redisplay (w, event)
 *
 *   Description:
 *   -----------
 *     Cause the widget, identified by w, to be redisplayed.
 *
 *
 *   Inputs:
 *   ------
 *     w = widget to be redisplayed;
 *     event = event structure identifying need for redisplay on this
 *             widget.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   DrawToggle()
 *   XDrawString()
 *************************************<->***********************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Redisplay( w, event, region )
        Widget w ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget w,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
   register XmSashWidget sash = (XmSashWidget) w;

     _XmDrawShadows (XtDisplay (w), XtWindow (w), 
                      sash->primitive.top_shadow_GC,
                      sash->primitive.bottom_shadow_GC, 
		      0,0,w->core.width, w->core.height,
                      sash->primitive.shadow_thickness,
		      XmSHADOW_OUT);

     if (sash->sash.has_focus) HighlightSash(w);
}
