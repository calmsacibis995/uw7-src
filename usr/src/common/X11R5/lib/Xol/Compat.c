#ifndef	NOIDENT
#ident	"@(#)olmisc:Compat.c	1.5"
#endif

/*
 *************************************************************************
 *
 * Notes:
 *	This file should only contain obsoleted stuff.
 *
 *	Currently, this file contains pre-defined atoms prior to
 *	GS4i and various initialization routines. These variables
 *	and routines should be obsoleted in later release.
 * 
 ****************************file*header**********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


	/* "Pre-defined" Atoms */

Atom	WM_DECORATION_HINTS = 0;
Atom	WM_DISMISS = 0;

Atom	WM_WINDOW_MOVED = 0;
Atom	WM_TAKE_FOCUS = 0;
Atom	WM_SAVE_YOURSELF = 0;
Atom	WM_DELETE_WINDOW = 0;
Atom	BANG = 0;

Atom	_OL_HELP_KEY = 0;

Atom	_OL_WIN_ATTR = 0;
Atom	_OL_WT_BASE = 0;
Atom	_OL_WT_CMD = 0;
Atom	_OL_WT_PROP = 0;
Atom	_OL_WT_HELP = 0;
Atom	_OL_WT_NOTICE = 0;
Atom	_OL_WT_OTHER = 0;

Atom	_OL_DECOR_ADD = 0;
Atom	_OL_DECOR_DEL = 0;
Atom	_OL_DECOR_CLOSE = 0;
Atom	_OL_DECOR_RESIZE = 0;
Atom	_OL_DECOR_HEADER = 0;
Atom	_OL_DECOR_PIN = 0;

Atom	_OL_WIN_COLORS = 0;
Atom	_OL_PIN_STATE = 0;
Atom	_OL_WIN_BUSY = 0;

Atom	_OL_MENU_FULL = 0;
Atom	_OL_MENU_LIMITED = 0;
Atom	_OL_NONE = 0;

Atom	_OL_COPY = 0;
Atom	_OL_CUT = 0;

Atom	WM_STATE = 0;
Atom	WM_CHANGE_STATE = 0;
Atom	WM_ICON_SIZE = 0;
Atom	WM_PROTOCOLS = 0;

Atom	OL_MANAGER_STATE = 0;

Atom	_OL_WSM_QUEUE = 0;
Atom	_OL_WSM_REPLY = 0;

Atom	_OL_FM_QUEUE = 0;
Atom	_OL_FM_REPLY = 0;

#ifdef __STDC__
#define INTERN(property) property = XInternAtom(dpy,#property,False)
#else
#define INTERN(property) property = XInternAtom(dpy,"property",False)
#endif

extern void
WSMInitialize(dpy)
	Display *	dpy;
{
	INTERN(_OL_WSM_QUEUE);
	INTERN(_OL_WSM_REPLY);
} /* end of WSMInitialize */

extern void
FMInitialize(dpy)
	Display *	dpy;
{
	INTERN(_OL_FM_QUEUE);
	INTERN(_OL_FM_REPLY);
} /* end of FMInitialize */

void
InitializeOpenLook(dpy)
	Display *	dpy;
{
	INTERN(WM_DECORATION_HINTS);
	INTERN(WM_DISMISS);

	INTERN(WM_WINDOW_MOVED);
	INTERN(WM_TAKE_FOCUS);
	INTERN(BANG);

	INTERN(_OL_HELP_KEY);
	INTERN(_OL_WIN_ATTR);
	INTERN(_OL_WT_BASE);
	INTERN(_OL_WT_CMD);
	INTERN(_OL_WT_PROP);
	INTERN(_OL_WT_HELP);
	INTERN(_OL_WT_NOTICE);
	INTERN(_OL_WT_OTHER);
	INTERN(_OL_DECOR_ADD);
	INTERN(_OL_DECOR_DEL);
	INTERN(_OL_DECOR_CLOSE);
	INTERN(_OL_DECOR_RESIZE);
	INTERN(_OL_DECOR_HEADER);
	INTERN(_OL_DECOR_PIN);
	INTERN(_OL_WIN_COLORS);
	INTERN(_OL_PIN_STATE);
	INTERN(_OL_WIN_BUSY);
	INTERN(_OL_MENU_FULL);
	INTERN(_OL_MENU_LIMITED);
	INTERN(_OL_NONE);
	INTERN(_OL_COPY);
	INTERN(_OL_CUT);

	INTERN(WM_STATE);
	INTERN(WM_CHANGE_STATE);
	INTERN(WM_ICON_SIZE);
	INTERN(WM_PROTOCOLS);

	INTERN(WM_SAVE_YOURSELF);
	INTERN(WM_DELETE_WINDOW);

	INTERN(OL_MANAGER_STATE);

	WSMInitialize(dpy);
	FMInitialize(dpy);

} /* end of InitializeOpenLook */
