#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:InputGizmo.h	1.3"
#endif

#ifndef _InputGizmo_h
#define _InputGizmo_h

typedef struct	_InputGizmo {
	HelpInfo *	help;
	char *		name;
	char *		text;
	int		width;
	void		(*callback)();
	XtPointer	client_data;
} InputGizmo;

/* globals... */

extern GizmoClassRec	InputGizmoClass[];

/* function prototypes... */

extern void	SetInputGizmoText(Gizmo, char *);
extern char *	GetInputGizmoText(Gizmo);

#endif /* _InputGizmo_h */
