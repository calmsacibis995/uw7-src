#pragma ident	"@(#)m1.2libs:Xm/FileSB.h	1.2"
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
#ifndef _XmFSelect_h
#define _XmFSelect_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Type definitions for FileSB resources: */

#ifdef _NO_PROTO

typedef void (*XmQualifyProc)() ;
typedef void (*XmSearchProc)() ;

#else

typedef void (*XmQualifyProc)( Widget, XtPointer, XtPointer) ;
typedef void (*XmSearchProc)( Widget, XtPointer) ;

#endif


/* Class record constants */

externalref WidgetClass xmFileSelectionBoxWidgetClass;

typedef struct _XmFileSelectionBoxClassRec * XmFileSelectionBoxWidgetClass;
typedef struct _XmFileSelectionBoxRec      * XmFileSelectionBoxWidget;


#ifndef XmIsFileSelectionBox
#define XmIsFileSelectionBox(w) (XtIsSubclass((w),xmFileSelectionBoxWidgetClass))
#endif


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmFileSelectionBoxGetChild() ;
extern void XmFileSelectionDoSearch() ;
extern Widget XmCreateFileSelectionBox() ;
extern Widget XmCreateFileSelectionDialog() ;

#else

extern Widget XmFileSelectionBoxGetChild( 
                        Widget fs,
#if NeedWidePrototypes
                        unsigned int which) ;
#else
                        unsigned char which) ;
#endif /* NeedWidePrototypes */
extern void XmFileSelectionDoSearch( 
                        Widget fs,
                        XmString dirmask) ;
extern Widget XmCreateFileSelectionBox( 
                        Widget p,
                        String name,
                        ArgList args,
                        Cardinal n) ;
extern Widget XmCreateFileSelectionDialog( 
                        Widget ds_p,
                        String name,
                        ArgList fsb_args,
                        Cardinal fsb_n) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmFSelect_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
