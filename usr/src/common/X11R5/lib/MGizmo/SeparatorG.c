#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SeparatorG.c	1.2"
#endif

#include <stdio.h>

#include <Xm/Separator.h>

#include "Gizmo.h"
#include "SeparatorP.h"

/* The code in this file produces a separator gizmo
 * the specify horizontal and vertical spacing betwix gizmos
 */

static void	FreeSeparatorGizmo(SeparatorGizmoP *);
static void	DumpSeparatorGizmo(SeparatorGizmoP *, int);
static Gizmo	CreateSeparatorGizmo(Widget, SeparatorGizmo *, Arg *, int);
static Widget	AttachmentWidget();
static XtPointer	QuerySeparatorGizmo(SeparatorGizmoP *, int, char *);

GizmoClassRec SeparatorGizmoClass[] = {
	"SeparatorGizmo",
	CreateSeparatorGizmo,	/* Create */
	FreeSeparatorGizmo,	/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpSeparatorGizmo,	/* Dump */
	NULL,			/* Manipulate */
	QuerySeparatorGizmo,	/* Query */
	NULL,			/* Set value by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeSeparatorGizmo
 */

static void
FreeSeparatorGizmo(SeparatorGizmoP *g)
{
	FREE(g);
}

/*
 * CreateSeparatorGizmo
 */

static Gizmo
CreateSeparatorGizmo(Widget parent, SeparatorGizmo *g, Arg *args, int numArgs)
{
	Arg			arg[100];
	SeparatorGizmoP *	separator;
	int			i = 0;

	separator = (SeparatorGizmoP *)CALLOC(1, sizeof(SeparatorGizmoP));

	separator->name = g->name;
	XtSetArg(arg[i], XmNheight, g->height); i++;
	XtSetArg(arg[i], XmNwidth,  g->width); i++;
	XtSetArg(arg[i], XmNseparatorType, g->type); i++;
	XtSetArg(arg[i], XmNorientation, g->orientation); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	separator->w = XtCreateManagedWidget(
		"separator", xmSeparatorWidgetClass, parent, arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(separator->w, g->help);
	}

	return (Gizmo)separator;
}

static void
DumpSeparatorGizmo(SeparatorGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "separator(%s) = 0x%x 0x%x\n", g->name, g, g->w);
}

static XtPointer
QuerySeparatorGizmo(SeparatorGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)g->w;
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
		return NULL;
	}
}

static Widget
AttachmentWidget(SeparatorGizmoP *g)
{
	return g->w;
}
