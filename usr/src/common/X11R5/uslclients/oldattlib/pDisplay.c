#ifndef	NOIDENT
#ident	"@(#)oldattlib:pDisplay.c	1.1"
#endif
/*
 pDisplay.c (C source file)
	Acc: 575322249 Fri Mar 25 14:44:09 1988
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

	void fpDisplay (stream, p, verbose)
	FILE * stream;
	Display * p;
	int verbose;

	void fpScreen (stream, p, verbose)
	FILE * stream;
	Screen * p;
	int verbose;

	void fpVisual (stream, p, verbose)
	FILE * stream;
	Visual * p;
	int verbose;

	void fpDepth (stream, p, verbose)
	FILE * stream;
	Depth * p;
	int verbose;

	void fpScreenFormat (stream, p, verbose)
	FILE * stream;
	ScreenFormat * p;
	int verbose;

	author:
		Ross Hilbert
		AT&T 11/25/87
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

static char buf[80];

void fpVisual (stream, p, verbose)
FILE * stream;
Visual * p;
int verbose;
{
	PR (visualid, LONGHEX);
	PM (class, _visualclass);

	if (verbose)
	{
		PR (red_mask, LONGHEX);
		PR (green_mask, LONGHEX);
		PR (blue_mask, LONGHEX);
		PR (bits_per_rgb, INT);
		PR (map_entries, INT);
	}
}

void fpDepth (stream, p, verbose)
FILE * stream;
Depth * p;
int verbose;
{
	int i;

	PR (depth, INT);
	PR (nvisuals, INT);

	if (verbose)
	{
		for (i = 0; i < p->nvisuals; ++i)
		{
			sprintf (buf, "visuals[%d]", i);
			fprintf (stream, "%*s   \n", WIDTH, buf);
			fpVisual (stream, &p->visuals[i], verbose);
		}
	}
}

void fpScreen (stream, p, verbose)
FILE * stream;
Screen * p;
int verbose;
{
	int i;

	PR (width, INT);
	PR (height, INT);
	PR (mwidth, INT);
	PR (mheight, INT);
	PR (root_depth, INT);
	PR (root_visual, LONGHEX);
	PR (default_gc, LONGHEX);
	PR (cmap, LONGHEX);
	PR (white_pixel, LONGHEX);
	PR (black_pixel, LONGHEX);
	PR (max_maps, INT);
	PR (min_maps, INT);
	PM (backing_store, _backing_store);
	PM (save_unders, _boolean);
	PG (root_input_mask, _eventmasks);
	PR (ndepths, INT);

	if (verbose)
	{
		for (i = 0; i < p->ndepths; ++i)
		{
			sprintf (buf, "depths[%d]", i);
			fprintf (stream, "%*s   \n", WIDTH, buf);
			fpDepth (stream, &p->depths[i], verbose);
		}
	}
}

void fpScreenFormat (stream, p, verbose)
FILE * stream;
ScreenFormat * p;
int verbose;
{
	PR (depth, INT);
	PR (bits_per_pixel, INT);
	PR (scanline_pad, INT);
}

void fpDisplay (stream, p, verbose)
FILE * stream;
Display * p;
int verbose;
{
	int i;

	PR (fd, INT);
	PR (proto_major_version, INT);
	PR (proto_minor_version, INT);
	PR (vendor, STRING);
	PM (byte_order, _bytebitorder);
	PR (bitmap_unit, INT);
	PR (bitmap_pad, INT);
	PM (bitmap_bit_order, _bytebitorder);
	PR (vnumber, INT);
	PR (release, INT);
	PR (max_request_size, UINT);
	PR (display_name, STRING);
	PR (default_screen, INT);
	PR (motion_buffer, INT);
	PR (min_keycode, INT);
	PR (max_keycode, INT);
	PR (keysyms_per_keycode, INT);
	PR (nformats, INT);
	PR (nscreens, INT);

	if (verbose)
	{
		for (i = 0; i < p->nformats; ++i)
		{
			sprintf (buf, "pixmap_format[%d]", i);
			fprintf (stream, "%*s   \n", WIDTH, buf);
			fpScreenFormat (stream, &p->pixmap_format[i], verbose);
		}
		for (i = 0; i < p->nscreens; ++i)
		{
			sprintf (buf, "screens[%d]", i);
			fprintf (stream, "%*s   \n", WIDTH, buf);
			fpScreen (stream, &p->screens[i], verbose);
		}
	}
}
