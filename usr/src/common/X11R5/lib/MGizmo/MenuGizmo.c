/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MenuGizmo.c	1.27.1.1"
#endif

#include <stdio.h>

#include <Xm/RowColumn.h>
#include <Xm/MainW.h>
#include <Xm/CascadeBG.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/ToggleBG.h>
#include <Xm/ArrowBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/Form.h>

#include <DtI.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"

static Gizmo		CreateMenuGizmo();
static Gizmo		CreateMenuBar();
static Gizmo		CreatePulldownMenu();
static Gizmo		CreatePopupMenu();
static Gizmo		CreateCommandMenu();
static Gizmo		CreateOptionMenu();
static void		FreeMenu();
static XtPointer	QueryMenu();
static void		DumpMenu();
static Widget		AttachmentWidget();

GizmoClassRec PulldownMenuGizmoClass[] = {
	"MenuBarGizmo",
	CreatePulldownMenu,	/* Create */
	FreeMenu,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpMenu,		/* Dump */
	NULL,			/* Manipulate */
	QueryMenu,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

GizmoClassRec MenuBarGizmoClass[] = {
	"MenuBarGizmo",
	CreateMenuBar,		/* Create */
	FreeMenu,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpMenu,		/* Dump */
	NULL,			/* Manipulate */
	QueryMenu,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

GizmoClassRec PopupMenuGizmoClass[] = {
	"PopupMenuGizmo",
	CreatePopupMenu,	/* Create */
	FreeMenu,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpMenu,		/* Dump */
	NULL,			/* Manipulate */
	QueryMenu,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

GizmoClassRec CommandMenuGizmoClass[] = {
	"CommandMenuGizmo",
	CreateCommandMenu,	/* Create */
	FreeMenu,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpMenu,		/* Dump */
	NULL,			/* Manipulate */
	QueryMenu,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

GizmoClassRec OptionMenuGizmoClass[] = {
	"OptionMenuGizmo",
	CreateOptionMenu,	/* Create */
	FreeMenu,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpMenu,		/* Dump */
	NULL,			/* Manipulate */
	QueryMenu,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeMenu
 */
static void 
FreeMenu(MenuGizmoP *g)
{
	int i;

	if (g == NULL) {
		return;		/* Don't free a nonexistant menu */
	}
	if (g->count-- != 0) {	/* Don't free menu if used in more */
		return;		/* than one place. */
	}
	for (i=0; i<g->numItems; i++) {
		if (g->items[i].subMenu != NULL) {
			FreeMenu(g->items[i].subMenu);
		}
		if (g->items[i].cd != NULL) {
			FREE(g->items[i].cd);
		}
	}

	FREE(g->items);
	FREE(g);
}

/* Given the type of the item (ItemType) return the appropriate WidgetClass.
 * If the type is one of the separator types then also return (in separator)
 * the appropriate resource for XmNseparator.
 */
static WidgetClass
GetClass(ItemType type, int *separator, int *arrow)
{
	WidgetClass	class;

	*separator = XmINVALID_SEPARATOR_TYPE;

	class = xmArrowButtonGadgetClass;
	*arrow = -1;
	switch(type) {
		case I_ARROW_UP_BUTTON: {
			*arrow = XmARROW_UP;
			break;
		}
		case I_ARROW_DOWN_BUTTON: {
			*arrow = XmARROW_DOWN;
			break;
		}
		case I_ARROW_RIGHT_BUTTON: {
			*arrow = XmARROW_RIGHT;
			break;
		}
		case I_ARROW_LEFT_BUTTON: {
			*arrow = XmARROW_LEFT;
			break;
		}
		case I_PIXMAP_BUTTON: {
			class = xmPushButtonWidgetClass;
			break;
		}
		case I_PUSH_BUTTON: {
			class = xmPushButtonGadgetClass;
			break;
		}
		case I_RADIO_BUTTON: {
			class = xmToggleButtonGadgetClass;
			break;
		}
		case I_TOGGLE_BUTTON: {
			class = xmToggleButtonGadgetClass;
			break;
		}
		case I_SEPARATOR_0_LINE:
		case I_SEPARATOR_1_LINE:
		case I_SEPARATOR_2_LINE:
		case I_SEPARATOR_1_DASH:
		case I_SEPARATOR_2_DASH:
		case I_SEPARATOR_ETCHED_IN:
		case I_SEPARATOR_ETCHED_OUT:
		case I_SEPARATOR_DASHED_IN:
		case I_SEPARATOR_DASHED_OUT: {
			*separator = type;
			class = xmSeparatorGadgetClass;
			break;
		}
	}
	return class;
}

static XtCallbackRec cb[] = {
	{(XtCallbackProc)NULL, (XtPointer)NULL},
	{(XtCallbackProc)NULL, (XtPointer)NULL}
};

static MenuGizmoCallbackStruct *
SetCallback(MenuGizmo *old, int i, WidgetClass class, Arg *arg, int *j)
{
	MenuGizmoCallbackStruct *	cd = NULL;

	if (old->items[i].callback != NULL) {
		cb[0].callback = (XtCallbackProc)(old->items[i].callback);
		cb[0].closure = old->items[i].clientData ? 
		    old->items[i].clientData : old->clientData;
	}
	else if (old->callback != NULL) {
		cd = (MenuGizmoCallbackStruct *) MALLOC(
			sizeof(MenuGizmoCallbackStruct)
		);
		cd->index = i;
		cd->clientData = old->items[i].clientData ?
				old->items[i].clientData : old->clientData;
		cb[0].callback = (XtCallbackProc)old->callback;
		cb[0].closure = (XtPointer)cd;
	}
	if (
		class == xmPushButtonWidgetClass ||
		class == xmPushButtonGadgetClass ||
		class == xmArrowButtonGadgetClass 
	) {
		XtSetArg(arg[*j], XmNactivateCallback, cb); (*j)++;
	}
	else if (class == xmCascadeButtonGadgetClass){
		XtSetArg(arg[*j], XmNcascadingCallback, cb); (*j)++;
	}
	else {
		XtSetArg(arg[*j], XmNvalueChangedCallback, cb); (*j)++;
	}

	return cd;
}

static Gizmo
CreateMenuGizmo(Widget parent, Gizmo g, int type)
{
	Pixmap		pixmap;
	Arg		arg[100];
	MenuGizmo *	old = (MenuGizmo *)g;
	MenuGizmoP *	new = (MenuGizmoP *)CALLOC(1, sizeof(MenuGizmoP));
	Gizmo		gp;
	unsigned char *	mnemonic;
	KeySym		ks;
	int		sep;
	int		arrow;
	int		i;
	int		j;
	Widget		menu = NULL;
	Widget		cascade;
	WidgetClass	class;
	Boolean		radio = False;
	XmString	str;
	DmGlyphRec *	dmgp;
	char *		label;

	new->name = old->name;

	if (old->items == NULL) {
		new->items = NULL;
		return((Gizmo)new);
	}
	/* Count size needed for copying the menu items */
	for (i=0; old->items[i].label; i++) {
	}
	new->numItems = i;

	new->items = (MenuItemsP *) CALLOC(i, sizeof(MenuItemsP));

	/* Loop thru the items in this menu and create the buttons */
	for (i=0; old->items[i].label; i++) {
		j = 0;		/* Arg counter */

		/* Create the pull down menu if needed */
		/* type == XmMENU_PULLDOWN, XmMENU_POPUP or XmWORK_AREA */
		if (type == XmMENU_PULLDOWN || type == XmMENU_POPUP) {
			menu = XmCreatePulldownMenu(
				parent, "_pulldown", NULL, 0
			);
			if (old->help != NULL) {
				GizmoRegisterHelp(menu, old->help);
			}
			/* Set the sub menu id only for pull downs. */
			XtSetArg(arg[j], XmNsubMenuId, menu); j++;
		}

		/* If there is a sub menu then the class should be cascade. */
		/* Or if a menu bar is being created :: XmMENU_POPUP. */
		/* Otherwise, the class should be taken from the */
		/* items class. */
		if (old->items[i].subMenu != NULL || type == XmMENU_POPUP) {
			class = xmCascadeButtonGadgetClass;
		}
		else {
			/* Given the item type get the xmClass */
			class = GetClass(old->items[i].type, &sep, &arrow);
			if (old->items[i].type == I_RADIO_BUTTON) {
				XtSetArg(
					arg[j],
					XmNindicatorType, XmONE_OF_MANY
				); j++;
				radio = True;
			}
			/* Only set separator type for separator items */
			if (sep != XmINVALID_SEPARATOR_TYPE) {
				XtSetArg(arg[j], XmNseparatorType, sep); j++;
			}
			/* Only set arrorDirection of arrow items */
			if (arrow != -1) {
				XtSetArg(arg[j], XmNarrowDirection, arrow);j++;
			}
		}

		/* If the item's set field is on then also set the button */
		if (old->items[i].set == True) {
			XtSetArg(arg[j], XmNset, True); j++;
		}
		XtSetArg(arg[j], XmNsensitive, old->items[i].sensitive); j++;

		/* Set the mnemonic for this item */
		mnemonic = (unsigned char *)GGT(old->items[i].mnemonic);
		label = GGT(old->items[i].label);
		if (
			old->items[i].type != I_PIXMAP_BUTTON &&
			mnemonic != NULL &&
			(((ks = XStringToKeysym((char *)mnemonic)) != NoSymbol) ||
			(ks = (KeySym)(mnemonic[0]))) &&
			strchr(label, (char)(ks & 0xff)) != NULL
		) {
			XtSetArg(arg[j], XmNmnemonic, ks); j++;
		}

		/* Set center alignment for the button label */
		XtSetArg(arg[j], XmNalignment, XmALIGNMENT_CENTER); j++;

		/* Add individual callbacks for each button */
		if (old->callback != NULL || old->items[i].callback != NULL) {
			new->items[i].cd = SetCallback(old, i, class, arg, &j);
		}

		if (old->items[i].type == I_PIXMAP_BUTTON) {
			dmgp = DmGetPixmap(
				XtScreen(parent), old->items[i].label
			);
			if(dmgp != NULL) {
				XtSetArg(arg[j], XmNlabelType, XmPIXMAP); j++;
				XtSetArg(arg[j], XmNlabelPixmap, dmgp->pix); j++;
			}
		}

		/* Now create the button for this item */
		new->items[i].button = XtCreateManagedWidget(
			label, class, parent, arg, j
		);
		if (old->help != NULL) {
			GizmoRegisterHelp(new->items[i].button, old->help);
		}
		/* Set menu history for the option menu */
		if (old->items[i].set == True) {
			XtSetArg(arg[0], XmNmenuHistory, new->items[i].button);
			XtSetValues(parent, arg, 1);
		}

		/* Create the cascade that might hang off this button */
		if (old->items[i].subMenu != NULL) {
			gp = CreateMenuGizmo(
				menu, old->items[i].subMenu,
				XmMENU_PULLDOWN
			);
			new->items[i].subMenu = (MenuGizmoP *)gp;
		}
	}
	/* If a radio button was found in this menu then set radio behavior */
	if (radio == True) {
		i = 0;
		XtSetArg(arg[i], XmNradioAlwaysOne, True); i++;
		XtSetArg(arg[i], XmNradioBehavior, True); i++;
		XtSetValues(parent, arg, i);
	}

	/* Set input focus on the default button. */
	new->defaultItem = old->defaultItem;
	new->cancelItem = old->cancelItem;
	return((Gizmo)new);
}

/*
 * CreateMenuBar
 */
static Gizmo
CreateMenuBar(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	Widget		menubar;
	MenuGizmoP *	gp;

	menubar = XmCreateMenuBar(parent, g->name, args, numArgs);
	gp = CreateMenuGizmo(menubar, g, XmMENU_POPUP);
	gp->menu = menubar;
	XtVaSetValues(
		menubar,
		XmNdefaultButton, gp->items[gp->defaultItem].button,
		NULL
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->menu, g->help);
	}
	XtManageChild(menubar);

	return gp;
}

/*
 * Common routine for creating popup and popdown menus
 */
static MenuGizmoP *
CreatePopupPopdown(Widget menu, MenuGizmo *g)
{
	MenuGizmoP *	gp;

	gp = CreateMenuGizmo(menu, g, XmMENU_PULLDOWN);
	gp->menu = menu;
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->menu, g->help);
	}
	return gp;
}

/*
 * CreatePulldownMenu
 */
static Gizmo
CreatePulldownMenu(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	Widget		menu;

	menu = XmCreatePulldownMenu(parent, "_pulldown", args, numArgs);
	return (Gizmo)CreatePopupPopdown(menu, g);
}

/*
 * CreatePopupMenu
 */
static Gizmo
CreatePopupMenu(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	Widget		menu;

	menu = XmCreatePopupMenu(parent, "_popup", args, numArgs);
	return (Gizmo)CreatePopupPopdown(menu, g);
}

/*
 * _CreateActionMenu
 */
MenuGizmoP *
_CreateActionMenu(
	Widget parent, MenuGizmo *g, ArgList args, int numArgs,
	DmMnemonicInfo *mneInfo, Cardinal *numMne
)
{
	int			width;
	XtWidgetGeometry	size;
	Widget			form;
	MenuGizmoP *		gp;
	int			j = 0;
	int			i;
	Arg			arg[20];
	unsigned char *		mnemonic;

	form = XmCreateForm(parent, g->name, args, numArgs);

	gp = (MenuGizmoP *)CreateMenuGizmo(form, g, XmWORK_AREA);
	XtSetArg(
		arg[0], XmNdefaultButton, gp->items[gp->defaultItem].button
	);
	XtSetValues(form, arg, 1);
	gp->menu = form;
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->menu, g->help);
	}
	/* Setup buttons positions and collect mnemonics */
	*mneInfo = (DmMnemonicInfo)CALLOC(
		gp->numItems, sizeof(DmMnemonicInfoRec)
	);
	*numMne = gp->numItems;

	width = 0;
	XtSetArg(arg[0], XmNwidth, &j);
	for (i=0; i<gp->numItems; i++) {
		(*mneInfo)[i].mne = (unsigned char *)GGT(g->items[i].mnemonic);
		(*mneInfo)[i].mne_len = strlen((char *)(*mneInfo)[i].mne);
		(*mneInfo)[i].op = DM_B_MNE_ACTIVATE_BTN | DM_B_MNE_GET_FOCUS;
		(*mneInfo)[i].w = gp->items[i].button;
		XtGetValues(gp->items[i].button, arg, 1);
		width += j;
	}
	XtQueryGeometry(parent, NULL, &size);
	if ((int)size.width > width) {

		j = 0;
		XtSetArg(arg[j], XmNfractionBase, TIGHTNESS*gp->numItems-1);
		j++;
		XtSetArg(arg[j], XmNskipAdjust, True); j++;
		XtSetValues(form, arg, j);

		/* Position the buttons correctly */
		for (i=0; i<gp->numItems; i++) {
			j = 0;
			XtSetArg(
				arg[j], XmNleftAttachment,
				i ? XmATTACH_POSITION : XmATTACH_FORM
			); j++;
			XtSetArg(arg[j], XmNleftPosition, TIGHTNESS*i); j++;
			XtSetArg(
				arg[j], XmNrightAttachment,
				i != gp->numItems-1
				? XmATTACH_POSITION : XmATTACH_FORM
			); j++;
			XtSetArg(
				arg[j], XmNrightPosition,
				TIGHTNESS*i + (TIGHTNESS-1)
			); j++;
			XtSetValues(gp->items[i].button, arg, j);
		}
	}
	else {
		j = 0;
		XtSetArg(arg[j], XmNfractionBase, 2); j++;
		XtSetArg(arg[j], XmNskipAdjust, True); j++;
		XtSetValues(form, arg, j);

		/* Position the buttons correctly */
		for (i=0; i<gp->numItems; i++) {
			j = 0;
			if (i == 0) {
				XtSetArg(
					arg[j], XmNleftAttachment,
					XmATTACH_POSITION
				); j++;
				XtSetArg(arg[j], XmNleftPosition, 1); j++;
				XtSetArg(arg[j], XmNleftOffset, -width/2); j++;
			}
			else {
				XtSetArg(
					arg[j], XmNleftAttachment,
					XmATTACH_WIDGET
				); j++;
				XtSetArg(
					arg[j], XmNleftWidget,
					gp->items[i-1].button
				); j++;
			}
			XtSetValues(gp->items[i].button, arg, j);
		}
	}

	XtManageChild(form);

	return gp;
}

/*
 * _CreatePushButtons
 */
MenuGizmoP *
_CreatePushButtons(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	MenuGizmoP *	gp;
	int		i = 0;

	gp = (MenuGizmoP *)CreateMenuGizmo(parent, g, XmWORK_AREA);
	gp->menu = NULL;

	return gp;
}

/*
 * CreateCommandMenu, toggle, arrow, radio
 */
static Gizmo
CreateCommandMenu(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	Widget		rc;
	MenuGizmoP *	gp;
	int		i = 0;
	Arg		arg[100];
	DmMnemonicInfo	mneInfo;
	Cardinal	numMne;

	XtSetArg(arg[i], XmNorientation, g->layoutType); i++;
	XtSetArg(arg[i], XmNnumColumns, g->numColumns); i++;
	XtSetArg(arg[i], XmNpacking, XmPACK_COLUMN); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	rc = XmCreateRowColumn(parent, g->name, arg, i);
	gp = (MenuGizmoP *)CreateMenuGizmo(rc, g, XmWORK_AREA);
	gp->menu = rc;

	XtSetArg(
		arg[0], XmNdefaultButton, gp->items[gp->defaultItem].button
	);
	XtSetValues(rc, arg, 1);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->menu, g->help);
	}

	/* set up mnemonic info and register mnemonics */
	mneInfo = (DmMnemonicInfo)CALLOC(gp->numItems,
			sizeof(DmMnemonicInfoRec));
	numMne = gp->numItems;

	for (i=0; i<gp->numItems; i++) {
		mneInfo[i].mne = (unsigned char *)GGT(g->items[i].mnemonic);
		mneInfo[i].mne_len = strlen((char *)(mneInfo[i].mne));
		mneInfo[i].op = DM_B_MNE_ACTIVATE_BTN | DM_B_MNE_GET_FOCUS;
		mneInfo[i].w = gp->items[i].button;
	}
	DmRegisterMnemonic(gp->menu, mneInfo, numMne);
	FREE(mneInfo);

	XtManageChild(rc);

	return gp;
}

/*
 * CreateOptionMenu
 */
static Gizmo
CreateOptionMenu(Widget parent, MenuGizmo *g, ArgList args, int numArgs)
{
	Widget		option;
	Widget		pulldown;
	MenuGizmoP *	gp;
	int		i = 0;
	XmString	str;
	Arg		arg[100];

	pulldown = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

	str = XmStringCreateLocalized(GGT(g->title));
	XtSetArg(arg[i], XmNsubMenuId, pulldown); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	option = XmCreateOptionMenu(parent, g->name, arg, i);
	XmStringFree(str);
	gp = (MenuGizmoP *)CreateMenuGizmo(pulldown, g, XmMENU_PULLDOWN);
	gp->menu = option;
	XtSetArg(
		arg[0], XmNdefaultButton, gp->items[gp->defaultItem].button
	);
	XtSetValues(pulldown, arg, 1);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->menu, g->help);
	}
	XtManageChild(option);

	return gp;
}

/*
 * GetMenu
 *
 * Returns menu widget for the menu gizmo
 */
Widget
GetMenu(Gizmo g)
{
	MenuGizmoP *	gp = (MenuGizmoP *)g;

	return gp->menu;
}

/*
 * QueryMenu
 */
static XtPointer
QueryMenu(MenuGizmoP *g, int option, char *str)
{
	int		i;
	XtPointer	value = NULL;
	char		buf[256];
	char *		cp = NULL;
	char *		name;

	if (str && (cp = strchr(str, ':')) != NULL) {
		name = buf;
		i = cp-str;
		strncpy(name, str, i);
		name[i] = '\0';
		i = atoi(cp+1);
	}
	else {
		name = str;
	}
	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				if (cp != NULL) {
					return (XtPointer)(g->items[i].button);
				}
				return (XtPointer)g->menu;
			}
			case GetGizmoGizmo: {
				if (cp != NULL) {
					return (XtPointer)(g->items[i].subMenu);
				}
				return (XtPointer)g;
			}
			case GetItemWidgets: {
				WidgetList items;

				if (g->numItems > 0) {
					items = (WidgetList)malloc(sizeof(Widget) * g->numItems);
					for (i = 0; i < g->numItems; i++)
						items[i] = g->items[i].button;
					return (XtPointer)items;
				}
				else
					return (XtPointer)NULL;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	}
	else {
		for (i=0; value==NULL && i<g->numItems; i++) {
			if (g->items[i].subMenu != NULL) {
				value = QueryGizmo(
					MenuBarGizmoClass,
					g->items[i].subMenu,
					option, str
				);
			}
		}
		return value;
	}
}

/*
 * _SetMenuDefault:  called by PopupGizmo and FileGizmo to reset
 * the default button each time a PopupGizmo or FileGizmo is
 * mapped.  This is not currently working.
 */

void
_SetMenuDefault(Gizmo g, Widget w)
{
    MenuGizmoP 	*mg = (MenuGizmoP *)g;
    Arg		arg[2];
    int		i = 0;

    XtSetArg(arg[i], XmNcancelButton, mg->items[mg->cancelItem].button); i++;
    XtSetArg(arg[i], XmNdefaultButton, mg->items[mg->defaultItem].button); i++;
    XtSetValues(w, arg, i);
    i = 0;
    XtSetArg(arg[i], XmNshowAsDefault, True); i++;
    XtSetArg(arg[i], XmNdefaultButtonShadowThickness, 1); i++;
    XtSetValues (mg->items[mg->defaultItem].button, arg, i);

#ifdef NOT_USED
    /* This takes focus away from the textfield within
     * the shell.  We just want to set focus within the menubar.
     */
    XmProcessTraversal(mg->items[mg->defaultItem].button, XmTRAVERSE_CURRENT );
#endif
}
static void
DumpMenu(MenuGizmoP *g, int indent)
{
	int		i;
	static int	tab = 0;

	if (g->menu == NULL) {
		return;
	}
	fprintf(stderr, "%*s", (indent+tab)*4, " ");
	fprintf(stderr, "menu(%s) = 0x%x 0x%x\n", g->name, g, g->menu);
	tab += 1;
	for (i=0; i<g->numItems; i++) {
		fprintf(stderr, "%*s", (indent+tab)*4, " ");
		fprintf(stderr, "item(%d) = 0x%x\n", i, g->items[i].button);
		if (g->items[i].subMenu != NULL) {
			tab += 1;
			DumpMenu(g->items[i].subMenu, indent+1);
			tab -= 1;
		}
	}
	tab -= 1;
}

void
SetSubMenuValue (Gizmo g, Gizmo new, int item)
{
	MenuGizmoP *	gp = (MenuGizmoP *)g;
	MenuGizmoP *	newp = (MenuGizmoP *)new;
	Arg		arg[10];

	gp->items[item].subMenu = new;
	/* Indicate Menu is being shared. */
	gp->items[item].subMenu->count += 1;
	XtSetArg(arg[0], XmNsubMenuId, newp->menu);
	XtSetValues (gp->items[item].button, arg, 1);
}

static Widget
AttachmentWidget(MenuGizmoP *g)
{
	return g->menu;
}
