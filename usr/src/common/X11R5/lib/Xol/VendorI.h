#ifndef	NOIDENT
#ident	"@(#)mouseless:VendorI.h	1.21"
#endif

#ifndef _OL_VENDOR_I_H
#define _OL_VENDOR_I_H

/*
 ************************************************************************	
 * Description:
 *	This is the OPEN LOOK's internal private header file for
 * the vendor shell.  It has information associated with the class
 * extensions.
 *
 * Why a class extension?  Because if we don't use an extension, we
 * have to add fields to the VendorShellPartRec which means we must
 * recompile the Intrinsics library and therefore, our toolkit will
 * no longer be able to run on standard Intrinsics.
 *
 * Here's a summary of the extension scheme employed:
 * 	Each shell that is a subclass of a VendorShell has an extension
 * placed in their respective VendorShellClass's extension list.
 * Subclasses that wish to provide different default resource values
 * should explicitly place an extension on the list during their
 * static initialization or in their ClassInitialization.  (Note, a
 * subclass cannot do this in its ClassPartInitialization.)
 * If the subclass doesn't provide an extension, the VendorShell's
 * ClassPartInitialization procedure will create one.
 *	Subclasses that add an extension by static initialization must
 * set the 'record_type' field in the extension header to
 * the global XrmQuark OlXrmVendorClassExtension in their
 * ClassInitialization procedure.  This field cannot be set statically
 * since the VendorShell's ClassInitialization procedure does the
 * quarking of the string.
 *	Within the extension record is a pointer to the list of instance
 * data associated with each vendor widget instance.  The routine
 * _OlGetVendorPartExtension should be called by subclasses that want
 * to read the fields that are in the instance data.  NOTE: for
 * widgets that want to access the OlFocusData, they should use
 * _OlGetFocusData directly.
 ************************************************************************	
 */

#include <X11/ShellP.h>	/* Must include because X11/VendorP.h doesn't	*/
#include <X11/VendorP.h>
#include <Xol/OpenLookP.h>
#include <Xol/array.h>		/* for traversal list */

/*
 ************************************************************************	
 * Declare some externals
 ************************************************************************	
 */
typedef struct {
	Widget		focus_gadget;		/* monitoring gadgets	*/
	Widget		initial_focus_widget;	/* initial widget to get focus*/
	Widget		current_focus_widget;	/* current widget w/focus */
	Widget		activate_on_focus;	/* activate when focus arrives*/
	Widget		previous_focus_widget;	/* obsoleted	*/
	WidgetArray	traversal_list;
	OlDefine	focus_model;		/* current focus model	*/
	Boolean		resort_list;		/* re-sort traversal list */
} OlFocusData;

typedef struct OlShellList {
	Widget *	shells;
	Cardinal	nshells;
} OlShellList;

typedef struct OlIcGeometry {
    OlIc *		ic;
    unsigned short 	width;
    unsigned short 	height;
} OlIcGeometry;

typedef _OlArrayStruct(OlIcGeometry, OlIcArray) OlIcArray;

typedef struct _OlVendorPartExtensionRec {
	struct _OlVendorPartExtensionRec * next;/* next instance node	*/
	struct _OlVendorClassExtensionRec *
				class_extension; /* pointer to class ext.*/	
	Widget			vendor;		/* vendor widget's id	*/
	XtPointer		user_data;	/* Application data hook*/
	Boolean			busy;		/* busy window ?	*/
	Boolean			menu_button;	/* menu button ?  	*/
	Boolean			resize_corners;	/* has resize corners ?	*/
	Boolean			window_header;	/* has window header ?	*/
	Boolean			accelerators_do_grab;
	unsigned char		dyn_flags;	/* dynamic resource flags */
	XtCallbackList		consume_event;	/* consumeEvent callback */
	XtCallbackList		wm_protocol;	/* WM protocol callback */
	OlDefine		menu_type;	/* WM menu type		*/
	OlDefine		pushpin;	/* pushpin state	*/
	OlDefine		win_type;	/* window type (private)*/
	Widget			default_widget;	/* shell's default widget*/
	OlFocusData		focus_data;	/* focus management	*/
	struct OlAcceleratorList *		/* list of accelerators */
				accelerator_list; /* and mnemonics      */
	struct OlShellList *	shell_list;	/* shell descendants    */
	OlBitMask 		wm_protocol_mask;/* WM protocol mask	*/
	unsigned short		a_m_index;	/* accle, mnemonic  	*/
						/* update index - i18n	*/
	Widget			menubar_widget;	/* for menubars - MooLIT*/
	OlIcArray		ic_list;        /* input method ic's - i18n */
	XtWidgetGeometry 	status_area_geometry; /*input method status area */
} OlVendorPartExtensionRec, *OlVendorPartExtension;
	
/*
 ************************************************************************	
 * Declare some new types and inheritance tokens
 ************************************************************************	
 */

typedef void	(*OlSetDefaultProc) OL_ARGS((
    Widget,	/* shell;		shell widget's id	*/
    Widget	/* new_default;		new default widget	*/
));
#define XtInheritSetDefault		((OlSetDefaultProc) _XtInherit)

typedef void	(*OlGetDefaultProc) OL_ARGS((
    Widget,	/* shell;		shell widget's id	*/
    Widget	/* the_default;		current default		*/
));
#define XtInheritGetDefault		((OlGetDefaultProc) _XtInherit)

typedef struct _OlVendorClassExtensionRec {
	OlClassExtensionRec	header;		/* required header	*/
	XtResourceList		resources;	/* extension resources	*/
	Cardinal		num_resources;	/* number of resources	*/
	XtResourceList		private;	/* private resources	*/
	OlSetDefaultProc	set_default;	/* set shell's default	*/
	OlGetDefaultProc	get_default;	/* ping subclass	*/
	OlExtDestroyProc	destroy;	/* extension destroy	*/
	OlExtInitializeProc	initialize;	/* extension initialize */
	OlExtSetValuesFunc	set_values;	/* extension setvalues  */
	OlExtGetValuesProc	get_values;	/* extension getvalues  */
	OlTraversalFunc		traversal_handler;
	OlHighlightProc		highlight_handler;
	OlActivateFunc		activate;
	OlEventHandlerList	event_procs;
	Cardinal		num_event_procs;
	OlVendorPartExtension	part_list;	/* instance data list	*/
	_OlDynData		dyn_data;	/* dyn_data		*/
	OlTransparentProc	transparent_proc;
	OlWMProtocolProc	wm_proc;	/* wm msg handler	*/
	Boolean			override_callback; 
} OlVendorClassExtensionRec, *OlVendorClassExtension;

	/* Define a version field number that reflects the version of
	 * this release's vendor shell extension.  Newer versions
	 * should distinguish themselves from older ones by using a
	 * different version number.					*/

#define OlVendorClassExtensionVersion	1L

#define OL_B_VENDOR_BG	(1 << 0)
#define OL_B_VENDOR_BORDERCOLOR	(1 << 1)

/* Moolit definitions */

typedef struct
{
    long	flags;
    long	functions;
    long	decorations;
    int		inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints	PropMwmHints;


/* number of elements of size 32 in _MWM_HINTS */
#define PROP_MOTIF_WM_HINTS_ELEMENTS	4
#define PROP_MWM_HINTS_ELEMENTS		PROP_MOTIF_WM_HINTS_ELEMENTS

/* atom name for _MWM_HINTS property */
#define _XA_MOTIF_WM_HINTS	"_MOTIF_WM_HINTS"
#define _XA_MWM_HINTS		_XA_MOTIF_WM_HINTS

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS	(1L << 0)
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL		(1L << 0)
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL		(1L << 0)
#define MWM_DECOR_BORDER	(1L << 1)
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

#define RESIZE_CONSTANT		((unsigned int) 1 << 0)
#define HEADER_CONSTANT		((unsigned int) 1 << 1)
#define CLOSE_CONSTANT		((unsigned int) 1 << 2)

/*
 ************************************************************************	
 * Declare some externals
 ************************************************************************	
 */

extern XrmQuark	OlXrmVendorClassExtension;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

extern OlVendorClassExtension
_OlGetVendorClassExtension OL_ARGS((
	WidgetClass		/* vendor_class;	subclass	*/
));

extern OlVendorPartExtension
_OlGetVendorPartExtension OL_ARGS((
	Widget			/* vendor;	subclass instance	*/
));

extern OlFocusData *
_OlGetFocusData OL_ARGS((
	Widget,			/* widget;	start search here	*/
	OlVendorPartExtension *	/* part_ptr;	pointer or NULL		*/
));

extern void
_OlSetPinState OL_ARGS((
	Widget,			/* widget; 	widget in question	*/
	OlDefine		/* pinstate;	new pin state */
));

extern int
_OlGetMWMHints OL_ARGS((
	Display *,
	Window,
	PropMwmHints *
));

OLEndFunctionPrototypeBlock

#endif /* _OL_VENDOR_I_H */
