#pragma ident	"@(#)m1.2libs:Xm/List.c	1.29"
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
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#include <string.h>
#include <stdio.h>
#include <X11/Xatom.h>
#ifdef OSF_v1_2_4
#include <Xm/XmosP.h>	/* For ALLOCATE/DEALLOCATE_LOCAL. */
#endif /* OSF_v1_2_4 */
#include "XmI.h"
#include <Xm/AtomMgr.h>
#include <Xm/ScrollBarP.h>
#include <Xm/ScrolledWP.h>
#include <Xm/ListP.h>
#include <Xm/DragIconP.h>
#include <Xm/CutPaste.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropTrans.h>
#include <Xm/ScreenP.h>
#ifndef OSF_v1_2_4
#include <Xm/XmosP.h>	/* For ALLOCATE/DEALLOCATE_LOCAL. */
#endif /* OSF_v1_2_4 */


#ifdef NOVELL
#define LPART(w)	( ((XmListWidget)(w))->list )
#define PPART(w)	( ((XmListWidget)(w))->primitive )
#define CPART(w)	( ((XmListWidget)(w))->core )

	/* Return True if we located an empty column in last row */
#define EmptyColInLastRow(JthRow,Rows,LastRowSize,IthCol)\
		(JthRow == Rows - 1 && LastRowSize <= IthCol)

	/* The following definitions are used by CalcItemBound
	 * and CalcItemWidth */
#define ITEM_ON_RIGHT	( 1L << 1 )
#define ITEM_ON_LEFT	( 1L << 2 )
#define ITEM_ON_TOP	( 1L << 3 )
#define ITEM_ON_BELOW	( 1L << 4 )
#define ITEM_ON_VIEW	( 1L << 5 )

	/* The following flag is used by APIAddItems(), primary because
	 * the weird assumption in XmListAddItemUnselected() and
	 * XmListAddItem(), in these two routines, they assume that
	 * the given item is already not in or is already in the
	 * OnSelectedList[], unlike the XmListAddItems() and
	 * XmListAddItemsUnselected(). */
#define IGNORE_CHECK	( 1L << 2 )

	/* The following definitions is used by ListProcessDrag(),
	 * ListConvert(), ListCopyToClipboard(). */
#define SETUP_SELECTED_LIST	( 1L << 1 )
#define SETUP_CT_DATA		( 1L << 2 )
#define FREE_SELECTED_LIST	( 1L << 3 )

	/* Note that, a title item is not considered as a regular item.
	 * `top_position' is interpreted as top_row currently and
	 *	`top_position' is `static_rows'-based!
	 * `visibleItemCount' is interpreted as visibleRows (including
	 *	static_rows) currently.
	 */
								/* 0-based */
#define TOP_ITEM_ROW(w)	(LPART(w).top_position + (int)LPART(w).static_rows)
								/* 1-based */
#define BOT_ITEM_ROW(w)	(LPART(w).top_position + LPART(w).visibleItemCount)
#define TOP_VIZ_ITEM(w) TOP_ITEM_ROW(w) * (int)LPART(w).cols
#define BOT_VIZ_ITEM(w)	BOT_ITEM_ROW(w) * (int)LPART(w).cols - 1
#define FIRST_ITEM(w)	(int)(LPART(w).static_rows * LPART(w).cols)
#define TITLE_ROW(w,r)	(LPART(w).static_rows && r < (int)LPART(w).static_rows)
#define TITLE_ROW_ITEM(w,p)\
			(LPART(w).static_rows && p < FIRST_ITEM(w))
#endif /* NOVELL */



#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define	BUTTONDOWN 1
#define	SHIFTDOWN 2
#define	CTRLDOWN 4
#define	ALTDOWN 8
#define	TOPLEAVE 1
#define	BOTTOMLEAVE 2
#define	LEFTLEAVE 4
#define	RIGHTLEAVE 8
#define	CHAR_WIDTH_GUESS 10
#define NEWLINESTRING		"\012"
#define NEWLINESTRING_LEN	sizeof(NEWLINESTRING)-1
/****************
 *
 * List Error Messages
 *
 ****************/


#ifdef I18N_MSG
#define ListMessage0	catgets(Xm_catd,MS_LIst,MSG_LI_0,_XmMsgList_0000)
#define ListMessage5	catgets(Xm_catd,MS_LIst,MSG_LI_5,_XmMsgList_0005)
#define ListMessage6	catgets(Xm_catd,MS_LIst,MSG_LI_6,_XmMsgList_0006)
#define ListMessage8	catgets(Xm_catd,MS_LIst,MSG_LI_8,_XmMsgList_0007)
#define ListMessage11	catgets(Xm_catd,MS_LIst,MSG_LI_11,_XmMsgList_0008)
#define ListMessage12	catgets(Xm_catd,MS_LIst,MSG_LI_12,_XmMsgList_0009)
#define ListMessage13	catgets(Xm_catd,MS_LIst,MSG_LI_13,_XmMsgList_0010)
#define ListMessage14	catgets(Xm_catd,MS_LIst,MSG_LI_14,_XmMsgList_0011)
#define ListMessage15	catgets(Xm_catd,MS_LIst,MSG_LI_15,_XmMsgList_0012)
#define ListMessage16	catgets(Xm_catd,MS_LIst,MSG_LI_16,_XmMsgList_0013)
#else
#define ListMessage0	_XmMsgList_0000
#define ListMessage5	_XmMsgList_0005
#define ListMessage6	_XmMsgList_0006
#define ListMessage8	_XmMsgList_0007
#define ListMessage11	_XmMsgList_0008
#define ListMessage12	_XmMsgList_0009
#define ListMessage13	_XmMsgList_0010
#define ListMessage14	_XmMsgList_0011
#define ListMessage15	_XmMsgList_0012
#define ListMessage16	_XmMsgList_0013
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ListSetValuesAlmost();
static void VertSliderMove() ;
static void HorizSliderMove() ;
static void UpdateHighlight() ;
static void NullRoutine() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Redisplay() ;
static void Resize() ;
static int SetVizCount() ;
static Boolean SetValues() ;
static void Destroy() ;
static XtGeometryResult QueryProc() ;
static void CvtToExternalPos() ;
static XmImportOperator CvtToInternalPos() ;
#ifndef NOVELL
static void DrawListShadow() ;
#endif /* NOVELL */
static void DrawList() ;
static void DrawItem() ;
static void DrawHighlight() ;
static void SetClipRect() ;
static void SetDefaultSize() ;
static void MakeGC() ;
static void MakeHighlightGC() ;
static void ChangeHighlightGC() ;
static void SetVerticalScrollbar() ;
static void SetHorizontalScrollbar() ;
static void SetMaxWidth() ;
static void SetMaxHeight() ;
#ifdef NOVELL
static Boolean SetNewSize() ;
#else /* NOVELL */
static void SetNewSize() ;
static void ResetHeight() ;
static void ResetWidth() ;
#endif /* NOVELL */
static void FixStartEnd();
#ifdef NOVELL
static Boolean AddInternalElement() ;
#else /* NOVELL */
static void AddInternalElement() ;
static void DeleteInternalElement() ;
#endif /* NOVELL */
static void DeleteInternalElementPositions() ;
static void ReplaceInternalElement() ;
static void AddItem() ;
#ifndef NOVELL
static void DeleteItem() ;
#endif /* NOVELL */
static void DeleteItemPositions() ;
static void ReplaceItem() ;
static int ItemNumber() ;
#ifndef NOVELL
static int ItemExists() ;
#endif /* NOVELL */
static Boolean OnSelectedList() ;
static void CopyItems() ;
static void CopySelectedItems() ;
static void ClearItemList() ;
static void ClearSelectedList() ;
static void BuildSelectedList() ;
static void UpdateSelectedList() ;
static void UpdateSelectedIndices() ;
static int WhichItem() ;
static void SelectRange() ;
static void RestoreRange() ;
static void ArrangeRange() ;
static void HandleNewItem() ;
static void HandleExtendedItem() ;
static void VerifyMotion() ;
static void SelectElement() ;
static void KbdSelectElement() ;
static void UnSelectElement() ;
static void KbdUnSelectElement() ;
static void ExSelect() ;
static void ExUnSelect() ;
static void CtrlSelect() ;
static void CtrlUnSelect() ;
static void KbdShiftSelect() ;
static void KbdShiftUnSelect() ;
static void KbdCtrlSelect() ;
static void KbdCtrlUnSelect() ;
static void KbdActivate() ;
static void KbdCancel() ;
static void KbdToggleAddMode() ;
static void KbdSelectAll() ;
static void KbdDeSelectAll() ;
static void DefaultAction() ;
static void ClickElement() ;
static void ListFocusIn() ;
static void ListFocusOut() ;
static void BrowseScroll() ;
static void ListLeave() ;
static void ListEnter() ;
#ifdef NOVELL
static Boolean MakeItemVisible() ;
static void MColumnPrevElement() ;
static void MColumnNextElement() ;
#else /* NOVELL */
static void MakeItemVisible() ;
#endif /* NOVELL */
static void PrevElement() ;
static void NextElement() ;
static void NormalNextElement() ;
static void ShiftNextElement() ;
static void CtrlNextElement() ;
static void ExtendAddNextElement() ;
static void NormalPrevElement() ;
static void ShiftPrevElement() ;
static void CtrlPrevElement() ;
static void ExtendAddPrevElement() ;
static void KbdPrevPage() ;
static void KbdNextPage() ;
static void KbdLeftChar() ;
static void KbdLeftPage() ;
static void BeginLine() ;
static void KbdRightChar() ;
static void KbdRightPage() ;
static void EndLine() ;
static void TopItem() ;
static void EndItem() ;
static void ExtendTopItem() ;
static void ExtendEndItem() ;
static void ListItemVisible() ;
static void ListCopyToClipboard() ;
static void DragDropFinished() ;
static void ListProcessDrag() ;
static Boolean ListConvert() ;
static void APIAddItems() ;
static void CleanUpList() ;
static void APIReplaceItems() ;
static void APIReplaceItemsPos() ;
static void APISelect() ;
static void SetSelectionParams() ;
#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection() ;
static void ProcessPress() ;
#endif /* CDE_INTEGRATE */
#ifdef CDE_FILESB
static void ScrollBarDisplayPolicyDefault() ;
#endif

#ifdef NOVELL

static void	CalcCumHeight();
static int	CalcItemBound();
static int	CalcItemWidth();
static void	CalcNumRows();
static void	CalcNewSize();
static void	CalibrateHsb();
static Boolean	CalibrateVsb();
static Boolean	CheckHsb();
static void	CreateInternalList();
static void	DestroyInternalList();
static void	ExtendToTopOrBot();
static void	FreeGlyphData();
static void	GotoTopOrEnd();
static void	ItemDraw();
static void	ReplacePositions();
static void	SetElement();
static void	SetNewTopNKbdItem();
static Boolean	SetVizPos();
static Boolean	TreatSelectionData();
static void	ValidateRepValues();

#endif /* NOVELL */

#else

#ifdef NOVELL

static void	CalcCumHeight(Widget);
static int	CalcItemBound(Widget, int, int, unsigned long *, int *,
							int *, int *, int *);
static int	CalcItemWidth(Widget, int *, unsigned long *, int, int,
							int, int, int, int);
static void	CalcNumRows(Widget, int *, int *);
static void	CalcNewSize(XmListWidget, Boolean);
static void	CalibrateHsb(Widget, int, unsigned long);
static Boolean	CalibrateVsb(Widget, int, unsigned long, int);
static Boolean	CheckHsb(Widget, int);
static void	CreateInternalList(Widget);
static void	DestroyInternalList(Widget);
static void	ExtendToTopOrBot(Widget, Boolean);
static void	FreeGlyphData(Widget, ElementPtr);
static void	GotoTopOrEnd(Widget, unsigned long);
static void	ItemDraw(Widget, Boolean, int, int, int, int, int, int, int);
static void	ReplacePositions(Widget, int *, XmString *, int, Boolean);
static void	SetElement(Widget, XmString, int, ElementPtr);
static void	SetNewTopNKbdItem(Widget, int, int);
static Boolean	SetVizPos(Widget, int, unsigned long);
static Boolean	TreatSelectionData(Widget, unsigned long,
				XmListDragConvertStruct *, int *, char **);
static void	ValidateRepValues(Widget, unsigned long, unsigned char,
				unsigned char, XmStringDirection);

#endif /* NOVELL */

static void ListSetValuesAlmost( Widget,
				 Widget,
				 XtWidgetGeometry*,
				 XtWidgetGeometry*
				);
static void VertSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void HorizSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void UpdateHighlight( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void NullRoutine( 
                        Widget wid) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget request,
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget wid) ;
static int SetVizCount( 
                        XmListWidget lw) ;
static Boolean SetValues( 
                        Widget old,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget wid) ;
static XtGeometryResult QueryProc( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *ret) ;
static void CvtToExternalPos( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator CvtToInternalPos( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
#ifndef NOVELL
static void DrawListShadow( 
                        XmListWidget w) ;
#endif /* NOVELL */
static void DrawList( 
                        XmListWidget w,
                        XEvent *event,
#if NeedWidePrototypes
                        int all) ;
#else
                        Boolean all) ;
#endif /* NeedWidePrototypes */
static void DrawItem( 
                        Widget w,
                        int position) ;
static void DrawHighlight( 
                        XmListWidget lw,
                        int position,
#if NeedWidePrototypes
                        int on) ;
#else
                        Boolean on) ;
#endif /* NeedWidePrototypes */
static void SetClipRect( 
                        XmListWidget widget) ;
static void SetDefaultSize( 
                        XmListWidget lw,
                        Dimension *width,
                        Dimension *height) ;
static void MakeGC( 
                        XmListWidget lw) ;
static void MakeHighlightGC( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int AddMode) ;
#else
                        Boolean AddMode) ;
#endif /* NeedWidePrototypes */
static void ChangeHighlightGC( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int AddMode) ;
#else
                        Boolean AddMode) ;
#endif /* NeedWidePrototypes */
static void SetVerticalScrollbar( 
                        XmListWidget lw) ;
static void SetHorizontalScrollbar( 
                        XmListWidget lw) ;
static void SetMaxWidth( 
                        XmListWidget lw) ;
static void SetMaxHeight( 
                        XmListWidget lw) ;
#ifdef NOVELL
static Boolean SetNewSize( 
                        XmListWidget lw) ;
#else /* NOVELL */
static void SetNewSize( 
                        XmListWidget lw) ;
static void ResetHeight( 
                        XmListWidget lw) ;
static void ResetWidth( 
                        XmListWidget lw) ;
#endif /* NOVELL */
static void FixStartEnd(
			int pos,
			int *start,
			int *end);
#ifdef NOVELL
static Boolean AddInternalElement( 
                        XmListWidget lw,
                        XmString string,
                        int position,
#if NeedWidePrototypes
                        int selected,
                        int do_alloc) ;
#else
                        Boolean selected,
                        Boolean do_alloc) ;
#endif /* NeedWidePrototypes */
#else /* NOVELL */
static void AddInternalElement( 
                        XmListWidget lw,
                        XmString string,
                        int position,
#if NeedWidePrototypes
                        int selected,
                        int do_alloc) ;
#else
                        Boolean selected,
                        Boolean do_alloc) ;
#endif /* NeedWidePrototypes */
static void DeleteInternalElement( 
                        XmListWidget lw,
                        XmString string,
                        int position,
#if NeedWidePrototypes
                        int DoAlloc) ;
#else
                        Boolean DoAlloc) ;
#endif /* NeedWidePrototypes */
#endif /* NOVELL */
static void DeleteInternalElementPositions( 
                        XmListWidget lw,
                        int *position_list,
                        int position_count,
                        int oldItemCount,
#if NeedWidePrototypes
                        int DoAlloc) ;
#else
                        Boolean DoAlloc) ;
#endif /* NeedWidePrototypes */
static void ReplaceInternalElement( 
                        XmListWidget lw,
                        int position,
#if NeedWidePrototypes
                        int selected) ;
#else
                        Boolean selected) ;
#endif /* NeedWidePrototypes */
static void AddItem( 
                        XmListWidget lw,
                        XmString item,
                        int pos) ;
#ifndef NOVELL
static void DeleteItem( 
                        XmListWidget lw,
                        int pos) ;
#endif /* NOVELL */
static void DeleteItemPositions( 
                        XmListWidget lw,
                        int *position_list,
                        int position_count) ;
static void ReplaceItem( 
                        XmListWidget lw,
                        XmString item,
                        int pos) ;
static int ItemNumber( 
                        XmListWidget lw,
                        XmString item) ;
#ifndef NOVELL
static int ItemExists( 
                        XmListWidget lw,
                        XmString item) ;
#endif /* NOVELL */
static Boolean OnSelectedList( 
                        XmListWidget lw,
                        XmString item) ;
static void CopyItems( 
                        XmListWidget lw) ;
static void CopySelectedItems( 
                        XmListWidget lw) ;
static void ClearItemList( 
                        XmListWidget lw) ;
static void ClearSelectedList( 
                        XmListWidget lw) ;
static void BuildSelectedList( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int commit) ;
#else
                        Boolean commit) ;
#endif /*  NeedWidePrototypes */ 
static void UpdateSelectedList( 
                        XmListWidget lw) ;
static void UpdateSelectedIndices( 
                        XmListWidget lw) ;
static int WhichItem( 
                        XmListWidget w,
#if NeedWidePrototypes
#ifdef NOVELL
			Boolean	static_rows_as_topleave,
			int EventX,
#endif /* NOVELL */
                        int EventY) ;
#else
#ifdef NOVELL
			Boolean	static_rows_as_topleave,
			Position EventX,
#endif /* NOVELL */
                        Position EventY) ;
#endif /* NeedWidePrototypes */
static void SelectRange( 
                        XmListWidget lw,
                        int first,
                        int last,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void RestoreRange( 
                        XmListWidget lw,
                        int first,
                        int last,
#if NeedWidePrototypes
                        int dostart) ;
#else
                        Boolean dostart) ;
#endif /* NeedWidePrototypes */
static void ArrangeRange( 
                        XmListWidget lw,
                        int item) ;
static void HandleNewItem( 
                        XmListWidget lw,
                        int item,
                        int olditem) ;
static void HandleExtendedItem( 
                        XmListWidget lw,
                        int item) ;
static void VerifyMotion( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void UnSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdUnSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdShiftSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdShiftUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCtrlSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCtrlUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCancel( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdToggleAddMode( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdSelectAll( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdDeSelectAll( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DefaultAction( 
                        XmListWidget lw,
                        XEvent *event) ;
static void ClickElement( 
                        XmListWidget lw,
                        XEvent *event,
#if NeedWidePrototypes
                        int default_action) ;
#else
                        Boolean default_action) ;
#endif /* NeedWidePrototypes */
static void ListFocusIn( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BrowseScroll( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static void ListLeave( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListEnter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
#ifdef NOVELL
static Boolean MakeItemVisible( 
                        XmListWidget lw,
                        int item,
			int delta_row) ;
static void MColumnNextElement(
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MColumnPrevElement(
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
#else /* NOVELL */
static void MakeItemVisible( 
                        XmListWidget lw,
                        int item) ;
#endif /* NOVELL */
static void PrevElement( 
                        XmListWidget lw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NextElement( 
                        XmListWidget lw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NormalNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ShiftNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendAddNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NormalPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ShiftPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendAddPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdPrevPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdNextPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdLeftChar( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdLeftPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BeginLine( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdRightChar( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdRightPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void EndLine( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TopItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void EndItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendTopItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendEndItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListItemVisible( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListCopyToClipboard( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DragDropFinished( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void ListProcessDrag( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Boolean ListConvert( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
static void APIAddItems( 
                        XmListWidget lw,
                        XmString *items,
                        int item_count,
                        int pos,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void CleanUpList( 
                        XmListWidget lw) ;
static void APIReplaceItems( 
                        Widget w,
                        XmString *old_items,
                        int item_count,
                        XmString *new_items,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void APIReplaceItemsPos( 
                        Widget w,
                        XmString *new_items,
                        int item_count,
                        int position,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void APISelect( 
                        XmListWidget lw,
                        int item_pos,
#if NeedWidePrototypes
                        int notify) ;
#else
                        Boolean notify) ;
#endif /* NeedWidePrototypes */
static void SetSelectionParams( 
                        XmListWidget lw) ;

#ifdef CDE_INTEGRATE
static Boolean XmTestInSelection(
        XmListWidget w,
	XEvent *event) ;
static void ProcessPress(
        Widget w,
        XEvent *event,
        char **params,
        Cardinal *num_params) ;
#endif /* CDE_INTEGRATE */

#ifdef CDE_FILESB
static void ScrollBarDisplayPolicyDefault( 
        Widget widget,
        int offset,
        XrmValue *value) ;
#endif /* CDE_FILESB */
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#ifndef NOVELL
static Arg vSBArgs[26];
static Arg hSBArgs[26];
#endif

/**************
 *
 *  Translation tables for List. These are used to drive the selections
 *
 **************/

#define ListXlations1	_XmList_ListXlations1
#define ListXlations2	_XmList_ListXlations2
#ifdef CDE_INTEGRATE
_XmConst char _XmList_ListXlations_CDE[] = "\
~c ~s ~m ~a <Btn1Down>:process-press(ListBeginSelect,ListProcessDrag)\n\
c ~s ~m ~a <Btn1Down>:process-press(ListBeginToggle,ListProcessDrag)\n\
<Btn1Motion>:ListButtonMotion()\n\
~c ~s ~m ~a <Btn1Up>:ListEndSelect()\n\
c ~s ~m ~a <Btn1Up>:ListEndToggle()\n\
c ~s ~m a <Btn1Down>:ListProcessDrag()\n\
~c s ~m a <Btn1Down>:ListProcessDrag()";
_XmConst char _XmList_ListXlations_CDEBtn2[] = "\
<Btn2Down>:ListBeginExtend()\n\
<Btn2Motion>:ListButtonMotion()\n\
<Btn2Up>:ListEndExtend()";
#endif /* CDE_INTEGRATE */


/****************
 *
 *  Actions Lists
 *
 ****************/


static XtActionsRec ListActions[] =
{
  {"ListButtonMotion",		VerifyMotion},
  {"ListShiftSelect",		ExSelect},
  {"ListShiftUnSelect",		ExUnSelect},
  {"ListBeginExtend",  		ExSelect},
  {"ListEndExtend",		ExUnSelect},
  {"ListCtrlSelect",  		CtrlSelect},
  {"ListCtrlUnSelect",		CtrlUnSelect},
  {"ListBeginToggle",  		CtrlSelect},
  {"ListEndToggle",		CtrlUnSelect},
  {"ListShiftCtrlSelect",	ExSelect},
  {"ListShiftCtrlUnSelect",	ExUnSelect},
  {"ListExtendAddSelect",	ExSelect},
  {"ListExtendAddUnSelect",	ExUnSelect},
  {"ListItemSelect",		SelectElement},
  {"ListItemUnSelect",		UnSelectElement},
  {"ListBeginSelect",		SelectElement},
  {"ListEndSelect",		UnSelectElement},
  {"ListKbdBeginSelect",	KbdSelectElement},
  {"ListKbdEndSelect",		KbdUnSelectElement},
  {"ListKbdShiftSelect",	KbdShiftSelect},
  {"ListKbdShiftUnSelect",	KbdShiftUnSelect},
  {"ListKbdCtrlSelect",		KbdCtrlSelect},
  {"ListKbdCtrlUnSelect",  	KbdCtrlUnSelect},
  {"ListKbdBeginExtend",      	KbdShiftSelect},
  {"ListKbdEndExtend",  	KbdShiftUnSelect},
  {"ListKbdBeginToggle",      	KbdCtrlSelect},
  {"ListKbdEndToggle",  	KbdCtrlUnSelect},
  {"ListKbdSelectAll",    	KbdSelectAll},
  {"ListKbdDeSelectAll",	KbdDeSelectAll},
  {"ListKbdActivate",      	KbdActivate},
  {"ListKbdCancel",		KbdCancel},
  {"ListAddMode",		KbdToggleAddMode},
#ifdef NOVELL
  {"ListPrevItem",              MColumnPrevElement},
  {"ListNextItem",              MColumnNextElement},
#else
  {"ListPrevItem",      	NormalPrevElement},
  {"ListNextItem",      	NormalNextElement},
#endif
  {"ListPrevPage",    		KbdPrevPage},
  {"ListNextPage",    		KbdNextPage},
  {"ListLeftChar",    		KbdLeftChar},
  {"ListLeftPage",    		KbdLeftPage},
  {"ListRightChar",  		KbdRightChar},
  {"ListRightPage",  		KbdRightPage},
  {"ListCtrlPrevItem",  	CtrlPrevElement},
  {"ListCtrlNextItem",  	CtrlNextElement},
  {"ListShiftPrevItem",  	ShiftPrevElement},
  {"ListShiftNextItem",  	ShiftNextElement},
  {"List_ShiftCtrlPrevItem",    ExtendAddPrevElement},
  {"List_ShiftCtrlNextItem",    ExtendAddNextElement},
  {"ListAddPrevItem", 		CtrlPrevElement},
  {"ListAddNextItem", 		CtrlNextElement},
  {"ListExtendPrevItem", 	ShiftPrevElement},
  {"ListExtendNextItem", 	ShiftNextElement},
  {"ListExtendAddPrevItem",  	ExtendAddPrevElement},
  {"ListExtendAddNextItem",  	ExtendAddNextElement},
  {"ListBeginLine",  		BeginLine},
  {"ListEndLine",		EndLine},
  {"ListBeginData",		TopItem},
  {"ListEndData",		EndItem},
  {"ListBeginDataExtend",	ExtendTopItem},
  {"ListEndDataExtend",		ExtendEndItem},
  {"ListFocusIn",		ListFocusIn},
  {"ListFocusOut",		ListFocusOut},
  {"ListEnter",			ListEnter},
  {"ListLeave",			ListLeave},
  {"ListScrollCursorVertically",ListItemVisible},
  {"ListScrollCursorVisible",	ListItemVisible},	/* name above is
							** correct; maintain
							** this one for
							** 1.2.0 compatibility
							*/
  {"ListCopyToClipboard",	ListCopyToClipboard},
  {"ListProcessDrag",           ListProcessDrag},
#ifdef CDE_INTEGRATE
  {"process-press",             ProcessPress},
#endif /* CDE_INTEGRATE */
};



#ifndef NOVELL
/************************************************************************
 *									*
 *   Callback Functions							*
 *   These are the callback routines for the scrollbar actions.		*
 *									*
 ************************************************************************/

static XtCallbackRec VSCallBack[] =
{
   {VertSliderMove, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};

static XtCallbackRec HSCallBack[] =
{
   {HorizSliderMove, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};

static XtCallbackRec VCCallBack[] =
{
   {UpdateHighlight, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};
#endif /* NOVELL */

/************************************************************************
 *									*
 *  VertSliderMove							*
 *  Callback for the sliderMoved resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
VertSliderMove( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
VertSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer call_data)
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *lcd = (XmScrollBarCallbackStruct *) call_data ;
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);

    lw->list.vOrigin = (int ) lcd->value;
    lw->list.top_position = (int ) lcd->value;
    DrawList(lw, NULL, TRUE);
#ifndef NOVELL
	/* DrawList already did it */
    if (lcd->reason != XmCR_DRAG)
        UpdateHighlight(w,closure,lcd);
#endif
}

/************************************************************************
 *									*
 *  HorizSliderMove							*
 *  Callback for the sliderMoved resource of the horizontal scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
HorizSliderMove( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
HorizSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *lcd = (XmScrollBarCallbackStruct *) call_data ;
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

#ifndef NOVELL
	/* no need to call because DrawList() will handle it! */
    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
#endif /* NOVELL */
    lw->list.hOrigin = (int ) lcd->value;
    lw->list.XOrigin= (int ) lcd->value;
    DrawList(lw, NULL, TRUE);

}

/************************************************************************
 *									*
 *  UpdateHighlight							*
 *  Callback for the ValueChanged resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
UpdateHighlight( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
UpdateHighlight(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
    {
/****************
 *
 * This all goes away now...
 * 
        if (lcd->reason == XmCR_PAGE_DECREMENT)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

        if ((lw->list.CurrentKbdItem < lw->list.top_position) ||
            (lw->list.CurrentKbdItem >=
             lw->list.top_position+lw->list.visibleItemCount) |
            (lcd->reason == XmCR_PAGE_DECREMENT))
        {
            if (lcd->reason == XmCR_DECREMENT)
                lw->list.CurrentKbdItem = 
                   (lw->list.top_position+lw->list.visibleItemCount - 1);
            else
                lw->list.CurrentKbdItem = lw->list.top_position;                
                     DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
        }
 ****************/
	DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    }

}


/************************************************************************
 *									*
 * XmList Resources.							*
 * 									*
 ************************************************************************/
static XtResource resources[] =
{
#ifdef NOVELL
    {
        XmNstaticRowCount, XmCStaticRowCount, XmRShort,
	sizeof(short), XtOffsetOf( struct _XmListRec,list.static_rows),
	XmRImmediate, (XtPointer)0
    },
    {
        XmNitemInitCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec,list.item_init_cb),
	XmRCallback, (XtPointer)NULL
    },
    {		/* syn_resource? */
        XmNlistColumnSpacing, XmCListSpacing, XmRHorizontalDimension,
	sizeof(Dimension), XtOffsetOf( struct _XmListRec,list.col_spacing),
	XmRImmediate, (XtPointer)0
    },
    {
        XmNnumColumns, XmCNumColumns, XmRShort, sizeof(short),
        XtOffsetOf( struct _XmListRec,list.cols),
	XmRImmediate, (XtPointer)1
    },
#endif /* NOVELL */
    {
        XmNlistSpacing, XmCListSpacing, XmRVerticalDimension, sizeof(Dimension),
        XtOffsetOf( struct _XmListRec, list.ItemSpacing), XmRImmediate, (XtPointer) 0
    },
    {
        XmNlistMarginWidth, XmCListMarginWidth, XmRHorizontalDimension,
        sizeof (Dimension), XtOffsetOf( struct _XmListRec, list.margin_width),
        XmRImmediate, (XtPointer) 0
    },
    {
        XmNlistMarginHeight, XmCListMarginHeight, XmRVerticalDimension,
        sizeof (Dimension), XtOffsetOf( struct _XmListRec, list.margin_height),
        XmRImmediate, (XtPointer) 0
    },
    {
        XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
        XtOffsetOf( struct _XmListRec, list.font), XmRImmediate, NULL
    },
    {
        XmNstringDirection, XmCStringDirection, XmRStringDirection,
        sizeof(XmStringDirection), XtOffsetOf( struct _XmListRec, list.StrDir),
        XmRImmediate, (XtPointer) XmSTRING_DIRECTION_DEFAULT
    },
    {
        XmNitems, XmCItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffsetOf( struct _XmListRec,list.items), XmRStringTable, NULL
    },
    {
        XmNitemCount, XmCItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.itemCount), XmRImmediate, (XtPointer) 0
    },
    {
        XmNselectedItems, XmCSelectedItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffsetOf( struct _XmListRec,list.selectedItems), XmRStringTable, NULL
    },
    {
        XmNselectedItemCount, XmCSelectedItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.selectedItemCount), XmRImmediate, (XtPointer) 0
    },
    {
        XmNvisibleItemCount, XmCVisibleItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.visibleItemCount), XmRImmediate,(XtPointer) 0
    },
    {
        XmNtopItemPosition, XmCTopItemPosition, XmRTopItemPosition, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.top_position), XmRImmediate,(XtPointer) 0
    },
    {
        XmNselectionPolicy, XmCSelectionPolicy, XmRSelectionPolicy,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmListRec,list.SelectionPolicy), XmRImmediate,
        (XtPointer) XmBROWSE_SELECT
    },
    {
        XmNlistSizePolicy, XmCListSizePolicy, XmRListSizePolicy,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmListRec,list.SizePolicy), XmRImmediate,
        (XtPointer) XmVARIABLE
    },
    {
        XmNscrollBarDisplayPolicy, XmCScrollBarDisplayPolicy,
	XmRScrollBarDisplayPolicy, sizeof (unsigned char),
        XtOffsetOf( struct _XmListRec, list.ScrollBarDisplayPolicy),
#ifdef CDE_FILESB
        XmRCallProc, (XtPointer) ScrollBarDisplayPolicyDefault
#else
        XmRImmediate,  (XtPointer) XmAS_NEEDED
#endif
    },
    {
        XmNautomaticSelection, XmCAutomaticSelection, XmRBoolean,
        sizeof(Boolean), XtOffsetOf( struct _XmListRec,list.AutoSelect),
	XmRImmediate, (XtPointer) FALSE
    },

    {
        XmNdoubleClickInterval, XmCDoubleClickInterval, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.ClickInterval), XmRImmediate,
        (XtPointer) (-1)
    },
    {
        XmNsingleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.SingleCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNmultipleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.MultipleCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNextendedSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.ExtendCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNbrowseSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.BrowseCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNdefaultActionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.DefaultCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNhorizontalScrollBar, XmCHorizontalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmListRec, list.hScrollBar),XmRImmediate, NULL
    },
    {
        XmNverticalScrollBar, XmCVerticalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmListRec, list.vScrollBar),XmRImmediate, NULL
    },
    {
        XmNnavigationType, XmCNavigationType, XmRNavigationType, 
        sizeof (unsigned char), XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
        XmRImmediate, (XtPointer) XmTAB_GROUP
    }
};

/****************
 *
 * Resolution independent resources
 *
 ****************/

static XmSyntheticResource get_resources[] =
{
   { XmNlistSpacing,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.ItemSpacing),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNlistMarginWidth,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.margin_width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNlistMarginHeight,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNtopItemPosition,
     sizeof (int),
     XtOffsetOf( struct _XmListRec, list.top_position),
     CvtToExternalPos,
     CvtToInternalPos },
};


/************************************************************************
 *									*
 * 	              Class record for XmList class			*
 *									*
 ************************************************************************/

static XmBaseClassExtRec BaseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,				/* InitializePrehook	*/
    NULL,				/* SetValuesPrehook	*/
    NULL,				/* InitializePosthook	*/
    NULL,				/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    NULL,				/* secondaryCreate	*/
    NULL,		                /* getSecRes data	*/
    { 0 },				/* fastSubclass flags	*/
    NULL,				/* get_values_prehook	*/
    NULL,				/* get_values_posthook	*/
    NULL,                               /* classPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    XmInheritWidgetNavigable,           /* widgetNavigable      */
    XmInheritFocusChange,               /* focusChange          */
};

externaldef(xmlistclassrec) XmListClassRec xmListClassRec =
{
    {
	(WidgetClass) &xmPrimitiveClassRec,    	/* superclass	      */
	"XmList",    				/* class_name	      */
	sizeof(XmListRec),    			/* widget_size	      */
    	ClassInitialize,                        /* class_initialize   */
    	ClassPartInitialize, 			/* class part init    */
	FALSE,    				/* class_inited       */
	Initialize,    				/* initialize	      */
	NULL,					/* widget init hook   */
	XtInheritRealize,		    	/* realize	      */
	ListActions,			    	/* actions	      */
	XtNumber(ListActions),		    	/* num_actions	      */
	resources,			    	/* resources	      */
	XtNumber(resources),		    	/* num_resources      */
	NULLQUARK,			    	/* xrm_class	      */
	FALSE,				    	/* compress_motion    */
	XtExposeCompressMaximal,	    	/* compress_exposure  */
	TRUE,				    	/* compress enter/exit*/
	FALSE,				    	/* visible_interest   */
	Destroy,			    	/* destroy	      */
	Resize,				    	/* resize	      */
	Redisplay,			    	/* expose	      */
	SetValues,			    	/* set values 	      */
	NULL,				    	/* set values hook    */
	ListSetValuesAlmost,		        /* set values almost  */
	NULL,					/* get_values hook    */
	NULL,				    	/* accept_focus	      */
	XtVersion,				/* version	      */
        NULL,				        /* callback offset    */
	NULL,				        /* default trans      */
	QueryProc,              	    	/* query geo proc     */
	NULL,				    	/* disp accelerator   */
        (XtPointer) &BaseClassExtRec,           /* extension          */
    },
   {
         NullRoutine,  				/* Primitive border_highlight   */
         NullRoutine,  				/* Primitive border_unhighlight */
         NULL,   				/* translations                 */
         NULL,         				/* arm_and_activate             */
         get_resources,				/* get resources                */
         XtNumber(get_resources),   		/* num get_resources            */
         NULL,         				/* extension                    */

   },
   {
         (XtPointer) NULL,			/* extension		*/
   }
};

externaldef(xmlistwidgetclass) WidgetClass xmListWidgetClass = 
                               (WidgetClass)&xmListClassRec;


static void 
#ifdef _NO_PROTO
NullRoutine( wid)
        Widget wid ;
#else
NullRoutine(
        Widget wid )
#endif /* _NO_PROTO */
{
}
/************************************************************************
 *									*
 *                      Widget Instance Functions			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  BaseClassExtRec.record_type = XmQmotif ;
}

/************************************************************************
 *									*
 *  ClassPartInitialize - Set up the fast subclassing.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    char *xlats;
   _XmFastSubclassInit (wc, XmLIST_BIT);
   xlats = 
     (char *)ALLOCATE_LOCAL(strlen(ListXlations1) + strlen(ListXlations2) + 1);
   strcpy(xlats, ListXlations1);
   strcat(xlats, ListXlations2);
   wc->core_class.tm_table =(String ) XtParseTranslationTable(xlats);
   DEALLOCATE_LOCAL((char *)xlats);
}

/************************************************************************
 *									*
 * Initialize - initialize the instance.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( request, w, args, num_args )
        Widget request ;
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget request,
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define LW	(XmListWidget)w

	Arg		Args[24];
	Dimension	width, height;
	XtCallbackRec	mv_cb[2], vc_cb[2];
	register int	i;
	int		rows, last_row_size;

/* BEGIN OSF FIX CR 6014 */
	LPART(w).LastSetVizCount=
/* END OSF FIX CR 6014 */
	LPART(w).MaxItemHeight	=
	LPART(w).LeaveDir	=
	LPART(w).hOrigin	=
	LPART(w).vOrigin	=
	LPART(w).hExtent	=
	LPART(w).hmax		=
#ifndef OSF_v1_2_4
	LPART(w).Event		=
#endif /* OSF_v1_2_4 */
	LPART(w).DragID		=
	LPART(w).LastItem	=
	LPART(w).Event		=
	LPART(w).LastHLItem	=
	LPART(w).StartItem	=
	LPART(w).EndItem	=
	LPART(w).OldStartItem	=
	LPART(w).OldEndItem	=
	LPART(w).DownCount	=
	LPART(w).DownTime	=
	LPART(w).XOrigin	= 0;

	LPART(w).InternalList	 = NULL;
	LPART(w).selectedIndices = NULL;

	LPART(w).NormalGC	=
	LPART(w).InverseGC	=
	LPART(w).HighlightGC	=
	LPART(w).InsensitiveGC	= NULL;

	LPART(w).MouseMoved	=
	LPART(w).AppendInProgress =
	LPART(w).FromSetSB	=
	LPART(w).FromSetNewSize	=
	LPART(w).Traversing	=
	LPART(w).KbdSelection	= False;

	if (LPART(w).cols < 1)
		LPART(w).cols = 1;

	LPART(w).CurrentKbdItem = FIRST_ITEM(w);

	LPART(w).max_col_width = (LPART(w).cols == 1) ?
				 &LPART(w).MaxWidth :
				 (Dimension *)XtMalloc(sizeof(Dimension) *
							LPART(w).cols);
	for (i = 0; i < (int)LPART(w).cols; i++)
		LPART(w).max_col_width[i] = 0;


	if (LPART(w).ItemSpacing < 0)
	{
		_XmWarning(w, ListMessage11);
		LPART(w).ItemSpacing = 0;
	}

/* BEGIN OSF Fix CR 5740 */
	if (LPART(w).top_position < -1)
/* END OSF Fix CR 5740 */
	{
		_XmWarning(w, ListMessage15);
		LPART(w).top_position = 0;
	}

#ifdef CDE_INTEGRATE
    {
    Boolean btn1_transfer;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableBtn1Transfer", &btn1_transfer, NULL);
    if (btn1_transfer) /* for btn2 extend and transfer cases */
        XtOverrideTranslations(w, XtParseTranslationTable(_XmList_ListXlations_CDE));
    if (btn1_transfer == True) /* for btn2 extend case */
        XtOverrideTranslations(w, XtParseTranslationTable(_XmList_ListXlations_CDEBtn2));
    }
#endif /* CDE_INTEGRATE */

	CalcNumRows(w, &rows, &last_row_size);
	if (LPART(w).top_position >= rows)
		LPART(w).top_position = 0;

	if (LPART(w).ClickInterval < 0)
		LPART(w).ClickInterval = XtGetMultiClickTime(XtDisplay(w));

	if (PPART(w).highlight_thickness)
		LPART(w).HighlightThickness = PPART(w).highlight_thickness + 1;
	else
		LPART(w).HighlightThickness = 0;

	LPART(w).BaseX = LPART(w).HighlightThickness +
			 PPART(w).shadow_thickness +
			 (Position)LPART(w).margin_width;

	LPART(w).BaseY = LPART(w).HighlightThickness +
			 PPART(w).shadow_thickness +
			 (Position)LPART(w).margin_height;

	if (LPART(w).StrDir == XmSTRING_DIRECTION_DEFAULT)
	{
		if (!XmIsManager(XtParent(w)))
			LPART(w).StrDir = XmSTRING_DIRECTION_L_TO_R;
		else
		{
			XtSetArg(Args[0], XmNstringDirection, &LPART(w).StrDir);
			XtGetValues(XtParent(w), Args, 1);
		}
	}

	ValidateRepValues(
		w,
		(unsigned char)XmBROWSE_SELECT,	/* fb for SelectionPolicy */
		(unsigned char)XmAS_NEEDED,  /* fb for ScrollBarDisplayPolicy */
		(unsigned char)XmVARIABLE,	/* fb for SizePolicy */
		XmSTRING_DIRECTION_L_TO_R);	/* fb for StrDir */

	if (!LPART(w).font)
		LPART(w).font = _XmGetDefaultFontList (w, XmTEXT_FONTLIST);
	LPART(w).font = XmFontListCopy(LPART(w).font);

	if (LPART(w).SelectionPolicy == XmMULTIPLE_SELECT ||
	    LPART(w).SelectionPolicy == XmSINGLE_SELECT)
		LPART(w).AddMode = True;
	else
		LPART(w).AddMode = False;

	MakeGC(LW);
	MakeHighlightGC(LW, LPART(w).AddMode);

	LPART(w).spacing = LPART(w).ItemSpacing + LPART(w).HighlightThickness;
/****************
 *
 * Copy the font, item and selected item lists into our space. THE USER IS
 * RESPONSIBLE FOR FREEING THE ORIGINAL LISTS!
 *
 ****************/
	if (LPART(w).itemCount < 0)
		LPART(w).itemCount = 0;

	if (LPART(w).selectedItemCount < 0)
		LPART(w).selectedItemCount = 0;

	if ((LPART(w).itemCount && !LPART(w).items) ||
	    (!LPART(w).itemCount && LPART(w).items))
	{
		_XmWarning(w, ListMessage16);
		LPART(w).itemCount = 0;
		LPART(w).items = NULL;
	}

	if ((LPART(w).selectedItemCount && !LPART(w).selectedItems) ||
            (!LPART(w).selectedItemCount && LPART(w).selectedItems))
	{
		LPART(w).selectedItemCount = 0;
		LPART(w).selectedItems = NULL;
	}

/* BEGIN OSF Fix CR 5740 */
	if (LPART(w).top_position == -1)
		LPART(w).top_position = rows ? rows - 1 : 0;
/* END OSF Fix CR 5740 */

	CopySelectedItems(LW);

/****************
 *
 * If we have items, add them to the internal list and calculate our default
 * size.
 *
 ****************/

	if (LPART(w).itemCount)
	{
		CreateInternalList(w);

/* BEGIN OSF Fix CR 5560 */
			/* Before building selected list, clear selected
			 * list to avoid  memory leak.  */
		ClearSelectedList(LW);
/* END OSF Fix CR 5560 */

		BuildSelectedList(LW, True);
	}

/* BEGIN OSF Fix pir 2730 */
		/* Save from AddInternalElement/CreateInternalList */
	LPART(w).visibleItemCount = LPART(request).visibleItemCount;

			/* Assume that the user didn't set it.*/
	if (!LPART(w).visibleItemCount)
		LPART(w).visibleItemCount = SetVizCount(LW);
			/* Must have at least one visible. */
	else if (LPART(w).visibleItemCount < 0)
	{
		_XmWarning(w, ListMessage0);
		LPART(w).visibleItemCount = 1;
	}
	else /* Otherwise leave it to whatever it was set. */
		LPART(w).LastSetVizCount = LPART(w).visibleItemCount;
/* BEGIN OSF Fix CR 6014 */
/* END OSF Fix CR 6014 */
/* END OSF Fix pir 2730 */

	SetDefaultSize(LW, &width, &height);
	SetSelectionParams(LW);

	if (!CPART(request).width)
		CPART(w).width = width;

	if (!CPART(request).height) 
		CPART(w).height = height;
	else		/* We got a height - make sure viz tracks */
		LPART(w).visibleItemCount = SetVizCount(LW);
    

/****************
 *
 * OK, we're all set for the list stuff. Now look at our parent and see
 * if it's a ScrolledWindow subclass. If it is, create the scrollbars and set up
 * all the scrolling stuff.
 *
 * NOTE: THIS DOES NOT LET THE LIST BE SMART IN A AUTOMATIC SCROLLED
 *       WINDOW.
 ****************/

	if (!XmIsScrolledWindow(XtParent(w)) ||
	    ((XmScrolledWindowWidget)XtParent(w))->swindow.VisualPolicy ==
								    XmCONSTANT)
	{
		LPART(w).Mom = NULL;
		return;
	}

		/* [h|v]ScrollBar are resources, what if an app specifies
		 * them! the code didn't take care that, should we worry
		 * about it? */

	LPART(w).Mom = (XmScrolledWindowWidget)XtParent(w);

	mv_cb[0].closure =
	mv_cb[1].closure =
	vc_cb[0].closure =
	vc_cb[1].closure = (XtPointer)NULL;

	mv_cb[1].callback =
	vc_cb[1].callback = (XtCallbackProc)NULL;

		/* Define the common part for both VSB and HSB */
	XtSetArg(Args[0], XmNshadowThickness, PPART(w).shadow_thickness);
	XtSetArg(Args[1], XmNhighlightThickness, 0);
	XtSetArg(Args[2], XmNunitType, XmPIXELS);
	XtSetArg(Args[3], XmNtraversalOn, False);

#define SET_ORI_CB(ORI,MV_CB,VC_CB)\
	mv_cb[0].callback = (XtCallbackProc)MV_CB;\
	vc_cb[0].callback = (XtCallbackProc)VC_CB;\
	XtSetArg(Args[4], XmNorientation, ORI);\
	XtSetArg(Args[5], XmNincrementCallback, mv_cb);\
	XtSetArg(Args[6], XmNdecrementCallback, mv_cb);\
	XtSetArg(Args[7], XmNpageIncrementCallback, mv_cb);\
	XtSetArg(Args[8], XmNpageDecrementCallback, mv_cb);\
	XtSetArg(Args[9], XmNtoTopCallback, mv_cb);\
	XtSetArg(Args[10], XmNtoBottomCallback, mv_cb);\
	XtSetArg(Args[11], XmNdragCallback, mv_cb)

	SET_ORI_CB(XmVERTICAL, VertSliderMove, UpdateHighlight);
	XtSetArg(Args[12], XmNvalueChangedCallback, vc_cb);

	LPART(w).vScrollBar = (XmScrollBarWidget)XtCreateWidget(
							"VertScrollBar",
							xmScrollBarWidgetClass,
							(Widget)LPART(w).Mom,
							Args, 13);

		/* This is cheating... SIGH! */
	PPART(LPART(w).vScrollBar).unit_type = PPART(w).unit_type;
	SetVerticalScrollbar(LW);

/****************
 *
 * Only create the horizontal sb if in static size mode.
 *
 ****************/
	if (LPART(w).SizePolicy != XmVARIABLE || LPART(w).cols != 1)
	{
			/* Initialize all necessary fields first */
		LPART(w).hmin = 0;
		LPART(w).hmax = LPART(w).MaxWidth + LPART(w).BaseX * 2;
				/* Don't know why two fields */
		LPART(w).hOrigin = LPART(w).XOrigin;
		LPART(w).hExtent = CPART(w).width ;
		if (LPART(w).hExtent + LPART(w).hOrigin > LPART(w).hmax)
			LPART(w).hExtent = LPART(w).hmax - LPART(w).hOrigin;

		/* See above for 0-3! */

			/* Set 4-11 thru SET_ORI_CB() marco */
		SET_ORI_CB(XmHORIZONTAL, HorizSliderMove, NULL);

#undef SET_ORI_CB

			/* The rest starts with 18 */
		XtSetArg(Args[12], XmNminimum, LPART(w).hmin);
		XtSetArg(Args[13], XmNmaximum, LPART(w).hmax);
		XtSetArg(Args[14], XmNvalue, LPART(w).hOrigin);
		XtSetArg(Args[15], XmNsliderSize, LPART(w).hExtent);

/****************
 *
 * What do I set the inc to when I have no idea ofthe fonts??
 *
 ****************/
					/* What a hack! */
		XtSetArg(Args[16], XmNincrement, CHAR_WIDTH_GUESS);
		XtSetArg(Args[17], XmNpageIncrement, CPART(w).width);
	
		LPART(w).hScrollBar = (XmScrollBarWidget)XtCreateWidget(
							"HorScrollBar",
							xmScrollBarWidgetClass,
							(Widget)LPART(w).Mom,
							Args, 18);

			/* This is cheating... SIGH! */
		PPART(LPART(w).hScrollBar).unit_type = PPART(w).unit_type;
		SetHorizontalScrollbar(LW);
	}

	XmScrolledWindowSetAreas(
		(Widget)LPART(w).Mom,
		(Widget)LPART(w).hScrollBar,
		(Widget)LPART(w).vScrollBar,
		w);
#undef LW

#else /* NOVELL */
    register XmListWidget lw = (XmListWidget) w;
    Dimension	 width, height;

     int		 i, j;

    lw->list.LastItem = 0;
    lw->list.Event = 0;
    lw->list.LastHLItem = 0;
    lw->list.StartItem = 0;
    lw->list.EndItem = 0;
    lw->list.OldStartItem = 0;
    lw->list.OldEndItem = 0;
    lw->list.DownCount = 0;
    lw->list.DownTime = 0;
    lw->list.NormalGC = NULL;
    lw->list.InverseGC = NULL;
    lw->list.HighlightGC = NULL;
    lw->list.InsensitiveGC = NULL;
    lw->list.XOrigin = 0;
    lw->list.Traversing = FALSE;
    lw->list.KbdSelection = FALSE;
    lw->list.AddMode = FALSE;
    lw->list.CurrentKbdItem = 0;
    lw->list.AppendInProgress = FALSE;
    lw->list.FromSetSB = FALSE;
    lw->list.FromSetNewSize = FALSE;
#ifndef OSF_v1_2_4
    lw->list.Event = 0;
#endif /* OSF_v1_2_4 */
    lw->list.DragID = 0;
    lw->list.selectedIndices = NULL;
    lw->list.MaxItemHeight = 0;
    lw->list.LeaveDir = 0;
    lw->list.hOrigin = lw->list.vOrigin = 0;
    lw->list.hExtent = lw->list.hmax = 0;
/* BEGIN OSF Fix CR 6014 */
    lw->list.LastSetVizCount = 0;
/* END OSF Fix CR 6014 */

    if (lw->list.ItemSpacing < 0)
    {
        lw->list.ItemSpacing = 0;
        _XmWarning( (Widget) lw, ListMessage11);
    }
/* BEGIN OSF Fix CR 5740 */
    if (lw->list.top_position < -1)
/* END OSF Fix CR 5740 */
    {
        lw->list.top_position = 0;
        _XmWarning( (Widget) lw, ListMessage15);
    }

#ifdef CDE_INTEGRATE
    {
    Boolean btn1_transfer;
    XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(w)), "enableBtn1Transfer", &btn1_transfer, NULL);
    if (btn1_transfer) /* for btn2 extend and transfer cases */
        XtOverrideTranslations(w, XtParseTranslationTable(_XmList_ListXlations_CDE));
    if (btn1_transfer == True) /* for btn2 extend case */
        XtOverrideTranslations(w, XtParseTranslationTable(_XmList_ListXlations_CDEBtn2));
    }
#endif /* CDE_INTEGRATE */

    if (lw->list.ClickInterval < 0)
        lw->list.ClickInterval = XtGetMultiClickTime(XtDisplay(lw));
    if (lw->primitive.highlight_thickness)
        lw->list.HighlightThickness = lw->primitive.highlight_thickness + 1;
    else
        lw->list.HighlightThickness = 0;

    lw->list.BaseX = (Position )lw->list.margin_width +
    			   lw->list.HighlightThickness +
	            	   lw->primitive.shadow_thickness;

    lw->list.BaseY = (Position )lw->list.margin_height +
    			   lw->list.HighlightThickness +
			   lw->primitive.shadow_thickness;

    lw->list.MouseMoved = FALSE;
    lw->list.InternalList = NULL;
 /*  BEGIN OSF Fix pir 2730 */
 /* END OSF Fix pir 2730 */ 

    if(    !XmRepTypeValidValue( XmRID_SELECTION_POLICY,
                                    lw->list.SelectionPolicy, (Widget) lw)    )
    {
        lw->list.SelectionPolicy = XmBROWSE_SELECT;
    }
    if(    !XmRepTypeValidValue( XmRID_LIST_SIZE_POLICY, lw->list.SizePolicy,
                                                              (Widget) lw)    )
    {
        lw->list.SizePolicy = XmVARIABLE;
    }

    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                             lw->list.ScrollBarDisplayPolicy, (Widget) lw)    )
    {
        lw->list.ScrollBarDisplayPolicy = XmAS_NEEDED;
    }
    if (lw->list.StrDir == XmSTRING_DIRECTION_DEFAULT)
    {
        if (XmIsManager(lw->core.parent))
        {
            XtSetArg (vSBArgs[0], XmNstringDirection, &lw->list.StrDir);
            XtGetValues(lw->core.parent, vSBArgs, 1);
        }
        else
            lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
    }
    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION, lw->list.StrDir,
                                                              (Widget) lw)    )
    {
        lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
    }
    if (lw->list.font == NULL)
        lw->list.font = _XmGetDefaultFontList ( (Widget) lw,XmTEXT_FONTLIST);
    lw->list.font = XmFontListCopy(lw->list.font);

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        lw->list.AddMode = TRUE;

    MakeGC(lw);
    MakeHighlightGC(lw, lw->list.AddMode);
    lw->list.spacing = lw->list.ItemSpacing + lw->list.HighlightThickness;
/****************
 *
 * Copy the font, item and selected item lists into our space. THE USER IS
 * RESPONSIBLE FOR FREEING THE ORIGINAL LISTS!
 *
 ****************/
    if (lw->list.itemCount < 0) lw->list.itemCount = 0;
    if (lw->list.selectedItemCount < 0) lw->list.selectedItemCount = 0;

    if ((lw->list.itemCount && !lw->list.items) ||
        (!lw->list.itemCount && lw->list.items))
    {
        _XmWarning( (Widget) lw, ListMessage16);
    }
/* BEGIN OSF Fix CR 5740 */
    if (lw->list.top_position == -1)
      lw->list.top_position = lw->list.itemCount ? lw->list.itemCount - 1 : 0;
/* END OSF Fix CR 5740 */
    CopyItems(lw);
    CopySelectedItems(lw);

/****************
 *
 * If we have items, add them to the internal list and calculate our default
 * size.
 *
 ****************/

    if (lw->list.items && (lw->list.itemCount > 0))
    {
        lw->list.InternalList = (ElementPtr *)XtMalloc((sizeof(Element *) * lw->list.itemCount));
        for (i = 0; i < lw->list.itemCount; i++)
	    AddInternalElement(lw, lw->list.items[i], 0,
	    				OnSelectedList(lw,lw->list.items[i]),
					FALSE);
/* BEGIN OSF Fix CR 5560 */
	/* Before building selected list, clear selected list to avoid  memory leak.  */
	ClearSelectedList(lw);
/* END OSF Fix CR 5560 */
	BuildSelectedList(lw, TRUE);
    }
/* BEGIN OSF Fix pir 2730 */
    /* Must have at least one visible. */
    lw->list.visibleItemCount = ((XmListWidget)request)->list.visibleItemCount; /* Save from AddInternalElement */
    if (lw->list.visibleItemCount < 0)
    {
	lw->list.visibleItemCount = 1;
	_XmWarning((Widget )lw, ListMessage0);
    }
    /* Assume that the user didn't set it.*/
    else if (lw->list.visibleItemCount == 0)
      	lw->list.visibleItemCount = SetVizCount(lw);
    /* Otherwise leave it to whatever it was set. */
    else lw->list.LastSetVizCount = lw->list.visibleItemCount;
/* BEGIN OSF Fix CR 6014 */
/* END OSF Fix CR 6014 */
/* END OSF Fix pir 2730 */

    SetDefaultSize(lw,&width, &height);
    SetSelectionParams(lw);

    if (!request->core.width) lw->core.width = width;
    if (!request->core.height) 
        lw->core.height = height;
    else			/* We got a height - make sure viz tracks */
       	lw->list.visibleItemCount = SetVizCount(lw);
    

/****************
 *
 * OK, we're all set for the list stuff. Now look at our parent and see
 * if it's a ScrolledWindow subclass. If it is, create the scrollbars and set up
 * all the scrolling stuff.
 *
 * NOTE: THIS DOES NOT LET THE LIST BE SMART IN A AUTOMATIC SCROLLED
 *       WINDOW.
 ****************/

    if (! (XmIsScrolledWindow(lw->core.parent)) ||
        ( (XmIsScrolledWindow(lw->core.parent)) &&
         (((XmScrolledWindowWidget)lw->core.parent)->swindow.VisualPolicy ==
                                    XmCONSTANT)))
    {
	lw->list.Mom = NULL;
	return;
    }
    lw->list.Mom = (XmScrolledWindowWidget) lw->core.parent;
    i = j = 0;
    XtSetArg (vSBArgs[i], XmNorientation,(XtArgVal) (XmVERTICAL)); i++;
    XtSetArg (vSBArgs[i], XmNunitType, XmPIXELS); i++;
    XtSetArg (vSBArgs[i], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); i++;
    XtSetArg (vSBArgs[i], XmNhighlightThickness, (XtArgVal) 0); i++;

    XtSetArg(vSBArgs[i], XmNincrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageIncrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageDecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoTopCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoBottomCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdragCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNvalueChangedCallback, (XtArgVal) VCCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtraversalOn, (XtArgVal) FALSE); i++;

    lw->list.vScrollBar = (XmScrollBarWidget) XtCreateWidget(
    						   "VertScrollBar",
					           xmScrollBarWidgetClass,
					   	   (Widget) lw->list.Mom,
						   vSBArgs, i);
    (lw->list.vScrollBar)->primitive.unit_type = lw->primitive.unit_type;
    SetVerticalScrollbar(lw);

/****************
 *
 * Only create the horizontal sb if in static size mode.
 *
 ****************/
    if (lw->list.SizePolicy != XmVARIABLE)
    {
	XtSetArg (hSBArgs[j], XmNorientation,(XtArgVal) (XmHORIZONTAL)); j++;
        XtSetArg (hSBArgs[j], XmNunitType, XmPIXELS); j++;
	XtSetArg (hSBArgs[j], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); j++;
        XtSetArg (hSBArgs[j], XmNhighlightThickness, (XtArgVal) 0); j++;

	lw -> list.hmin = 0;
	XtSetArg (hSBArgs[j], XmNminimum, (XtArgVal) (lw->list.hmin)); j++;

	lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
	XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;

	lw -> list.hOrigin = lw->list.XOrigin;
	XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;

        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
	    lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
	XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What do I set the inc to when I have no idea ofthe fonts??
 *
 ****************/
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) (lw->core.width)); j++;

	XtSetArg(hSBArgs[j], XmNincrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdecrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageIncrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageDecrementCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoTopCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoBottomCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdragCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNtraversalOn, (XtArgVal) FALSE); j++;
	
	lw->list.hScrollBar = (XmScrollBarWidget) XtCreateWidget(
						  "HorScrollBar",
						  xmScrollBarWidgetClass,
					          (Widget) lw->list.Mom,
						  hSBArgs, j);
        (lw->list.hScrollBar)->primitive.unit_type = lw->primitive.unit_type;
        SetHorizontalScrollbar(lw);
    }
    XmScrolledWindowSetAreas( (Widget) lw->list.Mom, (Widget) lw->list.hScrollBar, (Widget) lw->list.vScrollBar, (Widget) lw);
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * ReDisplay - draw the visible list items.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Redisplay( wid, event, region )
        Widget wid ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget wid,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	/* The following Marco replaces DrawListShadow(), this function
	 * only calls _XmDrawShadows(), it doesn't make sense to have it
	 * defined as a function, currently, only Redisplay() uses it */
#define DRAW_LIST_SHADOW(W)\
		_XmDrawShadows(XtDisplay(W), XtWindow(W),\
 		     PPART(W).bottom_shadow_GC, PPART(W).top_shadow_GC,\
 		     0, 0, (int)CPART(W).width, (int)CPART(W).height,\
 		     PPART(W).shadow_thickness, XmSHADOW_OUT)

    DRAW_LIST_SHADOW(wid);
    SetClipRect((XmListWidget)wid);
    DrawList((XmListWidget)wid, event, True);

	/* I don't understand why #ifdef OSF_v1_2_4 block below, DrawList()
	 * should already take care the highlight, at least in our case... */

#undef DRAW_LIST_SHADOW
#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    DrawListShadow(lw);
    SetClipRect(lw);
    DrawList(lw, event, TRUE);
#ifdef OSF_v1_2_4

    /* CR 6529: Redraw the highlight too. */
    if (lw->list.Traversing)
      DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
#endif /* OSF_v1_2_4 */
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * Resize - redraw the list in response to orders from above.		*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define LW		(XmListWidget)wid

	int		borders, list_wd, top, rows, ignore;
	register int	viz;

		/* don't allow underflow! */
	borders = 2 * (LPART(wid).margin_width +
			LPART(wid).HighlightThickness +
			PPART(wid).shadow_thickness);

	if ((int)CPART(wid).width <= borders)
		return; /* why setting it to listwidth = 1, just return */

	list_wd = CPART(wid).width - borders;

/****************
 *
 * The current strategy here is to assume that the user initiated the
 * resize request on me, or on my parent. As such, we will calculate a
 * new visible item count, even though it may confuse the thing that
 * set the visible count in the first place.
 * Oh, well.
 *
 ****************/

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
	top = LPART(wid).top_position;
	viz = SetVizCount(LW);
	CalcNumRows(wid, &rows, &ignore);

	if (rows - top < viz)
	{
		if ((top = rows - viz) < 0)
			top = 0;
		LPART(wid).top_position = top;
	}
	LPART(wid).visibleItemCount = viz;
        SetVerticalScrollbar(LW);

        if (LPART(wid).SizePolicy != XmVARIABLE || LPART(wid).cols != 1)
        {
		if (LPART(wid).StrDir == XmSTRING_DIRECTION_R_TO_L)
		{
			if (list_wd + (int)LPART(wid).XOrigin >
						(int)LPART(wid).MaxWidth)
				LPART(wid).XOrigin = list_wd -
						     (int)LPART(wid).MaxWidth;
		}
		else /* XmSTRING_DIRECTION_L_TO_R */
		{
			if ((int)LPART(wid).MaxWidth - (int)LPART(wid).XOrigin <
								list_wd)
				LPART(wid).XOrigin = (int)LPART(wid).MaxWidth -
						     list_wd;
		}

		if (LPART(wid).XOrigin < 0)
			LPART(wid).XOrigin = 0;

		SetHorizontalScrollbar(LW);
        }

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
	if (XtIsRealized(wid))
		SetClipRect(LW);

#undef LW

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int	listwidth, top;
    register int viz;

    /* don't allow underflow! */
    int borders;
    borders = 2 * (lw->list.margin_width +
		   lw->list.HighlightThickness +
		   lw->primitive.shadow_thickness);
    if (lw->core.width <= borders)
	listwidth = 1;
    else
	listwidth = lw->core.width - borders;

/****************
 *
 * The current strategy here is to assume that the user initiated the
 * resize request on me, or on my parent. As such, we will calculate a
 * new visible item count, even though it may confuse the thing that
 * set the visible count in the first place.
 * Oh, well.
 *
 ****************/

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
	top = lw->list.top_position;
        viz = SetVizCount(lw);

	if ((lw->list.itemCount - top) < viz)
	{
	    top = lw->list.itemCount -  viz;
	    if (top < 0) top = 0;
	    lw->list.top_position = top;
	}
	lw->list.visibleItemCount = viz;
        SetVerticalScrollbar(lw);
        if (lw->list.SizePolicy != XmVARIABLE)
        {
            if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
            {
                if ((listwidth + lw->list.XOrigin) > lw->list.MaxWidth)
                    lw->list.XOrigin = listwidth - lw->list.MaxWidth;
            }
            else
                if ((lw->list.MaxWidth - lw->list.XOrigin) < listwidth)
                lw->list.XOrigin = lw->list.MaxWidth - listwidth;

            if (lw->list.XOrigin < 0) lw->list.XOrigin = 0;
            SetHorizontalScrollbar(lw);
        }
/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
    if (XtIsRealized(lw)) SetClipRect(lw);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * SetVizCount - return the number of items that would fit in the	*
 * current height.  If there are no items, guess.			*
 *									*
 ************************************************************************/
static int 
#ifdef _NO_PROTO
SetVizCount( lw )
        XmListWidget lw ;
#else
SetVizCount(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	viz, line_hi;
	int		borders, list_hi;

		/*  don't let list_hi underflow to a large positive number! */
	borders = 2 * (PPART(lw).shadow_thickness +
			LPART(lw).HighlightThickness +
			LPART(lw).margin_height);

	if ((int)CPART(lw).height <= borders)
		return(1);	/* why list_hi = 1;, just return now! */

	list_hi =  CPART(lw).height - borders;

			/* Just use the calculated heights of the items. */
	if (LPART(lw).InternalList && LPART(lw).itemCount)
	{
		line_hi = LPART(lw).MaxItemHeight;
	/* The fix below doesn't look right for two reasons:
	 *	1. the calcuation of `CumHeight' is that, position 0 (or
	 *	   (0th row in my case) is set to MaxItemHeight, others
	 *	   are incremented by MaxItemHeight + spacing.
	 *	   So, we don't really need to go thru all the checks,
	 *	   can simply let line_hi be MaxItemHeight.
	 *
	 *	2. At end, line_hi will be reduced by `spacing' for
	 *	   the reason I stated above, but, this is wrong
	 *	   because line_hi should MaxItemHeight if itemCount is 1!
	 */
#if 0
/* BEGIN OSF Fix CR 5460 */
/* CR 5460 orginal: I think lineheight should be the difference between 
**	two consecutive InternalList's CumHeight.
**	So, comment out the following if-else...
**  	if (lw->list.InternalList[top]->NumLines > 1)
** 	  lineheight = lw->list.InternalList[top]->height /
** 	    lw->list.InternalList[top]->NumLines;
**  	else
** 	  lineheight = lw->list.InternalList[top]->height;
*/
	if (top)
	  lineheight = lw->list.InternalList[top]->CumHeight -
			lw->list.InternalList[top-1]->CumHeight;
	else if (lw->list.itemCount == 1)
	  lineheight = lw->list.InternalList[0]->CumHeight;
	else
	  lineheight = lw->list.InternalList[1]->CumHeight -
			lw->list.InternalList[0]->CumHeight;
/* the following is to compensate for what is done in the while loop below. */
	lineheight -= lw->list.spacing;
/* END OSF Fix CR 5460 */
#endif
	}
/* BEGIN OSF Fix pir 2730 */
	else /* Have to guess by getting height of default font. */
	{
		XFontStruct *	font_struct = (XFontStruct *)NULL;

		_XmFontListGetDefaultFont(LPART(lw).font, &font_struct);
		if (font_struct)
			line_hi = font_struct->ascent + font_struct->descent;
		else		/* If no font available, use 1.
				 * (0 would cause infinite loop.) */
			line_hi = 1;
	}

	if (line_hi >= list_hi)
	{
		viz = 1;
	}
	else
	{
			/* Only the 1st one has line_hi, others have to
			 * include spacing... */
		viz = 1 + (list_hi - line_hi) /
			  (line_hi + (int)LPART(lw).spacing);
			/* Should we plus one more if there is remainder?
			 * The original code doesn't do that. */
	}
/* END OSF Fix pir 2730 */

	return(viz);
#else /* NOVELL */
    register int viz, lineheight, vizheight;
    int          top, listheight;
 /*  BEGIN OSF Fix pir 2730 */
     XFontStruct        *font_struct = (XFontStruct *) NULL;
 /* END OSF Fix pir 2730 */

     /*  don't let listheight underflow to a large positive number! */
     int borders;
     borders = 2 * (lw->primitive.shadow_thickness +
                  lw->list.HighlightThickness +
                  lw->list.margin_height);
     if (lw->core.height <= borders)
       listheight = 1;
     else
       listheight =  lw->core.height - borders;

 /* BEGIN OSF Fix pir 2730 */
     viz = 0;
 /* END OSF Fix pir 2730 */
      if (lw->list.InternalList && lw->list.itemCount)
       /* Just use the calculated heights of the items. */
       {
  	top = lw->list.top_position;

/* BEGIN OSF Fix CR 5460 */
/* CR 5460 orginal: I think lineheight should be the difference between 
**	two consecutive InternalList's CumHeight.
**	So, comment out the following if-else...
**  	if (lw->list.InternalList[top]->NumLines > 1)
** 	  lineheight = lw->list.InternalList[top]->height /
** 	    lw->list.InternalList[top]->NumLines;
**  	else
** 	  lineheight = lw->list.InternalList[top]->height;
** NOVELL - the fix doesn't look right, see comments in #ifdef NOVELL part!
**	    although CR 7646 takes care of `spacing', it is still too much
**	    see #ifdef NOVELL part.
*/
	if (top)
	  lineheight = lw->list.InternalList[top]->CumHeight -
			lw->list.InternalList[top-1]->CumHeight;
	else if (lw->list.itemCount == 1)
	  /* CR 7646: Include spacing! */
	  lineheight = lw->list.InternalList[0]->CumHeight + lw->list.spacing;
	else
	  lineheight = lw->list.InternalList[1]->CumHeight -
			lw->list.InternalList[0]->CumHeight;
/* the following is to compensate for what is done in the while loop below. */
	lineheight -= lw->list.spacing;
/* END OSF Fix CR 5460 */
 /* BEGIN OSF Fix pir 2730 */
       }
     else /* Have to guess by getting height of default font. */
       {
 	_XmFontListGetDefaultFont(lw->list.font, &font_struct);
	if (font_struct)
	  lineheight = font_struct->ascent + font_struct->descent;
	/* If no font available, use 1. (0 would cause infinite loop.) */
	else lineheight = 1;
       }
     
     vizheight = lineheight;
     while (	vizheight <= listheight)
       {
 	vizheight += lineheight + lw->list.spacing ;
 	viz++;
       }
 /* END OSF Fix pir 2730 */
     if (!viz) viz++;		/* Always have at least one item visible */
 
      return(viz);
#endif /* NOVELL */
   }


/************************************************************************
 *									*
 * SetValues - change the instance data					*
 *									*
 ************************************************************************/
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
#ifdef NOVELL

#define DIFF(f)		LPART(new_w).f != LPART(old).f	/* old == current */
#define PDIFF(f)	PPART(new_w).f != PPART(old).f	/* old == current */
#define CDIFF(f)	CPART(new_w).f != CPART(old).f	/* old == current */
#define USE_OLD(f)	LPART(new_w).f =  LPART(old).f
#define NLW		(XmListWidget)new_w
#define OLW		(XmListWidget)old
#define MOM		((Widget)LPART(new_w).Mom)
#define VSB		((Widget)LPART(new_w).vScrollBar)
#define HSB		((Widget)LPART(new_w).hScrollBar)

	Boolean		ret_val = False,
			new_size = False,
			reset_select = False,
			b_flag;
	register int	i;
	unsigned char	flag;

		/* Make the following resources available to
		 * XtCreateWidget() only */
	if (DIFF(static_rows))
		USE_OLD(static_rows);

	if (DIFF(cols))
		USE_OLD(cols);

	if (DIFF(col_spacing))
		USE_OLD(col_spacing);

	ValidateRepValues(
		new_w,
		LPART(old).SelectionPolicy,
		LPART(old).ScrollBarDisplayPolicy,
		LPART(old).SizePolicy,
		LPART(old).StrDir);

	if (DIFF(SelectionPolicy))
	{
		if (LPART(new_w).SelectionPolicy == XmMULTIPLE_SELECT ||
		    LPART(new_w).SelectionPolicy == XmSINGLE_SELECT)
			LPART(new_w).AddMode = True;
		else
			LPART(new_w).AddMode = False;

		if (DIFF(AddMode))
		{
			DrawHighlight(NLW, LPART(new_w).CurrentKbdItem, False);
			ChangeHighlightGC(NLW, LPART(new_w).AddMode);
			DrawHighlight(NLW, LPART(new_w).CurrentKbdItem, True);
		}
	}

/****************
 *
 * Scrolling attributes - If the scrollbars have changed, pass them onto
 * 		the scrolled window. Check for new resize policy, and disallow
 *		any changes.
 *
 ****************/	
		/* Intentionally-undocumented vestigial code from 1.0 */
	if (MOM && (DIFF(hScrollBar) || DIFF(vScrollBar)))
		XmScrolledWindowSetAreas(MOM, HSB, VSB, new_w);

	if (DIFF(SizePolicy))
	{
		_XmWarning(new_w, ListMessage5);
		USE_OLD(SizePolicy);
	}

/****************
 *
 * Visual attributes
 *
 ****************/

	if (DIFF(StrDir))
		ret_val = True;

	if (DIFF(margin_width) || DIFF(margin_height))
		new_size = True;

		/* Indicate whether ItemSpacing and/or
		 * highlight_thickness changes. */
#define DIFF_ITEM_SP	(1L << 1)
#define DIFF_THICKNESS	(1L << 2)
	flag = 0;
	if (DIFF(ItemSpacing))
	{
		if (LPART(new_w).ItemSpacing >= 0)
		{
			flag |= DIFF_ITEM_SP;
			new_size = True;
		}
		else
		{
			_XmWarning(new_w, ListMessage11);
			USE_OLD(ItemSpacing);
		}
	}

	if (PDIFF(highlight_thickness))
	{
		flag |= DIFF_THICKNESS;
		new_size = True;

		if (PPART(new_w).highlight_thickness)
			LPART(new_w).HighlightThickness =
					PPART(new_w).highlight_thickness + 1;
		else
			LPART(new_w).HighlightThickness = 0;
	}

	/* Indicate whether spacing and/or font changes */
#define DIFF_SPACE	(1L << 1)
#define DIFF_FONT	(1L << 2)

	if (flag)	/* ItemSpacing and/or HighlightThickness changes */
	{
		LPART(new_w).spacing = LPART(new_w).HighlightThickness +
				       LPART(new_w).ItemSpacing;

		if (flag & DIFF_THICKNESS)
			ChangeHighlightGC(NLW, LPART(new_w).AddMode);

		flag = DIFF_SPACE;
	}

#undef DIFF_ITEM_SP
#undef DIFF_THICKNESS

	if (DIFF(visibleItemCount))
	{
		if (LPART(new_w).visibleItemCount <= 0)
		{
			_XmWarning(new_w, ListMessage0);
			USE_OLD(visibleItemCount);

		}
		else
		{
			new_size = True;
			LPART(new_w).LastSetVizCount =
						LPART(new_w).visibleItemCount;
		}
	}

		/* Can't use DIFF(sensitive), see Intrinsic man pages */
	if ((b_flag = XtIsSensitive(new_w)) != XtIsSensitive(old))
	{
		/* Redraw all the time, not just when we've a Mom. */
		ret_val = True;

		if (b_flag == False)
		{
			DrawHighlight(NLW, LPART(new_w).CurrentKbdItem, False);
			LPART(new_w).Traversing = False;
		}
	}

/****************
 *
 * See if either of the lists has changed. If so, free up the old ones,
 * and allocate the new.
 *
 ****************/
	if (DIFF(selectedItems) || DIFF(selectedItemCount))
	{
		if (LPART(new_w).selectedItems &&
		    LPART(new_w).selectedItemCount)
		{
			reset_select = True;

			CopySelectedItems(NLW);
			LPART(new_w).selectedIndices = NULL;
		}
		else if (!LPART(new_w).selectedItemCount)
		{
			reset_select = True;

			LPART(new_w).selectedItems = NULL;
			LPART(new_w).selectedIndices = NULL;
		}
		else if (LPART(new_w).selectedItemCount &&
			 !LPART(new_w).selectedItems)
		{
			_XmWarning(new_w, ListMessage14);
			USE_OLD(selectedItems);
			USE_OLD(selectedItemCount);
		}
		/* else, ListMessage13 will never happen because the code
		 * above treats !LPART(new_w).selectedItemCount as a normal
		 * case... */

		if (reset_select)
			ClearSelectedList(OLW);
	}

/****************
 *
 * If the item list has changed to valid data, free up the old and create
 * the new. If the item count went to zero, delete the old internal list.
 * If the count went < 0, or is > 0 with a NULL items list, complain.
 *
 ****************/
	if (DIFF(items) || DIFF(itemCount))
	{
		int	rows, val;

		if (LPART(new_w).items && LPART(new_w).itemCount)
		{
			new_size = reset_select = True;

			CalcNumRows(new_w, &rows, &val);

				/* visibleItemCount is visibleRows for now */
			if (LPART(new_w).top_position +
			    LPART(new_w).visibleItemCount > rows)
			{
				val = rows - LPART(new_w).visibleItemCount;
				LPART(new_w).top_position = val > 0 ? val : 0;
			}
		}
		else if (!LPART(new_w).itemCount)
		{
			new_size = reset_select = True;

			LPART(new_w).items = NULL;
			LPART(new_w).InternalList = NULL;

				/* just set it to 0 because itemCount is 0 */
			LPART(new_w).top_position = 0;
		}
		else if (LPART(new_w).itemCount && !LPART(new_w).items)
		{
			_XmWarning(new_w, ListMessage12);
			USE_OLD(items);
			USE_OLD(itemCount);
                }
		/* else, ListMessage6 will never happen because the code
		 * above treats !LPART(new_w).itemCount as a normal case... */

		if (reset_select)
		{
#if 0
			/* The fix below looks like a hack and has
			 * performance implications because CopyItems()
			 * should be called only if both itemCount and items
			 * are valid!
			 *
			 * I think the intent here is to avoid XmString's
			 * count goes down to zero when the old item list
			 * is destroyed (via ClearItemList()), see
			 * DestroyInternalList(). This is because the
			 * original code destroys the old item list first
			 * and then creates the new one. The really fix
			 * is to reverse the order, i.e., doing creation
			 * first and then destroying */

		CopyItems(NLW); /* Fix for CR 5571 */
#endif
				/* In OSF 1.2.3, the following line (along
				 * with other lines) is in ClearItemList()
				 * thru old.core->self, so we have do following
				 * here and restore it back if old.itemCount,
				 * see below, sigh... */
			LPART(new_w).LastItem = 0;

			if (LPART(new_w).itemCount)
				CreateInternalList(new_w);

			if (LPART(old).itemCount)
			{
				int	save_this = LPART(new_w).LastItem;

				DestroyInternalList(old);
				LPART(new_w).LastItem = save_this;
			}
		}
	}

	if (PDIFF(highlight_color) || PDIFF(highlight_pixmap))
		MakeHighlightGC(NLW, LPART(new_w).AddMode);

	if (PDIFF(foreground) || CDIFF(background_pixel) || DIFF(font))
	{
#define FONT	LPART(new_w).font

		ret_val = True;
		if (!FONT)
		{
			FONT = _XmGetDefaultFontList(new_w, XmTEXT_FONTLIST);
		}
		else
		{
			if (DIFF(font))
			{
				new_size = True;
				flag |= DIFF_FONT;

				XmFontListFree(LPART(old).font);

				for (i = 0; i < LPART(new_w).itemCount; i++)
					_XmStringUpdate(FONT,
					  LPART(new_w).InternalList[i]->name);
			}
		}
		FONT = XmFontListCopy(FONT);
		MakeGC(NLW);
	}

#define EL	LPART(new_w).InternalList[i]
#define GD	EL->glyph_data

		/* The code below are combinations of ResetHeight() and
		 * ResetWidth(). This is because only SetValues() uses
		 * these two function calls in NOVELL enhancements */
	if (flag && LPART(new_w).itemCount)
	{
		if (flag & DIFF_FONT)
		{
			Dimension	max_hi = 0, gd_hi;

			for (i = 0; i < LPART(new_w).itemCount; i++)
			{
					/* Take OSF 1.2.3 approach! */
				_XmStringExtent(FONT,
					EL->name, &EL->width, &EL->height);

				if (GD)
				{
					EL->width +=GD->width + GD->h_pad;
					gd_hi = GD->height + 2 * GD->v_pad;
					if (EL->height < gd_hi)
						EL->height = gd_hi;
				}
				if (max_hi < EL->height)
					max_hi = EL->height;
			}
			LPART(new_w).MaxItemHeight = max_hi;
		}
		CalcCumHeight(new_w);
	}
#undef EL
#undef GD
#undef DIFF_SPACE
#undef DIFF_FONT
#undef FONT

		/* Note that top_position is top_row_position for now */
	b_flag = False;
	if (DIFF(top_position))
	{
/* BEGIN OSF Fix CR 5740 */
		if (LPART(new_w).top_position < -1)
/* END OSF Fix CR 5740 */
		{
			_XmWarning(new_w, ListMessage15);
			USE_OLD(top_position);
		}
		else
		{
			Arg	arg[1];

/* BEGIN OSF Fix CR 5740 */
			if (LPART(new_w).top_position == -1)
			{
				int	rows, val;

				CalcNumRows(new_w, &rows, &val);
				LPART(new_w).top_position = rows ? rows - 1 : 0;
			}
/* END OSF Fix CR 5740 */
			SetVerticalScrollbar(NLW);
			if (!new_size && !ret_val)
			{
				DrawList(NLW, NULL, True);
				b_flag = True;
			}
			else if (LPART(old).Traversing)
			{
				DrawHighlight(
					OLW, LPART(old).CurrentKbdItem, False);
			}
		}
	}

	if (new_size)
	{
		Dimension	wd, hi;

		ret_val = True;
		SetDefaultSize(NLW, &wd, &hi);

		LPART(new_w).BaseX = LPART(new_w).HighlightThickness +
				     PPART(new_w).shadow_thickness +
				     (Position)LPART(new_w).margin_width;

		LPART(new_w).BaseY = LPART(new_w).HighlightThickness +
				     PPART(new_w).shadow_thickness +
				    (Position )LPART(new_w).margin_height;

		if (LPART(new_w).SizePolicy != XmCONSTANT ||
		    !CPART(new_w).width)
			CPART(new_w).width = wd;
		CPART(new_w).height = hi;
	}

/****************
 *
 * Really change this whole if stmt???
    if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
        (ClearSelect) ||
        (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
 ***************/
	if (reset_select)
	{
		for (i = 0; i < LPART(new_w).itemCount; i++)
		{
			LPART(new_w).InternalList[i]->selected		=
			LPART(new_w).InternalList[i]->last_selected	=
				OnSelectedList(NLW, LPART(new_w).items[i]);
		}
	
			/* Although selected_indices is updated, the
			 * selected list may be out of date.
			 * This is a problem that we can't always
			 * rely on selected list and have to rely on
			 * UpdateSelectedList() even when it may not
			 * be necessary... sigh! */
		UpdateSelectedIndices(NLW);
		if (!new_size)
		{
			if (!b_flag)	/* See DIFF(top_position) */
				DrawList(NLW, NULL, True);
			SetSelectionParams(NLW);
		}
	}

		/* Don't know the logic of the code below */
	if (!LPART(new_w).FromSetNewSize)
	{
		if (LPART(new_w).SizePolicy != XmVARIABLE ||
		    LPART(new_w).cols != 1)
			SetHorizontalScrollbar(NLW);
		SetVerticalScrollbar(NLW);
	}

	return(ret_val);
#undef DIFF
#undef PDIFF
#undef CDIFF
#undef USE_OLD
#undef NLW
#undef OLW
#undef MOM
#undef VSB
#undef HSB

#else /* NOVELL */
    register XmListWidget oldlw = (XmListWidget) old;
    register XmListWidget newlw = (XmListWidget) new_w;
    Boolean 	  NewSize = FALSE,RetVal = FALSE, ResetSelect = FALSE;
    Dimension	  width, height;
    register int  i;
    int      j;

    width = 0;
    height = 0;


    if(    !XmRepTypeValidValue( XmRID_SELECTION_POLICY,
                              newlw->list.SelectionPolicy, (Widget) newlw)    )
    {
        newlw->list.SelectionPolicy = oldlw->list.SelectionPolicy;
    }

    if (newlw->list.SelectionPolicy != oldlw->list.SelectionPolicy)
    {
        if ((newlw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
            (newlw->list.SelectionPolicy == XmSINGLE_SELECT))
            newlw->list.AddMode = TRUE;
        else
            newlw->list.AddMode = FALSE;

        if(newlw->list.AddMode != oldlw->list.AddMode)
        {
	    DrawHighlight(newlw, newlw->list.CurrentKbdItem, FALSE);
            ChangeHighlightGC(newlw, newlw->list.AddMode);
            DrawHighlight(newlw, newlw->list.CurrentKbdItem, TRUE);
        }
    }
    if(    !XmRepTypeValidValue( XmRID_LIST_SIZE_POLICY,
                                   newlw->list.SizePolicy, (Widget) newlw)    )
    {
        newlw->list.SizePolicy = oldlw->list.SizePolicy;
    }
    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                       newlw->list.ScrollBarDisplayPolicy, (Widget) newlw)    )
    {
        newlw->list.ScrollBarDisplayPolicy = oldlw->list.ScrollBarDisplayPolicy;
    }
    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION, newlw->list.StrDir,
                                                           (Widget) newlw)    )
    {
        newlw->list.StrDir = oldlw->list.StrDir;
    }

/****************
 *
 * Scrolling attributes - If the scrollbars have changed, pass them onto
 * 		the scrolled window. Check for new resize policy, and disallow
 *		any changes.
 *
 ****************/	
	/* Intentionally-undocumented vestigial code from 1.0 */
    if ((newlw->list.hScrollBar != oldlw->list.hScrollBar) ||
       (newlw->list.vScrollBar != oldlw->list.vScrollBar))
    {
	if (newlw->list.Mom)
	    XmScrolledWindowSetAreas( (Widget) newlw->list.Mom, (Widget) newlw->list.hScrollBar,
				     (Widget) newlw->list.vScrollBar, (Widget) newlw);
    }

    if (newlw->list.SizePolicy != oldlw->list.SizePolicy)
    {
	_XmWarning( (Widget) newlw, ListMessage5);
        newlw->list.SizePolicy = oldlw->list.SizePolicy;
    }
/****************
 *
 * Visual attributes
 *
 ****************/

    if (newlw->list.StrDir != oldlw->list.StrDir) RetVal = TRUE;

    if ((newlw->list.margin_width != oldlw->list.margin_width) ||
       (newlw->list.margin_height != oldlw->list.margin_height))
            NewSize = TRUE;

    if (newlw->list.ItemSpacing != oldlw->list.ItemSpacing)
        if (newlw->list.ItemSpacing >= 0)
            NewSize = TRUE;
        else
        {
            newlw->list.ItemSpacing = oldlw->list.ItemSpacing;
            _XmWarning( (Widget) newlw, ListMessage11);
        }

    if ((newlw->list.ItemSpacing != oldlw->list.ItemSpacing) ||
        (newlw->primitive.highlight_thickness != oldlw->primitive.highlight_thickness))
    {
	NewSize = TRUE;
        if (newlw->primitive.highlight_thickness)
            newlw->list.HighlightThickness = newlw->primitive.highlight_thickness + 1;
        else
            newlw->list.HighlightThickness = 0;

        newlw->list.spacing = newlw->list.HighlightThickness +
			      newlw->list.ItemSpacing;
        ChangeHighlightGC(newlw, newlw->list.AddMode);
	ResetHeight(newlw);
    }

    if (newlw->list.visibleItemCount != oldlw->list.visibleItemCount)
    {
    	if (newlw->list.visibleItemCount <= 0)
    	{
	    newlw->list.visibleItemCount = oldlw->list.visibleItemCount;
	    _XmWarning( (Widget) newlw, ListMessage0);

    	}
        else
        {
	    NewSize = TRUE;
            newlw->list.LastSetVizCount = newlw->list.visibleItemCount;
        }
    }
	/* My version already did this part a long time ago... */
#ifndef OSF_v1_2_4
    if (newlw->core.sensitive != oldlw->core.sensitive)
#else /* OSF_v1_2_4 */
    if (XtIsSensitive(new_w) != XtIsSensitive(old))
#endif /* OSF_v1_2_4 */
    {
#ifdef OSF_v1_2_4
        /* CR 6412: Redraw all the time, not just when we've a Mom. */
	RetVal = TRUE;
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
	    RetVal = TRUE;
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
	if (newlw->core.sensitive == FALSE)
#else /* OSF_v1_2_4 */
	if (! XtIsSensitive(new_w))
#endif /* OSF_v1_2_4 */
	{
	    DrawHighlight(newlw, newlw->list.CurrentKbdItem, FALSE);
	    newlw->list.Traversing = FALSE;
	}
    }

/****************
 *
 * See if either of the lists has changed. If so, free up the old ones,
 * and allocate the new.
 *
 ****************/
    if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
        (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
    {
        if (newlw->list.selectedItems && (newlw->list.selectedItemCount > 0))
        {
           CopySelectedItems(newlw);
	   newlw->list.selectedIndices = NULL;
           ClearSelectedList(oldlw);
	   ResetSelect = TRUE;
        }
        else
            if (newlw->list.selectedItemCount == 0)
            {
	        ClearSelectedList(oldlw);
    		newlw->list.selectedItems = NULL;
    		newlw->list.selectedIndices = NULL;
                ResetSelect = TRUE;
            }
            else
            {
                if ((newlw->list.selectedItemCount > 0) &&
                    (newlw->list.selectedItems == NULL))
                {
	            _XmWarning( (Widget) newlw, ListMessage14);
                    newlw->list.selectedItems = oldlw->list.selectedItems;
                    newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
                }
                else
                {
	            _XmWarning( (Widget) newlw, ListMessage13);
                    newlw->list.selectedItems = oldlw->list.selectedItems;
                    newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
                }
            }

    }

/****************
 *
 * If the item list has changed to valid data, free up the old and create
 * the new. If the item count went to zero, delete the old internal list.
 * If the count went < 0, or is > 0 with a NULL items list, complain.
 *
 ****************/
    if ((newlw->list.items != oldlw->list.items) ||
        (newlw->list.itemCount != oldlw->list.itemCount))
    {
        CopyItems(newlw); /* Fix for CR 5571 - NOVELL, this has perf implications, see comments in #ifdef NOVELL part above */
	if (newlw->list.items && (newlw->list.itemCount > 0))
    	{
            if (oldlw->list.items && (oldlw->list.itemCount > 0))
            {
                j = oldlw->list.itemCount;
                for (i = oldlw->list.itemCount - 1; i >= 0; i--)
	        {
		    oldlw->list.itemCount--;
	    	    DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), FALSE);
	        }
                oldlw->list.itemCount = j;
	        if (oldlw->list.InternalList) XtFree((char *)oldlw->list.InternalList);
    	        ClearItemList(oldlw);
            }
	    ResetSelect = TRUE;
	    CopyItems(newlw);

	    newlw->list.InternalList = (ElementPtr *)XtMalloc((sizeof(Element *) *
	    						        newlw->list.itemCount));
	    if ((newlw->list.top_position + newlw->list.visibleItemCount) >
                    newlw->list.itemCount)
                newlw->list.top_position =
                        ((newlw->list.itemCount - newlw->list.visibleItemCount) > 0) ?
                         (newlw->list.itemCount - newlw->list.visibleItemCount) : 0;

	    for (i = 0; i < newlw->list.itemCount; i++)
	    	AddInternalElement(newlw, newlw->list.items[i], 0,
	    				OnSelectedList(newlw,newlw->list.items[i]),
					FALSE);
	    NewSize = TRUE;
	 }
	 else
	 {
            if (newlw->list.itemCount == 0)
            {
                j = oldlw->list.itemCount;
                for (i = oldlw->list.itemCount - 1; i >= 0; i--)
	        {
		    oldlw->list.itemCount--;
	    	    DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), FALSE);
	        }
	        if (oldlw->list.InternalList) XtFree((char *)oldlw->list.InternalList);
                oldlw->list.itemCount = j;
	        ClearItemList(oldlw);
	        newlw->list.InternalList = NULL;
                newlw->list.items = NULL;
		ResetSelect = TRUE;
	        NewSize = TRUE;
                if ((newlw->list.top_position + newlw->list.visibleItemCount) >
                    newlw->list.itemCount)
                newlw->list.top_position =
                        ((newlw->list.itemCount - newlw->list.visibleItemCount) > 0) ?
                         (newlw->list.itemCount - newlw->list.visibleItemCount) : 0;
            }
            else
            {
                if ((newlw->list.itemCount > 0) && (newlw->list.items == NULL))
                {
	            _XmWarning( (Widget) newlw, ListMessage12);
	            newlw->list.items = oldlw->list.items;
	            newlw->list.itemCount = oldlw->list.itemCount;
                }
                else
                {
	            _XmWarning( (Widget) newlw, ListMessage6);
	            newlw->list.items = oldlw->list.items;
	            newlw->list.itemCount = oldlw->list.itemCount;
                }
            }
	 }

    }

   if (newlw->primitive.highlight_color != oldlw->primitive.highlight_color ||
       newlw->primitive.highlight_pixmap != oldlw->primitive.highlight_pixmap)
        MakeHighlightGC(newlw, newlw->list.AddMode);

    if ((newlw->primitive.foreground != oldlw->primitive.foreground) ||
	(newlw->core.background_pixel != oldlw->core.background_pixel) ||
	(newlw->list.font != oldlw->list.font))
    {
        if (newlw->list.font != NULL)
	{
	    if (newlw->list.font != oldlw->list.font)
	    {
                XmFontListFree(oldlw->list.font);
                newlw->list.font = XmFontListCopy(newlw->list.font);
	      	for (i = 0; i < newlw->list.itemCount; i++)
		    _XmStringUpdate(newlw->list.font, newlw->list.InternalList[i]->name);
	    	NewSize = TRUE;
		ResetHeight(newlw);
		ResetWidth(newlw);
	    }
	}
	else
	{
            newlw->list.font = _XmGetDefaultFontList ( (Widget) newlw,XmTEXT_FONTLIST);
            newlw->list.font = XmFontListCopy(newlw->list.font);
	}
	MakeGC(newlw);
        RetVal = TRUE;
    }

    if (newlw->list.top_position != oldlw->list.top_position)
    {
/* BEGIN OSF Fix CR 5740 */
    	if (newlw->list.top_position < -1)
/* END OSF Fix CR 5740 */
    	{
	    newlw->list.top_position = oldlw->list.top_position;
	    _XmWarning( (Widget) newlw, ListMessage15);

    	}
        else
        {
/* BEGIN OSF Fix CR 5740 */
	  if (newlw->list.top_position == -1)
	    newlw->list.top_position =
	      newlw->list.itemCount ? newlw->list.itemCount - 1 : 0;
/* END OSF Fix CR 5740 */

          if (oldlw->list.Traversing)
               DrawHighlight(oldlw, oldlw->list.CurrentKbdItem, FALSE);
	    DrawList(newlw, NULL, TRUE);
            SetVerticalScrollbar(newlw);
        }
    }

/****************
 *
 * Really change this whole if stmt???
    if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
        (ClearSelect) ||
        (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
 ***************/
    if (ResetSelect)
    {
	for (i = 0; i < newlw->list.itemCount; i++)
        {
            newlw->list.InternalList[i]->selected = OnSelectedList(newlw,
							newlw->list.items[i]);
	    newlw->list.InternalList[i]->last_selected =
        	newlw->list.InternalList[i]->selected;
        }
	
/*	BuildSelectedList(newlw);*/
        UpdateSelectedIndices(newlw);
	if (!NewSize)
	{
	    DrawList (newlw, NULL, TRUE);
            SetSelectionParams(newlw);
	}
    }

    if (NewSize)
    {
	RetVal = TRUE;
	SetDefaultSize(newlw,&width, &height);
	newlw->list.BaseX = (Position )newlw->list.margin_width +
    			    newlw->list.HighlightThickness +
	            	    newlw->primitive.shadow_thickness;

	newlw->list.BaseY = (Position )newlw->list.margin_height +
    			    newlw->list.HighlightThickness +
			    newlw->primitive.shadow_thickness;

	if ((newlw->list.SizePolicy != XmCONSTANT) ||
            !(newlw->core.width))
	    newlw->core.width = width;
	newlw->core.height = height;
    }
    if (!newlw->list.FromSetNewSize)
    {
        if (newlw->list.SizePolicy != XmVARIABLE)
            SetHorizontalScrollbar(newlw);
        SetVerticalScrollbar(newlw);
    }
    return (Boolean)(RetVal);
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * Destroy - destroy the list instance.  Free up all of our memory.	*
 *									*
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
#ifdef NOVELL
	if (LPART(wid).DragID)
		XtRemoveTimeOut(LPART(wid).DragID);

	if (LPART(wid).NormalGC != NULL)
		XFreeGC(XtDisplay(wid), LPART(wid).NormalGC);

	if (LPART(wid).InverseGC != NULL)
		XFreeGC(XtDisplay(wid), LPART(wid).InverseGC);

	if (LPART(wid).HighlightGC != NULL)
		XFreeGC(XtDisplay(wid), LPART(wid).HighlightGC);

	if (LPART(wid).InsensitiveGC != NULL)
		XFreeGC(XtDisplay(wid), LPART(wid).InsensitiveGC);

	if (LPART(wid).itemCount)
		DestroyInternalList(wid);

	ClearSelectedList((XmListWidget)wid);
	XmFontListFree(LPART(wid).font);

/****************
 *
 * Free the callback lists.
 *
 ****************/
		/* Intrinsic should already do this for us! */
	XtRemoveAllCallbacks(wid, XmNsingleSelectionCallback);
	XtRemoveAllCallbacks(wid, XmNmultipleSelectionCallback);
	XtRemoveAllCallbacks(wid, XmNextendedSelectionCallback);
	XtRemoveAllCallbacks(wid, XmNbrowseSelectionCallback);
	XtRemoveAllCallbacks(wid, XmNdefaultActionCallback);

	if (LPART(wid).max_col_width != &LPART(wid).MaxWidth)
		XtFree((XtPointer)LPART(wid).max_col_width);

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int i,j;

/*    DashImage = NULL;


    XmDestroyPixmap(XtScreen(lw), lw ->list.DashTile);
*/
    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),lw->list.NormalGC);
    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),lw->list.InverseGC);
    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),lw->list.HighlightGC);
    if (lw->list.InsensitiveGC != NULL) XFreeGC(XtDisplay(lw),lw->list.InsensitiveGC);

    if (lw->list.itemCount)
    {
        j = lw->list.itemCount;
	for (i = lw->list.itemCount - 1; i >= 0; i--)
	{
	    lw->list.itemCount--;
	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
	}
        lw->list.itemCount = j;
	ClearItemList(lw);
	XtFree((char *) lw->list.InternalList);
    }

    ClearSelectedList(lw);
    XmFontListFree(lw->list.font);

/****************
 *
 * Free the callback lists.
 *
 ****************/
    XtRemoveAllCallbacks((Widget) lw, XmNsingleSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNmultipleSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNextendedSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNbrowseSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNdefaultActionCallback);

#endif /* NOVELL */
}


/************************************************************************
 *									*
 *  QueryProc - Look at a new geometry and add/delete scrollbars as     *
 *	needed.								*
 *									*
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryProc( wid, request, ret )
        Widget wid ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *ret ;
#else
QueryProc(
        Widget wid,
        XtWidgetGeometry *request,
        XtWidgetGeometry *ret )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
   Dimension	MyWidth, MyHeight, NewWidth, NewHeight, sbWidth, sbHeight,
                vizheight, lineheight, HSBheight, VSBwidth;
   Dimension    HSBht, VSBht;
   Dimension    pad, HSBbw, VSBbw;
   int          viz;
   Boolean      HasVSB, HasHSB;
   XtGeometryResult retval = XtGeometryYes;
   ret -> request_mode = 0;


/****************
 *
 * If this is a request generated by our code, just return yes.
 *
 ****************/
    if (lw->list.FromSetSB) return(retval);

#ifndef OSF_v1_2_4
    pad = (lw->list.Mom) ? ((XmScrolledWindowWidget)(lw->list.Mom))->swindow.pad
                         : 0;
#else /* OSF_v1_2_4 */
    pad = (lw->list.Mom ?
	   ((XmScrolledWindowWidget)(lw->list.Mom))->swindow.pad : 0);
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
    HSBht = (lw->list.hScrollBar) ? lw->list.hScrollBar->primitive.highlight_thickness * 2
                                  : 0;
    HSBbw = (lw->list.hScrollBar) ? lw->list.hScrollBar->core.border_width
                                  : 0;
    HSBheight = (lw->list.hScrollBar) ? lw->list.hScrollBar->core.height
                                      : 0;
    VSBht = (lw->list.vScrollBar) ? lw->list.vScrollBar->primitive.highlight_thickness * 2
                                  : 0;
    VSBwidth = (lw->list.vScrollBar) ? lw->list.vScrollBar->core.width
                                     : 0;
    VSBbw = (lw->list.hScrollBar) ? lw->list.vScrollBar->core.border_width
                                  : 0;
#else /* OSF_v1_2_4 */
    HSBht = (lw->list.hScrollBar ?
	     lw->list.hScrollBar->primitive.highlight_thickness * 2 : 0);
    HSBbw = (lw->list.hScrollBar ? lw->list.hScrollBar->core.border_width : 0);
    HSBheight = (lw->list.hScrollBar ? lw->list.hScrollBar->core.height : 0);
    VSBht = (lw->list.vScrollBar ?
	     lw->list.vScrollBar->primitive.highlight_thickness * 2 : 0);
    VSBwidth = (lw->list.vScrollBar ? lw->list.vScrollBar->core.width : 0);
    VSBbw = (lw->list.vScrollBar ? lw->list.vScrollBar->core.border_width : 0);
#endif /* OSF_v1_2_4 */
/****************
 *
 * If a preferred size query, make sure we use the last requested visible
 * item count for our basis.
 *
 ****************/
    if (request->request_mode == 0)
    {
        viz = lw->list.visibleItemCount;
/* BEGIN OSF Fix CR 6014 */
	if (lw->list.LastSetVizCount)
	  lw->list.visibleItemCount = lw->list.LastSetVizCount;
/* END OSF Fix CR 6014 */
        SetDefaultSize(lw,&MyWidth, &MyHeight);
        lw->list.visibleItemCount = viz;
    }
    else
        SetDefaultSize(lw,&MyWidth, &MyHeight);

/****************
 *
 * If the request mode is zero, fill out out default height & width.
 *
 ****************/
    if ((request->request_mode == 0) ||
        !lw->list.InternalList)
    {
        ret->width = MyWidth;
        ret->height = MyHeight;
        ret->request_mode = (CWWidth | CWHeight);
        return (XtGeometryAlmost);
    }

/****************
 *
 * If we're not scrollable, or don' have any scrollbars, return yes.
 *
 ****************/
   if ((!lw->list.Mom) ||
       (!lw->list.vScrollBar && !lw->list.hScrollBar))
          return(XtGeometryYes);

/****************
 *
 * Else we're being queried from a scrolled window - look at the
 * dimensions and manage/unmanage the scroll bars according to the
 * new size. The scrolled window will take care of the actual sizing.
 *
 ****************/

   if (request -> request_mode & CWWidth)
        NewWidth = request->width;
   else
        NewWidth = lw->core.width;

   if (request -> request_mode & CWHeight)
        NewHeight = request->height;
   else
        NewHeight = lw->core.height;

/****************
 *
 * Look at the dimensions and calculate if we need a scrollbar. This can
 * get hairy in the boundry conditions - where the addition/deletion of
 * one scrollbar can affect the other.
 *
 ****************/
    if (((NewHeight < MyHeight) &&
         (NewWidth  < MyWidth))  ||
         (lw->list.ScrollBarDisplayPolicy == XmSTATIC))
    {
        HasVSB = HasHSB = TRUE;
    }
    else
    {
/****************
 *
 * Else we have do do some hard work. See if there is a definite need for a
 * horizontal scroll bar; and set the availible height accordingly. Then,
 * figure out how many lines will fit in the space. If that is less than
 * the number of items. then fire up the vertical scrollbar. Then check
 * to see if the act of adding the vsb kicked in the hsb.
 * Amazingly, it *seems* to work right.
 *
 ****************/

        lineheight = lw->list.MaxItemHeight;

        {
	    int borders;
	    /* Don't let NewHeight underflow to become a large positive # */
	    borders = (2 * (lw->primitive.shadow_thickness +
			    lw->list.HighlightThickness +
			    lw->list.margin_height));
#ifdef NOVELL
	    if ((int)NewHeight <= borders)
#else /* NOVELL */
	    if (NewHeight <= borders)
#endif /* NOVELL */
		NewHeight = 1;
	    else
		NewHeight -= borders;
        }

#ifdef NOVELL
        if (MyWidth > NewWidth &&
	    (LPART(lw).SizePolicy != XmVARIABLE || LPART(lw).cols != 1)) 
#else	/* NOVELL */
        if ((MyWidth > NewWidth) && (lw->list.SizePolicy != XmVARIABLE)) 
#endif	/* NOVELL */
	{
	    /* Take the height of the horizontal SB into account, but */
	    /* don't allow sbHeight to underflow to a large positive # */
	    int borders;
	    borders = HSBheight + HSBht + HSBbw*2 + pad;
#ifdef NOVELL
	    if ((int)NewHeight <= borders)
#else /* NOVELL */
	    if (NewHeight <= borders)
#endif /* NOVELL */
		sbHeight = 1;
	    else
		sbHeight = NewHeight - borders;
	}
        else
            sbHeight = NewHeight;

        viz = 0;
	vizheight = lineheight;
	while (vizheight <= sbHeight)
	{
	    vizheight += lineheight + lw->list.spacing ;
	    viz++;
	}
	if (!viz) viz++;

        if (lw->list.itemCount > viz)
            HasVSB = TRUE;
        else
            HasVSB = FALSE;

        if (HasVSB) {
	    /* take the width of the vertical SB into account, but don't */
	    /* allow sbWidth to underflow to a large positive number */
	    int borders;
	    borders = VSBwidth + VSBht + VSBbw*2 + pad;
#ifdef NOVELL
	    if ((int)NewWidth <= borders)
#else /* NOVELL */
	    if (NewWidth <= borders)
#endif /* NOVELL */
		sbWidth = 1;
	    else
		sbWidth = NewWidth - borders;
        }
        else
           sbWidth = NewWidth;

	HasHSB = (MyWidth > sbWidth);
    }

    if (lw->list.vScrollBar)
        if (HasVSB)
            XtManageChild((Widget) lw->list.vScrollBar);
        else
            XtUnmanageChild((Widget) lw->list.vScrollBar);

    if (lw->list.hScrollBar)
#ifdef NOVELL
        if (HasHSB &&
	    (LPART(lw).SizePolicy != XmVARIABLE || LPART(lw).cols != 1))
            XtManageChild((Widget) lw->list.hScrollBar);
#else	/* NOVELL */
        if (HasHSB && (lw->list.SizePolicy != XmVARIABLE))
            XtManageChild((Widget) lw->list.hScrollBar);
#endif	/* NOVELL */
        else
            XtUnmanageChild((Widget) lw->list.hScrollBar);

   return(retval);
}

/************************************************************************
 *                                                                      *
 * Conversion routines for XmNtopItemPostion.  Necessary because the    *
 * outside world is 1-based, and the inside world is 0-based.  Sigh.    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CvtToExternalPos( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
CvtToExternalPos(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    (*value) = (XtArgVal) (lw->list.top_position + 1);
}

/* ARGSUSED */
static XmImportOperator 
#ifdef _NO_PROTO
CvtToInternalPos( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
CvtToInternalPos(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    (* (int *) value)--;
    return (XmSYNTHETIC_LOAD);
}



/************************************************************************
 *									*
 *                           Visiual Routines				*
 *									*
 ************************************************************************/

#ifndef NOVELL
/************************************************************************
 *									*
 * DrawListShadow - draw the shadow					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawListShadow( w )
        XmListWidget w ;
#else
DrawListShadow(
        XmListWidget w )
#endif /* _NO_PROTO */
{
/****************
 *
 * 
   if (w -> primitive.shadow_thickness)
     _XmDrawShadow (XtDisplay (w), XtWindow (w),
                  w -> primitive.bottom_shadow_GC,
                  w -> primitive.top_shadow_GC,
                  w -> primitive.shadow_thickness,
		  0,
		  0,
                  (int)w->core.width,
                  (int)w->core.height);
 *
 ****************/
     _XmDrawShadows (XtDisplay (w), XtWindow (w),
 		     w -> primitive.bottom_shadow_GC,
 		     w -> primitive.top_shadow_GC,
 		     0, 0,
 		     (int)w->core.width,
 		     (int)w->core.height,
 		     w -> primitive.shadow_thickness,
 		     XmSHADOW_OUT);
}
#endif /* NOVELL */

/************************************************************************
 *									*
 * DrawList - draw the contents of the list.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DrawList( w, event, all )
        XmListWidget w ;
        XEvent *event ;
        Boolean all ;
#else
DrawList(
        XmListWidget w,
        XEvent *event,
#if NeedWidePrototypes
        int all )
#else
        Boolean all )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	viz_wd,
			i, cur_row;
	int		borders, cum_wd, avail_hi,
			top_row, top_pos, cur_pos, viz_rows,
			rows, last_row_size;
	Dimension	sep_wd;
	Position	item_x, item_y = 0, delta;

	if (!XtIsRealized(w) || !(LPART(w).items && LPART(w).itemCount))
		return;

		/* don't allow underflow */
	borders = LPART(w).margin_width +
		  LPART(w).HighlightThickness +
		  PPART(w).shadow_thickness;
	if ((int)CPART(w).width <= borders)
		return; /* why viz_wd = 1? */

#define BDR	(int)PPART(w).shadow_thickness
	XClearArea(XtDisplay(w), XtWindow(w), BDR, BDR,
		CPART(w).width - 2 * BDR,
		CPART(w).height - 2 * BDR, False);
#undef BDR

	viz_wd = CPART(w).width - borders;

	CalcNumRows((Widget)w, &rows, &last_row_size);
	top_row = LPART(w).top_position;
	top_pos = top_row * LPART(w).cols;

	top_row += LPART(w).static_rows;
	viz_rows = top_row + LPART(w).visibleItemCount - LPART(w).static_rows;
	if (viz_rows > rows - LPART(w).static_rows)
		viz_rows = rows;

/* Draw static_rows first if there is any */

	if (!LPART(w).static_rows)
		goto draw_list;

		/* Borrow space (XmNhighlightThickness) from item(s)
		 * for separator drawing... */
	delta = PPART(w).highlight_thickness;

	item_x = LPART(w).BaseX - LPART(w).XOrigin;
	cum_wd = LPART(w).BaseX;

	for (i = 0; i < (int)LPART(w).cols; i++)
	{
		cum_wd += LPART(w).max_col_width[i];
		for (cur_row = 0; cur_row < (int)LPART(w).static_rows;cur_row++)
		{
				/* We don't need to continue if we located
				 * an empty column in last row */
			if (EmptyColInLastRow(cur_row, rows, last_row_size, i))
				break;

			cur_pos = i + cur_row * LPART(w).cols;

			item_y = LPART(w).InternalList[cur_pos]->CumHeight -
				 LPART(w).InternalList[i]->CumHeight +
				 LPART(w).BaseY - delta;
			ItemDraw((Widget)w,
				False,
				item_x, item_y, cur_pos, i,
				viz_wd, cum_wd, borders);
		}
		item_x += LPART(w).max_col_width[i] +
			  LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;
		cum_wd += LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;
	}

/* note for "delta", if delta is `0' then don't draw shadows/separator */
	if (!delta)
		goto draw_list;

#define P		LPART(w).static_rows * LPART(w).cols
#define STATIC_ROWS_HI	LPART(w).InternalList[P]->CumHeight -\
			LPART(w).spacing -\
			LPART(w).InternalList[0]->CumHeight

	sep_wd = PPART(w).highlight_thickness == 1 ?
					 2 : PPART(w).highlight_thickness;

	_XmDrawSeparator(XtDisplay(w), XtWindow(w),
		PPART(w).top_shadow_GC,
		PPART(w).bottom_shadow_GC,
		LPART(w).NormalGC,
		LPART(w).BaseX - LPART(w).HighlightThickness,
		LPART(w).BaseY - LPART(w).HighlightThickness +
				 STATIC_ROWS_HI +
				 PPART(w).highlight_thickness - delta,
		viz_wd - borders + 2 * LPART(w).HighlightThickness,
		sep_wd, sep_wd,
		0,
		XmHORIZONTAL,
		XmSHADOW_ETCHED_OUT);

#undef P
#undef STATIC_ROWS_HI

		/* may be unable to draw the list because of hi */
	item_y += delta;
draw_list:
	item_x = LPART(w).BaseX - LPART(w).XOrigin;
	cum_wd = LPART(w).BaseX;
	delta = 0;	/* re-use delta here */
	for (i = 0; i < (int)LPART(w).cols; i++)
	{
		cum_wd += LPART(w).max_col_width[i];
		for (cur_row = top_row; cur_row < viz_rows; cur_row++)
		{
				/* We don't need to continue if we located
				 * an empty column in last row */
			if (EmptyColInLastRow(cur_row, rows, last_row_size, i))
			{
				delta = 1;
				break;
			}

			cur_pos = i + cur_row * LPART(w).cols;

			LPART(w).InternalList[cur_pos]->LastTimeDrawn =
				LPART(w).InternalList[cur_pos]->selected;

			item_y = LPART(w).InternalList[cur_pos]->CumHeight -
				 LPART(w).InternalList[top_pos+i]->CumHeight +
				 LPART(w).BaseY;

			ItemDraw((Widget)w, True,
				item_x, item_y,
				cur_pos, i,
				viz_wd, cum_wd, borders);
		}

		item_x += LPART(w).max_col_width[i] +
			  LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;
		cum_wd += LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;
	}
		/* Re-adjust item_y, if last row has empty col(s) */
	if (delta)
	{
			/* find 1st col of last row */
		cur_pos = cur_row * LPART(w).cols;
		item_y = LPART(w).InternalList[cur_pos]->CumHeight -
			 LPART(w).InternalList[top_pos]->CumHeight +
			 LPART(w).BaseY;
	}
	item_y += LPART(w).MaxItemHeight;
	if ((int)CPART(w).height <= (int)LPART(w).BaseY)
		avail_hi = 1;
	else
		avail_hi = CPART(w).height - LPART(w).BaseY;

	if (item_y < avail_hi)
	{
            XClearArea(
		XtDisplay(w), XtWindow(w),
		LPART(w).BaseX, item_y,
		viz_wd - LPART(w).BaseX, avail_hi - item_y, False);
	}

        if (LPART(w).Traversing)
        {
            if (LPART(w).CurrentKbdItem >= LPART(w).itemCount)
                LPART(w).CurrentKbdItem = LPART(w).itemCount - 1;

            DrawHighlight(w, LPART(w).CurrentKbdItem, TRUE);
        }
#else /* NOVELL */
   register XmListWidget lw = (XmListWidget) w;
   register int Current;
   Position ItemX,ItemY = 0, BaseY;
   int      Top, Num;
   register int  VizWidth;
   GC	    gc;
   
   if (!XtIsRealized(lw)) return;

   if (lw->list.items && lw->list.itemCount)
   {
       {
       /* don't allow underflow */
       int borders;
       borders = ( (int )lw->list.margin_width +
		  lw->list.HighlightThickness +
		  lw->primitive.shadow_thickness);
       if (lw->core.width <= borders)
	   VizWidth = 1;
       else
	   VizWidth = lw->core.width - borders;
       }

	ItemX = lw->list.BaseX - (lw->list.XOrigin);
	BaseY = (int )lw->list.margin_height + lw->list.HighlightThickness
	            + lw->primitive.shadow_thickness;
	lw->list.BaseY = BaseY;

	Top = lw->list.top_position;
	Num = Top + lw->list.visibleItemCount;
	if (Num > lw->list.itemCount) Num = lw->list.itemCount;
	for (Current = Top; Current < Num; Current++)
	{
	    ItemY = (lw->list.InternalList[Current]->CumHeight -
	 	     lw->list.InternalList[Top]->CumHeight) +
	             BaseY;

	    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
	    {
                ItemX = (VizWidth - lw->list.InternalList[Current]->width)
                         + lw->list.XOrigin;
	    }

            if (!all &&
	       (lw->list.InternalList[Current]->selected ==
	        lw->list.InternalList[Current]->LastTimeDrawn))
		break;


	    lw->list.InternalList[Current]->LastTimeDrawn =
    	        lw->list.InternalList[Current]->selected;

	    XFillRectangle (XtDisplay(w),
	    		XtWindow(w),
            		((lw->list.InternalList[Current]->selected) ?
                  	  lw->list.NormalGC : lw->list.InverseGC),
			lw->list.BaseX,
		 	ItemY,
			VizWidth,
			lw->list.MaxItemHeight);

#ifndef OSF_v1_2_4
            if (lw->core.sensitive)
#else /* OSF_v1_2_4 */
            if (XtIsSensitive(w))
#endif /* OSF_v1_2_4 */
	       gc = (lw->list.InternalList[Current]->selected) ?
                  	  lw->list.InverseGC : lw->list.NormalGC;
	    else
	       gc = lw->list.InsensitiveGC;
	       
/****************
 *
 * You notice that I'm forcing the string direction to L_TO_R, regardless
 * of the actual value. This is because the drawstring code tries to be
 * smart about but what I really want, but it just gets in the way. So, I
 * do my own calculations, and lie to the draw code.
 *
 ****************/
	    _XmStringDraw (XtDisplay(w),
	    			XtWindow(w),
				lw->list.font,
	    		     	lw->list.InternalList[Current]->name,
				gc,
			        ItemX,
				ItemY + ((lw->list.MaxItemHeight - 
					lw->list.InternalList[Current]->height)
					>> 1),
				VizWidth,
				XmALIGNMENT_BEGINNING,
				XmSTRING_DIRECTION_L_TO_R,
				NULL);
	}

	ItemY = ItemY + lw->list.MaxItemHeight;
        {
	/* don't allow underflow! */
	int available_height;
	if (lw->core.height <= (Dimension)BaseY) available_height = 1;
	else available_height = lw->core.height - BaseY;
	if (ItemY < available_height)
            XClearArea (XtDisplay (lw), XtWindow (lw), lw->list.BaseX, ItemY,
	            VizWidth - lw->list.BaseX, (available_height - ItemY), False);
        }
        if (w->list.Traversing)
        {
            if (w->list.CurrentKbdItem >= w->list.itemCount)
                w->list.CurrentKbdItem = w->list.itemCount - 1;
            DrawHighlight(w,w->list.CurrentKbdItem, TRUE);
        }
   }
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * DrawItem - Draw the specified item from the internal list.		*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawItem( w, position )
        Widget w ;
        int position ;
#else
DrawItem(
        Widget w,
        int position )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

	register int		item_x, item_y;
	int			borders, cum_wd, j, row, col, viz_wd;

	if (!XtIsRealized(w))
		return;

	row = position / (int)LPART(w).cols;

/* Shall this routine handle title item drawing? Currently, it won't */
	if (position >= LPART(w).itemCount	||
	    row <  TOP_ITEM_ROW(w)		||
	    row >= BOT_ITEM_ROW(w))
		return;

	if (LPART(w).InternalList[position]->selected ==
	    LPART(w).InternalList[position]->LastTimeDrawn)
		return;

	borders = LPART(w).margin_width +
		  LPART(w).HighlightThickness +
		  PPART(w).shadow_thickness;

	if ((int)CPART(w).width <= borders)
		return;	/* why viz_wd = 1? */

	viz_wd = CPART(w).width - borders;

	LPART(w).InternalList[position]->LastTimeDrawn =
    	    LPART(w).InternalList[position]->selected;

	col = position % (int)LPART(w).cols;

	item_x = LPART(w).BaseX - LPART(w).XOrigin;

	for (j = 0; j < col; j++)
		item_x += LPART(w).max_col_width[j] +
			  LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;

#define TOP_POS		LPART(w).top_position * LPART(w).cols

	item_y = LPART(w).InternalList[position]->CumHeight -
		 LPART(w).InternalList[TOP_POS + col]->CumHeight +
		 LPART(w).BaseY;

#undef TOP_POS

	cum_wd = LPART(w).BaseX + LPART(w).max_col_width[0];
	for (j = 1; j < col+1; j++)	/* col + 1 columns */
		cum_wd += LPART(w).max_col_width[j] +
			  LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;

	ItemDraw(w, True, item_x, item_y,
				position, col, viz_wd, cum_wd, borders);

#else /* NOVELL */
    register XmListWidget lw = (XmListWidget) w;
    register Position x, y;
    register Dimension VizWidth;
    GC       gc;

    if (!XtIsRealized(lw)) return;

    if ((position >= lw->list.itemCount)  ||
        (position < lw->list.top_position)||
	(position >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;

    if (lw->list.InternalList[position]->selected ==
	lw->list.InternalList[position]->LastTimeDrawn)
	return;


    VizWidth = lw->core.width +  - ((int )lw->list.margin_width +
				 lw->list.HighlightThickness +
			         lw->primitive.shadow_thickness);


    x = lw->list.BaseX - lw->list.XOrigin;

    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
    {
        x = (VizWidth - lw->list.InternalList[position]->width)
                 + lw->list.XOrigin;
    }


    lw->list.InternalList[position]->LastTimeDrawn =
    	    lw->list.InternalList[position]->selected;

    y = (lw->list.InternalList[position]->CumHeight -
	 lw->list.InternalList[lw->list.top_position]->CumHeight) +
	 lw->list.BaseY;

    XFillRectangle (XtDisplay(lw),
	    	XtWindow(lw),
		((lw->list.InternalList[position]->selected)
                  ? lw->list.NormalGC : lw->list.InverseGC),
		lw->list.BaseX,
		y,
		VizWidth,
	 	lw->list.MaxItemHeight);

#ifndef OSF_v1_2_4
     if (lw->core.sensitive)
#else /* OSF_v1_2_4 */
     if (XtIsSensitive(w))
#endif /* OSF_v1_2_4 */
         gc = (lw->list.InternalList[position]->selected) ?
               	  lw->list.InverseGC : lw->list.NormalGC;
     else
         gc = lw->list.InsensitiveGC;

    _XmStringDraw(XtDisplay(lw),
    			XtWindow(lw),
			lw->list.font,
    		     	lw->list.InternalList[position]->name,
			gc,
		        x,
			y + ((lw->list.MaxItemHeight - 
			      lw->list.InternalList[position]->height)
			      >> 1),
			VizWidth,
			XmALIGNMENT_BEGINNING,
			XmSTRING_DIRECTION_L_TO_R,
			NULL);

#endif /* NOVELL */
}

/************************************************************************
 *									*
 * DrawHighlight - Draw or clear the traversal highlight on an item.	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawHighlight( lw, position, on )
        XmListWidget lw ;
        int position ;
        Boolean on ;
#else
DrawHighlight(
        XmListWidget lw,
        int position,
#if NeedWidePrototypes
        int on )
#else
        Boolean on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	ht;
	int		x, y, width, height;

	if (!XtIsRealized((Widget)lw) || !LPART(lw).Traversing ||
	    (ht = LPART(lw).HighlightThickness) < 1)
		return;

/****************
 *
 * First check for an "invisible" highlight...
 *
 ****************/

	if (TITLE_ROW_ITEM(lw, position))
		return;

		/* width <= 0 can mean, the `row' is not in view and
		 * the row is in view but the item is not */
	if (CalcItemBound((Widget)lw, position, ht, (unsigned long *)NULL,
					&x, &y, &width, &height) <= 0)
	{	/* i.e., width <= 0 */

		int	static_rows_hi = 0;

		if (LPART(lw).static_rows)
		{
#define P	LPART(lw).static_rows * LPART(lw).cols

			static_rows_hi = LPART(lw).InternalList[P]->CumHeight -
					 LPART(lw).InternalList[0]->CumHeight;
#undef P
		}

		x = LPART(lw).BaseX - ht;
		y = LPART(lw).BaseY - ht + static_rows_hi;
		width = CPART(lw).width - 2 * ((int)LPART(lw).margin_width +
						PPART(lw).shadow_thickness);
		height = CPART(lw).height - static_rows_hi - 2 *
			 (LPART(lw).margin_height + PPART(lw).shadow_thickness);
	}

	if (width <=  0 || height <= 0)
		return;

	ht = PPART(lw).highlight_thickness;
	if (on) 
	{
		if (LPART(lw).AddMode)
			_XmDrawHighlight(
				XtDisplay((Widget)lw), XtWindow((Widget)lw), 
				LPART(lw).HighlightGC,
				x, y, width, height, ht,
				LineOnOffDash);
		else
			_XmDrawSimpleHighlight(
				XtDisplay((Widget)lw), XtWindow((Widget)lw), 
				LPART(lw).HighlightGC, 
				x, y, width, height, ht);
	}
	else 
	{
		_XmClearBorder(XtDisplay((Widget)lw), XtWindow((Widget)lw), 
						x, y, width, height, ht);
	}
#else /* NOVELL */
    register Dimension  width, height, ht;
    register Position   x,y;

    if (!XtIsRealized(lw)) return;
    if (!lw->list.Traversing) return;

    ht = lw->list.HighlightThickness;
    if (ht < 1) return;
    x = lw->list.BaseX - ht;
    width = lw->core.width - 2 * ((int )lw->list.margin_width +
			         lw->primitive.shadow_thickness);

/****************
 *
 * First check for an "invisible" highlight...
 *
 ****************/
    if ((position < lw->list.top_position) ||
	(lw->list.items == NULL)           ||
	(lw->list.itemCount == 0)          ||
        (position >= (lw->list.top_position + lw->list.visibleItemCount)))
    {
	y = lw->list.BaseY - ht;
	height = lw->core.height - 2 * ((int )lw->list.margin_height +
 					      lw->primitive.shadow_thickness);
    }
    else
    {
        if (position >= lw->list.itemCount )
	    position = lw->list.itemCount - 1;
        y = (lw->list.InternalList[position]->CumHeight -
	     lw->list.InternalList[lw->list.top_position]->CumHeight) +
	     lw->list.BaseY - ht;
        height = lw->list.MaxItemHeight + (2 * ht);
    }

    if (width <=  0 || height <= 0) return;

    ht = lw->primitive.highlight_thickness;
    if (on) 
    {
 	if (lw->list.AddMode)
	    _XmDrawHighlight(XtDisplay (lw), XtWindow (lw), 
			     lw->list.HighlightGC, 
			     x, y, width, height, ht,
			     LineOnOffDash);
	else
	    _XmDrawSimpleHighlight(XtDisplay (lw), XtWindow (lw), 
				 lw->list.HighlightGC, 
				 x, y, width, height, ht);
      } 
     else 
     {
 	_XmClearBorder(XtDisplay (lw), XtWindow (lw), 
 		       x, y, width, height, ht);
      }
#endif /* NOVELL */
#if FALSE
   if (on)
   {
	gc = lw->list.HighlightGC;
	rect[0].x = x;
	rect[0].y = y;
	rect[0].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[0].height = ht;

	rect[1].x = x;
	rect[1].y = y;
	rect[1].width = ht;
	rect[1].height = height;

	rect[2].x = x + ((width <= ht) ? 1 : width - ht); /* stop underflow */
	rect[2].y = y;
	rect[2].width = ht;
	rect[2].height = height;

	rect[3].x = x;
	rect[3].y = y + ((height <= ht) ? 1 : height - ht); /*stop underflow*/
	rect[3].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[3].height = ht;

/****************
 *
 * Draw the highlight. If not in add mode, just fill the rectangles. If
 * in add mode, we have to move the tiling origin so that the dashing
 * looks consistent throughout the window. Could also do the highlight
 * with wide lines, but odd highlight thicknesses make the endpoint
 * calculations hard...
 *
 ****************/
        if (lw->list.AddMode)
        {
            XSetTSOrigin (XtDisplay (lw), gc, rect[0].x, rect[0].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 2);
            XSetTSOrigin (XtDisplay (lw), gc, rect[2].x , rect[2].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[2], 1);
            XSetTSOrigin (XtDisplay (lw), gc, rect[3].x , rect[3].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[3], 1);
        }
        else
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 4);
    }
    else
    {

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, width, ht, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x + width - ht, y,
	            ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y+ height - ht,
	            width, ht, False);
    }
#endif
}

/************************************************************************
 *									*
 * SetClipRect - set a clipping rectangle for the visible area of the	*
 * list.								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetClipRect( widget )
        XmListWidget widget ;
#else
SetClipRect(
        XmListWidget widget )
#endif /* _NO_PROTO */
{
    register XmListWidget lw = widget;
#ifndef NOVELL
    register Position x,y, ht;
#else /* NOVELL */
    register Position x,y, ht, ht2;
#endif /* NOVELL */
    Dimension w,h;
    XRectangle rect;

#ifndef NOVELL
    ht = lw->list.HighlightThickness;

    x =  lw->list.margin_width + ht + lw->primitive.shadow_thickness;

    y =  lw->list.margin_height + ht + lw->primitive.shadow_thickness;

    /* make sure these values don't underflow! */
    if (lw->core.width <= 2*x) w = 1;
    else w = lw->core.width - (2 * x);
    if (lw->core.height <= 2*y) h = 1;
    else h = lw->core.height - (2 * y);

#else /* NOVELL */
    if (!LPART(lw).static_rows) {
	ht  = LPART(lw).HighlightThickness;
	ht2 = 0;
    }
    else {
	ht  = LPART(lw).HighlightThickness - PPART(lw).highlight_thickness;
	ht2 = LPART(lw).HighlightThickness;
    }

    x =  LPART(lw).margin_width + ht + PPART(lw).shadow_thickness;

    y =  LPART(lw).margin_height + ht + PPART(lw).shadow_thickness;

    /* make sure these values don't underflow! */
	/* fix warning messages */
    if ((int)lw->core.width <= 2*x - ht2) w = 1;
    else w = lw->core.width - (2 * x) + ht2;
    if ((int)lw->core.height <= 2*y - ht2) h = 1;
    else h = lw->core.height - (2 * y) + ht2;

#endif /* NOVELL */

    rect.x = 0;
    rect.y = 0;
    rect.width = w;
    rect.height = h;
    if (lw->list.NormalGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.NormalGC, x, y,
			   &rect, 1, Unsorted);
    if (lw->list.InverseGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.InverseGC, x, y,
			   &rect, 1, Unsorted);

    if (lw->list.InsensitiveGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.InsensitiveGC, x, y,
			   &rect, 1, Unsorted);

    if (lw->list.HighlightGC && ht)
    {
	x -= ht;
	y -= ht;
        rect.width += (2 * ht);
	rect.height += (2 * ht);
        XSetClipRectangles(XtDisplay(lw), lw->list.HighlightGC, x, y,
			   &rect, 1, Unsorted);
    }


}


/***************************************************************************
 *									   *
 * SetDefaultSize(lw,&width, &height)					   *
 * Take an instance of the list widget and figure out how big the list	   *
 * work area should be. This uses the last set visible item count, because *
 * that's really what we want for a default.                               *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
SetDefaultSize( lw, width, height )
        XmListWidget lw ;
        Dimension *width ;
        Dimension *height ;
#else
SetDefaultSize(
        XmListWidget lw,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int		vis_hi, wide_border, top, viz;

	wide_border =  2 * (PPART(lw).shadow_thickness +
			    LPART(lw).HighlightThickness +
			    LPART(lw).margin_width);

	top = LPART(lw).top_position;
/* BEGIN OSF Fix CR 5460, 6014 */
	if (LPART(lw).LastSetVizCount)
		viz =  LPART(lw).LastSetVizCount;
	else
		viz =  LPART(lw).visibleItemCount;
/* END OSF Fix CR 5460, 6014 */

/****************
 *
 * Figure out my default height. This is needlessly complex due to these
 * funky strings. If we have items, and if there are the full number of
 * items visible, then just use the cum height of the bottom item.
 * Else figure out the space taken by the
 * first visible items, divide by the number of lines if needed, and call
 * that the average height of the remaining items. It's a guess, but it
 * will be right for any single-font list.
 *
 ****************/

	if (LPART(lw).InternalList && LPART(lw).itemCount)
	{
		SetMaxHeight(lw);
			/* Just based `viz' and CalcCumHeight() to
			 * figure out the total height. There is no
			 * need to check like used to be. */
		vis_hi = viz * LPART(lw).MaxItemHeight +
			 (viz - 1) * LPART(lw).spacing;
	}
	else				/* No items - take a guess at height */
	{
		XFontStruct *	fs = (XFontStruct *)NULL;

		_XmFontListGetDefaultFont(LPART(lw).font, &fs);
		if (fs)
			vis_hi = viz * (fs->ascent + fs->descent +
					   LPART(lw).spacing);
		else
			vis_hi = LPART(lw).spacing ?
					LPART(lw).spacing * viz : 1;
	
		if (viz > 1)
			vis_hi -= LPART(lw).spacing;
	}

	*height = vis_hi + 2 * (PPART(lw).shadow_thickness +
				LPART(lw).HighlightThickness +
				LPART(lw).margin_height);

	if (LPART(lw).InternalList)
		SetMaxWidth(lw);
	else
		LPART(lw).MaxWidth = vis_hi >> 1;

	if (LPART(lw).InternalList || !XtIsRealized((Widget)lw))
		*width = LPART(lw).MaxWidth + wide_border;
	else  /* If no list, but realized, */
		*width = CPART(lw).width;  /* stay the same width. */

#else /* NOVELL */
    int	i, visheight, lineheight, wideborder, top, viz;
    XFontStruct *fs = (XFontStruct *)NULL;

    wideborder =  2 * (lw->primitive.shadow_thickness +
	               lw->list.HighlightThickness +
		       lw->list.margin_width);

    top = lw->list.top_position;
/* BEGIN OSF Fix CR 5460, 6014 */
    if (lw->list.LastSetVizCount) viz = lw->list.LastSetVizCount;
    else viz = lw->list.visibleItemCount;
/* END OSF Fix CR 5460, 6014 */

/****************
 *
 * Figure out my default height. This is needlessly complex due to these
 * funky strings. If we have items, and if there are the full number of
 * items visible, then just use the cum height of the bottom item.
 * Else figure out the space taken by the
 * first visible items, divide by the number of lines if needed, and call
 * that the average height of the remaining items. It's a guess, but it
 * will be right for any single-font list.
 *
 ****************/

    if (lw->list.InternalList  && lw->list.itemCount)
    {
    	SetMaxHeight(lw);
	
	if ((top + viz) <= lw->list.itemCount)
	    visheight = (lw->list.InternalList[top + viz - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight) +
			lw->list.MaxItemHeight;
        else
	{
	    lineheight = lw->list.MaxItemHeight;
	    i = viz - (lw->list.itemCount - top);
	    visheight = ((lw->list.InternalList[lw->list.itemCount - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight)) +
			 lw->list.MaxItemHeight +
			((lw->list.spacing + lineheight) * i);
	}
    }
    else				/* No items - take a guess at height */
    {
        _XmFontListGetDefaultFont(lw->list.font, &fs);
	if (fs)
	  visheight = (fs->ascent + fs->descent + lw->list.spacing) * viz;
	else visheight = lw->list.spacing ? (lw->list.spacing * viz) : 1;
	
        if (viz > 1)
            visheight -= lw->list.spacing;
    }
   *height = visheight +
	      (2 * (lw->primitive.shadow_thickness +
		    lw->list.HighlightThickness +
		    lw->list.margin_height));

    if (lw->list.InternalList)
        SetMaxWidth(lw);
    else
        lw->list.MaxWidth = visheight >> 1;

    if ((lw->list.InternalList) || (!XtIsRealized(lw)))
        *width = lw->list.MaxWidth + wideborder;
    else                                        /* If no list, but realized, */
        *width = lw->core.width;                /* stay the same width.      */
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * MakeGC - Get the GC's for normal and inverse.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
MakeGC( lw )
        XmListWidget lw ;
#else
MakeGC(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    XGCValues	 values;
    XtGCMask  valueMask;
    XFontStruct *fs = (XFontStruct *) NULL;
    Drawable drawable;

    valueMask = GCForeground | GCBackground | GCFont ;
    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.NormalGC);

    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.InverseGC);

    if (lw->list.InsensitiveGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.InsensitiveGC);
/****************
 *
 * This is sloppy - get the default font and use it for the GC. The
 * StringDraw routines will change it if needed.
 *
 ****************/
    _XmFontListGetDefaultFont(lw->list.font, &fs);

    values.foreground	= lw->primitive.foreground;
    values.background   = lw->core.background_pixel;
    if (!fs)
        valueMask &= ~GCFont;
    else
        values.font  = fs->fid;

    /* construct a drawable of this depth */
    if (DefaultDepthOfScreen(XtScreen(lw)) == lw->core.depth)
            drawable = RootWindowOfScreen(XtScreen(lw));
    else
            drawable = XCreatePixmap(DisplayOfScreen(XtScreen(lw)),
                    XtScreen(lw)->root, 1, 1, lw->core.depth);

    lw->list.NormalGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);

    values.foreground	= lw->core.background_pixel;
    values.background   = lw->primitive.foreground;

    lw->list.InverseGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);
    valueMask |= GCTile | GCFillStyle;
    values.tile = XmGetPixmapByDepth (XtScreen((Widget)(lw)), "50_foreground",
		lw->primitive.foreground, lw->core.background_pixel,
		lw->core.depth);
    values.fill_style = FillTiled;
    
    lw->list.InsensitiveGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);

    if (RootWindowOfScreen(XtScreen(lw)) != drawable)
            XFreePixmap(XtDisplay(lw), drawable);
}


/************************************************************************
 *									*
 *  MakeHighlightGC - Get the graphics context used for drawing the	*
 *                    highlight border. I have to use my own because I  *
 *		      need to set a clip rectangle, and I don't want to *
 *		      do that on the standard one (it's cached and 	*
 *		      shared among instances - that's the same reason I *
 *		      have to use the X calls.)				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
MakeHighlightGC( lw, AddMode )
        XmListWidget lw ;
        Boolean AddMode ;
#else
MakeHighlightGC(
        XmListWidget lw,
#if NeedWidePrototypes
        int AddMode )
#else
        Boolean AddMode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XtGCMask  valueMask;
    XGCValues values;
    Drawable drawable;

    valueMask = GCForeground | GCBackground | GCLineWidth 
		| GCLineStyle | GCDashList;
    values.foreground = lw->primitive.highlight_color;
    values.background = lw->core.background_pixel;
    values.line_width = lw->primitive.highlight_thickness;
#ifndef OSF_v1_2_4
    values.dashes = (values.line_width > 8)?values.line_width : 8 ;
#else /* OSF_v1_2_4 */
    values.dashes = MAX(values.line_width, 8);
#endif /* OSF_v1_2_4 */
  
    values.line_style = (AddMode) ? LineOnOffDash : LineSolid ;

    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.HighlightGC);

    /* construct a drawable of this depth */
    if (DefaultDepthOfScreen(XtScreen(lw)) == lw->core.depth)
            drawable = RootWindowOfScreen(XtScreen(lw));
    else
            drawable = XCreatePixmap(DisplayOfScreen(XtScreen(lw)),
                    XtScreen(lw)->root, 1, 1, lw->core.depth);

    lw->list.HighlightGC = XCreateGC (XtDisplay(lw),
    				      drawable,
    				      valueMask, &values);
}

/************************************************************************
 *                                                                      *
 * ChangeHighlightGC - change the highlight GC for add mode.  If        *
 * AddMode is true, change the fill style to dashed.  Else set the     *
 * fill style to solid                                                  *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ChangeHighlightGC( lw, AddMode )
        XmListWidget lw ;
        Boolean AddMode ;
#else
ChangeHighlightGC(
        XmListWidget lw,
#if NeedWidePrototypes
        int AddMode )
#else
        Boolean AddMode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XtGCMask  valueMask;
    XGCValues values;
/****************
    valueMask = GCFillStyle;
    values.fill_style = (AddMode) ? FillTiled : FillSolid;
 ****************/
     valueMask = GCLineStyle | GCLineWidth | GCDashList;
     values.line_width = lw->primitive.highlight_thickness;
#ifndef OSF_v1_2_4
     values.dashes = (values.line_width > 8)?values.line_width : 8 ;
#else /* OSF_v1_2_4 */
     values.dashes = MAX(values.line_width, 8);
#endif /* OSF_v1_2_4 */
     values.line_style = (AddMode) ? LineOnOffDash : LineSolid ;

    if (lw->list.HighlightGC)
        XChangeGC (XtDisplay(lw), lw->list.HighlightGC, valueMask, &values);

}
/************************************************************************
 *									*
 * SetVerticalScrollbar - set up all the vertical scrollbar stuff.	*
 *									*
 * Set up on an item basis. Min is 0, max is ItemCount, origin is	*
 * top_position, extent is visibleItemCount.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetVerticalScrollbar( lw )
        XmListWidget lw ;
#else
SetVerticalScrollbar(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	i, viz;
	Boolean	flag;
	Arg	args[5];
	int	rows, ignore;

	if (!LPART(lw).Mom || !LPART(lw).vScrollBar || LPART(lw).FromSetSB)
		return;

	LPART(lw).FromSetSB = True;

	CalcNumRows((Widget)lw, &rows, &ignore);

	viz = SetVizCount(lw);

	if (lw->list.ScrollBarDisplayPolicy != XmAS_NEEDED)
		flag = True;
	else if ((rows <= viz && !LPART(lw).top_position) ||
		 !LPART(lw).itemCount)
		flag = False;
	else
		flag = True;

	if (flag)
		XtManageChild((Widget)LPART(lw).vScrollBar);
	else
		XtUnmanageChild((Widget)LPART(lw).vScrollBar);

	i = 0;
	flag = False;
	if (LPART(lw).itemCount)
	{
		LPART(lw).vmax = rows;

		XtSetArg(args[i], XmNmaximum, (XtArgVal)LPART(lw).vmax); i++;

		LPART(lw).vOrigin = LPART(lw).top_position;
		XtSetArg(args[i], XmNvalue, (XtArgVal)LPART(lw).vOrigin); i++;

		LPART(lw).vExtent = LPART(lw).visibleItemCount < rows ?
					LPART(lw).visibleItemCount : rows;
		if (LPART(lw).vExtent + LPART(lw).vOrigin > LPART(lw).vmax)
			LPART(lw).vExtent = LPART(lw).vmax - LPART(lw).vOrigin;

		XtSetArg(args[i], XmNsliderSize, (XtArgVal)LPART(lw).vExtent);
				i++;
		XtSetArg(args[i], XmNincrement, (XtArgVal)1); i++;
		flag = True;
	}
	else if (XtIsManaged((Widget)LPART(lw).vScrollBar))
        {
		XtSetArg(args[i], XmNmaximum, (XtArgVal)1); i++;
		XtSetArg(args[i], XmNvalue, (XtArgVal)0); i++;
		XtSetArg(args[i], XmNsliderSize, (XtArgVal)1); i++;
		XtSetArg(args[i], XmNincrement, (XtArgVal)1); i++;
		flag = True;
        }
	if (flag)
	{
		int	pg_inc;

		pg_inc = LPART(lw).visibleItemCount > 1 ?
					LPART(lw).visibleItemCount - 1 : 1;
		XtSetArg(args[i], XmNpageIncrement, (XtArgVal)pg_inc); i++;
		XtSetValues((Widget)LPART(lw).vScrollBar, args, i);
	}

	LPART(lw).FromSetSB = False;

#else /* NOVELL */
    int i, viz;

    i = 0;

    if ((!lw->list.Mom)        ||
        (!lw->list.vScrollBar) ||
	(lw->list.FromSetSB))
        return;

    lw->list.FromSetSB = TRUE;
    viz = SetVizCount(lw);
    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (((lw->list.itemCount <= viz) && (lw->list.top_position == 0)) ||
             (lw->list.itemCount == 0))
	        XtUnmanageChild((Widget) lw->list.vScrollBar);
	else
	    XtManageChild((Widget) lw->list.vScrollBar);
    else
	XtManageChild((Widget) lw->list.vScrollBar);

    if (lw->list.items && lw->list.itemCount)
    {
        lw->list.vmax =  lw->list.itemCount;
        XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) (lw->list.vmax)); i++;

        lw -> list.vOrigin = lw->list.top_position;
        XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) lw->list.vOrigin); i++;

#ifndef OSF_v1_2_4
        lw->list.vExtent = (lw->list.visibleItemCount < lw->list.itemCount) ?
      			    lw->list.visibleItemCount : lw->list.itemCount;
#else /* OSF_v1_2_4 */
        lw->list.vExtent = MIN(lw->list.visibleItemCount, lw->list.itemCount);
#endif /* OSF_v1_2_4 */
        if ((lw->list.vExtent + lw->list.vOrigin) > lw->list.vmax)
            lw->list.vExtent = lw->list.vmax - lw->list.vOrigin;
        XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) (lw->list.vExtent)); i++;
        XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
        XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;
        XtSetValues((Widget) lw->list.vScrollBar, vSBArgs, i);
    }
    else
        if (XtIsManaged(lw->list.vScrollBar))
        {
            XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) 0); i++;
            XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;

            XtSetValues((Widget) lw->list.vScrollBar, vSBArgs, i);
        }
    lw->list.FromSetSB = FALSE;
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * SetHorizontalScrollbar - set up all the horizontal scrollbar stuff.	*
 *									*
 * This is set up differently than the vertical scrollbar. This is on a *
 * pixel basis, so redraws are kinda slow. Min is 0, max is (MaxWidth   *
 * + 2*border).								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetHorizontalScrollbar( lw )
        XmListWidget lw ;
#else
SetHorizontalScrollbar(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int		j, list_wd;
	Boolean		flag;
	Dimension	pg_inc;
	Arg		args[6];

	/* My code already fixed the problem below! */
/*
 * Fix for CR 5701 - Check to make sure that the flag FromSetSB has not
 *                   already been set before executing program.
 */
	if (!LPART(lw).Mom || !LPART(lw).hScrollBar || LPART(lw).FromSetSB)
		return;
/*
 * End fix for CR 5701
 */

	LPART(lw).FromSetSB = True;

		/* Should consider the VSB/HSB dimensions! */
	list_wd = CPART(lw).width - 2 * (int)(LPART(lw).margin_width +
					      LPART(lw).HighlightThickness +
					      PPART(lw).shadow_thickness);

	SetMaxWidth(lw);

	if (LPART(lw).ScrollBarDisplayPolicy != XmAS_NEEDED)
		flag = True;
	else if ((int)LPART(lw).MaxWidth > list_wd)
		flag = True;
	else {

		LPART(lw).BaseX = (int)LPART(lw).margin_width +
				       LPART(lw).HighlightThickness +
				       PPART(lw).shadow_thickness;

		LPART(lw).XOrigin = 0;
		flag = False;
	}

	if (flag)
		XtManageChild((Widget)LPART(lw).hScrollBar);
	else
		XtUnmanageChild((Widget)LPART(lw).hScrollBar);


	j = 0;
	flag = False;
	if (LPART(lw).itemCount)
	{
		if (LPART(lw).StrDir == XmSTRING_DIRECTION_R_TO_L)
		{
			XtSetArg(args[j], XmNprocessingDirection,
					(XtArgVal)XmMAX_ON_LEFT);
		}
		else
		{
			XtSetArg(args[j], XmNprocessingDirection,
					(XtArgVal)XmMAX_ON_RIGHT);
		}
		j++;
		LPART(lw).hmax = LPART(lw).MaxWidth + 2 * LPART(lw).BaseX;
		XtSetArg(args[j], XmNmaximum, (XtArgVal)LPART(lw).hmax); j++;
		if (LPART(lw).XOrigin < 0)
			LPART(lw).XOrigin = 0;
		LPART(lw).hOrigin = LPART(lw).XOrigin;
		XtSetArg(args[j], XmNvalue, (XtArgVal)LPART(lw).hOrigin); j++;
		LPART(lw).hExtent = CPART(lw).width;
		if (LPART(lw).hExtent + LPART(lw).hOrigin > LPART(lw).hmax)
			LPART(lw).hExtent = LPART(lw).hmax - LPART(lw).hOrigin;
		XtSetArg (args[j], XmNsliderSize, (XtArgVal)LPART(lw).hExtent);
		j++;

/****************
 *
 * What should the inc be??
 *
 ****************/
		pg_inc = (list_wd <= CHAR_WIDTH_GUESS) ? 1
                                             : (list_wd - CHAR_WIDTH_GUESS) ;
		if (pg_inc > CPART(lw).width)
			pg_inc = 1;
		XtSetArg(args[j], XmNincrement, (XtArgVal)CHAR_WIDTH_GUESS);j++;
		XtSetArg(args[j], XmNpageIncrement, (XtArgVal)pg_inc); j++;
		flag = True;
	}
	else if (XtIsManaged((Widget)LPART(lw).hScrollBar))
	{
		XtSetArg(args[j], XmNmaximum, (XtArgVal)1); j++;
		XtSetArg(args[j], XmNvalue, (XtArgVal)0); j++;
		XtSetArg(args[j], XmNsliderSize, (XtArgVal)1); j++;
		XtSetArg(args[j], XmNincrement, (XtArgVal)1); j++;
		flag = True;

	}

	if (flag)
		XtSetValues((Widget)LPART(lw).hScrollBar, args, j);

	LPART(lw).FromSetSB = False;

#else /* NOVELL */
    int j, listwidth;
    Dimension pginc;

    j = 0;

/*
 * Fix for CR 5701 - Check to make sure that the flag FromSetSB has not
 *                   already been set before executing program.
 */
    if ((!lw->list.Mom)        ||
        (!lw->list.hScrollBar) ||
        (lw->list.FromSetSB))
        return;
/*
 * End fix for CR 5701
 */
    lw->list.FromSetSB = TRUE;

    listwidth = lw->core.width - 2 * (int )(lw->list.margin_width +
			                    lw->list.HighlightThickness +
			                    lw->primitive.shadow_thickness);

    SetMaxWidth(lw);

    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (lw->list.MaxWidth <= listwidth)
	{
	    lw->list.BaseX = (int ) lw->list.margin_width +
			            lw->list.HighlightThickness +
			            lw->primitive.shadow_thickness;

	    lw->list.XOrigin = 0;
	    XtUnmanageChild((Widget) lw->list.hScrollBar);
	}
	else
	    XtManageChild((Widget) lw->list.hScrollBar);
    else
	XtManageChild((Widget) lw->list.hScrollBar);


    if (lw->list.items && lw->list.itemCount)
    {
        if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                     (XtArgVal) XmMAX_ON_LEFT); j++;
        }
        else
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                      (XtArgVal) XmMAX_ON_RIGHT); j++;
        }
        lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
        XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;
        if (lw->list.XOrigin < 0)
            lw->list.XOrigin = 0;
        lw -> list.hOrigin = lw->list.XOrigin;
        XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;
        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
            lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
        XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What should the inc be??
 *
 ****************/
        pginc = (listwidth <= CHAR_WIDTH_GUESS) ? 1
                                             : (listwidth - CHAR_WIDTH_GUESS) ;
        if (pginc > lw->core.width) pginc = 1;
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) pginc); j++;
        XtSetValues((Widget) lw->list.hScrollBar, hSBArgs, j);
    }
    else
        if (XtIsManaged((Widget) lw->list.hScrollBar))
        {
            XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) 0); j++;
            XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) 1); j++;
            XtSetValues((Widget) lw->list.hScrollBar, hSBArgs, j);

        }

    lw->list.FromSetSB = FALSE;
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * SetMaxWidth - scan the list and get the width in pixels of the	*
 * largest element.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMaxWidth( lw )
        XmListWidget lw ;
#else
SetMaxWidth(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	rows, last_row_size;
	int	i, j, k;

	if (!lw->list.itemCount)
		return;

	CalcNumRows((Widget)lw, &rows, &last_row_size);

	for (i = 0; i < (int)LPART(lw).cols; i++)
	{
		LPART(lw).max_col_width[i] = 0;
		for (j = 0; j < rows; j++)
		{
				/* we don't need to continue if we located
				 * an empty column in the last row */
			if (EmptyColInLastRow(j, rows, last_row_size, i))
				break;

			k = i + j * LPART(lw).cols;

			if (LPART(lw).InternalList[k]->width >
						LPART(lw).max_col_width[i])
				LPART(lw).max_col_width[i] =
					LPART(lw).InternalList[k]->width;
		}
	}

		/* We are going to overload MaxWidth here, this field will
		 * still contain total width, in the case of single column,
		 * max_col_width[0] is MaxWidth (see Initialize), so we
		 * don't need do anything, otherwise, add them together. */
	if (LPART(lw).cols > 1)
	{
		LPART(lw).MaxWidth = 0;
		for (i = 0; i < (int)LPART(lw).cols; i++)
			LPART(lw).MaxWidth += LPART(lw).max_col_width[i];

			/* Now include col_spacing and highlight_thickness*/
		LPART(lw).MaxWidth += (LPART(lw).cols - 1) *
					(LPART(lw).col_spacing +
					 2 * LPART(lw).HighlightThickness);
	}

#else /* NOVELL */
    int  maxwidth, len;
    register int i;

    if (!lw->list.itemCount) return;

    for (i = 0, maxwidth = 0; i < lw->list.itemCount; i++)
    {
	len = lw->list.InternalList[i]->width;
	if (maxwidth < len) maxwidth = len;
    }
    lw->list.MaxWidth = maxwidth;
#endif /* NOVELL */
}
/************************************************************************
 *									*
 * SetMaxHeight - scan the list and get the height in pixels of the	*
 * largest element.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMaxHeight( lw )
        XmListWidget lw ;
#else
SetMaxHeight(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int		max_hi;
	register int	i;

	if (!LPART(lw).itemCount)
	{
		LPART(lw).MaxItemHeight = 0;
		return;
	}

	for (i = 0, max_hi = 0; i < LPART(lw).itemCount; i++)
	{
#define HI	(int)LPART(lw).InternalList[i]->height

		if (max_hi < HI)
			max_hi = HI;
#undef HI
	}

	if (max_hi != LPART(lw).MaxItemHeight)
	{
		LPART(lw).MaxItemHeight = max_hi;
		CalcCumHeight((Widget)lw);
	}
#else /* NOVELL */
    int  maxheight, height;
    register int i;

    if (!lw->list.itemCount) return;

    for (i = 0, maxheight = 0; i < lw->list.itemCount; i++)
    {
	height = lw->list.InternalList[i]->height;
	if (maxheight < height) maxheight = height;
    }
    if (maxheight != lw->list.MaxItemHeight)
    {
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (i = 1; i < lw->list.itemCount; i++)
	    lw->list.InternalList[i]->CumHeight = lw->list.InternalList[i-1]->CumHeight +
						  maxheight +
						  lw->list.spacing;
    }
    lw->list.MaxItemHeight = maxheight;
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * SetNewSize - see if we need a new size.  If so, do it.  If the	*
 * current item count is different from the desired count, calc a new	*
 * height and width.  Else just look at the width and change if needed.	*
 *                                                                      *
 * NOTE: THIS CAN ONLY BE CALLED FROM THE API ROUTINES, SINCE IT USES   *
 * SETVALUES.                                                           *
 *									*
 ************************************************************************/
#ifdef NOVELL
static Boolean 
#else /* NOVELL */
static void 
#endif /* NOVELL */
#ifdef _NO_PROTO
SetNewSize( lw )
        XmListWidget lw ;
#else
SetNewSize(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	Dimension	width, height;
	Boolean		size_changed = False;

		/* If we are in a scrolled window, then the
		 * actual window size won't change, because
		 * we are dealing with virtual window */
	if (LPART(lw).Mom)
	{
		/*
 		 * if a larger font or multiple font strings are being used
		 * this will update the visible item count to the
		 * new value
		 */
		ListSetValuesAlmost(0, (Widget)lw, 0, 0);
		return(size_changed);
	}

	LPART(lw).FromSetNewSize = True;
	SetDefaultSize(lw, &width, &height);

	if (LPART(lw).SizePolicy == XmCONSTANT)
		width = CPART(lw).width;

	if (width != CPART(lw).width || height != CPART(lw).height)
	{
		Arg		args[2];
		unsigned char	units;

		size_changed = True;

			/* Should override the value in initialize() and
			 * set_values() if we only care about XmPIXELS */
		units = PPART(lw).unit_type;
		PPART(lw).unit_type = XmPIXELS;
		XtSetArg(args[0], XmNwidth, width);
		XtSetArg(args[1], XmNheight, height);
		XtSetValues((Widget)lw, args, 2);
		PPART(lw).unit_type = units;
	}
	LPART(lw).FromSetNewSize = False;

	return(size_changed);
#else /* NOVELL */
    Dimension width, height;
    unsigned char units;

    lw->list.FromSetNewSize = TRUE;
    SetDefaultSize(lw,&width, &height);

    if (lw->list.SizePolicy == XmCONSTANT)
        width = lw->core.width;

    if ((width != lw->core.width) ||
        (height != lw->core.height))
    {
        units = lw->primitive.unit_type;
        lw->primitive.unit_type = XmPIXELS;
        XtSetArg (vSBArgs[0], XmNwidth,(XtArgVal) width);
        XtSetArg (vSBArgs[1], XmNheight,(XtArgVal) height);
        XtSetValues((Widget) lw, vSBArgs, 2);
        lw->primitive.unit_type = units;
    }
    lw->list.FromSetNewSize = FALSE;
#endif /* NOVELL */
}

#ifndef NOVELL
/************************************************************************
 *									*
 * ResetHeight - recalculate the cumulative heights of the list.	*
 * Called when the font or spacing changes.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ResetHeight( lw )
        XmListWidget lw ;
#else
ResetHeight(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    Dimension maxheight = 0, height;
    if (lw->list.InternalList && lw->list.itemCount)
    {
	for (i = 0; i < lw->list.itemCount; i++)
	{
            height = _XmStringHeight(lw->list.font,lw->list.InternalList[i]->name);
	    lw->list.InternalList[i]->height = height;
	    if (maxheight < height) maxheight = height;
	}
	lw->list.MaxItemHeight = maxheight;
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (i = 1; i < lw->list.itemCount; i++)
	{
	    lw->list.InternalList[i]->CumHeight = maxheight +
						  lw->list.InternalList[i-1]->CumHeight +
						  lw->list.spacing;
	}
    }
}

/************************************************************************
 *									*
 * ResetWidth - recalculate the widths of the list elements.		*
 * Called when the font changes.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ResetWidth( lw )
        XmListWidget lw ;
#else
ResetWidth(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    if (lw->list.InternalList && lw->list.itemCount)
    {
	for (i = 0; i < lw->list.itemCount; i++)
            lw->list.InternalList[i]->width = _XmStringWidth(lw->list.font,
					          lw->list.InternalList[i]->name);
    }
}
#endif /* NOVELL */


/************************************************************************
 *									*
 * Item/Element Manupulation routines					*
 *									*
 ************************************************************************/

/* BEGIN OSF Fix CR 4656 */
/************************************************************************
 *									*
 * FixStartEnd - reset the (Old)StartItem and (Old)EndItem		*
 *	instance variables after a delection based on the postion of 	*
 * 	the item deleted.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
FixStartEnd(pos, start, end)
     int pos;
     int *start;
     int *end;
#else
FixStartEnd(int pos, int *start, int *end)
#endif /* _NO_PROTO */
{
  /* Beyond selected range, no effect. */
  if (pos > *end) return;
  
  /* Within selected range... */
  if ((pos <= *end) && (pos >= *start))
    {
      if (*start == *end) {
	/* Deleted last item of selection; reset to zero. */
	*start = 0;
	*end = 0;
      }
      else { /* Just move end pointer. */
	(*end)--;
      }
    }
  else /* Before selected range. Move both. */
    {
      (*start)--;
      (*end)--;
    }
}
/* END OSF Fix CR 4656 */

/***************************************************************************
 *									   *
 * AddInternalElement(lw, string, position, selected)			   *
 *									   *
 * Takes an element from the items list and adds it to the internal list.  *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 ***************************************************************************/
#ifdef NOVELL
static Boolean 
#else /* NOVELL */
static void 
#endif /* NOVELL */
#ifdef _NO_PROTO
AddInternalElement( lw, string, position, selected, do_alloc )
        XmListWidget lw ;
        XmString string ;
        int position ;
        Boolean selected ;
        Boolean do_alloc ;
#else
AddInternalElement(
        XmListWidget lw,
        XmString string,
        int position,
#if NeedWidePrototypes
        int selected,
        int do_alloc )
#else
        Boolean selected,
        Boolean do_alloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	curpos;
	ElementPtr	new_el;
	Boolean		reset_cum_hi = False;

	if (position) 
		curpos = position - 1;
	else
		curpos = LPART(lw).LastItem;

	LPART(lw).LastItem++;

	if (do_alloc)
		LPART(lw).InternalList = (ElementPtr *)XtRealloc(
						(char *)LPART(lw).InternalList,
						sizeof(ElementPtr) *
						LPART(lw).itemCount);

	new_el = (ElementPtr)XtMalloc(sizeof(Element));

	new_el->glyph_data = NULL;
	SetElement((Widget)lw, string, position, new_el);

	if (new_el->height > LPART(lw).MaxItemHeight)
	{
		LPART(lw).MaxItemHeight = new_el->height;
		reset_cum_hi = True;
	}

	if (!reset_cum_hi)
	{
		int	which_row;

		new_el->CumHeight = LPART(lw).MaxItemHeight;
		which_row = curpos / (int)LPART(lw).cols;
		if (which_row)
		{
			int	item_above;

			if (LPART(lw).cols == 1)
			{
				item_above = curpos - 1;
			}
			else
			{
				int	which_col;

				which_col = curpos % (int)LPART(lw).cols;
				item_above = which_col +
					     (which_row - 1) * LPART(lw).cols;
			}
			new_el->CumHeight += LPART(lw).spacing +
			     LPART(lw).InternalList[item_above]->CumHeight;
		}
	}

	new_el->selected	=
	new_el->last_selected	= selected;
	new_el->LastTimeDrawn	= !selected;

				/* Add onto the end of the list */
	if (!position || position == LPART(lw).LastItem)
	{
		LPART(lw).InternalList[curpos] = new_el;
	}
	else			/* in the middle of the list */
	{
		ElementPtr	old, tmp;

		old = LPART(lw).InternalList[curpos];
		LPART(lw).InternalList[curpos] = new_el;
		for (curpos++; curpos < LPART(lw).itemCount; curpos++)
		{
			tmp = lw->list.InternalList[curpos];
			LPART(lw).InternalList[curpos] = old;
			if (!reset_cum_hi)
				LPART(lw).InternalList[curpos]->CumHeight +=
						LPART(lw).MaxItemHeight +
						LPART(lw).spacing;
			old = tmp;
		}
	}

	return(reset_cum_hi);
#else /* NOVELL */
    register int   curpos;
    ElementPtr  new_el, old, tmp;
    Boolean   	ResetCum = FALSE;
    Dimension   maxheight;
/** WHAT ABOUT NON-CONTIGIOUS POSITIONS **/
    if (position) 
        curpos = position - 1;
    else
	curpos = lw->list.LastItem;

    lw->list.LastItem++;

    if (do_alloc)
        lw->list.InternalList = (ElementPtr *)XtRealloc((char *)lw->list.InternalList,
                                            (sizeof(Element *) * lw->list.itemCount));


    new_el = (ElementPtr )XtMalloc(sizeof(Element));

    new_el->name = _XmStringCreate(string);
    new_el->length = XmStringLength(string);
    _XmStringExtent(lw->list.font, new_el->name,
		    &new_el->width, &new_el->height);
    new_el->NumLines = _XmStringLineCount(new_el->name);
    if (new_el->height > lw->list.MaxItemHeight)
    {
	lw->list.MaxItemHeight = new_el->height;
        ResetCum = TRUE;
    }
    new_el->CumHeight = lw->list.MaxItemHeight;
    if (curpos)
        new_el->CumHeight += (lw->list.spacing +
			   lw->list.InternalList[curpos-1]->CumHeight);
    new_el->selected = selected;
    new_el->last_selected = selected;
    new_el->LastTimeDrawn = !selected;

/****************
 *
 * If we are inserting at an existing position we need to insert it -
 *
 ****************/

    maxheight = lw->list.MaxItemHeight;
    if (!position || (position == lw->list.LastItem))	/* Add onto the end of the list */
        lw->list.InternalList[curpos] = new_el;
    else
    {
        old = lw->list.InternalList[curpos];
        lw->list.InternalList[curpos] = new_el;
	for (curpos++; curpos < lw->list.itemCount; curpos++)
	{
	    tmp = lw->list.InternalList[curpos];
	    lw->list.InternalList[curpos] = old;
	    lw->list.InternalList[curpos]->CumHeight += maxheight + lw->list.spacing;
	    old = tmp;
        }

    }

    if (ResetCum)
    {

	lw->list.InternalList[0]->CumHeight = maxheight;
	for (curpos = 1; curpos < lw->list.LastItem; curpos++)
	{
	    lw->list.InternalList[curpos]->CumHeight = 
                maxheight +
                lw->list.InternalList[curpos-1]->CumHeight +
                lw->list.spacing;
	}
    }
#endif /* NOVELL */
}
      
#ifndef NOVELL
/***************************************************************************
 *									   *
 * DeleteInternalElement(lw, string, position, DoAlloc)	       		   *
 *									   *
 * Deletes an element from the internal list. If position is 0, we look    *
 * for the specified string; if the string is NULL we look for specified   *
 * position.								   *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 * ALSO NOTE that this function is sometimes called just after             *
 *    DeleteItem.   This function expects position to be ONE               *
 *    based, while the other expects position to be ZERO based.            *
 *                                                                         *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteInternalElement( lw, string, position, DoAlloc )
        XmListWidget lw ;
        XmString string ;
        int position ;
        Boolean DoAlloc ;
#else
DeleteInternalElement(
        XmListWidget lw,
        XmString string,
        int position,
#if NeedWidePrototypes
        int DoAlloc )
#else
        Boolean DoAlloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Element    	*item;
    register int	curpos;
    Dimension   OldHeight;

    if (!position && string) position = ItemNumber(lw, string);
    if (!position)
    {
	_XmWarning( (Widget) lw, ListMessage8);
	return;	/* We're screwed up */
    }

    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    OldHeight = item->height + lw->list.spacing;
    _XmStringFree(item->name);
    XtFree((char *)item);

    for ( ; curpos < lw->list.itemCount ; curpos++)
    {
	lw->list.InternalList[curpos] = lw->list.InternalList[curpos + 1];
	lw->list.InternalList[curpos]->CumHeight -= OldHeight;
    }

    lw->list.LastItem--;
    
/* BEGIN OSF Fix CR 4656 */
    /* Fix selection delimiters. */
    FixStartEnd(curpos, &lw->list.StartItem, &lw->list.EndItem);
    
    /* Fix old selection delimiters. */
    FixStartEnd(curpos, &lw->list.OldStartItem, &lw->list.OldEndItem);
/* END OSF Fix CR 4656 */

    if (DoAlloc)
    	lw->list.InternalList = (ElementPtr *)XtRealloc((char *) lw->list.InternalList,
                                            (sizeof(Element *) * lw->list.itemCount));

}
#endif /* NOVELL */
/***************************************************************************
 *									   *
 * DeleteInternalElementPositions                                          *
 *     (lw, position_list, position_count, oldItemCount, DoAlloc)     	   *
 *                                                                         *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteInternalElementPositions( lw, position_list, position_count, oldItemCount, DoAlloc )
        XmListWidget  lw ;
        int          *position_list ;
        int           position_count ;
        int           oldItemCount ;
        Boolean       DoAlloc ;
#else
DeleteInternalElementPositions(
        XmListWidget  lw,
        int          *position_list,
        int           position_count,
        int           oldItemCount,
#if NeedWidePrototypes
        int           DoAlloc )
#else
        Boolean       DoAlloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	ElementPtr *	targetPP;
	register int	i, j;

	/* no need to check position_list and position_count because
	 * the caller already did the check! */

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to NULL.  Do not
    try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack the "InternalList" in place ignoring the previously freed
    positions.   "CumHeight" has to be recomputed for all surviving
    elements within "InternalList" - we call "SetMaxHeight" to do this.    

    Reallocate the list.

    This function can work in tandem with DeleteItemPositions which 
    will reset list.itemCount (just as DeleteItem does).   This is
    why we must have oldItemCount passed to us.

*/

	for (i = 0; i < position_count; i++)
	{
		if ((j = position_list[i] - 1) >= 0 &&
		    j < oldItemCount)
		{
			targetPP = &LPART(lw).InternalList[j];
			if (*targetPP)
			{
				_XmStringFree((*targetPP)->name);
				if ((*targetPP)->glyph_data)
					FreeGlyphData((Widget)lw, *targetPP);
				XtFree((char*)*targetPP);
				*targetPP = NULL;
				LPART(lw).LastItem--;

/* BEGIN OSF Fix CR 4656 */
					/* Fix selection delimiters. */
				FixStartEnd(j, &LPART(lw).StartItem,
					    &LPART(lw).EndItem);
    
					/* Fix old selection delimiters. */
				FixStartEnd(j, &LPART(lw).OldStartItem,
					    &LPART(lw).OldEndItem);
/* END OSF Fix CR 4656 */
			}
		}
	}

	j = 0;
	for (i = 0; i < oldItemCount; i++) 
	{
		if (LPART(lw).InternalList[i] != NULL) 
		{
			LPART(lw).InternalList[j] = LPART(lw).InternalList[i];
			j++;
		}
	}
    
	if (DoAlloc)
		LPART(lw).InternalList = (ElementPtr *)XtRealloc(
				(char *)LPART(lw).InternalList,
				sizeof(ElementPtr) * LPART(lw).itemCount);

		/* Should not use ResetHeight()! This routine should be
		 * called when the `font' and/or `spacing' changes */
	SetMaxHeight(lw);

	if (LPART(lw).itemCount)
		CalcCumHeight((Widget)lw);
#else /* NOVELL */
    ElementPtr *targetPP;
    int         ix;
    int         jx;
    int         item_pos;

/*  See what caller can do to flag errors, if necessary,
    when this information is not present.
*/
    if (!position_list || !position_count) 
        return ;

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to NULL.  Do not
    try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack the "InternalList" in place ignoring the previously freed
    positions.   "CumHeight" has to be recomputed for all surviving
    elements within "InternalList" - we call "ResetHeight" to do this.    

    Reallocate the list.

    This function can work in tandem with DeleteItemPositions which 
    will reset list.itemCount (just as DeleteItem does).   This is
    why we must have oldItemCount passed to us.

    (Note, ResetHeight makes another pass though the list, we may want
    to reset the height of each item *as* we re-pack the list.  Since
    other functions, that call ResetHeight, make their own passes through
    the list too, it may be good to take another look at ResetHeight and
    ResetWidth to see if they can be leveraged differently.)
*/
    for (ix = 0; ix < position_count; ix++)
    { 
        item_pos = position_list[ ix ] - 1;
        if ( item_pos >= 0 && item_pos < oldItemCount )
	{
            targetPP = &(lw->list.InternalList[ item_pos ]);
	    if (*targetPP)
	      {
		_XmStringFree ( (*targetPP)->name );
		XtFree( (char*) *targetPP );
		*targetPP = NULL;
		lw->list.LastItem--;

/* BEGIN OSF Fix CR 4656 */
		/* Fix selection delimiters. */
		FixStartEnd(item_pos, &lw->list.StartItem, &lw->list.EndItem);
    
		/* Fix old selection delimiters. */
		FixStartEnd(item_pos, &lw->list.OldStartItem, &lw->list.OldEndItem);
/* END OSF Fix CR 4656 */
	      }
        }        
    }

    jx = 0;
    for (ix = 0; ix < oldItemCount; ix++) 
    {
        if ( lw->list.InternalList[ ix ] != NULL ) 
        {
            lw->list.InternalList[ jx ] = lw->list.InternalList[ ix ];
            jx++;            
        } 
    }
    
    if (DoAlloc)
    	lw->list.InternalList = 
            (ElementPtr*) XtRealloc( (char*) lw->list.InternalList,
                                     (sizeof(Element*) * lw->list.itemCount));

    ResetHeight( lw );
#endif /* NOVELL */
}


/***************************************************************************
 *									   *
 * ReplaceInternalElement(lw, position, selected)                          *
 *									   *
 * Replaces the existing internal item with the specified new one. The new *
 * item is constructed by looking at the list.items - this means that the  *
 * external one has to be replaced first! Note that this does not reset    *
 * the CumHeight fields - it's up to the caller to do that.                *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
ReplaceInternalElement( lw, position, selected)
        XmListWidget lw ;
        int position ;
	Boolean selected ;
#else
ReplaceInternalElement(
        XmListWidget lw,
        int position,
#if NeedWidePrototypes
        int selected)
#else
        Boolean selected)
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	intern_pos;
	ElementPtr	el;
	int		col;
    
	intern_pos = position - 1;

	el = LPART(lw).InternalList[intern_pos];
	_XmStringFree(el->name);              /* Free the old name */

	if (el->glyph_data)
		FreeGlyphData((Widget)lw, el);

	SetElement((Widget)lw, LPART(lw).items[intern_pos], position, el);

	col = intern_pos % (int)LPART(lw).cols;
	if ((int)el->width > (int)LPART(lw).max_col_width[col])
		LPART(lw).max_col_width[col] = el->width;

		/* The logic here can cause selected_list out of date */
	if (selected)
		el->selected = OnSelectedList(lw, LPART(lw).items[intern_pos]);
	else
		el->selected = False;

	el->last_selected = el->selected;
	el->LastTimeDrawn = !el->selected;

	if (el->height > LPART(lw).MaxItemHeight)
	{
		LPART(lw).MaxItemHeight = el->height;
			/* should optimize, this is good for now */
		CalcCumHeight((Widget)lw);
	}
#else /* NOVELL */
    register int   curpos;
    Element    	*item;
    Dimension   maxheight;
    
    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    _XmStringFree(item->name);              /* Free the old name */

    item->name = _XmStringCreate(lw->list.items[curpos]);
    item->length = XmStringLength(lw->list.items[curpos]);
    _XmStringExtent(lw->list.font, item->name, &item->width, &item->height);
    item->NumLines = _XmStringLineCount(item->name);
    if (selected)
        item->selected = OnSelectedList(lw, lw->list.items[curpos]);
    else
        item->selected = FALSE;
    item->last_selected = item->selected;
    item->LastTimeDrawn = !item->selected;
    if (item->height > lw->list.MaxItemHeight)
    {
	lw->list.MaxItemHeight = item->height;
	maxheight = lw->list.MaxItemHeight;
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (curpos = 1; curpos < lw->list.LastItem; curpos++)
	{
	    lw->list.InternalList[curpos]->CumHeight = maxheight +
						  lw->list.InternalList[curpos-1]->CumHeight +
						  lw->list.spacing;
	}
    }
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * AddItem - add an item to the item list at the specified position	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
AddItem( lw, item, pos )
        XmListWidget lw ;
        XmString item ;
        int pos ;
#else
AddItem(
        XmListWidget lw,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
		/* This is an expensive function because it reallocs
		 * the space each time this routine is called */
	int		total_items;
	XmString	new_str;

	total_items = LPART(lw).itemCount + 1;
	LPART(lw).items = (XmString *)XtRealloc(
				(char *)LPART(lw).items,
				sizeof(XmString) * total_items);

	new_str = XmStringCopy(item);

	if (pos >= LPART(lw).itemCount)	/* Add onto the end of the list */
	{
		LPART(lw).items[pos] = new_str;
	}
	else
	{
		XmString	old, tmp;

		old = LPART(lw).items[pos];
		LPART(lw).items[pos] = new_str;
		for (pos++; pos < total_items; pos++)
		{
			tmp = LPART(lw).items[pos];
			LPART(lw).items[pos] = old;
			old = tmp;
		}
	}

	/* Defer max_col_width[] adjustment for two reasons,
	 *	a. it will call AddInternalElement() eventually and
	 *	   the internal data will have string width info, so
	 *	   no reason to waste XmStringWidth() call here.
	 *	b. `item' can be NULL, and can be set when
	 *	   item_init_cb is called, so...
	 */

	LPART(lw).itemCount = total_items;

#else /* NOVELL */
    int	     TotalItems, i;
    XmString old, new_str, tmp;

    TotalItems = lw->list.itemCount + 1;
    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));
    new_str = XmStringCopy(item);

    if (pos >= lw->list.itemCount)	/* Add onto the end of the list */
        lw->list.items[pos] = new_str;
    else
    {
        old = lw->list.items[pos];
        lw->list.items[pos] = new_str;
	for (pos++; pos < TotalItems; pos++)
	{
	    tmp = lw->list.items[pos];
	    lw->list.items[pos] = old;
	    old = tmp;
        }

    }
    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;

    lw->list.itemCount = TotalItems;
#endif /* NOVELL */
}


#ifndef NOVELL
/************************************************************************
 *									*
 * DeleteItem - delete an item from the item list.			*
 *									*
 *    Note that this function is sometimes called just before           *  
 *    DeleteInternalElement.   This function expects position to be     *
 *    ZERO based, while the other expects position to be ONE based.     *
 *                                                                      *
 * ON DELETE, DO WE UPDATE MAXWIDTH??					*
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteItem( lw, pos )
        XmListWidget lw ;
        int pos ;
#else
DeleteItem(
        XmListWidget lw,
        int pos )
#endif /* _NO_PROTO */
{

    int 	     TotalItems;

    if (lw->list.itemCount < 1)
      return;

    TotalItems = lw->list.itemCount - 1;
    XmStringFree(lw->list.items[pos]);

    if (pos < lw->list.itemCount)
    {
	for ( ; pos < TotalItems; pos++)
	    lw->list.items[pos] = lw->list.items[pos+1];

    }
    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));
    lw->list.itemCount = TotalItems;

}
#endif /* NOVELL */

/************************************************************************
 *									*
 * DeleteItemPositions                                                  *
 *     (lw, position_list, position_count)     	                        *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteItemPositions( lw, position_list, position_count )
        XmListWidget  lw ;
        int          *position_list ;
        int           position_count ;

#else
DeleteItemPositions(
        XmListWidget  lw,
        int          *position_list,
        int           position_count
        )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	/* The original code assumes that entries in XmNlists can't not
	 * be NULL, this is bad because people need to hack the code if
	 * they want to use XmListDelete API. In addition, this will
	 * create problems for multiple column support. Well,
	 * you have to allow empty column(s) in a list without hacking
	 * the code (e.g., give " "). */
#define MARK_AS_DELETED		(XmString)1

	register int		i, j;
	int			total_items;
    
	if (!LPART(lw).itemCount)
		return;

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to MARK_AS_DELETED.
    Do not try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack "items" in place ignoring the previously freed  positions.  
*/
	total_items = LPART(lw).itemCount;

	for (i = 0; i < position_count; i++)
	{ 
		if ((j = position_list[i] - 1) >= 0 &&
		    j < LPART(lw).itemCount)
		{
				/* No need to invoke if NULL.
				 * This fix shall be applied to
				 * #else NOVELL as well.
				 */
			if (LPART(lw).items[j])
				XmStringFree(LPART(lw).items[j]);

			LPART(lw).items[j] = MARK_AS_DELETED;
			total_items--;
		}        
	}

	j = 0;
	for (i = 0; i < LPART(lw).itemCount; i++) 
	{
		if (LPART(lw).items[i] != MARK_AS_DELETED)
		{
			LPART(lw).items[j] = LPART(lw).items[i];
			j++;
		} 
	}

	LPART(lw).items = (XmString *)XtRealloc(
				(char *)LPART(lw).items,
				sizeof(XmString) * total_items);

	LPART(lw).itemCount = total_items;

#undef MARK_AS_DELETED
#else /* NOVELL */
    int 	     TotalItems;
    int              item_pos;
    int              ix;
    int              jx;
    XmString	     item;
    
    if (lw->list.itemCount < 1)
      return;

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to NULL.  Do not 
    try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack "items" in place ignoring the previously freed  positions.  
*/
    TotalItems = lw->list.itemCount;

    for (ix = 0; ix < position_count; ix++)
    { 
        item_pos = position_list[ ix ] - 1;
        if ( item_pos >= 0 && item_pos < lw->list.itemCount )
	{
	  item = lw->list.items[item_pos];
	  if (item)
	    {
	      XmStringFree(item);
	      lw->list.items[item_pos] = NULL;
	      TotalItems--;
	    }
        }        
    }

    jx = 0;
    for (ix = 0; ix < lw->list.itemCount; ix++) 
    {
        if ( lw->list.items[ ix ] != NULL ) 
        {
            lw->list.items[ jx ] = lw->list.items[ ix ];
            jx++;            
        } 
    }


    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));

    lw->list.itemCount = TotalItems;
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * ReplaceItem - Replace an item at the specified position	        *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ReplaceItem( lw, item, pos )
        XmListWidget lw ;
        XmString item ;
        int pos ;
#else
ReplaceItem(
        XmListWidget lw,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
    int i;

    pos--;

    XmStringFree(lw->list.items[pos]);
    lw->list.items[pos] = XmStringCopy(item);

#ifndef NOVELL
	/* NOVELL: StringWidth and MaxWidth will be calculated in
	 * ReplaceInternalElement(), don't need spin CPU time here.
	 * ReplaceItem and ReplaceInternalElement is invoked as
	 * a pair (at least this is true for now)! */
    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;
#endif
}


/***************************************************************************
 *									   *
 * ItemNumber - returns the item number of the specified item in the 	   *
 * external item list.							   *
 *									   *
 ***************************************************************************/
static int 
#ifdef _NO_PROTO
ItemNumber( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
ItemNumber(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	/* InternalList already have _XmString information, so use it!
	 * XmStringCompare() is expensive because each call takes 2 mallocs
	 * and 2 fress. */

	register int	i;
	int		target = 0;
	_XmString	_item = _XmStringCreate(item);

	for (i = 0; i < LPART(lw).itemCount; i++)
		if (_XmStringByteCompare(
				LPART(lw).InternalList[i]->name, _item))
		{
			target = i + 1;
			break;
		}

	_XmStringFree(_item);
	return(target);

#else /* NOVELL */
    register int i;

    for (i = 0; i < lw->list.itemCount;	i++)
    	if (XmStringCompare(lw->list.items[i], item))
	    return(i+1);
    return (0);
#endif /* NOVELL */
}


#ifndef NOVELL
/***************************************************************************
 *									   *
 * ItemExists - returns TRUE if the specified item matches an item in the  *
 * List item list.							   *
 *									   *
 ***************************************************************************/
static int 
#ifdef _NO_PROTO
ItemExists( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
ItemExists(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
    register int     i;

    for (i = 0; i < lw->list.itemCount; i++)
    	if ((XmStringCompare(lw->list.items[i], item)))
	    return(TRUE);
    return (FALSE);
}
#endif /* NOVELL */


/************************************************************************
 *									*
 * OnSelectedList - Returns TRUE if the given item is on the selected	*
 * list.								*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
OnSelectedList( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
OnSelectedList(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
    register int  i;

    for (i = 0; i < lw->list.selectedItemCount; i++)
    	if (XmStringCompare(lw->list.selectedItems[i], item))
	    return(TRUE);
    return (FALSE);
}


/************************************************************************
 *									*
 * CopyItems - Copy the item list into our space.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CopyItems( lw )
        XmListWidget lw ;
#else
CopyItems(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmString	*il;

    if (lw->list.items && lw->list.itemCount)
    {
	il = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.itemCount));
	for (i = 0; i < lw->list.itemCount; i++)
            il[i] =  XmStringCopy(lw->list.items[i]);

	lw->list.items = il;
/* BEGIN OSF Fix CR 5337 */
/* END OSF Fix CR 5337 */
    }
}


/************************************************************************
 *									*
 * CopySelectedItems - Copy the selected item list into our space.	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CopySelectedItems( lw )
        XmListWidget lw ;
#else
CopySelectedItems(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmString	*sl;


    if (lw->list.selectedItems && lw->list.selectedItemCount)
    {
	sl = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.selectedItemCount));
	for (i = 0; i < lw->list.selectedItemCount; i++)
            sl[i] =  XmStringCopy(lw->list.selectedItems[i]);

	lw->list.selectedItems = sl;
/* BEGIN OSF Fix CR 5623 */
/* END OSF Fix CR 5623 */
    }
}

/************************************************************************
 *									*
 * ClearItemList - delete all elements from the item list, and		*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClearItemList( lw )
        XmListWidget lw ;
#else
ClearItemList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmListWidget lwa = (XmListWidget) lw->core.self ;

    if (!(lw->list.items && lw->list.itemCount)) return;
    for (i = 0; i < lw->list.itemCount; i++)
	XmStringFree(lw->list.items[i]);
    XtFree((char *) lw->list.items);
    lw->list.itemCount = 0;
    lw->list.items = NULL;
    lw->list.top_position = 0;
    /* Will use self pointer to find actual List widget instance,
     *   since in SetValues the "OldLW" is passed as the argument,
     *   and cleaning up the following fields in the OldLW instance
     *   record doesn't have value.
     */
    lwa->list.LastItem = 0;
    lwa->list.LastHLItem = 0;	/* NOVELL - FIRST_ITEM(lwa)? */
    lwa->list.StartItem = 0;
    lwa->list.EndItem = 0;
    lwa->list.OldStartItem = 0;
    lwa->list.OldEndItem = 0;
#ifdef NOVELL
    lwa->list.CurrentKbdItem = FIRST_ITEM(lwa);;
#else /* NOVELL */
    lwa->list.CurrentKbdItem = 0;
#endif /* NOVELL */
    lwa->list.XOrigin = 0;
}



/************************************************************************
 *									*
 * ClearSelectedList - delete all elements from the selected list, and	*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClearSelectedList( lw )
        XmListWidget lw ;
#else
ClearSelectedList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;

    if (!(lw->list.selectedItems && lw->list.selectedItemCount)) return;

    for (i = 0; i < lw->list.selectedItemCount; i++)
	XmStringFree(lw->list.selectedItems[i]);
    XtFree((char *) lw->list.selectedItems);
    lw->list.selectedItemCount = 0;
    lw->list.selectedItems = NULL;

    XtFree((char *)lw->list.selectedIndices);
    lw->list.selectedIndices = NULL;
}


/************************************************************************
 *									*
 *  BuildSelectedList - traverse the element list and construct a list	*
 *		       of selected elements and indices.		*
 *									*
 *  NOTE: Must be called in tandem with and *AFTER* ClearSelectedList.	*
 *        Otherwise you have a memory leak...				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
BuildSelectedList( lw, commit )
        XmListWidget lw ;
	Boolean	     commit;
#else
BuildSelectedList(
        XmListWidget lw ,
#if NeedWidePrototypes
                        int commit)
#else
                        Boolean commit)
#endif /*   NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int i,j, count;
    Boolean  sel;

    count = lw->list.itemCount;
    for (i = 0, j = 0; i < count; i++)
    {
        sel = lw->list.InternalList[i]->selected;
	if (sel) j++;
        if (commit)
	    lw->list.InternalList[i]->last_selected = sel; /* Commit the selection */
    }
    lw->list.selectedItemCount = j;
    lw->list.selectedItems = NULL;
    lw->list.selectedIndices = NULL;
    if (j == 0) return;
    lw->list.selectedItems = (XmString *)XtMalloc(sizeof(XmString) * j);
    lw->list.selectedIndices = (int *)XtMalloc(sizeof(int) * j);

    for (i = 0, j = 0; i < count; i++)
	if (lw->list.InternalList[i]->selected)
	{
            lw->list.selectedItems[j] =  XmStringCopy(lw->list.items[i]);
            lw->list.selectedIndices[j] = i + 1;
            j++;
        }

 }

/************************************************************************
 *									*
 *  UpdateSelectedList - Build a new selected list.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
UpdateSelectedList( lw )
        XmListWidget lw ;
#else
UpdateSelectedList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    ClearSelectedList(lw);
    BuildSelectedList(lw, TRUE);
}

/***************************************************************************
 *									   *
 * UpdateSelectedIndices - traverse the element list and construct a list  *
 * of selected indices.							   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
UpdateSelectedIndices( lw )
        XmListWidget lw ;
#else
UpdateSelectedIndices(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i,j;
    register int count = 0;

    for (i = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected) count++;
        
/****************
 *
 * Allocate the array. If the count is 0, but we have a valid pointer, free
 * the old space. Else there's a 'ole in me pocket...
 *
 ****************/
    if (count == 0)
    {
	if (lw->list.selectedIndices != NULL)
	{
	    XtFree((char *)lw->list.selectedIndices);
	    lw->list.selectedIndices = NULL;
	}
	return;
    }
    lw->list.selectedIndices = (int *)XtMalloc(sizeof(int) * count);

    for (i = 0, j = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected)
	{
            lw->list.selectedIndices[j] = i + 1;
	    j++;
        }
}



/************************************************************************
 *									*
 *             Event Handlers for the various selection modes		*
 *									*
 ************************************************************************/


/************************************************************************
 *									*
 * WhichItem - Figure out which element we are on. Check the cumheight  *
 * 	       of the visible items through a grody linear search for	*
 *   	       now - let's do a binary later??				8
 *									*
 ************************************************************************/
static int 
#ifdef NOVELL

#ifdef _NO_PROTO
WhichItem( w, static_rows_as_topleave, EventX, EventY )
        XmListWidget w ;
	Boolean  static_rows_as_topleave ;
        Position EventX ;
        Position EventY ;
#else
WhichItem(
        XmListWidget w,
	Boolean  static_rows_as_topleave,
#if NeedWidePrototypes
        int EventX,
        int EventY )
#else
        Position EventX,
        Position EventY )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */

#else /* NOVELL */
#ifdef _NO_PROTO
WhichItem( w, EventY )
        XmListWidget w ;
        Position EventY ;
#else
WhichItem(
        XmListWidget w,
#if NeedWidePrototypes
        int EventY )
#else
        Position EventY )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
#endif /* NOVELL */
{
#ifdef NOVELL
	register Position	x = EventX;
	register Position	y = EventY;
	register int		item;
	register int		BaseY;
	int			delta_hi, which_row, rows, last_row_size;
	int			lower_bdd, upper_bdd;
	int			i, j;
	Boolean			calc_col_only = False;

/* BEGIN OSF Fix CR 5081 */
	if (!LPART(w).itemCount)
		return(-1);

	if (LPART(w).Traversing && LPART(w).KbdSelection)
		return(LPART(w).CurrentKbdItem);

	if (y <= (int)LPART(w).BaseY - (int)LPART(w).HighlightThickness)
	{
		if (LPART(w).top_position) {
			if (LPART(w).cols == 1) {
					/* See BrowseScroll for reasons */
				LPART(w).col_info = 0;
				return(-1);
			}
			else {
				calc_col_only = True;
				item = -1;
				goto compute_col;
			}
		} else {
			if (LPART(w).cols == 1)
				return(TOP_VIZ_ITEM(w));
			else {
					/* VERY TOP */
				which_row = TOP_ITEM_ROW(w);
					/* have to compute `col_info' */
				goto compute_col;
			}
		}
	}
/* END OSF Fix CR 5081 */

	item = LPART(w).itemCount + 1;
	if (y > (int)CPART(w).height &&
	    BOT_VIZ_ITEM(w) + 1 >= LPART(w).itemCount) {

		if (LPART(w).cols == 1)
			return(item - 2);
		else {
			int	last_row_size;

				/* Note A: VERY BOTTOM */
			CalcNumRows((Widget)w, &which_row, &last_row_size);
			which_row--;	/* 0-based */
				/* have to compute `col_info' */
			goto compute_col;
		}
	}

	if (y >= ((int)CPART(w).height - (int)LPART(w).BaseY)) {

		if (LPART(w).cols == 1) {
				/* See BrowseScroll for reasons */
			LPART(w).col_info = 0;
			return(item);
		} else {
				/* have to compute `col_info' */
			calc_col_only = True;
			goto compute_col;
		}
	}

	if (LPART(w).static_rows) {

			/* check if it's a title item */
		which_row = 0;
		item = 0;
		BaseY = LPART(w).InternalList[item]->CumHeight -
			LPART(w).BaseY -
			LPART(w).HighlightThickness; 
		while(y > ((int)LPART(w).InternalList[item]->CumHeight -
			   (int)BaseY + (int)LPART(w).MaxItemHeight)) {
			which_row++;
			if (which_row >= (int)LPART(w).static_rows)
				break;
			item = which_row * LPART(w).cols;
			if (item >= LPART(w).itemCount)
				return(-1);
		}

		if (which_row < (int)LPART(w).static_rows) {

			if (!static_rows_as_topleave)
				return(-1);

			/* If I am here, then treating it as if TOPLEAVE */
			LPART(w).LeaveDir |= TOPLEAVE;
			if (LPART(w).top_position) {
				if (LPART(w).cols == 1) {
					/* See BrowseScroll for reasons */
					LPART(w).col_info = 0;
					return(-1);
				}
				else {
					calc_col_only = True;
					item = -1;
					goto compute_col;
				}
			} else {
				if (LPART(w).cols == 1)
					return(TOP_VIZ_ITEM(w));
				else {
						/* VERY TOP */
					which_row = TOP_ITEM_ROW(w);
					/* have to compute `col_info' */
					goto compute_col;
				}
			}
		}
	}

	which_row = TOP_ITEM_ROW(w);
	item = TOP_VIZ_ITEM(w);	/* 1st col of nth row */

/* BEGIN OSF Fix CR 5081 */
	BaseY = LPART(w).InternalList[item]->CumHeight - LPART(w).BaseY -
		LPART(w).HighlightThickness; 
/* END OSF Fix CR 5081 */
/****************
 *
 * Pull these invariants out of dereferences...
 *
 ****************/
		/* Shall take `spacing' into consideration when
		 * `static_rows' != 0 */
	delta_hi = (LPART(w).static_rows + 1) * LPART(w).MaxItemHeight +
		    LPART(w).static_rows * LPART(w).spacing;
	while(y > ((int)LPART(w).InternalList[item]->CumHeight - (int)BaseY +
								delta_hi))
	{
		which_row++;
			/* the 1st column of next row */
		item = which_row * LPART(w).cols;
		if (item >= LPART(w).itemCount)
			return(item);
	}

compute_col:
		/* Now we know which_row, use hOrigin to figure out
		 * the item based on x, Also adjust `x' because we are
		 * dealing with virtual coords now.
		 *
		 * Also take col_spacing into consideration because people
		 * may click on this dead space. */
	x += LPART(w).hOrigin;
	lower_bdd = LPART(w).BaseX - LPART(w).HighlightThickness;
			/* Shall we round up (col_spacing)? */
	upper_bdd = lower_bdd + LPART(w).max_col_width[0] + 2 *
		    LPART(w).HighlightThickness + LPART(w).col_spacing / 2 ;

	j = 0;

		/* Pointer must be way over the left side, if x <= lower_bdd */
	if (x > lower_bdd) {

		for (i = 1; i < (int)LPART(w).cols; i++) {

			if (lower_bdd < x && x <= upper_bdd)
				break;

			j++;
			lower_bdd = upper_bdd;
			upper_bdd = lower_bdd + LPART(w).max_col_width[i] + 2 *
			    LPART(w).HighlightThickness + LPART(w).col_spacing;
		}
	}

	if (calc_col_only)
		LPART(w).col_info = j;	/* See BrowseScroll for reasons */
	else {

		item = j + which_row * LPART(w).cols;

			/* Can happen if it was from `Note A' above */
		if (item >= LPART(w).itemCount)
			item -= LPART(w).cols;
	}
	return(item);

#else /* NOVELL */
    register XmListWidget lw = w;
    register Position y = EventY;
    register int item = lw->list.itemCount + 1;
    register int BaseY;

    if (lw->list.Traversing && lw->list.KbdSelection)
        return(lw->list.CurrentKbdItem);

/* BEGIN OSF Fix CR 5081 */
    if (!lw->list.items) return(-1);

    if (y <= (lw->list.BaseY - lw->list.HighlightThickness))
/* END OSF Fix CR 5081 */
    {
        if (lw->list.top_position)
            return(-1);
        else
            return(0);
    }
    if ((Dimension) y > lw->core.height)
    {
        if ((lw->list.top_position + lw->list.visibleItemCount) >=
            lw->list.itemCount)
            return (item - 2);
    }
    if (y >= (lw->core.height - lw->list.BaseY))/* NOVELL, don't know this one */
	     return(item);

    item = lw->list.top_position;
/* BEGIN OSF Fix CR 5081 */
    BaseY = lw->list.InternalList[item]->CumHeight  - lw->list.BaseY -
      lw->list.HighlightThickness; 
/* END OSF Fix CR 5081 */
/****************
 *
 * Pull these invariants out of dereferences...
 *
 ****************/
    while(y > (lw->list.InternalList[item]->CumHeight - BaseY +
		lw->list.MaxItemHeight))
    {
	item++;
	if (item >= lw->list.itemCount) return(item);
    }
    return(item);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * SelectRange - Select/deselect the range between start and end.       *
 *              This does not set the last_selected flag.               *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SelectRange( lw, first, last, select )
        XmListWidget lw ;
        int first ;
        int last ;
        Boolean select ;
#else
SelectRange(
        XmListWidget lw,
        int first,
        int last,
#if NeedWidePrototypes
        int select )
#else
        Boolean select )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int tmp, start, end;

    start = first; end = last;
    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    for (; start <= end; start++)
    {
        lw->list.InternalList[start]->selected = select;
        DrawItem((Widget) lw, start);
    }
}
/************************************************************************
 *									*
 * RestoreRange - Restore the range between start and end.              *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
RestoreRange( lw, first, last, dostart )
        XmListWidget lw ;
        int first ;
        int last ;
        Boolean dostart ;
#else
RestoreRange(
        XmListWidget lw,
        int first,
        int last,
#if NeedWidePrototypes
        int dostart )
#else
        Boolean dostart )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int tmp, start, end;
    start = first; end = last;

    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    tmp = lw->list.StartItem;
    for (; start <= end; start++)
        if ((start != tmp) || dostart)
	{
	    lw->list.InternalList[start]->selected =
            	    lw->list.InternalList[start]->last_selected;
	    DrawItem( (Widget) lw, start);
	}
}
/************************************************************************
 *                                                                      *
 * ArrangeRange - This does all the necessary magic for movement in     *
 * extended selection mode.  This code handles all the various cases    *
 * for relationships between the start, end and current item, restoring *
 * as needed, and selecting the appropriate range.  This is called in   *
 * both the mouse and keyboard cases.                                   *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ArrangeRange( lw, item )
        XmListWidget lw ;
        int item ;
#else
ArrangeRange(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
{
    register int    start, end, i;
    Boolean set;

    start = lw->list.StartItem;
    end = lw->list.EndItem;
    i = item;
    set = lw->list.InternalList[start]->selected;

    if (start < end)
    {
        if (i > end)
            SelectRange(lw, end, i, set);
#ifndef OSF_v1_2_4
        else
            if ((i < end) && (i >= start))
		if (!set)
                    RestoreRange(lw, i+1, end, FALSE);
		else SelectRange(lw,i+1,end,FALSE);
            else
                if (i <= start)
                {
		     if (!set)
                         RestoreRange(lw, start, end, FALSE);
		     else
		         SelectRange(lw,start,end,FALSE);
                     SelectRange(lw, i, start, set);
                }
#else /* OSF_v1_2_4 */
        else if ((i < end) && (i >= start))
	  {
	    /* CR 5676: Undo extended toggle drags properly. */
	    if (!set || (lw->list.Event & CTRLDOWN))
	      RestoreRange(lw, i+1, end, FALSE);
	    else 
	      SelectRange(lw,i+1,end,FALSE);
	  }
	else if (i <= start)
	  {
	    /* CR 5676: Undo extended toggle drags properly. */
	    if (!set || (lw->list.Event & CTRLDOWN))
	      RestoreRange(lw, start, end, FALSE);
	    else
	      SelectRange(lw,start,end,FALSE);
	    SelectRange(lw, i, start, set);
	  }
#endif /* OSF_v1_2_4 */
     }
     else
         if (start > end)
         {
             if (i <= end)
                 SelectRange(lw, i, end, set);
#ifndef OSF_v1_2_4
             else
                 if ((i > end) && (i <= start))
		     if (!set)
                         RestoreRange(lw, end, i-1, FALSE);
		     else
		         SelectRange(lw,end,i-1,FALSE);
                 else
                     if (i >= start)
                     {
			if (!set)
                            RestoreRange(lw, end, start, FALSE);
			else
			     SelectRange(lw,end,start,FALSE);
                         SelectRange(lw,start, i, set);
                     }
#else /* OSF_v1_2_4 */
             else if ((i > end) && (i <= start))
	       {
		 /* CR 5676: Undo extended toggle drags properly. */
		 if (!set || (lw->list.Event & CTRLDOWN))
		   RestoreRange(lw, end, i-1, FALSE);
		 else
		   SelectRange(lw,end,i-1,FALSE);
	       }
	     else if (i >= start)
	       {
		 /* CR 5676: Undo extended toggle drags properly. */
		 if (!set || (lw->list.Event & CTRLDOWN))
		   RestoreRange(lw, end, start, FALSE);
		 else
		   SelectRange(lw,end,start,FALSE);
		 SelectRange(lw,start, i, set);
	       }
#endif /* OSF_v1_2_4 */
          }
          else
              SelectRange(lw, start, i, set);
}

/************************************************************************
 *									*
 * HandleNewItem - called when a new item is selected in browse or	*
 * extended select mode.  This does the deselection of previous items	*
 * and the autoselection, if enabled.					*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleNewItem( lw, item, olditem )
        XmListWidget lw ;
        int item ;
        int olditem ;
#else
HandleNewItem(
        XmListWidget lw,
        int item,
        int olditem )
#endif /* _NO_PROTO */
{
    if (lw->list.LastHLItem == item) return;

    switch(lw->list.SelectionPolicy)
    {
	case XmBROWSE_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL, FALSE);
                    lw->list.DidSelection = TRUE;
                }
		lw->list.InternalList[lw->list.LastHLItem]->selected = FALSE;
		if (lw->list.LastHLItem != lw->list.CurrentKbdItem)
		    lw->list.InternalList[lw->list.LastHLItem]->last_selected = FALSE;
		DrawItem((Widget) lw, lw->list.LastHLItem);
		lw->list.InternalList[item]->selected = TRUE;
/*		lw->list.InternalList[item]->last_selected = TRUE;*/
		DrawItem((Widget) lw, item);
		lw->list.LastHLItem = item;
		lw->list.StartItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect) 
                {
                    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
                    ClickElement(lw,NULL, FALSE);
                    lw->list.CurrentKbdItem = item;
                    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
                }
		break;
	case XmEXTENDED_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (lw->list.DidSelection)
                        ClickElement(lw,NULL,FALSE);
                }
                ArrangeRange(lw, item);
		lw->list.LastHLItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect)
                {
		    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL, FALSE);
                    lw->list.DidSelection = TRUE;
                }
		break;
    }
}

/************************************************************************
 *									*
 * HandleExtendedItem - called when a new item is selected via the      *
 * keyboard in  extended select mode.  This does the deselection of     *
 * previous items and handles some of the add mode actions.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
HandleExtendedItem( lw, item )
        XmListWidget lw ;
        int item ;
#else
HandleExtendedItem(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
{
    Boolean set;
    register int     i, start, end;

    if (lw->list.LastHLItem == item) return;

    if (!lw->list.AddMode)      /* First the non-addmode case */
    {
        if (!(lw->list.Event & SHIFTDOWN))    /* And not shifted */
        {
            lw->list.StartItem = item;
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            for (i = 0; i < lw->list.itemCount; i++)
                if(lw->list.InternalList[i]->selected)
                    if (i != item)
                    {
                        lw->list.InternalList[i]->selected = FALSE;
                        lw->list.InternalList[i]->last_selected = FALSE;
                        DrawItem((Widget) lw, i);
                    }
            lw->list.InternalList[item]->selected = TRUE;
            lw->list.InternalList[item]->last_selected = TRUE;
            DrawItem((Widget) lw, item);
            ClickElement(lw,NULL,FALSE);
        }
        else                                /* Shifted */
        {
            if (lw->list.selectedItemCount == 0)
                lw->list.StartItem = item;
            set = lw->list.InternalList[lw->list.StartItem]->selected;
#ifndef OSF_v1_2_4
            start = (lw->list.StartItem < item)
                    ? lw->list.StartItem : item;
            end = (lw->list.StartItem < item)
                    ? item : lw->list.StartItem;
#else /* OSF_v1_2_4 */
            start = MIN(lw->list.StartItem, item);
            end = MAX(lw->list.StartItem, item);
#endif /* OSF_v1_2_4 */
/****************
 *
 * Deselect everything outside of the current range.
 *
 ****************/
            for (i = 0; i < start; i++)
                if (lw->list.InternalList[i]->selected)
                {
                    lw->list.InternalList[i]->selected = FALSE;
        	    DrawItem((Widget) lw, i);
                }
            for (i = end + 1; i < lw->list.itemCount; i++)
                if (lw->list.InternalList[i]->selected)
        	{
        	    lw->list.InternalList[i]->selected = FALSE;
           	    DrawItem((Widget) lw, i);
        	}
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            SelectRange(lw, lw->list.StartItem, item, set);
            ClickElement(lw,NULL,FALSE);
        }
    }
    else                                    /* Add Mode next... */
    {
        if (lw->list.Event & SHIFTDOWN)     /* Shifted */
        {
            ArrangeRange(lw, item);
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            ClickElement(lw,NULL,FALSE);
        }
    }
}

/************************************************************************
 *									*
 * VerifyMotion - event handler for motion within the list.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
VerifyMotion( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
VerifyMotion(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget w = (XmListWidget) wid ;
    int	item;
    Time  interval;
    register XmListWidget lw = w;
    unsigned char OldLeaveDir = lw->list.LeaveDir;
#ifdef NOVELL

	if (!LPART(lw).itemCount || LPART(lw).itemCount <= FIRST_ITEM(lw))
		return;
#endif /* NOVELL */
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if ((lw->list.SelectionPolicy == XmSINGLE_SELECT)  ||
    	(lw->list.SelectionPolicy == XmMULTIPLE_SELECT))
	return;


/****************
 *
 * First, see if we're out of the window. If we are, and 
 * if the direction is different than the last leave direction, fake a
 * leave window event. This allows you to drag out of the list, and then
 * futz around with the cursor outside of the window, and it will track
 * correctly.
 *
 ****************/
    if ((event->xmotion.x < (int )lw->core.width)  &&
        (event->xmotion.x > (int )lw->core.x)      &&
        (event->xmotion.y < (int )lw->core.height) &&
        (event->xmotion.y >(int )lw->core.y))
	{
	     if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	     lw->list.DragID = 0;
	}
    else
    {
        if (((event->xmotion.y >= (int )lw->core.height) &&
             (lw->list.LeaveDir & TOPLEAVE)) ||
            ((event->xmotion.y <= (int )lw->core.y) &&
	     (lw->list.LeaveDir & BOTTOMLEAVE))  ||
            ((event->xmotion.x <= (int )lw->core.x) &&
	     (lw->list.LeaveDir & RIGHTLEAVE))  ||
            ((event->xmotion.x >= (int )lw->core.width) &&
	     (lw->list.LeaveDir & LEFTLEAVE)))
	    {
	         if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	         lw->list.DragID = 0;
		 ListLeave((Widget) lw, event, params, num_params) ;
		 return;
	    }
    }

    lw->list.LeaveDir = 0;
    if (event->xmotion.y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->xmotion.y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->xmotion.x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->xmotion.x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;

#ifdef NOVELL
    item = WhichItem(lw, True /* static_rows_as_topleave */,
				event->xmotion.x, event->xmotion.y);
#else /* NOVELL */
    item = WhichItem(lw,event->xmotion.y);
#endif /* NOVELL */

    if (lw->list.LeaveDir)
    {
        if (lw->list.vScrollBar)
            interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
        else
            interval = 100;
        if (!lw->list.DragID ||
            (OldLeaveDir != lw->list.LeaveDir))
	{
    	     if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
             lw->list.DragID = XtAppAddTimeOut (
                                     XtWidgetToApplicationContext((Widget) lw),
                                          (unsigned long) interval,
                                                 BrowseScroll, (XtPointer) lw);
	}
    }

#ifdef NOVELL
	if (item == LPART(lw).LastHLItem ||
	    item >= LPART(lw).itemCount ||
	    item < TOP_VIZ_ITEM(lw) ||
	    item > BOT_VIZ_ITEM(lw))
		return;
#else /* NOVELL */
    if ((item == lw->list.LastHLItem) ||
        (item >= lw->list.itemCount)  ||
        (item < lw->list.top_position)||
	(item >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;
#endif /* NOVELL */

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;
    lw->list.DidSelection = FALSE;
    HandleNewItem(lw, item, lw->list.LastHLItem);
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

   if (*num_params != 2 || !XmIsList(w))
      return;
   if (XmTestInSelection((XmListWidget)w, event))
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
        XmListWidget w ;
        XEvent *event ;
#else
XmTestInSelection(
        XmListWidget w,
	XEvent *event )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
    int cur_item = WhichItem(w, False /* static_rows_as_topleave */,
					event->xbutton.x, event->xbutton.y);
#else /* NOVELL */
    int cur_item = WhichItem(w, event->xbutton.y);
#endif /* NOVELL */
    if (cur_item < 0) /* no items */
	return(False);
    if (cur_item >= w->list.itemCount)  /* below all items */
        return(False);
    if (!OnSelectedList(w, w->list.items[cur_item]))
        return(False);
    else {
	/* The determination of whether this is a transfer drag cannot be made
	   until a Motion event comes in.  It is not a drag as soon as a
	   ButtonUp event happens. */
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

#ifdef CDE_FILESB
enum{	XmPATH_MODE_FULL,	XmPATH_MODE_RELATIVE};
static void 
#ifdef _NO_PROTO
ScrollBarDisplayPolicyDefault( widget, offset, value )
        Widget widget ;
        int offset ;
        XrmValue *value ;
#else
ScrollBarDisplayPolicyDefault(
        Widget widget,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
	static unsigned char sb_display_policy;

	value->addr = (XPointer) &sb_display_policy;

	if (XmIsScrolledWindow(XtParent(widget)) &&
            XmIsFileSelectionBox(XtParent(XtParent(widget))))  {
		XtEnum path_mode;
		XtVaGetValues(XtParent(XtParent(widget)), "pathMode", &path_mode, NULL);
		if (path_mode == XmPATH_MODE_RELATIVE)
			sb_display_policy = XmAS_NEEDED;
		else
			sb_display_policy = XmSTATIC;
        }
	else
		sb_display_policy = XmAS_NEEDED;
}  /* end of ScrollBarDisplayPolicyDefault() */
#endif /* CDE_FILESB */
/***************************************************************************
 *									   *
 * Element Select - invoked on button down on a widget.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    Time interval;
    register int i, item;
    int     	start = 0, end = 1;
    Boolean sel;

#ifdef NOVELL
	if (!LPART(lw).itemCount || LPART(lw).itemCount <= FIRST_ITEM(lw))
		return;

	item = WhichItem(lw, False /* static_rows_as_topleave */,
				event->xbutton.x, event->xbutton.y);

	if (item >= LPART(lw).itemCount ||
	    item < TOP_VIZ_ITEM(lw) ||
	    item > BOT_VIZ_ITEM(lw))
		return;

	interval = (Time)LPART(lw).ClickInterval;

#else /* NOVELL */
    if (!lw->list.itemCount) return;

    interval = (Time) lw->list.ClickInterval;

    item = WhichItem(lw,event->xbutton.y);

    if ((item >= (lw->list.top_position+lw->list.visibleItemCount)) ||
        (item < lw->list.top_position) ||
	(item >= lw->list.itemCount)) return;
#endif /* NOVELL */

    lw->list.Event |= BUTTONDOWN;
    lw->list.LeaveDir = 0;

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
	if (lw->list.Event & SHIFTDOWN)
	    lw->list.SelectionType = XmMODIFICATION;
	else if (lw->list.Event & CTRLDOWN)
		 lw->list.SelectionType = XmADDITION;
	     else lw->list.SelectionType = XmINITIAL;

    }
/**************
 *
 * Look for a double click.
 *
 **************/
    if (!(lw->list.KbdSelection) &&	/* Sigh. No more doubleclick from space... */
        (lw->list.DownTime != 0) &&
        (lw->list.DownCount > 0) &&
        ( event->xbutton.time < (lw->list.DownTime + interval)))
    {
        lw->list.DownCount++;
        lw->list.DownTime = 0;
        return;
    }
/**************
 *
 *  Else initialize the count variables.
 *
 **************/


    lw->list.DownCount = 1;
    if (!(lw->list.KbdSelection))
        lw->list.DownTime = event->xbutton.time;
    lw->list.DidSelection = FALSE;
/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
    sel = lw->list.InternalList[item]->selected;
    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT)  ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)  ||
  	 (lw->list.SelectionPolicy == XmEXTENDED_SELECT))  &&
  	 ((!lw->list.AppendInProgress)                      ||
         ((!lw->list.AddMode) && 
          (lw->list.KbdSelection) &&
    	  (lw->list.SelectionPolicy == XmMULTIPLE_SELECT))))
    {
	for (i = 0; i < lw->list.itemCount; i++)
        {
/*            lw->list.InternalList[i]->last_selected = FALSE; */
	    if(lw->list.InternalList[i]->selected)
	    {
		lw->list.InternalList[i]->selected = FALSE;
		DrawItem((Widget) lw, i);
	    }
        }
    }

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
 	if (lw->list.Event & SHIFTDOWN )
            sel = lw->list.InternalList[lw->list.StartItem]->selected;
        else
     	if (lw->list.Event & CTRLDOWN )
        {
	    lw->list.InternalList[item]->selected =
	        !(lw->list.InternalList[item]->selected);
        }
        else
            if ((lw->list.Traversing) && (lw->list.AddMode))
            {
                lw->list.InternalList[item]->last_selected =
                    lw->list.InternalList[item]->selected;
                lw->list.InternalList[item]->selected =
                    !lw->list.InternalList[item]->selected;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }
    }
    else
        if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
            (lw->list.InternalList[item]->selected))
        {
	    lw->list.InternalList[item]->selected = FALSE;
        }
        else
            if (((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
                 (lw->list.SelectionPolicy == XmSINGLE_SELECT)) &&
                 (lw->list.AddMode))
            {
        	    lw->list.InternalList[item]->selected = !sel;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }


    DrawItem((Widget) lw, item);
    XmProcessTraversal( (Widget) lw, XmTRAVERSE_CURRENT);
    lw->list.LastHLItem = item;
    lw->list.MouseMoved = FALSE;
    lw->list.OldEndItem = lw->list.EndItem;
    lw->list.EndItem = item;

/****************
 *
 * If in extended select mode, and we're appending, select the
 * new range. Look and see if we need to unselect the old range
 * (the cases where the selection endpoint goes from one side of the
 * start to the other.)
 *
 ****************/
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
	(lw->list.Event & SHIFTDOWN))
	{
            start = lw->list.StartItem;
            end = lw->list.OldEndItem;
            i = item;
            if (start < end)
            {
                if (i > end)
                    SelectRange(lw, end+1, item, sel);
                else
                    if ((i < end) &&
                        (i >= start))
                        RestoreRange(lw, i+1, end, FALSE);
                    else
                        if (i < start)
                        {
			    if (sel)
			        SelectRange(lw, start+1, end, FALSE);
			    else
                                RestoreRange(lw, start+1, end, FALSE);
                            SelectRange(lw, item, start, sel);
                        }
            }
            if (start > end)
            {
                if (i < end)
                    SelectRange(lw, item, end+1, sel);
                else
                    if ((i > end) &&
                        (i <= start))
                        RestoreRange(lw, end, i-1, FALSE);
                    else
                        if (i > start)
                        {
			    if (sel)
			        SelectRange(lw, end, start-1, FALSE);
			    else
                                RestoreRange(lw, end, start-1, FALSE);
                            SelectRange(lw, start, item, sel);
                        }
            }
            if (start == end)
                SelectRange(lw, start, item, sel);
            if (lw->list.AutoSelect)
            {
		ClickElement(lw,NULL, FALSE);
	    }
	    return;
	}
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.StartItem = item;

    if ((lw->list.AutoSelect) &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
    {
	ClickElement(lw,NULL, FALSE);
    }
}

/***************************************************************************
 *									   *
 * KbdSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && lw->list.AddMode)
    {
        lw->list.Event |= CTRLDOWN;
	lw->list.AppendInProgress = TRUE;
    }
    SelectElement( (Widget) lw, event, params, num_params) ;
    lw->list.KbdSelection = FALSE;
}

/***************************************************************************
 *									   *
 * Element UnSelect - Handle the button up event.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
UnSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
UnSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int		item;
    char 	levent;

    if (!lw->list.itemCount) return;
#ifdef NOVELL
	if (!(LPART(lw).Event & BUTTONDOWN))
		return;

	item = WhichItem(lw, True /* static_rows_as_topleave */,
					event->xbutton.x, event->xbutton.y);
	if (item < TOP_VIZ_ITEM(lw))		/* TOPLEAVE */
		item = TOP_VIZ_ITEM(lw) + LPART(lw).col_info;
	else if (item > BOT_VIZ_ITEM(lw))	/* BOTTOMLEAVE */
		item = BOT_VIZ_ITEM(lw) - LPART(lw).cols+ LPART(lw).col_info+ 1;

		/* is it possible that WhichItem() return a value < 0 */
	if (item >= LPART(lw).itemCount)
		item = LPART(lw).itemCount - 1;
#else /* NOVELL */
    item = WhichItem(lw,event->xbutton.y);
    if (item < lw->list.top_position) item = lw->list.top_position;
    if (item > (lw->list.top_position+ lw->list.visibleItemCount)) 
        item = (lw->list.top_position+ lw->list.visibleItemCount - 1);
    if (item >= lw->list.itemCount)
        item = lw->list.itemCount - 1;

    if (!(lw->list.Event & BUTTONDOWN)) return;
#endif /* NOVELL */

    if (!lw->list.KbdSelection)
    {
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.OldEndItem = lw->list.EndItem;
    }
    if (lw->list.Traversing)
    {
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = item;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
    }
    else
         lw->list.CurrentKbdItem = item;

    levent = lw->list.Event;
    lw->list.Event = 0;
   if (!((lw->list.AutoSelect) && !(levent & (SHIFTDOWN | CTRLDOWN))  &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))    &&       /* GASP! */
	 (!(lw->list.AutoSelect && lw->list.DidSelection)))
    {
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);
        else
            ClickElement(lw,event,FALSE);
    }
    else
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);
    if (lw->list.AutoSelect)
       UpdateSelectedList(lw);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    lw->list.AppendInProgress = FALSE;
}


/***************************************************************************
 *									   *
 * KbdUnSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdUnSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdUnSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    UnSelectElement((Widget) lw, event, params, num_params) ;
    lw->list.KbdSelection = FALSE;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Shift Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    SelectElement((Widget) lw, event, params, num_params);

}

/************************************************************************
 *									*
 * Shift UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement((Widget) lw, event, params, num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Ctrl Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CtrlSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i,j;
    
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (CTRLDOWN);
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;

/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
#ifndef OSF_v1_2_4
    i = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldStartItem : lw->list.OldEndItem;
    j = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldEndItem : lw->list.OldStartItem;
#else /* OSF_v1_2_4 */
    i = MIN(lw->list.OldStartItem, lw->list.OldEndItem);
    j = MAX(lw->list.OldStartItem, lw->list.OldEndItem);
#endif /* OSF_v1_2_4 */
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;


    SelectElement((Widget)lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Ctrl UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CtrlUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard Shift Select						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdShiftSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdShiftSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;
    KbdSelectElement((Widget)lw,event,params,num_params);
}

/************************************************************************
 *									*
 * Keyboard Shift UnSelect						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdShiftUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdShiftUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Ctrl Select							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCtrlSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCtrlSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i, j;

    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdSelectElement((Widget)lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;

/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
#ifndef OSF_v1_2_4
    i = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldStartItem : lw->list.OldEndItem;
    j = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldEndItem : lw->list.OldStartItem;
#else /* OSF_v1_2_4 */
    i = MIN(lw->list.OldStartItem, lw->list.OldEndItem);
    j = MAX(lw->list.OldStartItem, lw->list.OldEndItem);
#endif /* OSF_v1_2_4 */
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;

    KbdSelectElement((Widget)lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Keyboard Ctrl UnSelect			        		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCtrlUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCtrlUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdUnSelectElement((Widget)lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard Activate                                                    *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdActivate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    XmParentInputActionRec  p_event ;
    int		i;
    
    if (!lw->list.itemCount || !lw->list.items ||
	LPART(wid).itemCount <= FIRST_ITEM(wid)) return;

    lw->list.AppendInProgress = FALSE;


    if ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
    {
        for (i = 0; i < lw->list.itemCount; i++)
        {
            if (lw->list.InternalList[i]->selected)
            {
                lw->list.InternalList[i]->selected = FALSE;
                lw->list.InternalList[i]->last_selected = FALSE;
    		DrawItem((Widget) lw, i);
            }
        }
    }

    lw->list.LastHLItem = lw->list.CurrentKbdItem;
    lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
    lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
    DrawItem((Widget) lw, lw->list.CurrentKbdItem);

    DefaultAction(lw,event);
    lw->list.Event = 0;
    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_ACTIVATE ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    _XmParentProcess(XtParent(lw), (XmParentProcessData) &p_event);

}

/************************************************************************
 *									*
 * Keyboard Cancel
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCancel( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCancel(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/* BEGIN OSF Fix CR 5117 */
/* END OSF Fix CR 5117 */
    XmParentInputActionRec  p_event ;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_CANCEL ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    if (!(lw->list.Event & BUTTONDOWN))		/* Only if not selecting */
    {
        if (_XmParentProcess(XtParent(lw), (XmParentProcessData) &p_event)) 
	    return;
    }

    if (((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
         (lw->list.SelectionPolicy != XmBROWSE_SELECT))  ||
        !(lw->list.Event & BUTTONDOWN))
        return;

    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
    lw->list.DragID = 0;

/* BEGIN OSF Fix CR 5117 */
    RestoreRange(lw, 0, lw->list.itemCount - 1, TRUE);
/* END OSF Fix CR 5117 */

    lw->list.StartItem = lw->list.OldStartItem;
    lw->list.EndItem = lw->list.OldEndItem;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
    if ((lw->list.AutoSelect) &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
    {
        ClickElement(lw,NULL, FALSE);
    }

}


/************************************************************************
 *									*
 * Keyboard toggle Add Mode                                             *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdToggleAddMode( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdToggleAddMode(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
        XmListSetAddMode( (Widget) lw, !(lw->list.AddMode));
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Select All                                                  *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdSelectAll( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdSelectAll(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i;
    lw->list.AppendInProgress = FALSE;
    if ((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
        (lw->list.SelectionPolicy != XmMULTIPLE_SELECT))
    {
        for (i = 0; i < lw->list.itemCount; i++)
            if (lw->list.InternalList[i]->selected)
            {
                lw->list.InternalList[i]->selected = FALSE;
                lw->list.InternalList[i]->last_selected = FALSE;
                DrawItem((Widget) lw, i);
            }
        lw->list.LastHLItem = lw->list.CurrentKbdItem;
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
        DrawItem((Widget) lw, lw->list.CurrentKbdItem);
    }
    else
        for (i = 0; i < lw->list.itemCount; i++)
            if (!(lw->list.InternalList[i]->selected))
            {
                lw->list.InternalList[i]->selected = TRUE;
                lw->list.InternalList[i]->last_selected = TRUE;
                DrawItem((Widget) lw, i);
            }

    ClickElement(lw,event,FALSE);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard DeSelect All                                                *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdDeSelectAll( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdDeSelectAll(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i, j;

    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
        (lw->list.SelectionPolicy == XmBROWSE_SELECT)) &&
        !(lw->list.AddMode))
	return;
	    
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && !(lw->list.AddMode)
        && (_XmGetFocusPolicy( (Widget) lw) == XmEXPLICIT))
        j = lw->list.CurrentKbdItem;
    else
        j = (-1);

    lw->list.AppendInProgress = FALSE;
    for (i = 0; i < lw->list.itemCount; i++)
        if ((lw->list.InternalList[i]->selected) && (i != j))
        {
            lw->list.InternalList[i]->selected = FALSE;
            lw->list.InternalList[i]->last_selected = FALSE;
            DrawItem((Widget) lw, i);
        }
    ClickElement(lw,event,FALSE);
    lw->list.Event = 0;
}


/***************************************************************************
 *									   *
 * DefaultAction - call the double click callback.			   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DefaultAction( lw, event )
        XmListWidget lw ;
        XEvent *event ;
#else
DefaultAction(
        XmListWidget lw,
        XEvent *event )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	Boolean		sv_auto_select = LPART(lw).AutoSelect;
	unsigned char	sv_selection_policy = LPART(lw).SelectionPolicy;

		/* Change the two values below, so that ClickElement()
		 * can exercise the right part of code */
			/* SO, UpdateSelectedList() will be called */
	LPART(lw).AutoSelect = False;
			/* SO, selected_items, selected_item_positions, and
			 * selected_item_count will be initialized */
	LPART(lw).SelectionPolicy = XmEXTENDED_SELECT;

	ClickElement(lw, event, True /* default_action */);

			/* undo what we did! */
	LPART(lw).AutoSelect = sv_auto_select;
	LPART(lw).SelectionPolicy = sv_selection_policy;

			/* do ClickElement() didn't do! */
	LPART(lw).DownCount = 0;
#else /* NOVELL */
    XmListCallbackStruct cb;
    int	item;
    int i, SLcount;
    
    item = lw->list.LastHLItem;
    lw->list.DidSelection = TRUE;

/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.reason = XmCR_DEFAULT_ACTION;
    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);
    cb.selected_item_count = 0;
    cb.selected_items = NULL;
    cb.selected_item_positions = NULL;

    UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if (lw->list.selectedItems && lw->list.selectedItemCount)
      {
	cb.selected_items = 
	  (XmString *)ALLOCATE_LOCAL(sizeof(XmString) * SLcount);
	cb.selected_item_positions = 
	  (int *)ALLOCATE_LOCAL(sizeof(int) * SLcount);
	for (i = 0; i < SLcount; i++)
	  {
	    cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
	    cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	  }
      }
    cb.selected_item_count = SLcount;

    XtCallCallbackList((Widget) lw,lw->list.DefaultCallback,&cb);

    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
    DEALLOCATE_LOCAL((char*)cb.selected_items);
    DEALLOCATE_LOCAL((char*)cb.selected_item_positions);
    XmStringFree(cb.item);

    lw->list.DownCount = 0;
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * ClickElement - invoked for all selection actions other than double	*
 * click.  This fills out the callback record and invokes the		*
 * appropriate callback.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClickElement( lw, event, default_action )
        XmListWidget lw ;
        XEvent *event ;
        Boolean default_action ;
#else
ClickElement(
        XmListWidget lw,
        XEvent *event,
#if NeedWidePrototypes
        int default_action )
#else
        Boolean default_action )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int			item, SLcount, i;
    XmListCallbackStruct cb;

    item = lw->list.LastHLItem;

    lw->list.DidSelection = TRUE;
/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);

    if (lw->list.AutoSelect)
    {
       ClearSelectedList(lw);
       BuildSelectedList(lw, FALSE);   /* Don't commit in auto mode. Yuk. */
    }
    else
	 UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (lw->list.selectedItems && lw->list.selectedItemCount)
    	{
	    cb.selected_items = 
	      (XmString *)ALLOCATE_LOCAL(sizeof(XmString) * SLcount);
            cb.selected_item_positions =
                            (int *)ALLOCATE_LOCAL(sizeof(int) * SLcount);
	    for (i = 0; i < SLcount; i++)
	    {
		cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
                cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	    }
	}
    }
/* BEGIN OSF Fix CR 4576 */
    cb.selected_item_count = SLcount;
/* END OSF Fix CR 4576 */

#ifdef NOVELL
  {
	XtCallbackList	cb_list = NULL;

	if (default_action)
	{
		cb.reason = XmCR_DEFAULT_ACTION;
		cb_list = LPART(lw).DefaultCallback;
	}
	else switch(LPART(lw).SelectionPolicy)
	{
		case XmSINGLE_SELECT:
			cb.reason = XmCR_SINGLE_SELECT;
			cb_list = LPART(lw).SingleCallback;
			break;
		case XmBROWSE_SELECT:
			cb.reason = XmCR_BROWSE_SELECT;
			cb_list = LPART(lw).BrowseCallback;
			break;
		case XmMULTIPLE_SELECT:
			cb.reason = XmCR_MULTIPLE_SELECT;
			cb_list = LPART(lw).MultipleCallback;
			break;
		case XmEXTENDED_SELECT:
			cb.reason = XmCR_EXTENDED_SELECT;
			cb.selection_type = LPART(lw).SelectionType;
			cb_list = LPART(lw).ExtendCallback;
			break;
	}
	XtCallCallbackList((Widget)lw, cb_list, &cb);
  }
#else /* NOVELL */
    if (default_action)
    {
        cb.reason = XmCR_DEFAULT_ACTION;
        XtCallCallbackList((Widget) lw,lw->list.DefaultCallback,&cb);
    }
    else
        switch(lw->list.SelectionPolicy)
        {
            case XmSINGLE_SELECT:
		    cb.reason = XmCR_SINGLE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.SingleCallback,&cb);
		    break;
	    case XmBROWSE_SELECT:
		    cb.reason = XmCR_BROWSE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.BrowseCallback,&cb);
		    break;
	    case XmMULTIPLE_SELECT:
		    cb.reason = XmCR_MULTIPLE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.MultipleCallback,&cb);
		    break;
	    case XmEXTENDED_SELECT:
	            cb.reason = XmCR_EXTENDED_SELECT;
		    cb.selection_type = lw->list.SelectionType;
		    XtCallCallbackList((Widget) lw,lw->list.ExtendCallback,&cb);
		    break;
        }
#endif /* NOVELL */

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (SLcount)
    	{
	    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
	    DEALLOCATE_LOCAL((char *) cb.selected_items);
	    DEALLOCATE_LOCAL((char *) cb.selected_item_positions);
	}
    }

    XmStringFree(cb.item);
}

/************************************************************************
 *									*
 * ListFocusIn								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListFocusIn( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListFocusIn(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->primitive.traversal_on &&
        (_XmGetFocusPolicy( (Widget) lw) == XmEXPLICIT) &&
        (event->xfocus.send_event))
	lw->list.Traversing = TRUE;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    _XmPrimitiveFocusIn( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *									*
 * ListFocusOut								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListFocusOut( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListFocusOut(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Traversing)) return;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.Traversing = FALSE;
    _XmPrimitiveFocusOut( (Widget) lw, event, NULL, NULL);
}


/************************************************************************
 *									*
 * BrowseScroll - timer proc that scrolls the list if the user has left *
 *		the window with the button down. If the button has been *
 *		released, call the standard click stuff.		*
 *									*
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
    XmListWidget lw = (XmListWidget) closure ;
    int		item, newitem;
    Boolean     vLeave = TRUE;
    Boolean     hLeave = TRUE;
    unsigned long interval;

    if (lw->list.DragID == 0) return;

    lw->list.DragID = 0;
/****************
 *
 * If the button went up, remove the timeout and call the cselection code.
 *
 ****************/
    if (!(lw->list.Event & BUTTONDOWN))
    {
	if (lw->list.DownCount > 1)
            DefaultAction(lw,NULL);
	else
            ClickElement(lw,NULL,FALSE);
        if (lw->list.Traversing)
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
            lw->list.CurrentKbdItem = lw->list.LastHLItem;;
	return;
    }
    item = lw->list.LastHLItem;
/****************
 *
 * See if the user moved out the top of the list and there's another
 * element to go to.
 *
 ****************/
    if (lw->list.LeaveDir & TOPLEAVE)
    {
     	if ((lw->list.top_position <= 0) ||
            !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
#ifdef NOVELL
		/* top_position must be > 0 when it's here */
	    LPART(lw).top_position--;
		/* We need `x' to compute which column but we don't
		 * have this info at the time when we are here, so we
		 * let WhichItem() set `col_info' for us. */
            item = TOP_VIZ_ITEM(lw) + LPART(lw).col_info;
            vLeave = FALSE;
#else /* NOVELL */
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
	    lw->list.top_position--;
            item = lw->list.top_position;
            vLeave = FALSE;
#endif /* NOVELL */
        }
    }
/****************
 *
 * Now see if we went off the end and need to scroll up
 *
 ****************/
    if (lw->list.LeaveDir & BOTTOMLEAVE)
    {
#ifdef NOVELL
		/* We need `x' to compute which column but we don't
		 * have this info at the time when we are here, so we
		 * let WhichItem() set `col_info' for us. */
	newitem = BOT_VIZ_ITEM(lw) + LPART(lw).col_info + 1;
#else /* NOVELL */
        newitem = lw->list.top_position + lw->list.visibleItemCount;
#endif /* NOVELL */
	if ((newitem >= lw->list.itemCount) ||
	    !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
#ifndef NOVELL
	/* DrawList() will handle it */
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
#endif /* NOVELL */
            lw->list.top_position++;
    	    item = newitem;
            vLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the right and need to scroll left
 *
 ****************/
    if (lw->list.LeaveDir & LEFTLEAVE)
    {
	if ((lw->list.hOrigin <= 0) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
#ifndef NOVELL
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
#endif /* NOVELL */
            lw->list.hOrigin -= lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the left and need to scroll right
 *
 ****************/
    if (lw->list.LeaveDir & RIGHTLEAVE)
    {
	if ((lw->list.hOrigin >= lw->list.hmax - lw->list.hExtent) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
#ifndef NOVELL
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
#endif /* NOVELL */
            lw->list.hOrigin += lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
    if (vLeave && hLeave) return;
    if (!vLeave)
        SetVerticalScrollbar(lw);
    if (!hLeave)
        SetHorizontalScrollbar(lw);
    DrawList(lw, NULL, TRUE);

    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
    else
        interval = 100;

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;

    if (item != lw->list.LastHLItem)
        HandleNewItem(lw, item, lw->list.LastHLItem);
    XSync (XtDisplay (lw), False);
    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) lw),
                       (unsigned long) interval, BrowseScroll, (XtPointer) lw);
}

/************************************************************************
 *									*
 * ListLeave - If the user leaves in Browse or Extended Select mode	*
 *	       with the button down, set up a timer to scroll the list	*
 *	       elements.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ListLeave( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListLeave(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    unsigned long interval;

    if ((_XmGetFocusPolicy( (Widget) lw) == XmPOINTER) &&
        (lw->primitive.highlight_on_enter))
    {
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.Traversing = FALSE;
    }

    if (((lw->list.SelectionPolicy != XmBROWSE_SELECT) &&
         (lw->list.SelectionPolicy != XmEXTENDED_SELECT)) ||
        !(lw->list.Event & BUTTONDOWN)) return;

    lw->list.LeaveDir = 0;
    if (event->xcrossing.y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->xcrossing.y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->xcrossing.x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->xcrossing.x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;
    if (lw->list.LeaveDir == 0)
    {
        lw->list.DragID = 0;
        return;
    }
    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.initial_delay;
    else
        interval = 200;

    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) lw),
                       (unsigned long) interval, BrowseScroll, (XtPointer) lw);
    _XmPrimitiveLeave( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *									*
 * ListEnter - If there is a drag timeout, remove it.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListEnter( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListEnter(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }
    if ((_XmGetFocusPolicy( (Widget) lw) == XmPOINTER) &&
        (lw->primitive.highlight_on_enter))
    {
	lw->list.Traversing = TRUE;
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    }
    _XmPrimitiveEnter( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *                                                                      *
 * MakeItemVisible - scroll the list (if needed) such that the given    *
 * item is visible                                                      *
 *                                                                      *
 ************************************************************************/
#ifdef NOVELL
static Boolean 
#ifdef _NO_PROTO
MakeItemVisible( lw, item, delta_row )
        XmListWidget	lw;
        int		item;
	int		delta_row;
#else
MakeItemVisible(
        XmListWidget	lw,
        int		item,
	int		delta_row)
#endif /* _NO_PROTO */
#else /* NOVELL */
static void 
#ifdef _NO_PROTO
MakeItemVisible( lw, item )
        XmListWidget lw ;
        int item ;
#else
MakeItemVisible(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
#endif /* NOVELL */
{
#ifdef NOVELL
	unsigned long	where;
	int		ht, x, y, wd, hi;
	Boolean		adj_hsb, draw_list = False;

		/* if list is not in a scrolled window, then return now! */
	if (!LPART(lw).Mom)
		return(draw_list);

	if ((ht = LPART(lw).HighlightThickness) < 1)
		ht = 0;

		/* item is currently in view, do nothing... */
	if (CalcItemBound((Widget)lw, item, ht, &where, &x, &y, &wd, &hi) > 0)
		return(draw_list);

	adj_hsb = True;
	if ( (where & ITEM_ON_TOP) || (where & ITEM_ON_BELOW) )
	{ /* the row is not in view */
		if (where & ITEM_ON_VIEW)
			adj_hsb = False;

		if (LPART(lw).vScrollBar)
		{
			draw_list = CalibrateVsb(
					(Widget)lw, item, where, delta_row);
		}
	}
	/* or, else, the row is in view, but not the item */

	if (adj_hsb && LPART(lw).hScrollBar)
	{
		draw_list = True;
		CalibrateHsb((Widget)lw, item, where);
	}

	if (draw_list)
		DrawList(lw, NULL, True);

	return(draw_list);
#else /* NOVELL */
    if (item < lw->list.top_position)
    {
     	if (lw->list.vScrollBar)
	{
	    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	    lw->list.top_position = item;
            DrawList(lw, NULL, TRUE);
            SetVerticalScrollbar(lw);
	}
    }
    if (item >= (lw->list.top_position + lw->list.visibleItemCount))
    {
     	if (!(lw->list.vScrollBar)) return;
	DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
        lw->list.top_position = item - (lw->list.visibleItemCount -1);
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}


#ifdef NOVELL
/************************************************************************
 *									*
 * MColumnPrevElement - called when the user hits Up arrow.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MColumnPrevElement( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MColumnPrevElement(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int item, olditem;

    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;

    if (!LPART(lw).itemCount)
		return;
    if (LPART(lw).cols<2) {
                NormalPrevElement(w, event, params, num_params);
                return;
    }
    if ((item = LPART(lw).CurrentKbdItem - LPART(lw).cols) < 
		FIRST_ITEM(lw))
		return;

    if (!LPART(lw).Mom && item < TOP_VIZ_ITEM(lw))
		return;

    olditem = LPART(lw).CurrentKbdItem;
    LPART(lw).CurrentKbdItem = item;

    if (!MakeItemVisible(lw, item, 0))
    { /* List not drawn, so take care the focus item visual */
		DrawHighlight(lw, olditem, False);
		DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
    }

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * MColumnNextElement - called when the user hits Down arrow.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MColumnNextElement( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
MColumnNextElement(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int item, olditem;

    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;

    if (!LPART(lw).itemCount)
		return;
    if (LPART(lw).cols<2) {
		NormalNextElement(w, event, params, num_params);
		return;
    }

    if ((item = LPART(lw).CurrentKbdItem + LPART(lw).cols) >= 
		LPART(lw).itemCount)
		return;

    if (!LPART(lw).Mom && item > BOT_VIZ_ITEM(lw))
		return;

    olditem = LPART(lw).CurrentKbdItem;
    LPART(lw).CurrentKbdItem = item;

    if (!MakeItemVisible(lw, item, 0))
    { /* List not drawn, so take care the focus item visual */
		DrawHighlight(lw, olditem, False);
		DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
    }

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}
#endif /* NOVELL */

/************************************************************************
 *									*
 * PrevElement - called when the user hits Up arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PrevElement( lw, event, params, num_params )
        XmListWidget lw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PrevElement(
        XmListWidget lw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    int item, olditem;

#ifdef NOVELL

	if (!LPART(lw).itemCount)
		return;
	if ((item = LPART(lw).CurrentKbdItem - 1) < FIRST_ITEM(lw))
		return;

		/* I don't really understand why the code is doing the
		 * check below, it looks like the author wants to disable
		 * this feature if a list is not in the scrolled window,
		 * so just follow his/her wishes. */
	if (!LPART(lw).Mom && item < TOP_VIZ_ITEM(lw))
		return;

	olditem = LPART(lw).CurrentKbdItem;
	LPART(lw).CurrentKbdItem = item;

	if (!MakeItemVisible(lw, item, 0))
	{ /* List not drawn, so take care the focus item visual */
		DrawHighlight(lw, olditem, False);
		DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
	}
#else /* NOVELL */
    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem - 1;
    if (item < 0) return;
    if ((!lw->list.Mom) &&
        (item < lw->list.top_position))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, olditem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
#endif /* NOVELL */

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * NextElement - called when the user hits Down arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
NextElement( lw, event, params, num_params )
        XmListWidget lw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NextElement(
        XmListWidget lw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    int item, olditem;

#ifdef NOVELL
	if (!LPART(lw).itemCount)
		return;
	if ((item = LPART(lw).CurrentKbdItem + 1) >= LPART(lw).itemCount)
		return;

		/* I don't really understand why the code is doing the,
		 * check below, it looks like the author wants to disable
		 * this feature if a list is not in the scrolled window,
		 * so just follow his/her wishes. */
	if (!LPART(lw).Mom && item > BOT_VIZ_ITEM(lw))
		return;

	olditem = LPART(lw).CurrentKbdItem;
	LPART(lw).CurrentKbdItem = item;

	if (!MakeItemVisible(lw, item, 0))
	{ /* List not drawn, so take care the focus item visual */
		DrawHighlight(lw, olditem, False);
		DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
	}
#else /* NOVELL */
    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem + 1;
    if (item >= lw->list.itemCount) return;
    if ((!lw->list.Mom) &&
        (item >= (lw->list.top_position + lw->list.visibleItemCount)))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, olditem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
#endif /* NOVELL */

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * Normal Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
NormalNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NormalNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    NextElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ShiftNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ShiftNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CtrlNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Next Element						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ExtendAddNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendAddNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}


/************************************************************************
 *									*
 * Normal Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
NormalPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NormalPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    PrevElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ShiftPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ShiftPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CtrlPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Prev Element						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ExtendAddPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendAddPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * PrevPage - called when the user hits PgUp                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdPrevPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdPrevPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	new_kbd_item, new_top;

	if (!LPART(wid).itemCount || !LPART(wid).top_position ||
	    !LPART(wid).Mom || LPART(wid).itemCount <= FIRST_ITEM(wid))
		return;

	new_top = LPART(wid).top_position - LPART(wid).visibleItemCount +
		  LPART(wid).static_rows + 1;

	if (new_top < 0)
		new_top = 0;
	new_kbd_item = LPART(wid).CurrentKbdItem + FIRST_ITEM(wid) -
		       LPART(wid).visibleItemCount * LPART(wid).cols + 1;

	if (new_kbd_item < 0)
		new_kbd_item = 0;

	SetNewTopNKbdItem(wid, new_top, new_kbd_item);
#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem, newtop;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (lw->list.top_position == 0) return;
    if (!lw->list.Mom) return;
    newtop = lw->list.top_position - lw->list.visibleItemCount + 1;
    if (newtop < 0) newtop = 0;
    item = lw->list.CurrentKbdItem - lw->list.visibleItemCount + 1;
    if (item < 0) item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = newtop;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT)  ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * NextPage - called when the user hits PgDn                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdNextPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdNextPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	new_kbd_item, new_top;
	int	rows, ignore;

	if (!LPART(wid).itemCount || !LPART(wid).Mom ||
	    LPART(wid).itemCount <= FIRST_ITEM(wid))
		return;

	CalcNumRows(wid, &rows, &ignore);
	if (LPART(wid).top_position >= rows - LPART(wid).visibleItemCount)
		return;

	new_top = LPART(wid).top_position + LPART(wid).visibleItemCount - 1;
	if (new_top > rows - LPART(wid).visibleItemCount)
		new_top = rows - LPART(wid).visibleItemCount;

	if (new_top == LPART(wid).top_position)
		return;

	new_kbd_item = LPART(wid).CurrentKbdItem - FIRST_ITEM(wid) +
		       LPART(wid).visibleItemCount * LPART(wid).cols - 1;

	if (new_kbd_item >= LPART(wid).itemCount)
		new_kbd_item = LPART(wid).itemCount - 1;

	SetNewTopNKbdItem(wid, new_top, new_kbd_item);
#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem, newtop;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    if (lw->list.top_position >=
        (lw->list.itemCount - lw->list.visibleItemCount)) return;
    newtop = lw->list.top_position + (lw->list.visibleItemCount - 1);
    if (newtop >= (lw->list.itemCount - lw->list.visibleItemCount))
        newtop = lw->list.itemCount - lw->list.visibleItemCount;
    item = lw->list.CurrentKbdItem + (lw->list.visibleItemCount - 1);
    if (item >= lw->list.itemCount)
        item = lw->list.itemCount - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = newtop;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
#endif /* NOVELL */
}
/************************************************************************
 *                                                                      *
 * KbdLeftChar - called when user hits left arrow.                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdLeftChar( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdLeftChar(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
#ifdef NOVELL
    if (LPART(lw).cols>1) {
        int item, olditem;

        if (!lw->list.Traversing) return;
        lw->list.AppendInProgress = FALSE;
        lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
        lw->list.SelectionType = XmINITIAL;


        if (!LPART(lw).itemCount)
                return;
        if ( (LPART(lw).CurrentKbdItem)%(LPART(lw).cols)==0 )
                return;
        else if ((item = LPART(lw).CurrentKbdItem - 1) < FIRST_ITEM(lw))
                return;

        if (!LPART(lw).Mom && item < TOP_VIZ_ITEM(lw))
                return;

        olditem = LPART(lw).CurrentKbdItem;
        LPART(lw).CurrentKbdItem = item;

        if (!MakeItemVisible(lw, item, 0))
        { /* List not drawn, so take care the focus item visual */
                DrawHighlight(lw, olditem, False);
                DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
        }
        if ((lw->list.AutoSelect) &&
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleNewItem(lw,item, olditem);
        else
            if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
        return;
    }
#endif /* NOVELL */
 
    XmListSetHorizPos( (Widget) lw, (lw->list.hOrigin - CHAR_WIDTH_GUESS));
}

/************************************************************************
 *                                                                      *
 * KbdLeftPage - called when user hits ctrl left arrow.                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdLeftPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdLeftPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, (lw->list.hOrigin - (lw->core.width - CHAR_WIDTH_GUESS -
                                        2 * (int )(lw->list.margin_width +
                                            lw->list.HighlightThickness +
                                            lw->primitive.shadow_thickness))));
}
/************************************************************************
 *                                                                      *
 * Begin Line - go to the beginning of the line                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BeginLine( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BeginLine(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, 0);
}

/************************************************************************
 *                                                                      *
 * KbdRightChar - called when user hits right arrow.                    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdRightChar( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdRightChar(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    int pos;

    if (!lw->list.Mom) return;
#ifdef NOVELL
    if (LPART(lw).cols >1) {
        int item, olditem;

        if (!lw->list.Traversing) return;
        lw->list.AppendInProgress = FALSE;
        lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
        lw->list.SelectionType = XmINITIAL;

        if (!LPART(lw).itemCount)
                return;
        if ((item = LPART(lw).CurrentKbdItem + 1) >=LPART(lw).itemCount)
                return;
        if ( item%(LPART(lw).cols) == 0 )
                return;

        if (!LPART(lw).Mom && item > BOT_VIZ_ITEM(lw))
                return;

        olditem = LPART(lw).CurrentKbdItem;
        LPART(lw).CurrentKbdItem = item;

        if (!MakeItemVisible(lw, item, 0))
        { /* List not drawn, so take care the focus item visual */
                DrawHighlight(lw, olditem, False);
                DrawHighlight(lw, LPART(lw).CurrentKbdItem, True);
        }

        if ((lw->list.AutoSelect) &&
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
        else
            if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
        return;
    }
#endif /* NOVELL */
    pos = lw->list.hOrigin + CHAR_WIDTH_GUESS; 

    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos( (Widget) lw, pos);
}

/************************************************************************
 *                                                                      *
 * KbdRightPage - called when user hits ctrl right arrow.               *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdRightPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdRightPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int pos; 

    if (!lw->list.Mom) return;

    pos = lw->list.hOrigin + (lw->core.width - CHAR_WIDTH_GUESS -
                             2 * (int )(lw->list.margin_width +
                                        lw->list.HighlightThickness +
                                        lw->primitive.shadow_thickness));
    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos( (Widget) lw, pos);
}
/************************************************************************
 *                                                                      *
 * End Line - go to the end of the line                                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EndLine( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
EndLine(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, lw->list.hmax - lw->list.hExtent);
}

/************************************************************************
 *                                                                      *
 * TopItem - go to the top item                                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TopItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TopItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	GotoTopOrEnd(wid, (unsigned long)ITEM_ON_TOP);

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int newtop;
    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) 
        newtop = lw->list.top_position;
    else
        newtop = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newtop;
    XmListSetPos( (Widget) lw, newtop + 1);
    if (!lw->list.AddMode)
        XmListSelectPos( (Widget) lw, newtop + 1, TRUE);
    lw->list.StartItem = newtop;
#endif /* NOVELL */
}
/************************************************************************
 *                                                                      *
 * EndItem - go to the bottom item                                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EndItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
EndItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	GotoTopOrEnd(wid, (unsigned long)ITEM_ON_BELOW);

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int newbot;
    if (!(lw->list.items && lw->list.itemCount)) return;

    if (!lw->list.Mom)
    {
        newbot = (lw->list.top_position + lw->list.visibleItemCount - 1);
        if (newbot >= (lw->list.itemCount - 1))
            newbot = lw->list.itemCount - 1;
    }
    else
         newbot = lw->list.itemCount - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newbot;
    XmListSetBottomPos( (Widget) lw, newbot + 1);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    if (!lw->list.AddMode)
        XmListSelectPos( (Widget) lw, newbot + 1, TRUE);
#endif /* NOVELL */
}
/************************************************************************
 *                                                                      *
 * ExtendTopItem - Extend the selection to the top item			*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendTopItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendTopItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	ExtendToTopOrBot(wid, True /* on_top */);

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;

    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;

    lw->list.Event |= (SHIFTDOWN);
    if (!lw->list.Mom)
        item = lw->list.top_position;
    else
        item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    lw->list.top_position = item;
    lw->list.CurrentKbdItem = item;
    DrawList(lw, NULL, TRUE);

    if (lw->list.vScrollBar)
        SetVerticalScrollbar(lw);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;
#endif /* NOVELL */
}
/************************************************************************
 *                                                                      *
 * ExtendEndItem - extend the selection to the bottom item		*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendEndItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendEndItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	ExtendToTopOrBot(wid, False /* !on_top */);

#else /* NOVELL */
        XmListWidget lw = (XmListWidget) wid ;
    int item, newitem, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;

    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;
    lw->list.Event |= (SHIFTDOWN);
    newitem = lw->list.itemCount - lw->list.visibleItemCount;
    item = lw->list.itemCount - 1;
    if (!lw->list.Mom)
    {
        newitem = lw->list.top_position;
        item = newitem + lw->list.visibleItemCount;
	if (item >= lw->list.itemCount)
	    item = lw->list.itemCount - 1;
    }
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    lw->list.CurrentKbdItem = item;
    lw->list.top_position = newitem;
    DrawList(lw, NULL, TRUE);
    if (lw->list.vScrollBar)
        SetVerticalScrollbar(lw);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;
#endif /* NOVELL */
}

/***************************************************************************
 *									   *
 * ListItemVisible - make the current keyboard item visible.  		   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListItemVisible( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListItemVisible(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define LW	(XmListWidget)wid

	int	delta_row;
    
	if (!LPART(wid).itemCount || !LPART(wid).Mom)
		return;
    
	if (*num_params == 0)
	{
		int	item;

		item = WhichItem(LW, False /* static_rows_as_topleave */,
					event->xbutton.x, event->xbutton.y);
		if (item < 0 || item > LPART(wid).itemCount ||
		    TITLE_ROW_ITEM(wid, item))
			return;

		if (!item)
			delta_row = 0;
		else
		{
			delta_row = (item / (int)LPART(wid).cols) -
				    LPART(wid).top_position -
				    LPART(wid).static_rows;
		}
	}
	else
	{
		int	percentage;

		sscanf(*params, "%d", &percentage);
		if (percentage >= 100)
			percentage = 99;
		delta_row = ((LPART(wid).visibleItemCount -
			      (int)LPART(wid).static_rows) * percentage) / 100;
	}

		/* either it's a no-op because CurrentKbdItem is in view or
		 * the list is re-drawed */
	(void)MakeItemVisible(LW, LPART(wid).CurrentKbdItem, delta_row);

#undef LW

#else /* NOVELL */
    XmListWidget lw = (XmListWidget) wid ;
    int 	item, percentage;
    
    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    
    if (*num_params == 0)
    {
	item = WhichItem(lw,event->xbutton.y);
	if (item > 0)
	    item -=lw->list.top_position;
	if ((item < 0) || (item > lw->list.itemCount))
	    return;
    }
    else
    {
        sscanf(*params, "%d", &percentage);
	if (percentage == 100) percentage--;
   	item = (lw->list.visibleItemCount * percentage) /100;
    }
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.top_position = lw->list.CurrentKbdItem - item;
    if (lw->list.top_position < 0) 
        lw->list.top_position = 0;
    DrawList(lw, NULL, TRUE);
    SetVerticalScrollbar(lw);    
#endif /* NOVELL */
}

/***************************************************************************
 *									   *
 * ListCopyToClipboard - copy the current selected items to the clipboard. *
 *									   *
 *	This is a *sloow* process...					   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListCopyToClipboard( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListCopyToClipboard(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int			total_size;
	char *			total;
	XmListDragConvertStruct	data;
	XmString		clip_label;
	long			item_id;	/* clipboard item id */
	long			data_id;	/* clipboard data id */


	if (!LPART(wid).itemCount || !LPART(wid).selectedItemCount ||
	    !TreatSelectionData(
		wid, (unsigned long)(SETUP_SELECTED_LIST | SETUP_CT_DATA |
				     FREE_SELECTED_LIST),
		&data, &total_size, &total))
			return;

		/* The original code has memory leak if
		 * XmClipboardStartCopy() or XmClipboardCopy() is failed. */
#define DPY	XtDisplay(wid)
#define WIN	XtWindow(wid)

	clip_label = XmStringCreateLtoR("XM_LIST", XmFONTLIST_DEFAULT_TAG);

		/* Should we call XmClipboardEndCopy() if XmClipboardCopy()
		 * is failed? */
	if (XmClipboardStartCopy(
			DPY, WIN, clip_label, event->xkey.time,
			wid, NULL, &item_id) == ClipboardSuccess &&
	    XmClipboardCopy(
			DPY, WIN, item_id, "COMPOUND_TEXT", total,
			(unsigned long)(total_size + 1),
			0, &data_id) == ClipboardSuccess)
	{
		(void)XmClipboardEndCopy(DPY, WIN, item_id);
	}

	XtFree(total);
#undef DPY
#undef WIN

#else /* NOVELL */
    XmListWidget lw = (XmListWidget) wid ;
    XrmValue	from_val;
    XrmValue	to_val;
    Boolean     ok;
    register int i;
    size_t total_size = 0; 
    long item_id = 0;                        /* clipboard item id */
    long data_id = 0;                         /* clipboard data id */
    int status = 0;                          /* clipboard status  */
    XmString clip_label;
    char *ctstring,*total = NULL;

    if ((!(lw->list.items && lw->list.itemCount)) ||
        (!(lw->list.selectedItems && lw->list.selectedItemCount)))
	return;

    for (i = 0; i < lw->list.selectedItemCount; i++)
    {  /* Make sure we start good... */
        from_val.addr = (char *)lw->list.selectedItems[i];
	ok = _XmCvtXmStringToCT(&from_val, &to_val);
	total = (char *) to_val.addr;
	total_size = to_val.size;
	if (ok) 
	{
	    total = XtRealloc((char *)total, (total_size + NEWLINESTRING_LEN + 1));
    	    memcpy(&total[total_size], NEWLINESTRING, NEWLINESTRING_LEN);
	    total[total_size + NEWLINESTRING_LEN] = 0;
	    total_size = total_size + NEWLINESTRING_LEN;
	    break;
	}
    }
    for (i++; i < lw->list.selectedItemCount; i++)
    {
        from_val.addr = (char *)lw->list.selectedItems[i];
	ok = _XmCvtXmStringToCT(&from_val, &to_val);
	ctstring = (char *) to_val.addr;
	if (ctstring != NULL)
	{
	    total = XtRealloc((char *)total, (total_size + to_val.size +
	    				      NEWLINESTRING_LEN + 1));
	    memcpy(&total[total_size], ctstring, to_val.size);
    	    memcpy(&total[total_size + to_val.size], 
	    			NEWLINESTRING, NEWLINESTRING_LEN);
	    total[total_size + to_val.size + NEWLINESTRING_LEN] = 0;
	    total_size = total_size + to_val.size + NEWLINESTRING_LEN;
	}
    }
/****************
 *
 *  OK, we now have a big compound text chunk.  Just drop it on the
 *  clipboard!
 *
 ****************/
   if (total != NULL)
   {
      clip_label = XmStringCreateLtoR ("XM_LIST", XmFONTLIST_DEFAULT_TAG);
     /* start copy to clipboard */
      status = XmClipboardStartCopy (XtDisplay(lw), XtWindow(lw),
                                     clip_label, event->xkey.time, (Widget )lw,
				     NULL, &item_id);

      if (status != ClipboardSuccess) return;

     /* move the data to the clipboard */
      status = XmClipboardCopy (XtDisplay(lw), XtWindow(lw),
                                item_id, "COMPOUND_TEXT", total,
                      (unsigned long)(total_size+1), 0, &data_id);

      if (status != ClipboardSuccess) return;

     /* end the copy to the clipboard */
      status = XmClipboardEndCopy (XtDisplay(lw), XtWindow(lw),
                                   item_id);

      XtFree(total);
    }
#endif /* NOVELL */
}


static void
#ifdef _NO_PROTO
DragDropFinished( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
DragDropFinished(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmListDragConvertStruct * conv = (XmListDragConvertStruct *) closure;
    int i;
  
    for (i = 0; i < conv->num_strings; i++) {
       XmStringFree(conv->strings[i]);
    }
    XtFree((char *) conv->strings);
    XtFree((char *) conv);
}

/***************************************************************************
 *									   *
 * ListProcessDrag - drag the selected items				   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListProcessDrag( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListProcessDrag(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define N_TARGETS		1
#define N_ARGS			9

    int				item;
    XtCallbackRec		dnd_finish_cb[2];
    Atom			targets[N_TARGETS];
    XmListDragConvertStruct *	conv;
    Widget			drag_icon;
    Arg				args[N_ARGS];
    
	if (!LPART(wid).itemCount)
		return;

	if ((item = WhichItem((XmListWidget)wid,
			      False /* static_rows_as_topleave */,
			      event->xbutton.x, event->xbutton.y)) < 0 ||
	    item >= LPART(wid).itemCount)
		return;

	conv = (XmListDragConvertStruct *)XtMalloc(
					sizeof(XmListDragConvertStruct));

	conv->w = wid;
    
		/* The data should be set when XmNdropStartCallback is called
		 * because you don't really know whether the drop is
		 * sucessful or not at this point. Or, we should just
		 * setup everything in the front instead this routine
		 * does some, and convert_proc does other part when
		 * COMPOUND_TEXT is requested. Anyway, it seems to me
		 * there is room for improvement here... */

		/* The old code uses selectedItemCount/selectedIndices,
		 * that may cause problems, I use XmListGetSelectedPos()
		 * in TreatSelectionData(), the CDE fix just ran thru
		 * the list, see #else NOVELL part ... 5/2/94 */
	if (LPART(wid).InternalList[item]->selected)
	{
			/* The return value has to be True in this case */
		(void)TreatSelectionData(
				wid, (unsigned long)SETUP_SELECTED_LIST,
				conv, NULL, NULL);
	}
	else
	{
		conv->strings = (XmString *)XtMalloc(sizeof(XmString));
		conv->num_strings = 1;
		conv->strings[0] = XmStringCopy(LPART(wid).items[item]);
	}


/****************
 * OK, now start the drag...
 ****************/
	targets[0] = XmInternAtom(XtDisplay(wid), "COMPOUND_TEXT", False);
	drag_icon = _XmGetTextualDragIcon(wid);

	dnd_finish_cb[0].callback = DragDropFinished;
	dnd_finish_cb[0].closure = (XtPointer)conv;
	dnd_finish_cb[1].callback = (XtCallbackProc)NULL;
	dnd_finish_cb[1].closure = (XtPointer)NULL;

	XtSetArg(args[0], XmNcursorForeground, PPART(wid).foreground);
	XtSetArg(args[1], XmNcursorBackground, CPART(wid).background_pixel);
	XtSetArg(args[2], XmNsourceCursorIcon, drag_icon);
	XtSetArg(args[3], XmNexportTargets, targets);
	XtSetArg(args[4], XmNnumExportTargets, N_TARGETS);
	XtSetArg(args[5], XmNconvertProc, ListConvert);
	XtSetArg(args[6], XmNclientData, conv);
	XtSetArg(args[7], XmNdragDropFinishCallback, dnd_finish_cb);
	XtSetArg(args[8], XmNdragOperations, XmDROP_COPY);
	(void)XmDragStart(wid, event, args, N_ARGS);

#undef N_TARGETS
#undef N_ARGS

#else /* NOVELL */
    XmListWidget lw = (XmListWidget) wid ;
    register int i;
    int item = 0;
    static XtCallbackRec dragDropFinishCB[] =
    {
           {DragDropFinished, NULL},
           {NULL, NULL},
    };
    Atom targets[3];
    Cardinal num_targets = 0;
    XmListDragConvertStruct * conv;
    Widget drag_icon;
    Arg args[10];
    int n;
    
    if (!(lw->list.items && lw->list.itemCount))
	return;
    item = WhichItem(lw,event->xbutton.y);
    if ((item < 0) || (item >= lw->list.itemCount))
        return;

    conv = (XmListDragConvertStruct *)XtMalloc(sizeof(XmListDragConvertStruct));

    conv->w = wid;
    
    if (lw->list.InternalList[item]->selected) {
#ifdef NON_OSF_FIX	/* OSF contact # 18904 */
	int j = 0;
	/*
	 ** lw->list.selectedItemCount may not give the real number of selected
	 ** items; run through list and precalculate number of selected items.
	 */
	conv->num_strings = 0;
	for (i = 0; i < lw->list.itemCount; i++)
	    if (lw->list.InternalList[i]->selected)
                (conv->num_strings)++;

	conv->strings = (XmString *) XtMalloc(sizeof(XmString) * 
					      conv->num_strings);
        for (i = 0; i < lw->list.itemCount; i++) {
	    if (lw->list.InternalList[i]->selected)
		conv->strings[j++] = XmStringCopy(lw->list.items[i]);
	}
#else
       conv->strings = (XmString *) XtMalloc(sizeof(XmString) *
					    lw->list.selectedItemCount);
       conv->num_strings= lw->list.selectedItemCount;
       for (i = 0; i < lw->list.selectedItemCount; i++) {
          conv->strings[i] = XmStringCopy(lw->list.items[lw->list.selectedIndices[i]-1]);
       }
#endif
    } else {
       conv->strings = (XmString *) XtMalloc(sizeof(XmString));
       conv->num_strings = 1;
       conv->strings[0] = XmStringCopy(lw->list.items[item]);
    }


/****************
 *
 * OK, now start the drag...
 *
 ****************/
    targets[0] = XmInternAtom(XtDisplay(lw), "COMPOUND_TEXT", False);
    num_targets++;
    drag_icon = _XmGetTextualDragIcon(wid);

    n = 0;
    XtSetArg(args[n], XmNcursorForeground, lw->primitive.foreground);  n++;
    XtSetArg(args[n], XmNcursorBackground, lw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++;
    XtSetArg(args[n], XmNexportTargets, targets);  n++;
    XtSetArg(args[n], XmNnumExportTargets, num_targets);  n++;
    XtSetArg(args[n], XmNconvertProc, ListConvert);  n++;
    XtSetArg(args[n], XmNclientData, conv);  n++;
    dragDropFinishCB[0].closure = (XtPointer) conv;
    XtSetArg(args[n], XmNdragDropFinishCallback, dragDropFinishCB);  n++;
    XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
    (void) XmDragStart(wid, event, args, n);
#endif /* NOVELL */
}

/***************************************************************************
 *									   *
 * ListConvert - Convert routine for dragNDrop.				   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
ListConvert( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
ListConvert(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define COMPOUND_TEXT	XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False)
#define TIMESTAMP	XmInternAtom(XtDisplay(w), "TIMESTAMP", False)
#define TARGETS		XmInternAtom(XtDisplay(w), "TARGETS", False)
#define MOTIF_DROP	XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False)

	Arg			args[1];
	XmListDragConvertStruct *conv;
    
	if (*selection != MOTIF_DROP)
		return(False);

	XtSetArg (args[0], XmNclientData, &conv);
	XtGetValues (w, args, 1);

	if (*target == TARGETS) 
	{
#define MAX_TARGS	3
		int	target_count = 0 ;
		Atom *targs = (Atom *)XtMalloc(
					(unsigned)(MAX_TARGS * sizeof(Atom)));

		*value = (XtPointer)targs;
		*targs++ = *target /* TARGETS */; target_count++;
		*targs++ = TIMESTAMP; target_count++;
		*targs++ = COMPOUND_TEXT; target_count++;
		*type = XA_ATOM;
		*length = target_count;
		*format = 32;
		return(True);
#undef MAX_TARGS
	}
	else if (*target == COMPOUND_TEXT)
	{
		int			total_size;
		char *			total;

		*type = *target /*COMPOUND_TEXT*/;
		*format = 8;

		if (TreatSelectionData(w, (unsigned long)SETUP_CT_DATA,
						conv, &total_size, &total))
		{
			*value = (char *)total;
			*length = total_size;
			return(True);
		}
	}

	return(False);

#undef COMPOUND_TEXT
#undef TIMESTAMP
#undef TARGETS
#undef MOTIF_DROP

#else /* NOVELL */
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    Atom TIMESTAMP = XmInternAtom(XtDisplay(w), "TIMESTAMP", False);
    Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
    Atom MOTIF_DROP = XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False);
    int target_count = 0 ;
    XmListDragConvertStruct *conv;
    Arg args[1];
    int MAX_TARGS = 10;
    XrmValue	from_val;
    XrmValue	to_val;
    Boolean     ok = FALSE;
    int 	total_size = 0; 
    char *total = NULL;
    int i;
    
    if (*selection == MOTIF_DROP) {
       XtSetArg (args[0], XmNclientData, &conv);
       XtGetValues (w, args, 1);
    } else
       return False;

    if (*target == TARGETS) 
    {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));

      *value = (XtPointer) targs;
      *targs++ = TARGETS; target_count++;
      *targs++ = TIMESTAMP; target_count++;
      *targs++ = COMPOUND_TEXT; target_count++;
      *type = XA_ATOM;
      *length = target_count;
      *format = 32;
      return True;
   }

   if (*target == COMPOUND_TEXT)
   {
        *type = COMPOUND_TEXT;
        *format = 8;
	if (conv->num_strings == 1)	/* No trailing newline */
	{
	  from_val.addr = (char *)conv->strings[0];
	  if ((ok = _XmCvtXmStringToCT(&from_val, &to_val)))
	  {
	    total_size += to_val.size;
	    total = XtRealloc((char *)total, total_size);
	    memcpy(&total[total_size - to_val.size],
				   to_val.addr, to_val.size);
	  }
	}
	else	/* Newline separated and trailing (b.c.) */
	  for (i = 0; i < conv->num_strings; i++) {
	     from_val.addr = (char *)conv->strings[i];
	     if ( _XmCvtXmStringToCT(&from_val, &to_val) )
	     {
		ok = TRUE;
		total_size += to_val.size;
		total = XtRealloc((char *)total,
				   (total_size + NEWLINESTRING_LEN));
		memcpy(&total[total_size - to_val.size],
				       to_val.addr, to_val.size);
		memcpy(&total[total_size], NEWLINESTRING, NEWLINESTRING_LEN);

		total_size += NEWLINESTRING_LEN;
	     }
	 }
    }

    if (ok) 
    {
         *value = (char*)total;
         *length = total_size;
         return True;
    }

    return False;
#endif /* NOVELL */
}



/************************************************************************
 *									*
 * Spiffy API entry points						*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * XmListAddItem - add the item at the specified position.		*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItem( w, item, pos )
        Widget w ;
        XmString item ;
        int pos ;
#else
XmListAddItem(
        Widget w,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
#if NeedWidePrototypes
	int		select;
#else
	Boolean		select;
#endif /* NOVELL */

		/* Can't select a title item */
	if (pos > 0 && TITLE_ROW_ITEM(w, pos - 1))
		select = False;
	else
#ifndef OSF_v1_2_4
			/* Look like IGNORE_CHECK is no longer necessary, see
			 * `else OSF_v1_2_4' block below, they finally
			 * catch up... */
		select = True;
#else /* OSF_v1_2_4 */
		select = True | IGNORE_CHECK;
#endif /* OSF_v1_2_4 */
	APIAddItems((XmListWidget)w, &item, 1, pos, select);
#else /* NOVELL */
    XmListWidget lw = (XmListWidget) w;
#ifndef OSF_v1_2_4
    int intern_pos   = pos-1;
    Boolean	     bot = FALSE;

    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
	bot = TRUE;
    }

    if ((lw->list.Traversing) && 
	(intern_pos <= lw->list.CurrentKbdItem) &&
	!bot)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    AddItem(lw, item, intern_pos);
    AddInternalElement(lw, item, pos, OnSelectedList(lw,item), TRUE);

    if ((intern_pos <= lw->list.CurrentKbdItem) && !bot)
    {
        lw->list.CurrentKbdItem++;
        if ((lw->list.AutoSelect) &&
            ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
            lw->list.LastHLItem ++;
    }

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
/*    SetTraversal(lw);*/
#else /* OSF_v1_2_4 */
    APIAddItems(lw, &item, 1, pos, TRUE);
#endif /* OSF_v1_2_4 */
#endif /* NOVELL */
}
/***************************************************************************
 *									   *
 * APIAddItems - do all the work for the XmListAddItems and		   *
 * AddItemsUnselected functions.					   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
APIAddItems( lw, items, item_count, pos, select )
        XmListWidget lw ;
        XmString *items ;
        int item_count ;
        int pos ;
        Boolean select ;
#else
APIAddItems(
        XmListWidget lw,
        XmString *items,
        int item_count,
        int pos,
#if NeedWidePrototypes
	int select) 
#else
        Boolean select) 
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#ifdef OSF_v1_2_4
	int		select_pos = 0;
#endif /* OSF_v1_2_4 */
	register int	i;
	int		intern_pos, col;
	Boolean		bot = False, itemSelect, calc_cum_hi;
	Boolean		re_draw;

	if (item_count < 1 || !items || pos < 0)
		return;

	if (!((intern_pos = pos - 1) < 0 || intern_pos >= LPART(lw).itemCount))
	{
			/* We need to re-calc cum_hi if it's in the middle */
		calc_cum_hi = True;
		bot = False;
	}
	else
	{
		calc_cum_hi = False;
		intern_pos = LPART(lw).itemCount;
		pos = LPART(lw).itemCount + 1;
		bot = True;
	}

	for (i = 0; i < item_count; i++)
	{
		AddItem(lw, items[i], intern_pos + i);
#ifndef OSF_v1_2_4
		if (select & IGNORE_CHECK)
			itemSelect = select & ~IGNORE_CHECK;
		else if (itemSelect = select)
		{
			/* the logic is really weird and not consistant
			 * and causing on_select_list out of date, sigh! */
		    itemSelect = OnSelectedList(lw, items[i]);
		}
#else /* OSF_v1_2_4 */
		itemSelect = select && OnSelectedList(lw, items[i]);

		/* CR 5833:  Enforce selection_policy. */
		if (itemSelect && 
		    (LPART(lw).SelectionPolicy == XmSINGLE_SELECT ||
		     LPART(lw).SelectionPolicy == XmBROWSE_SELECT))
		{
			select_pos = pos + i;
			itemSelect = False;
		}

#endif /* OSF_v1_2_4 */
			/* AddInternalElement() should say which row
			 * should be re-calculated */
		if (AddInternalElement(lw, items[i], pos + i, itemSelect, True)
		    && !calc_cum_hi)
			calc_cum_hi = True;

			/* Readjust max_col_width[] here, it was originally
			 * in AddItem(), see reasons there for more info */
		col = (intern_pos + i) % (int)LPART(lw).cols;
		if ((int)LPART(lw).InternalList[intern_pos + i]->width >
		    (int)LPART(lw).max_col_width[col])
			LPART(lw).max_col_width[col] =
				LPART(lw).InternalList[intern_pos + i]->width;
	}

	if (calc_cum_hi)
		CalcCumHeight((Widget)lw);

	if (intern_pos <= lw->list.CurrentKbdItem &&
	    LPART(lw).itemCount > 1 && !bot)
	{
			/* Traversing is set when FocusIn and Enter */
		if (LPART(lw).Traversing && 
		    intern_pos <= LPART(lw).CurrentKbdItem && !bot)
			DrawHighlight(lw, LPART(lw).CurrentKbdItem, False);

		LPART(lw).CurrentKbdItem += item_count;

#ifndef OSF_v1_2_4
			/* Note that XmListDelete API didn't include
			 * AutoSelect checking, so what's the rule,
			 * finally they catch up.. see else part below */
		if (LPART(lw).AutoSelect &&
		    (LPART(lw).SelectionPolicy == XmEXTENDED_SELECT ||
		     LPART(lw).SelectionPolicy == XmBROWSE_SELECT))
#else /* OSF_v1_2_4 */
			/* CR 5804:  Don't check lw->list.AutoSelect here. */
		if (LPART(lw).SelectionPolicy == XmEXTENDED_SELECT ||
		    LPART(lw).SelectionPolicy == XmBROWSE_SELECT)
#endif /* OSF_v1_2_4 */
			LPART(lw).LastHLItem += item_count;
	}

#ifdef OSF_v1_2_4

    /* CR 5833: Enforce single/browse selection_policy. */
    if (select_pos)
      {
	/* Select the last of the matching new items. */
	lw->list.InternalList[select_pos - 1]->selected = TRUE;
	lw->list.InternalList[select_pos - 1]->last_selected = TRUE;
	lw->list.InternalList[select_pos - 1]->LastTimeDrawn = FALSE;

	/* Deselect the previous selected item. */
	if (lw->list.selectedItemCount > 0)
	  {
	    for (i = 0; 
		 i < MIN(lw->list.itemCount,
			 lw->list.top_position + lw->list.visibleItemCount);
		 i++)
	      if (lw->list.InternalList[i]->selected && (i+1 != select_pos))
		{
		  lw->list.InternalList[i]->selected = FALSE;
		  lw->list.InternalList[i]->last_selected = FALSE;
		  DrawItem((Widget) lw, i);
		  break;
		}

	    UpdateSelectedList(lw);
	  }
      }

#endif /* OSF_v1_2_4 */

	re_draw = (intern_pos < BOT_VIZ_ITEM(lw) + 1) ? True : False;

	CalcNewSize(lw, re_draw);

#else /* NOVELL */
    int intern_pos   = pos-1;
    Boolean      bot = FALSE, itemSelect;
#ifdef OSF_v1_2_4
    int select_pos   = 0;
#endif /* OSF_v1_2_4 */
    register int i;

    if ((items == NULL)     ||
        (item_count == 0))
        return;

    if (intern_pos < 0 || intern_pos >= lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
	bot = TRUE;
    }

    if ((lw->list.Traversing) && 
        (intern_pos <= lw->list.CurrentKbdItem) &&
	!bot)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        AddItem(lw, items[i], intern_pos + i);
#ifndef OSF_v1_2_4
	if (select)
	    itemSelect = OnSelectedList(lw,items[i]);
	else 
	    itemSelect = FALSE;
#else /* OSF_v1_2_4 */
	itemSelect = select && OnSelectedList(lw,items[i]);

	/* CR 5833:  Enforce selection_policy. */
	if (itemSelect && 
	    ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
	     (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
	  {
	    select_pos = pos + i;
	    itemSelect = False;
	  }

#endif /* OSF_v1_2_4 */
        AddInternalElement(lw, items[i], pos + i, itemSelect, TRUE);
    }

    if ((intern_pos <= lw->list.CurrentKbdItem) &&
        (lw->list.itemCount > 1) &&
	!bot)
    {
        lw->list.CurrentKbdItem += item_count;
#ifndef OSF_v1_2_4
        if ((lw->list.AutoSelect) &&
            ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
#else /* OSF_v1_2_4 */
	/* CR 5804:  Don't check lw->list.AutoSelect here. */
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
	    (lw->list.SelectionPolicy == XmBROWSE_SELECT))
#endif /* OSF_v1_2_4 */
            lw->list.LastHLItem += item_count;
    }
#ifdef OSF_v1_2_4

    /* CR 5833: Enforce single/browse selection_policy. */
    if (select_pos)
      {
	/* Select the last of the matching new items. */
	lw->list.InternalList[select_pos - 1]->selected = TRUE;
	lw->list.InternalList[select_pos - 1]->last_selected = TRUE;
	lw->list.InternalList[select_pos - 1]->LastTimeDrawn = FALSE;

	/* Deselect the previous selected item. */
	if (lw->list.selectedItemCount > 0)
	  {
	    for (i = 0; 
		 i < MIN(lw->list.itemCount,
			 lw->list.top_position + lw->list.visibleItemCount);
		 i++)
	      if (lw->list.InternalList[i]->selected && (i+1 != select_pos))
		{
		  lw->list.InternalList[i]->selected = FALSE;
		  lw->list.InternalList[i]->last_selected = FALSE;
		  DrawItem((Widget) lw, i);
		  break;
		}

	    UpdateSelectedList(lw);
	  }
      }

#endif /* OSF_v1_2_4 */
    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
/*    SetTraversal(lw);*/
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListAddItems - add the items starting at the specified position.   *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItems( w, items, item_count, pos )
        Widget w ;
        XmString *items ;
        int item_count ;
        int pos ;
#else
XmListAddItems(
        Widget w,
        XmString *items,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    APIAddItems(lw, items, item_count, pos, TRUE);
}
/************************************************************************
 *									*
 * XmListAddItemsUnselected - add the items starting at the specified   *
 *   The selected List is not checked.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItemsUnselected( w, items, item_count, pos )
        Widget w ;
        XmString *items ;
        int item_count ;
        int pos ;
#else
XmListAddItemsUnselected(
        Widget w,
        XmString *items,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    APIAddItems(lw, items, item_count, pos, FALSE);
}

/************************************************************************
 *									*
 * XmListAddItemUnselected - add the item at the specified position.	*
 *     This does not check the selected list - the item is assumed to 	*
 *     be unselected.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItemUnselected( w, item, pos )
        Widget w ;
        XmString item ;
        int pos ;
#else
XmListAddItemUnselected(
        Widget w,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

	APIAddItems((XmListWidget)w, &item, 1, pos, False);

#else /* NOVELL */
    XmListWidget lw = (XmListWidget) w;
    int intern_pos   = pos-1;
    Boolean bot = FALSE;
    
    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
	bot = TRUE;
    }

    if (lw->list.Traversing)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    AddItem(lw, item, intern_pos);
    AddInternalElement(lw, item, pos, FALSE, TRUE);

    if ((intern_pos <= lw->list.CurrentKbdItem) && !bot &&
        (lw->list.itemCount > 1))
    {
        lw->list.CurrentKbdItem += 1;
#ifndef OSF_v1_2_4
        if ((lw->list.AutoSelect) &&
            ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
#else /* OSF_v1_2_4 */
	/* CR 5804:  Don't check lw->list.AutoSelect here. */
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
	    (lw->list.SelectionPolicy == XmBROWSE_SELECT))
#endif /* OSF_v1_2_4 */
            lw->list.LastHLItem ++;
    }

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
/*    SetTraversal(lw);*/
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListDeleteItem - delete the specified item from the list.		*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListDeleteItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	XmString	items[1];

	items[0] = item;
	XmListDeleteItems(w, items, 1);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;

    if (lw->list.itemCount < 1) 
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    item_pos = ItemNumber(lw, item);
    if (item_pos < 1 || item_pos > lw->list.itemCount)
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }
    XmListDeletePos( (Widget) lw, item_pos);
#endif /* NOVELL */
}

/************************************************************************
 *                                                                      *
 * CleanUpList - redraw the list if the items go to 0, and check for    *
 *   traversal locations.                                               *
 *   ** NOTE: CAN ONLY BE USED FROM API DELETE ROUTINES **              *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CleanUpList( lw )
        XmListWidget lw ;
#else
CleanUpList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{

   Dimension VertMargin, HorzMargin;

   if (!lw->list.itemCount)            /* Special case for deleting the last item */
   {
       HorzMargin = lw->list.margin_width +
	     lw->primitive.shadow_thickness;

       VertMargin = lw->list.margin_height +
	     lw->primitive.shadow_thickness;

       if (XtIsRealized(lw))
           XClearArea (XtDisplay (lw), XtWindow (lw),
                       HorzMargin,
                       VertMargin,
                       lw->core.width - (2 * HorzMargin),
                       lw->core.height - (2 *VertMargin),
                       False);
    }
    
}

/************************************************************************
 *									*
 * XmListDeleteItems - delete the specified items from the list.	*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItems( w, items, item_count )
        Widget w ;
        XmString *items ;
        int item_count ;
#else
XmListDeleteItems(
        Widget w,
        XmString *items,
        int item_count )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int *		pos_list;
	register int	i, j, item_pos;

	if (!items || item_count < 1 || !LPART(w).itemCount)
		return;

	pos_list = (int *)XtMalloc(sizeof(int) * item_count);

		/* `j' is the actual counter */
	for (i = 0, j = 0; i < item_count; i++)
	{
		if (item_pos = ItemNumber((XmListWidget)w, items[i]))
		{
			pos_list[j++] = item_pos;
		}
	}

	XmListDeletePositions(w, pos_list, j);

	XtFree((char *)pos_list);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    Boolean        redraw = FALSE;
    int		   item_pos;
    XmString	   *copy;
    
    register int    i;

    if ((items == NULL) || (item_count == 0)) return;

    if (lw->list.itemCount < 1) 
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    /* Make a copy of items in case of XmNitems from w */
    copy = (XmString *)ALLOCATE_LOCAL(item_count * sizeof(XmString));
    for (i = 0; i < item_count; i++)
      {
	copy[i] = XmStringCopy(items[i]);
      }
    
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        item_pos = ItemNumber(lw, copy[i]);
        if (item_pos < 1 || item_pos > lw->list.itemCount)
            _XmWarning(w, ListMessage8);
        else
        {
            if (lw->list.CurrentKbdItem >= (item_pos - 1))
            {
#ifdef NON_OSF_FIX
		if (lw->list.CurrentKbdItem > 0)
#endif /* NON_OSF_FIX */
                	lw->list.CurrentKbdItem--;
            }
	    /* Fix for 2798 - If the LastHLItem is the item that 
	       has been deleted, decrement the LastHLItem. */
	    if ((lw->list.LastHLItem > 0) &&
		(lw->list.LastHLItem == (item_pos - 1)))
	      lw->list.LastHLItem--;
	    /* End Fix 2798 */
            if (item_pos < (lw->list.top_position + lw->list.visibleItemCount))
                redraw = TRUE;
            DeleteItem(lw,item_pos - 1);
            DeleteInternalElement(lw,NULL,item_pos ,TRUE);
        }
    }
    UpdateSelectedList(lw);

    if (lw->list.itemCount)
    {
        if ((lw->list.itemCount - lw->list.top_position) < 
	    lw->list.visibleItemCount) 
        {
            lw->list.top_position = 
	      lw->list.itemCount - lw->list.visibleItemCount;
            if (lw->list.top_position < 0)
                lw->list.top_position = 0;
            redraw = TRUE;
        }
    }
    else lw->list.top_position = 0;
    
    if ((redraw) && (lw->list.itemCount))
        DrawList(lw, NULL, TRUE);

    CleanUpList(lw);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

    /* Free memory for copied list. */
    for (i = 0; i < item_count; i++)
      {
	XmStringFree(copy[i]);
      }
    DEALLOCATE_LOCAL((char *)copy);
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListDeletePositions - delete the specified positions from the list *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeletePositions( w, position_list, position_count )
        Widget  w ;
        int    *position_list ;
        int     position_count ;
#else
XmListDeletePositions(
        Widget    w,
        int      *position_list,
        int       position_count )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define LW	(XmListWidget)w

	register int	i;
	int		oldItemCount, item_pos;
	Boolean		re_draw = False;

#ifdef OSF_v1_2_4
	if (!position_list || position_count < 1 || !LPART(w).itemCount)
		return;
#else /* OSF_v1_2_4 */
	if (!position_list || position_count < 1)
		return;

	/* CR 5760:  Generate warnings for empty lists too. */
	if (LPART(w).itemCount < 1) 
	{
		_XmWarning(w, ListMessage8);
		return;
	}
#endif /* OSF_v1_2_4 */

		/* Note that, the if check below is in XmListDeletePos()
		 * and XmListDeleteItemsPos() */
	if (LPART(w).Traversing)
		DrawHighlight (LW, LPART(w).CurrentKbdItem, False);

		/* Save itemCount because DeleteItemPositions recomputes
		 * value. */
	oldItemCount = LPART(w).itemCount; 

	for (i = 0; i < position_count; i++)
	{
			/* Can't delete a title item */
		if ((item_pos = position_list[i]) < FIRST_ITEM(w) + 1 ||
		    item_pos > LPART(w).itemCount) 
		{
			_XmWarning(w, ListMessage8);
			position_list[i] = -1; /* mark position to be ignored */
		}
		else
		{
			/* Fix for 2798 - If the LastHLItem is the item that
			 * has been deleted, decrement the LastHLItem. */
			if (LPART(w).LastHLItem > 0 &&
			    LPART(w).LastHLItem == item_pos - 1)
				LPART(w).LastHLItem--;
			/* End Fix 2798 */

			if (!re_draw && item_pos - 1 < BOT_VIZ_ITEM(w) + 1)
				re_draw = True;
		}
	}

	DeleteItemPositions(LW, position_list, position_count);
	DeleteInternalElementPositions(
		LW, position_list, position_count, oldItemCount, True);

	UpdateSelectedList(LW);

	if (LPART(w).CurrentKbdItem >= LPART(w).itemCount)
	{
			/* the old code didn't check the boundary
			 * situation (i.e., the if check below),
			 * CDE only has NON_OSF_FIX #ifdef in
			 * XmListDeleteItems(), 5/2/94 */
		LPART(w).CurrentKbdItem = LPART(w).itemCount - 1;
		if (LPART(w).CurrentKbdItem < FIRST_ITEM(w))
			LPART(w).CurrentKbdItem = FIRST_ITEM(w);

			/* Note that, the LPART(w).AutoSelect check is
			 * commented out in XmListDeletePos() and
			 * XmListDeleteItemsPos() */
		if (LPART(w).SelectionPolicy == XmEXTENDED_SELECT ||
		    LPART(w).SelectionPolicy == XmBROWSE_SELECT)
			LPART(w).LastHLItem = LPART(w).CurrentKbdItem;
	}

	if (LPART(w).itemCount)
	{
		int	rows, ignore;

		CalcNumRows(w, &rows, &ignore);

		if (rows - LPART(w).top_position < LPART(w).visibleItemCount)
		{
			LPART(w).top_position = rows -
						LPART(w).visibleItemCount;
			if (LPART(w).top_position < 0)
				LPART(w).top_position = 0;
			re_draw = True;
		}
	}
	/*** fix for mr#ul94-24455 ****/
	/** if itemCount == 0, top should be 0 **/
	if( LPART(w).itemCount == 0 )
	{
	   LPART(w).top_position = 0;
	}
	
	/**** end of fix ***/

	CleanUpList(LW);

	CalcNewSize(LW, re_draw);

#undef LW
#else /* NOVELL */
    XmListWidget   lw = (XmListWidget) w;
    Boolean        redraw = FALSE;
    Boolean        UpdateLastHL;
    int		   item_pos;
    int            oldItemCount;
   
    register int    i;

    if ((position_list == NULL) || (position_count == 0)) return;

#ifndef OSF_v1_2_4
    if (lw->list.itemCount < 1) return;
#else /* OSF_v1_2_4 */
    /* CR 5760:  Generate warnings for empty lists too. */
    if (lw->list.itemCount < 1) 
      {
	_XmWarning(w, ListMessage8);
	return;
      }
#endif /* OSF_v1_2_4 */

#ifndef OSF_v1_2_4
    UpdateLastHL =  (( lw->list.AutoSelect) &&
                     ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                      (lw->list.SelectionPolicy == XmBROWSE_SELECT)));
#else /* OSF_v1_2_4 */
    /* CR 5804:  Don't check lw->list.AutoSelect here. */
    UpdateLastHL = ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
		    (lw->list.SelectionPolicy == XmBROWSE_SELECT));
#endif /* OSF_v1_2_4 */

    DrawHighlight (lw, lw->list.CurrentKbdItem, FALSE);

/*  Save itemCount because DeleteItemPositions recomputes value.
*/
    oldItemCount = lw->list.itemCount; 

    for (i = 0; i < position_count; i++)
    {
        item_pos = position_list[i];
        if (item_pos < 1 || item_pos > lw->list.itemCount) 
        {
            _XmWarning( (Widget) lw, ListMessage8);
            position_list[i] = -1;   /* mark position to be ignored */
        }
        else
            if (item_pos < (lw->list.top_position + lw->list.visibleItemCount))
                redraw = TRUE;
    }

    DeleteItemPositions (lw, position_list, position_count);
    DeleteInternalElementPositions 
                        (lw, position_list, position_count, oldItemCount, TRUE);

    if (lw->list.CurrentKbdItem >= lw->list.LastItem)
    {
        lw->list.CurrentKbdItem = lw->list.LastItem;
        if (lw->list.CurrentKbdItem < 0)
            lw->list.CurrentKbdItem = 0;
        if (UpdateLastHL) 
            lw->list.LastHLItem = lw->list.CurrentKbdItem;
    }
    UpdateSelectedList (lw);

    if (lw->list.itemCount)
    {
        if ((lw->list.itemCount - lw->list.top_position) < lw->list.visibleItemCount) 
        {
            lw->list.top_position = lw->list.itemCount - lw->list.visibleItemCount;
            if (lw->list.top_position < 0)
                lw->list.top_position = 0;
            redraw = TRUE;
        }
    }

	/*** fix for mr#ul94-24455****/
    if( lw->list.itemCount == 0 )
    {
	lw->list.top_position = 0;
    }
	
	/**** end of fix ****/

    if ((redraw) && (lw->list.itemCount))
        DrawList(lw, NULL, TRUE);

    CleanUpList(lw);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmDeletePos - delete the item at the specified position from the	*
 *list.									*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeletePos( w, pos )
        Widget w ;
        int pos ;
#else
XmListDeletePos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	pos_list[1];

	pos_list[0] = pos;
	XmListDeletePositions(w, pos_list, 1);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int item_pos, last, new_top, old_kbd;

#ifndef OSF_v1_2_4
    if (lw->list.itemCount < 1) return;
#else /* OSF_v1_2_4 */
    /* CR 5760:  Generate warnings for empty lists too. */
    if (lw->list.itemCount < 1) 
      {
    	_XmWarning(w, ListMessage8);
	return;
      }

#endif /* OSF_v1_2_4 */
    item_pos  = pos - 1;
    if (item_pos < 0)
    {
        item_pos = lw->list.itemCount - 1;
	pos = lw->list.itemCount;
    }

    if (item_pos >= lw->list.itemCount)
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }
    if ((lw->list.Traversing) && (item_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    old_kbd = lw->list.CurrentKbdItem;

    DeleteItem            (lw,item_pos);
    DeleteInternalElement (lw,NULL,pos,TRUE);
    UpdateSelectedList    (lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= 1;
            if (lw->list.CurrentKbdItem < 0)
                lw->list.CurrentKbdItem = 0;
/*            if ((lw->list.AutoSelect) &&*/
              if((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                 (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                lw->list.LastHLItem = lw->list.CurrentKbdItem;            
        }

/****************
 *
 * Delete policy: if the item is not visible, adjust top_position
 * to preserve list display. If there are too few items in
 * the list following top_position, adjust top_position to
 * keep the list display full.
 *
 ****************/
    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
            new_top--;
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
                    new_top--;                    
            }
        if (lw->list.top_position != new_top)
        {
            DrawHighlight(lw, old_kbd, FALSE);
#ifndef OSF_v1_2_4
            lw->list.top_position = (new_top > 0) ? new_top : 0;
#else /* OSF_v1_2_4 */
            lw->list.top_position = MAX(new_top, 0);
#endif /* OSF_v1_2_4 */
            DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }
    else lw->list.top_position = 0;

    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmDeleteItemsPos - delete the items at the specified position        *
 * from the list.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItemsPos( w, item_count, pos )
        Widget w ;
        int item_count ;
        int pos ;
#else
XmListDeleteItemsPos(
        Widget w,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int *		pos_list;
	int		item_pos;
	register int	i;

#ifndef OSF_v1_2_4

	if (!LPART(w).itemCount || item_count < 1)
		return;

	item_pos  = pos - 1;

		/* Can't delete a title item! */
	if (item_pos < FIRST_ITEM(w)  ||
	    item_pos >= LPART(w).itemCount)
	{
		_XmWarning(w, ListMessage8);
		return;
	}

#else /* OSF_v1_2_4 */

	/* CR 7270:  Deleting zero items is not an error. */
	if (item_count == 0)
		return;

	item_pos  = pos - 1;

	/* CR 5760:  Generate warnings for empty lists too. */
	if (!LPART(w).itemCount ||
	    item_count < 0 ||
	    item_pos < FIRST_ITEM(w)  ||	/* Can't delete a title item! */
	    item_pos >= LPART(w).itemCount)
	{
		_XmWarning(w, ListMessage8);
		return;
	}

#endif /* OSF_v1_2_4 */

	if (item_pos + item_count >= LPART(w).itemCount)
		item_count = LPART(w).itemCount - item_pos;

	pos_list = (int *)XtMalloc(sizeof(int) * item_count);
	for (i = 0; i < item_count; i++)
	{
		pos_list[i] = pos + i;
	}
	XmListDeletePositions(w, pos_list, item_count);

	XtFree((char *)pos_list);

#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int item_pos, last, new_top, old_kbd;
    register int    i;

#ifndef OSF_v1_2_4
    if ((lw->list.itemCount < 1) || (item_count < 1)) return;
#else /* OSF_v1_2_4 */
    /* CR 7270:  Deleting zero items is not an error. */
    if (item_count == 0)
      return;
#endif /* OSF_v1_2_4 */

#ifdef OSF_v1_2_4
    /* CR 5760:  Generate warnings for empty lists too. */
    if ((lw->list.itemCount < 1) || (item_count < 0)) 
      {
	_XmWarning(w, ListMessage8);
	return;
      }

#endif /* OSF_v1_2_4 */
    item_pos  = pos - 1;

    if ((item_pos < 0)  ||
       (item_pos >= lw->list.itemCount))
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    if ((item_pos + item_count) >= lw->list.itemCount)
        item_count = lw->list.itemCount - item_pos;

    if (lw->list.Traversing)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    old_kbd = lw->list.CurrentKbdItem;

/****************
 *
 * Delete the elements. Because of the way the internal routines
 * work (they ripple up the list items after each call), we keep
 * deleting the "same" element for item_count times.
 *
 ****************/
    for (i = 0; i < item_count; i++)
    {
        DeleteItem(lw,item_pos);
        DeleteInternalElement(lw,NULL,pos,TRUE);    /* Save the allocs? */
    }

    UpdateSelectedList(lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= item_count;
            if (lw->list.CurrentKbdItem < 0)
                lw->list.CurrentKbdItem = 0;
/*            if ((lw->list.AutoSelect) &&*/
              if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                 (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                lw->list.LastHLItem = lw->list.CurrentKbdItem;            

        }

    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
	{
            new_top-= item_count;
	    if (new_top < 0) new_top = 0;
	}
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
 /*
  * Fix for 5080 - Do not let new_top go negative.  Very, very bad
  *                 things happen when it does.
  */
                {
                    new_top -= item_count;                    
                    if (new_top < 0) new_top = 0;
                }
/*
 * End 5080 Fix
 */
            }
        if (lw->list.top_position != new_top)
        {
            DrawHighlight(lw, old_kbd, FALSE);
            lw->list.top_position = new_top;
            DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }
    else lw->list.top_position = 0;
    
    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}

/************************************************************************
 *                                                                      *
 * XmListDeleteAllItems - clear the list.                               *
 *                                                                      *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteAllItems( w )
        Widget w ;
#else
XmListDeleteAllItems(
        Widget w )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#define LW		(XmListWidget)w

	if (!LPART(w).itemCount)
		return;

	DrawHighlight(LW, LPART(w).CurrentKbdItem, False);

	DestroyInternalList(w);

		/* no need to call BuildSelectedList() because all items
		 * are deleted */
	ClearSelectedList(LW);
        CleanUpList(LW);
        (void)SetNewSize(LW);
        if (LPART(w).SizePolicy != XmVARIABLE || LPART(w).cols != 1)
		SetHorizontalScrollbar(LW);
	SetVerticalScrollbar(LW);

#undef LW

#else /* NOVELL */
    XmListWidget lw = (XmListWidget) w;
    register int i;
    int          j;
    if (lw->list.items && (lw->list.itemCount > 0))
    {
       DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
       j = lw->list.itemCount;
       for (i = lw->list.itemCount - 1; i >= 0; i--)
        {
	    lw->list.itemCount--;
    	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
        }
        if (lw->list.InternalList) XtFree((char *) lw->list.InternalList);
        lw->list.InternalList = NULL;
        lw->list.itemCount = j;
        ClearItemList(lw);
	UpdateSelectedList(lw);
        CleanUpList(lw);
        SetNewSize(lw);
        if (lw->list.SizePolicy != XmVARIABLE)
            SetHorizontalScrollbar(lw);
        SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}
/************************************************************************
 *									*
 * APIReplaceItems - replace the given items with new ones.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APIReplaceItems( w, old_items, item_count, new_items, select )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
	Boolean  select ;
#else
APIReplaceItems(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items,
#if NeedWidePrototypes
	int select)
#else
        Boolean select)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	i, j, k;
	int *		pos_list;
	_XmString	_item;

	if (!old_items || !new_items || item_count < 1 || !LPART(w).itemCount)
		return;

	pos_list = (int *)XtMalloc(sizeof(int) * item_count);

		/* `j' is the actual counter */
	for (i = 0, j = 0; i < item_count; i++)
	{
/* BEGIN OSF Fix CR 5763 */
			/* OSF fix uses XmStringCompare(), but
			 * InternalList[] already have _XmString info,
			 * we should just use them...
			 *
			 * XmStringCompare() is expensive because
			 * each call takes 2 mallocs and 2 frees. */
		_item = _XmStringCreate(old_items[i]);
		for (k = 1; k <= LPART(w).itemCount; k++)
		{
			if (_XmStringByteCompare(
				LPART(w).InternalList[k-1]->name, _item))
			{
				pos_list[j++] = k;
			}
		}
		_XmStringFree(_item);
/* END OSF Fix CR 5763 */
	}

	ReplacePositions(w, pos_list, new_items, j, (Boolean)select);

	XtFree((char *)pos_list);

#else /* NOVELL */
/* BEGIN OSF Fix CR 5763 */
    register int i, j;
/* END OSF Fix CR 5763 */
    XmListWidget lw = (XmListWidget) w;
    Boolean      ReDraw = FALSE;

    if ((old_items == NULL)     ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    for (i = 0; i < item_count; i++)
    {
/* BEGIN OSF Fix CR 5763 */
      for (j = 1; j <= lw->list.itemCount; j++)
	{
	  if (XmStringCompare(lw->list.items[j-1], old_items[i]))
	    {
	      if (j <= (lw->list.top_position + lw->list.visibleItemCount))
                ReDraw = TRUE;
	      ReplaceItem(lw, new_items[i], j);
	      ReplaceInternalElement(lw, j, select);
	    }
	}
/* END OSF Fix CR 5763 */
    }
    ResetHeight(lw);

    if (ReDraw)
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}
/************************************************************************
 *									*
 * XmListReplaceItems - replace the given items with new ones.          *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItems( w, old_items, item_count, new_items )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
#else
XmListReplaceItems(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items )
#endif /* _NO_PROTO */
{
    APIReplaceItems( w, old_items, item_count, new_items, TRUE );
}
/************************************************************************
 *									*
 * XmListReplaceItemsUnselected - replace the given items with new ones.*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsUnselected( w, old_items, item_count, new_items )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
#else
XmListReplaceItemsUnselected(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items )
#endif /* _NO_PROTO */
{
    APIReplaceItems( w, old_items, item_count, new_items, FALSE );
}
/************************************************************************
 *									*
 * APIReplaceItemsPos- replace the given items with new ones.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APIReplaceItemsPos(  w, new_items, item_count, position, select )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
	Boolean  select ;
#else
APIReplaceItemsPos(
        Widget w,
        XmString *new_items,
        int item_count,
 	int position,
#if NeedWidePrototypes
	int select)
#else
        Boolean select)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	i;
	int *		pos_list;

		/* defer item_count checking */
	if (!LPART(w).itemCount || !new_items ||
	    position < 1 || position > LPART(w).itemCount)
		return;

#define INTERN_POS	(position - 1)

	if (INTERN_POS + item_count > LPART(w).itemCount)
	{
		item_count = LPART(w).itemCount - INTERN_POS;
	}

#undef INTERN_POS

	if (item_count < 1)
		return;

	pos_list = (int *)XtMalloc(sizeof(int) * item_count);

	for (i = 0; i < item_count; i++, position++)
		pos_list[i] = position;

	ReplacePositions(w, pos_list, new_items, item_count, (Boolean)select);

	XtFree((char *)pos_list);
#else /* NOVELL */
    XmListWidget lw = (XmListWidget) w;
    int          intern_pos;
    register int i;

    if ((position < 1)          ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    intern_pos = position - 1;

    if ((intern_pos + item_count) > lw->list.itemCount)
        item_count = lw->list.itemCount - intern_pos;

    for (i = 0; i < item_count; i++, position++)
    {
        ReplaceItem(lw, new_items[i], position);
        ReplaceInternalElement(lw, position, select);
    }

    ResetHeight(lw);

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListReplaceItemsPos - replace the given items at the specified     *
 *      position with new ones.                                         *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsPos( w, new_items, item_count, position )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
#else
XmListReplaceItemsPos(
        Widget w,
        XmString *new_items,
        int item_count,
        int position )
#endif /* _NO_PROTO */
{
APIReplaceItemsPos(  w, new_items, item_count, position, TRUE );
}
/************************************************************************
 *									*
 * XmListReplaceItemsPosUnselected - replace the given items at the     *
 *    specified position with new ones.                                 *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsPosUnselected( w, new_items, item_count, position )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
#else
XmListReplaceItemsPosUnselected(
        Widget w,
        XmString *new_items,
        int item_count,
        int position )
#endif /* _NO_PROTO */
{
APIReplaceItemsPos(  w, new_items, item_count, position, FALSE );
}

/************************************************************************
 *									*
 * XmListReplacePositions - Replace a set of items based on a list of   *
 *			    positions.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplacePositions( w, position_list, item_list, item_count )
        Widget    w;
        int      *position_list;
        XmString *item_list;
        int       item_count; 
#else
XmListReplacePositions(
        Widget    w,
        int      *position_list,
        XmString *item_list,
        int       item_count )
#endif /* _NO_PROTO */
{
#ifdef NOVELL

#ifdef OSF_v1_2_4
	/* CR 5760:  Generate warnings for empty lists too. */
	if (LPART(w).itemCount < 1 &&
	    (position_list || item_list || item_count))
	{
		if (position_list || item_count)
			_XmWarning(w, ListMessage8);
		return;
	}
#endif /* OSF_v1_2_4 */

	if (!position_list || !item_list || item_count < 1 ||
	    !LPART(w).itemCount)
		return;

	ReplacePositions(w, position_list, item_list, item_count, True);

#else /* NOVELL */
    int item_pos;
    register int i;
    XmListWidget lw = (XmListWidget) w;
    Boolean      ReDraw = FALSE;

#ifdef OSF_v1_2_4
    /* CR 5760:  Generate warnings for empty lists too. */
    if ((lw->list.itemCount < 1) &&
	(position_list || item_list || item_count))
      {
	if (position_list || item_count)
	  _XmWarning(w, ListMessage8);
        return;
      }

#endif /* OSF_v1_2_4 */
    if ((position_list  == NULL)  ||
        (item_list      == NULL)  ||
        (lw->list.items == NULL)  ||
        (item_count     == 0))
        return;

    for (i = 0; i < item_count; i++)
    {
        item_pos = position_list[i];

        if (item_pos < 1 || item_pos > lw->list.itemCount)
            _XmWarning( (Widget) lw, ListMessage8);
        else
        {
            if (item_pos <= (lw->list.top_position + lw->list.visibleItemCount))
                ReDraw = TRUE;
            ReplaceItem            (lw, item_list[i], item_pos);
            ReplaceInternalElement (lw, item_pos, TRUE);
        }
    }
    ResetHeight(lw);

    if (ReDraw)
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * APISelect - do the necessary selection work for the API select	*
 * routines								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APISelect( lw, item_pos, notify )
        XmListWidget lw ;
        int item_pos ;
        Boolean notify ;
#else
APISelect(
        XmListWidget lw,
        int item_pos,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int	i;

    	item_pos--;
/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
	if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
	     (lw->list.SelectionPolicy == XmEXTENDED_SELECT)))
	{
	    for (i = 0; i < lw->list.itemCount; i++)
	        if(lw->list.InternalList[i]->selected)
	        {
		    lw->list.InternalList[i]->selected = FALSE;
		    lw->list.InternalList[i]->last_selected = FALSE;
		    DrawItem((Widget) lw, i);
	        }

	}
	if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
            lw->list.SelectionType = XmINITIAL;

	if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
            (lw->list.InternalList[item_pos]->selected))
        {
            lw->list.InternalList[item_pos]->selected = FALSE;
            lw->list.InternalList[item_pos]->last_selected = FALSE;
        }
        else
        {
    	    lw->list.InternalList[item_pos]->selected = TRUE;
    	    lw->list.InternalList[item_pos]->last_selected = TRUE;
        }

	DrawItem((Widget) lw, item_pos);
	lw->list.LastHLItem = item_pos;

    	if (notify)
    	    ClickElement(lw, NULL,FALSE);
	else
	    UpdateSelectedList(lw);
}

/************************************************************************
 *                                                                      *
 * SetSelectionParams - update the selection parameters so that an API  *
 * selection looks the same as a user selection.                        *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetSelectionParams( lw )
        XmListWidget lw ;
#else
SetSelectionParams(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int	start, end, i;

	if (LPART(lw).itemCount)
	{
		for (i = LPART(lw).itemCount - 1; i >= FIRST_ITEM(lw); i--)
		    if (LPART(lw).InternalList[i]->selected)
		    {
			end = i;
			while (i >= FIRST_ITEM(lw) &&
			       LPART(lw).InternalList[i]->selected)
					i--;

			start = i + 1;
			LPART(lw).OldEndItem	= LPART(lw).EndItem;
			LPART(lw).EndItem	= end;
			LPART(lw).OldStartItem	= LPART(lw).StartItem;
			LPART(lw).StartItem	= start;
			LPART(lw).LastHLItem	= end;

			if (LPART(lw).Traversing)
			       DrawHighlight(
					lw, LPART(lw).CurrentKbdItem, False);
			LPART(lw).CurrentKbdItem = end;
			if (LPART(lw).Traversing)
			       DrawHighlight(lw, end, True);
			return;
		    }

/****************
 *
 * When we get here, there are no selected items in the list.
 *
 ****************/
		LPART(lw).OldEndItem	= LPART(lw).EndItem;
		LPART(lw).OldStartItem	= LPART(lw).StartItem;
		LPART(lw).EndItem	=
		LPART(lw).StartItem	=
		LPART(lw).LastHLItem	= 0;
	}
#else /* NOVELL */
    register int start, end, i;

    if (lw->list.items && lw->list.itemCount)
    {
        for (i = lw->list.itemCount - 1; i >= 0; i--)
            if (lw->list.InternalList[i]->selected)
            {
                end = i;
                while (i && (lw->list.InternalList[i]->selected)) i--;
		if ((i ==0) && (lw->list.InternalList[i]->selected))
		    start = i;
		else
                    start = i + 1;
                lw->list.OldEndItem = lw->list.EndItem;
                lw->list.EndItem = end;
                lw->list.OldStartItem = lw->list.StartItem;
                lw->list.StartItem = start;
                lw->list.LastHLItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
                lw->list.CurrentKbdItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
                return;
            }
/****************
 *
 * When we get here, there are no selected items in the list.
 *
 ****************/
        lw->list.OldEndItem = lw->list.EndItem;
        lw->list.EndItem = 0;
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.StartItem = 0;
        lw->list.LastHLItem = 0;
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSelectItem - select the given item and issue a callback if so	*
 * requested.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSelectItem( w, item, notify )
        Widget w ;
        XmString item ;
        Boolean notify ;
#else
XmListSelectItem(
        Widget w,
        XmString item,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	item_pos;

	if (LPART(w).itemCount &&
	    (item_pos = ItemNumber((XmListWidget)w, item)))
	{
		XmListSelectPos(w, item_pos, notify);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;
    if (lw->list.itemCount < 1) return;

    if ((item_pos = ItemNumber(lw,item)))
    {
        APISelect(lw, item_pos, notify);
        SetSelectionParams(lw);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSelectPos - select the item at the given position and issue a  *
 * callback if so requested.						*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSelectPos( w, pos, notify )
        Widget w ;
        int pos ;
        Boolean notify ;
#else
XmListSelectPos(
        Widget w,
        int pos,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	if (!LPART(w).itemCount)
		return;

	if (!pos)
		pos = LPART(w).itemCount;

	if (pos > FIRST_ITEM(w) && pos <= LPART(w).itemCount)
	{
		APISelect((XmListWidget)w, pos, notify);
		SetSelectionParams((XmListWidget)w);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return;

    if (pos >= 0 && pos <= lw->list.itemCount)
    {
	if (pos == 0) pos = lw->list.itemCount;
        APISelect(lw, pos, notify);
        SetSelectionParams(lw);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListDeselectItem - deselect the given item and issue a callback if *
 * so requested.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListDeselectItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	item_pos;

	if (LPART(w).itemCount &&
	    (item_pos = ItemNumber((XmListWidget)w, item)))
	{
		XmListDeselectPos(w, item_pos);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;

    if ((i =  ItemNumber(lw,item)))
    {
        i--;
	lw->list.InternalList[i]->selected = FALSE;
	lw->list.InternalList[i]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem((Widget) lw, i);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListDeselectPos - deselect the item at the given position and issue*
 * a callback if so requested.						*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListDeselectPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	if (!LPART(w).itemCount)
		return;

	if (!pos)
		pos = LPART(w).itemCount;

	if (pos > FIRST_ITEM(w) && pos <= LPART(w).itemCount)
	{
		pos--;
		LPART(w).InternalList[pos]->selected = FALSE;
		LPART(w).InternalList[pos]->last_selected = FALSE;
		UpdateSelectedList((XmListWidget)w);
		DrawItem(w, pos);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos >= 0 && pos <= lw->list.itemCount)
    {
        pos--;
	if (pos < 0) pos = lw->list.itemCount - 1;
	lw->list.InternalList[pos]->selected = FALSE;
	lw->list.InternalList[pos]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem((Widget) lw, pos);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmDeselectAllItems - hose the entire selected list			*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectAllItems( w )
        Widget w ;
#else
XmListDeselectAllItems(
        Widget w )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	register int  i;

	if (!LPART(w).itemCount || !LPART(w).selectedItemCount)
		return;

	for (i = 0; i < LPART(w).itemCount; i++)
		if (LPART(w).InternalList[i]->selected)
		{
			LPART(w).InternalList[i]->selected	=
			LPART(w).InternalList[i]->last_selected = False;
			DrawItem(w, i);
		}

	ClearSelectedList((XmListWidget)w);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int  i;

    if (lw->list.itemCount < 1) return;
    if (lw->list.selectedItemCount > 0)
    {
	for (i = 0; i < lw->list.itemCount; i++)
	    if(lw->list.InternalList[i]->selected)
	    {
	        lw->list.InternalList[i]->selected = FALSE;
	        lw->list.InternalList[i]->last_selected = FALSE;
	        DrawItem((Widget) lw, i);
	    }
	ClearSelectedList(lw);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSetPos - Make the specified position the top visible position	*
 * in the list.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListSetPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	(void)SetVizPos(w, pos, ITEM_ON_TOP);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        pos--;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = pos;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSetBottomPos - Make the specified position the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetBottomPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListSetBottomPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	(void)SetVizPos(w, pos, ITEM_ON_BELOW);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int top;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        top = pos - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSetItem - Make the specified item the top visible item 	*
 * in the list.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListSetItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	item_pos;

	if (LPART(w).itemCount &&
	    (item_pos = ItemNumber((XmListWidget)w, item)))
	{
		(void)SetVizPos(w, item_pos, ITEM_ON_TOP);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;
    if ((i =  ItemNumber(lw,item)))
    {
        i--;
	if (i == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = i;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListSetBottomItem - Make the specified item the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetBottomItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListSetBottomItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	item_pos;

	if (LPART(w).itemCount &&
	    (item_pos = ItemNumber((XmListWidget)w, item)))
	{
		(void)SetVizPos(w, item_pos, ITEM_ON_BELOW);
	}
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    int i, top;

    if (lw->list.itemCount < 1) return;
    if ((i = ItemNumber(lw,item)))
    {
        top = i - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListSetAddMode - Programatically set add mode.                     *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetAddMode( w, add_mode )
        Widget w ;
        Boolean add_mode ;
#else
XmListSetAddMode(
        Widget w,
#if NeedWidePrototypes
        int add_mode )
#else
        Boolean add_mode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

#ifdef NOVELL
	/* Should we return if (LPART(w).AddMode == add_mode) */
#endif /* NOVELL */

    if ((!add_mode) &&
        ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
	 (lw->list.SelectionPolicy == XmMULTIPLE_SELECT)))
	return;			/*  Can't be false for single or multiple */

    if ((add_mode) && (lw->list.SelectionPolicy == XmBROWSE_SELECT))
	return;			/*  Can't be true for browse */

    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.AddMode = add_mode;
    ChangeHighlightGC(lw, lw->list.AddMode);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
/****************
 *
 * Funky hacks for Ellis: If we enter add mode with one item selected,
 * deselect the current one. If we leave add mode with no items selected,
 * select the current one.
 *
 * rgcote 8/23/93: Here's a little more background for the "Funky hacks".
 * In normal mode, one item must be selected at all times.  In add mode,
 * this is not the case. In add mode, a user wants to specify a set of,
 * possibly discontiguous, items to be the selection. If, when going from
 * normal mode to add mode, more than one item is already selected than
 * we assume that the user started selecting the items and decided that
 * he/she needed to now select more items that are discontiguous from the
 * currently selected items. On the other hand, if only one item was
 * selected, we assume that the single selected item is just the result
 * of normal mode forcing one item to be selected and we unselect that
 * single item.
 *
 ****************/
    if ((add_mode) &&
        (lw->list.itemCount != 0) &&
        (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
        (lw->list.selectedItemCount == 1) &&
        (lw->list.InternalList[lw->list.CurrentKbdItem]->selected))
    {
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = FALSE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = FALSE;
	DrawList(lw, NULL, TRUE);
#ifndef OSF_v1_2_4
        ClickElement(lw, NULL,FALSE);
#else /* OSF_v1_2_4 */
	UpdateSelectedList(lw);
#endif /* OSF_v1_2_4 */
    }
#ifndef OSF_v1_2_4
    else
        if ((!add_mode) &&
#else /* OSF_v1_2_4 */
    else if ((!add_mode) &&
#endif /* OSF_v1_2_4 */
            (lw->list.itemCount != 0) &&
            (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
            (lw->list.selectedItemCount == 0))
#ifndef OSF_v1_2_4
        {
            lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
            lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
            DrawList(lw, NULL, TRUE);
            ClickElement(lw, NULL,FALSE);
        }

#else /* OSF_v1_2_4 */
    {
	lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
	lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
	DrawList(lw, NULL, TRUE);
	UpdateSelectedList(lw);
    }
#endif /* OSF_v1_2_4 */
}

/************************************************************************
 *									*
 * XmListItemExists - returns TRUE if the given item exists in the	*
 * list.								*
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListItemExists( w, item )
        Widget w ;
        XmString item ;
#else
XmListItemExists(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
		/* `item' can be NULL! */
	if (!LPART(w).itemCount)
		return(False);
	else
		return(ItemNumber((XmListWidget)w, item) ? True : False);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return(FALSE);
    return (ItemExists(lw, item));
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListItemPosition - returns the index (1-based) of the given item.  *
 * Returns 0 if not found.                                              *
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListItemPos( w, item )
        Widget w ;
        XmString item ;
#else
XmListItemPos(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
		/* `item' can be NULL, but the old code doesn't allow it!
		 * what should we do? */
	if (!item || !LPART(w).itemCount)
		return(0);
	else
		return(ItemNumber((XmListWidget)w, item));
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    if (item == NULL) return(0);
    return (ItemNumber(lw, item));
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListGetKbdItemPos - returns the index (1-based) of the current     *
 *                       keyboard item.                                 *
 * Returns 0 if not found.                                              *
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListGetKbdItemPos( w )
        Widget w ;
#else
XmListGetKbdItemPos(
        Widget w )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	if (!LPART(w).itemCount || TITLE_ROW_ITEM(w, LPART(w).CurrentKbdItem))
		return(0);
	else
		return(LPART(w).CurrentKbdItem + 1);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.items == NULL) return ( 0 );
    return ( lw->list.CurrentKbdItem + 1 );
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListSetKbdItemPos - allows user to set the current keyboard item   *
 * Returns True  if successful.                                         *
 *         False if not                                                 *
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListSetKbdItemPos( w, pos )
        Widget w ;
        int    pos ;
#else
XmListSetKbdItemPos(
        Widget w,
        int    pos )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	XmListWidget	lw = (XmListWidget)w;
	int		old_item;

	if (!LPART(w).itemCount || pos < 0 || pos > LPART(w).itemCount)
		return (False);

	if (!pos)
		pos = LPART(w).itemCount;

	if (--pos < FIRST_ITEM(w))
		return(False);

	old_item = LPART(w).CurrentKbdItem;
	LPART(w).CurrentKbdItem = pos;

	if (!MakeItemVisible(lw, LPART(w).CurrentKbdItem, 0))
	{ /* List not drawn, so take care the focus item visual */
		DrawHighlight(lw, old_item, False);
		DrawHighlight(lw, pos, True);
	}
	return(True);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;

    if ((lw->list.items == NULL)  ||
        (pos < 0)		  ||
	( pos > lw->list.itemCount) ) 
        return ( False );
    if (pos == 0) pos = lw->list.itemCount;

    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = pos - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE );

    MakeItemVisible(lw,lw->list.CurrentKbdItem); /* do we need to do this? */
    return(TRUE);
#endif /* NOVELL */
}


/************************************************************************
 *									*
 * XmListGetMatchPos - returns the positions that an item appears at in *
 * the list. CALLER MUST FREE SPACE!                                    *
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListGetMatchPos( w, item, pos_list, pos_count )
        Widget w ;
        XmString item ;
        int **pos_list ;
        int *pos_count ;
#else
XmListGetMatchPos(
        Widget w,
        XmString item,
        int **pos_list,
        int *pos_count )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	/* InternalList already have _XmString information, so use it!
	 * XmStringCompare() is expensive because each call takes 2 mallocs
	 * and 2 fress. */

	register int	i, *pos;
	int		j;
	_XmString	_item;

	if (!LPART(w).itemCount)
		return(False);

	*pos_list = NULL;
	*pos_count = 0;

	_item = _XmStringCreate(item);
	pos = (int *)XtMalloc((sizeof(int) * LPART(w).itemCount));
	j = 0;

	for (i = 0; i < LPART(w).itemCount; i++)
		if (_XmStringByteCompare(LPART(w).InternalList[i]->name, _item))
			pos[j++] = i + 1;

	_XmStringFree(_item);

	if (j == 0)
	{
		XtFree((char *)pos);
		return(False);
	}

	if (j != LPART(w).itemCount)
		pos = (int *)XtRealloc((char *)pos, (sizeof(int) * j));

	*pos_list = pos;
	*pos_count = j;
	return(True);
#else /* NOVELL */
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;

#ifdef OSF_v1_2_4
    /* CR 7648: Be friendly and initialize out parameters. */
    *pos_list = NULL;
    *pos_count = 0;

#endif /* OSF_v1_2_4 */
    if ((lw->list.items == NULL) ||
        (lw->list.itemCount <= 0))
#ifndef OSF_v1_2_4
        return(FALSE);
#else /* OSF_v1_2_4 */
      return(FALSE);
#endif /* OSF_v1_2_4 */

    pos = (int *)XtMalloc((sizeof(int) * lw->list.itemCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
        if ((XmStringCompare(lw->list.items[i], item)))
            pos[j++] = (i+1);

    if (j == 0)
    {
        XtFree((char *)pos);
        return (FALSE);
    }
    pos = (int *)XtRealloc((char *) pos, (sizeof(int) * j));

    *pos_list = pos; *pos_count = j;
    return(TRUE);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmListGetSelectedPos - returns the positions of the selected items   *
 * in the list. CALLER MUST FREE SPACE!                                 *
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListGetSelectedPos( w, pos_list, pos_count )
        Widget w ;
        int **pos_list ;
        int *pos_count ;
#else
XmListGetSelectedPos(
        Widget w,
        int **pos_list,
        int *pos_count )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;
    register int  selectedCount=0;

#ifdef OSF_v1_2_4
    /* CR 7648: Be friendly and initialize out parameters. */
    *pos_list = NULL;
    *pos_count = 0;

#endif /* OSF_v1_2_4 */
    if ((lw->list.items == NULL)        ||
        (lw->list.itemCount <= 0)       ||
        (lw->list.selectedItemCount <= 0))
      {
#ifndef OSF_v1_2_4
	*pos_count = 0;
#endif /* OSF_v1_2_4 */
        return(FALSE);
      }

    /* 
    ** lw->list.selectedItemCount may not give the real number of selected
    ** items; run through list and precalculate number of selected items.
    */
    for (i = 0; i < lw->list.itemCount; i++)
        if (lw->list.InternalList[i]->selected)
		selectedCount++;
#ifndef OSF_v1_2_4
    if (0 == selectedCount)
#else /* OSF_v1_2_4 */
    if (selectedCount == 0)
#endif /* OSF_v1_2_4 */
      { 
#ifndef OSF_v1_2_4
	*pos_count = 0;
#endif /* OSF_v1_2_4 */
	return (FALSE);
      }

    pos = (int *)XtMalloc((sizeof(int) * selectedCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
      if (lw->list.InternalList[i]->selected)
	{
	  pos[j] = (i+1);
	  j++;
	}

    *pos_list = pos; *pos_count = j;
    return(TRUE);
}


/************************************************************************
 *									*
 * XmListSetHorizPos - move the hsb.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetHorizPos( w, position )
        Widget w ;
        int position ;
#else
XmListSetHorizPos(
        Widget w,
        int position )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;

    if (!lw->list.hScrollBar)
/*	_XmWarning( (Widget) lw, ListMessage9)*/;
    else
    {
        if (position < lw->list.hmin) position = lw->list.hmin;
        if ((lw->list.hExtent + position) > lw->list.hmax)
	    position = lw->list.hmax - lw->list.hExtent;
/*	    lw->list.hExtent = lw->list.hmax - position;*/

/*      if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);*/
        if (position == lw->list.hOrigin) return;
	lw->list.hOrigin = position;
	lw->list.XOrigin= position;
        SetHorizontalScrollbar(lw);
	DrawList(lw, NULL, TRUE);
    }
}

#ifdef NOVELL
/************************************************************************
 *									*
 * XmListXYToPos - return the index of the item underneath position X/Y *
 *	returns 0 if there is no item at position X/Y.			*
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListXYToPos( w, x, y ) 
        Widget w ;
        Position x ;
        Position y ;
#else
XmListXYToPos(
        Widget w,
        Position x,
        Position y )
#endif /* _NO_PROTO */
{
/****************
 *
 * Remember to convert to the 1-based user world
 *
 ****************/
/* BEGIN OSF Fix CR 5662 */
	if (y < 0 || (int)y >= (int)CPART(w).height - (int)LPART(w).BaseY)
/* END OSF Fix CR 5662 */
		return (0);
	else
		return (WhichItem((XmListWidget)w,
				False /* static_rows_as_topleave */, x, y) + 1);
} /* end of XmListXYToPos */

#endif /* NOVELL */

/************************************************************************
 *									*
 * XmListYToPos - return the index of the item underneath position Y    *
 *	returns 0 if there is no item at position Y.			*
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListYToPos( w, y ) 
        Widget w ;
        Position y ;
#else
XmListYToPos(
        Widget w,
        Position y )
#endif /* _NO_PROTO */
{
/****************
 *
 * Remember to convert to the 1-based user world
 *
 ****************/
#ifdef NOVELL
		/* Can only return 1st visible column of that row
		 * because lack of x info */
	return(XmListXYToPos(w, 1, y));
#else /* NOVELL */
/* BEGIN OSF Fix CR 5081 */
  XmListWidget lw = (XmListWidget)w;
  
/* BEGIN OSF Fix CR 5662 */
  if ((y < 0) || (y >= (lw->core.height - lw->list.BaseY)))
/* END OSF Fix CR 5662 */
    return (0);
  else
    return (WhichItem(lw, y) + 1);
#endif /* NOVELL */
/* END OSF Fix CR 5081 */
}


/************************************************************************
 *									*
 * XmListPosToBounds                                                    *
 *	                                                                *
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListPosToBounds( w, position, x, y, width, height )
        Widget      w ;
        int         position ;
        Position   *x ;
        Position   *y ;
        Dimension  *width ;
        Dimension  *height ;
#else
XmListPosToBounds(
        Widget      w,
        int         position,
        Position   *x,
        Position   *y,
        Dimension  *width,
        Dimension  *height )
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	ht;
	int	xx, yy, wd, hi;

	if (!XtIsRealized(w) || !LPART(w).itemCount)
		return(False);

/* BEGIN OSF Fix CR 5764 */
    /* Remember we're 0-based */
	if (position == 0) 
		position = LPART(w).itemCount - 1;
	else
		position--;
/* END OSF Fix CR 5764 */

	if ((ht = LPART(w).HighlightThickness) < 1)
		ht = 0;

		/* should we allow an app to locate a title item,
		 * if so, additional check is needed! */
	if (CalcItemBound(w, position, ht, (unsigned long *)NULL,
						&xx, &yy, &wd, &hi) <= 0)
		return(False);

	*x = xx;
	*y = yy;
	*width = wd;
	*height = hi;
	return(True);

#else /* NOVELL */
    register XmListWidget lw;
    register Dimension    ht;

    Position   ix;          /* values computed ahead...  */
    Position   iy;          /* ...of time...             */
    Dimension  iwidth;      /* ...for debugging...       */
    Dimension  iheight;     /* ...purposes               */

    if (!XtIsRealized(w)) return (False);

    lw = (XmListWidget) w;
/* BEGIN OSF Fix CR 5764 */
    /* Remember we're 0-based */
    if (position == 0) 
        position = lw->list.itemCount - 1;
    else
        position--;			
/* END OSF Fix CR 5764 */

    if ((position >=  lw->list.itemCount)    ||
        (position <   lw->list.top_position) ||
	(position >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return (False);

    ht = lw->list.HighlightThickness;
    if (ht < 1) ht = 0;

    ix = lw->list.BaseX - ht;
    iwidth = lw->core.width - 2 * ((int )lw->list.margin_width
		  	               + lw->primitive.shadow_thickness);

    iy = (lw->list.InternalList[position]->CumHeight -
	  lw->list.InternalList[lw->list.top_position]->CumHeight) +
	  lw->list.BaseY - ht;
    iheight = lw->list.MaxItemHeight + (2 * ht);

    if ( x      != NULL )   *x      = ix;
    if ( y      != NULL )   *y      = iy;
    if ( height != NULL )   *height = iheight;
    if ( width  != NULL )   *width  = iwidth;

    return (True);
#endif /* NOVELL */
}
/************************************************************************
 *									*
 * XmListUpdateSelectedList - regen the selected list.			*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListUpdateSelectedList( w )
        Widget w ;
#else
XmListUpdateSelectedList(
        Widget w )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    UpdateSelectedList(lw);
/* BEGIN OSF Fix CR 5409 */
/* END OSF Fix CR 5409 */
}
/************************************************************************
 *									*
 * XmListPosSelected - Return selection state of item at position	*
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListPosSelected(w, pos)
     Widget	w;
     int	pos;
#else
XmListPosSelected(
    Widget w, 
    int pos)
#endif /* _NO_PROTO */
{
#ifdef NOVELL
	int	int_pos;
  
	if (!LPART(w).itemCount || pos < 0 || pos > LPART(w).itemCount)
		return(False);
  
	if (pos == 0)
		int_pos = LPART(w).itemCount - 1;
	else
		int_pos = pos - 1;
  
	return (LPART(w).InternalList[int_pos]->selected);
#else /* NOVELL */
  int		int_pos;
  XmListWidget	lw = (XmListWidget) w;
  
  if ((lw->list.items == NULL) || (pos < 0) || (pos > lw->list.itemCount))
    return(False);
  
  if (pos == 0) int_pos = lw->list.LastItem - 1;
  else int_pos = pos - 1;
  
  return (lw->list.InternalList[int_pos]->selected);
#endif /* NOVELL */
}

/************************************************************************
 *									*
 * XmCreateList - hokey interface to XtCreateWidget.			*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateList( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateList(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{

    return ( XtCreateWidget( name,
			     xmListWidgetClass,
			     parent,
			     args,
			     argCount ) );

}

/************************************************************************
 *									*
 * XmCreateScrolledList - create a list inside of a scrolled window.	*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateScrolledList( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateScrolledList(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
    Widget sw, lw;
    int i;
    char *s;
    ArgList Args;

/* NOVELL: ALLOCATE_LOCAL/DEALLOCATE_LOCAL in 386/486 platform are just
 * XtMalloc/XtFree, we should do some optimization here... */
    s = ALLOCATE_LOCAL(((name) ? strlen(name) : 0) + 3); /* Name+NULL+"SW" */
    if (name) {
       strcpy(s, name);
       strcat(s, "SW");
    } else {
       strcpy(s, "SW");
    }

/* NOVELL: can use some optimization here... */
    Args = (ArgList) XtCalloc(argCount+4, sizeof(Arg));
    for (i = 0; i < argCount; i++)
    {
        Args[i].name = args[i].name;
        Args[i].value = args[i].value;
    }

    XtSetArg (Args[i], XmNscrollingPolicy, (XtArgVal )XmAPPLICATION_DEFINED); i++;
    XtSetArg (Args[i], XmNvisualPolicy, (XtArgVal )XmVARIABLE); i++;
    XtSetArg (Args[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmSTATIC); i++;
    XtSetArg (Args[i], XmNshadowThickness, (XtArgVal ) 0); i++;
    sw = XtCreateManagedWidget(s , xmScrolledWindowWidgetClass, parent,
                               (ArgList)Args, i);
    DEALLOCATE_LOCAL(s);
/* BEGIN OSF Fix CR 5460 */

    i=argCount;
#ifndef NOVELL
	/* I don't understand why it always forces the policy to be
	 * XmSTATIC, don't force it in the NOVELL copy!! Don't know
	 * how ScrollBarDisplayPolicyDefault() (CDE_FILESB) will work!!! */
    XtSetArg (Args[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmSTATIC); i++;
#endif /* NOVELL */
    lw = XtCreateWidget( name, xmListWidgetClass, sw, Args, i);
    XtFree((char *) Args);

/* END OSF Fix CR 5460 */
    XtAddCallback (lw, XmNdestroyCallback, _XmDestroyParentCallback, NULL);
    return (lw);

}

#ifdef NOVELL

static void
#ifdef _NO_PROTO
CreateInternalList(w)
	Widget	w;
#else
CreateInternalList(Widget w)
#endif
{
	register int     i;

		/* Assume caller already did itemCount check */
	CopyItems((XmListWidget)w);

	LPART(w).InternalList = (ElementPtr *)XtMalloc(sizeof(ElementPtr) *
						       LPART(w).itemCount);

	for (i = 0; i < LPART(w).itemCount; i++)
	{
		(void)AddInternalElement(
			(XmListWidget)w, LPART(w).items[i], i + 1,
			OnSelectedList((XmListWidget)w, LPART(w).items[i]),
			False);
	}

		/* Now calculate the CumHeight... */
	CalcCumHeight(w);
} /* end of CreateInternalList */

static void
#ifdef _NO_PROTO
CalcCumHeight(w)
	Widget	w;
#else
CalcCumHeight(Widget w)
#endif
{
	int	i, j, k;
	int	rows, last_row_size;

	CalcNumRows(w, &rows, &last_row_size);

	for (i = 0; i < (int)LPART(w).cols; i++) /* init the very first row */
	{
			/* we don't need to continue if we located
			 * an empty column in last row */
		if (EmptyColInLastRow(0, rows, last_row_size, i))
			break;

		LPART(w).InternalList[i]->CumHeight = LPART(w).MaxItemHeight;
	}
	for (i = 0; i < (int)LPART(w).cols; i++)
	{
		for (j = 1; j < rows; j++)
		{
				/* we don't need to continue if we located
				 * an empty column in last row */
			if (EmptyColInLastRow(j, rows, last_row_size, i))
				break;

#define PREV		k - LPART(w).cols

			k = i + j * LPART(w).cols;
			LPART(w).InternalList[k]->CumHeight =
					LPART(w).MaxItemHeight +
					LPART(w).InternalList[PREV]->CumHeight +
					LPART(w).spacing;
#undef PREV
		}
	}
} /* end of CalcCumHeight */

static void
#ifdef _NO_PROTO
CalcNumRows(w, rows, last_row_size)
	Widget	w;
	int *	rows;
	int *	last_row_size;
#else
CalcNumRows(Widget w, int * rows, int * last_row_size)
#endif
{
	*rows = LPART(w).itemCount / (int)LPART(w).cols;
		/* # of items in last row */
	*last_row_size = LPART(w).itemCount % (int)LPART(w).cols;

	if (*last_row_size)
		*rows += 1;
	else
		*last_row_size = LPART(w).cols;
} /* end of CalcNumRows */

static void
#ifdef _NO_PROTO
ItemDraw(w, do_fill, item_x, item_y, pos, col, viz_wd, cum_wd, bdr)
	Widget	w;
	Boolean	do_fill;
	int	item_x;
	int	item_y;
	int	pos;
	int	col;
	int	viz_wd;
	int	cum_wd;
	int	bdr;
#else
ItemDraw(Widget w, Boolean do_fill, int item_x, int item_y,
	 int pos, int col,
	 int viz_wd, int cum_wd, int bdr)
#endif
{
	GC		gc;
	int		x, wd;
	int		glyph_y, glyph_wd;
	int		clip_x, clip_y, delta, ht2;

#define MAX_COL_WD	(int)LPART(w).max_col_width[col]
#define DELTA_WD	(int)0

	if ((wd = CalcItemWidth(
			w, &x, (unsigned long *)NULL, item_x, LPART(w).BaseX,
			bdr, MAX_COL_WD, DELTA_WD, cum_wd)) <= 0)
	{
		return;
	}

#undef DELTA_WD

	if (do_fill)
	{
		XFillRectangle(
			XtDisplay(w), XtWindow(w),
			(LPART(w).InternalList[pos]->selected ?
					LPART(w).NormalGC : LPART(w).InverseGC),
			x, item_y,
			wd, LPART(w).MaxItemHeight);
	}

#ifndef OSF_v1_2_4
	if (!CPART(w).sensitive)
#else /* OSF_v1_2_4 */
	if (!XtIsSensitive(w))
#endif /* OSF_v1_2_4 */
	{
		gc = LPART(w).InsensitiveGC;
	}
	else
	{
		if (LPART(w).InternalList[pos]->selected)
		{
			gc = LPART(w).InverseGC;
		}
		else
		{
			gc = LPART(w).NormalGC;
		}
	}

#define GLYPH_X		x	/* assume we can reuse `x' after `do_fill' */
#define GLYPH_DATA	LPART(w).InternalList[pos]->glyph_data
#define GLYPH_ON_LEFT	(LPART(w).InternalList[pos]->glyph_data->glyph_pos ==\
							XmGLYPH_ON_LEFT)
#define GLYPH_ON_RIGHT	(LPART(w).InternalList[pos]->glyph_data->glyph_pos ==\
							XmGLYPH_ON_RIGHT)
#define PIXMAP		LPART(w).InternalList[pos]->glyph_data->pixmap
#define MASK		LPART(w).InternalList[pos]->glyph_data->mask
#define DEPTH		LPART(w).InternalList[pos]->glyph_data->depth
#define ITEM_WD		LPART(w).InternalList[pos]->width
#define GLYPH_WD	(int)LPART(w).InternalList[pos]->glyph_data->width
#define GLYPH_HI	LPART(w).InternalList[pos]->glyph_data->height
#define GLYPH_H_PAD	LPART(w).InternalList[pos]->glyph_data->h_pad
#define GLYPH_V_PAD	LPART(w).InternalList[pos]->glyph_data->v_pad

	GLYPH_X = item_x;
	if (LPART(w).StrDir == XmSTRING_DIRECTION_R_TO_L)
	{
		item_x += MAX_COL_WD - ITEM_WD;
	}

	if (GLYPH_DATA && GLYPH_ON_LEFT)
	{
		item_x += GLYPH_WD + GLYPH_H_PAD;
	}

/****************
 *
 * You notice that I'm forcing the string direction to L_TO_R, regardless
 * of the actual value. This is because the drawstring code tries to be
 * smart about but what I really want, but it just gets in the way. So, I
 * do my own calculations, and lie to the draw code.
 *
 ****************/

#define ITEM_Y	item_y + ((int)LPART(w).MaxItemHeight -\
			  (int)LPART(w).InternalList[pos]->height) / 2

	_XmStringDraw(
		XtDisplay(w), XtWindow(w),
		LPART(w).font,
		LPART(w).InternalList[pos]->name,
		gc,
		item_x,
		ITEM_Y,
		LPART(w).max_col_width[col],
		XmALIGNMENT_BEGINNING,
		XmSTRING_DIRECTION_L_TO_R,
		NULL);

	if (!GLYPH_DATA)
		return;

	if (GLYPH_ON_RIGHT)
	{
		if (LPART(w).StrDir == XmSTRING_DIRECTION_R_TO_L)
			GLYPH_X += MAX_COL_WD - GLYPH_WD;
		else
			GLYPH_X += ITEM_WD - GLYPH_WD;
	}
	else /* GLYPH_ON_LEFT */
	{
		if (LPART(w).StrDir == XmSTRING_DIRECTION_R_TO_L)
			GLYPH_X += MAX_COL_WD - ITEM_WD;
		/* else no need to adjust because GLYPH_X == item_x */
	}

	glyph_y = ITEM_Y;
	if (!GLYPH_V_PAD)	/* center it */
		glyph_y += ((int)LPART(w).MaxItemHeight - (int)GLYPH_HI) / 2;
	else
		glyph_y += GLYPH_V_PAD;

		/* can be futher optimized by checking lower right point,
		 * return right away if the lower right pointer is not in
		 * view. Should have a special check when both upper left
		 * and lower right points are not in view but the middle
		 * part is! This optimization is useful only when having
		 * pretty glyph and this usually won't happen (I think). */
	if (!LPART(w).static_rows) {

				/* See SetClipRect for ht2... */
		delta = 0;
		ht2   = 0;
	} else {

		delta = PPART(w).highlight_thickness;
		ht2   = LPART(w).HighlightThickness;
	}

	clip_x = GLYPH_X < LPART(w).BaseX - delta ? LPART(w).BaseX - delta :
						    GLYPH_X;
	clip_y = glyph_y < LPART(w).BaseY - delta ? LPART(w).BaseY - delta :
						    glyph_y;

		/* Adjust glyph_wd when the lower right point is not in view,
		 * return to the caller if the adjusted width <= 0 */
	if (GLYPH_X + GLYPH_WD > ((int)CPART(w).width - bdr))
	{
		/* CalcItemWidth() should use same equation... */
		/* GLYPH_WD - GLYPH_X - GLYPH_WD + (int)CPART(w).width - bdr */
		if ( (glyph_wd = (int)CPART(w).width - bdr - GLYPH_X) <= 0 )
			return;
	}
	else
	{
		glyph_wd = GLYPH_WD;
	}

	XSetClipOrigin(XtDisplay(w), gc, clip_x, clip_y);
	if (MASK != None)
		XSetClipMask(XtDisplay(w), gc, MASK);

	if (DEPTH == 1)
		XCopyPlane(XtDisplay(w), PIXMAP, XtWindow(w), gc,
				0, 0, glyph_wd, GLYPH_HI,
				GLYPH_X, glyph_y,
				1);
	else
		XCopyArea(XtDisplay(w), PIXMAP, XtWindow(w), gc,
				0, 0, glyph_wd, GLYPH_HI,
				GLYPH_X, glyph_y);

#define CLIP_X_ORIGIN	(LPART(w).BaseX - delta)
#define CLIP_Y_ORIGIN	(LPART(w).BaseY - delta)

	if (MASK == None)
	{
		XSetClipOrigin(XtDisplay(w), gc, CLIP_X_ORIGIN, CLIP_Y_ORIGIN);
	}
	else
	{
		XRectangle	rect;

		rect.x = rect.y = 0;
		
			/* check underflow */
		if ((rect.width = CPART(w).width - 2*CLIP_X_ORIGIN + ht2) <= 0)
			rect.width = 1;

		if ((rect.height= CPART(w).height - 2*CLIP_Y_ORIGIN + ht2) <= 0)
			rect.height = 1;

		XSetClipRectangles(
			XtDisplay(w), gc,
			CLIP_X_ORIGIN, CLIP_Y_ORIGIN, &rect, 1, Unsorted);
	}

#undef MAX_COL_WD
#undef ITEM_Y
#undef GLYPH_X
#undef GLYPH_DATA
#undef GLYPH_ON_LEFT
#undef GLYPH_ON_RIGHT
#undef PIXMAP
#undef MASK
#undef DEPTH
#undef ITEM_WD
#undef GLYPH_WD
#undef GLYPH_HI
#undef GLYPH_H_PAD
#undef GLYPH_V_PAD
#undef CLIP_X_ORIGIN
#undef CLIP_Y_ORIGIN
} /* end of ItemDraw */

static int
#ifdef _NO_PROTO
CalcItemWidth(w, new_x, where, item_x, base_x, bdr, init_wd, delta_wd, cum_wd)
	Widget	w;
	int *	new_x;
	unsigned long *	where;
	int	item_x, base_x, bdr;
	int	init_wd, delta_wd;
	int	cum_wd;
#else
CalcItemWidth(Widget w, int * new_x, unsigned long * where,
	      int item_x, int base_x, int bdr,
	      int init_wd, int delta_wd, int cum_wd)
#endif
{
	Boolean	adj_wd = False;
	int	wd;

#define ITEM_RI_X	(item_x + init_wd)
#define VIEW_RI_X	((int)CPART(w).width - bdr)

	if (item_x >= 0)
	{
		*new_x = item_x;
		if (ITEM_RI_X > VIEW_RI_X)
			adj_wd = True;
	}
	else
	{
		*new_x = base_x;
		adj_wd = True;
	}

	if (!adj_wd)
	{
		wd = init_wd;
	}
	else
	{
		if (item_x > 0)
		{
			wd = init_wd - delta_wd - cum_wd + CPART(w).width +
			     LPART(w).XOrigin - bdr;
		}
		else
		{
			if (ITEM_RI_X > VIEW_RI_X)
			{
				wd = CPART(w).width - 2 * bdr;
			}
			else
			{
				wd = cum_wd - LPART(w).XOrigin - bdr + delta_wd;
			}
		}
	}

	if (where)
	{
		if (wd > 0)
		{
			*where |= ITEM_ON_VIEW;
		}
		else
		{
			if (item_x > 0)
				*where |= ITEM_ON_RIGHT;
			else
				*where |= ITEM_ON_LEFT;
		}
	}
#undef LOWER_RI

	return(wd);
} /* end of CalcItemWidth */

/*
 * This function returns `*wd',
 * `*y' and `*hi' are not initialized if *wd <= 0 (goes to *x and *wd if
 * the given item is not in the view, the first check in the code below.
 */
static int
#ifdef _NO_PROTO
CalcItemBound(w, pos, ht, where, x, y, wd, hi)
	Widget	w;
	int	pos;
	int	ht;
	unsigned long *	where;
	int *	x;
	int *	y;
	int *	wd;
	int *	hi;
#else
CalcItemBound(Widget w, int pos, int ht, unsigned long * where,
	      int * x, int * y, int * wd, int * hi)
#endif
{
	int	row, col, i, bdr, item_x, cum_wd;
	Boolean	reset_wd = False;

	row = pos / (int)LPART(w).cols;
	if (pos >= LPART(w).itemCount)
		return((int)0);

	col = pos % (int)LPART(w).cols;	/* nth col */

	*x = bdr = LPART(w).BaseX - ht; /* shadow + margin */

		/* adjust x because virtual coords */
	*x -= LPART(w).XOrigin;

	for (i = 0; i < col; i++)
		*x += LPART(w).max_col_width[i] + 2 * ht +
		     LPART(w).col_spacing;

	item_x = *x;

	if (where)
		*where = 0;

	if (row <  TOP_ITEM_ROW(w)	||
	    row >= BOT_ITEM_ROW(w))
	{
		if (!where)
			return((int)0);

		reset_wd = True;
		if (row < TOP_ITEM_ROW(w))
			*where |= ITEM_ON_TOP;
		else
			*where |= ITEM_ON_BELOW;
	}

	cum_wd = LPART(w).BaseX + LPART(w).max_col_width[0];
	for (i = 1; i < col + 1; i++)
		cum_wd += LPART(w).max_col_width[i] +
			  LPART(w).col_spacing + 2 *
			  LPART(w).HighlightThickness;

#define BASE_X		bdr
#define DELTA_WD	(int)ht

	if ((*wd = CalcItemWidth(
		w, x, where, item_x, BASE_X, bdr,
		(int)LPART(w).max_col_width[col] + 2 * (int)ht,
		DELTA_WD, cum_wd)) > 0)
	{
		if (reset_wd)
		{
			*wd = 0;
		}
		else
		{
			int	top_pos = LPART(w).top_position * LPART(w).cols;

			*y = LPART(w).InternalList[pos]->CumHeight -
			     LPART(w).InternalList[top_pos+col]->CumHeight +
			     LPART(w).BaseY - ht;
			*hi = LPART(w).MaxItemHeight + 2 * ht;
		}
	}
#undef BASE_X
#undef DELTA_WD

	return(*wd);
} /* end of CalcItemBound */

static Boolean
#ifdef _NO_PROTO
CheckHsb(w, item)
	Widget		w;
	int		item;
#else
CheckHsb(Widget w, int item)
#endif
{
	Boolean	draw_list = False;

	if (LPART(w).hScrollBar)
	{
		int		ht, x, y, wd, hi;
		unsigned long	where;

		if ((ht = LPART(w).HighlightThickness) < 1)
			ht = 0;

		if (CalcItemBound(w, item, ht, &where,
					 &x, &y, &wd, &hi) <= 0 &&
		    ((where & ITEM_ON_RIGHT) || (where & ITEM_ON_LEFT)))
		{
			draw_list = True;
			CalibrateHsb(w, item, where);
		}
	}

	return(draw_list);
} /* end of CheckHsb */

static void
#ifdef _NO_PROTO
CalibrateHsb(w, item, where)
	Widget		w;
	int		item;
	unsigned long	where;
#else
CalibrateHsb(Widget w, int item, unsigned long where)
#endif
{
	int	i, col, x;

	x = 0;
	col = item % (int)LPART(w).cols;

	for (i = 0; i < col; i++)
		x += LPART(w).max_col_width[i] +
		     LPART(w).col_spacing + 2 *
		     LPART(w).HighlightThickness;

	if (where & ITEM_ON_RIGHT)
	{
		int	avail_wd = CPART(w).width - 2 * (
				   (int)LPART(w).margin_width +
				   (int)LPART(w).HighlightThickness +
				   (int)PPART(w).shadow_thickness);

		if (avail_wd > (int)LPART(w).max_col_width[col])
		{
			x -= (avail_wd - LPART(w).max_col_width[col]);
		}
	}

	LPART(w).hOrigin = LPART(w).XOrigin = x;
	SetHorizontalScrollbar((XmListWidget)w);
} /* end of CalibrateHsb */

static Boolean
#ifdef _NO_PROTO
CalibrateVsb(w, item, where, delta_row)
	Widget		w;
	int		item;
	unsigned long	where;
	int		delta_row
#else
CalibrateVsb(Widget w, int item, unsigned long where, int delta_row)
#endif
{
	int	row, top;
	Arg	arg[1];

	row = item / (int)LPART(w).cols;
		/* Note that, top_position is always static_rows behind */
	if (where & ITEM_ON_TOP)
	{
		top = row - LPART(w).static_rows - delta_row;
	}
	else
	{
		top = row + 1 - LPART(w).visibleItemCount - delta_row;

	}

	if (top < 0)
		top = 0;

	if (top == LPART(w).top_position)
		return(False);
	else
		LPART(w).top_position = top;

	if (LPART(w).vScrollBar)
	{
		SetVerticalScrollbar((XmListWidget)w);
	}

	return(True);
} /* end of CalibrateVsb */

/* Assume that the caller already set el->glyph_data to NULL */
static void
#ifdef _NO_PROTO
SetElement(w, string, position, el)
	Widget		w;
	XmString	string;
	int		position;
	ElementPtr	el;
#else
SetElement(Widget w, XmString string, int position, ElementPtr el)
#endif
{
	XmString	label;
	Dimension	glyph_wd = 0,
			glyph_hi = 0;

	if (!(label = string) && LPART(w).item_init_cb)
	{
		XmListItemInitCallbackStruct	cd;

			/* Note that defining `ev' in this structure is
			 * just trying to match the Motif generic call
			 * data structure, see XmAnyCallbackStruct */
		cd.reason	 = XmCR_ASK_FOR_ITEM_DATA;
		cd.event	 = (XEvent *)NULL; /* always NULL */
		cd.position	 = position ? position : LPART(w).LastItem;
		cd.label	 = (XmString)NULL;
		cd.pixmap	 =
		cd.mask		 = (Pixmap)None;
		cd.depth	 =
		cd.width	 =
		cd.height	 =
		cd.v_pad	 =
		cd.h_pad	 = 0;
		cd.glyph_pos	 = XmGLYPH_ON_LEFT;
		cd.static_data	 = False;

		XtCallCallbackList(w, LPART(w).item_init_cb, (XtPointer)&cd);

		if ( (cd.pixmap != (Pixmap)None && cd.depth &&
		      cd.width && cd.height) )
		{
			GlyphData * ptr;

			ptr = (GlyphData *)XtMalloc(sizeof(GlyphData));
			el->glyph_data = ptr;

			ptr->pixmap	   = cd.pixmap;
			ptr->mask	   = cd.mask;
			ptr->depth	   = cd.depth;
			ptr->width	   = cd.width;
			ptr->height	   = cd.height;
			ptr->h_pad	   = cd.h_pad;
			ptr->v_pad	   = cd.v_pad;
			ptr->glyph_pos	   = cd.glyph_pos;
			ptr->static_data   = cd.static_data;

			glyph_wd = cd.width + cd.h_pad;
			glyph_hi = cd.height + 2 * cd.v_pad;
		}
		if (label = cd.label)
			LPART(w).items[position - 1] = XmStringCopy(label);
	}

	if (label)
	{
		el->name = _XmStringCreate(label);
		el->length = XmStringLength(label);
		_XmStringExtent(
			LPART(w).font, el->name, &el->width, &el->height);
		el->width += glyph_wd;
		el->NumLines = _XmStringLineCount(el->name);

		if (glyph_hi > el->height)
			el->height = glyph_hi;
	}
	else
	{
		el->name = NULL;
		el->length = 0;
		el->NumLines = 0;
		el->width = glyph_wd;
		el->height = glyph_hi;
	}
} /* end of SetElement */

/* Assume caller already went thru checking, all given entries are valid
 * when this routine is called */
static void
#ifdef _NO_PROTO
ReplacePositions(w, position_list, item_list, item_count, selected)
	Widget		w;
	int *		position_list;
	XmString *	item_list;
	int		item_count;
	Boolean		selected;
#else
ReplacePositions(Widget w, int * position_list,
		 XmString * item_list, int item_count, Boolean selected)
#endif
{
	int		item_pos;
	register int	i;
	Boolean		re_draw = False;

#define LW	(XmListWidget)w

	for (i = 0; i < item_count; i++)
	{
		item_pos = position_list[i];

		if (item_pos < 1 || item_pos > LPART(w).itemCount)
		{
			_XmWarning(w, ListMessage8);
		}
		else
		{
			int	row = (item_pos - 1) / (int)LPART(w).cols;

				/* Set re_draw to True, if it's a title item
				 * or the item is currently in view */
			if (!re_draw && item_pos - 1 < BOT_VIZ_ITEM(w) + 1)
				re_draw = True;

			ReplaceItem(LW, item_list[i], item_pos);
			ReplaceInternalElement(
				LW, item_pos,
				TITLE_ROW(w, row) ? False : selected);
		}
	}

	CalcNewSize(LW, re_draw);

#undef LW
} /* end of ReplacePositions */

/* Assume Caller already checks the glyph_data pointer */
static void
#ifdef _NO_PROTO
FreeGlyphData(w, item)
	Widget		w;
	ElementPtr	item;
#else
FreeGlyphData(Widget w, ElementPtr item)
#endif
{
	if (item->glyph_data->static_data == False)
	{
		if (item->glyph_data->pixmap != None)
			XFreePixmap(XtDisplay(w), item->glyph_data->pixmap);

		if (item->glyph_data->mask != None)
			XFreePixmap(XtDisplay(w), item->glyph_data->mask);
	}
	XtFree((char *)item->glyph_data);
	item->glyph_data = NULL;
} /* end of FreeGlyphData */

/* Assume caller already itemCount */
static void
#ifdef _NO_PROTO
DestroyInternalList(w)
	Widget	w;
#else
DestroyInternalList(Widget w)
#endif
{
	register int	i;
	int		j;
	int *		pos_list;

	j = LPART(w).itemCount;
	pos_list = (int *)XtMalloc(sizeof(int) * j);

	for (i = 0; i < LPART(w).itemCount; i++)
		pos_list[i] = i + 1;

		/* Order is important here, because itemCount is 0 after
		 * ClearItemList is called */
	ClearItemList((XmListWidget)w);
	DeleteInternalElementPositions((XmListWidget)w, pos_list, j, j, False);
	XtFree((char *)LPART(w).InternalList);
        LPART(w).InternalList = NULL;

		/* Have to reset it to zero, because in Motif 1.2.3,
		 * ClearItemList() will set LastItem to 0, but
		 * DeleteInternalElementPositions() still decreases it,
		 * so it can become negative number... */
        LPART(w).LastItem = 0;

	XtFree((char *)pos_list);
} /* end of DestroyInternalList */

static Boolean
#ifdef _NO_PROTO
TreatSelectionData(w, op, data, total_size, total)
	Widget				w;
	unsigned long			op;
	XmListDragConvertStruct *	data;
	int *				total_size;
	char **				total;
#else
TreatSelectionData(Widget w, unsigned long op, XmListDragConvertStruct * data,
			int * total_size, char ** total)
#endif
{
	register int	i;
	Boolean		ret_val = True;

		/* We shouldn't totoally rely on the info in selectedIndices[],
		 * because this list can out of sync if people didn't use List
		 * API carefully. There are lots of places will generate this
		 * list on the fly, maybe we should totally ignored these info.
		 * It's weird enough to keep two arrays just for the info
		 * you really can't rely on it... sigh!
		 *
		 * See comments in XmListGetSelectedPos().
		 */
	if (op & SETUP_SELECTED_LIST)
	{
#define new_code
#ifdef new_code
		int *	list;
		int	count;

			/* we are in big trouble if failed */
		if (!XmListGetSelectedPos(w, &list, &count))
		{
			data->strings = NULL;
			data->num_strings = 0;
			return(False);
		}

		data->strings = (XmString *)XtMalloc(sizeof(XmString) * count);
		data->num_strings= count;

#define I	list[i] - 1
		for (i = 0; i < count; i++)
			data->strings[i] = XmStringCopy(LPART(w).items[I]);
#undef I

		XtFree((char *)list);
#else /* old code */
		data->strings = (XmString *)XtMalloc(sizeof(XmString) *
						LPART(w).selectedItemCount);
		data->num_strings= LPART(w).selectedItemCount;

#define I	LPART(w).selectedIndices[i] - 1
		for (i = 0; i < LPART(w).selectedItemCount; i++)
			data->strings[i] = XmStringCopy(LPART(w).items[I]);
#undef I
#endif /* old code */
#undef new_code
	}

	if (op & SETUP_CT_DATA)
	{
		XrmValue	from_val, to_val;
		Boolean		ok;

		*total_size = 0;
		*total = NULL;

		if (data->num_strings == 1)	/* No trailing newline */
		{
			from_val.addr = (char *)data->strings[0];
			if ((ok = _XmCvtXmStringToCT(&from_val, &to_val)))
			{
				*total_size += to_val.size;
				*total = XtRealloc((char *)*total, *total_size);
				memcpy(&(*total)[*total_size - to_val.size],
						to_val.addr, to_val.size);
			}
		}
		else	/* Newline separated and trailing (b.c.) */
		{
		  for (i = 0; i < data->num_strings; i++)
		  {
			from_val.addr = (char *)data->strings[i];
			ok = _XmCvtXmStringToCT(&from_val, &to_val);

			if (ok && to_val.size) 
			{
				*total_size += to_val.size;
				*total = XtRealloc(
						*total, (*total_size +
						 NEWLINESTRING_LEN));
				memcpy(&(*total)[*total_size - to_val.size],
						to_val.addr, to_val.size);
				memcpy(&(*total)[*total_size], NEWLINESTRING,
						NEWLINESTRING_LEN);

				*total_size += NEWLINESTRING_LEN;
			}
		  }
		}
		ret_val = (*total_size) ? True : False;

			/* Borrow DragDropFinished procedure to free up
			 * XmString copies and the `strings' pointer */
		if (op & FREE_SELECTED_LIST)
			DragDropFinished(w, NULL, (XtPointer)data);
	}

	return(ret_val);
} /* end of TreatSelectionData */

static Boolean
#ifdef _NO_PROTO
SetVizPos(w, pos, where)
	Widget		w;
	int		pos;	/* 1-based */
	unsigned long	where;	/* ITEM_ON_TOP or ITEM_ON_BELOW */
#else
SetVizPos(Widget w, int pos, unsigned long where)
#endif
{
	Boolean		draw_list = False; /* Treat illegal case as True */

	if (!LPART(w).itemCount)
		return(draw_list);

	if (!pos)
		pos = LPART(w).itemCount;

	if (pos > FIRST_ITEM(w) && pos <= LPART(w).itemCount)
	{
		Boolean draw_V, draw_H;

		draw_V = CalibrateVsb(w, pos - 1, where, 0);

			/* may need to adjust hsb */
		draw_H = CheckHsb(w, pos - 1);

		draw_list = draw_V || draw_H;

		if (draw_list)
			DrawList((XmListWidget)w, NULL, True);
	}
	return(draw_list);
} /* end of SetVizPos */

static void
#ifdef _NO_PROTO
CalcNewSize(lw, re_draw)
	XmListWidget	lw;
	Boolean		re_draw;
#else
CalcNewSize(XmListWidget lw, Boolean re_draw)
#endif
{
	Boolean		size_changed;

		/* SetNewSize() returns True if width/height is changed */
	size_changed = SetNewSize(lw);

		/* When having time, we should check why calling these
		 * two routines, it seems to me, nothing changes */
	if (LPART(lw).SizePolicy != XmVARIABLE || LPART(lw).cols != 1)
		SetHorizontalScrollbar(lw);

	SetVerticalScrollbar(lw);

		/* Force a re-draw only when width/height didn't change and
		 * re_draw is set, otherwise, SetNewSize() will force an
		 * expose via XtSetValues() in SetNewSize() */
	if (!size_changed && re_draw)
		DrawList(lw, NULL, True);
} /* end of CalcNewSize */

static void
#ifdef _NO_PROTO
ExtendToTopOrBot(w, on_top)
	Widget		w;
	Boolean		on_top;
#else
ExtendToTopOrBot(Widget w, Boolean on_top)
#endif
{
#define LW	(XmListWidget)w

	int	new_kbd_item, old_kbd_item;
	Boolean	list_drawn;

	if (!LPART(w).itemCount ||
	    LPART(w).itemCount <= FIRST_ITEM(w) ||
	    LPART(w).SelectionPolicy == XmBROWSE_SELECT ||
	    LPART(w).SelectionPolicy == XmSINGLE_SELECT)
		return;

	LPART(w).Event |= (SHIFTDOWN);

	old_kbd_item = LPART(w).CurrentKbdItem;
	if (LPART(w).Mom)
	{
		new_kbd_item = on_top ? FIRST_ITEM(w) : LPART(w).itemCount - 1;
	}
	else
	{
		int	delta1, delta2;

			/* See TOP_VIZ_ITEM() and BOT_VIZ_ITEM() above */
		if (on_top)
		{
			delta1 = (int)LPART(w).static_rows;
			delta2 = 0;
		}
		else
		{
			delta1 = LPART(w).visibleItemCount;
			delta2 = 1;
		}

		new_kbd_item = (LPART(w).top_position + delta1) *
						(int)LPART(w).cols - delta2;

		if (new_kbd_item >= LPART(w).itemCount)
			new_kbd_item = LPART(w).itemCount - 1;
	}

	if (LPART(w).CurrentKbdItem == new_kbd_item)
		return;

	LPART(w).CurrentKbdItem = new_kbd_item;

	if (LPART(w).Mom)
		list_drawn = MakeItemVisible(LW, new_kbd_item, 0);
	else
		list_drawn = False;

	if (!list_drawn)
	{
		DrawHighlight(LW, old_kbd_item, False);
		DrawHighlight(LW, new_kbd_item, True);
	}

		/* The old code below doesn't make sense because
		 * this routine will return immediately, if
		 * SelectionPolicy == XmBROWSE_SELECT */
#ifdef old_code
	if (LPART(w).AutoSelect &&
	    LPART(w).SelectionPolicy == XmBROWSE_SELECT)
			HandleNewItem(LW, new_kbd_item, old_kbd_item);
	else
#endif
        if (LPART(w).SelectionPolicy == XmEXTENDED_SELECT)
		HandleExtendedItem(LW, new_kbd_item);

	LPART(w).Event = 0;

#undef LW
} /* end of ExtendToTopOrBot */

static void
#ifdef _NO_PROTO
GotoTopOrEnd(w, where)
	Widget		w;
	unsigned long	where;
#else
GotoTopOrEnd(Widget w, unsigned long where)
#endif
{
	int	item;	/* 1-based */

	if (!LPART(w).itemCount ||
	    LPART(w).itemCount <= FIRST_ITEM(w))
		return;

	if (where == ITEM_ON_TOP)
	{
		item = LPART(w).Mom ? FIRST_ITEM(w) : TOP_VIZ_ITEM(w) + 1;
	}
	else
	{
		if (LPART(w).Mom ||
		    (item = BOT_VIZ_ITEM(w) + 1) > LPART(w).itemCount)
			item = LPART(w).itemCount;
	}

	if (LPART(w).CurrentKbdItem != item)
	{
		int		old_kbd_item = LPART(w).CurrentKbdItem;
		Boolean		list_drawn;

		LPART(w).CurrentKbdItem = item - 1;

		if (LPART(w).Mom) 
			list_drawn = SetVizPos(w, item, where);
		else
			list_drawn = False;

		if (!list_drawn)
		{
			DrawHighlight((XmListWidget)w, old_kbd_item, False);
			DrawHighlight((XmListWidget)w, item - 1, True);
		}
	}
	
	if (!LPART(w).AddMode)
		XmListSelectPos(w, item, True);

	if (where == ITEM_ON_TOP)
		LPART(w).StartItem = item - 1;
} /* end of GotoTopOrEnd */

/* common part of Kbd[Next|Prev]Page() */
static void
#ifdef _NO_PROTO
SetNewTopNKbdItem(w, new_top, new_kbd_item)
	Widget		w;
	int		new_top;
	int		new_kbd_item;
#else
SetNewTopNKbdItem(Widget w, int new_top, int new_kbd_item)
#endif
{
#define LW	(XmListWidget)w

	int	old_kbd_item;

	old_kbd_item = LPART(w).CurrentKbdItem;

		/* The old code didn't change CurrentKbdItem if vsb is NULL */
	if (LPART(w).vScrollBar)
	{
		Arg	arg[1];

		LPART(w).top_position = new_top;
		LPART(w).CurrentKbdItem = new_kbd_item;
		DrawList(LW, NULL, True);
		SetVerticalScrollbar(LW);
	}

	if (LPART(w).AutoSelect &&
	    LPART(w).SelectionPolicy == XmBROWSE_SELECT)
			HandleNewItem(LW, new_kbd_item, old_kbd_item);
	else if (LPART(w).SelectionPolicy == XmEXTENDED_SELECT ||
		 LPART(w).SelectionPolicy == XmBROWSE_SELECT)
			HandleExtendedItem(LW, new_kbd_item);

#undef LW

} /* end of SetNewTopNKbdItem */

static void
#ifdef NO_PROTO
ValidateRepValues(w, fb1, fb2, fb3, fb4)
	Widget			w;
	unsigned char		fb1; /* fallback for SelectionPolicy */
	unsigned char		fb2; /* fallback for ScrollBarDisplayPolicy */
	unsigned char		fb3; /* fallback for SizePolicy */
	XmStringDirection	fb4; /* fallback for StrDir */
#else
ValidateRepValues(Widget w, unsigned long fb1, unsigned char fb2,
			unsigned char fb3, XmStringDirection fb4)
#endif
{
#define DIFF(f,v)		LPART(w).f != v

	if (DIFF(SelectionPolicy, fb1) &&
	    !XmRepTypeValidValue(XmRID_SELECTION_POLICY,
				 LPART(w).SelectionPolicy, w))
		LPART(w).SelectionPolicy = fb1;

	if (DIFF(ScrollBarDisplayPolicy, fb2) &&
	    !XmRepTypeValidValue(XmRID_SCROLL_BAR_DISPLAY_POLICY,
				 LPART(w).ScrollBarDisplayPolicy, w))
		LPART(w).ScrollBarDisplayPolicy = fb2;

	if (DIFF(SizePolicy, fb3) &&
	    !XmRepTypeValidValue(XmRID_LIST_SIZE_POLICY,
				 LPART(w).SizePolicy, w))
		LPART(w).SizePolicy = fb3;

	if (DIFF(StrDir, fb4) &&
	    !XmRepTypeValidValue(XmRID_STRING_DIRECTION, LPART(w).StrDir, w))
		LPART(w).StrDir = fb4;

#undef DIFF
} /* end of ValidateRepValues */

#endif /* NOVELL */


static void
#ifdef _NO_PROTO
ListSetValuesAlmost(old, new, request, reply)
	Widget              old;
	Widget              new;
	XtWidgetGeometry    *request;  /** must be 0 for ifdef NOVELL **/
	XtWidgetGeometry    *reply;    /** must be 0 for ifdef NOVELL **/
#else
ListSetValuesAlmost(
	Widget              old,
	Widget              new,
	XtWidgetGeometry    *request,  /** must be 0 for ifdef NOVELL **/
	XtWidgetGeometry    *reply)    /** must be 0 for ifdef NOVELL **/
#endif /* _NO_PROTO */
{

    int viz;
    XmListWidget   lw = (XmListWidget) new;
    externalref XmPrimitiveClassRec xmPrimitiveClassRec;

#ifdef NOVELL
        viz = SetVizCount(lw);

         if( lw->list.visibleItemCount != viz )
         {
                lw->list.visibleItemCount = viz;
                DrawList(lw,NULL,TRUE);
         }

#else
    if ( lw->list.FromSetNewSize == TRUE &&
                ( request->request_mode == 0 ||
                        ( (request->request_mode & CWHeight) &&
                                        request->height != lw->core.height) ) )
    {
        /*
         * geometry request not granted
         * so resize will not be called
         * set visible count and redraw list if necessary
         */
        viz = SetVizCount(lw);

         if( lw->list.visibleItemCount != viz )
         {
                lw->list.visibleItemCount = viz;
                DrawList(lw,NULL,TRUE);
         }
   }

#endif
	/*
         * call superclass setvalues almost method 
	 */
    if( request && reply )
	(&xmPrimitiveClassRec)->core_class.set_values_almost(old,new
								,request
								,reply
							     );
}
