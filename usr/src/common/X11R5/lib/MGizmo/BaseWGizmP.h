#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:BaseWGizmP.h	1.1"
#endif

#ifndef _BaseWGizmP_h
#define _BaseWGizmP_h

#include "MenuGizmoP.h"
#include "MsgGizmoP.h"
#include "BaseWGizmo.h"

typedef struct _BaseWindowGizmoP {
	char *		name;		/* Name of shell and name of gizmo */
	MenuGizmoP *	menu;		/* Copy of menu structure */
	GizmoRec *	gizmos;		/* Copy of gizmos */
	MsgGizmoP *	footer;		/* Footer message area */
	int		numGizmos;	/* Number of gizmos */
	Widget		iconShell;	/* Icon shell widget */
	Widget		shell;		/* Shell widget */
	Widget		work;		/* Work area of base window */
} BaseWindowGizmoP;

#endif /* _BaseWGizmP_h */
