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
**      File:   HyperText.h
**      Date:   08/06/90
**
****************************************************************/

#ifndef _EXM_HYPERTEXT_H_
#define _EXM_HYPERTEXT_H_

#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:HyperText.h	1.3"
#endif

/***********************************************************************
 Name                Class              RepType     Default Value
 ----                -----              -------     -------------
 diskSource	     DiskSource		Boolean	    True  (file)
						    False (string)
 cacheString	     CacheString	Boolean     False - don't make a copy
							of the XmNstring, simply
							parse it and set the
							XmNstring to NULL
							afterward.
 string              String             String      NULL

 fontList            FontList           XmFontList  NULL
 keyColor            Foreground         Pixel       Blue

 select              Callback           Pointer     NULL
		     (HyperSegment * call_data;)

**************************************************************************/

#include <Xm/Xm.h>
#include "WidePosDef.h"

#define XmNcacheString		(_XmConst char *)"cacheString"
#define XmCCacheString		(_XmConst char *)"CacheString"
#define XmNdiskSource		(_XmConst char *)"diskSource"
#define XmCDiskSource		(_XmConst char *)"DiskSource"
#define XmNkeyColor		(_XmConst char *)"keyColor"
#define XmCTextBackground	(_XmConst char *)"TextBackground"
#define XmNfocusColor		(_XmConst char *)"focusColor"
#define XmCFocusColor		(_XmConst char *)"FocusColor"
#define XmNselect		(_XmConst char *)"select"
#define XmNtopRow		(_XmConst char *)"topRow"
#define XmCTopRow		(_XmConst char *)"TopRow"

extern WidgetClass exmHyperTextWidgetClass;
typedef struct _ExmHyperTextClassRec *ExmHyperTextWidgetClass;
typedef struct _ExmHyperTextRec *ExmHyperTextWidget;

typedef struct exm_hyper_segment ExmHyperSegment;
struct exm_hyper_segment {
	ExmHyperSegment *	next;
	ExmHyperSegment *	prev;
	String			key;	/* hyper text key if non NULL */
	String			script;	/* reference string */
	String			text;
	WidePosition		x, y;	/* position of segment */
	Dimension		w, h;	/* width/height */
	Dimension		len;	/* length of text */
	Dimension		tabs;	/* nos of tabs to be skipped first */
	Boolean			reverse_video;
	Boolean			shell_cmd;
};

typedef struct exm_hyper_line ExmHyperLine;
struct exm_hyper_line {
    ExmHyperSegment *	first_segment;
    ExmHyperSegment *	last_segment;
    ExmHyperLine *	next;
    ExmHyperLine *	prev;
};

#define ExmHyperSegmentKey(HS)		(HS ? HS->key : NULL)
#define ExmHyperSegmentRV(HS)		(HS ? HS->reverse_video : False)
#define ExmHyperSegmentScript(HS)	(HS ? HS->script : NULL)
#define ExmHyperSegmentText(HS)		(HS ? HS->text : NULL)
#define ExmHyperSegmentShellCmd(HS)	(HS ? HS->shell_cmd : False)

#endif /* _EXM_HYPERTEXT_H */
