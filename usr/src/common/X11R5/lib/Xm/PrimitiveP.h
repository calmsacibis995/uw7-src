#pragma ident	"@(#)m1.2libs:Xm/PrimitiveP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1989, 1990 DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#ifndef _XmPrimitiveP_h
#define _XmPrimitiveP_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Access Macros */

#define Prim_ShadowThickness(w) (((XmPrimitiveWidget)(w))->primitive.shadow_thickness)
#define Prim_HaveTraversal(w) (((XmPrimitiveWidget)(w))->primitive.have_traversal)

#define PCEPTR(wc)  ((XmPrimitiveClassExt *)(&(((XmPrimitiveWidgetClass)(wc))->primitive_class.extension)))
#define _XmGetPrimitiveClassExtPtr(wc, owner) \
  ((*PCEPTR(wc) && (((*PCEPTR(wc))->record_type) == owner))\
   ? PCEPTR(wc) \
   :((XmPrimitiveClassExt *) _XmGetClassExtensionPtr(((XmGenericClassExt *)PCEPTR(wc)), owner)))


#define XmPrimitiveClassExtVersion 1L


typedef struct _XmPrimitiveClassExtRec{
    XtPointer           next_extension;
    XrmQuark            record_type;
    long                version;
    Cardinal            record_size;
    XmWidgetBaselineProc widget_baseline;
    XmWidgetDisplayRectProc  widget_display_rect;
    XmWidgetMarginsProc widget_margins;
}XmPrimitiveClassExtRec, *XmPrimitiveClassExt;

typedef struct _XmPrimitiveClassPart
{
   XtWidgetProc         border_highlight;
   XtWidgetProc         border_unhighlight;
   String               translations;
   XtActionProc         arm_and_activate;
   XmSyntheticResource * syn_resources;   
   int                  num_syn_resources;   
   XtPointer            extension;
} XmPrimitiveClassPart;

typedef struct _XmPrimitiveClassRec
{
    CoreClassPart        core_class;
    XmPrimitiveClassPart primitive_class;
} XmPrimitiveClassRec;

#ifndef PRIMITIVE
externalref XmPrimitiveClassRec xmPrimitiveClassRec;
#endif


/*  The Primitive instance record  */

typedef struct _XmPrimitivePart
{
   Pixel   foreground;

   Dimension   shadow_thickness;
   Pixel   top_shadow_color;
   Pixmap  top_shadow_pixmap;
   Pixel   bottom_shadow_color;
   Pixmap  bottom_shadow_pixmap;

   Dimension   highlight_thickness;
   Pixel   highlight_color;
   Pixmap  highlight_pixmap;

   XtCallbackList help_callback;
   XtPointer      user_data;

   Boolean traversal_on;
   Boolean highlight_on_enter;
   Boolean have_traversal;

   unsigned char unit_type;
   XmNavigationType navigation_type;

   Boolean highlight_drawn;
   Boolean highlighted;

   GC      highlight_GC;
   GC      top_shadow_GC;
   GC      bottom_shadow_GC;
} XmPrimitivePart;

typedef struct _XmPrimitiveRec
{
   CorePart        core;
   XmPrimitivePart primitive;
} XmPrimitiveRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmTraverseLeft() ;
extern void _XmTraverseRight() ;
extern void _XmTraverseUp() ;
extern void _XmTraverseDown() ;
extern void _XmTraverseNext() ;
extern void _XmTraversePrev() ;
extern void _XmTraverseHome() ;
extern void _XmTraverseNextTabGroup() ;
extern void _XmTraversePrevTabGroup() ;
extern void _XmPrimitiveHelp() ;
extern void _XmPrimitiveParentActivate() ;
extern void _XmPrimitiveParentCancel() ;
extern Boolean _XmDifferentBackground() ;

#else

extern void _XmTraverseLeft( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseRight( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseUp( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseDown( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseNext( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraversePrev( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseHome( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraverseNextTabGroup( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmTraversePrevTabGroup( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveHelp( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveParentActivate( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern void _XmPrimitiveParentCancel( 
                        Widget pw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern Boolean _XmDifferentBackground( 
                        Widget w,
                        Widget parent) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmPrimitiveP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
