#ifndef NOIDENT
#pragma ident	"@(#)SWGizmo.h	15.1"
#endif

#ifndef _swgizmo_h
#define _swgizmo_h

typedef struct ScrolledWindowGizmo {
	char *		name;
	int		width;
	int		height;
	Widget		sw;
} ScrolledWindowGizmo;

extern GizmoClassRec	ScrolledWindowGizmoClass[];

#endif /* _swgizmo_h */
