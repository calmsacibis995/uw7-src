#pragma ident	"@(#)m1.2libs:Xm/Text.c	1.10"
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
#define TEXT

#ifdef OSF_v1_2_4
#include <string.h>
#include <X11/Xos.h>
#include <X11/keysymdef.h>
#endif /* OSF_v1_2_4 */
#include <Xm/TextP.h>
#include <Xm/TextStrSoP.h>
#include <Xm/BaseClassP.h>
#include <Xm/ManagerP.h>
#include "XmI.h"
#include <Xm/CutPaste.h>
#include <Xm/TextF.h>
#include <Xm/ScrolledW.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/VendorSEP.h>
#include <Xm/XmosP.h>
#include <Xm/AtomMgr.h>
#include <Xm/DropSMgr.h>
#include <Xm/Display.h>
#ifndef OSF_v1_2_4
#include <X11/Xos.h>
#include <X11/keysymdef.h>
#include <string.h>
#endif /* OSF_v1_2_4 */


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/* Resolution independence conversion functions */


#ifdef I18N_MSG
#define MESSAGE2	catgets(Xm_catd,MS_Text,MSG_T_2,_XmMsgText_0000)
#else
#define MESSAGE2	_XmMsgText_0000
#endif


/* Memory Management for global line table */
#define INIT_TABLE_SIZE 64
#define TABLE_INCREMENT 1024
#define XmDYNAMIC_BOOL 255


/* Change ChangeVSB() and RedisplayHBar from TextOut.c to non-static functions;
 * they are needed for updating the scroll bars after re-enable redisplay.
 * DisableRedisplay prohibits the visuals of the widget from being updated
 * as the widget's contents are changed.  If the widget is a scrolled widget,
 * this change prohibits the scroll bars from being updated until redisplay
 * is re-enabled.
 */
#ifdef _NO_PROTO
extern void _XmChangeVSB() ;
extern void _XmRedisplayHBar() ;
#else /* _NO_PROTO */
extern void _XmChangeVSB(
		XmTextWidget widget) ;
extern void _XmRedisplayHBar(
		XmTextWidget widget) ;
#endif /* _NO_PROTO */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void NullAddWidget() ;
static void NullRemoveWidget() ;
static XmTextPosition NullRead() ;
static XmTextStatus NullReplace() ;
static XmTextPosition NullScan() ;
static Boolean NullGetSelection() ;
static void NullSetSelection() ;
static void _XmCreateCutBuffers() ;
static Cardinal GetSecResData() ;
static void ClassPartInitialize() ;
static void ClassInitialize() ;
static void AddRedraw() ;
static _XmHighlightRec * FindHighlight() ;
static void DisplayText() ;
static void RedrawChanges() ;
static void DoMove() ;
static void RefigureLines() ;
static void RemoveLines() ;
static void AddLines() ;
static void InitializeLineTable() ;
static void FindHighlightingChanges() ;
static void Redisplay() ;
static void InsertHighlight() ;
static void Initialize() ;
static void InitializeHook() ;
static void Realize() ;
static void Destroy() ;
static void Resize() ;
static void DoExpose() ;
static void GetValuesHook() ;
static Boolean SetValues() ;
static XtGeometryResult QueryGeometry() ;
static int _XmTextGetSubstring() ;
static void _XmTextSetString() ;
static void _XmTextReplace() ;

#else

static void NullAddWidget( 
                        XmTextSource source,
                        XmTextWidget widget) ;
static void NullRemoveWidget( 
                        XmTextSource source,
                        XmTextWidget widget) ;
static XmTextPosition NullRead( 
                        XmTextSource source,
                        XmTextPosition position,
                        XmTextPosition last_position,
                        XmTextBlock block) ;
static XmTextStatus NullReplace( 
                        XmTextWidget widget,
			XEvent *event,
                        XmTextPosition *start,
                        XmTextPosition *end,
                        XmTextBlock block,
#if NeedWidePrototypes
                        int call_callbacks);
#else
    		        Boolean call_callbacks);
#endif /* NeedsWidePrototypes */
static XmTextPosition NullScan( 
                        XmTextSource source,
                        XmTextPosition position,
                        XmTextScanType sType,
                        XmTextScanDirection dir,
                        int n,
#if NeedWidePrototypes
                        int include) ;
#else
                        Boolean include) ;
#endif /* NeedWidePrototypes */
static Boolean NullGetSelection( 
                        XmTextSource source,
                        XmTextPosition *start,
                        XmTextPosition *end) ;
static void NullSetSelection( 
                        XmTextSource source,
                        XmTextPosition start,
                        XmTextPosition end,
                        Time time) ;
static void _XmCreateCutBuffers( 
                        Widget w) ;
static Cardinal GetSecResData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **secResDataRtn) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void ClassInitialize( void ) ;
static void AddRedraw( 
                        XmTextWidget widget,
                        XmTextPosition left,
                        XmTextPosition right) ;
static _XmHighlightRec * FindHighlight( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        XmTextScanDirection dir) ;
static void DisplayText( 
                        XmTextWidget widget,
                        XmTextPosition updateFrom,
                        XmTextPosition updateTo) ;
static void RedrawChanges( 
                        XmTextWidget widget) ;
static void DoMove( 
                        XmTextWidget widget,
                        int startcopy,
                        int endcopy,
                        int destcopy) ;
static void RefigureLines( 
                        XmTextWidget widget) ;
static void RemoveLines( 
                        XmTextWidget widget,
                        int num_lines,
                        unsigned int cur_index) ;
static void AddLines( 
                        XmTextWidget widget,
                        XmTextLineTable temp_table,
                        unsigned int tmp_index,
                        unsigned int current_index) ;
static void InitializeLineTable( 
                        XmTextWidget widget,
                        register int size) ;
static void FindHighlightingChanges( 
                        XmTextWidget widget) ;
static void Redisplay( 
                        XmTextWidget widget) ;
static void InsertHighlight( 
                        XmTextWidget widget,
                        XmTextPosition position,
                        XmHighlightMode mode) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void InitializeHook( 
                        Widget wid,
                        ArgList args,
                        Cardinal *num_args_ptr) ;
static void Realize( 
                        Widget w,
                        XtValueMask *valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget w) ;
static void Resize( 
                        Widget w) ;
static void DoExpose( 
                        Widget w,
                        XEvent *event,
                        Region region) ;
static void GetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args_ptr) ;
static Boolean SetValues( 
                        Widget oldw,
                        Widget reqw,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static XtGeometryResult QueryGeometry( 
                        Widget w,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *reply) ;
static int _XmTextGetSubstring( 
                        Widget widget,
                        XmTextPosition start,
                        int num_chars,
                        int buf_size,
                        char *buffer,
#if NeedWidePrototypes
                        int want_wchar) ;
#else
                        Boolean want_wchar) ;
#endif /* NeedWidePrototypes */
static void _XmTextSetString( 
                        Widget widget,
                        char *value) ;
static void _XmTextReplace( 
                        Widget widget,
                        XmTextPosition frompos,
                        XmTextPosition topos,
                        char *value,
#if NeedWidePrototypes
                        int is_wchar) ;
#else
                        Boolean is_wchar) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*
 * For resource list management. 
 */

static XmTextSourceRec nullsource;
static XmTextSource nullsourceptr = &nullsource;

#define _XmTextEventBindings1	_XmTextIn_XmTextEventBindings1
#define _XmTextEventBindings2	_XmTextIn_XmTextEventBindings2
#define _XmTextEventBindings3	_XmTextIn_XmTextEventBindings3
#ifdef CDE_INTEGRATE
_XmConst char _XmTextIn_XmTextEventBindings_CDE[] = "\
~c ~s ~m ~a <Btn1Down>:process-press(grab-focus,process-bdrag)\n\
c ~s ~m ~a <Btn1Down>:process-press(move-destination,process-bdrag)\n\
~c s ~m ~a <Btn1Down>:process-press(extend-start,process-bdrag)\n\
~c ~m ~a <Btn1Motion>:extend-adjust()\n\
~c ~m ~a <Btn1Up>:extend-end()\n\
c ~s ~m a <Btn1Down>:process-bdrag()\n\
c ~s ~m a <Btn1Motion>:secondary-adjust()\n\
c ~s ~m a <Btn1Up>:copy-to()\n\
~c s ~m a <Btn1Down>:process-bdrag()\n\
~c s ~m a <Btn1Motion>:secondary-adjust()\n\
~c s ~m a <Btn1Up>:move-to()";
_XmConst char _XmTextIn_XmTextEventBindings_CDEBtn2[] = "\
<Btn2Down>:extend-start()\n\
<Btn2Motion>:extend-adjust()\n\
<Btn2Up>:extend-end()";

#define _XmTextEventBindingsCDE _XmTextIn_XmTextEventBindings_CDE
#define _XmTextEventBindingsCDEBtn2 _XmTextIn_XmTextEventBindings_CDEBtn2
#endif /* CDE_INTEGRATE */

#define EraseInsertionPoint(widget)\
{\
    (*widget->text.output->DrawInsertionPoint)(widget,\
					 widget->text.cursor_position, off);\
}

#define TextDrawInsertionPoint(widget)\
{\
    (*widget->text.output->DrawInsertionPoint)(widget,\
					 widget->text.cursor_position, on);\
}

static XtResource resources[] =
{
    {
      XmNsource, XmCSource, XmRPointer, sizeof(XtPointer),
      XtOffsetOf( struct _XmTextRec, text.source),
      XmRPointer, (XtPointer) &nullsourceptr
    },

    {
      XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.activate_callback),
      XmRCallback, NULL
    },

    {
      XmNfocusCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.focus_callback),
      XmRCallback, NULL
    },

    {
      XmNlosingFocusCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.losing_focus_callback),
      XmRCallback, NULL
    },

    {
      XmNvalueChangedCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.value_changed_callback),
      XmRCallback, NULL
    },

    {
      XmNmodifyVerifyCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.modify_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNmodifyVerifyCallbackWcs, XmCCallback, XmRCallback, 
      sizeof(XtCallbackList), 
      XtOffsetOf( struct _XmTextRec, text.wcs_modify_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNmotionVerifyCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.motion_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNgainPrimaryCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.gain_primary_callback),
      XmRCallback, NULL
    },

    {
      XmNlosePrimaryCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextRec, text.lose_primary_callback),
      XmRCallback, NULL
    },

    {
      XmNvalue, XmCValue, XmRString, sizeof(String),
      XtOffsetOf( struct _XmTextRec, text.value),
      XmRString, ""
    },

    {
      XmNvalueWcs, XmCValueWcs, XmRValueWcs, sizeof(wchar_t*),
      XtOffsetOf( struct _XmTextRec, text.wc_value),
      XmRString, NULL
    },

    {
      XmNmaxLength, XmCMaxLength, XmRInt, sizeof(int),
      XtOffsetOf( struct _XmTextRec, text.max_length),
#ifndef OSF_v1_2_4
      XmRImmediate, (XtPointer) MAXINT
#else /* OSF_v1_2_4 */
      XmRImmediate, (XtPointer) INT_MAX
#endif /* OSF_v1_2_4 */
    },

    {
      XmNmarginHeight, XmCMarginHeight, XmRVerticalDimension, sizeof(Dimension),
      XtOffsetOf( struct _XmTextRec, text.margin_height),
      XmRImmediate, (XtPointer) 5
    },

    {
      XmNmarginWidth, XmCMarginWidth, XmRHorizontalDimension, sizeof(Dimension),
      XtOffsetOf( struct _XmTextRec, text.margin_width),
      XmRImmediate, (XtPointer) 5
    },

    {
      XmNoutputCreate, XmCOutputCreate,  XmRFunction, sizeof(OutputCreateProc),
      XtOffsetOf( struct _XmTextRec, text.output_create),
      XmRFunction, (XtPointer) NULL
    },

    {
      XmNinputCreate, XmCInputCreate, XmRFunction, sizeof(InputCreateProc),
      XtOffsetOf( struct _XmTextRec, text.input_create),
      XmRFunction, (XtPointer) NULL
    },

    {
      XmNtopCharacter, XmCTopCharacter, XmRTextPosition, sizeof(XmTextPosition),
      XtOffsetOf( struct _XmTextRec, text.top_character),
      XmRImmediate, (XtPointer) 0
    },

    {
      XmNcursorPosition, XmCCursorPosition, XmRTextPosition,
      sizeof (XmTextPosition),
      XtOffsetOf( struct _XmTextRec, text.cursor_position),
      XmRImmediate, (XtPointer) 0
    },

    {
      XmNeditMode, XmCEditMode, XmREditMode, sizeof(int),
      XtOffsetOf( struct _XmTextRec, text.edit_mode),
      XmRImmediate, (XtPointer) XmSINGLE_LINE_EDIT
    },

    {
      XmNautoShowCursorPosition, XmCAutoShowCursorPosition, XmRBoolean,
      sizeof(Boolean),
      XtOffsetOf( struct _XmTextRec, text.auto_show_cursor_position),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNeditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _XmTextRec, text.editable),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNverifyBell, XmCVerifyBell, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _XmTextRec, text.verify_bell),
      XmRImmediate, (XtPointer) XmDYNAMIC_BOOL
    },

   {
     XmNnavigationType, XmCNavigationType, XmRNavigationType,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
     XmRImmediate, (XtPointer) XmTAB_GROUP
   },

};

/* Definition for resources that need special processing in get values */

static XmSyntheticResource get_resources[] =
{
   {
     XmNmarginWidth,
     sizeof(Dimension),
     XtOffsetOf( struct _XmTextRec, text.margin_width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels
   },

   {
     XmNmarginHeight,
     sizeof(Dimension),
     XtOffsetOf( struct _XmTextRec, text.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels
   },
};

static XmBaseClassExtRec       textBaseClassExtRec = {
    NULL,                                     /* Next extension       */
    NULLQUARK,                                /* record type XmQmotif */
    XmBaseClassExtVersion,                    /* version              */
    sizeof(XmBaseClassExtRec),                /* size                 */
    XmInheritInitializePrehook,               /* initialize prehook   */
    XmInheritSetValuesPrehook,                /* set_values prehook   */
    XmInheritInitializePosthook,              /* initialize posthook  */
    XmInheritSetValuesPosthook,               /* set_values posthook  */
    XmInheritClass,   		      	      /* secondary class      */
    XmInheritSecObjectCreate,                 /* creation proc        */
    GetSecResData,                	      /* getSecResData 	      */
    {0},                                      /* fast subclass        */
    XmInheritGetValuesPrehook,                /* get_values prehook   */
    XmInheritGetValuesPosthook,               /* get_values posthook  */
    XmInheritClassPartInitPrehook,            /* classPartInitPrehook */
    XmInheritClassPartInitPosthook,           /* classPartInitPosthook*/
    NULL,                                     /* ext_resources        */
    NULL,                                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    XmInheritWidgetNavigable,                 /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
    NULL,		      		      /* wrapperData 	      */
};

XmPrimitiveClassExtRec _XmTextPrimClassExtRec = {
    NULL,
    NULLQUARK,
    XmPrimitiveClassExtVersion,
    sizeof(XmPrimitiveClassExtRec),
    _XmTextGetBaselines,                  /* widget_baseline */
    _XmTextGetDisplayRect,               /* widget_display_rect */
    _XmTextMarginsProc,			 /* get/set widget margins */
};

externaldef(xmtextclassrec) XmTextClassRec xmTextClassRec = {
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &xmPrimitiveClassRec,
    /* class_name	  */	"XmText",
    /* widget_size	  */	sizeof(XmTextRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_initiali*/	ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */	InitializeHook,
    /* realize		  */	Realize,
    /* actions		  */    NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	XtExposeCompressMaximal,
    /* compress_enterleave*/	TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	Destroy,
    /* resize		  */	Resize,
    /* expose		  */	DoExpose,
    /* set_values	  */	SetValues,
    /* set_values_hook	  */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,
    /* get_values_hook    */	GetValuesHook,
    /* accept_focus	  */	NULL,
    /* version		  */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table		  */	NULL,
    /* query_geometry     */    QueryGeometry,
    /* display accel	  */	NULL,
    /* extension	  */	(XtPointer)&textBaseClassExtRec,
  },

   {				/* primitive_class fields 	*/
      XmInheritBorderHighlight, /* Primitive border_highlight   */
      XmInheritBorderUnhighlight,/* Primitive border_unhighlight */
      NULL,         		/* translations                 */
      NULL,         		/* arm_and_activate           	*/
      get_resources,	    	/* get resources 	        */
      XtNumber(get_resources),	/* num get_resources            */
      (XtPointer) &_XmTextPrimClassExtRec,  /* extension                    */
   },

   {				/* text class fields */
      NULL,             	/* extension         */
   }
};

externaldef(xmtextwidgetclass) WidgetClass xmTextWidgetClass =
					 (WidgetClass) &xmTextClassRec;

/****************************************************************
 *
 * Definitions for the null source.
 *
 ****************************************************************/

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
NullAddWidget( source, widget )
        XmTextSource source ;
        XmTextWidget widget ;
#else
NullAddWidget(
        XmTextSource source,
        XmTextWidget widget )
#endif /* _NO_PROTO */
{}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
NullRemoveWidget( source, widget )
        XmTextSource source ;
        XmTextWidget widget ;
#else
NullRemoveWidget(
        XmTextSource source,
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
}

/* ARGSUSED */
static XmTextPosition 
#ifdef _NO_PROTO
NullRead( source, position, last_position, block )
        XmTextSource source ;
        XmTextPosition position ;
        XmTextPosition last_position ;
        XmTextBlock block ;
#else
NullRead(
        XmTextSource source,
        XmTextPosition position,
        XmTextPosition last_position,
        XmTextBlock block )
#endif /* _NO_PROTO */
{
    block->ptr = NULL;
    block->length = 0;
    block->format = XmFMT_8_BIT;

    return 0;
}

/* ARGSUSED */
static XmTextStatus 
#ifdef _NO_PROTO
NullReplace( widget, event, start, end, block, call_callbacks)
        XmTextWidget widget ;
        XEvent * event;
        XmTextPosition *start ;
        XmTextPosition *end ;
        XmTextBlock block ;
        Boolean call_callbacks ;
#else
NullReplace(
        XmTextWidget widget,
        XEvent * event,
        XmTextPosition *start,
        XmTextPosition *end,
        XmTextBlock block,
#if NeedWidePrototypes
        int call_callbacks )
#else
        Boolean call_callbacks )
#endif
#endif /* _NO_PROTO */
{
    return EditError;
}

/* ARGSUSED */
static XmTextPosition 
#ifdef _NO_PROTO
NullScan( source, position, sType, dir, n, include )
        XmTextSource source ;
        XmTextPosition position ;
        XmTextScanType sType ;
        XmTextScanDirection dir ;
        int n ;
        Boolean include ;
#else
NullScan(
        XmTextSource source,
        XmTextPosition position,
        XmTextScanType sType,
        XmTextScanDirection dir,
        int n,
#if NeedWidePrototypes
        int include )
#else
        Boolean include )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    return 0;
}
 
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
NullGetSelection( source, start, end )
        XmTextSource source ;
        XmTextPosition *start ;
        XmTextPosition *end ;
#else
NullGetSelection(
        XmTextSource source,
        XmTextPosition *start,
        XmTextPosition *end )
#endif /* _NO_PROTO */
{
    return FALSE;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
NullSetSelection( source, start, end, time )
        XmTextSource source ;
        XmTextPosition start ;
        XmTextPosition end ;
	Time time;
#else
NullSetSelection(
        XmTextSource source,
        XmTextPosition start,
        XmTextPosition end,
	Time time)
#endif /* _NO_PROTO */
{
}

static void 
#ifdef _NO_PROTO
_XmCreateCutBuffers( w )
        Widget w ;
#else
_XmCreateCutBuffers(
        Widget w )
#endif /* _NO_PROTO */
{
   static XContext context = (XContext)NULL;
   static char * tmp = NULL;
   Display *dpy = XtDisplay(w);
   Screen *screen = XtScreen(w);

   if (context == (XContext)NULL) context = XUniqueContext();

   if (XFindContext(dpy, (Window)screen, context, &tmp)) {
      XmTextContextData ctx_data;
      Widget xm_display = (Widget) XmGetXmDisplay(dpy);

      ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

      ctx_data->screen = screen;
      ctx_data->context = context;
      ctx_data->type = '\0';

      XtAddCallback(xm_display, XmNdestroyCallback,
                    (XtCallbackProc) _XmTextFreeContextData,
		    (XtPointer) ctx_data);

      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER0,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER1,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER2,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER3,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER4,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER5,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER6,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );
      XChangeProperty(dpy, RootWindowOfScreen(screen), XA_CUT_BUFFER7,
		      XA_STRING, 8, PropModeAppend, NULL, 0 );

      XSaveContext(dpy, (Window)screen, context, tmp);
   }
}

/****************************************************************
 *
 * Private definitions.
 *
 ****************************************************************/
/************************************************************************
 *
 *  GetSecResData
 *
 ************************************************************************/
/* ARGSUSED */
static Cardinal
#ifdef _NO_PROTO
GetSecResData( w_class, secResDataRtn )
        WidgetClass w_class ;
        XmSecondaryResourceData **secResDataRtn ;
#else
GetSecResData(
        WidgetClass w_class,
        XmSecondaryResourceData **secResDataRtn )
#endif /* _NO_PROTO */
{
   XmSecondaryResourceData               *secResDataPtr;

   secResDataPtr = (XmSecondaryResourceData *) XtMalloc(
			sizeof(XmSecondaryResourceData) * 2);

   _XmTextInputGetSecResData( &secResDataPtr[0] );
   _XmTextOutputGetSecResData( &secResDataPtr[1] );
   *secResDataRtn = secResDataPtr;

   return 2;
}



/****************************************************************
 *
 * ClassPartInitialize
 *     Set up the fast subclassing for the widget. Set up merged
 *     Translation table.
 *
 ****************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   XmTextWidgetClass twc = (XmTextWidgetClass) wc;
   WidgetClass super = twc->core_class.superclass;
   XmPrimitiveClassExt *wcePtr, *scePtr;
   char * event_bindings;

   wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
   scePtr = _XmGetPrimitiveClassExtPtr(super, NULLQUARK);

   if ((*wcePtr)->widget_baseline == XmInheritBaselineProc)
      (*wcePtr)->widget_baseline = (*scePtr)->widget_baseline;
 
   if ((*wcePtr)->widget_display_rect == XmInheritDisplayRectProc)
      (*wcePtr)->widget_display_rect  = (*scePtr)->widget_display_rect;

   event_bindings = (char *)XtMalloc(strlen(_XmTextEventBindings1) +
				     strlen(_XmTextEventBindings2) +
				     strlen(_XmTextEventBindings3) + 1);
   strcpy(event_bindings, _XmTextEventBindings1);
   strcat(event_bindings, _XmTextEventBindings2);
   strcat(event_bindings, _XmTextEventBindings3);
   xmTextClassRec.core_class.tm_table = 
     (String) XtParseTranslationTable(event_bindings);

   XtFree(event_bindings);

   _XmFastSubclassInit (wc, XmTEXT_BIT);
}

/****************************************************************
 *
 * ClassInitialize
 *   
 *
 ****************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
    xmTextClassRec.core_class.actions =
                                 (XtActionList)_XmdefaultTextActionsTable;
    xmTextClassRec.core_class.num_actions = _XmdefaultTextActionsTableSize;

    nullsource.AddWidget = NullAddWidget;
    nullsource.RemoveWidget = NullRemoveWidget;
    nullsource.ReadSource = NullRead;
    nullsource.Replace = NullReplace;
    nullsource.Scan = NullScan;
    nullsource.GetSelection = NullGetSelection;
    nullsource.SetSelection = NullSetSelection;

    textBaseClassExtRec.record_type = XmQmotif;
}


/*
 * Mark the given range of text to be redrawn.
 */
static void 
#ifdef _NO_PROTO
AddRedraw( widget, left, right )
        XmTextWidget widget ;
        XmTextPosition left ;
        XmTextPosition right ;
#else
AddRedraw(
        XmTextWidget widget,
        XmTextPosition left,
        XmTextPosition right )
#endif /* _NO_PROTO */
{
    RangeRec *r = widget->text.repaint.range;
    int i;

    if (left == widget->text.last_position &&
        widget->text.output->data->number_lines >= 1)
        left = (*widget->text.source->Scan)(widget->text.source, left,
                                       XmSELECT_POSITION, XmsdLeft, 1, TRUE);

    if (left < right) {
       for (i = 0; i < widget->text.repaint.number; i++) {
            if (left <= r[i].to && right >= r[i].from) {
                r[i].from = MIN(left, r[i].from);
                r[i].to = MAX(right, r[i].to);
                return;
            }
        }
        if (widget->text.repaint.number >= widget->text.repaint.maximum) {
            widget->text.repaint.maximum = widget->text.repaint.number + 1;
	    widget->text.repaint.range = r = (RangeRec *)
          XtRealloc((char *)r, widget->text.repaint.maximum * sizeof(RangeRec));
        }
        r[widget->text.repaint.number].from = left;
        r[widget->text.repaint.number].to = right;
        widget->text.repaint.number++;
    }
}

/*
 * Find the highlight record corresponding to the given position.  Returns a
 * pointer to the record.  The third argument indicates whether we are probing
 * the left or right edge of a highlighting range.
 */
static _XmHighlightRec * 
#ifdef _NO_PROTO
FindHighlight( widget, position, dir )
        XmTextWidget widget ;
        XmTextPosition position ;
        XmTextScanDirection dir ;
#else
FindHighlight(
        XmTextWidget widget,
        XmTextPosition position,
        XmTextScanDirection dir )
#endif /* _NO_PROTO */
{
    _XmHighlightRec *l = widget->text.highlight.list;
    int i;
    if (dir == XmsdLeft) {
	for (i=widget->text.highlight.number - 1 ; i>=0 ; i--)
	    if (position >= l[i].position) {
		l = l + i;
                break;
            }
    } else {
	for (i=widget->text.highlight.number - 1 ; i>=0 ; i--)
	    if (position > l[i].position) {
		l = l + i;
                break;
            }
    }
    return(l);
}

/*
 * Redraw the specified range of text.  Should only be called by
 * RedrawChanges(), below (as well as calling itself recursively).
 */
static void 
#ifdef _NO_PROTO
DisplayText( widget, updateFrom, updateTo )
        XmTextWidget widget ;
        XmTextPosition updateFrom ;
        XmTextPosition updateTo ;
#else
DisplayText(
        XmTextWidget widget,
        XmTextPosition updateFrom,
        XmTextPosition updateTo )
#endif /* _NO_PROTO */
{
    LineNum i;
    XmTextPosition nextstart;
    _XmHighlightRec *l1, *l2;

    if (updateFrom < widget->text.top_character)
	updateFrom = widget->text.top_character;
    if (updateTo > widget->text.bottom_position)
	updateTo = widget->text.bottom_position;
    if (updateFrom > updateTo) return;

    l1 = FindHighlight(widget, updateFrom, XmsdLeft);
    l2 = FindHighlight(widget, updateTo, XmsdRight);
    if (l1 != l2) {
	DisplayText(widget, updateFrom, l2->position);
	updateFrom = l2->position;
    }

    /*
     * Once we get here, we need to paint all of the text from updateFrom to
     * updateTo with current highlightmode.  We have to break this into
     * separate lines, and then call the output routine for each line.
     */

    for (i = _XmTextPosToLine(widget, updateFrom);
	 updateFrom <= updateTo && i < widget->text.number_lines;
	 i++) {
	nextstart = widget->text.line[i+1].start;
	(*widget->text.output->Draw)(widget, i, updateFrom,
			     MIN(updateTo, nextstart), l2->mode);
	updateFrom = nextstart;
    }
}

/*
 * Redraw the changed areas of the text.  This should only be called by
 * Redisplay(), below. 
 */
static void 
#ifdef _NO_PROTO
RedrawChanges( widget )
        XmTextWidget widget ;
#else
RedrawChanges(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    RangeRec *r = widget->text.repaint.range;
    XmTextPosition updateFrom, updateTo;
    int w, i;

    EraseInsertionPoint(widget);

    while (widget->text.repaint.number != 0) {
	updateFrom = r[0].from;
	w = 0;
	for (i=1 ; i<widget->text.repaint.number ; i++) {
	    if (r[i].from < updateFrom) {
		updateFrom = r[i].from;
		w = i;
	    }
	}
	updateTo = r[w].to;
	widget->text.repaint.number--;
	r[w].from = r[widget->text.repaint.number].from;
	r[w].to = r[widget->text.repaint.number].to;
	for (i=widget->text.repaint.number-1 ; i>=0 ; i--) {
	    while (i < widget->text.repaint.number) {
		updateTo = MAX(r[i].to, updateTo);
		widget->text.repaint.number--;
		r[i].from = r[widget->text.repaint.number].from;
		r[i].to = r[widget->text.repaint.number].to;
	    }
	}
	DisplayText(widget, updateFrom, updateTo);
    }
    if (widget->text.first_position == widget->text.last_position) {
	(*widget->text.output->Draw)(widget, (LineNum) 0,
	       			     widget->text.first_position,
		 		     widget->text.last_position,
			             XmHIGHLIGHT_NORMAL);
    }
    TextDrawInsertionPoint(widget);
}
    
static void 
#ifdef _NO_PROTO
DoMove( widget, startcopy, endcopy, destcopy )
        XmTextWidget widget ;
        int startcopy ;
        int endcopy ;
        int destcopy ;
#else
DoMove(
        XmTextWidget widget,
        int startcopy,
        int endcopy,
        int destcopy )
#endif /* _NO_PROTO */
{
    Line line = widget->text.line;
    LineNum i;

    EraseInsertionPoint(widget);
    if (widget->text.disable_depth == 0 &&
	(*widget->text.output->MoveLines)(widget, (LineNum) startcopy,
				     (LineNum) endcopy, (LineNum) destcopy)){
	_XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
        TextDrawInsertionPoint(widget);
	return;
    }
    for (i=destcopy ; i <= destcopy + endcopy - startcopy ; i++)
	AddRedraw(widget, line[i].start, line[i+1].start);
    _XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
    TextDrawInsertionPoint(widget);
}


/*
 * Find the starting position of the line that is delta lines away from the
 * line starting with position start.
 */
XmTextPosition 
#ifdef _NO_PROTO
_XmTextFindScroll( widget, start, delta )
        XmTextWidget widget ;
        XmTextPosition start ;
        int delta ;
#else
_XmTextFindScroll(
        XmTextWidget widget,
        XmTextPosition start,
        int delta )
#endif /* _NO_PROTO */
{
    register XmTextLineTable line_table;
    register unsigned int t_index;
    register unsigned int max_index = 0;

    line_table = widget->text.line_table;
    t_index = widget->text.table_index;

    max_index = widget->text.total_lines - 1;

   /* look forward to find the current record */
    if (line_table[t_index].start_pos < (unsigned int) start) {
       while (t_index <= max_index &&
	      line_table[t_index].start_pos < (unsigned int) start) t_index++;
    } else
  /* look backward to find the current record */
       while (t_index && 
	      line_table[t_index].start_pos > (unsigned int) start) t_index--;

    if (delta > 0) {
        t_index += delta;
	if (t_index > widget->text.total_lines - 1)
	   t_index = widget->text.total_lines - 1;
    } else {
	if (t_index > -delta)
	   t_index += delta;
	else
	   t_index = 0;
    }

    start = line_table[t_index].start_pos;

    widget->text.table_index = t_index;

    return start;
}

/* 
 * Refigure the line breaks in this widget.
 */
static void 
#ifdef _NO_PROTO
RefigureLines( widget )
        XmTextWidget widget ;
#else
RefigureLines(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    Line line = widget->text.line;
    LineNum i, j;
    static Line oldline = NULL;
    static Cardinal oldMaxLines = 0;
    static XmTextPosition tell_output_force_display = -1;
    int oldNumLines = widget->text.number_lines;
    int startcopy, endcopy, destcopy, lastcopy; /* %%% Document! */

    if (widget->text.in_refigure_lines || !widget->text.needs_refigure_lines)
        return;
    widget->text.in_refigure_lines = TRUE;
    widget->text.needs_refigure_lines = FALSE;
    if (XtIsRealized((Widget)widget)) EraseInsertionPoint(widget);
    if (oldMaxLines < oldNumLines + 2) {
	oldMaxLines = oldNumLines + 2;
	oldline = (Line) XtRealloc((char *)oldline,
				   oldMaxLines * sizeof(LineRec));
    }
    memcpy((void *) oldline, (void *) line,
	   (size_t) (oldNumLines + 1) * sizeof(LineRec));
    

    if (widget->text.pending_scroll != 0) {
	widget->text.new_top = _XmTextFindScroll(widget, widget->text.new_top,
					         widget->text.pending_scroll);
	widget->text.pending_scroll = 0;
    }
    if (widget->text.new_top < widget->text.first_position)
        widget->text.new_top = widget->text.first_position;
    line[0].start = widget->text.top_character = widget->text.new_top;
    line[0].past_end = FALSE;
    line[0].extra = NULL;

    widget->text.number_lines = 0;
    j = 0;
    startcopy = endcopy = lastcopy = destcopy = -99;
    for (i = 0 ; i == 0 || !line[i-1].past_end ; i++) {
	if (i+2 > widget->text.maximum_lines) {
	    widget->text.maximum_lines = i+2;
	    line = widget->text.line = (Line)
	       XtRealloc((char *)line, 
			widget->text.maximum_lines * sizeof(LineRec));
	}
	while (j < oldNumLines && oldline[j].start < line[i].start)
	    j++;
	if (j < oldNumLines && oldline[j].start >= oldline[j+1].start)
	    j = oldNumLines;
	if (j >= oldNumLines)
	    oldline[j].start = -1; /* Make comparisons fail. */
	if (line[i].start >= widget->text.forget_past ||
	      line[i].start != oldline[j].start ||
	      oldline[j].changed ||
	      oldline[j+1].changed) {
	    line[i].past_end =
		!(*widget->text.output->MeasureLine)(widget, i, line[i].start,
					    &line[i+1].start, &line[i].extra);
            line[i+1].extra = NULL;
	    if (!line[i].past_end && 
		(line[i+1].start == PASTENDPOS) &&
		(line[i].start != PASTENDPOS))
	      AddRedraw(widget, line[i].start, widget->text.last_position);
	} else {
	    line[i] = oldline[j];
	    oldline[j].extra = NULL;
	    line[i].past_end =
		!(*widget->text.output->MeasureLine)(widget, i, line[i].start,
						NULL, NULL);

	    line[i+1].start = oldline[j+1].start;
	    line[i+1].extra = oldline[j+1].extra;
	}
	if (!line[i].past_end) {
	    if (line[i].start != oldline[j].start ||
	          line[i+1].start != oldline[j+1].start ||
	          line[i].start >= widget->text.forget_past) {
		AddRedraw(widget, line[i].start, line[i+1].start);
	    } else {
		if (i != j && line[i+1].start >= widget->text.last_position)
		  AddRedraw(widget, widget->text.last_position,
					 widget->text.last_position);
		if (oldline[j].changed)
		    AddRedraw(widget, oldline[j].changed_position,
							 line[i+1].start);
		if (i != j && line[i].start != PASTENDPOS) {
		    if (endcopy == j-1) {
			endcopy = j;
			lastcopy++;
		    } else if (lastcopy >= 0 && j <= lastcopy) {
			/* This line was stomped by a previous move. */
			AddRedraw(widget, line[i].start, line[i+1].start);
		    } else {
			if (startcopy >= 0) 
			    DoMove(widget, startcopy, endcopy, destcopy);
			startcopy = endcopy = j;
			destcopy = lastcopy = i;
		    }
		}
	    }
	}
	line[i].changed = FALSE;
	if (!line[i].past_end) widget->text.number_lines++;
	else widget->text.bottom_position =
			 MIN(line[i].start, widget->text.last_position);
    }
    if (startcopy >= 0){
	DoMove(widget, startcopy, endcopy, destcopy);
    }
    for (j=0 ; j<oldNumLines ; j++)
	if (oldline[j].extra) {
	    XtFree((char *) oldline[j].extra);
	    oldline[j].extra = NULL;
	}
    widget->text.in_refigure_lines = FALSE;
    if (widget->text.top_character >= widget->text.last_position &&
        widget->text.last_position > widget->text.first_position &&
        widget->text.output->data->number_lines > 1) {
       widget->text.pending_scroll = -1; /* Try to not ever display nothing. */
       widget->text.needs_refigure_lines = TRUE;
    }
    if (widget->text.force_display >= 0) {
      if (widget->text.force_display < widget->text.top_character) {
         widget->text.new_top = (*widget->text.source->Scan)
					(widget->text.source,
					 widget->text.force_display,
				         XmSELECT_LINE, XmsdLeft, 1, FALSE);
	 widget->text.needs_refigure_lines = TRUE;
      } else if (widget->text.force_display > widget->text.bottom_position) {
       /* need to add one to account for border condition,
        * i.e. cursor at begginning of line
        */
	 if (widget->text.force_display < widget->text.last_position)
            widget->text.new_top = widget->text.force_display + 1;
	 else
            widget->text.new_top = widget->text.last_position;
	 widget->text.needs_refigure_lines = TRUE;
	 widget->text.pending_scroll -= widget->text.number_lines;
      } else if (widget->text.force_display ==
			 line[widget->text.number_lines].start) {
         widget->text.new_top = widget->text.force_display;
	 widget->text.pending_scroll -= (widget->text.number_lines - 1);
	 widget->text.needs_refigure_lines = TRUE;
      }
      tell_output_force_display = widget->text.force_display;
      widget->text.force_display = -1;
    }
    if (widget->text.needs_refigure_lines) {
	RefigureLines(widget);
        if (XtIsRealized((Widget)widget)) TextDrawInsertionPoint(widget);
	return;
    }
    AddRedraw(widget, widget->text.forget_past, widget->text.bottom_position);
#ifndef OSF_v1_2_4
    widget->text.forget_past = MAXINT;
#else /* OSF_v1_2_4 */
    widget->text.forget_past = INT_MAX;
#endif /* OSF_v1_2_4 */
    if (tell_output_force_display >= 0) {
        (*widget->text.output->MakePositionVisible)(widget,
                                                   tell_output_force_display);
	tell_output_force_display = -1;
    }
    if (XtIsRealized((Widget)widget)) TextDrawInsertionPoint(widget);
}


int
#ifdef _NO_PROTO
_XmTextGetTotalLines(widget)
Widget widget;
#else
_XmTextGetTotalLines(Widget widget)
#endif /* _NO_PROTO */
{
   return(((XmTextWidget)widget)->text.total_lines);
}


XmTextLineTable
#ifdef _NO_PROTO
_XmTextGetLineTable(widget, total_lines)
Widget widget;
int *total_lines;
#else
_XmTextGetLineTable(Widget widget, int *total_lines)
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) widget;
   XmTextLineTable line_table;

   *total_lines = tw->text.total_lines;
   line_table = (XmTextLineTable) XtMalloc((unsigned) *total_lines *
						 sizeof(XmTextLineTableRec));
   
   memcpy((void *) line_table, (void *) tw->text.line_table,
	  *total_lines * sizeof(XmTextLineTableRec));

   return line_table;
}


/************************************************************************
 *
 * RemoveLines() - removes the lines from the global line table.
 *      widget - the widget that contains the global table.
 *      num_lines - number of lines to be removed.
 *      cur_line - pointer to the start of the lines to be removed.
 *
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
RemoveLines(widget, num_lines, cur_index)
        XmTextWidget widget;
        int num_lines;
        unsigned int cur_index ;
#else
RemoveLines(
        XmTextWidget widget,
        int num_lines,
        unsigned int cur_index)
#endif /* _NO_PROTO */
{
     if (!num_lines) return;

    /* move the existing lines at the end of the buffer */
     if (widget->text.total_lines > cur_index)
        memmove((void *) &widget->text.line_table[cur_index - num_lines], 
	        (void *) &widget->text.line_table[cur_index],
	        (size_t) ((widget->text.total_lines - (cur_index)) *
		           sizeof (XmTextLineTableRec)));

    /* reduce total line count */
     widget->text.total_lines -= num_lines;
 
    /* fix for bug 5166 */
     if (widget->text.total_lines <= widget->text.table_index)
        widget->text.table_index = widget->text.total_lines - 1;
 

    /* Shrink Table if Necessary */
     if ((widget->text.table_size > TABLE_INCREMENT &&
         widget->text.total_lines <= widget->text.table_size-TABLE_INCREMENT) ||
         widget->text.total_lines <= widget->text.table_size >> 1) {
      
        widget->text.table_size = INIT_TABLE_SIZE;

        while (widget->text.total_lines >= widget->text.table_size) {
	   if (widget->text.table_size < TABLE_INCREMENT)
	      widget->text.table_size *= 2;
	   else
	      widget->text.table_size += TABLE_INCREMENT;
        }

        widget->text.line_table = (XmTextLineTable)
				  XtRealloc((char *) widget->text.line_table,
						   widget->text.table_size *
						   sizeof(XmTextLineTableRec));
     }
}

static void
#ifdef _NO_PROTO
AddLines(widget, temp_table, tmp_index, current_index)
        XmTextWidget widget;
    	XmTextLineTable temp_table;
    	unsigned int tmp_index;
	unsigned int current_index;
#else
AddLines(
    	XmTextWidget widget,
    	XmTextLineTable temp_table,
    	unsigned int tmp_index,
	unsigned int current_index)
#endif /* _NO_PROTO */
{
     register unsigned int i;
     register unsigned int size_needed;
     register unsigned int cur_index;
     register unsigned int temp_index;

     cur_index = current_index;
     temp_index = tmp_index;
     size_needed = widget->text.total_lines + temp_index;

    /* make sure table is big enough to handle the additional lines */
     if (widget->text.table_size < size_needed) {
	while (widget->text.table_size < size_needed)
	   if (widget->text.table_size < TABLE_INCREMENT)
	      widget->text.table_size *= 2;
	   else
	      widget->text.table_size += TABLE_INCREMENT;
        widget->text.line_table = (XmTextLineTable)
				  XtRealloc((char *) widget->text.line_table,
						     widget->text.table_size *
						    sizeof(XmTextLineTableRec));
     }

    /* move the existing lines at the end of the buffer */
     if (widget->text.total_lines > cur_index)
        memmove((void *) &widget->text.line_table[cur_index + temp_index], 
	        (void *) &widget->text.line_table[cur_index],
	        (size_t) ((widget->text.total_lines - cur_index) *
		           sizeof (XmTextLineTableRec)));

     widget->text.total_lines += temp_index;

    /* Add the lines from the temp table */
     if (temp_table)
        for (i = 0; i < temp_index; i++, cur_index++)
	    widget->text.line_table[cur_index] = temp_table[i];
}

void 
#ifdef _NO_PROTO
_XmTextRealignLineTable(widget, temp_table, temp_table_size, cur_index,
		 cur_start, cur_end)
	XmTextWidget widget;
        XmTextLineTable *temp_table;
	int *temp_table_size;
	register unsigned int cur_index;
	register XmTextPosition cur_start;
	register XmTextPosition cur_end;
#else /* _NO_PROTO */
_XmTextRealignLineTable(
	XmTextWidget widget,
        XmTextLineTable *temp_table,
        int *temp_table_size,
	register unsigned int cur_index,
	register XmTextPosition cur_start,
	register XmTextPosition cur_end)
#endif /* _NO_PROTO */

{
    register int table_size;
    register XmTextPosition line_end;
    register XmTextPosition next_start;
    XmTextLineTable line_table;

    if (temp_table) {
       line_table = *temp_table;
       table_size = *temp_table_size;
    } else {
       line_table = widget->text.line_table;
       table_size = widget->text.table_size;
    }

    line_table[cur_index].start_pos = next_start = cur_start;
    cur_index++;

    line_end = (*widget->text.source->Scan)(widget->text.source, cur_start,
                                            XmSELECT_LINE, XmsdRight, 1, TRUE);
    while (next_start < cur_end) {
          next_start = _XmTextFindLineEnd(widget, cur_start, NULL);
          if (next_start == PASTENDPOS || next_start == cur_end) break;
          if (next_start == cur_start)
	     next_start = (*widget->text.source->Scan) (widget->text.source,
                                                  cur_start, XmSELECT_POSITION,
                                                  XmsdRight, 1, TRUE);
	  if (cur_index >= table_size) {
             if (table_size < TABLE_INCREMENT)
                table_size *= 2;
             else
                table_size += TABLE_INCREMENT;

	     line_table = (XmTextLineTable) XtRealloc((char *)line_table,
						    table_size *
						    sizeof(XmTextLineTableRec));
	  }
	  line_table[cur_index].start_pos = (unsigned int) next_start;
          if (line_end == next_start) {
	     line_table[cur_index].virt_line = 0;
    	     line_end = (*widget->text.source->Scan)(widget->text.source,
						     next_start, XmSELECT_LINE,
						     XmsdRight, 1, TRUE);
	  } else
	     line_table[cur_index].virt_line = 1;
          cur_index++;
	  cur_start = next_start;
  }

  if (temp_table) {
     *temp_table = line_table;
     *temp_table_size = cur_index;
  } else {
     widget->text.total_lines = cur_index;
     widget->text.line_table = line_table;
     widget->text.table_size = table_size;
  }
}

static void
#ifdef _NO_PROTO
InitializeLineTable(widget, size)
        XmTextWidget widget;
	register int size;
#else
InitializeLineTable(
        XmTextWidget widget,
	register int size)
#endif /* _NO_PROTO */
{
    register unsigned int t_index;
    register XmTextLineTable line_table;

    line_table = (XmTextLineTable) XtMalloc(size * sizeof(XmTextLineTableRec));

    for (t_index = 0; t_index < size; t_index++) {
       line_table[t_index].start_pos = 0;
       line_table[t_index].virt_line = 0;
    }

    widget->text.line_table = line_table;
    widget->text.table_index = 0;
    widget->text.table_size = size;
}

unsigned int
#ifdef _NO_PROTO
_XmTextGetTableIndex(widget, pos)
        XmTextWidget widget;
        XmTextPosition pos;
#else
_XmTextGetTableIndex(
        XmTextWidget widget,
        XmTextPosition pos)
#endif /* _NO_PROTO */
{
   register XmTextLineTable line_table;
   register unsigned int cur_index;
   register unsigned int max_index;
   register XmTextPosition position;

   position = pos;
   max_index = widget->text.total_lines - 1;
   line_table = widget->text.line_table;
   cur_index = widget->text.table_index;

  /* look forward to find the current record */
   if (line_table[cur_index].start_pos < (unsigned int) position){
      while (cur_index < max_index &&
             line_table[cur_index].start_pos < (unsigned int) position)
         cur_index++;
      /* if over shot it by one */
      if (position < line_table[cur_index].start_pos) cur_index--;
   } else
     /* look backward to find the current record */
      while (cur_index &&
             line_table[cur_index].start_pos > (unsigned int) position)
	cur_index--;

   return (cur_index);
}



void 
#ifdef _NO_PROTO
_XmTextUpdateLineTable(widget, start, end, block, update)
        Widget widget;
        XmTextPosition start;
        XmTextPosition end;
	XmTextBlock block;
        Boolean update;
#else
_XmTextUpdateLineTable(
        Widget widget,
        XmTextPosition start,
        XmTextPosition end,
	XmTextBlock block,
#if NeedWidePrototypes
	int update)
#else
        Boolean update)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register unsigned int cur_index;
    register unsigned int begin_index;
    register unsigned int end_index;
    register XmTextLineTable line_table;
    register unsigned int max_index;
    register int lines_avail;
    register int length;
    register long delta;
    unsigned int start_index;
    unsigned int top_index;
    XmTextWidget tw = (XmTextWidget) widget;
    Boolean word_wrap = _XmTextShouldWordWrap(tw);
    XmTextPosition cur_start, cur_end;
    int diff = 0;
    int block_num_chars = 0;
    int char_size = 0;

    lines_avail = 0;
    max_index = tw->text.total_lines - 1;
    if (tw->text.char_size != 1) 
       block_num_chars = _XmTextCountCharacters(block->ptr, block->length);
    else
       block_num_chars = block->length;
    delta = block_num_chars - (end - start);
    length = block_num_chars;

   if (tw->text.line_table == NULL)
      if (tw->text.edit_mode == XmSINGLE_LINE_EDIT)
         InitializeLineTable(tw, 1);
      else
         InitializeLineTable(tw, INIT_TABLE_SIZE);

  /* if there is no change or we expect RelignLineTable()
     to be called before the line table is necessary */
   if ((start == end && length == 0) || 
       (word_wrap && !XtIsRealized(tw)
	&& XmIsScrolledWindow(XtParent(widget))
	&& XtIsShell(XtParent(XtParent(widget))))) {
      return;
   }

   line_table = tw->text.line_table;

   cur_index = _XmTextGetTableIndex(tw, start);
   top_index = _XmTextGetTableIndex(tw, tw->text.top_character);

   begin_index = start_index = end_index = cur_index;

   if (word_wrap && delta > 0)
      cur_end = end + delta;
   else
      cur_end = end;

  /* Find the cur_end position.
     Count the number of lines that were deleted. */
   if (end > start) {
      if (end_index < tw->text.total_lines) {
         while (end_index < max_index &&
		line_table[end_index + 1].start_pos <= (unsigned int) cur_end) {
		end_index++;
		lines_avail++;
         }
      } else if (line_table[end_index].start_pos > start &&
		 line_table[end_index].start_pos <= cur_end) {
		lines_avail++;
      }
   }

   cur_index = end_index;

   if (word_wrap) {
      register int i;
      XmTextLineTable temp_table = NULL;
      int temp_table_size = 0;

      if (line_table[start_index].virt_line) start_index--;

     /* get the start position of the line at the start index. */
      cur_start = line_table[begin_index].start_pos;

     /* If we are not at the end of the table, */
      if (cur_index < max_index) {
        /* find the next non-wordwrapped line. */
         while (cur_index < max_index) {
	       cur_index++;
	       if (!line_table[cur_index].virt_line) break;
         }
        /* Set the cur_end position to the position of
           the next non-wordwrapped line. */
         cur_end = line_table[cur_index].start_pos;
        /* estimate the temp table size */
         temp_table_size = cur_index - begin_index;
        /* make sure the size is not zero */
         if (!temp_table_size) temp_table_size++;
	/* do initial allocation of the temp_table */
         temp_table = (XmTextLineTable) XtMalloc(temp_table_size *
					         sizeof(XmTextLineTableRec)); 
        /* Determine the lines that have changed. */
         _XmTextRealignLineTable(tw, &temp_table, &temp_table_size,
			         0, cur_start, cur_end + delta);
      
        /* Compute the difference in the number of lines that have changed */
         diff = temp_table_size - (cur_index - begin_index);
   
        /* if new/wrapped lines were added, push line down*/
         if (diff > 0)
            AddLines(tw, NULL, diff, cur_index);
        /* if new/wrapped lines were deleted, move line up */
         else
	    RemoveLines(tw, -diff, cur_index);

        /*
	 * The line table may have been realloc'd in any of the three
	 * previous function calls, so it must be reassigned to prevent
	 * a stale pointer.
         */
         line_table = tw->text.line_table;

        /* Bypass the first entry in the temp_table */
	 begin_index++;

        /* Add the lines from the temp table */
         for (i = 1; i < temp_table_size; i++, begin_index++)
	     line_table[begin_index] = temp_table[i];
      
       /* Free temp table */
        XtFree((char *)temp_table);

       /* Adjust the cur_index by the number of lines that changed. */
        cur_index += diff;
        max_index += diff;

       /* Adjust start values in table by the amount of change. */
        while (cur_index <= max_index) {
            line_table[cur_index].start_pos += delta;
	    cur_index++;
        }
      } else
       /* add lines to the end */
         _XmTextRealignLineTable(tw, NULL, 0, begin_index,
			         cur_start, PASTENDPOS);
   } else {
      register char *ptr;
      register XmTextLineTable temp_table;
      register int temp_table_size;
      register int temp_index;

      temp_table = NULL;
      temp_table_size = 0;
      temp_index = 0;
      ptr = block->ptr;
      cur_start = start;

      while (cur_index < max_index) {
	  cur_index++;
          line_table[cur_index].start_pos += delta;
      }

      if (tw->text.char_size == 1) {
         while (length--) {
             cur_start++;
             if (*ptr++ == '\012') {
	         if (lines_avail && begin_index < tw->text.total_lines) {
	            begin_index++;
		    lines_avail--;
		    line_table[begin_index].start_pos = (unsigned int)cur_start;
	         } else {
	            if (temp_index >= temp_table_size) {
                       if (!temp_table_size) {
                          if (tw->text.output->data->columns > 1) {
                             temp_table_size = length / 
				       (tw->text.output->data->columns / 2);
                             if (!temp_table_size) temp_table_size = 1;
                          } else {
			     if (length)
			        temp_table_size = length;
			     else
			        temp_table_size = 1;
                          }
                       } else
	                  temp_table_size *= 2;
		       temp_table =(XmTextLineTable)XtRealloc((char*)temp_table,
			          temp_table_size * sizeof(XmTextLineTableRec));
	            }
	            temp_table[temp_index].start_pos = (unsigned int) cur_start;
	            temp_table[temp_index].virt_line = (unsigned int) 0;
                    temp_index++;
                 }
             }
         }
      } else {
	while (length--) {
	  char_size = mblen(ptr, tw->text.char_size);
	  if (char_size < 0) break; /* error */
	  cur_start++;
	  if (char_size == 1 && *ptr == '\012') {
	    ptr++;
	    if (lines_avail && begin_index < tw->text.total_lines) {
	      begin_index++;
	      lines_avail--;
	      line_table[begin_index].start_pos = (unsigned int)cur_start;
	    } else {
	      if (temp_index >= temp_table_size) {
		if (!temp_table_size) {
		  if (tw->text.output->data->columns > 1) {
		    temp_table_size = length /
		      (tw->text.output->data->columns / 2);
		    if (!temp_table_size) temp_table_size = 1;
		  } else {
		    if (length)
		      temp_table_size = length;
		    else
		      temp_table_size = 1;
		  }
		} else
		  temp_table_size *= 2;
		temp_table = (XmTextLineTable)
		  XtRealloc((char*)temp_table,
			    temp_table_size * sizeof(XmTextLineTableRec));
	      }
	      temp_table[temp_index].start_pos = (unsigned int) cur_start;
	      temp_table[temp_index].virt_line = (unsigned int) 0;
	      temp_index++;
	    }
	  } else {
	    ptr += char_size;
	  }
	}
      }
      
     /* add a block of lines to the line table */
      if (temp_index) {
         AddLines(tw, temp_table, temp_index, begin_index + 1);
      }

     /* remove lines that are no longer necessary */
      if (lines_avail)
         RemoveLines(tw, lines_avail, end_index + 1);

      diff = temp_index - lines_avail;

      if (temp_table) XtFree((char *)temp_table);
   }

   if (update) {
      if (start < tw->text.top_character) {
	 if (end < tw->text.top_character) {
	    tw->text.top_line += diff;
	    tw->text.new_top = tw->text.top_character + delta;
	 } else {
	    int adjusted;
	    if (diff < 0)
	       adjusted = diff + (top_index - start_index);
	    else
	       adjusted = diff - (top_index - start_index);
	    tw->text.top_line += adjusted;
	    if (adjusted + (int) start_index <= 0) {
	       tw->text.new_top = 0;
	    } else if (adjusted + start_index > max_index) {
	       tw->text.new_top = line_table[max_index].start_pos;
	    } else {
	       tw->text.new_top = line_table[start_index + adjusted].start_pos;
	    }
	 }
	 tw->text.top_character = tw->text.new_top;
	 tw->text.forget_past = MIN(tw->text.forget_past, tw->text.new_top);
	 
         tw->text.top_line = _XmTextGetTableIndex(tw, tw->text.new_top);

	 if (tw->text.top_line < 0) 
	    tw->text.top_line = 0;

         if (tw->text.top_line > tw->text.total_lines)
	    tw->text.top_line = tw->text.total_lines - 1;
      }

      if (tw->text.table_index > tw->text.total_lines)
	 tw->text.table_index = tw->text.total_lines;

      if (start <  tw->text.cursor_position && tw->text.on_or_off == on) {
         XmTextPosition cursorPos = tw->text.cursor_position;
         if (tw->text.cursor_position < end) {
	    if (tw->text.cursor_position - start <= block_num_chars)
	       cursorPos = tw->text.cursor_position;
            else
	       cursorPos = start + block_num_chars;
         } else {
	    cursorPos = tw->text.cursor_position - (end - start) +
			block_num_chars;
         }
         _XmTextSetCursorPosition(widget, cursorPos);
      }
   }
}


/*
 * Compare the old_highlight list and the highlight list, determine what
 * changed, and call AddRedraw with the changed areas.
 */
static void 
#ifdef _NO_PROTO
FindHighlightingChanges( widget )
        XmTextWidget widget ;
#else
FindHighlightingChanges(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    int n1 = widget->text.old_highlight.number;
    int n2 = widget->text.highlight.number;
    _XmHighlightRec *l1 = widget->text.old_highlight.list;
    _XmHighlightRec *l2 = widget->text.highlight.list;
    int i1, i2;
    XmTextPosition next1, next2, last_position;

    i1 = i2 = 0;
    last_position = 0;
    while (i1 < n1 && i2 < n2) {
	if (i1 < n1-1) next1 = l1[i1+1].position;
	else next1 = widget->text.last_position;
	if (i2 < n2-1) next2 = l2[i2+1].position;
	else next2 = widget->text.last_position;
	if (l1[i1].mode != l2[i2].mode) {
	    AddRedraw(widget, last_position, MIN(next1, next2));
	}
	last_position = MIN(next1, next2);
	if (next1 <= next2) i1++;
	if (next1 >= next2) i2++;
    }
}

/*
 * Actually do some work.  This routine gets called to actually paint all the
 * stuff that has been pending. Prevent recursive calls and text redisplays
 * during destroys
 */
static void 
#ifdef _NO_PROTO
Redisplay( widget )
        XmTextWidget widget ;
#else
Redisplay(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
   /* Prevent recursive calls or text redisplay during detroys. */
    if (widget->text.in_redisplay || widget->core.being_destroyed ||
	widget->text.disable_depth != 0 || !XtIsRealized(widget)) return;

    if (!widget->text.output->data->has_rect) _XmTextAdjustGC(widget);

    EraseInsertionPoint(widget);

    widget->text.in_redisplay = TRUE;

    RefigureLines(widget);
    widget->text.needs_redisplay = FALSE;

    if (widget->text.highlight_changed) {
	FindHighlightingChanges(widget);
	widget->text.highlight_changed = FALSE;
    }

    RedrawChanges(widget);

   /* Can be caused by auto-horiz scrolling... */
    if (widget->text.needs_redisplay) {
	RedrawChanges(widget);
	widget->text.needs_redisplay = FALSE;
    }
    widget->text.in_redisplay = FALSE;

    TextDrawInsertionPoint(widget);
}



/****************************************************************
 *
 * Definitions exported to output.
 *
 ****************************************************************/

/*
 * Mark the given range of text to be redrawn.
 */

void 
#ifdef _NO_PROTO
_XmTextMarkRedraw( widget, left, right )
        XmTextWidget widget ;
        XmTextPosition left ;
        XmTextPosition right ;
#else
_XmTextMarkRedraw(
        XmTextWidget widget,
        XmTextPosition left,
        XmTextPosition right )
#endif /* _NO_PROTO */
{
    if (left < right) {
 	AddRedraw(widget, left, right);
	widget->text.needs_redisplay = TRUE;
	if (widget->text.disable_depth == 0) Redisplay(widget);
    }
}


/*
 * Return the number of lines in the linetable.
 */
LineNum 
#ifdef _NO_PROTO
_XmTextNumLines( widget )
        XmTextWidget widget ;
#else
_XmTextNumLines(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    if (widget->text.needs_refigure_lines) RefigureLines(widget);
    return widget->text.number_lines;
}

void 
#ifdef _NO_PROTO
_XmTextLineInfo( widget, line, startpos, extra )
        XmTextWidget widget ;
        LineNum line ;
        XmTextPosition *startpos ;
        LineTableExtra *extra ;
#else
_XmTextLineInfo(
        XmTextWidget widget,
        LineNum line,
        XmTextPosition *startpos,
        LineTableExtra *extra )
#endif /* _NO_PROTO */
{
    if (widget->text.needs_refigure_lines) RefigureLines(widget);
    if (widget->text.number_lines >= line) {
       if (startpos) *startpos = widget->text.line[line].start;
       if (extra) *extra = widget->text.line[line].extra;
    } else {
       if (startpos){
          unsigned int cur_index = _XmTextGetTableIndex(widget,
				             widget->text.line[line - 1].start);
          if (cur_index < widget->text.total_lines - 1)
	     *startpos = widget->text.line_table[cur_index + 1].start_pos;
          else
	     *startpos = widget->text.last_position;
       }
       if (extra) *extra = NULL;
    }
}

/*
 * Return the line number containing the given position.  If text currently
 * knows of no line containing that position, returns NOLINE.
 */
LineNum 
#ifdef _NO_PROTO
_XmTextPosToLine( widget, position )
        XmTextWidget widget ;
        XmTextPosition position ;
#else
_XmTextPosToLine(
        XmTextWidget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    int i;
    if (widget->text.needs_refigure_lines) RefigureLines(widget);
    if (position < widget->text.top_character ||
			 position  > widget->text.bottom_position)
	return NOLINE;
    for (i=0 ; i<widget->text.number_lines ; i++)
	if (widget->text.line[i+1].start > position) return i;
    if (position == widget->text.line[widget->text.number_lines].start)
	return widget->text.number_lines;
    return NOLINE;  /* Couldn't find line with given position */ 
}



/****************************************************************
 *
 * Definitions exported to sources.
 *
 ****************************************************************/
void 
#ifdef _NO_PROTO
_XmTextInvalidate( widget, position, topos, delta )
        XmTextWidget widget ;
        XmTextPosition position ;
        XmTextPosition topos ;
        long delta ;
#else
_XmTextInvalidate(
        XmTextWidget widget,
        XmTextPosition position,
        XmTextPosition topos,
        long delta )
#endif /* _NO_PROTO */
{
    LineNum l;
    int i;
    XmTextPosition p, endpos;
    int shift = 0;
    int shift_start = 0;

#define ladjust(p) if ((p > position && p != PASTENDPOS) ||		       \
	(p == position && delta < 0)) {				     	       \
    	  p += delta;						     	       \
	  if (p < widget->text.first_position) p = widget->text.first_position;\
	  if (p > widget->text.last_position) p = widget->text.last_position;  \
	}

#define radjust(p) if ((p > position && p != PASTENDPOS) ||		       \
	(p == position && delta > 0)) {			                       \
    	  p += delta;					       		       \
	  if (p < widget->text.first_position) p = widget->text.first_position;\
	  if (p > widget->text.last_position) p = widget->text.last_position;  \
	}

    widget->text.first_position = (*widget->text.source->Scan)
					(widget->text.source, 0,
					 XmSELECT_ALL, XmsdLeft, 1, FALSE);
    widget->text.last_position = (*widget->text.source->Scan)
					(widget->text.source,  0,
					 XmSELECT_ALL, XmsdRight, 1, FALSE);
#ifndef OSF_v1_2_4
    if (delta == NODELTA) {
#else /* OSF_v1_2_4 */
    if (delta == LONG_MAX) {
#endif /* OSF_v1_2_4 */
       if (widget->text.top_character == topos && position != topos) {
	   widget->text.pending_scroll = -1;
	   widget->text.forget_past = MIN(widget->text.forget_past, position);
       }
       if (widget->text.top_character > position &&
               widget->text.bottom_position < topos) {
	   widget->text.new_top = position;
	   widget->text.pending_scroll = -1;
	   widget->text.forget_past = MIN(widget->text.forget_past, position);
       }
       
       if (widget->text.in_resize && widget->text.line_table != NULL) {
          unsigned int top_index, last_index, next_index;
          int index_offset, lines_used;

          top_index = widget->text.top_line;
          last_index = _XmTextGetTableIndex(widget, widget->text.last_position);
        
          lines_used = (last_index - top_index) + 1;

          if (top_index != 0 &&
	      widget->text.output->data->number_lines > lines_used) {
	     index_offset = widget->text.output->data->number_lines-lines_used;
             if (index_offset < widget->text.total_lines - lines_used)
	        next_index = top_index - index_offset;
             else
	        next_index = 0;
             widget->text.new_top = widget->text.top_character =
				  widget->text.line_table[next_index].start_pos;
          }
       }
           
       widget->text.forget_past = MIN(widget->text.forget_past, position);
    } else {
	for (i=0 ; i<widget->text.repaint.number ; i++) {
	    radjust(widget->text.repaint.range[i].from);
	    ladjust(widget->text.repaint.range[i].to);
	}
	for (i=0 ; i < widget->text.highlight.number ; i++)
	   if (delta < 0 &&
	       widget->text.highlight.list[i].position > position - delta)
	      ladjust(widget->text.highlight.list[i].position);
	for (i=0 ; i<widget->text.old_highlight.number ; i++)
	   if (delta < 0 &&
	       widget->text.old_highlight.list[i].position > position - delta)
	      ladjust(widget->text.old_highlight.list[i].position);
	for (i=0 ; i <= widget->text.number_lines && 
		   widget->text.line[i].start != PASTENDPOS; i++) {
	    if (delta > 0) {
	       radjust(widget->text.line[i].start);
	    } else {
	       if (widget->text.line[i].start > position &&
	           widget->text.line[i].start <= topos) {
		  if (i != 0 && shift_start == 0)
		      shift_start = i;
		  shift++;
	       } else {
	          radjust(widget->text.line[i].start);
	       }
	    }
	    if (widget->text.line[i].changed) {
		radjust(widget->text.line[i].changed_position);
	    }
	}
	if (shift){
	   for (i=shift_start; i < widget->text.number_lines; i++){
              if ( (i < (shift_start + shift)) && widget->text.line[i].extra)
                  XtFree((char *) widget->text.line[i].extra);
	      if (i + shift < widget->text.number_lines) {
	         widget->text.line[i].start = widget->text.line[i+shift].start;
	         widget->text.line[i].extra = widget->text.line[i+shift].extra;
              } else {
	         widget->text.line[i].start = PASTENDPOS;
	         widget->text.line[i].extra = NULL;
              }
	      widget->text.line[i].changed = TRUE;
	      if (widget->text.line[i].start != PASTENDPOS)
	         widget->text.line[i].changed_position = 
				       widget->text.line[i + 1].start - 1;
	      else
	         widget->text.line[i].changed_position = PASTENDPOS;
           }
	}
	ladjust(widget->text.bottom_position);
	_XmTextResetClipOrigin(widget, widget->text.cursor_position, False);
	widget->text.output->data->refresh_ibeam_off = True;
	endpos = topos;
	radjust(endpos);

      /* Force _XmTextPosToLine to not bother trying to recalculate. */
	widget->text.needs_refigure_lines = FALSE;
	for (l = _XmTextPosToLine(widget, position), p = position ;
	     l < widget->text.number_lines &&
             widget->text.line[l].start <= endpos ;
	     l++, p = widget->text.line[l].start) {
	    if (l != NOLINE) {
		if (widget->text.line[l].changed) {
		    widget->text.line[l].changed_position = MIN(p,
					 widget->text.line[l].changed_position);
		} else {
		    widget->text.line[l].changed_position = p;
		    widget->text.line[l].changed = TRUE;
		}
	    }
	}
    }
    (*widget->text.output->Invalidate)(widget, position, topos, delta);
    (*widget->text.input->Invalidate)(widget, position, topos, delta);
    widget->text.needs_refigure_lines = widget->text.needs_redisplay = TRUE;
    if (widget->text.disable_depth == 0) Redisplay(widget);
}

static void 
#ifdef _NO_PROTO
InsertHighlight( widget, position, mode )
        XmTextWidget widget ;
        XmTextPosition position ;
        XmHighlightMode mode ;
#else
InsertHighlight(
        XmTextWidget widget,
        XmTextPosition position,
        XmHighlightMode mode )
#endif /* _NO_PROTO */
{
    _XmHighlightRec *l1;
    _XmHighlightRec *l = widget->text.highlight.list;
    int i, j;

    l1 = FindHighlight(widget, position, XmsdLeft);
    if (l1->position == position)
	l1->mode = mode;
    else {
	i = (l1 - l) + 1;
	widget->text.highlight.number++;
	if (widget->text.highlight.number > widget->text.highlight.maximum) {
	    widget->text.highlight.maximum = widget->text.highlight.number;
	    l = widget->text.highlight.list = (_XmHighlightRec *)
		XtRealloc((char *) l, widget->text.highlight.maximum *
					 sizeof(_XmHighlightRec));
	}
	for (j=widget->text.highlight.number-1 ; j>i ; j--)
	    l[j] = l[j-1];
	l[i].position = position;
	l[i].mode = mode;
    }
}
	
void 
#ifdef _NO_PROTO
XmTextSetHighlight( w, left, right, mode )
        Widget w ;
        XmTextPosition left ;
        XmTextPosition right ;
        XmHighlightMode mode ;
#else
XmTextSetHighlight(
        Widget w,
        XmTextPosition left,
        XmTextPosition right,
        XmHighlightMode mode )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget)w;
    _XmHighlightRec *l;
    XmHighlightMode endmode;
    int i, j;
    
    if (XmIsTextField(w))
      {   
        XmTextFieldSetHighlight( w, left, right, mode) ;
        return ;
      } 
    if (left >= right || right <= 0 ||
	right > widget->text.last_position) return;

    EraseInsertionPoint(widget);
    if (!widget->text.highlight_changed) {
       widget->text.highlight_changed = TRUE;
       if (widget->text.old_highlight.maximum < widget->text.highlight.number) {
           widget->text.old_highlight.maximum = widget->text.highlight.number;
           widget->text.old_highlight.list = (_XmHighlightRec *)
  	 XtRealloc((char *)widget->text.old_highlight.list,
	           widget->text.old_highlight.maximum *
		   sizeof(_XmHighlightRec));
       }
       widget->text.old_highlight.number = widget->text.highlight.number;
       memcpy((void *) widget->text.old_highlight.list,
	     (void *) widget->text.highlight.list,
             (size_t) widget->text.old_highlight.number *
		      sizeof(_XmHighlightRec));
    }
    endmode = FindHighlight(widget, right, XmsdLeft)->mode;
    InsertHighlight(widget, left, mode);
    InsertHighlight(widget, right, endmode);
    l = widget->text.highlight.list;
    i = 1;
    while (i < widget->text.highlight.number) {
	if (l[i].position >= left && l[i].position < right)
	    l[i].mode = mode;
	if (l[i].mode == l[i-1].mode) {
	    widget->text.highlight.number--;
	    for (j=i ; j<widget->text.highlight.number ; j++)
		l[j] = l[j+1];
	} else i++;
    }
   /* Force the image GC to be updated based on the new highlight record */
    _XmTextMovingCursorPosition(widget, widget->text.cursor_position);
    widget->text.needs_redisplay = TRUE;
    if (widget->text.disable_depth == 0)
	Redisplay(widget);

    widget->text.output->data->refresh_ibeam_off = True;
    TextDrawInsertionPoint(widget);
}

/****************************************************************
 *
 * Creation definitions.
 *
 ****************************************************************/
/*
 * Create the text widget.  To handle default condition of the core
 * height and width after primitive has already reset it's height and
 * width, use request values and reset height and width to original
 * height and width state.
 */
/* ARGSUSED */
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
    XmTextWidget req = (XmTextWidget) rw ;
    XmTextWidget newtw = (XmTextWidget) nw ;

    switch (MB_CUR_MAX) {
       case 1: case 2: case 4: {
          newtw->text.char_size = (char)MB_CUR_MAX;
	  break;
       }
       case 3: {
          newtw->text.char_size = (char)4;
	  break;
       }
       default:
          newtw->text.char_size = (char)1;
    }
    if (req->core.width == 0) newtw->core.width = req->core.width;
    if (req->core.height == 0) newtw->core.height = req->core.height;

 /* Flag used in losing focus verification to indicate that a traversal
    key was pressed.  Must be initialized to False */
    newtw->text.traversed = False;

    newtw->text.total_lines = 1;
    newtw->text.top_line = 0;
    newtw->text.vsbar_scrolling = False;
    newtw->text.in_setvalues = False;

    if (newtw->text.output_create == NULL)
	newtw->text.output_create = _XmTextOutputCreate;
    if (newtw->text.input_create == NULL)
	newtw->text.input_create = _XmTextInputCreate;

   /*  The following resources are defaulted to invalid values to indicate    */
   /*  that it was not set by the application.  If it gets to this point      */
   /*  and they are still invalid then set them to their appropriate default. */

   if(    !XmRepTypeValidValue( XmRID_EDIT_MODE,
                                                newtw->text.edit_mode, nw)    )
   {
	newtw->text.edit_mode = XmSINGLE_LINE_EDIT;
   }

   /* All 8 buffers must be created to be able to rotate the cut buffers */
   _XmCreateCutBuffers(nw);

   if (newtw->text.verify_bell == (Boolean) XmDYNAMIC_BOOL)
   {
     if (_XmGetAudibleWarning(nw) == XmBELL) 
       newtw->text.verify_bell = True;
     else
       newtw->text.verify_bell = False;
   }
#ifdef CDE_INTEGRATE
    {
    Boolean btn1_transfer;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(nw)), "enableBtn1Transfer", &btn1_transfer, NULL);
    if (btn1_transfer) /* for btn2 extend and transfer cases */
        XtOverrideTranslations(nw,
            XtParseTranslationTable(_XmTextEventBindingsCDE));
    if (btn1_transfer == True) /* for btn2 extend case */
        XtOverrideTranslations(nw,
            XtParseTranslationTable(_XmTextEventBindingsCDEBtn2));
    }
#endif /* CDE_INTEGRATE */
}

/*
 * Create a text widget.  Note that most of the standard stuff is actually
 * to be done by the output create routine called here, since output is in
 * charge of window handling.
 */
static void 
#ifdef _NO_PROTO
InitializeHook( wid, args, num_args_ptr )
        Widget wid ;
        ArgList args ;
        Cardinal *num_args_ptr ;
#else
InitializeHook(
        Widget wid,
        ArgList args,
        Cardinal *num_args_ptr )
#endif /* _NO_PROTO */
{
    register XmTextWidget widget;
    Cardinal num_args = *num_args_ptr;
    XmTextSource source;
    XmTextPosition top_character;
    XmTextBlockRec block;
    Position dummy;
    Boolean used_source = False;

    widget = (XmTextWidget) wid ;

    /* If text.wc_value is set, it overrides. Call _Xm..Create with it. */
    if (widget->text.source == nullsourceptr) {
       if (widget->text.wc_value != NULL) {
	  source = _XmStringSourceCreate((char*)widget->text.wc_value, True);
	  widget->text.value = NULL;
	  widget->text.wc_value = NULL;
       } else {
          source = _XmStringSourceCreate(widget->text.value, False);
	  widget->text.value = NULL;
       }
    } else {
       source = widget->text.source;
       if (widget->text.wc_value != NULL) {
	 char * tmp_value;
	 int num_chars, n_bytes;

	 for (num_chars=0; widget->text.wc_value[num_chars]!=0L; num_chars++);

	 tmp_value = XtMalloc((unsigned) 
			      (num_chars + 1) * (int)widget->text.char_size);
	 n_bytes = wcstombs(tmp_value, widget->text.wc_value,
			    (num_chars + 1) * (int)widget->text.char_size);
	 if (n_bytes == -1) n_bytes = 0;
	 tmp_value[n_bytes] = 0;  /* NULL terminate the string */
	 _XmStringSourceSetValue(widget, tmp_value);
	 XtFree(tmp_value);
	 widget->text.wc_value = NULL;
       } else if (widget->text.value != NULL) {
	 /* Default value or argument ? */
	 int i;
	 for (i = 0; i < num_args; i++)
	   if (widget->text.value == (char *)args[i].value &&
	       (args[i].name == XmNvalue || 
		strcmp(args[i].name, XmNvalue) == 0)) {
	     _XmStringSourceSetValue(widget, widget->text.value);
	     break;
	   }
       }
       widget->text.value = NULL;
       used_source = True;
    }

    widget->text.disable_depth = 1;
    widget->text.first_position = 0;
    widget->text.last_position = 0;
    widget->text.dest_position = 0;

    widget->text.needs_refigure_lines = widget->text.needs_redisplay = TRUE;
    widget->text.number_lines = 0;
    widget->text.maximum_lines = 1;
    widget->text.line = (Line) XtMalloc(sizeof(LineRec));
    widget->text.line->start = PASTENDPOS;
    widget->text.line->changed = False;
    widget->text.line->changed_position = PASTENDPOS;
    widget->text.line->past_end = False;
    widget->text.line->extra = NULL;
    widget->text.repaint.number = widget->text.repaint.maximum = 0;
    widget->text.repaint.range = (RangeRec *) XtMalloc(sizeof(RangeRec));
    widget->text.highlight.number = widget->text.highlight.maximum = 1;
    widget->text.highlight.list = (_XmHighlightRec *)
	                              XtMalloc(sizeof(_XmHighlightRec));
    widget->text.highlight.list[0].position = 0;
    widget->text.highlight.list[0].mode = XmHIGHLIGHT_NORMAL;
    widget->text.old_highlight.number = 0;
    widget->text.old_highlight.maximum = 1;
    widget->text.old_highlight.list = (_XmHighlightRec *)
	                                  XtMalloc(sizeof(_XmHighlightRec));
    widget->text.highlight_changed = FALSE;
    widget->text.on_or_off = on;
    widget->text.force_display = -1;
    widget->text.in_redisplay = widget->text.in_refigure_lines = FALSE;
    widget->text.in_resize = FALSE;
    widget->text.in_expose = FALSE;
    widget->text.pending_scroll = 0;
    widget->text.new_top = widget->text.top_character;
    widget->text.bottom_position = 0;
    widget->text.add_mode = False;
    widget->text.pendingoff = True;
    widget->text.forget_past = 0;

   /* Initialize table */
    if (widget->text.edit_mode == XmSINGLE_LINE_EDIT)
       InitializeLineTable(widget, 1);
    else
       InitializeLineTable(widget, INIT_TABLE_SIZE);

    (*widget->text.source->RemoveWidget)(widget->text.source, widget);
    widget->text.source = source;
    (*widget->text.source->AddWidget)(widget->text.source, widget);

    (*widget->text.output_create)(wid, args, num_args);

    XmTextSetEditable(wid, widget->text.editable);
    XmTextSetMaxLength(wid, widget->text.max_length);

    (*widget->text.input_create)(wid, args, num_args);

    widget->text.first_position = (*widget->text.source->Scan)
					(widget->text.source, 0,
					 XmSELECT_ALL, XmsdLeft, 1, FALSE);
    widget->text.last_position = (*widget->text.source->Scan)
					(widget->text.source, 0,
					 XmSELECT_ALL, XmsdRight, 1, FALSE);

    if (widget->text.cursor_position < 0)
       widget->text.cursor_position = 0;

    if (widget->text.cursor_position > widget->text.last_position)
      widget->text.cursor_position = widget->text.last_position;

    widget->text.dest_position = widget->text.cursor_position;

    if (!widget->text.editable || !XtIsSensitive(wid))
       _XmTextSetDestinationSelection(wid, 0, False, (Time)NULL);

    if (widget->text.edit_mode == XmMULTI_LINE_EDIT)
       top_character = (*widget->text.source->Scan)(widget->text.source, 
                                                 widget->text.top_character, 
						 XmSELECT_LINE, XmsdLeft, 1, 
						 FALSE);
    else
       top_character = widget->text.top_character;

    widget->text.new_top = top_character;
    widget->text.top_character = 0;
#ifndef OSF_v1_2_4
    _XmTextInvalidate(widget, top_character, top_character, NODELTA);
#else /* OSF_v1_2_4 */
    _XmTextInvalidate(widget, top_character, top_character, LONG_MAX);
#endif /* OSF_v1_2_4 */
    if (widget->text.disable_depth == 0)
        Redisplay(widget);

/*
 * Fix for CR 5704 - If the source has already been created, do not use
 *                   the original code - it has already been processed and
 *                   the gaps are not where they were the first time 
 *                   through for this source.  Instead, use
 *                   code similar to that used in XmTextSetSource().
 */
    if (!used_source) {
      widget->text.source->data->gap_start[0] = '\0'; /*Hack to utilize initial
							value when setting line
							table - saves a malloc
							and free. */
      if (widget->text.char_size == 1) {
	block.ptr = widget->text.source->data->ptr;
	if (block.ptr == NULL) block.length = 0;
	else block.length = strlen(block.ptr);
      } else 
	(void)(*widget->text.source->ReadSource)(source, 0, 
						 source->data->length,
						 &block);
    } else
      (void)(*widget->text.source->ReadSource)(source, 0, source->data->length,
					       &block);

    _XmTextUpdateLineTable(wid, 0, 0, &block, False);

    _XmStringSourceSetGappedBuffer(source->data, widget->text.cursor_position);

    widget->text.forget_past = widget->text.first_position;
    
    widget->text.disable_depth = 0;
    (*widget->text.output->PosToXY)(widget, widget->text.cursor_position,
                                    &(widget->text.cursor_position_x), &dummy);
}

static void 
#ifdef _NO_PROTO
Realize( w, valueMask, attributes )
        Widget w ;
        XtValueMask *valueMask ;
        XSetWindowAttributes *attributes ;
#else
Realize(
        Widget w,
        XtValueMask *valueMask,
        XSetWindowAttributes *attributes )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    Position dummy;
#ifndef OSF_v1_2_4
    Arg im_args[6];  /* To set initial values to input method */
    Cardinal n = 0;
    OutputData o_data = tw->text.output->data;
    XPoint xmim_point;
#endif /* OSF_v1_2_4 */

    (*tw->text.output->realize)(w, valueMask, attributes);
    (*tw->text.output->PosToXY)(tw, tw->text.cursor_position,
                                 &(tw->text.cursor_position_x), &dummy);
#ifndef OSF_v1_2_4
    
    if (tw->text.editable) {
      (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &xmim_point.x, 
				  &xmim_point.y);
      n = 0;
      XtSetArg(im_args[n], XmNfontList, o_data->fontlist); n++;
      XtSetArg(im_args[n], XmNbackground, w->core.background_pixel); n++;
      XtSetArg(im_args[n], XmNforeground, tw->primitive.foreground); n++;
      XtSetArg(im_args[n], XmNbackgroundPixmap, 
	       w->core.background_pixmap);n++;
      XtSetArg(im_args[n], XmNspotLocation, &xmim_point); n++;
      XtSetArg(im_args[n], XmNlineSpace, o_data->lineheight); n++;
      XmImSetValues(w, im_args, n);
    }
#endif /* OSF_v1_2_4 */
}


/****************************************************************
 *
 * Semi-public definitions.
 *
 ****************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w;
    int j;

    (*widget->text.source->RemoveWidget)(widget->text.source, widget);
    if (widget->text.input->destroy) (*widget->text.input->destroy)(w);
    if (widget->text.output->destroy) (*widget->text.output->destroy)(w);

    for (j = 0; j < widget->text.number_lines; j++) {
        if (widget->text.line[j].extra)
           XtFree((char *)widget->text.line[j].extra);
    }

    XtFree((char *)widget->text.line);

    XtFree((char *)widget->text.repaint.range);
    XtFree((char *)widget->text.highlight.list);
    XtFree((char *)widget->text.old_highlight.list);

    if (widget->text.line_table != NULL)
	XtFree((char *)widget->text.line_table);

    XtRemoveAllCallbacks (w, XmNactivateCallback);
    XtRemoveAllCallbacks (w, XmNfocusCallback);
    XtRemoveAllCallbacks (w, XmNlosingFocusCallback);
    XtRemoveAllCallbacks (w, XmNvalueChangedCallback);
    XtRemoveAllCallbacks (w, XmNmodifyVerifyCallback);
    XtRemoveAllCallbacks (w, XmNmotionVerifyCallback);
    XtRemoveAllCallbacks (w, XmNgainPrimaryCallback);
    XtRemoveAllCallbacks (w, XmNlosePrimaryCallback);
}

static void 
#ifdef _NO_PROTO
Resize( w )
        Widget w ;
#else
Resize(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* this flag prevents resize requests */
    tw->text.in_resize = True;

    if (_XmTextShouldWordWrap(tw))
      _XmTextRealignLineTable(tw, NULL, 0, 0, 0, PASTENDPOS);

    (*(tw->text.output->resize))(w, FALSE);

    tw->text.in_resize = False;
}

static void 
#ifdef _NO_PROTO
DoExpose( w, event, region )
        Widget w ;
        XEvent *event ;
        Region region ;
#else
DoExpose(
        Widget w,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* this flag prevents resize requests */
    tw->text.in_expose = True;

    (*(((XmTextWidget)w)->text.output->expose))(w, event, region);

    tw->text.in_expose = False;
}

static void 
#ifdef _NO_PROTO
GetValuesHook( w, args, num_args_ptr )
        Widget w ;
        ArgList args ;
        Cardinal *num_args_ptr ;
#else
GetValuesHook(
        Widget w,
        ArgList args,
        Cardinal *num_args_ptr )
#endif /* _NO_PROTO */
{
    XmTextWidget widget = (XmTextWidget) w;
    Cardinal num_args = *num_args_ptr;
    int i;

    XtGetSubvalues((XtPointer) widget,
		   resources, XtNumber(resources), args, num_args);

    for (i = 0; i < num_args; i++) {
        if (!strcmp(args[i].name, XmNvalue)) {
           *((XtPointer *)args[i].value) =
                  (XtPointer)_XmStringSourceGetValue(GetSrc(widget), False);
        }
    }

    for (i = 0; i < num_args; i++) {
        if (!strcmp(args[i].name, XmNvalueWcs)) {
           *((XtPointer *)args[i].value) =
                  (XtPointer)_XmStringSourceGetValue(GetSrc(widget), True);
        }
    }

    (*widget->text.output->GetValues)(w, args, num_args);
    (*widget->text.input->GetValues)(w, args, num_args);
}

void
#ifdef _NO_PROTO
_XmTextSetTopCharacter( widget, top_character )
	Widget widget ;
	XmTextPosition top_character ;
#else
_XmTextSetTopCharacter(
	Widget widget,
	XmTextPosition top_character )
#endif /* _NO_PROTO */
{

    XmTextWidget tw = (XmTextWidget) widget;
    LineNum line_num;

    if (tw->text.edit_mode != XmSINGLE_LINE_EDIT) {
	/* fix for CR 3610
	top_character = (*tw->text.source->Scan)(tw->text.source,
	    top_character, XmSELECT_LINE, XmsdLeft, 1, FALSE);
	*/
	line_num = _XmTextGetTableIndex(tw, top_character);
	top_character = tw->text.line_table[line_num].start_pos;
    }

    if (top_character != tw->text.new_top) {
	EraseInsertionPoint(tw);
	tw->text.new_top = top_character;
	tw->text.pending_scroll = 0;
	tw->text.needs_refigure_lines = tw->text.needs_redisplay = TRUE;
	if (tw->text.disable_depth == 0)
	    Redisplay(tw);
	_XmTextResetClipOrigin(tw, tw->text.cursor_position, False);
	TextDrawInsertionPoint(tw);
    }
}


/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValues( oldw, reqw, new_w, args, num_args )
        Widget oldw ;
        Widget reqw ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget oldw,
        Widget reqw,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmTextWidget old = (XmTextWidget) oldw;
    XmTextWidget newtw = (XmTextWidget) new_w;
    XmTextPosition new_cursor_pos;
    Boolean o_redisplay;
    Position dummy;
    Boolean need_new_cursorPos = False;
    Boolean need_text_redisplay = False;

    if (newtw->core.being_destroyed) return False;

    newtw->text.in_setvalues = True;

    if (!newtw->text.output->data->has_rect) _XmTextAdjustGC(newtw);

/* CR 5453 begin */
    if (newtw->text.cursor_position<0)
	newtw->text.cursor_position=0;
/* CR 5453 end */
    EraseInsertionPoint(newtw);

    _XmTextDisableRedisplay(newtw, TRUE);

  /* set cursor_position to a known acceptable value (0 is always acceptable)
   */
    new_cursor_pos = newtw->text.cursor_position;
    newtw->text.cursor_position = 0;

#ifndef OSF_v1_2_4
    if (newtw->core.sensitive == False &&
#else /* OSF_v1_2_4 */
    if (! XtIsSensitive(new_w) &&
#endif /* OSF_v1_2_4 */
	newtw->text.input->data->has_destination) {
       _XmTextSetDestinationSelection(new_w, 0, True,
				  XtLastTimestampProcessed(XtDisplay(new_w)));
    }

    if(    !XmRepTypeValidValue( XmRID_EDIT_MODE,
				newtw->text.edit_mode, new_w) )
    {
	 newtw->text.edit_mode = old->text.edit_mode;
    }

    if ((old->text.top_character != newtw->text.top_character) &&
        (newtw->text.top_character != newtw->text.new_top)) {
       XmTextPosition new_top;
       if (newtw->text.output->data->resizeheight)
	  new_top = 0;
       else
          new_top = newtw->text.top_character;

       newtw->text.top_character = old->text.top_character;
       _XmTextSetTopCharacter(new_w, new_top);
#ifdef OSF_v1_2_4
       if (newtw->text.needs_refigure_lines)
	 newtw->text.top_character = new_top;
#endif /* OSF_v1_2_4 */
    }

    if (old->text.source != newtw->text.source) {
        XmTextSource source = newtw->text.source;
        newtw->text.source = old->text.source;
	o_redisplay = newtw->text.needs_redisplay;
#ifndef OSF_v1_2_4
        XmTextSetSource(new_w, source, newtw->text.top_character, 0);
#else /* OSF_v1_2_4 */
        XmTextSetSource(new_w, source, old->text.top_character, 0);
#endif /* OSF_v1_2_4 */
	need_text_redisplay = newtw->text.needs_redisplay;
	newtw->text.needs_redisplay = o_redisplay;
    }

    if (old->text.editable != newtw->text.editable) {
       Boolean editable = newtw->text.editable;
       newtw->text.editable = old->text.editable;
       XmTextSetEditable(new_w, editable);
    }

    XmTextSetMaxLength(new_w, newtw->text.max_length);

   /* Four cases to handle for value:
    *   1. user set both XmNvalue and XmNwcValue.
    *   2. user set the opposite resource (i.e. value is a char*
    *      and user set XmNwcValue, or vice versa).
    *   3. user set the corresponding resource (i.e. value is a char*
    *      and user set XmNValue, or vice versa).
    *   4. user set neither XmNValue nor XmNwcValue
    */

   /* OSF says:  if XmNvalueWcs set, it overrides all else */

    if (newtw->text.wc_value != NULL) {
    /* user set XmNvalueWcs resource - it rules ! */
	wchar_t * wc_value;
	char * tmp_value;
        int num_chars, n_bytes;

	num_chars = n_bytes = 0;

	for (num_chars = 0, wc_value = newtw->text.wc_value;
	     wc_value[num_chars] != 0L;) num_chars++;

	tmp_value = XtMalloc((unsigned) 
			     (num_chars + 1) * (int)newtw->text.char_size);
	n_bytes = wcstombs(tmp_value, newtw->text.wc_value,
			   (num_chars + 1) * (int)newtw->text.char_size);
	if (n_bytes == -1) n_bytes = 0;
	tmp_value[n_bytes] = 0;  /* NULL terminate the string */
	o_redisplay = newtw->text.needs_redisplay;
	newtw->text.wc_value = NULL;
	newtw->text.value = NULL;
	_XmStringSourceSetValue(newtw, tmp_value);
	need_text_redisplay = newtw->text.needs_redisplay;
	newtw->text.needs_redisplay = o_redisplay;
	XtFree(tmp_value);
	need_new_cursorPos = True;
    } else if (newtw->text.value != NULL) {
	char * tmp_value;

        newtw->text.pendingoff = TRUE;
	o_redisplay = newtw->text.needs_redisplay;
	tmp_value = newtw->text.value;
        newtw->text.value = NULL;
        _XmStringSourceSetValue(newtw, tmp_value);
	need_text_redisplay = newtw->text.needs_redisplay;
	newtw->text.needs_redisplay = o_redisplay;
	need_new_cursorPos = True;
    }

  /* return cursor_position to it's original changed value */
    newtw->text.cursor_position = new_cursor_pos;

    if (old->text.cursor_position != newtw->text.cursor_position) {
#ifndef OSF_v1_2_4
	XmTextPosition new_position = newtw->text.cursor_position;
	newtw->text.cursor_position = old->text.cursor_position;

        if (new_position > newtw->text.source->data->length)
           _XmTextSetCursorPosition(new_w, newtw->text.source->data->length);
        else
	   _XmTextSetCursorPosition(new_w, new_position);
#else /* OSF_v1_2_4 */
      XmTextPosition new_position = newtw->text.cursor_position;
      newtw->text.cursor_position = old->text.cursor_position;
      
      if (new_position > newtw->text.source->data->length)
	_XmTextSetCursorPosition(new_w, newtw->text.source->data->length);
      else
	_XmTextSetCursorPosition(new_w, new_position);
#endif /* OSF_v1_2_4 */
    } else if (need_new_cursorPos) {
#ifndef OSF_v1_2_4
           XmTextPosition cursorPos;

           cursorPos = (*newtw->text.source->Scan)(newtw->text.source,
                                        XmTextGetCursorPosition(new_w),
                                           XmSELECT_ALL, XmsdLeft, 1, TRUE);
           _XmTextSetCursorPosition(new_w, cursorPos);
#else /* OSF_v1_2_4 */
      XmTextPosition cursorPos = -1;
      int ix;
      
      for (ix = 0; ix < *num_args; ix++)
	if (strcmp(args[ix].name, XmNcursorPosition) == 0) {
	  cursorPos = (XmTextPosition)args[ix].value;
	  break;
	}
      if (cursorPos == -1)
	cursorPos = (*newtw->text.source->Scan)(newtw->text.source,
						XmTextGetCursorPosition(new_w),
						XmSELECT_ALL, XmsdLeft, 1, 
						TRUE);
      _XmTextSetCursorPosition(new_w, cursorPos);
#endif /* OSF_v1_2_4 */
    } else 
      if (newtw->text.cursor_position > newtw->text.source->data->length) {
#ifndef OSF_v1_2_4
           _XmTextSetCursorPosition(new_w, newtw->text.source->data->length);
#else /* OSF_v1_2_4 */
	_XmTextSetCursorPosition(new_w, newtw->text.source->data->length);
#endif /* OSF_v1_2_4 */
      }
    
    o_redisplay = (*newtw->text.output->SetValues)
				(oldw, reqw, new_w, args, num_args);
    (*newtw->text.input->SetValues)(oldw, reqw, new_w, args, num_args);
    newtw->text.forget_past = 0;
    newtw->text.disable_depth--;   /* _XmTextEnableRedisplay() is not called
				       because we don't want a repaint yet */
    TextDrawInsertionPoint(newtw); /* increment cursor_on stack in lieu of
				       _XmTextEnableRedisplay() call. */
    (*newtw->text.output->PosToXY)(newtw, newtw->text.cursor_position,
                                   &(newtw->text.cursor_position_x), &dummy);

    if (o_redisplay) newtw->text.needs_redisplay = True;

    TextDrawInsertionPoint(newtw);

    if (XtSensitive(new_w) != XtSensitive(oldw)) {
       if (XtSensitive(new_w)) {
    	  EraseInsertionPoint(newtw);
	  newtw->text.output->data->blinkstate = off;
	  _XmTextToggleCursorGC(new_w);
	  TextDrawInsertionPoint(newtw);
       } else {
          if (newtw->text.output->data->hasfocus) {
	     newtw->text.output->data->hasfocus = False;
             _XmTextChangeBlinkBehavior(newtw, False);
	     EraseInsertionPoint(newtw);
	     _XmTextToggleCursorGC(new_w);
	     newtw->text.output->data->blinkstate = on;
	     TextDrawInsertionPoint(newtw);
          }
       }
       if (newtw->text.source->data->length > 0)
          newtw->text.needs_redisplay = True;
    }

    if ((!newtw->text.editable || !XtIsSensitive(new_w)) &&
	_XmTextHasDestination(new_w))
       _XmTextSetDestinationSelection(new_w, 0, False, (Time)NULL);

    /* don't shrink to nothing */
    if (newtw->core.width == 0) newtw->core.width = old->core.width;
    if (newtw->core.height == 0) newtw->core.height = old->core.height;

    /* Optimization for the case when only XmNvalue changes. 
       This considerably reduces flashing due to unneeded redraws */
    if (need_text_redisplay && 
	!newtw->text.needs_redisplay && 
	newtw->text.disable_depth == 0) {
      EraseInsertionPoint(newtw);
      newtw->text.disable_depth++;
      newtw->text.needs_redisplay = True;
      _XmTextEnableRedisplay(newtw);
      newtw->text.needs_redisplay = False;
    }

    newtw->text.in_setvalues = newtw->text.needs_redisplay;

    return newtw->text.needs_redisplay;
}

static XtGeometryResult 
#ifdef _NO_PROTO
QueryGeometry( w, intended, reply )
        Widget w ;
        XtWidgetGeometry *intended ;
        XtWidgetGeometry *reply ;
#else
QueryGeometry(
        Widget w,
        XtWidgetGeometry *intended,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (GMode (intended) & (~(CWWidth | CWHeight)))
       return(XtGeometryNo);

    reply->request_mode = (CWWidth | CWHeight);

    (*tw->text.output->GetPreferredSize)(w, &reply->width, &reply->height);
    if ((reply->width != intended->width) ||
        (reply->height != intended->height) ||
        (GMode(intended) != GMode(reply)))
       return (XtGeometryAlmost);
    else {
        reply->request_mode = 0;
        return (XtGeometryYes);
    }
}

static int
#ifdef _NO_PROTO
_XmTextGetSubstring( widget, start, num_chars, buf_size, buffer, want_wchar )
        Widget widget;
        XmTextPosition start;
        int num_chars;
        int buf_size;
        char *buffer;
        Boolean want_wchar;
#else
_XmTextGetSubstring(
        Widget widget,
        XmTextPosition start,
        int num_chars,
        int buf_size,
        char *buffer,
#if NeedWidePrototypes
	int want_wchar)
#else
        Boolean want_wchar )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;

    if (XmIsTextField(widget)){
       if (want_wchar){ /* wants wchar_t* data */
          return (XmTextFieldGetSubstringWcs(widget, start, num_chars, 
					  buf_size, (wchar_t *)buffer));
       } else /* wants char* data */
          return (XmTextFieldGetSubstring(widget, start, num_chars, 
					  buf_size, buffer));
    } else {
       XmTextBlockRec block;
       XmTextPosition pos, end;
       wchar_t * wc_buffer = (wchar_t*)buffer;
       int destpos = 0;

       end = start + num_chars;

       num_chars = 0; /* We're done with the value passed in, so let's
		       * re-use it when needed for the wchar functionality
		       * instead of creating a local automatic variable.
		       */

       for (pos = start; pos < end; ){
           pos = (*tw->text.source->ReadSource)(tw->text.source, pos, end,
                                                &block);
           if (block.length == 0) {
               if (!want_wchar)
                  buffer[destpos] = '\0';
               else 
                  wc_buffer[destpos] = (wchar_t)0L;
               return XmCOPY_TRUNCATED;
           }

           if (!want_wchar) {
              if (((destpos + block.length) * sizeof(char)) >= buf_size)
                 return XmCOPY_FAILED;
           } else { /* Need number of characters for buffer comparison */
              num_chars = _XmTextCountCharacters(block.ptr, block.length);
              if (((destpos + num_chars) * sizeof(char)) >= buf_size)
                 return XmCOPY_FAILED;
           }

           if (!want_wchar) {
              (void)memcpy((void*)&buffer[destpos], (void*)block.ptr, 
			   block.length);
              destpos += block.length;
           } else { /* want wchar_t* data */
              num_chars = mbstowcs(&wc_buffer[destpos], block.ptr, num_chars);
	      if (num_chars < 0) num_chars = 0;
              destpos += num_chars;
           }
       }

       if (!want_wchar)
          buffer[destpos] = '\0';
       else
          wc_buffer[destpos] = (wchar_t)0L;
    }

    return XmCOPY_SUCCEEDED;
}

/* Count the number of characters represented in the char* str.  By
 * definition, if MB_CUR_MAX == 1 then num_count_bytes == number of characters.
 * Otherwise, use mblen to calculate. */

int
#ifdef _NO_PROTO
_XmTextCountCharacters( str, num_count_bytes )
	char *str;
	int num_count_bytes;
#else
_XmTextCountCharacters(
        char *str,
        int num_count_bytes)
#endif /* _NO_PROTO */
{
   char * bptr;
   int count = 0;
   int char_size = 0;

   if (num_count_bytes <= 0)
      return 0;

   if (MB_CUR_MAX == 1 || MB_CUR_MAX == 0) /* Sun sets MB_CUR_MAX to 0, Argg!!*/
      return num_count_bytes;

   for (bptr = str; num_count_bytes > 0; count++, bptr+= char_size){
      char_size = mblen(bptr, MB_CUR_MAX);
      if (char_size <= 0) break; /* error */
      num_count_bytes -= char_size;
   }
   return count;
}


/****************************************************************
 *
 * Public definitions.
 *
 ****************************************************************/

Widget 
#ifdef _NO_PROTO
XmCreateScrolledText( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateScrolledText(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
  Widget swindow;
  Widget stext;
  Arg args_cache[30];
  ArgList merged_args;
#ifndef OSF_v1_2_4
  int i, n;
#else /* OSF_v1_2_4 */
  int n;
#endif /* OSF_v1_2_4 */
  char s_cache[30];
  char *s;
  Cardinal s_size;
  Cardinal arg_size = (argcount + 4) * sizeof(Arg);

  s_size = ((name) ? strlen(name) : 0) + 3;

  s = (char *) XmStackAlloc(s_size, s_cache);  /* Name + NULL + "SW" */
  if (name) {
     strcpy(s, name);
     strcat(s, "SW");
  } else {
     strcpy(s, "SW");
  }

 /*
  * merge the application arglist with the required preset arglist, for
  * creating the scrolled window portion of the scroll text.
  */
#ifndef OSF_v1_2_4
  merged_args = (ArgList)XmStackAlloc(arg_size, args_cache);
  for(i=0, n=0; i < argcount; i++ ) {
    if (strcmp(arglist[i].name, XmNtranslations)) {
      merged_args[n].name = arglist[i].name;
      merged_args[n].value = arglist[i].value;
      n++;
    }
#else /* OSF_v1_2_4 */
  merged_args = (ArgList)XmStackAlloc(arg_size*sizeof(Arg), args_cache);
  for(n=0; n < argcount; n++ ) {
    merged_args[n].name = arglist[n].name;
    merged_args[n].value = arglist[n].value;
#endif /* OSF_v1_2_4 */
  }
  XtSetArg(merged_args[n], XmNscrollingPolicy,
			   (XtArgVal) XmAPPLICATION_DEFINED); n++;
  XtSetArg(merged_args[n], XmNvisualPolicy, (XtArgVal) XmVARIABLE); n++;
  XtSetArg(merged_args[n], XmNscrollBarDisplayPolicy, (XtArgVal) XmSTATIC); n++;
  XtSetArg(merged_args[n], XmNshadowThickness, (XtArgVal) 0); n++;
 
  swindow = XtCreateManagedWidget(s, xmScrolledWindowWidgetClass, parent,
							 merged_args, n);
  XmStackFree(s, s_cache);
  XmStackFree((char *)merged_args, args_cache);

  /* Create Text widget.  */
  stext = XtCreateWidget(name, xmTextWidgetClass, swindow, arglist, argcount);

  /* Add callback to destroy ScrolledWindow parent. */
  XtAddCallback (stext, XmNdestroyCallback, _XmDestroyParentCallback, NULL);

  /* Return Text.*/
  return (stext);
}

Widget 
#ifdef _NO_PROTO
XmCreateText( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateText(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
    return XtCreateWidget(name, xmTextWidgetClass, parent, arglist, argcount);
}

int
#ifdef _NO_PROTO
XmTextGetSubstring( widget, start, num_chars, buf_size, buffer )
        Widget widget;
	XmTextPosition start;
	int num_chars;
	int buf_size;
	char *buffer;
#else
XmTextGetSubstring(
        Widget widget,
	XmTextPosition start,
	int num_chars,
	int buf_size,
	char *buffer )
#endif /* _NO_PROTO */
{
    return(_XmTextGetSubstring(widget, start, num_chars, buf_size, 
			       buffer, False));
}

int
#ifdef _NO_PROTO
XmTextGetSubstringWcs( widget, start, num_chars, buf_size, buffer )
        Widget widget;
	XmTextPosition start;
	int num_chars;
	int buf_size;
	wchar_t *buffer;
#else
XmTextGetSubstringWcs(
        Widget widget,
	XmTextPosition start,
	int num_chars,
	int buf_size,
	wchar_t *buffer )
#endif /* _NO_PROTO */
{
    return(_XmTextGetSubstring(widget, start, num_chars, buf_size, 
			(char*) buffer, True));
}

char * 
#ifdef _NO_PROTO
XmTextGetString( widget )
        Widget widget ;
#else
XmTextGetString(
        Widget widget )
#endif /* _NO_PROTO */
{
    char *text_copy;

    if (XmIsTextField(widget))
       text_copy = XmTextFieldGetString(widget);
    else
       text_copy = _XmStringSourceGetValue(GetSrc(widget), False);

    return (text_copy);
}


wchar_t *
#ifdef _NO_PROTO
XmTextGetStringWcs( widget )
        Widget widget ;
#else
XmTextGetStringWcs(
        Widget widget )
#endif /* _NO_PROTO */
{
    wchar_t *text_copy;

    if (XmIsTextField(widget))
       text_copy = (wchar_t *) XmTextFieldGetStringWcs(widget);
    else
       text_copy = (wchar_t *) _XmStringSourceGetValue(GetSrc(widget), True);

    return (text_copy);
}

XmTextPosition 
#ifdef _NO_PROTO
XmTextGetLastPosition( widget )
        Widget widget ;
#else
XmTextGetLastPosition(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetLastPosition(widget));
    else {
       XmTextSource source = GetSrc(widget);
       return (*source->Scan)(source, 0, XmSELECT_ALL, XmsdRight, 1, TRUE);
    }
}



static void 
#ifdef _NO_PROTO
_XmTextSetString( widget, value )
        Widget widget ;
        char *value ;
#else
_XmTextSetString(
        Widget widget,
        char *value )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldSetString(widget, value);
    else {
       XmTextWidget tw = (XmTextWidget) widget;

       tw->text.pendingoff = TRUE;
       if (value == NULL) value = "";
       _XmStringSourceSetValue(tw, value);

       /* after set, move insertion cursor to beginning of string. */
       _XmTextSetCursorPosition(widget, 0);
    }
}

void 
#ifdef _NO_PROTO
XmTextSetString( widget, value )
        Widget widget ;
        char *value ;
#else
XmTextSetString(
        Widget widget,
        char *value )
#endif /* _NO_PROTO */
{
   _XmTextSetString(widget, value);
}

void 
#ifdef _NO_PROTO
XmTextSetStringWcs( widget, wc_value )
        Widget widget ;
        wchar_t *wc_value ;
#else
XmTextSetStringWcs(
        Widget widget,
        wchar_t *wc_value )
#endif /* _NO_PROTO */
{
   char * tmp;
   wchar_t *tmp_wc;
   int num_chars = 0;
   int result;
   XmTextWidget tw = (XmTextWidget) widget;

   if( XmIsTextField( widget) )
     {   
        XmTextFieldSetStringWcs( widget, wc_value) ;
        return ;
     } 
   for (num_chars = 0, tmp_wc = wc_value; *tmp_wc != (wchar_t)0L;
	num_chars++) tmp_wc++;

   tmp = XtMalloc((unsigned) (num_chars + 1) * (int)tw->text.char_size);
   result = wcstombs(tmp, wc_value, (num_chars + 1) * (int)tw->text.char_size);

   if (result == (size_t) -1) /* if wcstombs fails, it returns (size_t) -1 */
      tmp = "";               /* if invalid data, pass in the empty string */

   _XmTextSetString(widget, tmp);

   XtFree(tmp);
}


static void 
#ifdef _NO_PROTO
_XmTextReplace( widget, frompos, topos, value, is_wchar )
        Widget widget ;
        XmTextPosition frompos ;
        XmTextPosition topos ;
        char *value ;
        Boolean is_wchar;
#else
_XmTextReplace(
        Widget widget,
        XmTextPosition frompos,
        XmTextPosition topos,
        char *value, 
#if NeedWidePrototypes
        int is_wchar)
#else
        Boolean is_wchar)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;
    XmTextSource source;
    XmTextBlockRec block, newblock;
    Boolean editable, freeBlock;
    Boolean need_free = False;
    int max_length;
    int num_chars;
    wchar_t * tmp_wc;
    XmTextPosition selleft, selright, cursorPos;
    char * tmp_block = NULL;

    source = GetSrc(tw);

    EraseInsertionPoint(tw);

    if ((*source->GetSelection)(tw->text.source, &selleft, &selright)) {
       if ((selleft > frompos && selleft < topos)  || 
           (selright >frompos && selright < topos) ||
           (selleft <= frompos && selright >= topos)) {
          (*source->SetSelection)(tw->text.source, tw->text.cursor_position,
				  tw->text.cursor_position,
				  XtLastTimestampProcessed(XtDisplay(widget)));
          if (tw->text.input->data->pendingdelete)
             tw->text.pendingoff = FALSE;
       }
    }

    block.format = XmFMT_8_BIT;
    if (!is_wchar) {
       if (value == NULL)
          block.length = 0;
       else
          block.length = strlen(value);
       block.ptr = value;
    } else { /* value is really a wchar_t ptr cast to char* */
       if (value == NULL) {
          block.length = 0;
       } else {
          for (tmp_wc = (wchar_t*)value, num_chars = 0; 
	       *tmp_wc != (wchar_t)0L; 
	       num_chars++) tmp_wc++;
          tmp_block = XtMalloc((unsigned) 
				     (num_chars + 1) * (int)tw->text.char_size);
          block.ptr = tmp_block;
          need_free = True;
	  tmp_wc = (wchar_t *) value;
	  /* if successful, wcstombs returns number of bytes, else -1 */
	  block.length = wcstombs(block.ptr, tmp_wc, 
				  (num_chars + 1) * (int)tw->text.char_size);
	  if (block.length == -1){
	     block.length = 0; /* if error, don't insert anything */
	     block.ptr[0] = '\0'; /* use the empty string */
	  }
       }
    }
    editable = _XmStringSourceGetEditable(source);
    max_length = _XmStringSourceGetMaxLength(source);

    _XmStringSourceSetEditable(source, TRUE);
#ifndef OSF_v1_2_4
    _XmStringSourceSetMaxLength(source, MAXINT);
#else /* OSF_v1_2_4 */
    _XmStringSourceSetMaxLength(source, INT_MAX);
#endif /* OSF_v1_2_4 */
    if (_XmTextModifyVerify(tw, NULL, &frompos, &topos,
                            &cursorPos, &block, &newblock, &freeBlock)) {
       (*source->Replace)(tw, NULL, &frompos, &topos, &newblock, False);
       if (frompos == tw->text.cursor_position && frompos == topos) {
	 /* Replace will not move us, we still want this to happen */
	 if (tw->text.char_size != 1) 
	   topos = frompos + _XmTextCountCharacters(newblock.ptr,
						    newblock.length);
	 else
	   topos = frompos + newblock.length;
	 _XmTextSetCursorPosition((Widget)tw, topos);
       }
       _XmTextValueChanged(tw, NULL);
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    }
    if (need_free)
       XtFree(tmp_block); 
    _XmStringSourceSetEditable(source, editable);
    _XmStringSourceSetMaxLength(source, max_length);

    if (tw->text.input->data->has_destination)
       _XmTextSetDestinationSelection(widget, tw->text.cursor_position,
                            False, XtLastTimestampProcessed(XtDisplay(widget)));

    TextDrawInsertionPoint(tw);
}

void
#ifdef _NO_PROTO
XmTextReplace( widget, frompos, topos, value )
        Widget widget ;
        XmTextPosition frompos ;
        XmTextPosition topos ;
        char *value ;
#else
XmTextReplace(
        Widget widget,
        XmTextPosition frompos,
        XmTextPosition topos,
        char *value )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldReplace(widget, frompos, topos, value);
    else 
       _XmTextReplace( widget, frompos, topos, value, False );
}
   
void
#ifdef _NO_PROTO
XmTextReplaceWcs( widget, frompos, topos, value )
        Widget widget ;
        XmTextPosition frompos ;
        XmTextPosition topos ;
        wchar_t *value ;
#else
XmTextReplaceWcs(
        Widget widget,
        XmTextPosition frompos,
        XmTextPosition topos,
        wchar_t *value )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldReplaceWcs(widget, frompos, topos, (wchar_t*) value);
    else 
       _XmTextReplace( widget, frompos, topos, (char*) value, True );
}

void 
#ifdef _NO_PROTO
XmTextInsert( widget, position, value )
        Widget widget ;
        XmTextPosition position ;
        char *value ;
#else
XmTextInsert(
        Widget widget,
        XmTextPosition position,
        char *value )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldInsert(widget, position, value);
    else {
       XmTextReplace(widget, position, position, value);
    }
}

   
void
#ifdef _NO_PROTO
XmTextInsertWcs( widget, position, wc_value )
        Widget widget ;
        XmTextPosition position ;
        wchar_t *wc_value ;
#else
XmTextInsertWcs(
        Widget widget,
        XmTextPosition position,
        wchar_t *wc_value )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldInsertWcs(widget, position, wc_value);
    else {
       XmTextReplaceWcs(widget, position, position, wc_value);
    }
}


void 
#ifdef _NO_PROTO
XmTextSetAddMode( widget, state )
        Widget widget ;
        Boolean state ;
#else
XmTextSetAddMode(
        Widget widget,
#if NeedWidePrototypes
        int state )
#else
        Boolean state )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldSetAddMode(widget, state);
    else {
       XmTextWidget tw = (XmTextWidget) widget;

       if (tw->text.add_mode == state) return;

       EraseInsertionPoint(tw);

       tw->text.add_mode = state;

       _XmTextToggleCursorGC(widget);

       TextDrawInsertionPoint(tw);
    }
}

Boolean 
#ifdef _NO_PROTO
XmTextGetAddMode( widget )
        Widget widget ;
#else
XmTextGetAddMode(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetAddMode(widget));
    else {
       XmTextWidget tw = (XmTextWidget) widget;

       return (tw->text.add_mode);
    }
}

Boolean 
#ifdef _NO_PROTO
XmTextGetEditable( widget )
        Widget widget ;
#else
XmTextGetEditable(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetEditable(widget));
    else
       return _XmStringSourceGetEditable(GetSrc(widget));
}

void 
#ifdef _NO_PROTO
XmTextSetEditable( widget, editable )
        Widget widget ;
        Boolean editable ;
#else
XmTextSetEditable(
        Widget widget,
#if NeedWidePrototypes
        int editable )
#else
        Boolean editable )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Arg args[6];
    Cardinal n = 0;
    XPoint xmim_point;
    XmTextWidget tw = (XmTextWidget) widget;

    if (XmIsTextField(widget))
       XmTextFieldSetEditable(widget, editable);
    else {
       if (!tw->text.editable && editable) {
          OutputData o_data = tw->text.output->data;

          XmImRegister(widget, (unsigned int) NULL);

          (*tw->text.output->PosToXY)(tw, tw->text.cursor_position,
                                      &xmim_point.x, &xmim_point.y);
          n = 0;
          XtSetArg(args[n], XmNfontList, o_data->fontlist); n++;
          XtSetArg(args[n], XmNbackground, 
		   widget->core.background_pixel); n++;
          XtSetArg(args[n], XmNforeground, tw->primitive.foreground); n++;
          XtSetArg(args[n], XmNbackgroundPixmap,
                   widget->core.background_pixmap);n++;
          XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
          XtSetArg(args[n], XmNlineSpace, o_data->lineheight); n++;
          XmImSetValues(widget, args, n);
       } else if (tw->text.editable && !editable){
           XmImUnregister(widget);
       }

       tw->text.editable = editable;

       n = 0;

       if (editable) {
          XtSetArg(args[n], XmNdropSiteActivity, XmDROP_SITE_ACTIVE); n++;
       } else {
 	  XtSetArg(args[n], XmNdropSiteActivity, XmDROP_SITE_INACTIVE); n++;
       }

       XmDropSiteUpdate(widget, args, n);

       _XmStringSourceSetEditable(GetSrc(tw), editable);
    }
}

int 
#ifdef _NO_PROTO
XmTextGetMaxLength( widget )
        Widget widget ;
#else
XmTextGetMaxLength(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetMaxLength(widget));
    else
       return _XmStringSourceGetMaxLength(GetSrc(widget));
}

void 
#ifdef _NO_PROTO
XmTextSetMaxLength( widget, max_length )
        Widget widget ;
        int max_length ;
#else
XmTextSetMaxLength(
        Widget widget,
        int max_length )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldSetMaxLength(widget, max_length);
    else {
       XmTextWidget tw = (XmTextWidget) widget;

       tw->text.max_length = max_length;
       _XmStringSourceSetMaxLength(GetSrc(tw), max_length);
    }
}



XmTextPosition 
#ifdef _NO_PROTO
XmTextGetTopCharacter( widget )
        Widget widget ;
#else
XmTextGetTopCharacter(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;

    if (tw->text.needs_refigure_lines)
	RefigureLines(tw);
    return tw->text.top_character;
}
    

#ifdef NOT_DEF
void
#ifdef _NO_PROTO
_XmTextSetTopCharacter( widget, top_character )
	Widget widget ;
	XmTextPosition top_character ;
#else
_XmTextSetTopCharacter(
	Widget widget,
	XmTextPosition top_character )
#endif /* _NO_PROTO */
{

    XmTextWidget tw = (XmTextWidget) widget;

    if (tw->text.edit_mode != XmSINGLE_LINE_EDIT)
       top_character = (*tw->text.source->Scan)(tw->text.source, top_character,
					   XmSELECT_LINE, XmsdLeft, 1, FALSE);

    if (top_character != tw->text.new_top) {
	tw->text.new_top = top_character;
	tw->text.pending_scroll = 0;
	tw->text.needs_refigure_lines = tw->text.needs_redisplay = TRUE;
	if (tw->text.disable_depth == 0)
	    Redisplay(tw);
    }
}
#endif /* NOT_DEF */


void 
#ifdef _NO_PROTO
XmTextSetTopCharacter( widget, top_character )
        Widget widget ;
        XmTextPosition top_character ;
#else
XmTextSetTopCharacter(
        Widget widget,
        XmTextPosition top_character )
#endif /* _NO_PROTO */
{

    XmTextWidget tw = (XmTextWidget) widget;

    if (tw->text.output->data->resizeheight){
       if (tw->text.top_character == 0)
	  return;
       else
	  top_character = 0;
    }

    _XmTextSetTopCharacter(widget, top_character);
}


XmTextPosition 
#ifdef _NO_PROTO
XmTextGetCursorPosition( widget )
        Widget widget ;
#else
XmTextGetCursorPosition(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetCursorPosition(widget));
    else {
       XmTextWidget tw = (XmTextWidget) widget;

       return tw->text.cursor_position;
    }
}

XmTextPosition 
#ifdef _NO_PROTO
XmTextGetInsertionPosition( widget )
        Widget widget ;
#else
XmTextGetInsertionPosition(
        Widget widget )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       return(XmTextFieldGetCursorPosition(widget));
    else
       return(XmTextGetCursorPosition(widget));
}

void 
#ifdef _NO_PROTO
_XmTextSetCursorPosition( widget, position )
        Widget widget ;
        XmTextPosition position ;
#else
_XmTextSetCursorPosition(
        Widget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;
    XmTextSource source;
    XmTextVerifyCallbackStruct cb;
    Position dummy;
    int n = 0;
    XPoint xmim_point;
    Arg args[6];

    if (position < 0){
       position = 0;
    }
	  
    if (position > tw->text.last_position) {
       position = tw->text.last_position;
    }

    source = GetSrc(tw);

   /* if position hasn't changed, don't call the modify verify callback */
    if (position != tw->text.cursor_position) {
      /* Call Motion Verify Callback before Cursor Changes Positon */
       cb.reason = XmCR_MOVING_INSERT_CURSOR;
       cb.event  = NULL;
       cb.currInsert = tw->text.cursor_position;
       cb.newInsert = position;
       cb.doit = True;
       XtCallCallbackList (widget, tw->text.motion_verify_callback,
                           (XtPointer) &cb);

      /* Cancel action upon application request */
       if (!cb.doit) {
          if (tw->text.verify_bell) XBell(XtDisplay(widget), 0);
          return;
       }
    }

   /* Erase insert cursor prior to move */
    EraseInsertionPoint(tw);
    tw->text.cursor_position = position;

   /*
    * If not in add_mode and pending delete state is on reset
    * the selection.
    */
    if (!tw->text.add_mode && tw->text.pendingoff &&
        _XmStringSourceHasSelection(source))
       (*source->SetSelection)(source, position, position,
	  XtLastTimestampProcessed(XtDisplay(widget)));

    /* ensure that IBeam at new location will be displayed correctly */
    _XmTextMovingCursorPosition(tw, position); /*correct GC for new location */

    if (tw->text.auto_show_cursor_position)
       XmTextShowPosition(widget, position);
    if (tw->text.needs_redisplay && tw->text.disable_depth == 0)
       Redisplay(tw);

    (*tw->text.output->PosToXY) (tw, position, &(tw->text.cursor_position_x), 
				 &dummy);

    _XmTextResetClipOrigin(tw, position, False); /* move clip origin */
    tw->text.output->data->refresh_ibeam_off = True; /* update IBeam off area
						      * before drawing IBeam */

    (*tw->text.output->PosToXY)(tw, position, &xmim_point.x, &xmim_point.y);
    n = 0;
    XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
    XmImSetValues((Widget)tw, args, n);
    TextDrawInsertionPoint(tw);
}

void 
#ifdef _NO_PROTO
XmTextSetInsertionPosition( widget, position )
        Widget widget ;
        XmTextPosition position ;
#else
XmTextSetInsertionPosition(
        Widget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
  XmTextWidget tw = (XmTextWidget) widget;

  if (XmIsTextField(widget))
    XmTextFieldSetInsertionPosition(widget, position);
  else {
    _XmTextSetCursorPosition(widget, position);
    
    _XmTextSetDestinationSelection(widget, tw->text.cursor_position, False,
				 XtLastTimestampProcessed(XtDisplay(widget)));
  }
}

void 
#ifdef _NO_PROTO
XmTextSetCursorPosition( widget, position )
        Widget widget ;
        XmTextPosition position ;
#else
XmTextSetCursorPosition(
        Widget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    if (XmIsTextField(widget))
       XmTextFieldSetCursorPosition(widget, position);
    else {
       XmTextSetInsertionPosition(widget, position);
    }
}



Boolean 
#ifdef _NO_PROTO
XmTextRemove( widget )
        Widget widget ;
#else
XmTextRemove(
        Widget widget )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldRemove(widget));
   else {
      XmTextWidget tw = (XmTextWidget) widget;
      XmTextSource source = tw->text.source;
      XmTextPosition left, right;

     if ( tw->text.editable == False ) 
         return False;                

      if (!(*source->GetSelection)(source, &left, &right) ||
         left == right) {
         tw->text.input->data->anchor = tw->text.cursor_position;
         return False;
      }


      XmTextReplace(widget, left, right, NULL);

      if (tw->text.cursor_position > left)
         _XmTextSetCursorPosition(widget, left);

      tw->text.input->data->anchor = tw->text.cursor_position;

      return True;
   }
}

Boolean 
#ifdef _NO_PROTO
XmTextCopy( widget, copy_time )
        Widget widget ;
        Time copy_time ;
#else
XmTextCopy(
        Widget widget,
        Time copy_time )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldCopy(widget, copy_time));
   else {
      char *selected_string = XmTextGetSelection (widget); /* text selection */
      long item_id = 0L;                    	     /* clipboard item id */
      long data_id = 0L;                             /* clipboard data id */
      int status;                                    /* clipboard status  */
      XmString clip_label;
      XTextProperty tmp_prop;
      Display *display = XtDisplay(widget);
      Window window = XtWindow(widget);
      char *atom_name;

      /*
       * Using the Xm clipboard facilities,
       * copy the selected text to the clipboard
       */
      if (selected_string != NULL) {
         clip_label = XmStringCreateLtoR ("XM_TEXT", XmFONTLIST_DEFAULT_TAG);
        /* start copy to clipboard */
        status = XmClipboardStartCopy(display, window, clip_label, copy_time,
				       widget, NULL, &item_id);

         if (status != ClipboardSuccess) {
	   XtFree(selected_string);
	   XmStringFree(clip_label);
           return False;
         }
   
         status = XmbTextListToTextProperty(display, &selected_string, 1,
			                    (XICCEncodingStyle)XStdICCTextStyle,
					    &tmp_prop);

         if (status != Success && status <= 0) {
	    XmClipboardCancelCopy(display, window, item_id);
	    XtFree(selected_string);
	    XmStringFree(clip_label);
	    return False;
         }

         atom_name = XGetAtomName(display, tmp_prop.encoding);

        /* move the data to the clipboard */
         status = XmClipboardCopy(display, window, item_id, atom_name,
			          (XtPointer)tmp_prop.value, tmp_prop.nitems,
				  0, &data_id);

         XtFree(atom_name);

         if (status != ClipboardSuccess) {
	    XmClipboardCancelCopy(display, window, item_id);
            XFree((char*)tmp_prop.value);
	    XmStringFree(clip_label);
	    return False;
	 }

        /* end the copy to the clipboard */
         status = XmClipboardEndCopy (display, window, item_id);

	 XtFree((char*)tmp_prop.value);
         XmStringFree(clip_label);

         if (status != ClipboardSuccess) return False;
      } else

         return False;
      if (selected_string!=NULL) 
	XtFree(selected_string);
        return True;
   }
 }

Boolean 
#ifdef _NO_PROTO
XmTextCut( widget, cut_time )
        Widget widget ;
        Time cut_time ;
#else
XmTextCut(
        Widget widget,
        Time cut_time )
#endif /* _NO_PROTO */
{
  if (XmIsTextField(widget))
     return(XmTextFieldCut(widget, cut_time));
  else {
     XmTextWidget tw = (XmTextWidget) widget;

     if ( tw->text.editable == False )  /* can't cut if you can't edit */
         return False;                  /* assume this means not possible */
                                        /* (NO ASSUMPTIONS - ie, NO COPY) */

     if (XmTextCopy(widget, cut_time))
        if (XmTextRemove(widget)) {
           if (tw->text.input->data->has_destination)
              _XmTextSetDestinationSelection(widget, tw->text.cursor_position,
					     False, cut_time);
           else
              XmTextSetAddMode(widget, False);
           return True;
        }

     return False;
  }
}


/*
 * Retrieves the current data from the clipboard
 * and paste it at the current cursor position
 */
Boolean 
#ifdef _NO_PROTO
XmTextPaste( widget )
        Widget widget ;
#else
XmTextPaste(
        Widget widget )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldPaste(widget));
   else {
      XmTextWidget tw = (XmTextWidget) widget;
      XmTextSource source = tw->text.source;
      XmTextPosition sel_left = 0;
      XmTextPosition sel_right = 0;
      XmTextPosition paste_pos_left, paste_pos_right, cursorPos;
      int status;                                /* clipboard status        */
      char * buffer;                             /* temporary text buffer   */
      unsigned long length;                      /* length of buffer        */
      unsigned long outlength = 0L;              /* length of bytes copied  */
      long private_id = 0L;                      /* id of item on clipboard */
      Boolean dest_disjoint = True;
      XmTextBlockRec block, newblock;
      Display *display = XtDisplay(widget);
      Window window = XtWindow(widget);
      Boolean get_ct = False;
      Boolean freeBlock;
      XTextProperty tmp_prop;
      int malloc_size = 0;
      int num_vals;
      char **tmp_value;
      char * total_tmp_value = NULL;
      int i;

      if ( tw->text.editable == False ) return False;                

      paste_pos_left = paste_pos_right= tw->text.cursor_position;

      status = XmClipboardInquireLength(display, window, "STRING", &length);

      if (status == ClipboardNoData || length == 0) {
         status = XmClipboardInquireLength(display, window, "COMPOUND_TEXT",
					   &length);
#ifndef OSF_v1_2_4
         if (status == ClipboardNoData || length == 0) return False;
#else /* OSF_v1_2_4 */
         if (status == ClipboardNoData || length == 0 || 
	     status == ClipboardLocked) return False;
#endif /* OSF_v1_2_4 */
         get_ct = True;
#ifndef OSF_v1_2_4
      }
#else /* OSF_v1_2_4 */
      } else if (status == ClipboardLocked)
	return False;
#endif /* OSF_v1_2_4 */

      /* malloc length of clipboard data */
      buffer = XtMalloc((unsigned) length);

      if (!get_ct) {
         status = XmClipboardRetrieve(display, window, "STRING", buffer,
				       length, &outlength, &private_id);
      } else {
         status = XmClipboardRetrieve(display, window, "COMPOUND_TEXT",
				       buffer, length, &outlength, &private_id);
      }


      if (status != ClipboardSuccess) {
	XmClipboardEndRetrieve(display, window);
	XtFree(buffer);
        return False;
      }

      if (XmTextGetSelectionPosition(widget, &sel_left, &sel_right)) {
         if (tw->text.input->data->pendingdelete &&
             paste_pos_left >= sel_left && paste_pos_right <= sel_right) {
            paste_pos_left = sel_left;
            paste_pos_right = sel_right;
            dest_disjoint = False;
         }
      }

      tmp_prop.value = (unsigned char *) buffer;
      if (!get_ct)
         tmp_prop.encoding = XA_STRING;
      else
	 tmp_prop.encoding = XmInternAtom(display, "COMPOUND_TEXT", False);

      tmp_prop.format = 8;
      tmp_prop.nitems = outlength;
      num_vals = 0;

      status = XmbTextPropertyToTextList(display, &tmp_prop, &tmp_value,
					 &num_vals);

     /* if no conversions, num_vals doesn't change */
      if (num_vals && (status == Success || status > 0)) {
	 for (i = 0; i < num_vals ; i++)
	     malloc_size += strlen(tmp_value[i]);

	 total_tmp_value = XtMalloc ((unsigned) malloc_size + 1);
	 total_tmp_value[0] = '\0';
	 for (i = 0; i < num_vals ; i++)
	    strcat(total_tmp_value, tmp_value[i]);
	 block.ptr = total_tmp_value;
	 block.length = strlen(total_tmp_value);
	 block.format = XmFMT_8_BIT;
	 XFreeStringList(tmp_value);
      } else {
	 malloc_size = 1; /* to force space to be freed */
	 total_tmp_value = XtMalloc ((unsigned)1);
	 *total_tmp_value = '\0';
	 block.ptr = total_tmp_value;
	 block.length = 0;
	 block.format = XmFMT_8_BIT;
      }

      /* add new text */
     if (_XmTextModifyVerify(tw, NULL, &paste_pos_left, &paste_pos_right,
                             &cursorPos, &block, &newblock, &freeBlock)) {
	if ((*source->Replace)(tw, NULL, &paste_pos_left, &paste_pos_right, 
			       &newblock, False) != EditDone) {
	   if (tw->text.verify_bell) XBell(display, 0);
	} else {
	    tw->text.input->data->anchor = paste_pos_left;

	    _XmTextSetCursorPosition(widget, cursorPos);

	    _XmTextSetDestinationSelection(widget, tw->text.cursor_position,
			     False, XtLastTimestampProcessed(display));

	    if (sel_left != sel_right) {
	       if (!dest_disjoint) {
		  (*source->SetSelection)(source, tw->text.dest_position,
				      tw->text.dest_position,
				      XtLastTimestampProcessed(display));
	       } else {
		  if (!tw->text.add_mode)
		     (*source->SetSelection)(source, tw->text.dest_position,
				      tw->text.dest_position,
				      XtLastTimestampProcessed(display));
	       }
	    }
	    _XmTextValueChanged(tw, NULL);
	}
	if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
     }
     XtFree(buffer);
     if (malloc_size != 0) XtFree(total_tmp_value);
   }

   return True;
}

char * 
#ifdef _NO_PROTO
XmTextGetSelection( widget )
        Widget widget ;
#else
XmTextGetSelection(
        Widget widget )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldGetSelection(widget));
   else {
      XmTextSource source;
      XmTextPosition left, right;

      source = GetSrc(widget);
      if ((!(*source->GetSelection)(source, &left, &right)) || right == left)
	  return NULL;

      return(_XmStringSourceGetString((XmTextWidget)widget, left, 
                                       right, False));
   }
}

wchar_t *
#ifdef _NO_PROTO
XmTextGetSelectionWcs( widget )
        Widget widget ;
#else
XmTextGetSelectionWcs(
        Widget widget )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldGetSelectionWcs(widget)); 
   else {
      XmTextSource source;
      XmTextPosition left, right;

      source = GetSrc(widget);
      if (!(*source->GetSelection)(source, &left, &right))
          return NULL;

      return((wchar_t *)_XmStringSourceGetString((XmTextWidget)widget, left, 
                                                 right, True));
   }
}



void 
#ifdef _NO_PROTO
XmTextSetSelection( widget, first, last, set_time )
        Widget widget ;
        XmTextPosition first ;
        XmTextPosition last ;
        Time set_time ;
#else
XmTextSetSelection(
        Widget widget,
        XmTextPosition first,
        XmTextPosition last,
        Time set_time )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      XmTextFieldSetSelection(widget, first, last, set_time);
   else {
      XmTextSource source;
      XmTextWidget tw = (XmTextWidget) widget;

      if (first < 0 || last > tw->text.last_position) return;

      source = GetSrc(widget);
      (*source->SetSelection)(source, first, last, set_time);
      tw->text.pendingoff = FALSE;
      _XmTextSetCursorPosition(widget, last);
      _XmTextSetDestinationSelection(widget, tw->text.cursor_position, False,
                                     set_time);
   }
}

void 
#ifdef _NO_PROTO
XmTextClearSelection( widget, clear_time )
        Widget widget ;
        Time clear_time ; /* Unused, kept for backwards compatibility */
#else
XmTextClearSelection(
        Widget widget,
        Time clear_time )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      XmTextFieldClearSelection(widget, clear_time);
   else {
      XmTextWidget tw = (XmTextWidget) widget;
      XmTextSource source = GetSrc(widget);

      (*source->SetSelection)(source, 1, -999, source->data->prim_time);
      if (tw->text.input->data->pendingdelete) {
         tw->text.pendingoff = FALSE;
      }
   }
}

Boolean 
#ifdef _NO_PROTO
XmTextGetSelectionPosition( widget, left, right )
        Widget widget ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
XmTextGetSelectionPosition(
        Widget widget,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldGetSelectionPosition(widget, left, right));
   else {
      XmTextWidget tw = (XmTextWidget) widget;

      return (*tw->text.source->GetSelection)(tw->text.source, left, right);
   }
}

XmTextPosition 
#ifdef _NO_PROTO
XmTextXYToPos( widget, x, y )
        Widget widget ;
        Position x ;
        Position y ;
#else
XmTextXYToPos(
        Widget widget,
#if NeedWidePrototypes
        int x,
        int y )
#else
        Position x,
        Position y )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldXYToPos(widget, x, y));
   else {
      XmTextWidget tw = (XmTextWidget) widget;

      return (*tw->text.output->XYToPos)(tw, x, y);
   }
}

Boolean 
#ifdef _NO_PROTO
XmTextPosToXY( widget, position, x, y )
        Widget widget ;
        XmTextPosition position ;
        Position *x ;
        Position *y ;
#else
XmTextPosToXY(
        Widget widget,
        XmTextPosition position,
        Position *x,
        Position *y )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldPosToXY(widget, position, x, y));
   else {
      XmTextWidget tw = (XmTextWidget) widget;

      return (*tw->text.output->PosToXY)(tw, position, x, y);
   }
}

XmTextSource 
#ifdef _NO_PROTO
XmTextGetSource( widget )
        Widget widget ;
#else
XmTextGetSource(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;

    return tw->text.source;
}

void 
#ifdef _NO_PROTO
XmTextSetSource( widget, source, top_character, cursor_position )
        Widget widget ;
        XmTextSource source ;
        XmTextPosition top_character ;
        XmTextPosition cursor_position ;
#else
XmTextSetSource(
        Widget widget,
        XmTextSource source,
        XmTextPosition top_character,
        XmTextPosition cursor_position )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;
    XmTextPosition pos = 0;
    XmTextPosition last_pos = 0;
    XmTextPosition old_pos = 0;
    XmTextBlockRec block;
    int n = 0;
    XPoint xmim_point;
    Arg args[1];

    EraseInsertionPoint(tw);
    if (source == NULL) {
       _XmWarning(widget, MESSAGE2);
       return;
    }

   /* zero out old line table */
    block.ptr = NULL;
    block.length = 0;
    _XmTextUpdateLineTable(widget, 0, 0, &block, False);
#ifdef OSF_v1_2_4
    tw->text.total_lines = 1;
#endif /* OSF_v1_2_4 */

    (*tw->text.source->RemoveWidget)(tw->text.source, tw);
    tw->text.source = source;
 
    /*
     * Fix for 3669 - Check to ensure that the input cursor_position
     *                 is within acceptable bounds.
     */

    if (cursor_position > source->data->length)
      cursor_position = source->data->length;
    else if (cursor_position < 0)
      cursor_position = 0;

    /* End Fix 3669 */
 
    tw->text.cursor_position = cursor_position;
    _XmTextMovingCursorPosition(tw, cursor_position); /*correct GC for
						       * new location */
    _XmTextResetClipOrigin(tw, cursor_position, False);/* move clip origin */
    tw->text.output->data->refresh_ibeam_off = True;
    (*tw->text.source->AddWidget)(tw->text.source, tw);
    _XmStringSourceSetGappedBuffer(source->data, cursor_position);
    top_character = (*tw->text.source->Scan)(tw->text.source, top_character,
					   XmSELECT_LINE, XmsdLeft, 1, FALSE);
    tw->text.new_top = top_character;
    tw->text.top_character = 0;

   /* reset line table with new source */
    last_pos = (XmTextPosition) source->data->length;
    while (pos < last_pos) {
        pos = (*tw->text.source->ReadSource)(source, pos, last_pos, &block);
        if (block.length == 0)
           break;
        _XmTextUpdateLineTable(widget, old_pos, old_pos, &block, False);
        old_pos = pos;
    }

#ifndef OSF_v1_2_4
    _XmTextInvalidate(tw, top_character, top_character, NODELTA);
#else /* OSF_v1_2_4 */
    _XmTextInvalidate(tw, top_character, top_character, LONG_MAX);
#endif /* OSF_v1_2_4 */
    if (tw->text.disable_depth == 0)
	Redisplay(tw);

   /* Tell the input method the new x,y location of the cursor */
    (*tw->text.output->PosToXY)(tw, cursor_position, &xmim_point.x,
				&xmim_point.y);
    n = 0;
    XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
    XmImSetValues((Widget)tw, args, n);

    TextDrawInsertionPoint(tw);

}


/*
 * Force the given position to be displayed.  If position < 0, then don't force
 * any position to be displayed.
 */
/* ARGSUSED */
void 
#ifdef _NO_PROTO
XmTextShowPosition( widget, position )
        Widget widget ;
        XmTextPosition position ;
#else
XmTextShowPosition(
        Widget widget,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      XmTextFieldShowPosition(widget, position);
   else {
      XmTextWidget tw = (XmTextWidget) widget;
    
      if (!tw->text.needs_refigure_lines && (position < 0 ||
		  (position >= tw->text.top_character &&
			   position < tw->text.bottom_position))) {
	  (*tw->text.output->MakePositionVisible)(tw, position);
	  return;
      }
      tw->text.force_display = position;
      tw->text.needs_refigure_lines = tw->text.needs_redisplay = TRUE;
      if (tw->text.disable_depth == 0) Redisplay(tw);
   }
}

void 
#ifdef _NO_PROTO
XmTextScroll( widget, n )
        Widget widget ;
        int n ;
#else
XmTextScroll(
        Widget widget,
        int n )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) widget;

    tw->text.pending_scroll += n;
    tw->text.needs_refigure_lines = tw->text.needs_redisplay = TRUE;

    if (tw->text.disable_depth == 0) Redisplay(tw);
}

int 
#ifdef _NO_PROTO
XmTextGetBaseline( widget )
        Widget widget ;
#else
XmTextGetBaseline(
        Widget widget )
#endif /* _NO_PROTO */
{
   if (XmIsTextField(widget))
      return(XmTextFieldGetBaseline(widget));
   else {
      Dimension *baselines;
      int temp_bl;
      int line_count;

      (void) _XmTextGetBaselines(widget, &baselines, &line_count);

      if (line_count)
         temp_bl = (int) baselines[0];
      else
	 temp_bl = 0;

      XtFree((char *) baselines);
      return (temp_bl);
   }
   
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmTextDisableRedisplay( widget, losesbackingstore )
        XmTextWidget widget ;
        Boolean losesbackingstore ;
#else
_XmTextDisableRedisplay(
        XmTextWidget widget,
#if NeedWidePrototypes
        int losesbackingstore )
#else
        Boolean losesbackingstore )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    widget->text.disable_depth++;
    EraseInsertionPoint(widget);
}

void 
#ifdef _NO_PROTO
_XmTextEnableRedisplay( widget )
        XmTextWidget widget ;
#else
_XmTextEnableRedisplay(
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    if (widget->text.disable_depth) widget->text.disable_depth--;
    if (widget->text.disable_depth == 0 && widget->text.needs_redisplay)
	Redisplay(widget);

    /* If this is a scrolled widget, better update the scroll bars to reflect
     * any changes that have occured while redisplay has been disabled.  */

    if (widget->text.disable_depth == 0){
       if (widget->text.output->data->scrollvertical &&
                XtClass(widget->core.parent) == xmScrolledWindowWidgetClass &&
                !widget->text.vsbar_scrolling)
          _XmChangeVSB(widget);
       if (widget->text.output->data->scrollhorizontal &&
		XtClass(widget->core.parent) == xmScrolledWindowWidgetClass)
          _XmRedisplayHBar(widget);
    }

    TextDrawInsertionPoint(widget);
}

void 
#ifdef _NO_PROTO
XmTextDisableRedisplay( widget )
        Widget widget ;
#else
XmTextDisableRedisplay(
        Widget widget)
#endif /* _NO_PROTO */
{
   _XmTextDisableRedisplay((XmTextWidget)widget, False);
}

void 
#ifdef _NO_PROTO
XmTextEnableRedisplay( widget )
        Widget widget ;
#else
XmTextEnableRedisplay(
        Widget widget )
#endif /* _NO_PROTO */
{
    _XmTextEnableRedisplay((XmTextWidget)widget);
}

Boolean
#ifdef _NO_PROTO
XmTextFindString( w, start, search_string, direction, position)
        Widget w;
        XmTextPosition start;
        char *search_string;
        XmTextDirection direction;
        XmTextPosition *position;
#else
XmTextFindString(
        Widget w,
        XmTextPosition start,
        char* search_string,
        XmTextDirection direction,
        XmTextPosition *position)
#endif /* _NO_PROTO */
{
    XmSourceData data;
    if (XmIsTextField(w)) return False;

    data = ((XmTextWidget)w)->text.source->data;

    if (start > data->length)
       start = data->length;
    else if (start < 0)
       start = 0;

    if (direction == XmTEXT_BACKWARD)
       return (_XmTextFindStringBackwards(w, start, search_string, position));
    else
       return (_XmTextFindStringForwards(w, start, search_string, position));

}

Boolean
#ifdef _NO_PROTO
XmTextFindStringWcs( w, start, wc_string, direction, position)
        Widget w;
        XmTextPosition start;
        wchar_t *wc_string;
        XmTextDirection direction;
        XmTextPosition *position;
#else
XmTextFindStringWcs(
        Widget w,
        XmTextPosition start,
        wchar_t* wc_string,
        XmTextDirection direction,
        XmTextPosition *position)
#endif /* _NO_PROTO */
{
   wchar_t *tmp_wc;
   char *string;
   int num_chars = 0;
   Boolean return_val = False;
   XmTextWidget tw = (XmTextWidget) w;
   int wcs_ret_val = 0;

   if (!XmIsTextField(w)) {
      for (num_chars = 0, tmp_wc = wc_string; *tmp_wc != (wchar_t)0L;
           num_chars++) tmp_wc++;
      string = XtMalloc ((unsigned) (num_chars + 1) * (int)tw->text.char_size);
      wcs_ret_val = wcstombs(string, wc_string,
                      (num_chars + 1) * (int)tw->text.char_size);
      if (wcs_ret_val >= 0)
         return_val = XmTextFindString( w, start, string, direction, position);
      XtFree(string);
      return(return_val);
   } else
      return False;
}
