#pragma ident	"@(#)m1.2libs:Xm/Manager.c	1.4"
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
#include <Xm/ManagerP.h>
#include <Xm/BaseClassP.h>
#include <Xm/GadgetP.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/VirtKeysP.h>
#include "RepTypeI.h"
#include "MessagesI.h"


#define IsButton(w) ( \
      XmIsPushButton(w)   || \
      XmIsToggleButton(w) || \
      XmIsArrowButton(w)  || \
      XmIsCascadeButton(w)|| \
      XmIsDrawnButton(w))

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void GetXFromShell() ;
static void GetYFromShell() ;
static void ClassInitialize() ;
static CompositeClassExtension FindClassExtension() ;
static void BuildManagerResources() ;
static void ClassPartInitialize() ;
static void GetBackgroundGC() ;
static void GetHighlightGC() ;
static void GetTopShadowGC() ;
static void GetBottomShadowGC() ;
static void Initialize() ;
static void Realize() ;
static void Destroy() ;
static Boolean CallVisualChange() ;
static Boolean ChildVisualChange() ;
static Boolean SetValues() ;
static void InsertChild() ;
static void DeleteChild() ;
static void ManagerMotion() ;
static void ManagerEnter() ;
static void ManagerLeave() ;
static void AddMotionHandlers() ;
static void DoMagicBBCompatibilityStuff() ;
static void ConstraintInitialize() ;
static void CheckRemoveMotionHandlers() ;
static void ConstraintDestroy() ;
static Boolean ConstraintSetValues() ;
static Boolean ManagerParentProcess() ;
static XmNavigability WidgetNavigable() ;
#ifdef CDE_TAB 
extern Boolean _XmTraverseWillWrap();
#endif /* CDE_TAB */

#else

static void GetXFromShell( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetYFromShell( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void ClassInitialize( void ) ;
static CompositeClassExtension FindClassExtension( 
                        WidgetClass widget_class) ;
static void BuildManagerResources(
			WidgetClass c) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void GetBackgroundGC( 
                        XmManagerWidget mw) ;
static void GetHighlightGC( 
                        XmManagerWidget mw) ;
static void GetTopShadowGC( 
                        XmManagerWidget mw) ;
static void GetBottomShadowGC( 
                        XmManagerWidget mw) ;
static void Initialize( 
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Realize( 
                        Widget w,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget w) ;
static Boolean CallVisualChange( 
                        XmGadget child,
                        XmGadgetClass w_class,
                        Widget cur,
                        Widget new_w) ;
static Boolean ChildVisualChange( 
                        XmManagerWidget cur,
                        XmManagerWidget new_w) ;
static Boolean SetValues( 
                        Widget current,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InsertChild( 
                        Widget child) ;
static void DeleteChild( 
                        Widget child) ;
static void ManagerMotion( 
                        Widget wid,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void ManagerEnter( 
                        Widget wid,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void ManagerLeave( 
                        Widget wid,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void AddMotionHandlers( 
                        XmManagerWidget mw) ;
static void DoMagicBBCompatibilityStuff( 
                        Widget wid) ;
static void ConstraintInitialize( 
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void CheckRemoveMotionHandlers( 
                        XmManagerWidget mw) ;
static void ConstraintDestroy( 
                        Widget w) ;
static Boolean ConstraintSetValues( 
                        Widget current,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean ManagerParentProcess( 
                        Widget widget,
                        XmParentProcessData data) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;
#ifdef CDE_TAB 
extern Boolean _XmTraverseWillWrap(
			Widget w,
			XmTraversalDirection dir);
#endif /* CDE_TAB */
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/***********************************/
/*  Default actions for XmManager  */

static XtActionsRec actions[] = {
	{ "ManagerEnter",               _XmManagerEnter },
	{ "ManagerLeave",               _XmManagerLeave },
	{ "ManagerFocusIn",             _XmManagerFocusIn },  
	{ "ManagerFocusOut",            _XmManagerFocusOut },  
	{ "ManagerGadgetPrevTabGroup",  _XmGadgetTraversePrevTabGroup},
	{ "ManagerGadgetNextTabGroup",  _XmGadgetTraverseNextTabGroup},
	{ "ManagerGadgetTraversePrev",  _XmGadgetTraversePrev },  
	{ "ManagerGadgetTraverseNext",  _XmGadgetTraverseNext },  
	{ "ManagerGadgetTraverseLeft",  _XmGadgetTraverseLeft },  
	{ "ManagerGadgetTraverseRight", _XmGadgetTraverseRight },  
	{ "ManagerGadgetTraverseUp",    _XmGadgetTraverseUp },  
	{ "ManagerGadgetTraverseDown",  _XmGadgetTraverseDown },  
	{ "ManagerGadgetTraverseHome",  _XmGadgetTraverseHome },
	{ "ManagerGadgetSelect",        _XmGadgetSelect },
	{ "ManagerParentActivate",      _XmManagerParentActivate },
	{ "ManagerParentCancel",        _XmManagerParentCancel },
	{ "ManagerGadgetButtonMotion",  _XmGadgetButtonMotion },
	{ "ManagerGadgetKeyInput",      _XmGadgetKeyInput },
	{ "ManagerGadgetHelp",          _XmManagerHelp },
        { "ManagerGadgetArm",           _XmGadgetArm },
        { "ManagerGadgetDrag",          _XmGadgetDrag },
	{ "ManagerGadgetActivate",      _XmGadgetActivate },
	{ "ManagerGadgetMultiArm",      _XmGadgetMultiArm },
	{ "ManagerGadgetMultiActivate", _XmGadgetMultiActivate },
        { "Enter",           _XmManagerEnter },         /* Motif 1.0 BC. */
        { "FocusIn",         _XmManagerFocusIn },       /* Motif 1.0 BC. */
        { "Help",            _XmManagerHelp },          /* Motif 1.0 BC. */
        { "Arm",             _XmGadgetArm },            /* Motif 1.0 BC. */
	{ "Activate",        _XmGadgetActivate },       /* Motif 1.0 BC. */

};
 

/****************************************/
/*  Resource definitions for XmManager  */

static XtResource resources[] =
{
   {
     XmNunitType, XmCUnitType, XmRUnitType, 
     sizeof (unsigned char), XtOffsetOf(XmManagerRec, manager.unit_type),
     XmRCallProc, (XtPointer) _XmUnitTypeDefault
   },

   {
     XmNx, XmCPosition, XmRHorizontalPosition, 
     sizeof(Position), XtOffsetOf(WidgetRec, core.x), 
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNy, XmCPosition, XmRVerticalPosition, 
     sizeof(Position), XtOffsetOf(WidgetRec, core.y), 
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNwidth, XmCDimension, XmRHorizontalDimension, 
     sizeof(Dimension), XtOffsetOf(WidgetRec, core.width), 
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNheight, XmCDimension, XmRVerticalDimension, 
     sizeof(Dimension), XtOffsetOf(WidgetRec, core.height), 
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNborderWidth, XmCBorderWidth, XmRHorizontalDimension, 
     sizeof(Dimension), XtOffsetOf(WidgetRec, core.border_width), 
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNforeground, XmCForeground, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(XmManagerRec, manager.foreground),
     XmRCallProc, (XtPointer) _XmForegroundColorDefault
   },

   {
     XmNbackground, XmCBackground, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(WidgetRec, core.background_pixel),
     XmRCallProc, (XtPointer) _XmBackgroundColorDefault
   },

   {
     XmNbackgroundPixmap, XmCPixmap, XmRXmBackgroundPixmap, 
     sizeof (Pixmap), XtOffsetOf(WidgetRec, core.background_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },

   {
     XmNhighlightColor, XmCHighlightColor, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(XmManagerRec, manager.highlight_color),
     XmRCallProc, (XtPointer) _XmHighlightColorDefault
   },

   {
     XmNhighlightPixmap, XmCHighlightPixmap, XmRManHighlightPixmap,
     sizeof (Pixmap), XtOffsetOf(XmManagerRec, manager.highlight_pixmap),
     XmRCallProc, (XtPointer) _XmManagerHighlightPixmapDefault
   },

   {
     XmNnavigationType, XmCNavigationType, XmRNavigationType, 
     sizeof (unsigned char), 
     XtOffsetOf(XmManagerRec, manager.navigation_type),
     XmRImmediate, (XtPointer) XmTAB_GROUP,
   },

   {
     XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, 
     sizeof (Dimension), XtOffsetOf(XmManagerRec, manager.shadow_thickness),
     XmRImmediate, (XtPointer) 0
   },

   {
     XmNtopShadowColor, XmCTopShadowColor, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(XmManagerRec, manager.top_shadow_color),
     XmRCallProc, (XtPointer) _XmTopShadowColorDefault
   },

   {
     XmNtopShadowPixmap, XmCTopShadowPixmap, XmRManTopShadowPixmap,
     sizeof (Pixmap), XtOffsetOf(XmManagerRec, manager.top_shadow_pixmap),
     XmRCallProc, (XtPointer) _XmManagerTopShadowPixmapDefault
   },

   {
     XmNbottomShadowColor, XmCBottomShadowColor, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(XmManagerRec, manager.bottom_shadow_color),
     XmRCallProc, (XtPointer) _XmBottomShadowColorDefault
   },

   {
     XmNbottomShadowPixmap, XmCBottomShadowPixmap, XmRManBottomShadowPixmap,
     sizeof (Pixmap), XtOffsetOf(XmManagerRec, manager.bottom_shadow_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },

   {
     XmNhelpCallback, XmCCallback, XmRCallback, 
     sizeof(XtCallbackList), XtOffsetOf(XmManagerRec, manager.help_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNuserData, XmCUserData, XmRPointer, 
     sizeof(XtPointer), XtOffsetOf(XmManagerRec, manager.user_data),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNtraversalOn, XmCTraversalOn, XmRBoolean, 
     sizeof(Boolean), XtOffsetOf(XmManagerRec, manager.traversal_on),
     XmRImmediate, (XtPointer) TRUE
   },
   {
     XmNstringDirection, XmCStringDirection, XmRStringDirection,
     sizeof(XmStringDirection), 
     XtOffsetOf(XmManagerRec, manager.string_direction),
     XmRImmediate, (XtPointer) XmSTRING_DIRECTION_DEFAULT
   },
   {   
     XmNinitialFocus, XmCInitialFocus, XmRWidget,
     sizeof(Widget), XtOffsetOf(XmManagerRec, manager.initial_focus),
     XmRImmediate, NULL
   } 
};

/***************************************/
/*  Definition for synthetic resources */

static XmSyntheticResource syn_resources[] =
{
   { XmNx,
     sizeof (Position), XtOffsetOf(WidgetRec, core.x), 
     GetXFromShell, _XmToHorizontalPixels },

   { XmNy,
     sizeof (Position), XtOffsetOf(WidgetRec, core.y), 
     GetYFromShell,  _XmToVerticalPixels },

   { XmNwidth,
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.width),
     _XmFromHorizontalPixels, _XmToHorizontalPixels },

   { XmNheight,
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.height), 
     _XmFromVerticalPixels, _XmToVerticalPixels },

   { XmNborderWidth, 
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.border_width), 
     _XmFromHorizontalPixels, _XmToHorizontalPixels },

   { XmNshadowThickness, 
     sizeof (Dimension), XtOffsetOf(XmManagerRec, manager.shadow_thickness), 
     _XmFromHorizontalPixels, _XmToHorizontalPixels }
};


/*******************************************/
/*  Declaration of class extension records */

static XmBaseClassExtRec baseClassExtRec = {
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
    NULL,               		/* getSecRes data	*/
    { 0 },      			/* fastSubclass flags	*/
    NULL,				/* getValuesPrehook	*/
    NULL,				/* getValuesPosthook	*/
    NULL,                               /* ClassPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    WidgetNavigable,                    /* widgetNavigable      */
    NULL                                /* focusChange          */
};

static CompositeClassExtensionRec compositeClassExtRec = {
    NULL,
    NULLQUARK,
    XtCompositeExtensionVersion,
    sizeof(CompositeClassExtensionRec),
    TRUE,                               /* accepts_objects */
};

static XmManagerClassExtRec managerClassExtRec = {
    NULL,
    NULLQUARK,
    XmManagerClassExtVersion,
    sizeof(XmManagerClassExtRec),
    NULL                                /* traversal_children */
};

/******************************************/
/*  The Manager class record definition.  */

externaldef(xmmanagerclassrec) XmManagerClassRec xmManagerClassRec =
{
   {
      (WidgetClass) &constraintClassRec,     /* superclass            */
     "XmManager",		             /* class_name	      */
      sizeof(XmManagerRec),                  /* widget_size	      */
      ClassInitialize,                       /* class_initialize      */
      ClassPartInitialize,                   /* class part initialize */
      False,                                 /* class_inited          */
      Initialize,                            /* initialize	      */
      NULL,                                  /* initialize hook       */
      Realize,                               /* realize	              */
      actions,                               /* actions               */
      XtNumber(actions),                     /* num_actions	      */
      resources,                             /* resources	      */
      XtNumber(resources),                   /* num_resources         */
      NULLQUARK,                             /* xrm_class	      */
      True,                                  /* compress_motion       */
      XtExposeCompressMaximal,               /* compress_exposure     */
      True,                                  /* compress enterleave   */
      False,                                 /* visible_interest      */
      Destroy,                               /* destroy               */
      NULL,                                  /* resize                */
      NULL,                                  /* expose                */
      SetValues,                             /* set_values	      */
      NULL,                                  /* set_values_hook       */
      XtInheritSetValuesAlmost,              /* set_values_almost     */
      _XmManagerGetValuesHook,               /* get_values_hook       */
      NULL,                                  /* accept_focus	      */
      XtVersion,                             /* version               */
      NULL,                                  /* callback private      */
      _XmManager_defaultTranslations,        /* tm_table              */
      NULL,                                  /* query geometry        */
      NULL,                                  /* display_accelerator   */
      (XtPointer)&baseClassExtRec,           /* extension             */
   },
   {					     /* composite class   */
      NULL,                                  /* Geometry Manager  */
      NULL,                                  /* Change Managed    */
      InsertChild,                           /* Insert Child      */
      DeleteChild,	                     /* Delete Child      */
      (XtPointer)&compositeClassExtRec,      /* extension         */
   },

   {						/* constraint class	*/
      NULL,					/* resources		*/
      0,					/* num resources	*/
      sizeof (XmManagerConstraintRec),	        /* constraint record	*/
      ConstraintInitialize,			/* initialize		*/
      ConstraintDestroy,			/* destroy		*/
      ConstraintSetValues,			/* set values		*/
      NULL,					/* extension		*/
   },

   {						/* manager class	  */
      _XmManager_managerTraversalTranslations,  /* default translations   */
      syn_resources,				/* syn resources      	  */
      XtNumber(syn_resources),			/* num_syn_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_syn_cont_resources */
      ManagerParentProcess,                     /* parent_process         */
      (XtPointer)&managerClassExtRec,		/* extension		  */
   },
};

externaldef(xmmanagerwidgetclass) WidgetClass xmManagerWidgetClass = 
                                 (WidgetClass) &xmManagerClassRec;




/**************************************************************************
**
** Synthetic resource hooks function section
**
**************************************************************************/

static void 
#ifdef _NO_PROTO
GetXFromShell( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetXFromShell(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
    /* return the x in the child's unit type; for children of shell, return
     * the parent's x relative to the origin, in pixels */

    Widget parent = XtParent(wid);
    
    if (XtIsShell(parent)) {   
	/* at the moment menuShell doesn't reset x,y values to 0, so 
	** we'll have them counted twice if we use XtTranslateCoords
	*/
        *value = (XtArgVal) parent->core.x;
    } else {
	*value = (XtArgVal) wid->core.x ;
	_XmFromHorizontalPixels(wid,  resource_offset, value);
    }
}

static void 
#ifdef _NO_PROTO
GetYFromShell( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
GetYFromShell(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{   
    /* return the y in the child's unit type; for children of shell, return
     * the parent's y relative to the origin, in pixels */

    Widget parent = XtParent(wid);

    if (XtIsShell(parent)) {   
	/* at the moment menuShell doesn't reset x,y values to 0, so 
	** we'll have them counted twice if we use XtTranslateCoords
	*/
        *value = (XtArgVal) parent->core.y;
    } else {
	*value = (XtArgVal) wid->core.y ;
	_XmFromVerticalPixels(wid,  resource_offset, value);
    }
}



/*********************************************************************
 *
 * ClassInitialize
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   _XmRegisterConverters();   /* Register Library Conversion Rtnes */
   _XmRegisterPixmapConverters();

   _XmInitializeExtensions();
   baseClassExtRec.record_type = XmQmotif;
}


static CompositeClassExtension 
#ifdef _NO_PROTO
FindClassExtension( widget_class )
        WidgetClass widget_class ;
#else
FindClassExtension(
        WidgetClass widget_class )
#endif /* _NO_PROTO */
{
    CompositeClassExtension ext;
    for (ext = (CompositeClassExtension)((CompositeWidgetClass)widget_class)
	       ->composite_class.extension;
	 ext != NULL && ext->record_type != NULLQUARK;
	 ext = (CompositeClassExtension)ext->next_extension);

    if (ext != NULL) {
	if (  ext->version == XtCompositeExtensionVersion
	      && ext->record_size == sizeof(CompositeClassExtensionRec)) {
	    /* continue */
	} else {
	    String params[1];
	    Cardinal num_params = 1;
	    params[0] = widget_class->core_class.class_name;
	    XtErrorMsg( "invalidExtension", "ManagerClassPartInitialize",
		        "XmToolkitError",
		 _XmMsgManager_0000,
		 params, &num_params);
	}
    }
    return ext;
}



/**********************************************************************
 *
 *  BuildManagerResources
 *	Build up the manager's synthetic and constraint synthetic
 *	resource processing list by combining the super classes with 
 *	this class.
 *
 **********************************************************************/
static void 
#ifdef _NO_PROTO
BuildManagerResources( c )
        WidgetClass c ;
#else
BuildManagerResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
    XmManagerWidgetClass wc = (XmManagerWidgetClass) c ;
    XmManagerWidgetClass sc;

    sc = (XmManagerWidgetClass) wc->core_class.superclass;

    _XmInitializeSyntheticResources(wc->manager_class.syn_resources,
				    wc->manager_class.num_syn_resources);

    _XmInitializeSyntheticResources(
			wc->manager_class.syn_constraint_resources,
			wc->manager_class.num_syn_constraint_resources);

    if (sc == (XmManagerWidgetClass) constraintWidgetClass) return;
    
    _XmBuildResources (&(wc->manager_class.syn_resources),
		       &(wc->manager_class.num_syn_resources),
		       sc->manager_class.syn_resources,
		       sc->manager_class.num_syn_resources);

    _XmBuildResources (&(wc->manager_class.syn_constraint_resources),
		       &(wc->manager_class.num_syn_constraint_resources),
		       sc->manager_class.syn_constraint_resources,
		       sc->manager_class.num_syn_constraint_resources);
}



/*********************************************************************
 *
 * ClassPartInitialize
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
    static Boolean first_time = TRUE;
    XmManagerWidgetClass mw = (XmManagerWidgetClass) wc;
    XmManagerWidgetClass super = (XmManagerWidgetClass)
	                        mw->core_class.superclass;
    CompositeClassExtension ext = FindClassExtension(wc);
    XmManagerClassExt *mext = (XmManagerClassExt *)
	_XmGetClassExtensionPtr( (XmGenericClassExt *)
				&(mw->manager_class.extension), NULLQUARK) ;
    
    if (ext == NULL) {
	XtPointer *extP
	    = &((CompositeWidgetClass)wc)->composite_class.extension;
	ext = XtNew(CompositeClassExtensionRec);
	memcpy( ext, FindClassExtension(wc->core_class.superclass),
	       sizeof(CompositeClassExtensionRec));
	ext->next_extension = *extP;
	*extP = (XtPointer)ext;
    }
    
    if (*mext) {
	if ((*mext)->traversal_children == XmInheritTraversalChildrenProc) {
	    XmManagerClassExt *smext = (XmManagerClassExt *)
		_XmGetClassExtensionPtr( (XmGenericClassExt *)
			&(super->manager_class.extension), NULLQUARK) ;
	    (*mext)->traversal_children = (*smext)->traversal_children ;
	}
    } else {
	*mext = (XmManagerClassExt) XtCalloc(1, sizeof(XmManagerClassExtRec)) ;
	(*mext)->record_type     = NULLQUARK ;
	(*mext)->version = XmManagerClassExtVersion ;
	(*mext)->record_size     = sizeof( XmManagerClassExtRec) ;
    }
    
    
    /* deal with inheritance */
    if (mw->manager_class.translations == XtInheritTranslations)
	mw->manager_class.translations = super->manager_class.translations;
    else if (mw->manager_class.translations)
	mw->manager_class.translations = (String)
	    XtParseTranslationTable(mw->manager_class.translations);
    
    if (mw->manager_class.parent_process == XmInheritParentProcess)
	mw->manager_class.parent_process = 
	    super->manager_class.parent_process;
    
    
    _XmBaseClassPartInitialize(wc);
    
    _XmFastSubclassInit (wc, XmMANAGER_BIT);
    
    
    /* Carry this ugly non portable code that deal with Xt internals... */
    if (first_time) {
        _XmSortResourceList((XrmResource **)
			    xmManagerClassRec.core_class.resources,
			    xmManagerClassRec.core_class.num_resources);
        first_time = FALSE;
    }
    
    /* synthetic resource management */
    BuildManagerResources((WidgetClass) wc);
    
}




/*********************************************************************
 *
 *  GetBackgroundGC
 *	Get the graphics context used by children for erasing there
 *	highlight border.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
GetBackgroundGC( mw )
        XmManagerWidget mw ;
#else
GetBackgroundGC(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = mw->core.background_pixel;
   values.background = mw->manager.foreground;

   if ((mw->core.background_pixmap != None) &&
       (mw->core.background_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = mw->core.background_pixmap;
   }

   mw->manager.background_GC = XtGetGC ((Widget) mw, valueMask, &values);
}



   
/************************************************************************
 *
 *  GetHighlightGC
 *     Get the graphics context used for drawing the border.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetHighlightGC( mw )
        XmManagerWidget mw ;
#else
GetHighlightGC(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = mw->manager.highlight_color;
   values.background = mw->core.background_pixel;

   if ((mw->manager.highlight_pixmap != None) &&
       (mw->manager.highlight_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = mw->manager.highlight_pixmap;
   }

   mw->manager.highlight_GC = XtGetGC ((Widget) mw, valueMask, &values);
}



   
/************************************************************************
 *
 *  GetTopShadowGC
 *     Get the graphics context used for drawing the top shadow.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetTopShadowGC( mw )
        XmManagerWidget mw ;
#else
GetTopShadowGC(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = mw->manager.top_shadow_color;
   values.background = mw->manager.foreground;
    
   if ((mw->manager.top_shadow_pixmap != None) &&
       (mw->manager.top_shadow_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = mw->manager.top_shadow_pixmap;
   }

   mw->manager.top_shadow_GC = XtGetGC ((Widget) mw, valueMask, &values);
}




/************************************************************************
 *
 *  GetBottomShadowGC
 *     Get the graphics context used for drawing the top shadow.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetBottomShadowGC( mw )
        XmManagerWidget mw ;
#else
GetBottomShadowGC(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = mw->manager.bottom_shadow_color;
   values.background = mw->manager.foreground;
    
   if ((mw->manager.bottom_shadow_pixmap != None) &&
       (mw->manager.bottom_shadow_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = mw->manager.bottom_shadow_pixmap;
   }

   mw->manager.bottom_shadow_GC = XtGetGC ((Widget) mw, valueMask, &values);
}




/************************************************************************
 *
 *  Initialize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( request, new_w, args, num_args )
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
            XmManagerWidget mw = (XmManagerWidget) new_w ;
            XtTranslations translations ;
            char * name ;

   /*  Initialize manager and composite instance data  */

   mw->manager.selected_gadget = NULL;
   mw->manager.highlighted_widget = NULL;
   mw->manager.event_handler_added = False;
   mw->manager.active_child = NULL;
   mw->manager.keyboard_list = NULL;
   mw->manager.num_keyboard_entries = 0;
   mw->manager.size_keyboard_list = 0;


   /*  Checking for valid stringDirection */
   if(    (mw->manager.string_direction == XmSTRING_DIRECTION_DEFAULT)
       || !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
                        mw->manager.string_direction, (Widget) mw)    )
    {   
        if (XmIsManager(mw->core.parent))
            mw->manager.string_direction = 
            ((XmManagerWidget)(mw->core.parent))->manager.string_direction;
        else
            mw->manager.string_direction = XmSTRING_DIRECTION_L_TO_R;
    }

   translations = (XtTranslations) ((XmManagerClassRec *)XtClass( mw))
                                      ->manager_class.translations ;
   if(    mw->manager.traversal_on
       && translations  &&  mw->core.tm.translations
       && !XmIsRowColumn( mw)    )
   {   
       /*  If this widget is requesting traversal then augment its
       * translation table with some additional events.
       * We will only augment translations for a widget which
       * already has some translations defined; this allows widgets
       * which want to set different translations (i.e. menus) to
       * it at a later point in time.
       * We do not override RowColumn and Label subclasses, these
       * are handled by those classes.
       */
       XtOverrideTranslations( (Widget) mw, translations) ;
       } 

   XtInsertEventHandler( (Widget) mw, (KeyPressMask | KeyReleaseMask), FALSE,
                            _XmVirtKeysHandler, NULL, XtListHead) ;

   if(    (mw->manager.navigation_type != XmDYNAMIC_DEFAULT_TAB_GROUP)
       && !XmRepTypeValidValue( XmRID_NAVIGATION_TYPE, 
                        mw->manager.navigation_type, (Widget) mw)    )
   {   mw->manager.navigation_type = XmNONE ;
       } 
   _XmNavigInitialize( request, new_w, args, num_args);

   /*  Verify resource data  */

   if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE, mw->manager.unit_type,
                                          (Widget) mw)    )
   {
      mw->manager.unit_type = XmPIXELS;
   }

   /*  Convert the fields from unit values to pixel values  */

   _XmManagerImportArgs( (Widget) mw, args, num_args);

   /*  See if the background pixmap name was set by the converter.  */
   /*  If so, generate the background pixmap and put into the       */
   /*  associated core field.                                       */

   name = _XmGetBGPixmapName();
   if (name != NULL)
   {
      mw->core.background_pixmap = 
         XmGetPixmapByDepth (XtScreen (mw), name,
		mw->manager.foreground, mw->core.background_pixel,
		mw->core.depth);
      _XmClearBGPixmapName();
   }


   /*  Get the shadow drawing GC's  */

   GetBackgroundGC (mw);
   GetTopShadowGC (mw);
   GetBottomShadowGC (mw);
   GetHighlightGC (mw);


   /* Copy accelerator widget from parent or set to NULL.
    */
   {
      XmManagerWidget p = (XmManagerWidget) XtParent(mw);

      if (XmIsManager(p) && p->manager.accelerator_widget)
         mw->manager.accelerator_widget = p->manager.accelerator_widget;
      else
         mw->manager.accelerator_widget = NULL;
   }
}




/*************************************************************************
 *
 *  Realize
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

    /*	Make sure height and width are not zero.
    */
   if (!XtWidth(w)) XtWidth(w) = 1 ;
   if (!XtHeight(w)) XtHeight(w) = 1 ;
    
   valueMask |= CWBitGravity | CWDontPropagate;
   attributes->bit_gravity = NorthWestGravity;
   attributes->do_not_propagate_mask =
      ButtonPressMask | ButtonReleaseMask |
      KeyPressMask | KeyReleaseMask | PointerMotionMask;

   XtCreateWindow (w, InputOutput, CopyFromParent, valueMask, attributes);
}



/************************************************************************
 *
 *  Destroy
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
   XmManagerWidget	mw = (XmManagerWidget) w;

   _XmNavigDestroy(w);

   XtReleaseGC (w, mw->manager.background_GC);
   XtReleaseGC (w, mw->manager.top_shadow_GC);
   XtReleaseGC (w, mw->manager.bottom_shadow_GC);
   XtReleaseGC (w, mw->manager.highlight_GC);
}




/************************************************************************
 *
 *  CallVisualChange
 *	Call the VisualChange class functions of a widget in super class
 *	to sub class order.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
CallVisualChange( child, w_class, cur, new_w )
        XmGadget child ;
        XmGadgetClass w_class ;
        Widget cur ;
        Widget new_w ;
#else
CallVisualChange(
        XmGadget child,
        XmGadgetClass w_class,
        Widget cur,
        Widget new_w )
#endif /* _NO_PROTO */
{
   Boolean redisplay = False;

   if (w_class->rect_class.superclass != (WidgetClass) &xmGadgetClassRec)
      redisplay = 
         CallVisualChange (child, 
			   (XmGadgetClass) w_class->rect_class.superclass, 
			   cur, new_w);

   if (w_class->gadget_class.visual_change != NULL)
       redisplay |= (*(w_class->gadget_class.visual_change))( (Widget) child, 
							   cur, new_w);
   return (redisplay);
}




/************************************************************************
 *
 *  ChildVisualChange
 *	Loop through the child set of new and for any gadget that has
 *	a non-NULL visual_change class function, call the function.
 *      The class function will return True if the gadget needs to
 *	redraw.
 *	Now also adds support for XmPushButton subclasses which are
 *	interested in our background and possibly in other visual
 *	changes.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
ChildVisualChange( cur, new_w )
        XmManagerWidget cur ;
        XmManagerWidget new_w ;
#else
ChildVisualChange(
        XmManagerWidget cur,
        XmManagerWidget new_w )
#endif /* _NO_PROTO */
{
   register int i;
   XmGadget child;
   WidgetClass w_class;
   Boolean redisplay = False;

   for (i = 0; i < new_w->composite.num_children; i++)
   {
      child = (XmGadget) new_w->composite.children[i];

      if (XmIsGadget(child))
      {
         w_class = XtClass (child);

         if (CallVisualChange (child, (XmGadgetClass) w_class, 
			       (Widget) cur, (Widget) new_w))
            redisplay = True;
      }
      /* The current implementation of Buttons grabs the button's parent's
      ** background color to draw its unhighlighting; this is
      ** done at several points in the code, including Redisplay. However,
      ** the button isn't properly notified of changes that occur in that 
      ** color. In the absence of a formal mechanism for transferring this
      ** information, we force a redraw here. 
      ** No change to return value.
      */
      else if ((XtIsRealized(child)) && IsButton(child))
		XClearArea(XtDisplay(child), XtWindow(child),
			0,0,0,0,TRUE);
   }

   return (redisplay);
}



/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( current, request, new_w, args, num_args )
        Widget current ;
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget current,
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    Boolean returnFlag = False;
    Boolean visualFlag = False;
    XmManagerWidget curmw = (XmManagerWidget) current;
    XmManagerWidget newmw = (XmManagerWidget) new_w;
    char *name ;

    /*  Process the change in values */
   
    /* If traversal has been turned on, then augment the translations
    *    of the new widget.
    */
    if(    newmw->manager.traversal_on
        && (newmw->manager.traversal_on != curmw->manager.traversal_on)
        && newmw->core.tm.translations
        && ((XmManagerClassRec *) XtClass( newmw))
                                     ->manager_class.translations    )
    {   
        XtOverrideTranslations( (Widget) newmw, (XtTranslations) 
                           ((XmManagerClassRec *) XtClass( newmw))
                                        ->manager_class.translations) ;
        }
    if(    newmw->manager.initial_focus != curmw->manager.initial_focus    )
      {
	_XmSetInitialOfTabGroup( (Widget) newmw,
				newmw->manager.initial_focus) ;
      }
    if( curmw->manager.navigation_type != newmw->manager.navigation_type   )
      {
	if(    !XmRepTypeValidValue( XmRID_NAVIGATION_TYPE, 
			 newmw->manager.navigation_type, (Widget) newmw)    )
	  {
	    newmw->manager.navigation_type = curmw->manager.navigation_type ;
	  } 
      }
    returnFlag = _XmNavigSetValues(current, request, new_w, args, num_args);

    /*  Validate changed data.  */

    if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE, newmw->manager.unit_type,
                                                        (Widget) newmw)    )
    {
       newmw->manager.unit_type = curmw->manager.unit_type;
    }

    /*  Checking for valid stringDirection */
    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
                       newmw->manager.string_direction, (Widget) newmw)    )
    {
        newmw->manager.string_direction = curmw->manager.string_direction ;
    }

   /*  Convert the necessary fields from unit values to pixel values  */

   _XmManagerImportArgs( (Widget) newmw, args, num_args);

   /* Need to check to see if type converter for XmNbackgroundPixmap
    * was called, for XtVaTypedArg interface to SetValues.
    */
   name = _XmGetBGPixmapName();
   if (name != NULL)
     {
       Mask    window_mask;
       XSetWindowAttributes attributes;

       window_mask = CWBackPixmap;
       attributes.background_pixmap =
       newmw->core.background_pixmap = XmGetPixmapByDepth( XtScreen( newmw),
                 name, newmw->manager.foreground, newmw->core.background_pixel,
                                                            newmw->core.depth);
						 
       XChangeWindowAttributes( XtDisplay(newmw), XtWindow(newmw), window_mask,
		 &attributes);

       _XmClearBGPixmapName();
     }
   /*  If either of the background, shadow, or highlight colors or  */
   /*  pixmaps have changed, destroy and recreate the gc's.         */

   if (curmw->core.background_pixel != newmw->core.background_pixel ||
       curmw->core.background_pixmap != newmw->core.background_pixmap)
   {
      XtReleaseGC ( (Widget) newmw, newmw->manager.background_GC);
      GetBackgroundGC (newmw);
      returnFlag = True;
      visualFlag = True;
   }

   if (curmw->manager.top_shadow_color != newmw->manager.top_shadow_color ||
       curmw->manager.top_shadow_pixmap != newmw->manager.top_shadow_pixmap)
   {
      XtReleaseGC ((Widget) newmw, newmw->manager.top_shadow_GC);
      GetTopShadowGC (newmw);
      returnFlag = True;
      visualFlag = True;
   }

   if (curmw->manager.bottom_shadow_color != 
          newmw->manager.bottom_shadow_color ||
       curmw->manager.bottom_shadow_pixmap != 
          newmw->manager.bottom_shadow_pixmap)
   {
      XtReleaseGC ((Widget) newmw, newmw->manager.bottom_shadow_GC);
      GetBottomShadowGC (newmw);
      returnFlag = True;
      visualFlag = True;
   }

   if (curmw->manager.highlight_color != newmw->manager.highlight_color ||
       curmw->manager.highlight_pixmap != newmw->manager.highlight_pixmap)
   {
      XtReleaseGC ((Widget) newmw, newmw->manager.highlight_GC);
      GetHighlightGC (newmw);
      returnFlag = True;
      visualFlag = True;
   }

   if (curmw->manager.foreground != newmw->manager.foreground       ||
       curmw->core.background_pixel != newmw->core.background_pixel ||
       curmw->core.background_pixmap != newmw->core.background_pixmap)
      visualFlag = True;


   /*  Inform children of possible visual changes  */

   if (visualFlag)
      if (ChildVisualChange (curmw, newmw))
         returnFlag = True;


   /*  Return flag to indicate if redraw is needed.  */

   return (returnFlag);
}




   
/*********************************************************************
 *
 * InsertChild
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
InsertChild( child )
        Widget child ;
#else
InsertChild(
        Widget child )
#endif /* _NO_PROTO */
{
    CompositeClassRec *cc = (CompositeClassRec *) compositeWidgetClass;

    if (!XtIsRectObj(child))
	return;
	
    (*(cc->composite_class.insert_child)) (child);
}

/*********************************************************************
 *
 * DeleteChild
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
DeleteChild( child )
        Widget child ;
#else
DeleteChild(
        Widget child )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) child->core.parent;
    Widget tab_group ;
    CompositeClassRec *cc = (CompositeClassRec *) compositeWidgetClass;
    
    if (!XtIsRectObj(child))
	return;
    
    if (mw->manager.selected_gadget == (XmGadget) child)
	mw->manager.selected_gadget = NULL;
    
    if(    mw->manager.initial_focus == child    )
	{
	    mw->manager.initial_focus = NULL ;
	}
    if(    mw->manager.active_child == child    )
	{
	    mw->manager.active_child = NULL;
	}
    if(    (tab_group = XmGetTabGroup( child))
       &&  (tab_group != (Widget) mw)
       &&  XmIsManager( tab_group)
       &&  (((XmManagerWidget) tab_group)->manager.active_child == child) )
	{
	    ((XmManagerWidget) tab_group)->manager.active_child = NULL ;
	}
    (*(cc->composite_class.delete_child)) (child);
}




/************************************************************************
 *
 *  ManagerMotion
 *	This function handles the generation of motion, enter, and leave
 *	window events for gadgets and the dispatching of these events to
 *	the gadgets.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ManagerMotion( wid, closure, event, cont )
        Widget wid ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
ManagerMotion(
        Widget wid,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
            XmManagerWidget mw = (XmManagerWidget) wid ;
   XmGadget gadget;
   XmGadget oldGadget;


   /*  Event on the managers window and not propagated from a child  */

   /* ManagerMotion() creates misleading Enter/Leave events.  A race condition
    * exists such that it's possible that when ManagerMotion() is called, the
    * manager does not yet have the focus.  Dropping the Enter on the floor
    * caused ManagerMotion() to translate the first subsequent motion event
    * into an enter to dispatch to the gadget.  Subsequently button gadget 
    * (un)highlighting on enter/leave was unreliable.  This problem requires 
    * additional investigation. 
    * The quick fix, currently, is for ManagerEnter()
    * and ManagerLeave() to use the event whether or not the manager has the 
    * focus.  
    * In addition, in dispatching enter/leaves to gadgets here in this 
    * routine, ManagerMotion(), bear in mind that we are passing a 
    * XPointerMovedEvent and should probably be creating a synthethic 
    * XCrossingEvent instead.
    *
    * if ((event->subwindow != 0) || !mw->manager.has_focus)
    */
   if (event->xmotion.subwindow != 0) 
      return;

   gadget = _XmInputForGadget((Widget) mw, event->xmotion.x, 
			      event->xmotion.y);
   oldGadget = (XmGadget) mw->manager.highlighted_widget;


   /*  Dispatch motion events to the child  */

   if (gadget != NULL)
   {
      if (gadget->gadget.event_mask & XmMOTION_EVENT)
         _XmDispatchGadgetInput((Widget) gadget, event, XmMOTION_EVENT);
   }


   /*  Check for and process a leave window condition  */

   if (oldGadget != NULL && gadget != oldGadget)
   {
      if (oldGadget->gadget.event_mask & XmLEAVE_EVENT)
         _XmDispatchGadgetInput( (Widget) oldGadget, event, XmLEAVE_EVENT);

      mw->manager.highlighted_widget = NULL;
   }


   /*  Check for and process an enter window condition  */

   if (gadget != NULL && gadget != oldGadget)
   {
      if (gadget->gadget.event_mask & XmENTER_EVENT)
      {
         _XmDispatchGadgetInput( (Widget) gadget, event, XmENTER_EVENT);
         mw->manager.highlighted_widget = (Widget) gadget;
      }
      else
         mw->manager.highlighted_widget = NULL;
   }
}




/************************************************************************
 *
 *  ManagerEnter
 *	This function handles the generation of motion and enter window
 *	events for gadgets and the dispatching of these events to the
 *	gadgets.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ManagerEnter( wid, closure, event, cont )
        Widget wid ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
ManagerEnter(
        Widget wid,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
            XmManagerWidget mw = (XmManagerWidget) wid ;
   XmGadget gadget;

   /* See ManagerMotion()
    * if (!(mw->manager.has_focus = (Boolean) event->xcrossing.focus)) 
    *    return;
    */

   /*
    * call the traversal action in order to synch things up. This
    * should be cleaned up into a single module |||
    */
   _XmManagerEnter((Widget) mw, event, NULL, NULL);

   gadget = _XmInputForGadget( (Widget) mw, event->xcrossing.x,
                                           event->xcrossing.y);
   /*  Dispatch motion and enter events to the child  */

   if (gadget != NULL)
   {
      if (gadget->gadget.event_mask & XmMOTION_EVENT)
         _XmDispatchGadgetInput( (Widget) gadget, event, XmMOTION_EVENT);

      if (gadget->gadget.event_mask & XmENTER_EVENT)
      {
         _XmDispatchGadgetInput( (Widget) gadget, event, XmENTER_EVENT);
         mw->manager.highlighted_widget = (Widget) gadget;
      }
      else
         mw->manager.highlighted_widget = NULL;

   }
}




/************************************************************************
 *
 *  ManagerLeave
 *	This function handles the generation of leave window events for
 *	gadgets and the dispatching of these events to the gadgets.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ManagerLeave( wid, closure, event, cont )
        Widget wid ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
ManagerLeave(
        Widget wid,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
            XmManagerWidget mw = (XmManagerWidget) wid ;
   XmGadget oldGadget;

   /* See ManagerMotion()
    * if (!(mw->manager.has_focus = (Boolean) event->xcrossing.focus)) 
    *    return;
    */

   oldGadget = (XmGadget) mw->manager.highlighted_widget;

   if (oldGadget != NULL)
   {
      if (oldGadget->gadget.event_mask & XmLEAVE_EVENT)
         _XmDispatchGadgetInput( (Widget) oldGadget, event, XmLEAVE_EVENT);
      mw->manager.highlighted_widget = NULL;
   }
   /*
    * call the traversal action in order to synch things up. This
    * should be cleaned up into a single module |||
    */
   _XmManagerLeave( (Widget) mw, event, NULL, NULL);

}



/************************************************************************
 *
 *  AddMotionHandlers
 *	Add the event handlers necessary to synthisize motion events
 *	for gadgets.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
AddMotionHandlers( mw )
        XmManagerWidget mw ;
#else
AddMotionHandlers(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   mw->manager.event_handler_added = True;

   XtAddEventHandler ((Widget) mw, PointerMotionMask, False, 
		      ManagerMotion, NULL);
   XtAddEventHandler ((Widget) mw, EnterWindowMask, False, 
		      ManagerEnter, NULL);
   XtAddEventHandler ((Widget) mw, LeaveWindowMask, False, 
		      ManagerLeave, NULL);
}



static void
#ifdef _NO_PROTO
DoMagicBBCompatibilityStuff( wid)
        Widget wid ;
#else
DoMagicBBCompatibilityStuff(
        Widget wid)
#endif /* _NO_PROTO */
{
static char _XmBulletinB_defaultAccelerators[] = "\
\043override\n\
~s ~m ~a <Key>Return:BulletinBoardReturn()\n\
<Key>osfActivate:BulletinBoardReturn()\n\
<Key>osfCancel:BulletinBoardCancel()";

if(    !_XmIsStandardMotifWidgetClass( XtClass( wid))    )
  {
    /* For backwards compatibility with subclasses of Motif, maintain
     * this bogus accelerator *?!O*%X* (behavior) for these widgets.
     */
    Widget ancestor = wid ;

    while(    ancestor && !XtIsShell( ancestor)    )
      {
        if(    XmIsBulletinBoard( ancestor)    )
          {
            if(    !XmIsText( wid)
               &&  !XmIsTextField( wid)
               &&  !XmIsList( wid)    )
              {
                if(    !ancestor->core.accelerators    )
                  {
                    Arg arg[1] ;
                    XtAccelerators parsed = XtParseAcceleratorTable(
                                           _XmBulletinB_defaultAccelerators) ;
                    XtSetArg( arg[0], XmNaccelerators, parsed) ;
                    XtSetValues( ancestor, arg, 1) ;
                  }
                XtInstallAccelerators( wid, ancestor) ;
              }
            break ;
          }
        ancestor = XtParent( ancestor) ;
      }
  }
}


/************************************************************************
 *
 *  ConstraintInitialize
 *	The constraint destroy procedure checks to see if a gadget
 *	child is being destroyed.  If so, the managers motion processing
 *	event handlers are checked to see if they need to be removed.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ConstraintInitialize( request, new_w, args, num_args )
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal * num_args ;
#else
ConstraintInitialize(
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal * num_args )
#endif /* _NO_PROTO */
{
   XmGadget g;
   XmManagerWidget parent;

   if (!XtIsRectObj(new_w)) return;

   parent = (XmManagerWidget) new_w->core.parent;

   if (XmIsGadget (new_w))
   {
      g = (XmGadget) new_w;


      if ((g->gadget.event_mask & 
           (XmENTER_EVENT | XmLEAVE_EVENT | XmMOTION_EVENT)) &&
           parent->manager.event_handler_added == False)
         AddMotionHandlers (parent);
   }
   else if (XtIsWidget(new_w))
     {
       if (parent->manager.accelerator_widget)
         {
           XtInstallAccelerators (new_w, parent->manager.accelerator_widget);
         }
       else
         {
           DoMagicBBCompatibilityStuff( new_w) ;
         }
     }
   else
     {
           /* non-widget non-gadget RectObj */
     }
}





/************************************************************************
 *
 *  CheckRemoveMotionHandlers
 *	This function loops through the child set checking each gadget 
 *	to see if the need motion events or not.  If no gadget's need
 *	motion events and the motion event handlers have been added,
 *	then remove the event handlers.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CheckRemoveMotionHandlers( mw )
        XmManagerWidget mw ;
#else
CheckRemoveMotionHandlers(
        XmManagerWidget mw )
#endif /* _NO_PROTO */
{
   register int i;
   register Widget child;


   /*  If there are any gadgets which need motion events, return.  */

   if (!mw->core.being_destroyed)
   {
      for (i = 0; i < mw->composite.num_children; i++)
      {
         child = mw->composite.children[i];
   
         if (XmIsGadget(child))
         {
            if (((XmGadget) child)->gadget.event_mask & 
                (XmENTER_EVENT | XmLEAVE_EVENT | XmMOTION_EVENT))
            return;
         }
      }
   }


   /*  Remove the motion event handlers  */

   mw->manager.event_handler_added = False;

   XtRemoveEventHandler ((Widget) mw, PointerMotionMask, False, 
			 ManagerMotion, NULL);
   XtRemoveEventHandler ((Widget) mw, EnterWindowMask, False, 
			 ManagerEnter, NULL);
   XtRemoveEventHandler ((Widget) mw, LeaveWindowMask, False, 
			 ManagerLeave, NULL);
}




/************************************************************************
 *
 *  ConstraintDestroy
 *	The constraint destroy procedure checks to see if a gadget
 *	child is being destroyed.  If so, the managers motion processing
 *	event handlers are checked to see if they need to be removed.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ConstraintDestroy( w )
        Widget w ;
#else
ConstraintDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
   XmGadget g;
   XmManagerWidget parent;

   if (!XtIsRectObj(w)) return;

   if (XmIsGadget (w))
   {
      g = (XmGadget) w;
      parent = (XmManagerWidget) w->core.parent;

      if (g->gadget.event_mask & 
          (XmENTER_EVENT | XmLEAVE_EVENT | XmMOTION_EVENT))
         CheckRemoveMotionHandlers (parent);

      if (parent->manager.highlighted_widget == w)
         parent->manager.highlighted_widget = NULL;

      if (parent->manager.selected_gadget == g)
         parent->manager.selected_gadget = NULL;
   }
}



/************************************************************************
 *
 *  ConstraintSetValues
 *	Make sure the managers event handler is set appropriately for
 *	gadget event handling.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
ConstraintSetValues( current, request, new_w, args, num_args )
        Widget current ;
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal * num_args ;
#else
ConstraintSetValues(
        Widget current,
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal * num_args )
#endif /* _NO_PROTO */
{
   XmGadget currentg, newg;
   XmManagerWidget parent;
   unsigned int motion_events;

   if (!XtIsRectObj(new_w)) return(FALSE);

   /*  If the child is a gadget and its event mask has changed with  */
   /*  respect to the event types which need motion events on the    */
   /*  parent.                                                       */

   if (XmIsGadget (new_w))
   {
      currentg = (XmGadget) current;
      newg = (XmGadget) new_w;
      parent = (XmManagerWidget) new_w->core.parent;

      motion_events = XmENTER_EVENT | XmLEAVE_EVENT | XmMOTION_EVENT;

      if ((newg->gadget.event_mask & motion_events) !=
          (currentg->gadget.event_mask & motion_events))
      {
         if ((newg->gadget.event_mask & motion_events) &&
             parent->manager.event_handler_added == False)
            AddMotionHandlers (parent);

         if ((~(newg->gadget.event_mask & motion_events)) &&
             parent->manager.event_handler_added == True)
            CheckRemoveMotionHandlers (parent);
      }
   }

   return (False);
}

/****************************************************************/
static Boolean 
#ifdef _NO_PROTO
ManagerParentProcess( widget, data )
        Widget widget ;
        XmParentProcessData data ;
#else
ManagerParentProcess(
        Widget widget,
        XmParentProcessData data )
#endif /* _NO_PROTO */
{

    return( _XmParentProcess( XtParent( widget), data)) ;
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
    if(    wid->core.sensitive
       &&  wid->core.ancestor_sensitive
       &&  ((XmManagerWidget) wid)->manager.traversal_on    )
	{ 
	    XmNavigationType nav_type = ((XmManagerWidget) wid)
		->manager.navigation_type ;
	    if(    (nav_type == XmSTICKY_TAB_GROUP)
	       ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	       ||  (    (nav_type == XmTAB_GROUP)
		    &&  !_XmShellIsExclusive( wid))    )
		{
		    return XmDESCENDANTS_TAB_NAVIGABLE ;
		}
	    return XmDESCENDANTS_NAVIGABLE ;
	}
    return XmNOT_NAVIGABLE ;
}



/****************************************************************
 ****************************************************************
 **
 ** External functions, both _Xm and Xm.
 ** First come the action procs and then the other external entry points.
 **
 ****************************************************************
 ****************************************************************/


void 
#ifdef _NO_PROTO
_XmGadgetTraversePrevTabGroup( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraversePrevTabGroup(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
#ifdef CDE_TAB
   if ((XmIsPushButtonGadget(ref_wid) || XmIsArrowButtonGadget(ref_wid)) &&
      !_XmTraverseWillWrap(ref_wid, XmTRAVERSE_PREV))  {
      Boolean button_tab = False;
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject(ref_wid)), "enableButtonTab", &button_tab, NULL);
      if (button_tab)
           _XmMgrTraversal (ref_wid, XmTRAVERSE_PREV);
      else
           _XmMgrTraversal( ref_wid, XmTRAVERSE_PREV_TAB_GROUP) ;
   }
   else
#endif /* CDE_TAB */
  _XmMgrTraversal( ref_wid, XmTRAVERSE_PREV_TAB_GROUP) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseNextTabGroup( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseNextTabGroup(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
#ifdef CDE_TAB
   if ((XmIsPushButtonGadget(ref_wid) || XmIsArrowButtonGadget(ref_wid)) &&
      !_XmTraverseWillWrap(ref_wid, XmTRAVERSE_NEXT))  {
     Boolean button_tab = False;
     XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject(ref_wid)), "enableButtonTab", &button_tab, NULL);
     if (button_tab)
         _XmMgrTraversal (ref_wid, XmTRAVERSE_NEXT);
     else
         _XmMgrTraversal( ref_wid, XmTRAVERSE_NEXT_TAB_GROUP) ;
   }
   else
#endif /* CDE_TAB */
  _XmMgrTraversal( ref_wid, XmTRAVERSE_NEXT_TAB_GROUP) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseLeft( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseLeft(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_LEFT) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseRight( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseRight(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_RIGHT) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseUp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseUp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_UP) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseDown( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseDown(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_DOWN) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseNext( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseNext(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_NEXT) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraversePrev( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraversePrev(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_PREV) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetTraverseHome( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetTraverseHome(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  Widget ref_wid = ((XmManagerWidget) wid)->manager.active_child ;

  if(    ref_wid == NULL    )
    {
      ref_wid = wid ;
    }
  _XmMgrTraversal( ref_wid, XmTRAVERSE_HOME) ;
}

void 
#ifdef _NO_PROTO
_XmGadgetSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
   XmManagerWidget mw = (XmManagerWidget) wid ;
            Widget child ;

    if(    _XmGetFocusPolicy( (Widget) mw) == XmEXPLICIT    )
    {   
        child = mw->manager.active_child ;
        if(    child  &&  !XmIsGadget( child)    )
        {   child = NULL ;
            } 
        }
    else /* FocusPolicy == XmPOINTER */
    {   child = (Widget) _XmInputForGadget( (Widget) mw, event->xkey.x, event->xkey.y) ;
        } 
    if(    child
        && (((XmGadgetClass)XtClass( child))->gadget_class.arm_and_activate)  )
    {   
        (*(((XmGadgetClass)XtClass( child))->gadget_class.arm_and_activate))(
                                                    child, event, NULL, NULL) ;
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmManagerParentActivate( mw, event, params, num_params )
        Widget mw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerParentActivate( 
        Widget mw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
        Widget child ;
	XmParentInputActionRec  pp_data ;

    pp_data.process_type = XmINPUT_ACTION ;
    pp_data.action = XmPARENT_ACTIVATE ;
    pp_data.event = event ;
    pp_data.params = params ;
    pp_data.num_params = num_params ;

    if(    !_XmParentProcess( mw, (XmParentProcessData) &pp_data)    )
    {   
        if(    _XmGetFocusPolicy( mw) == XmEXPLICIT    )
        {   
            child = ((XmManagerWidget) mw)->manager.active_child ;
            }
        else /* FocusPolicy == XmPOINTER */
        {   child = (Widget) _XmInputForGadget( mw, event->xkey.x,
					       event->xkey.y) ;
            } 

	if(    child
	   && !XmIsPushButtonGadget( child)
	   && !XmIsArrowButtonGadget( child)
	   && !XmIsToggleButtonGadget( child)
	   && !XmIsCascadeButtonGadget( child) )
        {   
            if(    ((XmGadgetClass) XtClass( child))
	       ->gadget_class.arm_and_activate    )
            {   (*(((XmGadgetClass) XtClass( child))->gadget_class
		   .arm_and_activate))( child, event, NULL, NULL) ;
                }
            } 
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmManagerParentCancel( mw, event, params, num_params )
        Widget mw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerParentCancel( 
        Widget mw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
	XmParentInputActionRec  pp_data ;

    pp_data.process_type = XmINPUT_ACTION ;
    pp_data.action = XmPARENT_CANCEL ;
    pp_data.event = event ;
    pp_data.params = params ;
    pp_data.num_params = num_params ;

    _XmParentProcess( mw, (XmParentProcessData) &pp_data) ;
    }

void 
#ifdef _NO_PROTO
_XmGadgetButtonMotion( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetButtonMotion(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmManagerWidget mw = (XmManagerWidget) wid ;
            Widget child ;

    if(    _XmGetFocusPolicy( (Widget) mw) == XmEXPLICIT    )
    {   
        child = mw->manager.active_child ;
        if(    child  &&  !XmIsGadget( child)    )
        {   child = NULL ;
            } 
        }
    else /* FocusPolicy == XmPOINTER */
    {   child = (Widget) _XmInputForGadget( (Widget) mw, event->xmotion.x,
					   event->xmotion.y) ;
        } 
    if(    child    )
    {   _XmDispatchGadgetInput( child, event, XmMOTION_EVENT);
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmGadgetKeyInput( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetKeyInput(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
            XmManagerWidget mw = (XmManagerWidget) wid ;
            Widget child ;

    if(    _XmGetFocusPolicy( (Widget) mw) == XmEXPLICIT    )
    {   
        child = mw->manager.active_child ;
        if(    child  &&  !XmIsGadget( child)    )
        {   child = NULL ;
            } 
        }
    else /* FocusPolicy == XmPOINTER */
	{   
	    child = (Widget) _XmInputForGadget( (Widget) mw, 
					       event->xkey.x, event->xkey.y) ;
        } 
    if(    child    )
    {   _XmDispatchGadgetInput( child, event, XmKEY_EVENT);
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmGadgetArm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetArm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) wid ;
    XmGadget gadget;

    if ((gadget = _XmInputForGadget( (Widget) mw, event->xbutton.x,
				    event->xbutton.y)) != NULL)
    {
	XmProcessTraversal( (Widget) gadget, XmTRAVERSE_CURRENT);
        _XmDispatchGadgetInput( (Widget) gadget, event, XmARM_EVENT);
        mw->manager.selected_gadget = gadget;
    }
    else
      {
        if(    _XmIsNavigable( wid)    )
          {   
            XmProcessTraversal( wid, XmTRAVERSE_CURRENT) ;
          } 
      }

    mw->manager.eligible_for_multi_button_event = NULL;
}

void 
#ifdef _NO_PROTO
_XmGadgetDrag( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetDrag(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) wid ;
    XmGadget gadget;

    if ((gadget = _XmInputForGadget( (Widget) mw, event->xbutton.x,
				    event->xbutton.y)) != NULL)
    {
        _XmDispatchGadgetInput( (Widget) gadget, event, XmBDRAG_EVENT);
        mw->manager.selected_gadget = gadget;
    }

    mw->manager.eligible_for_multi_button_event = NULL;
}

void 
#ifdef _NO_PROTO
_XmGadgetActivate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmManagerWidget mw = (XmManagerWidget) wid ;	
        XmGadget gadget;

    /* we emulate automatic grab with owner_events = false by sending
     * the button up to the button down gadget
     */
    if ((gadget = mw->manager.selected_gadget) != NULL)
    {
        _XmDispatchGadgetInput( (Widget) gadget, event, XmACTIVATE_EVENT);
        mw->manager.selected_gadget = NULL;
        mw->manager.eligible_for_multi_button_event = gadget;
        }
    }

void 
#ifdef _NO_PROTO
_XmManagerHelp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerHelp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmManagerWidget mw = (XmManagerWidget) wid ;
	XmGadget gadget;

	if (!_XmIsEventUnique(event))
	   return;

        if (_XmGetFocusPolicy( (Widget) mw) == XmEXPLICIT)
        {
          if ((gadget = (XmGadget) mw->manager.active_child) != NULL)
             _XmDispatchGadgetInput( (Widget) gadget, event, XmHELP_EVENT);
          else
             _XmSocorro( (Widget) mw, event, NULL, NULL);
        }
        else
        {
	    if ((gadget = (XmGadget) 
		 _XmInputInGadget( (Widget) mw, event->xkey.x, event->xkey.y))
		!= NULL)
               _XmDispatchGadgetInput( (Widget) gadget, event, XmHELP_EVENT);
          else
               _XmSocorro( (Widget) mw, event, NULL, NULL);
        }

	_XmRecordEvent(event);
}


void 
#ifdef _NO_PROTO
_XmGadgetMultiArm( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetMultiArm(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) wid ;	
    XmGadget gadget;

    gadget = _XmInputForGadget( (Widget) mw, event->xbutton.x,
			       event->xbutton.y);
    /*
     * If we're not set up for multi_button events, check to see if the
     * input gadget has changed from the active_child.  This means that the
     * user is quickly clicking between gadgets of this manager widget.  
     * If so, arm the gadget as if it were the first button press.
     */
    if (mw->manager.eligible_for_multi_button_event &&
	((gadget = _XmInputForGadget( (Widget) mw, event->xbutton.x,
				     event->xbutton.y)) ==
	  mw->manager.eligible_for_multi_button_event))
    {
        _XmDispatchGadgetInput( (Widget) gadget, event, XmMULTI_ARM_EVENT);
	    mw->manager.selected_gadget = gadget;
    }
    else
       if (gadget && (gadget != (XmGadget)mw->manager.active_child))
	   _XmGadgetArm( (Widget) mw, event, params, num_params);
       else
	   mw->manager.eligible_for_multi_button_event = NULL;
}

void 
#ifdef _NO_PROTO
_XmGadgetMultiActivate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmGadgetMultiActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmManagerWidget mw = (XmManagerWidget) wid ;	
    XmGadget gadget;

    /*
     * If we're not set up for multi_button events, call _XmGadgetActivate
     * in case we're quickly selecting a new gadget in which it should
     * be activated as if it were the first button press.
     */
    if (mw->manager.eligible_for_multi_button_event &&
	   ((gadget = mw->manager.selected_gadget) ==
	      mw->manager.eligible_for_multi_button_event))
    {
        _XmDispatchGadgetInput((Widget) gadget, event,
		   XmMULTI_ACTIVATE_EVENT);
    }
    else
       _XmGadgetActivate( (Widget) mw, event, params, num_params);
}




/*--- The following routines should probably go in a new Xm.c module ---*/

/**************************************************************************
 *                                                                        *
 * _XmSocorro - Help dispatch function.  Start at the widget help was     *
 *   invoked on, find the first non-null help callback list, and call it. *
 *   -- Called by various widgets across Xm                               *
 *                                                                        *
 *************************************************************************/
void 
#ifdef _NO_PROTO
_XmSocorro( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmSocorro(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmAnyCallbackStruct cb;    

    if (w == NULL) return;

    cb.reason = XmCR_HELP;
    cb.event = event;

    do {
        if ((XtHasCallbacks(w, XmNhelpCallback) == XtCallbackHasSome))
        {
            XtCallCallbacks (w, XmNhelpCallback, &cb);
            return;
        }
        else
            w = XtParent(w);
    }    
    while (w != NULL);
}



/****************************************************************
 *
 * This is the entry point for parent processing.
 *
 ****************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmParentProcess( widget, data )
        Widget widget ;
        XmParentProcessData data ;
#else
_XmParentProcess(
        Widget widget,
        XmParentProcessData data )
#endif /* _NO_PROTO */
{
    XmManagerWidgetClass manClass ;
	    
    manClass = (XmManagerWidgetClass) widget->core.widget_class ;
    
    if(    XmIsManager( widget)
       && manClass->manager_class.parent_process    ) {   
	return( (*manClass->manager_class.parent_process)( widget, data)) ;
    } 
	    
    return( FALSE) ;
}




/************************************************************************
 *
 *  _XmClearShadowType
 *	Clear the right and bottom border area and save 
 *	the old width, height and shadow type.
 *      Used by various subclasses for resize larger situation, where the
 *      inside shadow is not exposed.
 *   Maybe that should be moved in Draw.c, maybe not, since it's a widget API
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmClearShadowType( w, old_width, old_height, old_shadow_thickness, 
		   old_highlight_thickness )
        Widget w ;
        Dimension old_width ;
        Dimension old_height ;
        Dimension old_shadow_thickness ;
        Dimension old_highlight_thickness ;
#else
_XmClearShadowType(
        Widget w,
#if NeedWidePrototypes
        int old_width,
        int old_height,
        int old_shadow_thickness,
        int old_highlight_thickness )
#else
        Dimension old_width,
        Dimension old_height,
        Dimension old_shadow_thickness,
        Dimension old_highlight_thickness )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   if (XtIsRealized(w))
   {
      if (old_width <= w->core.width)
	 XClearArea (XtDisplay (w), XtWindow (w),
	    old_width - old_shadow_thickness - old_highlight_thickness, 0,
	    old_shadow_thickness, old_height - old_highlight_thickness, 
	    False);

      if (old_height <= w->core.height)
	 XClearArea (XtDisplay (w), XtWindow (w),
	    0, old_height - old_shadow_thickness - old_highlight_thickness, 
	    old_width - old_highlight_thickness, old_shadow_thickness, 
	    False);
   }
}



/************************************************************************
 *
 *  _XmDestroyParentCallback
 *     Destroy parent. Used by various dialog subclasses
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmDestroyParentCallback( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
_XmDestroyParentCallback(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
   XtDestroyWidget (XtParent (w));
}




/***********************************************************************
 * Certain widgets, such as those in a menu, would like the application
 * to look non-scraged (i.e. all exposure events have been processed)
 * before invoking a callback which takes a long time to do its thing.
 * This function grabs all exposure events off the queue, and processes
 * them.
 ***********************************************************************/

void 
#ifdef _NO_PROTO
XmUpdateDisplay( w )
        Widget w ;
#else
XmUpdateDisplay(
        Widget w )
#endif /* _NO_PROTO */
{
   XEvent event;
   Display * display = XtDisplay(w);

   XSync (display, 0);

   while (XCheckMaskEvent(display, ExposureMask, &event))
      XtDispatchEvent(&event);
}

