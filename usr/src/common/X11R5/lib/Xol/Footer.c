#ifndef	NOIDENT
#ident	"@(#)footer:Footer.c	1.20"
#endif

#include "stdio.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/FooterP.h"

#define ClassName Footer
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&footerClassRec)
#define superClass	((WidgetClass)&primitiveClassRec)
#define className	"Footer"

#define FOOTER_DEBUG
#if	defined(FOOTER_DEBUG)
# define N(X) (X? (*X? X : "(empty)") : "(null)")
static Boolean		footer_debug;
#endif

/*
 * Convenient macros:
 */

#define CopyString(S)     if (S) S = XtNewString(S)
#define FreeString(S)     if (S) XtFree(S)

#define InRegion(R,G) \
   (XRectInRegion(R,(G)->x,(G)->y,(G)->width,(G)->height) != RectangleOut)

#define Width(W,P) OlScreenPointToPixel(OL_HORIZONTAL,(P),XtScreen(W))
#define Height(W,P) OlScreenPointToPixel(OL_VERTICAL,(P),XtScreen(W))

#define Baseline(W,D) \
	_OlMax(Height(W, FOOTER_BASELINE), D + Height(W, FOOTER_MIN_GAP))

#define Round(A,B) ((int)(A + (B)-1) / (int)(B))

#define HaveFoot(W,WHICH) \
	(FOOTER_P(W).WHICH.foot && FOOTER_P(W).WHICH.weight)

	/*
	 * Some typed numbers to keep our pal, the ANSI-C compiler, happy.
	 */
#if	defined(__STDC__)
# define u2	2U
#else
# define u2	2
#endif

/*
 * Local routines:
 */

static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		Destroy OL_ARGS((
	Widget			w
));
static void		ExposeProc OL_ARGS((
	Widget			w,
	XEvent *		xevent,		/*NOTUSED*/
	Region			region
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static XtGeometryResult	QueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
));
static void		ComputeWidgetGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		ComputeFootSize OL_ARGS((
	Widget			w,
	XRectangle *		g,
	OlgTextLbl *		lbl,
	OlDefine		which
));
static void		ComputeFootGeometry OL_ARGS((
	Widget			w,
	XRectangle *		g,
	OlgTextLbl *		lbl,
	OlDefine		which
));
static void		DrawFoot OL_ARGS((
	Widget			w,
	Region			region,
	OlDefine		which
));
static void		ClearFoot OL_ARGS((
	Widget			new,
	Widget			current,
	OlDefine		which
));
static void		GetCrayons OL_ARGS((
	Widget			w
));
static void		FreeCrayons OL_ARGS((
	Widget			w
));
static Dimension	Subtract OL_ARGS((
	Dimension		a,
	Dimension		b
));

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(FooterRec,F)

    /*
     * Override the XtNhighlightThickness resource, to avoid having the
     * geometry change--the Footer can't take focus so no need for it
     * to leave space for a "focus ring".
     */
    {	/* G */
	XtNhighlightThickness, XtCReadOnly,
	XtRDimension, sizeof(Dimension), offset(primitive.highlight_thickness),
	XtRImmediate, (XtPointer)0
    },

    {	/* SGI */
	XtNleftFoot, XtCFoot,
	XtRString, sizeof(String), offset(footer.left.foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNleftWeight, XtCWeight,
	XtRDimension, sizeof(Dimension), offset(footer.left.weight),
	XtRImmediate, (XtPointer)DEFAULT_LEFT_WEIGHT
    },
    {	/* SGI */
	XtNrightFoot, XtCFoot,
	XtRString, sizeof(String), offset(footer.right.foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNrightWeight, XtCWeight,
	XtRDimension, sizeof(Dimension), offset(footer.right.weight),
	XtRImmediate, (XtPointer)DEFAULT_RIGHT_WEIGHT
    },

#undef	offset
};

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

FooterClassRec		footerClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(FooterRec),
/* class_initialize     */ (XtProc)              0,
/* class_part_init   (D)*/ (XtWidgetClassProc)   0,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* realize           (I)*/                       XtInheritRealize,
/* actions           (U)*/ (XtActionList)        0,
/* num_actions          */ (Cardinal)            0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/ (XtWidgetProc)        0, /* See ExposeProc */
/* expose            (I)*/                       ExposeProc,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          0,
/* accept_focus      (I)*/ (XtAcceptFocusProc)   0, /* can't accept */
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       QueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* focus_on_select      */			 True,
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* register_focus    (I)*/                       XtInheritRegisterFocus,
/* activate          (I)*/                       XtInheritActivateFunc,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * Footer class:
	 */
	{
/* extension            */ 0
	}
};

WidgetClass		footerWidgetClass = thisClass;

/**
 ** Initialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
Initialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	OlCheckReadOnlyResources (new, (Widget)0, args, *num_args);

	CopyString (FOOTER_P(new).left.foot);
	CopyString (FOOTER_P(new).right.foot);

	GetCrayons (new);

#if	defined(FOOTER_DEBUG)
	footer_debug = (getenv("FOOTER_DEBUG") != 0);
	if (footer_debug) {
		printf (
			"(Footer)Initialize: %s/%x (\"%s\",\"%s\") CORE(%d,%d)\n",
			XtName(new), new,
			N(FOOTER_P(new).left.foot),
			N(FOOTER_P(new).right.foot),
			CORE_P(new).width, CORE_P(new).height
		);
	}
#endif
	if (!CORE_P(new).width || !CORE_P(new).height) {
		XtWidgetGeometry	g;

		ComputeWidgetGeometry (new, (XtWidgetGeometry *)0, &g);
		if (!CORE_P(new).width)
			CORE_P(new).width = g.width? g.width : 1;
		if (!CORE_P(new).height)
			CORE_P(new).height = g.height? g.height : 1;
#if	defined(FOOTER_DEBUG)
		if (footer_debug) {
			printf (
				"	(%d,%d)\n",
				g.height, g.width
			);
		}
#endif
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
	FreeString (FOOTER_P(w).left.foot);
	FreeString (FOOTER_P(w).right.foot);
	FreeCrayons (w);
	return;
} /* Destroy */

/**
 ** ExposeProc()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ExposeProc (
	Widget			w,
	XEvent *		xevent,		/*NOTUSED*/
	Region			region
)
#else
ExposeProc (w, xevent, region)
	Widget			w;
	XEvent *		xevent;
	Region			region;
#endif
{
	/*
	 * We don't need a resize method, since we rely on the default
	 * bit_gravity of ForgetGravity to always trash the window content
	 * on resize.
	 */

#if	defined(OL_VERSION) && OL_VERSION >= 5
	XtExposeProc		expose = CORE_C(superClass).expose;

	/*
	 * Envelope our superclass' expose method to get the Motif
	 * border, as needed.
	 */
	if (expose)
		(*expose) (w, xevent, region);
#endif

#if	defined(FOOTER_DEBUG)
	if (footer_debug) {
		printf (
			"(Footer)ExposeProc: %s/%x\n",
			XtName(w), w
		);
	}
#endif
	DrawFoot (w, region, OL_LEFT);
	DrawFoot (w, region, OL_RIGHT);

	return;
} /* ExposeProc */

/**
 ** SetValues()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
SetValues (
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
SetValues (current, request, new, args, num_args)
	Widget			current;
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	struct boolean_pair {
		Boolean			left;
		Boolean			right;
	}			redisplay,
				freeit;

	redisplay.left = False;
	redisplay.right = False;
	freeit.left = False;
	freeit.right = False;

#define DIFFERENT(F) \
	(((FooterWidget)new)->F != ((FooterWidget)current)->F)


	OlCheckReadOnlyResources (new, current, args, *num_args);

	if (
		DIFFERENT(core.background_pixel)
	     || DIFFERENT(primitive.font_color)
	     || DIFFERENT(primitive.font->fid)
	     || DIFFERENT(primitive.font_list)
	) {
		FreeCrayons (new);
		GetCrayons (new);
		redisplay.left = True;
		redisplay.right = True;
	}

	if (
		DIFFERENT(footer.left.weight)
	     || DIFFERENT(footer.right.weight)
#if	defined(OL_VERSION) && OL_VERSION >= 5
	     || DIFFERENT(primitive.shadow_thickness)
#endif
	) {
		redisplay.left = True;
		redisplay.right = True;
	}

	if (DIFFERENT(footer.left.foot) || DIFFERENT(footer.right.foot)) {
		/*
		 * If the old and new footer are empty--but not null--then
		 * no need to redisplay the footer because nothing will be
		 * cleared and nothing will be drawn. But do make a "copy"
		 * of the empty string to keep it freeable.
		 */
#if	defined(FOOTER_DEBUG)
		if (footer_debug) {
			printf (
				"(Footer)SetValues: %s/%x (\"%s\",\"%s\") -> (\"%s\",\"%s\")\n",
				XtName(new), new,
				N(FOOTER_P(current).left.foot),
				N(FOOTER_P(current).right.foot),
				N(FOOTER_P(new).left.foot),
				N(FOOTER_P(new).right.foot)
			);
		}
#endif
#define EMPTY(P) ((P) && !*(P))
#define BOTH_EMPTY(FOOT) \
	(EMPTY(FOOTER_P(new).FOOT.foot) && EMPTY(FOOTER_P(current).FOOT.foot))
#define CHECK(FOOT) \
		if (DIFFERENT(footer.FOOT.foot)) {			\
			if (BOTH_EMPTY(FOOT))				\
				/*					\
				 * Restore current, since it's already	\
				 * malloc'd.				\
				 */					\
				FOOTER_P(new).FOOT.foot			\
					= FOOTER_P(current).FOOT.foot;	\
			else {						\
				CopyString (FOOTER_P(new).FOOT.foot);	\
				redisplay.FOOT = True;			\
				/*					\
				 * Delay freeing current until after we	\
				 * clear the area occupied by the old	\
				 * footer.				\
				 */					\
				freeit.FOOT = True;			\
			}						\
		}

		CHECK (left)
		CHECK (right)
#undef	BOTH_EMPTY
#undef	EMPTY
#undef	CHECK
	}

#undef	DIFFERENT
	/*
	 * We play a subtle trick here: We aren't supposed to do any
	 * (re)displaying in this routine, but should just return a
	 * Boolean that indicates whether the Intrinsics should force
	 * a redisplay. But the Intrinsics does this by ``calling the
	 * Xlib XClearArea function on the widget's window.''
	 * This would cause the entire widget to need a redisplay!
	 * Thus, instead of returning True here, we instead call
	 * XClearArea ourselves on just the area of the left or right
	 * footer and return False. The XClearArea done in ClearFoot
	 * will generate an expose event for a smaller region.
	 * However, if both the left and right footers have to be
	 * redisplayed, we just return True and let Xt do the work.
	 */
	if (redisplay.left && !redisplay.right)
		ClearFoot (new, current, OL_LEFT);
	if (!redisplay.left && redisplay.right)
		ClearFoot (new, current, OL_RIGHT);
	if (freeit.left)
		FreeString (FOOTER_P(current).left.foot);
	if (freeit.right)
		FreeString (FOOTER_P(current).right.foot);
	return (redisplay.left && redisplay.right);
} /* SetValues */

/**
 ** QueryGeometry()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryGeometry (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
)
#else
QueryGeometry (w, request, preferred)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	preferred;
#endif
{
	XtGeometryResult	result	= XtGeometryYes;


#if	defined(FOOTER_DEBUG)
	if (footer_debug) {
		printf (
			"(Footer)QueryGeometry: %s/%x\n",
			XtName(w), w
		);
# define P(FLAG,FIELD,SFIELD) \
		if (request->request_mode & FLAG) \
			printf ("  %12s: %d\n", SFIELD, request->FIELD)
		P (CWX, x, "x");
		P (CWY, y, "y");
		P (CWWidth, width, "width");
		P (CWHeight, height, "height");
		P (CWBorderWidth, border_width, "border_width");
# undef	P
	}
#endif

	preferred->request_mode = CWWidth|CWHeight|CWBorderWidth;
	ComputeWidgetGeometry (w, request, preferred);
	preferred->border_width = 0;

#define CHECK(BIT,F) \
	if (!(request->request_mode & BIT) || request->F != preferred->F)\
		result = XtGeometryAlmost

	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
	CHECK (CWBorderWidth, border_width);
#undef	CHECK

	/*
	 * If the best we can do is our current size, returning anything
	 * but XtGeometryNo would be a waste of time.
	 */
#define SAME(F)	(preferred->F == CORE_P(w).F)
	if (
		result == XtGeometryAlmost
	     && SAME(width) && SAME(height) && SAME(border_width)
	)
		result = XtGeometryNo;
#undef	SAME

#if	defined(FOOTER_DEBUG)
	if (footer_debug) {
		printf (
			"(Footer)QueryGeometry: %s/%x returns %s\n",
			XtName(w), w,
			(result == XtGeometryYes?
				  "XtGeometryYes"
				: (result == XtGeometryNo?
					  "XtGeometryNo"
					: "XtGeometryAlmost"))
		);
# define P(FLAG,FIELD,SFIELD) \
		if (request->request_mode & FLAG) \
			printf ("  %12s: %d\n", SFIELD, request->FIELD)
		P (CWX, x, "x");
		P (CWY, y, "y");
		P (CWWidth, width, "width");
		P (CWHeight, height, "height");
		P (CWBorderWidth, border_width, "border_width");
# undef	P
	}
#endif
	return (result);
} /* QueryGeometry */

/**
 ** ComputeWidgetGeometry()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeWidgetGeometry (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
ComputeWidgetGeometry (w, request, response)
	Widget			w;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	OlgTextLbl		lbl;

	XRectangle		l;
	XRectangle		r;

	Dimension		margin;

	short			ascent;
	short			descent;


	/*
	 * If the caller has a suggested width in request, then we will
	 * return that, subject to the minimum needed for the margins
	 * and (possibly) inter-footer spacing. If the caller suggests a
	 * height, it must be large enough for spec. compliance or the
	 * font at hand, whichever is larger.
	 */


	response->width =
	response->height =
#if	defined(OL_VERSION) && OL_VERSION < 5
		0;
#else
		2 * PRIMITIVE_P(w).shadow_thickness;
#endif

	ComputeFootSize (w, &l, &lbl, OL_LEFT);
	ComputeFootSize (w, &r, &lbl, OL_RIGHT);
	margin = Width(w, FOOTER_MARGIN);

	/*
	 * ComputeFootSize ensured that, where a weight is zero, the
	 * corresponding width or height is set to zero. Thus we don't
	 * have to worry about weights of zero below.
	 */
	if (!l.width || !r.width) {
		if (request && request->request_mode & CWWidth)
			response->width	= _OlMax(request->width, u2*margin);
		else {
			if (!l.width)
				response->width += r.width + 2*margin;
			else
				response->width += l.width + 2*margin;
		}
	} else {
		Dimension pad = margin + (Dimension)Width(w, FOOTER_INTERSPACE);
		Dimension total;

		if (request && request->request_mode & CWWidth)
			response->width	= _OlMax(request->width, u2*pad);
		else {
			l.width += pad;
			r.width += pad;
			total = FOOTER_P(w).left.weight + FOOTER_P(w).right.weight;
			response->width += _OlMax(
				Round(total * l.width, FOOTER_P(w).left.weight),
				Round(total * r.width, FOOTER_P(w).right.weight)
			);
		}
	}

	ascent = lbl.font_list?
		lbl.font_list->max_bounds.ascent : lbl.font->ascent;
	descent = lbl.font_list?
		lbl.font_list->max_bounds.descent : lbl.font->descent;
	response->height += _OlMax(
		(int)Height(w, FOOTER_HEIGHT),
		(int)Height(w, FOOTER_MIN_GAP) + ascent + (int)Baseline(w, descent)
	);
	if (request && request->request_mode & CWHeight)
		response->height
			= _OlMax(request->height, response->height);

	return;
} /* ComputeWidgetGeometry */

/**
 ** ComputeFootSize()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeFootSize (
	Widget			w,
	XRectangle *		g,
	OlgTextLbl *		lbl,
	OlDefine		which
)
#else
ComputeFootSize (w, g, lbl, which)
	Widget			w;
	XRectangle *		g;
	OlgTextLbl *		lbl;
	OlDefine		which;
#endif
{
	g->x = g->y = g->width = g->height = 0;

	lbl->mnemonic    = 0;
	lbl->accelerator = 0;
	lbl->flags       = 0;
	lbl->normalGC    = FOOTER_P(w).font_gc;
	lbl->font        = PRIMITIVE_P(w).font;
#if	defined(I18N)
	lbl->font_list   = PRIMITIVE_P(w).font_list;
#endif

	switch (which) {
	case OL_LEFT:
		if (!HaveFoot(w, left))
			return;
		lbl->label = FOOTER_P(w).left.foot;
		lbl->justification = TL_LEFT_JUSTIFY;
		break;

	case OL_RIGHT:
		if (!HaveFoot(w, right))
			return;
		lbl->label = FOOTER_P(w).right.foot;
		lbl->justification = TL_RIGHT_JUSTIFY;
		break;
	}	

	/*
	 * Do the following to keep OlgDrawTextLabel() from barfing
	 * on a null GC.
	 *
	 * MORE: Is this enough? Should the GC be distinct from
	 * "normalGC"?
	 */
	lbl->inverseGC = FOOTER_P(w).font_gc;

	OlgSizeTextLabel (
		XtScreen(w), FOOTER_P(w).attrs, lbl, &g->width, &g->height
	);

	return;
} /* ComputeFootSize */

/**
 ** ComputeFootGeometry()
 **/

static void
#if	OlNeedFunctionPrototypes
ComputeFootGeometry (
	Widget			w,
	XRectangle *		g,
	OlgTextLbl *		lbl,
	OlDefine		which
)
#else
ComputeFootGeometry (w, g, lbl, which)
	Widget			w;
	XRectangle *		g;
	OlgTextLbl *		lbl;
	OlDefine		which;
#endif
{
	Dimension		margin;
	Dimension		total;
	Dimension		left;
	Dimension		width  = CORE_P(w).width;
	Dimension		height = CORE_P(w).height;

	short			ascent;
	short			descent;


#if	defined(OL_VERSION) && OL_VERSION >= 5
	width = Subtract(width, 2 * PRIMITIVE_P(w).shadow_thickness);
	height = Subtract(height, 2 * PRIMITIVE_P(w).shadow_thickness);
#endif

	ComputeFootSize (w, g, lbl, which);
	if (!g->width)
		return;

	margin = Width(w, FOOTER_MARGIN);

	total = FOOTER_P(w).left.weight + FOOTER_P(w).right.weight;
	left = Round(FOOTER_P(w).left.weight * width, total);

	if (which == OL_LEFT) {
		g->x = margin;
		if (HaveFoot(w, right))
			g->width = Subtract(left, margin + (Dimension)Width(w, FOOTER_INTERSPACE));
		else
			g->width = Subtract(width, 2 * margin);
	} else {
		if (HaveFoot(w, left))
			g->x = left + (Dimension)Width(w, FOOTER_INTERSPACE);
		else
			g->x = margin;
		g->width = Subtract(width, g->x + margin);
	}

	/*
	 * Let the y-position go negative--its an arbitrary choice as to
	 * what to clip when the widget isn't tall enough, and we choose
	 * to clip the top.
	 */

	ascent = lbl->font_list?
		lbl->font_list->max_bounds.ascent : lbl->font->ascent;
	descent = lbl->font_list?
		lbl->font_list->max_bounds.descent : lbl->font->descent;

	g->y = (int)height - (int)(ascent + Baseline(w, descent));
	g->height = ascent + descent;

	return;
} /* ComputeFootGeometry */

/**
 ** DrawFoot()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawFoot (
	Widget			w,
	Region			region,
	OlDefine		which
)
#else
DrawFoot (w, region, which)
	Widget			w;
	Region			region;
	OlDefine		which;
#endif
{
	if (XtIsRealized(w)) {
		OlgTextLbl		lbl;
		XRectangle		g;

		ComputeFootGeometry (w, &g, &lbl, which);
		if (InRegion(region, &g))
			OlgDrawTextLabel (
				XtScreen(w), XtWindow(w),
				FOOTER_P(w).attrs,
				g.x, g.y, g.width, g.height,
				&lbl
			);
	}
	return;
} /* DrawFoot */

/**
 ** ClearFoot()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearFoot (
	Widget			new,
	Widget			current,
	OlDefine		which
)
#else
ClearFoot (new, current, which)
	Widget			new;
	Widget			current;
	OlDefine		which;
#endif
{
	if (XtIsRealized(new)) {
		OlgTextLbl		lbl;
		XRectangle		new_r;
		XRectangle		current_r;
		XRectangle		union_r;
#define RightEdge(R) (int)(R.x + R.width)
#define BottomEdge(R) (int)(R.y + R.height)

		ComputeFootGeometry (new, &new_r, &lbl, which);
		ComputeFootGeometry (current, &current_r, &lbl, which);
		union_r.x = _OlMin(new_r.x, current_r.x);
		union_r.y = _OlMin(new_r.y, current_r.y);
		union_r.width = _OlMax(RightEdge(new_r), RightEdge(current_r)) - union_r.x;
		union_r.height = _OlMax(BottomEdge(new_r), BottomEdge(current_r)) - union_r.y;
#if	defined(FOOTER_DEBUG)
		if (footer_debug) {
			printf (
				"ClearFoot(%s/%x, %s): (%d,%d;%d,%d) + (%d,%d;%d,%d) -> (%d,%d;%d,%d)\n",
				XtName(new), new,
				which == OL_LEFT? "LEFT":"RIGHT",
				new_r.x, new_r.y, new_r.width, new_r.height,

				current_r.x, current_r.y, current_r.width, current_r.height,
				union_r.x, union_r.y, union_r.width, union_r.height
			);
		}
#endif
		XClearArea (
			XtDisplay(new), XtWindow(new),
			union_r.x, union_r.y, union_r.width, union_r.height,
			True
		);
#undef	RightEdge
#undef	BottomEdge
	}
	return;
} /* ClearFoot */

/**
 ** GetCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
GetCrayons (
	Widget			w
)
#else
GetCrayons (w)
	Widget			w;
#endif
{
	XGCValues		v;


	v.foreground = PRIMITIVE_P(w).font_color;
	v.font       = PRIMITIVE_P(w).font->fid;
	v.background = CORE_P(w).background_pixel;
	FOOTER_P(w).font_gc = XtGetGC(
		w, GCForeground | GCFont | GCBackground, &v
	);

	FOOTER_P(w).attrs = OlgCreateAttrs(
		XtScreen(w),
		PRIMITIVE_P(w).font_color,
		(OlgBG *)&(CORE_P(w).background_pixel),
		False,
		OL_DEFAULT_POINT_SIZE
	);

	return;
} /* GetCrayons */

/**
 ** FreeCrayons()
 **/

static void
#if	OlNeedFunctionPrototypes
FreeCrayons (
	Widget			w
)
#else
FreeCrayons (w)
	Widget			w;
#endif
{
	if (FOOTER_P(w).font_gc) {
		XtReleaseGC (w, FOOTER_P(w).font_gc);
		FOOTER_P(w).font_gc = 0;
	}
	if (FOOTER_P(w).attrs) {
		OlgDestroyAttrs (FOOTER_P(w).attrs);
		FOOTER_P(w).attrs = 0;
	}
	return;
} /* FreeCrayons */

/**
 ** Subtract()
 **/

static Dimension
#if	OlNeedFunctionPrototypes
Subtract (
	Dimension		a,
	Dimension		b
)
#else
Subtract (a, b)
	Dimension		a;
	Dimension		b;
#endif
{
	int	diff = (int)a - (int)b;
	return (diff < 0? 0 : (Dimension)diff);
} /* Subtract */
