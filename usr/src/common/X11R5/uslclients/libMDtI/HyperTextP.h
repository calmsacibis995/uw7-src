/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperTextP.h
**      Date:   08/06/90
**
****************************************************************/

#ifndef _EXM_HYPERTEXTP_H_
#define _EXM_HYPERTEXTP_H_

#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:HyperTextP.h	1.3"
#endif

#include <Xm/PrimitiveP.h>
#include "HyperText.h"

#ifndef XmRDimension
#define XmRDimension		"Int"           /* for 6386 */
#endif

/************************************
 *
 *  Class structure
 *
 ***********************************/

/* New fields for the HyperText widget class record */
typedef struct _ExmHyperTextClass {
    char empty;                  /* not used */
} ExmHyperTextClassPart;

/* Full class record declaration */
typedef struct _ExmHyperTextClassRec {
    CoreClassPart		core_class;
    XmPrimitiveClassPart	primitive_class;
    ExmHyperTextClassPart	hyper_text_class;
} ExmHyperTextClassRec;

extern ExmHyperTextClassRec exmHyperTextClassRec;

/***************************************
 *
 *  Local structures
 *
 **************************************/

/*********** functions defined in HyperText0.c *********************/

extern ExmHyperLine *	ExmHyperLineFromFile(Widget, String);
extern ExmHyperLine *	ExmHyperLineFromString(Widget, String);

/***************************************
 *
 *  Instance (widget) structure
 *
 **************************************/

/* New fields for the HyperText widget record */
typedef struct _ExmHyperTextPart {
    String         string;	/* XmNstring */
    Pixel          key_color;	/* XmNkeyColor */
    Pixel	   focus_color;	/* XmNfocusColor */
    XmFontList	   font_list;	/* XmNfontList */
    XtCallbackList callbacks;	/* XmNselect */
    unsigned char  rows;	/* XmNrows,    in line_height units */
    unsigned char  cols;	/* XmNcolumns, in letter `n' width units */
    Boolean	   disk_source;	/* XmNdiskSource */
    Boolean	   cache_string;/* XmNcacheString, False by default, it means,
					don't make a copy of the XmNstring,
					simply parsing it and then set the
					field to NULL afterward, the benifit
					of this is to avoid duplicate `string'
					copies in the app and the widget. */

    /*	The reason for keeping `font' entry is for performance.
     *	Motif toolkit deals with `string' thru `XmString', this
     *	is too expense for HyperText because a HyperText string
     *	usually contains many segments (HyperTextSegment) basing
     *	on the key word(s) in the content. Creating these many
     *	XmString in C locale is just too much. The font member
     *	will be set to a valid pointer if XmFontType is XmFONT_IS_FONT
     *	otherwise, the font member is set to NULL (XmFONT_IS_FONTSET).
     *
     *	In the latter case (i.e., XmFONT_IS_FONTSET), *Xm* should be
     *	used for drawing and for calculating, XmString will be created
     *	and freed on demand (sigh!!) */
    XFontStruct *  font;
    ExmHyperLine * first_line;
    ExmHyperLine * last_line;
    ExmHyperSegment* highlight;
    XtIntervalId   timer_id;
    GC		   gc;
    Widget	   hsb;		/* horizontal scrollbar */
    Widget	   vsb;		/* vertical scrollbar */
    WidePosition   x_offset;	/* offset w.r.t. the real window */
    WidePosition   y_offset;
    WidePosition   actual_width;  /* actual width/height of the window */
    WidePosition   actual_height; /* core.width/height has virtual size */
    unsigned short misc_flag;	  /* misc flag serves various purposes */
    Dimension      line_height;	/* served as slide unit in y direction */
    Dimension      tab_width;	/* served as slide unit in x direction */
    int   	   top_row;	/* the line number of the line scrolled
				   on top of the window */
} ExmHyperTextPart;

/* Full widget declaration */
typedef struct _ExmHyperTextRec {
    CorePart		core;
    XmPrimitivePart	primitive;
    ExmHyperTextPart	hyper_text;
} ExmHyperTextRec;

#ifdef MOOLIT
		/* dynamic resource bit masks */
#define OL_B_HYPERTEXT_BG		(1<<0)
#define OL_B_HYPERTEXT_FONTCOLOR	(1<<1)
#define OL_B_HYPERTEXT_FONT		(1<<2)
#endif

#endif /* _EXM_HYPERTEXTP_H_  */
