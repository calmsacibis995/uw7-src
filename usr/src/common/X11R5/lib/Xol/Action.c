/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)mouseless:Action.c	1.61"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the generic translations, generic action
 *	routines and convenience routines to manage the event handling
 *	within the toolkit.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/VendorI.h>
#include <Xol/ManagerP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/EventObjP.h>				/* For gadgets	*/
#include <Xol/Flat.h>
#include <Xol/PopupMenu.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Action  Procedures 
 *		3. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static OlEventHandlerProc
		GetEventHandler OL_ARGS((WidgetClass, WidgetClass, int));
static Boolean	assoc_check_cycle OL_ARGS((Widget, Widget));

					/* action procedures		*/

static void	HandleFocusChange OL_ARGS((Widget, OlVirtualEvent));
static void	HandleKeyPress OL_ARGS((Widget, OlVirtualEvent));
static void	ShellGotFocus OL_ARGS((Widget));

					/* public procedures		*/

void		OlAction OL_ARGS((Widget, XEvent *, String *, Cardinal *));
Boolean		OlActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
Boolean		OlAssociateWidget OL_ARGS((Widget, Widget, Boolean));
void		OlUnassociateWidget OL_ARGS((Widget));
Boolean		OlSetAppEventProc OL_ARGS((Widget, OlDefine,
				 OlEventHandlerList, Cardinal));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#ifdef DONT_RM_HACK
			/* HACK to get the flattened widgets to know
			 * which sub-object is to receive focus when a
			 * mnemonic is pressed.
			 */
XtPointer	Ol_mnemonic_data = (XtPointer)NULL;
#endif

#define VENDOR		0
#define PRIMITIVE	1
#define MANAGER		2
#define	GADGET		3
#define	OTHER		4

#define ASSOC_LIST_STEP	64

#define	NULL_WIDGET	((Widget)NULL)
#define NULL_DATA	((XtPointer)NULL)
#define PCLASS(wc,field) (((PrimitiveWidgetClass)wc)->primitive_class.field)
#define MCLASS(wc,field) (((ManagerWidgetClass)wc)->manager_class.field)
#define GCLASS(wc,field) (((EventObjClass)wc)->event_class.field)

	/* Define Generic translation table and Generic Action Table	*/

OLconst char
_OlGenericTranslationTable[] = "\
	<FocusIn>:	OlAction()	\n\
	<FocusOut>:	OlAction()	\n\
	<Key>:		OlAction()	\n\
	<BtnDown>:	OlAction()	\n\
	<BtnUp>:	OlAction()	\n\
";

	/* Define Generic Event Handler List	*/

OLconst OlEventHandlerRec
_OlGenericEventHandlerList[] = {
	{ FocusIn,	HandleFocusChange	},
	{ FocusOut,	HandleFocusChange	},
	{ KeyPress,	HandleKeyPress		},
	/* don't handle button presses or releases even though
	 * we've selected for them.
	 */
};
OLconst Cardinal _OlGenericEventHandlerListSize =
			XtNumber(_OlGenericEventHandlerList);

static Cardinal		AssocWidgetListSize = 0;
static Cardinal		AssocWidgetListAllocSize = 0;
static WidgetList	LeaderList = NULL;
static WidgetList	FollowerList = NULL;
static Boolean		assoc_list_modified = FALSE;

static OlEventHandlerList	pre_consume_list = NULL;
static OlEventHandlerList	post_consume_list = NULL;
static int			pre_consume_list_size = 0;
static int			post_consume_list_size = 0;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetEventHandler - this routine returns the event handler procedure
 * registered for a particular event type.
 ****************************procedure*header*****************************
 */
static OlEventHandlerProc
GetEventHandler(wc, wc_special, xevent_type)
	WidgetClass	wc;
	WidgetClass	wc_special;
	int		xevent_type;
{
	OlEventHandlerList	elist;
	Cardinal		num;

	if (wc_special == primitiveWidgetClass) {
		elist	= PCLASS(wc, event_procs);
		num	= PCLASS(wc, num_event_procs);
	} else if (wc_special == managerWidgetClass) {
		elist	= MCLASS(wc, event_procs);
		num	= MCLASS(wc, num_event_procs);
	} else if (wc_special == eventObjClass) {
		elist	= GCLASS(wc, event_procs);
		num	= GCLASS(wc, num_event_procs);
	} else {
		OlVendorClassExtension	ext = _OlGetVendorClassExtension(wc);

		if (ext) {
			elist	= ext->event_procs;
			num	= ext->num_event_procs;
		} else {
			elist	= (OlEventHandlerList)NULL;
			num	= 0;
		}
	}

	while(num != 0 && elist->type != xevent_type) {
		++elist;
		--num;
	}
	return( ((num != 0) ? elist->handler : NULL) );
} /* END OF GetEventHandler() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

static void
ShellGotFocus OLARGLIST((w))
	OLGRA( Widget,	w)
{
	OlVendorClassExtension	ext;
	OlFocusData *		fd = _OlGetFocusData(w, NULL);
	Time			time = CurrentTime;
	Widget			the_default;

	if (fd->initial_focus_widget != NULL_WIDGET &&
	    fd->initial_focus_widget != w &&
	    XtCallAcceptFocus(fd->initial_focus_widget, &time) == True)
	{
		return;
	}

	if ((the_default = _OlGetDefault(w)) != NULL_WIDGET &&
	    the_default != w &&
	    XtCallAcceptFocus(the_default, &time) == True)
	{
		return;
	}

	if ( (ext = _OlGetVendorClassExtension(XtClass(w))) != NULL &&
	     ext->traversal_handler )
	{
			/* NULL is a special flag, it tells
			 * TraversalHandler() that if no one
			 * can have focus, don't set focus
			 * to "shell" because this "shell"
			 * already has "focus"...
			 */
		(*ext->traversal_handler)(w, NULL, OL_IMMEDIATE, time);
		return;
	}

	return;
} /* end of ShellGotFocus */

/*
 *************************************************************************
 * HandleFocusChange - this routine is called whenever an object receives
 * a focus change event.
 ****************************procedure*header*****************************
 */
static void
HandleFocusChange(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	/* although this check is to avoid unnecessary focus
	 * changes when dragging an object (e.g., DnD op), as
	 * a result, it improves some performance...
	 */
    if (ve->xevent->xfocus.mode == NotifyGrab ||
	ve->xevent->xfocus.mode == NotifyUngrab)
		return;

    switch(ve->xevent->xfocus.detail)
    {
    /* case NotifyPointer:		*/
    /* case NotifyPointerRoot:		*/
    /* case NotifyVirtual:		*/
    /* case NotifyNonlinearVirtual:	*/
    /* 	return;				*/
    /* 	break;				*/
						/* we like these types */
    case NotifyInferior:
    case NotifyDetailNone:
    case NotifyAncestor:
    case NotifyNonlinear:

		/* The `if' is for "mwm", "mwm" sends out a
		 * WM_TAKE_FOCUS message AND then set focus
		 * to the toplevel shell (should never do this
		 * because this window in not in "mwm" space,
		 * but...) when clicking on "mwm" title. In
		 * our design, a toplevel shell should never
		 * see FocusIn unless no one from that window
		 * can take "focus", so we can't just invoke
		 * XtAcceptFocus(...) because this may run into
		 * a loop...
		 */
	if ( ve->xevent->type == FocusIn && XtIsVendorShell(w) )
	{
		ShellGotFocus(w);
	}
	else
	{
		_OlSetCurrentFocusWidget(w, (ve->xevent->type == FocusIn) ?
					 OL_IN : OL_OUT);
	}

	break;
    }
} /* END OF HandleFocusChange() */

/*
 *************************************************************************
 * HandleKeyPress - this routine is called whenever an object receives
 * a keypress
 ****************************procedure*header*****************************
 */
static void
HandleKeyPress(w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	Widget		d;	/* destination widget	*/
	XtPointer	data;

				/* Always consume the event	*/

	ve->consumed = TRUE;

	if ((d = _OlFetchMnemonicOwner(w, &data, ve))) {
		Time	time = ve->xevent->xkey.time;

		/*
		 * A mnemonic moves the focus to the widget and
		 * activates it: We (attempt to) set the focus here,
		 * but don't bother waiting to see if it was successful.
		 * We don't bother to check to see if the widget already
		 * has focus, since some widget (e.g., the flats) might
		 * have to move focus when a mnemonic is pressed.
		 */

		/* If the object being activated via a mnemonic is a
		 * flattened widget, set global flag so that the
		 * flattened widget's accept focus procedure knows which
		 * sub-object is to receive focus.
		 */
		if (XtIsSubclass(d, flatWidgetClass) == True)
		{
			Cardinal item_index = (Cardinal)data - 1;

			if (OlFlatCallAcceptFocus(d, item_index, time) == True)
			{
				(void)OlActivateWidget(d, OL_SELECTKEY, data);
			}
		}
		else
		{
			if (OlGetCurrentFocusWidget(d) == d ||
			    XtCallAcceptFocus(d, &time) == True)
			{
				(void)OlActivateWidget(d, OL_SELECTKEY, data);
			}
		}
	} else if ((d = _OlFetchAcceleratorOwner(w, &data, ve))) {
		/*
		 * An accelerator just activates the widget without
		 * moving focus.
		 */
		(void) OlActivateWidget(d, OL_SELECTKEY, data);

	} else {
		switch(ve->virtual_name) {
		case OL_MENUBARKEY:
		{
		  Widget	root;
		  if ((d = _OlGetMenubarWidget(w)) != (Widget)NULL ||
		      ( !_OlIsEmptyMenuStack(w) &&
		       (root = (Widget)_OlRootOfMenuStack(w),
			(d = _OlGetMenubarWidget(root)) != (Widget)NULL &&
			d == root) ))
		  {
		    OlActivateWidget(d, ve->virtual_name, NULL);
		  }
		  break;
		}
		  
		case OL_IMMEDIATE:
		case OL_PREV_FIELD:
		case OL_NEXT_FIELD:
		case OL_MOVERIGHT:
		case OL_MOVELEFT:
		case OL_MOVEUP:
		case OL_MOVEDOWN:
		case OL_MULTIRIGHT:
		case OL_MULTILEFT:
		case OL_MULTIUP:
		case OL_MULTIDOWN:
			(void) OlMoveFocus(w, ve->virtual_name,
					   ve->xevent->xkey.time);
			break;
		case OL_TOGGLEPUSHPIN:
		case OL_CANCEL:
		case OL_DEFAULTACTION:
			if ((d = _OlGetShellOfWidget(w)) != NULL_WIDGET)
			{
				(void) OlActivateWidget(d, ve->virtual_name,
							NULL_DATA);
			}
			break;
		case OL_HELP:
			_OlProcessHelpKey(w, ve->xevent);
			break;
		case OL_COPY:
		case OL_CUT:
			/* trap these two keys here, this allows an app does */
			/* copy/cut when the input focus is on other widget  */
			/* e.g, sweep a piece of text from a staticText, and */
			/*	then traverse to a text/textedit widget.     */
			/*	press OL_COPY and OL_PASTE. The highlighted  */
			/*	text from the staticText will be saved in the*/
			/*	clipboard after OL_COPY. The saved text will */
			/*	be showed on the text/textEdit after OL_PASTE*/

			/* Currently, we only handled the intra-process part */
			/* In the future, we should also allow this feature  */
			/* to the inter-process part.                        */

			/* note: the only way to get the selection owner is  */
			/*              thru a Xlib call                     */
			{
				Window win;

				if ((win = XGetSelectionOwner(XtDisplayOfObject(w),
						XA_PRIMARY)) != None)
				{
					d = XtWindowToWidget(XtDisplayOfObject(w), win);
					(void) OlActivateWidget(d,
							ve->virtual_name,
							NULL_DATA);
					break;
				}
			}
			/* FALLTHROUGH  `default' */
		default:
			if (ve->virtual_name != OL_UNKNOWN_KEY_INPUT)
			{
				(void) OlActivateWidget(w, ve->virtual_name,
							NULL_DATA);
			}
			break;
		}
	}
} /* END OF HandleKeyPress() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlAction - this is the generic action procedure used by all OPEN LOOK
 * widgets in their translation tables.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlAction(w, xevent, params, num_params)
	Widget		w;
	XEvent *	xevent;
	String *	params;
	Cardinal *	num_params;
{
	extern Widget 		_OlRemapDestinationWidget OL_ARGS(( Widget,
								    XEvent *));
	WidgetClass		wc_special;
	WidgetClass		wc;
	OlEventHandlerProc	handler;
	OlVirtualEventRec	ve;
	OlEventHandlerList	elist;
	int			i;

	Widget			shell = _OlGetShellOfWidget(w);
	Arg			arg[1];	/* used in two places */

#ifdef I18N
extern int Ol_last_event_received;

		/*
		 * Increment the event counter, see DoLookup() in Dynamic.c
		 */
	Ol_last_event_received++;
#endif

	if (xevent->type != FocusIn && xevent->type != FocusOut)
	{
		Boolean			pending_dsdm_info = False;

		XtSetArg(arg[0], XtNpendingDSDMInfo, &pending_dsdm_info);
		XtGetValues(shell, arg, 1);

		if (pending_dsdm_info)
		{
			/* Fetching DSDM DB because of drag-and-drop,
			 * we should ignore all events (i.e., Motion,
			 * Enter, Leave, etc.) at this point
			 * because these events are generated before
			 * calling XGrabPointer in OlDetermineMouseAction
			 * and/or OlDnDTrackDragCursor etc...
			 */
			return;
		}
	}

	if ( xevent->type == EnterNotify &&
	     xevent->xcrossing.mode == NotifyUngrab &&
	     xevent->xcrossing.detail == NotifyAncestor )
	{
			/* Based on X11R5 XLib C Library manual (8.4.2)
			 * (where describes "mode" field) - The mode
			 * member is set to indicate whether the events
			 * are ..., pseudo-motion events when a grab
			 * activates (NotifyGrab), or pseudo-motion
			 * events when a gran deactivates (NotifyUngrab).
			 *
			 * In this case, mwm grabs Button1 and will call
			 * ?XSetInputFocus? if a based window doesn't
			 * have focus when Button1 is pressed. This will
			 * generate an EnterNotify before the Button Press
			 * event. Obviously, this event can be ignored
			 * (as we handled "FocusChange"), otherwise it
			 * can cause damages (e.g., flat buttons).
			 *
			 * Note that the last check may not necessary
			 * but that's what I saw when running mwm and
			 * olwm...
			 */
		return;
	}

	switch (xevent->type)
	{
		case KeyPress:		/* FALLTHROUGH */
		case KeyRelease:	/* FALLTHROUGH */
		case FocusIn:		/* FALLTHROUGH */
		case FocusOut:
				/* switch to gadget id if necessary */
			w = _OlRemapDestinationWidget(w, xevent);
			break;
		default:
			break;
	}


	wc_special = _OlClass(w);
	if (wc_special != primitiveWidgetClass &&
	    wc_special != eventObjClass &&
	    wc_special != vendorShellWidgetClass &&
	    wc_special != managerWidgetClass)
	{
		return;
	}

	OlLookupInputEvent(w, xevent, &ve, OL_DEFAULT_IE);

		/* If "focus_on_select" is True then use "Input Focus	*/
		/* Follow SELECT Button Press" policy otherwise subclass*/
		/* will handle "input focus"...				*/
	if (ve.xevent->type == ButtonPress && ve.virtual_name == OL_SELECT)
	{
			/* I have to use CurrentTime because olwm
			 * always uses "CurrentTime" when sending
			 * WM_TAKE_FOCUS...
			 */
		Time	time = /*CurrentTime */ve.xevent->xbutton.time;
		Boolean	is_flat, is_primitive;
		Widget	cfw;
		int	focus_on_select = False;

			/* Macro is defined in OpenLookP.h		*/
		_OlGetClassField(w, focus_on_select, focus_on_select);

			/* if this base window doesn't have
			 * have focus now THEN,
			 *
			 * if this is a primitive widget and
			 * this widget can take focus and
			 * and it's traversable
			 * THEN
			 *	set focus to this widget
			 * ELSE
			 *	let shell to decide which
			 *	one should take focus if this
			 *	base window doesn't have focus
			 *	currently...
			 */
#define IS_FLAT		(is_flat = XtIsSubclass(w, flatWidgetClass))
#define IS_PRIMITIVE	(is_primitive = XtIsSubclass(w, primitiveWidgetClass))
#define NOT_SAME_ITEM	(ve.item_index != OL_NO_ITEM &&		\
			 OlFlatGetFocusItem(w) != ve.item_index)
#define NOT_SAME_WIDGET	(cfw != w)

		if ( focus_on_select == True &&
		     ((cfw = OlGetCurrentFocusWidget(w)) == NULL || 
		      ((IS_PRIMITIVE && NOT_SAME_WIDGET) ? True :
		       (is_primitive && IS_FLAT && NOT_SAME_ITEM))) )
		{
			Boolean		done = False;

			if ( is_primitive && OlCanAcceptFocus(w, time) )
			{
				Boolean	traversal_on = False;

				XtSetArg(arg[0], XtNtraversalOn, &traversal_on);
				XtGetValues(w, arg, 1);

				if (traversal_on == True)
				{
					done = True;
					is_flat && ve.item_index != OL_NO_ITEM ?
						(void)OlFlatCallAcceptFocus(
								w,
								ve.item_index,
								time) :
						(void)XtCallAcceptFocus(
								w,
								&time);
				}
			}

			if (!done)
			{
				(void)XtCallAcceptFocus(shell, &time);
			}
		}

#undef IS_FLAT
#undef IS_PRIMITIVE
#undef NOT_SAME_ITEM
#undef NOT_SAME_WIDGET
	}

	/*** CHECK APPLICATION EVENT PROCS ***/
	/*
	 * NOTE: If a spring loaded shell is mapped, the application event
	 *       handler will be caused more than once. The application
	 *	 can recognize the extra copies by comparing the serial
	 *	 numbers of consecutive events.
	 */
	for (i = pre_consume_list_size, elist = pre_consume_list;
		 (i > 0) && (elist->type != xevent->type); i--, elist++) ;
	if (i)
		(*(elist->handler))(w, &ve);

	if (OlHasCallbacks(w, XtNconsumeEvent) == XtCallbackHasSome)
	{
		OlCallCallbacks(w, XtNconsumeEvent, (XtPointer)&ve);
	}

	wc = XtClass(w);

	while (ve.consumed == FALSE) {

		handler = GetEventHandler(wc, wc_special, xevent->type);

		if (handler) {
			(*handler)(w, &ve);
		}

		if (wc == primitiveWidgetClass || wc == eventObjClass ||
		    wc == vendorShellWidgetClass || wc == managerWidgetClass)
		{
			if (ve.consumed == FALSE) {
				/* check application event procs */
				for (i=post_consume_list_size,
				     elist=post_consume_list;
		 		     (i > 0) && (elist->type != xevent->type);
				 	i--, elist++) ;
				if (i)
					(*(elist->handler))(w, &ve);
			}

			ve.consumed = TRUE;
		} else {
			wc = wc->core_class.superclass;
		}
	}
} /* END OF OlAction() */

/*
 *************************************************************************
 * OlActivateWidget - this calls a widget's class procedure to activate
 * it.  If the widget to be activated is not busy and sensitive and its
 * ancenstors are sensitive, TRUE is returned; else FALSE is returned.
 ****************************procedure*header*****************************
 */
Boolean
OlActivateWidget OLARGLIST((w, type, data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	static int		level=0;
	WidgetClass		wc;
	WidgetClass		wc_special;
	OlActivateFunc		activate;
	OlVendorClassExtension	ext;
	Boolean			ret_val;


	if (w == NULL_WIDGET) {
		return(False);
	} else if (XtIsSensitive(w) == FALSE) {
		return(False);
	}

	++level;		/* record the number of recursions	*/

				/* If we get this far, it's ok to activate
				 * the widget.				*/

	wc = XtClass(w);
	wc_special = _OlClass(w);

	if (wc_special == vendorShellWidgetClass) {
		ext = _OlGetVendorClassExtension(wc);
		activate = (OlActivateFunc) (ext ? ext->activate : NULL);
	} else if (wc_special == primitiveWidgetClass) {
		activate = ((PrimitiveWidgetClass)
					wc)->primitive_class.activate;
	} else if (wc_special == eventObjClass) {
		activate = ((EventObjClass) wc)->event_class.activate;
	} else if (wc_special == managerWidgetClass) {
		activate = ((ManagerWidgetClass) wc)->manager_class.activate;
	} else {
		activate = (OlActivateFunc)NULL;
	}

	ret_val = ((activate != NULL) && (XtIsShell(w) || XtIsManaged(w))) ?
			(*activate)(w, type, data) : False;

	/* try the asociated widget list */
	if (ret_val == FALSE) {
		Widget *l;
		Widget *f;
		Widget *e;

		/* from now on, want to know if list has been modified */
		if (level == 1)
			assoc_list_modified = FALSE;

		for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
			if (*l == w) {
				f = FollowerList + (l - LeaderList);
				do {
					ret_val= OlActivateWidget(*f,type,data);
					if ((ret_val == TRUE) ||
					    (assoc_list_modified == TRUE))
					{
						--level;
						return(TRUE);
					}
					f++;
					l++;
				}
				while ((l < e) && (*l == w));
			}
	}

	--level;
	return(ret_val);
} /* END OF OlActivateWidget() */

/*
 *************************************************************************
 * OlAssociateWidget - 
 * NOTE: The FollowerList and the LeaderList are two parallel arrays. They
 *	 are kept in separate arrays, because in the future we may want to
 *	 have a function returing a list of followers given the leader.
 ****************************procedure*header*****************************
 */
Boolean
OlAssociateWidget OLARGLIST((leader, follower, disable_traversal))
	OLARG( Widget,	leader)
	OLARG( Widget,	follower)
	OLGRA( Boolean,	disable_traversal)
{
	register Widget *l;
	register Widget *f;
	register Widget *e;

	if ((leader == NULL) || (follower == NULL)) {
		OlVaDisplayWarningMsg(	(Display *)NULL,
					OleNfileAction,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileAction_msg1);
		return(FALSE);
	}

	if (AssocWidgetListSize == AssocWidgetListAllocSize) {
		AssocWidgetListAllocSize += ASSOC_LIST_STEP;
		l = (WidgetList)XtRealloc((char *)LeaderList,
				AssocWidgetListAllocSize * sizeof(Widget));
		f = (WidgetList)XtRealloc((char *)FollowerList,
				AssocWidgetListAllocSize * sizeof(Widget));
		if ((l == NULL) || (f == NULL)) {
			AssocWidgetListAllocSize -= ASSOC_LIST_STEP;
			OlVaDisplayWarningMsg(	XtDisplay(leader),
						OleNnoMemory,
						OleTxtRealloc,
						OleCOlToolkitWarning,
						OleMnoMemory_xtRealloc,
						"Action.c",
						"OlAssociateWidget");
			return(FALSE);
		}

		LeaderList = l;
		FollowerList = f;
	}

	/*
	 * The follower widget cannot already be a follower widget in the
	 * table.
	 */
	for (f=FollowerList, e=FollowerList+AssocWidgetListSize; f < e; f++)
		if (*f == follower) {
			OlVaDisplayWarningMsg(	XtDisplay(leader),
						OleNfileAction,
						OleTmsg2,
						OleCOlToolkitWarning,
						OleMfileAction_msg2);
			return(FALSE);
		}

	/*
	 * See if the leader widget is already in the list. If so, add the new
	 * entry right next to the existing entries.
	 */
	for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
		if (*l == leader) {
			/* find the last entry with the same leader */
			do l++;
			while ((l < e) && (*l == leader));

			if (l < e) {
				/* shift down the remaining entries */
				OlMemMove(Widget, l + 1, l, e - l);

				/* also the follower list */
				f = FollowerList + (l - LeaderList);
				OlMemMove(Widget, f + 1, f, e - l);
			}
			break;
		}

	/* add an entry */
	f = FollowerList + (l - LeaderList);
	*l = leader;
	*f = follower;
	AssocWidgetListSize++;

	/* check for cycle */
	if (assoc_check_cycle(*l, *f) == TRUE) {
		OlVaDisplayWarningMsg(	XtDisplay(leader),
					OleNfileAction,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileAction_msg3);
		OlUnassociateWidget(*f);
		return(FALSE);
	}

	assoc_list_modified = TRUE;

	if (disable_traversal == TRUE) {
		static Arg arg[] = {
			{ XtNtraversalOn, FALSE },
		};

		XtSetValues(follower, arg, XtNumber(arg));
	}

	/* done */
	return(TRUE);
}

/*
 *************************************************************************
 * OlUnassociateWidget - 
 ****************************procedure*header*****************************
 */
void
OlUnassociateWidget(follower)
Widget follower;
{
	register Widget *l;
	register Widget *f;
	register Widget *e;
	
	for (f=FollowerList, e=FollowerList+AssocWidgetListSize; f < e; f++)
		if (*f == follower) {

			assoc_list_modified = TRUE;

			/* shift up the remaining entries */
			(void)memcpy((XtPointer)f, (XtPointer)(f+1),
				 (int)(e - f) * sizeof(Widget));

			/* also the leader list */
			l = LeaderList + (f - FollowerList);
			(void)memcpy((XtPointer)l, (XtPointer)(l+1),
				 (int)(e - f) * sizeof(Widget));

			AssocWidgetListSize--;
			return;
		}
}

/* ARGSUSED */
Boolean
OlSetAppEventProc OLARGLIST((w, listtype, list, count))
	OLARG(Widget, w)
	OLARG(OlDefine, listtype)
	OLARG(OlEventHandlerList, list)
	OLGRA(Cardinal, count)
{
	OlEventHandlerList *ptr;
	int *listsize;

	switch(listtype) {
	case OL_PRE:
		ptr = &pre_consume_list;
		listsize = &pre_consume_list_size;
		break;
	case OL_POST:
		ptr = &post_consume_list;
		listsize = &post_consume_list_size;
		break;
	default:
		return(FALSE);
	}

	if (*ptr)
		XtFree((char*) *ptr);

	if ((*ptr = (OlEventHandlerList)XtMalloc(sizeof(OlEventHandlerRec) *
			count)) == NULL) {
		*ptr = NULL;
		*listsize = 0;
		return(FALSE);
	}

	(void) memcpy(*ptr, list, sizeof(OlEventHandlerRec) * count);
	*listsize = count;
	return(TRUE);
}

/*
 *************************************************************************
 * assoc_check_cycle -  This function checks for cycles after an entry is
 *			is added to the table. It basically traverses all the
 *			connected nodes in the graph starting at the follower
 *			node and if it finds the leader node, then a cycle
 *			is found.
 ****************************procedure*header*****************************
 */
static Boolean
assoc_check_cycle(target, w)
Widget target;
Widget w;
{
	register Widget *l;
	register Widget *f;
	register Widget *e;

	for (l=LeaderList, e=LeaderList+AssocWidgetListSize; l < e; l++)
		if (*l == w) {
			f = FollowerList + (l - LeaderList);
			do {
				if (*f == target)
					return(TRUE);
				if (assoc_check_cycle(target, *f) == TRUE)
					return(TRUE);
				l++;
				f++;
			} while ((l < e) && (*l == w));
			return(FALSE);
		}
	return(FALSE);
}
