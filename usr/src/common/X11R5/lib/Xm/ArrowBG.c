#pragma ident	"@(#)m1.2libs:Xm/ArrowBG.c	1.4"
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
#include <Xm/ArrowBGP.h>
#include <Xm/ManagerP.h>
#include "RepTypeI.h"
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
static Boolean VisualChange() ;
static void InputDispatch() ;
static void Arm() ;
static void Activate() ;
static void ArmAndActivate() ;
static void ArmTimeout() ;
static void Disarm() ;
static void Enter() ;
static void Leave() ;
static void Help() ;
static void ActivateCommonG() ;
static void DrawArrowG() ;

#else

static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetArrowGC( 
                        XmArrowButtonGadget ag) ;
static void Redisplay( 
                        Widget w,
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
static Boolean VisualChange( 
                        Widget gad,
                        Widget cmw,
                        Widget nmw) ;
static void InputDispatch( 
                        Widget wid,
                        XEvent *event,
                        Mask event_mask) ;
static void Arm( 
                        XmArrowButtonGadget aw,
                        XEvent *event) ;
static void Activate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmAndActivate( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmTimeout( 
                        XtPointer data,
                        XtIntervalId *id) ;
static void Disarm( 
                        XmArrowButtonGadget aw,
                        XEvent *event) ;
static void Enter( 
                        XmArrowButtonGadget aw,
                        XEvent *event) ;
static void Leave( 
                        XmArrowButtonGadget aw,
                        XEvent *event) ;
static void Help( 
                        XmArrowButtonGadget aw,
                        XEvent *event) ;
static void ActivateCommonG( 
                        XmArrowButtonGadget ag,
                        XEvent *event,
                        Mask event_mask) ;
static void DrawArrowG(
		        XmArrowButtonGadget ag,
		        GC top_gc,
		        GC bottom_gc,
		        GC center_gc) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*  Resource list for Arrow  */

static XtResource resources[] = 
{
   {
     XmNmultiClick,
     XmCMultiClick,
     XmRMultiClick,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmArrowButtonGadgetRec, arrowbutton.multiClick),
     XmRImmediate,
     (XtPointer) XmMULTICLICK_KEEP
   },

   {
      XmNarrowDirection, XmCArrowDirection, XmRArrowDirection, 
      sizeof(unsigned char),
      XtOffsetOf( struct _XmArrowButtonGadgetRec, arrowbutton.direction), 
      XmRImmediate, (XtPointer) XmARROW_UP
   },

   {
     XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonGadgetRec, arrowbutton.activate_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonGadgetRec, arrowbutton.arm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNdisarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmArrowButtonGadgetRec, arrowbutton.disarm_callback),
     XmRPointer, (XtPointer) NULL
   }
};


/*  The Arrow class record definition  */

externaldef(xmarrowbuttongadgetclassrec) XmArrowButtonGadgetClassRec xmArrowButtonGadgetClassRec =
{
   {
      (WidgetClass) &xmGadgetClassRec,  /* superclass            */
      "XmArrowButtonGadget",            /* class_name	         */
      sizeof(XmArrowButtonGadgetRec),   /* widget_size	         */
      (XtProc)NULL,                     /* class_initialize      */
      ClassPartInitialize,              /* class_part_initialize */
      FALSE,                            /* class_inited          */
      Initialize,                       /* initialize	         */
      (XtArgsProc)NULL,                 /* initialize_hook       */
      NULL,        			  /* realize	         */
      NULL,                             /* actions               */
      0,			        /* num_actions    	 */
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
      NULL, 				  /* accept_focus         */	
      XtVersion,                        /* version               */
      (XtPointer)NULL,                  /* callback private      */
      (String)NULL,                     /* tm_table              */
      (XtGeometryHandler)NULL,          /* query_geometry        */
      NULL,				  /* display_accelerator   */
      (XtPointer)NULL,			/* extension             */
   },

   {
      XmInheritBorderHighlight,      			/* border highlight   */
      XmInheritBorderUnhighlight,			/* border_unhighlight */
      ArmAndActivate,					/* arm_and_activate   */
      InputDispatch,					/* input dispatch     */
      VisualChange,					/* visual_change      */
      (XmSyntheticResource *)NULL,			/* syn resources      */
      0,						/* num syn_resources  */
      (XmCacheClassPartPtr)NULL,	 	        /* class cache part   */
      (XtPointer) NULL,         			/* extension          */
   },

   {
      (XtPointer)NULL,			/* extension	      */
   }
};

externaldef(xmarrowbuttongadgetclass) WidgetClass xmArrowButtonGadgetClass = 
   (WidgetClass) &xmArrowButtonGadgetClassRec;



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
   _XmFastSubclassInit (wc, XmARROW_BUTTON_GADGET_BIT);
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
        Cardinal *num_args)
#endif /* _NO_PROTO */
{
        XmArrowButtonGadget request = (XmArrowButtonGadget) rw ;
        XmArrowButtonGadget new_w = (XmArrowButtonGadget) nw ;
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

   if (request->rectangle.width == 0) new_w->rectangle.width += 15;
   if (request->rectangle.height == 0) new_w->rectangle.height += 15;

   /*  Set the internal arrow variables */

   new_w->arrowbutton.timer = 0;
   new_w->arrowbutton.selected = False;

   /*  Get the drawing graphics contexts.  */

   GetArrowGC (new_w);


   /*  Initialize the interesting input types.  */

   new_w->gadget.event_mask =  XmARM_EVENT | XmACTIVATE_EVENT | XmHELP_EVENT |
        XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT | XmENTER_EVENT | XmLEAVE_EVENT
		| XmMULTI_ARM_EVENT|  XmMULTI_ACTIVATE_EVENT;

}




/************************************************************************
 *
 *  GetArrowGC
 *     Get the graphics context used for drawing the arrowbutton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetArrowGC( ag )
        XmArrowButtonGadget ag ;
#else
GetArrowGC(
        XmArrowButtonGadget ag )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;
   XImage *t_im;
   Pixmap t_pix;
   XmManagerWidget mw;

   mw = (XmManagerWidget) XtParent(ag);

   valueMask = GCForeground | GCBackground | GCFillStyle;

   values.foreground = mw->manager.foreground;
   values.background = mw->core.background_pixel;
   values.fill_style = FillSolid;

   ag->arrowbutton.arrow_GC = XtGetGC ((Widget) mw, valueMask, &values);

   valueMask = GCForeground | GCGraphicsExposures;
   values.graphics_exposures = False;

   if ((t_pix = XmGetPixmapByDepth(XtScreen((Widget) ag), 
		UNAVAILABLE_STIPPLE_NAME, (Pixel) 1, (Pixel) 0, 1))
                   == XmUNSPECIFIED_PIXMAP)
   {
       if (_XmGetImage(XtScreen((Widget) ag), "50_foreground", &t_im))
       {
           XGCValues t_values;
           XtGCMask  t_valueMask;
	   GC t_gc;

           t_pix = XCreatePixmap(XtDisplay((Widget) ag),
                                 RootWindowOfScreen(XtScreen((Widget) ag)),
                                 t_im->width, t_im->height, 1);

           t_values.foreground = 1;
           t_values.background = 0;
           t_valueMask = (GCForeground | GCBackground);
           t_gc = XCreateGC(XtDisplay((Widget) ag), t_pix, t_valueMask,
                            &t_values);

           XPutImage(XtDisplay((Widget) ag), t_pix, t_gc, t_im, 0, 0, 0, 0,
                     t_im->width, t_im->height);
	   XFreeGC(XtDisplay((Widget) ag), t_gc);

           values.fill_style = FillStippled;
           values.foreground = BlackPixelOfScreen(XtScreen(ag));
           values.stipple = t_pix;
           valueMask |= (GCFillStyle | GCStipple);

           _XmInstallPixmap(t_pix, XtScreen((Widget) ag),
                            UNAVAILABLE_STIPPLE_NAME, (Pixel) 1, (Pixel) 0);

       }
   }
   else
   {
       values.fill_style = FillStippled;
       values.foreground = BlackPixelOfScreen(XtScreen(ag));
       values.stipple = t_pix;
       valueMask |= (GCFillStyle | GCStipple);
   }

   ag->arrowbutton.insensitive_GC = XtGetGC((Widget) mw, valueMask, &values);
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
   XmArrowButtonGadget aw = (XmArrowButtonGadget) w ;
   int iwidth, iheight;

   iwidth = (int) aw->rectangle.width - 2 * aw->gadget.highlight_thickness;
   iheight = (int) aw->rectangle.height - 2 * aw->gadget.highlight_thickness;

   /*  Draw the arrow  */
   if ((iwidth > 0) && (iheight > 0))
   {
     if (aw->gadget.shadow_thickness > 0)
       _XmDrawShadows (XtDisplay (aw), XtWindow (aw),
                    XmParentTopShadowGC (aw),
                    XmParentBottomShadowGC (aw),
                    aw->rectangle.x + aw->gadget.highlight_thickness,
                    aw->rectangle.y + aw->gadget.highlight_thickness,
                    aw->rectangle.width - 2 *
                      aw->gadget.highlight_thickness,
                    aw->rectangle.height-2 *
                      aw->gadget.highlight_thickness,
                    aw->gadget.shadow_thickness, XmSHADOW_OUT);


     if (aw->arrowbutton.selected && aw->rectangle.sensitive)
       DrawArrowG(aw, XmParentBottomShadowGC (aw),
		  XmParentTopShadowGC (aw),
		  ((aw->rectangle.sensitive &&
		    aw->rectangle.ancestor_sensitive) ?
		   aw->arrowbutton.arrow_GC : aw->arrowbutton.insensitive_GC));
     else
       DrawArrowG(aw, XmParentTopShadowGC (aw),
		  XmParentBottomShadowGC (aw),
		  ((aw->rectangle.sensitive &&
		    aw->rectangle.ancestor_sensitive) ?
		   aw->arrowbutton.arrow_GC : aw->arrowbutton.insensitive_GC));
   }
   if (aw->gadget.highlighted)
   {   
       (*(xmArrowButtonGadgetClassRec.gadget_class.border_highlight))(
                                                                 (Widget) aw) ;
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
   XmArrowButtonGadget aw = (XmArrowButtonGadget) w ;
   XmManagerWidget mw = (XmManagerWidget) XtParent(aw);
   
   if (aw->arrowbutton.timer)
      XtRemoveTimeOut (aw->arrowbutton.timer);

   XtReleaseGC ((Widget) mw, aw->arrowbutton.arrow_GC);
   XtReleaseGC ((Widget) mw, aw->arrowbutton.insensitive_GC);

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
        Cardinal *num_args)
#endif /* _NO_PROTO */
{
   XmArrowButtonGadget current = (XmArrowButtonGadget) cw ;
   XmArrowButtonGadget new_w = (XmArrowButtonGadget) nw ;

   Boolean returnFlag = FALSE;
    

    /*  Check the data put into the new widget.  */

   if(    !XmRepTypeValidValue( XmRID_ARROW_DIRECTION, 
                                 new_w->arrowbutton.direction, (Widget) new_w)    )
   {
      new_w -> arrowbutton.direction = current -> arrowbutton.direction;
   }


   /*  ReInitialize the interesting input types.  */

   new_w->gadget.event_mask |=  XmARM_EVENT | XmACTIVATE_EVENT | XmHELP_EVENT |
        XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT | XmENTER_EVENT | XmLEAVE_EVENT
	    | XmMULTI_ARM_EVENT | XmMULTI_ACTIVATE_EVENT;

   if (new_w->arrowbutton.direction != current->arrowbutton.direction ||
       new_w ->rectangle.sensitive != current->rectangle.sensitive ||
       new_w ->rectangle.ancestor_sensitive != 
	  current->rectangle.ancestor_sensitive ||
       new_w->gadget.highlight_thickness != current->gadget.highlight_thickness ||
       new_w->gadget.shadow_thickness != current->gadget.shadow_thickness)
   {
      returnFlag = TRUE;
   }

   return (returnFlag);
}




/************************************************************************
 *
 *  VisualChange
 *	This function is called from XmManagerClass set values when
 *	the managers visuals have changed.  The gadget regenerates any
 *	GC based on the visual changes and returns True indicating a
 *	redraw is needed.  Otherwize, False is returned.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
VisualChange( gad, cmw, nmw )
        Widget gad ;
        Widget cmw ;
        Widget nmw ;
#else
VisualChange(
        Widget gad,
        Widget cmw,
        Widget nmw)
#endif /* _NO_PROTO */
{
        XmGadget gw = (XmGadget) gad;
        XmManagerWidget curmw = (XmManagerWidget) cmw;
        XmManagerWidget newmw = (XmManagerWidget) nmw;

   XmArrowButtonGadget abg = (XmArrowButtonGadget) gw;
   XmManagerWidget mw = (XmManagerWidget) XtParent(gw);

   if (curmw->manager.foreground != newmw->manager.foreground ||
       curmw->core.background_pixel != newmw->core.background_pixel)
   {
      XtReleaseGC ((Widget) mw, abg->arrowbutton.arrow_GC);
      XtReleaseGC ((Widget) mw, abg->arrowbutton.insensitive_GC);
      GetArrowGC (abg);
      return (True);
   }

   return (False);
}




/************************************************************************
 *
 *  InputDispatch
 *     This function catches input sent by a manager and dispatches it
 *     to the individual routines.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
InputDispatch( wid, event, event_mask )
        Widget wid ;
        XEvent *event ;
        Mask event_mask ;
#else
InputDispatch(
        Widget wid,
        XEvent *event,
        Mask event_mask )
#endif /* _NO_PROTO */
{
        XmArrowButtonGadget ag = (XmArrowButtonGadget) wid;

   if ((event_mask & XmARM_EVENT)  || 
       ((ag->arrowbutton.multiClick == XmMULTICLICK_KEEP) &&
       (event_mask & XmMULTI_ARM_EVENT)))
      Arm (ag, event);
   else if (event_mask & XmACTIVATE_EVENT)
   { 
      ag->arrowbutton.click_count = 1;  
      ActivateCommonG (ag, event, event_mask);
   }
   else if (event_mask & XmMULTI_ACTIVATE_EVENT)
   { 
      /* if XmNMultiClick resource is set to DISCARD - do nothing
       * else call ActivateCommon() and increment clickCount;
       */
      if (ag->arrowbutton.multiClick == XmMULTICLICK_KEEP)
      {  
	 ag->arrowbutton.click_count++;
	 ActivateCommonG ( ag, event, event_mask);
      }
   }

   else if (event_mask & XmHELP_EVENT) Help (ag, event);
   else if (event_mask & XmENTER_EVENT) Enter (ag, event);
   else if (event_mask & XmLEAVE_EVENT) Leave (ag, event);
   else if (event_mask & XmFOCUS_IN_EVENT) 
      _XmFocusInGadget ((Widget) ag, event, NULL, NULL);
   else if (event_mask & XmFOCUS_OUT_EVENT) 
      _XmFocusOutGadget ((Widget)ag, event, NULL, NULL);
}

/************************************************************************
 *
 *  arm
 *     This function processes button 1 down occuring on the arrowbutton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Arm( aw, event )
        XmArrowButtonGadget aw ;
        XEvent *event ;
#else
Arm(
        XmArrowButtonGadget aw,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmArrowButtonCallbackStruct call_value;

   aw->arrowbutton.selected = True;

   DrawArrowG(aw, XmParentBottomShadowGC(aw), XmParentTopShadowGC(aw), NULL);

   if (aw->arrowbutton.arm_callback)
   {
      XFlush(XtDisplay(aw));
      
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) aw, aw->arrowbutton.arm_callback, &call_value);
   }
}




/************************************************************************
 *
 *  activate
 *     This function processes button 1 up occuring on the arrowbutton.
 *     If the button 1 up occurred inside the button the activate
 *     callbacks are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Activate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Activate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmArrowButtonGadget aw = (XmArrowButtonGadget) wid ;
   XButtonPressedEvent *buttonEvent = (XButtonPressedEvent *) event ;

   XmPushButtonCallbackStruct call_value;

   aw->arrowbutton.selected = False;

   DrawArrowG(aw, XmParentTopShadowGC(aw), XmParentBottomShadowGC(aw), NULL);

   if (((buttonEvent->x < aw->rectangle.x + aw->rectangle.width) && 
      (buttonEvent->x >= aw->rectangle.x)) &&
      ((buttonEvent->y < aw->rectangle.y + aw->rectangle.height) &&
      (buttonEvent->y >= aw->rectangle.y)) &&
      (aw->arrowbutton.activate_callback))
   {
      XFlush(XtDisplay(aw));
      
      call_value.reason = XmCR_ACTIVATE;
      call_value.event = (XEvent *) buttonEvent;
	  call_value.click_count = aw->arrowbutton.click_count;
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
ArmAndActivate( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ArmAndActivate(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmArrowButtonGadget ab = (XmArrowButtonGadget) w ;
   XmPushButtonCallbackStruct call_value;

   ab -> arrowbutton.selected = TRUE;
   ab->arrowbutton.click_count = 1;
   (*(ab->object.widget_class->core_class.expose)) 
      ( (Widget) ab, event, FALSE);

   XFlush (XtDisplay (ab));

   if (ab->arrowbutton.arm_callback)
   {
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      call_value.click_count = ab->arrowbutton.click_count;
      XtCallCallbackList ((Widget) ab, ab->arrowbutton.arm_callback, &call_value);
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
   call_value.click_count = 1;          /* always 1 in kselect */

   if (ab->arrowbutton.activate_callback)
    {
       XFlush (XtDisplay (ab));
       XtCallCallbackList ((Widget) ab, ab->arrowbutton.activate_callback,
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

   if (ab->object.being_destroyed == False)
   {
      ab->arrowbutton.timer = XtAppAddTimeOut(
                                     XtWidgetToApplicationContext((Widget) ab),
                                                (unsigned long) DELAY_DEFAULT,
                                                ArmTimeout,
                                                (XtPointer)ab);
   }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ArmTimeout( data, id )
        XtPointer data ;
        XtIntervalId *id ;
#else
ArmTimeout(
        XtPointer data,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
   XmArrowButtonGadget ab = (XmArrowButtonGadget) data ;

   ab -> arrowbutton.timer = 0;
   if (XtIsRealized (ab) && XtIsManaged (ab)) {
      Redisplay ( (Widget) ab, NULL, FALSE);
      XFlush (XtDisplay (ab));
   }
   return;
}

        

/************************************************************************
 *
 *  disarm
 *     This function processes button 1 up occuring on the arrowbutton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Disarm( aw, event )
        XmArrowButtonGadget aw ;
        XEvent *event ;
#else
Disarm(
        XmArrowButtonGadget aw,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmArrowButtonCallbackStruct call_value;

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
Enter( aw, event )
        XmArrowButtonGadget aw ;
        XEvent *event ;
#else
Enter(
        XmArrowButtonGadget aw,
        XEvent *event )
#endif /* _NO_PROTO */
{
   _XmEnterGadget ((Widget) aw, event, NULL, NULL);

   if (aw->arrowbutton.selected && aw->rectangle.sensitive)
     DrawArrowG(aw, XmParentBottomShadowGC(aw), XmParentTopShadowGC(aw), NULL);
}


/************************************************************************
 *
 *  Leave
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Leave( aw, event )
        XmArrowButtonGadget aw ;
        XEvent *event ;
#else
Leave(
        XmArrowButtonGadget aw,
        XEvent *event )
#endif /* _NO_PROTO */
{
   _XmLeaveGadget ((Widget) aw, event, NULL, NULL);

   if (aw->arrowbutton.selected && aw->rectangle.sensitive)
     DrawArrowG(aw, XmParentTopShadowGC(aw), XmParentBottomShadowGC(aw), NULL);
}


/************************************************************************
 *
 *  Help
 *     This function processes Function Key 1 press occuring on 
 *     the arrowbutton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( aw, event )
        XmArrowButtonGadget aw ;
        XEvent *event ;
#else
Help(
        XmArrowButtonGadget aw,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmArrowButtonCallbackStruct call_value;

   call_value.reason = XmCR_HELP;
   call_value.event = event;
   _XmSocorro((Widget) aw, event, NULL, NULL);
}
 



/************************************************************************
 *
 *  XmCreateArrowButtonGadget
 *	Create an instance of an arrowbutton and return the widget id.
 *
 ************************************************************************/

Widget 
#ifdef _NO_PROTO
XmCreateArrowButtonGadget( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateArrowButtonGadget(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmArrowButtonGadgetClass, 
                           parent, arglist, argcount));
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ActivateCommonG( ag, event, event_mask )
        XmArrowButtonGadget ag ;
        XEvent *event ;
        Mask event_mask ;
#else
ActivateCommonG(
        XmArrowButtonGadget ag,
        XEvent *event,
        Mask event_mask )
#endif /* _NO_PROTO */
{
      if (event->type == ButtonRelease)
         {
            Activate( (Widget) ag, event, NULL, NULL);
            Disarm ( ag, event);
         }
         else  /* assume KeyPress or KeyRelease */
        (* (((XmArrowButtonGadgetClassRec *)(ag->object.widget_class))->
			gadget_class.arm_and_activate))
			( (Widget) ag, event, NULL, NULL) ;
}

/* Wrapper around _XmDrawArrow to calculate sizes. */
static void
#ifdef _NO_PROTO
DrawArrowG(ag, top_gc, bottom_gc, center_gc)
     XmArrowButtonGadget ag;
     GC			 top_gc;
     GC			 bottom_gc;
     GC			 center_gc;
#else
DrawArrowG(XmArrowButtonGadget ag,
	   GC		       top_gc,
	   GC		       bottom_gc,
	   GC		       center_gc)
#endif /* _NO_PROTO */
{
  Position x, y;
  Dimension width, height;
  Dimension shadow = 
    ag->gadget.highlight_thickness + ag->gadget.shadow_thickness;

  /* Don't let large shadows cause confusion. */
  if (shadow <= (ag->rectangle.width / 2))
    {
      x = ag->rectangle.x + shadow;
      width = ag->rectangle.width - (shadow * 2);
    }
  else
    {
      x = ag->rectangle.x + ag->rectangle.width / 2;
      width = 0;
    }

  if (shadow <= (ag->rectangle.height / 2))
    {
      y = ag->rectangle.y + shadow;
      height = ag->rectangle.height - (shadow * 2);
    }
  else
    {
      y = ag->rectangle.y + ag->rectangle.height / 2;
      height = 0;
    }

  /* ShadowThickness is hardcoded to 2 for 1.1 compatibility. */
  _XmDrawArrow (XtDisplay ((Widget) ag), XtWindow ((Widget) ag),
		top_gc, bottom_gc, center_gc,
		x, y, width, height, 2, ag->arrowbutton.direction);
}
