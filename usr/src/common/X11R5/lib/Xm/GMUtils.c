#pragma ident	"@(#)m1.2libs:Xm/GMUtils.c	1.2"
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
/*******************************************************************
 * This file contains the common geometry management set of routines
 * for both pure BulletinBoard (not subclasses) and DrawingArea.
 ****************/


/* Include files:
*/
#include "XmI.h"
#include "GMUtilsI.h"
#include <Xm/ManagerP.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*******************************************************************
 * Figure out how much size we need. Shrink wrap around the children.
 ****************/
void 
#ifdef _NO_PROTO
_XmGMCalcSize(manager, margin_width, margin_height, 
            replyWidth, replyHeight)
XmManagerWidget manager ;
Dimension margin_width ;
Dimension margin_height ;
Dimension *replyWidth ;
Dimension *replyHeight ;
#else
_XmGMCalcSize(XmManagerWidget manager,
#if NeedWidePrototypes
            int margin_width,
            int margin_height,        
#else
            Dimension margin_width,
            Dimension margin_height,        
#endif /* NeedWidePrototypes */
            Dimension *replyWidth,
            Dimension *replyHeight )
#endif /* _NO_PROTO */
{
    register int i ;
    register Widget child ;   
    int right, bottom ;

    *replyWidth = *replyHeight = 0 ;

    for (i = 0; i < manager->composite.num_children; i++) {
        child = manager->composite.children[i];
        if (XtIsManaged (child)) {
          right = XtX(child) + 2*XtBorderWidth(child) + XtWidth(child) ;
            bottom = XtY(child) + 2*XtBorderWidth(child) + XtHeight(child) ;

          if (right > (int) *replyWidth) *replyWidth = right ;
          if (bottom > (int) *replyHeight) *replyHeight = bottom ;
      }
    }

    *replyWidth += margin_width + MGR_ShadowThickness(manager);
    *replyHeight += margin_height + MGR_ShadowThickness(manager);

    if (!(*replyWidth)) *replyWidth = 10;
    if (!(*replyHeight)) *replyHeight = 10;
}


/****************************************************************
 * Calculate the size needed for the Manager
 * Request a change in size from parent if needed.
 ****************/
Boolean 
#ifdef _NO_PROTO
_XmGMDoLayout(manager, margin_width, margin_height, resize_policy, queryonly) 
XmManagerWidget manager ;
Dimension margin_width, margin_height;  
int resize_policy;
Boolean queryonly ;
#else
_XmGMDoLayout(XmManagerWidget manager,
#if NeedWidePrototypes
            int margin_width,
            int margin_height,        
#else
            Dimension margin_width,
            Dimension margin_height,        
#endif /* NeedWidePrototypes */
            int resize_policy,
#if NeedWidePrototypes
            int queryonly
#else
            Boolean queryonly
#endif /* NeedWidePrototypes */
)
#endif /* _NO_PROTO */
{
    /* return:
         True means that the layout is accepted, either with no request
       to the parent, or with a successfull serie of requests. 
       No means that the layout is denied, without request (NONE
       policy for instance) or with request (parent does not accept the
       change in size or proposes something unacceptable).
       Note that when the parent returns almost, since this function returns
       either true or false (see below), we don't handle almost reply 
       to the instigator. 
       The queryonly flag just controls the "reality" of the parent request
       and the call to resize.
    */

    XtWidgetGeometry request ;
    XtWidgetGeometry reply ;

    request.request_mode = CWWidth|CWHeight ;
    if (queryonly) request.request_mode |= XtCWQueryOnly ;

    _XmGMCalcSize(manager, margin_width, margin_height, 
                &request.width, &request.height);
    
    /* no change, just accept it */
    if ((XtWidth(manager) == request.width) && 
      (XtHeight(manager) == request.height)) 
      return (True);   

    /* the current manager sizes are bigger than the new ones and
       we don't want to strink, just accept the layout as is, since
       it already fits */
    if ((resize_policy == XmRESIZE_GROW ||
       resize_policy == XmRESIZE_NONE) &&
      ((XtWidth(manager) >= request.width) && 
       (XtHeight(manager) >= request.height)) )
      return (True);

    /* the previous test passes over, so one of the needed sizes must be 
       bigger than one of the current, and we can't do that with NONE */
    if (resize_policy == XmRESIZE_NONE )
      return (False);

    /* we can't shrink on one side while growing on the other, just
       overwrite the shrinking side */
    if(resize_policy == XmRESIZE_GROW    ) {   
      if(request.width < XtWidth(manager)) 
          request.width = XtWidth(manager) ;
        if(request.height < XtHeight(manager)) 
          request.height = XtHeight(manager) ;
    } 
    
    /* now the request */
    switch(XtMakeGeometryRequest((Widget)manager, 
                               &request,
                               &reply)) {
    case XtGeometryYes:
      if (!queryonly) 
             /* call the resize proc, since the widget set has now
                a Yes policy */
          (* XtCoreProc(manager,resize))  ((Widget)manager) ;
      return(True) ;
    case XtGeometryAlmost:  
         /* The following behavior is based on the shrink wrapper
            behavior of this manager. What we have asked is the minimum.
            If the almost returned size is smaller than the 
            requested one, refuse, since it will always clip something */
      if ((reply.width < request.width) || 
          (reply.height < request.height)) {   
          return( False) ;
      } else {
         /* if almost returned a bigger size than requested, 
            accept it since it cannot hurt the shrink wrap behavior,
            neither the Grow policy (requested always > current).
            Apply it if not queryonly. If queryonly, we don't 
            have to re-request something, since no change is needed 
            and we already know it's OK */
          if (!queryonly) {
              (void) XtMakeResizeRequest((Widget)manager, 
                                        reply.width, 
                                        reply.height, 
                                        NULL, NULL) ;
              (* XtCoreProc(manager,resize))  ((Widget)manager) ;
          }
          return(True) ;
      }
    case XtGeometryNo:
    default:
      break ;
    }
    return( False) ;
}

/* Enforce margins for children if margins are non-zero */
void 
#ifdef _NO_PROTO
_XmGMEnforceMargin(manager, margin_width, margin_height, setvalue)
XmManagerWidget manager ;
Dimension margin_width ;
Dimension margin_height ;
Boolean setvalue ;
#else
_XmGMEnforceMargin(XmManagerWidget manager,
#if NeedWidePrototypes
                 int margin_width,
                 int margin_height, 
                 int setvalue
#else
                 Dimension margin_width,
                 Dimension margin_height,        
                 Boolean setvalue
#endif /* NeedWidePrototypes */
)
#endif /* _NO_PROTO */
{
    int i ;
    register Widget child ;
    register Boolean do_move ;
    Position newx, newy ;
           
    for(i = 0 ; i < manager->composite.num_children ; i++) {   
        do_move = False ;
        child = (Widget) manager->composite.children[i] ;
        if(XtIsManaged (child)) {   
            if ((margin_width != 0) && 
              (XtX(child) < (int) margin_width )) {   
              do_move = True ;
              newx = margin_width ;
          } else 
              newx = XtX(child) ;
          if ((margin_height != 0) && 
              (XtY(child) < (int) margin_height )) {   
              do_move = True ;
                newy = margin_height  ;
          } else
              newy = XtY(child) ;
          if(do_move) {
              if (setvalue) {
                  Arg args[2] ;
                  XtSetArg(args[0], XmNx, newx); 
                  XtSetArg(args[1], XmNy, newy);
                  XtSetValues(child, args, 2);
              } else
              _XmMoveObject(child, newx, newy);
          }
        }
    }
}
             

/****************************************************************
 * Handle query geometry requests for both BB and DA.
 ****************/
XtGeometryResult
#ifdef _NO_PROTO
_XmGMHandleQueryGeometry(widget, intended, desired, 
                  margin_width, margin_height, resize_policy)
Widget  widget ;
XtWidgetGeometry * intended ;
XtWidgetGeometry * desired ;
Dimension  margin_width, margin_height ;
int resize_policy ;
#else
_XmGMHandleQueryGeometry(Widget widget,
                       XtWidgetGeometry * intended,
                       XtWidgetGeometry * desired,
#if NeedWidePrototypes
                       int margin_width,
                       int margin_height,        
#else
                       Dimension margin_width,
                       Dimension margin_height,    
#endif /* NeedWidePrototypes */
                       int resize_policy)
#endif
{
    Dimension width, height ;
    
    /* first determine what is the desired size, using the resize_policy. */
    if (resize_policy == XmRESIZE_NONE) {
	desired->width = XtWidth(widget) ;
	desired->height = XtHeight(widget) ;
    } else {
	if (GMode( intended) & CWWidth) width = intended->width;
	if (GMode( intended) & CWHeight) height = intended->height;
	
	_XmGMCalcSize ((XmManagerWidget)widget, 
		       margin_width, margin_height, &width, &height);
	if ((resize_policy == XmRESIZE_GROW) &&
	    ((width < XtWidth(widget)) ||
	     (height < XtHeight(widget)))) {
	    desired->width = XtWidth(widget) ;
	    desired->height = XtHeight(widget) ;
	} else {
	    desired->width = width ;
	    desired->height = height ;
	}
    }
    
    /* deal with user initial size setting */
    if (!XtIsRealized(widget))  {
	if (XtWidth(widget) != 0) desired->width = XtWidth(widget) ;
	if (XtHeight(widget) != 0) desired->height = XtHeight(widget) ;
    }	    

    return _XmGMReplyToQueryGeometry(widget, intended, desired) ;
}


/****************************************************************
 *
 * _XmGMReplyToQueryGeometry.
 *
 * This is the generic handling of Almost, No and Yes replies
 * based on the intended values and the given desirs.
 *
 * This can be used by any widget that really only care about is
 * width and height dimension. It jsut has to compute its desired size
 * using its own private layout routine and resources before calling
 * this one that will deal with the Xt reply value cuisine.
 *
 ****************/
XtGeometryResult
#ifdef _NO_PROTO
_XmGMReplyToQueryGeometry(widget, intended, desired)
Widget  widget ;
XtWidgetGeometry * intended ;
XtWidgetGeometry * desired ;
#else
_XmGMReplyToQueryGeometry(Widget widget,
                       XtWidgetGeometry * intended,
                       XtWidgetGeometry * desired)
#endif
{
    /* the caller should have set desired width and height*/
    desired->request_mode = (CWWidth | CWHeight) ;

    /* Accept any x, y, border and stacking. If the proposed
       geometry matches the desired one, and the parent intends
       to use these values (flags are set in intended),
       return Yes. Otherwise, the parent intends to use values for
       width and height that differ from the desired size, return No
       if the desired is the current and Almost if the desired size
       is different from the current size */
    if ((IsWidth(intended)) &&
      (intended->width == desired->width) &&
      (IsHeight(intended)) &&
      (intended->height == desired->height)) {
      return XtGeometryYes ;
    }
    
    if ((desired->width == XtWidth(widget)) &&
      (desired->height == XtHeight(widget))) {
      return XtGeometryNo ;
    }

    return XtGeometryAlmost ;
}


/****************
 * Return True if w intersects with any other siblins.
 ****************/
Boolean
#ifdef _NO_PROTO
_XmGMOverlap(manager, w)
XmManagerWidget manager ;
Widget w ;
#else
_XmGMOverlap(XmManagerWidget manager,
           Widget w)
#endif
{   
    register int      i ;
    Position  left1 = XtX(w) ;
    Position  top1 = XtY(w) ;
    Dimension right1 = XtX(w) + 2*XtBorderWidth(w) + XtWidth(w) ;
    Dimension bottom1 = XtY(w) + 2*XtBorderWidth(w) + XtHeight(w);

/****************/
    for(i=0 ; i<manager->composite.num_children ; i++) {   
      Widget          kid = manager->composite.children[i] ;
      Position        left2 = XtX(kid) ;
      Position        top2 = XtY(kid) ;
      Dimension       right2 = XtX(kid) + 2*XtBorderWidth(kid) + 
                               XtWidth(kid);
      Dimension       bottom2 = XtY(kid) + 2*XtBorderWidth(kid) + 
                                XtHeight(kid) ;
        if(w != kid && 
         (((left1 >= left2) && ((Dimension)left1 <= right2)) ||
          ((left2 >= left1) && ((Dimension)left2 <= right1))) &&
         (((top1 >= top2) && ((Dimension)top1 <= bottom2)) ||
          ((top2 >= top1) && ((Dimension)top2 <= bottom1)))    )
          {   return( True) ;
            } 
    }
    return( False) ;
}


/****************
 * Handle geometry requests from children.
 ****************/
XtGeometryResult
#ifdef _NO_PROTO
_XmGMHandleGeometryManager(parent, w, request, reply, 
                         margin_width, margin_height, resize_policy,
                         allow_overlap)
Widget          parent, w ;
XtWidgetGeometry * request ;
XtWidgetGeometry * reply ;
Dimension margin_width ;
Dimension margin_height ;
int resize_policy ;
int allow_overlap ;
#else
_XmGMHandleGeometryManager(Widget parent, Widget w,
                         XtWidgetGeometry * request,
                         XtWidgetGeometry * reply,
#if NeedWidePrototypes
                         int margin_width,
                         int margin_height,        
#else
                         Dimension margin_width,
                         Dimension margin_height,    
#endif /* NeedWidePrototypes */
                         int resize_policy,
                         int allow_overlap)
#endif
{

    /* Policy: Yes
         if margin is non null requests inside the margin or negative 
       are a priori almosted (or denied if the resizepolicy does not
       authorize it).
         That's the only case where almost is returned, no management
         of a limit position or size is done 
    */


    XtWidgetGeometry localReply ;
    Dimension width, height, borderWidth ;
    Position  x, y ;
    XtGeometryResult returnCode = XtGeometryNo ;
    Boolean geoFlag = False, queryonly = False ;
    XmManagerWidget manager = (XmManagerWidget) parent ;

    localReply = *request ;
    localReply.request_mode = CWX|CWY|CWWidth|CWHeight|CWBorderWidth ;

    if(!IsX(request)) localReply.x = XtX(w) ;
    if(!IsY(request)) localReply.y = XtY(w) ;
    if(!IsWidth(request)) localReply.width = XtWidth(w) ;
    if(!IsHeight(request)) localReply.height = XtHeight(w) ;
    if(!IsBorder(request)) localReply.border_width = XtBorderWidth(w) ;

    /*  check for x less than margin width
    */
    if(IsX(request) && (margin_width != 0)
       && (request->x < (int)margin_width)) {   
        localReply.x = (Position) margin_width ;
        returnCode = XtGeometryAlmost ;
    }
    /*  check for y less than margin height  */
    if(IsY(request) && (margin_height != 0)
       && (request->y < (int)margin_height)) {   
        localReply.y = (Position) margin_height ;
        returnCode = XtGeometryAlmost ;
    }

    /* Save current size and set to new size
    */
    x = XtX(w) ;
    y = XtY(w) ;
    width = XtWidth(w) ;
    height = XtHeight(w) ;
    borderWidth = XtBorderWidth(w) ;

    XtX(w) = localReply.x ;
    XtY(w) = localReply.y ;
    XtWidth(w) = localReply.width ;
    XtHeight(w) = localReply.height ;
    XtBorderWidth(w) = localReply.border_width ;

    if(!allow_overlap && _XmGMOverlap (manager, w)) {   
        returnCode = XtGeometryNo ;
    } else  {   
      /* if we already know that we are not gonna change anything */
      if ((returnCode == XtGeometryAlmost) || IsQueryOnly(request)) 
          queryonly = True ;

      /* see if the new layout is OK */
      geoFlag = _XmGMDoLayout(manager, 
                              margin_width, margin_height, 
                              resize_policy, queryonly) ;
      /* if we cannot adapt the new size but the child is still inside
	 go for it */
      if(!geoFlag && ((localReply.x + localReply.width +
		       (localReply.border_width << 1))
		      <= (XtWidth(manager) - margin_width))
	 && ((localReply.y + localReply.height +
	      (localReply.border_width << 1))
	     <= (XtHeight(manager) - margin_height)))
	  geoFlag = True ;

      if (geoFlag) {
          if (returnCode != XtGeometryAlmost)
              returnCode = XtGeometryYes ;
        } else
          returnCode = XtGeometryNo ;
            
      if  (returnCode == XtGeometryAlmost){
          if (reply) *reply = localReply ;
          else returnCode = XtGeometryNo ;
            } 
    }

    if ((returnCode != XtGeometryYes) || queryonly) {   
	/* Can't oblige, so restore previous values. */
	XtX(w) = x ;
	XtY(w) = y ;
	XtWidth(w) = width ;
	XtHeight(w) = height ;
	XtBorderWidth(w) = borderWidth ;
      } 

    return returnCode ;
}
