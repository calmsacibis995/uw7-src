#ifndef NOIDENT
#pragma ident	"@(#)SWGizmo.c	15.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "SWGizmo.h"

extern Widget		CreateScrolledWindowGizmo();
extern void		FreeScrolledWindowGizmo();
extern Gizmo		CopyScrolledWindowGizmo();
static XtPointer	QueryScrolledWindowGizmo();

GizmoClassRec ScrolledWindowGizmoClass[] = {
	"ScrolledWindowGizmo",
	CreateScrolledWindowGizmo,	/* Create	*/
	CopyScrolledWindowGizmo,	/* Copy		*/
	FreeScrolledWindowGizmo,	/* Free		*/
	NULL,				/* Map		*/
	NULL,				/* Get		*/
	NULL,				/* Get Menu	*/
	NULL,				/* Build	*/
	NULL,				/* Manipulate	*/
	QueryScrolledWindowGizmo	/* Query	*/
};

static Gizmo
CopyScrolledWindowGizmo (gizmo)
ScrolledWindowGizmo *	gizmo;
{
	ScrolledWindowGizmo * new = (ScrolledWindowGizmo *) MALLOC (
		sizeof (ScrolledWindowGizmo)
	);

	new->name = gizmo->name;
	new->width = gizmo->width;
	new->height = gizmo->height;

	return (Gizmo)new;
}

static void
FreeScrolledWindowGizmo (gizmo)
ScrolledWindowGizmo *	gizmo;
{
	FREE (gizmo);
}

static Widget
CreateScrolledWindowGizmo (parent, g)
Widget			parent;
ScrolledWindowGizmo *	g;
{
	Arg	args[10];

	XtSetArg (args[0], XtNwidth, g->width);
	XtSetArg (args[1], XtNheight, g->height);
	g->sw = XtCreateManagedWidget (
		"scrolled window gizmo",
		scrolledWindowWidgetClass,
		parent,
		args,
		2
	);
	return g->sw;
}

static XtPointer
QueryScrolledWindowGizmo (ScrolledWindowGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->sw);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(gizmo);
				break;
			}
		}
	}
	else {
		return (XtPointer)NULL;
	}
}
