#pragma ident	"@(#)m1.2libs:Xm/Traversal.c	1.2"
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
static char SCCSID[] = "OSF/Motif: @(#)Traversal.c	4.16 92/03/02";
#endif /* lint */
#endif /* REV_INFO */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "TraversalI.h"
#include "TravActI.h"
#include <Xm/GadgetP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/BaseClassP.h>
#include <Xm/VendorSEP.h>
#include <Xm/MenuShellP.h>
#include "RepTypeI.h"
#include "CallbackI.h"
#include <Xm/VirtKeysP.h>
#include <Xm/ScrolledWP.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Widget FindFirstManaged() ;
static Boolean _XmCallTraverseObscured() ;
static Boolean _XmIsTraversable() ;
static Widget FindFirstFocus() ;

#else

static Widget FindFirstManaged( 
                        Widget wid) ;
static Boolean _XmCallTraverseObscured( 
                        Widget new_focus,
                        XmTraversalDirection dir) ;
static Boolean _XmIsTraversable( 
                        Widget wid,
#if NeedWidePrototypes
                        int require_in_view) ;
#else
                        Boolean require_in_view) ;
#endif /* NeedWidePrototypes */
static Widget FindFirstFocus( 
                        Widget wid) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


XmFocusData 
#ifdef _NO_PROTO
_XmCreateFocusData()
#else
_XmCreateFocusData( void )
#endif /* _NO_PROTO */
{
  return (XmFocusData) XtCalloc(1, sizeof(XmFocusDataRec)) ;
}

void 
#ifdef _NO_PROTO
_XmDestroyFocusData( focusData )
        XmFocusData focusData ;
#else
_XmDestroyFocusData(
        XmFocusData focusData )
#endif /* _NO_PROTO */
{
  _XmFreeTravGraph( &(focusData->trav_graph)) ;
  XtFree((char *) focusData->trav_graph.excl_tab_list) ;
  XtFree((char *) focusData) ;
}

void 
#ifdef _NO_PROTO
_XmSetActiveTabGroup( focusData, tabGroup )
        XmFocusData focusData ;
        Widget tabGroup ;
#else
_XmSetActiveTabGroup(
        XmFocusData focusData,
        Widget tabGroup )
#endif /* _NO_PROTO */
{
    focusData->active_tab_group = tabGroup;
}

Widget 
#ifdef _NO_PROTO
_XmGetActiveItem( w )
        Widget w ;
#else
_XmGetActiveItem(
        Widget w )
#endif /* _NO_PROTO */
{
  return XmGetFocusWidget( w) ;
}

void 
#ifdef _NO_PROTO
_XmNavigInitialize( request, new_wid, args, num_args )
        Widget request ;
        Widget new_wid ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmNavigInitialize(
        Widget request,
        Widget new_wid,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{   
  XmFocusData focusData ;

  if(    (focusData = _XmGetFocusData( new_wid)) != NULL    )
    {
      XmNavigationType navType = _XmGetNavigationType( new_wid) ;
      
      if(    navType == XmEXCLUSIVE_TAB_GROUP    )
	{
	  ++(focusData->trav_graph.exclusive) ;
	  _XmTabListAdd( &(focusData->trav_graph), new_wid) ;
	}
      else
	{
	  if(    navType == XmSTICKY_TAB_GROUP    )
	    {
	      _XmTabListAdd( &(focusData->trav_graph), new_wid) ;
	    }
	}
      if(    focusData->trav_graph.num_entries
	 &&  _XmGetNavigability( new_wid)    )
	{
	  /* If the graph exists, add the new navigable widget.
	   */
	  _XmTravGraphAdd( &(focusData->trav_graph), new_wid) ;
	}
    }
  /* If the traversal graph doesn't exist, do nothing, since the
   * new widget will be picked-up when the graph is needed and created.
   */
  return ;
}

Boolean 
#ifdef _NO_PROTO
_XmNavigSetValues( current, request, new_wid, args, num_args )
        Widget current ;
        Widget request ;
        Widget new_wid ;
        ArgList args ;
        Cardinal *num_args ;
#else
_XmNavigSetValues(
        Widget current,
        Widget request,
        Widget new_wid,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
  /* This routine is called from the SetValues method of Manager,
   * Primitive, and Gadget to keep the traversal data structures
   * up-to-date in regards to changes in the traversability of widgets.
   *
   * There are three purposes for this routine:
   *
   *   1:  Update the traversal graph in response to changes in
   *       a widget's resources such that the widget is newly
   *       eligible to receive the traversal focus.
   *
   *   2:  Update the focus data according to changes in
   *       the Motif 1.0 "exclusive tab group" behavior.
   *
   *   3:  If the new widget of the SetValues call is the focus
   *       widget and it becomes ineligible to have the focus,
   *       then find an alternative to receive the focus (or
   *       reset the focus for the hierarchy to the bootstrap
   *       condition).
   */

  XmFocusData focusData ;

  if(    (focusData = _XmGetFocusData( new_wid)) != NULL    )
    {
      XmTravGraph graph = &(focusData->trav_graph) ;
      XmNavigationType newNavType = _XmGetNavigationType( new_wid) ;
      XmNavigationType curNavType = _XmGetNavigationType( current) ;
      Boolean ChangeInExclusive = FALSE ;

      if(    curNavType != newNavType    )
	{
	  if(    (curNavType == XmEXCLUSIVE_TAB_GROUP)
	     ||  (newNavType == XmEXCLUSIVE_TAB_GROUP)    )
	    {
	      /* This widget was "exclusive", now it is not (or vice-versa).
	       * Update the value of the focus data "exclusive" field.
	       */
	      ChangeInExclusive = TRUE ;

	      if(    newNavType == XmEXCLUSIVE_TAB_GROUP    )
		{
		  ++(graph->exclusive) ;
		}
	      else
		{
		  --(graph->exclusive) ;
		}
	    }
	  if(    (newNavType == XmEXCLUSIVE_TAB_GROUP)
	     ||  (newNavType == XmSTICKY_TAB_GROUP)    )
	    {
	      if(    (curNavType != XmEXCLUSIVE_TAB_GROUP)
		 &&  (curNavType != XmSTICKY_TAB_GROUP)    )
		{
		  _XmTabListAdd( graph, new_wid) ;
		}
	    }
	  else
	    {
	      if(    (curNavType == XmEXCLUSIVE_TAB_GROUP)
		 ||  (curNavType == XmSTICKY_TAB_GROUP)    )
		{
		  _XmTabListDelete( graph, new_wid) ;
		}
	    }
	}
      if(    XtIsRealized( new_wid)
	 &&  (focusData->focus_policy == XmEXPLICIT)    )
	{
	  if(    graph->num_entries    )
	    {
	      if(    ChangeInExclusive    )
		{
		  /* Since widget has changed to/from exlusive tab group
		   * behavior, need to re-make the traversal graph (as needed).
		   */
		  _XmFreeTravGraph( graph) ;
		}
	      else
		{
		  XmNavigability cur_nav = _XmGetNavigability( current) ;
		  XmNavigability new_nav = _XmGetNavigability( new_wid) ;

		  if(    !cur_nav  &&  new_nav    )
		    {
		      /* Newly navigable widget; add it to the
		       * traversal graph.
		       */
		      _XmTravGraphAdd( graph, new_wid) ;
		    }
		  else
		    {
		      if(    cur_nav != new_nav    )
			{
			  /* Navigability changed; need to re-create the
			   * graph the next time it is needed.
			   */
			  _XmFreeTravGraph( graph) ;
			}
		    }
		}
	    }
	  if(    !(focusData->focus_item)    )
	    {
	      Widget shell ;

	      if(    XmIsTraversable( new_wid)
		 &&  (shell = _XmFindTopMostShell( new_wid))
		 &&  _XmFocusIsInShell( shell)
		 &&  (focusData->focalPoint != XmMySelf)    )
		{
		  /* Hierarchy currently has no focus, and this widget is
		   * now traversable, so bootstrap the focus for the hierarchy.
		   * Avoid initializing traversal if the shell has the
		   * focus (focalPoint == XmMySelf), rather than a
		   * descendant of the shell.  This makes the VTS work.
		   */
		  _XmMgrTraversal( shell, XmTRAVERSE_CURRENT) ;
		}
	    }
	  else
	    {
	      if(    (focusData->focus_item == new_wid)
		 &&  !_XmIsTraversable( new_wid, TRUE)    )
		{
		  /* The new_wid now has the focus and is no longer
		   * traversable, so traverse away from it to the
		   * next traversable item.
		   */
		  Widget new_focus = _XmTraverseAway( graph, new_wid,
                                    (focusData->active_tab_group != new_wid)) ;
		  if(    !new_focus    )
		    {
		      /* Could not find another widget eligible to take
		       * the focus, so use any widget to re-initialize/clear
		       * the focus in the widget hierarchy.
		       */
		      new_focus = new_wid ;
		    }
		  _XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT) ;

		  if(    !XtIsSensitive( new_wid)    )
		    {
		      /* Since widget has become insensitive, it did not
		       * receive the focus-out event.  Call the focus
		       * change method directly.
		       */
		      _XmWidgetFocusChange( new_wid, XmFOCUS_OUT) ;
		    }
		  return TRUE ;
		}
	    }
	}
    }
  return FALSE ;
}

void 
#ifdef _NO_PROTO
_XmNavigChangeManaged( wid )
        Widget wid ;
#else
_XmNavigChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{   
  /* This routine must be called from the ChangeManaged method of
   * all composite widgets that may have traversable children.
   * This routine checks to see if the focus widget is traversable;
   * if it is not, then an alternative traversable widget is found
   * or the focus for the hierarchy is reset to the bootstrap condition.
   *
   * This routine also detects the condition for which there is no
   * focus widget in the hierarchy and a newly managed widget is
   * now eligible to have the focus; the focus is then initialized.
   */
  XmFocusData focus_data ;

  if(    XtIsRealized( wid)
     &&  (focus_data = _XmGetFocusData( wid))
     &&  (focus_data->focus_policy == XmEXPLICIT)    )
    {
      if(    focus_data->focus_item == NULL    )
	{
	  Widget firstManaged ;

	  if(    XtIsShell( wid)    )
	    {
	      if(    focus_data->first_focus == NULL    )
		{
		  focus_data->first_focus = FindFirstFocus( wid) ;
		}
	      if(    (firstManaged = FindFirstManaged( wid)) != NULL    )
		{
		  /* Set bootstrap trigger for hierarchy that
		   * has no current focus.
		   */
		  XtSetKeyboardFocus( wid, firstManaged) ;
		}
	    }
	}
      else
	{
	  /* If the focus widget is being destroyed, do nothing for now.
	   * We need to wait until _XmNavigDestroy is called to initiate
	   * the focus change; if we don't defer selection of the focus
	   * widget, the Intrinsics-generated focus-out event for the
	   * focus widget will go to the newly-selected focus widget
	   * (instead of the widget being destroyed, as intended).
	   */
	  if(    !(focus_data->focus_item->core.being_destroyed)
	     &&  !_XmIsTraversable( focus_data->focus_item, TRUE)    )
	    {
	      Widget new_focus = _XmTraverseAway( &(focus_data->trav_graph),
                                     focus_data->focus_item,
		                          (focus_data->active_tab_group
		                                  != focus_data->focus_item)) ;
	      if(    !new_focus    )
		{
		  new_focus = focus_data->focus_item ;
		}
	      _XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT) ;
	    }
	} 
    }
  return ;
}

static Widget
#ifdef _NO_PROTO
FindFirstManaged( wid)
	Widget wid ;
#else
FindFirstManaged(
	Widget wid)
#endif /* _NO_PROTO */
{
  if(    XtIsShell( wid)    )
    {
      unsigned i = 0 ;

      while(    i < ((CompositeWidget) wid)->composite.num_children    )
	{
	  if(    XtIsManaged( ((CompositeWidget) wid)
				                  ->composite.children[i])    )
	    {
	      return ((CompositeWidget) wid)->composite.children[i] ;
	    }
	  ++i ;
	}
    }
  return NULL ;
}

void
#ifdef _NO_PROTO
_XmNavigResize( wid)
	Widget wid ;
#else
_XmNavigResize(
	Widget wid)
#endif /* _NO_PROTO */
{
  /* This routine must be called by all composites with (potentially)
   * traversable children.  This is generally handled for all managers
   * in the resize wrapper routines.
   *
   * This routine makes sure that the focus widget is always in view,
   * either by invoking the XmNtraverseObscurredCallback mechansism
   * of Scrolled Window or by finding an alternative focus widget.
   */
  XmFocusData focus_data ;

  if(    XtIsRealized( wid)  &&  !XtIsShell( wid)
     &&  (focus_data = _XmGetFocusData( wid))    )
    {
      /* If the focus item is being destroyed, do nothing, since this
       * will be handled more appropriately by _XmNavigDestroy().
       */
      if(    (focus_data->focus_policy == XmEXPLICIT)
	 &&  (    !(focus_data->focus_item)
	      ||  !((focus_data->focus_item)->core.being_destroyed))    )
	{
	  if(    !(focus_data->focus_item)    )
	    {
	      /* Hierarchy has no focus widget; re-initialize/clear the
	       * focus, but only if the parent is a managed shell (to
	       * avoid premature initialization during XtRealizeWidget).
	       */
	      Widget parent = XtParent( wid) ;
	      Widget firstManaged ;

	      if(    parent && XtIsShell( parent)
		 &&  (firstManaged = FindFirstManaged( parent))    )
		{
		  /* Set bootstrap trigger for hierarchy that
		   * has no current focus.
		   */
		  XtSetKeyboardFocus( wid, firstManaged) ;
		}
	    }
	  else
	    {
	      if(    !_XmIsTraversable( focus_data->focus_item, TRUE)    )
		{
		  /* Widget is not traversable, either because it is not
		   * viewable or some other reason.  Test again, this
		   * time allowing for obscured traversal.
		   *
		   * If it is not traversable regardless of the
		   * XmNtraverseObscuredCallback, or traversal to the
		   * obscured widget fails for some other reason, traverse
		   * away from the non-traversable widget.
		   */
		  if(    !_XmIsTraversable( focus_data->focus_item, FALSE)
		     ||  !_XmMgrTraversal( focus_data->focus_item,
			                          XmTRAVERSE_CURRENT)    )
		    {
		      Widget new_focus = _XmTraverseAway(
			     &(focus_data->trav_graph), focus_data->focus_item,
                                (focus_data->active_tab_group
				                  != focus_data->focus_item)) ;
		      if(    !new_focus    )
			{
			  new_focus = focus_data->focus_item ;
			}
		      _XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT) ;
		    }
		}
	    }
	}
    }
}

void 
#ifdef _NO_PROTO
_XmValidateFocus( wid )
        Widget wid ;
#else
_XmValidateFocus(
        Widget wid )
#endif /* _NO_PROTO */
{
  XmFocusData focus_data = _XmGetFocusData( wid) ;

  if(    focus_data
     &&  (focus_data->focus_policy == XmEXPLICIT)
     &&  (focus_data->focus_item != NULL)
     &&  !_XmIsTraversable( focus_data->focus_item, TRUE)    )
    {
      Widget new_focus = _XmTraverseAway( &(focus_data->trav_graph),
		                 focus_data->focus_item,
                    (focus_data->active_tab_group != focus_data->focus_item)) ;
      if(    !new_focus    )
	{
	  new_focus = wid ;
	}
      _XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT) ;
    }
}

void 
#ifdef _NO_PROTO
_XmNavigDestroy( wid )
        Widget wid ;
#else
_XmNavigDestroy(
        Widget wid )
#endif /* _NO_PROTO */
{   
  /* This routine is used to keep the traversal data up-to-date with
   * regards to widgets which are being destroyed.  It must be called
   * by all composites that might have traversable children.  The
   * DeleteChild method for Manager calls this routine, so its
   * subclasses can explicitly chain to its superclasses DeleteChild
   * method or call this routine directly.
   *
   * In addition to finding a new focus widget if it is being
   * destroyed, this routine must make sure that there are no
   * stale pointers to the widget being destroyed in any of its
   * data structures.
   */
  XmFocusData focusData = _XmGetFocusData( wid) ;

  if(    focusData    )
    {
      XmTravGraph trav_list = &(focusData->trav_graph) ;
      XmNavigationType navType = _XmGetNavigationType( wid) ;

      if(    wid == focusData->first_focus    )
	{
	  focusData->first_focus = NULL ;
	}
      if(    navType == XmEXCLUSIVE_TAB_GROUP    )
	{
	  --(trav_list->exclusive) ;
	  _XmTabListDelete( trav_list, wid) ;
	}
      else
	{
	  if(    navType == XmSTICKY_TAB_GROUP    )
	    {
	      _XmTabListDelete( trav_list, wid) ;
	    }
	}
      if(    focusData->focus_item == wid    )
	{
	  /* The focus widget for this hierarhcy is being destroyed.
	   * Traverse away if in explicit mode, or just clear the
	   * focus item field.
	   */
	  Widget new_focus ;

	  if(    (focusData->focus_policy != XmEXPLICIT)
	     ||  (    !(new_focus = _XmTraverseAway( trav_list,
				       focusData->focus_item,
                                         (focusData->active_tab_group != wid)))
		  &&  !(new_focus = _XmFindTopMostShell( wid)))
	     ||  !_XmMgrTraversal( new_focus, XmTRAVERSE_CURRENT)    )
	    {
	      focusData->focus_item = NULL ;
	    }
	}
      if(    focusData->trav_graph.num_entries    )
	{
	  _XmTravGraphRemove( trav_list, wid) ;
	}
      if(    focusData->active_tab_group == wid    )
	{
	  focusData->active_tab_group = NULL ;
	}
      if(    focusData->old_focus_item == wid    )
	{
	  focusData->old_focus_item = NULL ;
	}
      if(    focusData->pointer_item == wid    )
	{
	  focusData->pointer_item = NULL ;
	}
    }
  return ;
}

Boolean
#ifdef _NO_PROTO
_XmCallFocusMoved( old, new_wid, event )
        Widget old ;
        Widget new_wid ;
        XEvent *event ;
#else
_XmCallFocusMoved(
        Widget old,
        Widget new_wid,
        XEvent *event )
#endif /* _NO_PROTO */
{
        Widget w ;
        Widget topShell ;
        XtCallbackList callbacks ;
        Boolean contin = TRUE ;
    
    if (old) 
      w = old;
    else /* if (new_wid) -- if there's no w assignment we're in big trouble! */
      w = new_wid;
      
    topShell 	= (Widget) _XmFindTopMostShell(w);

    /*
     * make sure it's a shell that has a vendorExt object
     */
    if (XmIsVendorShell(topShell))
      {
	  XmWidgetExtData		extData;
	  XmVendorShellExtObject	vendorExt;

	  extData	= _XmGetWidgetExtData(topShell, XmSHELL_EXTENSION);

	  if ((vendorExt = (XmVendorShellExtObject) extData->widget) != NULL)
	    {
		if ((callbacks = vendorExt->vendor.focus_moved_callback) != NULL)
		  {
		      XmFocusMovedCallbackStruct	callData;
		      
		      callData.event		= event;
		      callData.cont		= True;
		      callData.old_focus	= old;
		      callData.new_focus	= new_wid;
		      callData.focus_policy   = vendorExt->vendor.focus_policy;

                      _XmCallCallbackList((Widget) vendorExt, callbacks,
                                                        (XtPointer) &callData);
                      contin = callData.cont ;
		  }
	    }
      }
    return( contin) ;
    }

Boolean 
#ifdef _NO_PROTO
_XmMgrTraversal( wid, direction )
        Widget wid ;
        XmTraversalDirection direction ;
#else
_XmMgrTraversal(
        Widget wid,
        XmTraversalDirection direction)
#endif /* _NO_PROTO */
{
  /* This routine is the workhorse for all traversal activities.
   */
  Widget top_shell ;
  Widget old_focus ;
  Widget new_focus ;
  Widget new_active_tab ;
  XmFocusData focus_data;
  XmTravGraph trav_list ;
  Boolean rtnVal = FALSE ;
  static Boolean traversal_in_progress = FALSE ;

  if(    traversal_in_progress
     ||  !(top_shell = _XmFindTopMostShell( wid))
     ||  top_shell->core.being_destroyed
     ||  !(focus_data = _XmGetFocusData( wid))
     ||  (focus_data->focus_policy != XmEXPLICIT)    )
    {
      return FALSE ;
    }
  traversal_in_progress = TRUE ;

  /* Recursive traversal calls can sometimes be generated during
   * the handling of focus events and associated callbacks.
   * In this version of Motif, recursive calls always fail.
   *
   * Future enhancements could include the addition of a queue
   * for recursive calls; these calls would then be serviced on
   * a FIFO basis following the completion of the initial traversal
   * processing.  Sequential FIFO processing is essential for
   * providing a consistent and predicable environment for
   * focus change callbacks and event processing.
   */
  trav_list = &(focus_data->trav_graph) ;
  old_focus = focus_data->focus_item ;

  if(    (old_focus == NULL)
     &&  (wid == top_shell)
     &&  focus_data->first_focus
     &&  _XmIsTraversable( focus_data->first_focus, TRUE)    )
    {
      new_focus = focus_data->first_focus ;
    }
  else
    {
      new_focus = _XmTraverse( trav_list, direction, wid) ;
    }
  if(    new_focus
     &&  (new_focus == old_focus)
     &&  focus_data->old_focus_item    )
    {
      /* When traversal does not cause the focus to change
       * to a different widget, focus-change events should
       * not be generated.  The old_focus_item will be NULL
       * when the focus is moving into this shell hierarchy
       * from a different shell; in this case, focus-in
       * events should be generated below.
       */
      rtnVal = TRUE ;
    }
  else
    {
      if(    new_focus
	 &&  (new_active_tab = XmGetTabGroup( new_focus))
	 &&  _XmCallFocusMoved( old_focus, new_focus, NULL)
	 &&  _XmCallTraverseObscured( new_focus, direction)    )
	{
	  /* Set the keyboard focus in two steps; first to None, then
	   * to the new focus widget.  This will cause appropriate
	   * focus-in and focus-out events to be generated, even if
	   * the focus change is between two gadgets.
	   *
	   * Note that XtSetKeyboardFocus() generates focus change
	   * events "in-line", so focus data and manager active_child
	   * fields are not updated until after the focus-out events have
	   * been generated and dispatched to the current focus item.
	   *
	   * The FocusResetFlag is used to tell event actions procs to
	   * ignore any focus-in event that might be generated by the
	   * window manager (which won't like the fact that there the
	   * focus is now going to point to nobody).
	   */
	  _XmSetFocusResetFlag( top_shell, TRUE) ;
	  XtSetKeyboardFocus( top_shell, None) ;
	  _XmSetFocusResetFlag( top_shell, FALSE) ;

	  _XmClearFocusPath( old_focus) ;

	  focus_data->active_tab_group = new_active_tab ;

	  if(    (new_active_tab != new_focus)
	     &&  XmIsManager( new_active_tab)    )
	    {
	      ((XmManagerWidget) new_active_tab)
	                                   ->manager.active_child = new_focus ;
	    }
	  if(    (new_active_tab != XtParent( new_focus))  /* Set above. */
	     &&  XmIsManager( XtParent( new_focus))    )
	    {
	      ((XmManagerWidget) XtParent( new_focus))
		                           ->manager.active_child = new_focus ;
	    }
	  focus_data->focus_item = new_focus ;
	  focus_data->old_focus_item = old_focus ? old_focus : new_focus ;

	  /* Setting the focus data and manager active_child fields enables
	   * focus-in events to be propagated to the new focus widget.
	   */
	  XtSetKeyboardFocus( top_shell, new_focus) ;

	  rtnVal = TRUE ;
	}
      else
	{
	  /* Have failed to traverse to a new widget focus widget.
	   * If the current focus widget is no longer traversable,
	   * then reset focus data to its bootstrap state.
	   */
	  if(    !old_focus
	     ||  !_XmIsTraversable( old_focus, TRUE)    )
	    {
	      Widget firstManaged = FindFirstManaged( top_shell) ;

	      _XmSetFocusResetFlag( top_shell, TRUE) ;
	      XtSetKeyboardFocus( top_shell, firstManaged) ;
	      _XmSetFocusResetFlag( top_shell, FALSE) ;

	      _XmClearFocusPath( old_focus) ;
	      _XmFreeTravGraph( trav_list) ;
	    }
	}
    }
  if(    trav_list->num_entries
     &&  (focus_data->focalPoint == XmUnrelated)
     &&  (    XmIsVendorShell( top_shell)
	 ||  !_XmFocusIsInShell( top_shell))    )
    {
      /* Free the graversal graph whenever the focus is out of this
       * shell hierarchy, so memory use is limited to one traversal
       * graph per display.  Since VendorShell has a handler which
       * tracks the input focus, all we need to do is look at the
       * focusData field.  For MenuShell and others, we need to go
       * through the X server to find out where the focus is.
       *
       * Note the logic of the above conditional; VendorShell is the
       * only shell class that maintains the focalPoint field of the
       * focus data.  So, if its a VendorShell and focalPoint says
       * "unrelated", we have the answer; any other shell and we need
       * to call the generic focus test routine.
       */
      _XmFreeTravGraph( trav_list) ;
    }
  traversal_in_progress = FALSE ;
  return rtnVal ;
}

static Boolean
#ifdef _NO_PROTO
_XmCallTraverseObscured( new_focus, dir)
        Widget new_focus ;
        XmTraversalDirection dir ;
#else
_XmCallTraverseObscured(
        Widget new_focus,
        XmTraversalDirection dir)
#endif /* _NO_PROTO */
{   
            Widget ancestor = new_focus ;
            XRectangle focus_rect ;
            Widget sw ;
            XmTraverseObscuredCallbackStruct call_data ;

    call_data.reason = XmCR_OBSCURED_TRAVERSAL ;
    call_data.event = NULL ;
    call_data.traversal_destination = new_focus ;
    call_data.direction = dir ;

    _XmSetRect( &focus_rect, new_focus) ;

    while ((ancestor = _XmGetClippingAncestor( ancestor, &focus_rect)) != NULL)
    {   
        if ((sw = _XmIsScrollableClipWidget( ancestor, &focus_rect)) != NULL)
        {   
                    XtCallbackList callbacks = ((XmScrolledWindowWidget) sw)
                                           ->swindow.traverseObscuredCallback ;
            XtCallCallbackList( sw, callbacks, (XtPointer) &call_data) ;

            ancestor = sw ;
            } 
        else
        {   _XmIntersectRect( &focus_rect, ancestor, &focus_rect) ;
            } 
        } 
    return _XmIsTraversable( new_focus, TRUE);
    }

void 
#ifdef _NO_PROTO
_XmClearFocusPath( wid )
        Widget wid ;
#else
_XmClearFocusPath(
        Widget wid )
#endif /* _NO_PROTO */
{
  /* This routine should be called whenever the focus of a shell
   * hierarchy needs to be reset to the bootstrap condition.
   *
   * This routine clears the active_child field of all manager
   * widget ancestors of the widget argument, and clears other
   * focus widget fields of the focus data record.  The clearing
   * of the old_focus_item field indicates to the traversal code
   * that the focus is not in this shell hierarchy.
   */
  XmFocusData focus_data ;

  while(    wid  &&  !XtIsShell( wid)    )
    {
      if(    XmIsManager( wid)    )
	{
	  ((XmManagerWidget) wid)->manager.active_child = NULL ;
	}
      wid = XtParent( wid) ;
    }
  if(    (focus_data = _XmGetFocusData( wid)) != NULL    )
    {
      focus_data->focus_item = NULL ;
      focus_data->old_focus_item = NULL ;
      focus_data->active_tab_group = NULL ;
    }
}

Boolean 
#ifdef _NO_PROTO
_XmFocusIsHere( w )
        Widget w ;
#else
_XmFocusIsHere(
        Widget w )
#endif /* _NO_PROTO */
{
    XmFocusData focus_data;
    Widget	item;

    if ((focus_data = _XmGetFocusData( w)) &&
	(item = focus_data->focus_item))
      {
	  for (; !XtIsShell(item); item = XtParent(item))
	    if (item == w)
	      return True;
      }
    return(False);
}

void 
#ifdef _NO_PROTO
_XmProcessTraversal( w, dir, check )
        Widget w ;
        XmTraversalDirection dir ;
        Boolean check ;
#else
_XmProcessTraversal(
        Widget w,
        XmTraversalDirection dir,
#if NeedWidePrototypes
        int check )
#else
        Boolean check )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
  _XmMgrTraversal( w, dir) ;
}

unsigned char 
#ifdef _NO_PROTO
_XmGetFocusPolicy( w )
        Widget w ;
#else
_XmGetFocusPolicy(
        Widget w )
#endif /* _NO_PROTO */
{   
            Widget topmost_shell ;

    /* Find the topmost shell widget
    */
    topmost_shell = _XmFindTopMostShell( w) ;

    if(    XtIsVendorShell( topmost_shell)    )
    {   
        return( ((XmVendorShellExtObject) (_XmGetWidgetExtData( topmost_shell,
                           XmSHELL_EXTENSION))->widget)->vendor.focus_policy) ;
        } 
    else
    {   if(    XmIsMenuShell( topmost_shell)    )
        {   
            return( ((XmMenuShellWidget) topmost_shell)
                                                   ->menu_shell.focus_policy) ;
            } 
        } 
    return( XmPOINTER) ;
    }

Widget 
#ifdef _NO_PROTO
_XmFindTopMostShell( w )
        Widget w ;
#else
_XmFindTopMostShell(
        Widget w )
#endif /* _NO_PROTO */
{   
    while(    w && !XtIsShell( w)    )
    {   w = XtParent( w) ;
        } 
    return( w) ;
    }

void 
#ifdef _NO_PROTO
_XmFocusModelChanged( wid, client_data, call_data )
        Widget wid ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
_XmFocusModelChanged(
        Widget wid,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
  /* Invoked by the VendorShell widget, when the focus_policy changes.
   * Registered as a callback by both the Manager and Primitive classes,
   * when the parent is a VendorShell widget.
   */
  unsigned char new_focus_policy = (unsigned) call_data ;
  Widget shell = _XmFindTopMostShell( wid) ;
  XmFocusData focus_data = _XmGetFocusData( shell) ;

  if(    focus_data    )
    {
      if(    new_focus_policy == XmEXPLICIT    )
	{
	  Widget new_item = focus_data->pointer_item ;

	  if(    new_item != NULL    )
	    {
	      if(    XmIsManager( new_item)
		 &&  (((XmManagerWidget) new_item)
		                     ->manager.highlighted_widget != NULL)    )
		{
		  new_item = ((XmManagerWidget) new_item)
		                                 ->manager.highlighted_widget ;
		}
	      _XmWidgetFocusChange( new_item, XmLEAVE) ;
	    }
	  if(    (new_item == NULL)
	     ||  !_XmMgrTraversal( new_item, XmTRAVERSE_CURRENT)    )
	    {
	      _XmMgrTraversal( shell, XmTRAVERSE_CURRENT) ;
	    }
	}
      else /* new_focus_policy == XmPOINTER */
	{
	  if(    focus_data->focus_item    )
	    {
	      Widget firstManaged = FindFirstManaged( shell) ;

	      _XmWidgetFocusChange( focus_data->focus_item, XmFOCUS_OUT) ;

	      _XmClearFocusPath( focus_data->focus_item) ;
	      _XmSetFocusResetFlag( shell, TRUE) ;
	      XtSetKeyboardFocus( shell, firstManaged) ;
	      _XmSetFocusResetFlag( shell, FALSE) ;
	    }
	  _XmFreeTravGraph( &(focus_data->trav_graph)) ;
	}
    }
}

Boolean 
#ifdef _NO_PROTO
_XmGrabTheFocus( w, event )
        Widget w ;
        XEvent *event ;
#else
_XmGrabTheFocus(
        Widget w,
        XEvent *event )
#endif /* _NO_PROTO */
{
  /* Function used by TextEdit widgets to grab the focus.
   */
  return _XmMgrTraversal( w, XmTRAVERSE_CURRENT) ;
}

XmFocusData 
#ifdef _NO_PROTO
_XmGetFocusData( wid )
        Widget wid ;
#else
_XmGetFocusData(
        Widget wid )
#endif /* _NO_PROTO */
{   
  /* This function returns a pointer to the focus data associated with the
   * topmost shell.  This allows us to treat the location opaquely.
   */
  while(    wid && !XtIsShell( wid)    )
    {
      wid = XtParent( wid) ;
    } 
  if(    wid  &&  !(wid->core.being_destroyed)    )
    {
      if(    XmIsVendorShell( wid)    )
	{   
	  XmVendorShellExtObject vse = (XmVendorShellExtObject)
	                 _XmGetWidgetExtData( wid, XmSHELL_EXTENSION)->widget ;
	  if(    vse  &&  vse->vendor.focus_data    )
	    {
	      vse->vendor.focus_data->focus_policy = vse->vendor.focus_policy ;
	      return vse->vendor.focus_data ;
	    }
	}
      else
	{
	  if(    XmIsMenuShell( wid)
	     &&  ((XmMenuShellWidget) wid)->menu_shell.focus_data    )
	    {   
	      ((XmMenuShellWidget) wid)->menu_shell.focus_data->
		                    focus_policy = ((XmMenuShellWidget) wid)
		                                    ->menu_shell.focus_policy ;
	      return ((XmMenuShellWidget) wid)->menu_shell.focus_data ;  
	    }
	}
    }
  return NULL ;
}

Boolean 
#ifdef _NO_PROTO
_XmCreateVisibilityRect( w, rectPtr )
        Widget w ;
        XRectangle *rectPtr ;
#else
_XmCreateVisibilityRect(
        Widget w,
        XRectangle *rectPtr )
#endif /* _NO_PROTO */
{   
    /* This function will generate a rectangle describing the portion of the
    *    specified widget which is not clipped by any of its ancestors.
    *    It also verifies that the ancestors are both managed and
    *    mapped_when_managed.
    *  It will return TRUE if the rectangle returned in rectPtr has a
    *    non-zero area; it will return FALSE if the widget is not visible.
    *  If w is the work area child of an automatic scrolled window with
    *    a non-null XmNtraverseObscuredCallback, then the clip window
    *    is used as the initial rectangle for w.
    */
            Widget sw ;

    if(    !_XmIsViewable( w)    )
    {   
        _XmClearRect( rectPtr) ;
        return( False) ;
        }
    if(    w  &&  XtParent( w)
        && (sw = _XmIsScrollableClipWidget( XtParent( w), rectPtr))    )
    {   
        w = sw ;

        if(    !_XmIsViewable( w)    )
        {   
            _XmClearRect( rectPtr) ;
            return( False) ;
            }
        } 
    else
    {   _XmSetRect( rectPtr, w) ;
        } 

    /* Process all widgets, excluding the shell widget.
    */
    while(    (w = XtParent( w))  &&  !XtIsShell( w)    )
    {   
        if(    !_XmIsViewable( w)
            || !_XmIntersectRect( rectPtr, w, rectPtr)    )
        {   
            _XmClearRect( rectPtr) ;
            return( False) ;
            }
        }
    return( True) ;
    }

void 
#ifdef _NO_PROTO
_XmSetRect( rect, w )
        register XRectangle *rect ;
        Widget w ;
#else
_XmSetRect(
        register XRectangle *rect,
        Widget w )
#endif /* _NO_PROTO */
{
  /* Initialize the rectangle structure to the specified values.
   * The widget must be realized.
   */
   Position x, y;

   XtTranslateCoords(XtParent(w), w->core.x, w->core.y, &x, &y);
   rect->x = x + w->core.border_width;
   rect->y = y + w->core.border_width;
   rect->width = w->core.width;
   rect->height = w->core.height;
}

int 
#ifdef _NO_PROTO
_XmIntersectRect( srcRectA, widget, dstRect )
        register XRectangle *srcRectA ;
        register Widget widget ;
        register XRectangle *dstRect ;
#else
_XmIntersectRect(
        register XRectangle *srcRectA,
        register Widget widget,
        register XRectangle *dstRect )
#endif /* _NO_PROTO */
{
  /* Intersects the specified rectangle with the rectangle describing the
   * passed-in widget.  Returns True if they intersect, or False if they
   * do not.
   */
        XRectangle srcRectB ;
        Position x, y ;

    XtTranslateCoords( XtParent( widget), widget->core.x, widget->core.y,
                                                                      &x, &y) ;
    srcRectB.x = x + XtBorderWidth( widget) ;
    srcRectB.y = y + XtBorderWidth( widget) ;
    srcRectB.width = XtWidth( widget) ;
    srcRectB.height = XtHeight( widget) ;

    return( (int) _XmIntersectionOf( srcRectA, &srcRectB, dstRect)) ;
    }

int 
#ifdef _NO_PROTO
_XmEmptyRect( r )
        register XRectangle *r ;
#else
_XmEmptyRect(
        register XRectangle *r )
#endif /* _NO_PROTO */
{
   if (r->width <= 0 || r->height <= 0)
      return (TRUE);

   return (FALSE);
}

void 
#ifdef _NO_PROTO
_XmClearRect( r )
        register XRectangle *r ;
#else
_XmClearRect(
        register XRectangle *r )
#endif /* _NO_PROTO */
{
   r->x = 0;
   r->y = 0;
   r->width = 0;
   r->height = 0;
}

Boolean
#ifdef _NO_PROTO
_XmIsNavigable( wid)
	Widget wid ;
#else
_XmIsNavigable( 
	Widget wid)
#endif /* _NO_PROTO */
{
  XmNavigability nav = _XmGetNavigability( wid) ;
  if(    (nav != XmTAB_NAVIGABLE)
     &&  (nav != XmCONTROL_NAVIGABLE)    )
    {
      return FALSE ;
    }
  while(    (wid = XtParent( wid)) && !XtIsShell( wid)    )
    {
      if(    !_XmGetNavigability( wid)    )
	{
	  return FALSE ;
	}
    }
  return TRUE ;
}

void
#ifdef _NO_PROTO
_XmWidgetFocusChange( wid, change)
        Widget wid ;
        XmFocusChange change ;
#else
_XmWidgetFocusChange(
        Widget wid,
        XmFocusChange change)
#endif /* _NO_PROTO */
{   
        XmBaseClassExt *er ;

    if(    XtIsRectObj( wid)
        && !wid->core.being_destroyed    )
    {   
        if(    (er = _XmGetBaseClassExtPtr( XtClass( wid), XmQmotif))
            && (*er)
           && ((*er)->version >= XmBaseClassExtVersion)
            && (*er)->focusChange    )
        {   
            (*((*er)->focusChange))( wid, change) ;
            } 
        else
        {   /* From here on is compatibility code.
            */
                    WidgetClass wc ;

            if(    XmIsPrimitive( wid)    )
            {   
                wc = (WidgetClass) &xmPrimitiveClassRec ;
                }
            else
            {   if(    XmIsGadget( wid)     )
                {   
                    wc = (WidgetClass) &xmGadgetClassRec ;
                    } 
                else
                {   if(    XmIsManager( wid)    )
                    {   
                        wc = (WidgetClass) &xmManagerClassRec ;
                        } 
                    else
                    {   wc = NULL ;
                        } 
                    } 
                } 
            if(    wc
                && (er = _XmGetBaseClassExtPtr( wc, XmQmotif))
                && (*er)
	        && ((*er)->version >= XmBaseClassExtVersion)
                && (*er)->focusChange    )
            {   
                (*((*er)->focusChange))( wid, change) ;
                } 
            }
        }
    return ;
    } 

Widget 
#ifdef _NO_PROTO
_XmNavigate( wid, direction )
        Widget wid ;
        XmTraversalDirection direction ;
#else
_XmNavigate(
        Widget wid,
        XmTraversalDirection direction )
#endif /* _NO_PROTO */
{
  XmFocusData focus_data;
  Widget nav_wid = NULL ;
  Widget shell = _XmFindTopMostShell( wid) ;

  if(    (focus_data = _XmGetFocusData( shell))
     &&  (focus_data->focus_policy == XmEXPLICIT)    )
    {
      XmTravGraph trav_list = &(focus_data->trav_graph) ;

      nav_wid = _XmTraverse( trav_list, direction, wid) ;

      if(    trav_list->num_entries
	 &&  (focus_data->focalPoint == XmUnrelated)
	 &&  (    XmIsVendorShell( shell)
	      ||  !_XmFocusIsInShell( shell))    )
	{
	  _XmFreeTravGraph( trav_list) ;
	}
    }
  return nav_wid ;
}

Widget 
#ifdef _NO_PROTO
_XmFindNextTabGroup( wid )
        Widget wid ;
#else
_XmFindNextTabGroup(
        Widget wid )
#endif /* _NO_PROTO */
{
  return _XmNavigate( wid, XmTRAVERSE_NEXT_TAB_GROUP) ;
}

Widget 
#ifdef _NO_PROTO
_XmFindPrevTabGroup( wid )
        Widget wid ;
#else
_XmFindPrevTabGroup(
        Widget wid )
#endif /* _NO_PROTO */
{
  return _XmNavigate( wid, XmTRAVERSE_PREV_TAB_GROUP) ;
}

void
#ifdef _NO_PROTO
_XmSetInitialOfTabGroup( tab_group, init_focus)
	Widget tab_group ;
	Widget init_focus ;
#else
_XmSetInitialOfTabGroup(
	Widget tab_group,
	Widget init_focus)
#endif /* _NO_PROTO */
{
  XmFocusData focus_data ;

  if(    XmIsManager( tab_group)    )
    {
      ((XmManagerWidget) tab_group)->manager.initial_focus = init_focus ;
    }
  if(    (focus_data = _XmGetFocusData( tab_group))
     &&  focus_data->trav_graph.num_entries    )
    {
      _XmSetInitialOfTabGraph( &(focus_data->trav_graph),
			                               tab_group, init_focus) ;
    }
}

static Boolean
#ifdef _NO_PROTO
_XmIsTraversable( wid, require_in_view)
        Widget wid ;
	Boolean require_in_view ;
#else
_XmIsTraversable( 
        Widget wid,
#if NeedWidePrototypes
	int require_in_view)
#else
	Boolean require_in_view)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
  if(    wid
     &&  _XmIsNavigable( wid)    )
    {
      if(    require_in_view    )
	{
	  return (XmGetVisibility( wid) != XmVISIBILITY_FULLY_OBSCURED) ;
	}
      else
	{
	  /* _XmGetEffectiveView() returns the view port in
	   * which the widget could be viewed through the use
	   * of the XmNtraverseObscuredCallback of ScrolledWindow.
	   */
	  XRectangle visRect ;

	  return _XmGetEffectiveView( wid, &visRect) ;
	}
    } 
  return FALSE ;
} 

void
#ifdef _NO_PROTO
_XmResetTravGraph( wid )
	Widget wid ;
#else
_XmResetTravGraph(
	Widget wid)
#endif /* _NO_PROTO */
{
  XmFocusData focus_data = _XmGetFocusData( wid) ;

  if(    focus_data  &&  focus_data->trav_graph.num_entries    )
    {
      _XmFreeTravGraph( &(focus_data->trav_graph)) ;
    }
}

Boolean
#ifdef _NO_PROTO
_XmFocusIsInShell( wid )
	Widget wid ;
#else
_XmFocusIsInShell(
        Widget wid)
#endif /* _NO_PROTO */
{
  Window focus ;
  Widget focus_wid ;
  Widget shell_of_wid = _XmFindTopMostShell( wid) ;
  XmFocusData focus_data ;
  int revert ;

  if(    XmIsVendorShell( shell_of_wid)
     &&  (focus_data = _XmGetFocusData( shell_of_wid))    )
    {
      if(    focus_data->focalPoint != XmUnrelated    )
	{
	  return TRUE ;
	}
    }
  else
    {
      XGetInputFocus( XtDisplay( shell_of_wid), &focus, &revert) ;

      if(    (focus != PointerRoot)
	 &&  (focus != None)
	 &&  (focus_wid = XtWindowToWidget( XtDisplay( shell_of_wid), focus))
	 &&  (shell_of_wid == _XmFindTopMostShell( focus_wid))    )
	{
	  return TRUE ;
	}
    }
return FALSE ;
}

Boolean
#ifdef _NO_PROTO
_XmShellIsExclusive( wid)
	Widget wid ;
#else
_XmShellIsExclusive(
	Widget wid)
#endif /* _NO_PROTO */
{
  XmFocusData focusData = _XmGetFocusData( wid) ;

  if(    focusData
     &&  focusData->trav_graph.exclusive    )
    {
      return TRUE ;
    }
  return FALSE ;
}

static Widget
#ifdef _NO_PROTO
FindFirstFocus( wid)
	Widget wid ;
#else
FindFirstFocus(
	Widget wid)
#endif /* _NO_PROTO */
{
  Widget shell = _XmFindTopMostShell( wid) ;

  return _XmNavigate( shell, XmTRAVERSE_CURRENT) ;
}

Widget
#ifdef _NO_PROTO
_XmGetFirstFocus( wid)
	Widget wid ;
#else
_XmGetFirstFocus(
	Widget wid)
#endif /* _NO_PROTO */
{
  XmFocusData focus_data = _XmGetFocusData( wid) ;

  if(    focus_data    )
    {
      if(    focus_data->focus_item    )
	{
	  return focus_data->focus_item ;
	}
      else
	{
	  if(    focus_data->first_focus == NULL    )
	    {
	      focus_data->first_focus = FindFirstFocus( wid) ;
	    }
          return focus_data->first_focus ;
	}
    }
  return NULL ;
}


/*******************
 * Public procedures
 *******************/

Boolean
#ifdef _NO_PROTO
XmIsTraversable( wid)
        Widget wid ;
#else
XmIsTraversable( 
        Widget wid)
#endif /* _NO_PROTO */
{   
  return _XmIsTraversable( wid, FALSE) ;
} 

XmVisibility
#ifdef _NO_PROTO
XmGetVisibility( wid)
        Widget wid ;
#else
XmGetVisibility( 
        Widget wid)
#endif /* _NO_PROTO */
{   

        XRectangle rect ;

    if(    !wid
        || !_XmCreateVisibilityRect( wid, &rect)    )
    {   
        return( XmVISIBILITY_FULLY_OBSCURED) ;
        } 
    if(    (rect.width != XtWidth( wid))
        || (rect.height != XtHeight( wid))    )
    {   
        return( XmVISIBILITY_PARTIALLY_OBSCURED) ;
        } 
    return( XmVISIBILITY_UNOBSCURED) ;
    } 

Widget
#ifdef _NO_PROTO
XmGetTabGroup( wid)
        Widget wid ;
#else
XmGetTabGroup( 
        Widget wid)
#endif /* _NO_PROTO */
{   
  XmFocusData focus_data ;
  Boolean exclusive ;

  if(    !wid
     || (_XmGetFocusPolicy( wid) != XmEXPLICIT)
     || !(focus_data = _XmGetFocusData( wid))    )
    {   
      return( NULL) ;
    } 
  exclusive = !!(focus_data->trav_graph.exclusive) ;

  do
    {
      XmNavigationType nav_type = _XmGetNavigationType( wid) ;
      
      if(    (nav_type == XmSTICKY_TAB_GROUP)
	 ||  (nav_type == XmEXCLUSIVE_TAB_GROUP)
	 ||  (    (nav_type == XmTAB_GROUP)
	      &&  !exclusive)    )
	{
	  return( wid) ;
	}
    } while(    (wid = XtParent( wid)) && !XtIsShell( wid)    ) ;

  return wid ;
} 

Widget
#ifdef _NO_PROTO
XmGetFocusWidget( wid)
        Widget wid ;
#else
XmGetFocusWidget( 
        Widget wid)
#endif /* _NO_PROTO */
{   
  Widget focus_wid = NULL ;
  XmFocusData focus_data = _XmGetFocusData( wid) ;

  if(    focus_data != NULL    )
    {   
      if(    focus_data->focus_policy == XmEXPLICIT    )
        {
	  focus_wid = focus_data->focus_item ;
	} 
      else
        {
	  focus_wid = focus_data->pointer_item ;

	  if(    (focus_wid != NULL)
	     &&  XmIsManager( focus_wid)
	     &&  (((XmManagerWidget) focus_wid)
		                     ->manager.highlighted_widget != NULL)    )
	    {
	      focus_wid = ((XmManagerWidget) focus_wid)
		                                 ->manager.highlighted_widget ;
	    }
	} 
    }
  return focus_wid ;
}

Boolean 
#ifdef _NO_PROTO
XmProcessTraversal( w, dir )
        Widget w ;
        XmTraversalDirection dir ;
#else
XmProcessTraversal(
        Widget w,
        XmTraversalDirection dir)
#endif /* _NO_PROTO */
{   
  XmFocusData focus_data ;

  if(    (focus_data = _XmGetFocusData( w))
     &&  (focus_data->focus_policy == XmEXPLICIT)    )
    {   
      if(    dir != XmTRAVERSE_CURRENT    )
        {   
	  if(    focus_data->focus_item    )
	    {
	      w = focus_data->focus_item ;
            }
	  else
	    {
	      w = _XmFindTopMostShell( w) ;
	    }
        }
      return _XmMgrTraversal( w, dir) ;
    }
  return FALSE ;
}

void 
#ifdef _NO_PROTO
XmAddTabGroup( tabGroup )
        Widget tabGroup ;
#else
XmAddTabGroup(
        Widget tabGroup )
#endif /* _NO_PROTO */
{
    Arg		arg;

    XtSetArg(arg, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP);
    XtSetValues(tabGroup, &arg, 1);
}

void 
#ifdef _NO_PROTO
XmRemoveTabGroup( w )
        Widget w ;
#else
XmRemoveTabGroup(
        Widget w )
#endif /* _NO_PROTO */
{
  Arg		arg;

  XtSetArg(arg, XmNnavigationType, XmNONE);
  XtSetValues(w, &arg, 1);
}
