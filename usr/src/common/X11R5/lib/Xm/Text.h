#pragma ident	"@(#)m1.2libs:Xm/Text.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmText_h
#define _XmText_h

#include <Xm/Xm.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------- *
 *   type defines *
 * -------------- */
typedef struct _XmTextSourceRec *XmTextSource;
typedef struct _XmTextClassRec *XmTextWidgetClass;
typedef struct _XmTextRec *XmTextWidget;

/* -------------- *
 * extern class   *
 * -------------- */
externalref WidgetClass       xmTextWidgetClass;


/* --------------------------------------- *
 *  text widget fast subclassing fallback  *
 * --------------------------------------- */

#ifndef XmIsText
#define XmIsText(w)	XtIsSubclass(w, xmTextWidgetClass)
#endif /* XmIsText */


/* ----------------------------------- *
 *   text widget public functions      *
 * ----------------------------------- */

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmTextSetHighlight() ;
extern Widget XmCreateScrolledText() ;
extern Widget XmCreateText() ;
extern int XmTextGetSubstring() ;
extern int XmTextGetSubstringWcs() ;
extern char * XmTextGetString() ;
extern wchar_t * XmTextGetStringWcs() ;
extern XmTextPosition XmTextGetLastPosition() ;
extern void XmTextSetString() ;
extern void XmTextSetStringWcs() ;
extern void XmTextReplace() ;
extern void XmTextReplaceWcs() ;
extern void XmTextInsert() ;
extern void XmTextInsertWcs() ;
extern void XmTextSetAddMode() ;
extern Boolean XmTextGetAddMode() ;
extern Boolean XmTextGetEditable() ;
extern void XmTextSetEditable() ;
extern int XmTextGetMaxLength() ;
extern void XmTextSetMaxLength() ;
extern XmTextPosition XmTextGetTopCharacter() ;
extern void XmTextSetTopCharacter() ;
extern XmTextPosition XmTextGetCursorPosition() ;
extern XmTextPosition XmTextGetInsertionPosition() ;
extern void XmTextSetInsertionPosition() ;
extern void XmTextSetCursorPosition() ;
extern Boolean XmTextRemove() ;
extern Boolean XmTextCopy() ;
extern Boolean XmTextCut() ;
extern Boolean XmTextPaste() ;
extern char * XmTextGetSelection() ;
extern wchar_t * XmTextGetSelectionWcs() ;
extern void XmTextSetSelection() ;
extern void XmTextClearSelection() ;
extern Boolean XmTextGetSelectionPosition() ;
extern XmTextPosition XmTextXYToPos() ;
extern Boolean XmTextPosToXY() ;
extern XmTextSource XmTextGetSource() ;
extern void XmTextSetSource() ;
extern void XmTextShowPosition() ;
extern void XmTextScroll() ;
extern int XmTextGetBaseline() ;
extern void XmTextDisableRedisplay() ;
extern void XmTextEnableRedisplay() ;
extern Boolean XmTextFindString() ;
extern Boolean XmTextFindStringWcs() ;

#else

extern void XmTextSetHighlight( 
                        Widget w,
                        XmTextPosition left,
                        XmTextPosition right,
                        XmHighlightMode mode) ;
extern Widget XmCreateScrolledText( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget XmCreateText( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern int XmTextGetSubstring( 
                        Widget widget,
                        XmTextPosition start,
                        int num_chars,
                        int buf_size,
                        char *buffer) ;
extern int XmTextGetSubstringWcs( 
                        Widget widget,
                        XmTextPosition start,
                        int num_chars,
                        int buf_size,
                        wchar_t *buffer) ;
extern char * XmTextGetString( 
                        Widget widget) ;
extern wchar_t * XmTextGetStringWcs( 
                        Widget widget) ;
extern XmTextPosition XmTextGetLastPosition( 
                        Widget widget) ;
extern void XmTextSetString( 
                        Widget widget,
                        char *value) ;
extern void XmTextSetStringWcs( 
                        Widget widget,
                        wchar_t *wc_value) ;
extern void XmTextReplace( 
                        Widget widget,
                        XmTextPosition frompos,
                        XmTextPosition topos,
                        char *value) ;
extern void XmTextReplaceWcs( 
                        Widget widget,
                        XmTextPosition frompos,
                        XmTextPosition topos,
                        wchar_t *value) ;
extern void XmTextInsert( 
                        Widget widget,
                        XmTextPosition position,
                        char *value) ;
extern void XmTextInsertWcs( 
                        Widget widget,
                        XmTextPosition position,
                        wchar_t *wc_value) ;
extern void XmTextSetAddMode( 
                        Widget widget,
#if NeedWidePrototypes
                        int state) ;
#else
                        Boolean state) ;
#endif /* NeedWidePrototypes */
extern Boolean XmTextGetAddMode( 
                        Widget widget) ;
extern Boolean XmTextGetEditable( 
                        Widget widget) ;
extern void XmTextSetEditable( 
                        Widget widget,
#if NeedWidePrototypes
                        int editable) ;
#else
                        Boolean editable) ;
#endif /* NeedWidePrototypes */
extern int XmTextGetMaxLength( 
                        Widget widget) ;
extern void XmTextSetMaxLength( 
                        Widget widget,
                        int max_length) ;
extern XmTextPosition XmTextGetTopCharacter( 
                        Widget widget) ;
extern void XmTextSetTopCharacter( 
                        Widget widget,
                        XmTextPosition top_character) ;
extern XmTextPosition XmTextGetCursorPosition( 
                        Widget widget) ;
extern XmTextPosition XmTextGetInsertionPosition( 
                        Widget widget) ;
extern void XmTextSetInsertionPosition( 
                        Widget widget,
                        XmTextPosition position) ;
extern void XmTextSetCursorPosition( 
                        Widget widget,
                        XmTextPosition position) ;
extern Boolean XmTextRemove( 
                        Widget widget) ;
extern Boolean XmTextCopy( 
                        Widget widget,
                        Time copy_time) ;
extern Boolean XmTextCut( 
                        Widget widget,
                        Time cut_time) ;
extern Boolean XmTextPaste( 
                        Widget widget) ;
extern char * XmTextGetSelection( 
                        Widget widget) ;
extern wchar_t * XmTextGetSelectionWcs( 
                        Widget widget) ;
extern void XmTextSetSelection( 
                        Widget widget,
                        XmTextPosition first,
                        XmTextPosition last,
                        Time set_time) ;
extern void XmTextClearSelection( 
                        Widget widget,
                        Time clear_time) ;
extern Boolean XmTextGetSelectionPosition( 
                        Widget widget,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
extern XmTextPosition XmTextXYToPos( 
                        Widget widget,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
extern Boolean XmTextPosToXY( 
                        Widget widget,
                        XmTextPosition position,
                        Position *x,
                        Position *y) ;
extern XmTextSource XmTextGetSource( 
                        Widget widget) ;
extern void XmTextSetSource( 
                        Widget widget,
                        XmTextSource source,
                        XmTextPosition top_character,
                        XmTextPosition cursor_position) ;
extern void XmTextShowPosition( 
                        Widget widget,
                        XmTextPosition position) ;
extern void XmTextScroll( 
                        Widget widget,
                        int n) ;
extern int XmTextGetBaseline( 
                        Widget widget) ;
extern void XmTextDisableRedisplay( 
                        Widget widget) ;
extern void XmTextEnableRedisplay( 
                        Widget widget) ;
extern Boolean XmTextFindString( 
                        Widget w,
                        XmTextPosition start,
                        char *search_string,
                        XmTextDirection direction,
                        XmTextPosition *position) ;
extern Boolean XmTextFindStringWcs( 
                        Widget w,
                        XmTextPosition start,
                        wchar_t *wc_string,
                        XmTextDirection direction,
                        XmTextPosition *position) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmText_h */
/* DON'T ADD STUFF AFTER THIS #endif */
