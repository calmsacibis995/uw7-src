#pragma ident	"@(#)m1.2libs:Xm/DrawP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDrawP_h
#define _XmDrawP_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmDrawShadows() ;
extern void _XmClearBorder() ;
extern void _XmDrawSeparator() ;
extern void _XmDrawDiamond() ;
extern void _XmDrawHighlight() ;
extern void _XmDrawSimpleHighlight() ;
extern void _XmDrawArrow() ;

#else

extern void _XmDrawShadows( 
                        Display *display,
                        Drawable d,
                        GC top_gc,
                        GC bottom_gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shad_thick,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shad_thick,
#endif /* NeedWidePrototypes */
                        unsigned int shad_type) ;
extern void _XmClearBorder( 
                        Display *display,
                        Window w,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shadow_thick) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shadow_thick) ;
#endif /* NeedWidePrototypes */
extern void _XmDrawSeparator( 
                        Display *display,
                        Drawable d,
                        GC top_gc,
                        GC bottom_gc,
                        GC separator_gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shadow_thick,
                        int margin,
                        unsigned int orientation,
                        unsigned int separator_type) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shadow_thick,
                        Dimension margin,
                        unsigned char orientation,
                        unsigned char separator_type) ;
#endif /* NeedWidePrototypes */
extern void _XmDrawDiamond( 
                        Display *display,
                        Drawable d,
                        GC top_gc,
                        GC bottom_gc,
                        GC center_gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shadow_thick,
                        int fill) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shadow_thick,
                        Dimension fill) ;
#endif /* NeedWidePrototypes */
extern void _XmDrawSimpleHighlight( 
                        Display *display,
                        Drawable d,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int highlight_thick);
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension highlight_thick);
#endif /* NeedWidePrototypes */
extern void _XmDrawHighlight( 
                        Display *display,
                        Drawable d,
                        GC gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int highlight_thick,
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension highlight_thick,
#endif /* NeedWidePrototypes */
                        int line_style) ;
extern void _XmDrawArrow( 
                        Display *display,
                        Drawable d,
                        GC top_gc,
                        GC bot_gc,
                        GC cent_gc,
#if NeedWidePrototypes
                        int x,
                        int y,
                        int width,
                        int height,
                        int shadow_thick,
                        unsigned int direction) ;
#else
                        Position x,
                        Position y,
                        Dimension width,
                        Dimension height,
                        Dimension shadow_thick,
                        unsigned char direction) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDrawP_h */
