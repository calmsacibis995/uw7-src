#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ListGizmo.c	1.3"
#endif

#include <stdio.h>
#include <Xm/List.h>
#include "Gizmo.h"
#include "ListGizmoP.h"

static Gizmo		CreateListGizmo();
static void		FreeListGizmo();
static void		DumpListGizmo();
static XtPointer	QueryListGizmo();
static Widget		AttachmentWidget();

GizmoClassRec ListGizmoClass[] = {
	"ListGizmo",
	CreateListGizmo,
	FreeListGizmo,
	NULL,		/* map */
	NULL,		/* get */
	NULL,		/* get menu */
	DumpListGizmo,
	NULL,		/* manipulate */
	QueryListGizmo,
	NULL,		/* set value by name */
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeListGizmo
 */
static void 
FreeListGizmo(ListGizmoP *g)
{
	FREE(g);
}

/*
 * CreateListGizmo
 */
static Gizmo
CreateListGizmo(Widget parent, ListGizmo *g, Arg *args, int num)
{
	ListGizmoP *	list;
	Arg		arg[100];
	XmString	str[100];
	XmString *	strings;
	int		i;

	list = (ListGizmoP *)CALLOC(1, sizeof(ListGizmoP));
	list->name = g->name;

	/* Make localized strings, alloc space if needed */
	strings = str;
	if (g->numItems > 100) {
		strings = (XmString *)MALLOC(sizeof(XmString)*g->numItems);
	}
	for (i=0; i<g->numItems; i++) {
		strings[i] = XmStringCreateLocalized(GGT(g->items[i]));
	}

	i = 0;
	XtSetArg(arg[i], XmNitems, g->numItems ? strings : NULL); i++;
	XtSetArg(arg[i], XmNitemCount, g->numItems); i++;
	XtSetArg(arg[i], XmNvisibleItemCount, g->visible); i++;
	i = AppendArgsToList(arg, i, args, num);
	list->listWidget = XtCreateManagedWidget(
		list->name, xmListWidgetClass, parent,
		arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(list->listWidget, g->help);
	}
	if (g->callback != NULL) {
		XtAddCallback(
			list->listWidget,
			XmNdefaultActionCallback,
			(XtCallbackProc)g->callback,
			(XtPointer)g->clientData
		);
	}

	if (strings != str) {
		FREE(strings);
	}

	return (Gizmo)list;
}

static void
DumpListGizmo(ListGizmoP *g)
{
	fprintf(stderr, "list(%s) = 0x%x 0x%x\n", g->name, g, g->listWidget);
}

static XtPointer
QueryListGizmo(ListGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->listWidget);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
				break;
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

static Widget
AttachmentWidget(ListGizmoP *g)
{
	return g->listWidget;
}
