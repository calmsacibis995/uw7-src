#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SpaceGizmo.h	1.1"
#endif

#ifndef _SpaceGizmo_h
#define _SpaceGizmo_h

#include "Gizmo.h"

typedef struct _SpaceGizmo {
	char *		name;
	Dimension	height;		/* In millimeters */
	Dimension	width;		/* In millimeters */
} SpaceGizmo;

extern GizmoClassRec SpaceGizmoClass[];

#endif /* _SpaceGizmo_h */
