#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:NumericGP.h	1.2"
#endif

#ifndef _NumericGP_h
#define _NumericGP_h

#include "NumericGiz.h"

typedef struct	_NumericGizmoP {
	char *		name;
	Widget		spinBox;
	int		initial;
	int		current;
	int		previous;
} NumericGizmoP;

#endif /* _NumericGP_h */
