#ifndef	NOIDENT
#ident	"@(#)changebar:ChangeBar.c	1.19"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/ChangeBar.h"

#define ClassName ChangeBar
#include <Xol/NameDefs.h>

/*
 * Define some macros for checking a widget's class heritage:
 */

#include "Xol/ControlAre.h"
#define IsControlArea(W) XtIsSubclass(W,controlAreaWidgetClass)

#include "Xol/PopupMenuP.h"
#define IsPopupMenuShell(W) XtIsSubclass(W,popupMenuShellWidgetClass)

#include "Xol/Category.h"
#define IsCategory(W) XtIsSubclass(W,categoryWidgetClass)

/*
 * Local routines:
 */

static void		GetGCs OL_ARGS((
	Widget			w,
	Pixel			color,
	ChangeBar *		cb
));
static void		FreeGCs OL_ARGS((
	Widget			w,
	ChangeBar *		cb
));
static void		PropagateToCategory OL_ARGS((
	Widget			w,
	OlDefine		state,
	unsigned int		propagate
));

/**
 ** THESE SHOULD GO AWAY SOON!
 **/

Pixel
_OlContrastingColor OLARGLIST((w, pixel, percent))
	OLARG( Widget,			w)
	OLARG( Pixel,			pixel)
	OLGRA( int,			percent)
{
	return (OlContrastingColor(w, pixel, percent));
}

ChangeBar *
_OlCreateChangeBar OLARGLIST((w))
	OLGRA( Widget,	w)
{
	return (OlCreateChangeBar(
		w, OlContrastingColor(w, CORE_P(w).background_pixel, 25)
	));
}

void
_OlDestroyChangeBar OLARGLIST((w, cb))
	OLARG( Widget,			w)
	OLGRA( ChangeBar *,		cb)
{
	OlDestroyChangeBar (w, cb);
}

void
_OlSetChangeBarState OLARGLIST((w, state, propagate))
	OLARG( Widget,			w)
	OLARG( OlDefine,		state)
	OLGRA( unsigned int,		propagate)
{
	OlSetChangeBarState (w, state, propagate);
}

void
_OlFlatSetChangeBarState OLARGLIST((w, indx, state, propagate))
	OLARG( Widget,			w)
	OLARG( Cardinal,		indx)
	OLARG( OlDefine,		state)
	OLGRA( unsigned int,		propagate)
{
	OlFlatSetChangeBarState (w, indx, state, propagate);
}

void
_OlDrawChangeBar OLARGLIST((w, cb, state, expose, x, y, region))
	OLARG( Widget,			w)
	OLARG( ChangeBar *,		cb)
	OLARG( OlDefine,		state)
	OLARG( Boolean,			expose)
	OLARG( Position,		x)
	OLARG( Position,		y)
	OLGRA( Region,			region)
{
	OlDrawChangeBar (w, cb, state, expose, x, y, region);
}

void
_OlGetChangeBarGCs OLARGLIST((w, cb))
	OLARG( Widget,			w)
	OLGRA( ChangeBar *,		cb)
{
	return;
}

void
_OlFreeChangeBarGCs OLARGLIST((w, cb))
	OLARG( Widget,			w)
	OLGRA( ChangeBar *,		cb)
{
	OlChangeBarSetValues (
	    w, OlContrastingColor(w, CORE_P(w).background_pixel, 25), cb
	);
}

/**
 ** OlCreateChangeBar()
 **/

ChangeBar *
OlCreateChangeBar OLARGLIST((w, color))
	OLARG( Widget,		w )
	OLGRA( Pixel,		color )
{
	ChangeBar *		cb;


	cb = (ChangeBar *)memset(XtNew(ChangeBar), 0, sizeof(ChangeBar));

#define PtToPixel(AXIS,V) OlScreenPointToPixel(AXIS,V,XtScreenOfObject(w))
	cb->width  = PtToPixel(OL_HORIZONTAL, CHANGE_BAR_WIDTH);
	cb->height = PtToPixel(OL_VERTICAL,   CHANGE_BAR_HEIGHT);
	cb->pad	   = PtToPixel(OL_HORIZONTAL, CHANGE_BAR_PAD);
#undef	PtToPixel

	GetGCs (w, color, cb);

	return (cb);
} /* OlCreateChangeBar */

/**
 ** OlDestroyChangeBar()
 **/

void
OlDestroyChangeBar OLARGLIST((w, cb))
	OLARG( Widget,		w )
	OLGRA( ChangeBar *,	cb )
{
	if (cb) {
		FreeGCs (w, cb);
		XtFree ((char *)cb);
	}
	return;
} /* OlDestroyChangeBar */

/**
 ** OlSetChangeBarState()
 **/

void
OlSetChangeBarState OLARGLIST((w, state, propagate))
	OLARG( Widget,		w )
	OLARG( OlDefine,	state )
	OLGRA( unsigned int,	propagate )
{
	Widget			x = 0;


	if (propagate & OL_PROPAGATE_TO_CONTROL_AREA) {
		/*
		 * Find the ancestor of this widget which is a direct
		 * child of a control area. We assume the widget passed
		 * to us isn't a control area.
		 *
		 * Note: Don't stop at a control area that's a descendent
		 * of a menu.
		 */
		do
			do
				w = XtParent((x = w));
			while (w && !IsControlArea(w));
		while (w && IsPopupMenuShell(_OlGetShellOfWidget(w)));
		if (!w)
			x = 0;
	} else
		x = w;

	if (x) {
		Arg arg;
		XtSetArg (arg, XtNchangeBar, state);
		XtSetValues (x, &arg, 1);
		PropagateToCategory (w, state, propagate);
	}

	return;
} /* OlSetChangeBarState */

/**
 ** OlFlatSetChangeBarState()
 **/

void
OlFlatSetChangeBarState OLARGLIST((w, indx, state, propagate))
	OLARG( Widget,		w )
	OLARG( Cardinal,	indx )
	OLARG( OlDefine,	state )
	OLGRA( unsigned int,	propagate )
{
	Arg			arg;

	XtSetArg (arg, XtNchangeBar, state);
	OlFlatSetValues (w, indx, &arg, 1);
	PropagateToCategory (w, state, propagate);
	return;
} /* OlFlatSetChangeBarState */

/**
 ** OlDrawChangeBar()
 **/

void
OlDrawChangeBar OLARGLIST((w, cb, state, expose, x, y, region))
	OLARG( Widget,		w )
	OLARG( ChangeBar *,	cb )
	OLARG( OlDefine,	state )
	OLARG( Boolean,		expose )
	OLARG( Position,	x )
	OLARG( Position,	y )
	OLGRA( Region,		region )
{
	Display *		display	= XtDisplayOfObject(w);

	Window			window	= XtWindowOfObject(w);

	Dimension		width	= cb->width;
	Dimension		height	= cb->height;

	GC			gc;


	if (window == None) /* i.e. not realized */
		return;

	/*
	 * If a region was given, see if the change bar is within it.
	 */
	if (region)
		switch (XRectInRegion(region, x, y, width, height)) {

		case RectangleOut:
			return;

		case RectanglePart:
			/*
			 * Don't bother trying to optimize drawing a
			 * partial change bar, as they are pretty small.
			 */
		case RectangleIn:
			break;

		}

	/*
	 * Now draw (or erase) the change bar according to its type.
	 */
	switch ((expose? OL_NONE : state)) {

	case OL_DIM:
		gc = cb->dim_GC;
		goto Draw;
	case OL_NORMAL:
		gc = cb->normal_GC;
Draw:		XFillRectangle (display, window, gc, x, y, width, height);
		break;

	case OL_NONE:
		XClearArea (display, window, x, y, width, height, expose);
		break;

	}

	return;
} /* OlDrawChangeBar */

/**
 ** OlChangeBarSetValues()
 **/

void
OlChangeBarSetValues OLARGLIST((w, color, cb))
	OLARG (Widget,		w )
	OLARG (Pixel,		color )
	OLGRA (ChangeBar *,	cb )
{
	FreeGCs (w, cb);
	GetGCs (w, color, cb);
	return;
} /* OlChangeBarSetValues */

/**
 ** GetGCs()
 **/

static void
GetGCs OLARGLIST((w, color, cb))
	OLARG( Widget,		w )
	OLARG( Pixel,		color )
	OLGRA( ChangeBar *,	cb )
{
	XGCValues		v;


	v.foreground = color;
	cb->normal_GC = XtGetGC(w, GCForeground, &v);

	v.fill_style = FillStippled;
	v.stipple    = OlGet50PercentGrey(XtScreenOfObject(w));
	cb->dim_GC = XtGetGC(w, GCForeground|GCFillStyle|GCStipple, &v);

	return;
} /* GetGCs */

/**
 ** FreeGCs()
 **/

static void
FreeGCs OLARGLIST((w, cb))
	OLARG( Widget,		w )
	OLGRA( ChangeBar *,	cb )
{
	XtReleaseGC (w, cb->normal_GC);
	XtReleaseGC (w, cb->dim_GC);
	return;
} /* FreeGCs */

/**
 ** PropagateToCategory()
 **/

static void
PropagateToCategory OLARGLIST((w, state, propagate))
	OLARG( Widget,		w )
	OLARG( OlDefine,	state )
	OLGRA( unsigned int,	propagate )
{
	Widget			x;


	/*
	 * We propagate the change indication up to a category
	 * widget only if (1) we've been asked to, and (2) if
	 * the change bar state is dim or normal. We don't
	 * propagate the change up to a category widget if the
	 * change bar state is ``none'', because the category
	 * ``changed'' state is the union of potentially several
	 * changes. The client has to clear a category ``changed''
	 * state manually.
	 */
	if (propagate & OL_PROPAGATE_TO_CATEGORY && state != OL_NONE) {
		do
			w = XtParent((x = w));
		while (w && !IsCategory(w));
		if (x) {
			Arg arg;
			XtSetArg (arg, XtNchanged, True);
			XtSetValues (x, &arg, 1);
		}
	}
	return;
} /* PropagateToCategory */
