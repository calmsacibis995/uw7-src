#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ScaleGizmo.c	1.2"
#endif

#include <stdio.h>

#include <Xm/Scale.h>

#include "Gizmo.h"
#include "ScaleGizmP.h"

static Gizmo		CreateScale();
static void		FreeScale();
static void		ManipulateScale();
static void		DumpScale();
static XtPointer	QueryScale();
static Gizmo		SetScaleValueByName();
static Widget		AttachmentWidget();

GizmoClassRec ScaleGizmoClass[] = { 
	"ScaleGizmo",
	CreateScale,		/* Create */
	FreeScale,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpScale,		/* Dump */
	ManipulateScale,	/* Manipulate */
	QueryScale,		/* Query */
	SetScaleValueByName,	/* Set by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeScale
 */

static void
FreeScale(ScaleGizmoP *g)
{
	FREE(g);
}

/*
 * CreateScale
 */
static Gizmo
CreateScale(Widget parent, ScaleGizmo *g, ArgList args, int numArgs)
{
	ScaleGizmoP *	scale;
	int		i = 0;
	char *		initial;
	Arg		arg[100];
	XmString	str;

	scale = (ScaleGizmoP *)CALLOC(1, sizeof(ScaleGizmoP));

	/* Copy the name */
	scale->name = g->name;

	/* Copy the settings */
	scale->initial = g->value;
	scale->current = g->value;
	scale->previous = g->value;

	/* Create the scale widget */
	if (g->title != NULL) {
		str = XmStringCreateLocalized(GGT(g->title));
		XtSetArg(arg[i], XmNtitleString, str); i++;
	}
	XtSetArg(arg[i], XmNorientation, g->orientation); i++;
	XtSetArg(arg[i], XmNvalue, g->value); i++;
	XtSetArg(arg[i], XmNmaximum, g->max); i++;
	XtSetArg(arg[i], XmNminimum, g->min); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	scale->scale = XtCreateManagedWidget(
		"scale", xmScaleWidgetClass, parent, arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(scale->scale, g->help);
	}
	if (g->title != NULL) {
		XmStringFree(str);
	}
	return (Gizmo)scale;
}

/*
 * ManipulateScale
 */
static void
ManipulateScale(ScaleGizmoP *g, ManipulateOption option)
{
	Arg	arg[10];

	switch (option) {
		/* currentValue = widget value */
		case GetGizmoValue: {
			XtSetArg(arg[0], XmNvalue, &g->current);
			XtGetValues(g->scale, arg, 1);
			break;
		}
		/* previousValue = currentValue */
		case ApplyGizmoValue: {
			g->previous = g->current;
			break;
		}
		/* currentValue = initialValue */
		/* widget = initialValue */
		case ReinitializeGizmoValue: {
			g->current = g->initial;
			XtSetArg(arg[0], XmNvalue, g->initial);
			XtSetValues(g->scale, arg, 1);
			break;
		}
		/* currentValue = previousValue */
		/* widget = previousValue */
		case ResetGizmoValue: {
			g->current = g->previous;
			XtSetArg(arg[0], XmNvalue, g->previous);
			XtSetValues(g->scale, arg, 1);
			break;
		}
		default: {
			break;
		}
	}
}

/*
 * QueryScale
 */
static XtPointer
QueryScale(ScaleGizmoP *g, int option, char *name)
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
				return (XtPointer)g->scale;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	}
	else {
		return (XtPointer)NULL;
	}
}

static void
DumpScale(ScaleGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf (stderr, "scale(%s) = 0x%x\n", g->name, g);
}

void
SetScaleGizmoValue(Gizmo g, int value)
{
	Arg	arg[10];

	XtSetArg(arg[0], XmNvalue, value);
	XtSetValues(((ScaleGizmoP *)g)->scale, arg, 1);
}

static Gizmo
SetScaleValueByName(ScaleGizmoP *g, char *name, int value)
{
	if (!name || strcmp(name, g->name) == 0) {
		SetScaleGizmoValue((Gizmo)g, value);
	}
	else {
		return (Gizmo)NULL;
	}
}

int
GetScaleGizmoValue(Gizmo g)
{
	Arg	arg[10];
	int	value;

	XtSetArg(arg[0], XmNvalue, &value);
	XtGetValues(((ScaleGizmoP *)g)->scale, arg, 1);
	return value;
}

static Widget
AttachmentWidget(ScaleGizmoP *g)
{
	return g->scale;
}
