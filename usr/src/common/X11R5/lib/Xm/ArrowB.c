#pragma ident	"@(#)m1.2libs:Xm/ArrowB.c	1.4"
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
#include <stdio.h>
#include "XmI.h"
#include <Xm/ArrowBP.h>
#include "RepTypeI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>


#define DELAY_DEFAULT 100	

#define UNAVAILABLE_STIPPLE_NAME "_XmScrollBarUnavailableStipple"

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassPartInitialize() ;
static void Initialize() ;
static void GetArrowGC() ;
static void Redisplay() ;
static void Destroy() ;
static Boolean SetValues() ;
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
static void DrawArrow() ;

#else

static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetArrowGC( 
                        XmArrowButtonWidget aw) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Destroy( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Arm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MultiArm( 
                        Widget aw,
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
                        XEvent *buttonEvent) ;
static void ArmAndActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmTimeout( 
                        XtPointer closure,
                        XtIntervalId *id) ;
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
static void DrawArrow(
		        XmArrowButtonWidget aw,
		        GC top_gc,
		        GC bottom_gc,
		        GC center_gc) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*  Default translation table and action list  */

#define defaultTranslations	_XmArrowB_defaultTranslations

static XtActionsRec actionsList[] =
{
  { "Activate",    Activate	    },
  { "MultiActivate", MultiActivate	    },
  { "Arm",         Arm		    },
  { "MultiArm",    MultiArm	    },
  { "Disarm",      Disarm	    },
  { "ArmAndActivate", ArmAndActivate },
  { "Enter",       Enter             },
  { "Leave",       Leave             },
};


/*  Resource list for ArrowButton  */

static XtResource resources[] = 
{
   {
      XmNmultiClick, XmCMultiClick, XmRMultiClick, 
      sizeof(unsigned char),
      XtOffsetOf( struct _XmArrowButtonRec, arrowbutton.multiClick), 
      XmRImmediate, (XtPointer) XmMULTICLICK_KEEP
   },

   {
      XmNarrowDirection, XmCArrowDirection, XmRArrowDirection, 
      sizeof(unsigned char),
      XtOffsetOf( struct _XmArrowButtonRec, arrowbutton.direction), 
      XmRImmediate, (XtPointer) XmARROW_UP
   },

   {
     XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonRec, arrowbutton.activate_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonRec, arrowbutton.arm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNdisarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonRec, arrowbutton.disarm_callback),
     XmRPointer, (XtPointer) NULL
   }
};


/*  The ArrowButton class record definition  */

externaldef (xmarrowbuttonclassrec) XmArrowButtonClassRec xmArrowButtonClassRec=
{
   {
      (WidgetClass) &xmPrimitiveClassRec, /* superclass            */	
      "XmArrowButton",                  /* class_name	         */	
      sizeof(XmArrowButtonRec),         /* widget_size	         */	
      (XtProc)NULL,                     /* class_initialize      */    
      ClassPartInitialize,              /* class_part_initialize */
      FALSE,                            /* class_inited          */	
      Initialize,                       /* initialize	         */	
      (XtArgsProc)NULL,                 /* initialize_hook       */
      XtInheritRealize,                 /* realize	         */	
      actionsList,                      /* actions               */	
      XtNumber(actionsList),            /* num_actions    	 */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      XtExposeCompressMaximal,          /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      Destroy,                          /* destroy               */	
      (XtWidgetProc)NULL,               /* resize                */
      Redisplay,                        /* expose                */	
      SetValues,                        /* set_values	         */	
      (XtArgsFunc)NULL,                 /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      (XtArgsProc)NULL,                 /* get_values_hook       */
      (XtAcceptFocusProc)NULL,          /* accept_focus	         */	
      XtVersion,                        /* version               */
      (XtPointer)NULL,                  /* callback private      */
      defaultTranslations,              /* tm_table              */
      (XtGeometryHandler)NULL,          /* query_geometry        */
      (XtStringProc)NULL,		/* display_accelerator   */
      (XtPointer)NULL,			/* extension             */
   },

   {
      XmInheritBorderHighlight,         /* Primitive border_highlight   */
      XmInheritBorderUnhighlight,       /* Primitive border_unhighlight */
      XtInheritTranslations,            /* translations                 */
      ArmAndActivate,		        /* arm_and_activate             */
      (XmSyntheticResource *)NULL,	/* get resources      		*/
      0,				/* num get_resources  		*/
      (XtPointer) NULL,         	/* extension                    */
   },

   {
	(XtPointer) NULL,		/* extension			*/
   }

};

externaldef(xmarrowbuttonwidgetclass) WidgetClass xmArrowButtonWidgetClass =
			  (WidgetClass) &xmArrowButtonClassRec;


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
   _XmFastSubclassInit (wc, XmARROW_BUTTON_BIT);
}

      
/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
Initialize( rw, nw)
        Widget rw ;
        Widget nw ;
#else
Initialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args)
#endif /* _NO_PROTO */
{
        XmArrowButtonWidget request = (XmArrowButtonWidget) rw ;
        XmArrowButtonWidget new_w = (XmArrowButtonWidget) nw ;

   /*
    *  Check the data put into the new widget from .Xdefaults
    *  or through the arg list.
    */

   if(    !XmRepTypeValidValue( XmRID_ARROW_DIRECTION, 
                                 new_w->arrowbutton.direction, (Widget) new_w)    )
   {
      new_w -> arrowbutton.direction = XmARROW_UP;
   }


   /*  Set up a geometry for the widget if it is currently 0.  */

   if (request -> core.width == 0) new_w -> core.width += 15;
   if (request -> core.height == 0) new_w -> core.height += 15;


   /*  Set the internal arrow variables  */

   new_w->arrowbutton.timer = 0;
   new_w->arrowbutton.selected = False;

   /*  Get the drawing graphics contexts.  */

   GetArrowGC (new_w);

}




/************************************************************************
 *
 *  GetArrowGC
 *     Get the graphics context used for drawing the arrowbutton.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
GetArrowGC( aw )
        XmArrowButtonWidget aw ;
#else
GetArrowGC(
        XmArrowButtonWidget aw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;
   XImage *t_im;
   Pixmap t_pix;
   
    valueMask = GCForeground | GCBackground | GCFillStyle;

    values.foreground = aw -> primitive.foreground;
    values.background = aw -> core.background_pixel;
    values.fill_style = FillSolid;

    aw -> arrowbutton.arrow_GC = XtGetGC ((Widget) aw, valueMask, &values);

    valueMask = GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;

    if ((t_pix = XmGetPixmapByDepth(XtScreen((Widget) aw), 
		UNAVAILABLE_STIPPLE_NAME, (Pixel) 1, (Pixel) 0, 1))
                == XmUNSPECIFIED_PIXMAP)
    {
       if (_XmGetImage(XtScreen((Widget) aw), "50_foreground", &t_im))
       {
          XGCValues t_values;
          XtGCMask  t_valueMask;
   	  GC t_gc;

          t_pix = XCreatePixmap(XtDisplay(aw),
                                RootWindowOfScreen(XtScreen((Widget) aw)),
                                t_im->width, t_im->height, 1);

          t_values.foreground = 1;
          t_values.background = 0;
          t_valueMask = (GCForeground | GCBackground);
          t_gc = XCreateGC(XtDisplay((Widget) aw), t_pix, t_valueMask,
                                &t_values);

          XPutImage(XtDisplay((Widget) aw), t_pix, t_gc, t_im, 0, 0, 0, 0,
                    t_im->width, t_im->height);
          XFreeGC(XtDisplay((Widget) aw), t_gc);

          values.fill_style = FillStippled;
          values.foreground = BlackPixelOfScreen(XtScreen((Widget) aw));
          values.stipple = t_pix;
          valueMask |= (GCFillStyle | GCStipple);

          _XmInstallPixmap(t_pix, XtScreen((Widget) aw),
                           UNAVAILABLE_STIPPLE_NAME, (Pixel) 1, (Pixel) 0);
       }
   
    }
    else
    {
       values.fill_style = FillStippled;
       values.foreground = BlackPixelOfScreen(XtScreen((Widget) aw));
       values.stipple = t_pix;
       valueMask |= (GCFillStyle | GCStipple);
    }

    aw->arrowbutton.insensitive_GC = XtGetGC((Widget) aw, valueMask, &values);
}




/************************************************************************
 *
 *  Redisplay
 *     General redisplay function called on exposure events.
 *
 ************************************************************************/
/* ARGSUSED */
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
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   int iwidth, iheight;

   iwidth = (int) aw->core.width - 2 * aw->primitive.highlight_thickness;
   iheight = (int) aw->core.height - 2 * aw->primitive.highlight_thickness;

   /*  Draw the arrow  */

   if ((iwidth > 0) && (iheight > 0))
   {
     if (aw -> primitive.shadow_thickness > 0)
       _XmDrawShadows (XtDisplay (aw), XtWindow (aw),
                    aw -> primitive.top_shadow_GC,
                    aw -> primitive.bottom_shadow_GC,
                    aw -> primitive.highlight_thickness,
                    aw -> primitive.highlight_thickness,
                    aw->core.width - 2 * aw->primitive.highlight_thickness,
                    aw->core.height-2 * aw->primitive.highlight_thickness,
                    aw -> primitive.shadow_thickness,
                    XmSHADOW_OUT);

     if (aw->arrowbutton.selected && aw->core.sensitive)
       DrawArrow(aw, aw->primitive.bottom_shadow_GC,
		 aw->primitive.top_shadow_GC,
		 ((aw->core.sensitive && aw->core.ancestor_sensitive) ?
		  aw->arrowbutton.arrow_GC : aw->arrowbutton.insensitive_GC));
     else
       DrawArrow(aw, aw -> primitive.top_shadow_GC,
		 aw -> primitive.bottom_shadow_GC,
		 ((aw->core.sensitive && aw->core.ancestor_sensitive) ?
		  aw->arrowbutton.arrow_GC : aw->arrowbutton.insensitive_GC));
   }

   if (aw -> primitive.highlighted)
   {   
       (*(xmArrowButtonClassRec.primitive_class.border_highlight))(
                                                                 (Widget) aw) ;
       } 
   else
   {   if (_XmDifferentBackground ((Widget) aw, XtParent (aw)))
       {   
           (*(xmArrowButtonClassRec.primitive_class.border_unhighlight))(
                                                                 (Widget) aw) ;
           } 
       } 
}

/************************************************************************
 *
 *  Destroy
 *	Clean up allocated resources when the widget is destroyed.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmArrowButtonWidget aw = (XmArrowButtonWidget) w ;

   if (aw->arrowbutton.timer)
      XtRemoveTimeOut (aw->arrowbutton.timer);

   XtReleaseGC (w, aw -> arrowbutton.arrow_GC);
   XtReleaseGC (w, aw -> arrowbutton.insensitive_GC);

   XtRemoveAllCallbacks (w, XmNactivateCallback);
   XtRemoveAllCallbacks (w, XmNarmCallback);
   XtRemoveAllCallbacks (w, XmNdisarmCallback);
}




/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw)
        Widget cw ;
        Widget rw ;
        Widget nw ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args, 
        Cardinal *num_args)
#endif /* _NO_PROTO */
{
   XmArrowButtonWidget current = (XmArrowButtonWidget) cw ;
   XmArrowButtonWidget new_w = (XmArrowButtonWidget) nw ;

   Boolean returnFlag = FALSE;
    
    /*  Check the data put into the new widget.  */

   if(    !XmRepTypeValidValue( XmRID_ARROW_DIRECTION, 
                                 new_w->arrowbutton.direction, (Widget) new_w)    )
   {
      new_w -> arrowbutton.direction = current -> arrowbutton.direction;
   }


   /*  See if the GC's need to be regenerated and widget redrawn.  */

   if (new_w -> core.background_pixel != current -> core.background_pixel ||
       new_w -> primitive.foreground != current -> primitive.foreground)
   {
      returnFlag = TRUE;
      XtReleaseGC ((Widget) new_w, new_w -> arrowbutton.arrow_GC);
      XtReleaseGC ((Widget) new_w, new_w -> arrowbutton.insensitive_GC);
      GetArrowGC (new_w);
   }

   if (new_w -> arrowbutton.direction != current-> arrowbutton.direction ||
       new_w -> core.ancestor_sensitive != current-> core.ancestor_sensitive ||
       new_w -> core.sensitive != current-> core.sensitive ||
       new_w -> primitive.highlight_thickness != 
          current -> primitive.highlight_thickness                     ||
       new_w -> primitive.shadow_thickness !=
          current -> primitive.shadow_thickness)
   {
      returnFlag = TRUE;
   }

   return (returnFlag);
}




/************************************************************************
 *
 *  arm
 *     This function processes button 1 down occuring on the arrowButton.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
Arm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
	String *params ;
	Cardinal *num_params;
#else
Arm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   XmArrowButtonCallbackStruct call_value;

   (void) XmProcessTraversal((Widget) aw, XmTRAVERSE_CURRENT);

   aw -> arrowbutton.selected = True;
   aw -> arrowbutton.armTimeStamp = event->xbutton.time; /* see MultiActivate */

   DrawArrow(aw, aw->primitive.bottom_shadow_GC,
	     aw->primitive.top_shadow_GC, NULL);

   if (aw->arrowbutton.arm_callback)
   {
      XFlush(XtDisplay(aw));
      
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) aw, aw->arrowbutton.arm_callback, &call_value);
   }
}


static void 
#ifdef _NO_PROTO
MultiArm( aw, event )
        Widget aw ;
        XEvent *event ;
#else
MultiArm(
        Widget aw,
        XEvent *event,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
	if (((XmArrowButtonWidget) aw)->arrowbutton.multiClick 
	     == XmMULTICLICK_KEEP)
	   Arm (aw, event, NULL, NULL);
}



/************************************************************************
 *
 *  activate
 *     This function processes button 1 up occuring on the arrowButton.
 *     If the button 1 up occurred inside the button the activate
 *     callbacks are called.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
Activate( wid, buttonEvent )
        Widget wid ;
        XEvent *buttonEvent ;
#else
Activate(
        Widget wid,
        XEvent *buttonEvent,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;

   if (aw -> arrowbutton.selected == False)
      return;

   aw->arrowbutton.click_count = 1;
   ActivateCommon((Widget) aw, buttonEvent);
}

static void 
#ifdef _NO_PROTO
MultiActivate( wid, buttonEvent )
        Widget wid ;
        XEvent *buttonEvent ;
#else
MultiActivate(
        Widget wid,
        XEvent *buttonEvent,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
        XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   /* When a multi click sequence occurs and the user Button Presses and
    * holds for a length of time, the final release should look like a
    * new/separate activate.
    */

  if (aw->arrowbutton.multiClick == XmMULTICLICK_KEEP)
  {
   if ((buttonEvent->xbutton.time - aw->arrowbutton.armTimeStamp) > 
        XtGetMultiClickTime(XtDisplay(aw)))
     aw->arrowbutton.click_count = 1;
   else
     aw->arrowbutton.click_count++;

   ActivateCommon((Widget) aw, buttonEvent);
   Disarm ((Widget) aw, buttonEvent, NULL, NULL);
  }
}

static void 
#ifdef _NO_PROTO
ActivateCommon( wid, buttonEvent )
        Widget wid ;
        XEvent *buttonEvent ;
#else
ActivateCommon(
        Widget wid,
        XEvent *buttonEvent)
#endif /* _NO_PROTO */
{
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   XmArrowButtonCallbackStruct call_value;
   Dimension bw = aw->core.border_width;

   aw -> arrowbutton.selected = False;

   DrawArrow(aw, aw->primitive.top_shadow_GC,
	     aw->primitive.bottom_shadow_GC, NULL);

   if ((buttonEvent->xbutton.x >= -(int)bw) &&
       (buttonEvent->xbutton.x <  (int)(aw->core.width + bw)) &&
       (buttonEvent->xbutton.y >= -(int)bw) &&
       (buttonEvent->xbutton.y <  (int)(aw->core.height + bw)) &&
       (aw->arrowbutton.activate_callback))
   {
      XFlush(XtDisplay(aw));

      call_value.reason = XmCR_ACTIVATE;
      call_value.event = buttonEvent;
      call_value.click_count = aw->arrowbutton.click_count;

      if ((aw->arrowbutton.multiClick == XmMULTICLICK_DISCARD) &&
	  (call_value.click_count > 1)) { 
	  return;
      }

      XtCallCallbackList ((Widget) aw, aw->arrowbutton.activate_callback, &call_value);
   }
}




/************************************************************************
 *
 *     ArmAndActivate
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
ArmAndActivate( wid, event )
        Widget wid ;
        XEvent *event ;
#else
ArmAndActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
        XmArrowButtonWidget ab = (XmArrowButtonWidget) wid ;
   XmArrowButtonCallbackStruct call_value;

   ab -> arrowbutton.selected = TRUE;
   (*(ab->core.widget_class->core_class.expose))
      ((Widget) ab, event, FALSE);

   XFlush (XtDisplay (ab));

   if (ab->arrowbutton.arm_callback)
   {
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) ab, ab->arrowbutton.arm_callback, &call_value);
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
   call_value.click_count = 1;  /* always 1 in kselect */

   if (ab->arrowbutton.activate_callback)
   {
      XFlush (XtDisplay (ab));
      XtCallCallbackList((Widget)ab,ab->arrowbutton.activate_callback,
                        &call_value);
   }

   ab -> arrowbutton.selected = FALSE;

   if (ab->arrowbutton.disarm_callback)
   {
      XFlush (XtDisplay (ab));
      call_value.reason = XmCR_DISARM;
      XtCallCallbackList ((Widget) ab, ab->arrowbutton.disarm_callback,
                        &call_value);
   }

   /* If the button is still around, show it released, after a short delay */

   if (ab->core.being_destroyed == False)
   {
      ab->arrowbutton.timer = 
         XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)ab),
                         (unsigned long) DELAY_DEFAULT,
                         ArmTimeout, (XtPointer)ab);
   }
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
ArmTimeout( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
ArmTimeout(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
        XmArrowButtonWidget ab = (XmArrowButtonWidget) closure ;
   ab -> arrowbutton.timer = 0;
   if (XtIsRealized (ab) && XtIsManaged (ab)) {
	  (*(ab->core.widget_class->core_class.expose))
           ((Widget) ab, NULL, FALSE);
      XFlush (XtDisplay (ab));
   }
   return;
}

        

/************************************************************************
 *
 *  disarm
 *     This function processes button 1 up occuring on the arrowButton.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
Disarm( wid, event )
        Widget wid ;
        XEvent *event ;
#else
Disarm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params)
#endif /* _NO_PROTO */
{
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   XmArrowButtonCallbackStruct call_value;

   aw -> arrowbutton.selected = False;

   DrawArrow(aw, aw->primitive.top_shadow_GC,
	     aw->primitive.bottom_shadow_GC, NULL);

   call_value.reason = XmCR_DISARM;
   call_value.event = event;
   XtCallCallbackList ((Widget) aw, aw->arrowbutton.disarm_callback, &call_value);
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
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   _XmPrimitiveEnter (wid, event, params, num_params);

   if (aw->arrowbutton.selected && aw->core.sensitive)
     DrawArrow(aw, aw->primitive.bottom_shadow_GC,
	       aw->primitive.top_shadow_GC, NULL);
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
   XmArrowButtonWidget aw = (XmArrowButtonWidget) wid ;
   _XmPrimitiveLeave (wid, event, params, num_params);

   if (aw->arrowbutton.selected && aw->core.sensitive)
     DrawArrow(aw, aw->primitive.top_shadow_GC,
	       aw->primitive.bottom_shadow_GC, NULL);
}


/************************************************************************
 *
 *  XmCreateArrowButton
 *	Create an instance of an arrowbutton and return the widget id.
 *
 ************************************************************************/

Widget 
#ifdef _NO_PROTO
XmCreateArrowButton( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateArrowButton(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmArrowButtonWidgetClass, 
                           parent, arglist, argcount));
}

/* Wrapper around _XmDrawArrow to calculate sizes. */
static void
#ifdef _NO_PROTO
DrawArrow(aw, top_gc, bottom_gc, center_gc)
     XmArrowButtonWidget aw;
     GC			 top_gc;
     GC			 bottom_gc;
     GC			 center_gc;
#else
DrawArrow(XmArrowButtonWidget aw,
	  GC		      top_gc,
	  GC		      bottom_gc,
	  GC		      center_gc)
#endif /* _NO_PROTO */
{
  Position x, y;
  Dimension width, height;
  Dimension shadow = 
    aw->primitive.highlight_thickness + aw->primitive.shadow_thickness;

  /* Don't let large shadows cause confusion. */
  if (shadow <= (aw->core.width / 2))
    {
      x = shadow;
      width = aw->core.width - (shadow * 2);
    }
  else
    {
      x = aw->core.width / 2;
      width = 0;
    }

  if (shadow <= (aw->core.height / 2))
    {
      y = shadow;
      height = aw->core.height - (shadow * 2);
    }
  else
    {
      y = aw->core.height / 2;
      height = 0;
    }

  /* ShadowThickness is hardcoded to 2 for 1.1 compatibility. */
  _XmDrawArrow (XtDisplay ((Widget) aw), XtWindow ((Widget) aw),
		top_gc, bottom_gc, center_gc,
		x, y, width, height, 2, aw->arrowbutton.direction);
}
