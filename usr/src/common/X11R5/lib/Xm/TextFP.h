#pragma ident	"@(#)m1.2libs:Xm/TextFP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmTextFP_h
#define _XmTextFP_h

#include <Xm/PrimitiveP.h>
#include <Xm/TextF.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Defines for different cursors
 */

#define IBEAM_WIDTH 3
#define CARET_WIDTH 9
#define CARET_HEIGHT 5

#ifdef NOT_DEF
static unsigned char caretBits[] = {
   0x10, 0x00, 0x38, 0x00, 0x6c, 0x00, 0xc6, 0x00, 0x83, 0x01};
#endif /* NOT_DEF */

/*
 * Here is the Text Field Widget class structure.
 */

typedef struct _XmTextFieldClassPart
  {
    XtPointer extension;		/* Pointer to extension record. */
  }
  XmTextFieldClassPart;

typedef struct _XmTextFieldClassRec
  {
    CoreClassPart core_class;  /* Not RectObjClassPart so I can reference
				  core_class s */
    XmPrimitiveClassPart primitive_class;
    XmTextFieldClassPart text_class;
  }
  XmTextFieldClassRec;

externalref XmTextFieldClassRec xmTextFieldClassRec;

/*
 * Here is the Text Field Widget instance structures.
 */

typedef struct _XmTextFieldPart
  {
    XtCallbackList activate_callback;	   /* Command activate callback */
    XtCallbackList focus_callback;  /* Verify losing focus callback */
    XtCallbackList losing_focus_callback;  /* Verify losing focus callback */
    XtCallbackList modify_verify_callback; /* Verify value to change callback */
    XtCallbackList wcs_modify_verify_callback; /* Verify value to change 
						* callback */
    XtCallbackList motion_verify_callback; /* Verify insert cursor position to
					      change callback */
    XtCallbackList gain_primary_callback; /* Gained ownership of Primary
                                             Selection */
    XtCallbackList lose_primary_callback; /* Lost ownership of Primary
                                             Selection */
    XtCallbackList value_changed_callback; /* Notify that value has change						      callback */
    char * value;		/* pointer to widget value stored as char* */
    wchar_t * wc_value;		/* pointer to widget value stored as wchar_t* */

    XmFontList font_list;	/* Uses only the font portion of fontlist */
    XFontStruct *font;	        /* font retrieved from the fontlist */
    XmTextScanType *selection_array; /* Description of what to cycle
					     through on selections */
    _XmHighlightData highlight;    /* Info on the highlighting regions. */

    GC gc;			/* Normal GC for drawing text and cursor */
    GC image_gc;		/* Image GC for drawing text cursor*/
    GC save_gc;                 /* GC for saving/restoring under IBeam */

    Pixmap ibeam_off;		/* pixmap for area under the IBeam */
    Pixmap add_mode_cursor;	/* The add mode cursor pixmap */
    Pixmap cursor;		/* The ibeam cursor stencil */
    Pixmap putback;		/* AVAILABLE: was in 1.1 but not really used */
    Pixmap stipple_tile;	/* The tile pattern for the stippled I-beam */
    Pixmap image_clip;		/* The clip rect needed for image gc */

    XmTextPosition cursor_position;/* Character location of the insert cursor */
    XmTextPosition new_h_offset;/* Used in setvaluesalmost proc */
    XmTextPosition h_offset;  	/* The x position of the first character
                                   (relative to left edge of the widget) */
    XmTextPosition orig_left;     /* Left primary selection prior to extend */
    XmTextPosition orig_right;    /* Right primary selection prior to extend */
    XmTextPosition prim_pos_left; /* Left primary selection position */
    XmTextPosition prim_pos_right; /* Right primary selection position */
    XmTextPosition prim_anchor;	/* Primary selection pivot point */

    XmTextPosition sec_pos_left; /* Left secondary selection position */
    XmTextPosition sec_pos_right; /* Right secondary selection position */
    XmTextPosition sec_anchor;	/* Secondary selection pivot point */

    XmTextPosition stuff_pos;	/* Position to stuff the primary selection */

    Position select_pos_x;    /* x position for timer-based scrolling */

    Time prim_time;             /* Timestamp of primary selection */
    Time dest_time;             /* Timestamp of destination selection */
    Time sec_time;              /* Timestamp of secondary selection */
    Time last_time;             /* Time of last selection event */

    XtIntervalId timer_id;	/* Blinking cursor timer */
    XtIntervalId select_id;     /* Timer based scrolling identifier */

    int blink_rate;		/* Rate of blinking text cursor in msec */
    int selection_array_count;  /* Selection array count */
    int threshold;		/* Selection threshold */
    int size_allocd;		/* Size allocated for value string */
    int string_length;          /* The number of characters in the string 
				   (including the trailing NULL) */
    int cursor_height;		/* Save cursor dimensions */
    int cursor_width;		/* Save cursor dimensions */
    int sarray_index;		/* Index into selection array */
    int max_length;		/* Maximum number of character that can be
				   inserted into the text field widget */

    int max_char_size;          /* Max bytes per character in cur locale */
    short columns;		/* The number of characters in the width */

    Dimension margin_width;	/* Height between text borders and text */
    Dimension margin_height;	/* Width between text borders and text */
    Dimension average_char_width;/* Average character width based on font */
    Dimension margin_top;   /* Height between text borders and top of text */
    Dimension margin_bottom;/* Height between text borders and bottom of text */
    Dimension font_ascent;  /* Ascent of font or fontset used by widget */
    Dimension font_descent;  /* Descent of font or fontset used by widget */

    Boolean resize_width;	/* Allows the widget to grow horizontally
				   when borders are reached */
    Boolean pending_delete;	/* Delete primary selection on insert when
				   set to True */
    Boolean editable;		/* Sets editablility of text */
    Boolean verify_bell;          /* Determines if bell is sounded when verify
                                   *  callback returns doit - False
                                   */
    Boolean cursor_position_visible;	/* Sets visibility of insert cursor */

    Boolean traversed;          /* Flag used with losing focus verification to
                                   indicate a traversal key pressed event */
    Boolean add_mode;		/* Add mode for cursor movement */
    Boolean has_focus;		/* Flag that indicates whether the widget
			           has input focus */
    Boolean blink_on;		/* State of Blinking insert cursor */
    short int cursor_on;	/* Indicates whether the cursor is visible */
    Boolean refresh_ibeam_off;	/* Indicates whether the area under IBeam needs
				   to be re-captured */
    Boolean have_inverted_image_gc;  /* fg/bg of image gc have been swapped */
    Boolean has_primary;	/* Indicates that is has the
				   primary selection */
    Boolean has_secondary;	/* Indicates that is has the
				   secondary selection */
    Boolean has_destination;	/* Indicates that is has the
				   destination selection */
    Boolean sec_drag;           /* Indicates a secondary drag was made */ 
    Boolean selection_move;	/* Indicates that the action requires a
				   secondary move (i.e. copy & cut) */
    Boolean pending_off;	/* indicates pending delete state */
    Boolean fontlist_created;   /* Indicates that the text field widget created
				   it's own fontlist */
    Boolean has_rect;		/* currently has clipping rectangle */
    Boolean do_drop;		/* Indicates that the widget the recieved the
				   button release, did not have a previous
                                   button press, so it is o.k. to request
				   the MOTIF_DROP selection. */
    Boolean cancel;		/* Cancels selection actions when true */
    Boolean extending;		/* Indicates extending primary selection */
    Boolean sec_extending;      /* Indicates extending secondary selection */
    Boolean changed_visible;    /* Indicates whether the dest_visible flag
                                   is in a temporary changed state */
    Boolean have_fontset;       /* The widgets font is a fontset, not a 
				 * fontstruct... use R5 draw routines */
    Boolean in_setvalues;	/* used to disable unnecessary redisplays */
    Boolean do_resize;		/* used to prevent inappropriate resizes */
    Boolean redisplay;		/* used to set redisplay flag in setvalues */
    Boolean overstrike;		/* overstrike mode for character input */
    Boolean sel_start;		/* overstrike mode for character input */
    XtPointer extension;	/* Pointer to extension record. */
  }
  XmTextFieldPart;

typedef struct _XmTextFieldRec
  {
    CorePart core;
    XmPrimitivePart primitive;
    XmTextFieldPart text;
  }
  XmTextFieldRec;

/****************
 *
 * Macros for the uncached data
 *
 ****************/

#define TextF_ActivateCallback(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.activate_callback)
#define TextF_LosingFocusCallback(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.losing_focus_callback)
#define TextF_FocusCallback(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.focus_callback)
#define TextF_ModifyVerifyCallback(tfg)	        (((XmTextFieldWidget)(tfg)) -> \
					   text.modify_verify_callback)
#define TextF_ModifyVerifyCallbackWcs(tfg)      (((XmTextFieldWidget)(tfg)) -> \
					   text.wcs_modify_verify_callback)
#define TextF_MotionVerifyCallback(tfg)	        (((XmTextFieldWidget)(tfg)) -> \
					   text.motion_verify_callback)
#define TextF_ValueChangedCallback(tfg)	        (((XmTextFieldWidget)(tfg)) -> \
					   text.value_changed_callback)
#define TextF_Value(tfg)                        (((XmTextFieldWidget)(tfg)) -> \
					   text.value)
#define TextF_WcValue(tfg)                      (((XmTextFieldWidget)(tfg)) -> \
					   text.wc_value)
#define TextF_MarginHeight(tfg)		        (((XmTextFieldWidget)(tfg)) -> \
					   text.margin_height)
#define TextF_MarginWidth(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.margin_width)
#define TextF_CursorPosition(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.cursor_position)
#define TextF_Columns(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.columns)
#define TextF_MaxLength(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.max_length)
#define TextF_BlinkRate(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.blink_rate)
#define TextF_FontList(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.font_list)
#define TextF_Font(tfg)				(((XmTextFieldWidget)(tfg)) -> \
					   text.font)
#define TextF_FontAscent(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.font_ascent)
#define TextF_FontDescent(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.font_descent)
#define TextF_SelectionArray(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.selection_array)
#define TextF_SelectionArrayCount(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.selection_array_count)
#define TextF_ResizeWidth(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.resize_width)
#define TextF_PendingDelete(tfg)		(((XmTextFieldWidget)(tfg)) -> \
					   text.pending_delete)
#define TextF_Editable(tfg)			(((XmTextFieldWidget)(tfg)) -> \
					   text.editable)
#define TextF_CursorPositionVisible(tfg)	(((XmTextFieldWidget)(tfg)) -> \
					   text.cursor_position_visible)
#define TextF_Threshold(tfg)		   	(((XmTextFieldWidget)(tfg)) -> \
					   text.threshold)
#define TextF_UseFontSet(tfg)		   	(((XmTextFieldWidget)(tfg)) -> \
					   text.have_fontset)

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern int _XmTextFieldCountBytes() ;
extern Widget _XmTextFieldGetDropReciever() ;
extern void _XmTextFToggleCursorGC() ;
extern void _XmTextFieldDrawInsertionPoint() ;
extern void _XmTextFieldSetClipRect() ;
extern void _XmTextFieldSetCursorPosition() ;
extern Boolean _XmTextFieldReplaceText() ;
extern void _XmTextFieldDeselectSelection() ;
extern Boolean _XmTextFieldSetDestination() ;
extern void _XmTextFieldStartSelection() ;
extern void _XmTextFieldSetSel2() ;

#else

extern int _XmTextFieldCountBytes( 
                        XmTextFieldWidget tf,
                        wchar_t *wc_value,
                        int num_chars) ;
extern Widget _XmTextFieldGetDropReciever( 
                        Widget w) ;
extern void _XmTextFToggleCursorGC( 
                        Widget widget) ;
extern void _XmTextFieldDrawInsertionPoint( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int turn_on) ;
#else
                        Boolean turn_on) ;
#endif /* NeedWidePrototypes */
extern void _XmTextFieldSetClipRect( 
                        XmTextFieldWidget tf) ;
extern void _XmTextFieldSetCursorPosition( 
                        XmTextFieldWidget tf,
                        XEvent *event,
                        XmTextPosition position,
#if NeedWidePrototypes
                        int adjust_flag,
                        int call_cb) ;
#else
                        Boolean adjust_flag,
                        Boolean call_cb) ;
#endif /* NeedWidePrototypes */
extern Boolean _XmTextFieldReplaceText( 
                        XmTextFieldWidget tf,
                        XEvent *event,
                        XmTextPosition replace_prev,
                        XmTextPosition replace_next,
                        char *insert,
                        int insert_length,
#if NeedWidePrototypes
                        int move_cursor) ;
#else
                        Boolean move_cursor) ;
#endif /* NeedWidePrototypes */
extern void _XmTextFieldDeselectSelection( 
                        Widget w,
#if NeedWidePrototypes
                        int disown,
#else
                        Boolean disown,
#endif /* NeedWidePrototypes */
                        Time sel_time) ;
extern Boolean _XmTextFieldSetDestination( 
                        Widget w,
                        XmTextPosition position,
                        Time set_time) ;
extern void _XmTextFieldStartSelection( 
                        XmTextFieldWidget tf,
                        XmTextPosition left,
                        XmTextPosition right,
                        Time sel_time) ;
extern void _XmTextFieldSetSel2( 
                        Widget w,
                        XmTextPosition left,
                        XmTextPosition right,
#if NeedWidePrototypes
                        int disown,
#else
                        Boolean disown,
#endif /* NeedWidePrototypes */
                        Time sel_time) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTextFieldWidgetP_h */
