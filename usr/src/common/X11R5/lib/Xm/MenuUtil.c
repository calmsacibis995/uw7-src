#pragma ident	"@(#)m1.2libs:Xm/MenuUtil.c	1.5"
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
#include <stdio.h>
#include <ctype.h>
#include "XmI.h"
#include "MessagesI.h"
#include <Xm/XmP.h>
#include <X11/ShellP.h>
#include <Xm/CascadeBP.h>
#include <Xm/CascadeBGP.h>
#include <Xm/MenuUtilP.h>
#include <Xm/MenuShellP.h>
#include <Xm/RowColumnP.h>
#include <Xm/XmosP.h>
#include <Xm/ScreenP.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define GRABPTRERROR    catgets(Xm_catd,MS_CButton,MSG_CB_5,_XmMsgCascadeB_0003)
#define GRABKBDERROR    catgets(Xm_catd,MS_CButton,MSG_CB_6,_XmMsgRowColText_0024)
#else
#define GRABPTRERROR    _XmMsgCascadeB_0003
#define GRABKBDERROR    _XmMsgRowColText_0024
#endif


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void MenuTraverse() ;
static void GadgetCleanup() ;
static Boolean WrapRight() ;
static Boolean WrapLeft() ;
static void LocateChild() ;
static void MoveDownInMenuBar() ;
static void MoveLeftInMenuBar() ;
static void MoveRightInMenuBar() ;
static void FindNextMenuBarItem() ;
static void FindPrevMenuBarItem() ;
static Boolean ValidateMenuBarItem() ;
static Boolean FindNextMenuBarCascade() ;
static Boolean FindPrevMenuBarCascade() ;
static Boolean ValidateMenuBarCascade() ;
static void MenuUtilScreenDestroyCallback();

#else

static void MenuTraverse( 
                        Widget w,
                        XEvent *event,
                        XmTraversalDirection direction) ;
static void GadgetCleanup( 
                        XmRowColumnWidget rc,
                        XmGadget oldActiveChild) ;
static Boolean WrapRight( 
                        XmRowColumnWidget rc) ;
static Boolean WrapLeft( 
                        XmRowColumnWidget rc) ;
static void LocateChild( 
                        XmRowColumnWidget rc,
                        Widget wid,
                        XmTraversalDirection direction) ;
static void MoveDownInMenuBar( 
                        XmRowColumnWidget rc,
                        Widget pw) ;
static void MoveLeftInMenuBar( 
                        XmRowColumnWidget rc,
                        Widget pw) ;
static void MoveRightInMenuBar( 
                        XmRowColumnWidget rc,
                        Widget pw) ;
static void FindNextMenuBarItem( 
                        XmRowColumnWidget menubar) ;
static void FindPrevMenuBarItem( 
                        XmRowColumnWidget menubar) ;
static Boolean ValidateMenuBarItem( 
                        Widget oldActiveChild,
                        Widget newActiveChild) ;
static Boolean FindNextMenuBarCascade( 
                        XmRowColumnWidget menubar) ;
static Boolean FindPrevMenuBarCascade( 
                        XmRowColumnWidget menubar) ;
static Boolean ValidateMenuBarCascade( 
                        Widget oldActiveChild,
                        Widget newMenuChild) ;
static void MenuUtilScreenDestroyCallback ( 
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data );

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/* temp hold for core class translations used during subclass'
 * InitializePrehook & InitializePosthook
 */
static struct _XmTranslRec * coreClassTranslations;

Boolean
#ifdef _NO_PROTO
_XmIsActiveTearOff (widget)
       Widget widget;
#else
_XmIsActiveTearOff (
       Widget widget)
#endif /* _NO_PROTO */
{
    XmRowColumnWidget menu = (XmRowColumnWidget) widget;

    if (RC_TearOffActive(menu))
        return (True);
    else
        return (False);
}

Widget
#ifdef _NO_PROTO
_XmGetRC_PopupPosted (wid)
       Widget wid;
#else
_XmGetRC_PopupPosted (
       Widget wid)
#endif /* _NO_PROTO */
{
   if (XmIsRowColumn(wid))
      return (RC_PopupPosted(wid));
   else
      return NULL;
}

/*
 * The following two functions are used by menu and menu-item widgets to keep
 * track of whether we're in drag (button down) or traversal mode.
 */
Boolean
#ifdef _NO_PROTO
_XmGetInDragMode (widget)
        Widget widget;
#else
_XmGetInDragMode (
        Widget widget)
#endif
{
  return((_XmGetMenuState(widget))->MU_InDragMode);
}

void
#ifdef _NO_PROTO
_XmSetInDragMode( widget, mode )
        Widget widget;
        Boolean mode;
#else
_XmSetInDragMode(
        Widget widget,
#if NeedWidePrototypes
        int mode )
#else
        Boolean mode )
#endif
#endif
{
  (_XmGetMenuState(widget))->MU_InDragMode = mode;
}

/*
 * The following functions are used to separate the private class function
 * in RowColumn from the buttons that may be children of the RowColumn.
 * This is simply an interface supplied so that the buttons will not have
 * to have intimate knowledge of the RowColumn class functions.
 */


static XtPointer menuProcEntry = NULL;

/*
 * this routine is called at RowColumn class init to
 * save the address of the menuProcedureEntry routine.
 */
void 
#ifdef _NO_PROTO
_XmSaveMenuProcContext( address )
        XtPointer address ;
#else
_XmSaveMenuProcContext(
        XtPointer address )
#endif /* _NO_PROTO */
{
   menuProcEntry = address;
}


/*
 * This routine is used by the button children of the RowColumn (currently
 * all label and labelgadget subclasses) to get the address of the
 * menuProcedureEntry routine.  It is called by the buttons class init
 * routines.
 */
XtPointer 
#ifdef _NO_PROTO
_XmGetMenuProcContext()
#else
_XmGetMenuProcContext( void )
#endif /* _NO_PROTO */
{
   return menuProcEntry;
}

/*
 * Call XtGrabPointer with retry
 */
int 
#ifdef _NO_PROTO
_XmGrabPointer(widget, owner_events, event_mask, pointer_mode, 
   keyboard_mode,  confine_to, cursor, time)
	Widget widget;
	Bool owner_events; 
	unsigned int event_mask; 
	int pointer_mode; 
	int keyboard_mode; 
	Window confine_to; 
	Cursor cursor; 
	Time time;
#else
_XmGrabPointer(
	Widget widget,
	Bool owner_events, 
	unsigned int event_mask, 
	int pointer_mode, 
	int keyboard_mode, 
	Window confine_to, 
	Cursor cursor, 
	Time time )
#endif
{
   register int status, retry;

   for (retry=0; retry < 5; retry++)
   {
      if ((status = XtGrabPointer(widget, owner_events, event_mask, 
         			  pointer_mode, keyboard_mode, confine_to, 
				  cursor, time)) == GrabSuccess)
	 break;

      _XmSleep(1);
   }
   if (status != GrabSuccess)
      _XmWarning((Widget) widget, GRABPTRERROR);

   return(status);
}

/*
 * Call XtGrabKeyboard with retry
 */
int 
#ifdef _NO_PROTO
_XmGrabKeyboard(widget, owner_events, pointer_mode, keyboard_mode, time) 
	Widget widget;
	Bool owner_events; 
	int pointer_mode;
	int keyboard_mode; 
	Time time;
#else
_XmGrabKeyboard(
	Widget widget,
	Bool owner_events, 
	int pointer_mode,
	int keyboard_mode, 
	Time time )
#endif
{
   register int status, retry;

   for (retry=0; retry < 5; retry++)
   {
      if ((status = XtGrabKeyboard(widget, owner_events, 
         pointer_mode, keyboard_mode, time)) == GrabSuccess)
	 break;
      _XmSleep(1);
   }
   if (status != GrabSuccess)
      _XmWarning(widget, GRABKBDERROR);

   return(status);
}


void 
#ifdef _NO_PROTO
_XmMenuSetInPMMode ( wid, flag )
	Widget wid;
	Boolean flag;
#else
_XmMenuSetInPMMode (
	Widget wid,
#if NeedWidePrototypes
	int flag )
#else
	Boolean flag )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   _XmGetMenuState((Widget)wid)->MU_InPMMode = flag;
}

/*
 * This menuprocs procedure allows an external object to turn on and off menu
 * traversal.
 */
void
#ifdef _NO_PROTO
_XmSetMenuTraversal( wid, traversalOn )
        Widget wid ;
        Boolean traversalOn ;
#else
_XmSetMenuTraversal(
        Widget wid,
#if NeedWidePrototypes
        int traversalOn )
#else
        Boolean traversalOn )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   if (traversalOn)
   {
      _XmSetInDragMode(wid, False);
      if (!XmProcessTraversal(wid , XmTRAVERSE_CURRENT))
         XtSetKeyboardFocus(XtParent(wid), wid);
   }
   else
   {
     _XmSetInDragMode(wid, True);
     if(    XmIsMenuShell( XtParent( wid))    )
       {
	 /* Must be careful not to trash the traversal environment
	  * for RowColumns which are not using menu-specific traversal.
	  */
	 _XmLeafPaneFocusOut(wid);
       }
   }
}


void
#ifdef _NO_PROTO
_XmLeafPaneFocusOut( wid )
	Widget wid;
#else
_XmLeafPaneFocusOut( 
	Widget wid )
#endif
{
   XEvent fo_event;
   Widget widget;
   XmRowColumnWidget rc = (XmRowColumnWidget)wid;

   /* find the leaf pane */
   for( ; RC_PopupPosted(rc);
      rc = (XmRowColumnWidget) 
	 ((XmMenuShellWidget)RC_PopupPosted(rc))->composite.children[0]);

   fo_event.type = FocusOut;
   fo_event.xfocus.send_event = True;
   if ((widget = rc->manager.active_child) && XmIsCascadeButtonGadget(widget))
   {
      /* clear the internal focus path; also active_child = NULL which happens
       * to make cascadebutton focus out work correctly.
       */
      _XmClearFocusPath((Widget)rc);
      _XmDispatchGadgetInput(widget, NULL, XmFOCUS_OUT_EVENT);
      ((XmGadget)widget)->gadget.have_traversal = False;
   }
   else
   {
      if (widget && XmIsPrimitive(widget) &&
          (((XmPrimitiveWidgetClass)(widget->core.widget_class))->
            primitive_class.border_highlight != (XtWidgetProc)NULL))
         (*(((XmPrimitiveWidgetClass)(widget->core.widget_class))->
            primitive_class.border_unhighlight))((Widget) widget);
      else
	 _XmManagerFocusOut( (Widget) rc, &fo_event, NULL, NULL);

      /* clears the focus_item so that next TraverseToChild() will work */
      _XmClearFocusPath((Widget)rc);
   }
}

void
#ifdef _NO_PROTO
_XmMenuHelp( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmMenuHelp(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XmGadget gadget;

   if (!_XmIsEventUnique(event) ||
       (!RC_IsArmed(rc) && !(((RC_Type(rc) == XmMENU_OPTION) || (RC_Type(rc) == XmMENU_PULLDOWN)))))
     return;

   if (!_XmGetInDragMode ((Widget)rc))
   {
     if ((gadget = (XmGadget) rc->manager.active_child) != NULL)
	_XmDispatchGadgetInput( (Widget) gadget, event, XmHELP_EVENT);
     else
     {
	_XmSocorro( (Widget) rc, event, NULL, NULL);
	_XmMenuPopDown((Widget)rc, event, NULL);
     }
   }
   else
   {
     if ((gadget = (XmGadget) 
	  _XmInputInGadget((Widget) rc, event->xkey.x, event->xkey.y)) != NULL)
        _XmDispatchGadgetInput( (Widget) gadget, event, XmHELP_EVENT);
     else
     {
	_XmSocorro( (Widget) rc, event, NULL, NULL);
	_XmMenuPopDown((Widget)rc, event, NULL);
     }
   }
   _XmRecordEvent(event);
}

static void 
#ifdef _NO_PROTO
MenuTraverse( w, event, direction )
        Widget w ;
        XEvent *event ;
        XmTraversalDirection direction ;
#else
MenuTraverse(
        Widget w,
        XEvent *event,
        XmTraversalDirection direction )
#endif /* _NO_PROTO */
{
   Widget parent;

   /*
    * The case may occur where the reporting widget is in fact the
    * RowColumn widget, and not a child.  This will occur if the
    * RowColumn has not traversable children.
    */
   if (XmIsRowColumn(w))
      parent = w;
   else if (XmIsRowColumn(XtParent(w)))
      parent = XtParent(w);
   else
      return;

   if ((RC_Type(parent) == XmMENU_POPUP) || 
       (RC_Type(parent) == XmMENU_PULLDOWN) ||
       (RC_Type(parent) == XmMENU_BAR))
   {
      _XmRecordEvent(event);
      (*(((XmRowColumnWidgetClass)XtClass(parent))->row_column_class.
          traversalHandler))( (Widget) parent, (Widget) w, direction);
   }
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuTraverseLeft( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuTraverseLeft(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
    if (_XmIsEventUnique(event))
   {
	 MenuTraverse(wid, event, XmTRAVERSE_LEFT);
   }
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuTraverseRight( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuTraverseRight(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
   if (_XmIsEventUnique(event))
   {
	 MenuTraverse(wid, event, XmTRAVERSE_RIGHT);
   }
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuTraverseUp( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuTraverseUp(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
   if (_XmIsEventUnique(event))
   {
	 MenuTraverse(wid, event, XmTRAVERSE_UP);
   }
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuTraverseDown( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmMenuTraverseDown(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
   if (_XmIsEventUnique(event))
   {
	 MenuTraverse(wid, event, XmTRAVERSE_DOWN);
   }
}

/* ARGSUSED */
void 
#ifdef _NO_PROTO
_XmMenuEscape( w, event, params, num_params )
        Widget w ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmMenuEscape(
        Widget w,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   Widget parent = XtParent(w);

   /* Process the event only if not already processed */
   if (!_XmIsEventUnique(event))
      return;

   /*
    * Catch case where its a menubar w/ no submenus up - can't call
    *   menushell's popdown, call rowcolumn's instead.
    */
   if ((XmIsCascadeButton(w) || XmIsCascadeButtonGadget(w)) &&
	XmIsRowColumn(parent) && (RC_Type(parent) == XmMENU_BAR) &&
	!RC_PopupPosted(parent))
   {
      (*(((XmRowColumnClassRec *)XtClass(parent))->row_column_class.
	 menuProcedures)) (XmMENU_POPDOWN, parent, NULL, event, NULL);
   }
   else
       /* Let the menushell widget clean things up */
       (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
	  menu_shell_class.popdownOne))(w, event, NULL, NULL);
}

void 
#ifdef _NO_PROTO
_XmRC_GadgetTraverseDown( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmRC_GadgetTraverseDown(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   XmGadget gadget = (XmGadget)rc->manager.active_child;

   if (gadget && XmIsGadget(gadget))
      _XmMenuTraverseDown((Widget) gadget, event, param, num_param);
}

void 
#ifdef _NO_PROTO
_XmRC_GadgetTraverseUp( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmRC_GadgetTraverseUp(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   XmGadget gadget = (XmGadget)rc->manager.active_child;

   if (gadget && XmIsGadget(gadget))
      _XmMenuTraverseUp((Widget) gadget, event, param, num_param);
}

void 
#ifdef _NO_PROTO
_XmRC_GadgetTraverseLeft( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmRC_GadgetTraverseLeft(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   XmGadget gadget = (XmGadget)rc->manager.active_child;

   /*
    * If there is not active child, then this RowColumn has
    * not traversable children, so it's fielding traversal
    * requests itself.
    */
   if (gadget && XmIsGadget(gadget))
      _XmMenuTraverseLeft((Widget) gadget, event, param, num_param);
   else if (gadget == NULL)
      _XmMenuTraverseLeft((Widget) rc, event, param, num_param);
}

void 
#ifdef _NO_PROTO
_XmRC_GadgetTraverseRight( wid, event, param, num_param )
        Widget wid ;
        XEvent *event ;
        String *param ;
        Cardinal *num_param ;
#else
_XmRC_GadgetTraverseRight(
        Widget wid,
        XEvent *event,
        String *param,
        Cardinal *num_param )
#endif /* _NO_PROTO */
{
        XmRowColumnWidget rc = (XmRowColumnWidget) wid ;
   XmGadget gadget = (XmGadget)rc->manager.active_child;

   /*
    * If there is not active child, then this RowColumn has
    * not traversable children, so it's fielding traversal
    * requests itself.
    */
   if (gadget && XmIsGadget(gadget))
      _XmMenuTraverseRight((Widget) gadget, event, param, num_param);
   else if (gadget == NULL)
      _XmMenuTraverseRight((Widget) rc, event, param, num_param);
}


/*
 * In case we've moved into our out of a gadget, we need to take care
 * of the highlighting ourselves, since the gadget will not get a focus
 * event.
 */
static void 
#ifdef _NO_PROTO
GadgetCleanup( rc, oldActiveChild )
        XmRowColumnWidget rc ;
        XmGadget oldActiveChild ;
#else
GadgetCleanup(
        XmRowColumnWidget rc,
        XmGadget oldActiveChild )
#endif /* _NO_PROTO */
{
    XmGadget newActiveChild = (XmGadget)rc->manager.active_child;

    if (oldActiveChild != newActiveChild)
    {
        if (oldActiveChild && XmIsGadget(oldActiveChild))
        {
            _XmDispatchGadgetInput( (Widget) oldActiveChild, NULL,
                                                            XmFOCUS_OUT_EVENT);
            oldActiveChild->gadget.have_traversal = False;
        }
    }
}


/*
 * At the edge of the menu, decide what to do in this case
 */
static Boolean
#ifdef _NO_PROTO
WrapRight ( rc)
	XmRowColumnWidget rc ;
#else
WrapRight (
        XmRowColumnWidget rc )
#endif /* _NO_PROTO */
{
   Widget topLevel;
   Widget oldActiveChild = rc->manager.active_child;
   Boolean done = False;

   _XmGetActiveTopLevelMenu ((Widget) rc, (Widget *) &topLevel);

   /* if in a menubar system, try to move to next menubar item cascade */
   if (XmIsMenuShell(XtParent(rc)) && (RC_Type(topLevel) == XmMENU_BAR) &&
       (FindNextMenuBarCascade((XmRowColumnWidget) topLevel)))
   {
      GadgetCleanup(rc, (XmGadget) oldActiveChild);
      done = True;
   }

   return (done);
}

/*
 * At the edge of the menu, decide what to do in this case
 */
static Boolean
#ifdef _NO_PROTO
WrapLeft ( rc)
	XmRowColumnWidget rc ;
#else
WrapLeft (
        XmRowColumnWidget rc )
#endif /* _NO_PROTO */
{
   Widget oldActiveChild = rc->manager.active_child;
   Boolean done = False;

   /* 
    * If we're the topmost pulldown menupane from a menubar, then unpost 
    * and move to the next available item in the menubar, and post its 
    * submenu.
    */
   if (XmIsMenuShell(XtParent(rc)) &&
       (RC_Type (rc) != XmMENU_POPUP) && RC_CascadeBtn(rc) && 
       (RC_Type (XtParent(RC_CascadeBtn(rc))) == XmMENU_BAR) &&
       (FindPrevMenuBarCascade((XmRowColumnWidget) 
                                      XtParent(RC_CascadeBtn(rc)))))
   {
      GadgetCleanup(rc, (XmGadget) oldActiveChild);
      done = True;
   }

   /*
    * if we are in a pulldown from another posted menupane, unpost this one
    */
   else if ((RC_Type(rc) == XmMENU_PULLDOWN) && 
            (RC_Type(XtParent(RC_CascadeBtn(rc))) != XmMENU_OPTION) &&
            XmIsMenuShell(XtParent(rc)))
   {
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
                  menu_shell_class.popdownOne)) (XtParent(rc), NULL, NULL, 
                                                 NULL);
      done = True;
   }

   return (done);
}

/*
 * Search for the next menu item according to the direction
 */
static void
#ifdef _NO_PROTO
LocateChild ( rc, wid, direction)
	XmRowColumnWidget rc ;
        Widget wid;
        XmTraversalDirection direction;
#else
LocateChild (
        XmRowColumnWidget rc,
        Widget wid,
        XmTraversalDirection direction )
#endif /* _NO_PROTO */
{
   Boolean done = False;
   Widget nextWidget;

   /* special case a popped up submenu with no traversable items */
   if (XmIsRowColumn(wid) && 
       ((XmManagerWidget) wid)->manager.active_child == 0)
   {
     if (direction == XmTRAVERSE_LEFT)
       WrapLeft (rc);
     else if (direction == XmTRAVERSE_RIGHT)
       WrapRight (rc);
   }
   else
   {
     nextWidget = _XmNavigate(wid, direction);

     if (direction == XmTRAVERSE_LEFT)
     {
       /* watch for left wrap */
       if ((wid->core.x <= nextWidget->core.x) ||
	   (nextWidget->core.y + nextWidget->core.height <= wid->core.y) ||
	   (nextWidget->core.y >= wid->core.y + wid->core.height))
	 done = WrapLeft(rc);
     }
     else if (direction == XmTRAVERSE_RIGHT)
     {
       /* watch for right wrap */
       if ((wid->core.x >= nextWidget->core.x) ||
	   (wid->core.y + wid->core.height <= nextWidget->core.y) ||
	   (wid->core.y >= nextWidget->core.y + nextWidget->core.height))
	 done = WrapRight(rc);
     }
     
     if (!done)
       _XmProcessTraversal (nextWidget, XmTRAVERSE_CURRENT, True);
   }

}

 void 
#ifdef _NO_PROTO
_XmMenuTraversalHandler( w, pw, direction )
        Widget w ;
        Widget pw ;
        XmTraversalDirection direction ;
#else
_XmMenuTraversalHandler(
        Widget w,
        Widget pw,
        XmTraversalDirection direction )
#endif /* _NO_PROTO */
{
   XmRowColumnWidget rc = (XmRowColumnWidget) w;

   if (_XmGetInDragMode((Widget) rc))
      return;

   if (RC_Type(rc) != XmMENU_BAR)
   {
      /* check for cascading into a submenu */
      if (direction == XmTRAVERSE_RIGHT && 
          XmIsCascadeButtonGadget(pw) && CBG_Submenu(pw)) 
      {
         (*(((XmGadgetClassRec *)XtClass(pw))->gadget_class.
            arm_and_activate))( (Widget) pw, NULL, NULL, NULL);
      }
      else if (direction == XmTRAVERSE_RIGHT && 
               XmIsCascadeButton(pw) && CB_Submenu(pw))
      {
         (*(((XmPrimitiveClassRec *)XtClass(pw))->primitive_class.
             arm_and_activate)) ((Widget) pw, NULL, NULL, NULL);
      }
      
      else
         LocateChild (rc, pw, direction);
   }

   else
   {
       switch (direction)
       {
          case XmTRAVERSE_DOWN:
          {
             MoveDownInMenuBar (rc, pw);
             break;
          }

          case XmTRAVERSE_LEFT:
          {
             MoveLeftInMenuBar(rc, pw);
             break;
          }

          case XmTRAVERSE_RIGHT:
          {
             MoveRightInMenuBar(rc, pw);
             break;
          }
	  
          case XmTRAVERSE_UP:
	  default:
	     break;
       }
    }
}


/*
 * When the PM menubar mode is active, down arrow will
 * cause us to post the menupane associated with the active cascade button
 * in the menubar.
 */
static void 
#ifdef _NO_PROTO
MoveDownInMenuBar( rc, pw )
        XmRowColumnWidget rc ;
        Widget pw ;
#else
MoveDownInMenuBar(
        XmRowColumnWidget rc,
        Widget pw )
#endif /* _NO_PROTO */
{
    if (rc->manager.active_child == NULL)
        return;

    if (XmIsPrimitive(pw))
    {
        XmPrimitiveClassRec * prim;

	CB_SetTraverse (pw, TRUE);
        prim = (XmPrimitiveClassRec *)XtClass(pw);
        (*(prim->primitive_class.arm_and_activate)) ((Widget) pw, NULL,
						     NULL, NULL);
	CB_SetTraverse (pw, FALSE);
    }

    else if (XmIsGadget(pw))
    {
        XmGadgetClassRec * gad;
      
	CBG_SetTraverse (pw, TRUE);
        gad = (XmGadgetClassRec *)XtClass(pw);
        (*(gad->gadget_class.arm_and_activate)) ((Widget) pw, NULL,
						 NULL, NULL);
	CBG_SetTraverse (pw, FALSE);
    }
}

/* ARGSUSED */
static void 
#ifdef _NO_PROTO
MoveLeftInMenuBar( rc, pw )
        XmRowColumnWidget rc ;
        Widget pw ;
#else
MoveLeftInMenuBar(
        XmRowColumnWidget rc,
        Widget pw )
#endif /* _NO_PROTO */
{
   XmMenuState mst = _XmGetMenuState((Widget)rc);

   if ((mst->MU_CurrentMenuChild != NULL) &&
       (RC_PopupPosted(rc) != NULL) &&
       ((XmIsCascadeButtonGadget(pw) && !CBG_Submenu(pw)) ||
       (XmIsCascadeButton(pw) && !CB_Submenu(pw))))
   {
      /* Move to the previous item in the menubar */
      FindPrevMenuBarItem(rc);
   }
   else 
   {
      /* Move to the previous item in the menubar */
      mst->MU_CurrentMenuChild = NULL;
      FindPrevMenuBarItem(rc);
   }
}

static void 
#ifdef _NO_PROTO
MoveRightInMenuBar( rc, pw )
        XmRowColumnWidget rc ;
        Widget pw ;
#else
MoveRightInMenuBar(
        XmRowColumnWidget rc,
        Widget pw )
#endif /* _NO_PROTO */
{
   XmMenuState mst = _XmGetMenuState((Widget)rc);
   
   if ((rc->manager.active_child == NULL) &&
        ((XmIsCascadeButtonGadget(pw) && !CBG_Submenu(pw)) ||
        (XmIsCascadeButton(pw) && !CB_Submenu(pw))))
   {
      FindNextMenuBarCascade(rc);
   }
   else
   {
      /* Move to the next item in the menubar */
      mst->MU_CurrentMenuChild = NULL;
      FindNextMenuBarItem(rc);
   }
}


/*
 * Find the next cascade button in the menubar which can be traversed to.
 */
static void 
#ifdef _NO_PROTO
FindNextMenuBarItem( menubar )
        XmRowColumnWidget menubar ;
#else
FindNextMenuBarItem(
        XmRowColumnWidget menubar )
#endif /* _NO_PROTO */
{
   register int i, j;
   int upper_limit;
   Widget active_child;

   /*
    * We're not in the PM menubar mode if we don't have an active child.
    */
   if (menubar->manager.active_child == NULL)
       return;

   upper_limit = menubar->composite.num_children;
   active_child = menubar->manager.active_child;

   /* Find the index of the currently active item */
   for (i = 0; i < upper_limit; i++)
   {
      if (menubar->composite.children[i] == active_child)
	 break;
   }

   /* Start looking at the next child */
   for (j = 0, i++; j < upper_limit - 1; j++, i++)
   {
       /* Wrap, if necessary */
       if (i >= upper_limit)
	  i = 0;

	if (ValidateMenuBarItem(active_child, menubar->composite.children[i]))
	  return;
   }
}


/*
 * Find the previous cascade button in the menubar which can be traversed to.
 */
static void 
#ifdef _NO_PROTO
FindPrevMenuBarItem( menubar )
        XmRowColumnWidget menubar ;
#else
FindPrevMenuBarItem(
        XmRowColumnWidget menubar )
#endif /* _NO_PROTO */
{
   register int i, j;
   int upper_limit;
   Widget active_child;

   /* We're not in the PM menubar mode if we don't have an active child */
   if (menubar->manager.active_child == NULL)
       return;

   upper_limit = menubar->composite.num_children;
   active_child = menubar->manager.active_child;

   /* Find the index of the currently active item */
   for (i = 0; i < upper_limit; i++)
   {
       if (menubar->composite.children[i] == active_child)
	   break;
   }

   /* Start looking at the previous child */
   for (j = 0, --i; j < upper_limit - 1; j++, --i)
   {
       /* Wrap, if necessary */
       if (i < 0)
	  i = upper_limit - 1;

       if (ValidateMenuBarItem(active_child, menubar->composite.children[i]))
	  return;
   }
}

static Boolean 
#ifdef _NO_PROTO
ValidateMenuBarItem (oldActiveChild, newActiveChild)
	Widget oldActiveChild ;
        Widget newActiveChild ;
#else
ValidateMenuBarItem (
	Widget oldActiveChild,
        Widget newActiveChild)
#endif
{
   XmMenuState mst = _XmGetMenuState((Widget)oldActiveChild);

   if (XmIsTraversable(newActiveChild))
   {
      (void) XmProcessTraversal (newActiveChild, XmTRAVERSE_CURRENT);

      if (XmIsPrimitive(newActiveChild))
      {
         XmPrimitiveClassRec * prim;

         prim = (XmPrimitiveClassRec *)XtClass(newActiveChild);

         if (!mst->MU_InPMMode && CB_Submenu(newActiveChild))
            (*(prim->primitive_class.arm_and_activate)) (newActiveChild, NULL,
                                                                   NULL, NULL);
     }
      else if (XmIsGadget(newActiveChild))
      {
         XmGadgetClassRec * gadget;

         gadget = (XmGadgetClassRec *)XtClass(newActiveChild);

         if (!mst->MU_InPMMode && CBG_Submenu(newActiveChild))
            (*(gadget->gadget_class.arm_and_activate)) (newActiveChild, NULL,
                                                                   NULL, NULL);
      }
      return True;
   }
   else
      return False;
}

/*
 * Find the next hierarchy in the menubar which can be traversed to.
 */
static Boolean 
#ifdef _NO_PROTO
FindNextMenuBarCascade( menubar )
        XmRowColumnWidget menubar ;
#else
FindNextMenuBarCascade(
        XmRowColumnWidget menubar )
#endif /* _NO_PROTO */
{
   Widget active_child = NULL;
   register int i, j;
   int upper_limit;
   ShellWidget shell;
   XmMenuState mst = _XmGetMenuState((Widget)menubar);

   upper_limit = menubar->composite.num_children;

   /*
    * Determine which child is popped up.
    */
   shell = (ShellWidget) RC_PopupPosted(menubar);
   if (shell != NULL)
      active_child = mst->MU_CurrentMenuChild =
         RC_CascadeBtn(shell->composite.children[0]);

   /* Find the index of the currently active item */
   for (i = 0; i < upper_limit; i++)
   {
      if (menubar->composite.children[i] == mst->MU_CurrentMenuChild)
          break;
   }

   /* Start looking at the next child */
   for (j = 0, i++; j < upper_limit - 1; j++, i++)
   {
      /* Wrap, if necessary */
      if (i >= upper_limit)
          i = 0;

      mst->MU_CurrentMenuChild = menubar->composite.children[i];
      if (ValidateMenuBarCascade(active_child, mst->MU_CurrentMenuChild))
         return True;
   }
   return False;
}


/*
 * Find the previous hierarchy in the menubar which can be traversed to.
 */
static Boolean 
#ifdef _NO_PROTO
FindPrevMenuBarCascade( menubar )
        XmRowColumnWidget menubar ;
#else
FindPrevMenuBarCascade(
        XmRowColumnWidget menubar )
#endif /* _NO_PROTO */
{
    Widget active_child = NULL;
    register int i, j;
    int upper_limit;
    ShellWidget shell;
    XmMenuState mst = _XmGetMenuState((Widget)menubar);

    upper_limit = menubar->composite.num_children;

    /* Determine which child is popped up */
    shell = (ShellWidget) RC_PopupPosted(menubar);
    if (shell != NULL)
       active_child = mst->MU_CurrentMenuChild =
          RC_CascadeBtn(shell->composite.children[0]);

    /* Find the index of the currently active item */
    for (i = 0; i < upper_limit; i++)
    {
        if (menubar->composite.children[i] == mst->MU_CurrentMenuChild)
           break;
    }

    /* Start looking at the previous child */
    for (j = 0, --i; j < upper_limit - 1; j++, --i)
    {
        /* Wrap, if necessary */
        if (i < 0)
           i = upper_limit - 1;

        mst->MU_CurrentMenuChild = menubar->composite.children[i];
        if (ValidateMenuBarCascade(active_child, mst->MU_CurrentMenuChild))
           return True;
    }
    return False;
}

static Boolean 
#ifdef _NO_PROTO
ValidateMenuBarCascade (oldActiveChild, newMenuChild)
	Widget oldActiveChild, newMenuChild;
#else
ValidateMenuBarCascade (
	Widget oldActiveChild, Widget newMenuChild)
#endif
{
   XmRowColumnWidget menubar = (XmRowColumnWidget)XtParent(newMenuChild);
   /* Time _time = XtLastTimestampProcessed(XtDisplay(menubar)); */
   Time _time = CurrentTime;

   if (XmIsTraversable(newMenuChild))
   {
      if (XmIsCascadeButtonGadget(newMenuChild))
      {
         XmGadgetClassRec * gadget;

         gadget = (XmGadgetClassRec *)XtClass(newMenuChild);

         if (RC_PopupPosted(menubar) && !CBG_Submenu(newMenuChild))
         {
	     (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
		menu_shell_class.popdownEveryone))
		 (RC_PopupPosted(menubar),NULL, NULL, NULL);

            /* Return the X focus to the Menubar hierarchy from the menushell.
             * Set the Xt focus to the cascade
             */
            _XmMenuFocus((Widget) menubar, XmMENU_MIDDLE, _time);
            (void)XmProcessTraversal(newMenuChild, XmTRAVERSE_CURRENT);
         }
         else
         {
            (*(gadget->gadget_class.arm_and_activate)) (newMenuChild, NULL,
                                                                   NULL, NULL);
         }
         return True;
      }
      else if (XmIsCascadeButton (newMenuChild))
      {
         XmPrimitiveClassRec * prim;

         prim = (XmPrimitiveClassRec *)XtClass(newMenuChild);

         /* No submenu means PM mode */
         if (RC_PopupPosted(menubar) && !CB_Submenu(newMenuChild))
         {
	     (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
		menu_shell_class.popdownEveryone))
		 (RC_PopupPosted(menubar),NULL, NULL, NULL);

            /* Update X and Xt focus */
            _XmMenuFocus((Widget) menubar, XmMENU_MIDDLE, _time);
            (void)XmProcessTraversal(newMenuChild, XmTRAVERSE_CURRENT);
         }
         else
         {
            (*(prim->primitive_class.arm_and_activate)) (newMenuChild, NULL,
                                                                   NULL, NULL);
         }
         return True;
      }
   }
   return False;
}

/************************************************************************
 *
 * _XmSaveCoreClassTranslations
 *
 *  Save away the core class translations during the initialization
 *  routines.  This is used by rowcol and subclasses of Label that set their
 *  translations during initialization depending on whether they are in
 *  a menu.  The InitializePrehook calls this routine to save the
 *  core class translations.
 ************************************************************************/
void
#ifdef _NO_PROTO
_XmSaveCoreClassTranslations (widget)
        Widget widget;
#else
_XmSaveCoreClassTranslations(
        Widget widget)
#endif /* _NO_PROTO */
{
    struct _XmTranslRec *tempPtr = (struct _XmTranslRec *)
	XtMalloc(sizeof(struct _XmTranslRec));

    tempPtr->translations =
	(XtTranslations) widget->core.widget_class->core_class.tm_table;

    tempPtr->next =  (struct _XmTranslRec *) coreClassTranslations;

    coreClassTranslations = tempPtr;
}

/************************************************************************
 *
 * _XmRestoreCoreClassTranslations
 *
 *  Restore the core class translations during the initialization
 *  routines.  This is used by rowcol and subclasses of Label that set their
 *  translations during initialization depending on whether they are in
 *  a menu.  The InitializePosthook calls this routine to restore the
 *  core class translations.
 ************************************************************************/
void
#ifdef _NO_PROTO
_XmRestoreCoreClassTranslations (widget)
        Widget widget;
#else
_XmRestoreCoreClassTranslations(
        Widget widget)
#endif /* _NO_PROTO */
{
  if (coreClassTranslations)
  {
    struct _XmTranslRec * tempPtr;
    tempPtr = coreClassTranslations;
    
    widget->core.widget_class->core_class.tm_table =
	(String) tempPtr->translations;

    coreClassTranslations = (struct _XmTranslRec *) tempPtr->next;

    XtFree((char *) tempPtr);
  }
}

/************************************************************************
 *
 * _XmGetMenuState(wid)
 *
 ************************************************************************/
XmMenuState
#ifdef _NO_PROTO
_XmGetMenuState (wid)
        Widget wid;
#else
_XmGetMenuState(
        Widget wid)
#endif /* _NO_PROTO */
{
   XmScreen scrn = (XmScreen) XmGetXmScreen(XtScreen(wid));
   XmMenuState menu_state = (XmMenuState)NULL;

   if ((XmScreen)NULL != scrn)
   {
     menu_state  = 
	(XmMenuState)((XmScreenInfo *)(scrn->screen.screenInfo))->menu_state;

     if ((XmMenuState)NULL == menu_state)
     {
      menu_state = (XmMenuState)XtMalloc(sizeof(XmMenuStateRec));
      ((XmScreenInfo *)(scrn->screen.screenInfo))->menu_state = 
		(XtPointer)menu_state;
      XtAddCallback((Widget)scrn, XtNdestroyCallback, 
		MenuUtilScreenDestroyCallback, (XtPointer) NULL);

      menu_state->RC_LastSelectToplevel = NULL;
      menu_state->RC_ButtonEventStatus.time = -1;
      menu_state->RC_ButtonEventStatus.verified = FALSE;
      menu_state->RC_ButtonEventStatus.waiting_to_be_managed = TRUE;
      /*menu_state->RC_ButtonEventStatus.event = (XButtonEvent)NULL;*/
      menu_state->RC_ReplayInfo.time = 0;
      menu_state->RC_ReplayInfo.toplevel_menu = NULL;
      menu_state->RC_activeItem = NULL;
      menu_state->RC_allowAcceleratedInsensitiveUnmanagedMenuItems = False;
      menu_state->RC_menuFocus.oldFocus = (Window)NULL;
      menu_state->RC_menuFocus.oldRevert = 0;
      menu_state->RC_menuFocus.oldWidget = NULL;

      menu_state->MS_LastManagedMenuTime = (Time)0L;

      menu_state->MU_InDragMode = False;
      menu_state->MU_CurrentMenuChild = NULL;
      menu_state->MU_InPMMode = False;
     }
   }

   return menu_state;
}

static void 
MenuUtilScreenDestroyCallback
#ifdef _NO_PROTO
	( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
	( Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
   XmScreen scrn = (XmScreen) XmGetXmScreen(XtScreen(w));
   if ((XmScreen)NULL != scrn)
   {
	XmMenuState menu_state = 
  	  (XmMenuState)((XmScreenInfo *)(scrn->screen.screenInfo))->menu_state;
	if ((XmMenuState)NULL != menu_state)
	{
		XtFree((char*)menu_state);
		/* 
		((XmScreenInfo *)(scrn->screen.screenInfo))->menu_state = (XtPointer) NULL;
		*/
   	}
   }
}
