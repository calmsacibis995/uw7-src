#ifndef NOIDENT
#ident	"@(#)MGizmo:ContGizmo.c	1.5"
#endif

#include <stdio.h>

#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PanedW.h>

#include "Gizmo.h"
#include "ContGizmoP.h"
#include "LabelGizmP.h"

extern Gizmo		CreateContainerGizmo();
extern Gizmo		SetContainerValueByName();
extern void		FreeContainerGizmo();
static XtPointer	QueryContainerGizmo();
extern void		DumpContainerGizmo();
static void		AlignCaptions();
static Widget		AttachmentWidget();
static void		ManipulateContainerGizmo();

GizmoClassRec ContainerGizmoClass[] = {
	"ContainerGizmo",
	CreateContainerGizmo,		/* Create */
	FreeContainerGizmo,		/* Free */
	NULL,				/* Map */
	NULL,				/* Get */
	NULL,				/* Get Menu */
	DumpContainerGizmo,		/* Dump	*/
	ManipulateContainerGizmo,	/* Manipulate */
	QueryContainerGizmo,		/* Query */
	SetContainerValueByName,	/* Set value by name */
	AttachmentWidget		/* For attachments in base window */
};

static void
FreeContainerGizmo(Gizmo g)
{
	ContainerGizmoP *	gp = (ContainerGizmoP *)g;

	FreeGizmoArray(gp->gizmos, gp->numGizmos);
	FREE(gp);
}

static Gizmo
CreateScrolledWindowGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;

	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"scrolled window", xmScrolledWindowWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreateBulletinBoardGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;
	
	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"bulletin board", xmBulletinBoardWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreateRowColGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;

	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"row column", xmRowColumnWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreateFormGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;

	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"form", xmFormWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreateFrameGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;

	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"frame", xmFrameWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreatePanedWindowGizmo(
	Widget parent, ContainerGizmo *g, ContainerGizmoP *new,
	Arg *args, int numArgs
)
{
	Arg	arg[100];
	int	i = 0;

	XtSetArg(arg[i], XmNwidth, g->width); i++;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	new->w = XtCreateManagedWidget(
		"panedwindow", xmPanedWindowWidgetClass, parent, arg, i
	);
	return new;
}

static Gizmo
CreateContainerGizmo(Widget parent, ContainerGizmo *g, Arg *args, int numArgs)
{
	ContainerGizmoP *	new;
	new = (ContainerGizmoP *)CALLOC(1, sizeof(ContainerGizmoP));
	new->name = g->name;
	new->type = g->type;

	switch (g->type) {
		case G_CONTAINER_SW: {
			new = CreateScrolledWindowGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
		case G_CONTAINER_BB: {
			new = CreateBulletinBoardGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
		case G_CONTAINER_RC: {
			new = CreateRowColGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
		case G_CONTAINER_FORM: {
			new = CreateFormGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
		case G_CONTAINER_FRAME: {
			new = CreateFrameGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
		case G_CONTAINER_PANEDW: {
			new = CreatePanedWindowGizmo(
				parent, g, new, args, numArgs
			);
			break;
		}
	}
	if (g->numGizmos != 0) {
		new->numGizmos = g->numGizmos;
		new->gizmos = CreateGizmoArray(
			new->w, g->gizmos, g->numGizmos
		);
		AlignCaptions(new->gizmos, new->numGizmos);
	}
	if (g->help != NULL) {
		GizmoRegisterHelp(new->w, g->help);
	}
	return new;
}

static XtPointer
QueryContainerGizmo(ContainerGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->w);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
				break;
			}
		}
	}
	else {
		return (XtPointer)QueryGizmoArray(
			g->gizmos, g->numGizmos, option, name
		);
	}
}

static void
DumpContainerGizmo(ContainerGizmoP *g, int indent)
{
	char *	type;

	switch (g->type) {
		case G_CONTAINER_FRAME: {
			type = "frame";
			break;
		}
		case G_CONTAINER_SW: {
			type = "scrolled window";
			break;
		}
		case G_CONTAINER_BB: {
			type = "bulletin board";
			break;
		}
		case G_CONTAINER_RC: {
			type = "row column";
			break;
		}
		case G_CONTAINER_FORM: {
			type = "form";
			break;
		}
	}
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "%s(%s) = 0x%x 0x%x\n", type, g->name, g, g->w);
	DumpGizmoArray(g->gizmos, g->numGizmos, indent+1);
}

static void
AlignCaptions(GizmoArray gizmos, int numGizmos)
{
	GizmoRec *	g;
	Dimension	maxWidth = 0;
	Dimension	w;
	Arg		arg[1];

	XtSetArg(arg[0], XmNwidth, &w);
	for (g = gizmos; g < &gizmos[numGizmos]; g++) {
		if (g->gizmoClass == LabelGizmoClass) {
			if (((LabelGizmoP *)g->gizmo)->dontAlignLabel) {
				continue;
			}
			XtGetValues(((LabelGizmoP *)g->gizmo)->label, arg, 1);
			if (w > maxWidth) {
				maxWidth = w;
			}
		}
	}

	XtSetArg(arg[0], XmNwidth, maxWidth);
	for (g = gizmos; g < &gizmos[numGizmos]; g++) {
		if (g->gizmoClass == LabelGizmoClass) {
			if (((LabelGizmoP *)g->gizmo)->dontAlignLabel) {
				continue;
			}
			XtSetValues(((LabelGizmoP *)g->gizmo)->label, arg, 1);
		}
	}
}

static Gizmo
SetContainerValueByName(ContainerGizmoP *g, char *name, char *value)
{
	return SetGizmoArrayByName(g->gizmos, g->numGizmos, name, value);
}

static Widget
AttachmentWidget(ContainerGizmoP *g)
{
	return g->w;
}

static void
ManipulateContainerGizmo(ContainerGizmoP *g, ManipulateOption option)
{
	GizmoArray	gp = g->gizmos;
	int i;

	for (i=0; i<g->numGizmos; i++) {
		ManipulateGizmo(gp[i].gizmoClass, gp[i].gizmo, option);
	}
}
