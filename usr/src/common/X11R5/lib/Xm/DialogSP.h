#pragma ident	"@(#)m1.2libs:Xm/DialogSP.h	1.2"
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
#ifndef _XmDialogShellP_h
#define _XmDialogShellP_h

#include <Xm/XmP.h>
#include <Xm/DialogS.h>
#include <X11/ShellP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* macros: THESE BELONG IN XmP.h */
#define XtX(w)		 ((w)->core.x)
#define XtY(w)		 ((w)->core.y)
#define XtWidth(w)	 ((w)->core.width)
#define XtHeight(w)	 ((w)->core.height)
#define XtBorderWidth(w) ((w)->core.border_width)
#define XtBackground(w)	 ((w)->core.background_pixel)
#define XtSensitive(w)	 ((w)->core.sensitive && (w)->core.ancestor_sensitive) 

#ifndef XtParent
#define XtParent(w)	 ((w)->core.parent)
#endif


/* The DialogShell instance record */

typedef struct 
{
    /* internal fields */
    XtGrabKind 		grab_kind;
    Position		old_x, old_y;
} XmDialogShellPart;


/* Full instance record declaration */

typedef  struct _XmDialogShellRec 
{	
    CorePart		    core;
    CompositePart	    composite;
    ShellPart		    shell;
    WMShellPart		    wm;
    VendorShellPart	    vendor;
    TransientShellPart	    transient;
    XmDialogShellPart	    dialog;
} XmDialogShellRec;

typedef  struct _XmDialogShellWidgetRec /* OBSOLETE (for compatibility only).*/
{	
    CorePart		    core;
    CompositePart	    composite;
    ShellPart		    shell;
    WMShellPart		    wm;
    VendorShellPart	    vendor;
    TransientShellPart	    transient;
    XmDialogShellPart	    dialog;
} XmDialogShellWidgetRec;



/* DialogShell class structure */

typedef struct 
{
    XtPointer			extension;	 /* Pointer to extension record */
} XmDialogShellClassPart;


/* Full class record declaration */

typedef struct _XmDialogShellClassRec 
{
    CoreClassPart 		core_class;
    CompositeClassPart 		composite_class;
    ShellClassPart 		shell_class;
    WMShellClassPart	        wm_shell_class;
    VendorShellClassPart 	vendor_shell_class;
    TransientShellClassPart  	transient_shell_class;
    XmDialogShellClassPart 	dialog_shell_part;
} XmDialogShellClassRec;


externalref XmDialogShellClassRec  xmDialogShellClassRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDialogShellP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
