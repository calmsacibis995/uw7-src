#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MsgGizmo.c	1.2"
#endif

/*
 * MsgGizmo.c
 */

#include <stdio.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include "Gizmo.h"
#include "MsgGizmoP.h"

static Gizmo		CreateMsgGizmo();
static void		FreeMsgGizmo();
static void		DumpMsgGizmo();
static XtPointer	QueryMsgGizmo();
static Widget          	AttachmentWidget();

GizmoClassRec MsgGizmoClass[] = {
	"MsgGizmo",
	CreateMsgGizmo,
	FreeMsgGizmo,
	NULL,
	NULL,
	NULL,
	DumpMsgGizmo,
	NULL,	/* manipulate */
	QueryMsgGizmo,
	NULL,	/* setValueByName */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeMsgGizmo
 */

static void 
FreeMsgGizmo(MsgGizmoP *g)
{
	FREE(g);
}

/*
 * CreatMsgGizmo
 */

static Gizmo
CreateMsgGizmo(Widget parent, MsgGizmo *g, Arg *args, int num)
{
	MsgGizmoP *	label;
	XmString	string;
	int		i;
	Arg		arg[100];


	label = (MsgGizmoP *)CALLOC(1, sizeof(MsgGizmoP));
	label->name = g->name;

	label->msgForm = XtCreateManagedWidget (
		"form", xmFormWidgetClass, parent,
		NULL, 0
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(label->msgForm, g->help);
	}

	string = XmStringCreateLocalized(GGT(g->leftMsgText));
	i = 0;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
	i = AppendArgsToList (arg, i, args, num);
	label->leftMsgLabel = XtCreateManagedWidget(
		label->name, xmLabelGadgetClass, label->msgForm,
		arg, i
	);
	XmStringFree(string);

	string = XmStringCreateLocalized(GGT(g->rightMsgText));
	i = 0;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
	i = AppendArgsToList (arg, i, args, num);
	label->rightMsgLabel = XtCreateManagedWidget(
		label->name, xmLabelGadgetClass, label->msgForm,
		arg, i
	);
	XmStringFree(string);

	return (Gizmo)label;
}

static void
DumpMsgGizmo(MsgGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(
		stderr, "label (%s) = 0x%x 0x%x 0x%x\n", g->name,
		g, g->leftMsgLabel, g->rightMsgLabel
	);
}

static XtPointer
QueryMsgGizmo(MsgGizmoP *g, int option, char *name)
{
	XtPointer	value = NULL;

	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->leftMsgLabel);
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

extern void
SetMsgGizmoTextLeft(Gizmo g, char *label)
{
	XmString	string;
	Arg		args[1];
	int		i;

	string = XmStringCreateLocalized(GGT(label));
	i = 0;
	XtSetArg(args[i], XmNlabelString, string); i++;
	XtSetValues(((MsgGizmoP *)g)->leftMsgLabel, args, i);
		
	XmStringFree(string);
}

extern void
SetMsgGizmoTextRight(Gizmo g, char *label)
{
	XmString	string;
	Arg		args[1];
	int		i;

	string = XmStringCreateLocalized(GGT(label));
	i = 0;
	XtSetArg(args[i], XmNlabelString, string); i++;
	XtSetValues(((MsgGizmoP *)g)->rightMsgLabel, args, i);
		
	XmStringFree(string);
}

extern Widget
GetMsgGizmoWidgetRight(Gizmo g)
{
	return ((MsgGizmoP *)g)->rightMsgLabel;
}

static Widget
AttachmentWidget(MsgGizmoP *g)
{
	return g->msgForm;
}
