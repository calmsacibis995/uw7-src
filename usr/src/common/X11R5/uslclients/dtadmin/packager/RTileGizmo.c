#ifndef NOIDENT
#pragma ident	"@(#)RTileGizmo.c	15.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xol/Footer.h>
#include <Gizmo/Gizmos.h>
#include "RTileGizmo.h"

extern Widget		CreateRubberTileGizmo();
extern void		FreeRubberTileGizmo();
extern Gizmo		CopyRubberTileGizmo();
static XtPointer	QueryRubberTileGizmo();

GizmoClassRec RubberTileGizmoClass[] = {
	"RubberTileGizmo",
	CreateRubberTileGizmo,	/* Create	*/
	CopyRubberTileGizmo,	/* Copy		*/
	FreeRubberTileGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryRubberTileGizmo	/* Query	*/
};

static Gizmo
CopyRubberTileGizmo (gizmo)
RubberTileGizmo *	gizmo;
{
	RubberTileGizmo * new = (RubberTileGizmo *) MALLOC (
		sizeof (RubberTileGizmo)
	);

	new->name = gizmo->name;
	new->orientation = gizmo->orientation;
	new->footer = gizmo->footer;
	CopyGizmoArray (
		&new->gizmos, &new->numGizmos,
		gizmo->gizmos, gizmo->numGizmos);

	return (Gizmo)new;
}

static void
FreeRubberTileGizmo (gizmo)
RubberTileGizmo *	gizmo;
{
	FREE (gizmo);
}

static Widget
CreateRubberTileGizmo (parent, g)
Widget			parent;
RubberTileGizmo *	g;
{
	Arg	args[10];
	int	i = 0;

	XtSetArg (args[i], XtNweight, 1); i++;
	XtSetArg (args[i], XtNorientation, g->orientation); i++;
	g->rubberTile = XtCreateManagedWidget (
		"rubber tile gizmo",
		rubberTileWidgetClass,
		parent,
		args,
		i
	);
	CreateGizmoArray(g->rubberTile, g->gizmos, g->numGizmos);

	if (g->footer != NULL) {
		g->message = XtVaCreateManagedWidget (
			g->footer, footerWidgetClass, g->rubberTile,
			XtNweight,	0,
			XtNleftWeight,	99,
			XtNrightWeight,	1,
			(String)0
		);
	}
	else {
		g->message = (Widget)0;
	}

	return g->rubberTile;
}

static XtPointer
QueryRubberTileGizmo (RubberTileGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->rubberTile);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(gizmo);
				break;
			}
		}
	}
	else {
		return QueryGizmoArray (
			gizmo->gizmos, gizmo->numGizmos, option, name
		);
	}
}

Widget
GetRubberTileFooter (RubberTileGizmo *g)
{
	return g->message;
}
