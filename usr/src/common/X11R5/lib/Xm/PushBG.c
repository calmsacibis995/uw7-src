#pragma ident	"@(#)m1.2libs:Xm/PushBG.c	1.5"
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
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
/*
 * Include files & Static Routine Definitions
 */

#include <Xm/PushBGP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ManagerP.h>
#include "XmI.h"
#include <X11/ShellP.h>
#include <Xm/CascadeB.h>
#include <Xm/CacheP.h>
#include <Xm/ExtObjectP.h>
#include <Xm/RowColumnP.h>
#include <Xm/DrawP.h>
#include <Xm/MenuUtilP.h>
#include <string.h>
#include <stdio.h>
#include "TravActI.h"

#define DELAY_DEFAULT 100
#define XmINVALID_MULTICLICK 255

struct  PBTimeOutEvent
        {  XmPushButtonGadget  pushbutton;
           XEvent              *xevent;
        } ;

struct  PBbox
     { int pbx;
       int pby;
       int pbWidth;
       int pbHeight;
     };


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static int _XmPushBCacheCompare() ;
static void InputDispatch() ;
static void Arm() ;
static void Activate() ;
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
static void SecondaryObjectCreate() ;
static void InitializePosthook() ;
static void Initialize() ;
static void GetFillGC() ;
static void GetBackgroundGC() ;
static Boolean VisualChange() ;
static Boolean SetValuesPrehook() ;
static void GetValuesPrehook() ;
static void GetValuesPosthook() ;
static Boolean SetValuesPosthook() ;
static Boolean SetValues() ;
static void Help() ;
static void Destroy() ;
static void Resize();
static void ActivateCommonG() ;
static Cardinal GetPushBGClassSecResData() ;
static XtPointer GetPushBGClassSecResBase() ;
static void EraseDefaultButtonShadow() ;
static void DrawDefaultButtonShadow() ;
static int AdjustHighLightThickness() ;
static void Redisplay() ;
static void DrawPushButtonLabelGadget() ;
static void DrawPushButtonGadgetShadows() ;
static void DrawPBGadgetShadows() ;
static void EraseDefaultButtonShadows() ;
static void DrawDefaultButtonShadows() ;
static void DrawPushBGBackground() ;
static XmImportOperator ShowAsDef_ToHorizPix() ;
static Boolean ComputePBLabelArea() ;
static void ExportHighlightThickness() ;
static void SetPushButtonSize();

#else

static int _XmPushBCacheCompare( 
                        XtPointer A,
                        XtPointer B) ;
static void InputDispatch( 
                        Widget wid,
                        XEvent *event,
                        Mask event_mask) ;
static void Arm( 
                        XmPushButtonGadget pb,
                        XEvent *event) ;
static void Activate( 
                        XmPushButtonGadget pb,
                        XEvent *event) ;
static void ArmAndActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmTimeout( 
                        XtPointer data,
                        XtIntervalId *id) ;
static void Disarm( 
                        XmPushButtonGadget pb,
                        XEvent *event) ;
static void BtnDown( 
                        XmPushButtonGadget pb,
                        XEvent *event) ;
static void BtnUp( 
                        Widget wid,
                        XEvent *event) ;
static void Enter( 
                        Widget wid,
                        XEvent *event) ;
static void Leave( 
                        Widget wid,
                        XEvent *event) ;
static void BorderHighlight( 
                        Widget wid) ;
static void DrawBorderHighlight( 
                        Widget wid) ;
static void BorderUnhighlight( 
                        Widget wid) ;
static void KeySelect( 
                        Widget wid,
                        XEvent *event) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void SecondaryObjectCreate( 
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
                        XmPushButtonGadget pb) ;
static void GetBackgroundGC( 
                        XmPushButtonGadget pb) ;
static Boolean VisualChange( 
                        Widget wid,
                        Widget cmw,
                        Widget nmw) ;
static Boolean SetValuesPrehook( 
                        Widget oldParent,
                        Widget refParent,
                        Widget newParent,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesPrehook( 
                        Widget newParent,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesPosthook( 
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesPosthook( 
                        Widget current,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Help( 
                        XmPushButtonGadget pb,
                        XEvent *event) ;
static void Destroy( 
                        Widget wid) ;
static void Resize(
                        Widget wid) ;
static void ActivateCommonG( 
                        XmPushButtonGadget pb,
                        XEvent *event,
                        Mask event_mask) ;
static Cardinal GetPushBGClassSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **data_rtn) ;
static XtPointer GetPushBGClassSecResBase( 
                        Widget widget,
                        XtPointer client_data) ;
static void EraseDefaultButtonShadow( 
                        XmPushButtonGadget pb) ;
static void DrawDefaultButtonShadow( 
                        XmPushButtonGadget pb) ;
static int AdjustHighLightThickness( 
                        XmPushButtonGadget new_w,
                        XmPushButtonGadget current) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void DrawPushButtonLabelGadget( 
                        XmPushButtonGadget pb,
                        XEvent *event,
                        Region region) ;
static void DrawPushButtonGadgetShadows( 
                        XmPushButtonGadget pb) ;
static void DrawPBGadgetShadows( 
                        XmPushButtonGadget pb) ;
static void EraseDefaultButtonShadows( 
                        XmPushButtonGadget pb) ;
static void DrawDefaultButtonShadows( 
                        XmPushButtonGadget pb) ;
static void DrawPushBGBackground( 
                        XmPushButtonGadget pb) ;
static XmImportOperator ShowAsDef_ToHorizPix( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
static Boolean ComputePBLabelArea( 
                        XmPushButtonGadget pb,
                        struct PBbox *box) ;
static void ExportHighlightThickness( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
static void SetPushButtonSize(
                        XmPushButtonGadget newpb) ;
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/************************************************
 The uncached resources for Push Button  
 ************************************************/

static XtResource resources[] = 
{     
   {
     XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonGadgetRec, pushbutton.activate_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonGadgetRec, pushbutton.arm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNdisarmCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmPushButtonGadgetRec, pushbutton.disarm_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, 
     sizeof(Dimension),
     XtOffsetOf( struct _XmPushButtonGadgetRec, gadget.shadow_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNtraversalOn,
     XmCTraversalOn,
     XmRBoolean,
     sizeof (Boolean),
     XtOffsetOf( struct _XmGadgetRec, gadget.traversal_on),
     XmRImmediate, 
     (XtPointer) True
   },

   {
     XmNhighlightThickness,
     XmCHighlightThickness,
     XmRHorizontalDimension,
     sizeof (Dimension),
     XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
     XmRImmediate, 
     (XtPointer) 2
   },

   {
     XmNshowAsDefault,
     XmCShowAsDefault,
     XmRBooleanDimension,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonGadgetRec, pushbutton.show_as_default),
     XmRImmediate, (XtPointer) 0
   },

};

static XmSyntheticResource syn_resources[] =
{
   {
     XmNshowAsDefault,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonGadgetRec, pushbutton.show_as_default),
     _XmFromHorizontalPixels,
     ShowAsDef_ToHorizPix
   },
   {
     XmNhighlightThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
     ExportHighlightThickness,
     _XmToHorizontalPixels
   },

};

/**********************************************
 Cached resources for PushButton Gadget
 **********************************************/

static XtResource cache_resources[] =
{

   {
     XmNmultiClick,
     XmCMultiClick,
     XmRMultiClick,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.multiClick ),
     XmRImmediate,
     (XtPointer) XmINVALID_MULTICLICK
   },

   {
     XmNdefaultButtonShadowThickness,
     XmCDefaultButtonShadowThickness,
     XmRHorizontalDimension, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.default_button_shadow_thickness ),
     XmRImmediate,
     (XtPointer) 0
   },

   {
     XmNfillOnArm, XmCFillOnArm, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.fill_on_arm),
     XmRImmediate, (XtPointer) True
   },

   {
     XmNarmColor, XmCArmColor, XmRPixel, sizeof (Pixel),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.arm_color),
     XmRCallProc, (XtPointer) _XmSelectColorDefault
   },

   {
     XmNarmPixmap, XmCArmPixmap, XmRGadgetPixmap, sizeof (Pixmap),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.arm_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },
};

/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource cache_syn_resources[] =
{
   {
     XmNdefaultButtonShadowThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmPushButtonGCacheObjRec, pushbutton_cache.default_button_shadow_thickness ),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels
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

static XmCacheClassPart PushButtonClassCachePart = {
    {NULL, 0, 0},            /* head of class cache list */
    _XmCacheCopy,            /* Copy routine     */
    _XmCacheDelete,          /* Delete routine   */
    _XmPushBCacheCompare,    /* Comparison routine   */
};

static XmBaseClassExtRec   PushBGClassExtensionRec = {
    NULL,   		 			/*   next_extension    */
    NULLQUARK,    				/* record_typ  */
    XmBaseClassExtVersion,			/*  version  */
    sizeof(XmBaseClassExtRec), 			/* record_size  */
    XmInheritInitializePrehook,			/*  initializePrehook  */
    SetValuesPrehook,   			/* setValuesPrehook  */
    InitializePosthook,				/* initializePosthook  */
    SetValuesPosthook, 				/* setValuesPosthook  */
    (WidgetClass)&xmPushButtonGCacheObjClassRec,/* secondaryObjectClass */
    SecondaryObjectCreate, 		        /* secondaryObjectCreate */
    GetPushBGClassSecResData,                   /* getSecResData  */
    {0},           		                /* fast subclass  */
    GetValuesPrehook,   			/* getValuesPrehook  */
    GetValuesPosthook,   			/* getValuesPosthook  */
    (XtWidgetClassProc)NULL,                  /* classPartInitPrehook */
    (XtWidgetClassProc)NULL,                  /* classPartInitPosthook*/
    (XtResourceList)NULL,                     /* ext_resources        */
    (XtResourceList)NULL,                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    XmInheritWidgetNavigable,                 /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
};

/* ext rec static initialization */
externaldef(xmpushbuttongcacheobjclassrec)
XmPushButtonGCacheObjClassRec xmPushButtonGCacheObjClassRec =
{
  {
      /* superclass         */    (WidgetClass) &xmLabelGCacheObjClassRec,
      /* class_name         */    "XmPushButtonGadget",
      /* widget_size        */    sizeof(XmPushButtonGCacheObjRec),
      /* class_initialize   */    (XtProc)NULL,
      /* chained class init */    (XtWidgetClassProc)NULL,
      /* class_inited       */    False,
      /* initialize         */    (XtInitProc)NULL,
      /* initialize hook    */    (XtArgsProc)NULL,
      /* realize            */    NULL,
      /* actions            */    NULL,
      /* num_actions        */    0,
      /* resources          */    cache_resources,
      /* num_resources      */    XtNumber(cache_resources),
      /* xrm_class          */    NULLQUARK,
      /* compress_motion    */    False,
      /* compress_exposure  */    False,
      /* compress enter/exit*/    False,
      /* visible_interest   */    False,
      /* destroy            */    (XtWidgetProc)NULL,
      /* resize             */    NULL,
      /* expose             */    NULL,
      /* set_values         */    (XtSetValuesFunc)NULL,
      /* set values hook    */    (XtArgsFunc)NULL,
      /* set values almost  */    NULL,
      /* get values hook    */    (XtArgsProc)NULL,
      /* accept_focus       */    NULL,
      /* version            */    XtVersion,
      /* callback offsetlst */    NULL,
      /* default trans      */    NULL,
      /* query geo proc     */    NULL,
      /* display accelerator*/    NULL,
      /* extension record   */    NULL,
   },

   {
      /* synthetic resources */   cache_syn_resources,
      /* num_syn_resources   */   XtNumber(cache_syn_resources), 
      /* extension           */   NULL,
   }
};

/*  The PushButton class record definition  */

XmGadgetClassExtRec _XmPushBGadClassExtRec = {
    NULL,
    NULLQUARK,
    XmGadgetClassExtVersion,
    sizeof(XmGadgetClassExtRec),
    XmInheritBaselineProc,                  /* widget_baseline */
    XmInheritDisplayRectProc,               /* widget_display_rect */
};

externaldef(xmpushbuttongadgetclassrec)
	XmPushButtonGadgetClassRec xmPushButtonGadgetClassRec = 
{
  {
      (WidgetClass) &xmLabelGadgetClassRec,  	/* superclass            */
      "XmPushButtonGadget",             	/* class_name	         */
      sizeof(XmPushButtonGadgetRec),    	/* widget_size	         */
      ClassInitialize,         			/* class_initialize      */
      ClassPartInitialize,              	/* class_part_initialize */
      FALSE,                            	/* class_inited          */
      Initialize,          	                /* initialize	         */
      (XtArgsProc)NULL,                         /* initialize_hook       */
      NULL,                     		/* realize	         */
      NULL,                             	/* actions               */
      0,			        	/* num_actions    	 */
      resources,                        	/* resources	         */
      XtNumber(resources),              	/* num_resources         */
      NULLQUARK,                        	/* xrm_class	         */
      TRUE,                             	/* compress_motion       */
      XtExposeCompressMaximal,          	/* compress_exposure     */
      TRUE,                             	/* compress_enterleave   */
      FALSE,                            	/* visible_interest      */	
      Destroy,           	                /* destroy               */	
      Resize,	                		/* resize                */
      Redisplay,        		        /* expose                */	
      SetValues,      	                        /* set_values	         */	
      (XtArgsFunc)NULL,                       	/* set_values_hook       */
      XtInheritSetValuesAlmost,         	/* set_values_almost     */
      (XtArgsProc)NULL,                        	/* get_values_hook       */
      NULL,                 			/* accept_focus	         */	
      XtVersion,                        	/* version               */
      NULL,                             	/* callback private      */
      NULL,                             	/* tm_table              */
      XtInheritQueryGeometry,           	/* query_geometry        */
      NULL,					/* display_accelerator   */
      (XtPointer)&PushBGClassExtensionRec,	/* extension             */
   },

   {          /* gadget class record */
      BorderHighlight,		/* border highlight   */
      BorderUnhighlight,	/* border_unhighlight */
      ArmAndActivate,		/* arm_and_activate   */
      InputDispatch,		/* input dispatch     */
      VisualChange,    		/* visual_change      */
      syn_resources,   		/* syn resources      */
      XtNumber(syn_resources),  /* num syn_resources  */
      &PushButtonClassCachePart,	/* class cache part   */
      (XtPointer)&_XmPushBGadClassExtRec, /* extension          */
   },

   { 	/* label_class record */
 
      XmInheritWidgetProc,	/* setOverrideCallback */
      XmInheritMenuProc,	/* menu proc's entry   */
      NULL,			/* extension	       */   
   },

   {    /* pushbutton class record */
      NULL,			   /* extension 	 */
   }
};

externaldef(xmpushbuttongadgetclass) WidgetClass xmPushButtonGadgetClass = 
   (WidgetClass) &xmPushButtonGadgetClassRec;

/*****************************************************************
 *  SPECIAL PROPERTIES OF PUSHBUTTON GADGET INSIDE A MENU:
 *   When a PushButton (widget/gadget) is incorporated in a Menu
 *   (Pulldownor Popup) - its properties get modified in these ways:
 *   (1) The redisplay routine should not draw its background nor draw
 *	     its shadows.  It should draw only the label. To comply with 
 *	     this means that the arm-color and background color are not
 *	     of any use. As a result the values in the FillGC and BackgroundGC
 *	     are not initialized and are likely to be bogus. This causes
 *	     special casing of Initialize and SetValues routines.
 *   (2) PushButton does not show its depressed appearance in the
 *       menu. This will cause Arm(), DisArm(), ArmAndActivate routines
 *	     to have special cases.
 *   In short the properties of Pushbutton in a menu are so different
 *	  that practically all major routines in this widget will have to
 *	  special-cased to accommodate this difference as if two different
 *	  classes are being glued to one class.
 *******************************************************************/

/*******************************************************************
 *
 *  _XmPushBCacheCompare
 *
 *******************************************************************/
static int 
#ifdef _NO_PROTO
_XmPushBCacheCompare( A, B )
        XtPointer A ;
        XtPointer B ;
#else
_XmPushBCacheCompare(
        XtPointer A,
        XtPointer B )
#endif /* _NO_PROTO */
{
        XmPushButtonGCacheObjPart *pushB_inst =
                                              (XmPushButtonGCacheObjPart *) A ;
        XmPushButtonGCacheObjPart *pushB_cache_inst =
                                              (XmPushButtonGCacheObjPart *) B ;
    if((pushB_inst->fill_on_arm == pushB_cache_inst->fill_on_arm) &&
       (pushB_inst->arm_color == pushB_cache_inst->arm_color) &&
       (pushB_inst->arm_pixmap == pushB_cache_inst->arm_pixmap) &&
       (pushB_inst->unarm_pixmap == pushB_cache_inst->unarm_pixmap) &&
       (pushB_inst->fill_gc == pushB_cache_inst->fill_gc) &&
       (pushB_inst->background_gc == pushB_cache_inst->background_gc) &&
       (pushB_inst->multiClick == pushB_cache_inst->multiClick) &&
       (pushB_inst->default_button_shadow_thickness ==
                    pushB_cache_inst->default_button_shadow_thickness) &&
       (pushB_inst->timer == pushB_cache_inst->timer))
       return 1;
    else
       return 0;
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
        XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
	if ( (event_mask & XmARM_EVENT) ||
	    ((PBG_MultiClick(pb) == XmMULTICLICK_KEEP) &&
	     (event_mask & XmMULTI_ARM_EVENT)) )
	{
	    if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
		LabG_MenuType(pb) == XmMENU_POPUP)
		BtnDown (pb, event);
	    else
		Arm (pb, event);
	}

	else if (event_mask & XmACTIVATE_EVENT)
        {
	    PBG_ClickCount (pb) = 1; 
	    /* pb->pushbutton.click_count = 1; */
	    ActivateCommonG ( pb, event, event_mask);
        }
	else if (event_mask & XmMULTI_ACTIVATE_EVENT)
        { /* if XmNMultiClick resource is set to DISCARD - do nothing
             else increment clickCount and Call ActivateCommonG .
	     */
	    if (PBG_MultiClick(pb) == XmMULTICLICK_KEEP) 
	    { ( PBG_ClickCount (pb))++;
              ActivateCommonG ( pb, event, event_mask);
	  }
        }

	else if (event_mask & XmHELP_EVENT)
	    Help (pb, event);
	else if (event_mask & XmENTER_EVENT)
	    Enter ((Widget) pb, event);
	else if (event_mask & XmLEAVE_EVENT)
	    Leave ((Widget) pb, event);
	else if (event_mask & XmFOCUS_IN_EVENT)
	    _XmFocusInGadget( (Widget) pb, event, NULL, NULL);
	
	else if (event_mask & XmFOCUS_OUT_EVENT)
	    _XmFocusOutGadget( (Widget) pb, event, NULL, NULL);
                                                            
	else if (event_mask & XmBDRAG_EVENT)
	    _XmProcessDrag ((Widget) pb, event, NULL, NULL);
}



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
Arm( pb, event )
        XmPushButtonGadget pb ;
        XEvent *event ;
#else
Arm(
        XmPushButtonGadget pb,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmPushButtonCallbackStruct call_value;

   PBG_Armed(pb) = TRUE;

   (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
		rect_class.expose)) ((Widget) pb, event, (Region) NULL);

   if (PBG_ArmCallback(pb))
   {
      XFlush(XtDisplay (pb));

      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb), &call_value);
   }
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
Activate( pb, event )
        XmPushButtonGadget pb ;
        XEvent *event ;
#else
Activate(
        XmPushButtonGadget pb,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XButtonEvent *buttonEvent = (XButtonEvent *) event;
    XmPushButtonCallbackStruct call_value;   

   PBG_Armed(pb) = FALSE;

   (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
		rect_class.expose)) ((Widget) pb, event, (Region) NULL);

   if ((buttonEvent->x < pb->rectangle.x + pb->rectangle.width) &&
       (buttonEvent->y < pb->rectangle.y + pb->rectangle.height) &&
       (buttonEvent->x >= pb->rectangle.x) &&
       (buttonEvent->y >= pb->rectangle.y))
    {

       call_value.reason = XmCR_ACTIVATE;
       call_value.event = event;
       call_value.click_count = PBG_ClickCount(pb);
					/*   pb->pushbutton.click_count;   */

       /* _XmRecordEvent(event); */       /* Fix CR 3407 DRand 6/16/92 */

       /* if the parent is a RowColumn, notify it about the select */
       if (XmIsRowColumn(XtParent(pb)))
       {
	  (* xmLabelGadgetClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
							   XtParent(pb),
							   FALSE, pb,
							   &call_value);
       }

       if ((! LabG_SkipCallback(pb)) &&
	   (PBG_ActivateCallback(pb)))
       {
	  XFlush (XtDisplay (pb));
	  XtCallCallbackList ((Widget) pb, PBG_ActivateCallback(pb),
				 &call_value);
       }
    }
}

/************************************************************************
 *
 *     ArmAndActivate
 *  - Called if the PushButtonGadget is being selected via keyboard
 *    i.e. by pressing <return> or <space-bar>.
 *  - Called by SelectionBox and FileSelectionBox code when they receive
 *    a default-action callback on their embedded XmList widgets.
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
   XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;
   Boolean already_armed = PBG_Armed(pb);
   Boolean is_menupane = (LabG_MenuType(pb) == XmMENU_PULLDOWN) ||
			 (LabG_MenuType(pb) == XmMENU_POPUP);
   Boolean parent_is_torn;
   Boolean torn_has_focus = FALSE;    /* must be torn! */

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
      PBG_Armed(pb) = FALSE;

      if (parent_is_torn && !torn_has_focus)
      {
         /* Freeze tear off visuals in case accelerators are not in
          * same context
          */
         (* xmLabelGadgetClassRec.label_class.menuProcs)
            (XmMENU_RESTORE_TEAROFF_TO_MENUSHELL, XtParent(pb), NULL,
            event, NULL);
      }

      if (torn_has_focus)
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
            (XmMENU_POPDOWN, XtParent(pb), NULL, event, NULL);
      else
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);

      /* if its in a torn off menu pane, show depressed button briefly */
      if (torn_has_focus)     /*parent_is_torn!*/
      {
	 /* Set the focus here. */
	 XmProcessTraversal((Widget) pb, XmTRAVERSE_CURRENT);

	 _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
	    XmParentBottomShadowGC(pb), XmParentTopShadowGC(pb),
	    pb -> rectangle.x + pb -> gadget.highlight_thickness,
	    pb -> rectangle.y + pb -> gadget.highlight_thickness,
	    pb -> rectangle.width - 2 * pb->gadget.highlight_thickness,
	    pb -> rectangle.height - 2 * pb->gadget.highlight_thickness,
	    pb -> gadget.shadow_thickness,
	    XmSHADOW_OUT);
      }
   }
   else
   {
      /* we have no idea what the event is, so don't pass it through,
      ** in case some future subclass is smarter about reexposure
      */
      PBG_Armed(pb) = TRUE;
      (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
	  rect_class.expose)) ((Widget) pb, (XEvent *)NULL, (Region) NULL);
   }

   XFlush (XtDisplay (pb));

   if (event)
   {
       if (event->type == KeyPress)  PBG_ClickCount (pb) = 1;
   }

   /* If the parent is a RowColumn, set the lastSelectToplevel before the arm.
    * It's ok if this is recalled later.
    */
   if (XmIsRowColumn(XtParent(pb)))
   {
      (* xmLabelGadgetClassRec.label_class.menuProcs) (
	 XmMENU_GET_LAST_SELECT_TOPLEVEL, XtParent(pb));
   }

   if (PBG_ArmCallback(pb) && !already_armed)
   {
      call_value.reason = XmCR_ARM;
      call_value.event = event;
      call_value.click_count = PBG_ClickCount (pb);
      XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb), &call_value);
   }

   call_value.reason = XmCR_ACTIVATE;
   call_value.event = event;
   call_value.click_count = PBG_ClickCount (pb);

   /* if the parent is a RowColumn, notify it about the select */
   if (XmIsRowColumn(XtParent(pb)))
   {
      (* xmLabelGadgetClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
	 XtParent(pb), FALSE, pb, &call_value);
   }

   if ((! LabG_SkipCallback(pb)) && (PBG_ActivateCallback(pb)))
   {
      XFlush (XtDisplay (pb));
      XtCallCallbackList ((Widget) pb, PBG_ActivateCallback(pb), &call_value);
   }

   PBG_Armed(pb) = FALSE;
       
   if (PBG_DisarmCallback(pb))
   {
      XFlush (XtDisplay (pb));
      call_value.reason = XmCR_DISARM;
      XtCallCallbackList ((Widget) pb, PBG_DisarmCallback(pb), &call_value);
   }

   if (is_menupane)
   {
      if (torn_has_focus)
      {
	 /* Leave the focus widget in an armed state */
	 PBG_Armed(pb) = TRUE;

	 if (PBG_ArmCallback(pb))
	 {
	    XFlush (XtDisplay (pb));
	    call_value.reason = XmCR_ARM;
	    XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb), &call_value);
	 }
      }
      else
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
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
      if ((pb->object.being_destroyed == False) && (!(PBG_Timer(pb))))
	 PBG_Timer(pb) = (int) XtAppAddTimeOut(
	    XtWidgetToApplicationContext((Widget)pb),
	    (unsigned long) DELAY_DEFAULT, ArmTimeout, (XtPointer)(pb));
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
   XmPushButtonGadget pb = (XmPushButtonGadget) data;

   PBG_Timer(pb) = 0;
   if (XtIsRealized (pb) && XtIsManaged (pb))
   {
       if ((LabG_MenuType(pb) == XmMENU_PULLDOWN ||
	    LabG_MenuType(pb) == XmMENU_POPUP))
       {
           /* When rapidly clicking, the focus may have moved away from this
            * widget, so check before changing the shadow.
            */
           if (_XmFocusIsInShell((Widget)pb) &&
               (XmGetFocusWidget((Widget)pb) == (Widget)pb) )
           {
	      /* in a torn off menu, redraw shadows */
	      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			    XmParentTopShadowGC(pb),
			    XmParentBottomShadowGC(pb),
			    pb -> rectangle.x +
			     pb -> gadget.highlight_thickness,
			    pb -> rectangle.y +
			     pb -> gadget.highlight_thickness,
			    pb -> rectangle.width - 2 *
			     pb->gadget.highlight_thickness,
			    pb -> rectangle.height - 2 *
			     pb->gadget.highlight_thickness,
			    pb -> gadget.shadow_thickness,
			    XmSHADOW_OUT);
           }
       }
       else
       {
	   (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
	       rect_class.expose)) ((Widget) pb, NULL, (Region) NULL);
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
Disarm( pb, event )
        XmPushButtonGadget pb ;
        XEvent *event ;
#else
Disarm(
        XmPushButtonGadget pb,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmPushButtonCallbackStruct call_value;

   PBG_Armed(pb) = FALSE;

   if (PBG_DisarmCallback(pb))
   {
      call_value.reason = XmCR_DISARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, PBG_DisarmCallback(pb), &call_value);
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
BtnDown( pb, event )
        XmPushButtonGadget pb ;
        XEvent *event ;
#else
BtnDown(
        XmPushButtonGadget pb,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmPushButtonCallbackStruct call_value;
   Boolean already_armed;
   ShellWidget popup;

   /* Popdown other popus that may be up */
   if (!(popup = (ShellWidget)_XmGetRC_PopupPosted(XtParent(pb))))
   {
      if (!XmIsMenuShell(XtParent(XtParent(pb))))
      {
	 /* In case tear off not armed and no grabs in place, do it now.
	  * Ok if already armed and grabbed - nothing done.
	  */
	 (* xmLabelGadgetClassRec.label_class.menuProcs) 
	    (XmMENU_TEAR_OFF_ARM, XtParent(pb));
      }
   }

   if (popup)
   {
      Widget w;
      
      if (popup->shell.popped_up)
         (* xmLabelGadgetClassRec.label_class.menuProcs)
	    (XmMENU_SHELL_POPDOWN, (Widget) popup, NULL, event, NULL);

      /* If active_child is a cascade (highlighted), then unhighlight it. */
      w = ((XmManagerWidget)XtParent(pb))->manager.active_child;
      if (w && (XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w)))
	 XmCascadeButtonHighlight (w, FALSE);
   }

   /* Set focus to this button.  This must follow the possible
    * unhighlighting of the CascadeButton else it'll screw up active_child.
    */
   (void)XmProcessTraversal( (Widget) pb, XmTRAVERSE_CURRENT);
         /* get the location cursor - get consistent with Gadgets */

#ifdef CDE_VISUAL	 /* etched in menu button */
   {
   Boolean etched_in = False;
   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
   if (etched_in) {
       Boolean tmp_arm = PBG_Armed(pb);
       PBG_Armed(pb) = TRUE;
       Redisplay((Widget) pb, NULL, NULL);
       PBG_Armed(pb) = tmp_arm;
   }
   else
       _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
                  XmParentTopShadowGC(pb),
                  XmParentBottomShadowGC(pb),
                  pb -> rectangle.x + pb -> gadget.highlight_thickness,
                  pb -> rectangle.y + pb -> gadget.highlight_thickness,
                  pb -> rectangle.width - 2 * pb->gadget.highlight_thickness,
                  pb -> rectangle.height - 2 * pb->gadget.highlight_thickness,
                  pb -> gadget.shadow_thickness,
                  XmSHADOW_OUT);
   }
#else
   _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
                  XmParentTopShadowGC(pb),
                  XmParentBottomShadowGC(pb),
                  pb -> rectangle.x + pb -> gadget.highlight_thickness,
                  pb -> rectangle.y + pb -> gadget.highlight_thickness,
                  pb -> rectangle.width - 2 * pb->gadget.highlight_thickness,
                  pb -> rectangle.height - 2 * pb->gadget.highlight_thickness,
                  pb -> gadget.shadow_thickness,
                  XmSHADOW_OUT);
#endif

   already_armed = PBG_Armed(pb);
   PBG_Armed(pb) = TRUE;

   if (PBG_ArmCallback(pb) && !already_armed)
   {
      XFlush (XtDisplay (pb));

      call_value.reason = XmCR_ARM;
      call_value.event = event;
      XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb), &call_value);
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
BtnUp( wid, event )
        Widget wid ;
        XEvent *event ;
#else
BtnUp(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;
   Boolean flushDone = False;
   Boolean popped_up;
   Boolean is_menupane = (LabG_MenuType(pb) == XmMENU_PULLDOWN) ||
			 (LabG_MenuType(pb) == XmMENU_POPUP);
   Widget shell = XtParent(XtParent(pb));

   PBG_Armed(pb) = FALSE;

   if (is_menupane && !XmIsMenuShell(shell))
      (* xmLabelGadgetClassRec.label_class.menuProcs)
	 (XmMENU_POPDOWN, (Widget) pb, NULL, event, &popped_up);
   else
      (* xmLabelGadgetClassRec.label_class.menuProcs)
	 (XmMENU_BUTTON_POPDOWN, (Widget) pb, NULL, event, &popped_up);

   _XmRecordEvent(event);
   
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
   if (XmIsRowColumn(XtParent(pb)))
   {
      (* xmLabelGadgetClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						       XtParent(pb), FALSE, pb,
						       &call_value);
	 
      flushDone = True; 
   }

   if ((! LabG_SkipCallback(pb)) &&
       (PBG_ActivateCallback(pb)))
   {
      XFlush (XtDisplay (pb));
      flushDone = True;
      XtCallCallbackList ((Widget) pb, PBG_ActivateCallback(pb),
			     &call_value);
   }

   if (PBG_DisarmCallback(pb))
   {
         if (!flushDone)
	     XFlush (XtDisplay (pb));
         call_value.reason = XmCR_DISARM;
         call_value.event = event;
         XtCallCallbackList ((Widget) pb, PBG_DisarmCallback(pb), &call_value);
   }

   /* If the original shell does not indicate an active menu, but rather a
    * tear off pane, leave the button in an armed state.  Also, briefly
    * display the button as depressed to give the user some feedback of
    * the selection.
    */
    if (is_menupane)
    {
      if (!XmIsMenuShell(shell))
      {
	 if (XtIsSensitive(pb))
	 {
	     _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			      XmParentBottomShadowGC(pb),
			      XmParentTopShadowGC(pb),
			      pb -> rectangle.x +
				   pb -> gadget.highlight_thickness,
			      pb -> rectangle.y +
				   pb -> gadget.highlight_thickness,
			      pb -> rectangle.width - 2 *
				   pb->gadget.highlight_thickness,
			      pb -> rectangle.height - 2 *
			      pb->gadget.highlight_thickness,
			      pb -> gadget.shadow_thickness,
			      XmSHADOW_OUT);

	     XFlush (XtDisplay (pb));
	     flushDone = True;
	     
	     /* set timer to redraw the shadow out again */
	     if (pb->object.being_destroyed == False)
	     {
		 if (!(PBG_Timer(pb)))
		     PBG_Timer(pb) = (int) XtAppAddTimeOut
			 (XtWidgetToApplicationContext((Widget)pb),
			  (unsigned long) DELAY_DEFAULT,
			  ArmTimeout,
			  (XtPointer)(pb));
	     }

	     PBG_Armed(pb) = TRUE;
	     if (PBG_ArmCallback(pb))
	     {	
		 if (!flushDone)
		     XFlush (XtDisplay (pb));
		 call_value.reason = XmCR_ARM;
		 call_value.event = event;
		 XtCallCallbackList((Widget)pb, PBG_ArmCallback(pb), &call_value);
	     }
	  }
      }
      else
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(pb), NULL, event, NULL);
   }

   _XmSetInDragMode((Widget) pb, False);

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
Enter( wid, event )
        Widget wid ;
        XEvent *event ;
#else
Enter(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
       LabG_MenuType(pb) == XmMENU_POPUP)
   {
      if ((((ShellWidget) XtParent(XtParent(pb)))->shell.popped_up) &&
          _XmGetInDragMode((Widget) pb))
      {
          if (PBG_Armed(pb))
	     return;

	  /* So KHelp event is delivered correctly */
         _XmSetFocusFlag( XtParent(XtParent(pb)), XmFOCUS_IGNORE, TRUE);
         XtSetKeyboardFocus(XtParent(XtParent(pb)), (Widget)pb);
         _XmSetFocusFlag( XtParent(XtParent(pb)), XmFOCUS_IGNORE, FALSE);

#ifdef CDE_VISUAL	/* etched in menu button */
          {
          Boolean etched_in = False;

          XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
	  if (etched_in) {
	      PBG_Armed(pb) = TRUE;
	      Redisplay((Widget) pb, NULL, NULL);
	  }
	  else
	      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			  XmParentTopShadowGC(pb),
			  XmParentBottomShadowGC(pb),
			  pb -> rectangle.x + pb -> gadget.highlight_thickness,
			  pb -> rectangle.y + pb -> gadget.highlight_thickness,
			  pb -> rectangle.width - 2 *
			     pb->gadget.highlight_thickness,
			  pb -> rectangle.height - 2 *
			     pb->gadget.highlight_thickness,
			  pb -> gadget.shadow_thickness,
                          XmSHADOW_OUT);
          }
#else
	  _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			  XmParentTopShadowGC(pb),
			  XmParentBottomShadowGC(pb),
			  pb -> rectangle.x + pb -> gadget.highlight_thickness,
			  pb -> rectangle.y + pb -> gadget.highlight_thickness,
			  pb -> rectangle.width - 2 *
			     pb->gadget.highlight_thickness,
			  pb -> rectangle.height - 2 *
			     pb->gadget.highlight_thickness,
			  pb -> gadget.shadow_thickness,
                          XmSHADOW_OUT);
#endif
	  PBG_Armed(pb) = TRUE;

	  if (PBG_ArmCallback(pb))
	  {
	      XFlush (XtDisplay (pb));

	      call_value.reason = XmCR_ARM;
	      call_value.event = event;
	      XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb),
				  &call_value);
	  }

	  /* So KHelp event is delivered correctly */
	  XtSetKeyboardFocus(XtParent(XtParent(pb)), (Widget)pb);
      }
   }  
   else 
   {
      _XmEnterGadget( (Widget) pb, event, NULL, NULL);
      if (PBG_Armed(pb) == TRUE)
	   (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
			rect_class.expose)) ((Widget) pb, event, (Region) NULL);
   }
}


/************************************************************************
 *
 *  Leave
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Leave( wid, event )
        Widget wid ;
        XEvent *event ;
#else
Leave(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
        XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
       LabG_MenuType(pb) == XmMENU_POPUP)
   {
      if (_XmGetInDragMode((Widget) pb) && PBG_Armed(pb))
      {
#ifdef CDE_VISUAL	/* etched in menu button */
          Boolean etched_in = False;

          XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
	  if (etched_in) {
	      PBG_Armed(pb) = FALSE;
	      Redisplay((Widget) pb, NULL, NULL);
	  }
	  _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                       pb -> rectangle.x + pb -> gadget.highlight_thickness,
                       pb -> rectangle.y + pb -> gadget.highlight_thickness,
                       pb -> rectangle.width - 2 *
                          pb->gadget.highlight_thickness,
                       pb -> rectangle.height - 2 *
                          pb->gadget.highlight_thickness,
                       pb -> gadget.shadow_thickness);
#else
         _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                       pb -> rectangle.x + pb -> gadget.highlight_thickness,
                       pb -> rectangle.y + pb -> gadget.highlight_thickness,
                       pb -> rectangle.width - 2 *
                          pb->gadget.highlight_thickness,
                       pb -> rectangle.height - 2 *
                          pb->gadget.highlight_thickness,
                       pb -> gadget.shadow_thickness);
#endif

	 PBG_Armed(pb) = FALSE;

         if (PBG_DisarmCallback(pb))
         {
	    XFlush (XtDisplay (pb));

	    call_value.reason = XmCR_DISARM;
	    call_value.event = event;
	    XtCallCallbackList ((Widget) pb, PBG_DisarmCallback(pb), &call_value);
         }
      }
   }
   else 
   {
      _XmLeaveGadget( (Widget) pb, event, NULL, NULL);

      if (PBG_Armed(pb) == TRUE)
      {
	 PBG_Armed(pb) = FALSE;
	   (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
			rect_class.expose)) ((Widget) pb, event, (Region) NULL);
	 PBG_Armed(pb) = TRUE;
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
            XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
            XmPushButtonCallbackStruct call_value;
            XEvent * event = NULL;

   if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
       LabG_MenuType(pb) == XmMENU_POPUP)
   {
#ifdef CDE_VISUAL	/* etched in menu button */
      Boolean etched_in = False;
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
      if (etched_in) {
	  PBG_Armed(pb) = TRUE;
	  Redisplay((Widget) pb, NULL, NULL);
      }
      else
	  _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		     XmParentTopShadowGC(pb),
		     XmParentBottomShadowGC(pb),
		     pb -> rectangle.x + pb -> gadget.highlight_thickness,
		     pb -> rectangle.y + pb -> gadget.highlight_thickness,
		     pb -> rectangle.width -
		       2 * pb->gadget.highlight_thickness,
		     pb -> rectangle.height -
		       2 * pb->gadget.highlight_thickness,
		     pb -> gadget.shadow_thickness,
                     XmSHADOW_OUT);
#else
      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		     XmParentTopShadowGC(pb),
		     XmParentBottomShadowGC(pb),
		     pb -> rectangle.x + pb -> gadget.highlight_thickness,
		     pb -> rectangle.y + pb -> gadget.highlight_thickness,
		     pb -> rectangle.width -
		       2 * pb->gadget.highlight_thickness,
		     pb -> rectangle.height -
		       2 * pb->gadget.highlight_thickness,
		     pb -> gadget.shadow_thickness,
                     XmSHADOW_OUT);
#endif
      PBG_Armed(pb) = TRUE;

      if (PBG_ArmCallback(pb))
      {
	 XFlush (XtDisplay (pb));

	 call_value.reason = XmCR_ARM;
	 call_value.event = event;
	 XtCallCallbackList ((Widget) pb, PBG_ArmCallback(pb), &call_value);
      }
   }
   else
   {   DrawBorderHighlight( (Widget) pb) ;
       } 
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
            XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
            register Dimension highlight_width ;

    if(    !pb->rectangle.width  ||  !pb->rectangle.height )
    {   
        return ;
        } 
    pb->gadget.highlighted = True ;
    pb->gadget.highlight_drawn = True ;

    if(    (PBG_DefaultButtonShadowThickness (pb) > 0)    )
    {   
        highlight_width = pb->gadget.highlight_thickness - Xm3D_ENHANCE_PIXEL ;
        } 
    else
    {   highlight_width = pb->gadget.highlight_thickness ;
        } 

    if (highlight_width > 0)
#ifdef CDE_VISUAL	/* default button */
    {
	int x=pb->rectangle.x, y=pb->rectangle.y;
	int width=pb->rectangle.width, height=pb->rectangle.height;
	int default_button_shadow_thickness;
        Boolean default_button = False;

        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);

	if (default_button &&
	    PBG_DefaultButtonShadowThickness (pb)) {
	    if (PBG_Compatible(pb))
		default_button_shadow_thickness = PBG_ShowAsDefault(pb);
	    else    
		default_button_shadow_thickness = 
		    PBG_DefaultButtonShadowThickness (pb);	    
	    x = y = (default_button_shadow_thickness * 2) + Xm3D_ENHANCE_PIXEL;
	    width -= 2 * x;
	    height -= 2 * y;
	    x += pb->rectangle.x;	/* add offset */
	    y += pb->rectangle.y;	/* add offset */
	}
	_XmDrawSimpleHighlight( XtDisplay( pb), XtWindow( pb), 
			       ((XmManagerWidget)
				(pb->object.parent))->manager.highlight_GC,
			       x, y, width,
			       height, highlight_width) ;
    }
#else
	_XmDrawSimpleHighlight( XtDisplay( pb), XtWindow( pb), 
			       ((XmManagerWidget)
				(pb->object.parent))->manager.highlight_GC,
			       pb->rectangle.x, pb->rectangle.y,
			       pb->rectangle.width, pb->rectangle.height, 
			       highlight_width) ;
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
        XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;
   XEvent * event = NULL;

   if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
       LabG_MenuType(pb) == XmMENU_POPUP)
   {
      if (!PBG_Armed(pb))
          return;

#ifdef CDE_VISUAL	/* etched in menu button */
      {
	  Boolean etched_in = False;
	  XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
	  if (etched_in) {
	      PBG_Armed(pb) = FALSE;
	      Redisplay((Widget) pb, NULL, NULL);
	  }
	  _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                    pb -> rectangle.x + pb -> gadget.highlight_thickness,
                    pb -> rectangle.y + pb -> gadget.highlight_thickness,
                    pb -> rectangle.width -
                          2 * pb->gadget.highlight_thickness,
                    pb -> rectangle.height -
                        2 * pb->gadget.highlight_thickness,
                    pb -> gadget.shadow_thickness);
      }
#else
      _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                    pb -> rectangle.x + pb -> gadget.highlight_thickness,
                    pb -> rectangle.y + pb -> gadget.highlight_thickness,
                    pb -> rectangle.width -
                          2 * pb->gadget.highlight_thickness,
                    pb -> rectangle.height -
                        2 * pb->gadget.highlight_thickness,
                    pb -> gadget.shadow_thickness);
#endif

      PBG_Armed(pb) = FALSE;

      if (PBG_DisarmCallback(pb))
      {
	 XFlush (XtDisplay (pb));

	 call_value.reason = XmCR_DISARM;
	 call_value.event = event;
	 XtCallCallbackList ((Widget) pb, PBG_DisarmCallback(pb), &call_value);
      }
   }
   else
   {
#ifdef CDE_VISUAL	/* default button */
       int border = pb->gadget.highlight_thickness -Xm3D_ENHANCE_PIXEL;
       Boolean default_button = False;

       XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);

       if (default_button &&
	   PBG_DefaultButtonShadowThickness (pb) && (border > 0)) {
	   int x, y;
	   int width=pb->rectangle.width,height=pb->rectangle.height;
	   int default_button_shadow_thickness;

	   pb->gadget.highlighted = False;
	   pb->gadget.highlight_drawn = False;
	   if (PBG_Compatible(pb))
	       default_button_shadow_thickness = PBG_ShowAsDefault(pb);
	   else    
	       default_button_shadow_thickness = 
		   PBG_DefaultButtonShadowThickness (pb);	    
	   x = y = (default_button_shadow_thickness * 2) + Xm3D_ENHANCE_PIXEL;
	   width -= 2 * x;
	   height -= 2 * y;
	   x += pb->rectangle.x;	/* add offset */
	   y += pb->rectangle.y;	/* add offset */
	   _XmClearBorder (XtDisplay (pb), XtWindow (pb),
			   x, y, width, height, border);	    
	}
	else
	    (*(xmGadgetClassRec.gadget_class.border_unhighlight))( wid) ;
#else
       (*(xmGadgetClassRec.gadget_class.border_unhighlight))( wid) ;
#endif
       } 
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
KeySelect( wid, event )
        Widget wid ;
        XEvent *event ;
#else
KeySelect(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
        XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmPushButtonCallbackStruct call_value;

   if (!_XmIsEventUnique(event))
      return;

   if (!_XmGetInDragMode((Widget) pb))
   {

      PBG_Armed(pb) = FALSE;

      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);
      }

      _XmRecordEvent(event);

      call_value.reason = XmCR_ACTIVATE;
      call_value.event = event;

      /* if the parent is a RowColumn, notify it about the select */
      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelGadgetClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
							  XtParent(pb),
							  FALSE, pb,
							  &call_value);
      }

      if ((! LabG_SkipCallback(pb)) &&
	  (PBG_ActivateCallback(pb)))
      {
	 XFlush (XtDisplay (pb));
	 XtCallCallbackList ((Widget) pb, PBG_ActivateCallback(pb),
				&call_value);
      }

      if (XmIsRowColumn(XtParent(pb)))
      {
	 (* xmLabelGadgetClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(pb), NULL, event, NULL);
      }
   }
}

/***********************************************************
*
*  ClassInitialize
*
************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  Cardinal                    wc_num_res, sc_num_res;
  XtResource                  *merged_list;
  int                         i, j;
  XtResourceList              uncompiled;
  Cardinal                    num;

/**************************************************************************
   Label's and Pushbutton's resource lists are being merged into one
   and assigned to xmPushButtonGCacheObjClassRec. This is for performance
   reasons, since, instead of two calls to XtGetSubResources() XtGetSubvaluse()
   and XtSetSubvalues() for both the superclass and the widget class, now
   we have just one call with a merged resource list.
   NOTE: At this point the resource lists for Label and Pushbutton do
         have unique entries, but if there are resources in the superclass
         that are being overwritten by the subclass then the merged_lists
         need to be created differently.
****************************************************************************/

  wc_num_res = xmPushButtonGCacheObjClassRec.object_class.num_resources;

  sc_num_res = xmLabelGCacheObjClassRec.object_class.num_resources;

  merged_list = (XtResource *)XtMalloc((sizeof(XtResource) * (wc_num_res +
                                                                 sc_num_res)));

  _XmTransformSubResources(xmLabelGCacheObjClassRec.object_class.resources,
                           sc_num_res, &uncompiled, &num);

  for (i = 0; i < num; i++)
  {

  merged_list[i] = uncompiled[i];

   }
  XtFree((char *)uncompiled);

  for (i = 0, j = num; i < wc_num_res; i++, j++)
  {
   merged_list[j] =
        xmPushButtonGCacheObjClassRec.object_class.resources[i];
  }

  xmPushButtonGCacheObjClassRec.object_class.resources = merged_list;
  xmPushButtonGCacheObjClassRec.object_class.num_resources =
                wc_num_res + sc_num_res ;

  PushBGClassExtensionRec.record_type = XmQmotif;
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
   _XmFastSubclassInit (wc, XmPUSH_BUTTON_GADGET_BIT);
}

/************************************************************************
*
*  SecondaryObjectCreate
*
************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SecondaryObjectCreate( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SecondaryObjectCreate(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
  XmBaseClassExt              *cePtr;
  XmWidgetExtData             extData;
  WidgetClass                 wc;
  Cardinal                    size;
  XtPointer                   newSec, reqSec;

  cePtr = _XmGetBaseClassExtPtr(XtClass(new_w), XmQmotif);
  wc = (*cePtr)->secondaryObjectClass;
  size = wc->core_class.widget_size;

  newSec = _XmExtObjAlloc(size);
  reqSec = _XmExtObjAlloc(size);  

    /*
     * Since the resource lists for label and pushbutton were merged at
     * ClassInitialize time we need to make only one call to 
     * XtGetSubresources()
     */

  XtGetSubresources(new_w,
                    newSec,
                    NULL, NULL,
                    wc->core_class.resources,
                    wc->core_class.num_resources,
                    args, *num_args );


  extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
  extData->widget = (Widget)newSec;
  extData->reqWidget = (Widget)reqSec;

  ((XmPushButtonGCacheObject)newSec)->ext.extensionType = XmCACHE_EXTENSION;
  ((XmPushButtonGCacheObject)newSec)->ext.logicalParent = new_w;

  _XmPushWidgetExtData(new_w, extData,
                         ((XmPushButtonGCacheObject)newSec)->ext.extensionType);
  memcpy(reqSec, newSec, size);

  /*
   * fill out cache pointers
  */
   LabG_Cache(new_w) = &(((XmLabelGCacheObject)extData->widget)->label_cache);
   LabG_Cache(req) = &(((XmLabelGCacheObject)extData->reqWidget)->label_cache);

   PBG_Cache(new_w) =
	  &(((XmPushButtonGCacheObject)extData->widget)->pushbutton_cache);
   PBG_Cache(req) =
	  &(((XmPushButtonGCacheObject)extData->reqWidget)->pushbutton_cache);

}

/************************************************************************
 *
 *  InitializePosthook
 *
 ************************************************************************/
/* ARGSUSED */
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
    XmWidgetExtData     ext;
    XmPushButtonGadget  pbw = (XmPushButtonGadget)new_w;

    /*
    * - register parts in cache.
    * - update cache pointers
    * - and free req
    */

    LabG_Cache(pbw) = (XmLabelGCacheObjPart *)
      _XmCachePart( LabG_ClassCachePart(pbw),
                    (XtPointer) LabG_Cache(pbw),
                    sizeof(XmLabelGCacheObjPart));

    PBG_Cache(pbw) = (XmPushButtonGCacheObjPart *)
	   _XmCachePart( PBG_ClassCachePart(pbw),
			 (XtPointer) PBG_Cache(pbw),
			 sizeof(XmPushButtonGCacheObjPart));

   /*
    * might want to break up into per-class work that gets explicitly
    * chained. For right now, each class has to replicate all
    * superclass logic in hook routine
    */
   
   /*
    * free the req subobject used for comparisons
    */
    _XmPopWidgetExtData((Widget) pbw, &ext, XmCACHE_EXTENSION);
    _XmExtObjFree((XtPointer)ext->widget);
    _XmExtObjFree((XtPointer)ext->reqWidget);
    XtFree( (char *) ext);

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
   XmPushButtonGadget request = (XmPushButtonGadget) rw ;
   XmPushButtonGadget new_w = (XmPushButtonGadget) nw ;
   XmGadgetPart  *pbgadget;
   int increase;
   int adjustment = 0;

   if (PBG_MultiClick(new_w) == XmINVALID_MULTICLICK)
   {
      if (LabG_MenuType(new_w) == XmMENU_POPUP ||
          LabG_MenuType(new_w) == XmMENU_PULLDOWN)
         PBG_MultiClick(new_w) = XmMULTICLICK_DISCARD;
      else
         PBG_MultiClick(new_w) = XmMULTICLICK_KEEP;
   }

    /* if menuProcs is not set up yet, try again */

    if (xmLabelGadgetClassRec.label_class.menuProcs == (XmMenuProc)NULL)
	xmLabelGadgetClassRec.label_class.menuProcs =
	    (XmMenuProc) _XmGetMenuProcContext();


   PBG_Armed(new_w) = FALSE;
   PBG_Timer(new_w) = 0;


/*
 * Fix to introduce Resource XmNdefaultBorderWidth and compatibility
 *	variable.
 *  if defaultBorderWidth > 0, the program knows about this resource
 *	and is therefore a Motif 1.1 program; otherwise it is a Motif 1.0
 *      program and old semantics of XmNshowAsDefault prevails.
 *  - Sankar 2/1/90.
 */


   if (PBG_DefaultButtonShadowThickness(new_w) > 0)
      PBG_Compatible (new_w) = False;
   else
      PBG_Compatible (new_w) = True; 

   if ( PBG_Compatible (new_w)) 
	 PBG_DefaultButtonShadowThickness(new_w) = PBG_ShowAsDefault(new_w);

   /* no unarm_pixmap but do have an arm_pixmap, use that */
   if ((LabG_Pixmap(new_w) == XmUNSPECIFIED_PIXMAP) &&
       (PBG_ArmPixmap(new_w) != XmUNSPECIFIED_PIXMAP))
   {
      LabG_Pixmap(new_w) = PBG_ArmPixmap(new_w);
      if (request->rectangle.width == 0)
         new_w->rectangle.width = 0;
      if (request->rectangle.height == 0)
         new_w->rectangle.height = 0;

      _XmCalcLabelGDimensions((Widget) new_w);
      (* xmLabelGadgetClassRec.rect_class.resize) ((Widget) new_w);
   }

   if (PBG_ArmPixmap(new_w) != XmUNSPECIFIED_PIXMAP)
   {
       if (request->rectangle.width == 0)
         new_w->rectangle.width = 0;
       if (request->rectangle.height == 0)
         new_w->rectangle.height = 0;
       SetPushButtonSize(new_w);
   }

   PBG_UnarmPixmap(new_w) = LabG_Pixmap(new_w);


   if (PBG_DefaultButtonShadowThickness(new_w))
   { 
     /*
      * Special hack for 3d enhancement of location cursor high light.
      *  - Make the box bigger . During drawing of location cursor
      *    make it smaller.  See in Primitive.c
      *  May be we should use the macro: G_HighLightThickness(pbgadget);
      */

     pbgadget = (XmGadgetPart *) (&(new_w->gadget));
     pbgadget->highlight_thickness += Xm3D_ENHANCE_PIXEL;
     adjustment += Xm3D_ENHANCE_PIXEL;
     
     increase =  2 * PBG_DefaultButtonShadowThickness(new_w) +
       new_w->gadget.shadow_thickness;
     
     increase += adjustment ;
     
     /* Add the increase to the rectangle to compensate for extra space */
     if (increase != 0)
       {
	 LabG_MarginLeft(new_w) += increase;
	 LabG_MarginRight(new_w) += increase;
	 LabG_TextRect_x(new_w) += increase;
	 new_w->rectangle.width += (increase  << 1);
	 
	 LabG_MarginTop(new_w) += increase;
	 LabG_MarginBottom(new_w) += increase;
	 LabG_TextRect_y(new_w) += increase ;
	 new_w->rectangle.height +=  (increase  << 1);
       }
   }

   if (LabG_MenuType(new_w)  == XmMENU_POPUP ||
       LabG_MenuType(new_w)  == XmMENU_PULLDOWN)
   {
      new_w->gadget.traversal_on = TRUE;
   }

   /* Get the background fill GC */

#ifndef CDE_VISUAL	/* etched in menu button */
   if (LabG_MenuType(new_w) != XmMENU_PULLDOWN &&
       LabG_MenuType(new_w) != XmMENU_POPUP)
#endif
   {
      GetFillGC (new_w);
      GetBackgroundGC (new_w);
   }
   
   /*  Initialize the interesting input types.  */

   new_w->gadget.event_mask = XmARM_EVENT | XmACTIVATE_EVENT | XmHELP_EVENT |
        XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT | XmENTER_EVENT | XmLEAVE_EVENT
        | XmMULTI_ARM_EVENT |  XmMULTI_ACTIVATE_EVENT | XmBDRAG_EVENT;
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
        XmPushButtonGadget pb ;
#else
GetFillGC(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;
   XmManagerWidget mw = (XmManagerWidget) XtParent(pb);
   
   valueMask = GCForeground | GCBackground | GCFillStyle;

   values.foreground = PBG_ArmColor(pb);
   values.background = mw -> core.background_pixel;
   values.fill_style = FillSolid;

   PBG_FillGc(pb) = XtGetGC ((Widget) mw, valueMask, &values);
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
        XmPushButtonGadget pb ;
#else
GetBackgroundGC(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{
   XGCValues       values;
   XtGCMask        valueMask;
   short             myindex;
   XFontStruct     *fs;
   XmManagerWidget mw = (XmManagerWidget) XtParent(pb);


   valueMask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;

   _XmFontListSearch(LabG_Font(pb),
		     XmFONTLIST_DEFAULT_TAG,
		     &myindex ,
		     &fs); 
   values.foreground = mw->core.background_pixel;
   values.background = mw->manager.foreground;
   values.graphics_exposures = False;
   if (fs==NULL)
     valueMask &= ~GCFont;
   else
     values.font     = fs->fid;

   PBG_BackgroundGc(pb) = XtGetGC( (Widget) mw,valueMask,&values);
}


/************************************************************************
 *
 *  VisualChange
 *	This function is called from XmManagerClass set values when
 *	the managers visuals have changed.  The gadget regenerates any
 *	GC based on the visual changes and returns True indicating a
 *	redraw is needed.  Otherwise, False is returned.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
VisualChange( wid, cmw, nmw )
        Widget wid ;
        Widget cmw ;
        Widget nmw ;
#else
VisualChange(
        Widget wid,
        Widget cmw,
        Widget nmw )
#endif /* _NO_PROTO */
{
        XmGadget gw = (XmGadget) wid ;
        XmManagerWidget curmw = (XmManagerWidget) cmw ;
        XmManagerWidget newmw = (XmManagerWidget) nmw ;
        XmPushButtonGCacheObjPart  oldCopy;
   XmPushButtonGadget pbg = (XmPushButtonGadget) gw;
   XmManagerWidget mw = (XmManagerWidget) XtParent(gw);

   /*  See if the GC need to be regenerated and widget redrawn.  */
/* BEGIN OSF Fix pir 2746 */
   if (curmw -> core.background_pixel != newmw -> core.background_pixel 
#ifndef CDE_VISUAL	/* etched in menu button */
       && LabG_MenuType(pbg) != XmMENU_PULLDOWN &&
      LabG_MenuType(pbg) != XmMENU_POPUP
#endif
       )
/* END OSF Fix pir 2746 */
   {
      XtReleaseGC ((Widget) mw, PBG_FillGc(pbg));
      XtReleaseGC ((Widget) mw, PBG_BackgroundGc(pbg));

     /* Since GC's are cached we need make the following function calls */
     /* to update the cache correctly. */

      _XmCacheCopy((XtPointer) PBG_Cache(pbg), &oldCopy, sizeof(XmPushButtonGCacheObjPart));
      _XmCacheDelete ((XtPointer) PBG_Cache(pbg));
      PBG_Cache(pbg) = &oldCopy;

      GetFillGC (pbg);	
      GetBackgroundGC(pbg);

      PBG_Cache(pbg) = (XmPushButtonGCacheObjPart *)
      _XmCachePart(PBG_ClassCachePart(pbg),
                      (XtPointer) PBG_Cache(pbg),
                      sizeof(XmPushButtonGCacheObjPart));
      return (True);
   }
   return (False);
}





/************************************************************************
 *
 *  SetValuesPrehook
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValuesPrehook( oldParent, refParent, newParent, args, num_args )
        Widget oldParent ;
        Widget refParent ;
        Widget newParent ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesPrehook(
        Widget oldParent,
        Widget refParent,
        Widget newParent,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{   XmWidgetExtData             extData;
    XmBaseClassExt              *cePtr;
    WidgetClass                 ec;
    Cardinal                    size;
    XmPushButtonGCacheObject    newSec, reqSec;
    
    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmPushButtonGCacheObject)_XmExtObjAlloc(size);
    reqSec = (XmPushButtonGCacheObject)_XmExtObjAlloc(size);

    newSec->object.self = (Widget)newSec;
    newSec->object.widget_class = ec;
    newSec->object.parent = XtParent(newParent);
    newSec->object.xrm_name = newParent->core.xrm_name;
    newSec->object.being_destroyed = False;
    newSec->object.destroy_callbacks = NULL;
    newSec->object.constraints = NULL;

    newSec->ext.logicalParent = newParent;
    newSec->ext.extensionType = XmCACHE_EXTENSION;

    memcpy( &(newSec->label_cache), 
            LabG_Cache(newParent), 
	    sizeof(XmLabelGCacheObjPart));

    memcpy( &(newSec->pushbutton_cache), 
            PBG_Cache(newParent), 
            sizeof(XmPushButtonGCacheObjPart));

    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    extData->reqWidget = (Widget)reqSec;    
    _XmPushWidgetExtData(newParent, extData, XmCACHE_EXTENSION);

    /*
     * Since the resource lists for label and pushbutton were merged at
     * ClassInitialize time we need to make only one call to
     * XtSetSubvalues()
     */

    XtSetSubvalues((XtPointer)newSec,
                    ec->core_class.resources,
                    ec->core_class.num_resources,
                    args, *num_args);

       
    memcpy((XtPointer)reqSec, (XtPointer)newSec, size);    

    LabG_Cache(newParent) = &(((XmLabelGCacheObject)newSec)->label_cache);
    LabG_Cache(refParent) = &(((XmLabelGCacheObject)extData->reqWidget)->label_cache);

    PBG_Cache(newParent) =
 	 &(((XmPushButtonGCacheObject)newSec)->pushbutton_cache);
    PBG_Cache(refParent) =
	 &(((XmPushButtonGCacheObject)extData->reqWidget)->pushbutton_cache);

    _XmExtImportArgs((Widget)newSec, args, num_args);

    return FALSE;
}

/************************************************************************
 *
 *  GetValuesPrehook
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetValuesPrehook( newParent, args, num_args )
        Widget newParent ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesPrehook(
        Widget newParent,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{   
    XmWidgetExtData             extData;
    XmBaseClassExt              *cePtr;
    WidgetClass                 ec;
    XmPushButtonGCacheObject    newSec;
    Cardinal                    size;

    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmPushButtonGCacheObject)_XmExtObjAlloc(size);

    newSec->object.self = (Widget)newSec;
    newSec->object.widget_class = ec;
    newSec->object.parent = XtParent(newParent);
    newSec->object.xrm_name = newParent->core.xrm_name;
    newSec->object.being_destroyed = False;
    newSec->object.destroy_callbacks = NULL;
    newSec->object.constraints = NULL;

    newSec->ext.logicalParent = newParent;
    newSec->ext.extensionType = XmCACHE_EXTENSION;

    memcpy( &(newSec->label_cache),
            LabG_Cache(newParent), 
	    sizeof(XmLabelGCacheObjPart));

    memcpy( &(newSec->pushbutton_cache),
            PBG_Cache(newParent), 
            sizeof(XmPushButtonGCacheObjPart));

    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    _XmPushWidgetExtData(newParent, extData, XmCACHE_EXTENSION);

/* Note that if a resource is defined in the superclass's as well as a
   subclass's resource list and if a NULL is passed in as the third
   argument to XtSetArg, then when a GetSubValues() is done by the
   superclass the NULL is replaced by a value. Now when the subclass
   gets the arglist it doesn't see a NULL and thinks it's an address
   it needs to stuff a value into and sure enough it breaks. 
   This means that we have to pass the same arglist with the NULL to
   both the superclass and subclass and propagate the values up once
   the XtGetSubValues() are done.*/

    /*
     * Since the resource lists for label and pushbutton were merged at
     * ClassInitialize time we need to make only one call to
     * XtGetSubvalues()
     */

    XtGetSubvalues((XtPointer)newSec,
                   ec->core_class.resources,
                   ec->core_class.num_resources,
                   args, *num_args);
   
    _XmExtGetValuesHook((Widget)newSec, args, num_args);
}

/************************************************************************
 *
 *  GetValuesPosthook
 *
 ************************************************************************/
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
GetValuesPosthook( new_w, args, num_args )
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesPosthook(
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
     XmWidgetExtData             ext;

     _XmPopWidgetExtData(new_w, &ext, XmCACHE_EXTENSION);

     _XmExtObjFree((XtPointer)ext->widget);
     XtFree( (char *) ext);
}


/************************************************************************
 *
 *  SetValuesPosthook
 *
 ************************************************************************/
/*ARGSUSED*/
static Boolean 
#ifdef _NO_PROTO
SetValuesPosthook( current, req, new_w, args, num_args )
        Widget current ;
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesPosthook(
        Widget current,
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{  XmWidgetExtData                  ext;

  /*
   * - register parts in cache.
   * - update cache pointers
   * - and free req
   */
  /* assign if changed! */
  if (!_XmLabelCacheCompare((XtPointer)LabG_Cache(new_w), 
                            (XtPointer)LabG_Cache(current)))
  {
       _XmCacheDelete( (XtPointer) LabG_Cache(current));  /* delete the old one */
       LabG_Cache(new_w) = (XmLabelGCacheObjPart *)
                         _XmCachePart(LabG_ClassCachePart(new_w), (XtPointer) LabG_Cache(new_w),
                              sizeof(XmLabelGCacheObjPart));
  }
  else
       LabG_Cache(new_w) = LabG_Cache(current);


  /* assign if changed! */
  if (!_XmPushBCacheCompare((XtPointer)PBG_Cache(new_w),
		            (XtPointer)PBG_Cache(current)))
  {
      _XmCacheDelete( (XtPointer) PBG_Cache(current));  /* delete the old one */
      PBG_Cache(new_w) = (XmPushButtonGCacheObjPart *)
           	     _XmCachePart(PBG_ClassCachePart(new_w),
	                         (XtPointer) PBG_Cache(new_w),
				 sizeof(XmPushButtonGCacheObjPart));
  }
  else
       PBG_Cache(new_w) = PBG_Cache(current);

  _XmPopWidgetExtData(new_w, &ext, XmCACHE_EXTENSION);

  _XmExtObjFree((XtPointer)ext->widget);
  _XmExtObjFree((XtPointer)ext->reqWidget);

  XtFree( (char *) ext);

  return FALSE;
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
/************************************
   XmGadgetPart  *pbgadget;
************************************/
        XmPushButtonGadget current = (XmPushButtonGadget) cw ;
        XmPushButtonGadget request = (XmPushButtonGadget) rw ;
        XmPushButtonGadget new_w = (XmPushButtonGadget) nw ;
   int increase;
   Boolean  flag = FALSE;    /* our return value */
   XmManagerWidget curmw = (XmManagerWidget) XtParent(current);
   XmManagerWidget newmw = (XmManagerWidget) XtParent(new_w);
	int adjustment;

/*
 * Fix to introduce Resource XmNdefaultBorderWidth and compatibility
 *      variable.
 *  if  defaultBorderWidth of the current and new are different, then
 *  the programmer is setting the resource XmNdefaultBorderWidth; i.e. it
 *  defaultBorderWidth > 0, the program knows about this resource
 *  a Motif 1.1 program; otherwise it is a Motif 1.0
 *      program and old semantics of XmNshowAsDefault prevails.
 *     Note if (PBG_ShowAsDefault(gadget) == 0 ) then we are NOT currently 
 *      drawing defaultBorderWidth; if it is > 0, we should be drawing
 *	    the shadow in defaultorderWidth; 
 *  - Sankar 2/1/90.
 */


   if ( PBG_DefaultButtonShadowThickness(new_w) !=
                     PBG_DefaultButtonShadowThickness(current))

     PBG_Compatible (new_w) = False;

   if ( PBG_Compatible (new_w))
         PBG_DefaultButtonShadowThickness(new_w) = PBG_ShowAsDefault(new_w);

   adjustment = AdjustHighLightThickness (new_w, current);

   if (PBG_DefaultButtonShadowThickness(new_w) !=
	        PBG_DefaultButtonShadowThickness(current))
   {

/************** not referenced ****************************
     pbgadget = (XmGadgetPart *) (&(new_w->gadget));
***********************************************************/
      if (PBG_DefaultButtonShadowThickness(new_w) >
		PBG_DefaultButtonShadowThickness(current))
      {
         if (PBG_DefaultButtonShadowThickness(current) > 0)
            increase =  (2 * PBG_DefaultButtonShadowThickness(new_w) +
                         new_w->gadget.shadow_thickness) -
                        (2 * PBG_DefaultButtonShadowThickness(current) +
                         current->gadget.shadow_thickness);
         else
            increase =  (2 * PBG_DefaultButtonShadowThickness(new_w) +
                         new_w->gadget.shadow_thickness);
      }
      else
      {
         if (PBG_DefaultButtonShadowThickness(new_w) > 0)
            increase = - ((2 * PBG_DefaultButtonShadowThickness(current) +
                           current->gadget.shadow_thickness) -
                          (2 * PBG_DefaultButtonShadowThickness(new_w) +
                           new_w->gadget.shadow_thickness));
         else
            increase = - (2 * PBG_DefaultButtonShadowThickness(current) +
                          current->gadget.shadow_thickness);
      }

      increase += adjustment;

      if (LabG_RecomputeSize(new_w) || request->rectangle.width == 0)
      {
         LabG_MarginLeft(new_w) += increase;
         LabG_MarginRight(new_w) += increase;
         new_w->rectangle.width += (increase << 1) ;
         flag = TRUE;
      }
      else
      /* add the change to the rectangle */
      if (increase != 0)
       {  
          LabG_MarginLeft(new_w) += increase;
          LabG_MarginRight(new_w) += increase;
          new_w->rectangle.width += (increase << 1);
          flag = TRUE;
       }

      if (LabG_RecomputeSize(new_w) || request->rectangle.height == 0)
      {
         LabG_MarginTop(new_w) += increase;
         LabG_MarginBottom(new_w) += increase ;
         new_w->rectangle.height +=  (increase << 1);
         flag = TRUE;
      }
    else
      /* add the change to the rectangle */
      if (increase != 0)
      { 
        LabG_MarginTop(new_w)  += increase;
        LabG_MarginBottom(new_w) += increase;
        new_w->rectangle.height += (increase << 1);
        flag = TRUE;
      }

      _XmReCacheLabG((Widget) new_w);
   }

   if ((PBG_ArmPixmap(new_w) != PBG_ArmPixmap(current)) &&
      (LabG_LabelType(new_w) == XmPIXMAP) && (PBG_Armed(new_w))) 
      flag = TRUE;
      
   /* no unarm_pixmap but do have an arm_pixmap, use that */
   if ((LabG_Pixmap(new_w) == XmUNSPECIFIED_PIXMAP) &&
       (PBG_ArmPixmap(new_w) != XmUNSPECIFIED_PIXMAP))
   {
      LabG_Pixmap(new_w) = PBG_ArmPixmap(new_w);
      if (LabG_RecomputeSize(new_w) &&
          request->rectangle.width == current->rectangle.width)
         new_w->rectangle.width = 0;
      if (LabG_RecomputeSize(new_w) &&
          request->rectangle.height == current->rectangle.height)
         new_w->rectangle.width = 0;

      _XmCalcLabelGDimensions((Widget) new_w);
      (* xmLabelGadgetClassRec.rect_class.resize) ((Widget) new_w);
   }

   if (LabG_Pixmap(new_w) != LabG_Pixmap(current))
   {
      PBG_UnarmPixmap(new_w) = LabG_Pixmap(new_w);
      if ((LabG_LabelType(new_w) == XmPIXMAP) && (!PBG_Armed(new_w)))
	 flag = TRUE;
   }
   if ((LabG_LabelType(new_w) == XmPIXMAP) &&
       (PBG_ArmPixmap(new_w) != PBG_ArmPixmap(current)))
   {
       if ((LabG_RecomputeSize(new_w)))
       {
          if (request->rectangle.width == current->rectangle.width)
             new_w->rectangle.width = 0;
          if (request->rectangle.height == current->rectangle.height)
             new_w->rectangle.height = 0;
       }
       SetPushButtonSize(new_w);
       flag = TRUE;
   }

   if ((PBG_FillOnArm(new_w) != PBG_FillOnArm(current)) &&
       (PBG_Armed(new_w) == TRUE))
	 flag = TRUE;

#ifndef CDE_VISUAL	/* etched in menu button */
   if (LabG_MenuType(new_w) != XmMENU_PULLDOWN &&
       LabG_MenuType(new_w) != XmMENU_POPUP)
#endif
   {
      /*  See if the GC need to be regenerated and widget redrawn.  */

      if (PBG_ArmColor(new_w) != PBG_ArmColor(current))
      {
	 if (PBG_Armed(new_w)) flag = TRUE;  /* see PIR 5091 */
	 XtReleaseGC ((Widget) newmw, PBG_FillGc(new_w));
	 GetFillGC (new_w);
      }

      if (newmw -> core.background_pixel != curmw -> core.background_pixel) 
      {
	flag = TRUE;
	XtReleaseGC ((Widget) newmw, PBG_BackgroundGc(new_w));
	GetBackgroundGC (new_w);
      }
   }
   
   /*  Initialize the interesting input types.  */

   new_w->gadget.event_mask = XmARM_EVENT | XmACTIVATE_EVENT | XmHELP_EVENT |
        XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT | XmENTER_EVENT | XmLEAVE_EVENT
         | XmMULTI_ARM_EVENT | XmMULTI_ACTIVATE_EVENT | XmBDRAG_EVENT;

/* BEGIN OSF Fix pir 3469 */
    if (flag == False && XtIsRealized((Widget) new_w))
/* END OSF Fix pir 3469 */
        /* No size change has taken place. */
    {

        if ( (PBG_ShowAsDefault(current) != 0) &&
             (PBG_ShowAsDefault(new_w) == 0) )
                 EraseDefaultButtonShadow (new_w);

        if ( (PBG_ShowAsDefault(current) == 0) &&
             (PBG_ShowAsDefault(new_w) != 0) )
                 DrawDefaultButtonShadow (new_w);
    }

   return(flag);
}

/************************************************************************
 *
 *  Help
 *     This function processes Function Key 1 press occuring on the PushButton.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( pb, event )
        XmPushButtonGadget pb ;
        XEvent *event ;
#else
Help(
        XmPushButtonGadget pb,
        XEvent *event )
#endif /* _NO_PROTO */
{
   Boolean is_menupane = (LabG_MenuType(pb) == XmMENU_PULLDOWN) ||
			 (LabG_MenuType(pb) == XmMENU_POPUP);

   if (is_menupane)
   {
      (* xmLabelGadgetClassRec.label_class.menuProcs)
	  (XmMENU_BUTTON_POPDOWN, XtParent(pb), NULL, event, NULL);
   }

   _XmSocorro( (Widget) pb, event, NULL, NULL);

   if (is_menupane)
   {
      (* xmLabelGadgetClassRec.label_class.menuProcs)
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
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmPushButtonGadget pb = (XmPushButtonGadget) wid ;
   XmManagerWidget mw = (XmManagerWidget) XtParent(pb);

   if (PBG_Timer(pb))
      XtRemoveTimeOut (PBG_Timer(pb));

/* BEGIN OSF Fix pir 2746 */
#ifndef CDE_VISUAL	/* etched in menu button */
   if (LabG_MenuType(pb) != XmMENU_PULLDOWN &&
       LabG_MenuType(pb) != XmMENU_POPUP)
#endif
   {
     XtReleaseGC ((Widget) mw, PBG_FillGc(pb));
     XtReleaseGC ((Widget) mw, PBG_BackgroundGc(pb));
   }
/* END OSF Fix pir 2746 */
   XtRemoveAllCallbacks ((Widget) pb, XmNactivateCallback);
   XtRemoveAllCallbacks ((Widget) pb, XmNarmCallback);
   XtRemoveAllCallbacks ((Widget) pb, XmNdisarmCallback);

   _XmCacheDelete( (XtPointer) PBG_Cache(pb));
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
  register XmPushButtonGadget pb = (XmPushButtonGadget) w;

  if (LabG_IsPixmap(w)) 
    SetPushButtonSize(pb);
  else
    (* xmLabelGadgetClassRec.rect_class.resize)( (Widget) pb);

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
XmCreatePushButtonGadget( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreatePushButtonGadget(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmPushButtonGadgetClass, 
                           parent, arglist, argcount));
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ActivateCommonG( pb, event, event_mask )
        XmPushButtonGadget pb ;
        XEvent *event ;
        Mask event_mask ;
#else
ActivateCommonG(
        XmPushButtonGadget pb,
        XEvent *event,
        Mask event_mask )
#endif /* _NO_PROTO */
{
      if ( ( LabG_MenuType(pb) == XmMENU_PULLDOWN) ||
              ( LabG_MenuType(pb) == XmMENU_POPUP))
         if (event->type == ButtonRelease)
            BtnUp ((Widget) pb, event);
         else  /* assume KeyRelease */
            KeySelect ((Widget) pb, event);
      else
      {
         if (event->type == ButtonRelease)
         {
            Activate (pb, event);
            Disarm (pb, event);
         }
         else  /* assume KeyPress or KeyRelease */
        (* (((XmPushButtonGadgetClassRec *)(pb->object.widget_class))->
			gadget_class.arm_and_activate))
			((Widget) pb, event, NULL, NULL);
      }
}
/****************************************************
 *   Functions for manipulating Secondary Resources.
 *********************************************************/
/*
 * GetPushBGSecResData()
 *    Create a XmSecondaryResourceDataRec for each secondary resource;
 *    Put the pointers to these records in an array of pointers;
 *    Return the pointer to the array of pointers.
 */
/*ARGSUSED*/
static Cardinal 
#ifdef _NO_PROTO
GetPushBGClassSecResData( w_class, data_rtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **data_rtn ;
#else
GetPushBGClassSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **data_rtn )
#endif /* _NO_PROTO */
{   int arrayCount;
    XmBaseClassExt  bcePtr;
    String  resource_class, resource_name;
    XtPointer  client_data;

    bcePtr = &(PushBGClassExtensionRec );
    client_data = NULL;
    resource_class = NULL;
    resource_name = NULL;
    arrayCount =
      _XmSecondaryResourceData ( bcePtr, data_rtn, client_data,  
				resource_name, resource_class,
				GetPushBGClassSecResBase) ;

    return (arrayCount);
}

/*
 * GetPushBGClassResBase ()
 *   retrun the address of the base of resources.
 */
static XtPointer 
#ifdef _NO_PROTO
GetPushBGClassSecResBase( widget, client_data )
        Widget widget ;
        XtPointer client_data ;
#else
GetPushBGClassSecResBase(
        Widget widget,
        XtPointer client_data )
#endif /* _NO_PROTO */
{	XtPointer  widgetSecdataPtr; 
    size_t  labg_cache_size = sizeof (XmLabelGCacheObjPart);
    size_t  pushbg_cache_size = sizeof (XmPushButtonGCacheObjPart);
	char *cp;

    widgetSecdataPtr = (XtPointer) 
			(XtMalloc ( labg_cache_size + pushbg_cache_size + 1));

    if (widgetSecdataPtr)
	  { cp = (char *) widgetSecdataPtr;
        memcpy( cp, LabG_Cache(widget), labg_cache_size);
	    cp += labg_cache_size;
	    memcpy( cp, PBG_Cache(widget), pushbg_cache_size);
	  }
/* else Warning: error cannot allocate Memory */
/*     widgetSecdataPtr = (XtPointer) ( LabG_Cache(widget)); */

	return (widgetSecdataPtr);
}


/*
 * EraseDefaultButtonShadow (pb)
 *  - Called from SetValues() - effort to optimize shadow drawing.
 */
static void 
#ifdef _NO_PROTO
EraseDefaultButtonShadow( pb )
        XmPushButtonGadget pb ;
#else
EraseDefaultButtonShadow(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{  int done = False;
   int size, x, y, width, height;

   if (!(XtIsRealized(pb)) ) done = True;   

   /* No need to undraw anything if the button is unmanaged, this can 
      result in corrupt graphics elsewhere in the Manager since the gadget
      field might be uninitialized */

   if (!(XtIsManaged (pb)) ) done = True;  

      if (!(done))
      {
         if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
                     LabG_MenuType(pb) == XmMENU_POPUP)
          {
             ShellWidget mshell = (ShellWidget)XtParent(XtParent(pb));
             if (!mshell->shell.popped_up) done = True;
          }
      }

      if (!(done))
        {   size = (int) (PBG_DefaultButtonShadowThickness(pb));
            x = (int) (pb -> rectangle.x ) + 
						(int) (pb -> gadget.highlight_thickness) ;
        y =  (int) (pb -> rectangle.y) + 
					(int) (pb -> gadget.highlight_thickness);
        width = (int) ( pb -> rectangle.width) - 2 *
                           ( (int) pb->gadget.highlight_thickness);
            height = (int) (pb -> rectangle.height) - 
						2 * ( (int) (pb->gadget.highlight_thickness));

#ifdef CDE_VISUAL	/* default button */
        {
        Boolean default_button = False;

        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);
	    if (default_button) {
		x = y = Xm3D_ENHANCE_PIXEL;
		width = pb->rectangle.width - 2 * x;
		height = pb->rectangle.height - 2 * y;
		x += pb->rectangle.x;
		y += pb->rectangle.y;
	    }
        }
#endif
           _XmClearBorder (XtDisplay (pb), XtWindow (pb),
                           x, y, width, height, size);
        }
}
/*
 * DrawDefaultButtonShadow (pb)
 *  - Called from SetValues() - effort to optimize shadow drawing.
 */
static void 
#ifdef _NO_PROTO
DrawDefaultButtonShadow( pb )
        XmPushButtonGadget pb ;
#else
DrawDefaultButtonShadow(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{  
   int done = False;
   int size, x, y, width, height;

   if (!(XtIsRealized(pb))) 
      done = True;  

   if (!(done))
   {
      if (LabG_MenuType(pb) == XmMENU_PULLDOWN || 
          LabG_MenuType(pb) == XmMENU_POPUP)
      {
	 ShellWidget mshell = (ShellWidget)XtParent(XtParent(pb));
	 if (!mshell->shell.popped_up) done = True;
      }
   }

   if (!(done))
   {   
      size = (int) (PBG_DefaultButtonShadowThickness(pb));
      x = (int) (pb -> rectangle.x + pb -> gadget.highlight_thickness);
      y = (int) (pb -> rectangle.y + pb -> gadget.highlight_thickness);
      width =  (int)( pb -> rectangle.width - 
                    ( pb->gadget.highlight_thickness << 1));
      height = (int) (pb -> rectangle.height -
                    (pb->gadget.highlight_thickness << 1));

#ifdef CDE_VISUAL	/* default button */
      {
      Boolean default_button = False;

      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);
      if (default_button) {
	  x = y = Xm3D_ENHANCE_PIXEL;
	  width = pb->rectangle.width - 2 * x;
	  height = pb->rectangle.height - 2 * y;
	  x += pb->rectangle.x;
	  y += pb->rectangle.y;
      }
      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		      XmParentBottomShadowGC(pb),
                      default_button ?
		       XmParentBottomShadowGC(pb) : XmParentTopShadowGC(pb),
		      x, y, width, height, size, XmSHADOW_OUT);
      }
#else
      _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
		       XmParentBottomShadowGC(pb),
		       XmParentTopShadowGC(pb),
		       x, y, width, height, size, XmSHADOW_OUT);
#endif
   }
}

void 
#ifdef _NO_PROTO
_XmClearBGCompatibility( pbg )
        Widget pbg ;
#else
_XmClearBGCompatibility(
        Widget pbg )
#endif /* _NO_PROTO */
{
	 PBG_Compatible (pbg) = False;
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
        XmPushButtonGadget new_w ;
        XmPushButtonGadget current ;
#else
AdjustHighLightThickness(
        XmPushButtonGadget new_w,
        XmPushButtonGadget current )
#endif /* _NO_PROTO */
{	XmGadgetPart  *pbnew, *pbcurrent;
	int adjustment = 0;

	pbnew = (XmGadgetPart *) (&(new_w->gadget));
	pbcurrent = (XmGadgetPart *) (&(current->gadget));

	if (PBG_DefaultButtonShadowThickness(new_w) )
	{  if ( !(PBG_DefaultButtonShadowThickness(current)))
		 { pbnew->highlight_thickness += Xm3D_ENHANCE_PIXEL;
	       adjustment += Xm3D_ENHANCE_PIXEL;
		 }
	   else
	      if (pbnew->highlight_thickness !=
			pbcurrent->highlight_thickness)
		     {  pbnew->highlight_thickness += Xm3D_ENHANCE_PIXEL;
	     	    adjustment += Xm3D_ENHANCE_PIXEL;
			 }
	}
	else
	 { if (PBG_DefaultButtonShadowThickness(current))
		/* default_button_shadow_thickness was > 0 and is now
		 * being set to 0. 
		 * - so take away the adjustment for enhancement.
		 */
	     { if ( pbnew->highlight_thickness ==
			pbcurrent->highlight_thickness)
		       { pbnew->highlight_thickness -= Xm3D_ENHANCE_PIXEL;
				 adjustment -= Xm3D_ENHANCE_PIXEL;
			   }
	     }
		/*
	         * This will have a bug if in a XtSetValues the application
		 * removes the default_button_shadow_thickness and also
		 * sets the high-light-thickness to a value of
 		 * (old-high-light-thickness (from previous XtSetValue) +
		 *  Xm3D_ENHANCE_PIXEL).
		 * This will be documented.
		 */
	   }
    return (adjustment);
}

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
   XmPushButtonGadget pb = (XmPushButtonGadget) wid ;

   if (XtIsRealized(pb))
   {
      if (LabG_MenuType(pb) == XmMENU_PULLDOWN ||
          LabG_MenuType(pb) == XmMENU_POPUP)
      {
         ShellWidget mshell = (ShellWidget)XtParent(XtParent(pb));

         if (!mshell->shell.popped_up)
        	 return;

#ifdef CDE_VISUAL	/* etched in menu button */
     {
	 Boolean etched_in = False;
	 XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget)pb)), "enableEtchedInMenu", &etched_in, NULL);
	 if (etched_in) {
	     DrawPushBGBackground (pb);
	     DrawPushButtonLabelGadget (pb, event, region);
	     if (PBG_Armed(pb))
		 DrawPushButtonGadgetShadows (pb);
	 }
	 else
	     DrawPushButtonLabelGadget (pb, event, region);
     }
#else
	 DrawPushButtonLabelGadget (pb, event, region);
#endif
      }
      else
      {
	 DrawPushBGBackground (pb);
	 DrawPushButtonLabelGadget (pb, event, region);
	 DrawPushButtonGadgetShadows (pb);
      
	 if (pb -> gadget.highlighted)
	 {   
	    DrawBorderHighlight( (Widget) pb) ;
	 } 
      }
   }
}
/*
 * DrawPushButtonLabelGadget()
 */
static void 
#ifdef _NO_PROTO
DrawPushButtonLabelGadget( pb, event, region )
        XmPushButtonGadget pb ;
        XEvent *event ;
        Region region ;
#else
DrawPushButtonLabelGadget(
        XmPushButtonGadget pb,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{   GC tmp_gc = NULL;
    Boolean   replaceGC = False;
    XmManagerWidget mw = (XmManagerWidget) XtParent(pb);
    Boolean deadjusted = False;

      if ((LabG_MenuType(pb) != XmMENU_PULLDOWN &&
       LabG_MenuType(pb) != XmMENU_POPUP) &&
         PBG_FillOnArm(pb))
	  {
       if ((LabG_LabelType(pb) == XmSTRING) &&
             (PBG_Armed(pb)) &&
         (PBG_ArmColor(pb) == mw->manager.foreground))
         {
            tmp_gc = LabG_NormalGC(pb);
            LabG_NormalGC(pb) = PBG_BackgroundGc(pb);
            replaceGC = True;
         }
       }
     if (LabG_LabelType(pb) == XmPIXMAP)
     {
        if (PBG_Armed(pb))
           if (PBG_ArmPixmap(pb) != XmUNSPECIFIED_PIXMAP)
          LabG_Pixmap(pb) = PBG_ArmPixmap(pb);
           else
          LabG_Pixmap(pb) = PBG_UnarmPixmap(pb);

        else   /* pushbutton is unarmed */
           LabG_Pixmap(pb) = PBG_UnarmPixmap(pb);
      }

    /*
     * Temporarily remove the Xm3D_ENHANCE_PIXEL hack ("adjustment")
     *             from the margin values, so we don't confuse LabelG.  The
     *             original code did the same thing, but in a round-about way.
     */
    if( PBG_DefaultButtonShadowThickness(pb) > 0 )
      { 
	deadjusted = True;
	LabG_MarginLeft(pb) -= Xm3D_ENHANCE_PIXEL;
	LabG_MarginRight(pb) -= Xm3D_ENHANCE_PIXEL;
	LabG_MarginTop(pb) -= Xm3D_ENHANCE_PIXEL;
	LabG_MarginBottom(pb) -= Xm3D_ENHANCE_PIXEL;
      }
    
    (* xmLabelGadgetClassRec.rect_class.expose) ((Widget) pb, event, region);
    
    if (deadjusted)
      {
	LabG_MarginLeft(pb) += Xm3D_ENHANCE_PIXEL;
	LabG_MarginRight(pb) += Xm3D_ENHANCE_PIXEL;
	LabG_MarginTop(pb) += Xm3D_ENHANCE_PIXEL;
	LabG_MarginBottom(pb) += Xm3D_ENHANCE_PIXEL;
      }
    
      if (replaceGC)
         LabG_NormalGC(pb) = tmp_gc;
}
/*
 * DrawPushButtonGadgetShadows()
 *  Note: PushButton has two types of shadows: primitive-shadow and
 *  default-button-shadow.
 *  Following shadows  are drawn:
 *  if pushbutton is in a menu only primitive shadows are drawn;
 *   else
 *    { draw default shadow if needed;
 *  draw primitive shadow ;
 *    }
 */
static void 
#ifdef _NO_PROTO
DrawPushButtonGadgetShadows( pb )
        XmPushButtonGadget pb ;
#else
DrawPushButtonGadgetShadows(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{

    if (PBG_DefaultButtonShadowThickness(pb))
	{ EraseDefaultButtonShadows (pb);
	  if (PBG_ShowAsDefault(pb))
		DrawDefaultButtonShadows (pb);
	}

    if (pb->gadget.shadow_thickness > 0)
	   DrawPBGadgetShadows(pb);
}

/*
 * DrawPBGadgetShadows (pb)
 *   - Should be called only if PrimitiveShadowThickness > 0
 */
static void 
#ifdef _NO_PROTO
DrawPBGadgetShadows( pb )
        XmPushButtonGadget pb ;
#else
DrawPBGadgetShadows(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{
   GC topgc, bottomgc;
   int dx, dy, width, height, adjust, shadow_thickness;

   if (PBG_Armed(pb))
   { 
      bottomgc  = XmParentTopShadowGC(pb); 
      topgc = XmParentBottomShadowGC(pb);
   }
   else
   {
      bottomgc  =  XmParentBottomShadowGC(pb);
      topgc = XmParentTopShadowGC(pb);
   }

   shadow_thickness = pb -> gadget.shadow_thickness;

   if ( (shadow_thickness > 0) && (topgc) && (bottomgc))
   { 
      if (pb->pushbutton.compatible)
         adjust = PBG_ShowAsDefault(pb);
      else
	 adjust = PBG_DefaultButtonShadowThickness(pb);

      if (adjust > 0)
      {   
	 adjust = (adjust << 1);
         dx = pb -> gadget.highlight_thickness + adjust +  
	      pb -> gadget.shadow_thickness;
      }
      else
	 dx = pb->gadget.highlight_thickness;

      dy = dx;
      width = pb -> rectangle.width - (dx << 1);
      height = pb -> rectangle.height - (dy << 1);

	   
      if ( (width > 0) && (height > 0))	
      { 
	 dx += pb->rectangle.x;
	 dy += pb->rectangle.y;
	 _XmDrawShadows (XtDisplay (pb), XtWindow (pb), topgc,
	    bottomgc, dx, dy, width, height, shadow_thickness, XmSHADOW_OUT);
      }
   }
}

static void 
#ifdef _NO_PROTO
EraseDefaultButtonShadows( pb )
        XmPushButtonGadget pb ;
#else
EraseDefaultButtonShadows(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{
   int dx, dy, width, height, default_button_shadow;
   int disp;

   if (pb->pushbutton.compatible)
      default_button_shadow = (int) (PBG_ShowAsDefault(pb));
   else
      default_button_shadow = (int) (PBG_DefaultButtonShadowThickness(pb));

   if (default_button_shadow > 0)
   { 
      dx = pb->gadget.highlight_thickness;
      disp = (dx << 1);
      width = pb->rectangle.width - disp;
      height = pb->rectangle.height - disp;

      if ( (width > 0) && (height > 0))	
      { 
	 dy = dx;
	 dx += pb->rectangle.x;
	 dy += pb->rectangle.y;
#ifdef CDE_VISUAL	/* default button */
        {
         Boolean default_button = False;

         XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
             &default_button, NULL);
	 if (default_button) {
	     dx = dy = Xm3D_ENHANCE_PIXEL;
	     width = pb->rectangle.width - 2 * dx;
	     height = pb->rectangle.height - 2 * dy;
	     dx += pb->rectangle.x;
	     dy += pb->rectangle.y;
	 }
        }
#endif
	 _XmClearBorder (XtDisplay (pb), XtWindow (XtParent(pb)),
	       dx, dy, width, height, default_button_shadow);
      }
   }
}

/*
 * DrawDefaultButtonShadows()
 *  - get the topShadowColor and bottomShadowColor from the parent;
 *    use those colors to construct top and bottom gc; use these
 *    GCs to draw the shadows of the button.
 *  - Should not be called if pushbutton is in a row column or in a menu.
 *  - Should be called only if a defaultbuttonshadow is to be drawn.
 */
static void 
#ifdef _NO_PROTO
DrawDefaultButtonShadows( pb )
        XmPushButtonGadget pb ;
#else
DrawDefaultButtonShadows(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{
   GC topgc, bottomgc;
   int dx, dy, width, height, default_button_shadow_thickness;


   topgc  = XmParentBottomShadowGC(pb);
   bottomgc = XmParentTopShadowGC(pb);

   if ( (bottomgc == NULL) || (topgc == NULL) ) 
      return;
	
   if (pb->pushbutton.compatible)
      default_button_shadow_thickness = (int) (PBG_ShowAsDefault(pb));
   else
      default_button_shadow_thickness = 
         (int) (PBG_DefaultButtonShadowThickness(pb));

   /*
    *
    * Compute location of bounding box to contain the defaultButtonShadow.
    */
   if (default_button_shadow_thickness > 0)
   { 
      dx = pb->gadget.highlight_thickness;
      dy = dx;
      width = pb->rectangle.width - (dx << 1);
      height= pb->rectangle.height - (dx << 1);
		 
      if ( (width > 0) && (height > 0))	
      { 
	 dx += pb->rectangle.x;
	 dy += pb->rectangle.y;

#ifdef CDE_VISUAL	/* default button */
        {
         Boolean default_button = False;

         XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject((Widget) pb)), "defaultButtonEmphasis",
            &default_button, NULL);
	 if (default_button) {
	     dx = dy = Xm3D_ENHANCE_PIXEL;
	     width = pb->rectangle.width - 2 * dx;
	     height = pb->rectangle.height - 2 * dy;
	     dx += pb->rectangle.x;
	     dy += pb->rectangle.y;
	     bottomgc = topgc;
	 }
        }
#endif
	 _XmDrawShadows (XtDisplay (pb), XtWindow (pb),
			  topgc, bottomgc, dx, dy, width, height,
			  default_button_shadow_thickness, XmSHADOW_OUT);
      }
   }
}

/*
 *  DrawPushBGBackground (pb)
 *  Note: This routine should never be called if the PushButtonGadget is
 *	  in a menu (Pull-down or Popup) since in these cases the background
 *	  GC in the PushButtonGadget will contain uncertain (uninitialized)
 *	  values. This will result in a core-dump when the XFillRectangle()
 *	  is called - since Xlib will try to do XFlushGC. 
 * 
 */
static void 
#ifdef _NO_PROTO
DrawPushBGBackground( pb )
        XmPushButtonGadget pb ;
#else
DrawPushBGBackground(
        XmPushButtonGadget pb )
#endif /* _NO_PROTO */
{  GC  tmp_gc;
	struct PBbox box;
     Boolean result;

#ifndef CDE_VISUAL	/* etched in menu button */
	if ((LabG_MenuType(pb) == XmMENU_PULLDOWN ) ||
	      (LabG_MenuType(pb) == XmMENU_POPUP) )
	{  /*  tmp_gc = NULL  */ ; 
	}
    else
#endif
	{ if ((PBG_Armed(pb)) && (PBG_FillOnArm(pb)))
            tmp_gc = PBG_FillGc(pb);
          else tmp_gc =  PBG_BackgroundGc(pb);
	  if ( tmp_gc)
	  { 
	    result = ComputePBLabelArea (pb, &box);
    	if ( (result) &&  (box.pbWidth > 0) && (box.pbHeight > 0))
           XFillRectangle (XtDisplay(pb), XtWindow(pb), tmp_gc,
 	                 box.pbx, box.pby, box.pbWidth, box.pbHeight);
	  }
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


static Boolean 
#ifdef _NO_PROTO
ComputePBLabelArea( pb, box )
        XmPushButtonGadget pb ;
        struct PBbox *box ;
#else
ComputePBLabelArea(
        XmPushButtonGadget pb,
        struct PBbox *box )
#endif /* _NO_PROTO */
{   Boolean result = True;
    int dx, adjust;
    short fill = 0; 
    XmManagerWidget mw = (XmManagerWidget) XtParent(pb);

    
    if ((PBG_ArmColor(pb) == mw->manager.top_shadow_color) ||
        (PBG_ArmColor(pb) == mw->manager.bottom_shadow_color))
            fill = 1;

    if (pb == NULL) result = False;
    else
    { 
       if (PBG_DefaultButtonShadowThickness(pb) > 0)
       { 
           adjust = PBG_DefaultButtonShadowThickness(pb) +
                    pb -> gadget.shadow_thickness;
           adjust = (adjust << 1);
           dx = pb -> gadget.highlight_thickness + adjust + fill; 
       }
       else
           dx = pb -> gadget.highlight_thickness +
                    pb -> gadget.shadow_thickness + fill;

       box->pbx = dx + pb->rectangle.x;
       box->pby = dx +pb->rectangle.y;
       adjust = (dx << 1);
       box->pbWidth = pb->rectangle.width - adjust;
       box->pbHeight= pb->rectangle.height - adjust;
     }
    return (result);
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
    if (PBG_DefaultButtonShadowThickness(widget) ||
        PBG_ShowAsDefault(widget))
    {
        if ((int)*value >= Xm3D_ENHANCE_PIXEL)
          *value -= Xm3D_ENHANCE_PIXEL;
    }

    _XmFromHorizontalPixels (widget, offset, value);
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
     XmPushButtonGadget newpb;
#else
SetPushButtonSize(
     XmPushButtonGadget newpb)
#endif /* _NO_PROTO */
{
  int leftx, rightx, dispx;
  unsigned int junk;
  unsigned int  onW = 0, onH = 0,  d;

  LabG_AccTextRect(newpb).width = 0;
  LabG_AccTextRect(newpb).height = 0;

   /* We know it's a pixmap so find out how how big it is */
  if (PBG_ArmPixmap(newpb) != XmUNSPECIFIED_PIXMAP)
      XGetGeometry (XtDisplay(newpb),
                    PBG_ArmPixmap(newpb),
                    (Window*)&junk, /* returned root window */
                    (int*)&junk, (int*)&junk, /* x, y of pixmap */
                    &onW, &onH, /* width, height of pixmap */
                    &junk,    /* border width */
                    &d);      /* depth */

  if ((onW > LabG_TextRect(newpb).width) || (onH > LabG_TextRect(newpb).height))
  {

    LabG_TextRect(newpb).width =  (unsigned short) onW;
    LabG_TextRect(newpb).height = (unsigned short) onH;
  }

  if (LabG__acceleratorText(newpb) != NULL)
  {
        Dimension w,h ;

        /*
         * If we have a string then size it.
         */
        if (!_XmStringEmpty (LabG__acceleratorText(newpb)))
        {
           _XmStringExtent(LabG_Font(newpb),
                           LabG__acceleratorText(newpb), &w, &h);
           LabG_AccTextRect(newpb).width = (unsigned short)w;
           LabG_AccTextRect(newpb).height = (unsigned short)h;
        }
  }

  /* increase margin width if necessary to accomadate accelerator text */
  if (LabG__acceleratorText(newpb) != NULL)

        if (LabG_MarginRight(newpb) <
          LabG_AccTextRect(newpb).width + LABELG_ACC_PAD)
        {
         LabG_MarginRight(newpb) =
             LabG_AccTextRect(newpb).width + LABELG_ACC_PAD;
        }

    /* Has a width been specified?  */

  if (newpb->rectangle.width == 0)
        newpb->rectangle.width =
            LabG_TextRect(newpb).width +
              LabG_MarginLeft(newpb) + LabG_MarginRight(newpb) +
                  (2 * (LabG_MarginWidth(newpb) +
                        newpb->gadget.highlight_thickness
                               + newpb->gadget.shadow_thickness));

  leftx =  newpb->gadget.highlight_thickness +
             newpb->gadget.shadow_thickness + LabG_MarginWidth(newpb) +
             LabG_MarginLeft(newpb);

  rightx = newpb->rectangle.width - newpb->gadget.highlight_thickness -
             newpb->gadget.shadow_thickness - LabG_MarginWidth(newpb) -
             LabG_MarginRight(newpb);


  switch (LabG_Alignment(newpb))
  {
     case XmALIGNMENT_BEGINNING:
         LabG_TextRect(newpb).x = leftx;
       break;

     case XmALIGNMENT_END:
       LabG_TextRect(newpb).x = rightx - LabG_TextRect(newpb).width;
       break;

     default:
       /* XmALIGNMENT_CENTER */
       dispx = ( (rightx -leftx) - LabG_TextRect(newpb).width)/2;
       LabG_TextRect(newpb).x = leftx + dispx;

       break;
  }

    /*  Has a height been specified?  */
  if (newpb->rectangle.height == 0)
        newpb->rectangle.height = Max(LabG_TextRect(newpb).height,
                                    LabG_AccTextRect(newpb).height)
          + LabG_MarginTop(newpb)
              + LabG_MarginBottom(newpb)
                  + (2 * (LabG_MarginHeight(newpb)
                          + newpb->gadget.highlight_thickness
                          + newpb->gadget.shadow_thickness));

  LabG_TextRect(newpb).y = newpb->gadget.highlight_thickness
          + newpb->gadget.shadow_thickness
              + LabG_MarginHeight(newpb) + LabG_MarginTop(newpb) +
                  ((newpb->rectangle.height - LabG_MarginTop(newpb)
                    - LabG_MarginBottom(newpb)
                    - (2 * (LabG_MarginHeight(newpb)
                            + newpb->gadget.highlight_thickness
                            + newpb->gadget.shadow_thickness))
                    - LabG_TextRect(newpb).height) / 2);

  if (LabG__acceleratorText(newpb) != NULL)
  {

       LabG_AccTextRect(newpb).x = (newpb->rectangle.width -
          newpb->gadget.highlight_thickness -
          newpb->gadget.shadow_thickness -
          LabG_MarginWidth(newpb) -
          LabG_MarginRight(newpb) +
          LABELG_ACC_PAD);

       LabG_AccTextRect(newpb).y = newpb->gadget.highlight_thickness
             + newpb->gadget.shadow_thickness
                 + LabG_MarginHeight(newpb) + LabG_MarginTop(newpb) +
                     ((newpb->rectangle.height - LabG_MarginTop(newpb)
                       - LabG_MarginBottom(newpb)
                       - (2 * (LabG_MarginHeight(newpb)
                               + newpb->gadget.highlight_thickness
                               + newpb->gadget.shadow_thickness))
                       - LabG_AccTextRect(newpb).height) / 2);

  }

  if (newpb->rectangle.width == 0)    /* set core width and height to a */
        newpb->rectangle.width = 1;     /* default value so that it doesn't */
  if (newpb->rectangle.height == 0)   /* cause a Toolkit Error */
        newpb->rectangle.height = 1;

}

