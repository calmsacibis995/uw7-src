#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SpaceGizmo.c	1.2"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/RectObj.h>

#include "Gizmo.h"
#include "SpaceGizmP.h"

/* The code in this file produces a space gizmo
 * the specify horizontal and vertical spacing betwix gizmos
 */

static void	FreeSpaceGizmo(SpaceGizmoP *g);
static void	DumpSpaceGizmo(SpaceGizmoP *g, int);
static Gizmo	CreateSpaceGizmo(Widget parent, SpaceGizmo *g, Arg *, int);
static Widget	AttachmentWidget();

GizmoClassRec SpaceGizmoClass[] = {
	"SpaceGizmo",
	CreateSpaceGizmo,	/* Create */
	FreeSpaceGizmo,		/* Free */
	NULL,			/* Map */
	NULL,			/* Get */
	NULL,			/* Get Menu */
	DumpSpaceGizmo,		/* Dump */
	NULL,			/* Manipulate */
	NULL,			/* Query */
	NULL,			/* Set value by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeSpaceGizmo
 */

static void
FreeSpaceGizmo(SpaceGizmoP *g)
{
	FREE(g);
}

/*
 * CreateSpaceGizmo
 */

static Gizmo
CreateSpaceGizmo(Widget parent, SpaceGizmo *g, Arg *args, int numArgs)
{
	Arg		arg[100];
	Dimension	height = g->height;
	Dimension	width  = g->width;
	SpaceGizmoP *	space;
	int		i = 0;

	space = (SpaceGizmoP *)CALLOC(1, sizeof(SpaceGizmoP));

	space->name = g->name;
	XtSetArg(arg[i], XtNheight, height); i++;
	XtSetArg(arg[i], XtNwidth,  width); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	space->rectObj = XtCreateManagedWidget(
		"rectObj", rectObjClass, parent, arg, i
	);

	return (Gizmo)space;
}

static void
DumpSpaceGizmo(SpaceGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "space(%s) = 0x%x 0x%x\n", g->name, g, g->rectObj);
}

static Widget
AttachmentWidget(SpaceGizmoP *g)
{
	return g->rectObj;
}
