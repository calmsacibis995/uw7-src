#ifndef NOIDENT
#pragma ident	"@(#)RTileGizmo.h	15.1"
#endif

#ifndef _rubbertilegizmo_h
#define _rubbertilegizmo_h

typedef struct RubberTileGizmo {
	char *		name;
	char *		footer;
	OlDefine	orientation;	/* OL_VERTICAL | OL_HORIZONTAL */
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		rubberTile;
	Widget		message;
} RubberTileGizmo;

extern GizmoClassRec	RubberTileGizmoClass[];

extern Widget		GetRubberTileFooter(RubberTileGizmo *rt);

#endif /* _rubbertilegizmo_h */
