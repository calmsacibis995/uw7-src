#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:AlphaGizmo.c	1.4"
#endif

/*
 * AlphaGizmo.c
 */

#include <Xm/Text.h>
#include <DtWidget/SpinBox.h>
#include "Gizmo.h"
#include "AlphaGizmP.h"

static Gizmo		CreateAlphaGizmo();
static Gizmo		SetAlphaValueByName();
static void		FreeAlphaGizmo();
static void		DumpAlphaGizmo();
static void		ManipulateAlphaGizmo();
static XtPointer	QueryAlphaGizmo();
static Widget		AttachmentWidget();

GizmoClassRec AlphaGizmoClass[] = {
	"AlphaGizmo",
	CreateAlphaGizmo,
	FreeAlphaGizmo,
	NULL,	/* map */
	NULL,	/* get */
	NULL,	/* get menu */
	DumpAlphaGizmo,
	ManipulateAlphaGizmo,
	QueryAlphaGizmo,
	SetAlphaValueByName,
	AttachmentWidget
};

/*
 * FreeAlphaGizmo
 */

static void 
FreeAlphaGizmo(AlphaGizmoP * g)
{
	FREE(g->initial);
	FREE(g->current);
	FREE(g->previous);
	FREE(g);
}

/*
 * CreateAlphaGizmo
 */

static Gizmo
CreateAlphaGizmo(Widget parent, AlphaGizmo * g, Arg * args, int num)
{
	XmStringTable	strings;
	XmString	string;
	AlphaGizmoP *	alpha;
	Arg		arg[100];
	int		i;


	alpha = (AlphaGizmoP *)CALLOC(1, sizeof(AlphaGizmoP));
	alpha->name = g->name;
	alpha->items = (char **)CALLOC(g->numItems, sizeof(char *));
	memcpy(alpha->items, g->items, g->numItems * sizeof(char *));

	alpha->current = STRDUP(g->items[g->defaultItem]);
	alpha->previous = STRDUP(g->items[g->defaultItem]);
	alpha->initial  = STRDUP(g->items[g->defaultItem]);

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
	XtSetArg(arg[i], XmNarrowLayout, XmARROWS_END); i++;
	XtSetArg(arg[i], XmNeditable, False); i++;
	XtSetArg(arg[i], XmNitemCount, g->numItems); i++;
	XtSetArg(arg[i], XmNitems, strings); i++;
	XtSetArg(arg[i], XmNposition, g->defaultItem); i++;
	i = AppendArgsToList (arg, i, args, num);
	alpha->spinButton = DtCreateSpinBox (
		parent, alpha->name, arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(alpha->spinButton, g->help);
	}
	XtManageChild(alpha->spinButton);

	for (i = 0; i < g->numItems; i++) {
		XmStringFree(strings[i]);
	}
	XtFree((XtPointer)strings);

	return (Gizmo)alpha;
}

static void
DumpAlphaGizmo(AlphaGizmoP * g)
{
	printf(
		"alpha(%s) = 0x%x 0x%x\n",
		g->name,
		g,
		g->spinButton
	);
}

static void
ManipulateAlphaGizmo(AlphaGizmoP * g, ManipulateOption option)
{
	XmString	string;
	String		str;
	Arg		arg[2];
	int		i;
	int		pos;

	switch (option) {
		case GetGizmoValue: {
			FREE(g->current);
			XtSetArg(arg[0], XmNposition, &pos);
			XtGetValues(g->spinButton, arg, 1);
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
			(void)DtSpinBoxSetItem(
				(DtSpinBoxWidget)g->spinButton, string
			);
			XmStringFree(string);
			break;
		}
		case ResetGizmoValue: {
			FREE(g->current);
			g->current = STRDUP(g->previous);
			string = XmStringCreateLocalized(GGT(g->current));
			(void)DtSpinBoxSetItem(
				(DtSpinBoxWidget)g->spinButton, string
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
QueryAlphaGizmo(AlphaGizmoP * g, int option, char * name)
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
				return (XtPointer)(g->spinButton);
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
SetAlphaGizmoValue(Gizmo g, char * value)
{
	XmString	string;
	Arg		arg[1];
	int		i;

	string = XmStringCreateLocalized(GGT(value));
	(void)DtSpinBoxSetItem(
		(DtSpinBoxWidget)((AlphaGizmoP *)g)->spinButton, string
	);
	XmStringFree(string);
}

static Gizmo
SetAlphaValueByName(AlphaGizmoP *g, char *name, char *value)
{
	Arg	arg[1];

	if (!name || strcmp(name, g->name) == 0) {
		SetAlphaGizmoValue(g, value);
		return (Gizmo)g;
	}
	else {
		return NULL;
	}
}

extern char *
GetAlphaGizmoValue(Gizmo g)
{
	Arg		arg[1];
	int		i;
	int		pos;

	i = 0;
	XtSetArg(arg[i], XmNposition, &pos); i++;
	XtGetValues(((AlphaGizmoP *)g)->spinButton, arg, i);

	return ((AlphaGizmoP *)g)->items[pos];
}

static Widget
AttachmentWidget(AlphaGizmoP *alpha)
{
	return alpha->spinButton;
}
