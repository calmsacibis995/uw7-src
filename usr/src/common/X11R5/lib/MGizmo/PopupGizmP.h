#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:PopupGizmP.h	1.3"
#endif

#ifndef _PopupGizmP_h
#define _PopupGizmP_h

#include "PopupGizmo.h"

typedef struct _PopupGizmoP {
	char *		name;		/* name of the shell */
	Gizmo		menu;		/* Pointer to menu info */
	GizmoArray	gizmos;		/* the gizmo list */
	int		numGizmos;	/* number of gizmos */
	Widget		shell;		/* Popup shell */
	Widget		workArea;	/* rowcol for client use */
	Widget		rowColumn;	/* immed. child of shell */
	MsgGizmoP *	footer;		/* Base window footer */
	int		separatorType;	/* separator type */
} PopupGizmoP;

#endif /* _PopupGizmP_h */
