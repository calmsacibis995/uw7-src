#pragma ident	"@(#)m1.2libs:Xm/TextF.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmTextF_h
#define _XmTextF_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************
 * type defines   *
 ******************/
typedef struct _XmTextFieldClassRec *XmTextFieldWidgetClass;
typedef struct _XmTextFieldRec *XmTextFieldWidget;

/******************
 * extern class   *
 ******************/
externalref WidgetClass       xmTextFieldWidgetClass;


/************************
 * fast subclass define *
 ************************/
#ifndef XmIsTextField
#define XmIsTextField(w)     XtIsSubclass(w, xmTextFieldWidgetClass)
#endif /* XmIsTextField */


/********************
 * public functions *
 ********************/

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void ToggleCursorGC() ;
extern char * XmTextFieldGetString() ;
extern int XmTextFieldGetSubstring() ;
extern wchar_t * XmTextFieldGetStringWcs() ;
extern int XmTextFieldGetSubstringWcs() ;
extern XmTextPosition XmTextFieldGetLastPosition() ;
extern void XmTextFieldSetString() ;
extern void XmTextFieldSetStringWcs() ;
extern void XmTextFieldReplace() ;
extern void XmTextFieldReplaceWcs() ;
extern void XmTextFieldInsert() ;
extern void XmTextFieldInsertWcs() ;
extern void XmTextFieldSetAddMode() ;
extern Boolean XmTextFieldGetAddMode() ;
extern Boolean XmTextFieldGetEditable() ;
extern void XmTextFieldSetEditable() ;
extern int XmTextFieldGetMaxLength() ;
extern void XmTextFieldSetMaxLength() ;
extern XmTextPosition XmTextFieldGetCursorPosition() ;
extern XmTextPosition XmTextFieldGetInsertionPosition() ;
extern void XmTextFieldSetCursorPosition() ;
extern void XmTextFieldSetInsertionPosition() ;
extern Boolean XmTextFieldGetSelectionPosition() ;
extern char * XmTextFieldGetSelection() ;
extern wchar_t * XmTextFieldGetSelectionWcs() ;
extern Boolean XmTextFieldRemove() ;
extern Boolean XmTextFieldCopy() ;
extern Boolean XmTextFieldCut() ;
extern Boolean XmTextFieldPaste() ;
extern void XmTextFieldClearSelection() ;
extern void XmTextFieldSetSelection() ;
extern XmTextPosition XmTextFieldXYToPos() ;
extern Boolean XmTextFieldPosToXY() ;
extern void XmTextFieldShowPosition() ;
extern void XmTextFieldSetHighlight() ;
extern int XmTextFieldGetBaseline() ;
extern Widget XmCreateTextField() ;

#else

extern void ToggleCursorGC( 
                        Widget widget,
#if NeedWidePrototypes
                        int state) ;
#else
                        Boolean state) ;
#endif /* NeedWidePrototypes */
extern char * XmTextFieldGetString( 
                        Widget w) ;
extern int XmTextFieldGetSubstring( 
                        Widget widget,
                        XmTextPosition start,
                        int num_chars,
                        int buf_size,
                        char *buffer) ;
extern wchar_t * XmTextFieldGetStringWcs( 
                        Widget w) ;
extern int XmTextFieldGetSubstringWcs( 
                        Widget widget,
                        XmTextPosition start,
                        int num_chars,
                        int buf_size,
                        wchar_t *buffer) ;
extern XmTextPosition XmTextFieldGetLastPosition( 
                        Widget w) ;
extern void XmTextFieldSetString( 
                        Widget w,
                        char *value) ;
extern void XmTextFieldSetStringWcs( 
                        Widget w,
                        wchar_t *wc_value) ;
extern void XmTextFieldReplace( 
                        Widget w,
                        XmTextPosition from_pos,
                        XmTextPosition to_pos,
                        char *value) ;
extern void XmTextFieldReplaceWcs( 
                        Widget w,
                        XmTextPosition from_pos,
                        XmTextPosition to_pos,
                        wchar_t *wc_value) ;
extern void XmTextFieldInsert( 
                        Widget w,
                        XmTextPosition position,
                        char *value) ;
extern void XmTextFieldInsertWcs( 
                        Widget w,
                        XmTextPosition position,
                        wchar_t *wcstring) ;
extern void XmTextFieldSetAddMode( 
                        Widget w,
#if NeedWidePrototypes
                        int state) ;
#else
                        Boolean state) ;
#endif /* NeedWidePrototypes */
extern Boolean XmTextFieldGetAddMode( 
                        Widget w) ;
extern Boolean XmTextFieldGetEditable( 
                        Widget w) ;
extern void XmTextFieldSetEditable( 
                        Widget w,
#if NeedWidePrototypes
                        int editable) ;
#else
                        Boolean editable) ;
#endif /* NeedWidePrototypes */
extern int XmTextFieldGetMaxLength( 
                        Widget w) ;
extern void XmTextFieldSetMaxLength( 
                        Widget w,
                        int max_length) ;
extern XmTextPosition XmTextFieldGetCursorPosition( 
                        Widget w) ;
extern XmTextPosition XmTextFieldGetInsertionPosition( 
                        Widget w) ;
extern void XmTextFieldSetCursorPosition( 
                        Widget w,
                        XmTextPosition position) ;
extern void XmTextFieldSetInsertionPosition( 
                        Widget w,
                        XmTextPosition position) ;
extern Boolean XmTextFieldGetSelectionPosition( 
                        Widget w,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
extern char * XmTextFieldGetSelection( 
                        Widget w) ;
extern wchar_t * XmTextFieldGetSelectionWcs( 
                        Widget w) ;
extern Boolean XmTextFieldRemove( 
                        Widget w) ;
extern Boolean XmTextFieldCopy( 
                        Widget w,
                        Time clip_time) ;
extern Boolean XmTextFieldCut( 
                        Widget w,
                        Time clip_time) ;
extern Boolean XmTextFieldPaste( 
                        Widget w) ;
extern void XmTextFieldClearSelection( 
                        Widget w,
                        Time sel_time) ;
extern void XmTextFieldSetSelection( 
                        Widget w,
                        XmTextPosition first,
                        XmTextPosition last,
                        Time sel_time) ;
extern XmTextPosition XmTextFieldXYToPos( 
                        Widget w,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */
extern Boolean XmTextFieldPosToXY( 
                        Widget w,
                        XmTextPosition position,
                        Position *x,
                        Position *y) ;
extern void XmTextFieldShowPosition( 
                        Widget w,
                        XmTextPosition position) ;
extern void XmTextFieldSetHighlight( 
                        Widget w,
                        XmTextPosition left,
                        XmTextPosition right,
                        XmHighlightMode mode) ;
extern int XmTextFieldGetBaseline( 
                        Widget w) ;
extern Widget XmCreateTextField( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTextF_h */
/* DON'T ADD STUFF AFTER THIS #endif */
