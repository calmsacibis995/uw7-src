 /* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)button:RectButton.c	1.33"
#endif

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	rectangular button widget.
 *
 ************************************************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ButtonP.h>
#include <Xol/RectButtoP.h>
#include <Xol/ExclusiveP.h>
#include <Xol/NonexclusP.h>


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
static Boolean	PreviewState();
static void	RectButtonHandler();
static void	ToggleState();
static Boolean	SetState();
static Boolean	SetDefault();
static Boolean	RectEventChecksOut();
static void	RegisterConverters();
static int	GetShellBehavior();

					/* class procedures		*/

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static Boolean	SetValues();
					/* action procedures		*/

					/* public procedures		*/


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define EXCLUSIVES 0
#define NONEXCLUSIVES 1
#define NEITHER 1

static Boolean false = FALSE;

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */

#if 1
static char defaultTranslations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction()";
#else
static char defaultTranslations[] = "#augment\n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction()";
#endif

static OlEventHandlerRec
RBevents[] = {
	{ ButtonPress,	RectButtonHandler},
	{ ButtonRelease,RectButtonHandler},
	{ EnterNotify,	RectButtonHandler},
	{ LeaveNotify,	RectButtonHandler},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		XtOffset(ButtonWidget,button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_RECTBUTTON) },

	{XtNparentReset,
		XtCParentReset,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(RectButtonWidget, rect.parent_reset),
		XtRBoolean,
		(XtPointer) &false},
	};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

RectButtonClassRec rectButtonClassRec = {
  {
    (WidgetClass) &(buttonClassRec),	/* superclass		  */	
    "RectButton",			/* class_name		  */
    sizeof(RectButtonRec),		/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    NULL,				/* initialize		  */
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
    NULL,				/* destroy		  */
    XtInheritResize,			/* resize		  */
    XtInheritExpose,			/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    NULL,				/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {					/* primitive class	*/
      True,				/* focus_on_select	*/
      XtInheritHighlightHandler,	/* highlight_handler 	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      ActivateWidget,			/* activate		*/
      RBevents,				/* event_procs		*/
      XtNumber(RBevents),		/* num_event_procs	*/
      OlVersion,			/* version             */
      NULL,				/* extension           */
      { NULL, 0 },			/* dyn_data		*/
      XtInheritTransparentProc,		/* transparent_proc	*/
  },
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* RectButtonClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass rectButtonWidgetClass = (WidgetClass) &rectButtonClassRec;


/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 * RectEventChecksOut - This function checks button vs. shell behavior
 *
 *********************function*header************************
 */
static Boolean RectEventChecksOut(sb,vname)
int sb;
OlVirtualName vname;
{
	if(sb==StayUpMenu) return TRUE; 	/* select or menu button okay */

	if(sb==PressDragReleaseMenu) {		/* menu button only */
		if(vname==OL_MENU || vname==OL_MENUKEY) return TRUE;
			else return FALSE;
	}
						/* other shell behaviors */

	if(vname==OL_MENU || vname==OL_MENUKEY) 
		return FALSE;

	return TRUE; 

}	/*  RectEventChecksOut  */

/*
 ************************************************************
 *
 * GetShellBehavior - This function gets context of button
 *
 *********************function*header************************
 */

static int GetShellBehavior(w)
Widget w;
{
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent;

	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		return ew->exclusives.shell_behavior;
	}

	if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		return new->nonexclusives.shell_behavior;
	}

	return (int) OtherBehavior;

}	/*  GetShellBehavior  */

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
 * this widget.
 ****************************procedure*header*****************************
 */
static Boolean
#if OlNeedFunctionPrototypes
ActivateWidget(
	Widget		w,
	OlVirtualName	type,
	XtPointer	data)
#else /* OlNeedFunctionPrototypes */
ActivateWidget(w, type, data)
	Widget		w;
	OlVirtualName	type;
	XtPointer	data;
#endif /* OlNeedFunctionPrototypes */
{
	Boolean ret=FALSE;
	Boolean activate=TRUE;
	XEvent dummy_event;

	ret=PreviewState(w,&dummy_event,type,activate);
	if(ret==FALSE)
		return FALSE;
	ret=SetState(w,&dummy_event,type,activate);
		return ret;

} /* END OF ActivateWidget() */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *	to the current values, and sets them in the new
 *	widget.  It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************function*header************************
 */
static Boolean
SetValues (current, request, new, args, num_args)
	Widget current;
	Widget request;
	Widget new;
	ArgList	args;
	Cardinal * num_args;
{
	RectButtonWidget bw = (RectButtonWidget) current;
	RectButtonWidget newbw = (RectButtonWidget) new;
	Boolean needs_redisplay = FALSE;
	Widget parent = XtParent(new);

	/*
	 *  Has the is_default resource changed?
	 */
	if (bw->button.is_default != newbw->button.is_default)  {

		/*
		 *  Notify the parent of the button that the button
		 *  default has changed.
		 */

		/*
		 *  This must set the old button widget to the
		 *  new value too because this call to XtSetValues
		 *  cannot access the value in the newbw or reqbw
		 *  since they are allocated in a strange way (see
		 *  the code for _XtSetValues).
		 */

		/*
		 *  Exclusives depends on having ONLY True or False
		 *  values to set at this point.
		 */
		if (newbw->button.is_default != FALSE)
			newbw->button.is_default = TRUE;

		bw->button.is_default = newbw->button.is_default;

		{
		Arg arg;

		XtSetArg(arg, XtNresetDefault, bw->button.is_default);
		XtSetValues(parent, &arg, 1);
		}

		needs_redisplay = TRUE;
		}

	/*
	 *  Has the set resource changed?
	 */
	if (bw->button.set != newbw->button.set)  {

		/*
		 *  Notify the parent of the button that the button
		 *  default has changed.
		 */

		if (newbw->rect.parent_reset == FALSE)  {
			/*
			 *  This must set the old button widget to the
			 *  new value too because this call to XtSetValues
			 *  cannot access the value in the newbw or reqbw
			 *  since they are allocated in a strange way (see
			 *  the code for _XtSetValues).
			 */
			/*
			 *  Exclusives depends on having ONLY True or False
			 *  values to set at this point.
			 */
			if (newbw->button.set != FALSE)
				newbw->button.set = TRUE;
	
			bw->button.set = newbw->button.set;
	
			{
			Arg arg;
	
			XtSetArg(arg, XtNresetSet, bw->button.set);
			XtSetValues(parent, &arg, 1);
			}
			}
		needs_redisplay = TRUE;
		}

	/*
	 *  Always reset the parent_reset value to False.
	 */
	newbw->rect.parent_reset = FALSE;

	return (XtIsRealized((Widget)newbw) && needs_redisplay);

}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 ************************************************************
 *
 *  ToggleState - this function is called to change the state 
 *	of the button, were it to be selected.
 *
 *********************function*header************************
 */

static void ToggleState(w)
Widget w;
{
	RectButtonWidget bw = (RectButtonWidget) w;
	Arg arg[2];

	if(w==(Widget)0) return;

	if(bw->button.set!=FALSE ) {
		XtSetArg(arg[0], XtNparentReset, TRUE);
		XtSetArg(arg[1], XtNset, FALSE);
		XtSetValues(w, arg, 2);
		return;
	}
				/* here if button originally FALSE */

	XtSetArg(arg[0], XtNparentReset, TRUE);
	XtSetArg(arg[1], XtNset, TRUE);
	XtSetValues(w, arg, 2);

	return;

} /* ToggleState */

/*
 ************************************************************
 *
 *  PreviewState - this function is called on a select or menu button 
 *	down event.  It previews the appearance of the button if it
 *	were to be selected, including unsetting the appearance of
 *	any other set button.
 *
 *********************function*header************************
 */

static Boolean PreviewState(w,event,vname,activate)
Widget w;
XEvent *event;
OlVirtualName vname;
Boolean activate;
{
	RectButtonWidget bw = (RectButtonWidget) w;
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent,looks_set,set_child;
	Boolean noneset;
	int parent_type=NEITHER,sb;
	XCrossingEvent *xce;

	if(!XtIsSensitive(w)) 
		return TRUE;

	if(bw->button.busy)
		return TRUE;

					/* filter out pointer crossings */
	if(!activate) {

	if(event->type==EnterNotify || event->type==LeaveNotify) {
		xce = (XCrossingEvent *) &(event->xcrossing);
		if(xce->mode!=NotifyNormal) {
			return FALSE; 
		}
		
	}
	}

	if(vname!=OL_UNKNOWN_INPUT) {	/* value with Enter/LeaveNotify */
		sb= GetShellBehavior(w);
		if(!RectEventChecksOut(sb,vname)) { 
			return FALSE;
		}
	}


	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		parent_type=EXCLUSIVES;
		looks_set= ew->exclusives.looks_set;
		set_child= ew->exclusives.set_child;
		noneset= ew->exclusives.noneset;
	}
	else if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		parent_type=NONEXCLUSIVES;
	}

	if(parent_type==NONEXCLUSIVES || parent_type==NEITHER) {
		ToggleState(w);
		return TRUE;
	}

					/* if here, EXCLUSIVES is parent */
					
					/* if !nonset two buttons to do */

	if(w==set_child && w==looks_set && noneset) {

		ToggleState(w);
		ew->exclusives.looks_set=(Widget)0;
		
		return TRUE;
	}

	if(w==set_child && w!=looks_set && noneset) return TRUE;

	if(w==set_child && w==looks_set && !noneset) return TRUE;

	if(w==set_child && w!=looks_set && !noneset) {

		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}

	if(w!=set_child && w==looks_set && noneset)  return TRUE;

	if(w!=set_child && w!=looks_set && noneset) {

		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}

	if(w!=set_child && w==looks_set && !noneset) return TRUE;

	if(w!=set_child && w!=looks_set && !noneset) {
		ToggleState(looks_set);
		ToggleState(w);
		ew->exclusives.looks_set=w;
		return TRUE;
	}
	
	return TRUE;	/* should not get here */

}	/* PreviewState */

/*
 ************************************************************
 *
 *  RectButtonHandler - this function is called by OPEN LOOK
 *	for requested events.
 *
 *********************function*header************************
 */

static void
RectButtonHandler(w,ve)
	Widget		w;
	OlVirtualEvent	ve;
{

	Boolean activate=FALSE;

	switch(ve->xevent->type) {

		case EnterNotify:
			if (ve->virtual_name == OL_MENUDEFAULT)  {
				ve->consumed = SetDefault(w,OL_MENUDEFAULT);
				break;
			}
			/*  FALLTHROUGH  */
		case LeaveNotify:
			if(ve->virtual_name==OL_SELECT
				|| ve->virtual_name==OL_MENU) {
			ve->consumed =
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
			}
			break;

		case ButtonPress:

		(void) _OlUngrabPointer(w);

		if(ve->virtual_name==OL_MENUDEFAULT)
			ve->consumed = SetDefault(w,OL_MENUDEFAULT);

		else if(ve->virtual_name==OL_SELECT
			|| ve->virtual_name==OL_MENU) {
			ve->consumed =
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
		}
			break;

		case ButtonRelease:

			ve->consumed =
			SetState(w,ve->xevent,ve->virtual_name,activate);

			break;
	}
}

/*
 ************************************************************
 *
 *  SetState - this function is called on a select or menu button up
 *	event.  It invokes the users select and unselect callback(s) 
 *	and changes the state of the rectangular button from 
 *	set to unset or unset to set, as well as unsetting any
 *	previously set button.
 *
 *********************function*header************************
 */
static Boolean SetState(w,event,vname,activate)
Widget w;
XEvent *event;
OlVirtualName vname;
Boolean activate;
{
	RectButtonWidget	bw = (RectButtonWidget) w;
	ExclusivesWidget	ew;
	Widget			parent = XtParent(w);
	Widget			set_child;
	int			parent_type=NEITHER;
	int			sb;
	Boolean			ret;

	if(!XtIsSensitive(w)) return TRUE;

	if(!activate) {

	if(!(RectEventChecksOut(GetShellBehavior(w),vname)))
		return FALSE;
	}

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		parent_type=EXCLUSIVES;
		set_child=ew->exclusives.set_child;
	}
	else if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		parent_type=NONEXCLUSIVES;
	}

		/* only case with no change of state or actions */

	if(parent_type==EXCLUSIVES	&& w==set_child 
					&& ew->exclusives.noneset==FALSE) {

		return TRUE;
	}
		/* all other cases state will change state and have actions */

	if(!bw->button.set) {
		if(bw->button.unselect!=(XtCallbackList)NULL)
			XtCallCallbacks(w,XtNunselect,(XtPointer)NULL);
		if(parent_type==EXCLUSIVES) ew->exclusives.set_child=(Widget)0;
		return TRUE;
	}

	if(bw->button.set) {
		if(bw->button.select!=(XtCallbackList)NULL)
			XtCallCallbacks(w,XtNselect,(XtPointer)NULL);
		if(bw->button.post_select!=(XtCallbackList)NULL)
			XtCallCallbacks(XtParent(w),XtNpostSelect,(XtPointer)NULL);
		if(parent_type==EXCLUSIVES) { 
			if(set_child!=(Widget)0) {

			if(bw->button.unselect!=(XtCallbackList)NULL)
				XtCallCallbacks(set_child,XtNunselect,(XtPointer)NULL);
			}
			ew->exclusives.set_child=w;
		}
		return TRUE;
	}
} /* END OF SetState() */

/*
 ************************************************************
 *
 *  SetDefault - this function is called on a button up
 *	event.  It invokes the users callback and changes
 *	the state of the  button back to default.
 *
 *********************function*header************************
 */
static Boolean SetDefault(w,vname)
Widget w;
OlVirtualName vname;
{
	RectButtonWidget bw = (RectButtonWidget) w;
	ShellBehavior sb;
	ExclusivesWidget ew;
	NonexclusivesWidget new;
	Widget parent;
	Arg arg;

	if(!XtIsSensitive(w)) return TRUE;

	parent = XtParent(w);

	if(XtIsSubclass(parent, exclusivesWidgetClass)) {
		ew = (ExclusivesWidget) parent;
		sb = ew->exclusives.shell_behavior;
	}

	else if(XtIsSubclass(parent, nonexclusivesWidgetClass)) {
		new = (NonexclusivesWidget) parent;
		sb = new->nonexclusives.shell_behavior;
	}

        if (sb != PressDragReleaseMenu &&
            sb != StayUpMenu &&
            sb != PinnedMenu &&
            sb != UnpinnedMenu)
                return FALSE;


        if (bw->button.is_default == FALSE) {
		XtSetArg(arg, XtNdefault, TRUE);
                XtSetValues(w, &arg, 1);

                _OlSetDefault(w, True);
		return TRUE;
	}
}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */
