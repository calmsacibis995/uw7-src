#pragma ident	"@(#)m1.2libs:Xm/Gadget.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include <Xm/GadgetP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ManagerP.h>
#include <Xm/DrawP.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/Vendor.h>
#include "RepTypeI.h"
#include <Xm/ExtObjectP.h>
#include <stdio.h>
#include <ctype.h>


#define INVALID_UNIT_TYPE 255

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void GetHighlightColor() ;
static void GetTopShadowColor() ;
static void GetBottomShadowColor() ;
static void ClassInitialize() ;
static void ClassPartInit() ;
static void SecondaryObjectCreate() ;
static void Initialize() ;
static void Destroy() ;
static Boolean SetValues() ;
static void HighlightBorder() ;
static void UnhighlightBorder() ;
static void FocusChange() ;
static XmNavigability WidgetNavigable() ;

#else

static void GetHighlightColor( 
                        Widget w,
                        int offset,
                        XtArgVal *value) ;
static void GetTopShadowColor( 
                        Widget w,
                        int offset,
                        XtArgVal *value) ;
static void GetBottomShadowColor( 
                        Widget w,
                        int offset,
                        XtArgVal *value) ;
static void ClassInitialize( void ) ;
static void ClassPartInit( 
                        WidgetClass g) ;
static void SecondaryObjectCreate( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void HighlightBorder( 
                        Widget w) ;
static void UnhighlightBorder( 
                        Widget w) ;
static void FocusChange( 
                        Widget wid,
                        XmFocusChange change) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  Resource definitions for Subclasses of Gadget */

static XtResource resources[] =
{
   {
     XmNx, XmCPosition, XmRHorizontalPosition, sizeof(Position),
     XtOffsetOf( struct _WidgetRec, core.x), XmRImmediate, (XtPointer) 0
   },

   {
     XmNy, XmCPosition, XmRVerticalPosition, sizeof(Position),
     XtOffsetOf( struct _WidgetRec, core.y), XmRImmediate, (XtPointer) 0
   },

   {
     XmNwidth, XmCDimension, XmRHorizontalDimension, sizeof(Dimension),
     XtOffsetOf( struct _WidgetRec, core.width), XmRImmediate, (XtPointer) 0
   },

   {
     XmNheight, XmCDimension, XmRVerticalDimension, sizeof(Dimension),
     XtOffsetOf( struct _WidgetRec, core.height), XmRImmediate, (XtPointer) 0
   },

   {
     XmNborderWidth, XmCBorderWidth, XmRHorizontalDimension, sizeof(Dimension),
     XtOffsetOf( struct _WidgetRec, core.border_width), XmRImmediate, (XtPointer) 0
   },

   {
     XmNtraversalOn, XmCTraversalOn, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmGadgetRec, gadget.traversal_on),
     XmRImmediate, (XtPointer) True
   },

   {
     XmNhighlightOnEnter, XmCHighlightOnEnter, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmGadgetRec, gadget.highlight_on_enter),
     XmRImmediate, (XtPointer) False
   },

   {
     XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
     sizeof (Dimension), XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, 
     sizeof (Dimension), XtOffsetOf( struct _XmGadgetRec, gadget.shadow_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNunitType, XmCUnitType, XmRUnitType, sizeof (unsigned char),
     XtOffsetOf( struct _XmGadgetRec, gadget.unit_type),
     XmRCallProc, (XtPointer) _XmUnitTypeDefault
   },

   {
     XmNnavigationType, XmCNavigationType, XmRNavigationType, sizeof (unsigned char),
     XtOffsetOf( struct _XmGadgetRec, gadget.navigation_type),
     XmRImmediate, (XtPointer) XmNONE
   },

   {
     XmNhelpCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmGadgetRec, gadget.help_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNuserData, XmCUserData, XmRPointer, sizeof(XtPointer),
     XtOffsetOf( struct _XmGadgetRec, gadget.user_data),
     XmRPointer, (XtPointer) NULL
   },
};


/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
   { XmNx,
     sizeof (Position),
     XtOffsetOf( struct _RectObjRec, rectangle.x),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNy,
     sizeof (Position),
     XtOffsetOf( struct _RectObjRec, rectangle.y),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNwidth,
     sizeof (Dimension),
     XtOffsetOf( struct _RectObjRec, rectangle.width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNheight,
     sizeof (Dimension),
     XtOffsetOf( struct _RectObjRec, rectangle.height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNhighlightThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNshadowThickness,
     sizeof (Dimension),
     XtOffsetOf( struct _XmGadgetRec, gadget.shadow_thickness),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNhighlightColor,
     sizeof (Pixel),
     XtOffsetOf( struct _XmGadgetRec, object.parent),
     GetHighlightColor,
     NULL },

   { XmNtopShadowColor,
     sizeof (Pixel),
     XtOffsetOf( struct _XmGadgetRec, object.parent),
     GetTopShadowColor,
     NULL },

   { XmNbottomShadowColor,
     sizeof (Pixel),
     XtOffsetOf( struct _XmGadgetRec, object.parent),
     GetBottomShadowColor,
     NULL },
};

static XmBaseClassExtRec baseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,                               /* Initialize Prehook  */
    NULL,                               /* SetValues Prehook   */
    NULL,                               /* Initialize Posthook */
    NULL,                               /* SetValues PostHook  */
    NULL,                               /* SecondaryObjectClass */
    SecondaryObjectCreate,              /* SecondaryObjectCreate */
    NULL,		                /* getSecRes data	*/
    {0},				/* fastSubclass flags	*/
    NULL,                               /* GetValues Prehook   */
    NULL,                               /* GetValues Posthook   */
    NULL,                               /* classPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    WidgetNavigable,                    /* widgetNavigable      */
    FocusChange,                        /* focusChange          */
};

XmGadgetClassExtRec  _XmGadClassExtRec = {
    NULL,
    NULLQUARK,
    XmGadgetClassExtVersion,
    sizeof(XmGadgetClassExtRec),
    NULL,                               /* widget_baseline */
    NULL,                               /* widget_display_rect */
};

/*  The gadget class record definition  */

externaldef(xmgadgetclassrec) XmGadgetClassRec xmGadgetClassRec =
{
   {
      (WidgetClass) &rectObjClassRec,   /* superclass	         */	
      "XmGadget",                       /* class_name	         */	
      sizeof(XmGadgetRec),              /* widget_size	         */	
      ClassInitialize,                  /* class_initialize      */
      ClassPartInit,                    /* class part initialize */
      False,                            /* class_inited          */	
      Initialize,                       /* initialize	         */	
      NULL,                             /* initialize_hook       */
      NULL,	                        /* realize	         */	
      NULL,				/* actions               */	
      0,				/* num_actions	         */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      True,                             /* compress_motion       */
      True,                             /* compress_exposure     */	
      True,                             /* compress_enterleave   */
      False,                            /* visible_interest      */
      Destroy,                          /* destroy               */	
      NULL,                             /* resize                */	
      NULL,				/* expose                */	
      SetValues,                        /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      _XmGadgetGetValuesHook,           /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      NULL,                             /* tm_table              */
      NULL,                             /* query_geometry        */
      NULL,				/* display_accelerator   */
      (XtPointer)&baseClassExtRec,      /* extension             */
   },

   {
      HighlightBorder,		        /* border_highlight   */
      UnhighlightBorder,		/* border_unhighlight */
      NULL,				/* arm_and_activate   */
      NULL,				/* input_dispatch     */
      NULL,				/* visual_change      */
      syn_resources,			/* syn resources      */
      XtNumber(syn_resources),		/* num_syn_resources  */
      NULL,				/* cache_part	      */
      (XtPointer)&_XmGadClassExtRec,	/* extension          */
   }
};

externaldef(xmgadgetclass) WidgetClass xmGadgetClass = 
		           (WidgetClass) &xmGadgetClassRec;



/************************************************************************
 *
 *  The following functions are synthetic hooks.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
GetHighlightColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
GetHighlightColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) XtParent(w);

    *value = (XtArgVal) mw->manager.highlight_color;
}

static void 
#ifdef _NO_PROTO
GetTopShadowColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
GetTopShadowColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) XtParent(w);

    *value = (XtArgVal) mw->manager.top_shadow_color;
}

static void 
#ifdef _NO_PROTO
GetBottomShadowColor( w, offset, value)
        Widget w ;
		int offset ;
		XtArgVal *value ;
#else
GetBottomShadowColor(
        Widget w,
		int offset,
		XtArgVal *value )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) XtParent(w);

    *value = (XtArgVal) mw->manager.bottom_shadow_color;
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
   _XmInitializeExtensions();
   baseClassExtRec.record_type = XmQmotif;
}


/************************************************************************
 *
 *  ClassPartInit
 *	Used by subclasses of gadget to inherit class record procedures.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInit( g )
        WidgetClass g ;
#else
ClassPartInit(
        WidgetClass g )
#endif /* _NO_PROTO */
{
    static Boolean first_time = TRUE;
    XmGadgetClass wc = (XmGadgetClass) g;
    XmGadgetClass super = (XmGadgetClass) wc->rect_class.superclass;

    if (wc->gadget_class.border_highlight == XmInheritWidgetProc)
	wc->gadget_class.border_highlight = 
           super->gadget_class.border_highlight;

    if (wc->gadget_class.border_unhighlight == XmInheritWidgetProc)
	wc->gadget_class.border_unhighlight =
	   super->gadget_class.border_unhighlight;

    if (wc->gadget_class.arm_and_activate == XmInheritArmAndActivate)
        wc->gadget_class.arm_and_activate =
           super->gadget_class.arm_and_activate;

    if (wc->gadget_class.input_dispatch == XmInheritInputDispatch)
        wc->gadget_class.input_dispatch =
           super->gadget_class.input_dispatch;

    if (wc->gadget_class.visual_change == XmInheritVisualChange)
        wc->gadget_class.visual_change =
           super->gadget_class.visual_change;

   _XmBaseClassPartInitialize((WidgetClass) wc);
   _XmFastSubclassInit (g, XmGADGET_BIT);

    if (first_time)
    {
        _XmSortResourceList((XrmResource **) xmGadgetClassRec.rect_class.resources,
            xmGadgetClassRec.rect_class.num_resources);
        first_time = FALSE;
    }

   _XmBuildGadgetResources((WidgetClass) wc);
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
    Arg                         myArgs[1];
    ArgList                     mergedArgs;

    XtSetArg(myArgs[0] ,XmNlogicalParent, new_w);

    if (*num_args)
       mergedArgs = XtMergeArgLists(args, *num_args, myArgs, XtNumber(myArgs));
    else
       mergedArgs = myArgs;


    cePtr = _XmGetBaseClassExtPtr(XtClass(new_w), XmQmotif);
    (void) XtCreateWidget(XtName(new_w),
                         (*cePtr)->secondaryObjectClass,
			 XtParent(new_w) ? XtParent(new_w) : new_w,
			 mergedArgs, *num_args + 1);

    if (mergedArgs != myArgs)
      XtFree( (char *) mergedArgs);
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
    XmGadget request = (XmGadget) rw ;
    XmGadget 			gw = (XmGadget) nw;
    XmBaseClassExt              *cePtr;
    XtInitProc                  secondaryCreate;

   if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE, gw->gadget.unit_type,
                                                             (Widget) gw)    )
   {
      gw->gadget.unit_type = XmPIXELS;
   }


   /*  Convert the fields from unit values to pixel values  */

   _XmGadgetImportArgs( (Widget) gw, args, num_args);
   _XmGadgetImportSecondaryArgs( (Widget) gw, args, num_args);

    cePtr = _XmGetBaseClassExtPtr(XtClass(gw), XmQmotif);

    if ((*cePtr) &&
	(*cePtr)->secondaryObjectClass &&
	(secondaryCreate = (*cePtr)->secondaryObjectCreate))
      (*secondaryCreate)( (Widget) request, (Widget) gw, args, num_args);

   gw->gadget.event_mask = 0;
   gw->gadget.have_traversal = FALSE ;
   gw->gadget.highlighted = FALSE ;
   gw->gadget.highlight_drawn = FALSE ;

   if(    (gw->gadget.navigation_type != XmDYNAMIC_DEFAULT_TAB_GROUP)
       && !XmRepTypeValidValue( XmRID_NAVIGATION_TYPE, 
                                  gw->gadget.navigation_type, (Widget) gw)    )
   {   gw->gadget.navigation_type = XmNONE ;
       } 

   _XmNavigInitialize ((Widget) request, (Widget) gw, args, num_args);

   gw->gadget.have_traversal = FALSE ;

   /*  Check the geometry information for the widget  */

   if (request->rectangle.width == 0)
      gw->rectangle.width += gw->gadget.highlight_thickness * 2 +
                             gw->gadget.shadow_thickness * 2;

   if (request->rectangle.height == 0)
      gw->rectangle.height += gw->gadget.highlight_thickness * 2 + 
                              gw->gadget.shadow_thickness * 2;


   /*  Force the border width to 0  */

   gw->rectangle.border_width = 0;

   return ;
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
   _XmNavigDestroy(w);
   XtRemoveAllCallbacks (w, XmNhelpCallback);

}




/************************************************************************
 *
 *  SetValues
 *     Perform and updating necessary for a set values call.
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
        XmGadget cur = (XmGadget) cw ;
        XmGadget req = (XmGadget) rw ;
        XmGadget new_w = (XmGadget) nw ;
   Boolean returnFlag;

   if(    cur->gadget.navigation_type != new_w->gadget.navigation_type    )
     {
       if(    !XmRepTypeValidValue( XmRID_NAVIGATION_TYPE, 
			        new_w->gadget.navigation_type, (Widget) new_w)    )
	 {
	   new_w->gadget.navigation_type = cur->gadget.navigation_type ;
	 } 
     }
   returnFlag = _XmNavigSetValues ((Widget) cur, (Widget) req, (Widget) new_w,
				                               args, num_args);
   /*  Validate changed data.  */

   if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE, new_w->gadget.unit_type,
                                                             (Widget) new_w)    )
   {
      new_w->gadget.unit_type = cur->gadget.unit_type;
   }


   /*  Convert the necessary fields from unit values to pixel values  */

   _XmGadgetImportArgs((Widget) new_w, args, num_args);

   /*  Check for resize conditions  */

   if (cur->gadget.shadow_thickness != new_w->gadget.shadow_thickness ||
       cur->gadget.highlight_thickness != new_w->gadget.highlight_thickness)
      returnFlag = True;
   

   /*  Force the border width to 0  */

   new_w->rectangle.border_width = 0;

   if(    new_w->gadget.highlight_drawn
      &&  (    !XtIsSensitive( (Widget) new_w)
	   ||  (    cur->gadget.highlight_on_enter
		&&  !(new_w->gadget.highlight_on_enter)
		&&  (_XmGetFocusPolicy( (Widget) new_w) == XmPOINTER)))    )
     {
       if(    ((XmGadgetClass) XtClass( new_w))
	                                 ->gadget_class.border_unhighlight    )
	 {
	   (*(((XmGadgetClass) XtClass( new_w))
	                   ->gadget_class.border_unhighlight))( (Widget) new_w) ;
	 }
     }

   /*  Return a flag which may indicate that a redraw needs to occur.  */
   
   return (returnFlag);
}


/**********************************************************************
 *
 *  _XmBuildGadgetResources
 *	Build up the gadget's synthetic resource processing list 
 *	by combining the super classes with this class.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmBuildGadgetResources( c )
        WidgetClass c ;
#else
_XmBuildGadgetResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
	XmGadgetClass wc = (XmGadgetClass) c ;
	XmGadgetClass sc;
	XmBaseClassExt *classExtPtr;
	XmExtClassRec *secondaryObjClass;
	WidgetClass secObjSuperClass;

	sc = (XmGadgetClass) wc->rect_class.superclass;

	_XmInitializeSyntheticResources(wc->gadget_class.syn_resources,
		wc->gadget_class.num_syn_resources);

	/*
	 * RectObj has no synthetic resources to incorporate.
	 */
	if (sc != (XmGadgetClass) rectObjClass)
	{
		_XmBuildResources (&(wc->gadget_class.syn_resources),
			&(wc->gadget_class.num_syn_resources),
			sc->gadget_class.syn_resources,
			sc->gadget_class.num_syn_resources);
	}

	classExtPtr = _XmGetBaseClassExtPtr(c, XmQmotif);
	secondaryObjClass = (XmExtClassRec *)
		((*classExtPtr)->secondaryObjectClass);

	/*
	 * Not all gadgets have secondary objects.
	 */
	if (secondaryObjClass == NULL) return;

	secObjSuperClass = secondaryObjClass->object_class.superclass;

	_XmInitializeSyntheticResources(
		secondaryObjClass->ext_class.syn_resources,
		secondaryObjClass->ext_class.num_syn_resources);

	/*
	 * ExtObject has no synthetic resources to incorporate.
	 */
	if (secObjSuperClass != (WidgetClass) xmExtObjectClass)
	{
		_XmBuildResources (
			&(secondaryObjClass->ext_class.syn_resources),
			&(secondaryObjClass->ext_class.num_syn_resources),
			((XmExtClassRec *)secObjSuperClass)
				->ext_class.syn_resources,
			((XmExtClassRec *)secObjSuperClass)
				->ext_class.num_syn_resources);
	}
}

static void 
#ifdef _NO_PROTO
HighlightBorder( w )
        Widget w ;
#else
HighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{   
            XmGadget g ;

    g = (XmGadget) w ;

    if(    g->rectangle.width == 0 || g->rectangle.height == 0
        || g->gadget.highlight_thickness == 0    )
    {   
        return ;
        } 

    g->gadget.highlighted = True ;
    g->gadget.highlight_drawn = True ;

    _XmDrawSimpleHighlight( XtDisplay( (Widget) g), XtWindow( (Widget) g), 
           ((XmManagerWidget)(g->object.parent))->manager.highlight_GC,
             g->rectangle.x, g->rectangle.y, g->rectangle.width,
               g->rectangle.height, g->gadget.highlight_thickness) ;
    return ;
    }

static void 
#ifdef _NO_PROTO
UnhighlightBorder( w )
        Widget w ;
#else
UnhighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{   
            XmGadget g ;

    g = (XmGadget) w ;

    g->gadget.highlighted = False ;
    g->gadget.highlight_drawn = False ;

    if(    !g->rectangle.width  ||  !g->rectangle.height
        || !g->gadget.highlight_thickness    )
    {   
        return ;
        } 

    _XmClearBorder( XtDisplay( g), XtWindow( g), g->rectangle.x,
                       g->rectangle.y, g->rectangle.width, g->rectangle.height,
                                               g->gadget.highlight_thickness) ;
    return ;
    }

static void
#ifdef _NO_PROTO
FocusChange( wid, change)
        Widget wid ;
        XmFocusChange change ;
#else
FocusChange(
        Widget wid,
        XmFocusChange change)
#endif /* _NO_PROTO */
{   
  /* Enter/Leave is called only in pointer mode,
   * Focus in/out only called in explicit mode.
   */
  switch(    change    )
    {
    case XmENTER:
      if(    !(((XmGadget) wid)->gadget.highlight_on_enter)    )
	{
	  break ;
	}
      /* Drop through. */
    case XmFOCUS_IN:
      if(    change == XmFOCUS_IN    ) /* Because of drop-though. */
	{
	  ((XmGadget) wid)->gadget.have_traversal = TRUE ;
	}
      if(    ((XmGadgetClass) XtClass( wid))
                                           ->gadget_class.border_highlight    )
        {   
	  (*(((XmGadgetClass) XtClass( wid))
                                      ->gadget_class.border_highlight))( wid) ;
	} 
      break ;
    case XmLEAVE:
      if(    !(((XmGadget) wid)->gadget.highlight_on_enter)    )
	{
	  break ;
	}
      /* Drop through. */
    case XmFOCUS_OUT:
      if(    change == XmFOCUS_OUT    ) /* Because of drop-though. */
	{
	  ((XmGadget) wid)->gadget.have_traversal = FALSE ;
	}
      if(    ((XmGadgetClass) XtClass( wid))
                                         ->gadget_class.border_unhighlight    )
        {   
	  (*(((XmGadgetClass) XtClass( wid))
                                    ->gadget_class.border_unhighlight))( wid) ;
	} 
      break ;
    }
  return ;
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
  if(    ((XmGadget) wid)->rectangle.sensitive
     &&  ((XmGadget) wid)->rectangle.ancestor_sensitive
     &&  ((XmGadget) wid)->gadget.traversal_on    )
    {   
      XmNavigationType nav_type = ((XmGadget) wid)->gadget.navigation_type ;

      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	 ||  (    (nav_type == XmTAB_GROUP)
	      &&  !_XmShellIsExclusive( wid))    )
	{
	  return XmTAB_NAVIGABLE ;
	}
      return XmCONTROL_NAVIGABLE ;
    }
  return XmNOT_NAVIGABLE ;
}
