#ifndef	NOIDENT
#ident	"@(#)olmisc:OlCommon.c	1.79"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		This file contains routines that are common to
 *		OPEN LOOK (TM - AT&T) widgets.
 * 
 ****************************file*header**********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookI.h>
#include <Xol/VendorI.h>
#include <Xol/LayoutExtP.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/Notice.h>
#include <Xol/Stub.h>
#include <Xol/PopupMenu.h>

#if defined(SVR4_0) || defined(SVR4) || defined(sun)
#include <dlfcn.h>
#endif	/* SVR4 */

#if OlNeedFunctionPrototypes

#include <stdlib.h>		/* for getenv()	*/

#include <stdarg.h>
#define VA_START(a,n)	va_start(a,n)

#else	/* OlNeedFunctionPrototypes */

extern char *	getenv();

#include <varargs.h>
#define VA_START(a,n)	va_start(a)

#endif	/* OlNeedFunctionPrototypes */

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

WidgetClass	_OlClass OL_ARGS((Widget));
void		_OlClearWidget OL_ARGS((Widget, Boolean));
String		_OlGetApplicationTitle OL_ARGS((Widget));	/* to retrieve appl title	*/
Widget		_OlGetMenubarWidget OL_ARGS((Widget));
void		_OlGetRefNameOrWidget OL_ARGS((Widget, ArgList, Cardinal *));
Widget		_OlGetShellOfWidget OL_ARGS((register Widget));	/* Find's a widget's shell id	*/
Widget		_OlGetTrollWidget OL_NO_ARGS();		/* returns the Troll Widget	*/

void		_OlResolveGUISymbol OL_ARGS((char *, void **, ...));

void		_OlSetApplicationTitle OL_ARGS((char *));	/* to store appl title */
void		_OlSetMenubarWidget OL_ARGS((Widget, Boolean));
void		_OlSetWMProtocol OL_ARGS((Display *, Window, Atom));

					/* public procedures		*/

String	OlFindHelpFile OL_ARGS((Widget, OLconst char *));	/* Help convenience */
OlDefine	OlGetGui OL_NO_ARGS();	/* Get Gui switch convenience	*/
void	OlSetGui OL_ARGS((OlDefine));	/* Set Gui switch convenience	*/
Widget	OlitInitialize OL_ARGS((OLconst char *, OLconst char*, XrmOptionDescRec *,
				Cardinal, int *,
				char *[]));/* Initialization convenience */
void	OlPreInitialize OL_ARGS((OLconst char *, XrmOptionDescRec *, Cardinal, int *,
				 char *[]));	/* Pre-initialization	*/
void	OlPostInitialize OL_ARGS((OLconst char *, XrmOptionDescRec *, Cardinal,
				  int *, char *[]));	/* Post-initialization		*/
void	OlToolkitInitialize OL_ARGS ((int *, char **, XtPointer));

#ifdef we_dont_want_collisions_with_our_macros

String	OlWidgetClassToClassName OL_ARGS((WidgetClass));
String	OlWidgetToClassName OL_ARGS((Widget));

#endif

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

						/* static variables	*/
static char *	OlApplicationTitle = (char *)NULL;
static OlDefine	OlGuiMode = OL_MOTIF_GUI;

						/* global variables	*/
Widget		OlApplicationWidget = (Widget)NULL;
XrmName		_OlApplicationName = 0;
Display *	toplevelDisplay = (Display *)NULL;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _OlClass - returns one of the Open Look fundamental class types
 ****************************procedure*header*****************************
 */
WidgetClass
_OlClass OLARGLIST((w))
    OLGRA( Widget,	w)
{
    WidgetClass wc;

    if (XtIsVendorShell(w))
	return ( (WidgetClass)vendorShellWidgetClass );

    for (wc = XtClass(w); wc; wc = wc->core_class.superclass)
	if ((wc == primitiveWidgetClass) ||
	    (wc == eventObjClass) ||
	    (wc == managerWidgetClass))
	    break;

    return (wc);
}

/*
 *************************************************************************
 * _OlClearWidget
 ****************************procedure*header*****************************
 */
void
_OlClearWidget OLARGLIST((w, exposures))
	OLARG( Widget,	w)
	OLGRA( Boolean,	exposures)
{
	if (w != (Widget)NULL && XtIsRealized(w) == True)
	{
		if (_OlIsGadget(w) == True)
		{
			XClearArea(XtDisplayOfObject(w),
				XtWindowOfObject(w),
				(int)w->core.x, (int)w->core.y,
				(unsigned int)w->core.width,
				(unsigned int)w->core.height,
				(Bool)exposures);
		}
		else
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0,0,0,0, (Bool)exposures);
		}
	}
} /* END OF _OlClearWidget() */

/*
 * OlGetApplicationResources
 *
 * The \fIOlGetApplicationResources\fR procedure is used to
 * retrieve application resources. It currently is an indirect
 * interface to XtGetApplication Resources.
 *
 * OlRegisterDynamicCallback(3), OlUnregisterDynamicCallback(3)
 * OlCallDynamicCallbacks(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 * ...
 */
void
OlGetApplicationResources OLARGLIST((w, base, resources, num_resources, args,
					num_args))
	OLARG( Widget,		w)
	OLARG( XtPointer,	base)
	OLARG( XtResource *,	resources)
	OLARG( int,		num_resources)
	OLARG( ArgList,		args)
	OLGRA( Cardinal,	num_args)
{
	XtGetApplicationResources(w, base, resources, num_resources,
                                        args, num_args);
} /* END OF OlGetApplicationResources */

/*
 *************************************************************************
 * _OlGetApplicationTitle - retrieves a saved title
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
String
_OlGetApplicationTitle OLARGLIST((w))
    OLGRA( Widget,	w)
{
#ifdef R3_FIX
    Widget shell;
    Arg arg[1];

    if (OlApplicationTitle != NULL)
	return (OlApplicationTitle);

    shell = _OlGetShellOfWidget(w);

    if (XtIsSubclass(shell, applicationShellWidgetClass)) {
	String argv[];

	XtSetArg(arg[0], XtNargv, &argv);
	XtGetValues(shell, arg, XtNumber(arg));
	return ((argv) ? argv[0] : NULL);

    } else if (XtIsSubclass(shell, wmShellWidgetClass)) {
	String title;

	XtSetArg(arg[0], XtNtitle, &title);
	XtGetValues(shell, arg, XtNumber(arg));
	return (title);

    } else
	return (NULL);

#else /* R3_FIX */
    return ((OlApplicationTitle == NULL) ?
	XrmQuarkToString(_OlApplicationName) : OlApplicationTitle);
#endif /* R3_FIX */
} /* END OF _OlGetApplicationTitle */

/*
 *************************************************************************
 * _OlGetMenubarWidget - this routine simply returns the 1 widget in the
 * application that has menubar_behavior set to true, if any.
 ****************************procedure*header*****************************
 */
Widget
_OlGetMenubarWidget OLARGLIST((w))
	OLGRA( Widget,	w)		/* Any widget or shell	*/
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	Widget		menubar_widget = (Widget)NULL;
	
	if (part != (OlVendorPartExtension)NULL)
	{
		menubar_widget = part->menubar_widget;
	}

	return (menubar_widget);
} /* END OF _OlGetMenubarWidget() */

/*
 *************************************************************************
 * _OlGetRefNameOrWidget - this routine will be called by GetValuesHook()
 *	from Primitive, EventObj, and Manager. The purpose of this routine
 *	is to get XtNreferenceName and/or XtNreferenceWidget.
 ****************************procedure*header*****************************
 */
void
_OlGetRefNameOrWidget OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
#define IS_TRAVERSALABLE(w, flag)					     \
	{								     \
		WidgetClass	wc_special = _OlClass(w);		     \
		if (wc_special == primitiveWidgetClass)			     \
			flag = ((PrimitiveWidget)w)->primitive.traversal_on; \
		else if (wc_special == eventObjClass)			     \
			flag = ((EventObj)w)->event.traversal_on;	     \
		else if (wc_special == managerWidgetClass)		     \
			flag = ((ManagerWidget)w)->manager.traversal_on;     \
		else							     \
			flag = False;					     \
	}

	int		i,
			ref_res[2],
			wanted_res,
			pos;
	Boolean		traversalable;
	Widget		shell = _OlGetShellOfWidget(w),
			ref_widget;
	OlFocusData *	fd = _OlGetFocusData(shell, NULL);
	WidgetArray *	list;

	IS_TRAVERSALABLE(w, traversalable);
	if (traversalable == False)
		return;
	ref_res[0] = ref_res[1] = -1;
	wanted_res = 0;
	for (i = 0; i < *num_args; i++)
	{
		if (strcmp(args[i].name, XtNreferenceName) == 0 ||
		    strcmp(args[i].name, XtNreferenceWidget) == 0)
			ref_res[wanted_res++] = i;
	}
	if (wanted_res == 0)	/* didn't catch any */
		return;
	if (fd == NULL)
		return;

	list = &(fd->traversal_list);
	if ((pos = _OlArrayFind(list, w)) == _OL_NULL_ARRAY_INDEX)
		return;

	if (pos == _OlArraySize(list) - 1)	/* last item */
		return;

	for (i = pos+1; i < _OlArraySize(list); i++)
	{
		Boolean		traversalable;

		ref_widget = _OlArrayElement(list, i);
		IS_TRAVERSALABLE(ref_widget, traversalable);
		if (traversalable == True)
			break;

		ref_widget = NULL;	/* set to NULL and keep looking */
	}
	if (ref_widget == NULL)
		return;

	for (i = 0; i < wanted_res; i++)
	{
		if (strcmp(args[i].name, XtNreferenceName) == 0)
		{
			char ** data = (char **)(args[i].value);

			*data = XtName(ref_widget);
		}
		else		/* must be XtNreferenceWidget */
		{
			Widget * data = (Widget *)(args[i].value);

			*data = ref_widget;
		}
	}
#undef IS_TRAVERSALABLE
} /* END OF _OlGetRefNameOrWidget() */

/*
 *************************************************************************
 * _OlGetShellOfWidget - this routine starts at the given widget looking to
 * see if the widget is a shell widget.  If it is not, it searches up the
 * widget tree until it finds a shell or until a NULL widget is
 * encountered.  The procedure returns either the located shell widget or
 * NULL.
 ****************************procedure*header*****************************
 */
Widget
_OlGetShellOfWidget OLARGLIST((w))
	OLGRA( register Widget,	w)		/* Widget to begin search at	*/
{
	while(w != (Widget) NULL && !XtIsShell(w))
		w = w->core.parent;
	return(w);
} /* END OF _OlGetShellOfWidget() */

/*
 *************************************************************************
 * _OlGetTrollWidget - returns the Troll Widget's id.  Application's
 * should never attempt to destroy the troll widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
Widget
_OlGetTrollWidget OL_NO_ARGS()
{
    static Widget troll = (Widget) NULL;

    if (troll == (Widget) NULL) {
	Widget		shell;
	Arg		args[2];
	XtTranslations	empty_translations;

	/* this prevents the shell from being mapped (when realized) */
	XtSetArg(args[0], XtNmappedWhenManaged, FALSE);

		/* Is this OK that we use "troll_shell" as class name?	*/
		/* If not then we need to use _XtGetPerDisplay, See	*/
		/* Xt/Create.c:XtCreateApplicationShell...		*/
	shell = XtAppCreateShell(
			(OLconst char *)"troll_shell", (OLconst char *)"troll_shell", shellWidgetClass,
			toplevelDisplay, args, 1);

		/* Create the troll widget */
	empty_translations = XtParseTranslationTable((OLconst char *)"");
	XtSetArg(args[0], XtNwindow, RootWindowOfScreen(XtScreen(shell)));
		/* we don't want "troll" inherit translation */
	XtSetArg(args[1], XtNtranslations, empty_translations);
	troll = XtCreateManagedWidget((OLconst char *)"troll", stubWidgetClass,
				      shell, args, 2);
	_OlDeleteDescendant(troll);
	XtRealizeWidget(shell);
    }

    return(troll);
} /* END OF _OlGetTrollWidget() */

/*
 *************************************************************************
 * _OlSetApplicationTitle - saves a title for later use
 ****************************procedure*header*****************************
 */
void
_OlSetApplicationTitle OLARGLIST((title))
	OLGRA( char *,	title)
{
	if (OlApplicationTitle != NULL)
		XtFree(OlApplicationTitle);

	OlApplicationTitle = (title == NULL) ? 
			NULL : strcpy(XtMalloc(strlen(title) + 1), title);

} /* END OF _OlSetApplicationTitle */

/*
 *************************************************************************
 * _OlSetMenubarWidget - this routine simply sets or unsets the
 * menubar_widget of the application's vendorShell.
 ****************************procedure*header*****************************
 */
void
_OlSetMenubarWidget OLARGLIST((w, want_to_set))
	OLARG( Widget,		w)		/* Any widget or shell	*/
	OLGRA( Boolean,		want_to_set)
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	
	if (part != (OlVendorPartExtension)NULL)
	{
		if (part->menubar_widget != (Widget)NULL && want_to_set == True)
		{
			Arg	arg[1];

			XtSetArg(arg[0], XtNmenubarBehavior, False);
			XtSetValues(part->menubar_widget, arg, 1);
		}
		if (want_to_set == True)
			part->menubar_widget = w;
		else
			part->menubar_widget = (Widget)NULL;
	}
} /* END OF _OlSetMenubarWidget() */

/****************************************************************************
 * _OlSetWMProtocol- 
 */
void
_OlSetWMProtocol OLARGLIST((display, window, property))
	OLARG( Display *,	display)
	OLARG( Window,		window)
	OLGRA( Atom,		property)
{
	Atom *		atoms;
	int		num_atoms;
	int		i;

	atoms = GetAtomList(
		display, window, XA_WM_PROTOCOLS(display), &num_atoms, False);

	if ((atoms != NULL) && (num_atoms > 0))
		for (i = 0; i < num_atoms; i++)
			if (atoms[i] == property)
			{
				free(atoms);
				return;		/* property already set */
			}

	XChangeProperty(display, window, XA_WM_PROTOCOLS(display), XA_ATOM,
			32, PropModeAppend, (unsigned char *) &property, 1);
	if (atoms)
		free(atoms);
}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlFindHelpFile - this is a convenience routine used by applications to 
 * get the appropriate help file for an appliation
 ****************************procedure*header*****************************
 */
String
OlFindHelpFile OLARGLIST((w, filename))
        OLARG( Widget,	w)		/* widget id 	*/
        OLGRA( OLconst char *,	filename)	/* file to find */
{
  char *ret, *directory_ret;
  if (filename) {
    directory_ret = (char *)_OlGetHelpDirectory(w);
    ret = XtMalloc(strlen(directory_ret) + strlen(filename) + 2);
    sprintf(ret, (OLconst char *)"%s/%s", directory_ret, filename);
    return (String)ret;
  }
  else return (String)NULL;
}

/*
 *************************************************************************
 * OlGetGui - simply returns the OlGuiMode value
 ****************************procedure*header*****************************
 */

OlDefine
OlGetGui OL_NO_ARGS()
{
  return OlGuiMode;
}

/*
 *************************************************************************
 * OlInitialize - this is a convenience routine used by applications to 
 * initialize the Xt (TM) and OPEN LOOK (TM) toolkits
 ****************************procedure*header*****************************
 */
Widget
OlInitialize OLARGLIST((shell_name, classname, urlist, num_urs, argc, argv))
	OLARG( OLconst char *,	shell_name) /* initial shell instance name */
	OLARG( OLconst char *,	classname)  /* application class	   */
	OLARG( XrmOptionDescRec *, urlist)
	OLARG( Cardinal,	num_urs)
	OLARG( int *,		argc)
	OLGRA( char *,		argv[])
{
	if (OlApplicationWidget != (Widget)NULL)
	{
		OlVaDisplayWarningMsg(
			(Display *)NULL,
			OleNOlInitialize,
			OleTbadInitialize,
			OleCOlToolkitWarning,
			OleMOlInitialize_badInitialize,
			(OLconst char *)"OlInitialize"
		);
	}
	else
	{
		/* OlPreInitialize will be called from OlToolkitInitialize */

		OlToolkitInitialize(argc, argv, (XtPointer)NULL);

		/* OlRegisterConverters will called from Vendor:ClassInit. */

		/* OlMidInitialize is gone because it == XtInitialize...   */
		(void)XtInitialize(
			shell_name, classname, urlist, num_urs, argc, argv);

		/* OlPostInitialize will be called from Vendor.c:Initialize*/
	}

		/* OlApplicationWidget and toplevelDisplay are defined in  */
		/* Vendor.c:Initialize()... Since XtInitialize will create */
		/* a toplevel widget which is a subclass of Vendor. You are*/
		/* guaranteed that these two GLOBALs will be initialized.. */
	return(OlApplicationWidget);
} /* END OF OlInitialize() */

/*
 *************************************************************************
 * OlPreInitialize - this routines initializes the OPEN LOOK (TM) parts
 * that must be initialized prior to the Xt (TM) toolkit.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlPreInitialize OLARGLIST ((classname, urlist, num_urs, argc, argv))
	OLARG( OLconst char *,		classname)
	OLARG( XrmOptionDescRec *,urlist)
	OLARG( Cardinal,	num_urs)
	OLARG( int *,		argc)
	OLGRA( char *,		argv[])
{
	static Boolean initialized = False;

	if (initialized == True) {
		_OlApplicationName = XrmStringToName( argv[0] );

		OlVaDisplayWarningMsg(	(Display *) NULL,
					OleNOlInitialize,
					OleTbadInitialize,
					OleCOlToolkitWarning,
					OleMOlInitialize_badInitialize,
					(OLconst char *)"OlPreInitialize");

		return;
	}

	_OlApplicationName = XrmStringToName( argv[0] );

	initialized = True;

				/* Force a reference to the vendor object
				 * file.				*/
	_OlLoadVendorShell();

#define RESIZE_METHOD	((VendorShellClassRec *)vendorShellWidgetClass)->\
				core_class.resize

	if (RESIZE_METHOD != _OlDefaultResize)
	{
		OlVaDisplayErrorMsg(
			(Display *) NULL,
			OleNOlInitialize,
			OleTbadInitialize,
			OleCOlToolkitError,
			OleMOlInitialize_badLinkOrder
		);
	}
#undef RESIZE_METHOD

#ifdef SHARELIB
	/*
	 * This function fixes all the external references that were
	 * supposed to get resolved at compile time, but couldn't because
	 * of static shared library.
	 */
	_OlSharedLibFixup();
#endif

		/* pre-load the quark table */
	_OlLoadQuarkTable();

	_OlInitializeLayoutCoreClassExtension ();
	_OlInitializeHandlesCoreClassExtension ();

	return;
} /* END OF OlPreInitialize() */

/*
 *************************************************************************
 * OlPostInitialize - this routines initializes the OPEN LOOK (TM) parts
 * that must be initialized after the OPEN LOOK "pre-initialize"
 * procedure and after the Xt (TM) toolkit has been initialized.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlPostInitialize OLARGLIST((classname, urlist, num_urs, argc, argv))
	OLARG( OLconst char *,		classname)
	OLARG( XrmOptionDescRec *, urlist)
	OLARG( Cardinal,	num_urs)
	OLARG( int *,		argc)
	OLGRA( char *,		argv[])
{
	static Boolean initialized = False;

	Widget	troll;		/* widget needed for InitVirtual... (R3) */
	String	lang;


	if (initialized == True) {

		OlVaDisplayWarningMsg(	(Display *) NULL,
					OleNOlInitialize,
					OleTbadInitialize,
					OleCOlToolkitWarning,
					OleMOlInitialize_badInitialize,
					(OLconst char *)"OlPostInitialize");
		return;
	}

	initialized = True;

	_XmColorObjCreate( OlApplicationWidget, NULL, 0 );

				/* Get the application's attributes	*/

	_OlInitAttributes(OlApplicationWidget);

		/* xnllanguage would have been set by now, so reset $LANG
		 * so that both Xt and Xol are in sync.
		 *
		 * No need to append a & before lang...
		 *
		 * 6 = "LANG" + "=" + "NULL terminated".
		 *
		 * I removed putenv(LANG=C) from the top of OlInitialize
		 * because it's redundant comparing to what being done below.
		 */
	lang = (char *)XtMalloc(6 + strlen(ol_app_attributes.xnllanguage));
	sprintf(lang, (OLconst char *)"LANG=%s", ol_app_attributes.xnllanguage);
	putenv(lang);

	InitializeOpenLook(XtDisplay(OlApplicationWidget));

		/* Guarantee that we're the first to widgetize the root
		 * window						*/

	troll = _OlGetTrollWidget();

	_OlInitDynamicHandler(troll);

	/*  For each screen of the display, read the toolkit defaults
	    file into the screen database.  */
	_OlCombineToolkitDB(OlApplicationWidget);

} /* END OF OlPostInitialize() */

/*
 * OlReplayBtnEvent
 *
 * The \fIOlReplayBtnEvent\fR procedure is used to replay a button press
 * event to the next window (towards the root) that is interested in
 * button events.  This provides a means of propagating events up
 * a window tree.
 *
 * See also:
 *
 * LookupOlInputEvent(3)
 *
 * Synopsis:
 *
 * ...
 */
void
OlReplayBtnEvent OLARGLIST((w, client_data, event))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLGRA( XEvent *,	event)
{

	Display *		dpy = XtDisplayOfObject(w);
	Window			win = XtWindowOfObject(w);
	Window			root;
	Window			parent;
	Window *		children;
	unsigned int		nchildren;
	XWindowAttributes	window_attributes;

	XUngrabPointer(XtDisplayOfObject(w), CurrentTime);

	do
	{
		XQueryTree(dpy, win, &root, &parent, &children, &nchildren);
		if (children != NULL)
			XFree((char *)children);
		XGetWindowAttributes(dpy, parent, &window_attributes);

		event-> xany.window = parent;
		event-> xbutton.x += window_attributes.x;
		event-> xbutton.y += window_attributes.y;

		if (!(window_attributes.all_event_masks & ButtonPressMask ||
		      window_attributes.your_event_mask & ButtonPressMask))
			win = parent;
	} while (win == parent && win != root);

	if (win != parent)
		XSendEvent(XtDisplayOfObject(w), parent, True,
				ButtonPressMask, event);

} /* END OF OlReplayBtnEvent */

/*
 *************************************************************************
 * OlSetGui - simply set the OlGuiMode variable
 ****************************procedure*header*****************************
 */
#ifndef UNKNOWN_GUI
#define UNKNOWN_GUI(x)	((x != OL_OPENLOOK_GUI) && (x != OL_MOTIF_GUI))
#endif

void
OlSetGui OLARGLIST((type))
  OLGRA( OlDefine,  type )
{
  if (UNKNOWN_GUI(type))
    OlVaDisplayWarningMsg((Display *)NULL,
                          OleNbadGui,
			  OleTsetGui,
                          OleCOlToolkitWarning,
			  OleMbadGui_setGui);
  else OlGuiMode = type;
}
#undef UNKNOWN_GUI

/*
 *************************************************************************
 * OlUpdateDisplay - force the appearance of the given widget to be
 *			updated right away. i.e., process all pending
 *			exposure events immediately.
 ****************************procedure*header*****************************
 */
void
OlUpdateDisplay OLARGLIST((w))
	OLGRA( Widget,		w)
{
	Display *	dpy = XtDisplayOfObject(w);
	Window		window = XtWindow(w);
	XEvent		xevent;

		/* flush the event queue */
	XSync(dpy, False);

		/* peel off Expose events manually and dispatch them */
	while (XCheckWindowEvent(dpy, window, ExposureMask, &xevent) == True)
		XtDispatchEvent(&xevent);

} /* END OF OlUpdateDisplay */

/*
 *************************************************************************
 * OlWidgetClassToClassName - given a widget class, return the class name
 ****************************procedure*header*****************************
 */
#undef OlWidgetClassToClassName

String
OlWidgetClassToClassName OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)
{
#define OlWidgetClassToClassName(wc) ((wc)->core_class.class_name)

	return OlWidgetClassToClassName(wc);

} /* END OF OlWidgetClassToClassName */

/*
 *************************************************************************
 * OlWidgetToClassName - given a widget, return the class name
 ****************************procedure*header*****************************
 */
#undef OlWidgetToClassName

String
OlWidgetToClassName OLARGLIST((w))
	OLGRA( Widget,	w)
{
#define OlWidgetToClassName(w) OlWidgetClassToClassName(XtClass(w))

	return OlWidgetToClassName(w);

} /* END OF OlWidgetToClassName */

/* The following structure maintains, on a per screen basis, */
/* 25.4*{Width,Height}OfScreen/72/{Width,Height}MMOfScreen */

typedef struct _List{
    double width;
    double height;
    struct _List *next;
    Screen *screen;
} List;

double
Ol_ScreenPointToPixel OLARGLIST((dir, value, scr))
	OLARG( int,		dir)
	OLARG( double,		value)
	OLGRA( Screen *,	scr)
{
    static List *head = NULL;
    static List *pl = NULL;

    /* Use width/height in pl if pl has the correct screen */
    if (pl == NULL || pl->screen != scr) {
	/* Otherwise look for the correct screen starting at head */
	for (pl=head; pl!=NULL; pl=pl->next) {
	    /* If correct screen value - break with non-Null pl */
	    if (pl->screen == scr) break;
	}
	if (pl == NULL) {
	    /* No match so create a new node in the list */
	    pl = (List *) XtMalloc (sizeof (List));
	    pl->width = 25.4 * (double)WidthOfScreen(scr) /
			((double)WidthMMOfScreen(scr) * 72.0);
	    pl->height = 25.4 * (double)HeightOfScreen(scr) /
			((double)HeightMMOfScreen(scr) * 72.0);
	    pl->screen = scr;
	    pl->next = head;
	    head = pl;
	}
    }
    return (dir == OL_HORIZONTAL ? pl->width : pl->height) * value;
} /* end of Ol_ScreenPointToPixel */

double
OlRound OLARGLIST((dbl))
	OLGRA( double,	dbl)
{
    return (dbl < 0) ? dbl - .5 : dbl + .5;
} /* end of OlRound */


#if !defined(LIBOL)
#define LIBOL (OLconst char *)"libOlitO.so"
#endif

#if !defined(LIBMO)
#define LIBMO (OLconst char *)"libOlitM.so"
#endif

#if defined(SVR4_0) || defined(SVR4) || defined(sun)

static void *
FindGuiLib OL_NO_ARGS()
{
#if !defined(DEFAULTPATH)
#define DEFAULTPATH (OLconst char *)"/usr/X/lib"
#endif

	OLconst char *	lib_name;
	void *		answer = (void *)NULL;

	lib_name = OlGetGui() == OL_MOTIF_GUI ? LIBMO : LIBOL;

	if ( (answer = dlopen(lib_name, RTLD_LAZY)) == (void *)NULL )
	{
#define BUF_SIZE		256
		char		local_buf[256];
		char *		dft_name;
		unsigned int	total_size;

		total_size = strlen(DEFAULTPATH) + strlen(lib_name) + 2;
		if (total_size <= BUF_SIZE)
			dft_name = local_buf;
		else
			dft_name = XtMalloc(total_size);

		sprintf(dft_name, (OLconst char *)"%s/%s",
					DEFAULTPATH, lib_name);

		answer = dlopen(dft_name, RTLD_LAZY);

		if (dft_name != local_buf)
			XtFree(dft_name);

#undef BUF_SIZE
	}
	return(answer);
} /* end of FindGuiLib */

#endif

extern void
_OlResolveGUISymbol OLARGLIST((name, addr, OLVARGLIST))
	OLARG( char *,	name)	/* 1st pair */
	OLARG( void **,	addr)
	OLVARGS
{
	/* Dynamic linking in SVR4.0 only (not 3.2) */
#if defined(SVR4_0) || defined(SVR4) || defined(sun)
	va_list		ap;
	char *		symbol_name;
	void **		symbol_addr;

	static void *	uselib = (void *)NULL;

	if (name == (char *)NULL || addr == (void **)NULL)
		return;

	if (uselib == (void *)NULL &&
	    (uselib = FindGuiLib()) == (void *)NULL)
	{
		OlVaDisplayWarningMsg(
			OlDefaultDisplay,
			OleNOlResolveGUISymbol,
			OleTdlopen,
			OleCOlToolkitWarning,
			OleMOlResolveGUISymbol_dlopen,
			dlerror()
		);
		return;
	} /* end if uselib == NULL */

		/* We now have the dynamic library we want open.
		 * Get symbols one at time, assign addresses.
		 */ 
	symbol_name = name;
	symbol_addr = addr;
	VA_START(ap, addr);
	for(;;)
	{
		if ((*symbol_addr = dlsym(uselib, symbol_name)) == NULL)
		{
			OlVaDisplayWarningMsg(
				OlDefaultDisplay,
				OleNOlResolveGUISymbol,
				OleTdlsym,
				OleCOlToolkitWarning,
				OleMOlResolveGUISymbol_dlsym,
				dlerror()
			);
		}
		if ( (symbol_name = va_arg(ap, char *)) == NULL ||
		     (symbol_addr = va_arg(ap, void **)) == NULL )
		{
			break;
		}
	}
	va_end(ap);
#else
	return;
#endif
} /* end of _OlResolveGUISymbol */

#define GUIVAR (OLconst char *)"XGUI"

void
OlToolkitInitialize OLARGLIST ((argc, argv, closure))
  OLARG( int *,		argc )
  OLARG( char **,	argv )
  OLGRA( XtPointer,	closure )
{
	String	xgui;

	if (OlApplicationWidget != (Widget)NULL)
	{
		OlVaDisplayWarningMsg(
			(Display *)NULL,
			OleNOlInitialize,
			OleTbadInitialize,
			OleCOlToolkitWarning,
			OleMOlInitialize_badInitialize,
			(OLconst char *)"OlToolkitInitialize"
		);
		return;
	}

	/* We know: the default is Motif */

		/* Process env variable */
	xgui = getenv((OLconst char *)GUIVAR);

	if (xgui && xgui[0]) {
		if (!strcmp(xgui, (OLconst char *)"MOTIF"))
			OlSetGui(OL_MOTIF_GUI);
		else if (!strcmp(xgui, (OLconst char *)"OPEN_LOOK"))
			OlSetGui(OL_OPENLOOK_GUI);
		else
		{
			OlVaDisplayWarningMsg(
				(Display *)NULL,
				OleNbadGui,
				OleTguiVar,
				OleCOlToolkitWarning,
				OleMbadGui_guiVar
			);
		}
	} /* if (xgui...) */

		/* Process command line */
	if (argc != NULL && argv != NULL && *argc > 1)
	{
			/* leave room for more gui's later */
		static OLconst char * guiNames[] = {
			"-motif",
			"-openlook",
		};
		int	i, j,
			num = XtNumber(guiNames);
		char **	args = argv;
		Boolean found = False;

		for (i = 1, args++; i < *argc; i++, args++) {
			for (j = 0; j < num; j++) {
				if (!strcmp(*args, guiNames[j]))
				break;
			}
			if (j < num) {		/* match! */
				if (found)	/* found one before this one */
					OlVaDisplayWarningMsg(
						(Display *)NULL,
						OleNbadGui,
						OleTcommandLine,
						OleCOlToolkitWarning,
						OleMbadGui_commandLine
					);
				else {		/* ok.  Set the gui. */
					found = True;

					switch (j) {
						case 0:
						   OlSetGui(OL_MOTIF_GUI);
						   break;
						case 1:
						   OlSetGui(OL_OPENLOOK_GUI);
						   break;
					} /* switch */

				} /* else */
	
					/* remove found arg from argc & argv */
				if (i == (*argc)--)
					argv[i] = NULL;
				else {
					int k;

					for (k = i; k < *argc; k++)
						argv[k] = argv[k+1];
					argv[k] = NULL;
				} /* else */
			} /* if (j < num) */
		} /* for (i = 1...) */
	} /* if (argc != NULL && argv != NULL && *argc > 1) */

	OlPreInitialize(
		(String)NULL,			/* classname	*/
		(XrmOptionDescRec *)NULL,	/* urlist	*/
		(Cardinal)0,			/* num_urs	*/
		argc, argv
	);
} /* end of OlToolkitInitialize */
