#pragma ident	"@(#)m1.2libs:Xm/TextFSelP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmTextFSelP_h
#define _XmTextFSelP_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean _XmTextFieldConvert() ;
extern void _XmTextFieldLoseSelection() ;

#else

extern Boolean _XmTextFieldConvert( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
extern void _XmTextFieldLoseSelection( 
                        Widget w,
                        Atom *selection) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmTextFSelP_h */
/* DON't ADD STUFF AFTER THIS #endif */
