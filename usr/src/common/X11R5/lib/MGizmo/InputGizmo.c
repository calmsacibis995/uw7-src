#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:InputGizmo.c	1.7"
#endif

/*
 * InputGizmo.c
 */

#include <stdio.h>

#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include "Gizmo.h"
#include "InputGizmoP.h"

static Gizmo		CreateInputGizmo();
static void		FreeInputGizmo();
static void		DumpInputGizmo();
static void		ManipulateInputGizmo();
static XtPointer	QueryInputGizmo();
static Gizmo		SetInputGizmoValueByName();
static Widget		AttachmentWidget();

GizmoClassRec InputGizmoClass[] = {
	"InputGizmo",
	CreateInputGizmo,
	FreeInputGizmo,
	NULL,	/* map */
	NULL,	/* get */
	NULL,	/* get menu */
	DumpInputGizmo,
	ManipulateInputGizmo,
	QueryInputGizmo,
	SetInputGizmoValueByName,
	AttachmentWidget
};

/*
 * FreeInputGizmo
 */

static void 
FreeInputGizmo(InputGizmoP * g)
{
	FREE(g->current);
	FREE(g->initial);
	FREE(g->previous);
	FREE(g);
}

/*
 * CreateInputGizmo
 */

static Gizmo
CreateInputGizmo(Widget parent, InputGizmo * g, Arg * args, int num)
{
	InputGizmoP *	input;
	Widget		rc;
	Arg		arg[100];
	int		i;

	input = (InputGizmoP *)CALLOC(1, sizeof(InputGizmoP));
	input->name = g->name;

	if (g->text) {
		input->current = STRDUP(g->text);
		input->initial = STRDUP(g->text);
		input->previous = STRDUP(g->text);
	} else {
		input->initial = STRDUP("");
		input->current = STRDUP("");
		input->previous = STRDUP("");
	}

	i = 0;
	XtSetArg(arg[i], XmNeditable, True); i++;
	if (g->width > 0){
	    XtSetArg(arg[i], XmNcolumns, g->width); i++;
	}
	XtSetArg(arg[i], XmNvalue, g->text); i++;
	i = AppendArgsToList (arg, i, args, num);
	input->textField = XtCreateManagedWidget(
		input->name, xmTextFieldWidgetClass, parent,
		arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(input->textField, g->help);
	}

	if (g->callback){
	    XtAddCallback(
			  input->textField,
			  XmNactivateCallback,
			  g->callback,
			  g->client_data
	    );
	}

	return (Gizmo)input;
}

static void
DumpInputGizmo(InputGizmoP * g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "input (%s) = 0x%x 0x%x\n", g->name, g, g->textField);
}

static void
ManipulateInputGizmo(InputGizmoP * g, ManipulateOption option)
{
	Arg	arg[2];
	int	i;

	switch (option) {
		case GetGizmoValue: {
			FREE(g->current);
			XtSetArg(arg[0], XmNvalue, &g->current);
			XtGetValues(g->textField, arg, 1);
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
			XtSetArg(arg[0], XmNvalue, g->initial);
			XtSetValues(g->textField, arg, 1);
			break;
		}
		case ResetGizmoValue: {
			FREE(g->current);
			g->current = STRDUP(g->previous);
			XtSetArg(arg[0], XmNvalue, g->current);
			XtSetValues(g->textField, arg, 1);
			break;
		}
		default: {
			break;
		}
	}
}

static XtPointer
QueryInputGizmo(InputGizmoP * g, int option, char * name)
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
				return (XtPointer)(g->textField);
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
SetInputGizmoText(Gizmo g, char * text)
{
	XmTextFieldSetString(((InputGizmoP *)g)->textField, text);
}

extern char *
GetInputGizmoText(Gizmo g)
{
        InputGizmoP *ig = (InputGizmoP *) g;

	return(XmTextFieldGetString(ig->textField));
}

static Gizmo
SetInputGizmoValueByName(InputGizmoP * g, char * name, char * value)
{
	if (!name || strcmp(name, g->name) == 0) {
		SetInputGizmoText((Gizmo)g, value);
		return (Gizmo)g;
	}
	else {
		return NULL;
	}
}

static Widget
AttachmentWidget(InputGizmoP *g)
{
	return g->textField;
}
