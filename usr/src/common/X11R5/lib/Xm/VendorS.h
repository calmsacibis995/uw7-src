#pragma ident	"@(#)m1.2libs:Xm/VendorS.h	1.2"
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
#ifndef _XmVendorS_h
#define _XmVendorS_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsVendorShell
#define XmIsVendorShell(w)	XtIsSubclass(w, vendorShellWidgetClass)
#endif /* XmIsVendorShell */

typedef struct _XmVendorShellRec *XmVendorShellWidget;
typedef struct _XmVendorShellClassRec *XmVendorShellWidgetClass;
externalref WidgetClass vendorShellWidgetClass;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean XmIsMotifWMRunning() ;

#else

extern Boolean XmIsMotifWMRunning( 
                        Widget shell) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmVendorS_h */
/* DON'T ADD STUFF AFTER THIS #endif */
