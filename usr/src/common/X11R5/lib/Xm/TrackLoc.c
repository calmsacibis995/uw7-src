#pragma ident	"@(#)m1.2libs:Xm/TrackLoc.c	1.2"
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
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/XmP.h>
#include "MessagesI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define GRABPTRERROR    catgets(Xm_catd,MS_CButton,MSG_CB_5,_XmMsgCascadeB_0003)
#else
#define GRABPTRERROR    _XmMsgCascadeB_0003
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Widget _XmInputInWidget() ;

#else

static Widget _XmInputInWidget( 
                        Widget w,
#if NeedWidePrototypes
                        int x,
                        int y) ;
#else
                        Position x,
                        Position y) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/



/******************************************************/
/* copy from _XmInputInGadget, buts works for widget too,
   only used in this module so far, so let it static */
static Widget 
#ifdef _NO_PROTO
_XmInputInWidget (w, x, y)
Widget w;
Position x;
Position y;

#else /* _NO_PROTO */
_XmInputInWidget(
        Widget w,
#if NeedWidePrototypes
        int x,
        int y)
#else
        Position x,
        Position y)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   int i;
   Widget child;
   CompositeWidget cw = (CompositeWidget) w;
   
   /* loop over the child list to find if there is one at x,y */
   /* well, overlapping won't really work, since I have no standard way to
      check visibility */
   for (i = 0; i < cw->composite.num_children; i++) {
      child = cw->composite.children[i];
      if (XtIsManaged (child)) {
         if (x >= child->core.x && y >= child->core.y && 
             x < child->core.x + child->core.width    && 
             y < child->core.y + child->core.height) 
            return (child);
      }
   }
   return (NULL);
}





/* new tracking locate function that returns the event that trigerred
   the return + this function uses XNextEvent instead of just XMaskEvent
   which was only eating some events and thus causing problem when
   back to the application */

Widget 
#ifdef _NO_PROTO
XmTrackingEvent( widget, cursor, confineTo, pev )
        Widget widget ;
        Cursor cursor ;
        Boolean confineTo ;
        XEvent * pev ;
#else
XmTrackingEvent(
        Widget widget,
        Cursor cursor,
#if NeedWidePrototypes
        int confineTo,
#else
        Boolean confineTo,
#endif /* NeedWidePrototypes */
	XEvent * pev)
#endif /* _NO_PROTO */
{
    Window      w, confine_to = None;
    Time        lastTime;
    Widget      child;
    Boolean     key_has_been_pressed= False;
    Widget      target ;
    Position x, y ;
    int status;
    
    if (widget == NULL) return(NULL);

    w = XtWindowOfObject(widget);
    if (confineTo) confine_to = w;

    lastTime = XtLastTimestampProcessed(XtDisplay(widget));
    XmUpdateDisplay(widget);    

    if ((status = XtGrabPointer(widget, True, 
          /* The following truncation of masks is due to a bug in the Xt API.*/
			 (unsigned int) (ButtonPressMask | ButtonReleaseMask), 
				GrabModeAsync, GrabModeAsync, 
				confine_to, cursor, lastTime)) != 
	GrabSuccess) {

	_XmWarning(widget, GRABPTRERROR);

	return NULL ;
    }

    while (True) {
	/* eat all events, not just button's */
          XNextEvent(XtDisplay(widget), pev);

	/* dispatch Expose: CR 6010 */
	if (pev->type == Expose ) XtDispatchEvent(pev);

	/* track only button1release (BSelect) and 
	   non first keyrelease (because activation might happen
	   thru the keyboard and you don't want return right away */
        if (((pev->type == ButtonRelease) && 
	   (pev->xbutton.button == Button1)) || 
	    ((pev->type == KeyRelease) && key_has_been_pressed)) {

            if ((!confineTo) && (pev->xbutton.window == w)) {
		/* this is the case where we are not confine, so the user
		   can click outside the window, but we want to return
		   NULL */
		if ((pev->xbutton.x < 0) || (pev->xbutton.y < 0) ||
		    (pev->xbutton.x > widget->core.width) ||
		    (pev->xbutton.y > widget->core.height))
		    {
			XtUngrabPointer(widget, lastTime);
			return(NULL);
		    }
	    }

	    target = XtWindowToWidget(pev->xbutton.display,
				      pev->xbutton.window);
	    
/* New algorithm that solves the problem of mouse insensitive widgets: 
    ( i.e you can't get help on Label.)
   When we get the Btn1Up event with the window in which the event ocurred,
   and convert it to a widget.  If that widget is a primitive, return it.
   Otherwise, it is a composite or a shell, and we do the following:

   Walk down the child list (for gadgets AND WIDGETS), looking for one which
   contains the x,y.  If none is found, return the manager itself,
   If a primitive or gadget is found, return the object found.
   If a manager is found, execute the above algorithm recursively with
   that manager. */

	    if (target) {
		/* do not change the original pev coordinates */
		x = pev->xbutton.x ;
		y = pev->xbutton.y ;
		/* do not enter the loop for primitive */
		while (XtIsComposite(target) || XtIsShell(target)) {
		    if ((child = _XmInputInWidget(target, x,y))) {
			target = child;
			/* if a gadget or a primitive is found, return */
			if (!XtIsComposite(child)) break ;

			/* otherwise, loop */
			x = x - XtX(child) ;
			y = y - XtY(child) ;
		    } else break ;
		    /* no child found at this place: return the manager */
		}
	    }
            break ;
        } else if (pev->type == KeyPress) key_has_been_pressed = True ;
	/* to avoid exiting on the first keyreleased coming from
	   the keypress activatation of the function itself */
    }

    XtUngrabPointer(widget, lastTime);
    return(target);
}



/* reimplementation of the old function using the new one and a dummy event */
Widget 
#ifdef _NO_PROTO
XmTrackingLocate( widget, cursor, confineTo )
        Widget widget ;
        Cursor cursor ;
        Boolean confineTo ;
#else
XmTrackingLocate(
        Widget widget,
        Cursor cursor,
#if NeedWidePrototypes
        int confineTo )
#else
        Boolean confineTo )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XEvent ev ;

    return XmTrackingEvent(widget, cursor, confineTo, &ev);
}
