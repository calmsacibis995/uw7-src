#pragma ident	"@(#)m1.2libs:Xm/TearOffB.c	1.3"
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
*  (c) Copyright 1989, 1990 DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
/*
 * Include files & Static Routine Definitions
 */

#include "XmI.h"
#include <Xm/TearOffBP.h>
#include <Xm/TearOffP.h>
#include <Xm/RowColumnP.h>
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include "RepTypeI.h"

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void GetSeparatorGC() ;
static void ClassInitialize() ;
static void Initialize() ;
static Boolean SetValues() ;
static void Destroy() ;
static void Redisplay() ;
static void BDrag();
static void BActivate();
static void KActivate();

#else

static void GetSeparatorGC(
                        XmTearOffButtonWidget tob) ;

static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget w) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void BDrag(
			Widget wid,
			XEvent *event,
			String *param,
			Cardinal *num_param) ;
static void BActivate(
			Widget wid,
			XEvent *event,
			String *param,
			Cardinal *num_param) ;
static void KActivate(
			Widget wid,
			XEvent *event,
			String *param,
			Cardinal *num_param) ;
static void ClassInitialize( void );

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/* Definition for resources that need special processing in get values */

static XmSyntheticResource syn_resources[] =
{
   {
      XmNmargin,
      sizeof (Dimension),
      XtOffsetOf( struct _XmTearOffButtonRec, tear_off_button.margin),
      _XmFromHorizontalPixels,
      _XmToHorizontalPixels
   },
};

/*  Resource list for Separator  */

static XtResource resources[] =
{
   {
      XmNseparatorType, XmCSeparatorType, XmRSeparatorType, sizeof (unsigned char),
      XtOffsetOf( struct _XmTearOffButtonRec, tear_off_button.separator_type),
      XmRImmediate, (XtPointer) XmSHADOW_ETCHED_OUT_DASH
   },

   {
      XmNmargin,
      XmCMargin,
      XmRHorizontalDimension,
      sizeof (Dimension),
      XtOffsetOf( struct _XmTearOffButtonRec, tear_off_button.margin),
      XmRImmediate, (XtPointer)  0
   },

	/* The value 1 will signal RowColumn to set the default value
	   for height */
   {
     XmNheight, XmCDimension, XmRVerticalDimension, sizeof(Dimension),
     XtOffsetOf( struct _WidgetRec, core.height), XmRImmediate, 
     (XtPointer) 1
   },

};


/*************************************<->*************************************
 *
 *
 *   Description:   translation tables for class: PushButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

#define overrideTranslations	_XmTearOffB_overrideTranslations


/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: TearOffButton
 *   -----------
 *
 *   Matches string descriptors with internal routines.
 *   Note that Primitive will register additional event handlers
 *   for traversal.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"BDrag", 		BDrag		 	},
  {"BActivate", 	BActivate		},
  {"KActivate", 	KActivate		},
};

XmPrimitiveClassExtRec _XmTearOffBPrimClassExtRec = {
     NULL,
     NULLQUARK,
     XmPrimitiveClassExtVersion,
     sizeof(XmPrimitiveClassExtRec),
     XmInheritBaselineProc,                  /* widget_baseline */
     XmInheritDisplayRectProc,               /* widget_display_rect */
};

externaldef(xmtearoffbuttonclassrec)  
	XmTearOffButtonClassRec xmTearOffButtonClassRec = {
  {
/* core_class record */	
    /* superclass	  */	(WidgetClass) &xmPushButtonClassRec,
    /* class_name	  */	"XmTearOffButton",
    /* widget_size	  */	sizeof(XmTearOffButtonRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    NULL,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */    NULL,
    /* realize		  */	XtInheritRealize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	XtExposeCompressMaximal,
    /* compress_enterlv   */    TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	Destroy,
    /* resize		  */	XtInheritResize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus	  */	NULL,
    /* version            */	XtVersion,
    /* callback_private   */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry     */	XtInheritQueryGeometry, 
    /* display_accelerator */   NULL,
    /* extension          */    NULL,
  },

  { /* primitive_class record       */

    /* Primitive border_highlight   */	XmInheritBorderHighlight,
    /* Primitive border_unhighlight */	XmInheritBorderUnhighlight,
    /* translations		    */  XtInheritTranslations,
    /* arm_and_activate		    */  XmInheritArmAndActivate,
    /* get resources		    */  syn_resources,
    /* num get_resources	    */  XtNumber(syn_resources),
    /* extension		    */  (XtPointer)&_XmTearOffBPrimClassExtRec,
  },

  { /* label_class record */
 
    /* setOverrideCallback	*/	XmInheritWidgetProc,
    /* menu procedures		*/	XmInheritMenuProc,
    /* menu traversal xlation	*/ 	XtInheritTranslations,
    /* extension		*/	(XtPointer) NULL,
  },

  { /* pushbutton_class record */
    /* extension		*/	(XtPointer) NULL,
  },

  { /* tearoffbutton_class record */
    /* Button override xlation	*/	XtInheritTranslations,
  }

};
externaldef(xmtearoffbuttonwidgetclass)
   WidgetClass xmTearOffButtonWidgetClass = (WidgetClass)&xmTearOffButtonClassRec;


/************************************************************************
 *
 *  GetSeparatorGC
 *     Get the graphics context used for drawing the separator.
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
GetSeparatorGC( tob )
        XmTearOffButtonWidget tob ;
#else
GetSeparatorGC(
        XmTearOffButtonWidget tob )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;

   values.foreground = tob->primitive.foreground;
   values.background = tob->core.background_pixel;

   if (tob -> tear_off_button.separator_type == XmSINGLE_DASHED_LINE ||
       tob -> tear_off_button.separator_type == XmDOUBLE_DASHED_LINE)
   {
      valueMask = valueMask | GCLineStyle;
      values.line_style = LineDoubleDash;
   }

   tob->tear_off_button.separator_GC = 
      XtGetGC ((Widget) tob, valueMask, &values);
}

/************************************************************************
 *
 *  ClassInitialize
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   xmTearOffButtonClassRec.tearoffbutton_class.translations =
      (String)XtParseTranslationTable(overrideTranslations);
}

/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/
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
   XmTearOffButtonWidget new_w = (XmTearOffButtonWidget) nw ;

   GetSeparatorGC((XmTearOffButtonWidget)nw);

   XtOverrideTranslations(nw, (XtTranslations)((XmTearOffButtonClassRec *)
     XtClass(nw))->tearoffbutton_class.translations);

   if(!XmRepTypeValidValue(XmRID_SEPARATOR_TYPE,
			 new_w->tear_off_button.separator_type,
			 (Widget) new_w)) {
     new_w -> tear_off_button.separator_type = XmSHADOW_ETCHED_OUT_DASH;
   }

   /* force the orientation */
   new_w->tear_off_button.orientation = XmHORIZONTAL ;
}

/************************************************************************
 *
 *  Redisplay (tob, event, region)
 *   Description:
 *   -----------
 *     Cause the widget, identified by tob, to be redisplayed.
 *
 ************************************************************************/

/*ARGSUSED*/
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
   XmTearOffButtonWidget tob = (XmTearOffButtonWidget) wid ;

   /*
    * Where do we check for dependency on MenyType ??
    */
   if (XtIsRealized(tob))
   { 

      _XmDrawSeparator(XtDisplay(tob), XtWindow(tob),
                  tob->primitive.top_shadow_GC,
                  tob->primitive.bottom_shadow_GC,
                  tob->tear_off_button.separator_GC,
                  tob->primitive.highlight_thickness,
                  tob->primitive.highlight_thickness,
                  tob->core.width - 2*tob->primitive.highlight_thickness,
                  tob->core.height - 2*tob->primitive.highlight_thickness,
                  tob->primitive.shadow_thickness,
                  tob->tear_off_button.margin,
                  tob->tear_off_button.orientation,
                  tob->tear_off_button.separator_type);

      if (tob -> primitive.highlighted)
      {   
          (*(xmTearOffButtonClassRec.primitive_class.border_highlight))(
                                                                (Widget) tob) ;
          } 
      else
      {   if (_XmDifferentBackground( (Widget) tob, XtParent (tob)))
	  {   
              (*(xmTearOffButtonClassRec.primitive_class.border_unhighlight))(
                                                                (Widget) tob) ;
              } 
          } 
   }
}

/************************************************************************
 *
 *  Destroy
 *      Remove the callback lists.
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
   XtReleaseGC (wid, 
      ((XmTearOffButtonWidget) wid)->tear_off_button.separator_GC);
}

/************************************************************************
 *
 *  SetValues
 *
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
   XmTearOffButtonWidget current = (XmTearOffButtonWidget) cw ;
   XmTearOffButtonWidget new_w = (XmTearOffButtonWidget) nw ;
   Boolean flag = FALSE;

   if(!XmRepTypeValidValue(XmRID_SEPARATOR_TYPE,
                           new_w->tear_off_button.separator_type, (Widget) new_w)) 
   {
      new_w -> tear_off_button.separator_type = XmSHADOW_ETCHED_OUT_DASH;
   }

   /* force the orientation */
   new_w -> tear_off_button.orientation = XmHORIZONTAL;

   if ((new_w->core.background_pixel != current->core.background_pixel) ||
       (new_w->tear_off_button.separator_type !=
         current->tear_off_button.separator_type) ||
       (new_w->primitive.foreground != current->primitive.foreground))
   {
      XtReleaseGC ((Widget) new_w, new_w->tear_off_button.separator_GC);
      GetSeparatorGC (new_w);
      flag = TRUE;
   }

   if ((new_w->tear_off_button.margin != current->tear_off_button.margin) ||
       (new_w->primitive.shadow_thickness !=
         current->primitive.shadow_thickness))
   {
      flag = TRUE;
   }

   return (flag);
}

/************************************************************************
 *
 *  BDrag
 *    On button 2 down, tear off the menu
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
BDrag( wid, event, param, num_param )
	Widget wid;
	XEvent *event;
	String *param;
	Cardinal *num_param;
#else
BDrag( Widget wid,
	XEvent *event,
	String *param,
	Cardinal *num_param )
#endif /* _NO_PROTO */
{
   _XmTearOffInitiate(XtParent(wid), event);
}

/************************************************************************
 *
 *  BActivate
 *    On either a button down or a button up, tear off the menu
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
BActivate( wid, event, param, num_param )
	Widget wid;
	XEvent *event;
	String *param;
	Cardinal *num_param;
#else
BActivate( Widget wid,
	XEvent *event,
	String *param,
	Cardinal *num_param )
#endif /* _NO_PROTO */
{
   Widget parent = XtParent(wid);

   if (_XmMatchBtnEvent(event, XmIGNORE_EVENTTYPE, RC_PostButton(parent),
	  RC_PostModifiers(parent)) ||
       _XmMatchBSelectEvent(parent, event))
   {
      _XmTearOffInitiate(parent, event);
   }
}

/************************************************************************
 *
 *  KActivate
 *    Initiate Tear Off on a keypress
 ************************************************************************/
static void
#ifdef _NO_PROTO
KActivate( wid, event, param, num_param )
	Widget wid;
	XEvent *event;
	String *param;
	Cardinal *num_param;
#else
KActivate( Widget wid,
	XEvent *event,
	String *param,
	Cardinal *num_param )
#endif /* _NO_PROTO */
{
   XButtonEvent xb_ev ;
   Widget parent = XtParent(wid);
   Position x, y;

   /* stick the tear off at the same location as the submenu */
   XtTranslateCoords(parent, XtX(parent), XtY(parent), 
      &x, &y);

   xb_ev = event->xbutton;
   xb_ev.x_root = x;
   xb_ev.y_root = y;

   _XmTearOffInitiate(parent, (XEvent *) &xb_ev);
}
