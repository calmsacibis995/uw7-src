#pragma ident	"@(#)m1.2libs:Xm/Transltns.c	1.3"
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

/* Define _XmConst before including TransltnsP.h, so that the
 * declarations will be in agreement with the definitions.
 */
#ifndef _XmConst
#if (defined(__STDC__) && __STDC__)  ||  !defined( NO_CONST )
#define _XmConst const
#else
#define _XmConst
#endif /* __STDC__ */
#endif /* _XmConst */

#include <Xm/TransltnsP.h>


#ifndef XT_HAS_TRANS_FIX

/* This is the old-style translation table, which must be used
 * when libXm is to be used with versions of libXt which do not
 * have the :-production translation fix.
 *
 * For systems that do have this patch (fix-trackers seq 2797),
 * translations have been cleaned up and reorganized as follows:
 *  - Sort translations based roughly on the event type.  Canonical
 *      order is: Map/Unmap, Enter/Leave, Focus, Btn, Key.
 *  - Within the <Key> translations group by key, putting osfMumble
 *      keys first.
 *  - Prefix osf key translations with ':', and reorder as necessary
 *      to remove ~mod.
 */


/*** ArrowB.c ***/ 
externaldef(translations) _XmConst char _XmArrowB_defaultTranslations[] = "\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfHelp:Help()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";

/*** BulletinB.c ***/
externaldef(translations) _XmConst char _XmBulletinB_defaultTranslations[] = "\
<Key>osfHelp:ManagerGadgetHelp()\n\
<Key>space:ManagerGadgetSelect()\n\
<Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>:ManagerGadgetKeyInput()\n\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()";

/*** CascadeB.c ***/
externaldef(translations) _XmConst char _XmCascadeB_menubar_events[] = "\
<BtnDown>:MenuBarSelect()\n\
<EnterWindow>Normal:MenuBarEnter()\n\
<LeaveWindow>Normal:MenuBarLeave()\n\
<BtnUp>:DoSelect()\n\
<Key>osfSelect:KeySelect()\n\
<Key>osfActivate:KeySelect()\n\
<Key>osfHelp:Help()\n\
<Key>osfCancel:CleanupMenuBar()\n\
~s<Key>Return:KeySelect()\n\
~s<Key>space:KeySelect()";

externaldef(translations) _XmConst char _XmCascadeB_p_events[] = "\
<BtnDown>:StartDrag()\n\
<EnterWindow>:DelayedArm()\n\
<LeaveWindow>:CheckDisarm()\n\
<BtnUp>:DoSelect()\n\
<Key>osfSelect:KeySelect()\n\
<Key>osfActivate:KeySelect()\n\
<Key>osfHelp:Help()\n\
<Key>osfCancel:CleanupMenuBar()\n\
~s<Key>Return:KeySelect()\n\
~s<Key>space:KeySelect()";

/*** DrawingA.c ***/
externaldef(translations) _XmConst char _XmDrawingA_defaultTranslations[] = "\
~s ~m ~a <Key>Return:DrawingAreaInput() ManagerParentActivate()\n\
<Key>Return:DrawingAreaInput() ManagerGadgetSelect()\n\
<Key>osfActivate:DrawingAreaInput() ManagerParentActivate()\n\
<Key>osfCancel:DrawingAreaInput() ManagerParentCancel()\n\
<Key>osfHelp:DrawingAreaInput() ManagerGadgetHelp()\n\
<Key>space:DrawingAreaInput() ManagerGadgetSelect()\n\
<Key>osfSelect:DrawingAreaInput() ManagerGadgetSelect()\n\
<KeyDown>:DrawingAreaInput() ManagerGadgetKeyInput()\n\
<KeyUp>:DrawingAreaInput()\n\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:DrawingAreaInput() ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:DrawingAreaInput() ManagerGadgetActivate()\n\
<Btn1Up>:DrawingAreaInput() ManagerGadgetActivate()\n\
<Btn1Down>(2+):DrawingAreaInput() ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):DrawingAreaInput() ManagerGadgetMultiActivate()\n\
<Btn2Down>:DrawingAreaInput() ManagerGadgetDrag()\n\
<BtnDown>:DrawingAreaInput()\n\
<BtnUp>:DrawingAreaInput()";

externaldef(translations) _XmConst char _XmDrawingA_traversalTranslations[] = "\
<Key>osfUp:DrawingAreaInput() ManagerGadgetTraverseUp()\n\
<Key>osfDown:DrawingAreaInput() ManagerGadgetTraverseDown()\n\
<Key>osfLeft:DrawingAreaInput() ManagerGadgetTraverseLeft()\n\
<Key>osfRight:DrawingAreaInput() ManagerGadgetTraverseRight()\n\
<EnterWindow>:ManagerEnter()\n\
<LeaveWindow>:ManagerLeave()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()\n\
s<Key>Tab:DrawingAreaInput() ManagerGadgetPrevTabGroup()\n\
~s<Key>Tab:DrawingAreaInput() ManagerGadgetNextTabGroup()\n\
<Key>osfBeginLine:DrawingAreaInput() ManagerGadgetTraverseHome()";

/*** DrawnB.c ***/
externaldef(translations) _XmConst char _XmDrawnB_defaultTranslations[] = "\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfHelp:Help()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";

/*** Frame.c ***/
externaldef(translations) _XmConst char _XmFrame_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<FocusIn>:FocusIn()\n\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
<Btn1Down>:Arm()\n\
<Btn1Up>:Activate()\n\
<Btn2Down>:ManagerGadgetDrag()";

/*** Label.c ***/
externaldef(translations) _XmConst char _XmLabel_defaultTranslations[] = "\
<Btn2Down>:ProcessDrag()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfHelp:Help()";

externaldef(translations) _XmConst char _XmLabel_menuTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Key>osfHelp:Help()";

externaldef(translations) _XmConst char _XmLabel_menu_traversal_events[] = "\
<Unmap>:Unmap()\n\
<FocusOut>:FocusOut()\n\
<FocusIn>:FocusIn()\n\
<Key>osfCancel:MenuEscape()\n\
<Key>osfLeft:MenuTraverseLeft()\n\
<Key>osfRight:MenuTraverseRight()\n\
<Key>osfUp:MenuTraverseUp()\n\
<Key>osfDown:MenuTraverseDown()";

/*** List.c ***/
externaldef(translations) _XmConst char _XmList_ListXlations1[] = "\
s c <Key>osfBeginLine:ListBeginDataExtend()\n\
c <Key>osfBeginLine:ListBeginData()\n\
<Key>osfBeginLine:ListBeginLine()\n\
s c <Key>osfEndLine:ListEndDataExtend()\n\
c <Key>osfEndLine:ListEndData()\n\
<Key>osfEndLine:ListEndLine()\n\
<Key>osfPageLeft:ListLeftPage()\n\
c <Key>osfPageUp:ListLeftPage()\n\
~c <Key>osfPageUp:ListPrevPage()\n\
<Key>osfPageRight:ListRightPage()\n\
c <Key>osfPageDown:ListRightPage()\n\
~c <Key>osfPageDown:ListNextPage()\n\
~c ~s <KeyDown>osfSelect:ListKbdBeginSelect()\n\
~c ~s <KeyUp>osfSelect:ListKbdEndSelect()\n\
~Ctrl Shift <KeyDown>osfSelect:ListKbdBeginExtend()\n\
~Ctrl Shift <KeyUp>osfSelect:ListKbdEndExtend()\n\
<Key>osfActivate:ListKbdActivate()\n\
<Key>osfAddMode:ListAddMode()\n\
<Key>osfHelp:PrimitiveHelp()\n\
<Key>osfCancel:ListKbdCancel()\n";

externaldef(translations) _XmConst char _XmList_ListXlations2[] = "\
~c ~s <Key>osfLeft:ListLeftChar()\n\
c ~s <Key>osfLeft:ListLeftPage()\n\
~c ~s <Key>osfRight:ListRightChar()\n\
c ~s <Key>osfRight:ListRightPage()\n\
s <Key>osfUp:ListExtendPrevItem()\n\
~s <Key>osfUp:ListPrevItem()\n\
s <Key>osfDown:ListExtendNextItem()\n\
~s <Key>osfDown:ListNextItem()\n\
~s c ~m ~a <Key>slash:ListKbdSelectAll()\n\
~s c ~m ~a <Key>backslash:ListKbdDeSelectAll()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
~s ~m ~a <Key>Return:ListKbdActivate()\n\
~s ~m ~a <KeyDown>space:ListKbdBeginSelect()\n\
~s ~m ~a <KeyUp>space:ListKbdEndSelect()\n\
s ~m ~a <KeyDown>space:ListKbdBeginExtend()\n\
s ~m ~a <KeyUp>space:ListKbdEndExtend()\n\
~s c ~m ~a <Key>osfInsert:ListCopyToClipboard()\n\
<Key>osfCopy:ListCopyToClipboard()\n\
Button1<Motion>:ListButtonMotion()\n\
s ~m ~a <Btn1Down>:ListBeginExtend()\n\
s ~m ~a <Btn1Up>:ListEndExtend()\n\
c ~s ~m ~a <Btn1Down>:ListBeginToggle()\n\
c ~s ~m ~a <Btn1Up>:ListEndToggle()\n\
~s ~c ~m ~a <Btn1Down>:ListBeginSelect()\n\
~s ~c ~m ~a <Btn1Up>:ListEndSelect()\n\
<Btn2Down>:ListProcessDrag()\n\
<Enter>:ListEnter()\n\
<Leave>:ListLeave()\n\
<FocusIn>:ListFocusIn()\n\
<FocusOut>:ListFocusOut()\n\
<Unmap>:PrimitiveUnmap()";

/*** Manager.c ***/
externaldef(translations) _XmConst char _XmManager_managerTraversalTranslations[] = "\
<Key>osfBeginLine:ManagerGadgetTraverseHome()\n\
<Key>osfUp:ManagerGadgetTraverseUp()\n\
<Key>osfDown:ManagerGadgetTraverseDown()\n\
<Key>osfLeft:ManagerGadgetTraverseLeft()\n\
<Key>osfRight:ManagerGadgetTraverseRight()\n\
s ~m ~a <Key>Tab:ManagerGadgetPrevTabGroup()\n\
~m ~a <Key>Tab:ManagerGadgetNextTabGroup()\n\
<EnterWindow>:ManagerEnter()\n\
<LeaveWindow>:ManagerLeave()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()";

externaldef(translations) _XmConst char _XmManager_defaultTranslations[] = "\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>osfHelp:ManagerGadgetHelp()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()\n\
<Key>:ManagerGadgetKeyInput()\n\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()";

/*** MenuShell.c ***/
externaldef(translations) _XmConst char _XmMenuShell_translations [] = "\
<BtnDown>:ClearTraversal()\n\
<BtnUp>:MenuShellPopdownDone()";

/*** Primitive.c ***/
externaldef(translations) _XmConst char _XmPrimitive_defaultTranslations[] = "\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfBeginLine:PrimitiveTraverseHome()\n\
<Key>osfHelp:PrimitiveHelp()\n\
<Key>osfUp:PrimitiveTraverseUp()\n\
<Key>osfDown:PrimitiveTraverseDown()\n\
<Key>osfLeft:PrimitiveTraverseLeft()\n\
<Key>osfRight:PrimitiveTraverseRight()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
<Unmap>:PrimitiveUnmap()";

/*** PushB.c ***/
externaldef(translations) _XmConst char _XmPushB_defaultTranslations[] = "\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
<Btn2Down>:ProcessDrag()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfHelp:Help()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";

externaldef(translations) _XmConst char _XmPushB_menuTranslations[] = "\
<BtnDown>:BtnDown()\n\
<BtnUp>:BtnUp()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfActivate:ArmAndActivate()\n\
<Key>osfCancel:MenuEscape()\n\
<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:ArmAndActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";

/*** RowColumn.c ***/
externaldef(translations) _XmConst char _XmRowColumn_menu_traversal_table[] = "\
<Key>osfHelp:MenuHelp()\n\
<Key>osfLeft:MenuGadgetTraverseLeft()\n\
<Key>osfRight:MenuGadgetTraverseRight()\n\
<Key>osfUp:MenuGadgetTraverseUp()\n\
<Key>osfDown:MenuGadgetTraverseDown()\n\
<Unmap>:MenuUnmap()\n\
<FocusIn>:MenuFocusIn()\n\
<FocusOut>:MenuFocusOut()\n\
<EnterWindow>Normal:MenuEnter()";

externaldef(translations) _XmConst char _XmRowColumn_option_table[]= "\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>osfHelp:MenuHelp()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()\n\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()";

externaldef(translations) _XmConst char _XmRowColumn_bar_table[]= "\
<Key>osfSelect:MenuBarGadgetSelect()\n\
<Key>osfActivate:MenuBarGadgetSelect()\n\
<Key>osfHelp:MenuHelp()\n\
<Key>osfCancel:MenuGadgetEscape()\n\
~s ~m ~a <Key>Return:MenuBarGadgetSelect()\n\
~s ~m ~a <Key>space:MenuBarGadgetSelect()\n\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()";

externaldef(translations) _XmConst char _XmRowColumn_menu_table[]= "\
<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>osfActivate:ManagerGadgetSelect()\n\
<Key>osfHelp:MenuHelp()\n\
<Key>osfCancel:MenuGadgetEscape()\n\
~s ~m ~a <Key>Return:ManagerGadgetSelect()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()\n\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()";

/*** Sash.c ***/
externaldef(translations) _XmConst char _XmSash_defTranslations[] = "\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfHelp:Help()\n\
c ~s <Key>osfUp:SashAction(Key,LargeIncr,Up)\n\
~c ~s <Key>osfUp:SashAction(Key,DefaultIncr,Up)\n\
c ~s <Key>osfDown:SashAction(Key,LargeIncr,Down)\n\
~c ~s <Key>osfDown:SashAction(Key,DefaultIncr,Down)\n\
s ~m ~a <Key>Tab:PrevTabGroup()\n\
~m ~a <Key>Tab:NextTabGroup()\n\
~c ~s ~m ~a <Btn1Down>:SashAction(Start)\n\
~c ~s ~m ~a <Btn1Motion>:SashAction(Move)\n\
~c ~s ~m ~a <Btn1Up>:SashAction(Commit)\n\
~c ~s ~m ~a <Btn2Down>:SashAction(Start)\n\
~c ~s ~m ~a <Btn2Motion>:SashAction(Move)\n\
~c ~s ~m ~a <Btn2Up>:SashAction(Commit)\n\
<FocusIn>:SashFocusIn()\n\
<FocusOut>:SashFocusOut()\n\
<Unmap>:PrimitiveUnmap()\n\
<EnterWindow>:enter()\n\
<LeaveWindow>:leave()";

/*** ScrollBar.c ***/
externaldef(translations) _XmConst char _XmScrollBar_defaultTranslations[] = "\
~s ~c ~m ~a <Btn1Down>:Select()\n\
~s ~c ~m ~a <Btn1Up>:Release()\n\
~s ~c ~m ~a Button1<PtrMoved>:Moved()\n\
~s ~c ~m ~a <Btn2Down>:Select()\n\
~s ~c ~m ~a <Btn2Up>:Release()\n\
~s ~c ~m ~a Button2<PtrMoved>:Moved()\n\
~s c ~m ~a <Btn1Down>:TopOrBottom()\n\
~s c ~m ~a <Btn1Up>:Release()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:CancelDrag()\n\
<Key>osfBeginLine:TopOrBottom()\n\
<Key>osfEndLine:TopOrBottom()\n\
<Key>osfPageLeft:PageUpOrLeft(1)\n\
c <Key>osfPageUp:PageUpOrLeft(1)\n\
<Key>osfPageUp:PageUpOrLeft(0)\n\
<Key>osfPageRight:PageDownOrRight(1)\n\
c <Key>osfPageDown:PageDownOrRight(1)\n\
<Key>osfPageDown:PageDownOrRight(0)\n\
<Key>osfHelp:PrimitiveHelp()\n\
~s ~c <Key>osfUp:IncrementUpOrLeft(0)\n\
~s ~c <Key>osfDown:IncrementDownOrRight(0)\n\
~s ~c <Key>osfLeft:IncrementUpOrLeft(1)\n\
~s ~c <Key>osfRight:IncrementDownOrRight(1)\n\
~s c <Key>osfUp:PageUpOrLeft(0)\n\
~s c <Key>osfDown:PageDownOrRight(0)\n\
~s c <Key>osfLeft:PageUpOrLeft(1)\n\
~s c <Key>osfRight:PageDownOrRight(1)\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
<Unmap>:PrimitiveUnmap()\n\
<Enter>:PrimitiveEnter()\n\
<Leave>:PrimitiveLeave()";

/*** ScrolledW.c ***/
externaldef(translations) _XmConst char _XmScrolledW_ScrolledWindowXlations[] = "\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
<Key>osfActivate:ManagerParentActivate()\n\
<Key>osfCancel:ManagerParentCancel()\n\
c <Key>osfBeginLine:SWTopLine()\n\
<Key>osfBeginLine:SWBeginLine()\n\
c <Key>osfEndLine:SWBottomLine()\n\
<Key>osfEndLine:SWEndLine()\n\
<Key>osfPageLeft:SWLeftPage()\n\
c <Key>osfPageUp:SWLeftPage()\n\
<Key>osfPageRight:SWRightPage()\n\
c <Key>osfPageDown:SWRightPage()\n\
<Key>osfPageUp:SWUpPage()\n\
<Key>osfPageDown:SWDownPage()\n\
<Key>osfHelp:ManagerGadgetHelp()\n\
<Key>osfUp:ManagerGadgetTraverseUp()\n\
<Key>osfDown:ManagerGadgetTraverseDown()\n\
<Key>osfLeft:ManagerGadgetTraverseLeft()\n\
<Key>osfRight:ManagerGadgetTraverseRight()\n\
s ~m ~a<Key>Tab:ManagerGadgetPrevTabGroup()\n\
~m ~a <Key>Tab:ManagerGadgetNextTabGroup()\n\
<EnterWindow>:ManagerEnter()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()\n\
<Btn2Down>:ManagerGadgetDrag()";

externaldef(translations) _XmConst char _XmScrolledW_ClipWindowTranslationTable[] = "\
c <Key>osfBeginLine:SWTopLineGrab()\n\
<Key>osfBeginLine:SWBeginLineGrab()\n\
c <Key>osfEndLine:SWBottomLineGrab()\n\
<Key>osfEndLine:SWEndLineGrab()\n\
<Key>osfPageLeft:SWLeftPageGrab()\n\
c <Key>osfPageUp:SWLeftPageGrab()\n\
<Key>osfPageRight:SWRightPageGrab()\n\
c <Key>osfPageDown:SWRightPageGrab()\n\
<Key>osfPageUp:SWUpPageGrab()\n\
<Key>osfPageDown:SWDownPageGrab()\n\
<MapNotify>:SWNoop()";

externaldef(translations) _XmConst char _XmScrolledW_WorkWindowTranslationTable[] = "\
c ~s <Key>osfBeginLine:SWTopLineWork()\n\
~c ~s <Key>osfBeginLine:SWBeginLineWork()\n\
c ~s <Key>osfEndLine:SWBottomLineWork()\n\
~c ~s <Key>osfEndLine:SWEndLineWork()";


/*** SelectioB.c ***/
externaldef(translations) _XmConst char _XmSelectioB_defaultTextAccelerators[] = "\
\043override\n\
<Key>osfUp:SelectionBoxUpOrDown(0)\n\
<Key>osfDown:SelectionBoxUpOrDown(1)\n\
<Key>osfBeginLine:SelectionBoxUpOrDown(2)\n\
<Key>osfEndLine:SelectionBoxUpOrDown(3)\n\
s c ~m ~a <Key>space:SelectionBoxRestore()";

/*** TearOffB.c ***/
externaldef(translations) _XmConst char _XmTearOffB_overrideTranslations[] = "\
<Btn2Down>:BDrag()\n\
<BtnUp>:BActivate()\n\
<Key>osfSelect:KActivate()\n\
<Key>osfActivate:KActivate()\n\
~s ~m ~a <Key>Return:KActivate()\n\
~s ~m ~a <Key>space:KActivate()";

/*** TextF.c ***/
externaldef(translations) _XmConst char _XmTextF_EventBindings1[] = "\
m <Key>osfPrimaryPaste:cut-primary()\n\
a <Key>osfPrimaryPaste:cut-primary()\n\
<Key>osfPrimaryPaste:copy-primary()\n\
m <Key>osfCut:cut-primary()\n\
a <Key>osfCut:cut-primary()\n\
<Key>osfCut:cut-clipboard()\n\
<Key>osfPaste:paste-clipboard()\n\
m <Key>osfCopy:copy-primary()\n\
a <Key>osfCopy:copy-primary()\n\
<Key>osfCopy:copy-clipboard()\n\
s <Key>osfBeginLine:beginning-of-line(extend)\n\
<Key>osfBeginLine:beginning-of-line()\n\
s <Key>osfEndLine:end-of-line(extend)\n\
<Key>osfEndLine:end-of-line()\n\
s <Key>osfPageLeft:page-left(extend)\n\
<Key>osfPageLeft:page-left()\n\
s c<Key>osfPageUp:page-left(extend)\n\
c <Key>osfPageUp:page-left()\n\
s <Key>osfPageRight:page-right(extend)\n\
<Key>osfPageRight:page-right()\n\
s c <Key>osfPageDown:page-right(extend)\n\
c <Key>osfPageDown:page-right()\n\
<Key>osfClear:clear-selection()\n\
~m ~a <Key>osfBackSpace:delete-previous-character()\n\
s m <Key>osfDelete:cut-primary()\n\
s a <Key>osfDelete:cut-primary()\n\
s ~c ~m ~a <Key>osfDelete:cut-clipboard()\n\
~s c ~m ~a <Key>osfDelete:delete-to-end-of-line()\n\
~s ~c ~m ~a <Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextF_EventBindings2[] = "\
c m <Key>osfInsert:copy-primary()\n\
c a <Key>osfInsert:copy-primary()\n\
s ~c ~m ~a <Key>osfInsert:paste-clipboard()\n\
~s c ~m ~a <Key>osfInsert:copy-clipboard()\n\
~c s ~m ~a <Key>osfSelect:key-select()\n\
~c ~s ~m ~a <Key>osfSelect:set-anchor()\n\
<Key>osfActivate:activate()\n\
<Key>osfAddMode:toggle-add-mode()\n\
<Key>osfHelp:Help()\n\
<Key>osfCancel:process-cancel()\n\
s c <Key>osfLeft:backward-word(extend)\n\
c <Key>osfLeft:backward-word()\n\
s <Key>osfLeft:key-select(left)\n\
<Key>osfLeft:backward-character()\n\
s c <Key>osfRight:forward-word(extend)\n\
c <Key>osfRight:forward-word()\n\
s <Key>osfRight:key-select(right)\n\
<Key>osfRight:forward-character()\n\
<Key>osfUp:traverse-prev()\n\
<Key>osfDown:traverse-next()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
~m ~a <Key>Tab:next-tab-group()\n\
~s ~m ~a <Key>Return:activate()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";

externaldef(translations) _XmConst char _XmTextF_EventBindings3[] = "\
~c s ~m ~a <Btn1Down>:extend-start()\n\
c ~s ~m ~a <Btn1Down>:move-destination()\n\
~c ~s ~m ~a <Btn1Down>:grab-focus()\n\
~c ~m ~a <Btn1Motion>:extend-adjust()\n\
~c ~m ~a <Btn1Up>:extend-end()\n\
<Btn2Down>:process-bdrag()\n\
m ~a <Btn2Motion>:secondary-adjust()\n\
~m a <Btn2Motion>:secondary-adjust()\n\
~s <Btn2Up>:copy-to()\n\
~c <Btn2Up>:move-to()\n\
<FocusIn>:focusIn()\n\
<FocusOut>:focusOut()\n\
<Enter>:enter()\n\
<Leave>:leave()\n\
<Unmap>:unmap()";

/*** TextIn.c ***/
externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings1[] = "\
m <Key>osfPrimaryPaste:cut-primary()\n\
a <Key>osfPrimaryPaste:cut-primary()\n\
<Key>osfPrimaryPaste:copy-primary()\n\
m <Key>osfCut:cut-primary()\n\
a <Key>osfCut:cut-primary()\n\
<Key>osfCut:cut-clipboard()\n\
<Key>osfPaste:paste-clipboard()\n\
m <Key>osfCopy:copy-primary()\n\
a <Key>osfCopy:copy-primary()\n\
<Key>osfCopy:copy-clipboard()\n\
s c <Key>osfBeginLine:beginning-of-file(extend)\n\
c <Key>osfBeginLine:beginning-of-file()\n\
s <Key>osfBeginLine:beginning-of-line(extend)\n\
<Key>osfBeginLine:beginning-of-line()\n\
s c <Key>osfEndLine:end-of-file(extend)\n\
c <Key>osfEndLine:end-of-file()\n\
s <Key>osfEndLine:end-of-line(extend)\n\
<Key>osfEndLine:end-of-line()\n\
s <Key>osfPageLeft:page-left(extend)\n\
<Key>osfPageLeft:page-left()\n\
s c <Key>osfPageUp:page-left(extend)\n\
c <Key>osfPageUp:page-left()\n\
s <Key>osfPageUp:previous-page(extend)\n\
<Key>osfPageUp:previous-page()\n\
s <Key>osfPageRight:page-right(extend)\n\
<Key>osfPageRight:page-right()\n\
s c <Key>osfPageDown:page-right(extend)\n\
c <Key>osfPageDown:page-right()\n\
s <Key>osfPageDown:next-page(extend)\n\
<Key>osfPageDown:next-page()\n\
<Key>osfClear:clear-selection()\n\
~m ~a <Key>osfBackSpace:delete-previous-character()\n\
s m <Key>osfDelete:cut-primary()\n\
s a <Key>osfDelete:cut-primary()\n\
s ~c ~m ~a <Key>osfDelete:cut-clipboard()\n\
~s c ~m ~a <Key>osfDelete:delete-to-end-of-line()\n\
~s ~c ~m ~a <Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings2[] = "\
c m <Key>osfInsert:copy-primary()\n\
c a <Key>osfInsert:copy-primary()\n\
s ~c ~m ~a <Key>osfInsert:paste-clipboard()\n\
~s c ~m ~a <Key>osfInsert:copy-clipboard()\n\
~c s ~m ~a <Key>osfSelect:key-select()\n\
~c ~s ~m ~a <Key>osfSelect:set-anchor()\n\
<Key>osfActivate:activate()\n\
<Key>osfAddMode:toggle-add-mode()\n\
<Key>osfHelp:Help()\n\
<Key>osfCancel:process-cancel()\n\
s c <Key>osfLeft:backward-word(extend)\n\
c <Key>osfLeft:backward-word()\n\
s <Key>osfLeft:key-select(left)\n\
<Key>osfLeft:backward-character()\n\
s c <Key>osfRight:forward-word(extend)\n\
c <Key>osfRight:forward-word()\n\
s <Key>osfRight:key-select(right)\n\
<Key>osfRight:forward-character()\n\
s c <Key>osfUp:backward-paragraph(extend)\n\
c <Key>osfUp:backward-paragraph()\n\
s <Key>osfUp:process-shift-up()\n\
<Key>osfUp:process-up()\n\
s c <Key>osfDown:forward-paragraph(extend)\n\
c <Key>osfDown:forward-paragraph()\n\
s <Key>osfDown:process-shift-down()\n\
<Key>osfDown:process-down()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
c ~m ~a <Key>Tab:next-tab-group()\n\
~m ~a <Key>Tab:process-tab()\n\
c ~s ~m ~a <Key>Return:activate()\n\
~c ~s ~m ~a <Key>Return:process-return()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";

externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings3[] = "\
~c s ~m ~a <Btn1Down>:extend-start()\n\
c ~s ~m ~a <Btn1Down>:move-destination()\n\
~c ~s ~m ~a <Btn1Down>:grab-focus()\n\
~c ~m ~a <Btn1Motion>:extend-adjust()\n\
~c ~m ~a <Btn1Up>:extend-end()\n\
<Btn2Down>:process-bdrag()\n\
m ~a <Btn2Motion>:secondary-adjust()\n\
~m a <Btn2Motion>:secondary-adjust()\n\
~s <Btn2Up>:copy-to()\n\
~c <Btn2Up>:move-to()\n\
<EnterWindow>:enter()\n\
<LeaveWindow>:leave()\n\
<FocusIn>:focusIn()\n\
<FocusOut>:focusOut()\n\
<Unmap>:unmap()";

/*** ToggleB.c ***/
externaldef(translations) _XmConst char _XmToggleB_defaultTranslations[] = "\
<Btn1Down>:Arm()\n\
<Btn1Up>:Select() Disarm()\n\
<Btn2Down>:ProcessDrag()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfHelp:Help()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";

externaldef(translations) _XmConst char _XmToggleB_menuTranslations[] = "\
<BtnDown>:BtnDown()\n\
<BtnUp>:BtnUp()\n\
<Key>osfSelect:ArmAndActivate()\n\
<Key>osfActivate:ArmAndActivate()\n\
<Key>osfHelp:Help()\n\
<Key>osfCancel:MenuEscape()\n\
~s ~m ~a <Key>Return:ArmAndActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()\n\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()";



#else /* XT_HAS_TRANS_FIX */



/*** ArrowB.c ***/ 
externaldef(translations) _XmConst char _XmArrowB_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

/*** BulletinB.c ***/
externaldef(translations) _XmConst char _XmBulletinB_defaultTranslations[] = "\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()\n\
:<Key>osfHelp:ManagerGadgetHelp()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
:<Key>osfSelect:ManagerGadgetSelect()\n\
<Key>space:ManagerGadgetSelect()\n\
<Key>Return:ManagerParentActivate()\n\
<Key>:ManagerGadgetKeyInput()";

/*** CascadeB.c ***/
externaldef(translations) _XmConst char _XmCascadeB_menubar_events[] = "\
<EnterWindow>Normal:MenuBarEnter()\n\
<LeaveWindow>Normal:MenuBarLeave()\n\
<BtnDown>:MenuBarSelect()\n\
<BtnUp>:DoSelect()\n\
:<Key>osfSelect:KeySelect()\n\
:<Key>osfActivate:KeySelect()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:CleanupMenuBar()\n\
~s<Key>Return:KeySelect()\n\
~s<Key>space:KeySelect()";

externaldef(translations) _XmConst char _XmCascadeB_p_events[] = "\
<EnterWindow>:DelayedArm()\n\
<LeaveWindow>:CheckDisarm()\n\
<BtnDown>:StartDrag()\n\
<BtnUp>:DoSelect()\n\
:<Key>osfSelect:KeySelect()\n\
:<Key>osfActivate:KeySelect()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:CleanupMenuBar()\n\
~s<Key>Return:KeySelect()\n\
~s<Key>space:KeySelect()";

/*** DrawingA.c ***/
externaldef(translations) _XmConst char _XmDrawingA_defaultTranslations[] = "\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:DrawingAreaInput() ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:DrawingAreaInput() ManagerGadgetActivate()\n\
<Btn1Up>:DrawingAreaInput() ManagerGadgetActivate()\n\
<Btn1Down>(2+):DrawingAreaInput() ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):DrawingAreaInput() ManagerGadgetMultiActivate()\n\
<Btn2Down>:DrawingAreaInput() ManagerGadgetDrag()\n\
<BtnDown>:DrawingAreaInput()\n\
<BtnUp>:DrawingAreaInput()\n\
:<Key>osfActivate:DrawingAreaInput() ManagerParentActivate()\n\
:<Key>osfCancel:DrawingAreaInput() ManagerParentCancel()\n\
:<Key>osfHelp:DrawingAreaInput() ManagerGadgetHelp()\n\
:<Key>osfSelect:DrawingAreaInput() ManagerGadgetSelect()\n\
~s ~m ~a <Key>Return:DrawingAreaInput() ManagerParentActivate()\n\
<Key>Return:DrawingAreaInput() ManagerGadgetSelect()\n\
<Key>space:DrawingAreaInput() ManagerGadgetSelect()\n\
<KeyDown>:DrawingAreaInput() ManagerGadgetKeyInput()\n\
<KeyUp>:DrawingAreaInput()";

externaldef(translations) _XmConst char _XmDrawingA_traversalTranslations[] = "\
<EnterWindow>:ManagerEnter()\n\
<LeaveWindow>:ManagerLeave()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()\n\
:<Key>osfUp:DrawingAreaInput() ManagerGadgetTraverseUp()\n\
:<Key>osfDown:DrawingAreaInput() ManagerGadgetTraverseDown()\n\
:<Key>osfLeft:DrawingAreaInput() ManagerGadgetTraverseLeft()\n\
:<Key>osfRight:DrawingAreaInput() ManagerGadgetTraverseRight()\n\
:<Key>osfBeginLine:DrawingAreaInput() ManagerGadgetTraverseHome()\n\
s<Key>Tab:DrawingAreaInput() ManagerGadgetPrevTabGroup()\n\
~s<Key>Tab:DrawingAreaInput() ManagerGadgetNextTabGroup()";

/*** DrawnB.c ***/
externaldef(translations) _XmConst char _XmDrawnB_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

/*** Frame.c ***/
externaldef(translations) _XmConst char _XmFrame_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<FocusIn>:FocusIn()\n\
<Btn1Down>:Arm()\n\
<Btn1Up>:Activate()\n\
<Btn2Down>:ManagerGadgetDrag()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
~s ~m ~a <Key>Return:ManagerParentActivate()";

/*** Label.c ***/
externaldef(translations) _XmConst char _XmLabel_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Btn2Down>:ProcessDrag()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()";

externaldef(translations) _XmConst char _XmLabel_menuTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
:<Key>osfHelp:Help()";

externaldef(translations) _XmConst char _XmLabel_menu_traversal_events[] = "\
<Unmap>:Unmap()\n\
<FocusOut>:FocusOut()\n\
<FocusIn>:FocusIn()\n\
:<Key>osfCancel:MenuEscape()\n\
:<Key>osfLeft:MenuTraverseLeft()\n\
:<Key>osfRight:MenuTraverseRight()\n\
:<Key>osfUp:MenuTraverseUp()\n\
:<Key>osfDown:MenuTraverseDown()";

/*** List.c ***/
#ifdef SUN_MOTIF
externaldef(translations) _XmConst char _XmList_ListXlations1[] = "\
s c <Key>osfBeginLine:ListBeginDataExtend()\n\
:c <Key>osfBeginLine:ListBeginData()\n\
:<Key>osfBeginLine:ListBeginLine()\n\
s c <Key>osfEndLine:ListEndDataExtend()\n\
:c <Key>osfEndLine:ListEndData()\n\
:<Key>osfEndLine:ListEndLine()\n\
:<Key>osfPageLeft:ListLeftPage()\n\
:c <Key>osfPageUp:ListLeftPage()\n\
:<Key>osfPageUp:ListPrevPage()\n\
:<Key>osfPageRight:ListRightPage()\n\
:c <Key>osfPageDown:ListRightPage()\n\
:<Key>osfPageDown:ListNextPage()\n\
:s <KeyDown>osfSelect:ListKbdBeginExtend()\n\
:s <KeyUp>osfSelect:ListKbdEndExtend()\n\
:<KeyDown>osfSelect:ListKbdBeginSelect()\n\
:<KeyUp>osfSelect:ListKbdEndSelect()\n\
:<Key>osfActivate:ListKbdActivate()\n\
:<Key>osfAddMode:ListAddMode()\n\
:<Key>osfHelp:PrimitiveHelp()\n\
:<Key>osfCancel:ListKbdCancel()\n";

externaldef(translations) _XmConst char _XmList_ListXlations2[] = "\
<Unmap>:PrimitiveUnmap()\n\
<Enter>:ListEnter()\n\
<Leave>:ListLeave()\n\
<FocusIn>:ListFocusIn()\n\
<FocusOut>:ListFocusOut()\n\
Button1<Motion>:ListButtonMotion()\n\
s ~m ~a <Btn1Down>:ListBeginExtend()\n\
s ~m ~a <Btn1Up>:ListEndExtend()\n\
c ~s ~m ~a <Btn1Down>:ListBeginToggle()\n\
c ~s ~m ~a <Btn1Up>:ListEndToggle()\n\
~s ~c ~m ~a <Btn1Down>:ListBeginSelect()\n\
~s ~c ~m ~a <Btn1Up>:ListEndSelect()\n\
<Btn2Down>:ListProcessDrag()\n\
:c <Key>osfLeft:ListLeftPage()\n\
:<Key>osfLeft:ListLeftChar()\n\
:c <Key>osfRight:ListRightPage()\n\
:<Key>osfRight:ListRightChar()\n\
s <Key>osfUp:ListExtendPrevItem()\n\
:<Key>osfUp:ListPrevItem()\n\
s <Key>osfDown:ListExtendNextItem()\n\
:<Key>osfDown:ListNextItem()\n\
:c <Key>osfInsert:ListCopyToClipboard()\n\
:<Key>osfCopy:ListCopyToClipboard()\n\
~s c ~m ~a <Key>slash:ListKbdSelectAll()\n\
~s c ~m ~a <Key>backslash:ListKbdDeSelectAll()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
~s ~m ~a <Key>Return:ListKbdActivate()\n\
~s ~m ~a <KeyDown>space:ListKbdBeginSelect()\n\
~s ~m ~a <KeyUp>space:ListKbdEndSelect()\n\
s ~m ~a <KeyDown>space:ListKbdBeginExtend()\n\
s ~m ~a <KeyUp>space:ListKbdEndExtend()";
#else /* SUN_MOTIF */

/*** List.c ***/
externaldef(translations) _XmConst char _XmList_ListXlations1[] = "\
:s c <Key>osfBeginLine:ListBeginDataExtend()\n\
:c <Key>osfBeginLine:ListBeginData()\n\
:<Key>osfBeginLine:ListBeginLine()\n\
:s c <Key>osfEndLine:ListEndDataExtend()\n\
:c <Key>osfEndLine:ListEndData()\n\
:<Key>osfEndLine:ListEndLine()\n\
:<Key>osfPageLeft:ListLeftPage()\n\
:c <Key>osfPageUp:ListLeftPage()\n\
:<Key>osfPageUp:ListPrevPage()\n\
:<Key>osfPageRight:ListRightPage()\n\
:c <Key>osfPageDown:ListRightPage()\n\
:<Key>osfPageDown:ListNextPage()\n\
:s <KeyDown>osfSelect:ListKbdBeginExtend()\n\
:s <KeyUp>osfSelect:ListKbdEndExtend()\n\
:<KeyDown>osfSelect:ListKbdBeginSelect()\n\
:<KeyUp>osfSelect:ListKbdEndSelect()\n\
:<Key>osfActivate:ListKbdActivate()\n\
:<Key>osfAddMode:ListAddMode()\n\
:<Key>osfHelp:PrimitiveHelp()\n\
:<Key>osfCancel:ListKbdCancel()\n";

externaldef(translations) _XmConst char _XmList_ListXlations2[] = "\
<Unmap>:PrimitiveUnmap()\n\
<Enter>:ListEnter()\n\
<Leave>:ListLeave()\n\
<FocusIn>:ListFocusIn()\n\
<FocusOut>:ListFocusOut()\n\
Button1<Motion>:ListButtonMotion()\n\
s ~m ~a <Btn1Down>:ListBeginExtend()\n\
s ~m ~a <Btn1Up>:ListEndExtend()\n\
c ~s ~m ~a <Btn1Down>:ListBeginToggle()\n\
c ~s ~m ~a <Btn1Up>:ListEndToggle()\n\
~s ~c ~m ~a <Btn1Down>:ListBeginSelect()\n\
~s ~c ~m ~a <Btn1Up>:ListEndSelect()\n\
<Btn2Down>:ListProcessDrag()\n\
:c <Key>osfLeft:ListLeftPage()\n\
:<Key>osfLeft:ListLeftChar()\n\
:c <Key>osfRight:ListRightPage()\n\
:<Key>osfRight:ListRightChar()\n\
:s <Key>osfUp:ListExtendPrevItem()\n\
:<Key>osfUp:ListPrevItem()\n\
:s <Key>osfDown:ListExtendNextItem()\n\
:<Key>osfDown:ListNextItem()\n\
:c <Key>osfInsert:ListCopyToClipboard()\n\
:<Key>osfCopy:ListCopyToClipboard()\n\
~s c ~m ~a <Key>slash:ListKbdSelectAll()\n\
~s c ~m ~a <Key>backslash:ListKbdDeSelectAll()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
~s ~m ~a <Key>Return:ListKbdActivate()\n\
~s ~m ~a <KeyDown>space:ListKbdBeginSelect()\n\
~s ~m ~a <KeyUp>space:ListKbdEndSelect()\n\
s ~m ~a <KeyDown>space:ListKbdBeginExtend()\n\
s ~m ~a <KeyUp>space:ListKbdEndExtend()";
#endif /* SUN_MOTIF */

/*** Manager.c ***/
externaldef(translations) _XmConst char _XmManager_managerTraversalTranslations[] = "\
<EnterWindow>:ManagerEnter()\n\
<LeaveWindow>:ManagerLeave()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()\n\
:<Key>osfBeginLine:ManagerGadgetTraverseHome()\n\
:<Key>osfUp:ManagerGadgetTraverseUp()\n\
:<Key>osfDown:ManagerGadgetTraverseDown()\n\
:<Key>osfLeft:ManagerGadgetTraverseLeft()\n\
:<Key>osfRight:ManagerGadgetTraverseRight()\n\
s ~m ~a <Key>Tab:ManagerGadgetPrevTabGroup()\n\
~m ~a <Key>Tab:ManagerGadgetNextTabGroup()";

externaldef(translations) _XmConst char _XmManager_defaultTranslations[] = "\
<BtnMotion>:ManagerGadgetButtonMotion()\n\
<Btn1Down>:ManagerGadgetArm()\n\
<Btn1Down>,<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Up>:ManagerGadgetActivate()\n\
<Btn1Down>(2+):ManagerGadgetMultiArm()\n\
<Btn1Up>(2+):ManagerGadgetMultiActivate()\n\
<Btn2Down>:ManagerGadgetDrag()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
:<Key>osfSelect:ManagerGadgetSelect()\n\
:<Key>osfHelp:ManagerGadgetHelp()\n\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()\n\
<Key>:ManagerGadgetKeyInput()";

/*** MenuShell.c ***/
externaldef(translations) _XmConst char _XmMenuShell_translations [] = "\
<BtnDown>:ClearTraversal()\n\
<BtnUp>:MenuShellPopdownDone()";

/*** Primitive.c ***/
externaldef(translations) _XmConst char _XmPrimitive_defaultTranslations[] = "\
<Unmap>:PrimitiveUnmap()\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfBeginLine:PrimitiveTraverseHome()\n\
:<Key>osfHelp:PrimitiveHelp()\n\
:<Key>osfUp:PrimitiveTraverseUp()\n\
:<Key>osfDown:PrimitiveTraverseDown()\n\
:<Key>osfLeft:PrimitiveTraverseLeft()\n\
:<Key>osfRight:PrimitiveTraverseRight()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()";

/*** PushB.c ***/
externaldef(translations) _XmConst char _XmPushB_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Btn1Down>:Arm()\n\
<Btn1Down>,<Btn1Up>:Activate() Disarm()\n\
<Btn1Down>(2+):MultiArm()\n\
<Btn1Up>(2+):MultiActivate()\n\
<Btn1Up>:Activate() Disarm()\n\
<Btn2Down>:ProcessDrag()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

externaldef(translations) _XmConst char _XmPushB_menuTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<BtnDown>:BtnDown()\n\
<BtnUp>:BtnUp()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfActivate:ArmAndActivate()\n\
:<Key>osfCancel:MenuEscape()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:ArmAndActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

/*** RowColumn.c ***/
externaldef(translations) _XmConst char _XmRowColumn_menu_traversal_table[] = "\
<Unmap>:MenuUnmap()\n\
<EnterWindow>Normal:MenuEnter()\n\
<FocusIn>:MenuFocusIn()\n\
<FocusOut>:MenuFocusOut()\n\
:<Key>osfHelp:MenuHelp()\n\
:<Key>osfLeft:MenuGadgetTraverseLeft()\n\
:<Key>osfRight:MenuGadgetTraverseRight()\n\
:<Key>osfUp:MenuGadgetTraverseUp()\n\
:<Key>osfDown:MenuGadgetTraverseDown()";

externaldef(translations) _XmConst char _XmRowColumn_option_table[]= "\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
:<Key>osfSelect:ManagerGadgetSelect()\n\
:<Key>osfHelp:MenuHelp()\n\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()";

externaldef(translations) _XmConst char _XmRowColumn_bar_table[]= "\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()\n\
:<Key>osfSelect:MenuBarGadgetSelect()\n\
:<Key>osfActivate:MenuBarGadgetSelect()\n\
:<Key>osfHelp:MenuHelp()\n\
:<Key>osfCancel:MenuGadgetEscape()\n\
~s ~m ~a <Key>Return:MenuBarGadgetSelect()\n\
~s ~m ~a <Key>space:MenuBarGadgetSelect()";

externaldef(translations) _XmConst char _XmRowColumn_menu_table[]= "\
<BtnDown>:MenuBtnDown()\n\
<BtnUp>:MenuBtnUp()\n\
:<Key>osfSelect:ManagerGadgetSelect()\n\
:<Key>osfActivate:ManagerGadgetSelect()\n\
:<Key>osfHelp:MenuHelp()\n\
:<Key>osfCancel:MenuGadgetEscape()\n\
~s ~m ~a <Key>Return:ManagerGadgetSelect()\n\
~s ~m ~a <Key>space:ManagerGadgetSelect()";

/*** Sash.c ***/
externaldef(translations) _XmConst char _XmSash_defTranslations[] = "\
<Unmap>:PrimitiveUnmap()\n\
<EnterWindow>:enter()\n\
<LeaveWindow>:leave()\n\
<FocusIn>:SashFocusIn()\n\
<FocusOut>:SashFocusOut()\n\
~c ~s ~m ~a <Btn1Down>:SashAction(Start)\n\
~c ~s ~m ~a <Btn1Motion>:SashAction(Move)\n\
~c ~s ~m ~a <Btn1Up>:SashAction(Commit)\n\
~c ~s ~m ~a <Btn2Down>:SashAction(Start)\n\
~c ~s ~m ~a <Btn2Motion>:SashAction(Move)\n\
~c ~s ~m ~a <Btn2Up>:SashAction(Commit)\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfHelp:Help()\n\
:c <Key>osfUp:SashAction(Key,LargeIncr,Up)\n\
:<Key>osfUp:SashAction(Key,DefaultIncr,Up)\n\
:c <Key>osfDown:SashAction(Key,LargeIncr,Down)\n\
:<Key>osfDown:SashAction(Key,DefaultIncr,Down)\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
s ~m ~a <Key>Tab:PrevTabGroup()\n\
~m ~a <Key>Tab:NextTabGroup()";

/*** ScrollBar.c ***/
externaldef(translations) _XmConst char _XmScrollBar_defaultTranslations[] = "\
<Unmap>:PrimitiveUnmap()\n\
<Enter>:PrimitiveEnter()\n\
<Leave>:PrimitiveLeave()\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
~s ~c ~m ~a <Btn1Down>:Select()\n\
~s ~c ~m ~a <Btn1Up>:Release()\n\
~s ~c ~m ~a Button1<PtrMoved>:Moved()\n\
~s ~c ~m ~a <Btn2Down>:Select()\n\
~s ~c ~m ~a <Btn2Up>:Release()\n\
~s ~c ~m ~a Button2<PtrMoved>:Moved()\n\
~s c ~m ~a <Btn1Down>:TopOrBottom()\n\
~s c ~m ~a <Btn1Up>:Release()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:CancelDrag()\n\
:<Key>osfBeginLine:TopOrBottom()\n\
:<Key>osfEndLine:TopOrBottom()\n\
:<Key>osfPageLeft:PageUpOrLeft(1)\n\
:c <Key>osfPageUp:PageUpOrLeft(1)\n\
:<Key>osfPageUp:PageUpOrLeft(0)\n\
:<Key>osfPageRight:PageDownOrRight(1)\n\
:c <Key>osfPageDown:PageDownOrRight(1)\n\
:<Key>osfPageDown:PageDownOrRight(0)\n\
:<Key>osfHelp:PrimitiveHelp()\n\
:c <Key>osfUp:PageUpOrLeft(0)\n\
:c <Key>osfDown:PageDownOrRight(0)\n\
:c <Key>osfLeft:PageUpOrLeft(1)\n\
:c <Key>osfRight:PageDownOrRight(1)\n\
:<Key>osfUp:IncrementUpOrLeft(0)\n\
:<Key>osfDown:IncrementDownOrRight(0)\n\
:<Key>osfLeft:IncrementUpOrLeft(1)\n\
:<Key>osfRight:IncrementDownOrRight(1)\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()";

/*** ScrolledW.c ***/
externaldef(translations) _XmConst char _XmScrolledW_ScrolledWindowXlations[] = "\
<EnterWindow>:ManagerEnter()\n\
<FocusOut>:ManagerFocusOut()\n\
<FocusIn>:ManagerFocusIn()\n\
<Btn2Down>:ManagerGadgetDrag()\n\
:<Key>osfActivate:ManagerParentActivate()\n\
:<Key>osfCancel:ManagerParentCancel()\n\
:c <Key>osfBeginLine:SWTopLine()\n\
:<Key>osfBeginLine:SWBeginLine()\n\
:c <Key>osfEndLine:SWBottomLine()\n\
:<Key>osfEndLine:SWEndLine()\n\
:<Key>osfPageLeft:SWLeftPage()\n\
:c <Key>osfPageUp:SWLeftPage()\n\
:<Key>osfPageUp:SWUpPage()\n\
:<Key>osfPageRight:SWRightPage()\n\
:c <Key>osfPageDown:SWRightPage()\n\
:<Key>osfPageDown:SWDownPage()\n\
:<Key>osfHelp:ManagerGadgetHelp()\n\
:<Key>osfUp:ManagerGadgetTraverseUp()\n\
:<Key>osfDown:ManagerGadgetTraverseDown()\n\
:<Key>osfLeft:ManagerGadgetTraverseLeft()\n\
:<Key>osfRight:ManagerGadgetTraverseRight()\n\
~s ~m ~a <Key>Return:ManagerParentActivate()\n\
s ~m ~a <Key>Tab:ManagerGadgetPrevTabGroup()\n\
~m ~a <Key>Tab:ManagerGadgetNextTabGroup()";

externaldef(translations) _XmConst char _XmScrolledW_ClipWindowTranslationTable[] = "\
<MapNotify>:SWNoop()\n\
:c <Key>osfBeginLine:SWTopLineGrab()\n\
:<Key>osfBeginLine:SWBeginLineGrab()\n\
:c <Key>osfEndLine:SWBottomLineGrab()\n\
:<Key>osfEndLine:SWEndLineGrab()\n\
:<Key>osfPageLeft:SWLeftPageGrab()\n\
:c <Key>osfPageUp:SWLeftPageGrab()\n\
:<Key>osfPageUp:SWUpPageGrab()\n\
:<Key>osfPageRight:SWRightPageGrab()\n\
:c <Key>osfPageDown:SWRightPageGrab()\n\
:<Key>osfPageDown:SWDownPageGrab()";

externaldef(translations) _XmConst char _XmScrolledW_WorkWindowTranslationTable[] = "\
:c <Key>osfBeginLine:SWTopLineWork()\n\
:<Key>osfBeginLine:SWBeginLineWork()\n\
:c <Key>osfEndLine:SWBottomLineWork()\n\
:<Key>osfEndLine:SWEndLineWork()";


/*** SelectioB.c ***/
externaldef(translations) _XmConst char _XmSelectioB_defaultTextAccelerators[] = "\
\043override\n\
:<Key>osfUp:SelectionBoxUpOrDown(0)\n\
:<Key>osfDown:SelectionBoxUpOrDown(1)\n\
:<Key>osfBeginLine:SelectionBoxUpOrDown(2)\n\
:<Key>osfEndLine:SelectionBoxUpOrDown(3)\n\
s c ~m ~a <Key>space:SelectionBoxRestore()";

/*** TearOffB.c ***/
externaldef(translations) _XmConst char _XmTearOffB_overrideTranslations[] = "\
<Btn2Down>:BDrag()\n\
<BtnUp>:BActivate()\n\
:<Key>osfSelect:KActivate()\n\
:<Key>osfActivate:KActivate()\n\
~s ~m ~a <Key>Return:KActivate()\n\
~s ~m ~a <Key>space:KActivate()";

/*** TextF.c ***/
#ifdef SUN_MOTIF
externaldef(translations) _XmConst char _XmTextF_EventBindings1[] = "\
:m <Key>osfPrimaryPaste:cut-primary()\n\
:a <Key>osfPrimaryPaste:cut-primary()\n\
:<Key>osfPrimaryPaste:copy-primary()\n\
:m <Key>osfCut:cut-primary()\n\
:a <Key>osfCut:cut-primary()\n\
:<Key>osfCut:cut-clipboard()\n\
:<Key>osfPaste:paste-clipboard()\n\
:m <Key>osfCopy:copy-primary()\n\
:a <Key>osfCopy:copy-primary()\n\
:<Key>osfCopy:copy-clipboard()\n\
s <Key>osfBeginLine:beginning-of-line(extend)\n\
:<Key>osfBeginLine:beginning-of-line()\n\
s <Key>osfEndLine:end-of-line(extend)\n\
:<Key>osfEndLine:end-of-line()\n\
s <Key>osfPageLeft:page-left(extend)\n\
:<Key>osfPageLeft:page-left()\n\
s c<Key>osfPageUp:page-left(extend)\n\
:c <Key>osfPageUp:page-left()\n\
s <Key>osfPageRight:page-right(extend)\n\
:<Key>osfPageRight:page-right()\n\
s c <Key>osfPageDown:page-right(extend)\n\
:c <Key>osfPageDown:page-right()\n\
:<Key>osfClear:clear-selection()\n\
:<Key>osfBackSpace:delete-previous-character()\n\
s m <Key>osfDelete:cut-primary()\n\
s a <Key>osfDelete:cut-primary()\n\
s <Key>osfDelete:cut-clipboard()\n\
:c <Key>osfDelete:delete-to-end-of-line()\n\
:<Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextF_EventBindings2[] = "\
:c m <Key>osfInsert:copy-primary()\n\
:c a <Key>osfInsert:copy-primary()\n\
s <Key>osfInsert:paste-clipboard()\n\
:c <Key>osfInsert:copy-clipboard()\n\
:s <Key>osfSelect:key-select()\n\
:<Key>osfSelect:set-anchor()\n\
:<Key>osfActivate:activate()\n\
:<Key>osfAddMode:toggle-add-mode()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:process-cancel()\n\
s c <Key>osfLeft:backward-word(extend)\n\
:c <Key>osfLeft:backward-word()\n\
s <Key>osfLeft:key-select(left)\n\
:<Key>osfLeft:backward-character()\n\
s c <Key>osfRight:forward-word(extend)\n\
:c <Key>osfRight:forward-word()\n\
s <Key>osfRight:key-select(right)\n\
:<Key>osfRight:forward-character()\n\
:<Key>osfUp:traverse-prev()\n\
:<Key>osfDown:traverse-next()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
~m ~a <Key>Tab:next-tab-group()\n\
~s ~m ~a <Key>Return:activate()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";
#else /* SUN_MOTIF */

/*** TextF.c ***/
externaldef(translations) _XmConst char _XmTextF_EventBindings1[] = "\
:m <Key>osfPrimaryPaste:cut-primary()\n\
:a <Key>osfPrimaryPaste:cut-primary()\n\
:<Key>osfPrimaryPaste:copy-primary()\n\
:m <Key>osfCut:cut-primary()\n\
:a <Key>osfCut:cut-primary()\n\
:<Key>osfCut:cut-clipboard()\n\
:<Key>osfPaste:paste-clipboard()\n\
:m <Key>osfCopy:copy-primary()\n\
:a <Key>osfCopy:copy-primary()\n\
:<Key>osfCopy:copy-clipboard()\n\
:s <Key>osfBeginLine:beginning-of-line(extend)\n\
:<Key>osfBeginLine:beginning-of-line()\n\
:s <Key>osfEndLine:end-of-line(extend)\n\
:<Key>osfEndLine:end-of-line()\n\
:s <Key>osfPageLeft:page-left(extend)\n\
:<Key>osfPageLeft:page-left()\n\
:s c<Key>osfPageUp:page-left(extend)\n\
:c <Key>osfPageUp:page-left()\n\
:s <Key>osfPageRight:page-right(extend)\n\
:<Key>osfPageRight:page-right()\n\
:s c <Key>osfPageDown:page-right(extend)\n\
:c <Key>osfPageDown:page-right()\n\
:<Key>osfClear:clear-selection()\n\
:<Key>osfBackSpace:delete-previous-character()\n\
:s m <Key>osfDelete:cut-primary()\n\
:s a <Key>osfDelete:cut-primary()\n\
:s <Key>osfDelete:cut-clipboard()\n\
:c <Key>osfDelete:delete-to-end-of-line()\n\
:<Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextF_EventBindings2[] = "\
:c m <Key>osfInsert:copy-primary()\n\
:c a <Key>osfInsert:copy-primary()\n\
:s <Key>osfInsert:paste-clipboard()\n\
:c <Key>osfInsert:copy-clipboard()\n\
:s <Key>osfSelect:key-select()\n\
:<Key>osfSelect:set-anchor()\n\
:<Key>osfActivate:activate()\n\
:<Key>osfAddMode:toggle-add-mode()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:process-cancel()\n\
:s c <Key>osfLeft:backward-word(extend)\n\
:c <Key>osfLeft:backward-word()\n\
:s <Key>osfLeft:key-select(left)\n\
:<Key>osfLeft:backward-character()\n\
:s c <Key>osfRight:forward-word(extend)\n\
:c <Key>osfRight:forward-word()\n\
:s <Key>osfRight:key-select(right)\n\
:<Key>osfRight:forward-character()\n\
:<Key>osfUp:traverse-prev()\n\
:<Key>osfDown:traverse-next()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
~m ~a <Key>Tab:next-tab-group()\n\
~s ~m ~a <Key>Return:activate()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";
#endif /* SUN_MOTIF */

externaldef(translations) _XmConst char _XmTextF_EventBindings3[] = "\
<Unmap>:unmap()\n\
<Enter>:enter()\n\
<Leave>:leave()\n\
<FocusIn>:focusIn()\n\
<FocusOut>:focusOut()\n\
~c s ~m ~a <Btn1Down>:extend-start()\n\
c ~s ~m ~a <Btn1Down>:move-destination()\n\
~c ~s ~m ~a <Btn1Down>:grab-focus()\n\
~c ~m ~a <Btn1Motion>:extend-adjust()\n\
~c ~m ~a <Btn1Up>:extend-end()\n\
<Btn2Down>:process-bdrag()\n\
m ~a <Btn2Motion>:secondary-adjust()\n\
~m a <Btn2Motion>:secondary-adjust()\n\
~s <Btn2Up>:copy-to()\n\
~c <Btn2Up>:move-to()";

/*** TextIn.c ***/
#ifdef SUN_MOTIF
externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings1[] = "\
:m <Key>osfPrimaryPaste:cut-primary()\n\
:a <Key>osfPrimaryPaste:cut-primary()\n\
:<Key>osfPrimaryPaste:copy-primary()\n\
:m <Key>osfCut:cut-primary()\n\
:a <Key>osfCut:cut-primary()\n\
:<Key>osfCut:cut-clipboard()\n\
:<Key>osfPaste:paste-clipboard()\n\
:m <Key>osfCopy:copy-primary()\n\
:a <Key>osfCopy:copy-primary()\n\
:<Key>osfCopy:copy-clipboard()\n\
s c <Key>osfBeginLine:beginning-of-file(extend)\n\
:c <Key>osfBeginLine:beginning-of-file()\n\
s <Key>osfBeginLine:beginning-of-line(extend)\n\
:<Key>osfBeginLine:beginning-of-line()\n\
s c <Key>osfEndLine:end-of-file(extend)\n\
:c <Key>osfEndLine:end-of-file()\n\
s <Key>osfEndLine:end-of-line(extend)\n\
:<Key>osfEndLine:end-of-line()\n\
s <Key>osfPageLeft:page-left(extend)\n\
:<Key>osfPageLeft:page-left()\n\
s c <Key>osfPageUp:page-left(extend)\n\
:c <Key>osfPageUp:page-left()\n\
s <Key>osfPageUp:previous-page(extend)\n\
:<Key>osfPageUp:previous-page()\n\
s <Key>osfPageRight:page-right(extend)\n\
:<Key>osfPageRight:page-right()\n\
s c <Key>osfPageDown:page-right(extend)\n\
:c <Key>osfPageDown:page-right()\n\
s <Key>osfPageDown:next-page(extend)\n\
:<Key>osfPageDown:next-page()\n\
:<Key>osfClear:clear-selection()\n\
:<Key>osfBackSpace:delete-previous-character()\n\
s m <Key>osfDelete:cut-primary()\n\
s a <Key>osfDelete:cut-primary()\n\
s <Key>osfDelete:cut-clipboard()\n\
:c <Key>osfDelete:delete-to-end-of-line()\n\
:<Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings2[] = "\
:c m <Key>osfInsert:copy-primary()\n\
:c a <Key>osfInsert:copy-primary()\n\
s <Key>osfInsert:paste-clipboard()\n\
:c <Key>osfInsert:copy-clipboard()\n\
:s <Key>osfSelect:key-select()\n\
:<Key>osfSelect:set-anchor()\n\
:<Key>osfActivate:activate()\n\
:<Key>osfAddMode:toggle-add-mode()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:process-cancel()\n\
s c <Key>osfLeft:backward-word(extend)\n\
:c <Key>osfLeft:backward-word()\n\
s <Key>osfLeft:key-select(left)\n\
:<Key>osfLeft:backward-character()\n\
s c <Key>osfRight:forward-word(extend)\n\
:c <Key>osfRight:forward-word()\n\
s <Key>osfRight:key-select(right)\n\
:<Key>osfRight:forward-character()\n\
s c <Key>osfUp:backward-paragraph(extend)\n\
:c <Key>osfUp:backward-paragraph()\n\
s <Key>osfUp:process-shift-up()\n\
:<Key>osfUp:process-up()\n\
s c <Key>osfDown:forward-paragraph(extend)\n\
:c <Key>osfDown:forward-paragraph()\n\
s <Key>osfDown:process-shift-down()\n\
:<Key>osfDown:process-down()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
c ~m ~a <Key>Tab:next-tab-group()\n\
~m ~a <Key>Tab:process-tab()\n\
c ~s ~m ~a <Key>Return:activate()\n\
~c ~s ~m ~a <Key>Return:process-return()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";
#else /* SUN_MOTIF */

/*** TextIn.c ***/
externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings1[] = "\
:m <Key>osfPrimaryPaste:cut-primary()\n\
:a <Key>osfPrimaryPaste:cut-primary()\n\
:<Key>osfPrimaryPaste:copy-primary()\n\
:m <Key>osfCut:cut-primary()\n\
:a <Key>osfCut:cut-primary()\n\
:<Key>osfCut:cut-clipboard()\n\
:<Key>osfPaste:paste-clipboard()\n\
:m <Key>osfCopy:copy-primary()\n\
:a <Key>osfCopy:copy-primary()\n\
:<Key>osfCopy:copy-clipboard()\n\
:s c <Key>osfBeginLine:beginning-of-file(extend)\n\
:c <Key>osfBeginLine:beginning-of-file()\n\
:s <Key>osfBeginLine:beginning-of-line(extend)\n\
:<Key>osfBeginLine:beginning-of-line()\n\
:s c <Key>osfEndLine:end-of-file(extend)\n\
:c <Key>osfEndLine:end-of-file()\n\
:s <Key>osfEndLine:end-of-line(extend)\n\
:<Key>osfEndLine:end-of-line()\n\
:s <Key>osfPageLeft:page-left(extend)\n\
:<Key>osfPageLeft:page-left()\n\
:s c <Key>osfPageUp:page-left(extend)\n\
:c <Key>osfPageUp:page-left()\n\
:s <Key>osfPageUp:previous-page(extend)\n\
:<Key>osfPageUp:previous-page()\n\
:s <Key>osfPageRight:page-right(extend)\n\
:<Key>osfPageRight:page-right()\n\
:s c <Key>osfPageDown:page-right(extend)\n\
:c <Key>osfPageDown:page-right()\n\
:s <Key>osfPageDown:next-page(extend)\n\
:<Key>osfPageDown:next-page()\n\
:<Key>osfClear:clear-selection()\n\
:<Key>osfBackSpace:delete-previous-character()\n\
:s m <Key>osfDelete:cut-primary()\n\
:s a <Key>osfDelete:cut-primary()\n\
:s <Key>osfDelete:cut-clipboard()\n\
:c <Key>osfDelete:delete-to-end-of-line()\n\
:<Key>osfDelete:delete-next-character()\n";

externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings2[] = "\
:c m <Key>osfInsert:copy-primary()\n\
:c a <Key>osfInsert:copy-primary()\n\
:s <Key>osfInsert:paste-clipboard()\n\
:c <Key>osfInsert:copy-clipboard()\n\
:s <Key>osfSelect:key-select()\n\
:<Key>osfSelect:set-anchor()\n\
:<Key>osfActivate:activate()\n\
:<Key>osfAddMode:toggle-add-mode()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:process-cancel()\n\
:s c <Key>osfLeft:backward-word(extend)\n\
:c <Key>osfLeft:backward-word()\n\
:s <Key>osfLeft:key-select(left)\n\
:<Key>osfLeft:backward-character()\n\
:s c <Key>osfRight:forward-word(extend)\n\
:c <Key>osfRight:forward-word()\n\
:s <Key>osfRight:key-select(right)\n\
:<Key>osfRight:forward-character()\n\
:s c <Key>osfUp:backward-paragraph(extend)\n\
:c <Key>osfUp:backward-paragraph()\n\
:s <Key>osfUp:process-shift-up()\n\
:<Key>osfUp:process-up()\n\
:s c <Key>osfDown:forward-paragraph(extend)\n\
:c <Key>osfDown:forward-paragraph()\n\
:s <Key>osfDown:process-shift-down()\n\
:<Key>osfDown:process-down()\n\
c ~m ~a <Key>slash:select-all()\n\
c ~m ~a <Key>backslash:deselect-all()\n\
s ~m ~a <Key>Tab:prev-tab-group()\n\
c ~m ~a <Key>Tab:next-tab-group()\n\
~m ~a <Key>Tab:process-tab()\n\
c ~s ~m ~a <Key>Return:activate()\n\
~c ~s ~m ~a <Key>Return:process-return()\n\
c ~s ~m ~a <Key>space:set-anchor()\n\
c s ~m ~a <Key>space:key-select()\n\
s ~c ~m ~a <Key>space:self-insert()\n\
<Key>:self-insert()\n";
#endif /* SUN_MOTIF*/

externaldef(translations) _XmConst char _XmTextIn_XmTextEventBindings3[] = "\
<Unmap>:unmap()\n\
<EnterWindow>:enter()\n\
<LeaveWindow>:leave()\n\
<FocusIn>:focusIn()\n\
<FocusOut>:focusOut()\n\
~c s ~m ~a <Btn1Down>:extend-start()\n\
c ~s ~m ~a <Btn1Down>:move-destination()\n\
~c ~s ~m ~a <Btn1Down>:grab-focus()\n\
~c ~m ~a <Btn1Motion>:extend-adjust()\n\
~c ~m ~a <Btn1Up>:extend-end()\n\
<Btn2Down>:process-bdrag()\n\
m ~a <Btn2Motion>:secondary-adjust()\n\
~m a <Btn2Motion>:secondary-adjust()\n\
~s <Btn2Up>:copy-to()\n\
~c <Btn2Up>:move-to()";

/*** ToggleB.c ***/
externaldef(translations) _XmConst char _XmToggleB_defaultTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<Btn1Down>:Arm()\n\
<Btn1Up>:Select() Disarm()\n\
<Btn2Down>:ProcessDrag()\n\
:<Key>osfActivate:PrimitiveParentActivate()\n\
:<Key>osfCancel:PrimitiveParentCancel()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfHelp:Help()\n\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

externaldef(translations) _XmConst char _XmToggleB_menuTranslations[] = "\
<EnterWindow>:Enter()\n\
<LeaveWindow>:Leave()\n\
<BtnDown>:BtnDown()\n\
<BtnUp>:BtnUp()\n\
:<Key>osfSelect:ArmAndActivate()\n\
:<Key>osfActivate:ArmAndActivate()\n\
:<Key>osfHelp:Help()\n\
:<Key>osfCancel:MenuEscape()\n\
~s ~m ~a <Key>Return:ArmAndActivate()\n\
~s ~m ~a <Key>space:ArmAndActivate()";

#endif /* XT_HAS_TRANS_FIX */




/*** VirtKeys.c ***/

/* Do not abbreviate for meta, shift, lock, alt, etc */

externaldef(translations) _XmConst char _XmVirtKeys_fallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift<Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfSelect:<Key>Select\n\
osfActivate:<Key>KP_Enter\n\
osfClear:<Key>Clear\n\
osfUndo:<Key>Undo";


/*"Acorn Computers Ltd"
* Acorn RISC iX versions 1.0->1.2 running on Acorn R140, R225, R260
* (all national keyboard variants)*/

externaldef(translations) _XmConst char _XmVirtKeys_acornFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:Alt <Key>Right\n\
osfBeginLine:Alt <Key>Left\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfActivate:<Key>KP_Enter\n\
osfCopy:<Key>Select";


/*"Apollo Computer Inc."*/

externaldef(translations) _XmConst char _XmVirtKeys_apolloFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>apRightBar\n\
osfBeginLine:<Key>apLeftBar\n\
osfPageLeft:<Key>apLeftBox\n\
osfPageRight:<Key>apRightBox\n\
osfPageUp:<Key>apUpBox\n\
osfPageDown:<Key>apDownBox\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>apCharDel\n\
osfInsert:<Key>Select\n\
osfAddMode:Shift<Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfCopy:<Key>apCopy\n\
osfCut:<Key>apCut\n\
osfPaste:<Key>apPaste\n\
osfUndo:<Key>Undo";


/*"Data General Corporation Rev 04"
* AViiON */

externaldef(translations) _XmConst char _XmVirtKeys_dgFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*"DECWINDOWS DigitalEquipmentCorp."*/

externaldef(translations) _XmConst char _XmVirtKeys_decFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:Alt<Key>Right\n\
osfBeginLine:Alt<Key>Left\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>Delete\n\
osfDelete:<Key>DRemove\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift<Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfSelect:<Key>Select\n\
osfActivate:<Key>KP_Enter\n\
osfPrimaryPaste:<Key>F14\n\
osfQuickPaste:<Key>F17";


/*"Double Click Imaging, Inc. KeyX"
* for the version of KeyX running on 386 AT bus compatibles. */

externaldef(translations) _XmConst char _XmVirtKeys_dblclkFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*"Hewlett-Packard Company" */

externaldef(translations) _XmConst char _XmVirtKeys_hpFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>F7\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>hpDeleteChar\n\
osfInsert:<Key>hpInsertChar\n\
osfAddMode:Shift<Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfSelect:<Key>Select\n\
osfClear:<Key>Clear\n\
osfUndo:<Key>Undo\n\
osfPrimaryPaste:<Key>hpInsertLine\n\
osfQuickPaste:<Key>hpDeleteLine";


/*"International Business Machines"
* for AIX/PS2 and RS/6000 systems */

externaldef(translations) _XmConst char _XmVirtKeys_ibmFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*  Intergraph keyboard support        */
/* Intergraph */

externaldef(translations) _XmConst char _XmVirtKeys_ingrFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:Alt<Key>Right\n\
osfBeginLine:Alt<Key>Left\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift<Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*"Megatek Corporation"
* Megatek X-Cellerator */

externaldef(translations) _XmConst char _XmVirtKeys_megatekFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>R13\n\
osfBeginLine:<Key>F27\n\
osfPageUp:<Key>F29\n\
osfPageDown:<Key>F35\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfCopy:<Key>F16\n\
osfCut:<Key>F20\n\
osfPaste:<Key>F18\n\
osfUndo:<Key>F14";


/*"Motorola Inc. (Microcomputer Division)" */
/* (c) Copyright 1990 Motorola Inc. */
/* Motorola provides these key bindings as is,
	with no guarantees or warranties implied.
	Motorola is under no obligation to support,
	update, or extend these key bindings for
	future releases. */

externaldef(translations) _XmConst char _XmVirtKeys_motorolaFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*"Silicon Graphics Inc." */

externaldef(translations) _XmConst char _XmVirtKeys_sgiFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfActivate:<Key>KP_Enter\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";


/*"Siemens Munich by SP-4's Hacker Crew"
* Siemens WX200 system */

externaldef(translations) _XmConst char _XmVirtKeys_siemensWx200FallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>F29\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:<Key>Menu\n\
osfMenuBar:<Key>F10";


/*"Siemens Munich (SP-4's hacker-clan)"
* Siemens 9733 system */

externaldef(translations) _XmConst char _XmVirtKeys_siemens9733FallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete_char\n\
osfInsert:<Key>Insert_char\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>Help\n\
osfMenu:<Key>Linefeed\n\
osfMenuBar:<Key>F10";


/*"Sun Microsystems, Inc."
*"X11/NeWS - Sun Microsystems Inc."
 */

externaldef(translations) _XmConst char _XmVirtKeys_sunFallbackBindingString[] = "\
osfActivate:<Key>KP_Enter\n\
osfCancel:<Key>Escape\n\
osfHelp:<Key>Help\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfBeginLine:<Key>Home\n\
osfEndLine:<Key>End\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfUndo:<Key>Undo\n\
osfAddMode:Shift<Key>F8\n\
osfCopy:<Key>SunCopy\n\
osfCut:<Key>SunCut\n\
osfPaste:<Key>SunPaste";

/*"Tektronix, Inc." */

externaldef(translations) _XmConst char _XmVirtKeys_tekFallbackBindingString[] = "\
osfCancel:<Key>Escape\n\
osfLeft:<Key>Left\n\
osfUp:<Key>Up\n\
osfRight:<Key>Right\n\
osfDown:<Key>Down\n\
osfEndLine:<Key>End\n\
osfBeginLine:<Key>Home\n\
osfPageUp:<Key>Prior\n\
osfPageDown:<Key>Next\n\
osfBackSpace:<Key>BackSpace\n\
osfDelete:<Key>Delete\n\
osfInsert:<Key>Insert\n\
osfAddMode:Shift <Key>F8\n\
osfHelp:<Key>F1\n\
osfMenu:Shift<Key>F10\n\
osfMenuBar:<Key>F10";

