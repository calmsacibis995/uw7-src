#if	!defined(NOIDENT)
#ident	"@(#)rubbertile:RubberTile.c	2.4"
#endif

#include "stdlib.h"

#include "X11/IntrinsicP.h"
#include "X11/Shell.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/RubberTilP.h"
#include "Xol/Error.h"

#define ClassName RubberTile
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&rubberTileClassRec)
#define superClass	((WidgetClass)&panesClassRec)
#define className	"RubberTile"

/*
 * Private routines:
 */

static void		ClassInitialize OL_ARGS((
	void
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static void		ConstraintInitialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static Boolean		ConstraintSetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));

/*
 * Private data:
 */

#if	defined(DEBUG)
static RubberTileWidget		rw = 0;
static RubberTileConstraints	rc = 0;
#endif

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(RubberTileRec,F)

    {	/* SGI */
	XtNorientation, XtCOrientation,
	XtROlDefine, sizeof(OlDefine), offset(rubber_tile.orientation),
	XtRImmediate, (XtPointer)OL_VERTICAL
    }

#undef	offset
};

/*
 * Constraint resource list:
 */

static XtResource	constraints[] = {
#define offset(F)	XtOffsetOf(RubberTileConstraintRec, F)

	/*
	 * This resource is from Panes, but we wish to force it to
	 * be read-only and initialized according to the orientation
	 * resource.
	 */
    {	/* G */
	XtNrefPosition, XtCReadOnly,
	XtROlDefine, sizeof(OlDefine), offset(panes.position),
	XtRImmediate, (XtPointer)OL_BOTTOM /* See ConstraintInitialize */
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

RubberTileClassRec	rubberTileClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(RubberTileRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/ (XtWidgetClassProc)   0,
/* class_inited         */                       False,
/* initialize        (D)*/ (XtInitProc)          0,
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
/* tm_table          (I)*/ (String)              0,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
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
/* resources            */                       constraints,
/* num_resources        */                       XtNumber(constraints),
/* constraint_size      */                       sizeof(RubberTileConstraintRec),
/* initialize        (D)*/                       ConstraintInitialize,
/* destroy           (U)*/ (XtWidgetProc)        0,
/* set_values        (D)*/                       ConstraintSetValues,
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
	}
};

WidgetClass		rubberTileWidgetClass = thisClass;

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
	 * For XtNorientation:
	 */
	_OlAddOlDefineType ("vertical",   OL_VERTICAL);
	_OlAddOlDefineType ("horizontal", OL_HORIZONTAL);
	return;
} /* ClassInitialize */

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
	(((RubberTileWidget)new)->F != ((RubberTileWidget)current)->F)


	if (DIFFERENT(rubber_tile.orientation)) {
		Widget			child;
		Cardinal		n;

		/*
		 * Simply changing the .position constraint from bottom to
		 * right or vice versa, then laying out the widget, should
		 * be sufficient. In particular, the node tree won't need
		 * to change--the relationship of the panes is maintained.
		 */
		if (RUBBERTILE_P(new).orientation == OL_VERTICAL)
			FOR_EACH_CHILD (new, child, n)
				PANES_CP(child).position = OL_BOTTOM;
		else
			FOR_EACH_CHILD (new, child, n)
				PANES_CP(child).position = OL_RIGHT;
		do_layout = True;
	}

	OlLayoutWidgetIfLastClass (new, thisClass, do_layout, False);

#undef	DIFFERENT
	return (False);
} /* SetValues */

/**
 ** ConstraintInitialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
ConstraintInitialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
#else
ConstraintInitialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	OlCheckReadOnlyConstraintResources (new, (Widget)0, args, *num_args);
	PANES_CP(new).position
		= RUBBERTILE_P(XtParent(new)).orientation == OL_HORIZONTAL?
						OL_RIGHT : OL_BOTTOM;
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
	ArgList			args,
	Cardinal *		num_args
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
	OlCheckReadOnlyConstraintResources (new, current, args, *num_args);
	OlLayoutWidgetIfLastClass (XtParent(new), thisClass, False, False);
	return (False);
} /* ConstraintSetValues */
