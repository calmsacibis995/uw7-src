#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ComboGizmP.h	1.1"
#endif

#ifndef _ComboGizmP_h
#define _ComboGizmP_h

#include "ComboGizmo.h"

typedef struct	_ComboBoxGizmoP {
	char *		name;
	char **		items;
	Widget		comboBox;
	char *		initial;
	char *		current;
	char *		previous;
} ComboBoxGizmoP;

#endif /* _ComboGizmP_h */
