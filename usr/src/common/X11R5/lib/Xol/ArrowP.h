#ifndef	NOIDENT
#ident	"@(#)arrow:ArrowP.h	1.13"
#endif

/*
 ArrowP.h (C header file)
	Acc: 573911696 Wed Mar  9 06:54:56 1988
	Mod: 572968131 Sat Feb 27 08:48:51 1988
	Sta: 573856879 Tue Mar  8 15:41:19 1988
	Owner: 2011
	Group: 1985
	Permissions: 444
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/*
* $Header$
*/

/*copyright	"%c%"*/

/*
 *  ArrowP.h - Private definitions for Arrow widget
 *
 */

#ifndef _XtArrowP_h
#define _XtArrowP_h

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Arrow.h>

/*****************************************************************************
 *
 * Arrow Widget Private Data
 *
 *****************************************************************************/

/* New fields for the Arrow widget class record */
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} ArrowClassPart;

/* Full Class record declaration */
typedef struct _ArrowClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    ArrowClassPart	arrow_class;
} ArrowClassRec;

extern ArrowClassRec arrowClassRec;

/* New fields for the Arrow widget record */
typedef struct {
  XtCallbackList btnDown;
  XtCallbackList btnMotion;
  XtCallbackList btnUp;
  XtIntervalId timerid;
  OlDefine direction; /* OL_NONE, OL_TOP, OL_BOTTOM, OL_LEFT, OL_RIGHT */
  int scale; /* ... */
  int repeatRate;
  int initialDelay;
  int centerLine;	/* GetValues Only */
  GC fggc, bggc;
  Boolean normal;
} ArrowPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _ArrowRec {
	CorePart	core;
	PrimitivePart	primitive;
	ArrowPart	arrow;
} ArrowRec;

#endif /* _XtArrowP_h */

