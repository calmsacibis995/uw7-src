#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ComboGizmo.h	1.1"
#endif

#ifndef _ComboGizmo_h
#define _ComboGizmo_h

typedef struct	_ComboBoxGizmo {
	HelpInfo *	help;
	char *		name;
	char *		defaultItem;
	char **		items;
	int		numItems;
	int		visible;
} ComboBoxGizmo;

/* globals... */

extern GizmoClassRec	ComboBoxGizmoClass[];

/* function prototypes... */

extern void		SetComboGizmoValue(Gizmo, char *);
extern char *		GetComboGizmoValue(Gizmo);

#endif /* _ComboGizmo_h */
