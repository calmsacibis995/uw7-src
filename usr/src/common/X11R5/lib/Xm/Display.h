#pragma ident	"@(#)m1.2libs:Xm/Display.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDisplay_h
#define _XmDisplay_h

#include <Xm/Xm.h>
#include <X11/Shell.h>
#include <Xm/DragC.h>
#include <Xm/DropSMgr.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsDisplay
#define XmIsDisplay(w) (XtIsSubclass(w, xmDisplayClass))
#endif /* XmIsXmDisplay */

enum {
	XmDRAG_NONE,
	XmDRAG_DROP_ONLY,
	XmDRAG_PREFER_PREREGISTER,
	XmDRAG_PREREGISTER,
	XmDRAG_PREFER_DYNAMIC,
	XmDRAG_DYNAMIC,
	XmDRAG_PREFER_RECEIVER
};

/* Class record constants */

typedef struct _XmDisplayRec *XmDisplay;
typedef struct _XmDisplayClassRec *XmDisplayClass;
externalref 	WidgetClass xmDisplayClass;

#define XmGetDisplay(w) XmGetXmDisplay(XtDisplayOfObject(w))


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmGetDragContext() ;
extern Widget XmGetXmDisplay() ;

#else

extern Widget XmGetDragContext( 
                        Widget w,
                        Time time) ;
extern Widget XmGetXmDisplay( 
                        Display *display) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDisplay_h */


