#pragma ident	"@(#)m1.2libs:Xm/Screen.h	1.2"
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

#ifndef _XmScreen_h
#define _XmScreen_h

#include <Xm/Xm.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsScreen
#define XmIsScreen(w) (XtIsSubclass(w, xmScreenClass))
#endif /* XmIsScreen */

/* Class record constants */

typedef struct _XmScreenRec *XmScreen;
typedef struct _XmScreenClassRec *XmScreenClass;
externalref 	WidgetClass xmScreenClass;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmGetXmScreen() ;

#else

extern Widget XmGetXmScreen( 
                        Screen *screen) ;
#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScreen_h */

