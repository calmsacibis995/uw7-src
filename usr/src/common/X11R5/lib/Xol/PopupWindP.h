#ifndef	NOIDENT
#ident	"@(#)popupwindo:PopupWindP.h	1.14"
#endif

#ifndef _PopupWindowShellP_h
#define _PopupWindowShellP_h
#include	<X11/ShellP.h>
#include	<Xol/PopupWindo.h> 


typedef struct
{
    char no_class_fields;		/* Makes compiler happy */
} PopupWindowShellClassPart;

typedef struct _PropWindowClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	WMShellClassPart	wm_shell_class;
	VendorShellClassPart	vendor_shell_class;
	TransientShellClassPart		transient_shell_class;
	PopupWindowShellClassPart	popup_shell_class;
} PopupWindowShellClassRec;

extern PopupWindowShellClassRec popupWindowShellClassRec;

/* New fields for the Open Look Popup Window widget */

#define OK_ITEM               0   /* Motif mode only */
#define APPLY_ITEM            1
#define SET_DEFAULT_ITEM      2
#define RESET_ITEM            3
#define RESET_TO_FACTORY_ITEM 4
#define CANCEL_ITEM           5   
#define MAX_BUTTONS           6

typedef struct {
	Widget		upperControlArea;
	Widget		lowerControlArea;
	Widget		footerPanel;
	Boolean		propchange;
	XtCallbackList	apply;
	XtCallbackList	setDefaults;
	XtCallbackList	reset;
	XtCallbackList	resetFactory;
	XtCallbackList	verify;
	XtCallbackList	cancel;
	Widget		menu;
} PopupWindowShellPart;

typedef struct _PopupWindowShellRec
{
	CorePart 	core;
	CompositePart 	composite;
	ShellPart 	shell;
	WMShellPart	wm;
	VendorShellPart	vendor;
	TransientShellPart	transient;
	PopupWindowShellPart	popupwindow;
} PopupWindowShellRec;

extern void _OlPWBringDownPopup OL_ARGS((Widget,Boolean));
extern void _OlmPWAddButtons OL_ARGS((PopupWindowShellWidget));
extern void _OloPWAddButtons OL_ARGS((PopupWindowShellWidget));
#endif
