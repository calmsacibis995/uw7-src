#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:SpaceGizmo.h	1.4"
#endif

#ifndef _SpaceGizmo_h
#define _SpaceGizmo_h

typedef struct _SpaceGizmo {
      Dimension height;        /* In millimeters */
      Dimension width;         /* In millimeters */
      Widget    rectObj;
} SpaceGizmo;

extern GizmoClassRec SpaceGizmoClass[];

#endif /* _SpaceGizmo_h */
