#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:AlphaGizmP.h	1.1"
#endif

#ifndef _AlphaGizmP_h
#define _AlphaGizmP_h

#include "AlphaGizmo.h"

typedef struct	_AlphaGizmoP {
	char *		name;
	char **		items;
	Widget		spinButton;
	char *		initial;
	char *		current;
	char *		previous;
} AlphaGizmoP;

#endif /* _AlphaGizmP_h */
