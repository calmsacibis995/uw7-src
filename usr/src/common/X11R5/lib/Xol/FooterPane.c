#ifndef	NOIDENT
#ident	"@(#)panel:FooterPane.c	2.6"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/FooterPanP.h"

#define ClassName FooterPanel
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&footerPanelClassRec)
#define superClass	((WidgetClass)&rubberTileClassRec)
#define className	"FooterPanel"

/*
 * Local routines:
 */

static void		ChangeManaged OL_ARGS((
	Widget			w
));
static void		ConstraintInitialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static Boolean		ConstraintSetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));

/*
 * Constraint resource list:
 */

static XtResource	constraints[] = {
#define offset(F) XtOffsetOf(FooterPanelConstraintRec, F)

    {	/* SGI */
	XtNweight, XtCWeight,
	XtRShort, sizeof(short), offset(panes.weight),
	XtRImmediate, (XtPointer)XtUnspecifiedShort
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

FooterPanelClassRec	footerPanelClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(FooterPanelRec),
/* class_initialize     */ (XtProc)              0,
/* class_part_init   (D)*/ (XtWidgetClassProc)   0,
/* class_inited         */                       False,
/* initialize        (D)*/ (XtInitProc)          0,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* realize           (I)*/                       XtInheritRealize,
/* actions           (U)*/ (XtActionList)        0,
/* num_actions          */ (Cardinal)            0,
/* resources         (D)*/ (XtResourceList)      0,
/* num_resources        */ (Cardinal)            0,
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/ (XtWidgetProc)        0,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/ (XtExposeProc)        0,
/* set_values        (D)*/ (XtSetValuesFunc)     0,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          0,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Composite class
	 */
	{
/* geometry_manager  (I)*/                       XtInheritGeometryManager,
/* change_managed    (I)*/                       ChangeManaged,
/* insert_child      (I)*/                       XtInheritInsertChild,
/* delete_child      (I)*/                       XtInheritDeleteChild,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */                       constraints,
/* num_resources        */                       XtNumber(constraints),
/* constraint_size      */                       sizeof(FooterPanelConstraintRec),
/* initialize           */                       ConstraintInitialize,
/* destroy              */ (XtWidgetProc)        0,
/* set_values           */                       ConstraintSetValues,
/* extension            */ (XtPointer)           0
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
	 * Panes class:
	 */
	{
/* node_size         (I)*/                       XtInheritNodeSize,
/* node_initialize   (D)*/ (OlPanesNodeProc)     0,
/* node_destroy      (U)*/ (OlPanesNodeProc)     0,
/* state_size        (I)*/                       XtInheritPartitionStateSize,
/* partition_initial (I)*/                       XtInheritPartitionInitialize,
/* partition         (I)*/                       XtInheritPartition,
/* partition_accept  (I)*/                       XtInheritPartitionAccept,
/* partition_destroy (I)*/                       XtInheritPartitionDestroy,
/* steal_geometry    (I)*/                       XtInheritStealGeometry,
/* recover_geometry  (I)*/                       XtInheritRecoverGeometry,
/* pane_geometry     (I)*/                       XtInheritPaneGeometry,
/* configure_pane    (I)*/                       XtInheritConfigurePane,
/* accumulate_size   (I)*/                       XtInheritAccumulateSize,
/* extension            */ (XtPointer)           0
	},
	/*
	 * RubberTile class:
	 */
	{
/* extension            */ (XtPointer)           0
	},
	/*
	 * FooterPanel class:
	 */
	{
/* extension            */ (XtPointer)           0
	}
};

WidgetClass		footerPanelWidgetClass = thisClass;

/**
 ** ChangeManaged()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeManaged (
	Widget			w
)
#else
ChangeManaged (w)
	Widget			w;
#endif
{
	Cardinal		n;

	Widget			child;
	Widget			first;
	Widget			last;


	/*
	 * For all managed children that have a default weight resource,
	 * reset the weights. All get a weight of 1, except the last.
	 * Exception: If there is only one managed child, the client is
	 * being silly, but we'll make that child resizable.
	 *
	 * After resetting the weights, envelope our superclass'
	 * change_managed procedure to do the layout.
	 */
	first = 0;
	last = 0;
	FOR_EACH_MANAGED_CHILD (w, child, n) {
		if (!first)
			first = child;
		last = child;
	}
	FOR_EACH_MANAGED_CHILD (w, child, n)
		if (FOOTERPANEL_CP(child).default_weight) {
			/*
			 * We have to carefully inform the Panes class of
			 * the new weight, since it caches the weight in
			 * the node structure.
			 */
#define current (Widget)&object
			ObjectRec		object;
			PanesConstraintRec	constraints;

			CORE_P(current).xrm_name = CORE_P(child).xrm_name;
			CORE_P(current).constraints = (XtPointer)&constraints;
			PANES_CP(current) = PANES_CP(child);
			PANES_CP(child).weight =
				(child != last || child == first? 1 : 0);
			(void)OlPanesChangeConstraints (child, current);
#undef	current
		}
	(*COMPOSITE_C(superClass).change_managed)(w);

	return;
} /* ChangeManaged */

/**
 ** ConstraintInitialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ConstraintInitialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
ConstraintInitialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	/*
	 * Mark this child as taking the default weight, then set
	 * the weight to a likely default. ChangeManaged will correct
	 * the weight as needed.
	 */
	FOOTERPANEL_CP(new).default_weight
			= PANES_CP(new).weight == XtUnspecifiedShort;
	if (FOOTERPANEL_CP(new).default_weight)
		PANES_CP(new).weight = 1;
	return;
} /* ConstraintInitialize */

/**
 ** ConstraintSetValues()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
ConstraintSetValues (
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
ConstraintSetValues (current, request, new, args, num_args)
	Widget			current;
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	if (PANES_CP(new).weight != PANES_CP(current).weight)
		FOOTERPANEL_CP(new).default_weight = False;
	return (False);
} /* ConstraintSetValues */
