#ifndef	NOIDENT
#ident	"@(#)mouseless:Vendor.c	1.110"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the routines necessary to manipulate the
 *	Vendor Shell class.  Rather than changing the class and instance
 *	parts of the Vendor shell and re-compiling the Intrinsics,
 *	we'll use the class extension mechanism.  At toolkit startup,
 *	the necessary class fields are loaded into the vendor shell
 *	class record.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <memory.h>

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookI.h>
#include <Xol/Accelerate.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/OlClients.h>
#include <Xol/PrimitiveP.h>
#include <Xol/LayoutExtP.h>
#include <Xol/HandlesExP.h>
#ifdef	OpenWindowsEnvironment	/*OWV3*/
#include <DrawAreaP.h>
#endif	/*OpenWindowsEnvironment*/	/*OWV3*/
#include <Xol/VendorI.h>

#ifdef GUI_DEP
#include <OlDnDVCXP.h>		/* for new drag and drop support */
#else
#include <DnD/OlDnDVCX.h>	/* for GUI indendent DnD	 */
#endif

#define ClassName VendorShell
#include <Xol/NameDefs.h>

#include <Xol/TextEditP.h>	/* for XIM StatusArea bugs       */

#if !defined(PATH_MAX)

#if defined(sun)
#include <sys/param.h>
#define PATH_MAX	MAXPATHLEN
#endif

#endif

/*
 * Define the following if you want to be able to turn on debugging
 * information in the binary product.
 */
#define	GEOMETRY_DEBUG
#if	defined(GEOMETRY_DEBUG)
static Boolean		geometry_debug = False;

static void
Debug__DumpGeometry OLARGLIST((g))
	OLGRA(XtWidgetGeometry *, g)
{
# define P(SFIELD,FIELD,FLAG) \
	if (g->request_mode & FLAG) printf (" %s/%d", SFIELD, g->FIELD)
	P("x", x, CWX);
	P("y", y, CWY);
	P("width", width, CWWidth);
	P("height", height, CWHeight);
	P("border_width", border_width, CWBorderWidth);
# undef	P
	return;
}
#endif

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static Boolean	CallExtPart OL_ARGS((int, WidgetClass, Widget, Widget,
				     Widget, ArgList, Cardinal *,
				     XtPointer, XtPointer, XtPointer));
static OlVendorPartExtension
		GetInstanceData OL_ARGS((Widget, int));
static void	ResortTraversalList OL_ARGS((WidgetArray *, Boolean *));
static void	SetupInputFocusProtocol OL_ARGS((Widget, OlFocusData *));
static void	SetWMAttributes OL_ARGS((Widget, OlVendorPartExtension,
					 Boolean));
static Boolean	UseOLWinAttr OL_ARGS((Widget));
static char *	VendorGetBase OL_ARGS((Widget, Boolean, _OlDynResourceList));
static XtCallbackList
		CreateVendorCBList OL_ARGS((XtCallbackList));
static void	UpdateIcGeometries OL_ARGS((Widget, OlVendorPartExtension, XtWidgetGeometry *));
static int	CompareOlIcGeometries OL_ARGS((char *, char *));


					/* class procedures		*/

static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static Boolean	ActivateWidget OL_ARGS((Widget,OlVirtualName,XtPointer));
static void	ClassInitialize OL_NO_ARGS();
static void	ClassPartInitialize OL_ARGS((WidgetClass));
static void	Destroy OL_ARGS((Widget));
static void	ExtDestroy OL_ARGS((Widget, XtPointer));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void	Realize OL_ARGS((Widget, XtValueMask *,XSetWindowAttributes *));
static void	GetValues OL_ARGS((Widget, ArgList, Cardinal *));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				   ArgList, Cardinal *));
static Widget	TraversalHandler OL_ARGS((Widget, Widget, OlDefine, Time));
static void	WMMsgHandler OL_ARGS((Widget, OlDefine, OlWMProtocolVerify *));
static void		ChangeManaged OL_ARGS((
	Widget			w
));
static void     Layout OL_ARGS((Widget			w,
				Boolean			resizable,
				Boolean			query_only,
				Boolean			cached_best_fit_ok_hint,
				Widget			who_asking,
				XtWidgetGeometry *	request,
				XtWidgetGeometry *	response
));
					/* action procedures		*/

static void	TakeFocusEventHandler OL_ARGS((Widget, XtPointer,
						XEvent *, Boolean *));
static void	WMMessageHandler OL_ARGS((Widget, XtPointer,
					  XEvent *, Boolean *));

					/* public procedures		*/

Widget			_OlGetDefault OL_ARGS((Widget));
OlFocusData *		_OlGetFocusData
				OL_ARGS((Widget, OlVendorPartExtension *));
OlVendorClassExtension	_OlGetVendorClassExtension OL_ARGS((WidgetClass));
OlVendorPartExtension	_OlGetVendorPartExtension OL_ARGS((Widget));
extern int		_OlGetMWMHints OL_ARGS((Display *, Window,
							PropMwmHints *));
void			_OlLoadVendorShell OL_NO_ARGS();
void			_OlSetDefault OL_ARGS((Widget, Boolean));
void			_OlSetPinState OL_ARGS((Widget, OlDefine));
void			_OlRegisterShell OL_ARGS((Widget));
void			_OlUnregisterShell OL_ARGS((Widget));
void			OlWMProtocolAction OL_ARGS((Widget,
					OlWMProtocolVerify *, OlDefine));

#ifdef	sunOWV3	/*OWV3*/
void			_OlFixResourceList OL_ARGS((WidgetClass));
void			_OlCopyParentsVisual OL_ARGS((widget, closure, value));
void			_OlSetupColormap OL_ARGS((widget, closure, value));
#endif	/*sun*/	/*OWV3*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

XrmQuark			OlXrmVendorClassExtension = NULLQUARK;
#define VENDOR_EXTENSION	(OLconst char *)"OlVendorClassExtension"

#define	CREATE			1
#define DESTROY			2

#define INIT_PROC		1
#define DESTROY_PROC		2
#define GET_PROC		3
#define SET_FUNC		4

#define NULL_EXTENSION		((OlVendorClassExtension)NULL)
#define NULL_PART_PTR		((OlVendorPartExtension*)NULL)
#define NULL_PART		((OlVendorPartExtension)NULL)
#define NULL_FOCUS_DATA		((OlFocusData *)NULL)
#define NULL_WIDGET		((Widget)NULL)

#define SHELL_LIST_STEP		16

#define VCLASS(wc, r)	(((VendorShellWidgetClass)(wc))->vendor_shell_class.r)

#define GET_EXT(wc)	(OlVendorClassExtension) _OlGetClassExtension( \
			    (OlClassExtension)VCLASS(wc, extension), \
			    OlXrmVendorClassExtension, \
			    OlVendorClassExtensionVersion)

#define CBLSIZE(cb)	((int)(cb->closure))

#if     !defined(New)
# define New(M) XtNew(M)
#endif

#if     !defined(Free)
# define Free(M) XtFree(M)
#endif

#if     !defined(Malloc)
# define Malloc(N) XtMalloc(N)
#endif

#if     !defined(Realloc)
# define Realloc(P,N) XtRealloc(P,N)
#endif

#if     !defined(Array)
# define Array(P,T,N) \
        ((N)? \
                  ((P)? \
                          (T *)Realloc((char *)(P), sizeof(T) * (N)) \
                        : (T *)Malloc(sizeof(T) * (N)) \
                  ) \
                : (T *)((P)? (Free((char *)P),(T *)0) : (T *)0) \
        )
#endif

/*
 * The structure below is used by Open Windows environments.
 *	See notes in UseOLWinAttr().
 */
typedef struct {
	Atom		win_type;
	long		menu_type;
	long		pin_state;
} OwOLWinAttr;

/*
 * This is used in SetWMAttributes, because it requires this info to know
 * whether to add or delete from the default decoration of certain window
 * type. It needs to add or delete certain decorations because Vendor has
 * allows more flexible decorations than what is allowed by the standard
 * window types.
 */
static struct WinTypeDecor {
	OlDefine	win_type;
	String		win_atom_name;
	Boolean 	window_header;
	Boolean 	menu_button;
	Boolean 	pushpin;
	Boolean 	resize_corners;
	OlDefine 	menu_type;
} wintype_decor[] = {
/* WINTYPE	NAME		HEADER	CLOSE	PIN	RESIZE MENUTYPE */
{ OL_WT_BASE,	"_OL_WT_BASE",	True,	True,	False,	True,  OL_MENU_FULL },
{ OL_WT_CMD,	"_OL_WT_CMD",	True,	False,	True,	True,  OL_MENU_LIMITED},
#ifndef	sunOWV3
{ OL_WT_NOTICE,	"_OL_WT_NOTICE",False,	False,	False,	False, OL_NONE },
#else	/*sun*/
{ OL_WT_NOTICE,	"_OL_WT_NOTICE",True,	False,	False,	False, OL_NONE },
#endif	/*sun*/
{ OL_WT_HELP,	"_OL_WT_HELP",	True,	False,	True,	True, OL_MENU_LIMITED},
{ OL_WT_OTHER,	"_OL_WT_OTHER",	False,	False,	False,	False, OL_NONE },
{ 0,		NULL,		False,	False,	False,	False, OL_NONE },
};

/* Don't change the above structure values without checking the defaults with
 * olwm - they must match for all defaults, such as resize corners, pin,
 * header, close, and menutype for a specific window type.
 */

#define BYTE_OFFSET	XtOffsetOf(OlVendorPartExtensionRec, dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET,
	 OL_B_VENDOR_BG, VendorGetBase },
{ { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Black" }, BYTE_OFFSET,
	 OL_B_VENDOR_BORDERCOLOR, VendorGetBase },
};
#undef BYTE_OFFSET

extern void _OlNewAcceleratorResourceValues OL_ARGS((XtPointer));

/*
 * These are used to maintain a list of base window shells, so that
 * _OlDynResProc() can traverse to all the shells. The non-base window
 * shells are linked from one of the base window shells. These links are
 * maintained in the vendor shell extension.
 */
Widget *_OlShell_list = NULL;
Cardinal _OlShell_list_size = 0;
Cardinal _OlShell_list_alloc_size = 0;

/* The default status area geometry */
static XtWidgetGeometry defaultStatusAreaGeometry = {0};


/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */
static XtActionsRec
ActionTable[] = {
	{ "OlAction",	OlAction }
};


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */
#define OFFSET(field)	XtOffsetOf(VendorShellRec, field)

static XtResource
resources[] = {
    { XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
	OFFSET(shell.allow_shell_resize), XtRImmediate,
	(XtPointer) ((Boolean)True) },

    { XtNancestorSensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	OFFSET(core.ancestor_sensitive), XtRImmediate,
	(XtPointer) ((Boolean)True) },

    { XtNinput, XtCInput, XtRLongBoolean, sizeof(Bool),
	OFFSET(wm.wm_hints.input), XtRImmediate, (XtPointer) ((Bool)FALSE) },
#ifdef	sunOWV3	/*OWV3*/
    { XtNvisual, XtCVisual, XtRVisual, sizeof(Visual*),
	OFFSET(shell.visual), XtRCallProc, (XtPointer) _OlCopyParentsVisual},
    { XtNcolormap, XtCColormap, XtRColormap, sizeof(Colormap),
	OFFSET(core.colormap), XtRCallProc, (XtPointer) _OlSetupColormap},
#endif	/*sun*/	/*OWV3*/

};

#undef OFFSET
			/* This resource list is to initialize the
			 * extension data.				*/

#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
					/* Focus management resources	*/

		/* must in position 0, see ClassInitialize */
    { XtNfocusWidget, XtCFocusWidget, XtRWidget, sizeof(Widget),
	OFFSET(focus_data.initial_focus_widget), XtRWidget, (XtPointer) NULL },

    { XtNfocusModel, XtCFocusModel, XtROlDefine, sizeof(OlDefine),
	OFFSET(focus_data.focus_model), XtRImmediate,
	 (XtPointer) OL_CLICK_TO_TYPE },

				/* Generic Vendor Shell resources	*/

    { XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	OFFSET(busy), XtRImmediate, (XtPointer)False },

    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRImmediate, (XtPointer)True },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRImmediate, (XtPointer)True },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRImmediate, (XtPointer)True },

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_FULL },

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_BASE },

    { XtNacceleratorsDoGrab, XtCAcceleratorsDoGrab, XtRBoolean, sizeof(Boolean),
	OFFSET(accelerators_do_grab), XtRImmediate, (XtPointer)False },

#ifdef NOT_USE
/*
   Will need this in Xt R5. But for now, vendor is maintaining its own list
   of callbacks.
*/
    { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	OFFSET(consume_event), XtRCallback, (XtPointer)NULL },
    { XtNwmProtocol, XtCWMProtocol, XtRCallback, sizeof(XtCallbackList),
	OFFSET(wm_protocol), XtRCallback, (XtPointer)NULL },
#endif
    { XtNstatusAreaGeometry, XtCStatusAreaGeometry, 
	  XtRWidgetGeometry, sizeof(XtWidgetGeometry), OFFSET(status_area_geometry),
	  XtRWidgetGeometry, (XtPointer)&defaultStatusAreaGeometry},

    { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	OFFSET(user_data), XtRPointer, (XtPointer) NULL },

    { XtNwmProtocolInterested, XtCWMProtocolInterested,
	XtROlBitMask, sizeof(OlBitMask),
	OFFSET(wm_protocol_mask), XtRImmediate,
	(XtPointer) (OL_WM_DELETE_WINDOW | OL_WM_TAKE_FOCUS) },
};

/*
 *
 * Special resource list for transient shell.
 *
 */
static XtResource
transient_resources[] = {
					/* Focus management resources	*/

		/* must in position 0, see ClassInitialize */
    { XtNfocusWidget, XtCFocusWidget, XtRWidget, sizeof(Widget),
	OFFSET(focus_data.initial_focus_widget), XtRWidget, (XtPointer) NULL },

				/* Generic Vendor Shell resources	*/

    { XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	OFFSET(busy), XtRImmediate, (XtPointer)False },

    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRImmediate, (XtPointer)False },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRImmediate, (XtPointer)True },

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_BASE },

    { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	OFFSET(user_data), XtRPointer, (XtPointer) NULL }
};

#undef OFFSET

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

static OlVendorClassExtensionRec
vendor_extension_rec = {
	{
#ifdef GUI_DEP
		(XtPointer)&dnd_vendor_extension_rec,/* next_extension	*/
#else
		(XtPointer)NULL,		/* next_extension	*/
#endif
		NULLQUARK,			/* record_type		*/
		OlVendorClassExtensionVersion,	/* version		*/
		sizeof(OlVendorClassExtensionRec)/* record_size		*/
	},	/* End of OlClassExtension header */
	ext_resources,				/* resources		*/
	XtNumber(ext_resources),		/* num_resources	*/
	NULL,					/* private		*/
	NULL,					/* set_default		*/
	NULL,					/* get_default		*/
	ExtDestroy,				/* destroy		*/
	NULL,					/* initialize		*/
	NULL,					/* set_values		*/
	NULL,					/* get_values		*/
	TraversalHandler,			/* traversal_handler	*/
	NULL,					/* highlight_handler	*/
	ActivateWidget,				/* activate		*/
	NULL,	/* See ClassInitialize */	/* event_procs		*/
	0,	/* See ClassInitialize */	/* num_event_procs	*/
	NULL,					/* part_list		*/
	{ dyn_res, XtNumber(dyn_res) },		/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	FALSE,					/* override_callback	*/
}, *vendor_extension = &vendor_extension_rec;

static
LayoutCoreClassExtensionRec	layoutCoreClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */ (XrmQuark) 0, /* see ClassInitialize */
/* version              */            OlLayoutCoreClassExtensionVersion,
/* record_size          */            sizeof(LayoutCoreClassExtensionRec),
/* layout            (I)*/            Layout,
/* query_alignment   (I)*/ (XtGeometryHandler)0,
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

VendorShellClassRec
vendorShellClassRec = {
  {						/* core class		*/
	(WidgetClass) &wmShellClassRec,		/* superclass		*/
	"VendorShell",				/* class_name		*/
	sizeof(VendorShellRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	ClassPartInitialize,			/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	Realize,				/* realize		*/
	ActionTable, 				/* actions		*/
	XtNumber(ActionTable), 			/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	FALSE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	FALSE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	_OlDefaultResize,			/* resize		*/
	XtInheritExpose,			/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	GetValues,				/* get_values_hook	*/
	AcceptFocus,				/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL, /* See ClassInitialize */		/* tm_table		*/
	_OlDefaultQueryGeometry,		/* query_geometry	*/
  	NULL,					/* display_accelerator  */
    	(XtPointer)&layoutCoreClassExtension  	/* extension		*/
  },
  {						/* composite class	*/
	_OlDefaultGeometryManager,		/* geometry_manager	*/
	ChangeManaged,				/* change_managed	*/
	_OlDefaultInsertChild,			/* insert_child		*/
	_OlDefaultDeleteChild,			/* delete_child		*/
	NULL					/* extension         	*/
  },
  {						/* shell class		*/
	NULL					/* extension		*/
  },
  {						/* WMShell class	*/
	NULL					/* extension		*/
  },
  {						/* VendorShell class	*/
	(XtPointer)&vendor_extension_rec	/* extension		*/
  }
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass vendorShellWidgetClass = (WidgetClass) &vendorShellClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/****************************procedure*header*****************************
 * CallExtPart - this procedure forwardly-chains calls to the instance
 * extension part class procedures of the vendor shell.
 */
static Boolean
CallExtPart OLARGLIST((proc_type, wc, current, request, new, args, num_args, cur_part, req_part, new_part))
	OLARG( int,		proc_type)
	OLARG( WidgetClass,	wc)
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLARG( Cardinal *,	num_args)
	OLARG( XtPointer,	cur_part)
	OLARG( XtPointer,	req_part)
	OLGRA( XtPointer,	new_part)
{
	Boolean			ret_val = False;
	OlVendorClassExtension	ext = GET_EXT(wc);

#if 0
		/* the VCLASS macro should return the vendor extension	*/
		/* pointer from a given widget class. But this macro	*/
		/* always returns "vendor_extension_rec", as a result,	*/
		/* an extension method will be called more than once.	*/

		/* I guess either we fix VCLASS or comment out the code	*/
		/* below because other subclasses (e.g., toplevelShell)	*/
		/* won't be interested in our extension record...	*/
	if (wc != vendorShellWidgetClass)
	{
	    if (CallExtPart(proc_type, wc->core_class.superclass,
			current, request, new,
			args, num_args,
			cur_part, req_part, new_part) == TRUE)
	    {
		ret_val = TRUE;
	    }
	}
#endif

	if (ext != NULL_EXTENSION) {
	    switch (proc_type)
	    {
	    case INIT_PROC :
		if (ext->initialize != (OlExtInitializeProc)NULL)
		{
		    (*ext->initialize)(request, new,
				args, num_args, req_part, new_part);
		}
		break;
	
	    case DESTROY_PROC :
		if (ext->destroy != (OlExtDestroyProc)NULL)
		{
		    (*ext->destroy)(current, cur_part);
		}
		break;

	    case SET_FUNC :
		if (ext->set_values != (OlExtSetValuesFunc)NULL)
		{
			if ((*ext->set_values)(current, request, new,
				args, num_args, cur_part, req_part, new_part)
			     == TRUE)
			{
				ret_val = TRUE;
			}
		}
		break;

	    case GET_PROC :
		if (ext->get_values != (OlExtGetValuesProc)NULL)
		{
		    (*ext->get_values)(current, args, num_args, cur_part);
		}
		break;

	    } /* end of switch(proc_type) */
	}

	return (ret_val);
} /* END OF CallInitProc() */

/*
 *************************************************************************
 * GetInstanceData - this routine creates or destroys a vendor's instance
 * extension record (which hangs off the vendor class extension record).
 * The action field determines what to do.  The routine returns the
 * address of the newly created extension or NULL when destroying an
 * extension.
 ****************************procedure*header*****************************
 */
static OlVendorPartExtension
GetInstanceData OLARGLIST((w, action))
	OLARG( Widget,		w)
	OLGRA( int,		action)
{
	OlVendorPartExtension	part = NULL_PART;
	OlVendorClassExtension	extension = GET_EXT(XtClass(w));

					/* Set the widget to be itself	*/
	w = w->core.self;

	if (extension != NULL_EXTENSION)
	{

		if (action == DESTROY)
		{
			OlVendorPartExtension *	part_ptr =
						&extension->part_list;

			for (part = *part_ptr;
			     part != NULL_PART && w != part->vendor;)
			{
				part_ptr	= &part->next;
				part		= *part_ptr;
			}

			*part_ptr = part->next;
			XtFree((char *)part);
			part = NULL_PART;
		}
		else /* CREATE */
		{
			part = (OlVendorPartExtension)XtCalloc(1,
					sizeof(OlVendorPartExtensionRec));
			part->vendor		= w->core.self;
			part->next		= extension->part_list;
			part->class_extension	= extension;
			extension->part_list	= part;
		}
	}
	else
	{
			/* We should never get here	*/

			/* do we need check "w"?	*/
		OlVaDisplayWarningMsg(
			XtDisplay(w),
			OleNfileVendor,
			OleTmsg6,
			OleCOlToolkitWarning,
			OleMfileVendor_msg6
		);
	}
	return(part);
} /* END OF GetInstanceData() */

/****************************procedure*header*****************************
 * ResortTraversalList -
 ****************************procedure*header*****************************/
static void
ResortTraversalList OLARGLIST((array, resort_list))
	OLARG( WidgetArray *,	array)
	OLGRA( Boolean *,	resort_list)
{
	static Widget *		list = NULL;
	static Widget *		ref_list = NULL;
	static int		list_slots_left = 0;
	static int		list_alloced = 0;

	register int		i, how_many, pos;
	Arg			arg[1];
	String			ref_name;


		/* it's a no-op if this flag is off */
	if (*resort_list == False)
		return;

	*resort_list = False;
	how_many = 0;

	if (array != NULL && !_OL_ARRAY_IS_EMPTY(array))
	{
		XtSetArg(arg[0], XtNreferenceName, (XtArgVal)&ref_name);
		list_slots_left = list_alloced;

		for (i = 0; i < _OlArraySize(array); i++)
		{
			ref_name = NULL; /* give ref_name "default" value */
			XtGetValues(_OlArrayElement(array, i), arg, 1);
			if (ref_name != NULL)
			{
				pos = _OlWidgetArrayFindByName(array, ref_name);

					/* free the string */
				_OlFreeRefName(_OlArrayElement(array, i));

				if (pos == _OL_NULL_ARRAY_INDEX)
				{
					OlVaDisplayWarningMsg(
						XtDisplay(_OlArrayElement(array, i)),
						OleNfileVendor,
						OleTmsg5,
						OleCOlToolkitWarning,
						OleMfileVendor_msg5,
						ref_name
					);

					continue;
				}
				if (list_slots_left == 0)
				{
					int	more_slots;

					more_slots = (list_alloced / 2) + 2;
					list_alloced += more_slots;
					list_slots_left += more_slots;
					list = (Widget *) XtRealloc(
							(char *)list,
							list_alloced *
							sizeof(Widget));
					ref_list = (Widget *) XtRealloc(
							(char *)ref_list,
							list_alloced *
							sizeof(Widget));
				}
				ref_list[how_many]=_OlArrayElement(array, pos);
				list[how_many++]=_OlArrayElement(array, i);
				list_slots_left--;
			}
		}
		for (i = 0; i < how_many; i++)
		{
			_OlUpdateTraversalWidget(
				list[i], NULL, ref_list[i], False);
		}
	}
} /* END OF ResortTraversalList */

/****************************procedure*header*****************************
 * SetupInputFocusProtocol - Sets WM_TAKE_FOCUS protocol on `w' and adds
 * an event handler to process the WM_TAKE_FOCUS client messages.
 */
static void
SetupInputFocusProtocol OLARGLIST((w, fd))
	OLARG( Widget,		w)
	OLGRA( OlFocusData *,	fd)
{
	Widget		shell = _OlGetShellOfWidget(w); /* VendorShell Widget*/
	Display *	dpy = XtDisplay(shell);

		/* set WM protocol (TAKE_FOCUS property) on toplevel	*/
	_OlSetWMProtocol(dpy, XtWindow(shell), XA_WM_TAKE_FOCUS(dpy));

	/* add event handler to catch (TAKE_FOCUS) Client Messages	*/

	XtAddEventHandler(shell, (EventMask)NoEventMask, TRUE,
		     TakeFocusEventHandler, (XtPointer)fd);

} /* END OF SetupInputFocusProtocol() */

/*
 *************************************************************************
 * SetWMAttributes -
 ****************************procedure*header*****************************
 */
static void
SetWMAttributes OLARGLIST((w, part, dobusy))
	OLARG( Widget,			w)
	OLARG( OlVendorPartExtension,	part)
	OLGRA( Boolean,			dobusy)
{
	Atom		decor_add_type,
			decor_del_type,
			ol_decor_add,
			ol_decor_del,
			ol_win_attr;
	Boolean		use_OLWinAttr;
	OLWinAttr	wa;
	OwOLWinAttr	ow_wa;
	unsigned char * data;
	int		len_attr;

	struct WinTypeDecor *decor;

	/* define as 4 although we only have 3, change it when having more */
	/* Should do the same in SetValues...			   	   */
#define MAX_DEL_ADD	4
	Atom		del_list[MAX_DEL_ADD], add_list[MAX_DEL_ADD];
	unsigned int	del_constants[MAX_DEL_ADD], add_constants[MAX_DEL_ADD];
#undef MAX_DEL_ADD
	int		delcount = 0, addcount = 0;
	Display *	dpy= XtDisplay(w);
	Window   	win= XtWindow(w);
	Atom		mwm_hints;
	PropMwmHints	prop;

	mwm_hints = XInternAtom (dpy, _XA_MWM_HINTS, False);
	/* For now,  don't decorate transient window at all - the
	 * default for these is O.K. when mwm is running.  However,
	 * some decorations are needed for other types of windows.
	 * Only the OL_WT_OTHER windows get careful considerations.
	 */
	prop.flags |= MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
	switch((int)part->win_type) {
		default:
			break;
		case OL_WT_OTHER:
		   prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
		   prop.decorations = MWM_DECOR_BORDER;
		   prop.functions = MWM_FUNC_MOVE;
		   prop.inputMode = 0;
			break;
		case OL_WT_BASE:
		   prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
		   prop.decorations = MWM_DECOR_BORDER | MWM_DECOR_RESIZEH|
				MWM_DECOR_TITLE | MWM_DECOR_MENU |
				MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE;
		   prop.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE |
				MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE |
				MWM_FUNC_CLOSE;
			break;
		case OL_WT_CMD: /* do nothing, or... */
		   prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
		   prop.decorations = MWM_DECOR_BORDER | MWM_DECOR_RESIZEH|
				MWM_DECOR_TITLE | MWM_DECOR_MENU ;
		   prop.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE |
				MWM_FUNC_CLOSE;
			break;
		case OL_WT_HELP: /* remove resize corners/functionality */
		   prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
		   prop.decorations = MWM_DECOR_TITLE | MWM_DECOR_MENU |
					MWM_DECOR_BORDER | MWM_DECOR_RESIZEH;
		   prop.functions = MWM_FUNC_MOVE | MWM_FUNC_CLOSE |
							MWM_FUNC_RESIZE;
			break;
		case OL_WT_NOTICE: /* allow a border, but no other
				    * decorations.
				    */
		   prop.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
		   prop.decorations = MWM_DECOR_BORDER;
		   prop.functions = MWM_FUNC_MOVE;
			break;
	} /* end switch */

#define SET_FIELD(f,v)	wa.f = (v); ow_wa.f = (v)

		/* set up window attribute structure */
	if ((use_OLWinAttr = UseOLWinAttr(w)) == True)
	{
		len_attr = sizeof(OLWinAttr) / sizeof(long);
		data	 = (unsigned char *)&wa;

		decor_add_type = ol_decor_add = XA_OL_DECOR_ADD(dpy);
		decor_del_type = ol_decor_del = XA_OL_DECOR_DEL(dpy);
	}
	else
	{
		len_attr = sizeof(OwOLWinAttr) / sizeof(long);
		data	 = (unsigned char *)&ow_wa;

		decor_add_type = decor_del_type = XA_ATOM;
		ol_decor_add = XA_OL_DECOR_ADD(dpy);
		ol_decor_del = XA_OL_DECOR_DEL(dpy);
	}

	wa.flags = _OL_WA_WIN_TYPE;
	if (part->pushpin != OL_NONE)
		wa.flags |= _OL_WA_PIN_STATE;

	SET_FIELD(win_type, XA_OL_WT_OTHER(dpy));	/* default */
	SET_FIELD(pin_state, part->pushpin == OL_IN ? 1 : 0);

	for (decor=wintype_decor; decor->win_type != NULL; decor++) {
		if (part->win_type == decor->win_type) {

#define WIN_ATOM	XInternAtom(dpy, decor->win_atom_name, False)

			SET_FIELD(win_type, WIN_ATOM);
#undef WIN_ATOM

			if (part->window_header != decor->window_header) {
				if (decor->window_header == True) {
					del_list[delcount] =
						XA_OL_DECOR_HEADER(dpy);
					del_constants[delcount++] =
						HEADER_CONSTANT;
				}
				else {
					/* not supposed to be a header, but
					 * they want one.	
					 */
					add_list[addcount] =
						XA_OL_DECOR_HEADER(dpy);
					add_constants[addcount++] =
						HEADER_CONSTANT;
				}
			}
			if (part->menu_button != decor->menu_button) {
				if (decor->menu_button == True) {
					/* Supposed to have a menu button,
					 * but they don't want one.
					 * for our window types, we must
					 * assume "limited" menus, but not
					 * complete removal of window menu
					 * buttons in motif mode - maybe we
					 * should check menu type - if ==
					 * OL_MENU_NONE, then we can dump it.
					 */
					del_list[delcount] =
						XA_OL_DECOR_CLOSE(dpy);
					del_constants[delcount++] =
						CLOSE_CONSTANT;
				}
				else {
					add_list[addcount] =
						XA_OL_DECOR_CLOSE(dpy);
					add_constants[addcount++] =
						CLOSE_CONSTANT;
				}
			}
			if (part->resize_corners != decor->resize_corners) {
				if (decor->resize_corners == True) {
					del_list[delcount] =
						XA_OL_DECOR_RESIZE(dpy);
					del_constants[delcount++] =
						RESIZE_CONSTANT;
				}
				else {
					add_list[addcount] =
						XA_OL_DECOR_RESIZE(dpy);
					add_constants[addcount++] =
						RESIZE_CONSTANT;
				}
			}
			if ((part->pushpin != OL_NONE) != 
				(decor->pushpin != False)) {
				if (decor->pushpin != OL_NONE)
					del_list[delcount++] =
						XA_OL_DECOR_PIN(dpy);
				else
					add_list[addcount++] =
						XA_OL_DECOR_PIN(dpy);
			}
			if (part->menu_type != decor->menu_type) {
				wa.flags |= _OL_WA_MENU_TYPE;

				switch(part->menu_type) {
				case OL_MENU_FULL:
				      SET_FIELD(menu_type,XA_OL_MENU_FULL(dpy));
					break;
				case OL_MENU_LIMITED: /* FALL THROUGH */
				case OL_MENU_CANCEL:
					wa.flags |= _OL_WA_CANCEL;
				  SET_FIELD(menu_type, XA_OL_MENU_LIMITED(dpy));

					wa.cancel = part->menu_type ==
							OL_MENU_LIMITED ? 0:1;
					break;
				case OL_NONE:
					wa.flags &= ~_OL_WA_MENU_TYPE;
					SET_FIELD(menu_type, XA_OL_NONE(dpy));
					break;
				}
			}
			break;
		} /* if */
	} /* for */
/* for motif mode, down here we can check delcnt and addcnt - if either
 * is non-zero, then we can work with them to build up our motif hints.
 * must check: resize corners, header (no pushpin), and close.  Must
 * also check the menu type.
 */
	ol_win_attr = XA_OL_WIN_ATTR(dpy);
	XChangeProperty(
		dpy, win, ol_win_attr,
		ol_win_attr, 32, PropModeReplace, data, len_attr);

	if (delcount) {
		int i;

		XChangeProperty(
			dpy, win, ol_decor_del,
			decor_del_type, 32, PropModeReplace,
			(unsigned char *)del_list, delcount
		);

			/* For motif mode... */
		for (i=0; i < delcount; i++)
			switch((int)del_constants[i]) {
				default:
					break;
				case RESIZE_CONSTANT:
					switch(part->win_type) {
						default: break;
						case OL_WT_BASE:
						case OL_WT_CMD:
		   		prop.decorations &= ~ MWM_DECOR_RESIZEH;
		   		prop.functions	 &= ~ MWM_FUNC_RESIZE;
							break;
					}
					break;
				case HEADER_CONSTANT:
					switch((int)part->win_type) {
						default: break;
						case OL_WT_BASE:
						case OL_WT_CMD:
						case OL_WT_HELP:
		   		prop.decorations &= ~ MWM_DECOR_TITLE;
					/* Is functionality affected? */
							break;
					}
					break;
				case CLOSE_CONSTANT:
					/* Effect for popup windows and
					 * for base windows?
					 * Does it still have a menu?
					 * The following takes away the
					 * menu button in motif mode,
					 * leaving only accelerators(?)
					 * to access menu functions...
					 * not really equivalent to olwm,
					 * because in olwm you can still use
					 * the menu button.
					 */
					switch((int)part->win_type) {
						default: break;
						case OL_WT_BASE:
						case OL_WT_CMD:
						case OL_WT_HELP:
		   		prop.decorations &= ~MWM_DECOR_MENU;
					/* Is functionality affected? */
							break;
					}
					break;
			} /* end switch (delete_list) */
	} /* end if(delcount) */

	if (addcount) {
		int i;

		XChangeProperty(
			dpy, win, ol_decor_add,
			decor_add_type, 32, PropModeReplace,
		 	(unsigned char *)add_list, addcount
		);

		for (i=0; i < addcount; i++)
			switch((int)add_constants[i]) {
				default:
					break;
				case RESIZE_CONSTANT:
					switch(part->win_type) {
						default: break;
						case OL_WT_HELP:
						case OL_WT_NOTICE:
						case OL_WT_OTHER:
		   		prop.decorations |=  MWM_DECOR_RESIZEH;
		   		prop.functions	 |=  MWM_FUNC_RESIZE;
							break;
					}
					break;
				case HEADER_CONSTANT:
					switch(part->win_type) {
						default: break;
						case OL_WT_NOTICE:
						case OL_WT_OTHER:
		   		prop.decorations |=  MWM_DECOR_TITLE;
					/* Is functionality affected? */
							break;
					}
					break;
				case CLOSE_CONSTANT:
					switch(part->win_type) {
						default: break;
						case OL_WT_CMD:
						case OL_WT_HELP:
						case OL_WT_NOTICE:
						case OL_WT_OTHER:
		   		prop.decorations &= ~MWM_DECOR_MENU;
					/* Is functionality affected? */
							break;
					}
					break;
			} /* end switch (add_list) */
	} /* end if (addcount) */
	XChangeProperty (dpy, win, mwm_hints,mwm_hints, 
			32, PropModeReplace, 
			(unsigned char *) &prop, PROP_MWM_HINTS_ELEMENTS);
	if (dobusy)
		SetWMWindowBusy(dpy, win, (part->busy == True ?
				WMWindowIsBusy : WMWindowNotBusy));
#undef SET_FIELD
} /* end of SetWMAttributes */

/*
 * UseOLWinAttr - find out the type of "OPEN LOOK WINDOW ATTR"
 *	posted in the base window. It returns True if "OLWinAttr"
 *	is used (defined in OlClients.h) otherwise, it returns False,
 *	i.e., "OwOLWinAttr" is used (defined on the top of this file).
 *
 *	The routine performs the following steps, in sequence:
 *
 *		. if OlIsWMRunning is True
 *		  	then return True
 *		. else if environment variable, "SHORT_OLWINATTR", is set
 *		  and the value is "false"
 *		  	then return True
 *		. else if environment variable, "SHORT_OLWINATTR", is not set
 *		  and applic. resource, useShortOLWinAttr" is False
 *			then return True
 *		. else
 *			return False
 */
	/* copy the code below from OlCommon.c...	*/
#if OlNeedFunctionPrototypes
#include <stdlib.h>	/* for getenv() */
#else
extern char *		getenv();
#endif

static Boolean
UseOLWinAttr OLARGLIST((w))
	OLGRA( Widget,	w)
{
#define FIRST_TIME		-1
#define ENV_VAR			(OLconst char *)"SHORT_OLWINATTR"

	static short		flag = FIRST_TIME; /* cheat a little */

		/* flag should be set to either True or False afterward */
	if (flag == FIRST_TIME)
	{
		char *			value;
		_OlAppAttributes *	ol_attr;

		if (OlIsWMRunning(XtDisplay(w), XtScreen(w)) == True	||
			/* ENV has higher priority...	*/
		    ((value = getenv(ENV_VAR)) != (char *)NULL &&
		     !strcmp(value, (OLconst char *)"false"))				||
		    (value == (char *)NULL &&
		     (ol_attr = _OlGetAppAttributesRef(w),
		      ol_attr->short_olwinattr == False)))
			flag = True;
		else
			flag = False;
	}

	return((Boolean)flag);

#undef FIRST_TIME
#undef ENV_VAR
} /* end of UseOLWinAttr */

/* ARGSUSED */
static char *
VendorGetBase OLARGLIST((w, init, res))
	OLARG( Widget,			w)
	OLARG( Boolean,			init)
	OLGRA(_OlDynResourceList,	res)
{
	OlVendorPartExtension part;

	/*
	 * Normally, you would need to check the init flag and may
	 * need to allocate the extension. But here, vendor is the one
	 * calling this function. Thus the extension must have been
	 * allocated previously.
	 */
/*
	if (init == TRUE) {
		part = GetInstanceData(w, CREATE);
	}
	else
*/
		part = _OlGetVendorPartExtension(w);

	return((char *)part);
} /* end of VendorGetBase */

static XtCallbackList
CreateVendorCBList OLARGLIST((in))
	OLGRA( XtCallbackList,	in)
{
	XtCallbackList cb;
	int i = 0;

	if (in)
		for (cb=in; cb->callback; i++, cb++) ;

	if (i && (cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec) * ++i))) {
		(void)memcpy((char *)cb, (char *)in, sizeof(XtCallbackRec) * i);
		return(cb);
	}
	return(NULL);
} /* end of CreateVendorCBList */
/*
 *************************************************************************
 * UpdateIcGeometries - notify all IC's associated with the current
 * vendor shell of a change in geometry.  All IC's owned by descendants
 * of a single shell share the same status area.
 *************************************************************************
 */
static void	
UpdateIcGeometries OLARGLIST((w, part, geometry))
    OLARG(Widget,			w)
    OLARG(OlVendorPartExtension,	part)
    OLGRA(XtWidgetGeometry *,      	geometry)
{
    int             num_ics = _OlArraySize(&part->ic_list);
    XRectangle  status_area;
    XRectangle  preedit_area;
    OlIcValues ic_values[3];
    int               index;
    TextEditWidget ctx = (TextEditWidget)w;	/* for True StatusArea  */

#ifdef STATUS_DEBUG
    fprintf(stderr, "UpdateIcGeometries: %d ic's in list\n", num_ics);
#endif  

    
    status_area.x = geometry->x;
    status_area.y = geometry->y;
    status_area.width = ctx->core.width;	/* True StatusArea width */
    status_area.height = geometry->height;
    ic_values[0].attr_name = OlNstatusArea;
    ic_values[0].attr_value = (void *)&status_area;

    preedit_area.x = 0;
    preedit_area.y = 0;
    preedit_area.width = ctx->core.width;
    preedit_area.height = ctx->core.height;
    ic_values[1].attr_name = OlNpreeditArea;
    ic_values[1].attr_value = (void *)&preedit_area;
    ic_values[2].attr_name = NULL;
    ic_values[2].attr_value = NULL;

    for (index = 0; index < num_ics; index++){
	OlSetIcValues(_OlArrayElement(&part->ic_list,index).ic, ic_values);
    }
    
} /* end of UpdateIcGeometries */

/*
 *************************************************************************
 * ComareOlIcGeometries - comparison function for OlIcGeometry items.
 * Two items are equal if they have the same IC.
 *************************************************************************
 */
static int
CompareOlIcGeometries OLARGLIST((A, B))
    OLARG(char *,	A)
    OLGRA(char *,	B)
{
    OlIcGeometry *item_A = (OlIcGeometry *)A;
    OlIcGeometry *item_B = (OlIcGeometry *)B;
    int retval = 0;

    if (item_A->ic < item_B->ic)
	retval = -1;
    if (item_A->ic > item_B->ic)
	retval = 1;
    return (retval);

} /* end of CompareOlIcGeometries */


/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
	OLARG( Widget,	w)
	OLGRA( Time *,	time)
{
    if (OlCanAcceptFocus(w, *time))
    {
	OlFocusData *	fd = _OlGetFocusData(w, NULL);
    	WidgetArray *	list = &(fd->traversal_list);
	Widget		the_default;

	/* Try to move focus to the widget that was set to receive it.
	 * If that fails, try the next one.
	 */
    	if (fd->initial_focus_widget != NULL_WIDGET &&
    	    fd->initial_focus_widget != w &&
	    XtCallAcceptFocus(fd->initial_focus_widget, time) == TRUE)
	{
		return(True);
	}
	
	if ((the_default = _OlGetDefault(w)) != NULL_WIDGET &&
	     the_default != w &&
	    XtCallAcceptFocus(the_default, time) == TRUE)
	{
		return(True);
	}

	if (_OlArraySize(list) > (Cardinal)0 &&
	    TraversalHandler(w, _OlArrayElement(list, 0), OL_IMMEDIATE,
				*time) != NULL_WIDGET)
	{
		return(True);
	}

		    /* No initial_focus_widget, no default widget and no
		     * descendent wants focus, so set it to the shell.	*/

	return(OlSetInputFocus(w, RevertToNone, *time));
    }
    return (False);
} /* END OF AcceptFocus() */

/****************************procedure*header*****************************
 * ActivateWidget - this procedure is a default activate proc for vendor
 *			this routine allows the help window worked properly
 *			on OL_CANCEL.
 */
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	Boolean			consumed = False;
	OlVendorPartExtension part = _OlGetVendorPartExtension(w);

	switch (type)
	{
		case OL_CANCEL:
			consumed = True;
				/* means it's not base/applicWindow */
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
			{
				XtPopdown(w);
			}
			else if (part->pushpin != OL_NONE)
			{
				XtUnmapWidget(w);
			}
			break;
		case OL_DEFAULTACTION:
			consumed = True;
			if ((w 	= _OlGetDefault(w)) != (Widget)NULL) {
				(void) OlActivateWidget(w,OL_SELECTKEY,
						(XtPointer)OL_NO_ITEM);
			}
			break;
		case OL_TOGGLEPUSHPIN:
			{
				if (part->pushpin != OL_NONE) {
					consumed = True;
					if (part->pushpin == OL_OUT)
						_OlSetPinState(w, OL_IN);
					else {
						_OlSetPinState(w, OL_OUT);
						if (XtIsSubclass(w,
						    topLevelShellWidgetClass)
						    == False)
						  XtPopdown(w);
						else
						  XtUnmapWidget(w);
					}
				}
			}
			break;
		default:
			break;
	}

	return (consumed);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * ClassInitialize -
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	VendorShellWidgetClass vwc =
				(VendorShellWidgetClass)vendorShellWidgetClass;
	Cardinal size = sizeof(XtResource) * vendor_extension->num_resources;

		/*
		 * The call below was in OlMidInitialize and
		 * in theory, this call should be in OlPreInitialize
		 * but we don't remember why placing it in OlMidInitialize.
		 * So put over here should be safe.
		 */
	OlRegisterConverters();

			/* Quarkify our record_type name so that other
			 * subclasses can use it.			*/

	OlXrmVendorClassExtension = XrmStringToQuark(VENDOR_EXTENSION);

	vendor_extension->header.record_type = OlXrmVendorClassExtension;
	layoutCoreClassExtension.record_type = XtQLayoutCoreClassExtension;

			/* Make a copy of the uncompiled resource
			 * list and stick it in the private slot.  This
			 * way, we can guarantee that we always have
			 * an uncompiled list around.			*/

	vendor_extension->private = (XtResourceList) XtMalloc(size);
	
	(void)memcpy((char *)vendor_extension->private,
			(OLconst char *)vendor_extension->resources, (int)size);

	 vwc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	 vendor_extension->event_procs	= (OlEventHandlerList)
						_OlGenericEventHandlerList;
	 vendor_extension->num_event_procs = (Cardinal)
						_OlGenericEventHandlerListSize;

	_OlAddOlDefineType ((OLconst char *)"click_to_type", OL_CLICK_TO_TYPE);
	_OlAddOlDefineType ((OLconst char *)"click-to-type", OL_CLICK_TO_TYPE);
	_OlAddOlDefineType ((OLconst char *)"real_estate",   OL_REALESTATE);
	_OlAddOlDefineType ((OLconst char *)"real-estate",   OL_REALESTATE);
	_OlAddOlDefineType ((OLconst char *)"none",          OL_NONE);
	_OlAddOlDefineType ((OLconst char *)"out",           OL_OUT);
	_OlAddOlDefineType ((OLconst char *)"in",            OL_IN);
	_OlAddOlDefineType ((OLconst char *)"full",          OL_MENU_FULL);
	_OlAddOlDefineType ((OLconst char *)"limited",       OL_MENU_LIMITED);
	_OlAddOlDefineType (XtNcancel,        OL_MENU_CANCEL);
	_OlAddOlDefineType ((OLconst char *)"base",          OL_WT_BASE);
	_OlAddOlDefineType ((OLconst char *)"cmd",           OL_WT_CMD);
	_OlAddOlDefineType ((OLconst char *)"command",       OL_WT_CMD);
	_OlAddOlDefineType ((OLconst char *)"notice",        OL_WT_NOTICE);
	_OlAddOlDefineType ((OLconst char *)"help",          OL_WT_HELP);
	_OlAddOlDefineType ((OLconst char *)"other",         OL_WT_OTHER);

#ifdef GUI_DEP
	_OlDnDDoExtensionClassInit(&dnd_vendor_extension_rec);
			/* initialize the dnd class extension */
#endif

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * ClassPartInitialize - this routine initializes the vendor shell
 * extension.  It's main responsibility is to incorporate the default
 * resources for subclasses.
 ****************************procedure*header*****************************
 */
static void
ClassPartInitialize OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)
{
	OlVendorClassExtension	super_ext;
	OlVendorClassExtension	ext;
	Cardinal		size;
	Boolean			examine_resources = True;
#ifdef SHARELIB
   void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit      (*__libXol__XtInherit)
#endif

#ifdef	sunOWV3	/*OWV3*/
	_OlFixResourceList(wc);
#endif	/*sun*/	/*OWV3*/

#ifdef GUI_DEP
	_OlDnDDoExtensionClassPartInit(wc);	/* initialize the dnd class
						 * part extension
						 */
#endif

	/* If this widget class is the VendorShellWidgetClass, return. */
	if (wc == vendorShellWidgetClass) {
		return;
	}

			/* Get the superclass's vendor extension.  We
			 * don't have to check to see if we found the
			 * superclass extension, since we always create
			 * one when the superclass came through this
			 * routine earlier.				*/

	super_ext	= GET_EXT(wc->core_class.superclass);
	ext		= GET_EXT(wc);

	if (ext == NULL_EXTENSION)
	{
			/* this class has no extension so create one
			 * for it using the superclass as the template	*/

		size	= sizeof(OlVendorClassExtensionRec);
		ext	= (OlVendorClassExtension) XtMalloc(size);

		(void)memcpy((char *)ext, (OLconst char *)super_ext, (int)size);

		ext->header.next_extension	= VCLASS(wc, extension);
		VCLASS(wc, extension)		= (XtPointer)ext;

			/* if transient shell, put in special resource list,
			 * else set a flag not to examine the resources
			 * since they were copied from the superclass.	*/

		if (wc == transientShellWidgetClass)
		{
			ext->resources		= transient_resources;
			ext->num_resources	= XtNumber(transient_resources);
			ext->private		= (XtResourceList)NULL;
		}
		else
		{
			examine_resources = False;
		}
	}

			/* Now fill in the private resources list	*/

	if (ext->num_resources == (Cardinal)0)
	{
				/* give the super class's resource
				 * lists to this class.			*/

		ext->resources		= super_ext->resources;
		ext->num_resources	= super_ext->num_resources;
		ext->private		= super_ext->private;
	}
	else if (examine_resources == True)
	{
		Cardinal	i;
		Cardinal	j;
		XtResourceList	rsc;

		size = sizeof(XtResource) * super_ext->num_resources;

		rsc = (XtResourceList) XtMalloc(size);
	
			/* Loop over the resources copying superclass
			 * resources that are not specified in this
			 * class.					*/

		for (i=0; i < super_ext->num_resources; ++i)
		{
			for (j=0; j < ext->num_resources; ++j)
			{
				if (!strcmp(ext->resources[j].resource_name,
				    super_ext->private[i].resource_name))
				{
					/* Take this subclass's resource
					 * structure			*/

					rsc[i] = ext->resources[j];
					break;
				}
			}

				/* If j equals the number of resources
				 * in this subclass, then the subclass
				 * did not override the superclass
				 * value so, use the superclass's	*/

			if (j == ext->num_resources) {
				rsc[i] = super_ext->private[i];
			}
		}

				/* cache the results in this extension	*/

		ext->resources		= rsc;
		ext->num_resources	= super_ext->num_resources;

				/* Now make a copy of the uncompiled
				 * resource list in this extension	*/
				
		ext->private = (XtResourceList) XtMalloc(size);

		(void)memcpy((char *)ext->private,
				(OLconst char *)ext->resources, (int)size);
	}

			/* Always initialize the part_list field	*/

	ext->part_list = NULL_PART;

				/* Inherit the SetDefault Procedure	*/

	if (ext->set_default == XtInheritSetDefault) {
		ext->set_default = super_ext->set_default;
	}

				/* Inherit the Traversal Procedure	*/

	if (ext->traversal_handler == XtInheritTraversalHandler) {
		ext->traversal_handler = super_ext->traversal_handler;
	}

				/* Inherit the highlight Procedure	*/

	if (ext->highlight_handler == XtInheritHighlightHandler) {
		ext->highlight_handler = super_ext->highlight_handler;
	}

				/* Inherit the wm_msg Procedure		*/

	if (ext->wm_proc == XtInheritWMProtocolProc) {
		ext->wm_proc = super_ext->wm_proc;
	}

				/* Inherit the transparent Procedure	*/

	if (ext->transparent_proc == XtInheritTransparentProc) {
		ext->transparent_proc = super_ext->transparent_proc;
	}

	if (ext->dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		ext->dyn_data = super_ext->dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(ext->dyn_data), &(super_ext->dyn_data));
	}

} /* END OF ClassPartInitialize() */

/*
 *************************************************************************
 * Destroy - the new vendor shell class destroy procedure.
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)	/* vendor widget being destroyed	*/
{
	OlVendorPartExtension	part_ext;
	OlFocusData *		fd = _OlGetFocusData(w, &part_ext);

				/* Remove the Help Event handler	*/

	XtRemoveEventHandler(w, (EventMask)NoEventMask, True,
		_OlPopupHelpTree, (XtPointer) NULL);

				/* Remove the TakeFocus Handler		*/
	XtRemoveEventHandler(w, (EventMask)NoEventMask, TRUE,
		     TakeFocusEventHandler, (XtPointer)fd);

				/* Remove the WMMessage Handler		*/
	XtRemoveEventHandler(w, (EventMask)PropertyChangeMask, TRUE,
		     WMMessageHandler, (XtPointer)NULL);

	_OlDestroyKeyboardHooks(w);

	_OlUnregisterShell(w);
	


	/* Call the class-extension-part destroy procedure. */

	(void) CallExtPart(DESTROY_PROC, XtClass(w), 
			w,			/* current widget */
			NULL, NULL, NULL, 0,
			(XtPointer)part_ext,	/* current ext part */
			(XtPointer)NULL, (XtPointer)NULL);

#ifdef GUI_DEP
	/* call the dnd class extension destroy method */

	CallDnDVCXExtensionMethods(CallDnDVCXDestroy, XtClass(w), w, 
				   NULL_WIDGET, NULL_WIDGET, (ArgList)NULL,
				   (Cardinal *)NULL);
#endif

				/* Destroy the extension record		*/

	(void)GetInstanceData(w, DESTROY);

#ifdef	OpenWindowsEnvironment	/*OWV3*/
	_OlDelWMColormapWindows(w);
#endif	/*OpenWindowsEnvironment*/	/*OWV3*/
} /* END OF Destroy() */

/****************************procedure*header*****************************
 * ExtDestroy - Class Extension Destroy procedure
 */
/* ARGSUSED */
static void
ExtDestroy OLARGLIST((w, part_ext))
	OLARG( Widget,		w)	   /* vendor widget being destroyed */
	OLGRA( XtPointer,	part_ext)  /* Class extension part */
{
    OlVendorPartExtension part = (OlVendorPartExtension)part_ext;

    if (part == NULL)
	return;

    _OlArrayFree (&part->focus_data.traversal_list);

    /* free the IC list in the part extension */
    _OlArrayFree(&part->ic_list);

    if (part->consume_event)
	XtFree((char *)part->consume_event);

    if (part->wm_protocol)
	XtFree((char *)part->wm_protocol);
} /* end of ExtDestroy */

/*
 *************************************************************************
 * Realize - Sets up the input focus protocol with the window manager if
 * there is a focus widget.
 * This can only be done at realize time, since we need a window to put
 * the take focus property on.
 ****************************procedure*header*****************************
 */
static void
Realize OLARGLIST((w, value_mask, attributes))
	OLARG( Widget,			w)
	OLARG( XtValueMask *,		value_mask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
	XtRealizeProc		super_realize;
	OlVendorPartExtension	part_ext;
	OlFocusData *		fd = _OlGetFocusData(w, &part_ext);
	Display *		dpy = XtDisplay(w);


			/* Use the superclass to create the window	*/

	super_realize = vendorShellWidgetClass->core_class.
				superclass->core_class.realize;

	if (super_realize != (XtRealizeProc)NULL) {
		(*super_realize)(w, value_mask, attributes);
	} else {
		OlVaDisplayErrorMsg(
			XtDisplay(w),
			OleNfileVendor,
			OleTmsg4,
			OleCOlToolkitError,
			OleMfileVendor_msg4
		);
	}

	if (fd != NULL_FOCUS_DATA) {
		SetupInputFocusProtocol(w, fd);
	}

	/* always interested in these WM messages */
	if (_OL_WM_TESTBIT(part_ext->wm_protocol_mask,
			OL_WM_DELETE_WINDOW))
		_OlSetWMProtocol(XtDisplay(w), XtWindow(w),
			XA_WM_DELETE_WINDOW(dpy));
	if (_OL_WM_TESTBIT(part_ext->wm_protocol_mask,
			OL_WM_SAVE_YOURSELF))
		_OlSetWMProtocol(XtDisplay(w), XtWindow(w),
			XA_WM_SAVE_YOURSELF(dpy));

	SetWMAttributes(w, part_ext, (part_ext->busy) ? True : False);

#ifdef GUI_DEP
	/* do some post-realisation setup for the dnd class extension */

	_OlDnDCallVCXPostRealizeSetup(w);
#endif

#ifdef	OpenWindowsEnvironment	/*OWV3*/
	_OlAddWMColormapWindows(w);
#endif	/*OpenWindowsEnvironment*/	/*OWV3*/

	OlCheckRealize (w);

	return;
} /* END OF Realize() */
/*
 * StatFile
 *
 * This function searches downward from \fIpath\fP for the file \FIname\fP.
 *
 */

static char *
StatFile OLARGLIST((path, name))
	OLARG(char *, path)
	OLGRA(char *, name)
{
   DIR *           dp;
   struct dirent * p;
   char *          pwd;
   char *          filename = NULL;
   char            pwd_buffer[PATH_MAX];
   char            buffer[PATH_MAX];

   if ((pwd = getcwd(pwd_buffer, PATH_MAX)) != NULL)
   {
      if (chdir(path) == 0)
      {
         if ((dp = opendir(".")) != NULL)
         {
            while((p = readdir(dp)) != NULL)
            {
               if (*p->d_name != '.')
               {
                  strcpy(buffer, p->d_name);
                  strcat(buffer, "/");
                  strcat(buffer, name);

                  if (access(buffer, R_OK) == 0)
                  {
                     filename = (char *)strdup(buffer);
                     break;
                  }
               }
            }
            closedir(dp);
         }
         chdir(pwd);
      }
   }

   return (filename);

} /* end of StatFile */
/*
 * DirPredicate
 *
 * This file search predicate function is used to find path which is a
 * directory.  The default predicate for XtResolvePathname() does not
 * consider files which are directories as found paths.  This function
 * overrides that behaviour.
 *
 */

static Boolean
DirPredicate OLARGLIST((path))
	OLGRA(char *, path)
{
   struct stat status;

   return ( (stat(path, &status) == 0) &&
            (status.st_mode & S_IFDIR) &&
            (access(path, R_OK|X_OK) == 0)  );

} /* end of DirPredicate */
/*
 * FindHelpFile
 *
 * This function searches for the application's help file which is
 * presumed to be in the file '<application_name>.hlp'.  The help
 * directory (retrieved using XtResolvePathname()) is searched using
 * the function StatFile.  To allow the XtResolvePathname() function
 * to find a directory a special predicate function (DirPredicate())
 * is used.
 *
 * This function is designed to return the value retrieved in the previous
 * call.  This is an optimization since an application can have many
 * shells.  To do this the filename returned is defined as static initialized
 * to NULL.  On the first call the function ensures that the filename
 * will be set to either the path found in the search or to the constant
 * null-string ("").  The caller (Initialize()) checks to see if the 
 * contents of the return string is greater that length zero.  If not
 * it does not register help since the path could not be found.
 *
 */

static char *
FindHelpFile OLARGLIST((w))
	OLGRA(Widget, w)
{
   static char * filename = NULL;

   char *        name;
   char *        class;
   char *        path;
   char          buffer[PATH_MAX];

   if (filename == NULL)
   {
      filename = "";

      XtGetApplicationNameAndClass(XtDisplay(w), &name, &class);

      path = XtResolvePathname(XtDisplay(w), "help", "", "", NULL, 
                               NULL, 0, DirPredicate);

      if (path != NULL)
      {
         strcpy(buffer, name);
         strcat(buffer, ".hlp");

         if ((filename = StatFile(path, buffer)) == NULL)
            filename = "";

         XtFree(path);
      }
   }

   return (filename);

} /* end of FindHelpFile */

/*
 *************************************************************************
 * Initialize - this routine is the new vendor shell class initialize
 * procedure.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	OlVendorPartExtensionRec	req_part;
	OlVendorPartExtension		part = GetInstanceData(new, CREATE);
	OlFocusData *			fd = &(part->focus_data);
	MaskArg				mask_args[4];
	XtCallbackList			cb = NULL;
	XtCallbackList			wm_cb = NULL;
        char *                          p;

	static Boolean			first_time = True;

	if (first_time == True)
	{
		extern Widget		OlApplicationWidget;
		extern Display *	toplevelDisplay;

		first_time = False;

		OlApplicationWidget = new;
		toplevelDisplay = XtDisplay(new);

		OlPostInitialize(
			(String)NULL,			/* classname	*/
			(XrmOptionDescRec *)NULL,	/* urlist	*/
			(Cardinal)0,			/* num_urs	*/
			(int *)NULL,			/* argc		*/
			(char **)NULL			/* argv		*/
		);
	}

			/* Make sure this shell does not redirect
			 * keypress events				*/

	XtSetKeyboardFocus(new, NULL_WIDGET);

			/* Add an event handler to trap the help client
			 * messages from the window manager.		*/

	XtAddEventHandler(new, (EventMask)NoEventMask, True,
				_OlPopupHelpTree, (XtPointer) NULL);

        /*
         * Find the help file to use if the client doesn't register help
         * Note: the semantics of OlRegisterHelp are that if the client
         * subsequently registers help on the same id (e.g., widget) then
         * the previous registration is discarded.
         */

        if ((p = FindHelpFile(new)) != NULL && *p)
           OlRegisterHelp(OL_WIDGET_HELP, new, "", OL_DISK_SOURCE, p);

			/* Add an event handler to catch client messages
			 * from the window manager concerning
			 * WM protocol.					*/
	XtAddEventHandler(new, (EventMask)PropertyChangeMask,
			True, WMMessageHandler, (XtPointer) NULL);

			/* Now set the subvalue data.  We'll use the 
			 * resource database to do this.
			 * NOTE: that I've hacked the XtGetSubresources
			 * call to use a NULL name and class string.
			 * Hopefully, the Instrinsics will publicly
			 * allow this.					*/

	XtGetSubresources(new, (XtPointer)part, (String)NULL, (String)NULL,
			part->class_extension->resources,
			part->class_extension->num_resources, args, *num_args);

			/* Check XtNconsumeEvent */
	_OlSetMaskArg(mask_args[0], XtNconsumeEvent, &cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlSetMaskArg(mask_args[2], XtNwmProtocol, &wm_cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);
	part->consume_event = CreateVendorCBList(cb);
	part->wm_protocol   = CreateVendorCBList(wm_cb);
	
			/* Set the request part equal to the new part	*/

	req_part = *part;

			/* Initialize the OlFocusData structure.	*/

	fd->focus_gadget		= NULL_WIDGET;
	fd->activate_on_focus		= NULL_WIDGET;
	fd->current_focus_widget	= NULL_WIDGET;
	fd->resort_list			= False;
	_OlArrayInit (&fd->traversal_list);

	part->default_widget		= NULL_WIDGET;
	part->accelerator_list		= 0;
	part->shell_list		= 0;

	/* must always express interests on these message types */
	part->wm_protocol_mask	    |= OL_WM_DELETE_WINDOW | OL_WM_TAKE_FOCUS;

		/* add shell to traversal list, since shell doesn't have */
		/* reference_name and reference_widget resources, so     */
		/* use NULLs instead                                     */
	_OlUpdateTraversalWidget(new, NULL, NULL_WIDGET, True);


	/* Add this shell to the list for tracking
	   dynamic changes to accelerators
	 */
	_OlRegisterShell(new);

	/* Call the class extension part initialize procedure. */

	(void) CallExtPart(INIT_PROC, XtClass(new), NULL_WIDGET,
			request, new, args, num_args, (XtPointer)NULL,
			(XtPointer)&req_part, (XtPointer)part);

	_OlInitDynResources(new, &(part->class_extension->dyn_data));
	_OlCheckDynResources(new, &(part->class_extension->dyn_data),
			 args, *num_args);

#ifdef GUI_DEP
	/* now initialize the dnd class ext part */

	CallDnDVCXExtensionMethods(CallDnDVCXInitialize, XtClass(new),
				   NULL_WIDGET, request, new, args, num_args);
#endif

	/* MooLIT extension for menubars */
	
	part->menubar_widget = NULL_WIDGET;

	/* initialize the list of ic's for input method */
	_OlArrayInitialize(&part->ic_list, _OlArrayDefaultInitial, _OlArrayDefaultStep,
			   CompareOlIcGeometries);
	/* initialize the geometry of the status area to 0 */
	part->status_area_geometry.request_mode = 0;
	
#if	defined(GEOMETRY_DEBUG)
	geometry_debug = (getenv("GEOMETRY_DEBUG") != 0);
#endif
} /* END OF Initialize() */

/*
 *************************************************************************
 * GetValues - this routine is the new vendor shell class GetValues
 * procedure.
 ****************************procedure*header*****************************
 */
static void
GetValues OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)	/* VendorShellWidget subclass	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	OlVendorPartExtension	part = _OlGetVendorPartExtension(w);

					/* Get the subvalue data.	*/

	if (part != NULL_PART) {
		MaskArg mask_args[4];

		XtGetSubvalues((XtPointer)part,
			part->class_extension->resources,
			part->class_extension->num_resources, args, *num_args);

		/* XtNconsumeEvent */
		_OlSetMaskArg(mask_args[0], XtNconsumeEvent,
			      part->consume_event, OL_COPY_MASK_VALUE);
		_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList),
			      OL_COPY_SIZE);
		_OlSetMaskArg(mask_args[2], XtNwmProtocol,
			      part->wm_protocol, OL_COPY_MASK_VALUE);
		_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList),
			      OL_COPY_SIZE);
		_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);

		(void) CallExtPart(GET_PROC, XtClass(w),
				w, NULL_WIDGET, NULL_WIDGET, args, num_args,
				(XtPointer)part, (XtPointer)NULL,
				(XtPointer)NULL);
	}

#ifdef GUI_DEP
	/* now do a get for the dnd class extension */

	CallDnDVCXExtensionMethods(CallDnDVCXGetValues, XtClass(w), w,
				   NULL_WIDGET, NULL_WIDGET, args, num_args);
#endif
} /* END OF GetValues() */

/*
 *************************************************************************
 * SetValues - this routine is the new vendor shell class SetValues
 * procedure.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	OlVendorClassExtension		ext;
	OlVendorPartExtensionRec	cur_part;
	OlVendorPartExtensionRec	req_part;
	OlVendorPartExtension		new_part =
					_OlGetVendorPartExtension(new);
	Boolean				ext_return;
	/* define as 4 although we only have 3, change it when having more */
	/* Should do the same in SetWMAttributes...			   */
#define MAX_DEL_ADD	4
	Atom				del_list[MAX_DEL_ADD];
	Atom 				add_list[MAX_DEL_ADD];
#undef MAX_DEL_ADD
	Display *			dpy = XtDisplay(new);
	int 				delcount = 0;
	int 				addcount = 0;
	XtCallbackList			cb = NULL;
	XtCallbackList			wm_cb = NULL;
	MaskArg				mask_args[4];
	unsigned int			del_constants[2],
					add_constants[2];
	XtWidgetGeometry               *cur_status, *new_status;

	if (new_part == NULL_PART) {
		return(False);
	}

	ext = new_part->class_extension;

				/* make a copy of the record, then do
				 * a set values on it.			*/

	cur_part	= *new_part;

				/* Fill in the application's requests.
				 * (Do this in the new part and then
				 * copy the results into the request
				 * part.				*/

	XtSetSubvalues((XtPointer)new_part, ext->resources,
			ext->num_resources, args, *num_args);
	req_part = *new_part;
	
	/* The request_mode field of the XtWidgetGeometry structure
	 * is used by the caller to indicate which fields are being
	 * updated.  All other fields retain their previous values.
	 */

	cur_status = &cur_part.status_area_geometry;
	new_status = &new_part->status_area_geometry;
#ifdef STATUS_DEBUG
#define NEWLY_SET(flag,field) \
	((new_status->request_mode & flag) && \
	 (cur_status->field != new_status->field))
	fprintf(stderr,"Vendor SV: change %s %s %s %s",
		(NEWLY_SET(CWX,x)) ? "X": "",
		(NEWLY_SET(CWY,y)) ? "Y": "",
		(NEWLY_SET(CWWidth,width)) ? "width": "",
		(NEWLY_SET(CWHeight,height)) ? "height": "");
#undef NEWLY_SET		
	fprintf(stderr,"new status[x,y,w,h]= [%d,%d,%d,%d]\n",
		new_status->x, new_status->y, new_status->width, new_status->height);
#endif

#define COPY_IF_NOT_SET(flag,field) 			\
	if (!(new_status->request_mode & (flag)) && 	\
	    (cur_status->request_mode & (flag))){ 	\
	    new_status->request_mode |= flag;		\
	    new_status->field = cur_status->field; 	\
	}						\
	else
	COPY_IF_NOT_SET(CWX, x);
	COPY_IF_NOT_SET(CWY, y);
	COPY_IF_NOT_SET(CWWidth, width);
	COPY_IF_NOT_SET(CWHeight, height);
#undef COPY_IF_NOT_SET
	
	/* if the geometry is completely specified and a change
	 * occurred in this call, call OlSetIcValues 
	 */

#define CHANGED(F) (new_status->F != cur_status->F)
	if (((new_status->request_mode & (CWX | CWY | CWWidth | CWHeight)) ==
	    (CWX | CWY | CWWidth | CWHeight)) &&
	    (CHANGED(x) | CHANGED(y) | CHANGED(width) | CHANGED(height)))
	    UpdateIcGeometries(new, new_part, &new_part->status_area_geometry);
#undef CHANGED


	/* XtNconsumeEvent */
	_OlSetMaskArg(mask_args[0], XtNconsumeEvent, &cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlSetMaskArg(mask_args[2], XtNwmProtocol, &wm_cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);
	if (cb) {
		if (new_part->consume_event)
			XtFree((char *)new_part->consume_event);
		new_part->consume_event = CreateVendorCBList(cb);
	}
	if (wm_cb) {
		if (new_part->wm_protocol)
			XtFree((char *)new_part->wm_protocol);
		new_part->wm_protocol = CreateVendorCBList(wm_cb);
	}

	if (XtIsRealized(new)) {
			    /* read-only resource after initialize	*/
	    new_part->window_header = cur_part.window_header;

	    if ((new_part->menu_type != cur_part.menu_type) ||
		(new_part->busy != cur_part.busy) ||
		(new_part->pushpin != cur_part.pushpin))
		SetWMAttributes(new, new_part, 
			(new_part->busy != cur_part.busy) ? True : False);
	    else {
	    if ((new_part->menu_button != cur_part.menu_button) &&
		(new_part->window_header == True)) {
			if (new_part->menu_button == True) {
				add_list[addcount] = XA_OL_DECOR_CLOSE(dpy);
				add_constants[addcount++] = CLOSE_CONSTANT;
			}
			else {
				del_list[delcount] = XA_OL_DECOR_CLOSE(dpy);
				del_constants[delcount++] = CLOSE_CONSTANT;
			}
	    }
	
	    if (new_part->resize_corners != cur_part.resize_corners) {
			if (new_part->resize_corners == True) {
				add_list[addcount++] = XA_OL_DECOR_RESIZE(dpy);
				add_constants[addcount++] = RESIZE_CONSTANT;
			}
			else {
				del_list[delcount] = XA_OL_DECOR_RESIZE(dpy);
				del_constants[delcount++] = RESIZE_CONSTANT;
			}
	    }

	if (addcount || delcount) {
		int		i;
		PropMwmHints	mwmh;
		Atom		mwm_hints;

		mwm_hints = XInternAtom (dpy, _XA_MWM_HINTS, False);
		_OlGetMWMHints(dpy, XtWindow(new),&mwmh);
	    if (delcount) {
		Atom	ol_decor_del = XA_OL_DECOR_DEL(dpy);

		XChangeProperty(dpy, XtWindow(new), ol_decor_del,
			ol_decor_del, 32, PropModeReplace,
		 	(unsigned char *) del_list, delcount);
		for (i=0; i < delcount; i++) {
			switch((int)del_constants[i]) {
				default:
					break;
				case RESIZE_CONSTANT: /* any win type */
		   			mwmh.decorations &=  ~MWM_DECOR_RESIZEH;
		   			mwmh.functions	 &=  ~MWM_FUNC_RESIZE;
					break;
				case CLOSE_CONSTANT:
		   			mwmh.decorations &= ~MWM_DECOR_MENU;
					/* Is functionality affected? */
					break;
			} /* end switch (del_constants) */
		} /* for */
	    } /* end if (delcount) */

	    if (addcount)  {
		Atom	ol_decor_add = XA_OL_DECOR_ADD(dpy);

		XChangeProperty(dpy, XtWindow(new), ol_decor_add,
			ol_decor_add, 32, PropModeReplace,
		 	(unsigned char *) add_list, addcount);
		for (i=0; i < addcount; i++) {
			switch((int)add_constants[i]) {
				default:
					break;
				case RESIZE_CONSTANT: /* any win type */
		   			mwmh.decorations |=  MWM_DECOR_RESIZEH;
		   			mwmh.functions	 |=  MWM_FUNC_RESIZE;
					break;
				case CLOSE_CONSTANT:
		   			mwmh.decorations |= MWM_DECOR_MENU;
					/* Is functionality affected? */
					break;
			} /* end switch (add_list) */
		} /* for() */
	} /* end if (addcount) */
	XChangeProperty (dpy, XtWindow(new), mwm_hints,mwm_hints, 
			32, PropModeReplace, 
			(unsigned char *) &mwmh, PROP_MWM_HINTS_ELEMENTS);

	} /* end if addcount || delcount */
	    } /* else */
	}

	/*
	 * This line assumes that THIS SetValues always return False,
	 * thus saving the OR operation with the extension return value.
	 */
	if (ext->set_values) {
		/* 
		 * special set_values proc for subclasses that wants to
		 * override Vendor's setvalues work.  This is done after
		 * everything else in this procedure since subclasses
		 * normally get their set_values procedure calling only.
		 * after the superclass has completed it's work.
		 */
		ext_return = CallExtPart(SET_FUNC, XtClass(new), current,
				request, new, args, num_args,
				(XtPointer)&cur_part, (XtPointer)&req_part,
				(XtPointer)new_part);
	}
	else {
		ext_return   = False;
	}

	_OlCheckDynResources(new, &(ext->dyn_data), args, *num_args);

	/* handle background transparency */
    	if ((_OlDynResProcessing == FALSE) &&
	    (new->core.background_pixel != current->core.background_pixel) ||
            (new->core.background_pixmap != current->core.background_pixmap)) {
		int i;
		WidgetList child = ((CompositeWidget)new)->composite.children;
		OlTransparentProc proc;
		
		for (i=((CompositeWidget)new)->composite.num_children; i > 0;
		 	i--,child++) {
			if (proc = _OlGetTransProc(*child))
				(*proc)(*child, new->core.background_pixel,
					new->core.background_pixmap);
		} /* for */
	} /* if */

#ifdef GUI_DEP
	ext_return |= CallDnDVCXExtensionMethods(CallDnDVCXSetValues,
						 XtClass(new), current,
						 request, new, args, num_args);

#endif
	return(ext_return);
} /* END OF SetValues() */

/****************************procedure*header*****************************
 *
 * _OlGetMWMHints() - get MWM_HINTS property from window w.
 */
/* ARGSUSED */
extern int
_OlGetMWMHints OLARGLIST((display, w, mwmh))
	OLARG( Display *,		display)
	OLARG( Window,		w)
	OLGRA( PropMwmHints *,	mwmh)
{
	Atom			atr;
	int			afr;
	unsigned long		nir,
				bar;
	PropMwmHints		*prop;
	int			Failure;
	Atom	mwm_hints;
	mwm_hints = XInternAtom (display, _XA_MWM_HINTS, False);

	if ((Failure = XGetWindowProperty(display, w, mwm_hints, 0L,
				PROP_MWM_HINTS_ELEMENTS, False,
				mwm_hints, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop))) != Success)
		return Failure;

        if (atr != mwm_hints||
			nir <  PROP_MWM_HINTS_ELEMENTS || afr != 32)
	{
               if (prop != (PropMwmHints *) 0)
                        free((char *)prop);

                return BadValue;
	}
	else
	{
		mwmh->flags = prop->flags;
		mwmh->decorations = prop->decorations;
		mwmh->functions = prop->functions;
		mwmh->inputMode = prop->inputMode;
		if (prop != (PropMwmHints *) NULL)
			free((char *)prop);
		return Success;
	}
} /* end _OlGetMWMHints() */

/****************************procedure*header*****************************
 *
 * TraversalHandler - traverses to next object
 */
/* ARGSUSED */
static Widget
TraversalHandler OLARGLIST((shell, w, direction, time))
	OLARG( Widget,		shell)		/* traversal manager */
	OLARG( Widget,		w)		/* starting widget */
	OLARG( OlDefine,	direction)
	OLGRA( Time,		time)
{
    Boolean	via_focus_change = False;
    OlFocusData * focus_data = _OlGetFocusData(shell, NULL_PART_PTR);
    WidgetArray * list;			/* traversal list */
    int		start_pos;
    int		pos;
    Time	timestamp = time;
    OlDefine	old_dir = direction;

    if (w == NULL)
    {
		/* Called from Action.c:HandleFocusChange() -
		 * see notes there for reasons,
		 * if this is the case, then don't check on
		 * shell and don't give focus to "shell" if
		 * no one can take focus at the time.
		 */
	via_focus_change = True;
	w = shell;
    }

    if (focus_data == NULL)
    {
	OlVaDisplayWarningMsg(
		XtDisplay(shell),
		OleNfileVendor,
		OleTmsg3,
		OleCOlToolkitWarning,
		OleMfileVendor_msg3
	);
	return (NULL_WIDGET);
    }


    /*
     * If shell is not sensitive, don't bother switching focus, just set
     * focus to the shell.
     */
    if (!via_focus_change && XtIsSensitive(shell) == False) {
        Boolean flag1 = shell->core.sensitive;
	Boolean flag2 = shell->core.ancestor_sensitive;

        shell->core.sensitive = True;
	shell->core.ancestor_sensitive = True;
	if (OlCanAcceptFocus(shell, time) == False)
		w = NULL_WIDGET;
	else
	{
	    if (OlSetInputFocus(shell, RevertToNone, time))
		w = shell;
	    else
		w = NULL_WIDGET;
	}
        shell->core.sensitive = flag1;
	shell->core.ancestor_sensitive = flag2;
	return(w);
    }

    /*	re-map direction to inter-object movement (ones understood by
	shell traversal handler.
     */
    switch(direction) {

    case OL_MOVELEFT:
    case OL_MULTILEFT:
    case OL_MOVEUP:
    case OL_MULTIUP:
	direction = OL_PREVFIELD;
	break;

    case OL_MOVERIGHT:
    case OL_MULTIRIGHT:
    case OL_MOVEDOWN:
    case OL_MULTIDOWN:
	direction = OL_NEXTFIELD;
	break;
    }

    list = &(focus_data->traversal_list);	/* get list */

		/* resort the list if necessary */
    ResortTraversalList(list, &focus_data->resort_list);

    /*	Get position in list. */
    if ((start_pos = _OlArrayFind(list, w)) == _OL_NULL_ARRAY_INDEX)
    {
	Widget ancestor = w;

	/* widget not found in list.  Walk up the widget tree looking
	   for an ancestor which could be considered its "managing"
	   ancestor.  This works in particular for controls inside a
	   [non]exclusives, for instance.  The control receives the
	   traversal action key but it's the ancestor that is on the
	   list.

	   It's okay to make ancestor = XtParent(w) right away since
	   ancestor != shell; shell would have been found on list.
	 */
	do
	{
	    ancestor = XtParent(ancestor);
	    if ((start_pos = _OlArrayFind(list, ancestor))
	      != _OL_NULL_ARRAY_INDEX)
		break;
	} while (ancestor != shell);	/* do this last so that shell
					   can be looked for as well
					 */
    }

    /*	if start_pos is *still* NULL, make it zero (start from beginning and
	issue a warning, otherwise, adjust start_pos according to direction.
	(It's unlikely no ancestor was found on list since Shell is put
	on list.  It *is* possible that shell is not Vendor shell).
     */

    if (start_pos == _OL_NULL_ARRAY_INDEX)
    {
	start_pos = 0;

	OlVaDisplayWarningMsg(
		XtDisplay(shell),
		OleNfileVendor,
		OleTmsg2,
		OleCOlToolkitWarning,
		OleMfileVendor_msg2,
		XtName(shell),
		XtName(w),
		OlWidgetToClassName(w)
	);

    } else {
	switch (direction)
	{
	case OL_NEXTFIELD :
	    start_pos = (start_pos + 1) % _OlArraySize(list);
	    break;

	case OL_PREVFIELD :
	    start_pos = (start_pos == 0) ?
				_OlArraySize(list) - 1 : start_pos - 1;
	    break;

	case OL_IMMEDIATE :
	    direction = OL_NEXTFIELD;	/* for processing at bottom of loop */
	    break;			/* but start_pos remains the same */

	default :
	    OlVaDisplayWarningMsg(
			XtDisplay(w),
			OleNfileVendor,
			OleTmsg1,
			OleCOlToolkitWarning,
			OleMfileVendor_msg1
	    );
	    direction = OL_NEXTFIELD;	/* fixup direction */
	    break;
	}
    }

    /*	enter main loop to find widget to traverse to.
	Any intra-object traversal direction has been remapped to
	inter-object traversal and IMMEDIATE has been changed to
	NEXTFIELD for processing at bottom of loop.  'start_pos' has
	been established based on direction.

	The shell is skipped in the list since it is not intuitive to
	traverse to it.  It must be in the list, however, to be able
	to traverse *from* it to some descendant.
     */
    pos = start_pos;		/* initial index */

    do
    {
	register Widget trav_widget = _OlArrayElement(list, pos);	/* get widget */

	/* Check for widget being destroyed */
	if (trav_widget->core.being_destroyed == False)
	{
	    WidgetClass	wc_special = _OlClass(trav_widget);
	    Boolean	traversable;

	    if (wc_special == primitiveWidgetClass)
	    {
		traversable = ((PrimitiveWidget)trav_widget)->primitive.traversal_on;

	    } else if (wc_special == eventObjClass) {
		traversable = ((EventObj)trav_widget)->event.traversal_on;

	    } else if (wc_special == managerWidgetClass) {
		traversable = ((ManagerWidget)trav_widget)->manager.traversal_on;

	    } else if (wc_special == vendorShellWidgetClass) {
		traversable = False;	/* skip the shell */

	    } else {
		traversable = False;
	    }

			/* Inform *flat buttons* thru a private resource,
			 * XtNtraversalType, so that *flat buttons* knows
			 * how to deal with [next|prev]FieldKey.
			 * note that *flat buttons* will re-set this value
			 * to "OL_IGNORE" afterward.
			 */
	    if (traversable &&
	        (old_dir == OL_PREV_FIELD || old_dir == OL_NEXT_FIELD))
	    {
		Arg	args[1];
		XtSetArg(args[0], XtNtraversalType, (XtArgVal)old_dir);
		XtSetValues(trav_widget, args, 1);
	    }

	    /* Check for traversable widget that can take focus */
	    if (traversable && XtCallAcceptFocus(trav_widget, &timestamp))
		return(trav_widget);
	}

	/* get "next" index depending on direction */
	if (direction == OL_NEXTFIELD)
	{
	    pos = (pos + 1) % _OlArraySize(list);	/* next index */
	
	} else if (pos != 0) {
	    pos--;				/* previous index (simple) */

	} else {
	    pos = _OlArraySize(list) - 1;	/* previous (ring around) */
	}

    } while (pos != start_pos);

		/* there is a timing problem here if we don't do this	*/
		/* check. A BadMatch error will happen if a user pops	*/
		/* up/down a menu really quick (e.g., press-drag)	*/
    if (!via_focus_change && OlCanAcceptFocus(shell, time))
    {
	if (OlSetInputFocus(shell, RevertToNone, time))
	    return(shell);
    }
    return(NULL_WIDGET);

} /* END OF TraversalHandler() */

static void
WMMsgHandler OLARGLIST((w, action, wmpv))
	OLARG( Widget, 			w)
	OLARG( OlDefine,		action)
	OLGRA( OlWMProtocolVerify *,	wmpv)
{
	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {
		switch(action) {
		case OL_QUIT:
		case OL_DEFAULTACTION:
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
				XtPopdown(w);
			else {
				OlVendorPartExtension	part = 
					_OlGetVendorPartExtension(
					 _OlGetShellOfWidget(w));

				XtUnmapWidget(w);
			    	if (part->pushpin == OL_NONE)
					exit(0);
			}
			break;
		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		case OL_DISMISS:
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
				XtPopdown(w);
			else
				XtUnmapWidget(w);
			break;
		}
	}
} /* end of WMMsgHandler */

/**
 ** FetchChild()
 **/

static Widget
#if     OlNeedFunctionPrototypes
FetchChild (
        Widget                  w,
        Boolean *               more
)
#else
FetchChild (w, more)
        Widget                  w;
        Boolean *               more;
#endif
{
        Widget                  returned_child = 0;
        Widget                  child;

        Cardinal                n;


        if (more)
                *more = False;
        FOR_EACH_MANAGED_CHILD (w, child, n)
                if (!returned_child)
                        returned_child = child;
                else {
                        if (more)
                                *more = True;
                        break;
		    }

        return (returned_child);
} /* FetchChild */

/**
 ** ChangeManaged()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeManaged (
	Widget			w
)
#else
ChangeManaged (w)
	Widget		w;
#endif
{
	Widget		child = FetchChild(w, (Boolean *)0);


	/*
	 * We need to envelope the Shell change_managed, to get the
	 * XtNgeometry resource parsed and various window manager hints
	 * set. Examination of Shell.c (X11R5) shows that all its
	 * change_managed procedure does is call an internal routine
	 * (GetGeometry) that does the XtNgeometry and wm hints stuff,
	 * then pass the geometry on to its only child. We don't want
	 * to configure the child yet, since we may have to account
	 * for a status area. However, we do need to allow the child's
	 * current geometry to be used as the default geometry of the
	 * shell.
	 *
	 * None of this matters if the widget is already realized.
	 */

	if (!XtIsRealized(w) && child) {
		XtWidgetProc change_managed =
		COMPOSITE_C(SUPER_C(vendorShellWidgetClass)).change_managed;

		Cardinal num_children = COMPOSITE_P(w).num_children;


		/*
		 * See Xt/Shell.c:GetGeometry
		 */
		if (!CORE_P(w).width || !CORE_P(w).height)
			((WMShellWidget)w)->wm.size_hints.flags |= PSize;
		if (!CORE_P(w).width)
			CORE_P(w).width = CORE_P(child).width;
		if (!CORE_P(w).height)
			CORE_P(w).height = CORE_P(child).height;
		COMPOSITE_P(w).num_children = 0;
		(*change_managed) (w);
		COMPOSITE_P(w).num_children = num_children;
	}

	/*
	 * Can't use _OlDefaultChangeManaged, since it passes True for
	 * the resizable flag, and this tells Layout to use the child's
	 * size as the desired size. But we don't want that, we want to
	 * configure the child to the (new) shell size.
	 *
	 * MORE: Change _OlDefaultChangeManaged to act differently with
	 * shells?
	 */
	OlSimpleLayoutWidget (w, False, False);
	OlUpdateHandles (w);

	return;
} /* ChangeManaged */

/**
 ** Layout()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Layout (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
Layout (w, resizable, query_only, cached_best_fit_ok_hint, who_asking, request, response)
	Widget			w;
	Boolean			resizable;
	Boolean			query_only;
	Boolean			cached_best_fit_ok_hint;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	Widget			child;
	XtWidgetGeometry	best_fit;
	XtWidgetGeometry	available;
	Dimension 		child_border;


	/* if we have a child, use its size (or requested size) 
         * as our best fit.  Include the child's border_width.
	 */
	child = FetchChild(w, (Boolean *)NULL);


#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (
			"VendorLayout: %s/%s/%x (%d,%d) child %s resizable/%d who_asking/%s query/%d\n",
			XtName(w), CLASS(XtClass(w)), w,
			CORE_P(w).width, CORE_P(w).height,
			child? XtName(child) : "(none)",
			resizable,
			who_asking? XtName(who_asking) : "(none)",
			query_only
		);
	}
#endif

#define COMPUTE(DIM)                             	\
	if (child && who_asking == child){       	\
	    child_border = request->border_width; 	\
	    best_fit.DIM = request->DIM +        	\
		2*child_border;				\
	}else if (child){                         	\
	    child_border = CORE_P(child).border_width;	\
	    best_fit.DIM = CORE_P(child).DIM     	\
		+ 2*child_border;			\
        }else{                                     	\
            child_border = 0;				\
	    best_fit.DIM = CORE_P(w).DIM;		\
	}
			    
	COMPUTE (width);
	COMPUTE (height);
#undef  COMPUTE

	
	if (!best_fit.width)
	    best_fit.width = 1;
	if (!best_fit.height)
	    best_fit.height = 1;

	/*
	 * We only care about our width and height.
	 */
	best_fit.request_mode = CWWidth|CWHeight;
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (" best_fit");
		Debug__DumpGeometry (&best_fit);
		printf ("\n");
	}
#endif

	/* if it is just the Vendor shell asking, return our current
	 * size as the preferred size.
	 */
	if (who_asking == w){
	    if (response){
		*response = best_fit;
	    }
	    return;
	}

	/*
	 * If we can resize, try to resolve any delta between our
	 * current size and the best-fit size by asking our parent
	 * for the new size. If we don't get it all from our parent,
	 * that's life.
	 */
	OlAvailableGeometry (
		w, resizable, query_only, who_asking,
		request, &best_fit, &available
	);
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (" available");
		available.request_mode = CWWidth|CWHeight;
		Debug__DumpGeometry (&available);
		printf ("\n");
	}
#endif

	/* Configure our only managed child as appropriate, placing it at 0,0.
	 * OlConfigureChild will simply fill in the response if the child is querying.
	 */

	if (child){
	    OlConfigureChild(child, 0, 0, available.width - 2*child_border, 
			     available.height - 2*child_border, 
			     child_border, query_only, who_asking, response);
	}

	return;
} /* Layout */


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * TakeFocusEventHandler - This function responds to the window manager's
 * WM_TAKE_FOCUS message by giving focus to the widget registered by the
 * application or to the last widget that had focus.   OlMoveFocus()
 * is used to set the focus so that if the registered widget can not
 * accept focus, the traversal list will be searched for one that can.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
TakeFocusEventHandler OLARGLIST((w, client_data, event, continue_to_dispatch))
	OLARG( Widget,		w)		/* VendorShell Widget	*/
	OLARG( XtPointer,	client_data)	/* OlFocusData		*/
	OLARG( XEvent *,	event)
	OLGRA( Boolean *,	continue_to_dispatch)
{
    if (event->type == ClientMessage &&
        event->xclient.message_type == XA_WM_PROTOCOLS(event->xany.display) &&
        (Atom)event->xclient.data.l[0] == XA_WM_TAKE_FOCUS(event->xany.display))
    {
	Time		time = (Time)event->xclient.data.l[1];

	(void)XtCallAcceptFocus(w, &time);
    }
} /* END OF TakeFocusEventHandler() */

/****************************procedure*header*****************************
 *
 * WMMessageHandler - handles message from the window manager which
 * come in the form of Property or Client Messages, and property changes.
 * Vendor needs to track property changes to keep the resource values in
 * vendor in sync with the window property values, because some old clients
 * may be doing XChangeProperty or calling convenience routines to update
 * decorations directly.
 */
/* ARGSUSED */
static void
WMMessageHandler OLARGLIST((widget, data, xevent, cont_to_dispatch))
        OLARG( register Widget,	widget)
        OLARG( XtPointer,	data)
        OLARG( XEvent *,	xevent)
        OLGRA( Boolean *,	cont_to_dispatch)
{
	OlVendorPartExtension	part = 
			_OlGetVendorPartExtension(_OlGetShellOfWidget(widget));
	Display *		dpy = xevent->xany.display;
	Atom			ol_win_attr,
				ol_decor_add;

		/* Should we handle PropertyDelete???? */
        if (xevent->xany.type == PropertyNotify &&
	    xevent->xproperty.state == PropertyNewValue)
	{
		if (xevent->xproperty.atom == XA_OL_PIN_STATE(dpy)) {
        		long pushpin_state = -1;

                	GetWMPushpinState(
				dpy, XtWindow(widget), &pushpin_state);

                	if (pushpin_state == WMPushpinIsIn)
                	{
				part->pushpin = OL_IN;
                	}
                	else if (pushpin_state == WMPushpinIsOut)
                	{
				part->pushpin = OL_OUT;
                	}
			else
				part->pushpin = OL_NONE;
		}
		else if (xevent->xproperty.atom == XA_OL_WIN_BUSY(dpy)) {
			long busy_state;

			GetWMWindowBusy(dpy, XtWindow(widget),
					&busy_state);

			part->busy = (busy_state == WMWindowIsBusy ?
						TRUE : FALSE);
		}
		else if ((xevent->xproperty.atom ==
				(ol_decor_add = XA_OL_DECOR_ADD(dpy))) ||
		         (xevent->xproperty.atom == XA_OL_DECOR_DEL(dpy))) {
			Atom *		atoms;
			Atom *		save_atoms;
			Boolean 	state;
			int		n;

				/* get the atom list w.r.t.		   */
				/* xproperty.atom not just "_OL_DECOR_ADD" */
			save_atoms = atoms = GetAtomList(
						dpy,
						XtWindow(widget),
						xevent->xproperty.atom,
						&n, False);

			if (atoms) {
				if (xevent->xproperty.atom == ol_decor_add)
					state = True;
				else
					state = False;

				while (n--) {
					if (*atoms == XA_OL_DECOR_CLOSE(dpy))
						part->menu_button = state;
					if (*atoms == XA_OL_DECOR_RESIZE(dpy))
						part->resize_corners = state;
					if (*atoms == XA_OL_DECOR_HEADER(dpy))
						part->window_header = state;
					if (*atoms == XA_OL_DECOR_PIN(dpy)) {
						if (state == False)
							part->pushpin = OL_NONE;
						else if (part->pushpin ==
							 OL_NONE)
							part->pushpin = OL_OUT;
					}
					atoms++;
				}
				XtFree((char *)save_atoms);
			}
		}
		else if (xevent->xproperty.atom ==
				(ol_win_attr = XA_OL_WIN_ATTR(dpy))) {
			Atom		actual_type;
			Boolean		use_OLWinAttr;
			int		actual_format;
			long		long_len;
			unsigned long	nitems,
					bytes_after;
			union {	
				OLWinAttr *	wa;
				OwOLWinAttr *	ow_wa;
			} info;

			OLWinAttr		wa;

			use_OLWinAttr = UseOLWinAttr(widget);

			long_len = use_OLWinAttr ?
					sizeof(OLWinAttr)   / sizeof(long) :
					sizeof(OwOLWinAttr) / sizeof(long);

			if (XGetWindowProperty(
				dpy, XtWindow(widget),
				ol_win_attr, 0L, long_len, False,
				ol_win_attr, &actual_type, &actual_format,
				&nitems, &bytes_after,
				(unsigned char **)&info) == Success)
			{
				if (actual_type != ol_win_attr ||
				    actual_format != 32 ||
				    nitems != long_len ||
				    bytes_after != 0)
				{
					if (info.wa != (OLWinAttr *)NULL)
						XtFree((XtPointer)info.wa);
					return;
				}

					/* "wa" is always used for checking */
				if (use_OLWinAttr == True)
					wa = *info.wa;
				else
				{
#define SET_FIELD(f)	wa.f = info.ow_wa->f
					SET_FIELD(win_type);
					SET_FIELD(menu_type);
					SET_FIELD(pin_state);
#undef SET_FIELD

				}

				XtFree((XtPointer)info.wa);

				if (use_OLWinAttr == False ||
				    (wa.flags & _OL_WA_WIN_TYPE)) {
					struct WinTypeDecor *decor;

#define WIN_ATOM	XInternAtom(dpy, decor->win_atom_name, False)

					for (decor=wintype_decor;
					     decor->win_type != NULL; decor++)
						if (WIN_ATOM == wa.win_type) {
							/* copy std values */
#define COPY(X)		part->X = decor->X
							COPY(win_type);
							COPY(window_header);
							COPY(menu_button);
							COPY(resize_corners);
#undef COPY

#define PIN	decor->pushpin
							part->pushpin =
								PIN == True ?
								OL_OUT :
								OL_NONE;
#undef PIN
							break;
						}
#undef WIN_ATOM

				}

				if (wa.flags & _OL_WA_MENU_TYPE) {
#define TYPE part->menu_type
					if (wa.menu_type ==XA_OL_MENU_FULL(dpy))
						TYPE = OL_MENU_FULL;
					else if (wa.menu_type ==XA_OL_NONE(dpy))
						TYPE = OL_NONE;
					else if (wa.menu_type ==
							XA_OL_MENU_LIMITED(dpy))
					{
						if (use_OLWinAttr &&
						    (wa.flags & _OL_WA_CANCEL)
						    && (wa.cancel))
							TYPE = OL_MENU_CANCEL;
						else
							TYPE = OL_MENU_LIMITED;
					}
#undef TYPE
				}

				if ((use_OLWinAttr == False &&
				     part->pushpin != OL_NONE) ||
				    wa.flags & _OL_WA_PIN_STATE) {
					if (wa.pin_state)
						part->pushpin = OL_IN;
					else
						part->pushpin = OL_OUT;
				}
			}
		}
	}
	else if (xevent->xclient.message_type == XA_WM_PROTOCOLS(dpy)) {
		int msgtype = 0;

		if (xevent->xclient.data.l[0] == XA_WM_SAVE_YOURSELF(dpy)) {
			msgtype = OL_WM_SAVE_YOURSELF;
		}
		else if (xevent->xclient.data.l[0] == XA_WM_DELETE_WINDOW(dpy)){
			msgtype = OL_WM_DELETE_WINDOW;
		}
		else if (xevent->xclient.data.l[0] == XA_WM_TAKE_FOCUS(dpy)) {
			msgtype = OL_WM_TAKE_FOCUS;
		}

		if (msgtype) {
			OlWMProtocolVerify st;
			OlVendorClassExtension	ext = part->class_extension;

			st.msgtype = msgtype;
			st.xevent  = xevent;

			
 			if ((ext->override_callback == FALSE) &&
			    (OlHasCallbacks(widget, XtNwmProtocol) ==
				 XtCallbackHasSome))
				OlCallCallbacks(widget, XtNwmProtocol,
					 (XtPointer)&st);
			else if (ext->wm_proc)
				(*(ext->wm_proc))(widget,OL_DEFAULTACTION,&st);
		}
	}
} /* END OF WMMessageHandler() */




/****************************public*procedures****************************
 *
 * Public Procedures
 *
 */

/*
 *************************************************************************
 * _OlGetDefault - this routine gets the default associated with a shell.
 ****************************procedure*header*****************************
 */
Widget
_OlGetDefault OLARGLIST((w))
	OLGRA( Widget,	w)		/* Any widget or shell	*/
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	Widget		default_widget;
	
	if (part != NULL_PART) {
		OlGetDefaultProc proc = part->class_extension->get_default;

		if (proc != (OlGetDefaultProc)NULL)
		{
			(*proc)(part->vendor, part->default_widget);
		}
		default_widget = part->default_widget;
	} else {
		default_widget = NULL_WIDGET;
	}

	return (default_widget);
} /* END OF _OlGetDefault() */

/*
 *************************************************************************
 * _OlGetFocusData - this routine routines the focus data associated with
 * the supplied widget.
 * If the a vendor extension part pointer is supplied, the part pointer
 * is initialized with the vendor extension part's address.
 ****************************procedure*header*****************************
 */
OlFocusData *
_OlGetFocusData OLARGLIST((w, part_ptr))
	OLARG( Widget,			w)
	OLGRA( OlVendorPartExtension *,	part_ptr)
{
	OlFocusData *	fd = NULL_FOCUS_DATA;

	if (part_ptr != NULL_PART_PTR)
		*part_ptr = NULL_PART;

	if (w == NULL_WIDGET)
	    return(NULL_FOCUS_DATA);

	w = _OlGetShellOfWidget(w);

	if ((w != NULL_WIDGET) && (XtIsVendorShell(w) == True))
	{
		OlVendorPartExtension part = _OlGetVendorPartExtension(w);

		if (part_ptr != NULL_PART_PTR)
			*part_ptr = part;

		if (part != NULL_PART)
			fd = &(part->focus_data);
	}
	return (fd);
} /* END OF _OlGetFocusData() */

/*
 *************************************************************************
 * _OlGetVendorClassExtension - this routine returns the class extension
 * associated with a vendor subclass.  Subclasses of the vendor shell
 * should use this routine to access the vendor's class part.
 ****************************procedure*header*****************************
 */
OlVendorClassExtension
_OlGetVendorClassExtension OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)		/* Vendor subclass	*/
{
	OlVendorClassExtension	ext = NULL_EXTENSION;

	if (!wc) {
		OlVaDisplayWarningMsg(
			(Display *)NULL,
			OleNnullWidget,
			OleTclassPointer,
			OleCOlToolkitWarning,
			OleMnullWidget_classPointer,
			(OLconst char *)"_OlGetVendorClassExtension"
		);
	} else {
		ext = GET_EXT(wc);
	}
	return(ext);
} /* END OF _OlGetVendorClassExtension() */

/*
 *************************************************************************
 * _OlGetVendorPartExtension - this routine returns the part extension
 * data associated with a vendor subclass widget instance.  Subclasses
 * of the vendor shell should use this routine to access the vendor's
 * part.
 ****************************procedure*header*****************************
 */
OlVendorPartExtension
_OlGetVendorPartExtension OLARGLIST((w))
	OLGRA( Widget,	w)			/* Vendor subclass	*/
{
	OlVendorPartExtension	part = NULL_PART;

	if (w == NULL_WIDGET) {
		OlVaDisplayWarningMsg(
			(Display *)NULL,
			OleNnullWidget,
			OleTclassPointer,
			OleCOlToolkitWarning,
			OleMnullWidget_classPointer,
			(OLconst char *)"_OlGetVendorClassExtension"
		);
	} else if (XtIsVendorShell(w) == True) {
		OlVendorClassExtension	extension = GET_EXT(XtClass(w));
		OlVendorPartExtension *	part_ptr = &extension->part_list;

		for (part = *part_ptr; part != NULL_PART && w != part->vendor;)
		{
			part_ptr	= &part->next;
			part		= *part_ptr;
		}
	}
	return(part);
} /* END OF _OlGetVendorPartExtension() */

/****************************procedure*header*****************************
 * _OlLoadVendorShell - this routine is called during the toolkit
 * initialization to force a reference to this file's object file.
 * If at some point in the future loading an entire new vendor class
 * record causes a problem (e.g., masks new routines placed into
 * the Intrinsic's vendor class record by MIT), then we can simply
 * remove our class record and insert our procedures in there
 * corresponding fields.  If we do this, then our procedures will be
 * responsible for calling the replacing Intrinsic's routine.
 *
   Take this opportunity to fix up shell inheritance.  Transient and its
   superclasses should inherit fields from Core but this is not the way
   it is shipped from MIT.
 */
void
_OlLoadVendorShell()
{
#ifdef SHARELIB
   void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit      (*__libXol__XtInherit)
#endif

#ifndef GUI_DEP
		/* if we can't do it here then move them to	*/
		/* OlToolkitInitialize()...			*/
	extern OlDnDDragKeyStatus
			_OlHandleDragKey OL_ARGS((Widget, XEvent *));

	OlDnDVCXInitialize();
	OlDnDRegisterDragKeyProc(_OlHandleDragKey);
#endif

#define ForceInheritance(wc) \
	wc->core_class.accept_focus = XtInheritAcceptFocus; \
	wc->core_class.tm_table = XtInheritTranslations;

    ForceInheritance(transientShellWidgetClass);
    ForceInheritance(topLevelShellWidgetClass);
    ForceInheritance(applicationShellWidgetClass);

#undef ForceInheritance

} /* END OF _OlLoadVendorShell() */

/*
 *************************************************************************
 * _OlSetDefault - this routine sets the default widget on a vendor
 * shell.  Then if the particular shell has a SetDefault procedure it
 * is called with the new default widget id.  If a widget wants tobe the
 * default and there is already a default widget, the old default widget
 * is unset by calling XtSetValues on it.
 * Note: this routine does not check to see if the widget which
 * called this routine has actually set it's default value.
 ****************************procedure*header*****************************
 */
void
_OlSetDefault OLARGLIST((w, wants_to_be_default))
	OLARG( Widget,	w)
    	OLGRA( Boolean,	wants_to_be_default)
{
	Widget			old_default;
	OlVendorClassExtension	ext = NULL_EXTENSION;
	OlVendorPartExtension	part;

	part = _OlGetVendorPartExtension(_OlGetShellOfWidget(w));

	if (part == NULL_PART) {
		return;
	}

	ext		= part->class_extension;
	old_default	= part->default_widget;

	if (wants_to_be_default == False) {

				/* If this widget no longer wants to
				 * be the default and it's currently,
				 * the default, remove it; else, do
				 * nothing.				*/

		if (part->default_widget == w) {
			part->default_widget = NULL_WIDGET;
		}
	} else {

				/* If this widget wants to be the default
				 * and it's not the current default,
				 * set it to be so, else, noop.		*/

		if (part->default_widget != w) {
			part->default_widget = w;

			if (old_default != NULL_WIDGET) {
				Arg	args[1];

				XtSetArg(args[0], XtNdefault, False);
				XtSetValues(old_default, args, 1);
			}
		}
	}

			/* If the shell's default changed, call the class
			 * procedure with the old and new defaults.	*/

	if (part->vendor->core.being_destroyed == False &&
	    part->default_widget != old_default &&
	    ext->set_default != (OlSetDefaultProc)NULL)
	{
		(* ext->set_default) (part->vendor, part->default_widget);
	}
} /* END OF _OlSetDefault() */

/*************************************************************************
 * _OlSetPinState -
 */
void
_OlSetPinState OLARGLIST((w, pin_state))
	OLARG( Widget,		w)
	OLGRA( OlDefine,	pin_state)
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	
	if (part->pushpin != pin_state) {
		part->pushpin = pin_state;
		SetWMAttributes(w, part, False);
	}
} /* end of _OlSetPinState */

/*
 * Add a new shell to the shell list. The shell list is used to keep track
 * of all the shells in an application, so that a dynamic change can be
 * propagated to all the shells.
 */
static void
_OlAddToShellList OLARGLIST((w))
	OLGRA( Widget, w)
{
	/* add to the base shell list */
	if (_OlShell_list_alloc_size == _OlShell_list_size) {
		Widget *new;

		_OlShell_list_alloc_size += SHELL_LIST_STEP;
		new = (Widget *)XtRealloc((char *)_OlShell_list,
			_OlShell_list_alloc_size * sizeof(Widget));
		if (new == NULL) {
			_OlShell_list_alloc_size -= SHELL_LIST_STEP;
			OlVaDisplayErrorMsg(
				XtDisplay(w),
				OleNnoMemory,
				OleTgeneral,
				OleCOlToolkitError,
				OleMnoMemory_general,
				(OLconst char *)"OlRegisterShell"
			);
		}

		_OlShell_list = new;
	}

	_OlShell_list[_OlShell_list_size++] = w;
} /* end of _OlAddToShellList */

static void
_OlDelFromShellList OLARGLIST((w))
	OLGRA( Widget, w)
{
	register int i;
	register Widget *sh;
	
	for (i=0, sh=_OlShell_list; i < _OlShell_list_size; i++, sh++)
		if (*sh == w) {
			int remains = _OlShell_list_size -
					 (int)(sh - _OlShell_list) - 1;

			if (remains)
				(void) memcpy((void *)sh, (void *)(sh+1),
					remains * sizeof(Widget));
			_OlShell_list_size--;
			break;
		}
} /* end of _OlDelFromShellList */

/**
 ** _OlRegisterShell()
 ** _OlUnregisterShell()
 **/

void
_OlRegisterShell OLARGLIST((w))
	OLGRA( Widget, w)
{
	Widget			vsw	= _OlFindVendorShell(w, False);

	Widget *		shells;

	Cardinal		nshells;

	OlVendorPartExtension	pe	= 0;


	if (vsw) {
	    if (pe = _OlGetVendorPartExtension(vsw)) {
		if (pe->shell_list) {
			shells  = pe->shell_list->shells;
			nshells = pe->shell_list->nshells;
		} else {
			shells  = 0;
			nshells = 0;
		}

		/*
		 * Add this new shell to the list of shells on the
		 * top-level, even though this shell might BE the
		 * top-level. This simplifies the processing of shells.
		 */
		nshells++;
		shells = Array(shells, Widget, nshells);
		shells[nshells - 1] = w;

		if (!pe->shell_list) {
			/* first time */
			pe->shell_list = New(OlShellList);
			OlRegisterDynamicCallback(
					_OlNewAcceleratorResourceValues,
					(XtPointer)vsw);

			_OlAddToShellList(vsw);
		}
		pe->shell_list->shells  = shells;
		pe->shell_list->nshells = nshells;

	    } /* if (pe...) */
	} /* if (vsw) */
	else
		/* still should add to shell list */
		_OlAddToShellList(w);
	return;
} /* end of _OlRegisterShell */

void
_OlUnregisterShell OLARGLIST((w))
	OLGRA( Widget, w)
{
	Widget			vsw	= _OlFindVendorShell(w, False);

	Widget *		shells;
	Widget *		p;

	Cardinal		nshells;
	Cardinal		i;

	OlVendorPartExtension	pe	= 0;


	if (vsw) {
	    if (((pe = _OlGetVendorPartExtension(vsw)) != NULL)
		  && pe->shell_list) {
		shells  = pe->shell_list->shells;
		nshells = pe->shell_list->nshells;

		for (i = 0, p = shells; i < nshells; i++, p++)
			if (*p == w)
				break;
		if (i >= nshells)
			return;

		nshells--;

		/*
		 * Note: "nshells" is now the new size of the list (sans
		 * item to be deleted).
		 * "p - shells" equals the number of items before the
		 * item to be deleted. Thus the difference is
		 * the number of items after the one to be deleted.
		 * The following will move the latter items down, to
		 * cover up (delete) the defunct item.
		 */
		if (nshells)
			OlMemMove (Widget, p, p+1, nshells - (p - shells));
		shells = Array(shells, Widget, nshells);

		pe->shell_list->shells  = shells;
		pe->shell_list->nshells = nshells;

		if (OlUnregisterDynamicCallback(_OlNewAcceleratorResourceValues,
			 (XtPointer)vsw)) {
			/*
			 * Found it in the callback list, means it is also
			 * in the OlShell_list. So remove it from
			 * OlShell_list.
			 */
			_OlDelFromShellList(vsw);
		}
	    } /* if (pe...) */
	} /* if (vsw) */
	else
		/* still should remove from shell list */
		_OlDelFromShellList(w);
	return;
} /* end of _OlUnregisterShell */

void
OlWMProtocolAction OLARGLIST((w, st,  action))
	OLARG( Widget,	w)
	OLARG( OlWMProtocolVerify *, st)
	OLGRA( OlDefine, action)
{
	OlVendorPartExtension	part;

	if (XtIsSubclass(w, vendorShellWidgetClass) && 
	    (part = _OlGetVendorPartExtension(_OlGetShellOfWidget(w))) &&
	    (part->class_extension->wm_proc)) {
		(*(part->class_extension->wm_proc))(w, action, st);
	}
} /* end of OlWMProtocolAction  */

/**
 ** _OlFindVendorShell()
 **/

Widget
_OlFindVendorShell OLARGLIST((w, is_mnemonic))
	OLARG( Widget,	w)
	OLGRA( Boolean,	is_mnemonic)
{
	do {
		while (w && !XtIsVendorShell(w))
			w = XtParent(w);
		if (is_mnemonic || !w || !XtParent(w))
			break;
		w = XtParent(w);
	} while (w);

	return (w);
} /* end of _OlFindVendorShell */

void 
OlAddCallback OLARGLIST((widget, name, callback, closure))
	OLARG( Widget,		widget)
	OLARG( String,		name)
	OLARG( XtCallbackProc,	callback)
	OLGRA( XtPointer,	closure)
{
	int wm;

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;
		XtCallbackList *first;

		if (!pe)
			return;

		if (wm)
			first = &(pe->wm_protocol);
		else
			first = &(pe->consume_event);

		if ((cb = *first) == NULL) {
			/* first time */
			cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec)*2);
			if (cb == NULL) {
					/* this probably should be an error */
				OlVaDisplayWarningMsg(
					XtDisplay(widget),
					OleNnoMemory,
					OleTgeneral,
					OleCOlToolkitWarning,
					OleMnoMemory_general,
					(OLconst char *)"OlAddCallback"
				);
				return;
			}

			*first = cb;
			cb->callback = callback;
			cb->closure  = closure;
			(cb+1)->callback = NULL;
		}
		else {
			int i;

			/* count the # of entries */
			for (i=0, cb=*first; cb->callback; i++,cb++);

			cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec) *
				(i+2));
			if (cb == NULL) {
					/* this probably should be an error */
				OlVaDisplayWarningMsg(
					XtDisplay(widget),
					OleNnoMemory,
					OleTgeneral,
					OleCOlToolkitWarning,
					OleMnoMemory_general,
					(OLconst char *)"OlAddCallback"
				);
				return;
			}
			cb->callback = callback;
			cb->closure  = closure;
			(void) memcpy((char *)(cb+1), (char *)(*first),
			       sizeof(XtCallbackRec) * (i+1));
			XtFree((char *)*first); /* free the old list */
			*first = cb;
		}
	}
	else
		XtAddCallback(widget, name, callback, closure);
} /* end of OlAddCallback */

void 
OlRemoveCallback OLARGLIST((widget, name, callback, closure))
	OLARG( Widget,		widget)
	OLARG( String,		name)
	OLARG( XtCallbackProc,	callback)
	OLGRA( XtPointer,	closure)
{
	int wm;

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;
		XtCallbackList *first;

		if (!pe)
			return;
		if (wm) {
			cb = pe->wm_protocol;
			first = &(pe->wm_protocol);
		}
		else {
			cb = pe->consume_event;
			first = &(pe->consume_event);
		}
		if (!cb)
			return;
		if ((callback == NULL) || (cb == NULL))
			return;

		for (; cb->callback; cb++)
			if ((cb->callback == callback) &&
			    (cb->closure  == closure)) {
				/*
				 * Found it!
				 * just shift up the rest of the list.
				 * don't bother shrinking the space.
				 */
				do {
					*cb = *(cb+1);
					cb++;
				} while (cb->callback);

				/* if an empty list */
				if (cb == *first) {
					XtFree((char *)*first);
					*first = NULL;
				}
				break;
			}
	}
	else
		XtRemoveCallback(widget, name, callback, closure);
} /* end of OlRemoveCallback */

XtCallbackStatus 
OlHasCallbacks OLARGLIST((widget, callback_name))
	OLARG( Widget, widget)
	OLGRA( OLconst char *, callback_name)
{
	int wm;

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(callback_name, XtNwmProtocol)) != 0) ||
	     !strcmp(callback_name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;

		if (!pe)
			return(XtCallbackNoList);
		if (wm)
			cb = pe->wm_protocol;
		else
			cb = pe->consume_event;
		if (cb)
			return(XtCallbackHasSome);
		else
			return(XtCallbackHasNone);
	}
	else
		return(XtHasCallbacks(widget, callback_name));
} /* end of OlHasCallbacks */

void 
OlCallCallbacks OLARGLIST((widget, name, call_data))
	OLARG( Widget,		widget)
	OLARG( OLconst char *,	name)
	OLGRA( XtPointer,	call_data)
{
	int wm;

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;

		if (!pe)
			return;
		if (wm)
			cb = pe->wm_protocol;
		else
			cb = pe->consume_event;
		if (!cb)
			return;

		for (; cb->callback; cb++)
			(*(cb->callback))(widget, cb->closure, call_data);
		
	}
	else
		XtCallCallbacks(widget, name, call_data);
} /* end of OlCallCallbacks */

#ifdef	sunOWV3	/*OWV3*/

/*
 *************************************************************************
 * _OlFixResourceList - this function moves the visual 
 * resource in front of the colormap resource in the 
 * widget's resource list.  This is done so the visual 
 * resource get resolved before the colormap resource.
 ****************************procedure*header*****************************
 */
void
_OlFixResourceList OLARGLIST((wc))
	OLGRA( WidgetClass, wc)
{
	Cardinal num_res = wc->core_class.num_resources;
	XrmQuark name;
	XrmResourceList	*new_res, *nres, *visual_pos,
		*res = (XrmResourceList *)
		wc->core_class.resources;
	int i = 0, gotit = 0;

	if (num_res > 0) {
		/* allocate new resource list */
		new_res = nres = (XrmResourceList *)
			XtMalloc(num_res *
			sizeof(XrmResourceList));

		/*
		 * find colormap resource
		 * and move up one position
		 */
		name = XrmStringToName(XtNcolormap);
		while (i < num_res) {
			if (name == (*res)->xrm_name) {
				visual_pos = new_res++;
				*new_res++ = *res++;
				i++;
				break;
			}
			*new_res++ = *res++;
			i++;
		}

		/*
		 * find visual resource and
		 * move up in front of colormap
		 */
		name = XrmStringToName(XtNvisual);
		while (i < num_res) {
			if (name == (*res)->xrm_name) {
				*visual_pos = *res++;
				i++;
				gotit = 1;
				break;
			}
			*new_res++ = *res++;
			i++;
		}

		if (gotit) {
			/* copy the remaining */
			OlMemMove(XrmResourceList,
					(char *)new_res, (char *)res,
					num_res - i);

			/* free the old list */
			XtFree((char  *)
				(wc->core_class.resources));

			/* replace with the new list */
			wc->core_class.resources = 
				(XtResourceList)nres;
		} else
			XtFree(nres);
	}
}

/*
 *************************************************************************
 * _OlCopyParentsVisual - 
 ****************************procedure*header*****************************
 */
void
_OlCopyParentsVisual OLARGLIST((widget, closure, value))
	OLARG( Widget,		widget)
	OLARG( int,		closure)
	OLGRA( XrmValue *,	value)
{
	static Visual *visual;

	if (XtParent(widget) == NULL)
		visual = DefaultVisualOfScreen (widget->core.screen);
	else
		visual = OlVisualOfObject(XtParent(widget));

	value->addr = (caddr_t) &visual;
}

/*
 *************************************************************************
 * _OlSetupColormap - 
 ****************************procedure*header*****************************
 */
void
_OlSetupColormap OLARGLIST((widget, closure, value))
	OLARG( Widget,		widget)
	OLARG( int,		closure)
	OLGRA( XrmValue *,	value)
{
	Screen *screen = XtScreenOfObject(widget);
	Visual *visual;
	static Colormap cm;
	XrmValue from, to;

	if (XtIsShell(widget))
		 visual=((ShellWidget)widget)->shell.visual;
	else if (XtIsSubclass(widget, drawAreaWidgetClass))
		 visual=((DrawAreaWidget)widget)->draw_area.visual;
	else
	{
		OlVaDisplayErrorMsg(
			XtDisplay(w),
			OleNbadVisual,
			OleTgeneral,
			OleCOlToolkitError,
			OleMbadVisual_noVisual,
				/* required for OleMbadVisual_noVisual */
			XtName(widget),
			(OLconst char *)"_OlSetupColormap"
		);
	}

	if (visual == CopyFromParent)
	{
		OlVaDisplayErrorMsg(
			XtDisplay(w),
			OleNbadVisual,
			OleTgeneral,
			OleCOlToolkitError,
			OleMbadVisual_copyFromParent,
			(OLconst char *)"_OlSetupColormap"
		);
	}

	/*
	 * Call the Visual to Colormap converter
	 * to create a colormap
	 */
	from.size = sizeof(Visual **);
	from.addr = (caddr_t) &visual;

	to.size = sizeof(Colormap);
	to.addr = (caddr_t) &cm;

	if (XtConvertAndStore(widget, XtRVisual, &from,
		XtRColormap, &to) == False)
		OlVaDisplayErrorMsg(
			XtDisplay(w),
			OleNbadConversion,
			OleTgeneral,
			OleCOlToolkitError,
			OleMbadConversion_invalidVisual,
			(OLconst char *)"_OlSetupColormap"
		);

	value->addr = (caddr_t) &cm;
}

#endif	/*sun*/	/*OWV3*/

/*
 *************************************************************************
 * _OlRegisterIc - register an input context (IC) associated with w with its
 * shell.  The shell is responsible for notifying the IC of the geometry
 * of the status window.  All IC's under a single shell will share a 
 * single status window.  The height(width) of the status window will be determined
 * by the largest height(width) passed to OlRegisterIc.  It is assumed
 * that w is realized and that height is non-zero.  (Height should be
 * based on the font being used by w.) Width may be 0 if w does not have 
 * a suggested width for its status area.
 ****************************procedure*header*****************************
 */
extern void
_OlRegisterIc OLARGLIST((w, ic, width, height))
    OLARG(Widget,	w)
    OLARG(OlIc *,	ic)
    OLARG(unsigned short, width)
    OLGRA(unsigned short, height)
{
    Widget               shell = _OlGetShellOfWidget(w);
    OlVendorPartExtension part = _OlGetVendorPartExtension(shell);
    unsigned short  max_height = height;
    unsigned short   max_width = width;
    XtWidgetGeometry    status;
    OlIcGeometry   ic_geometry;
    int                  index;
    XRectangle     status_rect;
    OlIcValues    ic_values[2];


#ifdef STATUS_DEBUG
    fprintf(stderr, "OlRegisterIc:");
#endif

    /* get the current geometry of the status area for the shell */
    status.request_mode = 0;
    XtVaGetValues(shell,
		  XtNstatusAreaGeometry, &status,
		  NULL);

#ifdef STATUS_DEBUG	
    fprintf(stderr, "Current statusAre[x,y,w,h] = [%d,%d,%d,%d]\n",
	    status.x, status.y, status.width, status.height);
#endif

    /* Calculate the width and height.  Assume that the widget 
     * has specified a valid height. If no width is specified, use
     * the previously specified status area width, or, failing that,
     * the width of the shell.  In the final case, all subsequent IC's 
     * will use the entire width of the shell because we take the 
     * maximum of all IC widths.
     */
    if ((status.request_mode & CWWidth) && status.width > max_width)
	max_width = status.width;
    if (!max_width)
	max_width = shell->core.width;
    if ((status.request_mode & CWHeight) && status.height > max_height)
	max_height = status.height;

    /* Add the IC to the list for this shell.
     * A widget should only register it's IC once,
     * but we'll be nice and check if this IC has already been
     * registered and just update its width and height in that case.
     */
    ic_geometry.ic = ic;
    ic_geometry.width = max_width;
    ic_geometry.height = max_height;
    if ((index = _OlArrayFind(&part->ic_list, ic_geometry)) == _OL_NULL_ARRAY_INDEX)
	_OlArrayOrderedInsert(&part->ic_list, ic_geometry);
    else{
	_OlArrayElement(&part->ic_list,index).width = max_width;
	_OlArrayElement(&part->ic_list,index).height = max_height;
    }

#define ALL_BITS (CWWidth|CWHeight|CWX|CWY)

    if (((status.request_mode & ALL_BITS) != ALL_BITS) ||
	(max_width != status.width) || (max_height != status.height)){
	/* This IC has caused a change in the status area geometry,
	 * or the X and Y positions are not yet set.
	 * Update the width and height of the status area.
	 * The Vendor SetValues will call OlSetIcValues for each of
	 * its IC's to notify the input method of a geometry change.
	 * Then force a layout to take account for the new status area geomtry.
	 */
	status.request_mode = CWWidth|CWHeight;
	status.width        = max_width;
	status.height       = max_height;
	XtVaSetValues(shell,
		      XtNstatusAreaGeometry, &status,
		      NULL);
	OlLayoutWidget(shell, True, False, False, (Widget) NULL, NULL, NULL);
    }
    else{	
	/* The current status area geometry is correct.
	 * Simply pass it to this IC.
	 */

	status_rect.x = status.x;
	status_rect.y = status.y;
	status_rect.width = status.width;
	status_rect.height = status.height;
	ic_values[0].attr_name = OlNstatusArea;
	ic_values[0].attr_value = (void *)&status_rect;
	ic_values[1].attr_name = NULL;
	ic_values[1].attr_value = NULL;
	OlSetIcValues(ic, ic_values);
#ifdef STATUS_DEBUG
	fprintf(stderr,"_OlRegisterIc: geometry unchanged, calling OlSetIcValues\n");
#endif
    }
#undef ALL_BITS
} /* end of _OlRegisterIc */

/*
 *************************************************************************
 * _OlUnregisterIc - remove an input context (IC) associated with w from
 * the IC list of its shell.  Each registered IC specifies a width and
 * height for the status area geometry.  Since the status area is shared
 * we use the maximum specified value.  When removing an IC, we need to 
 * check if it was the largest in either width or height and update
 * the statusAreaGeometry if so.  Also, we need to check if this was the
 * last IC removed.
 ****************************procedure*header*****************************
 */
extern void
_OlUnregisterIc OLARGLIST((w, ic))
    OLARG(Widget,	w)
    OLGRA(OlIc *,	ic)
{
    Widget                shell = _OlGetShellOfWidget(w);
    OlVendorPartExtension  part = _OlGetVendorPartExtension(shell);
    unsigned short   new_height = 0;  
    unsigned short    new_width = 0;  
    XtWidgetGeometry     status;
    OlIcGeometry bogus_geometry;
    int                   index;
    int              array_size;


#ifdef STATUS_DEBUG
    fprintf(stderr, "OlUnregisterIc:");
#endif

    /* get the current geometry of the status area for the shell */
    status.request_mode = 0;
    XtVaGetValues(shell,
		  XtNstatusAreaGeometry, &status,
		  NULL);

#ifdef STATUS_DEBUG	
    fprintf(stderr, "Current statusAre[x,y,w,h] = [%d,%d,%d,%d]\n",
	    status.x, status.y, status.width, status.height);
#endif

    /* Remove the IC from the list before calculating the new status 
     * geometry 
     */
    bogus_geometry.ic = ic;
    if ((index = _OlArrayFind(&part->ic_list, bogus_geometry)) != _OL_NULL_ARRAY_INDEX)
	_OlArrayDelete(&part->ic_list, index);
#ifdef STATUS_DEBUG
    else
	fprintf(stderr, "OlUnregisterIc: IC not previously registered!\n");
#endif

    /* Calculate the max width and height from the remaining ICs, if any.
     * Update the statusAreaGeometry if it has changed.
     */
    array_size = _OlArraySize(&part->ic_list);
    if (!array_size){
	status.request_mode = CWWidth | CWHeight;
	status.width = 0;
	status.height = 0;
    }
    else{
	for (index = 0; index < array_size; index++){
	    if (_OlArrayElement(&part->ic_list,index).width > new_width)
		new_width = _OlArrayElement(&part->ic_list,index).width;
	    if (_OlArrayElement(&part->ic_list,index).height > new_height)
		new_height = _OlArrayElement(&part->ic_list,index).height;
	    
	}
	status.request_mode = 0;
	if (new_width != status.width){
	    status.request_mode |= CWWidth;
	    status.width = new_width;
	}
	if (new_height != status.height){
	    status.request_mode |= CWHeight;
	    status.height = new_height;
	}
    }
    /*
     * If removing this IC affects the status geometry
     * update XtNstatusAreaGeometry and layout the shell 
     * (with its new status area geometry.)
     */
    if (status.request_mode){
	XtVaSetValues(shell,
		      XtNstatusAreaGeometry, &status,
		      NULL);
	OlLayoutWidget(shell, True, False, False, (Widget) NULL, NULL, NULL);
    }

} /* end of _OlUnregisterIc */
