#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:BaseWGizmo.h	1.2"
#endif

#ifndef _BaseWGizmo_h
#define _BaseWGizmo_h

typedef struct _BaseWindowGizmo {
	HelpInfo *	help;		/* Help info */
	char *		name;		/* Name of shell and name of gizmo */
	char *		title;		/* Title of base window */
	MenuGizmo *	menu;		/* Menubar */
	GizmoArray	gizmos;		/* List of gizmos */
	int		numGizmos;	/* Number of gizmos in list */
	MsgGizmo *	footer;		/* Base window footer */
	char *		iconName;	/* Name to display when iconified */
	char *		iconPixmap;	/* File containing icon pixmap */
} BaseWindowGizmo;

extern GizmoClassRec	BaseWindowGizmoClass[];

extern Widget		GetBaseWindowIconShell(Gizmo);
extern Widget		GetBaseWindowShell(Gizmo);
extern Gizmo		GetBaseWindowMenuBar(Gizmo);
extern void		SetBaseWindowTitle(Gizmo, char *);
extern void		SetBaseWindowLeftMsg(Gizmo, char *);
extern void		SetBaseWindowRightMsg(Gizmo, char *);
extern void		SetBaseWindowGizmoPixmap(Gizmo, char *);
extern void		SetBaseWindowIconName(Gizmo, char *);

#endif /* _BaseWGizmo_h */
