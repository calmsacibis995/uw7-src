#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ListGizmo.h	1.1"
#endif

#ifndef _ListGizmo_h
#define _ListGizmo_h

typedef struct	_ListGizmo {
	HelpInfo *	help;
	char *		name;
	char **		items;
	int		numItems;
	int		visible;	/* Number of items visible in list */
	void		(*callback)();
	XtPointer	clientData;
} ListGizmo;

extern GizmoClassRec	ListGizmoClass[];

#endif /* _ListGizmo_h */
