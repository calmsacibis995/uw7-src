#pragma ident	"@(#)m1.2libs:Xm/LabelG.h	1.2"
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
#ifndef _XmLabelG_h
#define _XmLabelG_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Widget class and record definitions  */


externalref WidgetClass xmLabelGadgetClass;

typedef struct _XmLabelGadgetClassRec * XmLabelGadgetClass;
typedef struct _XmLabelGadgetRec      * XmLabelGadget;
typedef struct _XmLabelGCacheObjRec   * XmLabelGCacheObject;

/*fast subclass define */
#ifndef XmIsLabelGadget
#define XmIsLabelGadget(w)     XtIsSubclass(w, xmLabelGadgetClass)
#endif /* XmIsLabelGadget */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateLabelGadget() ;

#else

extern Widget XmCreateLabelGadget( 
                        Widget parent,
                        char *name,
                        Arg *arglist,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmLabelG_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
