#pragma ident	"@(#)m1.2libs:Xm/TravAct.c	1.2"
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
#ifdef  REV_INFO
#ifndef lint
static char SCCSID[] = "OSF/Motif: @(#)TravAct.c	1.7 92/02/20";
#endif /* lint */
#endif /* REV_INFO */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "TraversalI.h"
#include "TravActI.h"
#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/VendorSEP.h>
#include <Xm/MenuShellP.h>
#include "RepTypeI.h"
#include <Xm/VirtKeysP.h>
#include <Xm/ScrolledWP.h>


#define EVENTS_EQ(ev1, ev2) \
  ((((ev1)->type == (ev2)->type) &&\
    ((ev1)->serial == (ev2)->serial) &&\
    ((ev1)->time == (ev2)->time) &&\
    ((ev1)->x == (ev2)->x) &&\
    ((ev1)->y == (ev2)->y)) ? TRUE : FALSE)


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean UpdatePointerData() ;
static void FlushPointerData() ;
static void DispatchGadgetInput() ;

#else

static Boolean UpdatePointerData( 
                        Widget w,
                        XEvent *event) ;
static void FlushPointerData( 
                        Widget w,
                        XEvent *event) ;
static void DispatchGadgetInput( 
                        XmGadget g,
                        XEvent *event,
                        Mask mask) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/*
 * The following functions are used by the widgets to query or modify one
 * of the display dependent global variabled used by traversal mechanism.
 */

typedef struct {
   Display * display;
   unsigned short flag;
} _XmBooleanEntry;


static _XmBooleanEntry * resetFocusFlagList = NULL;
static int resetFocusListSize = 0;

/*
 * Get the state of the 'ResettingFocus' flag, based upon the
 * display to which the widget is tied.
 */
Boolean 
#ifdef _NO_PROTO
_XmGetFocusResetFlag( w )
        Widget w ;
#else
_XmGetFocusResetFlag(
        Widget w )
#endif /* _NO_PROTO */
{
   return( (Boolean) _XmGetFocusFlag(w, XmFOCUS_RESET) );
}

/*
 * Set the state of the 'ResettingFocus' flag.
 */

void 
#ifdef _NO_PROTO
_XmSetFocusResetFlag( w, value )
        Widget w ;
        Boolean value ;
#else
_XmSetFocusResetFlag(
        Widget w,
#if NeedWidePrototypes
        int value )
#else
        Boolean value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   _XmSetFocusFlag(w, XmFOCUS_RESET, value);
}

unsigned short
#ifdef _NO_PROTO
_XmGetFocusFlag( w, mask )
	Widget w;
	unsigned int mask;
#else
_XmGetFocusFlag(Widget w, unsigned int mask)
#endif /* _NO_PROTO */
{
   register int i;

   for (i = 0; i < resetFocusListSize; i++)
   {
      if (resetFocusFlagList[i].display == XtDisplay(w))
         return ((unsigned short)resetFocusFlagList[i].flag & mask);
   }

   return((unsigned short) NULL );
}

void 
#ifdef _NO_PROTO
_XmSetFocusFlag( w, mask, value )
        Widget w ;
	unsigned int mask ;
        Boolean value ;
#else
_XmSetFocusFlag(
        Widget w,
	unsigned int mask,
#if NeedWidePrototypes
        int value )
#else
        Boolean value )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   register int i;

   /* See if an entry already exists */
   for (i = 0; i < resetFocusListSize; i++)
   {
      if (resetFocusFlagList[i].display == XtDisplay(w))
      {
	 if (value)
            resetFocusFlagList[i].flag |= mask;
	 else
            resetFocusFlagList[i].flag &= ~mask;
         return;
      }
   }

   /* Allocate a new entry */
   resetFocusListSize++;
   resetFocusFlagList = (_XmBooleanEntry *) XtRealloc(
                            (char *) resetFocusFlagList,
                               (sizeof(_XmBooleanEntry) * resetFocusListSize));
   resetFocusFlagList[i].display = XtDisplay(w);
   resetFocusFlagList[i].flag = 0;
   if (value)
      resetFocusFlagList[i].flag |= mask;
}

static Boolean 
#ifdef _NO_PROTO
UpdatePointerData( w, event )
        Widget w ;
        XEvent *event ;
#else
UpdatePointerData(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmFocusData		focusData;

    if ((focusData = _XmGetFocusData( w)) != NULL)
      {
	  XCrossingEvent *lastEvent = &(focusData->lastCrossingEvent);

	  focusData->needToFlush = TRUE;

	  if (!EVENTS_EQ(lastEvent,((XCrossingEvent *) event)))
	    {
		focusData->old_pointer_item = focusData->pointer_item ;
		focusData->pointer_item = w;
		focusData->lastCrossingEvent = *(XCrossingEvent *) event;
		return TRUE;
	    }
      }
    return FALSE;
}

static void 
#ifdef _NO_PROTO
FlushPointerData( w, event )
        Widget w ;
        XEvent *event ;
#else
FlushPointerData(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmFocusData		focusData = _XmGetFocusData( w);

    if (focusData && focusData->needToFlush)
      {
	  XCrossingEvent	lastEvent;

	  lastEvent = focusData->lastCrossingEvent;

	  focusData->needToFlush = FALSE;
	  /* 
	   * We are munging data into the event to fake out the focus
	   * code when Mwm is trying to catch up with the pointer.
	   * This event that we are munging might already have been
	   * munged by XmDispatchGadgetInput from a motion event to a
	   * crossing event !!!!!
	   */
	  
	  lastEvent.serial = event->xcrossing.serial;
	  lastEvent.time = event->xcrossing.time;
	  lastEvent.focus = True;
	  XtDispatchEvent((XEvent *) &lastEvent);
      }
}

/************************************************************************
 *
 *  _XmTrackShellFocus
 *
 *  This handler is added by ShellExt initialize to the front of the
 * queue
 *     
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmTrackShellFocus( widget, client_data, event, dontSwallow )
        Widget widget ;
        XtPointer client_data ;
        XEvent *event ;
        Boolean *dontSwallow ;
#else
_XmTrackShellFocus(
        Widget widget,
        XtPointer client_data,
        XEvent *event,
        Boolean *dontSwallow )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject	ve = (XmVendorShellExtObject) client_data ;
    XmFocusData			focusData;
    XmGeneology			oldFocalPoint;
    XmGeneology			newFocalPoint;

    if (widget->core.being_destroyed)
      {
	  *dontSwallow = False;
	  return;
      }
    if(    !(focusData = ve->vendor.focus_data)    )
      {
	return ;
      }
    oldFocalPoint = newFocalPoint = focusData->focalPoint;

    switch( event->type ) {
      case EnterNotify:
      case LeaveNotify:
	
	/*
	 * If operating in a focus driven model, then enter and
	 * leave events do not affect the keyboard focus.
	 */
	if ((event->xcrossing.detail != NotifyInferior)
	    &&	(event->xcrossing.focus))
	  {	      
	      switch (oldFocalPoint)
		{
		  case XmUnrelated:
		    if (event->type == EnterNotify)
		      newFocalPoint = XmMyAncestor;
  	            break;
		  case XmMyAncestor:
		    if (event->type == LeaveNotify)
		      newFocalPoint = XmUnrelated;
		    break;
		  case XmMyDescendant:
		  case XmMyCousin:
		  case XmMySelf:
		  default:
		    break;
		}	
	  }
	break;
      case FocusIn:
	switch (event->xfocus.detail)
	  {
	    case NotifyNonlinear:
	    case NotifyAncestor:
	    case NotifyInferior:
	      newFocalPoint = XmMySelf;
	      break;
	    case NotifyNonlinearVirtual:
	    case NotifyVirtual:
	      newFocalPoint = XmMyDescendant;
	      break;
	    case NotifyPointer:
	      newFocalPoint = XmMyAncestor;
	      break;
	  }
	break;
      case FocusOut:
	switch (event->xfocus.detail)
	  {
	    case NotifyPointer:
	    case NotifyNonlinear:
	    case NotifyAncestor:
	    case NotifyNonlinearVirtual:
	    case NotifyVirtual:
	      newFocalPoint = XmUnrelated;
	      break;
	    case NotifyInferior:
	      return;
	  }
	break;
    }
    if(    newFocalPoint == XmUnrelated    )
      {
	focusData->old_focus_item = NULL ;

	if(    focusData->trav_graph.num_alloc    )
	  {
	    /* Free traversal graph, since focus is leaving hierarchy.
	     */
	    _XmFreeTravGraph( &(focusData->trav_graph)) ;
	  }
      }
    if(    (focusData->focus_policy == XmEXPLICIT)
       &&  (oldFocalPoint != newFocalPoint)
       &&  focusData->focus_item    )
      {
	if(    oldFocalPoint == XmUnrelated    )
	  {
	    _XmCallFocusMoved( NULL, focusData->focus_item, event) ;
	  }
	else
	  {
	    if(    newFocalPoint == XmUnrelated    )
	      {
		_XmCallFocusMoved( focusData->focus_item, NULL, event) ;
	      }
	  }
      }
    focusData->focalPoint = newFocalPoint;
}

/************************************************************************
 *
 *  Enter & Leave
 *      Enter and leave event processing routines.
 *
 ************************************************************************/

void 
#ifdef _NO_PROTO
_XmPrimitiveEnter( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveEnter(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
  if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
      if(    event->xcrossing.focus    )
        {   
	  _XmCallFocusMoved( XtParent( wid), wid, event) ;

	  _XmWidgetFocusChange( wid, XmENTER) ;
	}
      UpdatePointerData( wid, event) ;
    }
}

void 
#ifdef _NO_PROTO
_XmPrimitiveLeave( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveLeave(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
  if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
      if(    event->xcrossing.focus    )
        {   
	  _XmCallFocusMoved( wid, XtParent( wid), event) ;

	  _XmWidgetFocusChange( wid, XmLEAVE) ;
	}
    }	
}

/************************************************************************
 *
 *  Focus In & Out
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmPrimitiveFocusInInternal( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveFocusInInternal(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
    if(    !(event->xfocus.send_event)
        || _XmGetFocusFlag( wid, XmFOCUS_IGNORE)    )
    {   
        return ;
        } 
    if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
        /* Maybe Mwm trying to catch up  with us
        */
        if(    XtIsShell( XtParent( wid))    )
        {   
            FlushPointerData( wid, event) ;
            }
        }
    else 
    {   /* We should only be recieving the focus from a traversal request.
        */
        if(    !_XmGetActiveTabGroup( wid)    )
        {   
            _XmMgrTraversal( _XmFindTopMostShell( wid),
			                           XmTRAVERSE_NEXT_TAB_GROUP) ;
            }
        else
        {   
            _XmWidgetFocusChange( wid, XmFOCUS_IN) ;
            }
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmPrimitiveFocusOut( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveFocusOut(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
  if(    event->xfocus.send_event
     &&  !(wid->core.being_destroyed)
     &&  (_XmGetFocusPolicy( wid) == XmEXPLICIT)    )
    {   
      _XmWidgetFocusChange( wid, XmFOCUS_OUT) ;
    }
}

void 
#ifdef _NO_PROTO
_XmPrimitiveFocusIn( pw, event, params, num_params )
        Widget pw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveFocusIn(
        Widget pw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    _XmPrimitiveFocusInInternal( pw, event, params, num_params);

    return ;
    }

/************************************************************************
 *
 *  _XmEnterGadget
 *     This function processes enter window conditions occuring in a gadget
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmEnterGadget( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmEnterGadget(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
  if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
      XmFocusData focusData = _XmGetFocusData( wid) ;

      /* we may be getting called as a result of Mwm catching up
       * with the pointer and setting input focus to the shell
       * which then gets forwarded to us
       */
      if(    focusData && (focusData->focalPoint != XmUnrelated)    )
        {   
	  _XmCallFocusMoved( XtParent( wid), wid, event) ;

	  _XmWidgetFocusChange( wid, XmENTER) ;
        }
    }
}

/************************************************************************
 *
 *  DispatchGadgetInput
 *	This routine is used instead of _XmDispatchGadgetInput due to
 *	the fact that it needs to dispatch to unmanaged gadgets
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DispatchGadgetInput( g, event, mask )
        XmGadget g ;
        XEvent *event ;
        Mask mask ;
#else
DispatchGadgetInput(
        XmGadget g,
        XEvent *event,
        Mask mask )
#endif /* _NO_PROTO */
{
   if (g->gadget.event_mask & mask && XtIsSensitive (g))
     (*(((XmGadgetClass) (g->object.widget_class))->
	gadget_class.input_dispatch)) ((Widget) g, event, mask);
}

/************************************************************************
 *
 *  _XmLeaveGadget
 *     This function processes leave window conditions occuring in a gadget
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmLeaveGadget( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmLeaveGadget(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
    if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
        _XmCallFocusMoved( wid, XtParent( wid), event) ;

	_XmWidgetFocusChange( wid, XmLEAVE) ;
        }
    return ;
    }

/************************************************************************
 *
 *  _XmFocusInGadget
 *     This function processes focusIn conditions occuring in a gadget
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmFocusInGadget( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmFocusInGadget(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    if(    _XmGetFocusPolicy( wid) == XmEXPLICIT    )
    {   
        _XmWidgetFocusChange( wid, XmFOCUS_IN) ;
        } 
    return ;
    }


/************************************************************************
 *
 *  _XmFocusOutGadget
 *     This function processes FocusOut conditions occuring in a gadget
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmFocusOutGadget( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmFocusOutGadget(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    if(    _XmGetFocusPolicy( wid) == XmEXPLICIT    )
    {   
        _XmWidgetFocusChange( wid, XmFOCUS_OUT) ;
        } 
    return ;
    }

/************************************************************************
 *
 *  Enter, FocusIn and Leave Window procs
 *
 *     These two procedures handle traversal activation and deactivation
 *     for manager widgets. They are invoked directly throught the
 *     the action table of a widget.
 *
 ************************************************************************/

/************************************************************************
 *
 *  _XmManagerEnter
 *     This function handles both focusIn and Enter. Don't ask me why
 *     :-( 
 *
 ************************************************************************/
void 
#ifdef _NO_PROTO
_XmManagerEnter( wid, event_in, params, num_params )
        Widget wid ;
        XEvent *event_in ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerEnter(
        Widget wid,
        XEvent *event_in,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmManagerWidget   mw = (XmManagerWidget) wid ;
    XCrossingEvent    * event = (XCrossingEvent *) event_in ;

    if (_XmGetFocusPolicy( (Widget) mw) == XmPOINTER)
      {
	  if (UpdatePointerData((Widget) mw, event_in) && event->focus)
	    {
		Widget	old;
		
		if (event->detail == NotifyInferior)
                {   
                    old = XtWindowToWidget(event->display, event->subwindow);
                    } 
		else
		{   old = XtParent(mw);
                    } 
		_XmCallFocusMoved( old, (Widget) mw, (XEvent *) event);
		_XmWidgetFocusChange( (Widget) mw, XmENTER) ;
	    }
      }
}

void 
#ifdef _NO_PROTO
_XmManagerLeave( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerLeave(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    /*
     *  this code is inefficient since it is called twice for each
     * internal move in  the hierarchy |||
     */
    if (event->type == LeaveNotify)
      {
	  if (_XmGetFocusPolicy( wid) == XmPOINTER)
	    {
		Widget		new_wid;
		
		if (event->xcrossing.detail == NotifyInferior)
		  new_wid = XtWindowToWidget(event->xcrossing.display, 
					 event->xcrossing.subwindow);
		else 
		  {
		      new_wid = XtParent( wid);
		  }
		if (UpdatePointerData( wid, event) && event->xcrossing.focus)
		  {
		    _XmCallFocusMoved( wid, new_wid, event);

		    _XmWidgetFocusChange( wid, XmLEAVE) ;
		  }
	    }
      }
}

void 
#ifdef _NO_PROTO
_XmManagerFocusInInternal( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerFocusInInternal(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
            Widget		child ;
    /*
    * Managers ignore all focus events which have been generated by the
    * window system; only those sent to us by a window manager or the
    * Xtk focus code is accepted.
    * Bail out if the focus policy is not set to explicit
    */
    if(    !(event->xfocus.send_event)
        || _XmGetFocusFlag( wid, XmFOCUS_RESET | XmFOCUS_IGNORE)    )
    {   
        return ;
        } 
    if(    _XmGetFocusPolicy( wid) == XmPOINTER    )
    {   
        FlushPointerData( wid, event) ;
        } 
    else
    {   /* if the heirarchy doesn't have an active tab group give it one
        */
        if(    !_XmGetActiveTabGroup( wid)    )
        {   
	    /* Bootstrap. */
            _XmMgrTraversal( _XmFindTopMostShell( wid),
			                           XmTRAVERSE_NEXT_TAB_GROUP) ;
            } 
        else
        {   /* If focus went to a gadget, then force it to highlight
            */
            if(    (child = ((XmManagerWidget) wid)->manager.active_child)
                && XmIsGadget( child)    )
            {   
                DispatchGadgetInput( (XmGadget) child, event,
                                                            XmFOCUS_IN_EVENT) ;
                }
	    else
	      {
		_XmWidgetFocusChange( wid, XmFOCUS_IN) ;
	      }
            }
        }
    return ;
    }

/*
 * Non-menu widgets use this entry point, so that they will ignore focus
 * events during menu activities.
 */
void 
#ifdef _NO_PROTO
_XmManagerFocusIn( mw, event, params, num_params )
        Widget mw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerFocusIn(
        Widget mw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
   _XmManagerFocusInInternal(mw, event, params, num_params);
}


/*
 * If the manager widget received a FocusOut while it is processing its
 * FocusIn event, then it knows that the focus has been successfully moved
 * to one of its children.  However, if no FocusOut is received, then the
 * manager widget must manually force the child to take the focus.
 */
void 
#ifdef _NO_PROTO
_XmManagerFocusOut( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerFocusOut(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{   
            Widget child ;

    if(    !event->xfocus.send_event    
        || _XmGetFocusFlag( wid, XmFOCUS_IGNORE)    )
    {   return ;
        } 
    if(    _XmGetFocusPolicy( wid) == XmEXPLICIT    )
    {   
        /* If focus is in a gadget, then force it to unhighlight
        */
        if(    (child = ((XmManagerWidget) wid)->manager.active_child)
            && XmIsGadget( child)    )
        {   
            DispatchGadgetInput( (XmGadget) child, event, XmFOCUS_OUT_EVENT) ;
            }
	else
	  {
	    _XmWidgetFocusChange( wid, XmFOCUS_OUT) ;
	  }
        }
    return ;
    }

void 
#ifdef _NO_PROTO
_XmManagerUnmap( mw, event, params, num_params )
        Widget mw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmManagerUnmap(
        Widget mw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  /* This functionality is bogus, since a good implementation
   * requires more code (hooks for mapping of widgets) than its
   * worth.  To move focus away from a widget when it is unmapped
   * implies the ability to recover from the case when the last
   * traversable widget in a hierarchy is unmapped and then re-mapped.
   * Since we don't have the hooks in place for the mapping of these
   * widgets, and since the old code only worked some of the time,
   * and since it is arguable that the focus should never be
   * changed in response to a widget being unmapped, we should choose
   * to do NO traversal in response to the unmapping of a widget.
   * However, historical precedent again defeats good design.
   */
  _XmValidateFocus( mw) ;
}

void 
#ifdef _NO_PROTO
_XmPrimitiveUnmap( pw, event, params, num_params )
        Widget pw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
_XmPrimitiveUnmap(
        Widget pw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
  _XmValidateFocus( pw) ;
}


