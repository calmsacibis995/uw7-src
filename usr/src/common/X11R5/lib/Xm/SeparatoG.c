#pragma ident	"@(#)m1.2libs:Xm/SeparatoG.c	1.3"
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

#include <Xm/SeparatoGP.h>
#include <Xm/BaseClassP.h>
#include "XmI.h"
#include <X11/ShellP.h>
#include <Xm/RowColumnP.h>
#include <Xm/ExtObjectP.h>
#include <Xm/LabelGP.h>
#include <Xm/CacheP.h>
#include "RepTypeI.h"
#include <Xm/DrawP.h>
#include <stdio.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void InputDispatch() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void SecondaryObjectCreate() ;
static void InitializePosthook() ;
static Boolean SetValuesPrehook() ;
static void GetValuesPrehook() ;
static void GetValuesPosthook() ;
static Boolean SetValuesPosthook() ;
static void Initialize() ;
static void GetSeparatorGC() ;
static void Redisplay() ;
static void Destroy() ;
static Boolean VisualChange() ;
static Boolean SetValues() ;
static void Help() ;
static Cardinal GetSeparatorGClassSecResData() ;
static XtPointer GetSeparatorGClassSecResBase() ;

#else

static void InputDispatch( 
                        Widget sg,
                        XEvent *event,
                        Mask event_mask) ;
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
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetSeparatorGC( 
                        XmSeparatorGadget sg) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Destroy( 
                        Widget sg) ;
static Boolean VisualChange( 
                        Widget wid,
                        Widget cmw,
                        Widget nmw) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Help( 
                        Widget sg,
                        XEvent *event) ;
static Cardinal GetSeparatorGClassSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **data_rtn) ;
static XtPointer GetSeparatorGClassSecResBase( 
                        Widget widget,
                        XtPointer client_data) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  Resource list for Separator  */

static XtResource resources[] =
{
   {
     XmNtraversalOn, XmCTraversalOn, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmGadgetRec, gadget.traversal_on),
     XmRImmediate, (XtPointer) False
   },
   {
     XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
     sizeof (Dimension), XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
     XmRImmediate, (XtPointer) 0
   },
};


static XtResource cache_resources[] = 
{
   {
      XmNseparatorType,
      XmCSeparatorType,
      XmRSeparatorType,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmSeparatorGCacheObjRec, separator_cache.separator_type),
      XmRImmediate, (XtPointer) XmSHADOW_ETCHED_IN
   },

   {
      XmNmargin, 
      XmCMargin, 
      XmRHorizontalDimension, 
      sizeof (Dimension),
      XtOffsetOf( struct _XmSeparatorGCacheObjRec, separator_cache.margin),
      XmRImmediate, (XtPointer)  0
   },

   {
      XmNorientation,
      XmCOrientation,
      XmROrientation,
      sizeof (unsigned char),
      XtOffsetOf( struct _XmSeparatorGCacheObjRec, separator_cache.orientation),
      XmRImmediate, (XtPointer) XmHORIZONTAL
   },
};

static XmSyntheticResource cache_syn_resources[] = 
{
   {
      XmNmargin, 
      sizeof (Dimension),
      XtOffsetOf( struct _XmSeparatorGCacheObjRec, separator_cache.margin),
      _XmFromHorizontalPixels,
      _XmToHorizontalPixels,
   },
};

/* ext rec static initialization */
externaldef(xmseparatorgcacheobjclassrec)
XmSeparatorGCacheObjClassRec xmSeparatorGCacheObjClassRec =
{
  {
    /* superclass         */    (WidgetClass) &xmExtClassRec,
    /* class_name         */    "XmSeparatorGadget",
    /* widget_size        */    sizeof(XmSeparatorGCacheObjRec),
    /* class_initialize   */    NULL,
    /* chained class init */    NULL,
    /* class_inited       */    False,
    /* initialize         */    NULL,
    /* initialize hook    */    NULL,
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
    /* destroy            */    NULL,
    /* resize             */    NULL,
    /* expose             */    NULL,
    /* set_values         */    NULL,
    /* set values hook    */    NULL,
    /* set values almost  */    NULL,
    /* get values hook    */    NULL,
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

static XmBaseClassExtRec   separatorBaseClassExtRec = {
    NULL,    					/* next_extension        */
    NULLQUARK,					/* record_typ            */
    XmBaseClassExtVersion,			/* version               */
    sizeof(XmBaseClassExtRec),			/* record_size           */
    XmInheritInitializePrehook,			/* initializePrehook     */
    SetValuesPrehook,				/* setValuesPrehook      */
    InitializePosthook,				/* initializePosthook    */
    SetValuesPosthook,				/* setValuesPosthook     */
    (WidgetClass)&xmSeparatorGCacheObjClassRec,	/* secondaryObjectClass  */
    SecondaryObjectCreate,		        /* secondaryObjectCreate */
    GetSeparatorGClassSecResData,	        /* getSecResData */
    {0},			                /* Other Flags           */
    GetValuesPrehook,				/* getValuesPrehook      */
    GetValuesPosthook,				/* getValuesPosthook     */
};

static XmCacheClassPart SeparatorClassCachePart = {
    {NULL, 0, 0},        /* head of class cache list */
    _XmCacheCopy,       /* Copy routine     */
    _XmCacheDelete,     /* Delete routine   */
    _XmSeparatorCacheCompare,    /* Comparison routine   */
};


/*  The Separator class record definition  */

externaldef(xmseparatorgadgetclassrec) XmSeparatorGadgetClassRec xmSeparatorGadgetClassRec =

{
   {
      (WidgetClass) &xmGadgetClassRec,  /* superclass            */
      "XmSeparatorGadget",              /* class_name	         */
      sizeof(XmSeparatorGadgetRec),     /* widget_size	         */
      ClassInitialize,         		/* class_initialize      */
      ClassPartInitialize,              /* class_part_initialize */
      FALSE,                            /* class_inited          */
      Initialize,                       /* initialize	         */
      NULL,                             /* initialize_hook       */
      NULL,	                        /* realize	         */
      NULL,                             /* actions               */
      0,			        /* num_actions    	 */
      resources,                        /* resources	         */
      XtNumber(resources),		/* num_resources         */
      NULLQUARK,                        /* xrm_class	         */
      TRUE,                             /* compress_motion       */
      TRUE,                             /* compress_exposure     */
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      Destroy,                          /* destroy               */	
      NULL,                             /* resize                */
      Redisplay,                        /* expose                */	
      SetValues,                        /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      NULL,                             /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      NULL,                             /* tm_table              */
      NULL,                             /* query_geometry        */
      NULL,				/* display_accelerator   */
      (XtPointer)&separatorBaseClassExtRec, /* extension         */
   },

   {
      NULL, 			/* border highlight   */
      NULL,      		/* border_unhighlight */
      NULL,			/* arm_and_activate   */
      InputDispatch,		/* input dispatch     */
      VisualChange,		/* visual_change      */
      NULL,			/* syn_resources      */
      0,  			/* num_syn_resources  */
      &SeparatorClassCachePart, /* class cache part   */
      NULL,         		/* extension          */
   },

   {
      NULL,         		/* extension */
   }
};

externaldef(xmseparatorgadgetclass) WidgetClass xmSeparatorGadgetClass = 
   (WidgetClass) &xmSeparatorGadgetClassRec;



/************************************************************************
 *
 *  InputDispatch
 *     This function catches input sent by a manager and dispatches it
 *     to the individual routines.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
InputDispatch( sg, event, event_mask )
        Widget sg ;
        XEvent *event ;
        Mask event_mask ;
#else
InputDispatch(
        Widget sg,
        XEvent *event,
        Mask event_mask )
#endif /* _NO_PROTO */
{
   if (event_mask & XmHELP_EVENT) Help (sg, event);
}

/*******************************************************************
 *
 *  _XmSeparatorCacheCompare
 *
 *******************************************************************/
int 
#ifdef _NO_PROTO
_XmSeparatorCacheCompare( A, B )
        XtPointer A ;
        XtPointer B ;
#else
_XmSeparatorCacheCompare(
        XtPointer A,
        XtPointer B )
#endif /* _NO_PROTO */
{
        XmSeparatorGCacheObjPart *separator_inst =
                                               (XmSeparatorGCacheObjPart *) A ;
        XmSeparatorGCacheObjPart *separator_cache_inst =
                                               (XmSeparatorGCacheObjPart *) B ;
    if((separator_inst->margin == separator_cache_inst->margin) &&
       (separator_inst->orientation == separator_cache_inst->orientation) &&
       (separator_inst->separator_type == separator_cache_inst->separator_type) &&
       (separator_inst-> separator_GC == separator_cache_inst->separator_GC))
       return 1;
    else
       return 0;
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
  separatorBaseClassExtRec.record_type = XmQmotif;
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
   _XmFastSubclassInit (wc, XmSEPARATOR_GADGET_BIT);
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
     * fetch the resources in superclass to subclass order
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

  ((XmSeparatorGCacheObject)newSec)->ext.extensionType = XmCACHE_EXTENSION;
  ((XmSeparatorGCacheObject)newSec)->ext.logicalParent = new_w;

  _XmPushWidgetExtData(new_w, extData,
                      ((XmSeparatorGCacheObject)newSec)->ext.extensionType);
   memcpy(reqSec, newSec, size);
 
  /*
   * fill out cache pointers
   */
  SEPG_Cache(new_w) =
              &(((XmSeparatorGCacheObject)extData->widget)->separator_cache);
  SEPG_Cache(req) =
              &(((XmSeparatorGCacheObject)extData->reqWidget)->separator_cache);

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
    XmSeparatorGadget sw = (XmSeparatorGadget)new_w;

    /*
     * - register parts in cache.
     * - update cache pointers
     * - and free req
     */

    SEPG_Cache(sw) = (XmSeparatorGCacheObjPart *)
      _XmCachePart( SEPG_ClassCachePart(sw),
                    (XtPointer) SEPG_Cache(sw),
                    sizeof(XmSeparatorGCacheObjPart));

    /*
     * might want to break up into per-class work that gets explicitly
     * chained. For right now, each class has to replicate all
     * superclass logic in hook routine
     */

    /*
     * free the req subobject used for comparisons
     */
    _XmPopWidgetExtData((Widget) sw, &ext, XmCACHE_EXTENSION);
    _XmExtObjFree((XtPointer)ext->widget);
    _XmExtObjFree(ext->reqWidget);
    XtFree( (char *) ext);

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
{
    XmWidgetExtData             extData;
    XmBaseClassExt              *cePtr;
    WidgetClass                 ec;
    XmSeparatorGCacheObject     newSec, reqSec;
    Cardinal                    size;

    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmSeparatorGCacheObject)_XmExtObjAlloc(size);
    reqSec = (XmSeparatorGCacheObject)_XmExtObjAlloc(size);

    newSec->object.self = (Widget)newSec;
    newSec->object.widget_class = ec;
    newSec->object.parent = XtParent(newParent);
    newSec->object.xrm_name = newParent->core.xrm_name;
    newSec->object.being_destroyed = False;
    newSec->object.destroy_callbacks = NULL;
    newSec->object.constraints = NULL;

    newSec->ext.logicalParent = newParent;
    newSec->ext.extensionType = XmCACHE_EXTENSION;

    memcpy( &(newSec->separator_cache),
            SEPG_Cache(newParent),
            sizeof(XmSeparatorGCacheObjPart));

    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    extData->reqWidget = (Widget)reqSec;
    _XmPushWidgetExtData(newParent, extData, XmCACHE_EXTENSION);

    XtSetSubvalues((XtPointer)newSec,
                   ec->core_class.resources,
                   ec->core_class.num_resources,
                   args, *num_args);

    memcpy((XtPointer)reqSec, (XtPointer)newSec, size);

    SEPG_Cache(newParent) = &(((XmSeparatorGCacheObject)newSec)->separator_cache);
    SEPG_Cache(refParent) =
	      &(((XmSeparatorGCacheObject)extData->reqWidget)->separator_cache);

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
    XmSeparatorGCacheObject     newSec;
    Cardinal                    size;

    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmSeparatorGCacheObject)_XmExtObjAlloc(size);

    newSec->object.self = (Widget)newSec;
    newSec->object.widget_class = ec;
    newSec->object.parent = XtParent(newParent);
    newSec->object.xrm_name = newParent->core.xrm_name;
    newSec->object.being_destroyed = False;
    newSec->object.destroy_callbacks = NULL;
    newSec->object.constraints = NULL;

    newSec->ext.logicalParent = newParent;
    newSec->ext.extensionType = XmCACHE_EXTENSION;

    memcpy( &(newSec->separator_cache), 
            SEPG_Cache(newParent),
            sizeof(XmSeparatorGCacheObjPart));

    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    _XmPushWidgetExtData(newParent, extData, XmCACHE_EXTENSION);

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
/* ARGSUSED */
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
    XmWidgetExtData ext;

    _XmPopWidgetExtData(new_w, &ext, XmCACHE_EXTENSION);

    _XmExtObjFree(ext->widget);
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
{
    XmWidgetExtData             ext;

    /*
     * - register parts in cache.
     * - update cache pointers
     * - and free req
     */

    /* assign if changed! */
    if (!_XmSeparatorCacheCompare(SEPG_Cache(new_w),
                        SEPG_Cache(current)))
    {
          _XmCacheDelete( (XtPointer) SEPG_Cache(current));  /* delete the old one */
          SEPG_Cache(new_w) = (XmSeparatorGCacheObjPart *)
            _XmCachePart(SEPG_ClassCachePart(new_w),
                         (XtPointer) SEPG_Cache(new_w),
                         sizeof(XmSeparatorGCacheObjPart));
     } else
           SEPG_Cache(new_w) = SEPG_Cache(current);

    _XmPopWidgetExtData(new_w, &ext, XmCACHE_EXTENSION);

    _XmExtObjFree(ext->widget);
    _XmExtObjFree(ext->reqWidget);

    XtFree( (char *) ext);

    return FALSE;
}


      
/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/
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
        XmSeparatorGadget request = (XmSeparatorGadget) rw ;
        XmSeparatorGadget new_w = (XmSeparatorGadget) nw ;
   new_w -> gadget.traversal_on = FALSE;

   /* Force highlightThickness to zero if in a menu. */
   if (XmIsRowColumn(XtParent(new_w)) &&
       ((RC_Type(XtParent(new_w)) == XmMENU_PULLDOWN) ||
        (RC_Type(XtParent(new_w)) == XmMENU_POPUP)))
     new_w->gadget.highlight_thickness = 0;

   if(    !XmRepTypeValidValue( XmRID_SEPARATOR_TYPE,
                                   SEPG_SeparatorType( new_w), (Widget) new_w)    )
   {
      SEPG_SeparatorType(new_w) = XmSHADOW_ETCHED_IN;
   }

   if(    !XmRepTypeValidValue( XmRID_ORIENTATION,
                                     SEPG_Orientation( new_w), (Widget) new_w)    )
   {
      SEPG_Orientation(new_w) = XmHORIZONTAL;
   }

   if (SEPG_Orientation(new_w) == XmHORIZONTAL)
   {
      if (request -> rectangle.width == 0)
	 new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness +2;

      if (request -> rectangle.height == 0)
      {
	 new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness;

	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.height += 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH)
	    new_w -> rectangle.height += new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE)
	    new_w -> rectangle.height += 5;
	 else
	    if (new_w -> rectangle.height == 0)
	       new_w -> rectangle.height = 1;
      }
   }

   if (SEPG_Orientation(new_w) == XmVERTICAL)
   {
      if (request -> rectangle.height == 0)
	 new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness +2;

      if (request -> rectangle.width == 0)
      {
	 new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness;

	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.width += 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH)
	    new_w -> rectangle.width += new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE)
	    new_w -> rectangle.width += 5;
	 else
	    if (new_w -> rectangle.width == 0)
	       new_w -> rectangle.width = 1;
      }
   }



   /*  Get the drawing graphics contexts.  */

   GetSeparatorGC (new_w);
  
   /* only want help input events */

   new_w->gadget.event_mask = XmHELP_EVENT;

}




/************************************************************************
 *
 *  GetSeparatorGC
 *     Get the graphics context used for drawing the separator.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetSeparatorGC( sg )
        XmSeparatorGadget sg ;
#else
GetSeparatorGC(
        XmSeparatorGadget sg )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;
   XmManagerWidget mw;
   
   mw = (XmManagerWidget) XtParent(sg);

   valueMask = GCForeground | GCBackground;

   values.foreground = mw -> manager.foreground;
   values.background = mw -> core.background_pixel;

   if (SEPG_SeparatorType(sg) == XmSINGLE_DASHED_LINE ||
       SEPG_SeparatorType(sg) == XmDOUBLE_DASHED_LINE)
   {
      valueMask = valueMask | GCLineStyle;
      values.line_style = LineDoubleDash;
   }

   SEPG_SeparatorGC(sg) = XtGetGC ((Widget) mw, valueMask, &values);
}




/************************************************************************
 *
 *  Redisplay
 *     Invoke the application exposure callbacks.
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
        XmSeparatorGadget mw = (XmSeparatorGadget) wid ;

     if (XmIsRowColumn(XtParent(mw))) {
       Widget rowcol = XtParent(mw);

       if ((RC_Type(rowcol) == XmMENU_PULLDOWN ||
            RC_Type(rowcol) == XmMENU_POPUP)    &&
           (! ((ShellWidget)XtParent(rowcol))->shell.popped_up)) {
           /* in a menu system that is not yet popped up, ignore */
           return;
       }
     }

    _XmDrawSeparator(XtDisplay((Widget) mw), XtWindow((Widget) mw),
                  XmParentTopShadowGC (mw),
                  XmParentBottomShadowGC (mw),
                  SEPG_SeparatorGC(mw),
                  mw->rectangle.x + mw->gadget.highlight_thickness,
                  mw->rectangle.y + mw->gadget.highlight_thickness ,
                  mw->rectangle.width - 2*mw->gadget.highlight_thickness,
                  mw->rectangle.height - 2*mw->gadget.highlight_thickness,
                  mw->gadget.shadow_thickness,
                  SEPG_Margin(mw),
                  SEPG_Orientation(mw),
                  SEPG_SeparatorType(mw));
}




/************************************************************************
 *
 *  Destroy
 *	Remove the callback lists.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( sg )
        Widget sg ;
#else
Destroy(
        Widget sg )
#endif /* _NO_PROTO */
{
   XmManagerWidget mw = (XmManagerWidget) XtParent(sg);

   XtReleaseGC( (Widget) mw, SEPG_SeparatorGC(sg));

   _XmCacheDelete( (XtPointer) SEPG_Cache(sg));
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
        XmSeparatorGCacheObjPart  oldCopy;
   XmSeparatorGadget sg = (XmSeparatorGadget) gw;
   XmManagerWidget mw = (XmManagerWidget) XtParent(gw);


   if (curmw->manager.foreground != newmw->manager.foreground ||
       curmw->core.background_pixel != newmw->core.background_pixel)
   {
      XtReleaseGC( (Widget) mw, SEPG_SeparatorGC(sg));

      /* Since the GC's are cached we need to make the following calls */
      /* to update the cache correctly */

      _XmCacheCopy((XtPointer) SEPG_Cache(sg), &oldCopy, sizeof(XmSeparatorGCacheObjPart));
      _XmCacheDelete ((XtPointer) SEPG_Cache(sg));
      SEPG_Cache(sg) = &oldCopy;
      GetSeparatorGC (sg);
      SEPG_Cache(sg) = (XmSeparatorGCacheObjPart *)
      _XmCachePart(SEPG_ClassCachePart(sg),
                      (XtPointer) SEPG_Cache(sg),
                      sizeof(XmSeparatorGCacheObjPart));
      return (True);
   }

   return (False);
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
        XmSeparatorGadget current = (XmSeparatorGadget) cw ;
        XmSeparatorGadget request = (XmSeparatorGadget) rw ;
        XmSeparatorGadget new_w = (XmSeparatorGadget) nw ;
   Boolean flag = FALSE;   
   XmManagerWidget new_mw = (XmManagerWidget) XtParent(new_w);
   XmManagerWidget curr_mw = (XmManagerWidget) XtParent(current);

   /*
    * We never allow our traversal flags to be changed during SetValues();
    * this is enforced by our superclass.
    */
   /*  Force traversal_on to FALSE */
   new_w -> gadget.traversal_on = FALSE;
 
   /* Force highlightThickness to zero if in a menu. */
   if (XmIsRowColumn(XtParent(new_w)) &&
       ((RC_Type(XtParent(new_w)) == XmMENU_PULLDOWN) ||
        (RC_Type(XtParent(new_w)) == XmMENU_POPUP)))
     new_w->gadget.highlight_thickness = 0;

   if(    !XmRepTypeValidValue( XmRID_SEPARATOR_TYPE,
                                   SEPG_SeparatorType( new_w), (Widget) new_w)    )
   {
      SEPG_SeparatorType(new_w) = SEPG_SeparatorType(current);
   }

   if(    !XmRepTypeValidValue( XmRID_ORIENTATION,
                                     SEPG_Orientation( new_w), (Widget) new_w)    )
   {
      SEPG_Orientation(new_w) = SEPG_Orientation(current);
   }

   if (SEPG_Orientation(new_w) == XmHORIZONTAL)
   {
      if (request -> rectangle.width == 0)
	 new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness + 2;

      if (request -> rectangle.height == 0)
      {
	 new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness;

	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.height += 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH) 
	    new_w -> rectangle.height += new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE)
	    new_w -> rectangle.height += 5;
	 else
	    if (new_w -> rectangle.height == 0)
	       new_w -> rectangle.height = 1;
      }

      if ((SEPG_SeparatorType(new_w) != SEPG_SeparatorType(current) ||
           new_w->gadget.shadow_thickness != current->gadget.shadow_thickness ||
           new_w->gadget.highlight_thickness != current->gadget.highlight_thickness) && 
	   request -> rectangle.height == current -> rectangle.height)
      {
	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness + 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT || 
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH) 
	    new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness +
				       new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE) 
	    new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness + 5;
         } 
   }

   if (SEPG_Orientation(new_w) == XmVERTICAL)
   {
      if (request -> rectangle.height == 0)
	 new_w -> rectangle.height = 2 * new_w -> gadget.highlight_thickness + 2;

      if (request -> rectangle.width == 0)
      {
	 new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness;

	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.width += 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH) 
	    new_w -> rectangle.width += new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE)
	    new_w -> rectangle.width += 5;
	 else
	    if (new_w -> rectangle.width == 0)
	       new_w -> rectangle.width = 1;
      }

      if ((SEPG_SeparatorType(new_w) != SEPG_SeparatorType(current) ||
           new_w->gadget.shadow_thickness != current->gadget.shadow_thickness ||
           new_w->gadget.highlight_thickness != current->gadget.highlight_thickness) &&
	   request -> rectangle.width == current -> rectangle.width)
      {
	 if (SEPG_SeparatorType(new_w) == XmSINGLE_LINE ||
	     SEPG_SeparatorType(new_w) == XmSINGLE_DASHED_LINE)
	    new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness + 3;
	 else if (SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT || 
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_IN_DASH ||
		  SEPG_SeparatorType(new_w) == XmSHADOW_ETCHED_OUT_DASH) 
	    new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness +
				       new_w -> gadget.shadow_thickness;
	 else if (SEPG_SeparatorType(new_w) == XmDOUBLE_LINE ||
		  SEPG_SeparatorType(new_w) == XmDOUBLE_DASHED_LINE) 
	    new_w -> rectangle.width = 2 * new_w -> gadget.highlight_thickness + 5;
         } 
   }
  
   if (SEPG_Orientation(new_w) != SEPG_Orientation(current) ||
       SEPG_Margin(new_w) != SEPG_Margin(current) ||
       new_w -> gadget.shadow_thickness != current -> gadget.shadow_thickness)
      flag = TRUE;

   if (SEPG_SeparatorType(new_w) != SEPG_SeparatorType(current) ||
       new_mw -> core.background_pixel != curr_mw -> core.background_pixel ||
       new_mw -> manager.foreground != curr_mw -> manager.foreground)
   {
      XtReleaseGC( (Widget) new_mw, SEPG_SeparatorGC(new_w));
      GetSeparatorGC (new_w);
      flag = TRUE;
   }
  
   /* Initialize the interesting input types */
   new_w->gadget.event_mask = XmHELP_EVENT;

   return (flag);
}


/************************************************************************
 *
 *  Help
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( sg, event )
        Widget sg ;
        XEvent *event ;
#else
Help(
        Widget sg,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget parent = (XmRowColumnWidget) XtParent(sg);
   XmAnyCallbackStruct call_value;

   if (XmIsRowColumn(parent))
   {
      if (RC_Type(parent) == XmMENU_POPUP ||
	  RC_Type(parent) == XmMENU_PULLDOWN)
      {
	 (* ((XmRowColumnWidgetClass) parent->core.widget_class)->
	  row_column_class.menuProcedures)
	     (XmMENU_POPDOWN, XtParent(sg), NULL, event, NULL);
      }
   }

   call_value.reason = XmCR_HELP;
   call_value.event = event;
   _XmSocorro( (Widget) sg, event, NULL, NULL);
}


/************************************************************************
 *
 *  XmCreateSeparatorGadget
 *	Create an instance of a separator and return the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateSeparatorGadget( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateSeparatorGadget(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget (name, xmSeparatorGadgetClass, 
                           parent, arglist, argcount));
}


/****************************************************
 *   Functions for manipulating Secondary Resources.
 *********************************************************/
/*
 * GetSeparatorGClassSecResData()
 *    Create a XmSecondaryResourceDataRec for each secondary resource;
 *    Put the pointers to these records in an array of pointers;
 *    Return the pointer to the array of pointers.
 *	client_data = Address of the structure in the class record which
 *	  represents the (template of ) the secondary data.
 */
static Cardinal 
#ifdef _NO_PROTO
GetSeparatorGClassSecResData( w_class, data_rtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **data_rtn ;
#else
GetSeparatorGClassSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **data_rtn )
#endif /* _NO_PROTO */
{   int arrayCount;
    XmBaseClassExt  bcePtr;
    String  resource_class, resource_name;
    XtPointer  client_data;

    bcePtr = &(separatorBaseClassExtRec );
    client_data = NULL;
    resource_class = NULL;
    resource_name = NULL;
    arrayCount =
      _XmSecondaryResourceData ( bcePtr, data_rtn, client_data,
                resource_name, resource_class,
                GetSeparatorGClassSecResBase); 
    return (arrayCount);

}

/*
 * GetSeparatorGClassResBase ()
 *   retrun the address of the base of resources.
 *  If client data is the same as the address of the secndary data in the
 *	class record then send the base address of the cache-resources for this
 *	instance of the widget. 
 * Right now we  do not try to get the address of the cached_data from
 *  the Gadget component of this instance - since Gadget class does not
 *	have any cached_resources defined. If later secondary resources are
 *	defined for Gadget class then this routine will have to change.
 */
static XtPointer 
#ifdef _NO_PROTO
GetSeparatorGClassSecResBase( widget, client_data )
        Widget widget ;
        XtPointer client_data ;
#else
GetSeparatorGClassSecResBase(
        Widget widget,
        XtPointer client_data )
#endif /* _NO_PROTO */
{	XtPointer  widgetSecdataPtr; 
  
	widgetSecdataPtr = (XtPointer) (SEPG_Cache(widget));


    return (widgetSecdataPtr);
}
