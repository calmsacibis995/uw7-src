#ifndef	NOIDENT
#ident	"@(#)panes:PanesP.h	1.9"
#endif

#ifndef _PANESP_H
#define _PANESP_H

#include "Xol/ManagerP.h"
#include "Xol/Panes.h"
#include "Xol/LayoutExtP.h"
#include "Xol/HandlesExP.h"

/*
 * Node record:
 */

typedef enum OlPanesNodeType {
	OlPanesTreeNode,
	OlPanesLeafNode,
	OlPanesAnyNode
}			OlPanesNodeType;

typedef struct OlPanesNodeAny {
	OlPanesNodeType		type;
	XRectangle		geometry;
	Position		space;
	short			weight;
}			OlPanesNodeAny;

typedef struct OlPanesNodeLeaf {
	OlPanesNodeType		type;
	XRectangle		geometry; /* aggregate for all subnodes */
	Position		space;
	short			weight;
	/*------------------------------*/
	Widget			w;
	Widget *		decorations;
	Cardinal		num_decorations;
}			OlPanesNodeLeaf;

typedef struct OlPanesNodeTree {
	OlPanesNodeType		type;
	XRectangle		geometry; /* agg. panes + decorations */
	Position		space;
	short			weight;
	/*------------------------------*/
	Cardinal		num_nodes;
	OlDefine		orientation;
}			OlPanesNodeTree;

typedef union PanesNodePart {
	OlPanesNodeAny		any;
	OlPanesNodeLeaf		leaf;
	OlPanesNodeTree		tree;
}			PanesNodePart;

typedef struct _PanesNode {
	PanesNodePart		panes;
}			PanesNode;

#define PN_ANY(N)  (N)->panes.any
#define PN_TREE(N) (N)->panes.tree
#define PN_LEAF(N) (N)->panes.leaf

#define FOR_EACH_PANE_DECORATION(NODE,W,N) \
	for (N = 0; N < PN_LEAF(NODE).num_decorations			\
		&& (W = PN_LEAF(NODE).decorations[N]); N++)

/*
 * Partition state record:
 */

typedef struct PanesPartitionState {
	int			weight;
	int			delta;
	int			partition;
	int			residual;
	struct available {
		Dimension		width;
		Dimension		height;
	}			available;
	OlDefine		orientation;
}			PanesPartitionState;

/*
 * Class record:
 */

typedef void		(*OlPanesStealProc) OL_ARGS((
	Widget			w,
	PanesNode *		new_node,
	PanesNode *		old_node
));
typedef void		(*OlPanesNodeProc) OL_ARGS((
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent
));
typedef void		(*OlPanesPartitionInit) OL_ARGS((
	Widget			w,
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	current_geometry,
	XtWidgetGeometry *	new_geometry
));
typedef void		(*OlPanesPartitionProc) OL_ARGS((
	Widget			w,
	XtPointer		state,
	PanesNode *		node,
	XtWidgetGeometry *	partition
));
typedef void		(*OlPanesPartitionDestroyProc) OL_ARGS((
	Widget			w,
	XtPointer		state
));
typedef void		(*OlPanesGeometryProc) OL_ARGS((
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	geometry
));
typedef XtGeometryResult (*OlPanesConfigureProc) OL_ARGS((
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	allocated,
	XtWidgetGeometry *	preferred,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
typedef void		(*OlPanesAccumulateSizeProc) OL_ARGS((
	Widget			w,
	Boolean			cached_best_fit_ok_hint,
	Boolean			query_only,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result
));

typedef struct _PanesClassPart {
	/*
	 * Public:
	 */
	Cardinal			node_size;
	OlPanesNodeProc			node_initialize;
	OlPanesNodeProc			node_destroy;
	Cardinal			state_size;
	OlPanesPartitionInit		partition_initialize;
	OlPanesPartitionProc		partition;
	OlPanesPartitionProc		partition_accept;
	OlPanesPartitionDestroyProc	partition_destroy;
	OlPanesStealProc		steal_geometry;
	OlPanesNodeProc			recover_geometry;
	OlPanesGeometryProc		pane_geometry;
	OlPanesConfigureProc		configure_pane;
	OlPanesAccumulateSizeProc	accumulate_size;
	XtPointer			extension;

	/*
	 * Private:
	 */
}			PanesClassPart;

typedef struct _PanesClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	PanesClassPart		panes_class;
}			PanesClassRec;

extern PanesClassRec	panesClassRec;

#define PANES_C(WC) ((PanesWidgetClass)(WC))->panes_class

#define XtInheritNodeSize		((Cardinal)(~0))
#define XtInheritPartitionStateSize	((Cardinal)(~0))
#define XtInheritPartitionInitialize	((OlPanesPartitionInit)_XtInherit)
#define XtInheritPartition		((OlPanesPartitionProc)_XtInherit)
#define XtInheritPartitionAccept	((OlPanesPartitionProc)_XtInherit)
#define XtInheritPartitionDestroy	((OlPanesPartitionDestroyProc)_XtInherit)
#define XtInheritStealGeometry		((OlPanesStealProc)_XtInherit)
#define XtInheritRecoverGeometry	((OlPanesNodeProc)_XtInherit)
#define XtInheritPaneGeometry		((OlPanesGeometryProc)_XtInherit)
#define XtInheritConfigurePane		((OlPanesConfigureProc)_XtInherit)
#define XtInheritAccumulateSize		((OlPanesAccumulateSizeProc)_XtInherit)

/*
 * Instance record:
 */
	
typedef struct _PanesPart {
	/*
	 * Public:
	 */
	OlLayoutResources	layout;
	Widget *		pane_order;
	XtCallbackList		query_geometry;
	Dimension		left_margin;
	Dimension		right_margin;
	Dimension		top_margin;
	Dimension		bottom_margin;
	Boolean			set_min_hints;

	/*
	 * Private:
	 */
	PanesNode *		nodes;
	Cardinal		num_nodes;
	Cardinal		num_panes;
	Cardinal		num_decorations;
	XtOrderProc		insert_position;
}			PanesPart;

typedef struct _PanesRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	PanesPart		panes;
}			PanesRec;

#define PANES_P(W) ((PanesWidget)(W))->panes

#define FOR_EACH_PANE(PW,W,N) \
	for (N = 0; N < PANES_P(PW).num_panes				\
		&& (W = COMPOSITE_P(PW).children[N]); N++)

#define FOR_EACH_MANAGED_PANE(PW,W,N) \
	FOR_EACH_PANE (PW, W, N)					\
		if (XtIsManaged(W))

#define ListOfDecorations(W) \
	(COMPOSITE_P(W).children + PANES_P(W).num_panes)

#define FOR_EACH_DECORATION(PW,W,N) \
	for (N = 0; N < PANES_P(PW).num_decorations			\
		 && (W = ListOfDecorations(PW)[N]); N++)

/*
 * Constraint record:
 */
typedef struct	_PanesConstraintPart {
	/*
	 * Public:
	 */
	String			ref_name;
	Widget			ref_widget;
	int			gravity;
	OlDefine		type;
	OlDefine		position;
	Position		space;
	short			weight;

	/*
	 * Private:
	 */
}			PanesConstraintPart;

typedef struct _PanesConstraintRec {
	PanesConstraintPart	panes;
}			PanesConstraintRec;

#define PANES_CP(W) ((PanesConstraintRec *)(W)->core.constraints)->panes

/*
 * Public types:
 */

typedef unsigned int	OlPanesWalkType;
#define NoOrder		0x0000
#define PreOrder	0x0001
#define PostOrder	0x0002

typedef Boolean		(*OlPanesWalkProc) OL_ARGS((
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent,
	Cardinal		n,
	OlPanesWalkType		walk,
	XtPointer		client_data
));

/*
 * Public routines:
 */

OLBeginFunctionPrototypeBlock

extern void
OlPanesWalkTree OL_ARGS((
	Widget			w,
	OlPanesNodeType		type,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
));
extern Boolean
OlPanesWalkChildren OL_ARGS((
	Widget			w,
	PanesNode *		parent,
	OlPanesNodeType		type,
	OlPanesWalkProc		func,
	XtPointer		client_data
));
extern void
OlPanesFindNode OL_ARGS((
	Widget			w,
	Widget			child,
	PanesNode **		node,
	PanesNode **		parent
));
extern void
OlPanesFindSiblings OL_ARGS((
	Widget			w,
	PanesNode *		node,
	PanesNode *		parent,
	PanesNode **		fore,
	PanesNode **		aft
));
extern PanesNode *
OlPanesFindParent OL_ARGS((
	Widget			w,
	PanesNode *		node
));
extern void
OlPanesWalkAncestors OL_ARGS((
	Widget			w,
	PanesNode *		node,
	OlPanesWalkType		walk,
	OlPanesWalkProc		func,
	XtPointer		client_data
));
extern Boolean
OlPanesChangeConstraints OL_ARGS((
	Widget			/* new */,
	Widget			current
));
extern void
OlPanesReconstructTree OL_ARGS((
	Widget			w
));
extern void
OlPanesQueryGeometry OL_ARGS((
	Widget			w,
	PanesNode *		node,
	XtWidgetGeometry *	available,
	XtWidgetGeometry *	aggregate,
	XtWidgetGeometry *	allocated
));
extern void
OlPanesLeafGeometry OL_ARGS((
	Widget			w,
	PanesNode *		node,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	result,
	XtWidgetGeometry *	pane
));

OLEndFunctionPrototypeBlock

/*
 * Debugging use:
 */
extern Cardinal		OlPanesTreeLevel;

#endif
