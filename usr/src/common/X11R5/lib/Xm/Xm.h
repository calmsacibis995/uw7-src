#pragma ident	"@(#)m1.2libs:Xm/Xm.h	1.3"
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
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _Xm_h
#define _Xm_h

#ifndef _NO_PROTO
#if    !(defined(__STDC__) && __STDC__) \
    && !defined(__cplusplus) && !defined(c_plusplus) \
    && !defined(FUNCPROTO) && !defined(XTFUNCPROTO) && !defined(XMFUNCPROTO)
#define _NO_PROTO
#endif /* __STDC__ */
#endif /* _NO_PROTO */

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xm/XmStrDefs.h>
#include <Xm/VirtKeys.h>

#ifdef __cplusplus
extern "C" {
#endif


#define XmVERSION	1
#define XmREVISION	2
#ifndef OSF_v1_2_4
#define XmUPDATE_LEVEL	3
#else /* OSF_v1_2_4 */
#define XmUPDATE_LEVEL	4
#endif /* OSF_v1_2_4 */
#define XmVersion (XmVERSION * 1000 + XmREVISION)
#ifndef OSF_v1_2_4
#define XmVERSION_STRING "@(#)OSF/Motif Version 1.2.3"
#else /* OSF_v1_2_4 */
#define XmVERSION_STRING "@(#)OSF/Motif Version 1.2.4"
#endif /* OSF_v1_2_4 */

externalref int xmUseVersion;


/* define used to denote an unspecified pixmap  */

#define	XmUNSPECIFIED_PIXMAP		2

/*******************
 *  
 * Defines for resources to be defaulted by vendors.
 * String are initialized in Xmos.c
 *
 ****************/

#define	XmSTRING_OS_CHARSET		XmSTRING_ISO8859_1
#ifndef	XmFALLBACK_CHARSET
#define	XmFALLBACK_CHARSET		XmSTRING_ISO8859_1
#endif

#define XmDEFAULT_FONT                  _XmSDEFAULT_FONT
#define XmDEFAULT_BACKGROUND            _XmSDEFAULT_BACKGROUND
#define XmDEFAULT_DARK_THRESHOLD        20
#define XmDEFAULT_LIGHT_THRESHOLD       90
#define XmDEFAULT_FOREGROUND_THRESHOLD  70

externalref  char    _XmSDEFAULT_FONT[];            /* In Xmos.c */
externalref  char    _XmSDEFAULT_BACKGROUND[];      /* In Xmos.c */

/****************
 *
 * XmString structure defines. These must be here (at the start of the file) 
 * becaused they are used later on.
 *
 ****************/
typedef enum{ XmFONT_IS_FONT, XmFONT_IS_FONTSET } XmFontType;

enum { XmSTRING_DIRECTION_L_TO_R,	XmSTRING_DIRECTION_R_TO_L,
       XmSTRING_DIRECTION_DEFAULT = 255
     };

typedef unsigned char XmStringDirection;      
typedef unsigned char *	XmString;		/* opaque to outside */
typedef XmString *	XmStringTable;		/* opaque to outside */
typedef char *		XmStringCharSet;	/* Null term string */
typedef unsigned char	XmStringComponentType;	/* component tags */

typedef struct _XmFontListRec       *XmFontListEntry;  /* opaque to outside */
typedef struct _XmFontListRec       *XmFontList;       /* opaque to outside */
typedef struct __XmStringContextRec *_XmStringContext; /* opaque to outside */
typedef struct __XmStringRec        *_XmString;        /* opaque to outside */
typedef struct _XmtStringContextRec *XmStringContext;  /* opaque to outside */
typedef struct _XmFontListContextRec *XmFontContext;   /* opaque to outside */

enum{	XmSTRING_COMPONENT_UNKNOWN,	XmSTRING_COMPONENT_CHARSET,
	XmSTRING_COMPONENT_TEXT,	XmSTRING_COMPONENT_DIRECTION,
        XmSTRING_COMPONENT_SEPARATOR,   XmSTRING_COMPONENT_LOCALE_TEXT
	/* 6-125 reserved */
	} ;

#define XmSTRING_COMPONENT_END		((XmStringComponentType) 126)

#define XmSTRING_COMPONENT_USER_BEGIN	((XmStringComponentType) 128)
			/* 128-255 are user tags */
#define XmSTRING_COMPONENT_USER_END	((XmStringComponentType) 255)

/************************************************************************
 *
 *  Base widget class and record definitions.
 *	Included are the definitions for XmPrimitive, XmManager,
 *      and XmGadget.
 *
 ************************************************************************/


/*  Primitive widget class and record definitions  */

#ifndef PRIMITIVE
externalref WidgetClass xmPrimitiveWidgetClass;
#endif

typedef struct _XmPrimitiveClassRec * XmPrimitiveWidgetClass;
typedef struct _XmPrimitiveRec      * XmPrimitiveWidget;


/*  Gadget widget class and record definitions  */

#ifndef GADGET
externalref WidgetClass xmGadgetClass;
#endif

typedef struct _XmGadgetClassRec * XmGadgetClass;
typedef struct _XmGadgetRec      * XmGadget;


/*  Manager widger class and record definitions  */


#ifndef MANAGER
externalref WidgetClass xmManagerWidgetClass;
#endif

typedef struct _XmManagerClassRec * XmManagerWidgetClass;
typedef struct _XmManagerRec      * XmManagerWidget;


/************************************************************************
 *  Class Test Macros (fast subclassing is in XmP.h)
 ************************************************************************/

#ifndef XmIsPrimitive
#define XmIsPrimitive(w)	XtIsSubclass(w, xmPrimitiveWidgetClass)
#endif  /* XmIsPrimitive */

#ifndef XmIsGadget
#define XmIsGadget(w)		XtIsSubclass(w, xmGadgetClass)
#endif  /* XmIsGadget */

#ifndef XmIsManager
#define XmIsManager(w)		XtIsSubclass(w, xmManagerWidgetClass)
#endif  /* XmIsManager */


/************************************************************************
 *  Primitive Resources and define values
 ************************************************************************/

/* size policy values  */

enum{	XmCHANGE_ALL,			XmCHANGE_NONE,
	XmCHANGE_WIDTH,			XmCHANGE_HEIGHT
	} ;

/*  unit type values  */

enum{	XmPIXELS,			Xm100TH_MILLIMETERS,
	Xm1000TH_INCHES,		Xm100TH_POINTS,
	Xm100TH_FONT_UNITS
	} ;

/* DeleteResponse values */

enum{	XmDESTROY,			XmUNMAP,
	XmDO_NOTHING
	} ;
enum{	XmEXPLICIT,			XmPOINTER
	} ;
/************************************************************************
 *  Navigation defines 
 ************************************************************************/

enum{	XmNONE,				XmTAB_GROUP,
	XmSTICKY_TAB_GROUP,		XmEXCLUSIVE_TAB_GROUP
	} ;

#define	XmDYNAMIC_DEFAULT_TAB_GROUP	(255)

/************************************************************************
 * Audible warning
 ************************************************************************/

enum{	/* XmNONE */			XmBELL = 1
	} ;

/************************************************************************
 *  Menu defines
 ************************************************************************/

enum{	XmNO_ORIENTATION,		XmVERTICAL,
	XmHORIZONTAL
	} ;
enum{	XmWORK_AREA,			XmMENU_BAR,
	XmMENU_PULLDOWN,		XmMENU_POPUP,
	XmMENU_OPTION
	} ;
enum{	XmNO_PACKING,			XmPACK_TIGHT,
	XmPACK_COLUMN,			XmPACK_NONE
	} ;
enum{/* XmALIGNMENT_BASELINE_TOP,	XmALIGNMENT_CENTER,
	XmALIGNMENT_BASELINE_BOTTOM, */	XmALIGNMENT_CONTENTS_TOP = 3,
	XmALIGNMENT_CONTENTS_BOTTOM
	} ;
enum{	XmTEAR_OFF_ENABLED,		XmTEAR_OFF_DISABLED
	} ;
enum{	XmUNPOST,		 	XmUNPOST_AND_REPLAY
	} ;
enum{   XmLAST_POSITION = -1,           XmFIRST_POSITION
	} ;
/************************************************************************
 *  Label/Frame defines
 ************************************************************************/

enum{	XmALIGNMENT_BEGINNING,		XmALIGNMENT_CENTER,
	XmALIGNMENT_END
	} ;
enum{   XmALIGNMENT_BASELINE_TOP,    /* XmALIGNMENT_CENTER, */
	XmALIGNMENT_BASELINE_BOTTOM = 2, XmALIGNMENT_WIDGET_TOP,
	XmALIGNMENT_WIDGET_BOTTOM
   	} ;
/************************************************************************
 *  Frame defines
 ************************************************************************/

enum{	XmFRAME_GENERIC_CHILD,          XmFRAME_WORKAREA_CHILD,
        XmFRAME_TITLE_CHILD
	} ;
/************************************************************************
 *  ToggleButton  defines
 ************************************************************************/

enum{	XmN_OF_MANY = 1,		XmONE_OF_MANY
	} ;
/************************************************************************
 *  Form defines
 ************************************************************************/

enum{	XmATTACH_NONE,			XmATTACH_FORM,
	XmATTACH_OPPOSITE_FORM,		XmATTACH_WIDGET,
	XmATTACH_OPPOSITE_WIDGET,	XmATTACH_POSITION,
	XmATTACH_SELF
	} ;
enum{	XmRESIZE_NONE,			XmRESIZE_GROW,
	XmRESIZE_ANY
	} ;
/****************************************************************************
 *  Callback reasons 
 ****************************************************************************/

enum{	XmCR_NONE,			XmCR_HELP,
	XmCR_VALUE_CHANGED,		XmCR_INCREMENT,
	XmCR_DECREMENT,			XmCR_PAGE_INCREMENT,
	XmCR_PAGE_DECREMENT,		XmCR_TO_TOP,
	XmCR_TO_BOTTOM,			XmCR_DRAG,
	XmCR_ACTIVATE,			XmCR_ARM,
	XmCR_DISARM,			XmCR_MAP = 16,
	XmCR_UNMAP,			XmCR_FOCUS,
	XmCR_LOSING_FOCUS,		XmCR_MODIFYING_TEXT_VALUE,
	XmCR_MOVING_INSERT_CURSOR,	XmCR_EXECUTE,
	XmCR_SINGLE_SELECT,		XmCR_MULTIPLE_SELECT,
	XmCR_EXTENDED_SELECT,		XmCR_BROWSE_SELECT,
	XmCR_DEFAULT_ACTION,		XmCR_CLIPBOARD_DATA_REQUEST,
	XmCR_CLIPBOARD_DATA_DELETE,	XmCR_CASCADING,
	XmCR_OK,			XmCR_CANCEL,
	XmCR_APPLY = 34,		XmCR_NO_MATCH,
	XmCR_COMMAND_ENTERED,		XmCR_COMMAND_CHANGED,
	XmCR_EXPOSE,			XmCR_RESIZE,
	XmCR_INPUT,			XmCR_GAIN_PRIMARY,
	XmCR_LOSE_PRIMARY,		XmCR_CREATE,
	XmCR_TEAR_OFF_ACTIVATE,		XmCR_TEAR_OFF_DEACTIVATE,
	XmCR_OBSCURED_TRAVERSAL,	XmCR_PROTOCOLS
#ifdef OSF_v1_2_4
	/* XmCR_FOCUS_MOVED = 48 */
#endif /* OSF_v1_2_4 */
	} ;
/************************************************************************
 *  Callback structures 
 ************************************************************************/

typedef struct
{
    int     reason;
    XEvent  *event;
} XmAnyCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    int	    click_count;
} XmArrowButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Window  window;
} XmDrawingAreaCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Window  window;
    int	    click_count;
} XmDrawnButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    int	    click_count;
} XmPushButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Widget  widget;
    char    *data;
    char    *callbackstruct;
} XmRowColumnCallbackStruct;

typedef struct
{
   int reason;
   XEvent * event;
   int value;
   int pixel;
} XmScrollBarCallbackStruct;

typedef struct
{
   int reason;
   XEvent * event;
   int set;
} XmToggleButtonCallbackStruct;

typedef struct
{
   int 	     reason;
   XEvent    *event;
   XmString  item;
   int       item_length;
   int       item_position;
   XmString  *selected_items;
   int       selected_item_count;
   int       *selected_item_positions;
   char      selection_type;
} XmListCallbackStruct;

typedef struct
{
    int reason;
    XEvent	*event;
    XmString	value;
    int		length;
} XmSelectionBoxCallbackStruct;

typedef struct
{
    int reason;
    XEvent	*event;
    XmString	value;
    int		length;
} XmCommandCallbackStruct;

typedef struct
{
    int 	reason;
    XEvent	*event;
    XmString	value;
    int		length;
    XmString	mask;
    int		mask_length;
    XmString	dir ;
    int		dir_length ;
    XmString    pattern ;
    int		pattern_length ;
} XmFileSelectionBoxCallbackStruct;


typedef struct 
{
   int reason;
   XEvent * event;
   int value;
} XmScaleCallbackStruct;


/************************************************************************
 *  PushButton defines
 ************************************************************************/

enum{	XmMULTICLICK_DISCARD,		XmMULTICLICK_KEEP
	} ;
/************************************************************************
 *  DrawnButton defines
 ************************************************************************/

enum{	XmSHADOW_IN = 7,		XmSHADOW_OUT
	} ;
/************************************************************************
 *  Arrow defines
 ************************************************************************/

enum{	XmARROW_UP,			XmARROW_DOWN,
	XmARROW_LEFT,			XmARROW_RIGHT
	} ;
/************************************************************************
 *  Separator defines
 *  Note: XmINVALID_SEPARATOR_TYPE marks the last+1 separator type
 ************************************************************************/

enum{	XmNO_LINE,			XmSINGLE_LINE,
	XmDOUBLE_LINE,			XmSINGLE_DASHED_LINE,
	XmDOUBLE_DASHED_LINE,		XmSHADOW_ETCHED_IN,
	XmSHADOW_ETCHED_OUT,		XmSHADOW_ETCHED_IN_DASH,
	XmSHADOW_ETCHED_OUT_DASH,	XmINVALID_SEPARATOR_TYPE
	} ;

enum{	XmPIXMAP = 1,			XmSTRING
	} ;

/************************************************************************
 *  Drag and Drop #defines
 ************************************************************************/

enum{	XmWINDOW,		     /* XmPIXMAP, */
	XmCURSOR = 2
	} ;

/************************************************************************
 *  ScrollBar #defines
 ************************************************************************/

enum{	XmMAX_ON_TOP,			XmMAX_ON_BOTTOM,
	XmMAX_ON_LEFT,			XmMAX_ON_RIGHT
	} ;
/************************************************************************
 *									*
 * List Widget defines							*
 *									*
 ************************************************************************/

enum{	XmSINGLE_SELECT,		XmMULTIPLE_SELECT,
	XmEXTENDED_SELECT,		XmBROWSE_SELECT
	} ;
enum{	XmSTATIC,			XmDYNAMIC
	} ;
/************************************************************************
 *									*
 * Scrolled Window defines.						*
 *									*
 ************************************************************************/

enum{	XmVARIABLE,			XmCONSTANT,
	XmRESIZE_IF_POSSIBLE
	} ;
enum{	XmAUTOMATIC,			XmAPPLICATION_DEFINED
	} ;
enum{	/* XmSTATIC */			XmAS_NEEDED = 1
	} ;

#define SW_TOP		1
#define SW_BOTTOM	0
#define SW_LEFT		2
#define SW_RIGHT	0

#define XmTOP_LEFT	(SW_TOP | SW_LEFT)
#define XmBOTTOM_LEFT	(SW_BOTTOM  | SW_LEFT)
#define XmTOP_RIGHT	(SW_TOP | SW_RIGHT)
#define XmBOTTOM_RIGHT	(SW_BOTTOM  | SW_RIGHT)

/************************************************************************
 *									*
 * MainWindow Resources                                                 *
 *									*
 ************************************************************************/

enum{	XmCOMMAND_ABOVE_WORKSPACE,	XmCOMMAND_BELOW_WORKSPACE
	} ;
/************************************************************************
 *									*
 * Text Widget defines							*
 *									*
 ************************************************************************/

enum{	XmMULTI_LINE_EDIT,		XmSINGLE_LINE_EDIT
	} ;

typedef enum{
	XmTEXT_FORWARD,
	XmTEXT_BACKWARD
	} XmTextDirection;

typedef long XmTextPosition;
typedef Atom XmTextFormat;

#define XmFMT_8_BIT	((XmTextFormat) XA_STRING)	/* 8-bit text. */
#define XmFMT_16_BIT	((XmTextFormat) 2)		/* 16-bit text. */

#define FMT8BIT		XmFMT_8_BIT	/* For backwards compatibility only.*/
#define FMT16BIT	XmFMT_16_BIT	/* For backwards compatibility only.*/

typedef enum{
	XmSELECT_POSITION,		XmSELECT_WHITESPACE,
	XmSELECT_WORD,			XmSELECT_LINE,
	XmSELECT_ALL,			XmSELECT_PARAGRAPH
	} XmTextScanType ;

typedef enum{
	XmHIGHLIGHT_NORMAL,		XmHIGHLIGHT_SELECTED,
	XmHIGHLIGHT_SECONDARY_SELECTED
	} XmHighlightMode ;

/* XmTextBlock's are used to pass text around. */

typedef struct {
    char *ptr;                  /* Pointer to data. */
    int length;                 /* Number of characters (not bytes) of data. */
    XmTextFormat format;       /* Representations format */
} XmTextBlockRec, *XmTextBlock;

typedef struct
{
    int reason;
    XEvent  *event;
    Boolean doit;
    long currInsert, newInsert;
    long startPos, endPos;
    XmTextBlock text;
} XmTextVerifyCallbackStruct, *XmTextVerifyPtr;

/* XmTextBlockWcs's are used in 1.2 modifyVerifyWcs callbacks for Text[Field]
 * widgets. */

typedef struct {
    wchar_t *wcsptr;            /* Pointer to data. */
    int length;                 /* Number of characters (not bytes) of data. */
} XmTextBlockRecWcs, *XmTextBlockWcs;

typedef struct
{
    int reason;
    XEvent  *event;
    Boolean doit;
    long currInsert, newInsert;
    long startPos, endPos;
    XmTextBlockWcs text;
} XmTextVerifyCallbackStructWcs, *XmTextVerifyPtrWcs;

/* functions renamed after 1.0 release due to resource name overlap */
#define XmTextGetTopPosition                XmTextGetTopCharacter
#define XmTextSetTopPosition                XmTextSetTopCharacter

#define XmCOPY_FAILED		0
#define XmCOPY_SUCCEEDED	1
#define XmCOPY_TRUNCATED	2

/************************************************************************
 *									*
 *  DIALOG defines..  BulletinBoard and things common to its subclasses *
 *          CommandBox    MessageBox    Selection    FileSelection      *
 *									*
 ************************************************************************/

/* child type defines for Xm...GetChild() */

enum{	XmDIALOG_NONE,			XmDIALOG_APPLY_BUTTON,
	XmDIALOG_CANCEL_BUTTON,		XmDIALOG_DEFAULT_BUTTON,
	XmDIALOG_OK_BUTTON,		XmDIALOG_FILTER_LABEL,
	XmDIALOG_FILTER_TEXT,		XmDIALOG_HELP_BUTTON,
	XmDIALOG_LIST,			XmDIALOG_LIST_LABEL,
	XmDIALOG_MESSAGE_LABEL,		XmDIALOG_SELECTION_LABEL,
	XmDIALOG_SYMBOL_LABEL,		XmDIALOG_TEXT,
	XmDIALOG_SEPARATOR,		XmDIALOG_DIR_LIST,
	XmDIALOG_DIR_LIST_LABEL
	} ;

#define XmDIALOG_HISTORY_LIST     	XmDIALOG_LIST
#define XmDIALOG_PROMPT_LABEL     	XmDIALOG_SELECTION_LABEL
#define XmDIALOG_VALUE_TEXT       	XmDIALOG_TEXT
#define XmDIALOG_COMMAND_TEXT     	XmDIALOG_TEXT
#define XmDIALOG_FILE_LIST        	XmDIALOG_LIST
#define XmDIALOG_FILE_LIST_LABEL  	XmDIALOG_LIST_LABEL

/*  dialog style defines  */

enum{	XmDIALOG_MODELESS,		XmDIALOG_PRIMARY_APPLICATION_MODAL,
	XmDIALOG_FULL_APPLICATION_MODAL,XmDIALOG_SYSTEM_MODAL
	} ;

/* The following is for compatibility only. Its use is deprecated.
 */
#define XmDIALOG_APPLICATION_MODAL	XmDIALOG_PRIMARY_APPLICATION_MODAL

/************************************************************************
 * XmSelectionBox, XmFileSelectionBox and XmCommand - misc. stuff       *
 ***********************************************************************/

/* Defines for Selection child placement
*/
enum{	XmPLACE_TOP,			XmPLACE_ABOVE_SELECTION,
	XmPLACE_BELOW_SELECTION
	} ;

/* Defines for file type mask:
*/
#define XmFILE_DIRECTORY (1 << 0)
#define XmFILE_REGULAR   (1 << 1)
#define XmFILE_ANY_TYPE  (XmFILE_DIRECTORY | XmFILE_REGULAR)

/* Defines for selection dialog type:
*/
enum{	XmDIALOG_WORK_AREA,		XmDIALOG_PROMPT,
	XmDIALOG_SELECTION,		XmDIALOG_COMMAND,
	XmDIALOG_FILE_SELECTION
	} ;

/************************************************************************
 *  XmMessageBox           stuff not common to other dialogs            *
 ***********************************************************************/

/* defines for dialog type */

enum{	XmDIALOG_TEMPLATE,		XmDIALOG_ERROR,
	XmDIALOG_INFORMATION,		XmDIALOG_MESSAGE,
	XmDIALOG_QUESTION,		XmDIALOG_WARNING,
	XmDIALOG_WORKING
	} ;

/*  Traversal types  */

typedef enum{
	XmVISIBILITY_UNOBSCURED,	XmVISIBILITY_PARTIALLY_OBSCURED,
	XmVISIBILITY_FULLY_OBSCURED
	} XmVisibility ;


typedef enum{
	XmTRAVERSE_CURRENT,		XmTRAVERSE_NEXT,
	XmTRAVERSE_PREV,		XmTRAVERSE_HOME,
	XmTRAVERSE_NEXT_TAB_GROUP,	XmTRAVERSE_PREV_TAB_GROUP,
	XmTRAVERSE_UP,			XmTRAVERSE_DOWN,
	XmTRAVERSE_LEFT,		XmTRAVERSE_RIGHT
	} XmTraversalDirection ;

typedef struct _XmTraverseObscuredCallbackStruct
{	int			reason ;
	XEvent *		event ;
	Widget			traversal_destination ;
	XmTraversalDirection	direction ;
	} XmTraverseObscuredCallbackStruct ;

typedef unsigned char   XmNavigationType;


/***********************************************************************
 *
 * SimpleMenu declarations and definitions.
 *
 ***********************************************************************/

typedef unsigned char XmButtonType;
typedef XmButtonType * XmButtonTypeTable;
typedef KeySym * XmKeySymTable;
typedef XmStringCharSet * XmStringCharSetTable;

enum{	XmPUSHBUTTON = 1,		XmTOGGLEBUTTON,
	XmRADIOBUTTON,			XmCASCADEBUTTON,
	XmSEPARATOR,			XmDOUBLE_SEPARATOR,
	XmTITLE
	} ;
#define XmCHECKBUTTON			XmTOGGLEBUTTON


/********    BaseClass.c    ********/
#ifdef _NO_PROTO
typedef XtPointer	(*XmResourceBaseProc)();
#else
typedef XtPointer	(*XmResourceBaseProc)( Widget, XtPointer) ;
#endif

typedef struct _XmSecondaryResourceDataRec{
    XmResourceBaseProc	base_proc;
    XtPointer		client_data;
    String		name;
    String		res_class;
    XtResourceList	resources;
    Cardinal		num_resources;
}XmSecondaryResourceDataRec, *XmSecondaryResourceData;

/********    Public Function Declarations for BaseClass.c    ********/
#ifdef _NO_PROTO

extern Cardinal XmGetSecondaryResourceData() ;

#else

extern Cardinal XmGetSecondaryResourceData( 
                        WidgetClass w_class,
                        XmSecondaryResourceData **secondaryDataRtn) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for ImageCache.c    ********/
#ifdef _NO_PROTO

extern Boolean XmInstallImage() ;
extern Boolean XmUninstallImage() ;
extern Pixmap XmGetPixmap() ;
extern Pixmap XmGetPixmapByDepth() ;
extern Boolean XmDestroyPixmap() ;

#else

extern Boolean XmInstallImage( 
                        XImage *image,
                        char *image_name) ;
extern Boolean XmUninstallImage( 
                        XImage *image) ;
extern Pixmap XmGetPixmap( 
                        Screen *screen,
                        char *image_name,
                        Pixel foreground,
                        Pixel background) ;
extern Pixmap XmGetPixmapByDepth( 
                        Screen *screen,
                        char *image_name,
                        Pixel foreground,
                        Pixel background,
			int depth) ;
extern Boolean XmDestroyPixmap( 
                        Screen *screen,
                        Pixmap pixmap) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for Manager.c    ********/
#ifdef _NO_PROTO

extern void XmUpdateDisplay() ;

#else

extern void XmUpdateDisplay( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Primitive.c    ********/

typedef long XmOffset;
typedef XmOffset *XmOffsetPtr;

/********    Public Function Declarations for Primitive.c    ********/
#ifdef _NO_PROTO

extern void XmResolvePartOffsets() ;
extern void XmResolveAllPartOffsets() ;
extern Boolean XmWidgetGetBaselines();
extern Boolean XmWidgetGetDisplayRect();
#else

extern void XmResolvePartOffsets( 
                        WidgetClass w_class,
                        XmOffsetPtr *offset) ;
extern void XmResolveAllPartOffsets( 
                        WidgetClass w_class,
                        XmOffsetPtr *offset,
                        XmOffsetPtr *constraint_offset) ;
extern Boolean XmWidgetGetBaselines(
                        Widget wid,
                        Dimension **baselines,
                        int *line_count);
extern Boolean XmWidgetGetDisplayRect(
                        Widget wid,
                        XRectangle *displayrect);

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for ResConvert.c    ********/
#ifdef _NO_PROTO

extern void XmRegisterConverters() ;
extern void XmCvtStringToUnitType() ;
extern char * XmRegisterSegmentEncoding() ;
extern char * XmMapSegmentEncoding() ;
extern XmString XmCvtCTToXmString() ;
extern Boolean XmCvtTextToXmString() ;
extern char * XmCvtXmStringToCT() ;
extern Boolean XmCvtXmStringToText() ;

#else

extern void XmRegisterConverters( void ) ;
extern void XmCvtStringToUnitType( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
extern char * XmRegisterSegmentEncoding( 
                        char *fontlist_tag,
                        char *ct_encoding) ;
extern char * XmMapSegmentEncoding( 
                        char *fontlist_tag) ;
extern XmString XmCvtCTToXmString( 
                        char *text) ;
extern Boolean XmCvtTextToXmString( 
                        Display *display,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val,
                        XtPointer *converter_data) ;
extern char * XmCvtXmStringToCT( 
                        XmString string) ;
extern Boolean XmCvtXmStringToText( 
                        Display *display,
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val,
                        XtPointer *converter_data) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for ResInd.c    ********/
#ifdef _NO_PROTO

extern int XmConvertUnits() ;
extern int XmCvtToHorizontalPixels() ;
extern int XmCvtToVerticalPixels() ;
extern int XmCvtFromHorizontalPixels() ;
extern int XmCvtFromVerticalPixels() ;
extern void XmSetFontUnits() ;
extern void XmSetFontUnit() ;

#else

extern int XmConvertUnits( 
                        Widget widget,
                        int dimension,
                        register int from_type,
                        register int from_val,
                        register int to_type) ;
extern int XmCvtToHorizontalPixels( 
                        Screen *screen,
                        register int from_val,
                        register int from_type) ;
extern int XmCvtToVerticalPixels( 
                        Screen *screen,
                        register int from_val,
                        register int from_type) ;
extern int XmCvtFromHorizontalPixels( 
                        Screen *screen,
                        register int from_val,
                        register int to_type) ;
extern int XmCvtFromVerticalPixels( 
                        Screen *screen,
                        register int from_val,
                        register int to_type) ;
extern void XmSetFontUnits( 
                        Display *display,
                        int h_value,
                        int v_value) ;
extern void XmSetFontUnit( 
                        Display *display,
                        int value) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for MenuUtil.c    ********/
#ifdef _NO_PROTO

extern void XmSetMenuCursor() ;
extern Cursor XmGetMenuCursor() ;

#else

extern void XmSetMenuCursor( 
                        Display *display,
                        Cursor cursorId) ;
extern Cursor XmGetMenuCursor( 
                        Display *display) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for Simple.c    ********/
#ifdef _NO_PROTO

extern Widget XmCreateSimpleMenuBar() ;
extern Widget XmCreateSimplePopupMenu() ;
extern Widget XmCreateSimplePulldownMenu() ;
extern Widget XmCreateSimpleOptionMenu() ;
extern Widget XmCreateSimpleRadioBox() ;
extern Widget XmCreateSimpleCheckBox() ;

#else

extern Widget XmCreateSimpleMenuBar( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;
extern Widget XmCreateSimplePopupMenu( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;
extern Widget XmCreateSimplePulldownMenu( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;
extern Widget XmCreateSimpleOptionMenu( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;
extern Widget XmCreateSimpleRadioBox( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;
extern Widget XmCreateSimpleCheckBox( 
                        Widget parent,
                        String name,
                        ArgList args,
                        Cardinal arg_count) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for VaSimple.c   ********/
#ifdef _NO_PROTO
extern Widget XmVaCreateSimpleMenuBar() ;
extern Widget XmVaCreateSimplePopupMenu() ;
extern Widget XmVaCreateSimplePulldownMenu() ;
extern Widget XmVaCreateSimpleOptionMenu() ;
extern Widget XmVaCreateSimpleRadioBox() ;
extern Widget XmVaCreateSimpleCheckBox() ;
#else /* _NO_PROTO */
extern Widget XmVaCreateSimpleMenuBar( 
			Widget parent,
			String name,
			...) ;
extern Widget XmVaCreateSimplePopupMenu(
			Widget parent,
			String name,
			XtCallbackProc callback,
			...) ;
extern Widget XmVaCreateSimplePulldownMenu( 
			Widget parent,
			String name,
			int post_from_button,
			XtCallbackProc callback,
			...) ;
extern Widget XmVaCreateSimpleOptionMenu(
			Widget parent,
			String name,
                        XmString option_label,
                        KeySym option_mnemonic,
                        int button_set,
                        XtCallbackProc callback,
			...) ;
extern Widget XmVaCreateSimpleRadioBox( 
			Widget parent,
			String name,
			int button_set,
			XtCallbackProc callback,
			...) ;
extern Widget XmVaCreateSimpleCheckBox( 
			Widget parent,
			String name,
			XtCallbackProc callback,
			...) ;
#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for TrackLoc.c    ********/
#ifdef _NO_PROTO

extern Widget XmTrackingEvent() ;
extern Widget XmTrackingLocate() ;

#else

extern Widget XmTrackingEvent( 
                        Widget widget,
                        Cursor cursor,
#if NeedWidePrototypes
                        int confineTo,
#else
                        Boolean confineTo,
#endif /* NeedWidePrototypes */
                        XEvent *pev) ;
extern Widget XmTrackingLocate( 
                        Widget widget,
                        Cursor cursor,
#if NeedWidePrototypes
                        int confineTo) ;
#else
                        Boolean confineTo) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Visual.c    ********/
#ifdef _NO_PROTO
typedef void (*XmColorProc) ();
#else
typedef void (*XmColorProc) (XColor *bg_color, XColor *fg_color,
	XColor *sel_color, XColor *ts_color, XColor *bs_color);
#endif

/********    Public Function Declarations for Visual.c    ********/
#ifdef _NO_PROTO

extern XmColorProc XmSetColorCalculation() ;
extern XmColorProc XmGetColorCalculation() ;
extern void XmGetColors() ;
extern void XmChangeColor() ;

#else

extern XmColorProc XmSetColorCalculation( 
                        XmColorProc proc) ;
extern XmColorProc XmGetColorCalculation( void ) ;
extern void XmGetColors( 
                        Screen *screen,
                        Colormap color_map,
                        Pixel background,
                        Pixel *foreground_ret,
                        Pixel *top_shadow_ret,
                        Pixel *bottom_shadow_ret,
                        Pixel *select_ret) ;
extern void XmChangeColor(
                        Widget widget,
                        Pixel background) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for XmString.c    ********/
#ifdef _NO_PROTO

extern XmString XmStringCreate() ;
extern XmString XmStringCreateSimple() ;
extern XmString XmStringCreateLocalized() ;
extern XmString XmStringDirectionCreate() ;
extern XmString XmStringSeparatorCreate() ;
extern XmString XmStringSegmentCreate() ;
extern XmString XmStringLtoRCreate() ;
extern XmString XmStringCreateLtoR() ;
extern Boolean XmStringInitContext() ;
extern void XmStringFreeContext() ;
extern XmStringComponentType XmStringGetNextComponent() ;
extern XmStringComponentType XmStringPeekNextComponent() ;
extern Boolean XmStringGetNextSegment() ;
extern Boolean XmStringGetLtoR() ;
extern XmFontListEntry XmFontListEntryCreate() ;
extern void XmFontListEntryFree() ;
extern XtPointer XmFontListEntryGetFont() ;
extern char * XmFontListEntryGetTag() ;
extern XmFontList XmFontListAppendEntry() ;
extern XmFontListEntry XmFontListNextEntry() ;
extern XmFontList XmFontListRemoveEntry() ;
extern XmFontListEntry XmFontListEntryLoad() ;
extern XmFontList XmFontListCreate() ;
extern XmFontList XmStringCreateFontList() ;
extern void XmFontListFree() ;
extern XmFontList XmFontListAdd() ;
extern XmFontList XmFontListCopy() ;
extern Boolean XmFontListInitFontContext() ;
extern Boolean XmFontListGetNextFont() ;
extern void XmFontListFreeFontContext() ;
extern XmString XmStringConcat() ;
extern XmString XmStringNConcat() ;
extern XmString XmStringCopy() ;
extern XmString XmStringNCopy() ;
extern Boolean XmStringByteCompare() ;
extern Boolean XmStringCompare() ;
extern int XmStringLength() ;
extern Boolean XmStringEmpty() ;
extern Boolean XmStringHasSubstring() ;
extern void XmStringFree() ;
extern Dimension XmStringBaseline() ;
extern Dimension XmStringWidth() ;
extern Dimension XmStringHeight() ;
extern void XmStringExtent() ;
extern int XmStringLineCount() ;
extern void XmStringDraw() ;
extern void XmStringDrawImage() ;
extern void XmStringDrawUnderline() ;

#else

extern XmString XmStringCreate( 
                        char *text,
                        XmStringCharSet charset) ;
extern XmString XmStringCreateSimple( 
                        char *text) ;
extern XmString XmStringCreateLocalized( 
                        String text) ;
extern XmString XmStringDirectionCreate( 
#if NeedWidePrototypes
                        int direction) ;
#else
                        XmStringDirection direction) ;
#endif /* NeedWidePrototypes */
extern XmString XmStringSeparatorCreate( void ) ;
extern XmString XmStringSegmentCreate( 
                        char *text,
                        XmStringCharSet charset,
#if NeedWidePrototypes
                        int direction,
                        int separator) ;
#else
                        XmStringDirection direction,
                        Boolean separator) ;
#endif /* NeedWidePrototypes */
extern XmString XmStringLtoRCreate( 
                        char *text,
                        XmStringCharSet charset) ;
extern XmString XmStringCreateLtoR( 
                        char *text,
                        XmStringCharSet charset) ;
extern Boolean XmStringInitContext( 
                        XmStringContext *context,
                        XmString string) ;
extern void XmStringFreeContext( 
                        XmStringContext context) ;
extern XmStringComponentType XmStringGetNextComponent( 
                        XmStringContext context,
                        char **text,
                        XmStringCharSet *charset,
                        XmStringDirection *direction,
                        XmStringComponentType *unknown_tag,
                        unsigned short *unknown_length,
                        unsigned char **unknown_value) ;
extern XmStringComponentType XmStringPeekNextComponent( 
                        XmStringContext context) ;
extern Boolean XmStringGetNextSegment( 
                        XmStringContext context,
                        char **text,
                        XmStringCharSet *charset,
                        XmStringDirection *direction,
                        Boolean *separator) ;
extern Boolean XmStringGetLtoR( 
                        XmString string,
                        XmStringCharSet charset,
                        char **text) ;
extern XmFontListEntry XmFontListEntryCreate( 
                        char *tag,
                        XmFontType type,
                        XtPointer font) ;
extern void XmFontListEntryFree( 
                        XmFontListEntry *entry) ;
extern XtPointer XmFontListEntryGetFont( 
                        XmFontListEntry entry,
                        XmFontType *typeReturn) ;
extern char * XmFontListEntryGetTag( 
                        XmFontListEntry entry) ;
extern XmFontList XmFontListAppendEntry( 
                        XmFontList old,
                        XmFontListEntry entry) ;
extern XmFontListEntry XmFontListNextEntry( 
                        XmFontContext context) ;
extern XmFontList XmFontListRemoveEntry( 
                        XmFontList old,
                        XmFontListEntry entry) ;
extern XmFontListEntry XmFontListEntryLoad( 
                        Display *display,
                        char *fontName,
                        XmFontType type,
                        char *tag) ;
extern XmFontList XmFontListCreate( 
                        XFontStruct *font,
                        XmStringCharSet charset) ;
extern XmFontList XmStringCreateFontList( 
                        XFontStruct *font,
                        XmStringCharSet charset) ;
extern void XmFontListFree( 
                        XmFontList fontlist) ;
extern XmFontList XmFontListAdd( 
                        XmFontList old,
                        XFontStruct *font,
                        XmStringCharSet charset) ;
extern XmFontList XmFontListCopy( 
                        XmFontList fontlist) ;
extern Boolean XmFontListInitFontContext( 
                        XmFontContext *context,
                        XmFontList fontlist) ;
extern Boolean XmFontListGetNextFont( 
                        XmFontContext context,
                        XmStringCharSet *charset,
                        XFontStruct **font) ;
extern void XmFontListFreeFontContext( 
                        XmFontContext context) ;
extern XmString XmStringConcat( 
                        XmString a,
                        XmString b) ;
extern XmString XmStringNConcat( 
                        XmString first,
                        XmString second,
                        int n) ;
extern XmString XmStringCopy( 
                        XmString string) ;
extern XmString XmStringNCopy( 
                        XmString str,
                        int n) ;
extern Boolean XmStringByteCompare( 
                        XmString a1,
                        XmString b1) ;
extern Boolean XmStringCompare( 
                        XmString a,
                        XmString b) ;
extern int XmStringLength( 
                        XmString string) ;
extern Boolean XmStringEmpty( 
                        XmString string) ;
extern Boolean XmStringHasSubstring( 
                        XmString string,
                        XmString substring) ;
extern void XmStringFree( 
                        XmString string) ;
extern Dimension XmStringBaseline( 
                        XmFontList fontlist,
                        XmString string) ;
extern Dimension XmStringWidth( 
                        XmFontList fontlist,
                        XmString string) ;
extern Dimension XmStringHeight( 
                        XmFontList fontlist,
                        XmString string) ;
extern void XmStringExtent( 
                        XmFontList fontlist,
                        XmString string,
                        Dimension *width,
                        Dimension *height) ;
extern int XmStringLineCount( 
                        XmString string) ;
extern void XmStringDraw( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        XmString string,
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
extern void XmStringDrawImage( 
                        Display *d,
                        Window w,
                        XmFontList fontlist,
                        XmString string,
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
extern void XmStringDrawUnderline( 
                        Display *d,
                        Window w,
                        XmFontList fntlst,
                        XmString str,
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
                        XmString under) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for Dest.c    ********/
#ifdef _NO_PROTO

extern Widget XmGetDestination() ;

#else

extern Widget XmGetDestination( 
                        Display *display) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********    Public Function Declarations for Traversal.c    ********/
#ifdef _NO_PROTO

extern Boolean XmIsTraversable() ;
extern XmVisibility XmGetVisibility() ;
extern Widget XmGetTabGroup() ;
extern Widget XmGetFocusWidget() ;
extern Boolean XmProcessTraversal() ;
extern void XmAddTabGroup() ;
extern void XmRemoveTabGroup() ;

#else

extern Boolean XmIsTraversable( 
                        Widget wid) ;
extern XmVisibility XmGetVisibility( 
                        Widget wid) ;
extern Widget XmGetTabGroup( 
                        Widget wid) ;
extern Widget XmGetFocusWidget( 
                        Widget wid) ;
extern Boolean XmProcessTraversal( 
                        Widget w,
                        XmTraversalDirection dir) ;
extern void XmAddTabGroup( 
                        Widget tabGroup) ;
extern void XmRemoveTabGroup( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


/********    Public Function Declarations for XmIm.c    ********/
#ifdef _NO_PROTO

extern void XmImRegister() ;
extern void XmImUnregister() ;
extern void XmImSetFocusValues() ;
extern void XmImSetValues() ;
extern void XmImUnsetFocus() ;
extern XIM XmImGetXIM() ;
extern int XmImMbLookupString() ;
extern void XmImVaSetFocusValues() ;
extern void XmImVaSetValues() ;

#else

extern void XmImRegister( 
                        Widget w,
			unsigned int reserved) ;
extern void XmImUnregister( 
                        Widget w) ;
extern void XmImSetFocusValues( 
                        Widget w,
                        ArgList args,
                        Cardinal num_args) ;
extern void XmImSetValues( 
                        Widget w,
                        ArgList args,
                        Cardinal num_args) ;
extern void XmImUnsetFocus( 
                        Widget w) ;
extern XIM XmImGetXIM( 
                        Widget w) ;
extern int XmImMbLookupString( 
                        Widget w,
                        XKeyPressedEvent *event,
                        char *buf,
                        int nbytes,
                        KeySym *keysym,
                        int *status) ;
extern void XmImVaSetFocusValues( 
                        Widget w,
                        ...) ;
extern void XmImVaSetValues( 
                        Widget w,
                        ...) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

/********        ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

/* The following includes are for source compatibility.  They might be
*    removed at some future time.
*/
#include <Xm/VendorS.h>

#endif /* _Xm_h */
 /* DON'T ADD STUFF AFTER THIS #endif */
