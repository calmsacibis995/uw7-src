#pragma ident	"@(#)m1.2libs:Xm/Dest.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
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

#include <Xm/XmP.h>
#include <Xm/DisplayP.h>

/* 
   This function is used for setting the "last editable widget on which a
   select, edit, insert, or paste operation was performed and is a destination
   for quick paste and certain clipboard functions" (for this display 
   connection) and is not necessarily a text widget.  
   Under current usage by Motif internals:
   This function is for squirreling away the widget that has the destination
   cursor so that it can be retrieved when pasting from a menu.  Called by 
   _XmTextSetDestinationSelection.
*/
#ifdef _NO_PROTO
void _XmSetDestination (dpy, w)
     Display * dpy; 
     Widget w;			
#else /* _NO_PROTO */
void _XmSetDestination (Display *dpy, Widget w)
#endif /* _NO_PROTO */
{
      XmDisplay   dd = (XmDisplay) XmGetXmDisplay(dpy);	/* w may be NULL */
      if ((XmDisplay)NULL != dd)
	((XmDisplayInfo *)(dd->display.displayInfo))->destinationWidget =
		w;	
}

/* This public function retrieves the widget saved by _XmSetDestination. */
#ifdef _NO_PROTO
Widget XmGetDestination (display)
     Display * display;
#else /* _NO_PROTO */
Widget XmGetDestination (Display *display)
#endif /* _NO_PROTO */
{
      XmDisplay   dd = (XmDisplay) XmGetXmDisplay(display);
      Widget w = (Widget)NULL;

      if ((XmDisplay)NULL != dd)
	 w = ((XmDisplayInfo *)(dd->display.displayInfo))->destinationWidget;
      return w;
}
