#ifndef	NOIDENT
#ident	"@(#)oldattlib:XPause.c	1.1"
#endif
/*
 XPause.c (C source file)
	Acc: 575322166 Fri Mar 25 14:42:46 1988
	Mod: 570731707 Mon Feb  1 11:35:07 1988
	Sta: 573929805 Wed Mar  9 11:56:45 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <X11/Xlib.h>
#include <Xos.h>
#include <stdio.h>

#if SVR4
#define SELECT select
#else 
#define SELECT pollselect
#endif

XPause (dpy,sec,usec)
Display * dpy;
int sec;
int usec;
{
	int maxfds = ConnectionNumber(dpy) + 1;
	int readfds = 1 << ConnectionNumber(dpy);
	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	if (SELECT (maxfds, &readfds, NULL, NULL, &timeout) == -1)
		return 1;

	return 0;
}
