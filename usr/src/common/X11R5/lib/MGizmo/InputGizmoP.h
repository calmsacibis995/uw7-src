#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:InputGizmoP.h	1.1"
#endif

#ifndef _InputGizmoP_h
#define _InputGizmoP_h

#include "InputGizmo.h"

typedef struct	_InputGizmoP {
	char *		name;
	Widget		textField;
	char *		initial;
	char *		current;
	char *		previous;
} InputGizmoP;

#endif /* _InputGizmoP_h */
