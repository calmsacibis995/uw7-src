#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:PopupGizmo.h	1.4"
#endif

#ifndef _PopupGizmo_h
#define _PopupGizmo_h

#include "MsgGizmoP.h"

typedef struct _PopupGizmo {
	HelpInfo *	help;	/* help information */
	char *		name;	/* name of the shell */
	char *		title;	/* title (for the wm) */
	MenuGizmo *	menu;	/* Pointer to menu info */
	GizmoArray	gizmos;	/* the gizmo list */
	int		numGizmos;/* number of gizmos */
	MsgGizmo *	footer;	/* footer */
	int		separatorType;	/* separator type */
} PopupGizmo;

extern GizmoClassRec PopupGizmoClass[];

extern void	BringDownPopup(Widget);
extern Widget	GetPopupGizmoShell(Gizmo);
extern Widget	GetPopupGizmoRowCol(Gizmo);
extern void	SetPopupWindowLeftMsg(Gizmo g, char *msg);
extern void	SetPopupWindowRightMsg(Gizmo g, char *msg);

#endif /* _PopupGizmo_h */
