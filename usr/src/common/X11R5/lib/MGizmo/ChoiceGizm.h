#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ChoiceGizm.h	1.1"
#endif

#ifndef _ChoiceGizmo_h
#define _ChoiceGizmo_h

typedef enum {
	G_TOGGLE_BOX,
	G_RADIO_BOX,
	G_OPTION_BOX
} ChoiceType;

typedef struct _ChoiceGizmo {
	HelpInfo *	help;		/* Help information */
	char *		name;		/* Name of menu gizmo */
	MenuGizmo *	menu;		/* Choice buttons */
	ChoiceType	type;		/* Type of menu created */
} ChoiceGizmo;

extern GizmoClassRec	ChoiceGizmoClass[];

#endif _ChoiceGizmo_h
