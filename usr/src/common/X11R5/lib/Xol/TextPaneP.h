#ifndef	NOIDENT
#ident	"@(#)oldtext:TextPaneP.h	1.13"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        TextPaneP.h
 **
 **   Project:     X Widgets
 **
 **   Description: TextPane widget private include file
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 *****************************************************************************
 *************************************<+>*************************************/

#ifndef _OlTextPaneP_h
#define _OlTextPaneP_h


#include <Xol/DisplayP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/SourceP.h>
#include <Xol/TextPane.h>
#include <Xol/TextPosP.h>


/****************************************************************
 *
 * TextPane widget private
 *
 ****************************************************************/

#define MAXCUT	30000	/* Maximum number of characters that can be cut. */

#define LF	0x0a
#define CR	0x0d
#define TAB	0x09
#define BS	0x08
#define SP	0x20
#define DEL	0x7f
#define BSLASH	'\\'

#define isNewline(c) (c=='\n')
#define isWhiteSpace(c) ( c==' ' || c=='\t' || c=='\n' || c=='\r' )

/* dynamic resources bit masks */
#define OL_B_TEXTPANE_BG		(1 << 0)
#define OL_B_TEXTPANE_FONTCOLOR		(1 << 1)

typedef int OlTextLineRange ;

/* NOTE:  more of the following information will eventually be moved
   to the files SourceP.h and DisplayP.h.
   */
  
/* Private TextPane Definitions */

typedef int (*ActionProc)();

typedef OlSelectType SelectionArray[20];

typedef struct {
  unsigned char *string;
  int value;
} OlSetValuePair;
  
/*************************************************************************
*
*  #define's for inheriting TextPaneClass procedures.
*
*************************************************************************/

#define XtInheritCopySubstring	((unsigned char *(*)())_XtInherit)
#define XtInheritCopySelection	((unsigned char *(*)())_XtInherit)
#define XtInheritUnsetSelection	((XtWidgetProc)_XtInherit)
#define XtInheritSetSelection	((XtWidgetProc)_XtInherit)
#define XtInheritReplaceText	((int (*)())_XtInherit)
#define XtInheritRedrawText	((XtWidgetProc)_XtInherit)

unsigned char *	_OlTextCopySubString();
void _OlUpdateVerticalSB();
void _OlTextScrollAbsolute();

/*************************************************************************
*
*  New fields for the TextPane widget class record
*
************************************************************************/
typedef struct {
  unsigned char*  (*copy_substring)();
  unsigned char*  (*copy_selection)();
  XtWidgetProc    unset_selection;
  void    	  (*set_selection)();
  int		    (*replace_text)();
  XtWidgetProc    redraw_text;
} TextPaneClassPart;

/*************************************************************************
*
* Full class record declaration for Text class
*
************************************************************************/
typedef struct _TextPaneClassRec {
    CoreClassPart	 core_class;
    PrimitiveClassPart	 primitive_class;
    TextPaneClassPart	 textedit_class;
} TextPaneClassRec;

extern TextPaneClassRec textPaneClassRec; 

/*************************************************************************
*
* New fields for the TextPane widget instance record 
*
************************************************************************/
typedef struct _TextPanePart {
    OlDefine	    srctype;        /* used by args & rm to set source */
    OlTextSource    *source;
    OlTextSink	    *sink;
    OlLineTable     lt;
    OlPositionTable pt;		    /* table of first positions for each line*/
    OlTextPosition  insertPos;
    OlTextPosition  oldinsert;
    OlInsertState   insert_state;
    OlTextSelection s;
    OlScanDirection extendDir;
    OlTextSelection origSel;        /* the selection being modified */
    char	*clip_contents;     /* the selection on the CLIPBOARD */
    SelectionArray  sarray;         /* Array to cycle for selections. */
    Dimension       leftmargin;     /* Width of left margin. */
    Dimension       rightmargin;    /* Width of right margin. */
    Dimension       topmargin;      /* Width of top margin. */
    Dimension       bottommargin;   /* Width of bottom margin. */
    int             options;        /* wordbreak, scroll, etc. */
    Time            lasttime;       /* timestamp of last processed action */
    Time            time;           /* time of last key or button action */ 
    Position        ev_x, ev_y;     /* x, y coords for key or button action */
    OlTextPosition  *updateFrom;    /* Array of start positions for update. */
    OlTextPosition  *updateTo;      /* Array of end positions for update. */
    int             numranges;      /* How many update ranges there are. */
    int             maxranges;      /* How many ranges we have space for */
    Boolean         showposition;   /* True if we need to show the position. */
    GC              gc;
    XtCallbackList  mark;
    XtCallbackList  motion_verification;
    XtCallbackList  modify_verification;
    XtCallbackList  leave_verification;
    Boolean	    wrap_mode;
    OlDefine        wrap_form;
    OlDefine        wrap_break;
    OlDefine        scroll_mode;
    OlDefine        scroll_state;
    OlDefine        grow_mode;
    OlDefine        grow_state;     /* tells whether further growth should
				       be attempted */
    Dimension	    prevW, prevH;   /* prev values of window width, height */
    Widget          verticalSB;
    char *          file;
    char *          string;
    unsigned char   dyn_flags;
    Boolean         transparent;
    Boolean         recompute_size;
    Boolean         visible_insertion;
    Boolean         update_flag;   /* turn updates on and off */
    XtCallbackList execute;        /* Execute callback list */
	void (*text_clear_buffer)();
	unsigned char* (*text_copy_buffer)();
	OlTextPosition (*text_get_insert_point)();
	OlTextPosition (*text_get_last_pos)();
	void (*text_insert)();
	int (*text_read_sub_str)();
	void (*text_redraw)();
	int    (*text_replace)();
	void (*text_set_insert_point)();
	void (*text_set_source)();
	void (*text_update)();
} TextPanePart;

/****************************************************************
 *
 * Full instance record declaration
 *
****************************************************************/

typedef struct _TextPaneRec {
  CorePart	core;
  PrimitivePart	primitive;
  TextPanePart	text;
} TextPaneRec;


#endif
/* DON'T ADD STUFF AFTER THIS #endif */
