#pragma ident	"@(#)m1.2libs:Xm/ToggleB.c	1.7"
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
/*
 * Include files & Static Routine Definitions
 */
#include <stdio.h>

#include "XmI.h"
#include <X11/ShellP.h>
#include "RepTypeI.h"
#include <Xm/BaseClassP.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawP.h>
#include <Xm/LabelP.h>
#include <Xm/ManagerP.h>
#include <Xm/MenuUtilP.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleBP.h>
#include <Xm/TransltnsP.h>
#include "TravActI.h"

#define XmINVALID_TYPE  255	/* dynamic default flag for IndicatorType */
#define XmINVALID_BOOLEAN 85    /* dynamic default flag for VisibleWhenOff */

#define PixmapOn(w)            ((w)->toggle.on_pixmap)
#define PixmapOff(w)           ((w)->label.pixmap)
#define Pixmap_Insen_On(w)     ((w)->toggle.insen_pixmap)
#define Pixmap_Insen_Off(w)    ((w)->label.pixmap_insen)
#define IsNull(p)              (p == XmUNSPECIFIED_PIXMAP)
#define IsOn(w)                ((w)->toggle.visual_set)
#define IsArmed(w)             ((w)->toggle.Armed)

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void InitializePrehook() ;
static void InitializePosthook() ;
static void SetAndDisplayPixmap() ;
static void Help() ;
static void ToggleButtonCallback() ;
static void Leave() ;
static void Enter() ;
static void Arm() ;
static void Select() ;
static void Disarm() ;
static void ArmAndActivate() ;
static void BtnDown() ;
static void BtnUp() ;
static void GetGC() ;
static void Initialize() ;
static void Destroy() ;
static void DrawToggle() ;
static void BorderHighlight() ;
static void BorderUnhighlight() ;
static void KeySelect() ;
static void ComputeSpace() ;
static void Redisplay() ;
static void Resize() ;
static Boolean SetValues() ;
static void DrawToggleShadow() ;
static void DrawToggleLabel() ;
static void SetToggleSize() ;
#ifdef CDE_VISUAL	/* toggle button indicator visual */
static void DrawRadio();
#endif

#else

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
static void SetAndDisplayPixmap( 
                        XmToggleButtonWidget tb,
                        XEvent *event,
                        Region region) ;
static void Help( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ToggleButtonCallback( 
                        XmToggleButtonWidget data,
                        unsigned int reason,
                        unsigned int value,
                        XEvent *event) ;
static void Leave( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Enter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Arm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Select( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Disarm( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ArmAndActivate( 
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
static void GetGC( 
                        XmToggleButtonWidget tw) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget wid) ;
static void DrawToggle( 
                        XmToggleButtonWidget w) ;
static void BorderHighlight( 
                        Widget wid) ;
static void BorderUnhighlight( 
                        Widget wid) ;
static void KeySelect( 
                        Widget wid,
                        XEvent *event,
                        String *param,
                        Cardinal *num_param) ;
static void ComputeSpace( 
                        XmToggleButtonWidget tb) ;
static void Redisplay( 
                        Widget w,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget w) ;
static Boolean SetValues( 
                        Widget current,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void DrawToggleShadow( 
                        XmToggleButtonWidget tb) ;
static void DrawToggleLabel( 
                        XmToggleButtonWidget tb) ;
static void SetToggleSize( 
                        XmToggleButtonWidget newtb) ;
#ifdef CDE_VISUAL	/* toggle button indicator visual */
static void DrawRadio(
		      XmToggleButtonWidget w,
		      int x,
		      int y,
		      int length);
#endif

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/



/*************************************<->*************************************
 *
 *
 *   Description:  default translation table for class: ToggleButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/
static XtTranslations default_parsed;

#define defaultTranslations	_XmToggleB_defaultTranslations

static XtTranslations menu_parsed;

#define menuTranslations	_XmToggleB_menuTranslations

/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: ToggleButton
 *   -----------
 *
 *   Matches string descriptors with internal routines.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"Arm", 	     Arm            },
  {"ArmAndActivate", ArmAndActivate },
  {"Disarm", 	     Disarm         },
  {"Select", 	     Select         },
  {"Enter", 	     Enter          },
  {"Leave", 	     Leave          },
  {"BtnDown",        BtnDown        },
  {"BtnUp",          BtnUp          },
  {"KeySelect",      KeySelect      },
  {"Help",           Help},
};




/*************************************<->*************************************
 *
 *
 *   Description:  resource list for class: ToggleButton
 *   -----------
 *
 *   Provides default resource settings for instances of this class.
 *   To get full set of default settings, examine resouce list of super
 *   classes of this class.
 *
 *************************************<->***********************************/

static XtResource resources[] = 
{
   {
     XmNindicatorSize, 
     XmCIndicatorSize, 
     XmRVerticalDimension, 
     sizeof(Dimension),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.indicator_dim),
     XmRImmediate, (XtPointer) XmINVALID_DIMENSION
   },

   {
     XmNindicatorType, XmCIndicatorType, XmRIndicatorType,sizeof(unsigned char),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.ind_type),
     XmRImmediate, (XtPointer) XmINVALID_TYPE
   },

   {
     XmNvisibleWhenOff, XmCVisibleWhenOff, XmRBoolean, sizeof(Boolean),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.visible),
     XmRImmediate, (XtPointer) XmINVALID_BOOLEAN
   },

   {
     XmNspacing, 
     XmCSpacing, 
     XmRHorizontalDimension, 
     sizeof(Dimension),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.spacing),
     XmRImmediate, (XtPointer) 4
   },

   {
     XmNselectPixmap, XmCSelectPixmap, XmRPrimForegroundPixmap, sizeof(Pixmap),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.on_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP 
   },

   {
     XmNselectInsensitivePixmap, XmCSelectInsensitivePixmap, XmRPrimForegroundPixmap,
     sizeof(Pixmap),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.insen_pixmap),
     XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXMAP
   },

   {
     XmNset, XmCSet, XmRBoolean, sizeof(Boolean),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.set),
     XmRImmediate, (XtPointer) False
   },

   {
      XmNindicatorOn, XmCIndicatorOn, XmRBoolean, sizeof (Boolean),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.ind_on),
      XmRImmediate, (XtPointer) True
   },

   {
      XmNfillOnSelect, XmCFillOnSelect, XmRBoolean, sizeof (Boolean),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.fill_on_select),
      XmRImmediate, (XtPointer) XmINVALID_BOOLEAN
   },

   {
      XmNselectColor, XmCSelectColor, XmRPixel, sizeof (Pixel),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.select_color),
      XmRCallProc, (XtPointer) _XmSelectColorDefault
   },

   {
      XmNvalueChangedCallback, XmCValueChangedCallback, XmRCallback,
      sizeof (XtCallbackList),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.value_changed_CB),
      XmRPointer, (XtPointer)NULL 
   },

   {
      XmNarmCallback, XmCArmCallback, XmRCallback,
      sizeof (XtCallbackList),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.arm_CB),
      XmRPointer, (XtPointer)NULL 
   },

   {
      XmNdisarmCallback, XmCDisarmCallback, XmRCallback,
      sizeof (XtCallbackList),
      XtOffsetOf( struct _XmToggleButtonRec, toggle.disarm_CB),
      XmRPointer, (XtPointer)NULL 
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

/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
   { XmNspacing,
     sizeof (Dimension),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.spacing),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNindicatorSize,
     sizeof (Dimension),
     XtOffsetOf( struct _XmToggleButtonRec, toggle.indicator_dim),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

};

/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: ToggleButton
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/
static XmBaseClassExtRec       toggleBBaseClassExtRec = {
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
    XmInheritGetSecResData,                   /* getSecResData */
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

XmPrimitiveClassExtRec _XmToggleBPrimClassExtRec = {
     NULL,
     NULLQUARK,
     XmPrimitiveClassExtVersion,
     sizeof(XmPrimitiveClassExtRec),
     XmInheritBaselineProc,                  /* widget_baseline */
     XmInheritDisplayRectProc,               /* widget_display_rect */
     (XmWidgetMarginsProc)NULL,              /* widget_margins */
};

externaldef(xmtogglebuttonclassrec) 
	XmToggleButtonClassRec xmToggleButtonClassRec = {
   {
    /* superclass	  */	(WidgetClass) &xmLabelClassRec,
    /* class_name	  */	"XmToggleButton",
    /* widget_size	  */	sizeof(XmToggleButtonRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    ClassPartInitialize,				
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */    (XtArgsProc)NULL,
    /* realize		  */	XmInheritRealize,
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
    /* accept_focus       */    (XtAcceptFocusProc)NULL,
    /* version            */	XtVersion,
    /* callback_private   */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry     */	XtInheritQueryGeometry, 
    /* display_accelerator */   (XtStringProc)NULL,
    /* extension record   */    (XtPointer)&toggleBBaseClassExtRec,
   },

   {
    /* Primitive border_highlight   */	BorderHighlight,
    /* Primitive border_unhighlight */  BorderUnhighlight,
    /* translations                 */ 	XtInheritTranslations,
										/* (XtTranslations) _XtInherit, */
    /* arm_and_activate             */  ArmAndActivate,
    /* syn resources                */  syn_resources,         
    /* num syn_resources            */  XtNumber(syn_resources),    
    /* extension                    */  (XtPointer)&_XmToggleBPrimClassExtRec,
   },

   {
    /* SetOverrideCallback     */    XmInheritWidgetProc,
    /* menu procedures    */	     XmInheritMenuProc,
    /* menu traversal xlation  */    XtInheritTranslations,
    /* extension               */    NULL,
   },

   {
    /* extension               */    (XtPointer) NULL,
   }
};

externaldef(xmtogglebuttonwidgetclass)
   WidgetClass xmToggleButtonWidgetClass = (WidgetClass)&xmToggleButtonClassRec;

#ifdef CDE_VISUAL       /* etched in menu button */

typedef struct {
    GC          arm_GC;
    GC          normal_GC;
} tb_extension_type;

static XContext extension_context ;
static Widget cache_w ;
static tb_extension_type        *tb_extension;

static void
#ifdef _NO_PROTO
InitExtension( tb )
        XmToggleButtonWidget tb;
#else
InitExtension(
        XmToggleButtonWidget tb)
#endif /* _NO_PROTO */
{
    XGCValues       values;
    XtGCMask        valueMask;
    short           myindex;
    XFontStruct     *fs;
    Pixel           select_pixel;
 
    tb_extension = (tb_extension_type *) XtMalloc(sizeof(tb_extension_type));
    XSaveContext( XtDisplay(tb), (XID) tb, extension_context,
                 (XPointer) tb_extension) ;
    cache_w = (Widget) tb;
 
    XmGetColors(XtScreen(tb), tb->core.colormap, tb->core.background_pixel,
                NULL, NULL, NULL, &select_pixel);
 
    valueMask = GCForeground | GCBackground | GCGraphicsExposures;
    values.foreground = tb->core.background_pixel;
    values.background = tb->primitive.foreground;
    values.graphics_exposures = False;
    tb_extension->normal_GC = XtGetGC( (Widget) tb,valueMask,&values);
 
    valueMask = GCForeground | GCBackground | GCGraphicsExposures;
    values.foreground = select_pixel;
    values.background = tb->primitive.foreground;
    values.graphics_exposures = False;
    tb_extension->arm_GC = XtGetGC( (Widget) tb,valueMask,&values);
}
 
static tb_extension_type *
#ifdef _NO_PROTO
GetExtension( w)
    Widget w;
#else
GetExtension(Widget  w)
#endif /* _NO_PROTO */
{
    if ( w != cache_w )
        if( XFindContext( XtDisplay(w), (XID)w, extension_context,
                         (XPointer *) &tb_extension)    )
            cache_w = NULL ;
        else
            cache_w = w ;       /* success */
    return tb_extension ;
}
 
static void
#ifdef _NO_PROTO
FreeExtension(w)
    Widget w;
#else
FreeExtension(Widget w)
#endif
{
    GetExtension(w);
    if (tb_extension) {
        XtFree((char *)tb_extension);
        cache_w = NULL;
    }
    XDeleteContext( XtDisplay(w), (XID)w, extension_context) ;
}
 
#endif /* CDE_VISUAL */

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

   default_parsed       = XtParseTranslationTable(defaultTranslations);
   menu_parsed          = XtParseTranslationTable(menuTranslations);

   /* set up base class extension quark */
   toggleBBaseClassExtRec.record_type = XmQmotif;

#ifdef CDE_VISUAL
   extension_context = XUniqueContext();
#endif
}

/*****************************************************************************
 *
 * ClassPartInitialize
 *   Set up fast subclassing for the widget.
 *
 ****************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
  _XmFastSubclassInit (wc, XmTOGGLE_BUTTON_BIT);
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


/*********************************************************************
 *
 * redisplayPixmap
 *   does the apropriate calculations based on the toggle button's
 *   current pixmap and calls label's Redisplay routine.
 *
 * This routine was added to fix CR 4839 and CR 4838
 * D. Rand 7/6/92
 * 
 ***********************************************************************/

static void
#ifdef _NO_PROTO
redisplayPixmap(tb, event, region)
     XmToggleButtonWidget tb;
     XEvent *event;
     Region region;
#else
redisplayPixmap(XmToggleButtonWidget tb, XEvent *event, Region region)
#endif
{
  Pixmap todo;
#ifndef OSF_v1_2_4
  unsigned int onH = 0, onW = 0, junk, d;
  int w, h;
#else /* OSF_v1_2_4 */
  Window root;
  int rx, ry;
  unsigned int onH = 0, onW = 0, border, d;
  unsigned int w, h;
#endif /* OSF_v1_2_4 */
  int x, y, offset;
  short saveY;
  unsigned short saveWidth, saveHeight;
  
  offset = tb -> primitive.highlight_thickness +
    tb -> primitive.shadow_thickness;

  x = offset + tb -> label.margin_width + tb -> label.margin_left;

  y = offset + tb -> label.margin_height + tb -> label.margin_top;

  w = XtWidth(tb) - x - offset - tb -> label.margin_right
    - tb -> label.margin_width;

#ifndef OSF_v1_2_4
  w = Max(0, w);
#else /* OSF_v1_2_4 */
  w = Max(0, (int)w);
#endif /* OSF_v1_2_4 */

  h = XtHeight(tb) - y - offset - tb -> label.margin_bottom
    - tb -> label.margin_height;

#ifndef OSF_v1_2_4
  h = Max(0, h);
#else /* OSF_v1_2_4 */
  h = Max(0, (int)h);
#endif /* OSF_v1_2_4 */

  XClearArea(XtDisplay(tb), XtWindow(tb), x, y, w, h, False);

  todo = tb -> label.pixmap;

#ifndef OSF_v1_2_4
  if ( (! tb -> core.sensitive) && tb -> label.pixmap_insen )
#else /* OSF_v1_2_4 */
  if ( (! XtIsSensitive((Widget) tb)) && tb -> label.pixmap_insen )
#endif /* OSF_v1_2_4 */
    todo = tb -> label.pixmap_insen;
      
  if ( ! IsNull(todo) )
    XGetGeometry (XtDisplay(tb),
		  todo,
#ifndef OSF_v1_2_4
		  (Window*)&junk, /* returned root window */
		  (int*)&junk, (int*)&junk, /* x, y of pixmap */
		  &onW, &onH, /* width, height of pixmap */
		  &junk,    /* border width */
		  &d);      /* depth */
#else /* OSF_v1_2_4 */
		  &root,	/* returned root window */
		  &rx, &ry,	/* returned x, y of pixmap */
		  &onW, &onH,	/* returned width, height of pixmap */
		  &border,	/* returned border width */
		  &d);		/* returned depth */
#endif /* OSF_v1_2_4 */

  saveY = Lab_TextRect_y(tb);
  saveWidth = Lab_TextRect_width(tb);
  saveHeight = Lab_TextRect_height(tb);

  h = (XtHeight(tb) - onH) / 2;
#ifndef OSF_v1_2_4
  Lab_TextRect_y(tb) = Max(0, h);
#else /* OSF_v1_2_4 */
  Lab_TextRect_y(tb) = Max(0, (int)h);
#endif /* OSF_v1_2_4 */
  Lab_TextRect_height(tb) = onH;
  Lab_TextRect_width(tb) = onW;
  (* xmLabelClassRec.core_class.expose) ((Widget) tb, event, region);

  Lab_TextRect_y(tb) = saveY;
  Lab_TextRect_width(tb) = saveWidth;
  Lab_TextRect_height(tb) = saveHeight;
}

/***********************************************************************
 *
 * SetAndDisplayPixmap
 *   Sets the appropriate on, off pixmap in label's pixmap field and
 *   calls redisplayPixmap
 *
 ***********************************************************************/
static void 
#ifdef _NO_PROTO
SetAndDisplayPixmap( tb, event, region )
        XmToggleButtonWidget tb ;
        XEvent *event ;
        Region region ;
#else
SetAndDisplayPixmap(
        XmToggleButtonWidget tb,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
 if (IsOn (tb))
 {
#ifndef OSF_v1_2_4
   if ((tb->core.sensitive) && (tb->core.ancestor_sensitive))
#else /* OSF_v1_2_4 */
   if (XtIsSensitive((Widget) tb))
#endif /* OSF_v1_2_4 */
   {
     if ( ! IsNull (PixmapOn (tb)))
     {
       Pixmap tempPix;

       tempPix = PixmapOff(tb);
       PixmapOff(tb) = PixmapOn(tb);
       redisplayPixmap(tb, event, region);
       PixmapOff(tb) = tempPix;
     }
     else
       redisplayPixmap(tb, event, region);
   }
   else
   {
     if ( ! IsNull (Pixmap_Insen_On (tb)))
     {
       Pixmap tempPix;

       tempPix = Pixmap_Insen_Off(tb);
       Pixmap_Insen_Off(tb) = Pixmap_Insen_On(tb);
       redisplayPixmap(tb, event, region);
       Pixmap_Insen_Off(tb) = tempPix;
     }
     else
       redisplayPixmap(tb, event, region);
   }
 }
 else
   redisplayPixmap(tb, event, region);
}

/*************************************************************************
 *
 *  Help
 *     This routine is called if the user has made a help selection
 *     on the widget.
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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   Boolean is_menupane = (tb -> label.menu_type == XmMENU_PULLDOWN) ||
			 (tb -> label.menu_type == XmMENU_POPUP);

   if (is_menupane)
   {
      (* xmLabelClassRec.label_class.menuProcs)
	  (XmMENU_BUTTON_POPDOWN, XtParent(tb), NULL, event, NULL);
   }

   _XmPrimitiveHelp( (Widget) tb, event, params, num_params);

   if (is_menupane)
   {
      (* xmLabelClassRec.label_class.menuProcs)
	 (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	 XtParent(tb), NULL, event, NULL);
   }
}


/*************************************************************************
 *
 * ToggleButtonCallback
 *   This is the widget's application callback routine
 *
 *************************************************************************/
static void 
#ifdef _NO_PROTO
ToggleButtonCallback( data, reason, value, event )
        XmToggleButtonWidget data ;
        unsigned int reason ;
        unsigned int value ;
        XEvent *event ;
#else
ToggleButtonCallback(
        XmToggleButtonWidget data,
        unsigned int reason,
        unsigned int value,
        XEvent *event )
#endif /* _NO_PROTO */
{

    XmToggleButtonCallbackStruct temp;

    temp.reason = reason;
    temp.set= value;
    temp.event  = event;

    switch (reason)
      {
        case XmCR_VALUE_CHANGED:
            XtCallCallbackList ((Widget) data, data->toggle.value_changed_CB, &temp);
            break;

        case XmCR_ARM          :
            XtCallCallbackList ((Widget) data, data->toggle.arm_CB, &temp);
            break;

        case XmCR_DISARM       :
            XtCallCallbackList ((Widget) data, data->toggle.disarm_CB, &temp);
            break;

       }

}


/**************************************************************************
 *
 * Leave
 *  This procedure is called when  the mouse button is pressed and  the
 *  cursor moves out of the widget's window. This procedure is used
 *  to change the visuals.
 *
*************************************************************************/
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
   XmToggleButtonWidget w = (XmToggleButtonWidget) wid;
   int edge, x, y;
   int fill;

   if (w -> label.menu_type == XmMENU_PULLDOWN ||
       w -> label.menu_type == XmMENU_POPUP)
   {
      if (_XmGetInDragMode((Widget)w) && w->toggle.Armed &&
	  (/* !ActiveTearOff || */ event->xcrossing.mode == NotifyNormal))
      {
	 w -> toggle.Armed = FALSE;
#ifdef CDE_VISUAL       /* etched in menu button */
          {
              Boolean etched_in = False;
              XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableEtchedInMenu", &etched_in, NULL);
              if (etched_in)
              {
                DrawToggleLabel(w);
		if (w->toggle.ind_on)
                    DrawToggle(w);
              }
          }
#endif
	 _XmClearBorder (XtDisplay (w), XtWindow (w),
		 w-> primitive.highlight_thickness,
		 w-> primitive.highlight_thickness,
		 w-> core.width - 2 * w->primitive.highlight_thickness,
		 w-> core.height - 2 * w->primitive.highlight_thickness,
		 w-> primitive.shadow_thickness);


	 if (w->toggle.disarm_CB)
	 {
	    XFlush (XtDisplay (w));
	    ToggleButtonCallback(w, XmCR_DISARM, w->toggle.set, event);
	 }
      }
   }

   else
   {  _XmPrimitiveLeave( (Widget) w,  event, params, num_params);
#ifdef OSF_v1_2_4

      /* CR 8020: We may have armed while outside the toggle. */
      IsOn(w) = w->toggle.set;

#endif /* OSF_v1_2_4 */
      if( w->toggle.indicator_set || _XmStringEmpty(w->label._label) ) {
	edge = w->toggle.indicator_dim;
      } else {
	edge = Min((int)w->toggle.indicator_dim, 
		   Max(0, (int)w->core.height -
		       2*(w->primitive.highlight_thickness +
			  w->primitive.shadow_thickness +
			  (int)w->label.margin_height) +
		       w->label.margin_top +
		       w->label.margin_bottom));
      }

      if (DefaultDepthOfScreen (XtScreen (w)) == 1) /* Monochrome Display */
        fill = FALSE;
      else
      {
        if ((w->primitive.top_shadow_color != w->toggle.select_color) &&
            (w->primitive.bottom_shadow_color != w->toggle.select_color))
              fill = TRUE;
        else
              fill = FALSE;
      }

      x = w->primitive.highlight_thickness + w->primitive.shadow_thickness +
	  w->label.margin_width; 

      if ( w->toggle.indicator_set || _XmStringEmpty(w->label._label) )
	y = (w->core.height - w->toggle.indicator_dim)/2;
      else
      {
        y = w->label.TextRect.y;
        if (w->label.menu_type == XmMENU_POPUP ||
            w->label.menu_type == XmMENU_PULLDOWN)
          y += (w->toggle.indicator_dim + 2) / 4; /* adjust in menu */
      }
 
      if ((w->toggle.ind_type) == XmN_OF_MANY)
      {
	 if (w->toggle.Armed == TRUE)
	 { 
#ifndef OSF_v1_2_4
	    if (IsOn(w) == TRUE)
		IsOn(w) = FALSE;
	    else
		IsOn(w) = TRUE;
#endif /* OSF_v1_2_4 */
	    if (w->toggle.ind_on)
	    {
	       /* if the toggle indicator is square shaped then adjust the
		  indicator width and height, so that it looks proportional
		  to a diamond shaped indicator of the same width and height */

	       int new_edge;
	       new_edge = edge - 3 - ((edge - 10)/10);
	                                      /* Subtract 3 pixels + 1  */
                                              /* pixel for every 10 pixels, */
                                              /* from the width and height. */

	       /* Adjust x,y so that the indicator is centered relative
		  to the label*/
	       y = y + ((edge - new_edge) / 2);
	       x = x + ((edge - new_edge) / 2);

	       edge = new_edge;

	       if ((w->toggle.visible) ||
		   ((!w->toggle.visible) && (IsOn(w))))
	       {
		  _XmDrawShadows (XtDisplay (w), XtWindow (w), 
                            ((IsOn(w)) ? 
                               w -> primitive.bottom_shadow_GC :
                               w -> primitive.top_shadow_GC),
                            ((IsOn(w)) ? 
                               w -> primitive.top_shadow_GC :
                               w -> primitive.bottom_shadow_GC), 
                             x, y, edge, edge, 2, XmSHADOW_OUT);

    
		  if (w->toggle.fill_on_select)
		      if (edge > (fill ? 4 : 6))
			  XFillRectangle (XtDisplay ((Widget) w), 
					  XtWindow ((Widget) w),
					 ((IsOn(w)) ?
					    w->toggle.select_GC :
					    w -> toggle.background_gc),
					  ((fill) ? x+2 : x+3),
					  ((fill) ? y+2 : y+3),
					  ((fill) ? edge-4 : edge-6),
					  ((fill) ? edge-4 : edge-6));
	       }

	       if (!w->toggle.visible)
	       {
		  if (!IsOn(w))
		      if (edge > 0)
			  XFillRectangle( XtDisplay ((Widget) w),
                                              XtWindow ((Widget) w),
                                              w->toggle.background_gc,
                                              x, y, edge, edge);
	       }
	    }     
	    else
	    {
               if (w->primitive.shadow_thickness > 0)
                 DrawToggleShadow (w);
               if (w->toggle.fill_on_select && !Lab_IsPixmap(w))
                 DrawToggleLabel (w);
	    }
	    if (Lab_IsPixmap(w))
	    {
	       SetAndDisplayPixmap( w, event, NULL);
	    }
	 }
      }
      else
      {
	 if (w->toggle.Armed == TRUE)
	 { 
#ifndef OSF_v1_2_4
	    if (IsOn(w) == TRUE)
		IsOn(w) = FALSE;
	    else
		IsOn(w) = TRUE;
  
#endif /* OSF_v1_2_4 */
	    if (w->toggle.ind_on)
	    {
	       if ((w->toggle.visible) ||
		   ((!w->toggle.visible) && (IsOn(w))))
#ifdef CDE_VISUAL	/* toggle button indicator visual */
	       {
		   Boolean toggle_visual = False;
		   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)w)),
				 "enableToggleVisual", &toggle_visual, NULL);
		   if (toggle_visual)
		       DrawRadio(w, x, y, edge);
		   else
		       _XmDrawDiamond(XtDisplay(w), XtWindow((Widget) w),
				      ((IsOn(w)) ?
				       w -> primitive.bottom_shadow_GC :
				       w -> primitive.top_shadow_GC),
				      ((IsOn(w)) ?
				       w -> primitive.top_shadow_GC :
				       w -> primitive.bottom_shadow_GC),
				      (((IsOn(w))  &&
					(w->toggle.fill_on_select)) ?
				       w -> toggle.select_GC :
				       w -> toggle.background_gc),
				      x, y, edge, edge,
				      w->primitive.shadow_thickness, fill);
	       }
#else
		   _XmDrawDiamond( XtDisplay(w), 
				       XtWindow(w),
					 ((IsOn(w)) ?
					    w -> primitive.bottom_shadow_GC :
					    w -> primitive.top_shadow_GC),
					 ((IsOn(w)) ?
					    w -> primitive.top_shadow_GC :
					    w -> primitive.bottom_shadow_GC),
					 (((IsOn(w))  &&
					   (w->toggle.fill_on_select)) ?
					    w -> toggle.select_GC :
					    w -> toggle.background_gc),
					  x, y, edge, edge,
					  w->primitive.shadow_thickness, fill);
#endif
      
	       if (!w->toggle.visible)
	       {
		  if (!IsOn(w))
		      if (edge > 0)
			  XFillRectangle( XtDisplay ((Widget) w),
                                             XtWindow ((Widget) w),
                                             w->toggle.background_gc,
                                             x, y, edge, edge);
	       }
	    }
	    else
	    {
               if (w->primitive.shadow_thickness > 0)
                 DrawToggleShadow (w);
               if (w->toggle.fill_on_select && !Lab_IsPixmap(w))
                 DrawToggleLabel (w);
	    }
	    if (Lab_IsPixmap(w))
	    {
	       SetAndDisplayPixmap( w, event, NULL);
	    }
	 }
      }
   }
}

/**************************************************************************
 *
 * Enter
 *   This procedure is called when the mouse button is pressed and the
 *   cursor reenters the widget's window. This procedure changes the visuals
 *   accordingly.
 *
 **************************************************************************/
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
   XmToggleButtonWidget w = (XmToggleButtonWidget) wid ;
   int edge, x, y;
   int  fill;

   if (w -> label.menu_type == XmMENU_PULLDOWN ||
       w -> label.menu_type == XmMENU_POPUP)
   {
      if ((((ShellWidget) XtParent(XtParent(w)))->shell.popped_up) &&
	  _XmGetInDragMode((Widget)w))
      {
	 if (w->toggle.Armed)
	    return;

	 /* So KHelp event is delivered correctly */
	 _XmSetFocusFlag( XtParent(XtParent(w)), XmFOCUS_IGNORE, TRUE);
	 XtSetKeyboardFocus(XtParent(XtParent(w)), (Widget)w);
	 _XmSetFocusFlag( XtParent(XtParent(w)), XmFOCUS_IGNORE, FALSE);

#ifdef CDE_VISUAL	/* etched in menu button */
	 w -> toggle.Armed = TRUE; 
          {
              Boolean etched_in = False;
              XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableEtchedInMenu", &etched_in, NULL);
	      if (etched_in) 
	      {
		DrawToggleLabel(w);
		if (w->toggle.ind_on)
		    DrawToggle(w);
	      }
	      _XmDrawShadows (XtDisplay (w), XtWindow (w),
		w -> primitive.top_shadow_GC,
		w -> primitive.bottom_shadow_GC,
		w -> primitive.highlight_thickness,
		w -> primitive.highlight_thickness,
		w -> core.width - 2 * w->primitive.highlight_thickness,
		w -> core.height - 2 * w->primitive.highlight_thickness,
		w -> primitive.shadow_thickness,
                etched_in ? XmSHADOW_IN : XmSHADOW_OUT);
          }
#else
	  _XmDrawShadows (XtDisplay (w), XtWindow (w),
		w -> primitive.top_shadow_GC,
		w -> primitive.bottom_shadow_GC,
		w -> primitive.highlight_thickness,
		w -> primitive.highlight_thickness,
		w -> core.width - 2 * w->primitive.highlight_thickness,
		w -> core.height - 2 * w->primitive.highlight_thickness,
		w -> primitive.shadow_thickness, XmSHADOW_OUT);
	 w -> toggle.Armed = TRUE; 
#endif

	 if (w->toggle.arm_CB)
	 {
	    XFlush (XtDisplay (w));
	    ToggleButtonCallback(w, XmCR_ARM, w->toggle.set, event);
	 }
      }
   }
   else
   {
      _XmPrimitiveEnter( (Widget) w, event, params, num_params);
#ifdef OSF_v1_2_4

      /* CR 7301: We may have armed while outside the toggle. */
      IsOn(w) = !w->toggle.set;

#endif /* OSF_v1_2_4 */
      if( w->toggle.indicator_set || _XmStringEmpty(w->label._label) ) {
	edge = w->toggle.indicator_dim;
      } else {
	edge = Min((int)w->toggle.indicator_dim, 
		   Max(0, (int)w->core.height -
		       2*(w->primitive.highlight_thickness +
			  w->primitive.shadow_thickness +
			  (int)w->label.margin_height) +
		       w->label.margin_top +
		       w->label.margin_bottom));
      }

      if (DefaultDepthOfScreen (XtScreen (w)) == 1) /* Monochrome Display */
        fill = FALSE;
      else
      {
        if ((w->primitive.top_shadow_color != w->toggle.select_color) &&
            (w->primitive.bottom_shadow_color != w->toggle.select_color))
	      fill = TRUE;
        else
              fill = FALSE;          
      }

      x = w->primitive.highlight_thickness + w->primitive.shadow_thickness +
	  w->label.margin_width;
      
      if( w->toggle.indicator_set || _XmStringEmpty(w->label._label) )
	y = (w->core.height - w->toggle.indicator_dim)/2;
      else
      {
        y = w->label.TextRect.y;
        if (w->label.menu_type == XmMENU_POPUP ||
            w->label.menu_type == XmMENU_PULLDOWN)
          y += (w->toggle.indicator_dim + 2) / 4; /* adjust in menu */
      }

      if ((w->toggle.ind_type) == XmN_OF_MANY) 
      {
	 if (w->toggle.Armed == TRUE)
	 { 
#ifndef OSF_v1_2_4
	    if (IsOn(w) == TRUE)
		IsOn(w) = FALSE;
	    else
		IsOn(w) = TRUE;
#endif /* OSF_v1_2_4 */
	    if (w->toggle.ind_on)
	    {
	       /* if the toggle indicator is square shaped then adjust the
		  indicator width and height, so that it looks proportional
		  to a diamond shaped indicator of the same width and height */

	       int new_edge;

	       new_edge = edge - 3 - ((edge - 10)/10);
	                                      /* Subtract 3 pixels+1 */
                                              /* pixel for every 10 pixels, */
                                              /* from the width and height. */

	       /* Adjust x,y so that the indicator is centered
		  relative to the label*/
	       y = y + ((edge - new_edge) / 2);
	       x = x + ((edge - new_edge) / 2);

	       edge = new_edge;

	       if ((w->toggle.visible) ||
		   ((!w->toggle.visible) && (IsOn(w))))
	       {
		  _XmDrawShadows (XtDisplay (w), XtWindow (w), 
			       ((IsOn(w)) ? 
				  w -> primitive.bottom_shadow_GC :
				  w -> primitive.top_shadow_GC),
			       ((IsOn(w)) ? 
				  w -> primitive.top_shadow_GC :
				  w -> primitive.bottom_shadow_GC), 
				 x, y, edge, edge, 2, XmSHADOW_OUT);

		  if (w->toggle.fill_on_select)
		      if (edge > (fill ? 4 : 6) )
			  XFillRectangle(XtDisplay((Widget) w),
					 XtWindow ((Widget) w),
				       ((IsOn(w)) ?
					  w->toggle.select_GC :
					  w->toggle.background_gc),
					 ((fill) ? x+2 : x+3),
					 ((fill) ? y+2 : y+3),
					 ((fill) ? edge-4 : edge-6),
					 ((fill) ? edge-4 : edge-6));
	       }

	       if (!w->toggle.visible)
	       {
		  if (!IsOn(w))
		      if (edge > 0)
			  XFillRectangle (XtDisplay ((Widget) w),
					  XtWindow ((Widget) w),
					  w->toggle.background_gc,
					  x, y, edge, edge);
	       }
	    }
	    else
	    {
               if (w->primitive.shadow_thickness > 0)
                 DrawToggleShadow (w);
               if (w->toggle.fill_on_select && !Lab_IsPixmap(w))
                 DrawToggleLabel (w);
	    }

	    if (Lab_IsPixmap(w))
	    {
	       SetAndDisplayPixmap( w, event, NULL);
	    }
	 }
      }
      else 
      {
	 if (w->toggle.Armed == TRUE) 
	 { 
#ifndef OSF_v1_2_4
	    if (IsOn(w) == TRUE)
		IsOn(w) = FALSE;
	    else
		IsOn(w) = TRUE;
 
#endif /* OSF_v1_2_4 */
	    if (w->toggle.ind_on)
	    {
	       if ((w->toggle.visible) ||
		   ((!w->toggle.visible) && (IsOn(w))))
#ifdef CDE_VISUAL	/* toggle button indicator visual */
	       {
		   Boolean toggle_visual = False;
		   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)w)),
				 "enableToggleVisual", &toggle_visual, NULL);
		   if (toggle_visual)
		       DrawRadio(w, x, y, edge);
		   else
		       _XmDrawDiamond(XtDisplay(w), XtWindow((Widget) w),
				      ((IsOn(w)) ?
				       w -> primitive.bottom_shadow_GC :
				       w -> primitive.top_shadow_GC),
				      ((IsOn(w)) ?
				       w -> primitive.top_shadow_GC :
				       w -> primitive.bottom_shadow_GC),
				      (((IsOn(w))  &&
					(w->toggle.fill_on_select)) ?
				       w -> toggle.select_GC :
				       w -> toggle.background_gc),
				      x, y, edge, edge,
				      w->primitive.shadow_thickness, fill);
	       }
#else
		   _XmDrawDiamond( XtDisplay(w), XtWindow(w),
				    ((IsOn(w)) ?
				       w -> primitive.bottom_shadow_GC :
				       w -> primitive.top_shadow_GC),
				    ((IsOn(w)) ?
				       w -> primitive.top_shadow_GC :
				       w -> primitive.bottom_shadow_GC),
				    (((IsOn(w)) &&
				       (w->toggle.fill_on_select)) ?
				    w -> toggle.select_GC :
				       w -> toggle.background_gc),
				    x, y, edge, edge,
				    w->primitive.shadow_thickness, fill);
#endif
	       if (!w->toggle.visible)
	       {
		  if (!IsOn(w))
		      if (edge > 0)
			  XFillRectangle( XtDisplay ((Widget) w),
					 XtWindow ((Widget) w),
					 w->toggle.background_gc,
					 x, y, edge, edge);
	       }
	    }
	    else
	    {
               if (w->primitive.shadow_thickness > 0) DrawToggleShadow (w);
               if (w->toggle.fill_on_select && !Lab_IsPixmap(w))
                 DrawToggleLabel (w);
	    }
	    if (Lab_IsPixmap(w))
	    {
	       SetAndDisplayPixmap( w, event, NULL);
	    }
	 }
      }
   }
}


/****************************************************************************
 *
 *     Arm
 *       This function processes button down occuring on the togglebutton.
 *       Mark the togglebutton as armed and display it armed.
 *       The callbacks for XmNarmCallback are called.
 *
 ***************************************************************************/
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
  XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;

  (void)XmProcessTraversal( (Widget) tb, XmTRAVERSE_CURRENT);
  IsOn(tb) = (tb->toggle.set == TRUE) ? FALSE : TRUE;
  tb->toggle.Armed = TRUE;
  if (tb->toggle.ind_on)
  {
    DrawToggle(tb);
  }
  else
  {
   if (tb->primitive.shadow_thickness > 0) DrawToggleShadow (tb);
   if (tb->toggle.fill_on_select && !Lab_IsPixmap(tb)) DrawToggleLabel(tb);
  }
  if (Lab_IsPixmap(tb))
  {
    SetAndDisplayPixmap( tb, event, NULL);
  }

  if (tb->toggle.arm_CB)
  {
   XFlush(XtDisplay(tb));

   ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
  }
}


/************************************************************************
 *
 *     Select 
 *       Mark the togglebutton as unarmed (i.e. inactive).
 *       If the button release occurs inside of the ToggleButton, the
 *       callbacks for XmNvalueChangedCallback are called.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Select( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Select(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
  XmToggleButtonCallbackStruct call_value;
  XButtonEvent *buttonEvent = (XButtonEvent *) event;
  Dimension bw = tb->core.border_width ;
  Boolean hit;

  if (tb->toggle.Armed == FALSE)
     return;

  tb->toggle.Armed = FALSE;

#ifdef OSF_v1_2_4
  /* CR 8068: Verify that this is in fact a button event. */
#endif /* OSF_v1_2_4 */
  /* Check to see if BtnUp is inside the widget */
#ifndef OSF_v1_2_4
  hit = ((buttonEvent->x >= -(int)bw) &&
#else /* OSF_v1_2_4 */
  hit = ((event->xany.type == ButtonPress || 
	  event->xany.type == ButtonRelease) &&
	 (buttonEvent->x >= -(int)bw) &&
#endif /* OSF_v1_2_4 */
	 (buttonEvent->x < (int)(tb->core.width + bw)) &&
	 (buttonEvent->y >= -(int)bw) &&
	 (buttonEvent->y < (int)(tb->core.height + bw)));

  if (hit)
    tb->toggle.set = (tb->toggle.set) ? FALSE : TRUE;

  /* Redisplay after changing state. */
  (* ((WidgetClass)XtClass(tb))->core_class.expose)(wid, event, (Region) NULL);

  if (hit)
  {
     /* if the parent is a RowColumn, notify it about the select */
     if (XmIsRowColumn(XtParent(tb)))
     {
	call_value.reason = XmCR_VALUE_CHANGED;
	call_value.event = event;
	call_value.set = tb->toggle.set;
	(* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(tb), FALSE, tb,
						    &call_value);
     }

     if ((! tb->label.skipCallback) &&
	 (tb->toggle.value_changed_CB))
     {
	XFlush(XtDisplay(tb));
	ToggleButtonCallback(tb, XmCR_VALUE_CHANGED, tb->toggle.set, event);
     }
  }
     
}


/**********************************************************************
 *
 *    Disarm
 *      The callbacks for XmNdisarmCallback are called..
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
        XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;

   if (tb->toggle.disarm_CB)
    ToggleButtonCallback(tb, XmCR_DISARM, tb->toggle.set, event);

/* BEGIN OSF Fix pir 2826 */
   Redisplay((Widget) tb, event, (Region) NULL);
/* END OSF Fix pir 2826 */
 }

/************************************************************************
 *
 *     ArmAndActivate
 *       This routine arms and activates a ToggleButton. It is called on
 *       <Key> Return and a <Key> Space, as well as when a mnemonic or
 *       button accelerator has been activated.
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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   XmToggleButtonCallbackStruct call_value;
   Boolean already_armed = tb -> toggle.Armed;
   Boolean is_menupane = (tb -> label.menu_type == XmMENU_PULLDOWN) ||
			 (tb -> label.menu_type == XmMENU_POPUP);
   Boolean parent_is_torn;
   Boolean torn_has_focus = FALSE;

   if (is_menupane && !XmIsMenuShell(XtParent(XtParent(tb))))
   {
      parent_is_torn = TRUE;

      if (_XmFocusIsInShell((Widget)tb))
      {
         /* In case allowAcceleratedInsensitiveUnmanagedMenuItems is True */
         if (!XtIsSensitive((Widget)tb) || (!XtIsManaged((Widget)tb)))
            return;
         torn_has_focus = TRUE;
      }
   } else
      parent_is_torn = FALSE;

   tb -> toggle.Armed = FALSE;

   tb->toggle.set = (tb->toggle.set == TRUE) ? FALSE : TRUE;
   IsOn(tb) = tb->toggle.set;

   if (is_menupane)
   {
      if (parent_is_torn && !torn_has_focus)
      {
	 /* Freeze tear off visuals in case accelerators are not in 
	  * same context 
	  */
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_TEAROFF_TO_MENUSHELL, XtParent(tb), NULL, 
	    event, NULL);
      }

      if (torn_has_focus)
	 (* xmLabelClassRec.label_class.menuProcs)
            (XmMENU_POPDOWN, XtParent(tb), NULL, event, NULL);
      else
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(tb), NULL, event, NULL);

      if (torn_has_focus)
	 XmProcessTraversal((Widget) tb, XmTRAVERSE_CURRENT);

      /* Draw the toggle indicator in case of tear off */
      if (tb->toggle.ind_on)
      {
	 DrawToggle(tb);
      }
      else
      {
	 if (tb->toggle.fill_on_select && !Lab_IsPixmap(tb))
	    DrawToggleLabel(tb);
      }
      if (Lab_IsPixmap(tb))
      {
	 SetAndDisplayPixmap( tb, event, NULL);
      }
   }
   else
   { 
      if (tb->toggle.ind_on) DrawToggle(tb);
      else
      {
       if (tb -> primitive.shadow_thickness > 0) DrawToggleShadow (tb);
       if (tb->toggle.fill_on_select && !Lab_IsPixmap(tb))
          DrawToggleLabel (tb);
      }
      if (Lab_IsPixmap(tb))
      {
         SetAndDisplayPixmap( tb, event, NULL);
      }
   }

   /* If the parent is a RowColumn, set the lastSelectToplevel before the arm.
    * It's ok if this is recalled later.
    */
   if (XmIsRowColumn(XtParent(tb)))
   {
      (* xmLabelClassRec.label_class.menuProcs) (
         XmMENU_GET_LAST_SELECT_TOPLEVEL, XtParent(tb));
   }

   if (tb->toggle.arm_CB && !already_armed)
   {
      XFlush(XtDisplay(tb));
      ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
   }

   /* if the parent is a RowColumn, notify it about the select */
   if (XmIsRowColumn(XtParent(tb)))
   {
      call_value.reason = XmCR_VALUE_CHANGED;
      call_value.event = event;
      call_value.set = tb->toggle.set;
      (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK,
	 XtParent(tb),  FALSE, tb, &call_value);
   }
   
   if ((! tb->label.skipCallback) &&
       (tb->toggle.value_changed_CB))
   {
      XFlush(XtDisplay(tb));
      ToggleButtonCallback(tb, XmCR_VALUE_CHANGED, tb->toggle.set, event);
   }

   if (tb->toggle.disarm_CB)
   {
      XFlush(XtDisplay(tb));
      ToggleButtonCallback(tb, XmCR_DISARM, tb->toggle.set, event);
   }

   if (is_menupane)
   {
      if (torn_has_focus)
      {
	 tb -> toggle.Armed = TRUE;
	 if (tb->toggle.arm_CB) 
	 {
	    XFlush(XtDisplay(tb));
	    ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
	 }
      } else
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(tb), NULL, event, NULL);
   }
}


/************************************************************************
 *
 *     BtnDown
 *       This function processes a button down occuring on the togglebutton
 *       when it is in a popup, pulldown, or option menu.
 *       Popdown the posted menu.
 *       Turn parent's traversal off.
 *       Mark the togglebutton as armed (i.e. active).
 *       The callbacks for XmNarmCallback are called.
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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   Boolean validButton;
   Boolean already_armed;
   ShellWidget popup;
#ifdef CDE_VISUAL       /* etched in menu button */
   Boolean etched_in = False;
   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)), "enableEtchedInMenu", &etched_in, NULL);
#endif

   /* Support menu replay, free server input queue until next button event */
   XAllowEvents(XtDisplay(tb), SyncPointer, CurrentTime);

   already_armed = tb -> toggle.Armed;
   tb -> toggle.Armed = TRUE;
   if (event && (event->type == ButtonPress))
   {
#ifdef CDE_VISUAL       /* etched in menu button */
       if (etched_in) 
       {
	   DrawToggleLabel(tb);
	   if (tb->toggle.ind_on)
	       DrawToggle(tb);
       }
#endif
       (* xmLabelClassRec.label_class.menuProcs) (XmMENU_BUTTON,
						  XtParent(tb), NULL, event,
						  &validButton);
   }

   if (!validButton)
       return;

   _XmSetInDragMode((Widget)tb, True);

   /* Popdown other popups that may be up */
   if (!(popup = (ShellWidget)_XmGetRC_PopupPosted(XtParent(tb))))
   {
      if (!XmIsMenuShell(XtParent(XtParent(tb))))
      {
	 /* In case tear off not armed and no grabs in place, do it now.
	  * Ok if already armed and grabbed - nothing done.
	  */
	 (* xmLabelClassRec.label_class.menuProcs) 
	    (XmMENU_TEAR_OFF_ARM, XtParent(tb));
      }
   }

   if  (popup)
   {
      Widget w;

      if (popup->shell.popped_up)
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_SHELL_POPDOWN, (Widget) popup, NULL, event, NULL);

      /* If the active_child is a cascade (highlighted), then unhighlight it.*/
      w = ((XmManagerWidget)XtParent(tb))->manager.active_child;
      if (w && (XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w)))
	 XmCascadeButtonHighlight (w, FALSE);
   }

   /* Set focus to this button.  This must follow the possible
    * unhighlighting of the CascadeButton else it'll screw up active_child.
    */
   (void)XmProcessTraversal( (Widget) tb, XmTRAVERSE_CURRENT);
	 /* get the location cursor - get consistent with Gadgets */

   if (tb->toggle.arm_CB && !already_armed)
   {
      XFlush (XtDisplay (tb));

      ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
   }

   _XmRecordEvent(event);
}


/************************************************************************
 *
 *     BtnUp
 *       This function processes a button up occuring on the togglebutton
 *       when it is in a popup, pulldown, or option menu.
 *       Mark the togglebutton as unarmed (i.e. inactive).
 *       The callbacks for XmNvalueChangedCallback are called.
 *       The callbacks for XmNdisarmCallback are called.
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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   XmToggleButtonCallbackStruct call_value;
   Boolean validButton;
   Boolean popped_up;
   Dimension bw = tb->core.border_width ;
   Boolean is_menupane = (tb -> label.menu_type == XmMENU_PULLDOWN) ||
			 (tb -> label.menu_type == XmMENU_POPUP);
   Widget shell = XtParent(XtParent(tb));

   if (event && (event->type == ButtonRelease))
       (* xmLabelClassRec.label_class.menuProcs) (XmMENU_BUTTON,
						  XtParent(tb), NULL, event,
						  &validButton);

   if (!validButton || (tb -> toggle.Armed == FALSE))
       return;
   
   tb -> toggle.Armed = FALSE;

   if (is_menupane && !XmIsMenuShell(shell))
      (* xmLabelClassRec.label_class.menuProcs)
	 (XmMENU_POPDOWN, (Widget) tb, NULL, event, &popped_up);
   else
      (* xmLabelClassRec.label_class.menuProcs)
	 (XmMENU_BUTTON_POPDOWN, (Widget) tb, NULL, event, &popped_up);

   _XmRecordEvent(event);

   if (popped_up)
      return;

   /* Check to see if BtnUp is inside the widget */

   if ((event->xbutton.x >= -(int)bw) &&
       (event->xbutton.x < (int)(tb->core.width + bw)) &&
       (event->xbutton.y >= -(int)bw) &&
       (event->xbutton.y < (int)(tb->core.height + bw)))
   {
      tb->toggle.set = (tb->toggle.set == TRUE) ? FALSE : TRUE;
      IsOn(tb) = tb->toggle.set;

      /* if the parent is a RowColumn, notify it about the select */
      if (XmIsRowColumn(XtParent(tb)))
      {
	 call_value.reason = XmCR_VALUE_CHANGED;
	 call_value.event = event;
	 call_value.set = tb->toggle.set;
	 (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(tb), FALSE, tb,
						    &call_value);
      }
      
      if ((! tb->label.skipCallback) &&
	  (tb->toggle.value_changed_CB))
      {
	 XFlush(XtDisplay(tb));
	 ToggleButtonCallback(tb, XmCR_VALUE_CHANGED, tb->toggle.set, event);
      }
      
      if (tb->toggle.disarm_CB)
	  ToggleButtonCallback(tb, XmCR_DISARM, tb->toggle.set, event);

      if (is_menupane)
      {
	 if (!XmIsMenuShell(shell))
	 {
	    if (XtIsSensitive(tb))
	    {
	       if (tb->toggle.ind_on)
	       {
		 DrawToggle(tb);
	       }
	       else
	       {
		 if (tb->toggle.fill_on_select && !Lab_IsPixmap(tb))
		   DrawToggleLabel(tb);
	       }
	       if (Lab_IsPixmap(tb))
	       {
		   SetAndDisplayPixmap( tb, event, NULL);
	       }
	       tb -> toggle.Armed = TRUE;
	       if (tb->toggle.arm_CB) 
	       {
		   XFlush(XtDisplay(tb));
		   ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
	       }
	    }
	 }
	 else
	    (* xmLabelClassRec.label_class.menuProcs)
	       (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	       XtParent(tb), NULL, event, NULL);
      }
   }

   _XmSetInDragMode((Widget)tb, False);

   /* For the benefit of tear off menus, we must set the focus item 
    * to this button.  In normal menus, this would not be a problem
    * because the focus is cleared when the menu is unposted.
    */
   if (!XmIsMenuShell(shell))
      XmProcessTraversal((Widget) tb, XmTRAVERSE_CURRENT);
}

#ifdef CDE_VISUAL   /* exclusive button select color change */

static Pixel
#ifdef _NO_PROTO
_XmGetDefaultColor( widget, type)
        Widget widget ;
    int type;
#else
_XmGetDefaultColor(
        Widget widget,
			 int type)
#endif /* _NO_PROTO */
{
	XmColorData *color_data;

	if (!XtIsWidget(widget))
		widget = widget->core.parent;

	color_data = _XmGetColors(XtScreen((Widget)widget),
		widget->core.colormap, widget->core.background_pixel);

	return _XmAccessColorData(color_data, type);
}
#endif

/************************************************************************
 *
 *  GetGC
 *	Get the graphics context to be used to fill the interior of
 *	a square or diamond when selected.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
GetGC( tw )
        XmToggleButtonWidget tw ;
#else
GetGC(
        XmToggleButtonWidget tw )
#endif /* _NO_PROTO */
{
   XGCValues values;
   XtGCMask  valueMask;
   XFontStruct *fs = (XFontStruct *) NULL;

   valueMask = GCForeground | GCBackground | GCFillStyle | GCGraphicsExposures;
   if ((DefaultDepthOfScreen (XtScreen (tw)) == 1) /*  Monochrome Display */
        && (tw -> core.background_pixel == tw -> toggle.select_color))
       values.foreground = tw->primitive.foreground;
   else {
       values.foreground = tw -> toggle.select_color;

#ifdef CDE_VISUAL   /* exclusive button select color change */

/* check if the select_color is the default value or
not. If it is the default value then change the GC's foreground color
to the primitive highlight color. Else the default select_color has
been overridden and should be left alone. */
       {
       Boolean toggle_color = False;
       XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget) tw)), "enableToggleColor", 
           &toggle_color, NULL);

       if (toggle_color &&
	   (tw->toggle.ind_type == XmONE_OF_MANY) &&
	   (values.foreground == _XmGetDefaultColor((Widget) tw, XmSELECT)))
	   values.foreground = tw -> primitive.highlight_color;
       }
#endif
   }

   values.background = tw -> core.background_pixel;
   values.fill_style = FillSolid;
   values.graphics_exposures = FALSE;

   tw -> toggle.select_GC =  XtGetGC ((Widget) tw, valueMask, &values);

   valueMask = GCForeground | GCBackground | GCFillStyle | GCGraphicsExposures;

   /* When foreground and select colors coincide, this GC is used
    * by XmLabel to draw the text. It requires a font to pacify
    * the XmString draw functions.
    */
   _XmFontListGetDefaultFont(tw->label.font, &fs);
   if (fs != NULL) {
      valueMask |= GCFont;
      values.font = fs->fid;
   }

   values.foreground = tw->core.background_pixel;
   values.background = tw->primitive.foreground;
   values.fill_style = FillSolid;
   values.graphics_exposures = FALSE;

   tw->toggle.background_gc = XtGetGC((Widget) tw, valueMask, &values);
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
        XmToggleButtonWidget request = (XmToggleButtonWidget) rw ;
        XmToggleButtonWidget new_w = (XmToggleButtonWidget) nw ;
    int maxIndicatorSize;   /* Max Indicator size permissible */
    int delta;
    int boxSize;

    new_w->toggle.Armed = FALSE;

    /* if menuProcs is not set up yet, try again */
    if (xmLabelClassRec.label_class.menuProcs == (XmMenuProc)NULL)
	xmLabelClassRec.label_class.menuProcs =
	    (XmMenuProc) _XmGetMenuProcContext();

    if (new_w->label.menu_type == XmMENU_POPUP ||
	new_w->label.menu_type == XmMENU_PULLDOWN)
    {
       if (new_w->primitive.shadow_thickness <= 0)
	   new_w->primitive.shadow_thickness = 2;

       if (new_w->toggle.visible == XmINVALID_BOOLEAN)
	   new_w->toggle.visible = FALSE;
       
       new_w->primitive.traversal_on = TRUE;
    }
    else
    {
       if (new_w->toggle.visible == XmINVALID_BOOLEAN)
	   new_w->toggle.visible = TRUE;

    }

    /*
     * If fillOnSelect has not been set, copy indicatorOn.
     * This provides 1.1 compatibility: fillOnSelect == true was
     * the default and had no effect when indicatorOn == false.
     * In 1.2 it causes the background to be filled with selectColor.
     * Don't want to surprise applications which set indicatorOn false,
     * let fillOnSelect default to true, and didn't expect the background
     * to be filled.
     */  
    if (new_w->toggle.fill_on_select == XmINVALID_BOOLEAN)
         new_w->toggle.fill_on_select = new_w->toggle.ind_on;

    /*
     * if the indicatorType has not been set, then
     * find out if radio behavior is set for RowColumn parents and
     * then set indicatorType.  If radio behavior is true, default to
     * one of many, else default to n of many.
     */
    if(    (new_w->toggle.ind_type == XmINVALID_TYPE)
        || !XmRepTypeValidValue( XmRID_INDICATOR_TYPE,
                                       new_w->toggle.ind_type, (Widget) new_w)    )
    {
       if  (XmIsRowColumn(XtParent(new_w)))
       {
	  Arg arg[1];
	  Boolean radio;

	  XtSetArg (arg[0], XmNradioBehavior, &radio);
	  XtGetValues (XtParent(new_w), arg, 1);

	  if (radio)
	      new_w->toggle.ind_type = XmONE_OF_MANY;
	  else
	      new_w->toggle.ind_type = XmN_OF_MANY;
       }
       else
	   new_w->toggle.ind_type = XmN_OF_MANY;
    }
	

    if (IsNull (PixmapOff(new_w)) &&            /* no Off pixmap but do have */
        ! IsNull (PixmapOn(new_w)))           /* an On, so use that */
    {
       PixmapOff(new_w) = PixmapOn(new_w);
       if (request->core.width == 0)
         new_w->core.width = 0;
       if (request->core.height == 0)
         new_w->core.height = 0;

       _XmCalcLabelDimensions((Widget) new_w);
       (* xmLabelClassRec.core_class.resize)( (Widget) new_w);
    }

    if (IsNull (Pixmap_Insen_Off(new_w)) &&      /* no Off pixmap but do have */
        ! IsNull (Pixmap_Insen_On(new_w)))       /* an On, so use that */
    {
       Pixmap_Insen_Off(new_w) = Pixmap_Insen_On(new_w);
       if (request->core.width == 0)
	   new_w->core.width = 0;
       if (request->core.height == 0)
	   new_w->core.height = 0;


       _XmCalcLabelDimensions((Widget) new_w);
       (* xmLabelClassRec.core_class.resize)( (Widget) new_w);
    }

/* BEGIN OSF Fix pir 1778 */
     if (Lab_IsPixmap(new_w) &&
       (!IsNull(PixmapOff(new_w)) || !IsNull(PixmapOn(new_w)) ||
        !IsNull(Pixmap_Insen_Off(new_w)) || !IsNull(Pixmap_Insen_On(new_w))))
       {
       if (request->core.width == 0)
         new_w->core.width = 0;
       if (request->core.height == 0)
         new_w->core.height = 0;
       SetToggleSize(new_w);
       }
/* END OSF Fix pir 1778 */

    if (new_w->toggle.indicator_dim == XmINVALID_DIMENSION)  {
      new_w->toggle.indicator_set = Lab_IsPixmap(new_w);
      if (new_w->toggle.ind_on)
      {

	/* DETERMINE HOW HIGH THE TOGGLE INDICATOR SHOULD BE */

	if Lab_IsPixmap(new_w) 
	{
	   /*set indicator size proportional to size of pixmap*/	 
	   if (new_w->label.TextRect.height < 13)
	       new_w->toggle.indicator_dim = new_w->label.TextRect.height;
	   else
	       new_w->toggle.indicator_dim = 13 + (new_w->label.TextRect.height/13);
	}

	else
	{
	   /*set indicator size proportional to size of font*/	 
	   Dimension height;
	   int line_count;

	   height = _XmStringHeight (new_w->label.font, new_w->label._label);
	   if( (line_count = _XmStringLineCount (new_w->label._label)) < 1)
	     line_count = 1;
	   /* Shiz recommends toggles in menus have smaller indicators */
	   if (new_w->label.menu_type == XmMENU_POPUP ||
	       new_w->label.menu_type == XmMENU_PULLDOWN) {
	     new_w->toggle.indicator_dim = Max(XmDEFAULT_INDICATOR_DIM,
	       (height / ((Dimension)line_count))*2/3);
	   } else
	       new_w->toggle.indicator_dim = Max(XmDEFAULT_INDICATOR_DIM,
		 height / ((Dimension)line_count));
	}
      } else
	new_w->toggle.indicator_dim = 0;
    } else
      new_w->toggle.indicator_set = TRUE;

    if (new_w->toggle.ind_on)
    {
 /*
  *   Enlarge the text rectangle if needed to accomodate the size of
  *     indicator button. Adjust the dimenions of superclass Label-Gadget
  *     so that the toggle-button may be accommodated in it.
  */
/* BEGIN OSF Fix pir 2480 */
    if ( new_w->label.menu_type != XmMENU_POPUP &&
         new_w->label.menu_type != XmMENU_PULLDOWN )
      maxIndicatorSize = new_w->toggle.indicator_dim +
	                 2 * (new_w->primitive.shadow_thickness +
			 Xm3D_ENHANCE_PIXEL); 
    else
      maxIndicatorSize = new_w->toggle.indicator_dim;
/* END OSF Fix pir 2480 */

    boxSize = (int)( new_w->label.TextRect.height)  +
				(int) new_w->label.margin_top + (int) new_w->label.margin_bottom; 
 
	if (maxIndicatorSize > boxSize)
	 { delta = maxIndicatorSize - boxSize;
	   new_w->label.margin_top += delta/2;
	   new_w->label.margin_bottom += delta /2;
	 }

    /* Make room for toggle indicator and spacing */

       if ((new_w->label.margin_left) < (new_w->toggle.indicator_dim +
                                       new_w->toggle.spacing))
	   new_w->label.margin_left = (new_w->toggle.indicator_dim +
				     new_w->toggle.spacing);
    }

    if (request->core.width == 0)
    {
       new_w->core.width = new_w->label.TextRect.width +
                          2 * new_w->label.margin_width +   
                          new_w->label.margin_right +
                          new_w->label.margin_left +
		          2 * (new_w->primitive.highlight_thickness +
                               new_w->primitive.shadow_thickness); 

       if (new_w->core.width == 0)
	   new_w->core.width = 1; 

       if ((new_w->label._acc_text != NULL) && (new_w->toggle.ind_on))
	   new_w->label.acc_TextRect.x = new_w->core.width -
                                         new_w->primitive.highlight_thickness -
                                         new_w->primitive.shadow_thickness -
                                         new_w->label.margin_width -
                                         new_w->label.margin_right +
                                         LABEL_ACC_PAD;
    }

    if (request->core.height == 0)
	new_w->core.height = Max(new_w->toggle.indicator_dim,
	    new_w->label.TextRect.height + 2 * new_w->label.margin_height + 
	        new_w->label.margin_top + new_w->label.margin_bottom)  + 
	    2 * (new_w->primitive.highlight_thickness +
		new_w->primitive.shadow_thickness);

    new_w->label.TextRect.y =  (short) new_w->primitive.highlight_thickness
       + new_w->primitive.shadow_thickness
           + new_w->label.margin_height + new_w->label.margin_top +
               ((new_w->core.height - new_w->label.margin_top
                 - new_w->label.margin_bottom
                 - (2 * (new_w->label.margin_height
                         + new_w->primitive.highlight_thickness
                         + new_w->primitive.shadow_thickness))
                 - new_w->label.TextRect.height) / 2);

    if (new_w->core.height == 0)
	new_w->core.height = 1;


    if (new_w->toggle.set)
        IsOn(new_w) = TRUE; /* When toggles first come up, if
                                           XmNset is TRUE, then they are
                                           displayed set */
    else
        IsOn(new_w) = FALSE;

    (* (new_w->core.widget_class->core_class.resize)) ((Widget) new_w);

#ifdef CDE_VISUAL
    InitExtension(new_w);
#endif
    GetGC (new_w);
}   





/************************************************************************
 *
 *  Destroy
 *	Free toggleButton's graphic context.
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
        XmToggleButtonWidget tw = (XmToggleButtonWidget) wid ;
   XtReleaseGC ((Widget) tw, tw -> toggle.select_GC);
   XtReleaseGC ((Widget) tw, tw -> toggle.background_gc);

   XtRemoveAllCallbacks ((Widget) tw, XmNvalueChangedCallback);
   XtRemoveAllCallbacks ((Widget) tw, XmNarmCallback);
   XtRemoveAllCallbacks ((Widget) tw, XmNdisarmCallback);

#ifdef CDE_VISUAL
   FreeExtension((Widget) wid);
#endif
}






#ifdef CDE_VISUAL	/* toggle button indicator visual */
static void
#ifdef _NO_PROTO
DrawRadio(w, x, y, length)
    XmToggleButtonWidget w;
    int x, y, length;
#else
DrawRadio(XmToggleButtonWidget w,
	      int x,
	      int y,
	      int length)
#endif /* _NO_PROTO */
{
    XDrawArc(XtDisplay ((Widget) w),
	     XtWindow ((Widget) w),
	     IsOn(w) ?
	     w -> primitive.bottom_shadow_GC :
	     w -> primitive.top_shadow_GC,
	     x, y, length, length,
	     45 * 64,
	     180 * 64);
    XDrawArc(XtDisplay ((Widget) w),
	     XtWindow ((Widget) w),
	     IsOn(w) ?
	     w -> primitive.top_shadow_GC :
	     w -> primitive.bottom_shadow_GC,
	     x, y, length, length,
	     45 * 64,
	     -180 * 64);

    length -= 6;
    if (length < 1)
	length = 1;
    XFillArc(XtDisplay ((Widget) w),
	     XtWindow ((Widget) w),
             IsOn(w) ? w -> toggle.select_GC : w -> toggle.background_gc,
	     x+3, y+3, length, length,
	     0,
	     360 * 64);
}

static void
#ifdef _NO_PROTO
DrawCheck(w, x, y, length)
    Widget w;
    int x, y, length;
#else
DrawCheck(Widget w,
	      int x,
	      int y,
	      int length)
#endif /* _NO_PROTO */
{
    static unsigned char bitmap_8[] = {
	0x80, 0xc0, 0x42, 0x67, 0x3e, 0x3c, 0x18, 0x10};

    static unsigned char bitmap_11[] = {
	0x00, 0x04, 0x00, 0x06, 0x00, 0x03, 0x80, 0x01, 0xc6, 0x00, 0xef, 0x00,
	0x7e, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x30, 0x00, 0x00, 0x00};

    static unsigned char bitmap_13[] = {
	0x00, 0x10, 0x00, 0x18, 0x00, 0x0c, 0x00, 0x0e, 0x04, 0x07, 0x8e, 0x03,
	0xdf, 0x03, 0xfe, 0x01, 0xfc, 0x01, 0xf8, 0x00, 0xf0, 0x00, 0x60, 0x00,
	0x40, 0x00};

    static unsigned char bitmap_18[] = {
	0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x00,
	0x00, 0x60, 0x00, 0x00, 0x70, 0x00, 0x04, 0x38, 0x00, 0x0e, 0x1c, 0x00,
	0x1f, 0x1e, 0x00, 0x3f, 0x0f, 0x00, 0xfe, 0x0f, 0x00, 0xfc, 0x0f, 0x00,
	0xf8, 0x07, 0x00, 0xf0, 0x07, 0x00, 0xe0, 0x07, 0x00, 0xc0, 0x03, 0x00,
	0x80, 0x03, 0x00, 0x00, 0x01, 0x00};

    typedef struct { unsigned char *bits;
		     Pixmap bitmap;
		     int	length;
	 } info_t;
    static info_t info_list[] = {
    { bitmap_8, XtUnspecifiedPixmap, 8 },
    { bitmap_11, XtUnspecifiedPixmap, 11 },
    { bitmap_13, XtUnspecifiedPixmap, 13 },
    { bitmap_18, XtUnspecifiedPixmap, 18 }};
    int i, centering;
    info_t *info;
    GC gc = ((XmToggleButtonWidget) w)->label.normal_GC;
    Boolean etched_in = False;

    info = info_list;
    /* if box is too small then none of the checks will fit so return */
    if (length < info->length)
	return;

    for(i=0; i<XtNumber(info_list); i++)
	if (length >= info_list[i].length)
	    info = info_list+i;

    centering = (length - info->length) / 2;
    x += centering;
    y += centering;

    if (info->bitmap == XtUnspecifiedPixmap)
	info->bitmap = XCreateBitmapFromData(XtDisplay(w), XtWindow(w),
			   (char *) info->bits, info->length, info->length);

    XSetStipple(XtDisplay(w), gc, info->bitmap);
    XSetFillStyle(XtDisplay(w), gc, FillStippled);
    XSetTSOrigin(XtDisplay(w), gc, x, y);
 
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableEtchedInMenu", 
			&etched_in, NULL);
      if (etched_in)
      {
	GetExtension(w);
	XFillRectangle(XtDisplay(w), XtWindow(w), 
		IsArmed((XmToggleButtonWidget)w)?
		 tb_extension->arm_GC: gc, x, y, info->length, info->length);
      }
      else
    	XFillRectangle(XtDisplay(w), XtWindow(w), gc, x, y,
		   info->length, info->length);
 
    XSetFillStyle(XtDisplay(w), gc, FillSolid);
    XSetTSOrigin(XtDisplay(w), gc, 0, 0);
}
#endif


/*************************************<->*************************************
 *
 *  DrawToggle(w)
 *     Depending on the state of this widget, draw the ToggleButton.
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
DrawToggle( w )
        XmToggleButtonWidget w ;
#else
DrawToggle(
        XmToggleButtonWidget w )
#endif /* _NO_PROTO */
{
   int x, y, edge;
   Boolean   fill;
#ifdef CDE_VISUAL
   Boolean etched_in = False;
   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)),
                 "enableEtchedInMenu", &etched_in, NULL);
#endif

  if( w->toggle.indicator_set || _XmStringEmpty(w->label._label) ) {
    edge = w->toggle.indicator_dim;
  } else {
    edge = Min((int)w->toggle.indicator_dim, 
             Max(0, (int)w->core.height - 2*(w->primitive.highlight_thickness +
		 w->primitive.shadow_thickness +
		(int)w->label.margin_height) +
		 w->label.margin_top +
		 w->label.margin_bottom));

  }

  if (DefaultDepthOfScreen (XtScreen (w)) == 1) /* Monochrome Display */
    fill = FALSE;
  else
  {
    if ((w->primitive.top_shadow_color != w->toggle.select_color) &&
        (w->primitive.bottom_shadow_color != w->toggle.select_color))
           fill = TRUE;
    else
           fill = FALSE;
  }
      
  x = w->primitive.highlight_thickness + w->primitive.shadow_thickness +
      w->label.margin_width;

  if( w->toggle.indicator_set || _XmStringEmpty(w->label._label) )
    y = (int)((w->core.height - w->toggle.indicator_dim))/2;
  else
  {
    y = w->label.TextRect.y;
    if (w->label.menu_type == XmMENU_POPUP ||
        w->label.menu_type == XmMENU_PULLDOWN)
      y += (w->toggle.indicator_dim + 2) / 4; /* adjust in menu */
  }

  if ((w->toggle.ind_type) == XmN_OF_MANY)
  {
      /* if the toggle indicator is square shaped then adjust the
         indicator width and height, so that it looks proportional
         to a diamond shaped indicator of the same width and height */

     int new_edge;
     
     new_edge = edge - 3 - ((edge - 10)/10); /* Subtract 3 pixels + 1 pixel */
                                              /* for every 10 pixels, from   */
                                              /* width and height.           */

     /* Adjust x,y so that the indicator is centered relative to the label */
     y = y + ((edge - new_edge) / 2); 
     x = x + ((edge - new_edge) / 2);
     edge = new_edge;

     if ((w->toggle.visible) ||
	 ((!w->toggle.visible) && (IsOn(w))))
     {
#ifdef CDE_VISUAL	/* toggle button indicator visual */
	 int shadow_thickness;
	 int rect_x;
	 int rect_y;
	 int length;
         Boolean toggle_visual = False;

         XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)w)), 
		"enableToggleVisual", &toggle_visual, NULL);
         shadow_thickness = (toggle_visual ? 1 : 2);
	 rect_x = x + shadow_thickness;
	 rect_y = y + shadow_thickness;
	 length = edge - shadow_thickness * 2;

        _XmDrawShadows(XtDisplay ((Widget) w),
                     XtWindow ((Widget) w),
                     ((IsOn(w)) ?
                      w -> primitive.bottom_shadow_GC :
                      w -> primitive.top_shadow_GC),
                     ((IsOn(w)) ?
                      w -> primitive.top_shadow_GC :
                      w -> primitive.bottom_shadow_GC),
                     x, y, edge, edge,
		       shadow_thickness,	       
		       XmSHADOW_OUT);
 
	 if (!fill && IsOn(w)) {
	     rect_x += 1;
	     rect_y += 1;
	     length -= 2;
	 }

         if (etched_in)
         {
	     GC tmp_GC;
	     GetExtension((Widget)w);
	     if ( (IsOn(w)) && (w->toggle.fill_on_select) )
	     {
		if ( (IsArmed(w)) && 
		     (w->toggle.select_color== 
                      _XmGetDefaultColor((Widget) w, XmSELECT)) )
			tmp_GC = tb_extension->normal_GC;
		else  /* User specifies own select-color */
			tmp_GC = w -> toggle.select_GC;
	     }
	     else
	     {
		if ( IsArmed(w) ) 
			tmp_GC = tb_extension->arm_GC;
		else
			tmp_GC = w -> toggle.background_gc;
	     }
	     XFillRectangle (XtDisplay ((Widget) w),
                            XtWindow ((Widget) w),
			    tmp_GC,
			    rect_x,
			    rect_y,
			    length,
			    length);
	 }
	 else
	     XFillRectangle (XtDisplay ((Widget) w),
                            XtWindow ((Widget) w),
                            (((IsOn(w)) &&
                              (w->toggle.fill_on_select)) ?
					w -> toggle.select_GC :
					w -> toggle.background_gc),
			    rect_x,
			    rect_y,
			    length,
			    length);
	 
	 if (IsOn(w) && toggle_visual)
	     DrawCheck((Widget) w,
			   x + shadow_thickness, y + shadow_thickness,
			   edge - shadow_thickness * 2);
#else /* CDE_VISUAL */
        _XmDrawShadows(XtDisplay ((Widget) w),
                     XtWindow ((Widget) w),
                     ((IsOn(w)) ?
                      w -> primitive.bottom_shadow_GC :
                      w -> primitive.top_shadow_GC),
                     ((IsOn(w)) ?
                      w -> primitive.top_shadow_GC :
                      w -> primitive.bottom_shadow_GC),
                     x, y, edge, edge, 2, XmSHADOW_OUT);
 
        if (edge > (fill ? 4 : 6) )
            XFillRectangle (XtDisplay ((Widget) w),
                            XtWindow ((Widget) w),
                            (((IsOn(w)) &&
                              (w->toggle.fill_on_select)) ?
                             w -> toggle.select_GC :
                             w -> toggle.background_gc),
                            ((fill) ? x+2 : x+3),
                            ((fill) ? y+2 : y+3),
                            ((fill) ? edge-4 : edge-6),
                            ((fill) ? edge-4 : edge-6));
#endif /* CDE_VISUAL */
     }
  }
  else
  {
    if ((w->toggle.visible) ||
          ((!w->toggle.visible) && (IsOn(w))))
#ifdef CDE_VISUAL	/* toggle button indicator visual */
        {
        Boolean toggle_visual = False;
        XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)w)), "enableToggleVisual", &toggle_visual, NULL);
	if (toggle_visual)
	    DrawRadio(w, x, y, edge);
	else
	    _XmDrawDiamond(XtDisplay(w), XtWindow((Widget) w),
		     ((IsOn(w)) ?
		      w -> primitive.bottom_shadow_GC :
		      w -> primitive.top_shadow_GC),
		     ((IsOn(w)) ?
		      w -> primitive.top_shadow_GC :
		      w -> primitive.bottom_shadow_GC),
		     (((IsOn(w))  &&
		       (w->toggle.fill_on_select)) ?
		      w -> toggle.select_GC :
		      w -> toggle.background_gc),
		     x, y, edge, edge,
		     w->primitive.shadow_thickness, fill);
       }
#else
       _XmDrawDiamond(XtDisplay(w), XtWindow((Widget) w),
		     ((IsOn(w)) ?
		      w -> primitive.bottom_shadow_GC :
		      w -> primitive.top_shadow_GC),
		     ((IsOn(w)) ?
		      w -> primitive.top_shadow_GC :
		      w -> primitive.bottom_shadow_GC),
		     (((IsOn(w))  &&
		       (w->toggle.fill_on_select)) ?
		      w -> toggle.select_GC :
		      w -> toggle.background_gc),
		     x, y, edge, edge,
		     w->primitive.shadow_thickness, fill);
#endif
  }

   if ((!w->toggle.visible) && (!IsOn(w)))
   {
       if (edge > 0)
#ifdef CDE_VISUAL
	{
	   GetExtension((Widget)w);
	   XFillRectangle( XtDisplay ((Widget) w),
			  XtWindow ((Widget) w),
			  (etched_in && IsArmed(w)) ? tb_extension->arm_GC:
				w->toggle.background_gc,
			  x, y, edge, edge);
	}
#else
	   XFillRectangle( XtDisplay ((Widget) w),
			  XtWindow ((Widget) w),
			  w->toggle.background_gc,
			  x, y, edge, edge);
#endif
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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   XEvent * event = NULL;

   if (tb -> label.menu_type == XmMENU_PULLDOWN ||
       tb -> label.menu_type == XmMENU_POPUP)
   {
#ifdef CDE_VISUAL	/* etched in menu button */
      Boolean etched_in = False;
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)), "enableEtchedInMenu", &etched_in, NULL);
      if (etched_in)
      {
          Boolean tmp_Armed= tb->toggle.Armed;
          tb->toggle.Armed = True;
          DrawToggleLabel(tb);
	  if (tb->toggle.ind_on)
              DrawToggle(tb);
          tb->toggle.Armed = tmp_Armed;
      }
      
      _XmDrawShadows (XtDisplay (tb), XtWindow (tb),
		   tb -> primitive.top_shadow_GC,
		   tb -> primitive.bottom_shadow_GC,
		   tb -> primitive.highlight_thickness,
		   tb -> primitive.highlight_thickness,
		   tb -> core.width - 2 *
		       tb->primitive.highlight_thickness,
		   tb -> core.height - 2 *
		       tb->primitive.highlight_thickness,
		   tb -> primitive.shadow_thickness,
                   etched_in ? XmSHADOW_IN : XmSHADOW_OUT);
#else
      _XmDrawShadows (XtDisplay (tb), XtWindow (tb),
		   tb -> primitive.top_shadow_GC,
		   tb -> primitive.bottom_shadow_GC,
		   tb -> primitive.highlight_thickness,
		   tb -> primitive.highlight_thickness,
		   tb -> core.width - 2 *
		       tb->primitive.highlight_thickness,
		   tb -> core.height - 2 *
		       tb->primitive.highlight_thickness,
		   tb -> primitive.shadow_thickness, XmSHADOW_OUT);
#endif

      if( !tb->toggle.Armed  &&  tb->toggle.arm_CB )
      {
	 XFlush (XtDisplay (tb));
	 ToggleButtonCallback(tb, XmCR_ARM, tb->toggle.set, event);
      }
      tb -> toggle.Armed = TRUE;
   }
   else 
   {    (*(xmLabelClassRec.primitive_class.border_highlight))((Widget) tb) ;
       } 

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
   XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   XEvent * event = NULL;

   if( tb -> label.menu_type == XmMENU_PULLDOWN ||
       tb -> label.menu_type == XmMENU_POPUP )
   {
#ifdef CDE_VISUAL       /* etched in menu button */
      Boolean etched_in = False;
      XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(wid)), "enableEtchedInMenu", &etched_in, NULL);
      if (etched_in)
      {
          Boolean tmp_Armed = tb->toggle.Armed;
          tb -> toggle.Armed = FALSE;
          DrawToggleLabel(tb);
	  if (tb->toggle.ind_on)
              DrawToggle(tb);
          tb -> toggle.Armed = tmp_Armed;
      }
#endif
      _XmClearBorder (XtDisplay (tb), XtWindow (tb),
		   tb -> primitive.highlight_thickness,
		   tb -> primitive.highlight_thickness,
		   tb -> core.width - 2 *
			tb->primitive.highlight_thickness,
		   tb -> core.height - 2 *
			tb->primitive.highlight_thickness,
		   tb -> primitive.shadow_thickness);

      if(tb->toggle.Armed && tb->toggle.disarm_CB)
      {
	 XFlush (XtDisplay (tb));
	 ToggleButtonCallback(tb, XmCR_DISARM, tb->toggle.set, event);
      }
      tb -> toggle.Armed = FALSE;
   }
   else 
   {   (*(xmLabelClassRec.primitive_class.border_unhighlight))((Widget) tb) ;
       } 
}


/*************************************<->*************************************
 *
 *  KeySelect
 *    If the menu system traversal is enabled, do an activate and disarm
 *
 *************************************<->***********************************/
/*ARGUSED*/
static void 
#ifdef _NO_PROTO
KeySelect( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
KeySelect(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmToggleButtonWidget tb = (XmToggleButtonWidget) wid ;
   XmToggleButtonCallbackStruct call_value;

   if (!_XmIsEventUnique(event))
      return;

   if (!_XmGetInDragMode((Widget)tb))
   {
      if (tb->toggle.ind_on)
      {
	 DrawToggle(tb);
      }
      else
      {
	 if (tb->toggle.fill_on_select && !Lab_IsPixmap(tb))
	    DrawToggleLabel(tb);
      }
      if (Lab_IsPixmap(tb))
      {
	 SetAndDisplayPixmap( tb, event, NULL);
      }
      tb->toggle.Armed = FALSE;
      tb->toggle.set = (tb->toggle.set == TRUE) ? FALSE : TRUE;

      if (XmIsRowColumn(XtParent(tb)))
      {
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_BUTTON_POPDOWN, XtParent(tb), NULL, event, NULL);
      }

      _XmRecordEvent(event);

      /* if the parent is a RowColumn, notify it about the select */
      if (XmIsRowColumn(XtParent(tb)))
      {
	 call_value.reason = XmCR_VALUE_CHANGED;
	 call_value.event = event;
	 call_value.set = tb->toggle.set;
	 (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK, 
						    XtParent(tb), FALSE, tb,
						    &call_value);
      }
      
      if ((! tb->label.skipCallback) &&
	  (tb->toggle.value_changed_CB))
      {
	 XFlush(XtDisplay(tb));
	 ToggleButtonCallback(tb, XmCR_VALUE_CHANGED, tb->toggle.set, event);
      }

      if (XmIsRowColumn(XtParent(tb)))
      {
	 (* xmLabelClassRec.label_class.menuProcs)
	    (XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL, 
	    XtParent(tb), NULL, event, NULL);
      }
   }
}

/************************************************************************
 *
 * Compute Space
 *
 ***********************************************************************/
static void 
#ifdef _NO_PROTO
ComputeSpace( tb )
        XmToggleButtonWidget tb ;
#else
ComputeSpace(
        XmToggleButtonWidget tb )
#endif /* _NO_PROTO */
{

   int needed_width;
   int needed_height;

  /* COMPUTE SPACE FOR DRAWING TOGGLE */

   needed_width = tb->label.TextRect.width +
                  tb->label.margin_left + tb->label.margin_right +
                  (2 * (tb->primitive.shadow_thickness +
                        tb->primitive.highlight_thickness +
                        tb->label.margin_width));

   needed_height = tb->label.TextRect.height +
                   tb->label.margin_top + tb->label.margin_bottom +
                   (2 * (tb->primitive.shadow_thickness +
                         tb->primitive.highlight_thickness +
                         tb->label.margin_height));

   if (needed_height > tb->core.height)
       if (tb->toggle.ind_on)
          tb->label.TextRect.y = tb->primitive.shadow_thickness +
                                 tb->primitive.highlight_thickness +
                                 tb->label.margin_height +
                                 tb->label.margin_top +
                                 ((tb->core.height - tb->label.margin_top
                                 - tb->label.margin_bottom
                                 - (2 * (tb->label.margin_height
                                 + tb->primitive.highlight_thickness
                                 + tb->primitive.shadow_thickness))
                                 - tb->label.TextRect.height) / 2);


  if ((needed_width > tb->core.width) ||
     ((tb->label.alignment == XmALIGNMENT_BEGINNING) 
       && (needed_width < tb->core.width)) ||
     ((tb->label.alignment == XmALIGNMENT_CENTER)
       && (needed_width < tb->core.width) 
       && (tb->core.width - needed_width < tb->label.margin_left)) ||
     (needed_width == tb->core.width))
  {

    if (tb->toggle.ind_on)
      tb->label.TextRect.x = tb->primitive.shadow_thickness +
                             tb->primitive.highlight_thickness +
                             tb->label.margin_width +
                             tb->label.margin_left;
  }

} /* ComputeSpace */

/*************************************<->*************************************
 *
 *  Redisplay (w, event, region)
 *     Cause the widget, identified by w, to be redisplayed.
 *
 *************************************<->***********************************/
/*ARGUSED*/
static void 
#ifdef _NO_PROTO
Redisplay( w, event, region )
        Widget w ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget w,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
   register XmToggleButtonWidget tb = (XmToggleButtonWidget) w;
#ifdef CDE_VISUAL	/* etched in menu button */
   Boolean etched_in = False;
#endif

   if (! XtIsRealized(w) ) return;    /* Fix CR #4884, D. Rand 6/4/92 */

#ifdef CDE_VISUAL       /* etched in menu button */
   XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)tb)), "enableEtchedInMenu", &etched_in, NULL);
#endif

   ComputeSpace (tb);

   if (Lab_IsPixmap(tb))
       SetAndDisplayPixmap(tb, event, region);
   else
   {
      if (!tb->toggle.ind_on && tb->toggle.fill_on_select)
       DrawToggleLabel (tb);
      else {
       (* xmLabelClassRec.core_class.expose) (w, event, region);
      }
   }

   if (tb->toggle.ind_on)
   {
      if (!(tb->toggle.Armed))
       IsOn(tb) = tb->toggle.set;
      DrawToggle(tb);
   }

   if (tb -> label.menu_type == XmMENU_PULLDOWN ||
       tb -> label.menu_type == XmMENU_POPUP) 
   {
      if ((tb->toggle.Armed) && 
	  (tb->primitive.shadow_thickness > 0))
#ifdef CDE_VISUAL	/* etched in menu button */
          {
          _XmDrawShadows (XtDisplay (tb), XtWindow (tb),
                tb -> primitive.top_shadow_GC,
                tb -> primitive.bottom_shadow_GC,
                tb ->primitive.highlight_thickness,
                tb ->primitive.highlight_thickness,
                (int)tb->core.width-2*tb->primitive.highlight_thickness,
                (int)tb->core.height-2*tb->primitive.highlight_thickness,
                tb -> primitive.shadow_thickness,
                etched_in ? XmSHADOW_IN : XmSHADOW_OUT);
          }
#else
          _XmDrawShadows (XtDisplay (tb), XtWindow (tb),
                tb -> primitive.top_shadow_GC,
                tb -> primitive.bottom_shadow_GC,
                tb ->primitive.highlight_thickness,
                tb ->primitive.highlight_thickness,
                (int)tb->core.width-2*tb->primitive.highlight_thickness,
                (int)tb->core.height-2*tb->primitive.highlight_thickness,
                tb -> primitive.shadow_thickness, XmSHADOW_OUT);
#endif
   }

   else
   {
      DrawToggleShadow (tb);
      }
   }

/**************************************************************************
 *
 * Resize(w, event)
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
  register XmToggleButtonWidget tb = (XmToggleButtonWidget) w;

/* BEGIN OSF Fix pir 1778 */
  if (Lab_IsPixmap(w)) 
    SetToggleSize(tb);
  else
    (* xmLabelClassRec.core_class.resize)( (Widget) tb);
 /* END OSF Fix pir 1778 */
}

/***************************************************************************
 *
 *  SetValues(current, request, new_w)
 *     This is the set values procedure for the ToggleButton class.  It is
 *     called last (the set values rtnes for its superclasses are called
 *     first).
 *
 *************************************<->***********************************/
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
    XmToggleButtonWidget curcbox = (XmToggleButtonWidget) current;
    XmToggleButtonWidget newcbox = (XmToggleButtonWidget) new_w;
    Boolean  flag = FALSE;    /* our return value */
    
    int maxIndicatorSize;   /* Max Indicator size permissible */
    int delta;
    int boxSize;
 
    /**********************************************************************
     * Calculate the window size:  The assumption here is that if
     * the width and height are the same in the new and current instance
     * record that those fields were not changed with set values.  Therefore
     * its okay to recompute the necessary width and height.  However, if
     * the new and current do have different width/heights then leave them
     * alone because that's what the user wants.
     *********************************************************************/

     if (IsNull (PixmapOff(newcbox)) &&       /* no Off pixmap but do have */
          ! IsNull (PixmapOn(newcbox)))           /* an On, so use that */
     {
         PixmapOff(newcbox) = PixmapOn(newcbox);
         if ((newcbox->label.recompute_size) &&
            (request->core.width == current->core.width))
              new_w->core.width = 0;
         if ((newcbox->label.recompute_size) &&
            (request->core.height == current->core.height))
              new_w->core.height = 0;

         _XmCalcLabelDimensions(new_w);
         (* xmLabelClassRec.core_class.resize)( (Widget) new_w);
     }

     if (IsNull (Pixmap_Insen_Off(newcbox)) &&   /* no On pixmap but do have */
         ! IsNull (Pixmap_Insen_On(newcbox)))      /* an Off, so use that */
     {
         Pixmap_Insen_Off(newcbox) = Pixmap_Insen_On(newcbox);
         if ((newcbox->label.recompute_size) &&
            (request->core.width == current->core.width))
              new_w->core.width = 0;
         if ((newcbox->label.recompute_size) &&
            (request->core.height == current->core.height))
              new_w->core.height = 0;

         _XmCalcLabelDimensions(new_w);
         (* xmLabelClassRec.core_class.resize)( (Widget) new_w);
    }

/* BEGIN OSF Fix pir 1778 */
     /* Have to reset the TextRect width because label's resize will have
        mucked with it. */
    if (Lab_IsPixmap(newcbox) &&
       (!IsNull(PixmapOff(newcbox)) || !IsNull(PixmapOn(newcbox)) ||
        !IsNull(Pixmap_Insen_Off(newcbox)) ||
        !IsNull(Pixmap_Insen_On(newcbox))))
       {
       if ((newcbox->label.recompute_size))
         {
           if (request->core.width == current->core.width)
             new_w->core.width = 0;
           if (request->core.height == current->core.height)
             new_w->core.height = 0;
         }

       SetToggleSize(newcbox);
     }
/* END OSF Fix pir 1778 */

     if ((newcbox->label._label != curcbox->label._label) ||
         (PixmapOff(newcbox) != PixmapOff(curcbox)) ||
         (newcbox->label.font != curcbox->label.font) ||
         (newcbox->toggle.spacing != curcbox->toggle.spacing) ||
         (PixmapOn(newcbox) != PixmapOn(curcbox)) ||
         (Pixmap_Insen_On(newcbox) != Pixmap_Insen_On(curcbox)) ||
         (newcbox->toggle.ind_on != curcbox->toggle.ind_on) ||
         (newcbox->toggle.indicator_dim != curcbox->toggle.indicator_dim) ||
	 (Lab_IsPixmap(newcbox) != Lab_IsPixmap(curcbox)))
     {
       if (newcbox->label.recompute_size)
       {
         if (request->core.width == current->core.width)
            new_w->core.width = 0;
         if (request->core.height == current->core.height)
            new_w->core.height = 0;
       }

       if ((PixmapOn(newcbox) != PixmapOn(curcbox)) ||
           (Pixmap_Insen_On(newcbox) != Pixmap_Insen_On(curcbox)))
       {
        _XmCalcLabelDimensions(new_w);
/* BEGIN OSF Fix pir 1778 */
        SetToggleSize(newcbox);
/* END OSF Fix pir 1778 */
       }

       if (( newcbox->toggle.indicator_dim == XmINVALID_DIMENSION) ||
           ( PixmapOff(newcbox) != PixmapOff(curcbox)))
                 newcbox->toggle.indicator_set = FALSE;

       if (!(newcbox->toggle.indicator_set))
       {

	 if ((newcbox->label._label != curcbox->label._label) ||
	      (PixmapOff(newcbox) != PixmapOff(curcbox)) ||
	      (newcbox->label.font != curcbox->label.font) ||
	      (newcbox->toggle.ind_on != curcbox->toggle.ind_on)) 
	 {
	   if Lab_IsPixmap(new_w)
	   {
	      if (newcbox->label.TextRect.height < 13)
		 newcbox->toggle.indicator_dim = newcbox->label.TextRect.height;
	      else
		 newcbox->toggle.indicator_dim = 13 +
		     (newcbox->label.TextRect.height/13);
	   }
	   else
	   {
	    Dimension height;
	    int line_count;

	    height = _XmStringHeight (newcbox->label.font,
				      newcbox->label._label);
	    line_count = _XmStringLineCount (newcbox->label._label);
/* 
 * Fix for 5203 - Make the calculation for the indicator_dim be the same
 *                as in the Initialize procedure, i.e. Popup and Pulldown
 *                menus should have smaller indicators
 */
	    if (line_count < 1)
              line_count = 1;
            if (newcbox->label.menu_type == XmMENU_POPUP ||
               newcbox->label.menu_type == XmMENU_PULLDOWN) {
              newcbox->toggle.indicator_dim = Max(XmDEFAULT_INDICATOR_DIM,
                (height / ((Dimension)line_count))*2/3);
            } else
                newcbox->toggle.indicator_dim = Max(XmDEFAULT_INDICATOR_DIM,
                  height / ((Dimension)line_count));
/*
 * End 5203 Fix
 */
	   }

	 }
       } 

       if (Lab_IsPixmap(newcbox))
         newcbox->toggle.indicator_set = TRUE;

#ifdef OSF_v1_2_4
       if (newcbox->toggle.ind_on)
       {
#endif /* OSF_v1_2_4 */
 /*
  * Fix CR 5568 - If the indicator is on and the user has changed the
  *             indicator dimension, calculate the new top and bottom
  *             margins in a place where they can effect the core width
  *             and height.
  */
  /*  Recompute the Top and bottom margins and the height of the text
   *  rectangle to  accommodate the size of toggle indicator.
   *  if (we are given a new toggleIndicator size)
   *    { if (user has given new top or bottom margin)
   *           { compute to accomodate new toggle button size;
   *           }
   *       else (user has set new top/bottom margin)
   *           { Recompute margin to accommodate new toogleButtonIndicatorSize;
   *           }
   *    }
   */
    if (newcbox->toggle.indicator_dim != curcbox->toggle.indicator_dim)
    { maxIndicatorSize = (int) (newcbox->toggle.indicator_dim) +
                              2 * (newcbox->primitive.shadow_thickness +
                                          Xm3D_ENHANCE_PIXEL);
      boxSize = (int) (newcbox->label.TextRect.height) +
                 (int) (newcbox->label.margin_top) +
                  (int)(newcbox->label.margin_bottom);
     if (maxIndicatorSize != boxSize)
       { delta = maxIndicatorSize - boxSize;
         if ( newcbox->label.margin_top == curcbox->label.margin_top)
            /* User has not specified new top margin */
           { newcbox->label.margin_top = Max ( XmDEFAULT_TOP_MARGIN,
                         (int) newcbox->label.margin_top + delta/2);
           }
         else
           /* User has sepcified a top margin  and
             Margin must not be less than user specified amount */
          { newcbox->label.margin_top = Max( newcbox->label.margin_top,
                             (newcbox->label.margin_top + delta/2));
          }

         if ( newcbox->label.margin_bottom == curcbox->label.margin_bottom)
            /* User has not specified new bottom margin */
           { newcbox->label.margin_bottom = Max ( XmDEFAULT_BOTTOM_MARGIN,
                         (int) newcbox->label.margin_bottom + delta/2);
           }
         else
           /* User has sepcified a bottom margin  and
             Margin must not be less than user specified amount */
          { newcbox->label.margin_bottom = Max( newcbox->label.margin_bottom,
                             (newcbox->label.margin_bottom + delta/2));
          }
        }
     }


#ifndef OSF_v1_2_4
       if (newcbox->toggle.ind_on)
       {
#endif /* OSF_v1_2_4 */
          if ((newcbox->label.margin_left < (newcbox->toggle.indicator_dim +
                                              newcbox->toggle.spacing)) ||
	      newcbox->toggle.spacing != curcbox->toggle.spacing)
              newcbox->label.margin_left = newcbox->toggle.indicator_dim +
                                           newcbox->toggle.spacing;
       }


       if (newcbox->label.recompute_size)
       {
         if (request->core.width == current->core.width)
            new_w->core.width = 0;
         if (request->core.height == current->core.height)
            new_w->core.height = 0;
       }

       if (new_w->core.width == 0)
       {
         newcbox->core.width =
                   newcbox->label.TextRect.width + 
                   newcbox->label.margin_left + newcbox->label.margin_right +
                   2 * (newcbox->primitive.highlight_thickness +
                        newcbox->primitive.shadow_thickness +
                        newcbox->label.margin_width);

         if (newcbox->core.width == 0)
           newcbox->core.width = 1;

         flag = TRUE;
       }

       if (new_w->core.height == 0)
       {
         newcbox->core.height = Max(newcbox->toggle.indicator_dim,
	     newcbox->label.TextRect.height + 2*newcbox->label.margin_height +
	         newcbox->label.margin_top + newcbox->label.margin_bottom) +
	     2 * (newcbox->primitive.highlight_thickness +
                 newcbox->primitive.shadow_thickness);

         if (newcbox->core.height == 0)
           newcbox->core.height = 1;

         flag = TRUE;
       }


     }

    if ((newcbox->primitive.foreground != curcbox->primitive.foreground) ||
        (newcbox->core.background_pixel != curcbox->core.background_pixel) ||
	(newcbox->toggle.select_color != curcbox->toggle.select_color))
    {
        XtReleaseGC( (Widget) curcbox, curcbox->toggle.select_GC);
        XtReleaseGC( (Widget) curcbox, curcbox->toggle.background_gc);
        GetGC(newcbox);
        flag = TRUE;
    }

    if ((curcbox -> toggle.ind_type != newcbox -> toggle.ind_type) ||
       (curcbox -> toggle.visible != newcbox -> toggle.visible)) 
    {
      if(    !XmRepTypeValidValue( XmRID_INDICATOR_TYPE,
                               newcbox->toggle.ind_type, (Widget) newcbox)    )
      {
         newcbox->toggle.ind_type = curcbox->toggle.ind_type;
      }
       flag = True;
    }

    if (curcbox -> toggle.set != newcbox -> toggle.set) 
    {
      IsOn(newcbox) = newcbox-> toggle.set;	
      if (flag == False && XtIsRealized(newcbox))
	{
	  if (newcbox->toggle.ind_on)
	    DrawToggle (newcbox);
	  else
	    {
	      /* Begin fixing OSF 5946 */ 
	      if(newcbox->primitive.shadow_thickness > 0)
		DrawToggleShadow (newcbox);
	      if (newcbox->toggle.fill_on_select && !Lab_IsPixmap(newcbox))
		DrawToggleLabel (newcbox);
	      if (Lab_IsPixmap(newcbox))
		SetAndDisplayPixmap(newcbox, NULL, NULL);
	      /* End fixing OSF 5946 */ 
	    }
	}

     /**  flag = True;		**/
    }

    return(flag);
}

/***************************************************************
 *
 * XmToggleButtonGetState
 *   This function gets the state of the toggle widget.
 *
 ***************************************************************/
Boolean 
#ifdef _NO_PROTO
XmToggleButtonGetState( w )
        Widget w ;
#else
XmToggleButtonGetState(
        Widget w )
#endif /* _NO_PROTO */
{
    XmToggleButtonWidget tw = (XmToggleButtonWidget) w;

    if( XmIsGadget(w) ) {
      return XmToggleButtonGadgetGetState(w);
    }

    return (tw->toggle.set);
}

/****************************************************************
 *
 * XmTogglebuttonSetState
 *   This function sets the state of the toggle widget.
 *
 ****************************************************************/
void 
#ifdef _NO_PROTO
XmToggleButtonSetState( w, newstate, notify )
        Widget w ;
        Boolean newstate ;
        Boolean notify ;
#else
XmToggleButtonSetState(
        Widget w,
#if NeedWidePrototypes
        int newstate,
        int notify )
#else
        Boolean newstate,
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmToggleButtonWidget tw = (XmToggleButtonWidget) w;

   if( XmIsGadget(w) ) {
      XmToggleButtonGadgetSetState(w, newstate, notify);
      return;
   }

   if (tw->toggle.set != newstate)
   {
      tw->toggle.set = newstate;
      IsOn(tw) = newstate;
      if (XtIsRealized (tw))
      {
         if (tw->toggle.ind_on)
            DrawToggle(tw);
         else
         {
             if (tw->primitive.shadow_thickness > 0)
               DrawToggleShadow (tw);
             if (tw->toggle.fill_on_select && !Lab_IsPixmap(tw))
               DrawToggleLabel (tw);
         }
         if (Lab_IsPixmap(tw))
            SetAndDisplayPixmap( tw, NULL, NULL);
         
      }
      if (notify)
      {
          /* if the parent is a RowColumn, notify it about the select */
          if (XmIsRowColumn(XtParent(tw)))
          {
             XmToggleButtonCallbackStruct call_value;
             call_value.reason = XmCR_VALUE_CHANGED;
             call_value.event = NULL;
             call_value.set = tw->toggle.set;
             (* xmLabelClassRec.label_class.menuProcs) (XmMENU_CALLBACK,
                XtParent(tw), FALSE, tw, &call_value);
          }

          if ((! tw->label.skipCallback) &&
              (tw->toggle.value_changed_CB))
          {
             XFlush(XtDisplay(tw));
             ToggleButtonCallback(tw, XmCR_VALUE_CHANGED, tw->toggle.set, NULL);
          }
      }
   }
} 
  
/***********************************************************************
 *
 * XmCreateToggleButton
 *   Creates an instance of a togglebutton and returns the widget id.
 *
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateToggleButton( parent, name, arglist, argCount )
        Widget parent ;
        char *name ;
        Arg *arglist ;
        Cardinal argCount ;
#else
XmCreateToggleButton(
        Widget parent,
        char *name,
        Arg *arglist,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
    return (XtCreateWidget(name,xmToggleButtonWidgetClass,parent,arglist,argCount));
}
/*
 * DrawToggleShadow (tb)
 *   - Should be called only if ToggleShadow are to be drawn ;
 *	if the IndicatorOn resource is set to false top and bottom shadows
 *	will be switched depending on whether the Toggle is selected or
 *  unselected.
 *   No need to call the routine if shadow_thickness is 0.
 */
static void 
#ifdef _NO_PROTO
DrawToggleShadow( tb )
        XmToggleButtonWidget tb ;
#else
DrawToggleShadow(
        XmToggleButtonWidget tb )
#endif /* _NO_PROTO */
{   
   GC topgc, bottomgc;
   int width, height;
   int hilite_thickness;

   if (!tb->toggle.ind_on)
   { 
      if (IsOn(tb))
      { 
	 topgc = tb -> primitive.bottom_shadow_GC;
	 bottomgc = tb -> primitive.top_shadow_GC;
      }
      else
      {
	 topgc = tb -> primitive.top_shadow_GC;
	 bottomgc = tb -> primitive.bottom_shadow_GC;
      }
   }
   else
   {
      topgc = tb -> primitive.top_shadow_GC;
      bottomgc = tb -> primitive.bottom_shadow_GC;
   }

   hilite_thickness =  tb->primitive.highlight_thickness;
   width  =  (int) (tb->core.width - (hilite_thickness << 1));
   height = (int) (tb->core.height - (hilite_thickness << 1));
	
   _XmDrawShadows (XtDisplay (tb), XtWindow (tb),
                 topgc, bottomgc,
                 hilite_thickness, hilite_thickness, width, height,
                 tb->primitive.shadow_thickness, XmSHADOW_OUT);
}

/*
 * DrawToggleLabel (tb)
 *    Called when XmNindicatorOn  is set to false and XmNfillOnSelect
 *    is set true. Fill toggle with selectColor or background
 *    depending on toggle value, and draw label.
 */
static void 
#ifdef _NO_PROTO
DrawToggleLabel( tb )
        XmToggleButtonWidget tb ;
#else
DrawToggleLabel(
        XmToggleButtonWidget tb )
#endif /* _NO_PROTO */
{
    Dimension margin = tb->primitive.highlight_thickness +
                      tb->primitive.shadow_thickness;
    Position fx = margin;
    Position fy = margin;
    int fw = tb->core.width - 2 * margin;
    int fh = tb->core.height - 2 * margin;
    Boolean restore_gc = False;
    GC tmp_gc = NULL;
#ifdef CDE_VISUAL
    Boolean etched_in=False;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay((Widget)tb)), 
			"enableEtchedInMenu", &etched_in, NULL);
#endif

    if (tb->primitive.top_shadow_color == tb->toggle.select_color ||
      tb->primitive.bottom_shadow_color == tb->toggle.select_color)
    {
      fx += 1;
      fy += 1;
      fw -= 2;
      fh -= 2;
    }

    if (fw < 0 || fh < 0)
      return;

#ifdef CDE_VISUAL
    if (etched_in)
    {   
        GetExtension((Widget)tb);
        XFillRectangle (XtDisplay(tb), XtWindow(tb), 
			IsArmed(tb)? tb_extension->arm_GC:
				tb_extension->normal_GC,
                        fx, fy, fw, fh);
    }
    else
#endif
    XFillRectangle (XtDisplay(tb), XtWindow(tb), (IsOn(tb) ?
                      tb->toggle.select_GC : 
		      tb->toggle.background_gc),
                      fx, fy, fw, fh);


    if (tb->primitive.foreground == tb->toggle.select_color &&
      IsOn(tb))
    {
        tmp_gc = tb->label.normal_GC;
#ifdef CDE_VISUAL
        tb->label.normal_GC = IsArmed(tb) ? tb_extension->arm_GC:
				tb->toggle.background_gc;
#else
        tb->label.normal_GC = tb->toggle.background_gc;
#endif
        restore_gc = True;
    }

    (* xmLabelClassRec.core_class.expose) ((Widget) tb, NULL, NULL);

    if (restore_gc)
      tb->label.normal_GC = tmp_gc;
}


/* BEGIN OSF Fix pir 1778 */
/*************************************************************************
 *
 * SetToggleSize(newtb)
 * Set size properly when XmNselectPixmap or XmNselectInsensitivePixmaps
 * are set in addition to the corresponding labelPixmaps.  Have to pick
 * the largest dimensions.
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
SetToggleSize(newtb)
     XmToggleButtonWidget newtb;
#else
SetToggleSize(
     XmToggleButtonWidget newtb)
#endif /* _NO_PROTO */
{
   XmLabelPart         *lp = &(newtb->label);

#ifndef OSF_v1_2_4
   unsigned int junk;
   unsigned int  onW = 0 , onH = 0, offW = 0, offH = 0, d;
#else /* OSF_v1_2_4 */
   Window root;
   int x,y;
   unsigned int  onW = 0, onH = 0, offW = 0, offH = 0, border, d;
#endif /* OSF_v1_2_4 */

    /* initialize TextRect width and height to 0, change later if needed */
    lp->TextRect.width = 0;
    lp->TextRect.height = 0;
    lp->acc_TextRect.width = 0;
    lp->acc_TextRect.height = 0;

   /* We know it's a pixmap so find out how how big it is */
#ifndef OSF_v1_2_4
   if ((newtb->core.sensitive) && (newtb->core.ancestor_sensitive))
#else /* OSF_v1_2_4 */
   if (XtIsSensitive((Widget) newtb))
#endif /* OSF_v1_2_4 */
     {
       if (!IsNull(PixmapOn(newtb)))
       XGetGeometry (XtDisplay(newtb),
                     PixmapOn(newtb),
#ifndef OSF_v1_2_4
                     (Window*)&junk, /* returned root window */
                     (int*)&junk, (int*)&junk, /* x, y of pixmap */
                     &onW, &onH, /* width, height of pixmap */
                     &junk,    /* border width */
                     &d);      /* depth */
#else /* OSF_v1_2_4 */
		     &root,	/* returned root window */
                     &x, &y,	/* returned x, y of pixmap */
                     &onW, &onH, /* returned width, height of pixmap */
                     &border,	/* returned border width */
                     &d);	/* returned depth */
#endif /* OSF_v1_2_4 */

       if (!IsNull(PixmapOff(newtb)))
       XGetGeometry (XtDisplay(newtb),
                     PixmapOff(newtb),
#ifndef OSF_v1_2_4
                     (Window*)&junk, /* returned root window */
                     (int*)&junk, (int*)&junk, /* x, y of pixmap */
                     &offW, &offH, /* width, height of pixmap */
                     &junk,    /* border width */
                     &d);      /* depth */
#else /* OSF_v1_2_4 */
		     &root,	/* returned root window */
                     &x, &y,	/* returned x, y of pixmap */
                     &offW, &offH, /* returned width, height of pixmap */
                     &border,	/* returned border width */
                     &d);	/* returned depth */
#endif /* OSF_v1_2_4 */

     }
   else
     {
       if (!IsNull(Pixmap_Insen_On(newtb)))
       XGetGeometry (XtDisplay(newtb),
                     Pixmap_Insen_On(newtb),
#ifndef OSF_v1_2_4
                     (Window*)&junk, /* returned root window */
                     (int*)&junk, (int*)&junk, /* x, y of pixmap */
                     &onW, &onH, /* width, height of pixmap */
                     &junk,    /* border width */
                     &d);      /* depth */
#else /* OSF_v1_2_4 */
		     &root,	/* returned root window */
                     &x, &y,	/* returned x, y of pixmap */
                     &onW, &onH, /* returned width, height of pixmap */
                     &border,	/* returned border width */
                     &d);	/* returned depth */
#endif /* OSF_v1_2_4 */

       if (!IsNull(Pixmap_Insen_Off(newtb)))
       XGetGeometry (XtDisplay(newtb),
                     Pixmap_Insen_Off(newtb),
#ifndef OSF_v1_2_4
                     (Window*)&junk, /* returned root window */
                     (int*)&junk, (int*)&junk, /* x, y of pixmap */
                     &offW, &offH, /* width, height of pixmap */
                     &junk,    /* border width */
                     &d);      /* depth */
#else /* OSF_v1_2_4 */
		     &root,	/* returned root window */
                     &x, &y,	/* returned x, y of pixmap */
                     &offW, &offH, /* returned width, height of pixmap */
                     &border,	/* returned border width */
                     &d);	/* returned depth */
#endif /* OSF_v1_2_4 */

     }
   lp->TextRect.width = (unsigned short) ((onW > offW) ? onW : offW);
   lp->TextRect.height = (unsigned short) ((onH > offH) ? onH : offH);

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

    if (newtb->core.width == 0)
        newtb->core.width = (Dimension)
          lp->TextRect.width +
              lp->margin_left + lp->margin_right +
                  (2 * (lp->margin_width
                        + newtb->primitive.highlight_thickness
                        + newtb->primitive.shadow_thickness));

    switch (lp -> alignment)
    {
     case XmALIGNMENT_BEGINNING:
       lp->TextRect.x = (short) lp->margin_width +
         lp->margin_left +
             newtb->primitive.highlight_thickness +
                 newtb->primitive.shadow_thickness;

       break;

     case XmALIGNMENT_END:
       lp->TextRect.x = (short) newtb->core.width -
         (newtb->primitive.highlight_thickness +
          newtb->primitive.shadow_thickness +
          lp->margin_width + lp->margin_right +
          lp->TextRect.width);
       break;

     default:
       lp->TextRect.x =  (short) newtb->primitive.highlight_thickness
         + newtb->primitive.shadow_thickness
             + lp->margin_width + lp->margin_left +
                 ((newtb->core.width - lp->margin_left
                   - lp->margin_right
                   - (2 * (lp->margin_width
                           + newtb->primitive.highlight_thickness
                           + newtb->primitive.shadow_thickness))
                   - lp->TextRect.width) / 2);

       break;
    }

    /* Has a height been specified? */

    if (newtb->core.height == 0)
        newtb->core.height = (Dimension)
          Max(lp->TextRect.height, lp->acc_TextRect.height) +
              lp->margin_top +
                  lp->margin_bottom
                      + (2 * (lp->margin_height
                              + newtb->primitive.highlight_thickness
                              + newtb->primitive.shadow_thickness));

    lp->TextRect.y =  (short) newtb->primitive.highlight_thickness
        + newtb->primitive.shadow_thickness
          + lp->margin_height + lp->margin_top +
              ((newtb->core.height - lp->margin_top
                - lp->margin_bottom
                - (2 * (lp->margin_height
                       + newtb->primitive.highlight_thickness
                        + newtb->primitive.shadow_thickness))
                - lp->TextRect.height) / 2);

    if (lp->_acc_text != NULL)
    {

       lp->acc_TextRect.x = (short) newtb->core.width -
         newtb->primitive.highlight_thickness -
             newtb->primitive.shadow_thickness -
                 newtb->label.margin_width -
                     newtb->label.margin_right +
                         LABEL_ACC_PAD;

       lp->acc_TextRect.y =  (short) newtb->primitive.highlight_thickness
         + newtb->primitive.shadow_thickness
             + lp->margin_height + lp->margin_top +
                 ((newtb->core.height - lp->margin_top
                   - lp->margin_bottom
                   - (2 * (lp->margin_height
                           + newtb->primitive.highlight_thickness
                           + newtb->primitive.shadow_thickness))
                   - lp->acc_TextRect.height) / 2);

     }

   if (newtb->core.width == 0)    /* set core width and height to a */
     newtb->core.width = 1;       /* default value so that it doesn't */
   if (newtb->core.height == 0)   /* generate a Toolkit Error */
     newtb->core.height = 1;
}
/* END OSF Fix pir 1778 */
