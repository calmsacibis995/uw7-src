#ifndef	NOIDENT
#ident	"@(#)oldattlib:pGC.c	1.1"
#endif
/*
 pGC.c (C source file)
	Acc: 575322301 Fri Mar 25 14:45:01 1988
	Mod: 575321563 Fri Mar 25 14:32:43 1988
	Sta: 575321563 Fri Mar 25 14:32:43 1988
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

	void fpGC (stream, mask, gc)
	FILE * stream;
	unsigned long mask;
	GC gc;

	void fpXGCValues (stream, mask, gc)
	FILE * stream;
	unsigned long mask;
	XGCValues * gc;

	author:
		Ross Hilbert
		AT&T 10/30/87
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
	fprintf (stream, FORMAT, gc->MEMBER); \
	fprintf (stream, "\n"); \
}
#else
#define PR(MEMBER, FORMAT) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, FORMAT, gc->MEMBER); \
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
	fprint_match (stream, (unsigned long)gc->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PM(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_match (stream, (unsigned long)gc->MEMBER, IDS); \
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
	fprint_mask (stream, (unsigned long)gc->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PG(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_mask (stream, (unsigned long)gc->MEMBER, IDS); \
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
	if (gc->MEMBER == None) \
		fprintf (stream, "None"); \
	else if (gc->MEMBER == ParentRelative) \
		fprintf (stream, "ParentRelative"); \
	else if (gc->MEMBER == CopyFromParent) \
		fprintf (stream, "CopyFromParent"); \
	else \
		fprintf (stream, "0x%lx", gc->MEMBER); \
	fprintf (stream, "\n"); \
}
#else
#define PP(MEMBER) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	if (gc->MEMBER == None) \
		fprintf (stream, "None"); \
	else if (gc->MEMBER == ParentRelative) \
		fprintf (stream, "ParentRelative"); \
	else if (gc->MEMBER == CopyFromParent) \
		fprintf (stream, "CopyFromParent"); \
	else \
		fprintf (stream, "0x%lx", gc->MEMBER); \
	fprintf (stream, "\n"); \
}
#endif

void fpXGCValues (stream, mask, gc)
FILE * stream;
unsigned long mask;
XGCValues * gc;
{
	if (mask & GCFunction)		PM (function, _GXfunction);
	if (mask & GCPlaneMask)		PR (plane_mask, LONGHEX);
	if (mask & GCForeground)	PR (foreground, LONGHEX);
	if (mask & GCBackground)	PR (background, LONGHEX);
	if (mask & GCLineWidth)		PR (line_width, INT);
	if (mask & GCLineStyle)		PM (line_style, _line_style);
	if (mask & GCCapStyle)		PM (cap_style, _cap_style);
	if (mask & GCJoinStyle)		PM (join_style, _join_style);
	if (mask & GCFillStyle)		PM (fill_style, _fill_style);
	if (mask & GCFillRule)		PM (fill_rule, _fill_rule);
	if (mask & GCArcMode)		PM (arc_mode, _arc_mode);
	if (mask & GCTile)		PP (tile);
	if (mask & GCStipple)		PP (stipple);
	if (mask & GCTileStipXOrigin)	PR (ts_x_origin, INT);
	if (mask & GCTileStipYOrigin)	PR (ts_y_origin, INT);
        if (mask & GCFont)		PR (font, LONGHEX);
	if (mask & GCSubwindowMode)	PM (subwindow_mode, _subwindow_mode);
	if (mask & GCGraphicsExposures)	PM (graphics_exposures, _boolean);
	if (mask & GCClipXOrigin)	PR (clip_x_origin, INT);
	if (mask & GCClipYOrigin)	PR (clip_y_origin, INT);
	if (mask & GCClipMask)		PP (clip_mask);
	if (mask & GCDashOffset)	PR (dash_offset, INT);
	if (mask & GCDashList)		PR (dashes, INT);
}

static int ALL = 
		GCFunction |
		GCPlaneMask |
		GCForeground |
		GCBackground |
		GCLineWidth |
		GCLineStyle |
		GCCapStyle |
		GCJoinStyle |
		GCFillStyle |
		GCFillRule |
		GCArcMode |
		GCTile |
		GCStipple |
		GCTileStipXOrigin |
		GCTileStipYOrigin |
		GCFont |
		GCSubwindowMode |
		GCGraphicsExposures |
		GCClipXOrigin |
		GCClipYOrigin |
		GCClipMask |
		GCDashOffset |
		GCDashList;

void fpGC (stream, mask, gc)
FILE * stream;
unsigned long mask;
GC gc;
{
	XGCValues * p = &gc -> values;
	if (!mask) mask = ALL;
	if (gc -> rects) mask &= ~GCClipMask;
	if (gc -> dashes) mask &= ~GCDashList;
	fpXGCValues (stream, mask, p);
}
