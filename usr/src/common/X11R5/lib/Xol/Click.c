#if	!defined(NOIDENT)
#ident	"@(#)olmisc:Click.c	1.6"
#endif

#include "X11/IntrinsicP.h"

#include "Xol/OpenLookP.h"

/*
 * Convenient macros:
 */

#define Action(C) (C > 1? MOUSE_MULTI_CLICK : MOUSE_CLICK)

/*
 * Local types:
 */

typedef struct ClickData {
	Widget			w;
	OlMouseActionCallback	callback;
	XtPointer		client_data;
	Cardinal		count;
	Time			time;
}			ClickData;

/*
 * Local data:
 */

	/*
	 * MORE: Allow multiple displays.
	 */

static XButtonEvent	prev = { 0 };
static Cardinal		nclicks = 0;

static XtIntervalId	timer = 0;

static Cardinal		multi_click_timeout;
static Cardinal		mouse_damping_factor;

static ClickData	click = { 0 };

/*
 * Local routines:
 */

static void		ClickTimeOut OL_ARGS((
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
));
static ButtonAction	DetermineMouseAction OL_ARGS((
	Widget			w,
	XEvent *		event,
	Cursor			cursor,
	Time *			time
));
static void		RegisterDynamicUpdate OL_ARGS((
	Widget			w
));
static void		UpdateLocal OL_ARGS((
	XtPointer		client_data
));
static Boolean		EssentiallySamePoint OL_ARGS((
	XEvent *		a,
	XEvent *		b
));

/**
 ** OlResetMouseAction
 **/

void
#if	OlNeedFunctionPrototypes
OlResetMouseAction (
	Widget			w
)
#else
OlResetMouseAction (w)
	Widget			w;
#endif
{
	prev.root = None;
	nclicks = 0;
	return;
} /* OlResetMouseAction */

/**
 ** OlDetermineMouseAction
 **/

ButtonAction
#if	OlNeedFunctionPrototypes
OlDetermineMouseAction (
	Widget			w,
	XEvent *		event
)
#else
OlDetermineMouseAction (w, event)
	Widget			w;
	XEvent *		event;
#endif
{
	return (DetermineMouseAction(w, event, None, (Time *)0));
} /* OlDetermineMouseAction */

/**
 ** OlDetermineMouseActionWithCount
 **/

ButtonAction
#if	OlNeedFunctionPrototypes
OlDetermineMouseActionWithCount (
	Widget			w,
	XEvent *		event,
	Cardinal *		count
)
#else
OlDetermineMouseActionWithCount (w, event, count)
	Widget			w;
	XEvent *		event;
	Cardinal *		count;
#endif
{
	ButtonAction action
		= DetermineMouseAction(w, event, None, (Time *)0);
	if (count)
		*count = nclicks;
	return (action);
} /* OlDetermineMouseActionWithCount */

/**
 ** OlDetermineMouseActionEntirely()
 **/

ButtonAction
#if	OlNeedFunctionPrototypes
OlDetermineMouseActionEntirely (
	Widget			w,
	XEvent *		event,
	Cursor			cursor,
	OlMouseActionCallback	callback,
	XtPointer		client_data
)
#else
OlDetermineMouseActionEntirely (w, event, cursor, callback, client_data)
	Widget			w;
	XEvent *		event;
	Cursor			cursor;
	OlMouseActionCallback	callback;
	XtPointer		client_data;
#endif
{
	ButtonAction		action;

	Time			time;

	Boolean			call_previous_action;
	Boolean			call_current_action;


	if (timer) {
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
		timer = OlAddTimeOut(
			w, multi_click_timeout, ClickTimeOut, &click
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
} /* OlDetermineMouseActionEntirely */

/**
 ** ClickTimeOut()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ClickTimeOut (
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
)
#else
ClickTimeOut (client_data, id)
	XtPointer		client_data;
	XtIntervalId *		id;
#endif
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
	OlResetMouseAction (cd->w);

	return;
} /* ClickTimeOut */

/**
 ** DetermineMouseAction()
 **/

static ButtonAction
#if	OlNeedFunctionPrototypes
DetermineMouseAction (
	Widget			w,
	XEvent *		event,
	Cursor			cursor,
	Time *			time
)
#else
DetermineMouseAction (w, event, cursor, time)
	Widget			w;
	XEvent *		event;
	Cursor			cursor;
	Time *			time;
#endif
{
	Display *		display = XtDisplayOfObject(w);

	Window			window = XtWindowOfObject(w);

	ButtonAction		action;

	XEvent			new;

	Boolean			grabbed;

	Time			grabtime;

#define INTEREST ButtonMotionMask|ButtonReleaseMask


	RegisterDynamicUpdate (w);

	/*
	 * To avoid problems with improperly balanced Press/Release
	 * or Press/Motion events, insist on being called only with
	 * a ButtonPress event.
	 */
	if (event->type != ButtonPress) {
		OlVaDisplayWarningMsg (
			display,
			"illegalEvent", "determineMouseAction",
			OleCOlToolkitWarning,
			"Widget %s: must start DetermineMouseAction with ButtonPress event\n",
			XtName(w)
		);
		return (NOT_DETERMINED);
	}

	/*
	 * We would rather use the event timestamp, but the event may
	 * have been replayed by someone who had a passive grab on the
	 * button, and that could cause the event time to not work.
	 * So we try the event time, but if that fails try CurrentTime.
	 */
	grabtime = event->xbutton.time;
TryAgain:
	switch (XGrabPointer(
		display, window, False, INTEREST,
		GrabModeAsync, GrabModeAsync, None, cursor, grabtime
	)) {
	case GrabSuccess:
		grabbed = True;
		break;

	case GrabInvalidTime:
		if (grabtime != CurrentTime) {
			grabtime = CurrentTime;
			goto TryAgain;
		}
		/*FALLTHROUGH*/
	case GrabNotViewable:
	case AlreadyGrabbed:
	case GrabFrozen:
		/*
		 * It is debatable whether we should just return here or
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

		/*
		 * This used to be XWindowEvent, but that would loop
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

		switch (new.type) {
		case MotionNotify:
			/*
			 * If we could not grab the pointer, then mouse
			 * drags (as indicated by a motion at this point)
			 * are not possible.
			 */
			if (grabbed) {
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
				 && new.xbutton.time - prev.time < multi_click_timeout
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
} /* DetermineMouseAction */

/**
 ** RegisterDynamicUpdate()
 **/

static void
#if	OlNeedFunctionPrototypes
RegisterDynamicUpdate (
	Widget			w
)
#else
RegisterDynamicUpdate (w)
	Widget			w;
#endif
{
	/*
	 * MORE: Allow multiple displays.
	 */
	static Boolean first_time = True;
	if (first_time) {
		UpdateLocal ((XtPointer)w);
		OlRegisterDynamicCallback (UpdateLocal, (XtPointer)w);
		first_time = False;
	}
	return;
} /* RegisterDynamicUpdate */

/**
 ** UpdateLocal()
 **/

static void
#if	OlNeedFunctionPrototypes
UpdateLocal (
	XtPointer		client_data
)
#else
UpdateLocal (client_data)
	XtPointer		client_data;
#endif	
{
	Widget			w = (Widget)client_data;

	Arg			args[2];

	/*
	 * MORE: Use Xt's multiClickTime.
	 */
	XtSetArg (args[0], XtNmultiClickTimeout, &multi_click_timeout);
	XtSetArg (args[1], XtNmouseDampingFactor, &mouse_damping_factor);
	OlGetApplicationValues (w, args, 2);
	return;
} /* UpdateLocal */

/**
 ** EssentiallySamePoint()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
EssentiallySamePoint (
	XEvent *		a,
	XEvent *		b
)
#else
EssentiallySamePoint (w, a, b)
	XEvent *		a;
	XEvent *		b;
#endif
{
	/*
	 * XMotionEvent and XButtonEvent have in common the fields of
	 * interest to us here, so either event type can be used.
	 */
#define A ((XMotionEvent *)a)
#define B ((XMotionEvent *)b)

#define MDF (int)mouse_damping_factor

	return (
	     -MDF < A->x_root - B->x_root && A->x_root - B->x_root < MDF
	  && -MDF < A->y_root - B->y_root && A->y_root - B->y_root < MDF
	);

#undef	MDF
#undef	B
#undef	A
} /* EssentiallySamePoint */
