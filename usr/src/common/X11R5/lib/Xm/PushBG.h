#pragma ident	"@(#)m1.2libs:Xm/PushBG.h	1.2"
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

#ifndef _XmPButtonG_h
#define _XmPButtonG_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsPushButtonGadget
#define XmIsPushButtonGadget(w) XtIsSubclass(w, xmPushButtonGadgetClass)
#endif /* XmIsPushButtonGadget */

externalref WidgetClass xmPushButtonGadgetClass;

typedef struct _XmPushButtonGadgetClassRec   *XmPushButtonGadgetClass;
typedef struct _XmPushButtonGadgetRec        *XmPushButtonGadget;
typedef struct _XmPushButtonGCacheObjRec     *XmPushButtonGCacheObject;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreatePushButtonGadget() ;

#else

extern Widget XmCreatePushButtonGadget( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmPButtonG_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
