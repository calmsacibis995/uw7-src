#pragma ident	"@(#)m1.2libs:Xm/TextF.c	1.14.1.1"
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
*  (c) Copyright 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#include <stdio.h>
#include <limits.h>  /* required for MB_LEN_MAX definition */
#include <string.h>
#include <ctype.h>
#include "XmI.h"
#include <Xm/TextFP.h>
#include <X11/Xatom.h>
#include <X11/ShellP.h>
#include <X11/VendorP.h>
#include <X11/keysym.h>
#ifdef HP_MOTIF
#include <X11/XHPlib.h>
#endif /* HP_MOTIF */
#include <Xm/AtomMgr.h>
#include <Xm/CutPaste.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropTrans.h>
#include <Xm/Display.h>
#include <Xm/ManagerP.h>
#include <Xm/ScreenP.h>
#include "MessagesI.h"
#include <Xm/TextFSelP.h>
#include <Xm/DragIconP.h>
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/XmosP.h>
#include "GMUtilsI.h"

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
#define MSG1		catgets(Xm_catd,MS_TField,MSG_TF_1,_XmMsgTextF_0000)
#define MSG2		catgets(Xm_catd,MS_TField,MSG_TF_2,_XmMsgTextF_0001)
#define MSG3		catgets(Xm_catd,MS_Text,MSG_T_7,_XmMsgTextF_0002)
#define MSG4		catgets(Xm_catd,MS_Text,MSG_T_8,_XmMsgTextF_0003)
#define MSG5		catgets(Xm_catd,MS_Text,MSG_T_9,_XmMsgTextF_0004)
#define MSG6		catgets(Xm_catd,MS_TField,MSG_TF_3,_XmMsgTextF_0005)
#define MSG7		catgets(Xm_catd,MS_TField,MSG_TF_4,_XmMsgTextF_0006)
#define WC_MSG1		catgets(Xm_catd,MS_Text,MSG_T_10,_XmMsgTextFWcs_0000)
#define WC_MSG2		catgets(Xm_catd,MS_Text,MSG_T_11,_XmMsgTextFWcs_0001)
#define GRABKBDERROR	catgets(Xm_catd,MS_CButton,MSG_CB_6,_XmMsgRowColText_0024)
#else
#define MSG1	_XmMsgTextF_0000
#define MSG2	_XmMsgTextF_0001
#define MSG3	_XmMsgTextF_0002
#define MSG4	_XmMsgTextF_0003
#define MSG5	_XmMsgTextF_0004
#define MSG6	_XmMsgTextF_0005
#define MSG7	_XmMsgTextF_0006
#define WC_MSG1	_XmMsgTextFWcs_0000
#define WC_MSG2	_XmMsgTextFWcs_0001
#define GRABKBDERROR	_XmMsgRowColText_0024
#endif


#define MAXINT 2147483647

#define TEXT_INCREMENT 32
#define PRIM_SCROLL_INTERVAL 100
#define SEC_SCROLL_INTERVAL 200
#define XmDYNAMIC_BOOL 255

#define EventBindings1	_XmTextF_EventBindings1
#define EventBindings2	_XmTextF_EventBindings2
#define EventBindings3	_XmTextF_EventBindings3
#ifdef CDE_INTEGRATE

_XmConst char _XmTextF_EventBindings_CDE[] = "\
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
_XmConst char _XmTextF_EventBindings_CDEBtn2[] = "\
<Btn2Down>:extend-start()\n\
<Btn2Motion>:extend-adjust()\n\
<Btn2Up>:extend-end()";

#define EventBindingsCDE	_XmTextF_EventBindings_CDE
#define EventBindingsCDEBtn2	_XmTextF_EventBindings_CDEBtn2

#endif /* CDE_INTEGRATE */

#define TEXT_MAX_INSERT_SIZE 64    /* Size of buffer for XLookupString. */

typedef struct {
    Boolean has_destination;
    XmTextPosition position;
    int replace_length;
    Boolean quick_key;
} TextFDestDataRec, *TextFDestData;

typedef struct {
    XmTextFieldWidget tf;
} TextFGCDataRec, *TextFGCData;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static int _XmTextFieldCountCharacters() ;
static void MakeCopy() ;
static void WcsMakeCopy() ;
static void FreeContextData() ;
static TextFDestData GetTextFDestData() ;
static void SetDropContext() ;
static void DeleteDropContext() ;
static TextFGCData GetTextFGCData() ;
static _XmHighlightRec * FindHighlight() ;
static void InsertHighlight() ;
static void TextFieldSetHighlight() ;
static Boolean GetXYFromPos() ;
static Boolean CurrentCursorState() ;
static void PaintCursor() ;
static void BlinkInsertionPoint() ;
static void HandleTimer() ;
static void ChangeBlinkBehavior() ;
static void GetRect() ;
static void CheckHasRect() ;
static void XmSetFullGC() ;
static void XmSetMarginGC() ;
static void XmResetSaveGC() ;
static void XmSetNormGC() ;
static void XmSetInvGC() ;
static void DrawText() ;
static int FindPixelLength() ;
static void DrawTextSegment() ;
static void RedisplayText() ;
static void ComputeSize() ;
static XtGeometryResult TryResize() ;
static Boolean AdjustText() ;
static void AdjustSize() ;
static Boolean ModifyVerify() ;
static void ResetClipOrigin() ;
static void InvertImageGC() ;
static void ResetImageGC() ;
static void SetCursorPosition() ;
static void VerifyBounds() ;
static XmTextPosition GetPosFromX() ;
static Boolean SetDestination() ;
static Boolean VerifyLeave() ;
static Boolean _XmTextFieldIsWordBoundary() ;
static Boolean _XmTextFieldIsWSpace() ;
static void FindWord() ;
static void FindPrevWord() ;
static void FindNextWord() ;
static void CheckDisjointSelection() ;
static Boolean NeedsPendingDelete() ;
static Boolean NeedsPendingDeleteDisjoint() ;
static Time GetServerTime() ;
static void InsertChar() ;
static void DeletePrevChar() ;
static void DeleteNextChar() ;
static void DeletePrevWord() ;
static void DeleteNextWord() ;
static void DeleteToEndOfLine() ;
static void DeleteToStartOfLine() ;
static void ProcessCancel() ;
static void Activate() ;
static void SetAnchorBalancing() ;
static void SetNavigationAnchor() ;
static void CompleteNavigation() ;
static void SimpleMovement() ;
static void BackwardChar() ;
static void ForwardChar() ;
static void BackwardWord() ;
static void ForwardWord() ;
static void EndOfLine() ;
static void BeginningOfLine() ;
static void SetSelection() ;
static void ProcessHorizontalParams() ;
static void ProcessSelectParams() ;
static void KeySelection() ;
static void TextFocusIn() ;
static void TextFocusOut() ;
static void SetScanIndex() ;
static void ExtendScanSelection() ;
static void SetScanSelection() ;
static void StartPrimary() ;
static void MoveDestination() ;
static void ExtendPrimary() ;
static void ExtendEnd() ;
static void DoExtendedSelection() ;
static void DoSecondaryExtend() ;
static void BrowseScroll() ;
static Boolean CheckTimerScrolling() ;
static void RestorePrimaryHighlight() ;
static void StartDrag() ;
static void StartSecondary() ;
static void ProcessBDrag() ;
static void ExtendSecondary() ;
static void DoStuff() ;
static void Stuff() ;
static void HandleSelectionReplies() ;
static void SecondaryNotify() ;
static void HandleTargets() ;
static void ProcessBDragRelease() ;
static void ProcessCopy() ;
static void ProcessMove() ;
static void DeleteSelection() ;
static void ClearSelection() ;
static void PageRight() ;
static void PageLeft() ;
static void CopyPrimary() ;
static void CutPrimary() ;
static void SetAnchor() ;
static void ToggleOverstrike() ;
static void ToggleAddMode() ;
static void SelectAll() ;
static void DeselectAll() ;
static void VoidAction() ;
static void CutClipboard() ;
static void CopyClipboard() ;
static void PasteClipboard() ;
static void TraverseDown() ;
static void TraverseUp() ;
static void TraverseHome() ;
static void TraverseNextTabGroup() ;
static void TraversePrevTabGroup() ;
static void TextEnter() ;
static void TextLeave() ;
static void ClassPartInitialize() ;
static void Validates() ;
static Boolean LoadFontMetrics() ;
static void ValidateString() ;
static void InitializeTextStruct() ;
static Pixmap GetClipMask() ;
static void LoadGCs() ;
static void MakeIBeamOffArea() ;
static void MakeIBeamStencil() ;
static void MakeAddModeCursor() ;
static void MakeCursors() ;
static void DropDestroyCB() ;
static void DropTransferCallback() ;
static void HandleDrop() ;
static void DragProcCallback() ;
static void DropProcCallback() ;
static void RegisterDropSite() ;
static void Initialize() ;
static void Realize() ;
static void Destroy() ;
static void Resize() ;
static XtGeometryResult QueryGeometry() ;
static void TextFieldExpose() ;
static Boolean SetValues() ;
static Boolean TextFieldGetBaselines() ;
static Boolean TextFieldGetDisplayRect() ;
static void TextFieldMarginsProc() ;
static Boolean TextFieldRemove();
#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection() ;
static void ProcessPress() ;
#endif /* CDE_INTEGRATE */

#else

static int _XmTextFieldCountCharacters( 
                        XmTextFieldWidget tf,
                        char *ptr,
                        int n_bytes) ;
static void MakeCopy( 
                        Widget w,
                        int n,
                        XtArgVal *value) ;
static void WcsMakeCopy( 
                        Widget w,
                        int n,
                        XtArgVal *value) ;
static void FreeContextData( 
                        Widget w,
                        XtPointer clientData,
                        XtPointer callData) ;
static TextFDestData GetTextFDestData( 
                        Widget w) ;
static void SetDropContext( 
                        Widget w) ;
static void DeleteDropContext( 
                        Widget w) ;
static TextFGCData GetTextFGCData( 
                        Widget w) ;
static _XmHighlightRec * FindHighlight( 
                        XmTextFieldWidget w,
                        XmTextPosition position) ;
static void InsertHighlight( 
                        XmTextFieldWidget w,
                        XmTextPosition position,
                        XmHighlightMode mode) ;
static void TextFieldSetHighlight( 
                        XmTextFieldWidget tf,
                        XmTextPosition left,
                        XmTextPosition right,
                        XmHighlightMode mode) ;
static Boolean GetXYFromPos( 
                        XmTextFieldWidget tf,
                        XmTextPosition position,
                        Position *x,
                        Position *y) ;
static Boolean CurrentCursorState( 
                        XmTextFieldWidget tf) ;
static void PaintCursor( 
                        XmTextFieldWidget tf) ;
static void BlinkInsertionPoint( 
                        XmTextFieldWidget tf) ;
static void HandleTimer( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static void ChangeBlinkBehavior( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int turn_on) ;
#else
                        Boolean turn_on) ;
#endif /* NeedWidePrototypes */
static void GetRect( 
                        XmTextFieldWidget tf,
                        XRectangle *rect) ;
static void CheckHasRect( 
                        XmTextFieldWidget tf) ;
static void XmSetFullGC( 
                        XmTextFieldWidget tf,
                        GC gc) ;
static void XmSetMarginGC( 
                        XmTextFieldWidget tf,
                        GC gc) ;
static void XmResetSaveGC( 
                        XmTextFieldWidget tf,
                        GC gc) ;
static void XmSetNormGC( 
                        XmTextFieldWidget tf,
                        GC gc,
#if NeedWidePrototypes
                        int change_stipple,
                        int stipple) ;
#else
                        Boolean change_stipple,
                        Boolean stipple) ;
#endif /* NeedWidePrototypes */
static void XmSetInvGC( 
                        XmTextFieldWidget tf,
                        GC gc) ;
static void DrawText( 
                        XmTextFieldWidget tf,
                        GC gc,
                        int x,
                        int y,
                        char *string,
                        int length) ;
static int FindPixelLength( 
                        XmTextFieldWidget tf,
                        char *string,
                        int length) ;
static void DrawTextSegment( 
                        XmTextFieldWidget tf,
                        XmHighlightMode mode,
                        XmTextPosition prev_seg_start,
                        XmTextPosition seg_start,
                        XmTextPosition seg_end,
                        XmTextPosition next_seg,
#if NeedWidePrototypes
                        int stipple,
#else
                        Boolean stipple,
#endif /* NeedWidePrototypes */
                        int y,
                        int *x) ;
static void RedisplayText( 
                        XmTextFieldWidget tf,
                        XmTextPosition start,
                        XmTextPosition end) ;
static void ComputeSize( 
                        XmTextFieldWidget tf,
                        Dimension *width,
                        Dimension *height) ;
static XtGeometryResult TryResize( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int width,
                        int height) ;
#else
                        Dimension width,
                        Dimension height) ;
#endif /* NeedWidePrototypes */
static Boolean AdjustText( 
                        XmTextFieldWidget tf,
                        XmTextPosition position,
#if NeedWidePrototypes
                        int flag) ;
#else
                        Boolean flag) ;
#endif /* NeedWidePrototypes */
static void AdjustSize( 
                        XmTextFieldWidget tf) ;
static Boolean ModifyVerify( 
                        XmTextFieldWidget tf,
                        XEvent *event,
                        XmTextPosition *replace_prev,
                        XmTextPosition *replace_next,
                        char **insert,
                        int *insert_length,
			XmTextPosition *newInsert,
			int *free_insert) ;
static void ResetClipOrigin( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int clip_mask_reset) ;
#else
                        Boolean clip_mask_reset) ;
#endif /* NeedWidePrototypes */
static void InvertImageGC( 
                        XmTextFieldWidget tf) ;
static void ResetImageGC( 
                        XmTextFieldWidget tf) ;
static void SetCursorPosition( 
                        XmTextFieldWidget tf,
                        XEvent *event,
                        XmTextPosition position,
#if NeedWidePrototypes
                        int adjust_flag,
                        int call_cb,
			int set_dest) ;
#else
                        Boolean adjust_flag,
                        Boolean call_cb,
                        Boolean set_dest) ;
#endif /* NeedWidePrototypes */
static void VerifyBounds( 
                        XmTextFieldWidget tf,
                        XmTextPosition *from,
                        XmTextPosition *to) ;
static XmTextPosition GetPosFromX( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int x) ;
#else
                        Position x) ;
#endif /* NeedWidePrototypes */
static Boolean SetDestination( 
                        Widget w,
                        XmTextPosition position,
#if NeedWidePrototypes
                        int disown,
#else
                        Boolean disown,
#endif /* NeedWidePrototypes */
                        Time set_time) ;
static Boolean VerifyLeave( 
                        XmTextFieldWidget tf,
                        XEvent *event) ;
static Boolean _XmTextFieldIsWordBoundary( 
                        XmTextFieldWidget tf,
                        XmTextPosition pos1,
                        XmTextPosition pos2) ;
static Boolean _XmTextFieldIsWSpace( 
                        wchar_t wide_char,
                        wchar_t *white_space,
                        int num_entries) ;
static void FindWord( 
                        XmTextFieldWidget tf,
                        XmTextPosition begin,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
static void FindPrevWord( 
                        XmTextFieldWidget tf,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
static void FindNextWord( 
                        XmTextFieldWidget tf,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
static void CheckDisjointSelection( 
                        Widget w,
                        XmTextPosition position,
                        Time sel_time) ;
static Boolean NeedsPendingDelete( 
                        XmTextFieldWidget tf) ;
static Boolean NeedsPendingDeleteDisjoint( 
                        XmTextFieldWidget tf) ;
static Time GetServerTime( 
                        Widget w) ;
static void InsertChar( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeletePrevChar( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeleteNextChar( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeletePrevWord( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeleteNextWord( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeleteToEndOfLine( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeleteToStartOfLine( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ProcessCancel( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void Activate( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SetAnchorBalancing( 
                        XmTextFieldWidget tf,
                        XmTextPosition position) ;
static void SetNavigationAnchor( 
                        XmTextFieldWidget tf,
                        XmTextPosition position,
#if NeedWidePrototypes
                        int extend) ;
#else
                        Boolean extend) ;
#endif /* NeedWidePrototypes */
static void CompleteNavigation( 
                        XmTextFieldWidget tf,
                        XEvent *event,
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
                        XmTextPosition cursorPos,
                        XmTextPosition position) ;
static void BackwardChar( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ForwardChar( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void BackwardWord( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ForwardWord( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void EndOfLine( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void BeginningOfLine( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SetSelection( 
                        XmTextFieldWidget tf,
                        XmTextPosition left,
                        XmTextPosition right,
#if NeedWidePrototypes
                        int redisplay) ;
#else
                        Boolean redisplay) ;
#endif /* NeedWidePrototypes */
static void ProcessHorizontalParams( 
                        Widget w,
                        XEvent *event,
                        char **params,
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
                        char **params,
                        Cardinal *num_params) ;
static void TextFocusIn( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TextFocusOut( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SetScanIndex( 
                        XmTextFieldWidget tf,
                        XEvent *event) ;
static void ExtendScanSelection( 
                        XmTextFieldWidget tf,
                        XEvent *event) ;
static void SetScanSelection( 
                        XmTextFieldWidget tf,
                        XEvent *event) ;
static void StartPrimary( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void MoveDestination( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ExtendPrimary( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ExtendEnd( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DoExtendedSelection( 
                        Widget w,
                        Time time) ;
static void DoSecondaryExtend( 
                        Widget w,
                        Time ev_time) ;
static void BrowseScroll( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static Boolean CheckTimerScrolling( 
                        Widget w,
                        XEvent *event) ;
static void RestorePrimaryHighlight( 
                        XmTextFieldWidget tf,
                        XmTextPosition prim_left,
                        XmTextPosition prim_right) ;
static void StartDrag( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void StartSecondary( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ProcessBDrag( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ExtendSecondary( 
                        Widget w,
                        XEvent *event,
                        char **params,
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
                        char **params,
                        Cardinal *num_params) ;
static void HandleSelectionReplies( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void SecondaryNotify( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void HandleTargets( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void ProcessBDragRelease( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ProcessCopy( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ProcessMove( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeleteSelection( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ClearSelection( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void PageRight( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void PageLeft( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void CopyPrimary( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void CutPrimary( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SetAnchor( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ToggleOverstrike( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void ToggleAddMode( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void SelectAll( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void DeselectAll( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void VoidAction( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void CutClipboard( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void CopyClipboard( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void PasteClipboard( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TraverseDown( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TraverseUp( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TraverseHome( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TraverseNextTabGroup( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TraversePrevTabGroup( 
                        Widget w,
                        XEvent *event,
                        char **params,
                        Cardinal *num_params) ;
static void TextEnter( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TextLeave( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ClassPartInitialize( 
                        WidgetClass w_class) ;
static void Validates( 
                        XmTextFieldWidget tf) ;
static Boolean LoadFontMetrics( 
                        XmTextFieldWidget tf) ;
static void ValidateString( 
                        XmTextFieldWidget tf,
                        char *value,
#if NeedWidePrototypes
                        int is_wchar) ;
#else
                        Boolean is_wchar) ;
#endif /* NeedWidePrototypes */
static void InitializeTextStruct( 
                        XmTextFieldWidget tf) ;
static Pixmap GetClipMask( 
                        XmTextFieldWidget tf,
                        char *pixmap_name) ;
static void LoadGCs( 
                        XmTextFieldWidget tf,
                        Pixel background,
                        Pixel foreground) ;
static void MakeIBeamOffArea( 
                        XmTextFieldWidget tf,
#if NeedWidePrototypes
                        int width,
                        int height) ;
#else
                        Dimension width,
                        Dimension height) ;
#endif /* NeedWidePrototypes */
static void MakeIBeamStencil( 
                        XmTextFieldWidget tf,
                        int line_width) ;
static void MakeAddModeCursor( 
                        XmTextFieldWidget tf,
                        int line_width) ;
static void MakeCursors( 
                        XmTextFieldWidget tf) ;
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
static void Initialize( 
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Realize( 
                        Widget w,
                        XtValueMask *valueMask,
                        XSetWindowAttributes *attributes) ;
static void Destroy( 
                        Widget wid) ;
static void Resize( 
                        Widget w) ;
static XtGeometryResult QueryGeometry( 
                        Widget w,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *reply) ;
static void TextFieldExpose( 
                        Widget w,
                        XEvent *event,
                        Region region) ;
static Boolean SetValues( 
                        Widget old,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean TextFieldGetBaselines( 
                        Widget w,
                        Dimension **baselines,
                        int *line_count) ;
static Boolean TextFieldGetDisplayRect( 
                        Widget w,
                        XRectangle *display_rect) ;
static void TextFieldMarginsProc( 
                        Widget w,
                        XmBaselineMargins *margins_rec) ;
static Boolean TextFieldRemove(Widget w,
			     XEvent *event);
#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection(
        XmTextFieldWidget w,
	XEvent *event) ;
static void ProcessPress(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params) ;
#endif /* CDE_INTEGRATE */
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/



static XmTextScanType sarray[] = {
    XmSELECT_POSITION, XmSELECT_WORD, XmSELECT_LINE
};

static int sarraysize = XtNumber(sarray);

static XContext _XmTextFDestContext = 0;
static XContext _XmTextFGCContext = 0;
static XContext _XmTextFDNDContext = 0;


/* default translations and action recs */
static XtActionsRec text_actions[] = {
/* Text Replacing Bindings */
  {"self-insert",		InsertChar},
  {"delete-previous-character",	DeletePrevChar},
  {"delete-next-character",	DeleteNextChar},
  {"delete-previous-word",	DeletePrevWord},
  {"delete-next-word",		DeleteNextWord},
  {"delete-to-end-of-line",	DeleteToEndOfLine},
  {"delete-to-start-of-line",	DeleteToStartOfLine},
/* Miscellaneous Bindings */
  {"activate",			Activate},
  {"process-cancel",		ProcessCancel},
  {"process-bdrag",		ProcessBDrag},
/* Motion Bindings */
  {"backward-character",	BackwardChar},
  {"forward-character",		ForwardChar},
  {"backward-word",		BackwardWord},
  {"forward-word",		ForwardWord},
  {"end-of-line",		EndOfLine},
  {"beginning-of-line",		BeginningOfLine},
  {"page-left",			PageLeft},
  {"page-right",		PageRight},
/* Selection Bindings */
  {"key-select",		KeySelection},
  {"grab-focus",		StartPrimary},
  {"move-destination",		MoveDestination},
  {"extend-start",		ExtendPrimary},
  {"extend-adjust",		ExtendPrimary},
  {"extend-end",		ExtendEnd},
  {"delete-selection",		DeleteSelection},
  {"clear-selection",		ClearSelection},
  {"cut-primary",		CutPrimary},
  {"copy-primary",		CopyPrimary},
  {"set-anchor",		SetAnchor},
  {"toggle-overstrike",		ToggleOverstrike},
  {"toggle-add-mode",		ToggleAddMode},
  {"select-all",		SelectAll},
  {"deselect-all",		DeselectAll},
/* Quick Cut and Paste Bindings */
  {"secondary-start",		StartSecondary},
  {"secondary-adjust",		ExtendSecondary},
  {"copy-to",			ProcessCopy},
  {"move-to",			ProcessMove},
  {"quick-cut-set",		VoidAction},
  {"quick-copy-set",		VoidAction},
  {"do-quick-action",		VoidAction},
/* Clipboard Bindings */
  {"cut-clipboard",		CutClipboard},
  {"copy-clipboard",		CopyClipboard},
  {"paste-clipboard",		PasteClipboard},
/* Traversal */
  {"traverse-next",		TraverseDown},
  {"traverse-prev",		TraverseUp},
  {"traverse-home",		TraverseHome},
  {"next-tab-group",		TraverseNextTabGroup},
  {"prev-tab-group",		TraversePrevTabGroup},
/* Focus */
  {"focusIn",			TextFocusIn},
  {"focusOut",			TextFocusOut},
  {"enter",			TextEnter},
  {"leave",			TextLeave},
#ifdef CDE_INTEGRATE
/* Integrating selection and transfer */
  {"process-press",		ProcessPress},
#endif /*CDE_INTEGRATE */
};

static XtResource resources[] =
{
    {
      XmNactivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.activate_callback),
      XmRCallback, NULL
    },

    {
      XmNlosingFocusCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.losing_focus_callback),
      XmRCallback, NULL
    },

    {
      XmNfocusCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.focus_callback),
      XmRCallback, NULL
    },

    {
      XmNmodifyVerifyCallback, XmCCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.modify_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNmodifyVerifyCallbackWcs, XmCCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.wcs_modify_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNmotionVerifyCallback, XmCCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.motion_verify_callback),
      XmRCallback, NULL
    },

    {
      XmNgainPrimaryCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.gain_primary_callback),
      XmRCallback, NULL
    },

    {
      XmNlosePrimaryCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.lose_primary_callback),
      XmRCallback, NULL
    },

    {
      XmNvalueChangedCallback, XmCCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffsetOf( struct _XmTextFieldRec, text.value_changed_callback),
      XmRCallback, NULL
    },

    {
      XmNvalue, XmCValue, XmRString, sizeof(String),
      XtOffsetOf( struct _XmTextFieldRec, text.value),
      XmRString, ""
    },

    {
      XmNvalueWcs, XmCValueWcs, XmRValueWcs, sizeof(wchar_t*),
      XtOffsetOf( struct _XmTextFieldRec, text.wc_value),
      XmRString, NULL
    },

    {
      XmNmarginHeight, XmCMarginHeight, XmRVerticalDimension,
      sizeof(Dimension),
      XtOffsetOf( struct _XmTextFieldRec, text.margin_height),
      XmRImmediate, (XtPointer) 5
    },

    {
      XmNmarginWidth, XmCMarginWidth, XmRHorizontalDimension,
      sizeof(Dimension),
      XtOffsetOf( struct _XmTextFieldRec, text.margin_width),
      XmRImmediate, (XtPointer) 5
    },

    {
      XmNcursorPosition, XmCCursorPosition, XmRTextPosition,
      sizeof (XmTextPosition),
      XtOffsetOf( struct _XmTextFieldRec, text.cursor_position),
      XmRImmediate, (XtPointer) 0
    },

    {
      XmNcolumns, XmCColumns, XmRShort, sizeof(short),
      XtOffsetOf( struct _XmTextFieldRec, text.columns),
      XmRImmediate, (XtPointer) 20
    },

    {
      XmNmaxLength, XmCMaxLength, XmRInt, sizeof(int),
      XtOffsetOf( struct _XmTextFieldRec, text.max_length),
      XmRImmediate, (XtPointer) MAXINT
    },

    {
      XmNblinkRate, XmCBlinkRate, XmRInt, sizeof(int),
      XtOffsetOf( struct _XmTextFieldRec, text.blink_rate),
      XmRImmediate, (XtPointer) 500
    },

    {
      XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
      XtOffsetOf( struct _XmTextFieldRec, text.font_list),
      XmRString, NULL
    },

    {
      XmNselectionArray, XmCSelectionArray, XmRPointer,
      sizeof(XtPointer),
      XtOffsetOf( struct _XmTextFieldRec, text.selection_array),
      XmRImmediate, (XtPointer) sarray
    },

    {
      XmNselectionArrayCount, XmCSelectionArrayCount, XmRInt, sizeof(int),
      XtOffsetOf( struct _XmTextFieldRec, text.selection_array_count),
      XmRInt, (XtPointer) &sarraysize
    },

    {
      XmNresizeWidth, XmCResizeWidth, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _XmTextFieldRec, text.resize_width),
      XmRImmediate, (XtPointer) False
    },

    {
      XmNpendingDelete, XmCPendingDelete, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _XmTextFieldRec, text.pending_delete),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNeditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffsetOf( struct _XmTextFieldRec, text.editable),
      XmRImmediate, (XtPointer) True
    },

    {
      XmNcursorPositionVisible, XmCCursorPositionVisible, XmRBoolean,
      sizeof(Boolean),
      XtOffsetOf( struct _XmTextFieldRec, text.cursor_position_visible),
      XmRImmediate, (XtPointer) True
    },

   {
     XmNverifyBell, XmCVerifyBell, XmRBoolean, sizeof(Boolean),
     XtOffsetOf( struct _XmTextFieldRec, text.verify_bell),
     XmRImmediate, (XtPointer) XmDYNAMIC_BOOL
   },

   {
     XmNselectThreshold, XmCSelectThreshold, XmRInt, sizeof(int),
     XtOffsetOf( struct _XmTextFieldRec, text.threshold),
     XmRImmediate, (XtPointer) 5
   },

   {
     XmNnavigationType, XmCNavigationType, XmRNavigationType,
     sizeof (unsigned char),
     XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
     XmRImmediate, (XtPointer) XmTAB_GROUP
   },
};

/* Definition for resources that need special processing in get values */

static XmSyntheticResource syn_resources[] =
{
   {
     XmNmarginWidth,
     sizeof(Dimension),
     XtOffsetOf( struct _XmTextFieldRec, text.margin_width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels
   },

   {
     XmNmarginHeight,
     sizeof(Dimension),
     XtOffsetOf( struct _XmTextFieldRec, text.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels
   },

   {
     XmNvalue,
     sizeof(char *),
     XtOffsetOf( struct _XmTextFieldRec, text.value),
     MakeCopy,
   },

   {
     XmNvalueWcs,
     sizeof(wchar_t *),
     XtOffsetOf( struct _XmTextFieldRec, text.wc_value),
     WcsMakeCopy,
   },

};

XmPrimitiveClassExtRec _XmTextFPrimClassExtRec = {
    NULL,
    NULLQUARK,
    XmPrimitiveClassExtVersion,
    sizeof(XmPrimitiveClassExtRec),
    TextFieldGetBaselines,                  /* widget_baseline */
    TextFieldGetDisplayRect,               /* widget_display_rect */
    TextFieldMarginsProc,                  /* get/set widget margins */
};


externaldef(xmtextfieldclassrec) XmTextFieldClassRec xmTextFieldClassRec =
{
   {
      (WidgetClass) &xmPrimitiveClassRec,	/* superclass         */
      "XmTextField",				/* class_name         */
      sizeof(XmTextFieldRec),		        /* widget_size        */
      (XtProc)NULL,				/* class_initialize   */
      ClassPartInitialize,			/* class_part_initiali*/
      FALSE,					/* class_inited       */
      Initialize,				/* initialize         */
      (XtArgsProc)NULL,				/* initialize_hook    */
      Realize,					/* realize            */
      text_actions,				/* actions            */
      XtNumber(text_actions),			/* num_actions        */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      XtExposeCompressMaximal,			/* compress_exposure  */
      TRUE,					/* compress_enterleave*/
      FALSE,					/* visible_interest   */
      Destroy,					/* destroy            */
      Resize,					/* resize             */
      TextFieldExpose,				/* expose             */
      SetValues,				/* set_values         */
      (XtArgsFunc)NULL,				/* set_values_hook    */
      XtInheritSetValuesAlmost,			/* set_values_almost  */
      (XtArgsProc)NULL,				/* get_values_hook    */
      (XtAcceptFocusProc)NULL,			/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      NULL,					/* tm_table           */
      QueryGeometry,				/* query_geometry     */
      (XtStringProc)NULL,			/* display accel      */
      NULL,					/* extension          */
   },

   {  	                          		/* Xmprimitive        */
      XmInheritBorderHighlight,        		/* border_highlight   */
      XmInheritBorderUnhighlight,              	/* border_unhighlight */
      NULL,					/* translations	      */
      (XtActionProc)NULL,             		/* arm_and_activate   */
      syn_resources,            		/* syn resources      */
      XtNumber(syn_resources),  		/* num syn_resources  */
      (XtPointer) &_XmTextFPrimClassExtRec,	/* extension          */
   },

   {                            		/* text class s */
      NULL,                     		/* extension              */
   }
};

externaldef(xmtextfieldwidgetclass) WidgetClass xmTextFieldWidgetClass =
					 (WidgetClass) &xmTextFieldClassRec;

/* USE ITERATIONS OF mblen TO COUNT THE NUMBER OF CHARACTERS REPRESENTED
 * BY n_bytes BYTES POINTED TO BY ptr, a pointer to char*.
 * n_bytes does not include NULL terminator (if any), nor does return.
 */
/* ARGSUSED */
static int
#ifdef _NO_PROTO
_XmTextFieldCountCharacters( tf, ptr, n_bytes )
        XmTextFieldWidget tf ;
        char *ptr ;
        int n_bytes ;
#else
_XmTextFieldCountCharacters(
        XmTextFieldWidget tf,
        char *ptr,
        int n_bytes )
#endif /* _NO_PROTO */
{
   char * bptr;
   int count = 0;
   int char_size = 0;

   if (n_bytes <= 0 || ptr == NULL || *ptr == '\0')
      return 0;

   if (tf->text.max_char_size == 1)
      return n_bytes;

   bptr = ptr;

   for (bptr = ptr; n_bytes > 0; count++, bptr+= char_size){
      char_size = mblen(bptr, tf->text.max_char_size);
      if (char_size < 0) break; /* error */
      n_bytes -= char_size;
   }
   return count;
}

/* USE ITERATIONS OF wctomb TO COUNT THE NUMBER OF BYTES REQUIRED FOR THE
 * MULTI-BYTE REPRESENTION OF num_chars WIDE CHARACTERS IN wc_value.
 * COUNT TERMINATED IF NULL ENCOUNTERED IN THE STRING.
 * NUMBER OF BYTES IS RETURNED.
 */
/* ARGSUSED */
int
#ifdef _NO_PROTO
_XmTextFieldCountBytes( tf, wc_value, num_chars )
	XmTextFieldWidget tf;
	wchar_t * wc_value; 
	int num_chars;
#else /* _NO_PROTO */
_XmTextFieldCountBytes(
	XmTextFieldWidget tf,
	wchar_t * wc_value, 
	int num_chars)
#endif /* _NO_PROTO */
{
   wchar_t 	* wc_ptr;
   char 	tmp[MB_LEN_MAX];  /* defined in limits.h: max in any locale */
   int 		n_bytes = 0;
   int          n_bytes_per_char = 0;

   if (num_chars <= 0 || wc_value == NULL || *wc_value == (wchar_t)0L)
      return 0;

   if (tf->text.max_char_size == 1)
      return num_chars;

   wc_ptr = wc_value;
   while ((num_chars > 0) && (*wc_ptr != (wchar_t)0L)){
      n_bytes_per_char = wctomb(tmp, *wc_ptr);
      if (n_bytes_per_char > 0 )
	 n_bytes += n_bytes_per_char;
      num_chars--;
      wc_ptr++;
   }
   return n_bytes;
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
MakeCopy( w, n, value )
        Widget w ;
	int n;
        XtArgVal *value ;
#else
MakeCopy(
        Widget w,
	int n,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    (*value) = (XtArgVal) XmTextFieldGetString (w);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
WcsMakeCopy( w, n, value )
        Widget w ;
	int n;
        XtArgVal *value ;
#else
WcsMakeCopy(
        Widget w,
	int n,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    (*value) = (XtArgVal) XmTextFieldGetStringWcs (w);
}

static void
#ifdef _NO_PROTO
FreeContextData(w, clientData, callData)
    Widget w;
    XtPointer clientData;
    XtPointer callData;
#else
FreeContextData( 
	Widget w,
	XtPointer clientData,
	XtPointer callData )
#endif /* _NO_PROTO */
{
    XmTextContextData ctx_data = (XmTextContextData) clientData;
    Display *display = DisplayOfScreen(ctx_data->screen);
    XtPointer data_ptr;

    if (XFindContext(display, (Window) ctx_data->screen,
                     ctx_data->context, (char **) &data_ptr)) {

       if (ctx_data->type == _XM_IS_PIXMAP_CTX) {
          XFreePixmap(display, (Pixmap) data_ptr);
       } else if (ctx_data->type != '\0') {
          if (data_ptr)
             XtFree((char *) data_ptr);
       }

       XDeleteContext (display, (Window) ctx_data->screen, ctx_data->context);
    }

    XtFree ((char *) ctx_data);
}

static TextFDestData 
#ifdef _NO_PROTO
GetTextFDestData( w )
        Widget w ;
#else
GetTextFDestData(
        Widget w )
#endif /* _NO_PROTO */
{
   static TextFDestData dest_data;
   Display *display = XtDisplay(w);
   Screen *screen = XtScreen(w);

   if (_XmTextFDestContext == 0)
      _XmTextFDestContext = XUniqueContext();

   if (XFindContext(display, (Window) screen,
                    _XmTextFDestContext, (char **) &dest_data)) {
       XmTextContextData ctx_data;
       Widget xm_display = (Widget) XmGetXmDisplay(display);

       ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

       ctx_data->screen = screen;
       ctx_data->context = _XmTextFDestContext;
       ctx_data->type = _XM_IS_DEST_CTX;

       dest_data = (TextFDestData) XtCalloc(1, sizeof(TextFDestDataRec));

       XtAddCallback(xm_display, XmNdestroyCallback, 
                     (XtCallbackProc) FreeContextData, (XtPointer) ctx_data);

       XSaveContext(XtDisplay(w), (Window) screen,
                    _XmTextFDestContext, (XPointer)dest_data);
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
   Screen *screen = XtScreen(w);

   if (_XmTextFDNDContext == 0)
      _XmTextFDNDContext = XUniqueContext();

   XSaveContext(display, (Window)screen,
                _XmTextFDNDContext, (XPointer)w);
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
   Screen *screen = XtScreen(w);

   XDeleteContext(display, (Window)screen, _XmTextFDNDContext);
}


Widget
#ifdef _NO_PROTO
_XmTextFieldGetDropReciever( w )
        Widget w ;
#else
_XmTextFieldGetDropReciever(
        Widget w )
#endif /* _NO_PROTO */
{
   Widget widget;

   if (_XmTextFDNDContext == 0) return NULL;

   if (!XFindContext(XtDisplay(w), (Window) XtScreen(w),
                     _XmTextFDNDContext, (char **) &widget)) {
      return widget;
   } 

   return NULL;
}



static TextFGCData 
#ifdef _NO_PROTO
GetTextFGCData( w )
        Widget w ;
#else
GetTextFGCData(
        Widget w )
#endif /* _NO_PROTO */
{
   static TextFGCData gc_data;
   Display *display = XtDisplay(w);
   Screen *screen = XtScreen(w);

   if (_XmTextFGCContext == 0)
      _XmTextFGCContext = XUniqueContext();

   if (XFindContext(display, (Window)screen,
                    _XmTextFGCContext, (char **)&gc_data)) {
       XmTextContextData ctx_data;
       Widget xm_display = (Widget) XmGetXmDisplay(display);

       ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

       ctx_data->screen = screen;
       ctx_data->context = _XmTextFGCContext;
       ctx_data->type = _XM_IS_GC_DATA_CTX;

       gc_data = (TextFGCData) XtCalloc(1, sizeof(TextFGCDataRec));

       XtAddCallback(xm_display, XmNdestroyCallback, 
                     (XtCallbackProc) FreeContextData, (XtPointer) ctx_data);

       XSaveContext(display, (Window)screen, _XmTextFGCContext, 
		    (XPointer)gc_data);
       gc_data->tf = (XmTextFieldWidget) w;
   }

   if (gc_data->tf == NULL) gc_data->tf = (XmTextFieldWidget) w;

   return gc_data;
}

void
#ifdef _NO_PROTO
_XmTextFToggleCursorGC( widget )
        Widget widget;
#else
_XmTextFToggleCursorGC(
        Widget widget )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) widget;
    XGCValues values;
    unsigned long valuemask = GCFillStyle|GCFunction|GCForeground|GCBackground;

    if (!XtIsRealized(widget)) return;

    if (tf->text.overstrike) {
      if (!tf->text.add_mode && XtIsSensitive(widget) &&
	  (tf->text.has_focus || tf->text.has_destination)) {
	values.fill_style = FillSolid;
      } else {
	values.fill_style = FillTiled;
      }
      values.foreground = values.background =
	tf->primitive.foreground ^ tf->core.background_pixel;
      values.function = GXxor;
    } else {
      if (XtIsSensitive(widget) && !tf->text.add_mode &&
	  (tf->text.has_focus || tf->text.has_destination)) {
	if (tf->text.cursor == XmUNSPECIFIED_PIXMAP) return;
	values.stipple = tf->text.cursor;
      } else {
	if (tf->text.add_mode_cursor == XmUNSPECIFIED_PIXMAP) return;
	values.stipple = tf->text.add_mode_cursor;
      }
      values.fill_style = FillStippled;
      values.function = GXcopy;
      if (tf->text.have_inverted_image_gc){
	values.background = tf->primitive.foreground;
	values.foreground = tf->core.background_pixel;
      } else {
	values.foreground = tf->primitive.foreground;
	values.background = tf->core.background_pixel;
      }
      valuemask |= GCStipple;
    }
    XChangeGC(XtDisplay(widget), tf->text.image_gc, valuemask, &values);
}

/*
 * Find the highlight record corresponding to the given position.  Returns a
 * pointer to the record.  The third argument indicates whether we are probing
 * the left or right edge of a highlighting range.
 */
static _XmHighlightRec * 
#ifdef _NO_PROTO
FindHighlight( w, position )
        XmTextFieldWidget w ;
        XmTextPosition position ;
#else
FindHighlight(
        XmTextFieldWidget w,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    _XmHighlightRec *l = w->text.highlight.list;
    int i;

    for (i=w->text.highlight.number - 1 ; i>=0 ; i--)
        if (position >= l[i].position) {
            l = l + i;
            break;
        }

    return(l);
}

static void 
#ifdef _NO_PROTO
InsertHighlight( w, position, mode )
        XmTextFieldWidget w ;
        XmTextPosition position ;
        XmHighlightMode mode ;
#else
InsertHighlight(
        XmTextFieldWidget w,
        XmTextPosition position,
        XmHighlightMode mode )
#endif /* _NO_PROTO */
{
    _XmHighlightRec *l1;
    _XmHighlightRec *l = w->text.highlight.list;
    int i, j;

    l1 = FindHighlight(w, position);
    if (l1->position == position)
       l1->mode = mode;
    else {
       i = (l1 - l) + 1;
       w->text.highlight.number++;
       if (w->text.highlight.number > w->text.highlight.maximum) {
          w->text.highlight.maximum = w->text.highlight.number;
          l = w->text.highlight.list = (_XmHighlightRec *)XtRealloc((char *) l,
              (unsigned)(w->text.highlight.maximum * sizeof(_XmHighlightRec)));
       }
       for (j=w->text.highlight.number-1 ; j>i ; j--)
           l[j] = l[j-1];
       l[i].position = position;
       l[i].mode = mode;
    }
}

static void 
#ifdef _NO_PROTO
TextFieldSetHighlight( tf, left, right, mode )
        XmTextFieldWidget tf ;
        XmTextPosition left ;
        XmTextPosition right ;
        XmHighlightMode mode ;
#else
TextFieldSetHighlight(
        XmTextFieldWidget tf,
        XmTextPosition left,
        XmTextPosition right,
        XmHighlightMode mode )
#endif /* _NO_PROTO */
{
    _XmHighlightRec *l;
    XmHighlightMode endmode;
    int i, j;

    if (left >= right || right <= 0) return;

    _XmTextFieldDrawInsertionPoint(tf, False);
    endmode = FindHighlight(tf, right)->mode;
    InsertHighlight(tf, left, mode);
    InsertHighlight(tf, right, endmode);
    l = tf->text.highlight.list;
    i = 1;
    while (i < tf->text.highlight.number) {
        if (l[i].position >= left && l[i].position < right)
            l[i].mode = mode;
        if (l[i].mode == l[i-1].mode) {
            tf->text.highlight.number--;
            for (j=i ; j<tf->text.highlight.number ; j++)
                l[j] = l[j+1];
        } else i++;
    }
    if (TextF_CursorPosition(tf) > left && TextF_CursorPosition(tf) < right){
       if (mode == XmHIGHLIGHT_SELECTED){
          InvertImageGC(tf);
       } else if (mode != XmHIGHLIGHT_SELECTED){
          ResetImageGC(tf);
       }
    }
    tf->text.refresh_ibeam_off = True;
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/*
 * Get x and y based on position.
 */
static Boolean 
#ifdef _NO_PROTO
GetXYFromPos( tf, position, x, y )
        XmTextFieldWidget tf ;
        XmTextPosition position ;
        Position *x ;
        Position *y ;
#else
GetXYFromPos(
        XmTextFieldWidget tf,
        XmTextPosition position,
        Position *x,
        Position *y )
#endif /* _NO_PROTO */
{
   /* initialize the x and y positions to zero */
    *x = 0;
    *y = 0;

    if (position > tf->text.string_length) return False;

    if (tf->text.max_char_size != 1) {
       *x += FindPixelLength(tf, (char*)TextF_WcValue(tf), (int)position);
    } else {
       *x += FindPixelLength(tf, TextF_Value(tf), (int)position);
    }

    *y += tf->primitive.highlight_thickness + tf->primitive.shadow_thickness
	  + tf->text.margin_top + TextF_FontAscent(tf);
    *x += (Position) tf->text.h_offset;

    return True;
}

static Boolean 
#ifdef _NO_PROTO
CurrentCursorState( tf )
        XmTextFieldWidget tf ;
#else
CurrentCursorState(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
    if (tf->text.cursor_on < 0) return False;
    if (tf->text.blink_on || !XtIsSensitive(tf))
        return True;
    return False;
}

/*
 * Paint insert cursor
 */
static void 
#ifdef _NO_PROTO
PaintCursor( tf )
        XmTextFieldWidget tf ;
#else
PaintCursor(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
    Position x, y;
    XmTextPosition position;

    if (!TextF_CursorPositionVisible(tf)) return;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
    CheckHasRect(tf);

    if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

    position = TextF_CursorPosition(tf);
    (void) GetXYFromPos(tf, position, &x, &y);

    if (!tf->text.overstrike)
      x -=(tf->text.cursor_width >> 1) + 1; /* "+1" for 1 pixel left of char */
    else {
      int pxlen;
      if (tf->text.max_char_size != 1) 
	pxlen = FindPixelLength(tf, (char*)&(TextF_WcValue(tf)[position]), 1);
      else 
	pxlen = FindPixelLength(tf, &(TextF_Value(tf)[position]), 1);
      if (pxlen > tf->text.cursor_width)
	x += (pxlen - tf->text.cursor_width) >> 1;
    }
    y = (y + (Position) TextF_FontDescent(tf)) -
	 (Position) tf->text.cursor_height;

/* If time to paint the I Beam... first capture the IBeamOffArea, then draw
 * the IBeam */

    if (tf->text.refresh_ibeam_off == True){ /* get area under IBeam first */
      /* Fill is needed to realign clip rectangle with gc */
       XFillRectangle(XtDisplay((Widget)tf), XtWindow((Widget)tf),
                         tf->text.save_gc, 0, 0, 0, 0);
       XCopyArea(XtDisplay(tf), XtWindow(tf), tf->text.ibeam_off, 
		 tf->text.save_gc, x, y, tf->text.cursor_width, 
		 tf->text.cursor_height, 0, 0);
       tf->text.refresh_ibeam_off = False;
    }

    if ((tf->text.cursor_on >= 0) && tf->text.blink_on) {
       XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.image_gc, x, y,
		      tf->text.cursor_width, tf->text.cursor_height);
    } else {
       XCopyArea(XtDisplay(tf), tf->text.ibeam_off, XtWindow(tf), 
		 tf->text.save_gc, 0, 0, tf->text.cursor_width, 
		 tf->text.cursor_height, x, y);
    }
}

void 
#ifdef _NO_PROTO
_XmTextFieldDrawInsertionPoint( tf, turn_on )
        XmTextFieldWidget tf ;
        Boolean turn_on ;
#else
_XmTextFieldDrawInsertionPoint(
        XmTextFieldWidget tf,
#if NeedWidePrototypes
        int turn_on )
#else
        Boolean turn_on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    if (turn_on == True) {
       tf->text.cursor_on += 1;
       if (TextF_BlinkRate(tf) == 0 || !tf->text.has_focus)
	  tf->text.blink_on = True;
    } else {
       if (tf->text.blink_on && (tf->text.cursor_on == 0))
	  if (tf->text.blink_on == CurrentCursorState(tf) &&
	      XtIsRealized((Widget)tf)){
	     tf->text.blink_on = !tf->text.blink_on;
             PaintCursor(tf);
       }
       tf->text.cursor_on -= 1;
    }

    if (tf->text.cursor_on < 0 || !XtIsRealized((Widget) tf))
        return;

    PaintCursor(tf);
}

static void 
#ifdef _NO_PROTO
BlinkInsertionPoint( tf )
        XmTextFieldWidget tf ;
#else
BlinkInsertionPoint(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
    if ((tf->text.cursor_on >= 0) &&
	tf->text.blink_on == CurrentCursorState(tf) && XtIsRealized(tf)) {
       tf->text.blink_on = !tf->text.blink_on;
       PaintCursor(tf);
    }
}



/*
 * Handle blink on and off
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleTimer( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
HandleTimer(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) closure;

    if (TextF_BlinkRate(tf) != 0)
        tf->text.timer_id =
		 XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)tf),
				 (unsigned long)TextF_BlinkRate(tf),
                                 HandleTimer,
                                 (XtPointer) closure);
    if (tf->text.has_focus && XtIsSensitive(tf)) BlinkInsertionPoint(tf);
}


/*
 * Change state of blinking insert cursor on and off
 */
static void 
#ifdef _NO_PROTO
ChangeBlinkBehavior( tf, turn_on )
        XmTextFieldWidget tf ;
        Boolean turn_on ;
#else
ChangeBlinkBehavior(
        XmTextFieldWidget tf,
#if NeedWidePrototypes
        int turn_on )
#else
        Boolean turn_on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{

    if (turn_on) {
        if (TextF_BlinkRate(tf) != 0 && tf->text.timer_id == (XtIntervalId)0)
            tf->text.timer_id =
                XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)tf),
			        (unsigned long)TextF_BlinkRate(tf),
                                HandleTimer,
                                (XtPointer) tf);
        tf->text.blink_on = True;
    } else {
        if (tf->text.timer_id)
            XtRemoveTimeOut(tf->text.timer_id);
        tf->text.timer_id = (XtIntervalId)0;
    }
}

static void 
#ifdef _NO_PROTO
GetRect( tf, rect )
        XmTextFieldWidget tf ;
        XRectangle *rect ;
#else
GetRect(
        XmTextFieldWidget tf,
        XRectangle *rect )
#endif /* _NO_PROTO */
{
  Dimension margin_width = TextF_MarginWidth(tf) +
	                   tf->primitive.shadow_thickness +
			   tf->primitive.highlight_thickness;
  Dimension margin_top = tf->text.margin_top + tf->primitive.shadow_thickness +
			 tf->primitive.highlight_thickness;
  Dimension margin_bottom = tf->text.margin_bottom +
			    tf->primitive.shadow_thickness +
			    tf->primitive.highlight_thickness;

  if (margin_width < tf->core.width)
     rect->x = margin_width;
  else
     rect->x = tf->core.width;

  if (margin_top  < tf->core.height)
     rect->y = margin_top;
  else
     rect->y = tf->core.height;

  if ((2 * margin_width) < tf->core.width)
     rect->width = (int) tf->core.width - (2 * margin_width);
  else
     rect->width = 0;

  if ((margin_top + margin_bottom) < tf->core.height)
     rect->height = (int) tf->core.height - (margin_top + margin_bottom);
  else
     rect->height = 0;
}

static void
#ifdef _NO_PROTO
CheckHasRect( tf )
        XmTextFieldWidget tf ;
#else
CheckHasRect(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
  TextFGCData gc_data = GetTextFGCData((Widget)tf);

 /*
  * If the current global gc data record has another widget as
  * current widget, set it's has_rect to False.  Set my has_rect to
  * false also, we'll reset the has_rect when we set the actually
  * modify the GC.
  */
  if (gc_data->tf != tf) {
     if (gc_data->tf)
        gc_data->tf->text.has_rect = False;
     tf->text.has_rect = False;
  }
 /* set the current widget to my text field widget */
  gc_data->tf = tf;

}

static void 
#ifdef _NO_PROTO
XmSetFullGC( tf, gc )
        XmTextFieldWidget tf ;
        GC gc ;
#else
XmSetFullGC(
        XmTextFieldWidget tf,
        GC gc )
#endif /* _NO_PROTO */
{
  XRectangle ClipRect;

 /* adjust clip rectangle to allow the cursor to paint into the margins */
  ClipRect.x = tf->primitive.shadow_thickness +
               tf->primitive.highlight_thickness;
  ClipRect.y = tf->primitive.shadow_thickness +
               tf->primitive.highlight_thickness;
  ClipRect.width = tf->core.width - (2 * (tf->primitive.shadow_thickness +
                                          tf->primitive.highlight_thickness));
  ClipRect.height = tf->core.height - (2 * (tf->primitive.shadow_thickness +
                                           tf->primitive.highlight_thickness));

  XSetClipRectangles(XtDisplay(tf), gc, 0, 0, &ClipRect, 1,
                     Unsorted);
}

static void 
#ifdef _NO_PROTO
XmSetMarginGC( tf, gc )
        XmTextFieldWidget tf ;
        GC gc ;
#else
XmSetMarginGC(
        XmTextFieldWidget tf,
        GC gc )
#endif /* _NO_PROTO */
{
  XRectangle ClipRect;

  GetRect(tf, &ClipRect);
  XSetClipRectangles(XtDisplay(tf), gc, 0, 0, &ClipRect, 1,
                     Unsorted);
}


static void
#ifdef _NO_PROTO
XmResetSaveGC( tf, gc )
        XmTextFieldWidget tf ;
        GC gc ;
#else
XmResetSaveGC(
        XmTextFieldWidget tf,
        GC gc )
#endif /* _NO_PROTO */
{
  XRectangle ClipRect;

  ClipRect.x = 0;
  ClipRect.y = 0;
  ClipRect.width = tf->core.width;
  ClipRect.height = tf->core.height;
  XSetClipRectangles(XtDisplay(tf), gc, 0, 0, &ClipRect, 1, Unsorted);
}

/*
 * Set new clipping rectangle for text field.  This is
 * done on each focus in event since the text field widgets
 * share the same GC.
 */
void 
#ifdef _NO_PROTO
_XmTextFieldSetClipRect( tf )
        XmTextFieldWidget tf ;
#else
_XmTextFieldSetClipRect(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
  XGCValues values;
  unsigned long valuemask = (unsigned long) 0;
  

  XmSetMarginGC(tf, tf->text.gc);
  XmSetFullGC(tf, tf->text.image_gc);
  XmResetSaveGC(tf, tf->text.save_gc);

  ResetClipOrigin(tf, False);


 /* Restore cached save gc to state correct for this instantiation */
  if (tf->text.save_gc){
     valuemask = (GCFunction | GCBackground | GCForeground);
     values.function = GXcopy;
     values.foreground = tf->primitive.foreground ;
     values.background = tf->core.background_pixel;
     XChangeGC(XtDisplay(tf), tf->text.save_gc, valuemask, &values);
  }

 /* Restore cached text gc to state correct for this instantiation */

  if (tf->text.gc){
     if (!TextF_UseFontSet(tf) && (TextF_Font(tf) != NULL)){
        valuemask |= GCFont;
        values.font = TextF_Font(tf)->fid;
     }
     valuemask |= GCGraphicsExposures;
     values.graphics_exposures = (Bool) True;
     values.foreground = tf->primitive.foreground ^ tf->core.background_pixel;
     values.background = 0;
     XChangeGC(XtDisplay(tf), tf->text.gc, valuemask, &values);
  }

 /* Restore cached image gc to state correct for this instantiation */
  if (tf->text.image_gc){
     valuemask = (GCForeground | GCBackground);
     if (tf->text.overstrike) {
       values.background = values.foreground = 
	 tf->core.background_pixel ^ tf->primitive.foreground;
     } else if (tf->text.have_inverted_image_gc){
       values.background = tf->primitive.foreground;
       values.foreground = tf->core.background_pixel;
     } else {
       values.foreground = tf->primitive.foreground;
       values.background = tf->core.background_pixel;
     }
     XChangeGC(XtDisplay(tf), tf->text.image_gc, valuemask, &values);
  }

  _XmTextFToggleCursorGC((Widget)tf);

  tf->text.has_rect = True;
}

static void 
#ifdef _NO_PROTO
XmSetNormGC( tf, gc, change_stipple, stipple )
        XmTextFieldWidget tf ;
        GC gc ;
	Boolean change_stipple;
	Boolean stipple;
#else
XmSetNormGC(
        XmTextFieldWidget tf,
        GC gc,
#if NeedWidePrototypes
	int change_stipple,
        int stipple)
#else
	Boolean change_stipple,
        Boolean stipple)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    unsigned long valueMask = (GCForeground | GCBackground);
    XGCValues values;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
    CheckHasRect(tf);

    if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
    values.foreground = tf->primitive.foreground;
    values.background = tf->core.background_pixel;
    if (change_stipple) {
       valueMask |= GCTile | GCFillStyle;
       values.tile = tf->text.stipple_tile;
       if (stipple) values.fill_style = FillTiled;
       else values.fill_style = FillSolid;
    }

    XChangeGC(XtDisplay(tf), gc, valueMask, &values);
}

static void 
#ifdef _NO_PROTO
XmSetInvGC( tf, gc )
        XmTextFieldWidget tf ;
        GC gc ;
#else
XmSetInvGC(
        XmTextFieldWidget tf,
        GC gc )
#endif /* _NO_PROTO */
{
    unsigned long valueMask = (GCForeground | GCBackground);
    XGCValues values;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
    CheckHasRect(tf);

    if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
    values.foreground = tf->core.background_pixel;
    values.background = tf->primitive.foreground;

    XChangeGC(XtDisplay(tf), gc, valueMask, &values);
}

static void
#ifdef _NO_PROTO
DrawText( tf, gc, x, y, string, length )
        XmTextFieldWidget tf ;
        GC gc ;
        int x ;
        int y ;
        char * string ;
        int length ;
#else
DrawText(
        XmTextFieldWidget tf,
        GC  gc,
        int x,
        int y,
        char * string,
        int length )
#endif /* _NO_PROTO */
{
   if (TextF_UseFontSet(tf)){
      if (tf->text.max_char_size != 1) 
         XwcDrawString (XtDisplay(tf), XtWindow(tf), (XFontSet)TextF_Font(tf),
			gc, x, y, (wchar_t*) string, length);

      else  /* one byte chars */
         XmbDrawString (XtDisplay(tf), XtWindow(tf), (XFontSet)TextF_Font(tf),
			gc, x, y, string, length);

   } else { /* have a font struct, not a font set */
      if (tf->text.max_char_size != 1) { /* was passed a wchar_t*  */
	 char stack_cache[400], *tmp;
	 wchar_t tmp_wc;
	 wchar_t *wc_string = (wchar_t*)string;
	 int i, csize, num_bytes = 0;
	 /* ptr = tmp = XtMalloc((int)(length + 1)*sizeof(wchar_t)); */
	 tmp = (char *)XmStackAlloc((Cardinal) ((length + 1)*sizeof(wchar_t)),
				    stack_cache);
	 tmp_wc = wc_string[length];
	 wc_string[length] = 0L;
	 num_bytes = wcstombs(tmp, wc_string,
			      (int)((length + 1) * sizeof(wchar_t)));
	 wc_string[length] = tmp_wc;
	 if (num_bytes >= 0)
            XDrawString (XtDisplay(tf), XtWindow(tf), gc, x, y, tmp, num_bytes);
	 XmStackFree(tmp, stack_cache);
      } else /* one byte chars */
         XDrawString (XtDisplay(tf), XtWindow(tf), gc, x, y, string, length);
   }
}

static int
#ifdef _NO_PROTO
FindPixelLength( tf, string, length)
        XmTextFieldWidget tf ;
        char * string ;
        int length ;
#else
FindPixelLength(
        XmTextFieldWidget tf,
        char * string,
        int length )
#endif /* _NO_PROTO */
{
   if (TextF_UseFontSet(tf)) {
      if (tf->text.max_char_size != 1)
         return (XwcTextEscapement((XFontSet)TextF_Font(tf), 
                                   (wchar_t *) string, length));
      else /* one byte chars */
         return (XmbTextEscapement((XFontSet)TextF_Font(tf), string, length));
   } else { /* have font struct, not a font set */
      if (tf->text.max_char_size != 1) { /* was passed a wchar_t*  */
	 wchar_t *wc_string = (wchar_t*)string;
	 wchar_t wc_tmp = wc_string[length];
	 char stack_cache[400], *tmp;
	 int num_bytes, i, ret_len = 0;

	 wc_string[length] = 0L;
         tmp = (char*)XmStackAlloc((Cardinal)((length + 1) * sizeof(wchar_t)),
				   stack_cache);
         num_bytes = wcstombs(tmp, wc_string, 
			      (int)((length + 1)*sizeof(wchar_t)));
	 wc_string[length] = wc_tmp;
	 if (num_bytes >= 0)
            ret_len = XTextWidth(TextF_Font(tf), tmp, num_bytes);
         XmStackFree(tmp, stack_cache);
	 return (ret_len);
      } else /* one byte chars */
         return (XTextWidth(TextF_Font(tf), string, length));
   }
}

static void
#ifdef _NO_PROTO
DrawTextSegment( tf, mode, prev_seg_start, seg_start, seg_end, next_seg,
		 stipple, y, x)
        XmTextFieldWidget tf ;
        XmHighlightMode mode;
	XmTextPosition prev_seg_start;
	XmTextPosition seg_start;
	XmTextPosition seg_end;
	XmTextPosition next_seg;
	Boolean stipple;
        int y ;
        int *x ;
#else
DrawTextSegment(
        XmTextFieldWidget tf,
        XmHighlightMode mode,
	XmTextPosition prev_seg_start,
	XmTextPosition seg_start,
	XmTextPosition seg_end,
	XmTextPosition next_seg,
#if NeedWidePrototypes
        int stipple,
#else
        Boolean stipple,
#endif /* NeedWidePrototypes */
        int y,
        int *x)
#endif /* _NO_PROTO */
{
    int x_seg_len;

    /* update x position up to start position */

    if (tf->text.max_char_size != 1) {
       *x += FindPixelLength(tf, (char*)(TextF_WcValue(tf) + prev_seg_start),
                                 (int)(seg_start - prev_seg_start));
       x_seg_len = FindPixelLength(tf, (char*)(TextF_WcValue(tf) + seg_start),
                                           (int)seg_end - (int)seg_start);
    } else {
       *x += FindPixelLength(tf, TextF_Value(tf) + prev_seg_start,
                                 (int)(seg_start - prev_seg_start));
       x_seg_len = FindPixelLength(tf, TextF_Value(tf) + seg_start,
                                           (int)seg_end - (int)seg_start);
    }
    if (mode == XmHIGHLIGHT_SELECTED) {
      /* Draw the selected text using an inverse gc */
       XmSetNormGC(tf, tf->text.gc, False, False);
       XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.gc, *x, 
		      y - TextF_FontAscent(tf), x_seg_len,
		      TextF_FontAscent(tf) + TextF_FontDescent(tf));
       XmSetInvGC(tf, tf->text.gc);
    } else {
       XmSetInvGC(tf, tf->text.gc);
       XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.gc, *x, 
		      y - TextF_FontAscent(tf), x_seg_len,
		      TextF_FontAscent(tf) + TextF_FontDescent(tf));
       XmSetNormGC(tf, tf->text.gc, True, stipple);
    }

    if (tf->text.max_char_size != 1) {
       DrawText(tf, tf->text.gc, *x, y, (char*) (TextF_WcValue(tf) + seg_start),
                                           (int)seg_end - (int)seg_start);
    } else {
       DrawText(tf, tf->text.gc, *x, y, TextF_Value(tf) + seg_start,
                                           (int)seg_end - (int)seg_start);
    }
    if (stipple) XmSetNormGC(tf, tf->text.gc, True, !stipple);
   
    if (mode == XmHIGHLIGHT_SECONDARY_SELECTED)
       XDrawLine(XtDisplay(tf), XtWindow(tf), tf->text.gc, *x, y,
                              *x + x_seg_len - 1, y);

   /* update x position up to the next highlight position */
    if (tf->text.max_char_size != 1)
       *x += FindPixelLength(tf, (char*) (TextF_WcValue(tf) + seg_start),
				    (int)(next_seg - (int)seg_start));
    else
       *x += FindPixelLength(tf, TextF_Value(tf) + seg_start,
				    (int)(next_seg - (int)seg_start));
}


/*
 * Redisplay the new adjustments that have been made the the text
 * field widget.
 */
static void 
#ifdef _NO_PROTO
RedisplayText( tf, start, end )
        XmTextFieldWidget tf ;
        XmTextPosition start ;
        XmTextPosition end ;
#else
RedisplayText(
        XmTextFieldWidget tf,
        XmTextPosition start,
        XmTextPosition end )
#endif /* _NO_PROTO */
{
  _XmHighlightRec *l = tf->text.highlight.list;
  XRectangle rect;
  int x, y, i;
  Dimension margin_width = TextF_MarginWidth(tf) +
	                   tf->primitive.shadow_thickness +
			   tf->primitive.highlight_thickness;
  Dimension margin_top = tf->text.margin_top + tf->primitive.shadow_thickness +
			 tf->primitive.highlight_thickness;
  Dimension margin_bottom = tf->text.margin_bottom +
	                    tf->primitive.shadow_thickness +
			    tf->primitive.highlight_thickness;
  Boolean stipple = False;

  if (!XtIsRealized(tf)) return;

  if (tf->text.in_setvalues) {
     tf->text.redisplay = True;
     return;
  }

  if ((int)tf->core.width - (int)(2 * margin_width) <= 0)
    return;
  if ((int)tf->core.height - (int)(margin_top + margin_bottom) <= 0)
    return;

 /*
  * Make sure the cached GC has the clipping rectangle
  * set to the current widget.
  */
  CheckHasRect(tf);

  if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

  _XmTextFieldDrawInsertionPoint(tf, False);

 /* Get the current rectangle.
  */
  GetRect(tf, &rect);

  x = (int) tf->text.h_offset;
  y = margin_top + TextF_FontAscent(tf);

  if (!XtSensitive(tf)) stipple = True;

 /* search through the highlight array and draw the text */
  for (i = 0; i + 1 < tf->text.highlight.number; i++) {

     /* make sure start is within current highlight */
      if (l[i].position <= start && start < l[i+1].position &&
          l[i].position < end) {

         if (end > l[i+1].position) {
	    
	    DrawTextSegment(tf, l[i].mode, l[i].position, start,
			    l[i+1].position, l[i+1].position, stipple, y, &x);

           /* update start position to the next highlight position */
            start = l[i+1].position;

         } else {

	    DrawTextSegment(tf, l[i].mode, l[i].position, start,
			    end, l[i+1].position, stipple, y, &x);
            start = end;
         }
#ifdef IBM_MOTIF /* work around for compiler optimize option bug */
         if (end > l[i+1].position) {
           /* update start position to the next highlight position */
            start = l[i+1].position;
         } else {
            start = end;
         }
#endif /* IBM_MOTIF */
      } else { /* start not within current record */
         if (tf->text.max_char_size != 1)
            x += FindPixelLength(tf, (char*)(TextF_WcValue(tf) + l[i].position),
                                 (int)(l[i+1].position - l[i].position));
         else
            x += FindPixelLength(tf, TextF_Value(tf) + l[i].position,
                                 (int)(l[i+1].position - l[i].position));
     }
  }  /* end for loop */

 /* complete the drawing of the text to the end of the line */
  if (l[i].position < end) {
     DrawTextSegment(tf, l[i].mode, l[i].position, start,
		     end, tf->text.string_length, stipple, y, &x);
  } else {
     if (tf->text.max_char_size != 1)
        x += FindPixelLength(tf, (char*) (TextF_WcValue(tf) + l[i].position),
		             tf->text.string_length - (int)l[i].position);
     else 
        x += FindPixelLength(tf, TextF_Value(tf) + l[i].position,
		             tf->text.string_length - (int)l[i].position);
  }

  if (x < rect.x + rect.width) {
     XmSetInvGC(tf, tf->text.gc);
     XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.gc, x, rect.y,
                    rect.x + rect.width - x, rect.height);
  }
  tf->text.refresh_ibeam_off = True;
  _XmTextFieldDrawInsertionPoint(tf, True);
}


/*
 * Use the font along with the resources that have been set
 * to determine the height and width of the text field widget.
 */
static void 
#ifdef _NO_PROTO
ComputeSize( tf, width, height )
        XmTextFieldWidget tf ;
        Dimension *width ;
        Dimension *height ;
#else
ComputeSize(
        XmTextFieldWidget tf,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    Dimension tmp = 0;

    if (TextF_ResizeWidth(tf) &&
	TextF_Columns(tf) < tf->text.string_length){

       if (tf->text.max_char_size != 1) 
          tmp = FindPixelLength(tf, (char *)TextF_WcValue(tf),
	                        tf->text.string_length);
       else
          tmp = FindPixelLength(tf, TextF_Value(tf), tf->text.string_length);

       
       *width = tmp + (2 * (TextF_MarginWidth(tf) + 
			    tf->primitive.shadow_thickness + 
			    tf->primitive.highlight_thickness));
    } else {
	*width = TextF_Columns(tf) * tf->text.average_char_width +
	    2 * (TextF_MarginWidth(tf) + tf->primitive.shadow_thickness +
		 tf->primitive.highlight_thickness);
    }

    if (height != NULL)
	*height = TextF_FontDescent(tf) + TextF_FontAscent(tf) +
	    2 * (TextF_MarginHeight(tf) + tf->primitive.shadow_thickness +
		 tf->primitive.highlight_thickness);
}


/*
 * TryResize - Attempts to resize the width of the text field widget.
 * If the attempt fails or is ineffective, return GeometryNo.
 */
static XtGeometryResult 
#ifdef _NO_PROTO
TryResize( tf, width, height )
        XmTextFieldWidget tf ;
        Dimension width ;
        Dimension height ;
#else
TryResize(
        XmTextFieldWidget tf,
#if NeedWidePrototypes
        int width,
        int height )
#else
        Dimension width,
        Dimension height )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Dimension reswidth, resheight;
    Dimension origwidth = tf->core.width;
    XtGeometryResult result;

    result = XtMakeResizeRequest((Widget)tf, width, height,
                                 &reswidth, &resheight);

    if (result == XtGeometryAlmost) {
        result = XtMakeResizeRequest((Widget)tf, reswidth, resheight,
				     &reswidth, &resheight);

        if (reswidth == origwidth)
           result = XtGeometryNo;
        return result;
    }

   /*
    * Caution: Some geometry managers return XtGeometryYes
    *	        and don't change the widget's size.
    */
    if (tf->core.width != width && tf->core.height != height)
        result = XtGeometryNo;

    return result;
}


/*
 * Function AdjustText
 *
 * AdjustText ensures that the character at the given position is entirely
 * visible in the Text Field widget.  If the character is not already entirely
 * visible, AdjustText changes the Widget's h_offsetring appropriately.  If
 * the text must be redrawn, AdjustText calls RedisplayText.
 *
 */

static Boolean 
#ifdef _NO_PROTO
AdjustText( tf, position, flag )
        XmTextFieldWidget tf ;
        XmTextPosition position ;
        Boolean flag ;
#else
AdjustText(
        XmTextFieldWidget tf,
        XmTextPosition position,
#if NeedWidePrototypes
        int flag )
#else
        Boolean flag )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  int left_edge = 0;
  int diff;
  Dimension margin_width = TextF_MarginWidth(tf) +
			      tf->primitive.shadow_thickness +
                              tf->primitive.highlight_thickness;
  Dimension thickness    = 2 * (tf->primitive.shadow_thickness +
                              tf->primitive.highlight_thickness);
  Dimension temp;

  if (tf->text.max_char_size != 1) {
     left_edge = FindPixelLength(tf, (char *) TextF_WcValue(tf),
				 (int)position) + (int) tf->text.h_offset;
  } else {
     left_edge = FindPixelLength(tf, TextF_Value(tf), (int)position) +
		    (int) tf->text.h_offset;
  }

 /*
  * Make sure the cached GC has the clipping rectangle
  * set to the current widget.
  */
  CheckHasRect(tf);

  if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

  if ((diff = left_edge - margin_width) < 0) {
    /* We need to scroll the string to the right. */
     if (!XtIsRealized(tf)) {
       tf->text.h_offset -= diff;
       return True;
     }
     _XmTextFieldDrawInsertionPoint(tf, False);
     tf->text.h_offset -= diff;
     XmSetInvGC(tf, tf->text.gc);
     XmSetFullGC(tf, tf->text.gc);
     if (tf->core.height <= thickness)
       temp = 0;
     else
       temp = tf->core.height - thickness;
     XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.gc,
		    tf->primitive.shadow_thickness +
                    tf->primitive.highlight_thickness,
		    tf->primitive.shadow_thickness +
                    tf->primitive.highlight_thickness,
                    TextF_MarginWidth(tf),
                    temp);
     XmSetMarginGC(tf, tf->text.gc);
     RedisplayText(tf, 0, tf->text.string_length); 
     _XmTextFieldDrawInsertionPoint(tf, True);
     return True;
   } else if ((diff = ( left_edge -
		       (int)(tf->core.width - margin_width))) > 0) {
           /* We need to scroll the string to the left. */
            if (!XtIsRealized(tf)) {
              tf->text.h_offset -= diff;
              return True;
            }
            _XmTextFieldDrawInsertionPoint(tf, False);
            tf->text.h_offset -= diff;
            XmSetInvGC(tf, tf->text.gc);
            XmSetFullGC(tf, tf->text.gc);
	    if (tf->core.width <= thickness)
	      temp = 0;
	    else
	      temp = tf->core.width - thickness;
            XFillRectangle(XtDisplay(tf), XtWindow(tf), tf->text.gc,
                           tf->core.width - margin_width,
		           tf->primitive.shadow_thickness +
                           tf->primitive.highlight_thickness,
                           TextF_MarginWidth(tf),
			   temp);
             XmSetMarginGC(tf, tf->text.gc);
             RedisplayText(tf, 0, tf->text.string_length); 
             _XmTextFieldDrawInsertionPoint(tf, True);
             return True;
  }

  if (flag) RedisplayText(tf, position, tf->text.string_length); 

  return False;
}
/*
 * AdjustSize
 *
 * Adjust size will resize the text to ensure that all the text is visible.
 * It will also adjust text that is shrunk.  Shrinkage is limited to the
 * size determined by the XmNcolumns resource.
 */

static void 
#ifdef _NO_PROTO
AdjustSize( tf )
        XmTextFieldWidget tf ;
#else
AdjustSize(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
  XtGeometryResult result = XtGeometryYes;
  int left_edge = 0;
  int diff;
  Boolean redisplay = False;
  Dimension margin_width = TextF_MarginWidth(tf) +
			      tf->primitive.shadow_thickness +
                              tf->primitive.highlight_thickness;

   if (tf->text.max_char_size != 1) {
      left_edge = FindPixelLength(tf, (char *) TextF_WcValue(tf), 
				  tf->text.string_length) + margin_width;
   } else {
      left_edge = FindPixelLength(tf, TextF_Value(tf), 
				  tf->text.string_length) + margin_width;
   }

  if ((diff = (left_edge - (tf->core.width - (margin_width)))) > 0) {
     if (tf->text.in_setvalues) {
	tf->core.width += diff;
	tf->text.new_h_offset = margin_width - diff;
	return;
     }
    /* Attempt to resize.  If it doesn't succeed, do scrolling.  */
     result = TryResize(tf, tf->core.width + diff, tf->core.height);
     if (result == XtGeometryYes) {
        (* (tf->core.widget_class->core_class.resize))((Widget)tf);
	return;
     } else
       /* We need to scroll the string to the left. */
        tf->text.h_offset = margin_width - diff;
  } else {
     Dimension width;

    /* If the new size is smaller than core size, we need
     * to shrink.  Note: new size will never be less than the
     * width determined by the columns resource.
     */
     ComputeSize(tf, &width, NULL);
     if (width < tf->core.width) {
        if (tf->text.in_setvalues) {
	   tf->core.width = width;
	   return;
        }
        result = TryResize(tf, width, tf->core.height);
        if (result == XtGeometryYes) {
           (* (tf->core.widget_class->core_class.resize))((Widget)tf);
           return;
        }
     }
  }

  redisplay = AdjustText(tf, TextF_CursorPosition(tf), False);

  if (!redisplay)
     RedisplayText(tf, 0, tf->text.string_length);
}
/* If MB_CUR_MAX == 1, insert is a char* pointer; else, it is a wchar_t *
 * pointer and must be appropriately cast.  In all cases, insert_length
 * is the number of characters, not the number of bytes pointed to by
 * insert
 */

static Boolean 
#ifdef _NO_PROTO
ModifyVerify( tf, event, replace_prev, replace_next,
	      insert, insert_length, newInsert, free_insert )
        XmTextFieldWidget tf ;
        XEvent *event ;
        XmTextPosition *replace_prev ;
        XmTextPosition *replace_next ;
        char **insert ;
        int *insert_length ;
        XmTextPosition *newInsert ;
        int *free_insert ;
#else
ModifyVerify(
        XmTextFieldWidget tf,
        XEvent *event,
        XmTextPosition *replace_prev,
        XmTextPosition *replace_next,
        char **insert,
        int *insert_length,
        XmTextPosition *newInsert,
	int *free_insert )
#endif /* _NO_PROTO */
{
     XmTextVerifyCallbackStruct vcb;
     XmTextVerifyCallbackStructWcs wcs_vcb;
     XmTextBlockRec newblock;
     XmTextBlockRecWcs wcs_newblock;
     Boolean do_free = False;
     Boolean wcs_do_free = False;
     int count;
     wchar_t *wptr;
     
     *newInsert = TextF_CursorPosition(tf);
     *free_insert = (int)False;

    /* if there are no callbacks, don't waste any time... just return  True */
     if (!TextF_ModifyVerifyCallback(tf) && !TextF_ModifyVerifyCallbackWcs(tf))
	return(True);

     newblock.length = *insert_length * tf->text.max_char_size;

     if (*insert_length) {
	 if (TextF_ModifyVerifyCallback(tf)){
            newblock.ptr = (char *) XtMalloc((unsigned)
			            newblock.length + tf->text.max_char_size);
	    if (tf->text.max_char_size == 1) {
              (void)memcpy((void*)newblock.ptr, (void*)*insert,
			   newblock.length);
	      newblock.ptr[newblock.length]='\0';
	    } else {
	       count = (int) wcstombs(newblock.ptr, (wchar_t*)*insert,
				                newblock.length);
	       if (count < 0) { /* bad wchar; don't pass anything */
		  newblock.ptr[0] = '\0';
		  newblock.length = 0;
	       } else if (count == newblock.length) {
		  newblock.ptr[newblock.length] = '\0';
	       } else {
		  newblock.ptr[count] = '\0';
		  newblock.length = count;
	       }
	    }
            do_free = True;
	 } else 
            newblock.ptr = NULL;
     } else 
	newblock.ptr = NULL;
	
    /* Fill in the appropriate structs */
     vcb.reason = XmCR_MODIFYING_TEXT_VALUE;
     vcb.event = (XEvent *) event;
     vcb.doit = True;
     vcb.currInsert = TextF_CursorPosition(tf);
     vcb.newInsert = TextF_CursorPosition(tf);
     vcb.text = &newblock;
     vcb.startPos = *replace_prev;
     vcb.endPos = *replace_next;

     /* Call the modify verify callbacks. */
     if (TextF_ModifyVerifyCallback(tf))
        XtCallCallbackList((Widget) tf, TextF_ModifyVerifyCallback(tf),
			   (XtPointer) &vcb);

     if (TextF_ModifyVerifyCallbackWcs(tf) && vcb.doit){
	if (do_free){ /* there is a char* modify verify callback; the data we
		       * want is in vcb struct */
	   wcs_newblock.wcsptr = (wchar_t *) XtMalloc((unsigned) 
				      (vcb.text->length + 1) * sizeof(wchar_t));
	   wcs_newblock.length = mbstowcs(wcs_newblock.wcsptr, vcb.text->ptr,
				          vcb.text->length);
	   if (wcs_newblock.length < 0) { /* bad value; don't pass anything */
	      wcs_newblock.wcsptr[0] = 0L;
	      wcs_newblock.length = 0;
           } else 
	      wcs_newblock.wcsptr[wcs_newblock.length] = 0L;
	} else { /* there was no char* modify verify callback; use data
		  * passed in from caller instead of that in vcb struct. */
	  wcs_newblock.wcsptr = (wchar_t *) XtMalloc((unsigned) 
				       (*insert_length + 1) * sizeof(wchar_t));
	  if (tf->text.max_char_size == 1) 
	    wcs_newblock.length = mbstowcs(wcs_newblock.wcsptr, *insert,
				          *insert_length);
	  else {
	    wcs_newblock.length = *insert_length;
	    (void)memcpy((void*)wcs_newblock.wcsptr, (void*)*insert,
			 *insert_length * sizeof(wchar_t));
	  }	    
	  if (wcs_newblock.length < 0) { /* bad value; don't pass anything */
	    wcs_newblock.wcsptr[0] = 0L;
	    wcs_newblock.length = 0;
	  } else 
	    wcs_newblock.wcsptr[wcs_newblock.length] = 0L;

	}
	wcs_do_free = True;
	wcs_vcb.reason = XmCR_MODIFYING_TEXT_VALUE;
	wcs_vcb.event = (XEvent *) event;
	wcs_vcb.doit = True;
	wcs_vcb.currInsert = vcb.currInsert;
	wcs_vcb.newInsert = vcb.newInsert;
	wcs_vcb.text = &wcs_newblock;
	wcs_vcb.startPos = vcb.startPos;
	wcs_vcb.endPos = vcb.endPos;

        XtCallCallbackList((Widget) tf, TextF_ModifyVerifyCallbackWcs(tf),
			   (XtPointer) &wcs_vcb);

     }
     /* copy the newblock.ptr, length, start, and end to the pointers passed */
     if (TextF_ModifyVerifyCallbackWcs(tf)) { /* use wcs_vcb data */
        *insert_length = wcs_vcb.text->length; /* length is char count*/
	if (wcs_vcb.doit) {
	   if (tf->text.max_char_size == 1){ /* caller expects char */
	      wcs_vcb.text->wcsptr[wcs_vcb.text->length] = 0L;
	      if (*insert_length > 0) {
		 *insert = XtMalloc((unsigned) *insert_length + 1);
		 *free_insert = (int)True;
		 count = wcstombs(*insert, wcs_vcb.text->wcsptr,
				  *insert_length + 1);
		 if (count < 0) {
		    (*insert)[0] = 0;
		    *insert_length = 0;
		 }
	      }        
	   } else {  /* callback struct has wchar*; caller expects wchar* */
	      if (*insert_length > 0) {
		 *insert = 
		   XtMalloc((unsigned)(*insert_length + 1) * sizeof(wchar_t));
		 *free_insert = (int)True;
		 (void)memcpy((void*)*insert, (void*)wcs_vcb.text->wcsptr,
			       *insert_length * sizeof(wchar_t));
		 wptr = (wchar_t*) *insert;
		 wptr[*insert_length] = 0L;
	      }
	   }
	   *replace_prev = wcs_vcb.startPos;
	   *replace_next = wcs_vcb.endPos;
           *newInsert = wcs_vcb.newInsert;
	}
     } else { /* use vcb data */
	if (vcb.doit) {
	   if (tf->text.max_char_size == 1){  /* caller expects char* */
              *insert_length =  vcb.text->length;
              if (*insert_length > 0) {
		 *insert = XtMalloc((unsigned) *insert_length + 1);
		 *free_insert = (int)True;
                 (void)memcpy((void*)*insert, (void*)vcb.text->ptr,
			      *insert_length);
	         (*insert)[*insert_length] = 0;
              }
	   } else {                          /* caller expects wchar_t* back */
              *insert_length =  _XmTextFieldCountCharacters(tf, vcb.text->ptr,
							    vcb.text->length);
	      if (*insert_length > 0) {
		 *insert = 
		   XtMalloc((unsigned)(*insert_length + 1) * sizeof(wchar_t));
		 *free_insert = (int)True;
		 count = mbstowcs((wchar_t*)*insert, vcb.text->ptr,
				   *insert_length);
		 wptr = (wchar_t*) *insert;
		 if (count < 0) {
		    wptr[0] = 0L;
		    *insert_length = 0;
		 } else 
		    wptr[count] = 0L;
	      }
	   }
           *replace_prev = vcb.startPos;
           *replace_next = vcb.endPos;
           *newInsert = vcb.newInsert;
	}
     }
     if (do_free) XtFree(newblock.ptr);
     if (wcs_do_free) XtFree((char*)wcs_newblock.wcsptr);
        
    /* If doit becomes False, then don't allow the change. */
     if (TextF_ModifyVerifyCallbackWcs(tf))
        return wcs_vcb.doit;
     else
        return vcb.doit;
}
static void 
#ifdef _NO_PROTO
ResetClipOrigin(tf, clip_mask_reset)
	XmTextFieldWidget tf;
	Boolean clip_mask_reset;
#else /* _NO_PROTO */
ResetClipOrigin(
	XmTextFieldWidget tf, 
#if NeedWidePrototypes
        int clip_mask_reset)
#else
        Boolean clip_mask_reset)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   unsigned long valuemask = (GCTileStipXOrigin | GCTileStipYOrigin |
			     GCClipXOrigin | GCClipYOrigin);
   XGCValues values;
   int x, y, clip_mask_x, clip_mask_y;
   Position x_pos, y_pos;
   (void) GetXYFromPos(tf, TextF_CursorPosition(tf), &x_pos, &y_pos);

   if (!XtIsRealized((Widget)tf)) return;

   x = (int) x_pos; y = (int) y_pos;

   x -=(tf->text.cursor_width >> 1) + 1;

   clip_mask_y = y = (y + TextF_FontDescent(tf)) - tf->text.cursor_height;

   if (x < tf->primitive.highlight_thickness + 
       tf->primitive.shadow_thickness + (int)(TextF_MarginWidth(tf))){
	  clip_mask_x = tf->primitive.highlight_thickness +
          tf->primitive.shadow_thickness + (int)(TextF_MarginWidth(tf));
   } else
     clip_mask_x = x; 

   if (clip_mask_reset) {
      values.ts_x_origin = x;
      values.ts_y_origin = y;
      values.clip_x_origin = clip_mask_x;
      values.clip_y_origin = clip_mask_y;
      XChangeGC(XtDisplay(tf), tf->text.image_gc, valuemask, &values);
   }
   else 
      XSetTSOrigin(XtDisplay(tf), tf->text.image_gc, x, y);
}

static void
#ifdef _NO_PROTO
InvertImageGC (tf)
	XmTextFieldWidget tf ;
#else
InvertImageGC (
	XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
   unsigned long valuemask = (GCForeground | GCBackground);
   XGCValues values;
   Display *dpy = XtDisplay(tf);

   if (tf->text.have_inverted_image_gc) return;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

   if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

   if (!tf->text.overstrike) {
     values.background = tf->primitive.foreground;
     values.foreground = tf->core.background_pixel;
     
     XChangeGC(dpy, tf->text.image_gc, valuemask, &values);
   }

   tf->text.have_inverted_image_gc = True;
}

static void
#ifdef _NO_PROTO
ResetImageGC (tf)
        XmTextFieldWidget tf ;
#else
ResetImageGC (
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
   unsigned long valuemask = (GCForeground | GCBackground);
   XGCValues values;
   Display *dpy = XtDisplay(tf);

   if (!tf->text.have_inverted_image_gc) return;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

   if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

   if (!tf->text.overstrike) {
     values.foreground = tf->primitive.foreground;
     values.background = tf->core.background_pixel;
     
     XChangeGC(dpy, tf->text.image_gc, valuemask, &values);
   }

   tf->text.have_inverted_image_gc = False;
}



/*
 * Calls the motion verify callback.  If the doit flag is true,
 * then reset the cursor_position and call AdjustText() to
 * move the text if need be.
 */

void 
#ifdef _NO_PROTO
_XmTextFieldSetCursorPosition( tf, event, position,
			      adjust_flag, call_cb)
        XmTextFieldWidget tf ;
        XEvent *event ;
        XmTextPosition position ;
        Boolean adjust_flag ;
        Boolean call_cb ;
#else
_XmTextFieldSetCursorPosition(
        XmTextFieldWidget tf,
        XEvent *event,
        XmTextPosition position,
#if NeedWidePrototypes
        int adjust_flag,
        int call_cb)
#else
        Boolean adjust_flag,
        Boolean call_cb)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  SetCursorPosition(tf, event, position, adjust_flag, call_cb, True);
}

static void 
#ifdef _NO_PROTO
SetCursorPosition( tf, event, position,
		  adjust_flag, call_cb, set_dest)
        XmTextFieldWidget tf ;
        XEvent *event ;
        XmTextPosition position ;
        Boolean adjust_flag ;
        Boolean call_cb ;
        Boolean set_dest;
#else
SetCursorPosition(
        XmTextFieldWidget tf,
        XEvent *event,
        XmTextPosition position,
#if NeedWidePrototypes
        int adjust_flag,
        int call_cb, 
	int set_dest)
#else
        Boolean adjust_flag,
        Boolean call_cb,
        Boolean set_dest)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextVerifyCallbackStruct cb;
    Boolean flag = False;
    XPoint xmim_point;
    _XmHighlightRec *hl_list = tf->text.highlight.list;
    int i;

    if (position < 0) position = 0;

    if (position > tf->text.string_length)
       position = tf->text.string_length;

    if (TextF_CursorPosition(tf) != position && call_cb) {
      /* Call Motion Verify Callback before Cursor Changes Positon */
       cb.reason = XmCR_MOVING_INSERT_CURSOR;
       cb.event  = event;
       cb.currInsert = TextF_CursorPosition(tf);
       cb.newInsert = position;
       cb.doit = True;
       XtCallCallbackList((Widget) tf, TextF_MotionVerifyCallback(tf),
			  (XtPointer) &cb);

       if (!cb.doit) {
          if (tf->text.verify_bell) XBell(XtDisplay((Widget)tf), 0);
	  return;
       }
    }
    _XmTextFieldDrawInsertionPoint(tf, False);

    TextF_CursorPosition(tf) = position;

    if (!tf->text.add_mode && tf->text.pending_off && tf->text.has_primary) {
       SetSelection(tf, position, position, True);
       flag = True;
    }

   /* Deterimine if we need an inverted image GC or not.  Get the highlight
    * record for the cursor position.  If position is on a boundary of
    * a highlight, then we always display cursor in normal mode (i.e. set
    * normal image GC).  If position is within a selected highlight rec,
    * then make sure the image GC is inverted.  If we've moved out of a
    * selected highlight region, restore the normal image GC. */

    for (i = tf->text.highlight.number - 1; i >= 0; i--){
       if (position >= hl_list[i].position || i == 0)
	  break;
    }

    if (position == hl_list[i].position)
       ResetImageGC(tf);
    else if (hl_list[i].mode != XmHIGHLIGHT_SELECTED)
       ResetImageGC(tf);
    else 
       InvertImageGC(tf);

    if (adjust_flag) (void) AdjustText(tf, position, flag);

    ResetClipOrigin(tf, False);

    tf->text.refresh_ibeam_off = True;
    _XmTextFieldDrawInsertionPoint(tf, True);

    (void) GetXYFromPos(tf, TextF_CursorPosition(tf),
			&xmim_point.x, &xmim_point.y);
    XmImVaSetValues((Widget)tf, XmNspotLocation, &xmim_point, NULL);

    if (set_dest)
      (void) SetDestination((Widget) tf, TextF_CursorPosition(tf), False, 
			    XtLastTimestampProcessed(XtDisplay((Widget)tf)));
}


/*
 * This routine is used to verify that the positions are within the bounds
 * of the current TextField widgets value.  Also, it ensures that left is
 * less than right.
 */
static void 
#ifdef _NO_PROTO
VerifyBounds( tf, from, to )
        XmTextFieldWidget tf ;
        XmTextPosition *from ;
        XmTextPosition *to ;
#else
VerifyBounds(
        XmTextFieldWidget tf,
        XmTextPosition *from,
        XmTextPosition *to )
#endif /* _NO_PROTO */
{
  XmTextPosition tmp;

    if (*from < 0) 
       *from = 0;
    else if (*from > tf->text.string_length) {
       *from = tf->text.string_length;
    }
    if (*to < 0 ) 
       *to = 0;
    else if (*to > tf->text.string_length) {
       *to = tf->text.string_length;
    }
    if (*from > *to) {
       tmp = *to;
       *to = *from;
       *from = tmp;
    }
}

/*
 * Function _XmTextFieldReplaceText
 *
 * _XmTextFieldReplaceText is a utility function for the text-modifying
 * action procedures below (InsertChar, DeletePrevChar, and so on). 
 * _XmTextFieldReplaceText does the real work of editing the string,
 * including:
 *
 *   (1) invoking the modify verify callbacks,
 *   (2) allocating more memory for the string if necessary,
 *   (3) doing the string manipulation,
 *   (4) moving the selection (the insertion point), and
 *   (5) redrawing the text.
 *
 * Though the procedure claims to take a char* argument, MB_CUR_MAX determines
 * what the different routines will actually pass to it.  If MB_CUR_MAX is
 * greater than 1, then "insert" points to wchar_t data and we must set up
 * the appropriate cast.  In all cases, insert_length is the number of
 * characters (not bytes) to be inserted.
 */

Boolean 
#ifdef _NO_PROTO
_XmTextFieldReplaceText( tf, event, replace_prev, replace_next,
		         insert, insert_length, move_cursor )
        XmTextFieldWidget tf ;
        XEvent *event ;
        XmTextPosition replace_prev ;
        XmTextPosition replace_next ;
        char *insert ;
        int insert_length ;
	Boolean move_cursor ;
#else
_XmTextFieldReplaceText(
        XmTextFieldWidget tf,
        XEvent *event,
        XmTextPosition replace_prev,
        XmTextPosition replace_next,
        char *insert,
        int insert_length,
#if NeedWidePrototypes
	int move_cursor )
#else
	Boolean move_cursor )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  int replace_length, i;
  char *src, *dst;
  wchar_t *wc_src, *wc_dst;
  int delta = 0;
  XmTextPosition cursorPos, newInsert;
  XmTextPosition old_pos = replace_prev;
  int free_insert = (int)False;

  VerifyBounds(tf, &replace_prev, &replace_next);

  if (!TextF_Editable(tf)) {
     if (tf->text.verify_bell) XBell(XtDisplay((Widget)tf), 0);
     return False;
  }

  replace_length = (int) (replace_next - replace_prev);
  delta = insert_length - replace_length;

 /* Disallow insertions that go beyond max length boundries.
  */
  if ((delta >= 0) && 
      ((tf->text.string_length + delta) - (TextF_MaxLength(tf)) > 0)) { 
      if (tf->text.verify_bell) XBell(XtDisplay(tf), 0);
      return False;
  }


 /* If there are modify verify callbacks, verify that we want to continue
  * the action.
  */
  newInsert = TextF_CursorPosition(tf);

  if (TextF_ModifyVerifyCallback(tf) || TextF_ModifyVerifyCallbackWcs(tf)) {
    /* If the function ModifyVerify() returns false then don't
     * continue with the action.
     */
     if (!ModifyVerify(tf, event, &replace_prev, &replace_next,
		       &insert, &insert_length, &newInsert, &free_insert)) {
        if (tf->text.verify_bell) XBell(XtDisplay(tf), 0);
	if (free_insert) XtFree(insert);
	return False;
     } else {
        VerifyBounds(tf, &replace_prev, &replace_next);
        replace_length = (int) (replace_next - replace_prev);
        delta = insert_length - replace_length;

       /* Disallow insertions that go beyond max length boundries.
        */
        if ((delta >= 0) && 
	    ((tf->text.string_length + delta) - (TextF_MaxLength(tf)) > 0)) { 
            if (tf->text.verify_bell) XBell(XtDisplay(tf), 0);
	    if (free_insert) XtFree(insert);
            return False;
        }

     }
  }

 /* make sure selections are turned off prior to changeing text */
  if (tf->text.has_primary &&
      tf->text.prim_pos_left != tf->text.prim_pos_right)
     XmTextFieldSetHighlight((Widget)tf, tf->text.prim_pos_left,
			     tf->text.prim_pos_right, XmHIGHLIGHT_NORMAL);

  _XmTextFieldDrawInsertionPoint(tf, False);

  /* Allocate more space if we need it.
   */
  if (tf->text.max_char_size == 1){
  if (tf->text.string_length + insert_length - replace_length >=
      tf->text.size_allocd)
    {
      tf->text.size_allocd += MAX(insert_length + TEXT_INCREMENT,
                                        (tf->text.size_allocd * 2));
      tf->text.value = (char *) XtRealloc((char*)TextF_Value(tf), 
		              (unsigned) (tf->text.size_allocd * sizeof(char)));
    }
  } else {
  if ((tf->text.string_length + insert_length - replace_length) *
                                        sizeof(wchar_t) >= tf->text.size_allocd)
    {
      tf->text.size_allocd +=
			  MAX((insert_length + TEXT_INCREMENT)*sizeof(wchar_t),
                              (tf->text.size_allocd * 2));
      tf->text.wc_value = (wchar_t *) XtRealloc((char*)TextF_WcValue(tf), 
		           (unsigned) tf->text.size_allocd);
    }
  }

  if (tf->text.has_primary && replace_prev < tf->text.prim_pos_right &&
			      replace_next > tf->text.prim_pos_left) {
     if (replace_prev <= tf->text.prim_pos_left) {
	if (replace_next < tf->text.prim_pos_right) {
          /* delete encompasses left half of the selection
	   * so move left endpoint
           */
	   tf->text.prim_pos_left = replace_next;
	} else {
          /* delete encompasses the selection so set selection to NULL */
	   tf->text.prim_pos_left = tf->text.prim_pos_right;
	}
     } else {
	if (replace_next > tf->text.prim_pos_right) {
	  /* delete encompasses the right half of the selection
	   * so move right endpoint
	   */
	   tf->text.prim_pos_right = replace_next;
	} else {
	  /* delete is completely within the selection
	   * so set selection to NULL
	   */
	   tf->text.prim_pos_right = tf->text.prim_pos_left;
	}
     }
  }

  if (tf->text.max_char_size == 1) {
     if (replace_length > insert_length)
       /* We need to shift the text at and after replace_next to the left. */
       for (src = TextF_Value(tf) + replace_next,
            dst = src + (insert_length - replace_length),
            i = (int) ((tf->text.string_length + 1) - replace_next);
            i > 0;
            ++src, ++dst, --i)
         *dst = *src;
     else if (replace_length < insert_length)
       /* We need to shift the text at and after replace_next to the right. */
       /* Need to add 1 to string_length to handle the NULL terminator on */
       /* the string. */
       for (src = TextF_Value(tf) + tf->text.string_length,
            dst = src + (insert_length - replace_length),
            i = (int) ((tf->text.string_length + 1) - replace_next);
            i > 0;
            --src, --dst, --i)
         *dst = *src;

    /* Update the string.
     */
     if (insert_length != 0) {
        for (src = insert,
             dst = TextF_Value(tf) + replace_prev,
             i = insert_length;
             i > 0;
             ++src, ++dst, --i)
          *dst = *src;
     }
   } else {  /* have wchar_t* data */
     if (replace_length > insert_length)
       /* We need to shift the text at and after replace_next to the left. */
       for (wc_src = TextF_WcValue(tf) + replace_next,
            wc_dst = wc_src + (insert_length - replace_length),
            i = (int) ((tf->text.string_length + 1) - replace_next);
            i > 0;
            ++wc_src, ++wc_dst, --i)
         *wc_dst = *wc_src;
     else if (replace_length < insert_length)
       /* We need to shift the text at and after replace_next to the right. */
       /* Need to add 1 to string_length to handle the NULL terminator on */
       /* the string. */
       for (wc_src = TextF_WcValue(tf) + tf->text.string_length,
            wc_dst = wc_src + (insert_length - replace_length),
            i = (int) ((tf->text.string_length + 1) - replace_next);
            i > 0;
            --wc_src, --wc_dst, --i)
         *wc_dst = *wc_src;

    /* Update the string.
     */
     if (insert_length != 0) {
        for (wc_src = (wchar_t *)insert,
             wc_dst = TextF_WcValue(tf) + replace_prev,
             i = insert_length;
             i > 0;
             ++wc_src, ++wc_dst, --i)
          *wc_dst = *wc_src;
     }
   }

  if (tf->text.has_primary &&
      tf->text.prim_pos_left != tf->text.prim_pos_right) {
     if (replace_prev <= tf->text.prim_pos_left) {
        tf->text.prim_pos_left += delta;
        tf->text.prim_pos_right += delta;
     }
     if (tf->text.prim_pos_left > tf->text.prim_pos_right)
	tf->text.prim_pos_right = tf->text.prim_pos_left;
  }

 /* make sure the selection are redisplay, since they were turned off earlier */
  if (tf->text.has_primary &&
      tf->text.prim_pos_left != tf->text.prim_pos_right)
     XmTextFieldSetHighlight((Widget)tf, tf->text.prim_pos_left,
			     tf->text.prim_pos_right, XmHIGHLIGHT_SELECTED);

  tf->text.string_length += insert_length - replace_length;

  if (move_cursor) {
     if (TextF_CursorPosition(tf) != newInsert) {
        if (newInsert > tf->text.string_length) {
	   cursorPos = tf->text.string_length;
	} else if (newInsert < 0) {
           cursorPos = 0;
        } else {
           cursorPos = newInsert;
        }
     } else
       cursorPos = replace_next + (insert_length - replace_length);
     if (event != NULL) {
        (void)SetDestination((Widget)tf, cursorPos, False, event->xkey.time);
     } else {
        (void) SetDestination((Widget)tf, cursorPos, False,
			      XtLastTimestampProcessed(XtDisplay((Widget)tf)));
     }
     _XmTextFieldSetCursorPosition(tf, event, cursorPos, False, True);
  }

  if (TextF_ResizeWidth(tf) && tf->text.do_resize) {
     AdjustSize(tf);
  } else {
     AdjustText(tf, TextF_CursorPosition(tf), False);
     RedisplayText(tf, old_pos, tf->text.string_length);
  }

  _XmTextFieldDrawInsertionPoint(tf, True);
  if (free_insert) XtFree(insert);
  return True;
}

/*
 * Reset selection flag and selection positions and then display
 * the new settings.
 */
void 
#ifdef _NO_PROTO
_XmTextFieldDeselectSelection( w, disown, sel_time )
        Widget w ;
        Boolean disown ;
        Time sel_time ;
#else
_XmTextFieldDeselectSelection(
        Widget w,
#if NeedWidePrototypes
        int disown,
#else
        Boolean disown,
#endif /* NeedWidePrototypes */
        Time sel_time )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;

  if (disown)
    /*
     * Disown the primary selection (This function is a no-op if
     * this widget doesn't own the primary selection)
     */
     XtDisownSelection(w, XA_PRIMARY, sel_time);
  if (tf != NULL) {
     _XmTextFieldDrawInsertionPoint(tf, False);
     tf->text.has_primary = False;
     TextFieldSetHighlight(tf, tf->text.prim_pos_left,
		        tf->text.prim_pos_right, XmHIGHLIGHT_NORMAL);
     tf->text.prim_pos_left = tf->text.prim_pos_right =
	       tf->text.prim_anchor = TextF_CursorPosition(tf);

     if (!tf->text.has_focus) XmTextFieldSetAddMode(w, False);

     RedisplayText(tf, 0, tf->text.string_length);

     _XmTextFieldDrawInsertionPoint(tf, True);
  }
}

/*
 * Finds the cursor position from the given X value.
 */
static XmTextPosition 
#ifdef _NO_PROTO
GetPosFromX( tf, x )
        XmTextFieldWidget tf ;
        Position x ;
#else
GetPosFromX(
        XmTextFieldWidget tf,
#if NeedWidePrototypes
        int x )
#else
        Position x )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextPosition position;
    int temp_x = 0;
    int next_char_width = 0;

   /* Decompose the x to equal the length of the text string */
    temp_x += (int) tf->text.h_offset;

   /* Next width is an offset allowing button presses on the left side 
    * of a character to select that character, while button presses
    * on the rigth side of the character select the  NEXT character.
    */

    if (tf->text.string_length > 0) {

       if (tf->text.max_char_size != 1) {
          next_char_width = FindPixelLength(tf, (char*)TextF_WcValue(tf), 1);
       } else {
          next_char_width = FindPixelLength(tf, TextF_Value(tf), 1);
       }
    }

    for (position = 0; temp_x + next_char_width/2 < (int) x &&
	               position < tf->text.string_length; position++){

       temp_x+=next_char_width;    /* 
				    * We still haven't reached the x pos.
				    * Add the width and find the next chars
				    * width. 
				    */

	/*
	 * If there is a next position, find its width.  Otherwise, use the
	 * current "next" width.
	 */

       if (tf->text.string_length > position + 1) {
          if (tf->text.max_char_size != 1) {
             next_char_width = FindPixelLength(tf,
				  (char*)(TextF_WcValue(tf) + position + 1), 1);
	  } else {
             next_char_width = FindPixelLength(tf,
				           TextF_Value(tf) + position + 1, 1);
	  }
       } 
    } /* for */

    return position;
}


/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetDestination( w, position, disown, set_time )
        Widget w ;
        XmTextPosition position ;
        Boolean disown ;
        Time set_time ;
#else
SetDestination(
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Boolean result = TRUE;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
                                        "MOTIF_DESTINATION", False);

    if (!XtIsRealized(w)) return False;

    _XmTextFieldDrawInsertionPoint(tf, False);

    if (!disown) {
        if (!tf->text.has_destination) {
	    if (!set_time) set_time = GetServerTime(w);
            result = XtOwnSelection(w, MOTIF_DESTINATION, set_time,
                                    _XmTextFieldConvert,
				    _XmTextFieldLoseSelection,
                                    (XtSelectionDoneProc) NULL);
            tf->text.dest_time = set_time;
            tf->text.has_destination = result;

            if (result) _XmSetDestination(XtDisplay(w), w);
      	    _XmTextFToggleCursorGC(w);
        }
    } else {
        if (tf->text.has_destination)
	   if (!set_time) set_time = GetServerTime(w);
           XtDisownSelection(w, MOTIF_DESTINATION, set_time);

          /* Call XmGetDestination(dpy) to get widget that last had
             destination cursor. */
           if (w == XmGetDestination(XtDisplay(w)))
              _XmSetDestination(XtDisplay(w), (Widget)NULL);

           tf->text.has_destination = False;
      	   _XmTextFToggleCursorGC(w);
    }

    _XmTextFieldDrawInsertionPoint(tf, True);

    return result;
}

Boolean 
#ifdef _NO_PROTO
_XmTextFieldSetDestination( w, position, set_time )
        Widget w ;
        XmTextPosition position ;
        Time set_time ;
#else
_XmTextFieldSetDestination(
        Widget w,
        XmTextPosition position,
        Time set_time )
#endif /* _NO_PROTO */
{
   Boolean result;

   result = SetDestination(w, position, False, set_time);

   return result;
}


/*
 * Calls the losing focus verify callback to verify that the application
 * want to traverse out of the text field widget.  Returns the result.
 */
static Boolean 
#ifdef _NO_PROTO
VerifyLeave( tf, event )
        XmTextFieldWidget tf ;
        XEvent *event ;
#else
VerifyLeave(
        XmTextFieldWidget tf,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmTextVerifyCallbackStruct  cbdata;

    cbdata.reason = XmCR_LOSING_FOCUS;
    cbdata.event = event;
    cbdata.doit = True;
    cbdata.currInsert = TextF_CursorPosition(tf);
    cbdata.newInsert = TextF_CursorPosition(tf);
    cbdata.startPos = TextF_CursorPosition(tf);
    cbdata.endPos = TextF_CursorPosition(tf);
    cbdata.text = NULL;
    XtCallCallbackList((Widget) tf, TextF_LosingFocusCallback(tf), 
		       (XtPointer) &cbdata);
    return(cbdata.doit);
}

/* This routine is used to determine if two adjacent wchar_t characters
 * constitute a word boundary */
/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
_XmTextFieldIsWordBoundary( tf, pos1, pos2 )
	XmTextFieldWidget tf ;
	XmTextPosition pos1 ;
	XmTextPosition pos2 ;
#else
_XmTextFieldIsWordBoundary(
	XmTextFieldWidget tf,
	XmTextPosition pos1 ,
	XmTextPosition pos2 )
#endif /* _NO_PROTO */
{
   int size_pos1 = 0;
   int size_pos2 = 0;
   char s1[MB_LEN_MAX];
   char s2[MB_LEN_MAX];

/* if positions aren't adjacent, return False */
   if(pos1 < pos2 && ((pos2 - pos1) != 1)) 
      return False;
   else if(pos2 < pos1 && ((pos1 - pos2) != 1)) 
      return False;

   if (tf->text.max_char_size == 1) { /* data is char* and one-byte per char */
      if (isspace((unsigned char)TextF_Value(tf)[pos1]) || 
	  isspace((unsigned char)TextF_Value(tf)[pos2])) return True;
   } else {
      size_pos1 = wctomb(s1, TextF_WcValue(tf)[pos1]);
      size_pos2 = wctomb(s2, TextF_WcValue(tf)[pos2]);
      if (size_pos1 == 1 && (size_pos2 != 1 || isspace((unsigned char)*s1)))
	return True;
      if (size_pos2 == 1 && (size_pos1 != 1 || isspace((unsigned char)*s2)))
	return True;
   }
   return False;
}

/* This routine accepts an array of wchar_t's containing wchar encodings
 * of whitespace characters (and the number of array elements), comparing
 * the wide character passed to each element of the array.  If a match
 * is found, we got a white space.  This routine exists only because
 * iswspace(3c) is not yet standard.  If a system has isw* available,
 * calls to this routine should be changed to iswspace(3c) (and callers
 * should delete initialization of the array), and this routine should
 * be deleted.  Its a stop gap measure to avoid allocating an instance
 * variable for the white_space array and/or declaring a widget wide
 * global for the data and using a macro.  Its ugly, but it works and 
 * in the long run will be replaced by standard functionality. */

/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
_XmTextFieldIsWSpace( wide_char, white_space, num_entries )
	wchar_t wide_char ;
	wchar_t * white_space ;
	int num_entries ;
#else
_XmTextFieldIsWSpace(
	wchar_t wide_char,
	wchar_t * white_space ,
	int num_entries )
#endif /* _NO_PROTO */
{
   int i;

   for (i=num_entries; i > 0; i--){
      if (wide_char == white_space[i]) return True;
   }
   return False;
}

static void 
#ifdef _NO_PROTO
FindWord( tf, begin, left, right )
        XmTextFieldWidget tf ;
        XmTextPosition begin ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
FindWord(
        XmTextFieldWidget tf,
        XmTextPosition begin,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{
    XmTextPosition start, end;
    wchar_t white_space[3];

    if (tf->text.max_char_size == 1) {
       for (start = begin; start > 0; start--) {
          if (isspace((unsigned char)TextF_Value(tf)[start - 1])) {
	     break;
          }
       }
       *left = start;

       for (end = begin; end <= tf->text.string_length; end++) {
          if (isspace((unsigned char)TextF_Value(tf)[end])) {
             end++;
             break;
          }
       }
       *right = end - 1;
    } else { /* check for iswspace and iswordboundary in each direction */
       (void)mbtowc(&white_space[0], " ", 1);
       (void)mbtowc(&white_space[1], "\n", 1);
       (void)mbtowc(&white_space[2], "\t", 1);
       for (start = begin; start > 0; start --) {
          if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[start-1],white_space, 3)
	      || _XmTextFieldIsWordBoundary(tf, (XmTextPosition) start - 1, 
					    start)) {
		 break;
	  }
       }
       *left = start;

       for (end = begin; end <= tf->text.string_length; end++) {
	   if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[end], white_space, 3)){
	      end++;
	      break;
	   } else if (end < tf->text.string_length) {
	      if (_XmTextFieldIsWordBoundary(tf, end, (XmTextPosition)end + 1)){
	         end += 2; /* want to return position of next word; end + 1 */
		 break;    /* is that position && *right = end - 1... */
              }
           }
       }
       *right = end - 1;
   }
}

static void 
#ifdef _NO_PROTO
FindPrevWord( tf, left, right )
        XmTextFieldWidget tf ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
FindPrevWord(
        XmTextFieldWidget tf,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{

    XmTextPosition start = TextF_CursorPosition(tf);
    wchar_t white_space[3];

    if (tf->text.max_char_size != 1) {
       (void)mbtowc(&white_space[0], " ", 1);
       (void)mbtowc(&white_space[1], "\n", 1);
       (void)mbtowc(&white_space[2], "\t", 1);
    }


    if (tf->text.max_char_size == 1) {
       if ((start > 0) && 
	   (isspace((unsigned char)TextF_Value(tf)[start - 1]))) {
           for (; start > 0; start--) {
               if (!isspace((unsigned char)TextF_Value(tf)[start - 1])) {
                  start--;
                  break;
               }
           }
       }
       FindWord(tf, start, left, right);
    } else { 
       if ((start > 0) && (_XmTextFieldIsWSpace(TextF_WcValue(tf)[start - 1],
						white_space, 3))) {
          for (; start > 0; start--) {
	     if (!_XmTextFieldIsWSpace(TextF_WcValue(tf)[start -1], 
				       white_space, 3)){
		start--;
		break;
             }
          }
       } else if ((start > 0) && 
		  _XmTextFieldIsWordBoundary(tf, (XmTextPosition) start - 1, 
					     start)){
          start--;
       }
       FindWord(tf, start, left, right);
    }
}

static void 
#ifdef _NO_PROTO
FindNextWord( tf, left, right )
        XmTextFieldWidget tf ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
FindNextWord(
        XmTextFieldWidget tf,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{

    XmTextPosition end = TextF_CursorPosition(tf);
    wchar_t white_space[3];

    if (tf->text.max_char_size != 1) {
       (void)mbtowc(&white_space[0], " ", 1);
       (void)mbtowc(&white_space[1], "\n", 1);
       (void)mbtowc(&white_space[2], "\t", 1);
    }


    if(tf->text.max_char_size == 1) {
       if (isspace((unsigned char)TextF_Value(tf)[end])) {
           for (end = TextF_CursorPosition(tf);
                end < tf->text.string_length; end++) {
               if (!isspace((unsigned char)TextF_Value(tf)[end])) {
                  break;
               }
           }
       }
       FindWord(tf, end, left, right);
      /*
       * Set right to the last whitespace following the end of the
       * current word.
       */
       while (*right < tf->text.string_length &&
              isspace((unsigned char)TextF_Value(tf)[(int)*right]))
             *right = *right + 1;
       if (*right < tf->text.string_length)
          *right = *right - 1;
   } else {
      if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[end], white_space, 3)) {
	 for ( ; end < tf->text.string_length; end ++) {
	   if (!_XmTextFieldIsWSpace(TextF_WcValue(tf)[end], white_space, 3)) {
	       break;
           }
         }
      } else { /* if for other reasons at word boundry, advance to next word */
	 if ((end < tf->text.string_length) && 
	      _XmTextFieldIsWordBoundary(tf, end, (XmTextPosition) end + 1))
	      end++;
      }
      FindWord(tf, end, left, right);
      /*
       * If word boundary caused by whitespace, set right to the last 
       * whitespace following the end of the current word.
       */
      if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[(int)*right], white_space, 3))      {
         while (*right < tf->text.string_length &&
               _XmTextFieldIsWSpace(TextF_WcValue(tf)[(int)*right], 
				    white_space, 3)) {
            *right = *right + 1;
	 }
	 if (*right < tf->text.string_length)
            *right = *right - 1;
      }
   }
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition left = 0, right = 0;

    if (tf->text.add_mode || 
        (XmTextFieldGetSelectionPosition(w, &left, &right) && left != right &&
         position >= left && position <= right))
       tf->text.pending_off = FALSE;
    else
       tf->text.pending_off = TRUE;

    if (left == right) {
       (void) SetDestination(w, position, False, sel_time);
       tf->text.prim_anchor = position;
    } else {
       (void) SetDestination(w, position, False, sel_time);
       if (!tf->text.add_mode) tf->text.prim_anchor = position;
    }
}

static Boolean 
#ifdef _NO_PROTO
NeedsPendingDelete( tf )
        XmTextFieldWidget tf ;
#else
NeedsPendingDelete(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
   return (tf->text.add_mode ?
	   (TextF_PendingDelete(tf) &&
	    tf->text.has_primary &&
	    tf->text.prim_pos_left != tf->text.prim_pos_right &&
	    tf->text.prim_pos_left <= TextF_CursorPosition(tf) &&
	    tf->text.prim_pos_right >= TextF_CursorPosition(tf)) :
	   (tf->text.has_primary &&
	    tf->text.prim_pos_left != tf->text.prim_pos_right));
}

static Boolean 
#ifdef _NO_PROTO
NeedsPendingDeleteDisjoint( tf )
        XmTextFieldWidget tf ;
#else
NeedsPendingDeleteDisjoint(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
  return (TextF_PendingDelete(tf) &&
	  tf->text.has_primary &&
	  tf->text.prim_pos_left != tf->text.prim_pos_right &&
	  tf->text.prim_pos_left <= TextF_CursorPosition(tf) &&
	  tf->text.prim_pos_right >= TextF_CursorPosition(tf));
}

static Boolean
#ifdef _NO_PROTO
PrintableString(tf, str, n, use_wchar)
     XmTextFieldWidget tf;
     char* str;
     int n;
     Boolean use_wchar;
#else
PrintableString(XmTextFieldWidget tf,
              char* str,
              int n,
              Boolean use_wchar)
#endif
{
#ifdef SUPPORT_ZERO_WIDTH
  /* some locales (such as Thai) have characters that are
   * printable but non-spacing. These should be inserted,
   * even if they have zero width.
   */
  if (tf->text.max_char_size == 1) {
    int i;
    if (!use_wchar) {
      for (i = 0; i < n; i++) {
      if (!isprint((unsigned char)str[i])) {
        return False;
      }
      }
    } else {
      char scratch[8];
      wchar_t *ws = (wchar_t *) str;
      for (i = 0; i < n; i++) {
      if (wctomb(scratch, ws[i]) <= 0)
        return False;
      if (!isprint((unsigned char)scratch[0])) {
        return False;
      }
      }
    }
    return True;
  } else {
    /* tf->text.max_char_size > 1 */
#ifdef HAS_WIDECHAR_FUNCTIONS
    if (use_wchar) {
      int i;
      wchar_t *ws = (wchar_t *) str;
      for (i = 0; i < n; i++) {
      if (!iswprint(ws[i])) {
        return False;
      }
      }
      return True;
    } else {
      int i, csize;
      wchar_t wc;
      for (i = 0, csize = mblen(str, tf->text.max_char_size);
         i < n;
         i += csize, csize=mblen(&(str[i]), tf->text.max_char_size))
      {
        if (csize < 0)
          return False;
        if (mbtowc(&wc, &(str[i]), tf->text.max_char_size) <= 0)
          return False;
        if (!iswprint(wc)) {
          return False;
        }
      }
    }
#else /* HAS_WIDECHAR_FUNCTIONS */
    /*
     * This will only check if any single-byte characters are non-
     * printable. Better than nothing...
     */
    int i, csize;
    if (!use_wchar) {
      for (i = 0, csize = mblen(str, tf->text.max_char_size);
         i < n;
         i += csize, csize=mblen(&(str[i]), tf->text.max_char_size))
      {
        if (csize < 0)
          return False;
        if (csize == 1 && !isprint((unsigned char)str[i])) {
          return False;
        }
      }
    } else {
      char scratch[8];
      wchar_t *ws = (wchar_t *) str;
      for (i = 0; i < n; i++) {
      if ((csize = wctomb(scratch, ws[i])) <= 0)
        return False;
      if (csize == 1 && !isprint((unsigned char)scratch[0])) {
        return False;
      }
      }
    }
#endif /* HAS_WIDECHAR_FUNCTIONS */
    return True;
  }
#else  /* SUPPORT_ZERO_WIDTH */
  if (TextF_UseFontSet(tf)) {
	if(use_wchar) 
    		return (XwcTextEscapement((XFontSet)TextF_Font(tf), (wchar_t *)str, n) != 0);
	else
    		return (XmbTextEscapement((XFontSet)TextF_Font(tf), str, n) != 0);
  }
  else
    return (XTextWidth(TextF_Font(tf), str, n) != 0);
#endif /* SUPPORT_ZERO_WIDTH */
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


/****************************************************************
 *
 * Input functions defined in the action table.
 *
 ****************************************************************/

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
InsertChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
InsertChar(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  char insert_string[TEXT_MAX_INSERT_SIZE];
  XmTextPosition cursorPos, nextPos;
  wchar_t *wc_insert_string;
  int insert_length, i;
  int num_chars;
  Boolean replace_res;
  Boolean pending_delete = False;
  Status status_return;
  XmAnyCallbackStruct cb;

  if (!TextF_Editable(tf)) {
     if (tf->text.verify_bell) XBell(XtDisplay((Widget)tf), 0);
  }

 /* Determine what was pressed.
  */
  insert_length = XmImMbLookupString(w, (XKeyEvent *) event, insert_string, 
		                     TEXT_MAX_INSERT_SIZE, (KeySym *) NULL, 
				     &status_return);

 /* If there is more data than we can handle, bail out */
  if (status_return == XBufferOverflow || insert_length > TEXT_MAX_INSERT_SIZE)
     return;

 /* *LookupString in some cases can return the NULL as a character, such
  * as when the user types <Ctrl><back_quote> or <Ctrl><@>.  Text widget
  * can't handle the NULL as a character, so we dump it here.
  */

  for (i=0; i < insert_length; i++)
     if (insert_string[i] == 0) insert_length = 0; /* toss out input string */
     
  if (insert_length > 0) {
   /* do not insert non-printing characters */
    if (!PrintableString(tf, insert_string, insert_length, False))
      return;

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (NeedsPendingDeleteDisjoint(tf)){
       if (!XmTextFieldGetSelectionPosition(w, &cursorPos, &nextPos) ||
            cursorPos == nextPos) {
          tf->text.prim_anchor = TextF_CursorPosition(tf);
       }
       pending_delete = True;

       tf->text.prim_anchor = TextF_CursorPosition(tf);

    } else {
       cursorPos = nextPos = TextF_CursorPosition(tf);
    }


    if (tf->text.overstrike) {
       if (nextPos != tf->text.string_length) nextPos++;
    }
    if (tf->text.max_char_size == 1) {
       if (tf->text.overstrike) nextPos += insert_length;
       if (nextPos > tf->text.string_length) nextPos = tf->text.string_length;
       replace_res = _XmTextFieldReplaceText(tf, (XEvent *) event, cursorPos,
					     nextPos, insert_string,
					     insert_length, True);
    } else {
       char stack_cache[100];
       insert_string[insert_length] = '\0'; /* NULL terminate for mbstowcs */
       wc_insert_string = (wchar_t*)XmStackAlloc((Cardinal)(insert_length+1) *						     sizeof(wchar_t), stack_cache);
       num_chars = mbstowcs( wc_insert_string, insert_string, insert_length+1);
       if (num_chars < 0) num_chars = 0;
       if (tf->text.overstrike) nextPos += num_chars;
       if (nextPos > tf->text.string_length) nextPos = tf->text.string_length;
       replace_res = _XmTextFieldReplaceText(tf, (XEvent *) event, cursorPos,
					     nextPos, (char*) wc_insert_string,
					     num_chars, True);
       XmStackFree((char *)wc_insert_string, stack_cache);
    }

    if (replace_res) {
        if (pending_delete) {
           XmTextFieldSetSelection(w, TextF_CursorPosition(tf),
                               TextF_CursorPosition(tf), event->xkey.time);
             if (cursorPos < TextF_CursorPosition(tf) &&
                 TextF_CursorPosition(tf) == tf->text.string_length &&
                 nextPos - cursorPos > tf->text.columns)
               AdjustText(tf, TextF_CursorPosition(tf)-1, True);
        }
        CheckDisjointSelection(w, TextF_CursorPosition(tf),
			       event->xkey.time);
        _XmTextFieldSetCursorPosition(tf, event, TextF_CursorPosition(tf),
 				False, True);
  	cb.reason = XmCR_VALUE_CHANGED;
        cb.event = event;
        XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
                           (XtPointer) &cb);

    }
    _XmTextFieldDrawInsertionPoint(tf, True);
  }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeletePrevChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeletePrevChar(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmAnyCallbackStruct cb;

  /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else { 
     if (tf->text.has_primary &&
         tf->text.prim_pos_left != tf->text.prim_pos_right) {
        if (TextF_CursorPosition(tf) - 1 >= 0)
           if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf) - 1,
                                     TextF_CursorPosition(tf), NULL, 0, True)) {
              CheckDisjointSelection(w, TextF_CursorPosition(tf),
	               event->xkey.time);
              _XmTextFieldSetCursorPosition(tf, event,
					    TextF_CursorPosition(tf),
					    False, True);
              cb.reason = XmCR_VALUE_CHANGED;
              cb.event = event;
              XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                 (XtPointer) &cb);
           }
      } else if (TextF_CursorPosition(tf) - 1 >= 0) {
        if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf) - 1,
                                    TextF_CursorPosition(tf), NULL, 0, True)) {
            CheckDisjointSelection(w, TextF_CursorPosition(tf),
	             event->xkey.time);
            _XmTextFieldSetCursorPosition(tf, event, TextF_CursorPosition(tf), 
					  False, True);
            cb.reason = XmCR_VALUE_CHANGED;
            cb.event = event;
            XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		               (XtPointer) &cb);
        }
      }  
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeleteNextChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeleteNextChar(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmAnyCallbackStruct cb;

 /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else { 
      if (tf->text.has_primary &&
           tf->text.prim_pos_left != tf->text.prim_pos_right) {
          if (TextF_CursorPosition(tf) < tf->text.string_length)
             if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf),
                                 TextF_CursorPosition(tf) + 1, NULL, 0, True)) {
                 CheckDisjointSelection(w, TextF_CursorPosition(tf),
			                event->xkey.time);
                 _XmTextFieldSetCursorPosition(tf, event, 
					       TextF_CursorPosition(tf), 
					       False, True);
                 cb.reason = XmCR_VALUE_CHANGED;
                 cb.event = event;
                 XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                    (XtPointer) &cb);
             }
       } else if (TextF_CursorPosition(tf) < tf->text.string_length)
          if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf),
                                 TextF_CursorPosition(tf) + 1, NULL, 0, True)) {
              CheckDisjointSelection(w, TextF_CursorPosition(tf),
			             event->xkey.time);
              _XmTextFieldSetCursorPosition(tf, event, 
					    TextF_CursorPosition(tf),
					    False, True);
              cb.reason = XmCR_VALUE_CHANGED;
              cb.event = event;
              XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                 (XtPointer) &cb);
          }
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeletePrevWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeletePrevWord(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition left, right;
  XmAnyCallbackStruct cb;

 /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else { 
       FindPrevWord(tf, &left, &right);
       if (tf->text.has_primary &&
           tf->text.prim_pos_left != tf->text.prim_pos_right) {
          if (_XmTextFieldReplaceText(tf, event, left, TextF_CursorPosition(tf),
				      NULL, 0, True)) {
             CheckDisjointSelection(w, TextF_CursorPosition(tf),
                                    event->xkey.time);
             _XmTextFieldSetCursorPosition(tf, event, 
					   TextF_CursorPosition(tf), 
					   False, True);
             cb.reason = XmCR_VALUE_CHANGED;
             cb.event = event;
             XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                (XtPointer) &cb);
          }
       } else if (TextF_CursorPosition(tf) - 1 >= 0)
          if (_XmTextFieldReplaceText(tf, event, left, TextF_CursorPosition(tf),
				      NULL, 0, True)) {
              CheckDisjointSelection(w, TextF_CursorPosition(tf),
			             event->xkey.time);
              _XmTextFieldSetCursorPosition(tf, event,
					    TextF_CursorPosition(tf),
					    False, True);
              cb.reason = XmCR_VALUE_CHANGED;
              cb.event = event;
              XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                 (XtPointer) &cb);
          }
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeleteNextWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeleteNextWord(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition left, right;
  XmAnyCallbackStruct cb;

 /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else { 
       FindNextWord(tf, &left, &right);
       if (tf->text.has_primary &&
           tf->text.prim_pos_left != tf->text.prim_pos_right) {
          if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf),
				      right, NULL, 0, True)){
             CheckDisjointSelection(w, TextF_CursorPosition(tf),
				    event->xkey.time);
             _XmTextFieldSetCursorPosition(tf, event,
					   TextF_CursorPosition(tf),
					   False, True);
             cb.reason = XmCR_VALUE_CHANGED;
             cb.event = event;
             XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                (XtPointer) &cb);
          }
       } else if (TextF_CursorPosition(tf) < tf->text.string_length)
          if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf),
				      right, NULL, 0, True)){
              CheckDisjointSelection(w, TextF_CursorPosition(tf),
			             event->xkey.time);
              _XmTextFieldSetCursorPosition(tf, event, 
					    TextF_CursorPosition(tf), 
					    False, True);
              cb.reason = XmCR_VALUE_CHANGED;
              cb.event = event;
              XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		                 (XtPointer) &cb);
          }
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeleteToEndOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeleteToEndOfLine(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmAnyCallbackStruct cb;
    
 /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else if (TextF_CursorPosition(tf) < tf->text.string_length) {
     if (_XmTextFieldReplaceText(tf, event, TextF_CursorPosition(tf),
                                 tf->text.string_length, NULL, 0, True)) {
         CheckDisjointSelection(w, TextF_CursorPosition(tf),
			        event->xkey.time);
         _XmTextFieldSetCursorPosition(tf, event, TextF_CursorPosition(tf),
				       False, True);
         cb.reason = XmCR_VALUE_CHANGED;
         cb.event = event;
         XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		            (XtPointer) &cb);
       }
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeleteToStartOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeleteToStartOfLine(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmAnyCallbackStruct cb;
    
 /* if pending delete is on and there is a selection */
  _XmTextFieldDrawInsertionPoint(tf, False);
  if (NeedsPendingDelete(tf)) (void) TextFieldRemove(w, event);
  else if (TextF_CursorPosition(tf) - 1 >= 0) {
    if (_XmTextFieldReplaceText(tf, event, 0, 
			        TextF_CursorPosition(tf), NULL, 0, True)) {
        CheckDisjointSelection(w, TextF_CursorPosition(tf),
			       event->xkey.time);
        _XmTextFieldSetCursorPosition(tf, event, TextF_CursorPosition(tf),
				      False, True);
        cb.reason = XmCR_VALUE_CHANGED;
        cb.event = event;
        XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		           (XtPointer) &cb);
       }
  }
  _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessCancel( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ProcessCancel(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    XmParentInputActionRec  p_event ;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_CANCEL ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (tf->text.has_secondary) {
       tf->text.cancel = True;
       _XmTextFieldSetSel2(w, 0, 0, False, event->xkey.time);
       tf->text.has_secondary = False;
       XtUngrabKeyboard(w, CurrentTime);
    }

    if (tf->text.has_primary && tf->text.extending) {
       tf->text.cancel = True;
      /* reset orig_left and orig_right */
       XmTextFieldSetSelection(w, tf->text.orig_left,
			       tf->text.orig_right, event->xkey.time);
    }

    if (!tf->text.cancel)
       (void) _XmParentProcess(XtParent(tf), (XmParentProcessData) &p_event);

    if (tf->text.select_id) {
       XtRemoveTimeOut(tf->text.select_id);
       tf->text.select_id = 0;
    }
    _XmTextFieldDrawInsertionPoint(tf, True);

}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Activate( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
Activate(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmAnyCallbackStruct cb;
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmParentInputActionRec  p_event ;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_ACTIVATE ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    cb.reason = XmCR_ACTIVATE;
    cb.event  = event;
    XtCallCallbackList(w, TextF_ActivateCallback(tf), (XtPointer) &cb);

    (void) _XmParentProcess(XtParent(w), (XmParentProcessData) &p_event);
}

static void
#ifdef _NO_PROTO
SetAnchorBalancing(tf, position)
XmTextFieldWidget tf;
XmTextPosition position;
#else
SetAnchorBalancing(
        XmTextFieldWidget tf,
        XmTextPosition position)
#endif /* _NO_PROTO */
{
    XmTextPosition left, right;
    float bal_point;

    if (!XmTextFieldGetSelectionPosition((Widget)tf, &left, &right) ||
	left == right) {
          tf->text.prim_anchor = position;
    } else {
          bal_point = (float)(((float)(right - left) / 2.0) + (float)left);

         /* shift anchor and direction to opposite end of the selection */
          if ((float)position < bal_point) {
             tf->text.prim_anchor = tf->text.orig_right;
          } else if ((float)position > bal_point) {
             tf->text.prim_anchor = tf->text.orig_left;
          }
    }
}

static void
#ifdef _NO_PROTO
SetNavigationAnchor(tf, position, extend)
XmTextFieldWidget tf;
XmTextPosition position;
Boolean extend;
#else
SetNavigationAnchor(
        XmTextFieldWidget tf,
        XmTextPosition position,
#if NeedWidePrototypes
        int extend )
#else
        Boolean extend )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextPosition left, right;

    if (!tf->text.add_mode) {
       if (extend) {
          SetAnchorBalancing(tf, position);
       } else {
         if (XmTextFieldGetSelectionPosition((Widget)tf, &left, &right) &&
            left != right) {
           SetSelection(tf, position, position, True);
           tf->text.prim_anchor = position;
         }
       }
    } else if (extend) {
       SetAnchorBalancing(tf, position);
    }
}

static void
#ifdef _NO_PROTO
CompleteNavigation(tf, event, position, time, extend)
XmTextFieldWidget tf;
XEvent *event;
XmTextPosition position;
Time time;
Boolean extend;
#else
CompleteNavigation(
        XmTextFieldWidget tf,
	XEvent *event,
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
    Boolean     backward = False;

    if ((tf->text.add_mode &&
	 XmTextFieldGetSelectionPosition((Widget)tf, &left, &right) &&
         position >= left && position <= right) || extend)
       tf->text.pending_off = FALSE;
    else
       tf->text.pending_off = TRUE;

    _XmTextFieldSetCursorPosition(tf, event, position, True, True);

    if (extend) {
       if (tf->text.prim_anchor > position) {
          left = position;
          right = tf->text.prim_anchor;
          backward = True;
       } else {
          left = tf->text.prim_anchor;
          right = position;
       }
       XmTextFieldSetSelection((Widget)tf, left, right, time);

    /*  Begin fix for CR 5994 */
    if ( backward ) 
      _XmTextFieldSetCursorPosition(tf, event, position, False, False);
    /*  End fix for CR 5994 */

       tf->text.orig_left = left;
       tf->text.orig_right = right;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SimpleMovement( w, event, params, num_params, cursorPos, position )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
        XmTextPosition cursorPos ;
        XmTextPosition position ;
#else
SimpleMovement(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params,
        XmTextPosition cursorPos,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  Boolean extend = False;

  if (*num_params > 0 && !strcmp(*params, "extend")) extend = True;

  _XmTextFieldDrawInsertionPoint(tf, False);
  SetNavigationAnchor(tf, cursorPos, extend);
  CompleteNavigation(tf, event, position, event->xkey.time, extend);
  _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BackwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
BackwardChar(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition cursorPos, position;

    cursorPos = TextF_CursorPosition(tf);

    if (cursorPos > 0) {
       _XmTextFieldDrawInsertionPoint(tf, False);
       position = cursorPos - 1;
       SimpleMovement((Widget) tf, event, params, num_params,
		      cursorPos, position);
       _XmTextFieldDrawInsertionPoint(tf, True);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ForwardChar( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ForwardChar(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition cursorPos, position;

    cursorPos = TextF_CursorPosition(tf);

    if (cursorPos < tf->text.string_length) {
       _XmTextFieldDrawInsertionPoint(tf, False);
       position = cursorPos + 1;
       SimpleMovement((Widget) tf, event, params, num_params,
		      cursorPos, position);
       _XmTextFieldDrawInsertionPoint(tf, True);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BackwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
BackwardWord(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmTextPosition cursorPos, position, dummy;

   cursorPos = TextF_CursorPosition(tf);

   if (cursorPos > 0) {
      _XmTextFieldDrawInsertionPoint(tf, False);
      FindPrevWord(tf, &position, &dummy);
      SimpleMovement((Widget) tf, event, params, num_params,
		     cursorPos, position);
      _XmTextFieldDrawInsertionPoint(tf, True);
   }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ForwardWord( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ForwardWord(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition cursorPos, position, dummy;
    wchar_t white_space[3];

    if (tf->text.max_char_size != 1) {
       (void)mbtowc(&white_space[0], " ", 1);
       (void)mbtowc(&white_space[1], "\n", 1);
       (void)mbtowc(&white_space[2], "\t", 1);
    }

    cursorPos = TextF_CursorPosition(tf);

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (cursorPos < tf->text.string_length) {
       if (tf->text.max_char_size == 1) {
         if (isspace((unsigned char)TextF_Value(tf)[cursorPos]))
	    FindWord(tf, cursorPos, &dummy, &position);
         else
            FindNextWord(tf, &dummy, &position);
          if(isspace((unsigned char)TextF_Value(tf)[position])){
	     for (;position < tf->text.string_length; position++){
	        if (!isspace((unsigned char)TextF_Value(tf)[position]))
	   	break;
             }
          }
       } else {
	  if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[cursorPos],
				   white_space, 3))
	     FindWord(tf, cursorPos, &dummy, &position);
	  else
	     FindNextWord(tf, &dummy, &position);
          if (_XmTextFieldIsWSpace(TextF_WcValue(tf)[position],
				   white_space, 3)){
	     for (; position < tf->text.string_length; position++) {
		if (!_XmTextFieldIsWSpace(TextF_WcValue(tf)[position], 
					  white_space, 3))
		   break;
	     }
	  }
       }
       SimpleMovement((Widget) tf, event, params, num_params,
		      cursorPos, position);
    }
    _XmTextFieldDrawInsertionPoint(tf, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EndOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
EndOfLine(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmTextPosition cursorPos, position;

   cursorPos = TextF_CursorPosition(tf);

   if (cursorPos < tf->text.string_length) {
      _XmTextFieldDrawInsertionPoint(tf, False);
      position = tf->text.string_length;
      SimpleMovement((Widget) tf, event, params, num_params,
		     cursorPos, position);
      _XmTextFieldDrawInsertionPoint(tf, True);
   }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BeginningOfLine( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
BeginningOfLine(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmTextPosition cursorPos, position;

   cursorPos = TextF_CursorPosition(tf);

   if (cursorPos > 0) {
      position = 0;
      _XmTextFieldDrawInsertionPoint(tf, False);
      SimpleMovement((Widget) tf, event, params, num_params,
		     cursorPos, position);
      _XmTextFieldDrawInsertionPoint(tf, True);
   }
}

static void 
#ifdef _NO_PROTO
SetSelection( tf, left, right, redisplay )
        XmTextFieldWidget tf ;
        XmTextPosition left ;
        XmTextPosition right ;
        Boolean redisplay ;
#else
SetSelection(
        XmTextFieldWidget tf,
        XmTextPosition left,
        XmTextPosition right,
#if NeedWidePrototypes
        int redisplay )
#else
        Boolean redisplay )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmTextPosition display_left, display_right;
   XmTextPosition old_prim_left, old_prim_right;

   if (left < 0) left = 0;
   if (right < 0) right = 0;

   if (left > tf->text.string_length)
      left = tf->text.string_length;
   if (right > tf->text.string_length)
      right = tf->text.string_length;

   if (left == right && tf->text.prim_pos_left != tf->text.prim_pos_right)
      XmTextFieldSetAddMode((Widget)tf, False);
   if (left == tf->text.prim_pos_left && right == tf->text.prim_pos_right)
      return;

   TextFieldSetHighlight(tf, tf->text.prim_pos_left,
		         tf->text.prim_pos_right, XmHIGHLIGHT_NORMAL);

   old_prim_left = tf->text.prim_pos_left;
   old_prim_right = tf->text.prim_pos_right;

   if (left > right) {
      tf->text.prim_pos_left = right;
      tf->text.prim_pos_right = left;
   } else {
      tf->text.prim_pos_left = left;
      tf->text.prim_pos_right = right;
   }

   TextFieldSetHighlight(tf, tf->text.prim_pos_left,
	                    tf->text.prim_pos_right, XmHIGHLIGHT_SELECTED);

   if (redisplay) {
      if (old_prim_left > tf->text.prim_pos_left) {
         display_left = tf->text.prim_pos_left;
      } else if (old_prim_left < tf->text.prim_pos_left) {
         display_left = old_prim_left;
      } else
         display_left = (old_prim_right > tf->text.prim_pos_right) ?
		        tf->text.prim_pos_right : old_prim_right;

      if (old_prim_right < tf->text.prim_pos_right) {
         display_right = tf->text.prim_pos_right;
      } else if (old_prim_right > tf->text.prim_pos_right) {
         display_right = old_prim_right;
      } else
         display_right = (old_prim_left < tf->text.prim_pos_left) ?
		         tf->text.prim_pos_left : old_prim_left;

      RedisplayText(tf, display_left, display_right);
   }
   tf->text.refresh_ibeam_off = True;
}


/*
 * Begin the selection by gaining ownership of the selection
 * and setting the selection parameters.
 */
void 
#ifdef _NO_PROTO
_XmTextFieldStartSelection( tf, left, right, sel_time )
        XmTextFieldWidget tf ;
        XmTextPosition left ;
        XmTextPosition right ;
        Time sel_time ;
#else
_XmTextFieldStartSelection(
        XmTextFieldWidget tf,
        XmTextPosition left,
        XmTextPosition right,
        Time sel_time )
#endif /* _NO_PROTO */
{
  if (!XtIsRealized((Widget)tf)) return;

  /* if we don't already own the selection */
  if (!tf->text.has_primary) {
    /*
     * Try to gain ownership. This function identifies the
     * XtConvertSelectionProc and the XtLoseSelectionProc.
     */
     if (XtOwnSelection((Widget)tf, XA_PRIMARY, sel_time, _XmTextFieldConvert, 
		      _XmTextFieldLoseSelection, (XtSelectionDoneProc) NULL)) {
       XmAnyCallbackStruct cb;

       tf->text.prim_time = sel_time;
       _XmTextFieldDrawInsertionPoint(tf, False);
       tf->text.has_primary = True; 
       tf->text.prim_pos_left = tf->text.prim_pos_right =
			   tf->text.prim_anchor = TextF_CursorPosition(tf);
      /*
       * Set the selection boundries for highlighting the text,
       * and marking the selection.
       */
       SetSelection(tf, left, right, True);

       _XmTextFieldDrawInsertionPoint(tf, True);

      /* Call the gain selection callback */
       cb.reason = XmCR_GAIN_PRIMARY;
       cb.event = NULL;
       XtCallCallbackList((Widget) tf, tf->text.gain_primary_callback, 
			  (XtPointer) &cb);

    } else 
     /*
      * Failed to gain ownership of the selection so make sure
      * the text does not think it owns the selection.
      * (this might be overkill)
      */
       _XmTextFieldDeselectSelection((Widget)tf, True, sel_time);
  } else {
       _XmTextFieldDrawInsertionPoint(tf, False);
       XmTextFieldSetHighlight((Widget)tf, tf->text.prim_pos_left,
		          tf->text.prim_pos_right, XmHIGHLIGHT_NORMAL);
       tf->text.prim_pos_left = tf->text.prim_pos_right =
			   tf->text.prim_anchor = TextF_CursorPosition(tf);
      /*
       * Set the new selection boundries for highlighting the text,
       * and marking the selection.
       */
       SetSelection(tf, left, right, True);

       _XmTextFieldDrawInsertionPoint(tf, True);
  }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ProcessHorizontalParams( w, event, params, num_params, left, right, position )
        Widget w ;
        XEvent *event ;
        char **params ;
	Cardinal *num_params;
	XmTextPosition *left ;
	XmTextPosition *right ;
	XmTextPosition *position ;
#else
ProcessHorizontalParams(
        Widget w,
        XEvent *event,
        char **params,
	Cardinal *num_params,
	XmTextPosition *left,
	XmTextPosition *right,
        XmTextPosition *position )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition old_cursorPos = TextF_CursorPosition(tf);

    *position = TextF_CursorPosition(tf);

    if (!XmTextFieldGetSelectionPosition(w, left, right) || *left == *right) {
        tf->text.orig_left = tf->text.orig_right = tf->text.prim_anchor;
        *left = *right = old_cursorPos;
    }

    if (*num_params > 0 && !strcmp(*params, "right")) {
       if (*position >= tf->text.string_length) return;
       (*position)++;
    } else if (*num_params > 0 && !strcmp(*params, "left")) {
       if (*position <= 0) return;
       (*position)--;
    }
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
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   *position = TextF_CursorPosition(tf);

   if (!XmTextFieldGetSelectionPosition(w, left, right) || *left == *right) {
      if (*position > tf->text.prim_anchor) {
        *left = tf->text.prim_anchor;
        *right = *position;
      } else {
        *left = *position;
        *right = tf->text.prim_anchor;
      }
   }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KeySelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
KeySelection(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextPosition position, left, right;
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition cursorPos;

  _XmTextFieldDrawInsertionPoint(tf,False); /* Turn off I beam blink
					       during selection */

  tf->text.orig_left = tf->text.prim_pos_left;
  tf->text.orig_right = tf->text.prim_pos_right;

  cursorPos = TextF_CursorPosition(tf);
  if (*num_params > 0 && (!strcmp(*params,"right") || !strcmp(*params, "left")))
     SetAnchorBalancing(tf, cursorPos);

  tf->text.extending = True;

  if (*num_params == 0) {
     position = cursorPos;
     ProcessSelectParams(w, event, &left, &right, &position);
  } else if (*num_params > 0 && (!strcmp(*params, "right") ||
				 !strcmp(*params, "left"))) {
     ProcessHorizontalParams(w, event, params, num_params, &left,
			     &right, &position);
  }

  cursorPos = position;

  if (position < 0 || position > tf->text.string_length) {
     _XmTextFieldDrawInsertionPoint(tf,True); /* Turn on I beam now
						 that we are done */
     return;
  }

 /* shift anchor and direction to opposite end of the selection */
  if (position > tf->text.prim_anchor) {
     right = cursorPos = position;
     left = tf->text.prim_anchor;
  } else {
     left = cursorPos = position;
     right = tf->text.prim_anchor;
  }

  if (left > right) {
     XmTextPosition tempIndex = left;
     left = right;
     right = tempIndex;
  }

  if (tf->text.has_primary)
     SetSelection(tf, left, right, True);
  else
     _XmTextFieldStartSelection(tf, left, right, event->xbutton.time);

  tf->text.pending_off = False;

  _XmTextFieldSetCursorPosition(tf, event, cursorPos, True, True);
  (void) SetDestination(w, cursorPos, False, event->xkey.time);

  tf->text.orig_left = tf->text.prim_pos_left;
  tf->text.orig_right = tf->text.prim_pos_right;

  _XmTextFieldDrawInsertionPoint(tf,True); /* Turn on I beam now
					      that we are done */

}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextFocusIn( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TextFocusIn(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmAnyCallbackStruct cb;
   XPoint xmim_point;

   if (event->xfocus.send_event && !(tf->text.has_focus)) {
   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
      CheckHasRect(tf);

      if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
      tf->text.has_focus = True;
      if (XtSensitive(w)) _XmTextFToggleCursorGC(w);
      _XmTextFieldDrawInsertionPoint(tf, False);
      tf->text.blink_on = False;

      tf->text.refresh_ibeam_off = True;
      if (_XmGetFocusPolicy(w) == XmEXPLICIT) {
         if (((XmTextFieldWidgetClass)
		XtClass(w))->primitive_class.border_highlight) {   
            (*((XmTextFieldWidgetClass)
		      XtClass(w))->primitive_class.border_highlight)(w);
         } 
	 if (!tf->text.has_destination)
            (void) SetDestination(w, TextF_CursorPosition(tf), False,
				  XtLastTimestampProcessed(XtDisplay(w)));
      }
      if (tf->core.sensitive) ChangeBlinkBehavior(tf, True);
      _XmTextFieldDrawInsertionPoint(tf, True);
      (void) GetXYFromPos(tf, TextF_CursorPosition(tf),
			  &xmim_point.x, &xmim_point.y);
      XmImVaSetFocusValues(w, XmNspotLocation, &xmim_point, NULL);

      cb.reason = XmCR_FOCUS;
      cb.event = event;
      XtCallCallbackList (w, tf->text.focus_callback, (XtPointer) &cb);
   }

   _XmPrimitiveFocusIn(w, event, params, num_params);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextFocusOut( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TextFocusOut(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   if (event->xfocus.send_event && tf->text.has_focus) {
      tf->text.has_focus = False;
      ChangeBlinkBehavior(tf, False);
      _XmTextFieldDrawInsertionPoint(tf, False);
      _XmTextFToggleCursorGC(w);
      tf->text.blink_on = True;
      _XmTextFieldDrawInsertionPoint(tf, True);
      if(    ((XmTextFieldWidgetClass) XtClass(tf))
                                      ->primitive_class.border_unhighlight    )
         {   (*((XmTextFieldWidgetClass) XtClass(tf))
                          ->primitive_class.border_unhighlight)( (Widget) tf) ;
             } 
      XmImUnsetFocus(w);
   }

   /* If traversal is on, then the leave verification callback is called in
      the traversal event handler */
   if (event->xfocus.send_event && !tf->text.traversed &&
       _XmGetFocusPolicy(w) == XmEXPLICIT) {
        if (!VerifyLeave(tf, event)) {
           if (tf->text.verify_bell) XBell(XtDisplay(w), 0);
           return;
        }
   } else
        if (tf->text.traversed) {
	   tf->text.traversed = False;
        }
}

static void 
#ifdef _NO_PROTO
SetScanIndex( tf, event )
        XmTextFieldWidget tf ;
        XEvent *event ;
#else
SetScanIndex(
        XmTextFieldWidget tf,
        XEvent *event )
#endif /* _NO_PROTO */
{
   Time sel_time;

   if (event->type == ButtonPress) sel_time = event->xbutton.time;
   else sel_time = event->xkey.time;
	
	
   if (sel_time > tf->text.last_time &&
	sel_time - tf->text.last_time < XtGetMultiClickTime(XtDisplay(tf))) {
/*
 * Fix for HaL DTS 9841 - Increment the sarray_index first, then check to 
 *			  see if it is greater that the count.  Otherwise,
 *			  an error will occur.
 */
       tf->text.sarray_index++;
       if (tf->text.sarray_index >= TextF_SelectionArrayCount(tf)) {
	  tf->text.sarray_index = 0;
       }
/*
 * End fix for HaL DTS 9841
 */
    } else
       tf->text.sarray_index = 0;

    tf->text.last_time = sel_time;
}
    
static void 
#ifdef _NO_PROTO
ExtendScanSelection( tf, event )
        XmTextFieldWidget tf ;
        XEvent *event ;
#else
ExtendScanSelection(
        XmTextFieldWidget tf,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmTextPosition pivot_left, pivot_right;
   XmTextPosition left, right;
   XmTextPosition new_position = GetPosFromX(tf, (Position) event->xbutton.x);
   XmTextPosition cursorPos = TextF_CursorPosition(tf);
   Boolean pivot_modify = False;
   float bal_point;

   if (!XmTextFieldGetSelectionPosition((Widget)tf, &left, &right) ||
       left == right) {
       tf->text.orig_left = tf->text.orig_right = bal_point =
       tf->text.prim_anchor = TextF_CursorPosition(tf);
   } else
        bal_point = (float)(((float)(right - left) / 2.0) + (float)left);

   if (!tf->text.extending)
      if ((float)new_position < bal_point) {
         tf->text.prim_anchor = tf->text.orig_right;
      } else if ((float)new_position > bal_point) {
         tf->text.prim_anchor = tf->text.orig_left;
      }

   tf->text.extending = True;

   switch (TextF_SelectionArray(tf)[tf->text.sarray_index]) {
       case XmSELECT_POSITION:
      	   if (tf->text.has_primary)
	      SetSelection(tf, tf->text.prim_anchor, new_position, True);
           else if (new_position != tf->text.prim_anchor)
     	      _XmTextFieldStartSelection(tf, tf->text.prim_anchor,
			     new_position, event->xbutton.time);
           tf->text.pending_off = False;
           cursorPos = new_position;
           break;
       case XmSELECT_WHITESPACE:
       case XmSELECT_WORD:
	   FindWord(tf, new_position, &left, &right);
           FindWord(tf, tf->text.prim_anchor,
		    &pivot_left, &pivot_right);
           tf->text.pending_off = False;
           if (left != pivot_left || right != pivot_right) {
              if (left > pivot_left)
                 left = pivot_left;
              if (right < pivot_right)
                 right = pivot_right;
              pivot_modify = True;
           }
      	   if (tf->text.has_primary)
              SetSelection(tf, left, right, True);
      	   else
     	      _XmTextFieldStartSelection(tf, left, right, event->xbutton.time);

           if (pivot_modify) {
              if ((((right - left) / 2) + left) <= new_position) {
                 cursorPos = right;
              } else
                 cursorPos = left;
           } else {
	      if (left >= TextF_CursorPosition(tf))
                 cursorPos = left;
              else
                 cursorPos = right;
           }
           break;
       default:
	   break;
   }
   if (cursorPos != TextF_CursorPosition(tf)) {
      (void) SetDestination((Widget)tf, cursorPos, False, event->xkey.time);
      _XmTextFieldSetCursorPosition(tf, event, cursorPos, True, True);
   }
}

static void 
#ifdef _NO_PROTO
SetScanSelection( tf, event )
        XmTextFieldWidget tf ;
        XEvent *event ;
#else
SetScanSelection(
        XmTextFieldWidget tf,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmTextPosition left, right;
   XmTextPosition new_position = 0;
   XmTextPosition cursorPos = TextF_CursorPosition(tf);
   Position dummy = 0;
  
   SetScanIndex(tf, event);

   if (event->type == ButtonPress)
       new_position = GetPosFromX(tf, (Position) event->xbutton.x);
   else
       new_position = TextF_CursorPosition(tf);

   _XmTextFieldDrawInsertionPoint(tf,False); /* Turn off I beam
						blink during selection */

   switch (TextF_SelectionArray(tf)[tf->text.sarray_index]) {
       case XmSELECT_POSITION:
           tf->text.prim_anchor = new_position;
      	   if (tf->text.has_primary) {
              SetSelection(tf, new_position, new_position, True);
              tf->text.pending_off = False;
           }
           cursorPos = new_position;
           break;
       case XmSELECT_WHITESPACE:
       case XmSELECT_WORD:
	   FindWord(tf, TextF_CursorPosition(tf), &left, &right);
      	   if (tf->text.has_primary)
              SetSelection(tf, left, right, True);
      	   else
     	      _XmTextFieldStartSelection(tf, left, right, event->xbutton.time);
           tf->text.pending_off = False;
           if ((((right - left) / 2) + left) <= new_position)
              cursorPos = right;
           else
              cursorPos = left;
           break;
       case XmSELECT_LINE:
       case XmSELECT_PARAGRAPH:
       case XmSELECT_ALL:
      	   if (tf->text.has_primary)
              SetSelection(tf, 0, tf->text.string_length, True);
      	   else
              _XmTextFieldStartSelection(tf, 0, tf->text.string_length,
			     event->xbutton.time);
           tf->text.pending_off = False;
   	   if (event->type == ButtonPress)
              if ((tf->text.string_length) / 2 <= new_position)
                 cursorPos = tf->text.string_length;
              else
                 cursorPos = 0;
           break;
   }

   (void) SetDestination((Widget)tf, cursorPos, False, event->xkey.time);
   if (cursorPos != TextF_CursorPosition(tf)) {
      _XmTextFieldSetCursorPosition(tf, event, cursorPos, True, True);
   }
   GetXYFromPos(tf, cursorPos, &(tf->text.select_pos_x),
                &dummy);
   _XmTextFieldDrawInsertionPoint(tf,True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
StartPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
StartPrimary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;

  if (!tf->text.has_focus && _XmGetFocusPolicy(w) == XmEXPLICIT)
     (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

  _XmTextFieldDrawInsertionPoint(tf,False);
  SetScanSelection(tf, event); /* use scan type to set the selection */
  _XmTextFieldDrawInsertionPoint(tf,True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveDestination( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
MoveDestination(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition left, right;
  XmTextPosition new_position;
  Boolean old_has_focus = tf->text.has_focus;
  Boolean reset_cursor = False;

  new_position = GetPosFromX(tf, (Position) event->xbutton.x);

  _XmTextFieldDrawInsertionPoint(tf, False);
  if (XmTextFieldGetSelectionPosition(w, &left, &right) && (right != left))
     (void) SetDestination(w, new_position, False, event->xbutton.time);

  tf->text.pending_off = False;

  if (!tf->text.has_focus && _XmGetFocusPolicy(w) == XmEXPLICIT)
     (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

  /* Doing the the MoveDestination caused a traversal into my, causing
   * me to gain focus... Cursor is now on when it shouldn't be. */
  if ((reset_cursor = !old_has_focus && tf->text.has_focus) != False)
     _XmTextFieldDrawInsertionPoint(tf, False);

  _XmTextFieldSetCursorPosition(tf, event, new_position,
				True, True);
  if (new_position < left && new_position > right)
     tf->text.pending_off = True;

  /*
   * if cursor was turned off as a result of the focus state changing
   * then we need to undo the decrement to the cursor_on variable
   * by redrawing the insertion point.
   */
  if (reset_cursor)
     _XmTextFieldDrawInsertionPoint(tf, True);
  _XmTextFieldDrawInsertionPoint(tf, True);
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
       parameters: the action to call when the event is a selection,
       and the action to call when the event is a transfer. */

   if (*num_params != 2 || !XmIsTextField(w))
      return;
   if (XmTestInSelection((XmTextFieldWidget)w, event))
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
}
	

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
XmTestInSelection( w, event )
        XmTextFieldWidget w ;
        XEvent *event ;
#else
XmTestInSelection(
        XmTextFieldWidget w,
	XEvent *event )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition position, left, right;
    Position left_x, right_x, dummy;

    position = GetPosFromX(tf, (Position) event->xbutton.x);

    /*  First check if there is a selection at all */
    if (!(XmTextFieldGetSelectionPosition((Widget)w, &left, &right)))  {
	return(False);
    }
    /*  If left == right, then there is no selecion */
    if (left == right)
	return(False);
    /*  There is a selection - is this event inside it? */
    if (!(position >= left && position <= right))  {
	return(False);
    }
    /* or if it is part of a multiclick sequence */
    if (event->xbutton.time > tf->text.last_time &&
	    event->xbutton.time - tf->text.last_time <
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

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ExtendPrimary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;

  if (tf->text.cancel) return;

  _XmTextFieldDrawInsertionPoint(tf, False);
  tf->text.do_drop = False;

  if (!CheckTimerScrolling(w, event)){
     if (event->type == ButtonPress)
        DoExtendedSelection(w, event->xbutton.time);
     else
        DoExtendedSelection(w, event->xkey.time);
  } else
     ExtendScanSelection(tf, event); /* use scan type to set the selection */

  _XmTextFieldDrawInsertionPoint(tf, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendEnd( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ExtendEnd(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;

  if (tf->text.prim_pos_left == 0 && tf->text.prim_pos_right == 0)
     tf->text.orig_left = tf->text.orig_right = TextF_CursorPosition(tf);
  else {
     tf->text.orig_left = tf->text.prim_pos_left;
     tf->text.orig_right = tf->text.prim_pos_right;
     tf->text.cancel = False;
  }

  if (tf->text.select_id) {
     XtRemoveTimeOut(tf->text.select_id);
     tf->text.select_id = 0;
  }
    tf->text.select_pos_x = 0;
    tf->text.extending = False;
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
DoExtendedSelection(w, time)
        Widget w;
        Time time;
#else
DoExtendedSelection(
        Widget w,
        Time time )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition position, left, right, cursorPos;
    XmTextPosition pivot_left, pivot_right;
    Boolean pivot_modify = False;
    float bal_point;

    if (tf->text.cancel) {
          if (tf->text.select_id) XtRemoveTimeOut(tf->text.select_id);
          tf->text.select_id = 0;
          return;
    }

    cursorPos = TextF_CursorPosition(tf);
    _XmTextFieldDrawInsertionPoint(tf, False);
    if (!(XmTextFieldGetSelectionPosition(w, &left, &right)) || left == right) {
        tf->text.prim_anchor = tf->text.cursor_position;
        left = right = TextF_CursorPosition(tf);
        tf->text.orig_left = tf->text.orig_right = tf->text.prim_anchor;
        bal_point = tf->text.prim_anchor;
    } else
        bal_point = (float)(((float)(tf->text.orig_right - tf->text.orig_left)
                                     / 2.0) + (float)tf->text.orig_left);

    position = XmTextFieldXYToPos(w, tf->text.select_pos_x, 0);

   if (!tf->text.extending)
      if ((float)position < bal_point) {
         tf->text.prim_anchor = tf->text.orig_right;
      } else if ((float)position > bal_point) {
         tf->text.prim_anchor = tf->text.orig_left;
      }

   tf->text.extending = True;

    /* Extend selection in same way as ExtendScan would do */

   switch (TextF_SelectionArray(tf)[tf->text.sarray_index]) {
       case XmSELECT_POSITION:
           if (tf->text.has_primary)
              SetSelection(tf, tf->text.prim_anchor, position, True);
           else if (position != tf->text.prim_anchor)
              _XmTextFieldStartSelection(tf, tf->text.prim_anchor,
                             position, time);
           tf->text.pending_off = False;
           cursorPos = position;
           break;
       case XmSELECT_WHITESPACE:
       case XmSELECT_WORD:
           FindWord(tf, position, &left, &right);
           FindWord(tf, tf->text.prim_anchor,
                    &pivot_left, &pivot_right);
           tf->text.pending_off = False;
           if (left != pivot_left || right != pivot_right) {
              if (left > pivot_left)
                 left = pivot_left;
              if (right < pivot_right)
                 right = pivot_right;
              pivot_modify = True;
           }
           if (tf->text.has_primary)
              SetSelection(tf, left, right, True);
           else
              _XmTextFieldStartSelection(tf, left, right, time);

           if (pivot_modify) {
              if ((((right - left) / 2) + left) <= position) {
                 cursorPos = right;
              } else
                 cursorPos = left;
           } else {
              if (left >= TextF_CursorPosition(tf))
                 cursorPos = left;
              else
                 cursorPos = right;
           }
           break;
       default:
           break;
   }
   if (cursorPos != TextF_CursorPosition(tf)) {
      (void) SetDestination((Widget)tf, cursorPos, False, time);
      _XmTextFieldSetCursorPosition(tf, NULL, cursorPos, True, True);
   }
    _XmTextFieldDrawInsertionPoint(tf, True);
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    XmTextPosition position = XmTextFieldXYToPos(w, tf->text.select_pos_x, 0);

    if (tf->text.cancel) return;

    if (position < tf->text.sec_anchor) {
       if (tf->text.sec_pos_left > 0)
          _XmTextFieldSetSel2(w, position, tf->text.sec_anchor, False, ev_time);
       XmTextFieldShowPosition(w, tf->text.sec_pos_left);
    } else if (position > tf->text.sec_anchor) {
       if (tf->text.sec_pos_right < tf->text.string_length)
       _XmTextFieldSetSel2(w, tf->text.sec_anchor, position, False, ev_time);
       XmTextFieldShowPosition(w, tf->text.sec_pos_right);
    } else {
       _XmTextFieldSetSel2(w, position, position, False, ev_time);
       XmTextFieldShowPosition(w, position);
    }
    ResetClipOrigin(tf, False);

    tf->text.sec_extending = True;
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (tf->text.cancel) {
       tf->text.select_id = 0;
       return;
    }

    if (!tf->text.select_id) return;

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (tf->text.sec_extending)
      DoSecondaryExtend(w, XtLastTimestampProcessed(XtDisplay(w)));
    else if (tf->text.extending)
      DoExtendedSelection(w, XtLastTimestampProcessed(XtDisplay(w)));

    XSync (XtDisplay(w), False);

    _XmTextFieldDrawInsertionPoint(tf, True);

    tf->text.select_id = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
                                 (unsigned long) PRIM_SCROLL_INTERVAL,
                                 BrowseScroll, (XtPointer) w);
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Dimension margin_size = TextF_MarginWidth(tf) +
                               tf->primitive.shadow_thickness +
                               tf->primitive.highlight_thickness;
    Dimension top_margin = TextF_MarginHeight(tf) +
                            tf->primitive.shadow_thickness +
                            tf->primitive.highlight_thickness;

    tf->text.select_pos_x = event->xmotion.x;

    if ((event->xmotion.x > (int) margin_size) &&
      (event->xmotion.x < (int) (tf->core.width - margin_size))  &&
      (event->xmotion.y > (int) top_margin) &&
        (event->xmotion.y < (int) (top_margin + TextF_FontAscent(tf) +
                                 TextF_FontDescent(tf)))) {

       if (tf->text.select_id) {
          XtRemoveTimeOut(tf->text.select_id);
          tf->text.select_id = 0;
       }
    } else {
       /* to the left of the text */
        if (event->xmotion.x <= (int) margin_size)
           tf->text.select_pos_x = (Position) (margin_size -
                                          (tf->text.average_char_width + 1));
       /* to the right of the text */
      else if (event->xmotion.x >= (int) (tf->core.width - margin_size))
           tf->text.select_pos_x = (Position) ((tf->core.width - margin_size) +
                                           tf->text.average_char_width + 1);
       if (!tf->text.select_id)
          tf->text.select_id = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
                                       (unsigned long) SEC_SCROLL_INTERVAL,
                                       BrowseScroll, (XtPointer) w);
       return True;
    }
    return False;
}

static void 
#ifdef _NO_PROTO
RestorePrimaryHighlight( tf, prim_left, prim_right )
        XmTextFieldWidget tf ;
        XmTextPosition prim_left ;
        XmTextPosition prim_right ;
#else
RestorePrimaryHighlight(
        XmTextFieldWidget tf,
        XmTextPosition prim_left,
        XmTextPosition prim_right )
#endif /* _NO_PROTO */
{
   if (tf->text.sec_pos_right >= prim_left &&
       tf->text.sec_pos_right <= prim_right) {
     /* secondary selection is totally inside primary selection */
      if (tf->text.sec_pos_left >= prim_left) {
         TextFieldSetHighlight(tf, prim_left, tf->text.sec_pos_left,
                            XmHIGHLIGHT_SELECTED);
         TextFieldSetHighlight(tf, tf->text.sec_pos_left,
                               tf->text.sec_pos_right,
                               XmHIGHLIGHT_NORMAL);
         TextFieldSetHighlight(tf, tf->text.sec_pos_right, prim_right,
                               XmHIGHLIGHT_SELECTED);
     /* right side of secondary selection is inside primary selection */
      } else {
         TextFieldSetHighlight(tf, tf->text.sec_pos_left, prim_left,
                               XmHIGHLIGHT_NORMAL);
         TextFieldSetHighlight(tf, prim_left, tf->text.sec_pos_right,
                               XmHIGHLIGHT_SELECTED);
      }
   } else {
     /* left side of secondary selection is inside primary selection */
      if (tf->text.sec_pos_left <= prim_right &&
	  tf->text.sec_pos_left >= prim_left) {
         TextFieldSetHighlight(tf, tf->text.sec_pos_left, prim_right,
                               XmHIGHLIGHT_SELECTED);
         TextFieldSetHighlight(tf, prim_right, tf->text.sec_pos_right,
                               XmHIGHLIGHT_NORMAL);
      } else  {
       /* secondary selection encompasses the primary selection */
        if (tf->text.sec_pos_left <= prim_left &&
            tf->text.sec_pos_right >= prim_right){
           TextFieldSetHighlight(tf, tf->text.sec_pos_left, prim_left,
                                 XmHIGHLIGHT_NORMAL);
           TextFieldSetHighlight(tf, prim_left, prim_right,
                                 XmHIGHLIGHT_SELECTED);
           TextFieldSetHighlight(tf, prim_right, tf->text.sec_pos_right,
                                 XmHIGHLIGHT_NORMAL);
     /* secondary selection is outside primary selection */
        } else {
           TextFieldSetHighlight(tf, prim_left, prim_right,
                                 XmHIGHLIGHT_SELECTED);
           TextFieldSetHighlight(tf, tf->text.sec_pos_left,
                                 tf->text.sec_pos_right,
                                 XmHIGHLIGHT_NORMAL);
        }
      }
   }
}

void 
#ifdef _NO_PROTO
_XmTextFieldSetSel2( w, left, right, disown, sel_time )
        Widget w ;
        XmTextPosition left ;
        XmTextPosition right ;
        Boolean disown ;
        Time sel_time ;
#else
_XmTextFieldSetSel2(
        Widget w,
        XmTextPosition left,
        XmTextPosition right,
#if NeedWidePrototypes
        int disown,
#else
        Boolean disown,
#endif /* NeedWidePrototypes */
        Time sel_time )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Boolean result;

    if (tf->text.has_secondary) {
       XmTextPosition prim_left, prim_right;

       if (left == tf->text.sec_pos_left && right == tf->text.sec_pos_right)
          return;

      /* If the widget has the primary selection, make sure the selection
       * highlight is restored appropriately.
       */
       if (XmTextFieldGetSelectionPosition(w, &prim_left, &prim_right))
          RestorePrimaryHighlight(tf, prim_left, prim_right);
       else
          TextFieldSetHighlight(tf, tf->text.sec_pos_left,
		                  tf->text.sec_pos_right, XmHIGHLIGHT_NORMAL);
    }

    if (left < right) {
       if (!tf->text.has_secondary) {
          result = XtOwnSelection(w, XA_SECONDARY, sel_time, 
				  _XmTextFieldConvert,
			          _XmTextFieldLoseSelection,
				  (XtSelectionDoneProc) NULL);
          tf->text.sec_time = sel_time;
          tf->text.has_secondary = result;
          if (result) {
             tf->text.sec_pos_left = left;
             tf->text.sec_pos_right = right;
          } 
       } else {
          tf->text.sec_pos_left = left;
          tf->text.sec_pos_right = right;
       }
       tf->text.sec_drag = True;
   } else {
       tf->text.sec_pos_left = tf->text.sec_pos_right = left;
       if (disown) {
          XtDisownSelection(w, XA_SECONDARY, sel_time);
          tf->text.has_secondary = False;
       }
   }

   TextFieldSetHighlight((XmTextFieldWidget) w, tf->text.sec_pos_left,
		      tf->text.sec_pos_right, XmHIGHLIGHT_SECONDARY_SELECTED);

  /* This can be optimized for performance enhancement */

    RedisplayText(tf, 0, tf->text.string_length);
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Atom targets[4];
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    int status = 0;
    Cardinal num_targets = 0;
    Widget drag_icon;
    Arg args[10];
    int n;

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
#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif
    targets[num_targets++] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    targets[num_targets++] = XA_STRING;
    targets[num_targets++] = XmInternAtom(XtDisplay(w), "TEXT", False);

    drag_icon = _XmGetTextualDragIcon(w);

    n = 0;
    XtSetArg(args[n], XmNcursorBackground, tf->core.background_pixel);  n++;
    XtSetArg(args[n], XmNcursorForeground, tf->primitive.foreground);  n++;
    XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++;
    XtSetArg(args[n], XmNexportTargets, targets);  n++;
    XtSetArg(args[n], XmNnumExportTargets, num_targets);  n++;
    XtSetArg(args[n], XmNconvertProc, _XmTextFieldConvert);  n++;
    XtSetArg(args[n], XmNclientData, w);  n++;
    if (TextF_Editable(tf)) {
       XtSetArg(args[n], XmNdragOperations, (XmDROP_MOVE | XmDROP_COPY)); n++;
    } else {
       XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
    }
    (void) XmDragStart(w, event, args, n);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
StartSecondary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
StartSecondary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition position = GetPosFromX(tf, (Position) event->xbutton.x);
  int status;

  tf->text.sec_anchor = position;
  tf->text.selection_move = FALSE;

  status = XtGrabKeyboard(w, False, GrabModeAsync, GrabModeAsync,
			  event->xbutton.time);

  if (status != GrabSuccess)
     _XmWarning(w, GRABKBDERROR);
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition position, left, right;
    Position left_x, right_x, dummy;

    position = GetPosFromX(tf, (Position) event->xbutton.x);

    tf->text.sec_pos_left = position;

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (XmTextFieldGetSelectionPosition(w, &left, &right) && 
	left != right) {
        if ((position > left && position < right) ||
	  /* Take care of border conditions */
	   (position == left &&
            GetXYFromPos(tf, left, &left_x, &dummy) &&
	    event->xbutton.x > left_x) ||
	   (position == right &&
            GetXYFromPos(tf, right, &right_x, &dummy) &&
	    event->xbutton.x < right_x)) {
           tf->text.sel_start = False;
           StartDrag(w, event, params, num_params);
	} else {
	   tf->text.sel_start = True;
	   XAllowEvents(XtDisplay(w), AsyncBoth, event->xbutton.time);
	   StartSecondary(w, event, params, num_params);
	}
    } else {
       tf->text.sel_start = True;
       XAllowEvents(XtDisplay(w), AsyncBoth, event->xbutton.time);
       StartSecondary(w, event, params, num_params);
    }
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendSecondary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ExtendSecondary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XmTextPosition position = GetPosFromX(tf, (Position) event->xbutton.x);

  if (tf->text.cancel) return;

  _XmTextFieldDrawInsertionPoint(tf, False);
  if (position < tf->text.sec_anchor) {
     _XmTextFieldSetSel2(w, position, tf->text.sec_anchor,
			 False, event->xbutton.time);
  } else if (position > tf->text.sec_anchor) {
     _XmTextFieldSetSel2(w, tf->text.sec_anchor, position, 
			 False, event->xbutton.time);
  } else {
     _XmTextFieldSetSel2(w, position, position, False, event->xbutton.time);
  }

  tf->text.sec_extending = True;

  if (!CheckTimerScrolling(w, event))
     DoSecondaryExtend(w, event->xmotion.time);

  _XmTextFieldDrawInsertionPoint(tf, True);
}


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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    _XmTextPrimSelect *prim_select = (_XmTextPrimSelect *) closure;
    Atom NULL_ATOM = XmInternAtom(XtDisplay(w), "NULL", False);
    XmTextPosition right, left;
    int prim_char_length = 0;
    Boolean replace_res = False;
    XTextProperty tmp_prop;
    int i, status;
    int malloc_size;
    int num_vals;
    char **tmp_value;
    XmAnyCallbackStruct cb;

    if (!tf->text.has_focus && _XmGetFocusPolicy(w) == XmEXPLICIT)
       (void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);

    if (!(*length) && *type != NULL_ATOM ) {
      /* Backwards compatibility for 1.0 Selections */
       if (prim_select->target == XmInternAtom(XtDisplay(w), "TEXT", False)) {
          prim_select->target = XA_STRING;
          XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, DoStuff,
                           (XtPointer)prim_select, prim_select->time);
       } else {
        if (--prim_select->ref_count == 0)
          XtFree((char*)prim_select);
       }
       XtFree((char *)value);
       value = NULL;
       return;
    }

   /* if length == 0 and *type is the NULL atom we are assuming
    * that a DELETE target is requested.
    */
    if (*type == NULL_ATOM ) {
       if (prim_select->num_chars > 0 && tf->text.selection_move) {
          prim_char_length = prim_select->num_chars;
          XmTextFieldSetSelection(w, prim_select->position,
				  prim_select->position + prim_char_length,
				  prim_select->time);
          tf->text.prim_anchor = prim_select->position;
          (void) SetDestination(w, TextF_CursorPosition(tf),
				False, prim_select->time);
       }
    } else {
       int max_length = 0;
       Boolean local = tf->text.has_primary;

       if (tf->text.selection_move && local) {
          max_length = TextF_MaxLength(tf);
          TextF_MaxLength(tf) = MAXINT;
       }

       if (*type == XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False) ||
           *type == XA_STRING) {
	  tmp_prop.value = (unsigned char *) value;
	  tmp_prop.encoding = *type;
	  tmp_prop.format = *format;
	  tmp_prop.nitems = *length;
	  num_vals = 0;
	  status = XmbTextPropertyToTextList(XtDisplay(w), &tmp_prop,
					     &tmp_value, &num_vals);
	 /* if no conversion, num_vals is not changed */
	 /* status will be >0 if some characters could not be converted */
	  if (num_vals && (status == Success || status > 0)) {
	     if (tf->text.max_char_size == 1){
		char * total_tmp_value;

		for (i = 0, malloc_size = 1; i < num_vals ; i++)
		   malloc_size += strlen(tmp_value[i]);
		prim_select->num_chars = malloc_size - 1;
		total_tmp_value = XtMalloc ((unsigned) malloc_size);
		total_tmp_value[0] = '\0';
		for (i = 0; i < num_vals ; i++)
		   strcat(total_tmp_value, tmp_value[i]);
		replace_res = _XmTextFieldReplaceText(tf, NULL, 
						      prim_select->position,
						      prim_select->position, 
						      total_tmp_value,
						      strlen(total_tmp_value),
						      False);
		XFreeStringList(tmp_value);
		XtFree(total_tmp_value);
	     } else {
		wchar_t * wc_value;
		int return_val = 0;

		prim_select->num_chars = 0;
		for (i = 0, malloc_size = sizeof(wchar_t); i < num_vals ; i++)
		   malloc_size += strlen(tmp_value[i]) * sizeof(wchar_t);
		wc_value = (wchar_t*)XtMalloc ((unsigned) malloc_size);
		for (i = 0; i < num_vals ; i++) {
		   return_val = mbstowcs(wc_value + prim_select->num_chars,
				             tmp_value[i],
				             (size_t)malloc_size -
					     prim_select->num_chars);
		   if (return_val > 0) prim_select->num_chars += return_val;
		}
		replace_res = _XmTextFieldReplaceText(tf, NULL, 
						      prim_select->position,
						      prim_select->position,
						      (char*)wc_value, 
						      prim_select->num_chars,
						      False);
		XtFree((char*)wc_value);
	     }
	  } else { /* initialize prim_select values for possible delete oper */
	     prim_select->num_chars = 0;
	  }
       } else {
	     if (tf->text.max_char_size == 1){
               /* Note: *length may be truncated during cast to int */
		prim_select->num_chars = (int) *length;
		replace_res = _XmTextFieldReplaceText(tf, NULL, 
						      prim_select->position,
						      prim_select->position, 
						      (char *) value, 
						      prim_select->num_chars,
						      False);
	     } else {
		wchar_t * wc_value;

		wc_value = (wchar_t*)XtMalloc ((unsigned)
                                               (*length * sizeof(wchar_t)));
		prim_select->num_chars = mbstowcs(wc_value, (char *) value,
					 (size_t) *length);
		if (prim_select->num_chars > 0)
		   replace_res = _XmTextFieldReplaceText(tf, NULL, 
						         prim_select->position,
						         prim_select->position,
						         (char*)wc_value, 
						         prim_select->num_chars,
						         False);
		XtFree((char*)wc_value);
	     }
       }

       if (replace_res) {
          XmTextPosition cursorPos;

          tf->text.pending_off = FALSE;
          cursorPos = prim_select->position + prim_select->num_chars; 
	  if (prim_select->num_chars > 0 && !tf->text.selection_move){
		(void) SetDestination(w, cursorPos, False, prim_select->time);
		_XmTextFieldSetCursorPosition(tf, NULL, cursorPos, 
					      True, True);
	  }
	  if (XmTextFieldGetSelectionPosition(w, &left, &right)) {
             if (tf->text.selection_move && left < prim_select->position)
                prim_select->position -= prim_select->num_chars;
             if (left <= cursorPos && right >= cursorPos)
                tf->text.pending_off = TRUE;
          } else {
             if (!tf->text.selection_move && !tf->text.add_mode &&
                 prim_select->num_chars != 0)
                tf->text.prim_anchor = prim_select->position;
          }
	  if (tf->text.selection_move) {
              prim_select->ref_count++;
              XtGetSelectionValue(w, XA_PRIMARY,
                                  XmInternAtom(XtDisplay(w), "DELETE", False),
                                  DoStuff, (XtPointer)prim_select,
                                  prim_select->time);
           }
           cb.reason = XmCR_VALUE_CHANGED;
           cb.event = (XEvent *)NULL;
           XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		              (XtPointer) &cb);
       }

       if (tf->text.selection_move && local) {
          TextF_MaxLength(tf) = max_length;
       }
    }

    XtFree((char *)value);
    value = NULL;
    if (--prim_select->ref_count == 0)
       XtFree((char*)prim_select);
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Stuff( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
Stuff(
        Widget w,
        XEvent *event,
        char **params,
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
		      HandleTargets,
		      (XtPointer)tmp, event->xbutton.time);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleSelectionReplies( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
HandleSelectionReplies(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   Atom property = (Atom) closure;
   TextFDestData dest_data;
   XmTextPosition left, right;
   int adjustment = 0;
   XmAnyCallbackStruct cb;

   if (event->type != SelectionNotify) return;

   XtRemoveEventHandler(w, (EventMask) NULL, TRUE,
                        HandleSelectionReplies,
		       (XtPointer) XmInternAtom(XtDisplay(w),
						"_XM_TEXT_I_S_PROP", False));

   dest_data = GetTextFDestData(w);

   if (event->xselection.property == None) {
      (void) _XmTextFieldSetSel2(w, 0, 0, False, event->xselection.time);
      tf->text.selection_move = False;
   } else {
      if (dest_data->has_destination) {
         adjustment = (int) (tf->text.sec_pos_right - tf->text.sec_pos_left);

         XmTextFieldSetHighlight(w, tf->text.sec_pos_left,
		                 tf->text.sec_pos_right, XmHIGHLIGHT_NORMAL);
         if (dest_data->position <= tf->text.sec_pos_left) {
            tf->text.sec_pos_left += adjustment - dest_data->replace_length;
            tf->text.sec_pos_right += adjustment - dest_data->replace_length;
         } else if (dest_data->position > tf->text.sec_pos_left &&
                    dest_data->position < tf->text.sec_pos_right) {
            tf->text.sec_pos_left -= dest_data->replace_length;
            tf->text.sec_pos_right += adjustment - dest_data->replace_length;
         }
      }

      left = tf->text.sec_pos_left;
      right = tf->text.sec_pos_right;

      (void) _XmTextFieldSetSel2(w, 0, 0, False, event->xselection.time);
      tf->text.has_secondary = False;

      if (tf->text.selection_move) {
         if (_XmTextFieldReplaceText(tf, event, left, right, NULL, 0, False)) {
           if (dest_data->has_destination && TextF_CursorPosition(tf) > right){
              XmTextPosition cursorPos;
              cursorPos = TextF_CursorPosition(tf) - (right - left);
              if (!dest_data->quick_key)
                _XmTextFieldSetCursorPosition(tf, event, cursorPos,
					      True, True);
              (void) SetDestination((Widget) tf, cursorPos, False,
                                                       event->xselection.time);
           }
           if (!dest_data->has_destination) {
               tf->text.prim_anchor = TextF_CursorPosition(tf);
               XmTextFieldSetAddMode(w, False);	
           }
           cb.reason = XmCR_VALUE_CHANGED;
           cb.event = event;
           XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		              (XtPointer) &cb);
         }
         tf->text.selection_move = False;
      }
   }

   XDeleteProperty(XtDisplay(w), event->xselection.requestor, property);
}


/*
 * Notify the primary selection that the secondary selection
 * wants to insert it's selection data into the primary selection.
 */
   /* REQUEST TARGETS FROM SELECTION RECEIVER; THEN CALL HANDLETARGETS
    * WHICH LOOKS AT THE TARGET LIST AND DETERMINE WHAT TARGET TO PLACE 
    * IN THE PAIR.  IT WILL THEN DO ANY NECESSARY CONVERSIONS BEFORE 
    * TELLING THE RECEIVER WHAT TO REQUEST AS THE SELECTION VALUE.
    * THIS WILL GUARANTEE THE BEST CHANCE AT A SUCCESSFUL EXCHANGE.
    */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SecondaryNotify( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
SecondaryNotify(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Atom XM_TEXT_PROP = XmInternAtom(XtDisplay(w), "_XM_TEXT_I_S_PROP", False);
    Atom CS_OF_LOCALE; /* to be initialized by XmbTextListToTextProperty */
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    TextFDestData dest_data;
    XTextProperty tmp_prop;
    _XmTextInsertPair tmp_pair[1];
    _XmTextInsertPair *pair = tmp_pair;
    XmTextPosition left, right;
    int status = 0;

    if (tf->text.selection_move == TRUE && tf->text.has_destination &&
        TextF_CursorPosition(tf) >= tf->text.sec_pos_left &&
        TextF_CursorPosition(tf) <= tf->text.sec_pos_right) {
       (void) _XmTextFieldSetSel2(w, 0, 0, False, event->xbutton.time);
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
       CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
                                 * characters.  But just in case someones
                                 * Xlib is broken, this prevents a core dump.
                                 */
#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif

   /*
    * Determine what the reciever supports so you can tell 'em what to
    * request.
    */

   /* fill in atom pair */
    pair->selection = XA_SECONDARY;
    pair->target = CS_OF_LOCALE;

   /* add the insert selection property on the text field widget's window */
    XChangeProperty(XtDisplay(w), XtWindow(w), XM_TEXT_PROP, 
    		    XmInternAtom(XtDisplay(w), "ATOM_PAIR", False),
		    32, PropModeReplace, (unsigned char *)pair, 2);

    dest_data = GetTextFDestData(w);

    dest_data->has_destination = tf->text.has_destination;
    dest_data->position = TextF_CursorPosition(tf);
    dest_data->replace_length = 0;

    if (*(num_params) == 1) dest_data->quick_key = True;
    else dest_data->quick_key = False;

    if (XmTextFieldGetSelectionPosition(w, &left, &right) && left != right) {
       if (dest_data->position >= left && dest_data->position <= right)
          dest_data->replace_length = (int) (right - left);
    }

   /* add an event handler to handle selection notify events */
    XtAddEventHandler(w, (EventMask) NULL, TRUE,
		      HandleSelectionReplies, (XtPointer)XM_TEXT_PROP);

   /*
    * Make a request for the primary selection to convert to 
    * type INSERT_SELECTION as per ICCCM.
    */ 
    XConvertSelection(XtDisplay(w),
    		      XmInternAtom(XtDisplay(w), "MOTIF_DESTINATION", False),
    		      XmInternAtom(XtDisplay(w), "INSERT_SELECTION", False),
                      XM_TEXT_PROP, XtWindow(w), event->xbutton.time);
}

   /*
    * LOOKS AT THE TARGET LIST AND DETERMINE WHAT TARGET TO PLACE 
    * IN THE PAIR.  IT WILL THEN DO ANY NECESSARY CONVERSIONS BEFORE 
    * TELLING THE RECEIVER WHAT TO REQUEST AS THE SELECTION VALUE.
    * THIS WILL GUARANTEE THE BEST CHANCE AT A SUCCESSFUL EXCHANGE.
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Atom CS_OF_LOCALE; /* to be initialized by XmbTextListToTextProperty */
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w),"COMPOUND_TEXT", False);
    XmTextPosition left, right;
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
    int i;

    if (!length) {
       XtFree((char *)value);
       value = NULL;
       XtFree((char *)tmp_action->event);
       XtFree((char *)tmp_action);
       return; /* Supports no targets, so don't bother sending anything */
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
       CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
                                 * characters.  But just in case someones
                                 * Xlib is broken, this prevents a core dump.
                                 */

#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif

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
      select_pos =  GetPosFromX(tf, (Position)tmp_action->event->xbutton.x);
   } else {
      select_pos = TextF_CursorPosition(tf);
   }

   if (XmTextFieldGetSelectionPosition(w, &left, &right) &&
       left != right && select_pos > left && select_pos < right) {
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XButtonEvent      *ev = (XButtonEvent *) event;
    XmTextPosition position;

   /* Work around for intrinsic bug.  Remove once bug is fixed. */
    XtUngrabPointer(w, ev->time);

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (!tf->text.cancel) XtUngrabKeyboard(w, CurrentTime);

    position = GetPosFromX(tf, (Position) event->xbutton.x);

    if (tf->text.sel_start) {
       if (tf->text.has_secondary &&
	       tf->text.sec_pos_left != tf->text.sec_pos_right) {
          if (ev->x > tf->core.width || ev->x < 0 ||
	      ev->y > tf->core.height || ev->x < 0) {
             _XmTextFieldSetSel2(w, 0, 0, False, event->xkey.time);
             tf->text.has_secondary = False;
          } else {
	     SecondaryNotify(w, event, params, num_params);
          }
       } else if (!tf->text.sec_drag && !tf->text.cancel &&
		  tf->text.sec_pos_left == position) {
	  tf->text.stuff_pos =  GetPosFromX(tf, (Position) event->xbutton.x);
	/*
	 * Copy contents of primary selection to the stuff position found above.
	 */
	  Stuff(w, event, params, num_params);
       }
    }

    if (tf->text.select_id) {
       XtRemoveTimeOut(tf->text.select_id);
       tf->text.select_id = 0;
    }

    tf->text.sec_extending = False;

    tf->text.sec_drag = False;
    tf->text.sel_start = False;
    tf->text.cancel = False;
    _XmTextFieldDrawInsertionPoint(tf, True);
}

static void 
#ifdef _NO_PROTO
ProcessCopy( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ProcessCopy(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    tf->text.selection_move = FALSE;
    ProcessBDragRelease(w, event, params, num_params);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

static void 
#ifdef _NO_PROTO
ProcessMove( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ProcessMove(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    tf->text.selection_move = TRUE;
    ProcessBDragRelease(w, event, params, num_params);
    _XmTextFieldDrawInsertionPoint(tf, True);
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeleteSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeleteSelection(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    (void) TextFieldRemove(w, event);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ClearSelection( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ClearSelection(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition left = tf->text.prim_pos_left;
    XmTextPosition right = tf->text.prim_pos_right;
    int num_spaces = 0;
    XmAnyCallbackStruct cb;
    Boolean rep_result = False;

    if (left < right)
       num_spaces = (int)(right - left);
    else
       num_spaces = (int)(left - right);

    if (num_spaces) {
       _XmTextFieldDrawInsertionPoint(tf, False);
       if (tf->text.max_char_size == 1){
          char spaces_cache[100];
          Cardinal spaces_size;
          char *spaces;
          int i;

          spaces_size = num_spaces + 1;

          spaces = (char *)XmStackAlloc(spaces_size, spaces_cache);

          for (i = 0; i < num_spaces; i++) spaces[i] = ' ';
          spaces[num_spaces] = 0;

	  rep_result = _XmTextFieldReplaceText(tf, (XEvent *)event, left, right,
				               spaces, num_spaces, False);
          if (TextF_CursorPosition(tf) > left)
	     ResetClipOrigin(tf, False);     
          XmStackFree(spaces, spaces_cache);
       } else {
          wchar_t *wc_spaces;
          int i;

          wc_spaces = (wchar_t *)XtMalloc((unsigned)
                                          (num_spaces + 1) * sizeof(wchar_t));

          for (i = 0; i < num_spaces; i++){
             (void)mbtowc(&wc_spaces[i], " ", 1);
          }
          
	  rep_result = _XmTextFieldReplaceText(tf, (XEvent *)event, left, right,
					       (char*)wc_spaces, num_spaces,
					       False);
          if (TextF_CursorPosition(tf) > left)
	     ResetClipOrigin(tf, False);     

          XtFree((char*)wc_spaces);
       }
       if (rep_result) {
          cb.reason = XmCR_VALUE_CHANGED;
          cb.event = event;
          XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		             (XtPointer) &cb);
       }
       _XmTextFieldDrawInsertionPoint(tf, True);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageRight( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
PageRight(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    Position x, y;
    int length = 0;
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Dimension margin_width = TextF_MarginWidth(tf) +
	                     tf->primitive.shadow_thickness +
			     tf->primitive.highlight_thickness;

    if (tf->text.max_char_size != 1){
       length = FindPixelLength(tf, (char*)TextF_WcValue(tf),
				tf->text.string_length);
    } else {
       length = FindPixelLength(tf, TextF_Value(tf), tf->text.string_length);
    }

    _XmTextFieldDrawInsertionPoint(tf, False);

    if (*num_params > 0 && !strcmp(*params, "extend"))
       SetAnchorBalancing(tf, TextF_CursorPosition(tf));

    GetXYFromPos(tf, TextF_CursorPosition(tf), &x, &y);

    if (length - ((int)(tf->core.width - (2 * margin_width)) -
	 tf->text.h_offset) > tf->core.width - (2 * margin_width))
       tf->text.h_offset -= tf->core.width - (2 * margin_width);
    else
       tf->text.h_offset = -(length - (tf->core.width - (2 * margin_width)));

    RedisplayText(tf, 0, tf->text.string_length);
    _XmTextFieldSetCursorPosition(tf, event, GetPosFromX(tf, x), 
				  True, True);

    if (*num_params > 0 && !strcmp(*params, "extend"))
       KeySelection(w, event, params, num_params);

    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PageLeft( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
PageLeft(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    Position x, y;
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    int margin_width = (int)TextF_MarginWidth(tf) +
	                     tf->primitive.shadow_thickness +
			     tf->primitive.highlight_thickness;

    _XmTextFieldDrawInsertionPoint(tf, False);

    if (*num_params > 0 && !strcmp(*params, "extend"))
       SetAnchorBalancing(tf, TextF_CursorPosition(tf));

    GetXYFromPos(tf, TextF_CursorPosition(tf), &x, &y);
    if (margin_width  <= tf->text.h_offset +
			    ((int)tf->core.width - (2 * margin_width)))
       tf->text.h_offset = margin_width;
    else
       tf->text.h_offset += tf->core.width - (2 * margin_width);

    RedisplayText(tf, 0, tf->text.string_length);
    _XmTextFieldSetCursorPosition(tf, event, GetPosFromX(tf, x),
				  True, True);

    if (*num_params > 0 && !strcmp(*params, "extend"))
       KeySelection(w, event, params, num_params);

    _XmTextFieldDrawInsertionPoint(tf, True);
}

static void 
#ifdef _NO_PROTO
CopyPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
CopyPrimary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    tf->text.selection_move = False;

   /* perform the primary paste action */
    Stuff(w, event, params, num_params);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

static void 
#ifdef _NO_PROTO
CutPrimary( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
CutPrimary(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    tf->text.selection_move = True;
    Stuff(w, event, params, num_params);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SetAnchor( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
SetAnchor(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition left, right;

    tf->text.prim_anchor = TextF_CursorPosition(tf);
    (void) SetDestination(w, tf->text.prim_anchor, False, event->xkey.time);
    if (XmTextFieldGetSelectionPosition(w, &left, &right)) {
       _XmTextFieldStartSelection(tf, tf->text.prim_anchor,
                                  tf->text.prim_anchor, event->xkey.time);
       XmTextFieldSetAddMode(w, False);
    }
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
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    tf->text.overstrike = !tf->text.overstrike;
    tf->text.refresh_ibeam_off = True;
    if (tf->text.overstrike)
      tf->text.cursor_width = tf->text.cursor_height >> 1;
    else {
      tf->text.cursor_width = 5;
      if (tf->text.cursor_height > 19) 
	tf->text.cursor_width++;
      ResetClipOrigin(tf, False);
    }
    _XmTextFToggleCursorGC(w);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ToggleAddMode( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
ToggleAddMode(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition left, right;

    _XmTextFieldDrawInsertionPoint(tf, False);

    XmTextFieldSetAddMode(w, !tf->text.add_mode);	
    if (tf->text.add_mode &&
        (!(XmTextFieldGetSelectionPosition(w, &left, &right)) ||
	 left == right))
       tf->text.prim_anchor = TextF_CursorPosition(tf);

    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SelectAll( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
SelectAll(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    if (tf->text.has_primary)
       SetSelection(tf, 0, tf->text.string_length, True);
    else
       _XmTextFieldStartSelection(tf, 0, tf->text.string_length,
           	      		  event->xbutton.time);

    /* Call _XmTextFieldSetCursorPosition to force image gc to be updated
     * in case the i-beam is contained within the selection */

    tf->text.pending_off = False;

    _XmTextFieldSetCursorPosition(tf, NULL, TextF_CursorPosition(tf),
				  False, False);
    tf->text.prim_anchor = 0;

    (void) SetDestination(w, TextF_CursorPosition(tf),
			  False, event->xkey.time);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DeselectAll( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
DeselectAll(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    SetSelection(tf, TextF_CursorPosition(tf), TextF_CursorPosition(tf), True);
    tf->text.pending_off = True;
    _XmTextFieldSetCursorPosition(tf, event, TextF_CursorPosition(tf),
				  True, True);
    tf->text.prim_anchor = TextF_CursorPosition(tf);
    (void) SetDestination(w, TextF_CursorPosition(tf),
			  False, event->xkey.time);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
VoidAction( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
VoidAction(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  /* Do Nothing */
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CutClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
CutClipboard(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    _XmTextFieldDrawInsertionPoint((XmTextFieldWidget)w, False);
    (void) XmTextFieldCut(w, event->xkey.time);
    _XmTextFieldDrawInsertionPoint((XmTextFieldWidget)w, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CopyClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
CopyClipboard(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldDrawInsertionPoint(tf, False);
    (void) XmTextFieldCopy(w, event->xkey.time);
    (void) SetDestination(w, TextF_CursorPosition(tf), False, event->xkey.time);
    _XmTextFieldDrawInsertionPoint(tf, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PasteClipboard( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
PasteClipboard(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    _XmTextFieldDrawInsertionPoint((XmTextFieldWidget)w, False);
    (void) XmTextFieldPaste(w);
    _XmTextFieldDrawInsertionPoint((XmTextFieldWidget)w, True);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseDown( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TraverseDown(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (tf->primitive.navigation_type == XmNONE && VerifyLeave(tf, event)) {
       tf->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_DOWN))
          tf->text.traversed = False;
    }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseUp( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TraverseUp(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (tf->primitive.navigation_type == XmNONE && VerifyLeave(tf, event)) {
       tf->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_UP))
          tf->text.traversed = False;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseHome( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TraverseHome(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

   /* Allow the verification routine to control the traversal */
    if (tf->primitive.navigation_type == XmNONE && VerifyLeave(tf, event)) {
       tf->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_HOME))
          tf->text.traversed = False;
    }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraverseNextTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TraverseNextTabGroup(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

   /* Allow the verification routine to control the traversal */
    if (VerifyLeave(tf, event)) {
       tf->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP))
          tf->text.traversed = False;
    }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TraversePrevTabGroup( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        char **params ;
        Cardinal *num_params ;
#else
TraversePrevTabGroup(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

   /* Allow the verification routine to control the traversal */
    if (VerifyLeave(tf, event)) {
       tf->text.traversed = True;
       if (!_XmMgrTraversal(w, XmTRAVERSE_PREV_TAB_GROUP))
          tf->text.traversed = False;
    }
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextEnter( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TextEnter(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmAnyCallbackStruct cb;
    XPoint xmim_point;

    /* Use != NotifyInferior along with event->xcrossing.focus to avoid
     * sending input method info if reason for the event is pointer moving
     * from TextF widget to over-the-spot window (case when over-the-spot
     * is child of TextF widget). */
    if (_XmGetFocusPolicy(w) != XmEXPLICIT && !(tf->text.has_focus) &&
	event->xcrossing.focus &&
        (event->xcrossing.detail != NotifyInferior)) {
       /*
        * Make sure the cached GC has the clipping rectangle
        * set to the current widget.
        */
       CheckHasRect(tf);

       if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
       _XmTextFieldDrawInsertionPoint(tf, False);
       tf->text.blink_on = False;
       tf->text.has_focus = True;
       _XmTextFToggleCursorGC(w);
       if (XtIsSensitive(w)) ChangeBlinkBehavior(tf, True);
       _XmTextFieldDrawInsertionPoint(tf, True);
       GetXYFromPos(tf, TextF_CursorPosition(tf), &xmim_point.x, 
		    &xmim_point.y);
       XmImVaSetFocusValues(w, XmNspotLocation, &xmim_point, NULL);
       cb.reason = XmCR_FOCUS;
       cb.event = event;
       XtCallCallbackList (w, tf->text.focus_callback, (XtPointer) &cb);
    }

    _XmPrimitiveEnter(w, event, params, num_params);
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
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   /* use detail!= NotifyInferior to handle focus change due to pointer
    * wandering into over-the-spot input window - we don't want to change
    * IM's focus state in this case. */
   if (_XmGetFocusPolicy(w) != XmEXPLICIT && tf->text.has_focus &&
       event->xcrossing.focus &&
       (event->xcrossing.detail != NotifyInferior)) {
      if (tf->core.sensitive) ChangeBlinkBehavior(tf, False);
      _XmTextFieldDrawInsertionPoint(tf, False);
      tf->text.has_focus = False;
      _XmTextFToggleCursorGC(w);
      tf->text.blink_on = True;
      _XmTextFieldDrawInsertionPoint(tf, True);
      (void) VerifyLeave(tf, event);
      XmImUnsetFocus(w);
   }

   _XmPrimitiveLeave(w, event, params, num_params);
}

/****************************************************************
 *
 * Private definitions.
 *
 ****************************************************************/

/*
 * ClassPartInitialize sets up the fast subclassing for the widget.i
 * It also merges translation tables.
 */
static void 
#ifdef _NO_PROTO
ClassPartInitialize( w_class )
        WidgetClass w_class ;
#else
ClassPartInitialize(
        WidgetClass w_class )
#endif /* _NO_PROTO */
{
    char *event_bindings;

    _XmFastSubclassInit (w_class, XmTEXT_FIELD_BIT);
    event_bindings = (char *)XtMalloc((unsigned) (strlen(EventBindings1) +
                                      strlen(EventBindings2) +
                                      strlen(EventBindings3) + 1));

    strcpy(event_bindings, EventBindings1);
    strcat(event_bindings, EventBindings2);
    strcat(event_bindings, EventBindings3);
    w_class->core_class.tm_table = 
			      (String) XtParseTranslationTable(event_bindings);
    XtFree(event_bindings);
}

/****************************************************************
 *
 * Private functions used in Initialize.
 *
 ****************************************************************/

/*
 * Verify that the resource settings are valid.  Print a warning
 * message and reset the s if the are invalid.
 */
static void 
#ifdef _NO_PROTO
Validates( tf )
        XmTextFieldWidget tf ;
#else
Validates(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
    XtPointer temp_ptr;

    if (TextF_CursorPosition(tf) < 0) {
	  _XmWarning ((Widget)tf, MSG1);
          TextF_CursorPosition(tf) = 0;
    }

    if (TextF_Columns(tf) <= 0) {
	  _XmWarning ((Widget)tf, MSG2);
	  TextF_Columns(tf) = 20;
    }

    if (TextF_SelectionArray(tf) == NULL) 
       TextF_SelectionArray(tf) = (XmTextScanType *) sarray;

    if (TextF_SelectionArrayCount(tf) <= 0) 
       TextF_SelectionArrayCount(tf) = XtNumber(sarray);

/*
 * Fix for HaL DTS 9841 - copy the selectionArray into dedicated memory.
 */
    temp_ptr = (XtPointer)TextF_SelectionArray(tf);
    TextF_SelectionArray(tf) = (XmTextScanType *)XtMalloc (
		 TextF_SelectionArrayCount(tf) * sizeof(XmTextScanType));
    memcpy((void *)TextF_SelectionArray(tf), (void *)temp_ptr,
	   (TextF_SelectionArrayCount(tf) * sizeof(XmTextScanType)));
/*
 * End fix for HaL DTS 9841
 */
}

static Boolean 
#ifdef _NO_PROTO
LoadFontMetrics( tf )
        XmTextFieldWidget tf ;
#else
LoadFontMetrics(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
    XmFontContext context;
    XmFontListEntry next_entry;
    XmFontType type_return = XmFONT_IS_FONT;
    XtPointer tmp_font;
    Boolean have_font_struct = False;
    Boolean have_font_set = False;
    XFontSetExtents *fs_extents;
    XFontStruct *font;
    unsigned long charwidth = 0;
    char* font_tag = NULL;
    Boolean return_val = 1; /* non-zero == success */

    if (!XmFontListInitFontContext(&context, TextF_FontList(tf)))
       _XmWarning ((Widget)tf, MSG3);

    do {
       next_entry = XmFontListNextEntry(context);
       if (next_entry) {
          tmp_font = XmFontListEntryGetFont(next_entry, &type_return);
          if (type_return == XmFONT_IS_FONTSET) { 
	     font_tag = XmFontListEntryGetTag(next_entry);
	     if (!have_font_set){ /* this saves the first fontset found, just in
                                   * case we don't find a default tag set.
                                   */
	        TextF_UseFontSet(tf) = True;
	        TextF_Font(tf) = (XFontStruct *)tmp_font;
	        have_font_struct = True; /* we have a font set, so no need to 
                                          * consider future font structs */
	        have_font_set = True;    /* we have a font set. */

	        if (!strcmp(XmFONTLIST_DEFAULT_TAG, font_tag))
	           break; /* Break out!  We've found the one we want. */

	     } else if (!strcmp(XmFONTLIST_DEFAULT_TAG, font_tag)){
                TextF_Font(tf) = (XFontStruct *)tmp_font;
	        have_font_set = True;    /* we have a font set. */
	        break; /* Break out!  We've found the one we want. */
	     }
          } else if (!have_font_struct){/* return_type must be XmFONT_IS_FONT */
	     TextF_UseFontSet(tf) = False;
	     TextF_Font(tf)=(XFontStruct*)tmp_font; /* save the first font
                                                     * struct in case no font 
                                                     * set is found */
	     have_font_struct = True;                     
          }
       }
    } while(next_entry != NULL);

    if (!have_font_struct && !have_font_set)
          _XmWarning ((Widget)tf, MSG4);

    if (tf->text.max_char_size > 1 && !have_font_set){
     /*_XmWarning((Widget)tf, MSGnnn); */
     /* printf ("You've got the wrong font baby, Uh-Huh!\n"); */
     /* Must have a font set, as text will be rendered only with new R5 calls 
      * If LoadFontMetrics called from InitializeTextStruct, then max_char_size
      * will be reset to "1"; otherwise, it is called from SetValues and set
      * values will retain use of old fontlist (which is presumed correct
      * for the current locale). */

       return_val = 0; /* tell caller that this font won't work for MB_CUR_MAX*/
    }
    XmFontListFreeFontContext(context);

    if(TextF_UseFontSet(tf)){
       fs_extents = XExtentsOfFontSet((XFontSet)TextF_Font(tf));
#ifdef NON_OSF_FIX
	charwidth = (unsigned long)fs_extents->max_logical_extent.width;
#else /* NON_OSF_FIX */
	charwidth = (unsigned long)fs_extents->max_ink_extent.width;
#endif /* NON_OSF_FIX */
       /* max_ink_extent.y is number of pixels from origin to top of
        * rectangle (i.e. y is negative) */
#ifdef NON_OSF_FIX
       TextF_FontAscent(tf) = -fs_extents->max_logical_extent.y;
       TextF_FontDescent(tf) = fs_extents->max_logical_extent.height +
                               fs_extents->max_logical_extent.y;
#else /* NON_OSF_FIX */
       TextF_FontAscent(tf) = -fs_extents->max_ink_extent.y;
       TextF_FontDescent(tf) = fs_extents->max_ink_extent.height +
                               fs_extents->max_ink_extent.y;
#endif /* NON_OSF_FIX */
    } else {
       font = TextF_Font(tf);
       if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &charwidth)) ||
            charwidth == 0) {
          if (font->per_char && font->min_char_or_byte2 <= '0' &&
                                font->max_char_or_byte2 >= '0')
              charwidth = font->per_char['0' - font->min_char_or_byte2].width;
          else
              charwidth = font->max_bounds.width;
       }
       TextF_FontAscent(tf) = font->max_bounds.ascent;
       TextF_FontDescent(tf) = font->max_bounds.descent;
    }
    tf->text.average_char_width = (Dimension) charwidth;
    return (return_val);
}


/* ValidateString makes the following assumption:  if MB_CUR_MAX == 1, value
 * is a char*, otherwise value is a wchar_t*.  The Boolean "is_wchar" indicates
 * if value points to char* or wchar_t* data.
 *
 * It is ValidateString's task to verify that "value" contains only printing
 * characters; all others are discarded.  ValidateString then mallocs data
 * to store the value and assignes it to tf->text.value (if MB_CUR_MAX == 1)
 * or to tf->text.wc_value (if MB_CUR_MAX != 1), setting the opposite
 * pointer to NULL.  It is the callers responsibility to free data before
 * calling ValidateString.
 */
static void 
#ifdef _NO_PROTO
ValidateString( tf, value, is_wchar )
        XmTextFieldWidget tf ;
        char *value ;
	Boolean is_wchar;
#else
ValidateString(
        XmTextFieldWidget tf,
        char *value,
#if NeedWidePrototypes
	int is_wchar)
#else
	Boolean is_wchar)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   /* if value is wchar_t *, must count the characters; else use strlen */

   int str_len = 0;
   int i, j;
   char stack_cache[400];
   int ret_val = 0;

   if (!is_wchar) {
      char *temp_str, *curr_str, *start_temp;

      str_len = strlen(value);
      temp_str = (char*)XmStackAlloc((Cardinal)str_len + 1, stack_cache);
      start_temp = temp_str;
      curr_str = value;

      for (i = 0; i < str_len;) {
         if (tf->text.max_char_size == 1){
            if (PrintableString(tf, curr_str, 1, False)) {
               *temp_str = *curr_str;
               temp_str++;
            } else {
               char warn_str[BUFSIZ];
               sprintf(warn_str, MSG5, *curr_str);
               _XmWarning ((Widget)tf, warn_str);
            }
	    curr_str++;
	    i++;
	 } else {
	    wchar_t tmp;
	    int num_conv;
	    num_conv = mbtowc(&tmp, curr_str, tf->text.max_char_size);
            if (num_conv >= 0 && PrintableString(tf, (char*)&tmp, 1, True)) {
	       for (j = 0; j < num_conv; j++) {
                  *temp_str = *curr_str;
                  temp_str++;
		  curr_str++;
		  i++; 
		}
            } else {
	       char scratch[8];
               char warn_str[BUFSIZ];
	       int k;
	       for (k=0; k<mblen(curr_str, tf->text.max_char_size); k++)
			scratch[k] = curr_str[k];
	       scratch[k] = '\0';
               sprintf(warn_str, WC_MSG1, scratch);
               _XmWarning ((Widget)tf, warn_str);
		if (num_conv > 0) {	
		  curr_str += num_conv;
	          i += num_conv;
		}
		else {
		  curr_str++;
		  i++;
	       }
            }
	 }
      }
      *temp_str = '\0';

      /* value contains validated string; now stuff it into the proper
       * instance pointer. */
      if (tf->text.max_char_size == 1) {
         tf->text.string_length = strlen(start_temp);
        /* malloc the space for the text value */
         TextF_Value(tf) = (char *) memcpy(
			     XtMalloc((unsigned)(tf->text.string_length + 30)),
			       (void *)start_temp, tf->text.string_length + 1);
         tf->text.size_allocd = tf->text.string_length + 30;
         TextF_WcValue(tf) = NULL;
      } else { /* Need wchar_t* data to set as the widget's value */
         /* count number of wchar's */
         str_len = strlen(start_temp);
         tf->text.string_length = str_len;

         tf->text.size_allocd = (tf->text.string_length + 30)*sizeof(wchar_t);
         TextF_WcValue(tf) = (wchar_t*)XtMalloc((unsigned)tf->text.size_allocd);
         tf->text.string_length = mbstowcs(TextF_WcValue(tf), start_temp,
                                           tf->text.string_length + 30);
	 if (tf->text.string_length < 0) tf->text.string_length = 0;
         TextF_Value(tf) = NULL;
      }
      XmStackFree(start_temp, stack_cache);
   } else {  /* pointer passed points to wchar_t* data */
      wchar_t *wc_value, *wcs_temp_str, *wcs_start_temp, *wcs_curr_str;
      char scratch[8];
      int new_len = 0;
      int csize = 1;

      wc_value = (wchar_t *) value;
      for (str_len = 0, i = 0; *wc_value != (wchar_t)0L; str_len++)
          wc_value++; /* count number of wchars */
      wcs_temp_str=(wchar_t *)XmStackAlloc((Cardinal)
					   ((str_len+1) * sizeof(wchar_t)),
					   stack_cache);
      wcs_start_temp = wcs_temp_str;
      wcs_curr_str = (wchar_t *) value;

      for (i = 0; i < str_len; i++, wcs_curr_str++) {
	 if (tf->text.max_char_size == 1){
	    csize = wctomb(scratch, *wcs_curr_str);
            if (csize >= 0 && PrintableString(tf, scratch, csize, False)) {
	       *wcs_temp_str = *wcs_curr_str;
	       wcs_temp_str++;
	       new_len++;
	    } else {
	       char warn_str[BUFSIZ];
	       if (csize >= 0)
                  scratch[csize]= '\0';
	       else
		  scratch[0] = '\0';
	       sprintf(warn_str, WC_MSG1, scratch);
	       _XmWarning ((Widget)tf, warn_str);
	    }
	 } else {
            if (PrintableString(tf, (char*)wcs_curr_str, 1, True)) {
	       *wcs_temp_str = *wcs_curr_str;
	       wcs_temp_str++;
	       new_len++;
	    } else {
               char warn_str[BUFSIZ];
               csize = wctomb(scratch, *wcs_curr_str);
	       if (csize >= 0)
                  scratch[csize]= '\0';
	       else
		  scratch[0] = '\0';
               sprintf(warn_str, WC_MSG1, scratch);
               _XmWarning ((Widget)tf, warn_str);
	    }
	 }
      } 
      str_len = new_len;

      *wcs_temp_str = (wchar_t)0L; /* terminate with a wchar_t NULL */

      tf->text.string_length = str_len; /* This is *wrong* if MB_CUR_MAX > 2
					 * with no font set... but what can
					 * ya do? Spec says let it dump core. */

      tf->text.size_allocd = (str_len + 30) * sizeof(wchar_t);
      if (tf->text.max_char_size == 1) { /* Need to store data as char* */
         TextF_Value(tf) = XtMalloc((unsigned)tf->text.size_allocd);
         ret_val = wcstombs(TextF_Value(tf), wcs_start_temp,
			    tf->text.size_allocd);
	 if (ret_val < 0) tf->text.value[0] = NULL;
         TextF_WcValue(tf) = NULL;
      } else { /* Need to store data as wchar_t* */
         TextF_WcValue(tf) = (wchar_t*)memcpy(XtMalloc((unsigned)
						       tf->text.size_allocd),
                                              	       (void*)wcs_start_temp,
                                                       (1 + str_len) *
						       sizeof(wchar_t));
         TextF_Value(tf) = NULL;
      }
      XmStackFree((char *)wcs_start_temp, stack_cache);
   }
}

/*
 * Initialize the s in the text fields instance record.
 */
static void 
#ifdef _NO_PROTO
InitializeTextStruct( tf )
        XmTextFieldWidget tf ;
#else
InitializeTextStruct(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
   /* Flag used in losing focus verification to indicate that a traversal
    * key was pressed.  Must be initialized to False.
    */

    Arg args[6];  /* To set initial values to input method */
    Cardinal n = 0;
    XPoint xmim_point;
#ifdef HP_MOTIF
    KEYBOARD_ID kid;
#endif /* HP_MOTIF */

    tf->text.traversed = False;
    
    tf->text.add_mode = False;
    tf->text.has_focus = False;
    tf->text.blink_on = True;
    tf->text.cursor_on = 0;
    tf->text.has_rect = False;
    tf->text.has_primary = False;
    tf->text.has_secondary = False;
    tf->text.has_destination = False;
    tf->text.overstrike = False;
    tf->text.selection_move = False;
    tf->text.sel_start = False;
    tf->text.pending_off = True;
    tf->text.fontlist_created = False;
    tf->text.cancel = False;
    tf->text.extending = False;
    tf->text.prim_time = 0;
    tf->text.dest_time = 0;
    tf->text.select_id = 0;
    tf->text.select_pos_x = 0;
    tf->text.sec_extending = False;
    tf->text.sec_drag = False;
    tf->text.changed_visible = False;
    tf->text.refresh_ibeam_off = True;
    tf->text.in_setvalues = False;
    tf->text.do_resize = True;
    tf->text.have_inverted_image_gc = False;
    tf->text.margin_top = TextF_MarginHeight(tf);
    tf->text.margin_bottom = TextF_MarginHeight(tf);

    /* copy over the font list */
    if (TextF_FontList(tf) == NULL) {
       TextF_FontList(tf) = _XmGetDefaultFontList((Widget)tf,
				              (unsigned char) XmTEXT_FONTLIST);
       tf->text.fontlist_created = True;
    }

    TextF_FontList(tf) = (XmFontList)XmFontListCopy(TextF_FontList(tf));

    switch(MB_CUR_MAX){
       case 1: case 2: case 4: {
          tf->text.max_char_size = MB_CUR_MAX;
          break;
       }
       case 3: {
          tf->text.max_char_size = 4;
          break;
       }
       default:
          tf->text.max_char_size = 1;
    }

   /* LoadFontMetrics fails only if a font set is required, but one is not
    * provided.  The only time a font set is required is MB_CUR_MAX > 1...
    * so we'll pretend MB_CUR_MAX *is* 1... its the only guaranteed way to
    * avoid a core dump.
    */

    (void)LoadFontMetrics(tf);

    tf->text.gc = NULL;
    tf->text.image_gc = NULL;
    tf->text.save_gc = NULL;

    tf->text.new_h_offset = tf->text.h_offset = TextF_MarginWidth(tf) +
			      		      tf->primitive.shadow_thickness +
                              		      tf->primitive.highlight_thickness;

    /* ValidateString will verify value contents, convert to appropriate
     * storage form (i.e. char* or wchar_t*), place in the appropriate
     * location (text.value or text.wc_value), and null out opposite
     * pointer.  */

    if (TextF_WcValue(tf) != NULL) { /* XmNvalueWcs was set - it rules */
       TextF_Value(tf) = NULL;
       ValidateString(tf, (char*)TextF_WcValue(tf), True);
    } else if (TextF_Value(tf) != NULL)
       ValidateString(tf, TextF_Value(tf), False);
    else /* TextF_Value(tf) is null pointer */
       ValidateString(tf, "", False);

    if (TextF_CursorPosition(tf) > tf->text.string_length)
       TextF_CursorPosition(tf) = tf->text.string_length;

    tf->text.orig_left = tf->text.orig_right = tf->text.prim_pos_left =
     tf->text.prim_pos_right = tf->text.prim_anchor = TextF_CursorPosition(tf);

    tf->text.sec_pos_left = tf->text.sec_pos_right =
     	tf->text.sec_anchor = TextF_CursorPosition(tf);

    tf->text.stuff_pos = TextF_CursorPosition(tf);

    tf->text.cursor_height = tf->text.cursor_width = 0;
    tf->text.stipple_tile = (Pixmap) XmGetPixmapByDepth(XtScreen(tf),
						 "50_foreground",
                                                 tf->primitive.foreground,
                                                 tf->core.background_pixel,
						 tf->core.depth);
    tf->text.add_mode_cursor = XmUNSPECIFIED_PIXMAP;
    tf->text.cursor = XmUNSPECIFIED_PIXMAP;
    tf->text.ibeam_off = XmUNSPECIFIED_PIXMAP;
    tf->text.image_clip = XmUNSPECIFIED_PIXMAP;

    tf->text.last_time = 0;

    tf->text.sarray_index = 0;

   /* Initialize highlight elements */
    tf->text.highlight.number = tf->text.highlight.maximum = 1;
    tf->text.highlight.list = (_XmHighlightRec *)XtMalloc((unsigned)
						 sizeof(_XmHighlightRec));
    tf->text.highlight.list[0].position = 0;
    tf->text.highlight.list[0].mode = XmHIGHLIGHT_NORMAL;

    tf->text.timer_id = (XtIntervalId)0;

    XmTextFieldSetEditable((Widget)tf, TextF_Editable(tf));

    if (TextF_Editable(tf)){
       XmImRegister((Widget)tf, (unsigned int) NULL);
       GetXYFromPos(tf, TextF_CursorPosition(tf), &xmim_point.x, &xmim_point.y);
       n = 0;
       XtSetArg(args[n], XmNfontList, TextF_FontList(tf)); n++;
       XtSetArg(args[n], XmNbackground, tf->core.background_pixel); n++;
       XtSetArg(args[n], XmNforeground, tf->primitive.foreground); n++;
       XtSetArg(args[n], XmNbackgroundPixmap,tf->core.background_pixmap);n++;
       XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
       XtSetArg(args[n], XmNlineSpace, 
		TextF_FontAscent(tf)+ TextF_FontDescent(tf)); n++;
       XmImSetValues((Widget)tf, args, n);
    }
#ifdef HP_MOTIF
    kid = _XHPFindKeyboardID(XtDisplay(tf), getenv("KBD_LANG"));
    if (kid >= 0)
       XHPSetKeyboardMapping(XtDisplay(tf), kid, NULL);
#endif /* HP_MOTIF */
}

static Pixmap 
#ifdef _NO_PROTO
GetClipMask( tf, pixmap_name)
        XmTextFieldWidget tf ;
        char *pixmap_name ;
#else
GetClipMask(
        XmTextFieldWidget tf,
        char *pixmap_name)
#endif /* _NO_PROTO */
{
   Display *dpy = XtDisplay(tf);
   Screen *screen = XtScreen(tf);
   XGCValues values;
   GC fillGC;
   Pixmap clip_mask;

   clip_mask = XCreatePixmap(dpy, RootWindowOfScreen(screen), 
			     tf->text.cursor_width, tf->text.cursor_height, 1);

   values.foreground = 1;
   values.background = 0;
   fillGC = XCreateGC(dpy, clip_mask, GCForeground | GCBackground, &values);

   XFillRectangle(dpy, clip_mask, fillGC, 0, 0, tf->text.cursor_width,
		  tf->text.cursor_height);

  /* Install the clipmask for pixmap caching */
   (void) _XmInstallPixmap(clip_mask, screen, pixmap_name, 1, 0);

   XFreeGC(XtDisplay(tf), fillGC);

   return(clip_mask);
}

/*
 * Get the graphics context for filling the background, and for drawing
 * and inverting text.  Used a unique pixmap so all text field widgets
 * share common GCs.
 */
static void 
#ifdef _NO_PROTO
LoadGCs( tf, background, foreground )
        XmTextFieldWidget tf ;
        Pixel background ;
        Pixel foreground ;
#else
LoadGCs(
        XmTextFieldWidget tf,
        Pixel background,
        Pixel foreground )
#endif /* _NO_PROTO */
{
   Display *display = XtDisplay((Widget)tf);
   Screen *screen = XtScreen((Widget)tf);
   XGCValues values;
   static XContext context = 0;
   static Pixmap tf_cache_pixmap;
   unsigned long value_mask = (GCFunction | GCForeground | GCBackground | 
			      GCClipMask | GCArcMode | GCDashOffset);
   unsigned long dynamic_mask;

   if (context == 0)
      context = XUniqueContext();

   if (XFindContext(display, (Window)screen, 
		    context, (char **) &tf_cache_pixmap)){
     XmTextContextData ctx_data;
     Widget xm_display = (Widget) XmGetXmDisplay(display);

     ctx_data = (XmTextContextData) XtMalloc(sizeof(XmTextContextDataRec));

     ctx_data->screen = screen;
     ctx_data->context = context;
     ctx_data->type = _XM_IS_PIXMAP_CTX;

    /* Get the Pixmap identifier that the X Toolkit uses to cache our */
    /* GC's.  We never actually use this Pixmap; just so long as it's */
    /* a unique identifier. */
     tf_cache_pixmap =  XCreatePixmap(display,
				     (Drawable) RootWindowOfScreen(screen),
				     (unsigned int) 1, (unsigned int) 1,
				     (unsigned int) 1);

     XtAddCallback(xm_display, XmNdestroyCallback, 
                   (XtCallbackProc) FreeContextData, (XtPointer) ctx_data);

     XSaveContext(display, (Window)screen, context, (XPointer) tf_cache_pixmap);
   }
   values.clip_mask = tf_cache_pixmap; /* use in caching Text Field gc's */
   values.arc_mode = ArcPieSlice; /* Used in differentiating from Text
				     widget GC caching */

   CheckHasRect(tf);

  /*
   * Get GC for saving area under the cursor.
   */
   values.function = GXcopy;
   values.foreground = tf->primitive.foreground ;
   values.background = tf->core.background_pixel;
   values.dash_offset = MOTIF_PRIVATE_GC;
   if (tf->text.save_gc != NULL)
      XtReleaseGC((Widget)tf, tf->text.save_gc);
   dynamic_mask = (GCClipMask);
   tf->text.save_gc = XtAllocateGC((Widget) tf, tf->core.depth, value_mask,
				   &values, dynamic_mask, 0);

  /*
   * Get GC for drawing text.
   */

   if (!TextF_UseFontSet(tf)){
      value_mask |= GCFont | GCGraphicsExposures;
      values.font = TextF_Font(tf)->fid;
   } else {
      value_mask |= GCGraphicsExposures;
   }
   values.graphics_exposures = (Bool) TRUE;
   values.foreground = foreground ^ background;
   values.background = 0;
   if (tf->text.gc != NULL)
       XtReleaseGC((Widget)tf, tf->text.gc);
   dynamic_mask |=  GCForeground | GCBackground | GCFillStyle | GCTile;
   tf->text.gc = XtAllocateGC((Widget) tf, tf->core.depth, value_mask,
                              &values, dynamic_mask, 0);

   /* Create a temporary GC - change it later in make IBEAM */
   value_mask |= GCTile;
   values.tile = tf->text.stipple_tile;
   if (tf->text.image_gc != NULL)
       XtReleaseGC((Widget)tf, tf->text.image_gc);
   dynamic_mask = (GCForeground | GCBackground | GCStipple | GCFillStyle |
                   GCTileStipXOrigin | GCTileStipYOrigin | GCFunction |
                   GCClipMask | GCClipXOrigin | GCClipYOrigin);
   tf->text.image_gc = XtAllocateGC((Widget) tf, tf->core.depth, value_mask,
                           &values, dynamic_mask, 0);
}

static void 
#ifdef _NO_PROTO
MakeIBeamOffArea( tf, width, height )
        XmTextFieldWidget tf ;
        Dimension width ;
        Dimension height ;
#else
MakeIBeamOffArea(
        XmTextFieldWidget tf,
#if NeedWidePrototypes
        int width,
        int height)
#else
        Dimension width,
        Dimension height)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   Display *dpy = XtDisplay(tf);
   Screen  *screen = XtScreen(tf);
   GC fillGC;

  /* Create a pixmap for storing the screen data where the I-Beam will 
   * be painted */

   tf->text.ibeam_off = XCreatePixmap(dpy, RootWindowOfScreen(screen), width,
				      height, tf->core.depth);

  /* Create a GC for drawing 0's into the pixmap */
   fillGC = XCreateGC(dpy, tf->text.ibeam_off, 0, (XGCValues *) NULL);

  /* Initialize the pixmap to 0's */
   XFillRectangle(dpy, tf->text.ibeam_off, fillGC, 0, 0, width, height);

  /* Free the GC */
   XFreeGC(XtDisplay(tf), fillGC);
}

static void 
#ifdef _NO_PROTO
MakeIBeamStencil( tf, line_width )
        XmTextFieldWidget tf ;
        int line_width ;
#else
MakeIBeamStencil(
        XmTextFieldWidget tf,
        int line_width )
#endif /* _NO_PROTO */
{
   Screen *screen = XtScreen(tf);
   char pixmap_name[17];
   XGCValues values;
   unsigned long valuemask;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

   if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
   sprintf(pixmap_name, "_XmTextF_%d_%d", tf->text.cursor_height, line_width);
   tf->text.cursor = (Pixmap) XmGetPixmapByDepth(screen, pixmap_name, 1, 0, 1);

   if (tf->text.cursor == XmUNSPECIFIED_PIXMAP) {
      Display *dpy = XtDisplay(tf);
      GC fillGC;
      XSegment segments[3];
      XRectangle ClipRect;

     /* Create a pixmap for the I-Beam stencil */
      tf->text.cursor = XCreatePixmap(dpy, XtWindow(tf), tf->text.cursor_width,
				      tf->text.cursor_height, 1);

     /* Create a GC for "cutting out" the I-Beam shape from the pixmap in
      * order to create the stencil.
      */
      fillGC = XCreateGC(dpy, tf->text.cursor, 0, (XGCValues *)NULL);

     /* Fill in the stencil with a solid in preparation
      * to "cut out" the I-Beam
      */
      XFillRectangle(dpy, tf->text.cursor, fillGC, 0, 0, tf->text.cursor_width,
		     tf->text.cursor_height);

     /* Change the GC for use in "cutting out" the I-Beam shape */
      values.foreground = 1;
      values.line_width = line_width;
      XChangeGC(dpy, fillGC, GCForeground | GCLineWidth, &values);

     /* Draw the segments of the I-Beam */
     /* 1st segment is the top horizontal line of the 'I' */
      segments[0].x1 = 0;
      segments[0].y1 = line_width - 1;
      segments[0].x2 = tf->text.cursor_width;
      segments[0].y2 = line_width - 1;

     /* 2nd segment is the bottom horizontal line of the 'I' */
      segments[1].x1 = 0;
      segments[1].y1 = tf->text.cursor_height - 1;
      segments[1].x2 = tf->text.cursor_width;
      segments[1].y2 = tf->text.cursor_height - 1;

     /* 3rd segment is the vertical line of the 'I' */
      segments[2].x1 = tf->text.cursor_width >> 1;
      segments[2].y1 = line_width;
      segments[2].x2 = tf->text.cursor_width >> 1;
      segments[2].y2 = tf->text.cursor_height - 1;

     /* Set the clipping rectangle of the image GC from drawing */
      ClipRect.width = tf->text.cursor_width;
      ClipRect.height = tf->text.cursor_height;
      ClipRect.x = 0;
      ClipRect.y = 0;

      XSetClipRectangles(XtDisplay(tf), fillGC, 0, 0, &ClipRect, 1, Unsorted);

     /* Draw the segments onto the cursor */
      XDrawSegments(dpy, tf->text.cursor, fillGC, segments, 3);

    /* Install the cursor for pixmap caching */
      (void) _XmInstallPixmap(tf->text.cursor, XtScreen(tf), pixmap_name, 1, 0);

     /* Free the fill GC */
      XFreeGC(XtDisplay(tf), fillGC);
   }

  /* Get/create the image_gc used to paint the I-Beam */

    sprintf(pixmap_name, "_XmText_CM_%d", tf->text.cursor_height);
    tf->text.image_clip = XmGetPixmapByDepth(XtScreen(tf), pixmap_name,
					     1, 0, 1);
    if (tf->text.image_clip == XmUNSPECIFIED_PIXMAP)
       tf->text.image_clip = GetClipMask(tf, pixmap_name);

    valuemask = (GCClipMask | GCStipple | GCForeground | GCBackground | 
		 GCFillStyle);
    if (!tf->text.overstrike) {
      values.foreground = tf->primitive.foreground;
      values.background = tf->core.background_pixel;
    } else 
      values.background = values.foreground = 
	tf->core.background_pixel ^ tf->primitive.foreground;
    values.clip_mask = tf->text.image_clip;
    values.stipple = tf->text.cursor;
    values.fill_style = FillStippled;
    XChangeGC(XtDisplay(tf), tf->text.image_gc, valuemask, &values);

}


/* The IBeam Stencil must have already been created before this routine
 * is called.
 */

static void 
#ifdef _NO_PROTO
MakeAddModeCursor( tf, line_width )
        XmTextFieldWidget tf ;
        int line_width ;
#else
MakeAddModeCursor(
        XmTextFieldWidget tf,
        int line_width )
#endif /* _NO_PROTO */
{
   Screen *screen = XtScreen(tf);
   char pixmap_name[25];

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

   if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);
   sprintf(pixmap_name, "_XmTextF_AddMode_%d_%d",
	   tf->text.cursor_height, line_width);

   tf->text.add_mode_cursor = (Pixmap) XmGetPixmapByDepth(screen, pixmap_name,
							  1, 0, 1);

   if (tf->text.add_mode_cursor == XmUNSPECIFIED_PIXMAP) {
      GC fillGC;
      XtGCMask  valueMask;
      XGCValues values;
      Display *dpy = XtDisplay(tf);
      Pixmap stipple;
      XImage *image;

      _XmGetImage(screen, "50_foreground", &image);

      stipple = XCreatePixmap(dpy, XtWindow(tf), image->width, image->height,1);

      tf->text.add_mode_cursor =  XCreatePixmap(dpy, XtWindow(tf),
					        tf->text.cursor_width,
			                        tf->text.cursor_height, 1);

      fillGC = XCreateGC(dpy, tf->text.add_mode_cursor, 0, (XGCValues *)NULL);

      XPutImage(dpy, stipple, fillGC, image, 0, 0, 0, 0, image->width, 
		image->height);

      XCopyArea(dpy, tf->text.cursor, tf->text.add_mode_cursor, fillGC, 0, 0, 
		tf->text.cursor_width, tf->text.cursor_height, 0, 0);

      valueMask = (GCTile | GCFillStyle | GCForeground |
		   GCBackground | GCFunction);
      values.function = GXand;
      values.tile = stipple;
      values.fill_style = FillTiled;
      values.foreground = tf->primitive.foreground; 
      values.background = tf->core.background_pixel;

      XChangeGC(XtDisplay(tf), fillGC, valueMask, &values);

      XFillRectangle(dpy, tf->text.add_mode_cursor, fillGC,
		     0, 0, tf->text.cursor_width, tf->text.cursor_height);

      /* Install the pixmap for pixmap caching */
      _XmInstallPixmap(tf->text.add_mode_cursor,
		       XtScreen(tf), pixmap_name, 1, 0);

      XFreePixmap(dpy, stipple);
      XFreeGC(dpy, fillGC);
  }
}


static void 
#ifdef _NO_PROTO
MakeCursors( tf )
        XmTextFieldWidget tf ;
#else
MakeCursors(
        XmTextFieldWidget tf )
#endif /* _NO_PROTO */
{
   Screen *screen = XtScreen(tf);
   int line_width = 1;

   if (!XtIsRealized((Widget) tf)) return;

   tf->text.cursor_width = 5;
   tf->text.cursor_height = TextF_FontAscent(tf) + TextF_FontDescent(tf);

  /* setup parameters to make a thicker I-Beam */
   if (tf->text.cursor_height > 19) {
      tf->text.cursor_width++;
      line_width = 2;
   }

  /* Remove old ibeam off area */
   if (tf->text.ibeam_off != XmUNSPECIFIED_PIXMAP)
      XFreePixmap(XtDisplay((Widget)tf), tf->text.ibeam_off);

  /* Remove old insert cursor */
   if (tf->text.cursor != XmUNSPECIFIED_PIXMAP) {
       (void) XmDestroyPixmap(screen, tf->text.cursor);
       tf->text.cursor = XmUNSPECIFIED_PIXMAP;
   }

  /* Remove old add mode cursor */
   if (tf->text.add_mode_cursor != XmUNSPECIFIED_PIXMAP) {
       (void) XmDestroyPixmap(screen, tf->text.add_mode_cursor);
       tf->text.add_mode_cursor = XmUNSPECIFIED_PIXMAP;
   }

  /* Remove old image_clip pixmap */
   if (tf->text.image_clip != XmUNSPECIFIED_PIXMAP) {
       (void) XmDestroyPixmap(screen, tf->text.image_clip);
       tf->text.image_clip = XmUNSPECIFIED_PIXMAP;
   }

  /* Create area in which to save text located underneath I beam */
   MakeIBeamOffArea(tf, MAX(tf->text.cursor_height>>1, tf->text.cursor_height),
		    tf->text.cursor_height);

  /* Create a new i-beam cursor */
   MakeIBeamStencil(tf, line_width);

  /* Create a new add_mode cursor */
   MakeAddModeCursor(tf, line_width);

   ResetClipOrigin(tf, False);

   if (tf->text.overstrike)
     tf->text.cursor_width = tf->text.cursor_height >> 1;
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
    XmTextFieldWidget tf = (XmTextFieldWidget) transfer_rec->widget;
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    Atom CS_OF_LOCALE;
    XmTextPosition insertPosLeft, insertPosRight, left, right, cursorPos;
    int max_length = 0;
    Boolean local = tf->text.has_primary;
    char * total_tmp_value;
    wchar_t * wc_total_tmp_value;
    char ** tmp_value;
    int malloc_size = 0;
    int num_vals, status;
    Arg args[8];
    Cardinal n, i;
    unsigned long total_length = 0;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XTextProperty tmp_prop;
    Boolean replace = False;
    XmAnyCallbackStruct cb;

   /* When type = NULL, we are assuming a DELETE request has been requested */
    if (*type == XmInternAtom(XtDisplay(transfer_rec->widget), "NULL", False)) {
       if (transfer_rec->num_chars > 0 && transfer_rec->move) {
          tf->text.prim_anchor = transfer_rec->insert_pos;
          cursorPos = transfer_rec->insert_pos + transfer_rec->num_chars;
          _XmTextFieldSetCursorPosition(tf, NULL, cursorPos,
					False, True);
          (void) SetDestination((Widget)tf, TextF_CursorPosition(tf),
                                False, transfer_rec->timestamp);
          XmTextFieldSetSelection((Widget)tf, tf->text.prim_anchor,
				  TextF_CursorPosition(tf),
                                  transfer_rec->timestamp);
       }
       if (value) {
	  XtFree((char *)value);
          value = NULL;
       }
       return;
    }
#ifdef NON_OSF_FIX
	tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(transfer_rec->widget),
				      &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       CS_OF_LOCALE = tmp_prop.encoding;
    else
       CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
                              * characters.  But just in case someones
                              * Xlib is broken, this prevents a core dump.
                              */

#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif

    if (!value || (*type != CS_OF_LOCALE && *type != COMPOUND_TEXT &&
                   *type != XA_STRING )) {
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
       /* value NEEDS TO BE FREED */
       tmp_prop.value = (unsigned char *) value; 
       tmp_prop.encoding = *type;
       tmp_prop.format = 8;
       tmp_prop.nitems = *length;
       status = 0;

       status = XmbTextPropertyToTextList(XtDisplay(transfer_rec->widget),
					  &tmp_prop, &tmp_value, &num_vals);

      /* if no conversion, num_vals is not changed */
       if (num_vals && (status == Success || status > 0)) {
          for (i = 0; i < num_vals ; i++)
              malloc_size += strlen(tmp_value[i]);

          total_tmp_value = XtMalloc ((unsigned) malloc_size + 1);
          total_tmp_value[0] = '\0';
          for (i = 0; i < num_vals ; i++)
             strcat(total_tmp_value, tmp_value[i]);
          total_length = strlen(total_tmp_value);
	  XFreeStringList(tmp_value);
       } else  {
          if (value) {
	     XtFree((char *)value);
	     value = NULL;
          }
          return;
       }
    } else {
       total_tmp_value = (char *)value;
       total_length = *length;
    }

    if (TextF_PendingDelete(tf) && tf->text.has_primary &&
       tf->text.prim_pos_left != tf->text.prim_pos_right &&
       insertPosLeft > tf->text.prim_pos_left &&
       insertPosRight < tf->text.prim_pos_right) {
      insertPosLeft = tf->text.prim_pos_left;
      insertPosRight = tf->text.prim_pos_right;
    }

    transfer_rec->num_chars = _XmTextFieldCountCharacters(tf, total_tmp_value, 
							  total_length);

    _XmTextFieldDrawInsertionPoint(tf, False);

    if (transfer_rec->move && local) {
       max_length = TextF_MaxLength(tf);
       TextF_MaxLength(tf) = MAXINT;
    }

    if (tf->text.max_char_size == 1) {
       if (_XmTextFieldReplaceText(tf, NULL, insertPosLeft, insertPosRight,
				   (char *) total_tmp_value,
				   (int)total_length, False))
	  replace = True;
    } else {
       wc_total_tmp_value = (wchar_t*)XtMalloc((unsigned)
					       total_length * sizeof(wchar_t));
      /* Note: casting total_length to an int may result in a truncation. */
       total_length = mbstowcs(wc_total_tmp_value, total_tmp_value,
			       (int)total_length);
       if (total_length > 0) {
          if (_XmTextFieldReplaceText(tf, NULL, insertPosLeft, insertPosRight,
				      (char *) wc_total_tmp_value,
				      (int)total_length, False))
	     replace = True;
       }
       XtFree((char*)wc_total_tmp_value);
    }

    if (replace) {
       tf->text.pending_off = FALSE;
       if (transfer_rec->num_chars > 0 && !transfer_rec->move) {
          cursorPos = transfer_rec->insert_pos + transfer_rec->num_chars;
          _XmTextFieldSetCursorPosition(tf, NULL, cursorPos, 
					False, True);
          SetDestination((Widget)tf, TextF_CursorPosition(tf), False,
			 transfer_rec->timestamp);
       }
       if (XmTextFieldGetSelectionPosition((Widget)tf, &left, &right)) {
          if (transfer_rec->move && left < transfer_rec->insert_pos)
	     transfer_rec->insert_pos -= transfer_rec->num_chars;
          if (TextF_CursorPosition(tf) < left ||
	      TextF_CursorPosition(tf) > right)
	     tf->text.pending_off = TRUE;
       } else {
          if (!transfer_rec->move && !tf->text.add_mode &&
              transfer_rec->num_chars != 0)
	     tf->text.prim_anchor = insertPosLeft;
       }
       if (transfer_rec->move) {
          XmDropTransferEntryRec transferEntries[1];
          XmDropTransferEntryRec *transferList = NULL;

          transferEntries[0].client_data = (XtPointer) transfer_rec;
          transferEntries[0].target = XmInternAtom(XtDisplay(w),"DELETE",False);
          transferList = transferEntries;
          XmDropTransferAdd(w, transferEntries, 1);
       }
       cb.reason = XmCR_VALUE_CHANGED;
       cb.event = (XEvent *)NULL;
       XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		          (XtPointer) &cb);
    }

    if (transfer_rec->move && local) {
       TextF_MaxLength(tf) = max_length;
    }

    XtFree(total_tmp_value);
    _XmTextFieldDrawInsertionPoint(tf, True);
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
    Cardinal numExportTargets, n;
    Atom *exportTargets;
    Arg args[10];
    XmTextPosition insert_pos, left, right;
    Display *display = XtDisplay(w);

    drag_cont = cb->dragContext;

    n = 0;
    XtSetArg(args[n], XmNsourceWidget, &initiator); n++;
    XtSetArg(args[n], XmNexportTargets, &exportTargets); n++;
    XtSetArg(args[n], XmNnumExportTargets, &numExportTargets); n++;
    XtGetValues((Widget) drag_cont, args, n);
    
    insert_pos = GetPosFromX((XmTextFieldWidget) w, cb->x);

    if (cb->operation & XmDROP_MOVE && w == initiator &&
        XmTextFieldGetSelectionPosition(w, &left, &right) &&
        left != right && insert_pos >= left && insert_pos <= right) {
       n = 0;
       XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
       XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
    } else {
       XmDropTransferEntryRec transferEntries[2];
       XmDropTransferEntryRec *transferList = NULL;
       Atom TEXT = XmInternAtom(display, "TEXT", False);
       Atom COMPOUND_TEXT = XmInternAtom(display, "COMPOUND_TEXT", False);
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

       status = XmbTextListToTextProperty(display, &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
       if (status == Success)
	  CS_OF_LOCALE = tmp_prop.encoding;
       else
	  CS_OF_LOCALE = 99999; /* XmbTextList... should never fail for XPCS
				 * characters.  But just in case someones
				 * Xlib is broken, this prevents a core dump.
				 */
#ifdef NON_OSF_FIX
       if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif


      /* intialize data to send to drop transfer callback */
       transfer_rec = (_XmTextDropTransferRec *)
		       XtMalloc(sizeof(_XmTextDropTransferRec));
       transfer_rec->widget = w;
       transfer_rec->insert_pos = insert_pos;
       transfer_rec->num_chars = 0;
       transfer_rec->timestamp = cb->timeStamp;
       transfer_rec->move = False;

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
#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif

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
         /* we currently don't care about these messages */
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
#ifdef NON_OSF_FIX
    if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif

    targets[1] = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    targets[2] = XA_STRING;
    targets[3] = XmInternAtom(XtDisplay(w), "TEXT", False);

    n = 0;
    XtSetArg(args[n], XmNimportTargets, targets); n++;
    XtSetArg(args[n], XmNnumImportTargets, 4); n++;
    XtSetArg(args[n], XmNdropProc, DragProcCallback); n++;
    XtSetArg(args[n], XmNdropProc, DropProcCallback); n++;
    XmDropSiteRegister(w, args, n);
}

/*
 * Initialize
 *    Intializes the text data and ensures that the data in new
 * is valid.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Initialize( request, new_w, args, num_args )
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget req_tf = (XmTextFieldWidget) request;
    XmTextFieldWidget new_tf = (XmTextFieldWidget) new_w;
    Dimension width, height;
  
    Validates(new_tf);

    InitializeTextStruct(new_tf);

    LoadGCs(new_tf, new_tf->core.background_pixel,
		    new_tf->primitive.foreground );

    ComputeSize(new_tf, &width, &height);
  
    if (req_tf->core.width == 0)
       new_tf->core.width = width;
    if (req_tf->core.height == 0)
       new_tf->core.height = height;

    RegisterDropSite(new_w);
 
    if (new_tf->text.verify_bell == (Boolean) XmDYNAMIC_BOOL)
    {
      if (_XmGetAudibleWarning(new_w) == XmBELL) 
	new_tf->text.verify_bell = True;
      else
	new_tf->text.verify_bell = False;
    }
#ifdef CDE_INTEGRATE
    {
    Boolean btn1_transfer;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(new_w)), "enableBtn1Transfer", &btn1_transfer, NULL);
    if (btn1_transfer) /* for btn2 extend and transfer cases */
        XtOverrideTranslations(new_w, XtParseTranslationTable(EventBindingsCDE));
    if (btn1_transfer == True) /* for btn2 extend case */
        XtOverrideTranslations(new_w, XtParseTranslationTable(EventBindingsCDEBtn2));
    }
#endif /* CDE_INTEGRATE */
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
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   Arg args[6];  /* To set initial values to input method */
   Cardinal n = 0;
   XPoint xmim_point;

   XtCreateWindow(w, (unsigned int) InputOutput,
                   (Visual *) CopyFromParent, *valueMask, attributes);
   MakeCursors(tf);


   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

   _XmTextFieldSetClipRect(tf);

    if (TextF_Editable(tf)){
      GetXYFromPos(tf, TextF_CursorPosition(tf), &xmim_point.x, &xmim_point.y);
      n = 0;
      XtSetArg(args[n], XmNfontList, TextF_FontList(tf)); n++;
      XtSetArg(args[n], XmNbackground, tf->core.background_pixel); n++;
      XtSetArg(args[n], XmNforeground, tf->primitive.foreground); n++;
      XtSetArg(args[n], XmNbackgroundPixmap,tf->core.background_pixmap);n++;
      XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
      XtSetArg(args[n], XmNlineSpace, 
	       TextF_FontAscent(tf)+ TextF_FontDescent(tf)); n++;
      XmImSetValues((Widget)tf, args, n);
    }
}

static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) wid ;
    Widget dest = XmGetDestination(XtDisplay(wid));

    if (dest == wid)
       _XmSetDestination(XtDisplay(wid), NULL); 

    if (tf->text.timer_id)
	XtRemoveTimeOut(tf->text.timer_id);

    if (tf->text.has_rect) {
       TextFGCData gc_data = GetTextFGCData(wid);
       gc_data->tf->text.has_rect = False;
       gc_data->tf = NULL;
    }

    if (tf->text.max_char_size == 1)
       XtFree(TextF_Value(tf));
    else
       XtFree((char *)TextF_WcValue(tf));

    XmDestroyPixmap(XtScreen(tf), tf->text.stipple_tile);

    XtReleaseGC(wid, tf->text.gc);
    XtReleaseGC(wid, tf->text.image_gc);
    XtReleaseGC(wid, tf->text.save_gc);

    XtFree((char *)tf->text.highlight.list);

    if (tf->text.fontlist_created)
       XmFontListFree((XmFontList)TextF_FontList(tf));

    if (tf->text.add_mode_cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(XtScreen(tf), tf->text.add_mode_cursor);

    if (tf->text.cursor != XmUNSPECIFIED_PIXMAP)
       (void) XmDestroyPixmap(XtScreen(tf), tf->text.cursor);

    if (tf->text.ibeam_off != XmUNSPECIFIED_PIXMAP)
       XFreePixmap(XtDisplay((Widget)tf), tf->text.ibeam_off);

    if (tf->text.image_clip != XmUNSPECIFIED_PIXMAP)
       XmDestroyPixmap(XtScreen(tf), tf->text.image_clip);

/*
 * Fix for HaL DTS 9841 - release the data for the selectionArray.
 */
    XtFree((char *)TextF_SelectionArray(tf));

    XtRemoveAllCallbacks(wid, XmNactivateCallback);
    XtRemoveAllCallbacks(wid, XmNlosingFocusCallback);
    XtRemoveAllCallbacks(wid, XmNfocusCallback);
    XtRemoveAllCallbacks(wid, XmNmodifyVerifyCallback);
    XtRemoveAllCallbacks(wid, XmNmotionVerifyCallback);
    XtRemoveAllCallbacks(wid, XmNvalueChangedCallback);
    XtRemoveAllCallbacks(wid, XmNgainPrimaryCallback);
    XtRemoveAllCallbacks(wid, XmNlosePrimaryCallback);

    XmImUnregister(wid);
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
  XmTextFieldWidget tf = (XmTextFieldWidget) w;

  tf->text.do_resize = False;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

  _XmTextFieldSetClipRect(tf);

  tf->text.h_offset = TextF_MarginWidth(tf) + tf->primitive.shadow_thickness +
                      			      tf->primitive.highlight_thickness;

  tf->text.refresh_ibeam_off = True;

  (void) AdjustText(tf, TextF_CursorPosition(tf), True);

  tf->text.do_resize = True;
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
    /* this function deals with resizeWidth False */
    ComputeSize((XmTextFieldWidget) widget, 
		&desired->width, &desired->height);

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}


/*
 * Redisplay will redraw shadows, borders, and text.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TextFieldExpose( w, event, region )
        Widget w ;
        XEvent *event ;
        Region region ;
#else
TextFieldExpose(
        Widget w,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
  XmTextFieldWidget tf = (XmTextFieldWidget) w;
  XGCValues values;
  

  if (event->xany.type != Expose) return;

  tf->text.do_resize = False;

   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(tf);

  if (!tf->text.has_rect) _XmTextFieldSetClipRect(tf);

  /* I can get here even though the widget isn't visible (i.e. my parent is
   * sized so that I have nothing visible.  In this case, capturing the putback
   * area yields garbage...  And if this area is not in an area where text
   * will be drawn (i.e. forcing something new/valid to be there next time I
   * go to capture it) the garbage persists.  To prevent this, initialize the
   * putback area and then update it to a solid background color.
   */

  tf->text.refresh_ibeam_off = False;
  values.foreground = tf->core.background_pixel;
  XChangeGC(XtDisplay(w), tf->text.save_gc, GCForeground, &values);
  XFillRectangle(XtDisplay(w), tf->text.ibeam_off, tf->text.save_gc, 0, 0,
		    tf->text.cursor_width, tf->text.cursor_height);
  values.foreground = tf->primitive.foreground;
  XChangeGC(XtDisplay(w), tf->text.save_gc, GCForeground, &values);

  _XmTextFieldDrawInsertionPoint(tf, False);

  if (XtIsRealized(tf)) {
     if (tf->primitive.shadow_thickness > 0)
       _XmDrawShadows(XtDisplay(tf), XtWindow(tf),
             tf->primitive.bottom_shadow_GC, 
             tf->primitive.top_shadow_GC,
             (int) tf->primitive.highlight_thickness,
             (int) tf->primitive.highlight_thickness,
             (int) (tf->core.width - (2 * tf->primitive.highlight_thickness)),
             (int) (tf->core.height - (2 * tf->primitive.highlight_thickness)),
             (int) tf->primitive.shadow_thickness,
             XmSHADOW_OUT);


     if (tf->primitive.highlighted)
     {   
         if(    ((XmTextFieldWidgetClass) XtClass(tf))
                                        ->primitive_class.border_highlight    )
         {   
             (*((XmTextFieldWidgetClass) XtClass(tf))
                            ->primitive_class.border_highlight)( (Widget) tf) ;
             } 
         } 
     else
     {   if(    ((XmTextFieldWidgetClass) XtClass(tf))
                                      ->primitive_class.border_unhighlight    )
         {   (*((XmTextFieldWidgetClass) XtClass(tf))
                          ->primitive_class.border_unhighlight)( (Widget) tf) ;
             } 
         } 

     RedisplayText(tf, 0, tf->text.string_length);
  }

  tf->text.refresh_ibeam_off = True;

  _XmTextFieldDrawInsertionPoint(tf, True);

  tf->text.do_resize = True;
}

/*
 *
 * SetValues
 *    Checks the new text data and ensures that the data is valid.
 * Invalid values will be rejected and changed back to the old
 * values.
 *
 */
/* ARGSUSED */

static Boolean 
#ifdef _NO_PROTO
SetValues( old, request, new_w, args, num_args )
        Widget old ;
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget old,
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget new_tf = (XmTextFieldWidget) new_w;
    XmTextFieldWidget old_tf = (XmTextFieldWidget) old;
    Boolean cursor_pos_set = False;
    Boolean new_size = False;
    Boolean redisplay = False;
    Boolean redisplay_text = False;
    Boolean new_font = False;
    Boolean mod_ver_ret = False;
    Boolean diff_values = False;
    Dimension new_width = new_tf->core.width;
    Dimension new_height = new_tf->core.height;
    Arg im_args[6];
    XPoint xmim_point;
    XmTextPosition new_position = 0;
    XmTextPosition newInsert;
    int n = 0;

    if (new_w->core.being_destroyed) return False;

    new_tf->text.in_setvalues = True;
    new_tf->text.redisplay = False;

   /* If new cursor position, copy the old cursor pos to the new widget
    * so that when we turn off the i-beam, the current location (old
    * widget) is used, but the new i-beam parameters (on/off, state, ...)
    * are utilized.  Then move the cursor.  Otherwise, just turn off
    * the i-beam. */

    if (TextF_CursorPosition(new_tf) != TextF_CursorPosition(old_tf)) {
       new_position = TextF_CursorPosition(new_tf) ;
       TextF_CursorPosition(new_tf) = TextF_CursorPosition(old_tf);
       _XmTextFieldDrawInsertionPoint(old_tf, False);
       new_tf->text.blink_on = old_tf->text.blink_on;
       new_tf->text.cursor_on = old_tf->text.cursor_on;
       _XmTextFieldSetCursorPosition(new_tf, NULL, new_position,
				     True, True);
       (void) SetDestination(new_w, TextF_CursorPosition(new_tf), False,
			     XtLastTimestampProcessed(XtDisplay(new_w)));
       cursor_pos_set = True;
    } else {
      _XmTextFieldDrawInsertionPoint(old_tf, False);
      new_tf->text.blink_on = old_tf->text.blink_on;
      new_tf->text.cursor_on = old_tf->text.cursor_on;
    }

    if (new_w->core.sensitive == False &&
        new_tf->text.has_destination) {
       (void) SetDestination(new_w, TextF_CursorPosition(new_tf),
			     True, XtLastTimestampProcessed(XtDisplay(new_w)));
    }

    if (TextF_SelectionArray(new_tf) == NULL) 
       TextF_SelectionArray(new_tf) = TextF_SelectionArray(old_tf);

    if (TextF_SelectionArrayCount(new_tf) <= 0) 
       TextF_SelectionArrayCount(new_tf) = TextF_SelectionArrayCount(old_tf);

/*
 * Fix for HaL DTS 9841 - If the new and old selectionArrays do not match,
 *			  free the old array and then copy the new array.
 */
    if (TextF_SelectionArray(new_tf) != TextF_SelectionArray(old_tf))
    {
      XtPointer temp_ptr;

      XtFree((char *)TextF_SelectionArray(old_tf));
      temp_ptr = (XtPointer)TextF_SelectionArray(new_tf);
      TextF_SelectionArray(new_tf) = (XmTextScanType *)XtMalloc (
                 TextF_SelectionArrayCount(new_tf) * sizeof(XmTextScanType));
      memcpy((void *)TextF_SelectionArray(new_tf), (void *)temp_ptr,
             (TextF_SelectionArrayCount(new_tf) * sizeof(XmTextScanType)));
    }
/*
 * End fix for HaL DTS 9841
 */


   /* Make sure the new_tf cursor position is a valid value.
    */
    if (TextF_CursorPosition(new_tf) < 0) {
       _XmWarning (new_w, MSG1);
       TextF_CursorPosition(new_tf) = TextF_CursorPosition(old_tf);
       cursor_pos_set = False;
    }

    if (TextF_FontList(new_tf)!= TextF_FontList(old_tf)) {
       new_font = True;
       if (TextF_FontList(new_tf) == NULL)
          TextF_FontList(new_tf) = _XmGetDefaultFontList(new_w, XmTEXT_FONTLIST);
       TextF_FontList(new_tf) =
			    (XmFontList)XmFontListCopy(TextF_FontList(new_tf));
       if (!LoadFontMetrics(new_tf)){ /* Fails if font set required but not
                                       * available. */
          XmFontListFree((XmFontList)TextF_FontList(new_tf));
          TextF_FontList(new_tf) = TextF_FontList(old_tf);
          (void)LoadFontMetrics(new_tf); /* it *was* correct, so re-use it */
          new_font = False;
       } else {
          XtSetArg(im_args[n], XmNfontList, TextF_FontList(new_tf)); n++;
          redisplay = True;
       }
    }

    /* Four cases to handle for value:
     *   1. user set both XmNvalue and XmNwcValue.
     *   2. user set the opposite resource (i.e. value is a char*
     *      and user set XmNwcValue, or vice versa).
     *   3. user set the corresponding resource (i.e. value is a char*
     *      and user set XmNValue, or vice versa).
     *   4. user set neither XmNValue nor XmNwcValue
     */

    /* OSF says:  if XmNvalueWcs set, it overrides all else */

     if (new_tf->text.max_char_size == 1) {  
       /* wc_value on new will be NULL unless XmNvalueWcs was set.   */
        if (TextF_WcValue(new_tf) != NULL){ /* must be new if MB_CUR... == 1 */
           ValidateString(new_tf, (char*) TextF_WcValue(new_tf), True);
           diff_values = True;
        } else if (TextF_Value(new_tf) != TextF_Value(old_tf)) {
           diff_values = True;
           if (TextF_Value(new_tf) == NULL) {
              ValidateString(new_tf, "", False);
           } else
              ValidateString(new_tf, TextF_Value(new_tf), False);
        } /* else, no change so don't do anything */
     } else {
        if (TextF_WcValue(new_tf) != TextF_WcValue(old_tf)) {
           diff_values = True;
           if (TextF_WcValue(new_tf) == NULL) {
              TextF_WcValue(new_tf) = (wchar_t*) XtMalloc(sizeof(wchar_t));
              *TextF_WcValue(new_tf) = (wchar_t)NULL;
           }
           ValidateString(new_tf, (char*)TextF_WcValue(new_tf), True);
        } else if (TextF_Value(new_tf) != TextF_Value(old_tf)) {
           /* Someone set XmNvalue */
           diff_values = True;
           if (TextF_Value(new_tf) == NULL)
              ValidateString(new_tf, "", True);
           else
              ValidateString(new_tf, TextF_Value(new_tf), False);

        } /* else, no change so don't do anything */
     }

    if (diff_values) { /* old value != new value */
       Boolean do_it = True;
      /* If there are modify verify callbacks, verify that we want to continue
       * the action.
       */
       if (TextF_ModifyVerifyCallback(new_tf) || 
	   TextF_ModifyVerifyCallbackWcs(new_tf)) {
         /* If the function ModifyVerify() returns false then don't
          * continue with the action.
          */
	  char *temp, *old;
	  int free_insert;
          XmTextPosition fromPos = 0, toPos;
	  int ret_val = 0;
          toPos = old_tf->text.string_length;
	  if (new_tf->text.max_char_size == 1) {
	     temp = TextF_Value(new_tf);
	     mod_ver_ret = ModifyVerify(new_tf, NULL, &fromPos, &toPos,
					&temp, &new_tf->text.string_length,
					&newInsert, &free_insert);
	  } else {
	     old = temp = XtMalloc((unsigned)((new_tf->text.string_length + 1) *
					      new_tf->text.max_char_size));
	     ret_val = wcstombs(temp, TextF_WcValue(new_tf), 
	         (new_tf->text.string_length + 1) * new_tf->text.max_char_size);
	     if (ret_val < 0) temp[0] = NULL;
	     mod_ver_ret = ModifyVerify(new_tf, NULL, &fromPos, &toPos, &temp,
					&new_tf->text.string_length, &newInsert,
					&free_insert);
	     if (old != temp) XtFree (old);
          }
	  if (free_insert) XtFree(temp);
          if (!mod_ver_ret) {
             if (new_tf->text.verify_bell) XBell(XtDisplay(new_w), 0);
	     if (new_tf->text.max_char_size == 1) {
                TextF_Value(new_tf) = (char *) memcpy(
					   XtRealloc(TextF_Value(new_tf),
                                           (unsigned)old_tf->text.size_allocd),
                                           (void*)TextF_Value(old_tf),
					   old_tf->text.string_length + 1);
                new_tf->text.string_length = old_tf->text.string_length;
                new_tf->text.size_allocd = old_tf->text.size_allocd;
                XtFree(TextF_Value(old_tf));
             } else {
		/* realloc to old size, cast to wchar_t*, and copy the data */
		TextF_WcValue(new_tf) = (wchar_t*)memcpy(
		       XtRealloc((char *)TextF_WcValue(new_tf),
                       (unsigned)old_tf->text.size_allocd),
                       (void*)TextF_WcValue(old_tf),
		       (unsigned) old_tf->text.size_allocd);

                new_tf->text.string_length = old_tf->text.string_length;
                new_tf->text.size_allocd = old_tf->text.size_allocd;
                XtFree((char *)TextF_WcValue(old_tf));
	     }
             do_it = False;
          }
       }


       if (do_it) {
          XmAnyCallbackStruct cb;

	  if (new_tf->text.max_char_size == 1)
             XtFree(TextF_Value(old_tf));
	  else
             XtFree((char *)TextF_WcValue(old_tf));

          XmTextFieldSetHighlight(new_w, new_tf->text.prim_pos_left,
			          new_tf->text.prim_pos_right,
				  XmHIGHLIGHT_NORMAL);

          new_tf->text.pending_off = True;    

	  /* if new_position was > old_tf->text.string_length, last time
	   * the SetCursorPosition didn't take.
	   */
          if (!cursor_pos_set || new_position > old_tf->text.string_length){
             _XmTextFieldSetCursorPosition(new_tf, NULL, new_position,
					   True, False);
             if (new_tf->text.has_destination)
                (void) SetDestination(new_w, TextF_CursorPosition(new_tf),
			     False, XtLastTimestampProcessed(XtDisplay(new_w)));
	  }

          if (TextF_ResizeWidth(new_tf) && new_tf->text.do_resize)
             AdjustSize(new_tf);
          else {
             new_tf->text.h_offset = TextF_MarginWidth(new_tf) + 
	      		         new_tf->primitive.shadow_thickness +
                                 new_tf->primitive.highlight_thickness;
             if (!AdjustText(new_tf, TextF_CursorPosition(new_tf), False))
                redisplay_text = True;
          }

          cb.reason = XmCR_VALUE_CHANGED;
          cb.event = NULL;
          XtCallCallbackList(new_w, TextF_ValueChangedCallback(new_tf),
			     (XtPointer) &cb);

       }
    }

    if (new_tf->primitive.foreground != old_tf->primitive.foreground ||
        TextF_FontList(new_tf)!= TextF_FontList(old_tf) ||
        new_tf->core.background_pixel != old_tf->core.background_pixel) {
       LoadGCs(new_tf, new_tf->primitive.foreground,
			 new_tf->core.background_pixel);
       MakeCursors(new_tf);
   /*
    * Make sure the cached GC has the clipping rectangle
    * set to the current widget.
    */
   CheckHasRect(new_tf);

       _XmTextFieldSetClipRect(new_tf);
       if (new_tf->text.have_inverted_image_gc){
	  new_tf->text.have_inverted_image_gc = False;
          InvertImageGC(new_tf);
       }
       redisplay = True;
       XtSetArg(im_args[n], XmNbackground, new_tf->core.background_pixel); n++;
       XtSetArg(im_args[n], XmNforeground, new_tf->primitive.foreground); n++;
    }

    if (new_tf->text.has_focus && XtSensitive(new_tf) &&
        TextF_BlinkRate(new_tf) != TextF_BlinkRate(old_tf)) {

        if (TextF_BlinkRate(new_tf) == 0) {
            new_tf->text.blink_on = True;
            if (new_tf->text.timer_id) {
                XtRemoveTimeOut(new_tf->text.timer_id);
                new_tf->text.timer_id = (XtIntervalId)0;
            }
        } else if (new_tf->text.timer_id == (XtIntervalId)0) {
           new_tf->text.timer_id =
		 XtAppAddTimeOut(XtWidgetToApplicationContext(new_w),
				 (unsigned long)TextF_BlinkRate(new_tf),
                                 HandleTimer,
                                 (XtPointer) new_tf);
        }
        BlinkInsertionPoint(new_tf);
    }

    if (TextF_MarginHeight(new_tf) != TextF_MarginHeight(old_tf)) {
       new_tf->text.margin_top = TextF_MarginHeight(new_tf);
       new_tf->text.margin_bottom = TextF_MarginHeight(new_tf);
    }

    new_size = TextF_MarginWidth(new_tf) != TextF_MarginWidth(old_tf) ||
               TextF_MarginHeight(new_tf) != TextF_MarginHeight(old_tf) ||
               TextF_FontList(new_tf) != TextF_FontList(old_tf);


    if (TextF_Columns(new_tf) < 0) {
       _XmWarning (new_w, MSG7);
       TextF_Columns(new_tf) = TextF_Columns(old_tf);
    }

    if (!(new_width != old_tf->core.width &&
	  new_height != old_tf->core.height)) {
       if (TextF_Columns(new_tf) != TextF_Columns(old_tf) || new_size) {
	  Dimension width, height;

	  ComputeSize(new_tf, &width, &height);
	  AdjustText(new_tf, 0, False);

	  if (new_width == old_tf->core.width)
	     new_w->core.width = width;
	  if (new_height == old_tf->core.height)
	     new_w->core.height = height;
          new_tf->text.h_offset = TextF_MarginWidth(new_tf) +
                           new_tf->primitive.shadow_thickness +
                           new_tf->primitive.highlight_thickness;
	  redisplay = True;
       }
    } else {
       if (new_width != new_tf->core.width)
          new_tf->core.width = new_width;
       if (new_height != new_tf->core.height)
          new_tf->core.height = new_height;
    }

    new_tf->text.refresh_ibeam_off = 1; /* force update of putback area */

    _XmTextFieldDrawInsertionPoint(new_tf, True);

    if (XtIsSensitive(new_tf) != XtIsSensitive(old_tf)) {
       if (XtSensitive(new_w)) {
          _XmTextFieldDrawInsertionPoint(new_tf, False);
          new_tf->text.blink_on = False;
          _XmTextFToggleCursorGC(new_w);
         _XmTextFieldDrawInsertionPoint(new_tf, True);
       } else {
          if (new_tf->text.has_focus) {
             new_tf->text.has_focus = False;
             ChangeBlinkBehavior(new_tf, False);
             _XmTextFieldDrawInsertionPoint(new_tf, False);
             _XmTextFToggleCursorGC(new_w);
             new_tf->text.blink_on = True;
             _XmTextFieldDrawInsertionPoint(new_tf, True);
          }
       }
       if (new_tf->text.string_length > 0) redisplay = True;
    }

    GetXYFromPos(new_tf, TextF_CursorPosition(new_tf), &xmim_point.x, 
		 &xmim_point.y);

    if (TextF_Editable(old_tf) != TextF_Editable(new_tf)) {
       Boolean editable = TextF_Editable(new_tf);
       TextF_Editable(new_tf) = TextF_Editable(old_tf);
       XmTextFieldSetEditable(new_w, editable);
    }

    XtSetArg(im_args[n], XmNbackgroundPixmap,
	     new_tf->core.background_pixmap); n++;
    XtSetArg(im_args[n], XmNspotLocation, &xmim_point); n++;
    XtSetArg(im_args[n], XmNlineSpace, 
	     TextF_FontAscent(new_tf) + TextF_FontDescent(new_tf)); n++;
    XmImSetValues((Widget)new_tf, im_args, n);

    if (new_font) XmFontListFree((XmFontList)TextF_FontList(old_tf));

    if (!redisplay) redisplay = new_tf->text.redisplay;

    /* If I'm forced to redisplay, then actual widget won't be updated
     * until the expose proc.  Force the ibeam putback to be refreshed
     * at expose time so that it reflects true visual state of the
     * widget.  */

    if (redisplay) new_tf->text.refresh_ibeam_off = True;

    new_tf->text.in_setvalues = False;

    /* 
     * Force new clip rectangles to be computed during redisplay,
     * *after* XtSetValues decides on final geometry. 
     */
    if (redisplay) new_tf->text.has_rect = False; 

    if ((!TextF_Editable(new_tf) || !XtIsSensitive(new_w)) &&
        new_tf->text.has_destination)
       (void) SetDestination(new_w, 0, False, (Time)0);

    /* don't shrink to nothing */
    if (new_tf->core.width == 0) new_tf->core.width = old_tf->core.width;
    if (new_tf->core.height == 0) new_tf->core.height = old_tf->core.height;

    if (!redisplay && redisplay_text) 
      RedisplayText(new_tf, 0, new_tf->text.string_length);

    return redisplay;
}
static Boolean 
#ifdef _NO_PROTO
TextFieldRemove( w, event)
        Widget w ;
        XEvent *event ;
#else
TextFieldRemove(
        Widget w,
	XEvent *event)
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmTextPosition left, right;
   XmAnyCallbackStruct cb;

   if (TextF_Editable(tf) == False)
      return False;

   if (!XmTextFieldGetSelectionPosition(w, &left, &right) || left == right) {
      tf->text.prim_anchor = TextF_CursorPosition(tf);
      return False;
   }

   if (_XmTextFieldReplaceText(tf, event, left, right, NULL, 0, True)){
      XmTextFieldSetSelection(w, TextF_CursorPosition(tf),
                              TextF_CursorPosition(tf),
			      XtLastTimestampProcessed(XtDisplay(w)));
      cb.reason = XmCR_VALUE_CHANGED;
      cb.event = event;
      XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		         (XtPointer) &cb);
   }
   tf->text.prim_anchor = TextF_CursorPosition(tf);

   return True;
}

/***********************************<->***************************************

 *                              Public Functions                             *
 ***********************************<->***************************************/

char * 
#ifdef _NO_PROTO
XmTextFieldGetString( w )
        Widget w ;
#else
XmTextFieldGetString(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   char *temp_str;
   int ret_val = 0;

   if (tf->text.string_length > 0) {
      if (tf->text.max_char_size == 1) {
         return(XtNewString(TextF_Value(tf)));
      } else {
	 temp_str = (char *) XtMalloc((unsigned) tf->text.max_char_size *
				      (tf->text.string_length + 1));
	 ret_val = wcstombs(temp_str, TextF_WcValue(tf), 
			 (tf->text.string_length + 1) * tf->text.max_char_size);
	 if (ret_val < 0) temp_str[0] = NULL;
	 return temp_str;
      }
   } else
      return(XtNewString(""));
}

int 
#ifdef _NO_PROTO
XmTextFieldGetSubstring( widget, start, num_chars, buf_size, buffer )
        Widget widget;
        XmTextPosition start;
        int num_chars;
        int buf_size;
        char *buffer;
#else
XmTextFieldGetSubstring(
	Widget widget,
        XmTextPosition start,
        int num_chars,
        int buf_size,
        char *buffer )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) widget;
   int ret_value = XmCOPY_SUCCEEDED;
   int n_bytes = 0;
   int wcs_ret = 0;

   if (tf->text.max_char_size != 1)
      n_bytes = _XmTextFieldCountBytes(tf, TextF_WcValue(tf)+start, num_chars);
   else
      n_bytes = num_chars; 

   if (buf_size < n_bytes + 1 )
      return XmCOPY_FAILED;

   if (start + num_chars > tf->text.string_length) {
      num_chars = (int) (tf->text.string_length - start);
      if (tf->text.max_char_size != 1)
         n_bytes = _XmTextFieldCountBytes(tf, TextF_WcValue(tf)+start,
                   num_chars);
      else
         n_bytes = num_chars; 
      ret_value = XmCOPY_TRUNCATED;
   }
      
   if (num_chars > 0) {
      if (tf->text.max_char_size == 1) {
	 (void)memcpy((void*)buffer, (void*)&TextF_Value(tf)[start], num_chars);
      } else {
	 wcs_ret = wcstombs(buffer, &TextF_WcValue(tf)[start], n_bytes);
	 if (wcs_ret < 0) n_bytes = 0;
      }
      buffer[n_bytes] = '\0';
   } else
      ret_value = XmCOPY_FAILED;

   return (ret_value);
}


wchar_t *
#ifdef _NO_PROTO
XmTextFieldGetStringWcs( w )
        Widget w ;
#else
XmTextFieldGetStringWcs(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   wchar_t *temp_wcs;
   int num_wcs = 0;

   if (tf->text.string_length > 0) {
      temp_wcs = (wchar_t*) XtMalloc((unsigned) sizeof(wchar_t) *
						(tf->text.string_length + 1));
      if (tf->text.max_char_size != 1) {
         (void)memcpy((void*)temp_wcs, (void*)TextF_WcValue(tf), 
                sizeof(wchar_t) * (tf->text.string_length + 1));
      } else {
         num_wcs = mbstowcs(temp_wcs, TextF_Value(tf),
                         tf->text.string_length + 1);
	 if (num_wcs < 0)
	    temp_wcs[0] = 0L;
      }
      return temp_wcs;
   } else {
      temp_wcs = (wchar_t*) XtMalloc((unsigned) sizeof(wchar_t));
      temp_wcs[0] = (wchar_t)0L; /* put a wchar_t NULL in position 0 */
      return temp_wcs;
   }
}

int 
#ifdef _NO_PROTO
XmTextFieldGetSubstringWcs( widget, start, num_chars, buf_size, buffer )
        Widget widget;
        XmTextPosition start;
        int num_chars;
        int buf_size;
        wchar_t *buffer;
#else
XmTextFieldGetSubstringWcs(
	Widget widget,
        XmTextPosition start,
        int num_chars,
        int buf_size,
        wchar_t *buffer )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) widget;
   int ret_value = XmCOPY_SUCCEEDED;
   int num_wcs = 0;

   if (start + num_chars > tf->text.string_length) {
      num_chars = (int) (tf->text.string_length - start);
      ret_value = XmCOPY_TRUNCATED;
   }
      
   if (buf_size < num_chars + 1 )
      return XmCOPY_FAILED;

   if (num_chars > 0) {
      if (tf->text.max_char_size == 1) {
	 num_wcs = mbstowcs(buffer, &TextF_Value(tf)[start], num_chars);
	 if (num_wcs < 0) num_chars = 0;
      } else {
	 (void)memcpy((void*)buffer, (void*)&TextF_WcValue(tf)[start], 
	       (size_t) num_chars * sizeof(wchar_t));
      }
      buffer[num_chars] = '\0';
   } else if (num_chars == 0) {
      buffer[num_chars] = '\0';
   } else
      ret_value = XmCOPY_FAILED;

   return (ret_value);
}


XmTextPosition 
#ifdef _NO_PROTO
XmTextFieldGetLastPosition( w )
        Widget w ;
#else
XmTextFieldGetLastPosition(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   return((tf->text.string_length));
}

void 
#ifdef _NO_PROTO
XmTextFieldSetString( w, value )
        Widget w ;
        char *value ;
#else
XmTextFieldSetString(
        Widget w,
        char *value )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmAnyCallbackStruct cb;
    XmTextPosition fromPos, toPos, newInsert;
    int length;
    int free_insert = False;
    int ret_val = 0;
    char * tmp_ptr;
    char * mod_value = NULL;

    fromPos = 0;

    if (value == NULL) value = "";
    toPos = tf->text.string_length;
    if (tf->text.max_char_size == 1)
      length = strlen(value);
    else {
      for (length = 0, tmp_ptr = value, ret_val = mblen(tmp_ptr, MB_CUR_MAX);
	   ret_val > 0; 
	   ret_val = mblen(tmp_ptr, MB_CUR_MAX)){
	 if (ret_val < 0){
	    length = 0; /* If error, treat the whole string as bad */
	    break;
	 } else {
	    length += ret_val;
	    tmp_ptr += ret_val;
	 }
      }
    }

    if (tf->core.sensitive && tf->text.has_focus)
        ChangeBlinkBehavior(tf, False);
    _XmTextFieldDrawInsertionPoint(tf, False);

    if (TextF_ModifyVerifyCallback(tf) || TextF_ModifyVerifyCallbackWcs(tf)) {
         /* If the function ModifyVerify() returns false then don't
          * continue with the action.
          */
	  if (tf->text.max_char_size == 1) {
             if (!ModifyVerify(tf, NULL, &fromPos, &toPos,
			       &value, &length, &newInsert, &free_insert)) {
                if (tf->text.verify_bell) XBell(XtDisplay(w), 0);
	        if (free_insert) XtFree(value);
                return;
             }
	  } else {
             wchar_t * wbuf;
             wchar_t * orig_wbuf;
             wbuf = (wchar_t*)XtMalloc((unsigned)
				       ((strlen(value) + 1) * sizeof(wchar_t)));
             length = mbstowcs(wbuf, value, (size_t)(strlen(value) + 1));
             if (length < 0) length = 0;
             orig_wbuf = wbuf; /* save the pointer to free later */

             if (!ModifyVerify(tf, NULL, &fromPos, &toPos, (char**)&wbuf, 
			       &length, &newInsert, &free_insert)) {
                if (tf->text.verify_bell) XBell(XtDisplay(w), 0);
	        if (free_insert) XtFree((char*)wbuf);
		XtFree((char*)orig_wbuf);
                return;
             }
	     else {
		mod_value = XtMalloc((unsigned)
				     ((length + 1) * tf->text.max_char_size));
		ret_val = wcstombs(mod_value, wbuf, (size_t)
				   ((length + 1) * tf->text.max_char_size));
	        if (free_insert) {
		   XtFree((char*)wbuf);
		   free_insert = False;
		}
		XtFree((char*)orig_wbuf);
		if (ret_val < 0){
		   XtFree(mod_value);
		   length = strlen(value);
		} else {
		   value = mod_value;
		}
	     }
	  }
    }

    XmTextFieldSetHighlight(w, 0, tf->text.string_length,
			    XmHIGHLIGHT_NORMAL);

    if (tf->text.max_char_size == 1)
       XtFree(TextF_Value(tf));
    else   /* convert to wchar_t before calling ValidateString */
       XtFree((char *)TextF_WcValue(tf));

    ValidateString(tf, value, False);
    if(mod_value) XtFree(mod_value);

    tf->text.pending_off = True;    

    SetCursorPosition(tf, NULL, 0, True, True, False);

    if (TextF_ResizeWidth(tf) && tf->text.do_resize)
       AdjustSize(tf);
    else {
       tf->text.h_offset = TextF_MarginWidth(tf) + 
	   		   tf->primitive.shadow_thickness +
                           tf->primitive.highlight_thickness;
       if (!AdjustText(tf, TextF_CursorPosition(tf), False))
          RedisplayText(tf, 0, tf->text.string_length);
    }

    cb.reason = XmCR_VALUE_CHANGED;
    cb.event = NULL;
    XtCallCallbackList(w, TextF_ValueChangedCallback(tf), (XtPointer) &cb);

    tf->text.refresh_ibeam_off = True;

    if (tf->core.sensitive && tf->text.has_focus)
        ChangeBlinkBehavior(tf, True);
    _XmTextFieldDrawInsertionPoint(tf, True);
    if (free_insert) XtFree(value);
}

void 
#ifdef _NO_PROTO
XmTextFieldSetStringWcs( w, wc_value )
        Widget w ;
        wchar_t *wc_value ;
#else
XmTextFieldSetStringWcs(
        Widget w,
        wchar_t *wc_value )
#endif /* _NO_PROTO */
{

   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   char * tmp;
   wchar_t *tmp_wc;
   int num_chars = 0;
   int result;

   for (num_chars = 0, tmp_wc = wc_value; *tmp_wc != (wchar_t)0L; num_chars++)
        tmp_wc++;  /* count number of wchar_t's */

   tmp = XtMalloc((unsigned) (num_chars + 1) * tf->text.max_char_size);
   result = wcstombs(tmp, wc_value, (num_chars + 1) * tf->text.max_char_size);

   if (result == (size_t) -1) /* if wcstombs fails, it returns (size_t) -1 */
      tmp = "";               /* if invalid data, pass in the empty string */

   XmTextFieldSetString(w, tmp);

   XtFree(tmp);
}


void 
#ifdef _NO_PROTO
XmTextFieldReplace( w, from_pos, to_pos, value )
        Widget w ;
        XmTextPosition from_pos ;
        XmTextPosition to_pos ;
        char *value ;
#else
XmTextFieldReplace(
        Widget w,
        XmTextPosition from_pos,
        XmTextPosition to_pos,
        char *value )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    int save_maxlength = TextF_MaxLength(tf);
    Boolean save_editable = TextF_Editable(tf);
    Boolean deselected = False;
    Boolean rep_result = False;
    wchar_t *wc_value;
    int length = 0;
    XmAnyCallbackStruct cb;

    if (value == NULL) value = "";

    VerifyBounds(tf, &from_pos, &to_pos);

    if (tf->text.has_primary) {
       if ((tf->text.prim_pos_left > from_pos && 
              tf->text.prim_pos_left < to_pos) || 
           (tf->text.prim_pos_right >from_pos && 
              tf->text.prim_pos_right < to_pos) ||
           (tf->text.prim_pos_left <= from_pos && 
              tf->text.prim_pos_right >= to_pos)) {
                _XmTextFieldDeselectSelection(w, False,
			                XtLastTimestampProcessed(XtDisplay(w)));
		deselected = True;
       }
    }

    TextF_Editable(tf) = True;
    TextF_MaxLength(tf) = MAXINT;
    if (tf->text.max_char_size == 1) {
       length = strlen(value);
       rep_result = _XmTextFieldReplaceText(tf, NULL, from_pos, 
				            to_pos, value, length, False);
    } else { /* need to convert to wchar_t* before calling Replace */
       wc_value = (wchar_t *) XtMalloc((unsigned) sizeof(wchar_t) *
				       (1 + strlen(value)));
       length = mbstowcs(wc_value, value, (unsigned) (strlen(value) + 1));
       if (length < 0)
	  length = 0;
       else
          rep_result = _XmTextFieldReplaceText(tf, NULL, from_pos, to_pos, 
				         (char*)wc_value, length, False);
       XtFree((char *)wc_value);
    }
    if (from_pos <= TextF_CursorPosition(tf)) {
      XmTextPosition cursorPos;
      /* Replace will not move us, we still want this to happen */
      if (TextF_CursorPosition(tf) < to_pos) {
	if (TextF_CursorPosition(tf) - from_pos <= length)
	  cursorPos = TextF_CursorPosition(tf);
	else
	  cursorPos = from_pos + length;
      } else {
	cursorPos = TextF_CursorPosition(tf) - (to_pos - from_pos) + length;
      }
      XmTextFieldSetInsertionPosition((Widget)tf, cursorPos);
    }
    TextF_Editable(tf) = save_editable;
    TextF_MaxLength(tf) = save_maxlength;

    /* 
     * Replace Text utilizes an optimization in deciding which text to redraw;
     * in the case that the selection has been changed (as above), this can
     * cause part/all of the replaced text to NOT be redrawn.  The following
     * AdjustText call ensures that it IS drawn in this case.
     */

    if (deselected)
       AdjustText(tf, from_pos, True);

    (void) SetDestination(w, TextF_CursorPosition(tf), False,
			  XtLastTimestampProcessed(XtDisplay(w)));
    if (rep_result) {
       cb.reason = XmCR_VALUE_CHANGED;
       cb.event = (XEvent *)NULL;
       XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		          (XtPointer) &cb);
    }
}

/* TOM - XmTextFieldReplaceWcs not converted */
void 
#ifdef _NO_PROTO
XmTextFieldReplaceWcs( w, from_pos, to_pos, wc_value )
        Widget w ;
        XmTextPosition from_pos ;
        XmTextPosition to_pos ;
        wchar_t *wc_value ;
#else
XmTextFieldReplaceWcs(
        Widget w,
        XmTextPosition from_pos,
        XmTextPosition to_pos,
        wchar_t *wc_value )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    int save_maxlength = TextF_MaxLength(tf);
    Boolean save_editable = TextF_Editable(tf);
    Boolean deselected = False;
    Boolean rep_result = False;
    wchar_t *tmp_wc;
    char *tmp;
    int wc_length = 0;
    XmAnyCallbackStruct cb;

    if (wc_value == NULL) wc_value = (wchar_t*)"";

    VerifyBounds(tf, &from_pos, &to_pos);

    if (tf->text.has_primary) {
       if ((tf->text.prim_pos_left > from_pos &&
              tf->text.prim_pos_left < to_pos) ||
           (tf->text.prim_pos_right >from_pos &&
              tf->text.prim_pos_right < to_pos) ||
           (tf->text.prim_pos_left <= from_pos &&
              tf->text.prim_pos_right >= to_pos)) {
                _XmTextFieldDeselectSelection(w, False,
			                XtLastTimestampProcessed(XtDisplay(w)));
                deselected = True;
       }
    }

   /* Count the number of wide chars in the array */
    for (wc_length = 0, tmp_wc = wc_value; *tmp_wc != (wchar_t)0L; wc_length++)
	tmp_wc++;  /* count number of wchar_t's */

    TextF_Editable(tf) = True;
    TextF_MaxLength(tf) = MAXINT;

    if (tf->text.max_char_size != 1) {
       rep_result = _XmTextFieldReplaceText(tf, NULL, from_pos, to_pos,
                                            (char*)wc_value, wc_length, False);
    } else {     /* need to convert to char* before calling Replace */
       tmp = XtMalloc((unsigned) (wc_length + 1) * tf->text.max_char_size);
       wc_length = wcstombs(tmp, wc_value, 
			    (wc_length + 1) * tf->text.max_char_size);

       if (wc_length == (size_t) -1){ /* if wcstombs fails, it returns -1 */
          tmp = "";                   /* if invalid data, pass in the empty 
                                       * string */
          wc_length = 0;
       }
       rep_result = _XmTextFieldReplaceText(tf, NULL, from_pos, to_pos,
                                            (char*)tmp, wc_length, False);
       XtFree(tmp);
    }
    if (from_pos <= TextF_CursorPosition(tf)) {
      XmTextPosition cursorPos;
      /* Replace will not move us, we still want this to happen */
      if (TextF_CursorPosition(tf) < to_pos) {
	if (TextF_CursorPosition(tf) - from_pos <= wc_length)
	  cursorPos = TextF_CursorPosition(tf);
	else
	  cursorPos = from_pos + wc_length;
      } else {
	cursorPos = TextF_CursorPosition(tf) - (to_pos - from_pos) + wc_length;
      }
      XmTextFieldSetInsertionPosition((Widget)tf, cursorPos);
    }
    TextF_Editable(tf) = save_editable;
    TextF_MaxLength(tf) = save_maxlength;

    /*
     * Replace Text utilizes an optimization in deciding which text to redraw;
     * in the case that the selection has been changed (as above), this can
     * cause part/all of the replaced text to NOT be redrawn.  The following
     * AdjustText call ensures that it IS drawn in this case.
     */

    if (deselected)
       AdjustText(tf, from_pos, True);

    (void) SetDestination(w, TextF_CursorPosition(tf), False,
			  XtLastTimestampProcessed(XtDisplay(w)));
    if (rep_result) {
       cb.reason = XmCR_VALUE_CHANGED;
       cb.event = (XEvent *)NULL;
       XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
		          (XtPointer) &cb);
    }
}


void 
#ifdef _NO_PROTO
XmTextFieldInsert( w, position, value )
        Widget w ;
        XmTextPosition position ;
        char *value ;
#else
XmTextFieldInsert(
        Widget w,
        XmTextPosition position,
        char *value )
#endif /* _NO_PROTO */
{
    /* XmTextFieldReplace takes care of converting to wchar_t* if needed */
    XmTextFieldReplace(w, position, position, value);
}

void 
#ifdef _NO_PROTO
XmTextFieldInsertWcs( w, position, wcstring )
        Widget w ;
        XmTextPosition position ;
        wchar_t *wcstring ;
#else
XmTextFieldInsertWcs(
        Widget w,
        XmTextPosition position,
        wchar_t *wcstring )
#endif /* _NO_PROTO */
{
    /* XmTextFieldReplaceWcs takes care of converting to wchar_t* if needed */
    XmTextFieldReplaceWcs(w, position, position, wcstring);
}

void 
#ifdef _NO_PROTO
XmTextFieldSetAddMode( w, state )
        Widget w ;
        Boolean state ;
#else
XmTextFieldSetAddMode(
        Widget w,
#if NeedWidePrototypes
        int state )
#else
        Boolean state )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   if (tf->text.add_mode == state) return;

   _XmTextFieldDrawInsertionPoint(tf, False);
   tf->text.add_mode = state;
   _XmTextFToggleCursorGC(w);
   _XmTextFieldDrawInsertionPoint(tf, True);
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldGetAddMode( w )
        Widget w ;
#else
XmTextFieldGetAddMode(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   return (tf->text.add_mode);
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldGetEditable( w )
        Widget w ;
#else
XmTextFieldGetEditable(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   return(TextF_Editable(tf));
}

void 
#ifdef _NO_PROTO
XmTextFieldSetEditable( w, editable )
        Widget w ;
        Boolean editable ;
#else
XmTextFieldSetEditable(
        Widget w,
#if NeedWidePrototypes
        int editable )
#else
        Boolean editable )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XPoint xmim_point;
   Arg args[6];  /* To set initial values to input method */
   Cardinal n = 0;

  /* if widget previously wasn't editable, no input method has yet been
   * registered.  So, if we're making it editable now, register the IM and
   * give the IM the relevent values. */

   if (!TextF_Editable(tf) && editable){ 
      XmImRegister((Widget)tf, (unsigned int) NULL);

      GetXYFromPos(tf, TextF_CursorPosition(tf), &xmim_point.x, 
		   &xmim_point.y);
      n = 0;
      XtSetArg(args[n], XmNfontList, TextF_FontList(tf)); n++;
      XtSetArg(args[n], XmNbackground, tf->core.background_pixel); n++;
      XtSetArg(args[n], XmNforeground, tf->primitive.foreground); n++;
      XtSetArg(args[n], XmNbackgroundPixmap,tf->core.background_pixmap);n++;
      XtSetArg(args[n], XmNspotLocation, &xmim_point); n++;
      XtSetArg(args[n], XmNlineSpace,
               TextF_FontAscent(tf)+ TextF_FontDescent(tf)); n++;
      XmImSetValues((Widget)tf, args, n);
   } else if (TextF_Editable(tf) && !editable){
       XmImUnregister(w);
   }

   TextF_Editable(tf) = editable;

   n = 0;
   if (editable) {
      XtSetArg(args[n], XmNdropSiteActivity, XmDROP_SITE_ACTIVE); n++;
   } else {
      XtSetArg(args[n], XmNdropSiteActivity, XmDROP_SITE_INACTIVE); n++;
   }

   XmDropSiteUpdate((Widget)tf, args, n);
}

int 
#ifdef _NO_PROTO
XmTextFieldGetMaxLength( w )
        Widget w ;
#else
XmTextFieldGetMaxLength(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   return(TextF_MaxLength(tf));
}

void 
#ifdef _NO_PROTO
XmTextFieldSetMaxLength( w, max_length )
        Widget w ;
        int max_length ;
#else
XmTextFieldSetMaxLength(
        Widget w,
        int max_length )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   TextF_MaxLength(tf) = max_length;
}

XmTextPosition 
#ifdef _NO_PROTO
XmTextFieldGetCursorPosition( w )
        Widget w ;
#else
XmTextFieldGetCursorPosition(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   return TextF_CursorPosition(tf);
}

XmTextPosition 
#ifdef _NO_PROTO
XmTextFieldGetInsertionPosition( w )
        Widget w ;
#else
XmTextFieldGetInsertionPosition(
        Widget w )
#endif /* _NO_PROTO */
{
   return XmTextFieldGetCursorPosition(w);
}

/* Obsolete - shouldn't be here ! */
void 
#ifdef _NO_PROTO
XmTextFieldSetCursorPosition( w, position )
        Widget w ;
        XmTextPosition position ;
#else
XmTextFieldSetCursorPosition(
        Widget w,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   SetCursorPosition(tf, NULL, position, True, False, False);
}

void 
#ifdef _NO_PROTO
XmTextFieldSetInsertionPosition( w, position )
        Widget w ;
        XmTextPosition position ;
#else
XmTextFieldSetInsertionPosition(
        Widget w,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   SetCursorPosition(tf, NULL, position, True, True, False);
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldGetSelectionPosition( w, left, right )
        Widget w ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
XmTextFieldGetSelectionPosition(
        Widget w,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;

   if (!tf->text.has_primary) return False;

   *left = tf->text.prim_pos_left;
   *right = tf->text.prim_pos_right;

   return True;
}

char * 
#ifdef _NO_PROTO
XmTextFieldGetSelection( w )
        Widget w ;
#else
XmTextFieldGetSelection(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   size_t length, num_chars;
   char *value;

   if (tf->text.prim_pos_left == tf->text.prim_pos_right)
	 return NULL;
   num_chars = (size_t) (tf->text.prim_pos_right - tf->text.prim_pos_left);
   length = num_chars;
   if (tf->text.max_char_size == 1) {
      value = XtMalloc((unsigned) num_chars + 1);
      (void) memcpy((void*)value, 
		  (void*)(TextF_Value(tf) + tf->text.prim_pos_left), num_chars);
   } else {
      value = XtMalloc((unsigned) ((num_chars + 1) * tf->text.max_char_size));
      length = wcstombs(value, TextF_WcValue(tf) + tf->text.prim_pos_left, 
                        (num_chars + 1) * tf->text.max_char_size);
      if (length == (size_t) -1) {
	 length = 0;
      } else {
	for (length = 0;num_chars > 0; num_chars--)
	  length += mblen(&value[length], tf->text.max_char_size);
      }
   }
   value[length] = (char)'\0';
   return (value);
}

wchar_t *
#ifdef _NO_PROTO
XmTextFieldGetSelectionWcs( w )
        Widget w ;
#else
XmTextFieldGetSelectionWcs(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   size_t length;
   wchar_t *wc_value;
   int return_val = 0;

   if (tf->text.prim_pos_left == tf->text.prim_pos_right)
         return NULL;
   length = (size_t) (tf->text.prim_pos_right - tf->text.prim_pos_left);

   wc_value = (wchar_t*)XtMalloc((unsigned) (length + 1) * sizeof(wchar_t));

   if (tf->text.max_char_size == 1) {
      return_val = mbstowcs(wc_value, TextF_Value(tf) + tf->text.prim_pos_left, 
                            length);
      if (return_val < 0)
	 length = 0;
   } else {
      (void)memcpy((void*)wc_value, 
	     (void*)(TextF_WcValue(tf) + tf->text.prim_pos_left), 
             length * sizeof(wchar_t));
   }
   wc_value[length] = (wchar_t)0L;
   return (wc_value);
}


Boolean 
#ifdef _NO_PROTO
XmTextFieldRemove( w )
        Widget w ;
#else
XmTextFieldRemove(
        Widget w )
#endif /* _NO_PROTO */
{
  return TextFieldRemove(w, NULL);
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldCopy( w, clip_time )
        Widget w ;
        Time clip_time ;
#else
XmTextFieldCopy(
        Widget w,
        Time clip_time )
#endif /* _NO_PROTO */
{
   /* XmTextFieldGetSelection gets char* rep of data, so no special handling
    * needed
    */
   char * selected_string = XmTextFieldGetSelection(w); /* text selection */
   long item_id = 0L;                      /* clipboard item id */
   long data_id = 0;                        /* clipboard data id */
   int status = 0;                         /* clipboard status  */
   XmString clip_label;
   XTextProperty tmp_prop;
   Display *display = XtDisplay(w);
   Window window = XtWindow(w);
   char *atom_name;

  /* using the clipboard facilities, copy the selected text to the clipboard */
   if (selected_string != NULL) {
      clip_label = XmStringCreateLtoR ("XM_TEXT_FIELD",
				       XmFONTLIST_DEFAULT_TAG);
     /* start copy to clipboard */
      status = XmClipboardStartCopy(display, window, clip_label, clip_time,
				     w, (XmCutPasteProc)NULL, &item_id);

      if (status != ClipboardSuccess) {
	XtFree(selected_string);
	XmStringFree(clip_label);
	return False;
      }

#ifdef NON_OSF_FIX
	tmp_prop.value = NULL;
#endif
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
	XmClipboardCancelCopy(XtDisplay(w), XtWindow(w), item_id);
	XtFree(selected_string);
	XmStringFree(clip_label);
        return False;
      }

     /* end the copy to the clipboard */
      status = XmClipboardEndCopy(display, window, item_id);
#ifdef NON_OSF_FIX
      if (tmp_prop.value != NULL) XFree((char *)tmp_prop.value);
#endif
      XmStringFree(clip_label);

      if (status != ClipboardSuccess) {
	XtFree (selected_string);
	return False;
      }
	
   } else
     {
       return False;
     }
   if (selected_string != NULL)
     XtFree (selected_string);

   return True;
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldCut( w, clip_time )
        Widget w ;
        Time clip_time ;
#else
XmTextFieldCut(
        Widget w,
        Time clip_time )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Boolean success = False;

    if (TextF_Editable(tf) == False)
       return False;

    if (XmTextFieldCopy(w, clip_time))
       if (XmTextFieldRemove(w))
          success = True;
    return success;
}


/*
 * Retrieves the current data from the clipboard
 * and paste it at the current cursor position
 */
Boolean 
#ifdef _NO_PROTO
XmTextFieldPaste( w )
        Widget w ;
#else
XmTextFieldPaste(
        Widget w )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   XmTextPosition sel_left = 0;
   XmTextPosition sel_right = 0;
   XmTextPosition paste_pos_left, paste_pos_right;
   int status = 0;                            /* clipboard status        */
   char* buffer;                              /* temporary text buffer   */
   unsigned long length;                      /* length of buffer        */
   unsigned long outlength = 0L;              /* length of bytes copied  */
   long private_id = 0;                       /* id of item on clipboard */
   Boolean dest_disjoint = True;
   Boolean rep_status = False;                /* did Replace succeed? */
   Display *display = XtDisplay(w);
   Window window = XtWindow(w);
   Boolean get_ct = False;
   XTextProperty tmp_prop;
   int malloc_size = 0;
   int num_vals;
   char **tmp_value;
   int i;
   XmAnyCallbackStruct cb;

   if (TextF_Editable(tf) == False) return False;

   paste_pos_left = paste_pos_right = TextF_CursorPosition(tf);

   status = XmClipboardInquireLength(display, window, "STRING", &length);

   if (status == ClipboardNoData || length == 0) {
      status = XmClipboardInquireLength(display, window, "COMPOUND_TEXT",
					&length);
      if (status == ClipboardNoData || length == 0) return False;
      get_ct = True;
   }

   /* malloc length of clipboard data */
   buffer = XtMalloc((unsigned) length);

   if (!get_ct) {
      status = XmClipboardRetrieve(display, window, "STRING", buffer,
				    length, &outlength, &private_id);
   } else {
      status = XmClipboardRetrieve(display, window, "COMPOUND_TEXT", buffer,
				    length, &outlength, &private_id);
   }

   if (status != ClipboardSuccess) {
      XmClipboardEndRetrieve(display, window);
      XtFree(buffer);
      return False;
   }

   if (XmTextFieldGetSelectionPosition(w, &sel_left, &sel_right)) {
      if (tf->text.pending_delete &&
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

  /* add new text */
   if (num_vals && (status == Success || status > 0)) {
      if (tf->text.max_char_size == 1){
	 char * total_tmp_value;

	 for (i = 0, malloc_size = 1; i < num_vals ; i++)
	    malloc_size += strlen(tmp_value[i]);
	 total_tmp_value = XtMalloc ((unsigned) malloc_size);
	 total_tmp_value[0] = '\0';
	 for (i = 0; i < num_vals ; i++)
	    strcat(total_tmp_value, tmp_value[i]);
	 rep_status = _XmTextFieldReplaceText(tf, NULL, paste_pos_left,
					      paste_pos_right, 
					      total_tmp_value,
					      strlen(total_tmp_value), True);
	 XFreeStringList(tmp_value);
	 if (malloc_size) XtFree(total_tmp_value);
      } else {
	 wchar_t * wc_value;
         int num_chars = 0;
	 int return_val = 0;

	 for (i = 0, malloc_size = sizeof(wchar_t); i < num_vals ; i++)
	    malloc_size += strlen(tmp_value[i]);
	 wc_value = (wchar_t*)XtMalloc((unsigned)malloc_size * sizeof(wchar_t));
        /* change malloc_size to the number of wchars available in wc_value */
	 for (i = 0; i < num_vals ; i++){
	    return_val = mbstowcs(wc_value + num_chars, tmp_value[i],
			  (size_t)malloc_size - num_chars);
            if (return_val > 0) num_chars += return_val;
	 }
	 rep_status = _XmTextFieldReplaceText(tf, NULL, paste_pos_left,
					      paste_pos_right,
					      (char*)wc_value, 
					      num_chars, True);
	 if (malloc_size) XtFree((char*)wc_value);
      }
   }

   if (rep_status) {
       tf->text.prim_anchor = sel_left;
       (void) SetDestination(w, TextF_CursorPosition(tf), False,
			     XtLastTimestampProcessed(display));
       if (sel_left != sel_right) {
           if (!dest_disjoint || !tf->text.add_mode) {
              XmTextFieldSetSelection(w, TextF_CursorPosition(tf),
                                      TextF_CursorPosition(tf),
				      XtLastTimestampProcessed(display));
           }
        }
       cb.reason = XmCR_VALUE_CHANGED;
       cb.event = (XEvent *)NULL;
       XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
     		          (XtPointer) &cb);
   }
   XtFree(buffer);

   return True;
}

void 
#ifdef _NO_PROTO
XmTextFieldClearSelection( w, sel_time )
        Widget w ;
        Time sel_time ;
#else
XmTextFieldClearSelection(
        Widget w,
        Time sel_time )
#endif /* _NO_PROTO */
{
    _XmTextFieldDeselectSelection(w, False, sel_time);
}

void 
#ifdef _NO_PROTO
XmTextFieldSetSelection( w, first, last, sel_time )
        Widget w ;
        XmTextPosition first ;
        XmTextPosition last ;
        Time sel_time ;
#else
XmTextFieldSetSelection(
        Widget w,
        XmTextPosition first,
        XmTextPosition last,
        Time sel_time )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    _XmTextFieldStartSelection(tf, first, last, sel_time);
    tf->text.pending_off = False;
    SetCursorPosition(tf, NULL, last, True, True, False);
}

/* ARGSUSED */
XmTextPosition 
#ifdef _NO_PROTO
XmTextFieldXYToPos( w, x, y )
        Widget w ;
        Position x ;
        Position y ;
#else
XmTextFieldXYToPos(
        Widget w,
#if NeedWidePrototypes
        int x,
        int y )
#else
        Position x,
        Position y )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    return(GetPosFromX(tf, x));
}

Boolean 
#ifdef _NO_PROTO
XmTextFieldPosToXY( w, position, x, y )
        Widget w ;
        XmTextPosition position ;
        Position *x ;
        Position *y ;
#else
XmTextFieldPosToXY(
        Widget w,
        XmTextPosition position,
        Position *x,
        Position *y )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    return(GetXYFromPos(tf, position, x, y));
}


/*
 * Force the given position to be displayed.  If position < 0, then don't force
 * any position to be displayed.
 */
void 
#ifdef _NO_PROTO
XmTextFieldShowPosition( w, position )
        Widget w ;
        XmTextPosition position ;
#else
XmTextFieldShowPosition(
        Widget w,
        XmTextPosition position )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (position < 0) return;

    AdjustText(tf, position, True);
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
XmTextFieldSetHighlight( w, left, right, mode )
        Widget w ;
        XmTextPosition left ;
        XmTextPosition right ;
        XmHighlightMode mode ;
#else
XmTextFieldSetHighlight(
        Widget w,
        XmTextPosition left,
        XmTextPosition right,
        XmHighlightMode mode )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (left >= right || right <= 0) return;

    if (left < 0) left = 0;

    if (right > tf->text.string_length)
       right = tf->text.string_length;

    TextFieldSetHighlight(tf, left, right, mode);

    RedisplayText(tf, left, right);

}

/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
TextFieldGetBaselines( w, baselines, line_count )
        Widget w ;
        Dimension ** baselines;
        int *line_count;
#else
TextFieldGetBaselines(
        Widget w,
        Dimension ** baselines,
        int *line_count )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Dimension *base_array;
    
    *line_count = 1;

    base_array = (Dimension *) XtMalloc(sizeof(Dimension));

    base_array[0] = tf->text.margin_top + tf->primitive.shadow_thickness +
		    tf->primitive.highlight_thickness + TextF_FontAscent(tf);

   *baselines = base_array;

    return (TRUE);
}

int 
#ifdef _NO_PROTO
XmTextFieldGetBaseline( w )
        Widget w ;
#else
XmTextFieldGetBaseline(
        Widget w )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Dimension margin_top = tf->text.margin_top +
	                      tf->primitive.shadow_thickness +
			      tf->primitive.highlight_thickness;
    
    return((int) margin_top + (int) TextF_FontAscent(tf));
}

static Boolean
#ifdef _NO_PROTO
TextFieldGetDisplayRect( w, display_rect )
        Widget w;
        XRectangle * display_rect;
#else
TextFieldGetDisplayRect(
        Widget w,
        XRectangle * display_rect )
#endif /* _NO_PROTO */
{
   XmTextFieldWidget tf = (XmTextFieldWidget) w;
   Position margin_width = TextF_MarginWidth(tf) +
	               	   tf->primitive.shadow_thickness +
		       	   tf->primitive.highlight_thickness;
   Position margin_top = tf->text.margin_top + tf->primitive.shadow_thickness +
                       	 tf->primitive.highlight_thickness;
   Position margin_bottom = tf->text.margin_bottom +
			    tf->primitive.shadow_thickness +
                       	    tf->primitive.highlight_thickness;
   (*display_rect).x = margin_width;
   (*display_rect).y = margin_top;
   (*display_rect).width = tf->core.width - (2 * margin_width);
   (*display_rect).height = tf->core.height - (margin_top + margin_bottom);

   return(TRUE);
}


/* ARGSUSED */
static void
#ifdef _NO_PROTO
TextFieldMarginsProc( w, margins_rec )
        Widget w ;
        XmBaselineMargins *margins_rec;
#else
TextFieldMarginsProc(
        Widget w,
        XmBaselineMargins *margins_rec )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;

    if (margins_rec->get_or_set == XmBASELINE_SET) {
       tf->text.margin_top = margins_rec->margin_top;
       tf->text.margin_bottom = margins_rec->margin_bottom;
    } else {
       margins_rec->margin_top = tf->text.margin_top;
       margins_rec->margin_bottom = tf->text.margin_bottom;
       margins_rec->text_height = TextF_FontAscent(tf) + TextF_FontDescent(tf);
       margins_rec->shadow = tf->primitive.shadow_thickness;
       margins_rec->highlight = tf->primitive.highlight_thickness;
    }
}

/*
 * Text Field w creation convienence routine.
 */

Widget 
#ifdef _NO_PROTO
XmCreateTextField( parent, name, arglist, argcount )
        Widget parent ;
        char *name ;
        ArgList arglist ;
        Cardinal argcount ;
#else
XmCreateTextField(
        Widget parent,
        char *name,
        ArgList arglist,
        Cardinal argcount )
#endif /* _NO_PROTO */
{
    return (XtCreateWidget(name, xmTextFieldWidgetClass,
                           parent, arglist, argcount));
}

/****************************************************************/
/****************************************************************/
