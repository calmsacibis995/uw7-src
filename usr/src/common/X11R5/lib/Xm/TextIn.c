#pragma ident	"@(#)m1.2libs:Xm/TextIn.c	1.10"
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
/* Private definitions. */

#include <stdio.h>
#include <string.h>
#include <Xm/TextP.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/Vendor.h>
#include <X11/VendorP.h>
#ifdef HP_MOTIF
#include <X11/XHPlib.h>
#endif /* HP_MOTIF */
#include <Xm/AtomMgr.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropTrans.h>
#include <Xm/Display.h>
#include <Xm/ScreenP.h>
#include <Xm/ManagerP.h>
#include "MessagesI.h"
#include <Xm/TextStrSoP.h>
#include <Xm/TextSelP.h>
#include <Xm/DragIconP.h>
#include <Xm/TransltnsP.h>
#include <Xm/ScrollBarP.h>
#include <Xm/XmosP.h>
#ifdef SUN_MOTIF
#include <X11/keysym.h>
#include "_VirtKeysI.h"
#endif

/* 0-width text input support */
#if defined(SUPPORT_ZERO_WIDTH) && defined(HAS_WIDECHAR_FUNCTIONS)
#ifdef HP_MOTIF
#include <wchar.h>
#endif
#if defined(SUN_MOTIF) /* && defined(NOVELL_MOTIF) */
#include <widec.h>
#include <wctype.h>
#endif
#ifdef IBM_MOTIF
#include <wchar.h>
#endif
#endif /* SUPPORT_ZERO_WIDTH & HAS_WIDECHAR_FUNCTIONS */

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1		catgets(Xm_catd,MS_Text,MSG_T_5,_XmMsgTextIn_0000)
#define GRABKBDERROR	catgets(Xm_catd,MS_CButton,MSG_CB_6,\
				_XmMsgRowColText_0024)
#else
#define MSG1	_XmMsgTextIn_0000
#define GRABKBDERROR	_XmMsgRowColText_0024
#endif

#define TEXT_MAX_INSERT_SIZE 512

typedef struct {
    Boolean has_destination;
    XmTextPosition position;
    long replace_length;
    Boolean quick_key;
    XmTextWidget widget;
} TextDestDataRec, *TextDestData;

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static TextDestData GetTextDestData() ;
static void SetDropContext() ;
static void DeleteDropContext() ;
static Boolean NeedsPendingDeleteDisjoint() ;
static void CheckSync() ;
static void RingBell() ;
static Time GetServerTime() ;
static Boolean DeleteOrKill() ;
static void StuffFromBuffer() ;
static void UnKill() ;
static void RemoveCurrentSelection() ;
static void DeleteCurrentSelection() ;
static void KillCurrentSelection() ;
static void CheckDisjointSelection() ;
static void SelfInsert() ;
static void InsertString() ;
static void ProcessVerticalParams() ;
static void ProcessHorizontalParams() ;
static void ProcessSelectParams() ;
static void KeySelection() ;
static void SetAnchorBalancing() ;
static void SetNavigationAnchor() ;
static void CompleteNavigation() ;
static void SimpleMovement() ;
static void MoveForwardChar() ;
static void MoveBackwardChar() ;
static void MoveForwardWord() ;
static void MoveBackwardWord() ;
static void MoveForwardParagraph() ;
static void MoveBackwardParagraph() ;
static void MoveToLineStart() ;
static void MoveToLineEnd() ;
static void _MoveNextLine() ;
static void MoveNextLine() ;
static void _MovePreviousLine() ;
static void MovePreviousLine() ;
static void MoveNextPage() ;
static void MovePreviousPage() ;
static void MovePageLeft() ;
static void MovePageRight() ;
static void MoveBeginningOfFile() ;
static void MoveEndOfFile() ;
static void ScrollOneLineUp() ;
static void ScrollOneLineDown() ;
static void ScrollCursorVertically() ;
static void AddNewLine() ;
static void InsertNewLine() ;
static void InsertNewLineAndBackup() ;
static void InsertNewLineAndIndent() ;
static void RedrawDisplay() ;
static void Activate() ;
static void ToggleOverstrike() ;
static void ToggleAddMode() ;
static void SetCursorPosition() ;
static void RemoveBackwardChar() ;
static void DeleteBackwardChar() ;
static void KillBackwardChar() ;
static void RemoveForwardWord() ;
static void DeleteForwardWord() ;
static void KillForwardWord() ;
static void RemoveBackwardWord() ;
static void DeleteBackwardWord() ;
static void KillBackwardWord() ;
static void RemoveForwardChar() ;
static void KillForwardChar() ;
static void DeleteForwardChar() ;
static void RemoveToEndOfLine() ;
static void RemoveToStartOfLine() ;
static void DeleteToStartOfLine() ;
static void KillToStartOfLine() ;
static void DeleteToEndOfLine() ;
static void KillToEndOfLine() ;
static void RestorePrimaryHighlight() ;
static void SetSelectionHint() ;
static void a_Selection() ;
static void SetAnchor() ;
static void DoSelection() ;
static void SetScanType() ;
static void StartPrimary() ;
static void StartSecondary() ;
static void StartDrag() ;
static void ProcessBDrag() ;
static Boolean dragged() ;
static void DoExtendedSelection() ;
static void DoSecondaryExtend() ;
static void BrowseScroll() ;
static Boolean CheckTimerScrolling() ;
static void StartExtendSelection() ;
static void ExtendSelection() ;
static void ExtendSecondary() ;
static void ExtendEnd() ;
static void DoGrabFocus() ;
static void MoveDestination() ;
static void DoStuff() ;
static void Stuff() ;
static void HandleSelectionReplies() ;
static void SecondaryNotify() ;
static void HandleTargets() ;
static void VoidAction() ;
static void ExtendSecondaryEnd() ;
static void SelectAll() ;
static void DeselectAll() ;
static void ClearSelection() ;
static void ProcessBDragRelease() ;
static void ProcessCopy() ;
static void ProcessMove() ;
static void CopyPrimary() ;
static void CutPrimary() ;
static void CutClipboard() ;
static void CopyClipboard() ;
static void PasteClipboard() ;
static Boolean VerifyLeave() ;
static void TextLeave() ;
static void TextFocusIn() ;
static void TextFocusOut() ;
static void TraverseDown() ;
static void TraverseUp() ;
static void TraverseHome() ;
static void TraverseNextTabGroup() ;
static void TraversePrevTabGroup() ;
static void ProcessCancel() ;
static void ProcessReturn() ;
static void ProcessTab() ;
static void ProcessUp() ;
static void ProcessDown() ;
static void ProcessShiftUp() ;
static void ProcessShiftDown() ;
static void ProcessHome() ;
static void Invalidate() ;
static void InputGetValues() ;
static void InputSetValues() ;
static void InputDestroy() ;
static XtPointer InputBaseProc() ;
static void DropDestroyCB() ;
static void DropTransferCallback() ;
static void HandleDrop() ;
static void DragProcCallback() ;
static void DropProcCallback() ;
static void RegisterDropSite() ;
static XmTextPosition XtoPosInLine() ;
#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection() ;
static void ProcessPress() ;
#endif /* CDE_INTEGRATE */

#else

static TextDestData GetTextDestData( 
                        Widget tw) ;
static void SetDropContext( 
                        Widget w) ;
static void DeleteDropContext( 
                        Widget w) ;
static Boolean NeedsPendingDeleteDisjoint( 
                        XmTextWidget tw,
                        XmTextPosition *left,
                        XmTextPosition *right,
			int check_add_mode) ;
static void CheckSync( 
                        Widget w,
                        XtPointer tmp,
                        XEvent *event,
                        Boolean *cont) ;
static void RingBell( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Time GetServerTime( 
                        Widget w) ;
static Boolean DeleteOrKill( 
                        XmTextWidget tw,
                        XEvent *event,
                        XmTextPosition from,
                        XmTextPosition to,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void StuffFromBuffer( 
                        XmTextWidget tw,
                        XEvent *event,
                        int buffer) ;
static void UnKill( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveCurrentSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void DeleteCurrentSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillCurrentSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CheckDisjointSelection( 
                        Widget w,
                        XmTextPosition position,
                        Time sel_time) ;
static void SelfInsert( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void InsertString( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessVerticalParams( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessHorizontalParams( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
                        XmTextPosition *left,
                        XmTextPosition *right,
                        XmTextPosition *position) ;
static void ProcessSelectParams( 
                        Widget w,
                        XEvent *event,
                        XmTextPosition *left,
                        XmTextPosition *right,
                        XmTextPosition *position) ;
static void KeySelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SetAnchorBalancing( 
                        XmTextWidget tw,
                        XmTextPosition position) ;
static void SetNavigationAnchor( 
                        XmTextWidget tw,
                        XmTextPosition position,
                        Time time,
#if NeedWidePrototypes
                        int extend) ;
#else
                        Boolean extend) ;
#endif /* NeedWidePrototypes */
static void CompleteNavigation( 
                        XmTextWidget tw,
                        XmTextPosition position,
                        Time time,
#if NeedWidePrototypes
                        int extend) ;
#else
                        Boolean extend) ;
#endif /* NeedWidePrototypes */
static void SimpleMovement( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
                        XmTextScanDirection dir,
                        XmTextScanType type,
#if NeedWidePrototypes
                        int include) ;
#else
                        Boolean include) ;
#endif /* NeedWidePrototypes */
static void MoveForwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveBackwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveForwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveBackwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveForwardParagraph( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveBackwardParagraph( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveToLineStart( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveToLineEnd( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _MoveNextLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int pendingoff) ;
#else
                        Boolean pendingoff) ;
#endif /* NeedWidePrototypes */
static void MoveNextLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _MovePreviousLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int pendingoff) ;
#else
                        Boolean pendingoff) ;
#endif /* NeedWidePrototypes */
static void MovePreviousLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveNextPage( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MovePreviousPage( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MovePageLeft( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MovePageRight( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveBeginningOfFile( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveEndOfFile( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ScrollOneLineUp( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ScrollOneLineDown( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ScrollCursorVertically( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void AddNewLine( 
                        Widget w,
                        XEvent *event,
#if NeedWidePrototypes
                        int move_cursor) ;
#else
                        Boolean move_cursor) ;
#endif /* NeedWidePrototypes */
static void InsertNewLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void InsertNewLineAndBackup( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void InsertNewLineAndIndent( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RedrawDisplay( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Activate( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ToggleOverstrike( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ToggleAddMode( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SetCursorPosition( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveBackwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void DeleteBackwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillBackwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveForwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void DeleteForwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillForwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveBackwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void DeleteBackwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillBackwardWord( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveForwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void KillForwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DeleteForwardChar( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RemoveToEndOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void RemoveToStartOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params,
#if NeedWidePrototypes
                        int kill) ;
#else
                        Boolean kill) ;
#endif /* NeedWidePrototypes */
static void DeleteToStartOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillToStartOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DeleteToEndOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KillToEndOfLine( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void RestorePrimaryHighlight( 
                        InputData data,
                        XmTextPosition prim_left,
                        XmTextPosition prim_right) ;
static void SetSelectionHint( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void a_Selection( 
                        XmTextWidget tw,
#if NeedWidePrototypes
                        int x,
                        int y,
#else
                        Position x,
                        Position y,
#endif /* NeedWidePrototypes */
                        Time sel_time) ;
static void SetAnchor( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DoSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SetScanType( 
                        Widget w,
                        InputData data,
                        XEvent *event) ;
static void StartPrimary( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void StartSecondary( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void StartDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessBDrag( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static Boolean dragged( 
                        SelectionHint selectionHint,
                        XEvent *event,
                        int threshold) ;
static void DoExtendedSelection( 
                        Widget w,
                        Time ev_time) ;
static void DoSecondaryExtend( 
                        Widget w,
                        Time ev_time) ;
static void BrowseScroll( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static Boolean CheckTimerScrolling( 
                        Widget w,
                        XEvent *event) ;
static void StartExtendSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendSecondary( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendEnd( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DoGrabFocus( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MoveDestination( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DoStuff( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void Stuff( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void HandleSelectionReplies( 
                        Widget w,
                        XtPointer closure,
                        XEvent *ev,
                        Boolean *cont) ;
static void SecondaryNotify( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void HandleTargets( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void VoidAction( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendSecondaryEnd( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SelectAll( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DeselectAll( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ClearSelection( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessBDragRelease( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessCopy( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessMove( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CopyPrimary( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CutPrimary( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CutClipboard( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CopyClipboard( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void PasteClipboard( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Boolean VerifyLeave( 
                        Widget w,
                        XEvent *event) ;
static void TextLeave( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TextFocusIn( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TextFocusOut( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TraverseDown( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TraverseUp( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TraverseHome( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TraverseNextTabGroup( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TraversePrevTabGroup( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessCancel( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessReturn( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessTab( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessUp( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessDown( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessShiftUp( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessShiftDown( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessHome( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void Invalidate( 
                        XmTextWidget tw,
                        XmTextPosition position,
                        XmTextPosition topos,
                        long delta) ;
static void InputGetValues( 
                        Widget wid,
                        ArgList args,
                        Cardinal num_args) ;
static void InputSetValues( 
                        Widget oldw,
                        Widget reqw,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void InputDestroy( 
                        Widget w) ;
static XtPointer InputBaseProc( 
                        Widget widget,
                        XtPointer client_data) ;
static void DropDestroyCB( 
                        Widget w,
                        XtPointer clientData,
                        XtPointer callData) ;
static void DropTransferCallback( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void HandleDrop( 
                        Widget w,
                        XmDropProcCallbackStruct *cb) ;
static void DragProcCallback( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void DropProcCallback( 
                        Widget w,
                        XtPointer client,
                        XtPointer call) ;
static void RegisterDropSite( 
                        Widget w) ;
static XmTextPosition XtoPosInLine( 
                        XmTextWidget tw,
#if NeedWidePrototypes
                        int x,
#else
                        Position x,
#endif /* NeedWidePrototypes */
                        LineNum line) ;
#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection(
        XmTextWidget w,
	XEvent *event) ;
static void ProcessPress(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params) ;
#endif /* CDE_INTEGRATE */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/



static XContext _XmTextDestContext = 0;
static XContext _XmTextDNDContext = 0;

static XmTextScanType sarray[] = {
    XmSELECT_POSITION, XmSELECT_WORD, XmSELECT_LINE, XmSELECT_ALL
};

static int sarraysize = XtNumber(sarray);


static XtResource input_resources[] = {
    {
      XmNselectionArray, XmCSelectionArray, XmRPointer, sizeof(XtPointer),
      XtOffsetOf( struct _InputDataRec, sarray),
      XmRImmediate, (XtPointer) sarray
    },

    {
      XmNselectionArrayCount, XmCSelectionArrayCount, XmRInt, sizeof(int),
      XtOffsetOf( struct _InputDataRec, sarraycount),
      XmRInt, (XtPointer) &sarraysize
   },

    {
      XmNpendingDelete, XmCPendingDelete, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _InputDataRec, pendingdelete),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNselectThreshold, XmCSelectThreshold, XmRInt, sizeof(int),
      XtOffsetOf( struct _InputDataRec, threshold),
      XmRImmediate, (XtPointer) 5
    },

};

static TextDestData 
#ifdef _NO_PROTO
GetTextDestData( tw )
        Widget tw ;
#else
GetTextDestData(
        Widget tw )
#endif /* _NO_PROTO */
{
   static TextDestData dest_data;
   Display *display = XtDisplay(tw);
   Screen *screen = XtScreen(tw);

   if (_XmTextDestContext == 0)
      _XmTextDestContext = XUniqueContext();

   if (XFindContext(display, (Window)screen,
		    _XmTextDestContext, (char **) &dest_data)) {
       XmTextContextData ctx_data;
       Widget xm_display = (Widget) XmGetXmDisplay(display);

       ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

       ctx_data->screen = screen;
       ctx_data->context = _XmTextDestContext;
       ctx_data->type = _XM_IS_DEST_CTX;

       dest_data = (TextDestData) XtCalloc(1, sizeof(TextDestDataRec));

       XtAddCallback(xm_display, XmNdestroyCallback,
                     (XtCallbackProc) _XmTextFreeContextData,
		     (XtPointer) ctx_data);

       XSaveContext(display, (Window)screen,
		    _XmTextDestContext, (char *)dest_data);
   }
		
   return dest_data;
}

static void
#ifdef _NO_PROTO
SetDropContext( w )
        Widget w ;
#else
SetDropContext(
        Widget w )
#endif /* _NO_PROTO */
{
   Display *display = XtDisplay(w);
   Screen  *screen = XtScreen(w);

   if (_XmTextDNDContext == 0)
      _XmTextDNDContext = XUniqueContext();

   XSaveContext(display, (Window)screen,
                _XmTextDNDContext, (XPointer)w);
}


static void
#ifdef _NO_PROTO
DeleteDropContext( w )
        Widget w ;
#else
DeleteDropContext(
        Widget w )
#endif /* _NO_PROTO */
{
   Display *display = XtDisplay(w);
   Screen  *screen = XtScreen(w);

   XDeleteContext(display, (Window)screen, _XmTextDNDContext);
}


Widget
#ifdef _NO_PROTO
_XmTextGetDropReciever( w )
        Widget w ;
#else
_XmTextGetDropReciever(
        Widget w )
#endif /* _NO_PROTO */
{
   Display *display = XtDisplay(w);
   Screen  *screen = XtScreen(w);
   Widget widget;

   if (_XmTextDNDContext == 0) return NULL;

   if (!XFindContext(display, (Window)screen,
                     _XmTextDNDContext, (char **) &widget)) {
      return widget;
   }

   return NULL;
}


static Boolean 
#ifdef _NO_PROTO
NeedsPendingDeleteDisjoint( tw, left, right, check_add_mode )
        XmTextWidget tw ;
	XmTextPosition *left;
	XmTextPosition *right;
        int check_add_mode;
#else
NeedsPendingDeleteDisjoint(
        XmTextWidget tw,
	XmTextPosition *left,
	XmTextPosition *right,
	int check_add_mode )
#endif /* _NO_PROTO */
{
  InputData data = tw->text.input->data;

  if (!(*tw->text.source->GetSelection)(tw->text.source, left, right)) {
      *left = *right = tw->text.cursor_position;
      return False;
  } else 
    if (check_add_mode && !tw->text.add_mode)
      return (*left != *right);
    else
      return (data->pendingdelete && 
              *left != *right && *left <= tw->text.cursor_position &&
              *right >= tw->text.cursor_position);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CheckSync( w, tmp, event, cont )
        Widget w ;
        XtPointer tmp ;
        XEvent *event ;
        Boolean *cont ;
#else
CheckSync(
        Widget w,
        XtPointer tmp,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XEvent ev2;
    Boolean onewaiting;

    if (XPending(XtDisplay(tw))) {
	XPeekEvent(XtDisplay(tw), &ev2);
	onewaiting = (ev2.xany.type == KeyPress &&
		      ev2.xany.window == XtWindow(tw));
    } else onewaiting = FALSE;
    if (data->syncing) {
	if (!onewaiting) {
	    data->syncing = FALSE;
	    _XmTextEnableRedisplay(tw);
	}
    } else {
	if (onewaiting) {
	    data->syncing = TRUE;
	    _XmTextDisableRedisplay(tw, FALSE);
	}
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
RingBell( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
RingBell(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.verify_bell)
       XBell(XtDisplay(tw), 0);
}


static Time 
#ifdef _NO_PROTO
GetServerTime( w )
        Widget w ;
#else
GetServerTime(
        Widget w )
#endif /* _NO_PROTO */
{
  XEvent event;
  EventMask shellMask;

  while(!XtIsShell(w)) w = XtParent(w);

  shellMask =  XtBuildEventMask(w);
 
  if (!(shellMask & PropertyChangeMask))
     XSelectInput(XtDisplay(w), XtWindow(w), 
                  shellMask | PropertyChangeMask);

  XChangeProperty(XtDisplay(w), XtWindow(w), XA_WM_HINTS, XA_WM_HINTS,
	          32, PropModeAppend, (unsigned char *)NULL, 0);

  XWindowEvent(XtDisplay(w), XtWindow(w), PropertyChangeMask, &event);

  if (!(shellMask & PropertyChangeMask))
     XSelectInput(XtDisplay(w), XtWindow(w), shellMask);

  return (event.xproperty.time);
}

Boolean 
#ifdef _NO_PROTO
_XmTextHasDestination( w )
        Widget w ;
#else
_XmTextHasDestination(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    return (data->has_destination);
}


Boolean 
#ifdef _NO_PROTO
_XmTextSetDestinationSelection( w, position, disown, set_time )
        Widget w ;
        XmTextPosition position ;
        Boolean disown ;
        Time set_time ;
#else
_XmTextSetDestinationSelection(
        Widget w,
        XmTextPosition position,
#if NeedWidePrototypes
        int disown,
#else
        Boolean disown,
#endif /* NeedWidePrototypes */
        Time set_time )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    Boolean result = TRUE;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
			                "MOTIF_DESTINATION", False);

    if (!XtIsRealized(w)) return False;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);

    if (!disown) {
        if (!data->has_destination) {
            if (!set_time) set_time = GetServerTime(w);
            result = XtOwnSelection(w, MOTIF_DESTINATION, set_time,
			         _XmTextConvert, 
			         _XmTextLoseSelection,
			         (XtSelectionDoneProc) NULL);
	    data->dest_time = set_time;
	    data->has_destination = result;
	    if (result) _XmSetDestination(XtDisplay(w), w);
            _XmTextToggleCursorGC(w);
        }
        tw->text.dest_position = position;
    } else {
        if (data->has_destination) {
           if (!set_time) set_time = GetServerTime(w);
	   XtDisownSelection(w, MOTIF_DESTINATION, set_time);

          /* Call XmGetDestination(dpy) to get widget that last had
             destination cursor. */
           if (w == XmGetDestination(XtDisplay(w)))
             _XmSetDestination(XtDisplay(w), (Widget)NULL);

           data->has_destination = False;
           _XmTextToggleCursorGC(w);
        }
    }

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);

    return result;
}

static Boolean 
#ifdef _NO_PROTO
DeleteOrKill( tw, event, from, to, kill )
        XmTextWidget tw ;
        XEvent *event ;
        XmTextPosition from ;
        XmTextPosition to ;
        Boolean kill ;
#else
DeleteOrKill(
        XmTextWidget tw,
        XEvent *event,
        XmTextPosition from,
        XmTextPosition to,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextBlockRec block, newblock;
    XmTextPosition cursorPos;
    Boolean freeBlock;
    char *ptr;

    _XmTextDisableRedisplay(tw,False);
    if (kill && from < to) {
	ptr = _XmStringSourceGetString(tw, from, to, False);
        XRotateBuffers(XtDisplay(tw), 1);
	XStoreBuffer(XtDisplay(tw), ptr, strlen(ptr), 0);
	XtFree(ptr);
    }
    block.ptr = "";
    block.length = 0;
    block.format = XmFMT_8_BIT;

    if (_XmTextModifyVerify(tw, event, &from, &to,
                             &cursorPos, &block, &newblock, &freeBlock)) {
       if ((*tw->text.source->Replace)(tw, NULL, &from, 
				       &to, &newblock, False) != EditDone) {
	   _XmTextEnableRedisplay(tw);
	   RingBell((Widget)tw, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
	   if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
	   return FALSE;
       } else {
	   tw->text.needs_redisplay = tw->text.needs_refigure_lines = True;
	   _XmTextEnableRedisplay(tw);
	   _XmTextSetDestinationSelection((Widget)tw, tw->text.cursor_position,
					  False, event->xkey.time);
       }
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    } else {
       _XmTextEnableRedisplay(tw);
       RingBell((Widget)tw, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
       return FALSE;
    }
    return TRUE;
}

static void 
#ifdef _NO_PROTO
StuffFromBuffer( tw, event, buffer )
        XmTextWidget tw ;
        XEvent *event ;
        int buffer ;
#else
StuffFromBuffer(
        XmTextWidget tw,
        XEvent *event,
        int buffer )
#endif /* _NO_PROTO */
{
    XmTextPosition cursorPos;
    XmTextPosition from_pos, to_pos;
    XmTextBlockRec block, newblock;
    Boolean freeBlock;

    from_pos = to_pos = XmTextGetCursorPosition((Widget) tw);
    block.ptr = XFetchBuffer(XtDisplay(tw), &(block.length), buffer);
    block.format = XmFMT_8_BIT;
    if (_XmTextModifyVerify(tw, event, &from_pos, &to_pos,
                             &cursorPos, &block, &newblock, &freeBlock)) {
       if ((*tw->text.source->Replace)(tw, NULL, &from_pos, 
				      &to_pos, &newblock, False) != EditDone) {
	   RingBell((Widget)tw, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
       } else {
	  _XmTextSetCursorPosition((Widget)tw, cursorPos);
	  _XmTextSetDestinationSelection((Widget)tw, tw->text.cursor_position,
					 False, event->xkey.time);
	  _XmTextValueChanged(tw, event);
       }
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    } else {
       RingBell((Widget)tw, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
    }
    if (block.ptr) XtFree(block.ptr);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
UnKill( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
UnKill(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    StuffFromBuffer(tw, event, 0);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
RemoveCurrentSelection( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveCurrentSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos, left, right;

    if (!(*tw->text.source->GetSelection)
			(tw->text.source, &left, &right)) {
        XBell(XtDisplay(tw), 0);
    } else {
        if (left < right) {
	   cursorPos = XmTextGetCursorPosition(w);
           (*tw->text.source->SetSelection)(tw->text.source, cursorPos,
					         cursorPos, event->xkey.time);
	   if (DeleteOrKill(tw, event, left, right, kill)) {
	      if (cursorPos > left && cursorPos <= right) {
		  _XmTextSetCursorPosition(w, left);
                  _XmTextSetDestinationSelection(w, tw->text.cursor_position,
						 False, event->xkey.time);
              }
              _XmTextValueChanged(tw, event);
	   } else 
              (*tw->text.source->SetSelection)(tw->text.source, left,
					           right, event->xkey.time);
        }
    }
}

static void 
#ifdef _NO_PROTO
DeleteCurrentSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteCurrentSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    RemoveCurrentSelection(w, event, params, num_params, FALSE);
}

static void 
#ifdef _NO_PROTO
KillCurrentSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillCurrentSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    RemoveCurrentSelection(w, event, params, num_params, TRUE);
}

static void 
#ifdef _NO_PROTO
CheckDisjointSelection( w, position, sel_time )
        Widget w ;
        XmTextPosition position ;
        Time sel_time ;
#else
CheckDisjointSelection(
        Widget w,
        XmTextPosition position,
        Time sel_time )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, right;
    InputData data = tw->text.input->data;

    left = right = position;

    if (tw->text.add_mode ||
	((*tw->text.source->GetSelection)(tw->text.source, &left, &right) &&
	 left != right && position >= left && position <= right))
       tw->text.pendingoff = FALSE;
    else
       tw->text.pendingoff = TRUE;

    if (left == right) {
        _XmTextSetDestinationSelection(w, position, False, sel_time);
       data->anchor = position;
    } else {
       _XmTextSetDestinationSelection(w, position, False, sel_time);
       if (!tw->text.add_mode) data->anchor = position;
    }
}

static Boolean
#ifdef _NO_PROTO
PrintableString(tw, str, n)
     XmTextWidget tw;
     char* str;
     int n;
#else
PrintableString(XmTextWidget tw,
              char* str,
              int n)
#endif
{
  OutputData o_data = tw->text.output->data;
#ifdef SUPPORT_ZERO_WIDTH
  /* some locales (such as Thai) have characters that are
   * printable but non-spacing. These should be inserted,
   * even if they have zero width.
   */
  if (tw->text.char_size == 1) {
    int i;
    for (i = 0; i < n; i++) {
      if (!isprint((unsigned char)str[i])) {
      return False;
      }
    }
    return True;
  } else {
    /* tw->text.char_size > 1 */
#ifdef HAS_WIDECHAR_FUNCTIONS
    int i, csize;
    wchar_t wc;
    for (i = 0, csize = mblen(str, tw->text.char_size);
       i < n;
       i += csize, csize=mblen(&(str[i]), tw->text.char_size))
      {
      if (csize < 0)
        return False;
      if (mbtowc(&wc, &(str[i]), tw->text.char_size) <= 0)
        return False;
      if (!iswprint(wc)) {
        return False;
      }
      }
#else /* HAS_WIDECHAR_FUNCTIONS */
    /*
     * This will only check if any single-byte characters are non-
     * printable. Better than nothing...
     */
    int i, csize;
    for (i = 0, csize = mblen(str, tw->text.char_size);
       i < n;
       i += csize, csize=mblen(&(str[i]), tw->text.char_size))
      {
      if (csize < 0)
        return False;
      if (csize == 1 && !isprint((unsigned char)str[i])) {
        return False;
      }
      }
#endif /* HAS_WIDECHAR_FUNCTIONS */
    return True;
  }
#else  /* SUPPORT_ZERO_WIDTH */
  if (o_data->use_fontset) {
    return (XmbTextEscapement((XFontSet)o_data->font, str, n) != 0);
  } else {
    return (XTextWidth(o_data->font, str, n) != 0);
  }
#endif /* SUPPORT_ZERO_WIDTH */
}
static void 
#ifdef _NO_PROTO
SelfInsert( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SelfInsert(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    char str[TEXT_MAX_INSERT_SIZE];
    int n, i;
    XmTextPosition cursorPos, beginPos, nextPos, lastPos;
    XmTextPosition left, right;
    XmTextBlockRec block, newblock;
    OutputData o_data = tw->text.output->data;
    XFontStruct *font = o_data->font;
    Status status_return;
    Boolean pending_delete = False;
    Boolean freeBlock;
#ifdef SUN_MOTIF
    Display *dpy = event->xany.display;
    Modifiers mode_switch,num_lock;
    KeySym keysym_return;
#endif

 /* Determine what was pressed.
  */
    n = XmImMbLookupString(w, (XKeyEvent *) event, str, TEXT_MAX_INSERT_SIZE, 
			   (KeySym *) NULL, &status_return);

#ifdef SUN_MOTIF
    num_lock = _XmGetModifierBinding(dpy, NumLock);
    mode_switch = _XmGetModifierBinding(dpy, ModeSwitch);

    if((num_lock & event->xkey.state) &&
	!(~num_lock & ~LockMask & ~mode_switch &  event->xkey.state) &&
	_XmIsKPKey(dpy, event->xkey.keycode, &keysym_return)) {

		if(keysym_return != XK_KP_Enter) 
			n = _XmTranslateKPKeySym(keysym_return,
				 str, TEXT_MAX_INSERT_SIZE);	
    }
#endif

 /* If the user has input more data than we can handle, bail out */
    if (status_return == XBufferOverflow || n > TEXT_MAX_INSERT_SIZE)
	return;

 /* *LookupString in some cases can return the NULL as a character, such
  * as when the user types <Ctrl><back_quote> or <Ctrl><@>.  Text widget
  * can't handle the NULL as a character, so we dump it here.
  */

    for (i=0; i < n; i++)
       if (str[i] == 0) n = 0; /* just toss out the entire input string */

    if (n > 0) {
	(*tw->text.output->DrawInsertionPoint)(tw,
					       tw->text.cursor_position, off);
        str[n]='\0';

        if (!PrintableString(tw, str, n) && strchr(str, '\t') == NULL) {
           (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
on);
           return;
        }

        beginPos = nextPos = XmTextGetCursorPosition(w);
	if (NeedsPendingDeleteDisjoint(tw, &left, &right, FALSE)) {
	   beginPos = left;
	   nextPos = right;
           pending_delete = True;
        } else if (data->overstrike) {
	  nextPos += _XmTextCountCharacters(str, n);
	  lastPos = (*(tw->text.source->Scan))(tw->text.source,
					       beginPos, XmSELECT_LINE,
					       XmsdRight, 1, TRUE);
	  if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) {
	    if (nextPos > lastPos) nextPos = lastPos;
	  } else
	    if (nextPos >= lastPos) 
	      if (lastPos < tw->text.source->data->length)
		nextPos = lastPos-1;
	      else
		nextPos = lastPos;
	}
	block.ptr = str;
	block.length = n;
	block.format = XmFMT_8_BIT;
        if (_XmTextModifyVerify(tw, event, &beginPos, &nextPos,
                                &cursorPos, &block, &newblock, &freeBlock)) {
           if (pending_delete)
              (*tw->text.source->SetSelection)(tw->text.source, cursorPos,
					         cursorPos, event->xkey.time);
	   if ((*tw->text.source->Replace)(tw, NULL, &beginPos, &nextPos,
					   &newblock, False) != EditDone) {
	      RingBell(w, event, params, num_params);
	   } else {
	     _XmTextSetCursorPosition(w, cursorPos);
	     CheckDisjointSelection(w, tw->text.cursor_position,
				    event->xkey.time);
	     _XmTextValueChanged(tw, event);
	   }
           if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
        } else {
	   RingBell(w, event, params, num_params);
        }
	(*tw->text.output->DrawInsertionPoint)(tw,
					       tw->text.cursor_position, on);
    }
}

static void 
#ifdef _NO_PROTO
InsertString( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
InsertString(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos, newCursorPos, beginPos, nextPos, left, right;
    char *str;
    int i;
    XmTextBlockRec block, newblock;
    Boolean value_changed = False;
    Boolean pending_delete = False;
    Boolean freeBlock;

    _XmTextDisableRedisplay(tw, TRUE);
    cursorPos = beginPos = nextPos = XmTextGetCursorPosition(w);

    if (NeedsPendingDeleteDisjoint(tw, &left, &right, FALSE)) {
       beginPos = left;
       nextPos = right;
       pending_delete = True;
    }


    for (i=0 ; i<*num_params ; i++) {
	str = params[i];
	block.ptr = str;
	block.length = strlen(str);
	block.format = XmFMT_8_BIT;
        if (_XmTextModifyVerify(tw, event, &beginPos, &nextPos,
                                &newCursorPos, &block, &newblock, &freeBlock)) {
           if (pending_delete) {
              (*tw->text.source->SetSelection)(tw->text.source, cursorPos,
				               cursorPos, event->xkey.time);
              pending_delete = False;
           }
	   if ((*tw->text.source->Replace)(tw, NULL, &beginPos, &nextPos,
					   &newblock, False) != EditDone) {
	       RingBell(w, event, params, num_params);
               if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
	       break;
	   } else {
               if (freeBlock && newblock.ptr) {
		  XtFree(newblock.ptr);
		  newblock.ptr = NULL;
               }
               cursorPos = newCursorPos;
	       value_changed = True;
	   }
        } else {
	   RingBell(w, event, params, num_params);
	   break;
        }
    }

    if (value_changed) {
       _XmTextSetCursorPosition(w, cursorPos);
       CheckDisjointSelection(w, tw->text.cursor_position, event->xkey.time);
       _XmTextValueChanged(tw, event);
    }

    _XmTextEnableRedisplay(tw);

}

static void 
#ifdef _NO_PROTO
ProcessVerticalParams( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessVerticalParams(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    char *dir;
    Cardinal num;

    if (*num_params > 0 && !strcmp(*params, "up")) {
       dir = "extend";
       num = 1;
       _MovePreviousLine(w, event, &dir, &num, False);
    } else if (*num_params > 0 && !strcmp(*params, "down")) {
        dir = "extend";
        num = 1;
        _MoveNextLine(w, event, &dir, &num, False);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessHorizontalParams( w, event, params, num_params, left,
			 right, position )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        XmTextPosition *left ;
        XmTextPosition *right ;
        XmTextPosition *position ;
#else
ProcessHorizontalParams(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
        XmTextPosition *left,
        XmTextPosition *right,
        XmTextPosition *position )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition old_cursorPos = XmTextGetCursorPosition(w);

    *position = (*tw->text.source->Scan)(tw->text.source,
				        XmTextGetCursorPosition(w),
			 	        XmSELECT_POSITION, XmsdRight, 1, False);

    if (!(*tw->text.source->GetSelection)
		(tw->text.source, left, right) || *left == *right) {
        data->origLeft = data->origRight = data->anchor;
        *left = *right = old_cursorPos;
    }

    /* move text cursor in direction of cursor key */
    if (*num_params > 0 && !strcmp(*params,"right")) {
       (*position)++;
    } else if (*num_params > 0 && !strcmp(*params,"left"))
       (*position)--;
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessSelectParams( w, event, left, right, position )
        Widget w ;
        XEvent *event ;
        XmTextPosition *left ;
        XmTextPosition *right ;
        XmTextPosition *position ;
#else
ProcessSelectParams(
        Widget w,
        XEvent *event,
        XmTextPosition *left,
        XmTextPosition *right,
        XmTextPosition *position )
#endif /* _NO_PROTO */
{
  XmTextWidget tw = (XmTextWidget) w;
  InputData data = tw->text.input->data;

  if (!((*tw->text.source->GetSelection)(tw->text.source, left, right))
      || *left == *right) {
      if (*position > data->anchor) {
        *left = data->anchor;
        *right = *position;
      } else {
        *left = *position;
        *right = data->anchor;
      }
  }
}

static void 
#ifdef _NO_PROTO
KeySelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KeySelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextPosition position, left, right, cursorPos;
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextScanDirection  cursorDir;                              /* PIR1858 */
    XmTextPosition       tempIndex;                              /* PIR1858 */


    /* reset origLeft and origRight */
    (*tw->text.source->GetSelection)(tw->text.source,
                                    &(data->origLeft), &(data->origRight));

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && (!strcmp(*params, "right") || 
				   !strcmp(*params, "left"))) {
       SetAnchorBalancing(tw, cursorPos);
    }

    data->selectionHint.x = data->selectionHint.y = 0;
    data->extending = TRUE;
  
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, FALSE);

    if (*num_params == 0) {
       position = cursorPos;
       ProcessSelectParams(w, event, &left, &right, &position);
    } else if (*num_params > 0 && 
	       (!strcmp(*params, "up") || !strcmp(*params, "down"))) {
       ProcessVerticalParams(w, event, params, num_params);
       _XmTextEnableRedisplay(tw);
       data->extending = FALSE;
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
       return;
    } else if (*num_params > 0 && (!strcmp(*params, "right") || 
				   !strcmp(*params, "left"))) {
       ProcessHorizontalParams(w, event, params, num_params, &left,
			       &right, &position);
    }
    cursorPos = position;

    if (position < 0 || position > tw->text.last_position) {
       _XmTextEnableRedisplay(tw);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
       return;
    }

    if (cursorPos < data->anchor)
       data->extendDir = XmsdLeft;
    else if (cursorPos > data->anchor)
       data->extendDir = XmsdRight;

    cursorDir = data->extendDir;   /* PIR1858 */
    if (data->extendDir == XmsdRight) {
       if ( cursorPos < right )  /* PIR1858:  We are backtracking  */
          cursorDir = XmsdLeft;  /* PIR1858 */
       right = cursorPos = (*tw->text.source->Scan)(tw->text.source,
				        position, data->stype, cursorDir,
					1, FALSE);
       left = data->anchor;
    } else {
       if ( cursorPos > left )  /* PIR1858: We are backtracking */
	  cursorDir = XmsdRight;/* PIR1858 */
       left = cursorPos = (*tw->text.source->Scan)(tw->text.source,
				        position, data->stype, cursorDir,
					1, FALSE);
       right = data->anchor;
    }

    if (left > right) {  /* PIR1858: We are on other side of anchor */
       tempIndex = left;
       left = right;
       right = tempIndex;
    }  /* end PIR1858 */

    (*tw->text.source->SetSelection)(tw->text.source, left, right,
				    event->xkey.time);
    tw->text.pendingoff = FALSE;
    _XmTextSetCursorPosition(w, cursorPos);
    _XmTextSetDestinationSelection(w, tw->text.cursor_position,
				   False, event->xkey.time);

    if (tw->text.auto_show_cursor_position &&
        cursorPos == tw->text.bottom_position)
        (*tw->text.output->MakePositionVisible)( tw, cursorPos);

    _XmTextEnableRedisplay(tw);

    /* reset origLeft and origRight */
    (*tw->text.source->GetSelection)(tw->text.source,
                                    &(data->origLeft), &(data->origRight));
    data->extending = FALSE;
    (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
}


static void
#ifdef _NO_PROTO
SetAnchorBalancing(tw, position)
XmTextWidget tw;
XmTextPosition position;
#else
SetAnchorBalancing(
	XmTextWidget tw,
	XmTextPosition position)
#endif /* _NO_PROTO */
{
    InputData data = tw->text.input->data;
    XmTextPosition left, right;
    float bal_point;

    if (!((*tw->text.source->GetSelection)(tw->text.source, &left, &right))
         || left == right) {
          data->anchor = position;
    } else {
          bal_point = (float)(((float)(right - left) / 2.0) + (float)left);

         /* shift anchor and direction to opposite end of the selection */
          if ((float)position < bal_point) {
             data->extendDir = XmsdLeft;
             data->anchor = data->origRight;
          } else if ((float)position > bal_point) {
             data->extendDir = XmsdRight;
             data->anchor = data->origLeft;
          }
    }
}

static void
#ifdef _NO_PROTO
SetNavigationAnchor(tw, position, time, extend)
XmTextWidget tw;
XmTextPosition position;
Time time;
Boolean extend;
#else
SetNavigationAnchor(
	XmTextWidget tw,
	XmTextPosition position,
        Time time,
#if NeedWidePrototypes
        int extend )
#else
        Boolean extend )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextPosition left, right;
    InputData data = tw->text.input->data;

    if (!tw->text.add_mode) {
       if (extend) {
          SetAnchorBalancing(tw, position);
       } else {
         if (((*tw->text.source->GetSelection)(tw->text.source, &left, &right))
             && left != right) {
            (*tw->text.source->SetSelection)(tw->text.source, position,
					     position, time);
            data->anchor = position;
         }
       }
    } else if (extend) {
       SetAnchorBalancing(tw, position);
    }
}

static void
#ifdef _NO_PROTO
CompleteNavigation(tw, position, time, extend)
XmTextWidget tw;
XmTextPosition position;
Time time;
Boolean extend;
#else
CompleteNavigation(
	XmTextWidget tw,
	XmTextPosition position,
        Time time,
#if NeedWidePrototypes
        int extend )
#else
        Boolean extend )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextPosition left, right;
    InputData data = tw->text.input->data;

    if ((tw->text.add_mode && (*tw->text.source->GetSelection)
         (tw->text.source, &left, &right) &&
         position >= left && position <= right) || extend)
       tw->text.pendingoff = FALSE;
    else
       tw->text.pendingoff = TRUE;

    if (extend) {
       if (data->anchor > position) {
          left = position;
          right = data->anchor;
       } else {
	  left = data->anchor;
          right = position;
       }
       (*tw->text.source->SetSelection)(tw->text.source, left, right, time);

       data->origLeft = left;
       data->origRight = right;
    }
    _XmTextSetCursorPosition((Widget)tw, position);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SimpleMovement( w, event, params, num_params, dir, type, include )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        XmTextScanDirection dir ;
        XmTextScanType type ;
        Boolean include ;
#else
SimpleMovement(
        Widget w,
        XEvent *event,
        String *params ,
        Cardinal *num_params ,
        XmTextScanDirection dir,
        XmTextScanType type,
#if NeedWidePrototypes
        int include )
#else
        Boolean include )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextPosition cursorPos;
    XmTextWidget tw = (XmTextWidget) w;
    Boolean extend = False;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);
    cursorPos = (*tw->text.source->Scan)(tw->text.source, cursorPos,
                                            type, dir, 1, include);


    CompleteNavigation(tw, cursorPos, event->xkey.time, extend);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}    


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveForwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveForwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    SimpleMovement(w, event, params, num_params,
		   XmsdRight, XmSELECT_POSITION, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveBackwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveBackwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    SimpleMovement(w, event, params, num_params,
		   XmsdLeft, XmSELECT_POSITION, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveForwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveForwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition position, cursorPos;
    Boolean extend = False;

    cursorPos = XmTextGetCursorPosition(w);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

/* Add if We only want to select to the end of the word w/o spaces
    if (*num_params > 0 && !strcmp(*params, "extend")) {
       position = (*tw->text.source->Scan)(tw->text.source, cursorPos,
                                        XmSELECT_WORD, XmsdRight, 1, FALSE);
       if(position == cursorPos){
           position = (*tw->text.source->Scan)(tw->text.source, position,
                                           XmSELECT_WORD, XmsdRight, 1, TRUE);
           position = (*tw->text.source->Scan)(tw->text.source, position,
                                           XmSELECT_WORD, XmsdRight, 1, FALSE);
       }
    } else
*/
    position = (*tw->text.source->Scan)(tw->text.source, cursorPos,
                                        XmSELECT_WORD, XmsdRight, 1, TRUE);

    CompleteNavigation(tw, position, event->xkey.time, extend);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveBackwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveBackwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition position, cursorPos;
    Boolean extend = False;

    cursorPos = XmTextGetCursorPosition(w);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    position = (*tw->text.source->Scan)(tw->text.source, cursorPos,
                                        XmSELECT_WORD, XmsdLeft, 1, FALSE);
    if(position == cursorPos){
        position = (*tw->text.source->Scan)(tw->text.source, position,
                                        XmSELECT_WORD, XmsdLeft, 1, TRUE);
        position = (*tw->text.source->Scan)(tw->text.source, position,
                                        XmSELECT_WORD, XmsdLeft, 1, FALSE);
    }

    CompleteNavigation(tw, position, event->xkey.time, extend);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveForwardParagraph( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveForwardParagraph(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   SimpleMovement(w, event, params, num_params,
		  XmsdRight, XmSELECT_PARAGRAPH, FALSE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveBackwardParagraph( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveBackwardParagraph(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   SimpleMovement(w, event, params, num_params,
		  XmsdLeft, XmSELECT_PARAGRAPH, FALSE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveToLineStart( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveToLineStart(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    LineNum line;
    XmTextPosition position, cursorPos;
    Boolean extend = False;

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    XmTextShowPosition(w, cursorPos);
    line = _XmTextPosToLine(tw, cursorPos);
    if (line == NOLINE) {
       XBell(XtDisplay(tw), 0);
    } else {
	_XmTextLineInfo(tw, line, &position, (LineTableExtra *) NULL);
        CompleteNavigation(tw, position, event->xkey.time, extend);
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveToLineEnd( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveToLineEnd(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    LineNum line;
    XmTextPosition position, cursorPos;
    Boolean extend = False;

    cursorPos = XmTextGetCursorPosition(w);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    XmTextShowPosition(w, cursorPos);
    line = _XmTextPosToLine(tw, cursorPos);
    if (line == NOLINE) {
       XBell(XtDisplay(tw), 0);
    } else {
	_XmTextLineInfo(tw, line+1, &position, (LineTableExtra *) NULL);
	if (position == PASTENDPOS)
	  position = (*tw->text.source->Scan)(tw->text.source, position,
					      XmSELECT_ALL, XmsdRight, 1, TRUE);
	else
	  position = (*tw->text.source->Scan)(tw->text.source, position,
					       XmSELECT_POSITION, XmsdLeft,
					       1, TRUE);
        CompleteNavigation(tw, position, event->xkey.time, extend);
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_MoveNextLine( w, event, params, num_params, pendingoff )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean pendingoff ;
#else
_MoveNextLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int pendingoff )
#else
        Boolean pendingoff )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    LineNum line;
    XmTextPosition currentPos, newPos, start, start2;
    XmTextPosition next;
#ifdef NOT_DEF
    int col;
#endif /* NOT_DEF */
    Position savePosX = tw->text.cursor_position_x;
    Boolean extend = False;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;

    currentPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, currentPos, event->xkey.time, extend);

    XmTextShowPosition(w, currentPos);
    line = _XmTextPosToLine(tw, currentPos);
    if (line == NOLINE) {
       XBell(XtDisplay(tw), 0);
       return;
    }

#ifdef NOT_DEF
    if ((int)tw->text.char_size > 1) {
#endif /* NOT_DEF */
       _XmTextLineInfo(tw, line+1, &start, (LineTableExtra *) NULL);
       if (start == PASTENDPOS) {
          newPos = (*tw->text.source->Scan)(tw->text.source, currentPos, 
					    XmSELECT_ALL, XmsdRight, 1, TRUE);
          CompleteNavigation(tw, newPos, event->xkey.time, extend);
          tw->text.pendingoff = pendingoff;
       } else {
         /* If 16-bit characters are offset by half-width, need to handle it:
          * AABBCC...   Now move cursor down from beginning of CC
          * cDDEEFF...  Should be at beginning of either EE or FF, down again
          * GGHHII...   Should now be at beginning of II */
         /* Force Scroll to next line so that XtoPos..  won't fail */
          if (line == tw->text.number_lines - 1) {
             XmTextShowPosition(w, start);
	     /* This may cause a multi-line scroll.  We better reset line */
	     line = _XmTextPosToLine(tw, start);
	     newPos = XtoPosInLine(tw, savePosX, line);
          } else
             newPos = XtoPosInLine(tw, savePosX, line+1);
          next = (*tw->text.source->Scan)(tw->text.source, newPos, 
					  XmSELECT_LINE, XmsdRight, 1, FALSE);

          CompleteNavigation(tw, newPos, event->xkey.time, extend);

          if ( tw->text.cursor_position != next )
             tw->text.cursor_position_x = savePosX;
       }
#ifdef NOT_DEF
    } else {
	/* JOE: we should discuss why the logic is different here */
        _XmTextLineInfo(tw, line, &start, (LineTableExtra *) NULL);
        col = currentPos - start;       /* %%% needs reviewing */
        _XmTextLineInfo(tw, line+1, &start, (LineTableExtra *) NULL);
        if (start == PASTENDPOS)
          newPos = (*tw->text.source->Scan)(tw->text.source, currentPos,
                                             XmSELECT_ALL, XmsdRight, 1, TRUE);
        else
          newPos = (*tw->text.source->Scan)(tw->text.source, start,
                                             XmSELECT_POSITION, XmsdRight,
                                             col, TRUE);

        CompleteNavigation(tw, newPos, event->xkey.time, extend);
    }
#endif /* NOT_DEF */

    XmTextShowPosition(w, tw->text.cursor_position);
    line = _XmTextPosToLine(tw, tw->text.cursor_position);
    if (line != NOLINE) {
       _XmTextLineInfo(tw, line, &start2, (LineTableExtra *) NULL);
       if (start2 != start && start != PASTENDPOS) {
	  newPos = (*tw->text.source->Scan)(tw->text.source, start,
                                            XmSELECT_LINE, XmsdRight,
					    1, FALSE);
          CompleteNavigation(tw, newPos, event->xkey.time, extend);
       }
    }
}

static void 
#ifdef _NO_PROTO
MoveNextLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveNextLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    _MoveNextLine(w, event, params, num_params, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_MovePreviousLine( w, event, params, num_params, pendingoff )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean pendingoff ;
#else
_MovePreviousLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int pendingoff )
#else
        Boolean pendingoff )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    LineNum line;
    XmTextPosition top, currentPos, newPos, origstart, start, start2;
    XmTextPosition	next;
    Boolean changed = False;
    Position savePosX = tw->text.cursor_position_x;
    Boolean extend = False;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;

    top = XmTextGetTopCharacter(w);

    currentPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, currentPos, event->xkey.time, extend);

    XmTextShowPosition(w, currentPos);
    line = _XmTextPosToLine(tw, currentPos);
    if (line == NOLINE) {
	_XmWarning(w, MSG1);
	newPos = currentPos;
    } else {
	_XmTextLineInfo(tw, line, &origstart, (LineTableExtra *) NULL);
	if (line == 0) {
	    XmTextScroll(w, -1);
	    line = _XmTextPosToLine(tw, currentPos);
	    if (line == 0) {
		newPos = (*tw->text.source->Scan)(tw->text.source,
				 currentPos, XmSELECT_ALL, XmsdLeft, 1, TRUE);
		changed = True;
		goto done;
	    }
	    if (line == NOLINE) line = 1;
	}
	_XmTextLineInfo(tw, line-1, &start, (LineTableExtra *) NULL);
	/* If 16-bit characters are offset by half-width, need to handle it:
	 * AABBCC...   Now move cursor up from beginning of II
	 * cDDEEFF...  Should be at beginning of either EE or FF; up again
	 * GGHHII...   Should now be at beginning of CC */
	newPos = XtoPosInLine(tw, tw->text.cursor_position_x, line-1);
	next = (*tw->text.source->Scan)(tw->text.source, newPos, XmSELECT_LINE,
					XmsdRight, 1, FALSE);
	if ( newPos == next ) changed = True;
	XmTextShowPosition(w, newPos);
	line = _XmTextPosToLine(tw, newPos);
	if (line != NOLINE) {
	    _XmTextLineInfo(tw, line, &start2, (LineTableExtra *) NULL);
	    if (start2 != start) {
		newPos = (*tw->text.source->Scan)(tw->text.source,
			 origstart, XmSELECT_POSITION, XmsdLeft, 1, TRUE);
	    }
	}
    }
  done:
    CompleteNavigation(tw, newPos, event->xkey.time, extend);
    if (!changed)
       tw->text.cursor_position_x = savePosX;
}

static void 
#ifdef _NO_PROTO
MovePreviousLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MovePreviousLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    _MovePreviousLine(w, event, params, num_params, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveNextPage( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveNextPage(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos;
    Position x, y;
    int n;
    Boolean extend = False;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, FALSE);

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &x, &y);

    n = _XmTextNumLines(tw);
    if (n > 1) n--;

    XmTextScroll(w, n);

   /* When y = 0, improper scrolling results.  This makes
    * sure no extra scroll results.
    */
    if (y <= 0) {
       OutputData o_data = tw->text.output->data;
       y += o_data->topmargin;
    }

    cursorPos = (*tw->text.output->XYToPos)(tw, x, y);

    CompleteNavigation(tw, cursorPos, event->xkey.time, extend);

    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MovePreviousPage( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MovePreviousPage(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos;
    Position x, y;
    int n;
    Boolean extend = False;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) return;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, FALSE);

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &x, &y);

    n = _XmTextNumLines(tw);
    if (n > 1) n--;

    XmTextScroll(w, -n);

    cursorPos = (*tw->text.output->XYToPos)(tw, x, y);

    CompleteNavigation(tw, cursorPos, event->xkey.time, extend);

    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MovePageLeft( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MovePageLeft(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos;
    Position x, y;
    Boolean extend = False;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, FALSE);

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &x, &y);
    _XmTextChangeHOffset(tw, -tw->text.inner_widget->core.width);
    cursorPos = (*tw->text.output->XYToPos)(tw, x, y);

    CompleteNavigation(tw, cursorPos, event->xkey.time, extend);

    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MovePageRight( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MovePageRight(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos;
    Position x, y;
    Boolean extend = False;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, FALSE);

    cursorPos = XmTextGetCursorPosition(w);

    if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

    SetNavigationAnchor(tw, cursorPos, event->xkey.time, extend);

    (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &x, &y);
    _XmTextChangeHOffset(tw, tw->text.inner_widget->core.width);
    cursorPos = (*tw->text.output->XYToPos)(tw, x, y);

    CompleteNavigation(tw, cursorPos, event->xkey.time, extend);

    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveBeginningOfFile( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveBeginningOfFile(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    SimpleMovement(w, event, params, num_params, XmsdLeft, XmSELECT_ALL, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveEndOfFile( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveEndOfFile(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    SimpleMovement(w, event, params, num_params, XmsdRight, XmSELECT_ALL, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}
    



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ScrollOneLineUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ScrollOneLineUp(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    XmTextScroll(w, 1);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ScrollOneLineDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ScrollOneLineDown(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    XmTextScroll(w, -1);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ScrollCursorVertically( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ScrollCursorVertically(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextWidget tw = (XmTextWidget) w;
  XmTextPosition pos;
  LineNum desired_line, cur_line;
  int percentage;
  OutputData data = tw->text.output->data;
  
  if (*num_params == 0) {
    pos = (*tw->text.output->XYToPos)(tw, event->xbutton.x, 
				      event->xbutton.y);
    if (pos == tw->text.line[tw->text.number_lines].start)
      desired_line = tw->text.number_lines-1;
    else
      for(desired_line=0; desired_line<tw->text.number_lines-1; desired_line++)
	if (tw->text.line[desired_line+1].start > pos) break;
  } else {
    tw->text.top_character = 0;
    tw->text.bottom_position = tw->text.last_position;
    sscanf(*params, "%d", &percentage);
    desired_line = ((data->number_lines - 1) * percentage) /100;
  }
  if (tw->text.cursor_position == tw->text.line[tw->text.number_lines].start)
    cur_line = tw->text.number_lines;
  else
    for (cur_line=0; cur_line<tw->text.number_lines; cur_line++)
      if (tw->text.line[cur_line+1].start > tw->text.cursor_position) break;

  XmTextScroll(w, (int)(cur_line - desired_line));
}

static void 
#ifdef _NO_PROTO
AddNewLine( w, event, move_cursor )
        Widget w ;
        XEvent *event ;
	Boolean move_cursor ;
#else
AddNewLine(
        Widget w,
        XEvent *event,
#if NeedWidePrototypes
	int move_cursor )
#else
	Boolean move_cursor )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos, beginPos, nextPos, left, right;
    XmTextBlockRec block, newblock;
    Boolean pending_delete = False;
    Boolean freeBlock;
    char str[32];

    str[0] = '\n';
    str[1] = 0;
    block.length = 1;
    block.ptr = str;
    block.format = XmFMT_8_BIT;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    beginPos = nextPos = XmTextGetCursorPosition(w);
    if (NeedsPendingDeleteDisjoint(tw, &left, &right, FALSE)) {
       beginPos = left;
       nextPos = right;
       pending_delete = True;
    }
    if (_XmTextModifyVerify(tw, event, &beginPos, &nextPos,
                                &cursorPos, &block, &newblock, &freeBlock)) {
        if (pending_delete) {
           (*tw->text.source->SetSelection)(tw->text.source, cursorPos,
				            cursorPos, event->xkey.time);
        }
        if ((*tw->text.source->Replace)(tw, NULL, &beginPos, &nextPos,
					&newblock, False) != EditDone) {
           if (tw->text.verify_bell) XBell(XtDisplay(tw), 0);
        } else {
	   if (move_cursor) {
	      _XmTextSetCursorPosition(w, cursorPos);
           } else {
	      _XmTextSetCursorPosition(w, beginPos);
           }
	   CheckDisjointSelection(w, tw->text.cursor_position,
				  event->xkey.time);
	   _XmTextValueChanged(tw, event);
       }
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    } else {
       if (tw->text.verify_bell) XBell(XtDisplay(tw), 0);
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InsertNewLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
InsertNewLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    AddNewLine(w, event, True);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}    


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InsertNewLineAndBackup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
InsertNewLineAndBackup(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    AddNewLine(w, event, False);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
InsertNewLineAndIndent( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
InsertNewLineAndIndent(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextBlockRec block, newblock;
    XmTextPosition  pos, from_pos, to_pos, left, right, cursorPos, newCursorPos;
    Boolean freeBlock, value_changed = False;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, TRUE);
    pos = XmTextGetCursorPosition(w);
    left = (*tw->text.source->Scan)(tw->text.source, pos,
 	    XmSELECT_LINE, XmsdLeft,  1, FALSE);
    if(left != (*tw->text.source->Scan)(tw->text.source, left,
                                    XmSELECT_WHITESPACE, XmsdRight, 1, FALSE)){
        AddNewLine(w, event, True);
    } else {
        right = (*tw->text.source->Scan)(tw->text.source, left,
                                       XmSELECT_WHITESPACE, XmsdRight, 1, TRUE);
        if(right > pos) 
	    right = pos;
        AddNewLine(w, event, True);
        cursorPos = from_pos = to_pos = XmTextGetCursorPosition(w);
        while(left < right){
            left=(*tw->text.source->ReadSource)(tw->text.source,
						 left, right, &block);
            if (_XmTextModifyVerify(tw, event, &from_pos, &to_pos,
			 &newCursorPos, &block, &newblock, &freeBlock)) {
	       if ((*tw->text.source->Replace)(tw, NULL, &from_pos, &to_pos,
					       &newblock, False) != EditDone) {
	           RingBell(w, event, params, num_params);
	           if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
	           break;
	       } else {
                   cursorPos = newCursorPos;
	           if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
		   value_changed = True;
	       }
            } else {
	       RingBell(w, event, params, num_params);
	       break;
            }
        }
        _XmTextSetCursorPosition(w, cursorPos);
        CheckDisjointSelection(w, tw->text.cursor_position, event->xkey.time);
	if (value_changed)
	  _XmTextValueChanged(tw, event);
    }
    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
RedrawDisplay( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
RedrawDisplay(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition top = XmTextGetTopCharacter(w);

    _XmTextInvalidate(tw, top, top, NODELTA);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Activate( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Activate(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmAnyCallbackStruct cb;
    XmTextWidget tw = (XmTextWidget) w;
    XmParentInputActionRec  p_event ;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_ACTIVATE ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    cb.reason = XmCR_ACTIVATE;
    cb.event  = event;
    XtCallCallbackList(w, tw->text.activate_callback, (XtPointer) &cb);

    (void) _XmParentProcess(XtParent(tw), (XmParentProcessData) &p_event);
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
ToggleOverstrike( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ToggleOverstrike(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    OutputData o_data = tw->text.output->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->overstrike = !data->overstrike;
    o_data->refresh_ibeam_off = True;
    if (data->overstrike)
      o_data->cursorwidth = o_data->cursorheight >> 1;
    else {
      o_data->cursorwidth = 5;
      if (o_data->cursorheight > 19) 
	o_data->cursorwidth++;
      _XmTextResetClipOrigin(tw, tw->text.cursor_position, False);
    }
    _XmTextToggleCursorGC(w);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ToggleAddMode( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ToggleAddMode(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition left, right;

    (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, off);

    tw->text.add_mode = !tw->text.add_mode;

    _XmTextToggleCursorGC(w);

    (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);

    if (tw->text.add_mode &&
        (!(*tw->text.source->GetSelection)(data->widget->text.source,
					    &left, &right) || left == right)) {
       data->anchor = tw->text.dest_position;
    }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SetCursorPosition( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SetCursorPosition(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    _XmTextSetCursorPosition(w, (*tw->text.output->XYToPos)(tw,
					 event->xbutton.x, event->xbutton.y));
}

static void 
#ifdef _NO_PROTO
RemoveBackwardChar( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveBackwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos, nextPos, left, right;

    cursorPos = nextPos = XmTextGetCursorPosition(w);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (NeedsPendingDeleteDisjoint(tw, &left, &right, TRUE)) {
       if (kill)
	  KillCurrentSelection(w, event, params, num_params);
       else
	  DeleteCurrentSelection(w, event, params, num_params);
    } else {
        nextPos = XmTextGetCursorPosition(w);
	cursorPos = (*tw->text.source->Scan)(tw->text.source, nextPos,
				       XmSELECT_POSITION, XmsdLeft, 1, TRUE);
	if (DeleteOrKill(tw, event, cursorPos, nextPos, kill)) {
	   _XmTextSetCursorPosition(w, cursorPos);
           CheckDisjointSelection(w, tw->text.cursor_position,
				  event->xkey.time);
           _XmTextValueChanged(tw, event);
        }
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteBackwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteBackwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveBackwardChar(w, event, params, num_params, FALSE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillBackwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillBackwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveBackwardChar(w, event, params, num_params, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


static void 
#ifdef _NO_PROTO
RemoveForwardWord( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveForwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, right;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (NeedsPendingDeleteDisjoint(tw, &left, &right, TRUE)) {
       if (kill)
	  KillCurrentSelection(w, event, params, num_params);
       else
	  DeleteCurrentSelection(w, event, params, num_params);
    } else {
        _XmTextDisableRedisplay(tw, TRUE);
        left = XmTextGetCursorPosition(w);
        right = (*tw->text.source->Scan)(tw->text.source, left,
                                             XmSELECT_WORD, XmsdRight, 1, TRUE);

        if (left < right) {
           if (DeleteOrKill(tw, event, left, right, kill)) {
	      _XmTextSetCursorPosition(w, left);
              CheckDisjointSelection(w, tw->text.cursor_position,
				     event->xkey.time);
              _XmTextValueChanged(tw, event);
           }
        }
        _XmTextEnableRedisplay(tw);
   }
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteForwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteForwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveForwardWord(w, event, params, num_params, FALSE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillForwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillForwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveForwardWord(w, event, params, num_params, TRUE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
RemoveBackwardWord( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveBackwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, right;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (NeedsPendingDeleteDisjoint(tw, &left, &right, TRUE)) {
       if (kill)
          KillCurrentSelection(w, event, params, num_params);
       else
	  DeleteCurrentSelection(w, event, params, num_params);
    } else {
       _XmTextDisableRedisplay(tw, TRUE);
       right = XmTextGetCursorPosition(w);
       left = (*tw->text.source->Scan)(tw->text.source, right,
                                        XmSELECT_WORD, XmsdLeft, 1, FALSE);
       if (left == right){
          left = (*tw->text.source->Scan)(tw->text.source, left,
                                          XmSELECT_WORD, XmsdLeft, 1, TRUE);
          left = (*tw->text.source->Scan)(tw->text.source, left,
                                          XmSELECT_WORD, XmsdLeft, 1, FALSE);
       }
       if (left < right) {
          if (DeleteOrKill(tw, event, left, right, kill)) {
	     _XmTextSetCursorPosition(w, left);
             CheckDisjointSelection(w, tw->text.cursor_position,
				    event->xkey.time);
	     _XmTextValueChanged(tw, event);
          }
       }
       _XmTextEnableRedisplay(tw);
   }
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteBackwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteBackwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveBackwardWord(w, event, params, num_params, FALSE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillBackwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillBackwardWord(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveBackwardWord(w, event, params, num_params, TRUE);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
RemoveForwardChar( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveForwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition cursorPos, nextPos, left, right;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (NeedsPendingDeleteDisjoint(tw, &left, &right, TRUE)) {
       if (kill)
	  KillCurrentSelection(w, event, params, num_params);
       else
	  DeleteCurrentSelection(w, event, params, num_params);
    } else {
        cursorPos = XmTextGetCursorPosition(w);
	nextPos = (*tw->text.source->Scan)(tw->text.source, cursorPos,
				       XmSELECT_POSITION, XmsdRight, 1, TRUE);
	if (DeleteOrKill(tw, event, cursorPos, nextPos, kill)) {
	   _XmTextSetCursorPosition(w, cursorPos);
           CheckDisjointSelection(w, tw->text.cursor_position,event->xkey.time);
	   _XmTextValueChanged(tw, event);
       }
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillForwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillForwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveForwardChar(w, event, params, num_params, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteForwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteForwardChar(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveForwardChar(w, event, params, num_params, FALSE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
RemoveToEndOfLine( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveToEndOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, right;
    LineNum line;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, TRUE);
    left = XmTextGetCursorPosition(w);
    line = _XmTextPosToLine(tw, left);
    if (line == NOLINE)
      right = left;
    else {
      _XmTextLineInfo(tw, line+1, &right, (LineTableExtra *) NULL);
      if (right == PASTENDPOS)
      right = (*tw->text.source->Scan)(tw->text.source, right,
                                       XmSELECT_ALL, XmsdRight, 1, TRUE);
      else
      right = (*tw->text.source->Scan)(tw->text.source, right,
                                       XmSELECT_POSITION, XmsdLeft, 1, TRUE);
    }
    if (left < right) {
	if (DeleteOrKill(tw, event, left, right, kill)) {
	   _XmTextSetCursorPosition(w, left);
           CheckDisjointSelection(w, tw->text.cursor_position,event->xkey.time);
	   _XmTextValueChanged(tw, event);
        }
    } else if (left == right)
	DeleteForwardChar(w, event, params, num_params);	

    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
RemoveToStartOfLine( w, event, params, num_params, kill )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        Boolean kill ;
#else
RemoveToStartOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
#if NeedWidePrototypes
        int kill )
#else
        Boolean kill )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, cursorPos;
    LineNum line;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextDisableRedisplay(tw, TRUE);
    cursorPos = XmTextGetCursorPosition(w);
    XmTextShowPosition(w, cursorPos);
    line = _XmTextPosToLine(tw, cursorPos);
    if (line == NOLINE) {
       XBell(XtDisplay(tw), 0);
    } else {
	_XmTextLineInfo(tw, line, &left, (LineTableExtra *) NULL);
        if (left < cursorPos) {
	   if (DeleteOrKill(tw, event, left, cursorPos, kill)) {
	      _XmTextSetCursorPosition(w, left);
              CheckDisjointSelection(w, tw->text.cursor_position,
				     event->xkey.time);
	      _XmTextValueChanged(tw, event);
	   }
        } else if (left == cursorPos)
	   DeleteBackwardChar(w, event, params, num_params);	

    }
    _XmTextEnableRedisplay(tw);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteToStartOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteToStartOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveToStartOfLine(w, event, params, num_params, FALSE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillToStartOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillToStartOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveToStartOfLine(w, event, params, num_params, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
DeleteToEndOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeleteToEndOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveToEndOfLine(w, event, params, num_params, FALSE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
KillToEndOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KillToEndOfLine(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    RemoveToEndOfLine(w, event, params, num_params, TRUE);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
RestorePrimaryHighlight( data, prim_left, prim_right )
        InputData data ;
        XmTextPosition prim_left ;
        XmTextPosition prim_right ;
#else
RestorePrimaryHighlight(
        InputData data,
        XmTextPosition prim_left,
        XmTextPosition prim_right )
#endif /* _NO_PROTO */
{
   if (data->sel2Right >= prim_left && data->sel2Right <= prim_right) {
     /* secondary selection is totally inside primary selection */
      if (data->sel2Left >= prim_left) {
         XmTextSetHighlight((Widget) data->widget, prim_left, data->sel2Left,
                            XmHIGHLIGHT_SELECTED);
         XmTextSetHighlight((Widget) data->widget, data->sel2Left, data->sel2Right,
                            XmHIGHLIGHT_NORMAL);
         XmTextSetHighlight((Widget) data->widget, data->sel2Right, prim_right,
                            XmHIGHLIGHT_SELECTED);
     /* right side of secondary selection is inside primary selection */
      } else {
         XmTextSetHighlight((Widget) data->widget, prim_left, data->sel2Right,
                            XmHIGHLIGHT_SELECTED);
         XmTextSetHighlight((Widget) data->widget, data->sel2Left, prim_left,
                            XmHIGHLIGHT_NORMAL);
      }
   } else {
     /* left side of secondary selection is inside primary selection */
      if (data->sel2Left <= prim_right && data->sel2Left >= prim_left) {
         XmTextSetHighlight((Widget) data->widget, data->sel2Left, prim_right,
                            XmHIGHLIGHT_SELECTED);
         XmTextSetHighlight((Widget) data->widget, prim_right, data->sel2Right,
                            XmHIGHLIGHT_NORMAL);
      } else  {
       /* secondary selection encompasses the primary selection */
        if (data->sel2Left <= prim_left && data->sel2Right >= prim_right){
           XmTextSetHighlight((Widget) data->widget, data->sel2Left, prim_left,
                              XmHIGHLIGHT_NORMAL);
           XmTextSetHighlight((Widget) data->widget, prim_left, prim_right,
                              XmHIGHLIGHT_SELECTED);
           XmTextSetHighlight((Widget) data->widget, prim_right, data->sel2Right,
                              XmHIGHLIGHT_NORMAL);
     /* secondary selection is outside primary selection */
        } else {
           XmTextSetHighlight((Widget) data->widget, prim_left, prim_right,
                              XmHIGHLIGHT_SELECTED);
           XmTextSetHighlight((Widget) data->widget, data->sel2Left, data->sel2Right,
                              XmHIGHLIGHT_NORMAL);
        }
      }
   }
}

Boolean 
#ifdef _NO_PROTO
_XmTextSetSel2( tw, left, right, set_time )
        XmTextWidget tw ;
        XmTextPosition left ;
        XmTextPosition right ;
        Time set_time ;
#else
_XmTextSetSel2(
        XmTextWidget tw,
        XmTextPosition left,
        XmTextPosition right, /* if right == -999, then we're in */
        Time set_time )       /*   LoseSelection, so don't call  */
#endif /* _NO_PROTO */        /*   XtDisownSelection. */
{
    InputData data = tw->text.input->data;
    Boolean result = TRUE;

    _XmTextDisableRedisplay(data->widget, FALSE);
    if (data->hasSel2) {
       XmTextPosition prim_left, prim_right;

      /* If the tw has the primary selection, make sure the selection
       * highlight is restored appropriately.
       */
       if ((*data->widget->text.source->GetSelection)(data->widget->text.source,
					    &prim_left, &prim_right))
          RestorePrimaryHighlight(data, prim_left, prim_right);
       else
          XmTextSetHighlight((Widget) data->widget, data->sel2Left, 
			     data->sel2Right, XmHIGHLIGHT_NORMAL);
    }

    if (left <= right) {
       if (!data->hasSel2) {
          result = XtOwnSelection((Widget) data->widget, XA_SECONDARY, set_time,
			         _XmTextConvert, 
			         _XmTextLoseSelection,
			         (XtSelectionDoneProc) NULL);
	  data->sec_time = set_time;
	  data->hasSel2 = result;
       } else 
	  result = TRUE;
       if (result) {
	  XmTextSetHighlight((Widget) data->widget, left, right,
	  		      XmHIGHLIGHT_SECONDARY_SELECTED);
	  data->sel2Left = left;
	  data->sel2Right = right;
       }
    } else {
	data->hasSel2 = FALSE;
	if (right != -999)
	   XtDisownSelection((Widget) data->widget, XA_SECONDARY, set_time);
    }
    _XmTextEnableRedisplay(data->widget);
    return result;
}

Boolean 
#ifdef _NO_PROTO
_XmTextGetSel2( tw, left, right )
        XmTextWidget tw;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
_XmTextGetSel2(
        XmTextWidget tw,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{
    InputData data = tw->text.input->data;

    if (data->hasSel2 && data->sel2Left <= data->sel2Right) {
	*left = data->sel2Left;
	*right = data->sel2Right;
	return TRUE;
    } else {
        data->hasSel2 = FALSE;
        return FALSE;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SetSelectionHint( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SetSelectionHint(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    data->selectionHint.x = event->xbutton.x;
    data->selectionHint.y = event->xbutton.y;
}

/*
 * This routine implements multi-click selection in a hardwired manner.
 * It supports multi-click entity cycling (char, word, line, file) and mouse
 * motion adjustment of the selected entities (i.e. select a word then, with
 * button still down, adjust which word you really meant by moving the mouse).
 * [Note: This routine is to be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
 */
static void 
#ifdef _NO_PROTO
a_Selection( tw, x, y, sel_time )
        XmTextWidget tw ;
        Position x ;
        Position y ;
        Time sel_time ;
#else
a_Selection(
        XmTextWidget tw,
#if NeedWidePrototypes
        int x,
        int y,
#else
        Position x,
        Position y,
#endif
        Time sel_time )
#endif /* _NO_PROTO */
{
    InputData data = tw->text.input->data;
    XmTextPosition position, newLeft, newRight;

    _XmTextDisableRedisplay(tw, FALSE);
    position = (*tw->text.output->XYToPos)(tw, x, y);

    newLeft = (*tw->text.source->Scan)(tw->text.source, position,
			 data->stype, XmsdLeft, 1, FALSE);
    newRight = (*tw->text.source->Scan)(tw->text.source, position,
		 data->stype, XmsdRight, 1, data->stype == XmSELECT_LINE);
    if (data->stype == XmSELECT_WORD && (int)tw->text.char_size > 1) {
	if (position == (*tw->text.source->Scan)
	    (tw->text.source, newLeft, data->stype, XmsdRight, 1, FALSE))
	    newLeft = position;
    }
    (*tw->text.source->SetSelection)(tw->text.source, newLeft,
					 newRight, sel_time);
    tw->text.pendingoff = FALSE;
    if (position - newLeft < newRight - position) {
	_XmTextSetCursorPosition((Widget) tw, newLeft); 
	data->extendDir = XmsdLeft;
    } else {
	_XmTextSetCursorPosition((Widget) tw, newRight); 
	data->extendDir = XmsdRight;
    }
    _XmTextSetDestinationSelection((Widget)tw, tw->text.cursor_position,
				   False, sel_time);
    XmTextShowPosition((Widget) tw, (XmTextPosition) -1);
    _XmTextEnableRedisplay(tw);
    data->origLeft = newLeft;
    data->origRight = newRight;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SetAnchor( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SetAnchor(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition left, right;

    data->anchor = XmTextGetCursorPosition(w);
    _XmTextSetDestinationSelection(w, data->anchor, False, event->xkey.time);
    if ((*tw->text.source->GetSelection)
                     (tw->text.source, &left, &right)) {
       (*tw->text.source->SetSelection)(tw->text.source, data->anchor,
					    data->anchor, event->xkey.time);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DoSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DoSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    a_Selection(tw, event->xbutton.x, event->xbutton.y,
		event->xbutton.time);
}

static void 
#ifdef _NO_PROTO
SetScanType( w, data, event )
        Widget w ;
        InputData data ;
        XEvent *event ;
#else
SetScanType(
        Widget w,
        InputData data,
        XEvent *event )
#endif /* _NO_PROTO */
{
    int i;
    int multi_click_time;

    multi_click_time = XtGetMultiClickTime(XtDisplay(w));

    if (event->xbutton.time > data->lasttime &&
        event->xbutton.time - data->lasttime <
			 (multi_click_time == 200 ? 500 : multi_click_time)) {

        i = 0;
	while (i < data->sarraycount && data->sarray[i] != data->stype) i++;

	if (++i >= data->sarraycount) i = 0;
	data->stype = data->sarray[i];
    } else {			/* single-click event */
	data->stype = data->sarray[0];
    }
    data->lasttime = event->xbutton.time;
}

static void 
#ifdef _NO_PROTO
StartPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
StartPrimary(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{                                              
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition left, right;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->anchor = (*tw->text.output->XYToPos)(tw, event->xbutton.x,
					           event->xbutton.y);
    SetSelectionHint(w, event, params, num_params);
    SetScanType(w, data, event);
    if (data->stype != XmSELECT_POSITION || 
        ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) &&
	 left != right))
       DoSelection(w, event, params, num_params);
    else
       _XmTextSetDestinationSelection(w, data->anchor,
				      False, event->xbutton.time);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
    
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
StartSecondary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
StartSecondary(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{                                              
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    int status;

    data->sel_start = True;
    XAllowEvents(XtDisplay(w), AsyncBoth, event->xbutton.time);
    data->Sel2Hint.x = event->xbutton.x;
    data->Sel2Hint.y = event->xbutton.y;
    data->selectionMove = FALSE;
    data->cancel = False;

    status = XtGrabKeyboard(w, False, GrabModeAsync,
			        GrabModeAsync, CurrentTime);

    if (status != GrabSuccess) _XmWarning(w, GRABKBDERROR);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
StartDrag( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
StartDrag(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{                                              
    XmTextWidget tw = (XmTextWidget) w;
    Atom targets[4];
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status = 0;
    Cardinal num_targets = 0;
    Widget drag_icon;
    Arg args[10];
    int n = 0;

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       targets[num_targets++] = tmp_prop.encoding;
    else
       targets[num_targets++] = 99999; /* XmbTextList...  should never fail
					* for XPCS characters.  But just in
					* case someones Xlib is broken,
					* this prevents a core dump.
                            		*/

    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

    targets[num_targets++] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    targets[num_targets++] = XA_STRING;
    targets[num_targets++] = XmInternAtom(XtDisplay(w), "TEXT", False);

    drag_icon = _XmGetTextualDragIcon(w);

    n = 0;
    XtSetArg(args[n], XmNcursorBackground, tw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNcursorForeground, tw->primitive.foreground);  n++;
    XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++;
    XtSetArg(args[n], XmNexportTargets, targets);  n++;
    XtSetArg(args[n], XmNnumExportTargets, num_targets);  n++;
    XtSetArg(args[n], XmNconvertProc, _XmTextConvert);  n++;
    XtSetArg(args[n], XmNclientData, w);  n++;
    if (tw->text.editable) {
       XtSetArg(args[n], XmNdragOperations, (XmDROP_MOVE | XmDROP_COPY)); n++;
    } else {
       XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
    }
    (void) XmDragStart(w, event, args, n);
}


/* ARGSUSED */
static void
#ifdef _NO_PROTO
ProcessBDrag( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ProcessBDrag(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition position, left, right;
    Position left_x, left_y, right_x, right_y;
    InputData data = tw->text.input->data;

    position = (*tw->text.output->XYToPos)(tw, event->xbutton.x,
                                           event->xbutton.y);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) &&
	(right != left)) {
       if ((position > left && position < right) ||
	   /* Take care of border conditions */
	    ((position == left &&
	      (*tw->text.output->PosToXY)(tw, left, &left_x, &left_y) &&
	      event->xbutton.x > left_x)) ||
            ((position == right &&
	      (*tw->text.output->PosToXY)(tw, right, &right_x, &right_y) &&
	      event->xbutton.x < right_x))) {
          data->sel_start = False;
          StartDrag(w, event, params, num_params);
       } else {
          StartSecondary(w, event, params, num_params);
       }
    } else {
       StartSecondary(w, event, params, num_params);
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/*
 * This routine implements extension of the currently selected text in
 * the "current" mode (i.e. char word, line, etc.). It worries about
 * extending from either end of the selection and handles the case when you
 * cross through the "center" of the current selection (e.g. switch which
 * end you are extending!).
 * [NOTE: This routine will be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
*/
static Boolean 
#ifdef _NO_PROTO
dragged( selectionHint, event, threshold )
        SelectionHint selectionHint ;
        XEvent *event ;
        int threshold ;
#else
dragged(
        SelectionHint selectionHint,
        XEvent *event,
        int threshold )
#endif /* _NO_PROTO */
{
  int xdiff, ydiff;
    xdiff = abs(selectionHint.x - event->xbutton.x);
    ydiff = abs(selectionHint.y - event->xbutton.y);
    if((xdiff > threshold) || (ydiff > threshold))
        return TRUE;
    else
	return FALSE;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DoExtendedSelection(w, ev_time)
	Widget w;
	Time ev_time;
#else
DoExtendedSelection(
	Widget w,
	Time ev_time ) 
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition position, left, right, cursorPos;
    float bal_point;

    if (data->cancel) {
	  if (data->select_id) XtRemoveTimeOut(data->select_id);
          data->select_id = 0;
          return;
    }

    _XmTextDisableRedisplay(tw, FALSE);
    if (!((*tw->text.source->GetSelection)
	 (tw->text.source, &left, &right)) || left == right) {
        data->anchor = tw->text.dest_position;
        left = right = XmTextGetCursorPosition(w);
	data->origLeft = data->origRight = data->anchor;
        bal_point = data->anchor;
    } else 
        bal_point = (float)(((float)(data->origRight - data->origLeft) / 2.0) +
		       (float)data->origLeft);

    position = (*tw->text.output->XYToPos)(tw, data->select_pos_x,
					  data->select_pos_y);

 /* shift anchor and direction to opposite end of the selection */

       if ((float)position <= bal_point) {
          data->anchor = data->origRight;
          if (!data->extending)
             data->extendDir = XmsdLeft;
       } else if ((float)position > bal_point) {
          data->anchor = data->origLeft;
          if (!data->extending)
             data->extendDir = XmsdRight;
       } 

    data->extending = TRUE;

    /* check for change in extend direction */
    if ((data->extendDir == XmsdRight && position < data->anchor) ||
	  (data->extendDir == XmsdLeft && position > data->anchor)) {
	data->extendDir =
	    (data->extendDir == XmsdRight) ? XmsdLeft : XmsdRight;

	left = data->origLeft;
	right = data->origRight;
    }
    
	
    if (data->extendDir == XmsdRight) {
	right = cursorPos =
		 (*tw->text.source->Scan)(tw->text.source, position,
						    data->stype, XmsdRight, 1,
                                                  data->stype == XmSELECT_LINE);
        left = data->anchor;
    } else {
        left = cursorPos = (*tw->text.source->Scan)(tw->text.source,
						        position, data->stype,
							XmsdLeft, 1, FALSE);
	if (data->stype == XmSELECT_WORD &&
	    (int)tw->text.char_size > 1) {
	   if (position == (*tw->text.source->Scan) (tw->text.source,
							 left, data->stype,
							 XmsdRight, 1, FALSE))
              left = cursorPos = position;
        }
        right = data->anchor;
    }

    (*tw->text.source->SetSelection)(tw->text.source, left, right, ev_time);
    tw->text.pendingoff = FALSE;
    _XmTextSetCursorPosition(w, cursorPos);
    _XmTextSetDestinationSelection(w, tw->text.cursor_position, False, ev_time);
    _XmTextEnableRedisplay(tw);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DoSecondaryExtend(w, ev_time)
	Widget w;
	Time ev_time;
#else
DoSecondaryExtend(
	Widget w,
	Time ev_time ) 
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition position, left, right;

    position = (*tw->text.output->XYToPos)(tw, data->select_pos_x,
                                          data->select_pos_y);

    _XmTextDisableRedisplay(tw, FALSE);
    _XmTextGetSel2(tw, &left, &right);
    /* check for change in extend direction */
    if ((data->Sel2ExtendDir == XmsdRight && position < data->Sel2OrigLeft) ||
	  (data->Sel2ExtendDir == XmsdLeft &&
				 position > data->Sel2OrigRight)) {
	data->Sel2ExtendDir =
	    (data->Sel2ExtendDir == XmsdRight) ? XmsdLeft : XmsdRight;
	left = data->Sel2OrigLeft;
	right = data->Sel2OrigRight;
    }
	
    if (data->Sel2ExtendDir == XmsdRight)
	right = (*tw->text.source->Scan)(tw->text.source, position,
				XmSELECT_POSITION, XmsdRight,1, FALSE);
    else
	left = (*tw->text.source->Scan)(tw->text.source, position,
				XmSELECT_POSITION, XmsdLeft,  1, FALSE);
    (void) _XmTextSetSel2(tw, left, right, ev_time);
    XmTextShowPosition(w, position);
    _XmTextEnableRedisplay(tw);
}

/************************************************************************
 *                                                                      *
 * BrowseScroll - timer proc that scrolls the list if the user has left *
 *              the window with the button down. If the button has been *
 *              released, call the standard click stuff.                *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
BrowseScroll( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
BrowseScroll(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
    Widget w = (Widget) closure ;
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmScrollBarWidget vsb = (XmScrollBarWidget) tw->text.output->data->vbar;
    unsigned long interval;

    if (data->cancel) {
       data->select_id = 0;
       return;
    }
    
    if (!data->select_id) return;

    if (data->Sel2Extending)
      DoSecondaryExtend(w, XtLastTimestampProcessed(XtDisplay(w)));
    else if (data->extending)
      DoExtendedSelection(w, XtLastTimestampProcessed(XtDisplay(w)));

    if (tw->text.output->data->vbar)
       interval = (unsigned long) vsb->scrollBar.repeat_delay;
    else
       interval = 100;
    
    XSync (XtDisplay(w), False);
    
    data->select_id = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
				   interval, BrowseScroll, (XtPointer) w);
}
    

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
CheckTimerScrolling( w, event )
        Widget w ;
        XEvent *event ;
#else
CheckTimerScrolling(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    OutputData o_data = tw->text.output->data;
    XmScrollBarWidget vsb = (XmScrollBarWidget) o_data->vbar;
    unsigned long interval;

    data->select_pos_x = event->xmotion.x;
    data->select_pos_y = event->xmotion.y;

    if ((event->xmotion.x > (int) o_data->leftmargin) &&
	(event->xmotion.x < (int) (tw->core.width - o_data->rightmargin))  &&
	(event->xmotion.y > (int) o_data->topmargin) &&
        (event->xmotion.y < (int) (o_data->topmargin + (o_data->lineheight *
				                      o_data->number_lines)))) {

       if (data->select_id) {
          XtRemoveTimeOut(data->select_id);
          data->select_id = 0;
       }
    } else {
       /* to the left of the text */
        if (event->xmotion.x <= (int) o_data->leftmargin)
           data->select_pos_x = (Position) (o_data->leftmargin -
				            (o_data->averagecharwidth + 1));
       /* to the right of the text */
	else if (event->xmotion.x >= (int) (tw->core.width - 
				            o_data->rightmargin))
           data->select_pos_x = (Position) ((tw->core.width -
				             o_data->rightmargin) +
				             o_data->averagecharwidth + 1);
       /* above the text */
        if (event->xmotion.y <= (int) o_data->topmargin) {
           data->select_pos_y = (int) (o_data->topmargin - o_data->lineheight);
           if (tw->text.top_line == 0)
              data->select_pos_x = 0;

       /* below the text */
        } else if (event->xmotion.y >= (int) (o_data->topmargin +
				   (o_data->lineheight * o_data->number_lines)))
           data->select_pos_y = o_data->topmargin + (o_data->lineheight *
				                    (o_data->number_lines + 1));

       if (o_data->vbar)
          interval = (unsigned long) vsb->scrollBar.initial_delay;
       else
          interval = 200;

       if (!data->select_id)
          data->select_id = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
				         interval, BrowseScroll, (XtPointer) w);
       return True; 
    }
    return False;
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
StartExtendSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
StartExtendSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    data->cancel = False;
    ExtendSelection(w, event, params, num_params);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    OutputData o_data = tw->text.output->data;

    if (data->cancel) return;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (!o_data->hasfocus && _XmGetFocusPolicy(w) == XmEXPLICIT)
       (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

    if (data->selectionHint.x || data->selectionHint.y){
        if(!dragged(data->selectionHint, event, data->threshold)) {
           (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					          on);
	    return;
	}
	a_Selection(tw, data->selectionHint.x, data->selectionHint.y,
		    event->xbutton.time);
        data->selectionHint.x = data->selectionHint.y = 0;
	data->extending = True;
    }

    if (!CheckTimerScrolling(w, event))
       DoExtendedSelection(w, event->xbutton.time);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendSecondary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendSecondary(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition position, hintposition;

    if (data->cancel) return;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    position = (*tw->text.output->XYToPos)(tw, event->xbutton.x,
                                          event->xbutton.y);
    if(data->Sel2Hint.x || data->Sel2Hint.y){
        if(!dragged(data->Sel2Hint, event, data->threshold)){
           (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
						  on);
	    return;
	}
	hintposition = (*tw->text.output->XYToPos)(tw, data->Sel2Hint.x,
                                          data->Sel2Hint.y);
	if(position < hintposition) {
            data->Sel2Extending = _XmTextSetSel2(tw, position, hintposition,
					         event->xbutton.time);
	    data->Sel2OrigLeft = hintposition; /**/
	    data->Sel2OrigRight = hintposition;
	    data->Sel2ExtendDir = XmsdLeft;
	} else {
	    data->Sel2Extending = _XmTextSetSel2(tw, hintposition, position,
					         event->xbutton.time);
	    data->Sel2OrigLeft = hintposition;
	    data->Sel2OrigRight = hintposition; /**/
	    data->Sel2ExtendDir = XmsdRight;
	}
        data->Sel2Hint.x = data->Sel2Hint.y = 0;
    } 

    if(!data->Sel2Extending){
        (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					       on);
	return;
    }

    if (!CheckTimerScrolling(w, event))
       DoSecondaryExtend(w, event->xmotion.time);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
ExtendEnd( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendEnd(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    if(data->extending || dragged(data->Sel2Hint, event, data->threshold)){
        ExtendSelection(w, event, params, num_params);
        (*tw->text.source->GetSelection)(tw->text.source,
				    &(data->origLeft), &(data->origRight));
    }

    if (data->select_id) {
       XtRemoveTimeOut(data->select_id);
       data->select_id = 0;
    }

    data->select_pos_x = 0;
    data->select_pos_y = 0;
    data->extending = FALSE;
    data->selectionHint.x = data->selectionHint.y = 0;

    data->cancel = True;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DoGrabFocus( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DoGrabFocus(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{

   XmTextWidget tw = (XmTextWidget) w;
   InputData data = tw->text.input->data;
   OutputData o_data = tw->text.output->data;

    data->cancel = False;
   /* to the left of the text */
    if (event->xbutton.x <= (int) o_data->leftmargin)
       event->xbutton.x = (Position) (o_data->leftmargin + 1);

   /* to the right of the text */
    else if (event->xbutton.x >= (int) (tw->core.width - o_data->rightmargin))
       event->xbutton.x = (Position)((tw->core.width - o_data->rightmargin)- 1);
   /* above the text */
    if (event->xbutton.y <= (int) o_data->topmargin)
       event->xbutton.y = (int) (o_data->topmargin + 1);

   /* below the text */
    else if (event->xbutton.y >= (int)(o_data->topmargin + (o_data->lineheight *
						         o_data->number_lines)))
       event->xbutton.y = (o_data->topmargin +
		(o_data->lineheight * o_data->number_lines)) - 1;

   if (_XmGetFocusPolicy(w) == XmEXPLICIT)
       (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

   StartPrimary(w, event, params, num_params);
   if (data->stype == XmSELECT_POSITION)
      SetCursorPosition(w, event, params, num_params);
}
 
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveDestination( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MoveDestination(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;
   XmTextPosition new_pos, left, right;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   new_pos = (*tw->text.output->XYToPos)(tw, event->xbutton.x,
					           event->xbutton.y);

   if ((*tw->text.source->GetSelection)(tw->text.source, &left, &right)
        && (right != left))
      _XmTextSetDestinationSelection(w, new_pos, False, event->xkey.time);

   tw->text.pendingoff = False;
   if (_XmGetFocusPolicy(w) == XmEXPLICIT)
      (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

   _XmTextSetCursorPosition(w, new_pos);
   if (tw->text.cursor_position < left || tw->text.cursor_position > right)
      tw->text.pendingoff = TRUE;
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* Pastes the primary selection to the stuff position. */

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DoStuff( w, closure, seltype, type, value, length, format )
        Widget w ;
        XtPointer closure ;
        Atom *seltype ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
DoStuff(
        Widget w,
        XtPointer closure,
        Atom *seltype,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    OutputData o_data = tw->text.output->data;
    Atom NULL_ATOM = XmInternAtom(XtDisplay(w), "NULL", False);
    XmTextBlockRec block, newblock;
    XmTextPosition cursorPos = tw->text.cursor_position;
    XmTextPosition right, left, replace_from, replace_to;
    _XmTextPrimSelect *prim_select = (_XmTextPrimSelect *) closure;
    XTextProperty tmp_prop;
    int i, status = 0;
    int malloc_size = 0;
    int num_vals;
    char **tmp_value;
    char * total_tmp_value = NULL;
    Boolean freeBlock;

    if (!o_data->hasfocus && _XmGetFocusPolicy(w) == XmEXPLICIT)
     (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

    if (!(*length) && *type != NULL_ATOM) {
      /* Backwards compatibility for 1.0 Selections */
       if (prim_select->target == XmInternAtom(XtDisplay(tw), "TEXT", False)) {
          prim_select->target = XA_STRING;
          XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, DoStuff,
			   (XtPointer)prim_select, prim_select->time);
       }
       XtFree((char *)value);
       value = NULL;
       return;
    }

   /* if length == 0 and *type is the NULL atom we are assuming
    * that a DELETE target is requested.
    */
    if (*type == NULL_ATOM) {
       if (prim_select->num_chars > 0 && data->selectionMove) {
          data->anchor = prim_select->position;
          cursorPos = prim_select->position + prim_select->num_chars;
          _XmTextSetCursorPosition(w, cursorPos);
          _XmTextSetDestinationSelection(w, tw->text.cursor_position,
					 False, prim_select->time);
          (*tw->text.source->SetSelection)(tw->text.source, data->anchor,
					   tw->text.cursor_position,
					   prim_select->time);
      }
    } else {
       XmTextSource source = GetSrc(w);
       int max_length = 0;
       Boolean local = _XmStringSourceHasSelection(source);
       Boolean pendingoff;

      /* The on_or_off flag is set to prevent unecessary
	 cursor shifting during the Replace operation */
       tw->text.on_or_off = off;

       if (*type == XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False) ||
	   *type == XA_STRING) {
          tmp_prop.value = (unsigned char *) value;
          tmp_prop.encoding = *type;
          tmp_prop.format = *format;
          tmp_prop.nitems = *length;
	  num_vals = 0;
          status = XmbTextPropertyToTextList(XtDisplay(w), &tmp_prop,
                                             &tmp_value, &num_vals);

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
       } else {
          block.ptr = (char*)value;
          block.length = (int) *length; /* NOTE: this causes a truncation on
					   some architectures */
          block.format = XmFMT_8_BIT;
       }

       if (data->selectionMove && local) {
	  max_length = _XmStringSourceGetMaxLength(source);
          _XmStringSourceSetMaxLength(source, MAXINT);
       }
	  
       replace_from = replace_to = prim_select->position;

       pendingoff = tw->text.pendingoff;
       tw->text.pendingoff = FALSE;

       if (_XmTextModifyVerify(tw, NULL, &replace_from, &replace_to,
                               &cursorPos, &block, &newblock, &freeBlock)) {
          prim_select->num_chars = _XmTextCountCharacters(newblock.ptr,
							  newblock.length);
	  if ((*tw->text.source->Replace)(tw, NULL, &replace_from, &replace_to,
					  &newblock, False) != EditDone) {
	      RingBell(w, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
	      tw->text.pendingoff = pendingoff;
	  } else {
	      if (newblock.length > 0 && !data->selectionMove) {
		 _XmTextSetCursorPosition(w, cursorPos);
		 _XmTextSetDestinationSelection(w, tw->text.cursor_position,
						False, prim_select->time);
	      }
	      if (XmTextGetSelectionPosition(w, &left, &right)) {
		  if (data->selectionMove) {
		     if (left < replace_from) {
		       prim_select->position = replace_from -
					       prim_select->num_chars;
		     } else {
		       prim_select->position = replace_from;
                     }
		  }
		  if (cursorPos < left || cursorPos > right)
		     tw->text.pendingoff = TRUE;
	      } else {
		  if (!data->selectionMove && !tw->text.add_mode &&
		      prim_select->num_chars != 0)
		     data->anchor = prim_select->position;
	      }
	      if (data->selectionMove) {
		 prim_select->ref_count++;
		 XtGetSelectionValue(w, XA_PRIMARY,
				    XmInternAtom(XtDisplay(w), "DELETE", False),
				    DoStuff, (XtPointer)prim_select,
				    prim_select->time);
	      }
	      _XmTextValueChanged(tw, NULL);
	  }
	  if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
       } else {
	  RingBell(w, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
	  tw->text.pendingoff = pendingoff;
       }

       if (data->selectionMove && local) {
          _XmStringSourceSetMaxLength(source, max_length);
       }

       tw->text.on_or_off = on;
    }
    if (malloc_size != 0) XtFree(total_tmp_value);
    XtFree((char *)value);
    if (--prim_select->ref_count == 0)
       XtFree((char *)prim_select);
    value = NULL;
}


/* This function make the request to do a primary paste */

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Stuff( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
Stuff(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  _XmTextActionRec *tmp = (_XmTextActionRec*)XtMalloc(sizeof(_XmTextActionRec));

/* Request targets from the selection owner so you can decide what to
 * request.  The decision process and request for the selection is
 * taken care of in HandleTargets().
 */

  tmp->event = (XEvent *) XtMalloc(sizeof(XEvent));
  memcpy((void *)tmp->event, (void *)event, sizeof(XEvent));

  tmp->params = params;
  tmp->num_params = num_params;

  XtGetSelectionValue(w, XA_PRIMARY,
		      XmInternAtom(XtDisplay(w), "TARGETS", False),
		      HandleTargets, (XtPointer)tmp, event->xbutton.time);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleSelectionReplies( w, closure, ev, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *ev ;
        Boolean *cont ;
#else
HandleSelectionReplies(
        Widget w,
        XtPointer closure,
        XEvent *ev,
        Boolean *cont )
#endif /* _NO_PROTO */
{
   XSelectionEvent *event = (XSelectionEvent *) ev;
   XmTextWidget tw = (XmTextWidget) w;
   XmTextWidget dest_tw;
   InputData data = tw->text.input->data;
   Atom property = (Atom) closure;
   TextDestData dest_data;
   long adjustment = 0;
   XmTextBlockRec block, newblock;
   XmTextPosition left, right;
   Boolean freeBlock;

   if (event->type != SelectionNotify) return;

   XtRemoveEventHandler(w, (EventMask) NULL, TRUE, HandleSelectionReplies,
		       (XtPointer) XmInternAtom(XtDisplay(w),
					        "_XM_TEXT_I_S_PROP", False));

   dest_data = GetTextDestData(w);
   dest_tw = dest_data->widget;

   if (event->property == None) {
      (void) _XmTextSetSel2(tw, 1, 0, XtLastTimestampProcessed(XtDisplay(w)));
      data->selectionMove = False;
   } else {
      XmTextPosition cursorPos;

      if (dest_data->has_destination) {
         adjustment = data->sel2Right - data->sel2Left;

         if (dest_data->position <= data->sel2Left) {
            data->sel2Left -= dest_data->replace_length;
            data->sel2Right += adjustment - dest_data->replace_length;
         } else if (dest_data->position > data->sel2Left &&
                    dest_data->position < data->sel2Right) {
            data->sel2Left -= dest_data->replace_length;
            data->sel2Right += adjustment - dest_data->replace_length;
         }
     }

     left = data->sel2Left;
     right = data->sel2Right;

     (void) _XmTextSetSel2(tw, 1, 0, XtLastTimestampProcessed(XtDisplay(w)));

     if (data->selectionMove) {
        block.ptr = "";
        block.length = 0;
        block.format = XmFMT_8_BIT;
        if (dest_data->position <= data->sel2Left) left += adjustment;
        if (_XmTextModifyVerify(tw, ev, &left, &right,
                                &cursorPos, &block, &newblock, &freeBlock)) {
	   if ((*tw->text.source->Replace)(tw, NULL, &left, &right, 
					   &newblock, False) != EditDone) {
	      RingBell(w, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
	   } else {
              int count;
	      count = _XmTextCountCharacters(newblock.ptr, newblock.length);

	      if (dest_data->has_destination && dest_data->position > right) {
                 if (cursorPos == left + count)
	            cursorPos = dest_data->position + count;    	
		 if (!dest_data->quick_key)
		    _XmTextSetCursorPosition((Widget)dest_tw, cursorPos);
		 _XmTextSetDestinationSelection((Widget)dest_tw,
						dest_tw->text.cursor_position,
						False, event->time);
	      } else if (count > 0 && dest_data->has_destination) {
		 if (!dest_data->quick_key)
		    _XmTextSetCursorPosition((Widget)dest_tw, cursorPos);
		 _XmTextSetDestinationSelection((Widget)dest_tw,
						dest_tw->text.cursor_position,
						False, event->time);
	      }
	      if (!dest_tw->text.source->data->hasselection) {
		 dest_tw->text.input->data->anchor = dest_data->position;
	      }
	      if (!dest_data->has_destination) {
		 XmTextSetAddMode((Widget)dest_tw, False);
	      }
	      _XmTextValueChanged(tw, ev);
	   }
	   if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
        } else {
           RingBell(w, (XEvent *) NULL, (String *) NULL, (Cardinal) 0);
        }
        data->selectionMove = False;
     }
   }

   XDeleteProperty(XtDisplay(w), event->requestor, property);
}


/* Send a client message to perform the quick cut/copy and paste */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SecondaryNotify( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SecondaryNotify(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextWidget dest_tw;
    InputData data = tw->text.input->data;
    Atom XM_TEXT_PROP = XmInternAtom(XtDisplay(w),"_XM_TEXT_I_S_PROP", False);
    Atom CS_OF_LOCALE; /* to be initialized by XmbTextListToTextProperty */
    TextDestData dest_data;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status = 0;
    _XmTextInsertPair tmp_pair[1];
    _XmTextInsertPair *pair = tmp_pair;
    XmTextPosition left, right;

    if (data->selectionMove == TRUE && data->has_destination &&
        tw->text.dest_position >= data->sel2Left &&
        tw->text.dest_position <= data->sel2Right) {
       (void)_XmTextSetSel2(tw, 1, 0, event->xbutton.time);
       return;
    }

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       CS_OF_LOCALE = tmp_prop.encoding;
    else
       CS_OF_LOCALE = (Atom)9999; /* Kludge for failure of XmbText... to
                                      * handle XPCS characters.  Should never
                                      * happen, but this prevents a core dump
                                      * if X11 is broken.
                                      */
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

   /* fill in atom pair */
    pair->selection = XA_SECONDARY;
    pair->target = CS_OF_LOCALE;

   /* add the insert selection property on the text field tw's window */
    XChangeProperty(XtDisplay(w), XtWindow(w), XM_TEXT_PROP, 
		    XmInternAtom(XtDisplay(w), "ATOM_PAIR", False),
		    32, PropModeReplace, (unsigned char *)pair, 2);

    dest_data = GetTextDestData(w);

    dest_data->replace_length = 0;

    dest_tw = tw;

    if (!dest_tw->text.input->data->has_destination &&
       dest_tw->text.source->data->numwidgets > 1) {
       int i;

       for (i=0 ; i<tw->text.source->data->numwidgets ; i++) {
           dest_tw = (XmTextWidget) tw->text.source->data->widgets[i];
         if (dest_tw->text.input->data->has_destination) break;
       }
       if (i == tw->text.source->data->numwidgets) dest_tw = tw;
    }

    dest_data->has_destination = dest_tw->text.input->data->has_destination;
    dest_data->position = dest_tw->text.dest_position;
    dest_data->widget = dest_tw;

    if (*(num_params) == 1) dest_data->quick_key = True;
    else dest_data->quick_key = False;

    if ((*dest_tw->text.source->GetSelection)
		(dest_tw->text.source, &left, &right) && left != right) {
       if (dest_data->position >= left && dest_data->position <= right)
	  dest_data->replace_length = right - left;
    }

   /* add an event handler to handle selection notify events */
    XtAddEventHandler(w, (EventMask) NULL, TRUE,
		      HandleSelectionReplies, (XtPointer) XM_TEXT_PROP);

   /* special fix for handling case of shared source with secondary select */
    XmTextSetHighlight((Widget) data->widget, data->sel2Left,
				data->sel2Right, XmHIGHLIGHT_NORMAL);

   /*
    * Make a request for the primary selection to convert to
    * type INSERT_SELECTION as per ICCCM.
    */
    XConvertSelection(XtDisplay(w),
		      XmInternAtom(XtDisplay(w), "MOTIF_DESTINATION", False),
		      XmInternAtom(XtDisplay(w), "INSERT_SELECTION", False),
		      XM_TEXT_PROP, XtWindow(w), event->xbutton.time);
}

   /* REQUEST TARGETS FROM SELECTION RECEIVER; MOVE THE REST OF THIS
    * TO A NEW ROUTINE (THE NAME OF WHICH IS PASSED DURING THE REQUEST
    * FOR TARGETS).  THE NEW ROUTINE WILL LOOK AT THE TARGET LIST AND
    * DETERMINE WHAT TARGET TO PLACE IN THE PAIR.  IT WILL THEN DO
    * ANY NECESSARY CONVERSIONS BEFORE "THRUSTING" THE SELECTION VALUE
    * ONTO THE RECEIVER.  THIS WILL GUARANTEE THE BEST CHANCE AT A
    * SUCCESSFUL EXCHANGE.
    */

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleTargets( w, closure, seltype, type, value, length, format )
        Widget w ;
        XtPointer closure ;
        Atom *seltype ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
HandleTargets(
        Widget w,
        XtPointer closure,
        Atom *seltype,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    Atom CS_OF_LOCALE; /* to be initialized by XmbTextListToTextProperty */
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w),"COMPOUND_TEXT", False);
    Boolean supports_locale_data = False;
    Boolean supports_CT = False;
    Atom *atom_ptr;
    _XmTextActionRec *tmp_action = (_XmTextActionRec *) closure;
    _XmTextPrimSelect *prim_select; 
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status = 0;
    XtPointer closures[2];
    Atom targets[2];
    XmTextPosition select_pos;
    XmTextPosition left, right;
    int i;

    if (!length) {
       XtFree((char *)value);
       value = NULL;
       XtFree((char *)tmp_action->event);
       XtFree((char *)tmp_action);
       return;
    }

    atom_ptr = (Atom *)value;

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       CS_OF_LOCALE = tmp_prop.encoding;
    else
       CS_OF_LOCALE = (Atom)9999; /* Kludge for failure of XmbText... to 
				      * handle XPCS characters.  Should never 
				      * happen, but this prevents a core dump 
				      * if X11 is broken.
				      */
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

    for (i = 0; i < *length; i++, atom_ptr++) {
      if (*atom_ptr == CS_OF_LOCALE) {
	 supports_locale_data = True;
	 break;
      }
      if (*atom_ptr == COMPOUND_TEXT)
	 supports_CT = True;
    }


    /*
    * Set stuff position to the x and y position of
    * the button pressed event for primary pastes.
    */
    if (tmp_action->event->type == ButtonRelease) {
      select_pos = (*tw->text.output->XYToPos)(tw,
				   (Position) tmp_action->event->xbutton.x,
				   (Position) tmp_action->event->xbutton.y);
    } else {
      select_pos = XmTextGetCursorPosition(w);
    }

    if ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) && 
       left != right && select_pos > left && 
       select_pos < right) {
      XtFree((char *)value);
      value = NULL;
      XtFree((char *)tmp_action->event);
      XtFree((char *)tmp_action);
      return;
    }

    prim_select = (_XmTextPrimSelect *)
		  XtMalloc((unsigned) sizeof(_XmTextPrimSelect));

    prim_select->position = select_pos;

    if (tmp_action->event->type == ButtonRelease) {
      prim_select->time = tmp_action->event->xbutton.time;
    } else {
      prim_select->time = tmp_action->event->xkey.time;
    }

    prim_select->num_chars = 0;

    /* If owner and I are using the same codeset, ask for it.  If not,
    * and if the owner supports compound text, ask for compound text.
    * If not, fall back position is to ask for STRING and try to
    * convert it locally.
    */

    if (supports_locale_data)
      prim_select->target = targets[0] = XmInternAtom(XtDisplay(w), "TEXT",
						      False);
    else if (supports_CT)
      prim_select->target = targets[0] = COMPOUND_TEXT;
    else
      prim_select->target = targets[0] = XA_STRING;

   closures[0] = (char *)prim_select;

   prim_select->ref_count = 1;
  /* Make request to call DoStuff() with the primary selection. */
   XtGetSelectionValue(w, XA_PRIMARY, targets[0], DoStuff,
		       (XtPointer)prim_select,
		       tmp_action->event->xbutton.time);

   XtFree((char *)value);
   value = NULL;
   XtFree((char *)tmp_action->event);
   XtFree((char *)tmp_action);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
VoidAction( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
VoidAction(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   /* Do Nothing */
}

/*
 * This function set the final position of the secondary selection and
 * calls SecondaryNotify().
 */
static void 
#ifdef _NO_PROTO
ExtendSecondaryEnd( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendSecondaryEnd(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    if (!data->cancel) XtUngrabKeyboard(w, CurrentTime);

   /* if the pointer is inside the text area, do the secondary transfer */
    if (event->xbutton.x > tw->core.width || event->xbutton.x < 0 || 
        event->xbutton.y > tw->core.height || event->xbutton.y < 0 ){
       if (data->hasSel2 && data->Sel2Extending) {
          data->cancel = True;
          _XmTextSetSel2(tw, 1, 0, event->xkey.time);
       }
    }

    if ((data->Sel2Extending || dragged(data->Sel2Hint, event, data->threshold))
        && !data->cancel){
       _XmTextGetSel2(tw, &(data->Sel2OrigLeft), &(data->Sel2OrigRight));
       SecondaryNotify(w, event, params, num_params);
    }

   /* Re-initialize the secondary selection data */
    data->select_pos_x = 0;
    data->select_pos_y = 0;
    data->Sel2Extending = FALSE;
    data->Sel2Hint.x = data->Sel2Hint.y = 0;

    if (data->select_id) {
       XtRemoveTimeOut(data->select_id);
       data->select_id = 0;
    }

    data->cancel = True;
}


/*
 * This Action Proc selects all of the text.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SelectAll( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SelectAll(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->stype = XmSELECT_ALL;
    (*tw->text.source->SetSelection)(tw->text.source, 0,
				    XmTextGetLastPosition(w), event->xkey.time);
    _XmTextMovingCursorPosition(tw, XmTextGetCursorPosition(w));
    data->anchor = 0;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/*
 * This Action Proc deselects all of the text.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeselectAll( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
DeselectAll(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XmTextPosition cursorPos = XmTextGetCursorPosition(w);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    _XmTextSetDestinationSelection(w, cursorPos, False, event->xkey.time);
    data->stype = XmSELECT_POSITION;
    (*tw->text.source->SetSelection)(tw->text.source, cursorPos,
					         cursorPos, event->xkey.time);
    _XmTextMovingCursorPosition(tw, XmTextGetCursorPosition(w));
    data->anchor = cursorPos;
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/*
 * This Action Proc replaces the primary selection with spaces
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ClearSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ClearSelection(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left, right;
    Boolean freeBlock;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (!(*tw->text.source->GetSelection)
			(tw->text.source, &left, &right)) {
        XBell(XtDisplay(tw), 0);
    } else if (left != right) {
           char *select_string = _XmStringSourceGetString(tw, left, right, 
							  False);
           XmTextBlockRec block, newblock;
           XmTextPosition cursorPos;
	   int num_spaces = right - left;
           int i;

           for(i = 0; i < num_spaces; i++) {
              if (select_string[i] != '\012') select_string[i] = ' ';
           }

	   block.ptr = select_string;
	   block.length = num_spaces;
	   block.format = XmFMT_8_BIT;
           if (_XmTextModifyVerify(tw, event, &left, &right,
                                   &cursorPos, &block, &newblock, &freeBlock)) {
	      if ((*tw->text.source->Replace)(tw, NULL, &left, &right,
					      &newblock, False) != EditDone) {
		 RingBell(w, event, params, num_params);
	      } else {
		_XmTextSetDestinationSelection(w, tw->text.cursor_position,
					       False, event->xkey.time);
		_XmTextValueChanged(tw, event);
	      }
              if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
           } else {
	      RingBell(w, event, params, num_params);
	   }
           XtFree(select_string);
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static void 
#ifdef _NO_PROTO
ProcessBDragRelease( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessBDragRelease(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;
    XButtonEvent	*ev = (XButtonEvent *) event;

   /* Work around for intrinsic bug.  Remove once bug is fixed. */
    XtUngrabPointer(w, ev->time);

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (data->sel_start) {
       if (dragged(data->Sel2Hint, event, data->threshold)) {
	  if (data->Sel2Extending) {
	    /*
	     * Secondary selection:
	     * Causes the converter to perform a delete action of the
	     * secondary selection when the Convert routine is called.
	     */
	     ExtendSecondaryEnd(w, event, params, num_params);
	  } else {
	    /* Not a drag action, not secondary selection, not Quick transfer.
	     * At least we have to ungrab the keyboard...
	     */
	    if (!data->cancel) XtUngrabKeyboard(w, CurrentTime);
	  }
       } else {
	/*
	 * Quick transfer: Copy contents of primary selection to the 
	 * stuff position found above.
	 */
	  Stuff(w, event, params, num_params);
	  if (!data->cancel) XtUngrabKeyboard(w, CurrentTime);
       }
       data->sel_start = False;
   }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/*
 * This function does a primary copy and paste on mouse button actions.
 * It copies the contents of the primary selection to the x and y
 * position of the button pressed event.
 */
static void 
#ifdef _NO_PROTO
ProcessCopy( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessCopy(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->selectionMove = False;
    ProcessBDragRelease(w, event, params, num_params);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);

    data->cancel = True;
}

/* This function does a primary cut and paste on mouse button actions. */
static void 
#ifdef _NO_PROTO
ProcessMove( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessMove(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->selectionMove = True;
    ProcessBDragRelease(w, event, params, num_params);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);

    data->cancel = True;
}


/* This function does a primary copy and paste on keyboard actions. */
static void 
#ifdef _NO_PROTO
CopyPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CopyPrimary(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->selectionMove = False;

   /* perform the primary paste action */
    Stuff(w, event, params, num_params);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* This function does a primary cut and paste on keyboard actions. */
static void 
#ifdef _NO_PROTO
CutPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CutPrimary(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    data->selectionMove = True;

   /* perform the primary paste action */
    Stuff(w, event, params, num_params);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CutClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CutClipboard(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   XmTextCut(w, event->xkey.time);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CopyClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CopyClipboard(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   XmTextCopy(w, event->xkey.time);
   _XmTextSetDestinationSelection(w, tw->text.cursor_position,
			          False, event->xkey.time);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PasteClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PasteClipboard(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;

   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
   XmTextPaste(w);
   (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}

static Boolean 
#ifdef _NO_PROTO
VerifyLeave( w, event )
        Widget w ;
        XEvent *event ;
#else
VerifyLeave(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    XmTextVerifyCallbackStruct  cbdata;

    cbdata.reason = XmCR_LOSING_FOCUS;
    cbdata.event = event;
    cbdata.doit = True;
    cbdata.currInsert = tw->text.cursor_position;
    cbdata.newInsert = tw->text.cursor_position;
    cbdata.startPos = tw->text.cursor_position;
    cbdata.endPos = tw->text.cursor_position;
    cbdata.text = NULL;
    XtCallCallbackList(w, tw->text.losing_focus_callback, (XtPointer) &cbdata);
    return(cbdata.doit);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextLeave( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TextLeave(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    if (_XmGetFocusPolicy(w) == XmPOINTER) 
       VerifyLeave(w, event);

    _XmPrimitiveLeave(w, event, params, num_params);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextFocusIn( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TextFocusIn(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    Widget dest;

    if (!event->xfocus.send_event) return;
    dest = XmGetDestination(XtDisplay(w));

    if (_XmGetFocusPolicy(w) == XmEXPLICIT && !_XmTextHasDestination(w) &&
	(!dest || _XmFindTopMostShell(dest) == _XmFindTopMostShell(w)))
       _XmTextSetDestinationSelection(w, tw->text.cursor_position,
			         False, XtLastTimestampProcessed(XtDisplay(w)));

    _XmPrimitiveFocusIn(w, event, params, num_params);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextFocusOut( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TextFocusOut(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* If traversal is on, then the leave verification callback is called in
      the traversal event handler */
    if (event->xfocus.send_event && _XmGetFocusPolicy(w) == XmEXPLICIT &&
        !tw->text.traversed) {
       (void) VerifyLeave(w, event);
    } else
       if (tw->text.traversed) tw->text.traversed = False;

    _XmPrimitiveFocusOut(w, event, params, num_params);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TraverseDown(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

  /* Find out if there is anything else to traverse to */
   /* Allow the verification routine to control the traversal */
    if (tw->primitive.navigation_type == XmNONE && VerifyLeave(w, event)) {
      tw->text.traversed = True;
      if (!_XmMgrTraversal(w, XmTRAVERSE_DOWN))
         tw->text.traversed = False;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TraverseUp(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* Allow the verification routine to control the traversal */
    if (tw->primitive.navigation_type == XmNONE && VerifyLeave(w, event)) {
       tw->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_UP))
          tw->text.traversed = False;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseHome( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TraverseHome(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* Allow the verification routine to control the traversal */
    if (tw->primitive.navigation_type == XmNONE && VerifyLeave(w, event)) {
       tw->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_HOME))
          tw->text.traversed = False;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseNextTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TraverseNextTabGroup(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* Allow the verification routine to control the traversal */
    if (VerifyLeave(w, event)) {
       tw->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP))
          tw->text.traversed = False;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraversePrevTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TraversePrevTabGroup(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

   /* Allow the verification routine to control the traversal */
    if (VerifyLeave(w, event)) {
       tw->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_PREV_TAB_GROUP))
          tw->text.traversed = False;
    }
}

#ifdef CDE_INTEGRATE

#define SELECTION_ACTION	0
#define TRANSFER_ACTION		1

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessPress( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ProcessPress(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   /*  This action happens when Button1 is pressed and the Selection
       and Transfer are integrated on Button1.  It is passed two
       parameters: the action to call when the event is not on the
       selection, and the action to call when the event is on the
       selection.  */

   if (*num_params != 2 || !XmIsText(w))
      return;
   if (XmTestInSelection((XmTextWidget)w, event))
      XtCallActionProc(w, params[TRANSFER_ACTION], event, params, *num_params);
   else
      XtCallActionProc(w, params[SELECTION_ACTION], event, params, *num_params);
}

/* ARGSUSED */
static Bool
#ifdef _NO_PROTO
LookForButton (display, event, arg )
	Display * display;
	XEvent * event;
	XPointer arg;
#else
LookForButton (
	Display * display,
	XEvent * event,
	XPointer arg)
#endif /* _NO_PROTO */
{

#define DAMPING	5
#define ABS_DELTA(x1, x2) (x1 < x2 ? x2 - x1 : x1 - x2)

    if( event->type == MotionNotify)  {
        XEvent * press = (XEvent *) arg;

        if (ABS_DELTA(press->xbutton.x_root, event->xmotion.x_root) > DAMPING ||
            ABS_DELTA(press->xbutton.y_root, event->xmotion.y_root) > DAMPING)
	    return(True);
    }
    else if (event->type == ButtonRelease)
        return(True);
    return(False);

#undef DAMPING
#undef ABS_DELTA
}
	

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
XmTestInSelection( w, event )
        XmTextWidget w ;
        XEvent *event ;
#else
XmTestInSelection(
        XmTextWidget w,
	XEvent *event )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition position, left, right;
    Position left_x, left_y, right_x, right_y;

    position = (*tw->text.output->XYToPos)(tw, event->xbutton.x,
                                           event->xbutton.y);

    /* Check if we have the selection */
    if (!(*tw->text.source->GetSelection)(tw->text.source, &left, &right))
	return(False);
    /* If the right == the left then no characters are selected */
    if (right == left) 
	return(False);
    /* Is the new position inside the selection ? */
    if (!(position >= left && position <= right))
	return(False);
    /* or if it is part of a multiclick sequence */
    if (event->xbutton.time > tw->text.input->data->lasttime &&
	     event->xbutton.time - tw->text.input->data->lasttime <
              XtGetMultiClickTime(XtDisplay((Widget)w)))
	return(False);
    {
	/* The determination of whether this is a transfer drag cannot be made
	   until a Motion event comes in.  It is not a drag as soon as a
	   ButtonUp event happens or the MultiClickTimeout expires. */
        XEvent new;

        XPeekIfEvent(XtDisplay(w), &new, LookForButton, (XPointer)event);
        switch (new.type)  {
            case MotionNotify:
               return(True);  
               break;
            case ButtonRelease:
               return(False);  
               break;
        }
	return(False);
    }
}

#endif /* CDE_INTEGRATE */
/***************************************************************************
 * Functions to process text tw in multi-line edit mode versus single  *
 * line edit mode.                                                         *
 ***************************************************************************/

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessCancel( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessCancel(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;
    InputData data = tw->text.input->data;

    XmParentInputActionRec  p_event ;

    data->cancel = False;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_CANCEL ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    if (data->hasSel2 && data->Sel2Extending) {
       data->cancel = True;
       _XmTextSetSel2(tw, 1, 0, event->xkey.time);
       XtUngrabKeyboard(w, CurrentTime);
    }

    if (_XmStringSourceHasSelection(tw->text.source) && data->extending) {
       data->cancel = True;
      /* reset origLeft and origRight */
       (*tw->text.source->SetSelection)(tw->text.source, data->origLeft,
					data->origRight, event->xkey.time);
    }

    if (!data->cancel)
       (void) _XmParentProcess(XtParent(tw), (XmParentProcessData) &p_event);

    if (data->select_id) {
       XtRemoveTimeOut(data->select_id);
       data->select_id = 0;
    }
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);

    data->cancel = True;
}

static void 
#ifdef _NO_PROTO
ProcessReturn( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessReturn(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT)
       Activate(w, event, params, num_params);
    else {
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					      off);
       InsertNewLine(w, event, params, num_params);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    }
}

static void 
#ifdef _NO_PROTO
ProcessTab( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessTab(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT || !tw->text.editable)
       TraverseNextTabGroup(w, event, params, num_params);
    else
       SelfInsert(w, event, params, num_params);
}

static void 
#ifdef _NO_PROTO
ProcessUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessUp(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmMULTI_LINE_EDIT){
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					      off);
       MovePreviousLine(w, event, params, num_params);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    } else if (w != XmGetTabGroup(w))
       TraverseUp(w, event, params, num_params);
}

static void 
#ifdef _NO_PROTO
ProcessDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessDown(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmMULTI_LINE_EDIT){
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					      off);
       MoveNextLine(w, event, params, num_params);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    } else if (w != XmGetTabGroup(w))
       TraverseDown(w, event, params, num_params);
}

static void 
#ifdef _NO_PROTO
ProcessShiftUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessShiftUp(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) {
       TraverseUp(w, event, params, num_params);
    } else {
       char *dir = "extend";
       Cardinal num = 1;
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					      off);
       _MovePreviousLine(w, event, &dir, &num, False);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    }
}

static void 
#ifdef _NO_PROTO
ProcessShiftDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessShiftDown(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    if (tw->text.edit_mode == XmSINGLE_LINE_EDIT) {
       TraverseDown(w, event, params, num_params);
    } else {
        char *dir = "extend";
        Cardinal num = 1;
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
					      off);
        _MoveNextLine(w, event, &dir, &num, False);
       (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    }
}

static void 
#ifdef _NO_PROTO
ProcessHome( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ProcessHome(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w;

    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, off);
    MoveToLineStart(w, event, params, num_params);
    (*tw->text.output->DrawInsertionPoint)( tw, tw->text.cursor_position, on);
}


static XtActionsRec ZdefaultTextActionsTable[] = {
/* Insert bindings */
  {"self-insert",		SelfInsert},
  {"insert-string",		InsertString},
/* Motion bindings */
  {"grab-focus",		DoGrabFocus},
  {"set-insertion-point",	SetCursorPosition},
  {"forward-character", 	MoveForwardChar},
  {"backward-character", 	MoveBackwardChar},
  {"forward-word", 		MoveForwardWord},
  {"backward-word", 		MoveBackwardWord},
  {"forward-paragraph", 	MoveForwardParagraph},
  {"backward-paragraph", 	MoveBackwardParagraph},
  {"beginning-of-line", 	MoveToLineStart},
  {"end-of-line", 		MoveToLineEnd},
  {"next-line", 		MoveNextLine},
  {"previous-line", 		MovePreviousLine},
  {"next-page", 		MoveNextPage},
  {"previous-page", 		MovePreviousPage},
  {"page-left", 		MovePageLeft},
  {"page-right", 		MovePageRight},
  {"beginning-of-file", 	MoveBeginningOfFile},
  {"end-of-file", 		MoveEndOfFile},
  {"move-destination",		MoveDestination},
  {"scroll-one-line-up", 	ScrollOneLineUp},
  {"scroll-one-line-down", 	ScrollOneLineDown},
  {"scroll-cursor-vertically", 	ScrollCursorVertically},
/* Delete bindings */
  {"delete-selection", 		DeleteCurrentSelection},
  {"delete-next-character", 	DeleteForwardChar},
  {"delete-previous-character",	DeleteBackwardChar},
  {"delete-next-word", 		DeleteForwardWord},
  {"delete-previous-word", 	DeleteBackwardWord},
  {"delete-to-end-of-line", 	DeleteToEndOfLine},
  {"delete-to-start-of-line",	DeleteToStartOfLine},
/* Kill bindings */
  {"kill-selection", 		KillCurrentSelection},
  {"kill-next-character", 	KillForwardChar},
  {"kill-previous-character",	KillBackwardChar},
  {"kill-next-word", 		KillForwardWord},
  {"kill-previous-word", 	KillBackwardWord},
  {"kill-to-end-of-line", 	KillToEndOfLine},
  {"kill-to-start-of-line",	KillToStartOfLine},
/* Unkill bindings */
  {"unkill", 			UnKill},
  {"stuff", 			Stuff},
/* New line bindings */
  {"newline-and-indent", 	InsertNewLineAndIndent},
  {"newline-and-backup", 	InsertNewLineAndBackup},
  {"newline",			InsertNewLine},
/* Selection bindings */
  {"select-all", 		SelectAll},
  {"deselect-all", 		DeselectAll},
  {"select-start", 		StartPrimary},
  {"quick-cut-set", 		VoidAction},
  {"quick-copy-set", 		VoidAction},
  {"do-quick-action", 		VoidAction},
  {"key-select", 		KeySelection},
  {"set-anchor", 		SetAnchor},
  {"select-adjust", 		DoSelection},
  {"select-end", 		DoSelection},
  {"extend-start", 		StartExtendSelection},
  {"extend-adjust", 		ExtendSelection},
  {"extend-end", 		ExtendEnd},
  {"set-selection-hint",	SetSelectionHint},
  {"process-bdrag",		ProcessBDrag},
  {"secondary-start",		StartSecondary},
  {"secondary-drag",		StartDrag},
  {"secondary-adjust",		ExtendSecondary},
  {"secondary-notify",          ProcessBDragRelease},
  {"clear-selection",		ClearSelection},
  {"copy-to",			ProcessCopy},
  {"move-to",			ProcessMove},
  {"copy-primary",		CopyPrimary},
  {"cut-primary",		CutPrimary},
/* Clipboard bindings */
  {"copy-clipboard",		CopyClipboard},
  {"cut-clipboard",		CutClipboard},
  {"paste-clipboard",		PasteClipboard},
/* Miscellaneous bindings */
  {"beep", 			RingBell},
  {"redraw-display", 		RedrawDisplay},
  {"activate",			Activate},
  {"toggle-overstrike",		ToggleOverstrike},
  {"toggle-add-mode",		ToggleAddMode},
  {"Help",			_XmPrimitiveHelp},
  {"enter",                     _XmPrimitiveEnter},
  {"leave",			TextLeave},
  {"focusIn",			TextFocusIn},
  {"focusOut",			TextFocusOut},
  {"unmap",			_XmPrimitiveUnmap},
/* Process multi-line and single line bindings */
  {"process-cancel",		ProcessCancel},
  {"process-return",		ProcessReturn},
  {"process-tab",		ProcessTab},
  {"process-up",		ProcessUp},
  {"process-down",		ProcessDown},
  {"process-shift-up",		ProcessShiftUp},
  {"process-shift-down",	ProcessShiftDown},
  {"process-home",		ProcessHome},
/* Traversal bindings*/
  {"traverse-next",		TraverseDown},
  {"traverse-prev",		TraverseUp},
  {"traverse-home",		TraverseHome},
  {"next-tab-group",		TraverseNextTabGroup},
  {"prev-tab-group",		TraversePrevTabGroup},
#ifdef CDE_INTEGRATE
/* Integrating selection and transfer */
  {"process-press",		ProcessPress},
#endif /*CDE_INTEGRATE */
};

externaldef(nonvisible) XtPointer _XmdefaultTextActionsTable =
					 (XtPointer) ZdefaultTextActionsTable;

externaldef(nonvisible) Cardinal _XmdefaultTextActionsTableSize = 
                                XtNumber(ZdefaultTextActionsTable);

/* added <Key> event */
#define _XmTextEventBindings1	_XmTextIn_XmTextEventBindings1
#define _XmTextEventBindings2	_XmTextIn_XmTextEventBindings2
#define _XmTextEventBindings3	_XmTextIn_XmTextEventBindings3

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Invalidate( tw, position, topos, delta )
        XmTextWidget tw ;
        XmTextPosition position ;
        XmTextPosition topos ;
        long delta ;
#else
Invalidate(
        XmTextWidget tw,
        XmTextPosition position,
        XmTextPosition topos,
        long delta )
#endif /* _NO_PROTO */
{
    InputData data = tw->text.input->data;
    if (delta == NODELTA) return; /* Just use what we have as best guess. */
    if (data->origLeft > position) data->origLeft += delta;
    if (data->origRight >= position) data->origRight += delta;
}

static void 
#ifdef _NO_PROTO
InputGetValues( wid, args, num_args )
        Widget wid ;
        ArgList args ;
        Cardinal num_args ;
#else
InputGetValues(
        Widget wid,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
        XmTextWidget tw = (XmTextWidget) wid ;
    XtGetSubvalues((XtPointer) tw->text.input->data,
		   input_resources, XtNumber(input_resources), args, num_args);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InputSetValues( oldw, reqw, new_w, args, num_args )
        Widget oldw ;
        Widget reqw ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InputSetValues(
        Widget oldw,
        Widget reqw,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) new_w ;
    InputData data = tw->text.input->data;
    XtPointer temp_ptr;

    temp_ptr = (XtPointer)data->sarray;

    XtSetSubvalues((XtPointer) data,
		   input_resources, XtNumber(input_resources), args, *num_args);
/*
 * Fix for HaL DTS 9841 - If the new selectionArray is different than the old
 *                        selectionArray, delete the old selectionArray and
 * 			  then copy the new selectionArray.
 */
    if ((XtPointer)data->sarray != temp_ptr)
    {
      XtFree((char *)temp_ptr);
      temp_ptr = (XtPointer)data->sarray;
      data->sarray = (XmTextScanType *)XtMalloc(data->sarraycount *
                      sizeof(XmTextScanType));
      memcpy((void *)data->sarray, (void *)temp_ptr, (data->sarraycount *
                sizeof(XmTextScanType)));
    }
/*
 * End Fix for HaL DTS 9841
 */
}

static void 
#ifdef _NO_PROTO
InputDestroy( w )
        Widget w ;
#else
InputDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) w ;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(tw),
			                "MOTIF_DESTINATION", False);
    Widget dest = XmGetDestination(XtDisplay(w));

    if (dest == w)
       _XmSetDestination(XtDisplay(w), NULL); 

    if (tw->core.window == XGetSelectionOwner(XtDisplay(tw),
						  MOTIF_DESTINATION))
       XtDisownSelection(w, MOTIF_DESTINATION,
			 XtLastTimestampProcessed(XtDisplay(w)));

    if (tw->core.window == XGetSelectionOwner(XtDisplay(tw),
						  XA_PRIMARY))
       XtDisownSelection(w, XA_PRIMARY, 
			 XtLastTimestampProcessed(XtDisplay(w)));

    if (tw->core.window == XGetSelectionOwner(XtDisplay(tw),
                                                  XA_SECONDARY))
       XtDisownSelection(w, XA_SECONDARY,
			 XtLastTimestampProcessed(XtDisplay(w)));

/*
 * Fix for HaL DTS 9841 - release the data for the selectionArray.
 */
    XtFree((char *)tw->text.input->data->sarray);
    XtFree((char *)tw->text.input->data);
    XtFree((char *)tw->text.input);

/*
    XmDropSiteUnregister(w);
*/
    XmImUnregister(w);
}

/* ARGSUSED */
static XtPointer
#ifdef _NO_PROTO
InputBaseProc( widget, client_data)
      Widget widget;
      XtPointer client_data;
#else
InputBaseProc(
      Widget widget,
      XtPointer client_data)
#endif /* _NO_PROTO */
{
      XmTextWidget tw = (XmTextWidget) widget;
      return (XtPointer) tw->text.input;
}


/* ARGSUSED */
void
#ifdef _NO_PROTO
_XmTextInputGetSecResData( secResDataRtn )
      XmSecondaryResourceData *secResDataRtn;
#else
_XmTextInputGetSecResData(
      XmSecondaryResourceData *secResDataRtn )
#endif /* _NO_PROTO */
{
     XmSecondaryResourceData               secResData;

     secResData = XtNew(XmSecondaryResourceDataRec);

     _XmTransformSubResources(input_resources, XtNumber(input_resources), 
                              &(secResData->resources),
                              &(secResData->num_resources));

     secResData->name = NULL;
     secResData->res_class = NULL;
     secResData->client_data = NULL;
     secResData->base_proc = InputBaseProc;
     *secResDataRtn = secResData;
}

XmTextPosition 
#ifdef _NO_PROTO
_XmTextGetAnchor( tw )
        XmTextWidget tw ;
#else
_XmTextGetAnchor(
        XmTextWidget tw )
#endif /* _NO_PROTO */
{
    InputData data = tw->text.input->data;

    return(data->anchor);
}


/* ARGSUSED */
static void
#ifdef _NO_PROTO
DropDestroyCB(w, clientData, callData)
    Widget      w;
    XtPointer   clientData;
    XtPointer   callData;
#else
DropDestroyCB(
    Widget      w,
    XtPointer   clientData,
    XtPointer   callData )
#endif /* NO_PROTO */
{
    DeleteDropContext(w);
    XtFree((char *)clientData);
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
DropTransferCallback( w, closure, seltype, type, value, length, format )
        Widget w ;
        XtPointer closure ;
        Atom *seltype ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
DropTransferCallback(
        Widget w,
        XtPointer closure,
        Atom *seltype,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    _XmTextDropTransferRec *transfer_rec = (_XmTextDropTransferRec *) closure;
    XmTextWidget tw = (XmTextWidget) transfer_rec->widget;
    InputData data = tw->text.input->data;
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    Atom CS_OF_LOCALE;
    XmTextPosition insertPosLeft, insertPosRight, left, right, cursorPos;
    XmTextBlockRec block, newblock;
    XmTextSource source = GetSrc((Widget)tw);
    int max_length = 0;
    Boolean local = _XmStringSourceHasSelection(source);
    char * total_tmp_value = NULL;
    char ** tmp_value;
    int malloc_size = 0;
    int num_vals;
    Arg args[8];
    Cardinal n, i;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status;
    Boolean pendingoff;
    Boolean freeBlock;

   /* When type = NULL, we are assuming a DELETE request has been requested */
    if (*type == XmInternAtom(XtDisplay(tw), "NULL", False)) {
       if (transfer_rec->num_chars > 0 && transfer_rec->move) {
          data->anchor = transfer_rec->insert_pos;
          cursorPos = transfer_rec->insert_pos + transfer_rec->num_chars;
          _XmTextSetCursorPosition((Widget)tw, cursorPos);
          _XmTextSetDestinationSelection((Widget)tw, tw->text.cursor_position,
                                         False, transfer_rec->timestamp);
          (*tw->text.source->SetSelection)(tw->text.source, data->anchor,
                                           tw->text.cursor_position,
                                           transfer_rec->timestamp);
	if (value) {
	   XtFree((char *)value);
	   value = NULL;
        }
        return;
      }
    }

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       CS_OF_LOCALE = tmp_prop.encoding;
    else
       CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
                              * characters.  But just in case someones
                              * Xlib is broken, this prevents a core dump.
                              */

    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

    if (!value || (*type != COMPOUND_TEXT && *type != CS_OF_LOCALE &&
                   *type != XA_STRING)) {
        n = 0;
        XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
        XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
        XtSetValues(w, args, n);
        if (value) {
	   XtFree((char *)value);
	   value = NULL;
        }
        return;
    }

    insertPosLeft = insertPosRight = transfer_rec->insert_pos;

    if (*type == XA_STRING || *type == COMPOUND_TEXT) {
       tmp_prop.value = (unsigned char *) value;
       tmp_prop.encoding = *type;
       tmp_prop.format = 8;
       tmp_prop.nitems = *length;
       num_vals = 0;

       status = XmbTextPropertyToTextList(XtDisplay(w), &tmp_prop, &tmp_value,
                                 &num_vals);

      /* if no conversion, num_vals is not changed */
       
       if (num_vals && (status == Success || status > 0)) {
          for (i = 0; i < num_vals ; i++)
              malloc_size += strlen(*tmp_value + i);

          total_tmp_value = XtMalloc ((unsigned) malloc_size + 1);
          total_tmp_value[0] = '\0';
          for (i = 0; i < num_vals ; i++)
             strcat(total_tmp_value, *tmp_value + i);
          block.ptr = total_tmp_value;
          block.length = strlen(total_tmp_value);
          XFreeStringList(tmp_value);
       } else {
	  if (value) {
	     XtFree((char *)value);
	     value = NULL;
	  }
	  return;
       }
    } else {
       block.ptr = (char *)value;
       block.length = (int) *length; /* NOTE: this causes a truncation on
					some architectures */
    }

    block.format = XmFMT_8_BIT;

    if (data->pendingdelete && 
        ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) &&
        left != right) && insertPosLeft > left && insertPosRight < right) {
       insertPosLeft = left;
       insertPosRight = right;
    }

    if (transfer_rec->move && local) {
       max_length = _XmStringSourceGetMaxLength(source);
       _XmStringSourceSetMaxLength(source, MAXINT);
    }

    pendingoff = tw->text.pendingoff;
    tw->text.pendingoff = FALSE;

    if (_XmTextModifyVerify(tw, NULL, &insertPosLeft, &insertPosRight,
                                  &cursorPos, &block, &newblock, &freeBlock)) {
       if ((*tw->text.source->Replace)(tw,  NULL, &insertPosLeft,
			      &insertPosRight, &newblock, False) != EditDone) {
	  if (tw->text.verify_bell) XBell(XtDisplay(tw), 0);
	  tw->text.pendingoff = pendingoff;
       } else {
	  transfer_rec->num_chars = _XmTextCountCharacters(newblock.ptr,
							   newblock.length);
	  if (transfer_rec->num_chars > 0 && !transfer_rec->move) {
		 _XmTextSetCursorPosition((Widget)tw, cursorPos);
		 _XmTextSetDestinationSelection((Widget)tw,
						tw->text.cursor_position,False,
						transfer_rec->timestamp);
	  }
	  if (XmTextGetSelectionPosition((Widget)tw, &left, &right)) {
	      if (transfer_rec->move && left < insertPosLeft)
		 transfer_rec->insert_pos = insertPosLeft -
					    transfer_rec->num_chars;
	      if (cursorPos < left || cursorPos > right)
		 tw->text.pendingoff = TRUE;
	  } else {
	      if (!transfer_rec->move && !tw->text.add_mode &&
		  transfer_rec->num_chars != 0)
		 data->anchor = insertPosLeft;
	  }
	  if (transfer_rec->move) {
	     XmDropTransferEntryRec transferEntries[1];
	     XmDropTransferEntryRec *transferList = NULL;

	     transferEntries[0].client_data = (XtPointer) transfer_rec;
	     transferEntries[0].target = XmInternAtom(XtDisplay(w),"DELETE",
						      False);
	     transferList = transferEntries;
	     XmDropTransferAdd(w, transferEntries, 1);
	  }

	  if (transfer_rec->move && local) {
	     _XmStringSourceSetMaxLength(source, max_length);
	  }
	  _XmTextValueChanged(tw, (XEvent *) NULL);
       }
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    } else {
       if (tw->text.verify_bell) XBell(XtDisplay(tw), 0);
       tw->text.pendingoff = pendingoff;
    }
    if (total_tmp_value) XtFree(total_tmp_value);
    XtFree((char *)value);
    value = NULL;
}

static void
#ifdef _NO_PROTO
HandleDrop(w, cb)
        Widget w;
        XmDropProcCallbackStruct *cb;
#else
HandleDrop(
        Widget w,
        XmDropProcCallbackStruct *cb )
#endif /* _NO_PROTO */
{
    static XtCallbackRec dropDestroyCB[] = { {DropDestroyCB, NULL},
                 			     {(XtCallbackProc)NULL, NULL} };
    Widget drag_cont, initiator;
    XmTextWidget tw = (XmTextWidget) w;
    Cardinal numExportTargets, n;
    Atom *exportTargets;
    Arg args[10];
    XmTextPosition insert_pos, left, right;

    drag_cont = cb->dragContext;

    n = 0;
    XtSetArg(args[n], XmNsourceWidget, &initiator); n++;
    XtSetArg(args[n], XmNexportTargets, &exportTargets); n++;
    XtSetArg(args[n], XmNnumExportTargets, &numExportTargets); n++;
    XtGetValues((Widget) drag_cont, args, n);

    insert_pos = (*tw->text.output->XYToPos)(tw, cb->x, cb->y);

    if (cb->operation & XmDROP_MOVE && w == initiator &&
        ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) &&
	 left != right && insert_pos >= left && insert_pos <= right)) {
       XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
       XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
    } else {
       XmDropTransferEntryRec transferEntries[2];
       XmDropTransferEntryRec *transferList = NULL;
       Atom TEXT = XmInternAtom(XtDisplay(w), "TEXT", False);
       Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
       Atom CS_OF_LOCALE;
       char * tmp_string = "ABC"; /* these are characters in XPCS, so... safe */
       XTextProperty tmp_prop;
       _XmTextDropTransferRec *transfer_rec;
       Cardinal numTransfers = 0;
       Boolean locale_found = False;
       Boolean c_text_found = False;
       Boolean string_found = False;
       Boolean text_found = False;
       int status;

#ifdef NON_OSF_FIX
       tmp_prop.value = NULL;
#endif
       status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
       if (status == Success)
          CS_OF_LOCALE = tmp_prop.encoding;
       else
          CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
                                 * characters.  But just in case someones
                                 * Xlib is broken, this prevents a core dump.
                                 */

       if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

      /* intialize data to send to drop transfer callback */
       transfer_rec = (_XmTextDropTransferRec *)
		       XtMalloc(sizeof(_XmTextDropTransferRec));
       transfer_rec->widget = w;
       transfer_rec->insert_pos = insert_pos;
       transfer_rec->num_chars = 0;
       transfer_rec->timestamp = cb->timeStamp;

       if (cb->operation & XmDROP_MOVE) {
          transfer_rec->move = True;
       } else {
          transfer_rec->move = False;
       }

       transferEntries[0].client_data = (XtPointer) transfer_rec;
       transferList = transferEntries;
       numTransfers = 1;

       for (n = 0; n < numExportTargets; n++) {
	 if (exportTargets[n] == CS_OF_LOCALE) {
	    transferEntries[0].target = CS_OF_LOCALE;
	    locale_found = True;
	    break;
	 }
	 if (exportTargets[n] == COMPOUND_TEXT) c_text_found = True;
	 if (exportTargets[n] == XA_STRING) string_found = True;
	 if (exportTargets[n] == TEXT) text_found = True;
       }

       n = 0;
       if (locale_found || c_text_found || string_found || text_found) {
	 if (!locale_found) {
	    if (c_text_found)
	       transferEntries[0].target = COMPOUND_TEXT;
	    else if (string_found)
	       transferEntries[0].target = XA_STRING;
	    else
	       transferEntries[0].target = TEXT;
	 }

	 if (cb->operation & XmDROP_MOVE || cb->operation & XmDROP_COPY) {
		XtSetArg(args[n], XmNdropTransfers, transferList); n++;
		XtSetArg(args[n], XmNnumDropTransfers, numTransfers); n++;
	 } else {
		XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
		XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
	 }
       } else {
	 XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
	 XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
       }
       dropDestroyCB[0].closure = (XtPointer) transfer_rec;
       XtSetArg(args[n], XmNdestroyCallback, dropDestroyCB); n++;
       XtSetArg(args[n], XmNtransferProc, DropTransferCallback); n++;
    }
    SetDropContext(w);
    XmDropTransferStart(drag_cont, args, n);
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
DragProcCallback(w, client, call)
        Widget w;
        XtPointer client;
        XtPointer call;
#else
DragProcCallback(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDragProcCallbackStruct *cb = (XmDragProcCallbackStruct *)call;
    Widget drag_cont;
    Atom targets[4];
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    Arg args[10];
    Atom *exp_targets;
    Cardinal num_exp_targets, n;
    int status = 0;

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
				      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       targets[0] = tmp_prop.encoding;
    else
       targets[0] = 99999; /* XmbTextList... should never fail for XPCS
			    * characters.  But just in case someones
			    * Xlib is broken, this prevents a core dump.
			    */

    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

    targets[1] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    targets[2] = XA_STRING;
    targets[3] = XmInternAtom(XtDisplay(w), "TEXT", False);

    drag_cont = cb->dragContext;

    n = 0;
    XtSetArg(args[n], XmNexportTargets, &exp_targets); n++;
    XtSetArg(args[n], XmNnumExportTargets, &num_exp_targets); n++;
    XtGetValues(drag_cont, args, n);

    switch(cb->reason) {
       case XmCR_DROP_SITE_ENTER_MESSAGE:
	  if (XmTargetsAreCompatible(XtDisplay(drag_cont), exp_targets,
				     num_exp_targets, targets, 4))
	    cb->dropSiteStatus = XmVALID_DROP_SITE;
	  else
	    cb->dropSiteStatus = XmINVALID_DROP_SITE;
	  break;
       case XmCR_DROP_SITE_LEAVE_MESSAGE:
       case XmCR_DROP_SITE_MOTION_MESSAGE:
       case XmCR_OPERATION_CHANGED:
	 /* we currently don't care about these message */
	  break;
       default:
         /* other messages we consider invalid */
          cb->dropSiteStatus = XmINVALID_DROP_SITE;
          break;
    }
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
DropProcCallback(w, client, call)
        Widget w;
        XtPointer client;
        XtPointer call;
#else
DropProcCallback(
        Widget w,
        XtPointer client,
        XtPointer call )
#endif /* _NO_PROTO */
{
    XmDropProcCallbackStruct *cb = (XmDropProcCallbackStruct *) call;

    if (cb->dropAction != XmDROP_HELP) {
       HandleDrop(w, cb);
    } else {
       Arg args[2];

       XtSetArg(args[0], XmNtransferStatus, XmTRANSFER_FAILURE);
       XtSetArg(args[1], XmNnumDropTransfers, 0);
       XmDropTransferStart(cb->dragContext, args, 2);
    }
}


static void
#ifdef _NO_PROTO
RegisterDropSite(w)
        Widget w ;
#else
RegisterDropSite(
        Widget w )
#endif /* _NO_PROTO */
{
    Atom targets[4];
    Arg args[10];
    int n;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status = 0;

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       targets[0] = tmp_prop.encoding;
    else
       targets[0] = 99999; /* XmbTextList... should never fail for XPCS
                            * characters.  But just in case someones
                            * Xlib is broken, this prevents a core dump.
                            */

    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);

    targets[1] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    targets[2] = XA_STRING;
    targets[3] = XmInternAtom(XtDisplay(w), "TEXT", False);

    n = 0;
    XtSetArg(args[n], XmNimportTargets, targets); n++;
    XtSetArg(args[n], XmNnumImportTargets, 4); n++;
    XtSetArg(args[n], XmNdragProc, DragProcCallback); n++;
    XtSetArg(args[n], XmNdropProc, DropProcCallback); n++;
    XmDropSiteRegister(w, args, n);
}


void 
#ifdef _NO_PROTO
_XmTextInputCreate( wid, args, num_args )
        Widget wid ;
        ArgList args ;
        Cardinal num_args ;
#else
_XmTextInputCreate(
        Widget wid,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{

    Arg im_args[6];  /* To set initial values to input method */
    Cardinal n = 0;
    XmTextWidget tw = (XmTextWidget) wid ;
    Input input;
    InputData data;
    XtPointer temp_ptr;
    OutputData o_data = tw->text.output->data;
    XPoint xmim_point;
#ifdef HP_MOTIF
    KEYBOARD_ID kid;
#endif /* HP_MOTIF */

    tw->text.input = input = (Input) XtMalloc((unsigned) sizeof(InputRec));
    input->data = data = (InputData) XtMalloc((unsigned) sizeof(InputDataRec));
    XtGetSubresources((Widget) tw->core.parent, (XtPointer)data,
		      tw->core.name, "XmText", input_resources,
		      XtNumber(input_resources), args, num_args);
    data->widget = tw;

    if (data->sarray == NULL) data->sarray = (XmTextScanType *) sarray;

    if (data->sarraycount <= 0) data->sarraycount = XtNumber(sarray);

/*
 * Fix for HaL DTS 9841 - copy the selectionArray into dedicated memory.
 */
    temp_ptr = (XtPointer)data->sarray;
    data->sarray = (XmTextScanType *)XtMalloc(data->sarraycount *
		    sizeof(XmTextScanType));
    memcpy((void *)data->sarray, (void *)temp_ptr, (data->sarraycount *
		    sizeof(XmTextScanType)));
/*
 * End fix for HaL DTS 9841
 */

    data->lasttime = 0;
    data->cancel = True;
    data->stype = data->sarray[0];
    data->extendDir = XmsdRight;
    data->extending = FALSE;
    data->sel_start = FALSE;
    data->origLeft = 0;
    data->origRight = 0;
    data->selectionHint.x = data->selectionHint.y = 0;
    data->anchor = 0;

    data->hasSel2 = FALSE;
    data->sel2Left = 0;
    data->sel2Right = 0;
    data->Sel2OrigLeft =  0;
    data->Sel2OrigRight =  0;
    data->Sel2Extending = FALSE;
    data->Sel2Hint.x = data->Sel2Hint.y = 0;
    data->select_pos_x = data->select_pos_y = 0;

    data->select_id = 0;
    data->sec_time = 0;
    data->dest_time = 0;
    data->syncing = FALSE;
    data->has_destination = FALSE;
    data->overstrike = FALSE;

    XtAddEventHandler((Widget) tw, KeyPressMask, FALSE, CheckSync, NULL);

    input->Invalidate = Invalidate;
    input->GetValues = InputGetValues;
    input->SetValues = InputSetValues;
    input->destroy = InputDestroy;


    if (tw->text.editable) {
       XmImRegister(wid, (unsigned int) NULL);

       (*tw->text.output->PosToXY)(tw, tw->text.cursor_position, &xmim_point.x, 
		                   &xmim_point.y);
       n = 0;
       XtSetArg(im_args[n], XmNfontList, o_data->fontlist); n++;
       XtSetArg(im_args[n], XmNbackground, wid->core.background_pixel); n++;
       XtSetArg(im_args[n], XmNforeground, tw->primitive.foreground); n++;
       XtSetArg(im_args[n], XmNbackgroundPixmap, 
		wid->core.background_pixmap);n++;
       XtSetArg(im_args[n], XmNspotLocation, &xmim_point); n++;
       XtSetArg(im_args[n], XmNlineSpace, o_data->lineheight); n++;
       XmImSetValues(wid, im_args, n);
    }

    RegisterDropSite(wid);

#ifdef HP_MOTIF
    kid = _XHPFindKeyboardID(XtDisplay(wid), getenv("KBD_LANG"));
    if (kid >= 0)
       XHPSetKeyboardMapping(XtDisplay(wid), kid, NULL);
#endif /* HP_MOTIF */

}

static XmTextPosition
#ifdef _NO_PROTO
XtoPosInLine( tw, x, line )
        XmTextWidget tw ;
        Position x ;
        LineNum line ;
#else
XtoPosInLine(
        XmTextWidget tw,
#if NeedWidePrototypes
        int x,
#else
        Position x,
#endif /* NeedWidePrototypes */
        LineNum line )
#endif /* _NO_PROTO */
{
        OutputData data = tw->text.output->data;
        Position        x1, y1;
        XmTextPosition  pos;

        pos = (*tw->text.output->XYToPos)
            (tw, x, line * data->lineheight + data->topmargin);

        (*tw->text.output->PosToXY)(tw, pos, &x1, &y1);
        if ( pos > 0 && x1 > x ) return pos-1;
        else
            return pos;
}



