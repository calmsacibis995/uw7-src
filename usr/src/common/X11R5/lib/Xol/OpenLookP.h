#ifndef	NOIDENT
#ident	"@(#)olmisc:OpenLookP.h	1.72"
#endif

#ifndef __OpenLookP_h__
#define __OpenLookP_h__

/*
 *************************************************************************
 *
 * Description:
 * 		This is the include file for the OPEN LOOK (TM - AT&T)
 *		routines which are private to the toolkit.
 * 
 **************************file*header************************************
 */

#include <Xol/OpenLook.h>
#include <Xol/Error.h>

/*
 ************************************************************************
 *	Define True Constant Tokens
 *		Tokens that appear in this section should not have
 *		their values changed.
 ************************************************************************
 */

#define OL_VERSION		5
#define OL_REVISION		0
#define OlVersion		(OL_VERSION * 1000 + OL_REVISION)

#define OL_DEFAULT_POINT_SIZE	12
#define OL_DEFAULT_FONT_NAME	Nlucida
#define OL_DEFAULT_MOTIF_FONT	Nhelvetica

#define OL_MAX_VIRTUAL_MAPPINGS	6

/*************************************************************************
 *
 *	Macros
 */

#define _OlMax(x, y)	(((x) > (y)) ? (x) : (y))
#define _OlMin(x, y)	(((x) < (y)) ? (x) : (y))
#define _OlAssignMax(x, y)  if ((y) > (x)) x = (y)
#define _OlAssignMin(x, y)  if ((y) < (x)) x = (y)

						/* String length macro	*/

#define _OlStrlen(string)	((string) ? strlen((OLconst char *)(string)) : 0)
				/* geometry-related convenience macros	*/

#define _OlScreenWidth(w)	(WidthOfScreen(XtScreenOfObject(w)))
#define _OlScreenHeight(w)	(HeightOfScreen(XtScreenOfObject(w)))
#define _OlWidgetWidth(w)	((w)->core.width + 2*(w)->core.border_width)
#define _OlWidgetHeight(w)	((w)->core.height + 2*(w)->core.border_width)

#define OlWidgetClassToClassName(wc)	((wc)->core_class.class_name)
#define OlWidgetToClassName(w)		OlWidgetClassToClassName(XtClass(w))

#ifndef	XA_CLIPBOARD
#define XA_CLIPBOARD(d)	XInternAtom(d, "CLIPBOARD", False)
#endif

	/* See Primitive:Initialize() for explanation...	*/
#define _OlLoadDefaultFont(w,f)					\
		if ((f) == (XFontStruct *)NULL) {		\
			(f) = _OlGetDefaultFont((w), NULL);	\
			OlVaDisplayWarningMsg(			\
				XtDisplayOfObject((w)),		\
				OleNbadFont,			\
				OleTdefaultOLFont,		\
				OleCOlToolkitWarning,		\
				OleMbadFont_defaultOLFont,	\
				XtName((w)),			\
				OlWidgetToClassName((w)));	\
		} else

			/* Define Mask Arg List structure and macro	*/

typedef struct {
	String		name;		/* resource name */
	XtArgVal	value;		/* resource value */
	int		rule;		/* comparision rule (see below) */

} MaskArg, *MaskArgList;

#define _OlSetMaskArg(m_arg, n, v, r) \
	((m_arg).name = (String) (n), (m_arg).value = (XtArgVal) (v), \
	(m_arg).rule = (int) (r))

    /*	Convenience macro to point to desired field within one of the
	fundamental OpenLook widget class records.
     */
#define _OlGetClassField(w, field, dest)				\
    {									\
	WidgetClass __ol_class = _OlClass(w);				\
									\
	if (__ol_class == primitiveWidgetClass)				\
	    dest = ((PrimitiveWidgetClass)XtClass(w))->primitive_class.field; \
									\
	else if (__ol_class == eventObjClass)				\
	    dest = ((EventObjClass)XtClass(w))->event_class.field;	\
									\
	else if (__ol_class == managerWidgetClass)			\
	    dest = ((ManagerWidgetClass)XtClass(w))->manager_class.field; \
    }

/*
 *  Define macros used to dynamically resolve GUI-dependent symbols.
 *  They hide the difference between archive libraries, which
 *  use direct assignment to resolve the symbols, and shared
 *  libraries, which use the dynamic linking library interface.
 *
 *  OLRESOLVESTART 	
 *	Is required as the first line in an initialization 
 *	code segment. It takes no parameters. 
 *
 *  OLRESOLVE(symbol,fptr) 
 *	Zero or more occurrences of OLRESOLVE may appear in an initialization
 *	code segment.  Its parameters are the are the name of the symbol to 
 *	be resolved (no _Olm or _Olo needed, no quotes needed) and the name 
 *	of the pointer that will hold the resolved address.
 *
 *  OLRESOLVEEND(symbol,fptr) 
 *	A single occurrence of OLRESOLVEEND must terminate an initialization
 *	code segment.  Its parameters are the same as those for OLRESOLVE
 *
 *	Example Use:  (From ClassInitialize in StaticText.c):
 *
 *	OLRESOLVESTART
 *	OLRESOLVE(STActivateWidget, wc->primitive_class.activate)
 *	OLRESOLVE(STHandleButton, event_procs[0].handler)
 *	OLRESOLVE(STHandleButton, event_procs[1].handler)
 *	OLRESOLVE(STHandleMotion, event_procs[2].handler)
 *	OLRESOLVEEND(STHighlightSelection, _olmSTHighlightSelection)
 */

#if defined(__STDC__)
# define OlConcat(x,y) x ## y
# define OlConcatq(x,y)   OlQuote(x ## y)
# define OlQuote(x) #x
#else
# define OlConcat(x,y) x/**/y
# define OlConcatq(x,y)   OlQuote(x/**/y)
# define OlLq(x) "x
# define OlRq(x) x"
# define OlQuote(x) OlRq(OlLq(x))
#endif

#ifdef ARCHIVE
#define OLRESOLVESTART {\
		Boolean _OlGuiIsOpenLook = (OlGetGui() == OL_OPENLOOK_GUI);
#define OLRESOLVE(symbol,fptr) fptr = (_OlGuiIsOpenLook) ? \
					OlConcat(_Olo,symbol) : \
					OlConcat(_Olm,symbol);
#define OLRESOLVEEND(symbol,fptr) fptr = (_OlGuiIsOpenLook) ? \
					OlConcat(_Olo,symbol) : \
					OlConcat(_Olm,symbol);}
#else
#define OLRESOLVESTART {\
		Boolean _OlGuiIsOpenLook = (OlGetGui() == OL_OPENLOOK_GUI);\
             	_OlResolveGUISymbol(
#define OLRESOLVE(symbol,fptr) (_OlGuiIsOpenLook) ? \
				OlConcatq(_Olo,symbol) :\
				OlConcatq(_Olm,symbol), (void **)&fptr,
#define OLRESOLVEEND(symbol,fptr) (_OlGuiIsOpenLook) ? \
				OlConcatq(_Olo,symbol) :\
				OlConcatq(_Olm,symbol), (void **)&fptr,\
				(String)NULL);}
#endif

/*************************************************************************
 *
 *	typedef's, enum's, struct's
 */

/* Define a temporary  structure to get build  to work */

typedef struct {
	int unused;		/* TEMPORARY !!!!! */
} OlUnitType, *OlUnitTypeList, OlHelpInfo, *OlHelpInfoList, OlDynamicResource, *OlDynamicResourceList;

typedef Boolean	(*OlActivateFunc) OL_ARGS((
	Widget,		/* widget;	widget id */
	OlVirtualName,
	XtPointer
));
#define XtInheritActivateFunc ((OlActivateFunc) _XtInherit)

	/* Define a common structure used for all class extensions	*/
typedef struct {
    XtPointer	next_extension;	/* pointer to next in list		*/
    XrmQuark	record_type;	/* NULLQUARK				*/
    long	version;	/* version particular to extension record*/
    Cardinal	record_size;	/* sizeof() particular extension Record	*/
} OlClassExtensionRec, *OlClassExtension;

typedef void (*OlHighlightProc) OL_ARGS((
	Widget,		/* widget;		*/
	OlDefine	/* highlight_type;	*/
));
#define XtInheritHighlightHandler	((OlHighlightProc)_XtInherit)

typedef Widget	(*OlRegisterFocusFunc) OL_ARGS((Widget));
#define XtInheritRegisterFocus	((OlRegisterFocusFunc)_XtInherit)

typedef Widget (*OlTraversalFunc) OL_ARGS((
	Widget,		/* traversal_manager;	/* widget id */
	Widget,		/* w;			/* starting widget */
	OlVirtualName,	/* direction;		/* direction */
	Time		/* time;		/* request time */
));
#define XtInheritTraversalHandler	((OlTraversalFunc)_XtInherit)

					/* Virtual mapping structure	*/
typedef struct {
	OlDefine	type;		/* type flag			*/
	unsigned int	modifiers;	/* modifier mask		*/
	unsigned int	detail;		/* mapping detail, e.g., keycode*/
	String		composed;	/* Name of virtual key that
					 * was used to compose this one	*/
} _OlVirtualMapping;

    /* Define some prototypes for class-instance-extension-part procedures */

typedef void    (*OlExtDestroyProc) OL_ARGS((
    Widget,		/* shell;		/* shell's widget id	*/
    XtPointer		/* cur_part;		/* current ext part	*/
));

typedef void    (*OlExtGetValuesProc) OL_ARGS((
	Widget,		/* shell;		shell widget's id	*/
	ArgList,	/* args;		arg list		*/
	Cardinal *,	/* num_args;		# of args		*/
	XtPointer	/* ext_part;		current extension part	*/
));

typedef void	(*OlExtInitializeProc) OL_ARGS((
	Widget,		/* request;		request widget		*/
	Widget,		/* new;			new widget		*/
	ArgList,	/* args;		arg list		*/
	Cardinal *,	/* num_args;		# of args		*/
	XtPointer,	/* req_part;		request extension part	*/
	XtPointer	/* new_part;		new extension part	*/
));

typedef Boolean	(*OlExtSetValuesFunc) OL_ARGS((
	Widget,		/* current;		current widget		*/
	Widget,		/* request;		request widget		*/
	Widget,		/* new;			new widget		*/
	ArgList,	/* args;		arg list		*/
	Cardinal *,	/* num_args;		# of args		*/
	XtPointer,	/* cur_part;		current extension part	*/
	XtPointer,	/* req_part;		request extension part	*/
	XtPointer	/* new_part;		new extension part	*/
));

typedef int	ShellBehavior;	/* argument to XtNshellBehavior resource */

#define OtherBehavior		0 /* Application-defined shell behavior */
#define BaseWindow		1 /* Shell is a plain base window */
#define PopupWindow		2 /* Shell is a plain, unpinned popup */
#define PinnedWindow		3 /* Shell is pinned up */
#define PinnedMenu		4 /* Shell is pinned menu window */
#define PressDragReleaseMenu	5 /* Menu shell in press-drag-release mode */
#define StayUpMenu		6 /* Menu shell in stay-up mode */
#define UnpinnedMenu		7 /* Shell is unpinned menu window */


/* dynamic resource structure */
typedef struct __OlDynResource {
	XtResource	res;		/* resource structure */
	int		offset;		/* byte offset */
	int		bit_offset;	/* bit offset into a byte */
	char *		(*proc) OL_ARGS((Widget, Boolean,
				struct __OlDynResource *));/* base proc */
} _OlDynResource, *_OlDynResourceList;

typedef char *	(*_OlBaseProc) OL_ARGS((Widget, Boolean, _OlDynResourceList));

typedef struct {
	_OlDynResourceList	resources;
	int			num_resources;
} _OlDynData;

typedef void	(*OlTransparentProc) OL_ARGS((Widget, Pixel, Pixmap));
typedef void	(*OlWMProtocolProc) OL_ARGS((Widget,
					     OlDefine,	/* action */
					     OlWMProtocolVerify *));
#define XtInheritTransparentProc ((OlTransparentProc) _XtInherit)
#define XtInheritWMProtocolProc ((OlWMProtocolProc) _XtInherit)

/*************************************************************************
 *
 *	Private external declarations
 */

extern XrmName			_OlApplicationName;
extern OLconst XtActionsRec	_OlGenericActionTable[];
extern OLconst Cardinal		_OlGenericActionTableSize;
extern OLconst OlEventHandlerRec	_OlGenericEventHandlerList[];
extern OLconst Cardinal		_OlGenericEventHandlerListSize;
extern OLconst char		_OlGenericTranslationTable[];
extern Display *		toplevelDisplay;
extern Boolean			_OlDynResProcessing;
extern Widget *			_OlShell_list;
extern Cardinal			_OlShell_list_size;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

OlDefine
_OlAddAccelerator OL_ARGS((
	Widget		 	w,
	XtPointer		data,
	String			accelerator
));

extern void
_OlAddGrab OL_ARGS((
	Widget,		/* widget wanting grab	*/
	Boolean,	/* exclusives mode ?	*/
	Boolean		/* spring loaded ?	*/
));

OlDefine
_OlAddMnemonic OL_ARGS((
	Widget			w,
	XtPointer		data,
	char			mnemonic
));

extern void
_OlAddOlDefineType OL_ARGS((
	OLconst char * ,/* name;	OlDefine's name			*/
	OlDefine	/* value;	OlDefine's value		*/
));

extern void
_OlAddVirtualMappings OL_ARGS((			
	String		/* mappings;	virtual token mappings	*/
));

extern void
_OlAppAddVirtualMappings OL_ARGS((			
	Widget,		/* widget;	widget id or NULL	*/
	String		/* mappings;	virtual token mappings	*/
));

extern void
_OlAssociateWidget OL_ARGS((
	Widget,		/* lead;		lead widget		*/
	Widget,		/* follower;		follower widget		*/
	Boolean		/* disable_traversal;	don't trav. to follower	*/
));

	/* Beep the display only if Settings in Workspace permit it	*/
extern void
_OlBeepDisplay OL_ARGS((
	Widget,		/* widget;	- widget wanting beep	*/
	Cardinal	/* count;	- number of beeps	*/
));

extern void
_OlCallHighlightHandler OL_ARGS((Widget, OlDefine));

extern KeySym
_OlCanonicalKeysym OL_ARGS((
      Display *,      /* display;     */
      KeySym,         /* keysym;      */
      KeyCode *,      /* p_keycode;   */
      Modifiers *     /* p_modifiers; */
));

extern void
_OlCheckDynResources OL_ARGS((
	Widget, 	/* widget;					*/
	_OlDynData *,	/* data;					*/
	ArgList,	/* args;					*/
	Cardinal	/* num_args					*/
));

extern WidgetClass	/* returns one of Open Look fundamental class types */
_OlClass OL_ARGS((Widget));

			/* Clear the background of a widget or a gadget	*/
extern void
_OlClearWidget OL_ARGS((
	Widget,		/* widget;	- widget/gadget id to clear	*/
	Boolean		/* exposures;	- generate exposure event ?	*/
));

extern void
_OlConvertToXtArgVal OL_ARGS((
	char *,		/* src;		arbitrary source location	*/
	XtArgVal *,	/* dest;	address of XtArgVal to hold data*/
	Cardinal	/* size;	size of data			*/
));

extern void
_OlCopyFromXtArgVal OL_ARGS((
	XtArgVal,	/* src;		XtArgVal holding the data	*/
	char *,		/* dest;	data's destination location	*/
	Cardinal	/* size;	size of data			*/
));

extern void
_OlCopyToXtArgVal OL_ARGS((
	char *,		/* src;		arbitrary source location	*/
	XtArgVal *,	/* dest;	address of XtArgVal holding
					the address of data's destination*/
	Cardinal	/* size;	size of data			*/
));


		/* Composes an Arg list from a source Arg list and a
		 * source MaskArg list.					*/
extern void
_OlComposeArgList OL_ARGS((
	ArgList,	/* args;		- source Arg List	*/
	Cardinal,	/* num_args;		- number of source Args	*/
	MaskArgList,	/* mask_args;		- mask Arg List		*/
	Cardinal,	/* mask_num_args;	- number of mask Args	*/
	ArgList *,	/* dest_args;		- destination Arg List	*/
	Cardinal *	/* dest_num_args;	- number of dest Args	*/
));

			/* Copy an XtArgVal value into a destination of
			 * an arbitrary size.				*/
extern void
_OlCopyFromXtArgVal OL_ARGS((
	XtArgVal,		/* source;			*/
	char *,			/* destination;			*/
	Cardinal		/* size;			*/
));

			/* Copy information located at some address into
			 * an XtArgVal.					*/
extern void
_OlCopyToXtArgVal OL_ARGS((
	char *,		/* source;			*/
	XtArgVal *,	/* destination;			*/
	Cardinal	/* size;			*/
));

				/* Converts string to Gravity resource	*/
extern void
_OlCvtStringToGravity OL_ARGS((
	XrmValue *,	/* args;	arguments needed for conversion	*/
	Cardinal *,	/* num_args;	the number of converion args	*/
	XrmValue *,	/* fromVal;	value to convert		*/
	XrmValue *	/* toVal;	returned converted value	*/
));

				/* Converts string to OlDefine resource	*/
extern void
_OlCvtStringToOlDefine OL_ARGS((
	XrmValue *,	/* args;	arguments needed for conversion	*/
	Cardinal *,	/* num_args;	the number of converion args	*/
	XrmValue *,	/* fromVal;	value to convert		*/
	XrmValue *	/* toVal;	returned converted value	*/
));

extern void
_OlDeleteDescendant OL_ARGS((Widget));

extern void
_OlDefaultTransparentProc OL_ARGS((Widget, Pixel, Pixmap));

void
_OlDestroyKeyboardHooks OL_ARGS((
	Widget			w
));

			/* Dump the virtual buttons/keys to a file	*/
extern void
_OlDumpVirtualMappings();
	/* FILE *	fp;		- destination file pointer	*/
	/* Boolean	long_form;	- True for long listing		*/

extern void
_OlDynResProc OL_NO_ARGS();

extern Widget
_OlFetchAssociatedWidgetLeader OL_ARGS((
	Widget		/* follower;		follower widget	*/
));

extern void
_OlFetchAssociatedWidgetList OL_ARGS((
	Widget,		/* lead;		lead widget		*/
	WidgetList *,	/* ret_list;		followers		*/
	Cardinal *	/* num_followers;	number of followers	*/
));

extern Widget
_OlFetchMnemonicOwner OL_ARGS((
	Widget			w,
	XtPointer *		p_data,
	OlVirtualEvent		virtual_event
));

extern Widget
_OlFetchAcceleratorOwner OL_ARGS((
	Widget			w,
	XtPointer *		p_data,
	OlVirtualEvent		virtual_event
));
extern Widget
_OlFindVendorShell OL_ARGS((Widget, Boolean));

				/* Free font created by _OlGetFont() 	*/
extern void
_OlFreeFont OL_ARGS((
	Display *,	/* display;	- font's display		*/
	XFontStruct *	/* font_struct;	- XFontStruct returned by _OlGetFont*/
));

			/* Free XImages created with _OlGetImage()	*/
extern void
_OlFreeImage OL_ARGS((
	XImage *	/* ximage;	- XImage returned by _OlGetImage*/
));

extern void
_OlFreeRefName OL_ARGS((
	Widget		/* w;						*/
));


		/* to retrieve appl title */
extern String
_OlGetApplicationTitle OL_ARGS((
	Widget		/* widget;	any widget in application	*/
));

extern OlClassExtension
_OlGetClassExtension OL_ARGS((
	OlClassExtension,	/* extension;	start with this one	*/
	XrmQuark,		/* record_type;	type to look for	*/
	long			/* version;	if non-zero, look for it*/
));

extern Widget
_OlGetDefault OL_ARGS((
	Widget			/* widget; 	any widget or shell	*/
));

extern XFontStruct *
_OlGetDefaultFont OL_ARGS((
	Widget,			/* w					*/
	String			/* font (e.g. OlDefaultFont) or NULL    */
));

extern Modifiers
_OlGetDontCareModifiers OL_NO_ARGS();

	/* get dirty bit for a dynamic resource */
extern int
_OlGetDynBit OL_ARGS((
	Widget,
	_OlDynResourceList
));

	/* Font Loading & caching */
extern XFontStruct *
_OlGetFont OL_ARGS((
	Screen *,	/* screen;	- intended viewing screen id	*/
	int,		/* point_size;	- font point_size 		*/
	char *		/* name;	- raw font name or NULL		*/
));

	/* Image creation & caching */
extern XImage *
_OlGetImage OL_ARGS((
	Screen *,	/* screen;	- intended viewing screen id	*/
	int,		/* point_size;	- point_size of information	*/
	char *		/* name;	- raw bitmap name		*/
));


extern Widget
_OlGetMenubarWidget OL_ARGS((
        Widget		/* widget;	widget id or NULL		*/
));

					/* single click damping factor	*/
extern Cardinal
_OlGetMouseDampingFactor OL_ARGS((
	Widget		/* widget;	widget id or NULL		*/
));

extern Cardinal
_OlGetMultiObjectCount OL_ARGS((
	Widget		/* widget;	widget id or NULL		*/
));

extern void
_OlGetRefNameOrWidget OL_ARGS((
	Widget,		/* w;			*/
	ArgList,	/* args;		*/
	Cardinal *	/* num_args;		*/
));

	/* Search up a widget tree until the shell is found		*/
extern Widget
_OlGetShellOfWidget OL_ARGS((
	Widget		/* widget;	start search here	*/
));

				/* Get transparent_proc given a widget */
extern OlTransparentProc
_OlGetTransProc OL_ARGS((Widget));

				/* Get the id of the Troll Widget	*/
extern Widget
_OlGetTrollWidget OL_NO_ARGS();

		/* Get the mappings for a virtual button or virtual key	*/
extern Cardinal
_OlGetVirtualMappings OL_ARGS((
	String,			/* name;	- virtual button name	*/
	_OlVirtualMapping *,	/* mapping_list;- application-supplied
						  array to fill in	*/
	Cardinal		/* list_length;	- number of array elements*/
));

	/* Checks to see if a widget can grab the pointer */ 
extern Boolean
_OlGrabPointer OL_ARGS((
	Widget,		/* widget; 	- widget requesting grab	*/
	Bool,		/* event_mask;	- */
	unsigned int,	/* event_mask;	- */
	int,		/* pointer_mode; - */
	int,		/* keyboard_mode; - */
	Window,		/* confine_to; - */
	Cursor,		/* cursor; - */
	Time		/* time; - */
));

						/* Grab the server	*/
extern Boolean
_OlGrabServer OL_ARGS((
	Widget		/* widget; 	- widget requesting grab	*/
));

extern void
_OlInitDynamicHandler OL_ARGS((
	Widget		/* w;						*/
));

extern void
_OlInitDynResources OL_ARGS((
	Widget,		/* widget;					*/
	_OlDynData *	/* data;					*/
));

			/* Initialize the virtual button/key code	*/
extern void
_OlInitVirtualMappings OL_ARGS((
	Widget		/* widget;		application widget	*/
));

extern void
_OlInsertDescendant OL_ARGS((Widget));

	/* Sees if a modifier mask and button number is a virtual
	 * button mapping						*/
extern Boolean
_OlIsVirtualButton OL_ARGS((
	String,		/* name;    	- virtual button name		*/
	unsigned int,	/* state;      	- modifier mask			*/
	unsigned int,	/* button;     	- button number or zero		*/
	Boolean		/* exclusive;  	- True for perfect match	*/
));

	/* Sees if an XEvent is either a virtual button or virtual Key
	 * event.							*/
extern Boolean
_OlIsVirtualEvent OL_ARGS((
	char *,		/* name;    	- Virtual Button/key name	*/
	XEvent *,	/* xevent;	- XEvent to be checked		*/
	Boolean		/* exclusive;  	- True for perfect match	*/
));

	/* Sees if a modifier mask and a keycode is a virtual key
	 * mapping						*/
extern Boolean
_OlIsVirtualKey OL_ARGS((
	String,		/* name;    	- virtual key name		*/
	unsigned int,	/* state;      	- modifier mask			*/
	KeyCode,	/* keycode;     - button from XEvent or NULL	*/
	Boolean		/* exclusive;  	- True for perfect match	*/
));

extern int
_OlKeysymToSingleChar OL_ARGS((KeySym));

extern void
_OlLoadQuarkTable OL_NO_ARGS();

extern void
_OlLoadVendorShell OL_NO_ARGS();

extern String
_OlMakeAcceleratorText OL_ARGS((
	Widget			w,
	String			str
));

extern void
_OlMergeDynResources OL_ARGS((
	_OlDynData *,	/* new 						*/
	_OlDynData *	/* old 						*/
));

	/* Expose the event handler that pops up the help widget	*/
extern void
_OlPopupHelpTree OL_ARGS((
	Widget,		/* w;		- shell widget id		*/
	XtPointer,	/* client_data;	- unused			*/
	XEvent *,	/* xevent;	- help XEvent message		*/
	Boolean *	/* cont_to_dispatch				*/
));

extern void
_OlPopdownHelpTree OL_ARGS((
	Widget		/* w;		- shell widget id		*/
));

extern void
_OlPopdownTrailingCascade OL_ARGS((
	Widget,		/* w;		- shell widget id	*/
	Boolean		/* skip_first;	- first cascading menu	*/
));

extern void
_OlProcessHelpKey OL_ARGS((
	Widget,		/* widget receiving keypress event	*/
	XEvent *	/* KeyPress XEvent pointer		*/
));

extern void
_OlRegisterFocusWidget OL_ARGS((
	Widget,			/* widget				*/
	Boolean			/* Override current focus widget?	*/
));

/* register an Ic with its shell */
extern void
_OlRegisterIc OL_ARGS((
    Widget,		/* w; - the widget that owns the ic 	*/
    OlIc *,		/* ic; 					*/
    unsigned short, 	/* width; - desired status area width, 0 if unknown */
    unsigned short 	/* height; - desired status area height */
));


		/* Add an OlDefine type to it's converter's database	*/
/* Obsolete, use _OlAddOlDefine instead */
extern void
_OlRegisterOlDefineType OL_ARGS((
	XtAppContext,	/* app_context;	Application Context or NULL	*/
	String,		/* name;	OlDefine's name			*/
	OlDefine	/* value;	OlDefine's value		*/
));

extern void
_OlRegisterShell OL_ARGS((Widget));

extern void
_OlRemoveAccelerator OL_ARGS((
	Widget			w,
	XtPointer		data,
	Boolean			ignore_data,
	String			accelerator
));

extern void
_OlRemoveGrab OL_ARGS((
	Widget		/* Widget with prior grab	*/
));

extern void
_OlRemoveMnemonic OL_ARGS((
	Widget			w,
	XtPointer		data,
	Boolean			ignore_data,
	char			mnemonic
));

extern Boolean
_OlSelectDoesPreview OL_ARGS((
	Widget		/* widget;	widget id or NULL	*/
));

					/* to store appl title	*/
extern void
_OlSetApplicationTitle OL_ARGS((
	String		/* title;	/* set title to this	*/
));

extern void
_OlSetCurrentFocusWidget OL_ARGS((
	Widget,			/* widget; 	widget */
	OlDefine		/* state;	OL_IN or OL_OUT	*/
));

extern void
_OlSetDefault OL_ARGS((
	Widget,			/* widget; 	widget in question	*/
	Boolean			/* is_default;	is default or not ?	*/
));

	/* set dirty bit for a dynamic resource */
extern void
_OlSetDynBit OL_ARGS((
	Widget,
	_OlDynResourceList
));

extern void
_OlSetMenubarWidget OL_ARGS((
        Widget,		/* w;		widget id or NULL		*/
        Boolean		/* want_to_set;	set or not			*/
));


	/* append Atom to WM_PROTOCOLS property of toplevel window */
extern void
_OlSetWMProtocol OL_ARGS((
	Display *,	/* display;	display			*/
	Window,		/* window;	window (toplevel)	*/
	Atom		/* atom;	atom to append		*/
));

extern KeySym
_OlSingleCharToKeysym OL_ARGS((int));

extern void
_OlSortTraversalList OL_ARGS((Widget));

extern Boolean
_OlStringToOlBtnDef OL_ARGS((
      Display *,      /* display;             */
      XrmValue *,     /* args;                */
      Cardinal *,     /* num_args;            */
      XrmValue *,     /* from;                */
      XrmValue *,     /* to;                  */
      XtPointer *     /* converter_data;      */
));

extern Boolean
_OlStringToOlKeyDef OL_ARGS((
      Display *,      /* display              */
      XrmValue *,     /* args;                */
      Cardinal *,     /* num_args;            */
      XrmValue *,     /* from;                */
      XrmValue *,     /* to                   */
      XtPointer *     /* converter_data;      */
));

extern OlVirtualName
_OlStringToVirtualKeyName OL_ARGS((
      String          /* str;                 */
));

					/* Used to ungrab the pointer	*/
extern void
_OlUnassociateWidget OL_ARGS((
	Widget,		/* lead;		widget or NULL		*/
	Widget,		/* follower;		widget or NULL		*/
	Boolean		/* do_descendants;	do followers of the followers*/
));

/* unregister an Ic previously registered with its shell */
extern void
_OlUnregisterIc OL_ARGS((
    Widget,		/* w; - the widget that owns the ic 	*/
    OlIc *		/* ic; 					*/
));

extern void
_OlUnregisterShell OL_ARGS((Widget));

extern void
_OlUngrabPointer OL_ARGS((
	Widget		/* widget;	- widget wishing pointer ungrab	*/
));

					/* Used to ungrab the server	*/
extern void
_OlUngrabServer OL_ARGS((
	Widget		/* widget;	- widget wishing server ungrab	*/
));

	/* unset dirty bit for a dynamic resource */
extern void
_OlUnsetDynBit OL_ARGS((
	Widget,
	_OlDynResourceList
));

extern void
_OlUpdateTraversalWidget OL_ARGS((
	Widget,		/* w;						*/
	String,		/* ref_name;					*/
	Widget,		/* ref_widget;					*/
	Boolean		/* insert_update; True: insert, False: update	*/
));

extern Widget
_OlWidgetToGadget OL_ARGS((
	Widget,			/* w;	Composite		*/
	Position,		/* x;	Position over Composite	*/
	Position		/* y;	Position over Composite	*/
));

extern OlImFunctions *
_OlSetupInputMethod OL_ARGS((
	Display *,		/* dpy;				*/
	String,			/* im_name; Input Method Name	*/
	String,			/* lang_locale; Locale Name	*/
	String,			/* res_name;  			*/
	String			/* res_class;			*/
));


extern void
_OlResolveGUISymbol OL_ARGS((
	char *,
	void **,
	...
));

OLEndFunctionPrototypeBlock

#endif /* __OpenLookP_h__ */
