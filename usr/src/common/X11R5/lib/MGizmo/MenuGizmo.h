#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MenuGizmo.h	1.7"
#endif

#ifndef _MenuGizmo_h
#define _MenuGizmo_h

typedef enum {
	I_SEPARATOR_0_LINE = XmNO_LINE,
	I_SEPARATOR_1_LINE,	/* Single line */
	I_SEPARATOR_2_LINE,	/* Double line */
	I_SEPARATOR_1_DASH,	/* Single dashed line */
	I_SEPARATOR_2_DASH,	/* Double dashed line */
	I_SEPARATOR_ETCHED_IN,	/* Shadow etched in */
	I_SEPARATOR_ETCHED_OUT,	/* Shadow etched out */
	I_SEPARATOR_DASHED_IN,	/* Shadow etched in dashed */
	I_SEPARATOR_DASHED_OUT,	/* Shadow etched out dashed */
	I_ARROW_UP_BUTTON,
	I_ARROW_DOWN_BUTTON,
	I_ARROW_LEFT_BUTTON,
	I_ARROW_RIGHT_BUTTON,
	I_PUSH_BUTTON,
	I_RADIO_BUTTON,
	I_TOGGLE_BUTTON,
	I_PIXMAP_BUTTON
} ItemType;

#define I_SEPARATOR	I_SEPARATOR_1_LINE

typedef struct _MenuItems {
	XtArgVal		sensitive;	/* Sensitivity of button */
	char *			label;		/* Button label/pixmap file */
	char *			mnemonic;	/* Button mnemonic */
	ItemType		type;		/* Type button */
	struct _MenuGizmo *	subMenu;
	void			(*callback)();	/* SelectCB */
	XtPointer		clientData;	/* client data */
	XtArgVal		set;		/* Button state */
} MenuItems;

typedef struct _MenuGizmo {
	HelpInfo *		help;		/* Help information */
	char *			name;		/* Name of menu gizmo */
	char *			title;		/* Title of popup menu */
	MenuItems *		items;		/* Menu items */
	void			(*callback)();	/* SelectCB */
	XtPointer		clientData;
	uchar_t			layoutType;	/* XmVERTICAL, XmHORIZONTAL */
	short			numColumns;	/* Number of rows or columns */
	Cardinal		defaultItem;	/* The item with focus */
	Cardinal		cancelItem;	/* Item activating cancel */
} MenuGizmo;

typedef struct _MenuGizmoCallbackStruct {
	int		index;
	XtPointer	clientData;
} MenuGizmoCallbackStruct;

extern GizmoClassRec	PulldownMenuGizmoClass[];
extern GizmoClassRec	MenuBarGizmoClass[];
extern GizmoClassRec	PopupMenuGizmoClass[];
extern GizmoClassRec	CommandMenuGizmoClass[];
extern GizmoClassRec	OptionMenuGizmoClass[];

extern Widget		GetMenu(Gizmo);
extern void		SetSubMenuValue (Gizmo, Gizmo, int);

#endif _MenuGizmo_h
