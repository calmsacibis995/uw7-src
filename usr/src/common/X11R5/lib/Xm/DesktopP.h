#pragma ident	"@(#)m1.2libs:Xm/DesktopP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef  _XmDesktopP_h
#define _XmDesktopP_h

#include <Xm/ExtObjectP.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsDesktopObject
#define XmIsDesktopObject(w)	XtIsSubclass(w, xmDesktopClass)
#endif /* XmIsDesktopObject */

typedef struct _XmDesktopRec *XmDesktopObject;
typedef struct _XmDesktopClassRec *XmDesktopObjectClass;
externalref WidgetClass xmDesktopClass;


typedef struct _XmDesktopClassPart{
    WidgetClass		child_class;
    XtWidgetProc	insert_child;	  /* physically add child to parent  */
    XtWidgetProc      	delete_child;	  /* physically remove child	     */
    XtPointer		extension;
}XmDesktopClassPart, *XmDesktopClassPartPtr;

typedef struct _XmDesktopClassRec{
    ObjectClassPart		object_class;
    XmExtClassPart		ext_class;
    XmDesktopClassPart 		desktop_class;
}XmDesktopClassRec;

typedef struct {
    Widget		parent;
    Widget		*children;
    Cardinal		num_children;
    Cardinal		num_slots;
} XmDesktopPart, *XmDesktopPartPtr;

externalref XmDesktopClassRec 	xmDesktopClassRec;

typedef struct _XmDesktopRec{
    ObjectPart			object;
    XmExtPart			ext;
    XmDesktopPart		desktop;
}XmDesktopRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern WidgetClass _XmGetActualClass() ;
extern void _XmSetActualClass() ;

#else

extern WidgetClass _XmGetActualClass( 
                        Display *display,
                        WidgetClass w_class) ;
extern void _XmSetActualClass( 
                        Display *display,
                        WidgetClass w_class,
                        WidgetClass actualClass) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmDesktopP_h */
