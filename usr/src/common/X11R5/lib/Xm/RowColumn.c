#pragma ident	"@(#)m1.2libs:Xm/RowColumn.c	1.11"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3 -- menuHistory fix added
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

#include <Xm/RowColumnP.h>
#include <Xm/BaseClassP.h>
#include <Xm/DisplayP.h>
#include <stdio.h>
#include <ctype.h>
#include "XmI.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/ShellP.h>
#include <Xm/MenuShellP.h>
#include <Xm/LabelP.h>
#include <Xm/LabelGP.h>
#include <Xm/CascadeBP.h>
#include <Xm/CascadeBGP.h>
#include <Xm/PushBP.h>
#include <Xm/PushBGP.h>
#include <Xm/ToggleBP.h>
#include <Xm/ToggleBGP.h>
#include <Xm/SeparatorP.h>
#include <Xm/SeparatoGP.h>
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/VirtKeysP.h>
#include <Xm/Protocols.h>
#include <Xm/RCUtilsP.h>
#include <Xm/MenuUtilP.h>
#include <Xm/DrawP.h>
#include <Xm/AtomMgr.h>
#include <Xm/MwmUtil.h>
#include <Xm/VaSimpleP.h>
#include "RepTypeI.h"
#include <Xm/VendorSP.h>  /* for _XmGetDefaultDisplay and _XmAddGrab. */
#include <Xm/TearOffP.h>
#include <Xm/TearOffBP.h>
#include "GMUtilsI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#define UNDEFINED_TYPE -1
#define POST_TIME_OUT	3 /* sec */ * 1000

#define Double(x)       ((x) << 1)
#define Half(x)         ((x) >> 1)

#define IsSensitive(r)      XtIsSensitive(r)
#define IsManaged(w)        XtIsManaged(w)
#define IsNull(p)       ((p) == NULL)

#define PackTight(m)        (RC_Packing (m) == XmPACK_TIGHT)
#define PackColumn(m)       (RC_Packing (m) == XmPACK_COLUMN)
#define PackNone(m)         (RC_Packing (m) == XmPACK_NONE)

#define Asking(i)       ((i) == 0)
#define IsVertical(m)   \
       (((XmRowColumnWidget) (m))->row_column.orientation == XmVERTICAL)
#define IsHorizontal(m) \
       (((XmRowColumnWidget) (m))->row_column.orientation == XmHORIZONTAL)
#define IsAligned(m)    \
       (((XmRowColumnWidget) (m))->row_column.do_alignment)

#define IsPopup(m)     \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_POPUP)
#define IsPulldown(m)  \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_PULLDOWN)
#define IsOption(m)    \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_OPTION)
#define IsBar(m)       \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_BAR)
#define IsWorkArea(m)  \
    (((XmRowColumnWidget) (m))->row_column.type == XmWORK_AREA)
#define IsRadio(m)     \
    ((((XmRowColumnWidget) (m))->row_column.type == XmWORK_AREA) && \
         ((((XmRowColumnWidget) (m))->row_column.radio)))
#define IsHelp(m,w)     ((w) == RC_HelpPb (m))

#define WasManaged(w)  \
    (((XmRowColumnConstraintRec *) ((w)->core.constraints))-> \
     row_column.was_managed)

#define SavedMarginTop(w)  \
    (((XmRowColumnConstraintRec *) ((w)->core.constraints))-> \
     row_column.margin_top)

#define SavedMarginBottom(w)  \
    (((XmRowColumnConstraintRec *) ((w)->core.constraints))-> \
     row_column.margin_bottom)

#define SavedBaseline(w)  \
    (((XmRowColumnConstraintRec *) ((w)->core.constraints))-> \
     row_column.baseline)

#define BX(b)           ((b)->x)
#define BY(b)           ((b)->y)
#define BWidth(b)       ((b)->width)
#define BHeight(b)      ((b)->height)
#define BBorder(b)      ((b)->border_width)

#define SetPosition(b,x,y)  { BX (b) = x;  BY (b) = y; }

#define ChangeMargin(margin,new_w,sum)    \
    if ((margin) != new_w)        \
    {               \
        sum += new_w - (margin);  \
        (margin) = new_w;     \
    }

#define ChangeMarginDouble(margin,new_w,sum)    \
    if ((margin) != new_w)        \
    {               \
        sum += 2* (new_w - (margin));  \
        (margin) = new_w;     \
    }

#define ForAllChildren(m, i, q)     \
    for (i = 0, q = m->composite.children; \
     i < m->composite.num_children;     \
     i++, q++)

#define ForManagedChildren(m, i, q)  \
    for (i = 0, q = m->composite.children; \
     i < m->composite.num_children;     \
     i++, q++)          \
                    \
    if (XtIsManaged(*q))

#define AlignmentBaselineTop(m) \
(((XmRowColumnWidget) (m))->row_column.entry_vertical_alignment == XmALIGNMENT_BASELINE_TOP)
#define AlignmentBaselineBottom(m) \
(((XmRowColumnWidget) (m))->row_column.entry_vertical_alignment == XmALIGNMENT_BASELINE_BOTTOM)
#define AlignmentCenter(m) \
(((XmRowColumnWidget) (m))->row_column.entry_vertical_alignment == XmALIGNMENT_CENTER)
#define AlignmentTop(m) \
(((XmRowColumnWidget) (m))->row_column.entry_vertical_alignment == XmALIGNMENT_CONTENTS_TOP)
#define AlignmentBottom(m) \
(((XmRowColumnWidget) (m))->row_column.entry_vertical_alignment == XmALIGNMENT_CONTENTS_BOTTOM)

/* Warning Messages */

#ifdef I18N_MSG
#define BadWidthSVMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_2,\
						_XmMsgRowColumn_0000)
#define BadHeightSVMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_4,\
						_XmMsgRowColumn_0001)
#define BadPopupHelpMsg 		catgets(Xm_catd,MS_RColumn,MSG_RC_5,\
						_XmMsgRowColumn_0002)
#define BadPulldownHelpMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_6,\
						_XmMsgRowColumn_0003)
#define BadOptionHelpMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_7,\
						_XmMsgRowColumn_0004)
#define BadWorkAreaHelpMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_8,\
						_XmMsgRowColumn_0005)
#define BadTypeMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_9,\
						_XmMsgRowColumn_0006)
#define BadTypeParentMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_10,\
						_XmMsgRowColumn_0007)
#define BadTypeSVMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_11,\
						_XmMsgRowColumn_0008)
#define BadOrientationMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_12,\
						_XmMsgRowColumn_0009)
#define BadOrientationSVMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_13,\
						_XmMsgRowColumn_0010)
#define BadPackingMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_14,\
						_XmMsgRowColumn_0011)
#define BadPackingSVMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_15,\
						_XmMsgRowColumn_0012)
#define BadAlignmentMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_16,\
						_XmMsgRowColumn_0013)
#define BadAlignmentSVMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_17,\
						_XmMsgRowColumn_0014)
#define BadMenuBarHomogenousSVMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_18,\
						_XmMsgRowColumn_0015)
#define BadMenuBarEntryClassSVMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_19,\
						_XmMsgRowColumn_0016)
#define BadPulldownWhichButtonSVMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_20,\
						_XmMsgRowColumn_0017)
#define BadPulldownMenuPostSVMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_21,\
						_XmMsgRowColumn_0018)
#define BadMenuPostMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_22,\
						_XmMsgRowColumn_0019)
#define BadShadowThicknessSVMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_23,\
						_XmMsgRowColumn_0020)
#define WrongMenuChildMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_25,\
						_XmMsgRowColumn_0022)
#define WrongChildMsg			catgets(Xm_catd,MS_RColumn,MSG_RC_26,\
						_XmMsgRowColumn_0023)
#define BadOptionIsHomogeneousSVMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_27,\
						_XmMsgRowColumn_0025)
#define TearOffSharedMenupaneMsg	catgets(Xm_catd,MS_RColumn,MSG_RC_28,\
						_XmMsgRowColumn_0026)
#define BadMnemonicCharMsg		catgets(Xm_catd,MS_RColumn,MSG_RC_29,\
						_XmMsgRowColumn_0027)
#else
#define BadWidthSVMsg			_XmMsgRowColumn_0000
#define BadHeightSVMsg			_XmMsgRowColumn_0001
#define BadPopupHelpMsg 		_XmMsgRowColumn_0002
#define BadPulldownHelpMsg		_XmMsgRowColumn_0003
#define BadOptionHelpMsg		_XmMsgRowColumn_0004
#define BadWorkAreaHelpMsg		_XmMsgRowColumn_0005
#define BadTypeMsg			_XmMsgRowColumn_0006
#define BadTypeParentMsg		_XmMsgRowColumn_0007
#define BadTypeSVMsg			_XmMsgRowColumn_0008
#define BadOrientationMsg		_XmMsgRowColumn_0009
#define BadOrientationSVMsg		_XmMsgRowColumn_0010
#define BadPackingMsg			_XmMsgRowColumn_0011
#define BadPackingSVMsg			_XmMsgRowColumn_0012
#define BadAlignmentMsg			_XmMsgRowColumn_0013
#define BadAlignmentSVMsg		_XmMsgRowColumn_0014
#define BadMenuBarHomogenousSVMsg	_XmMsgRowColumn_0015
#define BadMenuBarEntryClassSVMsg	_XmMsgRowColumn_0016
#define BadPulldownWhichButtonSVMsg	_XmMsgRowColumn_0017
#define BadPulldownMenuPostSVMsg	_XmMsgRowColumn_0018
#define BadMenuPostMsg			_XmMsgRowColumn_0019
#define BadShadowThicknessSVMsg		_XmMsgRowColumn_0020
#define WrongMenuChildMsg		_XmMsgRowColumn_0022
#define WrongChildMsg			_XmMsgRowColumn_0023
#define BadOptionIsHomogeneousSVMsg	_XmMsgRowColumn_0025
#define TearOffSharedMenupaneMsg	_XmMsgRowColumn_0026
#define BadMnemonicCharMsg		_XmMsgRowColumn_0027
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Cardinal InsertPosition() ;
static void PostTimeOut() ;
static void ButtonEventHandler() ;
static void AddHandlersToPostFromWidget() ;
static void RemoveHandlersFromPostFromWidget() ;
static void AddPopupEventHandlers() ;
static void RemovePopupEventHandlers() ;
static void Destroy() ;
static void ConstraintDestroy() ;
static void fix_widget() ;
static void AddKid() ;
static void RemoveChild() ;
static void ManagedSetChanged() ;
static void Realize() ;
static Boolean do_entry_stuff() ;
static void do_size() ;
static Boolean set_values_non_popup() ;
static Boolean set_values_popup() ;
static void set_values_passive_grab() ;
static Boolean SetValues() ;
static char * GetRealKey() ;
static void MenuBarInitialize() ;
static void PreparePostFromList() ;
static void PopupInitialize() ;
static void PulldownInitialize() ;
static void OptionInitialize() ;
static void WorkAreaInitialize() ;
static void Initialize() ;
static void ConstraintInitialize() ;
static Boolean ConstraintSetValues() ;
static Widget create() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void InitializePrehook() ;
static void InitializePosthook() ;
static XmRowColumnWidget find_menu() ;
static void ButtonMenuPopDown() ;
static void _XmSwallowEventHandler() ;
static Boolean MakeChildList() ;
static void MenuArm() ;
static void MenuDisarm() ;
static void TearOffArm() ;
static void FixEventBindings() ;
static void MenuBarCleanup() ;
static void InvalidateOldFocus() ;
static void ArmAndActivate() ;
static void DoProcessMenuTree() ;
static void ProcessMenuTree() ;
static void ProcessSingleWidget() ;
static void AddToKeyboardList() ;
static void _AddToKeyboardList() ;
static void RemoveFromKeyboardList() ;
static int _XmMatchInKeyboardList() ;
static int OnPostFromList() ;
static void GetTopManager() ;
static void GrabKeyOnAssocWidgets() ;
static void UngrabKeyOnAssocWidgets() ;
static void KeyboardInputHandler() ;
static Boolean ProcessKey() ;
static Boolean CheckKey() ;
static void AddToPostFromList() ;
static void RemoveFromPostFromListOnDestroyCB();
static void RemoveFromPostFromList() ;
static Boolean InSharedMenupaneHierarchy() ;
static void SetCascadeField() ;
static Boolean _XmAllWidgetsAccessible() ;
static void BtnDownInRowColumn() ;
static void CheckUnpostAndReplay() ;
static void UpdateOptionMenuCBG() ;
static int is_in_widget_list() ;
static int in_menu() ;
static Boolean search_menu() ;
static void lota_magic() ;
static void VerifyMenuButton() ;
static Boolean UpdateMenuHistory() ;
static void SetMenuHistory() ;
static void SetOptionMenuHistory() ;
static void all_off_except() ;
static int no_toggles_on() ;
static void RadioBehaviorAndMenuHistory() ;
static char * which_callback() ;
static void ChildsActivateCallback() ;
static void EntryFired() ;
static void get_info() ;
static void adjust_last() ;
static void set_asking() ;
static void calc_help() ;
static void layout_column() ;
static void layout_vertical_tight() ;
static void layout_horizontal_tight() ;
static void layout_vertical() ;
static void layout_horizontal() ;
static void bound_entries() ;
static void find_largest_option_selection() ;
static void Layout() ;
static void OptionSizeAndLayout() ;
static void PositionMenu() ;
static Widget find_first_managed_child() ;
static void GetLastSelectToplevel() ;
static void PrepareToCascade() ;
static void LocatePulldown() ;
static void think_about_size() ;
static void PreferredSize() ;
static XtGeometryResult QueryGeometry() ;
static void CheckAndSetOptionCascade() ;
static void AdaptToSize() ;
static void Resize() ;
static void Redisplay() ;
static XtGeometryResult GeometryManager() ;
static void FixVisual() ;
static void FixCallback() ;
static void XmGetMenuKidMargins() ;
static void DoMarginAdjustment() ;
static void _XmMenuUnmap() ;
static void _XmMenuFocusOut() ;
static void _XmMenuFocusIn() ;
static void ActionNoop() ;
static void EventNoop() ;
static void _XmRC_FocusIn() ;
static void _XmRC_FocusOut() ;
static void _XmRC_Unmap() ;
static void _XmRC_Enter() ;
static void _XmRC_GadgetEscape() ;
static Boolean ShouldDispatchFocusOut() ;
static void _XmRC_GetMnemonicCharSet() ;
static void _XmRC_GetMenuAccelerator() ;
static void _XmRC_GetMenuPost() ;
static void _XmRC_GetLabelString() ;
static void _XmMenuBarGadgetSelect() ;
static void MenuStatus() ;
static void MenuProcedureEntry() ;
static void SetOrGetTextMargins() ;
static void TopOrBottomAlignment() ;
static void BaselineAlignment() ;
static void CenterAlignment() ;
static XmNavigability WidgetNavigable() ;
#else

static Cardinal InsertPosition( 
                        Widget w) ;
static void PostTimeOut( 
                        XtPointer wid,
                        XtIntervalId *id) ;
static void ButtonEventHandler( 
                        Widget w,
                        XtPointer data,
                        XEvent *event,
                        Boolean *cont) ;
static void AddHandlersToPostFromWidget( 
                        Widget popup,
                        Widget widget) ;
static void RemoveHandlersFromPostFromWidget( 
                        Widget popup,
                        Widget widget) ;
static void AddPopupEventHandlers( 
                        XmRowColumnWidget pane) ;
static void RemovePopupEventHandlers( 
                        XmRowColumnWidget pane) ;
static void Destroy( 
                        Widget w) ;
static void ConstraintDestroy( 
                        Widget w) ;
static void fix_widget( 
                        XmRowColumnWidget m,
                        Widget w) ;
static void AddKid( 
                        Widget w) ;
static void RemoveChild( 
                        Widget child) ;
static void ManagedSetChanged( 
                        Widget wid) ;
static void Realize( 
                        Widget wid,
                        XtValueMask *window_mask,
                        XSetWindowAttributes *window_attributes) ;
static Boolean do_entry_stuff( 
                        XmRowColumnWidget old,
                        XmRowColumnWidget new_w) ;
static void do_size( 
                        XmRowColumnWidget old,
                        XmRowColumnWidget new_w) ;
static Boolean set_values_non_popup( 
                        XmRowColumnWidget old,
                        XmRowColumnWidget new_w) ;
static Boolean set_values_popup( 
                        XmRowColumnWidget old,
                        XmRowColumnWidget new_w) ;
static void set_values_passive_grab( 
                        XmRowColumnWidget old,
                        XmRowColumnWidget new_w) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static char * GetRealKey( 
                        XmRowColumnWidget rc,
                        char *str) ;
static void MenuBarInitialize( 
                        XmRowColumnWidget bar) ;
static void PreparePostFromList( 
                        XmRowColumnWidget rowcol) ;
static void PopupInitialize( 
                        XmRowColumnWidget popup) ;
static void PulldownInitialize( 
                        XmRowColumnWidget pulldown) ;
static void OptionInitialize( 
                        XmRowColumnWidget option) ;
static void WorkAreaInitialize( 
                        XmRowColumnWidget work) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void ConstraintInitialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean ConstraintSetValues( 
                        Widget old,
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Widget create( 
                        Widget p,
                        char *name,
                        ArgList old_al,
                        Cardinal old_ac,
                        int type,
                        int is_radio) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass rcc) ;
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
static XmRowColumnWidget find_menu( 
                        Widget w) ;
static void ButtonMenuPopDown( 
                        Widget w,
                        XEvent *event,
                        Boolean *popped_up) ;
static void _XmSwallowEventHandler( 
                        Widget widget,
                        XtPointer client_data,
                        XEvent *event,
                        Boolean *continue_to_dispatch) ;
static Boolean MakeChildList( 
                        Widget wid,
                        Widget **childList,
                        Cardinal *numChildren) ;
static void MenuArm( 
                        Widget w) ;
static void MenuDisarm( 
                        Widget w) ;
static void TearOffArm( 
                        Widget w) ;
static void FixEventBindings( 
                        XmRowColumnWidget m,
                        Widget w) ;
static void MenuBarCleanup( 
                        XmRowColumnWidget rc) ;
static void InvalidateOldFocus( 
                        Widget oldWidget,
                        Window **poldFocus,
                        XEvent *event) ;
static void ArmAndActivate( 
                        Widget w,
                        XEvent *event,
                        String *parms,
                        Cardinal *num_parms) ;
static void DoProcessMenuTree( 
                        Widget w,
                        int mode) ;
static void ProcessMenuTree( 
                        XmRowColumnWidget w,
                        int mode) ;
static void ProcessSingleWidget( 
                        Widget w,
                        int mode) ;
static void AddToKeyboardList( 
                        Widget w,
                        char *kbdEventStr,
#if NeedWidePrototypes
                        int needGrab,
                        int isMnemonic) ;
#else
                        Boolean needGrab,
                        Boolean isMnemonic) ;
#endif /* NeedWidePrototypes */
static void _AddToKeyboardList( 
                        Widget w,
                        unsigned int eventType,
                        KeySym keysym,
                        unsigned int modifiers,
#if NeedWidePrototypes
                        int needGrab,
                        int isMnemonic) ;
#else
                        Boolean needGrab,
                        Boolean isMnemonic) ;
#endif /* NeedWidePrototypes */
static void RemoveFromKeyboardList( 
                        Widget w) ;
static int _XmMatchInKeyboardList( 
                        XmRowColumnWidget rowcol,
                        XKeyEvent *event,
                        int startIndex) ;
static int OnPostFromList( 
                        XmRowColumnWidget menu,
                        Widget widget) ;
static void GetTopManager( 
                        Widget w,
                        Widget *topManager) ;
static void GrabKeyOnAssocWidgets( 
                        XmRowColumnWidget rowcol,
#if NeedWidePrototypes
                        int detail,
#else
                        KeyCode detail,
#endif /* NeedWidePrototypes */
                        unsigned int modifiers) ;
static void UngrabKeyOnAssocWidgets( 
                        XmRowColumnWidget rowcol,
#if NeedWidePrototypes
                        int detail,
#else
                        KeyCode detail,
#endif /* NeedWidePrototypes */
                        unsigned int modifiers) ;
static void KeyboardInputHandler( 
                        Widget reportingWidget,
                        XtPointer data,
                        XEvent *event,
                        Boolean *cont) ;
static Boolean ProcessKey( 
                        XmRowColumnWidget rowcol,
                        XEvent *event) ;
static Boolean CheckKey( 
                        XmRowColumnWidget rowcol,
                        XEvent *event) ;
static void AddToPostFromList( 
                        XmRowColumnWidget m,
                        Widget widget) ;
static void RemoveFromPostFromListOnDestroyCB (
			Widget w,
			caddr_t clientData,
			caddr_t callData) ;
static void RemoveFromPostFromList( 
                        XmRowColumnWidget m,
                        Widget widget) ;
static Boolean InSharedMenupaneHierarchy( 
                        XmRowColumnWidget m) ;
static void SetCascadeField( 
                        XmRowColumnWidget m,
                        Widget cascadeBtn,
#if NeedWidePrototypes
                        int attach) ;
#else
                        Boolean attach) ;
#endif /* NeedWidePrototypes */
static Boolean _XmAllWidgetsAccessible( 
                        Widget w) ;
static void BtnDownInRowColumn( 
                        Widget rc,
                        XEvent *event,
#if NeedWidePrototypes
                        int x_root,
                        int y_root) ;
#else
                        Position x_root,
                        Position y_root) ;
#endif /* NeedWidePrototypes */
static void CheckUnpostAndReplay( 
                        Widget rc,
                        XEvent *event) ;
static void UpdateOptionMenuCBG( 
                        Widget cbg,
                        Widget memWidget) ;
static int is_in_widget_list( 
                        register XmRowColumnWidget m,
                        RectObj w) ;
static int in_menu( 
                        XmRowColumnWidget search_m,
                        XmRowColumnWidget *parent_m,
                        RectObj child,
                        Widget *w) ;
static Boolean search_menu( 
                        XmRowColumnWidget search_m,
                        XmRowColumnWidget *parent_m,
                        RectObj child,
                        Widget *w,
#if NeedWidePrototypes
                        int setHistory) ;
#else
                        Boolean setHistory) ;
#endif /* NeedWidePrototypes */
static void lota_magic( 
                        XmRowColumnWidget m,
                        RectObj child,
                        XmRowColumnWidget *parent_m,
                        Widget *w) ;
static void VerifyMenuButton( 
                        Widget w,
                        XEvent *event,
                        Boolean *valid) ;
static Boolean UpdateMenuHistory( 
                        XmRowColumnWidget menu,
                        Widget child,
#if NeedWidePrototypes
                        int updateOnMemWidgetMatch) ;
#else
                        Boolean updateOnMemWidgetMatch) ;
#endif /* NeedWidePrototypes */
static void SetMenuHistory( 
                        XmRowColumnWidget m,
                        RectObj child) ;
static void SetOptionMenuHistory( 
                        XmRowColumnWidget m,
                        RectObj child) ;
static void all_off_except( 
                        XmRowColumnWidget m,
                        Widget w) ;
static int no_toggles_on( 
                        XmRowColumnWidget m) ;
static void RadioBehaviorAndMenuHistory( 
                        XmRowColumnWidget m,
                        Widget w) ;
static char * which_callback( 
                        Widget w) ;
static void ChildsActivateCallback( 
                        XmRowColumnWidget rowcol,
                        Widget child,
                        XtPointer call_value) ;
static void EntryFired( 
                        Widget w,
                        XtPointer client_data,
                        XmAnyCallbackStruct *callback) ;
static void get_info( 
                        XmRowColumnWidget m,
                        Dimension *border,
                        Dimension *w,
                        Dimension *h,
                        int *items_per,
                        Dimension *baseline,
                        Dimension *shadow,
                        Dimension *highlight,
                        Dimension *margin_top,
                        Dimension *margin_height,
                        Dimension *text_height) ;
static void adjust_last( 
                        XmRowColumnWidget m,
                        int start_i,
#if NeedWidePrototypes
                        int w,
                        int h) ;
#else
                        Dimension w,
                        Dimension h) ;
#endif /* NeedWidePrototypes */
static void set_asking( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height,
#if NeedWidePrototypes
                        int b,
                        int max_x,
                        int max_y,
                        int x,
                        int y,
                        int w,
                        int h) ;
#else
                        Dimension b,
                        Position max_x,
                        Position max_y,
                        Position x,
                        Position y,
                        Dimension w,
                        Dimension h) ;
#endif /* NeedWidePrototypes */
static void calc_help( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height,
#if NeedWidePrototypes
                        int b,
                        int max_x,
                        int max_y,
#else
                        Dimension b,
                        Position max_x,
                        Position max_y,
#endif /* NeedWidePrototypes */
                        Position *x,
                        Position *y,
#if NeedWidePrototypes
                        int w,
                        int h) ;
#else
                        Dimension w,
                        Dimension h) ;
#endif /* NeedWidePrototypes */
static void layout_column( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void layout_vertical_tight( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void layout_horizontal_tight( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void layout_vertical( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void layout_horizontal( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void bound_entries( 
                        XmRowColumnWidget m,
                        Dimension *m_width,
                        Dimension *m_height) ;
static void find_largest_option_selection( 
                        XmRowColumnWidget submenu,
                        Dimension *c_width,
                        Dimension *c_height) ;
static void Layout( 
                        XmRowColumnWidget m,
                        Dimension *w,
                        Dimension *h) ;
static void OptionSizeAndLayout( 
                        register XmRowColumnWidget menu,
                        Dimension *width,
                        Dimension *height,
                        Widget instigator,
                        XtWidgetGeometry *request,
#if NeedWidePrototypes
                        int calcMenuDimension) ;
#else
                        Boolean calcMenuDimension) ;
#endif /* NeedWidePrototypes */
static void PositionMenu( 
                        register XmRowColumnWidget m,
                        XButtonPressedEvent *event) ;
static Widget find_first_managed_child( 
                        CompositeWidget m,
#if NeedWidePrototypes
                        int first_button) ;
#else
                        Boolean first_button) ;
#endif /* NeedWidePrototypes */
static void GetLastSelectToplevel( 
                        XmRowColumnWidget submenu) ;
static void PrepareToCascade( 
                        Widget cb,
                        XmRowColumnWidget submenu,
                        XEvent *event) ;
static void LocatePulldown( 
                        XmRowColumnWidget root,
                        XmCascadeButtonWidget p,
                        XmRowColumnWidget m,
                        XEvent *event) ;
static void think_about_size( 
                        register XmRowColumnWidget m,
                        Dimension *w,
                        Dimension *h,
                        Widget instigator,
                        XtWidgetGeometry *request) ;
static void PreferredSize( 
                        XmRowColumnWidget m,
                        Dimension *w,
                        Dimension *h) ;
static XtGeometryResult QueryGeometry( 
                        Widget wid,
                        XtWidgetGeometry *intended,
                        XtWidgetGeometry *reply) ;
static void CheckAndSetOptionCascade( 
                        XmRowColumnWidget menu) ;
static void AdaptToSize( 
                        XmRowColumnWidget m,
                        Widget instigator,
                        XtWidgetGeometry *request) ;
static void Resize( 
                        Widget wid) ;
static void Redisplay( 
                        Widget w,
                        XEvent *event,
                        Region region) ;
static XtGeometryResult GeometryManager( 
                        Widget instigator,
                        XtWidgetGeometry *desired,
                        XtWidgetGeometry *allowed) ;
static void FixVisual( 
                        XmRowColumnWidget m,
                        Widget w) ;
static void FixCallback( 
                        XmRowColumnWidget m,
                        Widget w) ;
static void XmGetMenuKidMargins( 
                        XmRowColumnWidget m,
                        Dimension *width,
                        Dimension *height,
                        Dimension *left,
                        Dimension *right,
                        Dimension *top,
                        Dimension *bottom) ;
static void DoMarginAdjustment( 
                        XmRowColumnWidget m) ;
static void _XmMenuUnmap( 
                        Widget wid,
                        XEvent *event,
                        String *param,
                        Cardinal *num_param) ;
static void _XmMenuFocusOut( 
                        Widget cb,
                        XEvent *event,
                        String *param,
                        Cardinal *num_param) ;
static void _XmMenuFocusIn( 
                        Widget wid,
                        XEvent *event,
                        String *param,
                        Cardinal *num_param) ;
static void ActionNoop( 
                        Widget wid,
                        XEvent *event,
                        String *param,
                        Cardinal *num_param) ;
static void EventNoop( 
                        Widget reportingWidget,
                        XtPointer data,
                        XEvent *event,
                        Boolean *cont) ;
static void _XmRC_FocusIn( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _XmRC_FocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _XmRC_Unmap( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _XmRC_Enter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void _XmRC_GadgetEscape( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Boolean ShouldDispatchFocusOut( 
                        Widget widget) ;
static void _XmRC_GetMnemonicCharSet( 
                        Widget wid,
                        int resource,
                        XtArgVal *value) ;
static void _XmRC_GetMenuAccelerator( 
                        Widget wid,
                        int resource,
                        XtArgVal *value) ;
static void _XmRC_GetMenuPost( 
                        Widget wid,
                        int resource,
                        XtArgVal *value) ;
static void _XmRC_GetLabelString( 
                        Widget wid,
                        int resource_offset,
                        XtArgVal *value) ;
static void _XmMenuBarGadgetSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MenuStatus( 
                        Widget wid,
                        int *menu_status) ;
static void MenuProcedureEntry( 
                        int proc,
                        Widget widget,
                        ...) ;
static void SetOrGetTextMargins( 
                        Widget wid,
#if NeedWidePrototypes
                        unsigned int op,
#else
                        unsigned char op,
#endif /* NeedWidePrototypes */
                        XmBaselineMargins *textMargins) ;
static void TopOrBottomAlignment( 
                        XmRowColumnWidget m,
#if NeedWidePrototypes
                        int h,
                        int shadow,
                        int highlight,
                        int baseline,
                        int margin_top,
                        int margin_height,
                        int text_height,
#else
                        Dimension h,
                        Dimension shadow,
                        Dimension highlight,
                        Dimension baseline,
                        Dimension margin_top,
                        Dimension margin_height,
                        Dimension text_height,
#endif /* NeedWidePrototypes */
                        Dimension *new_height,
                        int start_i,
                        int end_i) ;
static void BaselineAlignment( 
                        XmRowColumnWidget m,
#if NeedWidePrototypes
                        int h,
                        int shadow,
                        int highlight,
                        int baseline,
#else
                        Dimension h,
                        Dimension shadow,
                        Dimension highlight,
                        Dimension baseline,
#endif /* NeedWidePrototypes */
                        Dimension *new_height,
                        int start_i,
                        int end_i) ;
static void CenterAlignment( 
                        XmRowColumnWidget m,
#if NeedWidePrototypes
                        int h,
#else
                        Dimension h,
#endif /* NeedWidePrototypes */
                        int start_i,
                        int end_i) ;
static XmNavigability WidgetNavigable( 
                        Widget wid) ;
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#ifdef SCISSOR_CURSOR

#ifdef _NO_PROTO
static void ArmDisarmTearOffCB () ;
static Cursor CreateScissorCursor() ;
#else
static void ArmDisarmTearOffCB (Widget w,
                               XtPointer client_data,
                               XmAnyCallbackStruct *callback) ;
static Cursor CreateScissorCursor(Display *dpy) ;
#endif /* _NO_PROTO */

#define scissorFore_width 16
#define scissorFore_height 12
#define scissorFore_x_hot 9
#define scissorFore_y_hot 4
static unsigned char scissorFore_bits[] = {
   0x00, 0x00, 0x1c, 0x00, 0x66, 0x60, 0xc6, 0x18, 0xfc, 0x0d, 0x00, 0x07,
   0x00, 0x07, 0xfc, 0x0d, 0xc6, 0x18, 0x66, 0x60, 0x1c, 0x00, 0x00, 0x00};
#define scissorMask_width 16
#define scissorMask_height 12
static unsigned char scissorMask_bits[] = {
   0x1c, 0x00, 0x7e, 0x60, 0xff, 0xf8, 0xff, 0x7d, 0xfe, 0x1f, 0x80, 0x0f,
   0x80, 0x0f, 0xfe, 0x1f, 0xff, 0x7d, 0xff, 0xf8, 0x7e, 0x60, 0x1c, 0x00};
  
static Cursor scissor_cursor ;
#endif

/*
 * event translation tables for a menu widget, we use the parameters to
 * signal that this widget invoking the action proc is the menu, not a
 * child of the menu
 */

static XtTranslations menu_traversal_parsed;
#define menu_traversal_table 	_XmRowColumn_menu_traversal_table

static XtTranslations bar_parsed;
#define bar_table	_XmRowColumn_bar_table

static XtTranslations option_parsed;
#define option_table	_XmRowColumn_option_table

static XtTranslations menu_parsed;
#define menu_table 	_XmRowColumn_menu_table

/*
 * action binding table for row column widget
 */

static XtActionsRec action_table [] =
{
    {"Help",                _XmManagerHelp},
    {"MenuHelp",            _XmMenuHelp},
    {"MenuBtnDown",	    _XmMenuBtnDown},
    {"MenuBtnUp",	    _XmMenuBtnUp},
    {"PulldownBtnDown",     _XmMenuBtnDown},
    {"PulldownBtnUp",       _XmMenuBtnUp},
    {"PopupBtnDown",        _XmMenuBtnDown},
    {"PopupBtnUp",          _XmMenuBtnUp},
    {"MenuBarBtnDown",      _XmMenuBtnDown},
    {"MenuBarBtnUp",        _XmMenuBtnUp},
    {"WorkAreaBtnDown",     _XmGadgetArm},
    {"WorkAreaBtnUp",       _XmGadgetActivate},

    {"MenuBarGadgetSelect", _XmMenuBarGadgetSelect},

    {"FocusOut",            _XmMenuFocusOut},
    {"FocusIn",             _XmMenuFocusIn},
    {"Unmap",               _XmMenuUnmap},
    {"Noop",                ActionNoop},
    {"MenuTraverseLeft",    _XmMenuTraverseLeft},
    {"MenuTraverseRight",   _XmMenuTraverseRight},
    {"MenuTraverseUp",      _XmMenuTraverseUp},
    {"MenuTraverseDown",    _XmMenuTraverseDown},
    {"MenuEscape",	    _XmMenuEscape},

    {"MenuFocusIn",         _XmRC_FocusIn},
    {"MenuFocusOut",        _XmRC_FocusOut},
    {"MenuUnmap",           _XmRC_Unmap},
    {"MenuEnter",           _XmRC_Enter},

    {"MenuGadgetReturn",         _XmGadgetSelect},
    {"MenuGadgetEscape",         _XmRC_GadgetEscape},
    {"MenuGadgetTraverseLeft",   _XmRC_GadgetTraverseLeft},
    {"MenuGadgetTraverseRight",  _XmRC_GadgetTraverseRight},
    {"MenuGadgetTraverseUp",     _XmRC_GadgetTraverseUp},
    {"MenuGadgetTraverseDown",   _XmRC_GadgetTraverseDown}
};


/*
 * define the resourse stuff for a rowcolumn widget
 */

static Widget     resource_0_widget   = 0;
static Boolean    resource_False_boolean  = 0;
static Boolean    resource_True_boolean  = 1;
static short      resource_1_short    = 1;
static Dimension  resource_min_width   = 16;  /* 'cuz it's the size of */
static Dimension  resource_min_height  = 16;  /* a hot spot... */
static unsigned char resource_type        = XmWORK_AREA;
static unsigned char resource_alignment   = XmALIGNMENT_BEGINNING;
static unsigned char resource_vertical_alignment   = XmALIGNMENT_CENTER;
static unsigned char resource_packing     = XmNO_PACKING;
static unsigned char resource_orient      = XmNO_ORIENTATION;
static unsigned char resource_tearOffModel = XmTEAR_OFF_DISABLED;

static XtResource resources[]  =  
{
    {   XmNresizeWidth,
        XmCResizeWidth,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.resize_width),
        XmRImmediate,
        (XtPointer) TRUE
    },
    {   XmNresizeHeight,
        XmCResizeHeight,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.resize_height),
        XmRImmediate,
        (XtPointer) TRUE
    },
    {   XmNwhichButton,
        XmCWhichButton,
        XmRWhichButton,
        sizeof(unsigned int),
        XtOffsetOf( struct _XmRowColumnRec, row_column.postButton),
        XmRImmediate,
        (XtPointer) -1,
    },
    {   XmNmenuPost,
        XmCMenuPost,
        XmRString,
        sizeof(String),
        XtOffsetOf( struct _XmRowColumnRec, row_column.menuPost),
        XmRString,
        NULL,
    },
    {   XmNadjustLast,
        XmCAdjustLast,
        XmRBoolean,
        sizeof(Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.adjust_last),
        XmRImmediate,
        (XtPointer) TRUE,
    },
    {   XmNmarginWidth, 
        XmCMarginWidth, 
        XmRHorizontalDimension, 
        sizeof (Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.margin_width), 
        XmRImmediate, 
        (XtPointer) XmINVALID_DIMENSION
    },
    {   XmNmarginHeight, 
        XmCMarginHeight, 
        XmRVerticalDimension, 
        sizeof (Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.margin_height), 
        XmRImmediate, 
        (XtPointer) XmINVALID_DIMENSION
    },
    {   XmNentryCallback,
        XmCCallback, 
        XmRCallback, 
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_callback), 
        XmRCallback, 
        NULL
    },
    {   XmNmapCallback, 
        XmCCallback, 
        XmRCallback,
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmRowColumnRec, row_column.map_callback), 
        XmRCallback, 
        NULL
    },
    {   XmNunmapCallback, 
        XmCCallback, 
        XmRCallback, 
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmRowColumnRec, row_column.unmap_callback), 
        XmRCallback, 
        NULL
    },
    {   XmNorientation, 
        XmCOrientation, 
        XmROrientation, 
        sizeof(unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.orientation), 
        XmROrientation, 
        (XtPointer) &resource_orient
    },
    {   XmNspacing, 
        XmCSpacing, 
        XmRHorizontalDimension, 
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.spacing), 
        XmRImmediate, 
        (XtPointer) XmINVALID_DIMENSION
    },
    {   XmNentryBorder,         /* border width of all the */
        XmCEntryBorder,         /* entries, always uniform */
        XmRHorizontalDimension, 
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_border), 
        XmRImmediate, 
	(XtPointer) 0
    },
    {   XmNisAligned,           /* T/F, do all entrys have */
        XmCIsAligned,           /* same alignment */
        XmRBoolean, 
        sizeof(Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.do_alignment),
        XmRBoolean, 
        (XtPointer) &resource_True_boolean
    },
    {   XmNentryAlignment,          /* how entries are to be */
        XmCAlignment,               /* aligned */
        XmRAlignment, 
        sizeof(unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_alignment),
        XmRAlignment, 
        (XtPointer) &resource_alignment
    },
    {   XmNadjustMargin,            /* should all entries have */
        XmCAdjustMargin,            /* the same label margins */
        XmRBoolean, 
        sizeof(Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.adjust_margin),
        XmRBoolean, 
        (XtPointer) &resource_True_boolean
    },
    {   XmNpacking,         /* how to pack menu entries */
        XmCPacking,         /* Tight, Column, None */
        XmRPacking,
        sizeof (unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.packing),
        XmRPacking,
        (XtPointer) &resource_packing
    },
    {   XmNnumColumns,          /* if packing columnar then */
        XmCNumColumns,          /* this is how many */
        XmRShort,
        sizeof (short),
        XtOffsetOf( struct _XmRowColumnRec, row_column.num_columns),
        XmRShort, 
        (XtPointer) &resource_1_short
    },
    {   XmNradioBehavior,           /* should the menu enforce */
        XmCRadioBehavior,           /* toggle button exclusivity, */
        XmRBoolean,             /* ie, radio buttons */
        sizeof (Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.radio),
        XmRBoolean, 
        (XtPointer) &resource_False_boolean
    },
    {   XmNradioAlwaysOne,          /* should there always be one */
        XmCRadioAlwaysOne,          /* radio button on. */
        XmRBoolean,
        sizeof (Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.radio_one),
        XmRBoolean, 
        (XtPointer) &resource_True_boolean
    },
    {   XmNisHomogeneous,           /* should we enforce the */
        XmCIsHomogeneous,           /* rule that only one type of */
        XmRBoolean,             /* entry is allow in the menu */
        sizeof (Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.homogeneous),
        XmRBoolean, 
        (XtPointer) &resource_False_boolean
    },
    {   XmNentryClass,              /* if enforcing homogeneous */
        XmCEntryClass,              /* menu, this tells the class */
        XmRWidgetClass,
        sizeof (WidgetClass),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_class),
        XmRWidgetClass, 
        (XtPointer) NULL
    },
    {   XmNrowColumnType,       /* warning - non-standard resource */
        XmCRowColumnType, 
        XmRRowColumnType, 
        sizeof(unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.type), 
        XmRRowColumnType, 
        (XtPointer) &resource_type
    },
    {   XmNmenuHelpWidget,          /* which widget is the help */
        XmCMenuWidget,              /* widget */
        XmRMenuWidget, 
        sizeof (Widget),
        XtOffsetOf( struct _XmRowColumnRec, row_column.help_pushbutton), 
        XmRMenuWidget, 
        (XtPointer) &resource_0_widget
    },
    {   XmNlabelString,               /* option menus have a label */
        XmCXmString, 
        XmRXmString, 
        sizeof(_XmString),
        XtOffsetOf( struct _XmRowColumnRec, row_column.option_label), 
        XmRXmString, 
        (XtPointer)NULL
    },
    {   XmNsubMenuId,               /* option menus have built-in */
        XmCMenuWidget,              /* submenu */
        XmRMenuWidget, 
        sizeof (Widget),
        XtOffsetOf( struct _XmRowColumnRec, row_column.option_submenu), 
        XmRMenuWidget, 
        (XtPointer) &resource_0_widget
    },
    {   XmNmenuHistory,         /* pretend a subwidget fired */
        XmCMenuWidget,              /* off, used to pre-load the */
        XmRMenuWidget,              /* option menu and popup menu */
        sizeof (Widget),            /* mouse/muscle memory */
        XtOffsetOf( struct _XmRowColumnRec, row_column.memory_subwidget), 
        XmRMenuWidget, 
        (XtPointer) &resource_0_widget
    },
    {   XmNpopupEnabled,            /* are accelerator enabled */
        XmCPopupEnabled,            /* in the popup menu? */
        XmRBoolean,
        sizeof (Boolean),
        XtOffsetOf( struct _XmRowColumnRec, row_column.popup_enabled),
        XmRBoolean, 
        (XtPointer) &resource_True_boolean
    },
    {   XmNmenuAccelerator,         /* popup menu accelerator */
        XmCAccelerators,
        XmRString,
        sizeof (char *),
        XtOffsetOf( struct _XmRowColumnRec, row_column.menu_accelerator),
        XmRString, 
        (XtPointer) ""
    },
    {   XmNmnemonic,                /* option menu mnemonic */
        XmCMnemonic,
        XmRKeySym,
        sizeof (KeySym),
        XtOffsetOf( struct _XmRowColumnRec, row_column.mnemonic),
        XmRImmediate, 
        (XtPointer) NULL
    },
    {
	XmNmnemonicCharSet,
	XmCMnemonicCharSet,
	XmRString,
	sizeof(XmStringCharSet),
	XtOffsetOf( struct _XmRowColumnRec,row_column.mnemonicCharSet),
	XmRImmediate,
	(XtPointer) XmFONTLIST_DEFAULT_TAG
    },
    {
        XmNshadowThickness,
	XmCShadowThickness,
	XmRHorizontalDimension, 
	sizeof (Dimension),
	XtOffsetOf( struct _XmRowColumnRec, manager.shadow_thickness),
	XmRImmediate,
	(XtPointer) XmINVALID_DIMENSION
    },
    {
	XmNpostFromList,
	XmCPostFromList,
	XmRWidgetList,
	sizeof (Widget *),
	XtOffsetOf( struct _XmRowColumnRec, row_column.postFromList),
	XmRWidgetList,
	(XtPointer) NULL,
    },
    {
        XmNpostFromCount,
	XmCPostFromCount,
	XmRInt,
	sizeof (int),
	XtOffsetOf( struct _XmRowColumnRec, row_column.postFromCount),
	XmRImmediate,
	(XtPointer) -1
    },
    {
	XmNnavigationType, 
	XmCNavigationType, 
	XmRNavigationType, 
	sizeof (unsigned char),
	XtOffsetOf( struct _XmManagerRec, manager.navigation_type),
	XmRImmediate, 
	(XtPointer) XmDYNAMIC_DEFAULT_TAB_GROUP,
    },
    {   XmNentryVerticalAlignment,          /* how entries are to be */
        XmCVerticalAlignment,               /* aligned */
        XmRVerticalAlignment,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_vertical_alignment),
        XmRVerticalAlignment,
        (XtPointer) &resource_vertical_alignment
    },
    {   XmNtearOffModel,
        XmCTearOffModel, 
        XmRTearOffModel, 
        sizeof(unsigned char),
        XtOffsetOf( struct _XmRowColumnRec, row_column.TearOffModel), 
        XmRTearOffModel, 
        (XtPointer) &resource_tearOffModel
    },
    {   XmNtearOffMenuActivateCallback, 
        XmCCallback, 
        XmRCallback, 
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmRowColumnRec, row_column.tear_off_activated_callback), 
        XmRCallback, 
        NULL
    },
    {   XmNtearOffMenuDeactivateCallback, 
        XmCCallback, 
        XmRCallback, 
        sizeof (XtCallbackList),
        XtOffsetOf( struct _XmRowColumnRec, row_column.tear_off_deactivated_callback), 
        XmRCallback, 
        NULL
    },
    {
	XtNinsertPosition,
	XtCInsertPosition,
	XtRFunction,
	sizeof(XtOrderProc),
        XtOffsetOf(XmRowColumnWidgetRec, composite.insert_position),
	XtRImmediate,
	(XtPointer) InsertPosition
    },
};

#define RCIndex(w)    (((XmRowColumnConstraintRec *)(w)->core.constraints)\
                       ->row_column.position_index)
  
static XtResource constraint_resources[] = {
    {
       XmNpositionIndex,
       XmCPositionIndex,
       XmRShort,
       sizeof(short),
       XtOffsetOf(XmRowColumnConstraintRec, row_column.position_index), 
       XmRImmediate,
       (XtPointer) XmLAST_POSITION
    },
};
 

static XmSyntheticResource get_resources[] = 
{
    {
        XmNmnemonicCharSet,
        sizeof(XmStringCharSet),
        XtOffsetOf( struct _XmRowColumnRec,row_column.mnemonicCharSet),
        _XmRC_GetMnemonicCharSet,
        NULL,
    },
    {
        XmNmenuAccelerator,
        sizeof(char *),
        XtOffsetOf( struct _XmRowColumnRec, row_column.menu_accelerator),
        _XmRC_GetMenuAccelerator,
        NULL,
    },
    {   
        XmNmenuPost,
        sizeof(String),
        XtOffset(XmRowColumnWidget, row_column.menuPost),
        _XmRC_GetMenuPost,
        NULL,
    },
    {   XmNlabelString,               /* option menus have a label */
        sizeof(XmString),
        XtOffsetOf( struct _XmRowColumnRec, row_column.option_label),
        _XmRC_GetLabelString,
        NULL,
    },
    {   XmNspacing,
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec,row_column.spacing),
        _XmFromHorizontalPixels,
        _XmToHorizontalPixels,
    },
    {   XmNmarginHeight,
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.margin_height),
        _XmFromVerticalPixels,
        _XmToVerticalPixels,
    },
    {   XmNmarginWidth,
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.margin_width), 
        _XmFromHorizontalPixels,
        _XmToHorizontalPixels,
    },
    {
        XmNentryBorder,
        sizeof(Dimension),
        XtOffsetOf( struct _XmRowColumnRec, row_column.entry_border), 
        _XmFromHorizontalPixels,
        _XmToHorizontalPixels,
    },
};



/*
 * static initialization of the row column widget class record, must do
 * each field
 */

static XmBaseClassExtRec       rowColumnBaseClassExtRec = {
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
    XmInheritGetSecResData,                   /* getSecResData        */
    {NULL},                                   /* fast subclass        */
    XmInheritGetValuesPrehook,                /* get_values prehook   */
    XmInheritGetValuesPosthook,               /* get_values posthook  */
    NULL,                                     /* classPartInitPrehook */
    NULL,                                     /* classPartInitPosthook*/
    NULL,                                     /* ext_resources        */
    NULL,                                     /* compiled_ext_resources*/
    0,                                        /* num_ext_resources    */
    FALSE,                                    /* use_sub_resources    */
    WidgetNavigable,                          /* widgetNavigable      */
    XmInheritFocusChange,                     /* focusChange          */
};

static XmManagerClassExtRec managerClassExtRec = {
    NULL,
    NULLQUARK,
    XmManagerClassExtVersion,
    sizeof(XmManagerClassExtRec),
    MakeChildList                                /* traversal_children */
};

externaldef(xmrowcolumnclassrec) XmRowColumnClassRec xmRowColumnClassRec = 
{
    {                   /* core class record */
        (WidgetClass)&xmManagerClassRec, /* superclass ptr */
        "XmRowColumn",                 /* class_name */
        sizeof (XmRowColumnRec),       /* size of widget instance */
        ClassInitialize,               /* class init proc */
        ClassPartInitialize,           /* class part init */
        FALSE,                         /* class is not init'ed */
        Initialize,                    /* widget init proc*/
        NULL,                          /* init_hook proc */
        Realize,                       /* widget realize proc */
        action_table,                  /* class action table */
        XtNumber (action_table),
        resources,                     /* this class's resource list */
        XtNumber (resources),          /*  "     " resource_count */
        NULLQUARK,                     /* xrm_class            */
        TRUE,                          /* don't compress motion */
        XtExposeCompressMaximal,       /* do compress exposure */
        FALSE,                         /* don't compress enter-leave */
        FALSE,                         /* no VisibilityNotify */
        Destroy,                       /* class destroy proc */
        Resize,                        /* class resize proc */
        Redisplay,                        /* class expose proc */
        SetValues,                     /* class set_value proc */
        NULL,                          /* set_value_hook proc */
        XtInheritSetValuesAlmost,      /* set_value_almost proc */
        NULL,                          /* get_values_hook */
        NULL,                          /* class accept focus proc */
        XtVersion,                     /* current version */
        NULL,                          /* callback offset list */
        NULL,                          /* translation table */
        QueryGeometry,                /* query geo proc */
        NULL,                          /* display accelerator */
        (XtPointer)&rowColumnBaseClassExtRec,	/* extension */
    },
    {                  /* composite class record */
        GeometryManager,        /* childrens geo mgr proc */
        ManagedSetChanged,     /* set changed proc */
        AddKid,                 /* add a child */
        RemoveChild,            /* remove a child */
        NULL,                    /* extension */
    },
    {                  /* constraint class record */
        constraint_resources,    /* constraint resources */
        XtNumber(constraint_resources),    /* constraint resource_count */
        sizeof(XmRowColumnConstraintRec),  /* constraint_size */
        ConstraintInitialize,    /* initialize */
        ConstraintDestroy,       /* destroy */
	ConstraintSetValues,     /* set_values */
        NULL,                    /* extension */
    },
    {                  /* manager class record */
        XtInheritTranslations,   /* translations */
        get_resources,           /* syn_resources */
        XtNumber(get_resources), /* num_syn_resources */
        NULL,                    /* syn_constraint_resources */
        0,                       /* num_syn_constraint_resources */
        XmInheritParentProcess,  /* parent_process         */
        (XtPointer)&managerClassExtRec,     /* extension */

    },
    {                  /* row column class record */
        MenuProcedureEntry, /* proc to interface with menu widgets */
        ArmAndActivate,          /* proc to arm&activate menu */
        _XmMenuTraversalHandler, /* traversal handler */
        NULL,                    /* extension */
    }
};


/*
 * now make a public symbol that points to this class record
 */

externaldef(xmrowcolumnwidgetclass) WidgetClass xmRowColumnWidgetClass = 
   (WidgetClass) &xmRowColumnClassRec;


static Cardinal 
#ifdef _NO_PROTO
InsertPosition(w)
     Widget w;
#else
InsertPosition(Widget w)
#endif /* _NO_PROTO */
{
    XmRowColumnWidget rc = (XmRowColumnWidget) XtParent(w);

    /* if a positionIndex has been specified and is a correct value,
     * use it as the new position for this child
     */
    if (RCIndex(w) != XmLAST_POSITION) {
      if ((RCIndex(w) >= 0) && (RCIndex(w) <= rc->composite.num_children)) {
          return RCIndex(w) ;
      }
    }

    /* otherwise, return the default (we'll set its value in the instance field
       and reset the others index values for syblins at the end of AddKid */
    /* Note: the tearoffcontrol doesn't go thru this function, since
       AddKid filter its case */
    return rc->composite.num_children ;
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
PostTimeOut( wid, id )
        XtPointer wid ;
        XtIntervalId *id ;
#else
PostTimeOut(
        XtPointer wid,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{  
  XmRowColumnWidget popup = (XmRowColumnWidget) wid;
  XmMenuState mst = _XmGetMenuState((Widget)wid);

  popup->row_column.popup_timeout_timer = 0;
  if (mst->RC_ButtonEventStatus.waiting_to_be_managed)
  {
     XtUngrabPointer((Widget) popup, CurrentTime);
     mst->RC_ButtonEventStatus.waiting_to_be_managed = FALSE;
     mst->RC_ButtonEventStatus.verified = FALSE;
  }
}

/*
 * ButtonEventHandler is inserted at the head of the event handlers.  We must
 * pre-verify the events that popup a menupane.  When the application manages
 * the popup menupane, MenuShell's managed_set_changed(), checks the
 * verification.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ButtonEventHandler( w, data, event, cont )
        Widget w ;
        XtPointer data ;
        XEvent *event ;
        Boolean *cont ;
#else
ButtonEventHandler(
        Widget w,
        XtPointer data,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget popup = (XmRowColumnWidget) data ;
   XButtonEvent *xbutton_event = (XButtonEvent *)event;
   XmMenuState mst = _XmGetMenuState((Widget)w);

   /*
    * if this event  has been seen before, and some other popup has
    * already marked it as verified, then we don't need to bother with it.
    */
   if ((mst->RC_ButtonEventStatus.time == xbutton_event->time) &&
       (mst->RC_ButtonEventStatus.verified == True)) return;

   mst->RC_ButtonEventStatus.time = xbutton_event->time;
   mst->RC_ButtonEventStatus.verified = _XmMatchBtnEvent( event,
      RC_PostEventType(popup), RC_PostButton(popup), RC_PostModifiers(popup));

   if (mst->RC_ButtonEventStatus.verified)
   {
      mst->RC_ButtonEventStatus.waiting_to_be_managed = TRUE;
      if (!popup->core.being_destroyed && 
	  !popup->row_column.popup_timeout_timer)
	 popup->row_column.popup_timeout_timer =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) popup), 
	       (unsigned long) POST_TIME_OUT, PostTimeOut, (XtPointer) popup);

      mst->RC_ButtonEventStatus.event = *xbutton_event;

      if (RC_TornOff(popup) && !XmIsMenuShell(XtParent(popup)))
	 _XmRestoreTearOffToMenuShell((Widget)popup, event);

      /* 
       * popups keep active widget in postFromList in cascadeBtn field -
       * moved here from PositionMenu so XmGetPostedFromWidget set up
       * BEFORE application's event handler.
       */
      RC_CascadeBtn(popup) = XtWindowToWidget(XtDisplay(popup), 
	xbutton_event->window);
   }
}


static void 
#ifdef _NO_PROTO
AddHandlersToPostFromWidget( popup, widget )
        Widget popup ;
        Widget widget ;
#else
AddHandlersToPostFromWidget(
        Widget popup,
        Widget widget )
#endif /* _NO_PROTO */
{
   Cursor cursor;
   
   cursor = _XmGetMenuCursorByScreen(XtScreen(popup));

   XtInsertEventHandler(widget, ButtonPressMask|ButtonReleaseMask,
                     False, ButtonEventHandler, (XtPointer) popup, XtListHead);

   XtAddEventHandler(widget, KeyPressMask|KeyReleaseMask,
		     False, KeyboardInputHandler, (XtPointer) popup);

   /*
    * Add an event handler on the associated widget for ButtonRelease
    * events.  This is so that a quick press/release pair does not get
    * lost if the release is processed before our pointer grab is made.
    * This will guarantee that the associated widget gets the button
    * release event; it would be discarded if the widget was not selecting
    * for button release events.
    */
   XtAddEventHandler(widget, ButtonReleaseMask,
		     False, EventNoop, NULL);

   /* 
    * Must add a passive grab, so that owner_events is set to True
    * when the button grab is activated; this is so that enter/leave
    * events get dispatched by the server to the client.
    */
   XtGrabButton (widget, RC_PostButton(popup), RC_PostModifiers(popup), 
		 TRUE, (unsigned int)ButtonReleaseMask, GrabModeSync,
		 GrabModeSync, None, cursor);
}

static void 
#ifdef _NO_PROTO
RemoveHandlersFromPostFromWidget( popup, widget )
        Widget popup ;
        Widget widget ;
#else
RemoveHandlersFromPostFromWidget(
        Widget popup,
        Widget widget )
#endif /* _NO_PROTO */
{
   XtRemoveEventHandler(widget,	ButtonPressMask|ButtonReleaseMask,
			False, ButtonEventHandler, (XtPointer) popup);

   XtRemoveEventHandler(widget,	KeyPressMask|KeyReleaseMask,
			False, KeyboardInputHandler, (XtPointer) popup);

   XtRemoveEventHandler(widget, ButtonReleaseMask,
			False, EventNoop, NULL);

   /* Remove our passive grab */
   if (!widget->core.being_destroyed)
      XtUngrabButton (widget, RC_PostButton(popup), AnyModifier); 
}


/*
 * Add the Popup Menu Event Handlers needed for posting and accelerators
 */
static void 
#ifdef _NO_PROTO
AddPopupEventHandlers( pane )
        XmRowColumnWidget pane ;
#else
AddPopupEventHandlers(
        XmRowColumnWidget pane )
#endif /* _NO_PROTO */
{
   int i;
   
   /* to myself for gadgets */
   XtAddEventHandler( (Widget) pane, KeyPressMask|KeyReleaseMask,
		     False, KeyboardInputHandler, (XtPointer) pane);

   /* Add to Our shell parent */
   XtAddEventHandler(XtParent(pane), KeyPressMask|KeyReleaseMask,
		     False, KeyboardInputHandler, pane);

   /* add to all of the widgets in the postFromList*/
   for (i=0; i < pane->row_column.postFromCount; i++)
   {
      AddHandlersToPostFromWidget ((Widget) pane, pane->row_column.postFromList[i]);
   }
}

/*
 * Remove the Popup Menu Event Handlers needed for posting and accelerators
 */
static void 
#ifdef _NO_PROTO
RemovePopupEventHandlers( pane )
        XmRowColumnWidget pane ;
#else
RemovePopupEventHandlers(
        XmRowColumnWidget pane )
#endif /* _NO_PROTO */
{
   int i;
   
   /* Remove it from us */
   XtRemoveEventHandler((Widget) pane, KeyPressMask|KeyReleaseMask,
			False, KeyboardInputHandler, (XtPointer) pane);

   /* Remove it from our shell parent */
   XtRemoveEventHandler(XtParent(pane), KeyPressMask|KeyReleaseMask,
			False, KeyboardInputHandler, (XtPointer) pane);

   /* Remove it from the postFrom widgets */
   for (i=0; i < pane->row_column.postFromCount; i++)
   {
      RemoveHandlersFromPostFromWidget ((Widget) pane,
					pane->row_column.postFromList[i]);
   }
}



/*
 * Destroy the widget, and any subwidgets there are
 */
static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = (XmRowColumnWidget) w;
   Widget topManager;
   int i;

   if (RC_TornOff(m))
   {
      if (!XmIsMenuShell(XtParent(m)))
      {
	 _XmDestroyTearOffShell(XtParent(m));

	 /* Quick switch the parent for MenuShell's DeleteChild to work.
	  * We're taking for granted that the Transient shell's deletechild
	  * has already done everything it needs to do
	  */
	 m->core.parent = RC_ParentShell(m);

	 if (XmIsMenuShell(RC_ParentShell(m)))
	    (((CompositeWidgetClass)RC_ParentShell(m)->core.widget_class)->
	       composite_class.delete_child) ((Widget)m);
      } else
         if (RC_ParentShell(m))
	 {
	    _XmDestroyTearOffShell(RC_ParentShell(m));
	 }
   }

   if (RC_TearOffControl(m))
   {
      XtDestroyWidget(RC_TearOffControl(m));
      /* Unnecessary... RC_TearOffControl(new_w) = NULL; */
   }
   
   XtRemoveAllCallbacks (w, XmNentryCallback);
   XtRemoveAllCallbacks (w, XmNmapCallback);
   XtRemoveAllCallbacks (w, XmNunmapCallback);
   XtRemoveAllCallbacks (w, XmNtearOffMenuDeactivateCallback);
   XtRemoveAllCallbacks (w, XmNtearOffMenuActivateCallback);

   /*
    * If we had added any event handlers for processing accelerators or
    * mnemonics, then we must remove them now.
    */
   if (IsPopup(m))
   {
      if (RC_PopupEnabled(m))
	  RemovePopupEventHandlers (m);

      /* If a timer is present, we're going to take for granted that the
       * pending grab belongs to this popup because no other popup can have
       * intervened during this grab.
       */
      if (m->row_column.popup_timeout_timer)
      {
	 XtRemoveTimeOut(m->row_column.popup_timeout_timer);
	 /* Ungrab and reset the ButtonEventStatus record */
	 PostTimeOut( (XtPointer) m, 
	    (XtIntervalId *)&m->row_column.popup_timeout_timer);
      }

      /* Remove attach_widget destroy callbacks to update this popup's 
       * postFromList 
       */
      for (i=0; i < m->row_column.postFromCount; i++)
      {
	 if (! m->row_column.postFromList[i]->core.being_destroyed)
	 {
	    XtRemoveCallback(m->row_column.postFromList[i], XtNdestroyCallback,
	       (XtCallbackProc)RemoveFromPostFromListOnDestroyCB, (XtPointer)m);
	 }
      }
   }
   else if (IsOption(m) || IsBar(m))
   {
      /* Remove it from the associated widget */
      GetTopManager ((Widget) m, &topManager);
      XtRemoveEventHandler(topManager, KeyPressMask|KeyReleaseMask,
			   False, KeyboardInputHandler, m);

      /* Remove it from us */
      XtRemoveEventHandler( (Widget) m, KeyPressMask|KeyReleaseMask,
			   False, KeyboardInputHandler, (XtPointer) m);

   }

   /*
    * If we're still connected to a cascade button, then we need to break
    * that link, so that the cascade button doesn't attempt to reference
    * us again, and also so that accelerators and mnemonics can be cleared up.
    */
   else
   {
      Arg args[1];

      for (i=0; i < m->row_column.postFromCount; i++)
      {
	 if (! m->row_column.postFromList[i]->core.being_destroyed)
	 {
	    XtSetArg (args[0], XmNsubMenuId, NULL);
	    XtSetValues (m->row_column.postFromList[i], args, 1);
	 }
      }
   }

   if ((IsPopup(m) && RC_PopupEnabled(m))  ||
       (IsBar(m) && RC_MenuAccelerator(m)) ||
       (IsOption(m) && RC_Mnemonic(m)))
   {
      Cardinal num_children;

      /*
       * By the time we reach here, our children are destroyed, but
       * the children's list is bogus; so we need to temporarily zero
       * out our num_children field, so DoProcessMenuTree() will not
       * attempt to process our children.
       */
      num_children = m->composite.num_children;
      m->composite.num_children = 0;
      DoProcessMenuTree((Widget) m, XmDELETE);
      m->composite.num_children = num_children;
   }

   /* free postFromList for popups and pulldowns, zero postFromCount 
    * Moved after DoProcessMenuTree() so RemoveFromKeyboardList() can
    * detect if this was a shared menupane.
    */
   if (IsPopup(m) || IsPulldown(m))
   {
      XtFree ( (char *) m->row_column.postFromList);
      m->row_column.postFromCount = 0;
   }

   /* After DoProcessMenuTree() so RemoveFromKeyboardList() works properly. */
   XtFree((char *) MGR_KeyboardList(m));

   /* Free menuPost string. */
   if (!IsPulldown(m) && RC_MenuPost(m)) XtFree(RC_MenuPost(m));

   /* Free accelerator string */
   if ((IsPopup(m) && RC_MenuAccelerator(m)) ||
       (IsBar(m) && RC_MenuAccelerator(m)))
      XtFree(RC_MenuAccelerator(m));
}


/*
 * Destroy any keyboard grabs/entries for the child
 */
static void 
#ifdef _NO_PROTO
ConstraintDestroy( w )
        Widget w ;
#else
ConstraintDestroy(
        Widget w )
#endif /* _NO_PROTO */
{
   if (!XtIsRectObj(w)) return;

   DoProcessMenuTree(w, XmDELETE);
}


/*
 * do all the stuff needed to make a subwidget of a menu work correctly
 */
static void 
#ifdef _NO_PROTO
fix_widget( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
fix_widget(
        XmRowColumnWidget m,
        Widget w )
#endif /* _NO_PROTO */
{
    /*
     * now patchup the event binding table for the subwidget so that
     * it acts the way we want it to
     */

    FixEventBindings (m, w);

    /*
     * and patch the visual aspects of the subwidget
     */

    FixVisual (m, w);

    /*
     * and patch the callback list so that we will be called whenever
     * he fires off
     */

    FixCallback (m, w);
}



/*
 * Add a child to this row column widget
 */
static void 
#ifdef _NO_PROTO
AddKid( w )
        Widget w ;
#else
AddKid(
        Widget w )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget m = (XmRowColumnWidget) XtParent(w);
    Widget *p ;
    int i ;

    /* Special case the hidden tear off control */
    if (RC_FromInit(m))
    {
	/* can't let XmLAST_POSITION in, this value is used in
	 * geometry manager to denote a special case
	 */
	RCIndex(w) = 0 ; 
	return;
    }

    if (!IsWorkArea(m) /* it's a menu */ &&
        (XtClass(w) != xmLabelGadgetClass) && (XtIsRectObj(w)) &&
        !XmIsPushButtonGadget(w) &&
        !XmIsCascadeButtonGadget(w) &&
        !XmIsToggleButtonGadget(w) &&
        !XmIsSeparatorGadget(w) &&
        (XtClass(w) != xmLabelWidgetClass) &&
        !XmIsPushButton(w) &&
        !XmIsCascadeButton(w) &&
        !XmIsToggleButton(w) &&
        !XmIsSeparator(w))
        _XmWarning( (Widget) m, WrongMenuChildMsg);

    /*
     * if the rowcolumn is homogeneous, make sure that class matches
     * the entry class.  Three exceptions are made:  1) if the entry class is
     * CascadeButton or CascadeButtonGadget, either of those classes are
     * allowed. 2) if the entry class is ToggleButton or ToggleButtonGadget,
     * either of those classes are allowed.  3) if the entry class is
     * PushButton or PushButtonGadget, either of those classes are allowed.
     */
    if (XtIsRectObj(w) && RC_IsHomogeneous(m) && 
	(RC_EntryClass(m) != XtClass(w)))
    {
       if (! ((RC_EntryClass(m) == xmCascadeButtonWidgetClass &&
	       XmIsCascadeButtonGadget(w)) ||
	      (RC_EntryClass(m) == xmCascadeButtonGadgetClass &&
	       XmIsCascadeButton(w)) ||
	      (RC_EntryClass(m) == xmToggleButtonGadgetClass &&
	       XmIsToggleButton(w)) ||
	      (RC_EntryClass(m) == xmToggleButtonWidgetClass &&
	       XmIsToggleButtonGadget(w)) ||
	      (RC_EntryClass(m) == xmPushButtonGadgetClass &&
	       XmIsPushButton(w)) ||
	      (RC_EntryClass(m) == xmPushButtonWidgetClass &&
	       XmIsPushButtonGadget(w))))
       {
	  _XmWarning( (Widget) m, WrongChildMsg);
       }
    }
       
    /*
     * use composite class insert proc to do all the dirty work
     */
    (*((XmManagerWidgetClass)xmManagerWidgetClass)->composite_class.
                 insert_child) (w);

    /*
     * now change the subwidget so that it acts the way we want it to
     */
    fix_widget (m, w);

    /*
     * re-set the correct positionIndex values for everybody if 
     * the new kid has been inserted in the list instead of put at the end 
     */
    if(    RCIndex( w) == XmLAST_POSITION    )
      {   
        RCIndex( w) = m->composite.num_children - 1 ;
      } 
    if (RCIndex(w) != (m->composite.num_children - 1))
      {   
        i = RCIndex( w) ;
        p = m->composite.children + i ;
        while(    ++i < m->composite.num_children    )
          {   
            ++p ;
            RCIndex(*p) = i ;
          } 
      } 
    /*
     * if in a torn off menu pane, then add the event handlers for tear offs
     */
    if (RC_TornOff(m) && !XmIsMenuShell(XtParent(m)))
	_XmAddTearOffEventHandlers ((Widget) m);
 
    /*
     * Fix for CR 5401 - If the RowColumn is a radiobox, check to see if the
     *               menu history has been set.  If it has, the user has
     *               forced this selection or another child has already been
     *               managed.  If not, then this is the first managed child
     *               and it should be loaded into the menu history.
     */
    if (IsRadio(m) && (RC_MemWidget(m) == (Widget) NULL))
      RC_MemWidget(m) = w;

    return;
}

/*
 * "child" is no longer a valid memory widget (menu history).  We must
 * reset any option menus who are currently referring it.  A recursive 
 * search up the widget tree is necessary in case shared menupanes are 
 * utilized.
 */
static void 
#ifdef _NO_PROTO
ResetMatchingOptionMemWidget(menu, child)
	XmRowColumnWidget menu;
	Widget child;
#else
ResetMatchingOptionMemWidget(
	XmRowColumnWidget menu,
	Widget child )
#endif /* _NO_PROTO */
{
   int i;

   if (IsPulldown(menu))
   {
      for (i=0; i < menu->row_column.postFromCount; i++)
      {
         ResetMatchingOptionMemWidget(
	    (XmRowColumnWidget) XtParent(menu->row_column.postFromList[i]),
	    child);
      }
   }
   else if (IsOption(menu) && (child == RC_MemWidget(menu)))
      {
	 Widget cb;

	 if (RC_OptionSubMenu(menu) && RC_MemWidget(RC_OptionSubMenu(menu))) {
	   RC_MemWidget(menu) = RC_MemWidget(RC_OptionSubMenu(menu));
	 }
	 else {
	 RC_MemWidget(menu) = 
	    find_first_managed_child((CompositeWidget) RC_OptionSubMenu(menu), 
	       True);
	 /* For what it's worth - nothing in SharedMenupanes */
	 if (RC_OptionSubMenu(menu))
	    RC_MemWidget(RC_OptionSubMenu(menu)) = RC_MemWidget(menu);
	 }
	 if ((cb = XmOptionButtonGadget( (Widget) menu)) != NULL)
	    UpdateOptionMenuCBG (cb, RC_MemWidget(menu));
      }
}

/*
 * delete a single widget from a parent widget
 */
static void 
#ifdef _NO_PROTO
RemoveChild( child )
        Widget child ;
#else
RemoveChild(
        Widget child )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = (XmRowColumnWidget) XtParent(child);
   Widget *p ;
   int i ;

    if (child == RC_HelpPb (m)) 
       RC_HelpPb (m) = NULL;

    else if (child == RC_MemWidget(m))
       RC_MemWidget(m) = NULL;

    /*
     * If this child is in a top level menupane, then we want to remove
     * the event handler we added for catching keyboard input.
     */
    if (XtIsWidget(child) &&
	((IsPopup(m) || IsBar(m) || IsPulldown(m)) && 
	  XmIsLabel(child) && (child->core.widget_class != xmLabelWidgetClass)))
    {
	   XtRemoveEventHandler(child, KeyPressMask|KeyReleaseMask, False,
				KeyboardInputHandler, m);
    }

    /*
     * use composite class insert proc to do all the dirty work
     */
    (*((CompositeWidgetClass)compositeWidgetClass)->composite_class.
                delete_child) (child);

    /*
     * Re-set the correct positionIndex values for everybody if
     * the new kid was not deleted from the end of the list.
     * Composite class delete_child has already decremented num_chidren!
     */
    if (RCIndex(child) != m->composite.num_children)
      ForAllChildren (m, i, p)
        {
            RCIndex(*p) = i ;
        }

    /* If this child is any option menu's menu history, reset
     * that option menu's menu history.
     */
    ResetMatchingOptionMemWidget(m, child);
}


/*
 * The set of our managed children changed, so maybe change the size of the
 * row column widget to fit them; there is no instigator of this change, and 
 * ignore any dimensional misfit of the row column widget and the entries, 
 * which is a result of our geometry mgr being nasty.  Get it laid out.
 */
static void 
#ifdef _NO_PROTO
ManagedSetChanged( wid )
        Widget wid ;
#else
ManagedSetChanged(
        Widget wid )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget m = (XmRowColumnWidget) wid ;
    Widget  *q;
    int i;
    Dimension w = 0;
    Dimension h = 0;
    Boolean any_changed = FALSE;
    
    /*
     * We have to manage the "was_managed" field of the 
     * constraint record.
     */
    
    ForAllChildren(m, i, q)
	{
	    if (WasManaged(*q) != IsManaged(*q))
		{
		    any_changed = TRUE;
		}
	    
	    WasManaged(*q) = IsManaged(*q);
	}
    
    if (RC_TearOffControl(m))
	{
	    if (WasManaged(RC_TearOffControl(m)) != IsManaged(RC_TearOffControl(m)))
		any_changed = TRUE;
	    
	    WasManaged(RC_TearOffControl(m)) = IsManaged(RC_TearOffControl(m));
	}
    
    if (!any_changed)
	{
	    /* Must have been a popup child -- we don't really care */
	    return;
	}
    
    if ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
	(PackTight(m) && IsHorizontal(m)))
	{
	    ForManagedChildren(m, i, q)
		{
		    if (XmIsLabel(*q))
			{
			    Lab_MarginTop(*q) = SavedMarginTop(*q);
			    Lab_MarginBottom(*q) = SavedMarginBottom(*q);
			}
		    else if (XmIsLabelGadget(*q))
			{
			    _XmAssignLabG_MarginTop((XmLabelGadget)*q, SavedMarginTop(*q));
			    _XmAssignLabG_MarginBottom((XmLabelGadget)*q, SavedMarginBottom(*q));
			    _XmReCacheLabG(*q);
			}
		    else if (XmIsText(*q) || XmIsTextField(*q))
			{
			    XmBaselineMargins textMargins;
			    
			    textMargins.margin_top = SavedMarginTop(*q);
			    textMargins.margin_bottom = SavedMarginBottom(*q);
			    SetOrGetTextMargins(*q, XmBASELINE_SET, &textMargins);
			    
			}
		}
	}
    
    DoMarginAdjustment (m);
    
   /*
    * find out what size we need to be with the current set of kids
    */
   PreferredSize (m, &w, &h);

   /*
    * now decide if the menu needs to change size
    */
   if ((w != XtWidth (m)) || (h != XtHeight (m)))
   {
      XtWidgetGeometry menu_desired;
      menu_desired.request_mode = 0;
      
      if (w != XtWidth (m))
      {
	 menu_desired.width =  w;
	 menu_desired.request_mode |= CWWidth;
      }
      
      if (h != XtHeight (m))
      {
	 menu_desired.height = h;
	 menu_desired.request_mode |= CWHeight;
      }

      /* use a function that always accepts Almost */
      _XmMakeGeometryRequest( (Widget) m, &menu_desired);
   }

    
    /*
     * if we get to here the row column widget has been changed and his 
     * window has been resized, so effectively we need to do a Resize.
     */
    
    AdaptToSize (m, NULL, NULL);
    
    /*	Clear shadow if necessary. */
    
    if (m->row_column.old_shadow_thickness)
	_XmClearShadowType( (Widget) m, m->row_column.old_width, 
			   m->row_column.old_height, 
			   m->row_column.old_shadow_thickness, 0);
    
    /* and redraw it for shrinking size */
    if (XtIsRealized (m) && m->manager.shadow_thickness )
	_XmDrawShadows (XtDisplay (m), XtWindow (m),
			m->manager.top_shadow_GC,
			m->manager.bottom_shadow_GC,
			0, 0, m->core.width, m->core.height,
			m->manager.shadow_thickness,
			XmSHADOW_OUT);
    
    m->row_column.old_width = m->core.width;
    m->row_column.old_height = m->core.height;
    m->row_column.old_shadow_thickness = m->manager.shadow_thickness;
    
    _XmNavigChangeManaged( (Widget) m);
}                       


/*
 * make the row column widget appear
 */
static void 
#ifdef _NO_PROTO
Realize( wid, window_mask, window_attributes )
        Widget wid ;
        XtValueMask *window_mask ;
        XSetWindowAttributes *window_attributes ;
#else
Realize(
        Widget wid,
        XtValueMask *window_mask,
        XSetWindowAttributes *window_attributes )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget m = (XmRowColumnWidget) wid ;

   if (IsOption(m))
   {
      XmRowColumnWidget sm = (XmRowColumnWidget) RC_OptionSubMenu(m);
      Dimension w=0, h=0;

      if (!IsNull(sm))
      {
	 if (RC_MemWidget(m))
	 {
	    Widget cb;

	    /* Set the Option Menu's Cascade Button */
	    if ((cb = XmOptionButtonGadget( (Widget) m)) != NULL)
	       UpdateOptionMenuCBG (cb, RC_MemWidget(m));
	 }
	 else /* if there is no memory widget, set it up */
	    ResetMatchingOptionMemWidget(m, NULL);
	 
	 /* find out what size we need to be */
	 PreferredSize (m, &w, &h);

	 /* now decide if the menu needs to change size */
	 if ((w != XtWidth (m)) || (h != XtHeight (m)))
	 {
	    XtWidgetGeometry menu_desired;
	    
	    menu_desired.request_mode = 0;
	    
	    if (w != XtWidth (m))
	    {
	       menu_desired.width = w;
	       menu_desired.request_mode |= CWWidth;
	    }

	    if (h != XtHeight (m))
	    {
	       menu_desired.height = h;
	       menu_desired.request_mode |= CWHeight;
	    }
	    /* use a function that always accepts Almost */
	    _XmMakeGeometryRequest( (Widget) m, &menu_desired);
	 }

	 AdaptToSize (m, NULL, NULL);
      }
   }

   /* fix menu window so that any button down is OwnerEvent true. */
   if (!IsWorkArea(m))
   {
      /*
       * Originally, we simply set the OwnerGrabButtonMask in our
       * event mask.  Unfortunately, if the application ever modifies
       * our translations or adds an event handler which caused the
       * intrinsics to regenerate our X event mask, this bit was
       * lost.  So .. we add a dummy event handler for this mask bit,
       * thus guaranteeing that it is always part of our event mask.
       */
      window_attributes->event_mask |= OwnerGrabButtonMask;
      XtAddEventHandler( (Widget) m, OwnerGrabButtonMask, False,
                                                              EventNoop, NULL);
   }

   /*
    * Don't propagate events for row column widgets
    * and set bit gravity to NW
    */
   (*window_mask) |= CWDontPropagate | CWBitGravity;
   window_attributes->bit_gravity = NorthWestGravity;

   window_attributes->do_not_propagate_mask = ButtonPressMask| 
       ButtonReleaseMask|KeyPressMask|KeyReleaseMask|PointerMotionMask;

   XtCreateWindow ( (Widget) m, InputOutput, CopyFromParent, *window_mask, 
		   window_attributes);

   /*
    * Keep menus which are a child of shell widgets mapped at all times.
    * Mapping is now done by the menu shell widget.
    */
   
   if (XmIsMenuShell (XtParent(m)))
       m->core.mapped_when_managed = FALSE;

   if (RC_TearOffControl(m))
   {
      if (!XtIsRealized(RC_TearOffControl(m)))
	 XtRealizeWidget(RC_TearOffControl(m));
      XtMapWidget(RC_TearOffControl(m));
   }
}


/*
 * utilities for setvalue procs
 */
static Boolean 
#ifdef _NO_PROTO
do_entry_stuff( old, new_w )
        XmRowColumnWidget old ;
        XmRowColumnWidget new_w ;
#else
do_entry_stuff(
        XmRowColumnWidget old,
        XmRowColumnWidget new_w )
#endif /* _NO_PROTO */
{
    XtWidgetGeometry desired;

    Boolean need_expose = FALSE;

    if (RC_EntryBorder (old) != RC_EntryBorder (new_w))
    {
        Widget *p;
        int i;

        desired.request_mode = CWBorderWidth;
        desired.border_width = RC_EntryBorder (new_w);

        ForAllChildren (new_w, i, p)
        {
            _XmConfigureObject( *p,(*p)->core.x,(*p)->core.y,
                (*p)->core.width, (*p)->core.height,
                desired.border_width);
        }

        need_expose = TRUE;
    }

    if ((RC_EntryAlignment (old) != RC_EntryAlignment (new_w)) && 
        (IsAligned (new_w)) &&
        (!IsOption(new_w)))
    {
        Widget *p;
        Arg al[2];
        int i;

        XtSetArg (al[0], XmNalignment, RC_EntryAlignment(new_w));

        ForAllChildren (new_w, i, p)
        {
            XtSetValues (*p, al, 1);
        }

        need_expose  = TRUE;
    }

    if ((RC_EntryVerticalAlignment (old) != RC_EntryVerticalAlignment (new_w)) &&
        (!IsOption(new_w)))
    need_expose = TRUE;
    
    return (need_expose);
}

static void 
#ifdef _NO_PROTO
do_size( old, new_w )
        XmRowColumnWidget old ;
        XmRowColumnWidget new_w ;
#else
do_size(
        XmRowColumnWidget old,
        XmRowColumnWidget new_w )
#endif /* _NO_PROTO */
{
    Widget *p;
    int i;
    int orient = RC_Orientation (old) != RC_Orientation (new_w);
    Dimension w;
    Dimension h;

    if (orient)                 /* flip all the separator */
    {                       /* widgets too */
        Arg al[2];
        int ac = 0;

        XtSetArg (al[ac], XmNorientation, 
            (IsVertical (new_w) ? XmHORIZONTAL : XmVERTICAL));

        ForAllChildren (new_w, i, p)
        {
            if (XmIsSeparator(*p) || XmIsSeparatorGadget(*p))
            XtSetValues (*p, al, 1);
        }
    }

    if ((!XtWidth(new_w))  || (XtWidth (new_w) != XtWidth(old))   ||
        (!XtHeight(new_w)) || (XtHeight (new_w) != XtHeight(old)) ||
        (orient          || 
        ((IsPopup(new_w) || IsPulldown(new_w) || IsBar(new_w)) && 
            (MGR_ShadowThickness(new_w) != MGR_ShadowThickness(old))) ||
        (RC_EntryBorder (old)           != 	   RC_EntryBorder (new_w)) || 
        (RC_MarginW     (old)           != 	   RC_MarginW     (new_w)) || 
        (RC_MarginH     (old)           != 	   RC_MarginH     (new_w)) || 
        (RC_Spacing     (old)           != 	   RC_Spacing     (new_w)) || 
        (RC_Packing     (old)           != 	   RC_Packing     (new_w)) || 
        (RC_NCol        (old)           != 	   RC_NCol        (new_w)) || 
        (RC_AdjLast     (old)           != 	   RC_AdjLast     (new_w)) || 
        (RC_AdjMargin   (old)           != 	   RC_AdjMargin   (new_w)) || 
        (RC_EntryVerticalAlignment(old) !=         RC_EntryVerticalAlignment(new_w)) ||
        (RC_HelpPb      (old)           !=	   RC_HelpPb      (new_w))))
    {
	if (RC_AdjMargin(old) != RC_AdjMargin   (new_w))
	   DoMarginAdjustment(new_w);

        if (!RC_ResizeWidth(new_w) && RC_ResizeHeight(new_w))
        {
            w = new_w->core.width;
            h = 0;
        }
        else if (RC_ResizeWidth(new_w) && !RC_ResizeHeight(new_w))
        {
            w = 0;
            h = new_w->core.height;
        }
        else if (RC_ResizeWidth(new_w) && RC_ResizeHeight(new_w))
        {
            w = 0;
            h = 0;
        }
        else
        {
            AdaptToSize(new_w,NULL,NULL);
            return;
        }

        PreferredSize (new_w, &w, &h);

        XtWidth(new_w) = w;
        XtHeight(new_w) = h;

	AdaptToSize(new_w,NULL,NULL);
    }
}


static Boolean 
#ifdef _NO_PROTO
set_values_non_popup( old, new_w )
        XmRowColumnWidget old ;
        XmRowColumnWidget new_w ;
#else
set_values_non_popup(
        XmRowColumnWidget old,
        XmRowColumnWidget new_w )
#endif /* _NO_PROTO */
{
    Widget child;
    Arg args[4];
    int n;
    Boolean need_expose = FALSE;

    /* fdt : should this only be done for a menubar?? */
    need_expose |= RC_HelpPb (old) != RC_HelpPb (new_w);

    /*
     * If we are an option menu, then we must check to see if our mnemonic
     * has changed.  If we're a menubar, then see if our accelerator has
     * changed.
     */
    if (IsOption(new_w))
    {
       if (RC_OptionSubMenu(new_w) != RC_OptionSubMenu(old)) {
	  CheckAndSetOptionCascade(new_w); /* CR 4346 */

          XtSetArg(args[0], XmNsubMenuId, RC_OptionSubMenu(new_w));
          if ((child = XmOptionButtonGadget( (Widget) new_w)) != NULL)
             XtSetValues(child, args, 1);

	  if (!RC_MemWidget(new_w) || (RC_MemWidget(old) == RC_MemWidget(new_w)))
	  {
	     if ((child = find_first_managed_child(
		  (CompositeWidget) RC_OptionSubMenu(new_w), FIRST_BUTTON))
		 != NULL)
	     {
		RC_MemWidget (new_w) = child;
	     }
	  }
       }
       if (RC_MemWidget (old) != RC_MemWidget (new_w))
       {
          SetOptionMenuHistory (new_w, (RectObj) RC_MemWidget (new_w));
	  UpdateOptionMenuCBG (XmOptionButtonGadget((Widget)new_w), 
	     RC_MemWidget (new_w));
       }

       n = 0;
       if (RC_OptionLabel(new_w) != RC_OptionLabel(old)) {
          XtSetArg(args[n], XmNlabelString, RC_OptionLabel(new_w)); n++;
          XtSetArg(args[n], XmNlabelType, XmSTRING); n++;
       }
       if (RC_MnemonicCharSet(new_w) != RC_MnemonicCharSet(old))
       {
          XtSetArg(args[n], XmNmnemonicCharSet, RC_MnemonicCharSet(new_w)); n++;
       }
       if (n && (child = XmOptionLabelGadget( (Widget) new_w)))
          XtSetValues(child, args, n);
       DoProcessMenuTree((Widget) new_w, XmREPLACE);
    }
    else if (IsBar(new_w) && (RC_MenuAccelerator(new_w) != RC_MenuAccelerator(old)))
    {
        if (RC_MenuAccelerator(new_w))
        {
            RC_MenuAccelerator(new_w) = (String)strcpy(XtMalloc( XmStrlen(
                RC_MenuAccelerator(new_w)) + 1), RC_MenuAccelerator(new_w));
        }
        DoProcessMenuTree((Widget) new_w, XmREPLACE);
        if (RC_MenuAccelerator(old))
           XtFree(RC_MenuAccelerator(old));
    }

   /*
    * Moved here in case Option Menu geometry changed
    */
    need_expose |= do_entry_stuff (old, new_w);
    do_size (old, new_w);

    return (need_expose);
}

static Boolean 
#ifdef _NO_PROTO
set_values_popup( old, new_w )
        XmRowColumnWidget old ;
        XmRowColumnWidget new_w ;
#else
set_values_popup(
        XmRowColumnWidget old,
        XmRowColumnWidget new_w )
#endif /* _NO_PROTO */
{
    int need_expose  = FALSE;
    Arg args[4];
    int n = 0;

    need_expose |= do_entry_stuff (old, new_w);
    do_size (old, new_w);

    if ((XtX (old) != XtX (new_w)) ||     /* signal the shell that it */
        (XtY (old) != XtY (new_w)))       /* had better move itself */
    {                                   /* to the menu's location */
        RC_SetWidgetMoved (new_w, TRUE);      /* and that it has to move */
        RC_SetWindowMoved (new_w, TRUE);      /* the menu's window back */
    }

    /*
     * If we are a popup menu, then we need to check the 
     * state of the popupEnabled resource; we may need to add or remove the 
     * event handler we use to catch accelerators and mnemonics.
     */
    if (IsPopup(new_w))
    {
       if (RC_PopupEnabled(new_w) != RC_PopupEnabled(old))
       {
          if (RC_PopupEnabled(new_w))
          {
	     AddPopupEventHandlers(new_w);
             DoProcessMenuTree( (Widget) new_w, XmADD);
          }
          else
          {
	     RemovePopupEventHandlers (new_w);
             DoProcessMenuTree( (Widget) new_w, XmDELETE);
          }
       }

       /* See if our accelerator has changed */
       if (RC_MenuAccelerator(new_w) != RC_MenuAccelerator(old))
       {
          if (RC_MenuAccelerator(new_w))
          {
             RC_MenuAccelerator(new_w) = (String)strcpy(XtMalloc( XmStrlen(
                 RC_MenuAccelerator(new_w)) + 1), RC_MenuAccelerator(new_w));
          }

	  if (RC_PopupEnabled(new_w))
	     DoProcessMenuTree( (Widget) new_w, XmREPLACE);

          if (RC_MenuAccelerator(old))
             XtFree(RC_MenuAccelerator(old));
       }
    }

    /* For both pulldowns and popups */
       if (RC_TearOffModel(old) != RC_TearOffModel(new_w))
       {
	  if ((RC_TearOffModel(new_w) != XmTEAR_OFF_DISABLED) &&
	      !RC_TearOffControl(new_w))
	  {
	     XmTearOffButtonWidget tw;

	     /* prevent RowColumn: AddKid() from inserting tear off button
	      * into child list.
	      */
	     RC_SetFromInit(new_w, TRUE);	
	     tw = (XmTearOffButtonWidget) XtCreateWidget("TearOffControl", xmTearOffButtonWidgetClass,
		   (Widget)new_w, args, n);
	     RC_TearOffControl(new_w) = (Widget) tw;
	     /* ensure explicit height settings are honored */
	     if (XtHeight(tw) != 1) tw->label.recompute_size = False;
#ifdef SCISSOR_CURSOR
           XtAddCallback (RC_TearOffControl(new_w), XmNarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)0);
           XtAddCallback (RC_TearOffControl(new_w), XmNdisarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)1);
#endif
	     RC_SetFromInit(new_w, FALSE);

	     /* Catching this bogus case is rediculous, but still, who knows
	      * what the app developer from DSU will do?
	      * So, like the Init method, we'll put off realize/manage until
	      * later (RowColumn.c: Realize()) if the submenu isn't realized.
	      *
	      * If the menu is tear_off_activated, then you better not manage
	      * the tear off control or it shows up in the tear off menu!
	      */
	     if (XmIsMenuShell(XtParent(new_w)))
	     {
		if (XtIsRealized(new_w))
		{
		   XtRealizeWidget(RC_TearOffControl(new_w));
		   XtManageChild(RC_TearOffControl(new_w));
		} else
		   RC_TearOffControl(new_w)->core.managed = TRUE;
	     }
	  }
	  else
	     if ((RC_TearOffModel(new_w) = XmTEAR_OFF_DISABLED) && 
		 RC_TearOffControl(new_w))
	     {
		XtDestroyWidget(RC_TearOffControl(new_w));
		RC_TearOffControl(new_w) = NULL;
	     }
       }
       if ((old->core.background_pixel != new_w->core.background_pixel) &&
           RC_TearOffControl(new_w))
       {
          XtSetArg(args[0], XmNbackground, new_w->core.background_pixel);
          XtSetValues(RC_TearOffControl(new_w), args, 1);
       }

    return (need_expose);
}

static void 
#ifdef _NO_PROTO
set_values_passive_grab( old, new_w )
        XmRowColumnWidget old ;
        XmRowColumnWidget new_w ;
#else
set_values_passive_grab(
        XmRowColumnWidget old,
        XmRowColumnWidget new_w )
#endif /* _NO_PROTO */
{
   int i;
   Cursor cursor;

   if (IsPopup(old))
   {
      /* Keep our passive grab up to date. */
      if (RC_PopupEnabled(old))
      {
         /* Remove it from the postFrom widgets */
         for (i=0; i < old->row_column.postFromCount; i++)
         {
            /* Remove our passive grab */
            if (XtIsRealized(old->row_column.postFromList[i]))
            {
               XtUngrabButton (old->row_column.postFromList[i], 
		  RC_PostButton(old), RC_PostModifiers(old));
            }
         }

         if (RC_PopupEnabled(new_w))
         {
            cursor = _XmGetMenuCursorByScreen(XtScreen(new_w));

            /* add to all of the widgets in the postFromList*/
            for (i=0; i < new_w->row_column.postFromCount; i++)
            {
               /*
                * Must add a passive grab, so that owner_events is
                * set to True when the button grab is activated
                * this is so that enter/leave
                * events get dispatched by the server to the client.
                */
	       XtGrabButton (new_w->row_column.postFromList[i], 
			     RC_PostButton(new_w), RC_PostModifiers(new_w), 
			     TRUE, (unsigned int) ButtonReleaseMask,
			     GrabModeSync, GrabModeSync, None, cursor);
            }
         }
      }
   }
}

static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget old = (XmRowColumnWidget) cw ;
        XmRowColumnWidget req = (XmRowColumnWidget) rw ;
        XmRowColumnWidget new_w = (XmRowColumnWidget) nw ;
   int i;
   int need_expose = FALSE;

   if (!XtWidth(req))
   {
      _XmWarning( (Widget) new_w, BadWidthSVMsg);
      XtWidth(new_w) = XtWidth(old);
   }

   if (!XtHeight(req))
   {
      _XmWarning( (Widget) new_w, BadHeightSVMsg);
      XtHeight(new_w) = XtHeight(old);
   }

   if (!XmRepTypeValidValue( XmRID_ORIENTATION, RC_Orientation(new_w), 
       (Widget)new_w))
   {
      RC_Orientation(new_w) = RC_Orientation(old);
   }

   if (!XmRepTypeValidValue( XmRID_PACKING, RC_Packing(new_w),
       (Widget)new_w))
   {
      RC_Packing(new_w) = RC_Packing(old);
   }

   if (RC_Type(req) != RC_Type(old))
   {
      /* Type CANNOT be changed after initialization */
      _XmWarning( (Widget) new_w, BadTypeSVMsg);
      RC_Type(new_w) = RC_Type(old);
   }

   if (!XmRepTypeValidValue( XmRID_ALIGNMENT, RC_EntryAlignment(new_w),
       (Widget)new_w))
   {
      RC_EntryAlignment(new_w) = RC_EntryAlignment(old);
   }

   if (!XmRepTypeValidValue( XmRID_VERTICAL_ALIGNMENT, RC_EntryVerticalAlignment(new_w),
       (Widget)new_w))
   {
      RC_EntryVerticalAlignment(new_w) = RC_EntryVerticalAlignment(old);
   }

   if (IsBar(new_w))
   {
      if (RC_IsHomogeneous(req) != RC_IsHomogeneous(old))
      {
	 /* can't change this for menu bars */
	 _XmWarning( (Widget) new_w, BadMenuBarHomogenousSVMsg);
	 RC_IsHomogeneous(new_w) = TRUE;
      }
      if (RC_EntryClass(req) != RC_EntryClass(old))
      {
	 /* can't change this for menu bars */
	 _XmWarning( (Widget) new_w, BadMenuBarEntryClassSVMsg);
	 RC_EntryClass(new_w) = xmCascadeButtonWidgetClass;
      }
   }

   if (RC_MenuPost(new_w) != RC_MenuPost(old))
   {
      if (IsPulldown(new_w))
      {
         /* MenuPost cannot be changed via SetValues for Pulldowns */
         _XmWarning( (Widget) new_w, BadPulldownMenuPostSVMsg);
         /* just in case WhichButton was set */
         RC_PostButton(new_w) = RC_PostButton(old);
      }
      else
      {
         if (_XmMapBtnEvent(RC_MenuPost(new_w), &RC_PostEventType(new_w),
               &RC_PostButton(new_w), &RC_PostModifiers(new_w)) == FALSE)
         {
            _XmWarning( (Widget) new_w, BadMenuPostMsg);
            /* Do Nothing - No change to postButton/Modifiers/EventType */
         }
         else
            if (RC_MenuPost(new_w))
               RC_MenuPost(new_w) = XtNewString(RC_MenuPost(new_w));
            set_values_passive_grab(old, new_w);
            if (RC_MenuPost(old)) XtFree(RC_MenuPost(old));
      }
   }
   else   /* For backwards compatibility... */
      if (RC_PostButton(new_w) != RC_PostButton(old))
      {
         if (IsPulldown(new_w))
         {
            /* WhichButton cannot be changed via SetValues for Pulldowns */
            _XmWarning( (Widget) new_w, BadPulldownWhichButtonSVMsg);
            RC_PostButton(new_w) = RC_PostButton(old);
         }
         else
         {
            RC_PostModifiers(new_w) = AnyModifier;
            RC_PostEventType(new_w) = ButtonPress;
            set_values_passive_grab(old, new_w);
         }
      }


   /*
    * Shadow thickness is forced to zero for all types except
    * pulldown, popup, and menubar
    */
   if (IsPulldown(new_w) || IsPopup(new_w) || IsBar(new_w))
   {
      if (MGR_ShadowThickness(req) != MGR_ShadowThickness(old))
	  need_expose |= TRUE;
   }

   else if (MGR_ShadowThickness(req) != MGR_ShadowThickness(old))
   {
      _XmWarning( (Widget) new_w, BadShadowThicknessSVMsg);
      MGR_ShadowThickness(new_w) = 0;
   }

/* BEGIN OSF fix pir 2429 */
   if (IsOption(new_w) &&
       (RC_IsHomogeneous(req) != RC_IsHomogeneous(old)))
   {
      _XmWarning((Widget)new_w, BadOptionIsHomogeneousSVMsg);
      RC_IsHomogeneous(new_w) = FALSE;
   }
/* END OSF fix pir 2429 */


   /* postFromList changes, popups and pulldowns only */
   if (IsPopup(new_w) || IsPulldown(new_w))
   {
      if ((new_w->row_column.postFromList != old->row_column.postFromList) ||
	  (new_w->row_column.postFromCount != old->row_column.postFromCount))
      {
	 /* use temp - postFromCount decremented in RemoveFromPostFromList() */
	 int cnt;
	 if (old->row_column.postFromList)
	 {
	    cnt = old->row_column.postFromCount;
	    for (i=0; i < cnt; i++)
	    {
	       RemoveHandlersFromPostFromWidget((Widget) new_w,
		  old->row_column.postFromList[i]);
	    }
	    XtFree( (char *) old->row_column.postFromList);
	 }

         PreparePostFromList(new_w);
      }
   }
	 
   if (IsBar (new_w) || IsWorkArea (new_w) || IsOption (new_w))
       need_expose |= set_values_non_popup (old, new_w);
   else
       need_expose |= set_values_popup (old, new_w);
   
   return (need_expose);
}

static char * 
#ifdef _NO_PROTO
GetRealKey( rc, str )
        XmRowColumnWidget rc ;
        char *str ;
#else
GetRealKey(
        XmRowColumnWidget rc,
        char *str )
#endif /* _NO_PROTO */
{
   KeySym   keysym;
   Modifiers mods;
   char tmp[128];
   char *ks;
    
   keysym = XStringToKeysym(str);
   if (keysym == NoSymbol) 
      return(NULL);
            
   _XmVirtualToActualKeysym(XtDisplay(rc), keysym, &keysym, &mods);

   if (!(ks = XKeysymToString(keysym)))
      return(NULL);

   tmp[0] = '\0';
   if (mods & ControlMask)
      strcpy(tmp, "Ctrl ");

   if (mods & ShiftMask)
      strcat(tmp, "Shift ");

   if (mods & Mod1Mask) 
      strcat(tmp, "Alt ");
            
   strcat(tmp,"<KeyUp>");
   strcat(tmp, ks);

   return(XtNewString(tmp));
}

static void 
#ifdef _NO_PROTO
MenuBarInitialize( bar )
        XmRowColumnWidget bar ;
#else
MenuBarInitialize(
        XmRowColumnWidget bar )
#endif /* _NO_PROTO */
{
    Widget topManager;

    RC_IsHomogeneous(bar) = TRUE;
    RC_EntryClass(bar) = xmCascadeButtonWidgetClass;
    bar->manager.traversal_on = False;
    bar->row_column.lastSelectToplevel = (Widget) bar;

    if (RC_PostButton(bar) == -1)
        RC_PostButton(bar) = Button1;
    

    if (RC_Packing(bar) == XmNO_PACKING)
        RC_Packing(bar) = XmPACK_TIGHT;

    if (RC_Orientation(bar) == XmNO_ORIENTATION)
        RC_Orientation(bar) = XmHORIZONTAL;

    if (RC_Spacing(bar) == XmINVALID_DIMENSION)
	RC_Spacing(bar) = 0;

    XtOverrideTranslations((Widget) bar, menu_traversal_parsed);
    
    if (RC_MenuAccelerator(bar))
    {
       if (*RC_MenuAccelerator(bar) == '\0')
       {
	  if (!(RC_MenuAccelerator(bar) = GetRealKey(bar, "osfMenuBar")))
	     RC_MenuAccelerator(bar) = XtNewString("<KeyUp>F10");
       }
       else   /* Save a copy of the accelerator string */
	  RC_MenuAccelerator(bar) = XtNewString(RC_MenuAccelerator(bar));
    }

    /*
     * Add an event handler to both us and the associated widget; we
     * need one in case we have gadget children.
     */
    GetTopManager ((Widget) bar, &topManager);
    XtAddEventHandler( (Widget) bar, KeyPressMask|KeyReleaseMask,
        False, KeyboardInputHandler, (XtPointer) bar);
    XtAddEventHandler( (Widget) topManager, KeyPressMask|KeyReleaseMask,
        False, KeyboardInputHandler, (XtPointer) bar);
    
    if (RC_MenuAccelerator(bar))
        DoProcessMenuTree( (Widget) bar, XmADD);

    if (bar->manager.navigation_type == XmDYNAMIC_DEFAULT_TAB_GROUP)
        bar->manager.navigation_type = XmSTICKY_TAB_GROUP;
}

/*
 * prepare postFromList: if its at its default state, its parent should
 * be in the list.  If a list has been specified but the count has not,
 * then set the count to 0.  This is only useful for Popup and Pulldown panes.
 */
static void 
#ifdef _NO_PROTO
PreparePostFromList( rowcol )
        XmRowColumnWidget rowcol ;
#else
PreparePostFromList(
        XmRowColumnWidget rowcol )
#endif /* _NO_PROTO */
{
   Widget * tempPtr;
   Boolean forceParent = FALSE;
   int i;
   
   if (rowcol->row_column.postFromCount < 0)
   {
      if (IsPopup(rowcol) && rowcol->row_column.postFromList == NULL)
      {
	 /* default state for popups, set to parent */
	 rowcol->row_column.postFromCount = 1;
	 forceParent = True;
      }
      else
	  /* user provided a list but no count, default count to 0 */
	  rowcol->row_column.postFromCount = 0;
   }

   /* malloc enough space for 1 more addition to the list */
   rowcol->row_column.postFromListSize = rowcol->row_column.postFromCount + 1;

   tempPtr = rowcol->row_column.postFromList;
   rowcol->row_column.postFromList = (Widget *)
       XtMalloc (rowcol->row_column.postFromListSize * sizeof(Widget));

   if (tempPtr)
   {
      /* use temp - postFromCount incremented in AddToPostFromList() */
      int cnt = rowcol->row_column.postFromCount;
      /* reset the postFromCount for correct AddToPostFromList() assignment */
      rowcol->row_column.postFromCount = 0;	

      for (i=0; i < cnt; i++)
      {
	 XmAddToPostFromList ((Widget) rowcol, tempPtr[i]);
      }
   }
   else if (forceParent)
   {
      /* no postFromList, then parent of Popup is on this list */
      rowcol->row_column.postFromList[0] = XtParent(XtParent(rowcol));
   }
}

static void 
#ifdef _NO_PROTO
PopupInitialize( popup )
        XmRowColumnWidget popup ;
#else
PopupInitialize(
        XmRowColumnWidget popup )
#endif /* _NO_PROTO */
{
   Arg args[4];
   int n = 0;

   popup->row_column.lastSelectToplevel = (Widget) popup;
   
   if (RC_PostButton(popup) == -1)
       RC_PostButton(popup) = Button3;
    
   if (RC_Packing(popup) == XmNO_PACKING)
       RC_Packing(popup) = XmPACK_TIGHT;

   if (RC_Orientation(popup) == (char) XmNO_ORIENTATION)
       RC_Orientation(popup) = XmVERTICAL;

   if (RC_HelpPb(popup) != NULL)
   {
      _XmWarning( (Widget)popup, BadPopupHelpMsg);
      RC_HelpPb(popup) = NULL;
   }
   
   if (RC_Spacing(popup) == XmINVALID_DIMENSION)
       RC_Spacing(popup) = 0;

   XtOverrideTranslations( (Widget) popup, menu_traversal_parsed);

   /* If no accelerator specified, use the default */
   if (RC_MenuAccelerator(popup))
   {
      if (*RC_MenuAccelerator(popup) == '\0')
      {
	 if (!(RC_MenuAccelerator(popup) = GetRealKey(popup, "osfMenu")))
	    RC_MenuAccelerator(popup) = XtNewString("Shift<KeyUp>F10");
      }
      else   /* Save a copy of the accelerator string */
	 RC_MenuAccelerator(popup) = XtNewString(RC_MenuAccelerator(popup));
   }
   
   PreparePostFromList(popup);
    
   /* Add event handlers to all appropriate widgets */
   if (RC_PopupEnabled(popup))
   {
      AddPopupEventHandlers (popup);

      /* Register all accelerators */
      DoProcessMenuTree( (Widget) popup, XmADD);
   }

   if (RC_TearOffModel(popup) != XmTEAR_OFF_DISABLED)
   {
      /* prevent RowColumn: AddKid() from inserting tear off button
       * into child list.
       */
      RC_SetFromInit(popup, TRUE);	
      RC_TearOffControl(popup) = 
	 XtCreateWidget("TearOffControl", xmTearOffButtonWidgetClass,
	    (Widget)popup, args, n);
#ifdef SCISSOR_CURSOR
           XtAddCallback (RC_TearOffControl(popup), XmNarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)0);
           XtAddCallback (RC_TearOffControl(popup), XmNdisarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)1);
#endif

      RC_SetFromInit(popup, FALSE);	
      /* Can't call XtManageChild() 'cause popup's not realized yet */
      RC_TearOffControl(popup)->core.managed = TRUE;
   }
   popup->row_column.popup_timeout_timer = 0;
}

static void 
#ifdef _NO_PROTO
PulldownInitialize( pulldown )
        XmRowColumnWidget pulldown ;
#else
PulldownInitialize(
        XmRowColumnWidget pulldown )
#endif /* _NO_PROTO */
{
    Arg args[4];
    int n = 0;

    pulldown->row_column.lastSelectToplevel = (Widget) NULL;

    if (RC_Packing(pulldown) == XmNO_PACKING)
        RC_Packing(pulldown) = XmPACK_TIGHT;

    if (RC_Orientation(pulldown) == (char) XmNO_ORIENTATION)
        RC_Orientation(pulldown) = XmVERTICAL;

    if (RC_HelpPb(pulldown) != NULL)
    {
        _XmWarning( (Widget)pulldown, BadPulldownHelpMsg);
        RC_HelpPb(pulldown) = NULL;
    }

    if (RC_Spacing(pulldown) == XmINVALID_DIMENSION)
        RC_Spacing(pulldown) = 0;
    
    XtOverrideTranslations((Widget) pulldown, menu_traversal_parsed);

    RC_MenuAccelerator(pulldown) = NULL;
    PreparePostFromList(pulldown);

    /* add event handler to myself for gadgets */
    XtAddEventHandler( (Widget) pulldown, KeyPressMask|KeyReleaseMask,
		      False, KeyboardInputHandler, (XtPointer) pulldown);

    if (RC_TearOffModel(pulldown) != XmTEAR_OFF_DISABLED)
    {
       /* prevent RowColumn: AddKid() from inserting separator into child
	* list.
	*/
       RC_SetFromInit(pulldown, TRUE);	
       RC_TearOffControl(pulldown) = 
	  XtCreateWidget("TearOffControl", xmTearOffButtonWidgetClass,
	     (Widget)pulldown, args, n);

#ifdef SCISSOR_CURSOR
           XtAddCallback (RC_TearOffControl(pulldown), XmNarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)0);
           XtAddCallback (RC_TearOffControl(pulldown), XmNdisarmCallback, 
                          (XtCallbackProc)ArmDisarmTearOffCB, (XtPointer)1);
#endif

       RC_SetFromInit(pulldown, FALSE);	
       /* Can't call XtManageChild() 'cause pulldown's not realized yet */
       RC_TearOffControl(pulldown)->core.managed = TRUE;
    }
}
       
static void 
#ifdef _NO_PROTO
OptionInitialize( option )
        XmRowColumnWidget option ;
#else
OptionInitialize(
        XmRowColumnWidget option )
#endif /* _NO_PROTO */
{
    int n;
    Arg args[4];
    Widget topManager;
    Widget child;
    XmString empty_string = NULL ;

    /* BEGIN OSFfix pir 2121 */
    MGR_ShadowThickness(option) = 0;
    /* END OSFfix pir 2121 */

    if (RC_HelpPb(option) != NULL)
    {
        _XmWarning( (Widget)option, BadOptionHelpMsg);
        RC_HelpPb(option) = NULL;
    }

    RC_Packing(option) = XmPACK_TIGHT;
/* BEGIN OSF fix pir 2429 */
    RC_IsHomogeneous(option) = FALSE;
    if (RC_Orientation(option) == (char) XmNO_ORIENTATION)
       RC_Orientation(option) = XmHORIZONTAL;
/* END OSF fix pir 2429 */
    option->row_column.lastSelectToplevel = (Widget) option;

    if (RC_PostButton(option) == -1)
        RC_PostButton(option) = Button1;
    
    if (RC_Spacing(option) == XmINVALID_DIMENSION)
        RC_Spacing(option) = 3;

    XtOverrideTranslations( (Widget) option, (XtTranslations) 
         ((XmManagerClassRec *)XtClass(option))->manager_class.translations);

    /* Create the label widget portion of the option menu */
    n = 0;

    /* fix for 5235 */
    if (RC_OptionLabel(option)) {
	XtSetArg(args[n], XmNlabelString, RC_OptionLabel(option)); n++;
    } else {
	/* if NULL, "OptionLabel" will be used as default label, and we
	   want an empty string */
	/* Note: since this resource "labelString" in C only in the AES, 
	   no need to add a synthetic getvalue that will return NULL instead
	   of this empty_string */
	empty_string = XmStringCreateLocalized(XmS);
	XtSetArg(args[n], XmNlabelString, empty_string); n++;
    }
	
    if (RC_MnemonicCharSet(option))
    {
	XtSetArg(args[n], XmNmnemonicCharSet, RC_MnemonicCharSet(option)); n++;
    }

    child = XmCreateLabelGadget( (Widget) option, "OptionLabel",args,n);
    XtManageChild (child);

    if (empty_string != NULL) XmStringFree(empty_string);
    /* end fix for 5235 */

    /* Create the cascade button widget portion of the option menu */
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, RC_OptionSubMenu(option)); n++;
/* BEGIN OSFfix pir 2123 */
/* END OSFfix pir 2123 */
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;

    /*
     * set recomputeSize false:  the option menu continually recalculates
     * the best size for this button.  By setting this false, we prevent
     * geometry requests every time the label is updated to the most
     * recent label.  This also allows for the user to do a setvalues
     * on the buttons width or height and it will be honored, as long as
     * it is big enough to handle the largest button in the pulldown menu.
     */
    XtSetArg(args[n], XmNrecomputeSize, FALSE); n++;
    child = XmCreateCascadeButtonGadget((Widget)option,"OptionButton",args,n);
    XtManageChild (child);

    RC_MenuAccelerator(option) = NULL;

    /* Add event handlers for catching keyboard input */
    GetTopManager ((Widget) option, &topManager);
    XtAddEventHandler( (Widget) option, KeyPressMask|KeyReleaseMask,
        False, KeyboardInputHandler, (XtPointer) option);
    XtAddEventHandler( (Widget) topManager, KeyPressMask|KeyReleaseMask,
        False, KeyboardInputHandler, (XtPointer) option);

    if (RC_Mnemonic(option))
        DoProcessMenuTree( (Widget) option, XmADD);

    if (option->manager.navigation_type == XmDYNAMIC_DEFAULT_TAB_GROUP)
       option->manager.navigation_type = XmNONE;
}

static void 
#ifdef _NO_PROTO
WorkAreaInitialize( work )
        XmRowColumnWidget work ;
#else
WorkAreaInitialize(
        XmRowColumnWidget work )
#endif /* _NO_PROTO */
{
    MGR_ShadowThickness(work) = 0;

    if (RC_PostButton(work) == -1)
        RC_PostButton(work) = Button1;
    
    if (work->row_column.radio)
    {
       if (RC_Packing(work) == XmNO_PACKING)
          RC_Packing(work) = XmPACK_COLUMN;

       if (RC_EntryClass(work) == NULL)
          RC_EntryClass(work) = xmToggleButtonGadgetClass;
    }
    else
       if (RC_Packing(work) == XmNO_PACKING)
	   RC_Packing(work) = XmPACK_TIGHT;

    if (RC_Orientation(work) == (char) XmNO_ORIENTATION)
        RC_Orientation(work) = XmVERTICAL;

    if (RC_HelpPb(work) != NULL)
    {
        _XmWarning( (Widget)work, BadWorkAreaHelpMsg);
        RC_HelpPb(work) = NULL;
    }

    if (RC_Spacing(work) == XmINVALID_DIMENSION)
        RC_Spacing(work) = 3;

    XtOverrideTranslations( (Widget) work, (XtTranslations) 
             ((XmManagerClassRec *)XtClass(work))->manager_class.translations);

    RC_MenuAccelerator(work) = NULL;
    
    if (work->manager.navigation_type == XmDYNAMIC_DEFAULT_TAB_GROUP)
        work->manager.navigation_type = XmTAB_GROUP;
}

/*
 * Initialize a row column widget
 */
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
    XmRowColumnWidget req = (XmRowColumnWidget) rw ;
    XmRowColumnWidget m = (XmRowColumnWidget) nw ;
    Boolean CallNavigInitAgain = FALSE ;

    if (!XtWidth(req))
    {
        XtWidth(m) = 16;
    }

    if (!XtHeight(req))
    {
        XtHeight(m) = 16;
    }

    if (IsPulldown(m) || IsPopup(m))
    {
	if (RC_MarginW(m) == XmINVALID_DIMENSION)
	    RC_MarginW(m) = 0;
	if (RC_MarginH(m) == XmINVALID_DIMENSION)
	    RC_MarginH(m) = 0;
    } else
    {
	if (RC_MarginW(m) == XmINVALID_DIMENSION)
	    RC_MarginW(m) = 3;
	if (RC_MarginH(m) == XmINVALID_DIMENSION)
	    RC_MarginH(m) = 3;
    }

    if ((RC_Orientation(nw) != XmNO_ORIENTATION) &&
        !XmRepTypeValidValue( XmRID_ORIENTATION, RC_Orientation(nw), 
	 (Widget)nw))
    {
       RC_Orientation(nw) = XmNO_ORIENTATION;
    }

    if ((RC_Packing(nw) != XmNO_PACKING) &&
        !XmRepTypeValidValue( XmRID_PACKING, RC_Packing(nw),
	 (Widget)nw))
    {
       RC_Packing(nw) = XmNO_PACKING;
    }

    if (!XmRepTypeValidValue( XmRID_ROW_COLUMN_TYPE, RC_Type(nw),
	(Widget)nw))
    {
       RC_Type(nw) = XmWORK_AREA;
    } 
    else
       if ((RC_Type(req) == XmMENU_POPUP) || (RC_Type(req) == XmMENU_PULLDOWN))
       {
	    if (!XmIsMenuShell(XtParent(req)) ||
		!XtParent(XtParent(req)))
	    {
		_XmWarning( (Widget)m,BadTypeParentMsg);
		RC_Type(m) = XmWORK_AREA;
	    }
       }

    if (!XmRepTypeValidValue( XmRID_ALIGNMENT, RC_EntryAlignment(nw),
	(Widget)nw))
    {
       RC_EntryAlignment(nw) = XmALIGNMENT_BEGINNING;
    }

    if (!XmRepTypeValidValue( XmRID_VERTICAL_ALIGNMENT, RC_EntryVerticalAlignment(nw),
        (Widget)nw))
    {
       RC_EntryVerticalAlignment(nw) = XmALIGNMENT_CENTER;
    }

    RC_CascadeBtn(m) = NULL;
    RC_Boxes(m) = NULL;

    m->row_column.armed = 0;
    RC_SetExpose (m, TRUE);             /* and ready to paint gadgets */
    RC_SetWidgetMoved  (m, TRUE);       /* and menu and shell are not */
    RC_SetWindowMoved  (m, TRUE);       /* in synch, positiongally */
    RC_SetArmed  (m, FALSE);             
    RC_SetPoppingDown  (m, FALSE);      /* not popping down */
    RC_PopupPosted(m) = NULL;		/* no popup submenus posted */

    RC_TearOffControl(m) = NULL;
    RC_SetFromInit(m, FALSE);
    RC_SetTornOff(m, FALSE);
    RC_SetTearOffActive(m, FALSE);
    RC_SetFromResize(m, FALSE);

    RC_popupMenuClick(m) = TRUE;

    if (m->manager.shadow_thickness == XmINVALID_DIMENSION)
	m->manager.shadow_thickness = 2;

    m->row_column.old_width = XtWidth(m);
    m->row_column.old_height = XtHeight(m);
    m->row_column.old_shadow_thickness = m->manager.shadow_thickness;

    /* Post initialization for whichButton - done before PopupInitialize
     * because RC_PostModifiers used in eventual XtGrabButton()
     */
    RC_PostModifiers(m) = AnyModifier;
    RC_PostEventType(m) = ButtonPress;

    /* allow menuPost override */
    if ((RC_MenuPost(m) != NULL) && !IsPulldown(m)) {
        if (_XmMapBtnEvent(RC_MenuPost(m), &RC_PostEventType(m),
            &RC_PostButton(m), &RC_PostModifiers(m)) == FALSE)
        {
            _XmWarning( (Widget)m,BadMenuPostMsg);
        }
	RC_MenuPost(m) = XtNewString(RC_MenuPost(m));
    }

    if (m->manager.navigation_type == XmDYNAMIC_DEFAULT_TAB_GROUP)
      { 
        /* Call _XmNavigInitialize a second time only if XmNnavigationType
         * is XmDYNAMIC_DEFAULT_TAB_GROUP, which causes the first call
         * to _XmNavigInitialize to do nothing.
         */
        CallNavigInitAgain = TRUE ;
      } 
    if (IsBar(m))
        MenuBarInitialize(m);
    else if (IsPopup(m))
        PopupInitialize(m);
    else if (IsPulldown(m))
        PulldownInitialize(m);
    else if (IsOption(m))
        OptionInitialize(m);
    else
        WorkAreaInitialize(m);

    if (m->manager.navigation_type == XmDYNAMIC_DEFAULT_TAB_GROUP)
        m->manager.navigation_type = XmTAB_GROUP;

    if(    CallNavigInitAgain    )
      {   
        _XmNavigInitialize( rw, nw, args, num_args) ;
      } 

    if (IsOption(m))
	SetOptionMenuHistory (m, (RectObj) RC_MemWidget (m));
    else
	SetMenuHistory (m, (RectObj) RC_MemWidget (m));

    if (!IsWorkArea(m) && (XmIsManager(XtParent(m))))
    {
       /* save the parent's accelerator widget.  Force it to NULL if this RC
	* is a menu so Manager.c: ConstraintInitialize() doesn't overwrite
	* menu's "accelerators".  This is ack-awwful!
	*/
       m->manager.accelerator_widget = 
	  ((XmManagerWidget)XtParent(m))->manager.accelerator_widget;
       ((XmManagerWidget)XtParent(m))->manager.accelerator_widget = NULL;
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ConstraintInitialize( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
ConstraintInitialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{

    if (!XtIsRectObj(new_w)) return;

    WasManaged(new_w) = False;

    if (XmIsLabel(new_w))
    {
      SavedMarginTop(new_w) = Lab_MarginTop(new_w);;
      SavedMarginBottom(new_w) = Lab_MarginBottom(new_w);;
    }
    else if (XmIsLabelGadget(new_w))
    {
      SavedMarginTop(new_w) = LabG_MarginTop(new_w);;
      SavedMarginBottom(new_w) = LabG_MarginBottom(new_w);;
    }
    else if (XmIsText(new_w) || XmIsTextField(new_w))
    {
      XmBaselineMargins textMargins;

      SetOrGetTextMargins(new_w, XmBASELINE_GET, &textMargins);
      SavedMarginTop(new_w) = textMargins.margin_top;
      SavedMarginBottom(new_w) = textMargins.margin_bottom;
    }
    /*
     * Restore parent of parent's accelerator widget.
     * accelerator widget should be null if parent's parent is not a manager!
     */
    if (((XmManagerWidget)XtParent(new_w))->manager.accelerator_widget)
    {
       ((XmManagerWidget)XtParent(XtParent(new_w)))->manager.accelerator_widget = 
	  ((XmManagerWidget)XtParent(new_w))->manager.accelerator_widget;
       ((XmManagerWidget)XtParent(new_w))->manager.accelerator_widget = NULL;
    }
}

static Boolean 
#ifdef _NO_PROTO
ConstraintSetValues(old, req, new_w, args, num_args)
    Widget old, req, new_w;
    ArgList args;
    Cardinal *num_args;
#else
ConstraintSetValues(
                  Widget old, 
                  Widget req, 
                  Widget new_w,
                  ArgList args,
                  Cardinal *num_args)
#endif /* _NO_PROTO */
{
    XmRowColumnWidget rc = (XmRowColumnWidget) XtParent(new_w);
    register Widget tmp;
    int i ;
    XtWidgetGeometry current ;
  
    /* RCIndex (old) is valid in [0, num_children-1] and should stay valid */
    /* Note: the tearoffcontrol should not change its index value,
             undefined behavior */

    if (!XtIsRectObj(new_w)) return(FALSE);

    if (RCIndex(old) != RCIndex(new_w)) {

      /* special public value */
      if (RCIndex(new_w) == XmLAST_POSITION)
          RCIndex(new_w) = rc->composite.num_children - 1 ;

      if ((RCIndex(new_w) < 0) || 
          (RCIndex(new_w) >= rc->composite.num_children)) {
          RCIndex(new_w) = RCIndex(old) ; /* warning ? */
      } else {
          int inc ;       /* change the configuration of the children list:
             put the requesting child at the new position and
             shift the others as needed (2 cases here) */
          tmp = rc->composite.children[RCIndex(old)] ;
          if (RCIndex(new_w) < RCIndex(old)) inc = -1 ; else inc = 1 ;
          
          for (i = RCIndex(old)  ; i != RCIndex(new_w) ; i+=inc) {
              rc->composite.children[i] = rc->composite.children[i+inc];
              RCIndex(rc->composite.children[i]) = i ;
          }
          rc->composite.children[RCIndex(new_w)] = tmp ;

          /* save the current geometry of the child */
	  current.x = XtX(new_w) ;
	  current.y = XtY(new_w) ;
	  current.width = XtWidth(new_w) ;
	  current.height = XtHeight(new_w) ;
	  current.border_width = XtBorderWidth(new_w) ;
          
          /* re-layout, move the child and possibly change the rc size */
          WasManaged(new_w) = False ; /* otherwise, changemanaged just exits */

          ManagedSetChanged((Widget) rc) ;

          /* as we have changed the position/size of this child, next
             step after this setvalues chain is the geometry manager
             request. We need to tell the geometry manager that
             this request is to be always honored. As the positionIndex field
             itself is self-computable, we can use it to track this
             case. We set it to a magic value here, and in the geometry
             manager, we'll have to reset it to its correct value by
             re-computing it - adding a field in the instance is another way
             for doing that, clever but more expensive */
         if ((current.x != XtX(new_w)) ||
	     (current.width != XtWidth(new_w)) ||
	     (current.height != XtHeight(new_w)) ||
	     (current.border_width != XtBorderWidth(new_w)))
            RCIndex(new_w) = XmLAST_POSITION ;

	  return (True);
      }
    }
    return (False);
}

/*
 * the main create section, mostly just tacks on the type to the arg
 * list
 */
static Widget 
#ifdef _NO_PROTO
create( p, name, old_al, old_ac, type, is_radio )
        Widget p ;
        char *name ;
        ArgList old_al ;
        Cardinal old_ac ;
        int type ;
        int is_radio ;
#else
create(
        Widget p,               /* parent widget */
        char *name,
        ArgList old_al,
        Cardinal old_ac,
        int type,               /* menu kind to create */
        int is_radio )          /* the radio flag */
#endif /* _NO_PROTO */
{
    Arg al[256];
    Widget m;
    int i, ac = 0;

    if (is_radio)               /* get ours in ahead of the */
    {                           /* caller's, so his override */
        XtSetArg (al[ac], XmNpacking, XmPACK_COLUMN);    ac++;
        XtSetArg (al[ac], XmNradioBehavior, is_radio); ac++;
        XtSetArg (al[ac], XmNisHomogeneous, TRUE); ac++;
        XtSetArg (al[ac], XmNentryClass, xmToggleButtonGadgetClass); ac++;
    }

    for (i=0; i<old_ac; i++) al[ac++] = old_al[i];  /* copy into our list */

    if (type != UNDEFINED_TYPE)
    {
       XtSetArg (al[ac], XmNrowColumnType,  type);
       ac++;
    }

    /*
     * decide if we need to build a popup shell widget
     */

    if ((type == XmMENU_PULLDOWN) || (type == XmMENU_POPUP))
    {
        Arg s_al[256];
        XmMenuShellWidget pop = NULL;
        Widget pw;
        int s_ac = 0;
        char b[200];

        /*
         * if this is a pulldown of a pulldown or popup then the parent
         * should really be the shell of the parent not the indicated 
         * parent, this keeps the cascade tree correct
         */

        if ((XtParent(p) != NULL) && XmIsMenuShell(XtParent (p)))
            pw = XtParent (p);
        else
            pw = p;

        /* 
         * Shared menupanes are supported for all menu types but the option
         * menu.  If this is not an option menupane, then see if a shell is
         * already present; if so, then we'll use it.
         */
        if (XmIsRowColumn(p) && (IsBar(p) || IsPopup(p) || IsPulldown(p)))
        {
            for (i = 0; i < pw->core.num_popups; i++)
            {
                if ((XmIsMenuShell(pw->core.popup_list[i])) &&
                   (((XmMenuShellWidget)pw->core.popup_list[i])->menu_shell.
                                                            private_shell) &&
                   (!(pw->core.popup_list[i])->core.being_destroyed))
                {
                    pop = (XmMenuShellWidget)pw->core.popup_list[i];
                    break;
                }
            }
        }

        /* No shell - create a new one */
        if (pop == NULL)
        {
            /* should pass in the old al */
            for (i=0; i<old_ac; i++) s_al[s_ac++] = old_al[i];

            XtSetArg (s_al[s_ac], XmNwidth,             5);     s_ac++;
            XtSetArg (s_al[s_ac], XmNheight,        5);     s_ac++;
            XtSetArg (s_al[s_ac], XmNallowShellResize, TRUE);   s_ac++;
            XtSetArg (s_al[s_ac], XtNoverrideRedirect, TRUE);   s_ac++;

            sprintf (b, "popup_%s", name);

            pop = (XmMenuShellWidget)XtCreatePopupShell(b, 
                     xmMenuShellWidgetClass, pw, s_al, s_ac);

            /* Mark the shell as having been created by us */
            pop->menu_shell.private_shell = True;
        }

        m = XtCreateWidget ( name, xmRowColumnWidgetClass, (Widget) pop, al, ac);
    }
    else
        m = XtCreateWidget (name, xmRowColumnWidgetClass, (Widget) p, al, ac);

    return (m);
}


/*
 *************************************************************************
 *
 * Public Routines                                                        
 *
 *************************************************************************
 */
void 
#ifdef _NO_PROTO
XmMenuPosition( p, event )
        Widget p ;
        XButtonPressedEvent *event ;
#else
XmMenuPosition(
        Widget p,
        XButtonPressedEvent *event )
#endif /* _NO_PROTO */
{
   PositionMenu ((XmRowColumnWidget) p, event);
}

Widget 
#ifdef _NO_PROTO
XmCreateRowColumn( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateRowColumn(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, UNDEFINED_TYPE, FALSE));
}

Widget 
#ifdef _NO_PROTO
XmCreateWorkArea( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateWorkArea(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmWORK_AREA, FALSE));
}

Widget 
#ifdef _NO_PROTO
XmCreateRadioBox( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateRadioBox(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmWORK_AREA, TRUE));
}

Widget 
#ifdef _NO_PROTO
XmCreateOptionMenu( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateOptionMenu(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmMENU_OPTION, FALSE));
}

Widget 
#ifdef _NO_PROTO
XmOptionLabelGadget( m )
        Widget m ;
#else
XmOptionLabelGadget(
        Widget m )
#endif /* _NO_PROTO */
{
   int i;
   Widget child;
   
   if (XmIsRowColumn(m) && IsOption(m))
   {
      XmRowColumnWidget rowcol = (XmRowColumnWidget) m;
      
      if (rowcol->core.being_destroyed) 
         return NULL;

      for (i = 0; i < rowcol->composite.num_children; i++)
      {
	 child = rowcol->composite.children[i];

	 if (XtClass(child) == xmLabelGadgetClass)
	     return (child);
      }
   }

   /* did not find a label gadget in the child list */
   return (NULL);
}

Widget 
#ifdef _NO_PROTO
XmOptionButtonGadget( m )
        Widget m ;
#else
XmOptionButtonGadget(
        Widget m )
#endif /* _NO_PROTO */
{
   int i;
   Widget child;
   
   if (XmIsRowColumn(m) && IsOption(m))
   {
      XmRowColumnWidget rowcol = (XmRowColumnWidget) m;
      
      if (rowcol->core.being_destroyed) 
	 return NULL;

      for (i = 0; i < rowcol->composite.num_children; i++)
      {
	 child = rowcol->composite.children[i];

	 if (XmIsCascadeButtonGadget(child))
	     return (child);
      }
   }

   /* did not find a cascadebuttongadget in the child list */
   return (NULL);
}

Widget 
#ifdef _NO_PROTO
XmCreateMenuBar( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreateMenuBar(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmMENU_BAR, FALSE));
}

Widget 
#ifdef _NO_PROTO
XmCreatePopupMenu( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreatePopupMenu(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmMENU_POPUP, FALSE));
}

Widget 
#ifdef _NO_PROTO
XmCreatePulldownMenu( p, name, al, ac )
        Widget p ;
        char *name ;
        ArgList al ;
        Cardinal ac ;
#else
XmCreatePulldownMenu(
        Widget p,
        char *name,
        ArgList al,
        Cardinal ac )
#endif /* _NO_PROTO */
{
    return (create (p, name, al, ac, XmMENU_PULLDOWN, FALSE));
}

void 
#ifdef _NO_PROTO
XmAddToPostFromList( menu_wid, widget )
        Widget menu_wid ;
        Widget widget ;
#else
XmAddToPostFromList(
        Widget menu_wid,
        Widget widget )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget menu = (XmRowColumnWidget) menu_wid ;
   Arg args[1];

   /* only continue if its a vailid widget and a popup or pulldown menu */
   if (! XmIsRowColumn(menu) ||
       ! (IsPopup(menu) || IsPulldown(menu)) ||
       ! widget)
       return;
   
   if (OnPostFromList(menu, widget) == -1)
   {
      if (IsPulldown(menu))
      {
	 XtSetArg (args[0], XmNsubMenuId, menu);
	 XtSetValues (widget, args, 1);
      }
      else 
      {
	 AddToPostFromList (menu, widget);
	 AddHandlersToPostFromWidget ((Widget) menu, widget);
	 DoProcessMenuTree ((Widget)menu, XmADD);
      }
   }
}

void 
#ifdef _NO_PROTO
XmRemoveFromPostFromList( menu_wid, widget )
        Widget menu_wid ;
        Widget widget ;
#else
XmRemoveFromPostFromList(
        Widget menu_wid,
        Widget widget )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget menu = (XmRowColumnWidget) menu_wid ;
   Arg args[1];

   /* only continue if its a vailid widget and a popup or pulldown menu */
   if (! XmIsRowColumn(menu) ||
       ! (IsPopup(menu) || IsPulldown(menu)) ||
       ! widget)
       return;
	
   if ((OnPostFromList(menu, widget)) != -1)
   {
      if (IsPulldown(menu))
      {
	 XtSetArg (args[0], XmNsubMenuId, NULL);
	 XtSetValues (widget, args, 1);
      }
      else 
      {
	 RemoveFromPostFromList (menu, widget);
	 RemoveHandlersFromPostFromWidget ((Widget) menu, widget);
      }
   }
}

/*
 * Return the widget which the menu was posted from.  
 *   - If this is in a popup, it is the widget which initiated the post 
 *     (via positioning & managing or via armAndActivate).  
 *   - If it is in a pulldown from a menubar or option menu, then the 
 *     returned widget is the menubar or option menu.
 *   - If it is a tear off popup or pulldown, the postedFromWidget was 
 *     determined at tear time and stored away.
 */
Widget 
#ifdef _NO_PROTO
XmGetPostedFromWidget( menu )
        Widget menu ;
#else
XmGetPostedFromWidget(
        Widget menu )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget toplevel;

   if (menu && XmIsRowColumn(menu))
   {
      toplevel = (XmRowColumnWidget)
	 ((XmRowColumnWidget)menu)->row_column.lastSelectToplevel;

      if (toplevel && IsPopup(toplevel))
      {
	 /* active widget is kept in cascadeBtn field for popups */
	 return (RC_CascadeBtn(toplevel));
      }
      else
	 return ((Widget)toplevel);
   }
   return (NULL);
}

Widget
#ifdef _NO_PROTO
XmGetTearOffControl( menu )
        Widget menu;
#else
XmGetTearOffControl(
        Widget menu )
#endif
{
   if (menu && XmIsRowColumn(menu))
      return(RC_TearOffControl(menu));
   else
      return(NULL);
}

/*
 *************************************************************************
 *
 * Semi-public Routines                                                        
 *
 *************************************************************************
 */
/*
 * _XmPostPopupMenu is intended for use by applications that do their own
 * event processing/dispatching.  This convenience routine makes sure that
 * the menu system has a crack at verifying the event before MenuShell's
 * managed_set_changed() routine tries to post the popup.
 */
void
#ifdef _NO_PROTO
_XmPostPopupMenu( wid, event )
        Widget wid ;
        XEvent *event;
#else
_XmPostPopupMenu(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
   Window saveWindow;
   XmMenuState mst = _XmGetMenuState((Widget)wid);

   if (!(wid && XmIsRowColumn(wid) && IsPopup(wid)))
      return;

   /* We'll still verify the incoming button event.  But for all other cases
    * we'll just take for granted that the application knows what it's doing
    * and force the menu to post.
    */
   if (event->type == ButtonPress || event->type == ButtonRelease)
   {
      ButtonEventHandler( wid, (XtPointer) wid, event, NULL); /* drand #4973 */
   }
   else
      {
	 mst->RC_ButtonEventStatus.verified = True;
	 /* This could be trouble if the event type passed in does not have
	  * a time stamp!
	  */
	 mst->RC_ButtonEventStatus.time = event->xkey.time;
	 mst->RC_ButtonEventStatus.waiting_to_be_managed = True;
	 mst->RC_ButtonEventStatus.event = *((XButtonEvent *)event);
      }
   if (mst->RC_ButtonEventStatus.verified) /* sync up Xt timestamp */
   {
      saveWindow = event->xany.window;
      event->xany.window = 0;
      XtDispatchEvent(event);
      event->xany.window = saveWindow;
   }
   XtManageChild(wid);
}

/* These next two routines are provided to change and get the way popup and
 * option menus react to a posting ButtonClick.  The default (True) is for
 * the menupane to remain posted when a button release is received within
 * XtMultiClickTime of the posting button press.  Setting popupMenuClick to
 * False will cause these types of menus to always unpost on Button Release.
 * Note that these are _Xm functions for 1.2 only and will be replaced with
 * resource access in Motif 1.3!  Be forewarned!
 */
void
#ifdef _NO_PROTO
_XmSetPopupMenuClick( wid, popupMenuClick )
        Widget wid ;
        Boolean popupMenuClick;
#else
_XmSetPopupMenuClick(
        Widget wid,
#if NeedWidePrototypes
	int popupMenuClick)
#else
	Boolean popupMenuClick)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   if (wid && XmIsRowColumn(wid))
      RC_popupMenuClick(wid) = popupMenuClick;
}

Boolean
#ifdef _NO_PROTO
_XmGetPopupMenuClick( wid )
	Widget wid;
#else
_XmGetPopupMenuClick(
        Widget wid )
#endif /* _NO_PROTO */
{
   if (wid && XmIsRowColumn(wid))
      return((Boolean)RC_popupMenuClick(wid));
   return(True);		/* If undefined, always return CUA */
}

/* When an application uses shared menupanes, enabling accelerators are a
 * tricky situation.  Before now, the application was required to set all
 * menu-items sensitive when the menu hierarchy unposted.  Then when the
 * activate event arrived, if the event was via an accelerator, internal
 * verification was done.  NOW, with tear offs, this may not be possible.
 * The application may need to leave menu-items insensitive so that the
 * tear off acts and looks as desired.  In this case, the application
 * calls this function to pass the events to accelerated insensitive menu
 * items for its own internal validation.
 */
void
#ifdef _NO_PROTO
_XmAllowAcceleratedInsensitiveUnmanagedMenuItems(wid, allowed)
	Widget wid;
	Boolean allowed;
#else
_XmAllowAcceleratedInsensitiveUnmanagedMenuItems(
	Widget wid,
#if NeedWidePrototypes
        int allowed )
#else
        Boolean allowed )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   _XmGetMenuState((Widget)wid)->
      RC_allowAcceleratedInsensitiveUnmanagedMenuItems = (Boolean)allowed;
}

/*
 * class initialization
 */
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
   /*
    * parse the various translation tables
    */

   menu_parsed   = XtParseTranslationTable (menu_table);
   bar_parsed    = XtParseTranslationTable (bar_table);
   option_parsed    = XtParseTranslationTable (option_table);
   menu_traversal_parsed = XtParseTranslationTable (menu_traversal_table);
   
   /* set up base class extension quark */
   rowColumnBaseClassExtRec.record_type = XmQmotif;

#ifdef SCISSOR_CURSOR
   scissor_cursor = CreateScissorCursor(_XmGetDefaultDisplay()) ;
#endif
 
   /* set up the menu procedure entry for button children to access */
   _XmSaveMenuProcContext( (XtPointer) MenuProcedureEntry);
}

static void 
#ifdef _NO_PROTO
ClassPartInitialize( rcc )
        WidgetClass rcc ;
#else
ClassPartInitialize(
        WidgetClass rcc )
#endif /* _NO_PROTO */
{
    _XmFastSubclassInit(rcc,XmROW_COLUMN_BIT);
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
  _XmSaveCoreClassTranslations (new_w);

  if ((RC_Type(new_w) == XmMENU_PULLDOWN) ||
      (RC_Type(new_w) == XmMENU_POPUP))
  {
      new_w->core.widget_class->core_class.tm_table = (String) menu_parsed;
  }
  else if (RC_Type(new_w) == XmMENU_OPTION)
  {
      new_w->core.widget_class->core_class.tm_table = (String) option_parsed;
  }
  else if (RC_Type(new_w) == XmMENU_BAR)
  {
      new_w->core.widget_class->core_class.tm_table = (String) bar_parsed;
  }
  else
      new_w->core.widget_class->core_class.tm_table =
	  xmManagerClassRec.core_class.tm_table;
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



/***************************************************************************
 *
 *
 * next section is action routines, these are called by the row column event
 * handler in response to events.  The handler parses its way through
 * the actions table calling these routines as directed.  It is these
 * routines which will typically decide the application should be
 * called via a callback.  This is where callback reasons are produced.
 *
 */
static XmRowColumnWidget 
#ifdef _NO_PROTO
find_menu( w )
        Widget w ;
#else
find_menu(
        Widget w )
#endif /* _NO_PROTO */
{
    if (XmIsRowColumn(w))
        return ((XmRowColumnWidget) w);        /* row column itself */
    else
        return ((XmRowColumnWidget) XtParent (w)); /* subwidget */
}



/*
 * popdown anything that should go away
 */
void
#ifdef   _NO_PROTO
_XmMenuPopDown( w, event, popped_up )
        Widget w ;
        XEvent *event ;
	Boolean *popped_up ;
#else
_XmMenuPopDown(
        Widget w,
        XEvent *event,
	Boolean *popped_up )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = find_menu(w);
   XmRowColumnWidget toplevel_menu;
   XmMenuShellWidget ms;
   Boolean posted;
   Time _time;

   _time = __XmGetDefaultTime(w, event);

   _XmGetActiveTopLevelMenu ((Widget) rc, (Widget *) &toplevel_menu);

   /* MenuShell's PopdownDone expects the leaf node submenu so that it can
    * determine if BSelect might have occured in one of its gadgets.  It
    * will popdown everything from the active top down.  Use RC_PopupPosted
    * for menubar to PopdownDone can get to a menushell.
    */
   if (IsBar(rc))
   {
      if (RC_PopupPosted(rc))
      {
	 (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	    menu_shell_class.popdownDone))((Widget) RC_PopupPosted(rc), event, 
	    NULL, NULL);
      } 
      else /* in PM mode - requires special attention */
	 {
            /* No submenus posted; must clean up ourselves */
            _XmMenuFocus( (Widget) rc, XmMENU_END, _time);
            XtUngrabPointer( (Widget) rc, _time);

            MenuBarCleanup(rc);
            _XmSetInDragMode((Widget) rc, False);
	    MenuDisarm((Widget)rc);
	 }
   }
   else
   if (!XmIsMenuShell(XtParent(rc)))	/* torn! */
   {
      /* if popupPosted pop it down.
       * else restore state to inactive as if the menu was active... in case
       * the menu was really not active, _XmMenuFocus(), MenuDisarm(), and
       * XtUngrabPointer() hopefully return w/o doing anything.
       */ 
      if (RC_PopupPosted(rc))	/* if no popupPosted, nothing to do! */	
      {
	 (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	    menu_shell_class.popdownDone))((Widget) RC_PopupPosted(rc), event, 
	    NULL, NULL);
      } 
      else
      {
         _XmMenuFocus(XtParent(rc), XmMENU_END, _time);
	 MenuDisarm ((Widget) toplevel_menu);
	 XtUngrabPointer(XtParent(rc), _time);
      }
   }
   else if (IsOption(toplevel_menu) &&
	    !XmIsRowColumn(w)
	    && (w != RC_MemWidget(rc)))
   {
      _XmMenuFocus(XtParent(rc), XmMENU_END, _time);
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	 menu_shell_class.popdownEveryone))((Widget) XtParent(rc), event, 
	 NULL, NULL);
      MenuDisarm ((Widget) toplevel_menu);
      XtUngrabPointer(XtParent(rc), _time);
   }
   else
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	 menu_shell_class.popdownDone))((Widget) rc, event, NULL, NULL);
       
   if (IsPulldown(rc))
      ms = (XmMenuShellWidget)XtParent(rc);
   else if (IsPulldown(toplevel_menu) || IsPopup(toplevel_menu))
      ms = (XmMenuShellWidget)XtParent(toplevel_menu);
   else if (IsOption(toplevel_menu))
      ms = (XmMenuShellWidget)XtParent(RC_OptionSubMenu(toplevel_menu));
   else
      ms = NULL;

   if (ms && XmIsMenuShell(ms))	/* && !torn */
   {
      if ((posted = ms->shell.popped_up) != False)
         MenuDisarm((Widget) rc);
   }
   else
      posted = False;

   if (popped_up)
     *popped_up = posted;
}

/*
 *  - popdown anything that should go away (Calls _XmMenuPopDown)
 *
 * Called whenever a button is activated so that potential tear off restores
 * on the parent menupane don't occur.
 */
static void 
#ifdef _NO_PROTO
ButtonMenuPopDown( w, event, popped_up )
        Widget w ;
        XEvent *event ;
	Boolean *popped_up ;
#else
ButtonMenuPopDown(
        Widget w,
        XEvent *event,
	Boolean *popped_up )
#endif
{
   XmRowColumnWidget rc = find_menu(w);
   XmRowColumnWidget pane;
   short depth;
   Boolean posted;

   /* - Don't restore any torn panes in the active menu hierarchy.
    *   "lastSelectToplevel" must be preserved for possible callback usage.
    */

   for (pane=rc, depth=0;
	((IsPulldown(pane) || IsPopup(pane)) && XmIsMenuShell(XtParent(pane)));
	depth++)
   {
       if ((depth+1) > _XmExcludedParentPane.pane_list_size)
       {
	  _XmExcludedParentPane.pane_list_size += sizeof(Widget) * 4;
	  _XmExcludedParentPane.pane = 
	     (Widget *) XtRealloc((char *)_XmExcludedParentPane.pane,
		   _XmExcludedParentPane.pane_list_size);
       }
       _XmExcludedParentPane.pane[depth] = (Widget)pane;

       /* Someone (mwm?) has posted a popup in an unorthodox manner.  Either
	* the posting event was not verified or the postFromWidget is not
	* a Motif widget.  Try to save the situation.
	*/
       if (RC_CascadeBtn(pane))
	  pane = (XmRowColumnWidget) XtParent(RC_CascadeBtn(pane));
       else
	  break;
   }

   _XmExcludedParentPane.num_panes = depth;

   _XmMenuPopDown((Widget)rc, event, &posted);

   /* _XmExcludedParentPane structure is used to delay the restoration to
    * preserve menu state until after PushB and ToggleB have a chance to
    * call their arm, activate, and disarm callbacks.  If the menu (e.g.
    * option menu) was posted as a result of click (remain posted), the
    * button up may be caught by the menu-item's activate callback but
    * dropped on the floor.  In this case, _XmExcludedParentPane should
    * be reset.
    */
   if (posted)
     _XmExcludedParentPane.num_panes = 0;

   if (popped_up)
     *popped_up = posted;
}

/*
 * Swallow focus and crossing events while the menubar is active.  This
 * allows traversal to work in menubar while its submenus have the "real"
 * focus.
 */
static void 
#ifdef _NO_PROTO
_XmSwallowEventHandler(widget, client_data, event, continue_to_dispatch)
	Widget widget;
	XtPointer client_data;
	XEvent *event;
	Boolean *continue_to_dispatch;
#else
_XmSwallowEventHandler(
	Widget widget, 
	XtPointer client_data, 
	XEvent *event, 
	Boolean *continue_to_dispatch)
#endif
{
  /* while the Menubar is active don't allow any focus type events to the
   * shell
   */
  switch (event->type)
  {
     case EnterNotify:
     case LeaveNotify:
     case FocusOut:
     /* Allow FocusIn so when we XmProcessTraversal(menupane), the first item
      * gets the focus and highlight.
     case FocusIn:
      */
        *continue_to_dispatch = False;
  }
}

void 
#ifdef _NO_PROTO
_XmSetSwallowEventHandler(widget, add_handler)
	Widget widget;
	Boolean add_handler;
#else
_XmSetSwallowEventHandler(
	Widget widget, 
#if NeedWidePrototypes
        int add_handler )
#else
	Boolean add_handler )
#endif
#endif
{
   EventMask           eventMask;

   eventMask = EnterWindowMask | LeaveWindowMask | FocusChangeMask;
   if (add_handler)
      XtInsertEventHandler( _XmFindTopMostShell(widget), eventMask, False,
         _XmSwallowEventHandler, NULL, XtListHead);
   else
      XtRemoveEventHandler(_XmFindTopMostShell(widget), eventMask, False,
         _XmSwallowEventHandler, NULL);
}

/*
 * Class method for traversal.  If there is a tear off control visible,
 * it must be put into a list so that the traversal code can find it.
 */
static Boolean
#ifdef _NO_PROTO
MakeChildList ( wid, childList, numChildren)
        Widget wid ;
        Widget ** childList;
        Cardinal * numChildren;
#else
MakeChildList (
        Widget wid ,
        Widget ** childList ,
        Cardinal * numChildren )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget rc = (XmRowColumnWidget) wid;
    int i;

    if (RC_TearOffControl(rc))
    {
     /*
      * add the TOC to the children list
      */
     *childList = 
       (WidgetList) XtMalloc(sizeof(Widget) * (rc->composite.num_children+1));

     (*childList)[0] = RC_TearOffControl(rc);

     for (i=1; i <= rc->composite.num_children; i++)
     {
       (*childList)[i] = rc->composite.children[i-1];
     }

     *numChildren = rc->composite.num_children+1;

     return (True);
 }
   else
       return (False);
}

static void 
#ifdef _NO_PROTO
MenuArm( w )
        Widget w ;
#else
MenuArm(
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = find_menu(w);
   XmMenuState mst = _XmGetMenuState((Widget)w);
   
   if (!RC_IsArmed(m))
   {
      /*
       * indicate that the display is grabbed so that drag & drop will not
       * interfere with the menu interaction.
       */
      XmDisplay disp = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
      disp->display.userGrabbed = True;
      
      if (IsBar(m))
      {
         Widget topmostShell = _XmFindTopMostShell( (Widget) m);
         Arg args[1];

         /*
          * save the current highlighted item so that it can be restored after
          * the menubar traversal mode is finished.  This only makes sense
          * if the menubar itself is not the current tabgroup.
          */
         mst->RC_activeItem = _XmGetActiveItem( (Widget) m);
         if (mst->RC_activeItem && XtParent(mst->RC_activeItem) == (Widget)m)
            mst->RC_activeItem = NULL;

         /* Make sure focus policy is explicit during active menubar */
         if ((RC_OldFocusPolicy(m) = _XmGetFocusPolicy( (Widget) m)) !=
              XmEXPLICIT)
         {
           /* Generate a synthetic crossing event from current focus widget
            * to menubar so the current focus widget can unhighlight itself.
            * This is necessary in case the leave routine checks for XmPOINTER
            * focus policy (as in _XmLeaveGadget).
            */
            XCrossingEvent xcrossing;

            /* If activeItem is NULL, then there's no possible widget/gadget to
            * unhighlight.
             */
            if (mst->RC_activeItem)
            {
               xcrossing.type = LeaveNotify;
               xcrossing.serial =
                  LastKnownRequestProcessed(XtDisplay(mst->RC_activeItem));
               xcrossing.send_event = False;
               xcrossing.display = XtDisplay(mst->RC_activeItem);
               xcrossing.window = XtWindow(mst->RC_activeItem);
               xcrossing.subwindow = 0;
               xcrossing.time = 
		  XtLastTimestampProcessed(XtDisplay(mst->RC_activeItem));
               xcrossing.mode = 1;
               xcrossing.detail = NotifyNonlinear;
               xcrossing.same_screen = True;
               xcrossing.focus = True;
               xcrossing.state = 0;

               XtDispatchEvent((XEvent *) &xcrossing);
            }

            XtSetArg (args[0], XmNkeyboardFocusPolicy, XmEXPLICIT);
            XtSetValues (topmostShell, args, 1);
         }

         /*
          * _XmMenuFocus(XmMENU_BEGIN) already called; switch tab group.
          * 'widget' is cascade button for menubar case.
          */
         m->manager.traversal_on = True;
         XmProcessTraversal(w, XmTRAVERSE_CURRENT);

         /*
          * Menubars need their own exclusive/SL grab, so that they will
          * still get input even when a cascade button without a submenu
          * has the focus.
          */
         _XmAddGrab( (Widget) m, True, True);
         RC_SetBeingArmed(m, True);

         /* Swallow focus & crossing events to menubar (only) */
         _XmSetSwallowEventHandler((Widget) m, True);
      }

      RC_SetArmed (m, True);
   }
}

static void 
#ifdef _NO_PROTO
MenuDisarm( w )
        Widget w ;
#else
MenuDisarm(
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = find_menu(w);
   XmMenuState mst = _XmGetMenuState((Widget)w);

   if (RC_IsArmed(m))
   {
      /*
       * indicate that the display is ungrabbed so that drag & drop
       * is enabled if this is the toplevel menu
       */
      if (IsBar(m) || IsPopup(m) || IsOption(m) ||
	  (IsPulldown(m) && !(XmIsMenuShell(XtParent(m)))))
      {
	  XmDisplay disp = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
	  disp->display.userGrabbed = False;
      }

      if (IsBar(m))
      {
         Widget topmostShell = _XmFindTopMostShell( (Widget) m);
         Arg args[1];

         _XmRemoveGrab( (Widget) m);
         RC_SetBeingArmed(m, False);

         m->manager.traversal_on = False;

         /* restore focus policy */
         if (RC_OldFocusPolicy(m) == XmEXPLICIT)
         {
            /*
             * restore the activeItem that had the focus BEFORE the
             * menubar mode was entered.
             */
            if (mst->RC_activeItem)
            {
               XmProcessTraversal (mst->RC_activeItem, XmTRAVERSE_CURRENT);
               mst->RC_activeItem = NULL;
            } else
            {
               XmProcessTraversal (topmostShell, XmTRAVERSE_NEXT_TAB_GROUP);
            }
         }
         else   /* XmPOINTER */
         {
            if (m->manager.active_child)
            {
               XmCascadeButtonHighlight(m->manager.active_child, False);
               _XmClearFocusPath((Widget) m);
            }

	    /* Clear internal focus structures so key events will be
	     * dispatched correctly.
	     */
	    XtSetKeyboardFocus(topmostShell, None);

            XtSetArg (args[0], XmNkeyboardFocusPolicy, XmPOINTER);
            XtSetValues (topmostShell, args, 1);
         }
         _XmSetSwallowEventHandler((Widget) m, False);
     }

      /* tear off shell */      
     else if ((IsPulldown(m) || IsPopup(m)) && !XmIsMenuShell(XtParent(m)))
     {
         _XmRemoveGrab( (Widget) m);
         RC_SetBeingArmed(m, False);
     }
     RC_SetArmed (m, FALSE);
   }
}

static void 
#ifdef _NO_PROTO
TearOffArm( w )
        Widget w ;
#else
TearOffArm(
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = find_menu(w);
   Display *dpy = XtDisplay(w);
   /* Time _time = XtLastTimestampProcessed(XtDisplay(w)); */
   Time _time = CurrentTime;

   /* If the parent is torn and active, AND this is an initial selection
    * (e.g. ButtonPress), place the menu system into an active state by 
    * arming and setting up grabs.
    */
   if ((IsPulldown(m) || IsPopup(m)) &&
        !XmIsMenuShell(XtParent(m)) && !RC_IsArmed(m))
   {
      _XmMenuFocus((Widget)m, XmMENU_BEGIN, _time);

      _XmGrabPointer((Widget)m, True, 
         ((unsigned int) (ButtonPressMask | ButtonReleaseMask | 
	 EnterWindowMask | LeaveWindowMask)),
         GrabModeSync, GrabModeAsync,
         None, XmGetMenuCursor(dpy), _time);

      /* To support menu replay, keep the pointer in sync mode */
      XAllowEvents(dpy, SyncPointer, _time);

      (* xmLabelClassRec.label_class.menuProcs) (XmMENU_ARM, (Widget) m);

      /* A submenu is posted!  Set up modal grab */
      /* Make it act like a pseudo popup and give it spring loaded
       * characteristics.
       */
      _XmAddGrab( (Widget) m, True, True);

      XFlush(dpy);
   }
}


/**********************************************************************
 *
 * next section knows how to composite row column entries
 */
static void 
#ifdef _NO_PROTO
FixEventBindings( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
FixEventBindings(
        XmRowColumnWidget m,    /* row column (parent) widget */
        Widget w )              /* subwidget */
#endif /* _NO_PROTO */
{
   if (XtIsWidget(w) &&
       ((IsPopup(m) || IsBar(m) || IsPulldown(m)) && 
	 XmIsLabel(w) && (w->core.widget_class != xmLabelWidgetClass)))
   {
      XtAddEventHandler(w, KeyPressMask|KeyReleaseMask, False,
			KeyboardInputHandler, m);

   }

   /* set up accelerators and mnemonics */
   ProcessSingleWidget (w, XmADD);
}

/*
 * Class function which is used to clean up the menubar when it is leaving
 * the mode where the user has pressed F10 to start traversal in the
 * menubar.
 */
static void 
#ifdef _NO_PROTO
MenuBarCleanup( rc )
        XmRowColumnWidget rc ;
#else
MenuBarCleanup(
        XmRowColumnWidget rc )
#endif /* _NO_PROTO */
{
    _XmMenuSetInPMMode ((Widget)rc, False);
}


static void
#ifdef _NO_PROTO
InvalidateOldFocus(oldWidget, poldFocus, event)
	Widget oldWidget;
	Window **poldFocus;
	XEvent *event;
#else
InvalidateOldFocus(
	Widget oldWidget, Window **poldFocus, XEvent *event)
#endif
{
  *poldFocus = NULL;
}

static int SIF_ErrorFlag;

static int
#ifdef _NO_PROTO
SIF_ErrorHandler(display, event)
     Display *display;
     XErrorEvent *event;
#else
SIF_ErrorHandler(Display *display, XErrorEvent* event)
#endif
{
    SIF_ErrorFlag = event -> type;
    return 0 ; /* unused */
}

static void
#ifdef _NO_PROTO
SetInputFocus(display, focus, revert_to, time)
     Display *display;
     Window focus;
     int revert_to;
     Time time;
#else
SetInputFocus(Display *display, Window focus, int revert_to, Time time)
#endif
{
  XErrorHandler old_Handler;

  /* Setup error proc and reset error flag */
  old_Handler = XSetErrorHandler((XErrorHandler) SIF_ErrorHandler);
  SIF_ErrorFlag = 0;
  
  /* Set the input focus */
  XSetInputFocus(display, focus, revert_to, time);
  XSync(display, False);

  XSetErrorHandler(old_Handler);
}
  

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuFocus( w, operation, _time )
        Widget w ;
        int operation ;
        Time _time ;
#else
_XmMenuFocus(
        Widget w,
        int operation,
        Time _time )
#endif /* _NO_PROTO */
{
   XWindowAttributes   xwa ;
   XmMenuState mst = _XmGetMenuState((Widget)w);
 
   switch (operation)
      {
	case XmMENU_END:
	  if (mst->RC_menuFocus.oldFocus != 0)
	  {
	     /* Reset the focus when there's a valid window:
	      *  - the old focus window/widget is a shell and it's popped up
	      *  - the old focus window/widget is viewable (mapped)
	      * And the destroy callback hasn't invalidated oldFocus
	      */
	     if (mst->RC_menuFocus.oldWidget != (Widget) NULL)
	     {
		XtRemoveCallback(mst->RC_menuFocus.oldWidget, 
		   XtNdestroyCallback, (XtCallbackProc)InvalidateOldFocus, 
		   (XtPointer) &mst->RC_menuFocus.oldFocus);

		/* oldWidget is not destroyed so the window is valid. */
		if (XtIsRealized(mst->RC_menuFocus.oldWidget)) 
		{
		   /* 99.9% of the time, oldWidget is a shell.  So we'll
		    * funnel everything through XGetWindowAttributes.  For
		    * non-shells we could just call _XmIsViewable().
		    */
		   XGetWindowAttributes( XtDisplay(mst->RC_menuFocus.
		      oldWidget), mst->RC_menuFocus.oldFocus, &xwa);
		   if (xwa.map_state == IsViewable)
		   {
		     SetInputFocus(XtDisplay(w), mst->RC_menuFocus.
				   oldFocus, mst->RC_menuFocus.oldRevert, 
				   _time);
		   }
		 }
	     }
	     /*
	      * Since the focus may have been taken from a window and not a widget,
	      * set the focus back to that window if there is not associated widget.
	      * This was added to correct focus problems in CR 4896 and 4912.
	      */
	     else
	       {
		  SetInputFocus(XtDisplay(w), mst->RC_menuFocus.oldFocus,
				 mst->RC_menuFocus.oldRevert, _time);
	       }
	     mst->RC_menuFocus.oldFocus = 0;
	     mst->RC_menuFocus.oldRevert = 0;
	     mst->RC_menuFocus.oldWidget = (Widget) NULL;
	  }
	  XtUngrabKeyboard(w, _time);
	  break;
	case XmMENU_BEGIN:
	  /* We must grab the keyboard before the InputFocus is set for mwm
	   * to work correctly.
	   */
          _XmGrabKeyboard(w, True, GrabModeSync, GrabModeSync, _time);

	  XGetInputFocus(XtDisplay(w), &mst->RC_menuFocus.oldFocus, &mst->
	     RC_menuFocus.oldRevert);
	  mst->RC_menuFocus.oldWidget = XtWindowToWidget(XtDisplay(w), mst->
	     RC_menuFocus.oldFocus);
	  if (mst->RC_menuFocus.oldWidget)
	    XtAddCallback(mst->RC_menuFocus.oldWidget, XtNdestroyCallback, 
	      (XtCallbackProc)InvalidateOldFocus, (XtPointer) &mst->
		 RC_menuFocus.oldFocus);

	  SetInputFocus(XtDisplay(w), XtWindow(w), mst->RC_menuFocus.oldRevert,
			 _time);

	  XAllowEvents(XtDisplay(w), AsyncKeyboard, _time);

	  /* Call XAllowEvents(.., SyncKeyboard, ..) after the pointer is
	   * grabbed.
	   */

	  XFlush(XtDisplay(w));

	  break;
	case XmMENU_MIDDLE:
	  SetInputFocus(XtDisplay(w), XtWindow(w), mst->RC_menuFocus.oldRevert,
			 _time);
	  break;
      }
}


/*
 * Class function which is invoked when the post accelerator is received
 * for a popup menu or the menubar, or the post mnemonic is received for 
 * an option menu.
 */
static void 
#ifdef _NO_PROTO
ArmAndActivate( w, event, parms, num_parms )
        Widget w ;
        XEvent *event ;
        String *parms ;
        Cardinal *num_parms ;
#else
ArmAndActivate(
        Widget w,
        XEvent *event,
        String *parms,
        Cardinal *num_parms )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = (XmRowColumnWidget) w ;
   int i;
   XmCascadeButtonWidget child;
   Cursor cursor;
   XmMenuState mst = _XmGetMenuState((Widget)w);
   Time _time;

   _time = __XmGetDefaultTime(w, event);

   if (IsPopup(m))
   {
      if (RC_TornOff(m) && !XmIsMenuShell(XtParent(m)))
	 _XmRestoreTearOffToMenuShell((Widget)m, event);

      if (!XtIsManaged(m))
      {
	 Position x, y;

	 /* the posted from widget is saved in RC_CascadeBtn */
	 if (mst->RC_LastSelectToplevel)
	    RC_CascadeBtn(m) = mst->RC_LastSelectToplevel;
	 else
	    RC_CascadeBtn(m) = XtParent(XtParent(m));	/* help! help! */
	 
	 /* Position & post menupane; then enable traversal */
	 RC_SetWidgetMoved(m, True);

/*_XmLastSelectToplevel */

	 /* Position the pane off of the last selected toplevel, or if there
	  * isn't one, off of its parent (help!).
	  * Place it in the upper left corner.
	  */

	 if (mst->RC_LastSelectToplevel)
	     XtTranslateCoords(mst->RC_LastSelectToplevel, 0, 0, &x, &y);
	 else
	     XtTranslateCoords(XtParent(XtParent(m)), 0, 0, &x, &y);
	     
	 XtX(m) = x;
	 XtY(m) = y;

	 /* Verify popup for MenuShell's manage_set_changed() */
	 mst->RC_ButtonEventStatus.time = event->xkey.time;
	 mst->RC_ButtonEventStatus.verified = True;
	 mst->RC_ButtonEventStatus.event = *((XButtonEvent *)event);

	 XtManageChild( (Widget) m);
	 _XmSetInDragMode((Widget) m, False);
         XmProcessTraversal((Widget) m, XmTRAVERSE_CURRENT);
      }
      else
      {
         /* Let the menushell widget clean things up */
         (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
            menu_shell_class.popdownDone))(XtParent(m), event, NULL, NULL);
      }
   }
   else if (IsOption(m))
   {
      XmGadget g = (XmGadget) XmOptionButtonGadget( (Widget) m);

      /* Let the cascade button gadget do the work */
      (*(((XmGadgetClassRec *)XtClass(g))->gadget_class.
          arm_and_activate)) ((Widget) g, event, parms, num_parms);
   }
   else if (IsBar(m))
   {
      if (RC_IsArmed(m))
      {
	 _XmMenuPopDown((Widget) m, event, NULL);
      }
      else
      {
         _XmMenuSetInPMMode ((Widget)m, True);

	 /* traversal must be on for children to be traversable */
         m->manager.traversal_on = True;

         for (i = 0; i < m->composite.num_children; i++)
         {
	    child = (XmCascadeButtonWidget)m->composite.children[i];

            if (!IsHelp(m, (Widget)child) &&
	        XmIsTraversable( (Widget) child))
               break;
         }

         /* See if we found one */
        if ((i >= m->composite.num_children) &&
              !(RC_HelpPb (m) &&
                XmIsTraversable( (Widget) RC_HelpPb (m))))
	{
	    m->manager.traversal_on = False;
            return;
	}

	 cursor = _XmGetMenuCursorByScreen(XtScreen(m));
	 _XmMenuFocus( (Widget) m, XmMENU_BEGIN, _time);
	 MenuArm(m->composite.children[i]);

	 _XmGrabPointer ( (Widget) m, True,
			 ((unsigned int) (ButtonPressMask | ButtonReleaseMask |
			 EnterWindowMask | LeaveWindowMask)),
			 GrabModeSync, GrabModeAsync, None, cursor,
			 _time);

	 RC_SetBeingArmed(m, False);

         /* To support menu replay, keep the pointer in sync mode.
	  * The grab freezes the pointer queue, unfreeze for this
	  * instance where a keyevent has caused the grab.
	  */
	 XAllowEvents(XtDisplay(m), SyncPointer, _time);

         _XmSetInDragMode((Widget) m, False);
      }
   }
   else if (IsPulldown(m))	/* Catch the Escape in a cascading menu! */
   {
      /* Let the menushell widget clean things up */
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
         menu_shell_class.popdownOne))(XtParent(m), event, NULL, NULL);
   }
}


/*
 * The following functions are used to manipulate lists of keyboard events
 * which are of interest to the menu system; i.e. accelerators and mnemonics.
 * The top level component within a menu system (the menubar, or the popup
 * menupane, or the option menu, or the work area menu) keeps a list of
 * the events it cares about.  Items are only added to the list when the
 * associated menu component has a complete path to the top level component.
 * Items are removed when the complete path is broken.
 *
 * The times at which a complete path may be formed is 1) When a button
 * in a menupane becomes managed, when a pulldown menupane is attached
 * to a cascading button, when the application sets the popupEnabled
 * resource to True, or when an option menu is created.  A complete path 
 * may be broken when 1) a button in a menupane is unmanaged, a pulldown 
 * menupane is detached from a cascading button, when the application clears 
 * the popupEnabled resource or an option menu is destroyed.
 *
 * The event handler for catching keyboard events is added by the row column
 * widget during its initialize process, and removed when the row column
 * widget is destroyed.  Keyboard grabs, which are needed to make accelerators
 * work, are added either when the accelerator is registered, or when the 
 * associated widget is realized; grabs cannot be added to a widget which 
 * does not have a window!
 */ 


/*
 * This function is used both by the row column widget, and components which
 * are contained within a menu (toggle, pushbutton and cascadebutton).  The
 * row column widget uses it to process a tree of menu components; when a
 * menupane is linked to a cascade button, the new menupane, along with any
 * submenus cascading from it, will be processed.  The row column widget
 * will use the XmADD or XmDELETE mode to to this.  When a menu component
 * needs to change its accelerator or mnemonics, it will use the XmREPLACE
 * mode.
 *
 * When this function is used to delete a tree of keyboard actions, the
 * link between the menupane at the root (parameter 'w') and the cascade
 * button it is attached to must not yet have been broken.  This allows
 * the function to trace up the hierarchy and find the top level widget.
 */
static void 
#ifdef _NO_PROTO
DoProcessMenuTree( w, mode )
        Widget w ;
        int mode ;
#else
DoProcessMenuTree(
        Widget w,
        int mode )          /* Add, Replace or Delete */
#endif /* _NO_PROTO */
{
   /*
    * Depending upon the type of widget 'w' is, we may end up adding/deleting
    * keyboard actions for no widgets, the specified widget only, or the
    * specified widget and all others which cascade from it.
    */
   if (XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w))
   {
      /* If our parent is not a row column, then abort */
      if (XmIsRowColumn(XtParent(w)))
      {
         if (IsOption(XtParent(w)))
         {
            if (mode == XmREPLACE)
               return;

            /*
             * A cascade button in an option menu does not have an
             * accelerator or a mnemonic associated with it.  However,
             * its submenu may, so we need to process it.
             */
            if (XmIsCascadeButtonGadget(w))
                w = (Widget)CBG_Submenu(w);
         }
         else if (IsWorkArea(XtParent(w)))
         {
            /*
             * Since work area menus do not support cascade buttons
             * with submenus, we will treat this like a pushbutton.
             */
            if (mode == XmREPLACE)
            {  
               /* Remove old one, if it exists */
               ProcessSingleWidget(w, XmDELETE);
               mode = XmADD;
            }

            ProcessSingleWidget(w, mode);
            return;
         }
         else if (IsBar(XtParent(w)) || 
                  IsPopup(XtParent(w)) ||
                  IsPulldown(XtParent(w)))
         {
            if (mode == XmREPLACE)
            {
               /* When replacing, don't worry about submenus */
               ProcessSingleWidget(w, XmDELETE);
               ProcessSingleWidget(w, XmADD);
               return;
            }

            /*
             * If we are in a menubar, a popup menupane or a pulldown
             * menupane, then we need to not only modify our keyboard 
             * event, but also any which are defined in our submenu.
             */
            ProcessSingleWidget(w, mode);

            if (XmIsCascadeButtonGadget(w))
                w = (Widget)CBG_Submenu(w);
            else
                w = (Widget)CB_Submenu(w);
         }
      }
   }
   else if (XmIsToggleButtonGadget(w) || XmIsToggleButton(w) ||
            XmIsPushButtonGadget(w) || XmIsPushButton(w))
   {
      if (mode == XmREPLACE)
      {
         /* Remove old one */
         ProcessSingleWidget(w, XmDELETE);
         mode = XmADD;
      }

      /*
       * In both of these cases, we need only modify the keyboard
       * event associated with this widget.
       */
      ProcessSingleWidget(w, mode);
      return;
   }
   else if (XmIsRowColumn(w))
   {
      /*
       * When the popupEnabled resource is enabled for a popup menupane,
       * we need to add the accelerator associated with the menu, followed 
       * by all keyboard events associated with submenus.  The reverse
       * happens when the resource is disabled.
       *
       * When an option menu is created, we will add its posting mnemonic;
       * its submenu is taken care of when it is attached to the cascading
       * button.
       */
      if (IsPopup(w))
      {
         if (mode == XmREPLACE)
         {
            /* Don't worry about submenus during a replace operation */
            ProcessSingleWidget(w, XmDELETE);
            ProcessSingleWidget(w, XmADD);
            return;
         }

         ProcessSingleWidget(w, mode);
      }
      else if (IsOption(w) || IsBar(w))
      {
         if (mode == XmREPLACE)
         {
            ProcessSingleWidget(w, XmDELETE);
            mode = XmADD;
         }

         ProcessSingleWidget(w, mode);
         return;
      }
   }
   else
   {
      /* Unknown widget type; do nothing */
      return;
   }

   /* Process any submenus.
    * Don't call this if the widget is in the process of being destroyed
    *   since its children are already gone. 
    * Don't process if mode is DELETE and it's in a sharedmenupanehierarchy
    *   to preverve accelerators and mnemonics.  If we allow check to slip
    *   through to RemoveFromKeyboardList, the mnemonics disappear.
    */
   if (w && !(w->core.being_destroyed) && 
       !((mode == XmDELETE) &&
	 InSharedMenupaneHierarchy((XmRowColumnWidget)w)))
   {
       ProcessMenuTree((XmRowColumnWidget)w, mode);
   }
}


/*
 * Given a row column widget, all keyboard events are processed
 * for the items within the row column widget, and then recursively
 * for any submenus cascading from this row column widget.
 */
static void 
#ifdef _NO_PROTO
ProcessMenuTree( w, mode )
        XmRowColumnWidget w ;
        int mode ;
#else
ProcessMenuTree(
        XmRowColumnWidget w,
        int mode )
#endif /* _NO_PROTO */
{
   int i;
   Widget child;

   if (w == NULL)
      return;

   for (i = 0; i < w->composite.num_children; i++)
   {
      if (XtIsManaged((child = w->composite.children[i])))
      {
         ProcessSingleWidget(child, mode);

         if (XmIsCascadeButtonGadget(child))
         {
            ProcessMenuTree((XmRowColumnWidget) CBG_Submenu(child), mode);
         }
         else if (XmIsCascadeButton(child))
         {
            ProcessMenuTree((XmRowColumnWidget)
                             CB_Submenu( (XmCascadeButtonWidget) child), mode);
         }
      }
   }
}


/*
 * This function adds/deletes the mnemonic and/or accelerator associated
 * with the specified widget.  The work that is done is dependent both
 * on the widget in question, and sometimes the parent of the widget.
 *
 * When adding a keyboard event, we first check the component to see if
 * it has any keyboard events defined; if not, then nothing is done.  However,
 * when removing a keyboard event, we simply attempt to remove the entry for
 * the specified widget; we can't check to see if the widget had one defined,
 * because the instance structure may no longer contain the information.
 */
static void 
#ifdef _NO_PROTO
ProcessSingleWidget( w, mode )
        Widget w ;
        int mode ;
#else
ProcessSingleWidget(
        Widget w,
        int mode )
#endif /* _NO_PROTO */
{
   Arg args[2];
   Widget child;

   if (XmIsCascadeButtonGadget(w))
   {
      XmCascadeButtonGadget c = (XmCascadeButtonGadget)w;

      if (XmIsRowColumn(XtParent(w)) &&
          (IsBar(XtParent(w))))
      {
         if (mode == XmADD)
         {
            /* Menubar mnemonics are prefixed with the Mod1 modifier */
            if (LabG_Mnemonic(c) != NULL)
            {
	       _AddToKeyboardList (w, KeyRelease, LabG_Mnemonic(c), Mod1Mask,
				   True, False);

	       /* save it again as a mnemonic so that it is available w/o
                * the Mod1 when the menu system is posted */
	       _AddToKeyboardList (w, KeyRelease, LabG_Mnemonic(c), 0,
				   False, True);

            }
         }
         else
            RemoveFromKeyboardList(w);
      }
      else
      {
         if (mode == XmADD)
         {
            /* All other mnemonics are done without any modifiers */
            if (LabG_Mnemonic(c) != NULL)
            {
	       _AddToKeyboardList (w, KeyRelease, LabG_Mnemonic(c), 0,
				   False, True);
            }
         }
         else
            RemoveFromKeyboardList(w);
      }
   }
   else if (XmIsCascadeButton(w))
   {
      XmCascadeButtonWidget c = (XmCascadeButtonWidget)w;

      if (XmIsRowColumn(XtParent(w)) &&
          (IsBar(XtParent(w))))
      {
         if (mode == XmADD)
         {
            /* Menubar mnemonics are prefixed with the Mod1 modifier */
            if (Lab_Mnemonic(c) != NULL)
            {
	       /* save as an accelerator since it is available anytime */
	       _AddToKeyboardList (w, KeyRelease, Lab_Mnemonic(c), Mod1Mask,
				   True, False);

	       /* save again as a mnemonic so its available w/o Mod1
		* once the menu system is active */ 
	       _AddToKeyboardList (w, KeyRelease, Lab_Mnemonic(c), 0,
				   False, True);
            }
         }
         else
            RemoveFromKeyboardList(w);
      }
      else
      {
         if (mode == XmADD)
         {
            /* All other mnemonics are done without any modifiers */
            if (Lab_Mnemonic(c) != NULL)
            {
	       _AddToKeyboardList (w, KeyRelease, Lab_Mnemonic(c), 0,
				   False, True);
            }
         }
         else
            RemoveFromKeyboardList(w);
      }
   }
   else if (XmIsToggleButtonGadget(w) ||
            XmIsPushButtonGadget(w))
   {
      XmLabelGadget l = (XmLabelGadget) w;

      if (mode == XmADD)
      {
         /* These can have both an accelerator and a mnemonic */
         if (LabG_Mnemonic(l) != NULL)
         {
	     _AddToKeyboardList (w, KeyRelease, LabG_Mnemonic(l), 0,
				 False, True);
         }

         if (LabG_Accelerator(l) && (strlen(LabG_Accelerator(l)) > 0))
         {
            AddToKeyboardList(w, LabG_Accelerator(l), True, False);
         }
      }
      else
         RemoveFromKeyboardList(w);
   }
   else if (XmIsToggleButton(w) ||
            XmIsPushButton(w))
   {
      XmLabelWidget l = (XmLabelWidget) w;

      if (mode == XmADD)
      {
         /* These can have both an accelerator and a mnemonic */
         if (Lab_Mnemonic(l) != NULL)
         {
	     _AddToKeyboardList (w, KeyRelease, Lab_Mnemonic(l), 0,
				 False, True);
         }

         if (Lab_Accelerator(l) && (strlen(Lab_Accelerator(l)) > 0))
         {
            AddToKeyboardList(w, Lab_Accelerator(l), True, False);
         }
      }
      else
         RemoveFromKeyboardList(w);
   }
   else if (XmIsRowColumn(w))
   {
      XmRowColumnWidget m = (XmRowColumnWidget) w;

      if (IsPopup(m) || IsBar(m))
      {
         /* 
          * Popup Menus and the menubar may have an accelerator associated 
          * with them 
          */
         if (mode == XmADD)
         {
            if (RC_MenuAccelerator(m) && (strlen(RC_MenuAccelerator(m)) > 0))
            {
               AddToKeyboardList(w, RC_MenuAccelerator(m), True, False);
            }
         }
         else
            RemoveFromKeyboardList(w);
      }
      else if (IsOption(m))
      {
	 child = XmOptionLabelGadget( (Widget) m);
         /* Option menus may have a mnemonics associated with them */
         if (mode == XmADD)
         {
            if (RC_Mnemonic(m))
	    {
	       _AddToKeyboardList (w, KeyRelease, RC_Mnemonic(m), Mod1Mask,
				   True, True);
	       if (child)
	       {
		  XtSetArg(args[0], XmNmnemonic, RC_Mnemonic(m));
		  XtSetValues(child, args, 1);
	       }
	     }
         }
         else
         {
            RemoveFromKeyboardList(w);
            
            /* Tell the label gadget */
            if (child &&
		!child->core.being_destroyed)
            {
               XtSetArg(args[0], XmNmnemonic, '\0');
	       XtSetValues(child, args, 1);
            }
         }
      }
   }
}


/*
 * This function actually does the work of converting the accelerator
 * or mnemonic string into a workable format, and registering the keyboard 
 * grab, if possible.
 */
static void 
#ifdef _NO_PROTO
AddToKeyboardList( w, kbdEventStr, needGrab, isMnemonic )
        Widget w ;
        char *kbdEventStr ;
        Boolean needGrab ;
        Boolean isMnemonic ;
#else
AddToKeyboardList(
        Widget w,
        char *kbdEventStr,
#if NeedWidePrototypes
        int needGrab,
        int isMnemonic )
#else
        Boolean needGrab,
        Boolean isMnemonic )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   int eventType = KeyPress;
   unsigned keysym;
   unsigned int modifiers;
   KeySym vkeysym = NoSymbol;
   Modifiers vmodifiers = 0;

   if (kbdEventStr != NULL &&
       _XmMapKeyEvent(kbdEventStr, &eventType, &keysym, &modifiers) == True) {

      /* If it is a virtual key, get the physical key and modifiers */ 
      _XmVirtualToActualKeysym(XtDisplay(w), (KeySym)keysym, &vkeysym,
				&vmodifiers);
      if (vkeysym != NoSymbol) {
         keysym = (unsigned) vkeysym;
	 /* Also need to grab any modifiers in the virtual key binding */
         modifiers |= (unsigned int)vmodifiers;
      }

      _AddToKeyboardList (w, eventType, (KeySym)keysym, modifiers, needGrab,
				isMnemonic);
   }
}

/*
 * This is a lower level interface to AddToKeyboardList which avoids the
 * overhead of passing in a string.
 */
static void 
#ifdef _NO_PROTO
_AddToKeyboardList( w, eventType, keysym, modifiers, needGrab, isMnemonic )
        Widget w ;
        unsigned int eventType;
        KeySym keysym;
        unsigned int modifiers;
        Boolean needGrab ;
        Boolean isMnemonic ;
#else
_AddToKeyboardList(
        Widget w,
        unsigned int eventType,
        KeySym keysym,
        unsigned int modifiers,
#if NeedWidePrototypes
        int needGrab,
        int isMnemonic )
#else
        Boolean needGrab,
        Boolean isMnemonic )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   Widget rowcol;
   KeyCode detail = 1;  /* Keycoades lie in the range 8 - 255 */
   XmKeyboardData * list;
   XKeyEvent event;
   int i;

   /*
    * if needGrab is FALSE, delay the call to XKeysymToKeycode until the
    * first time the value of the keycode is needed.
    */
   if (needGrab)
   {
      /* Convert keysym to keycode; needed by X grab call */
      if ((detail = XKeysymToKeycode(XtDisplay(w), keysym)) == 0)
      {
	  _XmWarning( (Widget)w, BadMnemonicCharMsg);
	  return;
      }
   }

   if (XmIsRowColumn(w))
       rowcol = w;
   else
       rowcol = XtParent(w);

   /*
    * For shared menupanes, only enter the unique event once.
    */
   event.type = eventType;
   event.keycode = detail;
   event.state = isMnemonic ? modifiers & ~(ShiftMask | LockMask) : modifiers;
   
   /* Add to the list of keyboard entries */
   if (MGR_NumKeyboardEntries(rowcol) >= MGR_SizeKeyboardList(rowcol))
   {
      /* Grow list */
      MGR_SizeKeyboardList(rowcol) += 10;
      MGR_KeyboardList(rowcol) = 
	     (XmKeyboardData *)XtRealloc( (char *) MGR_KeyboardList(rowcol), 
		  (MGR_SizeKeyboardList(rowcol) * sizeof(XmKeyboardData)));
   }

   list = MGR_KeyboardList(rowcol);
   i = MGR_NumKeyboardEntries(rowcol);
   list[i].eventType = eventType;
   list[i].keysym = keysym;
   list[i].key = detail;
   list[i].modifiers = isMnemonic ? 
      (modifiers &  ~(ShiftMask | LockMask)) : modifiers;
   list[i].component = w;
   list[i].needGrab = needGrab;
   list[i].isMnemonic = isMnemonic;
   MGR_NumKeyboardEntries(rowcol)++;

   if (needGrab)
   {
      GrabKeyOnAssocWidgets ((XmRowColumnWidget) rowcol, detail, modifiers);
   }
}


/*
 * This function removes all keyboard entries associated with a particular
 * component within a row column widget.
 * If the component is a push or toggle button, the keyboard_list entry 
 * must be removed so that CheckKey() doesn't catch the old key events
 * if this is a replacement!  If the menu-item has a grab installed,
 * then we can only ungrab if the parent pane is not shared.  The same 
 * applies for (shareable) Popups.  Menubar and Option accelerators aren't 
 * shared and can always be ungrabbed.
 */
static void 
#ifdef _NO_PROTO
RemoveFromKeyboardList( w )
        Widget w ;
#else
RemoveFromKeyboardList(
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rowcol;
   XmKeyboardData * klist;
   int count;
   int i, j;
   Boolean notInSharedMenupaneHierarchy;

   if (XmIsRowColumn(w))
       rowcol = (XmRowColumnWidget)w;
   else
       rowcol = (XmRowColumnWidget)XtParent(w);

   if (IsWorkArea(rowcol))
      return;

   notInSharedMenupaneHierarchy = !InSharedMenupaneHierarchy(rowcol);

   klist = MGR_KeyboardList(rowcol);
   count = MGR_NumKeyboardEntries(rowcol);
   
   for (i = 0; i < count; )
   {
      if (klist[i].component == w)
      {
	 /* NOTE that the ungrabs on shared menupane associate widgets are not 
          * done and thus causes extra event deliveries and possible memory
          * leaks.  The problem is that it is difficult to tell whether an 
          * item should really be ungrabbed since the sharing of menupanes 
          * could mean that this item exists somewhere else on this hierarchy. 
	  */
         if (klist[i].needGrab && 
	     (w->core.being_destroyed || notInSharedMenupaneHierarchy))
	 {
	    Boolean ungrab;

	    /* This edge case occurs when the app destroys the menu item
	     * and recreates it with the same accelerator (e.g. in a callback).
	     * Since Constraint-Destroy occurs during phase II, after the
	     * new button is created, the ungrab would occur for the new
	     * button as well.  Note that for 2.0 where binary compatibility
	     * is not an issue (?), I'd use a ref count in the keyboardlist
	     * structure!  For now, let the destroyer suffer the n^2 alg!
	     */
	    for(j=0, ungrab=True; (j<count) && ungrab; j++)
	       if ( (klist[j].component != w) &&
		    klist[j].needGrab &&
		    (klist[i].key == klist[j].key) &&
		    (klist[i].modifiers == klist[j].modifiers) )
		  ungrab = False;

	    if (ungrab)
	       UngrabKeyOnAssocWidgets(rowcol, klist[i].key,
		  klist[i].modifiers);
	 }

         /* Move the rest of the entries up 1 slot */
         for (j = i; j < count -1; j++)
	     klist[j] = klist[j+1];

         MGR_NumKeyboardEntries(rowcol) = MGR_NumKeyboardEntries(rowcol)-1;
         count--;
      }
      else
         i++;
   }
}


/*
 * This function searches the list of keyboard events associated with the
 * specified  row column widget to see if any of them match the
 * passed in X event.  This function can be called multiple times, to get
 * all entries which match.
 */
static int 
#ifdef _NO_PROTO
_XmMatchInKeyboardList( rowcol, event, startIndex )
        XmRowColumnWidget rowcol ;
        XKeyEvent *event ;
        int startIndex ;
#else
_XmMatchInKeyboardList(
        XmRowColumnWidget rowcol,
        XKeyEvent *event,
        int startIndex )
#endif /* _NO_PROTO */
{
   XmKeyboardData * klist = MGR_KeyboardList(rowcol);
   int count = MGR_NumKeyboardEntries(rowcol);
   int i;

   if (klist == NULL)
      return(-1);

   for (i = startIndex; i < count; i++)
   {
      /*
       * We want to ignore shift and shift-lock for mnemonics.  So, OR the 
       * event's two bits with the (previously two bits initialized to zero) 
       * klist.modifier
       *
       * If the .key field is 1, then we have delayed calling XKeysymToKeycode
       * until now.
       */
       if (klist[i].key == 1)
	   klist[i].key = XKeysymToKeycode(XtDisplay(rowcol), klist[i].keysym);

       if (klist[i].key != NoSymbol)
       {
	   if (_XmMatchKeyEvent((XEvent *) event, klist[i].eventType,
				klist[i].key, klist[i].isMnemonic ? 
				klist[i].modifiers | (event->state &
						     (ShiftMask | LockMask)) :
				klist[i].modifiers)) 
	   {
	       return(i);
	   }
       }
   }

   /* No match */
   return (-1);
}

/*
 * search the postFromList and return the index of the found widget.  If it
 * is not found, return -1
 */
static int 
#ifdef _NO_PROTO
OnPostFromList( menu, widget )
        XmRowColumnWidget menu ;
        Widget widget ;
#else
OnPostFromList(
        XmRowColumnWidget menu,
        Widget widget )
#endif /* _NO_PROTO */
{
   int i;

   for (i = 0; i < menu->row_column.postFromCount; i++)
   {
      if (menu->row_column.postFromList[i] == widget)
	  return (i);
   }

   return (-1);
}
   
/*
 * Useful for MenuBars and Option Menus to determine where to set up the
 * event handlers and grabs.
 */
static void 
#ifdef _NO_PROTO
GetTopManager( w, topManager )
        Widget w ;
        Widget *topManager ;
#else
GetTopManager(
        Widget w,
        Widget *topManager )
#endif /* _NO_PROTO */
{
   while (XmIsManager(XtParent(w)))
       w = XtParent(w);

   * topManager = w;
}

/*
 * Returns the toplevel menu widget in an acive menu hierarchy.
 *
 * This function is only useful when the menu system is active.  That is
 * the only time that the CascadeBtn field in the RowColumn in guaranteed
 * to be valid.  
 */
void 
#ifdef _NO_PROTO
_XmGetActiveTopLevelMenu( wid, rwid )
        Widget wid ;
        Widget *rwid ;
#else
_XmGetActiveTopLevelMenu(
        Widget wid,
        Widget *rwid )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget w = (XmRowColumnWidget) wid ;
   XmRowColumnWidget *topLevel = (XmRowColumnWidget *) rwid ;

   /*
    * find toplevel by following up the chain. Popups use CascadeBtn to
    * keep the active widget in the postFromList.  Stop at tear off!
    */
   while (RC_CascadeBtn(w) && (!IsPopup(w)) && XmIsMenuShell(XtParent(w)))
       w = (XmRowColumnWidget) XtParent(RC_CascadeBtn(w));

   * topLevel = w;
} 

/*
 * set up the grabs on the appropriate assoc widgets.  For a popup, this
 * is all of the widgets on the postFromList.  For a menubar and option
 * menu, this is the top manager widget in their hierarchy.  For a
 * pulldown, the assoc widgets can only be determined by following the
 * chain up the postFromList.
 */
static void 
#ifdef _NO_PROTO
GrabKeyOnAssocWidgets( rowcol, detail, modifiers )
        XmRowColumnWidget rowcol ;
        KeyCode detail ;
        unsigned int modifiers ;
#else
GrabKeyOnAssocWidgets(
        XmRowColumnWidget rowcol,
#if NeedWidePrototypes
        int detail,
#else
        KeyCode detail,
#endif /* NeedWidePrototypes */
        unsigned int modifiers )
#endif /* _NO_PROTO */
{
   Widget topManager;
   int i;
   
   if (IsPopup(rowcol))
   {
      for (i=0; i < rowcol->row_column.postFromCount; i++)
         XtGrabKey(rowcol->row_column.postFromList[i], detail, modifiers,
            False, GrabModeAsync, GrabModeAsync);
   }
   else if (IsBar(rowcol) || IsOption(rowcol))
   {
      GetTopManager ((Widget) rowcol, &topManager);
      XtGrabKey(topManager, detail, modifiers, False, 
	 GrabModeAsync, GrabModeAsync);
   }
   else if (IsPulldown(rowcol))
   {
      for (i=0; i<rowcol->row_column.postFromCount; i++)
         GrabKeyOnAssocWidgets ((XmRowColumnWidget)
            XtParent(rowcol->row_column.postFromList[i]), detail, modifiers);
   }
}  

/*
 */
static void 
#ifdef _NO_PROTO
UngrabKeyOnAssocWidgets( rowcol, detail, modifiers )
        XmRowColumnWidget rowcol ;
        KeyCode detail ;
        unsigned int modifiers ;
#else
UngrabKeyOnAssocWidgets(
        XmRowColumnWidget rowcol,
#if NeedWidePrototypes
        int detail,
#else
        KeyCode detail,
#endif /* NeedWidePrototypes */
        unsigned int modifiers )
#endif /* _NO_PROTO */
{
   Widget assocWidget;
   int i;
   
   if (IsPopup(rowcol))
   {
      for (i=0; i < rowcol->row_column.postFromCount; i++)
      {
	 assocWidget = rowcol->row_column.postFromList[i];
	 if (!assocWidget->core.being_destroyed)
	    XtUngrabKey(assocWidget, detail, modifiers);
      }
   }
   else if (IsBar(rowcol) || IsOption(rowcol))
   {
      GetTopManager ((Widget) rowcol, &assocWidget);
      if (!assocWidget->core.being_destroyed)
         XtUngrabKey(assocWidget, detail, modifiers);
   }
   else if (IsPulldown(rowcol))
   {
      for (i=0; i<rowcol->row_column.postFromCount; i++)
         UngrabKeyOnAssocWidgets ((XmRowColumnWidget)
            XtParent(rowcol->row_column.postFromList[i]), detail, modifiers);
   }
}  
       
/*
 * This is the event handler which catches, verifies and dispatches all
 * accelerators and mnemonics defined for a given menu hierarchy.  It
 * is attached to the menu's associated widget, along with an assortment
 * of other widgets.
 */
static void 
#ifdef _NO_PROTO
KeyboardInputHandler( reportingWidget, data, event, cont )
        Widget reportingWidget ;
        XtPointer data ;
        XEvent *event ;
        Boolean *cont ;
#else
KeyboardInputHandler(
        Widget reportingWidget,
        XtPointer data,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget topLevel = (XmRowColumnWidget) data;
   ShellWidget topLevelShell = (ShellWidget)XtParent(topLevel);
   XmMenuState mst = _XmGetMenuState((Widget)topLevel);

   /* Process the event only if not already processed */
   if (!_XmIsEventUnique(event))
      return;

   if (IsBar(topLevel) || IsOption(topLevel))
       if (! _XmAllWidgetsAccessible((Widget) topLevel))
	   return;

   /* 
    * XmGetPostFromWidget() requires help to identify the topLevel widget
    * when a menupane is posted via accelerators.
    */
   if (IsBar(topLevel) || IsOption(topLevel))
      mst->RC_LastSelectToplevel = (Widget) topLevel;
   else if ((IsPopup(topLevel) || IsPulldown(topLevel)) &&
	    !XmIsMenuShell(topLevelShell) && 
	    _XmFocusIsInShell((Widget)topLevel))
      mst->RC_LastSelectToplevel = topLevel->row_column.tear_off_lastSelectToplevel;
   else if (IsPopup(topLevel))
   {
      /* If the popup is already posted, lastSelectToplevel already set! */
      if (!(XmIsMenuShell(topLevelShell) && topLevelShell->shell.popped_up))
         mst->RC_LastSelectToplevel = reportingWidget;	/* popup */
   }
   else
      mst->RC_LastSelectToplevel = NULL;

   ProcessKey (topLevel, event);

   /* reset toplevel "accelerator" state to NULL */
   mst->RC_LastSelectToplevel = NULL;
}

/*
 * try to find a match in the menu for the key event.   Cascade down the
 * submenus if necessary
 */
static Boolean 
#ifdef _NO_PROTO
ProcessKey( rowcol, event )
        XmRowColumnWidget rowcol ;
        XEvent *event ;
#else
ProcessKey(
        XmRowColumnWidget rowcol,
        XEvent *event )
#endif /* _NO_PROTO */
{
   Boolean found = FALSE;
   int i;
   Widget child;
   Widget SaveCascadeButton;

   /* Try to use it on the current rowcol */
   if (! CheckKey (rowcol, event))
   {
      /* not used, try moving down the cascade */
      for (i=0; (i < rowcol->composite.num_children) && (! found); i++)
      {
	 child = rowcol->composite.children[i];

	 /* only check sensitive and managed cascade buttons */
	 if (XtIsSensitive(child) && XtIsManaged(child))
	 {
	    if (XmIsCascadeButtonGadget(child))
	    {
	       if (CBG_Submenu(child))
	       {
		   SaveCascadeButton = RC_CascadeBtn(CBG_Submenu(child));
		   /* Build the menu cascade for menuHistory */
		   RC_CascadeBtn(CBG_Submenu(child)) = child;
		   found = ProcessKey((XmRowColumnWidget)
		        ((XmCascadeButtonGadget)child)->cascade_button.submenu,
			event);
		   /* Restore the cascade button / submenu link in case of
		    * shared menupanes.
		    */
		   if (!found)
		       RC_CascadeBtn(CBG_Submenu(child)) = SaveCascadeButton;

	       }
	    }
	    else if (XmIsCascadeButton(child))
	    {
	       if (CB_Submenu(child))
	       {
		   SaveCascadeButton = RC_CascadeBtn(CB_Submenu(child));
		   RC_CascadeBtn(CB_Submenu(child)) = child;
		   found = ProcessKey((XmRowColumnWidget) 
		        ((XmCascadeButtonWidget)child)->cascade_button.submenu,
			event);
		   if (!found)
		       RC_CascadeBtn(CB_Submenu(child)) = SaveCascadeButton;
	       }
	    }
	 }
      }
      return (found);
   }
   else
       return (True);
}

/*
 * Check if the key event is used in the rowcol
 */
static Boolean 
#ifdef _NO_PROTO
CheckKey( rowcol, event )
        XmRowColumnWidget rowcol ;
        XEvent *event ;
#else
CheckKey(
        XmRowColumnWidget rowcol,
        XEvent *event )
#endif /* _NO_PROTO */
{
   int menu_index = 0;
   XmKeyboardData * entry;
   ShellWidget shell;
   
   /* Process all matching key events */
   while ((menu_index = _XmMatchInKeyboardList(rowcol, (XKeyEvent *) event,
                                                            menu_index)) != -1)
   {
      entry = MGR_KeyboardList(rowcol) + menu_index;

      /* Ignore this entry if it is not accessible to the user */
      if (XmIsRowColumn(entry->component))
      {
	 /*
	  * Rowcols are not accessible if they are insensitive or
	  * if menubars or optionmenus are unmanaged.
	  */
	 if (! XtIsSensitive(entry->component) ||
	     ((RC_Type(entry->component) != XmMENU_POPUP) &&
	      (RC_Type(entry->component) != XmMENU_PULLDOWN) &&
	      (! XtIsManaged(entry->component))))
	 {
	    menu_index++;
	    continue;
	 }
      }
      else if (((XmIsMenuShell(XtParent(rowcol)) && 
		 ((XmMenuShellWidget)XtParent(rowcol))->shell.popped_up) || 
		!_XmGetMenuState((Widget)rowcol)->
		   RC_allowAcceleratedInsensitiveUnmanagedMenuItems) &&
               (!XtIsSensitive(entry->component) || 
		!XtIsManaged(entry->component)))
      {
      /* In general, insensitive or unmanaged buttons are not accessible.
       * However, to support shared menupanes, we will allow applications to 
       *   pass all accelerated items through, regardless of senstivity/managed.
       *   EXCEPT when the pane is posted, and then sensitivity is presumed
       *   valid.  
       * (The individual menu items also checks if the tear off has the focus.)
       */
	 menu_index++;
	 continue;
      }

      /* 
       * For a mnemonic, the associated component must be visible, and
       * it must be in the last menupane posted.
       * This only needs to be checked for a popup or pulldown menu pane.
       */
      if (entry->isMnemonic)
      {
         if ((XmIsLabel(entry->component) || 
              XmIsLabelGadget(entry->component)))
	 {
	    if (IsBar(XtParent(entry->component)) &&
		! RC_PopupPosted(XtParent(entry->component)) &&
		((XmManagerWidget) XtParent(entry->component))->
		     manager.active_child == NULL)
	    {
	      menu_index++;
	      continue;
	    }

	    else if (IsPopup(XtParent(entry->component)) ||
		     IsPulldown(XtParent(entry->component)))
	      {
		/* See if the associated shell is visible */
		shell = (ShellWidget)XtParent(XtParent(entry->component));
		
		/*
		 * Verify the pane is popped up, and the active pane is our 
		 * parent (this is necessary because of shared menupanes.
		 */
		if ((!shell->shell.popped_up) ||
		    (shell->composite.children[0] != 
		     XtParent(entry->component)))
		  {
		    menu_index++;
		    continue;
		  }

		/* Verify we are the last pane */
		if (RC_PopupPosted(shell->composite.children[0]))
		  {
		    menu_index++;
		    continue;
		  }
	      }
	  }
         else if (XmIsRowColumn(entry->component))
         {
	    /*
	     * Ignore the posting mnemonic for an option menu, if its
	     * menupane is already posted.
	     */
	    if (RC_PopupPosted(entry->component))
	    {
	       menu_index++;
	       continue;
	    }
	 }
      }

      /* Key event - make sure we're not in drag mode */
      _XmSetInDragMode(entry->component, False);

      /* Perform the action associated with the keyboard event */
      if (XmIsPrimitive(entry->component))
      {
         XmPrimitiveClassRec * prim;

         prim = (XmPrimitiveClassRec *)XtClass(entry->component);

         (*(prim->primitive_class.arm_and_activate)) 
                                         (entry->component, event, NULL, NULL);
      }
      else if (XmIsGadget(entry->component))
      {
         XmGadgetClassRec * gadget;

         gadget = (XmGadgetClassRec *)XtClass(entry->component);

         (*(gadget->gadget_class.arm_and_activate)) 
                                         (entry->component, event, NULL, NULL);
      }
      else if (XmIsRowColumn(entry->component))
      {
         XmRowColumnClassRec * rc;

         rc = (XmRowColumnClassRec *)XtClass(entry->component);
         (*(rc->row_column_class.armAndActivate)) (entry->component, event, 
                                                                   NULL, NULL);
      }

      /* used the key */
      _XmRecordEvent(event);
      return (True);
   }

   /* did not use the key */
   return (False);
}

static void 
#ifdef _NO_PROTO
AddToPostFromList( m, widget )
        XmRowColumnWidget m ;
        Widget widget ;
#else
AddToPostFromList(
        XmRowColumnWidget m,
        Widget widget )
#endif /* _NO_PROTO */
{
   
   if (m->row_column.postFromListSize == m->row_column.postFromCount)
   {
      /* increase the size to fit the new one and one more */
      m->row_column.postFromListSize += 2;
      m->row_column.postFromList = (Widget *)
	  XtRealloc ( (char *) m->row_column.postFromList,
		     m->row_column.postFromListSize * sizeof(Widget));
   }

   m->row_column.postFromList[m->row_column.postFromCount++] = widget;

   /* If the popup's attach widget mysteriously is destroyed, remove the
    * attach widget from the popup's postFromList.  (Note the cascade
    * button already does this from its Destroy method).
    */ 
   if (IsPopup(m))
      XtAddCallback(widget, XtNdestroyCallback,
         (XtCallbackProc)RemoveFromPostFromListOnDestroyCB, (XtPointer)m);
}

static void
#ifdef _NO_PROTO
RemoveFromPostFromListOnDestroyCB (w, clientData, callData)
	Widget w;
	caddr_t clientData, callData;
#else
RemoveFromPostFromListOnDestroyCB (
	Widget w,
	caddr_t clientData,
	caddr_t callData )
#endif /* _NO_PROTO */
{
   /* The attach_widget has been destroyed.  Remove it from the Popup's 
    * postFromList.
    */
   XmRemoveFromPostFromList((Widget)clientData, w);
}

static void 
#ifdef _NO_PROTO
RemoveFromPostFromList( m, widget )
        XmRowColumnWidget m ;
        Widget widget ;
#else
RemoveFromPostFromList(
        XmRowColumnWidget m,
        Widget widget )
#endif /* _NO_PROTO */
{
   int i;
   Boolean found = False;

   for (i=0; i < m->row_column.postFromCount; i++)
   {
      if (!found)
      {
	 if (widget == m->row_column.postFromList[i])
	 {
	    /* remove this entry */
	    found = True;
	 }
      }
      else
	  m->row_column.postFromList[i-1] = m->row_column.postFromList[i];
   }
   if (found)
   {
      m->row_column.postFromCount--;

      if (IsPopup(m))
	 XtRemoveCallback(widget, XtNdestroyCallback,
	    (XtCallbackProc)RemoveFromPostFromListOnDestroyCB, (XtPointer)m);
   }
}


static Boolean 
#ifdef _NO_PROTO
InSharedMenupaneHierarchy( m )
        XmRowColumnWidget m ;
#else
InSharedMenupaneHierarchy(
        XmRowColumnWidget m)
#endif /* _NO_PROTO */
{
   while (m && XmIsRowColumn(m) && (IsPulldown(m) || IsPopup(m)))
   {
      if (m->row_column.postFromCount == 1)
	 m = (XmRowColumnWidget)XtParent(m->row_column.postFromList[0]);
      else
	 return(True);
   }

   return(False);
}

/*
 * This is a class function exported by the RowColumn widget.  It is used
 * by the CascadeButton widget to signal that a menupane has either been
 * attached to a cascade button widget, or detached from a cascade button
 * widget.
 */
static void 
#ifdef _NO_PROTO
SetCascadeField( m, cascadeBtn, attach )
        XmRowColumnWidget m ;
        Widget cascadeBtn ;
        Boolean attach ;
#else
SetCascadeField(
        XmRowColumnWidget m,
        Widget cascadeBtn,
#if NeedWidePrototypes
        int attach )
#else
        Boolean attach )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   int mode;

   if (attach)
   {
      mode = XmADD;

      /* if being attached to an option menu, set the option menus submenu */
      if (RC_Type(XtParent(cascadeBtn)) == XmMENU_OPTION)
	  RC_OptionSubMenu(XtParent(cascadeBtn)) = (Widget) m;

      if (XmIsMenuShell(XtParent(m)))
      {
	 XtX(XtParent(m)) = XtY(XtParent(m)) = 0;
      }

      if (m->row_column.postFromCount && 
	  (RC_TearOffModel(m) == XmTEAR_OFF_ENABLED))
         _XmWarning( (Widget)m, TearOffSharedMenupaneMsg);

      if (OnPostFromList (m, cascadeBtn) != -1)
	  /* already in the list, this means no work to do */
	  return;

      AddToPostFromList (m, cascadeBtn);
   }

   else
   {
      mode = XmDELETE;
      RemoveFromPostFromList (m, cascadeBtn);

      /* if being removed from an option menu, set the option menus submenu */
      if (RC_Type(XtParent(cascadeBtn)) == XmMENU_OPTION)
	  RC_OptionSubMenu(XtParent(cascadeBtn)) = (Widget) NULL;

      /* CR 5550 fix begin */
      if (    ( m != NULL )
	  && ( RC_CascadeBtn(m) == (Widget)cascadeBtn )
	  )
	{
	  RC_CascadeBtn(m) = (Widget) NULL;
	}
      /* CR 5550 fix end */

      /* if we're in a shared menupane hierarchy, don't process (delete) the 
       * accelerators and mnemonics!
       */
      if (InSharedMenupaneHierarchy(m))
	 return;
   }

   /* process the accelerators and mnemonics */
   DoProcessMenuTree( (Widget) m, mode);
}


/*
 * This function determines if the widget to which a menu is 
 * attached is accessible to the user.  The widget is considered
 * accessible if it, and its ancestors, are both sensitive and
 * managed.  This is useful for MenuBars and Option Menus only.
 */
static Boolean 
#ifdef _NO_PROTO
_XmAllWidgetsAccessible( w )
        Widget w ;
#else
_XmAllWidgetsAccessible(
        Widget w )
#endif /* _NO_PROTO */
{
   while (w && XtParent(w) && !XtIsShell(w))
   {
      if (!XtIsSensitive(w) || !XtIsManaged(w) || !w->core.mapped_when_managed)
         return (False);

      w = XtParent(w);
   }

   return (True);
}


/*
 * _XmMatchBSelectEvent() is intended to be used to check if the event is
 * a valid BSelect.  It will only validate the event if the menu hierarchy
 * is posted (so BSelect doesn't override whichButton to post a menu).
 */
Boolean
#ifdef _NO_PROTO
_XmMatchBSelectEvent( wid, event )
	Widget wid;
	XEvent *event;
#else
_XmMatchBSelectEvent( 
	Widget wid,
	XEvent *event )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc;

   /* First make sure the menu hierarchy is posted */
   /* But if it's torn off then don't worry about posted */
   if (XmIsMenuShell(XtParent(wid)))
   {
      _XmGetActiveTopLevelMenu(wid, (Widget *)&rc);
      if ( (IsPopup(rc) && 
            !((XmMenuShellWidget)XtParent(rc))->shell.popped_up) ||
	   (!IsPopup(rc) && !RC_PopupPosted(rc)) )
	 return(False);
   }
   
   rc = (XmRowColumnWidget)wid;

   if ( !event )
      return (False);

#ifdef CDE_MENU_BTN
   if (_XmMatchBtnEvent(event, XmIGNORE_EVENTTYPE, Button3, AnyModifier))
      return(True);
#endif /* CDE_MENU_BTN */

   if ( !_XmMatchBtnEvent( event,  XmIGNORE_EVENTTYPE, 
	  /*BSelect.button, BSelect.modifiers*/ Button1, AnyModifier ))
      return(False);

   return(True);
}

Boolean
#ifdef _NO_PROTO
_XmMatchBDragEvent( wid, event )
	Widget wid;
	XEvent *event;
#else
_XmMatchBDragEvent( 
	Widget wid,
	XEvent *event )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc;

   /* First make sure the menu hierarchy is posted */
   /* But if it's torn off then don't worry about posted */
   if (XmIsMenuShell(XtParent(wid)))
   {
      _XmGetActiveTopLevelMenu(wid, (Widget *)&rc);
      if ( (IsPopup(rc) && 
            !((XmMenuShellWidget)XtParent(rc))->shell.popped_up) ||
	   (!IsPopup(rc) && !RC_PopupPosted(rc)) )
	 return(False);
   }
   
   rc = (XmRowColumnWidget)wid;

   if ( !event )
      return (False);

   if ( !_XmMatchBtnEvent( event,  XmIGNORE_EVENTTYPE, 
	  /*BDrag.button, BDrag.modifiers*/ Button2, 0 ))
      return(False);

   return(True);
}

static void 
#ifdef _NO_PROTO
BtnDownInRowColumn( rc, event, x_root, y_root )
        Widget rc ;
        XEvent *event ;
        Position x_root;
        Position y_root;
#else
BtnDownInRowColumn(
        Widget rc,
        XEvent *event,
#if NeedWidePrototypes
	int x_root,
        int y_root) 
#else
	Position x_root,
	Position y_root	   )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmGadget gadget;

   Position relativeX = event->xbutton.x_root - x_root;
   Position relativeY = event->xbutton.y_root - y_root;
   
   _XmSetMenuTraversal (rc, False);

   if ((gadget = _XmInputInGadget( rc, relativeX, relativeY)) != NULL)
   {
       /* event occured in a gadget w/i the rowcol */
       _XmDispatchGadgetInput( (Widget) gadget, event,
			      XmARM_EVENT);
   } else
      if (!XmIsMenuShell(XtParent(rc)))
      {
	 TearOffArm((Widget)rc);
      }

   /* For style guide conformance and consistency with Popup's and pulldown-
    * buttons, popdown cascading submenus below this point on button press
    * where individual buttons wouldn't be taking care of this.
    */
   if ((!gadget ||
	!XtIsSensitive(gadget) || 
	!(XmIsCascadeButtonGadget(gadget))) &&		/* !cbg kludge */
       (RC_PopupPosted(rc)))
   {
       (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	  menu_shell_class.popdownEveryone))(RC_PopupPosted(rc),NULL,
					     NULL, NULL);
   }

   /* BtnDown in the menu(bar) but outside of any cascade button or cascade
    * gadget must install grabs so events continue to be processed by the
    * menubar and are not erroneously passed to other widgets.
    */
   if (IsBar(rc) && !RC_IsArmed(rc) && !gadget)
   {
     Time _time = CurrentTime;
     Widget top_shell;

     _XmMenuFocus((Widget) rc, XmMENU_BEGIN, _time);

     /* This nasty section of code is mostly to handle focus and highlighting.
      * The call to MenuArm() calls XmProcessTraversal() to move the focus to
      * the menubar.  We don't want the first cascade button to highlight.  So,
      * the menubar manage flag is turned off to circumvent the call to
      * XmProcessTraversal.  (This might be better served by sending in a flag
      * to MenuArm() that avoids the call completely - hint, hint).  Then we
      * do some focus hocus pocus to turn off focus so that the current focus
      * item becomes unhighlighted and NOT move the focus/highlight to the
      * menubar yet (See Traversal.c _XmMgrTraversal() for precedence).  The
      * next cascade button enter event will take care of that.
      */
     rc->core.managed = False;
     MenuArm((Widget) rc);

     /* RC_activeItem has been saved away.  Clear it so when the focus is
      * set back the optimized traversal code doesn't think the old
      * activeItem already has the focus.
      */
     _XmClearFocusPath((Widget) rc);

     rc->core.managed = True;
     top_shell = _XmFindTopMostShell(rc);
     _XmSetFocusResetFlag( top_shell, TRUE) ;
     XtSetKeyboardFocus( top_shell, None) ;
     _XmSetFocusResetFlag( top_shell, FALSE) ;

     _XmSetInDragMode((Widget) rc, True);
     _XmGrabPointer((Widget) rc, True,
                   ((unsigned int) (ButtonPressMask | ButtonReleaseMask |
                   EnterWindowMask | LeaveWindowMask)),
                   GrabModeSync, GrabModeAsync, None,
                   XmGetMenuCursor(XtDisplay(rc)), _time);
     RC_SetBeingArmed(rc, False);
   }

   _XmRecordEvent(event);
   XAllowEvents(XtDisplay(rc), SyncPointer,
		__XmGetDefaultTime((Widget) rc, event));
}

static void 
#ifdef _NO_PROTO
CheckUnpostAndReplay( rc, event )
        Widget rc ;
        XEvent *event ;
#else
CheckUnpostAndReplay(
        Widget rc,
        XEvent *event )
#endif /* _NO_PROTO */
{
   XmMenuState mst = _XmGetMenuState((Widget)rc);

    if (_XmGetUnpostBehavior(rc) == XmUNPOST_AND_REPLAY)
    {
	_XmGetActiveTopLevelMenu(rc, &mst->RC_ReplayInfo.toplevel_menu);
	mst->RC_ReplayInfo.time = event->xbutton.time;

	/* do this before popdown since ptr is ungrabed there */
	XAllowEvents(XtDisplay(rc), ReplayPointer, 
		     __XmGetDefaultTime(rc, event));
	_XmMenuPopDown(rc, event, NULL);
    }
    else
    {
	_XmSetMenuTraversal (rc, False);
	_XmRecordEvent(event);
	XAllowEvents(XtDisplay(rc), SyncPointer, 
		     __XmGetDefaultTime(rc, event));
    }
}

void 
#ifdef _NO_PROTO
_XmHandleMenuButtonPress( wid, event )
        Widget wid ;
        XEvent *event ;
#else
_XmHandleMenuButtonPress(
        Widget wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
    Position x_root, y_root;
    
    XtTranslateCoords (wid, 0, 0, &x_root, &y_root);
    
    if ((event->xbutton.x_root >= x_root) &&
	(event->xbutton.x_root < x_root + wid->core.width) &&
	(event->xbutton.y_root >= y_root) &&
	(event->xbutton.y_root < y_root + wid->core.height))
    {
	/* happened in this rowcolumn */
	BtnDownInRowColumn (wid, event, x_root, y_root);
    }
    else if (RC_PopupPosted(wid))
    {
	/* follow down menu heierarchy */
	_XmHandleMenuButtonPress (((CompositeWidget) RC_PopupPosted(wid))->
				  composite.children[0], event);

    }
    else
    {
	/* nothing else posted */
	CheckUnpostAndReplay (wid, event);
	return;
    }
}


    /*
 * Button Action Procs
 */
void 
#ifdef _NO_PROTO
_XmMenuBtnDown( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmMenuBtnDown(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   Position x_root, y_root;
   Widget topLevel;
   XmMenuState mst = _XmGetMenuState((Widget)wid);
   Time _time = __XmGetDefaultTime((Widget) wid, event);

    if (!_XmIsEventUnique(event))
	return;
    
    if ((_XmMatchBtnEvent( event, RC_PostEventType(rc), RC_PostButton(rc),
			  RC_PostModifiers(rc))) ||
	_XmMatchBSelectEvent((Widget)rc, event))
    {

	/* Overload _XmButtonEventStatus's time for MenuShell's 
	 * managed_set_changed routine to determine if an option menu is 
	 * trying to post using BSelect Click.  _XmButtonEventStatus's 
	 * verified is irrelevant.
	 */
	if (IsOption(rc)) 
	{
	    mst->RC_ButtonEventStatus.time = event->xbutton.time;
	}

	XtTranslateCoords ((Widget) rc, 0, 0, &x_root, &y_root);

	if ((event->xbutton.x_root >= x_root) &&
	    (event->xbutton.x_root < x_root + rc->core.width) &&
	    (event->xbutton.y_root >= y_root) &&
	    (event->xbutton.y_root < y_root + rc->core.height))
	{

	    if (!XmIsMenuShell(XtParent(rc)) &&
		RC_Type(rc) != XmMENU_BAR &&
		RC_Type(rc) != XmMENU_OPTION)
		XChangeActivePointerGrab(XtDisplay(rc), 
				 ((unsigned int) (ButtonReleaseMask | 
				  PointerMotionMask |
				  PointerMotionHintMask |
				  EnterWindowMask | LeaveWindowMask)),
				 _XmGetMenuCursorByScreen(XtScreen(rc)),
					 _time);

	    /* happened in this rowcolumn */
	    BtnDownInRowColumn ((Widget) rc, event, x_root, y_root);
	}
	else
	{
	    _XmGetActiveTopLevelMenu( (Widget) rc, &topLevel );
	    if ((Widget) rc == topLevel)
	    {
		/* already looked in the toplevel, move to a pulldown */
		if (RC_PopupPosted(rc))
		{
		    topLevel = ((CompositeWidget) RC_PopupPosted(rc))->
			composite.children[0];
		}

		else
		{
		    /* nothing else posted */
		    CheckUnpostAndReplay ((Widget) rc, event);
		    return;
		}
	    }
	    /* travel down menu to see if in another menu widget */
	    _XmHandleMenuButtonPress (topLevel, event);
	}
    }
    else
	XAllowEvents(XtDisplay(rc), SyncPointer, _time);
	
}

void 
#ifdef _NO_PROTO
_XmMenuBtnUp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmMenuBtnUp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget w = (XmRowColumnWidget) wid ;
   XmGadget gadget;
   Time _time = __XmGetDefaultTime(wid, event);
   
   /* Support menu replay, free server input queue until next button event */
   XAllowEvents(XtDisplay(w), SyncPointer, _time);

   if (! _XmIsEventUnique(event) || 
       ! (_XmMatchBtnEvent( event, XmIGNORE_EVENTTYPE, RC_PostButton(w),
            RC_PostModifiers(w)) ||
	  _XmMatchBSelectEvent((Widget)w, event)) ||
       (IsBar(w) && ! RC_IsArmed(w)))
      return;

   if (w->core.window == event->xbutton.window)
      gadget = _XmInputInGadget( (Widget) w, event->xbutton.x,
                                                             event->xbutton.y);
   else
      gadget = NULL;

   if ((gadget != NULL) && XtIsSensitive(gadget))
   {
      _XmDispatchGadgetInput( (Widget) gadget, event, XmACTIVATE_EVENT);
   }
   else if (IsBar(w) || _XmIsTearOffShellDescendant((Widget)w))
   {
      /* Only drop in here when no other widget took the event */
      _XmMenuPopDown((Widget) w, event, NULL);
      if (IsBar(w))		/* Only necessary for truth and beauty */
         MenuBarCleanup(w);
      MenuDisarm((Widget) w);
      _XmMenuFocus( (Widget) w, XmMENU_END, _time);
      XtUngrabPointer( (Widget) w, _time);
   }
   _XmSetInDragMode((Widget) w, False);

   /* In a tear off, if the button up occured over a label or separator or
    * not in a button child, reset the focus to the first qualified item.
    */
   if ((IsPulldown(w) || IsPopup(w)) && !XmIsMenuShell(XtParent(w)) && 
       (!gadget || (XtClass(gadget) == xmLabelGadgetClass) ||
	(XtClass(gadget) == xmSeparatorGadgetClass)))
   {
      _XmClearFocusPath((Widget)w);
      XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
   }
}

/*****************************************************************************
 *
 * RowColumn's map and unmap callbacks funneled through here.
 *
 *****************************************************************************/

void
#ifdef _NO_PROTO
_XmCallRowColumnMapCallback(wid, event)
	Widget wid;
	XEvent *event;
#else
_XmCallRowColumnMapCallback(
	Widget wid,
	XEvent *event )
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XmRowColumnCallbackStruct callback;

   if (!RC_Map_cb(rc))
      return;

   callback.reason = XmCR_MAP;
   callback.event  = event;
   callback.widget = NULL;      /* these next two fields are spec'd NULL */
   callback.data   = NULL;
   callback.callbackstruct = NULL;
   XtCallCallbackList ((Widget)rc, RC_Map_cb(rc), &callback);
}

void
#ifdef _NO_PROTO
_XmCallRowColumnUnmapCallback(wid, event)
	Widget wid;
	XEvent *event;
#else
_XmCallRowColumnUnmapCallback(
	Widget wid,
	XEvent *event )
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XmRowColumnCallbackStruct callback;

   if (!RC_Unmap_cb(rc))
      return;

   callback.reason = XmCR_UNMAP;
   callback.event  = event;
   callback.widget = NULL;      /* these next two fields are spec'd NULL */
   callback.data   = NULL;
   callback.callbackstruct = NULL;
   XtCallCallbackList ((Widget)rc, RC_Unmap_cb(rc), &callback);
}

static void 
#ifdef _NO_PROTO
UpdateOptionMenuCBG( cbg, memWidget )
        Widget cbg ;
        Widget memWidget ;
#else
UpdateOptionMenuCBG(
        Widget cbg,
        Widget memWidget )
#endif /* _NO_PROTO */
{
   XmString xmstr = NULL;
   Pixmap pix, ipix;
   Arg al[4];
   int ac = 0;

   if (! (cbg && memWidget) )
      return;

   if (XmIsLabelGadget(memWidget))
   {
      XmLabelGadget lg = (XmLabelGadget) memWidget;

      if (LabG_IsText (lg))
      {
         XtSetArg (al[ac], XmNlabelType, XmSTRING);    ac++;
	 xmstr = _XmStringCreateExternal(LabG_Font(lg), LabG__label(lg));
         XtSetArg (al[ac], XmNlabelString, xmstr);      ac++;

	 if (LabG_Font(lg) != LabG_Font(cbg))
	 {
            XtSetArg (al[ac], XmNfontList, LabG_Font(lg)); ac++;
	 }
      }
      else
      {
         XtSetArg (al[ac], XmNlabelType, XmPIXMAP);    ac++;
	 pix = LabG_Pixmap(lg);
         XtSetArg (al[ac], XmNlabelPixmap, pix);      ac++;
	 ipix = LabG_PixmapInsensitive(lg);
         XtSetArg (al[ac], XmNlabelInsensitivePixmap, ipix);      ac++;
      }
      XtSetValues (cbg, al, ac);
   }
   else if (XmIsLabel(memWidget))
   {
      XmLabelWidget lw = (XmLabelWidget) memWidget;
      
      if (Lab_IsText (lw))
      {
         XtSetArg (al[ac], XmNlabelType, XmSTRING);    ac++;
	 xmstr = _XmStringCreateExternal(Lab_Font(lw), lw->label._label);
         XtSetArg (al[ac], XmNlabelString, xmstr);      ac++;

	 if (lw->label.font != LabG_Font(cbg))
	 {
            XtSetArg (al[ac], XmNfontList, lw->label.font); ac++;
	 }
      }
      else
      {
         XtSetArg (al[ac], XmNlabelType, XmPIXMAP);    ac++;
	 pix = lw->label.pixmap;
         XtSetArg (al[ac], XmNlabelPixmap, pix);      ac++;
	 ipix = lw->label.pixmap_insen;
         XtSetArg (al[ac], XmNlabelInsensitivePixmap, ipix);      ac++;
      }
      XtSetValues (cbg, al, ac);
   }

   if (xmstr)
      XmStringFree(xmstr);
}	

static int 
#ifdef _NO_PROTO
is_in_widget_list( m, w )
        register XmRowColumnWidget m ;
        RectObj w ;
#else
is_in_widget_list(
        register XmRowColumnWidget m,
        RectObj w )
#endif /* _NO_PROTO */
{
    register Widget *q;
    register int i;

    if ((m == NULL) || (w == NULL)) return (FALSE);

    for (i = 0, q = m->composite.children;
         i < m->composite.num_children;
         i++, q++) 

        if ((*q == (Widget) w) && IsManaged (*q)) return (TRUE);

    return (FALSE);
}

static int 
#ifdef _NO_PROTO
in_menu( search_m, parent_m, child, w )
        XmRowColumnWidget search_m ;
        XmRowColumnWidget *parent_m ;
        RectObj child ;
        Widget *w ;
#else
in_menu(
        XmRowColumnWidget search_m,
        XmRowColumnWidget *parent_m,
        RectObj child,
        Widget *w )
#endif /* _NO_PROTO */
{
    if (is_in_widget_list (search_m, child))
    {
        *parent_m = search_m;
        *w = (Widget) child;
        return (TRUE);
    }

    return (FALSE);
}

static Boolean 
#ifdef _NO_PROTO
search_menu( search_m, parent_m, child, w, setHistory )
        XmRowColumnWidget search_m ;
        XmRowColumnWidget *parent_m ;
        RectObj child ;
        Widget *w ;
        Boolean setHistory;
#else
search_menu(
        XmRowColumnWidget search_m,
        XmRowColumnWidget *parent_m,
        RectObj child,
        Widget *w,
#if NeedWidePrototypes
	int setHistory )
#else
	Boolean setHistory )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register Widget *q;
    register int i;

    if ( ! in_menu (search_m, parent_m, child, w))
    {
        for (i = 0, q = search_m->composite.children;
             i < search_m->composite.num_children;
             i++, q++) 
        {
            if (XtIsManaged(*q))
            {
                if (XmIsCascadeButtonGadget(*q))
                {
                    XmCascadeButtonGadget p = 
                        (XmCascadeButtonGadget) *q;

                    if (CBG_Submenu(p) &&
                        search_menu ((XmRowColumnWidget) CBG_Submenu(p),
                           (XmRowColumnWidget *) parent_m, (RectObj) child, w,
				     setHistory))
                    {
			if (setHistory)
			    RC_MemWidget(search_m) = (Widget) child;
                        return (TRUE);
                    }
                }
                else if (XmIsCascadeButton(*q))
                {
                    XmCascadeButtonWidget p =
                        (XmCascadeButtonWidget) *q;

                    if (CB_Submenu(p) &&
                        search_menu ((XmRowColumnWidget) CB_Submenu(p),
                           (XmRowColumnWidget *) parent_m, (RectObj) child, w,
				     setHistory))
                    {
			if (setHistory)
			    RC_MemWidget(search_m) = (Widget) child;
                        return (TRUE);
                    }
                }
            }
        }
        return (FALSE);
    }
    if (setHistory)
	RC_MemWidget(search_m) = (Widget) child;
	
    return (TRUE);
}

static void 
#ifdef _NO_PROTO
lota_magic( m, child, parent_m, w )
        XmRowColumnWidget m ;
        RectObj child ;
        XmRowColumnWidget *parent_m ;
        Widget *w ;
#else
lota_magic(
        XmRowColumnWidget m,
        RectObj child,
        XmRowColumnWidget *parent_m,
        Widget *w )
#endif /* _NO_PROTO */
{
    *parent_m = NULL;
    *w = NULL;

    search_menu (m, parent_m, child, w, False);
}

/*
 * called by the buttons to verify that the passed in event is one that
 * should be acted upon.  This is called through the menuProcs handle
 */
static void 
#ifdef _NO_PROTO
VerifyMenuButton( w, event, valid )
        Widget w ;
        XEvent *event ;
        Boolean *valid ;
#else
VerifyMenuButton(
        Widget w,
        XEvent *event,
        Boolean *valid )
#endif /* _NO_PROTO */
{
   *valid = event && (_XmMatchBtnEvent( event, XmIGNORE_EVENTTYPE,
			 RC_PostButton(w), RC_PostModifiers(w)) ||
		     _XmMatchBSelectEvent( w, event)) ; 
}

/*
 * This routine is called at Initialize or SetValues time.  It updates
 * the memory widget in the current rowcolumn and up to the top level(s)
 * menus.  If there is a postFromList on the pulldown, it goes up each
 * branch.  If an option menu is found at the top, then its cascadebutton
 * is updated with the latest stuff.
 */
static Boolean 
#ifdef _NO_PROTO
UpdateMenuHistory( menu, child, updateOnMemWidgetMatch )
        XmRowColumnWidget menu ;
        Widget child ;
	Boolean updateOnMemWidgetMatch;
#else
UpdateMenuHistory(
        XmRowColumnWidget menu,
        Widget child,
#if NeedWidePrototypes
	int updateOnMemWidgetMatch)
#else
	Boolean updateOnMemWidgetMatch)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   int i;
   Widget cb;
   Boolean retval = False;
   
   if (IsOption(menu))
   {
      if (updateOnMemWidgetMatch && (RC_MemWidget(menu) != child))
         return(False);

      if ((cb = XmOptionButtonGadget( (Widget) menu)) != NULL)
      {
	 UpdateOptionMenuCBG (cb, child);
	 retval = True;
      }
   }
   else if (IsPulldown(menu))
   {
      for (i=0; i < menu->row_column.postFromCount; i++)
      {
	 Widget parent_menu = XtParent(menu->row_column.postFromList[i]);

	 if (UpdateMenuHistory ((XmRowColumnWidget) parent_menu, child,
			    updateOnMemWidgetMatch))
	 {
	    RC_MemWidget(parent_menu) = child;
	    /* Don't return immediately - allow next postFromWidget to
	     * update menuHistory as well.
	     */
	    retval = True;
	 }
      }
   }
   return(retval);
}

/*
 * this is a mess... the menu spec'd is the menu to set the history for;
 * the child spec'd is the child who we are pretending fired-off.   The
 * problem is the child may be in any sub-menu of this cascade.  This is
 * called by Initialize or SetValues.
 */
static void 
#ifdef _NO_PROTO
SetMenuHistory( m, child )
        XmRowColumnWidget m ;
        RectObj child ;
#else
SetMenuHistory(
        XmRowColumnWidget m,
        RectObj child )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget parent_m;
   Widget w;

   if (IsNull (child))
       return;

   /* make sure that the child is in the menu hierarchy */
   lota_magic (m, child, &parent_m, &w);

   if (w)
       if (UpdateMenuHistory (parent_m, w, False))
	  RC_MemWidget(parent_m) = w;       
}

/*
 * This routine is similar to the one above, except it only sets the
 * memory widget for the submenus down the cascade.  This is called during
 * option menus setvalues and initialize routines.
 */
static void 
#ifdef _NO_PROTO
SetOptionMenuHistory( m, child )
        XmRowColumnWidget m ;
        RectObj child ;
#else
SetOptionMenuHistory(
        XmRowColumnWidget m,
        RectObj child )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget parent_m = NULL;
   Widget w = NULL;

   if (IsNull (child))
	return;

    search_menu (m, &parent_m, child, &w, True);

    
}

static void 
#ifdef _NO_PROTO
all_off_except( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
all_off_except(
        XmRowColumnWidget m,
        Widget w )
#endif /* _NO_PROTO */
{
    register Widget *q;
    register int i;

    if (w)  /* then all widgets except this one go off */
    {
        ForManagedChildren (m, i, q)
        {
            if (*q != w)
            {
                if (XmIsToggleButtonGadget(*q))
                {
                   if (XmToggleButtonGadgetGetState (*q))
                        XmToggleButtonGadgetSetState (*q, FALSE, TRUE);
                }
                else if (XmIsToggleButton(*q))
                {
                   if (XmToggleButtonGetState (*q))
                        XmToggleButtonSetState (*q, FALSE, TRUE);
                }
            }
        }
    }
}

static int 
#ifdef _NO_PROTO
no_toggles_on( m )
        XmRowColumnWidget m ;
#else
no_toggles_on(
        XmRowColumnWidget m )
#endif /* _NO_PROTO */
{
    register Widget *q;
    register int i;

    ForManagedChildren (m, i, q)
    {
        if (XmIsToggleButtonGadget(*q))
        {
            if (XmToggleButtonGadgetGetState (*q)) 
               return (FALSE);
        }
        else if (XmIsToggleButton(*q))
        {
            if (XmToggleButtonGetState (*q)) 
               return (FALSE);
        }
    }

    return (TRUE);
}


/* 
 * note that this is potentially recursive, setting the state of a 
 * toggle in this row column widget may re-enter this routine...
 */
static void 
#ifdef _NO_PROTO
RadioBehaviorAndMenuHistory( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
RadioBehaviorAndMenuHistory(
        XmRowColumnWidget m,
        Widget w )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget menu;
   Widget cb;
   Boolean done = FALSE;
   
   if (! IsManaged(w))
       return;
   
   if (RC_RadioBehavior(m))
   {
      if (XmIsToggleButtonGadget(w))
      {
	 /* turned on */
	 if (XmToggleButtonGadgetGetState (w)) 
	     all_off_except (m, w);

	 /* he turned off */
	 else  
	 {
            if (RC_RadioAlwaysOne(m))
                if (no_toggles_on (m))
		    /* can't allow that */
                    XmToggleButtonGadgetSetState (w, TRUE, TRUE);
	 }
      }
      else if (XmIsToggleButton (w))
      {
	 /* turned on */
	 if (XmToggleButtonGetState (w)) 
	     all_off_except (m, w);

	 /* turned off */
	 else
	 {
            if (RC_RadioAlwaysOne(m))
                if (no_toggles_on (m))  
		    /* can't allow that */
                    XmToggleButtonSetState (w, TRUE, TRUE);
	 }
      }
      
      /* record for posterity */
      RC_MemWidget (m) = w; 
   }

   /* record the mouse memory and history widget all the way up the cascade */
   menu = m;
   cb = 0;
   while ( ! done)
   {
      RC_MemWidget (menu) = w;
      
      if (! IsPopup(menu) && RC_CascadeBtn(menu))
      {
	cb = RC_CascadeBtn(menu);
	menu = (XmRowColumnWidget) XtParent (cb);
      }

      else
	  done = TRUE;
   }

   /* option menu cascade button gadget must be updated */
   if (IsOption(menu))
       UpdateOptionMenuCBG (cb, w);
}


static char * 
#ifdef _NO_PROTO
which_callback( w )
        Widget w ;
#else
which_callback(
        Widget w )
#endif /* _NO_PROTO */
{
    if (XmIsPushButtonGadget(w) || XmIsPushButton(w) || 
        XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w) ||
	XmIsDrawnButton(w))
       return (XmNactivateCallback);

    if (XmIsToggleButtonGadget(w) || XmIsToggleButton(w))
       return (XmNvalueChangedCallback);

    return (NULL);
}

/*
 * This routine is used to emulate the override callback functionality that
 * was available in the R3 library used by Xm and to do the radio behavior
 * and menu history functionality for RowColumns.  The buttons call this
 * function through the MenuProcs interface.
 */
static void 
#ifdef _NO_PROTO
ChildsActivateCallback( rowcol, child, call_value )
        XmRowColumnWidget rowcol ;
        Widget child ;
        XtPointer call_value ;
#else
ChildsActivateCallback(
        XmRowColumnWidget rowcol,
        Widget child,
        XtPointer call_value )
#endif /* _NO_PROTO */
{
   Arg arg[1];
   int i;
   XtCallbackList callbacks;
   char *c = which_callback (child);       /* which callback to use */

   /*
    * Set up info before the entry callback is called
    */
   GetLastSelectToplevel(rowcol);

   if (rowcol->row_column.entry_callback != NULL)
   {
      XtSetArg (arg[0], c, &callbacks);
      XtGetValues (child, arg, 1);

      /* make sure the all of the drawing has been done before the callback */
      XFlush (XtDisplay (rowcol));

      /*
       * cycle through the list and call the entry fired routine for each
       * entry in this callback list, sending in the data for each entry.
       * If the list is NULL, or empty, call the entry fired function once.
       */
      if ((callbacks == NULL) || (callbacks[0].callback == NULL))
	  EntryFired (child, NULL, (XmAnyCallbackStruct *) call_value);

      else
      {
	  int count;
	  XtPointer * callbackClosure;
	  /*
	   * Note:  We must make a copy of the callback data returned since
	   * the information may be lost on the next Xt call. 
	   * We are only interested in the closure data for the entry callback.
	   */

	  for (count=0; callbacks[count].callback != NULL; count++);
	  	  
	  callbackClosure =
	      (XtPointer *) XtMalloc(sizeof(XtPointer) * count);

	  for (i=0; i < count; i++)
	      callbackClosure[i] = callbacks[i].closure;

	  for (i=0; i < count; i++)
	    EntryFired (child, callbackClosure[i], (XmAnyCallbackStruct *) call_value);

	  XtFree ((char *)callbackClosure);
      }
   }
   else
       /* no entry callback, but must do radio behavior & menu history */
       EntryFired (child, NULL, (XmAnyCallbackStruct *) call_value);
}
        
/*
 * This is the callback for widgets which are composited into row column
 * widgets.  It notifies the menu that some individual widget fired off; 
 * this allows * the row column widget to tell the application if it wants 
 * to know.  also to do various other automagical things
 */
static void 
#ifdef _NO_PROTO
EntryFired( w, client_data, callback )
        Widget w ;
        XtPointer client_data ;
        XmAnyCallbackStruct *callback ;
#else
EntryFired(
        Widget w,
        XtPointer client_data,
        XmAnyCallbackStruct *callback )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget m = (XmRowColumnWidget) XtParent (w);
    XmRowColumnCallbackStruct mr;

    mr.reason       = XmCR_ACTIVATE;    /* menu activated */
    mr.widget       = w;
    mr.data         = (char *) client_data;
    mr.callbackstruct   = (char *) callback;  /* subwidget structure */
    mr.event            = callback->event;

    XtCallCallbackList ((Widget) m, m->row_column.entry_callback, &mr);

    RadioBehaviorAndMenuHistory (m, w);
}


/*************************************************************************
 * 
 * this section is all the layout stuff, the whole thing is messy because
 * it has to operate in two different modes, one: a read-only mode which
 * is nice for making decisions about the size of the row column vs. the size
 * of the children.  two: a change everything mode which implements the
 * change.
 *
 * further complicated by the xtoolkit restriction that a subwidget making
 * a geo request (referred to as the 'instigator') of the row column may not 
 * have his resize proc called but all other widget children must.
 *
 * this is done by building a set of XtWidgetGeometry request blocks, one
 * for each child (widget and gadget), which holds the changes we would like to 
 * make for this child.  If needed then another pass is made over the requests 
 * to actually implement the changes.
 */


/*
 * count the widest & tallest entry dimensions
 * and compute entries per row/column
 */
static void 
#ifdef _NO_PROTO
get_info( m, border, w, h, items_per, baseline, shadow, highlight, margin_top, margin_height, text_height )
        XmRowColumnWidget m ;
        Dimension *border;
        Dimension *w ;
        Dimension *h ;
        int *items_per ;
        Dimension *baseline;
        Dimension *shadow;
        Dimension *highlight;
        Dimension *margin_top;
        Dimension *margin_height;
        Dimension *text_height;
#else
get_info(
        XmRowColumnWidget m,
        Dimension *border,
        Dimension *w,
        Dimension *h,
        int *items_per,
        Dimension *baseline,
        Dimension *shadow,
        Dimension *highlight,
        Dimension *margin_top,
        Dimension *margin_height,
        Dimension *text_height )
#endif /* _NO_PROTO */
{
    XtWidgetGeometry *b;
    int i, n = 0;

    *border = *w = *h = *baseline = *shadow = *highlight = *margin_top = *margin_height = *text_height = 0;

    if (RC_TearOffControl(m) && XtIsManaged(RC_TearOffControl(m)))
       i = 1;
    else
       i = 0;

    for (; RC_Boxes (m) [i].kid != NULL; i++)
    {
       b = &(RC_Boxes (m) [i].box);
       n++;

       if (*w < BWidth  (b))
	   *w = BWidth (b);
       if (*h < BHeight (b))
       {
	   *h = BHeight (b);
       }

       if (XtIsWidget(RC_Boxes (m) [i].kid))
         if (*border < RC_Boxes (m) [i].kid->core.border_width)
              *border = RC_Boxes (m) [i].kid->core.border_width;
       else if (XmIsGadget(RC_Boxes (m) [i].kid))
         if (*border < ((XmGadget)RC_Boxes (m) [i].kid)->rectangle.border_width)
             *border = ((XmGadget)RC_Boxes (m) [i].kid)->rectangle.border_width;
       
       if (XmIsLabel (RC_Boxes (m) [i].kid))
       {
         if (*baseline < RC_Boxes (m) [i].baseline)
         {
             *baseline = RC_Boxes (m) [i].baseline;
         }
         if (*shadow < Lab_Shadow(RC_Boxes (m) [i].kid))
             *shadow = Lab_Shadow(RC_Boxes (m) [i].kid);
         if (*highlight < Lab_Highlight(RC_Boxes (m) [i].kid))
             *highlight = Lab_Highlight(RC_Boxes (m) [i].kid);
         if (*margin_top < Lab_MarginTop(RC_Boxes (m) [i].kid))
             *margin_top = Lab_MarginTop(RC_Boxes (m) [i].kid);
         if (*margin_height < Lab_MarginHeight(RC_Boxes (m) [i].kid))
             *margin_height = Lab_MarginHeight(RC_Boxes (m) [i].kid);
         if (*text_height < Lab_TextRect_height(RC_Boxes (m) [i].kid))
             *text_height = Lab_TextRect_height(RC_Boxes (m) [i].kid);
       }
       else if (XmIsLabelGadget (RC_Boxes (m) [i].kid))
       {
         if (*baseline < RC_Boxes (m) [i].baseline)
         {
             *baseline = RC_Boxes (m) [i].baseline;
         }
         
         if (*shadow < LabG_Shadow(RC_Boxes (m) [i].kid))
             *shadow = LabG_Shadow(RC_Boxes (m) [i].kid);
         if (*highlight < LabG_Highlight(RC_Boxes (m) [i].kid))
             *highlight = LabG_Highlight(RC_Boxes (m) [i].kid);
         if (*margin_top < LabG_MarginTop(RC_Boxes (m) [i].kid))
             *margin_top = LabG_MarginTop(RC_Boxes (m) [i].kid);
         if (*margin_height < LabG_MarginHeight(RC_Boxes (m) [i].kid))
             *margin_height = LabG_MarginHeight(RC_Boxes (m) [i].kid);
         if (*text_height < LabG_TextRect_height(RC_Boxes (m) [i].kid))
             *text_height = LabG_TextRect_height(RC_Boxes (m) [i].kid);
       }

       else if (XmIsText (RC_Boxes (m) [i].kid) || XmIsTextField (RC_Boxes (m) [i].kid))
       {
         XmBaselineMargins textMargins;

         if (*baseline < RC_Boxes (m) [i].baseline)
         {
             *baseline = RC_Boxes (m) [i].baseline;
         }
         SetOrGetTextMargins(RC_Boxes (m) [i].kid, XmBASELINE_GET, &textMargins);
         if (*shadow < textMargins.shadow)
             *shadow = textMargins.shadow;
         if (*highlight < textMargins.shadow)
             *highlight = textMargins.shadow;
         if (*margin_top < textMargins.margin_top)
             *margin_top = textMargins.margin_top;
         if (*text_height < textMargins.text_height)
             *text_height = textMargins.text_height;
       }

                                                    
    }

    *items_per = n / RC_NCol (m);       /* calc column size */

    if ((n % RC_NCol (m)) != 0)         /* some left overs */
        (*items_per)++;             /* add another row/col */
}


/*
 * Make sure that entries in the right most column/row extend all the 
 * way to the right/bottom edge of the row column widget.  This keeps 
 * 'dead space' in the row column widget to a minimum.  For single 
 * column widgets, the only column is the right most.  
 *
 */
static void 
#ifdef _NO_PROTO
adjust_last( m, start_i, w, h )
        XmRowColumnWidget m ;
        int start_i ;
        Dimension w ;
        Dimension h ;
#else
adjust_last(
        XmRowColumnWidget m,
        int start_i,
#if NeedWidePrototypes
        int w,
        int h )
#else
        Dimension w,
        Dimension h )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XmRCKidGeometry kg = RC_Boxes (m);
   XtWidgetGeometry *b;
   register Dimension subtrahend;

   for ( ; kg [start_i].kid != NULL; start_i++)
   {
      b = &(kg[start_i].box);

      if (IsVertical (m))
      {
         subtrahend = MGR_ShadowThickness(m) + RC_MarginW (m) + BX (b)
	     + Double (BBorder (b));

	 /* if w (rowcol width) is greater than subtrahend (the smallest
	  * width of the child, we'll guarantee at least a width of 1.
	  */
	 if (w > subtrahend) 
	     BWidth (b) = w-subtrahend;
      }
      else
      {
         subtrahend =  MGR_ShadowThickness(m) + RC_MarginH (m) + BY (b)
	     + Double (BBorder (b));

         /* When adjusting the last line, text and label widgets or gadgets, */
         /* use the extra height that is added differently. Text just adds  */
         /* it on whereas label tries to center it in the extra space.      */
         /* In order to make the baselines align again as a result of the   */
         /* above behavior,  Text's margin top has to be adjusted. */
	 if (h > subtrahend) 
         {
             Dimension m_top;

	     /* Check for underflow */
	     /* The difference is what it grows in height */
	     m_top = ((h-subtrahend) > BHeight(b)) ?
		((h-subtrahend) - BHeight (b)) : 0 ;

	     BHeight (b) = h-subtrahend;
         
             if (m_top && 
		(XmIsText(kg [start_i].kid) || XmIsTextField(kg [start_i].kid)))
             {
              kg [start_i].margin_top += m_top/2; /* Since labels center it */
             }
         }
      }
   }
}


/*
 * decide exactly the dimensions of the row column widget we will return to 
 * an asking caller based on the accumulated layout information.
 */
static void 
#ifdef _NO_PROTO
set_asking( m, m_width, m_height, b, max_x, max_y, x, y, w, h )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
        Dimension b ;
        Position max_x ;
        Position max_y ;
        Position x ;
        Position y ;
        Dimension w ;
        Dimension h ;
#else
set_asking(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height,    /* if 0 then caller's asking */
#if NeedWidePrototypes
        int b,
        int max_x,
        int max_y,
        int x,
        int y,
        int w,
        int h )
#else
        Dimension b,
        Position max_x,
        Position max_y,
        Position x,
        Position y,
        Dimension w,
        Dimension h )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    long iheight;
    long iwidth;

    if (IsVertical (m))             /* tell caller what he wants */
    {
        if (Asking (*m_width))
            *m_width =   x + w + b      /* right edge of last child */
                   + MGR_ShadowThickness(m)
                   + RC_MarginW (m);    /* plus margin on right */

        if (Asking (*m_height))
        {
            if (max_y < y) max_y = y;

            iheight = (long) max_y                /* last unused y */
                - (long) (RC_Spacing (m))         /* up by unused spacing */
                + (long) (MGR_ShadowThickness(m))
                + (long) (RC_MarginH (m)) ;       /* plus margin on bottom */

            if (iheight < 0)             /* this is a temporary fix */
                *m_height = 0;           /* to make sure we don't   */
            else                         /* compute a negative height */
                *m_height = (short) iheight; /*in an unsigned short   */
        }
    }
    else
    {
        if (Asking (*m_width))
        {
            if (max_x < x) max_x = x;

            iwidth = (long) max_x
                - (long) (RC_Spacing (m))
                + (long) (MGR_ShadowThickness(m))
                + (long) (RC_MarginW (m)) ;

            if (iwidth < 0)
                *m_width = 0;
            else
                *m_width = (short) iwidth ;
        }

        if (Asking (*m_height))
            *m_height = y + h + b
                + MGR_ShadowThickness(m)
                + RC_MarginH (m);
    }
}




/*
 * Decide where to put the help child.  He better be the last one
 * 'cuz we may trash the x, y's
 */
static void 
#ifdef _NO_PROTO
calc_help( m, m_width, m_height, b, max_x, max_y, x, y, w, h )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
        Dimension b ;
        Position max_x ;
        Position max_y ;
        Position *x ;
        Position *y ;
        Dimension w ;
        Dimension h ;
#else
calc_help(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height,    /* if 0 then caller's asking */
#if NeedWidePrototypes
        int b,
        int max_x,
        int max_y,
#else
        Dimension b,
        Position max_x,
        Position max_y,
#endif /* NeedWidePrototypes */
        Position *x,
        Position *y,
#if NeedWidePrototypes
        int w,
        int h )
#else
        Dimension w,
        Dimension h )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   register Dimension subtrahend;

   if (IsVertical (m))             /* glue to bottom edge of ... */
   {
      if (Asking (*m_height))
      {
	 if (RC_NCol (m) == 1)       /* just use max_y */
	     *y = max_y;
	 else                /* go up from max_y */
	 {
	     subtrahend = RC_Spacing (m) + h + b;
	     *y = (max_y > subtrahend) ? max_y - subtrahend : 0; 
	 }
      }
      else
      {   
	  subtrahend = MGR_ShadowThickness(m) + RC_MarginH (m) + h + b;
	  *y = (*m_height > subtrahend) ? *m_height - subtrahend : 0;
      }
   }
   else                    /* glue to right edge of ... */
   {
      if (Asking (*m_width))
      {
	 if (RC_NCol (m) == 1)
	     *x = max_x;
	 else
	 {
	     subtrahend = RC_Spacing (m) + w + b;
	     *x = (max_x > subtrahend) ? max_x - subtrahend : 0;
	 }
      }
      else
      {
	 subtrahend = MGR_ShadowThickness(m) + RC_MarginW (m) + w + b;
	 *x = (*m_width > subtrahend) ? *m_width - subtrahend : 0;
      }
   }
}


/*
 * figure out where all the children of a column style widget go.  The
 * border widths are already set.  
 *
 * In columnar mode, all heights and widths are identical.  They are the
 * size of the largest item.
 *
 * For vertical widgets the items are laid out in columns, going down the
 * first column and then down the second.  For horizonatal, think of the
 * columns as rows. 
 *
 * By convention incoming row column size can be zero, indicating a request
 * for preferred size, this means lay it out and record the needed size.
 *
 * NOTE: the layout is predicated on the help child being the last one since
 * it messes up the x, y for a following child.
 */
static void 
#ifdef _NO_PROTO
layout_column( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
layout_column(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height )   /* if 0 then caller's asking */
#endif /* _NO_PROTO */
{
    XmRCKidGeometry kg = RC_Boxes (m);
    XtWidgetGeometry *bx;
    Position x, y, max_x = 0, max_y = 0;
    int items_per_column,
        kid_i,
        child_i,                    /* which child we are doing */
        col_c   = 0,                /* items in col being done */
        start_i = 0;                /* index of first item in col */
    Dimension border, w, h, baseline, shadow, highlight, margin_top, margin_height, text_height;
    Dimension toc_height;
    Dimension new_height= 0;
    Dimension toc_b, b;

    toc_b = b = Double (RC_EntryBorder (m));

    if (RC_TearOffControl(m) && XtIsManaged(RC_TearOffControl(m)))
    {
       XmTearOffButtonWidget tw = (XmTearOffButtonWidget)RC_TearOffControl(m);

       if (!RC_EntryBorder(m) && kg[0].kid && XtIsWidget(kg[0].kid))
	    toc_b = Double(kg[0].kid->core.border_width);

       toc_height = 0;

       /* Remember!  If toc exists, it has the  first kid geo */
       for (start_i = 1;  kg[start_i].kid != NULL; start_i++)
          if (kg[start_i].box.height > toc_height)
             toc_height = kg[start_i].box.height;

       toc_height = toc_height >> 1;    /* 1/2 height of highest */

       toc_height = MAX( toc_height, 2 + toc_b + 
	  Double(((XmPrimitiveWidget)kg[0].kid)->primitive.shadow_thickness));

       /* Sync up the kid geo */
       /* Fix CR# 4778 */
       if (tw->label.recompute_size == True)
	 kg[0].box.height = toc_height;
       else
	 kg[0].box.height = toc_height = XtHeight(tw);
       kg[0].box.width = XtWidth(m);

       child_i = 1;
    }
    else
       toc_height = toc_b = child_i = 0;

    /* loc of first item */
    x = MGR_ShadowThickness(m) + RC_MarginW (m);
    y = MGR_ShadowThickness(m) + RC_MarginH (m) + toc_height + toc_b;

    get_info (m, &border, &w, &h, &items_per_column, &baseline, &shadow, &highlight, &margin_top, &margin_height, &text_height);
 
    if (!RC_EntryBorder(m) && kg[child_i].kid && XtIsWidget(kg[child_i].kid))
         b = Double(border);

    /* Loop through and find the new height, if any,  that the RowColumn */
    /* children need to grow to as a result of adjusting the baselines.  */

    if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m))
    {
        for (kid_i = 0; kg [kid_i].kid != NULL; kid_i++)
        ;
        BaselineAlignment(m, h, shadow, highlight, baseline, &new_height, 0, kid_i);

    }
    else if (AlignmentTop(m) || AlignmentBottom(m))
    {
        for (kid_i = 0; kg [kid_i].kid != NULL; kid_i++) /* determine number */
        ;						 /* of kids          */
        TopOrBottomAlignment(m, h, shadow, highlight, baseline, margin_top, margin_height,
                          text_height, &new_height, 0, kid_i);     
    }
    else if (AlignmentCenter(m))
    {
        for (kid_i = 0; kg [kid_i].kid != NULL; kid_i++) /* determine number */
        ;                                                /* of kids          */
        CenterAlignment(m, h, start_i, kid_i);
    }

    if (!new_height)
      new_height = h;

    for (; kg [child_i].kid != NULL; child_i++)
    {
        bx = &(kg[child_i].box);

        BWidth  (bx) = w;           /* all have same dimensions */

        if (AlignmentCenter(m))
         BHeight(bx) = h;
        
        if (++col_c > items_per_column)     /* start a new column */
        {
	   if (IsVertical (m))         /* calc loc of new column */
	   {
	      x += w + b + RC_Spacing (m);    /* to the right */

	      /*back at top of menu */
	      y = MGR_ShadowThickness(m) + RC_MarginH (m) + toc_height + toc_b;
	   }
	   else                /* calc loc of new row */
	   {
	      /* back to left edge */
	      x = MGR_ShadowThickness(m) + RC_MarginW (m);
	      /* down a row */
	      y += new_height + b + RC_Spacing (m);
	   }

	   col_c = 1;              /* already doing this one */
	   start_i = child_i;          /* record index */
        }

        if (IsHelp (m, ((Widget) kg[child_i].kid))) 
            calc_help (m, m_width, m_height, b, max_x, max_y, &x, &y, w, new_height);

        SetPosition (bx, x, y);         /* plunk him down */

        if (IsVertical (m))         /* get ready for next item */
            y += new_height + b + RC_Spacing (m);
        else
            x += w + b + RC_Spacing (m);

        if (max_y < y)
	    max_y = y;       /* record for use later */
        if (max_x < x)
	    max_x = x;
     }

     if (new_height > h)
     {
        for(kid_i = 0; kid_i < child_i; kid_i++)
        {
          bx = &(kg[kid_i].box);
          if (BHeight(bx) != new_height)
          {
             if ((XmIsLabel (kg [kid_i].kid)) ||
                 (XmIsLabelGadget (kg [kid_i].kid)) ||
                 (XmIsTextField(kg [kid_i].kid)) ||
                 (XmIsText(kg [kid_i].kid)))
             {
               if (!XmIsText(kg [kid_i].kid) && !XmIsTextField(kg [kid_i].kid))
                kg[kid_i].margin_bottom += new_height - kg[kid_i].box.height;
               BHeight(bx) = new_height;
             }
          }
        }
     }
   
    set_asking (m, m_width, m_height, b, max_x, max_y, x, y, w, new_height);
    
    if (toc_height)
    {
       kg[0].box.x = MGR_ShadowThickness(m) + RC_MarginW (m);
       kg[0].box.y = MGR_ShadowThickness(m) + RC_MarginH (m);
       kg[0].box.height = toc_height;
       kg[0].box.width = *m_width - Double(MGR_ShadowThickness(m) + 
	  RC_MarginW(m)) - toc_b; 
    }

    if (RC_AdjLast(m))
        adjust_last (m, start_i, *m_width, *m_height);
}


/*
 * do a vertical tight (non-column) layout.
 *
 * In a tight layout one dimension of the items is left alone and the other
 * is kept uniform.  In a vertical row column widgets, the widths of each child 
 * are uniform for each column, the heights are never changed.  In a horiz 
 * row column widget, the widths are never changed and the heights are kept 
 * uniform for each row.
 *
 * It gets messy w.r.t. the help child because we don't know if there will
 * be room in the last column/row for it.  If there isn't room then a whole
 * new column/row has to be added.
 *
 * NOTE: the layout is predicated on the help child being the last one since
 * it messes up the x, y for a following child.
 */
static void 
#ifdef _NO_PROTO
layout_vertical_tight( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
layout_vertical_tight(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height )   /* if 0 then caller's asking */
#endif /* _NO_PROTO */
{
    XmRCKidGeometry kg = RC_Boxes (m);
    XtWidgetGeometry *bx;
    Position x, y, max_y = 0;
    Dimension h = 0;
    Dimension w = 0;                /* widest item width in col */
    int child_i, start_i;
    Dimension toc_height;
    Dimension toc_b, b;
    Dimension border = 0;
    
    toc_b = b = Double (RC_EntryBorder (m));

    if (RC_TearOffControl(m) && XtIsManaged(RC_TearOffControl(m)))
    {
       XmTearOffButtonWidget tw = (XmTearOffButtonWidget)RC_TearOffControl(m);

       if (!RC_EntryBorder(m) && kg[0].kid && XtIsWidget(kg[0].kid))
	   toc_b = Double(kg[0].kid->core.border_width);

       toc_height = 0;

       /* Remember!  If toc exists, it has the  first kid geo */
       for (start_i = 1;  kg[start_i].kid != NULL; start_i++)
	  if (kg[start_i].box.height > toc_height)
	     toc_height = kg[start_i].box.height;

       toc_height = toc_height >> 1;	/* 1/2 height of highest */

       toc_height = MAX( toc_height, 2 + toc_b + 
	  Double(((XmPrimitiveWidget)kg[0].kid)->primitive.shadow_thickness));

       /* Sync up the kid geo */
       /* Fix CR# 4778 */
       if (tw->label.recompute_size == True)
	 kg[0].box.height = toc_height;
       else
	 kg[0].box.height = toc_height = XtHeight(tw);
       kg[0].box.width = XtWidth(m);

       start_i = child_i = 1;
    }
    else
       toc_b = toc_height = start_i = child_i = 0;

    /* first item location */
    x = MGR_ShadowThickness(m) + RC_MarginW (m);
    y = MGR_ShadowThickness(m) + RC_MarginH (m) + toc_height + toc_b;

    for (; kg [child_i].kid != NULL; child_i++)
    {
       bx = &(kg[child_i].box);
       if (!RC_EntryBorder(m) && kg[child_i].kid &&
	   XtIsWidget(kg[child_i].kid))
         b = Double(kg[child_i].kid->core.border_width);

       h = BHeight (bx) + b;           /* calc this item's height */

       if (((y + h) > *m_height) && 
	   ( ! Asking (*m_height)) &&
	   (child_i))
       {                   /* start new column */
	  while (start_i < child_i)
	      kg[start_i++].box.width = w;    /* set uniform width */

	  x += w + Double(border) 
	      + MGR_ShadowThickness(m)
		  + RC_MarginW (m);       /* go right and */
            
	  y = MGR_ShadowThickness(m)
	      + RC_MarginH (m) + toc_height + toc_b;  /* back to top of menu */

	  w = BWidth (bx);            /* reset for new column */

          if (kg[child_i].kid && XtIsWidget(kg[child_i].kid))
            border = kg[child_i].kid->core.border_width;
          else
            border = ((XmGadget)kg[child_i].kid)->rectangle.border_width;
       }

       if (IsHelp (m, ((Widget) kg[child_i].kid))) 
	   calc_help (m, m_width, m_height, b, 0, max_y, &x, &y, w, h);

       SetPosition (bx, x, y);

       if (w < BWidth (bx))
          w = BWidth (bx);

       if (kg[child_i].kid && XtIsWidget(kg[child_i].kid))
       {
         if (border < kg[child_i].kid->core.border_width)
           border = kg[child_i].kid->core.border_width;
       }
       else
       {
         if (border < ((XmGadget)kg[child_i].kid)->rectangle.border_width)
           border = ((XmGadget)kg[child_i].kid)->rectangle.border_width;
       }

       y += h + RC_Spacing (m);        /* loc of next item */

       if (max_y < y)
	   max_y = y;       /* record for use later */
    }

    set_asking (m, m_width, m_height, Double(border), 0, max_y, x, y, w, h);

    /* Set toc width to the width of the pane */
    if (toc_height)
    {
       kg[0].box.x = MGR_ShadowThickness(m) + RC_MarginW (m);
       kg[0].box.y = MGR_ShadowThickness(m) + RC_MarginH (m);
       kg[0].box.height = toc_height;
       kg[0].box.width = *m_width - Double(MGR_ShadowThickness(m) + 
	  RC_MarginW(m)) - toc_b;
    }

    if (RC_AdjLast(m))
        adjust_last (m, start_i, *m_width, *m_height);
    else
	while (start_i < child_i)
	    kg[start_i++].box.width = w;    /* set uniform width */
}


static void 
#ifdef _NO_PROTO
layout_horizontal_tight( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
layout_horizontal_tight(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height )   /* if 0 then caller's asking */
#endif /* _NO_PROTO */
{
   XmRCKidGeometry kg = RC_Boxes (m);
   XtWidgetGeometry *bx;
   Position x, y, max_x = 0;
   Dimension w = 0;
   Dimension h = 0;                   /* tallest item height in row */
   Dimension new_height = 0;
   Dimension baseline = 0;
   Dimension shadow = 0;
   Dimension highlight = 0;
   Dimension margin_height = 0;
   Dimension margin_top = 0;
   Dimension text_height = 0;
   Dimension border = 0;
   int child_i, start_i;                /* index of first item in row */
   Dimension toc_height;
   Dimension toc_b, b;
   
   toc_b = b = Double (RC_EntryBorder (m));

   if (RC_TearOffControl(m) && XtIsManaged(RC_TearOffControl(m)))
   {
      XmTearOffButtonWidget tw = (XmTearOffButtonWidget)RC_TearOffControl(m);

      if (!RC_EntryBorder(m) && kg[0].kid && XtIsWidget(kg[0].kid))
	  toc_b = Double(kg[0].kid->core.border_width);

      toc_height = 0;

      /* Remember!  If toc exists, it has the  first kid geo */
      for (start_i = 1;  kg[start_i].kid != NULL; start_i++)
	 if (kg[start_i].box.height > toc_height)
	    toc_height = kg[start_i].box.height;

      toc_height = toc_height >> 2;    /* 1/4 height of highest */

      toc_height = MAX( toc_height, 2 + toc_b + 
	 Double(((XmPrimitiveWidget)kg[0].kid)->primitive.shadow_thickness));

      /* Sync up the kid geo */
      /* Fix CR# 4778 */
      if (tw->label.recompute_size == True)
	kg[0].box.height = toc_height;
      else
	kg[0].box.height = toc_height = XtHeight(tw);
      kg[0].box.width = XtWidth(m);

      start_i = child_i = 1;
   }
   else
      toc_b = toc_height = start_i = child_i = 0;

   /* first item location */
   x = MGR_ShadowThickness(m) + RC_MarginW (m);
   y = MGR_ShadowThickness(m) + RC_MarginH (m) + toc_height + toc_b;

   for (; kg [child_i].kid != NULL; child_i++)
   {
      bx = &(kg[child_i].box);
      if (!RC_EntryBorder(m) && kg[child_i].kid &&
	  XtIsWidget(kg[child_i].kid))
	  b = Double(kg[child_i].kid->core.border_width);

      w = BWidth (bx) + b;            /* item's width */

      if (((x + w) > *m_width) && 
	  ( ! Asking (*m_width)) &&
	  (child_i))
      {                   /* start a new row */

         if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m))
           BaselineAlignment(m, h, shadow, highlight, baseline, &new_height, start_i, child_i);
         else if (AlignmentTop(m) || AlignmentBottom(m))
           TopOrBottomAlignment(m, h, shadow, highlight, baseline, margin_top, margin_height,
                          text_height, &new_height, start_i, child_i);
         else if (AlignmentCenter(m))
           CenterAlignment(m, h, start_i, child_i);

         if (new_height > h)
         {
           while (start_i < child_i)
           {
             if (kg[start_i].box.height != new_height)
             {
               if ((XmIsLabel (kg [start_i].kid))  ||
                   (XmIsLabelGadget (kg [start_i].kid)) ||
                   (XmIsText(kg [start_i].kid)) ||
                   (XmIsTextField(kg [start_i].kid)))
               if (!XmIsText(kg [start_i].kid) && !XmIsTextField(kg [start_i].kid))
                kg[start_i].margin_bottom += new_height - kg[start_i].box.height;
               kg[start_i].box.height = new_height;
             }
             start_i++;
           }
           h = new_height;
         }
      
         start_i = child_i;
         
	 x = MGR_ShadowThickness(m) 
	     + RC_MarginW (m);       /* left edge of menu */

	 y += h + Double(border) + MGR_ShadowThickness(m) 
	     + RC_MarginH (m);       /* down to top of next row */

	 h = BHeight (bx);           /* reset for this row */
         new_height = 0;
         baseline = kg[child_i].baseline;
         if (kg[child_i].kid && XtIsWidget (kg[child_i].kid))
          border = kg[child_i].kid->core.border_width;
         else if (XmIsGadget (kg[child_i].kid))
          border = ((XmGadget)kg[child_i].kid)->rectangle.border_width;

         if (XmIsLabel (kg[child_i].kid))
         {
          shadow = Lab_Shadow(kg[child_i].kid);
          highlight = Lab_Highlight(kg[child_i].kid);
          margin_top = Lab_MarginTop(kg[child_i].kid);
          margin_height = Lab_MarginHeight(kg[child_i].kid);
          text_height = Lab_TextRect_height(kg[child_i].kid);
         }
         else if (XmIsLabelGadget (kg[child_i].kid))
         {
          shadow = LabG_Shadow(kg[child_i].kid);
          highlight = LabG_Highlight(kg[child_i].kid);
          margin_top = LabG_MarginTop(kg[child_i].kid);
          margin_height = LabG_MarginHeight(kg[child_i].kid);
          text_height = LabG_TextRect_height(kg[child_i].kid);
         }
         else if (XmIsText (kg[child_i].kid) || XmIsTextField (kg[child_i].kid))
         {
           XmBaselineMargins textMargins;

           SetOrGetTextMargins(kg[child_i].kid, XmBASELINE_GET, &textMargins);
           shadow = textMargins.shadow;
           highlight = textMargins.highlight;
           margin_top = textMargins.margin_top;
           text_height = textMargins.text_height;
         }
      }
      
      if (IsHelp (m, ((Widget) kg[child_i].kid))) 
	  calc_help (m, m_width, m_height, b, max_x, 0, &x, &y, w, h);
      
      SetPosition (bx, x, y);

      if ((XmIsLabel (kg[child_i].kid)) || (XmIsLabelGadget (kg[child_i].kid)) ||
          (XmIsText (kg[child_i].kid)) || (XmIsTextField (kg[child_i].kid)))
      {
       if (baseline < (kg[child_i].baseline))
        {
            baseline = kg[child_i].baseline;
        }

      }

      if (h < BHeight (bx))
      {
	  h = BHeight (bx);
      }

      if (kg[child_i].kid && XtIsWidget (kg[child_i].kid))
        if (border < kg[child_i].kid->core.border_width)
           border = kg[child_i].kid->core.border_width;
      else if (XmIsGadget (kg[child_i].kid))
        if (border < ((XmGadget)kg[child_i].kid)->rectangle.border_width)
             border = ((XmGadget)kg[child_i].kid)->rectangle.border_width;

      if (XmIsLabel (kg[child_i].kid))
      {
        if (shadow <  Lab_Shadow(kg[child_i].kid))
           shadow = Lab_Shadow(kg[child_i].kid);
        if (highlight <  Lab_Highlight(kg[child_i].kid))
           highlight = Lab_Highlight(kg[child_i].kid);
        if (margin_top < Lab_MarginTop(kg[child_i].kid))
           margin_top = Lab_MarginTop(kg[child_i].kid);
        if (margin_height < Lab_MarginHeight(kg[child_i].kid))
           margin_height = Lab_MarginHeight(kg[child_i].kid);
        if (text_height < Lab_TextRect_height(kg[child_i].kid))
           text_height = Lab_TextRect_height(kg[child_i].kid);
      }
      else if (XmIsLabelGadget (kg[child_i].kid))
      {
        if (shadow <  LabG_Shadow(kg[child_i].kid))
           shadow = LabG_Shadow(kg[child_i].kid);
        if (highlight <  LabG_Highlight(kg[child_i].kid))
           highlight = LabG_Highlight(kg[child_i].kid);
        if (margin_top < LabG_MarginTop(kg[child_i].kid))
           margin_top = LabG_MarginTop(kg[child_i].kid);
        if (margin_height < LabG_MarginHeight(kg[child_i].kid))
           margin_height = LabG_MarginHeight(kg[child_i].kid);
        if (text_height < LabG_TextRect_height(kg[child_i].kid))
           text_height = LabG_TextRect_height(kg[child_i].kid);
      }
      else if (XmIsText (kg[child_i].kid) || XmIsTextField (kg[child_i].kid))
      {
        XmBaselineMargins textMargins;

        SetOrGetTextMargins(kg[child_i].kid, XmBASELINE_GET, &textMargins);
        if (shadow < textMargins.shadow)
           shadow = textMargins.shadow;
        if (highlight <textMargins.highlight)
           highlight = textMargins.highlight;
        if (margin_top < textMargins.margin_top)
           margin_top = textMargins.margin_top;
        if (text_height < textMargins.text_height)
           text_height = textMargins.text_height;
      }
      
      x += w + RC_Spacing (m);        /* loc of next item */

      if (max_x < x)
	  max_x = x;       /* record for use later */
   }
   
   if (toc_height)
   {
       kg[0].box.x = MGR_ShadowThickness(m) + RC_MarginW (m);
       kg[0].box.y = MGR_ShadowThickness(m) + RC_MarginH (m);
       kg[0].box.height = toc_height;
       kg[0].box.width = *m_width - Double(MGR_ShadowThickness(m) + 
	  RC_MarginW(m)) - toc_b;
   }

   if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m))
        BaselineAlignment(m, h, shadow, highlight, baseline, &new_height, start_i, child_i);
   else if (AlignmentTop(m) || AlignmentBottom(m))
        TopOrBottomAlignment(m, h, shadow, highlight, baseline, margin_top, margin_height,
                          text_height, &new_height, start_i, child_i);
   else if (AlignmentCenter(m))
        CenterAlignment(m, h, start_i, child_i);

   if (new_height > h)
   {
        while (start_i < child_i)
        {
            bx = &(kg[start_i].box);
            if (BHeight(bx) != new_height)
            {
             if ((XmIsLabel (kg [start_i].kid)) ||
                 (XmIsLabelGadget (kg [start_i].kid)) ||
                 (XmIsText (kg [start_i].kid)) ||
                 (XmIsTextField (kg [start_i].kid)))
              if (!XmIsText (kg [start_i].kid) && !XmIsTextField (kg [start_i].kid))
                 kg[start_i].margin_bottom += new_height - kg [start_i].box.height;
             BHeight(bx) = new_height;
            }
            start_i++;
        }
   }

   if (new_height > h)
    set_asking (m, m_width, m_height, Double(border), max_x, 0, x, y, w, new_height);
   else
    set_asking (m, m_width, m_height, Double(border), max_x, 0, x, y, w, h);

   if (RC_AdjLast(m))
       adjust_last (m, start_i, *m_width, *m_height);
   else
       while (start_i < child_i)
       {
         if (new_height > h)
           kg[start_i++].box.height = new_height;
         else
	   kg[start_i++].box.height = h;   /* set uniform height */
       }
}


static void 
#ifdef _NO_PROTO
layout_vertical( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
layout_vertical(
        XmRowColumnWidget m,
        Dimension *m_width,         /* if 0 then caller's asking */
        Dimension *m_height )       /* if 0 then caller's asking */
#endif /* _NO_PROTO */
{
   if (PackColumn (m))
       layout_column (m, m_width, m_height);
   else
       layout_vertical_tight (m, m_width, m_height);
}

static void 
#ifdef _NO_PROTO
layout_horizontal( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
layout_horizontal(
        XmRowColumnWidget m,
        Dimension *m_width,     /* if 0 then caller's asking */
        Dimension *m_height )   /* if 0 then caller's asking */
#endif /* _NO_PROTO */
{
   if (PackColumn (m))
       layout_column (m, m_width, m_height);
   else
       layout_horizontal_tight (m, m_width, m_height);
}

/*
 * wrap a box around the entries, used with packing mode of none.
 *
 * we ignore negative positioning, ie. only worry about being wide enough
 * for the right edge of the rightmost entry (similarly for height)
 */
static void 
#ifdef _NO_PROTO
bound_entries( m, m_width, m_height )
        XmRowColumnWidget m ;
        Dimension *m_width ;
        Dimension *m_height ;
#else
bound_entries(
        XmRowColumnWidget m,
        Dimension *m_width,
        Dimension *m_height )
#endif /* _NO_PROTO */
{
    XtWidgetGeometry *b;
    XmRCKidGeometry kg = RC_Boxes (m);
    int i;
    Dimension w, max_w = 0, max_h = 0;
    Dimension toc_height;
    Dimension toc_b, bw;
    short temp;
    
    toc_b = bw = Double(RC_EntryBorder(m));

    if (RC_TearOffControl(m) && XtIsManaged(RC_TearOffControl(m)))
    {
       XmTearOffButtonWidget tw = (XmTearOffButtonWidget)RC_TearOffControl(m);

       if (!RC_EntryBorder(m) && kg[0].kid && XtIsWidget(kg[0].kid))
	    toc_b = Double(kg[0].kid->core.border_width);

       toc_height = 0;

       /* Remember!  If toc exists, it has the  first kid geo */
       for (i = 1;  kg[i].kid != NULL; i++)
          if (kg[i].box.height > toc_height)
             toc_height = kg[i].box.height;

       toc_height = toc_height >> 2;    /* 1/4 height of highest */

       toc_height = MAX( toc_height, 2 + toc_b + 
	  Double(((XmPrimitiveWidget)kg[0].kid)->primitive.shadow_thickness));

       /* Sync up the kid geo */
       /* Fix CR# 4778 */
       if (tw->label.recompute_size == True)
	 kg[0].box.height = toc_height;
       else
	 kg[0].box.height = toc_height = XtHeight(tw);
       kg[0].box.width = XtWidth(m);

       i = 1;
    }
    else
       toc_height = i = 0;

    for (; kg [i].kid != NULL; i++)
      {
        b = &(kg[i].box);
        if (!RC_EntryBorder(m) && kg[i].kid && XtIsWidget(kg[i].kid))
             bw = Double(kg[i].kid->core.border_width);

        if (Asking (*m_width))
        {
	    /* be careful about negative positions */
            w = BWidth (b) + bw;
            temp = ((short)w) + BX (b);
            if (temp <= 0)
                w = 1;
            else
                w = (Dimension) temp;

            if (w > max_w)
		max_w = w;
        }

        if (Asking (*m_height))
        {
            /* be careful about negative positions */
            w = BHeight (b) + Double (bw);
            temp = ((short)w) + BY (b);
            if (temp <= 0)
                w = 1;
            else
                w = (Dimension) temp;

            if (w > max_h)
		max_h = w;
        }
    }

    if (toc_height)
    {
       kg[0].box.x = MGR_ShadowThickness(m) + RC_MarginW (m);
       kg[0].box.y = MGR_ShadowThickness(m) + RC_MarginH (m);
       kg[0].box.height = toc_height;
       kg[0].box.width = max_w - Double(MGR_ShadowThickness(m) + 
	  RC_MarginW(m)) - toc_b;
    }

    if (Asking (*m_width))
	*m_width  = max_w;
    if (Asking (*m_height))
	*m_height = max_h;
}

static void
#ifdef _NO_PROTO
find_largest_option_selection( submenu, c_width, c_height )
        XmRowColumnWidget submenu ;
        Dimension *c_width ;
        Dimension *c_height ;
#else
find_largest_option_selection( 
	XmRowColumnWidget submenu, 
	Dimension *c_width, 
	Dimension *c_height )
#endif
{
   int i;

   if (submenu)
   {
      for(i=0; i < submenu->composite.num_children; i++)
      {
	 if (XtIsManaged(submenu->composite.children[i]))
	 {
	    Widget child = submenu->composite.children[i];

	    if (XtIsManaged(child))
	    {
	       if (XmIsCascadeButton(child))
	       {
		  find_largest_option_selection((XmRowColumnWidget)
                                         CB_Submenu(child), c_width, c_height);
	       }
	       else if (XmIsCascadeButtonGadget(child))
	       {
		  find_largest_option_selection((XmRowColumnWidget)
                                        CBG_Submenu(child), c_width, c_height);
	       }
	       else
	       {
		  /* The entire size of the largest menu
		   * item is used instead of only its TextRect.  This may
		   * result in large expanses of label white space when items 
		   * utilize left and right margins, shadow, or accelerator 
		   * text - but the glyph will be visible when the submenu is
		   * posted!
		   */
		   if (XmIsMenuShell(XtParent(submenu)))
		   {
		       if (XtWidth(child) > *c_width)
			   *c_width = XtWidth(child);
		       if (XtHeight(child) > *c_height)
			   *c_height = XtHeight(child);
		   }
		   
		   /*
		    * must be a torn pane.  Don't rely on its dimensions
		    * since it may be stretched in the tear off so that
		    * the label string fits into the titlebar
		    */
		   else
		   {
		       XtWidgetGeometry preferred;
		       
		       XtQueryGeometry (child, NULL, &preferred);
		       if (preferred.width > *c_width)
			   *c_width = preferred.width;
		       if (preferred.height > *c_height)
			   *c_height = preferred.height;
		       
		   }
	       }
	    }
	 }
      }
   }
}

/* routine called for all types except option menus */

static void 
#ifdef _NO_PROTO
Layout( m, w, h )
        XmRowColumnWidget m ;
        Dimension *w ;
        Dimension *h ;
#else
Layout(
        XmRowColumnWidget m,
        Dimension *w,
        Dimension *h )
#endif /* _NO_PROTO */
{
   if (IsVertical (m))
       layout_vertical (m, w, h);
   else
       layout_horizontal (m, w, h);
}


/*
 * Routine used to determine the size of the option menu or to layout
 * the option menu given the current size.  The boolean calcMenuDimension
 * indicates whether the dimensions of the menu should be recalculated.
 * This is true when called from think_about_size and false when called
 * from AdaptToSize.
 *
 * This combines the two routines from Motif1.1: think_about_option_size
 * and option_layout.  Also new for Motif 1.2, the instigator is considered.
 * If the instigator is the label or the cascabebuttongadget, then the
 * dimensions are honored if they are large enough.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
OptionSizeAndLayout ( menu, width, height, instigator, request,
		     calcMenuDimension )
        register XmRowColumnWidget menu ;
        Dimension *width ;
        Dimension *height ;
        Widget instigator ;
        XtWidgetGeometry *request ;
        Boolean calcMenuDimension ;
#else
OptionSizeAndLayout (
        register XmRowColumnWidget menu,
        Dimension *width,
        Dimension *height,
        Widget instigator,
        XtWidgetGeometry *request,
#if NeedWidePrototypes
        int calcMenuDimension )
#else
        Boolean calcMenuDimension )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   XtWidgetGeometry    *label_box, *button_box;
   Dimension c_width; 
   Dimension c_height;
   register XmRowColumnWidget p = (XmRowColumnWidget) RC_OptionSubMenu(menu);
   XmCascadeButtonGadget cb = 
      (XmCascadeButtonGadget)XmOptionButtonGadget( (Widget) menu);

   /*
    * if this is being destroyed, don't get new dimensions.  This routine
    * assumes that cb is valid.
    */

   if (menu->core.being_destroyed)
   {
       if (calcMenuDimension)
       {
           *width = XtWidth(menu);
           *height = XtHeight(menu);
       }
       return;
   }

   /* Find the interesting boxes */

   if (!XtIsManaged(XmOptionLabelGadget( (Widget) menu))) {
       label_box = NULL;
       button_box = &(RC_Boxes(menu)[0].box);
   } else {
       label_box = &(RC_Boxes(menu)[0].box);
       button_box = &(RC_Boxes(menu)[1].box);
   }


   if (p)
   {
      c_width = c_height = 0;

      find_largest_option_selection( p, &c_width, &c_height );

      c_width += Double(G_HighlightThickness(cb)) + G_ShadowThickness(cb) +
	         LabG_MarginRight(cb) + Double(MGR_ShadowThickness(p))  -
		 /* magic value */ 2;
      c_height += Double(G_HighlightThickness(cb)) + LabG_MarginTop(cb)
		 + LabG_MarginBottom(cb);
      
      /* allow settings in cbg to be honored if greater than best size */
      if (instigator == (Widget) cb)
      {
	  if ((request->request_mode & CWHeight) &&
	      (request->height > c_height))
	  {
	      c_height = request->height;
	  }
	  if ((request->request_mode & CWWidth) &&
	      (request->width > c_width))
	  {
	      c_width = request->width;
	  }
      }
      BWidth(button_box) = c_width;
      BHeight(button_box) = c_height;
  }
   else
   {
      /* Option menu draws a toggle indicator with a childless submenu */
      c_width = BWidth(button_box);
      c_height = BHeight(button_box);
   }

   /* treat separate the case where the label is unmanaged */
   if (!XtIsManaged(XmOptionLabelGadget( (Widget) menu))) {

       if (!calcMenuDimension &&  c_height > XtHeight(menu))
	   c_height = XtHeight(menu) - 2*RC_MarginH(menu);

       if (!calcMenuDimension && c_width > XtWidth (menu))
	   c_width = XtWidth(menu) - 2*RC_MarginW(menu);
      
       BWidth(button_box) = c_width;
       BHeight(button_box) = c_height;

       BX(button_box) = RC_MarginW(menu);
       BY(button_box) = RC_MarginH(menu);

       if (calcMenuDimension)
	   {
	       *width = c_width + 2*RC_MarginW(menu);
	       *height = c_height + 2*RC_MarginH(menu);
	   }
       return ;
   }

   if (IsHorizontal(menu))
   {
      /*
       * Set the height to the highest of the two but if calcMenuDimension
       * is false, limit it to the size of the option menu
       */

      if (BHeight(label_box) > c_height)
	  c_height = BHeight(label_box);

      if (!calcMenuDimension &&  c_height > XtHeight(menu))
	  c_height = XtHeight(menu) - 2*RC_MarginH(menu);

      BHeight(label_box) = c_height;
      BHeight(button_box) = c_height;

      /* The label box is placed at... */

      BX(label_box) = RC_MarginW(menu);
      BY(label_box) = RC_MarginH(menu);

      /* The button is placed just next to the label */

      BX(button_box) = BX(label_box) + BWidth(label_box) + RC_Spacing(menu);
      BY(button_box) = RC_MarginH(menu);

      if (calcMenuDimension)
      {
	  *width = BX(button_box) + c_width + RC_MarginW(menu);
	  *height = c_height + 2*RC_MarginH(menu);
      }
   }
   else	/* is vertical menu */
   {
      /*
       * Set the height to the highest of the two but if calcMenuDimension
       * is false, limit it to the size of the option menu
       */
      if (BWidth(label_box) > c_width)
	  c_width = BWidth(label_box);

      if (!calcMenuDimension && c_width > XtWidth (menu))
	  c_width = XtWidth(menu) - 2*RC_MarginW(menu);
      
      BWidth(label_box) = c_width;
      BWidth(button_box) = c_width;

      /* The label box is placed at... */
      BX(label_box) = RC_MarginW(menu);
      BY(label_box) = RC_MarginH(menu);

      /* The button is placed just below the label */
      BX(button_box) = RC_MarginW(menu);
      BY(button_box) = BY(label_box) + BHeight(label_box) + RC_Spacing(menu);

      if (calcMenuDimension)
      {
	  *width = c_width + 2*RC_MarginW(menu);
	  *height = BY(button_box) + c_height + RC_MarginH(menu);
      }
   }
}

   
/**************************************************************************
 *
 * class support procedures
 */

/*
 * position the row column widget where it wants to be; normally, this
 * is used only for popup or pulldown menupanes.
 */
static void 
#ifdef _NO_PROTO
PositionMenu( m, event )
        register XmRowColumnWidget m ;
        XButtonPressedEvent *event ;
#else
PositionMenu(
        register XmRowColumnWidget m,
        XButtonPressedEvent *event )
#endif /* _NO_PROTO */
{
    XmRowColumnWidget root;
    XmCascadeButtonWidget p;

    if (m == NULL) 
       return;

    switch (m->row_column.type)
    {
        case XmMENU_OPTION:
        case XmMENU_BAR:
        case XmWORK_AREA:
            break;

        /*
         * remaining cases take advantage of the fact that positioning of
         * a popup shells' only child is not normal.  Just change the widget
         * itself, when popped up the shell will put itself at this location
         * and move the child to 0,0.  Saves a bunch of thrashing about.
         */

        case XmMENU_POPUP:          /* position near mouse */
            XtX (m) = event->x_root;
            XtY (m) = event->y_root;
            RC_SetWidgetMoved (m, TRUE);
            break;

        case XmMENU_PULLDOWN:
            p = (XmCascadeButtonWidget) m->row_column.cascadeBtn;

            if (p != NULL) 
               root = (XmRowColumnWidget) XtParent (p);
            else
               return;

            if (! XmIsRowColumn(root)) 
               root = NULL;

            LocatePulldown(
                root,           /* menu above */
                p,              /* pulldown linking the two */
                m,              /* menu to position */
                (XEvent *) event);         /* event causing pulldown */

#if 0
            if ((XtX (m) == XtX (XtParent(m))) &&
                (XtY (m) == XtY (XtParent(m))))
            {
               XtX (m) = 0;
	       XtY (m) = 0;
            }
            else
#endif
               RC_SetWidgetMoved (m, TRUE);

            break;
    }
}

static Widget 
#ifdef _NO_PROTO
find_first_managed_child( m, first_button )
        CompositeWidget m ;
        Boolean first_button ;
#else
find_first_managed_child(
        CompositeWidget m,
#if NeedWidePrototypes
        int first_button )
#else
        Boolean first_button )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register Widget *kid;
    register int i = 0;
    register int n;

    if (!m)
       return(NULL);

    kid = m->composite.children;
    n = m->composite.num_children;

    while( (i < n) && 
	   ((*kid)->core.being_destroyed ||
	    (!XtIsManaged(*kid) || 
	    (first_button && 
	     !(XmIsPushButton(*kid) ||
	       XmIsPushButtonGadget(*kid) ||
	       XmIsToggleButton(*kid) ||
	       XmIsToggleButtonGadget(*kid)) 
	   ))) )
        kid++, i++;

    if (i >= n)
        return(NULL);
    else
        return(*kid);
}

/*
 * Called by PrepareToCascade() and ChildsActivateCallback().  
 * This will allow XmGetPostedFromWidget() to be called from map and activate
 * or entrycallback callbacks.
 */
static void 
#ifdef _NO_PROTO
GetLastSelectToplevel( submenu )
        XmRowColumnWidget submenu ;
#else
GetLastSelectToplevel(
        XmRowColumnWidget submenu )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget topLevel;
   XmMenuState mst = _XmGetMenuState((Widget)submenu);

   /* "_XmLastSelectToplevel" only set for accelerators via
    * KeyboardInputHandler().
    */
   if (IsPopup(submenu))
   { 
      /* A popup's lastSelectToplevel is always itself!  The cascadeBtn
       * indicates the postedFromWidget.
       */
      if (mst->RC_LastSelectToplevel)
	 RC_CascadeBtn(submenu) = mst->RC_LastSelectToplevel;
   }
   else
   {
      if (mst->RC_LastSelectToplevel)
	 topLevel = (XmRowColumnWidget)mst->RC_LastSelectToplevel;
      else
      {
	 _XmGetActiveTopLevelMenu ((Widget) submenu, (Widget *) &topLevel);
	 /* If the active topLevel menu is torn, then use it's saved away
	  * lastSelectTopLevel from tear-time.
	  */
	 if (RC_TearOffActive(topLevel))
	    topLevel = (XmRowColumnWidget) 
	       topLevel->row_column.tear_off_lastSelectToplevel;
      }
      
      submenu->row_column.lastSelectToplevel = (Widget) topLevel;
   }
}

/*
 * called by cascadebutton (or CBG) before cascading a menu.  The interface
 * is through the menuProcs handle.
 */
static void 
#ifdef _NO_PROTO
PrepareToCascade( cb, submenu, event )
        Widget cb ;
        XmRowColumnWidget submenu ;
        XEvent *event ;
#else
PrepareToCascade(
        Widget cb,
        XmRowColumnWidget submenu,
        XEvent *event )
#endif /* _NO_PROTO */
{
   RC_CascadeBtn(submenu) = cb;
   RC_PostButton(submenu) = RC_PostButton(XtParent(cb));
   RC_PostModifiers(submenu) = RC_PostModifiers(XtParent(cb));
   RC_PostEventType(submenu) = RC_PostEventType(XtParent(cb));
   RC_PopupPosted(XtParent(cb)) = XtParent(submenu);

   if (IsOption(XtParent(cb)))
       RC_MemWidget(submenu) = RC_MemWidget(XtParent(cb));
   
   PositionMenu (submenu, (XButtonPressedEvent *) event);

   /*
    * Set submenu's lastselectToplevel in case map callback needs info
    */
   GetLastSelectToplevel(submenu);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
LocatePulldown( root, p, m, event )
        XmRowColumnWidget root ;
        XmCascadeButtonWidget p ;
        XmRowColumnWidget m ;
        XEvent *event ;
#else
LocatePulldown(
        XmRowColumnWidget root,
        XmCascadeButtonWidget p,
        XmRowColumnWidget m,
        XEvent *event )
#endif /* _NO_PROTO */
{   
    Position x, y, x1, y1;

    if (root == NULL) 
      return;

    x = y = 0;                  /* punt, don't know */

    if (IsOption (root))           /* near hot spot */
    {                              /* of option button (p) */
        if (! XtIsRealized( (Widget) m))  
           XtRealizeWidget( (Widget) m);

        x = RC_MemWidget(m) ? 
	   (G_HighlightThickness(p) + MGR_ShadowThickness(m) -
	   XtX(RC_MemWidget(m))) : G_HighlightThickness(p);

        y = RC_MemWidget(m) ? (Half (XtHeight (p)) - (XtY(RC_MemWidget(m)) +
            Half(XtHeight(RC_MemWidget(m))))) : XtY(p);
    }   

    else if (IsBar (root))  /* aligned with left edge of */
    {                       /* cascade button, below cascade button */
        x = 0;
        y = XtHeight (p);
    }

    else if (XmIsCascadeButtonGadget(p) && CBG_HasCascade(p))
    {
	/* gadget; have to use parent as base for coordinates */
        x = XtX(p) + CBG_Cascade_x(p) + CBG_Cascade_width(p);
        y = XtY(p) + CBG_Cascade_y(p);

	/* cast only to eliminate warning */
	p = (XmCascadeButtonWidget)XtParent(p);

    }

    else if (XmIsCascadeButton(p) && CB_HasCascade(p))
    {
        x = CB_Cascade_x(p) + CB_Cascade_width(p);
        y = CB_Cascade_y(p);
    }

    /*
     * Take these co-ords which are in the cascade button 
     * widget's co-ord system and convert them to the root 
     * window co-ords.
     */
    XtTranslateCoords( (Widget) p, x, y, &x1, &y1);

    XtX (m) = x1;
    XtY (m) = y1;
}


static void 
#ifdef _NO_PROTO
think_about_size( m, w, h, instigator, request )
        register XmRowColumnWidget m ;
        Dimension *w ;
        Dimension *h ;
        Widget instigator ;
        XtWidgetGeometry *request ;
#else
think_about_size(
        register XmRowColumnWidget m,
        Dimension *w,
        Dimension *h,
        Widget instigator,
        XtWidgetGeometry *request )
#endif /* _NO_PROTO */
{
    if (IsOption(m))
    {
	OptionSizeAndLayout(m, w, h, instigator, request, TRUE);
        return;
    }

    if (!RC_ResizeWidth(m))  *w = XtWidth  (m);
    if (!RC_ResizeHeight(m)) *h = XtHeight (m);

    if (PackNone (m))
        bound_entries (m, w, h);
    else
        Layout (m, w, h);

    if (!RC_ResizeHeight(m) && !RC_ResizeWidth(m))
        return;

    if (*w < resource_min_width)
	*w = resource_min_width;
    if (*h <  resource_min_height)
	*h = resource_min_height;
}

static void 
#ifdef _NO_PROTO
PreferredSize( m, w, h )
        XmRowColumnWidget m ;
        Dimension *w ;
        Dimension *h ;
#else
PreferredSize(
        XmRowColumnWidget m,
        Dimension *w,
        Dimension *h )
#endif /* _NO_PROTO */
{
   Widget *q;
   int i;
   Dimension * baselines;
   int line_count;
   Dimension y;

   if ((!IsOption(m)) && ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
       (PackTight(m) && IsHorizontal(m))))
   {
     if ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
         (PackTight(m) && IsHorizontal(m)))
     {
      if (*h == 0)
      {
       ForManagedChildren(m, i, q) /* reset Top and Bottom Margins that were */
       {                           /* set for vertical Alignment to work     */
          if (XmIsLabel(*q))
          {
           Lab_MarginTop(*q) = SavedMarginTop(*q);
           Lab_MarginBottom(*q) = SavedMarginBottom(*q);
          }
          else if (XmIsLabelGadget(*q))
          {
           _XmAssignLabG_MarginTop((XmLabelGadget)*q, SavedMarginTop(*q));
           _XmAssignLabG_MarginBottom((XmLabelGadget)*q, SavedMarginBottom(*q));
           _XmReCacheLabG(*q);
          }
          else if (XmIsText(*q) || XmIsTextField(*q))
          {
            XmBaselineMargins textMargins;

            textMargins.margin_top = SavedMarginTop(*q);
            textMargins.margin_bottom = SavedMarginBottom(*q);
            SetOrGetTextMargins(*q, XmBASELINE_SET, &textMargins);

          }
       }
      }
     }

   /*
    * get array built for both widgets and gadgets layout is based only on 
    * this array, adjust width margins &  adjust height margins
    */
     RC_Boxes(m)=
       (XmRCKidGeometry)_XmRCGetKidGeo( (Widget) m, NULL, NULL, RC_EntryBorder(m),
				    RC_EntryBorder (m),
				    (IsVertical (m) && RC_DoMarginAdjust (m)),
				    (IsHorizontal (m) &&
				     RC_DoMarginAdjust (m)),
				    RC_HelpPb (m),
				    RC_TearOffControl(m),
				    XmGET_PREFERRED_SIZE);
     for (i = 0; RC_Boxes(m) [i].kid != NULL; i++)
     {
       Widget rc_kid;
       rc_kid = RC_Boxes(m) [i].kid;

       if (XmIsLabel(rc_kid))
       {
          if (!Lab_IsPixmap(rc_kid))
          {
            XmPrimitiveClassExt              *wcePtr;
            WidgetClass   wc = XtClass(rc_kid);

         /* The baseline functions returns the baseline on the current size */
         /* Since we need the preferred baseline we need to calculate the y   */
         /* coordinate on the preferred size and add this in to the baseline */
         /* returned, after subtracting the y coordinate of the current widget */
            y = Lab_Highlight(rc_kid) + Lab_Shadow(rc_kid) +
                Lab_MarginHeight(rc_kid) + Lab_MarginTop(rc_kid) +
                ((RC_Boxes(m) [i].box.height - Lab_MarginTop(rc_kid) -
		  Lab_MarginBottom(rc_kid) -
                (2 * (Lab_MarginHeight(rc_kid) + Lab_Shadow(rc_kid) +
		      Lab_Highlight(rc_kid))) -
                Lab_TextRect_height(rc_kid)) / 2);

	    if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)) {
		wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
		if (*wcePtr && (*wcePtr)->widget_baseline)
		    (*((*wcePtr)->widget_baseline)) (rc_kid, &baselines,
						&line_count);
		if (AlignmentBaselineTop(m))
		    RC_Boxes(m) [i].baseline = baselines[0] -
						Lab_TextRect_y(rc_kid) + y; 
		else if (AlignmentBaselineBottom(m))
		    RC_Boxes(m) [i].baseline = baselines[line_count - 1] -
						Lab_TextRect_y(rc_kid) + y;
		XtFree((char *)baselines);
	    }
            RC_Boxes(m) [i].margin_top = 0;
            RC_Boxes(m) [i].margin_bottom = 0;
          }
          else
          {
            RC_Boxes(m) [i].baseline = 0;
            RC_Boxes(m) [i].margin_top = 0;
            RC_Boxes(m) [i].margin_bottom = 0;
          }
        }
        else if (XmIsLabelGadget(rc_kid))
        {
          if (!LabG_IsPixmap(rc_kid))
          {
            XmGadgetClassExt              *wcePtr;
            WidgetClass   wc = XtClass(rc_kid);

         /* The baseline functions returns the baseline on the current size */
         /* Since we need the preferred baseline we need to calculate the y   */
         /* coordinate on the preferred size and add this in to the baseline */
         /* returned, after subtracting the y coordinate of the current widget */
            y = LabG_Highlight(rc_kid) + LabG_Shadow(rc_kid) +
                LabG_MarginHeight(rc_kid) + LabG_MarginTop(rc_kid) +
                ((RC_Boxes(m) [i].box.height - LabG_MarginTop(rc_kid) - LabG_MarginBottom(rc_kid) -
                (2 * (LabG_MarginHeight(rc_kid) + LabG_Shadow(rc_kid) + LabG_Highlight(rc_kid))) -
                LabG_TextRect_height(rc_kid)) / 2);

	    if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)) {
		wcePtr = _XmGetGadgetClassExtPtr(wc, NULLQUARK);
		if (*wcePtr && (*wcePtr)->widget_baseline)
		    (*((*wcePtr)->widget_baseline)) (rc_kid, &baselines,
						&line_count);
		if (AlignmentBaselineTop(m))
		    RC_Boxes(m) [i].baseline = baselines[0] -
					(((XmLabelGadget)rc_kid)->rectangle.y +
					LabG_TextRect_y(rc_kid)) + y;
		else if (AlignmentBaselineBottom(m))
		    RC_Boxes(m) [i].baseline = baselines[line_count - 1] -
					(((XmLabelGadget)rc_kid)->rectangle.y +
					LabG_TextRect_y(rc_kid)) + y;
		XtFree((char *)baselines);
	    }
            RC_Boxes(m) [i].margin_top = 0;
            RC_Boxes(m) [i].margin_bottom = 0;
          }
          else
          {
            RC_Boxes(m) [i].baseline = 0;
            RC_Boxes(m) [i].margin_top = 0;
            RC_Boxes(m) [i].margin_bottom = 0;
          }
        }
        else if (XmIsText(rc_kid) || XmIsTextField(rc_kid))
        {
            XmPrimitiveClassExt              *wcePtr;
            WidgetClass   wc = XtClass(rc_kid);

	    if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)) {
		wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
		if (*wcePtr && (*wcePtr)->widget_baseline)
		    (*((*wcePtr)->widget_baseline)) (rc_kid, &baselines,
						&line_count);
		if (AlignmentBaselineTop(m))
		    RC_Boxes(m) [i].baseline = baselines[0];
		else if (AlignmentBaselineBottom(m))
		    RC_Boxes(m) [i].baseline = baselines[line_count - 1];
		XtFree((char *)baselines);
	    }
            RC_Boxes(m) [i].margin_top = 0;
            RC_Boxes(m) [i].margin_bottom = 0;
        }
      }
   }
   else
   {
        /*
    * get array built for both widgets and gadgets layout is based only on 
    * this array, adjust width margins &  adjust height margins
    */
      RC_Boxes(m)=
       (XmRCKidGeometry)_XmRCGetKidGeo( (Widget) m, NULL, NULL, RC_EntryBorder(m),
				    RC_EntryBorder (m),
				    (IsVertical (m) && RC_DoMarginAdjust (m)),
				    (IsHorizontal (m) &&
				     RC_DoMarginAdjust (m)),
				    RC_HelpPb (m),
				    RC_TearOffControl(m),
				    XmGET_PREFERRED_SIZE);
   }
   think_about_size (m, w, h, NULL, NULL);

   XtFree ((char *) RC_Boxes(m));
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
    XmRowColumnWidget m = (XmRowColumnWidget) widget ;
    Dimension width = 0, height = 0 ;
    
    /* first determine what is the desired size, using the resizeWidth
       and resizeHeight resource and the intended value */
    if (GMode(intended) & CWWidth) width = intended->width;
    if (GMode(intended) & CWHeight) height = intended->height;

    if (!RC_ResizeWidth(m)) width = XtWidth(widget) ;
    if (!RC_ResizeHeight(m)) height = XtHeight(widget) ;

    PreferredSize (m, &width, &height);

    desired->width = width ;
    desired->height = height ;

    /* deal with user initial size setting */
    /****************
      Comment that out for now, too fragile...
    if (!XtIsRealized(widget))  {
	if (XtWidth(widget) != 0) desired->width = XtWidth(widget) ;
	if (XtHeight(widget) != 0) desired->height = XtHeight(widget) ;
    } ****************/

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}



static void
#ifdef _NO_PROTO
CheckAndSetOptionCascade( menu )
      XmRowColumnWidget menu;
#else
CheckAndSetOptionCascade(
      XmRowColumnWidget menu )
#endif
{
   Dimension width = 0;
   Dimension height = 0;
   int i;
   
   if (IsOption(menu) && !RC_FromResize(menu))
   {
      Widget cb;

      if ((cb = XmOptionButtonGadget( (Widget) menu)) != NULL)
      {
	 if (RC_OptionSubMenu(menu))
	 {
	     find_largest_option_selection
		 ((XmRowColumnWidget)RC_OptionSubMenu(menu), &width, &height );

	     width += Double(G_HighlightThickness(cb)) +
		 G_ShadowThickness(cb) + LabG_MarginRight(cb) +
		     Double(MGR_ShadowThickness(RC_OptionSubMenu(menu))) - 2;
	     
	     height += Double(G_HighlightThickness(cb)) + LabG_MarginTop(cb)
		 + LabG_MarginBottom(cb);
	     
	     /* change cb if needed */
	     if ((width != XtWidth(cb)) || (height != XtHeight(cb))) {
		 /* Fix for 5185 */
		 /* we have pixels, but the cascade unit type might not be 
		  pixel, so save it and restore it after the setvalues */
	       unsigned char saved_unit_type = 
		   ((XmGadget)cb)->gadget.unit_type ;

	       ((XmGadget)cb)->gadget.unit_type = XmPIXELS;
	       XtVaSetValues (cb, XmNwidth, width, XmNheight, height, NULL);
	       ((XmGadget)cb)->gadget.unit_type = saved_unit_type;
	     }
	 }
      }
   }

   /*
    * if its is a pulldown menu, travel up the cascades to verify the
    * option menus cascade button is sized large enough.  
    */
   else if (IsPulldown(menu))
   {
      for (i=0; i < menu->row_column.postFromCount; i++)
      {
       CheckAndSetOptionCascade((XmRowColumnWidget)
                            XtParent(menu->row_column.postFromList[i]));
      }
   }
}

/*
 * Layout the row column widget to fit it's current size; ignore possible 
 * non-fitting of the entries into a too small row column widget.
 *
 * Don't forget the instigator,
 */
static void 
#ifdef _NO_PROTO
AdaptToSize( m, instigator, request )
        XmRowColumnWidget m ;
        Widget instigator ;
        XtWidgetGeometry *request ;
#else
AdaptToSize(
        XmRowColumnWidget m,
        Widget instigator,
        XtWidgetGeometry *request )
#endif /* _NO_PROTO */
{
   Dimension w = XtWidth (m);
   Dimension h = XtHeight (m);
   short i;
   Widget *q;

   if ((!IsOption(m)) && ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
       (PackTight(m) && IsHorizontal(m))))
   {
     ForManagedChildren(m, i, q)
     {
        if (XmIsLabel(*q))
        {
         Lab_MarginTop(*q) = SavedMarginTop(*q);
         Lab_MarginBottom(*q) = SavedMarginBottom(*q);
        }
        else if (XmIsLabelGadget(*q))
        {
         _XmAssignLabG_MarginTop((XmLabelGadget)*q, SavedMarginTop(*q));
         _XmAssignLabG_MarginBottom((XmLabelGadget)*q, SavedMarginBottom(*q));
         _XmReCacheLabG(*q);
         
        }
        else if (XmIsText (*q) || XmIsTextField (*q))
        {
          XmBaselineMargins textMargins;

          textMargins.margin_top = SavedMarginTop(*q);
          textMargins.margin_bottom = SavedMarginBottom(*q);
          SetOrGetTextMargins(*q, XmBASELINE_SET, &textMargins);
        }
     }
   }
   /*
    * get array built for both widgets and gadgets,
    * layout is based only on this array,
    * adjust width margins and  adjust height margins
    */
   RC_Boxes(m) =
       (XmRCKidGeometry)_XmRCGetKidGeo( (Widget) m, instigator, request, RC_EntryBorder(m),
				    RC_EntryBorder (m),
				    (IsVertical (m) && RC_DoMarginAdjust (m)),
				    (IsHorizontal (m) &&
				     RC_DoMarginAdjust (m)),
				    RC_HelpPb (m),
				    RC_TearOffControl(m),
				    XmGET_PREFERRED_SIZE);

   if ((!IsOption(m)) && ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
       (PackTight(m) && IsHorizontal(m))))
   {
     for (i = 0; RC_Boxes(m) [i].kid != NULL; i++)
     {
       if (XmIsLabel(RC_Boxes(m) [i].kid))
       {
          if (XtHeight (RC_Boxes(m) [i].kid) != RC_Boxes(m) [i].box.height)
          {
              Widget lw = RC_Boxes(m) [i].kid;
               _XmConfigureObject( (Widget) lw, lw->core.x, lw->core.y,
                  lw->core.width, RC_Boxes(m) [i].box.height, lw->core.border_width);
          }

          if (!Lab_IsPixmap(RC_Boxes(m) [i].kid) &&
		(AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)))
          {
            XmPrimitiveClassExt              *wcePtr;
            WidgetClass   wc = XtClass(RC_Boxes(m) [i].kid);
            Dimension *baselines;
            int line_count;

            wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
            if (*wcePtr && (*wcePtr)->widget_baseline)
              (*((*wcePtr)->widget_baseline)) (RC_Boxes(m) [i].kid,
                                               &baselines, &line_count);
            if (AlignmentBaselineTop(m))
              RC_Boxes(m) [i].baseline = baselines[0];
            else if (AlignmentBaselineBottom(m))
              RC_Boxes(m) [i].baseline = baselines[line_count - 1];
            XtFree((char *)baselines);
          }
          else
            RC_Boxes(m) [i].baseline = 0;

        }
        else if (XmIsLabelGadget(RC_Boxes(m) [i].kid))
        {
          XmLabelGadget lg = (XmLabelGadget) RC_Boxes(m) [i].kid;

          if (XtHeight (RC_Boxes(m) [i].kid) != RC_Boxes(m) [i].box.height)
          {
               _XmConfigureObject( (Widget) lg, lg->rectangle.x, lg->rectangle.y,
                  lg->rectangle.width, RC_Boxes(m) [i].box.height, lg->rectangle.border_width);
          }

          if (!LabG_IsPixmap(RC_Boxes(m) [i].kid) &&
		(AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)))
          {
            XmGadgetClassExt              *wcePtr;
            WidgetClass   wc = XtClass(RC_Boxes(m) [i].kid);
            Dimension *baselines;
            int line_count;

            wcePtr = _XmGetGadgetClassExtPtr(wc, NULLQUARK);
            if (*wcePtr && (*wcePtr)->widget_baseline)
	      (*((*wcePtr)->widget_baseline)) (RC_Boxes(m) [i].kid,
                                               &baselines, &line_count);
            if (AlignmentBaselineTop(m))
              RC_Boxes(m) [i].baseline = baselines[0] - lg->rectangle.y;
            else if (AlignmentBaselineBottom(m))
              RC_Boxes(m) [i].baseline = baselines[line_count - 1] - lg->rectangle.y;
            XtFree((char *)baselines);
          }
          else
            RC_Boxes(m) [i].baseline = 0;


        }
        else if (XmIsText(RC_Boxes(m) [i].kid) || XmIsTextField(RC_Boxes(m) [i].kid))
        {
          XmPrimitiveClassExt              *wcePtr;
          WidgetClass   wc = XtClass(RC_Boxes(m) [i].kid);
          Dimension *baselines;
          int line_count;

          if (XtHeight (RC_Boxes(m) [i].kid) != RC_Boxes(m) [i].box.height)
          {
              Widget lw = RC_Boxes(m) [i].kid;
               _XmConfigureObject( (Widget) lw, lw->core.x, lw->core.y,
                  lw->core.width, RC_Boxes(m) [i].box.height, lw->core.border_width);
          }

	 if (AlignmentBaselineTop(m) || AlignmentBaselineBottom(m)) {
          wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);
          if (*wcePtr && (*wcePtr)->widget_baseline)
            (*((*wcePtr)->widget_baseline)) (RC_Boxes(m) [i].kid,
                                             &baselines, &line_count);
          if (AlignmentBaselineTop(m))
            RC_Boxes(m) [i].baseline = baselines[0];
          else if (AlignmentBaselineBottom(m))
            RC_Boxes(m) [i].baseline = baselines[line_count - 1];
          XtFree((char *)baselines);
	 }
        }
      }
   }
   if (!PackNone (m)) 
   {
       if (IsOption(m))
	   OptionSizeAndLayout (m, &w, &h, instigator, request, FALSE);
       else
	   Layout (m, &w, &h);
   }

   if ((!IsOption(m)) && ((PackColumn(m) && (IsVertical(m) || IsHorizontal(m))) ||
       (PackTight(m) && IsHorizontal(m))))
   {
     for (i = 0; RC_Boxes(m) [i].kid != NULL; i++)
     {
       if (XmIsLabel(RC_Boxes(m) [i].kid))
       {
         Lab_MarginTop(RC_Boxes(m) [i].kid) = RC_Boxes(m) [i].margin_top;
         Lab_MarginBottom(RC_Boxes(m) [i].kid) = RC_Boxes(m) [i].margin_bottom;

       }
       else if (XmIsLabelGadget(RC_Boxes(m) [i].kid))
       {
         _XmAssignLabG_MarginTop((XmLabelGadget)RC_Boxes(m) [i].kid, 
	    RC_Boxes(m) [i].margin_top);

         _XmAssignLabG_MarginBottom((XmLabelGadget)RC_Boxes(m) [i].kid, 
	    RC_Boxes(m) [i].margin_bottom);

         _XmReCacheLabG(RC_Boxes(m) [i].kid);
       }
       else if (XmIsText(RC_Boxes(m) [i].kid) || XmIsTextField(RC_Boxes(m) [i].kid))
       {
         XmBaselineMargins textMargins;

         textMargins.margin_top = RC_Boxes(m) [i].margin_top;
         textMargins.margin_bottom = RC_Boxes(m) [i].margin_bottom;
         SetOrGetTextMargins(RC_Boxes(m) [i].kid, XmBASELINE_SET, &textMargins);
      
       }
     }
   }   

   _XmRCSetKidGeo ((XmRCKidGeometry)RC_Boxes(m), instigator);

   /*
   ** Hack alert!
   ** This is special code to enforce that the CBG in an option menu is
   ** kept correctly-sized when the items in this pulldown, associated with
   ** that cascade, change. There is no protocol for communicating this
   ** information nor any mechanism for adapting one; so we make the size
   ** request ourselves, here. The point is not that we are setting to the
   ** correct size so much as that the cascade's size is being adjusted after
   ** the menu size is fixed. calc_CBG_dims is probably being called an extra
   ** time to figure out the real size. This is not a frequent case.
    *
    * Do not call this routine below if this call was initiated by a geometry
    * request from an option menu's label or cascade button.  In that case,
    * the geometry has already been taken care of and must not be meddled
    * with or it will reset some values incorrectly.
    */
   if (!IsOption(m) || !instigator ||
       !((instigator == RC_Boxes(m)[0].kid) ||
	 (instigator == RC_Boxes(m)[1].kid)))
   {
       CheckAndSetOptionCascade(m);
   }

/* The old geometry management took care of resizing the instigator if
   XtGeometryYes was returned even if core.width and core.height had
   not changed. However this is not the case with the new geometry
   management. Therefore if margins have changed but not the core
   width and height label's resize needs to be called to calculate
   the x & y coordinates for the label text, with the new margins. */

   if ((instigator) && (XmIsLabel(instigator) || XmIsLabelGadget(instigator)))
   {
     WidgetClass wc = XtClass(instigator);

      (*(wc->core_class.resize)) ((Widget) instigator);
   }
   XtFree ( (char *) RC_Boxes(m));
}


/*
 * Resize the row column widget, and any subwidgets there are.
 * Since the gravity is set to NW, handle shrinking when there may not
 * be a redisplay.
 */
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget m = (XmRowColumnWidget) wid ;
   Boolean		draw_shadow = False;

   RC_SetFromResize(m, TRUE);	

   /* clear the shadow if its needed (will check if its now larger) */
   _XmClearShadowType( (Widget) m, m->row_column.old_width,
		       m->row_column.old_height,
		       m->row_column.old_shadow_thickness, 0);

   /*
    * if it is now smaller, redraw the shadow since there may not be a
    * redisplay - DON'T draw shadows for OPTION MENUS!
    */
   if (!IsOption(m) &&
       (m->row_column.old_height > m->core.height ||
        m->row_column.old_width > m->core.width))
       draw_shadow = True;

   m->row_column.old_width = m->core.width;
   m->row_column.old_height = m->core.height;
   m->row_column.old_shadow_thickness = m->manager.shadow_thickness;

   AdaptToSize (m, NULL, NULL);

   if (draw_shadow && XtIsRealized (m) && m->manager.shadow_thickness )
       /* pop-out not pop-in */
       _XmDrawShadows (XtDisplay (m), XtWindow (m),
		      m->manager.top_shadow_GC,
		      m->manager.bottom_shadow_GC,
		      0, 0, m->core.width, m->core.height,
		      m->manager.shadow_thickness,
		      XmSHADOW_OUT);

   RC_SetFromResize(m, FALSE);	
}


/*
 * class Redisplay proc 
 */
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
    XmRowColumnWidget m = (XmRowColumnWidget) w;
    XEvent tempEvent;

    /* Ignore exposures generated as we unpost */
    if ((IsPopup (m) || IsPulldown (m)) &&
        !((XmMenuShellWidget)XtParent(m))->shell.popped_up)
    {
       RC_SetExpose (m, TRUE);
       return;
    }

    if (RC_DoExpose (m))            /* a one-shot set on popup */
    {                               /* so we ignore next expose */

        if (event == NULL)          /* Fast exposure is happening */
        {
            event = &tempEvent;
            event->xexpose.x = 0;
            event->xexpose.y = 0;
            event->xexpose.width = m->core.width;
            event->xexpose.height = m->core.height;
        }

        _XmRedisplayGadgets( (Widget) m, event, region);

        if (IsPopup (m) || IsPulldown (m) || IsBar(m))
        {
            if (MGR_ShadowThickness(m))
                _XmDrawShadows (XtDisplay (m), XtWindow (m),
                    /* pop-out not pop-in */
                    m->manager.top_shadow_GC,
                    m->manager.bottom_shadow_GC,
                    0, 0, m->core.width, m->core.height,
                    m->manager.shadow_thickness,
		    XmSHADOW_OUT);
        }
    }

    RC_SetExpose (m, TRUE);
}


/*
 * Geometry manager for subwidgets of a row column widget; be accomdating, 
 * try to say yes, and then deal with our parent's geometry mgr.
 *
 * I'm not even going to try to figure out what to do with an Almost
 * response from the row column widget's parent; just take it as a 'No'.
 */

static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( instigator, desired, allowed )
        Widget instigator ;
        XtWidgetGeometry *desired ;
        XtWidgetGeometry *allowed ;
#else
GeometryManager(
        Widget instigator,
        XtWidgetGeometry *desired,
        XtWidgetGeometry *allowed )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget m = (XmRowColumnWidget) XtParent(instigator);
   Dimension w = 0;
   Dimension h = 0;
   XtGeometryResult result = XtGeometryYes;

   /* First treat the special case resulting from a change in positionIndex */
   if (RCIndex(instigator) == XmLAST_POSITION) { 
                    /* set in ConstraintSetValues */
       int i ;

       /* first reset the value of positionIndex to its real value */
       for (i = 0 ; i < m->composite.num_children; i++)
         if (m->composite.children[i] == instigator) {
             RCIndex(instigator) = i ;
             break ; 
         }

       /* then accept the desired change */
       if (IsX(desired) && desired->x >= 0) 
         instigator->core.x = desired->x;
       if (IsY(desired) && desired->y >= 0) 
         instigator->core.y = desired->y;
       if (IsHeight(desired) && desired->height > 0)
         instigator->core.height = desired->height;
       if (IsWidth(desired) && desired->width > 0)
         instigator->core.width = desired->width;
      return XtGeometryYes; 
    }

   /*
    * find out what size we need to be with this new widget
    */
   RC_Boxes(m) = (XmRCKidGeometry)
       _XmRCGetKidGeo( (Widget) m, instigator, desired,
		     RC_EntryBorder(m), RC_EntryBorder (m),
		     (IsVertical (m) && RC_DoMarginAdjust (m)),
		     (IsHorizontal (m) && RC_DoMarginAdjust (m)),
		     RC_HelpPb (m), 
		     RC_TearOffControl(m),
		     XmGET_PREFERRED_SIZE);

   think_about_size(m, &w, &h, instigator, desired);

   /* check if the requested size if the same for the instigator
      as the computed size in RC_boxes, if not, return no to the 
      instigator 

   if (!IsOption(m)) {
       for (i = 0; RC_Boxes (m) [i].kid != NULL; i++) {
	   if ((RC_Boxes (m) [i].kid == instigator) && 
	       ((IsWidth(desired) && 
		 (RC_Boxes (m) [i].box.width != desired->width)) ||
		(IsHeight(desired) && 
		 (RC_Boxes (m) [i].box.height != desired->height))))
	       {
		   XtFree( (char *) RC_Boxes(m));
		   return XtGeometryNo ;
	       }
       }
   }

   This code breaks some other tests, I'm removing it */
       
   if (IsOption(m))
   {
      if (instigator == XmOptionButtonGadget( (Widget) m)) {
	  /*
	   * Fix for 5388 - Check to see if the OptionLabel is Managed.  
	   * If it is base comparisons off of RC_Boxes(m)[1].  
	   * If not, base comparisons off of RC_Boxes(m)[0]
	   */

          XtWidgetGeometry *button_box;
        
          if (!XtIsManaged(XmOptionLabelGadget( (Widget) m))) {
	      button_box = &(RC_Boxes(m)[0].box);
          } else {
	      button_box = &(RC_Boxes(m)[1].box);
          }
 
  	 /* 
  	  * Grow only
  	  */
	  if ((desired->request_mode & CWWidth) &&
	      (desired->width < BWidth(button_box))) {
	      allowed->width = BWidth(button_box);
	      allowed->height = BHeight(button_box);
	      allowed->request_mode = (CWHeight | CWWidth);
	      result = XtGeometryAlmost;
	  }
  
	  if ((desired->request_mode & CWHeight) &&
	      (desired->height < BHeight(button_box))) {
	      allowed->width = BWidth(button_box);
	      allowed->height = BHeight(button_box);
	      allowed->request_mode = (CWHeight | CWWidth);
	      result = XtGeometryAlmost;
	  }
	 
         if (result != XtGeometryYes)
         {
            XtFree( (char *) RC_Boxes(m));
            return(result);
         }
      }

      if (instigator == XmOptionLabelGadget( (Widget) m))
      {
	 /*
	  * Can't get shorter
	  */
         if ((desired->request_mode & CWHeight) &&
	     (desired->height < RC_Boxes(m)[0].box.height))
         {
	    allowed->width = RC_Boxes(m)[0].box.width;
	    allowed->height = RC_Boxes(m)[0].box.height;
	    allowed->request_mode = (CWHeight | CWWidth);
	    result = XtGeometryAlmost;
         }

         if (result != XtGeometryYes)
         {
            XtFree( (char *) RC_Boxes(m));
            return(result);
         }
      }
   }

   /*
    * now decide if the menu needs to change size.
    */

   XtFree( (char *) RC_Boxes(m));
   
   if ((w != XtWidth (m)) || (h != XtHeight (m)))
   {
      XtWidgetGeometry menu_desired, menu_allowed;
      menu_desired.request_mode = 0;

      if (w != XtWidth (m))
      {
	 menu_desired.width = w;
	 menu_desired.request_mode |= CWWidth;
      }

      if (h != XtHeight (m))
      {
	 menu_desired.height = h;
	 menu_desired.request_mode |= CWHeight;
      }

      /* Check for the query only bit. */
      if (desired->request_mode & XtCWQueryOnly)
         menu_desired.request_mode |= XtCWQueryOnly;

      result = XtMakeGeometryRequest( (Widget) m,&menu_desired,&menu_allowed);
      
      switch (result)
      {
       case XtGeometryAlmost:
       case XtGeometryNo:          /* give it up, don't change */

      /*
       * CR 5579 - If XtGeometryNo is returned, but the requested height and the
       *           requested width are less that the current, allow the children
       *           to shrink if they want to while maintaining our own size.
       */
         if ((XtWidth(m) < w) || (XtHeight(m) < h))
	 return (XtGeometryNo);
         break;
       default: /* fall out */
	 break;
      }
   }

   /* Check for the query only bit. */
   if (!(desired->request_mode & XtCWQueryOnly))
   {
       AdaptToSize(m,instigator,desired);

       /* Clear shadow if necessary. */
       if (m->row_column.old_shadow_thickness)
	   _XmClearShadowType( (Widget) m, m->row_column.old_width, 
			      m->row_column.old_height, 
			      m->row_column.old_shadow_thickness, 0);

       m->row_column.old_width = m->core.width;
       m->row_column.old_height = m->core.height;
       m->row_column.old_shadow_thickness = m->manager.shadow_thickness;
   }
   return (XtGeometryYes);
}

/*
 * fix the visual attributes of the subwidget to be what we like
 *
 *  1.  make border width uniform
 *  2.  maybe set the label alignment
 */
static void 
#ifdef _NO_PROTO
FixVisual( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
FixVisual(
        XmRowColumnWidget m,
        Widget w )
#endif /* _NO_PROTO */
{
   Arg al[10];
   int ac;
   
   if (RC_EntryBorder(m))
   {
      _XmConfigureObject( w, w->core.x,w->core.y, 
			 w->core.width, w->core.height, RC_EntryBorder(m));
   }

   if (IsOption(m))
       return;

   if (XmIsLabelGadget(w))
   {
      if (IsAligned (m))
      {
	 if (IsWorkArea(m) ||
	     ((w->core.widget_class != xmLabelWidgetClass) &&
	      (w->core.widget_class != xmLabelGadgetClass)))
	     
	 {
	    ac = 0;
	    XtSetArg (al[ac], XmNalignment, RC_EntryAlignment (m));
	    ac++;
	    XtSetValues (w, al, ac);
	 }
      }
   }
   else if (XmIsLabel (w))
   {
      if (IsAligned (m))
      {
	 if ((w->core.widget_class != xmLabelWidgetClass) ||
	     IsWorkArea(m))
	 {
	    ac = 0;
	    XtSetArg (al[ac], XmNalignment, RC_EntryAlignment (m));
	    ac++;
	    XtSetValues (w, al, ac);
	 }
      }
   }
}

/*
 * If an entryCallback exists, set a flag in the buttons to not do
 * their activate callbacks.
 */
static void 
#ifdef _NO_PROTO
FixCallback( m, w )
        XmRowColumnWidget m ;
        Widget w ;
#else
FixCallback(
        XmRowColumnWidget m,
        Widget w )
#endif /* _NO_PROTO */
{
   char *c = which_callback (w);       /* which callback to use */

   if (c == NULL)
       return;          /* can't do it to this guy */

   if (m->row_column.entry_callback)
   {
      /* 
       * Disable the buttons activate callbacks
       */
      if (XmIsLabelGadget(w))
	  (*(((XmLabelGadgetClassRec *)XtClass(w))->
	     label_class.setOverrideCallback)) (w);
      else
	  (*(((XmLabelClassRec *)XtClass(w))->
	     label_class.setOverrideCallback)) (w);
   }
}


static void 
#ifdef _NO_PROTO
XmGetMenuKidMargins( m, width, height, left, right, top, bottom)
        XmRowColumnWidget m ;
        Dimension *width ;
        Dimension *height ;
        Dimension *left ;
        Dimension *right ;
        Dimension *top ;
        Dimension *bottom ;
#else
XmGetMenuKidMargins(
        XmRowColumnWidget m,
        Dimension *width,
        Dimension *height,
        Dimension *left,
        Dimension *right,
        Dimension *top,
        Dimension *bottom )
#endif /* _NO_PROTO */
{
   register int i;
   Widget *q;

    *width = *height = *left = *right = *top = *bottom = 0;

    for (i = 0; i < m->composite.num_children; i++)
    {
        Widget p = (Widget) m->composite.children[i]; 

        if (IsManaged (p))
        {
            if (XmIsLabelGadget(p))
            {
                register XmLabelGadget lg = (XmLabelGadget) p;

                if (LabG_MarginWidth (lg) > *width)  
                       *width  = LabG_MarginWidth  (lg);

                if (LabG_MarginHeight(lg) > *height) 
                       *height = LabG_MarginHeight (lg);

                if (LabG_MarginLeft  (lg) > *left)   
                       *left   = LabG_MarginLeft   (lg);

                if (LabG_MarginRight (lg) > *right)  
                       *right  = LabG_MarginRight  (lg);
/*
                if (LabG_MarginTop   (lg) > *top)    
                       *top = LabG_MarginTop (lg);

                if (LabG_MarginBottom(lg) > *bottom) 
                       *bottom = LabG_MarginBottom (lg);
*/

            }
            else if (XmIsLabel(p))
            {
                register XmLabelWidget lw = (XmLabelWidget) p;

                if (Lab_MarginWidth (lw) > *width)  
                       *width  = Lab_MarginWidth  (lw);

                if (Lab_MarginHeight(lw) > *height) 
                       *height = Lab_MarginHeight (lw);

                if (Lab_MarginLeft  (lw) > *left)   
                       *left   = Lab_MarginLeft   (lw);

                if (Lab_MarginRight (lw) > *right)  
                       *right  = Lab_MarginRight  (lw);
/*
                if (Lab_MarginTop   (lw) > *top)    
                       *top = Lab_MarginTop (lw);

                if (Lab_MarginBottom(lw) > *bottom) 
                       *bottom = Lab_MarginBottom (lw);
*/

            }
        }
    }
    ForManagedChildren (m, i, q)
    {
      if (XmIsLabel(*q) || XmIsLabelGadget(*q))
      {
       if (SavedMarginTop(*q) > *top)
           *top = SavedMarginTop(*q);
       if (SavedMarginBottom(*q) > *bottom)
           *bottom = SavedMarginBottom(*q);
      }
    }
}

/*
 * Toggle buttons have this thingy hanging off the left of the
 * widget, before the text.  This dimension is known as the MarginLeft.
 * Pulldown's have hot spots in the MarginRight, accelerators go in the
 * marginRight also.
 *
 * For generality's sake we should insure that all
 * of the current label subclass widgets in the menu have the 
 * margins set to the same value.  
 */
static void 
#ifdef _NO_PROTO
DoMarginAdjustment( m )
        XmRowColumnWidget m ;
#else
DoMarginAdjustment(
        XmRowColumnWidget m )
#endif /* _NO_PROTO */
{
    register Widget *p;
    register int i; 
    Dimension m_w, m_h, m_l, m_r, m_t, m_b;
    Dimension w, h;

    if ((!RC_DoMarginAdjust (m)) || (IsOption(m)))
    {
      ForManagedChildren (m, i, p)
      {
        if (XmIsLabelGadget(*p))
        {
            XmLabelGadget q;    
 
            q = (XmLabelGadget) (*p);
            SavedMarginTop(*p) = LabG_MarginTop (q);
            SavedMarginBottom(*p) = LabG_MarginBottom (q);
        }
        else if (XmIsLabel(*p))
        {
            XmLabelWidget q;

            q = (XmLabelWidget) (*p);
            SavedMarginTop(*p) = Lab_MarginTop (q);
            SavedMarginBottom(*p) = Lab_MarginBottom (q);
        }
        else if (XmIsText(*p) || XmIsTextField(*p))
        {

          XmBaselineMargins textMargins;

          SetOrGetTextMargins(*p, XmBASELINE_GET, &textMargins);
          SavedMarginTop(*p) = textMargins.margin_top;
          SavedMarginBottom(*p) = textMargins.margin_bottom;
          
        }
       }
       return;
    }
    /*
     * this should almost be part
     * of the layout process, except this requires a setvalue not a resize...
     */

    XmGetMenuKidMargins (m, &m_w, &m_h, &m_l, &m_r, &m_t, &m_b);

    ForManagedChildren (m, i, p)
    {
        if (XmIsLabelGadget(*p))
        {
            XmLabelGadget q;
            /*
             * If in a popup or pulldown pane,
             * don't do labels; i.e. only do buttons.
             */
            if (((*p)->core.widget_class == xmLabelGadgetClass) &&
                (IsPulldown(m) || IsPopup(m)))
                continue;

            w = XtWidth  (*p);
            h = XtHeight (*p);

            q = (XmLabelGadget) (*p);

            if (IsVertical (m)) 
            {
	       /* change horiz margins to  be uniform */
	       if (LabG_MarginLeft(q) != m_l)
	       {
		  w += m_l - LabG_MarginLeft(q);
		  _XmAssignLabG_MarginLeft(q, m_l);
	       }

	       if (LabG_MarginRight(q) != m_r)
	       {
		  w += m_r - LabG_MarginRight(q);
		  _XmAssignLabG_MarginRight(q, m_r);
	       }

	       if (LabG_MarginWidth(q) != m_w)
	       {
		  w += m_w - LabG_MarginWidth(q);
		  _XmAssignLabG_MarginWidth(q, m_w);
	       }

               if (PackColumn(m))
               {
                 if (LabG_MarginTop(q) != m_t)
                 {
                   h += m_t - LabG_MarginTop(q);
                   _XmAssignLabG_MarginTop(q, m_t);
                 }

                 if (LabG_MarginBottom(q) != m_b)
                 {
                   h += m_b - LabG_MarginBottom(q);
                   _XmAssignLabG_MarginBottom(q, m_b);
                 }

                 if (LabG_MarginHeight(q) != m_h)
                 {
                   h += m_h - LabG_MarginHeight(q);
                   _XmAssignLabG_MarginHeight(q, m_h);
                 }

               }
       	       _XmReCacheLabG((Widget) q);

	       if (q->rectangle.width != w) 
	       {
		  _XmConfigureObject( (Widget) q, q->rectangle.x,
                                        q->rectangle.y, w, q->rectangle.height,
                                                    q->rectangle.border_width);
	       }

               if (q->rectangle.height != h)
               {
                  _XmConfigureObject( (Widget) q, q->rectangle.x,
                                        q->rectangle.y, q->rectangle.width, h,
                                                    q->rectangle.border_width);
               }
               if (PackColumn(m))
               {
/*                 Dimension temp = 0; */

                 SavedMarginTop(*p) = LabG_MarginTop (q);
                 SavedMarginBottom(*p) = LabG_MarginBottom (q);
/*                 XmWidgetGetBaselines((Widget)q, &temp); */
/*                 SavedBaseline(*p) = temp;  */
               }
            }
            else 
            {

	       /* change vert margins */
	       if (LabG_MarginTop(q) != m_t)
	       {
		  h += m_t - LabG_MarginTop(q);
		  _XmAssignLabG_MarginTop(q, m_t);
	       }

	       if (LabG_MarginBottom(q) != m_b)
	       {
		  h += m_b - LabG_MarginBottom(q);
		  _XmAssignLabG_MarginBottom(q, m_b);
	       }
	       
	       if (LabG_MarginHeight(q) != m_h)
	       {
		  h += m_h - LabG_MarginHeight(q);
		  _XmAssignLabG_MarginHeight(q, m_h);
	       }
	       
	       _XmReCacheLabG((Widget) q);

	       if (q->rectangle.height != h) 
	       {
		  _XmConfigureObject( (Widget) q, q->rectangle.x,
                                         q->rectangle.y, q->rectangle.width, h,
                                                    q->rectangle.border_width);
	       }
               SavedMarginTop(*p) = LabG_MarginTop (q);
               SavedMarginBottom(*p) = LabG_MarginBottom (q);
            }
	 }
        else if (XmIsLabel(*p))
        {
            XmLabelWidget lw;
            /*
             * If in a popup or pulldown pane,
             * don't do labels; i.e. only do buttons.
             */
            if (((*p)->core.widget_class == xmLabelWidgetClass) &&
                (IsPulldown(m) || IsPopup(m)))
                continue;

            w = XtWidth  (*p);
            h = XtHeight (*p);

            lw = (XmLabelWidget) (*p);

            if (IsVertical (m)) /* change horiz margins to */
            {                   /* be uniform */
               ChangeMargin (Lab_MarginLeft  (lw), m_l, w);
               ChangeMargin (Lab_MarginRight (lw), m_r, w);
               ChangeMargin (Lab_MarginWidth (lw), m_w, w);

               if (PackColumn(m))
               {
                ChangeMargin (Lab_MarginTop (lw), m_t, h);
                ChangeMargin (Lab_MarginBottom (lw), m_b, h);
                ChangeMarginDouble (Lab_MarginHeight (lw), m_h, h);
               }

               if (XtWidth (lw) != w) 
               {
                    _XmConfigureObject( (Widget) lw, lw->core.x, lw->core.y,
                                    w, lw->core.height, lw->core.border_width);
               }

               if (XtHeight (lw) != h)
               {
                    _XmConfigureObject( (Widget) lw, lw->core.x, lw->core.y,
                                    lw->core.width, h, lw->core.border_width);
               }

               if (PackColumn(m))
               {
/*                 Dimension temp = 0; */

                 SavedMarginTop(*p) = Lab_MarginTop (lw);
                 SavedMarginBottom(*p) = Lab_MarginBottom (lw);
/*                 XmWidgetGetBaselines((Widget)lw, &temp); */
/*                 SavedBaseline(*p) = temp; */
               }
            }
            else            /* change vert margins */
            {
/*                Dimension temp = 0; */
                ChangeMargin (Lab_MarginTop (lw), m_t, h);
                ChangeMargin (Lab_MarginBottom (lw), m_b, h);
                ChangeMarginDouble (Lab_MarginHeight (lw), m_h, h);

                if (XtHeight (lw) != h) 
                {
                    _XmConfigureObject( (Widget) lw, lw->core.x,lw->core.y,
                                     lw->core.width, h, lw->core.border_width);
                }
                SavedMarginTop(*p) = Lab_MarginTop (lw);
                SavedMarginBottom(*p) = Lab_MarginBottom (lw);
/*                XmWidgetGetBaselines((Widget)lw, &temp); */
/*                SavedBaseline(*p) = temp; */
            }
        }
    }
}


/*
 * Action routines specific to traversal.
 */

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmMenuUnmap( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuUnmap(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmCascadeButtonWidget cb = (XmCascadeButtonWidget) wid ;
   /*
    * Typically, when a cascade button is in a transient menupane, we just
    * want to ignore unmap requests.  However, when it is in a menubar, we
    * want it handled normally.
    */
   if (RC_Type(XtParent(cb)) == XmMENU_BAR)
      _XmPrimitiveUnmap( (Widget) cb, event, NULL, NULL);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmMenuFocusOut( cb, event, param, num_param )
        Widget cb ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuFocusOut(
        Widget cb,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
    /*
     * This ugly piece of code has once again reared its head.  This is to
     * take care of a focus out on the cascade of a tear off menupane.
     */
    if (!ShouldDispatchFocusOut(cb))
       return;

    /* Forward the event for normal processing */
    _XmPrimitiveFocusOut( cb, event, NULL, NULL);
}



/*
 * The normal primitive FocusIn procedure does nothing if a transient
 * operation is happening.  Since we are part of the transient operation,
 * and we still want focus to work, we need to use the internal function.
 */
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmMenuFocusIn( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuFocusIn(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmCascadeButtonWidget cb = (XmCascadeButtonWidget) wid ;
   /* Forward the event for normal processing */
   _XmPrimitiveFocusInInternal( (Widget) cb, event, NULL, NULL);
}



/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ActionNoop( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
ActionNoop(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
   /*
    * Do nothing; the purpose is to override the actions taken by the
    * Primitive translations.
    */
}
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EventNoop( reportingWidget, data, event, cont )
        Widget reportingWidget ;
        XtPointer data ;
        XEvent *event ;
        Boolean *cont ;
#else
EventNoop(
        Widget reportingWidget,
        XtPointer data,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
   /*
    * Do nothing; the purpose is to override the actions taken by the
    * Primitive translations.
    */
}


/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmRC_FocusIn( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmRC_FocusIn(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   /*
    * For popup and pulldown menupanes, we want to ignore focusIn request
    * which occur when we are not visible.
    */

   if (IsBar(rc))
      _XmManagerFocusIn( (Widget) rc, event, NULL, NULL);
   else if ((((XmMenuShellWidget)XtParent(rc))->shell.popped_up) &&
	    !_XmGetInDragMode((Widget) rc))
      _XmManagerFocusInInternal( (Widget) rc, event, NULL, NULL);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmRC_FocusOut( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmRC_FocusOut(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
    _XmManagerFocusOut( (Widget) rc, event, NULL, NULL);
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmRC_Unmap( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmRC_Unmap(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   /*
    * For popup and pulldown menupanes, we never care about being notified
    * when we are unmapped.  For menubars, we want normal unmapping 
    * processing to occur.
    */

   if (IsBar(wid))
      _XmManagerUnmap( wid, event, params, num_params);
}

static void 
#ifdef _NO_PROTO
_XmRC_Enter( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmRC_Enter(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   if (IsBar(rc) && RC_IsArmed(rc))
       return;

   _XmManagerEnter( (Widget) rc, event, NULL, NULL);
}


/*
 * Catch an 'Escape' which occurs within a gadget, and bring down the
 * menu system.
 */
static void 
#ifdef _NO_PROTO
_XmRC_GadgetEscape( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmRC_GadgetEscape(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   /* Process the event only if not already processed */
   if (!_XmIsEventUnique(event))
      return;

    if (IsBar(rc))
    {
        /*  
         * We're in the PM menubar mode, so let our own arm and activate 
         * procedure clean things up .
         */
        if (RC_IsArmed(rc))
            (* (((XmRowColumnClassRec *) (rc->core.widget_class))->
				row_column_class.armAndActivate))
					((Widget) rc, event, NULL, NULL);
    }
    else
    {
        /* Let the menushell widget clean things up */
        (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
          menu_shell_class.popdownOne))(XtParent(rc), event, NULL, NULL);
    }

   _XmRecordEvent(event);
}

/*
 * At one time this code was tossed out, but it's is once again needed to
 * disregard a focus out event to a cascade in a torn menupane when focus is
 * moved to its newly posted submenu.
 */
static Boolean
#ifdef _NO_PROTO
ShouldDispatchFocusOut( widget )
        Widget widget ;
#else
ShouldDispatchFocusOut(
        Widget widget )
#endif /* _NO_PROTO */
{
   /* skip focus out for CBs in Torn off panes with cascading submenus
    * unless the cascaded submenu is another torn off
    */
   if (XmIsCascadeButton(widget) &&
       RC_Type(XtParent(widget)) != XmMENU_BAR &&
       !XmIsMenuShell(XtParent(XtParent(widget))) &&
       CB_Submenu(widget) &&
       ((XmMenuShellWidget) XtParent(CB_Submenu(widget)))->shell.popped_up &&
       XmIsMenuShell(XtParent(CB_Submenu(widget))))
   {
       return (False);
   }   
   return (True);
}


/*
 * Copy the String in XmNmnemonicCharSet before returning it to the user.
 */
static void
#ifdef _NO_PROTO
_XmRC_GetMnemonicCharSet( wid, resource, value)
            Widget wid ;
            int resource ;
            XtArgVal *value ;
#else
_XmRC_GetMnemonicCharSet(
            Widget wid,
            int        resource,
            XtArgVal *value)
#endif
/****************           ARGSUSED  ****************/
{
  String        data ;
  Arg		al[1];
  Widget	label;
/****************/

  label = XmOptionLabelGadget(wid);

  if (label)
  {
     XtSetArg(al[0], XmNmnemonicCharSet, &data);
     XtGetValues(label, al, 1);

     *value = (XtArgVal) data;
  }
  else
     *value = (XtArgVal) NULL;

  return ;
}

/*
 * Copy the String in XmNmenuAccelerator before returning it to the user.
 */
static void
#ifdef _NO_PROTO
_XmRC_GetMenuAccelerator( wid, resource, value)
            Widget wid ;
            int resource ;
            XtArgVal *value ;
#else
_XmRC_GetMenuAccelerator(
            Widget wid,
            int resource,
            XtArgVal *value )
#endif
/****************           ARGSUSED  ****************/
{
  String        data ;
  XmRowColumnWidget rc  = (XmRowColumnWidget) wid;
/****************/

  if (rc->row_column.menu_accelerator != NULL) {
     data = (String)XtMalloc(strlen(RC_MenuAccelerator(rc)) + 1);
     strcpy(data, RC_MenuAccelerator(rc));
     *value = (XtArgVal) data ;
   }
  else *value = (XtArgVal) NULL;

  return ;
}

/*
 * Copy the String in XmNmenuPost before returning it to the user.
 */
static void
#ifdef _NO_PROTO
_XmRC_GetMenuPost( wid, resource, value )
       Widget wid;
       int resource;
       XtArgVal *      value ;
#else
_XmRC_GetMenuPost(
       Widget wid,
       int resource,
       XtArgVal *      value )
#endif
/****************           ARGSUSED  ****************/
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
/****************/

   if (rc->row_column.menuPost != NULL) 
   {
      *value = (XtArgVal) XtNewString(RC_MenuPost(rc)) ;
   }
   else *value = (XtArgVal) NULL;

   return ;
}

/*
 * Copy the XmString in XmNlabelString before returning it to the user.
 */
static void 
#ifdef _NO_PROTO
_XmRC_GetLabelString( wid, resource_offset, value )
        Widget wid ;
        int resource_offset ;
        XtArgVal *value ;
#else
_XmRC_GetLabelString(
        Widget wid,
        int resource_offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
  XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
  XmString        data ;
/****************/

  data = XmStringCopy(RC_OptionLabel(rc));
  *value = (XtArgVal) data ;

  return ;
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
_XmMenuBarGadgetSelect( wid, event, params, num_params )
        Widget wid;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmMenuBarGadgetSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   Widget child;

   /* Only dispatch the event to the cascade gadget when the menubar is
    * armed (which infers in explicit mode for menubars - see MenuArm())
    */
   if (RC_IsArmed(rc)/* && _XmGetFocusPolicy( (Widget) mw) == XmEXPLICIT*/)
   {
      child = rc->manager.active_child ;
      if(child && XmIsGadget(child) && XtIsSensitive(child))
	 _XmDispatchGadgetInput(child, event, XmACTIVATE_EVENT);
   }
}

static void
#ifdef _NO_PROTO
MenuStatus( wid, menu_status )
	Widget wid ;
	int *menu_status ;
#else
MenuStatus(
	Widget wid,
	int *menu_status)
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;

   /* We'll steal the RC_SetBit macro - it should still work for ints as well
    * as bytes.
    */
   RC_SetBit(*menu_status, XmMENU_TORN_BIT, RC_TornOff(rc));
   RC_SetBit(*menu_status, XmMENU_TEAR_OFF_SHELL_DESCENDANT_BIT,
      _XmIsTearOffShellDescendant((Widget)rc));
   RC_SetBit(*menu_status, XmMENU_POPUP_POSTED_BIT, RC_PopupPosted(rc));
}

/*
 * this entry is set in label and label gadget's class field so that
 * all communication from the buttons to the RowColumn are done through
 * this entry and then revectored to the appropriate routine.
 */
static void 
#ifdef _NO_PROTO
MenuProcedureEntry( va_alist )
        va_dcl
#else
MenuProcedureEntry(
        int proc,
        Widget widget,
        ... )
#endif /* _NO_PROTO */
{
        int flag ;
        XtPointer data ;
        XtPointer data2 ;
        va_list ap ;
#ifdef _NO_PROTO
        int proc ;
        Widget widget ;
   va_start( ap) ;
   proc = va_arg( ap, int) ;
   widget = va_arg( ap, Widget) ;
#else
   va_start( ap, widget) ;
#endif /* _NO_PROTO */

   switch (proc)
   {
     case XmMENU_CASCADING:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       PrepareToCascade(widget, (XmRowColumnWidget) data, (XEvent *) data2);
       break;
			
     case XmMENU_POPDOWN:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       _XmMenuPopDown (widget, (XEvent *) data, (Boolean *) data2);
       break;

     case XmMENU_SHELL_POPDOWN:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	  menu_shell_class.popdownEveryone))(widget, (XEvent *) data, 
	  NULL, NULL);
       break;

     case XmMENU_BUTTON:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       VerifyMenuButton (widget, (XEvent *) data, (Boolean *) data2);
       break;
       
     case XmMENU_CALLBACK:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       /* data points to the widget which was activated */
       ChildsActivateCallback ((XmRowColumnWidget) widget, 
	  (Widget) data, (XtPointer) data2);
       break;

     case XmMENU_TRAVERSAL:
       flag = va_arg( ap, int) ;
       /* data points to a boolean */
       _XmSetMenuTraversal(widget, (Boolean) flag);
       break;

     case XmMENU_SUBMENU:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       SetCascadeField ((XmRowColumnWidget) widget, (Widget) data, 
	  (Boolean) flag);
       break;

     case XmMENU_PROCESS_TREE:
       DoProcessMenuTree (widget, XmREPLACE);
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       break;

     case XmMENU_ARM:
       MenuArm (widget);
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       break;

     case XmMENU_DISARM:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       MenuDisarm (widget);
       break;

     case XmMENU_BAR_CLEANUP:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       MenuBarCleanup ((XmRowColumnWidget) widget);
       break;

     case XmMENU_STATUS:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       MenuStatus(widget, (int *) data);
       break;

     case XmMENU_MEMWIDGET_UPDATE:
       if (UpdateMenuHistory((XmRowColumnWidget)XtParent(widget), 
	   widget, True))
	  RC_MemWidget(XtParent(widget)) = widget;
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       break;

     case XmMENU_BUTTON_POPDOWN:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;
       ButtonMenuPopDown (widget, (XEvent *) data, (Boolean *) data2);
       break;

     case XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       _XmRestoreExcludedTearOffToToplevelShell(widget, (XEvent *) data);
       break;

     case XmMENU_RESTORE_TEAROFF_TO_TOPLEVEL_SHELL:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       _XmRestoreTearOffToToplevelShell(widget, (XEvent *) data);
       break;

     case XmMENU_RESTORE_TEAROFF_TO_MENUSHELL:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       _XmRestoreTearOffToMenuShell(widget, (XEvent *) data);
       break;

     case XmMENU_GET_LAST_SELECT_TOPLEVEL:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;
       data2 = va_arg( ap, XtPointer) ;

       GetLastSelectToplevel((XmRowColumnWidget) widget);
       break;

     case XmMENU_TEAR_OFF_ARM:
       flag = va_arg( ap, int) ;
       data = va_arg( ap, XtPointer) ;

       TearOffArm(widget);
       break;

     default:
       break;
  }
va_end( ap) ;
}

static void
#ifdef _NO_PROTO
SetOrGetTextMargins(wid, op, textMargins)
        Widget wid ;
        unsigned char op;
        XmBaselineMargins *textMargins ;
#else
SetOrGetTextMargins(
        Widget wid,
#if NeedWidePrototypes
        unsigned int op,
#else
        unsigned char op,
#endif /* NeedWidePrototypes */
        XmBaselineMargins *textMargins )
#endif
{

  XmPrimitiveClassExt           *wcePtr;
  WidgetClass wc = XtClass(wid);

  wcePtr = _XmGetPrimitiveClassExtPtr(wc, NULLQUARK);

  textMargins->get_or_set = op;

  if (*wcePtr && (*wcePtr)->widget_margins)
    (*((*wcePtr)->widget_margins)) (wid, textMargins) ;
}

static void
#ifdef _NO_PROTO
TopOrBottomAlignment(m, h, shadow, highlight, baseline, margin_top, margin_height,
                 text_height, new_height, start_i, end_i)
	XmRowColumnWidget m;
	Dimension h;
	Dimension shadow;
	Dimension highlight;
	Dimension baseline;
	Dimension margin_top;
	Dimension margin_height;
	Dimension text_height;
	Dimension *new_height;
        int start_i;
        int end_i;
#else
TopOrBottomAlignment(
	XmRowColumnWidget m,
#if NeedWidePrototypes
	int h,
	int shadow,
	int highlight,
	int baseline,
	int margin_top,
	int margin_height,
	int text_height,
#else
        Dimension h,
        Dimension shadow,
        Dimension highlight,
        Dimension baseline,
        Dimension margin_top,
        Dimension margin_height,
        Dimension text_height,
#endif /* NeedWidePrototypes */
	Dimension *new_height,
        int start_i,
        int end_i)
#endif
{
  XmRCKidGeometry kg = RC_Boxes (m);

  while (start_i < end_i)
  {
    if (XmIsLabel(kg [start_i].kid))
    {
      kg[start_i].margin_top = Lab_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = Lab_MarginBottom(kg[start_i].kid);
    
      if (Lab_Shadow(kg [start_i].kid) < shadow)
      {
         kg[start_i].margin_top += shadow - Lab_Shadow(kg [start_i].kid);
         kg[start_i].box.height += shadow - Lab_Shadow(kg [start_i].kid);
      }
      if (Lab_Highlight(kg [start_i].kid) < highlight)
      {
         kg[start_i].margin_top += highlight - Lab_Highlight(kg [start_i].kid);
         kg[start_i].box.height += highlight - Lab_Highlight(kg [start_i].kid);
      }
      if (Lab_MarginTop(kg [start_i].kid) < margin_top)
      {
         kg[start_i].margin_top += margin_top - Lab_MarginTop(kg [start_i].kid);
         kg[start_i].box.height += margin_top - Lab_MarginTop(kg [start_i].kid);
      }
      if (Lab_MarginHeight(kg [start_i].kid) < margin_height)
      {
         kg[start_i].margin_top += margin_height - Lab_MarginHeight(kg [start_i].kid);
         kg[start_i].box.height += margin_height - Lab_MarginHeight(kg [start_i].kid);
      }
      if (AlignmentBottom (m))
       if (Lab_TextRect_height(kg [start_i].kid) < text_height)
       {
         kg[start_i].margin_top += text_height - Lab_TextRect_height(kg [start_i].kid);
         kg[start_i].box.height += text_height - Lab_TextRect_height(kg [start_i].kid);
       }
      if (kg[start_i].box.height < h)
      {
        kg[start_i].margin_bottom += h - kg[start_i].box.height;
        kg[start_i].box.height = h;
      }
    }
    else if (XmIsLabelGadget(kg [start_i].kid))
    {
      kg[start_i].margin_top = LabG_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = LabG_MarginBottom(kg[start_i].kid);

      if (LabG_Shadow(kg [start_i].kid) < shadow)
      {
         kg[start_i].margin_top += shadow - LabG_Shadow(kg [start_i].kid);
         kg[start_i].box.height += shadow - LabG_Shadow(kg [start_i].kid);
      }

      if (LabG_Highlight(kg [start_i].kid) < highlight)
      {
         kg[start_i].margin_top += highlight - LabG_Highlight(kg [start_i].kid);
         kg[start_i].box.height += highlight - LabG_Highlight(kg [start_i].kid);
      }
      if (LabG_MarginTop(kg [start_i].kid) < margin_top)
      {
         kg[start_i].margin_top += margin_top - LabG_MarginTop(kg [start_i].kid);
         kg[start_i].box.height += margin_top - LabG_MarginTop(kg [start_i].kid);
      }
      if (LabG_MarginHeight(kg [start_i].kid) < margin_height)
      {
         kg[start_i].margin_top += margin_height - LabG_MarginHeight(kg [start_i].kid);
         kg[start_i].box.height += margin_height - LabG_MarginHeight(kg [start_i].kid);
      }
      if (AlignmentBottom (m))
       if (LabG_TextRect_height(kg [start_i].kid) < text_height)
       {
         kg[start_i].margin_top += text_height - LabG_TextRect_height(kg [start_i].kid);
         kg[start_i].box.height += text_height - LabG_TextRect_height(kg [start_i].kid);
       }
      if (kg[start_i].box.height < h)
      {
        kg[start_i].margin_bottom += h - kg[start_i].box.height;
        kg[start_i].box.height = h;
      }
    }
    else if (XmIsText(kg[start_i].kid) || XmIsTextField(kg[start_i].kid))
    {
      XmBaselineMargins textMargins;

      SetOrGetTextMargins(kg [start_i].kid, XmBASELINE_GET, &textMargins);
      kg[start_i].margin_top = textMargins.margin_top;
      kg[start_i].margin_bottom = textMargins.margin_bottom;
      
      if (textMargins.shadow < shadow)
      {
         kg[start_i].margin_top += shadow - textMargins.shadow;
         kg[start_i].box.height += shadow - textMargins.shadow;
      }

      if (textMargins.highlight < highlight)
      {
         kg[start_i].margin_top += highlight - textMargins.highlight;
         kg[start_i].box.height += highlight - textMargins.highlight;
      }

      if (textMargins.margin_top < (margin_top + margin_height))
      {
         kg[start_i].margin_top += (margin_top + margin_height) - textMargins.margin_top;
         kg[start_i].box.height += (margin_top + margin_height) - textMargins.margin_top;
      }

      if (AlignmentBottom (m))
       if (textMargins.text_height < text_height)
       {
         kg[start_i].margin_top += text_height - textMargins.text_height;
         kg[start_i].box.height += text_height - textMargins.text_height;
       }
      if (kg[start_i].box.height < h)
        kg[start_i].box.height = h;
    }
    if (kg[start_i].box.height > h)
      if (kg[start_i].box.height > *new_height)
        *new_height = kg[start_i].box.height;
    start_i++;
  }
}


static void
#ifdef _NO_PROTO
BaselineAlignment(m, h, shadow, highlight, baseline, new_height, start_i, end_i)
	XmRowColumnWidget m;
	Dimension h;
	Dimension shadow;
	Dimension highlight;
	Dimension baseline;
	Dimension *new_height;
        int start_i;
        int end_i;
#else
BaselineAlignment(
	XmRowColumnWidget m,
#if NeedWidePrototypes
	int h,
	int shadow,
	int highlight,
	int baseline,
#else
        Dimension h,
        Dimension shadow,
        Dimension highlight,
        Dimension baseline,
#endif /* NeedWidePrototypes */
	Dimension *new_height,
        int start_i,
        int end_i)
#endif
{
   XmRCKidGeometry kg = RC_Boxes (m);

   while (start_i < end_i)
   {
    if (XmIsLabel (kg [start_i].kid))
    {
        
      kg[start_i].margin_top = Lab_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = Lab_MarginBottom(kg[start_i].kid);

      if (!Lab_IsPixmap (kg [start_i].kid))
      {
        if (kg[start_i].baseline < baseline)
        {
          kg[start_i].margin_top += baseline - kg[start_i].baseline;
          if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > h)
          {
             if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > *new_height)
               *new_height = kg[start_i].box.height + (baseline - kg[start_i].baseline);
             kg[start_i].box.height += baseline - kg[start_i].baseline;
          }
          else
          {
             kg[start_i].margin_bottom += h  - (kg[start_i].box.height +
                                          (baseline - kg[start_i].baseline));
             kg[start_i].box.height = h;
          }
        }
        else
        {
             kg[start_i].margin_bottom += h  - (kg[start_i].box.height +
                                          (baseline - kg[start_i].baseline));
             kg[start_i].box.height = h;
        }
      }
      else
       kg[start_i].box.height = h;
    }
    else if (XmIsLabelGadget (kg [start_i].kid))
    {
      kg[start_i].margin_top = LabG_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = LabG_MarginBottom(kg[start_i].kid);

      if (!LabG_IsPixmap (kg[start_i].kid))
      {
        if (kg[start_i].baseline < baseline)
        {
          kg[start_i].margin_top += baseline - kg[start_i].baseline;
          if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > h)
          {
             if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > *new_height)
                *new_height = kg[start_i].box.height + (baseline - kg[start_i].baseline);
             kg[start_i].box.height += baseline - kg[start_i].baseline;
          }
          else
          {
             kg[start_i].margin_bottom += h  - (kg[start_i].box.height +
                                          (baseline - kg[start_i].baseline));
             kg[start_i].box.height = h;
          }
        }
        else
        {
            kg[start_i].margin_bottom += h  - (kg[start_i].box.height +
                                          (baseline - kg[start_i].baseline));
            kg[start_i].box.height = h;
        }
      }
      else
       kg[start_i].box.height = h;
    }
    else if (XmIsText (kg [start_i].kid) || XmIsTextField (kg [start_i].kid))
    {
      XmBaselineMargins textMargins;

      SetOrGetTextMargins(kg [start_i].kid, XmBASELINE_GET, &textMargins);
      kg[start_i].margin_top = textMargins.margin_top;
      kg[start_i].margin_bottom = textMargins.margin_bottom;

      if (kg[start_i].baseline < baseline)
      {
        kg[start_i].margin_top += baseline - kg[start_i].baseline;
        if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > h)
        {
           if (kg[start_i].box.height + (baseline - kg[start_i].baseline) > *new_height)
              *new_height = kg[start_i].box.height + (baseline - kg[start_i].baseline);
           kg[start_i].box.height += baseline - kg[start_i].baseline;
        }
        else
          kg[start_i].box.height = h;
      }
      else
        kg[start_i].box.height = h;
    }
    else
      kg[start_i].box.height = h;
    start_i++;
   }
}

static void
#ifdef _NO_PROTO
CenterAlignment(m, h, start_i, end_i)
        XmRowColumnWidget m;
        Dimension h;
        int start_i;
        int end_i;
#else
CenterAlignment(
        XmRowColumnWidget m,
#if NeedWidePrototypes
        int h,
#else
        Dimension h,
#endif /* NeedWidePrototypes */
        int start_i,
        int end_i)
#endif
{

  XmRCKidGeometry kg = RC_Boxes (m);

  while(start_i < end_i)
  {
    if (XmIsLabel (kg [start_i].kid))
    {
      kg[start_i].margin_top = Lab_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = Lab_MarginBottom(kg[start_i].kid);
    }
    else if (XmIsLabelGadget (kg [start_i].kid))
    {
      kg[start_i].margin_top = LabG_MarginTop(kg[start_i].kid);
      kg[start_i].margin_bottom = LabG_MarginBottom(kg[start_i].kid);
    }
    else if (XmIsText (kg [start_i].kid) || XmIsTextField (kg [start_i].kid))
    {
      XmBaselineMargins textMargins;

      SetOrGetTextMargins(kg [start_i].kid, XmBASELINE_GET, &textMargins);
      kg[start_i].margin_top = textMargins.margin_top;
      kg[start_i].margin_bottom = textMargins.margin_bottom;
    }

    kg[start_i++].box.height = h;
  }
}

static XmNavigability
#ifdef _NO_PROTO
WidgetNavigable( wid)
        Widget wid ;
#else
WidgetNavigable(
        Widget wid)
#endif /* _NO_PROTO */
{   
  XmNavigationType nav_type = ((XmManagerWidget) wid)
	                                            ->manager.navigation_type ;
  /* Need to make sure that XmDYNAMIC_DEFAULT_TAB_GROUP causes
   * return value of XmNOT_NAVIGABLE, so that initial call
   * to _XmNavigInitialize from Manager Initialize does nothing.
   */
  if(    wid->core.sensitive
     &&  wid->core.ancestor_sensitive
     &&  ((XmManagerWidget) wid)->manager.traversal_on
     &&  (nav_type != XmDYNAMIC_DEFAULT_TAB_GROUP)    )
    { 
      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	 ||  (    (nav_type == XmTAB_GROUP)
	      &&  !_XmShellIsExclusive( wid))    )
	{
	  return XmDESCENDANTS_TAB_NAVIGABLE ;
	}
      return XmDESCENDANTS_NAVIGABLE ;
    }
  return XmNOT_NAVIGABLE ;
}



#ifdef SCISSOR_CURSOR

static Cursor  
#ifdef _NO_PROTO
CreateScissorCursor(dpy) 
Display * dpy ;
#else
CreateScissorCursor(Display * dpy) 
#endif /* _NO_PROTO */
{
    Pixmap pfore, pmask ;
    XColor fore,back ; 

    pfore = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy),
                                (char *)scissorFore_bits,
                                scissorFore_width,
                                scissorFore_height);
    pmask = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy),
                                (char *)scissorMask_bits,
                                scissorMask_width,
                                scissorMask_height);

    /* find the color elsewhere ? */
    fore.pixel = BlackPixel(dpy, DefaultScreen(dpy)) ;
    back.pixel = WhitePixel(dpy, DefaultScreen(dpy)) ;
    XQueryColor(dpy,DefaultColormap (dpy, DefaultScreen(dpy)), &fore) ;
    XQueryColor(dpy, DefaultColormap (dpy, DefaultScreen(dpy)), &back) ;

    return XCreatePixmapCursor(dpy, pfore, pmask, &fore, &back,
                             scissorFore_x_hot, scissorFore_y_hot);
}

      
/*
 * This is the callback for changing on and off the scissor cursor in the
 * tear off menu button 
 */
static void 
#ifdef _NO_PROTO
ArmDisarmTearOffCB( w, client_data, callback )
        Widget w ;
        XtPointer client_data ;
        XmAnyCallbackStruct *callback ;
#else
ArmDisarmTearOffCB(
        Widget w,
        XtPointer client_data,
        XmAnyCallbackStruct *callback )
#endif /* _NO_PROTO */
{
    static Cursor regular_cursor ;
    Time _time = __XmGetDefaultTime(w, callback -> event);

    if ((int)client_data == 0) {  /* Arm the tear off control */
      /* save the current cursor */
      regular_cursor = _XmGetMenuCursorByScreen(XtScreen(w));

      XChangeActivePointerGrab(XtDisplay(w),
                               ButtonPressMask | ButtonReleaseMask |
                               EnterWindowMask | LeaveWindowMask,
                               scissor_cursor,
                               _time);

    } else {
      /* put it back */
      XChangeActivePointerGrab(XtDisplay(w),
                               ButtonPressMask | ButtonReleaseMask |
                               EnterWindowMask | LeaveWindowMask,
                               regular_cursor,
                               _time);
    }
}

#endif /* SCISSOR_CURSOR */

/*************************************************
 * This function extracts a time from an event or
 * returns the last processed time if the event
 * is NULL or isn't an event with a timestamp
 *************************************************/

Time 
#ifdef _NO_PROTO
__XmGetDefaultTime(wid, event)
     Widget wid;
     XEvent *event;
#else
__XmGetDefaultTime(Widget wid, XEvent *event)
#endif /* _NO_PROTO */
{
  return(CurrentTime);

  /* 
  if (event == NULL)
    return(XtLastTimestampProcessed(XtDisplay(wid)));
  else if (event -> type == ButtonPress ||
	   event -> type == ButtonRelease)
    return(event -> xbutton.time);
  else if (event -> type == KeyPress ||
	   event -> type == KeyRelease)
    return(event -> xkey.time);
  else if (event -> type == MotionNotify)
    return(event -> xmotion.time);
  else if (event -> type == EnterNotify ||
	   event -> type == LeaveNotify)
    return(event -> xcrossing.time);
  else
    return(XtLastTimestampProcessed(XtDisplay(wid)));
    */
}
