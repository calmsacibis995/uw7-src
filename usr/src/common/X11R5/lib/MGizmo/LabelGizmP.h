#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:LabelGizmP.h	1.3"
#endif

#ifndef _LabelGizmP_h
#define _LabelGizmP_h

#include "LabelGizmo.h"

typedef struct	_LabelGizmoP {
	char *		name;
	Boolean		dontAlignLabel;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		label;
	Widget		form;
	LabelPosition	labelPosition; /* default is G_LEFT_LABEL */
} LabelGizmoP;

#endif /* _LabelGizmP_h */
