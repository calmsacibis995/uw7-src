#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevButt.c	1.16"
#endif

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the AbbreviatedButton widget.
 *
 *************************************************************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlCursors.h>
#include <Xol/AbbrevButP.h>
#include <Xol/PopupMenuP.h>
#include <Xol/FButtons.h>

#define ClassName AbbrevButt
#include <Xol/NameDefs.h>

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

static void GetNormalGC OL_ARGS ((AbbreviatedButtonWidget));
static void GetDimensions OL_ARGS ((Widget, Boolean, Boolean));
static void StartPreview OL_ARGS ((Widget));
static void StopPreview OL_ARGS ((Widget, Boolean));
static void PutUpMenu OL_ARGS ((Widget, Boolean, int, int));
static void TakeDownMenu OL_ARGS ((Widget, int, int));
static void RevertButton OL_ARGS ((Widget));

					/* class procedures		*/

static void             ClassInitialize OL_NO_ARGS();
static void Initialize OL_ARGS ((Widget, Widget, ArgList, Cardinal *));
static void Destroy OL_ARGS ((Widget));
static void Redisplay OL_ARGS ((Widget, XEvent *, Region));
static Boolean SetValues OL_ARGS ((Widget, Widget, Widget, ArgList,
				   Cardinal *));
static void HighlightHandler OL_ARGS ((Widget, OlDefine));
static Boolean ActivateWidget OL_ARGS ((Widget, OlVirtualName, XtPointer));

					/* action procedures		*/

static void HandleEvent OL_ARGS ((Widget, OlVirtualEvent));
static void HandleKey OL_ARGS ((Widget, OlVirtualEvent));

					/* public procedures		*/
/* There are no public routines */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static OlEventHandlerRec
handlers[] = {
    { ButtonPress,	HandleEvent },
    { ButtonRelease,	HandleEvent },
    { EnterNotify,	HandleEvent },
    { LeaveNotify,	HandleEvent },
    { KeyPress,		HandleKey   },
};

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static char
translations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
";

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define AbOFFSET(field)	XtOffsetOf(AbbreviatedButtonRec,		\
					abbreviated_button.field)

static XtResource
resources[] = {

    { XtNpopupWidget, XtCPopupWidget, XtRWidget, sizeof(Widget),
	  AbOFFSET(popup), XtRWidget, (XtPointer)NULL
    },

    { XtNpreviewWidget, XtCPreviewWidget, XtRWidget, sizeof(Widget),
	  AbOFFSET(preview_widget), XtRWidget, (XtPointer)NULL
    },

    { XtNscale, XtCScale, XtRInt, sizeof(int),
	  AbOFFSET(scale), XtRImmediate, (XtPointer)OL_DEFAULT_POINT_SIZE
    },

    { XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),
	  AbOFFSET(button_type), XtRImmediate, (XtPointer)OL_MENU_BTN
    },
};
#undef AbOFFSET

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

AbbreviatedButtonClassRec
abbreviatedButtonClassRec = {
  {
	(WidgetClass) &primitiveClassRec,	/* superclass		*/
	"AbbreviatedButton",			/* class_name		*/
	sizeof(AbbreviatedButtonRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	XtInheritResize,			/* resize		*/
	Redisplay,				/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	translations,				/* tm_table		*/
	XtInheritQueryGeometry			/* query_geometry	*/
  },	/* End of CoreClass field initializations */
  {
        False,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	handlers,				/* event_procs		*/
	XtNumber(handlers),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },	/* End of Primitive field initializations */
  {
	NULL,					/* field not used	*/
  }	/* End of AbbreviatedButtonClass field initializations */
}; 

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass abbreviatedButtonWidgetClass =
		(WidgetClass)&abbreviatedButtonClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetNormalGC - this routine gets the normal GC for the Abbreviated
 * Flat Menu Button Widget.
 ****************************procedure*header*****************************
 */
static void
GetNormalGC OLARGLIST((abw))
    OLGRA( AbbreviatedButtonWidget,	abw)
{
    Pixel	focus_color;
    Boolean	has_focus;

    /* Destroy existing GC	*/

    if (abw->abbreviated_button.pAttrs != (OlgAttrs *)NULL) {
	OlgDestroyAttrs (abw->abbreviated_button.pAttrs);
    }

    focus_color = abw->primitive.input_focus_color;
    has_focus = abw->primitive.has_focus;

    if (has_focus)
    {
	if (abw->primitive.foreground == focus_color ||
	    abw->core.background_pixel == focus_color)
	{
	    /* reverse fg and bg. */
	    if (OlgIs3d ())
	    {
		abw->abbreviated_button.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject((Widget)abw),
				    abw->core.background_pixel,
				    (OlgBG *)&(abw->primitive.foreground),
				    False, abw->abbreviated_button.scale);
	    }
	    else
	    {
		abw->abbreviated_button.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject((Widget)abw),
				    abw->primitive.foreground,
				    (OlgBG *)&(abw->core.background_pixel),
				    False, abw->abbreviated_button.scale);
	    }
	}
	else
	    abw->abbreviated_button.pAttrs =
		OlgCreateAttrs (XtScreenOfObject((Widget)abw),
				abw->primitive.foreground,
				(OlgBG *)&(focus_color),
				False, abw->abbreviated_button.scale);
    }
    else
	abw->abbreviated_button.pAttrs =
	    OlgCreateAttrs (XtScreenOfObject((Widget)abw),
			    abw->primitive.foreground,
			    (OlgBG *)&(abw->core.background_pixel),
			    False, abw->abbreviated_button.scale);

} /* End of GetNormalGC() */

/*
 ************************************************************
 *
 *  GetDimensions - this function returns the width and height
 *  of the widget visual as function of point size and screen resolution.
 *
 *********************function*header************************
 */

static void
GetDimensions OLARGLIST ((w, change_width, change_height))
    OLARG (Widget,	w)
    OLARG (Boolean,	change_width)
    OLGRA (Boolean,	change_height)
{
    if (change_width == True || change_height == True)
    {
	AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;
	Dimension	width, height;

	OlgSizeAbbrevMenuB (XtScreen (w), abw->abbreviated_button.pAttrs,
			    &width, &height);

	if (change_width == True)
	{
	    abw->core.width = width;
	}

	if (change_height == True)
	{
	    abw->core.height = height;
	}
    }
} /* End of GetDimensions() */


/*
 *************************************************************************
 * StartPreview - initiates previewing
 * This function is called when the select button is pressed, or when
 * it is pressed and the window is entered and power-user mode is on.
 ****************************procedure*header*****************************
 */

static void
StartPreview OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;

    if (!abw->abbreviated_button.set &&
			abw->abbreviated_button.button_type == OL_MENU_BTN)
    {
	_OlSetPreviewMode(w);
	abw->abbreviated_button.set	     =
	abw->abbreviated_button.previewing = True;
	Redisplay (w, NULL, NULL);

	if (!abw->abbreviated_button.popup ||
	    !_OlGetMenuDefault(abw->abbreviated_button.popup,
			      &abw->abbreviated_button.flat,
			      &abw->abbreviated_button.dflt_index,
			      (Boolean)True))
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   (Cursor) GetOlQuestionCursor (XtScreenOfObject(w)));
	    abw->abbreviated_button.err_flg = True;
	}
	else
	    if (abw->abbreviated_button.preview_widget)
	    {
		_OlPreviewMenuDefault(abw->abbreviated_button.popup, 
			abw->abbreviated_button.preview_widget, OL_NO_ITEM);
	    }
    }

} /* End of StartPreview() */

/*
 *************************************************************************
 * StopPreview - terminates previewing
 * This function is called when the select button is released, or when
 * it is pressed and the window is left and power-user mode is on.
 ****************************procedure*header*****************************
 */

static void
StopPreview OLARGLIST ((w, fire))
    OLARG (Widget,	w)
    OLGRA (Boolean,	fire)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;

    if (abw->abbreviated_button.set &&
			abw->abbreviated_button.button_type == OL_MENU_BTN)
    {
	if (abw->abbreviated_button.err_flg)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlStandardCursor (XtScreenOfObject(w)));
	    abw->abbreviated_button.err_flg = False;
	}
	else
	{
	    if (fire)
		OlActivateWidget (abw->abbreviated_button.flat,
				  OL_SELECTKEY, (XtPointer)
				  (abw->abbreviated_button.dflt_index + 1));

	     if (abw->abbreviated_button.preview_widget)
		_OlClearWidget (abw->abbreviated_button.preview_widget, True);
	}

	abw->abbreviated_button.set	     =
	abw->abbreviated_button.previewing = False;
	_OlResetPreviewMode(w);
	Redisplay (w, NULL, NULL);
    }

} /* End of StopPreview() */

/*
 *************************************************************************
 * PutUpMenu - posts menu
 * This function is called when the menu button is pressed, or when
 * it is pressed and the window is entered.  Also for select button when
 * not in power-user mode.
 ****************************procedure*header*****************************
 */

static void
PutUpMenu OLARGLIST ((w, stayup, x, y))
    OLARG (Widget,	w)
    OLARG (Boolean,	stayup)
    OLARG (int,		x)
    OLGRA (int,		y)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;
    Position	root_x, root_y;
    XRectangle	rect;

    if (!abw->abbreviated_button.set)
    {
	abw->abbreviated_button.set = True;
	Redisplay (w, NULL, NULL);
	if (!abw->abbreviated_button.popup)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   (Cursor) GetOlQuestionCursor (XtScreenOfObject(w)));
	    abw->abbreviated_button.err_flg = True;
	}
	else
	{
	    XtTranslateCoords (XtParent (w), abw->core.x, abw->core.y,
			       &root_x, &root_y);
	    rect.x = root_x;
	    rect.y = root_y;
	    rect.width = abw-> core.width;
	    rect.height = abw-> core.height + abw->core.border_width;
	    _OlResetPreviewMode(abw->abbreviated_button.popup);
	    _OlPopupMenu (abw->abbreviated_button.popup, w, RevertButton,
		       &rect, AbbrevDropDownAlignment, True, XtWindow(w),
		       (Position)x, (Position)y);
	    if (stayup)
		_OlSetStayupMode(abw->abbreviated_button.popup);
	    else {
			/* if got Button Release after calling  */
			/* _OlPopupMenu then we have to reset   */
			/* to pending stayup otherwise a menu   */
			/* will never show up if time interval  */
			/* between a Button Press and a Button  */
			/* Release is really short...           */
		if (_OlIsInStayupMode(abw->abbreviated_button.popup))
			_OlSetStayupModeValue(abw->abbreviated_button.popup,
				PENDING_STAYUP);
	    }
        }

	/* This AddGrab must be done after the menu is popped up.  The menu
	 * code will do an XtAddGrab (w, True, True), but the abbreviated
	 * menu button must be in the modal cascade to get enter/leave
	 * events.  The Menu code will remove this grab when the root menu
	 * is brought down.
	 */
	XtAddGrab (w, False, False);
    }
} /* End of PutUpMenu() */

/*
 *************************************************************************
 * TakeDownMenu - unposts menu
 * This function is called when the menu button is pressed and the
 * window is left.  Also for select button when not in power-user mode.
 * The menu is unposted only if the window is exited from the top or left.
 ****************************procedure*header*****************************
 */

static void
TakeDownMenu OLARGLIST ((w, x, y))
    OLARG (Widget,	w)
    OLARG (int,		x)
    OLGRA (int,		y)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;

    if (abw->abbreviated_button.set)
    {
	if (abw->abbreviated_button.err_flg)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlStandardCursor (XtScreenOfObject(w)));
	    abw->abbreviated_button.err_flg = False;

	    abw->abbreviated_button.set = False;
	    Redisplay (w, NULL, NULL);
	    XtRemoveGrab (w);
	}
	else
	{
	    if (x < 0 || y < 0)
	    {
		/* The menu code will remove the grab that we
		 * added and call us to redraw the menu button.
		 * So, just unpost the menu.
		 */
		OlUnpostPopupMenu(abw->abbreviated_button.popup);
	    }
	}
    }

} /* End of TakeDownMenu() */

/*
 *************************************************************************
 * RevertButton - redraws abbreviated menu button after menu is unposted
 ****************************procedure*header*****************************
 */

static void
RevertButton OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) w;

    abw->abbreviated_button.set = False;
    Redisplay (w, NULL, NULL);
} /* End of RevertButton() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *  ClassInitialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void 
ClassInitialize()
{
	AbbreviatedButtonClassRec *wc = (AbbreviatedButtonClassRec *)
					abbreviatedButtonWidgetClass;

	_OlAddOlDefineType ("menubtn", OL_MENU_BTN);
	_OlAddOlDefineType ("windowbtn", OL_WINDOW_BTN);

	if (OlGetGui() == OL_OPENLOOK_GUI)  {
		wc->primitive_class.highlight_handler = HighlightHandler;
	}

} /* end of ClassInitialize() */

/*
 *************************************************************************
 * Initialize - Initializes the AbbrevMenuButton Instance.  Any conflicts 
 * between the "request" and "new" widgets should be resolved here.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST ((request, new, args, num_args))
    OLARG (Widget,	request)	/* What user wants		*/
    OLARG (Widget,	new)		/* What user gets, so far....	*/
    OLARG (ArgList,	args)
    OLGRA (Cardinal *,	num_args)
{
    AbbreviatedButtonWidget	abw = (AbbreviatedButtonWidget) new;

    abw->abbreviated_button.pAttrs = (OlgAttrs *) NULL;
    GetNormalGC (abw);

    GetDimensions (new, abw->core.width == 0, abw->core.height == 0);

    abw->abbreviated_button.set = False;
    abw->abbreviated_button.err_flg = False;

    if (!abw->abbreviated_button.popup ||
	(abw->abbreviated_button.button_type == OL_MENU_BTN &&
	!XtIsSubclass(abw->abbreviated_button.popup,
			popupMenuShellWidgetClass)))
    {
	OlVaDisplayWarningMsg(XtDisplay(abw),
			      OleNfileAbbrevFMen,
			      OleTmsg1,
			      OleCOlToolkitWarning,
			      OleMfileAbbrevFMen_msg1,
			      abw->core.name);
	abw->abbreviated_button.popup = NULL;
    }

} /* End of Initialize() */

/*
 *************************************************************************
 * Destroy - free the GCs stored in Abbreviated Flat MenuButton widget
 * and remove callbacks.
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    OlgDestroyAttrs (((AbbreviatedButtonWidget) w)->abbreviated_button.pAttrs);

} /* End of Destroy() */

/*
 *************************************************************************
 * Redisplay - this routine draws the Abbreviated Menu Button.  The
 * image will be as big as the window.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay OLARGLIST ((w, xevent, region))
    OLARG (Widget,	w)
    OLARG (XEvent,	*xevent)
    OLGRA (Region,	region)
{
    AbbreviatedButtonWidget	abw;
    unsigned			flags;

    if (!XtIsRealized (w))
	return;

    abw = (AbbreviatedButtonWidget) w;
    
    /* the button is draw as set if it really is set
     * or if it is 2-D and has input focus and the input
     * focus color conflicts with either the foreground or
     * background color.
     */
    if (abw->abbreviated_button.set ||
	(!OlgIs3d () && abw->primitive.has_focus &&
	 (abw->primitive.input_focus_color ==
	  abw->core.background_pixel ||
	  (abw->primitive.input_focus_color ==
	   abw->primitive.foreground))))
	flags = AM_SELECTED;
    else
	flags = AM_NORMAL;
    if (abw->abbreviated_button.button_type == OL_WINDOW_BTN)
	flags |= AM_WINDOW;
    
    OlgDrawAbbrevMenuB (XtScreen (w), XtWindow (w),
			abw->abbreviated_button.pAttrs, 0, 0, flags);

} /* End of Redisplay() */

/*
 *************************************************************************
 * SetValues - used to set resources associated with the
 * AbbreviatedButtonPart.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST ((current, request, new, args, num_args))
    OLARG (Widget,	current)
    OLARG (Widget,	request)
    OLARG (Widget,	new)
    OLARG (ArgList,	args)
    OLGRA (Cardinal *,	num_args)
{
    Boolean			redisplay = False;
    AbbreviatedButtonWidget	cabw = (AbbreviatedButtonWidget) current;
    AbbreviatedButtonWidget	nabw = (AbbreviatedButtonWidget) new;

    if (nabw->primitive.foreground != cabw->primitive.foreground ||
	nabw->core.background_pixel != cabw->core.background_pixel ||
	nabw->primitive.input_focus_color !=
		cabw->primitive.input_focus_color)
    {
	GetNormalGC (nabw);
	redisplay = True;
    }

    if (nabw->abbreviated_button.popup != cabw->abbreviated_button.popup &&
	cabw->abbreviated_button.button_type == OL_MENU_BTN)
    {
	/* There is a problem if the menu is pinned.  But don't worry about
	 * this now.
	 */

	if (!nabw->abbreviated_button.popup ||
	    !XtIsSubclass(nabw->abbreviated_button.popup,
				popupMenuShellWidgetClass))
	{

	  OlVaDisplayWarningMsg(XtDisplay(nabw),
				OleNfileAbbrevFMen,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileAbbrevFMen_msg1,
				nabw->core.name);
	  nabw->abbreviated_button.popup = NULL;
	}

	if (nabw->abbreviated_button.set)
	{
	    if (nabw->abbreviated_button.previewing)
	    {
		StopPreview (current, False);
		StartPreview (new);
	    }
	    else
	    {
		TakeDownMenu (current, -1, 0);
		PutUpMenu (new, True, 0, 0);
	    }
	}
    }

    return (redisplay);

} /* End of SetValues() */

/*
 *************************************************************************
 * HighlightHandler - changes the colors when this widget gains or loses
 * focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
HighlightHandler OLARGLIST ((w, type))
    OLARG (Widget,	w)
    OLGRA (OlDefine,	type)
{
    GetNormalGC ((AbbreviatedButtonWidget) w);
    Redisplay (w, NULL, NULL);
} /* End of HighlightHandler() */

/*
 *************************************************************************
 * ActivateWidget - this routine provides the external interface for
 * others to activate this widget indirectly.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST ((w, type, data))
    OLARG( Widget,		w)
    OLARG( OlVirtualName,	type)
    OLGRA( XtPointer,		data)
{
    Boolean	ret_val = False;
    AbbreviatedButtonWidget abw = (AbbreviatedButtonWidget) w;

    switch (type) {
    case OL_SELECTKEY:
	if (abw->abbreviated_button.button_type == OL_WINDOW_BTN)  {
	    	ret_val = True;
		if (abw->abbreviated_button.popup)
			XtPopup(abw->abbreviated_button.popup, XtGrabNone);
		break;
	}
	else {  /*  a OL_MENU_BTN */
		if (_OlSelectDoesPreview (w)) {
	    		ret_val = True;
	    		StartPreview (w);
	    		StopPreview (w, True);
	    		break;
		}
		/* Fall Through! */
	}
	/* FALLTHROUGH */
    case OL_MENUKEY:
	ret_val = True;
	if (abw->abbreviated_button.button_type == OL_MENU_BTN &&
				abw->abbreviated_button.popup)
	    PutUpMenu (w, True, 0, 0);
	break;
    }

    return (ret_val);

} /* End of ActivateWidget() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * HandleEvent - handles all button press/release and enter/leave events
 ****************************procedure*header*****************************
 */

static void
HandleEvent OLARGLIST ((w, ve))
    OLARG (Widget,	w)
    OLGRA (OlVirtualEvent,	ve)
{
    AbbreviatedButtonWidget abw = (AbbreviatedButtonWidget) w;

    if ((ve->xevent->type == EnterNotify || ve->xevent->type == LeaveNotify) &&
	(ve->xevent->xcrossing.mode != NotifyNormal || ve->xevent->xcrossing.detail == NotifyNonlinear))
	return;		/* Ignore enter events generated by grabs */

    switch (ve->virtual_name) {
    case OL_SELECT:
	if (abw->abbreviated_button.button_type == OL_WINDOW_BTN)  {
	    	ve->consumed = True;
		if (abw->abbreviated_button.popup)  {
			switch (ve->xevent->type)  {
			case ButtonPress:
				_OlUngrabPointer (w);
				/*  FALLTHROUGH */
	    		case EnterNotify:
				abw->abbreviated_button.set = True;
				Redisplay (w, NULL, NULL);
				break;
			case ButtonRelease:
				XtPopup(abw->abbreviated_button.popup,
								XtGrabNone);
				/*  FALLTHROUGH */
	    		case LeaveNotify:
				abw->abbreviated_button.set = False;
				Redisplay (w, NULL, NULL);
				break;
			}
		}
		break;
	}
	else {  /* a OL_MENU_BTN */
		if (_OlSelectDoesPreview (w)) {
	    		ve->consumed = True;
	    		switch (ve->xevent->type) {
	    		case ButtonPress:
				_OlUngrabPointer (w);
				/* FALLTHROUGH */
		
	    		case EnterNotify:
				StartPreview (w);
				break;
		
	    		case ButtonRelease:
	    		case LeaveNotify:
				StopPreview (w, ve->xevent->type == ButtonRelease);
				break;
	    		}
	    		break;
		}
			/* FALLTHROUGH */
	}
		
    case OL_MENU:
	ve->consumed = True;
	if (abw->abbreviated_button.button_type == OL_MENU_BTN)  {
		switch (ve->xevent->type) {
		case ButtonPress:
	    		_OlUngrabPointer (w);
	    		PutUpMenu (w, False, ve->xevent->xbutton.x_root,
		       		ve->xevent->xbutton.y_root);
		    	break;
	
		case EnterNotify:
		    	PutUpMenu (w, False, ve->xevent->xcrossing.x_root,
			       	ve->xevent->xcrossing.y_root);
		    	break;
	
		case LeaveNotify:
		    	TakeDownMenu (w, ve->xevent->xcrossing.x, ve->xevent->xcrossing.y);
		    	break;
	
		case ButtonRelease:
		    	if (abw->abbreviated_button.err_flg)
				TakeDownMenu (w, 0, 0);
	    	break;
		}
	    break;
	}
    }

} /* End of HandleEvent() */

/*
 *************************************************************************
 * HandleKey - handles all keypresses for this widget
 ****************************procedure*header*****************************
 */

static void
HandleKey OLARGLIST ((w, ve))
    OLARG (Widget,	w)
    OLGRA (OlVirtualEvent,	ve)
{
    if (ve->virtual_name == OL_MOVEDOWN)
    {
	ve->consumed = True;
	OlActivateWidget(w, OL_MENUKEY, (XtPointer)NULL);
    }

} /* End of HandleKey() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public routines */
