#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ModalGizmo.h	1.3"
#endif

#ifndef _ModalGizmo_h
#define _ModalGizmo_h

typedef struct _ModalGizmo {
	HelpInfo *	help;		/* Help information */
	char *		name;		/* Name of the shell */
	char *		title;		/* Title of the window */
	MenuGizmo *	menu;		/* Pointer to menu info */
	char *		message;	/* Message */
	GizmoArray	gizmos;		/* Gizmos for the upper area */
	int		numGizmos;	/* Number of gizmos */
	int		style;	/* XmNdialogStyle: */
				/* XmDIALOG_MODELESS */
				/* XmDIALOG_PRIMARY_APPLICATION_MODAL */
				/* XmDIALOG_FULL_APPLICATION_MODAL */
				/* XmDIALOG_SYSTEM_MODAL */
	int		type;	/* XmNdialogType: */
				/* XmDIALOG_TEMPLATE */
				/* XmDIALOG_ERROR */
				/* XmDIALOG_INFORMATION */
				/* XmDIALOG_MESSAGE */
				/* XmDIALOG_QUESTION */
				/* XmDIALOG_WARNING */
				/* XmDIALOG_WORKING */
	Boolean		autoUnmanage;	/* autoManage */
	Boolean		isHelp;	/* This variable is used to */
				/* indicate, in a two button case, that */
				/* the last button is a help button and */
				/* not a cancel button. */
} ModalGizmo;


extern GizmoClassRec ModalGizmoClass[];

extern Widget	GetModalGizmoShell(Gizmo);
extern Widget	GetModalGizmoDialogBox(Gizmo);
extern void	SetModalGizmoMessage(Gizmo, char *);

#endif /* _ModalGizmo_h */
