#ifndef	NOIDENT
#ident	"@(#)layout:LayoutExt.c	1.35"
#endif

#include "string.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"
#include "X11/Shell.h"

#include "Xol/OpenLookP.h"
#include "Xol/ManagerP.h"
#include "Xol/LayoutExtP.h"
#include "Xol/HandlesExP.h"
#include "Xol/Error.h"

#define ClassName LayoutExt
#include <Xol/NameDefs.h>

/*
 * Define the following if we need to watch out for dumb composites
 * that ignore XtCWQueryOnly.
 */
#define CHECK_IGNORED_QUERY			/* */

/*
 * Define the following if we need to avoid sending geometry queries
 * (XtCWQueryOnly) to the root geometry manager of Shell.
 */
#define DONT_QUERY_ROOT_GEOMETRY_MANAGER	/* */

/*
 * Define the following if you want to be able to turn on debugging
 * information in the binary product.
 */
#define	GEOMETRY_DEBUG
#if	defined(GEOMETRY_DEBUG)
static Boolean		geometry_debug = False;

static void
Debug__DumpGeometry OLARGLIST((g))
	OLGRA(XtWidgetGeometry *, g)
{
# define P(SFIELD,FIELD,FLAG) \
	if (g->request_mode & FLAG) printf (" %s/%d", SFIELD, g->FIELD)
	P("x", x, CWX);
	P("y", y, CWY);
	P("width", width, CWWidth);
	P("height", height, CWHeight);
	P("border_width", border_width, CWBorderWidth);
# undef	P
	return;
}
#endif

/*
 * Private types:
 */

typedef unsigned int	LocalGravity;
#define _LEFT	0x0001
#define _RIGHT	0x0002
#define _TOP	0x0004
#define _BOTTOM	0x0008

/*
 * Convenient macros:
 */

#define GetLayoutExtension(WC) \
	GetLayoutExtension_QInherited(WC, (Boolean *)0)

#define INIT(pVar) \
	_OlArrayInitialize(pVar, 5, 25, LayoutWidgetExtensionCompare)

	/*
	 * Stolen from Xt/IntrinsicI.h:
	 */
#define XtStackAlloc(size, stack_cache_array) \
	(size <= sizeof(stack_cache_array)?				\
		  (XtPointer)stack_cache_array : XtMalloc((unsigned)size))
#define XtStackFree(pointer, stack_cache_array) \
	(void)((XtPointer)(pointer) != (XtPointer)stack_cache_array?	\
			XtFree((XtPointer)pointer),0 : 0)

	/*
	 * Some typed numbers to keep our pal, the ANSI-C compiler, happy.
	 */
#if	defined(__STDC__)
# define u2	2U
#else
# define u2	2
#endif

/*
 * Private routines:
 */

static void		ClassPartInitialize OL_ARGS((
	WidgetClass		wc
));
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static XtGeometryResult	QueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred,
	Dimension *		border_width
));
static void		GetLayoutExtensionPointers OL_ARGS((
	Widget			w,
	LayoutCoreClassExtension *E,
	LayoutWidgetExtension *	e
));
static LayoutCoreClassExtension	GetLayoutExtension_QInherited OL_ARGS((
	WidgetClass		wc,
	Boolean *		inherited
));
static LayoutWidgetExtension	GetLayoutWidgetExtension OL_ARGS((
	LayoutCoreClassExtension E,
	Widget			w
));
static int		FindWidget OL_ARGS((
	LayoutCoreClassExtension E,
	Widget			w
));
static void		LayoutWidget OL_ARGS((
	Widget			w,
	LayoutCoreClassExtension E,
	LayoutWidgetExtension	e,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static int		LayoutWidgetExtensionCompare OL_ARGS((
	XtPointer		pA,
	XtPointer		pB
));
static LocalGravity	AugmentGravity OL_ARGS((
	int			gravity
));


/*
 * Private data:
 */

static struct save {
	XtWidgetClassProc	class_part_initialize;
	XtInitProc		initialize;
	XtWidgetProc		destroy;
}			save;

/*
 * Public data:
 */

XrmQuark		XtQLayoutCoreClassExtension = 0;

/**
 ** _OlInitializeLayoutCoreClassExtension()
 **/

void
#if	OlNeedFunctionPrototypes
_OlInitializeLayoutCoreClassExtension (
	void
)
#else
_OlInitializeLayoutCoreClassExtension ()
#endif
{
	/*
	 * We need to be "informed" when every class is initialized,
	 * when every widget and gadget is created, and when every widget
	 * and gadget is destroyed. We can get this by "enveloping"
	 * RectObj's class_part_initialize, initialize, and destroy
	 * methods.
	 */
#define ENVELOPE(METHOD,NEW) \
	save.METHOD = RECT_C(rectObjClass).METHOD;			\
	RECT_C(rectObjClass).METHOD = NEW

	ENVELOPE (class_part_initialize, ClassPartInitialize);
	ENVELOPE (initialize, Initialize);
	ENVELOPE (destroy, Destroy);
#undef	ENVELOPE

	XtQLayoutCoreClassExtension
			= XrmStringToQuark(XtNLayoutCoreClassExtension);

#if	defined(GEOMETRY_DEBUG)
	geometry_debug = (getenv("GEOMETRY_DEBUG") != 0);
#endif
	return;
} /* _OlInitializeLayoutCoreClassExtension */

/**
 ** _OlDefaultResize()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultResize (
	Widget			w
)
#else
_OlDefaultResize (w)
	Widget			w;
#endif
{
	/*
	 * If this widget's class doesn't have a layout method, "inherit"
	 * the correct superclass' resize method.
	 * Note: It is assumed that only Vendor, Manager, Primitive, and
	 * EventObj use _OlDefaultResize.
	 */
	LayoutCoreClassExtension E = GetLayoutExtension(XtClass(w));
	if (!E || !E->layout) {
		XtWidgetProc resize = CORE_C(SUPER_C(_OlClass(w))).resize;
		if (resize)
			(*resize)(w);
	} else {
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"Resize: %s/%s/%x\n",
				XtName(w), CLASS(XtClass(w)), w
			);
		}
#endif
		OlSimpleLayoutWidget (w, False, True);
	}

	return;
} /* _OlDefaultResize */

/**
 ** _OlDefaultQueryGeometry()
 **/

XtGeometryResult
#if	OlNeedFunctionPrototypes
_OlDefaultQueryGeometry (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
)
#else
_OlDefaultQueryGeometry (w, request, preferred)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
#endif
{
	return (QueryGeometry(w, request, preferred, (Dimension *)0));
} /* _OlDefaultQueryGeometry */

/**
 ** OlQueryGeometryFixedBorder()
 **/

XtGeometryResult
#if	OlNeedFunctionPrototypes
OlQueryGeometryFixedBorder (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
)
#else
OlQueryGeometryFixedBorder (w, request, preferred)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
#endif
{
	return (QueryGeometry(w, request, preferred, &CORE_P(w).border_width));
} /* OlQueryGeometryFixedBorder */

/**
 ** _OlDefaultGeometryManager()
 **/

XtGeometryResult
#if	OlNeedFunctionPrototypes
_OlDefaultGeometryManager (
	Widget			child,
	XtWidgetGeometry *	_request,
	XtWidgetGeometry *	reply
)
#else
_OlDefaultGeometryManager (child, _request, reply)
	Widget			child;
	XtWidgetGeometry *	_request;
	XtWidgetGeometry *	reply;
#endif
{
	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;

	Widget			gm = XtParent(child);

	XtGeometryResult	result = XtGeometryYes;

	XtWidgetGeometry	request;
	XtWidgetGeometry	response;

	Boolean			query = True;
	Boolean			cache_hint = False;

	static struct {
		Widget			child;
		XtWidgetGeometry	request;
	}			cache = { 0 };


	/*
	 * If the parent's class doesn't have a layout method, "inherit"
	 * the correct superclass' geometry_manager method, unless the
	 * child's parent (the geometry "manager") is a subclass of
	 * WMShell (see why below).
	 *
	 * NOTE: It is assumed that only Vendor and Manager use
	 * _OlDefaultGeometryManager.
	 */
	GetLayoutExtensionPointers (gm, &E, &e);
	if (!E || !E->layout) {
		XtGeometryHandler	geometry_manager;

		/*
		 * If the parent of this child is a subclass of WMShell
		 * that doesn't have a layout method, don't propagate a
		 * query. So far with X11R4 and X11R5 there appears to be
		 * a bug in Xt: Shell ignores the XtCWQueryOnly flag in
		 * its geometry_manager method, which means it will set
		 * the core fields of the widget even for a query. But it
		 * doesn't reconfigure the window and it returns
		 * XtGeometryYes; since XtMakeGeometryRequest obeys the
		 * XtCWQueryOnly flag, it doesn't reconfigure the window
		 * either. If we propogate a query and then the child
		 * later asks for real, the second request will be NO-OP'd
		 * because it will appear the widget and window already
		 * have the desired geometry.
		 *
		 * We boldly assume that most window managers are pleasant
		 * and will typically OK a geometry request. Thus, on
		 * queries, we optimistically return Yes. This keeps the
		 * widget and window in sync, but at the cost of fooling
		 * the child into thinking it can get what it wants. But
		 * there is no guarantee that a child will get what it
		 * earlier queried (this is different from the guarantee
		 * in an Almost case), so proper children have to be
		 * prepared for a lie.
		 */
		if (XtIsWMShell(gm) && _request->request_mode & XtCWQueryOnly)
			return (XtGeometryYes);

		geometry_manager
		    = COMPOSITE_C(SUPER_C(_OlClass(gm))).geometry_manager;
		if (geometry_manager)
			return (*geometry_manager)(child, _request, reply);
		else
			/*
			 * We need the following to guard against broken
			 * composites. Returning Yes may seem pretty
			 * generous, but is No any better? All old OLIT
			 * composites that behaved correctly should
			 * continue to work--none of them inherited
			 * geometry_manager, so none would get here.
			 */
			return (XtGeometryYes);
	}

	/*
	 * We don't care about any geometry except size and position.
	 */
	if (!(_request->request_mode & (CWX|CWY|CWWidth|CWHeight|CWBorderWidth)))
		return (XtGeometryYes);

	/*
	 * If this parent is already in the process of laying out its
	 * children, then any geometry request is suspicious. More to
	 * the point, we can't continue without requiring that all the
	 * layout class methods be reentrant!
	 */
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (
			"GeometryManager: %s/%s/%x %s/%s/%x layout_flags %x\n",
			XtName(gm), CLASS(XtClass(gm)), gm,
			XtName(child), CLASS(XtClass(child)), child,
			e->layout_flags
		);
	}
#endif
	if (e->layout_flags & _OlLayoutActive) {
		/*
		 * We make the assumption that a child asking for a size
		 * when the parent is doing layout is a bug, such as
		 * where a child was told to be a certain size and is
		 * incorrectly asking its parent for that size. Returning
		 * Yes is consistent with that assumption.
		 */
		return (XtGeometryYes);
	}

	/*
	 * Allow others (e.g. the Handles Extension) to know that the
	 * geometry_manager is operating.
	 * WARNING: If you add more return points, be sure to clear this
	 * flag before returning!
	 */
	e->layout_flags |= _OlLayoutInGeometryManager;

	/*
	 * Make a copy so that we can fiddle with the values.
	 * We will set or change values in request, then copy
	 * the complete structure to reply upon returning.
	 */
	request = *_request;

	/*
	 * We can skip some of the following if this is an immediate
	 * retry from the same child.
	 */
	if (cache.child == child) {
		/*
		 * A particular field "matches" if neither this request
		 * nor the previous request expressed interest, or if both
		 * expressed interest and the fields are the same. XORing
		 * the request_mode let's us easily catch mismatches in
		 * interest.
		 *
		 * We compare fields instead of comparing the entire
		 * structure to allow the caller to make innocuous changes
		 * to the request.
		 */
		XtGeometryMask mask
		    = cache.request.request_mode ^ request.request_mode;

#define SAME(BIT,F) \
	((mask & BIT) &&						\
	 (!(request.request_mode & BIT) || cache.request.F == request.F))
		if (
			SAME(CWX, x) && SAME(CWY, y)
		     && SAME(CWWidth, width) && SAME(CWHeight, height)
		     && SAME(CWBorderWidth, border_width)
		) {
			cache_hint = True;
			query = False;
		}
#undef	SAME
	}

	/*
	 * The layout method requires all geometry fields filled in.
	 */
#define VALID(BIT,F) \
	if (!(request.request_mode & BIT))				\
		request.F = CORE_P(child).F

	VALID (CWX, x);
	VALID (CWY, y);
	VALID (CWWidth, width);
	VALID (CWHeight, height);
	VALID (CWBorderWidth, border_width);
#undef	VALID

	/*
	 * Try a tentative (query-only) layout. If we discover the
	 * request can't be met, we'll return an almost. If the request
	 * can be met, we'll do the real layout. This keeps us from
	 * resizing the parent or moving other children in an almost
	 * condition--a condition we won't know for sure until we try
	 * the layout.
	 */

	OlLayoutWidget (
		gm, True, query, cache_hint, child, &request, &response
	);

#define CHECK(BIT,F) \
	if (request.request_mode & BIT && request.F != response.F)	\
		result = XtGeometryAlmost

	CHECK (CWX, x);
	CHECK (CWY, y);
	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
	CHECK (CWBorderWidth, border_width);
#undef	CHECK

	if (
		result == XtGeometryYes
	     && !(request.request_mode & XtCWQueryOnly)
	     && query
	)
		/*
		 * Having called layout once already with the hint False,
		 * forcing layout to recalculate the optimum geometry,
		 * we can reasonably hint that the cached optimum is now
		 * correct.
		 */
		OlLayoutWidget (
			gm, True, False, True, child, &response, &response
		);

	/*
	 * When returning XtGeometryYes, we have to update the widget's
	 * geometry to reflect the requested values. We're a Yes manager,
	 * not a Done manager, so we don't update the window's geometry
	 * but let the Intrinsics do that. The reply structure is
	 * undefined when returning XtGeometryYes.
	 *
	 * When returning XtGeometryAlmost, we need to tell the child
	 * all the fields we will change if it accepts the compromise.
	 *
	 * It is slightly ambiguous whether the reply structure is
	 * undefined when returning XtGeometryNo. The description of the
	 * geometry_manager method is pretty clear, but the description of
	 * the set_values_almost method might lead one to think the reply
	 * structure should be valid. We make it valid just in case the
	 * caller is mistaken.
	 *
	 * If returning XtGeometryAlmost, cache the child's ID and the
	 * compromise geometry in the likely event the child immediately
	 * accepts the compromise. Otherwise, clear the cache to avoid
	 * being confused by a another call from the same child.
	 */
	cache.child = 0;
	switch (result) {
	case XtGeometryYes:
		if (!(request.request_mode & XtCWQueryOnly)) {
			CORE_P(child).x = response.x;
			CORE_P(child).y = response.y;
			CORE_P(child).width = response.width;
			CORE_P(child).height = response.height;
			CORE_P(child).border_width = response.border_width;
		}
		break;
	case XtGeometryAlmost:
		/*
		 * Copy the fields we don't care about from the child's
		 * request--they're OK.
		 */
		response.request_mode
			= request.request_mode & (CWSibling|CWStackMode);
		response.sibling = request.sibling;
		response.stack_mode = request.stack_mode;

		response.request_mode
			|= CWX|CWY|CWWidth|CWHeight|CWBorderWidth;

		cache.child = child;
		cache.request = response;
		/*FALLTHROUGH*/
	case XtGeometryNo:
		if (reply)
			*reply = response;
		break;
	}

	/*
	 * If this isn't the Handles widget itself, and if we're able to
	 * satisfy this child's request, we can now tell the Handles child
	 * (if any) to lay out its stuff.
	 */
	e->layout_flags &= ~_OlLayoutInGeometryManager;
	if (
		!OlIsHandlesWidget(gm, child)
	     && !(request.request_mode & XtCWQueryOnly)
	     && result == XtGeometryYes
	)
		OlLayoutHandles (gm);

#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (
			"GeometryManager: %s/%s/%x %s/%s/%x %s",
			XtName(gm), CLASS(XtClass(gm)), gm,
			XtName(child), CLASS(XtClass(child)), child,
			(result == XtGeometryYes?
			      "Yes"
			    : (result == XtGeometryNo? "No" : "Almost"))
		);
		Debug__DumpGeometry (_request);
		if (result != XtGeometryYes) {
			printf (" ->");
			Debug__DumpGeometry (reply);
		}
		printf ("\n");
	}
#endif

	return (result);
} /* _OlDefaultGeometryManager */

/**
 ** _OlDefaultChangeManaged()
 **/

void
#if	OlNeedFunctionPrototypes
_OlDefaultChangeManaged (
	Widget			w
)
#else
_OlDefaultChangeManaged (w)
	Widget			w;
#endif
{
	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;


	/*
	 * If this widget's class doesn't have a layout method, "inherit"
	 * the correct superclass' change_managed method.
	 * Note: It is assumed that only Vendor and Manager use
	 * _OlDefaultChangeManaged.
	 */
	GetLayoutExtensionPointers (w, &E, &e);
	if (!E || !E->layout) {
		XtWidgetProc change_managed
			= COMPOSITE_C(SUPER_C(_OlClass(w))).change_managed;
		if (change_managed)
			(*change_managed)(w);
	} else {
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"ChangeManaged: %s/%s/%x\n",
				XtName(w), CLASS(XtClass(w)), w
			);
		}
#endif
		e->layout_flags |= _OlLayoutInChangeManaged;
		OlSimpleLayoutWidget (w, True, False);
		e->layout_flags &= ~_OlLayoutInChangeManaged;
		OlUpdateHandles (w);
	}

	return;
} /* _OlDefaultChangeManaged */

/**
 ** OlLayoutWidget()
 **/

void
#if	OlNeedFunctionPrototypes
OlLayoutWidget (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
OlLayoutWidget (w, resizable, query_only, cached_best_fit_ok_hint, who_asking, request, response)
	Widget			w;
	Boolean			resizable;
	Boolean			query_only;
	Boolean			cached_best_fit_ok_hint;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;

	GetLayoutExtensionPointers (w, &E, &e);
	if (E && E->layout)
		LayoutWidget (
			w, E, e,
			resizable, query_only, cached_best_fit_ok_hint,
			who_asking, request, response
		);

	/*
	 * Let the Handles widget (if any) learn about the new layout.
	 * change_managed and geometry_manager have other ways of doing
	 * this.
	 */
	if (
		e && !(e->layout_flags & _OlLayoutInChangeManaged)
	     && !(e->layout_flags & _OlLayoutInGeometryManager)
	     && !query_only
	)
		OlLayoutHandles (w);

	return;
} /* OlLayoutWidget */

/**
 ** OlSimpleLayoutWidget()
 **/

void
#if	OlNeedFunctionPrototypes
OlSimpleLayoutWidget (
	Widget			w,
	Boolean			resizable,
	Boolean			cached_best_fit_ok_hint
)
#else
OlSimpleLayoutWidget (w, resizable, cached_best_fit_ok_hint)
	Widget			w;
	Boolean			resizable;
	Boolean			cached_best_fit_ok_hint;
#endif
{
	OlLayoutWidget (
		w, resizable, False, cached_best_fit_ok_hint,
		(Widget)0, (XtWidgetGeometry *)0, (XtWidgetGeometry *)0
	);
	return;
} /* OlSimpleLayoutWidget */

/**
 ** OlLayoutWidgetIfLastClass()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlLayoutWidgetIfLastClass (
	Widget			w,
	WidgetClass		wc,
	Boolean			do_layout,
	Boolean			cached_best_fit_ok_hint
)
#else
OlLayoutWidgetIfLastClass (w, wc, do_layout, cached_best_fit_ok_hint)
	Widget			w;
	WidgetClass		wc;
	Boolean			do_layout;
	Boolean			cached_best_fit_ok_hint;
#endif
{
	Boolean			layout_needed = False;

	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;


	/*
	 * Each class may detect the need to to do a relayout, but to
	 * avoid laying things out more than once and/or to avoid having
	 * the super-class do the wrong layout (because a subclass changes
	 * some of the super-class' constraints) we have to wait until the
	 * "last" class' set_values is called.
	 */
	GetLayoutExtensionPointers (w, &E, &e);
	if (E && E->layout) {
		/*
		 * Store the "worst case" value for the 
		 * cached_best_fit_ok_hint flag.
		 */
		if (!cached_best_fit_ok_hint)
			e->layout_flags |= _OlLayoutNotOKHint;
		if (do_layout)
			e->layout_flags |= _OlLayoutDo;
		layout_needed = (e->layout_flags & _OlLayoutDo) != 0;

		if (XtClass(w) == wc && layout_needed) {
			LayoutWidget (
				w, E, e,
				True, False,
				(e->layout_flags & _OlLayoutNotOKHint) == 0,
				(Widget)0, (XtWidgetGeometry *)0, (XtWidgetGeometry *)0
			);
			/*
			 * Prepare for the next widget.
			 */
			e->layout_flags = 0;
		}
	}
	return (layout_needed);
} /* OlLayoutWidgetIfLastClass */

/**
 ** OlAvailableGeometry()
 **/

void
#if	OlNeedFunctionPrototypes
OlAvailableGeometry (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	available
)
#else
OlAvailableGeometry (w, resizable, query_only, who_asking, request, preferred, available)
	Widget			w;
	Boolean			resizable;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
	XtWidgetGeometry *	available;
#endif
{
#if	defined(CHECK_IGNORED_QUERY)
	Dimension		core_width;
	Dimension		core_height;
#endif

	XtWidgetGeometry	status_area;
	XtWidgetGeometry	pref;
	XtWidgetGeometry	req;

#define CoreEqualsPreferred(W,P) \
	((P)->width == CORE_P(W).width && (P)->height == CORE_P(W).height)


	/*
	 * If this widget is managing a status area, we need to fool
	 * the caller into using less geometry than really available,
	 * so that the status area has some room.
	 */
	status_area.request_mode = 0;
	XtVaGetValues (
		w,
		XtNstatusAreaGeometry, (XtArgVal)&status_area,
		(String)0
	);
	if (status_area.request_mode & (CWWidth|CWHeight)) {
		pref.width = preferred->width;
		if (status_area.request_mode & CWWidth) {
			if (pref.width < status_area.width)
				pref.width = status_area.width;
			if (	who_asking == w 
		             && request->request_mode & CWWidth
			     && request->width < status_area.width
			) {
				req.request_mode = request->request_mode;
				req.width = status_area.width;
				req.height = request->height;
				request = &req;
			}
		}
		pref.height = preferred->height;
		if (status_area.request_mode & CWHeight)
			pref.height += status_area.height;
		preferred = &pref;
	}
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug && status_area.request_mode) {
		printf (
			"OlAvailableGeometry: %s/%s/%x status_area",
			XtName(w), CLASS(XtClass(w)), w
		);
		Debug__DumpGeometry (&status_area);
		printf ("\n");
	}
#endif

	/*
	 * If we got here from an XtQueryGeometry, request may have the
	 * suggested size. However, it may not if the request was open-
	 * ended.
	 */
	if (who_asking == w) {
#define REQUEST(FIELD,FLAG) \
		if (request->request_mode & FLAG)			\
			available->FIELD = request->FIELD;		\
		else							\
			available->FIELD = preferred->FIELD

		REQUEST (width, CWWidth);
		REQUEST (height, CWHeight);
#undef	REQUEST

	/*
	 * Otherwise, if we can resize (and we need to) then ask the
	 * parent for the desired size.
	 */
	} else if (resizable && !CoreEqualsPreferred(w, preferred)) {
#if	defined(DONT_QUERY_ROOT_GEOMETRY_MANAGER)
	    if (XtIsShell(w) && query_only) {
		/*
		 * As of X11R5 (and earlier), the Shell's root geometry
		 * manager routine misbehaves when the XtCWQueryOnly
		 * is set. (We would be setting this below if query_only
		 * is True.) The bad behavior is that the shell's window
		 * is configured without regard for XtCWQueryOnly; see
		 * the big comment below under CHECK_IGNORED_QUERY for
		 * the general problem.
		 *
		 * A specific ramification of the problem with Shell is
		 * that, if we're running without a window manager (e.g.
		 * graphical login window), then Shell will get a timeout
		 * in trying the desired geometry and will think the
		 * window manager is "dead"--subsequent attempts will
		 * always get a No. Thus the CHECK_IGNORED_QUERY isn't
		 * sufficient to deal with this case.
		 */
		available->width = preferred->width;
		available->height = preferred->height;
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"Don't Query RootGeometryManager: %s/%s/%x available -> (%d,%d)",
				XtName(w), CLASS(XtClass(w)), w,
				available->width, available->height
			);
			Debug__DumpGeometry (preferred);
			printf ("\n");
		}
#endif
	    } else {
#endif
		preferred->request_mode = (CWWidth|CWHeight);
		if (query_only)
			preferred->request_mode |= XtCWQueryOnly;

#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"Making geometry request: %s/%s/%x",
				XtName(w), CLASS(XtClass(w)), w
			);
			Debug__DumpGeometry (preferred);
			printf ("\n");
		}
#endif
#if	defined(CHECK_IGNORED_QUERY)
		core_width = CORE_P(w).width;
		core_height = CORE_P(w).height;
TryAgain:
#endif
		switch (XtMakeGeometryRequest(w, preferred, available)) {
		case XtGeometryAlmost:
#if	defined(GEOMETRY_DEBUG)
			if (geometry_debug) printf (" Almost");
#endif
			if (!query_only)
				XtMakeGeometryRequest (
				      w, available, (XtWidgetGeometry *)0
				);
			break;
		case XtGeometryYes:
#if	defined(GEOMETRY_DEBUG)
			if (geometry_debug) printf (" Yes");
#endif
#if	defined(CHECK_IGNORED_QUERY)
			/*
			 * A dumb parent that ignores XtCWQueryOnly will
			 * update the widget's geometry when Yes, but
			 * XtMakeGeometryRequest will not reconfigure the
			 * widget's window. If we detect this case we
			 * reset the widget's geometry and try again
			 * without the query flag. This isn't perfect,
			 * since the widget's siblings may have different
			 * geometry than before thus the parent may decide
			 * it now can't give this child the same geometry;
			 * also, we don't know if the manager is an
			 * XtGeometryDone manager--in which case the
			 * widget's window is indeed correct. The
			 * alternative would be to do the window configure
			 * here. Yuck!
			 */
			if (
				preferred->request_mode & XtCWQueryOnly
			     && CoreEqualsPreferred(w, preferred)
			) {
#if	defined(GEOMETRY_DEBUG)
				if (geometry_debug) printf (" (dumb parent)");
#endif
				CORE_P(w).width = core_width;
				CORE_P(w).height = core_height;
				preferred->request_mode &= ~XtCWQueryOnly;
				goto TryAgain;
			}
#endif
			available->width  = preferred->width;
			available->height = preferred->height;
			break;
		case XtGeometryNo:
#if	defined(GEOMETRY_DEBUG)
			if (geometry_debug) printf (" No");
#endif
			available->width  = CORE_P(w).width;
			available->height = CORE_P(w).height;
			break;
		}
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			available->request_mode = CWWidth|CWHeight;
			Debug__DumpGeometry (available);
			printf ("\n");
		}
#endif
#if	defined(DONT_QUERY_ROOT_GEOMETRY_MANAGER)
	    }
#endif

	/*
	 * Otherwise (if we can't resize), use what we have.
	 */
	} else {
		available->width  = CORE_P(w).width;
		available->height = CORE_P(w).height;
	}

	if (status_area.request_mode & CWHeight)
		if (available->height > status_area.height)
			available->height -= status_area.height;
		else
			available->height = 1;

#undef	CoreEqualsPreferred
	return;
} /* OlAvailableGeometry */

/**
 ** OlAdjustGeometry()
 **/

void
#if	OlNeedFunctionPrototypes
OlAdjustGeometry (
	Widget			w,
	OlLayoutResources *	layout,
	XtWidgetGeometry *	best_fit,
	XtWidgetGeometry *	preferred
)
#else
OlAdjustGeometry (w, layout, best_fit, preferred)
	Widget			w;
	OlLayoutResources *	layout;
	XtWidgetGeometry *	best_fit;
	XtWidgetGeometry *	preferred;
#endif
{
	/*
	 * If OL_MINIMIZE, request the size that fits.
	 *
	 * If OL_MAXIMIZE, request the larger of the current size
	 * or the size that fits.
	 *
	 * If OL_IGNORE but the client hasn't given a size yet,
	 * request the size that fits.
	 */

#define DECISION(DIR,FLAG) \
	switch (layout->DIR) {						\
	case OL_MINIMIZE:						\
		preferred->DIR = best_fit->DIR;				\
		break;							\
	case OL_MAXIMIZE:						\
		preferred->DIR = _OlMax(best_fit->DIR, CORE_P(w).DIR);	\
		break;							\
	case OL_IGNORE:							\
		if (layout->flags & FLAG) {				\
			preferred->DIR = best_fit->DIR;			\
			layout->flags &= ~FLAG;				\
		} else							\
			preferred->DIR = CORE_P(w).DIR;			\
		break;							\
	}

	DECISION (width, OlLayoutWidthNotSet);
	DECISION (height, OlLayoutHeightNotSet);

#undef	DECISION
	return;
} /* OlAdjustGeometry */

/**
 ** OlInitializeGeometry()
 **/

void
#if	OlNeedFunctionPrototypes
OlInitializeGeometry (
	Widget			w,
	OlLayoutResources *	layout,
	Dimension		width,
	Dimension		height
)
#else
OlInitializeGeometry (w, layout, width, height)
	Widget			w;
	OlLayoutResources *	layout;
	Dimension		width;
	Dimension		height;
#endif
{
	layout->flags &= ~(OlLayoutWidthNotSet|OlLayoutHeightNotSet);
	if (!CORE_P(w).width) {
		CORE_P(w).width = width;
		if (!CORE_P(w).width)
			CORE_P(w).width = 1;
		layout->flags |= OlLayoutWidthNotSet;
	}
	if (!CORE_P(w).height) {
		CORE_P(w).height = height;
		if (!CORE_P(w).height)
			CORE_P(w).height = 1;
		layout->flags |= OlLayoutHeightNotSet;
	}
	return;
} /* OlInitializeGeometry */

/**
 ** OlConfigureChild()
 **/

void
#if	OlNeedFunctionPrototypes
OlConfigureChild (
	Widget			child,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	Dimension		border_width,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	response
)
#else
OlConfigureChild (child, x, y, width, height, border_width, query_only, who_asking, response)
	Widget			child;
	Position		x;
	Position		y;
	Dimension		width;
	Dimension		height;
	Dimension		border_width;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	response;
#endif
{
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (
			"Configure: %s/%s/%x %s/%s/%x (%d,%d;%d,%d;%d)%s%s\n",
			XtName(XtParent(child)),
			CLASS(XtClass(XtParent(child))), XtParent(child),
			XtName(child), CLASS(XtClass(child)), child,
			x, y, width, height, border_width,
			(who_asking == child? " asking":""),
			(query_only? " query":"")
		);
	}
#endif

	if (!query_only && who_asking != child) {
		XtConfigureWidget (
			child, x, y, width, height, border_width
		);
	} else if (who_asking == child && response) {
		response->x = x;
		response->y = y;
		response->width  = width;
		response->height = height;
		response->border_width = border_width;
	}
	return;
} /* OlConfigureChild */

/**
 ** OlConfigureStatusArea()
 **/

void
#if	OlNeedFunctionPrototypes
OlConfigureStatusArea (
	Widget			w
)
#else
OlConfigureStatusArea (w)
	Widget			w;
#endif
{
	XtWidgetGeometry	status_area;


	status_area.request_mode = 0;
	XtVaGetValues (
		w,
		XtNstatusAreaGeometry, (XtArgVal)&status_area,
		(String)0
	);
	/* Update the x,y position of the status area.
	 * If the width is not already set, set it
	 * to the width of the current widget.
	 * The request_mode field of the XtWidgetGeometry
	 * structure is set to tell the widget setvalue
	 * routine which fields we are updating.
	 */
	if (status_area.request_mode & CWHeight) {
		if (!(status_area.request_mode & CWWidth)){
		    status_area.width = (int)CORE_P(w).width;
		    status_area.request_mode = CWWidth;
		}
		else
		    status_area.request_mode = 0;
		status_area.x = 0;
		status_area.y = (int)CORE_P(w).height - (int)status_area.height;
		status_area.request_mode |= CWX|CWY;


#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"ConfigureStatusArea: %s/%s/%x (%d,%d) ->",
				XtName(w), CLASS(XtClass(w)), w,
				CORE_P(w).width, CORE_P(w).height
			);
			Debug__DumpGeometry (&status_area);
			printf("\n");
		}
#endif
		/*
		 * &status_area is correct; Xt requires types larger
		 * than XtArgVal to be passed by reference.
		 *
		 * MORE: Theoretically, should check size to decide
		 * how to pass the value.
		 */
		XtVaSetValues (
			w,
			XtNstatusAreaGeometry, (XtArgVal)&status_area,
			(String)0
		);
	}

	return;
} /* OlConfigureStatusArea */

/**
 ** OlCheckLayoutResources()
 **/

void
#if	OlNeedFunctionPrototypes
OlCheckLayoutResources (
	Widget			w,
	OlLayoutResources *	new,
	OlLayoutResources *	current
)
#else
OlCheckLayoutResources (w, new, current)
	Widget			w;
	OlLayoutResources *	new;
	OlLayoutResources *	current;
#endif
{
#define SET_OR_RESET(NEW,CURRENT,DIR,DEFAULT) \
	if (CURRENT)							\
		NEW->DIR = CURRENT->DIR;				\
	else								\
		NEW->DIR = DEFAULT

#define CHECK(DIR,RESOURCE) \
	switch (new->DIR) {						\
	case OL_MINIMIZE:						\
	case OL_MAXIMIZE:						\
	case OL_IGNORE:							\
		break;							\
	default:							\
		OlVaDisplayWarningMsg (					\
			XtDisplayOfObject(w),				\
			"illegalValue", "set",				\
			OleCOlToolkitWarning,				\
			"Widget %s: XtN%s resource given illegal value",\
			XtName(w), RESOURCE				\
		);							\
		SET_OR_RESET (new, current, DIR, OL_MINIMIZE);		\
	}

	CHECK (width, XtNlayoutWidth);
	CHECK (height, XtNlayoutHeight);
#undef	CHECK
#undef	SET_OR_RESET

	return;
} /* OlCheckLayoutResources */

/**
 ** OlResolveGravity()
 **/

void
#if	OlNeedFunctionPrototypes
OlResolveGravity (
	Widget			w,
	int			gravity,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	constrain,
	XtWidgetGeometry *	allocated,
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	optimum,
	XtGeometryHandler	query_geometry
)
#else
OlResolveGravity (w, gravity, available, constrain, allocated, preferred, optimum, query_geometry)
	Widget			w;
	int			gravity;
	XtWidgetGeometry *	available;
	XtWidgetGeometry *	constrain;
	XtWidgetGeometry *	allocated;
	XtWidgetGeometry *	preferred;
	XtWidgetGeometry *	optimum;
	XtGeometryHandler	query_geometry;
#endif
{
	XtWidgetGeometry	query;
	XtWidgetGeometry	response;

	XtGeometryMask		optimum_needs;

	LocalGravity		grav = AugmentGravity(gravity);

	/*
	 * MORE: Don't assume the widget's border width is constant,
	 * allow the widget to offer a different size in its response
	 * to our query.
	 */
	Dimension		bw2 = 2*CORE_P(w).border_width;


	/*
	 * available: total available space for the widget plus
	 *            "decorations" (whatever they are...)
	 * constrain: maximum size to which the widget plus decorarions
	 *            can be stretched.
	 * allocated: estimated size and position of the widget; the
	 *            estimated size should be the largest allowed by
	 *            constrain. allocated->request_mode flags may be 0
	 *            to indicate that the corresponding width/height
	 *            shouldn't constrain the widget's preference.
	 * preferred: perhaps gives widget's requested size
	 *            (preferred never null, but request_mode may be 0).
	 * optimum:   perhaps gives widget's previously queried size
	 *            (optimum is null if not known).
	 *
	 * "Gravity" positions the widget within available or constrain.
	 * constrain may differ from available if external constraints
	 * exist. allocated gives the starting position of the widget, and
	 * gives the nominal size of the widget. The difference between
	 * available and constrain is applied to the space from allocated
	 * to give the total space within which to position the widget.
	 *
	 * Exception: Where "gravity" will cause the widget to span width
	 * or height the dimension is from constrain, because we want
	 * constrain to indeed constrain.
	 *
	 * Also, where gravity causes attachment to one edge or centering,
	 * we will update constrain to be the same as available, since
	 * constrain no longer needs to constrain. Furthermore, in any
	 * case if we will return a larger size than in allocated, we
	 * update constrain to reflect that size.
	 *
	 * If we have a preferred dimension, take it, subject to an upper
	 * limit of allocated. It is assumed the preferred dimension was
	 * considered when the overall layout was computed, so it usually
	 * won't differ from allocated.
	 *
	 * If we don't have a preferred dimension, and if that dimension
	 * is to be "stretched" to fit allocated, then query the widget
	 * with the allocated dimension and take the widget's preference
	 * limited by allocated. If the dimension is not to be stretched,
	 * but center or justified, then query the widget for its optimum
	 * size or use optimum if given.
	 */


	/*
	 * Reduce available and constrain by the amount of space consumed
	 * by the widget's border. But make sure to avoid underflow when
	 * available or constrain aren't big enough for even the border.
	 */
#define CHECK(DIM) \
	if (DIM < bw2)							\
		bw2 = DIM

	CHECK (available->width);
	CHECK (available->height);
	CHECK (constrain->width);
	CHECK (constrain->height);
#undef	CHECK

	available->width -= bw2;
	available->height -= bw2;
	constrain->width -= bw2;
	constrain->height -= bw2;

	query.request_mode = 0;
	optimum_needs = 0;
#define CHECK(BIT,FIELD,FORE,AFT) \
	if ((preferred->request_mode & BIT) == 0) {			\
		if ((grav & (FORE|AFT)) == (FORE|AFT)) {		\
			query.request_mode |= BIT;			\
			query.FIELD = allocated->FIELD;			\
		} else							\
			optimum_needs |= BIT;				\
	}

	CHECK (CWWidth, width, _LEFT, _RIGHT)
	CHECK (CWHeight, height, _TOP, _BOTTOM)
#undef	CHECK

	if (query.request_mode || (optimum_needs && !optimum))
		OlQueryGeometry (w, &query, &response, query_geometry);

	if (optimum_needs) {
		XtWidgetGeometry *	p = optimum? optimum : &response;

		if (optimum_needs & CWWidth)
			preferred->width = p->width;
		if (optimum_needs & CWHeight)
			preferred->height = p->height;
	}
	if (query.request_mode) {
		if (query.request_mode & CWWidth)
			preferred->width = response.width;
		if (query.request_mode & CWHeight)
			preferred->height = response.height;
	}

#define	LIMIT(BIT,FIELD) \
	if (								\
	     allocated->request_mode & BIT				\
	  && preferred->FIELD > allocated->FIELD			\
	)								\
		preferred->FIELD = allocated->FIELD

	LIMIT (CWWidth, width);
	LIMIT (CWHeight, height);
#undef	LIMIT

#define GRAVITATE(POS,DIM,FORE,AFT) \
	if ((grav & (FORE|AFT)) == (FORE|AFT))				\
		preferred->POS = allocated->POS;			\
	else {								\
		Dimension	dim;					\
		int		sum;					\
									\
		preferred->POS = allocated->POS;			\
		/*							\
		 * available can be less than constrain, when the	\
		 * caller wasn't given enough in available for its	\
		 * minimum needs.					\
		 */							\
		sum = (int)available->DIM - (int)constrain->DIM;	\
		sum = (int)allocated->DIM + (sum > 0? sum : 0);		\
		dim = (Dimension)_OlMax(sum, (int)preferred->DIM);	\
		switch (grav & (FORE|AFT)) {				\
		case FORE:						\
			break;						\
		case AFT:						\
			preferred->POS += dim - preferred->DIM;		\
			break;						\
		default:						\
			preferred->POS += (dim - preferred->DIM) / u2;	\
			break;						\
		}							\
		constrain->DIM = available->DIM;			\
	}

	GRAVITATE (x, width, _LEFT, _RIGHT)
	GRAVITATE (y, height, _TOP, _BOTTOM)
#undef	GRAVITATE

	/*
	 * If we've allowed preferred to exceed allocated, update
	 * constrain to match the larger size.
	 */
#define UPDATE(FIELD) \
	if (allocated->FIELD < preferred->FIELD)			\
		constrain->FIELD += preferred->FIELD - allocated->FIELD

	UPDATE (width);
	UPDATE (height);
#undef	UPDATE

	available->width += bw2;
	available->height += bw2;
	constrain->width += bw2;
	constrain->height += bw2;

	return;
} /* OlResolveGravity */

/**
 ** OlQueryChildGeometry()
 **/

void
#if	OlNeedFunctionPrototypes
OlQueryChildGeometry (
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred
)
#else
OlQueryChildGeometry (w, suggested, preferred)
	Widget			w;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	preferred;
#endif
{
	OlQueryGeometry (w, suggested, preferred, XtQueryGeometry);
	return;
} /* OlQueryChildGeometry */

/**
 ** OlQueryGeometry()
 **/

void
#if	OlNeedFunctionPrototypes
OlQueryGeometry (
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred,
	XtGeometryHandler	query_geometry
)
#else
OlQueryGeometry (w, suggested, preferred, query_geometry)
	Widget			w;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	preferred;
	XtGeometryHandler	query_geometry;
#endif
{
	XtWidgetGeometry	junk;

#if	defined(GEOMETRY_DEBUG)
	Boolean yes = False;
	if (geometry_debug) {
		printf (
			"OlQueryGeometry: %s/%s/%x",
			XtName(w), CLASS(XtClass(w)), w
		);
	}
#endif
	/*
	 * We provide a wrapper around XtQueryGeometry because the latter
	 * doesn't enforce the specification as strongly as we would like.
	 * We also provide a level of convenience over XtQueryGeometry.
	 * In particular, OlQueryGeometry ensures that preferred
	 * always contains the geometry that should be used for the
	 * widget, copying the geometry from suggested or core, if
	 * necessary.
	 */

	/*
	 * suggested == NULL and suggested->request_mode == 0 are
	 * equivalent, so map the one into the other for our convenience.
	 */
	if (!suggested)
		(suggested = &junk)->request_mode = 0;

#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		Debug__DumpGeometry (suggested);
		printf ("\n");
	}
#endif
	switch ((*query_geometry)(w, suggested, preferred)) {

	case XtGeometryYes:
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) { printf (" Yes"); yes = True; }
#endif
		/*
		 * Yes: The widget accepts the suggestion. This is
		 * essentially the same as returning request_mode == 0;
		 * make it so and we can reuse the code for Almost.
		 */
		preferred->request_mode = 0;
		/*FALLTHROUGH*/

	case XtGeometryAlmost:
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug && !yes) printf (" Almost");
#endif
		/*
		 * Almost: The widget accepts some suggested fields and
		 * has preferences for others; the latter are identified
		 * in the request_mode returned.
		 */
#define CHECK(BIT,F) \
		if (!(preferred->request_mode & BIT))			\
			preferred->F = suggested->request_mode & BIT?	\
				suggested->F : CORE_P(w).F

		CHECK (CWX, x);
		CHECK (CWY, y);
		CHECK (CWWidth, width);
		CHECK (CWHeight, height);
		CHECK (CWBorderWidth, border_width);
#undef	CHECK
		break;

	case XtGeometryNo:
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) printf (" No");
#endif
		/*
		 * No: The widget likes things just the way they are.
		 */
		preferred->x = CORE_P(w).x;
		preferred->y = CORE_P(w).y;
		preferred->width = CORE_P(w).width;
		preferred->height = CORE_P(w).height;
		preferred->border_width = CORE_P(w).border_width;
		break;
	}
#if	defined(GEOMETRY_DEBUG)
	if (geometry_debug) {
		printf (" ->");
		Debug__DumpGeometry (preferred);
		printf ("\n");
	}
#endif

	return;
} /* OlQueryGeometry */

/**
 ** OlSetMinHints()
 **/

void
#if	OlNeedFunctionPrototypes
OlSetMinHints (
	Widget			w
)
#else
OlSetMinHints (w)
	Widget			w;
#endif
{
	Widget			parent = XtParent(w);

	XtWidgetGeometry	try;
	XtWidgetGeometry	min;

	if (XtIsWMShell(parent)) {
		try.request_mode = (CWWidth|CWHeight);
		try.width = try.height = 0;
		OlQueryChildGeometry (w, &try, &min);
		XtVaSetValues (
			parent,
			XtNminWidth,  (XtArgVal)min.width,
			XtNminHeight, (XtArgVal)min.height,
			(String)0
		);
	}
	return;
} /* OlSetMinHints */

/**
 ** OlClearMinHints()
 **/

void
#if	OlNeedFunctionPrototypes
OlClearMinHints (
	Widget			w
)
#else
OlClearMinHints (w)
	Widget			w;
#endif
{
	Widget			parent = XtParent(w);

	if (XtIsWMShell(parent))
		XtVaSetValues (
			XtParent(w),
			XtNminWidth,  (XtArgVal)0,
			XtNminHeight, (XtArgVal)0,
			(String)0
		);
	return;
} /* OlClearMinHints */

/**
 ** OlIsLayoutActive()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlIsLayoutActive (
	Widget			w
)
#else
OlIsLayoutActive (w)
	Widget			w;
#endif
{
	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;

	GetLayoutExtensionPointers (w, &E, &e);
	return (E? (e->layout_flags & _OlLayoutActive) != 0 : False);
} /* OlIsLayoutActive */

/**
 ** OlQueryAlignment()
 **/

XtGeometryResult
#if	OlNeedFunctionPrototypes
OlQueryAlignment (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
OlQueryAlignment (w, request, response)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	Boolean			result = XtGeometryNo;

	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;

	GetLayoutExtensionPointers (w, &E, &e);
	if (E && E->query_alignment) {
		response->request_mode = 0;
		result = (*E->query_alignment)(w, request, response);
#if	defined(GEOMETRY_DEBUG)
		if (geometry_debug) {
			printf (
				"OlQueryAlignment: %s/%s/%x",
				XtName(w), CLASS(XtClass(w)), w
			);
			Debug__DumpGeometry (request);
			if (result == XtGeometryYes) {
				printf (" ->");
				Debug__DumpGeometry (response);
			}
			printf ("\n");
		}
#endif
	}

	return (result);
} /* OlQueryAlignment */

/**
 ** ClassPartInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
ClassPartInitialize (
	WidgetClass		wc
)
#else
ClassPartInitialize (wc)
	WidgetClass		wc;
#endif
{
	Boolean			 inherited;

	LayoutCoreClassExtension E;


	if (save.class_part_initialize)
		(*save.class_part_initialize)(wc);

	/*
	 * If the extension was inherited from a superclass, don't
	 * initialize the extension (it's already been initialized!)
	 */
	E = GetLayoutExtension_QInherited(wc, &inherited);
	if (E && !inherited)
		INIT (&E->widgets);

	return;
} /* ClassPartInitialize */

/**
 ** Initialize()
 **/

static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
#else
Initialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	LayoutCoreClassExtension E = GetLayoutExtension(XtClass(new));
	LayoutWidgetExtensionRec e;


	if (save.initialize)
		(*save.initialize)(request, new, args, num_args);
	if (E) {
		e.self = new;
		e.layout_flags = 0;
		_OlArrayOrderedInsert (&E->widgets, e);
	}

	return;
} /* Initialize */

/**
 ** Destroy()
 **/

static void
#if	OlNeedFunctionPrototypes
Destroy (
	Widget			w
)
#else
Destroy (w)
	Widget			w;
#endif
{
	LayoutCoreClassExtension E = GetLayoutExtension(XtClass(w));

	if (save.destroy)
		(*save.destroy)(w);
	if (E) {
		/*
		 * Careful! _OlArrayDelete is a macro that evaluates its
		 * arguments more than once.
		 */
		int i = FindWidget(E, w);
		_OlArrayDelete (&E->widgets, i);
		if (!_OlArraySize(&E->widgets)) {
			_OlArrayFree (&E->widgets);
			INIT (&E->widgets);
		}
	}
	return;
} /* Destroy */

/**
 ** QueryGeometry()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryGeometry (
	Widget			w,
	XtWidgetGeometry *	_request,
	XtWidgetGeometry *	preferred,
	Dimension *		border_width
)
#else
QueryGeometry (w, _request, preferred, border_width)
	Widget			w;
	XtWidgetGeometry *	_request;
	XtWidgetGeometry *	preferred;
	Dimension *		border_width;
#endif
{
	LayoutCoreClassExtension E = GetLayoutExtension(XtClass(w));

	XtGeometryResult	result	= XtGeometryYes;

	XtWidgetGeometry	request;


	/*
	 * If this widget's class doesn't have a layout method, "inherit"
	 * the correct superclass' query_geometry method.
	 * Note: It is assumed that only Vendor, Manager, Primitive, and
	 * EventObj use _OlDefaultQueryGeometry.
	 */
	if (!E || !E->layout) {
		XtGeometryHandler query_geometry
			= CORE_C(SUPER_C(_OlClass(w))).query_geometry;

		if (query_geometry)
			return ((*query_geometry)(w, _request, preferred));
		else
			/*
			 * Many old OLIT composites have no query_geometry
			 * method, or more precisely, inherited a null
			 * query_geometry. Since they will now be
			 * inheriting a non-null query_geometry, but may
			 * not have a layout method, we need to duplicate
			 * the old behavior. Xt returns Yes when a widget
			 * has no query_geometry, so we need to return Yes
			 * here.
			 */
			return (XtGeometryYes);
	}

	/*
	 * Copy request, in case the caller is passing the same pointer
	 * for request and preferred.
	 */
	request = *_request;

	*preferred = request;
	preferred->request_mode = CWWidth|CWHeight;
	if (border_width)
		preferred->request_mode |= CWBorderWidth;

	/*
	 * For our convenience, make all size fields in request valid.
	 */
	if (!(request.request_mode & CWWidth))
		request.width = CORE_P(w).width;
	if (!(request.request_mode & CWHeight))
		request.height = CORE_P(w).height;
	if (!(request.request_mode & CWBorderWidth))
		if (border_width)
			request.border_width = *border_width;
		else
			request.border_width = CORE_P(w).border_width;

	/*
	 * Try a layout with this size; if it's not what we'd rather
	 * have, return an almost. Also, if the caller didn't ask about
	 * something we care about, return an almost.
	 */

	OlLayoutWidget (w, True, True, True, w, &request, preferred);
	if (border_width)
		preferred->border_width = *border_width;

#define CHECK(BIT,F) \
	if (!(request.request_mode & BIT) || request.F != preferred->F)\
		result = XtGeometryAlmost

	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
	if (border_width)
		CHECK (CWBorderWidth, border_width);
#undef	CHECK

	/*
	 * If the best we can do is our current size, returning anything
	 * but XtGeometryNo would be a waste of time.
	 */
#define SAME(F)	(preferred->F == CORE_P(w).F)
	if (
		result == XtGeometryAlmost
	     && SAME(width) && SAME(height)
	     && (!border_width || SAME(border_width))
	)
		result = XtGeometryNo;
#undef	SAME

	return (result);
} /* QueryGeometry */

/**
 ** GetLayoutExtensionPointers()
 **/

static void
#if	OlNeedFunctionPrototypes
GetLayoutExtensionPointers (
	Widget			w,
	LayoutCoreClassExtension * E,
	LayoutWidgetExtension *	e
)
#else
GetLayoutExtensionPointers (w, E, e)
	Widget			w;
	LayoutCoreClassExtension * E;
	LayoutWidgetExtension *	e;
#endif
{
	*E = GetLayoutExtension(XtClass(w));
	*e = (*E? GetLayoutWidgetExtension(*E, w) : 0);
	return;
} /* GetLayoutExtensionPointers */

/**
 ** GetLayoutExtension_QInherited()
 **/

static LayoutCoreClassExtension
#if	OlNeedFunctionPrototypes
GetLayoutExtension_QInherited (
	WidgetClass		wc,
	Boolean *		inherited
)
#else
GetLayoutExtension_QInherited (wc, inherited)
	WidgetClass		wc;
	Boolean *		inherited;
#endif
{
	LayoutCoreClassExtension E;

	/*
	 * WARNING: Shortly we may be recursively call this routine--don't
	 * pass inherited again!
	 */
#define GET_EXTENSION(WC) \
	GetLayoutExtension_QInherited((WC), (Boolean *)0)


	/*
	 * Start off assuming the extension will not be inherited.
	 */
	if (inherited)
		*inherited = False;

	E = (LayoutCoreClassExtension)_OlGetClassExtension(
		(OlClassExtension)CORE_C(wc).extension,
		XtQLayoutCoreClassExtension,
		OlLayoutCoreClassExtensionVersion
	);

	/*
	 * We interpret a missing extension to mean this class wants
	 * to inherit its superclass' extension.
	 */
	if (!E && SUPER_C(wc)) {
		/*
		 * This is the Major Inheritance.
		 */
		if (inherited)
			*inherited = True;
		E = GET_EXTENSION(SUPER_C(wc));
	}

	if (
		E && SUPER_C(wc)
	     && (E->layout == XtInheritLayout
	      || E->query_alignment == XtInheritQueryAlignment)
	) {
		/*
		 * This is the Minor Inheritance.
		 */
		LayoutCoreClassExtension sE = GET_EXTENSION(SUPER_C(wc));
		if (sE) {
			if (E->layout == XtInheritLayout)
				E->layout = sE->layout;
			if (E->query_alignment == XtInheritQueryAlignment)
				E->query_alignment = sE->query_alignment;
		}
	}

	if (E) {
		if (E->layout == XtInheritLayout)
			E->layout = 0;
		if (E->query_alignment == XtInheritQueryAlignment)
			E->query_alignment = 0;
	}

#undef	GET_EXTENSION
	return (E);
} /* GetLayoutExtension_QInherited */

/**
 ** GetLayoutWidgetExtension()
 **/

static LayoutWidgetExtension
#if	OlNeedFunctionPrototypes
GetLayoutWidgetExtension (
	LayoutCoreClassExtension E,
	Widget			w
)
#else
GetLayoutWidgetExtension (E, w)
	LayoutCoreClassExtension E;
	Widget			w;
#endif
{
	int i = FindWidget(E, w);
	return (&_OlArrayElement(&E->widgets, i));
} /* GetLayoutWidgetExtension */

/**
 ** FindWidget()
 **/

static int
#if	OlNeedFunctionPrototypes
FindWidget (
	LayoutCoreClassExtension E,
	Widget			w
)
#else
FindWidget (E, w)
	LayoutCoreClassExtension E;
	Widget			w;
#endif
{
	LayoutWidgetExtensionRec find;
	int			i;

	find.self = w;
	i = _OlArrayFind(&E->widgets, find);
	if (i == _OL_NULL_ARRAY_INDEX)
		OlVaDisplayErrorMsg (
			XtDisplayOfObject(w),
			"internalError", "noLayoutExtension",
			OleCOlToolkitError,
			"Widget %s: No Layout Extension for this widget",
			XtName(w)
		);
		/*NOTREACHED*/

	return (i);
} /* FindWidget */

/**
 ** LayoutWidget()
 **/

static void
#if	OlNeedFunctionPrototypes
LayoutWidget (
	Widget			w,
	LayoutCoreClassExtension E,
	LayoutWidgetExtension	e,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
LayoutWidget (w, E, e, resizable, query_only, cached_best_fit_ok_hint, who_asking, request, response)
	Widget			w;
	LayoutCoreClassExtension E;
	LayoutWidgetExtension	e;
	Boolean			resizable;
	Boolean			query_only;
	Boolean			cached_best_fit_ok_hint;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	/*
	 * Prevent subsequent calls to the layout method. Besides concerns
	 * about various subclasses causing a redundant call to layout,
	 * combining XtGeometryYes and XtGeometryDone composites can cause
	 * a redundant call (via resize).
	 */
	if (!CORE_P(w).being_destroyed && !(e->layout_flags & _OlLayoutActive)) {
		e->layout_flags |= _OlLayoutActive;
		(*E->layout) (
			w, resizable, query_only, cached_best_fit_ok_hint,
			who_asking, request, response
		);
		if (!query_only)
			OlConfigureStatusArea (w);
		e->layout_flags &= ~_OlLayoutActive;
	}
	return;
} /* LayoutWidget */

/**
 ** LayoutWidgetExtensionCompare()
 **/

static int
#if	OlNeedFunctionPrototypes
LayoutWidgetExtensionCompare (
	XtPointer		pA,
	XtPointer		pB
)
#else
LayoutWidgetExtensionCompare (pA, pB)
	XtPointer		pA;
	XtPointer		pB;
#endif
{
#define A ((LayoutWidgetExtension)pA)
#define B ((LayoutWidgetExtension)pB)

	return ((int)(A->self - B->self));

#undef	A
#undef	B
} /* LayoutWidgetExtensionCompare */

/**
 ** AugmentGravity()
 **/

static LocalGravity
#if	OlNeedFunctionPrototypes
AugmentGravity (
	int			gravity
)
#else
AugmentGravity (gravity)
	int			gravity;
#endif
{
	switch (gravity) {
	case AllGravity:
	case NorthSouthEastWestGravity:
	default:
		return (_LEFT|_RIGHT|_TOP|_BOTTOM);
	case SouthEastWestGravity:
		return (_LEFT|_RIGHT|_BOTTOM);
	case NorthEastWestGravity:
		return (_LEFT|_RIGHT|_TOP);
	case EastWestGravity:
		return (_LEFT|_RIGHT);
	case NorthSouthWestGravity:
		return (_LEFT|_TOP|_BOTTOM);
	case NorthSouthEastGravity:
		return (_RIGHT|_TOP|_BOTTOM);
	case NorthSouthGravity:
		return (_TOP|_BOTTOM);
	case CenterGravity:
		return (0);
	case NorthGravity:
		return (_TOP);
	case SouthGravity:
		return (_BOTTOM);
	case EastGravity:
		return (_RIGHT);
	case WestGravity:
		return (_LEFT);
	case NorthWestGravity:
		return (_LEFT|_TOP);
	case NorthEastGravity:
		return (_RIGHT|_TOP);
	case SouthWestGravity:
		return (_LEFT|_BOTTOM);
	case SouthEastGravity:
		return (_RIGHT|_BOTTOM);
	}
} /* AugmentGravity */
