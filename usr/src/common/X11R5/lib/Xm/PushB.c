#pragma ident	"@(#)m1.2libs:Xm/PushB.c	1.5"
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

#include <Xm/PushBP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ManagerP.h>
#include <stdio.h>
#include "XmI.h"
#include <X11/ShellP.h>
#include <Xm/CascadeB.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/MenuUtilP.h>
#ifdef CDE_VISUAL	/* etched in menu button */
#include <Xm/TearOffBP.h>
#endif
#include "TravActI.h"

#define XmINVALID_MULTICLICK 255
#define DELAY_DEFAULT 100	

struct	PBbox
     { int pbx;
       int pby;
       int pbWidth;
       int pbHeight;
     };

struct  PBTimeOutEvent
		{  XmPushButtonWidget  pushbutton;
	       XEvent			   *xevent;
		} ;


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
static void BtnDown() ;
static void BtnUp() ;
static void Enter() ;
static void Leave() ;
static void BorderHighlight() ;
static void DrawBorderHighlight() ;
static void BorderUnhighlight() ;
static void KeySelect() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void InitializePrehook() ;
static void InitializePosthook() ;
static void Initialize() ;
static void GetFillGC() ;
static void GetBackgroundGC() ;
static Boolean SetValues() ;
static void Help() ;
static void Destroy() ;
static void Resize();
static void EraseDefaultButtonShadow() ;
static void Redisplay() ;
static void DrawPushButtonBackground() ;
static void DrawPushButtonLabel() ;
static void DrawPushButtonShadows() ;
static Boolean ComputePBLabelArea() ;
static GC GetParentBackgroundGC() ;
static void DrawPBPrimitiveShadows() ;
static void DrawDefaultButtonShadows() ;
static XmImportOperator ShowAsDef_ToHorizPix() ;
static int AdjustHighLightThickness() ;
static void ExportHighlightThickness() ;
static void FillBorderWithParentColor() ;
static void SetPushButtonSize();
#ifdef CDE_VISUAL	/* etched in menu button */
static void DrawArmedMenuLabel();
#endif

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
static void ArmTimeout( 
                        XtPointer data,
                        XtIntervalId *id) ;
static void Disarm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BtnDown( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BtnUp( 
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
static void BorderHighlight( 
                        Widget wid) ;
static void DrawBorderHighlight( 
                        Widget wid) ;
static void BorderUnhighlight( 
                        Widget wid) ;
static void KeySelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void InitializePrehook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InitializePosthook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetFillGC( 
                        XmPushButtonWidget pb) ;
static void GetBackgroundGC( 
                        XmPushButtonWidget pb) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Help( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Destroy( 
                        Widget w) ;
static void Resize(
                        Widget w) ;
static void EraseDefaultButtonShadow( 
                        XmPushButtonWidget pb) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void DrawPushButtonBackground( 
                        XmPushButtonWidget pb) ;
static void DrawPushButtonLabel( 
                        XmPushButtonWidget pb,
                        XEvent *event,
                        Region region) ;
static void DrawPushButtonShadows( 
                        XmPushButtonWidget pb) ;
static Boolean ComputePBLabelArea( 
                        XmPushButtonWidget pb,
                        struct PBbox *box) ;
static GC GetParentBackgroundGC( 
                        XmPushButtonWidget pb) ;
static void DrawPBPrimitiveShadows( 
                        XmPushButtonWidget pb) ;
static void DrawDefaultButtonShadows( 
                        XmPushButtonWidget pb) ;
static XmImportOperator ShowAsDef_ToHorizPix( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
static int AdjustHighLightThickness( 
                        XmPushButtonWidget new_w,
                        XmPushButtonWidget current) ;
static void ExportHighlightThickness( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
static void FillBorderWithParentColor( 
                        XmPushButtonWidget pb,
                        int borderwidth,
                        int dx,
                        int dy,
                        int rectwidth,
                        int rectheight) ;
static void SetPushButtonSize(
                        XmPushButtonWidget newtb) ;
#ifdef CDE_VISUAL	/* etched in menu button */
static void DrawArmedMenuLabel(
        XmPushButtonWidget pb,
        XEvent *event,
        Region region );
#endif

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*************************************<->*************************************
 *
 *
 *   Description:   translation tables for class: PushButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/
static XtTranslations default_parsed;

#define defaultTranslations	_XmPushB_defaultTranslations

static XtTranslations menu_parsed;

#define menuTranslations	_XmPushB_menuTranslations


/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: PushButton
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
  {"MultiArm", MultiArm },
  {"Activate", 	Activate	 },
  {"MultiActivate", 	MultiActivate	 },
  {"ArmAndActivate", ArmAndActivate },
  {"Disarm", 	Disarm		 },
  {"BtnDown", 	BtnDown		 },
  {"BtnUp", 	BtnUp		 },
  {"Enter", 	Enter		 },
  {"Leave",	Leave		 },
  {"KeySelect",	KeySelect	 },
  {"Help",	Help		 },
};

/* Definition for resources that need special processing in get values */

static XmSyntheticResource syn_resources[] =
{
  {
     XmNshowAsDefault, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.show_as_default),
     _XmFromHorizontalPixels,
     ShowAsDef_ToHorizPix
  },

  {
     XmNdefaultButtonShadowThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.default_button_shadow_thickness ),
     _XmFromHorizontalPixels,  
     _XmToHorizontalPixels
  },

   {
     XmNhighlightThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPrimitiveRec, primitive.highlight_thickness),
     ExportHighlightThickness,
     _XmToHorizontalPixels
   },

};


/*  The resource list for Push Button  */

static XtResource resources[] = 
{     
   {
     XmNmultiClick,
     XmCMultiClick,
     XmRMultiClick,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.multiClick),
     XmRImmediate, (XtPointer) XmINVALID_MULTICLICK
   },

   {
     XmNfillOnArm,
     XmCFillOnArm,
     XmRBoolean,
     sizeof (Boolean),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.fill_on_arm),
     XmRImmediate, (XtPointer) True
   },

   {
     XmNarmColor,
     XmCArmColor,
     XmRPixel,
     sizeof (Pixel),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.arm_color),
     XmRCallProc, (XtPointer) _XmSelectColorDefault
   },

   {
     XmNarmPixmap,
     XmCArmPixmap,
     XmRPrimForegroundPixmap,
     sizeof (Pixmap),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.arm_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },

   {
     XmNshowAsDefault,
     XmCShowAsDefault,
     XmRBooleanDimension,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.show_as_default),
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNactivateCallback,
     XmCCallback,
     XmRCallback,
     sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.activate_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNarmCallback,
     XmCCallback,
     XmRCallback,
     sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.arm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNdisarmCallback,
     XmCCallback,
     XmRCallback,
     sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.disarm_callback),
     XmRPointer, (XtPointer) NULL
   },
   
   {
     XmNshadowThickness,
     XmCShadowThickness,
     XmRHorizontalDimension,
     sizeof(Dimension),
     XtOffsetOf( struct _XmPushButtonRec, primitive.shadow_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNdefaultButtonShadowThickness,
     XmCDefaultButtonShadowThickness, 
     XmRHorizontalDimension, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonRec, pushbutton.default_button_shadow_thickness ),
     XmRImmediate,
     (XtPointer) 0
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
   },
};



/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: PushButton
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/

static XmBaseClassExtRec       pushBBaseClassExtRec = {
    NULL,                                     /* Next extension       */
    NULLQUARK,                                /* record type XmQmotif */
    XmBaseClassExtVersion,                    /* version              */
    sizeof(XmBaseClassExtRec),                /* size                 */
    InitializePrehook,                        /* initialize prehook   */
    XmInheritSetValuesPrehook,                /* set_values prehook   */
    InitializePosthook,                       /* initialize posthook  */
    XmInheritSetValuesPosthook,               /* set_values posthook  */
    XmInheritClass,                           /* secondary class      */
    XmInheritSecObjectCreate,                 /* creation proc        */
    XmInheritGetSecResData,                   /* getSecResData        */
    {0},                                      /* fast subclass        */
    XmInheritGetValuesPrehook,                /* get_values prehook   */
    XmInheritGetValuesPosthook,               /* get_values posthook  */
    (XtWidgetClassProc)NULL,                  /* classPartInitPrehook */
    (XtWidgetClassProc)NULL,                  /* classPartInitPosthook*/
    NULL,                                     /* ext_resources        */
    NULL,                                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    XmInheritWidgetNavigable,                 /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
};

XmPrimitiveClassExtRec _XmPushBPrimClassExtRec = {
    NULL,
    NULLQUARK,
    XmPrimitiveClassExtVersion,
    sizeof(XmPrimitiveClassExtRec),
    XmInheritBaselineProc,                  /* widget_baseline */
    XmInheritDisplayRectProc,               /* widget_display_rect */
    (XmWidgetMarginsProc)NULL,              /* widget_margins */
};

externaldef(xmpushbuttonclassrec)  
	XmPushButtonClassRec xmPushButtonClassRec = {
  {
/* core_class record */	
    /* superclass	  */	(WidgetClass) &xmLabelClassRec,
    /* class_name	  */	"XmPushButton",
    /* widget_size	  */	sizeof(XmPushButtonRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */    (XtArgsProc)NULL,
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
    /* resize		  */	Resize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    (XtArgsFunc)NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	(XtArgsProc)NULL,
    /* accept_focus	  */	(XtAcceptFocusProc)NULL,
    /* version            */	XtVersion,
    /* callback_private   */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry     */	XtInheritQueryGeometry, 
    /* display_accelerator */   (XtStringProc)NULL,
    /* extension record   */    (XtPointer)&pushBBaseClassExtRec,
  },

  { /* primitive_class record       */

    /* Primitive border_highlight   */	BorderHighlight,
    /* Primitive border_unhighlight */	BorderUnhighlight,
    /* translations		    */  XtInheritTranslations,
    /* arm_and_activate		    */  ArmAndActivate,
    /* get resources		    */  syn_resources,
    /* num get_resources	    */  XtNumber(syn_resources),
    /* extension		    */  (XtPointer)&_XmPushBPrimClassExtRec,
  },

  { /* label_class record */
 
    /* setOverrideCallback*/	XmInheritWidgetProc,
    /* menu procedures    */	XmInheritMenuProc,
    /* menu traversal xlation */ XtInheritTranslations,
    /* extension	  */	(XtPointer) NULL,
  },

  { /* pushbutton_class record */

    /* extension	  */	(XtPointer) NULL,
  }

};
externaldef(xmpushbuttonwidgetclass)
   WidgetClass xmPushButtonWidgetClass = (WidgetClass)&xmPushButtonClassRec;


/************************************************************************
 *
 *     Arm
 *
 *     This function processes button 1 down occuring on the pushbutton.
 *     Mark the pushbutton as armed (i.e. active).
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;

   (void) XmProcessTraversal( (Widget) pb, XmTRAVERSE_CURRENT);

   pb -> pushbutton.armed = TRUE;

   if (event != NULL &&
      (event->xany.type == ButtonPress ||
       event->xany.type == ButtonRelease))
      pb -> pushbutton.armTimeStamp = event->xbutton.time;
   else
      pb -> pushbutton.armTimeStamp = 0;

   (* XtClass(pb)->core_class.expose)(wid, event, (Region) NULL);

   if (pb->pushbutton.arm_callback)
   {
      XFlush(XtDisplay (pb));

      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.arm_callback, &call_value);
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;

    if (pb->pushbutton.multiClick == XmMULTICLICK_KEEP)
            Arm ((Widget) pb, event, NULL, NULL);
}



/************************************************************************
 *
 *     Activate
 *
 *     Mark the pushbutton as unarmed (i.e. inactive).
 *     If the button release occurs inside of the PushButton, the 
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   if (pb -> pushbutton.armed == FALSE)
      return;

   pb->pushbutton.click_count = 1;
   ActivateCommon ((Widget) pb, buttonEvent, params, num_params);
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   /* When a multi click sequence occurs and the user Button Presses and
    * holds for a length of time, the final release should look like a
    * new_w/separate activate.
    */
   if (pb->pushbutton.multiClick == XmMULTICLICK_KEEP)
  {
     if ((buttonEvent->xbutton.time - pb->pushbutton.armTimeStamp) >
        XtGetMultiClickTime(XtDisplay(pb)))
          pb->pushbutton.click_count = 1;
      else
        pb->pushbutton.click_count++;
     ActivateCommon ((Widget) pb, buttonEvent, params, num_params);
     Disarm ((Widget) pb, buttonEvent, params, num_params);
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;
   Dimension bw = pb->core.border_width ;

   pb -> pushbutton.armed = FALSE;

   (* ((WidgetClass)XtClass(pb))->core_class.expose)(wid, event, (Region) NULL);

   if ((event->xany.type == ButtonPress ||
        event->xany.type == ButtonRelease) &&
       (event->xbutton.x >= -(int)bw) &&
       (event->xbutton.x < (int)(pb->core.width + bw)) &&
       (event->xbutton.y >= -(int)bw) &&
       (event->xbutton.y < (int)(pb->core.height + bw)))
   {
       call_value.reason = XmCR_ACTIVATE;
       call_value.event = event;
       call_value.click_count = pb->pushbutton.click_count;

       if ((pb->pushbutton.multiClick == XmMULTICLICK_DISCARD) &&	
	   (call_value.click_count > 1)) {
           return;
	}

       /* if the parent is a RowColumn, notify it about the select */
       if (XmIsRowColumn(XtParent(pb)))
       {
	  (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(pb), FALSE, pb,
						    &call_value);
       }

       if ((! pb->label.skipCallback) &&
	   (pb->pushbutton.activate_callback))
       {
	  XFlush (XtDisplay (pb));
	  XtCallCallbackList ((Widget) pb, pb->pushbutton.activate_callback,
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
   XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   Boolean already_armed = pb -> pushbutton.armed;
   XmPushButtonCallbackStruct call_value;
   Boolean is_menupane = (pb ->label.menu_type == XmMENU_PULLDOWN) ||
			 (pb ->label.menu_type == XmMENU_POPUP);
   Boolean parent_is_torn;
   Boolean torn_has_focus = FALSE;	/* must be torn! */

   if (is_menupane && !XmIsMenuShell(XtParent(XtParent(pb))))
   {
      parent_is_torn = TRUE;
      /* Because the pane is torn and the parent is a transient shell,
       * the shell's focal point from _XmGetFocusData should be valid
       * (as opposed to getting it from a MenuShell).
       */
      if (_XmFocusIsInShell((Widget)pb))
      {
         /* In case allowAcceleratedInsensitiveUnmanagedMenuItems is True */
         if (!XtIsSensitive((Widget)pb) || (!XtIsManaged((Widget)pb)))
            return;
	 torn_has_focus = TRUE;
      }
   }
   else
      parent_is_torn = FALSE;

   if (is_menupane)
   {
      pb -> pushbutton.armed = FALSE;

      if (parent_is_torn && !torn_has_focus)
      {
         /* Freeze tear off visuals in case accelerators are not in
          * same context
          */
         (* xmLabelClassRec.label_class.menuProcs)
            (XmMENU_RESTORE_TEAROFF_TO_MENUSHELL, XtParent(pb), NULL,
            event, NULL);
      }

      if (torn_has_focus)
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_POPDOWN, XtParent(pb), NULL, event, NULL);
      else
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);

      /* if its in a torn off menu pane, show depressed button briefly */
      if (torn_has_focus)	/*parent_is_torn!*/
      {
	  /* Set the focus here. */
	  XmProcessTraversal((Widget) pb, XmTRAVERSE_CURRENT);

	  _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
	     pb -> primitive.bottom_shadow_GC, pb -> primitive.top_shadow_GC,
	     pb -> primitive.highlight_thickness,
	     pb -> primitive.highlight_thickness,
	     pb -> core.width - 2 * pb->primitive.highlight_thickness,
	     pb -> core.height - 2 * pb->primitive.highlight_thickness,
	     pb -> primitive.shadow_thickness,
	     XmSHADOW_OUT);
      }
   }
   else 
   {
      pb -> pushbutton.armed = TRUE;
      (* XtClass(pb)->core_class.expose)(wid, event, (Region) NULL);
   }

   XFlush (XtDisplay (pb));

   /* If the parent is a RowColumn, set the lastSelectToplevel before the arm.
    * It's ok if this is recalled later.
    */
   if (XmIsRowColumn(XtParent(pb)))
   {
      (* xmLabelClassRec.label_class.menuProcs) (
	 XmMENU_GET_LAST_SELECT_TOPLEVEL, XtParent(pb));
   }

   if (pb->pushbutton.arm_callback && !already_armed)
   {
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList((Widget)pb, pb->pushbutton.arm_callback, &call_value);
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
   call_value.click_count = 1;	           /* always 1 in kselect */

   /* if the parent is a RowColumn, notify it about the select */
   if (XmIsRowColumn(XtParent(pb)))
   {
      (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
	 XtParent(pb), FALSE, pb, &call_value);
   }

   if ((! pb->label.skipCallback) && (pb->pushbutton.activate_callback))
   {
      XFlush (XtDisplay (pb));
      XtCallCallbackList ((Widget) pb, pb->pushbutton.activate_callback,
			     &call_value);
   }

   pb -> pushbutton.armed = FALSE;
   
   if (pb->pushbutton.disarm_callback)
   {
      XFlush (XtDisplay (pb));
      call_value.reason = XmCR_DISARM;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.disarm_callback,
	 &call_value);
   }

   if (is_menupane)
   {
      if (torn_has_focus)
      {
	 /* Leave the focus widget in an armed state */
	 pb -> pushbutton.armed = TRUE;

	 if (pb->pushbutton.arm_callback)
	 {
	    XFlush (XtDisplay (pb));
	    call_value.reason = XmCR_ARM;
	    XtCallCallbackList ((Widget) pb, pb->pushbutton.arm_callback,
				&call_value);
	 }
      } else
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(pb), NULL, event, NULL);
   }

   /*
    * If the button is still around, show it released, after a short delay.
    * This is done if the button is outside of a menus, or if in a torn
    * off menupane.
    */

   if (!is_menupane || torn_has_focus)
   {
      if ((pb->core.being_destroyed == False) && (!pb->pushbutton.timer))
        pb->pushbutton.timer = XtAppAddTimeOut(
                                     XtWidgetToApplicationContext((Widget)pb),
                                     (unsigned long) DELAY_DEFAULT,
                                     ArmTimeout,
                                     (XtPointer)(pb));
   }
}

/*ARGSUSED*/
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
   XmPushButtonWidget pb = (XmPushButtonWidget) data;

   pb -> pushbutton.timer = 0;
   if (XtIsRealized (pb) && XtIsManaged (pb))
   {
       if ((pb ->label.menu_type == XmMENU_PULLDOWN ||
	    pb ->label.menu_type == XmMENU_POPUP))
       {
	   /* When rapidly clicking, the focus may have moved away from this
	    * widget, so check before changing the shadow.
	    */
           if (_XmFocusIsInShell((Widget)pb) &&
	       (XmGetFocusWidget((Widget)pb) == (Widget)pb) )
	   {
	      /* in a torn off menu, redraw shadows */
	      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			    pb -> primitive.top_shadow_GC,
			    pb -> primitive.bottom_shadow_GC,
			    pb -> primitive.highlight_thickness,
			    pb -> primitive.highlight_thickness,
			    pb -> core.width - 2 *
			     pb->primitive.highlight_thickness,
			    pb -> core.height - 2 *
			     pb->primitive.highlight_thickness,
			    pb -> primitive.shadow_thickness,
			    XmSHADOW_OUT);
	   }
       }
       else
       {
	   (* XtClass(pb)->core_class.expose)((Widget) pb, NULL,
					      (Region) NULL);
       }

       XFlush (XtDisplay (pb));
   }
   return;
}



/************************************************************************
 *
 *    Disarm
 *
 *     Mark the pushbutton as unarmed (i.e. active).
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;

/* BEGIN OSF Fix pir 2826 */
   if (pb->pushbutton.armed == TRUE)
   {
     pb -> pushbutton.armed = FALSE;
     Redisplay((Widget) pb, event, (Region)NULL);
     if (XtClass(pb)->core_class.expose)
        (* XtClass(pb)->core_class.expose)((Widget)(pb), event, (Region)NULL);

   }
/* END OSF Fix pir 2826 */

   if (pb->pushbutton.disarm_callback)
   {
      call_value.reason = XmCR_DISARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.disarm_callback, &call_value);
   }
}


/************************************************************************
 *
 *     BtnDown
 *
 *     This function processes a button down occuring on the pushbutton
 *     when it is in a popup, pulldown, or option menu.
 *     Popdown the posted menu.
 *     Turn parent's traversal off.
 *     Mark the pushbutton as armed (i.e. active).
 *     The callbacks for XmNarmCallback are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
BtnDown( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BtnDown(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;
   Boolean validButton;
   Boolean already_armed;
   ShellWidget popup;

   /* Support menu replay, free server input queue until next button event */
   XAllowEvents(XtDisplay(pb), SyncPointer, CurrentTime);

   if (event && (event->type == ButtonPress))
       (* xmLabelClassRec.label_class.menuProcs) (XmMENU_BUTTON,
						  XtParent(pb), NULL, event,
						  &validButton);
   
   if (!validButton)
       return;

   _XmSetInDragMode((Widget)pb, True);

   /* Popdown other popus that may be up */
   if (!(popup = (ShellWidget)_XmGetRC_PopupPosted(XtParent(pb))))
   {
      if (!XmIsMenuShell(XtParent(XtParent(pb))))
      {
	 /* In case tear off not armed and no grabs in place, do it now.
	  * Ok if already armed and grabbed - nothing done.
	  */
	 (* xmLabelClassRec.label_class.menuProcs) 
	    (XmMENU_TEAR_OFF_ARM, XtParent(pb));
      }
   }

   if (popup)
   {
      Widget w;
      
      if (popup->shell.popped_up)
	  (* xmLabelClassRec.label_class.menuProcs)
	      (XmMENU_SHELL_POPDOWN, (Widget) popup, NULL, event, NULL);

      /* If active_child is a cascade (highlighted), then unhighlight it.  */
      w = ((XmManagerWidget)XtParent(pb))->manager.active_child;
      if (w && ((XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w))))
	  XmCascadeButtonHighlight (w, FALSE);
   }

   /* Set focus to this pushbutton.  This must follow the possible
    * unhighlighting of the CascadeButton else it'll screw up active_child.
    */
   (void)XmProcessTraversal( (Widget) pb, XmTRAVERSE_CURRENT);
   /* get the location cursor - get consistent with Gadgets */

   already_armed = pb -> pushbutton.armed;
   pb -> pushbutton.armed = TRUE;

   if (pb->pushbutton.arm_callback && !already_armed)
   {
      XFlush (XtDisplay (pb));

      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.arm_callback, &call_value);
   }
   _XmRecordEvent (event);
}




/************************************************************************
 *
 *     BtnUp
 *
 *     This function processes a button up occuring on the pushbutton
 *     when it is in a popup, pulldown, or option menu.
 *     Mark the pushbutton as unarmed (i.e. inactive).
 *     The callbacks for XmNactivateCallback are called.
 *     The callbacks for XmNdisarmCallback are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
BtnUp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BtnUp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   Widget parent =  XtParent(pb);
   XmPushButtonCallbackStruct call_value;
   Boolean flushDone = False;
   Boolean validButton;
   Boolean popped_up;
   Boolean is_menupane = (pb ->label.menu_type == XmMENU_PULLDOWN) ||
			 (pb ->label.menu_type == XmMENU_POPUP);
   Widget shell = XtParent(XtParent(pb));

   if (event && (event->type == ButtonRelease))
       (* xmLabelClassRec.label_class.menuProcs) (XmMENU_BUTTON,
						  parent, NULL, event,
						  &validButton);
   
   if (!validButton || (pb->pushbutton.armed == FALSE))
       return;

   pb -> pushbutton.armed = FALSE;

   if (is_menupane && !XmIsMenuShell(shell))
      (* xmLabelClassRec.label_class.menuProcs)
	 (XmMENU_POPDOWN, (Widget) pb, NULL, event, &popped_up);
   else
      (* xmLabelClassRec.label_class.menuProcs)
	 (XmMENU_BUTTON_POPDOWN, (Widget) pb, NULL, event, &popped_up);

   _XmRecordEvent(event);

   /* XmMENU_POPDOWN left the menu posted on button click - don't activate! */
   if (popped_up)
   {
     return;
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
#ifdef NON_OSF_FIX
   call_value.click_count = 1;
#endif /* NON_OSF_FIX */

   /* if the parent is a RowColumn, notify it about the select */
   if (XmIsRowColumn(parent))
   {
      (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						 parent, FALSE, pb,
						 &call_value);

      flushDone = True;
   }
   
   if ((! pb->label.skipCallback) &&
       (pb->pushbutton.activate_callback))
   {
      XFlush (XtDisplay (pb));
      flushDone = True;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.activate_callback,
			     &call_value);
   }
   if (pb->pushbutton.disarm_callback)
   {
      if (!flushDone)
	  XFlush (XtDisplay (pb));
      call_value.reason = XmCR_DISARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, pb->pushbutton.disarm_callback,
			  &call_value);
   }

   /* If the original shell does not indicate an active menu, but rather a
    * tear off pane, leave the button in an armed state.  Also, briefly
    * display the button as depressed to give the user some feedback of
    * the selection.
    */

   if (is_menupane) /* necessary check? */
   {
       if (!XmIsMenuShell(shell))
       {
	  if (XtIsSensitive(pb))
	  {
	     _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			      pb -> primitive.bottom_shadow_GC,
			      pb -> primitive.top_shadow_GC,
			      pb -> primitive.highlight_thickness,
			      pb -> primitive.highlight_thickness,
			      pb -> core.width - 2 *
			       pb->primitive.highlight_thickness,
			      pb -> core.height - 2 *
			       pb->primitive.highlight_thickness,
			      pb -> primitive.shadow_thickness,
			      XmSHADOW_OUT);
	     
	     XFlush (XtDisplay (pb));
	     flushDone = True;
	     
	     if (pb->core.being_destroyed == False)
	     {
		 if (!pb->pushbutton.timer)
		     pb->pushbutton.timer =
			 XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)pb),
					 (unsigned long) DELAY_DEFAULT,
					 ArmTimeout,
					 (XtPointer)(pb));
	     }

	     pb -> pushbutton.armed = TRUE;
	     if (pb->pushbutton.arm_callback)
	     {
		 if (!flushDone)
		     XFlush (XtDisplay (pb));
		 call_value.reason = XmCR_ARM;
		 call_value.event = event;
		 XtCallCallbackList ((Widget) pb, pb->pushbutton.arm_callback,
				     &call_value);
	     }
	  }
       }
       else
	  (* xmLabelClassRec.label_class.menuProcs)
	     (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	     XtParent(pb), NULL, event, NULL);
   }

   _XmSetInDragMode((Widget)pb, False);

   /* For the benefit of tear off menus, we must set the focus item
    * to this button.  In normal menus, this would not be a problem
    * because the focus is cleared when the menu is unposted.
    */
   if (!XmIsMenuShell(shell))
      XmProcessTraversal((Widget) pb, XmTRAVERSE_CURRENT);
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (pb ->label.menu_type == XmMENU_PULLDOWN ||
       pb ->label.menu_type == XmMENU_POPUP)
   {
      if ((((ShellWidget) XtParent(XtParent(pb)))->shell.popped_up) &&
          _XmGetInDragMode((Widget)pb))
      {
	 if (pb->pushbutton.armed)
	    return;

	  /* So KHelp event is delivered correctly */
         _XmSetFocusFlag( XtParent(XtParent(pb)), XmFOCUS_IGNORE, TRUE);
         XtSetKeyboardFocus(XtParent(XtParent(pb)), (Widget)pb);
         _XmSetFocusFlag( XtParent(XtParent(pb)), XmFOCUS_IGNORE, FALSE);

#ifdef CDE_VISUAL	/* etched in menu button */
         {
         Boolean etched_in = False;
         XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)),
		       "enableEtchedInMenu", &etched_in, NULL);

	 if (etched_in && !XtIsSubclass(wid, xmTearOffButtonWidgetClass) ) {
	     XFillRectangle (XtDisplay(pb), XtWindow(pb),
                            pb->pushbutton.fill_gc,
                   0, 0, pb->core.width, pb->core.height);
	     DrawArmedMenuLabel( pb, event, NULL );
	 }
	 _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			  pb -> primitive.top_shadow_GC,
			  pb -> primitive.bottom_shadow_GC,
			  pb -> primitive.highlight_thickness,
			  pb -> primitive.highlight_thickness,
			  pb -> core.width - 2 *
				pb->primitive.highlight_thickness,
			  pb -> core.height - 2 *
				pb->primitive.highlight_thickness,
			  pb -> primitive.shadow_thickness,
                          etched_in ? XmSHADOW_IN : XmSHADOW_OUT);
         }
#else
	  _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			  pb -> primitive.top_shadow_GC,
			  pb -> primitive.bottom_shadow_GC,
			  pb -> primitive.highlight_thickness,
			  pb -> primitive.highlight_thickness,
			  pb -> core.width - 2 *
				pb->primitive.highlight_thickness,
			  pb -> core.height - 2 *
				pb->primitive.highlight_thickness,
			  pb -> primitive.shadow_thickness,
                          XmSHADOW_OUT);
#endif
	  pb -> pushbutton.armed = TRUE;

	  if (pb->pushbutton.arm_callback)
	  {
	      XFlush (XtDisplay (pb));
		 
	      call_value.reason = XmCR_ARM;
	      call_value.event = event;
	      XtCallCallbackList ((Widget) pb,
				  pb->pushbutton.arm_callback, &call_value);
	  }
      }
   }  
   else 
   {
      _XmPrimitiveEnter( (Widget) pb, event, NULL, NULL);
      if (pb -> pushbutton.armed == TRUE)
	 (* XtClass(pb)->core_class.expose)(wid, event, (Region) NULL);
   }
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
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (pb ->label.menu_type == XmMENU_PULLDOWN ||
       pb ->label.menu_type == XmMENU_POPUP)
   {
      if (_XmGetInDragMode((Widget)pb) && pb->pushbutton.armed &&
          (/* !ActiveTearOff || */ event->xcrossing.mode == NotifyNormal))
      {
#ifdef CDE_VISUAL	/* etched in menu button */
         Boolean etched_in = False;
         XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)),
		       "enableEtchedInMenu", &etched_in, NULL);
	 if (etched_in && !XtIsSubclass(wid, xmTearOffButtonWidgetClass) ) {
	     XFillRectangle (XtDisplay(pb), XtWindow(pb),
			     pb->pushbutton.background_gc,
                  0, 0, pb->core.width, pb->core.height);
	     DrawPushButtonLabel (pb, event, NULL);
	 }
	 else
	     _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                       pb -> primitive.highlight_thickness,
                       pb -> primitive.highlight_thickness,
                       pb -> core.width - 2 *
                             pb->primitive.highlight_thickness,
                       pb -> core.height - 2 *
                             pb->primitive.highlight_thickness,
                       pb -> primitive.shadow_thickness);
#else
         _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                       pb -> primitive.highlight_thickness,
                       pb -> primitive.highlight_thickness,
                       pb -> core.width - 2 *
                             pb->primitive.highlight_thickness,
                       pb -> core.height - 2 *
                             pb->primitive.highlight_thickness,
                       pb -> primitive.shadow_thickness);
#endif

         pb -> pushbutton.armed = FALSE;

         if (pb->pushbutton.disarm_callback)
         {
	    XFlush (XtDisplay (pb));

	    call_value.reason = XmCR_DISARM;
	    call_value.event = event;
	    XtCallCallbackList ((Widget) pb, pb->pushbutton.disarm_callback, &call_value);
         }
      }
   }
   else 
   {
      _XmPrimitiveLeave( (Widget) pb, event, NULL, NULL);

      if (pb -> pushbutton.armed == TRUE)
      {
	 pb -> pushbutton.armed = FALSE;
	 (* XtClass(pb)->core_class.expose)(wid, event, (Region) NULL);
	 pb -> pushbutton.armed = TRUE;
      }
   }
}


/*************************************<->*************************************
 *
 *  BorderHighlight 
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
BorderHighlight( wid )
        Widget wid ;
#else
BorderHighlight(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;
   XEvent * event = NULL;

   if (pb ->label.menu_type == XmMENU_PULLDOWN ||
       pb ->label.menu_type == XmMENU_POPUP)
   {
#ifdef CDE_VISUAL	/* etched in menu button */
      Boolean etched_in = False;
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)), "enableEtchedInMenu", &etched_in, NULL);
      if (etched_in && !XtIsSubclass(wid, xmTearOffButtonWidgetClass) ) {
          XFillRectangle (XtDisplay(pb), XtWindow(pb),
			     pb->pushbutton.fill_gc,
                  0, 0, pb->core.width, pb->core.height);
	  DrawArmedMenuLabel( pb, event, NULL );
      }
      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		     pb -> primitive.top_shadow_GC,
		     pb -> primitive.bottom_shadow_GC,
		     pb -> primitive.highlight_thickness,
		     pb -> primitive.highlight_thickness,
		     pb -> core.width - 2 *
			   pb->primitive.highlight_thickness,
		     pb -> core.height - 2 *
			   pb->primitive.highlight_thickness,
		     pb -> primitive.shadow_thickness,
                     etched_in ? XmSHADOW_IN : XmSHADOW_OUT);
#else
      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		     pb -> primitive.top_shadow_GC,
		     pb -> primitive.bottom_shadow_GC,
		     pb -> primitive.highlight_thickness,
		     pb -> primitive.highlight_thickness,
		     pb -> core.width - 2 *
			   pb->primitive.highlight_thickness,
		     pb -> core.height - 2 *
			   pb->primitive.highlight_thickness,
		     pb -> primitive.shadow_thickness,
                     XmSHADOW_OUT);
#endif
      if (!pb->pushbutton.armed && pb->pushbutton.arm_callback)
      {
	 XFlush (XtDisplay (pb));

	 call_value.reason = XmCR_ARM;
	 call_value.event = event;
	 XtCallCallbackList ((Widget) pb, pb->pushbutton.arm_callback, &call_value);
      }
      pb -> pushbutton.armed = TRUE;
   }
   else
   {   DrawBorderHighlight( (Widget) pb) ;
       } 

   return ;
   }

static void
#ifdef _NO_PROTO
DrawBorderHighlight( wid )
        Widget wid ;
#else
DrawBorderHighlight(
        Widget wid)
#endif /* _NO_PROTO */
{   
            XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
            register Dimension highlight_width ;

    if(    !XtWidth( pb)  ||  !XtHeight( pb) )
    {   
        return ;
        } 
    pb->primitive.highlighted = True ;
    pb->primitive.highlight_drawn = True ;

    if(    pb->pushbutton.default_button_shadow_thickness    )
    {   
        highlight_width = pb->primitive.highlight_thickness
                                                         - Xm3D_ENHANCE_PIXEL ;
        } 
    else
    {   highlight_width = pb->primitive.highlight_thickness ;
        } 

    if (highlight_width > 0)
#ifdef CDE_VISUAL	/* default button */
    {
	int x=0, y=0, width=XtWidth( pb), height=XtHeight( pb);
	int default_button_shadow_thickness;
        Boolean default_button = False;

        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);

	if (default_button &&
	    pb->pushbutton.default_button_shadow_thickness) {
	    if (pb->pushbutton.compatible)
		default_button_shadow_thickness = 
		    (int) (pb->pushbutton.show_as_default);
	    else    
		default_button_shadow_thickness = 
		    (int) pb->pushbutton.default_button_shadow_thickness;
	    
	    x = y = (default_button_shadow_thickness * 2) + Xm3D_ENHANCE_PIXEL;
	    width -= 2 * x;
	    height -= 2 * y;
	}
	_XmDrawSimpleHighlight( XtDisplay( pb), XtWindow( pb), 
			       pb->primitive.highlight_GC, x, y, width,
			       height, highlight_width) ;
    }
#else
	_XmDrawSimpleHighlight( XtDisplay( pb), XtWindow( pb), 
			       pb->primitive.highlight_GC, 0, 0, XtWidth( pb),
			       XtHeight( pb), highlight_width) ;
#endif
    return ;
    } 

/*************************************<->*************************************
 *
 *  BorderUnhighlight
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
BorderUnhighlight( wid )
        Widget wid ;
#else
BorderUnhighlight(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;
   XEvent * event = NULL;

   if (pb ->label.menu_type == XmMENU_PULLDOWN ||
       pb ->label.menu_type == XmMENU_POPUP)
   {
#ifdef CDE_VISUAL	/* etched in menu button */
       Boolean etched_in = False;
       XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)),
		     "enableEtchedInMenu", &etched_in, NULL);
       if (etched_in && !XtIsSubclass(wid, xmTearOffButtonWidgetClass) ) {
	   XFillRectangle (XtDisplay(pb), XtWindow(pb),
			     pb->pushbutton.background_gc,
                  0, 0, pb->core.width, pb->core.height);
	   DrawPushButtonLabel (pb, event, NULL);
       }
       else
	   _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                    pb -> primitive.highlight_thickness,
                    pb -> primitive.highlight_thickness,
                    pb -> core.width - 2 *
                        pb->primitive.highlight_thickness,
                    pb -> core.height - 2 *
                        pb->primitive.highlight_thickness,
                    pb -> primitive.shadow_thickness);
#else
      _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                    pb -> primitive.highlight_thickness,
                    pb -> primitive.highlight_thickness,
                    pb -> core.width - 2 *
                        pb->primitive.highlight_thickness,
                    pb -> core.height - 2 *
                        pb->primitive.highlight_thickness,
                    pb -> primitive.shadow_thickness);
#endif

      if (pb->pushbutton.armed && pb->pushbutton.disarm_callback)
      {
	 XFlush (XtDisplay (pb));

	 call_value.reason = XmCR_DISARM;
	 call_value.event = event;
	 XtCallCallbackList ((Widget) pb, pb->pushbutton.disarm_callback, 
            &call_value);
      }
      pb -> pushbutton.armed = FALSE;
   }
   else 
   {   /* PushButton is not in a menu - parent may be a shell or manager
        */
#ifdef CDE_VISUAL	/* default button */
       int border = pb->primitive.highlight_thickness -Xm3D_ENHANCE_PIXEL;
        Boolean default_button = False;

        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);

       if (default_button &&
	   pb->pushbutton.default_button_shadow_thickness && (border > 0)) {
	   int x=0, y=0, width=XtWidth( pb), height=XtHeight( pb);
	   int default_button_shadow_thickness;

	   pb->primitive.highlighted = False ;
	   pb->primitive.highlight_drawn = False ;
	   if (pb->pushbutton.compatible)
	       default_button_shadow_thickness = 
		   (int) (pb->pushbutton.show_as_default);
	   else    
	       default_button_shadow_thickness = 
		   (int) pb->pushbutton.default_button_shadow_thickness;
	    
	   x = y = (default_button_shadow_thickness * 2) + Xm3D_ENHANCE_PIXEL;
	   width -= 2 * x;
	   height -= 2 * y;
	   _XmClearBorder (XtDisplay (pb), XtWindow (pb),
			   x, y, width, height, border);	    
	}
	else
	    (*(xmLabelClassRec.primitive_class.border_unhighlight))( wid) ;
#else
        (*(xmLabelClassRec.primitive_class.border_unhighlight))( wid) ;
#endif
        }
    return ;
    }

/*************************************<->*************************************
 *
 *  KeySelect
 *
 *  If the menu system traversal is enabled, do an activate and disarm
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
KeySelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KeySelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (!_XmIsEventUnique(event))
      return;

   if (!_XmGetInDragMode((Widget)pb))
   {

      pb -> pushbutton.armed = FALSE;

      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);
      }

      _XmRecordEvent(event);
      
      call_value.reason = XmCR_ACTIVATE;
      call_value.event = event;

      /* if the parent is a RowColumn, notify it about the select */
      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(pb), FALSE, pb,
						    &call_value);
      }

      if ((! pb->label.skipCallback) &&
	  (pb->pushbutton.activate_callback))
      {
	 XFlush (XtDisplay (pb));
	 XtCallCallbackList ((Widget) pb, pb->pushbutton.activate_callback,
				&call_value);
      }

      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(pb), NULL, event, NULL);
      }
   }
}


/*************************************<->*************************************
 *
 *  ClassInitialize 
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   /* parse the various translation tables */

   menu_parsed		= XtParseTranslationTable(menuTranslations);
   default_parsed	= XtParseTranslationTable(defaultTranslations);

   /* set up base class extension quark */
   pushBBaseClassExtRec.record_type = XmQmotif;

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
   _XmFastSubclassInit (wc, XmPUSH_BUTTON_BIT);
}

/************************************************************
 *
 * InitializePrehook
 *
 * Put the proper translations in core_class tm_table so that
 * the data is massaged correctly
 *
 ************************************************************/
static void
#ifdef _NO_PROTO
InitializePrehook( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializePrehook(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
  unsigned char type;

  _XmSaveCoreClassTranslations (new_w);

  if (XmIsRowColumn(XtParent(new_w)))
  {
    Arg arg[1];
    XtSetArg (arg[0], XmNrowColumnType, &type);
    XtGetValues (XtParent(new_w), arg, 1);
  }

  else 
    type = XmWORK_AREA;

  if (type == XmMENU_PULLDOWN ||
      type == XmMENU_POPUP)
    new_w->core.widget_class->core_class.tm_table = (String) menu_parsed;

  else 
    new_w->core.widget_class->core_class.tm_table = (String) default_parsed;
}

/************************************************************
 *
 * InitializePosthook
 *
 * restore core class translations
 *
 ************************************************************/
static void
#ifdef _NO_PROTO
InitializePosthook( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializePosthook(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
  _XmRestoreCoreClassTranslations (new_w);
}

/*************************************<->*************************************
 *
 *  Initialize 
 *
 *************************************<->***********************************/
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
   XmPushButtonWidget request = (XmPushButtonWidget) rw ;
   XmPushButtonWidget new_w = (XmPushButtonWidget) nw ;
   int increase;	
   int adjustment = 0;  

   if (new_w->pushbutton.multiClick == XmINVALID_MULTICLICK)
   {
      if (new_w->label.menu_type == XmMENU_POPUP ||
          new_w->label.menu_type == XmMENU_PULLDOWN)
         new_w->pushbutton.multiClick   = XmMULTICLICK_DISCARD;
      else
         new_w->pushbutton.multiClick = XmMULTICLICK_KEEP;
   }

   /* if menuProcs is not set up yet, try again */
   if (xmLabelClassRec.label_class.menuProcs == NULL)
       xmLabelClassRec.label_class.menuProcs =
                                          (XmMenuProc) _XmGetMenuProcContext();

/*
 * Fix to introduce Resource XmNdefaultBorderWidth and compatibility
 *  variable.
 *  if defaultBorderWidth > 0, the program knows about this resource
 *  and is therefore a Motif 1.1 program; otherwise it is a Motif 1.0
 *      program and old semantics of XmNshowAsDefault prevails.
 *  - Sankar 2/1/90.
 */
   if (new_w->pushbutton.default_button_shadow_thickness > 0)
		new_w->pushbutton.compatible = False;
      else 
                new_w->pushbutton.compatible = True;

/*
 * showAsDefault as boolean if compatibility is false (Motif 1.1) else
 *  treat it to indicate the thickness of defaultButtonShadow.
 */
   if (new_w->pushbutton.compatible)
      new_w->pushbutton.default_button_shadow_thickness 
			= new_w->pushbutton.show_as_default;

    new_w->pushbutton.armed = FALSE;
   new_w->pushbutton.timer = 0;

   /* no unarm_pixmap but do have an arm_pixmap, use that */
   if ((new_w->label.pixmap == XmUNSPECIFIED_PIXMAP) &&
       (new_w->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      new_w->label.pixmap = new_w->pushbutton.arm_pixmap;
      if (request->core.width == 0)
         new_w->core.width = 0;
      if (request->core.height == 0)
         new_w->core.height = 0;

      _XmCalcLabelDimensions(nw);
      (* xmLabelClassRec.core_class.resize) ((Widget) new_w);
   }

   if (new_w->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP)
   {
       if (request->core.width == 0)
         new_w->core.width = 0;
       if (request->core.height == 0)
         new_w->core.height = 0;
       SetPushButtonSize(new_w);
   }

   new_w->pushbutton.unarm_pixmap = new_w->label.pixmap;

   if (new_w->pushbutton.default_button_shadow_thickness)
   {
     /*
      * Special hack for 3d enhancement of location cursor highlight.
      *  - Make the box bigger. During drawing of location cursor
      *    make it smaller.  See in Primitive.c
      *  Maybe we should use the macro: G_HighLightThickness(pbgadget);
      */
     new_w->primitive.highlight_thickness += Xm3D_ENHANCE_PIXEL;
     adjustment = Xm3D_ENHANCE_PIXEL;
     increase =  2 * new_w->pushbutton.default_button_shadow_thickness +
       new_w->primitive.shadow_thickness;
     
     increase += adjustment ;
     
     /* Add the increase to the core to compensate for extra space */
     if (increase != 0)
       {
	 Lab_MarginLeft(new_w) += increase;
	 Lab_MarginRight(new_w) += increase;
	 Lab_TextRect_x(new_w) += increase ;
	 new_w->core.width += (increase << 1);
	 
	 Lab_MarginTop(new_w) += increase;
	 Lab_MarginBottom(new_w) += increase;
	 Lab_TextRect_y(new_w) += increase ;
	 new_w->core.height += (increase << 1);
       }
   }
	
   if (new_w->label.menu_type  == XmMENU_POPUP ||
       new_w->label.menu_type  == XmMENU_PULLDOWN)
   {
      new_w->primitive.traversal_on = TRUE;
#ifdef CDE_VISUAL	/* etched in menu button */
      GetFillGC (new_w);
      GetBackgroundGC (new_w);
#endif
   }  
   else
   {
	/* initialize GCs for fill and background only if the button is not
	** in a menu; note code elsewhere checks menuness before using these GCs
	*/
	GetFillGC (new_w);
	GetBackgroundGC (new_w);
   }
}




/************************************************************************
 *
 *  GetFillGC
 *     Get the graphics context used for filling in background of button.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetFillGC( pb )
        XmPushButtonWidget pb ;
#else
GetFillGC(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCFillStyle;

   values.foreground = pb -> pushbutton.arm_color;
   values.fill_style = FillSolid;

   pb -> pushbutton.fill_gc = XtGetGC ((Widget) pb, valueMask, &values);
}



/************************************************************************
 *
 *  GetBackgroundGC
 *     Get the graphics context used for filling in background of 
 *     the pushbutton when not armed.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetBackgroundGC( pb )
        XmPushButtonWidget pb ;
#else
GetBackgroundGC(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{
        XGCValues       values;
        XtGCMask        valueMask;
        short             myindex;
        XFontStruct     *fs;

        valueMask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
			  
        _XmFontListSearch(pb->label.font,
                          XmFONTLIST_DEFAULT_TAG,
                          &myindex,
                          &fs);
        values.foreground = pb->core.background_pixel;
        values.background = pb->primitive.foreground;
	values.graphics_exposures = False;

        if (fs==NULL)
          valueMask &= ~GCFont;
        else
          values.font     = fs->fid;

	/* add background_pixmap to GC */
	if (pb->core.background_pixmap != XmUNSPECIFIED_PIXMAP)
	{
	    values.tile = pb->core.background_pixmap;
	    values.fill_style = FillTiled;
	    valueMask |= (GCTile | GCFillStyle);
	}

        pb->pushbutton.background_gc = XtGetGC( (Widget) pb,valueMask,&values);
}


/*************************************<->*************************************
 *
 *  SetValues(current, request, new_w)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the pushbutton class.  It is
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
   XmPushButtonWidget current = (XmPushButtonWidget) cw ;
   XmPushButtonWidget request = (XmPushButtonWidget) rw ;
   XmPushButtonWidget new_w = (XmPushButtonWidget) nw ;
   int increase;
   Boolean  flag = FALSE;    /* our return value */
   int adjustment;

/*
 * Fix to introduce Resource XmNdefaultBorderWidth and compatibility
 *      variable.
 *  if the XmNdefaultBorderWidth resource in the current differ from the
 *  one in "new_w", then the programmer is setting this resource - so this
 *  is known to the programmer and hence it is a Motif1.1 program.
 *  If they are same then either it is a Motif 1.0 program or there has been
 *  no change in the resource (Motif 1.1 program). If it is a Motif 1.0 
 *  program, then we should copy the value of XmNshowAsDefault to the 
 *  XmNdefaultBorderWidth. If it is  Motif 1.1 program ( Compatible
 *  flag is flase) - then we should not do the copy.
 *  This logic will maintain the semantics of the  XmNshowAsDefault of Motif
 *  1.0. For a full explanation see the Design architecture document.
 *  - Sankar 2/2/90.
 */

   if ( (current->pushbutton.default_button_shadow_thickness) !=
	  (new_w->pushbutton.default_button_shadow_thickness) )
		new_w->pushbutton.compatible = False;

   if (new_w->pushbutton.compatible)
      new_w->pushbutton.default_button_shadow_thickness 
		= new_w->pushbutton.show_as_default;

   adjustment = AdjustHighLightThickness (new_w, current);

	/*
	 * Compute size change.
	 */
   if (new_w->pushbutton.default_button_shadow_thickness != 
		current->pushbutton.default_button_shadow_thickness)
   {


      if (new_w->pushbutton.default_button_shadow_thickness > 
		current->pushbutton.default_button_shadow_thickness)
      {
         if (current->pushbutton.default_button_shadow_thickness > 0)
            increase =  (2 * new_w->pushbutton.default_button_shadow_thickness +
                         new_w->primitive.shadow_thickness) -
                      (2 * current->pushbutton.default_button_shadow_thickness +
                         current->primitive.shadow_thickness);
         else
            increase =  (2 * new_w->pushbutton.default_button_shadow_thickness +
                         new_w->primitive.shadow_thickness);
      }
      else
      {
         if (new_w->pushbutton.default_button_shadow_thickness > 0)
        increase = - ((2 * current->pushbutton.default_button_shadow_thickness +
                           current->primitive.shadow_thickness) -
                          (2 * new_w->pushbutton.default_button_shadow_thickness +
                           new_w->primitive.shadow_thickness));
         else
        increase = - (2 * current->pushbutton.default_button_shadow_thickness +
                          current->primitive.shadow_thickness);
      }
      
      increase += adjustment ;

      if (new_w->label.recompute_size || request->core.width == 0)
      {
         Lab_MarginLeft(new_w) += increase;
         Lab_MarginRight(new_w) += increase;
         new_w->core.width +=  (increase << 1);
         flag = TRUE;
      }
	  else
		/* add the change due to default button to the core */
	   if ( increase != 0)
	   { 
	      Lab_MarginLeft(new_w) += increase;
	      Lab_MarginRight(new_w) += increase;
              new_w->core.width += (increase << 1);
	      flag = TRUE;
	   }

      if (new_w->label.recompute_size || request->core.height == 0)
      {
         Lab_MarginTop(new_w) += increase;
         Lab_MarginBottom(new_w) += increase;
         new_w->core.height += (increase << 1);
         flag = TRUE;
      }
      else
      /* add the change due to default button to the core */
       if ( increase != 0)
       { 
	  Lab_MarginTop(new_w) += increase;
	  Lab_MarginBottom(new_w) += increase;
          new_w->core.height += (increase << 1);
          flag = TRUE;
       }

   }

   if ((new_w->pushbutton.arm_pixmap != current->pushbutton.arm_pixmap) &&
      (new_w->label.label_type == XmPIXMAP) && (new_w->pushbutton.armed)) 
      flag = TRUE;
      
   /* no unarm_pixmap but do have an arm_pixmap, use that */
   if ((new_w->label.pixmap == XmUNSPECIFIED_PIXMAP) &&
       (new_w->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      new_w->label.pixmap = new_w->pushbutton.arm_pixmap;
      if (new_w->label.recompute_size &&
          request->core.width == current->core.width)
         new_w->core.width = 0;
      if (new_w->label.recompute_size &&
          request->core.height == current->core.height)
         new_w->core.width = 0;

      _XmCalcLabelDimensions(nw);
      (* xmLabelClassRec.core_class.resize) ((Widget) new_w);
   }

   if (new_w->label.pixmap != current->label.pixmap)
   {
      new_w->pushbutton.unarm_pixmap = new_w->label.pixmap;
      if ((new_w->label.label_type == XmPIXMAP) && (!new_w->pushbutton.armed))
	 flag = TRUE;
   }

   if ((new_w->label.label_type == XmPIXMAP) &&
       (new_w->pushbutton.arm_pixmap != current->pushbutton.arm_pixmap))
   {
       if ((new_w->label.recompute_size))
       {
          if (request->core.width == current->core.width)
             new_w->core.width = 0;
          if (request->core.height == current->core.height)
             new_w->core.height = 0;
       }
       SetPushButtonSize(new_w);
       flag = TRUE;
   }

   if ((new_w->pushbutton.fill_on_arm != current->pushbutton.fill_on_arm) &&
       (new_w->pushbutton.armed == TRUE))
	 flag = TRUE;

#ifndef CDE_VISUAL	/* etched in menu button */
   if (new_w ->label.menu_type != XmMENU_PULLDOWN &&
       new_w ->label.menu_type != XmMENU_POPUP)
#endif
   {
      /*  See if the GC need to be regenerated and widget redrawn.  */

      if (new_w -> pushbutton.arm_color != current -> pushbutton.arm_color)
      {
	 if (new_w->pushbutton.armed) flag = TRUE;  /* see PIR 5091 */
	 XtReleaseGC( (Widget) new_w, new_w->pushbutton.fill_gc);
	 GetFillGC( new_w);
      }

      if (new_w -> core.background_pixel != current -> core.background_pixel ||
	 ( new_w->core.background_pixmap != XmUNSPECIFIED_PIXMAP &&
	 new_w->core.background_pixmap != current->core.background_pixmap))
      {
	flag = TRUE;  /* label will cause redisplay anyway */
	XtReleaseGC( (Widget) new_w, new_w->pushbutton.background_gc);
	GetBackgroundGC( new_w);
      }
   }

/* BEGIN OSF Fix pir 3469 */
   if ( flag == False && XtIsRealized((Widget)new_w))
/* END OSF Fix pir 3469 */
    /* size s unchanged - optimize the shadow drawing  */
   {
      if ( (current->pushbutton.show_as_default != 0) &&
           (new_w->pushbutton.show_as_default == 0))
        	EraseDefaultButtonShadow (new_w);

      if ( (current->pushbutton.show_as_default == 0) &&
           (new_w->pushbutton.show_as_default != 0))
    	        DrawDefaultButtonShadows (new_w);
   }

   
   return(flag);
}

/**************************************************************************
 *
 * Resize(w)
 *
 **************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( w )
        Widget w ;
#else
Resize(
        Widget w )
#endif /* _NO_PROTO */
{
  register XmPushButtonWidget tb = (XmPushButtonWidget) w;

  if (Lab_IsPixmap(w)) 
    SetPushButtonSize(tb);
  else
    (* xmLabelClassRec.core_class.resize)( (Widget) tb);

}

/************************************************************************
 *
 *  Help
 *     This function processes Function Key 1 press occuring on the PushButton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Help(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmPushButtonWidget pb = (XmPushButtonWidget) wid ;
   Boolean is_menupane = (pb ->label.menu_type == XmMENU_PULLDOWN) ||
			 (pb ->label.menu_type == XmMENU_POPUP);

   if (is_menupane)
   {
      (* xmLabelClassRec.label_class.menuProcs)
	  (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);
   }

   _XmPrimitiveHelp( (Widget) pb, event, NULL, NULL);

/***
 * call_value.reason = XmCR_HELP;
 * call_value.event = event;
 * XtCallCallbackList ((Widget) pb, pb->primitive.help_callback, &call_value);
 ***/

   if (is_menupane)
   {
      (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(pb), NULL, event, NULL);
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
   XmPushButtonWidget pb = (XmPushButtonWidget) w ;
   if (pb->pushbutton.timer)
       XtRemoveTimeOut (pb->pushbutton.timer);
  
#ifndef CDE_VISUAL	/* etched in menu button */
/* BEGIN OSF Fix pir 2746 */
   if (pb->label.menu_type != XmMENU_PULLDOWN &&
       pb->label.menu_type != XmMENU_POPUP)
/* END OSF Fix pir 2746 */
#endif
   {
     XtReleaseGC ((Widget) pb, pb -> pushbutton.fill_gc);
     XtReleaseGC ((Widget) pb, pb -> pushbutton.background_gc);
   }
   XtRemoveAllCallbacks ((Widget) pb, XmNactivateCallback);
   XtRemoveAllCallbacks ((Widget) pb, XmNarmCallback);
   XtRemoveAllCallbacks ((Widget) pb, XmNdisarmCallback);
}


/************************************************************************
 *
 *		Application Accessible External Functions
 *
 ************************************************************************/


/************************************************************************
 *
 *  XmCreatePushButton
 *	Create an instance of a pushbutton and return the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreatePushButton( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreatePushButton(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmPushButtonWidgetClass, 
                           parent, arglist, argcount));
}



/*
 * EraseDefaultButtonShadow (pb)
 *  - Called from SetValues() - effort to optimize shadow drawing.
 */
static void 
#ifdef _NO_PROTO
EraseDefaultButtonShadow( pb )
        XmPushButtonWidget pb ;
#else
EraseDefaultButtonShadow(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{  int done = False;
   int size, dx, dy, width, height;

   if (!(XtIsRealized(pb)) ) done = True;   

   if (!(done))
   {
      if ((pb ->label.menu_type == XmMENU_PULLDOWN) ||
		       (pb ->label.menu_type == XmMENU_POPUP))
      { 
	 ShellWidget mshell = (ShellWidget)XtParent(XtParent(pb));
	 if (!mshell->shell.popped_up) done = True;
      }
   }

   if (!(done))
   { 
      size = pb -> pushbutton.default_button_shadow_thickness;
	    if (size > 0)
	    { size += Xm3D_ENHANCE_PIXEL;
	      dx = pb -> primitive.highlight_thickness;
	      dy = dx;
#ifdef CDE_VISUAL	/* default button */
              {
              Boolean default_button = False;

              XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget) pb)),
                  "defaultButtonEmphasis", &default_button, NULL);
	      if (default_button)
		  dx = dy = Xm3D_ENHANCE_PIXEL;
              }
#endif
	      width = pb -> core.width - ( dx << 1);
	      height = pb -> core.height - (dy << 1);
	      FillBorderWithParentColor( pb, size, dx, dy, width, height);
	    }
   }
}


/*************************************<->*************************************
 *
 *  Redisplay (pb, event, region)
 *   Completely rewritten to accommodate defaultButtonShadowThickness
 *   Description:
 *   -----------
 *     Cause the widget, identified by pb, to be redisplayed.
 *     If XmNfillOnArm is True and the pushbutton is not in a menu,
 *     the background will be filled with XmNarmColor.
 *     If XmNinvertOnArm is True and XmNLabelType is XmPIXMAP,
 *     XmNarmPixmap will be used in the label.
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
   XmPushButtonWidget pb = (XmPushButtonWidget) wid ;

   if (XtIsRealized(pb))
   { 
      if ( (pb ->label.menu_type == XmMENU_PULLDOWN) ||
	   (pb ->label.menu_type == XmMENU_POPUP) )
	 DrawPushButtonLabel (pb, event, region);	
      else
      { 
         DrawPushButtonBackground (pb);
	 DrawPushButtonLabel (pb, event, region);
	 DrawPushButtonShadows (pb);
		
	 if (pb -> primitive.highlighted)
	 {   
	    (*(((XmPushButtonWidgetClass) XtClass( pb))
	       ->primitive_class.border_highlight))( (Widget) pb) ;
	 } 
	 else
	 {   
	    if (_XmDifferentBackground( (Widget) pb, XtParent (pb)))
	    {   
	       (*(((XmPushButtonWidgetClass) XtClass( pb))
		  ->primitive_class.border_unhighlight))( (Widget) pb) ;
	    } 
         } 
      }
   }
}

/*
 * DrawPushButtonBackground ()
 *  - Compute the area allocated to the pushbutton and fill it with
 *    parent's background ;
 */
static void 
#ifdef _NO_PROTO
DrawPushButtonBackground( pb )
        XmPushButtonWidget pb ;
#else
DrawPushButtonBackground(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{    struct PBbox box;
     GC  tmp_gc;
     Bool result;

     result = ComputePBLabelArea (pb, &box);
     if (result)  /* computation is successful */
     { 
 	if ( (pb -> pushbutton.armed) && (pb->pushbutton.fill_on_arm))
	       tmp_gc = pb->pushbutton.fill_gc;
        else tmp_gc = pb->pushbutton.background_gc;

	if (tmp_gc)
          XFillRectangle (XtDisplay(pb), XtWindow(pb), tmp_gc,
                  box.pbx, box.pby, box.pbWidth, box.pbHeight);

     }
}
/*
 * DrawPushButtonLabel (pb, event, region)
 * Draw the label contained in the pushbutton.
 */
static void 
#ifdef _NO_PROTO
DrawPushButtonLabel( pb, event, region )
        XmPushButtonWidget pb ;
        XEvent *event ;
        Region region ;
#else
DrawPushButtonLabel(
        XmPushButtonWidget pb,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{  
   GC tmp_gc = NULL;
   Boolean replaceGC = False;
   Boolean deadjusted = False;

    if (pb ->label.menu_type != XmMENU_PULLDOWN &&
         pb ->label.menu_type != XmMENU_POPUP &&
          pb->pushbutton.fill_on_arm)
   {
    if ((pb->label.label_type == XmSTRING) && (pb->pushbutton.armed) &&
         (pb->pushbutton.arm_color == pb->primitive.foreground))
    {
        tmp_gc = pb->label.normal_GC;
        pb->label.normal_GC = pb->pushbutton.background_gc;
        replaceGC = True;
    }
  }

     if (pb->label.label_type == XmPIXMAP)
      {
     if (pb->pushbutton.armed)
        if (pb->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP)
           pb->label.pixmap = pb->pushbutton.arm_pixmap;
        else
           pb->label.pixmap = pb->pushbutton.unarm_pixmap;

     else   /* pushbutton is unarmed */
        pb->label.pixmap = pb->pushbutton.unarm_pixmap;
      }

     /*
      * Temporarily remove the Xm3D_ENHANCE_PIXEL hack ("adjustment")
      *           from the margin values, so we don't confuse Label.
      */
     if( pb->pushbutton.default_button_shadow_thickness > 0 )
       { 
         deadjusted = True;
         Lab_MarginLeft(pb) -= Xm3D_ENHANCE_PIXEL;
         Lab_MarginRight(pb) -= Xm3D_ENHANCE_PIXEL;
         Lab_MarginTop(pb) -= Xm3D_ENHANCE_PIXEL;
         Lab_MarginBottom(pb) -= Xm3D_ENHANCE_PIXEL;
       }
   
     (* xmLabelClassRec.core_class.expose) ((Widget) pb, event, region) ;
   
     if (deadjusted)
       {
         Lab_MarginLeft(pb) += Xm3D_ENHANCE_PIXEL;
         Lab_MarginRight(pb) += Xm3D_ENHANCE_PIXEL;
         Lab_MarginTop(pb) += Xm3D_ENHANCE_PIXEL;
         Lab_MarginBottom(pb) += Xm3D_ENHANCE_PIXEL;
       }
   
      if (replaceGC)
         pb->label.normal_GC = tmp_gc;

}

#ifdef CDE_VISUAL	/* etched in menu button */
static void 
#ifdef _NO_PROTO
DrawArmedMenuLabel( pb, event, region )
        XmPushButtonWidget pb ;
        XEvent *event ;
        Region region ;
#else
DrawArmedMenuLabel(
        XmPushButtonWidget pb,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{  
    GC tmp_gc = NULL;
    Boolean replaceGC = False;
    Boolean deadjusted = False;

    if (pb->label.label_type == XmSTRING &&
         (pb->pushbutton.arm_color == pb->primitive.foreground))    {
        tmp_gc = pb->label.normal_GC;
        pb->label.normal_GC = pb->pushbutton.background_gc;
        replaceGC = True;
    }

    if (pb->label.label_type == XmPIXMAP)    {
	if (pb->pushbutton.armed)
	    if (pb->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP)
		pb->label.pixmap = pb->pushbutton.arm_pixmap;
	    else
		pb->label.pixmap = pb->pushbutton.unarm_pixmap;

	else   /* pushbutton is unarmed */
	    pb->label.pixmap = pb->pushbutton.unarm_pixmap;
    }

    /*
     * Temporarily remove the Xm3D_ENHANCE_PIXEL hack ("adjustment")
     *           from the margin values, so we don't confuse Label.
     */
    if( pb->pushbutton.default_button_shadow_thickness > 0 )    { 
	deadjusted = True;
	Lab_MarginLeft(pb) -= Xm3D_ENHANCE_PIXEL;
	Lab_MarginRight(pb) -= Xm3D_ENHANCE_PIXEL;
	Lab_MarginTop(pb) -= Xm3D_ENHANCE_PIXEL;
	Lab_MarginBottom(pb) -= Xm3D_ENHANCE_PIXEL;
    }
   
    (* xmLabelClassRec.core_class.expose) ((Widget) pb, event, region) ;
   
    if (deadjusted)    {
	Lab_MarginLeft(pb) += Xm3D_ENHANCE_PIXEL;
	Lab_MarginRight(pb) += Xm3D_ENHANCE_PIXEL;
	Lab_MarginTop(pb) += Xm3D_ENHANCE_PIXEL;
	Lab_MarginBottom(pb) += Xm3D_ENHANCE_PIXEL;
    }
   
    if (replaceGC)
	pb->label.normal_GC = tmp_gc;
}
#endif

/*
 * DrawPushButtonShadows()
 *  Note: PushButton has two types of shadows: primitive-shadow and
 *	default-button-shadow.
 *  Following shadows  are drawn:
 *  if pushbutton is in a menu only primitive shadows are drawn;
 *   else
 *    { draw default shadow if needed;
 *	draw primitive shadow ;
 *    }
 */
static void 
#ifdef _NO_PROTO
DrawPushButtonShadows( pb )
        XmPushButtonWidget pb ;
#else
DrawPushButtonShadows(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{
   struct PBbox box;
   Boolean result;

    /*
     * PushButton background is different from the parent, then clear
     * the area (not occupied by label) with parents background color.
     */

	if (_XmDifferentBackground( (Widget) pb, XtParent (pb)))
	{
	   result = ComputePBLabelArea (pb, &box);
	   if (result)
	      FillBorderWithParentColor (pb, box.pbx, 0, 0,
		            pb->core.width, pb->core.height);
	}	 
  
	if (pb->pushbutton.default_button_shadow_thickness)
	{ 
	   if (pb->pushbutton.show_as_default)
	      DrawDefaultButtonShadows (pb);
	}
	  
        if (pb->primitive.shadow_thickness > 0)
 	   DrawPBPrimitiveShadows (pb);
    
}


/*
 *  ComputePBLabelArea ()
 *  - compute the area allocated to the label of pushbutton; 
 *    fill in the dimensions
 *    in the box; resturn indicates whether the values in box are valid or
 *    not;
 */
static Boolean 
#ifdef _NO_PROTO
ComputePBLabelArea( pb, box )
        XmPushButtonWidget pb ;
        struct PBbox *box ;
#else
ComputePBLabelArea(
        XmPushButtonWidget pb,
        struct PBbox *box )
#endif /* _NO_PROTO */
{   Boolean result = True;
    int dx, adjust;
    short fill = 0;

    if ((pb->pushbutton.arm_color == pb->primitive.top_shadow_color) ||
        (pb->pushbutton.arm_color == pb->primitive.bottom_shadow_color))
            fill = 1;

    if (pb == NULL) result = False;
    else
    { 
      if (pb->pushbutton.compatible)
	  adjust = pb -> pushbutton.show_as_default; 
      else
          adjust = pb -> pushbutton.default_button_shadow_thickness; 
      if (adjust > 0)
      { 
          adjust = adjust + pb-> primitive.shadow_thickness;
          adjust = (adjust << 1);
          dx = pb->primitive.highlight_thickness + adjust + fill;
      }
      else
          dx = pb->primitive.highlight_thickness +
               pb-> primitive.shadow_thickness + fill;


       box->pbx = dx;
       box->pby = dx;
       adjust = (dx << 1);
       box->pbWidth = pb->core.width - adjust;
       box->pbHeight= pb->core.height - adjust;
    }
    return (result);
}
	
static GC 
#ifdef _NO_PROTO
GetParentBackgroundGC( pb )
        XmPushButtonWidget pb ;
#else
GetParentBackgroundGC(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{	GC pgc;
	Widget  parent;
        XGCValues       values;
        XtGCMask        valueMask;

	parent = XtParent(pb);
	if (parent == NULL) pgc = NULL;

        else
 	 { valueMask =  GCForeground /* | GCBackground */ | GCGraphicsExposures;
	   values.foreground   = parent->core.background_pixel;
           values.graphics_exposures = False;
	   pgc = XtGetGC( parent, valueMask,&values);
	 }
	return (pgc);
}
/*
 * DrawPBPrimitiveShadow (pb)
 *   - Should be called only if PrimitiveShadowThickness > 0 
 */
static void 
#ifdef _NO_PROTO
DrawPBPrimitiveShadows( pb )
        XmPushButtonWidget pb ;
#else
DrawPBPrimitiveShadows(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{
      GC topgc, bottomgc;
      int dx, dy, width, height, adjust, shadow_thickness;
      if (pb->pushbutton.armed)
	  { bottomgc  = pb -> primitive.top_shadow_GC;
	    topgc  = pb -> primitive.bottom_shadow_GC;
	  }
       else 
	  { bottomgc  = pb -> primitive.bottom_shadow_GC;
	    topgc  = pb -> primitive.top_shadow_GC;
	  }

     
       shadow_thickness = pb->primitive.shadow_thickness;
       /*
	* This might have to be modified.
	*  - this is where dependency on compatibility with 1.0
        *    and defaultButtonShadowThickness etc. will showup.
        *  NOTE: defaultButtonShadowThickness is not supported in 
	    *   RowColumn children.
		*  1. Compute (x,y,width,height) for the rectangle within which
	    *	  the shadow is to be drawn.
        */

      if ( (shadow_thickness > 0) && (topgc) && (bottomgc))
       { 
	     if (pb->pushbutton.compatible)
				adjust = pb -> pushbutton.show_as_default;
		    else
				adjust = pb -> pushbutton.default_button_shadow_thickness;
	     if (adjust > 0)
		  { adjust = (adjust << 1);
		    dx = pb->primitive.highlight_thickness + 
				 adjust + pb->primitive.shadow_thickness ;
		  }
	     else
			dx = pb->primitive.highlight_thickness ;

         dy = dx;
         width = pb->core.width - 2 *dx;
         height = pb->core.height - 2 *dx;

         _XmDrawShadows (XtDisplay (pb), XtWindow (pb), topgc,
        bottomgc, dx, dy, width, height, shadow_thickness, XmSHADOW_OUT);

	}
}
/*
 * DrawDefaultButtonShadows()
 *  - get the topShadowColor and bottomShadowColor from the parent;
 *    use those colors to construct top and bottom gc; use these
 *	  GCs to draw the shadows of the button.
 *  - Should not be called if pushbutton is in a row column or in a menu.
 *  - Should be called only if a defaultbuttonshadow is to be drawn.
 */      
static void 
#ifdef _NO_PROTO
DrawDefaultButtonShadows( pb )
        XmPushButtonWidget pb ;
#else
DrawDefaultButtonShadows(
        XmPushButtonWidget pb )
#endif /* _NO_PROTO */
{
      GC topgc, bottomgc;
      int dx, dy, width, height, default_button_shadow_thickness;

      Widget  parent;
      XGCValues       values;
      XtGCMask        valueMask;
      Bool  borrowedGC = False;

      if ( ((pb->pushbutton.compatible) &&
			(pb->pushbutton.show_as_default == 0) ) ||
		 ( (!(pb->pushbutton.compatible)) &&
		    (pb->pushbutton.default_button_shadow_thickness == 0)))
		 return;
	  /*
	   * May need more complex computation for getting the GCs.
	   */

        parent = XtParent(pb);
       if (XmIsManager(parent))
       {  valueMask =  GCForeground  ;
		  values.foreground =   ((XmManagerWidget)(parent))-> 
                                manager.top_shadow_color;

          topgc = XtGetGC( (Widget) pb, valueMask,&values);

	      values.foreground =  ((XmManagerWidget)(parent))-> 
                                 manager.bottom_shadow_color;
 
		  bottomgc = XtGetGC( (Widget) pb, valueMask,&values);
		  borrowedGC = True;
	   }
	  else
	   { /* Use your own pixel for drawing */
	    bottomgc  = pb -> primitive.bottom_shadow_GC;
        topgc  = pb -> primitive.top_shadow_GC;
	   }
        if ( (bottomgc == NULL) || (topgc == NULL) ) return;


	if (pb->pushbutton.compatible)
	  default_button_shadow_thickness = 
			(int) (pb->pushbutton.show_as_default);
       else    
          default_button_shadow_thickness = 
			(int) pb->pushbutton.default_button_shadow_thickness;
    /*
     *
     * Compute location of bounding box to contain the defaultButtonShadow.
     */

        { dx = pb->primitive.highlight_thickness;

	 	 dy = dx;
         width = pb->core.width - 2 *dx;
 	 height = pb->core.height - 2 *dx;

#ifdef CDE_VISUAL	/* default button */
        {
        Boolean default_button = False;

        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);
	  if (default_button) {
	      dx = dy = Xm3D_ENHANCE_PIXEL;
	      width = pb->core.width - 2 * dx;
	      height = pb->core.height - 2 * dy;
	  }
	  _XmDrawShadows (XtDisplay (pb), XtWindow (pb), bottomgc,
                 default_button ? bottomgc : topgc,
		dx, dy, width, height,
		default_button_shadow_thickness, XmSHADOW_OUT); 
         }
#else
         _XmDrawShadows (XtDisplay (pb), XtWindow (pb), bottomgc, topgc, 
		dx, dy, width, height,
		default_button_shadow_thickness, XmSHADOW_OUT); 
#endif
	}

	if(borrowedGC)
	  { if (topgc) XtReleaseGC ((Widget) pb, topgc);
		if (bottomgc) XtReleaseGC ((Widget) pb, bottomgc);
	  }

}

static XmImportOperator 
#ifdef _NO_PROTO
ShowAsDef_ToHorizPix( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
ShowAsDef_ToHorizPix(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{           XtArgVal        oldValue ;
            XmImportOperator returnVal ;

    oldValue = *value ;
    returnVal = _XmToHorizontalPixels( widget, offset, value) ;

    if(    oldValue  &&  !*value    )
    {   *value = (XtArgVal) 1 ;
        } 
    return( returnVal) ;
    } 

void 
#ifdef _NO_PROTO
_XmClearBCompatibility( pb )
        Widget pb ;
#else
_XmClearBCompatibility(
        Widget pb )
#endif /* _NO_PROTO */
{

	((XmPushButtonWidget) pb)->pushbutton.compatible =  False;
}

/*
 * AdjustHighLightThickness ()
 *  HighlightThickness has a dependency on default_button-shadow-thickness;
 *  This routine (called from SetValues) adjust for that dependency.
 *  Applications should be aware that
 *  if a pushbutton gadget has  with (default_button-shadow-thickness == 0)
 * - then if through a XtSetValue it sets (default_button-shadow-thickness > 0)
 *  the application-specified highlight-thickness is internally increased by
 *  Xm3D_ENHANCE_PIXEL to enhance the 3D-appearance of the defaultButton
 *  Shadow. Similarly if a pushbutton gadget has ( default_button-shadow_
 *  thickness > 0), and it resets the (default_button-shadow-thickness = 0)
 *  through a XtSetValue , then the existing highlight-thickness is decreased
 *  by Xm3D_ENHANCE_PIXEL.
 *  The border-highlight when drawn is however is always of the same
 *  thickness as specified by the application since compensation is done
 *  in the drawing routine (see BorderHighlight).
 */
static int 
#ifdef _NO_PROTO
AdjustHighLightThickness( new_w, current )
        XmPushButtonWidget new_w ;
        XmPushButtonWidget current ;
#else
AdjustHighLightThickness(
        XmPushButtonWidget new_w,
        XmPushButtonWidget current )
#endif /* _NO_PROTO */
{  int adjustment = 0; 


	if (new_w->pushbutton.default_button_shadow_thickness > 0)
	{ if (current->pushbutton.default_button_shadow_thickness == 0)
	    { new_w->primitive.highlight_thickness += Xm3D_ENHANCE_PIXEL;
		  adjustment = Xm3D_ENHANCE_PIXEL;
		}
	  else
	   if (new_w->primitive.highlight_thickness !=
			current->primitive.highlight_thickness)
	     { new_w->primitive.highlight_thickness += Xm3D_ENHANCE_PIXEL;
		   adjustment = Xm3D_ENHANCE_PIXEL;
		 }
	}
    else
     { if (current->pushbutton.default_button_shadow_thickness > 0)
        /* default_button_shadow_thickness was > 0 and is now
         * being set to 0.
         * - so take away the adjustment for enhancement.
         */
	    {if (new_w->primitive.highlight_thickness ==
				current->primitive.highlight_thickness)
	       {  new_w->primitive.highlight_thickness -= Xm3D_ENHANCE_PIXEL;
			  adjustment -= Xm3D_ENHANCE_PIXEL;
		   }
        /*
         * This will appear to be a bug if in a XtSetValues the application
         * removes the default_button_shadow_thickness and also
         * sets the high-light-thickness to a value of
         * (old-high-light-thickness (from previous XtSetValue) +
         *  Xm3D_ENHANCE_PIXEL) at the same time.
         * This will be documented.
         */
       }
     }
	return (adjustment);
}

static void 
#ifdef _NO_PROTO
ExportHighlightThickness( widget, offset, value )
        Widget widget ;
        int offset ;
        XtArgVal *value ;
#else
ExportHighlightThickness(
        Widget widget,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
   XmPushButtonWidget pbw = (XmPushButtonWidget) widget;
   if (pbw->pushbutton.show_as_default ||
       pbw->pushbutton.default_button_shadow_thickness)
   {
          if ((int)*value >= Xm3D_ENHANCE_PIXEL)
              *value -= Xm3D_ENHANCE_PIXEL;
   }

   _XmFromHorizontalPixels (widget, offset, value);
}

static void 
#ifdef _NO_PROTO
FillBorderWithParentColor( pb, borderwidth, dx, dy, rectwidth, rectheight )
        XmPushButtonWidget pb ;
        int borderwidth ;
        int dx ;
        int dy ;
        int rectwidth ;
        int rectheight ;
#else
FillBorderWithParentColor(
        XmPushButtonWidget pb,
        int borderwidth,
        int dx,
        int dy,
        int rectwidth,
        int rectheight )
#endif /* _NO_PROTO */
{
   GC  tmp_gc;

   tmp_gc = GetParentBackgroundGC (pb);

   if (tmp_gc)
   {
      _XmDrawSimpleHighlight(XtDisplay(pb), XtWindow(pb), tmp_gc,
		  dx, dy, rectwidth, rectheight, borderwidth);

      XtReleaseGC( XtParent(pb), tmp_gc);
   }
}

/*************************************************************************
 *
 * SetPushButtonSize(newpb)
 *  Picks the larger dimension when the armPixmap is a
 *  different size than the label pixmap(i.e the unarm pixmap).
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
SetPushButtonSize(newpb)
     XmPushButtonWidget newpb;
#else
SetPushButtonSize(
     XmPushButtonWidget newpb)
#endif /* _NO_PROTO */
{
  XmLabelPart         *lp = &(newpb->label);

  unsigned int junk;
  unsigned int  onW = 0 , onH = 0, d;

  lp->acc_TextRect.width = 0;
  lp->acc_TextRect.height = 0;

   /* We know it's a pixmap so find out how how big it is */
  if (newpb->pushbutton.arm_pixmap != XmUNSPECIFIED_PIXMAP)
      XGetGeometry (XtDisplay(newpb),
                    newpb->pushbutton.arm_pixmap,
                    (Window*)&junk, /* returned root window */
                    (int*)&junk, (int*)&junk, /* x, y of pixmap */
                    &onW, &onH, /* width, height of pixmap */
                    &junk,    /* border width */
                    &d);      /* depth */

  if ((onW > lp->TextRect.width) || (onH > lp->TextRect.height))
  {

    lp->TextRect.width =  (unsigned short) onW;
    lp->TextRect.height = (unsigned short) onH;
  }

  if (lp->_acc_text != NULL)
  {
       Dimension w, h;

       /*
        * If we have a string then size it.
        */
       if (!_XmStringEmpty (lp->_acc_text))
       {
         _XmStringExtent(lp->font, lp->_acc_text, &w, &h);
         lp->acc_TextRect.width = (unsigned short)w;
         lp->acc_TextRect.height = (unsigned short)h;
       }
  }

     /* increase margin width if necessary to accomadate accelerator text */
  if (lp->_acc_text != NULL)
       if (lp->margin_right < lp->acc_TextRect.width + LABEL_ACC_PAD)
         lp->margin_right = lp->acc_TextRect.width + LABEL_ACC_PAD;

     /* Has a width been specified?  */

  if (newpb->core.width == 0)
        newpb->core.width = (Dimension)
          lp->TextRect.width +
              lp->margin_left + lp->margin_right +
                  (2 * (lp->margin_width
                        + newpb->primitive.highlight_thickness
                        + newpb->primitive.shadow_thickness));

  switch (lp -> alignment)
  {
      case XmALIGNMENT_BEGINNING:
        lp->TextRect.x = (short) lp->margin_width +
          lp->margin_left +
              newpb->primitive.highlight_thickness +
                  newpb->primitive.shadow_thickness;

        break;

  case XmALIGNMENT_END:
       lp->TextRect.x = (short) newpb->core.width -
         (newpb->primitive.highlight_thickness +
          newpb->primitive.shadow_thickness +
          lp->margin_width + lp->margin_right +
          lp->TextRect.width);
       break;

  default:
       lp->TextRect.x =  (short) newpb->primitive.highlight_thickness
         + newpb->primitive.shadow_thickness
             + lp->margin_width + lp->margin_left +
                 ((newpb->core.width - lp->margin_left
                   - lp->margin_right
                   - (2 * (lp->margin_width
                           + newpb->primitive.highlight_thickness
                           + newpb->primitive.shadow_thickness))
                   - lp->TextRect.width) / 2);

  break;
  }

    /* Has a height been specified? */
  if (newpb->core.height == 0)
        newpb->core.height = (Dimension)
          Max(lp->TextRect.height, lp->acc_TextRect.height) +
              lp->margin_top +
                  lp->margin_bottom
                      + (2 * (lp->margin_height
                              + newpb->primitive.highlight_thickness
                              + newpb->primitive.shadow_thickness));

  lp->TextRect.y =  (short) newpb->primitive.highlight_thickness
        + newpb->primitive.shadow_thickness
          + lp->margin_height + lp->margin_top +
              ((newpb->core.height - lp->margin_top
                - lp->margin_bottom
                - (2 * (lp->margin_height
                       + newpb->primitive.highlight_thickness
                        + newpb->primitive.shadow_thickness))
                - lp->TextRect.height) / 2);

  if (lp->_acc_text != NULL)
  {

       lp->acc_TextRect.x = (short) newpb->core.width -
         newpb->primitive.highlight_thickness -
             newpb->primitive.shadow_thickness -
                 newpb->label.margin_width -
                     newpb->label.margin_right +
                         LABEL_ACC_PAD;

       lp->acc_TextRect.y =  (short) newpb->primitive.highlight_thickness
         + newpb->primitive.shadow_thickness
             + lp->margin_height + lp->margin_top +
                 ((newpb->core.height - lp->margin_top
                   - lp->margin_bottom
                   - (2 * (lp->margin_height
                           + newpb->primitive.highlight_thickness
                           + newpb->primitive.shadow_thickness))
                   - lp->acc_TextRect.height) / 2);

  }

  if (newpb->core.width == 0)    /* set core width and height to a */
     newpb->core.width = 1;       /* default value so that it doesn't */
  if (newpb->core.height == 0)   /* generate a Toolkit Error */
     newpb->core.height = 1;

}

