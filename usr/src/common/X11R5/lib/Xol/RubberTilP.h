#ifndef	NOIDENT
#ident	"@(#)rubbertile:RubberTilP.h	2.1"
#endif

#ifndef _RUBBERTILEP_H
#define _RUBBERTILEP_H

#include "Xol/PanesP.h"
#include "Xol/RubberTile.h"

/*
 * Class record:
 */

typedef struct _RubberTileClassPart {
	/*
	 * Public:
	 */
	XtPointer		extension;

	/*
	 * Private:
	 */
}			RubberTileClassPart;

typedef struct _RubberTileClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	PanesClassPart		panes;
	RubberTileClassPart	rubber_tile_class;
}			RubberTileClassRec;

extern RubberTileClassRec	rubberTileClassRec;

/*
 * Instance record:
 */
	
typedef struct _RubberTilePart {
	/*
	 * Public:
	 */
	OlDefine		orientation;

	/*
	 * Private:
	 */
}			RubberTilePart;

typedef struct _RubberTileRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	PanesPart		panes;
	RubberTilePart		rubber_tile;
}			RubberTileRec;

#define RUBBERTILE_P(W) ((RubberTileWidget)(W))->rubber_tile

/*
 * Constraint record:
 */

typedef struct	_RubberTileConstraintPart {
	/*
	 * Public:
	 */

	/*
	 * Private:
	 */
	XtPointer		unused;
}			RubberTileConstraintPart;

typedef struct _RubberTileConstraintRec {
	PanesConstraintPart		panes;
	RubberTileConstraintPart	rubber_tile;
}			RubberTileConstraintRec;

#define RUBBERTILE_CP(W) (*(RubberTileConstraintRec **)&((W)->core.constraints))->rubber_tile

#endif
