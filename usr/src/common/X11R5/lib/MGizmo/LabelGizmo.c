#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:LabelGizmo.c	1.7"
#endif

/*
 * LabelGizmo.c
 */

#include <stdio.h>

#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>

#include "Gizmo.h"
#include "LabelGizmP.h"

static Gizmo		CreateLabel();
static void		FreeLabel();
static void		DumpLabel();
static void		ManipulateLabel();
static XtPointer	QueryLabel();
static Widget		AttachmentWidget();

GizmoClassRec LabelGizmoClass[] = { 
	"LabelGizmo", 
	CreateLabel, 
	FreeLabel, 
	NULL,	/* map */
	NULL,	/* get */
	NULL,	/* get menu */
	DumpLabel,
	ManipulateLabel,
	QueryLabel,
	NULL,
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeLabel
 */

static void 
FreeLabel(LabelGizmoP * g)
{
	FreeGizmoArray(g->gizmos, g->numGizmos);
	FREE(g);
}

/*
 * CreateLabel: If a GizmoArray has been passed in, create a form to
 *              contain the Label and the GizmoArray.  Otherwise, just	
 *				create an XmLabel.
 */

static Gizmo 
CreateLabel(Widget parent, LabelGizmo * g, Arg * args, int num)
{
	LabelGizmoP *	label;
	Widget		container;
	Widget		label_parent;
	XmString	string;
	int		i;
	Arg		arg[10];



	label = (LabelGizmoP *)CALLOC(1, sizeof(LabelGizmoP));
	label->name = g->name;
	label->dontAlignLabel = g->dontAlignLabel;

	if ( g->numGizmos > 0){
	    label->form = label_parent = XtCreateManagedWidget (
					  g->name, xmFormWidgetClass, parent, NULL, 0 );
	}
	else{
	    label_parent = parent;
	}
	string = XmStringCreateLocalized (GGT(g->label));
	i = 0;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	i = AppendArgsToList(arg, i, args, num);
	label->label = XtCreateManagedWidget (
		"label", xmLabelGadgetClass, label_parent,
		arg, i
	);
	XmStringFree(string);
	if (g->help != NULL) {
		GizmoRegisterHelp(label->label, g->help);
	}


	label->numGizmos = g->numGizmos;
	if (label->numGizmos > 0){
		int last = label->numGizmos - 1;
		Widget first_w, last_w;

		/* MORE: Attach the GizmoArray to the XmNLabel on the
		 * left and the Form edge on the right
		 */
		label->gizmos = CreateGizmoArray (label_parent, g->gizmos, 
										  g->numGizmos );

		first_w = GetAttachmentWidget(label->gizmos[0].gizmoClass, 
									  label->gizmos[0].gizmo );
		switch(g->labelPosition) {
		case G_TOP_LABEL:
			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
			XtSetValues(label->label, arg, i);

			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_WIDGET); i++;
			XtSetArg(arg[i], XmNtopWidget, label->label); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			break;
		case G_BOTTOM_LABEL:
			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_WIDGET); i++;
			XtSetArg(arg[i], XmNtopWidget, first_w); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNalignment, XmALIGNMENT_CENTER); i++;
			XtSetValues(label->label, arg, i);

			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			break;
		case G_RIGHT_LABEL:
			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
			XtSetValues(label->label, arg, i);

			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_WIDGET); i++;
			XtSetArg(arg[i], XmNrightWidget, label->label); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			break;
		default: /* G_LEFT_LABEL */
			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
			XtSetValues(label->label, arg, i);

			i = 0;
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
			XtSetArg(arg[i], XmNleftWidget, label->label); i++;
			break;
		}
		XtSetValues(first_w, arg, i);

		last_w = GetAttachmentWidget(label->gizmos[last].gizmoClass, 
									 label->gizmos[last].gizmo);
		XtSetArg(arg[0], XmNrightAttachment, XmATTACH_FORM);
		XtSetValues(last_w, arg, 1);

	}
	return (Gizmo)label;
}

/*
 * DumpLabel
 */

static void
DumpLabel(LabelGizmoP * g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "label(%s) = 0x%x 0x%x\n", g->name, g, g->label);
	DumpGizmoArray(g->gizmos, g->numGizmos, indent+1);
}

/*
 * ManipulateLabel
 */
static void
ManipulateLabel(LabelGizmoP * g, ManipulateOption option)
{
	GizmoArray	gp = g->gizmos;
	int             i;

	for (i = 0; i < g->numGizmos; i++) {
		ManipulateGizmo(gp[i].gizmoClass, gp[i].gizmo, option);
	}
}

/*
 * QueryLabel
 */
static XtPointer
QueryLabel(LabelGizmoP * g, int option, char * name)
{
	XtPointer	value;

	if (!name || !g->name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->label);
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
			}
			default: {
				return NULL;
			}
		}
	}
	else {
		return (XtPointer)QueryGizmoArray (
			g->gizmos, g->numGizmos, option, name
		);
	}
}

/*
 *	AttachmentWidget: return the widget to set attachments on when 
 * 		placing a LabelGizmo inside a Form.  If there is no GizmoArray
 *		this will be the XmLabel widget itself; otherwise, it will
 *		be the Form that contains the Label and the GizmoArray.
 */
static Widget
AttachmentWidget(LabelGizmoP *g)
{

	if (g->form)
		return g->form;
	else
		return g->label;
}
