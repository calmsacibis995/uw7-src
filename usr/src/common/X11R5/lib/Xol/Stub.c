/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)stub:Stub.c	1.34"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		OPEN LOOK(TM) stub widget source code.  This widget
 * is a method-driven widget, i.e., an application programmer can
 * set the class procedures via the XtSetValues() procedure.  This
 * widget is extremely useful to prototype other widgets.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xt/EventI.h> /* Xt (sic) so that we don't have to install it in
			* X11.  In the future, this dependency can be
			* removed if the exposure handling were done as
			* an event handler rather than as a class
			* procedure.  If an event handler is used, an
			* additional resource must be added to the stub to
			* indicate what type of exposures are looked at and
			* what type of compression should be used.  When
			* this is done, we can take out the code that mucks
			* with the event masks for the stub's window.	*/
#include <Xol/OpenLookP.h>
#include <Xol/StubP.h>

#define ClassName Stub
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

static void	_Realize();		/* default realize procedure	*/
static void	_Initialize();		/* default initialize proc.	*/
static void	GetWindowAttributes();	/* gets X Window attributes	*/
static long	SetEventMask();	/* modifies the event Mask of
					 * the widget			*/

					/* class procedures		*/

static void	Destroy();		/* destroys an instance		*/
static void	GetValuesHook();	/* getting sub data attributes	*/
static void	Initialize();		/* initializes an instance	*/
static void	InitializeHook();	/* initializes sub data		*/
static XtGeometryResult QueryGeometry(); /* Preferred geometry		*/
static void	Realize();		/* realizes an instance		*/
static void	Redisplay();		/* instance exposure handling	*/
static void	Resize();		/* resizes an instance		*/
static Boolean SetValues();		/* setting instance attributes	*/
static void	SetValuesAlmost();	/* conflict resolution for
					 * setting instance attributes	*/
static Boolean SetValuesHook();		/* setting sub data attributes	*/
static Widget  TraversalHandler();	/* handles traversal	        */

					/* Activate procedure */
static Boolean Activate OL_ARGS((Widget, OlVirtualName, XtPointer));
static Boolean AcceptFocus OL_ARGS((Widget, Time *)); /*AcceptFocus procedure */
static Widget RegisterFocus();		/* RegisterFocus procedure */
static void HighlightHandler();		/* Highlight Handler */

					/* action procedures		*/
/* There are no action procedures */

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

#define OFFSET(field) XtOffset(StubWidget, field)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no default translations or actions for this widget */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {
				/* Turn traversal off by default since
				 * there's no natural user feedback	*/
   { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
	OFFSET(primitive.traversal_on), XtRImmediate, (XtPointer)False },

   { XtNwindow, XtCWindow, XtRWindow, sizeof(Window),
	OFFSET(stub.window), XtRWindow, (XtPointer) NULL },

   { XtNreferenceStub, XtCReferenceStub, XtRWidget, sizeof(Widget),
	OFFSET(stub.reference_stub), XtRWidget, (XtPointer) NULL },

   { XtNdestroy, XtCDestroy, XtRFunction, sizeof(XtWidgetProc),
	OFFSET(stub.destroy), XtRFunction, (XtPointer) NULL },

   { XtNexpose, XtCExpose, XtRFunction, sizeof(XtExposeProc),
	OFFSET(stub.expose), XtRFunction, (XtPointer) NULL },

   { XtNgetValuesHook, XtCGetValuesHook, XtRFunction, sizeof(XtArgsProc),
	OFFSET(stub.get_values_hook), XtRFunction, (XtPointer) NULL },

   { XtNinitialize, XtCInitialize, XtRFunction, sizeof(XtInitProc),
	OFFSET(stub.initialize), XtRImmediate, (XtPointer) _Initialize },

   { XtNinitializeHook, XtCInitializeHook, XtRFunction, sizeof(XtArgsProc),
	OFFSET(stub.initialize_hook), XtRFunction, (XtPointer) NULL },

   { XtNrealize, XtCRealize, XtRFunction, sizeof(XtRealizeProc),
	OFFSET(stub.realize), XtRImmediate, (XtPointer) _Realize },

   { XtNresize, XtCResize, XtRFunction, sizeof(XtWidgetProc),
	OFFSET(stub.resize), XtRFunction, (XtPointer) NULL },

   { XtNsetValues, XtCSetValues, XtRFunction, sizeof(XtSetValuesFunc),
	OFFSET(stub.set_values), XtRFunction, (XtPointer) NULL },

   { XtNsetValuesAlmost, XtCSetValuesAlmost, XtRFunction,
	sizeof(XtAlmostProc), OFFSET(stub.set_values_almost),
	XtRFunction, (XtPointer) NULL },

   { XtNsetValuesHook, XtCSetValuesHook, XtRFunction, sizeof(XtArgsFunc),
	OFFSET(stub.set_values_hook), XtRFunction, (XtPointer) NULL },

   { XtNqueryGeometry, XtCQueryGeometry, XtRFunction,
	sizeof(XtGeometryHandler), OFFSET(stub.query_geometry),
	XtRFunction, (XtPointer) NULL },

   { XtNactivateFunc, XtCActivateFunc, XtRFunction, sizeof(OlActivateFunc),
	OFFSET(stub.activate), XtRFunction, (XtPointer) NULL },

   { XtNacceptFocusFunc, XtCAcceptFocusFunc, XtRFunction, sizeof(XtAcceptFocusProc),
	OFFSET(stub.accept_focus), XtRFunction, (XtPointer) NULL },

   { XtNhighlightHandlerProc, XtCHighlightHandlerProc, XtRFunction,
	sizeof(OlHighlightProc), OFFSET(stub.highlight), XtRFunction,
	(XtPointer) NULL },

   { XtNregisterFocusFunc, XtCRegisterFocusFunc, XtRFunction,
	sizeof(OlRegisterFocusFunc), OFFSET(stub.register_focus), XtRFunction,
	(XtPointer) NULL },

   { XtNtraversalHandlerFunc, XtCTraversalHandlerFunc, XtRFunction,
	sizeof(OlTraversalFunc), OFFSET(stub.traversal_handler),
	XtRFunction, (XtPointer) NULL },

};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

StubClassRec
stubClassRec = {
  {
    (WidgetClass) &primitiveClassRec,	/* superclass		  */	
    "Stub",				/* class_name		  */
    sizeof(StubRec),			/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    InitializeHook,			/* initialize_hook	  */
    Realize,				/* realize		  */
    NULL,				/* actions		  */
    NULL,				/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    TRUE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    TRUE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    Resize,				/* resize		  */
    Redisplay,				/* expose		  */
    SetValues,				/* set_values		  */
    SetValuesHook,			/* set_values_hook	  */
    SetValuesAlmost,			/* set_values_almost	  */
    GetValuesHook,			/* get_values_hook	  */
    AcceptFocus,			/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    QueryGeometry,			/* query_geometry	  */
    NULL,				/* display_accelerator	  */
    NULL				/* extension		  */
  },	/* CoreClass fields initialization */
  {					/* primitive class     */
      False,				/* focus_on_select     */
      (OlHighlightProc)HighlightHandler,/* highlight_handler   */
      (OlTraversalFunc)TraversalHandler,/* traversal_handler   */
      (OlRegisterFocusFunc)RegisterFocus,/* register_focus      */
      Activate,				/* activate            */
      NULL,				/* event_procs	       */
      0,				/* num_event_procs     */
      OlVersion,			/* version             */
      NULL				/* extension           */
  },
  {
    NULL				/* field not used    	  */
  }  /* StubClass field initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass stubWidgetClass = (WidgetClass) &stubClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _Realize - realizes a widget.  This involves creating the window for
 * the widget.  If a window is supplied, it is used as a window.  If
 * no window is supplied, one is created.
 ****************************procedure*header*****************************
 */
static void
_Realize(w, value_mask, attributes)
	Widget			w;
	XtValueMask *		value_mask;
	XSetWindowAttributes *	attributes;
{
	StubWidget	sw = (StubWidget) w;
	Window		window;

				/* If the application has not given us
				 * a window, create one			*/

	window		= sw->stub.window;
	sw->core.window = sw->stub.window;

	if (window == (Window) NULL) {
		XtCreateWindow(w, (unsigned int)InputOutput,
			(Visual *)CopyFromParent, *value_mask, attributes);
		return;
	}

				/* If the application has given us a
				 * window, make sure that no other
				 * widget owns this window		*/

	
	if (XtWindowToWidget(XtDisplay(w), window) != (Widget) NULL)
	  {
	    OlVaDisplayErrorMsg(XtDisplay(w),
				OleNfileStub,
				OleTmsg1,
				OleCOlToolkitError,
				OleMfileStub_msg1,
				XtWindowToWidget(XtDisplay(w),
						 window)->core.name, 
				window, window);
	  }
    
	if (*value_mask & CWEventMask) {
		XSelectInput(XtDisplay(w), window, attributes->event_mask);
	}

} /* END OF _Realize() */

/*
 *************************************************************************
 * _Initialize - this is the default initialization procedure
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
_Initialize(request, new)
	Widget request;
	Widget new;
{
	GetWindowAttributes(new);
} /* END OF _Initialize() */

/*
 *************************************************************************
 * GetWindowAttributes -
 ****************************procedure*header*****************************
 */
static void
GetWindowAttributes(w)
	Widget w;
{
	XWindowAttributes xwa;
	Window		  window = ((StubWidget) w)->stub.window;

	if (window == (Window) NULL)
		return;

	XGetWindowAttributes(XtDisplay(w), window, &xwa);

	w->core.x		= (Position) xwa.x;
	w->core.y		= (Position) xwa.y;
	w->core.width		= (Dimension) xwa.width;
	w->core.height		= (Dimension) xwa.height;
	w->core.border_width	= (Dimension) xwa.border_width;

} /* END OF GetWindowAttributes() */

/*
 *************************************************************************
 * SetEventMask - this routine checks to see if the Stub Widget actually
 * needs to be checking for exposure events.  This explicit check is
 * necessary since the class part has an Expose procedure, even though
 * the widget instance part may not have an expose procedure.
 *
 * Note: this routine is only needed since the 'expose' class field is
 * non-NULL.  If the future the expose class field should be NULL and
 * exposure handling should be done with an event handler.  When this is
 * done, this procedure will not be needed.
 ****************************procedure*header*****************************
 */
static long
SetEventMask(w, have_exposures)
	Widget	w;			/* The Stub Widget		*/
	Boolean	have_exposures;		/* True if the Stub Widget has
					 * exposures selected		*/
{
	StubWidget	sw = (StubWidget) w;
	XtEventTable	ev;		/* this widget's internal table	*/
	EventMask	mask = (EventMask)0;	/* starting mask	*/

	mask = XtBuildEventMask(w);
	if (sw->stub.expose)
		mask |= ExposureMask;
	else
		mask &= ~ExposureMask;

#ifdef DELETE
	for (ev = w->core.event_table; ev != NULL; ev = ev->next)
		if (ev->select == True)
			mask |= ev->mask;

	if (XtClass(w)->core_class.visible_interest == True)
		mask |= VisibilityChangeMask;

	if (sw->stub.expose != (XtExposeProc) NULL)
		mask |= ExposureMask;
#endif

	if (mask & ExposureMask) {
		if (have_exposures == False && XtIsRealized(w) == True)
			XSelectInput(XtDisplay(w), XtWindow(w), (long)mask);
	}
	else if (XtIsRealized(w) == True) {
		XSelectInput(XtDisplay(w), XtWindow(w), (long)mask);
	}

	return((long)mask);

} /* END OF SetEventMask() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * Destroy - 
 ****************************procedure*header*****************************
 */
static void
Destroy(w)
    Widget w;
{
	XtWidgetProc destroy = ((StubWidget) w)->stub.destroy;

	if (destroy != (XtWidgetProc) NULL)
		(*destroy)(w);
} /* END OF Destroy() */

/*
 *************************************************************************
 * GetValuesHook - 
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(w, args, num_args)
	Widget	   w;
	ArgList	   args;
	Cardinal * num_args;
{
	XtArgsProc gvh = ((StubWidget) w)->stub.get_values_hook;

	if (gvh != (XtArgsProc) NULL)
		(*gvh)(w, args, num_args);
} /* END OF GetValuesHook() */

/*
 *************************************************************************
 * Initialize - Initializes the instance record fields and resolves any
 * conflicts between the fields.
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
	StubWidget	nw = (StubWidget) new;
	XtInitProc	initialize = nw->stub.initialize;
	Widget		ref = nw->stub.reference_stub;
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

					/* Take care of inheritance	*/

	if (ref != (Widget) NULL &&
	    XtIsSubclass(ref, stubWidgetClass) == (Boolean)True)
	{
		StubWidget rs = (StubWidget) ref;

		nw->stub.destroy		= rs->stub.destroy;
		nw->stub.expose			= rs->stub.expose;
		nw->stub.get_values_hook	= rs->stub.get_values_hook;
		nw->stub.initialize		= rs->stub.initialize;
		nw->stub.initialize_hook	= rs->stub.initialize_hook;
		nw->stub.realize		= rs->stub.realize;
		nw->stub.resize			= rs->stub.resize;
		nw->stub.set_values		= rs->stub.set_values;
		nw->stub.set_values_almost	= rs->stub.set_values_almost;
		nw->stub.set_values_hook	= rs->stub.set_values_hook;
		nw->stub.query_geometry		= rs->stub.query_geometry;
	}
					/* Now call the instance
					 * initialize procedure		*/

	if (initialize != (XtInitProc) NULL)
		(*initialize)(request, new, args, num_args);

} /* END OF Initialize() */

/*
 *************************************************************************
 * InitializeHook - 
 ****************************procedure*header*****************************
 */
static void
InitializeHook(w, args, num_args)
	Widget	   w;
	ArgList	   args;
	Cardinal * num_args;
{
	XtArgsProc initialize_hook = ((StubWidget) w)->stub.initialize_hook;

	if (initialize_hook != (XtArgsProc) NULL)
		(*initialize_hook)(w, args, num_args);
} /* END OF InitializeHook() */

/*
 *************************************************************************
 * QueryGeometry
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeometry(w, intended, perferred_return)
	Widget		   w;
	XtWidgetGeometry * intended;
	XtWidgetGeometry * perferred_return;
{
	XtGeometryResult (*qg)() = ((StubWidget) w)->stub.query_geometry;
	XtGeometryResult result = XtGeometryYes;

	if (qg != (XtGeometryHandler) NULL)
		result = (*qg)(w, intended, perferred_return);
	return(result);
} /* END OF QueryGeometry() */

/*
 *************************************************************************
 * Realize - realizes an instance
 ****************************procedure*header*****************************
 */
static void
Realize(w, value_mask, attributes)
	Widget			w;
	XtValueMask *		value_mask;
	XSetWindowAttributes *	attributes;
{
	XtRealizeProc realize = ((StubWidget) w)->stub.realize;

			/* Because we're using the 'expose' class procedure
			 * to handle exposures, we must check to see if the
			 * an Exposure event mask is actually needed.  When
			 * an event handler is used for exposure handling,
			 * this check is no longer needed.
			 */
	attributes->event_mask = SetEventMask(w, False);

	if (realize != (XtRealizeProc) NULL)
	{
		(*realize) (w, value_mask, attributes);
	}
	else
	{
	  OlVaDisplayErrorMsg(XtDisplay(w),
			      OleNfileStub,
			      OleTmsg2,
			      OleCOlToolkitError,
			      OleMfileStub_msg2,
			      w->core.name);
	}
} /* END OF Realize() */

/*
 *************************************************************************
 * Redisplay - this routine handles exposures on the instance. In here
 * we check to see if we need to track exposure events.  We have to do
 * this often, since the addition of an event handler will cause the
 * Intrinsics to select on ExposureMask since every Stub widget has a
 * class expose procedure.  By checking to see if we need the ExposureMask
 * in here, we can guarantee that we only get one exposure before
 * we turn it off.
 *
 * Note: using the 'expose' class field (i.e., specifying this procedure as
 * the class' expose procedure) is sub-optimal since it causes the
 * Intrinsics to believe that evetn stub widget is interested in exposures.
 * This is not always true since some stub widgets may not have the XtNexpose
 * resource set and may not have added any exposure event handlers of their
 * own.  In this case, we must do as explained above (i.e., check to see if
 * we're really interested in exposures and turn the interest off if we're
 * not).
 * The correct way to deal with exposures is to add an event handler if the
 * XtNexpose resource is set.  The event handler would trap exposures and
 * call the application's procedures (specified with XtNexpose) after doing
 * appropriate exposure compression.  An additional resource should be
 * added (XtNcompression) to indicate the type of compression desired.
 * Also, a new class field should be added to the stub widget class.  This
 * new class field would be the exposure compression routine.
 ****************************procedure*header*****************************
 */
static void
Redisplay(w, xevent, region)
	Widget  w;		/* Stub widget id			*/
	XEvent *xevent;		/* unused */
	Region  region;		/* unused */
{
	XtExposeProc expose = ((StubWidget) w)->stub.expose;

	if (expose != (XtExposeProc) NULL)
		(*expose)(w, xevent, region);
	else
		(void)SetEventMask(w, True);
} /* END OF Redisplay() */

/*
 *************************************************************************
 * Resize - 
 ****************************procedure*header*****************************
 */
static void
Resize(w)
	Widget w;
{
	XtWidgetProc resize = ((StubWidget) w)->stub.resize;

	if (resize != (XtWidgetProc) NULL)
		(*resize)(w);
} /* END OF Resize() */

/*
 *************************************************************************
 * SetValues - this updates the attributes of the Stub Widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget current;		/* Current Stub widget		*/
	Widget request;		/* What the application wants		*/
	Widget new;		/* What the application gets, so far...	*/
	ArgList		args;
	Cardinal *	num_args;
{
	XtSetValuesFunc set_values = ((StubWidget)new)->stub.set_values;

	if (set_values != (XtSetValuesFunc) NULL)
		return((*set_values)(current, request, new, args, num_args));
	else
		return(False);
} /* END OF SetValues() */

/*
 *************************************************************************
 * SetValuesAlmost - set sub-widget data.
 ****************************procedure*header*****************************
 */
static void
SetValuesAlmost(w, new_widget, request, reply)
	Widget		   w;		/* geom. requested on this one	*/
	Widget		   new_widget;	/* geometry so far		*/
	XtWidgetGeometry * request;	/* original request		*/
	XtWidgetGeometry * reply;	/* reply to the request		*/
{
	XtAlmostProc sva = ((StubWidget) w)->stub.set_values_almost;

	if (sva != (XtAlmostProc) NULL)
		(*sva)(w, new_widget, request, reply);
} /* END OF SetValuesAlmost() */

/*
 *************************************************************************
 * SetValuesHook - set sub-widget data.  In here we check to see if
 * the Stub Widget really needs to select on Expose events.  The expose
 * mask will only be selected if the widget has an expose procedure for
 * it's instance part.  We don't update the select mask if there is no
 * expose procedure since this happens in only two ways:
 *	1. if the widget never had an expose procedure, or
 *	2. if the expose procedure was set to NULL.
 * If (1) is the case, the Redisplay() function will be called on the
 * first expose and will unselect the ExposureMask when the first
 * exposure is generated on the widget's window.  If (2) is the case,
 * the same argument applies.
 ****************************procedure*header*****************************
 */
static Boolean
SetValuesHook(w, args, num_args)
	Widget	   w;
	ArgList	   args;
	Cardinal * num_args;
{
	Boolean		redisplay = False;
	XtArgsFunc	svh = ((StubWidget) w)->stub.set_values_hook;

	if (svh != (XtArgsFunc) NULL)
	{
		redisplay = (*svh)(w, args, num_args);
	}

	if (((StubWidget) w)->stub.expose != (XtExposeProc) NULL &&
	    XtIsRealized(w) == True)
	{
		(void) SetEventMask(w, False);
	}
	return(redisplay);
} /* END OF SetValuesHook() */

static Boolean
Activate OLARGLIST((w, activation_type, data))
	OLARG( Widget, 		w)
	OLARG( OlVirtualName,	activation_type)
	OLGRA( XtPointer,	data)
{
	OlActivateFunc act = ((StubWidget) w)->stub.activate;
	
	if (!XtIsSensitive(w))
		return(TRUE);
	else {
		if (act == NULL)
			act = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.activate;
		
		if (act != NULL)
			return((*act)(w, activation_type, data));
		else
			return(FALSE);
	}
}

static Boolean
AcceptFocus OLARGLIST((w, timestamp))
	OLARG( Widget,	w)
	OLGRA( Time *,	timestamp)
{
	XtAcceptFocusProc accept_focus = ((StubWidget) w)->stub.accept_focus;
	
	if (accept_focus == NULL)
		accept_focus = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				core_class.accept_focus;

	if (accept_focus)
		return((*accept_focus)(w, timestamp));
	else
		return(FALSE);
}

static void
HighlightHandler(w, highlight_type)
Widget w;
OlDefine highlight_type;
{
	OlHighlightProc highlight = ((StubWidget) w)->stub.highlight;
	
	if (highlight == NULL)
		highlight = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.highlight_handler;

	if (highlight)
		(*highlight)(w, highlight_type);
}

static Widget
RegisterFocus(w)
Widget w;
{
	OlRegisterFocusFunc reg_focus = ((StubWidget) w)->stub.register_focus;

	if (reg_focus == NULL)
		reg_focus = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.register_focus;

	if (reg_focus)
		return((*reg_focus)(w));
	else
		return(NULL);
}

static Widget
TraversalHandler(shell, w, direction, time)
Widget shell;
Widget w;
OlDefine direction;
Time time;
{
	OlTraversalFunc traversal_func =((StubWidget)w)->stub.traversal_handler;

	if (traversal_func == NULL)
		traversal_func = ((PrimitiveWidgetClass)primitiveWidgetClass)->
				primitive_class.traversal_handler;

	if (traversal_func)
		return((*traversal_func)(shell, w, direction, time));
	else
		return(NULL);
}

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */

