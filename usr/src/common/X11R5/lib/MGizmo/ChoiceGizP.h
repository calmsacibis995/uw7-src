#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ChoiceGizP.h	1.1"
#endif

#ifndef _ChoiceGizP_h
#define _ChoiceGizP_h

#include "ChoiceGizm.h"

typedef struct _ChoiceGizmoP {
	char *		name;		/* Name of Gizmo */
	MenuGizmoP *	menu;		/* List of button widgets */
	ChoiceType	type;		/* G_{TOGGLE,RADIO,OPTION}_BOX */
	XtPointer	initial;
	XtPointer	current;
	XtPointer	previous;
} ChoiceGizmoP;

#endif _ChoiceGizP_h
