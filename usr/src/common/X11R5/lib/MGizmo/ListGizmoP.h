#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ListGizmoP.h	1.1"
#endif

#ifndef _ListGizmoP_h
#define _ListGizmoP_h

#include "ListGizmo.h"

typedef struct	_ListGizmoP {
	char *		name;
	Widget		listWidget;
} ListGizmoP;

extern GizmoClassRec	ListGizmoClass[];

#endif /* _ListGizmoP_h */
