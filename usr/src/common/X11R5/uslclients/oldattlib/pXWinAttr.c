#ifndef	NOIDENT
#ident	"@(#)oldattlib:pXWinAttr.c	1.1"
#endif
/*
 pXWinAttr.c (C source file)
	Acc: 575322459 Fri Mar 25 14:47:39 1988
	Mod: 575321574 Fri Mar 25 14:32:54 1988
	Sta: 575321574 Fri Mar 25 14:32:54 1988
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

	void fpXWindowAttributes (stream, p, verbose)
	FILE * stream;
	XWindowAttributes * p;
	int verbose;

	void fpXSetWindowAttributes (stream, mask, p)
	FILE * stream;
	unsigned long mask;
	XSetWindowAttributes * p;

	void fpXWindowChanges (stream, mask, p)
	FILE * stream;
	unsigned long mask;
	XWindowChanges * p;

	author:
		Ross Hilbert
		AT&T 10/20/87
************************************************************************/

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xprint.h"
#include "pID.h"

#define WIDTH	25

/*
	standard print
*/
#ifdef __STDC__
#define PR(MEMBER, FORMAT) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, FORMAT, p->MEMBER); \
	fprintf (stream, "\n"); \
}
#else
#define PR(MEMBER, FORMAT) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, FORMAT, p->MEMBER); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print match from id set
*/
#ifdef __STDC__
#define PM(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprint_match (stream, (unsigned long)p->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PM(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_match (stream, (unsigned long)p->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print group (mask) from id set
*/
#ifdef __STDC__
#define PG(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprint_mask (stream, (unsigned long)p->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PG(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_mask (stream, (unsigned long)p->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print pixmap
*/
#ifdef __STDC__
#define PP(MEMBER) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	if (p->MEMBER == None) \
		fprintf (stream, "None"); \
	else if (p->MEMBER == ParentRelative) \
		fprintf (stream, "ParentRelative"); \
	else if (p->MEMBER == CopyFromParent) \
		fprintf (stream, "CopyFromParent"); \
	else \
		fprintf (stream, "0x%lx", p->MEMBER); \
	fprintf (stream, "\n"); \
}
#else
#define PP(MEMBER) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	if (p->MEMBER == None) \
		fprintf (stream, "None"); \
	else if (p->MEMBER == ParentRelative) \
		fprintf (stream, "ParentRelative"); \
	else if (p->MEMBER == CopyFromParent) \
		fprintf (stream, "CopyFromParent"); \
	else \
		fprintf (stream, "0x%lx", p->MEMBER); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print visual class
*/
#ifdef __STDC__
#define PV(MEMBER) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprint_match (stream, (unsigned long)p->MEMBER->class, _visualclass); \
	fprintf (stream, "\n"); \
}
#else
#define PV(MEMBER) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_match (stream, (unsigned long)p->MEMBER->class, _visualclass); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print window with name
*/
#ifdef __STDC__
#define PW(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, "0x%lx", p->MEMBER); \
	if (p->MEMBER && XFetchName (p->screen->display, p->MEMBER, &name)) \
	{ \
		fprintf (stream, " (%s)", name); \
		XFree (name); \
	} \
	fprintf (stream, "\n"); \
}
#else
#define PW(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, "0x%lx", p->MEMBER); \
	if (p->MEMBER && XFetchName (p->screen->display, p->MEMBER, &name)) \
	{ \
		fprintf (stream, " (%s)", name); \
		XFree (name); \
	} \
	fprintf (stream, "\n"); \
}
#endif

void fpXWindowAttributes (stream, p, verbose)
FILE * stream;
XWindowAttributes * p;
int verbose;
{
	PR (x, INT);
	PR (y, INT);
	PR (width, INT);
	PR (height, INT);
	PR (border_width, INT);

	if (verbose)
	{
		PR (depth, INT);
		PV (visual);
		PW (root);
		PM (class, _class);
		PM (bit_gravity, _bit_gravity);
		PM (win_gravity, _win_gravity);
		PM (backing_store, _backing_store);
		PR (backing_planes, LONGHEX);
		PR (backing_pixel, LONGHEX);
		PM (save_under, _boolean);
		PR (colormap, LONGHEX);
		PM (map_installed, _boolean);
		PM (map_state, _map_state);
		PG (all_event_masks, _eventmasks);
		PG (your_event_mask, _eventmasks);
		PG (do_not_propagate_mask, _eventmasks);
		PM (override_redirect, _boolean);
		PR (screen, LONGHEX);
	}
}

void fpXSetWindowAttributes (stream, mask, p)
FILE * stream;
unsigned long mask;
XSetWindowAttributes * p;
{
	if (mask & CWBackPixmap)	PP (background_pixmap);
	if (mask & CWBackPixel)		PR (background_pixel, LONGHEX);
	if (mask & CWBorderPixmap)	PP (border_pixmap);
	if (mask & CWBorderPixel)	PR (border_pixel, LONGHEX);
	if (mask & CWBitGravity)	PM (bit_gravity, _bit_gravity);
	if (mask & CWWinGravity)	PM (win_gravity, _win_gravity);
	if (mask & CWBackingStore)	PM (backing_store, _backing_store);
	if (mask & CWBackingPlanes)	PR (backing_planes, LONGHEX);
	if (mask & CWBackingPixel)	PR (backing_pixel, LONGHEX);
	if (mask & CWSaveUnder)		PM (save_under, _boolean);
	if (mask & CWEventMask)		PG (event_mask, _eventmasks);
	if (mask & CWDontPropagate)	PG (do_not_propagate_mask, _eventmasks);
	if (mask & CWOverrideRedirect)	PM (override_redirect, _boolean);
	if (mask & CWColormap)		PR (colormap, LONGHEX);
	if (mask & CWCursor)		PR (cursor, LONGHEX);
}

void fpXWindowChanges (stream, mask, p)
FILE * stream;
unsigned long mask;
XWindowChanges * p;
{
	if (mask & CWX)			PR (x, INT);
	if (mask & CWY)			PR (y, INT);
	if (mask & CWWidth)		PR (width, INT);
	if (mask & CWHeight)		PR (height, INT);
	if (mask & CWBorderWidth)	PR (border_width, INT);
	if (mask & CWSibling)		PR (sibling, LONGHEX);
	if (mask & CWStackMode)		PM (stack_mode, _configuredetail);
}
