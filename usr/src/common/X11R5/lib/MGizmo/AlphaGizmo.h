#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:AlphaGizmo.h	1.1"
#endif

#ifndef _AlphaGizmo_h
#define _AlphaGizmo_h

typedef struct	_AlphaGizmo {
	HelpInfo *	help;
	char *		name;
	int		defaultItem;
	char **		items;
	int		numItems;
} AlphaGizmo;

/* globals... */

extern GizmoClassRec	AlphaGizmoClass[];

/* function prototypes... */

extern void		SetAlphaGizmoValue(Gizmo, char *);
extern char *		GetAlphaGizmoValue(Gizmo);

#endif /* _AlphaGizmo_h */
