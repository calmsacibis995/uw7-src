#pragma ident	"@(#)m1.2libs:Xm/PushB.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
/***********************************************************************
 *
 * PushButton Widget
 *
 ***********************************************************************/

#ifndef _XmPButton_h
#define _XmPButton_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsPushButton
#define XmIsPushButton(w) XtIsSubclass(w, xmPushButtonWidgetClass)
#endif /* XmIsPushButton */

/* PushButton Widget */

externalref WidgetClass xmPushButtonWidgetClass;

typedef struct _XmPushButtonClassRec *XmPushButtonWidgetClass;
typedef struct _XmPushButtonRec      *XmPushButtonWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreatePushButton() ;

#else

extern Widget XmCreatePushButton( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmPButton_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
