#if	!defined(NOIDENT)
#ident	"@(#)panes:Panes.c	1.36"
#endif

#include "string.h"
#include "stdlib.h"

#include "X11/IntrinsicP.h"
#include "X11/Shell.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/PanesP.h"
#include "Xol/Error.h"
#include "Xol/array.h"

#define ClassName Panes
#include <Xol/NameDefs.h>

#define thisClass	((WidgetClass)&panesClassRec)
#define superClass	((WidgetClass)&managerClassRec)
#define className	"Panes"

/*
 * Define the following if you want to be able to turn on debugging
 * information in the binary product.
 */
#define	PANES_DEBUG
#if	defined(PANES_DEBUG)
#include "stdio.h"
static Boolean		panes_debug = False;
#endif

/*
 * Private types:
 */

#define _FindByPane		0x01
#define _FindByDecoration	0x02
typedef enum FindType {
	FindByNode             = 0,
	FindByPane             = _FindByPane,
	FindByDecoration       = _FindByDecoration,
	FindByPaneOrDecoration = (_FindByPane|_FindByDecoration)
}			FindType;

typedef struct FindInfo {
	FindType		type;
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
}			FindInfo;

typedef struct AncestorInfo {
	PanesNode *		node;
	PanesNode **		nodes;
	Cardinal		num_nodes;
}			AncestorInfo;

typedef struct EffectSet {
	PanesNode *		node;
	PanesNode *		parent;
	XtWidgetGeometry	geometry;
}			EffectSet;

typedef struct Effect {
	EffectSet		horz;
	EffectSet		vert;
}			Effect;

/*
 * Convenient macros:
 */

#define LeftMargin(W)   MANAGER_P(W).shadow_thickness + PANES_P(W).left_margin
#define RightMargin(W)  MANAGER_P(W).shadow_thickness + PANES_P(W).right_margin
#define TopMargin(W)    MANAGER_P(W).shadow_thickness + PANES_P(W).top_margin
#define BottomMargin(W) MANAGER_P(W).shadow_thickness + PANES_P(W).bottom_margin

#define CheckReference(W,CHILD) \
	OlCheckReference (						\
		CHILD,							\
		&PANES_CP(CHILD).ref_widget, &PANES_CP(CHILD).ref_name,	\
		OlReferenceManagedSibling, XtNameToWidget, W		\
	)

	/*
	 * A likely upper limit on the number of panes and nodes. These
	 * numbers are used to size the stacks used for stack mallocs.
	 * If a number is too small regular malloc is used.
	 */
#define EstimatedMaximumPanes 20
#define EstimatedMaximumNodes 30

	/*
	 * WARNING: While the "index" computed by the NodeToIndex macro
	 * can be used in a number-of-nodes calculation (e.g. see uses
	 * of MemMove in AppendNodes and DeleteNodes), it should not be
	 * used as an array index (nodes[NodeToIndex(w,n)] won't work!)
	 */
#define NodeSize(W) PANES_C(XtClass(W)).node_size
#define NodeToIndex(W,N) \
	((char *)(N) - (char *)PANES_P(W).nodes) / NodeSize(W)
#define IndexToNode(W,I) \
	(PanesNode *)((char *)PANES_P(W).nodes + (I) * NodeSize(W))

#define NodePlusOne(W,N) (PanesNode *)((char *)(N) + NodeSize(W))
#define NodeMinusOne(W,N) (PanesNode *)((char *)(N) - NodeSize(W))
#define CopyNode(W,A,B) MemMove((XtPointer)(A), (XtPointer)(B), 1, NodeSize(W))

	/*
	 * The node tree is constructed such that the geometric relation
	 * between two sibling nodes can be determined by simply comparing
	 * the node pointers.
	 */
#define LeftOrAbove(A,B) ((A) < (B))

#define PositionedLeftOrAbove(P) \
	(PANES_CP(P).position == OL_LEFT || PANES_CP(P).position == OL_TOP)

#define IsHorizontal(N) (PN_TREE(N).orientation == OL_HORIZONTAL)

#define RightEdge(G) ((G).x + (int)(G).width)
#define BottomEdge(G) ((G).y + (int)(G).height)

	/*
	 * The following macros are used in the low-level layout routines.
	 * See AccumulateLeafGeometry, QueryDecorationSpans, and
	 * GetDecorationBreadths.
	 *
	 * In directions where OL_IGNORE is in effect, we don't want to
	 * use request because the affected-nodes hints will be used
	 * to account for a child that wants a new size. EXCEPT: The
	 * calculation of the affected nodes hints needs to use request
	 * to figure the geometry that will result if request is applied,
	 * regardless of OL_IGNORE. The obey_ignores flag tells how to
	 * proceed.
	 */
#define UseRequest(W,DIM,WHOASKING,OBEY) \
	(   W == WHOASKING						\
	 && (PANES_P(XtParent(W)).layout.DIM != OL_IGNORE || !OBEY)   )

	/*
	 * Stolen from Xt/IntrinsicI.h:
	 */
#define XtStackAlloc(size, stack_cache_array) \
	(size <= sizeof(stack_cache_array)?				\
		  (XtPointer)stack_cache_array : XtMalloc((unsigned)size))
#define XtStackFree(pointer, stack_cache_array) \
	(void)((XtPointer)(pointer) != (XtPointer)stack_cache_array?	\
			XtFree((XtPointer)pointer),0 : 0)

#define STREQU(A,B) (strcmp((A),(B)) == 0)

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

static void		ClassInitialize OL_ARGS((
	void
));
static void		ClassPartInitialize OL_ARGS((
	WidgetClass		wc
));
static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		ChangeManaged OL_ARGS((
	Widget			w
));
static void		InsertChild OL_ARGS((
	Widget			w
));
static void		DeleteChild OL_ARGS((
	Widget			w
));
static void		ConstraintInitialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		ConstraintDestroy OL_ARGS((
	Widget			w
));
static Boolean		ConstraintSetValues OL_ARGS((
	Widget			current,
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		ConstraintGetValuesHook OL_ARGS((
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
));
static void		NodeInitialize OL_ARGS((
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
));
static void		NodeDestroy OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent
));
static void		PartitionInitialize OL_ARGS((
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	current,
	XtWidgetGeometry *	new
));
static void		Partition OL_ARGS((
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	partition
));
static void		PartitionAccept OL_ARGS((
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,		/*NOTUSED*/
	XtWidgetGeometry *	partition
));
static void		PaneGeometry OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	XtWidgetGeometry *	geometry
));
static XtGeometryResult	ConfigurePane OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	allocated,
	XtWidgetGeometry *	preferred,
	Boolean			query_only,	/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response	/*NOTUSED*/
));
static void		AccumulateSize OL_ARGS((
	Widget			w,
	Boolean			cached_best_fit_ok_hint,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result
));
static void		UpdateHandles OL_ARGS((
	Widget			w,
	Widget			handles
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
static void		LayoutNodes OL_ARGS((
	Widget			w,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		LayoutNodesWithEffect OL_ARGS((
	Widget			w,
	PanesNode *		parent,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		LayoutNodesWithoutEffect OL_ARGS((
	Widget			w,
	PanesNode *		parent,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		LayoutLeaf OL_ARGS((
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
static void		QueryPaneGeometry OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	allocated
));
static XtGeometryResult	QueryGeometry OL_ARGS((
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred
));
static Boolean		CalculateAffectingGeometry OL_ARGS((
	Widget			w,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	available,
	Effect *		effect
));
static Boolean		CalculateAffectedGeometry OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Effect *		effect,
	Boolean			adjacent,
	XtWidgetGeometry *	result
));
static void		AccumulateTreeGeometry OL_ARGS((
	Widget			w,
	PanesNode **		nodes,
	PanesNode *		parent,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result
));
static void		AccumulateLeafGeometry OL_ARGS((
	Widget			w,
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result,
	XtWidgetGeometry *	pane,
	Boolean			obey_ignores
));
static void		QueryDecorationSpans OL_ARGS((
	PanesNode *		node,
	Dimension *		width,
	Dimension *		height,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Boolean			obey_ignores
));
static void		GetDecorationBreadths OL_ARGS((
	PanesNode *		node,
	Dimension *		left,
	Dimension *		right,
	Dimension *		top,
	Dimension *		bottom,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Boolean			obey_ignores
));
static void		ConfigureDecorations OL_ARGS((
	PanesNode *		node,
	XtWidgetGeometry *	available,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	response
));
#if	defined(USE_ACCUMULATE_WEIGHTS)
static void		AccumulateWeights OL_ARGS((
	Widget			w
));
#endif
static Boolean		AddWeight OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
));
static void		SubtractInterNodeSpace OL_ARGS((
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	geometry
));
static PanesNode *	CreatePane OL_ARGS((
	Widget			w,
	Widget			new
));
static void		CallNodeInitialize OL_ARGS((
	WidgetClass		wc,
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
));
static void		DeletePane OL_ARGS((
	Widget			w,
	PanesNode *		old,
	PanesNode *		parent
));
static void		CallNodeDestroy OL_ARGS((
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
));
static PanesNode *	AppendNodes OL_ARGS((
	Widget			w,
	PanesNode *		node,
	Cardinal		n
));
static void		DeleteNodes OL_ARGS((
	Widget			w,
	PanesNode *		node,
	Cardinal		n
));
static void		MemMove OL_ARGS((
	XtPointer		to,
	XtPointer		from,
	Cardinal		n,
	Cardinal		size
));
static void		ReconstructTree OL_ARGS((
	Widget			w,
	Boolean			destroy
));
static void		DestroyTree OL_ARGS((
	Widget			w
));
static Boolean		DestroyNode OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data	/*NOTUSED*/
));
static Boolean		WalkTree OL_ARGS((
	Widget			w,
	PanesNode **		nodes,
	PanesNode *		parent,
	Cardinal		n,
	OlPanesNodeType		type,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
));
static Boolean		Find OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
));
static void		FindNode OL_ARGS((
	Widget			w,
	Widget			child,
	FindType		type,
	PanesNode **		node,
	PanesNode **		parent
));
static Boolean		GetDirectAncestors OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
));
static void		SkipNode OL_ARGS((
	Widget			w,
	PanesNode **		node
));
static PanesNode *	NodePlusPlus OL_ARGS((
	Widget			w,
	PanesNode **		node
));
static Widget		ResolveReference OL_ARGS((
	Widget			w,
	Widget			child
));
static Cardinal		InsertPosition OL_ARGS((
	Widget			w
));
static void		SortAndAssignDecorations OL_ARGS((
	Widget			w
));
static int		SubOrder OL_ARGS((
	Widget *		w1,
	Widget *		w2
));
static void		FixUpPaneDecorationLists OL_ARGS((
	Widget			w,
	Cardinal		offset
));
static Boolean		FixList OL_ARGS((
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
));
static void		CheckConstraintResources OL_ARGS((
	Widget			new,
	Widget			current,
	ArgList			args,
	Cardinal *		num_args
));
static void		SetDefaultTypeResource OL_ARGS((
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
));
static Dimension	Subtract OL_ARGS((
	Dimension		a,
	Dimension		b
));
static Dimension	Space OL_ARGS((
	Dimension		base,
	Position		space
));

/*
 * Private data:
 */

#if	defined(DEBUG)
static PanesWidget	pw = 0;
static PanesConstraints	pc = 0;

static Cardinal		tab = 0;
# define DESCEND tab++;
# define ASCEND tab--;
#else
# define DESCEND
# define ASCEND
#endif

Cardinal		OlPanesTreeLevel = 0;

/*
 * Resources:
 */

	/*
	 * 0 points is OPEN LOOK default shadow thickness.
	 */
static char		shadow_thickness[] = "0 points";

	/*
	 * 2 points is the Motif default. See the resource table and
	 * ClassInitialize.
	 */
#define default_motif_shadow_thickness '2'

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(PanesRec,F)

    /*
     * This resource is from Core (or RectObj), but we wish to override
     * its default value.
     */
    {	/* G */
	XtNborderWidth, XtCBorderWidth,
	XtRDimension, sizeof(Dimension), offset(core.border_width),
	XtRImmediate, (XtPointer)0
    },

    /*
     * This resource is from Composite, but we wish to map it to a
     * different field. See Initialize and InsertPosition.
     */
    {
	XtNinsertPosition, XtCInsertPosition,
	XtRFunction, sizeof(XtOrderProc), offset(panes.insert_position),
	XtRImmediate, (XtPointer)0
    },

    /*
     * This resource is from Manager, but we need to change the default
     * when in Motif mode.
     */
    {	/* SGI */
	XtNshadowThickness, XtCShadowThickness,
	XtRDimension, sizeof(Dimension), offset(manager.shadow_thickness),
	XtRString, (XtPointer)shadow_thickness
    },

    /*
     * New resources:
     */
    {	/* SGI */
	XtNlayoutWidth, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(panes.layout.width),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNlayoutHeight, XtCLayout,
	XtROlDefine, sizeof(OlDefine), offset(panes.layout.height),
	XtRImmediate, (XtPointer)OL_MINIMIZE
    },
    {	/* SGI */
	XtNqueryGeometry, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(panes.query_geometry),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNleftMargin, XtCMargin,
	XtRDimension, sizeof(Dimension), offset(panes.left_margin),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNrightMargin, XtCMargin,
	XtRDimension, sizeof(Dimension), offset(panes.right_margin),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNtopMargin, XtCMargin,
	XtRDimension, sizeof(Dimension), offset(panes.top_margin),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNbottomMargin, XtCMargin,
	XtRDimension, sizeof(Dimension), offset(panes.bottom_margin),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNsetMinHints, XtCSetMinHints,
	XtRBoolean, sizeof(Boolean), offset(panes.set_min_hints),
	XtRImmediate, (XtPointer)True
    },

    /*
     * WARNING: We guarantee that the list given in this resource won't be
     * accessed except during the construction of the node tree.
     * (Note that this occurs in change_managed.)
     */
    {	/* SGI */
	XtNpaneOrder, XtCPaneOrder,
	XtRWidgetList, sizeof(Widget *), offset(panes.pane_order),
	XtRImmediate, (XtPointer)0
    },

#undef	offset
};

/*
 * Constraint resource list:
 */

static XtResource	constraints[] = {
#define offset(F)	XtOffsetOf(PanesConstraintRec, F)

    {	/* SGI */
	XtNrefName, XtCRefName,
	XtRString, sizeof(String), offset(panes.ref_name),
	XtRString, (XtPointer)0
    },
    {	/* SGI */
	XtNrefWidget, XtCRefWidget,
	XtRWidget, sizeof(Widget), offset(panes.ref_widget),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNrefPosition, XtCRefPosition,
	XtROlDefine, sizeof(OlDefine), offset(panes.position),
	XtRImmediate, (XtPointer)OL_BOTTOM
    },
    {	/* SGI */
	XtNrefSpace, XtCRefSpace,
	XtRPosition, sizeof(Position), offset(panes.space),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNpaneGravity, XtCGravity,
	XtRGravity, sizeof(int), offset(panes.gravity),
	XtRImmediate, (XtPointer)AllGravity
    },
    {	/* SGI */
	XtNpaneType, XtCPaneType,
	XtROlDefine, sizeof(OlDefine), offset(panes.type),
	XtRCallProc, (XtPointer)SetDefaultTypeResource
    },
    {	/* SGI */
	XtNweight, XtCWeight,
	XtRShort, sizeof(short), offset(panes.weight),
	XtRImmediate, (XtPointer)1
    },

#undef	offset
};

/*
 * Node resource list:
 */

static OLconst XRectangle	junk_rectangle = { 0 };

static XtResource	node_resources[] = {
#define offset(F)	XtOffsetOf(OlPanesNodeLeaf, F)

    {	/* G */
	XtNgeometry, XtCReadOnly,
	"Rectangle", sizeof(XRectangle), offset(geometry),
	"Rectangle", (XtPointer)&junk_rectangle
    },
    {	/* G */
	XtNdecorations, XtCReadOnly,
	XtRWidgetList, sizeof(Widget *), offset(decorations),
	XtRImmediate, (XtPointer)0
    },
    {	/* G */
	XtNnumDecorations, XtCReadOnly,
	XtRCardinal, sizeof(Cardinal), offset(num_decorations),
	XtRImmediate, (XtPointer)0
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
HandlesCoreClassExtensionRec	handlesCoreClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */ (XrmQuark) 0, /* see ClassInitialize */
/* version              */            OlHandlesCoreClassExtensionVersion,
/* record_size          */            sizeof(HandlesCoreClassExtensionRec),
/* update_handles    (I)*/            UpdateHandles
};

static
LayoutCoreClassExtensionRec	layoutCoreClassExtension = {
/* next_extension       */ (XtPointer)&handlesCoreClassExtension,
/* record_type          */ (XrmQuark) 0, /* see ClassInitialize */
/* version              */            OlLayoutCoreClassExtensionVersion,
/* record_size          */            sizeof(LayoutCoreClassExtensionRec),
/* layout            (I)*/            Layout,
/* query_alignment   (I)*/ (XtGeometryHandler)0,
};

static
ConstraintClassExtensionRec	constraintClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */            NULLQUARK,
/* version              */            XtConstraintExtensionVersion,
/* record_size          */            sizeof(ConstraintClassExtensionRec),
/* get_values_hook   (D)*/            ConstraintGetValuesHook
};

PanesClassRec		panesClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(PanesRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/                       ClassPartInitialize,
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
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       XtInheritExpose,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          0,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/ (String)              XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           &layoutCoreClassExtension
	},
	/*
	 * Composite class
	 */
	{
/* geometry_manager  (I)*/                       XtInheritGeometryManager,
/* change_managed    (I)*/                       ChangeManaged,
/* insert_child      (I)*/                       InsertChild,
/* delete_child      (I)*/                       DeleteChild,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */                       constraints,
/* num_resources        */                       XtNumber(constraints),
/* constraint_size      */                       sizeof(PanesConstraintRec),
/* initialize        (D)*/                       ConstraintInitialize,
/* destroy           (U)*/                       ConstraintDestroy,
/* set_values        (D)*/                       ConstraintSetValues,
/* extension            */ (XtPointer)           &constraintClassExtension
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
/* node_size         (I)*/                       sizeof(PanesNode),
/* node_initialize   (D)*/                       NodeInitialize,
/* node_destroy      (U)*/                       NodeDestroy,
/* state_size        (I)*/                       sizeof(PanesPartitionState),
/* partition_initial (I)*/                       PartitionInitialize,
/* partition         (I)*/                       Partition,
/* partition_accept  (I)*/                       PartitionAccept,
/* partition_destroy (I)*/ (OlPanesPartitionDestroyProc)0,
/* steal_geometry    (I)*/ (OlPanesStealProc)    0,
/* recover_geometry  (I)*/ (OlPanesNodeProc)     0,
/* pane_geometry     (I)*/                       PaneGeometry,
/* configure_pane    (I)*/                       ConfigurePane,
/* accumulate_size   (I)*/                       AccumulateSize,
/* extension            */ (XtPointer)           0,
	}                          
};

WidgetClass		panesWidgetClass = thisClass;

/**
 ** OlPanesWalkTree()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesWalkTree (
	Widget			w,
	OlPanesNodeType		type,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
)
#else
OlPanesWalkTree (w, type, walk, func, client_data)
	Widget			w;
	OlPanesNodeType		type;
	OlPanesWalkType		walk;
	OlPanesWalkProc		func;
	XtPointer		client_data;
#endif
{
	/*
	 * Don't pass &PANES_P(w).nodes, as the value will get changed.
	 */
	PanesNode *		node = PANES_P(w).nodes;

	if (node)
		(void)WalkTree (
			w, &node, (PanesNode *)0, 0, type, walk,
			func, client_data
		);
	return;
} /* OlPanesWalkTree */

/**
 ** OlPanesWalkChildren()
 **/

Boolean
#if	OlNeedFunctionPrototypes
OlPanesWalkChildren (
	Widget			w,
	PanesNode *		parent,
	OlPanesNodeType		type,
	OlPanesWalkProc		func,
	XtPointer		client_data
)
#else
OlPanesWalkChildren (w, parent, type, func, client_data)
	Widget			w;
	PanesNode *		parent;
	OlPanesNodeType		type;
	OlPanesWalkProc		func;
	XtPointer		client_data;
#endif
{
	Boolean			ok = True;

#define VISIT(NODE,N) \
	(((PN_ANY(NODE).type == type || type == OlPanesAnyNode) && func)?\
		(*func)(w, NODE, parent, N, PostOrder, client_data) : True)


	if (PN_ANY(parent).type == OlPanesTreeNode) {
		Cardinal		k;
		PanesNode *		nodes = NodePlusOne(w, parent);

		for (k = 0; ok && k < PN_TREE(parent).num_nodes; k++) {
			ok = VISIT(nodes, k);
			SkipNode (w, &nodes);
		}
	}

#undef	VISIT
	return (ok);
} /* OlPanesWalkChildren */

/**
 ** OlPanesFindNode()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesFindNode (
	Widget			w,
	Widget			child,
	PanesNode **		node,
	PanesNode **		parent
)
#else
OlPanesFindNode (w, child, node, parent)
	Widget			w;
	Widget			child;
	PanesNode **		node;
	PanesNode **		parent;
#endif
{
	FindNode (w, child, FindByPane, node, parent);
	return;
} /* OlPanesFindNode */

/**
 ** OlPanesFindSiblings()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesFindSiblings (
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent,
	PanesNode **		fore,
	PanesNode **		aft
)
#else
OlPanesFindSiblings (w, node, parent, fore, aft)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	PanesNode **		fore;
	PanesNode **		aft;
#endif
{
	PanesNode *		nodes;

	Cardinal		n;


	*fore = 0;
	*aft  = 0;

	if (PN_ANY(parent).type != OlPanesTreeNode)
		return;	/* MORE: complain */

	nodes = NodePlusOne(w, parent);
	for (n = 0; nodes != node && n < PN_TREE(parent).num_nodes; n++) {
		/*
		 * This node isn't the one we're looking for. Save it
		 * as the fore-sibling of the next node, then skip it
		 * (and its children, if it is a subtree).
		 */
		*fore = nodes;
		SkipNode (w, &nodes);
	}
	if (n < PN_TREE(parent).num_nodes - 1) {
		/*
		 * Skip over this node to find its aft-sibling.
		 */
		SkipNode (w, &nodes);
		*aft = nodes;
	}

	return;
} /* OlPanesFindSiblings */

/**
 ** OlPanesFindParent()
 **/

extern PanesNode *
#if	OlNeedFunctionPrototypes
OlPanesFindParent (
	Widget			w,
	PanesNode *		node
)
#else
OlPanesFindParent (w, node)
	Widget			w;
	PanesNode *		node;
#endif
{
	FindInfo		info;


	info.type = FindByNode;
	info.node = node;
	info.parent = 0;
	OlPanesWalkTree (w, OlPanesAnyNode, PreOrder, Find, (XtPointer)&info);

	return (info.parent);
} /* OlPanesFindParent */

/**
 ** OlPanesWalkAncestors()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesWalkAncestors (
	Widget			w,
	PanesNode *		node,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
)
#else
OlPanesWalkAncestors (w, node, walk, func, client_data)
	Widget			w;
	PanesNode *		node;
	OlPanesWalkType		walk;
	OlPanesWalkProc		func;
	XtPointer		client_data;
#endif
{
	/*
	 * Don't pass &PANES_P(w).nodes, as the value will get changed.
	 */
	PanesNode *		root = PANES_P(w).nodes;
	PanesNode *		stack[EstimatedMaximumNodes];
	PanesNode **		p;

	AncestorInfo		info;

	int			n;	/* int, to allow < 0 */

	Boolean			ok;


	if (!root)
		return;	/* Huh? *

	/*
	 * To allow PreOrder traversal of the ancestors, we need to know
	 * which branches to walk. We do this by doing a PostOrder walk
	 * of the tree accumulating the list of direct ancestors, then
	 * stepping through the list. Since we have the list, we can also
	 * do the PortOrder walk easily (but we could do the PostOrder
	 * walk without the list).
	 *
	 * See GetDirectAncestors for more details.
	 */

	p = info.nodes = (PanesNode **)XtStackAlloc(
		PANES_P(w).num_nodes * sizeof(PanesNode *), stack
	);

	/*
	 * The following walk will build the list of ancestors from
	 * the bottom up. Since the walk itself won't include the
	 * node at hand, we "manually" add it to the list.
	 */
	info.nodes[0] = info.node = node;
	info.num_nodes = 1;
	(void)WalkTree (
		w, &root, (PanesNode *)0, 0, OlPanesAnyNode, PostOrder,
		GetDirectAncestors, (XtPointer)&info
	);

	/*
	 * A forward step through the list gives us the PostOrder walk.
	 * Conversely, a backwards step through the list gives us the
	 * PreOrder walk.
	 */
	if (walk & PreOrder) {
		ok = True;
		for (p = info.nodes, n = info.num_nodes-1; n >= 0 && ok; n--)
			ok = (*func)(
				w, p[n], n? p[n-1] : (PanesNode *)0, n,
				PreOrder, client_data
			);
	}
	if (walk & PostOrder) {
		ok = True;
		for (p = info.nodes, n = 0; n < info.num_nodes && ok; n++)
			ok = (*func)(
				w, p[n], n? p[n-1] : (PanesNode *)0, n,
				PostOrder, client_data
			);
	}

	XtStackFree (info.nodes, stack);

	return;
} /* OlPanesWalkAncestors */

/**
 ** OlPanesChangeConstraints()
 **/

extern Boolean
#if	OlNeedFunctionPrototypes
OlPanesChangeConstraints (
	Widget			new,
	Widget			current
)
#else
OlPanesChangeConstraints (new, current)
	Widget			new;
	Widget			current;
#endif
{
	Widget			w = XtParent(new);

	Boolean			changed = False;

	PanesNode *		node = 0;
	PanesNode *		parent;


	OlPanesFindNode (w, new, &node, &parent);
	if (!node)
		return (False);

	/*
	 * CAUTION: We advertise to the client of this routine that
	 * it can safely set just the .panes part of current's constraint
	 * record, therefore we can only access that part.
	 */
#define DIFFERENT(F) (PANES_CP(current).F != PANES_CP(new).F)

	/*
	 * If the fundamental constraints (reference and position) have
	 * changed, delete the pane from the node tree, then maybe
	 * recreate it using the new constraints. If the parent is not
	 * realized, we don't have to recreate the node (and then layout
	 * the parent) here; instead, change_managed will do this when the
	 * parent is (re)realized.
	 */
	if (
		DIFFERENT(ref_name)
	     || DIFFERENT(ref_widget)
	     || DIFFERENT(position)
	) {
		DeletePane (w, node, parent);
		node = 0;
		if (XtIsRealized(w)) {
			node = CreatePane(w, new);
			/*
			 * MORE: Ugh!
			 */
			parent = OlPanesFindParent(w, node);
		}
		changed = True;
	}
	if (!node)
		return (changed);

/*
... if .space changed fix up .space in node(s)
 */
	if (DIFFERENT(weight)) {
		/*
		 * Subtract this node's weight from all its ancestors.
		 */
		if (parent)
			OlPanesWalkAncestors (
				w, parent, PostOrder,
				AddWeight, (XtPointer)-PN_ANY(node).weight
			);

		/*
		 * Add the new weight to this node and all its ancestors.
		 */
		PN_ANY(node).weight = 0;
		OlPanesWalkAncestors (
			w, node, PostOrder,
			AddWeight, (XtPointer)PANES_CP(new).weight
		);
	}

#undef	DIFFERENT
	return (changed);
} /* OlPanesChangeConstraints */

/**
 ** OlPanesReconstructTree()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesReconstructTree (
	Widget			w
)
#else
OlPanesReconstructTree (w)
	Widget			w;
#endif
{
	ReconstructTree (w, True);
	return;
} /* OlPanesReconstructTree */

/**
 ** OlPanesQueryGeometry()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesQueryGeometry (
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	allocated
)
#else
OlPanesQueryGeometry (w, node, available, aggregate, allocated)
	Widget			w;
	PanesNode *		node;
	XtWidgetGeometry *	available;
	XtWidgetGeometry *	aggregate;
	XtWidgetGeometry *	allocated;
#endif
{
	QueryPaneGeometry (
		w, node, (Widget)0, (XtWidgetGeometry *)0,
		available, aggregate, (XtWidgetGeometry *)0, allocated
	);
	return;
} /* OlPanesQueryGeometry */

/**
 ** OlPanesLeafGeometry()
 **/

extern void
#if	OlNeedFunctionPrototypes
OlPanesLeafGeometry (
	Widget			w,
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result,
	XtWidgetGeometry *	pane
)
#else
OlPanesLeafGeometry (w, node, who_asking, request, result, pane)
	Widget			w;
	PanesNode *		node;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	result;
	XtWidgetGeometry *	pane;
#endif
{
	AccumulateLeafGeometry (
		w, node, who_asking, request, result, pane,
		True
	);
	return;
} /* OlPanesLeafGeometry */

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

	/*
	 * For XtNrefPosition:
	 */
	_OlAddOlDefineType ("top",    OL_TOP);
	_OlAddOlDefineType ("bottom", OL_BOTTOM);
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("right",  OL_RIGHT);

	/*
	 * For XtNpaneType:
	 */
	_OlAddOlDefineType ("pane",         OL_PANE);
	_OlAddOlDefineType ("handles",      OL_HANDLES);
	_OlAddOlDefineType ("decoration",   OL_DECORATION);
	_OlAddOlDefineType ("constraining", OL_CONSTRAINING_DECORATION);
	_OlAddOlDefineType ("spanning",     OL_SPANNING_DECORATION);

	layoutCoreClassExtension.record_type = XtQLayoutCoreClassExtension;
	handlesCoreClassExtension.record_type = XtQHandlesCoreClassExtension;

	if (OlGetGui() == OL_MOTIF_GUI)
		shadow_thickness[0] = default_motif_shadow_thickness;

#if	defined(PANES_DEBUG)
	panes_debug = (getenv("PANES_DEBUG") != 0);
#endif
	return;
} /* ClassInitialize */

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
#define INHERIT(WC,F,INH) \
	if (PANES_C(WC).F == (INH))					\
		PANES_C(WC).F = PANES_C(SUPER_C(WC)).F

	INHERIT (wc, node_size, XtInheritNodeSize);
	INHERIT (wc, state_size, XtInheritPartitionStateSize);
	INHERIT (wc, partition_initialize, XtInheritPartitionInitialize);
	INHERIT (wc, partition, XtInheritPartition);
	INHERIT (wc, partition_accept, XtInheritPartitionAccept);
	INHERIT (wc, partition_destroy, XtInheritPartitionDestroy);
	INHERIT (wc, steal_geometry, XtInheritStealGeometry);
	INHERIT (wc, recover_geometry, XtInheritRecoverGeometry);
	INHERIT (wc, pane_geometry, XtInheritPaneGeometry);
	INHERIT (wc, configure_pane, XtInheritConfigurePane);
	INHERIT (wc, accumulate_size, XtInheritAccumulateSize);
#undef	INHERIT

#define SAME(method) PANES_C(wc).method == PANES_C(thisClass).method
	if (PANES_C(wc).state_size < PANES_C(thisClass).state_size) {
		if (
			SAME(partition_initialize)
		     || SAME(partition)
		     || SAME(partition_accept)
		) {
			OlVaDisplayErrorMsg (
				toplevelDisplay,
				"illegalStateSize", "classPartInit",
				OleCOlToolkitError,
				"WidgetClass %s: Inherited partition method(s) but set smaller state_size",
				CLASS(wc)
			);
			/*NOTREACHED*/
		}
	}
#undef	SAME

#define NOT_NULL(METHOD) \
	if (!PANES_C(wc).METHOD) {					\
		OlVaDisplayErrorMsg (					\
			toplevelDisplay,				\
			"noMethod", "classPartInit",			\
			OleCOlToolkitError,				\
			"WidgetClass %s: Null %s method",		\
			CLASS(wc), OlQuote(METHOD)			\
		);							\
		/*NOTREACHED*/						\
	}

	NOT_NULL (partition)
	NOT_NULL (pane_geometry)
	NOT_NULL (configure_pane)
	NOT_NULL (accumulate_size)
#undef	NOT_NULL

	return;
} /* ClassPartInitialize */

/**
 ** Initialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,	/*NOTUSED*/
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
	OlPanesNodeLeaf		junk;


	OlCheckLayoutResources (
		new, &PANES_P(new).layout, (OlLayoutResources *)0
	);

	PANES_P(new).nodes = 0;
	PANES_P(new).num_nodes = 0;
	PANES_P(new).num_panes = 0;
	PANES_P(new).num_decorations = 0;

	/*
	 * We want internal children to be in a particular order, thus we
	 * provide an insert_position procedure. We also let the client
	 * provide its version of the procedure. See InsertPosition.
	 */
	COMPOSITE_P(new).insert_position = InsertPosition;

	OlInitializeGeometry (
		new, &PANES_P(new).layout,
		LeftMargin(new) + RightMargin(new),
		TopMargin(new) + BottomMargin(new)
	);

	/*
	 * Cause the sub-resources to be compiled; nothing interesting
	 * is actually fetched at this point.
	 */
	XtGetSubresources (
		new, &junk, "junk", "Junk",
		node_resources, XtNumber(node_resources),
		(Arg *)0, (Cardinal)0
	);

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
	DestroyTree (w);
	return;
} /* Destroy */

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
	ArgList			args,
	Cardinal *		num_args
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
	Boolean			do_layout = False;

#define DIFFERENT(F) \
	(((PanesWidget)new)->F != ((PanesWidget)current)->F)


	OlCheckLayoutResources (
		new, &PANES_P(new).layout, &PANES_P(current).layout
	);

	if (
		DIFFERENT(panes.layout.width)
	     || DIFFERENT(panes.layout.height)
	     || DIFFERENT(panes.left_margin)
	     || DIFFERENT(panes.right_margin)
	     || DIFFERENT(panes.top_margin)
	     || DIFFERENT(panes.bottom_margin)
	)
		do_layout = True;

	if (DIFFERENT(panes.pane_order)) {
		ReconstructTree (new, True);
		do_layout = True;
	}

/*
... Move this to Manager?
 */
	if (DIFFERENT(panes.set_min_hints))
		if (PANES_P(new).set_min_hints) {
			/*
			 * Layout will call OlSetMinHints, so we need not
			 * call it unless we won't be calling Layout.
			 */
			if (!do_layout)
				OlSetMinHints (new);
		} else
			OlClearMinHints (new);

	OlLayoutWidgetIfLastClass (new, thisClass, do_layout, True);

#undef	DIFFERENT
	return (False);
} /* SetValues */

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
	ReconstructTree (w, False);
	(*COMPOSITE_C(superClass).change_managed)(w);
	return;
} /* ChangeManaged */

/**
 ** InsertChild()
 **/

static void
#if	OlNeedFunctionPrototypes
InsertChild (
	Widget			w
)
#else
InsertChild (w)
	Widget			w;
#endif
{
	XtWidgetProc		insert_child
				= COMPOSITE_C(superClass).insert_child;

	Widget			parent = XtParent(w);
	Widget *		old_decorations;

	Cardinal		offset;


	/*
	 * See also InsertPosition.
	 */
	old_decorations = ListOfDecorations(parent);
	if (insert_child)
		(*insert_child) (w);
	if ((offset = old_decorations - ListOfDecorations(parent)))
		FixUpPaneDecorationLists (XtParent(w), offset);

	return;
} /* InsertChild */

/**
 ** DeleteChild()
 **/

static void
#if	OlNeedFunctionPrototypes
DeleteChild (
	Widget			w
)
#else
DeleteChild (w)
	Widget			w;
#endif
{
	XtWidgetProc		delete_child
				= COMPOSITE_C(superClass).delete_child;

	Widget			parent = XtParent(w);
	Widget			ref;
	Widget *		old_decorations;

	Cardinal		offset;


	old_decorations = ListOfDecorations(parent);
	if (delete_child)
		(*delete_child) (w);

	switch (PANES_CP(w).type) {
	case OL_PANE:
		PANES_P(parent).num_panes--;
		break;
	case OL_DECORATION:
	case OL_CONSTRAINING_DECORATION:
	case OL_SPANNING_DECORATION:
		PANES_P(parent).num_decorations--;
		ref = ResolveReference(parent, w);
		if (ref) {
			PanesNode *		node;
			PanesNode *		junk;

			OlPanesFindNode (parent, ref, &node, &junk);
			if (node)
				PN_LEAF(node).num_decorations--;
		}
		break;
	case OL_HANDLES:
	case OL_OTHER:
	default:
		break;
	}

	if ((offset = old_decorations - ListOfDecorations(parent)))
		FixUpPaneDecorationLists (XtParent(w), offset);

	return;
} /* DeleteChild */

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
	/*
	 * WARNING: Subclasses may have additional values for the .type
	 * constraint that we don't know about. The subclasses must
	 * convert these values into a known value in their constraint_
	 * initialize. Therefore, most checks for the .type field are
	 * deferred until insert_child, which is called after all
	 * initializations (actually, see our insert_position routine.)
	 */

 	CheckConstraintResources (new, (Widget)0, args, num_args);
	if (PANES_CP(new).type == OL_HANDLES)
		return;

	if (PANES_CP(new).ref_name)
		PANES_CP(new).ref_name
			= XtNewString(PANES_CP(new).ref_name);
	CheckReference (XtParent(new), new);

	return;
} /* ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
#if	OlNeedFunctionPrototypes
ConstraintDestroy (
	Widget			w
)
#else
ConstraintDestroy (w)
	Widget			w;
#endif
{
	if (PANES_CP(w).ref_name)
		XtFree (PANES_CP(w).ref_name);
	return;
} /* ConstraintDestroy */

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
	Widget			w = XtParent(new);

	Boolean			do_layout = False;


 	CheckConstraintResources (new, current, args, num_args);
	if (PANES_CP(new).type == OL_HANDLES)
		return (False);

#define DIFFERENT(F) \
	( (*((PanesConstraintRec **)&(new->core.constraints)))->F !=	\
	  (*((PanesConstraintRec **)&(current->core.constraints)))->F )

	/*
	 * MORE: Allow the .type constraint to be changeable. Need to
	 * resort the panes list, then call ReconstructTree.
	 * HOWEVER: If this is done, then we have a problem with the
	 * pane_order resource, which we guarantee won't be accessed
	 * except when the tree is reconstructed.
	 */
	if (DIFFERENT(panes.type)) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"initAndGetOnly", "setValues",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource may not be set, only initialized",
			XtName(new), XtNpaneType
		);
		PANES_CP(new).type = PANES_CP(current).type;
	}

	/*
	 * If the reference widget or name has changed, clear the
	 * other field in order to get the proper referencing.
	 * For names, the string space will be freed out of "current".
	 *
	 * Note: If both the reference widget and the reference name
	 * have changed, leave both alone so that mismatches can
	 * be caught.
	 */
	if (DIFFERENT(panes.ref_widget) && DIFFERENT(panes.ref_name))
		/*EMPTY*/;
	else if (DIFFERENT(panes.ref_widget))
		PANES_CP(new).ref_name = 0;
	else if (DIFFERENT(panes.ref_name))
		PANES_CP(new).ref_widget = 0;
	if (DIFFERENT(panes.ref_name)) {
		if (PANES_CP(current).ref_name)
			XtFree (PANES_CP(current).ref_name);
		if (PANES_CP(new).ref_name)
			PANES_CP(new).ref_name
			    = XtNewString(PANES_CP(new).ref_name);
	}
	if (DIFFERENT(panes.ref_widget) || DIFFERENT(panes.ref_name))
		CheckReference (w, new);

	/*
	 * A change in the space constraint for a decoration is handled
	 * just by laying out again. A change in the space constraint for
	 * a pane is another matter (see OlPanesChangeConstraints).
	 */
	if (DIFFERENT(panes.space) && PANES_CP(new).type != OL_PANE)
		do_layout = True;

	/*
	 * Changes in some constraints are of concern only for panes that
	 * are managed; being a managed pane means a node exists.
	 */
	if (PANES_CP(new).type == OL_PANE) {
		if (
			OlPanesChangeConstraints(new, current)
		     || DIFFERENT(panes.weight)
		     || DIFFERENT(panes.gravity)
		)
			do_layout = True;
	}

	OlLayoutWidgetIfLastClass (w, thisClass, do_layout, False);

#undef	DIFFERENT
	return (False);
} /* ConstraintSetValues */

/**
 ** ConstraintGetValuesHook()
 **/

static void
#if	OlNeedFunctionPrototypes
ConstraintGetValuesHook (
	Widget			w,
	ArgList			args,
	Cardinal *		num_args
)
#else
ConstraintGetValuesHook (w, args, num_args)
	Widget			w;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	PanesNode *		node;
	PanesNode *		junk;

	/*
	 * Fetch information that is kept only in the PanesNode structure.
	 */
	OlPanesFindNode (XtParent(w), w, &node, &junk);
	if (node)
		XtGetSubvalues (
			(XtPointer)&PN_LEAF(node),
			node_resources, XtNumber(node_resources),
			args, *num_args
		);

	return;
} /* ConstraintGetValuesHook() */

/**
 ** NodeInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
NodeInitialize (
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
)
#else
NodeInitialize (w, node, parent)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
#endif
{
	if (PN_ANY(node).type == OlPanesLeafNode) {
		Widget			pane = PN_LEAF(node).w;
		PanesNode *		fore = 0;
		PanesNode *		aft  = 0;


		PN_LEAF(node).decorations = 0;
		PN_LEAF(node).num_decorations = 0;

		/*
		 * The space value is "copied" from the constraint record:
		 * To make layout easier, we store the space value in the
		 * node structure such that it is always relative to the
		 * node above or to the left. Branch nodes inherit their
		 * space from their left-most/top-most child.
		 *
		 * (1) If this node is positioned left-of or above its
		 * referenced node, add the space value to the referenced
		 * node. Otherwise, set the space value in this node's
		 * structure.
		 *
		 * (2) If this node has a sibling to the left or above,
		 * add its space value to this node. If there is also a
		 * a sibling right-of or below this node, subtract from it
		 * this space value.
		 *
		 * (3) If this node is now the left-most or top-most node
		 * in this branch, replace the parent's space value with
		 * this node's space value.
		 */
		if (parent)
			OlPanesFindSiblings (w, node, parent, &fore, &aft);
		if (PositionedLeftOrAbove(PN_LEAF(node).w)) {
			if (aft)
				PN_ANY(aft).space += PANES_CP(pane).space;
			PN_ANY(node).space = 0;
		} else
			PN_ANY(node).space = PANES_CP(pane).space;
		if (fore && PositionedLeftOrAbove(PN_LEAF(fore).w)) {
			Position space = PANES_CP(PN_LEAF(fore).w).space;
			PN_ANY(node).space += space;
			if (aft)
				PN_ANY(aft).space -= space;
		}
		if (parent && !fore)
			PN_ANY(parent).space = PN_ANY(node).space;

		/*
		 * Add the weight to this node and all its ancestors.
		 */
		PN_ANY(node).weight = 0;
		OlPanesWalkAncestors (
			w, node, PostOrder,
			AddWeight, (XtPointer)PANES_CP(pane).weight
		);
	} else {
		PanesNode *		child = NodePlusOne(w, node);

		/*
		 * A parent node inherits the inter-node space value
		 * from the child node that was split to make this new
		 * subtree. This routine is called while the split-from
		 * node is the only child of this new parent. Thus we
		 * won't get the wrong space from a new node that is
		 * positioned left-of/above the old node.
		 */
		PN_ANY(node).space = PN_ANY(child).space;

		/*
		 * A branch node has a weight equal to the sum of the
		 * weights of its children.
		 */
		PN_ANY(node).weight = PN_ANY(child).weight;
	}
	return;
} /* NodeInitialize */

/**
 ** NodeDestroy()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
NodeDestroy (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent
)
#else
NodeDestroy (w, node, parent)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
#endif
{
	/*
	 * Reduce the parent's weight and perhaps update the space
	 * for a sibling. Neither of these need be done if the node has
	 * no parent (and thus no siblings).
	 */
	if (PN_ANY(node).type == OlPanesLeafNode && parent) {
		PanesNode *		fore;
		PanesNode *		aft;

		/*
		 * This is mostly the reverse what we did in
		 * NodeInitialize (see above).
		 */
		OlPanesFindSiblings (w, node, parent, &fore, &aft);
		if (aft && PositionedLeftOrAbove(PN_LEAF(node).w))
			PN_ANY(aft).space
				-= PANES_CP(PN_LEAF(node).w).space;
		if (fore && PositionedLeftOrAbove(PN_LEAF(fore).w) && aft)
			PN_ANY(aft).space
				+= PANES_CP(PN_LEAF(fore).w).space;
		if (parent && !fore) {
			if (aft)
				PN_ANY(parent).space = PN_ANY(aft).space;
		/*	else
				parent will soon be destroyed	*/
		}

		/*
		 * Subtract this node's weight from all its ancestors.
		 */
		if (parent)
			OlPanesWalkAncestors (
				w, parent, PostOrder,
				AddWeight, (XtPointer)-PN_ANY(node).weight
			);
	}
	return;
} /* NodeDestroy */

/**
 ** PartitionInitialize()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
PartitionInitialize (
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	current,
	XtWidgetGeometry *	new
)
#else
PartitionInitialize (w, state, node, current, new)
	Widget			w;
	XtPointer		state;
	PanesNode *		node;
	XtWidgetGeometry *	current;
	XtWidgetGeometry *	new;
#endif
{
	PanesPartitionState *	S = (PanesPartitionState *)state;

	/*
	 * On resize, or when one pane wants to change size, we will
	 * have a discrepancy between the space available (new) and the
	 * space currently needed (current). This discrepancy can exist
	 * for any sub-tree of nodes. We partition the discrepancy
	 * according to the weights assigned each pane or summed per
	 * branch node.
	 */

	S->available.width = new->width;
	S->available.height = new->height;
	S->weight = PN_ANY(node).weight;
	S->orientation = PN_TREE(node).orientation;
	if (S->orientation == OL_HORIZONTAL)
		S->delta = new->width - current->width;
	else
		S->delta = new->height - current->height;
	S->residual = 0;

	return;
} /* PartitionInitialize */

/**
 ** Partition()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
Partition (
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	partition
)
#else
Partition (w, state, node, partition)
	Widget			w;
	XtPointer		state;
	PanesNode *		node;
	XtWidgetGeometry *	partition;
#endif
{
	PanesPartitionState *	S = (PanesPartitionState *)state;

	int			delta;


	if (S->weight) {
		delta = (S->delta * (int)PN_ANY(node).weight) / S->weight
		      + S->residual;
#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"Partition: %s/%s/%x",
			XtName(w), CLASS(XtClass(w)), w
		);
		if (PN_ANY(node).type == OlPanesLeafNode) {
			Widget z = PN_LEAF(node).w;
			printf (
				" (%s/%s/%x)",
				XtName(z), CLASS(XtClass(z)), z
			);
		}
		printf (
			" %d = (%d * %d) / %d + %d\n",
			delta,
			S->delta, PN_ANY(node).weight, S->weight,
			S->residual
		);
	}
#endif
		S->residual = 0;
	} else {
		/*
		 * For S->weight to be zero every individual weight must
		 * zero. This means no pane should get resized.
		 */
		delta = 0;
	}

	if (S->orientation == OL_HORIZONTAL)
		S->partition = PN_ANY(node).geometry.width + delta;
	else
		S->partition = PN_ANY(node).geometry.height + delta;
	if (S->partition <= 0)
		S->partition = 1;

	if (S->orientation == OL_HORIZONTAL) {
		partition->width = S->partition;
		partition->height = S->available.height;
	} else {
		partition->width = S->available.width;
		partition->height = S->partition;
	}

	return;
} /* Partition */

/**
 ** PartitionAccept()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
PartitionAccept (
	Widget			w,		/*NOTUSED*/
	XtPointer		state,
	PanesNode *		node,		/*NOTUSED*/
	XtWidgetGeometry *	partition
)
#else
PartitionAccept (w, state, node, partition)
	Widget			w;
	XtPointer		state;
	PanesNode *		node;
	XtWidgetGeometry *	partition;
#endif
{
	PanesPartitionState *	S = (PanesPartitionState *)state;

	/*
	 * partition has been updated to reflect what the node actually
	 * resized to. Any difference between this and what we had
	 * calculated for this node is applied to the next node (if any).
	 */
	if (S->orientation == OL_HORIZONTAL)
		S->residual = S->partition - partition->width;
	else
		S->residual = S->partition - partition->height;

	return;
} /* PartitionAccept */

/**
 ** PaneGeometry()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
PaneGeometry (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	XtWidgetGeometry *	geometry
)
#else
PaneGeometry (w, node, geometry)
	Widget			w;
	PanesNode *		node;
	XtWidgetGeometry *	geometry;
#endif
{
	Widget			pane = PN_LEAF(node).w;

	geometry->x = CORE_P(pane).x;
	geometry->y = CORE_P(pane).y;
	geometry->width = CORE_P(pane).width;
	geometry->height = CORE_P(pane).height;
	geometry->border_width = CORE_P(pane).border_width;
	return;
} /* PaneGeometry */

/**
 ** ConfigurePane()
 **/

/*ARGSUSED*/
static XtGeometryResult
#if	OlNeedFunctionPrototypes
ConfigurePane (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	allocated,
	XtWidgetGeometry *	preferred,
	Boolean			query_only,	/*NOTUSED*/
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response	/*NOTUSED*/
)
#else
ConfigurePane (w, node, available, aggregate, allocated, preferred, query_only, who_asking, request, response)
	Widget			w;
	PanesNode *		node;
	XtWidgetGeometry *	available;
	XtWidgetGeometry *	aggregate;
	XtWidgetGeometry *	allocated;
	XtWidgetGeometry *	preferred;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	static WidgetArray	deferred = _OL_ARRAY_INITIAL;

	int			indx;

	Widget			pane = PN_LEAF(node).w;


	/*
	 * If this pane is requesting a desired size, let the decorations
	 * have a shot at adjusting it to fit. We do this by returning
	 * XtGeometryAlmost now, to let the caller check the geometry
	 * against the decorations again. To avoid looping, we keep
	 * a list of children that have already come here with this
	 * request.
	 *
	 * MORE: I think we can rely on the fact that the widget list
	 * will act like a stack (FILO), and instead of searching the
	 * list for the child at hand just compare it to the last widget
	 * in the array.
	 */
	indx = _OlArrayFind(&deferred, who_asking);
	if (indx != _OL_NULL_ARRAY_INDEX)
		_OlArrayDelete (&deferred, indx);
	else if (pane == who_asking) {
		_OlArrayAppend (&deferred, who_asking);
		preferred->width = request->width;
		preferred->height = request->height;
		preferred->border_width = request->border_width;
		return (XtGeometryAlmost);
	}

	if (pane == who_asking)
		preferred->request_mode = CWWidth|CWHeight;
	else
		preferred->request_mode = 0;
	/*
	 * MORE: allocated must limit preferred in a direction that can't
	 * absorb something larger.
	 */
	allocated->request_mode = 0;
	OlResolveGravity (
		pane, PANES_CP(pane).gravity,
		available, aggregate, allocated, preferred,
		(XtWidgetGeometry *)0,
		QueryGeometry
	);

	return (XtGeometryYes);
} /* ConfigurePane */

/**
 ** AccumulateSize()
 **/

static void
#if	OlNeedFunctionPrototypes
AccumulateSize (
	Widget			w,
	Boolean			cached_best_fit_ok_hint,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result
)
#else
AccumulateSize (w, cached_best_fit_ok_hint, query_only, who_asking, request, result)
	Widget			w;
	Boolean			cached_best_fit_ok_hint;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	result;
#endif
{
	/*
	 * Don't pass &PANES_P(w).nodes, as the value will get changed.
	 */
	PanesNode *		root = PANES_P(w).nodes;

	if (cached_best_fit_ok_hint) {
		result->width = PN_ANY(root).geometry.width;
		result->height = PN_ANY(root).geometry.height;
	} else {
		result->width = 0;
		result->height = 0;
		if (root) {
			result->x = LeftMargin(w);
			result->y = TopMargin(w);
			AccumulateTreeGeometry (
				w, &root, (PanesNode *)0,
				query_only, who_asking, request, result
			);
		}
	}

	return;
} /* AccumulateSize */

/**
 ** UpdateHandles()
 **/

static void
#if	OlNeedFunctionPrototypes
UpdateHandles (
	Widget			w,
	Widget			handles
)
#else
UpdateHandles (w, handles)
	Widget			w;
	Widget			handles;
#endif
{
	Widget			stack[EstimatedMaximumPanes];
	Widget			child;
	Widget *		panes;

	Cardinal		n;
	Cardinal		k = 0;


	panes = (Widget *)XtStackAlloc(
		PANES_P(w).num_panes * sizeof(Widget), stack
	);
	FOR_EACH_MANAGED_PANE (w, child, n)
		panes[k++] = child;
	XtVaSetValues (
		handles,
		XtNpanes,    (XtArgVal)panes,
		XtNnumPanes, (XtArgVal)k,
		(String)0
	);

	/*
	 * The Handles widget copies the panes list, so we
	 * can free our copy.
	 */
	XtStackFree (panes, stack);

	return;
} /* UpdateHandles */

/**
 ** Layout()
 **/

static void
#if	OlNeedFunctionPrototypes
Layout (
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_ok_hint,
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
	PanesNode *		root = PANES_P(w).nodes;

	Effect			E;
	Effect *		effect = 0;

	XtWidgetGeometry	adjusted;
	XtWidgetGeometry	best_fit;
	XtWidgetGeometry	available;

	Dimension		left;
	Dimension		right;
	Dimension		top;
	Dimension		bottom;


#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"Layout: %s/%s/%x resizable %d query_only %d",
			XtName(w), CLASS(XtClass(w)), w,
			resizable, query_only
		);
		if (who_asking == w)
			printf (
				" XtQuery (%s%d,%s%d)\n",
				request->request_mode & CWWidth? "":"!",
				request->width,
				request->request_mode & CWHeight? "":"!",
				request->height
			);
		else if (who_asking)
			printf (
				" XtGeometry (%s%d,%s%d)\n",
				request->request_mode & CWWidth? "":"!",
				request->width,
				request->request_mode & CWHeight? "":"!",
				request->height
			);
		else
			printf ("\n");
	}
#endif

	left = LeftMargin(w);
	right = RightMargin(w);
	top = TopMargin(w);
	bottom = BottomMargin(w);

	/*
	 * MORE: We need to hone the interpretation of this flag, since
	 * allowing it to come here True can cause problems (see the
	 * _OlDefaultGeometryManager, where it calls the second time
	 * with True but we didn't cache the first time!)
	 */
	cached_best_fit_ok_hint = False;

	/*
	 * Walk the node tree to determine the best-fit size under the
	 * current conditions.
	 */
	(*PANES_C(XtClass(w)).accumulate_size) (
		w, cached_best_fit_ok_hint, query_only, who_asking, request, &best_fit
	);
#if	defined(USE_ACCUMULATE_WEIGHTS)
	AccumulateWeights (w);
#endif

	/*
	 * Include enough space to draw a surrounding "shadow", and enough
	 * for the margins.
	 */
#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"Layout: %s/%s/%x best_fit (%d,%d)",
			XtName(w), CLASS(XtClass(w)), w,
			best_fit.width, best_fit.height
		);
	}
#endif
	best_fit.width += left + right;
	best_fit.height += top + bottom;
#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			" -> (%d,%d)\n",
			best_fit.width, best_fit.height
		);
	}
#endif

	/*
	 * Make it easy on the poor slob who insists on creating
	 * us with no children (or no initial size).
	 */
	if (!best_fit.width)
		best_fit.width = 1;
	if (!best_fit.height)
		best_fit.height = 1;

	/*
	 * Adjust the best_fit to account for the XtNlayoutWidth and
	 * XtNlayoutHeight "constraints".
	 */
	OlAdjustGeometry (
		w, &PANES_P(w).layout, &best_fit, &adjusted
	);

	/*
	 * If we can resize, try to resolve any delta between our
	 * current size and the best-fit size by asking our parent
	 * for the new size. If we don't get it all from our parent,
	 * we'll grow or shrink our children to fill in any residual
	 * delta.
	 *
	 * If we can't resize, we'll spread the delta among the children.
	 *
	 * MORE: If the reason we thought we had a discrepency in the
	 * first place is because a child wanted to change size, we should
	 * adjust request to give it what we can get from the parent,
	 * without partitioning the difference among all children. What we
	 * have now penalizes the poor siblings.
	 */
	OlAvailableGeometry (
		w, resizable, query_only, who_asking, request,
		&adjusted, &available
	);
#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"Layout: %s/%s/%x adjusted (%d,%d) available (%d,%d)",
			XtName(w), CLASS(XtClass(w)), w,
			adjusted.width, adjusted.height,
			available.width, available.height
		);
	}
#endif
	/*
	 * At this point, available contains the size available for
	 * managing the children. Note that it may not be the same
	 * as the (composite's) core geometry (a query situation).
	 */

	/*
	 * Shift available to account for any surrounding "shadow" and
	 * margins, so that LayoutNodes doesn't have to worry about the
	 * shadow geometry.
	 */
	available.x = left;
	available.y = top;
	available.width = Subtract(available.width, left + right);
	available.height = Subtract(available.height, top + bottom);
#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			" -> (%d,%d)\n",
			available.width, available.height
		);
	}
#endif

	/*
	 * If a pane is "asking" to be resized and our layout constraints
	 * are OL_IGNORE, then we will steal the necessary space from
	 * (or give the extra space to) the adjacent node(s).
	 * CalculateAffectingGeometry will determine the geometry of the
	 * pane's and the pane's parent's node, for use in determining the
	 * geometry of affected sibling nodes.
	 */
	if (
		who_asking && XtParent(who_asking) == w
	     && CalculateAffectingGeometry(w, who_asking, request, &available, &E)
	)
		effect = &E;

	/*
	 * Lay out the nodes in the space available.
	 */
	if (root)
		LayoutNodes (
			w, &root, &available, effect,
			query_only, who_asking, request, response
		);

	/*
	 * If the Panes widget is just asking, return available. It has
	 * been updated by LayoutNodes to reflect the space actually used
	 * by the nodes.
	 */
	if (response && who_asking == w) {
		response->x = CORE_P(w).x;
		response->y = CORE_P(w).y;
		response->width = available.width + left + right;
		response->height = available.height + top + bottom;
		response->border_width = request->border_width;
	}

	/*
	 * If the composite is a direct child of a window-manager
	 * controlled shell, and if we've actually changed something
	 * in the composite's layout, and if the client allows,
	 * tell the window manager our new minimum size.
	 *
	 * MORE: The check for resizable is a hack to let us know if
	 * we've been called from Resize. In that case it is a waste
	 * of time to redo the min-size check since it hasn't changed.
	 */
	if (!query_only && PANES_P(w).set_min_hints && resizable)
		/*
		 * The !query_only check above prevents the endless loop.
		 * OlSetMinHints checks if the parent is a WMShell.
		 */
		OlSetMinHints (w);

	return;
} /* Layout */

/**
 ** LayoutNodes()
 **/

static void
#if	OlNeedFunctionPrototypes
LayoutNodes (
	Widget			w,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
LayoutNodes (w, nodes, available, effect, query_only, who_asking, request, response)
	Widget			w;
	PanesNode **		nodes;
	XtWidgetGeometry *	available;
	Effect *		effect;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	/*
	 * LayoutNodes gets called for every node (branch or leaf),
	 * so a single increment per visit will step us through the
	 * entire tree-array.
	 */
	PanesNode *		this = NodePlusPlus(w, nodes);

	DESCEND


	/*
	 * On entry, available contains the geometry available for this
	 * pane or sub-tree. On exit, available contains the geometry
	 * actually used. The x,y values are dictated downward--the caller
	 * determines the values--while the width,height values are
	 * "dictated" upwards--the caller may suggest the values, but the
	 * actual values depend on the demands of the children. Of course,
	 * if the demands of the children exceed the bounds of the parent,
	 * some children are clipped--so be it.
	 */

	if (PN_ANY(this).type == OlPanesLeafNode)
		LayoutLeaf (
			w, this, available, effect,
			query_only, who_asking, request, response
		);

	else if (
	     effect
	  && (this == effect->horz.parent || this == effect->vert.parent)
	)

		LayoutNodesWithEffect (
			w, this, nodes, available, effect,
			query_only, who_asking, request, response
		);

	else
		LayoutNodesWithoutEffect (
			w, this, nodes, available, effect,
			query_only, who_asking, request, response
		);

	/*
	 * Make the aggregate geometry reflect the actual geometry.
	 */
#if	defined(DONT_IGNORE_QUERY_ONLY)
	if (!query_only) {
#endif
		PN_ANY(this).geometry.x = available->x;
		PN_ANY(this).geometry.y = available->y;
		PN_ANY(this).geometry.width = available->width;
		PN_ANY(this).geometry.height = available->height;
#if	defined(DONT_IGNORE_QUERY_ONLY)
	}
#endif

	ASCEND
	return;
} /* LayoutNodes */

/**
 ** LayoutNodesWithEffect()
 **/

static void
#if	OlNeedFunctionPrototypes
LayoutNodesWithEffect (
	Widget			w,
	PanesNode *		parent,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
LayoutNodesWithEffect (w, parent, nodes, available, effect, query_only, who_asking, request, response)
	Widget			w;
	PanesNode *		parent;
	PanesNode **		nodes;
	XtWidgetGeometry *	available;
	Effect *		effect;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	PanesNode *		node;
	PanesNode *		stack[EstimatedMaximumNodes];
	PanesNode **		list;

	Cardinal		n;
	Cardinal		nfore;

	EffectSet *		E;

	Position *		pos;
	Position		space;

	Dimension *		dim;
	Dimension *		odim = 0;
	Dimension *		available_odim;

	XtWidgetGeometry	new;

	int			debit; /* allow "negative debit" */
	int			try;

	Boolean			reposition;


#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"LayoutNodesWithEffect: %s/%s/%x available (%d,%d)\n",
			XtName(w), CLASS(XtClass(w)), w,
			available->width,
			available->height
		);
	}
#endif

	if (IsHorizontal(parent)) {
		E = &effect->horz;
		new.x = E->geometry.x;
		new.y = available->y;
		pos = &new.x;
		dim = &new.width;
		if (PANES_P(w).layout.height != OL_IGNORE) {
			odim = &new.height;
			available_odim = &available->height;
		}
	} else {
		E = &effect->vert;
		new.x = available->x;
		new.y = E->geometry.y;
		pos = &new.y;
		dim = &new.height;
		if (PANES_P(w).layout.width != OL_IGNORE) {
			odim = &new.width;
			available_odim = &available->width;
		}
	}

	/*
	 * Construct a list of the affecting node's siblings, and keep
	 * track of which siblings come before and which after the
	 * affecting node.
	 */
	list = (PanesNode **)XtStackAlloc(
		PN_TREE(parent).num_nodes * sizeof(PanesNode *), stack
	);
	for (
		node = *nodes,      n = nfore = 0;
		n < PN_TREE(parent).num_nodes;
		SkipNode(w, &node), n++
	) {
		list[n] = node;
		if (LeftOrAbove(node, E->node))
			nfore++;
	}

	/*
	 * Run through the nodes that come before the affecting node, in
	 * reverse order (from closest to farthest). Calculate the new
	 * geometry that is forced by the affecting node, and lay out the
	 * node. If the node needs more space, debit the space from the
	 * node next in line away from the affecting node. If this is the
	 * furthest node, the (possibly accumulated) debit has to be
	 * applied to the affecting node--it can't be resized as much as
	 * it wants--and the intermediate nodes have to be shifted over
	 * to compensate.
	 */

	debit = 0;
	reposition = False;
	space = PN_ANY(E->node).space;
	for (n = nfore; n; n--) {
		node = list[n-1];

		/*
		 * If this node is not affected (either directly, or
		 * indirectly through an accumulated debit), then we can
		 * leave the node, and the remaining nodes on this side,
		 * alone.
		 */
		if (
		    !CalculateAffectedGeometry
			(w, node, parent, effect, n == nfore, &new)
		 && !debit
		)
			break;

		/*
		 * Deduct as much as possible of any existing debit from
		 * the size of the node. If not all the debit can be
		 * imposed on the node's size, the node will have to be
		 * shifted (if possible) later.
		 */
		if (debit) {
			if ((int)*dim >= debit) {
				*dim -= debit;
				debit = 0;
			} else {
				debit -= *dim;
				*dim = 0;
			}
			if (debit)
				reposition = True;
		}

		/*
		 * Position the node adjacent to the "previous" node:
	 	 *  curr-pos = prev-pos - inter-node-space - curr-dim
		 */
		*pos = Space(*pos, -space);
		*pos -= *dim;
		space = PN_ANY(node).space; /* for next pass */

		/*
		 * Lay out the node and see how much space it really
		 * wants. If it wanted more (or maybe even less!) update
		 * the debit.
		 */
		try = *dim;
		LayoutNodes (
			w, &node, &new, effect,
			query_only, who_asking, request, response
		);
		debit += (int)*dim - try;

		/*
		 * If the node took a different size, it will have to be
		 * shifted (if possible) later, to account for the delta.
		 */
		if (*dim != try)
			reposition = True;

		/*
		 * The "other" dimension may be able to float.
		 */
		if (odim)
			if (*odim > *available_odim)
				*available_odim = *odim;
	}

	/*
	 * If some debit remains, apply it to the affecting node; the
	 * "fore" nodes will be shifted over to compensate.
	 */
	if (debit) {
		/*
		 * Note: It is safe to assume that the affecting node can
		 * completely absorb the debit, since the debit arose out
		 * of trying to resize the affecting node from a stable
		 * configuration.
		 */
		if (IsHorizontal(parent)) {
			E->geometry.x += debit;
			E->geometry.width -= debit;
		} else {
			E->geometry.y += debit;
			E->geometry.height -= debit;
		}
		reposition = True;
	}

	/*
	 * Do the shifts that were delayed. (We delay all the shifts until
	 * here to avoid shifting a node more than once.) Start with the
	 * "fore" node closest to the affecting node and work outwards.
#if	defined(DONT_IGNORE_QUERY_ONLY)
	 *
	 * MORE: Can't use PN_ANY(node).geometry here if query_only is
	 * true, since it will not have been updated above.
#endif
	 */
	if (reposition) {
		if (IsHorizontal(parent)) {
			new.x = E->geometry.x;
			new.y = available->y;
		} else {
			new.x = available->x;
			new.y = E->geometry.y;
		}
		space = PN_ANY(E->node).space;
		for (n = nfore; n; ) {
			node = list[--n];
			new.width = PN_ANY(node).geometry.width;
			new.height = PN_ANY(node).geometry.height;
			*pos = Space(*pos, -space);
			*pos -= *dim;
			space = PN_ANY(node).space;
			if (IsHorizontal(parent)) {
				if (new.x == PN_ANY(node).geometry.x)
					break;
			} else {
				if (new.y == PN_ANY(node).geometry.y)
					break;
			}
			LayoutNodes (
				w, &node, &new, effect,
				query_only, who_asking, request, response
			);
		}
	}

	/*
	 * Do the same as above to the siblings on the other side.
	 */

	if (IsHorizontal(parent)) {
		new.x = RightEdge(E->geometry);
		new.y = available->y;
	} else {
		new.x = available->x;
		new.y = BottomEdge(E->geometry);
	}
	debit = 0;
	for (n = nfore+1; n < PN_TREE(parent).num_nodes; n++) {
		node = list[n];

		if (
		    !CalculateAffectedGeometry
			(w, node, parent, effect, n == nfore+1, &new)
		 && !debit
		)
			break;

		/*
		 * Apply as much as possible of any debit to the size of
		 * the node.
		 */
		if (debit)
			if ((int)*dim >= debit) {
				*dim -= debit;
				debit = 0;
			} else {
				debit -= *dim;
				*dim = 0;
			}

		/*
		 * Shift this node away from the previous node.
		 */
		*pos = Space(*pos, PN_ANY(node).space);

		/*
		 * Lay out the node and see how much space it really
		 * wants. If it wanted more (or maybe even less!) update
		 * the debit.
		 */
		try = *dim;
		LayoutNodes (
			w, &node, &new, effect,
			query_only, who_asking, request, response
		);
		debit += (int)*dim - try;

		/*
		 * Update the position for the next node.
		 */
		*pos += *dim;

		/*
		 * The "other" dimension may be able to float.
		 */
		if (odim)
			if (*odim > *available_odim)
				*available_odim = *odim;
	}

	/*
	 * If some debit remains, apply it to the affecting node
	 * and shift the "aft" nodes over.
	 */
	if (debit) {
		if (IsHorizontal(parent)) {
			E->geometry.width -= debit;
			new.x = RightEdge(E->geometry);
			new.y = available->y;
		} else {
			E->geometry.height -= debit;
			new.x = available->x;
			new.y = BottomEdge(E->geometry);
		}
		for (n = nfore+1; n < PN_TREE(parent).num_nodes; n++) {
			node = list[n];
			*pos = Space(*pos, PN_ANY(node).space);
			if (IsHorizontal(parent)) {
				if (new.x == PN_ANY(node).geometry.x)
					break;
			} else {
				if (new.y == PN_ANY(node).geometry.y)
					break;
			}
			new.width = PN_ANY(node).geometry.width;
			new.height = PN_ANY(node).geometry.height;
			LayoutNodes (
				w, &node, &new, effect,
				query_only, who_asking, request, response
			);
			*pos += *dim;
		}
	}

	/*
	 * Now lay out the affecting node.
	 */

#define SET(POS,OPOS,DIM,ODIM) \
	new.POS = E->geometry.POS;					\
	new.OPOS = available->OPOS;					\
	new.DIM = E->geometry.DIM;					\
	new.ODIM = available->ODIM

	if (IsHorizontal(parent)) {
		SET (x, y, width, height);
	} else {
		SET (y, x, height, width);
	}
#undef	SET
	node = E->node;
	LayoutNodes (
		w, &node, &new, effect,
		query_only, who_asking, request, response
	);
	if (odim)
		if (*odim > *available_odim)
			*available_odim = *odim;

	/*
	 * The work above did not move the nodes pointer, because we
	 * did not walk through the nodes in a simple order. Now we
	 * have to update the nodes pointer so that we leave here pointing
	 * to the next node for the caller to deal with.
	 */
	for (n = 0; n < PN_TREE(parent).num_nodes; n++)
		SkipNode (w, nodes);

	XtStackFree (list, stack);

	return;
} /* LayoutNodesWithEffect */

/**
 ** LayoutNodesWithoutEffect()
 **/

static void
#if	OlNeedFunctionPrototypes
LayoutNodesWithoutEffect (
	Widget			w,
	PanesNode *		parent,
	PanesNode **		nodes,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
LayoutNodesWithoutEffect (w, parent, nodes, available, effect, query_only, who_asking, request, response)
	Widget			w;
	PanesNode *		parent;
	PanesNode **		nodes;
	XtWidgetGeometry *	available;
	Effect *		effect;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	OlPanesPartitionInit	partition_initialize
				= PANES_C(XtClass(w)).partition_initialize;
	OlPanesPartitionProc	partition
				= PANES_C(XtClass(w)).partition;
	OlPanesPartitionProc	partition_accept
				= PANES_C(XtClass(w)).partition_accept;
	OlPanesPartitionDestroyProc partition_destroy
				= PANES_C(XtClass(w)).partition_destroy;

	Cardinal		n;

	XtWidgetGeometry	current;
	XtWidgetGeometry	new;

	XtPointer		state = 0;

	PanesPartitionState	stack[1];


#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"LayoutNodesWithoutEffect: %s/%s/%x available (%d,%d)\n",
			XtName(w), CLASS(XtClass(w)), w,
			available->width, available->height
		);
	}
#endif

	/*
	 * The upper-left corner of this tree and its first child start
	 * where requested. We make two uses of new below, first we set
	 * it to the available geometry, excluding inter-node spacing,
	 * for partition_initialize; second, we pass it to partition which
	 * sets it to the new geometry for each node.
	 */
	new.x = available->x;
	new.y = available->y;
	new.width = available->width;
	new.height = available->height;
	SubtractInterNodeSpace (w, parent, &new);

	current.x = PN_ANY(parent).geometry.x;
	current.y = PN_ANY(parent).geometry.y;
	current.width = PN_ANY(parent).geometry.width;
	current.height = PN_ANY(parent).geometry.height;
	SubtractInterNodeSpace (w, parent, &current);

	/*
	 * We zero this tree's orientation-dimension here, so we can sum
	 * it again according to the dimension to which the child really
	 * resizes. We also zero the perpendicular dimension here, so that
	 * we can track the maximum such dimension of the children.
	 */
	available->width = 0;
	available->height = 0;

	/*
	 * Initialize the partition state. Give the partitioning
	 * algorithm the current and new aggregate geometries--minus the
	 * inter-node space. This makes the partitioning fair, in that
	 * it considers only the space actually taken up by the nodes.
	 */
	if (PANES_C(XtClass(w)).state_size)
		state = XtStackAlloc(PANES_C(XtClass(w)).state_size, stack);
	if (partition_initialize)
		(*partition_initialize) (w, state, parent, &current, &new);

	/*
	 * Run through these nodes left-to-right or top-to-bottom,
	 * according to the orientation. Lay out each node, positioning it
	 * at [new.x,new.y]; new.x or new.y is then shifted over by the
	 * size of the node to prepare for the next node.
	 */
	for (n = 0; n < PN_TREE(parent).num_nodes; n++) {
		PanesNode *		node = *nodes;
		Position		space = PN_ANY(node).space;

		if (n != 0)
			if (IsHorizontal(parent))
				new.x = Space(new.x, space);
			else
				new.y = Space(new.y, space);

		(*partition) (w, state, node, &new);
		LayoutNodes (
			w, nodes, &new, effect,
			query_only, who_asking, request, response
		);

#define UPDATE(POS,DIM,PDIM) \
		new.POS += new.DIM;					\
		available->DIM += new.DIM;				\
		if (n != 0)						\
			available->DIM = Space(available->DIM, space);	\
		if (available->PDIM < new.PDIM)				\
			available->PDIM = new.PDIM

		if (IsHorizontal(parent)) {
			UPDATE (x,width, height);
		} else {
			UPDATE (y,height, width);
		}
#undef	UPDATE

		if (partition_accept)
			(*partition_accept) (w, state, node, &new);
	}

	if (partition_destroy)
		(*partition_destroy) (w, state);
	if (state)
		XtStackFree (state, stack);

	return;
} /* LayoutNodesWithoutEffect */

/**
 ** LayoutLeaf()
 **/

static void
#if	OlNeedFunctionPrototypes
LayoutLeaf (
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	available,
	Effect *		effect,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
)
#else
LayoutLeaf (w, node, available, effect, query_only, who_asking, request, response)
	Widget			w;
	PanesNode *		node;
	XtWidgetGeometry *	available;
	Effect *		effect;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	response;
#endif
{
	OlPanesConfigureProc	configure_pane
				= PANES_C(XtClass(w)).configure_pane;

	Widget			child    = PN_LEAF(node).w;

	Dimension		hbw_horz;
	Dimension		hbw_vert;
	Dimension		bw;

	XtWidgetGeometry	aggregate;
	XtWidgetGeometry	allocated;
	XtWidgetGeometry	preferred;
	XtWidgetGeometry *	effecting;

	XtGeometryResult	result;


#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
			"LayoutLeaf: %s/%s/%x %s/%s/%x available (%d,%d)\n",
			XtName(w), CLASS(XtClass(w)), w,
			XtName(child), CLASS(XtClass(child)), child,
			available->width, available->height
		);
	}
#endif

	/*
	 * available contains the space available for the pane plus its
	 * borders and decorations. The decorations are assumed to have
	 * fixed breadths (fixed dimension parallel to their position next
	 * to the pane). The decorations may also constrain the pane's
	 * span, i.e. the decorations may not allow certain sizes in the
	 * direction perpendicular to their position. For example, a
	 * vertical OPEN LOOK scrollbar has a fixed width and certain
	 * disallowed heights.
	 *
	 * Nonetheless, the pane has the final say as to its preferred
	 * size (we'll let the decorations take care of themselves if
	 * they don't like the pane's preferred size, e.g. by centering
	 * themselves).
	 */

	/*
	 * Note: If this pane is causing an "effect" (i.e. is asking for
	 * a geometry change but an OL_IGNORE situation requires resizing
	 * siblings to accomodate the request), then use allocated in
	 * place of request, since it reflects request as constrained by
	 * the siblings' needs. However, it doesn't matter where we get
	 * the border_width, it's all the same.
	 */
	if (
	     effect
	  && (node == effect->horz.node || node == effect->vert.node)
	)
		effecting = &allocated;
	else
		effecting = request;
	bw = (child == who_asking?
		request->border_width : CORE_P(child).border_width);

	/*
	 * The configure_pane method may change the set of decorations
	 * or may have a suggested geometry that it wants to compare
	 * against the decorations, so we need to loop here.
	 */
	available->border_width = bw;	/* for QueryPaneGeometry */
	preferred.request_mode = 0;	/* likewise              */
	do {
		/*
		 * Note: Currently QueryPaneGeometry uses request only
		 * if it applies to a decoration, not for the pane; it
		 * computes the pane from available, instead. If this
		 * changes, you need to worry about "effect" (see above).
		 */
		QueryPaneGeometry (
			w, node, who_asking, request,
			available, &aggregate, &preferred, &allocated
		);
#if	defined(PANES_DEBUG)
		if (panes_debug) {
			printf (
				" after QueryPane: (%d,%d)/(%d,%d) -> (%d,%d)/(%d,%d)\n",
				available->width, available->height,
				aggregate.width, aggregate.height,
				allocated.width, allocated.height,
				preferred.width, preferred.height
			);
		}
#endif

		/*
		 * Let a subclass override our default pane configuration.
		 * It is expected to calculate preferred from allocated,
		 * including the position as well as size. It can also
		 * add or remove decorations (returning XtGeometryAlmost
		 * if it does, so that we know to recalculate things),
		 * and it can actually configure the widget (returning
		 * XtGeometryDone if it does, so that we know not to
		 * configure the widget ourselves). It can also return
		 * a suggested preferred size, again returning Almost
		 * if it does, and setting the appropriate request_mode
		 * as well.
		 *
		 * The configure_pane procedure can also change aggregate
		 * to reflect the amount of space we claim this node
		 * consumes.
		 */
		result = (*configure_pane)(
			w, node,
			available, &aggregate, &allocated, &preferred,
			query_only, who_asking, effecting, response
		);
#if	defined(PANES_DEBUG)
		if (panes_debug) {
			printf (
				" after ConfigurePane: (%d,%d)/(%d,%d) -> (%d,%d)/(%d,%d) %s\n",
				available->width, available->height,
				aggregate.width, aggregate.height,
				allocated.width, allocated.height,
				preferred.width, preferred.height,
				(result == XtGeometryYes?
				      "Yes"
				    : (result == XtGeometryNo? "No" : "Almost"))
			);
		}
#endif

	} while (result == XtGeometryAlmost);

	if (result == XtGeometryYes)
		OlConfigureChild (
			child,
			preferred.x, preferred.y,
			preferred.width, preferred.height,
			bw,
			query_only, who_asking, response
		);

	hbw_horz = OlHandlesBorderThickness(child, OL_HORIZONTAL);
	hbw_vert = OlHandlesBorderThickness(child, OL_VERTICAL);
	preferred.x -= hbw_horz;
	preferred.y -= hbw_vert;
	preferred.width += 2*hbw_horz + 2*bw;
	preferred.height += 2*hbw_vert + 2*bw;
	ConfigureDecorations (
		node, &preferred, query_only, who_asking, response
	);

	/*
	 * Update available with the overall extent of the pane, so that
	 * the caller has the real size, not the requested size.
	 */
	available->width = aggregate.width;
	available->height = aggregate.height;

	return;
} /* LayoutLeaf */

/**
 ** QueryPaneGeometry()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
QueryPaneGeometry (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	allocated
)
#else
QueryPaneGeometry (w, node, who_asking, request, available, aggregate, suggested, allocated)
	Widget			w;
	PanesNode *		node;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	available;
	XtWidgetGeometry *	aggregate;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	allocated;
#endif
{
	Widget			child = PN_LEAF(node).w;

	Dimension		left;
	Dimension		right;
	Dimension		top;
	Dimension		bottom;
	Dimension		hbw_horz;
	Dimension		hbw_vert;
	Dimension		horz_stuff;
	Dimension		vert_stuff;
	Dimension		bw;


	hbw_horz = OlHandlesBorderThickness(child, OL_HORIZONTAL);
	hbw_vert = OlHandlesBorderThickness(child, OL_VERTICAL);
	bw = available->border_width;

	GetDecorationBreadths (
		node, &left, &right, &top, &bottom,
		who_asking, request, False
	);
	horz_stuff = left + 2*hbw_horz + 2*bw + right;
	vert_stuff = top + 2*hbw_vert + 2*bw + bottom;

#define SETSIZE(BIT,DIM,STUFF) \
	if (suggested && suggested->request_mode & BIT)			\
		allocated->DIM = suggested->DIM;			\
	else {								\
		/*							\
		 * Subtract the decoration breadths and both borders	\
		 * from available, using Subtract to maintain a minimum	\
		 * result of 1. Doing this ensures that allocated is	\
		 * large enough for at least a non-degenerate pane.	\
		 * If this generosity causes us to exceed the bounds of	\
		 * available, so be it. That's better than causing a	\
		 * protocol error by resizing a degenerate window.	\
		 */							\
		allocated->DIM = Subtract(available->DIM, STUFF);	\
		if (allocated->DIM == 0)				\
			allocated->DIM = 1;				\
	}

	SETSIZE (CWWidth, width, horz_stuff)
	SETSIZE (CWHeight, height, vert_stuff)
#undef	SETSIZE

	/*
	 * allocated is now what we think should be the size of the
	 * pane. But we define the span of a decoration to include the
	 * Handles widget's border and the pane's border, so we have to
	 * add them in, query the decorations for their desired span,
	 * then subtract the borders back out (grunt).
	 */
	allocated->width += 2*hbw_horz + 2*bw;
	allocated->height += 2*hbw_vert + 2*bw;
	QueryDecorationSpans (
		node, &allocated->width, &allocated->height,
		who_asking, request, False
	);
	allocated->width -= 2*hbw_horz + 2*bw;
	allocated->height -= 2*hbw_vert + 2*bw;

	allocated->x = available->x + left + hbw_horz;
	allocated->y = available->y + top + hbw_vert;

	aggregate->x = available->x;
	aggregate->y = available->y;
	aggregate->width = allocated->width + horz_stuff;
	aggregate->height = allocated->height + vert_stuff;

	return;
} /* QueryPaneGeometry */

/**
 ** QueryGeometry()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
QueryGeometry (
	Widget			w,
	XtWidgetGeometry *	suggested,
	XtWidgetGeometry *	preferred
)
#else
QueryGeometry (w, suggested, preferred)
	Widget			w;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	preferred;
#endif
{
	Widget			parent = XtParent(w);
	OlQueryGeometryCallData	c;

	if (PANES_P(parent).query_geometry) {
		/*
		 * The following is patterned after XtQueryGeometry.
		 */

		XtWidgetGeometry	empty;

		if (!suggested) {
			empty.request_mode = 0;
			suggested = &empty;
		}
		c.w = w;
		c.suggested = suggested;
		c.preferred = preferred;
		c.result = XtGeometryYes;
		preferred->request_mode = 0;

		XtCallCallbacks (parent, XtNqueryGeometry, (XtPointer)&c);

#define FILLIN(BIT,FIELD) \
		if (!(preferred->request_mode & BIT))			\
			preferred->FIELD = CORE_P(w).FIELD

		FILLIN (CWX, x);
		FILLIN (CWY, y);
		FILLIN (CWWidth, width);
		FILLIN (CWHeight, height);
		FILLIN (CWBorderWidth, border_width);
#undef	FILLIN
	} else
		c.result = XtQueryGeometry(w, suggested, preferred);
	return (c.result);
} /* QueryGeometry */

/**
 ** CalculateAffectingGeometry()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
CalculateAffectingGeometry (
	Widget			w,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	available,
	Effect *		effect
)
#else
CalculateAffectingGeometry (w, who_asking, request, available, effect)
	Widget			w;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	available;
	Effect *		effect;
#endif
{
	PanesNode *		node;
	PanesNode *		parent;
	PanesNode *		grandparent;

	XtWidgetGeometry	aggregate;


	/*
	 * If this child is not a managed pane or the decoration of a
	 * managed pane, then no panes are affected. If it is a managed
	 * pane (or its decoration) but the node has no parent, there are
	 * no siblings or cousins to affect.
	 */
	FindNode (w, who_asking, FindByPaneOrDecoration, &node, &parent);
	if (!node || !parent)
		return (False);
	grandparent = OlPanesFindParent(w, parent);

	/*
	 * Use the requested size of the child to calculate the aggregate
	 * geometry of its node. Use that result to figure a new aggregate
	 * for the parent node, with the following rules:
	 *
	 * - In the direction where the layout policy is not OL_IGNORE,
	 *   then there is no "effect", i.e. this child's desired geometry
	 *   change does not affect any siblings.
	 *
	 * Assuming, then, an OL_IGNORE policy:
	 *
	 * - A change in size parallel to the orientation of the child
	 *   and its siblings is absorbed by the siblings; no widgets
	 *   outside the family are affected.
	 *
	 * - A change in size perpendicular to the orientation affects
	 *   all the siblings equally, and will affect one or more of the
	 *   siblings of the parent. In this case the parent acts like
	 *   the above.
	 *
	 * - An exception to the perpendicular case is when the parent
	 *   has no siblings; it then cannot grow or shrink and thus the
	 *   child cannot grow or shrink.
	 *
	 * Note: Must constrain request to fit within the bounds of
	 * available, in the directions of OL_IGNORE policy, since the
	 * child can't cause the Panes widget to grow.
	 */

	AccumulateLeafGeometry (
		w, node,
		who_asking, request, &aggregate, (XtWidgetGeometry *)0,
		False
	);

#define CONSTRAIN(FIELD) \
	if (								\
		PANES_P(w).layout.FIELD == OL_IGNORE			\
	     && aggregate.FIELD > available->FIELD			\
	)								\
		aggregate.FIELD = available->FIELD

	CONSTRAIN (width);
	CONSTRAIN (height);
#undef	CONSTRAIN

	/*
	 * MORE: Fix this!
	 *
	 * WARNING: As of yet the following and LayoutNodesWithEffect and
	 * CalculateAffectedGeometry are broken. However, dumb luck helps
	 * us out for the typical case where the layout policy is the
	 * same in both width and height (OL_IGNORE in both dimensions).
	 *
	 * The problem arises for linear panes layouts (e.g. a RubberTile
	 * layout).
	 *
	 * Here's what's happening: For the linear case, with OL_IGNORE
	 * in both directions and an attempt to resize a pane in the
	 * perpendicular dimension, LayoutNodesWithEffect will call
	 * CalculateAffectedGeometry and get the new size for that
	 * perpendicular dimension for affected nodes. But this is wrong,
	 * it should get the old size, since OL_IGNORE is supposed to
	 * prevent any resize in that dimension. However, dumb luck in
	 * LayoutNodesWithEffect will conspire to impose the (correct)
	 * original geometry on the affecting node. This will cause the
	 * geometry request to fail, and Xt won't try to do anything,
	 * leaving the other panes as they were (thank goodness for the
	 * LayoutExtension, which first queries before actually doing the
	 * geometry request.)
	 */
#define INIT(E,WITH,POS,DIM,PERP,PPOS,PDIM) \
	if (PANES_P(w).layout.DIM == OL_IGNORE) {			\
		E->WITH.node = node;					\
		E->WITH.parent = parent;				\
		E->WITH.geometry = aggregate;				\
	} else								\
		E->WITH.node = 0;					\
	E->PERP.geometry.PPOS = aggregate.PPOS;				\
	E->PERP.geometry.PDIM = aggregate.PDIM;				\
	if (PANES_P(w).layout.PDIM == OL_IGNORE) {			\
		E->PERP.node = parent;					\
		E->PERP.parent = grandparent;				\
		E->PERP.geometry.POS = PN_ANY(parent).geometry.POS;	\
		E->PERP.geometry.DIM = PN_ANY(parent).geometry.DIM;	\
		if (!grandparent) {					\
		   E->PERP.geometry.PPOS = PN_ANY(parent).geometry.PPOS;\
		   E->PERP.geometry.PDIM = PN_ANY(parent).geometry.PDIM;\
		}							\
	} else								\
		E->PERP.node = 0;

	if (IsHorizontal(parent)) {
		INIT (effect, horz, x, width, vert, y, height)
	} else {
		INIT (effect, vert, y, height, horz, x, width)
	}
#undef	INIT

	return (effect->horz.node || effect->vert.node);
} /* CalculateAffectingGeometry */

/**
 ** CalculateAffectedGeometry()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
CalculateAffectedGeometry (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Effect *		effect,
	Boolean			adjacent,
	XtWidgetGeometry *	result
)
#else
CalculateAffectedGeometry (w, node, parent, effect, adjacent, result)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Effect *		effect;
	Boolean			adjacent;
	XtWidgetGeometry *	result;
#endif
{
	Boolean			response;

	PanesNode *		pn;

	short int		dim;	/* int to allow < 0 */


	/*
	 * Anticipate that at least some of the geometry won't change.
	 * (Don't set the "parallel" position here, the caller takes
	 * care of it.)
	 */
/*	result->x = PN_ANY(node).geometry.x;			*/
/*	result->y = PN_ANY(node).geometry.y;			*/
	result->width = PN_ANY(node).geometry.width;
	result->height = PN_ANY(node).geometry.height;

#define CALC(E,WITH,POS,DIM,PERP,OPOS,ODIM,EDGE) \
	pn = E->WITH.node;						\
	/*								\
	 * If this node is above or left-of the affecting node, and	\
	 * either the desired geometry of the affecting node overlaps	\
	 * this node or the desired geometry is right- or bottom-	\
	 * shifted and this node is adjacent to the affecting node,	\
	 * then construct a hint for this node's new geometry.		\
	 */								\
	if (								\
	    LeftOrAbove(node, pn)					\
	 && (E->WITH.geometry.POS < EDGE(PN_ANY(node).geometry)		\
	  || E->WITH.geometry.POS != PN_ANY(pn).geometry.POS && adjacent)\
	) {								\
		dim = E->WITH.geometry.POS - PN_ANY(node).geometry.POS;	\
		if (dim < 0) {						\
/*			result->POS = E->WITH.geometry.POS;	*/	\
			result->DIM = 0;				\
		} else							\
			result->DIM = dim;				\
		response = True;					\
	/*								\
	 * If this node is below or right-of the affecting node, and	\
	 * either the desired geometry of the affecting node overlaps	\
	 * this node or the desired geometry is left- or top-		\
	 * shifted and this node is adjacent to the affecting node,	\
	 * then construct a hint for this node's new geometry.		\
	 */								\
	} else if (							\
	    LeftOrAbove(pn, node)					\
	 && (PN_ANY(node).geometry.POS < EDGE(E->WITH.geometry)		\
	  || EDGE(E->WITH.geometry) != EDGE(PN_ANY(pn).geometry) && adjacent)\
	) {								\
		dim = EDGE(PN_ANY(node).geometry) - EDGE(E->WITH.geometry);\
		if (dim < 0) {						\
/*			result->POS = EDGE(E->WITH.geometry);	*/	\
			result->DIM = 0;				\
		} else							\
			result->DIM = dim;				\
		response = True;					\
	} else								\
		response = False;					\
	/*								\
	 * If the position or dimension perpendicular to the orientation\
	 * has changed, update the hint and tell the caller that things	\
	 * have changed.						\
	 *								\
	 * WARNING: See WARNING in CalculateAffectingGeometry.		\
	 */								\
	result->OPOS = E->WITH.geometry.OPOS;				\
	result->ODIM = E->WITH.geometry.ODIM;				\
	if (								\
		result->OPOS != PN_ANY(node).geometry.OPOS		\
	     || result->ODIM != PN_ANY(node).geometry.ODIM		\
	)								\
		response = True;

	if (parent == effect->horz.parent) {
		CALC (effect, horz, x, width, vert, y, height, RightEdge)
	} else {
		CALC (effect, vert, y, height, horz, x, width, BottomEdge)
	}
#undef	CALC

#if	defined(PANES_DEBUG)
	if (panes_debug) {
		printf (
		"CalculateAffectedGeometry: %s/%s/%x horz(%d,%d;%d,%d) vert(%d,%d;%d,%d) -> (%d,%d;%d,%d) vs (%d,%d;%d,%d) node %s, response %d\n",
			XtName(w), CLASS(XtClass(w)), w,
			effect->horz.geometry.x,
			effect->horz.geometry.y,
			effect->horz.geometry.width,
			effect->horz.geometry.height,
			effect->vert.geometry.x,
			effect->vert.geometry.y,
			effect->vert.geometry.width,
			effect->vert.geometry.height,
			result->x, result->y, result->width, result->height,
			PN_ANY(node).geometry.x,
			PN_ANY(node).geometry.y,
			PN_ANY(node).geometry.width,
			PN_ANY(node).geometry.height,
			PN_ANY(node).type == OlPanesLeafNode?
				XtName(PN_LEAF(node).w) : "(branch)",
			response
		);
	}
#endif
	return (response);
} /* CalculateAffectedGeometry */

/**
 ** AccumulateTreeGeometry()
 **/

static void
#if	OlNeedFunctionPrototypes
AccumulateTreeGeometry (
	Widget			w,
	PanesNode **		nodes,
	PanesNode *		parent,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result
)
#else
AccumulateTreeGeometry (w, nodes, parent, query_only, who_asking, request, result)
	Widget			w;
	PanesNode **		nodes;
	PanesNode *		parent;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	result;
#endif
{
	/*
	 * AccumulateTreeGeometry gets called for every node (branch or
	 * leaf), so a single increment per visit will step us through the
	 * entire tree-array.
	 */
	PanesNode *		this = NodePlusPlus(w, nodes);

	XtWidgetGeometry	g;

	DESCEND


	/*
	 * Node position is passed downwards, size is passed back.
	 * What this means is AccumulateTreeGeometry (a recursive routine)
	 * is passed the (x,y) position of the node ("this"), and will
	 * calculate the node's geometry. result contains both the
	 * passed position and the returned size.
	 */

	if (PN_ANY(this).type == OlPanesTreeNode) {
		Cardinal		n;

		g.width = 0;
		g.height = 0;
		for (n = 0; n < PN_TREE(this).num_nodes; n++) {
			if (IsHorizontal(this)) {
				if (n != 0)
					g.width = Space(g.width, PN_ANY(*nodes).space);
				g.x = result->x + g.width;
				g.y = result->y;
			} else {
				if (n != 0)
					g.height = Space(g.height, PN_ANY(*nodes).space);
				g.x = result->x;
				g.y = result->y + g.height;
			}
			AccumulateTreeGeometry (
				w, nodes, this,
				query_only, who_asking, request, &g
			);
		}
	} else
		AccumulateLeafGeometry (
			w, this,
			who_asking, request, &g, (XtWidgetGeometry *)0,
			True
		);

#if	defined(DONT_IGNORE_QUERY_ONLY)
	if (!query_only) {
#endif
		PN_ANY(this).geometry.x = result->x;
		PN_ANY(this).geometry.y = result->y;
		PN_ANY(this).geometry.width = g.width;
		PN_ANY(this).geometry.height = g.height;
#if	defined(DONT_IGNORE_QUERY_ONLY)
	}
#endif

	if (parent) {
#define UPDATE(DIM,PDIM) \
		result->DIM += g.DIM;					\
		if (g.PDIM > result->PDIM)				\
			result->PDIM = g.PDIM

		if (IsHorizontal(parent)) {
			UPDATE (width, height);
		} else {
			UPDATE (height, width);
		}
#undef	UPDATE
	} else {
		result->width = g.width;
		result->height = g.height;
	}

	ASCEND
	return;
} /* AccumulateTreeGeometry */

/**
 ** AccumulateLeafGeometry()
 **/

static void
#if	OlNeedFunctionPrototypes
AccumulateLeafGeometry (
	Widget			w,
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result,
	XtWidgetGeometry *	pane,
	Boolean			obey_ignores
)
#else
AccumulateLeafGeometry (w, node, who_asking, request, result, pane, obey_ignores)
	Widget			w;
	PanesNode *		node;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	XtWidgetGeometry *	result;
	XtWidgetGeometry *	pane;
	Boolean			obey_ignores;
#endif
{
	Widget			child = PN_LEAF(node).w;

	OlPanesGeometryProc	pane_geometry
				= PANES_C(XtClass(w)).pane_geometry;

	Dimension		left;
	Dimension		right;
	Dimension		top;
	Dimension		bottom;
	Dimension		hbw_horz;
	Dimension		hbw_vert;

	XtWidgetGeometry	core;
	XtWidgetGeometry	junk;


	/*
	 * The caller can choose to give a null pointer for pane, but
	 * we need to use pane for an intermediate result so make sure
	 * it is never null.
	 */
	if (!pane)
		pane = &junk;

	/*
	 * Calculate the pane's aggregate size from the current sizes of
	 * the pane, its border, and the decoration(s). Make sure, though,
	 * that the limitations of any spanning decorations are met.
	 *
	 * Normally we shouldn't query widget children here, rather we
	 * should just use the current core values. However, we want to
	 * correct pane sizes that have been poorly set (or not set) by
	 * the client, so that minimum decoration spans are satisfied.
	 * Having done this once (typically on getting here via the
	 * change_managed method with new panes) the pane's core values
	 * should be correct in the future, unless the (grand)parent has
	 * forced a too-small size.
	 */

	hbw_horz = OlHandlesBorderThickness(child, OL_HORIZONTAL);
	hbw_vert = OlHandlesBorderThickness(child, OL_VERTICAL);

#define GetPosAndDimFromRequestOrCore(G,W,POS,DIM,WHOASKING,REQUEST,OBEY) \
	if (UseRequest(W,DIM,WHOASKING,OBEY)) {				\
		(G)->POS = (REQUEST)->POS;				\
		(G)->DIM = (REQUEST)->DIM + 2*(REQUEST)->border_width;	\
	} else {							\
		(G)->POS = core.POS;					\
		(G)->DIM = core.DIM + 2*core.border_width;		\
	}

	/*
	 * A subclass may have a different idea of the pane's preferred
	 * geometry than we do.
	 */
	(*pane_geometry) (w, node, &core);

	GetPosAndDimFromRequestOrCore (
		pane, child, x, width, who_asking, request, obey_ignores
	)
	GetPosAndDimFromRequestOrCore (
		pane, child, y, height, who_asking, request, obey_ignores
	)
#undef	GetPosAndDimFromRequestOrCore

	/*
	 * The following isn't ideal, but it seems to be the best of
	 * the alternatives.
	 */
	if (
		child == who_asking
	     && PANES_P(w).layout.width != OL_IGNORE
	     && PANES_P(w).layout.height != OL_IGNORE
	  ||    !obey_ignores
	)
		pane->border_width = request->border_width;
	else
		pane->border_width = core.border_width;

	result->x = pane->x;
	result->y = pane->y;
	result->width  = pane->width + 2 * hbw_horz;
	result->height = pane->height + 2 * hbw_vert;
	QueryDecorationSpans (
		node, &result->width, &result->height,
		who_asking, request, obey_ignores
	);

	GetDecorationBreadths (
		node, &left, &right, &top, &bottom,
		who_asking, request, obey_ignores
	);
	result->x -= hbw_horz + left;
	result->y -= hbw_vert + top;
	result->width  += left + right;
	result->height += top + bottom;

	return;
} /* AccumulateLeafGeometry */

/**
 ** QueryDecorationSpans()
 **/

static void
#if	OlNeedFunctionPrototypes
QueryDecorationSpans (
	PanesNode *		node,
	Dimension *		width,
	Dimension *		height,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Boolean			obey_ignores
)
#else
QueryDecorationSpans (node, width, height, who_asking, request, obey_ignores)
	PanesNode *		node;
	Dimension *		width;
	Dimension *		height;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	Boolean			obey_ignores;
#endif
{
	Widget			decoration;

	Cardinal		n;

	XtWidgetGeometry	suggested;
	XtWidgetGeometry	preferred;


#define GetPosAndDimFromRequestOrCore(G,W,POS,DIM,WHOASKING,REQUEST,OBEY) \
	if (UseRequest(W,DIM,WHOASKING,OBEY)) {				\
		(G)->POS = (REQUEST)->POS;				\
		(G)->DIM = (REQUEST)->DIM + 2*(REQUEST)->border_width;	\
	} else {							\
		(G)->POS = CORE_P(W).POS;				\
		(G)->DIM = CORE_P(W).DIM + 2*CORE_P(W).border_width;	\
	}

	/*
	 * For each of width and heigth find the length of the longest
	 * constraining decoration.
	 */
	FOR_EACH_PANE_DECORATION (node, decoration, n)
	    if (PANES_CP(decoration).type == OL_CONSTRAINING_DECORATION) {
		/*
		 * If the child asking for a particular geometry is this
		 * decoration, we can't query it for its desired size.
		 * In that case use either request or core, depending on
		 * the OL_IGNORE case.
		 *
		 * MORE: It is probably impossible to guarantee that the
		 * returned spans meet the needs of all the decorations.
		 * (E.g. imagine two decorations, one wants only even
		 * span, other wants only odd span.) But can we do better
		 * than this?
		 *
		 * MORE: Only query in the length dimension, use core
		 * (or ignore) the breadth dimension.
		 *
		 * MODE: What if decoration wants a different breadth
		 * dimension than core?
		 */
		if (who_asking == decoration) {
			GetPosAndDimFromRequestOrCore (
				&preferred, decoration, x, width,
				who_asking, request, obey_ignores
			)
			GetPosAndDimFromRequestOrCore (
				&preferred, decoration, y, height,
				who_asking, request, obey_ignores
			)
		} else {
			suggested.width = *width;
			suggested.height = *height;
			suggested.border_width = CORE_P(decoration).border_width;
			suggested.request_mode = (CWWidth|CWHeight|CWBorderWidth);
			OlQueryChildGeometry (decoration, &suggested, &preferred);
			preferred.width += 2*suggested.border_width;
			preferred.height += 2*suggested.border_width;
		}

		switch (PANES_CP(decoration).position) {
		case OL_LEFT:
		case OL_RIGHT:
			if (*height < preferred.height)
				*height = preferred.height;
			break;
		case OL_TOP:
		case OL_BOTTOM:
			if (*width < preferred.width)
				*width = preferred.width;
			break;
		}
	    }

#undef	GetPosAndDimFromRequestOrCore
	return;
} /* QueryDecorationSpans */

/**
 ** GetDecorationBreadths()
 **/

static void
#if	OlNeedFunctionPrototypes
GetDecorationBreadths (
	PanesNode *		node,
	Dimension *		left,
	Dimension *		right,
	Dimension *		top,
	Dimension *		bottom,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	Boolean			obey_ignores
)
#else
GetDecorationBreadths (node, left, right, top, bottom, who_asking, request, obey_ignores)
	PanesNode *		node;
	Dimension *		left;
	Dimension *		right;
	Dimension *		top;
	Dimension *		bottom;
	Widget			who_asking;
	XtWidgetGeometry *	request;
	Boolean			obey_ignores;
#endif
{
	Widget			decoration;

	Cardinal		n;


#define	WIDTH(W) \
	(UseRequest(decoration, width, who_asking, obey_ignores)?	\
		  request->width + 2*request->border_width		\
		: _OlWidgetWidth(W))
#define HEIGHT(W) \
	(UseRequest(decoration, height, who_asking, obey_ignores)?	\
		  request->height + 2*request->border_width		\
		: _OlWidgetHeight(W))

	*left = *right = *top = *bottom = 0;

	/*
	 * Multiple decorations on the same side are "concatenated".
	 * See ConfigureDecorations for details.
	 */
	FOR_EACH_PANE_DECORATION (node, decoration, n) {
		Position	space = PANES_CP(decoration).space;
		switch (PANES_CP(decoration).position) {
		case OL_LEFT:
			*left += Space(WIDTH(decoration), space);
			break;
		case OL_RIGHT:
			*right += Space(WIDTH(decoration), space);
			break;
		case OL_TOP:
			*top += Space(HEIGHT(decoration), space);
			break;
		case OL_BOTTOM:
			*bottom += Space(HEIGHT(decoration), space);
			break;
		}
	}

#undef	WIDTH
#undef	HEIGHT
	return;
} /* GetDecorationBreadths */

/**
 ** ConfigureDecorations()
 **/

static void
#if	OlNeedFunctionPrototypes
ConfigureDecorations (
	PanesNode *		node,
	XtWidgetGeometry *	available,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	response
)
#else
ConfigureDecorations (node, available, query_only, who_asking, response)
	PanesNode *		node;
	XtWidgetGeometry *	available;
	Boolean			query_only;
	Widget			who_asking;
	XtWidgetGeometry *	response;
#endif
{
	Position		left;
	Position		right;
	Position		top;
	Position		bottom;

	Widget			decoration;

	Cardinal		n;


	/*
	 * The passed geometry is of the pane plus the border drawn by
	 * the Handles widget (if any).
	 *
	 * Multiple decorations on the same side of the pane are
	 * "concatenated", as the following picture suggests
	 * (a-h's define decorations, X's define the pane).
	 *
	 *               aaaaaaaaaa
	 *               aaaaaaaaaa
	 *               bbbbbbbbbbbbb
	 *      cccddddddXXXXXXXXXXXXXeeffffff
	 *      cccddddddXXXXXXXXXXXXXeeffffff
	 *      cccddddddXXXXXXXXXXXXXeeffffff
	 *      cccddddddXXXXXXXXXXXXXeeffffff
	 *      cccddddddXXXXXXXXXXXXXeeffffff
	 *      ccc      XXXXXXXXXXXXXeeffffff
	 *      ccc      XXXXXXXXXXXXXee          
	 *               ggggggggggggg
	 *               hhhhhhhh
	 */
                    
	left = available->x;
	right = RightEdge(*available);
	top = available->y;
	bottom = BottomEdge(*available);

	FOR_EACH_PANE_DECORATION (node, decoration, n) {
		Position	x;
		Position	y;
		Dimension	width;
		Dimension	height;
		Dimension	bw = CORE_P(decoration).border_width;
		Position	space = PANES_CP(decoration).space;

		 /* B for Breadth, L for Length */
#define CALCULATE(SIDE,BPOS,LPOS,BDIM,LDIM) \
		BPOS = SIDE;						\
		LPOS = available->LPOS;					\
		BDIM = CORE_P(decoration).BDIM;				\
		if (							\
			PANES_CP(decoration).type == OL_DECORATION	\
		     && (int)CORE_P(decoration).LDIM + (int)(2*bw)	\
						 < (int)available->LDIM	\
		)							\
			LDIM = CORE_P(decoration).LDIM;			\
		else							\
			LDIM = available->LDIM - 2*bw

		switch (PANES_CP(decoration).position) {
		case OL_LEFT:
			left -= Space(CORE_P(decoration).width + 2*bw, space);
			CALCULATE (left, x, y, width, height);
			break;
		case OL_RIGHT:
			right = Space(right, space);
			CALCULATE (right, x, y, width, height);
			right += CORE_P(decoration).width + 2*bw;
			break;
		case OL_TOP:
			top -= Space(CORE_P(decoration).height + 2*bw, space);
			CALCULATE (top, y, x, height, width);
			break;
		case OL_BOTTOM:
			bottom = Space(bottom, space);
			CALCULATE (bottom, y, x, height, width);
			bottom += CORE_P(decoration).height + 2*bw;
			break;
		}
		OlConfigureChild (
			decoration,
			x, y, width, height, bw,
			query_only, who_asking, response
		);
	}

	return;
} /* ConfigureDecorations */

/**
 ** AccumulateWeights()
 **/

#if	defined(USE_ACCUMULATE_WEIGHTS)

static void
#if	OlNeedFunctionPrototypes
AccumulateWeights (
	Widget			w
)
#else
AccumulateWeights (w)
	Widget			w;
#endif
{
	/*
	 * We will walk the tree, visiting each branch node before and
	 * after visiting its children. For branch nodes, the PreOrder
	 * visit will clear its .weight field and the PostOrder visit
	 * will copy its .weight field to the .weight field of its parent.
	 * Leaf visits just set their .weight field from the constraint
	 * record and add to their parent's field.
	 */
	OlPanesWalkTree (
		w, OlPanesAnyNode, PreOrder|PostOrder, AddWeight, (XtPointer)0
	);
	return;
} /* AccumulateWeights */

#endif

/**
 ** AddWeight()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
AddWeight (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
)
#else
AddWeight (w, node, parent, n, walk, client_data)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesWalkType		walk;
	XtPointer		client_data;
#endif
{
#if	defined(USE_ACCUMULATE_WEIGHTS)
	if (PN_ANY(node).type == OlPanesTreeNode) {
		if (walk == PreOrder)
			PN_ANY(node).weight = 0;
		else if (parent)
			PN_ANY(parent).weight += PN_ANY(node).weight;
	} else {
		PN_ANY(node).weight = PANES_CP(PN_LEAF(node).w).weight;
		if (parent)
			PN_ANY(parent).weight += PN_ANY(node).weight;
	}
#else
	PN_ANY(node).weight += (short)client_data;
#endif
	return (True);
} /* AddWeight */

/**
 ** SubtractInterNodeSpace()
 **/

static void
#if	OlNeedFunctionPrototypes
SubtractInterNodeSpace (
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	geometry
)
#else
SubtractInterNodeSpace (w, node, geometry)
	Widget			w;
	PanesNode *		node;
	XtWidgetGeometry *	geometry;
#endif
{
	Cardinal		n;
	Cardinal		num_nodes = PN_TREE(node).num_nodes;

	Dimension *		dim = IsHorizontal(node)?
				     &geometry->width : &geometry->height;

	node = NodePlusOne(w, node);
	for (n = 0; n < num_nodes; n++) {
		if (n != 0)
			if (PN_ANY(node).space > (int)*dim)
				*dim = 0;
			else
				*dim = Space(*dim, -PN_ANY(node).space);
		SkipNode (w, &node);
	}
	return;
} /* SubtractInterNodeSpace */

/**
 ** CreatePane()
 **/

static PanesNode *
#if	OlNeedFunctionPrototypes
CreatePane (
	Widget			w,
	Widget			new
)
#else
CreatePane (w, new)
	Widget			w;
	Widget			new;
#endif
{
	OlPanesStealProc	steal_geometry
				= PANES_C(XtClass(w)).steal_geometry;

	Widget			ref = ResolveReference(w, new);
	Widget			child;

	Cardinal		n;

	PanesNode *		node   = 0;
	PanesNode *		parent = 0;
	PanesNode *		old    = 0;

	OlDefine		position = PANES_CP(new).position;
	OlDefine		orientation;


	/*
	 * If the client did not provide a reference for this child, by
	 * default it references the previous managed child. If there is
	 * no previous child (i.e. this is the first managed child in the
	 * .children list) then it references the next managed child--but
	 * the position is opposite the next child's position. If there is
	 * no next child, there is no reference and the child at hand must
	 * be the root of the tree of panes.
	 *
	 * Exception: If the tree is empty we don't need a reference, for
	 * this pane will become the root of the tree.
	 */
	if (!ref && PANES_P(w).nodes) {
		FOR_EACH_MANAGED_PANE (w, child, n)
			if (child == new)
				break;
			else
				ref = child;
		if (!ref)
			FOR_EACH_MANAGED_PANE (w, child, n)
				if (child == new)
					continue;
				else {
					ref = child;
					switch (PANES_CP(ref).position) {
					case OL_LEFT:
						position = OL_RIGHT;
						break;
					case OL_RIGHT:
						position = OL_LEFT;
						break;
					case OL_TOP:
						position = OL_BOTTOM;
						break;
					case OL_BOTTOM:
						position = OL_TOP;
						break;
					}
					break;
				}
	}

	if (ref) {
		OlPanesFindNode (w, ref, &node, &parent);
		if (!node)
			return (0);
		switch (position) {
		case OL_LEFT:
		case OL_RIGHT:
			orientation = OL_HORIZONTAL;
			break;
		case OL_TOP:
		case OL_BOTTOM:
			orientation = OL_VERTICAL;
			break;
		}
	}

	/*
	 * Update the tree structure for the panes:
	 *
	 * If there is no referenced node, the new node becomes the root
	 * of the entire tree of panes.
	 *
	 * If this node does not have a parent, it is the single
	 * (leaf) node for the entire tree of panes. Conversely,
	 * if it does have a parent, it also has at least one sibling
	 * (by design).
	 *
	 * Note: node starts off pointing to the referenced node (if any),
	 * and this node is a leaf node. Thus, AppendNodes doesn't have
	 * to skip children nodes when it does its work. Also, regardless
	 * of which if-then-else branch is taken below, node will end up
	 * pointing to the new node.
	 */
	if (!ref)
		node = AppendNodes(w, (PanesNode *)0, 1);
	else if (parent && PN_TREE(parent).orientation == orientation) {
		/*
		 * The new pane is in the same direction as the rest of
		 * the panes in this group, so just increase the existing
		 * list of children.
		 *
		 * WARNING: AppendNodes will reallocate the node list,
		 * invalidating any PanesNode pointers we have.
		 */
		int p = (parent? NodeToIndex(w, parent) : -1);

		PN_TREE(parent).num_nodes++;
		old = AppendNodes(w, node, 1);
		node = NodePlusOne(w, old);
		parent = (p != -1? IndexToNode(w, p) : 0);
	} else {
		/*
		 * The new pane is added perpendicular to the other panes
		 * in this group, or is referenced to the only pane, so
		 * branch this node into two leaves. Doing this does not
		 * affect the number of nodes in the parent branch.
		 *
		 * Since we're creating the parent here, we have to
		 * initialize it. This is a bit awkward, since it already
		 * has an initialized child. However, that's not all bad,
		 * since it gives the initialization routines access to
		 * information about the pane that forms the root of the
		 * subtree. We make sure we pass to the initialization
		 * routines a consistent state of 1 child (not 2 children,
		 * since the new node-child has not yet been initialized).
		 *
		 * Note that some of the initialization of the parent node
		 * is done here instead of in the Panes node_initialize:
		 * .type and .num_nodes because node_initialize can't know
		 * this information otherwise, .orientation because we
		 * have the information available (saves node_initialize
		 * from figuring it out again).
		 *
		 * WARNING: AppendNodes will reallocate the node list,
		 * invalidating any PanesNode pointers we have.
		 */
		char stack[64];
		PanesNode * save = (PanesNode *)XtStackAlloc(NodeSize(w), stack);
		int gp = (parent? NodeToIndex(w, parent) : -1);

		CopyNode (w, save, node);
		parent = AppendNodes(w, node, 2);
		old = NodePlusOne(w, parent);
		CopyNode (w, old, save);

		PN_ANY(parent).type = OlPanesTreeNode;
		PN_TREE(parent).orientation = orientation;
		PN_TREE(parent).num_nodes = 1;
		CallNodeInitialize (
			XtClass(w), w, parent,
			gp != -1? IndexToNode(w, gp) : (PanesNode *)0
			/* gp => grandparent, i.e. the original parent */
		);

		PN_TREE(parent).num_nodes = 2;
		node = NodePlusOne(w, old);

		XtStackFree (save, stack);
	}

	/*
	 * If this pane is to come before the referenced pane, swap
	 * nodes.
	 */
	if (ref) {
		PanesNode *		tmp;

		switch (position) {
		case OL_TOP:
		case OL_LEFT:
			/*
			 * Since the new node is not yet initialized, we
			 * don't have to swap any data from it.
			 */
			CopyNode (w, node, old);
			tmp = node;
			node = old;
			old = tmp;
			break;
		}
	}

	/*
	 * Populate the new node.
	 */
	PN_ANY(node).type = OlPanesLeafNode;
	PN_LEAF(node).w = new;
	CallNodeInitialize (XtClass(w), w, node, parent);

	/*
	 * Allow a subclass to steal geometry from other nodes (typically
	 * the referenced node) to meet the geometry needs of the new
	 * pane. The steal_geometry routine is free to adjust the geometry
	 * of any of the panes at this point.
	 */
	if (steal_geometry && old)
		(*steal_geometry) (w, node, old);

	return (node);
} /* CreatePane */

/**
 ** CallNodeInitialize()
 **/

static void
#if	OlNeedFunctionPrototypes
CallNodeInitialize (
	WidgetClass		wc,
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
)
#else
CallNodeInitialize (wc, w, node, parent)
	WidgetClass		wc;
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
#endif
{
	if (wc != thisClass)
		CallNodeInitialize (SUPER_C(wc), w, node, parent);
	if (PANES_C(wc).node_initialize)
		(*PANES_C(wc).node_initialize) (w, node, parent);
	return;
} /* CallNodeInitialize */

/**
 ** DeletePane()
 **/

static void
#if	OlNeedFunctionPrototypes
DeletePane (
	Widget			w,
	PanesNode *		old,
	PanesNode *		parent
)
#else
DeletePane (w, old, parent)
	Widget			w;
	PanesNode *		old;
	PanesNode *		parent;
#endif
{
	OlPanesNodeProc		recover_geometry
				= PANES_C(XtClass(w)).recover_geometry;


	/*
	 * Allow a subclass to rearrange the geometry of the remaining
	 * panes to make up for this lost pane.
	 */
	if (recover_geometry)
		(*recover_geometry) (w, old, parent);

	/*
	 * Free up node space used by the subclass.
	 */
	CallNodeDestroy (w, old, parent);

	/*
	 * Remove the node from the tree.
	 * WARNING: DeleteNodes reallocates the node list,
	 * invalidating any PanesNode pointers we have.
	 */
	if (!parent) {
		/*
		 * This is the sole node in the tree--goodbye tree!
		 */
		DestroyTree (w);
	} else if (PN_TREE(parent).num_nodes > 2) {
		/*
		 * This is easy, just decrement the parent's count and
		 * remove the node.
		 */
		PN_TREE(parent).num_nodes--;
		DeleteNodes (w, old, 1);
	} else {
		/*
		 * This is a little harder, since simply deleting the
		 * node would leave just one node in this tree. Instead,
		 * we delete the node and the parent, leaving the sibling
		 * behind. The deletions have to be made carefully--check
		 * out these pictures:
		 *
		 * old = parent + "1":
		 * +========+=========+---------+------------------+
		 * | parent |   old   | sibling | sibling children |
		 * +========+=========+---------+------------------+
		 *
		 * old > parent + "1":
		 * +========+---------+------------------+=========+
		 * | parent | sibling | sibling children |   old   |
		 * +========+---------+------------------+=========+
		 *
		 * In both pictures, the nodes to be deleted are marked
		 * with +=====+. In the first picture, both nodes can
		 * be deleted in one step; in the second picture, two
		 * steps are needed, and we must reconstruct the other
		 * pointer because the first deletion may move the arena.
		 */
		CallNodeDestroy (w, parent, OlPanesFindParent(w, parent));
		if (old == NodePlusOne(w, parent))
			DeleteNodes (w, parent, 2);
		else {
			/*
			 * Delete the "rightmost" node first (i.e. not the
			 * parent), so that the index (i) doesn't change.
			 */
			Cardinal	i = NodeToIndex(w, parent);
			DeleteNodes (w, old, 1);
			DeleteNodes (w, IndexToNode(w, i), 1);
		}
	}

	return;
} /* DeletePane */

/**
 ** CallNodeDestroy()
 **/

static void
#if	OlNeedFunctionPrototypes
CallNodeDestroy (
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
)
#else
CallNodeDestroy (w, node, parent)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
#endif
{
	WidgetClass		wc;

	for (wc = XtClass(w); wc; wc = SUPER_C(wc)) {
		if (PANES_C(wc).node_destroy)
			(*PANES_C(wc).node_destroy) (w, node, parent);
		if (wc == thisClass)
			break;
	}
	return;
} /* CallNodeDestroy */

/**
 ** AppendNodes()
 **/

static PanesNode *
#if	OlNeedFunctionPrototypes
AppendNodes (
	Widget			w,
	PanesNode *		node,
	Cardinal		n
)
#else
AppendNodes (w, node, n)
	Widget			w;
	PanesNode *		node;
	Cardinal		n;
#endif
{
	Cardinal		i;


	if (node)
		i = NodeToIndex(w, node);

	PANES_P(w).num_nodes += n;
	PANES_P(w).nodes = (PanesNode *)XtRealloc(
		(char *)PANES_P(w).nodes, PANES_P(w).num_nodes * NodeSize(w)
	);

	if (node) {
		PanesNode *		to   = IndexToNode(w, i + 1 + n);
		PanesNode *		from = IndexToNode(w, i + 1);

		node = IndexToNode(w, i);
		MemMove (
			(XtPointer)to, (XtPointer)from,
			PANES_P(w).num_nodes - (i + 1 + n),
			NodeSize(w)
		);
	} else
		node = PANES_P(w).nodes;

	return (node);
} /* AppendNodes */

/**
 ** DeleteNodes()
 **/

static void
#if	OlNeedFunctionPrototypes
DeleteNodes (
	Widget			w,
	PanesNode *		node,
	Cardinal		n
)
#else
DeleteNodes (w, node, n)
	Widget			w;
	PanesNode *		node;
	Cardinal		n;
#endif
{
	Cardinal		i = NodeToIndex(w, node);
	PanesNode *		from = IndexToNode(w, i + n);

	MemMove (
		(XtPointer)node, (XtPointer)from,
		PANES_P(w).num_nodes - (i + n),
		NodeSize(w)
	);

	PANES_P(w).num_nodes -= n;
	PANES_P(w).nodes = (PanesNode *)XtRealloc(
		(char *)PANES_P(w).nodes, PANES_P(w).num_nodes * NodeSize(w)
	);

	return;
} /* DeleteNodes */

/**
 ** MemMove()
 **/

static void
#if	OlNeedFunctionPrototypes
MemMove (
	XtPointer		to,
	XtPointer		from,
	Cardinal		n,
	Cardinal		size
)
#else
MemMove (to, from, n, size)
	char *			to;
	char *			from;
	Cardinal		n;
	Cardinal		size;
#endif
{
#if	defined(SVR4)
	memmove (to, from, n * size);
#else
	size_t		count = n * size;
	size_t		k;

	if (to < from)
		for (k = 0; k < count; k++)
			*to++ = *from++;
	else
		for (k = count-1; k >= 0; k--)
			to[k] = from[k];
#endif
	return;
} /* MemMove */

/**
 ** ReconstructTree()
 **/

static void
#if	OlNeedFunctionPrototypes
ReconstructTree (
	Widget			w,
	Boolean			destroy
)
#else
ReconstructTree (w, destroy)
	Widget			w;
	Boolean			destroy;
#endif
{
	Widget *		panes;
	Widget			stack[EstimatedMaximumPanes];
	Widget			child;

	Cardinal		n;
	Cardinal		k;
	Cardinal		pending;
	Cardinal		pending_last_time;

	PanesNode *		node;
	PanesNode *		parent;


	if (destroy)
		DestroyTree (w);

	if (PANES_P(w).pane_order)
		panes = PANES_P(w).pane_order;
	else {
		panes = (Widget *)XtStackAlloc(
			(PANES_P(w).num_panes + 1) * sizeof(Widget), stack
		);
		k = 0;
		FOR_EACH_MANAGED_PANE (w, child, n)
			panes[k++] = child;
		panes[k] = 0;
	}

	/*
	 * For each newly unmanaged pane, delete it from the tree.
	 * Don't use the panes list set/calculated above, since it
	 * lists only managed children.
	 */
	FOR_EACH_PANE (w, child, n) {
		OlPanesFindNode (w, child, &node, &parent);
		if (node && !XtIsManaged(child))
			DeletePane (w, node, parent);
	}

	/*
	 * For each newly managed pane, resolve any pending widget
	 * references and add it to the tree. This may take several
	 * passes if the references are forward-chained.
	 */
	pending_last_time = 0;
	do {
		pending = 0;
		for (k = 0; panes[k]; k++) {
			OlPanesFindNode (w, panes[k], &node, &parent);
			/*
			 * Check the managed flag, just in case whoever
			 * set the .panes class field wasn't careful.
			 */
			if (!node && XtIsManaged(panes[k]))
				if (!CreatePane(w, panes[k])) {
					/*
					 * This pane may reference a new
					 * pane we will see later in this
					 * loop. We'll try this pane again
					 * in another pass through the
					 * loop.
					 */
					pending++;
					continue;
				}
		}

		/*
		 * If any panes were skipped because their nodes couldn't
		 * be created, run through the loop again. Watch for
		 * endless loops, though.
		 */
		if (pending && pending == pending_last_time) {
			OlVaDisplayErrorMsg (
				XtDisplay(w),
				"badPane", "constructTree",
				OleCOlToolkitError,
				"Widget %s: Could not create one or more panes",
				XtName(w)
			);
			/*NOTREACHED*/
		}

		pending_last_time = pending;
	} while (pending);

	/*
	 * Sort the decorations so that those managed are listed first
	 * and arranged by pane. Then, (re)assign the panes' pointers
	 * to these subordinates.
	 */
	SortAndAssignDecorations (w);

	if (!PANES_P(w).pane_order)
		XtStackFree (panes, stack);
	return;
} /* ReconstructTree */

/**
 ** DestroyTree()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyTree (
	Widget			w
)
#else
DestroyTree (w)
	Widget			w;
#endif
{
	OlPanesWalkTree (w, OlPanesAnyNode, PostOrder, DestroyNode, (XtPointer)0);
	if (PANES_P(w).nodes) {
		XtFree ((char *)PANES_P(w).nodes);
		PANES_P(w).nodes = 0;
	}
	return;
} /* DestroyTree */

/**
 ** DestroyNode()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
DestroyNode (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data	/*NOTUSED*/
)
#else
DestroyNode (w, node, parent, n, walk, client_data)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesWalkType		walk;
	XtPointer		client_data;
#endif
{
	CallNodeDestroy (w, node, parent);
	return (True);
} /* DestroyNode */

/**
 ** WalkTree()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
WalkTree (
	Widget			w,
	PanesNode **		nodes,
	PanesNode *		parent,
	Cardinal		n,
	OlPanesNodeType		type,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
)
#else
WalkTree (w, nodes, parent, n, type, walk, func, client_data)
	Widget			w;
	PanesNode **		nodes;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesNodeType		type;
	OlPanesWalkType		walk;
	OlPanesWalkProc		func;
	XtPointer		client_data;
#endif
{
	/*
	 * WalkTree gets called for every node (branch or leaf),
	 * so a single increment per visit will step us through the
	 * entire tree-array.
	 */
	PanesNode *		this = NodePlusPlus(w, nodes);

	Boolean			ok = True;

#define VISIT(NODE,WALK) \
	(((PN_ANY(NODE).type == type || type == OlPanesAnyNode) && func)?\
		(*func)(w, NODE, parent, n, WALK, client_data) : True)


	OlPanesTreeLevel++;
	DESCEND

	/*
	 * Leaf nodes get visited once, branch nodes can be visited
	 * before and/or after visiting the branch's children.
	 */
	if ((walk & PreOrder) || PN_ANY(this).type == OlPanesLeafNode)
		ok = VISIT(this, PreOrder);

	if (ok && PN_ANY(this).type == OlPanesTreeNode) {
		Cardinal		k;

		for (k = 0; ok && k < PN_TREE(this).num_nodes; k++)
			ok = WalkTree(
			  w, nodes, this, k, type, walk, func, client_data
			);
	}

	if ((walk & PostOrder) && PN_ANY(this).type == OlPanesTreeNode)
		ok = VISIT(this, PostOrder);

#undef	VISIT

	ASCEND
	OlPanesTreeLevel--;
	return (ok);
} /* WalkTree */

/**
 ** Find()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
Find (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
)
#else
Find (w, node, parent, n, walk, client_data)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesWalkType		walk;
	XtPointer		client_data;
#endif
{
	FindInfo *		info    = (FindInfo *)client_data;


	/*
	 * If we find the node, we return False to stop the search,
	 * else we return True to continue the search.
	 */

	switch (info->type) {

	case FindByPane:
	case FindByDecoration:
	case FindByPaneOrDecoration:
		if (info->type & _FindByPane)
			if (PN_LEAF(node).w == info->w) {
				info->node   = node;
				info->parent = parent;
				return (False);
			}
		if (info->type & _FindByDecoration) {
			Widget			decoration;
			Cardinal		n;

			FOR_EACH_PANE_DECORATION (node, decoration, n)
				if (decoration == info->w) {
					info->node   = node;
					info->parent = parent;
					return (False);
				}
		}
		break;

	case FindByNode:
		if (node == info->node) {
			info->parent = parent;
			return (False);
		}
		break;
	}

	return (True);
} /* Find */

/**
 ** FindNode()
 **/

static void
#if	OlNeedFunctionPrototypes
FindNode (
	Widget			w,
	Widget			child,
	FindType		type,
	PanesNode **		node,
	PanesNode **		parent
)
#else
FindNode (w, child, type, node, parent)
	Widget			w;
	Widget			child;
	FindType		type;
	PanesNode **		node;
	PanesNode **		parent;
#endif
{
	FindInfo		info;


	info.type = type;
	info.w = child;
	info.node = 0;
	info.parent = 0;
	OlPanesWalkTree (w, OlPanesLeafNode, NoOrder, Find, (XtPointer)&info);
	*node = info.node;
	*parent = info.parent;

	return;
} /* FindNode */

/**
 ** GetDirectAncestors()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
GetDirectAncestors (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
)
#else
GetDirectAncestors (w, node, parent, n, walk, client_data)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesWalkType		walk;
	XtPointer		client_data;
#endif
{
	AncestorInfo *		info = (AncestorInfo *)client_data;

	/*
	 * This procedure is called PostOrder, i.e. the first time here
	 * will be for the node we're looking for, and we'll put the
	 * parent pointer in the list. On backing up through the nodes,
	 * this procedure will be called for siblings and ancestors. When
	 * we get here with the parent of the node we're looking for--we
	 * can tell because the parent pointer will be the previous entry
	 * in the list--we'll put its parent pointer in the list. This
	 * continues up the tree.
	 */
	if (
		node == info->node
	     || info->num_nodes && info->nodes[info->num_nodes-1] == node
	)
		/*
		 * parent will be null when we reach the root of the tree
		 * (or if this is the only node in the tree). No need to
		 * put it in the list.
		 */
		if (parent)
			info->nodes[info->num_nodes++] = parent;

	return (True);
} /* GetDirectAncestors */

/**
 ** SkipNode()
 **/

static void
#if	OlNeedFunctionPrototypes
SkipNode (
	Widget			w,
	PanesNode **		node
)
#else
SkipNode (w, node)
	Widget			w;
	PanesNode **		node;
#endif
{
	(void)WalkTree(
		w, node, *node, 0, OlPanesLeafNode, NoOrder,
		(OlPanesWalkProc)0, (XtPointer)0
	);
	return;
} /* SkipNode */

/**
 ** NodePlusPlus()
 **/

static PanesNode *
#if	OlNeedFunctionPrototypes
NodePlusPlus (
	Widget			w,
	PanesNode **		node
)
#else
NodePlusPlus (w, node)
	Widget			w;
	PanesNode **		node;
#endif
{
	PanesNode * was = *node;
	*node = NodePlusOne(w, *node);
	return (was);
} /* NodePlusPlus */

/**
 ** ResolveReference()
 **/

static Widget
#if	OlNeedFunctionPrototypes
ResolveReference (
	Widget			w,
	Widget			child
)
#else
ResolveReference (w, child)
	Widget			w;
	Widget			child;
#endif
{
	Widget			ref;

#if	defined(CAN_CHAIN_DECORATION_REFERENCES)
	static WidgetArray	array = _OL_ARRAY_INITIAL;

	int			i;
	int			hint;


	/*
	 * We allow a decoration to reference another decoration, to
	 * provide an order for the decorations that is different from
	 * their creation order. Unfortunately, this means we have to
	 * watch out for looping chains of reference.
	 */
#endif

	OlResolveReference (
		child,
		&PANES_CP(child).ref_widget, &PANES_CP(child).ref_name,
		OlReferenceManagedSibling,
		XtNameToWidget, w
	);
	ref = PANES_CP(child).ref_widget;

	if (ref) {
		switch (PANES_CP(ref).type) {
		case OL_PANE:
			break;
#if	defined(CAN_CHAIN_DECORATION_REFERENCES)
		case OL_DECORATION:
		case OL_CONSTRAINING_DECORATION:
		case OL_SPANNING_DECORATION:
			if (PANES_CP(child).type == OL_PANE) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"paneRefDecoration", "resolveReference",
					OleCOlToolkitWarning,
					"Widget %s: Pane widget (%s) references a decoration (%s)",
					XtName(w), XtName(child), XtName(ref)
				);
				goto ClearRef;
			}
			i = _OlArrayFindHint(&array, &hint, ref);
			if (i != _OL_NULL_ARRAY_INDEX) {
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"cyclicChain", "resolveReference",
					OleCOlToolkitWarning,
					"Widget %s: Cyclic reference (%s, %s)",
					XtName(w), XtName(child), XtName(ref)
				);
				goto ClearRef;
			}
			_OlArrayHintedOrderedInsert (&array, hint, ref);
			ref = ResolveReference(w, ref);
			break;
		case OL_HANDLES:
		case OL_OTHER:
#endif
		default:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				"illegalReference", "resolveReference",
				OleCOlToolkitWarning,
				"Widget %s: Widget %s references illegal widget (%s)",
				XtName(w), XtName(child), XtName(ref)
			);

#if	defined(CAN_CHAIN_DECORATION_REFERENCES)
ClearRef:
#endif
			/*
			 * Prevent further complaints.
			 */
			if (PANES_CP(child).ref_name) {
				XtFree (PANES_CP(child).ref_name);
				PANES_CP(child).ref_name = 0;
			}
			PANES_CP(child).ref_widget = 0;
			ref = 0;
		}
	}

#if	defined(CAN_CHAIN_DECORATION_REFERENCES)
	/*
	 * Rolling back the call frame for a recursive call will "free"
	 * the array more than once, but that is OK as _OlArrayFree checks
	 * before actually freeing anything.
	 */
	_OlArrayFree (&array);
#endif

	return (ref);
} /* ResolveReference */

/**
 ** InsertPosition()
 **/

static Cardinal
#if	OlNeedFunctionPrototypes
InsertPosition (
	Widget			w
)
#else
InsertPosition (w)
	Widget			w;
#endif
{
	Widget			parent = XtParent(w);

	Cardinal		min;
	Cardinal		max;
	Cardinal		pos;


	/*
	 * All panes go up front in the .children list, decorations
	 * go at the end of the list. The client can help specify the
	 * order of panes and decorations, but is restricted in this way.
	 *
	 * This ordering facilitates the additional sorting of the
	 * decorations, by pane and managed state, done from
	 * ChangeManaged.
	 *
	 * Maintaining this order also avoids a potential problem in
	 * ChangeManaged if we delete decorations. If ChangeManaged is
	 * called from outside XtDispatchEvent, then the phase 2 of
	 * XtDestroyWidget will occur immediately when we delete a
	 * decoration, and that could collapse the .children list
	 * causing a pane to be skipped.
	 */
	switch (PANES_CP(w).type) {
	case OL_PANE:
		min = 0;
		max = PANES_P(parent).num_panes++;
		break;
	case OL_DECORATION:
	case OL_SPANNING_DECORATION:
	case OL_CONSTRAINING_DECORATION:
		min = PANES_P(parent).num_panes;
		max = min + PANES_P(parent).num_decorations++;
		break;
	case OL_HANDLES:
	case OL_OTHER:
	default:
		min = max = COMPOSITE_P(parent).num_children;
		break;
	}
	if (PANES_P(parent).insert_position) {
		pos = (*PANES_P(parent).insert_position)(w);
		if (pos < min)
			pos = min;
		if (pos > max)
			pos = max;
	} else
		pos = max;

	return (pos);
} /* InsertPosition */

/**
 ** SortAndAssignDecorations()
 **/

static void
#if	OlNeedFunctionPrototypes
SortAndAssignDecorations (
	Widget			w
)
#else
SortAndAssignDecorations (w)
	Widget			w;
#endif
{
	Widget			ref;
	Widget			decoration;
	Widget			last_ref;

	Cardinal		n;

	PanesNode *		node;
	PanesNode *		parent;


	if (!PANES_P(w).num_decorations)
		return;

	/*
	 * This will sort the decorations by managed state (unmanaged
	 * widgets go at the end) and by referenced pane--decorations
	 * of the same pane go together.
	 */
	qsort (
		(XtPointer)ListOfDecorations(w),
		PANES_P(w).num_decorations, sizeof(Widget),
		(int (*) OL_ARGS((const void *, const void *)))SubOrder
	);

	/*
	 * Since decorations of the same pane are now together,
	 * we merely have to assign the pane's list pointer to the
	 * first of its decorations and set the pane's decoration
	 * count.
	 */
	last_ref = 0;
	FOR_EACH_DECORATION (w, decoration, n) {
		if (!XtIsManaged(decoration))
			break;

		ref = ResolveReference(w, decoration);
		if (!ref) {
			/*
			 * Oh dear, the client has created a decoration
			 * before creating the referenced pane. This isn't
			 * allowed, but no real harm is done. All we have
			 * to do is clear the managed state--it's not
			 * necessary to re-sort the list.
			 *
			 * Note: It typically isn't a good idea to just
			 * clear the managed state, one also has to deal
			 * with the mapped_when_managed flag and with
			 * clearing RectObjs. But the client has erred,
			 * and we've told the user something is wrong.
			 * Let the client developer fix the problem.
			 * All we're trying to do is keep running as long
			 * as possible.
			 */
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				"noPane", "createSubordinate",
				OleCOlToolkitWarning,
				"Widget %s: Decoration widget (%s) has no reference",
				XtName(w), XtName(decoration)
			);
			CORE_P(decoration).managed = False;
			continue;
		}

		if (ref != last_ref) {
			last_ref = ref;
			OlPanesFindNode (w, ref, &node, &parent);
			if (!node) {
				/*
				 * Oh dear, the client has tried to
				 * manage a decoration without having
				 * managed the pane. This could happen
				 * when the client creates the decorations
				 * before it creates the pane. Sorry, this
				 * is not allowed. As above, we just need
				 * to unmanage it.
				 */
				OlVaDisplayWarningMsg (
					XtDisplay(w),
					"decorateTooEarly", "createPane",
					OleCOlToolkitWarning,
					"Widget %s: Decoration (%s) managed, but pane (%s) unmanaged",
					XtName(w), XtName(decoration), XtName(ref)
				);
				CORE_P(decoration).managed = False;
				last_ref = 0;
				continue;
			}
			PN_LEAF(node).decorations = &ListOfDecorations(w)[n];
			PN_LEAF(node).num_decorations = 0;
		}
		PN_LEAF(node).num_decorations++;
	}

	return;
} /* SortAndAssignDecorations */

/**
 ** SubOrder()
 **/

static int
#if	OlNeedFunctionPrototypes
SubOrder (
	Widget *		w1,
	Widget *		w2
)
#else
SubOrder (w1, w2)
	Widget *		w1;
	Widget *		w2;
#endif
{
	Widget			parent = XtParent(*w1); /* or w2 */
	Widget			ref1;
	Widget			ref2;


#define	W1_BEFORE_W2	-1
#define	W2_BEFORE_W1	1
#define	NO_ORDER	0

	if (XtIsManaged(*w1) && !XtIsManaged(*w2))
		return (W1_BEFORE_W2);
	if (!XtIsManaged(*w1) && XtIsManaged(*w2))
		return (W2_BEFORE_W1);
	if (!XtIsManaged(*w1) && !XtIsManaged(*w2))
		return (NO_ORDER);
	/*
	 * At this point, both must be managed.
	 */

	ref1 = ResolveReference(parent, *w1);
	ref2 = ResolveReference(parent, *w2);

	/*
	 * Impose an arbitrary but consistent order on decorations
	 * of different panes. This is sufficient to collate the
	 * decorations of like panes together.
	 */
	if (ref1 < ref2)
		return (W1_BEFORE_W2);
	if (ref2 < ref1)
		return (W2_BEFORE_W1);

#if	defined(CAN_CHAIN_DECORATION_REFERENCES)
	/*
	 * These two widgets decorate the same pane. We allow decorations
	 * to reference other decorations; ResolveReference followed the
	 * chain of references to the pane, thus ref1 and ref2 are pane
	 * widgets. Now we need to compare the immediate references.
	 */
	if (PANES_CP(*w1).ref_widget == *w2)
		return (W1_BEFORE_W2);
	else if (PANES_CP(*w2).ref_widget == *w1)
		return (W2_BEFORE_W1);
#endif

	return (NO_ORDER);

#undef	W1_BEFORE_W2
#undef	W2_BEFORE_W1
#undef	NO_ORDER
} /* SubOrder */

/**
 ** FixUpPaneDecorationLists()
 **/

static void
#if	OlNeedFunctionPrototypes
FixUpPaneDecorationLists (
	Widget			w,
	Cardinal		offset
)
#else
FixUpPaneDecorationLists (w, offset)
	Widget			w;
	Cardinal		offset;
#endif
{
	/*
	 * Whenever a child is inserted or deleted the composite's
	 * .children list may be relocated. Since each leaf node points
	 * into this list for its decorations, we have to reset the
	 * pointers.
	 */
	OlPanesWalkTree (w, OlPanesLeafNode, NoOrder, FixList, (XtPointer)offset);
	return;
} /* FixUpPaneDecorationLists */

/**
 ** FixList()
 **/

/*ARGSUSED*/
static Boolean
#if	OlNeedFunctionPrototypes
FixList (
	Widget			w,		/*NOTUSED*/
	PanesNode *		node,
	PanesNode *		parent,		/*NOTUSED*/
	Cardinal		n,		/*NOTUSED*/
	OlPanesWalkType		walk,		/*NOTUSED*/
	XtPointer		client_data
)
#else
FixList (w, node, parent, n, walk, client_data)
	Widget			w;
	PanesNode *		node;
	PanesNode *		parent;
	Cardinal		n;
	OlPanesWalkType		walk;
	XtPointer		client_data;
#endif
{
	/*
	 * offset = old_decorations - new_decorations
	 * new_decorations = old_decorations - offset
	 */
	if (PN_LEAF(node).decorations)
		PN_LEAF(node).decorations -= (Cardinal)client_data;
	return (True);
} /* FixList */

/**
 ** CheckConstraintResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckConstraintResources (
	Widget			new,
	Widget			current,
	ArgList			args,
	Cardinal *		num_args
)
#else
CheckConstraintResources (new, current, args, num_args)
	Widget			new;
	Widget			current;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Arg *			arg;
	Cardinal		n;

#define SET_OR_RESET(NEW,CURRENT,FIELD,DEFAULT) \
	if (CURRENT)							\
		PANES_CP(NEW).FIELD = PANES_CP(CURRENT).FIELD;		\
	else								\
		PANES_CP(NEW).FIELD = DEFAULT

	switch (PANES_CP(new).position) {
	case OL_LEFT:
	case OL_TOP:
	case OL_RIGHT:
	case OL_BOTTOM:
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName(new), XtNrefPosition
		);
		SET_OR_RESET (new, current, position, OL_BOTTOM);
		break;
	}

	switch (PANES_CP(new).type) {
	case OL_PANE:
	case OL_HANDLES:
	case OL_DECORATION:
	case OL_CONSTRAINING_DECORATION:
	case OL_SPANNING_DECORATION:
	case OL_OTHER:
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"illegalValue", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName(new), XtNpaneType
		);
		SET_OR_RESET (new, current, type, OL_PANE);
		break;
	}

	switch (PANES_CP(new).gravity) {
	case AllGravity:
	case NorthSouthEastWestGravity:
	case SouthEastWestGravity:
	case NorthEastWestGravity:
	case NorthSouthWestGravity:
	case NorthSouthEastGravity:
	case EastWestGravity:
	case NorthSouthGravity:
	case NorthGravity:
	case SouthGravity:
	case WestGravity:
	case NorthWestGravity:
	case SouthWestGravity:
	case EastGravity:
	case NorthEastGravity:
	case SouthEastGravity:
	case CenterGravity:
		break;
	default:
/*
... Generate suite of common warning/error routines.
 */
		OlVaDisplayWarningMsg (
			XtDisplayOfObject(new),
			"illegalValue", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName(new), XtNpaneGravity
		);
		SET_OR_RESET (new, current, gravity, AllGravity);
		break;
	}

	for (n = 0, arg = args; n < *num_args; n++, arg++)
		if (
			STREQU(arg->name, XtNgeometry)
		     || STREQU(arg->name, XtNdecorations)
		     || STREQU(arg->name, XtNnumDecorations)
		)
			OlVaDisplayWarningMsg (
				XtDisplayOfObject(new),
				"getOnly", "set",
				OleCOlToolkitWarning,
				"Widget %s: XtN%s resource may not be set, only fetched",
				XtName(new), arg->name
			);

#undef	SET_OR_RESET
	return;
} /* CheckConstraintResources */

/**
 ** SetDefaultTypeResource()
 **/

/*ARGSUSED*/
static void
#if	OlNeedFunctionPrototypes
SetDefaultTypeResource (
	Widget			w,
	int			offset,		/*NOTUSED*/
	XrmValue *		value
)
#else
SetDefaultTypeResource (w, offset, value)
	Widget			w;
	int			offset;
	XrmValue *		value;
#endif
{
	static OlDefine		type;

	type = OlIsHandles(w)? OL_HANDLES : OL_PANE;
	value->addr = (XtPointer)&type;
	return;
} /* SetDefaultTypeResource */

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
	int diff = (int)a - (int)b;
	return (diff > 0? (Dimension)diff : 0);
} /* Subtract */

/**
 ** Space()
 **/

static Dimension
#if	OlNeedFunctionPrototypes
Space (
	Dimension		base,
	Position		space
)
#else
Space (base, space)
	Dimension		base;
	Dimension		space;
#endif
{
	/*
	 * We need to be careful of underflow with large negative spaces.
	 */
	int sum = (int)base + (int)space;
	return (sum > 0? (Dimension)sum : 0);
} /* Space */
