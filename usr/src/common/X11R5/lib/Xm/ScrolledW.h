#pragma ident	"@(#)m1.2libs:Xm/ScrolledW.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmScrolledWindow_h
#define _XmScrolledWindow_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsScrolledWindow
#define XmIsScrolledWindow(w)	XtIsSubclass(w, xmScrolledWindowWidgetClass)
#endif /* XmIsScrolledWindow */


externalref WidgetClass xmScrolledWindowWidgetClass;

typedef struct _XmScrolledWindowClassRec * XmScrolledWindowWidgetClass;
typedef struct _XmScrolledWindowRec      * XmScrolledWindowWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmScrolledWindowSetAreas() ;
extern Widget XmCreateScrolledWindow() ;
extern void XmScrollVisible() ;

#else

extern void XmScrolledWindowSetAreas( 
                        Widget w,
                        Widget hscroll,
                        Widget vscroll,
                        Widget wregion) ;
extern Widget XmCreateScrolledWindow( 
                        Widget parent,
                        char *name,
                        ArgList args,
                        Cardinal argCount) ;
extern void XmScrollVisible(
			Widget      	scrw,
			Widget          wid,
			Dimension       hor_margin, 
			Dimension       ver_margin) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScrolledWindow_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
