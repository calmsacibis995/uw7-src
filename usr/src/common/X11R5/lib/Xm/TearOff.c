#pragma ident	"@(#)m1.2libs:Xm/TearOff.c	1.4"
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

#include <X11/cursorfont.h>

#include <Xm/TearOffP.h>
#include "XmI.h"
#include <Xm/BaseClassP.h>
#include <Xm/DisplayP.h>
#include <Xm/AtomMgr.h>
#include <Xm/GadgetP.h>
#include <Xm/LabelP.h>
#include <Xm/MenuShellP.h>
#include <Xm/MenuUtilP.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumnP.h>
#include <Xm/SeparatorP.h>
#include <Xm/MwmUtil.h>
#include <Xm/VirtKeysP.h>

#ifndef _XA_WM_DELETE_WINDOW
#define _XA_WM_DELETE_WINDOW    "WM_DELETE_WINDOW"
#endif /* _XA_WM_DELETE_WINDOW */

#define IsPopup(m)     \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_POPUP)
#define IsPulldown(m)  \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_PULLDOWN)
#define IsOption(m)    \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_OPTION)
#define IsBar(m)       \
    (((XmRowColumnWidget) (m))->row_column.type == XmMENU_BAR)

/* Bury these here for now - not spec'd for 1.2, maybe for 1.3? */
#define CREATE_TEAR_OFF 0
#define RESTORE_TEAR_OFF_TO_TOPLEVEL_SHELL 1
#define RESTORE_TEAR_OFF_TO_MENUSHELL 2
#define DESTROY_TEAR_OFF 3

#define OUTLINE_WIDTH	2
#define SEGS_PER_DRAW	(4*OUTLINE_WIDTH)


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void InitXmTearOffXorGC() ;
static void SetupOutline() ;
static void EraseOutline() ;
static void MoveOutline() ;
static void MoveOpaque() ;
static void PullExposureEvents() ;
static void GetConfigEvent() ;
static Cursor GetTearOffCursor() ;
static Boolean DoPlacement() ;
static void CallTearOffMenuActivateCallback() ;
static void CallTearOffMenuDeactivateCallback() ;
static void RemoveTearOffEventHandlers();
static void DismissOnPostedFromDestroy();
static void DisplayDestroyCallback ();

#else

static void InitXmTearOffXorGC( 
                        Widget wid) ;
static void SetupOutline( 
                        Widget wid,
                        XSegment pOutline[],
                        XEvent *event,
			Dimension delta_x, 
			Dimension delta_y ) ;
static void EraseOutline( 
                        Widget wid,
                        XSegment *pOutline) ;
static void MoveOutline( 
                        Widget wid,
                        XSegment *pOutline,
                        XEvent *event,
			Dimension delta_x, 
			Dimension delta_y ) ;
static void PullExposureEvents( 
                        Widget wid ) ;
static void MoveOpaque( 
                        Widget wid,
                        XEvent *event,
			Dimension delta_x, 
			Dimension delta_y ) ;
static void GetConfigEvent( 
                        Display *display,
                        Window window,
                        unsigned long mask,
                        XEvent *event) ;
static Cursor GetTearOffCursor( 
                        Widget wid) ;
static Boolean DoPlacement( 
                        Widget wid,
                        XEvent *event) ;
static void CallTearOffMenuActivateCallback( 
                        Widget wid,
                        XEvent *event,
#if NeedWidePrototypes
			int origin) ;
#else
			unsigned short origin) ;
#endif	/*NeedWidePrototypes */
static void CallTearOffMenuDeactivateCallback( 
                        Widget wid,
                        XEvent *event,
#if NeedWidePrototypes
			int origin) ;
#else
			unsigned short origin) ;
#endif	/*NeedWidePrototypes */
static void RemoveTearOffEventHandlers(
			Widget wid ) ;
static void DismissOnPostedFromDestroy(
			Widget w,
			XtPointer clientData,
			XtPointer callData ) ;
static void DisplayDestroyCallback ( 
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data );
#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/* The Drag stuff wasn't ready, nor compatible with tear offs in a timely
 * manner, so unfortunately much of the code is duplicated.
 */
static GC _XmTearOffXorGC;

/*
 * needed to cause latent restoration of tear off when menu is popping
 * down from a button activate so activateCBs get useful info.
 */
externaldef(_xmexcludedparentpane)
XmExcludedParentPaneRec _XmExcludedParentPane = {0, NULL, 0};

static void
#ifdef _NO_PROTO
InitXmTearOffXorGC(wid)
	Widget wid;
#else
InitXmTearOffXorGC(
	Widget wid )
#endif
{
    XGCValues gcv;
    XtGCMask  mask;

    mask = GCFunction | GCLineWidth | GCSubwindowMode | GCCapStyle;
    gcv.function = GXinvert;
    gcv.line_width = 0;
    gcv.cap_style = CapNotLast;
    gcv.subwindow_mode = IncludeInferiors;

    _XmTearOffXorGC = XCreateGC (XtDisplay(wid), wid->core.screen->root,
       mask, &gcv);
}

static void
#ifdef _NO_PROTO
SetupOutline(wid, pOutline, event, delta_x, delta_y)
	Widget wid;
	XSegment pOutline[];
	XEvent *event;
	Dimension delta_x; 
	Dimension delta_y;
#else
SetupOutline(
	Widget wid,
	XSegment pOutline[],
	XEvent *event,
	Dimension delta_x, 
	Dimension delta_y) 
#endif
{
   Position x, y;
   Dimension w, h;
   int n = 0;
   int i;

   x = event->xbutton.x_root - delta_x;
   y = event->xbutton.y_root - delta_y;
   w = wid->core.width;
   h = wid->core.height;

   for(i=0; i<OUTLINE_WIDTH; i++)
   {
      pOutline[n].x1 = x;
      pOutline[n].y1 = y;
      pOutline[n].x2 = x + w - 1;
      pOutline[n++].y2 = y;

      pOutline[n].x1 = x + w - 1;
      pOutline[n].y1 = y;
      pOutline[n].x2 = x + w - 1;
      pOutline[n++].y2 = y + h - 1;

      pOutline[n].x1 = x + w - 1;
      pOutline[n].y1 = y + h  - 1;
      pOutline[n].x2 = x;
      pOutline[n++].y2 = y + h - 1;

      pOutline[n].x1 = x;
      pOutline[n].y1 = y + h  - 1;
      pOutline[n].x2 = x;
      pOutline[n++].y2 = y;

      x += 1;
      y += 1;
      w -= 2;
      h -= 2;
   }

   XDrawSegments(XtDisplay(wid), wid->core.screen->root, _XmTearOffXorGC,
      pOutline, SEGS_PER_DRAW);
}

static void
#ifdef _NO_PROTO
EraseOutline(wid, pOutline)
	Widget wid;
	XSegment *pOutline;
#else
EraseOutline(
	Widget wid,
	XSegment *pOutline )
#endif
{
   XDrawSegments(XtDisplay(wid), wid->core.screen->root, _XmTearOffXorGC,
      pOutline, SEGS_PER_DRAW);
}

static void
#ifdef _NO_PROTO
MoveOutline(wid, pOutline, event, delta_x, delta_y)
	Widget wid;
	XSegment *pOutline;
	XEvent *event;
	Dimension delta_x; 
	Dimension delta_y;
#else
MoveOutline(
	Widget wid,
	XSegment *pOutline,
	XEvent *event,
	Dimension delta_x, 
	Dimension delta_y )
#endif
{
   EraseOutline(wid, pOutline);
   SetupOutline(wid, pOutline, event, delta_x, delta_y);
}

static void
#ifdef _NO_PROTO
PullExposureEvents(wid)
	Widget wid;
#else
PullExposureEvents(
	Widget wid )
#endif
{
   XEvent      event;
   /*
    * Force the exposure events into the queue
    */
   XSync (XtDisplay(wid), False);

   /*
    * Selectively extract the exposure events
    */
   while (XCheckMaskEvent (XtDisplay(wid), ExposureMask, &event))
   {
      /*
       * Dispatch widget related event:
       */
      XtDispatchEvent (&event);
   }
}

static void
#ifdef _NO_PROTO
MoveOpaque(wid, event, delta_x, delta_y)
	Widget wid;
	XEvent *event;
	Dimension delta_x; 
	Dimension delta_y;
#else
MoveOpaque(
	Widget wid,
	XEvent *event,
	Dimension delta_x, 
	Dimension delta_y )
#endif
{
   /* Move the MenuShell */
   XMoveWindow(XtDisplay(wid), XtWindow(XtParent(wid)), 
      event->xbutton.x_root - delta_x, event->xbutton.y_root - delta_y);

   /* cleanup exposed frame parts */
   PullExposureEvents (wid);
}


#define CONFIG_GRAB_MASK (ButtonPressMask|ButtonReleaseMask|\
		     PointerMotionMask|PointerMotionHintMask|\
		     KeyPress|KeyRelease)

#define POINTER_GRAB_MASK (ButtonPressMask|ButtonReleaseMask|\
                   PointerMotionMask|PointerMotionHintMask)


static void
#ifdef _NO_PROTO
GetConfigEvent(display, window, mask, event)
	Display *display;
	Window window;
	unsigned long mask;
	XEvent *event;
#else
GetConfigEvent(
	Display *display,
	Window window,
	unsigned long mask,
	XEvent *event )
#endif
{
   Window root_ret, child_ret;
   int root_x, root_y, win_x, win_y;
   unsigned int mask_ret;

   /* Block until a motion, button, or key event comes in */
   XWindowEvent(display, window, mask, event);

   if (event->type == MotionNotify &&
        event->xmotion.is_hint == NotifyHint)
   {
       /*
        * "Ack" the motion notify hint
        */
       if ((XQueryPointer (display, window, &root_ret,
               &child_ret, &root_x, &root_y, &win_x,
               &win_y, &mask_ret)))
       {
           /*
            * The query pointer values say that the pointer
            * moved to a new location.
            */
           event->xmotion.window = root_ret;
           event->xmotion.subwindow = child_ret;
           event->xmotion.x = root_x;
           event->xmotion.y = root_y;
           event->xmotion.x_root = root_x;
           event->xmotion.y_root = root_y;
       }
   }
}

static void 
DisplayDestroyCallback 
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
	XFreeCursor(XtDisplay(w), (Cursor)client_data);
}

static Cursor
#ifdef _NO_PROTO
GetTearOffCursor(wid)
       Widget wid;
#else
GetTearOffCursor(
       Widget wid )
#endif
{
	XmDisplay   dd = (XmDisplay) XmGetXmDisplay(XtDisplay(wid));
	Cursor TearOffCursor = 
		((XmDisplayInfo *)(dd->display.displayInfo))->TearOffCursor;
	
	if (0L == TearOffCursor)
		{
		/* create some data shared among all instances on this 
		** display; the first one along can create it, and 
		** any one can remove it; note no reference count
		*/
        	TearOffCursor = 
			XCreateFontCursor(XtDisplay(wid), XC_fleur);
		if (0L == TearOffCursor)
			TearOffCursor = XmGetMenuCursor(XtDisplay(wid));
		else
			XtAddCallback((Widget)dd, XtNdestroyCallback, 
			  DisplayDestroyCallback,(XtPointer)TearOffCursor);
		((XmDisplayInfo *)(dd->display.displayInfo))->TearOffCursor = 
			TearOffCursor;
		}

   return TearOffCursor;
}

static Boolean
#ifdef _NO_PROTO
DoPlacement( wid, event )
       Widget wid;
       XEvent *event;
#else
DoPlacement( 
       Widget wid,
       XEvent *event )
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XSegment outline[SEGS_PER_DRAW];
   Boolean placementDone;
   KeyCode KCancel;
   KeySym keysym = osfXK_Cancel;
   Modifiers mods;
   Boolean moveOpaque = False;
   Dimension delta_x, delta_y;
   Dimension old_x_root = 0;
   Dimension old_y_root = 0;

   _XmVirtualToActualKeysym(XtDisplay(rc), keysym, &keysym, &mods);
   KCancel = XKeysymToKeycode(XtDisplay(rc), keysym);

   /* Regrab the pointer and keyboard to the root window.  Grab in Async
    * mode to free up the input queues.
    */
   XGrabPointer(XtDisplay(rc), rc->core.screen->root, FALSE,
        (unsigned int)POINTER_GRAB_MASK,
        GrabModeAsync, GrabModeAsync, rc->core.screen->root,
        GetTearOffCursor(wid), CurrentTime);

   XGrabKeyboard(XtDisplay(rc), rc->core.screen->root, FALSE,
      GrabModeAsync, GrabModeAsync, CurrentTime);

   InitXmTearOffXorGC((Widget)rc);

   delta_x = event->xbutton.x_root - XtX(XtParent(rc));
   delta_y = event->xbutton.y_root - XtY(XtParent(rc));

   moveOpaque = _XmGetMoveOpaqueByScreen(XtScreen(rc));

   /* Set up a dummy event of the menu's current position in case the
    * move-opaque is cancelled.
    */
   if (moveOpaque)
   {
     old_x_root = XtX(XtParent(rc));
     old_y_root = XtY(XtParent(rc));
     MoveOpaque((Widget)rc, event, delta_x, delta_y);
   }
   else
      SetupOutline((Widget)rc, outline, event, delta_x, delta_y);

   placementDone = FALSE;

   while (!placementDone)
   {
      GetConfigEvent (XtDisplay(rc), rc->core.screen->root, CONFIG_GRAB_MASK,
         event);        /* ok to overwrite event? */

      switch (event->type)
      {
         case ButtonRelease:
            if (event->xbutton.button == /*BDrag */ 2)
            {
	       if (!moveOpaque)
                  EraseOutline((Widget)rc, outline);
	       else
		  /* Signal next menushell post to reposition */
		  XtX(XtParent(rc)) = XtY(XtParent(rc)) = 0;

               placementDone = TRUE;
	       event->xbutton.x_root -= delta_x;
	       event->xbutton.y_root -= delta_y;
            }
            break;

         case MotionNotify:
	    if (moveOpaque)
	       MoveOpaque((Widget)rc, event, delta_x, delta_y);
	    else
               MoveOutline((Widget)rc, outline, event, delta_x, delta_y);
            break;

         case KeyPress:
            if (event->xkey.keycode == KCancel)
            {
	       if (!moveOpaque)
                  EraseOutline((Widget)rc, outline);
	       else
	       {
		  event->xbutton.x_root = old_x_root;
		  event->xbutton.y_root = old_y_root;
		  MoveOpaque((Widget)rc, event, 0, 0);
	       }
               return(FALSE);
            }
            break;
      }
   }
   XFreeGC(XtDisplay(rc), _XmTearOffXorGC);

   XUngrabKeyboard(XtDisplay(rc), CurrentTime);
   XUngrabPointer(XtDisplay(rc), CurrentTime);

   return (TRUE);
}

static void
#ifdef _NO_PROTO
CallTearOffMenuActivateCallback(wid, event, origin)
	Widget wid;
	XEvent *event;
	unsigned short origin;
#else
CallTearOffMenuActivateCallback(
	Widget wid,
	XEvent *event,
#if NeedWidePrototypes
	int origin )
#else
	unsigned short origin )
#endif	/*NeedWidePrototypes */
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XmRowColumnCallbackStruct callback;

   if (!rc->row_column.tear_off_activated_callback)
      return;

   callback.reason = XmCR_TEAR_OFF_ACTIVATE;
   callback.event  = event;	
   callback.widget = NULL;	/* these next two fields are spec'd NULL */
   callback.data   = (char *) origin;
   callback.callbackstruct = NULL;
   XtCallCallbackList ((Widget)rc, rc->row_column.tear_off_activated_callback, 
      &callback);
}

static void
#ifdef _NO_PROTO
CallTearOffMenuDeactivateCallback(wid, event, origin)
	Widget wid;
	XEvent *event;
	unsigned short origin;
#else
CallTearOffMenuDeactivateCallback(
	Widget wid,
	XEvent *event,
#if NeedWidePrototypes
	int origin )
#else
	unsigned short origin )
#endif	/*NeedWidePrototypes */
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget) wid;
   XmRowColumnCallbackStruct callback;

   if (!rc->row_column.tear_off_deactivated_callback)
      return;

   callback.reason = XmCR_TEAR_OFF_DEACTIVATE;
   callback.event  = event;
   callback.widget = NULL;	/* these next two fields are spec'd NULL */
   callback.data   = (char *) origin;
   callback.callbackstruct = NULL;
   XtCallCallbackList ((Widget) rc, 
      rc->row_column.tear_off_deactivated_callback, &callback);
}

/*
 * This event handler is added to label widgets and separator widgets inside
 * a tearoff menu pane.  This enables the RowColumn to watch for the button
 * presses inside these widgets and to 'do the right thing'.
 */
void 
#ifdef _NO_PROTO
_XmTearOffBtnDownEventHandler( reportingWidget, data, event, cont )
        Widget reportingWidget ;
        XtPointer data ;
        XEvent *event ;
        Boolean *cont ;
#else
_XmTearOffBtnDownEventHandler(
        Widget reportingWidget,
        XtPointer data,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    Widget parent;

    if (reportingWidget)
    {
       /* make sure only called for widgets inside a menu rowcolumn */
	  if ((XmIsRowColumn(parent = XtParent(reportingWidget))) &&
	      (RC_Type(parent) != XmWORK_AREA))
	  {
	      _XmMenuBtnDown (parent, event, NULL, 0);
	  }
    }
    *cont = True;
}

void 
#ifdef _NO_PROTO
_XmTearOffBtnUpEventHandler( reportingWidget, data, event, cont )
        Widget reportingWidget ;
        XtPointer data ;
        XEvent *event ;
        Boolean *cont ;
#else
_XmTearOffBtnUpEventHandler(
        Widget reportingWidget,
        XtPointer data,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    Widget parent;

    if (reportingWidget)
    {
       /* make sure only called for widgets inside a menu rowcolumn */
	  if ((XmIsRowColumn(parent = XtParent(reportingWidget))) &&
	      (RC_Type(parent) != XmWORK_AREA))
	  {
	      _XmMenuBtnUp (parent, event, NULL, 0);
	  }
    }
    *cont = True;
}

void
#ifdef _NO_PROTO
_XmAddTearOffEventHandlers(wid)
	Widget wid;
#else
_XmAddTearOffEventHandlers(
	Widget wid )
#endif
{
    XmRowColumnWidget rc = (XmRowColumnWidget) wid;
    Widget child;
    int i;
    Cursor cursor = XmGetMenuCursor(XtDisplay(wid));

    for (i=0; i < rc->composite.num_children; i++)
    {
	child = rc->composite.children[i];

	/*
	 * the label and separator widgets do not care about
	 * button presses.  Add an event handler for these widgets
	 * so that the tearoff RowColumn is alerted about presses in
	 * these widgets.
	 */
	if ((XtClass(child) == xmLabelWidgetClass) ||
	    (XtClass(child) == xmSeparatorWidgetClass))
	{
    
	    XtAddEventHandler(child, ButtonPressMask, False,
			      _XmTearOffBtnDownEventHandler,  NULL);
	    XtAddEventHandler(child, ButtonReleaseMask, False,
			      _XmTearOffBtnUpEventHandler,  NULL);
	}

	if (XtIsWidget(child))
	   XtGrabButton (child, (int)AnyButton, AnyModifier, TRUE, 
	      (unsigned int)ButtonPressMask, GrabModeAsync, GrabModeAsync, 
	      None, cursor);
    }
}


static void
#ifdef _NO_PROTO
RemoveTearOffEventHandlers(wid)
	Widget wid;
#else
RemoveTearOffEventHandlers(
	Widget wid )
#endif
{
    XmRowColumnWidget rc = (XmRowColumnWidget) wid;
    Widget child;
    int i;

    for (i=0; i < rc->composite.num_children; i++)
    {
	child = rc->composite.children[i];

	/*
	 * Remove the event handlers on the label and separator widgets.
	 */
	if ((XtClass(child) == xmLabelWidgetClass) ||
	    (XtClass(child) == xmSeparatorWidgetClass))
	{
    
	    XtRemoveEventHandler(child, ButtonPressMask, False,
				 _XmTearOffBtnDownEventHandler,  NULL);
	    XtRemoveEventHandler(child, ButtonReleaseMask, False,
				 _XmTearOffBtnUpEventHandler,  NULL);
	}

	if (XtIsWidget(child) && !child->core.being_destroyed)
	   XtUngrabButton(child, (unsigned int)AnyButton, AnyModifier);
    }
}

void
#ifdef _NO_PROTO
_XmDestroyTearOffShell(wid)
	Widget wid;
#else
_XmDestroyTearOffShell(
	Widget wid )
#endif
{
   TopLevelShellWidget to_shell = (TopLevelShellWidget)wid;

   to_shell->composite.num_children = 0;
   
   if (to_shell->core.being_destroyed)
     return;

   XtPopdown((Widget)to_shell);
   /*to_shell->composite.num_children = 0;*/
   if (to_shell->core.background_pixmap != XtUnspecifiedPixmap)
   {
     XFreePixmap(XtDisplay(to_shell), to_shell->core.background_pixmap);
     to_shell->core.background_pixmap = XtUnspecifiedPixmap;
   }
   XtDestroyWidget((Widget)to_shell);
}

void
#ifdef _NO_PROTO
_XmDismissTearOff(shell, closure, call_data)
	Widget shell;
        XtPointer closure;
        XtPointer call_data;
#else
_XmDismissTearOff(
	Widget shell,
        XtPointer closure,
        XtPointer call_data)
#endif
{
   XmRowColumnWidget submenu = NULL;

   /* The first time a pane is torn, there's no tear off shell to destroy.
    */
   if (!shell ||
       !(((ShellWidget)shell)->composite.num_children) ||
       !(submenu = 
	 (XmRowColumnWidget)(((ShellWidget)shell)->composite.children[0])) ||
       !RC_TornOff(submenu))
      return;

   RC_SetTornOff(submenu, FALSE);
   RC_SetTearOffActive(submenu, FALSE);
   
   /* Unhighlight the active child and clear the focus for the next post */
   if (submenu->manager.active_child)
   {
      /* update visible focus/highlighting */
      if (XmIsPrimitive(submenu->manager.active_child))
      {
	  (*(((XmPrimitiveClassRec *)XtClass(submenu->manager.
	  active_child))->primitive_class.border_unhighlight))
	  (submenu->manager.active_child);
      }
      else if (XmIsGadget(submenu->manager.active_child))
      {
	  (*(((XmGadgetClassRec *)XtClass(submenu->manager.
	  active_child))->gadget_class.border_unhighlight))
	  (submenu->manager.active_child);
      }
      /* update internal focus state */
      _XmClearFocusPath((Widget) submenu);

      /* Clear the Intrinsic focus from the tear off widget hierarchy.
       * Necessary to remove the FocusChangeCallback from the active item.
       */
      XtSetKeyboardFocus(shell, NULL);
   }

   if (XmIsMenuShell(shell))
   {
      /* Shared menupanes require extra manipulation.  We gotta be able
       * to optimize this when there's more time.
       */
      if ((((ShellWidget)shell)->composite.num_children) > 1)
         XUnmapWindow(XtDisplay(submenu), XtWindow(submenu));

      _XmDestroyTearOffShell(RC_ParentShell(submenu));

      /* remove orphan destroy callback from postFromWidget */
      XtRemoveCallback(submenu->row_column.tear_off_lastSelectToplevel,
	 XtNdestroyCallback, (XtCallbackProc)DismissOnPostedFromDestroy, 
	 (XtPointer) RC_ParentShell(submenu));
   } 
   else	/* toplevel shell! */
   {
      /* Shared menupanes require extra manipulation.  We gotta be able
       * to optimize this when there's more time.
       */
      if ((((ShellWidget)RC_ParentShell(submenu))->composite.num_children) > 1)
         XUnmapWindow(XtDisplay(submenu), XtWindow(submenu));


      _XmDestroyTearOffShell(shell);
      if (submenu)
      {
	 XtParent(submenu) = RC_ParentShell(submenu);
         XReparentWindow(XtDisplay(submenu), XtWindow(submenu), 
	    XtWindow(XtParent(submenu)), XtX(submenu), XtY(submenu));
         submenu->core.mapped_when_managed = False;
         submenu->core.managed = False;
         XtManageChild(RC_TearOffControl(submenu));
      }

      /* Only Call the callbacks if we're not popped up (parent is not a
       * menushell).  Popping up the shell caused the unmap & deactivate
       * callbacks to be called already.
       */
      _XmCallRowColumnUnmapCallback((Widget)submenu, NULL);
      CallTearOffMenuDeactivateCallback((Widget)submenu, (XEvent *)closure,
	 DESTROY_TEAR_OFF);
      RemoveTearOffEventHandlers ((Widget) submenu);

      /* remove orphan destroy callback from postFromWidget */
      XtRemoveCallback(submenu->row_column.tear_off_lastSelectToplevel,
	 XtNdestroyCallback, (XtCallbackProc)DismissOnPostedFromDestroy, 
	 (XtPointer) shell);
   }
}

static void 
#ifdef _NO_PROTO
DismissOnPostedFromDestroy(w,  clientData, callData)
	Widget w;
	XtPointer clientData, callData;
#else
DismissOnPostedFromDestroy(
	Widget w,
	XtPointer clientData,
	XtPointer callData )
#endif
{
   _XmDismissTearOff((Widget)clientData, NULL, NULL);
}

#define DEFAULT_TEAR_OFF_TITLE ""
#define TEAR_OFF_TITLE_SUFFIX " Tear-off"

void
#ifdef _NO_PROTO
_XmTearOffInitiate(wid, event)
        Widget wid;
        XEvent *event ;
#else
_XmTearOffInitiate(
        Widget wid,
        XEvent *event )
#endif
{
   XmRowColumnWidget submenu = (XmRowColumnWidget)wid;
   Widget cb;
   XmRowColumnWidget rc;
   XmString label_xms;
   unsigned char label_type;
   Arg args[20];
   ShellWidget to_shell;
   Widget toplevel;
   Atom delete_atom;
   PropMwmHints *rprop = NULL;     /* receive pointer */
   PropMwmHints sprop;          /* send structure */
   Atom mwm_hints_atom;
   Atom actual_type;
   int actual_format;
   unsigned long num_items, bytes_after;
   XEvent newEvent;
   XmMenuState mst = _XmGetMenuState((Widget)wid);

   if (IsPulldown(submenu))
      cb = RC_CascadeBtn(submenu);
   else
      cb = NULL;

   if (RC_TearOffModel(submenu) == XmTEAR_OFF_DISABLED)
      return;

   /* The submenu must be posted before we allow the tear off action */
   if (!(XmIsMenuShell(XtParent(submenu)) &&
         ((XmMenuShellWidget)XtParent(submenu))->shell.popped_up))
      return;

   if (XmIsRowColumn(wid))
      rc = (XmRowColumnWidget)wid;
   else
      rc = (XmRowColumnWidget) XtParent (wid);
   _XmGetActiveTopLevelMenu((Widget)rc, (Widget *)&rc);

   /* Set up the important event fields for the new position */
   memcpy(&newEvent, event, sizeof(XButtonEvent));

   /* if it's from a BDrag, find the eventual destination */
   if ((event->xbutton.type == ButtonPress) &&
       (event->xbutton.button == 2/*BDrag*/))
   {
      if (!DoPlacement((Widget) submenu, &newEvent))     /* Cancelled! */
      {
         /* restore grabs back to menu and reset menu cursor? */

         /* If the toplevel menu is an option menu, the grabs are attached to
          * the top submenu else leave it as the menubar or popup.
          */
         if (IsOption(rc))
            rc = (XmRowColumnWidget)RC_OptionSubMenu(rc);

         _XmGrabPointer((Widget) rc, True,
            (unsigned int)(ButtonPressMask | ButtonReleaseMask |
            EnterWindowMask | LeaveWindowMask),
            GrabModeSync, GrabModeAsync, None,
            XmGetMenuCursor(XtDisplay(rc)), CurrentTime);

         _XmGrabKeyboard((Widget)rc, True, GrabModeSync, GrabModeSync,
            CurrentTime);
         XAllowEvents(XtDisplay(rc), AsyncKeyboard, CurrentTime);

         XAllowEvents(XtDisplay(rc), SyncPointer, CurrentTime);

         /* and reset the input focus to the leaf submenu's menushell */
         _XmMenuFocus(XtParent(submenu), XmMENU_MIDDLE, CurrentTime);
         return;
      }
   } 
   else
   {
      newEvent.xbutton.x_root = XtX(XtParent(submenu));
      newEvent.xbutton.y_root = XtY(XtParent(submenu));
   }
	  

   /* If the submenu already exists, take it down for a retear */
   _XmDismissTearOff(XtParent(submenu), (XtPointer)event, NULL);

   /* Shared menupanes require extra manipulation.  We gotta be able
    * to optimize this when there's more time.
    */
   if ((((ShellWidget)XtParent(submenu))->composite.num_children) > 1)
      XMapWindow(XtDisplay(submenu), XtWindow(submenu));

   /*
    * Popdown the menu hierarchy!
    */
   /* First save the GetPostedFromWidget */
   if (mst->RC_LastSelectToplevel)
   {
      submenu->row_column.tear_off_lastSelectToplevel =
	  mst->RC_LastSelectToplevel;
   }
   else
      if (RC_TornOff(rc) && RC_TearOffActive(rc))
         submenu->row_column.tear_off_lastSelectToplevel =
            rc->row_column.tear_off_lastSelectToplevel;
      else
      {
	 if (IsPopup(submenu))
	    submenu->row_column.tear_off_lastSelectToplevel =
	       RC_CascadeBtn(submenu);
	 else
	    submenu->row_column.tear_off_lastSelectToplevel = (Widget)rc;
      }

   if (!XmIsMenuShell(XtParent(rc)))    /* MenuBar or TearOff */
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
         menu_shell_class.popdownEveryone))(RC_PopupPosted(rc), event, NULL,
            NULL);
   else
      (*(((XmMenuShellClassRec *)xmMenuShellWidgetClass)->
         menu_shell_class.popdownEveryone))(XtParent(rc), event, NULL, NULL);

   _XmSetInDragMode((Widget) rc, False);

   /* popdownEveryone() calls popdownDone() which will call MenuDisarm() for
    * each pane.  We need to take care of non-popup toplevel menus (bar/option).
    */
   (*(((XmRowColumnClassRec *)XtClass(rc))->row_column_class.
      menuProcedures)) (XmMENU_DISARM, (Widget) rc);

   _XmMenuFocus( (Widget) rc, XmMENU_END, CurrentTime);
   XtUngrabPointer( (Widget) rc, CurrentTime);

   XtUnmanageChild(RC_TearOffControl(submenu));

   /* Use the toplevel application shell for the parent of the new transient
    * shell.  This way, the submenu won't inadvertently be destroyed on 
    * associated-widget's shell destruction.  We'll make the connection to 
    * associate-widget with XmNtransientFor.
    */
   for (toplevel = wid; XtParent(toplevel); )
      toplevel = XtParent(toplevel);

   XtSetArg(args[0], XmNdeleteResponse, XmDO_NOTHING);
   /* system & title */
   XtSetArg(args[1], XmNmwmDecorations, 
      MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU);
   XtSetArg(args[2], XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE);
   /* need shell resize for pulldown to resize correctly when reparenting
    * back without tear off control managed.
    */
   XtSetArg(args[3], XmNallowShellResize, True);
   if (IsPopup(submenu->row_column.tear_off_lastSelectToplevel))
   {
      XtSetArg(args[4], XmNtransientFor, 
	 _XmFindTopMostShell(RC_CascadeBtn(
	    submenu->row_column.tear_off_lastSelectToplevel)));
   }
   else
      XtSetArg(args[4], XmNtransientFor, 
	 _XmFindTopMostShell(submenu->row_column.tear_off_lastSelectToplevel));
   /* Sorry, still a menu - explicit mode only so focus doesn't screw up */
   XtSetArg(args[5], XmNkeyboardFocusPolicy, XmEXPLICIT);
   /* Start Fix CR 5459
    * It is important to set the shell's visual information (visual, colormap,
    * depth) to match the menu whose parent it is becoming.  If you fail to do
    * so, then when the user brings up a second copy of a menu that is already
    * posted, a BADMATCH error occurs.
    */
   XtSetArg(args[6], XmNvisual,
	    ((XmMenuShellWidget)(XtParent(wid)))->shell.visual);
   XtSetArg(args[7], XmNcolormap,
	    ((XmMenuShellWidget)(XtParent(wid)))->core.colormap);
   XtSetArg(args[8], XmNdepth,
	    ((XmMenuShellWidget)(XtParent(wid)))->core.depth);
   /* End Fix CR 5459 */
   to_shell = (ShellWidget)XtCreatePopupShell(DEFAULT_TEAR_OFF_TITLE,
      transientShellWidgetClass,
      toplevel, args, 9);

   if (cb)
   {
      Widget lwid, mwid;

      /* If the top menu of the active menu hierarchy is an option menu,
       * use the option menu's label for the name of the shell.
       */
      mwid = XmGetPostedFromWidget(XtParent(cb));
      if (mwid && IsOption(mwid))
	 lwid = XmOptionLabelGadget(mwid);
      else
	 lwid = cb;

      XtSetArg(args[0], XmNlabelType, &label_type); 
      XtSetArg(args[1], XmNlabelString, &label_xms); 
      XtGetValues((Widget)lwid, args, 2);

      if (label_type == XmSTRING)	/* better be a compound string! */
      {
	 XmStringContext context;
	 char *text;
	 XmStringCharSet xms_charset;
	 XmStringDirection xms_direction;
	 Boolean separator;
	 XmString title_xms, suffix_xms;

	 if (XmStringInitContext (&context, label_xms))
	 {
	    /* Get the charset so we can build an xms suffix */
	    if (XmStringGetNextSegment (context, &text, &xms_charset, 
	        &xms_direction, &separator))
	    {
	       suffix_xms = XmStringCreate(TEAR_OFF_TITLE_SUFFIX, xms_charset);

	       title_xms = XmStringConcat(label_xms, suffix_xms);

	       _XmStringUpdateWMShellTitle(title_xms, (Widget)to_shell);

	       XtFree(text);
	       XtFree((char*)xms_charset);
	       XmStringFree(title_xms);
	       XmStringFree(suffix_xms);
	    }
	    XmStringFreeContext(context);
	 }
	 XmStringFree(label_xms);
      } 
   }

   delete_atom = XmInternAtom(XtDisplay(to_shell), _XA_WM_DELETE_WINDOW,
       FALSE);

   XmAddWMProtocolCallback((Widget)to_shell, delete_atom,
      _XmDismissTearOff, NULL);

   /* Add internal destroy callback to postFromWidget to eliminate orphan tear
    * off menus.
    */
   XtAddCallback(submenu->row_column.tear_off_lastSelectToplevel,
      XtNdestroyCallback, (XtCallbackProc)DismissOnPostedFromDestroy, 
      (XtPointer) to_shell);

   RC_ParentShell(submenu) = XtParent(submenu);
   XtParent(submenu) = (Widget)to_shell;

   /* Needs to be set before the user gets a callback */
   RC_SetTornOff(submenu, TRUE);
   RC_SetTearOffActive(submenu, TRUE);

   _XmAddTearOffEventHandlers ((Widget) submenu);
   CallTearOffMenuActivateCallback((Widget)submenu, event,
      CREATE_TEAR_OFF);
   _XmCallRowColumnMapCallback((Widget)submenu, event);

   /* To get Traversal: _XmGetManagedInfo() to work correctly */
   submenu->core.mapped_when_managed = True;
   XtManageChild((Widget)submenu);

   /* Insert submenu into the new toplevel shell */
   (*((TransientShellWidgetClass)transientShellWidgetClass)->
      composite_class.insert_child)((Widget)submenu);

   /* Quick!  Configure the size (and location) of the shell before managing
    * so submenu doesn't get resized.
    */
   XtConfigureWidget((Widget) to_shell,
      newEvent.xbutton.x_root, newEvent.xbutton.y_root,
      XtWidth(submenu), XtHeight(submenu), XtBorderWidth(to_shell));

   /* Call change_managed routine to set up focus info */
   (*((TransientShellWidgetClass)transientShellWidgetClass)->
      composite_class.change_managed) ((Widget)to_shell);

   XtRealizeWidget((Widget)to_shell);

   /* Wait until after to_shell realize to set the focus */
   XmProcessTraversal((Widget)submenu, XmTRAVERSE_CURRENT);

   mwm_hints_atom = XmInternAtom(XtDisplay(to_shell), _XA_MWM_HINTS, FALSE);

   XGetWindowProperty(XtDisplay(to_shell), XtWindow(to_shell),
      mwm_hints_atom, 0, PROP_MWM_HINTS_ELEMENTS, False, mwm_hints_atom,
      &actual_type, &actual_format, &num_items, &bytes_after,
      (unsigned char **)&rprop);

   if ((actual_type != mwm_hints_atom) ||
       (actual_format != 32) ||
       (num_items < PROP_MOTIF_WM_INFO_ELEMENTS))
   {
       if (rprop != NULL) XFree((char *)rprop);
   }
   else
   {
      memset((void *)&sprop, 0, sizeof(sprop));
      memcpy(&sprop, rprop, (size_t)(actual_format >> 3) * num_items);
      if (rprop != NULL) XFree((char *)rprop);


      sprop.flags |= MWM_HINTS_STATUS;
      sprop.status |= MWM_TEAROFF_WINDOW;
      XChangeProperty(XtDisplay(to_shell), XtWindow(to_shell),
         mwm_hints_atom, mwm_hints_atom, 32, PropModeReplace,
         (unsigned char *) &sprop, PROP_MWM_HINTS_ELEMENTS);
   }

   /* Notify the server of the change */
   XReparentWindow(XtDisplay(to_shell), XtWindow(submenu), XtWindow(to_shell),
     0, 0);

   XtPopup((Widget)to_shell, XtGrabNone);

   RC_SetArmed (submenu, FALSE);

   RC_SetTearOffDirty(submenu, FALSE);
}

Boolean
#ifdef _NO_PROTO
_XmIsTearOffShellDescendant( wid )
	Widget wid;
#else
_XmIsTearOffShellDescendant(
	Widget wid )
#endif
{
   XmRowColumnWidget rc = (XmRowColumnWidget)wid;
   Widget cb;

   while (rc && (IsPulldown(rc) || IsPopup(rc)) && XtIsShell(XtParent(rc)))
   {
      if (RC_TearOffActive(rc))
         return(True);

      /* Popup is the top! "cascadeBtn" is postFromWidget! */
      if (IsPopup(rc))	
	 break;

      if (!(cb = RC_CascadeBtn(rc)))
         break;
      rc = (XmRowColumnWidget)XtParent(cb);
   }

   return (False);
}

void
#ifdef _NO_PROTO
_XmLowerTearOffObscuringPoppingDownPanes( ancestor, tearOff )
        Widget ancestor;
	Widget tearOff;
#else
_XmLowerTearOffObscuringPoppingDownPanes(
        Widget ancestor, 
	Widget tearOff )
#endif
{
   XRectangle tearOff_rect, intersect_rect;
   ShellWidget shell;

   _XmSetRect(&tearOff_rect, tearOff);
   if (IsBar(ancestor) || IsOption(ancestor))
   {
      if ((shell = (ShellWidget)RC_PopupPosted(ancestor)) != NULL)
	 ancestor = shell->composite.children[0];
   }

   while (ancestor && (IsPulldown(ancestor) || IsPopup(ancestor)))
   {
      if (_XmIntersectRect( &tearOff_rect, ancestor, &intersect_rect ))
      {
	 XtUnmapWidget(XtParent(ancestor));
	 RC_SetTearOffDirty(tearOff, True);
      }
      if ((shell = (ShellWidget)RC_PopupPosted(ancestor)) != NULL)
	 ancestor = shell->composite.children[0];
      else
	 break;
   }
   if (RC_TearOffDirty(tearOff))
      XFlush(XtDisplay(ancestor));
}

void
#ifdef _NO_PROTO
_XmRestoreExcludedTearOffToToplevelShell( wid, event )
        Widget wid;
	XEvent *event;
#else
_XmRestoreExcludedTearOffToToplevelShell(
        Widget wid, 
	XEvent *event )
#endif
{
   int i;
   Widget pane;

   for(i=0; i < _XmExcludedParentPane.num_panes; i++)
   {
      if ((pane = _XmExcludedParentPane.pane[i]) != NULL)
      {
	 /* Reset to NULL first so that _XmRestoreTearOffToToplevelShell()
	  * doesn't prematurely abort.
	  */
	 _XmExcludedParentPane.pane[i] = NULL;
	 _XmRestoreTearOffToToplevelShell(pane, event);
      }
      else
	 break;
   }
   _XmExcludedParentPane.num_panes = 0;
}

/*
 * The menupane was just posted, it's current parent is the original menushell.
 * Reparent the pane back to the tear off toplevel shell.
 */
void
#ifdef _NO_PROTO
_XmRestoreTearOffToToplevelShell( wid, event )
        Widget wid;
	XEvent *event;
#else
_XmRestoreTearOffToToplevelShell(
        Widget wid, 
	XEvent *event )
#endif
{
   XmRowColumnWidget rowcol = (XmRowColumnWidget)wid;
   XtGeometryResult answer;
   Dimension almostWidth, almostHeight;
   int i;

   for(i=0; i < _XmExcludedParentPane.num_panes; i++)
      if ((Widget)rowcol == _XmExcludedParentPane.pane[i])
	 return;

   if (RC_TornOff(rowcol) && !RC_TearOffActive(rowcol))
   {
      ShellWidget shell;

      /* Unmanage the toc before reparenting to preserve the focus item. */
      XtUnmanageChild(RC_TearOffControl(rowcol));

      /* In case we're dealing with a shared menushell, the rowcol is kept
       * managed.  We need to reforce the pane to be managed so that the
       * pane's geometry is recalculated.  This allows the tear off to be
       * forced larger by mwm so that the title is not clipped.  It's
       * done here to minimize the lag time/flashing.
       */
      XtUnmanageChild((Widget)rowcol);

      /* swap parents to the toplevel shell */
      shell = (ShellWidget)XtParent(rowcol);
      XtParent(rowcol) = RC_ParentShell(rowcol);
      RC_ParentShell(rowcol) = (Widget)shell;
      RC_SetTearOffActive(rowcol, TRUE);

      /* Sync up the server */
      XReparentWindow(XtDisplay(shell), XtWindow(rowcol),
         XtWindow(XtParent(rowcol)), 0, 0);

      XFlush(XtDisplay(shell));

      if (XtParent(rowcol)->core.background_pixmap != XtUnspecifiedPixmap)
      {
         XFreePixmap(XtDisplay(XtParent(rowcol)),
            XtParent(rowcol)->core.background_pixmap);
         XtParent(rowcol)->core.background_pixmap = XtUnspecifiedPixmap;
      }

      /* The menupost that reparented the pane back to the menushell has
       * wiped out the active_child.  We need to restore it.
       * Check this out if FocusPolicy == XmPOINTER!
       */
      rowcol->manager.active_child = _XmGetActiveItem((Widget)rowcol);

      _XmAddTearOffEventHandlers ((Widget) rowcol);

      /* Restore lastSelectToplevel as if the (torn) menu is posted */
      if (IsPulldown(rowcol))
	 rowcol->row_column.lastSelectToplevel =
	    rowcol->row_column.tear_off_lastSelectToplevel;
      else /* IsPopup */
	 RC_CascadeBtn(rowcol) =
	    rowcol->row_column.tear_off_lastSelectToplevel;

      CallTearOffMenuActivateCallback((Widget)rowcol, event, 
	 RESTORE_TEAR_OFF_TO_TOPLEVEL_SHELL);
      _XmCallRowColumnMapCallback((Widget)rowcol, event);

      /*
       * In case the rowcolumn's geometry has changed, make a resize
       * request to the top level shell so that it can changed.  All
       * geometry requests were handled through the menushell and the
       * top level shell was left unchanged.
       */
      answer = XtMakeResizeRequest (XtParent(rowcol), XtWidth(rowcol),
				    XtHeight(rowcol), &almostWidth,
				    &almostHeight);

      if (answer == XtGeometryAlmost)
	  answer = XtMakeResizeRequest (XtParent(rowcol), almostWidth,
					almostHeight, NULL, NULL);
	  
				    
      
      /* As in _XmTearOffInitiate(), To get Traversal: _XmGetManagedInfo()
       * to work correctly.
       */
      rowcol->core.mapped_when_managed = True;
      XtManageChild((Widget)rowcol);

      /* rehighlight the previous focus item */
      XmProcessTraversal(rowcol->row_column.tear_off_focus_item, 
	 XmTRAVERSE_CURRENT);
   }
}

void
#ifdef _NO_PROTO
_XmRestoreTearOffToMenuShell( wid, event )
        Widget wid;
	XEvent *event;
#else
_XmRestoreTearOffToMenuShell(
        Widget wid,
	XEvent *event )
#endif
{
   XmRowColumnWidget submenu = (XmRowColumnWidget) wid;
   XmMenuState mst = _XmGetMenuState((Widget)wid);

   if (RC_TornOff(submenu) && RC_TearOffActive(submenu))
   {
      ShellWidget shell;
      GC gc;
      XGCValues values;
      int i;
      Widget child;

      /* If the pane was previously obscured, it may require redrawing
       * before taking a pixmap snapshot.
       * Note: event could be NULL on right arrow browse through menubar back
       *   to this submenu.
       */
      if (RC_TearOffDirty(submenu) ||
	  (event && (event->type == ButtonPress) && 
	   (event->xbutton.time == mst->RC_ReplayInfo.time) &&
	   (mst->RC_ReplayInfo.toplevel_menu == (Widget)submenu)) ||
	  _XmFocusIsInShell((Widget)submenu))
      {
	 RC_SetTearOffDirty(submenu, False);
	 
	 /* First make sure that the previous active child is unhighlighted.
	  * In the tear off's inactive state, no children should be
	  * highlighted.
	  */
	 if ((child = submenu->manager.active_child) != NULL)
	 {
	    if (XtIsWidget(child))
	       (*(((XmPrimitiveClassRec *)XtClass(child))->
		  primitive_class.border_unhighlight))(child);
	    else
	       (*(((XmGadgetClassRec *)XtClass(child))->
		  gadget_class.border_unhighlight))(child);
	 }

	 /* Redraw the submenu and its gadgets */
	 if (XtClass(submenu)->core_class.expose)
	    (*(XtClass(submenu)->core_class.expose)) ((Widget)submenu, NULL, NULL);

	 /* Redraw the submenu's widgets */
	 for (i=0; i<submenu->composite.num_children; i++)
	 {
	    child = submenu->composite.children[i];
	    if (XtIsWidget(child))
	    {
	       if (XtClass(child)->core_class.expose)
		  (*(XtClass(child)->core_class.expose)) (child, event, NULL);
	    }
	 }
	 XFlush(XtDisplay(submenu));
      }

      shell =  (ShellWidget)XtParent(submenu);      /* this is a toplevel */

      /* Save away current focus item.  Then clear the focus path so that
       * the XmProcessTraversal() in _XmRestoreTOToTopLevelShell() re-
       * highlights the focus_item.
       */
      submenu->row_column.tear_off_focus_item = 
	 XmGetFocusWidget((Widget)submenu);
      _XmClearFocusPath((Widget) submenu);

      /* Get a pixmap holder first! */

      values.foreground = values.background = shell->core.background_pixel;
      values.graphics_exposures = False;
      values.subwindow_mode = IncludeInferiors;
      gc = XtGetGC((Widget) shell,
	 GCForeground | GCBackground | GCGraphicsExposures | GCSubwindowMode,
	 &values);

      /* Fix for CR #4855, use of default depth, DRand 6/4/92 */
      shell->core.background_pixmap = XCreatePixmap(XtDisplay(shell),
	 RootWindowOfScreen(XtScreen(shell)),
	 shell->core.width, shell->core.height,
         shell->core.depth);
      /* End of Fix #4855 */

      XCopyArea(XtDisplay(shell), XtWindow(submenu),
	 shell->core.background_pixmap, gc, 0, 0,
	 shell->core.width, shell->core.height,
	 0, 0);

      XtParent(submenu) = RC_ParentShell(submenu);
      RC_ParentShell(submenu) = (Widget)shell;
      RC_SetTearOffActive(submenu, False);

      submenu->core.mapped_when_managed = False;
      submenu->core.managed = False;

      /* Sync up the server */
      XSetWindowBackgroundPixmap(XtDisplay(shell), XtWindow(shell),
	 shell->core.background_pixmap);

      XReparentWindow(XtDisplay(shell), XtWindow(submenu),
	 XtWindow(XtParent(submenu)), XtX(submenu), XtY(submenu));

      XtManageChild(RC_TearOffControl(submenu));

      /* The traversal graph needs to be zeroed/freed when reparenting back
       * to a MenuShell.  This handles the case of shared/torn panes where
       * the pane moves from shell(context1) -> torn-shell -> shell(context2).
       * Shell(context2) receives the TravGraph from shell(context1).
       */
      if (submenu->row_column.postFromCount > 1)
         _XmResetTravGraph(submenu->core.parent);

      _XmCallRowColumnUnmapCallback((Widget)submenu, event);
      CallTearOffMenuDeactivateCallback((Widget)submenu, event,
	 RESTORE_TEAR_OFF_TO_MENUSHELL);
      RemoveTearOffEventHandlers ((Widget) submenu);
   }
}
