#ifndef	NOIDENT
#ident	"@(#)notice:ModalO.c	1.11"
#endif

/*******************************file*header*******************************
    Description: Modal.c - OPEN LOOK(TM) Modal Widget
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ModalP.h>
#include <Xol/VendorI.h>

#define ClassName ModalO
#include <Xol/NameDefs.h>


/*
 * Convenient macros:
 */

#define MODAL_C(wc) (((ModalShellWidgetClass)(wc))->modal_class)
#define CORE_C(wc) (((WidgetClass)(wc))->core_class)
#define COMPOSITE_C(wc) (((CompositeWidgetClass)(wc))->composite_class)
#define SUPER_C(wc) CORE_C(wc).superclass

#define CORE_P(w) ((Widget)(w))->core
#define COMPOSITE_P(w) ((ModalShellWidget)(w))->composite
#define MODAL_P(w) ((ModalShellWidget)(w))->modal
#define NPART(w)		( &((ModalShellWidget)(w))->modal_shell )

#define LeaveParams		LeaveWindowMask, False, LeaveEH, NULL
#define AddLeaveEH(w)		XtAddEventHandler(w, LeaveParams)
#define RemoveLeaveEH(w)	XtRemoveEventHandler(w, LeaveParams)

#define MapParams		StructureNotifyMask, False, MapEH, NULL
#define AddMapEH(w)		XtAddEventHandler(w, MapParams)
#define RemoveMapEH(w)		XtRemoveEventHandler(w, MapParams)

#define SHADOW_THICKNESS	2

/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */
						/* class procedures */
						/* action procedures */

static void LeaveEH OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void MapEH OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));

						/* public procedures */

extern void		_OloMDRedisplay OL_ARGS((
	Widget			w,
	XEvent *		event,
	Region			region
));
extern void	_OloMDLayout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
extern Boolean _OloMDCheckSetValues OL_ARGS((
	ModalShellWidget,
	ModalShellWidget
));
extern void _OloMDRemoveEventHandlers OL_ARGS((Widget));
extern void _OloMDAddEventHandlers OL_ARGS((Widget));

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/* ARGSUSED */
static void
LeaveEH OLARGLIST((w, client_data, event, cont_to_dispatch))
    OLARG(Widget,		w)
    OLARG(XtPointer,		client_data)
    OLARG(XEvent *,		event)
    OLGRA(Boolean *,		cont_to_dispatch)
{
    ModalShellWidget		mw = (ModalShellWidget)w;
    mw->modal_shell.do_unwarp = False;
}  /* end of LeaveEH() */

/******************************function*header****************************
 * MapEH- 
 */

/* ARGSUSED */
static void
MapEH OLARGLIST((w, client_data, event, cont_to_dispatch))
    OLARG(Widget,		w)
    OLARG(XtPointer,		client_data)
    OLARG(XEvent *,		event)
    OLGRA(Boolean *,		cont_to_dispatch)
{
    if (event->type == MapNotify)
	_OlBeepDisplay(w, 1);	/* figures out if beep is needed */
	
}  /* end of MapEH() */

/**
 ** _OloMDRedisplay()
 **/
/* ARGSUSED */
extern void
_OloMDRedisplay OLARGLIST((w, event, region))
	OLARG(Widget,	w)
	OLARG(XEvent *,	event)
	OLGRA(Region,	region)
{
	ModalShellWidget mw = (ModalShellWidget) w;

	/*  Draw a one point thick etched rectangle inside the Modal window. */
	OlgDrawBorderShadow( XtScreen(w), XtWindow(w),
		mw->modal_shell.attrs,
		OL_SHADOW_ETCHED_IN,	/* type */
		SHADOW_THICKNESS,	/* thickness */
		0,0,
		mw->core.width,
		mw->core.height);
			
}  /*  end of _OloMDRedisplay() */


/**
 ** _OloMDLayout()
 **/

/*ARGSUSED*/
extern void
_OloMDLayout OLARGLIST((w, resizable, query_only, cached_best_fit_hint,
	who_asking, request, response))
	OLARG(Widget,			w)
	OLARG(Boolean,			resizable)
	OLARG(Boolean,			query_only)
	OLARG(Boolean,			cached_best_fit_hint)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLGRA(XtWidgetGeometry *,	response)
{
	Cardinal		nchildren	= COMPOSITE_P(w).num_children;
	Widget *		child		= COMPOSITE_P(w).children;
	Dimension	_390points = OlPointToPixel(OL_HORIZONTAL, 390);
	Dimension	_350points = OlPointToPixel(OL_HORIZONTAL, 350);
	Dimension	_36points = OlPointToPixel(OL_VERTICAL, 36);
	Widget			static_text = NULL;
	Widget			control_area = NULL;
	Dimension		core_height, core_width;
	Dimension		preferred_ca_height, preferred_st_height;
	Dimension		preferred_ca_width;
	XtWidgetGeometry	preferred;
	XtWidgetGeometry	available;

#define CA_BW (2*CORE_P(control_area).border_width)
#define ST_BW (2*CORE_P(static_text).border_width)

	/*
	 * Compute optimum fit based on the specified size and layout
	 * of the Modal.  It is always 390 points wide and at least 72
	 * points high.
	 */
	preferred.width	= _390points;
	preferred.height = _36points * 2;

	/*  For the Modal, we assume that the child[0] is the 
	    static text widget and the child[1] is the control
	    area (or other type used for the buttons.)  */
	if (nchildren > (unsigned) 0 && XtIsManaged(child[0])) {
		XtWidgetGeometry req, reply;

		static_text = child[0];
		if (who_asking == static_text)  {
			/*  Allow the requested height.  */
			preferred.height += request->height + ST_BW;
			preferred_st_height = request->height;
		}
		else  {
			/*  Ask that the static text be 350 points wide. */
			req.width = _350points;
			req.request_mode = CWWidth;
			XtQueryGeometry(static_text, &req, &reply);
			if (reply.request_mode & CWHeight)  {
				preferred.height += reply.height + ST_BW;
				preferred_st_height = reply.height;
			}
			else  {
				preferred_st_height = CORE_P(static_text).height;
				preferred.height += preferred_st_height + ST_BW;
			}
		}
	}
	if (nchildren > 1 && XtIsManaged(child[1]))  {
		XtWidgetGeometry req, reply;

		control_area = child[1];

		if (who_asking == control_area)  {
			/*  Allow the requested height and width  */
			preferred_ca_height = request->height;
			preferred_ca_width = request->width;
		}
		else  {
			/*  Ask that the control area be 390 points wide. */
			req.width = _390points;
			req.request_mode = CWWidth;
			XtQueryGeometry(control_area, &req, &reply);
			preferred_ca_width = reply.width;
			if (reply.request_mode & CWHeight)
				preferred_ca_height = reply.height;
			else
				preferred_ca_height = CORE_P(control_area).height;
		}
		if (preferred_ca_width > _390points)
			preferred.width = preferred_ca_width + 2*SHADOW_THICKNESS + CA_BW;
		preferred.height += _OlMax((int)(preferred_ca_height + CA_BW), (int)_36points);
	}

	/*  Protect against the zero size protocol. */
	if (preferred.width == 0)
		preferred.width = 1;
	if (preferred.height == 0)
		preferred.height = 1;

	/*
	 * Skip the rest of this if our parent is just asking.
	 */
	if (who_asking == w) {
		if (response) {
			response->x            = CORE_P(w).x;
			response->y            = CORE_P(w).y;
			response->width        = preferred.width;
			response->height       = preferred.height;
			response->border_width = CORE_P(w).border_width;
		}
		return;
	}

	/*  Determine the available geometry according to the flags given */
	/*  Avoid the OlAvailableGeometry call when we know that it is a
	    query_only situation.  Assume that the window manager will grant
	    the preferred size.  The alternative is that the root 
	    geometry manager ignores the QueryOnly flag and actually sets
	    the size of the window to be the preferred (queried) size.
	    Then later the widget gets out of sync with the size of the
	    window.
	*/
	if (query_only)
		available = preferred;
	else
		OlAvailableGeometry(w, resizable, query_only, who_asking,
			request, &preferred, &available);

	/*
	 * Place all of the children in the space given in available.
	 */
	core_height = available.height;
	core_width = available.width;

	if (static_text  && XtIsManaged(static_text))  {
		Dimension	_20points = OlPointToPixel(OL_HORIZONTAL, 20);
		/*
		 * Set up the maximum preferred size.  We force the static text
		 * to be 350 points wide and as high as the preferred height
		 * from the query above.  If this is too high, then it will
		 * be clipped, and the stacking order of the control area will
		 * insure that the buttons are accessible.
		 */
		if (who_asking != static_text && !query_only)
			XtConfigureWidget (
				static_text,
				_20points, _36points,
				core_width - 2*_20points - ST_BW, preferred_st_height,
				CORE_P(static_text).border_width);
		else if (who_asking == static_text) {
			if (response)  {
				response->x            = _20points;
				response->y            = _36points;
				response->width        = core_width - 2*_20points - ST_BW;
				response->height       = preferred_st_height;
				response->border_width = CORE_P(static_text).border_width;
			}
			if (query_only)
				return;
		}
	}
	
	/*  Now size and position the control area widget if it isn't the
	    one asking.  The specification of the Modal says that the 
	    control area should be centered in a 390x36 rectangle at the
	    bottom of the Modal. */
	if (control_area  && XtIsManaged(control_area))  {
		Position	ca_x, ca_y;

		/*  Center the control area in the 390x36 rectangle, unless the
		 *  control area is taller than 36 points.
		 */
		if ((int)core_width > (int)(preferred_ca_width + CA_BW))
			ca_x = (Position)(core_width - (preferred_ca_width + CA_BW)) / 2;
		else  {
			ca_x = SHADOW_THICKNESS;
			preferred_ca_width = core_width - (SHADOW_THICKNESS*2) - CA_BW;
		}
		ca_y = ((int)core_height > (int)_OlMax((int)_36points, (int)(preferred_ca_height + SHADOW_THICKNESS + CA_BW))) ?
			core_height - _OlMax(_36points, (Dimension)
				(preferred_ca_height + SHADOW_THICKNESS + CA_BW)) :
			SHADOW_THICKNESS;
		if ((int)_36points > (int)(preferred_ca_height + CA_BW) && core_height > _36points)
			ca_y += (Position)(_36points - (preferred_ca_height + CA_BW)) / 2;

		if (who_asking != control_area && !query_only)
			XtConfigureWidget (
				control_area,
				ca_x, ca_y,
				preferred_ca_width, preferred_ca_height,
				CORE_P(control_area).border_width);
		else if (who_asking == control_area)  {
			if (response) {
				response->x            = ca_x;
				response->y            = ca_y;
				response->width        = preferred_ca_width;
				response->height       = preferred_ca_height;
				response->border_width = CORE_P(control_area).border_width;
			}
			if (query_only)
				return;
		}
	}

	return;
} /* end of _OloMDLayout() */


extern void
_OloMDAddEventHandlers OLARGLIST((w))
	OLGRA(Widget, w)
{
    AddLeaveEH(w);
    AddMapEH(w);
}  /* end of _OloMDAddEventHandlers() */

extern void
_OloMDRemoveEventHandlers OLARGLIST((w))
	OLGRA(Widget, w)
{
    RemoveLeaveEH(w);
    RemoveMapEH(w);
}  /* end of _OloMDRemoveEventHandlers() */

/* ARGSUSED */
extern Boolean
_OloMDCheckSetValues OLARGLIST((current,new))
	OLARG(ModalShellWidget, current)
	OLGRA(ModalShellWidget, new)
{
	/* noop - return redisplay False. */
	return(False);
}  /* end of _OloMDCheckSetValues() */
