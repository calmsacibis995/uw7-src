#ifndef	NOIDENT
#ident	"@(#)notice:NoticeP.h	1.17"
#endif

/* 
 * NoticeP.h - Private definitions for Notice widget
 */

#ifndef _NoticeP_h
#define _NoticeP_h

/*
 * Notice Widget Private Data
 */

#include <Xol/ModalP.h>
#include <Xol/Notice.h>

/***********************************************************************
 *
 *	Class structure
 *
 **********************************************************************/

/* New fields for the Notice widget class record */
typedef struct {
    XtPointer no_class_fields;	/* make compiler happy */
} NoticeShellClassPart;

/* Full class record declaration */
typedef struct _NoticeShellClassRec {
    CoreClassPart		core_class;
    CompositeClassPart  	composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart            wm_shell_class;
    VendorShellClassPart        vendor_shell_class;
    TransientShellClassPart	transient_shell_class;
    ModalShellClassPart		modal_shell_class;
    NoticeShellClassPart	notice_shell_class;
} NoticeShellClassRec;

/* Class record variable */
externalref NoticeShellClassRec noticeShellClassRec;

/***********************************************************************
 *
 *	Instance (widget) structure
 *
 **********************************************************************/

/* New fields for the Notice widget record */
typedef struct {
    /* "public" (resource) members */
    Widget	control;	/* control box for buttons */
    Widget	text;		/* notice text widget */
} NoticeShellPart;

/* Full instance record declaration */
typedef struct _NoticeShellRec {
    CorePart		core;
    CompositePart	composite;
    ShellPart		shell;
    WMShellPart         wm;
    VendorShellPart     vendor;
    TransientShellPart	transient;
    ModalShellPart	modal_shell;
    NoticeShellPart	notice_shell;
} NoticeShellRec;

#endif /* _NoticeP_h */
