#pragma ident	"@(#)m1.2libs:Xm/DrawnB.c	1.3"
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
/*
 * Include files & Static Routine Definitions
 */

#include <stdio.h>
#include <X11/X.h>
#include "XmI.h"
#include "RepTypeI.h"
#include <Xm/MenuUtilP.h>
#include <Xm/LabelP.h>
#include <Xm/DrawnBP.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>   


#define DELAY_DEFAULT 100	

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void Arm() ;
static void MultiArm() ;
static void Activate() ;
static void MultiActivate() ;
static void ActivateCommon() ;
static void ArmAndActivate() ;
static void ArmTimeout() ;
static void Disarm() ;
static void Enter() ;
static void Leave() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Resize() ;
static void Redisplay() ;
static void DrawPushButton() ;
static Boolean SetValues() ;
static void Realize() ;
static void Destroy() ;

#else

static void Arm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MultiArm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Activate( 
                        Widget wid,
                        XEvent *buttonEvent,
                        String *params,
                        Cardinal *num_params) ;
static void MultiActivate( 
                        Widget wid,
                        XEvent *buttonEvent,
                        String *params,
                        Cardinal *num_params) ;
static void ActivateCommon( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmAndActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmTimeout (
        		XtPointer closure,
        		XtIntervalId *id ) ;
static void Disarm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Enter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Leave( 
                        Widget wid,
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
static void Resize( 
                        Widget wid) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void DrawPushButton( 
                        XmDrawnButtonWidget db,
#if NeedWidePrototypes
                        int armed) ;
#else
                        Boolean armed) ;
#endif /* NeedWidePrototypes */
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Realize( 
                        Widget w,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*************************************<->*************************************
 *
 *
 *   Description:   translation tables for class: DrawnButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

#define defaultTranslations	_XmDrawnB_defaultTranslations


/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: DrawnButton
 *   -----------
 *
 *   Matches string descriptors with internal routines.
 *   Note that Primitive will register additional event handlers
 *   for traversal.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"Arm", 	Arm		 },
  {"Activate", 	Activate		 },
  {"MultiActivate", MultiActivate		 },
  {"MultiArm",	MultiArm },
  {"ArmAndActivate", ArmAndActivate },
  {"Disarm", 	Disarm		 },
  {"Enter", 	Enter		 },
  {"Leave",	Leave		 },
};


/*  The resource list for Drawn Button  */

static XtResource resources[] = 
{     
   {
     XmNmultiClick, XmCMultiClick, XmRMultiClick, sizeof (unsigned char),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.multiClick),
     XmRImmediate, (XtPointer) XmMULTICLICK_KEEP
   },

   {
     XmNpushButtonEnabled, XmCPushButtonEnabled, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.pushbutton_enabled),
     XmRImmediate, (XtPointer) False
   },

   {
     XmNshadowType, XmCShadowType, XmRShadowType, sizeof(unsigned char),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.shadow_type),
     XmRImmediate, (XtPointer) XmSHADOW_ETCHED_IN
   },

   {
     XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.activate_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.arm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNdisarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.disarm_callback),
     XmRPointer, (XtPointer) NULL
   },
   
   {
     XmNexposeCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.expose_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNresizeCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmDrawnButtonRec, drawnbutton.resize_callback),
     XmRPointer, (XtPointer) NULL
   },
   
   {
     XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, 
     sizeof(Dimension),
     XtOffsetOf( struct _XmDrawnButtonRec, primitive.shadow_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {    
     XmNlabelString, XmCXmString, XmRXmString, sizeof(XmString),
     XtOffsetOf( struct _XmDrawnButtonRec, label._label),
     XmRImmediate, (XtPointer) XmUNSPECIFIED
   },
   {
	XmNtraversalOn,
	XmCTraversalOn,
	XmRBoolean,
	sizeof(Boolean),
	XtOffsetOf( struct _XmPrimitiveRec, primitive.traversal_on),
	XmRImmediate,
	(XtPointer) True
   },

   {
	XmNhighlightThickness,
	XmCHighlightThickness,
	XmRHorizontalDimension,
	sizeof (Dimension),
	XtOffsetOf( struct _XmPrimitiveRec, primitive.highlight_thickness),
	XmRImmediate,
	(XtPointer) 2
   }
};

 XmPrimitiveClassExtRec _XmDrawnBPrimClassExtRec = {
      NULL,
      NULLQUARK,
      XmPrimitiveClassExtVersion,
      sizeof(XmPrimitiveClassExtRec),
      XmInheritBaselineProc,                  /* widget_baseline */
      XmInheritDisplayRectProc,               /* widget_display_rect */
      NULL,                                   /* widget_margins */
 };

/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: DrawnButton
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/

externaldef(xmdrawnbuttonclassrec) XmDrawnButtonClassRec xmDrawnButtonClassRec ={
  {
/* core_class record */	
    /* superclass	  */	(WidgetClass) &xmLabelClassRec,
    /* class_name	  */	"XmDrawnButton",
    /* widget_size	  */	sizeof(XmDrawnButtonRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */    NULL,
    /* realize		  */	Realize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	FALSE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	Destroy,
    /* resize		  */	Resize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus	  */	NULL,
    /* version            */	XtVersion,
    /* callback_private   */    NULL,
    /* tm_table           */    defaultTranslations,
    /* query_geometry     */	NULL, 
    /* display_accelerator */   NULL,
    /* extension          */    NULL,
  },

  { /* primitive_class record       */

    /* Primitive border_highlight   */	XmInheritWidgetProc,
    /* Primitive border_unhighlight */	XmInheritWidgetProc,
    /* translations		    */  XtInheritTranslations,
    /* arm_and_activate		    */  ArmAndActivate,
    /* get resources		    */  NULL,
    /* num get_resources	    */  0,
    /* extension		    */  (XtPointer)&_XmDrawnBPrimClassExtRec,
  },

  { /* label_class record */
 
    /* setOverrideCallback*/    XmInheritWidgetProc,
    /* Menu procedures    */    NULL,				
    /* menu trav xlations */	NULL,
    /* extension	  */	NULL,
  },

  { /* drawnbutton_class record */

    /* extension	  */    NULL,	
  }

};
externaldef(xmdrawnbuttonwidgetclass) WidgetClass xmDrawnButtonWidgetClass =
			     (WidgetClass)&xmDrawnButtonClassRec;


/************************************************************************
 *
 *     Arm
 *
 *     This function processes button 1 down occuring on the drawnbutton.
 *     Mark the drawnbutton as armed if XmNpushButtonEnabled is TRUE.
 *     The callbacks for XmNarmCallback are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Arm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Arm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
    XButtonEvent *buttonEvent = (XButtonEvent *) event;
    XmDrawnButtonCallbackStruct call_value;
   
    (void) XmProcessTraversal((Widget) db, XmTRAVERSE_CURRENT);

    db -> drawnbutton.armed = TRUE;
    if (event && (event->type == ButtonPress))
	db -> drawnbutton.armTimeStamp = buttonEvent->time;
    
    if (db->drawnbutton.pushbutton_enabled)
	DrawPushButton(db, db->drawnbutton.armed);

    if (db->drawnbutton.arm_callback) {
	XFlush(XtDisplay (db));

	call_value.reason = XmCR_ARM;
	call_value.event = event;
	call_value.window = XtWindow (db);
	XtCallCallbackList ((Widget) db, db->drawnbutton.arm_callback, 
			    &call_value);
    }
}


static void 
#ifdef _NO_PROTO
MultiArm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MultiArm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    if (((XmDrawnButtonWidget) wid)->drawnbutton.multiClick == XmMULTICLICK_KEEP)
			Arm (wid, event, NULL, NULL);
}

/************************************************************************
 *
 *     Activate
 *
 *     Mark the drawnbutton as unarmed (i.e. inactive).
 *     The foreground and background colors will revert to the 
 *     unarmed state if XmNinvertOnArm is set to TRUE.
 *     If the button release occurs inside of the DrawnButton, the 
 *     callbacks for XmNactivateCallback are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Activate( wid, buttonEvent, params, num_params )
        Widget wid ;
        XEvent *buttonEvent ;
        String *params ;
        Cardinal *num_params ;
#else
Activate(
        Widget wid,
        XEvent *buttonEvent,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   if (db -> drawnbutton.armed == FALSE)
      return;

   db->drawnbutton.click_count = 1;
   ActivateCommon ((Widget) db, buttonEvent, params, num_params);

}

static void 
#ifdef _NO_PROTO
MultiActivate( wid, buttonEvent, params, num_params )
        Widget wid ;
        XEvent *buttonEvent ;
        String *params ;
        Cardinal *num_params ;
#else
MultiActivate(
        Widget wid,
        XEvent *buttonEvent,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   /* When a multi click sequence occurs and the user Button Presses and
    * holds for a length of time, the final release should look like a
    * new/separate activate.
    */
  if (db->drawnbutton.multiClick == XmMULTICLICK_KEEP)  
  { if ((buttonEvent->xbutton.time - db->drawnbutton.armTimeStamp) >
	   XtGetMultiClickTime(XtDisplay(db)))
     db->drawnbutton.click_count = 1;
   else
     db->drawnbutton.click_count++;
   ActivateCommon ((Widget) db, buttonEvent, params, num_params) ;
   Disarm ((Widget) db, buttonEvent, params, num_params) ;
 }
}

static void 
#ifdef _NO_PROTO
ActivateCommon( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ActivateCommon(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   XButtonEvent *buttonEvent = (XButtonEvent *) event;
   XmDrawnButtonCallbackStruct call_value;
   Dimension bw = db->core.border_width ;
      
   if (event && (event->xbutton.type != ButtonRelease))
       return;
      
   db -> drawnbutton.armed = FALSE;
   if (db->drawnbutton.pushbutton_enabled)
	DrawPushButton(db, db->drawnbutton.armed);


   if ((buttonEvent->x >= -(int)bw) &&
       (buttonEvent->x < (int)(db->core.width + bw)) &&
       (buttonEvent->y >= -(int)bw) &&
       (buttonEvent->y < (int)(db->core.height + bw)) &&
       (db->drawnbutton.activate_callback))
   {
      XFlush(XtDisplay (db));

      call_value.reason = XmCR_ACTIVATE;
      call_value.event = event;
      call_value.window = XtWindow (db);
      call_value.click_count = db->drawnbutton.click_count;

      if ((db->drawnbutton.multiClick == XmMULTICLICK_DISCARD) &&
	  (call_value.click_count > 1))
      {
	  return;
      }

      if (XmIsRowColumn(XtParent(db)))
      {
	 (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(db),
						    FALSE, db,
						    &call_value);
      }

      if ((! db->label.skipCallback) &&
	  (db->drawnbutton.activate_callback))
      {
	 XtCallCallbackList ((Widget) db, db->drawnbutton.activate_callback,
				&call_value);
      }
   }
}

/************************************************************************
 *
 *     ArmAndActivate
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ArmAndActivate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ArmAndActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   XmDrawnButtonCallbackStruct call_value;

   db -> drawnbutton.armed = TRUE;
   if (db->drawnbutton.pushbutton_enabled)
	DrawPushButton(db, db->drawnbutton.armed);

   XFlush(XtDisplay (db));

   if (db->drawnbutton.arm_callback)
   {
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      call_value.window = XtWindow (db);
      XtCallCallbackList ((Widget) db, db->drawnbutton.arm_callback, &call_value);
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
   call_value.window = XtWindow (db);
   call_value.click_count = 1;		/* always 1 in kselect */

   if (XmIsRowColumn(XtParent(db)))
   {
      (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						 XtParent(db),
						 FALSE, db,
						 &call_value);
   }

   if ((! db->label.skipCallback) &&
       (db->drawnbutton.activate_callback))
   {
      XtCallCallbackList ((Widget) db, db->drawnbutton.activate_callback,
			     &call_value);
   }

   db->drawnbutton.armed = FALSE;
   
   if (db->drawnbutton.disarm_callback)
   {
      call_value.reason = XmCR_DISARM;
      XtCallCallbackList ((Widget) db, db->drawnbutton.disarm_callback,
                             &call_value);
   }

   /* If the button is still around, show it released, after a short delay */
   if (!db->core.being_destroyed && db->drawnbutton.pushbutton_enabled)
   {
       db->drawnbutton.timer = XtAppAddTimeOut(
				       XtWidgetToApplicationContext((Widget)db),
                                       (unsigned long) DELAY_DEFAULT,
                                       ArmTimeout,
                                       (XtPointer)db);
   }
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ArmTimeout( closure, id)
	XtPointer closure ;
	XtIntervalId *id ;
#else
ArmTimeout (
	XtPointer closure,
	XtIntervalId *id )
#endif
{
	XmDrawnButtonWidget db = (XmDrawnButtonWidget) closure ;
   db -> drawnbutton.timer = 0;
   if (db->drawnbutton.pushbutton_enabled &&
       XtIsRealized (db) && XtIsManaged (db))
   {
      DrawPushButton(db, db->drawnbutton.armed);
      XFlush (XtDisplay (db));
   }
   return;
}



/************************************************************************
 *
 *    Disarm
 *
 *     Mark the drawnbutton as unarmed (i.e. active).
 *     The foreground and background colors will revert to the 
 *     unarmed state if XmNinvertOnSelect is set to TRUE and the
 *     drawnbutton is not in a menu.
 *     The callbacks for XmNdisarmCallback are called..
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Disarm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Disarm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   XmDrawnButtonCallbackStruct call_value;

   db -> drawnbutton.armed = FALSE;

   if (db->drawnbutton.disarm_callback)
   {
      XFlush(XtDisplay (db));

      call_value.reason = XmCR_DISARM;
      call_value.event = event;
      call_value.window = XtWindow (db);
      XtCallCallbackList ((Widget) db, db->drawnbutton.disarm_callback, &call_value);
   }
}


/************************************************************************
 *
 *  Enter
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Enter( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Enter(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   _XmPrimitiveEnter (wid, event, params, num_params);

   if (db -> drawnbutton.pushbutton_enabled &&
      db -> drawnbutton.armed == TRUE)
      DrawPushButton(db, TRUE);
}


/************************************************************************
 *
 *  Leave
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Leave( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Leave(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
            XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   _XmPrimitiveLeave (wid, event, params, num_params);

   if (db -> drawnbutton.pushbutton_enabled &&
      db -> drawnbutton.armed == TRUE)
      DrawPushButton(db, FALSE);
}


/************************************************************************
 *
 *  ClassPartInitialize
 *     Set up the fast subclassing for the widget
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
   _XmFastSubclassInit (wc, XmDRAWN_BUTTON_BIT);
}

      
/*************************************<->*************************************
 *
 *  Initialize 
 *
 *************************************<->***********************************/
/*ARGSUSED*/
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
   XmDrawnButtonWidget new_w = (XmDrawnButtonWidget) nw ;

   new_w->drawnbutton.armed = FALSE;
   new_w->drawnbutton.timer = 0;

   /* if menuProcs is not set up yet, try again */
   if (xmLabelClassRec.label_class.menuProcs == (XmMenuProc)NULL)
      xmLabelClassRec.label_class.menuProcs =
	 (XmMenuProc) _XmGetMenuProcContext();

   if(    !XmRepTypeValidValue( XmRID_SHADOW_TYPE,
                               new_w->drawnbutton.shadow_type, (Widget) new_w)    )
   {
      new_w -> drawnbutton.shadow_type = XmSHADOW_ETCHED_IN;
   }

}

/*************************************<->*************************************
 *
 *  Resize (db)
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   XmDrawnButtonCallbackStruct call_value;

   (* xmLabelClassRec.core_class.resize) ((Widget) db);

   if (db->drawnbutton.resize_callback)
   {
      XFlush(XtDisplay (db));
      call_value.reason = XmCR_RESIZE;
      call_value.event = NULL;
      call_value.window = XtWindow (db);
      XtCallCallbackList ((Widget) db, db->drawnbutton.resize_callback, &call_value);
   }
}


/*************************************<->*************************************
 *
 *  Redisplay (db, event, region)
 *
 *************************************<->***********************************/
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
   XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   XmDrawnButtonCallbackStruct call_value;

   if (XtIsRealized(db)) 
   {
        if (event)
         (* xmLabelClassRec.core_class.expose) ((Widget) db, event, region);

 	if (db->drawnbutton.pushbutton_enabled)
 	    DrawPushButton(db, db->drawnbutton.armed);
  
 	else
 	    _XmDrawShadows(XtDisplay((Widget) db),
 			    XtWindow((Widget) db),
 			    db -> primitive.top_shadow_GC,
 			    db -> primitive.bottom_shadow_GC,
 			    db -> primitive.highlight_thickness,
 			    db -> primitive.highlight_thickness,
 			    db -> core.width - 2 *
 			       db -> primitive.highlight_thickness,
 			    db -> core.height - 2 *
 			       db -> primitive.highlight_thickness,	
			    db -> primitive.shadow_thickness,
 			    db->drawnbutton.shadow_type);
 			   
      if (db->drawnbutton.expose_callback)
      {
         XFlush(XtDisplay (db));

	 call_value.reason = XmCR_EXPOSE;
	 call_value.event = event;
	 call_value.window = XtWindow (db);
	 XtCallCallbackList ((Widget) db, db->drawnbutton.expose_callback, &call_value);
      }

   }
}


static void 
#ifdef _NO_PROTO
DrawPushButton( db, armed )
        XmDrawnButtonWidget db ;
        Boolean armed ;
#else
DrawPushButton(
        XmDrawnButtonWidget db,
#if NeedWidePrototypes
        int armed )
#else
        Boolean armed )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

     _XmDrawShadows (XtDisplay (db), XtWindow (db), 
 		     db -> primitive.top_shadow_GC,
 		     db -> primitive.bottom_shadow_GC, 
 		     db -> primitive.highlight_thickness,
 		     db -> primitive.highlight_thickness,
 		     db -> core.width - 2 * 
 		        db->primitive.highlight_thickness,
 		     db -> core.height - 2 * 
 		        db->primitive.highlight_thickness,
 		     db -> primitive.shadow_thickness,
 		     (armed)?XmSHADOW_IN:XmSHADOW_OUT);
/****************
 *
 * Old stuff...
   if (db->primitive.shadow_thickness > 0)
      _XmDrawShadow (XtDisplay (db), XtWindow (db), 
	    (armed
		? db -> primitive.bottom_shadow_GC
		: db -> primitive.top_shadow_GC),
	    (armed 
		? db -> primitive.top_shadow_GC 
		: db -> primitive.bottom_shadow_GC), 
	      db -> primitive.shadow_thickness,
	      db -> primitive.highlight_thickness,
	      db -> primitive.highlight_thickness,
	      db -> core.width - 2 * db->primitive.highlight_thickness,
	      db -> core.height - 2 * db->primitive.highlight_thickness);
 *
 ****************/
}



/*************************************<->*************************************
 *
 *  SetValues(current, request, new_w)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the drawnbutton class.  It is
 *     called last (the set values rtnes for its superclasses are called
 *     first).
 *
 *
 *   Inputs:
 *   ------
 *    current = original widget;
 *    request = original copy of request;
 *    new_w = copy of request which reflects changes made to it by
 *          set values procedures of its superclasses;
 *    last = TRUE if this is the last set values procedure to be called.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
/*ARGSUSED*/
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
   XmDrawnButtonWidget current = (XmDrawnButtonWidget) cw ;
   XmDrawnButtonWidget new_w = (XmDrawnButtonWidget) nw ;
   Boolean  flag = FALSE;    /* our return value */

    /*  Check the data put into the new widget.  */

   if(    !XmRepTypeValidValue( XmRID_SHADOW_TYPE,
                               new_w->drawnbutton.shadow_type, (Widget) new_w)    )
   {
      new_w->drawnbutton.shadow_type = current->drawnbutton.shadow_type ;
   }

   if (new_w -> drawnbutton.shadow_type != current-> drawnbutton.shadow_type ||
       new_w -> primitive.foreground != current -> primitive.foreground    ||
       new_w -> core.background_pixel != current -> core.background_pixel  ||
       new_w -> primitive.highlight_thickness != 
       current -> primitive.highlight_thickness                          ||
       new_w -> primitive.shadow_thickness !=
       current -> primitive.shadow_thickness)
   {
      flag = TRUE;
   }

   return(flag);
}




/*************************************************************************
 *
 *  Realize
 *	This function sets the bit gravity to forget.
 *
 *************************************************************************/
static void 
#ifdef _NO_PROTO
Realize( w, p_valueMask, attributes )
        Widget w ;
        XtValueMask *p_valueMask ;
        XSetWindowAttributes *attributes ;
#else
Realize(
        Widget w,
        XtValueMask *p_valueMask,
        XSetWindowAttributes *attributes )
#endif /* _NO_PROTO */
{
   Mask valueMask = *p_valueMask;

   valueMask |= CWBitGravity | CWDontPropagate;
   attributes->bit_gravity = ForgetGravity;
   attributes->do_not_propagate_mask =
      ButtonPressMask | ButtonReleaseMask |
      KeyPressMask | KeyReleaseMask | PointerMotionMask;

   XtCreateWindow (w, InputOutput, CopyFromParent, valueMask, attributes);
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
        XmDrawnButtonWidget db = (XmDrawnButtonWidget) wid ;
   if (db->drawnbutton.timer)
       XtRemoveTimeOut (db->drawnbutton.timer);

   XtRemoveAllCallbacks ((Widget) db, XmNactivateCallback);
   XtRemoveAllCallbacks ((Widget) db, XmNarmCallback);
   XtRemoveAllCallbacks ((Widget) db, XmNdisarmCallback);
   XtRemoveAllCallbacks ((Widget) db, XmNresizeCallback);
   XtRemoveAllCallbacks ((Widget) db, XmNexposeCallback);
}


/************************************************************************
 *
 *		Application Accessible External Functions
 *
 ************************************************************************/


/************************************************************************
 *
 *  XmCreateDrawnButton
 *	Create an instance of a drawnbutton and return the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateDrawnButton( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateDrawnButton(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmDrawnButtonWidgetClass, 
                           parent, arglist, argcount));
}
