#ifndef NOIDENT
#ident	"@(#)menu:MenuShell.c	1.95"
#endif

/*
 * MenuShell.c
 *
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>

#include <Xol/OpenLookP.h>
#include <Xol/OlCursors.h>
#include <Xol/LayoutExtP.h>
#include <Xol/HandlesExP.h>
#include <Xol/VendorI.h>
#include <Xol/PopupMenuP.h>
#include <Xol/FButtonsP.h>
#include <Xol/OlClients.h>
#include <Xol/Olg.h>

#define ClassName MenuShell
#include <Xol/NameDefs.h>

#if !defined(XlibSpecificationRelease)	/* symbol is introduced in X11R5 */
typedef char *XPointer;
#endif

#ifdef DEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x)
#endif

#define ABS_DELTA(a,b)  ((a) < (b) ? (b) - (a) : (a) - (b))

#define NULL_WIDGET	((Widget)NULL)
#define NPART(nw,field) ((nw)-> popup_menu_shell.field)
#define HAS_PUSHPIN(nw) ((_OlGetVendorPartExtension((Widget)nw))->pushpin != OL_NONE)
#define HAS_TITLE(nw)   (((PopupMenuShellWidget)(nw))-> wm.title != NULL && strcmp(((PopupMenuShellWidget)nw)->wm.title, " "))
#define TITLE_HEIGHT(nw) _OlMax(NPART(nw,pin.height), NPART(nw,title.height))

#define Width(W,P) OlScreenPointToPixel(OL_HORIZONTAL,(P), XtScreen(W))
#define Height(W,P) OlScreenPointToPixel(OL_VERTICAL,(P), XtScreen(W))

static char *	GetPartAddress OL_ARGS((Widget, Boolean, _OlDynResourceList));
static void	GetStayupInfo OL_ARGS((Widget,
				       Position *, Position *, Window *));


static void	MSSetDefault OL_ARGS((Widget, Widget));
static void	MSGetDefault OL_ARGS((Widget, Widget));

static void	MSGetGC OL_ARGS((PopupMenuShellWidget, PopupMenuShellPart *));

static void 	(*_olmMSCreateButton) OL_ARGS((Widget));

static void	MSLayout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));

static void	(*_olmMSLayoutPreferred) OL_ARGS((
	Widget			w,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Dimension *		width,
	Dimension *		height,
	Position *		xpos,
	Position *		ypos,
	Dimension *		child_space
));

static void	MSDestroy OL_ARGS((Widget));

static Boolean	MSActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));

static void	MSClassInitialize OL_NO_ARGS();

static void	MSRealize OL_ARGS((Widget, XtValueMask *,XSetWindowAttributes *));

static void	MSClassPartInitialize OL_ARGS((WidgetClass));

static void	InsertChild OL_ARGS((Widget));

static void	MSSetFocus OL_ARGS((Widget, Time));

static void	(*_olmMSHandleTitle) OL_ARGS((PopupMenuShellWidget,
				       PopupMenuShellPart *, char));

static void	(*_olmMSHandlePushpin) OL_ARGS((PopupMenuShellWidget,
				         PopupMenuShellPart *, OlDefine, char));

static void	PinPutIn OL_ARGS((PopupMenuShellWidget));

static Widget	MSTraversalHandler OL_ARGS((Widget, Widget, OlDefine, Time));

static void	MSKeyHandler OL_ARGS((Widget, OlVirtualEvent));

static void	MSButtonHandler OL_ARGS((Widget, OlVirtualEvent));

static void	MSShellButtonHandler OL_ARGS((Widget, XtPointer, XEvent *,
					      Boolean *));

static void	MSInitialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	MSRedisplay OL_ARGS((Widget, XEvent *, Region));

static Boolean	MSSetValues OL_ARGS((Widget, Widget, Widget,
				     ArgList, Cardinal *));

static void	WMMessageHandler();
static void	PopdownLayout OL_ARGS((Widget));

static void PopupMenuEH OL_ARGS((
	Widget    w,
	XtPointer client_data,
	XEvent *  xevent,
	Boolean * cont_to_dispatch
));

static void	SetStayupMode OL_ARGS((Widget));

#define FG_BIT		(1<<0)
#define FT_BIT		(1<<1)
#define FONT_BIT		(1<<2)
#define FONT_LIST_BIT		(1<<3)

#define BYTE_OFFSET	XtOffsetOf(OlVendorPartExtensionRec, dyn_flags)

static _OlDynResource dyn_res[] = {
   { { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
       XtRString, XtDefaultForeground }, BYTE_OFFSET, FG_BIT, GetPartAddress },
   { { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
       XtRString, XtDefaultForeground }, BYTE_OFFSET, FT_BIT, GetPartAddress },
   { { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *), 0,
       XtRString, OlDefaultBoldFont }, BYTE_OFFSET, FONT_BIT, GetPartAddress },
   { { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *), 0,
    XtRString, (XtPointer)NULL }, BYTE_OFFSET, FONT_LIST_BIT, GetPartAddress },
};

#undef BYTE_OFFSET

static OlEventHandlerRec MenuEvents[] = {
   { ButtonPress,        MSButtonHandler       },
   { ButtonRelease,      MSButtonHandler       },
   { KeyPress,           MSKeyHandler          }
   };

#define ShOFFSET(field) XtOffsetOf(PopupMenuShellRec, shell.field)
#define MuOFFSET(field) XtOffsetOf(PopupMenuShellRec, popup_menu_shell.field)

static XtResource resources[] = {
    { XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
	ShOFFSET(allow_shell_resize), XtRImmediate, (XtPointer)True },

    { XtNsaveUnder, XtCSaveUnder, XtRBoolean, sizeof(Boolean),
	ShOFFSET(save_under), XtRImmediate, (XtPointer)True },

    { XtNoverrideRedirect, XtCOverrideRedirect, XtRBoolean, sizeof(Boolean),
	ShOFFSET(override_redirect), XtRImmediate, (XtPointer)True },

    /* defaults for own resources */
    { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
	MuOFFSET(pushpinDefault), XtRImmediate, (XtPointer)False },

    { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        MuOFFSET(font), XtRString, (XtPointer) OlDefaultBoldFont },

    { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
        MuOFFSET(fontColor), XtRString, XtDefaultForeground },

    { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
	MuOFFSET(font_list), XtRString, (XtPointer)NULL },

    { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
        MuOFFSET(foreground), XtRString, XtDefaultForeground },

    { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
        MuOFFSET(shadow_thickness), XtRString, (XtPointer)"2 points" },

    { XtNoptionMenu, XtCOptionMenu, XtRBoolean, sizeof(Boolean),
	MuOFFSET(option_menu), XtRImmediate, (XtPointer)False },

    { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
        XtOffset(PopupMenuShellWidget, core.border_width),
        XtRImmediate, (XtPointer)0 },

    { XtNtitle, XtCTitle, XtRString, sizeof(String),
	XtOffset(PopupMenuShellWidget, wm.title), XtRString, (XtPointer)" " },

};
#undef MuOFFSET
#undef ShOFFSET

/*  See the Vendor.c transient shell's resource list for all other defaults. */

#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
        { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
          OFFSET(menu_button), XtRImmediate, (XtPointer)False },

        { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
          OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

        { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
          OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

        { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
          OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

        { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
          OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_CMD }
};

#undef OFFSET

static LayoutCoreClassExtensionRec layout_extension_rec = {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlLayoutCoreClassExtensionVersion,      /* version              */
        sizeof(LayoutCoreClassExtensionRec),    /* record_size          */
	MSLayout,				/* layout		*/
	NULL					/* query_alignment	*/
};

static OlVendorClassExtensionRec vendor_extension_rec = {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,          	        /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        MSSetDefault,                           /* set_default          */
	MSGetDefault,				/* get_default 		*/
	NULL,					/* destroy		*/
	NULL,					/* initialize		*/
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        MSTraversalHandler,                     /* traversal handler    */
        XtInheritHighlightHandler,    		/* highlight handler    */
        MSActivateWidget,			/* activate function    */
        MenuEvents,            			/* event_procs          */
        XtNumber(MenuEvents),  			/* num_event_procs      */
        NULL,					/* part_list            */
	{ dyn_res, XtNumber(dyn_res) },		/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	XtInheritWMProtocolProc,		/* wm_proc		*/
	FALSE,					/* override_callback	*/
};
OlVendorClassExtensionRec *vendor_extension = &vendor_extension_rec;

static
CompositeClassExtensionRec	compositeClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */            NULLQUARK,
/* version              */            XtCompositeExtensionVersion,
/* record_size          */            sizeof(CompositeClassExtensionRec),
/* accepts_objects      */            True
};

PopupMenuShellClassRec popupMenuShellClassRec = {
  { /* core_class fields	*/
    /* superclass		*/	(WidgetClass) &transientShellClassRec,
    /* class_name		*/	"PopupMenuShell",
    /* widget_size		*/	sizeof(PopupMenuShellRec),
    /* class_initialize		*/	MSClassInitialize,
    /* class_part_init		*/	MSClassPartInitialize,
    /* class_inited		*/	FALSE,
    /* initialize		*/	MSInitialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	MSRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	MSDestroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	MSRedisplay,
    /* set_values		*/	MSSetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	XtInheritAcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	XtInheritTranslations,
    /* query geometry		*/	XtInheritQueryGeometry,
    /* display_accelerator      */      XtInheritDisplayAccelerator,
    /* extension                */      (XtPointer)&layout_extension_rec
  },{ /* composite_class fields	*/
    /* geometry_manager         */      XtInheritGeometryManager,
    /* change_managed           */      _OlDefaultChangeManaged,
    /* insert_child		*/	InsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension                */      (XtPointer)&compositeClassExtension
  },{ /* Shell fields       	*/
					NULL
  },{ /* WMShell fields		*/
					NULL
  },{ /* VendorShell fields	*/
					(XtPointer)&vendor_extension_rec
  },{ /* TransientShell fields	*/
					NULL
  },{ /* PopupMenuShell fields	*/
					NULL
  }
};

WidgetClass popupMenuShellWidgetClass = (WidgetClass)&popupMenuShellClassRec;


/*
 * GetPartAddress - Called by the dynamic resource routines.  This routine
 * returns the base address of where the dynamic resource flags live.
 *
 * Note that this routine is from Menu.c
 */
/*ARGSUSED*/
static char *
GetPartAddress OLARGLIST((w, init, res))
	OLARG( Widget,			w)
	OLARG( Boolean,			init)
	OLGRA( _OlDynResourceList,	res)
{
	/*
	 * Normally, you would need to check the init flag and may
	 * need to allocate the extension since this procedure is called
	 * before the subclass's Initialize routine.  But in our case,
	 * the vendor superclass allocates the extension before
	 * handling the dynamic resources.  Therefore, we're guaranteed that
	 * our extension is already allocated.
	 */

	return((char *) _OlGetVendorPartExtension(w));
} /* end of GetPartAddress */


/*
 *************************************************************************
 * MSGetDefault - This routine is called by the convenience routine
 * _OlGetDefault.  If the menu has no default this routine attempts to
 * pick one.
 ****************************procedure*header*****************************
 */
static void
MSGetDefault OLARGLIST((menu_widget, new_default))
	OLARG( Widget,  menu_widget) 
	OLGRA( Widget,  new_default)
{
	if (new_default == (Widget)NULL) {
		MSSetDefault(menu_widget, NULL_WIDGET);
	}
} /* END OF MSGetDefault() */

/*
 * MSSetDefault
 *
 */
static void
MSSetDefault OLARGLIST((menu_widget, new_default))
	OLARG( Widget,  menu_widget)
	OLGRA( Widget,  new_default)
{
	PopupMenuShellWidget w = (PopupMenuShellWidget) menu_widget;
	PopupMenuShellPart *	nPart =
		&(((PopupMenuShellWidget)menu_widget)->popup_menu_shell);
	OlVendorPartExtension  vendor = _OlGetVendorPartExtension(menu_widget);

	/*  This function gets called after a the old default has been
	    setvalued to False.   There are three possibilities:
	    A NULL_WIDGET, the menu shell, or a child widget.  The function
	    only has to set the default for the NULL_WIDGET case since
	    a menu shell always has a default.  */

	if (new_default != NULL_WIDGET) {
		if (vendor->pushpin != OL_NONE)  {
			/*  The _OlSetDefault doesn't actually set the XtNvalue
		    	it just unsets the old default.  As a convenience,	
		    	put the setting of the pushpin here and in the rest
		    	of the code, just call _OlSetDefault(menu, True).*/
			if (new_default == menu_widget)
				nPart->pushpinDefault = True;
			else
				nPart->pushpinDefault = False;

     			(*_olmMSHandlePushpin)(w, nPart, vendor->pushpin, 'd');
		}
		return;
	}

		/* If there's a pushpin on this menu, make it
		 * the default; else, search the pane for the
		 * first eligible child.			*/

	if (vendor->pushpin != OL_NONE)  {
		nPart->pushpinDefault = True;
     		(*_olmMSHandlePushpin)(w, nPart, vendor->pushpin, 'd');
		vendor->default_widget = menu_widget;
		return;
	}
	else {
		Cardinal	num_kids = w->composite.num_children;
		WidgetList	kids = w->composite.children;
		Arg		args[1];

		XtSetArg(args[0], XtNdefault, True);

		nPart->pushpinDefault = False;

		for (; num_kids != 0; --num_kids, ++kids) {
			if (XtIsManaged(*kids)) {
				if (XtIsSubclass(*kids, flatWidgetClass)) {
					OlFlatSetValues((*kids), 0, args, 1);
				}
				else
					XtSetValues(*kids, args, 1);
				vendor->default_widget = *kids;
				break;
			}
		}
	}
} /* end of MSSetDefault */

/*
 * MSGetGC
 *
 */
static void
MSGetGC OLARGLIST((nw, nPart))
	OLARG( PopupMenuShellWidget, nw)
	OLGRA( PopupMenuShellPart *, nPart)
{
  XGCValues values;

  values.font = nPart-> font-> fid;
  values.foreground = nPart-> fontColor;
  values.background = nw-> core.background_pixel;
  values.graphics_exposures = False;

  if (nPart-> gc)
     XtReleaseGC((Widget)nw, nPart-> gc);

  nPart->gc = XtGetGC((Widget) nw, GCForeground | GCBackground | GCFont | GCGraphicsExposures, &values);
} /* end of MSGetGC */

/*
 * MSClassInitialize
 *
 */
static void
MSClassInitialize OL_NO_ARGS()
{
	vendor_extension-> header.record_type = OlXrmVendorClassExtension;
	layout_extension_rec.record_type = XtQLayoutCoreClassExtension;

	_OlAddOlDefineType("none",	OL_NONE);
	_OlAddOlDefineType("in",	OL_IN);
	_OlAddOlDefineType("out",	OL_OUT);
	_OlAddOlDefineType("busy",	OL_BUSY);

	/*  Resolve GUI dependent routines.  */
        OLRESOLVESTART
        OLRESOLVE(MSCreateButton, _olmMSCreateButton)
        OLRESOLVE(MSHandlePushpin, _olmMSHandlePushpin)
        OLRESOLVE(MSHandleTitle, _olmMSHandleTitle)
        OLRESOLVEEND(MSLayoutPreferred, _olmMSLayoutPreferred)

} /* end of MSClassInitialize */

/*************************************************************************
 * MSClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
MSClassPartInitialize OLARGLIST((wc))
	OLGRA( WidgetClass,		wc)
{
	OlClassExtension	ext;

	/*
	 * By default all subclasses accept objects. If a subclass
	 * chooses not to, it should define the CompositeExtension
	 * with accepts_objects False (or change the accepts_objects
	 * field or delete the extension in class_part_initialize).
	 */
	ext = _OlGetClassExtension(
		(OlClassExtension)&COMPOSITE_C(wc).extension,
		NULLQUARK, XtCompositeExtensionVersion
	);
	if (!ext) {
		ext = (OlClassExtension)
			XtMalloc(compositeClassExtension.record_size);
		memcpy (
			(XtPointer)ext, &compositeClassExtension,
			compositeClassExtension.record_size
		);
		ext->next_extension = COMPOSITE_C(wc).extension;
		COMPOSITE_C(wc).extension = (XtPointer)ext;
	}
}  /*  end of MSClassPartInitialize() */


/*
 * MSInitialize
 *
 */
/* ARGSUSED */
static void
MSInitialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	PopupMenuShellWidget nw         = (PopupMenuShellWidget)new;
	PopupMenuShellPart * nPart      = &(nw->popup_menu_shell);

	XtAddEventHandler(
		new, EnterWindowMask | LeaveWindowMask | ButtonMotionMask,
		False, MSShellButtonHandler, NULL
	);
	   
	nPart->fake_window = (Window)NULL;
	nPart->attrs = NULL;
	nPart->attrs = OlgCreateAttrs(
				XtScreen(new),
				nPart->foreground,
				(OlgBG *)&(new->core.background_pixel),
				False, OL_DEFAULT_POINT_SIZE
	);

		/* See Primitive:Initialize() for explanation... */
	if (nw->popup_menu_shell.font == NULL)  {
		_OlLoadDefaultFont(new, nw->popup_menu_shell.font);
	}

	nPart->gc = NULL;
	nPart->cursor = None;
	MSGetGC(nw, nPart);

	/*  Initialize the size of the pushpin and title.  */
	if (HAS_PUSHPIN(nw)) {
		(*_olmMSHandlePushpin)(nw, nPart, OL_OUT, 's');
	}
	else {
		nPart-> pin.x = 0;
		nPart-> pin.y = 0;
		nPart-> pin.height = 0;
		nPart-> pin.width  = 0;
	}

	if (HAS_TITLE(nw)) {
		(*_olmMSHandleTitle)(nw, nPart, 's');
	}
	else {
		nPart-> title.x = 0;
		nPart-> title.y = 0;
		nPart-> title.height = 0;
		nPart-> title.width  = 0;
	}

			/* Add an event handler to catch client messages
			 * from the window manager concerning
			 * WM protocol.					*/
	XtAddEventHandler(new, (EventMask)PropertyChangeMask,
			True, WMMessageHandler, (XtPointer) NULL);

			/* If the parent wishes us to add Event handlers
			 * for it, do so.				*/

	if (HAS_PUSHPIN(nw))  {
		(*_olmMSCreateButton)(new);
	}

} /* end of MSInitialize */

static void
MSRealize OLARGLIST((w, value_mask, attributes))
	OLARG( Widget,			w)
	OLARG( XtValueMask *,		value_mask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
	XtRealizeProc		super_realize;

			/* Use the superclass to create the window	*/

	super_realize = popupMenuShellWidgetClass->core_class.
				superclass->core_class.realize;

	/*  Create and set the cursor to the right arrow.  */
	if (OlGetGui() == OL_MOTIF_GUI)  {
		((PopupMenuShellWidget)w)->popup_menu_shell.cursor =
			attributes->cursor =
			XCreateFontCursor(XtDisplay(w), XC_right_ptr);
		*value_mask |= CWCursor;
	}

	if (super_realize != (XtRealizeProc)NULL) {
		(*super_realize)(w, value_mask, attributes);
	} 

}  /* end of MSRealize() */

/**
 ** InsertChild()
 **/

static void
InsertChild OLARGLIST((w))
	OLGRA(Widget,			w)
{
	if (!XtIsSubclass(w, flatButtonsWidgetClass))  {
		XtVaSetValues(w, XtNshellBehavior, UnpinnedMenu, (String)0);
	}
	(void) _OlDefaultInsertChild(w);
	return;
} /* InsertChild */

/**
/*
 * MSDestroy
 *
 */
static void
MSDestroy OLARGLIST((w))
	OLGRA(Widget,	w)
{
	PopupMenuShellWidget nw = (PopupMenuShellWidget)w;

	if (nw->popup_menu_shell.gc)
		XtReleaseGC(w, nw->popup_menu_shell.gc);

	if (NPART(nw, fake_window))
		XDestroyWindow(XtDisplay(nw), NPART(nw, fake_window));

	if (nw->popup_menu_shell.attrs)
		OlgDestroyAttrs (nw->popup_menu_shell.attrs);

	if (nw->popup_menu_shell.cursor != None)
		XFreeCursor(XtDisplay(w), nw->popup_menu_shell.cursor);

				/* Remove the WMMessage Handler		*/
	XtRemoveEventHandler(w, (EventMask)PropertyChangeMask, True,
		     WMMessageHandler, (XtPointer)NULL);

} /* end of MSDestroy */


/**
 ** MSLayout()
 **
 ** Layout the managed children.
 **/

/*ARGSUSED*/
static void
MSLayout OLARGLIST((w, resizable, query_only, cached_best_fit_hint,
	who_asking, request, response))
	OLARG(Widget,			w)
	OLARG(Boolean,			resizable)
	OLARG(Boolean,			query_only)
	OLARG(Boolean,			cached_best_fit_hint)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLGRA(XtWidgetGeometry *,	response)
{
	Cardinal		nchildren    = ((CompositeWidget)w)->composite.num_children;
	Cardinal		n;

	Widget *		pchild	     = ((CompositeWidget)w)->composite.children;
	Widget *		p;

	Dimension		child_width;
	Dimension		child_space;

	Position		xpos;
	Position		ypos;
	XtWidgetGeometry	available;
	XtWidgetGeometry	preferred;

	/*  Get the preferred size of the menu shell based on the GUI.  */
	(*_olmMSLayoutPreferred)(w, who_asking, request, &preferred.width,
		&preferred.height, &xpos, &ypos, &child_space);

	/*  Protect against the zero size protocol error.  */
	if (preferred.width == 0)
		preferred.width = 1;
	if (preferred.height == 0)
		preferred.height = 1;

	/*  determine the available geometry according to the flags given. */
	OlAvailableGeometry(w, resizable, query_only, who_asking, request, 
		&preferred, &available);

	if (who_asking == w) {
		if (response) {
			response->x            = w->core.x;
			response->y            = w->core.y;
			response->width        = available.width;
			response->height       = available.height;
			response->border_width = w->core.border_width;
		}
		return;
	}

	/*
	 * Adjust the size and/or position of each child.  The current core
	 *  width and height must be used.
	 */
	child_width = available.width - (2*xpos);

	for (p = pchild, n = 0; n < nchildren; n++, p++) {
		if (XtIsManaged(*p))  {
			Dimension		bw = (*p)->core.border_width;
			Dimension		h = (*p)->core.height;

			if (*p != who_asking && !query_only) {
				int measure = 0;
				OlDefine layoutType;

				/*  Make the menu item the calculated width. */
				/*  Must do this before the SetValues to avoid
				    a recursive Layout...  */
					XtConfigureWidget (
						*p,
						xpos,
						ypos,
						child_width,
						h,
						bw
					);

				/*  The FlatButtons do not span the width
				    of a widget, so set the min and max width
				    to the width of the widget.  This is going
				    to work only when the measure is 1.  */
				
				XtVaGetValues(*p, XtNmeasure, &measure,
					XtNlayoutType, &layoutType,
					(String)0);
				if (measure == 1 && layoutType == OL_FIXEDCOLS)  {
					XtVaSetValues(*p,
						XtNitemMinWidth, child_width,
						(String) 0);
				}

				/*  Add the height of the managed child to
				    the ypos. */
				ypos += (Position) h;

			} else if (who_asking == *p) {
				if (response)  {
					/*  This is the response to a QueryOnly
					    MakeGeometryRequest  */
					response->x = xpos;
					response->y = ypos;
					response->width  = child_width;
					response->height = request->height;
					response->border_width = bw;
					ypos += (Position) request->height;
				}

				if (query_only)
					return;
			}

			/*  Add the inter-item space.  */
			ypos += (Position) child_space;
		}
	}

	return;
} /* end of MSLayout() */

/*
 * MSActivateWidget
 *
 */
/* ARGSUSED */
static Boolean
MSActivateWidget OLARGLIST((w, type, data))
	OLARG( Widget,        w)
	OLARG( OlVirtualName, type)
	OLGRA( XtPointer,     data)
{
  PopupMenuShellWidget     nw      = (PopupMenuShellWidget)w;
  Boolean               ret_val = FALSE;
  FlatButtonsWidget	parent;
  Widget                popdown;
  Widget		dft_widget;

  switch(type)
   {
   case OL_SELECTKEY:
      if (_OlGetVendorPartExtension(w)->pushpin != OL_NONE)
		PinPutIn(nw);
      break;
   case OL_DEFAULTACTION:
      ret_val = True;
      if ((dft_widget = _OlGetDefault(w)) != (Widget)NULL) {
	(void)OlActivateWidget(dft_widget, OL_SELECTKEY, (XtPointer)OL_NO_ITEM);
      }
      break;
   case OL_TOGGLEPUSHPIN:
      ret_val = True;
      FPRINTF((stderr,"toggle pushpin\n"));
	if (nw->popup_menu_shell.pin.width == 0)
		/*  For some undetermined reason, the state of the pushpin is
		    being set to OL_OUT when it started out as OL_NONE.  This
		    is a hack to work around the problem.  Note that Motif mode
		    sets pin.width to 1 when it has a StayUp button. Yuk.  */
		break;
      switch((_OlGetVendorPartExtension(w))->pushpin)
         {
	 case OL_IN:
		PopdownLayout(w);
		break;
         case OL_OUT:
		PinPutIn(nw);
		break;
         default:
		break;
         }
      break;

   case OL_MOVERIGHT:
   case OL_MOVELEFT:
      if (_OlIsEmptyMenuStack(w))
         return (False);

      if (type == OL_MOVERIGHT)
         {
         popdown = _OlRootOfMenuStack(w);
         parent = (FlatButtonsWidget)_OlRootParentOfMenuStack(w);
         }
      else
         {
         popdown = w;
         parent = (FlatButtonsWidget)_OlTopParentOfMenuStack(w);
         }
      if (parent == (FlatButtonsWidget)_OlRootOfMenuStack(w)) {
	 Widget	root = _OlRootOfMenuStack(w);

	 _OlFBResetParentSetItem(w, False);

		/* As I Indicated in _OlRoot1OfMenuStack, if the
		 * stack is created because of the flat buttons
		 * then the root is a flat buttons not a shell widget,
		 * so is the check, otherwise focus is landing in
		 * wrong place...
		 *
		 * I have to give focus to someone before menu is
		 * done because menu is poking the event queue and
		 * settting the input focus, and meanwhile olwm sees
		 * focusIn in root window and send a take focus message
		 * to the base window that has focus before but use
		 * ?CurrentTime? as timestamp...
		 */
	 if (XtIsShell(root))
		 MSSetFocus(root, (Time)data);
	 else
		 OlSetInputFocus(root, RevertToNone, (Time)data);

         _OlPopdownCascade(root, False);
       }
      else
         {
	_OlFBResetParentSetItem(w, False);
         MSSetFocus((Widget)parent, (Time)data);
         _OlPopdownCascade(popdown, False);
         }
      break;
   case OL_CANCEL:
	ret_val = True;
	if((_OlGetVendorPartExtension(w))->pushpin == OL_IN)  {
		XtVaSetValues(w, XtNpushpin, OL_OUT, (String)0);
	}
	else if ((w = _OlRootOfMenuStack(w)) != NULL) {
		_OlFBResetParentSetItem(w, True);
		_OlPopdownCascade(w, False);
	}
      break;
   default:
      break;
   }

  return(ret_val);
} /* end of MSActivateWidget */

/*
 * MSSetFocus
 *
 */
static void
MSSetFocus OLARGLIST((w, time))
	OLARG( Widget,	w)
	OLGRA( Time,    time)
{
     OlVendorPartExtension   part;
     OlFocusData *           fd = _OlGetFocusData(w, &part);
     WidgetArray *           list = &(fd->traversal_list);
     Widget                  the_default;
     OlTraversalFunc         func;
     Time		     timestamp = time;

     if (!XtIsSubclass(w, popupMenuShellWidgetClass))
     {
		XtCallAcceptFocus(w, &timestamp);
		return;
     }

     if ( ((PopupMenuShellWidget)w)->popup_menu_shell.option_menu &&
	  fd->initial_focus_widget != NULL_WIDGET &&
	  fd->initial_focus_widget != w &&
	  OlGetGui() == OL_MOTIF_GUI &&
	  XtCallAcceptFocus(fd->initial_focus_widget, &timestamp) == True )
     {
		return;
     }

     if ((the_default = _OlGetDefault(w)) != NULL &&
           XtCallAcceptFocus(the_default, &timestamp) == True)
        {
        FPRINTF((stderr,"to the default\n"));
        return;
        }

     if (_OlArraySize(list) > (Cardinal)0 &&
         (func = part->class_extension->traversal_handler) &&
         (*func)(w, (Widget)_OlArrayElement(list, 0), OL_IMMEDIATE,
         timestamp) != NULL)
        {
        FPRINTF((stderr,"to the traversal list\n"));
        return;
        }

       /* No default widget and no descendent wants
        * focus, so set it to the menu.
        */
     (void)XtCallAcceptFocus(w, &timestamp);
     FPRINTF((stderr,"to the shell\n"));
} /* end of MSSetFocus */

static void
MSSetPinDefault OLARGLIST((w))
	OLGRA(PopupMenuShellWidget, w)
{
	if  ((_OlGetVendorPartExtension((Widget)w)->pushpin == OL_IN) &&
			!NPART(w, pushpinDefault)) {
		Arg arg[1];
		Widget child;
		Cardinal default_index;

		_OlGetMenuDefault((Widget)w, &child, &default_index, False);
		XtSetArg(arg[0], XtNdefault, False);
		OlFlatSetValues(child, default_index, arg, 1);
		NPART(w, pushpinDefault) = True;
	}
}  /* end of MSSetPinDefault() */

/*
 * MSTraversalHandler
 *
 */
static Widget
MSTraversalHandler OLARGLIST((shell, w, direction, time))
	OLARG( Widget,   shell)
	OLARG( Widget,   w)
	OLARG( OlDefine, direction)
	OLGRA( Time,     time)
{
  FPRINTF((stderr,"Traversal handler in shell called\n"));

  switch (direction)
   {
   case OL_MOVELEFT:
   case OL_MULTILEFT:
      if (_OlIsInStayupMode(shell) && _OlIsInMenuStack(shell))
         _OlPopdownCascade(shell, False);
      break;
   default:
      {
      OlVendorClassExtension ext = 
         _OlGetVendorClassExtension(vendorShellWidgetClass);
      (*ext-> traversal_handler)(shell, w, direction, time);
      }
      break;
   }

  /*
   * Note will this be used???
   */
  return (Widget)NULL;
} /* end of MSTraversalHandler */

/*
 * MSKeyHandler
 *
 */
static void
MenubarTakeFocusEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
#define DPY	xevent->xany.display

	if (xevent->type == ClientMessage &&
	    xevent->xclient.message_type == XA_WM_PROTOCOLS(DPY) &&
	    (Atom)xevent->xclient.data.l[0] == XA_WM_TAKE_FOCUS(DPY))
	{
		Time	time = (Time)xevent->xclient.data.l[1];
		Widget	menubar = (Widget)client_data;

		(void)XtCallAcceptFocus(menubar, &time);
		*cont_to_dispatch = False;
		XtRemoveRawEventHandler(
			w,
			(EventMask)NoEventMask, True,
			MenubarTakeFocusEH, (XtPointer)menubar);
	}
#undef DPY
} /* end of MenubarTakeFocusEH */

static void
NextMenuEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	if (xevent->type == FocusIn)
	{
		OlVirtualName	dir = (OlVirtualName)client_data;

		_OlSetCurrentFocusWidget(w, OL_IN);
		OlMoveFocus(w, dir, XtLastTimestampProcessed(XtDisplay(w)));
		*cont_to_dispatch = False;
		XtRemoveRawEventHandler(
			w, (EventMask)FocusChangeMask, False,
			NextMenuEH, (XtPointer)dir);
	}
} /* end of NextMenuEH */

static void
MSKeyHandler OLARGLIST((w, ve))
	OLARG( Widget,         w)
	OLGRA( OlVirtualEvent, ve)
{
  OlFocusData *        fd;
  Widget               cfw   = (Widget)NULL;
  PopupMenuShellWidget nw    = (PopupMenuShellWidget)w;
  PopupMenuShellPart * nPart = &(nw->popup_menu_shell);

  FPRINTF((stderr,"key handler in shell called\n"));

  /* 
   * Always consume the event
   */

  ve->consumed = True;

  /* If this menu is not the tail of the
   * cascade, ingore the keypress since
   * menu cascades are spring loaded and the
   * tail menu will handle the keypress.
   */

  if (_OlTopOfMenuStack(w) != (Widget)nw)
     {
     FPRINTF((stderr,"key ignored in shell key handler\n"));
     return;
     }

  if (ve->virtual_name == OL_MENUDEFAULTKEY) {
	if (HAS_PUSHPIN(nw))  {
		MSSetPinDefault(nw);
		(*_olmMSHandlePushpin)(nw, nPart,
			_OlGetVendorPartExtension((Widget)nw)->pushpin, 'd');
     		return;
	}
   }

  /*
   * Get the current focus widget within this menu
   */

  if ((fd = _OlGetFocusData(w, NULL)) != (OlFocusData *)NULL)
     {
     cfw = fd->current_focus_widget;
     }

	if (cfw == (Widget)NULL &&
		( ve->virtual_name == OL_CANCEL ||
		  ve->virtual_name == OL_MOVERIGHT ||
		  ve->virtual_name == OL_MULTIRIGHT ||
		  ve->virtual_name == OL_MULTIDOWN ||
		  ve->virtual_name == OL_MOVELEFT ||
		  ve->virtual_name == OL_MULTILEFT ||
		  ve->virtual_name == OL_MULTIUP ))
	{
		Widget	fbtn = _OlRootOfMenuStack(w);
		(void)OlActivateWidget(w, OL_CANCEL, (XtPointer)NULL);
		if ( fbtn != (Widget)NULL && ve->virtual_name != OL_CANCEL &&
			XtIsSubclass(fbtn, flatButtonsWidgetClass) )
		{
			OlVirtualName	dir;

			if (ve->virtual_name == OL_MOVERIGHT ||
			    ve->virtual_name == OL_MULTIRIGHT ||
			    ve->virtual_name == OL_MULTIDOWN )
					dir = OL_MULTIRIGHT;
			else
					dir = OL_MULTILEFT;

			if ( OlGetGui() == OL_MOTIF_GUI &&
			     fbtn == _OlGetMenubarWidget(fbtn) )
			{
				XtInsertRawEventHandler(
					_OlGetShellOfWidget(fbtn),
					(EventMask)NoEventMask, True,
					MenubarTakeFocusEH, (XtPointer)fbtn,
					XtListHead);
			}

			XtInsertRawEventHandler(
				fbtn, (EventMask)FocusChangeMask, False,
				NextMenuEH, (XtPointer)dir, XtListHead);
		}
		return;
	}

  if (cfw == (Widget)NULL || cfw != (Widget)nw)
     {
     return;
     }

/* The following code may be redundant... */
  if (_OlFetchMnemonicOwner(w, (XtPointer *)NULL, ve) != (Widget)NULL ||
      _OlFetchAcceleratorOwner(w, (XtPointer *)NULL, ve) != (Widget)NULL)
     {
     /* If we've matched an accelerator or a mnemonic,
      * let the superclass handle it.
      */
     ve->consumed = False;
     }
  else
     {
     switch(ve->virtual_name)
      {
      case OL_PREVFIELD:
      case OL_NEXTFIELD:
      case OL_MOVERIGHT:
      case OL_MOVEUP:
      case OL_MOVEDOWN:
      case OL_MULTIRIGHT:
      case OL_MULTIUP:
      case OL_MULTIDOWN:
         OlMoveFocus(cfw, ve->virtual_name, ve->xevent->xkey.time);
         break;
      case OL_MOVELEFT:
      case OL_MULTILEFT:
      case OL_TOGGLEPUSHPIN:
ve->consumed=False;
         (void) OlActivateWidget(w, ve->virtual_name, (XtPointer)NULL);
         break;
      default:
         if (ve->virtual_name != OL_UNKNOWN_KEY_INPUT) {
            (void) OlActivateWidget(cfw, ve->virtual_name, (XtPointer)NULL);
            }
         break;
      }
     }
} /* end of MSKeyHandler */

/*
 * MSButtonHandler
 *
 */
static void
MSButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,         w)
	OLGRA( OlVirtualEvent, ve)
{
  Boolean	cont_to_dispatch = True;

	/* always consume the event and also pass in "ve" to avoid	*/
	/* a redundant lookup (OlLookupInputEvent)...			*/
  ve->consumed = True;
  MSShellButtonHandler(w, (XtPointer)ve, ve-> xevent, &cont_to_dispatch);
} /* end of MSButtonHandler */

/*
 * MSShellButtonHandler
 *
 * Note that a 4th parameter (cont_to_dispatch) is added.
 *	(see XtEventHandler in R4)
 */
/* ARGSUSED */
static void
MSShellButtonHandler OLARGLIST((w, client_data, event, cont_to_dispatch))
	OLARG( Widget,    w)
	OLARG( XtPointer, client_data)
	OLARG( XEvent *,  event)
	OLGRA( Boolean *, cont_to_dispatch)
{
  PopupMenuShellWidget nw = (PopupMenuShellWidget)w;
  Widget            widget = XtWindowToWidget(event-> xany.display, 
                                            event-> xany.window);
  Widget            shell = widget ? _OlGetShellOfWidget(widget) : NULL;
  PopupMenuShellPart * nPart = &(nw->popup_menu_shell);
  Position          x;
  Position          y;
  Position          ox = nPart->pin.x;
  Position          oy = nPart->pin.y;
  OlVirtualEventRec ve_rec;
  OlVirtualEvent    virtual_event_ret;
  char              draw = 'd';
  Boolean	    def = False;
  OlVendorPartExtension  vendor = _OlGetVendorPartExtension(w);
  OlDefine	    pin_state;
  static OlDefine   last_pin_state = OL_NONE;
  static Boolean    pending_for_focus = False;


  FPRINTF((stderr,"fms button %d w %x widget %x\n", event-> type, w, widget));

		/* must be called from MSButtonHandler if	*/
		/*	client_data != NULL			*/
  if (client_data != (XtPointer)NULL)
	virtual_event_ret = (OlVirtualEvent)client_data;
  else {
	OlLookupInputEvent(w, event, &ve_rec, OL_DEFAULT_IE);
	virtual_event_ret = &ve_rec;
  }

  if (event->type == MotionNotify && pending_for_focus)  {
	int mouse_damping_factor = _OlGetMouseDampingFactor(w);
	Position stayup_x, stayup_y;
	Window stayup_win;
	GetStayupInfo(w, &stayup_x, &stayup_y, &stayup_win);

	/* check whether it's in the damping range */
	if ((ABS_DELTA(stayup_x, event->xbutton.x_root) > mouse_damping_factor) || 
		(ABS_DELTA(stayup_y, event->xbutton.y_root) > mouse_damping_factor))

		pending_for_focus = False;
  }

  /*
   * check for remap events???
   */
  if (w != widget) {
		/* The 2nd check is only for workspace menu...	*/
	if (event-> type == ButtonPress && !_OlIsInMenuStack(shell)) {
		if (!_OlIsPendingStayupMode(w)) {
			_OlResetStayupMode(w);
		}
		else {
				/* Make as stayup mode but don't give
				 * focus yet. If necessary, Focus should
				 * be *set* when seeing ButtonRelease
				 */
			SetStayupMode(w);
			pending_for_focus = True;
		}
        }
     }
  else {
     switch(virtual_event_ret->virtual_name) {
      case OL_MENUDEFAULT:
	  if (vendor->pushpin != OL_NONE)  {
		nPart->pushpinDefault = True;
		def = True;
	  }
          /* FALLTHROUGH */
      case OL_SELECT:
      case OL_MENU:
          switch(event-> type) {
             case MotionNotify:
		if (nw->popup_menu_shell.pin.height != 0) {
		
                	x = event-> xmotion.x;
                	y = event-> xmotion.y;

			_OlQueryResetStayupMode(w, XtWindow(w), x, y);

                	if (x >= ox && x <= ox + (Position)nPart-> pin.width &&
                    	   y >= oy && y <= oy + (Position)nPart-> pin.height) {
				if (def)  {
					pin_state = OL_OUT;
					if (nPart->pushpinDefault)
						_OlSetDefault(w, True);
				}
				else {
					pin_state = OL_IN;
				}
			}
                	else  {
				pin_state = OL_OUT;
				/*  Never draw the default change if not in the
				    pushpin's rectangle. */
				def = False;
			}
			if (pin_state != last_pin_state || def)  {
                	   	(*_olmMSHandlePushpin)(nw, nPart, pin_state, draw);
				last_pin_state = pin_state;
			}
			
		}
                break;
             case ButtonPress:
                _OlResetStayupMode(w);
		if (nw->popup_menu_shell.pin.height != 0) {
                	x = event-> xbutton.x;
                	y = event-> xbutton.y;

                	if (x >= ox && x <= ox + (Position)nPart-> pin.width &&
                    	    y >= oy && y <= oy + (Position)nPart-> pin.height) {
				if (def)  {
					pin_state = OL_OUT;
					if (nPart->pushpinDefault)
						_OlSetDefault(w, True);
				}
				else {
					pin_state = OL_IN;
				}
			}
                	else  {
				pin_state = OL_OUT;
				/*  Never draw the default change if not in the
				    pushpin's rectangle. */
				def = False;
			}
			if (pin_state != last_pin_state || def)  {
                	   	(*_olmMSHandlePushpin)(nw, nPart, pin_state, draw);
				last_pin_state = pin_state;
			}
		}
                break;
             case LeaveNotify:
                break;
             case EnterNotify:
		if (nw->popup_menu_shell.pin.height != 0) {
                	x = event-> xcrossing.x;
                	y = event-> xcrossing.y;
                	if (x >= ox && x <= ox + (Position)nPart-> pin.width &&
                    	    y >= oy && y <= oy + (Position)nPart-> pin.height) {
				if (def)  {
					pin_state = OL_OUT;
					if (nPart->pushpinDefault)
						_OlSetDefault(w, True);
				}
				else {
					pin_state = OL_IN;
				}
			}
                	else  {
				pin_state = OL_OUT;
				/*  Never draw the default change if not in the
				    pushpin's rectangle. */
				def = False;
			}
			if (pin_state != last_pin_state || def)  {
                	   	(*_olmMSHandlePushpin)(nw, nPart, pin_state, draw);
				last_pin_state = pin_state;
			}
		}
                break;
             case ButtonRelease:
		if (_OlIsNotInStayupMode(w)) {
			x = event-> xbutton.x;
			y = event-> xbutton.y;
			_OlFBResetParentSetItem(w, True);
			_OlResetStayupMode(w);
			if (nw->popup_menu_shell.pin.height != 0) {
				if (x >= ox &&
					x <= ox + (Position)nPart-> pin.width &&
			    		y >= oy &&
					y <= oy + (Position)nPart-> pin.height)
					if (virtual_event_ret->virtual_name != OL_MENUDEFAULT)
						PinPutIn(nw);
					else
						_OlSetDefault(w, True);
				/*  Make sure the pin is redrawn */
				last_pin_state = OL_NONE;
			}
                }
                break;
             default:
                break;
             }
         break;
      default:
         break;
      }
     }

  /*
   * test for spring loaded release events
   */

  if (event-> type == ButtonRelease && !_OlIsEmptyMenuStack(w))
  {
	if (_OlIsInStayupMode(w) || !pending_for_focus)
	{
		int mouse_damping_factor = _OlGetMouseDampingFactor(w);
		Position stayup_x, stayup_y;
		Window stayup_win;
		GetStayupInfo(w, &stayup_x, &stayup_y, &stayup_win);

		/* check whether it's in the damping range */
      		if ((ABS_DELTA(stayup_x, event->xbutton.x_root) > mouse_damping_factor) || 
      		(ABS_DELTA(stayup_y, event->xbutton.y_root) > mouse_damping_factor))
			_OlResetStayupMode(w);
	}

	if (_OlIsNotInStayupMode(w)) {
		_OlFBResetParentSetItem(w, True);
		_OlPopdownCascade(_OlRootOfMenuStack(w), False);
        }
	else {
		if (pending_for_focus || _OlIsPendingStayupMode(w))  {
		   _OlSetStayupMode(w);
		}
	}
	/*  The ButtonRelease should always reset the pending_for_focus */
	pending_for_focus = False;
	/*  Reset the last_pin_state so that it is always redrawn */
	last_pin_state = OL_NONE;
  }
} /* end of MSShellButtonHandler */

/*
 * MSRedisplay
 *
 */
/*ARGSUSED*/
static void
MSRedisplay OLARGLIST((w, event, region))
	OLARG( Widget,   w)
	OLARG( XEvent *, event)
	OLGRA( Region,   region)
{
  PopupMenuShellWidget nw    = (PopupMenuShellWidget)w;
  PopupMenuShellPart * nPart = &(nw->popup_menu_shell);

  /*  If the window manager is decorating the shell, then don't draw
	anything in either mode.  */
  if (nw->shell.override_redirect == False)
	return;

  if (OlGetGui() == OL_OPENLOOK_GUI)  {
     OlgDrawBox(XtScreen(nw), XtWindow(nw), nPart-> attrs, 0, 0,
		nw-> core.width, nw-> core.height, False);
  }
  else  {  /*  Motif-mode */
	if (nPart->shadow_thickness != 0)
		OlgDrawBorderShadow( XtScreen(nw), XtWindow(nw),
			nPart->attrs, OL_SHADOW_OUT,
			nPart->shadow_thickness,
			(Position)0, (Position)0,
			nw->core.width, nw->core.height);
  }


#define RectInRegion(R,RECT) \
	(XRectInRegion(R,(RECT)->x, (RECT)->y, (RECT)->width, (RECT)->height) \
		!= RectangleOut)

     if (HAS_PUSHPIN(nw) && RectInRegion(region, &(nPart->pin)))  {
        (*_olmMSHandlePushpin)(nw, nPart,
			(_OlGetVendorPartExtension(w))->pushpin, 'd');
     }
     if (HAS_TITLE(nw) && RectInRegion(region, &(nPart->title)))  {
        (*_olmMSHandleTitle)(nw, nPart, 'd');
     }
} /* end of MSRedisplay */

/*
 * MSSetValues
 *
 */
/* ARGSUSED */
static Boolean
MSSetValues OLARGLIST((cur_w, req_w, new_w, args, num_args))
	OLARG( Widget,		cur_w)
	OLARG( Widget,		req_w)
	OLARG( Widget,		new_w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	PopupMenuShellWidget	current = (PopupMenuShellWidget)cur_w;
	PopupMenuShellWidget	new = (PopupMenuShellWidget)new_w;
	PopupMenuShellPart *	nPart = &(new->popup_menu_shell);
	PopupMenuShellPart *	cPart = &(current->popup_menu_shell);
	Boolean			redisplay = False;
	Boolean			layout = False;

#define CHANGED(field)		(nPart->field != cPart->field)
#define CORE_CHANGED(field)	(new->core.field != current->core.field)
#define WM_CHANGED(field)	(new->wm.field != current->wm.field)

	if (CHANGED(shadow_thickness) || CHANGED(font_list)) {
		redisplay = True;
		layout = True;
	}

	/*  If we are responsible for drawing the title, then we need to
	    check the state of the window manager's title.  */
	if (WM_CHANGED(title) && new->shell.override_redirect == True) {
		redisplay = True;
		layout = True;
	}

		/*  See Primitive:Initialize() for explaination...  */
	if (CHANGED(font)) {
		_OlLoadDefaultFont(new_w, nPart->font);
		layout = True;
	}

	if (CORE_CHANGED(background_pixel)	||
	    CHANGED(fontColor)			||
	    CHANGED(font))
	{
		MSGetGC(new, nPart);
		redisplay = True;
	}

	if (CORE_CHANGED(background_pixel) || CHANGED(foreground))
	{
		OlgDestroyAttrs(nPart->attrs);

		nPart->attrs = OlgCreateAttrs(
					XtScreen(new_w),
					nPart->foreground,
					(OlgBG *)&(new->core.background_pixel),
					False, OL_DEFAULT_POINT_SIZE
		);

		redisplay = True;
	}

	/*  Layout procedure is only called at the end of the SetValues
	    chain to avoid extra calls.  */
	OlLayoutWidgetIfLastClass(new_w, popupMenuShellWidgetClass,
		layout, False);

	return (redisplay);

#undef CORE_CHANGED
#undef CHANGED
} /* end of MSSetValues */


/*
 * The following are public routines, including
 *	OlAddDefaultPopupMenuEH,
 *	OlPostPopupMenu
 *	OlUnpostPopupMenu
 *
 */

	/* I used the error message from Menu.c	*/
#define CALL_ERR(d,m)							   \
		OlVaDisplayErrorMsg(d, OleNfileMenu, OleTmsg11,		   \
				OleCOlToolkitError, OleMfileMenu_msg11, m)

/*
 * MenuPopupHandler
 *
 * This procedure is used to post a popup menu assigned to an
 * arbitrary widget.
 *
 */
static void
PopupMenuEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,    w)
	OLARG( XtPointer, client_data)
	OLARG( XEvent *,  xevent)
	OLGRA( Boolean *, cont_to_dispatch)
{
	Widget            popup_menu = (Widget)client_data;
	OlVirtualEventRec ve;

		/* check on "xevent" is not necessary because this	*/
		/* routine will only be accessed from			*/
		/* OlAddDefaultPopupMenuEH...				*/

	OlLookupInputEvent(w, xevent, &ve, OL_DEFAULT_IE);

	if (ve.virtual_name == OL_MENU)
	{
		OlPostPopupMenu(
			w, popup_menu, OL_MENU,
			(OlPopupMenuCallbackProc)NULL,
			xevent->xbutton.x_root, xevent->xbutton.y_root,
			xevent->xbutton.x, xevent->xbutton.y
		);

			/* no need to dispatch...			*/
		*cont_to_dispatch = False;
	}
} /* end of PopupMenuEH */

/*
 * OlAddDefaultPopupMenuEH -
 *
 */
extern void
OlAddDefaultPopupMenuEH OLARGLIST((menu_owner, popup_menu))
	OLARG( Widget,		menu_owner)
	OLGRA( Widget,		popup_menu)
{
	if (XtIsSubclass(popup_menu, popupMenuShellWidgetClass) == False)
	{
		CALL_ERR(XtDisplay(popup_menu), "OlAddDefaultPopupMenuEH");
	}

	XtAddEventHandler(
		menu_owner, ButtonPressMask, False,
		PopupMenuEH, (XtPointer)popup_menu
	);

} /* end of OlAddDefaultPopupMenuEH */

/*
 * OlPostPopupMenu -
 *
 */
extern void
OlPostPopupMenu OLARGLIST((menu_owner, popup_menu, activation_type, popdown, root_x, root_y, init_x, init_y))
	OLARG( Widget,			menu_owner)
	OLARG( Widget,			popup_menu)
	OLARG( OlVirtualName,		activation_type)
	OLARG( OlPopupMenuCallbackProc,	popdown)
	OLARG( Position,		root_x)
	OLARG( Position,		root_y)
	OLARG( Position,		init_x)
	OLGRA( Position,		init_y)
{
	XRectangle				rect;
	Dimension      TWO_POINTS = OlScreenPointToPixel(OL_HORIZONTAL,
					2, XtScreenOfObject(popup_menu));

	if (XtIsSubclass(popup_menu, popupMenuShellWidgetClass) == False)
	{
		CALL_ERR(XtDisplay(popup_menu), "OlPostPopupMenu");
	}

	if (activation_type != OL_MENU && activation_type != OL_MENUKEY)
	{
			/* maybe a warning here...	*/ 
		return;
	}

	rect.x		= root_x + TWO_POINTS;
	rect.y		= root_y + TWO_POINTS;
	rect.width	=
	rect.height	= 1;

	_OlResetPreviewMode(popup_menu);
	_OlPopupMenu(
		popup_menu, menu_owner, popdown, &rect, PopupAlignment,
		True, XtWindowOfObject(menu_owner), root_x, root_y
	);

	if (activation_type == OL_MENUKEY)
	{
		_OlSetStayupMode(popup_menu);
	}
	else
	{
			/* if got Button Release after calling  */
			/* _OlPopupMenu then we have to reset   */
			/* to pending stayup otherwise a menu   */
			/* will never show up if time interval  */
			/* between a Button Press and a Button  */
			/* Release is really short...           */
		if (_OlIsInStayupMode(popup_menu))
			_OlSetStayupModeValue(popup_menu, PENDING_STAYUP);
	}
} /* end of OlPostPopupMenu */

/*
 * OlUnpostPopupMenu -
 *
 */
extern void
OlUnpostPopupMenu OLARGLIST((popup_menu))
	OLGRA( Widget,		popup_menu)
{
	if (XtIsSubclass(popup_menu, popupMenuShellWidgetClass) == False)
	{
		CALL_ERR(XtDisplay(popup_menu), "OlUnpostPopupMenu");
	}

	_OlPopdownCascade(popup_menu, False);
} /* end of OlUnpostPopupMenu */

#undef CALL_ERR


/*
 * The following are private routines (see PopupMenuP.h for
 * the function names).
 *
 */

typedef struct {
	OlPopupMenuCallbackProc		popdown_callback;
	Widget				shell_widget;
	Widget				parent_widget;
} MenuStackRec, *MenuStack;

	/* each "token" represents info that is specific to a "screen"	*/
typedef struct {
	Screen *			key;
	short				stack_index;
	short				stack_size;
	MenuStack			menu_stack;
	Position			stay_up_x;
	Position			stay_up_y;
	Window				stay_up_window;
	OlStayupMode			stay_up_mode;
	Boolean				should_preview;
} DbTokenRec, *DbToken;

#define TheKey(t)			(t)->key
#define TheStackIndex(t)		(t)->stack_index
#define TheStackSize(t)			(t)->stack_size
#define TheMenuStack(t)			(t)->menu_stack
#define TheStayUpX(t)			(t)->stay_up_x
#define TheStayUpY(t)			(t)->stay_up_y
#define TheStayUpWindow(t)		(t)->stay_up_window
#define TheStayUpMode(t)		(t)->stay_up_mode
#define TheShouldPreview(t)		(t)->should_preview

static DbToken	DbFindToken OL_ARGS((Widget));

static void	SetStayupInfo OL_ARGS((Widget,
				       Position, Position, Window));


static DbToken
DbFindToken OLARGLIST((w))
	OLGRA( Widget,			w)
{
		/* I keep this index around to avoid mutiple lookup	*/
		/* although You won't see any benific when dealing	*/
		/* single/screen/display...				*/
	static short			index_from_last_call	= -1;

	static DbToken			table			= NULL;
	static short			num_slots_alloced	= 0;
	static short			num_entries		= 0;

	Cardinal			i;
	Screen *			key;

	key = XtScreenOfObject(w);

		/* Assume that we never free a token otherwise I need	*/
		/* to reset "index_from_last_call"...			*/
	if (index_from_last_call != -1 &&
	    TheKey(&table[index_from_last_call]) == key)
	{
		return(&table[index_from_last_call]);
	}

	for (i = 0; i < num_entries; i++)
	{
		if (TheKey(&table[i]) == key)
		{
			index_from_last_call = (short) i;
			return(&table[i]);
		}
	}

		/* Create a new one if I get over here...		*/

		/* Assume we need only one for most cases...		*/
		/* That is the reason I don't need "num_slots_left".	*/
#define ONE_MORE_SLOTS		1

	num_slots_alloced += ONE_MORE_SLOTS;

	table = (DbToken)XtRealloc(
			(char * ) table,
			(int)sizeof(DbTokenRec) *
				num_slots_alloced
	);
#undef ONE_MORE_SLOTS

#define Idx	&table[num_entries]

	TheKey(Idx)		= key;
	TheStackIndex(Idx)	= -1;
	TheStackSize(Idx)	= 0;
	TheMenuStack(Idx)	= NULL;
	TheStayUpX(Idx)		= 0;
	TheStayUpY(Idx)		= 0;
	TheStayUpWindow(Idx)	= None;
	TheStayUpMode(Idx)	= NO_STAYUP;
	TheShouldPreview(Idx)	= False;

#undef Idx

	index_from_last_call = num_entries;
	num_entries++;

	return(&table[num_entries-1]);
} /* end of DbFindToken */


static void
GetStayupInfo OLARGLIST((w, x, y, win))
	OLARG( Widget,		w)
	OLARG( Position *,	x)
	OLARG( Position *,	y)
	OLGRA( Window *,	win)
{
	DbToken			token;

	token	= DbFindToken(w);

	*x	= TheStayUpX(token);
	*y	= TheStayUpY(token);
	*win	= TheStayUpWindow(token);
} /* end of GetStayupInfo */

static void
SetStayupInfo OLARGLIST((w, x, y, win))
	OLARG( Widget,		w)
	OLARG( Position,	x)
	OLARG( Position,	y)
	OLGRA( Window,		win)
{
	DbToken			token;

	token			= DbFindToken(w);

	TheStayUpX(token)	= x;
	TheStayUpY(token)	= y;
	TheStayUpWindow(token)	= win;
} /* end of SetStayupInfo */

/*
 * _OlGetMenuDefault -
 *
 */
extern Boolean
_OlGetMenuDefault OLARGLIST((w, child, default_index, ultimate))
	OLARG( Widget,     w)
	OLARG( Widget *,   child)
	OLARG( Cardinal *, default_index)
	OLGRA( Boolean,    ultimate)
{
	Boolean    retval = True;  /* assumes must take a flat widget */
	WidgetList kid      = ((CompositeWidget)w)->composite.children;
	Arg        defaultArg[1];
	Boolean    is_default = False;
	Arg        numItemsArg[1];
	Cardinal   num_items = 0;
	Arg        popupMenuArg[1];
	Widget     popupMenu = NULL;
	Cardinal   j;

	*child = _OlGetDefault(w);

	if ((*child != NULL) && _OlIsFlat(*child))  {
		XtSetArg(defaultArg[0], XtNdefault, &is_default);
		XtSetArg(numItemsArg[0], XtNnumItems, &num_items);
		XtGetValues(*child, numItemsArg, 1);
		for (j = 0; !is_default && j < num_items; j++)
        		OlFlatGetValues(*child, j, defaultArg, 1);
		*default_index = --j;
		if (ultimate) {
			XtSetArg(popupMenuArg[0], XtNpopupMenu, &popupMenu);
			OlFlatGetValues(*child, *default_index, popupMenuArg,1);
			if (popupMenu)
				_OlGetMenuDefault(popupMenu, child,
					default_index, ultimate);
		}

	}

  return (retval);
} /* end of _OlGetMenuDefault */

/*
 * _OlGetStayupMode
 *
 */
extern OlStayupMode
_OlGetStayupMode OLARGLIST((w))
	OLGRA( Widget,		w)
{
	return(TheStayUpMode(DbFindToken(w)));
} /* end of _OlGetStayupMode */

/*
 * _OlInPreviewMode
 *
 * This function returns the current setting of should_preview
 * (either True or False).
 *
 */
extern Boolean
_OlInPreviewMode OLARGLIST((w))
	OLGRA( Widget,		w)
{
	return(TheShouldPreview(DbFindToken(w)));
} /* end of _OlInPreviewMode */

/*
 * _OlInitStayupMode
 *
 * The static variable stay_up_mode is maintained and queried using
 * a suite of external routines.  The value of stay_up_mode is set
 * to PENDING_STAYUP when a MENU button is pressed on an object which 
 * has an associated menu.  The stay_up_mode changes to IN_STAYUP iff
 *
 * 1. When SelectItem is called if the state is still PENDING_STAYUP
 * 2. The SELECTED item has an associated menu
 * 3. The should_preview flag is off (i.e., the user's not in POWER_USER
 *    or the SELECT was performed using a MENU button.)
 * 
 * The mode can change to NO_STAYUP (from PENDING_STAYUP) if prior to
 * button release:
 *
 * 1. The originally selected Item becomes unselected either by
 *    a. Selecting another item
 *    b. Selecting nothing
 * 2. "Enough" pointer motion is detected
 *
 * The suite of routines are used to enforce this behavior:
 *
 * _OlInitStayupMode
 * _OlQueryResetStayupMode
 * _OlResetStayupMode
 * _OlSetStayupMode
 * _OlIsInStayupMode
 * _OlIsNotInStayupMode
 * _OlIsPendingStayupMode
 *
 */
extern void
_OlInitStayupMode OLARGLIST((w, window, x, y))
	OLARG( Widget,		w)
	OLARG( Window,		window)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
  _OlInPreviewMode(w) ? _OlSetStayupModeValue(w, NO_STAYUP) :
			_OlSetStayupModeValue(w, PENDING_STAYUP);

  SetStayupInfo(w, x, y, window);
} /* end of _OlInitStayupMode */

/*
 * _OlIsEmptyMenuStack
 *
 */
extern Boolean
_OlIsEmptyMenuStack OLARGLIST((w))
	OLGRA( Widget,		w)
{
	return(TheStackIndex(DbFindToken(w)) < 0);
} /* end of _OlIsEmptyMenuStack */

/*
 * _OlIsInMenuStack
 *
 */
extern Boolean
_OlIsInMenuStack OLARGLIST((w))
	OLGRA( Widget,	w)
{
	register int	i;
	DbToken		token;

	token = DbFindToken(w);

	for (i = 0; i <= TheStackIndex(token); i++)
	   if ((TheMenuStack(token))[i].shell_widget == w)
	      return(True);

	return(False);
} /* end of _OlIsInMenuStack */

/*
 * _OlMenuLock
 *
 */
extern void
_OlMenuLock OLARGLIST((shell, w, event))
	OLARG( Widget,		shell)
	OLARG( Widget,		w)
	OLGRA( XEvent *,	event)
{
	if (event)
		if (w == _OlRootOfMenuStack(shell))
			_OlResetStayupMode(shell);
		else
			if (_OlTopOfMenuStack(shell) != shell)
				_OlPopdownCascade(shell, True);

	if (_OlIsEmptyMenuStack(shell))
	{
#define MASK	ButtonPressMask | ButtonMotionMask | ButtonReleaseMask

		_OlGrabPointer(w, True, MASK, GrabModeAsync, GrabModeAsync,
				None, None, CurrentTime);
				
/*
		while (XGrabPointer(
				XtDisplayOfObject(w), XtWindowOfObject(w),
				True, MASK, GrabModeAsync, GrabModeAsync,
				None, None, CurrentTime) != GrabSuccess)
			;
*/

#undef MASK

		XtAddGrab(w, True, True);
		_OlPushMenuStack(w, w, NULL);
	}
} /* end of _OlMenuLock */

/*
 * _OlPopMenuStack
 *
 */
extern Widget
_OlPopMenuStack OLARGLIST((ww, parent, popdown))
	OLARG( Widget,				ww)
	OLARG( Widget *,			parent)
	OLGRA( OlPopupMenuCallbackProc *,	popdown)
{
	DbToken		token;
	Widget		w;

	token = DbFindToken(ww);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	if (II >= 0)
	{
		w        = MM[II].shell_widget;
		*parent  = MM[II].parent_widget;
		*popdown = MM[II].popdown_callback;
	}
	else
	{
		w        =
		*parent  = (Widget)NULL;
		*popdown = (OlPopupMenuCallbackProc)NULL;
	}

	if (--II < -1)
		II = -1;

#undef MM
#undef II

	return (w);
} /* end of _OlPopMenuStack */

/*
 * _OlPopdownCascade
 *
 */
extern void
_OlPopdownCascade OLARGLIST((shell, replace_shell))
	OLARG( Widget,		shell)
	OLGRA( Boolean,		replace_shell)
{
  Widget			top;
  Widget			parent;
  OlPopupMenuCallbackProc	unset;

  FPRINTF((stderr,"start %s popdown %x (%d)\n", where, shell, replace_shell));

  if (shell == NULL)
  {
	return;
  }
  if (_OlIsEmptyMenuStack(shell))
     {
     FPRINTF((stderr, "Menu stack was empty!!! called for %s\n", where));
     return;
     }
  else
     if (!_OlIsInMenuStack(shell))
        {
        FPRINTF((stderr, "Shell %x wasn't up! called for %s\n", shell, where));
        return;
        }

  for (top = _OlPopMenuStack(shell, &parent, &unset); 
       top != NULL && top != shell; 
       top = _OlPopMenuStack(shell, &parent, &unset))
     {
     FPRINTF((stderr,"pop %x\n", top));
     if (unset)
        (*unset)(parent);
     if (XtClass(top) == popupMenuShellWidgetClass)
        {
        XtPopdown(top);
        }
     XtRemoveGrab(top);
     FPRINTF((stderr,"%s:%d remove grab from %x\n", __FILE__, __LINE__, top));
     }
				
  if (top == shell)
     if (replace_shell)
        _OlPushMenuStack(top, parent, unset);
     else
        {
        if (unset)
           (*unset)(parent);
        if (XtClass(top) == popupMenuShellWidgetClass)
           {
           XtPopdown(top);
           }
        XtRemoveGrab(top);
        FPRINTF((stderr,
		"%s:%d remove grab from %x\n", __FILE__, __LINE__, top));
        FPRINTF((stderr,"pop %x\n", top));
        }

  if (_OlIsEmptyMenuStack(shell))
     {
     _OlResetStayupMode(shell);
     _OlResetPreviewMode(shell);
     _OlUngrabPointer(shell);
/*
     XUngrabPointer(XtDisplay(shell), CurrentTime);
*/
     }

  FPRINTF((stderr,"end %s popdown\n", where));
} /* end of _OlPopdownCascade */


/*
 * LeaveStackMenu
 *
 */
static Boolean got_map = False;
static Boolean got_btn_up = False;

/* ARGSUSED */
static Bool
LeaveStackMenu OLARGLIST((dpy, event, arg))
	OLARG( Display *,	dpy)
	OLARG( XEvent *,	event)
	OLGRA( XPointer,	arg)
{
   Bool retval = False;

/*
 * checking the window is probably not necessary
 */
   switch (event-> type)
      {
      case LeaveNotify:
      case EnterNotify:
      case ButtonPress:
      case MotionNotify:
         retval = !got_map;
         break;
      case ButtonRelease:
		/* don't take this event away...	*/
         retval = False;
	 SetStayupInfo((Widget)arg, event->xbutton.x_root, 
		event->xbutton.y_root, XtWindow((Widget)arg));
         got_btn_up = True;
         break;
      case MapNotify:
         if (event-> xany.window == XtWindow((Widget)arg))
            retval = got_map = True;
         break;
      default:
         break;
      }

   return (retval);
} /* end of LeaveStackMenu */

/*
 * _OlPopupMenu
 *
 * This procedure is responsible for posting a supplied shell
 * which was brought up as a result of an event delivered to
 * the parent widget.  The specified shell is positioned relative
 * to shellx, shelly using the specified alignment.
 * The popdown procedure is called with the parent widget as an 
 * argument when the menu is later unposted.
 */
extern void
_OlPopupMenu OLARGLIST((shell, parent, popdown, rect, align, init_stayup, init_window, init_x, init_y))
	OLARG( Widget,			shell)
	OLARG( Widget,			parent)
	OLARG( OlPopupMenuCallbackProc,	popdown)
	OLARG( XRectangle *,		rect)
	OLARG( OlMenuAlignment,		align)
	OLARG( Boolean,			init_stayup)
	OLARG( Window,			init_window)
	OLARG( Position,		init_x)
	OLGRA( Position,		init_y)
{
  int            screen_width  = WidthOfScreen(XtScreenOfObject(shell));
  int            screen_height = HeightOfScreen(XtScreenOfObject(shell));
  Widget         child_widget;
  Cardinal       default_index;
  Arg            arg[3];
  int            maximum;
  Position       flat_x;
  Position       flat_y;
  Dimension      flat_width;
  Dimension      flat_height;
  Position       x;
  Position       y;
  Position       warpy;
  Boolean        warp_needed = 0;
  int            shellx;
  int            shelly;
  Window         child;
  XEvent         event;
  Dimension      ONE_POINT = OlScreenPointToPixel(OL_HORIZONTAL,
					1, XtScreenOfObject(shell));
  Cursor         cursor;
  Display *	 dpy;

  if (!XtIsRealized(shell))
	XtRealizeWidget(shell);

  if (((ShellWidget)shell)->shell.popped_up)  {
	XRaiseWindow(XtDisplay(shell), XtWindow(shell));
	OlSetInputFocus(shell, RevertToNone, XtLastTimestampProcessed(XtDisplay(shell)));
	return;
  }

  dpy = XtDisplay(shell);

  if ((_OlGetVendorPartExtension(shell))->pushpin == OL_IN)  {
	/*  The menu is pinned.  Call XtPopup to either raise it or
	    map it.  */
	if (((PopupMenuShellWidget)shell)->shell.override_redirect == False)  {
		XtPopup(shell, XtGrabNone);
		return;
	}
   }

  _OlGetMenuDefault(shell, &child_widget, &default_index, False);

  if (_OlIsInMenuStack(shell))
  {
	FPRINTF((stderr,"shell was already up!\n"));
	return;
  }

  if (child_widget != NULL)  {
	if (_OlIsFlat(child_widget))
		OlFlatGetItemGeometry(child_widget, default_index,
		       &flat_x, &flat_y, &flat_width, &flat_height);
	else if (child_widget == shell)  {
		if (OlGetGui() == OL_MOTIF_GUI)  {
		/*  In Motif mode, the shell is the default, but the
		    positioning needs to be done relative to the StayUp button.
		    Assuming that the 0 child is the StayUp button... */
			child_widget = ((ShellWidget)shell)->composite.children[0];
			if (child_widget && _OlIsFlat(child_widget))
				OlFlatGetItemGeometry(child_widget, 0, &flat_x,
					&flat_y, &flat_width, &flat_height);
			else
				/*  an unusual error case, since it would mean
				    that the StayUp button got deleted somehow.
				*/
				child_widget = shell;
		}
		else {
			flat_x = NPART((PopupMenuShellWidget)shell, pin.x);
			flat_y = NPART((PopupMenuShellWidget)shell, pin.y);
			flat_width = NPART((PopupMenuShellWidget)shell, pin.width);
			flat_height = NPART((PopupMenuShellWidget)shell, pin.height);
		}
	}
	else
		flat_x = flat_width = flat_y = flat_height = 0;

	XTranslateCoordinates(dpy, XtWindowOfObject(child_widget),
		XtWindow(shell), 0, 0, &shellx, &shelly, &child);
  }
  else  {  /* Child widget is null  */
      flat_x = flat_width = flat_y = flat_height = 0;
      shellx = shelly = 0;
  }

  switch (align)
   {
   case PullAsideAlignment: /* was West */
      x = rect->x + rect->width + ONE_POINT;
      y = (rect->y + rect->height / 2) - (flat_y + flat_height / 2 + shelly); 
      break;
   case AbbrevDropDownAlignment: /* was Northwest */
      x = rect->x;
      y = rect->y + rect->height;
      break;
   case DropDownAlignment: /* was North */
      if (OlGetGui() == OL_MOTIF_GUI) {
	x = rect->x;
	y = rect->y + rect->height;
      }
      else {
	x = rect->x + rect->width / 2 - shell-> core.width / 2;
	y = rect->y + rect->height + ONE_POINT;
      }
      break;
   case PopupAlignment: /* was default */
      x = rect->x + ONE_POINT;
      y = rect->y - (flat_y + flat_height / 2 + shelly); 
      break;
   default:
      break;
   }

  if (x < 0)
     {
     warp_needed |= 1;
     x = 0;
     }
  else
     if ((maximum = screen_width - shell-> core.width) < x)
        {
        warp_needed |= 1;
        x = (Position) maximum;
        }

  if (y < 0)
     {
     FPRINTF((stderr,"y was less than 0!!! (%d)\n", y));
     warp_needed |= 1;
     y = 0;
     }
  else
     if ((maximum = screen_height - shell-> core.height) < y)
        {
        warp_needed |= 1;
        y = (Position) maximum;
        }

  XtSetArg(arg[0], XtNx, x);
  XtSetArg(arg[1], XtNy, y);
  XtSetValues(shell, arg, 2);

  if ((_OlGetVendorPartExtension(shell))->pushpin == OL_IN)  {
	/*  The menu is pinned.  Call XtPopup to either raise it or
	    map it.  */
	if (((PopupMenuShellWidget)shell)->shell.override_redirect == True)  {
		PinPutIn((PopupMenuShellWidget)shell);
		return;
	}
   }

  if (warp_needed)
     {
     warpy = y + flat_y + flat_height / 2 + shelly;
     XWarpPointer(dpy, None, 
        RootWindowOfScreen(XtScreen(shell)), 0, 0, 0, 0, x, warpy);
     init_x = x;
     init_y = warpy;
     }

  if (init_stayup || warp_needed)
     _OlInitStayupMode(shell, init_window, init_x, init_y);

  if (XtIsSubclass(shell, popupMenuShellWidgetClass))
	cursor = ((PopupMenuShellWidget)shell)->popup_menu_shell.cursor;
  else
	cursor = None;

  if (_OlIsEmptyMenuStack(shell))
     {
	Widget	grab_widget;

		/*
		 * Simulate what _OlMenuLock is doing when parent
		 * is a subclass of flatButtonsWidgetClass. i.e., "parent"
		 * should always be on the root of a menu stack and
		 * the grab window should be XtWindow(parent).
		 *
		 * Note that the code below will be exercised when a
		 * menu is pinned...
		 *
		 */
	if (XtIsSubclass(parent, flatButtonsWidgetClass))
	{
		XtAddGrab(parent, True, True);
		_OlPushMenuStack(parent, parent, NULL);
		XtAddGrab(shell, False, False);
		grab_widget = parent;
	}
	else
	{
		XtAddGrab(shell, True, True);
		grab_widget = shell;
	}

     XtPopup(shell, XtGrabNone);
	/*
	 * may want to limit this using a counter so that we
	 * don't freeze if someone else has a grab
	*/
	_OlGrabPointer(grab_widget, True,
		ButtonPressMask|ButtonMotionMask|ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);
     }
  else
     {
     XtAddGrab(shell, False, False);
     XChangeActivePointerGrab(XtDisplay(shell), 
		ButtonPressMask|ButtonMotionMask|ButtonReleaseMask,
		cursor, CurrentTime);
     XtPopup(shell, XtGrabNone);
     }

  got_btn_up = False;
  got_map = False;

  while (got_map == False)
     XIfEvent(dpy, &event, LeaveStackMenu, (XPointer) shell);

  _OlPushMenuStack(shell, parent, popdown);

  if (got_btn_up)
     _OlSetStayupMode(shell);
} /* end of _OlPopupMenu */


/*
 * _OlPreviewMenuDefault
 *
 */
extern void
_OlPreviewMenuDefault OLARGLIST((shell, w, item_index))
	OLARG( Widget,		shell)
	OLARG( Widget,		w)
	OLGRA( Cardinal,	item_index)
{
  Widget		child;
  Cardinal		default_index;
  PopupMenuShellPart *	nPart =
			  &(((PopupMenuShellWidget)shell)->popup_menu_shell);
   
  _OlGetMenuDefault(shell, &child, &default_index, True);

  if (child != NULL && XtIsSubclass(child, popupMenuShellWidgetClass))
     {
     if (item_index != (Cardinal)OL_NO_ITEM)
        {
        Position	x, y;
        Dimension	width, height;

        OlFlatGetItemGeometry(w, item_index, &x, &y, &width, &height);

        (void) XClearArea(XtDisplay(w), XtWindow(w),
                          (int)x, (int)y,
                          (unsigned int)width, (unsigned int)height,
                          (Bool)FALSE);

	if (OlGetGui() == OL_OPENLOOK_GUI)  {
        	OlgDrawPushPin(XtScreen(w), XtWindow(w), nPart-> attrs,
           		x, y, width, height, PP_DEFAULT);
        	}
	}
     else
        {
        (void) XClearArea(XtDisplay(w), XtWindow(w),
                          0, 0, 0, 0, (Bool)FALSE);

	if (OlGetGui() == OL_OPENLOOK_GUI)  {
        	OlgDrawPushPin(XtScreen(w), XtWindow(w), nPart-> attrs,
           		0, 0, w-> core.width, w-> core.height, PP_DEFAULT);
	}
        }
     }
  else
     {
     if (item_index != (Cardinal)OL_NO_ITEM)
        {
        OlFlatPreviewItem(child, default_index, w, item_index);
        }
     else
        {
        Arg arg[1];

        XtSetArg (arg[0], XtNpreview, w);
        XtSetValues (child, arg, 1);
        }
     }
} /* end of _OlPreviewMenuDefault */

/*
 * _OlPushMenuStack
 *
 */
extern void
_OlPushMenuStack OLARGLIST((shell, parent, popdown))
	OLARG( Widget,			shell)
	OLARG( Widget,			parent)
	OLGRA( OlPopupMenuCallbackProc,	popdown)
{
	DbToken		token;

	token = DbFindToken(shell);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))
#define SS	TheStackSize(token)

	II++;

	if (II == SS)
	{
		SS += 5;

		MM = (MenuStack)XtRealloc((char *)MM, SS * sizeof(MenuStackRec));
	}

	MM[II].shell_widget     = shell;
	MM[II].parent_widget    = parent;
	MM[II].popdown_callback = popdown;

#undef SS
#undef MM
#undef II
} /* end of _OlPushMenuStack */

/*
 * _OlQueryResetStayupMode
 *
 * This procedure checks the current stay_up_mode, stay_up_window, 
 * stay_up_x, and stay_up_y against the given window, x, and y.  If 
 * the current stay_up_mode is PENDING_STAYUP and either:
 *
 * stay_up_window is not the same as the given window
 *    or
 * the difference between stay_up_x and the given x is greater than
 * the MOUSE_DAMPING_FACTOR
 *    or
 * the difference between stay_up_x and the given x is greater than
 * the MOUSE_DAMPING_FACTOR
 *
 * then the stay_up_mode is set to NO_STAYUP; otherwise the stay_up_mode
 * is left unchanged.
 *
 */
extern void
_OlQueryResetStayupMode OLARGLIST((w, window, x, y))
	OLARG( Widget,		w)
	OLARG( Window,		window)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
  Cardinal	mouse_damping_factor = _OlGetMouseDampingFactor(w);
  Position	stay_up_xx,
		stay_up_yy;
  Window	stay_up_win;

  GetStayupInfo(w, &stay_up_xx, &stay_up_yy, &stay_up_win);

  if ( _OlIsPendingStayupMode(w) )  {
	Position root_x, root_y;

	XtTranslateCoords(w, x, y, &root_x, &root_y);
	if ( (ABS_DELTA(stay_up_xx, root_x) > mouse_damping_factor) || 
		(ABS_DELTA(stay_up_yy, root_y) > mouse_damping_factor) )
		_OlResetStayupMode(w);
  }

} /* end of _OlQueryResetStayupMode */

/*
 * _OlRootOfMenuStack
 *
 */
extern Widget
_OlRootOfMenuStack OLARGLIST((w)) 
	OLGRA( Widget,		w)
{ 
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	return(II < 0 ? NULL : MM[0].shell_widget);

#undef MM
#undef II
} /* end of _OlRootOfMenuStack */

/*
 * _OlRoot1OfMenuStack -
 *	if a stack is created because of the flat buttons
 *	then the root is a flat buttons (not a shell widget),
 *	so we need this routine to support FButtons:TraverseItems().
 *
 *	Also see MSActivateWidget: OL_MOVELEFT/OL_MOVERIGHT...
 */
extern Widget
_OlRoot1OfMenuStack OLARGLIST((w)) 
	OLGRA( Widget,		w)
{ 
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	return(II < 1 ? NULL : MM[1].shell_widget);

#undef MM
#undef II
} /* end of _OlRoot1OfMenuStack */

/*
 * _OlRootParentOfMenuStack
 *
 */
extern Widget
_OlRootParentOfMenuStack OLARGLIST((w))
	OLGRA( Widget,		w)
{ 
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	return(II < 0 ? NULL : MM[0].parent_widget);

#undef MM
#undef II
} /* end of _OlRootParentOfMenuStack */

/*
 * _OlSetPreviewModeValue
 *
 */
extern void
_OlSetPreviewModeValue OLARGLIST((w, want_to_reset))
	OLARG( Widget,		w)
	OLGRA( Boolean,		want_to_reset)
{
	DbToken			token;

	token = DbFindToken(w);

	if (want_to_reset == True)
	{
		TheShouldPreview(token) = False;
	}
	else
	{
		TheShouldPreview(token) = _OlSelectDoesPreview(w);
	}
} /* end of OlSetPreviewModeValue */

/*
 * _OlSetStayupModeValue -
 *
 * This proceudre sets the stay_up_mode to "v" and calls
 * the MSSetFocus routine to assign keyboard focus to the top menu
 * in the menu cascade if v == IN_STAYUP.
 *
 */
extern void
_OlSetStayupModeValue OLARGLIST((w, v))
	OLARG( Widget,		w)
	OLGRA( OlStayupMode,	v)
{
	DbToken			token;

	token = DbFindToken(w);

	if ((TheStayUpMode(token) = v) == IN_STAYUP)
	{
		MSSetFocus(_OlTopOfMenuStack(w), CurrentTime);
	}
} /* end of _OlSetStayupModeValue */

static void
SetStayupMode OLARGLIST((w))
	OLGRA( Widget,		w)
{
	DbToken			token;

	token = DbFindToken(w);
	TheStayUpMode(token) = IN_STAYUP;
} /* end of SetStayupMode */

/*
 * _OlTopOfMenuStack
 *
 */
extern Widget
_OlTopOfMenuStack OLARGLIST((w)) 
	OLGRA( Widget,		w)
{ 
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	return(II < 0 ? NULL : MM[II].shell_widget);

#undef II
#undef MM
} /* end of _OlTopOfMenuStack */

/*
 * _OlTopParentOfMenuStack
 *
 */
extern Widget
_OlTopParentOfMenuStack OLARGLIST((w)) 
	OLGRA( Widget,		w)
{ 
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	return(II < 0 ? NULL : MM[II].parent_widget);

#undef II
#undef MM
} /* end of _OlTopParentOfMenuStack */

/*
 * _OlParentsInMenuStack -
 */
extern Cardinal
_OlParentsInMenuStack OLARGLIST((w, list))
	OLARG( Widget,		w)
	OLGRA( WidgetList *,	list)
{
	DbToken			token;

	token = DbFindToken(w);

#define II	TheStackIndex(token)
#define MM	(TheMenuStack(token))

	if (II >= 0)
	{
		Cardinal	i;
		WidgetList	stuff;

		stuff = (WidgetList)XtMalloc((II+1) * sizeof(Widget));

		*list = stuff;
		for (i = 0; i <= II; i++)
		{
			stuff[i] = MM[i].parent_widget;
		}
	}

	return(II+1);

#undef II
#undef MM
} /* end of _OlParentsInMenuStack */

/*
 *************************************************************************
 * PinPutIn - whenever the pushpin is put into its hole, this routine
 * is called.
 *	The pushpin we are talking about is the pushpin that the menu
 * owns, not the pushpin that the window manager owns.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
PinPutIn(menu)
	PopupMenuShellWidget	menu;
{
	static Arg notify[] = {
		{ XtNoverrideRedirect,	(XtArgVal) False },
		{ XtNpushpin,		(XtArgVal) OL_IN }
	};
	Display	*		dpy;
	Window			win;
	Widget			base_shell =
					_OlRootParentOfMenuStack((Widget)menu);
	Widget			RootMenu = _OlRootOfMenuStack((Widget)menu);
	XSizeHints 		sh;
	XWMHints *		wmh;
	Boolean			move_window = False;
	OlVendorPartExtension 	vendor = _OlGetVendorPartExtension((Widget)menu);

	if (menu == (PopupMenuShellWidget) NULL)
		return;

			/* Realize the menu now since we know it will
			 * be done down below anyway -- so we can
			 * code assuming we have a window.		*/

	if (XtIsRealized((Widget)menu) == False) {
		XtRealizeWidget((Widget)menu);
	}

	dpy		= XtDisplay((Widget) menu);
	win		= XtWindow((Widget) menu);

	if (menu->shell.popped_up == True) {
			/* If the menu is popped up, unmap it
			 * so we can reset some of the window
			 * hints (i.e., the window manager can
			 * only see the new hints when the
			 * window maps).			*/

		XUnmapWindow(dpy, win);
		XSync(dpy, (Bool)0);
		move_window = True;
	}

	wmh	= &(menu->wm.wm_hints);

				/* Set the Window Group of the menu
				 * if it's not already set		*/

	/*  Make sure it is an application shell.  */
	base_shell = _OlGetShellOfWidget(base_shell);

	if (base_shell != (Widget)NULL) {
			wmh->window_group	= (XID) XtWindow(base_shell);
			wmh->flags		|= WindowGroupHint;
	}

				/* Always set the input flag to false
				 * since we somehow keep dropping this	*/

	wmh->flags	|= InputHint;
	wmh->input	= (Bool)False;

						/* Apply the hints	*/
	XSetWMHints(dpy, win, wmh);

				/* Set the menu to be pinned		*/
 
	if (RootMenu != (Widget)NULL) {
		/* Remove all menus in the cascade except for this
		 * one.  Also, don't attempt to reset the input focus	*/

		_OlPopdownCascade(RootMenu, False);
	}

			/* Change the Override Redirect flag before
			 * doing before unmanaging the form's children
			 * since in R3, the shell's core geometry fields
			 * are not updated until the WindowManager gives
			 * us a ConfigureNotify Event.  We can't wait
			 * that long, so turn off the override.		*/

	XtSetValues((Widget) menu, notify, XtNumber(notify));

	OlSimpleLayoutWidget ((Widget) menu, True, False);

				/* Move the menu to the left to
				 * compensate for the decoration set.
				 * Don't do anything to the height
				 * since the above resize request
				 * already handled this.		*/

#define DECOR_H_MARGIN		4
	if (move_window == (Boolean)True) {
		Position margin = OlScreenPointToPixel(OL_HORIZONTAL,
				DECOR_H_MARGIN, XtScreen((Widget)menu));
		XtMoveWidget((Widget)menu, (menu->core.x - margin),
				menu->core.y);
	}

				/* Set the size hints for the window
				 * manager.				*/

	*((struct _OldXSizeHints *)&sh) = menu->wm.size_hints;
	sh.base_width = menu->wm.base_width;
	sh.base_height = menu->wm.base_height;
	sh.win_gravity = menu->wm.win_gravity;
	sh.flags	&= ~PPosition;
	sh.flags	|= USPosition;
	sh.x		= (int)menu->core.x;
	sh.y		= (int)menu->core.y;

	XSetNormalHints(dpy, win, &sh);

				/* Now determine what to do with the
				 * menu on screen			*/

	if (menu->shell.popped_up == False) {
		XtPopup((Widget)menu, XtGrabNone);
	}
	else {			/* Menu is already up	*/
		XMapWindow(dpy, win);
	}
} /* END OF PinPutIn() */

/****************************procedure*header*****************************
 *
 * WMMessageHandler - handles message from the window manager which
 * come in the form of Property or Client Messages, and property changes.
 * MenuShell looks for the WM_DELETE_WINDOW message so that it can transform
 * the menu back in to an override_redirect window.
 */
/* ARGSUSED */
static void
WMMessageHandler(widget, data, xevent, cont_to_dispatch)
        register Widget widget;
        XtPointer       data;
        XEvent *        xevent;
        Boolean *       cont_to_dispatch;
{
	if (xevent->xclient.message_type == XA_WM_PROTOCOLS(XtDisplay(widget))&&
		xevent->xclient.data.l[0] == XA_WM_DELETE_WINDOW(XtDisplay(widget))) {
		PopdownLayout(widget);
	}

}  /* end of WMMessageHandler() */

/****************************procedure*header*****************************
 *
 * PopdownLayout - a central place to do all the things necessary when
 * the menu goes from a pinned to unpinned state.
 */
static void
PopdownLayout OLARGLIST((w))
	OLGRA(Widget, w)
{
	/*  The pinned window has been dismissed by the user. */
	Arg args[2];

	XtPopdown(w);

	/*  Go through set values to notify the window manager.
	    The vendor will do a query geometry which will in turn
	    cause the menu to layout.  */
	XtSetArg(args[0], XtNoverrideRedirect, True);
	XtSetArg(args[1], XtNpushpin, OL_OUT);
	XtSetValues(w, args, 2);

	/*  Now, layout the widget so that it can resize itself to
	    have room for the title and pushpin.  */
	OlSimpleLayoutWidget(w, True, False);

}  /*  end of PopdownLayout() */
