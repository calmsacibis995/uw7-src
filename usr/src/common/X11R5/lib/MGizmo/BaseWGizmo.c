#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:BaseWGizmo.c	1.7"
#endif

/*
 * BaseWGizmo.c
 */

#include <stdio.h>

#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <DtI.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"
#include "BaseWGizmP.h"
#include "MsgGizmoP.h"

static Gizmo		CreateBaseWindow();
static void		FreeBaseWindow();
static void		DumpBaseWindow();
static Gizmo		SetBaseWindowValue();
static void		RealizeBaseWindow();
static void		ManipulateBaseWindow();
static XtPointer	QueryBaseWindow();

GizmoClassRec BaseWindowGizmoClass[] = { 
	"BaseWindowGizmo", 
	CreateBaseWindow, 
	FreeBaseWindow, 
	RealizeBaseWindow,
	NULL/*GetBaseWindowGizmo*/,
	NULL/*GetTheMenuGizmo*/,
	DumpBaseWindow,
	ManipulateBaseWindow,
	QueryBaseWindow,
	SetBaseWindowValue,
	NULL/* Attachment */
};

/*
 * FreeBaseWindow
 */

static void 
FreeBaseWindow(BaseWindowGizmoP *g)
{
	FreeGizmo(MenuBarGizmoClass, g->menu);
	FreeGizmoArray(g->gizmos, g->numGizmos);
	/*
	if (g->iconShell != NULL) {
		XtDestroyWidget(g->iconShell);
	}
	*/
	FREE(g);
}

void
SetBaseWindowGizmoPixmap(Gizmo g, char *name)
{
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;
	Arg			arg[10];
	Pixmap			iconPixmap;
	int			width;
	int			height;
	int			i = 0;

	if (name != NULL) {
		iconPixmap = PixmapOfFile(
			gp->iconShell, name, &width, &height
		);
		XtSetArg(arg[i], XtNbackgroundPixmap, iconPixmap); i++;
		XtSetArg(arg[i], XtNwidth, width); i++;
		XtSetArg(arg[i], XtNheight, height); i++;
		XtSetValues(gp->iconShell, arg, i);
	}
}

/*
 * CreateBaseWindow
 */
static Gizmo 
CreateBaseWindow(Widget parent, BaseWindowGizmo *g, Arg *args, int num)
{
	BaseWindowGizmoP *	gp;
	XmString		string;
	int			i;
	Arg			arg[100];

	/* Alloc the space and create the shell for the base window. */
	gp = (BaseWindowGizmoP *)CALLOC(1, sizeof(BaseWindowGizmoP));
	gp->name = g->name;

	/* Create the icon shell if needed */

	i = 0;
	if (g->iconPixmap != NULL) {
	    /* icon name and title are set on base window shell, not here */
		XtSetArg(arg[i], XtNborderWidth, 0); i++;
		XtSetArg(arg[i], XtNtranslations, XtParseTranslationTable(""));
		i++;
		XtSetArg(arg[i], XtNgeometry, "32x32"); i++;
		XtSetArg(arg[i], XtNmappedWhenManaged, False); i++;
		gp->iconShell = XtCreateApplicationShell(
			"shell", vendorShellWidgetClass, arg, i
		);
		XtRealizeWidget(gp->iconShell);
		SetBaseWindowGizmoPixmap((Gizmo)gp, g->iconPixmap);
	}

	i = 0;
	XtSetArg(arg[i], XtNtitle, g->title ? GGT(g->title) : " ");i++;
	XtSetArg(arg[i], XtNiconName, g->iconName ? GGT(g->iconName): " ");i++;
	XtSetArg(arg[i], XmNiconWindow, XtWindow(gp->iconShell)); i++;
	i = AppendArgsToList(arg, i, args, num);
	gp->shell = XtCreateApplicationShell(
		g->name, topLevelShellWidgetClass, arg, i
	);

	/* Add help if there is any */
	if (g->help != NULL) {
		if (gp->iconShell != NULL) {
			GizmoRegisterHelp(gp->iconShell, g->help);
		}
	}

	XtSetArg(arg[0], XmNshadowThickness, 0);
	gp->work = XtCreateManagedWidget(
		gp->name, xmFormWidgetClass, gp->shell, arg, 1
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->work, g->help);
	}

	/* Create the menubar within the form area. */
	if (g->menu != NULL) {
		gp->menu = CreateGizmo(
			   gp->work, MenuBarGizmoClass, g->menu, NULL, 0
		);
		i=0;
		XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
		XtSetValues(gp->menu->menu, arg, i);
	}
	/* Create the footer if one existes */
	if (g->footer != NULL) {
		gp->footer = CreateGizmo(
			gp->work, MsgGizmoClass, g->footer, NULL, 0
		);
		/* Attach the footer to the base of the window */
		i = 0;
		XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
		XtSetValues(gp->footer->msgForm, arg, i);
	}

	/* Create all of the gizmos supplied in the gizmo array.
	 * Attach the first gizmo to the top of the form (or the
	 * bottom of the menu, if one exists).  Attach the last gizmo
	 * to the bottom of the form (or the top of the footer, if one
	 * exists.
	 */
	gp->numGizmos = g->numGizmos;
	gp->gizmos = CreateGizmoArray(gp->work, g->gizmos, g->numGizmos);
	if (gp->numGizmos > 0) {
		int last = gp->numGizmos - 1;
		Widget firstW = GetAttachmentWidget(
			gp->gizmos[0].gizmoClass, gp->gizmos[0].gizmo
		);
		Widget lastW = GetAttachmentWidget(
			gp->gizmos[last].gizmoClass, gp->gizmos[last].gizmo
		);
		
		i=0;
		if (gp->menu) {
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_WIDGET);
			i++;
			XtSetArg(arg[i], XmNtopWidget, gp->menu->menu); i++;
		}
		else {
			XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
		}
		XtSetValues(firstW, arg, i);

		i=0;
		if (gp->footer) {
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_WIDGET);
			i++;
			XtSetArg(arg[i], XmNbottomWidget, gp->footer->msgForm);
			i++;
		}
		else {
			XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM);
			i++;
		}
		XtSetValues(lastW, arg, i);
	}
	return (Gizmo)gp;
}

/*
 * GetBaseWindowMenuBar
 */
Gizmo
GetBaseWindowMenuBar(Gizmo g)
{
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;

	return (MenuGizmoP *)(gp->menu);
}

/*
 * GetBaseWindowShell
 */
Widget
GetBaseWindowShell(Gizmo g)
{
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;

	return gp->shell;
}

static void
DumpBaseWindow(BaseWindowGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "base(%s) = 0x%x 0x%x\n", g->name, g, g->work);
	DumpGizmo(MenuBarGizmoClass, g->menu, indent+1);
	DumpGizmoArray(g->gizmos, g->numGizmos, indent+1);
}

static Gizmo
SetBaseWindowValue(BaseWindowGizmoP *g, char *name, char *value)
{
	return SetGizmoArrayByName(
		g->gizmos, g->numGizmos, name, value
	);
}

static void
RealizeBaseWindow(BaseWindowGizmoP *g)
{
	if (!XtIsRealized(g->shell)) {
		XtRealizeWidget(g->shell);
	}
	XtMapWidget(g->shell);
}

/*
 * QueryBaseWindow
 */
static XtPointer
QueryBaseWindow(BaseWindowGizmoP *g, int option, char *name)
{
	XtPointer	value;

	if (!name || !g->name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->work);
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
		value = QueryGizmo(
			MenuBarGizmoClass, g->menu, option, name
		);
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

/*
 * ManipulateBaseWindow
 */
static void
ManipulateBaseWindow(BaseWindowGizmoP *g, ManipulateOption option)
{
	GizmoArray	gp = g->gizmos;
	int		i;

	for (i = 0; i < g->numGizmos; i++) {
		ManipulateGizmo(gp[i].gizmoClass, gp[i].gizmo, option);
	}
}

void
SetBaseWindowTitle(Gizmo g, char *title)
{
	Arg			arg[10];
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;

	XtSetArg(arg[0], XtNtitle, title ? GGT(title) : " ");
	XtSetValues(gp->shell, arg, 1);
}
void
SetBaseWindowLeftMsg(Gizmo g, char *msg)
{
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;

	SetMsgGizmoTextLeft((Gizmo)(gp->footer), msg);
}

void
SetBaseWindowRightMsg(Gizmo g, char *msg)
{
	BaseWindowGizmoP *	gp = (BaseWindowGizmoP *)g;

	SetMsgGizmoTextRight((Gizmo)(gp->footer), msg);
}

Widget
GetBaseWindowIconShell(Gizmo g)
{
	return ((BaseWindowGizmoP *)g)->iconShell;
}

void
SetBaseWindowIconName(Gizmo g, char *name)
{
    Arg arg[2];

    XtSetArg(arg[0], XtNiconName, name ? GGT(name) : " ");
    XtSetValues(((BaseWindowGizmoP *)g)->shell, arg, 1);
}	
