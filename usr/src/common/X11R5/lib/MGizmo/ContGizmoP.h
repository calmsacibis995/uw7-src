#ifndef NOIDENT
#ident	"@(#)MGizmo:ContGizmoP.h	1.1"
#endif

#ifndef _ContGizmoP_h
#define _ContGizmoP_h

#include "ContGizmo.h"

typedef struct _ContainerGizmoP {
	char *		name;
	ContainerType	type;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		w;
} ContainerGizmoP;

#endif /* _ContGizmoP_h */
