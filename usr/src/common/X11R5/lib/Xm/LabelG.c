#pragma ident	"@(#)m1.2libs:Xm/LabelG.c	1.4"
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

#include <Xm/LabelGP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ManagerP.h>
#include "XmI.h"
#include <X11/keysymdef.h>
#include <X11/ShellP.h>
#include <Xm/MenuUtilP.h>
#include <X11/Xatom.h>
#include <Xm/CacheP.h>
#include <Xm/ExtObjectP.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <string.h>
#include <Xm/DragIconP.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/AtomMgr.h>
#include <Xm/ScreenP.h>
#include <Xm/XmosP.h>
#include <stdio.h>
#include <ctype.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define Pix(w)		LabG_Pixmap(w)
#define Pix_insen(w)	LabG_PixmapInsensitive(w)

/* Warning Messages */


#ifdef I18N_MSG
#define CS_STRING_MESSAGE	catgets(Xm_catd,MS_Label,MSG_L_4,\
					_XmMsgLabel_0003)
#define ACC_MESSAGE		catgets(Xm_catd,MS_Label,MSG_L_5,\
					_XmMsgLabel_0004)
#else
#define CS_STRING_MESSAGE	_XmMsgLabel_0003

#define ACC_MESSAGE		_XmMsgLabel_0004
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void SecondaryObjectCreate() ;
static void InitializePosthook() ;
static Boolean SetValuesPrehook() ;
static void GetValuesPrehook() ;
static void GetValuesPosthook() ;
static Boolean SetValuesPosthook() ;
static void SetNormalGC() ;
static void SetSize() ;
static void Initialize() ;
static XtGeometryResult QueryGeometry() ;
static void Destroy() ;
static void Redisplay() ;
static Boolean SetValues() ;
static Boolean VisualChange() ;
static void InputDispatch() ;
static void Help() ;
static void GetLabelString() ;
static void GetAccelerator() ;
static void GetAcceleratorText() ;
static XmStringCharSet _XmStringCharsetCreate() ;
static void GetMnemonicCharset() ;
static void QualifyLabelLocalCache() ;
static void SetOverrideCallback() ;
static Cardinal GetLabelBGClassSecResData() ;
static XtPointer GetLabelClassResBase() ;
static void SetValuesAlmost() ;
static Boolean XmLabelGadgetGetBaselines() ;
static Boolean XmLabelGadgetGetDisplayRect() ;
static Boolean Convert() ;
static Widget GetPixmapDragIcon() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass cl) ;
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
static void SetNormalGC( 
                        XmLabelGadget lw) ;
static void SetSize( 
                        Widget wid) ;
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static XtGeometryResult QueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *reply) ;
static void Destroy( 
                        Widget w) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean VisualChange( 
                        Widget wid,
                        Widget cmw,
                        Widget nmw) ;
static void InputDispatch( 
                        Widget wid,
                        XEvent *event,
                        Mask event_mask) ;
static void Help( 
                        Widget w,
                        XEvent *event) ;
static void GetLabelString( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void GetAccelerator( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void GetAcceleratorText( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmStringCharSet _XmStringCharsetCreate( 
                        XmStringCharSet stringcharset) ;
static void GetMnemonicCharset( 
                        Widget wid,
                        XrmQuark resource,
                        XtArgVal *value) ;
static void QualifyLabelLocalCache( 
                        XmLabelGadget w) ;
static void SetOverrideCallback( 
                        Widget w) ;
static Cardinal GetLabelBGClassSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **data_rtn) ;
static XtPointer GetLabelClassResBase( 
                        Widget widget,
                        XtPointer client_data) ;
static void SetValuesAlmost( 
                        Widget cw,
                        Widget nw,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static Boolean XmLabelGadgetGetBaselines( 
                        Widget wid,
                        Dimension **baselines,
                        int *line_count) ;
static Boolean XmLabelGadgetGetDisplayRect( 
                        Widget w,
                        XRectangle *displayrect) ;
static Boolean Convert( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
static Widget GetPixmapDragIcon( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  Label gadget resources  */
/*********
 * Uncached resources for Label Gadget
 */

static XtResource resources[] = 
{
    {
        XmNshadowThickness,
        XmCShadowThickness,
        XmRHorizontalDimension,
        sizeof (Dimension),
        XtOffsetOf( struct _XmLabelGadgetRec, gadget.shadow_thickness),
        XmRImmediate, (XtPointer) 0
    },

    {   XmNlabelPixmap,
        XmCLabelPixmap,
        XmRGadgetPixmap,
        sizeof(Pixmap),
        XtOffsetOf( struct _XmLabelGadgetRec,label.pixmap),
        XmRImmediate,
        (XtPointer) XmUNSPECIFIED_PIXMAP
    },

    {
         XmNlabelInsensitivePixmap,
         XmCLabelInsensitivePixmap,
         XmRGadgetPixmap,
         sizeof(Pixmap),
         XtOffsetOf( struct _XmLabelGadgetRec,label.pixmap_insen),
         XmRImmediate,
         (XtPointer) XmUNSPECIFIED_PIXMAP
    },


    {   XmNlabelString,
        XmCXmString,
        XmRXmString,
        sizeof(_XmString),
        XtOffsetOf( struct _XmLabelGadgetRec, label._label),
        XmRImmediate,
        (XtPointer) NULL
    },

    {   XmNfontList,
        XmCFontList,
        XmRFontList,
        sizeof(XmFontList),
        XtOffsetOf( struct _XmLabelGadgetRec, label.font),
        XmRImmediate,
        (XtPointer) NULL
    },

    {   XmNmnemonic,
        XmCMnemonic,
        XmRKeySym,
        sizeof(KeySym),
        XtOffsetOf( struct _XmLabelGadgetRec,label.mnemonic),
        XmRImmediate,
        (XtPointer) NULL
    },

    {   XmNmnemonicCharSet,
        XmCMnemonicCharSet,
        XmRString,
        sizeof(XmStringCharSet),
        XtOffsetOf( struct _XmLabelGadgetRec,label.mnemonicCharset),
        XmRImmediate,
        (XtPointer)  XmFONTLIST_DEFAULT_TAG
    },

    {   XmNaccelerator,
        XmCAccelerator,
        XmRString,
        sizeof(char *),
        XtOffsetOf( struct _XmLabelGadgetRec,label.accelerator),
        XmRImmediate,
        (XtPointer) NULL
    },

    {   XmNacceleratorText,
        XmCAcceleratorText,
        XmRXmString,
        sizeof(_XmString),
        XtOffsetOf( struct _XmLabelGadgetRec,label._acc_text),
        XmRImmediate,
        (XtPointer) NULL 
    },
    {
        XmNtraversalOn, 
        XmCTraversalOn, 
        XmRBoolean, 
        sizeof (Boolean),
        XtOffsetOf( struct _XmGadgetRec, gadget.traversal_on),
        XmRImmediate, (XtPointer) False
    },
   {
        XmNhighlightThickness, 
        XmCHighlightThickness, 
        XmRHorizontalDimension,
        sizeof (Dimension), 
        XtOffsetOf( struct _XmGadgetRec, gadget.highlight_thickness),
        XmRImmediate, (XtPointer) 0
   },

};
/*********
 * Cached resources for Label Gadget
 */
static XtResource cache_resources[] = 
{
    {
        XmNlabelType,
        XmCLabelType,
        XmRLabelType,
        sizeof(unsigned char),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.label_type), XmRImmediate,
        (XtPointer) XmSTRING
    },

    {   XmNalignment,
        XmCAlignment,
        XmRAlignment,
        sizeof(unsigned char),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.alignment), XmRImmediate, 
	(XtPointer) XmALIGNMENT_CENTER
    },

    {	XmNmarginWidth, 
	XmCMarginWidth, 
	XmRHorizontalDimension,
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_width), XmRImmediate,
	(XtPointer) 2
    },

    {	XmNmarginHeight, 
	XmCMarginHeight, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_height), XmRImmediate,
	(XtPointer) 2
    },

    {	XmNmarginLeft, 
	XmCMarginLeft, 
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_left), XmRImmediate,
	(XtPointer) 0
    },

    {	XmNmarginRight, 
	XmCMarginRight, 
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_right), XmRImmediate,
	(XtPointer) 0
    },

    {	XmNmarginTop, 
	XmCMarginTop, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_top), XmRImmediate,
	(XtPointer) 0
    },

    {	XmNmarginBottom, 
	XmCMarginBottom, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_bottom), XmRImmediate,
	(XtPointer) 0
    },

    {   XmNrecomputeSize,
        XmCRecomputeSize,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.recompute_size), XmRImmediate,
        (XtPointer) True
    },

    {
        XmNstringDirection,
        XmCStringDirection,
        XmRStringDirection,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.string_direction), XmRImmediate,
        (XtPointer) XmSTRING_DIRECTION_DEFAULT  
   },
};

/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
   { XmNlabelString,
     sizeof (_XmString),
     XtOffsetOf( struct _XmLabelGadgetRec, label._label),
     GetLabelString,
     NULL},

   { XmNaccelerator,
     sizeof (String),
     XtOffsetOf( struct _XmLabelGadgetRec, label.accelerator),
     GetAccelerator,
     NULL},

   { XmNacceleratorText,
     sizeof (_XmString),
     XtOffsetOf( struct _XmLabelGadgetRec, label._acc_text),
     GetAcceleratorText,
     NULL},

   { XmNmnemonicCharSet,
     sizeof (XmStringCharSet),
     XtOffsetOf( struct _XmLabelGadgetRec, label.mnemonicCharset),
     GetMnemonicCharset,
     NULL},
};

static XmSyntheticResource cache_syn_resources[] =
{

   { XmNmarginWidth, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_width), 
     _XmFromHorizontalPixels, 
     _XmToHorizontalPixels    
   },

   { XmNmarginHeight, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_height),
     _XmFromVerticalPixels, 
     _XmToVerticalPixels    
   },

   { XmNmarginLeft, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_left), 
     _XmFromHorizontalPixels, 
     _XmToHorizontalPixels    
   },

   { XmNmarginRight, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_right), 
     _XmFromHorizontalPixels, 
     _XmToHorizontalPixels    
   },

   { XmNmarginTop, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_top), 
     _XmFromVerticalPixels, 
     _XmToVerticalPixels    
   },

   { XmNmarginBottom, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelGCacheObjRec, label_cache.margin_bottom),
     _XmFromVerticalPixels, 
     _XmToVerticalPixels    
   },
};


static XmCacheClassPart LabelClassCachePart = {
	{NULL, 0, 0},		 /* head of class cache list */
	_XmCacheCopy,		/* Copy routine		*/
	_XmCacheDelete,		/* Delete routine	*/
	_XmLabelCacheCompare,	/* Comparison routine 	*/
};


static XmBaseClassExtRec       labelBaseClassExtRec = {
    NULL,                                     /* Next extension       */
    NULLQUARK,                                /* record type XmQmotif */
    XmBaseClassExtVersion,                    /* version              */
    sizeof(XmBaseClassExtRec),                /* size                 */
    XmInheritInitializePrehook,               /* initialize prehook   */
    SetValuesPrehook,                         /* set_values prehook   */
    InitializePosthook,                       /* initialize posthook  */
    SetValuesPosthook,                        /* set_values posthook  */
    (WidgetClass)&xmLabelGCacheObjClassRec,   /* secondary class      */
    SecondaryObjectCreate,                    /* creation proc        */
    GetLabelBGClassSecResData,                /* getSecResData */
    {0},                                      /* fast subclass        */
    GetValuesPrehook,                         /* get_values prehook   */
    GetValuesPosthook,                        /* get_values posthook  */
    NULL,                                     /* classPartInitPrehook */
    NULL,                                     /* classPartInitPosthook*/
    NULL,                                     /* ext_resources        */
    NULL,                                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    XmInheritWidgetNavigable,                 /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
};

/* ext rec static initialization */
externaldef (xmlabelgcacheobjclassrec)
XmLabelGCacheObjClassRec xmLabelGCacheObjClassRec =
{
  {
      /* superclass         */    (WidgetClass) &xmExtClassRec,
      /* class_name         */    "XmLabelGadget",
      /* widget_size        */    sizeof(XmLabelGCacheObjRec),
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


/* This is the class record that gets set at compile/link time  */
/* this is what is passed to the widget create routine as the   */
/* the class.  All fields must be inited at compile time.       */

XmGadgetClassExtRec _XmLabelGadClassExtRec = {
    NULL,
    NULLQUARK,
    XmGadgetClassExtVersion,
    sizeof(XmGadgetClassExtRec),
    XmLabelGadgetGetBaselines,                 /* widget_baseline */
    XmLabelGadgetGetDisplayRect,               /* widget_display_rect */
};

externaldef ( xmlabelgadgetclassrec) 
		XmLabelGadgetClassRec xmLabelGadgetClassRec = {
{
    /* superclass	  */	(WidgetClass) &xmGadgetClassRec,
    /* class_name	  */	"XmLabelGadget",
    /* widget_size	  */	sizeof(XmLabelGadgetRec),
    /* class_initialize   */    ClassInitialize,
    /* chained class init */	ClassPartInitialize,
    /* class_inited       */	False,
    /* initialize	  */	Initialize,
    /* initialize hook    */    NULL,
    /* realize		  */	NULL,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	True,
    /* compress_exposure  */	XtExposeCompressMaximal,
    /* compress enter/exit*/    True,
    /* visible_interest	  */	False,
    /* destroy		  */	Destroy,
    /* resize		  */	SetSize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set values hook    */    NULL,
    /* set values almost  */    SetValuesAlmost,
    /* get values hook    */    NULL,
    /* accept_focus	  */	NULL,
    /* version            */    XtVersion,
    /* callback offsetlst */    NULL,
    /* default trans      */    NULL,
    /* query geo proc	  */	QueryGeometry,
    /* display accelerator*/	NULL,
    /* extension record   */    (XtPointer)&labelBaseClassExtRec,	      
  },

  {				        /* XmGadget           */
      XmInheritBorderHighlight,         /* border_highlight   */
      XmInheritBorderUnhighlight,       /* border_unhighlight */
      NULL,                             /* arm_and_activate   */
      InputDispatch,			/* input dispatch     */
      VisualChange,			/* visual_change      */
      syn_resources,   	    		/* syn resources      */
      XtNumber(syn_resources),		/* num syn_resources  */
      &LabelClassCachePart,		/* class cache part   */
      (XtPointer)&_XmLabelGadClassExtRec,   /* extension          */
  },

  {					/* Label */
      SetOverrideCallback,              /* override_callback             */
      NULL,				/* menu procedure interface	 */
      NULL,                             /* extension record         */
  }
};

externaldef(xmlabelgadgetclass) WidgetClass xmLabelGadgetClass =  (WidgetClass) &xmLabelGadgetClassRec;

/*******************************************************************
 *
 *  _XmLabelCacheCompare
 *
 *******************************************************************/
int 
#ifdef _NO_PROTO
_XmLabelCacheCompare( A, B )
        XtPointer A ;
        XtPointer B ;
#else
_XmLabelCacheCompare(
        XtPointer A,
        XtPointer B )
#endif /* _NO_PROTO */
{
        XmLabelGCacheObjPart *label_inst = (XmLabelGCacheObjPart *) A ;
        XmLabelGCacheObjPart *label_cache_inst = (XmLabelGCacheObjPart *) B ;

   if ((label_inst->label_type == label_cache_inst->label_type) &&
       (label_inst->alignment == label_cache_inst->alignment) &&
       (label_inst->string_direction== label_cache_inst->string_direction) &&
       (label_inst->margin_height== label_cache_inst->margin_height) &&
       (label_inst->margin_width== label_cache_inst->margin_width) &&
       (label_inst->margin_left== label_cache_inst->margin_left) &&
       (label_inst->margin_right== label_cache_inst->margin_right) &&
       (label_inst->margin_top== label_cache_inst->margin_top) &&
       (label_inst->margin_bottom== label_cache_inst->margin_bottom) &&
       (label_inst->recompute_size== label_cache_inst->recompute_size) &&
       (label_inst->skipCallback== label_cache_inst->skipCallback) &&
       (label_inst->menu_type== label_cache_inst->menu_type)) 
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
  labelBaseClassExtRec.record_type = XmQmotif;
 
} 


/************************************************************************
 *
 *  ClassPartInitialize
 *	Processes the class fields which need to be inherited.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( cl )
        WidgetClass cl ;
#else
ClassPartInitialize(
        WidgetClass cl )
#endif /* _NO_PROTO */
{
        register XmLabelGadgetClass wc = (XmLabelGadgetClass) cl ;
   XmLabelGadgetClass super = (XmLabelGadgetClass)wc->rect_class.superclass;
   XmGadgetClassExt              *wcePtr, *scePtr;

   if (wc->label_class.setOverrideCallback == XmInheritSetOverrideCallback)
       wc->label_class.setOverrideCallback =
	super->label_class.setOverrideCallback;

   if (wc->rect_class.resize == XmInheritResize)
       wc->rect_class.resize = super->rect_class.resize;
   
   wcePtr = _XmGetGadgetClassExtPtr(wc, NULLQUARK);
   scePtr = _XmGetGadgetClassExtPtr(super, NULLQUARK);

   if ((*wcePtr)->widget_baseline == XmInheritBaselineProc)
      (*wcePtr)->widget_baseline = (*scePtr)->widget_baseline;

   if ((*wcePtr)->widget_display_rect == XmInheritDisplayRectProc)
      (*wcePtr)->widget_display_rect  = (*scePtr)->widget_display_rect;   

   _XmFastSubclassInit (((WidgetClass)wc), XmLABEL_GADGET_BIT);
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
    WidgetClass			wc;
    Cardinal			size;
    XtPointer		        newSec, reqSec;

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

    ((XmLabelGCacheObject)newSec)->ext.extensionType = XmCACHE_EXTENSION;
    ((XmLabelGCacheObject)newSec)->ext.logicalParent = new_w;

    _XmPushWidgetExtData(new_w, extData, 
			 ((XmLabelGCacheObject)newSec)->ext.extensionType);    
    memcpy(reqSec, newSec, size);

    /*
     * fill out cache pointers
     */
    LabG_Cache(new_w) = &(((XmLabelGCacheObject)extData->widget)->label_cache);
    LabG_Cache(req) = &(((XmLabelGCacheObject)extData->reqWidget)->label_cache);

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
    XmLabelGadget lw = (XmLabelGadget)new_w;
    
    /*
     * - register parts in cache.
     * - update cache pointers
     * - and free req
     */
    
    LabG_Cache(lw) = (XmLabelGCacheObjPart *)
      _XmCachePart( LabG_ClassCachePart(lw),
		   (XtPointer) LabG_Cache(lw),
		   sizeof(XmLabelGCacheObjPart));
    
    /*
     * might want to break up into per-class work that gets explicitly
     * chained. For right now, each class has to replicate all
     * superclass logic in hook routine
     */
    
    /*
     * free the req subobject used for comparisons
     */
    _XmPopWidgetExtData((Widget) lw, &ext, XmCACHE_EXTENSION);
    _XmExtObjFree((XtPointer)ext->widget);
    _XmExtObjFree((XtPointer)ext->reqWidget);
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
    XmLabelGCacheObject         newSec, reqSec;
    Cardinal			size;

    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmLabelGCacheObject)_XmExtObjAlloc(size);
    reqSec = (XmLabelGCacheObject)_XmExtObjAlloc(size);
 
    newSec->object.self = (Widget)newSec;
    newSec->object.widget_class = ec;
    newSec->object.parent = XtParent(newParent);
    newSec->object.xrm_name = newParent->core.xrm_name;
    newSec->object.being_destroyed = False;
    newSec->object.destroy_callbacks = NULL;
    newSec->object.constraints = NULL;
    
    newSec->ext.logicalParent = newParent;
    newSec->ext.extensionType = XmCACHE_EXTENSION;

    memcpy(&(newSec->label_cache), 
	   LabG_Cache(newParent),
	   sizeof(XmLabelGCacheObjPart));

    extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
    extData->widget = (Widget)newSec;
    extData->reqWidget = (Widget)reqSec;
    _XmPushWidgetExtData(newParent, extData, XmCACHE_EXTENSION);

    XtSetSubvalues((XtPointer)newSec, 
		   ec->core_class.resources, 
		   ec->core_class.num_resources,
		   args, *num_args);

    _XmExtImportArgs((Widget)newSec, args, num_args);

    memcpy((XtPointer)reqSec, (XtPointer)newSec, size);
    
    LabG_Cache(newParent) = &((newSec)->label_cache);
    LabG_Cache(refParent) = &((reqSec)->label_cache);
    
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
    XmLabelGCacheObject         newSec;
    Cardinal                    size;

    cePtr = _XmGetBaseClassExtPtr(XtClass(newParent), XmQmotif);
    ec = (*cePtr)->secondaryObjectClass;
    size = ec->core_class.widget_size;

    newSec = (XmLabelGCacheObject)_XmExtObjAlloc(size);

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
/* ARGSUSED */
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
    if (!_XmLabelCacheCompare((XtPointer)LabG_Cache(new_w),
			      (XtPointer)LabG_Cache(current)))
      
      {
	  _XmCacheDelete( (XtPointer) LabG_Cache(current));  /* delete the old one */
	  LabG_Cache(new_w) = (XmLabelGCacheObjPart *)
	    _XmCachePart(LabG_ClassCachePart(new_w),
			 (XtPointer) LabG_Cache(new_w),
			 sizeof(XmLabelGCacheObjPart));
      } else
	LabG_Cache(new_w) = LabG_Cache(current);
    
    _XmPopWidgetExtData(new_w, &ext, XmCACHE_EXTENSION);
    
    _XmExtObjFree((XtPointer)ext->widget);
    _XmExtObjFree((XtPointer)ext->reqWidget);

    XtFree( (char *) ext);
    
    return FALSE;
}






/************************************************************************
 *
 *  SetNormalGC
 *	Create the normal and insensitive GC's for the gadget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetNormalGC( lw )
        XmLabelGadget lw ;
#else
SetNormalGC(
        XmLabelGadget lw )
#endif /* _NO_PROTO */
{
   XGCValues       values;
   XtGCMask        valueMask;
   XmManagerWidget mw;
   XFontStruct     *fs = (XFontStruct *) NULL;


   mw = (XmManagerWidget) XtParent(lw);

   valueMask = GCForeground | GCBackground | GCFont |
               GCGraphicsExposures | GCClipMask;

    _XmFontListGetDefaultFont(LabG_Font(lw), &fs);
   values.foreground = mw->manager.foreground;
   values.background = mw->core.background_pixel;
   values.graphics_exposures = FALSE;
   values.clip_mask = None;

   if (fs==NULL)
     valueMask &= ~GCFont;
   else
     values.font       = fs->fid;

   LabG_NormalGC(lw) = XtGetGC( (Widget) mw, valueMask, &values);

   valueMask |= GCFillStyle | GCTile;
   values.fill_style = FillTiled;
   /* "50_foreground" is in the installed set and should always be found;
   ** omit check for XmUNSPECIFIED_PIXMAP
   */
   values.tile = XmGetPixmapByDepth (XtScreen((Widget)(lw)), "50_foreground",
				mw->manager.foreground,
				mw->core.background_pixel,
				mw->core.depth);
   LabG_InsensitiveGC(lw) = XtGetGC( (Widget) mw, valueMask, &values);

}

/************************************************************************
 *
 * _XmCalcLabelGDimensions()
 *   Calculates the dimensionsof the label text and pixmap, and updates
 *   the TextRect fields appropriately. Called at Initialize and SetValues.
 *   This is also called by the subclasses to recalculate label dimensions.
 *
 *************************************************************************/
void 
#ifdef _NO_PROTO
_XmCalcLabelGDimensions( wid )
        Widget wid ;
#else
_XmCalcLabelGDimensions(
        Widget wid )
#endif /* _NO_PROTO */
{
   XmLabelGadget newlw = (XmLabelGadget) wid ;

  /* initialize TextRect width and height to 0, reset if needed */

     LabG_TextRect(newlw).width = 0;
     LabG_TextRect(newlw).height = 0;
     LabG_AccTextRect(newlw).width = 0;
     LabG_AccTextRect(newlw).height = 0;

  if (LabG_IsPixmap (newlw))
  {

      if ((newlw->rectangle.sensitive) &&
          (newlw->rectangle.ancestor_sensitive))
      {
         if (Pix (newlw) != XmUNSPECIFIED_PIXMAP)
         {
            unsigned int junk;
            unsigned int  w = 0 , h = 0, d;

            XGetGeometry (XtDisplay(newlw),
                          Pix(newlw),
                          (Window *) &junk,    /* returned root window */
                          (int *) &junk, (int *) &junk,   /* x, y of pixmap */
                          &w, &h,              /* width, height of pixmap */
                          &junk,   /* border width */
                          &d);              /* depth */


            LabG_TextRect(newlw).width = (unsigned short) w;
            LabG_TextRect(newlw).height = (unsigned short) h;
         }
      }
      else
      {
         if (Pix_insen(newlw) != XmUNSPECIFIED_PIXMAP)
         {
            unsigned int junk;
            unsigned int  w = 0 , h = 0, d;

            XGetGeometry (XtDisplay(newlw),
                          Pix_insen(newlw),
                          (Window *) &junk,   /* returned root window */
                          (int *) &junk, (int *) &junk,   /* x, y of pixmap */
                          &w, &h,        /* width, height of pixmap */
                          &junk,    /* border width */
                          &d);          /* depth */

            LabG_TextRect(newlw).width = (unsigned short) w;
            LabG_TextRect(newlw).height = (unsigned short) h;
         }
      }

      if (LabG__acceleratorText(newlw) != NULL)
      {
         Dimension w,h ;

         /*
          * If we have a string then size it.
          */
         if (!_XmStringEmpty (LabG__acceleratorText(newlw)))
         {
            _XmStringExtent(LabG_Font(newlw),
                            LabG__acceleratorText(newlw), &w, &h);
            LabG_AccTextRect(newlw).width = (unsigned short)w;
            LabG_AccTextRect(newlw).height = (unsigned short)h;
         }
      }
   }
   
   else
    if (LabG_IsText (newlw))
    {
      Dimension w, h;

      /* If we have a string then size it.  */

      if (!_XmStringEmpty (LabG__label(newlw)))
      {
         _XmStringExtent (LabG_Font(newlw), LabG__label(newlw), &w, &h);
         LabG_TextRect(newlw).width = (unsigned short)w;
         LabG_TextRect(newlw).height = (unsigned short)h;
      }

      if (LabG__acceleratorText(newlw) != NULL)
      {
         /*
          * If we have a string then size it.
          */
         if (!_XmStringEmpty (LabG__acceleratorText(newlw)))
         {
            _XmStringExtent(LabG_Font(newlw), LabG__acceleratorText(newlw),
                            &w, &h);
            LabG_AccTextRect(newlw).width = w;
            LabG_AccTextRect(newlw).height = h;
         }
      }
    }
}    

/************************************************************************
 *
 *  SetSize
 *	Sets new width, new height, and new label.TextRect 
 *	appropriately. It is called by Initialize and SetValues.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetSize( wid )
        Widget wid ;
#else
SetSize(
        Widget wid )
#endif /* _NO_PROTO */
{  
        XmLabelGadget newlw = (XmLabelGadget) wid ;
   int leftx, rightx;

 
   /* increase margin width if necessary to accomodate accelerator text */
   if (LabG__acceleratorText(newlw) != NULL) 
       
       if (LabG_MarginRight(newlw) < 
	   LabG_AccTextRect(newlw).width + LABELG_ACC_PAD)
       {
	  LabG_MarginRight(newlw) = 
	      LabG_AccTextRect(newlw).width + LABELG_ACC_PAD;
       }

   /* Has a width been specified?  */

   if (newlw->rectangle.width == 0)
       newlw->rectangle.width =
           LabG_TextRect(newlw).width +
	       LabG_MarginLeft(newlw) + LabG_MarginRight(newlw) +
		   (2 * (LabG_MarginWidth(newlw) +
			 newlw->gadget.highlight_thickness
				+ newlw->gadget.shadow_thickness));

   leftx =    newlw->gadget.highlight_thickness +
	      newlw->gadget.shadow_thickness + LabG_MarginWidth(newlw) +
	      LabG_MarginLeft(newlw);

   rightx = newlw->rectangle.width - newlw->gadget.highlight_thickness -
	      newlw->gadget.shadow_thickness - LabG_MarginWidth(newlw) -
	      LabG_MarginRight(newlw);
		           
                    
   switch (LabG_Alignment(newlw))
   {
    case XmALIGNMENT_BEGINNING:
	  LabG_TextRect(newlw).x = leftx;
      break;

    case XmALIGNMENT_END:
      LabG_TextRect(newlw).x = rightx - LabG_TextRect(newlw).width;
      break;

    default:
      /* XmALIGNMENT_CENTER */
      LabG_TextRect(newlw).x = leftx + 
	      (rightx - leftx - (int)LabG_TextRect(newlw).width) /2;
      break;
   }

   /*  Has a height been specified?  */
   if (newlw->rectangle.height == 0)
       newlw->rectangle.height = Max(LabG_TextRect(newlw).height,
				     LabG_AccTextRect(newlw).height) 
	   + LabG_MarginTop(newlw) 
	       + LabG_MarginBottom(newlw)
		   + (2 * (LabG_MarginHeight(newlw)
			   + newlw->gadget.highlight_thickness
			   + newlw->gadget.shadow_thickness));

   LabG_TextRect(newlw).y =  (short) (newlw->gadget.highlight_thickness
           + newlw->gadget.shadow_thickness
               + LabG_MarginHeight(newlw) + LabG_MarginTop(newlw) +
                   ((newlw->rectangle.height - LabG_MarginTop(newlw)
                     - LabG_MarginBottom(newlw)
                     - (2 * (LabG_MarginHeight(newlw)
                             + newlw->gadget.highlight_thickness
                             + newlw->gadget.shadow_thickness))
                     - LabG_TextRect(newlw).height) / 2));

   if (LabG__acceleratorText(newlw) != NULL)
   {
      Dimension  base_label, base_accText, diff;

      LabG_AccTextRect(newlw).x = (newlw->rectangle.width -
	   newlw->gadget.highlight_thickness -
	   newlw->gadget.shadow_thickness -
	   LabG_MarginWidth(newlw) -
	   LabG_MarginRight(newlw) +
	   LABELG_ACC_PAD);

      LabG_AccTextRect(newlw).y = newlw->gadget.highlight_thickness
	      + newlw->gadget.shadow_thickness
		  + LabG_MarginHeight(newlw) + LabG_MarginTop(newlw) +
		      ((newlw->rectangle.height - LabG_MarginTop(newlw)
			- LabG_MarginBottom(newlw)
			- (2 * (LabG_MarginHeight(newlw)
				+ newlw->gadget.highlight_thickness
				+ newlw->gadget.shadow_thickness))
			- LabG_AccTextRect(newlw).height) / 2);

      /* make sure the label and accelerator text line up*/
      /* when the fonts are different */

      if (LabG_IsText (newlw))
      { 
	 base_label = 
	     _XmStringBaseline (LabG_Font(newlw), LabG__label(newlw));
	 base_accText = 
	     _XmStringBaseline (LabG_Font(newlw),
				LabG__acceleratorText(newlw));
	 
	 if (base_label > base_accText)
	 {
	    diff = base_label - base_accText;
	    LabG_AccTextRect(newlw).y = LabG_TextRect(newlw).y + diff - 1;
	 }
	 else if (base_label < base_accText)
	 {
	    diff = base_accText - base_label;
	    LabG_TextRect(newlw).y = LabG_AccTextRect(newlw).y + diff - 1;
	 }
      }
   }

   if (newlw->rectangle.width == 0)    /* set core width and height to a */
       newlw->rectangle.width = 1;     /* default value so that it doesn't */
   if (newlw->rectangle.height == 0)   /* a Toolkit Error */
       newlw->rectangle.height = 1;
}




/************************************************************************
 *
 *  Initialize
 *	This is the widget's instance initialize routine.  It is
 *	called once for each widget.
 *  Changes: Treat label, pixmap, labeltype, mnemoniccharset as independedntly
 *	settable resource.
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Initialize( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    unsigned char  stringDirection;
    Arg           myargs[1];
    int n;

    XmLabelGadget lw = (XmLabelGadget) new_w;
    XmLabelGadget rw = (XmLabelGadget) req;

    /* if menuProcs is not set up yet, try again */
    if (xmLabelGadgetClassRec.label_class.menuProcs == NULL)
	xmLabelGadgetClassRec.label_class.menuProcs =
	    (XmMenuProc) _XmGetMenuProcContext();

    LabG_SkipCallback(new_w) = FALSE;

    /* Check for Invalid enumerated types */

    if(    !XmRepTypeValidValue( XmRID_LABEL_TYPE, LabG_LabelType( new_w),
                                                                      new_w)    )
    {
      LabG_LabelType(new_w) = XmSTRING;
    }

    if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, LabG_Alignment( new_w),
                                                                      new_w)    )
    {
      LabG_Alignment(new_w) = XmALIGNMENT_CENTER;
    }
/*
 * Default string behavior : is same as that of parent.
 *  If string direction is not set then borrow it from the parent.
 *  Should we still check for it.
 */
   if (LabG_StringDirection(new_w) == XmSTRING_DIRECTION_DEFAULT)
    { if ( XmIsManager( XtParent(new_w)))
	  { n = 0;
        XtSetArg( myargs[n], XmNstringDirection, &stringDirection); n++;
        XtGetValues ( XtParent(lw), myargs, n);
        LabG_StringDirection(new_w) = stringDirection;
	  }
      else
       LabG_StringDirection(new_w) = XmSTRING_DIRECTION_L_TO_R;
    }

    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
                                          LabG_StringDirection( new_w), new_w)    )
    {
      LabG_StringDirection(new_w) = XmSTRING_DIRECTION_L_TO_R;
    }


    if (LabG_Font(new_w) == NULL)
    {
      XmFontList defaultFont;

      if (XtClass(lw) == xmLabelGadgetClass)
         defaultFont = _XmGetDefaultFontList( (Widget) lw, XmLABEL_FONTLIST);
      else
	 defaultFont = _XmGetDefaultFontList( (Widget) lw, XmBUTTON_FONTLIST);

      lw->label.font = XmFontListCopy (defaultFont);

    }
    /* Make a local copy of the font list */
    else
    {
      LabG_Font(new_w) = XmFontListCopy( LabG_Font(new_w));
    }

    if (XmIsRowColumn(XtParent(new_w)))
    {
       Arg arg[1];
       XtSetArg (arg[0], XmNrowColumnType, &LabG_MenuType(new_w));
       XtGetValues (XtParent(new_w), arg, 1);
    }
    else
	LabG_MenuType(new_w) = XmWORK_AREA;
	

    /*  Handle the label string :
     *   If no label string is given accept widget's name as default.
     *     convert the widgets name to an XmString before storing;
     *   else
     *     save a copy of the given string.
     *     If the given string is not an XmString issue an warning.
     */

   if (LabG__label(new_w) == NULL)
    {
          XmString string;

	  string = _XmOSGetLocalizedString ((char *) NULL,  /* reserved */
					    (Widget) lw,
					    XmNlabelString,
					    XrmQuarkToString(lw->object.xrm_name));
	  
          LabG__label(new_w) =  _XmStringCreate(string);
          XmStringFree (string);
     }
   else
    { if (_XmStringIsXmString( (XmString) LabG__label(new_w)))
           LabG__label(new_w) = _XmStringCreate( (XmString) LabG__label(new_w));       
      else
       {  XmString string;
          XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;

         _XmWarning( (Widget) lw, CS_STRING_MESSAGE);		
          string   =  XmStringLtoRCreate(XrmQuarkToString(lw->object.xrm_name),
				cset);
         LabG__label(new_w) =  _XmStringCreate(string);
         XmStringFree (string);
       }
    }

     /*
      * Convert the given mnemonicCharset to the internal Xm-form.
      */
     if (LabG_MnemonicCharset(new_w) != NULL)
     {
         LabG_MnemonicCharset (new_w) =
            _XmStringCharsetCreate (LabG_MnemonicCharset (new_w) );
     }
     else
         LabG_MnemonicCharset (new_w) =
            _XmStringCharsetCreate (XmFONTLIST_DEFAULT_TAG );

    /* Accelerators are currently only supported in menus */

    if ((LabG__acceleratorText(new_w) != NULL) &&
	((LabG_MenuType(new_w) == XmMENU_POPUP) ||
	 (LabG_MenuType(new_w) == XmMENU_PULLDOWN)))
     {
        if (_XmStringIsXmString( (XmString) LabG__acceleratorText(new_w)))
        {
	   /*
	    * Copy the input string into local space, if
	    * not a Cascade Button
	    */
           if ( XmIsCascadeButtonGadget(new_w))
              LabG__acceleratorText(new_w) = NULL;
           else
              LabG__acceleratorText(new_w)= 
		 _XmStringCreate( (XmString) LabG__acceleratorText(new_w));
        }
        else
        {
           _XmWarning( (Widget) lw, ACC_MESSAGE);
           LabG__acceleratorText(new_w) = NULL;
        }
    }

    else
        LabG__acceleratorText(new_w) = NULL;

    if ((LabG_Accelerator(new_w) != NULL) &&
        ((LabG_MenuType(new_w) == XmMENU_POPUP) ||
	 (LabG_MenuType(new_w) == XmMENU_PULLDOWN)))
    {
      char *s;

      /* Copy the accelerator into local space */

      s = XtMalloc (XmStrlen (LabG_Accelerator(new_w)) + 1);
      strcpy (s, LabG_Accelerator(new_w));
      LabG_Accelerator(lw) = s;
    }
    else
        LabG_Accelerator(lw) = NULL;

    LabG_SkipCallback(lw) = FALSE;

    /*  If zero width and height was requested by the application,  */
    /*  reset new's width and height to zero to allow SetSize()     */
    /*  to operate properly.                                        */

    if (rw->rectangle.width == 0) 
        lw->rectangle.width = 0;
    
    if (rw->rectangle.height == 0) 
        lw->rectangle.height = 0;
    

    _XmCalcLabelGDimensions(new_w);
    (* (((XmLabelGadgetClassRec *)(lw->object.widget_class))->
		rect_class.resize)) ((Widget) lw); 

    SetNormalGC(lw);

    /*  Force the label traversal flag when in a menu  */

    if ((XtClass(lw) == xmLabelGadgetClass) &&
       ((LabG_MenuType(new_w) == XmMENU_POPUP) ||
        (LabG_MenuType(new_w) == XmMENU_PULLDOWN) ||
        (LabG_MenuType(new_w) == XmMENU_OPTION)))
    {
       lw->gadget.traversal_on = False;
       lw->gadget.highlight_on_enter = False;
    }

   if ((LabG_MenuType(new_w) == XmMENU_POPUP) ||
       (LabG_MenuType(new_w) == XmMENU_PULLDOWN) ||
       (LabG_MenuType(new_w) == XmMENU_BAR))
       lw->gadget.highlight_thickness = 0;

   /*  Initialize the interesting input types.  */

   lw->gadget.event_mask = XmHELP_EVENT | XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT
                           | XmENTER_EVENT | XmLEAVE_EVENT | XmBDRAG_EVENT;

}


/************************************************************************
 *
 *  QueryGeometry
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryGeometry( wid, intended, reply )
        Widget wid ;
        XtWidgetGeometry *intended ;
        XtWidgetGeometry *reply ;
#else
QueryGeometry(
        Widget wid,
        XtWidgetGeometry *intended,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
        XmLabelGadget lg = (XmLabelGadget) wid ;
    reply->request_mode = 0;		/* set up fields I care about */

     /* Don't really know what to do with queries about x,y,border,stacking.
     * Since we are interpreting unset bits as a request for information
     * (asking about neither height or width does the old 0-0 request)
     * a caller asking about x,y should not get back width and height,
     * especially since it is an expensive operation.  So x, y, border, stack
     * all return No, this indicates we'd prefer to remain as is.  Parent
     * is free to change it anyway...
     *
     */

    if (GMode (intended) & ( ~ (CWWidth | CWHeight))) 
	  return (XtGeometryNo);

    if (LabG_RecomputeSize(lg) == FALSE)
          return (XtGeometryNo);


     /* pre-load the reply with input values */

     reply->request_mode = (CWWidth | CWHeight);

     reply->width = LabG_TextRect(lg).width +
                    (2 * (LabG_MarginWidth(lg) +
                    lg->gadget.highlight_thickness +
                    lg->gadget.shadow_thickness)) +
                    LabG_MarginLeft(lg) +
                    LabG_MarginRight(lg);

      if (reply->width == 0)
          reply->width = 1;

      reply->height = Max(LabG_TextRect(lg).height,
                          LabG_AccTextRect(lg).height)
                      + (2 * (LabG_MarginHeight(lg) +
                      lg->gadget.highlight_thickness +
                      lg->gadget.shadow_thickness)) +
                      LabG_MarginTop(lg) +
                      LabG_MarginBottom(lg);

      if (reply->height == 0)
          reply->height = 1;

     if (
	((GMode(intended) & CWWidth) && (reply->width != intended->width)) 
	||
	((GMode(intended) & CWHeight) && (reply->height != intended->height))
	|| 
 	(GMode (intended) != GMode (reply))
	)
           return (XtGeometryAlmost);
     else
     {
         reply->request_mode = 0;
         return (XtGeometryYes);
     }

}


/************************************************************************
 *
 *  Destroy
 *	Free up the label gadget allocated space.  This includes
 *	the label, and GC's.
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
    if (LabG__label(w) != NULL) _XmStringFree (LabG__label(w));
    if (LabG__acceleratorText(w) != NULL) _XmStringFree (LabG__acceleratorText(w));
    if (LabG_Accelerator(w) != NULL) XtFree (LabG_Accelerator(w));
    if (LabG_Font(w)  != NULL) XmFontListFree (LabG_Font(w));
    if (LabG_MnemonicCharset(w) != NULL ) XtFree (LabG_MnemonicCharset (w));

    _XmCacheDelete((XtPointer) LabG_Cache(w));

    XtReleaseGC (XtParent(w), LabG_NormalGC(w));
    XtReleaseGC (XtParent(w), LabG_InsensitiveGC(w));

}


/************************************************************************
 *
 *  Redisplay
 *
 ************************************************************************/
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
   XmLabelGadget lw = (XmLabelGadget) wid;
   GC gc;
   GC clipgc = NULL;
   XRectangle clip_rect;
   Dimension availW, availH, marginal_width, marginal_height, max_text_height;
   Boolean clip_set = False;

   if (LabG_MenuType(lw) == XmMENU_POPUP ||
       LabG_MenuType(lw) == XmMENU_PULLDOWN)
   {
      ShellWidget mshell = (ShellWidget) XtParent(XtParent(lw));
      if (! mshell->shell.popped_up)
	  return;
   }

   availH = lw->rectangle.height;
   availW = lw->rectangle.width;

   /*
    * Don't count MarginWidth to be consistent with Label Widget.
    *
    * Adjust definitions of temporary variables
    */
   marginal_width = LabG_MarginLeft(lw) + LabG_MarginRight(lw) +
     (2 * (lw->gadget.highlight_thickness + lw->gadget.shadow_thickness));
   
   marginal_height = LabG_MarginTop(lw) + LabG_MarginBottom(lw) +
     (2 * (lw->gadget.highlight_thickness + lw->gadget.shadow_thickness));
   
   max_text_height = Max(LabG_TextRect(lw).height, LabG_AccTextRect(lw).height);

   /* Clip should include critical margins (see Label.c) */
   if (availH < (marginal_height + max_text_height) ||
       availW < (marginal_width + LabG_TextRect(lw).width))
     {
       clip_rect.x = lw->rectangle.x + lw->gadget.highlight_thickness +
	 lw->gadget.shadow_thickness + LabG_MarginLeft(lw);
       clip_rect.y = lw->rectangle.y + lw->gadget.highlight_thickness +
	 lw->gadget.shadow_thickness + LabG_MarginTop(lw);
       
       /* Don't allow negative dimensions */
       if (availW > marginal_width) 
	 clip_rect.width = availW - marginal_width;
       else
	 clip_rect.width = 0;
       
       if (availH > marginal_height)
	 clip_rect.height = availH - marginal_height;
       else
	 clip_rect.height = 0;
       
      if ((lw->rectangle.sensitive) && (lw->rectangle.ancestor_sensitive))
        clipgc = LabG_NormalGC(lw);
      else
        clipgc = LabG_InsensitiveGC(lw);

      XSetClipRectangles(XtDisplay(lw), clipgc, 0,0, &clip_rect, 1, Unsorted);
      clip_set = True;
   }


   /*  Draw the pixmap or text  */

   if (LabG_IsPixmap(lw))
   {
     if ((lw->rectangle.sensitive) && (lw->rectangle.ancestor_sensitive))
     {
       if (Pix (lw) != XmUNSPECIFIED_PIXMAP)
       {
         gc = LabG_NormalGC(lw);

         XCopyArea (XtDisplay(lw), Pix(lw), XtWindow(lw),gc, 0, 0, 
                    LabG_TextRect(lw).width, LabG_TextRect(lw).height, 
                    lw->rectangle.x + LabG_TextRect(lw).x,
                    lw->rectangle.y + LabG_TextRect(lw).y); 
       }
     }
     else
     {
       if (Pix_insen (lw) != XmUNSPECIFIED_PIXMAP)
       {
         gc = LabG_InsensitiveGC(lw);

         XCopyArea (XtDisplay(lw), Pix_insen(lw), XtWindow(lw),gc, 0, 0,
                    LabG_TextRect(lw).width, LabG_TextRect(lw).height,
                    lw->rectangle.x + LabG_TextRect(lw).x,
                    lw->rectangle.y + LabG_TextRect(lw).y);

       }

     }

   }

   else if ( (LabG_IsText (lw)) && (LabG__label(lw) != NULL))
   {
      if (LabG_Mnemonic(lw) != NULL)
      { /*
         * A hack to use keysym as the mnemonic.
         */
          char tmp[2];
          tmp[0] = (( LabG_Mnemonic(lw)) & ( (long) (0xFF)));
          tmp[1] = '\0';

        _XmStringDrawMnemonic(XtDisplay(lw), XtWindow(lw),
                                 LabG_Font(lw), LabG__label(lw),
                                 (((lw->rectangle.sensitive) &&
                                   (lw->rectangle.ancestor_sensitive)) ?
                                 LabG_NormalGC(lw) :
                                 LabG_InsensitiveGC(lw)),
                                 lw->rectangle.x + LabG_TextRect(lw).x,
                                 lw->rectangle.y + LabG_TextRect(lw).y,
                                 LabG_TextRect(lw).width, LabG_Alignment(lw),
                                 XmSTRING_DIRECTION_L_TO_R, NULL,
                                 tmp,   /* LabG__mnemonic(lw), */ 
				 LabG_MnemonicCharset(lw));
		 }

     else
      _XmStringDraw (XtDisplay(lw), XtWindow(lw), 
                     LabG_Font(lw), LabG__label(lw),
                     (((lw->rectangle.sensitive) &&
                       (lw->rectangle.ancestor_sensitive)) ?
                     LabG_NormalGC(lw) :
                     LabG_InsensitiveGC(lw)),
                     lw->rectangle.x + LabG_TextRect(lw).x,
                     lw->rectangle.y + LabG_TextRect(lw).y,
                     LabG_TextRect(lw).width,
                     LabG_Alignment(lw), XmSTRING_DIRECTION_L_TO_R, NULL);
   }

   if (LabG__acceleratorText(lw) != NULL)
   {
        /* since accelerator text  is drawn by moving in from the right,
         it is possible to overwrite label text when there is clipping,
         Therefore draw accelerator text only if there is enough
         room for everything */

        if ((lw->rectangle.width) >= (2 * (lw->gadget.highlight_thickness +
                                           lw->gadget.shadow_thickness +
                                           LabG_MarginWidth(lw)) +
                                      LabG_MarginLeft(lw) + LabG_TextRect(lw).width +
                                      LabG_MarginRight(lw)))
           _XmStringDraw (XtDisplay(lw), XtWindow(lw),
                         LabG_Font(lw), LabG__acceleratorText(lw),
                         (((lw->rectangle.sensitive) &&
                         (lw->rectangle.ancestor_sensitive)) ?
                         LabG_NormalGC(lw) :
                         LabG_InsensitiveGC(lw)),
                         lw->rectangle.x + LabG_AccTextRect(lw).x,
                         lw->rectangle.y + LabG_AccTextRect(lw).y,
                         LabG_AccTextRect(lw).width, XmALIGNMENT_END,
                         XmSTRING_DIRECTION_L_TO_R, NULL);
   }


   /*  If set, reset the clipping rectangle to none  */

   if (clip_set)
      XSetClipMask (XtDisplay (lw), clipgc, None);

   /* Redraw the proper highlight  */

   if ((LabG_MenuType(lw) != XmMENU_POPUP) &&
       (LabG_MenuType(lw) != XmMENU_PULLDOWN))
   {
      if (lw->gadget.highlighted)
      {   
	 (*((XmGadgetClass) XtClass( lw))->gadget_class.border_highlight)(
								 (Widget) lw) ;
      } 
   }
}

/************************************************************************
 *
 *  SetValues
 *	This routine will take care of any changes that have been made
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
   XmLabelGadget current = (XmLabelGadget) cw ;
   XmLabelGadget req = (XmLabelGadget) rw ;
   XmLabelGadget new_w = (XmLabelGadget) nw ;
   Boolean flag = False;
   Boolean newstring = False;
   Boolean ProcessFlag = FALSE;
   Boolean CleanupFontFlag = FALSE;
   Boolean Call_SetSize = False;

   /*  If the label has changed, make a copy of the new label,  */
   /*  and free the old label.                                  */ 


   if (LabG__label(new_w)!= LabG__label(current))
   { newstring = True;
     if (LabG__label(new_w) == NULL)
      {
          XmString string;
          XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;

          string   =  XmStringLtoRCreate(XrmQuarkToString(current->
						object.xrm_name), cset);
          LabG__label(new_w) =  _XmStringCreate(string);
          XtFree((char *) string);
      }
     else
      { if (_XmStringIsXmString( (XmString) LabG__label(new_w)))
           LabG__label(new_w) = _XmStringCreate( (XmString) LabG__label(new_w));
        else
        {  XmString string;
           XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;

           _XmWarning( (Widget) new_w, CS_STRING_MESSAGE);
            string = XmStringLtoRCreate(XrmQuarkToString(new_w->object.xrm_name),
                                        cset);
           LabG__label(new_w) =  _XmStringCreate(string);
           XtFree((char *) string);
        }
     }

      _XmStringFree(LabG__label(current));
      LabG__label(current)= NULL;
      LabG__label(req)= NULL;
   }


   if ((LabG__acceleratorText(new_w)!= LabG__acceleratorText(current)) &&
	((LabG_MenuType(new_w) == XmMENU_POPUP) ||
	 (LabG_MenuType(new_w) == XmMENU_PULLDOWN)))
   {
      /* BEGIN OSF Fix pir 1098 */
      newstring = TRUE;
      /* END OSF Fix pir 1098 */
      if (LabG__acceleratorText(new_w) != NULL)
      {
	 if (_XmStringIsXmString( (XmString) LabG__acceleratorText(new_w)))
	 {
	    if ((XmIsCascadeButtonGadget(new_w)) &&
		(LabG__acceleratorText(new_w) != NULL))
		LabG__acceleratorText(new_w) = NULL;
	    else
		LabG__acceleratorText(new_w) = _XmStringCreate( (XmString) LabG__acceleratorText(new_w));
	    _XmStringFree(LabG__acceleratorText(current));
	    LabG__acceleratorText(current)= NULL;
	    LabG__acceleratorText(req)= NULL;
	 }
	 else
	 {
	    _XmWarning( (Widget) new_w, ACC_MESSAGE);
	    LabG__acceleratorText(new_w) = NULL;
	 }
      }
      /* BEGIN OSF Fix pir 1098 */
      else
	LabG_MarginRight(new_w) = 0;
      /* END OSF Fix pir 1098 */
   }
   else
       LabG__acceleratorText(new_w) = LabG__acceleratorText(current);

   if (LabG_Font(new_w) != LabG_Font(current))
   {
	CleanupFontFlag = True;
      if (LabG_Font(new_w) == NULL)
      {
     if (XtClass(new_w) == xmLabelGadgetClass)
        LabG_Font(new_w) = _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST);
     else
       LabG_Font(new_w) = _XmGetDefaultFontList( (Widget) new_w, XmBUTTON_FONTLIST); 
      }
      LabG_Font(new_w) = XmFontListCopy (LabG_Font(new_w));

      if (LabG_IsText(new_w))
      {
        _XmStringUpdate (LabG_Font(new_w), LabG__label(new_w));
        if (LabG__acceleratorText(new_w) != NULL)
         _XmStringUpdate (LabG_Font(new_w), LabG__acceleratorText(new_w));
      }
   }


   /*  ReInitialize the interesting input types.  */

   new_w->gadget.event_mask = XmHELP_EVENT;

   new_w->gadget.event_mask |= 
        XmFOCUS_IN_EVENT | XmFOCUS_OUT_EVENT | XmENTER_EVENT | XmLEAVE_EVENT |
        XmBDRAG_EVENT;

   if ((LabG_MenuType(new_w) == XmMENU_POPUP) ||
       (LabG_MenuType(new_w) == XmMENU_PULLDOWN) ||
       (LabG_MenuType(new_w) == XmMENU_BAR))
       new_w->gadget.highlight_thickness = 0;

   if(    !XmRepTypeValidValue( XmRID_LABEL_TYPE, LabG_LabelType( new_w),
                                                             (Widget) new_w)    )
   {
       LabG_LabelType(new_w) = LabG_LabelType(current);
   }

    /* ValidateInputs(new_w); */

   if ((LabG_IsText(new_w) &&
	 ((newstring) ||
         (LabG_Font(new_w) != LabG_Font(current)))) ||
       (LabG_IsPixmap(new_w) &&
         ((LabG_Pixmap(new_w) != LabG_Pixmap(current)) ||
          (LabG_PixmapInsensitive(new_w) != LabG_PixmapInsensitive(current)) ||
          /* When you have different sized pixmaps for sensitive and */
          /* insensitive states and sensitivity changes, */
          /* the right size is chosen. (osfP2560) */
          (new_w->rectangle.sensitive != current->rectangle.sensitive) ||
          (new_w->rectangle.ancestor_sensitive != current->rectangle.ancestor_sensitive))) ||
       (LabG_LabelType(new_w) != LabG_LabelType(current)))
   {
     /*
      * Fix for CR 5419 - Only set Call_SetSize if the true sizes change.
      */
     _XmCalcLabelGDimensions((Widget) new_w);

     if ((LabG_AccTextRect(new_w).width != LabG_AccTextRect(current).width) ||
	 (LabG_AccTextRect(new_w).height != LabG_AccTextRect(current).height) ||
	 (LabG_TextRect(new_w).width != LabG_TextRect(current).width) ||
	 (LabG_TextRect(new_w).height != LabG_TextRect(current).height))
       {
	 if (LabG_RecomputeSize(new_w))
	   {
	     if (req->rectangle.width == current->rectangle.width)
	       new_w->rectangle.width = 0;
	     if (req->rectangle.height == current->rectangle.height)
	       new_w->rectangle.height = 0;
	   }
	 
	 Call_SetSize = True;
       }
     /* End fix for CR 5419 */

     flag = True;
   }
     
   if ((LabG_Alignment(new_w)!= LabG_Alignment(current)) ||
       (LabG_StringDirection(new_w) != LabG_StringDirection(current))) {

      if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, LabG_Alignment( new_w),
                                                             (Widget) new_w)    )
      {
          LabG_Alignment(new_w) = LabG_Alignment(current);
      }

      if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION, 
                                 LabG_StringDirection( new_w), (Widget) new_w)    )
      {
          LabG_StringDirection(new_w) = LabG_StringDirection(current);
      }
       
      Call_SetSize = True;
      
      flag = True;

   }

       
   if ((LabG_MarginHeight(new_w) != LabG_MarginHeight(current)) ||
       (LabG_MarginWidth(new_w) != LabG_MarginWidth(current)) ||
       (LabG_MarginRight(new_w) != LabG_MarginRight(current)) ||
       (LabG_MarginLeft(new_w)!= LabG_MarginLeft(current)) ||
       (LabG_MarginTop(new_w)!= LabG_MarginTop(current)) ||
       (LabG_MarginBottom(new_w)!= LabG_MarginBottom(current)) ||
       (new_w->gadget.shadow_thickness != current->gadget.shadow_thickness) ||
       (new_w->gadget.highlight_thickness != current->gadget.highlight_thickness) ||

       ((new_w->rectangle.width <= 0) || (new_w->rectangle.height <= 0)))
   {

      if (LabG_RecomputeSize(new_w))
      {
        if (req->rectangle.width == current->rectangle.width)
            new_w->rectangle.width = 0;
        if (req->rectangle.height == current->rectangle.height)
            new_w->rectangle.height = 0;
      }
   
      Call_SetSize = True;
      
      flag = True;
   }

  /* SetSize is called only if we need to calculate the dimensions or */
  /* coordinates  for the string.				      */

   if (Call_SetSize)     
    (* (((XmLabelGadgetClassRec *)(new_w->object.widget_class))->
		rect_class.resize)) ((Widget) new_w); 

/*
 * If the sensitivity has changed then we must redisplay.
 */
	if( (new_w->rectangle.sensitive != current->rectangle.sensitive) ||
        (new_w->rectangle.ancestor_sensitive !=
        current->rectangle.ancestor_sensitive) )
      {
	    flag = True;
	  }


   /*  Force the traversal flag when in a menu.  */

   if ((XtClass(new_w) == xmLabelGadgetClass) &&
       ((LabG_MenuType(new_w) == XmMENU_POPUP) ||
	(LabG_MenuType(new_w) == XmMENU_PULLDOWN) ||
	(LabG_MenuType(new_w) == XmMENU_OPTION)))
   {
      new_w->gadget.traversal_on = False;
      new_w->gadget.highlight_on_enter = False;
   }


   /*  Recreate the GC's if the font has been changed  */

   if (LabG_Font(new_w) != LabG_Font(current))
   {
      XtReleaseGC (XtParent (current), LabG_NormalGC(current));
      XtReleaseGC (XtParent (current), LabG_InsensitiveGC(current));
      SetNormalGC(new_w);
      flag = True;
   }

   if ((LabG_MenuType(new_w) != XmWORK_AREA) &&
       (LabG_Mnemonic(new_w) != LabG_Mnemonic(current)))
   {
      /* New grabs only required if mnemonic changes */
      ProcessFlag = TRUE;
      if (LabG_LabelType(new_w) == XmSTRING)
	 flag = TRUE;
   }

   if (LabG_MnemonicCharset(new_w) != LabG_MnemonicCharset(current)) 
   {
      if (LabG_MnemonicCharset(new_w))
        LabG_MnemonicCharset(new_w) = 
	    _XmStringCharsetCreate(LabG_MnemonicCharset (new_w));
      else
        LabG_MnemonicCharset(new_w) = 
	    _XmStringCharsetCreate(XmFONTLIST_DEFAULT_TAG);

      if ( LabG_MnemonicCharset (current) != NULL)
	 XtFree ( LabG_MnemonicCharset (current));

      if (LabG_LabelType(new_w) == XmSTRING)
	 flag = TRUE;
   }

   if (((LabG_MenuType(new_w) == XmMENU_POPUP) ||
	(LabG_MenuType(new_w) == XmMENU_PULLDOWN)) &&
       (LabG_Accelerator(new_w) != LabG_Accelerator(current)))
   {
      if (LabG_Accelerator(new_w) != NULL)
      {
         char *s;

         /* Copy the accelerator into local space */

         s = XtMalloc (XmStrlen (LabG_Accelerator(new_w)) + 1);
         strcpy (s, LabG_Accelerator(new_w));
         LabG_Accelerator(new_w) = s;
      }

      if (LabG_Accelerator(current) != NULL)
        XtFree(LabG_Accelerator(current));

      LabG_Accelerator(current) = NULL;
      LabG_Accelerator(req) = NULL;
      ProcessFlag = TRUE;
   }
   else
      LabG_Accelerator(new_w) = LabG_Accelerator(current);

   if (ProcessFlag)
      (* xmLabelGadgetClassRec.label_class.menuProcs)
         (XmMENU_PROCESS_TREE, (Widget) new_w, NULL, NULL, NULL);

   if (flag && (LabG_MenuType(new_w) == XmMENU_PULLDOWN))
      (* xmLabelGadgetClassRec.label_class.menuProcs)
         (XmMENU_MEMWIDGET_UPDATE, (Widget) new_w, NULL, NULL, NULL);

    if (CleanupFontFlag)
      if (LabG_Font(current)) XmFontListFree(LabG_Font(current));

   return (flag);
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
   if (curmw->manager.foreground != newmw->manager.foreground ||
       curmw->core.background_pixel != newmw->core.background_pixel)
   {
      XtReleaseGC (XtParent (gw), LabG_NormalGC(gw));
      XtReleaseGC (XtParent (gw), LabG_InsensitiveGC(gw));
      SetNormalGC((XmLabelGadget) gw);
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
        XmLabelGadget lg = (XmLabelGadget) wid ;
   if (event_mask & XmHELP_EVENT) Help ((Widget) lg, event);
   else if (event_mask & XmENTER_EVENT) _XmEnterGadget ((Widget) lg, event,
                                                                   NULL, NULL);
   else if (event_mask & XmLEAVE_EVENT) _XmLeaveGadget ((Widget) lg, event,
                                                                   NULL, NULL);
   else if (event_mask & XmFOCUS_IN_EVENT) _XmFocusInGadget ((Widget) lg,
                                                            event, NULL, NULL);
   else if (event_mask & XmFOCUS_OUT_EVENT) _XmFocusOutGadget ((Widget) lg,
                                                            event, NULL, NULL);
   else if (event_mask & XmBDRAG_EVENT) _XmProcessDrag ((Widget) lg,
                                                            event, NULL, NULL);
}




/************************************************************************
 *
 *  Help
 *	This routine is called if the user made a help selection 
 *      on the widget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( w, event )
        Widget w ;
        XEvent *event ;
#else
Help(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmLabelGadget lg = (XmLabelGadget) w;
    XmAnyCallbackStruct temp;

    if (LabG_MenuType(lg) == XmMENU_POPUP ||
	LabG_MenuType(lg) == XmMENU_PULLDOWN)
    {
      (* xmLabelGadgetClassRec.label_class.menuProcs)
	  (XmMENU_POPDOWN, XtParent(lg), NULL, event, NULL);
    }
    
    temp.reason = XmCR_HELP;
    temp.event  = event;

    _XmSocorro(w, event, NULL, NULL);
}


/************************************************************************
 *
 *  GetLabelString
 *     This is a get values hook function that returns the external
 *     form of the label string from the internal form.
 *
 ***********************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetLabelString( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
GetLabelString(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelGadget lw = (XmLabelGadget) wid ;
  XmString string;

  string = _XmStringCreateExternal (LabG_Font(lw), LabG__label(lw));

  *value = (XtArgVal) string;

}

/************************************************************************
 *
 *  GetAccelerator
 *     This is a get values hook function that returns a copy
 *     of the accelerator string.
 *
 ***********************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
GetAccelerator( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
GetAccelerator(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelGadget lw = (XmLabelGadget) wid ;
  String string;

  string = XtNewString(LabG_Accelerator(lw));

  *value = (XtArgVal) string;
}

/************************************************************************
 *
 *  GetAcceleratorText
 *     This is a get values hook function that returns the external
 *     form of the accelerator text from the internal form.
 *
 ***********************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetAcceleratorText( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
GetAcceleratorText(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelGadget lw = (XmLabelGadget) wid ;
  XmString string;

  string = _XmStringCreateExternal (LabG_Font(lw), LabG__acceleratorText(lw));

  *value = (XtArgVal) string;

}

/************************************************************************
 *
 *  _XmStringCharsetCreate
 *
 ************************************************************************/
static XmStringCharSet 
#ifdef _NO_PROTO
_XmStringCharsetCreate( stringcharset )
        XmStringCharSet stringcharset ;
#else
_XmStringCharsetCreate(
        XmStringCharSet stringcharset )
#endif /* _NO_PROTO */
{
    char    *cset;
    char    *string;
    int size;

    string = (char *) (stringcharset);
    size = strlen ( string);
    cset = XtMalloc (size +1);
    if (cset != NULL)
    strcpy( cset, string);
    return ( (XmStringCharSet) cset);
}


/************************************************************************
 *
 *  GetMnemonicCharset
 *     This is a get values hook function that returns the external
 *     form of the mnemonicCharset from the internal form.
 *  : Returns a string containg the mnemonicCharset.
 *    Caller must free the string .
 *
 ***********************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetMnemonicCharset( wid, resource, value )
        Widget wid ;
        XrmQuark resource ;
        XtArgVal *value ;
#else
GetMnemonicCharset(
        Widget wid,
        XrmQuark resource,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelGadget lw = (XmLabelGadget) wid ;
  char *cset;
  int   size;

  cset = NULL;
  if (LabG_MnemonicCharset (lw))
    { size = strlen (LabG_MnemonicCharset (lw));
      if (size > 0)
     cset = (char *) (_XmStringCharsetCreate(LabG_MnemonicCharset (lw)));
    }

  *value = (XtArgVal) cset;

}


/************************************************************************
 *
 *  Caching Assignment help
 *     These routines are for manager widgets that go into Label's
 *     fields and set them, instead of doing a SetValues.
 *
 ************************************************************************/
static XmLabelGCacheObjPart local_cache; 
static Boolean local_cache_inited = FALSE;

/*
 * QualifyLabelLocalCache
 *  Checks to see if local cache is set up
 */
static void 
#ifdef _NO_PROTO
QualifyLabelLocalCache( w )
        XmLabelGadget w ;
#else
QualifyLabelLocalCache(
        XmLabelGadget w )
#endif /* _NO_PROTO */
{
    if (!local_cache_inited) 
    { 
        local_cache_inited = TRUE; 
        ClassCacheCopy(LabG_ClassCachePart(w))(LabG_Cache(w), &local_cache, 
            sizeof(local_cache)); 
    }
}


/************************************************************************
 *
 * _XmReCacheLabG()
 * Check to see if ReCaching is necessary as a result of fields having
 * been set by a mananger widget. This routine is called by the
 * manager widget in their SetValues after a change is made to any
 * of Label's cached fields.
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmReCacheLabG( wid )
        Widget wid ;
#else
_XmReCacheLabG(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmLabelGadget lw = (XmLabelGadget) wid ;
     if (local_cache_inited &&
        (!_XmLabelCacheCompare((XtPointer)&local_cache, (XtPointer)LabG_Cache(lw)))) 
     {	
           _XmCacheDelete( (XtPointer) LabG_Cache(lw));	/* delete the old one */
	   LabG_Cache(lw) = (XmLabelGCacheObjPart *)_XmCachePart(
	       LabG_ClassCachePart(lw), (XtPointer) &local_cache, sizeof(local_cache));
     } 
     local_cache_inited = FALSE;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginHeight( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginHeight(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_height = value;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginWidth( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginWidth(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_width = value;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginLeft( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginLeft(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_left = value;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginRight( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginRight(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_right = value;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginTop( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginTop(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_top = value;
}

void 
#ifdef _NO_PROTO
_XmAssignLabG_MarginBottom( lw, value )
        XmLabelGadget lw ;
        Dimension value ;
#else
_XmAssignLabG_MarginBottom(
        XmLabelGadget lw,
#if NeedWidePrototypes
        int value )
#else
        Dimension value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
       QualifyLabelLocalCache(lw);
       local_cache.margin_bottom = value;
}

/************************************************************************
 *
 *  SetOverrideCallback
 *
 * Used by subclasses.  If this is set true, then there is a RowColumn
 * parent with the entryCallback resource set.  The subclasses do not
 * do their activate callbacks, instead the RowColumn callbacks are called
 * by RowColumn.
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SetOverrideCallback( w )
        Widget w ;
#else
SetOverrideCallback(
        Widget w )
#endif /* _NO_PROTO */
{
   QualifyLabelLocalCache((XmLabelGadget) w);
   local_cache.skipCallback= True;
   _XmReCacheLabG(w);
}


/************************************************************************
 *
 *  XmCreateLabelGadget
 *	Externally accessable function for creating a label gadget.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateLabelGadget( parent, name, arglist, argCount )
        Widget parent ;
        char *name ;
        Arg *arglist ;
        Cardinal argCount ;
#else
XmCreateLabelGadget(
        Widget parent,
        char *name,
        Arg *arglist,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget(name, xmLabelGadgetClass, parent, arglist, argCount));
}

/*
 *  GetLabelBGClassSecResData ( ) 
 *    Class function to be called to copy secondary resource for external
 *  use.  i.e. copy the cached resources and send it back.
 */
/* ARGSUSED */
static Cardinal 
#ifdef _NO_PROTO
GetLabelBGClassSecResData( w_class, data_rtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **data_rtn ;
#else
GetLabelBGClassSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **data_rtn )
#endif /* _NO_PROTO */
{   int arrayCount;
    XmBaseClassExt  bcePtr;
    String  resource_class, resource_name;
    XtPointer  client_data;

    bcePtr = &( labelBaseClassExtRec);
    client_data = NULL;
    resource_class = NULL;
    resource_name = NULL;
    arrayCount =
      _XmSecondaryResourceData ( bcePtr, data_rtn, client_data,  
				resource_name, resource_class,
			        GetLabelClassResBase) ;

    return (arrayCount);

}

/*
 * GetLabelClassResBase ()
 *   retrun the address of the base of resources.
 *   - Not yet implemented.
 */
static XtPointer 
#ifdef _NO_PROTO
GetLabelClassResBase( widget, client_data )
        Widget widget ;
        XtPointer client_data ;
#else
GetLabelClassResBase(
        Widget widget,
        XtPointer client_data )
#endif /* _NO_PROTO */
{   XtPointer  widgetSecdataPtr;
    int  labg_cache_size = sizeof (XmLabelGCacheObjPart);
	char *cp;

	widgetSecdataPtr = (XtPointer) (XtMalloc ( labg_cache_size +1));

    if (widgetSecdataPtr)
      { cp = (char *) widgetSecdataPtr;
        memcpy( cp, LabG_Cache(widget), labg_cache_size);
      }

	return (widgetSecdataPtr);

}

static void 
#ifdef _NO_PROTO
SetValuesAlmost( cw, nw, request, reply )
        Widget cw ;
        Widget nw ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
SetValuesAlmost(
        Widget cw,
        Widget nw,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
        XmLabelGadget new_w = (XmLabelGadget) nw ;

    (* (((XmLabelGadgetClassRec *)(new_w->object.widget_class))->
		rect_class.resize)) ((Widget) new_w); 
   *request = *reply;

}

/************************************************************************
 *
 * XmLabelGadgetGetBaselines
 *
 * A Class function which when called returns True, if the widget has
 * a baseline and also determines the number of pixels from the y
 * origin to the first line of text and assigns it to the variable
 * being passed in.
 *
 ************************************************************************/

static Boolean
#ifdef _NO_PROTO
XmLabelGadgetGetBaselines(wid, baselines, line_count)
        Widget wid;
        Dimension **baselines;
        int *line_count;
#else
XmLabelGadgetGetBaselines(
        Widget wid,
        Dimension **baselines,
        int *line_count)
#endif /* _NO_PROTO */
{
  XmString string, string2;
  XmString string1 = NULL;
  XmStringContext context = NULL;
  char* text1;
  char* text2;
  XmStringCharSet char_set1, char_set2;
  XmStringDirection direction1, direction2;
  XmFontList FontList;
  Boolean separator1, separator2;
  Dimension *base_array;
  Dimension Offset;
  int index;
  XmLabelGadget lw = (XmLabelGadget)wid;

  if (LabG_IsPixmap(wid))
  {
    return (False);
  }
  else
  {
    index = 0;
    FontList = LabG_Font(lw);
    string = _XmStringCreateExternal (LabG_Font(lw), LabG__label(lw));

    if (!XmStringInitContext (&context, string))
      return (False);

    *line_count = XmStringLineCount(string);

    base_array = (Dimension *)XtMalloc((sizeof(Dimension) * (*line_count)));

    Offset = ((XmLabelGadget) wid)->rectangle.y + ((XmLabelGadget) wid)->label.TextRect.y;

    while (XmStringGetNextSegment (context, &text1, &char_set1, &direction1,
                                   &separator1))
    {
      if (string1)
	XmStringFree(string1);

      string1 = XmStringCreate(text1, char_set1);
      XtFree(text1);

      if (separator1)
      {
          while (XmStringPeekNextComponent(context)== XmSTRING_COMPONENT_SEPARATOR)
	  {
	      XmStringGetNextComponent (context, &text1, &char_set1, &direction1,
				NULL, NULL, NULL);
              base_array[index++] = Offset + XmStringBaseline (FontList, string1);
              Offset += XmStringHeight (FontList, string1);
	  }
      }
      else if (XmStringGetNextSegment (context, &text2, &char_set2, &direction2,
                         &separator2))
      {
        if (separator2)
        {
          string2 = XmStringCreate(text2, char_set2);
          string1 = XmStringConcat(string1, string2);
          base_array[index++] = Offset + XmStringBaseline (FontList, string1);
          Offset += XmStringHeight (FontList, string1);
        }
        else
        {
          string2 = XmStringCreate(text2, char_set2);
          string1 = XmStringConcat(string1, string2);
        }

	XtFree(text2);
	XmStringFree(string2);
	XtFree(char_set2);
      }
      else
      {
        XtFree(char_set1);
	break;
      }

      XtFree(char_set1);
    }
    base_array[index++] = Offset + XmStringBaseline (FontList, string1);

    XmStringFree(string1);

    *baselines = base_array;

    XmStringFreeContext(context);
    XmStringFree(string);

    return (True);
  }

}

/************************************************************************
 *
 * XmLabelGadgetGetDisplayRect
 *
 * A Class function which returns true if the widget being passed in
 * has a display rectangle associated with it. It also determines the
 * x,y coordinates of the character cell or pixmap relative to the origin,e
 * and the width and height in pixels of the smallest rectangle that encloses
 * the text or pixmap. This is assigned to the variable being passed in
 *
 ***********************************************************************/
static Boolean
#ifdef _NO_PROTO
XmLabelGadgetGetDisplayRect(w, displayrect)
        Widget w;
        XRectangle *displayrect;
#else
XmLabelGadgetGetDisplayRect(
        Widget w,
        XRectangle *displayrect)
#endif /* _NO_PROTO */
{
    XmLabelGadget wid = (XmLabelGadget) w;

  (*displayrect).x = wid->rectangle.x + wid->label.TextRect.x;
  (*displayrect).y = wid->rectangle.y + wid->label.TextRect.y;
  (*displayrect).width = wid->label.TextRect.width;
  (*displayrect).height = wid->label.TextRect.height;

  return (TRUE);
}

static Widget
#ifdef _NO_PROTO
GetPixmapDragIcon( w )
        Widget w ;
#else
GetPixmapDragIcon(
        Widget w )
#endif /* _NO_PROTO */
{
    XmLabelGadget lw = (XmLabelGadget) w;
    XmManagerWidget mw = (XmManagerWidget) XtParent(lw);
    Arg args[10];
    int n = 0;
    Widget drag_icon;
    Widget screen_object = XmGetXmScreen(XtScreen(w));
    unsigned int wid, hei, d, junk;
	
    /* it's a labelPixmap, use directly the pixmap */

    XGetGeometry (XtDisplay(w), Pix(lw),
		  (Window *) &junk,    /* returned root window */
		  (int *) &junk, (int *) &junk, /* x, y of pixmap */
		  &wid, &hei,       /* width, height of pixmap */
		  &junk,  /* border width */
		  &d);    /* depth */
    n = 0 ;
    XtSetArg(args[n], XmNhotX, 0);  n++;
    XtSetArg(args[n], XmNhotY, 0);  n++;
    XtSetArg(args[n], XmNwidth, wid);  n++;
    XtSetArg(args[n], XmNheight, hei);  n++;
    XtSetArg(args[n], XmNmaxWidth, wid);  n++;
    XtSetArg(args[n], XmNmaxHeight, hei);  n++;
    XtSetArg(args[n], XmNdepth, d);  n++;
    XtSetArg(args[n], XmNpixmap, Pix(lw));  n++;
    XtSetArg(args[n], XmNforeground, mw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNbackground, mw->manager.foreground);  n++;
    drag_icon = XtCreateWidget("drag_icon", xmDragIconObjectClass,
			       screen_object, args, n);
    return drag_icon ;
}


/* ARGSUSED */
void
#ifdef _NO_PROTO
_XmProcessDrag( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmProcessDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmLabelGadget lw = (XmLabelGadget) w;
    Atom targets[3];
    Cardinal num_targets = 0;
    Widget drag_icon;
    Arg args[10];
    int n;
    XmManagerWidget mw;
#if defined(CDE_NO_DRAG_FROM_LABELS)
    Boolean unselectable_drag;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplayOfObject(w)), "enableUnselectableDrag",
         &unselectable_drag, NULL);
    if (!unselectable_drag)
	return;
#endif /* defined(CDE_NO_DRAG_FROM_LABELS) */

    mw  = (XmManagerWidget) XtParent(lw);

   /* add targets that you believe you can convert To */
    if (LabG_IsPixmap(lw))
    {
      targets[0] = XA_PIXMAP;
                                                              num_targets++;
    }
    else
    {
      targets[0] = XmInternAtom(XtDisplay(lw), "COMPOUND_TEXT", False);
                                                              num_targets++;
    }
    
    n = 0;
    XtSetArg(args[n], XmNcursorBackground, mw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNcursorForeground, mw->manager.foreground);  n++;

    /* if it's a labelPixmap, only specify the pixmap icon */
    if (LabG_IsPixmap(lw) && (Pix(lw) != XmUNSPECIFIED_PIXMAP)) {
	drag_icon = GetPixmapDragIcon(w);
	XtSetArg(args[n], XmNsourcePixmapIcon, drag_icon); n++;
    } else {
	drag_icon = _XmGetTextualDragIcon(w);
	XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++; 
    }
    
    XtSetArg(args[n], XmNexportTargets, targets);  n++;
    XtSetArg(args[n], XmNnumExportTargets, num_targets);  n++;
    XtSetArg(args[n], XmNconvertProc, Convert);  n++;
    XtSetArg(args[n], XmNclientData, w);  n++;
    XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
    (void) XmDragStart(w, event, args, n);
}

/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
Convert( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
Convert(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmLabelGadget lw;
    XmManagerWidget mw  = (XmManagerWidget) XtParent(w);
    Display *display = XtDisplay(w);
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(mw), "COMPOUND_TEXT", False);
    Atom TIMESTAMP = XmInternAtom(XtDisplay(mw), "TIMESTAMP", False);
    Atom TARGETS = XmInternAtom(XtDisplay(mw), "TARGETS", False);
    Atom MOTIF_DROP = XmInternAtom(XtDisplay(mw), "_MOTIF_DROP", False);
    Atom BACKGROUND = XmInternAtom(display, "BACKGROUND", False);
    Atom FOREGROUND = XmInternAtom(display, "FOREGROUND", False);
    Atom PIXEL = XmInternAtom(display, "PIXEL", False);
    int target_count = 0;
    XtPointer c_ptr;
    Arg args[1];
    int MAX_TARGS = 10;
    XrmValue    from_val;
    XrmValue    to_val;
    Boolean     ok = FALSE;
    register total_size = 0;
    char *total = NULL;

    if (*selection == MOTIF_DROP) {
       XtSetArg (args[0], XmNclientData, &c_ptr);
       XtGetValues (w, args, 1);
       lw = (XmLabelGadget) c_ptr;
    } else
       return False;

    if (lw == NULL) return False;

    if (*target == TARGETS) {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));


      *value = (XtPointer) targs;
      *targs++ = TARGETS; target_count++;
      *targs++ = TIMESTAMP; target_count++;
      if (LabG_IsPixmap(lw)) 
      {
        *targs++ = XA_PIXMAP; target_count++;
        *targs++ = BACKGROUND; target_count++;
        *targs++ = FOREGROUND; target_count++;
        *targs++ = XA_COLORMAP; target_count++;
      } 
      else 
      {
        *targs++ = COMPOUND_TEXT; target_count++;
      }
      *type = XA_ATOM;
      *length = target_count;
      *format = 32;
      return True;
   }
   if (*target == COMPOUND_TEXT) {
      *type = COMPOUND_TEXT;
      *format = 8;
      from_val.addr = (char *)_XmStringCreateExternal (lw->label.font, lw->label._label);
      ok = _XmCvtXmStringToCT(&from_val, &to_val);
      total = (char *) to_val.addr;
      total_size = to_val.size;
    }
    if (ok)
    {
         *value = (char*)total;
         *length = total_size;
         return True;
    }
    if (*target == XA_PIXMAP) {
     /* Get widget's pixmap */
      Pixmap *pix;

      pix = (Pixmap *) XtMalloc(sizeof(Pixmap));
      *pix = lw->label.pixmap;
    /* value, type, length, and format must be set */
      *value = (XtPointer) pix;
      *type = XA_DRAWABLE;
      *length = sizeof(Pixmap);
      *format = 32;
      return True;
    }
    
    if (*target == BACKGROUND) {
     /* Get widget's background */
      Pixel *background;

      background = (Pixel *) XtMalloc(sizeof(Pixel));
      *background = mw->core.background_pixel;
    /* value, type, length, and format must be set */
      *value = (XtPointer) background;
      *type = PIXEL;
      *length = sizeof(Pixel);
      *format = 32;
      return True;
    }

    if (*target == FOREGROUND) {
     /* Get widget's foreground */
      Pixel *foreground;

      foreground = (Pixel *) XtMalloc(sizeof(Pixel));
      *foreground = mw->manager.foreground;
    /* value, type, length, and format must be set */
      *value = (XtPointer) foreground;
      *type = PIXEL;
      *length = sizeof(Pixel);
      *format = 32;
      return True;
    }

    if (*target == XA_COLORMAP) {
     /* Get widget's foreground */
      Colormap *colormap;

      colormap = (Colormap *) XtMalloc(sizeof(Colormap));
      *colormap = mw->core.colormap;
    /* value, type, length, and format must be set */
      *value = (XtPointer) colormap;
      *type = XA_COLORMAP;
      *length = sizeof(Pixel);
      *format = 32;
      return True;
    }

    return False;
}
