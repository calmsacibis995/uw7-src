#pragma ident	"@(#)m1.2libs:Xm/Scale.c	1.8"
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
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <stdio.h>
#include <limits.h>
#ifdef __cplusplus
extern "C" { /* some 'locale.h' do not have prototypes (sun) */
#endif
#include <X11/Xlocale.h>
#ifdef __cplusplus
} /* Close scope of 'extern "C"' declaration */
#endif /* __cplusplus */
#include "XmI.h"
#include <Xm/ScaleP.h>
#include <Xm/ScrollBarP.h>
#include <Xm/LabelG.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/DragC.h>
#include <Xm/DragIconP.h>
#include <Xm/AtomMgr.h>
#include "GMUtilsI.h"


#ifdef I18N_MSG
#include <langinfo.h>
#include <locale.h>
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MESSAGE1	catgets(Xm_catd,MS_Scale,MSG_S_1,_XmMsgScale_0000)
#define MESSAGE2	catgets(Xm_catd,MS_Scale,MSG_S_2,_XmMsgScale_0001)
#define MESSAGE3	catgets(Xm_catd,MS_Scale,MSG_S_3,_XmMsgScale_0002)
#define MESSAGE5	catgets(Xm_catd,MS_Scale,MSG_S_5,_XmMsgScaleScrBar_0004)
#define MESSAGE6	catgets(Xm_catd,MS_Scale,MSG_S_6,_XmMsgScale_0005)
#define MESSAGE7	catgets(Xm_catd,MS_Scale,MSG_S_7,_XmMsgScale_0006)
#define MESSAGE8	catgets(Xm_catd,MS_Scale,MSG_S_8,_XmMsgScale_0007)
#define MESSAGE9	catgets(Xm_catd,MS_Scale,MSG_S_9,_XmMsgScale_0008)
#else
#define MESSAGE1	_XmMsgScale_0000
#define MESSAGE2	_XmMsgScale_0001
#define MESSAGE3	_XmMsgScale_0002
#define MESSAGE5	_XmMsgScaleScrBar_0004
#define MESSAGE6	_XmMsgScale_0005
#define MESSAGE7	_XmMsgScale_0006
#define MESSAGE8	_XmMsgScale_0007
#define MESSAGE9	_XmMsgScale_0008
#endif



/* Convenience macros and definitions */

#define TotalWidth(w)   (w->core.width + (w->core.border_width * 2))
#define TotalHeight(w)  (w->core.height + (w->core.border_width * 2))

#define SCROLLBAR_MAX	1000000000
#define SCROLLBAR_MIN	0
#define SLIDER_SIZE	30
#define SCALE_VALUE_MARGIN 3
#define SCALE_DEFAULT_MAJOR_SIZE \
	(100 + (2 * sw->scale.highlight_thickness))
#define SCALE_DEFAULT_MINOR_SIZE \
	(15 + (2 * sw->scale.highlight_thickness))

#define MAXINT 2147483647

#ifdef CDE_VISUAL  /* sliding_mode */
#define SLIDING_MODE( w) \
            *(((((Widget) w) == rec_cache_w) || GetInstanceExt( (Widget) w)), \
                                                  &(rec_cache->sliding_mode))

typedef struct _Scale_InstanceExtRec
    {   
        XtEnum         sliding_mode ;
    } Scale_InstanceExtRec, *Scale_InstanceExt ;
#endif /* CDE_VISUAL */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ScaleGetTitleString() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void ProcessingDirectionDefault() ;
static void ValidateInitialState() ;
static Widget CreateScaleTitle() ;
static Widget CreateScaleScrollBar() ;
static void Initialize() ;
static void GetForegroundGC() ;
static void Redisplay() ;
static void Resize() ;
static void ValidateInputs() ;
static Boolean NeedNewSize() ;
static void HandleTitle() ;
static void HandleScrollBar() ;
static Boolean SetValues() ;
static void SetValuesAlmost() ;
static void Realize() ;
static void Destroy() ;
static XtGeometryResult GeometryManager() ;
static Dimension MaxLabelWidth() ;
static Dimension MaxLabelHeight() ;
static Dimension ValueTroughWidth() ;
static Dimension ValueTroughHeight() ;
static Dimension TitleWidth() ;
static Dimension TitleHeight() ;
static Dimension MajorLeadPad() ;
static Dimension MajorTrailPad() ;
static Dimension ScrollWidth() ;
static Dimension ScrollHeight() ;
static void GetScaleSize() ;
static void LayoutHorizontalLabels() ;
static void LayoutHorizontalScale() ;
static void LayoutVerticalLabels() ;
static void LayoutVerticalScale() ;
static void LayoutScale() ;
static void ChangeManaged() ;
static void GetValueString();
static void ShowValue() ;
static void CalcScrollBarData() ;
static void ValueChanged() ;
static XtGeometryResult QueryGeometry() ;
static XmNavigability WidgetNavigable() ;
static void StartDrag () ;
static Boolean DragConvertProc () ;

#else

static void ScaleGetTitleString( 
                        Widget wid,
                        int resource,
                        XtArgVal *value) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void ProcessingDirectionDefault( 
                        XmScaleWidget widget,
                        int offset,
                        XrmValue *value) ;
static void ValidateInitialState( 
                        XmScaleWidget req,
                        XmScaleWidget new_w) ;
static Widget CreateScaleTitle( 
                        XmScaleWidget new_w) ;
static Widget CreateScaleScrollBar( 
                        XmScaleWidget new_w) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetForegroundGC( 
                        XmScaleWidget sw) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget wid) ;
static void ValidateInputs( 
                        XmScaleWidget cur,
                        XmScaleWidget new_w) ;
static Boolean NeedNewSize( 
                        XmScaleWidget cur,
                        XmScaleWidget new_w) ;
static void HandleTitle( 
                        XmScaleWidget cur,
                        XmScaleWidget req,
                        XmScaleWidget new_w) ;
static void HandleScrollBar( 
                        XmScaleWidget cur,
                        XmScaleWidget req,
                        XmScaleWidget new_w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args_in,
                        Cardinal *num_args_in) ;
static void SetValuesAlmost( 
                        Widget cw,
                        Widget nw,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void Realize( 
                        register Widget w,
                        XtValueMask *p_valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget wid) ;
static XtGeometryResult GeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static Dimension MaxLabelWidth( 
                        XmScaleWidget sw) ;
static Dimension MaxLabelHeight( 
                        XmScaleWidget sw) ;
static Dimension ValueTroughWidth( 
                        XmScaleWidget sw) ;
static Dimension ValueTroughHeight( 
                        XmScaleWidget sw) ;
static Dimension TitleWidth( 
                        XmScaleWidget sw) ;
static Dimension TitleHeight( 
                        XmScaleWidget sw) ;
static Dimension MajorLeadPad( 
                        XmScaleWidget sw) ;
static Dimension MajorTrailPad( 
                        XmScaleWidget sw) ;
static Dimension ScrollWidth( 
                        XmScaleWidget sw) ;
static Dimension ScrollHeight( 
                        XmScaleWidget sw) ;
static void GetScaleSize( 
                        XmScaleWidget sw,
                        Dimension *w,
                        Dimension *h) ;
static void LayoutHorizontalLabels( 
                        XmScaleWidget sw,
                        XRectangle *scrollBox,
                        XRectangle *labelBox,
                        Widget instigator) ;
static void LayoutHorizontalScale( 
                        XmScaleWidget sw,
#if NeedWidePrototypes
                        int optimum_w,
                        int optimum_h,
#else
                        Dimension optimum_w,
                        Dimension optimum_h,
#endif /* NeedWidePrototypes */
                        Widget instigator) ;
static void LayoutVerticalLabels( 
                        XmScaleWidget sw,
                        XRectangle *scrollBox,
                        XRectangle *labelBox,
                        Widget instigator) ;
static void LayoutVerticalScale( 
                        XmScaleWidget sw,
#if NeedWidePrototypes
                        int optimum_w,
                        int optimum_h,
#else
                        Dimension optimum_w,
                        Dimension optimum_h,
#endif /* NeedWidePrototypes */
                        Widget instigator) ;
static void LayoutScale( 
                        XmScaleWidget sw,
#if NeedWidePrototypes
                        int resizable,
#else
                        Boolean resizable,
#endif /* NeedWidePrototypes */
                        Widget instigator) ;
static void ChangeManaged( 
                        Widget wid) ;
static void GetValueString(
                        XmScaleWidget sw,
			int value,
                        String buffer);
static void ShowValue( 
                        XmScaleWidget sw,
                        int value,
#if NeedWidePrototypes
                        int show_new) ;
#else
                        Boolean show_new) ;
#endif /* NeedWidePrototypes */
static void CalcScrollBarData( 
                        XmScaleWidget sw,
                        int *value,
                        int *slider_size,
                        int *increment,
                        int *page) ;
static void ValueChanged( 
                        Widget wid,
                        XtPointer closure,
                        XtPointer call_data) ;
static XtGeometryResult QueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *desired) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;

static void StartDrag (Widget  w, 
                       XtPointer data, 
		       XEvent  *event, 
		       Boolean *cont) ;

static Boolean DragConvertProc (
    Widget              w,
    Atom                *selection,
    Atom                *target,
    Atom                *typeRtn,
    XtPointer           *valueRtn,
    unsigned long       *lengthRtn,
    int                 *formatRtn,
    unsigned long       *max_lengthRtn,
    XtPointer           client_data,
    XtRequestId         *request_id) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


#ifdef CDE_VISUAL  /* sliding_mode */
static XContext cde_rec_context ;
static Widget rec_cache_w ;
static Scale_InstanceExt rec_cache ;
enum{	XmSLIDER,	XmTHERMOMETER};
static char *SlidingModeNames[] =
{   "slider", "thermometer"
    } ;
#define NUM_NAMES( list )        (sizeof( list) / sizeof( char *))

static XtResource cde_res[] = 
{
    {   "slidingMode",
        "SlidingMode",
        "SlidingMode",
        sizeof( XtEnum),
        XtOffsetOf( struct _Scale_InstanceExtRec, sliding_mode),
        XmRImmediate,
        (XtPointer) XmSLIDER
    }
} ;
#endif /* CDE_VISUAL */

/* Default translation table and action list */

/*  Resource definitions for Scale class */

static XtResource resources[] =
{
    {
	XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension,
	sizeof (Dimension),
	XtOffsetOf( struct _XmManagerRec, manager.shadow_thickness),
	XmRImmediate, (XtPointer) 2
	},
    {
	XmNvalue, XmCValue, XmRInt, sizeof(int),
	XtOffsetOf( struct _XmScaleRec,scale.value),
	XmRImmediate, (XtPointer) MAXINT
	},

    {
	XmNmaximum, XmCMaximum, XmRInt, sizeof(int),
	XtOffsetOf( struct _XmScaleRec,scale.maximum), 
	XmRImmediate, (XtPointer)100
   },

   {
     XmNminimum, XmCMinimum, XmRInt, sizeof(int),
     XtOffsetOf( struct _XmScaleRec,scale.minimum), XmRImmediate, (XtPointer)0
   },

   {
     XmNorientation, XmCOrientation, XmROrientation, sizeof(unsigned char),
     XtOffsetOf( struct _XmScaleRec,scale.orientation), 
     XmRImmediate, (XtPointer) XmVERTICAL
   },

   {
     XmNprocessingDirection, XmCProcessingDirection, XmRProcessingDirection,
     sizeof(unsigned char),
     XtOffsetOf( struct _XmScaleRec,scale.processing_direction), XmRCallProc, 
     (XtPointer) ProcessingDirectionDefault
   },

   {
     XmNtitleString, XmCTitleString, XmRXmString, sizeof(XmString),
     XtOffsetOf( struct _XmScaleRec,scale.title), XmRImmediate, NULL
   },

   {
     XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
     XtOffsetOf( struct _XmScaleRec, scale.font_list), XmRImmediate, NULL
   },

   {
     XmNshowValue, XmCShowValue, XmRBoolean, sizeof(Boolean),
     XtOffsetOf( struct _XmScaleRec,scale.show_value), XmRImmediate, (XtPointer) False
   },
         
   {
     XmNdecimalPoints, XmCDecimalPoints, XmRShort, sizeof(short),
     XtOffsetOf( struct _XmScaleRec,scale.decimal_points), XmRImmediate, (XtPointer) 0
   },

   {
     XmNscaleWidth, XmCScaleWidth, XmRHorizontalDimension,
	 sizeof (Dimension),
     XtOffsetOf( struct _XmScaleRec, scale.scale_width),
	 XmRImmediate, (XtPointer) 0
   },

   {
     XmNscaleHeight, XmCScaleHeight, XmRVerticalDimension,
	 sizeof (Dimension),
     XtOffsetOf( struct _XmScaleRec, scale.scale_height),
	 XmRImmediate, (XtPointer) 0
   },

   {
     XmNhighlightThickness, XmCHighlightThickness,
	 XmRHorizontalDimension, sizeof (Dimension),
     XtOffsetOf( struct _XmScaleRec, scale.highlight_thickness),
     XmRImmediate, (XtPointer) 2
   },

   {
     XmNhighlightOnEnter, XmCHighlightOnEnter, XmRBoolean, sizeof (Boolean),
     XtOffsetOf( struct _XmScaleRec, scale.highlight_on_enter),
     XmRImmediate, (XtPointer) False
   },


   {
     XmNvalueChangedCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmScaleRec,scale.value_changed_callback), 
     XmRCallback, NULL
   },

   { 
     XmNdragCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
     XtOffsetOf( struct _XmScaleRec,scale.drag_callback), XmRCallback, NULL
   },
   {
	XmNscaleMultiple, XmCScaleMultiple, XmRInt, sizeof(int),
	XtOffsetOf( struct _XmScaleRec,scale.scale_multiple), 
	XmRImmediate, (XtPointer) 0
   },
};


/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
	{ XmNtitleString,
	  sizeof (XmString),
	  XtOffsetOf( struct _XmScaleRec, scale.title), 
	  ScaleGetTitleString,
	  (XmImportProc)NULL 
	},
	{ XmNscaleWidth,
	  sizeof (Dimension),
	  XtOffsetOf( struct _XmScaleRec, scale.scale_width), 
	  _XmFromHorizontalPixels,
	  _XmToHorizontalPixels 
	},
	{ XmNscaleHeight,
	  sizeof (Dimension),
	  XtOffsetOf( struct _XmScaleRec, scale.scale_height), 
	  _XmFromVerticalPixels,
	  _XmToVerticalPixels
	}
};


/*  Scale class record definition  */

static XmBaseClassExtRec BaseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    (XtInitProc)NULL,			/* InitializePrehook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPrehook	*/
    (XtInitProc)NULL,			/* InitializePosthook	*/
    (XtSetValuesFunc)NULL,		/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    (XtInitProc)NULL,			/* secondaryCreate	*/
    (XmGetSecResDataFunc)NULL, 		/* getSecRes data	*/
    { 0 },      			/* fastSubclass flags	*/
    (XtArgsProc)NULL,			/* getValuesPrehook	*/
    (XtArgsProc)NULL,			/* getValuesPosthook	*/
    (XtWidgetClassProc)NULL,            /* classPartInitPrehook */
    (XtWidgetClassProc)NULL,            /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    WidgetNavigable,                    /* widgetNavigable      */
    (XmFocusChangeProc)NULL,            /* focusChange          */
    (XmWrapperData)NULL			/* wrapperData 		*/
};

externaldef(xmscaleclassrec) XmScaleClassRec xmScaleClassRec = 
{
   {                                            /* core_class fields    */
      (WidgetClass) &xmManagerClassRec,         /* superclass         */
      "XmScale",                                /* class_name         */
      sizeof(XmScaleRec),                       /* widget_size        */
      ClassInitialize,                          /* class_initialize   */
      ClassPartInitialize,                      /* class_part_init    */
      FALSE,                                    /* class_inited       */
      Initialize,                               /* initialize         */
      (XtArgsProc)NULL,                         /* initialize_hook    */
      Realize,                                  /* realize            */
      NULL,                                     /* actions            */
      0,                                        /* num_actions        */
      resources,                                /* resources          */
      XtNumber(resources),                      /* num_resources      */
      NULLQUARK,                                /* xrm_class          */
      TRUE,                                     /* compress_motion    */
      XtExposeCompressMaximal,                  /* compress_exposure  */
      TRUE,                                     /* compress_enterlv   */
      FALSE,                                    /* visible_interest   */
      Destroy,                                  /* destroy            */
      Resize,                                   /* resize             */
      Redisplay,                                /* expose             */
      SetValues,                                /* set_values         */
      (XtArgsFunc)NULL,                         /* set_values_hook    */
      SetValuesAlmost,                          /* set_values_almost  */
      (XtArgsProc)NULL,                         /* get_values_hook    */
      (XtAcceptFocusProc)NULL,                  /* accept_focus       */
      XtVersion,                                /* version            */
      NULL,                                     /* callback_private   */
      XtInheritTranslations,                    /* tm_table           */
      (XtGeometryHandler) QueryGeometry,        /* query_geometry     */
      (XtStringProc)NULL,                       /* display_accelerator*/
      (XtPointer)&BaseClassExtRec,              /* extension          */
   },

   {                                            /* composite_class fields */
      GeometryManager,                          /* geometry_manager   */
      ChangeManaged,                            /* change_managed     */
      XtInheritInsertChild,                     /* insert_child       */
      XtInheritDeleteChild,                     /* delete_child       */
      NULL,                                     /* extension          */
   },

   {                                            /* constraint_class fields */
      NULL,                                     /* resource list        */   
      0,                                        /* num resources        */   
      0,                                        /* constraint size      */   
      (XtInitProc)NULL,                         /* init proc            */   
      (XtWidgetProc)NULL,                       /* destroy proc         */   
      (XtSetValuesFunc)NULL,                    /* set values proc      */   
      NULL,                                     /* extension            */
   },


   {		/* manager_class fields */
      XtInheritTranslations,			/* translations           */
      syn_resources,				/* syn_resources      	  */
      XtNumber(syn_resources),			/* num_syn_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_syn_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension           	  */
   },

   {                                            /* scale class - none */     
      (XtPointer) NULL                          /* extension */
   }    
};

externaldef(xmscalewidgetclass) WidgetClass
	xmScaleWidgetClass = (WidgetClass)&xmScaleClassRec;



static void
#ifdef _NO_PROTO
ScaleGetTitleString(wid, resource, value)
        Widget wid;
        int resource;
        XtArgVal *value;
#else
ScaleGetTitleString(
        Widget wid,
        int resource,
        XtArgVal *value)
#endif /* _NO_PROTO */
/****************           ARGSUSED  ****************/
{
	XmScaleWidget scale = (XmScaleWidget) wid ;
	Arg           al[1] ;

	if (scale->scale.title == NULL) {
	    /* mean that the title has never been set, so 
	       we should return NULL, not the label value which
	       is the label name, not NULL,  in this case */
	    *value = (XtArgVal) NULL ;
	} else { 
	    /* title = -1, our magic value used to tell: look in
	       the label child. */
	    XtSetArg (al[0], XmNlabelString, value);	/* make a copy */
	    XtGetValues (scale->composite.children[0], al, 1);
	}
}

static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  BaseClassExtRec.record_type = XmQmotif ;
#ifdef CDE_VISUAL /* sliding_mode */
    XmRepTypeRegister( "SlidingMode", SlidingModeNames, NULL,
                                             NUM_NAMES(SlidingModeNames));
#endif  /* CDE_VISUAL */
}

/************************************************************************
 *
 *  ClassPartInitialize
 *     Initialize the fast subclassing.
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
   _XmFastSubclassInit (wc, XmSCALE_BIT);
}




/*********************************************************************
 *
 * ProcessingDirectionDefault
 *    This procedure provides the dynamic default behavior for
 *    the processing direction resource dependent on the orientation.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ProcessingDirectionDefault( widget, offset, value )
        XmScaleWidget widget ;
        int offset ;
        XrmValue *value ;
#else
ProcessingDirectionDefault(
        XmScaleWidget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static unsigned char direction;

	value->addr = (XPointer) &direction;

	if (widget->scale.orientation == XmHORIZONTAL)
		direction = XmMAX_ON_RIGHT;
	else /* XmVERTICAL  -- range checking done during widget
		                   initialization */
		direction = XmMAX_ON_TOP;
}



/*********************************************************************
 *  Initialize
 *      Validate all of the argument data for the widget, create the
 *	title label and scrollbar.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ValidateInitialState( req, new_w )
        XmScaleWidget req ;
        XmScaleWidget new_w ;
#else
ValidateInitialState(
        XmScaleWidget req,
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	Boolean default_value = FALSE;
        float value_range;
	
	if (new_w->scale.value == MAXINT)
	{
		new_w->scale.value = 0;
		default_value = True;
	}

	if (new_w->scale.minimum >= new_w->scale.maximum)
	{
		new_w->scale.minimum = 0;
		new_w->scale.maximum = 100;
		_XmWarning( (Widget) new_w, MESSAGE1);
	}

        value_range = (float)((float)new_w->scale.maximum - (float)new_w->scale.minimum);
        if (value_range > (float)((float)INT_MAX / (float) 2.0))
        {
             new_w->scale.minimum = 0;
	     if (new_w->scale.maximum > (INT_MAX / 2))
	         new_w->scale.maximum = INT_MAX / 2;
            _XmWarning( (Widget) new_w, MESSAGE9);
        }

	if (new_w->scale.value < new_w->scale.minimum)
	{
		new_w->scale.value = new_w->scale.minimum;
		if (!default_value) _XmWarning( (Widget) new_w, MESSAGE2);
	}

	if (new_w->scale.value > new_w->scale.maximum)
	{
		new_w->scale.value = new_w->scale.minimum;
		if (!default_value) _XmWarning( (Widget) new_w, MESSAGE3);
	}

	if(    !XmRepTypeValidValue( XmRID_ORIENTATION,
                                     new_w->scale.orientation, (Widget) new_w)    )
	{
		new_w->scale.orientation = XmVERTICAL;
	}

	if (new_w->scale.orientation == XmHORIZONTAL)
	{
		if ((new_w->scale.processing_direction != XmMAX_ON_RIGHT) &&
			(new_w->scale.processing_direction != XmMAX_ON_LEFT))

		{
			new_w->scale.processing_direction = XmMAX_ON_RIGHT;
			_XmWarning( (Widget) new_w, MESSAGE5);
		}
	}
	else
	{
		if ((new_w->scale.processing_direction != XmMAX_ON_TOP) &&
			(new_w->scale.processing_direction != XmMAX_ON_BOTTOM))
		{
			new_w->scale.processing_direction = XmMAX_ON_TOP;
			_XmWarning( (Widget) new_w, MESSAGE5);
		}
	}

	if (new_w->scale.scale_multiple > (new_w->scale.maximum 
		- new_w->scale.minimum))
	{
		_XmWarning( (Widget) new_w, MESSAGE7);
		new_w->scale.scale_multiple = (new_w->scale.maximum
			- new_w->scale.minimum) / 10;
	}
	else if (new_w->scale.scale_multiple < 0)
	{
		_XmWarning( (Widget) new_w, MESSAGE8);
		new_w->scale.scale_multiple = (new_w->scale.maximum
			- new_w->scale.minimum) / 10;
	}
	else if (new_w->scale.scale_multiple == 0)
		new_w->scale.scale_multiple = (new_w->scale.maximum
			- new_w->scale.minimum) / 10;
}

static Widget 
#ifdef _NO_PROTO
CreateScaleTitle( new_w )
        XmScaleWidget new_w ;
#else
CreateScaleTitle(
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	XmLabelGadget title;
	Arg args[5];
	int n;

	/*  Create the title label gadget  */

	/* title can be NULL or a valid XmString, if null,
	   the label will use its own name as XmString */
	n = 0;
	XtSetArg (args[n], XmNlabelString, new_w->scale.title);	n++;
	XtSetArg (args[n], XmNfontList, new_w->scale.font_list);	n++;

	title = (XmLabelGadget) XmCreateLabelGadget( (Widget) new_w, 
						    "Title",
		args, n);

	if (new_w->scale.title) {
	    XtManageChild ((Widget) title);
	    new_w->scale.title = (XmString) -1 ;
	} /* scale.title need to be set to some special not NULL value
	     in order to see any change at SetValues time and also to
	     return NULL at Getvalue time in the hook. This is pirs 3197:
	     when you setvalues a new xmstring as title, the value of the
	     title field, a pointer, might be the same. */

	return((Widget) title);
}

#ifdef CDE_VISUAL  /* sliding_mode */
static Scale_InstanceExt
#ifdef _NO_PROTO
NewInstanceExt( w, args, nargs )
        Widget w ;
        ArgList args ;
        Cardinal nargs ;
#else
NewInstanceExt(
        Widget w,
        ArgList args,
        Cardinal nargs)
#endif /* _NO_PROTO */
{   
  rec_cache = (Scale_InstanceExt) XtCalloc( 1, sizeof(Scale_InstanceExtRec)) ;
  XtGetSubresources( w, (XtPointer) rec_cache, NULL, NULL, cde_res,
                                             XtNumber( cde_res), args, nargs) ;
  XSaveContext( XtDisplay(w), (Window) w, cde_rec_context,
                                                        (XPointer) rec_cache) ;
  rec_cache_w = w ;
  return rec_cache ;
}

static Scale_InstanceExt
#ifdef _NO_PROTO
GetInstanceExt( w )
        Widget w ;
#else
GetInstanceExt(
        Widget w)
#endif /* _NO_PROTO */
{   
  if(    XFindContext( XtDisplay( w), (Window) w, cde_rec_context,
                                                  (XPointer *) &rec_cache)    )
    {
      rec_cache_w = NULL ;
      return NULL ;
    } 
  rec_cache_w = w ;
  return rec_cache ;
}

static void
#ifdef _NO_PROTO
FreeInstanceExt( w, rec )
        Widget w ;
        Scale_InstanceExt rec ;
#else
FreeInstanceExt(
        Widget w,
        Scale_InstanceExt rec)
#endif /* _NO_PROTO */
{   
  XDeleteContext( XtDisplay( w), (Window) w, cde_rec_context) ;
  XtFree( (char *) rec) ;
  if(    rec == rec_cache    )
    {  
      rec_cache_w = NULL ;
    } 
}
#endif /* CDE_VISUAL */

static Widget 
#ifdef _NO_PROTO
CreateScaleScrollBar( new_w )
        XmScaleWidget new_w ;
#else
CreateScaleScrollBar(
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	XmScrollBarWidget scrollbar;
	Arg args[25];
	int n;

	/*  Build up an arg list for and create the scrollbar  */

	n = 0;
	XtSetArg (args[n], XmNmaximum, SCROLLBAR_MAX);	n++;
	XtSetArg (args[n], XmNminimum, SCROLLBAR_MIN);	n++;
	XtSetArg (args[n], XmNshowArrows, False);		n++;
	XtSetArg (args[n], XmNorientation, new_w->scale.orientation);	n++;
	XtSetArg (args[n], XmNprocessingDirection,
		new_w->scale.processing_direction);       n++;
	XtSetArg (args[n], XmNhighlightColor, 
		new_w->manager.highlight_color);		n++;
	XtSetArg (args[n], XmNhighlightThickness, 
		new_w->scale.highlight_thickness);		n++;
	XtSetArg (args[n], XmNhighlightColor, 
		new_w->manager.highlight_color);		n++;
	XtSetArg (args[n], XmNhighlightOnEnter, 
		new_w->scale.highlight_on_enter);			n++;
	XtSetArg (args[n], XmNtraversalOn, 
		new_w->manager.traversal_on);				n++;
	XtSetArg (args[n], XmNshadowThickness, 
		new_w->manager.shadow_thickness);			n++;
	XtSetArg (args[n], XmNbackground, new_w->core.background_pixel);	n++;
	XtSetArg (args[n], XmNbackgroundPixmap, 
		new_w->core.background_pixmap);	n++;
	XtSetArg (args[n], XmNtopShadowColor,
		new_w->manager.top_shadow_color); n++;
	XtSetArg (args[n], XmNtopShadowPixmap, 
		new_w->manager.top_shadow_pixmap);		n++;
	XtSetArg (args[n], XmNbottomShadowColor, 
		new_w->manager.bottom_shadow_color);		n++;
	XtSetArg (args[n], XmNbottomShadowPixmap,
		new_w->manager.bottom_shadow_pixmap);		n++;
	XtSetArg (args[n], XmNunitType, XmPIXELS);	n++;
	if (new_w->scale.scale_width != 0)
	{
		XtSetArg (args[n], XmNwidth, new_w->scale.scale_width);	n++;
	}
	if (new_w->scale.scale_height != 0)
	{
		XtSetArg (args[n], XmNheight, new_w->scale.scale_height);	n++;
	}
#ifdef CDE_VISUAL  /* sliding_mode */
	if (SLIDING_MODE(new_w) == XmTHERMOMETER) {
		XtSetArg(args[n], XmNsliderSize, 1); n++;
	}
#endif /* CDE_VISUAL */

	scrollbar = (XmScrollBarWidget) XmCreateScrollBar( (Widget) new_w,
		"Scrollbar", args, n);

#ifdef CDE_VISUAL  /* sliding_mode */
	_CDESetScrollBarVisual( scrollbar, (SLIDING_MODE(new_w) == XmTHERMOMETER));
#else
	_XmSetEtchedSlider( scrollbar);
#endif

	XtManageChild((Widget) scrollbar);

	XtAddCallback((Widget) scrollbar, XmNvalueChangedCallback,
		ValueChanged, NULL);
	XtAddCallback((Widget) scrollbar, XmNdragCallback,
		ValueChanged, NULL);
	XtAddCallback((Widget) scrollbar, XmNtoTopCallback,
		ValueChanged,   NULL);
	XtAddCallback((Widget) scrollbar, XmNtoBottomCallback,
		ValueChanged,   NULL);

	return((Widget) scrollbar);
}

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
    XmScaleWidget req = (XmScaleWidget) rw ;
    XmScaleWidget new_w = (XmScaleWidget) nw ;
    Widget title;
    Widget scrollbar;

#ifdef CDE_VISUAL  /* sliding_mode */
    NewInstanceExt( nw, args, *num_args) ;
#endif

    /* Validate the incoming data  */                      
    ValidateInitialState(req, new_w);
    
    if (new_w->scale.font_list == NULL)
	new_w->scale.font_list =
	    _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST);
    
    /*  Set the scale font struct used for interactive value display  */
    /*  to the 0th font in the title font list.  If not font list is  */
    /*  provides, open up fixed and use that.                         */
    
#ifdef OSF_v1_2_4
    new_w->scale.font_list = XmFontListCopy(new_w->scale.font_list);

#endif /* OSF_v1_2_4 */
    if (new_w->scale.font_list)
	{
	_XmFontListGetDefaultFont(new_w->scale.font_list,
				      &new_w->scale.font_struct);
	}
    else
	{
	new_w->scale.font_struct = XLoadQueryFont (XtDisplay (new_w), "fixed");
	if (new_w->scale.font_struct == NULL)
	    new_w->scale.font_struct = XLoadQueryFont (XtDisplay (new_w), "*");
	}
    
    title = CreateScaleTitle(new_w);
    scrollbar = CreateScaleScrollBar(new_w);
    
    /*  Get the foreground GC and initialize internal variables  */
    
    GetForegroundGC (new_w);
    
    new_w->scale.show_value_x = 0;
    new_w->scale.show_value_y = 0;
    new_w->scale.show_value_width = 0;
    new_w->scale.show_value_height = 0;

#if defined(CDE_NO_DRAG_FROM_LABELS)
  {
  Boolean unselectable_drag;
  XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(nw)), "enableUnselectableDrag",
      &unselectable_drag, NULL);
  if (unselectable_drag)
    XtAddEventHandler(nw, ButtonPressMask, False, StartDrag, NULL);
  }
#else
    XtAddEventHandler(nw, ButtonPressMask, False, StartDrag, NULL);
#endif /* CDE_NO_DRAG_FROM_LABELS */
}


/************************************************************************
 *
 * StartDrag:
 * This routine is performed by the initiator when a drag starts 
 * (in this case, when mouse button 2 was pressed).  It starts 
 * the drag processing, and establishes a drag context
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
StartDrag(w, data, event, cont)
Widget  w;
XtPointer data ;
XEvent  *event;
Boolean *cont ;
#else
StartDrag (Widget  w, 
	   XtPointer data, 
	   XEvent  *event, 
	   Boolean *cont)
#endif /* _NO_PROTO */
{
   Widget drag_icon;
    Arg             args[10];
   Cardinal        n;
   Atom            exportList[2];
   XmScaleWidget sw = (XmScaleWidget) w ;

   /* first check that the click is OK: button 2 and in the value label */
   if ((!sw->scale.show_value) ||
       (event->xbutton.button != Button2) ||
       ((event->xbutton.x < sw->scale.show_value_x) ||
	(event->xbutton.y < sw->scale.show_value_y) ||
	(event->xbutton.x > sw->scale.show_value_x + 
	 sw->scale.show_value_width) ||
	(event->xbutton.y > sw->scale.show_value_y +
	 sw->scale.show_value_height))) return ;
   
   /* establish the list of valid target types */
   exportList[0] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
   exportList[1] = XmInternAtom(XtDisplay(w), "STRING", False);

   drag_icon = _XmGetTextualDragIcon(w);

   n = 0;
   XtSetArg(args[n], XmNcursorBackground, sw->core.background_pixel);  n++;
   XtSetArg(args[n], XmNcursorForeground, sw->manager.foreground);  n++;
   XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++; 
   XtSetArg(args[n], XmNexportTargets, exportList); n++;
   XtSetArg(args[n], XmNnumExportTargets, 2); n++;
   XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++ ;
   XtSetArg(args[n], XmNconvertProc, DragConvertProc); n++;
   XtSetArg(args[n], XmNclientData, w);  n++;
   (void) XmDragStart(w, event, args, n);
}


/************************************************************************
 *
 * DragConvertProc:
 * This routine returns the value of the scale, 
 * converted into compound text. 
 *
 ************************************************************************/
static Boolean
#ifdef _NO_PROTO
DragConvertProc (w, selection, target, typeRtn, 
		 valueRtn, lengthRtn, formatRtn, 
		 max_lengthRtn, client_data, 
		 request_id)
Widget              w;
Atom                *selection;
Atom                *target;
Atom                *typeRtn;
XtPointer           *valueRtn;
unsigned long       *lengthRtn;
int                 *formatRtn;
unsigned long       *max_lengthRtn;
XtPointer           client_data;
XtRequestId         *request_id;
#else
DragConvertProc (Widget              w,
		 Atom                *selection,
		 Atom                *target,
		 Atom                *typeRtn,
		 XtPointer           *valueRtn,
		 unsigned long       *lengthRtn,
		 int                 *formatRtn,
		 unsigned long       *max_lengthRtn,
		 XtPointer           client_data,
		 XtRequestId         *request_id)
#endif /* _NO_PROTO */
{

   static char tmpstring[100];
   char        *strlist;
   char        *passtext;
   XmScaleWidget sw ;
   Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
   Atom STRING = XmInternAtom(XtDisplay(w), "STRING", False);
   Atom MOTIF_DROP = XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False);
   Arg args[1];
   XtPointer c_ptr;
   XTextProperty tp;
      
   if (*selection == MOTIF_DROP) {
       XtSetArg (args[0], XmNclientData, &c_ptr);
       XtGetValues (w, args, 1);
       sw = (XmScaleWidget) c_ptr;
    } else
       return False;

    if (sw == NULL) return False;

   /* Begin fixing the bug OSF 4846 */
   /* get the value of the scale and convert it to compound text */
   GetValueString(sw, sw->scale.value, tmpstring);

   /* handle plain STRING first */
   if (*target == STRING) {
       *typeRtn = *target;
       *valueRtn = (XtPointer) tmpstring;
       *lengthRtn = strlen(tmpstring);
       *formatRtn = 8;
       return(True);
   }

   /* this routine processes only compound text now */
   if (*target != COMPOUND_TEXT) return(False);

   strlist = tmpstring; 
#ifdef NON_OSF_FIX
   tp.value = NULL;
#endif
   XmbTextListToTextProperty(XtDisplay(w), &strlist, 1, 
			     XCompoundTextStyle, &tp);
   passtext = XtNewString((char*)tp.value);
   XFree((char*)tp.value);
   /* End fixing the bug OSF 4846 */


   /* format the value for transfer.  convert the value from
   * compound string to compound text for the transfer */
   *typeRtn = COMPOUND_TEXT;
   *valueRtn = (XtPointer) passtext;
   *lengthRtn = strlen(passtext);
   *formatRtn = 8;
   return(True);
}


/************************************************************************
 *
 *  GetForegroundGC
 *     Get the graphics context used for drawing the slider value.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetForegroundGC( sw )
        XmScaleWidget sw ;
#else
GetForegroundGC(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground | GCFont;
   values.foreground = sw->manager.foreground;
   values.background = sw->core.background_pixel;
   values.font = sw->scale.font_struct->fid;

   sw->scale.foreground_GC = XtGetGC ((Widget) sw, valueMask, &values);
}




/************************************************************************
 *
 *  Redisplay
 *     General redisplay function called on exposure events.
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
        XmScaleWidget sw = (XmScaleWidget) wid ;
   _XmRedisplayGadgets( (Widget) sw, event, region);
   
   ShowValue (sw, sw->scale.value, sw->scale.show_value);
}




/************************************************************************
 *
 *  Resize
 *     Re-layout children.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmScaleWidget sw = (XmScaleWidget) wid ;
    LayoutScale (sw, FALSE, NULL);


   /*  If the scale is realized and the value is being displayed,   */
   /*  then ensure the value gets erased before being redisplayed.  */

   if (XtIsRealized (sw) && sw->scale.show_value)
   {
      sw->scale.last_value = sw->scale.value - 1;
      if (sw->scale.show_value)
         ShowValue (sw, sw->scale.value, True);
      else
         ShowValue (sw, sw->scale.value, False);
   }
}

static void 
#ifdef _NO_PROTO
ValidateInputs( cur, new_w )
        XmScaleWidget cur ;
        XmScaleWidget new_w ;
#else
ValidateInputs(
        XmScaleWidget cur,
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
   float value_range;
   /* Validate the incoming data  */                      

   if (new_w->scale.minimum >= new_w->scale.maximum)
   {
      new_w->scale.minimum = cur->scale.minimum;
      new_w->scale.maximum = cur->scale.maximum;
      _XmWarning( (Widget) new_w, MESSAGE1);
   }

   value_range = (float)((float)new_w->scale.maximum - (float)new_w->scale.minimum);
   if (value_range > (float)((float)INT_MAX / (float) 2.0))
   {
       new_w->scale.minimum = 0;
         if (new_w->scale.maximum > (INT_MAX / 2))
             new_w->scale.maximum = INT_MAX / 2;
        _XmWarning( (Widget) new_w, MESSAGE9);
   }
 
   if (new_w->scale.value < new_w->scale.minimum)
   {
      new_w->scale.value = new_w->scale.minimum;
      _XmWarning( (Widget) new_w, MESSAGE2);
   }

   if (new_w->scale.value > new_w->scale.maximum)
   {
      new_w->scale.value = new_w->scale.maximum;
      _XmWarning( (Widget) new_w, MESSAGE3);
   }

   if(    !XmRepTypeValidValue( XmRID_ORIENTATION,
                                     new_w->scale.orientation, (Widget) new_w)    )
   {
      new_w->scale.orientation = cur->scale.orientation;
   }

   if (new_w->scale.orientation == XmHORIZONTAL)
   {
      if (new_w->scale.processing_direction != XmMAX_ON_LEFT &&
          new_w->scale.processing_direction != XmMAX_ON_RIGHT)
      {
         new_w->scale.processing_direction = cur->scale.processing_direction;
         _XmWarning( (Widget) new_w, MESSAGE5);
      }
   }
   else
   {
      if (new_w->scale.processing_direction != XmMAX_ON_TOP &&
          new_w->scale.processing_direction != XmMAX_ON_BOTTOM)
      {
         new_w->scale.processing_direction = cur->scale.processing_direction;
         _XmWarning( (Widget) new_w, MESSAGE5);
      }
   }


   if (new_w->scale.scale_multiple != cur->scale.scale_multiple)
	{
		if (new_w->scale.scale_multiple > (new_w->scale.maximum 
			- new_w->scale.minimum))
		{
			_XmWarning( (Widget) new_w, MESSAGE7);
			new_w->scale.scale_multiple = (new_w->scale.maximum
				- new_w->scale.minimum) / 10;
		}
		else if (new_w->scale.scale_multiple < 0)
		{
			_XmWarning( (Widget) new_w, MESSAGE8);
			new_w->scale.scale_multiple = (new_w->scale.maximum
				- new_w->scale.minimum) / 10;
		}
		else if (new_w->scale.scale_multiple == 0)
			new_w->scale.scale_multiple = (new_w->scale.maximum
			- new_w->scale.minimum) / 10;
	}
}

static Boolean 
#ifdef _NO_PROTO
NeedNewSize( cur, new_w )
        XmScaleWidget cur ;
        XmScaleWidget new_w ;
#else
NeedNewSize(
        XmScaleWidget cur,
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	Boolean flag = FALSE;

#define DIFF(x) ((new_w->x) != (cur->x))

	if ((DIFF(scale.font_list)) ||
	    (DIFF(scale.highlight_thickness)) ||
	    (DIFF(scale.scale_height)) ||
	    (DIFF(scale.scale_width)) ||
	    (DIFF(scale.show_value)) ||
	    (DIFF(scale.orientation)) ||
	    (DIFF(manager.unit_type)) ||
	    (DIFF(manager.shadow_thickness)))
	    flag = TRUE;
#undef DIFF

	return(flag);
}

static void 
#ifdef _NO_PROTO
HandleTitle( cur, req, new_w )
        XmScaleWidget cur ;
        XmScaleWidget req ;
        XmScaleWidget new_w ;
#else
HandleTitle(
        XmScaleWidget cur,
        XmScaleWidget req,
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	Arg args[5];
	int n = 0;

	/* cur title is either NULL or (-1), as set in CreateScaleTitle,
	   so diff are always pertinent */
	/* new title can be NULL or a valid xmstring */
	if (new_w->scale.title != cur->scale.title) {
	    XtSetArg (args[n], XmNlabelString, new_w->scale.title);	n++;
	}

	if (new_w->scale.font_list != cur->scale.font_list) {
	    XtSetArg (args[n], XmNfontList, new_w->scale.font_list);	n++;
	}

	if (n) XtSetValues (new_w->composite.children[0], args, n);
	
	if (new_w->scale.title != cur->scale.title) {
	    if (new_w->scale.title != NULL) {
		/* new title differs from old one and is no null, so
		   it's a valid xmstring that we change to -1 */
		XtManageChild(new_w->composite.children[0]);
		new_w->scale.title = (XmString) -1 ;
	    }
	    else  /* new title differs from old one and is null,
		   so we let it be null, so that get scale title returns
		   null instead of the label string */
		XtUnmanageChild (new_w->composite.children[0]);
	}
}

static void 
#ifdef _NO_PROTO
HandleScrollBar( cur, req, new_w )
        XmScaleWidget cur ;
        XmScaleWidget req ;
        XmScaleWidget new_w ;
#else
HandleScrollBar(
        XmScaleWidget cur,
        XmScaleWidget req,
        XmScaleWidget new_w )
#endif /* _NO_PROTO */
{
	int value, increment, page, slider_size;
	Arg args[30];
	int n = 0;

	/* Build up an arg list to reset any attributes of the scrollbar */
	/* Note: this code must be changed, it's too much work each time, 
	   better check for cur Widget state before doing anything */

	n = 0;
	XtSetArg (args[n], XmNsensitive, new_w->core.sensitive);      n++;
	XtSetArg (args[n], XmNorientation, new_w->scale.orientation);	n++;
	XtSetArg (args[n], XmNprocessingDirection, 
		new_w->scale.processing_direction);		n++;
	XtSetArg (args[n], XmNhighlightColor, 
		new_w->manager.highlight_color);		n++;
	XtSetArg (args[n], XmNhighlightThickness, 
		new_w->scale.highlight_thickness);		n++;
	XtSetArg (args[n], XmNhighlightColor, 
		new_w->manager.highlight_color);		n++;
	XtSetArg (args[n], XmNshadowThickness, 
		new_w->manager.shadow_thickness);			n++;
	XtSetArg (args[n], XmNhighlightOnEnter, 
		new_w->scale.highlight_on_enter);			n++;
	XtSetArg (args[n], XmNtraversalOn, 
		new_w->manager.traversal_on);				n++;
	XtSetArg (args[n], XmNbackground, new_w->core.background_pixel);	n++;
	XtSetArg (args[n], XmNbackgroundPixmap,
		new_w->core.background_pixmap); 			n++;
	XtSetArg (args[n], XmNtopShadowColor,
		new_w->manager.top_shadow_color); 		n++;
	XtSetArg (args[n], XmNtopShadowPixmap, 
		new_w->manager.top_shadow_pixmap);		n++;
	XtSetArg (args[n], XmNbottomShadowColor, 
		new_w->manager.bottom_shadow_color);		n++;
	XtSetArg (args[n], XmNbottomShadowPixmap,
		new_w->manager.bottom_shadow_pixmap);		n++;
	if (new_w->scale.scale_width != cur->scale.scale_width)
	{
		XtSetArg (args[n], XmNwidth, new_w->scale.scale_width);	n++;
	}
	if (new_w->scale.scale_height != cur->scale.scale_height)
	{
		XtSetArg (args[n], XmNheight, new_w->scale.scale_height);	n++;
	}

	CalcScrollBarData(new_w, &value, &slider_size,
		&increment, &page);

	new_w->scale.slider_size = slider_size;

	XtSetArg (args[n], XmNvalue, value);		n++;
	XtSetArg (args[n], XmNsliderSize, new_w->scale.slider_size);	n++;
	XtSetArg (args[n], XmNincrement, increment);		n++;
	XtSetArg (args[n], XmNpageIncrement, page);		n++;

	XtSetValues (new_w->composite.children[1], args, n);
}

static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args_in, num_args_in )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args_in ;
        Cardinal *num_args_in ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args_in,
        Cardinal *num_args_in )
#endif /* _NO_PROTO */
{
        XmScaleWidget cur = (XmScaleWidget) cw ;
        XmScaleWidget req = (XmScaleWidget) rw ;
        XmScaleWidget new_w = (XmScaleWidget) nw ;
	Boolean redisplay = False;

	/* Make sure that processing direction tracks orientation */

	if ((new_w->scale.orientation != cur->scale.orientation)
		&&
		(new_w->scale.processing_direction ==
			cur->scale.processing_direction))
	{
		if ((new_w->scale.orientation == XmHORIZONTAL) &&
			(cur->scale.processing_direction == XmMAX_ON_TOP))
			new_w->scale.processing_direction = XmMAX_ON_RIGHT;
		else if ((new_w->scale.orientation == XmHORIZONTAL) &&
			(cur->scale.processing_direction ==
				XmMAX_ON_BOTTOM))
			new_w->scale.processing_direction = XmMAX_ON_LEFT;
		else if ((new_w->scale.orientation == XmVERTICAL) &&
			(cur->scale.processing_direction == XmMAX_ON_LEFT))
			new_w->scale.processing_direction = XmMAX_ON_BOTTOM;
		else if ((new_w->scale.orientation == XmVERTICAL) &&
			(cur->scale.processing_direction == XmMAX_ON_RIGHT))
			new_w->scale.processing_direction = XmMAX_ON_RIGHT;
	}

	/* Make scale width and height track orientation */

	if ((new_w->scale.orientation != cur->scale.orientation)
		&&
		((new_w->scale.scale_width == cur->scale.scale_width) &&
		(new_w->scale.scale_height == cur->scale.scale_height)))
	{
		new_w->scale.scale_width = cur->scale.scale_height;
		new_w->scale.scale_height = cur->scale.scale_width;
	}

	if ((new_w->scale.orientation != cur->scale.orientation)
		&&
		((XtWidth(new_w)== XtWidth(cur)) &&
		(XtHeight(new_w) == XtHeight(cur))))
	{
		XtWidth(new_w) = XtHeight(cur);
		XtHeight(new_w) = XtWidth(cur);
	}

	ValidateInputs(cur, new_w);

	HandleTitle(cur, req, new_w);
	HandleScrollBar(cur, req, new_w);

	/*  Set the font struct for the value display  */

	if (new_w->scale.font_list != cur->scale.font_list)
	{

		if ((cur->scale.font_list == NULL) && 
			(cur->scale.font_struct != NULL))
			XFreeFont(XtDisplay (cur), cur->scale.font_struct);
		
#ifdef OSF_v1_2_4
		if (cur->scale.font_list) XmFontListFree(cur->scale.font_list);
		
#endif /* OSF_v1_2_4 */
		if (new_w->scale.font_list == NULL)
		  new_w->scale.font_list =
		    _XmGetDefaultFontList( (Widget) new_w, XmLABEL_FONTLIST); 
		
#ifdef OSF_v1_2_4
		new_w->scale.font_list =XmFontListCopy(new_w->scale.font_list);

#endif /* OSF_v1_2_4 */
		if (new_w->scale.font_list != NULL)
                        _XmFontListGetDefaultFont(new_w->scale.font_list,
                                           &new_w->scale.font_struct);
		else
		{
			new_w->scale.font_struct =
				XLoadQueryFont(XtDisplay(new_w), "fixed");
			if (new_w->scale.font_struct == NULL)
			    new_w->scale.font_struct =
				XLoadQueryFont(XtDisplay(new_w), "*");
		}

		new_w->scale.last_value = new_w->scale.value - 1;
		XtReleaseGC ((Widget) new_w, new_w->scale.foreground_GC);
		GetForegroundGC (new_w);
		redisplay = True;
	}

	if ((new_w->scale.orientation != cur->scale.orientation)
		&&
		((XtWidth(new_w) == XtWidth(cur)) &&
		(XtHeight(new_w) == XtHeight(cur))))
	{
		XtWidth(new_w) = XtHeight(cur);
		XtHeight(new_w) = XtWidth(cur);
	}

	if (NeedNewSize(cur, new_w))
	{
		/*
		 * Re-calculate the size of the Scale if a new size was not 
		 * specified.
		 */

		if (new_w->core.width == cur->core.width)
			new_w->core.width = 0;

		if (new_w->core.height == cur->core.height)
			new_w->core.height = 0;

		GetScaleSize (new_w, 
			      &(new_w->core.width), &(new_w->core.height));
	}

	if ((new_w->scale.decimal_points != cur->scale.decimal_points) ||
		(new_w->scale.value != cur->scale.value) ||
		(new_w->scale.minimum != cur->scale.minimum) ||
 		(new_w->scale.maximum != cur->scale.maximum) ||
	        (new_w->scale.processing_direction != 
			cur->scale.processing_direction) ||
		(new_w->scale.minimum != cur->scale.minimum) ||
		(new_w->scale.maximum != cur->scale.maximum) ||
		(new_w->scale.show_value != cur->scale.show_value))
	{
		if (new_w->scale.value != cur->scale.value)
			new_w->scale.last_value = new_w->scale.value - 1;
		ShowValue(new_w, new_w->scale.value, new_w->scale.show_value);
	}


	/*  See if the GC needs to be regenerated  */

	if ((new_w->manager.foreground != cur->manager.foreground) ||
		(new_w->core.background_pixel != cur->core.background_pixel))
	{
		XtReleaseGC ((Widget) new_w, new_w->scale.foreground_GC);
		GetForegroundGC (new_w);
		redisplay = True;
	}

	return (redisplay);
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
	XmScaleWidget new_w = (XmScaleWidget) nw ;

	if (!reply->request_mode) /* A No from our parent */
		LayoutScale(new_w, False, NULL);
	*request = *reply;
}


/************************************************************************
 *
 *  Realize
 *	Can't use the standard Manager class realize procedure,
 *      because it creates a window with NW gravity, and the
 *      scale needs a gravity of None.
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

    /*	Make sure height and width are not zero.
    */
   if (!XtWidth(w)) XtWidth(w) = 1 ;
   if (!XtHeight(w)) XtHeight(w) = 1 ;
    
   valueMask |= CWDontPropagate;
   attributes->do_not_propagate_mask =
      ButtonPressMask | ButtonReleaseMask |
      KeyPressMask | KeyReleaseMask | PointerMotionMask;
        
   XtCreateWindow (w, InputOutput, CopyFromParent, valueMask, attributes);
}



/************************************************************************
 *
 *  Destroy
 *	Free the callback lists attached to the scale.
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
            XmScaleWidget sw = (XmScaleWidget) wid ;
   XtRemoveAllCallbacks ((Widget) sw, XmNvalueChangedCallback);
   XtRemoveAllCallbacks ((Widget) sw, XmNdragCallback);

   XtReleaseGC ((Widget) sw, sw->scale.foreground_GC);

   if (sw->scale.font_list == NULL && sw->scale.font_struct != NULL)
      XFreeFont (XtDisplay (sw), sw->scale.font_struct);
#ifdef OSF_v1_2_4

   if (sw->scale.font_list) XmFontListFree(sw->scale.font_list);

#endif /* OSF_v1_2_4 */
#ifdef CDE_VISUAL  /* sliding_mode */
    FreeInstanceExt( wid, GetInstanceExt( wid)) ;
#endif
}




/************************************************************************
 *
 *  GeometryManager
 *	Accept everything except change in position.
 *
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
GeometryManager(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{

   if (IsQueryOnly(request)) return XtGeometryYes;

   if (IsWidth(request)) w->core.width = request->width;
   if (IsHeight(request)) w->core.height = request->height;
   if (IsBorder(request)) w->core.border_width = request->border_width;

   LayoutScale ((XmScaleWidget) XtParent(w), True, w);

   return XtGeometryYes;
}

static Dimension 
#ifdef _NO_PROTO
MaxLabelWidth( sw )
        XmScaleWidget sw ;
#else
MaxLabelWidth(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	register int i;
	register Widget c;
	register Dimension tmp;
	Dimension max = 0;

	for ( i = 2; i < sw->composite.num_children; i++)
	{
		c = sw->composite.children[i];
		tmp = XtWidth(c) + (2 * XtBorderWidth(c));
		if (tmp > max)
			max = tmp;
	}

	return (max);
}

static Dimension 
#ifdef _NO_PROTO
MaxLabelHeight( sw )
        XmScaleWidget sw ;
#else
MaxLabelHeight(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	register int i;
	register Widget c;
	register Dimension max = 0;
	register Dimension tmp;

	for ( i = 2; i < sw->composite.num_children; i++)
	{
		c = sw->composite.children[i];
		tmp = XtHeight(c) + (2 * XtBorderWidth(c));
		if (tmp > max)
			max = tmp;
	}

	return (max);
}

static Dimension 
#ifdef _NO_PROTO
ValueTroughWidth( sw )
        XmScaleWidget sw ;
#else
ValueTroughWidth(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	char buff[15];
	register Dimension tmp_max, tmp_min;
	int direction, ascent, descent;
	XCharStruct overall_return;

	if (sw->scale.show_value)
	{
		if (sw->scale.decimal_points)
#ifdef I18N_MSG
			sprintf(buff, "%d%c", sw->scale.maximum,
                                              nl_langinfo(RADIXCHAR)[0]);
#else
			sprintf(buff, "%d.", sw->scale.maximum);
#endif
		else
			sprintf(buff, "%d", sw->scale.maximum);
		
		XTextExtents(sw->scale.font_struct, buff, strlen(buff),
			&direction, &ascent, &descent, &overall_return);
		
		tmp_max = overall_return.rbearing - overall_return.lbearing;
		
		if (sw->scale.decimal_points)
#ifdef I18N_MSG
			sprintf(buff, "%d%c", sw->scale.minimum,
                                              nl_langinfo(RADIXCHAR)[0]);
#else
			sprintf(buff, "%d.", sw->scale.minimum);
#endif
		else
			sprintf(buff, "%d", sw->scale.minimum);
		
		XTextExtents(sw->scale.font_struct, buff, strlen(buff),
			&direction, &ascent, &descent, &overall_return);
		
		tmp_min = overall_return.rbearing - overall_return.lbearing;

		return(MAX(tmp_min, tmp_max));
	}
	else
		return ((Dimension) 0);
}

static Dimension 
#ifdef _NO_PROTO
ValueTroughHeight( sw )
        XmScaleWidget sw ;
#else
ValueTroughHeight(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	char buff[15];
	register Dimension tmp_max, tmp_min, max;
	int direction, ascent, descent;
	XCharStruct overall_return;

	if (sw->scale.show_value)
	{
		if (sw->scale.decimal_points)
#ifdef I18N_MSG
			sprintf(buff, "%d%c", sw->scale.maximum,
                                              nl_langinfo(RADIXCHAR)[0]);
#else
			sprintf(buff, "%d.", sw->scale.maximum);
#endif
		else
			sprintf(buff, "%d", sw->scale.maximum);
		
		XTextExtents(sw->scale.font_struct, buff, strlen(buff),
			&direction, &ascent, &descent, &overall_return);
		
		tmp_max = ascent + descent;
		
		if (sw->scale.decimal_points)
#ifdef I18N_MSG
			sprintf(buff, "%d%c", sw->scale.minimum,
                                              nl_langinfo(RADIXCHAR)[0]);
#else
			sprintf(buff, "%d.", sw->scale.minimum);
#endif
		else
			sprintf(buff, "%d", sw->scale.minimum);
		
		XTextExtents(sw->scale.font_struct, buff, strlen(buff),
			&direction, &ascent, &descent, &overall_return);
		
		tmp_min = ascent + descent;

		max = ((tmp_max > tmp_min) ? tmp_max : tmp_min);

		return(max);
	}
	else
		return((Dimension) 0);
}

static Dimension 
#ifdef _NO_PROTO
TitleWidth( sw )
        XmScaleWidget sw ;
#else
TitleWidth(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	register Dimension tmp;
	register Widget title_widget = sw->composite.children[0];

	if (XtIsManaged(title_widget))
	{
		tmp = XtWidth(title_widget)
			+ (2 * XtBorderWidth(title_widget));

		if (sw->scale.orientation == XmVERTICAL)
			tmp += (XtHeight(title_widget)
				+ (2 * XtBorderWidth(title_widget))) >> 2;
	}
	else
		tmp = 0;
	
	return(tmp);
}

static Dimension 
#ifdef _NO_PROTO
TitleHeight( sw )
        XmScaleWidget sw ;
#else
TitleHeight(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	register Dimension tmp;
	register Widget title_widget = sw->composite.children[0];

	if (XtIsManaged(title_widget))
	{
		tmp = XtHeight(title_widget)
			+ (2 * XtBorderWidth(title_widget));

		if (sw->scale.orientation == XmHORIZONTAL)
			tmp += (XtHeight(title_widget)
				+ (2 * XtBorderWidth(title_widget))) >> 2;
	}
	else
		tmp = 0;
	
	return(tmp);
}

static Dimension 
#ifdef _NO_PROTO
MajorLeadPad( sw )
        XmScaleWidget sw ;
#else
MajorLeadPad(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	int diff;
	XmScrollBarWidget sb = (XmScrollBarWidget)
		(sw->composite.children[1]);
	int tic, tmp1 = 0, tmp2;

	if (sw->composite.num_children > 3)
	{
		tic = sb->primitive.highlight_thickness
			+ sb->primitive.shadow_thickness
			+ (Dimension) (((float) SLIDER_SIZE / 2.0) + 0.5);
		
		if (sw->scale.orientation == XmHORIZONTAL)
			diff = (TotalWidth(sw->composite.children[2]) / 2)
				- tic;
		else
			diff = (TotalHeight(sw->composite.children[2]) / 2)
				- tic;

		if (diff > 0)
			tmp1 += diff;
		
	}
	else if (sw->composite.num_children == 3)
	{
		/*
		 * This is a potential non-terminal recursion.
		 *
		 * Currently MajorScrollSize has knowledge of this potential
		 * problem and has guards around the call to this procedure.
		 * Modify with care.
		 */

		if (sw->scale.orientation == XmHORIZONTAL)
			diff = TotalWidth(sw->composite.children[2])
				- ScrollWidth(sw);
		else
			diff = TotalHeight(sw->composite.children[2])
				- ScrollHeight(sw);
		
		if (diff > 0)
			tmp1 = diff / 2;
		else
			tmp1 = 0;
	}
	tmp1 -= (sb->primitive.highlight_thickness
		+ sb->primitive.shadow_thickness);

	tmp2 = (int) (( ((int) ValueTroughWidth(sw)) - SLIDER_SIZE) / 2);
	tmp2 -= (sb->primitive.highlight_thickness
		+ sb->primitive.shadow_thickness);

	if (tmp1 < 0)
		tmp1 = 0;
	if (tmp2 < 0)
		tmp2 = 0;

	return(MAX(tmp1, tmp2));
}

static Dimension 
#ifdef _NO_PROTO
MajorTrailPad( sw )
        XmScaleWidget sw ;
#else
MajorTrailPad(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	int diff;
	XmScrollBarWidget sb = (XmScrollBarWidget)
		(sw->composite.children[1]);
	int tic, tmp1 = 0, tmp2;

	if (sw->composite.num_children > 3)
	{
		tic = sb->primitive.highlight_thickness
			+ sb->primitive.shadow_thickness
			+ (Dimension) (((float) SLIDER_SIZE / 2.0) + 0.5);
		
		if (sw->scale.orientation == XmHORIZONTAL)
			diff = (TotalWidth(sw->composite.children
						[sw->composite.num_children - 1]) / 2)
				- tic;
		else
			diff = (TotalHeight(sw->composite.children
						[sw->composite.num_children - 1]) / 2)
				- tic;

		if (diff > 0)
			tmp1 += diff;
	}
	else if (sw->composite.num_children == 3)
	{
		/*
		 * This is a potential non-terminal recursion.
		 *
		 * Currently MajorScrollSize has knowledge of this potential
		 * problem and has guards around the call to this procedure.
		 * Modify with care.
		 */

		if (sw->scale.orientation == XmHORIZONTAL)
			diff = TotalWidth(sw->composite.children[2])
				- ScrollWidth(sw);
		else
			diff = TotalHeight(sw->composite.children[2]) 
				- ScrollHeight(sw);
		
		if (diff > 0)
			tmp1 = diff / 2;
		else
			tmp1 = 0;
	}
	tmp1 -= (sb->primitive.highlight_thickness
		+ sb->primitive.shadow_thickness);

	tmp2 = (int) (( ((int) ValueTroughWidth(sw)) - SLIDER_SIZE) / 2);
	tmp2 -= (sb->primitive.highlight_thickness
		+ sb->primitive.shadow_thickness);

	if (tmp1 < 0)
		tmp1 = 0;
	if (tmp2 < 0)
		tmp2 = 0;

	return(MAX(tmp1, tmp2));
}

static Dimension 
#ifdef _NO_PROTO
ScrollWidth( sw )
        XmScaleWidget sw ;
#else
ScrollWidth(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	Dimension tmp;

	if (sw->scale.orientation == XmVERTICAL)
	{
		if (!(tmp = sw->scale.scale_width))
			tmp = SCALE_DEFAULT_MINOR_SIZE;
		else
			tmp = sw->scale.scale_width;
	}
	else
	{
		if (!(tmp = sw->scale.scale_width))
		{
			if (sw->core.width != 0)
			{
				/* Have to catch an indirect recursion here */
				if (sw->composite.num_children > 3)
					tmp = sw->core.width 
						- (MajorLeadPad(sw) + MajorTrailPad(sw));
				else
				{
					/* Magic to handle excessively wide values */
					int tmp2;
					XmScrollBarWidget sb = (XmScrollBarWidget)
						sw->composite.children[1];

					tmp2 = (int) (((int) ValueTroughWidth(sw))
						- SLIDER_SIZE) / 2;
					tmp2 -= (sb->primitive.highlight_thickness
						+ sb->primitive.shadow_thickness);
					if (tmp2 < 0)
						tmp2 = 0;

					tmp = sw->core.width - (2 * tmp2);
				}
			}
		}

		if (tmp <= 0)
		{
			if (sw->composite.num_children > 2)
			{
				/* Have to catch an indirect recursion here */
				if (sw->composite.num_children > 3)
				{
					Dimension tic, diff;
					XmScrollBarWidget sb = (XmScrollBarWidget)
						sw->composite.children[1];

					tmp = (sw->composite.num_children - 2)
						* MaxLabelWidth(sw);
					
					tic = sb->primitive.highlight_thickness
						+ sb->primitive.shadow_thickness
						+ (Dimension) (((float) SLIDER_SIZE / 2.0) 
							+ 0.5);

					diff = tic - (MaxLabelWidth(sw) / 2);

					if (diff > 0)
						tmp+= (2 * diff);
				}
				else
					tmp = MaxLabelWidth(sw);
			}
		}

		if (tmp <= 0)
			tmp = SCALE_DEFAULT_MAJOR_SIZE;
	}

	return(tmp);
}

static Dimension 
#ifdef _NO_PROTO
ScrollHeight( sw )
        XmScaleWidget sw ;
#else
ScrollHeight(
        XmScaleWidget sw )
#endif /* _NO_PROTO */
{
	Dimension tmp;

	if (sw->scale.orientation == XmHORIZONTAL)
	{
		if (!(tmp = sw->scale.scale_height))
			tmp = SCALE_DEFAULT_MINOR_SIZE;
		else
			tmp = sw->scale.scale_height;
	}
	else
	{
		if (!(tmp = sw->scale.scale_height))
		{
			if (sw->core.height != 0)
			{
				/* Have to catch an indirect recursion here */
				if (sw->composite.num_children > 3)
					tmp = sw->core.height 
						- (MajorLeadPad(sw) + MajorTrailPad(sw));
				else
					tmp = sw->core.height;
			}
			else
				tmp = 0;
		}

		if (tmp <= 0)
		{
			if (sw->composite.num_children > 2)
			{
				/* Have to catch an indirect recursion here */
				if (sw->composite.num_children > 3)
				{
					Dimension tic, diff;
					XmScrollBarWidget sb = (XmScrollBarWidget)
						sw->composite.children[1];

					tmp = (sw->composite.num_children - 2)
						* MaxLabelHeight(sw);
					
					tic = sb->primitive.highlight_thickness
						+ sb->primitive.shadow_thickness
						+ (Dimension) (((float) SLIDER_SIZE / 2.0) 
							+ 0.5);

					diff = tic - (MaxLabelHeight(sw) / 2);

					if (diff > 0)
						tmp+= (2 * diff);
				}
				else
					tmp = MaxLabelHeight(sw);
			}
		}

		if (tmp <= 0)
			tmp = SCALE_DEFAULT_MAJOR_SIZE;
	}

	return(tmp);
}

static void 
#ifdef _NO_PROTO
GetScaleSize( sw, w, h )
        XmScaleWidget sw ;
        Dimension *w ;
        Dimension *h ;
#else
GetScaleSize(
        XmScaleWidget sw,
        Dimension *w,
        Dimension *h )
#endif /* _NO_PROTO */
{
    if (sw->scale.orientation == XmHORIZONTAL)  {
	*w = MAX(TitleWidth(sw),
		 MajorLeadPad(sw) + ScrollWidth(sw) + MajorTrailPad(sw));

	*h = MaxLabelHeight(sw);
	*h += ValueTroughHeight(sw);
	if (sw->scale.show_value) *h += SCALE_VALUE_MARGIN;
	*h += ScrollHeight(sw);
	*h += TitleHeight(sw);
    } else /* sw->scale.orientation == XmVERTICAL */  {
	*w = MaxLabelWidth(sw);
	*w += ValueTroughWidth(sw);
	if (sw->scale.show_value) *w += SCALE_VALUE_MARGIN;
	*w += ScrollWidth(sw);
	*w += TitleWidth(sw);

	*h = MAX(TitleHeight(sw),
		 MajorLeadPad(sw) + ScrollHeight(sw) + MajorTrailPad(sw));
	}

    /* Don't ever desire 0 dimensions */
    if (!*w) *w = 1;

    if (!*h) *h = 1;
}

static void 
#ifdef _NO_PROTO
LayoutHorizontalLabels( sw, scrollBox, labelBox, instigator)
        XmScaleWidget sw ;
        XRectangle *scrollBox ;
        XRectangle *labelBox ;
        Widget instigator ;
#else
LayoutHorizontalLabels(
		       XmScaleWidget sw,
		       XRectangle *scrollBox,
		       XRectangle *labelBox,
		       Widget instigator )
#endif /* _NO_PROTO */
{
	Dimension first_tic, last_tic, tic_interval;
	XmScrollBarWidget sb = (XmScrollBarWidget)
		(sw->composite.children[1]);
	register Widget w;
	register int i;
	register Dimension tmp;
	register Position x, y, y1;

	y1 = labelBox->y + labelBox->height;

	if (sw->composite.num_children > 3)
	{
		tmp = sb->primitive.highlight_thickness
			+ sb->primitive.shadow_thickness
			+ (Dimension) (((float) SLIDER_SIZE / 2.0) + 0.5);
		
		first_tic = tmp + scrollBox->x;
		last_tic = (scrollBox->x + scrollBox->width) - tmp;
		tic_interval = (last_tic - first_tic)
			/ (sw->composite.num_children - 3);

		for (i = 2, tmp = first_tic;
			i < sw->composite.num_children;
			i++, tmp += tic_interval)
		{
			w = sw->composite.children[i];
			x = tmp - (TotalWidth(w) / 2);
			y = y1 - TotalHeight(w);
			if (instigator != w)
			    _XmMoveObject(w, x, y);
			else {
			    w->core.x = x ;
			    w->core.y = y ;
			}
		}
	}
	else if (sw->composite.num_children == 3)
	{
		w = sw->composite.children[2];
		y = y1 - TotalHeight(w);
		tmp = (scrollBox->width - TotalWidth(w)) / 2;
		x = scrollBox->x + tmp;
		if (instigator != w)
		    _XmMoveObject(w, x, y);
		else {
		    w->core.x = x ;
		    w->core.y = y ;
		}
	}
}

static void 
#ifdef _NO_PROTO
LayoutHorizontalScale( sw, optimum_w, optimum_h, instigator )
        XmScaleWidget sw ;
        Dimension optimum_w ;
        Dimension optimum_h ;
        Widget instigator;
#else
LayoutHorizontalScale(
		      XmScaleWidget sw,
#if NeedWidePrototypes
		      int optimum_w,
		      int optimum_h,
#else
		      Dimension optimum_w,
		      Dimension optimum_h,
#endif /* NeedWidePrototypes */
		      Widget instigator)
#endif /* _NO_PROTO */
{
	int diff_w, diff_h, tdiff;
	XRectangle labelBox, valueBox, scrollBox, titleBox;

	diff_h = XtHeight(sw) - optimum_h;
	diff_w = XtWidth(sw) - optimum_w;

	/* Figure out all of the y locations */
	if (diff_h >= 0)
	{
		/* 
		 * We place the title, scrollbar, and value from the right
		 */
		titleBox.height = TitleHeight(sw);
		titleBox.y = XtHeight(sw) - titleBox.height;

		scrollBox.height = ScrollHeight(sw);
		scrollBox.y = titleBox.y - scrollBox.height;

		valueBox.height = ValueTroughWidth(sw);
		valueBox.y = scrollBox.y - valueBox.height;

		/*
		 * Labels are placed all the way to the left, which leaves
		 * the dead space between the value and the labels.  I
		 * don't like it, but it is the 1.0 look.
		 */
		labelBox.y = 0;
		labelBox.height = MaxLabelHeight(sw);
	}
	else if ((tdiff = diff_h + TitleHeight(sw)) >= 0)
	{
		/* Place from the left and let the title get clipped */
		labelBox.y = 0;
		labelBox.height = MaxLabelHeight(sw);

		valueBox.y = labelBox.y + labelBox.height;
		valueBox.height = ValueTroughWidth(sw);

		scrollBox.y = valueBox.y + valueBox.height;
		scrollBox.height = ScrollHeight(sw);

		titleBox.y = scrollBox.y + scrollBox.height;
		titleBox.height = TitleHeight(sw);
	}
	else if ((tdiff += ValueTroughHeight(sw)) >= 0)
	{
		/*
		 * The title is outside the window, and the labels are
		 * allowed overwrite (occlude) the value display region
		 */
		titleBox.height = TitleHeight(sw);
		titleBox.y = XtHeight(sw);

		scrollBox.height = ScrollHeight(sw);
		scrollBox.y = titleBox.y - scrollBox.height;

		valueBox.height = ValueTroughHeight(sw);
		valueBox.y = scrollBox.y - valueBox.height;

		labelBox.y = 0;
		labelBox.height = MaxLabelHeight(sw);
	}
	else if ((tdiff += MaxLabelHeight(sw)) >= 0)
	{
		/*
		 * The title is outside the window, the value trough is 
		 * completely coincident with the label region, and the
		 * labels are clipped from the left
		 */
		titleBox.height = TitleHeight(sw);
		titleBox.y = XtHeight(sw);

		scrollBox.height = ScrollHeight(sw);
		scrollBox.y = titleBox.y - scrollBox.height;

		valueBox.height = ValueTroughHeight(sw);
		valueBox.y = scrollBox.y - valueBox.height;

		labelBox.height = MaxLabelHeight(sw);
		labelBox.y = scrollBox.y - labelBox.height;
	}
	else
	{
		/*
		 * Just center the scrollbar in the available space.
		 */
		titleBox.height = TitleHeight(sw);
		titleBox.y = XtHeight(sw);

		valueBox.height = ValueTroughHeight(sw);
		valueBox.y = titleBox.y;

		labelBox.height = MaxLabelHeight(sw);
		labelBox.y = valueBox.y;

		scrollBox.y = (XtHeight(sw) - ScrollHeight(sw)) / 2;
		scrollBox.height = ScrollHeight(sw);
	}

	if (diff_w >= 0)
	{
		scrollBox.x = MajorLeadPad(sw);
		scrollBox.width = ScrollWidth(sw);
	}
	else
	{
		Dimension sb_min, avail, lp, tp;
		XmScrollBarWidget sb = (XmScrollBarWidget)
			(sw->composite.children[1]);

		sb_min = (2 * sb->primitive.highlight_thickness)
			+ (4 * sb->primitive.shadow_thickness)
			+ SLIDER_SIZE;
		
		lp = MajorLeadPad(sw);
		tp = MajorTrailPad(sw);
		avail = XtWidth(sw) - lp - tp;

		if (avail < sb_min)
		{
			scrollBox.width = sb_min;
			scrollBox.x = (XtWidth(sw) - sb_min) / 2;
		}
		else
		{
			scrollBox.width = avail;
			scrollBox.x = lp;
		}
	}

	if (instigator != sw->composite.children[0])
	    _XmMoveObject(sw->composite.children[0], 0, titleBox.y);
	else {
	    sw->composite.children[0]->core.x = 0 ;
	    sw->composite.children[0]->core.y = titleBox.y ;
	}
	    
	
	if (instigator != sw->composite.children[1])
	    _XmConfigureObject(sw->composite.children[1],
			       scrollBox.x, scrollBox.y,
			       scrollBox.width, scrollBox.height, 0);
	else {
	    sw->composite.children[1]->core.x = scrollBox.x ;
	    sw->composite.children[1]->core.y = scrollBox.y ;
	    sw->composite.children[1]->core.width = scrollBox.width ;
	    sw->composite.children[1]->core.height = scrollBox.height ;
	    sw->composite.children[1]->core.border_width = 0 ;
	}
	{
		int n, value, increment, page, slider_size;
		Arg args[5];

		CalcScrollBarData(sw, &value, &slider_size,
			&increment, &page);

		sw->scale.slider_size = slider_size;

		n = 0;
		XtSetArg (args[n], XmNvalue, value);		n++;
		XtSetArg (args[n], XmNsliderSize, sw->scale.slider_size);	n++;
		XtSetArg (args[n], XmNincrement, increment);		n++;
		XtSetArg (args[n], XmNpageIncrement, page);		n++;

		XtSetValues(sw->composite.children[1], args, n);
	}

	LayoutHorizontalLabels(sw, &scrollBox, &labelBox, instigator);
}

static void 
#ifdef _NO_PROTO
LayoutVerticalLabels( sw, scrollBox, labelBox, instigator )
        XmScaleWidget sw ;
        XRectangle *scrollBox ;
        XRectangle *labelBox ;
        Widget instigator ;
#else
LayoutVerticalLabels(
		     XmScaleWidget sw,
		     XRectangle *scrollBox,
		     XRectangle *labelBox,
		     Widget instigator )
#endif /* _NO_PROTO */
{
	Dimension first_tic, last_tic, tic_interval;
	XmScrollBarWidget sb = (XmScrollBarWidget)
		(sw->composite.children[1]);
	register Widget w;
	register int i;
	register Dimension tmp;
	register Position x, x1, y;

	x1 = labelBox->x + labelBox->width;

	if (sw->composite.num_children > 3)
	{
		tmp = sb->primitive.highlight_thickness
			+ sb->primitive.shadow_thickness
			+ (Dimension) (((float) SLIDER_SIZE / 2.0) + 0.5);
		
		first_tic = tmp + scrollBox->y;
		last_tic = (scrollBox->y + scrollBox->height) - tmp;
		tic_interval = (last_tic - first_tic)
			/ (sw->composite.num_children - 3);

		for (i = 2, tmp = first_tic;
			i < sw->composite.num_children;
			i++, tmp += tic_interval)
		{
			w = sw->composite.children[i];
			y = tmp - (TotalHeight(w) / 2);
			x = x1 - TotalWidth(w);
			if (instigator != w)
			    _XmMoveObject(w, x, y);
			else {
			    w->core.x = x ;
			    w->core.y = y ;
			}
		}
	}
	else if (sw->composite.num_children == 3)
	{
		w = sw->composite.children[2];
		x = x1 - TotalWidth(w);
		tmp = (scrollBox->height - TotalHeight(w)) / 2;
		y = scrollBox->y + tmp;
		if (instigator != w)
		    _XmMoveObject(w, x, y);
		else {
		    w->core.x = x ;
		    w->core.y = y ;
		}
	}
}

static void 
#ifdef _NO_PROTO
LayoutVerticalScale( sw, optimum_w, optimum_h, instigator )
        XmScaleWidget sw ;
        Dimension optimum_w ;
        Dimension optimum_h ;
        Widget instigator ;
#else
LayoutVerticalScale(
        XmScaleWidget sw,
#if NeedWidePrototypes
        int optimum_w,
        int optimum_h,
#else
        Dimension optimum_w,
        Dimension optimum_h,
#endif /* NeedWidePrototypes */
	Widget instigator)
#endif /* _NO_PROTO */
{
	int diff_w, diff_h, tdiff;
	XRectangle labelBox, valueBox, scrollBox, titleBox;

	diff_h = XtHeight(sw) - optimum_h;
	diff_w = XtWidth(sw) - optimum_w;

	/* Figure out all of the x locations */
	if (diff_w >= 0)
	{
		/* 
		 * We place the title, scrollbar, and value from the right
		 */
		titleBox.width = TitleWidth(sw);
		titleBox.x = XtWidth(sw) - titleBox.width;

		scrollBox.width = ScrollWidth(sw);
		scrollBox.x = titleBox.x - scrollBox.width;

		valueBox.width = ValueTroughWidth(sw);
		valueBox.x = scrollBox.x - valueBox.width;

		/*
		 * Labels are placed all the way to the left, which leaves
		 * the dead space between the value and the labels.  I
		 * don't like it, but it is the 1.0 look.
		 */
		labelBox.x = 0;
		labelBox.width = MaxLabelWidth(sw);
	}
	else if ((tdiff = diff_w + TitleWidth(sw)) >= 0)
	{
		/* Place from the left and let the title get clipped */
		labelBox.x = 0;
		labelBox.width = MaxLabelWidth(sw);

		valueBox.x = labelBox.x + labelBox.width;
		valueBox.width = ValueTroughWidth(sw);

		scrollBox.x = valueBox.x + valueBox.width;
		scrollBox.width = ScrollWidth(sw);

		titleBox.x = scrollBox.x + scrollBox.width;
		titleBox.width = TitleWidth(sw);
	}
	else if ((tdiff += ValueTroughWidth(sw)) >= 0)
	{
		/*
		 * The title is outside the window, and the labels are
		 * allowed overwrite (occlude) the value display region
		 */
		titleBox.width = TitleWidth(sw);
		titleBox.x = XtWidth(sw);

		scrollBox.width = ScrollWidth(sw);
		scrollBox.x = titleBox.x - scrollBox.width;

		valueBox.width = ValueTroughWidth(sw);
		valueBox.x = scrollBox.x - valueBox.width;

		labelBox.x = 0;
		labelBox.width = MaxLabelWidth(sw);
	}
	else if ((tdiff += MaxLabelWidth(sw)) >= 0)
	{
		/*
		 * The title is outside the window, the value trough is 
		 * completely coincident with the label region, and the
		 * labels are clipped from the left
		 */
		titleBox.width = TitleWidth(sw);
		titleBox.x = XtWidth(sw);

		scrollBox.width = ScrollWidth(sw);
		scrollBox.x = titleBox.x - scrollBox.width;

		valueBox.width = ValueTroughWidth(sw);
		valueBox.x = scrollBox.x - valueBox.width;

		labelBox.width = MaxLabelWidth(sw);
		labelBox.x = scrollBox.x - labelBox.width;
	}
	else
	{
		/*
		 * Just center the scrollbar in the available space.
		 */
		titleBox.width = TitleWidth(sw);
		titleBox.x = XtWidth(sw);

		valueBox.width = ValueTroughWidth(sw);
		valueBox.x = titleBox.x;

		labelBox.width = MaxLabelWidth(sw);
		labelBox.x = valueBox.x;

		scrollBox.x = (XtWidth(sw) - ScrollWidth(sw)) / 2;
		scrollBox.width = ScrollWidth(sw);
	}

	if (diff_h >= 0)
	{
		scrollBox.y = MajorLeadPad(sw);
		scrollBox.height = ScrollHeight(sw);
	}
	else
	{
		Dimension sb_min, avail, lp, tp;
		XmScrollBarWidget sb = (XmScrollBarWidget)
			(sw->composite.children[1]);

		sb_min = (2 * sb->primitive.highlight_thickness)
			+ (4 * sb->primitive.shadow_thickness)
			+ SLIDER_SIZE;
		
		lp = MajorLeadPad(sw);
		tp = MajorTrailPad(sw);
		avail = XtHeight(sw) - lp - tp;

		if (avail < sb_min)
		{
			scrollBox.height = sb_min;
			scrollBox.y = (XtHeight(sw) - sb_min) / 2;
		}
		else
		{
			scrollBox.height = avail;
			scrollBox.y = lp;
		}
	}

	if (instigator != sw->composite.children[0])
	    _XmMoveObject(sw->composite.children[0], titleBox.x, 0);
	else {
	    sw->composite.children[0]->core.x = titleBox.x ;
	    sw->composite.children[0]->core.y = 0 ;
	}
	    
	if (instigator != sw->composite.children[1])
	    _XmConfigureObject(sw->composite.children[1],
			       scrollBox.x, scrollBox.y,
			       scrollBox.width, scrollBox.height, 0);
	else {
	    sw->composite.children[1]->core.x = scrollBox.x ;
	    sw->composite.children[1]->core.y = scrollBox.y ;
	    sw->composite.children[1]->core.width = scrollBox.width ;
	    sw->composite.children[1]->core.height = scrollBox.height ;
	    sw->composite.children[1]->core.border_width = 0 ;
	}
	 
	{
		int n, value, increment, page, slider_size;
		Arg args[5];

		CalcScrollBarData(sw, &value, &slider_size,
			&increment, &page);

		sw->scale.slider_size = slider_size;

		n = 0;
		XtSetArg (args[n], XmNvalue, value);		n++;
		XtSetArg (args[n], XmNsliderSize, sw->scale.slider_size);	n++;
		XtSetArg (args[n], XmNincrement, increment);		n++;
		XtSetArg (args[n], XmNpageIncrement, page);		n++;

		XtSetValues(sw->composite.children[1], args, n);
	}

	LayoutVerticalLabels(sw, &scrollBox, &labelBox, instigator);
}

static void 
#ifdef _NO_PROTO
LayoutScale( sw, resizable, instigator )
        XmScaleWidget sw ;
        Boolean resizable ;
        Widget instigator ;
#else
LayoutScale(
	    XmScaleWidget sw,
#if NeedWidePrototypes
	    int resizable,
#else
	    Boolean resizable,
#endif /* NeedWidePrototypes */
	    Widget instigator)
#endif /* _NO_PROTO */
{
	Dimension tmp_w, tmp_h, sav_w, sav_h;

	/* Save the current values */
	sav_w = XtWidth(sw);
	sav_h = XtHeight(sw);

	/* Mark the scale as anything goes */
	XtWidth(sw) = 0;
	XtHeight(sw) = 0;

	/* Find out what the best possible answer would be */
	GetScaleSize(sw, &tmp_w, &tmp_h);

	/* Restore the current values */
	XtWidth(sw) = sav_w;
	XtHeight(sw) = sav_h;

	/* Save the current values of desired sizes */
	sav_w = tmp_w ;
	sav_h = tmp_h ;

	/* if resizable, ask the parent for the new size until it says No
	   or Yes (its can only says Almost one time...) */
	if (resizable &&
	    ((tmp_w != XtWidth(sw)) || 
	     (tmp_h != XtHeight(sw)))) {
	    while (XtMakeResizeRequest((Widget) sw, tmp_w, tmp_h, 
				       &tmp_w, &tmp_h)
		   == XtGeometryAlmost) ;
	}
	
	if (sw->scale.orientation == XmHORIZONTAL)
		LayoutHorizontalScale(sw, tmp_w, tmp_h, instigator);
	else /* sw->scale.orientation == XmVERTICAL */
		LayoutVerticalScale(sw, tmp_w, tmp_h, instigator);
}



/*********************************************************************
 *  ChangeManaged
 *     Layout children.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmScaleWidget sw = (XmScaleWidget) wid ;
    Dimension tmp_w, tmp_h;

    if (!XtIsRealized(sw))  {
	/* the first time, only attemps to change size if not specified */
	if ((XtWidth(sw) == 0) || (XtHeight(sw) == 0)) {
	    GetScaleSize(sw, &tmp_w, &tmp_h);
	    while (XtMakeResizeRequest((Widget) sw, tmp_w, tmp_h, 
				       &tmp_w, &tmp_h)
		   == XtGeometryAlmost) ;
	}
	LayoutScale (sw, FALSE, NULL);
    }  else {
	/* Otherwise, Layout and change sizes if needed */
	LayoutScale(sw, TRUE, NULL);
    }
    
    _XmNavigChangeManaged( (Widget) sw);
}


/************************************************************************/
static void 
#ifdef _NO_PROTO
GetValueString(sw, value, buffer)
        XmScaleWidget sw ;
        int value ;
        String buffer;
#else
GetValueString(
        XmScaleWidget sw,
        int value,
        String buffer)
#endif /* _NO_PROTO */
{
    register int i;
    int  diff, dec_point_size;
#ifndef X_LOCALE
    struct lconv *loc_values;
#endif
	
    if (sw->scale.decimal_points > 0) {
      sprintf (buffer,"%.*d", sw->scale.decimal_points, value);

      diff = strlen(buffer) - sw->scale.decimal_points;
#ifndef X_LOCALE
      loc_values = localeconv();
      dec_point_size = strlen(loc_values->decimal_point);
#else
      dec_point_size = 1;
#endif

      for (i = strlen(buffer); i >= diff; i--)
	buffer[i+dec_point_size] = buffer[i];
      
#ifndef X_LOCALE
      for (i=0; i<dec_point_size; i++)
	buffer[diff+i] = loc_values->decimal_point[i];
#else
      buffer[diff] = '.';
#endif
    } else
      sprintf (buffer,"%d", value);
}




/************************************************************************
 *
 *  ShowValue
 *     Display or erase the slider value.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ShowValue( sw, value, show_new )
        XmScaleWidget sw ;
        int value ;
        Boolean show_new ;
#else
ShowValue(
        XmScaleWidget sw,
        int value,
#if NeedWidePrototypes
        int show_new )
#else
        Boolean show_new )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
	int x, y, width, height;
	XCharStruct width_return;
	static char buffer[256];
	int direction, descent;
	XmScrollBarWidget scrollbar;
	Region value_region;
	XRectangle value_rect;

	if (!XtIsRealized(sw))
		return;

	x = sw->scale.show_value_x;
	y = sw->scale.show_value_y;
	width = sw->scale.show_value_width;
	height = sw->scale.show_value_height;

	if (!show_new)  /* turn off the value display */
	{
		if (width) /* We were displaying, so we must clear it */
		{
			XClearArea (XtDisplay (sw), XtWindow (sw), x, y, width, 
				height, FALSE);
			value_rect.x = x;
			value_rect.y = y;
			value_rect.width = width;
			value_rect.height = height;
			value_region = XCreateRegion();
			XUnionRectWithRegion(&value_rect, value_region,
				value_region);
			_XmRedisplayGadgets( (Widget) sw, NULL, value_region);
			XDestroyRegion(value_region);
		}
		sw->scale.show_value_width = 0;
		return;
	}

	/*
	 * Time for the real work.
	 */

	if (width)
	{
		/* Clear the old one */
		value_rect.x = x;
		value_rect.y = y;
		value_rect.width = width;
		value_rect.height = height;
		value_region = XCreateRegion();
		XClearArea (XtDisplay (sw), XtWindow (sw), x, y, width, 
			height, FALSE);
		XUnionRectWithRegion(&value_rect, value_region,
			value_region);
		_XmRedisplayGadgets( (Widget) sw, NULL, value_region);
		XDestroyRegion(value_region);
	}

	/*  Get a string representation of the new value  */

	GetValueString(sw, value, buffer);

	/*  Calculate the x, y, width, and height of the string to display  */

	XTextExtents (sw->scale.font_struct, buffer, strlen(buffer),
		&direction, &height, &descent, &width_return);
	width = width_return.rbearing - width_return.lbearing;
	sw->scale.show_value_width = width;
	sw->scale.show_value_height = height + descent;

	scrollbar = (XmScrollBarWidget) sw->composite.children[1];

	if (sw->scale.orientation == XmHORIZONTAL)
	{
		x = scrollbar->core.x
			+ scrollbar->scrollBar.slider_x 
			- (width_return.rbearing - SLIDER_SIZE) / 2;
		y = scrollbar->core.y - 3;
	}
	else
	{
		x = scrollbar->core.x - width_return.rbearing;
		y = scrollbar->core.y + scrollbar->scrollBar.slider_y 
			+ SLIDER_SIZE + ((height - SLIDER_SIZE) / 2) - 3;
	}

	sw->scale.show_value_x = x + width_return.lbearing;
	sw->scale.show_value_y = y - height + 1;


	/*  Display the string  */

	XDrawImageString (XtDisplay(sw), XtWindow(sw),
	sw->scale.foreground_GC, x, y, buffer, strlen(buffer));

	sw->scale.last_value = value;
}




/*********************************************************************
 *
 * CalcScrollBarData
 * Figure out the scale derived attributes of the scrollbar child.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
CalcScrollBarData( sw, value, slider_size, increment, page )
        XmScaleWidget sw ;
        int *value ;
        int *slider_size ;
        int *increment ;
        int *page ;
#else
CalcScrollBarData(
        XmScaleWidget sw,
        int *value,
        int *slider_size,
        int *increment,
        int *page )
#endif /* _NO_PROTO */
{
	Dimension scrollbar_size;
	float sb_value;
	float tmp;
	int size;

	/*  Adjust the slider size to take SLIDER_SIZE area.    */
	/*  Adjust value to be in the bounds of the scrollbar.  */

	if (sw->scale.orientation == XmHORIZONTAL)
		scrollbar_size = sw->composite.children[1]->core.width;
	else
		scrollbar_size = sw->composite.children[1]->core.height;

	size = scrollbar_size - 2 * (sw->scale.highlight_thickness
			+ sw->manager.shadow_thickness);

	/* prevent divide by zero error and integer rollover */
	if (size <= 0)
		scrollbar_size = 1;
	else
		scrollbar_size -= 2 * (sw->scale.highlight_thickness
			+ sw->manager.shadow_thickness);

	*slider_size = ((SCROLLBAR_MAX - SCROLLBAR_MIN) / scrollbar_size)
		* SLIDER_SIZE;

	/*
	 * Now error check our arithmetic
	 */
	if (*slider_size < 0)
	{
		/* We just overflowed the integer size  */
		/* probably due to ratio between        */
		/* SLIDER_SIZE and scrollbar_size being */
		/* greater than approx. 4.3             */
		*slider_size = SCROLLBAR_MAX;
	}
	else if (*slider_size < 1)
	{
		/* ScrollBar will puke, so don't allow this */
		*slider_size = 1;
	}
	else if (*slider_size < SCROLLBAR_MIN)
	{
		/* or this */
		*slider_size = SCROLLBAR_MIN;
	}
	else if (*slider_size > SCROLLBAR_MAX)
	{
		/* or this either */
		*slider_size = SCROLLBAR_MAX;
	}


	sb_value = (float) (sw->scale.value - sw->scale.minimum) / 
		(float) (sw->scale.maximum - sw->scale.minimum);
        sb_value *= (float) (SCROLLBAR_MAX - SCROLLBAR_MIN - *slider_size);
	
	*value = (int) sb_value;

	if (*value > SCROLLBAR_MAX - *slider_size)
		*value = SCROLLBAR_MAX - *slider_size;
	if (*value < SCROLLBAR_MIN)
		*value = SCROLLBAR_MIN;

	/* Set up the increment processing correctly */

	tmp = (float) (SCROLLBAR_MAX - SCROLLBAR_MIN)
		- (float) *slider_size;

	*increment = (int) 
		((tmp / (float) (sw->scale.maximum - sw->scale.minimum))
		+ 0.5);
	if (*increment < 1)
		*increment = 1;

	*page = (int) (0.5 + ((float) sw->scale.scale_multiple * tmp
                          / (float) (sw->scale.maximum - sw->scale.minimum))) ;
	if (*page < 1)
		*page = 1;

}



/*********************************************************************
 *
 *  ValueChanged
 *	Callback procedure invoked from the scrollbars value being changed.
 *
 *********************************************************************/
static void 
#ifdef _NO_PROTO
ValueChanged( wid, closure, call_data )
        Widget wid ;
        XtPointer closure ;
        XtPointer call_data ;
#else
ValueChanged(
        Widget wid,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
        XmScrollBarWidget sbw = (XmScrollBarWidget) wid ;
	XmScaleWidget sw = (XmScaleWidget) XtParent (sbw);
	XmScrollBarCallbackStruct * scroll_callback;
	XmScaleCallbackStruct scale_callback;
	int value;
	float sb_value;

	scroll_callback = (XmScrollBarCallbackStruct *) call_data;
	value = scroll_callback->value;

	sb_value = (float) value 
            / (float) (SCROLLBAR_MAX - SCROLLBAR_MIN - sw->scale.slider_size) ;
	sb_value = (sb_value * 
			(float) (sw->scale.maximum - sw->scale.minimum))
		+ (float) sw->scale.minimum;

	/* Set up the round off correctly */
	if (sb_value < 0.0)
		sb_value -= 0.5;
	else if (sb_value > 0.0)
		sb_value += 0.5;


	value = (int) sb_value;

	if (sw->scale.show_value)
	{
		sw->scale.last_value = value - 1;
		ShowValue (sw, value, True);
	}

	sw->scale.value = value;
	scale_callback.event = scroll_callback->event;
	scale_callback.reason = scroll_callback->reason;
	scale_callback.value = value;

	if (scale_callback.reason == XmCR_DRAG)
		XtCallCallbackList((Widget) sw, sw->scale.drag_callback,
			&scale_callback);
	else /* value changed and to_top and to_bottom */
	{
		scale_callback.reason = XmCR_VALUE_CHANGED;
		XtCallCallbackList((Widget) sw,
			sw->scale.value_changed_callback, &scale_callback);
	}
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
    GetScaleSize ((XmScaleWidget) widget, &desired->width, &desired->height);

    /* deal with user initial size setting */
    if (!XtIsRealized(widget))  {
	if (XtWidth(widget) != 0) desired->width = XtWidth(widget) ;
	if (XtHeight(widget) != 0) desired->height = XtHeight(widget) ;
    }	    

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}





/************************************************************************
 *
 *	External API functions.
 *
 ************************************************************************/


/************************************************************************
 *
 *  XmScaleSetValue
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmScaleSetValue( w, value )
        Widget w ;
        int value ;
#else
XmScaleSetValue(
        Widget w,
        int value )
#endif /* _NO_PROTO */
{
   Arg args[1];

   XtSetArg (args[0], XmNvalue, value);
   XtSetValues (w, args, 1);
}




/************************************************************************
 *
 *  XmScaleGetValue
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmScaleGetValue( w, value )
        Widget w ;
        int *value ;
#else
XmScaleGetValue(
        Widget w,
        int *value )
#endif /* _NO_PROTO */
{
   XmScaleWidget sw = (XmScaleWidget) w;

   *value = sw->scale.value;
}

/************************************************************************
 *
 *  XmCreateScale
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateScale( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateScale(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
   return (XtCreateWidget(name, xmScaleWidgetClass, parent, arglist, argcount));
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
#ifndef OSF_v1_2_4
  if(    wid->core.sensitive
     &&  wid->core.ancestor_sensitive
#else /* OSF_v1_2_4 */
  if(    XtIsSensitive(wid)
#endif /* OSF_v1_2_4 */
     &&  ((XmManagerWidget) wid)->manager.traversal_on    )
    {   
      XmNavigationType nav_type
	                   = ((XmManagerWidget) wid)->manager.navigation_type ;
      
      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
         ||  (    (nav_type == XmTAB_GROUP)
	      &&  !_XmShellIsExclusive( wid))    )
	{
	  return XmDESCENDANTS_TAB_NAVIGABLE ;
	}
    }
  return XmNOT_NAVIGABLE ;
}
