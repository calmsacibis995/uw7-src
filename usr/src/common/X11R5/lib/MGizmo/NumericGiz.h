#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:NumericGiz.h	1.2"
#endif

#ifndef _NumericGizmo_h
#define _NumericGizmo_h

typedef struct	_NumericGizmo {
	HelpInfo *	help;
	char *		name;
	int		value;
	int		min;
	int		max;
	int		inc;
	int		radix;
} NumericGizmo;

/* globals... */

extern GizmoClassRec	NumericGizmoClass[];

/* function prototypes... */

extern void	SetNumericInitialValue(Gizmo, int);
extern void	SetNumericGizmoValue(Gizmo, int);
extern int	GetNumericGizmoValue(Gizmo);

#endif /* _NumericGizmo_h */
