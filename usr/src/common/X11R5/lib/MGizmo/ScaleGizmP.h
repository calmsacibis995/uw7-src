#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ScaleGizmP.h	1.1"
#endif

#ifndef _ScaleGizmP_h
#define _ScaleGizmP_h

#include "ScaleGizmo.h"

typedef struct	_ScaleGizmoP {
	char *		name;
	Widget		scale;
	int		initial;
	int		current;
	int		previous;
} ScaleGizmoP;

#endif /* _ScaleGizmP_h */
