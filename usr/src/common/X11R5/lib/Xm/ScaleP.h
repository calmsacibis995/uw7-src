#pragma ident	"@(#)m1.2libs:Xm/ScaleP.h	1.3"
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
#ifndef _XmScaleP_h
#define _XmScaleP_h


#include <Xm/Scale.h>
#include <Xm/ManagerP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constraint part record for Scale widget */

typedef struct _XmScaleConstraintPart
{
   char unused;
} XmScaleConstraintPart, * XmScaleConstraint;


/*  New fields for the Scale widget class record  */

typedef struct
{
   XtPointer extension;   /* Pointer to extension record */
} XmScaleClassPart;


/* Full class record declaration */

typedef struct _XmScaleClassRec
{
   CoreClassPart       core_class;
   CompositeClassPart  composite_class;
   ConstraintClassPart constraint_class;
   XmManagerClassPart  manager_class;
   XmScaleClassPart    scale_class;
} XmScaleClassRec;

externalref XmScaleClassRec xmScaleClassRec;


/* New fields for the Scale widget record */

typedef struct
{
   int            value;
   int            maximum;
   int            minimum;
   unsigned char  orientation;
   unsigned char  processing_direction;
   XmString       title; 
   XmFontList     font_list;
   XFontStruct  * font_struct;
   Boolean        show_value;
   short          decimal_points;
   Dimension      scale_width;
   Dimension      scale_height;
   Dimension      highlight_thickness;
   Boolean        highlight_on_enter;
   XtCallbackList drag_callback;
   XtCallbackList value_changed_callback;

   int last_value;
   int slider_size;
   GC  foreground_GC;
   int show_value_x;
   int show_value_y;
   int show_value_width;
   int show_value_height;
   int scale_multiple;
} XmScalePart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmScaleRec
{
    CorePart       core;
    CompositePart  composite;
    ConstraintPart constraint;
    XmManagerPart  manager;
    XmScalePart    scale;
} XmScaleRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScaleP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
