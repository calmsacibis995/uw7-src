#pragma ident	"@(#)m1.2libs:Xm/FontObj.c	1.9"
#ifdef USE_FONT_OBJECT
#include <Xm/Xm.h>
#include <Xm/FontObjP.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/Scale.h>
#include <Xm/Display.h>
#include <X11/Shell.h>
#include <Xm/BulletinB.h>
#include <Xm/MenuShell.h>
#ifdef I18N_MSG
#include "XmMsgI.h"
#endif

#define SHELL_LIST_STEP	16
#define CORE_C(WC) ((WidgetClass)(WC))->core_class
#define FONT_C(WC) ((FontObjectClass)(WC))->font_class

#define SANS_SERIF_MASK	(1 << 1)
#define SERIF_MASK	(1 << 2)
#define MONO_MASK	(1 << 3)

/* Global Data */

/* Local Data */
static XrmQuark	XmQDynamicFontListClassExtension = 0;
static XtResource SerifFontRes[] = {
    {	XmNserifFamilyFontList,
	XmCSerifFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	0,
	XmRString,
"-*-times-medium-r-normal--1*-120-*-*-p-*-iso8859-1=PLAIN_FONT_TAG,-*-times-medium-i-normal--1*-120-*-*-p-*-iso8859-1=ITALIC_FONT_TAG,-*-times-bold-r-normal--1*-120-*-*-p-*-iso8859-1=BOLD_FONT_TAG,-*-times-bold-i-normal--1*-120-*-*-p-*-iso8859-1=BOLD_ITALIC_FONT_TAG",
    }
};
static XtResource SansSerifFontRes[] = {
    {	XmNsansSerifFamilyFontList,
	XmCSansSerifFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	0,
	XmRString,
"-*-helvetica-medium-r-normal--1*-120-*-*-p-*-iso8859-1=PLAIN_FONT_TAG,-*-helvetica-medium-o-normal--1*-120-*-*-p-*-iso8859-1=ITALIC_FONT_TAG,-*-helvetica-bold-r-normal--1*-120-*-*-p-*-iso8859-1=BOLD_FONT_TAG,-*-helvetica-bold-o-normal--1*-120-*-*-p-*-iso8859-1=BOLD_ITALIC_FONT_TAG",
	}
};
static XtResource MonospacedFontRes[] = {
    {	XmNmonospacedFamilyFontList,
	XmCMonospacedFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	0,
	XmRString,
"-*-courier-medium-r-normal--1*-120-*-*-m-*-iso8859-1=PLAIN_FONT_TAG,-*-courier-medium-o-normal--1*-120-*-*-m-*-iso8859-1=ITALIC_FONT_TAG,-*-courier-bold-r-normal--1*-120-*-*-m-*-iso8859-1=BOLD_FONT_TAG,-*-courier-bold-o-normal--1*-120-*-*-m-*-iso8859-1=BOLD_ITALIC_FONT_TAG",
	}
};
static XtResource DynamicFamilyRes[] = {
    {	XmNserifFamilyFontList,
	XmCSerifFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	XtOffsetOf(FamilyFontLists, serif),
	XmRImmediate,
	NULL
    },
    {	XmNsansSerifFamilyFontList,
	XmCSansSerifFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	XtOffsetOf(FamilyFontLists, sans_serif),
	XmRImmediate,
	NULL
    },
    {	XmNmonospacedFamilyFontList,
	XmCMonospacedFamilyFontList,
	XmRFontList,
	sizeof(XmFontList),
	XtOffsetOf(FamilyFontLists, mono),
	XmRImmediate,
	NULL
    }
};
/********    External Function Declarations    ********/
#ifdef _NO_PROTO
void XmAddDynamicFontListClassExtension();
XmFontList _XmFontObjectGetDefaultFontList();
#else
void XmAddDynamicFontListClassExtension(
	WidgetClass	wc);
XmFontList _XmFontObjectGetDefaultFontList(
	Widget w,
	XmFontList font);
#endif /* _NO_PROTO */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void FontObjectClassInitialize() ;
static void FontObjectDestroy() ;
static void FontObjectInitialize() ;
static void FontObjectRealize() ;
static Boolean FontObjectSetValues();
static void FontObjectGetValuesHook();
static XmFontList GetDefaultFont();
static void CheckForFontListArg ();
static void ProcessWidgetNode ();
static void DynamicHandler ();
static char * GetStringDb ();
static void GetNewXtdefaults ();
static void DynResProc();
static void AddToShellList ();
static void DelFromShellList ();
static XmClassExtension GetClassExtension ();
static DynamicFontListClassExtension GetFontListExt();
static void InitializeWrapper();
static Boolean SetValuesWrapper();
static void DestroyWrapper();
static void ShellInitializeWrapper();

#else

static void FontObjectClassInitialize( 
                        void) ;
static void FontObjectDestroy( 
                        Widget wid) ;
static void FontObjectInitialize( 
                        Widget rq,
                        Widget nw,
                        ArgList Args,
                        Cardinal *numArgs) ;
static void FontObjectRealize(
			Widget w,
			XtValueMask * value_mask,
			XSetWindowAttributes * attributes);
static Boolean FontObjectSetValues(
			Widget current,
			Widget request,
			Widget nw,
			ArgList args,
			Cardinal * num_args) ;
static void FontObjectGetValuesHook(
			Widget w,
			ArgList args,
			Cardinal * num_args) ;
static XmFontList GetDefaultFont(
			Widget w,
			char * font_string) ;
static void CheckForFontListArg (
			Widget w,
			ArgList args,
			Cardinal * num_args);
static void ProcessWidgetNode (
			Widget w,
			XmFontList font,
			int font_mask);
static void DynamicHandler (
        		Widget          w,
        		XtPointer       client_data,
        		XEvent *        event,
        		Boolean *       cont_to_dispatch);
static char * GetStringDb (
        		Display * dpy);
static void GetNewXtdefaults (
			Widget	w);
static void DynResProc(
			FontObject fo,
			XmFontList font,
			int font_mask);
static void AddToShellList (
			Widget w);
static void DelFromShellList (
			Widget w,
			XtPointer client_data,
			XtPointer call_data);
static XmClassExtension GetClassExtension (
			XmClassExtension extension,
			XrmQuark record_type,
			long version);
static DynamicFontListClassExtension GetFontListExt(
			Widget w);
static void InitializeWrapper(
		        Widget rq,
        		Widget nw,
        		ArgList Args,
        		Cardinal *numArgs);
static Boolean SetValuesWrapper(
			Widget current,
			Widget request,
			Widget nw,
			ArgList args,
			Cardinal * num_args) ;
static void DestroyWrapper(
			Widget w);
static void ShellInitializeWrapper(
                        Widget rq,
                        Widget nw,
                        ArgList Args,
                        Cardinal *numArgs) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static XtResource resources[] = {
    {  
	XmNdynamicFontCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	XtOffset (FontObject, font.dynamic_font_callback),
	XmRImmediate,
	(XtPointer) NULL,
    },
};

FontObjectClassRec _xmFontObjectClassRec = 
{
    {
        (WidgetClass)&coreClassRec,       /* superclass            */
        "FontObject",                     /* class_name            */
        sizeof(FontObjectRec),            /* widget_size           */
        FontObjectClassInitialize,        /* class_initialize      */
        NULL,                             /* class_part_initialize */
        FALSE,                            /* class_inited          */
        FontObjectInitialize,             /* initialize            */
        NULL,                             /* initialize_hook       */
        FontObjectRealize,                /* realize               */
        NULL,                             /* actions               */
        0,                                /* num_actions           */
        resources,                        /* resources             */
        XtNumber(resources),              /* num_resources         */
        NULLQUARK,                        /* xrm_class             */
        FALSE,                            /* compress_motion       */
        FALSE,                            /* compress_exposure     */
        FALSE,                            /* compress_enterleave   */
        FALSE,                            /* visible_interest      */
        FontObjectDestroy,                /* destroy               */
        NULL,                             /* resize                */
        NULL,                             /* expose                */
        FontObjectSetValues,              /* set_values            */
        NULL,                             /* set_values_hook       */
        NULL,                             /* set_values_almost     */
        FontObjectGetValuesHook,          /* get_values_hook       */
        NULL,                             /* accept_focus          */
        XtVersion,                        /* version               */
        NULL,                             /* callback_offsets      */
        NULL,                             /* tm_table              */
        NULL,                             /* query_geometry        */
        NULL,                             /* display_accelerator   */
        NULL                              /* extension             */
    },
    {					/* fontObject class	*/
	NULL,				/* shell_initialize	*/
	NULL,				/* shell_destroy	*/
	NULL,				/* extension		*/
    },
};

WidgetClass _xmFontObjectClass = (WidgetClass)&_xmFontObjectClassRec;

/**********************************************************************/
/** _XmFontObjectCreate() - initialize_hook() from Display object... **/
/**         Used to create a FontObject.                             **/
/**********************************************************************/
/*ARGSUSED*/
void 
#ifdef _NO_PROTO
_XmFontObjectCreate( w, al, acPtr )
        Widget w ;
        ArgList al ;
        Cardinal *acPtr ;
#else
_XmFontObjectCreate(
        Widget w,
        ArgList al,
        Cardinal *acPtr )
#endif /* _NO_PROTO */
{
	Widget fo;

	fo = XtCreateWidget("fontObject", _xmFontObjectClass, w, NULL, 0);

	XtRealizeWidget(fo);
}  /* end of _XmFontObjectCreate() */

static DynamicFontListClassExtension
#ifdef _NO_PROTO
GetFontListExt(w)
	Widget w;
#else
GetFontListExt(
	Widget w)
#endif
{
	WidgetClass wc = XtClass(w);
	DynamicFontListClassExtension font_ext = NULL;

	while (wc != coreWidgetClass && font_ext == NULL && wc != NULL)  {
		if (CORE_C(wc).extension != NULL)
			font_ext = (DynamicFontListClassExtension)
				GetClassExtension(CORE_C(wc).extension,
				XmQDynamicFontListClassExtension,
				DynamicFontListClassExtensionVersion);
		wc = CORE_C(wc).superclass;
	}
	return(font_ext);
} /* end of GetFontListExt() */

static XmClassExtension
#ifdef _NO_PROTO
GetClassExtension (extension, record_type, version)
	XmClassExtension extension;
	XrmQuark record_type;
	long version;
#else
GetClassExtension (
	XmClassExtension extension,
	XrmQuark record_type,
	long version)
#endif
{
        while (extension != NULL &&
                !(extension->record_type == record_type &&
                  (!version || version == extension->version)))
        {
                extension = (XmClassExtension) extension->next_extension;
        }
        return (extension);
}  /* end of GetClassExtension() */

void
#ifdef _NO_PROTO
XmAddDynamicFontListClassExtension(wc)
	WidgetClass	wc;
#else
XmAddDynamicFontListClassExtension(
	WidgetClass	wc)
#endif
{
	DynamicFontListClassExtension new_ext;
	XtPointer save = CORE_C(wc).extension;

	new_ext = (DynamicFontListClassExtension)
			XtNew(DynamicFontListClassExtensionRec);
	if (!new_ext)
		/* out of memory */
		return;

	CORE_C(wc).extension = (XtPointer)new_ext;
	new_ext->next_extension = save;
	new_ext->record_type = XmQDynamicFontListClassExtension;
	new_ext->version = DynamicFontListClassExtensionVersion;
	new_ext->record_size = sizeof(DynamicFontListClassExtensionRec);

#define ENVELOPE(METHOD,NEW) \
        new_ext->METHOD = CORE_C(wc).METHOD;                      \
        CORE_C(wc).METHOD = NEW

        ENVELOPE (initialize, InitializeWrapper);
        ENVELOPE (set_values, SetValuesWrapper);
        ENVELOPE (destroy, DestroyWrapper);
#undef  ENVELOPE
        new_ext->fonts_used = XUniqueContext();
}  /* end of XmAddDynamicFontListClassExtension() */

static void
#ifdef _NO_PROTO
FontObjectClassInitialize( )
#else
FontObjectClassInitialize(void)
#endif
{
	XmQDynamicFontListClassExtension =
		XrmStringToQuark(XmNDynamicFontListClassExtension);

	/*  Add the extension to std Motif classes  */
	XmAddDynamicFontListClassExtension(xmTextWidgetClass);
	XmAddDynamicFontListClassExtension(xmTextFieldWidgetClass);
	XmAddDynamicFontListClassExtension(xmLabelWidgetClass);
	XmAddDynamicFontListClassExtension(xmLabelGadgetClass);
	XmAddDynamicFontListClassExtension(xmListWidgetClass);
	XmAddDynamicFontListClassExtension(xmScaleWidgetClass);
	XmAddDynamicFontListClassExtension(xmBulletinBoardWidgetClass);
	XmAddDynamicFontListClassExtension(xmMenuShellWidgetClass);
	XmAddDynamicFontListClassExtension(vendorShellWidgetClass);

	/*  Wrap the Shell's initialize and destroy to keep track
	    of all the shells.  Need to know the shells to make dynamic
	    changes to all the widgets in an application.  */
#define ENVELOPE(METHOD_C,METHOD_FO,NEW) \
        FONT_C(_xmFontObjectClass).METHOD_FO = CORE_C(shellWidgetClass).METHOD_C; \
        CORE_C(shellWidgetClass).METHOD_C = NEW

        ENVELOPE (initialize, shell_initialize, ShellInitializeWrapper);
#undef  ENVELOPE
	
}  /* end of FontObjectClassInitialize() */

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
FontObjectDestroy( wid )
        Widget wid ;
#else
FontObjectDestroy( Widget wid )
#endif /* _NO_PROTO */
{
	FontObject tmpFontObject = (FontObject)wid;

	/* The window may be the root window, so set it to null to prevent it
	   from being destroyed */
	wid->core.window = NULL;

	if (tmpFontObject->font.sans_serif)
		XmFontListFree (tmpFontObject->font.sans_serif);
	if (tmpFontObject->font.serif)
		XmFontListFree (tmpFontObject->font.serif);
	if (tmpFontObject->font.mono)
		XmFontListFree (tmpFontObject->font.mono);

	/* Unwrap the Core methods for shellClass that registered. */
#define UNENVELOPE(METHOD_C,METHOD_FO) \
        CORE_C(shellWidgetClass).METHOD_C = FONT_C(_xmFontObjectClass).METHOD_FO;

        UNENVELOPE (initialize, shell_initialize);
#undef  UNENVELOPE
}  /* end of FontObjectDestroy() */

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
FontObjectInitialize( rq, nw, Args, numArgs )
        Widget rq ;
        Widget nw ;
        ArgList Args ;
        Cardinal *numArgs ;
#else
FontObjectInitialize(
        Widget rq,
        Widget nw,
        ArgList Args,
        Cardinal *numArgs)
#endif /* _NO_PROTO */
{
	FontObject nw_fo = (FontObject) nw ;

	nw_fo->core.mapped_when_managed = False;
	nw_fo->core.width = 1;
	nw_fo->core.height = 1;
	nw_fo->font.sans_serif = NULL;
	nw_fo->font.serif = NULL;
	nw_fo->font.mono = NULL;
	nw_fo->font.shell_list = (WidgetList) NULL;
	nw_fo->font.shell_list_size = (Cardinal) 0;
	nw_fo->font.shell_list_alloc_size = (Cardinal) 0;
}  /* end of FontObjectInitialize() */

/*ARGSUSED*/
static void
#ifdef _NO_PROTO
FontObjectRealize(w, value_mask, attributes)
	Widget w;
	XtValueMask * value_mask;
	XSetWindowAttributes * attributes;
#else
FontObjectRealize(
	Widget w,
	XtValueMask * value_mask,
	XSetWindowAttributes * attributes)
#endif
{
	Widget rootw;
	Window root = RootWindowOfScreen(XtScreenOfObject(w));

	rootw = XtWindowToWidget(XtDisplayOfObject(w), root);

	if (!rootw)  {
		/*  Use the root window as the FontObject's window */
		rootw = w;
		rootw->core.window = root;
	}

	XtAddEventHandler(rootw, PropertyChangeMask, False,
                                     DynamicHandler, (XtPointer)NULL);
}  /* end of FontObjectRealize() */

/*ARGSUSED*/
static Boolean
#ifdef _NO_PROTO
FontObjectSetValues(current, request, nw, args, num_args)
	Widget current;
	Widget request;
	Widget nw;
	ArgList args;
	Cardinal * num_args;
#else
FontObjectSetValues(
	Widget current,
	Widget request,
	Widget nw,
	ArgList args,
	Cardinal * num_args)
#endif /* _NO_PROTO */
{
#define DIFFERENT(F) \
	(((FontObject)nw)->F != ((FontObject)current)->F)

	XmFontObjectCallbackStruct data;
	int i;

	data.event = NULL;
	/* The FamilyFontList resources are subresrouces so that the default
	   values are not converted until the application uses them.  So,
	   we have to manually look at the args and set the value into the
	   new widget. */
	for (i = 0; i < *num_args; i++)  {
		if (strcmp(args[i].name, XmNsansSerifFamilyFontList) == 0)
			((FontObject)nw)->font.sans_serif = (XmFontList)args[i].value;
		else if (strcmp(args[i].name, XmNmonospacedFamilyFontList) == 0)
			((FontObject)nw)->font.mono = (XmFontList)args[i].value;
		else if (strcmp(args[i].name, XmNserifFamilyFontList) == 0)
			((FontObject)nw)->font.serif = (XmFontList)args[i].value;
	}
	if (DIFFERENT(font.serif))  {
		if (((FontObject)nw)->font.serif == NULL)
			((FontObject)nw)->font.serif =
				GetDefaultFont(nw, XmNserifFamilyFontList);
		((FontObject)nw)->font.serif =
			XmFontListCopy(((FontObject)nw)->font.serif);
		data.reason = XmCR_SERIF_FAMILY_CHANGED;
		data.new_font = ((FontObject)nw)->font.serif;
		data.current_font = ((FontObject)current)->font.serif;
		DynResProc((FontObject)nw, data.new_font, SERIF_MASK);
		XtCallCallbacks(nw, XmNdynamicFontCallback, (XtPointer)&data);
		XmFontListFree(((FontObject)current)->font.serif);
	}
	if (DIFFERENT(font.sans_serif))  {
		if (((FontObject)nw)->font.sans_serif == NULL)
			((FontObject)nw)->font.sans_serif =
				GetDefaultFont(nw, XmNsansSerifFamilyFontList);
		((FontObject)nw)->font.sans_serif =
			XmFontListCopy(((FontObject)nw)->font.sans_serif);
		data.reason = XmCR_SANS_SERIF_FAMILY_CHANGED;
		data.new_font = ((FontObject)nw)->font.sans_serif;
		data.current_font = ((FontObject)current)->font.sans_serif;
		DynResProc((FontObject)nw, data.new_font, SANS_SERIF_MASK);
		XtCallCallbacks(nw, XmNdynamicFontCallback, (XtPointer)&data);
		XmFontListFree(((FontObject)current)->font.sans_serif);
	}
	if (DIFFERENT(font.mono))  {
		if (((FontObject)nw)->font.mono == NULL)
			((FontObject)nw)->font.mono =
				GetDefaultFont(nw, XmNmonospacedFamilyFontList);
		((FontObject)nw)->font.mono =
			XmFontListCopy(((FontObject)nw)->font.mono);
		data.reason = XmCR_MONOSPACED_FAMILY_CHANGED;
		data.new_font = ((FontObject)nw)->font.mono;
		data.current_font = ((FontObject)current)->font.mono;
		DynResProc((FontObject)nw, data.new_font, MONO_MASK);
		XtCallCallbacks(nw, XmNdynamicFontCallback, (XtPointer)&data);
		XmFontListFree(((FontObject)current)->font.mono);
	}
#undef DIFFERENT

	return(False);
} /* end of FontObjectSetValues() */

static void
#ifdef _NO_PROTO
FontObjectGetValuesHook(w, args, num_args)
	Widget w;
	ArgList args;
	Cardinal * num_args;
#else
FontObjectGetValuesHook(
	Widget w,
	ArgList args,
	Cardinal * num_args)
#endif /* _NO_PROTO */
{
	Cardinal i;
	FontObject fo = (FontObject) w;

	for (i = 0; i < *num_args; i++)  {
		if (strcmp(args[i].name, XmNserifFamilyFontList) == 0) {
			if (fo->font.serif == NULL) {
				fo->font.serif = 
					XmFontListCopy(GetDefaultFont(w, args[i].name));
			}
			*(XmFontList *)args[i].value = fo->font.serif;
		}
		if (strcmp(args[i].name, XmNsansSerifFamilyFontList) == 0) {
			if (fo->font.sans_serif == NULL) {
				fo->font.sans_serif =
					XmFontListCopy(GetDefaultFont(w, args[i].name));
			}
			*(XmFontList *)args[i].value = fo->font.sans_serif;
				
		}
		if (strcmp(args[i].name, XmNmonospacedFamilyFontList) == 0) {
			if (fo->font.mono == NULL) {
				fo->font.mono =
					XmFontListCopy(GetDefaultFont(w, args[i].name));
			}
			*(XmFontList *)args[i].value = fo->font.mono;
		}
	}
} /* end of FontObjectGetValuesHook() */

static XmFontList
#ifdef _NO_PROTO
GetDefaultFont(w, font_string)
	Widget w;
	char * font_string;
#else
GetDefaultFont(
	Widget w,
	char * font_string)
#endif
{
	XtArgVal font_list;
	XtResourceList res;

	if (strcmp(font_string, XmNserifFamilyFontList) == 0) {
		res = SerifFontRes;
	}
	if (strcmp(font_string, XmNsansSerifFamilyFontList) == 0) {
		res = SansSerifFontRes;
	}
	if (strcmp(font_string, XmNmonospacedFamilyFontList) == 0) {
		res = MonospacedFontRes;
	}
	XtGetSubresources(w, &font_list, NULL, NULL, res, 1, NULL, 0);

	return((XmFontList)font_list);
}  /* end of GetDefaultFont() */

/*
 * DynamicHandler - this routine is used to monitor changes of the
 * RESOURCE_MANAGER property that occur on the RootWindow.  When this
 * property changes, several application databases need to be
 * re-initialized.
 */
/* ARGSUSED */
static void
#ifdef _NO_PROTO
DynamicHandler (w, client_data, event, cont_to_dispatch)
        Widget          w;
        XtPointer       client_data;
        XEvent *        event;
        Boolean *       cont_to_dispatch;
#else
DynamicHandler (
        Widget          w,
        XtPointer       client_data,
        XEvent *        event,
        Boolean *       cont_to_dispatch)
#endif
{
	FontObject fo = (FontObject) XtNameToWidget(
		(Widget)XmGetXmDisplay(XtDisplayOfObject(w)), "fontObject");
        if ((event-> type == PropertyNotify) &&
            (event-> xproperty.atom == XA_RESOURCE_MANAGER) &&
            (event-> xproperty.state == PropertyNewValue)) {
		FamilyFontLists before_fonts, after_fonts;
		Arg args[3];
		int count = 0;

               	XtGetSubresources(w, &before_fonts, NULL, NULL,
			DynamicFamilyRes, XtNumber(DynamicFamilyRes), NULL, 0);

                /* Re-initialize the database so new clients get the change. */
                GetNewXtdefaults(w);

               	XtGetSubresources(w, &after_fonts, NULL, NULL,
			DynamicFamilyRes, XtNumber(DynamicFamilyRes), NULL, 0);

                /* Do dynamic resource processing */
		if (before_fonts.sans_serif != after_fonts.sans_serif) {
			XtSetArg(args[count], XmNsansSerifFamilyFontList, after_fonts.sans_serif);
			count++;
		}
		if (before_fonts.serif != after_fonts.serif)  {
			XtSetArg(args[count], XmNserifFamilyFontList, after_fonts.serif);
			count++;
		}
		if (before_fonts.mono != after_fonts.mono)  {
			XtSetArg(args[count], XmNmonospacedFamilyFontList, after_fonts.mono);
			count++;
		}

		if (count)
			XtSetValues((Widget)fo, args, count);
        }
} /* end of DynamicHandler */

/*
 * GetStringDb
 *
 * This procedure reads the RESOURCE_MANAGER property of the Root Window
 * and returns it.
 * This code was swiped (almost verbatim) from Xlib (XConnectDisplay).
 *
 */
static char *
#ifdef _NO_PROTO
GetStringDb (dpy)
        Display * dpy;
#else
GetStringDb (
        Display * dpy)
#endif
{
        Atom actual_type;
        int actual_format;
        unsigned long nitems;
        unsigned long leftover;
        char *  string_db;

        if (XGetWindowProperty(dpy, RootWindow(dpy, 0),
            XA_RESOURCE_MANAGER, 0L, 100000000L, False, XA_STRING,
            &actual_type, &actual_format, &nitems, &leftover,
            (unsigned char **) &string_db) != Success) {
                        string_db = (char *) NULL;
        }
        else if ((actual_type != XA_STRING) || (actual_format != 8)) {
                if (string_db != NULL) {
                        XFree ( string_db );
                        string_db = (char *) NULL;
                }
        }
        return(string_db);
} /* end of GetStringDb */

static void
#ifdef _NO_PROTO
GetNewXtdefaults (w)
	Widget	w;
#else
GetNewXtdefaults (
	Widget	w)
#endif
{
        Display * dpy = XtDisplayOfObject(w);
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
        int num_screens = ScreenCount(dpy);
	Screen * screen;
	int i;
#endif
        char *  string_db = GetStringDb(dpy);
        XrmDatabase db;

        if (string_db != (char *)NULL) {
                XrmDatabase rdb = XrmGetStringDatabase(string_db);

                /* free the string database     */
                XFree(string_db);

                        /* Merge the new database into the existing
			 * Intrinsics database.  Note, the merge
                         * is destructive for 'rdb,' so we don't
                         * have to free it explicitly.
                         */
		if (rdb == NULL)
			return;
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
		for (i = 0; i < num_screens; i++)  {
			screen = ScreenOfDisplay(dpy, i);
                	db = XtScreenDatabase(screen);
                        XrmMergeDatabases(rdb, &db);
		}
#else
               	db = XtDatabase(dpy);
                XrmMergeDatabases(rdb, &db);
#endif
        }
} /* end of GetNewXtdefaults */

/*
 * DynResProc: This function processes dynamic changes of fonts.
 */
static void
#ifdef _NO_PROTO
DynResProc(fo)
	FontObject fo;
	XmFontList font;
	int font_mask;
#else
DynResProc(
	FontObject fo,
	XmFontList font,
	int font_mask)
#endif
{
        /*
         * This is used to prevent recursion. Can't use DynResProcessing
         * here, because it may be turned off temporarily before calling
         * application callbacks.
         */
        static Boolean door = False;

        int i;

        /* check for recursion */
        if (door == True) {
                door = False;
                return;
        }

        door = True;

        /* loop through each base window shell */
        for (i=0; i < fo->font.shell_list_size; i++) {
		ProcessWidgetNode(fo->font.shell_list[i], font, font_mask);
        } /* for each shell */

        door = False;
} /* DynResProc() */

/*
 * Add a new shell to the shell list. The shell list is used to keep track
 * of all the shells in an application, so that a dynamic change can be
 * propagated to all the shells.
 */
static void
#ifdef _NO_PROTO
AddToShellList (w)
	Widget w;
#else
AddToShellList (
	Widget w)
#endif
{
	FontObject fo = (FontObject) XtNameToWidget(
		(Widget)XmGetXmDisplay(XtDisplayOfObject(w)), "fontObject");

	if (!fo)
		return;

        /* add to the base shell list */
        if (fo->font.shell_list_alloc_size == fo->font.shell_list_size) {
                Widget *nw;

                fo->font.shell_list_alloc_size += SHELL_LIST_STEP;
                nw = (Widget *)XtRealloc((char *)fo->font.shell_list,
                        fo->font.shell_list_alloc_size * sizeof(Widget));
                if (nw == NULL) {
                        fo->font.shell_list_alloc_size -= SHELL_LIST_STEP;
			return;
                }
                fo->font.shell_list = nw;
        }
        fo->font.shell_list[fo->font.shell_list_size++] = w;

	XtAddCallback(w, XtNdestroyCallback, DelFromShellList, fo);
} /* end of AddToShellList */

static void
#ifdef _NO_PROTO
DelFromShellList (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
#else
DelFromShellList (
	Widget w,
	XtPointer client_data,
	XtPointer call_data)
#endif
{
        register int i;
        register Widget *sh;
	FontObject fo = (FontObject) client_data;

	if (!fo)
		return;

        for (i=0, sh=fo->font.shell_list; i < fo->font.shell_list_size;
		i++, sh++)
                if (*sh == w) {
                        int remains = fo->font.shell_list_size -
                                         (int)(sh - fo->font.shell_list) - 1;

                        if (remains)
                                (void) memcpy((void *)sh, (void *)(sh+1),
                                        remains * sizeof(Widget));
                        fo->font.shell_list_size--;
                        break;
                }
} /* end of DelFromShellList */

static void
#ifdef _NO_PROTO
ProcessWidgetNode (w)
	Widget w;
	XmFontList font;
	int font_mask;
#else
ProcessWidgetNode (
	Widget w,
	XmFontList font,
	int font_mask)
#endif
{
	DynamicFontListClassExtension ext = GetFontListExt(w);

	if (ext)  {
		Arg args[5];
		XtPointer data = NULL;

		XFindContext(XtDisplayOfObject(w), (XID)w, ext->fonts_used, (XtPointer)&data);

		/* Only change widgets that use this font */
		if ((int)data & font_mask)  {
			XtSetArg(args[0], XmNfontList, font);
			XtSetArg(args[1], XmNbuttonFontList, font);
			XtSetArg(args[2], XmNdefaultFontList, font);
			XtSetArg(args[3], XmNlabelFontList, font);
			XtSetArg(args[4], XmNtextFontList, font);
                        XtSetValues(w, args, 5);
		}
        }

        if (XtIsComposite(w)) {
        	int i;
                /* traverse down the tree */
                WidgetList child = ((CompositeWidget)w)->composite.children;

                for (i=((CompositeWidget)w)->composite.num_children; i > 0;
                         i--,child++) {
                        ProcessWidgetNode(*child, font, font_mask);
                }
        }
} /* ProcessWidgetNode() */


static void
#ifdef _NO_PROTO
CheckForFontListArg (w, args, num_args)
	Widget w;
	ArgList args;
	Cardinal * num_args;
#else
CheckForFontListArg (
	Widget w,
	ArgList args,
	Cardinal * num_args)
#endif
{
	int i;

	for (i = 0; i < *num_args; i++)  {
		if (strcmp(args[i].name, XmNfontList) == 0 ||
			strcmp(args[i].name, XmNbuttonFontList) == 0 ||
			strcmp(args[i].name, XmNdefaultFontList) == 0 ||
			strcmp(args[i].name, XmNlabelFontList) == 0 ||
			strcmp(args[i].name, XmNtextFontList) == 0) {
			/*  add this widget to the fonts_used list */
			DynamicFontListClassExtension ext = GetFontListExt(w);
			if (ext) {
				int data;
				FontObject fo = (FontObject) XtNameToWidget(
					(Widget)XmGetXmDisplay(XtDisplayOfObject(w)),
					"fontObject");
				if ((XmFontList)args[i].value == fo->font.serif)
					data = SERIF_MASK;
				else if ((XmFontList)args[i].value == fo->font.sans_serif)
					data = SANS_SERIF_MASK;
				else if ((XmFontList)args[i].value == fo->font.mono)
					data = MONO_MASK;
				else {
					XDeleteContext(XtDisplayOfObject(w),
						(XID)w, ext->fonts_used);
					return;
				}
				XSaveContext(XtDisplayOfObject(w), (XID)w,
					ext->fonts_used, (XtPointer)data);
			}
			break;
		}
	}
}  /* end of CheckForFontListArg */

static void 
#ifdef _NO_PROTO
InitializeWrapper( rq, nw, Args, numArgs )
        Widget rq ;
        Widget nw ;
        ArgList Args ;
        Cardinal *numArgs ;
#else
InitializeWrapper(
        Widget rq,
        Widget nw,
        ArgList Args,
        Cardinal *numArgs)
#endif
{
	DynamicFontListClassExtension ext = GetFontListExt(nw);
	XmFontList font;
	Arg arg[1];
	int i;

	/* Most initialize functions copy the default font, so compare the
	   XmFontList pointers before calling initialize.  This covers the
	   widgets that use the FontFamily values in resource files.  */
	for (i = 0; i < *numArgs; i++) {
	 if (!strcmp(Args[i].name, XmNfontList) ||
		!strcmp(Args[i].name, XmNbuttonFontList) ||
		!strcmp(Args[i].name, XmNdefaultFontList) ||
		!strcmp(Args[i].name, XmNlabelFontList) ||
		!strcmp(Args[i].name, XmNtextFontList)) {
		int data = 0;
		FontObject fo = (FontObject) XtNameToWidget(
			(Widget)XmGetXmDisplay(XtDisplayOfObject(rq)), "fontObject");
		font = (XmFontList)Args[i].value;
		if (font == fo->font.serif)
			data = SERIF_MASK;
		else if (font == fo->font.sans_serif)
			data = SANS_SERIF_MASK;
		else if (font == fo->font.mono)
			data = MONO_MASK;
		if (data)
			XSaveContext(XtDisplayOfObject(nw), (XID)nw,
				ext->fonts_used, (XtPointer)data);
		break;
         }
	}
	if (ext && ext->initialize)
		(*ext->initialize)(rq, nw, Args, numArgs);
	return;
}  /* end of InitializeWrapper() */

static Boolean
#ifdef _NO_PROTO
SetValuesWrapper(current, request, nw, args, num_args)
	Widget current;
	Widget request;
	Widget nw;
	ArgList args;
	Cardinal * num_args;
#else
SetValuesWrapper(
	Widget current,
	Widget request,
	Widget nw,
	ArgList args,
	Cardinal * num_args)
#endif /* _NO_PROTO */
{
	DynamicFontListClassExtension ext = GetFontListExt(nw);
	Boolean ret = False;

	CheckForFontListArg(nw, args, num_args);
	if (ext && ext->set_values)
		ret = (*ext->set_values)(current, request, nw, args, num_args);
	return(ret);
}  /* end of SetValuesWrapper() */

static void
#ifdef _NO_PROTO
DestroyWrapper(w)
	Widget w;
#else
DestroyWrapper(
	Widget w)
#endif
{
	DynamicFontListClassExtension ext = GetFontListExt(w);

	XDeleteContext(XtDisplayOfObject(w), (XID)w, ext->fonts_used);
	if (ext && ext->destroy)
		(*ext->destroy)(w);
	return;
}  /* end of DestroyWrapper() */

static void
#ifdef _NO_PROTO
ShellInitializeWrapper(rq, nw, Args, numArgs)
	Widget rq;
	Widget nw;
	ArgList Args;
	Cardinal *numArgs;
#else
ShellInitializeWrapper(
	Widget rq,
	Widget nw,
	ArgList Args,
	Cardinal *numArgs)
#endif
{
	static int in_wrapper = 0;

	if (in_wrapper)
		return;
	
	in_wrapper = 1;
	AddToShellList (nw);
	if (FONT_C(_xmFontObjectClass).shell_initialize)
		(*FONT_C(_xmFontObjectClass).shell_initialize)(rq, nw, Args, numArgs);

	in_wrapper = 0;
	return;
}  /* end of ShellInitializeWrapper */

XmFontList
#ifdef _NO_PROTO
_XmFontObjectGetDefaultFontList(w, font)
	Widget w;
	XmFontList font;
#else
_XmFontObjectGetDefaultFontList(
	Widget w,
	XmFontList font)
#endif
{
	XmFontList new_font = font;
       	FontObject fo = (FontObject)XtNameToWidget((Widget)XmGetXmDisplay(
			XtDisplayOfObject(w)), "fontObject");

	if (!new_font) {
        	if (fo) {
			if (fo->font.sans_serif == NULL) {
				Arg args[1];

				XtSetArg(args[0], XmNsansSerifFamilyFontList,
					&new_font);
				XtGetValues((Widget)fo, args, 1);
			}
			else 
				new_font = fo->font.sans_serif;
		}
        }
	if (new_font) {
		DynamicFontListClassExtension ext = GetFontListExt(w);
		if (ext == NULL)
			return(new_font);
		if (new_font == fo->font.sans_serif)
			XSaveContext(XtDisplayOfObject(w), (XID)w,
			ext->fonts_used, (XtPointer)SANS_SERIF_MASK);
		else if (new_font == fo->font.mono)
			XSaveContext(XtDisplayOfObject(w), (XID)w,
			ext->fonts_used, (XtPointer)MONO_MASK);
		else if (new_font == fo->font.serif)
			XSaveContext(XtDisplayOfObject(w), (XID)w,
			ext->fonts_used, (XtPointer)SERIF_MASK);
	}
	return(new_font);
}  /* end of _XmFontObjectGetDefaultFontList() */
#endif /* USE_FONT_OBJECT */
