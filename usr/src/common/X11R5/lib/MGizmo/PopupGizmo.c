#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:PopupGizmo.c	1.11"
#endif

#include <stdio.h>

#include <Xm/SelectioB.h>
#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/BulletinB.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include <Xm/Separator.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"
#include "PopupGizmP.h"

static Gizmo		CreatePopupGizmo();
static Gizmo		SetPopupValueByName();
static void		FreePopupGizmo();
static void		MapPopupGizmo();
static Gizmo		GetTheMenuGizmo();
static void		ManipulatePopupGizmo();
static XtPointer	QueryPopupGizmo();
static void		DumpPopupGizmo();

GizmoClassRec PopupGizmoClass[] = { 
	"PopupGizmo",
	CreatePopupGizmo,	/* Create */
	FreePopupGizmo, 	/* Free */
	MapPopupGizmo,		/* Map */
	NULL,			/* Get */
	GetTheMenuGizmo,	/* Get menu */
	DumpPopupGizmo,		/* Dump */
	ManipulatePopupGizmo,	/* Manipulate */
	QueryPopupGizmo,	/* Query */
	SetPopupValueByName,	/* Set value by name */
	NULL			/* Attachment */
};

/*
 * FreePopupGizmo
 */

static void 
FreePopupGizmo(Gizmo g)
{
	PopupGizmoP *	gp = (PopupGizmoP *)g;

	FreeGizmo(CommandMenuGizmoClass, gp->menu);
	FreeGizmoArray(gp->gizmos, gp->numGizmos);
	FREE(gp);

}

/*
 * CreatePopupGizmo
 *
 * The CreatePopupGizmo function is used to create the Widget tree
 * defined by the PopupGizmo structure p.  parent is the
 * Widget parent of this new Widget tree.  args and num,
 * if non-NULL, are used as Args in the creation of the popup window
 * Widget which is returned by this function.
 */

static Gizmo
CreatePopupGizmo(Widget parent, PopupGizmo *g, Arg *args, int numArgs)
{
	PopupGizmoP *	gp = (PopupGizmoP *)CALLOC(1, sizeof(PopupGizmoP));
	Arg		arg[100];
	int		i;
	long		dec;
	long		func;
	Widget		form;
	DmMnemonicInfo	mneInfo;
	Cardinal	numMne;

	gp->name = g->name;
	i = 0;
	dec = MWM_DECOR_TITLE|MWM_DECOR_BORDER|MWM_DECOR_MENU;
	func = MWM_FUNC_MOVE|MWM_FUNC_CLOSE;
	XtSetArg(arg[i], XmNtitle, GGT(g->title)); i++;
	XtSetArg(arg[i], XmNallowShellResize, True); i++;
	XtSetArg(arg[i], XmNmwmDecorations, dec); i++;
	XtSetArg(arg[i], XmNmwmFunctions, func); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	gp->shell = XtCreatePopupShell(
		g->name, xmDialogShellWidgetClass, parent, arg, i
	);

	form = XtVaCreateWidget(
		"form", xmFormWidgetClass, gp->shell, 
		XmNresizePolicy, XmRESIZE_ANY,
		NULL
	);
	gp->rowColumn = XtVaCreateManagedWidget(
		"top rc", xmRowColumnWidgetClass, form,
		NULL
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->rowColumn, g->help);
	}
	gp->workArea = XtCreateManagedWidget(
		"workArea", xmRowColumnWidgetClass, gp->rowColumn, NULL, 0
	);
	if (g->menu != NULL) {
		Widget menuArea;

		menuArea = XtCreateManagedWidget(
			"menuArea", xmRowColumnWidgetClass, gp->rowColumn,
			NULL, 0);
		gp->menu = _CreateActionMenu(
			menuArea, g->menu, NULL, 0, &mneInfo, &numMne
		);
	}
	if (g->numGizmos > 0) {
		gp->numGizmos = g->numGizmos;
		gp->gizmos = CreateGizmoArray(
			gp->workArea, g->gizmos, g->numGizmos
		);
	}
	/* Create the footer and separator if requested */
	if (g->footer != NULL) {
		Widget msgArea;

		msgArea = XtCreateManagedWidget(
			"msgArea", xmRowColumnWidgetClass, gp->rowColumn,
			NULL, 0);
		if (g->separatorType != XmNO_LINE) {
			Widget separator;

			i = 0;
			XtSetArg(arg[i], XmNseparatorType, g->separatorType);
				i++;
			XtSetArg(arg[i], XmNorientation, XmHORIZONTAL); i++;
			separator = XtCreateManagedWidget(
				"separator", xmSeparatorWidgetClass,
				msgArea, arg, i
			);
		}
		gp->footer = CreateGizmo(
			msgArea, MsgGizmoClass, g->footer, NULL, 0
		);
	}
	if (g->menu != NULL) {
		DmRegisterMnemonic(gp->shell, mneInfo, numMne);
		FREE(mneInfo);
	}
	return (Gizmo)gp;
}

/*
 * BringDownPopup
 *
 * The BringDownPopup procedure is used to popdown a popup
 * shell Widget widget.
 */
void
BringDownPopup(Widget wid)
{
	XtPopdown(wid);
}

/*
 * GetTheMenuGizmo
 */
static Gizmo 
GetTheMenuGizmo(Gizmo g)
{
	return ((PopupGizmoP *)g)->menu;
}

/*
 * GetPopupGizmoShell
 */
Widget
GetPopupGizmoShell(Gizmo g)
{
	return ((PopupGizmoP *)g)->shell;
}

/*
 * GetPopupGizmoRowCol
 */
Widget
GetPopupGizmoRowCol(Gizmo g)
{
	return ((PopupGizmoP *)g)->rowColumn;
}

/*
 * MapPopupGizmo
 */
static void 
MapPopupGizmo(PopupGizmoP *g)
{
	Widget	shell = g->shell;


	XtManageChild(XtParent(g->rowColumn));
	_SetMenuDefault(g->menu, XtParent(g->rowColumn));
	XmProcessTraversal(g->workArea, XmTRAVERSE_CURRENT);
	XtPopup(shell, XtGrabNone);
	XRaiseWindow(XtDisplay(shell), XtWindow(shell));
}

/*
 * ManipulatePopupGizmo
 */
static void
ManipulatePopupGizmo(PopupGizmoP *g, ManipulateOption option)
{
	GizmoArray	gp = g->gizmos;
	int i;

	for (i=0; i<g->numGizmos; i++) {
		ManipulateGizmo(gp[i].gizmoClass, gp[i].gizmo, option);
	}
}

/*
 * QueryPopupGizmo
 */
static XtPointer
QueryPopupGizmo(PopupGizmoP *g, int option, char *name)
{
	XtPointer	value = NULL;

	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->workArea);
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
		if (g->menu) {
			value = QueryGizmo(
				CommandMenuGizmoClass, g->menu, option, name
			);
		}
		if (value) {
			return value;
		}
		else {
			return (XtPointer)QueryGizmoArray(
				g->gizmos, g->numGizmos, option, name
			);
		}
	}
}

static void
DumpPopupGizmo(PopupGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "popup(%s) = 0x%x 0x%x\n", g->name, g, g->shell);
	DumpGizmoArray(g->gizmos, g->numGizmos, indent+1);
	DumpGizmo(CommandMenuGizmoClass, g->menu, indent+1);
}

static Gizmo
SetPopupValueByName(PopupGizmoP *g, char *name, char *value)
{
	return SetGizmoArrayByName(
		g->gizmos, g->numGizmos, name, value
	);
}

void
SetPopupWindowLeftMsg(Gizmo g, char *msg)
{
	PopupGizmoP *gp = (PopupGizmoP *)g;

	SetMsgGizmoTextLeft((Gizmo)(gp->footer), msg);
}

void
SetPopupWindowRightMsg(Gizmo g, char *msg)
{
	PopupGizmoP *gp = (PopupGizmoP *)g;

	SetMsgGizmoTextRight((Gizmo)(gp->footer), msg);
}
