#pragma ident	"@(#)m1.2libs:Xm/Primitive.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include <Xm/BaseClassP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/GadgetP.h>
#include <Xm/ManagerP.h>
#include <Xm/TransltnsP.h>
#include <Xm/VirtKeysP.h>   /* for _XmVirtKeysHandler */
#include <Xm/DrawP.h>
#include "TraversalI.h"
#include "RepTypeI.h"


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ResolveSyntheticOffsets();
static void GetXFromShell() ;
static void GetYFromShell() ;
static void ClassInitialize() ;
static void BuildPrimitiveResources();
static void ClassPartInitialize() ;
static void GetHighlightGC() ;
static void GetTopShadowGC() ;
static void GetBottomShadowGC() ;
static void Initialize() ;
static void Realize() ;
static void Destroy() ;
static Boolean SetValues() ;
static void HighlightBorder() ;
static void UnhighlightBorder() ;
static XmNavigability WidgetNavigable() ;
static void FocusChange() ;
static Boolean IsSubclassOf() ;
#ifdef CDE_TAB 
extern Boolean _XmTraverseWillWrap();
#endif /* CDE_TAB */

#else

static void ResolveSyntheticOffsets(
                        WidgetClass wc, 
                        XmOffsetPtr *ipot, 
                        XmOffsetPtr *cpot) ;
static void GetXFromShell( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void GetYFromShell( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;

static void ClassInitialize( void ) ;
static void BuildPrimitiveResources(
                        WidgetClass c ) ;
static void ClassPartInitialize( 
                        WidgetClass w) ;
static void GetHighlightGC( 
                        XmPrimitiveWidget pw) ;
static void GetTopShadowGC( 
                        XmPrimitiveWidget pw) ;
static void GetBottomShadowGC( 
                        XmPrimitiveWidget pw) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Realize( 
                        register Widget w,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget current,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void HighlightBorder( 
                        Widget w) ;
static void UnhighlightBorder( 
                        Widget w) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;
static void FocusChange( 
                        Widget wid,
                        XmFocusChange change) ;
static Boolean IsSubclassOf( 
                        WidgetClass wc,
                        WidgetClass sc) ;
#ifdef CDE_TAB 
extern Boolean _XmTraverseWillWrap(
			Widget w,
			XmTraversalDirection dir);
#endif /* CDE_TAB */
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#ifdef OSF_v1_2_4
#ifdef ALIGN_SUBCLASS_PARTS
#define _ALIGN(size) (((size) + (sizeof(long)-1)) & ~(sizeof(long)-1))
#else
#define _ALIGN(size) (size)
#endif
#endif /* OSF_v1_2_4 */

/************************************************************************
 *
 *   Default translation table
 *	These translations will be compiled at class initialize.  When
 *	a subclass of primitive is created then these translations will
 *	be used to augment the translations of the subclass IFF
 *	traversal is on.  The SetValues routine will also augment
 *	a subclass's translations table IFF traversal goes from off to on.
 *	Since we are augmenting it should not be a problem when
 *	traversal goes from off to on to off and on again.
 *
 ************************************************************************/

#define defaultTranslations	_XmPrimitive_defaultTranslations


/************************************************************************
 *
 *   Action list.
 *
 *************************************<->***********************************/

static XtActionsRec actions[] =
{
  {"PrimitiveFocusIn",         _XmPrimitiveFocusIn},
  {"PrimitiveFocusOut",        _XmPrimitiveFocusOut},
  {"PrimitiveUnmap",           _XmPrimitiveUnmap},
  {"PrimitiveHelp",            _XmPrimitiveHelp},
  {"PrimitiveEnter",           _XmPrimitiveEnter},
  {"PrimitiveLeave",           _XmPrimitiveLeave},
  {"PrimitiveTraverseLeft",    _XmTraverseLeft},
  {"PrimitiveTraverseRight",   _XmTraverseRight},
  {"PrimitiveTraverseUp",      _XmTraverseUp },
  {"PrimitiveTraverseDown",    _XmTraverseDown },
  {"PrimitiveTraverseNext",    _XmTraverseNext },
  {"PrimitiveTraversePrev",    _XmTraversePrev },
  {"PrimitiveTraverseHome",    _XmTraverseHome },
  {"PrimitiveNextTabGroup",    _XmTraverseNextTabGroup },
  {"PrimitivePrevTabGroup",    _XmTraversePrevTabGroup },
  {"PrimitiveParentActivate",  _XmPrimitiveParentActivate },
  {"PrimitiveParentCancel",    _XmPrimitiveParentCancel },
  {"unmap",                    _XmPrimitiveUnmap},      /* Motif 1.0 BC. */
  {"Help",                     _XmPrimitiveHelp},       /* Motif 1.0 BC. */
  {"enter",                    _XmPrimitiveEnter},      /* Motif 1.0 BC. */
  {"leave",                    _XmPrimitiveLeave},      /* Motif 1.0 BC. */
  {"PrevTabGroup",	       _XmTraversePrevTabGroup},/* Motif 1.0 BC. */
  {"NextTabGroup",	       _XmTraverseNextTabGroup},/* Motif 1.0 BC. */
};


/*****************************************/
/*  Resource definitions for XmPrimitive */

static XtResource resources[] =
{
   {
     XmNunitType, XmCUnitType, XmRUnitType, 
     sizeof (unsigned char), XtOffsetOf(XmPrimitiveRec, primitive.unit_type),
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
     sizeof (Pixel), XtOffsetOf(XmPrimitiveRec, primitive.foreground),
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
     XmNtraversalOn, XmCTraversalOn, XmRBoolean, 
     sizeof (Boolean), XtOffsetOf(XmPrimitiveRec, primitive.traversal_on),
     XmRImmediate, (XtPointer) TRUE
   },

   {
     XmNhighlightOnEnter, XmCHighlightOnEnter, XmRBoolean, 
     sizeof(Boolean), 
     XtOffsetOf(XmPrimitiveRec, primitive.highlight_on_enter),
     XmRImmediate, (XtPointer) False
   },

   {
     XmNnavigationType, XmCNavigationType, XmRNavigationType, 
     sizeof (unsigned char), 
     XtOffsetOf(XmPrimitiveRec, primitive.navigation_type),
     XmRImmediate, (XtPointer) XmNONE
   },

   {
     XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
     sizeof (Dimension),
     XtOffsetOf(XmPrimitiveRec, primitive.highlight_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNhighlightColor, XmCHighlightColor, XmRPixel, 
     sizeof (Pixel), XtOffsetOf(XmPrimitiveRec, primitive.highlight_color),
     XmRCallProc, (XtPointer) _XmHighlightColorDefault
   },

   {
     XmNhighlightPixmap, XmCHighlightPixmap, XmRPrimHighlightPixmap,
     sizeof (Pixmap), XtOffsetOf(XmPrimitiveRec, primitive.highlight_pixmap),
     XmRCallProc, (XtPointer) _XmPrimitiveHighlightPixmapDefault
   },

   {
     XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension,
     sizeof (Dimension), 
     XtOffsetOf(XmPrimitiveRec, primitive.shadow_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNtopShadowColor, XmCTopShadowColor, XmRPixel, 
     sizeof (Pixel),
     XtOffsetOf(XmPrimitiveRec, primitive.top_shadow_color),
     XmRCallProc, (XtPointer) _XmTopShadowColorDefault
   },

   {
     XmNtopShadowPixmap, XmCTopShadowPixmap, XmRPrimTopShadowPixmap,
     sizeof (Pixmap),
     XtOffsetOf(XmPrimitiveRec, primitive.top_shadow_pixmap),
     XmRCallProc, (XtPointer) _XmPrimitiveTopShadowPixmapDefault
   },

   {
     XmNbottomShadowColor, XmCBottomShadowColor, XmRPixel, 
     sizeof (Pixel),
     XtOffsetOf(XmPrimitiveRec, primitive.bottom_shadow_color),
     XmRCallProc, (XtPointer) _XmBottomShadowColorDefault
   },

   {
     XmNbottomShadowPixmap, XmCBottomShadowPixmap, XmRPrimBottomShadowPixmap,
     sizeof (Pixmap),
     XtOffsetOf(XmPrimitiveRec, primitive.bottom_shadow_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },

   {
     XmNhelpCallback, XmCCallback, XmRCallback, 
     sizeof(XtCallbackList),
     XtOffsetOf(XmPrimitiveRec, primitive.help_callback),
     XmRPointer, (XtPointer) NULL
   },

   {
     XmNuserData, XmCUserData, XmRPointer, 
     sizeof(XtPointer),
     XtOffsetOf(XmPrimitiveRec, primitive.user_data),
     XmRImmediate, (XtPointer) NULL
   },
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
     GetYFromShell, _XmToVerticalPixels },

   { XmNwidth,
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.width),
     _XmFromHorizontalPixels, _XmToHorizontalPixels },

   { XmNheight,
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.height), 
     _XmFromVerticalPixels, _XmToVerticalPixels },

   { XmNborderWidth, 
     sizeof (Dimension), XtOffsetOf(WidgetRec, core.border_width), 
     _XmFromHorizontalPixels, _XmToHorizontalPixels },

   { XmNhighlightThickness, 
     sizeof (Dimension), 
     XtOffsetOf(XmPrimitiveRec, primitive.highlight_thickness), 
     _XmFromHorizontalPixels, _XmToHorizontalPixels },

   { XmNshadowThickness, 
     sizeof (Dimension),
     XtOffsetOf(XmPrimitiveRec, primitive.shadow_thickness), 
     _XmFromHorizontalPixels, _XmToHorizontalPixels }
};


/*******************************************/
/*  Declaration of class extension records */

#ifndef OSF_v1_2_4
static XmBaseClassExtRec baseClassExtRec = {
#else /* OSF_v1_2_4 */
XmBaseClassExtRec _XmPrimbaseClassExtRec = {
#endif /* OSF_v1_2_4 */
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
    FocusChange,                        /* focusChange          */
};

#ifndef OSF_v1_2_4
XmPrimitiveClassExtRec _XmPrimitiveClassExtRec = {
#else /* OSF_v1_2_4 */
XmPrimitiveClassExtRec _XmPrimClassExtRec = {
#endif /* OSF_v1_2_4 */
    NULL,
    NULLQUARK,
    XmPrimitiveClassExtVersion,
    sizeof(XmPrimitiveClassExtRec),
    NULL,                               /* widget_baseline */
    NULL,                               /* widget_display_rect */
    NULL                                /* widget_margins */
};

#ifdef OSF_v1_2_4
/* This one was added in 1.2.3, in place of the above.
   I restored the 1.2.2 one, but I don't want to take this one out, 
   so I keep it here, for shared bc ABI. no one uses this stuff 
   anyway, this should all be local variable in the first place */
XmPrimitiveClassExtRec _XmPrimitiveClassExtRec ;
#endif /* OSF_v1_2_4 */

/*******************************************/
/*  The Primitive class record definition  */

externaldef(xmprimitiveclassrec) XmPrimitiveClassRec xmPrimitiveClassRec =
{
   {
      (WidgetClass) &widgetClassRec,    /* superclass	         */	
      "XmPrimitive",                    /* class_name	         */	
      sizeof(XmPrimitiveRec),           /* widget_size	         */	
      ClassInitialize,                  /* class_initialize      */    
      ClassPartInitialize,              /* class_part_initialize */
      False,                            /* class_inited          */	
      Initialize,                       /* initialize	         */	
      NULL,                             /* initialize_hook       */
      Realize,                          /* realize	         */	
      actions,                          /* actions               */	
      XtNumber(actions),                /* num_actions	         */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      True,                             /* compress_motion       */
      XtExposeCompressMaximal,          /* compress_exposure     */	
      True,                             /* compress_enterleave   */
      False,                            /* visible_interest      */
      Destroy,                          /* destroy               */	
      NULL,                             /* resize                */	
      NULL,                             /* expose                */	
      SetValues,                        /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      _XmPrimitiveGetValuesHook,        /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      NULL,                             /* tm_table              */
      NULL,                             /* query_geometry        */
      NULL,				/* display_accelerator   */
#ifndef OSF_v1_2_4
      (XtPointer)&baseClassExtRec,      /* extension             */
#else /* OSF_v1_2_4 */
      (XtPointer)&_XmPrimbaseClassExtRec,  /* extension             */
#endif /* OSF_v1_2_4 */
   },

   {
      HighlightBorder,		        /* border_highlight   */
      UnhighlightBorder,		/* border_unhighlight */
      defaultTranslations,		/* translations       */
      NULL,				/* arm_and_activate   */
      syn_resources,			/* syn resources      */
      XtNumber(syn_resources),		/* num_syn_resources  */
#ifndef OSF_v1_2_4
      (XtPointer)&_XmPrimitiveClassExtRec,/* extension        */
#else /* OSF_v1_2_4 */
      (XtPointer)&_XmPrimClassExtRec,/* extension        */
#endif /* OSF_v1_2_4 */
   }
};

externaldef(xmprimitivewidgetclass) WidgetClass xmPrimitiveWidgetClass = 
                                    (WidgetClass) &xmPrimitiveClassRec;


/**************************************************************************
**
** Fix for Motif Binary Compatible Mechanism
** deal with the synthetic resources
**
**************************************************************************/
static void
#ifdef _NO_PROTO
ResolveSyntheticOffsets(wc, ipot, cpot)
        WidgetClass wc;
        XmOffsetPtr * ipot;
        XmOffsetPtr * cpot;
#else
ResolveSyntheticOffsets(
                        WidgetClass wc, 
                        XmOffsetPtr * ipot, 
                        XmOffsetPtr * cpot)
#endif /* _NO_PROTO */
{
    XmSyntheticResource* sr = NULL;
    Cardinal nsr = 0;     /* normal resources */
    XmSyntheticResource* scr = NULL;
    Cardinal nscr = 0;    /* constraint resources */
    Cardinal i;

    /* Get synthetic resource and synthetic constraint resource lists */

    if (IsSubclassOf(wc,xmPrimitiveWidgetClass)) {
        XmPrimitiveWidgetClass pwc = (XmPrimitiveWidgetClass)wc;

        sr = pwc->primitive_class.syn_resources;
        nsr = pwc->primitive_class.num_syn_resources;
    }
    else if (IsSubclassOf(wc,xmManagerWidgetClass)) {
        XmManagerWidgetClass mwc = (XmManagerWidgetClass)wc;

        sr = mwc->manager_class.syn_resources;
        nsr = mwc->manager_class.num_syn_resources;
        scr = mwc->manager_class.syn_constraint_resources;
        nscr = mwc->manager_class.num_syn_constraint_resources;
    }
    else if (IsSubclassOf(wc,xmGadgetClass)) {
        XmGadgetClass gwc = (XmGadgetClass)wc;

        sr = gwc->gadget_class.syn_resources;
        nsr = gwc->gadget_class.num_syn_resources;
    } else {
        return;
    }

    /* Patch resource offsets using part offset tables */

    if (sr && nsr > 0 && ipot) {
        for (i = 0; i < nsr; i++)
            sr[i].resource_offset = XmGetPartOffset(&(sr[i]),ipot);
    }

    if (scr && nscr > 0 && cpot) {
        for (i = 0; i < nscr; i++)
            scr[i].resource_offset = XmGetPartOffset(&(scr[i]),cpot);
    }
}

 

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
     ** the parent's x relative to the origin, in pixels
     */

    Position	rootx, rooty;
    Widget parent = XtParent(wid);

    if (XtIsShell(parent))
    {   
        XtTranslateCoords( (Widget) wid, 
		(Position) 0, (Position) 0, &rootx, &rooty) ;
        *value = (XtArgVal) rootx;
    }
    else
    {
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
     ** the parent's y relative to the origin, in pixels
     */

    Position	rootx, rooty;
    Widget parent = XtParent(wid);

    if (XtIsShell(parent))
    {   
        XtTranslateCoords( (Widget) wid, 
		(Position) 0, (Position) 0, &rootx, &rooty) ;
        *value = (XtArgVal) rooty;
    }
    else
    {
	*value = (XtArgVal) wid->core.y ;
	_XmFromVerticalPixels(wid,  resource_offset, value);
    }
}



/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the primitive class structure.  This is called only
 *    the first time a primitive widget is created.  It registers the
 *    resource type converters unique to this class.
 *
 *
 * After class init, the "translations" variable will contain the compiled
 * translations to be used to augment a widget's translation
 * table if they wish to have keyboard traversal on.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   _XmRegisterConverters();
   _XmRegisterPixmapConverters();

   _XmInitializeExtensions();
#ifndef OSF_v1_2_4
   baseClassExtRec.record_type = XmQmotif;
#else /* OSF_v1_2_4 */
   _XmPrimbaseClassExtRec.record_type = XmQmotif;
#endif /* OSF_v1_2_4 */

}


/**********************************************************************
 *
 *  BuildPrimitiveResources
 *	Build up the primitive's synthetic resource processing 
 *      list by combining the super classes with this class.
 *
 **********************************************************************/

static void 
#ifdef _NO_PROTO
BuildPrimitiveResources( c )
        WidgetClass c ;
#else
BuildPrimitiveResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
    XmPrimitiveWidgetClass wc = (XmPrimitiveWidgetClass) c ;
    XmPrimitiveWidgetClass sc;

    sc = (XmPrimitiveWidgetClass) wc->core_class.superclass;
    
    _XmInitializeSyntheticResources(wc->primitive_class.syn_resources,
				    wc->primitive_class.num_syn_resources);

    if (sc == (XmPrimitiveWidgetClass) widgetClass) return;

    _XmBuildResources (&(wc->primitive_class.syn_resources),
		       &(wc->primitive_class.num_syn_resources),
		       sc->primitive_class.syn_resources,
		       sc->primitive_class.num_syn_resources);
}



/************************************************************************
 *
 *  ClassPartInitialize
 *    Set up the inheritance mechanism for the routines exported by
 *    primitives class part.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( w )
        WidgetClass w ;
#else
ClassPartInitialize(
        WidgetClass w )
#endif /* _NO_PROTO */
{
   static Boolean first_time = TRUE;
   XmPrimitiveWidgetClass wc = (XmPrimitiveWidgetClass) w;
   XmPrimitiveWidgetClass super =
       (XmPrimitiveWidgetClass) wc->core_class.superclass;
   XmPrimitiveClassExt              *wcePtr, *scePtr;

   /* deal with inheritance */
   if (wc->primitive_class.border_highlight == XmInheritWidgetProc)
      wc->primitive_class.border_highlight =
         super->primitive_class.border_highlight;

   if (wc->primitive_class.border_unhighlight == XmInheritWidgetProc)
      wc->primitive_class.border_unhighlight =
         super->primitive_class.border_unhighlight;

   if (wc->primitive_class.translations == XtInheritTranslations)
      wc->primitive_class.translations = super->primitive_class.translations;
   else if (wc->primitive_class.translations)
      wc->primitive_class.translations = (String)
	  XtParseTranslationTable(wc->primitive_class.translations);

   if (wc->primitive_class.arm_and_activate == XmInheritArmAndActivate)
        wc->primitive_class.arm_and_activate =
           super->primitive_class.arm_and_activate;

   wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);

   if (((WidgetClass)wc != xmPrimitiveWidgetClass) &&
       (*wcePtr)) {

       scePtr = _XmGetPrimitiveClassExtPtr(super, NULLQUARK);

       if ((*wcePtr)->widget_baseline == XmInheritBaselineProc)
	   (*wcePtr)->widget_baseline = (*scePtr)->widget_baseline;

       if ((*wcePtr)->widget_display_rect == XmInheritDisplayRectProc)
	   (*wcePtr)->widget_display_rect  = (*scePtr)->widget_display_rect;

       if ((*wcePtr)->widget_margins == XmInheritMarginsProc)
	   (*wcePtr)->widget_margins  = (*scePtr)->widget_margins;
   }

   _XmBaseClassPartInitialize( (WidgetClass) wc);

   _XmFastSubclassInit (wc, XmPRIMITIVE_BIT);


   /* Carry this ugly non portable code that deal with Xt internals... */
   if (first_time) {
        _XmSortResourceList( (XrmResource **) 
			    xmPrimitiveClassRec.core_class.resources,
			    xmPrimitiveClassRec.core_class.num_resources);
        first_time = FALSE;
    }
   
   /* synthetic resource management */
    BuildPrimitiveResources((WidgetClass) wc);
}




   
/************************************************************************
 *
 *  GetHighlightGC
 *     Get the graphics context used for drawing the border.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetHighlightGC( pw )
        XmPrimitiveWidget pw ;
#else
GetHighlightGC(
        XmPrimitiveWidget pw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = pw->primitive.highlight_color;
   values.background = pw->core.background_pixel;

   if ((pw->primitive.highlight_pixmap != None) &&
       (pw->primitive.highlight_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile ;
      values.fill_style = FillTiled;
      values.tile = pw->primitive.highlight_pixmap;
   }

   pw->primitive.highlight_GC = XtGetGC ((Widget) pw, valueMask, &values);
}



   
/************************************************************************
 *
 *  GetTopShadowGC
 *     Get the graphics context used for drawing the top shadow.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetTopShadowGC( pw )
        XmPrimitiveWidget pw ;
#else
GetTopShadowGC(
        XmPrimitiveWidget pw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = pw->primitive.top_shadow_color;
   values.background = pw->primitive.foreground;
    
   if ((pw->primitive.top_shadow_pixmap != None) &&
       (pw->primitive.top_shadow_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = pw->primitive.top_shadow_pixmap;
   }

    pw->primitive.top_shadow_GC = XtGetGC ((Widget) pw, valueMask, &values);
}




/************************************************************************
 *
 *  GetBottomShadowGC
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetBottomShadowGC( pw )
        XmPrimitiveWidget pw ;
#else
GetBottomShadowGC(
        XmPrimitiveWidget pw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = pw->primitive.bottom_shadow_color;
   values.background = pw->primitive.foreground;
    
   if ((pw->primitive.bottom_shadow_pixmap != None) &&
       (pw->primitive.bottom_shadow_pixmap != XmUNSPECIFIED_PIXMAP))
   {
      valueMask |= GCFillStyle | GCTile;
      values.fill_style = FillTiled;
      values.tile = pw->primitive.bottom_shadow_pixmap;
   }

   pw->primitive.bottom_shadow_GC = XtGetGC((Widget) pw, valueMask, &values);
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
    XmPrimitiveWidget request = (XmPrimitiveWidget) rw ;
    XmPrimitiveWidget pw = (XmPrimitiveWidget) nw ;
    XtTranslations translations ;
    char * name ;
    
    translations = (XtTranslations) ((XmPrimitiveClassRec *) XtClass( pw))
	->primitive_class.translations ;
    if(    pw->primitive.traversal_on
       && translations  &&  pw->core.tm.translations
       && !XmIsLabel( pw)    )
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
	    XtOverrideTranslations( (Widget) pw, translations) ;
	} 
    
    XtInsertEventHandler( (Widget) pw, (KeyPressMask | KeyReleaseMask), FALSE,
			 _XmVirtKeysHandler, NULL, XtListHead) ;
    pw->primitive.have_traversal = FALSE ;
    pw->primitive.highlighted = FALSE ;
    pw->primitive.highlight_drawn = FALSE ;
    
    if((pw->primitive.navigation_type != XmDYNAMIC_DEFAULT_TAB_GROUP)
       && !XmRepTypeValidValue(XmRID_NAVIGATION_TYPE, 
                               pw->primitive.navigation_type, 
			       (Widget) pw))
	{   pw->primitive.navigation_type = XmNONE ;
	} 
    _XmNavigInitialize( (Widget) request, (Widget) pw, args, num_args);
    
    if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE,
				pw->primitive.unit_type, (Widget) pw)    )
	{
	    pw->primitive.unit_type = XmPIXELS;
	}
    
    
    /*  Convert the fields from unit values to pixel values  */
    
    _XmPrimitiveImportArgs( (Widget) pw, args, num_args);
    
    /*  Check the geometry information for the widget  */
    
    if (request->core.width == 0)
	pw->core.width += pw->primitive.highlight_thickness * 2 +
	    pw->primitive.shadow_thickness * 2;
    
    if (request->core.height == 0)
	pw->core.height += pw->primitive.highlight_thickness * 2 + 
	    pw->primitive.shadow_thickness * 2;
    
    
    /*  See if the background pixmap name was set by the converter.  */
    /*  If so, generate the background pixmap and put into the       */
    /*  associated core field.                                       */
    
    name = _XmGetBGPixmapName();
    if (name != NULL) {
	pw->core.background_pixmap = 
	    XmGetPixmapByDepth (XtScreen (pw), name,
				pw->primitive.foreground, 
				pw->core.background_pixel,
				pw->core.depth);
	_XmClearBGPixmapName();
    }
    
    /*  Get the graphics contexts for the border drawing  */
    
    GetHighlightGC (pw);
    GetTopShadowGC (pw);
    GetBottomShadowGC (pw);
    
    return ;
}




/************************************************************************
 *
 *  Realize
 *	General realize procedure for primitive widgets.  Lets the bit
 *	gravity default to Forget.
 *
 ************************************************************************/
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
   Mask valueMask = *p_valueMask;

   valueMask |= CWDontPropagate;
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
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
   XmPrimitiveWidget pw = (XmPrimitiveWidget) w ;

   _XmNavigDestroy(w);

   XtReleaseGC( w, pw->primitive.top_shadow_GC);
   XtReleaseGC( w, pw->primitive.bottom_shadow_GC);
   XtReleaseGC( w, pw->primitive.highlight_GC);

}




/************************************************************************
 *
 *  SetValues
 *     Perform and updating necessary for a set values call.
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
   XmPrimitiveWidget curpw = (XmPrimitiveWidget) current;
   XmPrimitiveWidget newpw = (XmPrimitiveWidget) new_w;
   Boolean returnFlag = False;
   char *name ;

   if(    newpw->primitive.traversal_on
       && (newpw->primitive.traversal_on != curpw->primitive.traversal_on)
       && newpw->core.tm.translations
       && ((XmPrimitiveClassRec *) XtClass( newpw))
                                            ->primitive_class.translations
       && !XmIsLabel(newpw)    ) {
       XtOverrideTranslations( (Widget) newpw, (XtTranslations) 
			      ((XmPrimitiveClassRec *) XtClass( newpw))
			      ->primitive_class.translations) ;
       }
   if(    curpw->primitive.navigation_type
      != newpw->primitive.navigation_type    )
     {
	 if(    !XmRepTypeValidValue( XmRID_NAVIGATION_TYPE, 
				     newpw->primitive.navigation_type, 
				     (Widget) newpw)    )
	 {
	     newpw->primitive.navigation_type
		 = curpw->primitive.navigation_type ;
	 } 
     }
   returnFlag = _XmNavigSetValues( current, request, new_w, args, num_args);

   /*  Validate changed data.  */

   if(    !XmRepTypeValidValue( XmRID_UNIT_TYPE,
                               newpw->primitive.unit_type, 
			       (Widget) newpw)    )
       {
       newpw->primitive.unit_type = curpw->primitive.unit_type;
   }


   /*  Convert the necessary fields from unit values to pixel values  */

   _XmPrimitiveImportArgs( (Widget) newpw, args, num_args);

   /*  Check for resize conditions  */

   if (curpw->primitive.shadow_thickness !=
       newpw->primitive.shadow_thickness ||
       curpw->primitive.highlight_thickness !=
       newpw->primitive.highlight_thickness)
      returnFlag = True;

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
       newpw->core.background_pixmap = XmGetPixmapByDepth( XtScreen( newpw),
               name, newpw->primitive.foreground, newpw->core.background_pixel,
                                                            newpw->core.depth);

       XChangeWindowAttributes( XtDisplay(newpw), XtWindow(newpw), window_mask,
                &attributes);

       _XmClearBGPixmapName();
     }
   /*  Check for GC changes  */

   if (curpw->primitive.highlight_color != newpw->primitive.highlight_color ||
       curpw->primitive.highlight_pixmap != newpw->primitive.highlight_pixmap)
   {
       XtReleaseGC ((Widget) newpw, newpw->primitive.highlight_GC);
       GetHighlightGC (newpw);
       returnFlag = True;
   }

   if (curpw->primitive.top_shadow_color != 
       newpw->primitive.top_shadow_color ||
       curpw->primitive.top_shadow_pixmap != 
       newpw->primitive.top_shadow_pixmap)
       {
       XtReleaseGC ((Widget) newpw, newpw->primitive.top_shadow_GC);
       GetTopShadowGC (newpw);
       returnFlag = True;
   }
   
   if (curpw->primitive.bottom_shadow_color != 
       newpw->primitive.bottom_shadow_color ||
       curpw->primitive.bottom_shadow_pixmap != 
       newpw->primitive.bottom_shadow_pixmap)
   {
      XtReleaseGC( (Widget) newpw, newpw->primitive.bottom_shadow_GC);
      GetBottomShadowGC (newpw);
      returnFlag = True;
   }

   if(    newpw->primitive.highlight_drawn
      &&  (    !XtIsSensitive( (Widget) newpw)
	   ||  (    (curpw->primitive.highlight_on_enter)
		&&  !(newpw->primitive.highlight_on_enter)
		&&  (_XmGetFocusPolicy( (Widget) newpw) == XmPOINTER)))    )
     {
       if(    ((XmPrimitiveWidgetClass) XtClass( newpw))
	  ->primitive_class.border_unhighlight    )
	 {
	     (*(((XmPrimitiveWidgetClass) XtClass( newpw))
		->primitive_class.border_unhighlight))( (Widget) newpw) ;
	 }
     }

   /*  Return a flag which may indicate that a redraw needs to occur.  */
   
   return (returnFlag);
}


/************************************************************************
 *
 *  HighlightBorder
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
HighlightBorder( w )
        Widget w ;
#else
HighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{   
    XmPrimitiveWidget pw = (XmPrimitiveWidget) w ;

    pw->primitive.highlighted = True ;
    pw->primitive.highlight_drawn = True ;

    if(XtWidth( pw) == 0 || XtHeight( pw) == 0
       || pw->primitive.highlight_thickness == 0) return ;


    _XmDrawSimpleHighlight( XtDisplay( pw), XtWindow( pw), 
		     pw->primitive.highlight_GC, 0, 0, 
		     XtWidth( pw), XtHeight( pw),
		     pw->primitive.highlight_thickness) ;
}


/************************************************************************
 *
 *  UnhighlightBorder
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
UnhighlightBorder( w )
        Widget w ;
#else
UnhighlightBorder(
        Widget w )
#endif /* _NO_PROTO */
{   
    XmPrimitiveWidget pw = (XmPrimitiveWidget) w ;

    pw->primitive.highlighted = False ;
    pw->primitive.highlight_drawn = False ;

    if(XtWidth( w) == 0 || XtHeight( w) == 0
       || pw->primitive.highlight_thickness == 0) return ;


    if(XmIsManager (pw->core.parent))  {   
        _XmDrawSimpleHighlight( XtDisplay( pw), XtWindow( pw), 
			       ((XmManagerWidget)(pw->core.parent))
			       ->manager.background_GC,
			       0, 0, XtWidth( w), XtHeight( w),
			       pw->primitive.highlight_thickness) ;
    } else 

	_XmClearBorder( XtDisplay (pw), XtWindow (pw), 0, 0, XtWidth( w),
		       XtHeight( w) , pw->primitive.highlight_thickness) ;
}


/************************************************************************
 *
 *  WidgetNavigable
 *
 ************************************************************************/
static XmNavigability
#ifdef _NO_PROTO
WidgetNavigable( wid)
        Widget wid ;
#else
WidgetNavigable(
        Widget wid)
#endif /* _NO_PROTO */
{   
#ifndef OSF_v1_2_4
    if(    wid->core.sensitive
       &&  wid->core.ancestor_sensitive
#else /* OSF_v1_2_4 */
    if(    XtIsSensitive(wid)
#endif /* OSF_v1_2_4 */
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
	    return XmCONTROL_NAVIGABLE ;
	}
    return XmNOT_NAVIGABLE ;
}


/************************************************************************
 *
 *  FocusChange
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
FocusChange( wid, change )
        Widget wid ;
        XmFocusChange change ;
#else
FocusChange(
        Widget wid,
        XmFocusChange change)
#endif
{   
    /* Enter/Leave is called only in pointer mode,
     * Focus in/out only called in explicit mode.
     */
    switch(    change    )
	{
	case XmENTER:
	    if(!(((XmPrimitiveWidget) wid)->primitive.highlight_on_enter))
		{
		    break ;
		}
	    /* Drop through. */
	case XmFOCUS_IN:
	    if(change == XmFOCUS_IN    ) /* Because of drop-though. */ {
		((XmPrimitiveWidget) wid)->primitive.have_traversal = TRUE ;
	    }
	    if(    ((XmPrimitiveWidgetClass) XtClass( wid))
	       ->primitive_class.border_highlight    )
		{   
		    (*(((XmPrimitiveWidgetClass) XtClass( wid))
		       ->primitive_class.border_highlight))( wid) ;
		} 
	    break ;
	case XmLEAVE:
	    if(!(((XmPrimitiveWidget) wid)->primitive.highlight_on_enter))
		{
		    break ;
		}
	    /* Drop through. */
	case XmFOCUS_OUT:
	    if(change == XmFOCUS_OUT    ) /* Because of drop-though. */{
		
		((XmPrimitiveWidget) wid)->primitive.have_traversal = FALSE ;
	    }
	    if(    ((XmPrimitiveWidgetClass) XtClass( wid))
	       ->primitive_class.border_unhighlight    )
		{   
		    (*(((XmPrimitiveWidgetClass) XtClass( wid))
		       ->primitive_class.border_unhighlight))( wid) ;
		} 
	    break ;
	}
    return ;
}



/****************************************************************
 ****************************************************************
 **
 ** External functions, both _Xm and Xm.
 ** First come the action procs and then the other external entry points.
 **
 ****************************************************************
 ****************************************************************/




/************************************************************************
 *
 *  The traversal event processing routines.
 *    The following set of routines are the entry points invoked from
 *    each primitive widget when one of the traversal event conditions
 *    occur.  These routines are externed in XmP.h.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmTraverseLeft( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseLeft(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_LEFT, True);
}

void 
#ifdef _NO_PROTO
_XmTraverseRight( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseRight(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_RIGHT, True);
}

void 
#ifdef _NO_PROTO
_XmTraverseUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseUp(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_UP, True);
}

void 
#ifdef _NO_PROTO
_XmTraverseDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseDown(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_DOWN, True);
}

void 
#ifdef _NO_PROTO
_XmTraverseNext( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseNext(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_NEXT, True);
}

void 
#ifdef _NO_PROTO
_XmTraversePrev( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraversePrev(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_PREV, True);
}

void 
#ifdef _NO_PROTO
_XmTraverseHome( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseHome(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmProcessTraversal (w, XmTRAVERSE_HOME, True);
}


void 
#ifdef _NO_PROTO
_XmTraverseNextTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraverseNextTabGroup(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef CDE_TAB
   if ((XmIsPushButton(w) || XmIsArrowButton(w) || XmIsDrawnButton(w)) &&
     !_XmTraverseWillWrap(w, XmTRAVERSE_NEXT)) {
     Boolean button_tab;
     XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableButtonTab", &button_tab, NULL);
     if (button_tab)
         _XmProcessTraversal (w, XmTRAVERSE_NEXT, True);
     else
         _XmProcessTraversal (w, XmTRAVERSE_NEXT_TAB_GROUP, True);
   }
   else
#endif /* CDE_TAB */
   _XmProcessTraversal (w, XmTRAVERSE_NEXT_TAB_GROUP, True);
}

void 
#ifdef _NO_PROTO
_XmTraversePrevTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmTraversePrevTabGroup(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef CDE_TAB
   if ((XmIsPushButton(w) || XmIsArrowButton(w) || XmIsDrawnButton(w)) &&
       !_XmTraverseWillWrap(w, XmTRAVERSE_PREV))  {
     Boolean button_tab;
     XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableButtonTab", &button_tab, NULL);
     if (button_tab)
         _XmProcessTraversal (w, XmTRAVERSE_PREV, True);
     else
         _XmProcessTraversal (w, XmTRAVERSE_PREV_TAB_GROUP, True);
   }
   else
#endif /* CDE_TAB */
   _XmProcessTraversal (w, XmTRAVERSE_PREV_TAB_GROUP, True);
}

void 
#ifdef _NO_PROTO
_XmPrimitiveHelp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveHelp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   if (!_XmIsEventUnique(event))
      return;

   _XmSocorro( wid, event, NULL, NULL);
   
   _XmRecordEvent(event);
}



void 
#ifdef _NO_PROTO
_XmPrimitiveParentActivate( pw, event, params, num_params )
        Widget pw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveParentActivate( 
        Widget pw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
    XmParentInputActionRec  pp_data ;

    pp_data.process_type = XmINPUT_ACTION ;
    pp_data.action = XmPARENT_ACTIVATE ;
    pp_data.event = event ;
    pp_data.params = params ;
    pp_data.num_params = num_params ;

    _XmParentProcess( XtParent( pw), (XmParentProcessData) &pp_data) ;
}

void 
#ifdef _NO_PROTO
_XmPrimitiveParentCancel( pw, event, params, num_params )
        Widget pw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveParentCancel( 
        Widget pw,
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

    _XmParentProcess( XtParent( pw), (XmParentProcessData) &pp_data) ;
}




/************************************************************************
 *
 *  _XmDifferentBackground is called from several Primitive to check
 *   on the parent background color.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmDifferentBackground( w, parent )
        Widget w ;
        Widget parent ;
#else
_XmDifferentBackground(
        Widget w,
        Widget parent )
#endif /* _NO_PROTO */
{
   if (XmIsPrimitive (w) && XmIsManager (parent))
   {
      if (w->core.background_pixel != parent->core.background_pixel ||
          w->core.background_pixmap != parent->core.background_pixmap)
         return (True);
   }

   return (False);
}


/************************************************************************
 *
 *   XmResolvePartOffsets
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmResolvePartOffsets( w_class, offset )
        WidgetClass w_class ;
        XmOffsetPtr *offset ;
#else
XmResolvePartOffsets(
        WidgetClass w_class,
        XmOffsetPtr *offset )
#endif /* _NO_PROTO */
{
   WidgetClass c, super = w_class->core_class.superclass;
   int i, classcount = 0;
   XmPartResource *pr;

#ifndef OSF_v1_2_4
   w_class->core_class.widget_size += super->core_class.widget_size;
#else /* OSF_v1_2_4 */
   w_class->core_class.widget_size = 
     _ALIGN(w_class->core_class.widget_size) + super->core_class.widget_size;
#endif /* OSF_v1_2_4 */

   for (c = w_class; c != NULL; c = c->core_class.superclass) classcount++;

   *offset = (XmOffsetPtr) XtMalloc(classcount * sizeof(XmOffset));

   for (i = classcount-1, c = super; i > 0; c = c->core_class.superclass, i--)
      (*offset)[i] = c->core_class.widget_size;

   (*offset)[0] = 0;

   for (i = 0; i < w_class->core_class.num_resources; i++) 
   {
      pr = (XmPartResource *) &w_class->core_class.resources[i];

      /* The next line updates this in place--be careful */

      w_class->core_class.resources[i].resource_offset =
         XmGetPartOffset(pr, offset);
   }
   /*
    * deal with the synthetic resources too. 
    */

   ResolveSyntheticOffsets(w_class, offset, NULL);

}

/************************************************************************
 *
 *   XmResolveAllPartOffsets
 *
 ************************************************************************/

/*
 *  FIX for 5178: remove dependency on Xt private data 
 */
static Boolean
#ifdef _NO_PROTO
IsSubclassOf( wc, sc )
	WidgetClass wc;
	WidgetClass sc;
#else
IsSubclassOf(
	WidgetClass wc,
	WidgetClass sc)
#endif /* _NO_PROTO */
{
	WidgetClass p = wc;

	for(; (p) && (p != sc); p = p->core_class.superclass);
	return (p == sc);
}


#define XtIsConstraintClass(wc) IsSubclassOf(wc, constraintWidgetClass)
/*
 *  end FIX for 5178.
 */

void 
#ifdef _NO_PROTO
XmResolveAllPartOffsets( w_class, offset, constraint_offset )
        WidgetClass w_class ;
        XmOffsetPtr *offset ;
        XmOffsetPtr *constraint_offset ;
#else
XmResolveAllPartOffsets(
        WidgetClass w_class,
        XmOffsetPtr *offset,
        XmOffsetPtr *constraint_offset )
#endif /* _NO_PROTO */
{
   WidgetClass c, super = w_class->core_class.superclass;
   ConstraintWidgetClass cc = NULL, scc = NULL;
   int i, classcount = 0;
   XmPartResource *pr;

   /*
    *  Set up constraint class pointers
    */
   if (XtIsConstraintClass(super))
   {
        cc = (ConstraintWidgetClass)w_class;
	scc = (ConstraintWidgetClass)super;
   }

   /*
    *  Update the part size value (initially, it is the size of this part)
    */
#ifndef OSF_v1_2_4
   w_class->core_class.widget_size += super->core_class.widget_size;
#else /* OSF_v1_2_4 */

   w_class->core_class.widget_size = 
     _ALIGN(w_class->core_class.widget_size) + super->core_class.widget_size;

#endif /* OSF_v1_2_4 */
   if (cc && scc)
#ifndef OSF_v1_2_4
       cc->constraint_class.constraint_size +=
#else /* OSF_v1_2_4 */
   {
       cc->constraint_class.constraint_size = 
	 _ALIGN(cc->constraint_class.constraint_size) + 
#endif /* OSF_v1_2_4 */
	   scc->constraint_class.constraint_size;
#ifdef OSF_v1_2_4
   }
#endif /* OSF_v1_2_4 */

   /*
    *  Count the number of superclasses and allocate the offset record(s)
    */
   for (c = w_class; c != NULL; c = c->core_class.superclass) classcount++;

   *offset = (XmOffsetPtr) XtMalloc(classcount * sizeof(XmOffset));
   if (cc)
       *constraint_offset = (XmOffsetPtr) XtMalloc(classcount 
						   * sizeof(XmOffset));
   else 
       if(constraint_offset != NULL) *constraint_offset = NULL;

   /*
    *  Fill in the offset table(s) with the offset of all parts
    */
   for (i = classcount-1, c = super; i > 0; c = c->core_class.superclass, i--)
       (*offset)[i] = c->core_class.widget_size;

   (*offset)[0] = 0;

   if (constraint_offset != NULL && *constraint_offset != NULL) {
       for (i = classcount-1, scc = (ConstraintWidgetClass) super; i > 0; 
	    scc = (ConstraintWidgetClass)(scc->core_class.superclass), i--)
	   if (XtIsConstraintClass((WidgetClass)scc))
	       (*constraint_offset)[i] = 
		   scc->constraint_class.constraint_size;
	   else
	       (*constraint_offset)[i] = 0;
	
       (*constraint_offset)[0] = 0;
   }

   /*
    *  Update the resource list(s) offsets in place
    */
   for (i = 0; i < w_class->core_class.num_resources; i++) 
   {
      pr = (XmPartResource *) &w_class->core_class.resources[i];

      /* The next line updates this in place--be careful */

      w_class->core_class.resources[i].resource_offset =
         XmGetPartOffset(pr, offset);
   }

   if (cc)
       for (i = 0; i < cc->constraint_class.num_resources; i++) 
       {
          pr = (XmPartResource *) &cc->constraint_class.resources[i];

          /* The next line updates this in place--be careful */

          cc->constraint_class.resources[i].resource_offset =
             XmGetPartOffset(pr, constraint_offset);
       }
   /*
    * deal with the synthetic resources too. 
    */

   ResolveSyntheticOffsets(w_class, offset, constraint_offset);

}


/************************************************************************
 *
 *   XmWidgetGetBaselines
 *
 ************************************************************************/

Boolean
#ifdef _NO_PROTO
XmWidgetGetBaselines(wid, baselines, line_count)
        Widget wid;
        Dimension **baselines;
        int *line_count;
#else
XmWidgetGetBaselines(
        Widget wid,
        Dimension **baselines,
        int *line_count)
#endif /* _NO_PROTO */
{
  if (XmIsPrimitive(wid))
      {
	  XmPrimitiveClassExt              *wcePtr;
	  WidgetClass   wc = XtClass(wid);
	  
	  wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
	  
	  if (*wcePtr && (*wcePtr)->widget_baseline)
	      {   return( (*((*wcePtr)->widget_baseline)) 
			 (wid, baselines, line_count)) ;
	      } 
      }
  else if (XmIsGadget(wid))
      {
	  XmGadgetClassExt              *wcePtr;
	  WidgetClass   wc = XtClass(wid);
	  
	  wcePtr = _XmGetGadgetClassExtPtr(wc, NULLQUARK);
	  
	  if (*wcePtr && (*wcePtr)->widget_baseline)
	      {   return( (*((*wcePtr)->widget_baseline)) 
			 (wid, baselines, line_count)) ;
	      } 
      }
  return (False);
}


/************************************************************************
 *
 *   XmWidgetDisplayRect
 *
 ************************************************************************/

Boolean
#ifdef _NO_PROTO
XmWidgetGetDisplayRect(wid, displayrect)
        Widget wid;
        XRectangle *displayrect;
#else
XmWidgetGetDisplayRect(
        Widget wid,
        XRectangle *displayrect)
#endif /* _NO_PROTO */
{
    if (XmIsPrimitive(wid))
	{
	    XmPrimitiveClassExt              *wcePtr;
	    WidgetClass   wc = XtClass(wid);
	    
	    wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
	    
	    if (*wcePtr && (*wcePtr)->widget_display_rect)
		(*((*wcePtr)->widget_display_rect)) (wid, displayrect);
	    return (True);
	}
    else if (XmIsGadget(wid))
	{
	    XmGadgetClassExt              *wcePtr;
	    WidgetClass   wc = XtClass(wid);
	    
	    wcePtr = _XmGetGadgetClassExtPtr(wc, NULLQUARK);
	    
	    if (*wcePtr && (*wcePtr)->widget_display_rect)
		(*((*wcePtr)->widget_display_rect)) (wid, displayrect);
	    return (True);
	}
    else 
	return (False);
}

