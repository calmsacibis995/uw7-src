#ifndef	NOIDENT
#ident	"@(#)notice:Modal.c	1.21"
#endif

/*******************************file*header*******************************
    Description: Modal.c - OPEN LOOK(TM) Modal Shell Widget
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/LayoutExtP.h>
#include <Xol/ModalP.h>
#include <Xol/VendorI.h>
#include <Xol/EventObjP.h>
#include <Xol/Flat.h>

#define ClassName Modal
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define MODAL_C(wc) (((ModalShellWidgetClass)(wc))->modal_class)

#define MODAL_P(w) ((ModalShellWidget)(w))->modal
#define MPART(w)		( &((ModalShellWidget)(w))->modal_shell )

#define SET_BASE	1
#define SET_EMANATE	2

/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */
static void	SetBusyState OL_ARGS((
	ModalShellWidget
));
static void	DestroyCB OL_ARGS((
    Widget		w,
    XtPointer		closure,
    XtPointer		call_data
));
static void	PopdownCB OL_ARGS((
    Widget		w,
    XtPointer		closure,
    XtPointer		call_data
));
static void	PopupCB OL_ARGS((
    Widget 	w,
    XtPointer closure,
    XtPointer call_data
));
static void	PositionModal OL_ARGS((
	ModalShellWidget
));

/* Look and feel dependent procedures */
static void (*_olmMDAddEventHandlers) OL_ARGS((
	Widget
));
static Boolean	(*_olmMDCheckSetValues) OL_ARGS((
	ModalShellWidget,
	ModalShellWidget
));
static void	(*_olmMDRemoveEventHandlers) OL_ARGS((
	Widget
));
						/* class procedures */
static Boolean		ActivateWidget OL_ARGS((
	Widget,
	OlVirtualName,
	XtPointer
));
static void		ClassInitialize OL_ARGS((
	void
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		WMMsgHandler OL_ARGS((
	Widget,
	OlDefine,
	OlWMProtocolVerify *
));
						/* public procedures */

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Open Look mode defaults */
static Boolean menu_button_default = False;
static Boolean resize_corners_default = False;
static Boolean window_header_default = False;


#ifdef DEBUG
Boolean _NDebug = False;
#define DPRINT(x) if (_Ndebug == True) (void)fprintf x
#else
#define DPRINT(x)
#endif


/***********************widget*translations*actions***********************
    Translations and Actions
*/
/* None */

/****************************widget*resources*****************************
 *
 * Modal Resources
 */

#define OFFSET(field)  XtOffsetOf(ModalShellRec, modal_shell.field)

static XtResource resources[] =
  {
    { XtNemanateWidget, XtCEmanateWidget, XtRWidget, sizeof(Widget),
	  OFFSET(emanate), XtRWidget, (XtPointer)NULL },

    { XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
	  OFFSET(warp_pointer), XtRImmediate, False },

    { XtNnoticeType, XtCNoticeType, XtROlDefine, sizeof(OlDefine),
	OFFSET(noticeType), XtRImmediate, (XtPointer)OL_ERROR },
  };

#undef OFFSET

/*************************************************************************
    Define Class Extension Resource List
*/
#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource ext_resources[] =
  {
    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRBoolean, (XtPointer)&menu_button_default},

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRBoolean, (XtPointer)&resize_corners_default},

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer) OL_NONE },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRBoolean, (XtPointer)&window_header_default},

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_NOTICE },
  };

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

static LayoutCoreClassExtensionRec layout_extension_rec = {
        NULL,                                   /* next_extension       */
        NULLQUARK, /* see ClassInit */          /* record_type          */
        OlLayoutCoreClassExtensionVersion,      /* version              */
        sizeof(LayoutCoreClassExtensionRec),    /* record_size          */
	NULL, /* see ClassInit */		/* layout		*/
	NULL					/* query_alignment	*/
};

static OlVendorClassExtensionRec vendor_extension_rec =
  {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,                          /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        (OlSetDefaultProc)NULL,                 /* set_default          */
	NULL,					/* get_default 		*/
	NULL,					/* destroy		*/
	NULL,					/* initialize		*/
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        XtInheritTraversalHandler,              /* traversal handler    */
        XtInheritHighlightHandler,    		/* highlight handler    */
        ActivateWidget,				/* activate function    */
        NULL,              			/* event_procs          */
        0,              			/* num_event_procs      */
        NULL,					/* part_list            */
	{ NULL, 0 },				/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	False,					/* override_callback	*/
  }, *vendor_extension = &vendor_extension_rec;

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

ModalShellClassRec modalShellClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &transientShellClassRec,
    /* class_name		*/	"ModalShell",
    /* widget_size		*/	sizeof(ModalShellRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	True,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
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
  },{
/* composite_class fields	*/
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	_OlDefaultChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },{
/* shell_class fields		*/
					NULL
  },{
/* WHShell_class fields		*/
					NULL
  },{
/* VendorShell_class fields	*/
					(XtPointer)&vendor_extension_rec
  },{
/* TransientShell_class fields	*/
					NULL
  },{
/* ModalShell_class fields	*/
					NULL
  }
};

WidgetClass modalShellWidgetClass = (WidgetClass)&modalShellClassRec;


/***************************private*procedures****************************
 *
 * Private Functions
 *
 */

/******************************function*header****************************
 * SetBusyState-  change busy state of emanate widget and the base shell
 *	only if the user hasn't already set it.
 */
static void
SetBusyState OLARGLIST((mw))
	OLGRA(ModalShellWidget, mw)
{
    Arg		state[1];
    Widget	base;
    Boolean	busy_state;
    OlVendorPartExtension  vendor;
    Widget *	list;
    Cardinal    num_shells;
    int		i;
    static _OlArrayDef(WidgetArray, busy_list);

    XtSetArg(state[0], XtNbusy, True);


    /* [un]busy ([un]freeze) application's base window
     * 1st, get base window (child of root)
     */
    for (base = (Widget)mw->modal_shell.emanate;
	 XtParent(base) != NULL; base = XtParent(base))
	;

    if (XtIsRealized(base))  {
	XtVaGetValues(base, XtNbusy, &busy_state, 0);
	if (!busy_state)  {
		XtSetValues(base, state, 1);
		_OlArrayAppend(&busy_list, base);
	}
    }

    vendor = _OlGetVendorPartExtension(base);
    list = vendor->shell_list->shells;

    if ((num_shells = vendor->shell_list->nshells) > (Cardinal) 0)  {
	for (i = 0; i < num_shells; i ++ )  {
		Widget shell = list[i];
		if (XtIsTransientShell(shell) &&
			!XtIsSubclass(shell, modalShellWidgetClass) &&
			shell != base &&
			((ShellWidget)shell)->shell.popped_up)  {
			XtVaGetValues(shell, XtNbusy, &busy_state, 0);
			if (!busy_state)  {
				XtSetValues(shell, state, 1);
				_OlArrayAppend(&busy_list, shell);
			}
		}
	}
    }
	
    if (!_OlIsFlat(mw->modal_shell.emanate))  {
	XtVaGetValues(mw->modal_shell.emanate, XtNbusy, &busy_state, 0);
    	if (!busy_state)  {
    		XtSetValues(mw->modal_shell.emanate, state, XtNumber(state));
		_OlArrayAppend(&busy_list, mw->modal_shell.emanate);
    	}
    }

    /*  If we set either the base or the emanate, then we must unset them
	in the popdown callback.  */
    if (_OlArraySize(&busy_list) > 0)  {
	XtAddCallback((Widget)mw, XtNpopdownCallback, PopdownCB,
		(XtPointer)&busy_list);
	XtAddCallback((Widget)mw, XtNdestroyCallback, DestroyCB,
		(XtPointer)&busy_list);
    }
    else
	_OlArrayFree(&busy_list);

}  /* end of SetBusyState() */



/******************************function*header****************************
 * PositionModal()
 */

static void
PositionModal OLARGLIST((mw))
    OLGRA(ModalShellWidget, mw)
{
    Widget	emanateW = mw->modal_shell.emanate;
    int		emanate_x, emanate_y;	/* must be int's */
    Dimension	emanate_w, emanate_h;
    Position	modal_x, modal_y;
    Dimension	modal_w, modal_h;
    Dimension	screen_w, screen_h;
    int		delta;			/* must be signed */
    Window	junkWin;
    Boolean	shifted_left;
    Boolean	use_emanate;

    /* hopefully, Xt doesn't reference base_width, base_height, and
	win_gravity, see ShellP.h:WMShellPart and Xlib.h:XSizeHints
	for details.
    */
    XSizeHints *size_hints = (XSizeHints *) &(mw->wm.size_hints);

    /* Modal must be Realized (geometry is needed and must
	set olwm size hints).
    */
    if (!XtIsRealized((Widget)mw))
	XtRealizeWidget((Widget)mw);

    /* Get origin of emanate widget relative to root window.  If emanate
	widget has no window (not realized) or its (window) parent is
	root, don't even try to translate coordinates
     */
    if (XtIsRealized(emanateW) &&
	(XtWindowOfObject(emanateW) != RootWindowOfScreen(XtScreen(mw))) &&
	OlGetGui() == OL_OPENLOOK_GUI)
    {
	XTranslateCoordinates(XtDisplay(mw), XtWindowOfObject(emanateW),
			      RootWindowOfScreen(XtScreen(mw)), 0, 0,
			      &emanate_x, &emanate_y, &junkWin);
	use_emanate = True;

	/* If 'emamante' is a gadget, coord's are actually parent's coord's.
	   Must adjust to gadget coord's.
	*/
	if (_OlIsGadget(emanateW))
	{
	    emanate_x += emanateW->core.x;
	    emanate_y += emanateW->core.y;
	}

    } else {
	use_emanate = False;
    }

    screen_w	= _OlScreenWidth((Widget)mw);
    screen_h	= _OlScreenHeight((Widget)mw);
    modal_w	= _OlWidgetWidth((Widget)mw);
    modal_h	= _OlWidgetHeight((Widget)mw);

    if (use_emanate)
    {
	emanate_w = _OlWidgetWidth(emanateW);
	emanate_h = _OlWidgetHeight(emanateW);

	modal_x = emanate_x + emanate_w;	/* right of control */
	modal_y = (Position)emanate_y;		/* level w/control */

	/* consider horizontal axis (x) first */
	delta = screen_w - modal_w;
	if (modal_x > delta)			/* too wide */
	{
	    modal_x = (Position)delta;		/* shift left */
	    shifted_left = True;
	    modal_y += emanate_h;		/* drop below control */

	} else {
	    shifted_left = False;
	}

	/* consider vertical axis (y) */
	delta = screen_h - modal_h;
	if (modal_y > delta)			/* too tall */
	{
	    if (shifted_left)
	    {
		modal_y = emanate_y - modal_h; /* above control */
		if (modal_y < 0)		/* still no good */
		{
		    modal_y = (Position)delta; /* bottom of screen */
		    delta = emanate_x - modal_w;
		    if (delta > 0)		/* left of control */
			modal_x = (Position)delta;
		}

	    } else {
		modal_y = (Position)delta; /* shift up */
	    }
	}

    } else {			/* don't use emanate: center modal */
	modal_x = (Position)(screen_w - modal_w) / (Position)2;
	modal_y = (Position)(screen_h - modal_h) / (Position)2;
    }

    XtMoveWidget((Widget)mw, modal_x, modal_y);

    /* Let olwm know about Modals' position.  Shouldn't have to set
	size but without it, Modal comes up very large (!).
    */
    size_hints->flags	= USPosition | PPosition | USSize | PSize;
    size_hints->x	= mw->core.x;
    size_hints->y	= mw->core.y;
    size_hints->width	= mw->core.width;
    size_hints->height	= mw->core.height;
    XSetNormalHints(XtDisplay(mw), XtWindow(mw), size_hints);
}  /* end of PositionModal() */

/*************************************************************************
 *
 * Class Procedures
 *
 */

/******************************function*header****************************
 * Destroy-
 */
static void
Destroy(w)
    Widget w;
{
	ModalShellWidget mw = (ModalShellWidget) w;

	(*_olmMDRemoveEventHandlers)(w);
	if (mw->modal_shell.attrs != NULL) {
		OlgDestroyAttrs(mw->modal_shell.attrs);
	}
	if (mw->modal_shell.pixmap != NULL)  {
		XFreePixmap(XtDisplay(mw), mw->modal_shell.pixmap);
	}

}  /* end of Destroy() */

/****************************procedure*header*****************************
    ActivateWidget - this procedure allows external forces to activate this
    widget indirectly.
*/
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST((w, type, data))
    OLARG(Widget,		w)
    OLARG(OlVirtualName,	type)
    OLGRA(XtPointer,		data)
{
    Boolean	ret_val = False;

    if (type == OL_DEFAULTACTION)
    {
	ret_val = True;
	w = _OlGetDefault(w);

	if (w != (Widget)NULL)
	    OlActivateWidget(w, OL_SELECTKEY, (XtPointer)OL_NO_ITEM);

    } else if (type == OL_CANCEL) {
	ret_val = True;

	_OlBeepDisplay(w, 1);

    }

    return(ret_val);
} /* end of ActivateWidget() */

static void
WMMsgHandler OLARGLIST((w, action, wmpv))
    OLARG(Widget, w)
    OLARG(OlDefine, action)
    OLGRA(OlWMProtocolVerify *, wmpv)
{
    if (wmpv->msgtype == OL_WM_DELETE_WINDOW)
    {
	switch(action)
	{
	case OL_DEFAULTACTION:
	case OL_DISMISS:
	    XtPopdown(w);
	    break;
	case OL_QUIT:
	    XtUnmapWidget(w);
	    exit(0);
	    break;
	case OL_DESTROY:
	    XtDestroyWidget(w);
	    break;
	}
    }
}  /* end of WMMsgHandler() */

static void
ClassInitialize()
{
    ModalShellClassRec * wc = (ModalShellClassRec *) modalShellWidgetClass;


    _OlAddOlDefineType ("error",  OL_ERROR);
    _OlAddOlDefineType ("information",  OL_INFORMATION);
    _OlAddOlDefineType ("question",  OL_QUESTION);
    _OlAddOlDefineType ("warning",  OL_WARNING);
    _OlAddOlDefineType ("working",  OL_WORKING);
    if (OlGetGui() == OL_MOTIF_GUI)
	menu_button_default = resize_corners_default = 
		window_header_default = (Boolean)True;

	/* dynamically link in the GUI dependent routines */
        OLRESOLVESTART
        OLRESOLVE(MDLayout, layout_extension_rec.layout)
        OLRESOLVE(MDCheckSetValues, _olmMDCheckSetValues)
        OLRESOLVE(MDRemoveEventHandlers, _olmMDRemoveEventHandlers)
        OLRESOLVE(MDRedisplay, wc->core_class.expose)
        OLRESOLVEEND(MDAddEventHandlers, _olmMDAddEventHandlers)

    layout_extension_rec.record_type = XtQLayoutCoreClassExtension;
    vendor_extension->header.record_type = OlXrmVendorClassExtension;
}  /* end of ClassInitialize() */

/******************************function*header****************************
 * Initialize():  override any superclass resources of interest and
 *		establish own resource defaults.
 */

/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
    OLARG(Widget,		request)
    OLARG(Widget,		new)
    OLARG(ArgList,		args)
    OLGRA(Cardinal *,		num_args)
{
    ModalShellWidget newmw = (ModalShellWidget) new;

    newmw->modal_shell.pixmap = NULL;
    newmw->modal_shell.line_y = 0;

    /* add callbacks so we know when we're being popped up and down */
    XtAddCallback(new, XtNpopupCallback, PopupCB, NULL);

    /*  create olg attributes to draw the border for Open Look and the
	separater for Motif. */
    newmw->modal_shell.attrs = OlgCreateAttrs(
		XtScreen(new),
		(Pixel) 0,	/* foreground ignored */
		(OlgBG *)&(new->core.background_pixel),
		False,		/* background is a pixel */
		OL_DEFAULT_POINT_SIZE);

    /*  Add event handlers to beep and warp pointer. */
    (*_olmMDAddEventHandlers)(new);

}  /* end of Initialize() */

/******************************function*header****************************
    SetValues- check for attempts to set read-only resources.  Pass on
    certain resource changes to Text.
*/

/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
    OLARG(Widget,	current)
    OLARG(Widget,	request)
    OLARG(Widget,	new)
    OLARG(ArgList,	args)
    OLGRA(Cardinal *,	num_args)
{
    Boolean	redisplay = False;
    Boolean	layout = False;
    ModalShellPart * newPart = MPART(new);

	layout = (*_olmMDCheckSetValues)((ModalShellWidget)current,
						(ModalShellWidget)new);

	if (new->core.background_pixel != current->core.background_pixel)  {
		if (newPart->attrs != NULL)
			OlgDestroyAttrs(newPart->attrs);
		newPart->attrs = OlgCreateAttrs(XtScreen(new),
			(Pixel) 0,
			(OlgBG *)&(new->core.background_pixel),
			False,
			OL_DEFAULT_POINT_SIZE);
		redisplay = True;
	}

	/*  Layout procedure is only called at the end of the SetValues
	    chain to avoid extra calls.  */
	OlLayoutWidgetIfLastClass(new, modalShellWidgetClass, layout, False);

	return (redisplay || layout);
}  /* end of SetValues() */

/*************************************************************************
 *
 * Action Procedures
 *
 */

/******************************function*header****************************
 * PopdownCB():  Modal pops down automatically like other OPEN LOOK
 *	popup windows.  This is done by using button postSelect callback
 * (specified when control area is created (see Initialize)).
 * The Xt-supplied callback procedure, XtCallbackPopdown, is used to
 * simply pop the Modal down.  PopdownCB is called when the Modal pops
 * down.  Here is where the pointer should be unwarped and the control &
 * application window un-busied.
 */
/* ARGSUSED */
static void
PopdownCB OLARGLIST((w, closure, call_data))
    OLARG(Widget,		w)
    OLARG(XtPointer,		closure)
    OLGRA(XtPointer,		call_data)
{
    WidgetArray *	busy_list = (WidgetArray *) closure;
    Arg		state[1];
    int i;

/*  WARPING
    ModalShellPart * mPart = &(mw->modal_shell);

    if (mPart->warp_pointer && mPart->do_unwarp)
	XWarpPointer(XtDisplay(mw), None,
		     RootWindowOfScreen(XtScreen(mw)), 0, 0, 0, 0,
		     mPart->root_x, mPart->root_y);
*/

    /* "unbusy" the appropriate things */
	XtSetArg(state[0], XtNbusy, False);

    for (i = 0; i < _OlArraySize(busy_list); i++)  {
	XtSetValues((Widget) _OlArrayElement(busy_list, i), state, 1);
    }

    /*  Remove this callback.  Another will be added the next time it
	is popped up.  */
    XtRemoveCallback(w, XtNpopdownCallback, PopdownCB, (XtPointer)closure);
    XtRemoveCallback(w, XtNdestroyCallback, DestroyCB, (XtPointer)closure);

    /*  Free the array of busy widgets.  */
    _OlArrayFree(busy_list);

}  /* end of PopdownCB() */

/* ARGSUSED */
static void
DestroyCB OLARGLIST((w, closure, call_data))
    OLARG(Widget,		w)
    OLARG(XtPointer,		closure)
    OLGRA(XtPointer,		call_data)
{
    WidgetArray *	busy_list = (WidgetArray *) closure;

    _OlArrayFree(busy_list);
}  /*  end of DestroyCB() */


/******************************function*header****************************
 * PopupCB():  called when Modal is popped up.
 */
/* ARGSUSED */
static void
PopupCB OLARGLIST((w, closure, call_data))
    OLARG(Widget, w)
    OLARG(XtPointer, closure)
    OLGRA(XtPointer, call_data)
{
    ModalShellWidget	mw = (ModalShellWidget)w;
    ModalShellPart *	mPart = &(mw->modal_shell);

    if (mPart->emanate == NULL)
	mPart->emanate = XtParent(mw);

    /* "busy" the appropriate things */
    SetBusyState(mw);

    PositionModal(mw);

/*  WARPING
    if (mPart->warp_pointer)
    {
        Widget		def;
	Window		junkWin;
	int		junkPos;
	unsigned int	junkMask;
	Position	x, y;

	XQueryPointer(XtDisplay(mw), XtWindow(mw), &junkWin, &junkWin,
		      &(mPart->root_x), &(mPart->root_y),
		      &junkPos, &junkPos, &junkMask);

	x = _OlXTrans(def, def->core.width/2);
	y = _OlYTrans(def, def->core.height - 1);

	XWarpPointer(XtDisplay(mw), None, XtWindowOfObject(def), 0, 0,
		     0, 0, x, y);
	mPart->do_unwarp = True;
    }
*/

    /* Set windowGroup to group Modal with its "application" window.  Use
	SetValues on XtNwindowGroup since this is a resource of the shell.
	Rely on Shell to use proper protocol.
    */
    if (mPart->emanate != NULL &&
	XtWindowOfObject(mPart->emanate) !=
	RootWindowOfScreen(XtScreenOfObject(mPart->emanate)))
    {
	Arg wm_arg[1];
	XtSetArg(wm_arg[0], XtNwindowGroup,
		 XtWindow(_OlGetShellOfWidget(mPart->emanate)));
	XtSetValues((Widget)mw, wm_arg, XtNumber(wm_arg));
    }
}  /* end of PopupCB() */
