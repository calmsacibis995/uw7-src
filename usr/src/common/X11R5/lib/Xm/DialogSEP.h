#pragma ident	"@(#)m1.2libs:Xm/DialogSEP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
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
#ifndef _XmDialogShellExtP_h
#define _XmDialogShellExtP_h

#include <Xm/VendorSEP.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsDialogShellExt
#define XmIsDialogShellExt(w)	XtIsSubclass(w, xmDialogShellExtObjectClass)
#endif /* XmIsDialogShellExt */

externalref WidgetClass xmDialogShellExtObjectClass;

typedef struct _XmDialogShellExtClassRec	*XmDialogShellExtObjectClass ;
typedef struct _XmDialogShellExtRec		*XmDialogShellExtObject ;


typedef struct _XmDialogShellExtClassPart{
   XtPointer extension;   /* Pointer to extension record */
}XmDialogShellExtClassPart, *XmDialogShellExtClassPartPtr;

typedef struct _XmDialogShellExtClassRec{
    ObjectClassPart		object_class;
    XmExtClassPart		ext_class;
    XmDesktopClassPart		desktop_class;
    XmShellExtClassPart		shell_class;
    XmVendorShellExtClassPart 	vendor_class;
    XmDialogShellExtClassPart 	dialog_class;
}XmDialogShellExtClassRec;

typedef struct _XmDialogShellExtPart{
    int		      	empty;
} XmDialogShellExtPart;

externalref XmDialogShellExtClassRec 	xmDialogShellExtClassRec;

typedef struct _XmDialogShellExtRec{
    ObjectPart			object;
    XmExtPart			ext;
    XmDesktopPart		desktop;
    XmShellExtPart		shell;
    XmVendorShellExtPart 	vendor;
    XmDialogShellExtPart 	dialog;
}XmDialogShellExtRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDialogShellExtP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
