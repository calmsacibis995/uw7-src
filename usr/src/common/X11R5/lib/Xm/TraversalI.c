#pragma ident	"@(#)m1.2libs:Xm/TraversalI.c	1.2"
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
static char SCCSID[] = "OSF/Motif: @(#)TraversalI.c	4.13 92/03/02";
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
#include <Xm/SashP.h>
#include <Xm/ScrolledWP.h>
#include <Xm/DrawingAP.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif


#define XmTRAV_LIST_ALLOC_INCREMENT 16
#define XmTAB_LIST_ALLOC_INCREMENT 8
#define STACK_SORT_LIMIT 128


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean EffectiveView() ;
static Boolean NodeIsTraversable() ;
static XmTraversalNode TraverseControl() ;
static XmTraversalNode NextControl() ;
static XmTraversalNode PrevControl() ;
static Boolean InitializeCurrent() ;
static XmTraversalNode GetNextNearestNode() ;
static XmTraversalNode TraverseTab() ;
static XmTraversalNode AllocListEntry() ;
static Boolean GetChildList() ;
static void GetNodeList() ;
static XmTraversalNode GetNodeFromGraph() ;
static XmTraversalNode GetNodeOfWidget() ;
static void LinkNodeList() ;
static void Sort();
static int CompareExclusive() ;
static int CompareNodesHoriz() ;
static int CompareNodesVert() ;
static void SortGraph() ;
static void SortNodeList() ;
static Boolean SetInitialNode() ;
static void SetInitialWidgets() ;
static void GetRectRelativeToShell() ;
static int SearchTabList() ;
static void DeleteFromTabList() ;
#ifdef CDE_TAB
Boolean _XmTraverseWillWrap ();
#endif /* CDE_TAB */
#else

static Boolean EffectiveView( 
                        Widget wid,
                        XRectangle *viewRect,
                        XRectangle *visRect) ;
static Boolean NodeIsTraversable( 
                        XmTraversalNode node) ;
static XmTraversalNode TraverseControl( 
                        XmTraversalNode cur_node,
                        XmTraversalDirection action) ;
static XmTraversalNode NextControl( 
                        XmTraversalNode ctl_node) ;
static XmTraversalNode PrevControl( 
                        XmTraversalNode ctl_node) ;
static Boolean InitializeCurrent( 
                        XmTravGraph list,
                        Widget wid,
#if NeedWidePrototypes
                        int renew_list_if_needed) ;
#else
                        Boolean renew_list_if_needed) ;
#endif /* NeedWidePrototypes */
static XmTraversalNode GetNextNearestNode( 
                        XmGraphNode graph,
                        XRectangle *rect) ;
static XmTraversalNode TraverseTab( 
                        XmTraversalNode cur_node,
                        XmTraversalDirection action) ;
static XmTraversalNode AllocListEntry( 
                        XmTravGraph list) ;
static Boolean GetChildList( 
                        Widget composite,
                        Widget **widget_list,
                        Cardinal *num_in_list) ;
static void GetNodeList( 
                        Widget wid,
                        XRectangle *parent_rect,
                        XmTravGraph trav_list,
                        int tab_parent,
                        int control_parent) ;
static XmTraversalNode GetNodeFromGraph( 
                        XmGraphNode graph,
                        Widget wid) ;
static XmTraversalNode GetNodeOfWidget( 
                        XmTravGraph trav_list,
                        Widget wid) ;
static void LinkNodeList( 
                        XmTravGraph list) ;
static void Sort(
                        void *base, 
			size_t n_mem, 
			size_t size,
			int(*compare)(XmConst void *, XmConst void *));
static int CompareExclusive( 
                        XmConst void *A,
                        XmConst void *B) ;
static int CompareNodesHoriz( 
                        XmConst void *A,
                        XmConst void *B) ;
static int CompareNodesVert( 
                        XmConst void *A,
                        XmConst void *B) ;
static void SortGraph( 
                        XmGraphNode graph,
#if NeedWidePrototypes
                        int exclusive) ;
#else
                        Boolean exclusive) ;
#endif /* NeedWidePrototypes */
static void SortNodeList( 
                        XmTravGraph trav_list) ;
static Boolean SetInitialNode( 
                        XmGraphNode graph,
                        XmTraversalNode init_node) ;
static void SetInitialWidgets( 
                        XmTravGraph trav_list) ;
static void GetRectRelativeToShell( 
                        Widget wid,
                        XRectangle *rect) ;
static int SearchTabList( 
                        XmTravGraph graph,
                        Widget wid) ;
static void DeleteFromTabList( 
                        XmTravGraph graph,
                        int indx) ;
#ifdef CDE_TAB
Boolean _XmTraverseWillWrap (
	Widget w,
	XmTraversalDirection dir);
#endif /* CDE_TAB */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/* #define DEBUG_PRINT */

#ifdef DEBUG_PRINT
#ifdef _NO_PROTO
static void PrintControl() ;
static void PrintTab() ;
static void PrintGraph() ;
static void PrintNodeList() ;
#else
static void PrintControl( 
                        XmTraversalNode node) ;
static void PrintTab( 
                        XmTraversalNode node) ;
static void PrintGraph( 
                        XmTraversalNode node) ;
static void PrintNodeList( 
                        XmTravGraph list) ;
#endif /* _NO_PROTO */
#endif /* DEBUG_PRINT */

static XmTravGraph SortReferenceGraph ;


XmNavigability
#ifdef _NO_PROTO
_XmGetNavigability( wid)
        Widget wid ;
#else
_XmGetNavigability(
        Widget wid)
#endif /* _NO_PROTO */
{   
  if(    XtIsRectObj( wid)
     && !wid->core.being_destroyed    )
    {   
      XmBaseClassExt *er ;

      if(    (er = _XmGetBaseClassExtPtr( XtClass( wid), XmQmotif))
	 && (*er)
           &&  ((*er)->version >= XmBaseClassExtVersion)
	 &&  (*er)->widgetNavigable    )
        {   
	  return (*((*er)->widgetNavigable))( wid) ;
	}
      else
        {
	  /* From here on is compatibility code.
	   */
	  WidgetClass wc ;

	  if(    XmIsPrimitive( wid)    )
            {   
	      wc = (WidgetClass) &xmPrimitiveClassRec ;
	    }
	  else
            {
	      if(    XmIsGadget( wid)     )
                {   
		  wc = (WidgetClass) &xmGadgetClassRec ;
		} 
	      else
                {
		  if(    XmIsManager( wid)    )
                    {   
		      wc = (WidgetClass) &xmManagerClassRec ;
		    } 
		  else
                    {
		      wc = NULL ;
		    } 
		} 
	    } 
	  if(    wc
	     &&  (er = _XmGetBaseClassExtPtr( wc, XmQmotif))
	     && (*er)
           &&  ((*er)->version >= XmBaseClassExtVersion)
	     &&  (*er)->widgetNavigable    )
            {   
	      return (*((*er)->widgetNavigable))( wid) ;
	    } 
	}
    }
  return XmNOT_NAVIGABLE ;
}

Boolean
#ifdef _NO_PROTO
_XmIsViewable( wid )
        Widget wid ;
#else
_XmIsViewable(
        Widget wid)
#endif /* _NO_PROTO */
{   
    /* This routine returns TRUE is the widget is realized, managed
     *   and, for window objects, mapped; returns FALSE otherwise.
     * A realized RowColumn with a MenuShell parent is always considered
     *   viewable.
     */
            XWindowAttributes xwa ;

    if(    !wid->core.being_destroyed  &&  XtIsRealized( wid)    )
    {   
        /* Treat menupanes specially.
        */
        if(    XmIsRowColumn( wid) && XmIsMenuShell( XtParent( wid))    )
        {   return( TRUE) ;
            }
        if(    XtIsManaged( wid)    )
        {   
            if (XmIsGadget(wid) || wid->core.mapped_when_managed)
            {   return( TRUE) ;
                } 
            /* Managed, but not necessarily mapped.
            */
            XGetWindowAttributes( XtDisplay( wid), XtWindow( wid), &xwa) ;

            if(    xwa.map_state == IsViewable    ) 
            {   return( TRUE) ;
                } 
            } 
        } 
    return( FALSE) ;
    }

Widget
#ifdef _NO_PROTO
_XmGetClippingAncestor( wid, visRect)
        Widget wid ;
        XRectangle *visRect ;
#else
_XmGetClippingAncestor(
        Widget wid,
        XRectangle *visRect)
#endif /* _NO_PROTO */
{   
    /* Returns the widget id of the first ancestor which clips
    *    the visRect.
    *  Returns NULL if visRect is totally unobscured.
    *  visRect is never altered.
    */
        XRectangle clipRect ;
        XRectangle viewRect ;

    while(    (wid = XtParent( wid))  &&  !XtIsShell( wid)    )
    {   
        _XmSetRect( &clipRect, wid) ;
        
        if(    !_XmIntersectionOf( visRect, &clipRect, &viewRect)
            || (viewRect.width != visRect->width)
            || (viewRect.height != visRect->height)    )
        {   
            return( wid) ;
            } 
        } 
    return( NULL) ;
    } 

Widget
#ifdef _NO_PROTO
_XmIsScrollableClipWidget( wid, visRect)
        Widget wid ;
        XRectangle *visRect ;
#else
_XmIsScrollableClipWidget(
        Widget wid,
        XRectangle *visRect)
#endif /* _NO_PROTO */
{   
  XmScrolledWindowWidget sw ;
  if(    XmIsDrawingArea( wid)
     && (((XmDrawingAreaWidget) wid)
                         ->drawing_area.resize_policy == XmRESIZE_SWINDOW)
     && (sw = (XmScrolledWindowWidget) XtParent( wid))
     && XmIsScrolledWindow( sw)
     && (sw->swindow.ClipWindow == (XmDrawingAreaWidget) wid)
     && sw->swindow.traverseObscuredCallback    )
    {   
      if(    visRect    )
        {
	  _XmSetRect( visRect, wid) ;
	} 
      return( (Widget) sw) ;
    }
  return( NULL) ;
} 

Boolean
#ifdef _NO_PROTO
_XmGetEffectiveView( wid, visRect)
        Widget wid ;
        XRectangle *visRect ;
#else
_XmGetEffectiveView( 
        Widget wid,
        XRectangle *visRect)
#endif /* _NO_PROTO */
{   
    return( EffectiveView( wid, NULL, visRect)) ;
    }

static Boolean
#ifdef _NO_PROTO
EffectiveView( wid, viewRect, visRect)
        Widget wid ;
        XRectangle *viewRect ;
        XRectangle *visRect ;
#else
EffectiveView( 
        Widget wid,
        XRectangle *viewRect,
        XRectangle *visRect)
#endif /* _NO_PROTO */
{   
    /* This function will generate a rectangle describing the portion of the
     * specified widget or its scrolling area (for non-null
     * XmNtraverseObscuredCallback) which is not clipped by any of its
     * ancestors.  It also verifies that the ancestors are viewable.
     *
     * It will return TRUE if the visibility rectangle returned in visRect
     * has a non-zero area, FALSE if the visibility area is zero or one 
     * of its ancestors is not viewable.
     */
            Boolean acceptClipping = TRUE ;

    if(    !_XmIsViewable( wid)    )
    {   
        _XmClearRect( visRect) ;

        return( FALSE) ;
        }
    _XmSetRect( visRect, wid) ;

    /* Process all widgets, excluding the shell widget
    */
    while(    (wid = XtParent( wid))  &&  !XtIsShell( wid)    )
    {   
        if(    !_XmIsViewable( wid)    )
        {   
            _XmClearRect( visRect) ;

            return( FALSE) ;
            } 
        if(    _XmIsScrollableClipWidget( wid, visRect)    )
        {   
            /* This wid is the clip window for a Scrolled Window.
            */
            acceptClipping = FALSE ;
            } 
        else
        {   if(    acceptClipping    )
            {   
                if(    !_XmIntersectRect( visRect, wid, visRect)    )
                {   
                    return( FALSE) ;
                    } 
                }
            else
            {           XRectangle tmpRect ;

                if(    !_XmIntersectRect( visRect, wid, &tmpRect)
                    || (visRect->width != tmpRect.width)
                    || (visRect->height != tmpRect.height)    )
                {   
                    _XmClearRect( visRect) ;

                    return( FALSE) ;
                    } 
                } 
            } 
        }
    if(    viewRect    )
    {   
        if(    acceptClipping    )
        {   
            if(    !_XmIntersectionOf( visRect, viewRect, visRect)    )
            {   
                return( FALSE) ;
                } 
            }
        else
        {           XRectangle tmpRect ;

            if(    !_XmIntersectionOf( visRect, viewRect, &tmpRect)
                || (visRect->width != tmpRect.width)
                || (visRect->height != tmpRect.height)    )
            {   
                _XmClearRect( visRect) ;

                return( FALSE) ;
                } 
            } 
        }
    return( TRUE) ;
    }

Boolean
#ifdef _NO_PROTO
_XmIntersectionOf( srcRectA, srcRectB, destRect)
        register XRectangle *srcRectA ;
        register XRectangle *srcRectB ;
        register XRectangle *destRect ;
#else
_XmIntersectionOf(
        register XRectangle *srcRectA,
        register XRectangle *srcRectB,
        register XRectangle *destRect)
#endif /* _NO_PROTO */
{   
    /* Returns TRUE if there is a non-zero area at the intersection of the
    *   two source rectangles, FALSE otherwise.  The destRect receives
    *   the rectangle of intersection (is cleared if no intersection).
    */
            int srcABot, srcBBot ;
            int srcARight, srcBRight ;
            int newHeight, newWidth ;

    srcABot = srcRectA->y + srcRectA->height - 1 ;
    srcBBot = srcRectB->y + srcRectB->height - 1 ;
    srcARight = srcRectA->x + srcRectA->width - 1 ;
    srcBRight = srcRectB->x + srcRectB->width - 1 ;

    if(    srcRectA->x >= srcRectB->x    ) 
    {   destRect->x = srcRectA->x ;
        } 
    else 
    {   destRect->x = srcRectB->x ;
        } 
    if(    srcRectA->y > srcRectB->y    ) 
    {   destRect->y = srcRectA->y ;
        } 
    else 
    {   destRect->y = srcRectB->y ;
        } 
    if(    srcARight >= srcBRight    ) 
    {   
        newWidth = srcBRight - destRect->x + 1 ;
        destRect->width = (newWidth > 0) ? newWidth : 0 ;
        }
    else
    {   newWidth = srcARight - destRect->x + 1 ;
        destRect->width = (newWidth > 0) ? newWidth : 0 ;
        }
    if(    srcABot > srcBBot    ) 
    {   
        newHeight = srcBBot - destRect->y + 1 ; 
        destRect->height = (newHeight > 0) ? newHeight : 0 ;
        }
    else 
    {   newHeight = srcABot - destRect->y + 1 ;
        destRect->height = (newHeight > 0) ? newHeight : 0 ;
        }
    if(    !destRect->width  ||  !destRect->height    )
    {   return( FALSE) ;
        } 
    return( TRUE) ;
    } 

XmNavigationType
#ifdef _NO_PROTO
_XmGetNavigationType( widget)
        Widget widget ;
#else
_XmGetNavigationType(
        Widget widget)
#endif /* _NO_PROTO */
{   
  if(    XmIsPrimitive( widget)    )
    {
      return ((XmPrimitiveWidget) widget)->primitive.navigation_type ;
    }
  else
    {
      if(    XmIsGadget( widget)    )
	{
	  return ((XmGadget) widget)->gadget.navigation_type ;	  
	}
      else
	{
	  if(    XmIsManager( widget)    )
	    {
	      return ((XmManagerWidget) widget)->manager.navigation_type ;
	    }
	}
    }
  return XmNONE ;
}
    
Widget
#ifdef _NO_PROTO
_XmGetActiveTabGroup( wid)
        Widget wid ;
#else
_XmGetActiveTabGroup(
        Widget wid)
#endif /* _NO_PROTO */
{   
  /* Return the active tab group for the specified widget hierarchy
   */
  XmFocusData focus_data = _XmGetFocusData( wid) ;

  if(    !focus_data    )
    {
      return NULL ;
    } 
  return focus_data->active_tab_group ;
}

static Boolean
#ifdef _NO_PROTO
NodeIsTraversable( node)
	XmTraversalNode node ;
#else
NodeIsTraversable(
	XmTraversalNode node)
#endif /* _NO_PROTO */
{
  if(    !node
     ||  !(node->any.widget)
     ||  (node->any.type == XmTAB_GRAPH_NODE)
     ||  (node->any.type == XmCONTROL_GRAPH_NODE)    )
    {
      return FALSE ;
    }
  return XmIsTraversable( node->any.widget) ;
}

static XmTraversalNode 
#ifdef _NO_PROTO
TraverseControl( cur_node, action )
        XmTraversalNode cur_node ;
        XmTraversalDirection action ;
#else
TraverseControl(
        XmTraversalNode cur_node,
        XmTraversalDirection action )
#endif /* _NO_PROTO */
{
  XmTraversalNode new_ctl ;

  if(    !cur_node    )
    {
      return NULL ;
    }
  if(    cur_node->any.type == XmCONTROL_GRAPH_NODE    )
    {
      cur_node = cur_node->graph.sub_head ;
      if(    !cur_node    )
	{
	  return NULL ;
	}
      action = XmTRAVERSE_HOME ;
    }
  else
    {
      if(    cur_node->any.type != XmCONTROL_NODE    )
	{
	  return NULL ;
	}
    }
  new_ctl = cur_node ;
  do
    {
      switch(    action    )
	{
	case XmTRAVERSE_NEXT:
	  new_ctl = NextControl( new_ctl) ;
	  break ;
	case XmTRAVERSE_RIGHT:
	  new_ctl = new_ctl->any.next ;
	  break ;
	case XmTRAVERSE_UP:
	  new_ctl = new_ctl->control.up ;
	  break ;
	case XmTRAVERSE_PREV:
	  new_ctl = PrevControl( new_ctl) ;
	  break ;
	case XmTRAVERSE_LEFT:
	  new_ctl = new_ctl->any.prev ;
	  break ;
	case XmTRAVERSE_DOWN:
	  new_ctl = new_ctl->control.down ;
	  break ;
	case XmTRAVERSE_HOME:
	  new_ctl = new_ctl->any.tab_parent.link->sub_head ;
	  cur_node = new_ctl->any.tab_parent.link->sub_tail ;
	  action = XmTRAVERSE_RIGHT ;       /* If new_ctl is not traversable.*/
	  break ;
	case XmTRAVERSE_CURRENT:
	  break ;
	default:
	  new_ctl = NULL ;
	  break ;
	}
    } while(    new_ctl
	    && !NodeIsTraversable( new_ctl)
	    && (    (new_ctl != cur_node)
		||  (new_ctl = NULL))    ) ;
  return new_ctl ;
}

static XmTraversalNode
#ifdef _NO_PROTO
NextControl( ctl_node)
	XmTraversalNode ctl_node ;
#else
NextControl(
	XmTraversalNode ctl_node)
#endif /* _NO_PROTO */
{
  register XmTraversalNode ptr = ctl_node ;
  XmTraversalNode next = NULL ;
  XmTraversalNode min = ctl_node ;

  do
    {
      if(    ((unsigned) ptr > (unsigned) ctl_node)
	 &&  (    ((unsigned) ptr < (unsigned) next)
	      ||  (next == NULL))    )
	{
	  next = ptr ;
	}
      if(    (unsigned) ptr < (unsigned) min    )
	{
	  min = ptr ;
	}
      ptr = ptr->any.next ;

    }while(    ptr != ctl_node    ) ;

  if(    next == NULL    )
    {
      next = min ;
    }
  return next ;
}

static XmTraversalNode
#ifdef _NO_PROTO
PrevControl( ctl_node)
	XmTraversalNode ctl_node ;
#else
PrevControl(
	XmTraversalNode ctl_node)
#endif /* _NO_PROTO */
{
  register XmTraversalNode ptr = ctl_node ;
  XmTraversalNode prev = NULL ;
  XmTraversalNode max = ctl_node ;

  do
    {
      if(    ((unsigned) ptr < (unsigned) ctl_node)
	 &&  (    ((unsigned) ptr > (unsigned) prev)
	      ||  (prev == NULL))    )
	{
	  prev = ptr ;
	}
      if(    (unsigned) ptr > (unsigned) max    )
	{
	  max = ptr ;
	}
      ptr = ptr->any.prev ;

    }while(    ptr != ctl_node    ) ;

  if(    prev == NULL   )
    {
      prev = max ;
    }
  return prev ;
}

static Boolean 
#ifdef _NO_PROTO
InitializeCurrent( list, wid, renew_list_if_needed )
        XmTravGraph list ;
        Widget wid ;
	Boolean renew_list_if_needed ;
#else
InitializeCurrent(
        XmTravGraph list,
        Widget wid,
#if NeedWidePrototypes
	int renew_list_if_needed )
#else
	Boolean renew_list_if_needed )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  /* Returns TRUE if the current node has been initialized
   * to a non-NULL value.
   * 
   * This routine first tries a linear search for the first
   * node in the node list.  If it fails to get a match, it
   * will do a linear search successively on each ancestor of
   * the widget, up to the shell.  If a match continues to
   * be elusive, this routine sets the current node to be
   * head of the list, which is always a tab-graph node.
   *
   * In general, this routine will either set list->current
   * to the node of the widget argument, or to a tab-graph
   * node.  The only time when the returned "current" does
   * not match the argument AND it is not a tab-graph node is
   * when an ancestor of the argument widget has navigability
   * of either XmCONTROL_NAVIGABLE or XmTAB_NAVIGABLE (such
   * as Drawing Area).
   */
  XmTraversalNode cur_node = list->current ;

  if(    cur_node    )
    {
      if(    !wid
	 ||  (wid == cur_node->any.widget)    )
	{
	  return TRUE ;
	}
      cur_node = NULL ;
    }
  if(    !(cur_node = GetNodeOfWidget( list, wid))    )
    {
      if(    renew_list_if_needed
	 &&  _XmGetNavigability( wid)    )
	{
	  return _XmNewTravGraph( list, list->top, wid) ;
	}
      while(    (wid = XtParent( wid))  &&  !XtIsShell( wid)
	    &&  !(cur_node = GetNodeOfWidget( list, wid))    )
	{ }
    }
  if(    cur_node    )
    {
      list->current = cur_node ;
    }
  else
    {
      if(    !(list->current)    )
	{
	  list->current = list->head ;
	}
    }
  return TRUE ;
}

Widget 
#ifdef _NO_PROTO
_XmTraverseAway( list, wid, wid_is_control )
        XmTravGraph list ;
        Widget wid ;
	Boolean wid_is_control ;
#else
_XmTraverseAway(
        XmTravGraph list,
        Widget wid,
#if NeedWidePrototypes
	int wid_is_control )
#else
	Boolean wid_is_control )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  /* This routine traverses away from the reference widget.  The routine
   * tries to return the widget that would be the next one traversed due
   * to a XmTRAVERSE_RIGHT or XmTRAVERSE_NEXT_TAB_GROUP.
   * When the wid_is_control argument is TRUE, this routine tries to
   * return the next traversable control following the reference widget.
   */
  if(    !(list->num_entries)    )
    {
      if(    !_XmNewTravGraph( list, list->top, wid)    )
	{
	  return NULL ;
	}
    }
  else
    {
      if(    !InitializeCurrent( list, wid, TRUE)    )
	{
	  return NULL ;
	}
    }
  if(    (list->current->any.widget != wid)
     &&  (list->current->any.type == XmTAB_GRAPH_NODE)    )
    {
      XmTraversalNode nearest_node ;
      XRectangle wid_rect ;

      if(    wid_is_control    )
	{
	  /* A tab-graph node is always immediately followed by its
	   * corresponding control-graph node.
	   */
	  list->current = list->current + 1 ;
	}
      GetRectRelativeToShell( wid, &wid_rect) ;
      
      if ( (nearest_node = GetNextNearestNode((XmGraphNode) list->current, 
					      &wid_rect) ) != NULL)
	{
	  list->current = nearest_node ;
	}
    }
  if(    (list->current->any.widget == wid)
     ||  !NodeIsTraversable( list->current)    )
    {
      XmTraversalNode rtnNode ;

      /* If current is a control node or control graph node, try to
       * traverse to a control.  If current is a tab node, or if the
       * attempted traversal to a control node fails, then traverse
       * to the next tab group.
       */
      if(    (    (list->current->any.type != XmCONTROL_NODE)
	      &&  (list->current->any.type != XmCONTROL_GRAPH_NODE))
	 ||  !(rtnNode = TraverseControl( list->current, XmTRAVERSE_RIGHT))   )
	{
	  rtnNode = TraverseTab( list->current, XmTRAVERSE_NEXT_TAB_GROUP) ;
	}
      list->current = rtnNode ;
    }
  if(    !(list->current)
     ||  (list->current->any.widget == wid)    )
    {
      return NULL ;
    }
  return list->current->any.widget ;
}

static XmTraversalNode
#ifdef _NO_PROTO
GetNextNearestNode( graph, rect)
	XmGraphNode graph ;
	XRectangle *rect ;
#else
GetNextNearestNode(
	XmGraphNode graph,
	XRectangle *rect)
#endif /* _NO_PROTO */
{
  XmTraversalNode node ;
  if(    (node = graph->sub_head) != NULL    )
    {
      XmTraversalNode *list_ptr ;
      unsigned idx ;
      XmTraversalNode *node_list ;
      XmTraversalNode storage[STACK_SORT_LIMIT] ;
      XmTraversalNodeRec reference ;
      unsigned num_nodes = 1 ;    /* One node for the reference rectangle. */

      /* Count the nodes in the graph.
       */
      do
	{
	  ++num_nodes ;
	} while(    (node != graph->sub_tail)
		&&  (node = node->any.next)    ) ;

      if(    num_nodes > STACK_SORT_LIMIT    )
	{
	  node_list = (XmTraversalNode *) XtMalloc( num_nodes
						 * sizeof( XmTraversalNode)) ;
	}
      else
	{
	  node_list = storage ;
	}
      /* Now copy nodes onto node list for sorting.
       */
      list_ptr = node_list ;
      reference.any.rect = *rect ; /* Only the rectangle is used in sorting. */
      reference.any.widget = NULL ;
      *list_ptr++ = &reference ;
      node = graph->sub_head ;
      idx = 1 ;                    /* Leave one for reference node. */
      do
	{
	  *list_ptr++ = node ;
	  node = node->any.next ;
	} while(    ++idx < num_nodes    ) ;

      Sort( node_list, num_nodes,
			         sizeof( XmTraversalNode), CompareNodesHoriz) ;
      /* Now find the reference node in the sorted list.
       */
      idx = 0 ;
      node = NULL ;
      do
	{
	  if(    node_list[idx] == &reference    )
	    {
	      /* Return node which follows the reference node in the
	       * sorted list.
	       */
	      ++idx ;
	      if(    idx == num_nodes    )
		{
		  /* If reference node was the last in the list, then
		   * wrap to the beginning of the list.
		   */
		  idx = 0 ;
		}
	      node = node_list[idx] ;
	      break ;
	    }
	} while(    idx++ < num_nodes    ) ;

      if(    num_nodes > STACK_SORT_LIMIT    )
	{
	  XtFree( (char *) node_list) ;
	}
    }
  return node ;
}

Widget 
#ifdef _NO_PROTO
_XmTraverse( list, action, reference_wid )
        XmTravGraph list ;
        XmTraversalDirection action ;
        Widget reference_wid ;
#else
_XmTraverse(
        XmTravGraph list,
        XmTraversalDirection action,
        Widget reference_wid )
#endif /* _NO_PROTO */
{
  XmTraversalNode rtnNode ;

  if(    (action == XmTRAVERSE_CURRENT)
     &&  reference_wid    )
    {
      /* Try short-circuit evaluation for XmTRAVERSE_CURRENT (for
       * improved performance only; would be handled correctly below).
       */
      XmNavigability wid_nav = _XmGetNavigability( reference_wid) ;

      if(    (wid_nav == XmTAB_NAVIGABLE)
	 ||  (wid_nav == XmCONTROL_NAVIGABLE)    )
	{
	  if(    XmIsTraversable( reference_wid)    )
	    {
	      return reference_wid ;
	    }
	  return NULL ;
	}
    }
  if(    !list->num_entries    )
    {
      if(    !_XmNewTravGraph( list, list->top, reference_wid)    )
	{
	  return NULL ;
	}
    }
  else
    {
      if(    !InitializeCurrent( list, reference_wid, TRUE)    )
	{
	  return NULL ;
	}
    }
  /* After the preceding initialization, list->current either points
   * to the node of the reference widget (or its nearest ancestor
   * which is in the traversal graph) or to the beginning of the list
   * (if the reference widget is NULL).
   *
   * If the reference widget is a composite, then there will be two
   * nodes associated with it; one containing control nodes and the
   * other containing tab nodes (including the node containing
   * is associated control nodes).  For a composite reference
   * widget, list->current will be the tab node, since it is
   * guaranteed to appear first in the list of nodes and will
   * match first in the search for the reference widget.
   */
  if(    action == XmTRAVERSE_CURRENT    )
    {
      if(    list->current->any.widget != reference_wid    )
	{
	  /* The reference widget was not found in the graph.
	   * Finding ancestors is not good enough for XmTRAVERSE_CURRENT.
	   */
	  return NULL ;
	}
      if(    (list->current->any.type == XmTAB_NODE)
	 ||  (list->current->any.type == XmCONTROL_NODE)    )
	{
	  if(    NodeIsTraversable( list->current)    )
	    {
	      return reference_wid ;
	    }
	  /* Since this node has no subgraph (no traversable descendents)
	   * and is not traversable, XmTRAVERSE_CURRENT fails.
	   */
	  return NULL ;
	}
    }
  if(    (action != XmTRAVERSE_NEXT_TAB_GROUP)
     &&  (action != XmTRAVERSE_PREV_TAB_GROUP)
     &&  (    (action != XmTRAVERSE_CURRENT)
	  ||  (list->current->any.type == XmCONTROL_GRAPH_NODE))    )
    {
      /* Either the action is strictly associated with controls
       * or action is XmTRAVERSE_CURRENT and list->current is
       * a control graph node.
       */
      rtnNode = TraverseControl( list->current, action) ;
    }
  else
    {
      /* The action is strictly associated with tab group traversal.
       */
      rtnNode = TraverseTab( list->current, action) ;
    }
  if(    !rtnNode    )
    {
      return NULL ;
    }
  return (list->current = rtnNode)->any.widget ;
}

static XmTraversalNode 
#ifdef _NO_PROTO
TraverseTab( cur_node, action )
        XmTraversalNode cur_node ;
        XmTraversalDirection action ;
#else
TraverseTab(
        XmTraversalNode cur_node,
        XmTraversalDirection action )
#endif /* _NO_PROTO */
{
  /* This routine handles tab group traversal.  It is assumed
   * that the "action" argument is not associated with traversal
   * among controls.
   *
   * Returns the leaf traversal node containing the widget which
   * would next receive the focus.
   *
   */
  XmTraversalNode new_tab ;

  if(    !cur_node    )
    {
      return NULL ;
    }
  if(    cur_node->any.type == XmCONTROL_NODE    )
    {
      /* This routine is for tab group traversal, so if current node
       * is a control node, reset cur_node to be its tab-group control
       * graph.
       */
      cur_node = (XmTraversalNode) cur_node->any.tab_parent.link ;

      /* A tab-graph node is the only node that might have a NULL
       * tab_parent, so cur_node is guaranteed to be non-NULL.
       */
    }
  new_tab = cur_node ;
  do
    {
      switch(    action    )
	{
	case XmTRAVERSE_NEXT_TAB_GROUP:
	case XmTRAVERSE_CURRENT:
	default:
	  if(    (new_tab->any.type == XmTAB_GRAPH_NODE)
	     &&  (new_tab->graph.sub_head)    )
	    {
	      /* new_tab is a tab graph with a non-null subgraph;
	       * go down into the graph to continue the search
	       * for the next traversable node.
	       */
	      new_tab = new_tab->graph.sub_head ;
	    }
	  else
	    {
	      /* new_tab is not a tab graph with a non-null subgraph.
	       */
	      if(    new_tab->any.next    )
		{
		  /* Try the next node at this level.
		   */
		  new_tab = new_tab->any.next ;
		}
	      else
		{
		  /* The next node is null, so move up out of this
		   * subgraph.  Keep going up until a non-null "next"
		   * is found, but stop and fail if going up above
		   * the cur_node if action is XmTRAVERSE_CURRENT.
		   */
		  XmTraversalNode top_node = new_tab ;

		  while(    (new_tab = (XmTraversalNode) new_tab
 			                                 ->any.tab_parent.link)
			&&  (    (action != XmTRAVERSE_CURRENT)
			     ||  (new_tab != cur_node))
			&&  !(new_tab->any.next)    )
		    {
		      top_node = new_tab ;
		    }
		  if(    (action == XmTRAVERSE_CURRENT)
		     &&  (new_tab == cur_node)    )
		    {
		      return NULL ;
		    }
		  if(    !new_tab    )
		    {
		      /* Got to the top level of the traversal graph
		       * without finding a non-null "next", so start
		       * the cycle over; "take it from the top".
		       */
		      new_tab = top_node ;
		    }
		  else
		    {
		      /* The above tests assure that new_tab->any.next
		       * is non-null.
		       */
		      new_tab = new_tab->any.next ;
		    }
		}
	    }
	  break ;
	case XmTRAVERSE_PREV_TAB_GROUP:
	  /* Same structure as for XmTRAVERSE_NEXT_TAB_GROUP,
	   * except the reverse direction.  Also, don't have
	   * to worry about XmTRAVERSE_CURRENT, which behaves
	   * like XmTRAVERSE_NEXT_TAB_GROUP for a specific subgraph.
	   */
	  if(    (new_tab->any.type == XmTAB_GRAPH_NODE)
	     &&  (new_tab->graph.sub_tail)    )
	    {
	      new_tab = new_tab->graph.sub_tail ;
	    }
	  else
	    {
	      if(    new_tab->any.prev    )
		{
		  new_tab = new_tab->any.prev ;
		}
	      else
		{
		  XmTraversalNode top_node = new_tab ;

		  while(    (new_tab = (XmTraversalNode) new_tab
			                                 ->any.tab_parent.link)
			&&  !(new_tab->any.prev)    )
		    {
		      top_node = new_tab ;
		    }
		  if(    !new_tab    )
		    {
		      new_tab = top_node ;
		    }
		  else
		    {
		      new_tab = new_tab->any.prev ;
		    }
		}
	    }
	  break ;
	}
      if(    new_tab == cur_node    )
	{
	  /* This is how we get out when there are no traversable
	   * nodes in this traversal graph.  We have traversed
	   * the entire graph and have come back to the starting
	   * point, so fail.
	   */
	  return NULL ;
	}
      if(    new_tab->any.type == XmCONTROL_GRAPH_NODE    )
	{
	  /* While tabbing, the only way to know whether a control graph
	   * is traversable is to attempt to traverse to it; can't depend
	   * on the ensuing NodeIsTraversable().
	   */
	  XmTraversalNode rtnNode = TraverseControl( new_tab, action) ;
	  if(    rtnNode    )
	    {
	      return rtnNode ;
	    }
	}
    } while(    !NodeIsTraversable( new_tab)    ) ;

  return new_tab ;
}

void 
#ifdef _NO_PROTO
_XmFreeTravGraph( trav_list )
        XmTravGraph trav_list ;
#else
_XmFreeTravGraph(
        XmTravGraph trav_list )
#endif /* _NO_PROTO */
{
  if(    trav_list->num_alloc    )
    {
      XtFree( (char *) trav_list->head) ;
      trav_list->head = NULL ;
      trav_list->next_alloc = trav_list->num_alloc ;
      trav_list->num_alloc = 0 ;
      trav_list->num_entries = 0 ;
      trav_list->current = NULL ;
      trav_list->top = NULL ;
    }
}

void 
#ifdef _NO_PROTO
_XmTravGraphRemove( tgraph, wid )
        XmTravGraph tgraph ;
        Widget wid ;
#else
_XmTravGraphRemove(
        XmTravGraph tgraph,
        Widget wid )
#endif /* _NO_PROTO */
{
  XmTraversalNode node ;
  if(    tgraph->num_entries    )
    {
      while(    (node = GetNodeOfWidget( tgraph, wid)) != NULL    )
	{
	  node->any.widget = NULL ;
	}
    }
  return ;
}

void 
#ifdef _NO_PROTO
_XmTravGraphAdd( tgraph, wid )
        XmTravGraph tgraph ;
        Widget wid ;
#else
_XmTravGraphAdd(
        XmTravGraph tgraph,
        Widget wid )
#endif /* _NO_PROTO */
{
  if(    tgraph->num_entries    )
    {
      if(    !GetNodeOfWidget( tgraph, wid)    )
	{
	  _XmFreeTravGraph( tgraph) ;
	}
    }
  return ;
}

void 
#ifdef _NO_PROTO
_XmTravGraphUpdate( tgraph, wid )
        XmTravGraph tgraph ;
        Widget wid ;
#else
_XmTravGraphUpdate(
        XmTravGraph tgraph,
        Widget wid )
#endif /* _NO_PROTO */
{
  _XmFreeTravGraph( tgraph) ;
  return ;
}

Boolean 
#ifdef _NO_PROTO
_XmNewTravGraph( trav_list, top_wid, init_current )
        XmTravGraph trav_list ;
        Widget top_wid ;
        Widget init_current ;
#else
_XmNewTravGraph(
        XmTravGraph trav_list,
        Widget top_wid,
        Widget init_current )
#endif /* _NO_PROTO */
{
  /* This procedure assumes that trav_list has been zeroed.
   * before the initial call to _XmNewTravGraph.  Subsequent
   * calls to _XmFreeTravGraph and _XmNewTravGraph do not
   * require re-initialization.
   */
  XRectangle w_rect ;

  if(    top_wid    )
    {
      trav_list->top = top_wid ;
    }
  else
    {
      if(    !(trav_list->top)    )
	{
	  top_wid = init_current ;
	  while(    top_wid  &&  !XtIsShell( top_wid)    )
	    {
	      top_wid = XtParent( top_wid) ;
	    }
	  trav_list->top = top_wid ;
	}
    }
  if(    !trav_list->top
     ||  trav_list->top->core.being_destroyed    )
    {
      _XmFreeTravGraph( trav_list) ;
      return FALSE ;
    }
  trav_list->num_entries = 0 ;
  trav_list->current = NULL ;

  /* Traversal topography uses a coordinate system which is relative
   * to the shell window (avoids toolkit/window manager involvement).
   * Initialize x and y to cancel to zero when shell coordinates are
   * added in.
   */
  w_rect.x = - (Position) (XtX( top_wid) + XtBorderWidth( top_wid)) ;
  w_rect.y = - (Position) (XtY( top_wid) + XtBorderWidth( top_wid)) ;
  w_rect.width = XtWidth( top_wid) ;
  w_rect.height = XtHeight( top_wid) ;

  GetNodeList( top_wid, &w_rect, trav_list, -1, -1) ;

  if(    (trav_list->num_entries + XmTRAV_LIST_ALLOC_INCREMENT) <
                                                      trav_list->num_alloc    )
    {
      /* Except for the very first allocation of the traversal graph for
       * this shell hierarchy, the size of the initial allocation is based
       * on the number of nodes allocated for the preceding version of the
       * graph (as per the next_alloc field of the XmTravGraph).  To prevent
       * a "grow-only" behavior in the size of the allocation, we must
       * routinely attempt to prune excessive memory allocation.
       */
      trav_list->num_alloc -= XmTRAV_LIST_ALLOC_INCREMENT ;
      trav_list->head = (XmTraversalNode) XtRealloc( (char *) trav_list->head,
		          trav_list->num_alloc * sizeof( XmTraversalNodeRec)) ;
    }
  LinkNodeList( trav_list) ;

  SortNodeList( trav_list) ;

  SetInitialWidgets( trav_list) ;

  InitializeCurrent( trav_list, init_current, FALSE) ;

#ifdef DEBUG_PRINT
  PrintNodeList( trav_list) ;
#endif

  return TRUE ;
}


#define UnallocLastListEntry( list) (--(list->num_entries))

static XmTraversalNode 
#ifdef _NO_PROTO
AllocListEntry( list )
        XmTravGraph list ;
#else
AllocListEntry(
        XmTravGraph list )
#endif /* _NO_PROTO */
{
  if(    !(list->num_alloc)    )
    {
      /* Use next_alloc (from previous allocation of list)
       * as starting point for size of array.
       */
      if(    list->next_alloc    )
	{
	  list->num_alloc = list->next_alloc ;
	}
      else
	{
	  list->num_alloc = XmTRAV_LIST_ALLOC_INCREMENT ;
	}
      list->head = (XmTraversalNode) XtMalloc( 
            		       list->num_alloc * sizeof( XmTraversalNodeRec)) ;
    }
  else
    {
      if(    list->num_entries == list->num_alloc    )
	{   
	  list->num_alloc += XmTRAV_LIST_ALLOC_INCREMENT ;

	  list->head = (XmTraversalNode) XtRealloc( (char *) list->head,
		               list->num_alloc * sizeof( XmTraversalNodeRec)) ;
	} 
    }
  return &(list->head[list->num_entries++]) ;
}

static Boolean 
#ifdef _NO_PROTO
GetChildList( composite, widget_list, num_in_list )
        Widget composite ;
        Widget **widget_list ;
        Cardinal *num_in_list ;
#else
GetChildList(
        Widget composite,
        Widget **widget_list,
        Cardinal *num_in_list )
#endif /* _NO_PROTO */
{
  XmManagerClassExt *mext ;

  if(    XmIsManager( composite)
     &&  (mext = (XmManagerClassExt *)
                             _XmGetClassExtensionPtr( (XmGenericClassExt *)
			         &(((XmManagerWidgetClass) XtClass( composite))
				       ->manager_class.extension), NULLQUARK))
     &&  *mext
     &&  (*mext)->traversal_children    )
    {
      return (*((*mext)->traversal_children))( composite, widget_list,
					                         num_in_list) ;
    }
  return FALSE ;
}

static void 
#ifdef _NO_PROTO
GetNodeList( wid, parent_rect, trav_list, tab_parent, control_parent )
        Widget wid ;
        XRectangle *parent_rect ;
        XmTravGraph trav_list ;
        int tab_parent ;
        int control_parent ;
#else
GetNodeList(
        Widget wid,
        XRectangle *parent_rect,
        XmTravGraph trav_list,
        int tab_parent,
        int control_parent )
#endif /* _NO_PROTO */
{   
  /* This routine returns a node list of all navigable widgets in
   * a sub-hierarchy.  The order of the list is such that the node
   * of widget's parent is guaranteed to precede the child widget's
   * node.
   */
  /* This routine fills in the following fields of the
   * traversal nodes by this routine:
   *   any.type
   *   any.widget
   *   any.nav_type
   *   any.rect
   *   any.tab_parent (initialized by offset)
   *   graph.sub_head (set to NULL)
   *   graph.sub_tail (set to NULL)
   */
  XmTraversalNode list_entry ;
  int list_entry_offset ;
  XmNavigability node_type ;

  if(    wid->core.being_destroyed
     ||  (    !(node_type = _XmGetNavigability( wid))
	  &&  !XtIsShell( wid))   )
    {
      return ;
    }
  list_entry_offset = (int) trav_list->num_entries ;
  list_entry = AllocListEntry( trav_list) ;

  list_entry->any.widget = wid ;
  list_entry->any.rect.x = parent_rect->x + XtX( wid) + XtBorderWidth( wid) ;
  list_entry->any.rect.y = parent_rect->y + XtY( wid) + XtBorderWidth( wid) ;
  list_entry->any.rect.width = XtWidth( wid) ;
  list_entry->any.rect.height = XtHeight( wid) ;
  list_entry->any.nav_type = list_entry_offset ? _XmGetNavigationType( wid)
                       : XmSTICKY_TAB_GROUP ; /* Bootstrap; first entry must */
			                      /*   behave like a tab group.  */
  if(    node_type == XmCONTROL_NAVIGABLE    )
    {
      list_entry->any.type = XmCONTROL_NODE ;
      list_entry->any.tab_parent.offset = control_parent ;
    }
  else
    {
      if(    node_type == XmTAB_NAVIGABLE    )
	{
	  list_entry->any.type = XmTAB_NODE ;
	  list_entry->any.tab_parent.offset = tab_parent ;
	}
      else
	{
	  if(    (    (node_type == XmNOT_NAVIGABLE)
		  &&  list_entry_offset)
	     ||  !XtIsComposite( wid)    )
	    {
	      UnallocLastListEntry( trav_list) ;
	    }
	  else
	    {
	      /* Node type is XmDESCENDANTS_NAVIGABLE
	       *  or XmDESCENDANTS_TAB_NAVIGABLE.
	       */
	      int controls_graph_offset ;
	      XmTraversalNode controls_graph ;
	      int i ;
	      Widget *trav_children ;
	      Cardinal num_trav_children ;
	      Boolean free_child_list ;
	      XRectangle tmp_rect ;
	      XRectangle *list_entry_rect = &tmp_rect;

	      tmp_rect  = list_entry->any.rect ;

	      if(    node_type == XmDESCENDANTS_NAVIGABLE    )
		{
		  /* This Composite has no purpose in the traversal
		   * graph, so use the passed-in parents and a
		   * temporary copy of this widget's rectangle to
		   * avoid the need for the memory of this list entry.
		   */
		  controls_graph_offset = control_parent ;
		  list_entry_offset = tab_parent ;
		  tmp_rect = *list_entry_rect ;
		  list_entry_rect = &tmp_rect ;
		  UnallocLastListEntry( trav_list) ;
		}
	      else /* node_type == XmDESCENDANTS_TAB_NAVIGABLE */
		{
		  list_entry->any.type = XmTAB_GRAPH_NODE ;
		  list_entry->graph.sub_head = NULL ;
		  list_entry->graph.sub_tail = NULL ;
		  list_entry->any.tab_parent.offset = tab_parent ;

		  controls_graph_offset = list_entry_offset + 1 ;
		  controls_graph = AllocListEntry( trav_list) ;
		  *controls_graph = trav_list->head[list_entry_offset] ;
		  controls_graph->any.tab_parent.offset = list_entry_offset ;
		  controls_graph->any.type = XmCONTROL_GRAPH_NODE ;
		}
	      if(    !(free_child_list = GetChildList( wid, &trav_children,
						      &num_trav_children))    )
		{
		  trav_children = ((CompositeWidget) wid)->composite.children ;
		  num_trav_children = ((CompositeWidget) wid)
	                                             ->composite.num_children ;
		}
	      i = 0 ;
	      while(    i < num_trav_children    )
		{   
		  GetNodeList( trav_children[i], list_entry_rect,
			 trav_list, list_entry_offset, controls_graph_offset) ;
		  ++i ;
		} 
	      if(    free_child_list    )
		{
		  XtFree( (char *) trav_children) ;
		}
	    }
	}
    }
  return ;
} 

static XmTraversalNode 
#ifdef _NO_PROTO
GetNodeFromGraph( graph, wid )
        XmGraphNode graph ;
        Widget wid ;
#else
GetNodeFromGraph(
        XmGraphNode graph,
        Widget wid )
#endif /* _NO_PROTO */
{
  XmTraversalNode node ;
  if(    wid
     &&  (node = graph->sub_head)    )
    {
      do
	{
	  if(    node->any.widget == wid    )
	    {
	      return node ;
	    }
	} while(    (node != graph->sub_tail)
		&&  (node = node->any.next)    ) ;
    }
  return NULL ;
}

static XmTraversalNode 
#ifdef _NO_PROTO
GetNodeOfWidget( trav_list, wid )
        XmTravGraph trav_list ;
        Widget wid ;
#else
GetNodeOfWidget(
        XmTravGraph trav_list,
        Widget wid )
#endif /* _NO_PROTO */
{
  /* This function returns the first node in the list to match the
   * widget argument.
   *
   * Note that tab-group composites will have two nodes in the
   * node list; a tab-graph node and a control-graph node.
   * The first match will always be the tab-graph node and
   * the corresponding control-graph node will always be the
   * next node in the list; many callers of this routine depend
   * on this behavior.
   *
   * Also note that the graph list is maintained such that
   * "obsolete" nodes have the widget id field set to NULL;
   * this ensures that this routine will never return a node
   * which is not part of the current linked traversal graph.
   */
  if(    wid    )
    {
      unsigned cnt = 0 ;
      XmTraversalNode list_ptr = trav_list->head ;
      
      while(    cnt++ < trav_list->num_entries    )
	{
	  if(    list_ptr->any.widget == wid    )
	    {
	      return list_ptr ;
	    }
	  ++list_ptr ;
	}
    }
  return NULL ;
}

static void 
#ifdef _NO_PROTO
LinkNodeList( list )
        XmTravGraph list ;
#else
LinkNodeList(
        XmTravGraph list )
#endif /* _NO_PROTO */
{
  XmTraversalNode head = list->head ;
  XmTraversalNode entry = head ;
  unsigned cnt = 0 ;

  while(    cnt++ < list->num_entries    )
    {
      XmGraphNode parent ;

      if(    entry->any.tab_parent.offset < 0    )
	{
	  parent = NULL ;
	}
      else
	{
	  parent = (XmGraphNode) &(head[entry->any.tab_parent.offset]) ;
	}
      entry->any.tab_parent.link = parent ;
      if(    parent    )
	{
	  if(    !parent->sub_tail    )
	    {
	      parent->sub_head = entry ;
	    }
	  else
	    {
	      parent->sub_tail->any.next = entry ;
	    }
	  entry->any.prev = parent->sub_tail ;
	  entry->any.next = NULL ;
	  parent->sub_tail = entry ;
	}
      else
	{
	  entry->any.prev = NULL ;
	  entry->any.next = NULL ;
	}
      ++entry ;
    }
  return ;
}

/*
 * CompareNodesHoriz() and CompareNodesVert() are not transitive;
 * the fudge factor used to distinguish distinct rows and columns
 * (the first two tests in each routine) only work properly for
 * adjacent nodes.  Passing a non-transitive comparison method to
 * qsort causes some implementations to dump core, and by definition
 * causes all implementations to return non-intuitive results.
 *
 * Determining the "right" order is a matter of some debate, so
 * we'll just treat the symptoms for now and cure the disease later.
 * This sort at least won't crash.
 */
static void 
#ifdef _NO_PROTO
Sort(base, n_mem, size, compare)
     char *base;
     size_t n_mem;
     size_t size;
     int (*compare)();
#else
Sort(void *base, 
     size_t n_mem, 
     size_t size,
     int (*compare)(XmConst void *, XmConst void *))
#endif /* _NO_PROTO */
{
  register char *ptr;
  register int index, limit;
  char tmp[64];

  /* Currently this routine is only used to sort XmTraversalNodes, */
  /* which are just pointers. */
  /* assert(sizeof(tmp) >= size); */

  /* Perform a simple bubble sort. */
  for (limit = n_mem; limit > 0; limit--)
    for (index = 1, ptr = (char*) base;
	 index < limit; 
	 index++, ptr += size)
      {
	if (compare(ptr, ptr + size) > 0)
	  {
	    memcpy(tmp, ptr, size);
	    memcpy(ptr, ptr + size, size);
	    memcpy(ptr + size, tmp, size);
	  }
      }
}

static int 
#ifdef _NO_PROTO
CompareExclusive( A, B )
        XmConst void *A ;
        XmConst void *B ;
#else
CompareExclusive(
        XmConst void *A,
        XmConst void *B )
#endif /* _NO_PROTO */
{
  XmConst XmTraversalNode nodeA = *((XmTraversalNode *) A) ;
  XmConst XmTraversalNode nodeB = *((XmTraversalNode *) B) ;
  int PositionA = SearchTabList( SortReferenceGraph, nodeA->any.widget) ;
  int PositionB = SearchTabList( SortReferenceGraph, nodeB->any.widget) ;

  if(    PositionA < PositionB    )
    {
      return -1 ;
    }
  if(    PositionA > PositionB    )
    {
      return 1 ;
    }
  return 0 ;  
}

static int 
#ifdef _NO_PROTO
CompareNodesHoriz( A, B )
        XmConst void *A ;
        XmConst void *B ;
#else
CompareNodesHoriz(
        XmConst void *A,
        XmConst void *B )
#endif /* _NO_PROTO */
{
  register XmConst XmTraversalNode nodeA = *((XmTraversalNode *) A) ;
  register XmConst XmTraversalNode nodeB = *((XmTraversalNode *) B) ;
  Dimension Acent = nodeA->any.rect.y + ((nodeA->any.rect.height) >> 1) ;
  Dimension Bcent = nodeB->any.rect.y + ((nodeB->any.rect.height) >> 1) ;

  if(    ((Dimension)(nodeA->any.rect.y + nodeA->any.rect.height) < Bcent)
     &&  (Acent < (Dimension)nodeB->any.rect.y)    )
    {
      return -1 ;
    }
  if(    ((nodeB->any.rect.y + nodeB->any.rect.height) < Acent)
     &&  (Bcent < (Dimension)nodeA->any.rect.y)    )
    {
      return 1 ;
    }
  if(    nodeA->any.rect.x != nodeB->any.rect.x    )
    {
      return (nodeA->any.rect.x < nodeB->any.rect.x) ? -1 : 1 ;
    }
  if(    nodeA->any.rect.y != nodeB->any.rect.y    )
    {
      return (nodeA->any.rect.y < nodeB->any.rect.y) ? -1 : 1 ;
    }
  if(    nodeA->any.rect.height != nodeB->any.rect.height    )
    {
      return (nodeA->any.rect.height < nodeB->any.rect.height) ? -1 : 1 ;
    }
  if(    nodeA->any.rect.width != nodeB->any.rect.width    )
    {
      return (nodeA->any.rect.width < nodeB->any.rect.width) ? -1 : 1 ;	      
    }
  return 0 ;  
}

static int 
#ifdef _NO_PROTO
CompareNodesVert( A, B )
        XmConst void *A ;
        XmConst void *B ;
#else
CompareNodesVert(
        XmConst void *A,
        XmConst void *B )
#endif /* _NO_PROTO */
{
  register XmConst XmTraversalNode nodeA = *((XmTraversalNode *) A) ;
  register XmConst XmTraversalNode nodeB = *((XmTraversalNode *) B) ;
  Dimension Acent = nodeA->any.rect.x + ((nodeA->any.rect.width) >> 1) ;
  Dimension Bcent = nodeB->any.rect.x + ((nodeB->any.rect.width) >> 1) ;
    
  if(    ((Dimension)(nodeA->any.rect.x + nodeA->any.rect.width) < Bcent)
     &&  (Acent < (Dimension)nodeB->any.rect.x)    )
    {
      return -1 ;
    }
  if(    ((nodeB->any.rect.x + nodeB->any.rect.width) < Acent)
     &&  (Bcent < (Dimension)nodeA->any.rect.x)    )
    {
      return 1 ;
    }
  if(    nodeA->any.rect.y != nodeB->any.rect.y    )
    {
      return (nodeA->any.rect.y < nodeB->any.rect.y) ? -1 : 1 ;
    }
  if(    nodeA->any.rect.x != nodeB->any.rect.x    )
    {
      return (nodeA->any.rect.x < nodeB->any.rect.x) ? -1 : 1 ;
    }
  if(    nodeA->any.rect.width != nodeB->any.rect.width    )
    {
      return (nodeA->any.rect.width < nodeB->any.rect.width) ? -1 : 1 ;	      
    }
  if(    nodeA->any.rect.height != nodeB->any.rect.height    )
    {
      return (nodeA->any.rect.height < nodeB->any.rect.height) ? -1 : 1 ;
    }
  return 0 ;  
}

static void 
#ifdef _NO_PROTO
SortGraph( graph, exclusive )
        XmGraphNode graph ;
        Boolean exclusive ;
#else
SortGraph(
        XmGraphNode graph,
#if NeedWidePrototypes
        int exclusive )
#else
        Boolean exclusive )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
  XmTraversalNode head = graph->sub_head ;

  if(    head    )
    {
      XmTraversalNode node = head ;
      XmTraversalNode *list_ptr ;
      unsigned num_nodes = 1 ;
      unsigned idx ;
      XmTraversalNode *node_list ;
      XmTraversalNode storage[STACK_SORT_LIMIT] ;

      while(    (node = node->any.next) != NULL    )
	{
	  ++num_nodes ;
	}
      if(    num_nodes > STACK_SORT_LIMIT    )
	{
	  node_list = (XmTraversalNode *) XtMalloc( num_nodes
						 * sizeof( XmTraversalNode)) ;
	}
      else
	{
	  node_list = storage ;
	}
      /* Now copy nodes onto node list for sorting.
       */
      node = head ;
      list_ptr = node_list ;
      do
	{
	  *list_ptr++ = node ;
	} while(    (node = node->any.next) != NULL    ) ;

      if(    graph->any.type == XmTAB_GRAPH_NODE    )
	{
	  if(    num_nodes > 1    )
	    {
	      if(    exclusive    )
		{
		  qsort( node_list, num_nodes,
			          sizeof( XmTraversalNode), CompareExclusive) ;
		}
	      else
		{
		  /* For non-exclusive tab traversal, leave the control
		   * subgraph at the beginning of the list, since it
		   * always gets first traversal.
		   */
		  Sort( (node_list + 1), (num_nodes - 1),
                                 sizeof( XmTraversalNode), CompareNodesHoriz) ;
		}
	    }
	}
      else /* graph->any.type == XmCONTROL_GRAPH_NODE */
	{
	  if(    !exclusive || (graph->any.nav_type == XmSTICKY_TAB_GROUP)    )
	    {
	      Sort( node_list, num_nodes, sizeof( XmTraversalNode),
	                                                   CompareNodesHoriz) ;
	    }
	}
      list_ptr = node_list ;
      graph->sub_head = *list_ptr ;
      (*list_ptr)->any.prev = NULL ;

      idx = 0 ;
      while(    ++idx < num_nodes    )
	{
	  /* This loop does one less than the number of nodes.
	   */
	  (*list_ptr)->any.next = *(list_ptr + 1) ;
	  ++list_ptr ;
	  (*list_ptr)->any.prev = *(list_ptr - 1) ;
	}
      (*list_ptr)->any.next = NULL ;
      graph->sub_tail = *list_ptr ;

      if(    graph->any.type == XmCONTROL_GRAPH_NODE    )
	{
	  /* Add links for wrapping behavior of control nodes.
	   */
	  graph->sub_head->any.prev = graph->sub_tail ;
	  graph->sub_tail->any.next = graph->sub_head ;

	  /* Now use current list to sort again, this time for
	   * up and down.
	   */
	  if(    !exclusive || (graph->any.nav_type == XmSTICKY_TAB_GROUP)    )
	    {
	      Sort( node_list, num_nodes, sizeof( XmTraversalNode),
	                                                    CompareNodesVert) ;
	    }
	  list_ptr = node_list ;
	  (*list_ptr)->control.up = list_ptr[num_nodes - 1] ;

	  idx = 0 ;
	  while(    ++idx < num_nodes    )
	    {
	      /* This loop does one less than the number of nodes.
	       */
	      (*list_ptr)->control.down = *(list_ptr + 1) ;
	      ++list_ptr ;
	      (*list_ptr)->control.up = *(list_ptr - 1) ;
	    }
	  (*list_ptr)->control.down = *node_list ;
	}

      if(    num_nodes > STACK_SORT_LIMIT    )
	{
	  XtFree( (char *) node_list) ;
	}
    }
  return ;
}

static void 
#ifdef _NO_PROTO
SortNodeList( trav_list )
        XmTravGraph trav_list ;
#else
SortNodeList(
        XmTravGraph trav_list )
#endif /* _NO_PROTO */
{
  XmTraversalNode ptr = trav_list->head ;  
  unsigned cnt = 0 ;

  SortReferenceGraph = trav_list ; /* Slime for CompareExclusive(). */

  while(    cnt++ < trav_list->num_entries    )
    {
      if(    (ptr->any.type == XmTAB_GRAPH_NODE)
	 ||  (ptr->any.type == XmCONTROL_GRAPH_NODE)    )
	 {
	   SortGraph( (XmGraphNode) ptr, !!(trav_list->exclusive)) ;
	 }
      ++ptr ;
    }
}

Boolean
#ifdef _NO_PROTO
_XmSetInitialOfTabGraph( trav_graph, tab_group, init_focus)
	XmTravGraph trav_graph ;
	Widget tab_group ;
	Widget init_focus ;
#else
_XmSetInitialOfTabGraph(
	XmTravGraph trav_graph,
	Widget tab_group,
	Widget init_focus)
#endif /* _NO_PROTO */
{
  XmTraversalNode tab_node = GetNodeOfWidget( trav_graph, tab_group) ;
  XmGraphNode control_graph_node ;

  if(    tab_node
     &&  (    (tab_node->any.type == XmTAB_GRAPH_NODE)
	  ||  (tab_node->any.type == XmCONTROL_GRAPH_NODE))    )
    {
      if(    SetInitialNode( (XmGraphNode) tab_node,
		         GetNodeFromGraph( (XmGraphNode) tab_node, init_focus))
	 ||  (    (control_graph_node = (XmGraphNode) GetNodeFromGraph(
				            (XmGraphNode) tab_node, tab_group))
	      &&  SetInitialNode( control_graph_node,
		             GetNodeFromGraph( control_graph_node, init_focus))
	      &&  SetInitialNode( (XmGraphNode) tab_node,
			            (XmTraversalNode) control_graph_node))    )
	{
	  return TRUE ;
	}
    }
  return FALSE ;
}

static Boolean
#ifdef _NO_PROTO
SetInitialNode( graph, init_node)
	XmGraphNode graph ;
	XmTraversalNode init_node ;
#else
SetInitialNode(
	XmGraphNode graph,
	XmTraversalNode init_node)
#endif /* _NO_PROTO */
{
  /* It is presumed that init_node was derived from a call
   * to GetNodeFromGraph with the same "graph" argument.
   * It is a bug if the parent of init_node is not graph.
   */
  if(    init_node    )
    {
      if(    init_node != graph->sub_head    )
	{
	  if(    graph->any.type == XmTAB_GRAPH_NODE    )
	    {
	      graph->sub_tail->any.next = graph->sub_head ;
	      graph->sub_head->any.prev = graph->sub_tail ;
	      graph->sub_head = init_node ;
	      graph->sub_tail = init_node->any.prev ;
	      graph->sub_tail->any.next = NULL ;
	      init_node->any.prev = NULL ;
	    }
	  else
	    {
	      graph->sub_head = init_node ;
	      graph->sub_tail = init_node->any.prev ;
	    }
	}
      return TRUE ;
    }
  return FALSE ;
}

static void 
#ifdef _NO_PROTO
SetInitialWidgets( trav_list )
        XmTravGraph trav_list ;
#else
SetInitialWidgets(
        XmTravGraph trav_list )
#endif /* _NO_PROTO */
{
  Widget init_focus ;
  XmTraversalNode ptr = trav_list->head ;  
  XmTraversalNode init_node ;
  unsigned cnt = 0 ;

  while(    cnt < trav_list->num_entries    )
    {
      if(    (    (ptr->any.type == XmTAB_GRAPH_NODE)
	      ||  (ptr->any.type == XmCONTROL_GRAPH_NODE))
	 &&  ptr->graph.sub_head    )
	{
	  if(    ptr->any.widget
	     &&  XmIsManager( ptr->any.widget)
	     &&  (init_focus = ((XmManagerWidget) ptr->any.widget)
	                                          ->manager.initial_focus)
	     &&  (init_node = GetNodeFromGraph( (XmGraphNode) ptr,
 					                      init_focus))    )
	    {
	      SetInitialNode( (XmGraphNode) ptr, init_node) ;
	    }
	  else
	    {
	      if(    ptr->any.type == XmTAB_GRAPH_NODE    )
		{
		  /* A tab graph node is always followed immediately
		   * by its corresponding control graph.
		   *
		   * Make sure that the control graph is the first
		   * tab node of every tab graph (unless specified
		   * otherwise).
		   */
		  SetInitialNode( (XmGraphNode) ptr, (ptr + 1)) ;
		}
	    }
	}
      ++ptr ;
      ++cnt ;
    }
}

static void 
#ifdef _NO_PROTO
GetRectRelativeToShell( wid, rect )
        Widget wid ;
        XRectangle *rect ;
#else
GetRectRelativeToShell(
        Widget wid,
        XRectangle *rect)
#endif /* _NO_PROTO */
{
  /* This routine returns the rectangle of the widget relative to its
   * nearest shell ancestor.
   */
  Position x = 0 ;
  Position y = 0 ;

  rect->width = XtWidth( wid) ;
  rect->height = XtHeight( wid) ;
  do
    {
      x += XtX( wid) + XtBorderWidth( wid) ;
      y += XtY( wid) + XtBorderWidth( wid) ;

    } while(    (wid = XtParent( wid))  &&  !XtIsShell( wid)    ) ;
  rect->x = x ;
  rect->y = y ;
}

void
#ifdef _NO_PROTO
_XmTabListAdd( graph, wid )
	XmTravGraph graph ;
	Widget wid ;
#else
_XmTabListAdd(
	XmTravGraph graph,
	Widget wid)
#endif /* _NO_PROTO */
{
  if(    SearchTabList( graph, wid) < 0    )
    {
      if(    !(graph->tab_list_alloc)    )
	{
	  Widget shell = _XmFindTopMostShell( wid) ;

	  graph->tab_list_alloc = XmTAB_LIST_ALLOC_INCREMENT ;
	  graph->excl_tab_list = (Widget *) XtMalloc(
                                     graph->tab_list_alloc * sizeof( Widget)) ;
	  graph->excl_tab_list[graph->num_tab_list++] = shell ;
	}
      if(    graph->num_tab_list >= graph->tab_list_alloc    )
	{
	  graph->tab_list_alloc += XmTAB_LIST_ALLOC_INCREMENT ;
	  graph->excl_tab_list = (Widget *) XtRealloc(
				                 (char *) graph->excl_tab_list,
                                      graph->tab_list_alloc * sizeof( Widget));
	}
      graph->excl_tab_list[graph->num_tab_list++] = wid ;
    }
}

void
#ifdef _NO_PROTO
_XmTabListDelete( graph, wid )
	XmTravGraph graph ;
	Widget wid ;
#else
_XmTabListDelete(
	XmTravGraph graph,
	Widget wid)
#endif /* _NO_PROTO */
{
  DeleteFromTabList( graph, SearchTabList( graph, wid)) ;

  if(    (graph->num_tab_list + XmTAB_LIST_ALLOC_INCREMENT)
                                                   < graph->tab_list_alloc    )
    {
      graph->tab_list_alloc -= XmTAB_LIST_ALLOC_INCREMENT ;
      graph->excl_tab_list = (Widget *) XtRealloc(
					         (char *) graph->excl_tab_list,
                                      graph->tab_list_alloc * sizeof( Widget));
    }
}

static int
#ifdef _NO_PROTO
SearchTabList( graph, wid )
	XmTravGraph graph ;
	Widget wid ;
#else
SearchTabList(
	XmTravGraph graph,
	Widget wid)
#endif /* _NO_PROTO */
{
  int i = 0 ;
  while(    i < graph->num_tab_list    )
    {
      if(    graph->excl_tab_list[i] == wid    )
	{
	  return i ;
	}
      ++i ;
    }
  return -1 ;
}

static void
#ifdef _NO_PROTO
DeleteFromTabList( graph, indx )
	XmTravGraph graph ;
	int indx ;
#else
DeleteFromTabList(
	XmTravGraph graph,
	int indx)
#endif /* _NO_PROTO */
{
  if(    indx >= 0    )
    {
      while(    indx < graph->num_tab_list    )
	{
	  graph->excl_tab_list[indx] = graph->excl_tab_list[indx+1] ;
	  ++indx ;
	}
      --(graph->num_tab_list) ;
    }
}


#ifdef CDE_TAB
Boolean
#ifdef _NO_PROTO
_XmTraverseWillWrap(w, dir)
	Widget w;
	XmTraversalDirection dir;
#else
_XmTraverseWillWrap (
	Widget w,
	XmTraversalDirection dir)
#endif /* _NO_PROTO */
{
  XmFocusData focusData = _XmGetFocusData(w);
  XmTravGraph graph;
  XmTraversalNode cur_node, new_ctl;

  if (!focusData)
    return(True);
  graph = &(focusData->trav_graph);
  cur_node = GetNodeOfWidget(graph, w);

  if(!cur_node)  {
      /*  This code initializes the traversal graph if necessary.  See
          _XmTraverse() */
      if (!graph->num_entries) {
          if (!_XmNewTravGraph( graph, graph->top, w))
	      return True ;
      } else {
          if (!InitializeCurrent( graph, w, True))
	      return True ;
      }
      cur_node = GetNodeOfWidget( graph, w) ;
      if (!cur_node)
          return(False);
  }
  if( cur_node->any.type == XmCONTROL_GRAPH_NODE ) {
      /*  This is only true on composites  */
      cur_node = cur_node->graph.sub_head ;
      if(!cur_node)
	  return True ;
    }
  else {
      if(cur_node->any.type != XmCONTROL_NODE)
	  return True ;
    }
  new_ctl = cur_node ;
  do {
      switch(dir) {
	case XmTRAVERSE_NEXT:
	/*  The action will cause wraping when the new_ctrl matches the
            last item on the tab_parent's list.  */
	  if (new_ctl == new_ctl->any.tab_parent.link->sub_head->any.prev)
      		return(True);
	  else
	        new_ctl = new_ctl->any.next ;
	  break ;
	case XmTRAVERSE_PREV:
	/*  Moving left will cause wrapping when the new control is the
	    first item in the tab group.  */
	  if (new_ctl == new_ctl->any.tab_parent.link->sub_head)
      		return(True);
	  else
	        new_ctl = new_ctl->any.prev ;
	  break ;
        default:
                return(False);
          break;
	}
    } while(    new_ctl
	    && !NodeIsTraversable( new_ctl)
	    && (    (new_ctl != cur_node)
		||  (new_ctl = NULL))    ) ;
  /*  If it didn't cause wrapping and return in the cases above, then
      return False.  */
  return False ;
}
#endif  /* CDE_TAB */


#ifdef DEBUG_PRINT

#define WNAME( node) (node->any.widget->core.name)
#define CNAME( node) (node->any.widget->core.widget_class->core_class.class_name)

static void
#ifdef _NO_PROTO
PrintControl( node )
	XmTraversalNode node ;
#else
PrintControl(
	XmTraversalNode node )
#endif /* _NO_PROTO */
{
  printf( "control: %X %s, x: %d, y: %d, parent: %X, next: %X %s, prev: %X %s \n",
	 node, CNAME( node), node->any.rect.x, node->any.rect.y,
	 node->any.tab_parent.link,
	 node->any.next,
	 node->any.next ? CNAME( node->any.next) : "null",
	 node->any.prev,
	 node->any.prev ? CNAME( node->any.prev) : "null") ;
}

static void
#ifdef _NO_PROTO
PrintTab( node )
	XmTraversalNode node ;
#else
PrintTab(
	XmTraversalNode node )
#endif /* _NO_PROTO */
{
  printf( "tab: %X %s, x: %d, y: %d, parent: %X, next: %X %s, prev: %X %s \n",
	 node, CNAME( node), node->any.rect.x, node->any.rect.y,
	 node->any.tab_parent.link,
	 node->any.next,
	 node->any.next ? CNAME( node->any.next) : "null",
	 node->any.prev,
	 node->any.prev ? CNAME( node->any.prev) : "null") ;
}

static void
#ifdef _NO_PROTO
PrintGraph( node )
	XmTraversalNode node ;
#else
PrintGraph(
	XmTraversalNode node )
#endif /* _NO_PROTO */
{
  char *gr_str ;
  if(    node->any.type == XmCONTROL_GRAPH_NODE    )
    {
      gr_str = "ctl_graph" ;
    }
  else
    {
      gr_str = "tab_graph" ;
    }
  printf( "%s: %X %s, x: %d, y: %d, parent: %X, sub_head: %X %s, sub_tail: %X, next: %X %s, prev: %X %s\n",
	 gr_str,
	 node, CNAME( node), node->any.rect.x, node->any.rect.y,
	 node->any.tab_parent.link,
	 node->graph.sub_head,
	 node->graph.sub_head ? CNAME( node->graph.sub_head) : "null",
	 node->graph.sub_tail,
	 node->any.next,
	 node->any.next ? CNAME( node->any.next) : "null",
	 node->any.prev,
	 node->any.prev ? CNAME( node->any.prev) : "null") ;
}

static void
#ifdef _NO_PROTO
PrintNodeList( list )
	XmTravGraph list ;
#else
PrintNodeList(
	XmTravGraph list )
#endif /* _NO_PROTO */
{
  XmTraversalNode ptr = list->head ;  
  unsigned idx = 0 ;
  while(    idx < list->num_entries    )
    {
      switch(    ptr->any.type    )
	{
	case XmTAB_GRAPH_NODE:
	case XmCONTROL_GRAPH_NODE:
	  PrintGraph( ptr) ;
	  break ;
	case XmTAB_NODE:
	  PrintTab( ptr) ;
	  break ;
	case XmCONTROL_NODE:
	  PrintControl( ptr) ;
	  break ;
	}
      ++ptr ;
      ++idx ;
    }
}

#endif /* DEBUG_PRINT */
