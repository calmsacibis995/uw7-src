#pragma ident	"@(#)m1.2libs:Xm/Separator.h	1.2"
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
/*  Separator Widget  */
#ifndef _XmSeparator_h
#define _XmSeparator_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsSeparator
#define XmIsSeparator(w) XtIsSubclass(w, xmSeparatorWidgetClass)
#endif /* XmIsSeparator */

externalref WidgetClass xmSeparatorWidgetClass;

typedef struct _XmSeparatorClassRec * XmSeparatorWidgetClass;
typedef struct _XmSeparatorRec      * XmSeparatorWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateSeparator() ;

#else

extern Widget XmCreateSeparator( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSeparator_h */
/* DON'T ADD STUFF AFTER THIS #endif */
