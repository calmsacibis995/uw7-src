#ifndef	NOIDENT
#ident	"@(#)oldattlib:Xinput.c	1.1"
#endif
/*
 Xinput.c (C source file)
	Acc: 575322225 Fri Mar 25 14:43:45 1988
	Mod: 575321561 Fri Mar 25 14:32:41 1988
	Sta: 575321561 Fri Mar 25 14:32:41 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/02/87
************************************************************************/

#include <X11/Xlib.h>
#include "Xinput.h"

void GetPointer (dpy, win, cursor, event)
Display * dpy;
Window win;
Cursor cursor;
XEvent * event;
{
	XEvent		up;

	while (XGrabPointer (dpy, win, False,
		ButtonPressMask|ButtonReleaseMask, GrabModeAsync,
		GrabModeAsync, None, cursor, CurrentTime) != GrabSuccess) ;
	XWindowEvent (dpy, win, ButtonPressMask, event);
	XWindowEvent (dpy, win, ButtonReleaseMask, &up);
	while (up.xbutton.time < event->xbutton.time)
		XWindowEvent (dpy, win, ButtonReleaseMask, &up);
	XUngrabPointer (dpy, CurrentTime);
}

Window GetApplicationWindow (dpy, scr, cursor)
Display * dpy;
int scr;
Cursor cursor;
{
	XEvent		event;
	Window		root = RootWindow (dpy, scr);

	GetPointer (dpy, root, cursor, &event);
	return event.xbutton.subwindow ? event.xbutton.subwindow : root;
}

Window GetWindow (dpy, scr, cursor)
Display * dpy;
int scr;
Cursor cursor;
{
	XEvent		event;
	Window		w = RootWindow (dpy, scr);
	Window		s, t;
	int		x, y, dx, dy;


	GetPointer (dpy, w, cursor, &event);
	s = event.xbutton.subwindow;

	if (s)
	{
		x = event.xbutton.x;
		y = event.xbutton.y;
		XTranslateCoordinates (dpy, w, s, x, y, &dx, &dy, &t);

		while (t)
		{
			w = s;
			s = t;
			x = dx;
			y = dy;
			XTranslateCoordinates (dpy, w, s, x, y, &dx, &dy, &t);
		}
		return s;
	}
	else
		return w;
}

void FlashWindow (dpy, win)
Display * dpy;
Window win;
{
	GC			gc;
	XGCValues		gcv;
	XWindowAttributes	w;

	gcv.function = GXequiv;
	gcv.foreground = (unsigned long) 0;
	gc = XCreateGC (dpy, win, GCFunction|GCForeground, &gcv);
	XGrabServer (dpy);
	XGetWindowAttributes (dpy, win, &w);
	XFillRectangle (dpy, win, gc, 0, 0, w.width, w.height);
	XFlush (dpy);
	sleep (1);
	XFillRectangle (dpy, win, gc, 0, 0, w.width, w.height);
	XUngrabServer (dpy);
	XFreeGC (dpy, gc);
}
