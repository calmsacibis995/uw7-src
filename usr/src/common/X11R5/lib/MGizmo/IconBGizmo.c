#ifndef NOIDENT
#ident	"@(#)MGizmo:IconBGizmo.c	1.1"
#endif

#include <stdio.h>

#include <Xm/ScrolledW.h>
#include <DtI.h>

#include "Gizmo.h"
#include "IconBGizmoP.h"
#include <X11/Xatom.h>
#include "DtI.h"
#include "FIconBoxP.h"	/* to get fontlist */
#include <Xm/DrawP.h>
#include "FIconbox.h"

extern Gizmo		CreateIconBoxGizmo();
extern void		FreeIconBoxGizmo();
static XtPointer	QueryIconBoxGizmo();
static Widget		AttachmentWidget();
extern void		DumpIconBoxGizmo();

GizmoClassRec IconBoxGizmoClass[] = {
	"IconBoxGizmo",
	CreateIconBoxGizmo,		/* Create */
	FreeIconBoxGizmo,		/* Free */
	NULL,				/* Map */
	NULL,				/* Get */
	NULL,				/* Get Menu */
	DumpIconBoxGizmo,		/* Dump	*/
	NULL,				/* Manipulate */
	QueryIconBoxGizmo,		/* Query */
	NULL,				/* Set value by name */
	AttachmentWidget		/* For attachments in base window */
};

#define OFFSET(field)   XtOffsetOf(Options, field)

static XtResource resources[] = {
	{
		XtNfolderCols, XtCCols, XtRUnsignedChar, sizeof(u_char),
    		OFFSET(folderCols), XtRImmediate,
		(XtPointer)DEFAULT_FOLDER_COLS
	},
	{
		XtNfolderRows, XtCRows, XtRUnsignedChar, sizeof(u_char),
    		OFFSET(folderRows), XtRImmediate,
		(XtPointer)DEFAULT_FOLDER_ROWS
	},
	{
		XtNgridWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    		OFFSET(gridWidth), XtRImmediate,
		(XtPointer)DEFAULT_GRID_WIDTH
	},
	{
		XtNgridHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    		OFFSET(gridHeight), XtRImmediate,
		(XtPointer)DEFAULT_GRID_HEIGHT
	},
};

static void
GetSwinSize(IconBoxGizmo *g, IconBoxGizmoP *gp)
{
	gp->height = (g->height) ? g->height : (
		gp->opt.gridHeight*gp->opt.folderRows
	);
	gp->width = (g->width) ? g->width : (
		gp->opt.gridWidth*gp->opt.folderCols
	);
}

static void
FreeIconBoxGizmo(Gizmo g)
{
	IconBoxGizmoP *	gp = (IconBoxGizmoP *)g;

	FREE(gp);
}

static String fields[] = {
	XmNlabelString,
	XmNx,
	XmNy,
	XmNwidth,
	XmNheight,
	XmNmanaged,
	XmNset,
	XmNsensitive,
	XmNuserData,
	XmNobjectData
};

static Gizmo
CreateIconBoxGizmo(Widget parent, IconBoxGizmo *ibg, Arg *args, int numArgs)
{
	IconBoxGizmoP *	gp;
	Arg		arg[100];
	int		i = 0;

	gp = (IconBoxGizmoP *)CALLOC(1, sizeof(IconBoxGizmoP));
	gp->name = ibg->name;
	gp->select = ibg->select;
	gp->dblSelect = ibg->dblSelect;
	gp->draw = ibg->draw;
	gp->items = ibg->items;
	gp->numItems = ibg->numItems;

	XtGetApplicationResources(
		parent, &gp->opt, resources, XtNumber(resources), NULL, 0
	);

	GetSwinSize(ibg, gp);
	i = 0;
	XtSetArg(arg[i], XmNwidth,		gp->width); i++;
	XtSetArg(arg[i], XmNheight,		gp->height); i++;
	XtSetArg(arg[i], XmNleftAttachment,	XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNrightAttachment,	XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNleftOffset,		4); i++;
	XtSetArg(arg[i], XmNrightOffset,	4); i++;
	XtSetArg(arg[i], XmNshadowThickness,	2); i++;
	XtSetArg(arg[i], XmNscrollingPolicy,	XmAPPLICATION_DEFINED); i++;
	XtSetArg(arg[i], XmNvisualPolicy,	XmVARIABLE); i++;
	XtSetArg(arg[i], XmNscrollBarDisplayPolicy,XmSTATIC);i++;
	gp->swin = XtCreateManagedWidget(
		"scrolled window", xmScrolledWindowWidgetClass, parent, arg, i
	);

	i = 0;
	XtSetArg(arg[i], XmNselectProc,(XtArgVal)gp->select); i++;
	XtSetArg(arg[i], XmNdblSelectProc,(XtArgVal)gp->dblSelect); i++;
#ifdef davef
	XtSetArg(arg[i], XmNmaxItemHeight,(XtArgVal)gp->opt.gridHeight); i++;
	XtSetArg(arg[i], XmNmaxItemWidth,(XtArgVal)gp->opt.gridWidth); i++;
#endif /* davef */
	XtSetArg(arg[i], XmNgridRows,(XtArgVal)gp->opt.folderRows);; i++;
	XtSetArg(arg[i], XmNgridColumns,(XtArgVal)gp->opt.folderCols); i++;
	XtSetArg(arg[i], XmNitemFields, fields); i++;
	XtSetArg(arg[i], XmNnumItemFields, XtNumber(fields)); i++;
	XtSetArg(arg[i], XmNdragCursorProc, DmCreateIconCursor); i++;
	XtSetArg(arg[i], XmNitems, gp->items); i++;
	XtSetArg(arg[i], XmNnumItems, gp->numItems); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	gp->box = XtCreateManagedWidget(
		"flat", exmFlatIconboxWidgetClass, gp->swin, arg, i
	);

	if (ibg->help != NULL) {
		GizmoRegisterHelp(gp->box, ibg->help);
	}
	return gp;
}

void
ResetIconBoxGizmo(Gizmo g, IconBoxGizmo *ibg, Arg *args, int numArgs)
{
	IconBoxGizmoP *	gp = (IconBoxGizmoP *)g;
	int		i = 0;
	Arg		arg[10];

	gp->numItems = ibg->numItems;
	gp->items = ibg->items;

	i = 0;
	XtSetArg(arg[i], XmNitemFields, fields); i++;
	XtSetArg(arg[i], XmNnumItemFields, XtNumber(fields)); i++;
	XtSetArg(arg[i], XmNitems, gp->items); i++;
	XtSetArg(arg[i], XmNnumItems, gp->numItems); i++;
	XtSetArg(arg[i], XmNitemsTouched, True); i++;
	XtSetValues(gp->box, arg, i);
}

static XtPointer
QueryIconBoxGizmo(IconBoxGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->box);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
				break;
			}
		}
	}
}

static void
DumpIconBoxGizmo(IconBoxGizmoP *g, int indent)
{
	char *	type;

	fprintf(stderr, "iconbox(%s) = 0x%x 0x%x\n", g->name, g, g->box);
}

Widget
GetIconBoxSW(Gizmo g)
{
	return ((IconBoxGizmoP *)g)->swin;
}

Dimension
GetIconBoxWidth(Gizmo g)
{
	return ((IconBoxGizmoP *)g)->width;
}

Dimension
GetIconBoxHeight(Gizmo g)
{
	return ((IconBoxGizmoP *)g)->height;
}

static Widget
AttachmentWidget(Gizmo g)
{
	return ((IconBoxGizmoP *)g)->swin;
}
