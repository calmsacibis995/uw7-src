#pragma ident	"@(#)m1.2libs:Xm/AtomMgrI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmAtomMgrI_h
#define _XmAtomMgrI_h

#include <Xm/AtomMgr.h>

#ifdef __cplusplus
extern "C" {
#endif

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmInternAtomAndName() ;

#else

extern void _XmInternAtomAndName( 
                        Display *display,
                        Atom atom,
                        String name) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmAtomMgrI_h */
