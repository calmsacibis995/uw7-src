#pragma ident	"@(#)m1.2libs:Xm/XmP.h	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/************************************<+>*************************************
 ****************************************************************************
 **
 **   File:        XmP.h
 **
 **   Description: This include file contains the class and instance record
 **                definitions for all meta classes.  It also contains externs
 **                for internally shared functions and defines for internally 
 **                shared values.
 **
 ****************************************************************************
 ************************************<+>*************************************/
#ifndef _XmP_h
#define _XmP_h

#include <Xm/Xm.h>
#include <X11/IntrinsicP.h>


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************
 *  Macros replacing toolkit macros so that gadgets are handled properly. 
 ***************************************************************************/

#ifdef XtName
#undef XtName
#endif
#define XtName(widget) XrmQuarkToString (((Object)(widget))->object.xrm_name)

#ifdef XtDisplay
#undef XtDisplay
#endif
#define XtDisplay(widget)						\
   (XtIsWidget(widget)         ?					\
      ((Widget)(widget))->core.screen->display :			\
      ((Object)(widget))->object.parent->core.screen->display)

#ifdef XtScreen
#undef XtScreen
#endif
#define XtScreen(widget) (XtIsWidget(widget)  ?		\
			 ((Widget)(widget))->core.screen   :	\
			 ((Object)(widget))->object.parent->core.screen)

#ifdef XtWindow
#undef XtWindow
#endif
#define XtWindow(widget) (XtIsWidget(widget)  ?			\
			 ((Widget)(widget))->core.window   :		\
			 ((Object)(widget))->object.parent->core.window)

#ifdef XtClass
#undef XtClass
#endif
#define XtClass(widget)	(((Object)(widget))->object.widget_class)

#ifdef XtSuperclass
#undef XtSuperclass
#endif
#define XtSuperclass(widget) (XtClass(widget)->core_class.superclass)


#ifdef XtIsRealized
#undef XtIsRealized
#endif
#define XtIsRealized(widget) (XtWindow((Widget)widget) != None)

#ifdef XtIsManaged
#undef XtIsManaged
#endif
#define XtIsManaged(widget) 					\
  (XtIsRectObj(widget) ? (((RectObj)(widget))->rectangle.managed) : False)


#ifdef XtIsSensitive
#undef XtIsSensitive
#endif
#define XtIsSensitive(widget) 					\
  ((!XtIsRectObj(widget)) ? False : 				\
   (((RectObj)(widget))->rectangle.sensitive && 		\
    ((RectObj)(widget))->rectangle.ancestor_sensitive))

#ifdef XtParent
#undef XtParent
#endif
#define XtParent(widget) (((Object)(widget))->object.parent)

#define XtX(w)		 ((w)->core.x)
#define XtY(w)		 ((w)->core.y)
#define XtWidth(w)	 ((w)->core.width)
#define XtHeight(w)	 ((w)->core.height)
#define XtBorderWidth(w) ((w)->core.border_width)
#define XtBackground(w)	 ((w)->core.background_pixel)
#define XtSensitive(w)	 ((w)->core.sensitive && (w)->core.ancestor_sensitive) 

#define XtCoreProc(w,proc) ((w)->core.widget_class->core_class.proc)


/***********************************************************************
 *
 * Miscellaneous Private Defines
 *
 ***********************************************************************/

#ifndef XmConst
#if defined(__STDC__)  ||  !defined( NO_CONST )	/* NovellPERF */
#define XmConst const
#else
#define XmConst
#endif /* __STDC__ */
#endif /* XmConst */


/* Defines used for menu/button communication */
enum{	XmMENU_POPDOWN,			XmMENU_PROCESS_TREE,
	XmMENU_TRAVERSAL,		XmMENU_SHELL_POPDOWN,
	XmMENU_CALLBACK,		XmMENU_BUTTON,
	XmMENU_CASCADING,		XmMENU_SUBMENU,
	XmMENU_ARM,			XmMENU_DISARM,
	XmMENU_BAR_CLEANUP,		XmMENU_STATUS,
	XmMENU_MEMWIDGET_UPDATE,	XmMENU_BUTTON_POPDOWN,
	XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL,
	XmMENU_RESTORE_TEAROFF_TO_TOPLEVEL_SHELL,
	XmMENU_RESTORE_TEAROFF_TO_MENUSHELL,
	XmMENU_GET_LAST_SELECT_TOPLEVEL,
	XmMENU_TEAR_OFF_ARM
	} ;

/***********************************************************************
 *
 * Status for menus
 *
 ***********************************************************************/

#define XmMENU_TORN_BIT                         (1 << 0)
#define XmMENU_TEAR_OFF_SHELL_DESCENDANT_BIT    (1 << 1)
#define XmMENU_POPUP_POSTED_BIT			(1 << 2)

#define XmIsTorn(mask)                          (mask & XmMENU_TORN_BIT)
#define XmIsTearOffShellDescendant(mask)        (mask & XmMENU_TEAR_OFF_SHELL_DESCENDANT_BIT)
#define XmPopupPosted(mask)        		(mask & XmMENU_POPUP_POSTED_BIT)


#ifdef _NO_PROTO
typedef void (*XmMenuProc)() ;
#else
typedef void (*XmMenuProc)( int, Widget, ...) ;
#endif

/***********************************************************************
 *
 * Simple Menu Structure
 *
 ***********************************************************************/

typedef struct _XmSimpleMenuRec {
	int count;
	int post_from_button;
	XtCallbackProc callback;
	XmStringTable label_string;
	String *accelerator;
	XmStringTable accelerator_text;
	XmKeySymTable mnemonic;
	XmStringCharSetTable mnemonic_charset;
	XmButtonTypeTable button_type;
	int button_set;
	XmString option_label;
        KeySym option_mnemonic;
} XmSimpleMenuRec, * XmSimpleMenu;


/* For MapEvent: _XmMatchBtnEvent */
#define XmIGNORE_EVENTTYPE      -1

/* Default minimum Toggle indicator dimension */
#define XmDEFAULT_INDICATOR_DIM   9

/************************************************************************
 * defines needed for 3D visual enhancement of defaultButtonshadow and
 *  implementation of ToggleButton Indicatorsize.
 ************************************************************************/

#define Xm3D_ENHANCE_PIXEL		2
#define XmDEFAULT_TOP_MARGIN		0
#define XmDEFAULT_BOTTOM_MARGIN		0



/************************************************************************
 *
 *  Resource definition function and structure used by Primitive, Gadget
 *  and Manager to define there get values hook processing lists
 *
 ************************************************************************/

typedef enum{ XmSYNTHETIC_NONE, XmSYNTHETIC_LOAD } XmImportOperator ;

#ifdef _NO_PROTO
typedef void (*XmExportProc)();
typedef XmImportOperator (*XmImportProc)();
#else
typedef void (*XmExportProc)( Widget, int, XtArgVal *) ;
typedef XmImportOperator (*XmImportProc)( Widget, int, XtArgVal *) ;
#endif

typedef struct _XmSyntheticResource
{
   String   resource_name;
   Cardinal resource_size;
   Cardinal resource_offset;
   XmExportProc export_proc;
   XmImportProc import_proc;
} XmSyntheticResource;


/*  Structure and defines for parent process data */

typedef struct
{
   int          process_type ;  /* Common to all parent process records. */
   } XmParentProcessAnyRec ;

typedef struct
{ 
   int          process_type ;  /* Common to all parent process records. */
   XEvent *     event ;
   int          action ;
   String *     params ;
   Cardinal *   num_params ;
} XmParentInputActionRec ;

typedef union
{
   XmParentProcessAnyRec  any ;
   XmParentInputActionRec input_action ;
} XmParentProcessDataRec, * XmParentProcessData ;

enum{   XmPARENT_PROCESS_ANY,  XmINPUT_ACTION
	} ;
enum{	XmPARENT_ACTIVATE,		XmPARENT_CANCEL
	} ;
#define XmRETURN XmPARENT_ACTIVATE       /* For Motif 1.1 BC. */
#define XmCANCEL XmPARENT_CANCEL         /* For Motif 1.1 BC. */


#define XmINVALID_DIMENSION (0xFFFF)

enum{	XmBASELINE_GET,			XmBASELINE_SET
	} ;

typedef struct _XmBaselineMargins
{
  unsigned char get_or_set;
  Dimension margin_top;
  Dimension margin_bottom;
  Dimension shadow;
  Dimension highlight;
  Dimension text_height;
  Dimension margin_height;
} XmBaselineMargins;


typedef enum{ XmFOCUS_IN, XmFOCUS_OUT, XmENTER, XmLEAVE } XmFocusChange ;

typedef enum{
        XmNOT_NAVIGABLE,                XmCONTROL_NAVIGABLE,
	XmTAB_NAVIGABLE,                XmDESCENDANTS_NAVIGABLE,
	XmDESCENDANTS_TAB_NAVIGABLE
  } XmNavigability ;

#define XmVoidProc      XtProc

#ifdef _NO_PROTO

typedef Boolean (*XmParentProcessProc)() ;
typedef void (*XmWidgetDispatchProc)() ;
typedef void (*XmMenuPopupProc)() ;
typedef void (*XmMenuTraversalProc)() ;
typedef void (*XmResizeFlagProc)() ;
typedef void (*XmRealizeOutProc)() ;
typedef Boolean (*XmVisualChangeProc)();  /* returns TRUE if redisplay */
typedef void (*XmTraversalProc)();
typedef void (*XmFocusMovedProc)() ;
typedef void (*XmCacheCopyProc)() ;
typedef void (*XmGadgetCacheProc)() ;
typedef int (*XmCacheCompareProc)() ;
typedef Boolean (*XmWidgetBaselineProc)();
typedef Boolean (*XmWidgetDisplayRectProc)();
typedef void (*XmWidgetMarginsProc)();
typedef XmNavigability (*XmWidgetNavigableProc)() ;
typedef void (*XmFocusChangeProc)() ;

#else

typedef Boolean (*XmParentProcessProc)( Widget, XmParentProcessData) ;
typedef void (*XmWidgetDispatchProc)( Widget, XEvent *, Mask) ;
typedef void (*XmMenuPopupProc)( Widget, Widget, XEvent *) ;
typedef void (*XmMenuTraversalProc)( Widget, Widget, XmTraversalDirection) ;
typedef void (*XmResizeFlagProc)(
			Widget,
#if NeedWidePrototypes
			int) ;
#else
			Boolean) ;
#endif /* NeedWidePrototypes */
typedef void (*XmRealizeOutProc)( Widget, Mask *, XSetWindowAttributes *) ;
typedef Boolean (*XmVisualChangeProc)( Widget, Widget, Widget) ;
typedef void (*XmTraversalProc)( Widget, XtPointer, XtPointer, int) ;
typedef void (*XmFocusMovedProc)( Widget, XtPointer, XtPointer) ;
typedef void (*XmCacheCopyProc)( XtPointer, XtPointer, size_t) ;
typedef void (*XmGadgetCacheProc)( XtPointer) ;
typedef int (*XmCacheCompareProc)( XtPointer, XtPointer) ;
typedef Boolean (*XmWidgetBaselineProc)(Widget, Dimension **, int *);
typedef Boolean (*XmWidgetDisplayRectProc)(Widget, XRectangle *);
typedef void (*XmWidgetMarginsProc)(Widget, XmBaselineMargins *);
typedef XmNavigability (*XmWidgetNavigableProc)( Widget) ;
typedef void (*XmFocusChangeProc)(Widget, XmFocusChange);
#endif


/****************
 *
 * Data structure for building a real translation table out of a 
 * virtual string.
 *
 ****************/
 typedef struct {
     Modifiers mod;
     char      *key;
     char      *action;
  } _XmBuildVirtualKeyStruct;
              
/***********************************************************************
 *
 * Types shared by Text and TextField widgets
 *
 ***********************************************************************/

/*
 * This struct is for support of Insert Selection targets.
 */
typedef struct {
    Atom selection;
    Atom target;
} _XmTextInsertPair;

typedef struct {
    XmTextPosition position;    /* Starting position. */
    XmHighlightMode mode;       /* Highlighting mode for this position. */
} _XmHighlightRec;

typedef struct {
    Cardinal number;            /* Number of different highlight areas. */
    Cardinal maximum;           /* Number we've allocated space for. */
    _XmHighlightRec *list;      /* Pointer to array of highlight data. */
} _XmHighlightData;

typedef enum { XmDEST_SELECT, XmPRIM_SELECT } XmSelectType;

typedef struct {
    Boolean done_status;	/* completion status of insert selection */
    Boolean success_status;	/* success status of insert selection */
    XmSelectType select_type;	/* insert selection type */
    XSelectionRequestEvent *event; /* event that initiated the
				      insert selection */
} _XmInsertSelect;

typedef struct {
    XEvent *event;
    String *params;
    Cardinal *num_params;
} _XmTextActionRec;

typedef struct {
    Widget widget;
    XmTextPosition insert_pos;
    int num_chars;
    Time timestamp;
    Boolean move;
} _XmTextDropTransferRec;

typedef struct {
    XmTextPosition position;
    Atom target;
    Time time;
    int num_chars;
    int ref_count;
} _XmTextPrimSelect;

typedef struct {
    Screen *screen;
    XContext context;
    unsigned char type;
} XmTextContextDataRec, *XmTextContextData;

enum {_XM_IS_DEST_CTX, _XM_IS_GC_DATA_CTX, _XM_IS_PIXMAP_CTX};

#define XmTEXT_DRAG_ICON_WIDTH 64
#define XmTEXT_DRAG_ICON_HEIGHT 64
#define XmTEXT_DRAG_ICON_X_HOT 10
#define XmTEXT_DRAG_ICON_Y_HOT 4


/***********************************************************************
 *
 * Types and functions for Geometry Utilities
 *
 ***********************************************************************/

/* Defines used by geometry manager utilities */

enum{	XmGET_ACTUAL_SIZE = 1,		XmGET_PREFERRED_SIZE,
	XmGEO_PRE_SET,			XmGEO_POST_SET
	} ;

/* Defaults for Geometry Utility defines are always 0.
*/
enum{	XmGEO_EXPAND,			XmGEO_CENTER,
	XmGEO_PACK
	} ;
enum{	XmGEO_PROPORTIONAL,		XmGEO_AVERAGING,
	XmGEO_WRAP
	} ;
enum{	XmGEO_ROW_MAJOR,		XmGEO_COLUMN_MAJOR
	} ;
/* XmGEO_COLUMN_MAJOR is not yet supported. */


typedef struct _XmGeoMatrixRec *XmGeoMatrix ;
typedef union _XmGeoMajorLayoutRec *XmGeoMajorLayout ;
typedef struct _XmKidGeometryRec
{
    Widget   kid;				/* ptr to kid */
    XtWidgetGeometry	box;			/* kid geo box */
} XmKidGeometryRec, *XmKidGeometry;

#ifdef _NO_PROTO
typedef void (*XmGeoArrangeProc)() ;
typedef Boolean (*XmGeoExceptProc)() ;
typedef void (*XmGeoExtDestructorProc)() ;
typedef void (*XmGeoSegmentFixUpProc)() ;
#else
typedef void (*XmGeoArrangeProc)( XmGeoMatrix,
#if NeedWidePrototypes
				 int, int,
#else
				 Position, Position,
#endif /* NeedWidePrototypes */
				 Dimension *, Dimension *) ;
typedef Boolean (*XmGeoExceptProc)( XmGeoMatrix ) ;
typedef void (*XmGeoExtDestructorProc)( XtPointer ) ;
typedef void (*XmGeoSegmentFixUpProc)( XmGeoMatrix, int, XmGeoMajorLayout,
                                                               XmKidGeometry) ;
#endif

typedef struct
{   Boolean         end ;        /* Flag to mark end of rows.                */
    XmGeoSegmentFixUpProc fix_up ;/* Used for non-ordinary layouts.          */
    Dimension       even_width ; /* If non-zero, set all boxes to same width.*/
    Dimension       even_height ;/* If non-zero, set all boxes to same height*/
    Dimension       min_height ; /* Minimum height, if stretch_height TRUE.  */
    Boolean         stretch_height ;/* Stretch height to fill vertically.    */
    Boolean         uniform_border ;/* Enforce on all kids this row, dflt F. */
    Dimension       border ;        /* Value to use if uniform_border set.   */
    unsigned char   fill_mode ; /* Possible values: XmGEO_PACK, XmGEO_CENTER,*/
				/*   or XmGEO_EXPAND (default).              */
    unsigned char   fit_mode ;  /* Method for fitting boxes into space,      */
                /* XmGEO_PROPORTIONAL (dflt), XmGEO_AVERAGING, or XmGEO_WRAP.*/
    Boolean         sticky_end ;  /* Last box in row sticks to edge, dflt F. */
    Dimension       space_above ; /* Between-line spacing, default 0.        */
    Dimension       space_end ;   /* End spacing (XmGEO_CENTER), default 0.  */
    Dimension       space_between ; /* Internal spacing, default 0.          */
    Dimension       max_box_height ;/* Set during arrange routine.           */
    Dimension       boxes_width ;   /* Set during arrange routine.           */
    Dimension       fill_width ;    /* Set during arrange routine.           */
    Dimension       box_count ;     /* Set during arrange routine.           */
    } XmGeoRowLayoutRec, *XmGeoRowLayout ;

typedef struct
{   Boolean         end ;        /* Flag to mark end of columns.             */
    XmGeoSegmentFixUpProc fix_up ;/* Used for non-ordinary layouts.          */
    Dimension       even_height ;/* If non-zero, set all boxes to same height*/
    Dimension       even_width ; /* If non-zero, set all boxes to same width.*/
    Dimension       min_width ;  /* Minimum width, if stretch_width TRUE.  */
    Boolean         stretch_width ;/* Stretch width to fill horizontally.    */
    Boolean         uniform_border ;/* Enforce on all kids this row, dflt F. */
    Dimension       border ;        /* Value to use if uniform_border set.   */
    unsigned char   fill_mode ; /* Possible values: XmGEO_PACK, XmGEO_CENTER,*/
				/*   or XmGEO_EXPAND (default).              */
    unsigned char   fit_mode ;  /* Method for fitting boxes into space,      */
                /* XmGEO_PROPORTIONAL (dflt), XmGEO_AVERAGING, or XmGEO_WRAP.*/
    Boolean         sticky_end ;  /* Last box in row sticks to edge, dflt F. */
    Dimension       space_left ;  /* Between-column spacing, default 0.      */
    Dimension       space_end ;   /* End spacing (XmGEO_CENTER), default 0.  */
    Dimension       space_between ; /* Internal spacing, default 0.          */
    Dimension       max_box_width ; /* Set during arrange routine.           */
    Dimension       boxes_height ;  /* Set during arrange routine.           */
    Dimension       fill_height ;   /* Set during arrange routine.           */
    Dimension       box_count ;     /* Set during arrange routine.           */
    } XmGeoColumnLayoutRec, *XmGeoColumnLayout ;

typedef union _XmGeoMajorLayoutRec
{
  XmGeoRowLayoutRec row ;
  XmGeoColumnLayoutRec col ;
} XmGeoMajorLayoutRec ;

typedef struct _XmGeoMatrixRec
{   Widget          composite ;     /* Widget managing layout.               */
    Widget          instigator ;    /* Widget initiating re-layout.          */
    XtWidgetGeometry instig_request ;/* Geometry layout request of instigatr.*/
    XtWidgetGeometry parent_request ;/* Subsequent layout request to parent. */
    XtWidgetGeometry *in_layout ;   /* Geo. of instig. in layout (after Get).*/
    XmKidGeometry   boxes ;/* Array of boxes, lines separated by NULL record.*/
    XmGeoMajorLayout layouts ;      /* Array of major_order format info.     */
    Dimension       margin_w ;/*Sum of margin, highlight, & shadow thickness.*/
    Dimension       margin_h ;/*Sum of margin, highlight, & shadow thickness.*/
    Boolean         stretch_boxes ; /* Set during arrange routine.           */
    Boolean         uniform_border ;/* Enforce on all kids, default FALSE.   */
    Dimension       border ;	    /* Value to use if uniform_border TRUE.  */
    Dimension       max_major ;     /* Set during arrange routine.           */
    Dimension       boxes_minor ;   /* Set during arrange routine.           */
    Dimension       fill_minor ;    /* Set during arrange routine.           */
    Dimension       width ;         /* Set during arrange routine.           */
    Dimension       height ;        /* Set during arrange routine.           */
    XmGeoExceptProc set_except ;
    XmGeoExceptProc almost_except ;
    XmGeoExceptProc no_geo_request ;
    XtPointer       extension ;
    XmGeoExtDestructorProc ext_destructor ;
    XmGeoArrangeProc arrange_boxes ;/* For user-defined arrangement routine. */
    unsigned char   major_order ;
    } XmGeoMatrixRec;

#ifdef _NO_PROTO
typedef XmGeoMatrix (*XmGeoCreateProc)() ;
#else
typedef XmGeoMatrix (*XmGeoCreateProc)( Widget, Widget, XtWidgetGeometry *) ;
#endif


#define XmInheritCallbackProc ((XtCallbackProc) _XtInherit)
#define XmInheritTraversalProc ((XmTraversalProc) _XtInherit)
#define XmInheritParentProcess ((XmParentProcessProc) _XtInherit)
#define XmInheritWidgetProc ((XtWidgetProc) _XtInherit)
#define XmInheritMenuProc ((XmMenuProc) _XtInherit)
#define XmInheritTranslations XtInheritTranslations
#define XmInheritCachePart	((XmCacheClassPartPtr) _XtInherit)
#define XmInheritBaselineProc ((XmWidgetBaselineProc) _XtInherit)
#define XmInheritDisplayRectProc ((XmWidgetDisplayRectProc) _XtInherit)
#define XmInheritMarginsProc ((XmWidgetMarginsProc) _XtInherit)
#define XmInheritGeoMatrixCreate ((XmGeoCreateProc) _XtInherit)
#define XmInheritFocusMovedProc ((XmFocusMovedProc) _XtInherit)
#define XmInheritClass		   ((WidgetClass) &_XmInheritClass)
#define XmInheritInitializePrehook ((XtInitProc) _XtInherit)
#define XmInheritSetValuesPrehook  ((XtSetValuesFunc) _XtInherit)
#define XmInheritGetValuesPrehook  ((XtArgsProc) _XtInherit)
#define XmInheritInitializePosthook ((XtInitProc) _XtInherit)
#define XmInheritSetValuesPosthook  ((XtSetValuesFunc) _XtInherit)
#define XmInheritGetValuesPosthook  ((XtArgsProc) _XtInherit)
#define XmInheritSecObjectCreate   ((XtInitProc) _XtInherit)
#define XmInheritGetSecResData	   ((XmGetSecResDataFunc) _XtInherit)
#define XmInheritInputDispatch	   ((XmWidgetDispatchProc) _XtInherit)
#define XmInheritVisualChange	   ((XmVisualChangeProc) _XtInherit)
#define XmInheritArmAndActivate	   ((XtActionProc) _XtInherit)
#define XmInheritActionProc	   ((XtActionProc) _XtInherit)
#define XmInheritFocusChange       ((XmFocusChangeProc) _XtInherit)
#define XmInheritWidgetNavigable   ((XmWidgetNavigableProc) _XtInherit)
#define XmInheritClassPartInitPrehook ((XtWidgetClassProc) _XtInherit)
#define XmInheritClassPartInitPosthook ((XtWidgetClassProc) _XtInherit)
#define XmInheritBorderHighlight   ((XtWidgetProc) _XtInherit)
#define XmInheritBorderUnhighlight   ((XtWidgetProc) _XtInherit)

/*  XtInheritFocusMovedProc is provided for backwards compatibility.
 *  Its use is deprecated.
 */
#define XtInheritFocusMovedProc XmInheritFocusMovedProc

/************************************************************************
 *
 *  Fast subclassing macros and definitions
 *
 ************************************************************************/
/* WARNING:  Application subclasses which choose to use fast
 *           subclassing must use only those bits between
 *           192 (XmFIRST_APPLICATION_SUBCLASS_BIT) and 255.
 *           All other fast subclass bits are reserved for
 *           future use.  Use of reserved fast subclass bits
 *           will cause binary compatibility breaks with
 *           future Motif versions.
 */
#define XmFIRST_APPLICATION_SUBCLASS_BIT    192

enum{	XmCASCADE_BUTTON_BIT = 1,	XmCASCADE_BUTTON_GADGET_BIT,
	XmCOMMAND_BOX_BIT,		XmDIALOG_SHELL_BIT,
	XmLIST_BIT,			XmFORM_BIT,
	XmTEXT_FIELD_BIT,		XmGADGET_BIT,
	XmLABEL_BIT,			XmLABEL_GADGET_BIT,
	XmMAIN_WINDOW_BIT,		XmMANAGER_BIT,
	XmMENU_SHELL_BIT,		XmDRAWN_BUTTON_BIT,
	XmPRIMITIVE_BIT,		XmPUSH_BUTTON_BIT,
	XmPUSH_BUTTON_GADGET_BIT,	XmROW_COLUMN_BIT,
	XmSCROLL_BAR_BIT,		XmSCROLLED_WINDOW_BIT,
	XmSELECTION_BOX_BIT,		XmSEPARATOR_BIT,
	XmSEPARATOR_GADGET_BIT,		XmTEXT_BIT,
	XmTOGGLE_BUTTON_BIT,		XmTOGGLE_BUTTON_GADGET_BIT,
	XmDROP_TRANSFER_BIT,		
          /* 28 is available */
	XmDROP_SITE_MANAGER_BIT, /* This is 29. */
	XmDISPLAY_BIT, XmSCREEN_BIT,
	XmARROW_BUTTON_BIT = 32,
	XmARROW_BUTTON_GADGET_BIT,	XmBULLETIN_BOARD_BIT,
	XmDRAWING_AREA_BIT,		XmFILE_SELECTION_BOX_BIT,
	XmFRAME_BIT,			XmMESSAGE_BOX_BIT,
	XmSASH_BIT,			XmSCALE_BIT,
	XmPANED_WINDOW_BIT,		XmVENDOR_SHELL_BIT,
	XmCLIP_WINDOW_BIT,              XmDRAG_ICON_BIT,
        XmTEAROFF_BUTTON_BIT,           XmDRAG_OVER_SHELL_BIT,
        XmDRAG_CONTEXT_BIT,

	XmFAST_SUBCLASS_TAIL_BIT /* New entries precede this. */
	} ;

#define XmLAST_FAST_SUBCLASS_BIT (XmFAST_SUBCLASS_TAIL_BIT - 1) 


#undef XmIsCascadeButton
#define XmIsCascadeButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmCASCADE_BUTTON_BIT))

#undef XmIsCascadeButtonGadget
#define XmIsCascadeButtonGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmCASCADE_BUTTON_GADGET_BIT))

#undef XmIsCommandBox
#define XmIsCommandBox(w)  \
  (_XmIsFastSubclass(XtClass(w), XmCOMMAND_BOX_BIT))

#undef XmIsDialogShell
#define XmIsDialogShell(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDIALOG_SHELL_BIT))

#undef XmIsDisplay
#define XmIsDisplay(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDISPLAY_BIT))

#undef XmIsList
#define XmIsList(w)  \
  (_XmIsFastSubclass(XtClass(w), XmLIST_BIT))

#undef XmIsForm
#define XmIsForm(w)  \
  (_XmIsFastSubclass(XtClass(w), XmFORM_BIT))

#undef XmIsTextField
#define XmIsTextField(w)  \
  (_XmIsFastSubclass(XtClass(w), XmTEXT_FIELD_BIT))

#undef XmIsGadget
#define XmIsGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmGADGET_BIT))

#undef XmIsLabel
#define XmIsLabel(w)  \
  (_XmIsFastSubclass(XtClass(w), XmLABEL_BIT))

#undef XmIsLabelGadget
#define XmIsLabelGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmLABEL_GADGET_BIT))

#undef XmIsMainWindow
#define XmIsMainWindow(w)  \
  (_XmIsFastSubclass(XtClass(w), XmMAIN_WINDOW_BIT))

#undef XmIsManager
#define XmIsManager(w)  \
  (_XmIsFastSubclass(XtClass(w), XmMANAGER_BIT))

#undef XmIsMenuShell
#define XmIsMenuShell(w)  \
  (_XmIsFastSubclass(XtClass(w), XmMENU_SHELL_BIT))

#undef XmIsDragIcon
#define XmIsDragIcon(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDRAG_ICON_BIT))

#undef XmIsDropSiteManager
#define XmIsDropSiteManager(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDROP_SITE_MANAGER_BIT))

#undef XmIsDropTransfer
#define XmIsDropTransfer(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDROP_TRANSFER_BIT))

#undef XmIsDragOverShell
#define XmIsDragOverShell(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDRAG_OVER_SHELL_BIT))

#undef XmIsDragContext
#define XmIsDragContext(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDRAG_CONTEXT_BIT))

#undef XmIsDrawnButton
#define XmIsDrawnButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDRAWN_BUTTON_BIT))

#undef XmIsPrimitive
#define XmIsPrimitive(w)  \
  (_XmIsFastSubclass(XtClass(w), XmPRIMITIVE_BIT))

#undef XmIsPushButton
#define XmIsPushButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmPUSH_BUTTON_BIT))

#undef XmIsPushButtonGadget
#define XmIsPushButtonGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmPUSH_BUTTON_GADGET_BIT))

#undef XmIsRowColumn
#define XmIsRowColumn(w)  \
  (_XmIsFastSubclass(XtClass(w), XmROW_COLUMN_BIT))

#undef XmIsScreen
#define XmIsScreen(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSCREEN_BIT))

#undef XmIsScrollBar
#define XmIsScrollBar(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSCROLL_BAR_BIT))

#undef XmIsScrolledWindow
#define XmIsScrolledWindow(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSCROLLED_WINDOW_BIT))

#undef XmIsSelectionBox
#define XmIsSelectionBox(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSELECTION_BOX_BIT))

#undef XmIsSeparator
#define XmIsSeparator(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSEPARATOR_BIT))

#undef XmIsSeparatorGadget
#define XmIsSeparatorGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSEPARATOR_GADGET_BIT))

#undef XmIsText
#define XmIsText(w)  \
  (_XmIsFastSubclass(XtClass(w), XmTEXT_BIT))

#undef XmIsTearOffButton
#define XmIsTearOffButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmTEAROFF_BUTTON_BIT))

#undef XmIsToggleButton
#define XmIsToggleButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmTOGGLE_BUTTON_BIT))

#undef XmIsToggleButtonGadget
#define XmIsToggleButtonGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmTOGGLE_BUTTON_GADGET_BIT))

#undef XmIsArrowButton
#define XmIsArrowButton(w)  \
  (_XmIsFastSubclass(XtClass(w), XmARROW_BUTTON_BIT))

#undef XmIsArrowButtonGadget
#define XmIsArrowButtonGadget(w)  \
  (_XmIsFastSubclass(XtClass(w), XmARROW_BUTTON_GADGET_BIT))

#undef XmIsBulletinBoard
#define XmIsBulletinBoard(w)  \
  (_XmIsFastSubclass(XtClass(w), XmBULLETIN_BOARD_BIT))

#undef XmIsDrawingArea
#define XmIsDrawingArea(w)  \
  (_XmIsFastSubclass(XtClass(w), XmDRAWING_AREA_BIT))

#undef XmIsFileSelectionBox
#define XmIsFileSelectionBox(w)  \
  (_XmIsFastSubclass(XtClass(w), XmFILE_SELECTION_BOX_BIT))

#undef XmIsFrame
#define XmIsFrame(w)  \
  (_XmIsFastSubclass(XtClass(w), XmFRAME_BIT))

#undef XmIsMessageBox
#define XmIsMessageBox(w)  \
  (_XmIsFastSubclass(XtClass(w), XmMESSAGE_BOX_BIT))

#undef XmIsSash
#define XmIsSash(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSASH_BIT))

#undef XmIsScale
#define XmIsScale(w)  \
  (_XmIsFastSubclass(XtClass(w), XmSCALE_BIT))

#undef XmIsPanedWindow
#define XmIsPanedWindow(w)  \
  (_XmIsFastSubclass(XtClass(w), XmPANED_WINDOW_BIT))


/************************************************************************
 *
 *  Defines and macros for the XmResolvePart function
 *
 ************************************************************************/

/*  Widget class indices used with XmPartOffset and XmField macros  */

#define XmObjectIndex 		0
#define ObjectIndex 		XmObjectIndex
#define XmRectObjIndex		(XmObjectIndex + 1)
#define RectObjIndex		XmRectObjIndex
#define XmWindowObjIndex	(XmRectObjIndex + 1)
#define WindowObjIndex		XmWindowObjIndex
#define XmCoreIndex 		0
#define CoreIndex 		XmCoreIndex
#define XmCompositeIndex 	(XmWindowObjIndex + 2)
#define CompositeIndex 		XmCompositeIndex
#define XmConstraintIndex 	(XmCompositeIndex + 1)
#define ConstraintIndex 	XmConstraintIndex
#define XmGadgetIndex	 	(XmRectObjIndex + 1)
#define XmPrimitiveIndex 	(XmWindowObjIndex + 2)
#define XmManagerIndex	 	(XmConstraintIndex + 1)

#define XmArrowBIndex		(XmPrimitiveIndex + 1)
#define XmArrowButtonIndex	XmArrowBIndex
#define XmLabelIndex		(XmPrimitiveIndex + 1)
#define XmListIndex		(XmPrimitiveIndex + 1)
#define XmScrollBarIndex	(XmPrimitiveIndex + 1)
#define XmSeparatorIndex	(XmPrimitiveIndex + 1)
#define XmTextIndex		(XmPrimitiveIndex + 1)

#define XmCascadeBIndex		(XmLabelIndex + 1)
#define XmCascadeButtonIndex	XmCascadeBIndex
#define XmDrawnBIndex		(XmLabelIndex + 1)
#define XmDrawnButtonIndex	XmDrawnBIndex
#define XmPushBIndex		(XmLabelIndex + 1)
#define XmPushButtonIndex	XmPushBIndex
#define XmToggleBIndex		(XmLabelIndex + 1)
#define XmToggleButtonIndex	XmToggleBIndex
#define XmTearOffButtonIndex    (XmPushBIndex + 1)

#define XmArrowBGIndex		(XmGadgetIndex + 1)
#define XmArrowButtonGadgetIndex XmArrowBGIndex
#define XmLabelGIndex		(XmGadgetIndex + 1)
#define XmLabelGadgetIndex	XmLabelGIndex
#define XmSeparatoGIndex	(XmGadgetIndex + 1)
#define XmSeparatorGadgetIndex	XmSeparatoGIndex

#define XmCascadeBGIndex	(XmLabelGIndex + 1)
#define XmCascadeButtonGadgetIndex XmCascadeBGIndex
#define XmPushBGIndex		(XmLabelGIndex + 1)
#define XmPushButtonGadgetIndex	XmPushBGIndex
#define XmToggleBGIndex		(XmLabelGIndex + 1)
#define XmToggleButtonGadgetIndex XmToggleBGIndex

#define XmBulletinBIndex	(XmManagerIndex + 1)
#define XmBulletinBoardIndex	XmBulletinBIndex
#define XmDrawingAIndex		(XmManagerIndex + 1)
#define XmDrawingAreaIndex	XmDrawingAIndex
#define XmFrameIndex		(XmManagerIndex + 1)
#define XmPanedWIndex		(XmManagerIndex + 1)
#define XmPanedWindowIndex	XmPanedWIndex
#define XmSashIndex             (XmPrimitiveIndex + 1)
#define XmRowColumnIndex	(XmManagerIndex + 1)
#define XmScaleIndex		(XmManagerIndex + 1)
#define XmScrolledWIndex	(XmManagerIndex + 1)
#define XmScrolledWindowIndex	XmScrolledWIndex

#define XmFormIndex		(XmBulletinBIndex + 1)
#define XmMessageBIndex		(XmBulletinBIndex + 1)
#define XmMessageBoxIndex	XmMessageBIndex
#define XmSelectioBIndex	(XmBulletinBIndex + 1)
#define XmSelectionBoxIndex	XmSelectioBIndex

#define XmMainWIndex		(XmScrolledWIndex + 1)
#define XmMainWindowIndex	XmMainWIndex

#define XmCommandIndex		(XmSelectioBIndex + 1)
#define XmFileSBIndex		(XmSelectioBIndex + 1)
#define XmFileSelectionBoxIndex	XmFileSBIndex

#define XmShellIndex 		(XmCompositeIndex + 1)
#define ShellIndex 		XmShellIndex
#define XmOverrideShellIndex 	(XmShellIndex + 1)
#define OverrideShellIndex 	XmOverrideShellIndex
#define XmWMShellIndex	 	(XmShellIndex + 1)
#define WMShellIndex	 	XmWMShellIndex
#define XmVendorShellIndex 	(XmWMShellIndex + 1)
#define VendorShellIndex 	XmVendorShellIndex
#define XmTransientShellIndex	(XmVendorShellIndex + 1)
#define TransientShellIndex	XmTransientShellIndex
#define XmTopLevelShellIndex 	(XmVendorShellIndex + 1)
#define TopLevelShellIndex 	XmTopLevelShellIndex
#define XmApplicationShellIndex (XmTopLevelShellIndex + 1)
#define ApplicationShellIndex 	XmApplicationShellIndex
#define XmDisplayIndex		(XmApplicationShellIndex + 1)

#define XmDialogSIndex		(XmTransientShellIndex + 1)
#define XmDialogShellIndex	XmDialogSIndex
#define XmMenuShellIndex	(XmOverrideShellIndex + 1)

#define XmDragIconIndex		(XmRectObjIndex + 1)
#define XmDropSiteManagerIndex	(XmObjectIndex + 1)
#define XmDropTransferIndex	(XmObjectIndex + 1)
#define XmDragOverShellIndex	(XmVendorShellIndex + 1)
#define XmDragContextIndex	(XmCoreIndex + 1)

#define XmLONGBITS (sizeof(long) * 8)
#define XmHALFLONGBITS (XmLONGBITS/2)

typedef struct _XmPartResource {
    String     resource_name;	/* Resource name			    */
    String     resource_class;	/* Resource class			    */
    String     resource_type;	/* Representation type desired		    */
    Cardinal   resource_size;	/* Size in bytes of representation	    */
    Cardinal   resource_offset;	/* Index within & offset within part 	    */
    String     default_type;	/* representation type of specified default */
    XtPointer  default_addr;   	/* Address of default resource		    */
} XmPartResource;

#if (defined(__STDC__) && !defined(UNIXCPP)) \
        || defined(__cplusplus)			/* NovellPERF */
# define XmPartOffset(part, variable) \
        ((part##Index) << XmHALFLONGBITS) + XtOffsetOf( part##Part, variable)

# define XmConstraintPartOffset(part, variable) \
        ((part##Index) << XmHALFLONGBITS) + \
	XtOffsetOf( part##ConstraintPart, variable)

# define XmGetPartOffset(r, offset) \
       ((r)->resource_offset & 0xffff) + \
	(*(offset))[(r)->resource_offset >> XmHALFLONGBITS];

# define XmField(widget, offsetrecord, part, variable, type) \
	(*(type *)(((char *) (widget)) + offsetrecord[part##Index] + \
		XtOffsetOf( part##Part, variable)))

# define XmConstraintField(widget, offsetrecord, part, variable, type) \
	(*(type *)(((char *) (widget)->core.constraints) + \
	offsetrecord[part##Index] + \
	XtOffsetOf( part##ConstraintPart, variable)))
#else
# define XmPartOffset(part, variable) \
        ((part/**/Index) << XmHALFLONGBITS) + XtOffsetOf( part/**/Part, variable)

# define XmConstraintPartOffset(part, variable) \
        ((part/**/Index) << XmHALFLONGBITS) + \
	XtOffsetOf( part/**/ConstraintPart, variable)

# define XmGetPartOffset(r, offset) \
       ((r)->resource_offset & 0xffff) + \
	(*(offset))[(r)->resource_offset >> XmHALFLONGBITS];

# define XmField(widget, offsetrecord, part, variable, type) \
	(*(type *)(((char *) (widget)) + offsetrecord[part/**/Index] + \
	XtOffsetOf( part/**/Part, variable)))

# define XmConstraintField(widget, offsetrecord, part, variable, type) \
	(*(type *)(((char *) (widget)->core.constraints) + \
	offsetrecord[part/**/Index] + \
	XtOffsetOf( part/**/ConstraintPart, variable)))
#endif

/***********************************************************************
 *
 * XmRegion structure
 *
 *  This structure must match the opaque libX Region structure.
 ***********************************************************************/

typedef struct {
    short x1, x2, y1, y2;
} XmRegionBox;

typedef struct _XmRegion {
    long	size;
    long	numRects;
    XmRegionBox	*rects;
    XmRegionBox	extents;
} XmRegionRec, *XmRegion;


/********    Private Function Declarations for GadgetUtil.c    ********/
#ifdef _NO_PROTO

extern XmGadget _XmInputInGadget() ;
extern XmGadget _XmInputForGadget() ;
extern void _XmConfigureObject() ;
extern void _XmResizeObject() ;
extern void _XmMoveObject() ;
extern void _XmRedisplayGadgets() ;
extern void _XmDispatchGadgetInput() ;
extern Time __XmGetDefaultTime() ;

#else

extern XmGadget _XmInputInGadget( 
                        Widget cw,
                        register int x,
                        register int y) ;
extern XmGadget _XmInputForGadget( 
                        Widget cw,
                        int x,
                        int y) ;
extern void _XmConfigureObject( 
                        Widget g,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int border_width) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension border_width) ;
#endif /* NeedWidePrototypes */
extern void _XmResizeObject( 
                        Widget g,
#if NeedWidePrototypes
                        int width,
                        int height,
                        int border_width) ;
#else
                        Dimension width,
                        Dimension height,
                        Dimension border_width) ;
#endif /* NeedWidePrototypes */
extern void _XmMoveObject( 
                        Widget g,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
extern void _XmRedisplayGadgets( 
                        Widget w,
                        register XEvent *event,
                        Region region) ;
extern void _XmDispatchGadgetInput( 
                        Widget g,
                        XEvent *event,
                        Mask mask) ;
extern Time __XmGetDefaultTime(Widget, XEvent*) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/******** _XmCreateImage ********/

/* The _XmCreateImage macro is used to create XImage with client
   specific data for the bit and byte order.
   We still have to do the following because XCreateImage
   will stuff here display specific data and we want 
   client specific values (i.e the bit orders we used for 
   creating the bitmap data in Motif) -- BUG 4262 */
/* Used in Motif 1.2 in DragIcon.c, MessageB.c, ReadImage.c and
   ImageCache.c */

#define _XmCreateImage(IMAGE, DISPLAY, DATA, WIDTH, HEIGHT, BYTE_ORDER) {\
    IMAGE = XCreateImage(DISPLAY,\
			 DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)),\
			 1,\
			 XYBitmap,\
			 0,\
			 DATA,\
			 WIDTH, HEIGHT,\
			 8,\
			 (WIDTH+7) >> 3);\
    IMAGE->byte_order = BYTE_ORDER;\
    IMAGE->bitmap_unit = 8;\
    IMAGE->bitmap_bit_order = LSBFirst;\
}

/********    Private Function Declarations for ImageCache.c    ********/
#ifdef _NO_PROTO

extern Boolean _XmInstallImage() ;
extern Boolean _XmGetImage() ;
extern Boolean _XmGetPixmapData() ;
extern Pixmap _XmGetPixmap() ;
extern Boolean _XmInstallPixmap() ;

#else

extern Boolean _XmInstallImage( 
                        XImage *image,
                        char *image_name,
                        int hot_x,
                        int hot_y) ;
extern Boolean _XmGetImage( 
                        Screen *screen,
                        char *image_name,
                        XImage **image) ;
extern Boolean _XmGetPixmapData( 
                        Screen *screen,
                        Pixmap pixmap,
                        char **image_name,
                        int *depth,
                        Pixel *foreground,
                        Pixel *background,
                        int *hot_x,
                        int *hot_y,
                        unsigned int *width,
                        unsigned int *height) ;
extern Pixmap _XmGetPixmap( 
                        Screen *screen,
                        char *image_name,
                        int depth,
                        Pixel foreground,
                        Pixel background) ;
extern Boolean _XmInstallPixmap( 
                        Pixmap pixmap,
                        Screen *screen,
                        char *image_name,
                        Pixel foreground,
                        Pixel background) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for MapEvents.c    ********/
#ifdef _NO_PROTO

extern Boolean _XmMapBtnEvent() ;
extern Boolean _XmMapKeyEvent() ;
extern Boolean _XmMatchBtnEvent() ;
extern Boolean _XmMatchKeyEvent() ;

#else

extern Boolean _XmMapBtnEvent( 
                        register String str,
                        int *eventType,
                        unsigned int *button,
                        unsigned int *modifiers) ;
extern Boolean _XmMapKeyEvent( 
                        register String str,
                        int *eventType,
                        unsigned *keysym,
                        unsigned int *modifiers) ;
extern Boolean _XmMatchBtnEvent( 
                        XEvent *event,
                        int eventType,
                        unsigned int button,
                        unsigned int modifiers) ;
extern Boolean _XmMatchKeyEvent( 
                        XEvent *event,
                        int eventType,
                        unsigned int key,
                        unsigned int modifiers) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for ReadImage.c    ********/
#ifdef _NO_PROTO

extern XImage * _XmGetImageFromFile() ;
extern XImage * _XmGetImageAndHotSpotFromFile() ;

#else

extern XImage * _XmGetImageFromFile( 
                        char *filename) ;
extern XImage * _XmGetImageAndHotSpotFromFile( 
                        char *filename,
                        int *hot_x,
                        int *hot_y) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    ResConvert.c    ********/

enum{	XmLABEL_FONTLIST = 1,		XmBUTTON_FONTLIST,
	XmTEXT_FONTLIST
	} ;

/********    Private Function Declarations for ResConvert.c    ********/
#ifdef _NO_PROTO

extern void _XmRegisterConverters() ;
extern void _XmWarning() ;
extern Boolean _XmStringsAreEqual() ;
extern XmFontList _XmGetDefaultFontList() ;
extern char * _XmConvertCSToString() ;
extern Boolean _XmCvtXmStringToCT() ;

#else

extern void _XmRegisterConverters( void ) ;
extern void _XmWarning( 
                        Widget w,
                        char *message) ;
extern Boolean _XmStringsAreEqual( 
                        register char *in_str,
                        register char *test_str) ;
extern XmFontList _XmGetDefaultFontList( 
                        Widget w,
#if NeedWidePrototypes
                        unsigned int fontListType) ;
#else
                        unsigned char fontListType) ;
#endif /* NeedWidePrototypes */
extern char * _XmConvertCSToString( 
                        XmString cs) ;
extern Boolean _XmCvtXmStringToCT( 
                        XrmValue *from,
                        XrmValue *to) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for ResInd.c    ********/
#ifdef _NO_PROTO

extern void _XmBuildResources() ;
extern void _XmInitializeSyntheticResources() ;
extern void _XmPrimitiveGetValuesHook() ;
extern void _XmGadgetGetValuesHook() ;
extern void _XmManagerGetValuesHook() ;
extern void _XmExtGetValuesHook() ;
extern void _XmExtImportArgs() ;
extern void _XmPrimitiveImportArgs() ;
extern void _XmGadgetImportArgs() ;
extern void _XmGadgetImportSecondaryArgs() ;
extern void _XmManagerImportArgs() ;

extern int _XmConvertUnits() ;
extern XmImportOperator _XmToHorizontalPixels() ;
extern XmImportOperator _XmToVerticalPixels() ;
extern void _XmFromHorizontalPixels() ;
extern void _XmFromVerticalPixels() ;
extern void _XmSortResourceList() ;
extern void _XmUnitTypeDefault() ;
extern unsigned char _XmGetUnitType() ;

#else

extern void _XmBuildResources( 
                        XmSyntheticResource **wc_resources_ptr,
                        int *wc_num_resources_ptr,
                        XmSyntheticResource *sc_resources,
                        int sc_num_resources) ;
extern void _XmInitializeSyntheticResources( 
                        XmSyntheticResource *resources,
                        int num_resources) ;
extern void _XmPrimitiveGetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmGadgetGetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmManagerGetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmExtGetValuesHook( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmExtImportArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmPrimitiveImportArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmGadgetImportArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmGadgetImportSecondaryArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmManagerImportArgs( 
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
extern int _XmConvertUnits( 
                        Screen *screen,
                        int dimension,
                        register int from_type,
                        register int from_val,
                        register int to_type) ;
extern XmImportOperator _XmToHorizontalPixels( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
extern XmImportOperator _XmToVerticalPixels( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
extern void _XmFromHorizontalPixels( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
extern void _XmFromVerticalPixels( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
extern void _XmSortResourceList( 
                        XrmResource *list[],
                        Cardinal len) ;
extern void _XmUnitTypeDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern unsigned char _XmGetUnitType( 
                        Widget widget) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for UniqueEvnt.c    ********/
#ifdef _NO_PROTO

extern Boolean _XmIsEventUnique() ;
extern void _XmRecordEvent() ;

#else

extern Boolean _XmIsEventUnique( 
                        XEvent *event) ;
extern void _XmRecordEvent( 
                        XEvent *event) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Visual.c    ********/
/* For the default color calculation and caching */

#define XmLOOK_AT_SCREEN          (1<<0)
#define XmLOOK_AT_CMAP            (1<<1)
#define XmLOOK_AT_BACKGROUND      (1<<2)
#define XmLOOK_AT_FOREGROUND      (1<<3)
#define XmLOOK_AT_TOP_SHADOW      (1<<4)
#define XmLOOK_AT_BOTTOM_SHADOW   (1<<5)
#define XmLOOK_AT_SELECT          (1<<6)

#define XmBACKGROUND     ((unsigned char) (1<<0))
#define XmFOREGROUND     ((unsigned char) (1<<1))
#define XmTOP_SHADOW     ((unsigned char) (1<<2))
#define XmBOTTOM_SHADOW  ((unsigned char) (1<<3))
#define XmSELECT         ((unsigned char) (1<<4))

/*  Structure used to hold color schemes  */
typedef struct _XmColorData
{  Screen * screen;
   Colormap color_map;
   unsigned char allocated;
   XColor background;
   XColor foreground;
   XColor top_shadow;
   XColor bottom_shadow;
   XColor select;
} XmColorData;

/********    Private Function Declarations for Visual.c    ********/
#ifdef _NO_PROTO

extern void _XmRegisterPixmapConverters() ;
extern char * _XmGetBGPixmapName() ;
extern void _XmClearBGPixmapName() ;
extern void _XmForegroundColorDefault() ;
extern void _XmHighlightColorDefault() ;
extern void _XmBackgroundColorDefault() ;
extern void _XmTopShadowColorDefault() ;
extern void _XmBottomShadowColorDefault() ;
extern void _XmSelectColorDefault() ;
extern void _XmPrimitiveTopShadowPixmapDefault() ;
extern void _XmManagerTopShadowPixmapDefault() ;
extern void _XmPrimitiveHighlightPixmapDefault() ;
extern void _XmManagerHighlightPixmapDefault() ;
extern void _XmGetDefaultThresholdsForScreen() ;
extern String _XmGetDefaultBackgroundColorSpec() ;
extern void _XmSetDefaultBackgroundColorSpec() ;
extern XmColorData * _XmGetDefaultColors() ;
extern Boolean _XmSearchColorCache() ;
extern XmColorData * _XmAddToColorCache() ;
extern Pixel _XmBlackPixel() ;
extern Pixel _XmWhitePixel() ;
extern Pixel _XmAccessColorData() ;
extern XmColorData * _XmGetColors() ;

#else

extern void _XmRegisterPixmapConverters( void ) ;
extern char * _XmGetBGPixmapName( void ) ;
extern void _XmClearBGPixmapName( void ) ;
extern void _XmForegroundColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmHighlightColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmBackgroundColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmTopShadowColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmBottomShadowColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmSelectColorDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmPrimitiveTopShadowPixmapDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmManagerTopShadowPixmapDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmPrimitiveHighlightPixmapDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmManagerHighlightPixmapDefault( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmGetDefaultThresholdsForScreen( 
                        Screen *screen) ;
extern String _XmGetDefaultBackgroundColorSpec( 
                        Screen *screen) ;
extern void _XmSetDefaultBackgroundColorSpec( 
                        Screen *screen,
                        String new_color_spec) ;
extern XmColorData * _XmGetDefaultColors( 
                        Screen *screen,
                        Colormap color_map) ;
extern Boolean _XmSearchColorCache( 
                        unsigned int which,
                        XmColorData *values,
                        XmColorData **ret) ;
extern XmColorData * _XmAddToColorCache( 
                        XmColorData *new_rec) ;
extern Pixel _XmBlackPixel( 
                        Screen *screen,
                        Colormap colormap,
                        XColor blackcolor) ;
extern Pixel _XmWhitePixel( 
                        Screen *screen,
                        Colormap colormap,
                        XColor whitecolor) ;
extern Pixel _XmAccessColorData( 
                        XmColorData *cd,
#if NeedWidePrototypes
                        unsigned int which) ;
#else
                        unsigned char which) ;
#endif /* NeedWidePrototypes */
extern XmColorData * _XmGetColors( 
                        Screen *screen,
                        Colormap color_map,
                        Pixel background) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for XmString.c    ********/
#ifdef _NO_PROTO

extern XFontStruct * _XmGetFirstFont() ;
extern Boolean _XmFontListGetDefaultFont() ;
extern Boolean _XmFontListSearch() ;
extern Boolean _XmStringIsXmString() ;
extern Boolean _XmStringInitContext() ;
extern Boolean _XmStringGetNextSegment() ;
extern void _XmStringFreeContext() ;
extern Dimension _XmStringWidth() ;
extern Dimension _XmStringHeight() ;
extern void _XmStringExtent() ;
extern Boolean _XmStringEmpty() ;
extern void _XmStringDraw() ;
extern void _XmStringDrawImage() ;
extern void _XmStringDrawUnderline() ;
extern void _XmStringDrawMnemonic() ;
extern _XmString _XmStringCreate() ;
extern void _XmStringFree() ;
extern char * _XmStringGetCurrentCharset() ;
extern char * _XmCharsetCanonicalize() ;
extern void _XmStringUpdate() ;
extern _XmString _XmStringCopy() ;
extern Boolean _XmStringByteCompare() ;
extern Boolean _XmStringHasSubstring() ;
extern XmString _XmStringCreateExternal() ;
extern Dimension _XmStringBaseline() ;
extern int _XmStringLineCount() ;
extern char * _XmStringGetTextConcat() ;
extern Boolean _XmStringIsCurrentCharset() ;
extern Boolean _XmStringSingleSegment() ;
extern void _XmStringUpdateWMShellTitle() ;

#else

extern XFontStruct * _XmGetFirstFont( 
                        XmFontListEntry entry) ;
extern Boolean _XmFontListGetDefaultFont( 
                        XmFontList fontlist,
                        XFontStruct **font_struct) ;
extern Boolean _XmFontListSearch( 
                        XmFontList fontlist,
                        XmStringCharSet charset,
                        short *indx,
                        XFontStruct **font_struct) ;
extern Boolean _XmStringIsXmString( 
                        XmString string) ;
extern Boolean _XmStringInitContext( 
                        _XmStringContext *context,
                        _XmString string) ;
extern Boolean _XmStringGetNextSegment( 
                        _XmStringContext context,
                        XmStringCharSet *charset,
                        XmStringDirection *direction,
                        char **text,
                        short *char_count,
                        Boolean *separator) ;
extern void _XmStringFreeContext( 
                        _XmStringContext context) ;
extern Dimension _XmStringWidth( 
                        XmFontList fontlist,
                        _XmString string) ;
extern Dimension _XmStringHeight( 
                        XmFontList fontlist,
                        _XmString string) ;
extern void _XmStringExtent( 
                        XmFontList fontlist,
                        _XmString string,
                        Dimension *width,
                        Dimension *height) ;
extern Boolean _XmStringEmpty( 
                        _XmString string) ;
extern void _XmStringDraw( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        _XmString string,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        unsigned int align,
                        unsigned int lay_dir,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        unsigned char align,
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip) ;
extern void _XmStringDrawImage( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        _XmString string,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        unsigned int align,
                        unsigned int lay_dir,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        unsigned char align,
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip) ;
extern void _XmStringDrawUnderline( 
                        Display *d,
                        Window w,
                        XmFontList f,
                        _XmString s,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        unsigned int align,
                        unsigned int lay_dir,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        unsigned char align,
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip,
                        _XmString u) ;
extern void _XmStringDrawMnemonic( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        _XmString string,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        unsigned int align,
                        unsigned int lay_dir,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        unsigned char align,
                        unsigned char lay_dir,
#endif /* NeedWidePrototypes */
                        XRectangle *clip,
                        String mnemonic,
                        XmStringCharSet charset) ;
extern _XmString _XmStringCreate( 
                        XmString cs) ;
extern void _XmStringFree( 
                        _XmString string) ;
extern char * _XmStringGetCurrentCharset( void ) ;
extern char * _XmCharsetCanonicalize( 
                        String charset) ;
extern void _XmStringUpdate( 
                        XmFontList fontlist,
                        _XmString string) ;
extern _XmString _XmStringCopy( 
                        _XmString string) ;
extern Boolean _XmStringByteCompare( 
                        _XmString a,
                        _XmString b) ;
extern Boolean _XmStringHasSubstring( 
                        _XmString string,
                        _XmString substring) ;
extern XmString _XmStringCreateExternal( 
                        XmFontList fontlist,
                        _XmString cs) ;
extern Dimension _XmStringBaseline( 
                        XmFontList fontlist,
                        _XmString string) ;
extern int _XmStringLineCount( 
                        _XmString string) ;
extern char * _XmStringGetTextConcat( 
                        XmString string) ;
extern Boolean _XmStringIsCurrentCharset(
			XmStringCharSet c) ;
extern Boolean _XmStringSingleSegment(
			XmString str,
			char **pTextOut,
			XmStringCharSet *pCharsetOut ) ;
extern void _XmStringUpdateWMShellTitle(
			XmString xmstr,
			Widget shell) ;
#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/
#ifdef _XmDEBUG_XMSTRING
#ifdef _NO_PROTO
extern void _Xm_dump_fontlist() ;
extern void _Xm_dump_fontlist_cache() ;
extern void _Xm_dump_external() ;
extern void _Xm_dump_internal() ;
#else /* _NO_PROTO */
extern void _Xm_dump_fontlist( 
                        XmFontList f) ;
extern void _Xm_dump_fontlist_cache( void ) ;
extern void _Xm_dump_external( 
                        XmString cs) ;
extern void _Xm_dump_internal( 
                        _XmString string) ;
#endif /* _NO_PROTO */
#endif /* _XmDEBUG_XMSTRING */

/********    Traversal.c    ********/

#define XmTAB_ANY	((XmNavigationType) 255)
#define XmNONE_OR_BC	((XmNavigationType) 254)

typedef struct _XmFocusMovedCallbackStruct{
    int 	reason;
    XEvent  	*event;
    Boolean 	cont;
    Widget	old_focus;
    Widget	new_focus;
    unsigned char focus_policy;
} XmFocusMovedCallbackStruct, *XmFocusMovedCallback;

typedef struct _XmFocusDataRec *XmFocusData;

/********    Private Function Declarations for Traversal.c    ********/
#ifdef _NO_PROTO

extern XmFocusData _XmCreateFocusData() ;
extern void _XmDestroyFocusData() ;
extern void _XmSetActiveTabGroup() ;
extern Widget _XmGetActiveItem() ;
extern void _XmNavigInitialize() ;
extern Boolean _XmNavigSetValues() ;
extern void _XmNavigChangeManaged() ;
extern void _XmNavigResize() ;
extern void _XmValidateFocus() ;
extern void _XmNavigDestroy() ;
extern Boolean _XmCallFocusMoved() ;
extern Boolean _XmMgrTraversal() ;
extern void _XmClearFocusPath() ;
extern Boolean _XmFocusIsHere() ;
extern void _XmProcessTraversal() ;
extern unsigned char _XmGetFocusPolicy() ;
extern Widget _XmFindTopMostShell() ;
extern void _XmFocusModelChanged() ;
extern Boolean _XmGrabTheFocus() ;
extern XmFocusData _XmGetFocusData() ;
extern Boolean _XmCreateVisibilityRect() ;
extern void _XmSetRect() ;
extern int _XmIntersectRect() ;
extern int _XmEmptyRect() ;
extern void _XmClearRect() ;
extern Boolean _XmIsNavigable() ;
extern void _XmWidgetFocusChange() ;
extern Widget _XmNavigate() ;
extern Widget _XmFindNextTabGroup() ;
extern Widget _XmFindPrevTabGroup() ;
extern void _XmSetInitialOfTabGroup() ;
extern void _XmResetTravGraph() ;
extern Boolean _XmFocusIsInShell() ;
extern Boolean _XmShellIsExclusive() ;
extern Widget _XmGetFirstFocus() ;

#else

extern XmFocusData _XmCreateFocusData( void ) ;
extern void _XmDestroyFocusData( 
                        XmFocusData focusData) ;
extern void _XmSetActiveTabGroup( 
                        XmFocusData focusData,
                        Widget tabGroup) ;
extern Widget _XmGetActiveItem( 
                        Widget w) ;
extern void _XmNavigInitialize( 
                        Widget request,
                        Widget new_wid,
                        ArgList args,
                        Cardinal *num_args) ;
extern Boolean _XmNavigSetValues( 
                        Widget current,
                        Widget request,
                        Widget new_wid,
                        ArgList args,
                        Cardinal *num_args) ;
extern void _XmNavigChangeManaged( 
                        Widget wid) ;
extern void _XmNavigResize( 
                        Widget wid) ;
extern void _XmValidateFocus( 
                        Widget wid) ;
extern void _XmNavigDestroy( 
                        Widget wid) ;
extern Boolean _XmCallFocusMoved( 
                        Widget old,
                        Widget new_wid,
                        XEvent *event) ;
extern Boolean _XmMgrTraversal( 
                        Widget wid,
                        XmTraversalDirection direction) ;
extern void _XmClearFocusPath( 
                        Widget wid) ;
extern Boolean _XmFocusIsHere( 
                        Widget w) ;
extern void _XmProcessTraversal( 
                        Widget w,
                        XmTraversalDirection dir,
#if NeedWidePrototypes
                        int check) ;
#else
                        Boolean check) ;
#endif /* NeedWidePrototypes */
extern unsigned char _XmGetFocusPolicy( 
                        Widget w) ;
extern Widget _XmFindTopMostShell( 
                        Widget w) ;
extern void _XmFocusModelChanged( 
                        Widget wid,
                        XtPointer client_data,
                        XtPointer call_data) ;
extern Boolean _XmGrabTheFocus( 
                        Widget w,
                        XEvent *event) ;
extern XmFocusData _XmGetFocusData( 
                        Widget wid) ;
extern Boolean _XmCreateVisibilityRect( 
                        Widget w,
                        XRectangle *rectPtr) ;
extern void _XmSetRect( 
                        register XRectangle *rect,
                        Widget w) ;
extern int _XmIntersectRect( 
                        register XRectangle *srcRectA,
                        register Widget widget,
                        register XRectangle *dstRect) ;
extern int _XmEmptyRect( 
                        register XRectangle *r) ;
extern void _XmClearRect( 
                        register XRectangle *r) ;
extern Boolean _XmIsNavigable( 
                        Widget wid) ;
extern void _XmWidgetFocusChange( 
                        Widget wid,
                        XmFocusChange change) ;
extern Widget _XmNavigate( 
                        Widget wid,
                        XmTraversalDirection direction) ;
extern Widget _XmFindNextTabGroup( 
                        Widget wid) ;
extern Widget _XmFindPrevTabGroup( 
                        Widget wid) ;
extern void _XmSetInitialOfTabGroup( 
                        Widget tab_group,
                        Widget init_focus) ;
extern void _XmResetTravGraph( 
                        Widget wid) ;
extern Boolean _XmFocusIsInShell( 
                        Widget wid) ;
extern Boolean _XmShellIsExclusive( 
                        Widget wid) ;
extern Widget _XmGetFirstFocus( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for TravAct.c    ********/
#ifdef _NO_PROTO

extern void _XmTrackShellFocus() ;
extern void _XmPrimitiveEnter() ;
extern void _XmPrimitiveLeave() ;
extern void _XmPrimitiveUnmap() ;
extern void _XmPrimitiveFocusInInternal() ;
extern void _XmPrimitiveFocusOut() ;
extern void _XmPrimitiveFocusIn() ;
extern void _XmEnterGadget() ;
extern void _XmLeaveGadget() ;
extern void _XmFocusInGadget() ;
extern void _XmFocusOutGadget() ;
extern void _XmManagerEnter() ;
extern void _XmManagerLeave() ;
extern void _XmManagerFocusInInternal() ;
extern void _XmManagerFocusIn() ;
extern void _XmManagerFocusOut() ;
extern void _XmManagerUnmap() ;

#else

extern void _XmTrackShellFocus( 
                        Widget widget,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *dontSwallow) ;
extern void _XmPrimitiveEnter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveLeave( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveUnmap( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusInInternal( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveFocusIn( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmEnterGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmLeaveGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmFocusInGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmFocusOutGadget( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerEnter( 
                        Widget wid,
                        XEvent *event_in,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerLeave( 
                        Widget wid,
                        XEvent *event_in,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusInInternal( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusIn( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmManagerUnmap( 
                        Widget mw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for GeoUtils.c    ********/
#ifdef _NO_PROTO

extern XtGeometryResult _XmHandleQueryGeometry() ;
extern XtGeometryResult _XmHandleGeometryManager() ;
extern void _XmHandleSizeUpdate() ;
extern XmGeoMatrix _XmGeoMatrixAlloc() ;
extern void _XmGeoMatrixFree() ;
extern Boolean _XmGeoSetupKid() ;
extern void _XmGeoMatrixGet() ;
extern void _XmGeoMatrixSet() ;
extern void _XmGeoAdjustBoxes() ;
extern void _XmGeoGetDimensions() ;
extern void _XmGeoArrangeBoxes() ;
extern Dimension _XmGeoBoxesSameWidth() ;
extern Dimension _XmGeoBoxesSameHeight() ;
extern void _XmSeparatorFix() ;
extern void _XmMenuBarFix() ;
extern void _XmGeoLoadValues() ;
extern int _XmGeoCount_kids() ;
extern XmKidGeometry _XmGetKidGeo() ;
extern void _XmGeoClearRectObjAreas() ;
extern void _XmSetKidGeo() ;
extern Boolean _XmGeometryEqual() ;
extern Boolean _XmGeoReplyYes() ;
extern XtGeometryResult _XmMakeGeometryRequest() ;

#else

extern XtGeometryResult _XmHandleQueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *desired,
#if NeedWidePrototypes
                        unsigned int policy,
#else
                        unsigned char policy,
#endif /* NeedWidePrototypes */
                        XmGeoCreateProc createMatrix) ;
extern XtGeometryResult _XmHandleGeometryManager( 
                        Widget wid,
                        Widget instigator,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed,
#if NeedWidePrototypes
                        unsigned int policy,
#else
                        unsigned char policy,
#endif /* NeedWidePrototypes */
                        XmGeoMatrix *cachePtr,
                        XmGeoCreateProc createMatrix) ;
extern void _XmHandleSizeUpdate( 
                        Widget wid,
#if NeedWidePrototypes
                        unsigned int policy,
#else
                        unsigned char policy,
#endif /* NeedWidePrototypes */
                        XmGeoCreateProc createMatrix) ;
extern XmGeoMatrix _XmGeoMatrixAlloc( 
                        unsigned int numRows,
                        unsigned int numBoxes,
                        unsigned int extSize) ;
extern void _XmGeoMatrixFree( 
                        XmGeoMatrix geo_spec) ;
extern Boolean _XmGeoSetupKid( 
                        XmKidGeometry geo,
                        Widget kidWid) ;
extern void _XmGeoMatrixGet( 
                        XmGeoMatrix geoSpec,
                        int geoType) ;
extern void _XmGeoMatrixSet( 
                        XmGeoMatrix geoSpec) ;
extern void _XmGeoAdjustBoxes( 
                        XmGeoMatrix geoSpec) ;
extern void _XmGeoGetDimensions( 
                        XmGeoMatrix geoSpec) ;
extern void _XmGeoArrangeBoxes( 
                        XmGeoMatrix geoSpec,
#if NeedWidePrototypes
                        int x,
                        int y,
#else
                        Position x,
                        Position y,
#endif /* NeedWidePrototypes */
                        Dimension *pW,
                        Dimension *pH) ;
extern Dimension _XmGeoBoxesSameWidth( 
                        XmKidGeometry rowPtr,
#if NeedWidePrototypes
                        int width) ;
#else
                        Dimension width) ;
#endif /* NeedWidePrototypes */
extern Dimension _XmGeoBoxesSameHeight( 
                        XmKidGeometry rowPtr,
#if NeedWidePrototypes
                        int height) ;
#else
                        Dimension height) ;
#endif /* NeedWidePrototypes */
extern void _XmSeparatorFix( 
                        XmGeoMatrix geoSpec,
                        int action,
                        XmGeoMajorLayout layoutPtr,
                        XmKidGeometry rowPtr) ;
extern void _XmMenuBarFix( 
                        XmGeoMatrix geoSpec,
                        int action,
                        XmGeoMajorLayout layoutPtr,
                        XmKidGeometry rowPtr) ;
extern void _XmGeoLoadValues( 
                        Widget wid,
                        int geoType,
                        Widget instigator,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *geoResult) ;
extern int _XmGeoCount_kids( 
                        register CompositeWidget c) ;
extern XmKidGeometry _XmGetKidGeo( 
                        Widget wid,
                        Widget instigator,
                        XtWidgetGeometry *request,
                        int uniform_border,
#if NeedWidePrototypes
                        int border,
#else
                        Dimension border,
#endif /* NeedWidePrototypes */
                        int uniform_width_margins,
                        int uniform_height_margins,
                        Widget help,
                        int geo_type) ;
extern void _XmGeoClearRectObjAreas( 
                        RectObj r,
                        XWindowChanges *old) ;
extern void _XmSetKidGeo( 
                        XmKidGeometry kg,
                        Widget instigator) ;
extern Boolean _XmGeometryEqual( 
                        Widget wid,
                        XtWidgetGeometry *geoA,
                        XtWidgetGeometry *geoB) ;
extern Boolean _XmGeoReplyYes( 
                        Widget wid,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *response) ;
extern XtGeometryResult _XmMakeGeometryRequest( 
                        Widget w,
                        XtWidgetGeometry *geom) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for Dest.c    ********/
#ifdef _NO_PROTO

extern void _XmSetDestination() ;

#else

extern void _XmSetDestination( 
                        Display *dpy,
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for XmIm.c    ********/
#ifdef _NO_PROTO

extern void _XmImChangeManaged() ;
extern void _XmImRealize() ;
extern void _XmImResize() ;
extern void _XmImRedisplay() ;

#else

extern void _XmImChangeManaged( 
                        Widget vw) ;
extern void _XmImRealize( 
                        Widget vw) ;
extern void _XmImResize( 
                        Widget vw) ;
extern void _XmImRedisplay( 
                        Widget vw) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for DragBS.c   ********/
#ifdef _NO_PROTO

extern void _XmInitAtomPairs() ;
extern void _XmInitTargetsTable() ;
extern Cardinal _XmIndexToTargets() ;
extern Cardinal _XmTargetsToIndex() ;
extern Atom _XmAllocMotifAtom() ;
extern void _XmFreeMotifAtom() ;
extern void _XmDestroyMotifWindow() ;
extern Window _XmGetDragProxyWindow() ;

#else

extern void _XmInitAtomPairs( 
                        Display *display) ;
extern void _XmInitTargetsTable( 
                        Display *display) ;
extern Cardinal _XmIndexToTargets( 
                        Widget shell,
                        Cardinal t_index,
                        Atom **targetsRtn) ;
extern Cardinal _XmTargetsToIndex( 
                        Widget shell,
                        Atom *targets,
                        Cardinal numTargets) ;
extern Atom _XmAllocMotifAtom( 
                        Widget shell,
                        Time time) ;
extern void _XmFreeMotifAtom( 
                        Widget shell,
                        Atom atom) ;
extern void _XmDestroyMotifWindow( 
                        Display *dpy) ;
extern Window _XmGetDragProxyWindow(
			Display *display) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for DragOverS.c    ********/
#ifdef _NO_PROTO

extern void _XmDragOverHide() ;
extern void _XmDragOverShow() ;
extern void _XmDragOverMove() ;
extern void _XmDragOverChange() ;
extern void _XmDragOverFinish() ;
extern Cursor _XmDragOverGetActiveCursor() ;
extern void _XmDragOverSetInitialPosition() ;

#else

extern void _XmDragOverHide( 
                        Widget w,
#if NeedWidePrototypes
                        int clipOriginX,
                        int clipOriginY,
#else
                        Position clipOriginX,
                        Position clipOriginY,
#endif /* NeedWidePrototypes */
                        XmRegion clipRegion) ;
extern void _XmDragOverShow( 
                        Widget w,
#if NeedWidePrototypes
                        int clipOriginX,
                        int clipOriginY,
#else
                        Position clipOriginX,
                        Position clipOriginY,
#endif /* NeedWidePrototypes */
                        XmRegion clipRegion) ;
extern void _XmDragOverMove( 
                        Widget w,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
extern void _XmDragOverChange( 
                        Widget w,
#if NeedWidePrototypes
                        unsigned int dropSiteStatus) ;
#else
                        unsigned char dropSiteStatus) ;
#endif /* NeedWidePrototypes */
extern void _XmDragOverFinish( 
                        Widget w,
#if NeedWidePrototypes
                        unsigned int completionStatus) ;
#else
                        unsigned char completionStatus) ;
#endif /* NeedWidePrototypes */

extern Cursor _XmDragOverGetActiveCursor(
			Widget w) ;
extern void _XmDragOverSetInitialPosition(
			Widget w,
#if NeedWidePrototypes
			int initialX,
			int initialY) ;
#else
			Position initialX,
			Position initialY) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for Region.c    ********/
#ifdef _NO_PROTO

extern XmRegion _XmRegionCreate() ;
extern XmRegion _XmRegionCreateSize() ;
extern void _XmRegionComputeExtents() ;
extern void _XmRegionGetExtents() ;
extern void _XmRegionUnionRectWithRegion() ;
extern void _XmRegionIntersectRectWithRegion() ;
extern long _XmRegionGetNumRectangles() ;
extern void _XmRegionGetRectangles() ;
extern void _XmRegionSetGCRegion() ;
extern void _XmRegionDestroy() ;
extern void _XmRegionOffset() ;
extern void _XmRegionIntersect() ;
extern void _XmRegionUnion() ;
extern void _XmRegionSubtract() ;
extern Boolean _XmRegionIsEmpty() ;
extern Boolean _XmRegionEqual() ;
extern Boolean _XmRegionPointInRegion() ;
extern void _XmRegionClear() ;
extern void _XmRegionShrink() ;
extern void _XmRegionDrawShadow() ;

#else

extern XmRegion _XmRegionCreate( void ) ;
extern XmRegion _XmRegionCreateSize(
			long size) ;
extern void _XmRegionComputeExtents(
			XmRegion r) ;
extern void _XmRegionGetExtents( 
                        XmRegion r,
                        XRectangle *rect) ;
extern void _XmRegionUnionRectWithRegion( 
                        XRectangle *rect,
                        XmRegion source,
                        XmRegion dest) ;
extern void _XmRegionIntersectRectWithRegion( 
                        XRectangle *rect,
                        XmRegion source,
                        XmRegion dest) ;
extern long _XmRegionGetNumRectangles(
			XmRegion r) ;
extern void _XmRegionGetRectangles( 
                        XmRegion r,
                        XRectangle **rects,
                        long *nrects) ;
extern void _XmRegionSetGCRegion( 
                        Display *dpy,
                        GC gc,
			int x_origin,
			int y_origin,
                        XmRegion r) ;
extern void _XmRegionDestroy( 
                        XmRegion r) ;
extern void _XmRegionOffset( 
                        XmRegion pRegion,
                        int x,
                        int y) ;
extern void _XmRegionIntersect( 
                        XmRegion reg1,
                        XmRegion reg2,
                        XmRegion newReg) ;
extern void _XmRegionUnion( 
                        XmRegion reg1,
                        XmRegion reg2,
                        XmRegion newReg) ;
extern void _XmRegionSubtract( 
                        XmRegion regM,
                        XmRegion regS,
                        XmRegion regD) ;
extern Boolean _XmRegionIsEmpty( 
                        XmRegion r) ;
extern Boolean _XmRegionEqual( 
                        XmRegion r1,
                        XmRegion r2) ;
extern Boolean _XmRegionPointInRegion( 
                        XmRegion pRegion,
                        int x,
                        int y) ;
extern void _XmRegionClear(
			XmRegion r ) ;
extern void _XmRegionShrink(
			XmRegion r,
			int dx,
                        int dy) ;
extern void _XmRegionDrawShadow(
			Display	*display,
			Drawable d,
			GC top_gc,
			GC bottom_gc,
			XmRegion region,
#if NeedWidePrototypes
			int border_thick,
			int shadow_thick,
#else
			Dimension border_thick,
			Dimension shadow_thick,
#endif /* NeedWidePrototypes */
			unsigned int shadow_type ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********    Private Function Declarations for DragUnder.c    ********/
#ifdef _NO_PROTO

extern void _XmDragUnderAnimation() ;

#else

extern void _XmDragUnderAnimation( 
                        Widget w,
                        XtPointer clientData,
                        XtPointer callData) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/********        ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#ifdef XM_1_1_BC

/* The following symbols are OBSOLETE and SHOULD NOT be used.
 * They are provided here as a source compatibilibity convenience,
 * to support the transition to the Motif 1.2 source environment.
 */

#define XmVPANED_BIT			XmPANED_WINDOW_BIT

#define LOOK_AT_SCREEN          (1<<0)
#define LOOK_AT_CMAP            (1<<1)
#define LOOK_AT_BACKGROUND      (1<<2)
#define LOOK_AT_FOREGROUND      (1<<3)
#define LOOK_AT_TOP_SHADOW      (1<<4)
#define LOOK_AT_BOTTOM_SHADOW   (1<<5)
#define LOOK_AT_SELECT          (1<<6)

#define XmStrlen(s)      ((s) ? strlen(s) : 0)

#define DEFAULT_INDICATOR_DIM   9

#ifndef MAX
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#endif

#define RX(r)		    (((RectObj) r)->rectangle.x)
#define RY(r)		    (((RectObj) r)->rectangle.y)
#define RWidth(r)	    (((RectObj) r)->rectangle.width)
#define RHeight(r)	    (((RectObj) r)->rectangle.height)
#define RBorder(r)	    (((RectObj) r)->rectangle.border_width)

#define GMode(g)	    ((g)->request_mode)
#define IsX(g)		    (GMode (g) & CWX)
#define IsY(g)		    (GMode (g) & CWY)
#define IsWidth(g)	    (GMode (g) & CWWidth)
#define IsHeight(g)	    (GMode (g) & CWHeight)
#define IsBorder(g)	    (GMode (g) & CWBorderWidth)
#define IsWidthHeight(g)    ((GMode (g) & CWWidth) || (GMode (g) & CWHeight))

#define MAXDIMENSION	((1 << 31)-1)

#define Max(x, y)	(((x) > (y)) ? (x) : (y))
#define Min(x, y)	(((x) < (y)) ? (x) : (y))
#define AssignMax(x, y)	if ((y) > (x)) x = (y)
#define AssignMin(x, y)	if ((y) < (x)) x = (y)

#define DIALOG_SUFFIX "_popup"
#define DIALOG_SUFFIX_SIZE 6

#define XM_3D_ENHANCE_PIXEL 2
#define XM_DEFAULT_TOP_MARGIN 0
#define XM_DEFAULT_BOTTOM_MARGIN 0

externalref WidgetClass xmWorldObjectClass;
externalref WidgetClass xmDesktopObjectClass;
externalref WidgetClass xmDisplayObjectClass;
externalref WidgetClass xmScreenObjectClass;


#ifndef _XmNO_BC_INCL

#include <Xm/VendorSP.h>
#include <Xm/ManagerP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/GadgetP.h>

#endif /* _XmNO_BC_INCL */

#endif /* XM_1_1_BC */


#include <Xm/BaseClassP.h>              /* To support fast subclass macros. */


#endif /* _XmP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
