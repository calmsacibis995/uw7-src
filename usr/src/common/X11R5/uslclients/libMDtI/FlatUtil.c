#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FlatUtil.c	1.3"
#endif

/******************************file*header********************************
 *
 * Description:
 *	This file contains various support and convenience routines
 *	for the flat widgets.
 */

						/* #includes go here	*/
#include <stdio.h>

#include "FlatP.h"

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations.
 */
		/***	private procedures		***/

static 	int		CheckId(Widget, _XmConst char *, Boolean, Cardinal);

				/* private DetermineMouseAction procedures */

static void		ClickTimeOut(XtPointer, XtIntervalId *);
static ExmButtonAction	DetermineMouseAction(Widget, XEvent *, Cursor, Time *);
static Boolean		EssentiallySamePoint(XEvent *, XEvent *);

		/***	private global procedures	***/

void		ExmFlatDrawExpandedItem(Widget w, ExmFlatItem item);
void		ExmFlatRefreshExpandedItem(Widget, ExmFlatItem, Boolean);
void		ExmFlatRefreshItem(Widget, Cardinal, Boolean);

			/* private global DetermineMouseAction procedures */

ExmButtonAction	ExmDetermineMouseAction(Widget w, XEvent * event);
ExmButtonAction	ExmDetermineMouseActionWithCount(Widget, XEvent *, Cardinal *);
ExmButtonAction	ExmDetermineMouseActionEntirely(Widget, XEvent *, Cursor,
						ExmMouseActionCallback,
						XtPointer);
void		ExmResetMouseAction(Widget w);

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */
#define Action(C) (C > 1? MOUSE_MULTI_CLICK : MOUSE_CLICK)

typedef struct ClickData {
    Widget			w;
    ExmMouseActionCallback	callback;
    XtPointer			client_data;
    Cardinal			count;
    Time			time;
} ClickData;

	/* MORE: Allow multiple displays. */
static XButtonEvent	prev = { 0 };
static Cardinal		nclicks = 0;

#define DFT_MDF_VALUE	8
static XtIntervalId	timer = 0;
static Cardinal		mouse_damping_factor = DFT_MDF_VALUE;
static ClickData	click = { 0 };

/****************************private*procedures***************************
 *
 * Private Procedures
 */

/****************************procedure*header*****************************
 * CheckId -
 */
static int
CheckId(Widget w, _XmConst char * proc_name,
	Boolean check_index, Cardinal item_index)
{
    int	success = 0;

    if (w == (Widget)NULL)
    {
	OlVaDisplayWarningMsg(((Display *)NULL, OleNnullWidget,
			       OleTflatState, OleCOlToolkitWarning,
			       OleMnullWidget_flatState, proc_name));
    }
    else if (ExmIsFlat(w) == False)
    {
	OlVaDisplayWarningMsg((XtDisplayOfObject(w),
			       OleNbadFlatSubclass,
			       OleTflatState,
			       OleCOlToolkitWarning,
			       OleMbadFlatSubclass_flatState,
			       proc_name,
			       XtName(w),
			       OlWidgetToClassName(w)));
    }
    else if (check_index == True && item_index > FPART(w).num_items)
    {
	OlVaDisplayWarningMsg((XtDisplay(w), OleNbadItemIndex,
			       OleTflatState, OleCOlToolkitWarning,
			       OleMbadItemIndex_flatState, XtName(w),
			       OlWidgetToClassName(w), proc_name, item_index));
    }
    else
    {
	success = 1;
    }
    return(success);
}					/* end of CheckId() */

/****************************procedure*header*****************************
/**
 ** ClickTimeOut()
 **/
/*ARGSUSED*/
static void
ClickTimeOut(
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
)
{
	ClickData *		cd = (ClickData *)client_data;


	timer = 0;

	/*
	 * The user hasn't done anything for a while, so we assume he
	 * or she has finished clicking. Report the final action, then
	 * clear things for a fresh start.
	 */
	if (cd->callback)
		(*cd->callback) (
			cd->w,
			Action(cd->count), cd->count, cd->time,
			cd->client_data
		);
	cd->count = 0;
	ExmResetMouseAction (cd->w);

	return;
} /* ClickTimeOut */

/****************************procedure*header*****************************
 * DetermineMouseAction()
 */
static ExmButtonAction
DetermineMouseAction(Widget	w,
		     XEvent *	event,
		     Cursor	cursor,
		     Time *	time)
{
    Display *		display = XtDisplayOfObject(w);
    Window			window = XtWindowOfObject(w);
    ExmButtonAction		action;
    XEvent			new;
    Boolean			grabbed;
    Time			grabtime;

#define INTEREST ButtonMotionMask|ButtonReleaseMask

    /* To avoid problems with improperly balanced Press/Release
     * or Press/Motion events, insist on being called only with
     * a ButtonPress event.
     */
    if (event->type != ButtonPress)
    {
	OlVaDisplayWarningMsg((display, "illegalEvent", "determineMouseAction",
			      OleCOlToolkitWarning,
			      "Widget %s: must start DetermineMouseAction with ButtonPress event\n",
			      XtName(w)));
	return (NOT_DETERMINED);
    }

    /* We would rather use the event timestamp, but the event may
     * have been replayed by someone who had a passive grab on the
     * button, and that could cause the event time to not work.
     * So we try the event time, but if that fails try CurrentTime.
     */
    grabtime = event->xbutton.time;
 TryAgain:
    switch (XGrabPointer(display, window, False, INTEREST, GrabModeAsync,
			 GrabModeAsync, None, cursor, grabtime)) {
    case GrabSuccess:
	grabbed = True;
	break;

    case GrabInvalidTime:
	if (grabtime != CurrentTime)
	{
	    grabtime = CurrentTime;
	    goto TryAgain;
	}
	/*FALLTHROUGH*/
    case GrabNotViewable:
    case AlreadyGrabbed:
    case GrabFrozen:
	/* It is debatable whether we should just return here or
	 * should continue. But if we impose the requirement that
	 * the caller should call DetermineMouseAction only from
	 * a button press event, then we can safely assume that
	 * there is a paired button release event in the queue,
	 * even if we can't now grab the pointer. This is because
	 * the server gives us an automatic grab on the press,
	 * releasing the grab only on the button release.
	 */
	grabbed = False;
	break;
    }

    do {
	action = NOT_DETERMINED;

	/* This used to be XWindowEvent, but that would loop
	 * forever if the above grab failed and the pointer was
	 * moved to another window. We really don't care which
	 * window, since:
	 *
	 * - for MOUSE_MOVE, we must have grabbed the pointer and
	 *   this question is moot;
	 *
	 * - for MOUSE_CLICK/MULTI_CLICK, who cares if the pointer
	 *   is now in another window, a click is a click....well,
	 *   there is a difference, i.e. we force the semantics
	 *   of a click to be relative to where the button is
	 *   pressed, not where it is released. If a client wants
	 *   different behavior, it should handle the events
	 *   itself.
	 */
	XMaskEvent (display, INTEREST, &new);

	switch (new.type)
	{
	case MotionNotify:
	    /* If we could not grab the pointer, then mouse
	     * drags (as indicated by a motion at this point)
	     * are not possible.
	     */
	    if (grabbed)
	    {
		if (!EssentiallySamePoint(&new, event))
		    action = MOUSE_MOVE;
		if (time)
		    *time = new.xmotion.time;
	    }
	    break;

	case ButtonRelease:
	    if (new.xbutton.button == event->xbutton.button) {
		action = MOUSE_CLICK;
		if (
		    new.xbutton.root == prev.root
		    && new.xbutton.time - prev.time < XtGetMultiClickTime(display)
		    && EssentiallySamePoint(&new, (XEvent *)&prev)
		    )
		    action = MOUSE_MULTI_CLICK;
		else
		    nclicks = 0;
		if (time)
		    *time = new.xbutton.time;
		prev = new.xbutton;
		nclicks++;
		if (grabbed)
		    XUngrabPointer (display, grabtime);
	    }
	    break;
	}
    } while (action == NOT_DETERMINED);

#undef	INTEREST
    return (action);
}					/* DetermineMouseAction */

/****************************procedure*header*****************************
 * EssentiallySamePoint()
 */
static Boolean
EssentiallySamePoint (XEvent * a, XEvent * b)
{
    /* XMotionEvent and XButtonEvent have in common the fields of
     * interest to us here, so either event type can be used.
     */
#define A ((XMotionEvent *)a)
#define B ((XMotionEvent *)b)

#define MDF (int)mouse_damping_factor

    return (-MDF < A->x_root - B->x_root && A->x_root - B->x_root < MDF
	    && -MDF < A->y_root - B->y_root && A->y_root - B->y_root < MDF);

#undef	MDF
#undef	B
#undef	A
}					/* EssentiallySamePoint */

/****************************public*procedures****************************
 *
 * Private Global Procedures
 */

/****************************procedure*header*****************************
 * ExmFlatDrawExpandedItem-
 */
void
ExmFlatDrawExpandedItem(Widget w, ExmFlatItem item)
{
    if (item->flat.managed && item->flat.mapped_when_managed)
    {
	ExmFlatDrawInfo	di;

	ExmFlatGetItemGeometry(w, item->flat.item_index,
			      &di.x, &di.y, &di.width, &di.height);

	di.screen = XtScreen(w);
	di.drawable = (Drawable)XtWindow(w);
	di.item_has_focus = (FPART(w).focus_item == item->flat.item_index);
	di.gc = (item->flat.selected) ?
	    FPART(w).select_gc : FPART(w).normal_gc;

	(*FCLASS(w).draw_item)(w, item, &di);
    }
} /* end of ExmFlatDrawExpandedItem */

/****************************procedure*header*****************************
 * ExmFlatRefreshExpandedItem - optionally clears an item and then makes
 * an in-line request to draw it.
 *
 * This routine calls the class's refresh routine, if it has one.
 * If a class doesn't have a refresh routine, a default refresh is done.
 */
void
ExmFlatRefreshExpandedItem(Widget w,		/* Flat Widget subclass	*/
			   ExmFlatItem item,	/* item to draw		*/
			   Boolean clear_area)	/* clear the area also?	*/
{
    if (!XtIsRealized(w) || FPART(w).items_touched)
    {
	return;
    }

    if (FCLASS(w).refresh_item)
    {
	(*FCLASS(w).refresh_item)(w, item, clear_area);
	return;
    }

    if (clear_area)
    {
	WidePosition	x, y;
	Dimension	width, height;

	ExmFlatGetItemGeometry(w, item->flat.item_index,
					&x, &y, &width, &height);

	/* Adjust (x, y) because we are dealing with virtual coords */
	x -= FPART(w).x_offset;
	y -= FPART(w).y_offset;
	(void)XClearArea(XtDisplay(w), XtWindow(w),
			 (int)x, (int)y,
			 (unsigned int)width, (unsigned int)height,
			 (Bool)False);
    }

    ExmFlatDrawExpandedItem(w, item);

} /* end of ExmFlatRefreshExpandedItem */

/****************************procedure*header*****************************
 * ExmFlatRefreshItem - convenient interface to OlFlatRefreshExpandedItem
 */
void
ExmFlatRefreshItem(Widget	w,		/* Flat Widget subclass	*/
		   Cardinal	item_index,	/* item to draw		*/
		   Boolean	clear_area)	/* clear the area also?	*/
{
    if (XtIsRealized(w) == (Boolean)True &&
	FPART(w).items_touched == False)
    {
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

	ExmFlatExpandItem(w, item_index, item);
	ExmFlatRefreshExpandedItem(w, item, clear_area);

	ExmFLAT_FREE_ITEM(item);
    }
} /* end of ExmFlatRefreshItem() */

/****************************procedure*header*****************************
 * ExmFlatSetFocus -
 */
void
ExmFlatSetFocus(Widget w, Cardinal item_index)
{
    if (item_index == ExmNO_ITEM)
    {
	/* Move focus to container if we don't already have it.
	 * last_focus_item will get focus.  Make easy check before
	 * calling XmProcessTraversal.
	 * NOTE: are there other easy checks?
	 */
	if (!PPART(w).have_traversal && PPART(w).traversal_on)
	    XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    } else
    {
	(void)ExmFlatCallAcceptFocus(w, item_index);
    }
}					/* end of ExmFlatSetFocus */

/****************************procedure*header*****************************
 * ExmFlatSetMouseDampingFactor - set the mouse damping factor.
 *	If a `0' value is passed, the value will be reset to
 *	the system default (DFT_MDT_VALUE).
 *
 *	The previous value is returned.
 */
Cardinal
ExmFlatSetMouseDampingFactor(Widget w, Cardinal value)
{
	Cardinal	old_value = mouse_damping_factor;

	if (value)
		mouse_damping_factor = value;
	else
		mouse_damping_factor = DFT_MDF_VALUE;

	return old_value;

} /* end of ExmSetMouseDampingFactor */

/****************************procedure*header*****************************
 * ExmDetermineMouseAction
 */
ExmButtonAction
ExmDetermineMouseAction(Widget w, XEvent * event)
{
    return (DetermineMouseAction(w, event, None, (Time *)0));

}					/* ExmDetermineMouseAction */

/****************************procedure*header*****************************
 * ExmDetermineMouseActionWithCount
 */
ExmButtonAction
ExmDetermineMouseActionWithCount(Widget w, XEvent * event, Cardinal * count)
{
    ExmButtonAction action = DetermineMouseAction(w, event, None, (Time *)0);
    if (count)
	*count = nclicks;
    return (action);
}					/* ExmDetermineMouseActionWithCount */

/****************************procedure*header*****************************
 * ExmDetermineMouseActionEntirely()
 */
ExmButtonAction
ExmDetermineMouseActionEntirely(Widget w,
				XEvent * event,
				Cursor cursor,
				ExmMouseActionCallback callback,
				XtPointer client_data)
{
    Display *			display = XtDisplayOfObject(w);
    ExmButtonAction		action;
    Time			time;
    Boolean			call_previous_action;
    Boolean			call_current_action;

    if (timer)
    {
	XtRemoveTimeOut (timer);
	timer = 0;
    }

    action = DetermineMouseAction(w, event, cursor, &time);
    switch (action) {
	/*
	 * Report a MOUSE_MOVE without delay. If an action had
	 * started before this, report it first and close it out.
	 */
    case MOUSE_MOVE:
	call_previous_action = True;
	call_current_action = True;
	break;

	/*
	 * Delay reporting a MOUSE_CLICK or MOUSE_MULTI_CLICK
	 * until we've waited long enough to know the user has
	 * stopped beating on the button.
	 *
	 * We ignore the difference between MOUSE_CLICK and
	 * MOUSE_MULTI_CLICK, since DetermineMouseAction relies
	 * on the event timestamps to tell the difference. Since
	 * we use a client-side timer anyway, we'll rely entirely
	 * on the timer.
	 */
    case MOUSE_MULTI_CLICK:
    case MOUSE_CLICK:
	call_previous_action = False;
	call_current_action = False;
	click.w = w;
	click.callback = callback;
	click.client_data = client_data;
	click.count++;
	click.time = time;
	timer = ExmAddTimeOut(
			      w, XtGetMultiClickTime(display),
			      ClickTimeOut, &click
			      );
	action = Action(click.count);
	break;

	/*
	 * If DeterineMouseAction returns an error, call any
	 * action that's pending, then close it out.
	 */
    default:
	call_previous_action = True;
	call_current_action = False;
	break;
    }

    if (call_previous_action && click.count)
	ClickTimeOut ((XtPointer)&click, (XtIntervalId *)0);
    if (call_current_action && callback)
	(*callback) (w, action, 0, time, client_data);

    return (action);
}					/* ExmDetermineMouseActionEntirely */

/****************************procedure*header*****************************
 * ExmResetMouseAction
 */
void
ExmResetMouseAction(Widget w)
{
    prev.root = None;
    nclicks = 0;
    return;
}					/* ExmResetMouseAction */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
 * ExmFlatCallAcceptFocus - public interface to setting focus
 * to a flattened widget item.
 */
Boolean
ExmFlatCallAcceptFocus(Widget w, Cardinal i)
{
    Boolean	took_it = False;

    if (CheckId(w, (_XmConst char *)"ExmFlatCallAcceptFocus", True, i) &&
	FCLASS(w).item_accept_focus)
    {
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

	ExmFlatExpandItem(w, i, item);
	took_it = (*FCLASS(w).item_accept_focus)(w, item);

	ExmFLAT_FREE_ITEM(item);
    }
    return (took_it);
}					/* end of ExmFlatCallAcceptFocus() */

/****************************procedure*header*****************************
 * ExmFlatGetFocusItem - returns the current focus item for a flat
 * widget.  If there is no current focus item, ExmNO_ITEM is returned.
 */
Cardinal
ExmFlatGetFocusItem(Widget w)
{
    if (!CheckId(w, (_XmConst char *)"ExmFlatGetFocusItem", False, 0))
    {
	return(ExmNO_ITEM);
    }
    return(((ExmFlatWidget)w)->flat.focus_item);

}					/* end of ExmFlatGetFocusItem() */

/****************************procedure*header*****************************
 * ExmFlatGetItemGeometry - returns the item at the given coordinates.
 */
#undef ExmFlatGetItemGeometry
void
ExmFlatGetItemGeometry(Widget w, 
		       Cardinal i, 		/* item_index	*/
		       WidePosition * x_ret, 
		       WidePosition * y_ret, 
		       Dimension * w_ret, 
		       Dimension * h_ret)
{
    if (!CheckId(w, (_XmConst char *)"ExmFlatGetItemGeometry", True, i))
    {
	*x_ret = *y_ret = (WidePosition)0;
	*w_ret = *h_ret = (Dimension)0;
    }
    else
    {
	ExmFlatDrawInfo	di;

#define ExmFlatGetItemGeometry(w,i,x,y,wi,h) \
	(*FCLASS(w).get_item_geometry)(w,i,x,y,wi,h)

    ExmFlatGetItemGeometry(w, i, &di.x, &di.y, &di.width, &di.height);

    *x_ret = di.x;
    *y_ret = di.y;
    *w_ret = di.width;
    *h_ret = di.height;
    }
}				/* end of ExmFlatGetItemGeometry() */

/****************************procedure*header*****************************
 * ExmFlatGetItemIndex - returns the item at the given coordinates.
 */
Cardinal
ExmFlatGetItemIndex(Widget w, WidePosition x, WidePosition y)
{
    if (!CheckId(w, (_XmConst char *)"ExmFlatGetItemIndex", False, 0))
    {
	return(ExmNO_ITEM);
    }
    return(ExmFlatGetIndex(w, x, y, False));
}					/* end of ExmFlatGetItemIndex() */
