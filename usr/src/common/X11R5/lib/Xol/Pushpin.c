/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)pushpin:Pushpin.c	1.47"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		OPEN LOOK(TM) Pushpin widget source code.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <Xol/OpenLookP.h>
#include <Xol/PushpinP.h>
#include <Xol/OblongButP.h>
#include <Xol/ButtonP.h>

#define ClassName Pushpin
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

static void	GetNewDimensions();	/* Calc. new window size	*/
static void	GetNormalGC();		/* Get pushpin's GC		*/
static unsigned	GetPinType();		/* Get pin's type (in/out/def)	*/
static void	previewCallback();	/* size or draw preview pin	*/
static void	PreviewManager();	/* Caches XImages		*/
static void	PreviewPushpin();	/* Previews Pushpin in another
					 * widget's window		*/

					/* class procedures		*/

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	ClassInitialize();	/* initializes widget class	*/
static void	Destroy();		/* Destroys the pushpin's data	*/
static void	Initialize();		/* initializes pushpin instance	*/
static void	Redisplay();		/* handles redisplay of window	*/
static Boolean	SetValues();		/* manages value changes	*/

					/* action procedures		*/

static void	Notify();		/* fires callback lists		*/
static void	SelectPin();		/* sets pushpin state		*/
static void	SetDefault OL_ARGS((Widget));
static void	UnselectPin();		/* Unsets the pushpin state	*/

					/* public procedures		*/

/* There are no public procedures */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define WORKAROUND	1		/* do not remove this line	*/

typedef struct _PreviewCache {
	struct _PreviewCache *	next;
	Screen *		screen;
	int			reference_count;
	Widget			shell;
	Widget			oblong;
} PreviewCache;

typedef struct {
    unsigned	callbackType;
    Screen	*scr;
    Drawable	win;
    OlgAttrs	*pAttrs;
    Position	x, y;
    Dimension	width, height;
    OlDefine	justification;
    Boolean	set;
    Boolean	sensitive;
} ProcLbl;		/* call data structure for drawing preview pins */

static PreviewCache *	cache_list = (PreviewCache *) NULL;

#define OFFSET(field) XtOffset(PushpinWidget, field)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 * Since the pushpin can be on a menu, the translations must work for
 * the MENU button as well as the SELECT button.
 *
 ***********************widget*translations*actions***********************
 */

static void	HandleButton();
static void	HandleCrossing();
static void	HandleMotion();
	
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
	<BtnMotion>:	OlAction()";

static OlEventHandlerRec event_procs[] =
{
	{ ButtonPress,		HandleButton	},
	{ ButtonRelease,	HandleButton	},
	{ EnterNotify,		HandleCrossing	},
	{ LeaveNotify,		HandleCrossing	},
	{ MotionNotify,		HandleMotion	},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = { 

   { XtNpushpinIn, XtCCallback, XtRCallback, sizeof(XtCallbackList), 
      OFFSET(pushpin.in_callback), XtRCallback, (XtPointer) NULL },

   { XtNpushpinOut, XtCCallback, XtRCallback, sizeof(XtCallbackList), 
      OFFSET(pushpin.out_callback), XtRCallback, (XtPointer) NULL },

   { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine), 
      OFFSET(pushpin.pin_state), XtRImmediate, (XtPointer) OL_OUT },

   { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean), 
      OFFSET(pushpin.is_default), XtRImmediate, (XtPointer) False },

   { XtNpreview, XtCPreview, XtRWidget, sizeof(Widget),
	OFFSET(pushpin.preview_widget), XtRWidget, (XtPointer) NULL },

   { XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int), 
      OFFSET(pushpin.shell_behavior), XtRImmediate, (XtPointer) BaseWindow },

   { XtNscale, XtCScale, XtRInt, sizeof(int), 
      OFFSET(pushpin.scale), XtRImmediate, (XtPointer) OL_DEFAULT_POINT_SIZE },

};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

PushpinClassRec
pushpinClassRec = {
  {
    (WidgetClass) &primitiveClassRec,	/* superclass		  */	
    "Pushpin",				/* class_name		  */
    sizeof(PushpinRec),			/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    XtInheritRealize,			/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    NULL,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    translations,			/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
  },	/* CoreClass fields initialization */
  {					/* primitive_class fields */
    True,				/* focus_on_select	*/
    NULL,				/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    ActivateWidget,			/* activate		*/
    event_procs,			/* event_procs		*/
    XtNumber(event_procs),		/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL				/* extension		*/
  },
  {
    NULL				/* field not used    	  */
  }  /* PushpinClass field initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass pushpinWidgetClass = (WidgetClass) &pushpinClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetNewDimensions - this routine calculates the new pushpin window
 * dimensions based on its "scale"
 ****************************procedure*header*****************************
 */
static void
GetNewDimensions(ppw, update_width, update_height)
	PushpinWidget	ppw;
	Boolean		update_width;
	Boolean		update_height;
{
	Dimension	width, height;

	OlgSizePushPin (XtScreen (ppw), ppw->pushpin.pAttrs, &width, &height);

	if (update_width == True)
	    ppw->core.width = width;

	if (update_height == True)
	    ppw->core.height = height;

} /* END OF GetNewDimensions() */

/*
 *************************************************************************
 * GetNormalGC - acquires the Graphics Context for normal pushpin state
 ****************************procedure*header*****************************
 */
static void
GetNormalGC(ppw)
	PushpinWidget ppw;
{
						/* Destroy existing GCs	*/

	if (ppw->pushpin.pAttrs != (OlgAttrs *) NULL)
		OlgDestroyAttrs (ppw->pushpin.pAttrs);

	ppw->pushpin.pAttrs = OlgCreateAttrs(
					XtScreen (ppw),
				      	ppw->primitive.foreground,
					(OlgBG *)&(ppw->core.background_pixel),
					False,
					ppw->pushpin.scale);
} /* END OF GetNormalGC() */

/*
 *************************************************************************
 * GetPinType - gets the correct type of pin to draw (in, out, or default).
 ****************************procedure*header*****************************
 */
static unsigned
GetPinType(ppw)
	PushpinWidget  ppw;
{
	unsigned	type;
	register PushpinPart *	pushpin = &(((PushpinWidget) ppw)->pushpin);

		/* Now determine which pin is to be drawn		*/

	if (pushpin->pin_state == (OlDefine)OL_IN) {
		if (pushpin->selected == (Boolean) True) 
					/* we want unpinned pushpin	*/
			if (pushpin->is_default == (Boolean) True)
				type = PP_DEFAULT;
			else
				type = PP_OUT;
		else
					/* we want pinned pushpin	*/
			type = PP_IN;
	}
	else {
		if (pushpin->selected == (Boolean) True) 
					/* we want pinned pushpin	*/
			type = PP_IN;
		else
			if (pushpin->is_default == (Boolean) True)
				type = PP_DEFAULT;
			else
				type = PP_OUT;
	}

	return(type);

} /* END OF GetPinType() */

/*
 *************************************************************************
 * previewCallback - Draw the pushpin in the preview button or determine
 * the size of a preview pin label.
 *************************************************************************
 */
/* ARGSUSED */
static void
previewCallback (w, client_data, call_data)
    Widget	w;
    XtPointer	client_data;
    XtPointer	call_data;
{
    ProcLbl	*lbl = (ProcLbl *) call_data;
    Dimension	width, height;

    switch (lbl->callbackType) {
    case OL_SIZE_PROC:
	OlgSizePushPin (lbl->scr, lbl->pAttrs, &lbl->width, &lbl->height);
	break;

    case OL_DRAW_PROC:
	OlgSizePushPin (lbl->scr, lbl->pAttrs, &width, &height);
	OlgDrawPushPin (lbl->scr, lbl->win, lbl->pAttrs, lbl->x, lbl->y,
			lbl->width, lbl->height, PP_IN);
	break;
    }
}

/*
 *************************************************************************
 * PreviewManager - this routine creates the shell and the oblong widget
 * that will be used preview the pushpin.  These are cached on a per
 * screen basis
 ****************************procedure*header*****************************
 */
static void
PreviewManager(ppw, destroy_mode)
	PushpinWidget	ppw;
	Boolean		destroy_mode;	/* Add or remove mode		*/
{
	PreviewCache *		self = (PreviewCache *) NULL;
	PreviewCache *		prev = (PreviewCache *) NULL;
	Screen *		screen = XtScreen((Widget) ppw);
	extern PreviewCache *	cache_list;

			/* Traverse the existing list to find a match	*/

	for (self = cache_list; self; self = self->next) {
		if (self->screen == screen)
			break;
		else
			prev = self;
	}

	if (destroy_mode == True) {		/* Delete mode		*/

		if (self == (PreviewCache *) NULL) {
		  OlVaDisplayWarningMsg(XtDisplay(ppw),
					OleNfilePushpin,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfilePushpin_msg1);
			return;
		}

		if (--self->reference_count == 0) {

				/* Destroy the previewing widgets.
				 * Destroying the shell will result in
				 * the death of the oblong child	*/

			XtDestroyWidget(self->shell);

			if (prev)
				prev->next = self->next;
			else
				cache_list = self->next;

			XtFree((char *) self);
		}

		return;
	}

						/* Create a new node	*/

	if (self == (PreviewCache *) NULL) {
		Arg	args[5];
		int	a;

						/* Create a new cache	*/

		self			= XtNew(PreviewCache);
		self->reference_count	= 0;
		self->screen		= screen;
		self->next		= cache_list;
		cache_list		= self;

		ppw->pushpin.preview_cache = (XtPointer) self;

				/* Create Widgets needed for previewing	*/
		a = 0;
		XtSetArg(args[a], XtNmappedWhenManaged, False);		++a;

			/*
			 * XtCreateApplicationShell uses application class
			 * name passed to XtInitialize and uses NULL as
			 * application name. Now we replaced both with
			 * "pushpin_hidden_shell". This should be OK
			 * this is a hidden shell.
			 */
		self->shell = XtAppCreateShell(
				"pushpin_hidden_shell",
				"pushpin_hidden_shell",
				topLevelShellWidgetClass,
				XtDisplayOfObject((Widget)ppw),
				args, a);

		a = 0;
		XtSetArg(args[a], XtNlabelJustify, OL_CENTER);		++a;
		XtSetArg(args[a], XtNrecomputeSize, False);		++a;
		XtSetArg(args[a], XtNmappedWhenManaged, False);		++a;
		XtSetArg(args[a], XtNlabelType, OL_PROC);		++a;
		XtSetArg(args[a], XtNbuttonType, OL_LABEL);		++a;
		self->oblong = XtCreateManagedWidget("pushpin_hidden_oblong",
				buttonWidgetClass, self->shell,
				args, a);

				/* Add callback to draw pushpin */
		XtAddCallback ((Widget)self->oblong, XtNlabelProc, previewCallback,
			       (XtPointer) 0);
	}

	++(self->reference_count);
	ppw->pushpin.preview_cache = (XtPointer) self;

} /* END OF PreviewManager() */

/*
 *************************************************************************
 * PreviewPushpin - this routine is called when the Pushpin is being
 * previewed by another widget.
 ****************************procedure*header*****************************
 */
static void
PreviewPushpin(ppw)
	PushpinWidget ppw;
{
	PreviewCache *		preview_cache;
	register Widget		destw = ppw->pushpin.preview_widget;
	Arg			args[10];
	int			a = 0;

	if (!XtIsRealized(destw))
		return;

/*  WORKAROUND - this error check will not work when previewing
	in a gadget.

	if (XtScreen(destw) != XtScreen((Widget) ppw)) {
		   OlVaDisplayWarningMsg(XtDisplay(ppw),
		              		OleNfilePushpin,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfilePushpin_msg2);
		return;
	}
*/

	preview_cache = (PreviewCache *) ppw->pushpin.preview_cache;

				/* Set the attributes of the OblongButton */
	a = 0;
	XtSetArg(args[a], XtNforeground, ppw->primitive.foreground);
	++a;
	XtSetArg(args[a], XtNbackground, ppw->core.background_pixel);
	++a;
	XtSetArg(args[a], XtNscale, ppw->pushpin.scale);
	++a;

		/* See if the hidden oblong is realized.  If it is not,
		 * realize it now.					*/

	if (XtIsRealized(preview_cache->shell) == False) {
		XtRealizeWidget(preview_cache->shell);
	}

	XtSetValues(preview_cache->oblong, args, a);

					/* Now Call the Button's
					 * Preview Routine		*/

	_OlButtonPreview((ButtonWidget) destw,
		(OblongButtonWidget) preview_cache->oblong);

} /* END OF PreviewPushpin() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ActivateWidget - this routine is used to activate the callbacks of
 * this widget. Currently, this routine is called by Menu.c with OL_SELECT
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	Boolean		consumed = False;
	PushpinPart	*pushpin = &(((PushpinWidget) w)->pushpin);

	if (type != OL_SELECTKEY)
		return (consumed);

	consumed = True;
	if (pushpin->pin_state == (OlDefine)OL_IN) {
		pushpin->pin_state = (OlDefine)OL_OUT;
		XtCallCallbacks(w, XtNpushpinOut, (XtPointer) NULL);
	} else {
		pushpin->pin_state = (OlDefine)OL_IN;
		XtCallCallbacks(w, XtNpushpinIn, (XtPointer) NULL);
	}
	return (consumed);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * ClassInitialize - Register OlDefine string values.
 ****************************procedure*header*****************************
 */
static void
ClassInitialize()
{
	_OlAddOlDefineType ("out", OL_OUT);
	_OlAddOlDefineType ("in",  OL_IN);
	
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - free the GCs stored in the pushpin widget.
 ****************************procedure*header*****************************
 */
static void
Destroy(w)
    Widget w;
{
	PushpinWidget ppw = (PushpinWidget) w;

					/* Destroy the Pushpin's GC	*/

	OlgDestroyAttrs (ppw->pushpin.pAttrs);

			/* remove this widget from the Preview list */

	PreviewManager(ppw, True);

						/* Remove All callbacks	*/

	XtRemoveAllCallbacks(w, XtNpushpinIn);
	XtRemoveAllCallbacks(w, XtNpushpinOut);

} /* END OF Destroy() */

/*
 *************************************************************************
 * Initialize - Initializes the instance record fields and resolves any
 * conflicts between the fields.  This routine registers a set of XImages
 * with the pushpin.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
	Widget request;
	Widget new;
	ArgList		args;
	Cardinal *	num_args;
{
	PushpinPart *	pushpin = &(((PushpinWidget) new)->pushpin);

						/* Create the normal GC	*/

	pushpin->pAttrs = (OlgAttrs *) NULL;
	GetNormalGC((PushpinWidget) new);

					/* Create the Preview cache.	*/

	pushpin->preview_cache = (XtPointer) NULL;
	PreviewManager((PushpinWidget) new, False);

			/* Initialize Widget data.  Set the window
			 * size to be big enough to fit the largest
			 * XImage for its point size.			*/

	GetNewDimensions((PushpinWidget) new,
			 new->core.width == (Dimension) 0,
			 new->core.height == (Dimension) 0);

	if (pushpin->pin_state != (OlDefine)OL_IN)
		pushpin->pin_state = (OlDefine)OL_OUT;

	pushpin->selected		= (Boolean)False;
	pushpin->preview_widget		= (Widget) NULL;

} /* END OF Initialize() */

/*
 *************************************************************************
 * Redisplay - this procedure redisplays the pushpin widget after
 * exposure events or when the state changes.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(w, xevent, region)
	Widget  w;		/* Pushpin widget id			*/
	XEvent *xevent;		/* unused */
	Region  region;		/* unused */
{
	PushpinWidget	 ppw = (PushpinWidget) w;
	unsigned	type;

	if (XtIsRealized(w) == False)
		return;

	type = GetPinType (ppw);

	OlgDrawPushPin (XtScreen (w), XtWindow (w), ppw->pushpin.pAttrs,
			0, 0, ppw->core.width, ppw->core.height, type);

} /* END OF Redisplay() */

/*
 *************************************************************************
 * SetValues - this routine manages changes to the pushpin fields caused
 * by an XtSetValues() call.  It returns a boolean specifying whether or
 * not a change requires a redisplay.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget current;		/* Current Pushpin widget		*/
	Widget request;		/* What the application wants		*/
	Widget new;		/* What the application gets, so far...	*/
	ArgList		args;
	Cardinal *	num_args;
{
	PushpinPart *cpp = &(((PushpinWidget) current)->pushpin);
	PushpinPart *npp = &(((PushpinWidget) new)->pushpin);
	Boolean      redisplay = False;

	if (npp->scale != cpp->scale) {
		GetNormalGC ((PushpinWidget) new);

			/* Always update the dimensions of the pushpin	*/

		GetNewDimensions((PushpinWidget) new, True, True);
		redisplay = True;
	}

	if (((PushpinWidget)new)->primitive.foreground !=
	    ((PushpinWidget)current)->primitive.foreground ||
	    new->core.background_pixel != current->core.background_pixel) {

		GetNormalGC((PushpinWidget) new);
		redisplay = True;
	}

	if (npp->is_default != cpp->is_default) {
		redisplay = True;
		_OlSetDefault(new, npp->is_default);
	}

	if (npp->preview_widget != (Widget) NULL) {
		PreviewPushpin((PushpinWidget) new);
		npp->preview_widget = (Widget) NULL;
	}

	if (npp->pin_state != cpp->pin_state) {
		if (npp->pin_state != OL_IN)	/* check validity	*/
			npp->pin_state = OL_OUT;
		if (npp->pin_state != cpp->pin_state)
			redisplay = True;
	}

	return (redisplay);

} /* END OF SetValues() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * Notify - fires the callbacks once the user decides final state of the
 * pushpin
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Notify(w, button)
	Widget    	w;		/* Pushpin widget id		*/
	OlVirtualName	button;		/* either OL_SELECT or OL_MENU	*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

	if (button == OL_MENU &&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu &&
	    pushpin->selected != True)
	{
		return;
	}

	pushpin->selected = False;

	ActivateWidget(w, OL_SELECTKEY, NULL);
} /* END OF Notify() */

/*
 *************************************************************************
 * SelectPin - this procedure is called when the user presses a valid
 * button on the pushpin or when a valid button is dragged over the 
 * the pushpin.  Valid buttons are the SELECT or MENU button.  But, the
 * MENU button is valid only if the pushpin is on a menu.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
SelectPin(w, button)
	Widget		w;		/* Pushpin widget id		*/
	OlVirtualName	button;		/* either OL_SELECT or OL_MENU	*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

				/* Give up the pointer grab		*/

	_OlUngrabPointer(w);

				/* Ignore selection of the pushpin by
				 * Menu button if the shell behavior
				 * is not "PressDragRelease" mode	*/
	
	if (button == OL_MENU &&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu)
		return;

	if (pushpin->selected == True)
		return;

	pushpin->selected = True;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
				(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}
} /* END OF SelectPin() */

/*
 *************************************************************************
 * SetDefault - called when user uses the mouse to set the pushpin
 * to be a default in the menu.
 ****************************procedure*header*****************************
 */
static void
SetDefault OLARGLIST((w))
	OLGRA( Widget,    w)	/* Pushpin widget id			*/
{
	PushpinWidget ppw = (PushpinWidget) w;

	if ((ppw->pushpin.shell_behavior != PressDragReleaseMenu &&
	     ppw->pushpin.shell_behavior != StayUpMenu) ||
	    ppw->pushpin.is_default == True)
		return;

	ppw->pushpin.is_default = True;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
			(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}

	_OlSetDefault(w, True);
} /* END OF SetDefault() */

/*
 *************************************************************************
 * UnselectPin - this procedure is called when the user leaves the
 * pushpin's window.  If the pushpin has been selected, its unselected.
 * No callbacks are made.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
UnselectPin(w)
	Widget    w;		/* Pushpin widget id			*/
{
	PushpinPart *pushpin = &(((PushpinWidget) w)->pushpin);

				/* Ignore selection of the pushpin by
				 * Menu button if the shell behavior
				 * is not "PressDragRelease" mode	*/
	
	if (pushpin->selected == (Boolean)False)
		return;

#ifdef oldcode /* this never happened */
	if ((*num_params != (Cardinal)0)	&&
	    pushpin->shell_behavior != StayUpMenu	&&
	    pushpin->shell_behavior != PressDragReleaseMenu)
		return;
#endif

	pushpin->selected = False;
	if (XtIsRealized(w) == (Boolean)True) {
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
			(unsigned int)0, (unsigned int)0, (Bool)False);
		Redisplay(w, (XEvent *)NULL, (Region) NULL);
		XFlush(XtDisplay(w));
	}
} /* END OF UnselectPin() */

static void
HandleButton(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	if (ve->xevent->type == ButtonPress)
	{
		_OlUngrabPointer(w);
	}

	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				SelectPin(w, ve->virtual_name);
			else
				Notify(w, ve->virtual_name);
			break;
		case OL_MENUDEFAULT:
			if (ve->xevent->type == ButtonPress)
			{
				ve->consumed = True;
				SetDefault(w);
			}
			break;
		default:
			if (ve->xevent->type == ButtonRelease)
			{
				ve->consumed = True;
				UnselectPin(w);
			}
			break;
	}
} /* end of HandleButton() */

static void
HandleCrossing(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				SelectPin(w, ve->virtual_name);
			else
				UnselectPin(w);
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			if (ve->xevent->type == EnterNotify)
				SetDefault(w);
			else
				UnselectPin(w);
			break;
		default:
			if (ve->xevent->type == LeaveNotify)
			{
				ve->consumed = True;
				UnselectPin(w);
			}
			break;
	}
} /* end of HandleCrossing() */

static void
HandleMotion(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:	/* FALLTHROUGH */
		case OL_MENU:
			ve->consumed = True;
			SelectPin(w, ve->virtual_name);
			break;
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault(w);
			break;
	}
} /* end of HandleMotion() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */
