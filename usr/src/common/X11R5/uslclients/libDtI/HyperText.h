#pragma ident	"@(#)libDtI:HyperText.h	1.5"

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

#ifndef HYPERTEXT_H
#define HYPERTEXT_H
#ident "@(#)libDtI:HyperText.h	1.1 8/20/91"

/***********************************************************************
 Name                Class              RepType     Default Value
 ----                -----              -------     -------------
 file                File               String      NULL
 string              String             String      NULL
 sourceType          sourceType         int         OL_STRING_SOURCRE
					       (other: OL_DISK_SOURCE)

 font                Font               FontStruct  OlDefaultFont
 keyColor            Foreground         Pixel       Blue

 leftMargin          Margin             Dimension   20
 rightMargin         Margin             Dimension   20
 topMargin           Margin             Dimension   20
 bottomMargin        Margin             Dimension   20

 recomputeSize       RecomputeSize      Boolean     TRUE
 select              Callback           Pointer     NULL
		     (HyperSegment * call_data;)

**************************************************************************/

#define XtNkeyColor           "keyColor"
#define XtCMaxSize            "MaxSize"

/* SPACE_ABOVE can be 0, but SPACE_BELOW has to be at least 1 */
#define SPACE_ABOVE  1
#define SPACE_BELOW  1

/* margin sizes */
#define LEFT_MARGIN		20
#define RIGHT_MARGIN	20
#define TOP_MARGIN		20
#define BOT_MARGIN		20

/* default highlight color */
#ifndef XtRDimension
/* for 6386 */
#define XtRDimension          "Int"
#endif

#define HtNewString(str)	((str) ? strdup(str) : NULL)

extern WidgetClass hyperTextWidgetClass;
typedef struct _HyperTextClassRec *HyperTextWidgetClass;
typedef struct _HyperTextRec *HyperTextWidget;

typedef struct hyper_segment HyperSegment;
struct hyper_segment {
	char *key;			/* non-NULL: hyper text key, NULL: not */
	char *script;       	/* reference string */
	char *text;
	unsigned short len;		/* length of text */
	Position x, y;			/* position of segment */
	Position y_text;		/* the y position for text baseline */
	Dimension w, h;
	unsigned short tabs;	/* number of tabs to be skipped first */
	Pixel color;
	Boolean use_color;		/* FALSE: default color, TRUE: color field */
	Boolean reverse_video;
	HyperSegment *next;
	HyperSegment *prev;
};

typedef struct hyper_line HyperLine;

struct hyper_line {
    HyperSegment *first_segment;
    HyperSegment *last_segment;
    HyperLine *next;
    HyperLine *prev;
};

/******* global functions **************************************/

extern char *HyperSegmentKey(HyperSegment *hs);
extern char *HyperSegmentScript(HyperSegment *hs);
extern char *HyperSegmentText(HyperSegment *hs);
extern Boolean HyperSegmentRV(HyperSegment *hs); /* reverse video or not */
extern void HyperLineFree(HyperLine *hl);
extern void HyperTextHighlightSegment(HyperTextWidget hw, HyperSegment *hs);
extern void HyperTextUnhighlightSegment(HyperTextWidget hw);
extern HyperSegment *HyperTextGetHighlightedSegment(HyperTextWidget hw);

#endif /* HYPERTEXT_H */
