#pragma ident	"@(#)m1.2libs:Xm/Label.c	1.3"
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

#include <Xm/LabelP.h>
#include <Xm/BaseClassP.h>
#include "XmI.h"
#include <X11/keysymdef.h>
#include <X11/ShellP.h>
#include <Xm/MenuUtilP.h>
#include <X11/Xatom.h>
#include "MessagesI.h"
#include "RepTypeI.h"
#include "GMUtilsI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/DragIconP.h>
#include <Xm/AtomMgr.h>
#include <Xm/ScreenP.h>
#include <Xm/XmosP.h>
#include <stdio.h>
#include <ctype.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define Pix(w)			((w)->label.pixmap)
#define Pix_insen(w)		((w)->label.pixmap_insen)

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
static void InitializePrehook() ;
static void InitializePosthook() ;
static void SetNormalGC() ;
static void SetSize() ;
static void Initialize() ;
static XtGeometryResult QueryGeometry() ;
static void Destroy() ;
static void Redisplay() ;
static void Enter() ;
static void Leave() ;
static Boolean SetValues() ;
static void SetOverrideCallback() ;
static void Help() ;
static void GetLabelString() ;
static void GetAccelerator() ;
static void GetAcceleratorText() ;
static XmStringCharSet _XmStringCharSetCreate() ;
static void GetMnemonicCharSet() ;
static void SetValuesAlmost() ;
static Boolean XmLabelGetDisplayRect() ;
static Boolean XmLabelGetBaselines() ;
static Widget GetPixmapDragIcon() ;
static void ProcessDrag() ;
static Boolean Convert() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass c) ;
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
static void SetNormalGC( 
                        XmLabelWidget lw) ;
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
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void SetOverrideCallback( 
                        Widget wid) ;
static void Help( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void GetLabelString( 
                        Widget wid,
                        XrmQuark resource,
                        XtArgVal *value) ;
static void GetAccelerator( 
                        Widget wid,
                        XrmQuark resource,
                        XtArgVal *value) ;
static void GetAcceleratorText( 
                        Widget wid,
                        XrmQuark resource,
                        XtArgVal *value) ;
static XmStringCharSet _XmStringCharSetCreate( 
                        XmStringCharSet stringcharset) ;
static void GetMnemonicCharSet( 
                        Widget wid,
                        XrmQuark resource,
                        XtArgVal *value) ;
static void SetValuesAlmost( 
                        Widget cw,
                        Widget nw,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static Boolean XmLabelGetDisplayRect( 
                        Widget w,
                        XRectangle *displayrect) ;
static Boolean XmLabelGetBaselines( 
                        Widget wid,
                        Dimension **baselines,
                        int *line_count) ;
static Widget GetPixmapDragIcon( 
                        Widget w) ;
static void ProcessDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Boolean Convert( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/* default translations and action recs */

static XtTranslations default_parsed;

#define defaultTranslations	_XmLabel_defaultTranslations

static XtTranslations menu_parsed;

#define menuTranslations	_XmLabel_menuTranslations


static XtActionsRec ActionsList[] = {
       {"Enter",		Enter},
       {"Leave",		Leave},
       {"Help",			Help},
       {"ProcessDrag",	 	ProcessDrag},
};


/* Here are the translations used by the subclasses for menu traversal */
/* The matching actions are defined in RowColumn.c                     */

#define menu_traversal_events	_XmLabel_menu_traversal_events


/* here are the resources that this widget adds */


static XtResource resources[] = 
{
 
    {
        XmNshadowThickness,
        XmCShadowThickness,
        XmRHorizontalDimension,
        sizeof (Dimension),
        XtOffsetOf( struct _XmLabelRec, primitive.shadow_thickness),
        XmRImmediate,
        (XtPointer) 0
    },

    {
         XmNalignment,
         XmCAlignment,
         XmRAlignment,
         sizeof(unsigned char),
	 XtOffsetOf( struct _XmLabelRec,label.alignment),
         XmRImmediate, 
	 (XtPointer) XmALIGNMENT_CENTER
    },

    {
         XmNlabelType,
         XmCLabelType,
         XmRLabelType,
         sizeof(unsigned char),
	 XtOffsetOf( struct _XmLabelRec,label.label_type),
         XmRImmediate,
         (XtPointer) XmSTRING
    },

    {
 	XmNmarginWidth, 
	XmCMarginWidth, 
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_width), 
	XmRImmediate,
	(XtPointer) 2
    },

    {
	XmNmarginHeight, 
	XmCMarginHeight, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_height),
	XmRImmediate,
	(XtPointer) 2
    },

    {
	XmNmarginLeft, 
	XmCMarginLeft, 
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_left), 
	XmRImmediate,
	(XtPointer) 0
    },

    {
	XmNmarginRight, 
	XmCMarginRight, 
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_right), 
	XmRImmediate,
	(XtPointer) 0
    },

    {
	XmNmarginTop, 
	XmCMarginTop, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_top), 
	XmRImmediate,
	(XtPointer) 0
    },

    {
	XmNmarginBottom, 
	XmCMarginBottom, 
	XmRVerticalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmLabelRec, label.margin_bottom), 
	XmRImmediate,
	(XtPointer) 0
    },


    {
         XmNfontList,
         XmCFontList,
         XmRFontList,
         sizeof(XmFontList),
         XtOffsetOf( struct _XmLabelRec,label.font),
         XmRImmediate,
	 (XtPointer) NULL
    },

    {
         XmNlabelPixmap,
         XmCLabelPixmap,
         XmRPrimForegroundPixmap,
         sizeof(Pixmap),
         XtOffsetOf( struct _XmLabelRec,label.pixmap),
         XmRImmediate,
         (XtPointer) XmUNSPECIFIED_PIXMAP
    },

    {
         XmNlabelInsensitivePixmap,
         XmCLabelInsensitivePixmap,
         XmRPrimForegroundPixmap,
         sizeof(Pixmap),
         XtOffsetOf( struct _XmLabelRec,label.pixmap_insen),
         XmRImmediate,
   	 (XtPointer) XmUNSPECIFIED_PIXMAP 
    },


    {    
         XmNlabelString,
         XmCXmString,
         XmRXmString,
         sizeof(_XmString),
         XtOffsetOf( struct _XmLabelRec, label._label),
         XmRImmediate,
         (XtPointer) NULL
    },

    {
         XmNmnemonic,
         XmCMnemonic,
         XmRKeySym,
         sizeof(KeySym),
         XtOffsetOf( struct _XmLabelRec,label.mnemonic),
         XmRImmediate,
         (XtPointer) NULL
    },

     {
          XmNmnemonicCharSet,
          XmCMnemonicCharSet,
          XmRString,
          sizeof(XmStringCharSet),
          XtOffsetOf( struct _XmLabelRec,label.mnemonicCharset),
          XmRImmediate,
	  (XtPointer) XmFONTLIST_DEFAULT_TAG    
     },

    {
         XmNaccelerator,
         XmCAccelerator,
         XmRString,
         sizeof(char *),
         XtOffsetOf( struct _XmLabelRec,label.accelerator),
         XmRImmediate,
         (XtPointer) NULL
    },

    {
         XmNacceleratorText,
         XmCAcceleratorText,
         XmRXmString,
         sizeof(_XmString),
         XtOffsetOf( struct _XmLabelRec,label._acc_text),
         XmRImmediate,
         (XtPointer) NULL
    },

   { 
        XmNrecomputeSize,
        XmCRecomputeSize,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmLabelRec,label.recompute_size),
        XmRImmediate,
        (XtPointer) True
   },

   { 
        XmNstringDirection,
        XmCStringDirection,
        XmRStringDirection,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmLabelRec,label.string_direction),
        XmRImmediate,
        (XtPointer) XmSTRING_DIRECTION_DEFAULT 
   },

   {
        XmNtraversalOn,
        XmCTraversalOn,
        XmRBoolean,
        sizeof (Boolean),
        XtOffsetOf( struct _XmPrimitiveRec, primitive.traversal_on),
        XmRImmediate, 
        (XtPointer) False
    },

    {
        XmNhighlightThickness,
        XmCHighlightThickness,
        XmRHorizontalDimension,
        sizeof (Dimension),
        XtOffsetOf( struct _XmPrimitiveRec, primitive.highlight_thickness),
        XmRImmediate, 
        (XtPointer) 0
    },

};



/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
   { 
     XmNmarginWidth, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_width), 
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels 
     },

   { 
     XmNmarginHeight, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_height),
     _XmFromVerticalPixels, 
     _XmToVerticalPixels 
     },

   { 
     XmNmarginLeft, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_left), 
     _XmFromHorizontalPixels, 
     _XmToHorizontalPixels 
     },

   { 
     XmNmarginRight, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_right), 
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels
     },

   { 
     XmNmarginTop, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_top), 
     _XmFromVerticalPixels,
     _XmToVerticalPixels
     },

   { 
     XmNmarginBottom, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmLabelRec, label.margin_bottom),
     _XmFromVerticalPixels,
     _XmToVerticalPixels
     },

   { XmNlabelString, 
     sizeof (_XmString),
     XtOffsetOf( struct _XmLabelRec, label._label),
     GetLabelString,
     NULL
     },

   { XmNmnemonicCharSet,
     sizeof (XmStringCharSet),
     XtOffsetOf( struct _XmLabelRec, label.mnemonicCharset),
     GetMnemonicCharSet,
     NULL
     },

   { XmNaccelerator,
     sizeof (String),
     XtOffsetOf( struct _XmLabelRec, label.accelerator),
     GetAccelerator,
     NULL
     },

   { XmNacceleratorText, 
     sizeof (_XmString),
     XtOffsetOf( struct _XmLabelRec, label._acc_text),
     GetAcceleratorText,
     NULL
     },
};


/* this is the class record that gets set at compile/link time */
/* this is what is passed to the widgetcreate routine as the   */
/* the class.  All fields must be inited at compile time       */

static XmBaseClassExtRec       labelBaseClassExtRec = {
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
    NULL,                                     /* classPartInitPrehook */
    NULL,                                     /* classPartInitPosthook*/
    NULL,                                     /* ext_resources        */
    NULL,                                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    XmInheritWidgetNavigable,                 /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
};

XmPrimitiveClassExtRec _XmLabelPrimClassExtRec = {
    NULL,
    NULLQUARK,
    XmPrimitiveClassExtVersion,
    sizeof(XmPrimitiveClassExtRec),
    XmLabelGetBaselines,                 /* widget_baseline */
    XmLabelGetDisplayRect,               /* widget_display_rect */
    NULL,				 /* widget_margins */
};

externaldef ( xmlabelclassrec) XmLabelClassRec xmLabelClassRec = {
  {
    /* superclass	  */	(WidgetClass) &xmPrimitiveClassRec,
    /* class_name	  */	"XmLabel",
    /* widget_size	  */	sizeof(XmLabelRec),
    /* class_initialize   */    ClassInitialize,
    /* chained class init */	ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize hook    */    NULL,
    /* realize		  */	XtInheritRealize,
    /* actions		  */	ActionsList,
    /* num_actions	  */	XtNumber(ActionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	XtExposeCompressMaximal,  
    /* compress enter/exit*/    TRUE,
    /* visible_interest	  */	FALSE,
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
    /* extension record   */    (XtPointer)&labelBaseClassExtRec
  },

  {				        /* Xmprimitive        */
      XmInheritBorderHighlight,         /* border_highlight   */
      XmInheritBorderUnhighlight,       /* border_unhighlight */
      XtInheritTranslations,            /* translations       */
      NULL,                             /* arm_and_activate   */
      syn_resources,   	    		/* syn resources      */
      XtNumber(syn_resources),		/* num syn_resources  */
      (XtPointer)&_XmLabelPrimClassExtRec,  /* extension          */
  },

  {					   /* XmLabel */
        SetOverrideCallback,               /* override_callback             */
	NULL,				   /* menu procedure interface      */
        NULL,				   /* translations                  */
	NULL,				   /* extension record              */

  }
};

externaldef( xmlabelwidgetclass) WidgetClass xmLabelWidgetClass =  
				(WidgetClass) &xmLabelClassRec;


/*********************************************************************
 *
 * ClassInitialize
 *       This is the class initialization routine.  It is called only
 *       the first time a widget of this class is initialized.
 *
 ********************************************************************/         
/* ARGSUSED */
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
   labelBaseClassExtRec.record_type = XmQmotif;

   xmLabelClassRec.label_class.translations =
     (String) (XtParseTranslationTable(menu_traversal_events));
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

/*********************************************************************
 *
 *  ClassPartInitialize
 *      Processes the class fields which need to be inherited.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( c )
        WidgetClass c ;
#else
ClassPartInitialize(
        WidgetClass c )
#endif /* _NO_PROTO */
{
    register XmLabelWidgetClass wc = (XmLabelWidgetClass) c ;
    XmLabelWidgetClass super = (XmLabelWidgetClass)wc->core_class.superclass;

    if (wc->label_class.setOverrideCallback == XmInheritSetOverrideCallback)
	wc->label_class.setOverrideCallback = 
	    super->label_class.setOverrideCallback;

    if (wc->label_class.translations == XtInheritTranslations )
	wc->label_class.translations = super->label_class.translations;

    _XmFastSubclassInit (wc, XmLABEL_BIT);

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

  if (new_w->core.widget_class->core_class.tm_table != NULL)
    return;

  _XmSaveCoreClassTranslations (new_w);

  if (XmIsRowColumn(XtParent(new_w)))
  {
    Arg arg[1];
    XtSetArg (arg[0], XmNrowColumnType, &type);
    XtGetValues (XtParent(new_w), arg, 1);
  }

  else 
    type = XmWORK_AREA;

  if (type == XmWORK_AREA)
    new_w->core.widget_class->core_class.tm_table = (String) default_parsed;

  else 
    new_w->core.widget_class->core_class.tm_table = (String) menu_parsed;
}


/************************************************************************
 *
 *  SetNormalGC
 *      Create the normal and insensitive GC's for the gadget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetNormalGC( lw )
        XmLabelWidget lw ;
#else
SetNormalGC(
        XmLabelWidget lw )
#endif /* _NO_PROTO */
{
        XGCValues       values;
        XtGCMask        valueMask;
        XFontStruct     *fs = (XFontStruct *) NULL;

        valueMask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;

       _XmFontListGetDefaultFont(lw->label.font, &fs);
        values.foreground = lw->primitive.foreground;
        values.background = lw->core.background_pixel;
	values.graphics_exposures = False;

	if (fs==NULL)
	  valueMask &= ~GCFont;
	else
          values.font     = fs->fid;

        lw->label.normal_GC = XtGetGC((Widget) lw,valueMask,&values);

        valueMask |= GCFillStyle | GCTile;
        values.fill_style = FillTiled;
	/* "50_foreground" is in the installed set and should always be found;
	** omit check for XmUNSPECIFIED_PIXMAP
	*/
        values.tile = XmGetPixmapByDepth
				(XtScreen((Widget)(lw)), "50_foreground",
				lw->primitive.foreground,
				lw->core.background_pixel,
				lw->core.depth);
        lw->label.insensitive_GC = XtGetGC((Widget) lw, valueMask, &values);

}

 /************************************************************************
  *
  * _XmCalcLabelDimensions()
  *   Calculates the dimensions of the label text and pixmap, and updates
  *   the TextRect fields appropriately. Called at Initialize and SetValues.
  *   Also called by subclasses to recalculate label dimensions.
  *
  ************************************************************************/
void 
#ifdef _NO_PROTO
_XmCalcLabelDimensions( wid )
        Widget wid ;
#else
_XmCalcLabelDimensions(
        Widget wid )
#endif /* _NO_PROTO */
{
  XmLabelWidget newlw = (XmLabelWidget) wid ;
  XmLabelPart        *lp;

  lp = &(newlw->label);

   /* initialize TextRect width and height to 0, change later if needed */

   lp->TextRect.width = 0;
   lp->TextRect.height = 0;
   lp->acc_TextRect.width = 0;
   lp->acc_TextRect.height = 0;

   if (Lab_IsPixmap(newlw))  /* is a pixmap so find out */
                                       /* how big it is */
   {

      if ((newlw->core.sensitive) && (newlw->core.ancestor_sensitive))
      {
         if (Pix(newlw) != XmUNSPECIFIED_PIXMAP)
         {
            unsigned int junk;
            unsigned int  w = 0 , h = 0, d;

            XGetGeometry (XtDisplay(newlw),
                          Pix(newlw),
                          (Window *) &junk,    /* returned root window */
                          (int *) &junk, (int *) &junk,        /* x, y of pixmap */
                          &w, &h,       /* width, height of pixmap */
                          &junk,  /* border width */
                          &d);          /* depth */

            lp->TextRect.width = (unsigned short) w;
            lp->TextRect.height = (unsigned short) h;
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
                          (Window *) &junk,          /* returned root window */
                          (int *) &junk, (int *) &junk,   /* x, y of pixmap */
                          &w, &h,        /* width, height of pixmap */
                          &junk,          /* border width */
                          &d);          /* depth */

            lp->TextRect.width = (unsigned short) w;
            lp->TextRect.height = (unsigned short) h;
         }
      }
      if (lp->_acc_text != NULL)
      {
         Dimension w,h ;

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
   }
   else
       if (Lab_IsText(newlw))
       {
          Dimension w, h;

          if (!_XmStringEmpty (lp->_label)) {

             /*
              * If we have a string then size it.
              */
             _XmStringExtent(lp->font, lp->_label, &w, &h);
             lp->TextRect.width = (unsigned short)w;
             lp->TextRect.height = (unsigned short)h;
          }

          if (lp->_acc_text != NULL) {
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
       }  /* else */
}       
 /************************************************************************
 *
 *  SetSize
 *      Sets new width, new height, and new label.TextRect
 *      appropriately. This routine is called by Initialize and
 *      SetValues.
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
        XmLabelWidget newlw = (XmLabelWidget) wid ;
   XmLabelPart        *lp;

   lp = &(newlw->label);
   
   /* increase margin width if necessary to accomodate accelerator text */
   if (lp->_acc_text != NULL)
       if (lp->margin_right < lp->acc_TextRect.width + LABEL_ACC_PAD)
       {
          lp->margin_right = lp->acc_TextRect.width + LABEL_ACC_PAD;
       }

   /* Has a width been specified?  */
   
   if (newlw->core.width == 0)
       newlw->core.width = (Dimension)
	   lp->TextRect.width + 
	       lp->margin_left + lp->margin_right +
		   (2 * (lp->margin_width 
			 + newlw->primitive.highlight_thickness
			 + newlw->primitive.shadow_thickness));
                               
   switch (lp -> alignment)
   {
    case XmALIGNMENT_BEGINNING:
      lp->TextRect.x = (short) lp->margin_width +
	  lp->margin_left +
	      newlw->primitive.highlight_thickness +
		  newlw->primitive.shadow_thickness;

      break;

    case XmALIGNMENT_END:
      lp->TextRect.x = (short) newlw->core.width - 
	  (newlw->primitive.highlight_thickness +
	   newlw->primitive.shadow_thickness +	
	   lp->margin_width + lp->margin_right +
	   lp->TextRect.width);
      break;

    default:
      lp->TextRect.x =  (short) (newlw->primitive.highlight_thickness
          + newlw->primitive.shadow_thickness
              + lp->margin_width + lp->margin_left +
                  ((newlw->core.width - lp->margin_left
                    - lp->margin_right
                    - (2 * (lp->margin_width
                            + newlw->primitive.highlight_thickness
                            + newlw->primitive.shadow_thickness))
                    - lp->TextRect.width) / 2));
      break;
   }

   /* Has a height been specified? */
   
   if (newlw->core.height == 0)
       newlw->core.height = (Dimension)
	   Max(lp->TextRect.height, lp->acc_TextRect.height) +
	       lp->margin_top +
		   lp->margin_bottom
		       + (2 * (lp->margin_height
			       + newlw->primitive.highlight_thickness
			       + newlw->primitive.shadow_thickness));

   lp->TextRect.y =  (short) (newlw->primitive.highlight_thickness
       + newlw->primitive.shadow_thickness
           + lp->margin_height + lp->margin_top +
               ((newlw->core.height - lp->margin_top
                 - lp->margin_bottom
                 - (2 * (lp->margin_height
                         + newlw->primitive.highlight_thickness
                         + newlw->primitive.shadow_thickness))
                 - lp->TextRect.height) / 2));

   if (lp->_acc_text != NULL)
   {
      Dimension  base_label, base_accText, diff;

      lp->acc_TextRect.x = (short) newlw->core.width - 
	  newlw->primitive.highlight_thickness -
	      newlw->primitive.shadow_thickness -
		  newlw->label.margin_width -
		      newlw->label.margin_right +
			  LABEL_ACC_PAD;

      lp->acc_TextRect.y =  (short) (newlw->primitive.highlight_thickness
	  + newlw->primitive.shadow_thickness
	      + lp->margin_height + lp->margin_top +
		  ((newlw->core.height - lp->margin_top
		    - lp->margin_bottom
		    - (2 * (lp->margin_height
			    + newlw->primitive.highlight_thickness
			    + newlw->primitive.shadow_thickness))
		    - lp->acc_TextRect.height) / 2));

      /* make sure the label and accelerator text line up*/
      /* when the fonts are different */

      if (Lab_IsText(newlw))
      {
	 base_label = _XmStringBaseline (lp->font, lp->_label);
	 base_accText = _XmStringBaseline (lp->font, lp->_acc_text);
	 
	 if (base_label > base_accText)
	 {
	    diff = base_label - base_accText;
	    lp->acc_TextRect.y = (short) lp->TextRect.y + diff - 1;
	 }
	 else if (base_label < base_accText)
	 {
	    diff = base_accText - base_label;
	    lp->TextRect.y = (short) lp->acc_TextRect.y + diff - 1;
	 }
      }
   }

   if (newlw->core.width == 0)    /* set core width and height to a */
       newlw->core.width = 1;       /* default value so that it doesn't */
   if (newlw->core.height == 0)   /* generate a Toolkit Error */
       newlw->core.height = 1;
}

/************************************************************
 *
 * Initialize
 *    This is the widget's instance initialize routine.  It is 
 *    called once for each widget                              
 *
 ************************************************************/
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
    XmLabelWidget lw = (XmLabelWidget) new_w;
    unsigned char  stringDirection;
    Arg           l_args[1];
    int	n;


    /* if menuProcs is not set up yet, try again */
    if (xmLabelClassRec.label_class.menuProcs == NULL)
	xmLabelClassRec.label_class.menuProcs =
	    (XmMenuProc) _XmGetMenuProcContext();

    /* Check for Invalid enumerated types */

    if(    !XmRepTypeValidValue( XmRID_LABEL_TYPE, lw->label.label_type,
                                                              (Widget) lw)    )
    {
      lw->label.label_type = XmSTRING;
    }

    if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, lw->label.alignment,
                                                              (Widget) lw)    )
    {
      lw->label.alignment = XmALIGNMENT_CENTER;
    }
    
/*
 * Default string behavior : is same as that of parent.
 *  If string direction is not set then borrow it from the parent.
 *  Should we still check for it.
 */

   if (lw->label.string_direction == XmSTRING_DIRECTION_DEFAULT)
    { if ( XmIsManager ( XtParent(lw))) 
	  { n = 0;
        XtSetArg( l_args[n], XmNstringDirection, &stringDirection); n++;
        XtGetValues ( XtParent(lw), l_args, n);
        lw->label.string_direction = stringDirection;
	  }
      else
	   lw->label.string_direction = XmSTRING_DIRECTION_L_TO_R;
    }

    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
                                  lw->label.string_direction, (Widget) lw)    )
    {
      lw->label.string_direction = XmSTRING_DIRECTION_L_TO_R;
    }

    if (lw->label.font == NULL)
    {
      XmFontList defaultFont;

      if (XtClass(lw) == xmLabelWidgetClass)
        defaultFont = _XmGetDefaultFontList( (Widget) lw, XmLABEL_FONTLIST);
      else
        defaultFont = _XmGetDefaultFontList( (Widget) lw, XmBUTTON_FONTLIST);

      lw->label.font = XmFontListCopy (defaultFont);

    }
    /* Make a local copy of the font list */
    else
    {
      lw->label.font = XmFontListCopy( lw->label.font);
    }

    /* get menu type and which button */
    if (XmIsRowColumn(XtParent(new_w)))
    {
       Arg arg[1];
       XtSetArg (arg[0], XmNrowColumnType, &lw->label.menu_type);
       XtGetValues (XtParent(new_w), arg, 1);
    }
    else
	lw->label.menu_type = XmWORK_AREA;

    /*  Handle the label string :
     *   If label is the constant XmUNSPECIFIED, creates a empty XmString.
     *    (this is used by DrawnB instead of a default conversion that leads
     *     to a leak when someone provides a real label)
     *   If no label string is given accept widget's name as default.
     *     convert the widgets name to an XmString before storing;
     *   else
     *     save a copy of the given string.
     *     If the given string is not an XmString issue an warning.
     */
    if ( lw->label._label == (_XmString) XmUNSPECIFIED)
        {
          XmString string;

	  string = _XmOSGetLocalizedString ((char *) NULL,  /* reserved */
					    (Widget) lw,
					    XmNlabelString,
					    "\0");
	  
          lw->label._label =  _XmStringCreate(string);
          XmStringFree (string);
        }
    else 
    if (lw->label._label == NULL)   
        {
          XmString string;

	  string = _XmOSGetLocalizedString ((char *) NULL,  /* reserved */
					    (Widget) lw,
					    XmNlabelString,
					    lw->core.name);
	  
          lw->label._label =  _XmStringCreate(string);
          XmStringFree (string);
        }
    else
 
      {  if (_XmStringIsXmString( (XmString) lw->label._label))
           lw->label._label= _XmStringCreate( (XmString) lw->label._label);
         else
         {
           XmString string;
           XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;

            _XmWarning( (Widget) lw, CS_STRING_MESSAGE);

           string   =  XmStringLtoRCreate(lw->core.name, cset);
           lw->label._label =  _XmStringCreate(string);
           XmStringFree (string);

         }
    }

     /*
      * Convert the given mnemonicCharset to the internal Xm-form.
      */
    if  (lw->label.mnemonicCharset != NULL )
             lw->label.mnemonicCharset =
		_XmStringCharSetCreate (lw->label.mnemonicCharset );
    else
             lw->label.mnemonicCharset =
		_XmStringCharSetCreate (XmFONTLIST_DEFAULT_TAG );

    /* Accelerators are currently only supported in menus */

    if ((lw->label._acc_text != NULL) &&
	((lw->label.menu_type == XmMENU_POPUP) ||
	 (lw->label.menu_type == XmMENU_PULLDOWN)))
     {
        if (_XmStringIsXmString( (XmString) lw->label._acc_text))
        {
                /*
                 * Copy the input string into local space, if
                 * not a Cascade Button
                 */
           if ( XmIsCascadeButton(lw))
              lw->label._acc_text = NULL;
           else
              lw->label._acc_text= _XmStringCreate( (XmString) lw->label._acc_text);
        }
        else
        {
           _XmWarning( (Widget) lw, ACC_MESSAGE);
           lw->label._acc_text = NULL;
        }
    }
    else
        lw->label._acc_text = NULL;


    if ((lw->label.accelerator != NULL) &&
        ((lw->label.menu_type == XmMENU_POPUP) ||
	 (lw->label.menu_type == XmMENU_PULLDOWN)))
    {
      char *s;

      /* Copy the accelerator into local space */

      s = XtMalloc (XmStrlen (lw->label.accelerator) + 1);
      strcpy (s, lw->label.accelerator);
      lw->label.accelerator = s;
    }
    else
         lw->label.accelerator = NULL;

    lw->label.skipCallback = FALSE;

   /*  If zero width and height was requested by the application,  */
   /*  reset new_w's width and height to zero to allow SetSize()     */
   /*  to operate properly.                                        */

   if (req->core.width == 0)
      lw->core.width = 0;  

   if (req->core.height == 0)
      lw->core.height = 0;

   _XmCalcLabelDimensions(new_w);
   (* (lw->core.widget_class->core_class.resize)) ((Widget) lw); 
   SetNormalGC(lw);

   /*  Force the label traversal flag when in a menu  */

   if ((XtClass(lw) == xmLabelWidgetClass) &&
       ((lw->label.menu_type == XmMENU_POPUP) ||
        (lw->label.menu_type == XmMENU_PULLDOWN) ||
        (lw->label.menu_type == XmMENU_OPTION)))
   {
      lw->primitive.traversal_on = FALSE;
      lw->primitive.highlight_on_enter = FALSE;
   }

   /* if in menu, override with menu traversal translations */
   if ((lw->label.menu_type == XmMENU_POPUP) ||
       (lw->label.menu_type == XmMENU_PULLDOWN) ||
       (lw->label.menu_type == XmMENU_BAR) ||
       (lw->label.menu_type == XmMENU_OPTION))
   {
      XtOverrideTranslations( (Widget) lw, (XtTranslations)
        ((XmLabelClassRec *)XtClass(lw))->label_class.translations);
   }

   else
   /* otherwise override with primitive traversal translations */
     XtOverrideTranslations( (Widget) lw, (XtTranslations)
                            ((XmPrimitiveClassRec *) XtClass(lw))
                            ->primitive_class.translations);

   if ((lw->label.menu_type == XmMENU_POPUP) ||
       (lw->label.menu_type == XmMENU_PULLDOWN) ||
       (lw->label.menu_type == XmMENU_BAR))
       lw->primitive.highlight_thickness = 0;

}

/************************************************************************
 *
 *  QueryGeometry
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryGeometry( widget, intended, desired )
        Widget widget ;
        XtWidgetGeometry *intended ;
        XtWidgetGeometry *desired ;
#else
QueryGeometry(
        Widget widget,
        XtWidgetGeometry *intended,
        XtWidgetGeometry *desired )
#endif /* _NO_PROTO */
{
    XmLabelWidget lw = (XmLabelWidget) widget ;

    if (lw->label.recompute_size == FALSE) {
        desired->width = XtWidth(widget) ;
	desired->height = XtHeight(widget) ;
    } else {
	desired->width = (Dimension) lw->label.TextRect.width +
	    (2 * (lw->label.margin_width +
		  lw->primitive.highlight_thickness +
		  lw->primitive.shadow_thickness)) +
		      lw->label.margin_left +
			  lw->label.margin_right;
	if (desired->width == 0) desired->width = 1;

	desired->height = (Dimension) Max(lw->label.TextRect.height,
					  lw->label.acc_TextRect.height)
	    + (2 * (lw->label.margin_height +
		    lw->primitive.highlight_thickness +
		    lw->primitive.shadow_thickness)) +
			lw->label.margin_top +
                      lw->label.margin_bottom;
	if (desired->height == 0) desired->height = 1;
    }

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}

/************************************************************************
 *
 *  Destroy
 *      Free up the label gadget allocated space.  This includes
 *      the label, and GC's.
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
    XmLabelWidget lw = (XmLabelWidget) w;

    if (lw->label._label != NULL) _XmStringFree (lw->label._label);
    if (lw->label._acc_text != NULL) _XmStringFree (lw->label._acc_text);
    if (lw->label.accelerator != NULL) XtFree (lw->label.accelerator);
    if (lw->label.font  != NULL) XmFontListFree (lw->label.font);
    if (lw->label.mnemonicCharset !=  NULL )
	XtFree (lw->label.mnemonicCharset);
    XtReleaseGC ((Widget) lw, lw->label.normal_GC);
    XtReleaseGC ((Widget) lw, lw->label.insensitive_GC);

}




/************************************************************************
 *
 *  Redisplay
 *

 ***********************************************************************/
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
   XmLabelWidget lw = (XmLabelWidget) wid;
   GC		    gc;
   GC clipgc = NULL;
   XRectangle clip_rect;
   XmLabelPart        *lp;
   Dimension availW, availH, marginal_width, marginal_height, max_text_height;
   Boolean clip_set = False;

   lp = &(lw->label);

   /*
    * Set a clipping area if the label will overwrite critical margins.
    * A critical margin is defined to be "margin_{left,right,
    * top,bottom} margin" because it used for other things
    * such as default_button_shadows and accelerators.   Note that
    * overwriting the "margin_{width,height} margins is allowed.
    */
   availH = lw->core.height;
   availW = lw->core.width;
   
   /* Adjust definitions of temporary variables */
   marginal_width = lp->margin_left + lp->margin_right +
     (2 * (lw->primitive.highlight_thickness +
	   lw->primitive.shadow_thickness));
   
   marginal_height = lp->margin_top + lp->margin_bottom +
     (2 * (lw->primitive.highlight_thickness +
	   lw->primitive.shadow_thickness));
   
   max_text_height = Max(lp->TextRect.height, lp->acc_TextRect.height);

   if (availH < (marginal_height + max_text_height) ||
       availW < (marginal_width + lp->TextRect.width))
     {
       clip_rect.x = lw->primitive.highlight_thickness +
	 lw->primitive.shadow_thickness + lp->margin_left;
       clip_rect.y = lw->primitive.highlight_thickness +
	 lw->primitive.shadow_thickness + lp->margin_top;
       
       /* Don't allow negative dimensions */
       if (availW > marginal_width)
	 clip_rect.width = availW - marginal_width;
       else
	 clip_rect.width = 0;
       
       if (availH > marginal_height)
	 clip_rect.height = availH - marginal_height;
       else
	 clip_rect.height = 0;
       
       if ((lw->core.sensitive) && (lw->core.ancestor_sensitive))
	 clipgc = lp->normal_GC;
       else
	 clipgc = lp->insensitive_GC;

       XSetClipRectangles(XtDisplay(lw), clipgc, 0,0, &clip_rect, 1, Unsorted);
       clip_set = True;
     }

   if (Lab_IsPixmap(lw))
   {
      if ((lw->core.sensitive) && (lw->core.ancestor_sensitive))
      {
	 if (Pix (lw) != XmUNSPECIFIED_PIXMAP)
	 {
	    gc = lp->normal_GC;

	    XCopyArea (XtDisplay(lw), Pix(lw), XtWindow(lw),gc, 0, 0, 
                       lp->TextRect.width, lp->TextRect.height,
                       lp->TextRect.x, lp->TextRect.y); 
	 }
      }
      else
      {
	 if (Pix_insen (lw) != XmUNSPECIFIED_PIXMAP)
	 {
        gc = lp->insensitive_GC;

	    XCopyArea (XtDisplay(lw), Pix_insen(lw), XtWindow(lw),gc, 0, 0, 
                       lp->TextRect.width, lp->TextRect.height,
                       lp->TextRect.x, lp->TextRect.y); 

	 }
      }
   }

   else if ( (Lab_IsText (lw)) && (lp->_label != NULL))
   {
      if (lp->mnemonic != NULL)
/*
 * When the DEC  routine comes call  the following routine.
 * - Call it now
 */
      { /*
         * A hack to use keysym as the mnemonic.
	 */
	char tmp[2];
	tmp[0] = ((lp->mnemonic) & ( (long) (0xFF)));
	tmp[1] = '\0';

        _XmStringDrawMnemonic(XtDisplay(lw), XtWindow(lw),
                                lp->font, lp->_label,
                                (((lw->core.sensitive) &&
                                  (lw->core.ancestor_sensitive)) ?
                                lp->normal_GC :
                                lp->insensitive_GC),
                                lp->TextRect.x, lp->TextRect.y,
                                lp->TextRect.width, lp->alignment,
                                lp->string_direction, NULL,
                                tmp, /* lp->mnemonic, */ lp->mnemonicCharset);
	}
/*
 * but for now keep the original code.
 *
 *
 *        _XmStringDrawUnderline(XtDisplay(lw), XtWindow(lw),
 *                               lp->font, lp->_label,
 *                               (((lw->core.sensitive) && 
 *                                 (lw->core.ancestor_sensitive))
 *                               lp->normal_GC :
 *                               lp->insensitive_GC),
 *                               lp->TextRect.x, lp->TextRect.y,
 *                               lp->TextRect.width, lp->alignment,
 *                               lp->string_direction, NULL,
 *                               lp->_mnemonic);
 *
 */

      else
          _XmStringDraw (XtDisplay(lw), XtWindow(lw),
                        lp->font, lp->_label,
                        (((lw->core.sensitive) &&
                          (lw->core.ancestor_sensitive)) ?
                        lp->normal_GC :
                        lp->insensitive_GC),
                        lp->TextRect.x, lp->TextRect.y,
                        lp->TextRect.width, lp->alignment,
                        lp->string_direction, NULL);

   }

   if (lp->_acc_text != NULL)
   {
      /* since accelerator text  is drawn by moving in from the right,
	 it is possible to overwrite label text when there is clipping,
	 Therefore draw accelerator text only if there is enough
	 room for everything */

      if ((lw->core.width) >= (2 * (lw->primitive.highlight_thickness +
                                       lw->primitive.shadow_thickness +
                                       lp->margin_width) +
                                       lp->margin_left + lp->TextRect.width +
                                       lp->margin_right))
	  _XmStringDraw (XtDisplay(lw), XtWindow(lw),
                              lp->font, lp->_acc_text,
                              (((lw->core.sensitive) &&
                              (lw->core.ancestor_sensitive)) ?
                              lp->normal_GC :
                              lp->insensitive_GC),
                              lp->acc_TextRect.x, lp->acc_TextRect.y,
                              lp->acc_TextRect.width, XmALIGNMENT_END,
                              lp->string_direction, NULL);
   }

    /*  If set, reset the clipping rectangle to none  */
    if (clip_set)
      XSetClipMask (XtDisplay (lw), clipgc, None);

    /* Redraw the proper highlight  */

    if ((lw->label.menu_type != XmMENU_POPUP) &&
	(lw->label.menu_type != XmMENU_PULLDOWN))
    {
       if (lw->primitive.highlighted)
       {   
	   (*((XmPrimitiveWidgetClass) XtClass( lw))
	      ->primitive_class.border_highlight)( (Widget) lw) ;
       } 
       else
       {   
	  if (_XmDifferentBackground ((Widget) lw, XtParent (lw)))
	  {   
	     (*((XmPrimitiveWidgetClass) XtClass( lw))
		->primitive_class.border_unhighlight)((Widget) lw) ;
	  } 
       } 
    }
}


/**********************************************************************
 *
 * Enter
 *
 *********************************************************************/
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
            XmLabelWidget w = (XmLabelWidget) wid ;
  if (w->primitive.highlight_on_enter)
   _XmPrimitiveEnter (wid, event, params, num_params);
}


/**********************************************************************
 *
 * Leave
 *
 *********************************************************************/
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
        XmLabelWidget w = (XmLabelWidget) wid ;
  if (w->primitive.highlight_on_enter)
   _XmPrimitiveLeave ((Widget) w, event, params, num_params);
}


/************************************************************************
 *
 *  SetValues
 *      This routine will take care of any changes that have been made
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
    XmLabelWidget current = (XmLabelWidget) cw ;
    XmLabelWidget req = (XmLabelWidget) rw ;
    XmLabelWidget new_w = (XmLabelWidget) nw ;
    Boolean flag = FALSE;
    Boolean newstring = FALSE;
    XmLabelPart        *newlp, *curlp, *reqlp;
    Boolean ProcessFlag = FALSE;
    Boolean CleanupFontFlag = FALSE;
    Boolean Call_SetSize = False;

    /* Get pointers to the label parts  */

    newlp = &(new_w->label);
    curlp = &(current->label);
    reqlp = &(req->label);


    /*  If the label has changed, make a copy of the new label,  */
    /*  and free the old label.                                  */

    if (newlp->_label!= curlp->_label)
    {   
       newstring = TRUE;
       if (newlp->_label == NULL)
       { 
	  XmString string;
          XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;
   
          string   =  XmStringLtoRCreate(new_w->core.name, cset);
          newlp->_label =  _XmStringCreate(string);
          XtFree((char *) string);
       }
       else
       { 
	  if (_XmStringIsXmString( (XmString) newlp->_label))
              newlp->_label = _XmStringCreate( (XmString) newlp->_label);     
	  else
          {
             XmString string;
	     XmStringCharSet cset = (XmStringCharSet) XmFONTLIST_DEFAULT_TAG;

	     _XmWarning( (Widget) new_w, CS_STRING_MESSAGE);

	     string   =  XmStringLtoRCreate(new_w->core.name, cset);
	     newlp->_label =  _XmStringCreate(string);
	     XtFree((char *) string);

          }
       }

       _XmStringFree(curlp->_label);
       curlp->_label= NULL;
       reqlp->_label= NULL;
    }


    if ((newlp->_acc_text!= curlp->_acc_text) &&
	((newlp->menu_type == XmMENU_POPUP) ||
	 (newlp->menu_type == XmMENU_PULLDOWN)))
    {
      /* BEGIN OSF Fix pir 1098 */
      newstring = TRUE;
      /* END OSF Fix pir 1098 */
      if (newlp->_acc_text != NULL)
      {
        if (_XmStringIsXmString( (XmString) newlp->_acc_text))
        {
	  if ((XmIsCascadeButton (new_w)) &&
	      (newlp->_acc_text != NULL))
	      newlp->_acc_text = NULL;

	  else
            newlp->_acc_text = _XmStringCreate( (XmString) newlp->_acc_text);
          _XmStringFree(curlp->_acc_text);
          curlp->_acc_text= NULL;
          reqlp->_acc_text= NULL;
        }
        else
        {
           _XmWarning( (Widget) new_w, ACC_MESSAGE);
            newlp->_acc_text = NULL;
        }
      }
      /* BEGIN OSF Fix pir 1098 */
      else
	newlp->margin_right = 0;
      /* END OSF Fix pir 1098 */
    }
    else
        newlp->_acc_text = curlp->_acc_text;


    if (newlp->font != curlp->font)
    {
      CleanupFontFlag = True;
      if (newlp->font == NULL)
      {
     if (XtClass(new_w) == xmLabelWidgetClass)
       newlp->font = _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST);
     else
       newlp->font = _XmGetDefaultFontList( (Widget) new_w, XmBUTTON_FONTLIST);
      }
      newlp->font = XmFontListCopy (newlp->font);

     if (Lab_IsText(new_w))
     {
       _XmStringUpdate (newlp->font, newlp->_label);
       if (newlp->_acc_text != NULL)
         _XmStringUpdate (newlp->font, newlp->_acc_text);
     }
    }

   if ((new_w->label.menu_type == XmMENU_POPUP) ||
       (new_w->label.menu_type == XmMENU_PULLDOWN) ||
       (new_w->label.menu_type == XmMENU_BAR))
       new_w->primitive.highlight_thickness = 0;

    if(    !XmRepTypeValidValue( XmRID_LABEL_TYPE, new_w->label.label_type,
                                                             (Widget) new_w)    )
    {
      new_w->label.label_type = current->label.label_type;
    }

    /* ValidateInputs(new_w); */

    if ((Lab_IsText(new_w) && 
	  ((newstring) ||
          (newlp->font != curlp->font))) ||
	(Lab_IsPixmap(new_w) &&
          ((newlp->pixmap != curlp->pixmap) ||
           (newlp->pixmap_insen  != curlp->pixmap_insen) ||
           /* When you have different sized pixmaps for sensitive and */
           /* insensitive states and sensitivity changes, */
           /* the right size is chosen. (osfP2560) */
           (new_w->core.sensitive != current->core.sensitive) ||
           (new_w->core.ancestor_sensitive != current->core.ancestor_sensitive))) ||
        (newlp->label_type != curlp->label_type))
    {
      /*
       * Fix for CR 5419 - Only set Call_SetSize if the true sizes change.
       */
      _XmCalcLabelDimensions((Widget) new_w);

      if ((newlp->acc_TextRect.width != curlp->acc_TextRect.width) ||
	  (newlp->acc_TextRect.height != curlp->acc_TextRect.height) ||
	  (newlp->TextRect.width != curlp->TextRect.width) ||
	  (newlp->TextRect.height != curlp->TextRect.height))
        {
	  if (newlp->recompute_size)
	    {
	      if (req->core.width == current->core.width)
		new_w->core.width = 0;
	      if (req->core.height == current->core.height)
		new_w->core.height = 0;
	    }
	  
	  Call_SetSize = True;
        }
      /* End fix for CR 5419 */

      flag = True;
    }

    if ((newlp->alignment != curlp->alignment) ||
        (newlp->string_direction != curlp->string_direction)) {

	if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, new_w->label.alignment,
				    (Widget) new_w)    )
	    {
		new_w->label.alignment = current->label.alignment;
	    }

        if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
				    new_w->label.string_direction, 
				    (Widget) new_w)    )
	    {
		new_w->label.string_direction = current->label.string_direction;
	    }

        Call_SetSize = True;
	
	flag = True;
    }

    if ((newlp->margin_height != curlp->margin_height) ||
        (newlp->margin_width != curlp->margin_width) ||
        (newlp->margin_left != curlp->margin_left) ||
        (newlp->margin_right != curlp->margin_right) ||
        (newlp->margin_top != curlp->margin_top) ||
        (newlp->margin_bottom != curlp->margin_bottom) ||
        (new_w->primitive.shadow_thickness !=
         current->primitive.shadow_thickness) ||
        (new_w->primitive.highlight_thickness !=
         current->primitive.highlight_thickness) ||
        ((new_w->core.width <= 0) || (new_w->core.height <= 0)))
    {
        if(    !XmRepTypeValidValue( XmRID_ALIGNMENT, new_w->label.alignment,
                                                             (Widget) new_w)    )
        {
              new_w->label.alignment = current->label.alignment;
        }

        if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION,
                                new_w->label.string_direction, (Widget) new_w)    )
        {
             new_w->label.string_direction = current->label.string_direction;
        }

        if (newlp->recompute_size)
        {
          if (req->core.width == current->core.width)
            new_w->core.width = 0;
          if (req->core.height == current->core.height)
            new_w->core.height = 0;
        }
        
        Call_SetSize = True;

        flag = True;
    }


  /* SetSize is called only if we need to calculate the dimensions or */
  /* coordinates  for the string.				      */

   if (Call_SetSize)
	   (* (new_w->core.widget_class->core_class.resize)) ((Widget) new_w); 



/*
 * If sensitivity of the label has changed then we must redisplay
 *  the label.
 */
	if ((new_w->core.sensitive != current->core.sensitive) ||
        (new_w->core.ancestor_sensitive != current->core.ancestor_sensitive)) 
	  {
	    flag = TRUE;
	  }

    if ((new_w->primitive.foreground !=
        current->primitive.foreground) ||
        (new_w->core.background_pixel !=
        current->core.background_pixel) ||
        (newlp->font != curlp->font))
    {
        XtReleaseGC((Widget) current, current->label.normal_GC);
        XtReleaseGC((Widget) current, current->label.insensitive_GC);
        SetNormalGC(new_w);
        flag = TRUE;
    }

    /*  Force the traversal flag when in a menu.  */

    if ((XtClass(new_w) == xmLabelWidgetClass) &&
	((new_w->label.menu_type == XmMENU_POPUP) ||
	 (new_w->label.menu_type == XmMENU_PULLDOWN) ||
	 (new_w->label.menu_type == XmMENU_OPTION)))
    {
       new_w->primitive.traversal_on = FALSE;
       new_w->primitive.highlight_on_enter = FALSE;
    }

    if (new_w->primitive.traversal_on &&
       (new_w->primitive.traversal_on != current->primitive.traversal_on) &&
       new_w->core.tm.translations)
    {
	if ((new_w->label.menu_type == XmMENU_POPUP) ||
	    (new_w->label.menu_type == XmMENU_PULLDOWN) ||
	    (new_w->label.menu_type == XmMENU_BAR) ||
	    (new_w->label.menu_type == XmMENU_OPTION))
	{
	    if (((XmLabelClassRec *)XtClass(new_w))->label_class.translations)
		XtOverrideTranslations ((Widget)new_w, (XtTranslations)
		((XmLabelClassRec *)XtClass(new_w))->label_class.translations);
	}
	else
	{
	    if (((XmLabelClassRec *) XtClass(new_w))
			->primitive_class.translations)
		XtOverrideTranslations ((Widget)new_w, (XtTranslations)
		((XmLabelClassRec *) XtClass(new_w))
			->primitive_class.translations);
	}
    }

    if ((new_w->label.menu_type != XmWORK_AREA) &&
        (new_w->label.mnemonic != current->label.mnemonic))
    {
       /* New grabs only required if mnemonic changes */
       ProcessFlag = TRUE;
       if (new_w->label.label_type == XmSTRING)
	   flag = TRUE;
    }

    if (new_w->label.mnemonicCharset != current->label.mnemonicCharset)
    {
       if (new_w->label.mnemonicCharset)
           new_w->label.mnemonicCharset =
    	      _XmStringCharSetCreate(new_w->label.mnemonicCharset);
       else
           new_w->label.mnemonicCharset =
    	      _XmStringCharSetCreate(XmFONTLIST_DEFAULT_TAG);

       if (current->label.mnemonicCharset != NULL)
	  XtFree (current->label.mnemonicCharset);

       if (new_w->label.label_type == XmSTRING)
	  flag = TRUE;
    }

    if (((new_w->label.menu_type == XmMENU_POPUP) ||
	 (new_w->label.menu_type == XmMENU_PULLDOWN)) &&
        (new_w->label.accelerator != current->label.accelerator))
    {
      if (newlp->accelerator != NULL)
      {
         char *s;

         /* Copy the accelerator into local space */

         s = XtMalloc (XmStrlen (newlp->accelerator) + 1);
         strcpy (s, newlp->accelerator);
         newlp->accelerator = s;
      }
      if (curlp->accelerator != NULL)
        XtFree(curlp->accelerator);
      curlp->accelerator = NULL;
      reqlp->accelerator = NULL;
      ProcessFlag = TRUE;
    }
    else
      newlp->accelerator = curlp->accelerator;
  
    if (ProcessFlag)
	(* xmLabelClassRec.label_class.menuProcs) (XmMENU_PROCESS_TREE,
           (Widget) new_w, NULL, NULL, NULL);

    if (flag && (new_w->label.menu_type == XmMENU_PULLDOWN))
       (* xmLabelClassRec.label_class.menuProcs) (XmMENU_MEMWIDGET_UPDATE,
          (Widget) new_w, NULL, NULL, NULL);

    if (CleanupFontFlag)
      if (curlp->font) XmFontListFree(curlp->font); 

    return(flag);
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
static void 
#ifdef _NO_PROTO
SetOverrideCallback( wid )
        Widget wid ;
#else
SetOverrideCallback(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmLabelWidget w = (XmLabelWidget) wid ;
   w->label.skipCallback = True;
}


/************************************************************************
 *
 *  Help
 *      This routine is called if the user made a help selection
 *      on the widget.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Help( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Help(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmLabelWidget lw = (XmLabelWidget) w;
    Widget parent = XtParent(lw);

    if (lw->label.menu_type == XmMENU_POPUP ||
	lw->label.menu_type == XmMENU_PULLDOWN)
	(* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_POPDOWN, parent, NULL, event, NULL);

	_XmPrimitiveHelp( (Widget) w, event, params, num_params);
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
GetLabelString( wid, resource, value )
        Widget wid ;
        XrmQuark resource ;
        XtArgVal *value ;
#else
GetLabelString(
        Widget wid,
        XrmQuark resource,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelWidget lw = (XmLabelWidget) wid ;
  XmString string;
 
  string = _XmStringCreateExternal (lw->label.font, lw->label._label);

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
GetAccelerator( wid, resource, value )
        Widget wid ;
        XrmQuark resource ;
        XtArgVal *value ;
#else
GetAccelerator(
        Widget wid,
        XrmQuark resource,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelWidget lw = (XmLabelWidget) wid ;
  String string;

  string = XtNewString( Lab_Accelerator(lw));

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
GetAcceleratorText( wid, resource, value )
        Widget wid ;
        XrmQuark resource ;
        XtArgVal *value ;
#else
GetAcceleratorText(
        Widget wid,
        XrmQuark resource,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelWidget lw = (XmLabelWidget) wid ;
  XmString string;

  string = _XmStringCreateExternal (lw->label.font, lw->label._acc_text);

  *value = (XtArgVal) string;

}
/************************************************************************
 *
 *  XmCreateLabelWidget
 *      Externally accessable function for creating a label gadget.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateLabel( parent, name, arglist, argCount )
        Widget parent ;
        char *name ;
        Arg *arglist ;
        Cardinal argCount ;
#else
XmCreateLabel(
        Widget parent,
        char *name,
        Arg *arglist,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
    return (XtCreateWidget(name,xmLabelWidgetClass,parent,arglist,argCount));
}

static XmStringCharSet 
#ifdef _NO_PROTO
_XmStringCharSetCreate( stringcharset )
        XmStringCharSet stringcharset ;
#else
_XmStringCharSetCreate(
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
 *  GetMnemonicCharSet
 *     This is a get values hook function that returns the external
 *     form of the mnemonicCharSet from the internal form.
 *  : Returns a string containg the mnemonicCharSet.
 *    Caller must free the string .
 ***********************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetMnemonicCharSet( wid, resource, value )
        Widget wid ;
        XrmQuark resource ;
        XtArgVal *value ;
#else
GetMnemonicCharSet(
        Widget wid,
        XrmQuark resource,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmLabelWidget lw = (XmLabelWidget) wid ;
  char *cset;
  int   size;

  cset = NULL;
  if (lw->label.mnemonicCharset)
    { size = strlen (lw->label.mnemonicCharset);
      if (size > 0)
     cset = (char *) (_XmStringCharSetCreate(lw->label.mnemonicCharset));
    }

  *value = (XtArgVal) cset;

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
        XmLabelWidget new_w = (XmLabelWidget) nw ;

	   (* (new_w->core.widget_class->core_class.resize)) ((Widget) new_w); 
		*request = *reply;
}

/************************************************************************
 *
 * XmLabelGetDisplayRect
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
XmLabelGetDisplayRect(w, displayrect)
        Widget w;
        XRectangle *displayrect;
#else
XmLabelGetDisplayRect(
        Widget w,
        XRectangle *displayrect)
#endif /* _NO_PROTO */
{
        XmLabelWidget wid = (XmLabelWidget) w ;
  (*displayrect).x = wid->label.TextRect.x;
  (*displayrect).y = wid->label.TextRect.y;
  (*displayrect).width = wid->label.TextRect.width;
  (*displayrect).height = wid->label.TextRect.height;

  return (TRUE);
}

/************************************************************************
 *
 * XmLabelGetBaselines
 *
 * A Class function which when called returns True, if the widget has
 * a baseline and also determines the number of pixels from the y
 * origin to the first line of text and assigns it to the variable
 * being passed in.
 *
 ************************************************************************/

static Boolean
#ifdef _NO_PROTO
XmLabelGetBaselines(wid, baselines, line_count)
        Widget wid;
        Dimension **baselines;
        int *line_count;
#else
XmLabelGetBaselines(
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
  XmLabelWidget lw = (XmLabelWidget)wid;

  if (Lab_IsPixmap(wid)) 
  {
    return (False);
  }
  else
  {
    index = 0;
    FontList = lw->label.font;
    string = _XmStringCreateExternal(lw->label.font, lw->label._label);

    if (!XmStringInitContext (&context, string))
      return (False);

    *line_count = XmStringLineCount(string);

    base_array = (Dimension *)XtMalloc((sizeof(Dimension) * (*line_count)));

    Offset = ((XmLabelWidget) wid)->label.TextRect.y;
    
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


static Widget
#ifdef _NO_PROTO
GetPixmapDragIcon( w )
        Widget w ;
#else
GetPixmapDragIcon(
        Widget w )
#endif /* _NO_PROTO */
{
    XmLabelWidget lw = (XmLabelWidget) w;
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
    XtSetArg(args[n], XmNforeground, lw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNbackground, lw->primitive.foreground);  n++;
    drag_icon = XtCreateWidget("drag_icon", xmDragIconObjectClass,
			       screen_object, args, n);
    return drag_icon ;
}


/* ARGSUSED */
static void
#ifdef _NO_PROTO
ProcessDrag( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmLabelWidget lw = (XmLabelWidget) w;
    Atom targets[3];
    Cardinal num_targets = 0;
    Widget drag_icon;
    Arg args[10];
    int n;

   /* add targets that you believe you can convert to */
    if (Lab_IsPixmap(lw))
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
    XtSetArg(args[n], XmNcursorBackground, lw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNcursorForeground, lw->primitive.foreground);  n++;

    /* if it's a labelPixmap, only specify the pixmap icon */
    if (Lab_IsPixmap(lw) && (Pix(lw) != XmUNSPECIFIED_PIXMAP)) {
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
    XmLabelWidget lw;
    Display *display = XtDisplay(w);
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    Atom TIMESTAMP = XmInternAtom(XtDisplay(w), "TIMESTAMP", False);
    Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
    Atom MOTIF_DROP = XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False);
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
       lw = (XmLabelWidget) c_ptr;
    } else
       return False;

    if (lw == NULL) return False;

    if (*target == TARGETS) {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));

      *value = (XtPointer) targs;
      *targs++ = TARGETS; target_count++;
      *targs++ = TIMESTAMP; target_count++;
      if (Lab_IsPixmap(lw)) 
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
      from_val.addr = (char *)_XmStringCreateExternal (lw->label.font,
						       lw->label._label);
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
      *background = lw->core.background_pixel;
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
      *foreground = lw->primitive.foreground;
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
      *colormap = lw->core.colormap;
    /* value, type, length, and format must be set */
      *value = (XtPointer) colormap;
      *type = XA_COLORMAP;
      *length = sizeof(Pixel);
      *format = 32;
      return True;
    }

    return False;
}


