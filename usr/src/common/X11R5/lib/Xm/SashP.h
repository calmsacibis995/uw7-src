#pragma ident	"@(#)m1.2libs:Xm/SashP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
 *  SashP.h - Private definitions for Sash widget (Used by VPane Widget)
 *
 */

#ifndef _XmSashP_h
#define _XmSashP_h

#include <Xm/PrimitiveP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 *
 * Sash Widget Private Data
 *
 *****************************************************************************/

/* New fields for the Sash widget class record */
typedef struct {
   XtPointer extension;   /* Pointer to extension record */
} XmSashClassPart;

/* Full Class record declaration */
typedef struct _XmSashClassRec {
    CoreClassPart         core_class;
    XmPrimitiveClassPart  primitive_class;
    XmSashClassPart    sash_class;
} XmSashClassRec;

typedef struct _XmSashClassRec *XmSashWidgetClass;

externalref XmSashClassRec xmSashClassRec;

/* New fields for the Sash widget record */
typedef struct {
  XtCallbackList sash_action;
  Boolean has_focus;
} XmSashPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _XmSashRec {
   CorePart         core;
   XmPrimitivePart  primitive;
   XmSashPart       sash;
} XmSashRec;

typedef struct _XmSashRec      *XmSashWidget;

typedef struct {
  XEvent *event;		/* the event causing the SashAction */
  String *params;		/* the TranslationTable params */
  Cardinal num_params;		/* count of params */
} SashCallDataRec, *SashCallData;

/* Class Record Constant */

externalref WidgetClass xmSashWidgetClass;

#ifndef XmIsSash
#define XmIsSash(w)	XtIsSubclass(w, xmSashWidgetClass)
#endif /* XmIsSash */


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSashP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
