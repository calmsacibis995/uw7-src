#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ChoiceGizm.c	1.4"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"
#include "ChoiceGizP.h"

static Gizmo		CreateChoice();
static void		FreeChoice();
static void		ManipulateChoice();
static void		DumpChoice();
static XtPointer	QueryChoice();
static void		ChoiceUnconverter(ChoiceGizmo *);
static void		ChoiceConverter();
static int		ValueToCode();
static void		SetItems();
extern Widget		AttachmentWidget();

GizmoClassRec ChoiceGizmoClass[] = { 
	"ChoiceGizmo",
	CreateChoice,		/* Create */
	FreeChoice,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpChoice,		/* Dump */
	ManipulateChoice,	/* Manipulate */
	QueryChoice,		/* Query */
	NULL,			/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeChoice
 */

static void
FreeChoice(ChoiceGizmoP *g)
{
	if (g->type == G_TOGGLE_BOX) {
		FREE(g->initial);
		FREE(g->current);
		FREE(g->previous);
	}
	FreeGizmo(CommandMenuGizmoClass, g->menu);
	FREE(g);
}

/*
 * CreateChoice
 */
static Gizmo
CreateChoice(Widget parent, ChoiceGizmo *g, ArgList args, int numArgs)
{
	ChoiceGizmoP *	choice;
	int		i;
	char *		initial = NULL;

	choice = (ChoiceGizmoP *)CALLOC(1, sizeof(ChoiceGizmoP));

	/* Copy the name */
	choice->name = g->name;
	choice->type = g->type;

	/* Copy the settings */
	/* These come from the value of 'set' from each item. */
	/* Take the settings from the items structure */
	for (i=0; g->menu->items[i].label!=NULL; i++) {
		/* Count the number of items */
	}
	if (g->type == G_TOGGLE_BOX) {
		initial = (XtPointer)MALLOC(i+1);
	}
	for (i=0; g->menu->items[i].label!=NULL; i++) {
		if (g->type == G_TOGGLE_BOX) {
			initial[i] = (g->menu->items[i].set == False) ? '_' : 'x';
		}
		else {
			if (g->menu->items[i].set == True) {
				initial = (char *)i;
				break;
			}
		}
	}
	if (g->type == G_TOGGLE_BOX) {
		initial[i] = '\0';
		choice->initial = initial;
		choice->current = (XtPointer)STRDUP(choice->initial);
		choice->previous = (XtPointer)STRDUP(choice->initial);
	}
	else {
		choice->initial = (XtPointer)initial;
		choice->previous = (XtPointer)initial;
		choice->current = (XtPointer)initial;
	}
	switch (g->type) {
		case G_RADIO_BOX:
		case G_TOGGLE_BOX: {
			choice->menu = (MenuGizmoP *)CreateGizmo(
				parent, CommandMenuGizmoClass,
				g->menu, args, numArgs
			);
			break;
		}
		case G_OPTION_BOX: {
			choice->menu = (MenuGizmoP *)CreateGizmo(
				parent, OptionMenuGizmoClass,
				g->menu, args, numArgs
			);
			break;
		}
	}
	if (g->help != NULL) {
		GizmoRegisterHelp(choice->menu->menu, g->help);
	}
	return (Gizmo)choice;
}

/*
 * GetValueFromWidget
 *
 * Get the current setting from the setting of the buttons.
 */
static void
GetCurrentFromButtons(ChoiceGizmoP *g)
{
	int		i;
	MenuGizmoP *	menu = g->menu;
	Boolean		set;
	char *		current;
	Arg		arg[10];
	Widget		w;

	/* If this is an option menu or a radio box then the */
	/* setting has to be taken from the menuHistory. */
	if (g->type != G_TOGGLE_BOX) {
		XtSetArg(arg[0], XmNmenuHistory, &w);
		XtGetValues(menu->menu, arg, 1);
		for (i=0; i<menu->numItems; i++) {
			if (w == menu->items[i].button) {
				g->current = (XtPointer)i;
				break;
			}
		}
	}
	else {
		current = (char *)g->current;
		for (i=0; i<menu->numItems; i++) {
			/* Get the setting of the ith button */
			XtSetArg (arg[0], XmNset, &set);
			XtGetValues(menu->items[i].button, arg, 1);
			/* Put x's in the setting string where */
			/* buttons are set */
			current[i] = set ? 'x' : '_';
		}
	}
}

/*
 * SetToggleItems
 */
static void
SetToggleItems(ChoiceGizmoP *g, ManipulateOption option)
{
	Arg		argOn[1];
	Arg		argOff[1];
	MenuGizmoP *	menu = g->menu;
	int		i;
	int		flag;
	char *	 	oldString;
	char *		newString;

	XtSetArg(argOn[0], XmNset, True);
	XtSetArg(argOff[0], XmNset, False);

	if (option == ResetGizmoValue) {
		newString = g->previous;
	}
	else {
		newString = g->initial;
	}

	flag = False;
	oldString = (char *)g->current;

	for (i=0; oldString[i]; i++) {
		if (oldString[i] != newString[i]) {
			flag = True;
		}
		XtSetValues(
			menu->items[i].button,
			newString[i] == 'x' ? argOn : argOff, 1
		);
	}
	if (flag) {
		FREE(oldString);
		g->current = (XtPointer)STRDUP(newString);
	}
}

/*
 * SetNontoggleItems
 */
static void
SetNontoggleItems(ChoiceGizmoP *g, ManipulateOption option)
{
	MenuGizmoP *	menu = g->menu;
	int		i;
	int		j;
	Arg		arg[10];
	Arg		argOn[1];
	Arg		argOff[1];

	XtSetArg(argOn[0], XmNset, True);
	XtSetArg(argOff[0], XmNset, False);

	if (option == ResetGizmoValue) {
		g->current = g->previous;
	}
	else {
		g->current = g->initial;
	}
	i = (int)g->current;
	if (g->type == G_RADIO_BOX) {
		/* Set or unset all buttons as appropriate */
		for (j=0; j<menu->numItems; j++) {
			XtSetValues(
				menu->items[j].button,
				(i==j) ? argOn : argOff, 1
			);
		}
	}
	XtSetArg(arg[0], XmNmenuHistory, menu->items[i].button);
	XtSetValues(menu->menu, arg, 1);
}

/*
 * ManipulateChoice
 */
static void
ManipulateChoice(ChoiceGizmoP *g, ManipulateOption option)
{
	switch (option) {
		/* currentValue = widget value */
		case GetGizmoValue: {
			GetCurrentFromButtons(g);
			break;
		}
		/* previousValue = currentValue */
		case ApplyGizmoValue: {
			if (g->type == G_TOGGLE_BOX) {
				if (g->previous) {
					FREE(g->previous);
				}
				g->previous = STRDUP(g->current);
			}
			else {
				g->previous = g->current;
			}
			break;
		}
		/* currentValue = previousValue */
		/* widget = previousValue */
		case ResetGizmoValue:
		case ReinitializeGizmoValue: {
			if (g->type == G_TOGGLE_BOX) {
				SetToggleItems(g, option);
			}
			else {
				SetNontoggleItems(g, option);
			}
			break;
		}
		default: {
			break;
		}
	}
}

/*
 * QueryChoice
 */
static XtPointer
QueryChoice(ChoiceGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoCurrentValue: {
				return (XtPointer)(g->current);
				break;
			}
			case GetGizmoPreviousValue: {
				return (XtPointer)(g->previous);
				break;
			}
			case GetGizmoWidget: {
				return (XtPointer)QueryGizmo (
					MenuBarGizmoClass, g->menu,
					option, name
				);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
				break;
			}
			default: {
				return (XtPointer)NULL;
				break;
			}
		}
	}
	else {
		return (XtPointer)QueryGizmo (
			MenuBarGizmoClass, g->menu, option, name
		);
	}
}

/*
 * ChoiceUnconverter
 */
static void
ChoiceUnconverter(ChoiceGizmo *g)
{
}

/*
 * ChoiceConverter
 */
static void
ChoiceConverter(XrmValue args, Cardinal numArgs, XrmValue from, XrmValue to)
{
}

/*
 * ValueToCode
 */
static int
ValueToCode(char *value, MenuItems *items, int nomatchCode)
{
}

static void
DumpChoice(ChoiceGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf (stderr, "choice(%s) = 0x%x\n", g->name, g);
	DumpGizmo(OptionMenuGizmoClass, g->menu, indent+1);
}

static Widget
AttachmentWidget(ChoiceGizmoP *g)
{
	return GetAttachmentWidget(MenuBarGizmoClass, g->menu);
}
