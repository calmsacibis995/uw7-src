#pragma ident	"@(#)m1.2libs:Xm/ScrolledWP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmScrolledWindowP_h
#define _XmScrolledWindowP_h

#include <Xm/ManagerP.h>
#include <Xm/ScrolledW.h>

#include <Xm/ScrollBar.h>
#include <Xm/DrawingA.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constraint part record for ScrolledWindow widget */
typedef struct _XmScrolledWindowConstraintPart
{
   unsigned char child_type;
} XmScrolledWindowConstraintPart, * XmScrolledWindowConstraint;


/* New fields for the ScrolledWindow widget class record */
typedef struct {
   XtPointer extension;   /* Pointer to extension record */
} XmScrolledWindowClassPart;

/****************
 *
 * Class record declaration
 *
 ****************/
typedef struct _XmScrolledWindowClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
    XmScrolledWindowClassPart	swindow_class;
} XmScrolledWindowClassRec;

externalref XmScrolledWindowClassRec xmScrolledWindowClassRec;

/****************
 *
 * Scrolled Window instance structure.
 *
 ****************/
typedef struct {

   int vmin;		  /*  slider minimum coordiate position     */
   int vmax;		  /*  slider maximum coordiate position     */
   int vOrigin;		  /*  slider edge location                  */
   int vExtent;		  /*  slider size                           */

   int hmin;		  /*  Same as above for horizontal bar.     */
   int hmax;
   int hOrigin;
   int hExtent;

   Position hsbX,hsbY;
   Dimension hsbWidth,hsbHeight;    /* Dimensions for the horiz bar */

   Position vsbX,vsbY;
   Dimension vsbWidth,vsbHeight;    /* Dimensions for the vertical bar */

   Dimension    GivenHeight, GivenWidth;

   Dimension	AreaWidth,AreaHeight;
   Dimension	WidthPad,HeightPad;
   Position	XOffset, YOffset;

   Dimension	pad;

   Boolean	hasHSB;
   Boolean	hasVSB;
   Boolean	InInit;
   Boolean	FromResize;

   unsigned char	VisualPolicy;
   unsigned char	ScrollPolicy;
   unsigned char	ScrollBarPolicy;
   unsigned char	Placement;
   
   XmScrollBarWidget   	hScrollBar;
   XmScrollBarWidget   	vScrollBar;
   XmDrawingAreaWidget 	ClipWindow;
   Widget              	WorkWindow;
   
   XtCallbackList       traverseObscuredCallback;
} XmScrolledWindowPart;


/************************************************************************
 *									*
 * Full instance record declaration					*
 *									*
 ************************************************************************/

typedef struct _XmScrolledWindowRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart constraint;
    XmManagerPart   manager;
    XmScrolledWindowPart   swindow;
} XmScrolledWindowRec;

#define DEFAULT_HEIGHT 20
#define DEFAULT_WIDTH 20


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern char * _XmGetRealXlations() ;
extern void _XmInitializeScrollBars() ;

#else

extern char * _XmGetRealXlations( 
                        Display *dpy,
                        _XmBuildVirtualKeyStruct *keys,
                        int num_keys) ;
extern void _XmInitializeScrollBars( 
                        Widget w) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScrolledWindowP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
