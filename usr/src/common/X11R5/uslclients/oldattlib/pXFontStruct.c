#ifndef	NOIDENT
#ident	"@(#)oldattlib:pXFontStruct.c	1.1"
#endif
/*
 pXFontStruct.c (C source file)
	Acc: 575322431 Fri Mar 25 14:47:11 1988
	Mod: 575321573 Fri Mar 25 14:32:53 1988
	Sta: 575321573 Fri Mar 25 14:32:53 1988
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

	void fpXCharStruct (stream, p, verbose)
	FILE * stream;
	XCharStruct * p;
	int verbose;

	void fpXFontProp (stream, p, verbose)
	FILE * stream;
	XFontProp * p;
	int verbose;

	void fpXFontStruct (stream, p, verbose)
	FILE * stream;
	XFontStruct * p;
	int verbose;

	author:
		Ross Hilbert
		AT&T 11/16/87
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
	print XCharStruct table header
*/
#define PrintCharHeader \
{ \
	fprintf (stream, "\n"); \
	fprintf (stream, "\tL => lbearing\n"); \
	fprintf (stream, "\tR => rbearing\n"); \
	fprintf (stream, "\tW => width\n"); \
	fprintf (stream, "\tA => ascent\n"); \
	fprintf (stream, "\tD => descent\n"); \
	fprintf (stream, "\tF => attributes\n"); \
	fprintf (stream, "\n"); \
	fprintf (stream, "\t character    L    R    W    A    D    F\n"); \
	fprintf (stream, "\t----------------------------------------\n"); \
}
/*
	print XCharStruct bounds entry in table
*/
#ifdef __STDC__
#define PrintCharBounds(MEMBER) \
{ \
	fprintf (stream, "\t%10s%5d%5d%5d%5d%5d%5d\n", #MEMBER, \
		(p->MEMBER).lbearing, (p->MEMBER).rbearing, (p->MEMBER).width, \
		(p->MEMBER).ascent, (p->MEMBER).descent, (p->MEMBER).attributes); \
}
#else
#define PrintCharBounds(MEMBER) \
{ \
	fprintf (stream, "\t%10s%5d%5d%5d%5d%5d%5d\n", "MEMBER", \
		(p->MEMBER).lbearing, (p->MEMBER).rbearing, (p->MEMBER).width, \
		(p->MEMBER).ascent, (p->MEMBER).descent, (p->MEMBER).attributes); \
}
#endif
/*
	print XCharStruct entry in table
*/
#define PrintCharStruct(i,j,p) \
{ \
	fprintf (stream, "\t%5d%5d%5d%5d%5d%5d%5d%5d\n", i, j, \
		(p)->lbearing, (p)->rbearing, (p)->width, \
		(p)->ascent, (p)->descent, (p)->attributes); \
}

static char buf[80];

void fpXCharStruct (stream, p, verbose)
FILE * stream;
XCharStruct * p;
int verbose;
{
	PR (lbearing, INT);
	PR (rbearing, INT);
	PR (width, INT);
	PR (ascent, INT);
	PR (descent, INT);
	PR (attributes, INT);
}

void fpXFontProp (stream, p, verbose)
FILE * stream;
XFontProp * p;
int verbose;
{
	PM (name, _knownproperties);
	PR (card32, ULONG);
}

void fpXFontStruct (stream, p, verbose)
FILE * stream;
XFontStruct * p;
int verbose;
{
	int i, j;

	PR (fid, HEX);
	PM (direction, _fontdirection);
	PR (min_char_or_byte2, UINT);
	PR (max_char_or_byte2, UINT);
	PR (min_byte1, UINT);
	PR (max_byte1, UINT);
	PM (all_chars_exist, _boolean);
	PR (default_char, UINT);
	PR (ascent, INT);
	PR (descent, INT);
	PR (n_properties, INT);

	if (verbose)
	{
		if (p->per_char)
		{
			int min1 = p->min_byte1;
			int max1 = p->max_byte1;
			int min2 = p->min_char_or_byte2;
			int max2 = p->max_char_or_byte2;
			XCharStruct * c = p->per_char;

			PrintCharHeader;
			PrintCharBounds (min_bounds);
			PrintCharBounds (max_bounds);

			for (i=min1; i<=max1; ++i)
				for (j=min2; j<=max2; ++j)
				{
					PrintCharStruct (i,j,c);
					c++;
				}
		}
		else
		{
			fprintf (stream, "%*s   \n", WIDTH, "min_bounds");
			fpXCharStruct (stream, &p->min_bounds, verbose);
			fprintf (stream, "%*s   \n", WIDTH, "max_bounds");
			fpXCharStruct (stream, &p->max_bounds, verbose);
		}
		for (i = 0; i < p->n_properties; ++i)
		{
			sprintf (buf, "properties[%d]", i);
			fprintf (stream, "%*s   \n", WIDTH, buf);
			fpXFontProp (stream, &p->properties[i], verbose);
		}
	}
}
