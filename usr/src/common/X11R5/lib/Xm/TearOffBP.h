#pragma ident	"@(#)m1.2libs:Xm/TearOffBP.h	1.2"
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
 *  TearOffBP.h - Private definitions for TearOffButton widget 
 *  (Used by RowColumn Tear Off Menupanes)
 *
 */

#ifndef _XmTearOffBP_h
#define _XmTearOffBP_h

#include <Xm/PushBP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 *
 * TearOffButton Widget Private Data
 *
 *****************************************************************************/

/* New fields for the TearOffButton widget class record */
typedef struct _XmTearOffButtonClassPart
{
    String translations;
} XmTearOffButtonClassPart;

/* Full Class record declaration */
typedef struct _XmTearOffButtonClassRec {
    CoreClassPart         core_class;
    XmPrimitiveClassPart  primitive_class;
    XmLabelClassPart      label_class;
    XmPushButtonClassPart pushbutton_class;
    XmTearOffButtonClassPart    tearoffbutton_class;
} XmTearOffButtonClassRec;

typedef struct _XmTearOffButtonClassRec *XmTearOffButtonWidgetClass;

externalref XmTearOffButtonClassRec xmTearOffButtonClassRec;

/* New fields for the TearOffButton widget record */
typedef struct {
   Dimension      margin;
   unsigned char  orientation;
   unsigned char separator_type;
   GC separator_GC;
} XmTearOffButtonPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _XmTearOffButtonRec {
   CorePart         core;
   XmPrimitivePart  primitive;
   XmLabelPart      label;
   XmPushButtonPart pushbutton;
   XmTearOffButtonPart tear_off_button;
} XmTearOffButtonRec;

typedef struct _XmTearOffButtonRec      *XmTearOffButtonWidget;

/* Class Record Constant */

externalref WidgetClass xmTearOffButtonWidgetClass;

#ifndef XmIsTearOffButton
#define XmIsTearOffButton(w)	XtIsSubclass(w, xmTearOffButtonWidgetClass)
#endif /* XmIsTearOffButton */


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTearOffButtonP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
