#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:LabelGizmo.h	1.2"
#endif

#ifndef _LabelGizmo_h
#define _LabelGizmo_h

typedef enum {
	G_LEFT_LABEL,
	G_RIGHT_LABEL,
	G_TOP_LABEL,
	G_BOTTOM_LABEL,
} LabelPosition;

typedef struct	_LabelGizmo {
	HelpInfo *	help;
	char *		name;
	char *		label;
	Boolean		dontAlignLabel;
	GizmoArray	gizmos;
	int		numGizmos;
	LabelPosition	labelPosition;
} LabelGizmo;

/* globals... */

extern GizmoClassRec	LabelGizmoClass[];

#endif /* _LabelGizmo_h */
