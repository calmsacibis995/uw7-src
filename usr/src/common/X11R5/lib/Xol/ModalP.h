#ifndef	NOIDENT
#ident	"@(#)notice:ModalP.h	1.5"
#endif

/* 
 * ModalP.h - Private definitions for Modal Shell widget
 */

#ifndef _ModalP_h
#define _ModalP_h

/*
 * Modal Widget Private Data
 */

#include <X11/ShellP.h>
#include <Xol/Modal.h>
#include <Xol/Olg.h>

/***********************************************************************
 *
 *	Class structure
 *
 **********************************************************************/

/* New fields for the Modal widget class record */
typedef struct {
    XtPointer no_class_fields;	/* make compiler happy */
} ModalShellClassPart;

/* Full class record declaration */
typedef struct _ModalShellClassRec {
    CoreClassPart		core_class;
    CompositeClassPart  	composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart            wm_shell_class;
    VendorShellClassPart        vendor_shell_class;
    TransientShellClassPart	transient_shell_class;
    ModalShellClassPart	modal_shell_class;
} ModalShellClassRec;

/* Class record variable */
externalref ModalShellClassRec modalShellClassRec;

/***********************************************************************
 *
 *	Instance (widget) structure
 *
 **********************************************************************/

/* New fields for the Modal widget record */
typedef struct {
    /* "public" (resource) members */
    Widget	emanate;	/* originating widget */
    Boolean	warp_pointer;
    OlDefine	noticeType;	/* Motif mode */

    /* private members */
    Pixmap	pixmap;		/* pix(bit)map widget, Motif mode */
    Boolean	do_unwarp;	/* unwarp only when no motion */
    int		root_x;		/* to restore warped pointer */
    int		root_y;		/* to restore warped pointer */
    OlgAttrs *	attrs;		/* to draw borders and lines */
    Position	line_y;		/* Motif mode line position */
} ModalShellPart;

/* Full instance record declaration */
typedef struct _ModalShellRec {
    CorePart		core;
    CompositePart	composite;
    ShellPart		shell;
    WMShellPart         wm;
    VendorShellPart     vendor;
    TransientShellPart	transient;
    ModalShellPart	modal_shell;
} ModalShellRec;

/*
 * Private types:
 */

typedef enum HowAsked {
	ParentQueried,
	ChildQueried,
	PleaseTry
}			HowAsked;

/*  declarations for dynamically linked procedures, both
 *  Motif (_Olm) and OPEN LOOK (_Olo) versions.
 *  (These are needed for building an archive library.)
 */
extern void _OlmMDAddEventHandlers OL_ARGS((Widget));
extern Boolean	_OlmMDCheckSetValues OL_ARGS((ModalShellWidget,	
															ModalShellWidget));
extern void	_OlmMDLayout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
extern void	_OlmMDRemoveEventHandlers OL_ARGS((Widget));
extern void	_OlmMDRedisplay OL_ARGS((Widget,	XEvent *, Region));

extern void _OloMDAddEventHandlers OL_ARGS((Widget));
extern Boolean	_OloMDCheckSetValues OL_ARGS((ModalShellWidget,	
															ModalShellWidget));
extern void	_OloMDLayout OL_ARGS(( Widget, Boolean, Boolean, Boolean,
	Widget, XtWidgetGeometry *, XtWidgetGeometry *));
extern void	_OloMDRemoveEventHandlers OL_ARGS((Widget));
extern void	_OloMDRedisplay OL_ARGS((Widget,	XEvent *, Region));



#endif /* _ModalP_h */
