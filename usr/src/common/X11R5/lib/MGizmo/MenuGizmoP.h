#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MenuGizmoP.h	1.6"
#endif

#ifndef _MenuGizmoP_h
#define _MenuGizmoP_h

#include "MenuGizmo.h"

#define TIGHTNESS	20

typedef struct _MenuItemsP {
	Widget			button;	/* Widget id for this button */
	struct _MenuGizmoP *	subMenu;
	MenuGizmoCallbackStruct *	cd;	/* Client data */
} MenuItemsP;

typedef struct _MenuGizmoP {
	char *			name;		/* Name of Gizmo */
	Widget			menu;		/* Menus widget id */
	int			numItems;	/* Number of items */
	struct _MenuItemsP *	items;		/* List of button ids */
	Cardinal		defaultItem;	/* Default item index */
	Cardinal		cancelItem;	/* Cancel item index */
	int			count;		/* Use count for SetSubMenu */
} MenuGizmoP;

extern MenuGizmoP *	_CreateActionMenu(
				Widget, MenuGizmo *, ArgList, int,
				DmMnemonicInfo *, Cardinal *
			);
extern MenuGizmoP *	_CreatePushButtons(Widget, MenuGizmo *, ArgList, int);
extern void		_SetMenuDefault(Gizmo, Widget);
#endif _MenuGizmoP_h
