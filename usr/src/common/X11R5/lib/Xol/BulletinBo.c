#ifndef	NOIDENT
#ident	"@(#)bboard:BulletinBo.c	2.12"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/BulletinBP.h"

#define ClassName Bulletin
#include <Xol/NameDefs.h>

/*
 * Local routines:
 */

static void		ClassInitialize OL_ARGS((
	void
));
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Layout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));

/*
 * Resources:
 */

	/*
	 * 0 points is OPEN LOOK default shadow thickness.
	 */
static String		shadow_thickness = "0 points";

	/*
	 * 2 points is the Motif default. See the resource table and
	 * ClassInitialize.
	 */
#define default_motif_shadow_thickness '2'

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(BulletinBoardRec,F)

    /*
     * This resource is from Manager, but we need to change the default
     * when in Motif mode.
     */
    {	/* SGI *
	XtNshadowThickness, XtCShadowThickness,
	XtRDimension, sizeof(Dimension), offset(manager.shadow_thickness),
	XtRString, (XtPointer)shadow_thickness
    },

    {	/* SGI */
	XtNlayoutWidth, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(bulletin.layout.width),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNlayoutHeight, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(bulletin.layout.height),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },

	/*
	 * The following is here for backwards compatibility.
	 */
    {	/* SGI */
	XtNlayout, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(bulletin.old_layout),
	XtRImmediate, (XtPointer)OL_NONE /* See Initialize */
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

static
LayoutCoreClassExtensionRec	layoutCoreClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */ (XrmQuark) 0, /* see ClassInitialize */
/* version              */            OlLayoutCoreClassExtensionVersion,
/* record_size          */            sizeof(LayoutCoreClassExtensionRec),
/* layout            (I)*/            Layout,
/* query_alignment   (I)*/ (XtGeometryHandler)0,
};

BulletinBoardClassRec	bulletinBoardClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &managerClassRec,
/* class_name           */                       "Bulletin",
/* widget_size          */                       sizeof(BulletinBoardRec),
/* class_initialize     */                       ClassInitialize,
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
/* destroy           (U)*/ (XtWidgetProc)        0,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       XtInheritExpose,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          0,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           &layoutCoreClassExtension
	},
	/*
	 * Composite class
	 */
	{
/* geometry_manager  (I)*/                       XtInheritGeometryManager,
/* change_managed    (I)*/                       XtInheritChangeManaged,
/* insert_child      (I)*/                       XtInheritInsertChild,
/* delete_child      (I)*/                       XtInheritDeleteChild,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */ (XtResourceList)      0,
/* num_resources        */ (Cardinal)            0,
/* constraint_size      */ (Cardinal)            0,
/* initialize        (D)*/ (XtInitProc)          0,
/* destroy           (U)*/ (XtWidgetProc)        0,
/* set_values        (D)*/ (XtSetValuesFunc)     0,
/* extension            */ (XtPointer)           0,
	},
	/*
	 * Manager class:
	 */
	{
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* focus_on_select      */			 True,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* activate          (I)*/                       XtInheritActivateFunc,
/* event_procs          */ (OlEventHandlerList)  0,
/* num_event_procs      */ (Cardinal)            0,
/* register_focus    (I)*/                       XtInheritRegisterFocus,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * BulletinBoard class:
	 */
	{
/* extension            */ (XtPointer)           0
	}
};

WidgetClass	bulletinBoardWidgetClass = (WidgetClass)&bulletinBoardClassRec;

/**
 ** ClassInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
ClassInitialize (
	void
)
#else
ClassInitialize ()
#endif
{
	/*
	 * For XtNlayoutWidth and XtNlayoutHeight:
	 */
	_OlAddOlDefineType ("maximize", OL_MAXIMIZE);
	_OlAddOlDefineType ("minimize", OL_MINIMIZE);
	_OlAddOlDefineType ("ignore",   OL_IGNORE);

	layoutCoreClassExtension.record_type = XtQLayoutCoreClassExtension;

	if (OlGetGui() == OL_MOTIF_GUI)
		shadow_thickness[0] = default_motif_shadow_thickness;

	return;
} /* ClassInitialize */

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
	if (BULLETIN_P(new).old_layout != OL_NONE)
		BULLETIN_P(new).layout.width =
		BULLETIN_P(new).layout.height = BULLETIN_P(new).old_layout;

	OlCheckLayoutResources (
		new, &BULLETIN_P(new).layout, (OlLayoutResources *)0
	);

	OlInitializeGeometry (
		new, &BULLETIN_P(new).layout,
		2*MANAGER_P(new).shadow_thickness,
		2*MANAGER_P(new).shadow_thickness
	);

	return;
} /* Initialize */

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
	Boolean			do_layout	= False;

#define DIFFERENT(F) \
	(((BulletinBoardWidget)new)->F != ((BulletinBoardWidget)current)->F)


	if (DIFFERENT(bulletin.old_layout))
		BULLETIN_P(new).layout.width =
		BULLETIN_P(new).layout.height = BULLETIN_P(new).old_layout;

	OlCheckLayoutResources (
		new, &BULLETIN_P(new).layout, &BULLETIN_P(current).layout
	);

	if (
		DIFFERENT(bulletin.layout.width)
	     || DIFFERENT(bulletin.layout.height)
	     || DIFFERENT(manager.shadow_thickness)
	)
		do_layout = True;

	OlLayoutWidgetIfLastClass (
		new, bulletinBoardWidgetClass, do_layout, True
	);

#undef	DIFFERENT
	return (False);
} /* SetValues */

/**
 ** Layout()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Layout (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
Layout (w, resizable, query_only, cached_best_fit_ok_hint, who_asking, request, response)
	Widget			w;
	Boolean			resizable;
	Boolean			query_only;
	Boolean			cached_best_fit_ok_hint;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	Widget			child;

	Cardinal		n;

	XtWidgetGeometry	best_fit;
	XtWidgetGeometry	adjusted;
	XtWidgetGeometry	available;


	/*
	 * The "best fit" exactly contains the extents of the children
	 * plus the border shadow.
	 */
	best_fit.width =
	best_fit.height = 0;
	FOR_EACH_MANAGED_CHILD (w, child, n) {
		Dimension		dim;

#define COMPUTE(POS,DIM) \
		if (who_asking == child)				\
			dim = request->POS + request->DIM		\
			    + 2*request->border_width;			\
		else							\
			dim = CORE_P(child).POS + CORE_P(child).DIM	\
			    + 2*CORE_P(child).border_width;		\
		if (dim > best_fit.DIM)					\
			best_fit.DIM = dim

		COMPUTE (x, width);
		COMPUTE (y, height);
#undef	COMPUTE
	}
	best_fit.width += 2*MANAGER_P(w).shadow_thickness;
	best_fit.height += 2*MANAGER_P(w).shadow_thickness;

	/*
	 * Make it easy on the poor slob who insists on creating
	 * us with no children (or no initial size).
	 */
	if (!best_fit.width)
		best_fit.width = 1;
	if (!best_fit.height)
		best_fit.height = 1;

	/*
	 * If the composite is asking about its preferred size, the
	 * answer depends on the layout attributes and the size requested.
	 * For OL_IGNORE the preferred size is whatever the client asks.
	 * For OL_MINIMIZE the preferred size is the best fit.
	 * For OL_MAXIMIZE the preferred size is what was requested unless
	 * it isn't enough.
	 */
	if (who_asking == w) {
		if (response) {
#define DECISION(DIM) \
			switch (BULLETIN_P(w).layout.DIM) {		\
			case OL_MINIMIZE:				\
				response->DIM = best_fit.DIM;		\
				break;					\
			case OL_MAXIMIZE:				\
				response->DIM = _OlMax(			\
					request->DIM, best_fit.DIM	\
				);					\
				break;					\
			case OL_IGNORE:					\
				if (request->DIM)			\
					response->DIM = request->DIM;	\
				else					\
					response->DIM = best_fit.DIM;	\
				break;					\
			}

			DECISION (width)
			DECISION (height)
#undef	DECISION
		}
		return;
	}

	/*
	 * Adjust the best_fit to account for the XtNlayoutWidth and
	 * XtNlayoutHeight "constraints".
	 */
	OlAdjustGeometry (
		w, &BULLETIN_P(w).layout, &best_fit, &adjusted
	);

	/*
	 * If we can resize, try to resolve any delta between our
	 * current size and the best-fit size by asking our parent
	 * for the new size. If we don't get it all from our parent,
	 * that's life.
	 */
	OlAvailableGeometry (
		w, resizable, query_only, who_asking,
		request, &adjusted, &available
	);

	/*
	 * A querying child is easy to answer.
	 */
	if (who_asking && response)
		*response = *request;

	return;
} /* Layout */
