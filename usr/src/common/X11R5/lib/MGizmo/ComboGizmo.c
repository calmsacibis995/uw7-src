#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ComboGizmo.c	1.4"
#endif

/*
 * ComboGizmo.c
 */

#include <Xm/Text.h>
#include <DtWidget/ComboBox.h>
#include "Gizmo.h"
#include "ComboGizmP.h"

static Gizmo		CreateComboGizmo();
static Gizmo		SetComboValueByName();
static void		FreeComboGizmo();
static void		DumpComboGizmo();
static void		ManipulateComboGizmo();
static XtPointer	QueryComboGizmo();
static Widget		AttachmentWidget();

GizmoClassRec ComboBoxGizmoClass[] = {
	"ComboGizmo",
	CreateComboGizmo,
	FreeComboGizmo,
	NULL,	/* map */
	NULL,	/* get */
	NULL,	/* get menu */
	DumpComboGizmo,
	ManipulateComboGizmo,
	QueryComboGizmo,
	SetComboValueByName,
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeComboGizmo
 */

static void 
FreeComboGizmo(ComboBoxGizmoP * g)
{
	FREE(g->initial);
	FREE(g->current);
	FREE(g->previous);
	FREE(g);
}

/*
 * CreateComboGizmo
 */

static Gizmo
CreateComboGizmo(Widget parent, ComboBoxGizmo * g, Arg * args, int num)
{
	ComboBoxGizmoP *	combo;
	XmString		string;
	XmStringTable		strings;
	Arg			arg[100];
	int			i;


	combo = (ComboBoxGizmoP *)CALLOC(1, sizeof(ComboBoxGizmoP));
	combo->name = g->name;
	combo->items = (char **)CALLOC(g->numItems, sizeof(char *));
	memcpy(combo->items, g->items, g->numItems * sizeof(char *));

	combo->current = STRDUP(g->defaultItem);
	combo->previous = STRDUP(g->defaultItem);
	combo->initial = STRDUP(g->defaultItem);

	/* create compound string table */

	strings = (XmStringTable)MALLOC(
		(g->numItems) * sizeof(XmString)
	);
	i = 0;
	while ( i < g->numItems) {
		strings[i] = XmStringCreateLocalized(GGT(g->items[i]));
		i++;
	}

	i = 0;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNarrowType, XmMOTIF); i++;
	XtSetArg(arg[i], XmNorientation, XmRIGHT); i++;
	XtSetArg(arg[i], XmNvisibleItemCount, g->visible); i++;
	XtSetArg(arg[i], XmNitems, strings); i++;
	XtSetArg(arg[i], XmNitemCount, g->numItems); i++;
	i = AppendArgsToList (arg, i, args, num);
	combo->comboBox = XtCreateManagedWidget (
		"combobox", dtComboBoxWidgetClass, parent,
		arg, i
	);
	string = XmStringCreateLocalized(GGT(g->defaultItem));
	(void)DtComboBoxSelectItem(
		(DtComboBoxWidget)combo->comboBox, string
	);
	XmStringFree(string);

	if (g->help != NULL) {
		GizmoRegisterHelp(combo->comboBox, g->help);
	}

	for (i = 0; i < g->numItems; i++) {
		XmStringFree(strings[i]);
	}
	XtFree((XtPointer)strings);

	return (Gizmo)combo;
}

static void
DumpComboGizmo(ComboBoxGizmoP * g)
{
	printf(
		"combo(%s) = 0x%x 0x%x\n",
		g->name,
		g,
		g->comboBox
	);
}

static void
ManipulateComboGizmo(ComboBoxGizmoP * g, ManipulateOption option)
{
	XmString	string;
	char *		str;
	Arg		arg[2];
	int		i;
	int		pos;

	switch (option) {
		case GetGizmoValue: {
			FREE(g->current);
			XtSetArg(arg[0], XmNselectedPosition, &pos);
			XtGetValues(g->comboBox, arg, 1);
			g->current = STRDUP(g->items[pos]);
			break;
		}
		case ApplyGizmoValue: {
			FREE(g->previous);
			g->previous = STRDUP(g->current);
			break;
		}
		case ReinitializeGizmoValue: {
			FREE(g->current);
			g->current = STRDUP(g->initial);
			string = XmStringCreateLocalized(GGT(g->initial));
			(void)DtComboBoxSetItem(
				(DtComboBoxWidget)g->comboBox, string
			);
			XmStringFree(string);
			break;
		}
		case ResetGizmoValue: {
			FREE(g->current);
			g->current = STRDUP(g->previous);
			string = XmStringCreateLocalized(GGT(g->current));
			(void)DtComboBoxSetItem(
				(DtComboBoxWidget)g->comboBox, string
			);
			XmStringFree(string);
			break;
		}
		default: {
			break;
		}
	}
}

static XtPointer
QueryComboGizmo(ComboBoxGizmoP * g, int option, char * name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoCurrentValue: {
				return (XtPointer)(g->current);
			}
			case GetGizmoPreviousValue: {
				return (XtPointer)(g->previous);
			}
			case GetGizmoWidget: {
				return (XtPointer)(g->comboBox);
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	} else {
		return (XtPointer)NULL;
	}
}

extern void
SetComboGizmoValue(Gizmo g, char * value)
{
	XmString	string;
	Arg		arg[1];
	int		i;

	string = XmStringCreateLocalized(GGT(value));
	(void)DtComboBoxSetItem(
		(DtComboBoxWidget)((ComboBoxGizmoP *)g)->comboBox, string
	);
	XmStringFree(string);
}

static Gizmo
SetComboValueByName(ComboBoxGizmoP *g, char *name, char *value)
{
	Arg	arg[1];

	if (!name || strcmp(name, g->name) == 0) {
		SetComboGizmoValue(g, value);
		return (Gizmo)g;
	}
	else {
		return NULL;
	}
}

extern char *
GetComboGizmoValue(Gizmo g)
{
	Arg		arg[1];
	int		i;
	int		pos;

	i = 0;
	XtSetArg(arg[i], XmNselectedPosition, &pos); i++;
	XtGetValues(((ComboBoxGizmoP *)g)->comboBox, arg, i);

	return ((ComboBoxGizmoP *)g)->items[pos];
}

static Widget
AttachmentWidget(ComboBoxGizmoP *g)
{
	return g->comboBox;
}
