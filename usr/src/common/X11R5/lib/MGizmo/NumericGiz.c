#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:NumericGiz.c	1.4"
#endif

/*
 * NumericGiz.c
 */

#include <DtWidget/SpinBox.h>
#include "Gizmo.h"
#include "NumericGP.h"

static Gizmo		CreateNumericGizmo();
static Gizmo		SetNumericGizmoValueByName();
static void		FreeNumericGizmo();
static void		DumpNumericGizmo();
static void		ManipulateNumericGizmo();
static XtPointer	QueryNumericGizmo();
static Widget		AttachmentWidget();

GizmoClassRec NumericGizmoClass[] = {
	"NumericGizmo",
	CreateNumericGizmo,
	FreeNumericGizmo,
	NULL,	/* map */
	NULL,	/* get */
	NULL,	/* get menu */
	DumpNumericGizmo,
	ManipulateNumericGizmo,
	QueryNumericGizmo,
	SetNumericGizmoValueByName,
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeNumericGizmo
 */

static void 
FreeNumericGizmo(NumericGizmoP *g)
{
	FREE(g);
}

/*
 * CreateNumericGizmo
 */

static Gizmo
CreateNumericGizmo(Widget parent, NumericGizmo *g, Arg * args, int num)
{
	NumericGizmoP *	numeric;
	Arg		arg[100];
	int		i;


	numeric = (NumericGizmoP *)CALLOC(1, sizeof(NumericGizmoP));
	numeric->name = g->name;

	numeric->initial = g->value;
	numeric->current = g->value;
	numeric->previous = g->value;

	i = 0;
	XtSetArg(arg[i], XmNarrowLayout, XmARROWS_END); i++;
	XtSetArg(arg[i], XmNspinBoxChildType, XmNUMERIC); i++;
	XtSetArg(arg[i], XmNdecimalPoints, g->radix); i++;
	XtSetArg(arg[i], XmNincrementValue, g->inc); i++;
	XtSetArg(arg[i], XmNmaximumValue, g->max); i++;
	XtSetArg(arg[i], XmNminimumValue, g->min); i++;
	XtSetArg(arg[i], XmNposition, g->value); i++;
	XtSetArg(arg[i], XmNeditable, False); i++;
	i = AppendArgsToList (arg, i, args, num);
	numeric->spinBox = DtCreateSpinBox (
		parent, numeric->name, arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(numeric->spinBox, g->help);
	}
	XtManageChild(numeric->spinBox);

	return (Gizmo)numeric;
}

static void
DumpNumericGizmo(NumericGizmoP *g)
{
	printf(
		"numeric(%s) = 0x%x 0x%x\n",
		g->name,
		g,
		g->spinBox
	);
}

static void
ManipulateNumericGizmo(NumericGizmoP *g, ManipulateOption option)
{
	Arg	arg[2];
	int	i;

	switch (option) {
		case GetGizmoValue: {
			i = 0;
			XtSetArg(arg[i], XmNposition, &g->current); i++;
			XtGetValues(g->spinBox, arg, i);
			break;
		}
		case ApplyGizmoValue: {
			g->previous = g->current;
			break;
		}
		case ReinitializeGizmoValue: {
			g->current = g->initial;
			i = 0;
			XtSetArg(arg[i], XmNposition, g->initial); i++;
			XtSetValues(g->spinBox, arg, i);
			break;
		}
		case ResetGizmoValue: {
			g->current = g->previous;
			i = 0;
			XtSetArg(arg[i], XmNposition, g->current); i++;
			XtSetValues(g->spinBox, arg, i);
			break;
		}
		default: {
			break;
		}
	}
}

static XtPointer
QueryNumericGizmo(NumericGizmoP *g, int option, char * name)
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
				return (XtPointer)(g->spinBox);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(g);
				break;
			}
			default: {
				return (XtPointer)(NULL);
			}
		}
	} else {
		return (XtPointer)(NULL);
	}
}

void
SetNumericInitialValue(Gizmo g, int value)
{
	((NumericGizmoP *)g)->initial = value;
}

void
SetNumericGizmoValue(Gizmo g, int value)
{
	Arg		arg[1];
	int		i;

	i = 0;
	XtSetArg(arg[i], XmNposition, value); i++;
	XtSetValues(((NumericGizmoP *)g)->spinBox, arg, i);
}

static Gizmo
SetNumericGizmoValueByName(NumericGizmoP * g, char * name, char * value)
{
	if (!name || strcmp(name, g->name) == 0) {
		SetNumericGizmoValue((Gizmo)g, (int)value);
		return (Gizmo)g;
	}
	else {
		return NULL;
	}
}

int
GetNumericGizmoValue(Gizmo g)
{
	Arg		arg[1];
	int		i;
	int		value;

	i = 0;
	XtSetArg(arg[i], XmNposition, &value); i++;
	XtGetValues(((NumericGizmoP *)g)->spinBox, arg, i);

	return value;
}

static Widget
AttachmentWidget(NumericGizmoP *g)
{
	return g->spinBox;
}
