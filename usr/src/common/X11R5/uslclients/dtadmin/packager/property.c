#ifndef NOIDENT
#pragma ident	"@(#)property.c	15.1"
#endif

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <X11/StringDefs.h>
#include "packager.h"
#include <Gizmo/BaseWGizmo.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>

void			cancelINFO();

static MenuItems	info_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelINFO,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpProps },
	{ NULL }
};
static MenuGizmo	info_menu = {0, "properties", NULL, info_menu_item };
static PopupGizmo	info_popup = {0,"popup",string_newinfoTitle,(Gizmo)&info_menu };

static void
cancelINFO(Widget wid, PackageRecord *pr, XtPointer call_data)
{
	BringDownPopup(pr->info);
}

/*
 * Create captioned text fields in the property sheet.
 */

static	Widget
InfoCaption(Widget parent, char *label)
{
	Widget	w_cap;

	XtSetArg(arg[0], XtNposition,	(XtArgVal)OL_LEFT);
	XtSetArg(arg[1], XtNalignment,	(XtArgVal)OL_CENTER);
	XtSetArg(arg[2], XtNspace,	(XtArgVal)6);
	XtSetArg(arg[3], XtNlabel,	(XtArgVal)GGT(label));
	w_cap = XtCreateManagedWidget("caption", captionWidgetClass,
			parent, arg, 4);

	XtSetArg(arg[0], XtNwidth,	(XtArgVal)24*x3mm);
	return XtCreateManagedWidget("text", staticTextWidgetClass,
			w_cap, arg, 1);
}

/*
 * Create the property sheet popup.
 */

void
CreateInfoSheet(PackageRecord *pr)
{
	Widget	w_up;

	info_menu_item[0].client_data = (XtPointer)pr;

	pr->infoPopup = CopyGizmo (PopupGizmoClass, &info_popup);
	CreateGizmo(pr->base->shell, PopupGizmoClass, pr->infoPopup, NULL, 0);

	pr->info = GetPopupGizmoShell(pr->infoPopup);

	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(pr->info, arg, 1);

	pr->name = InfoCaption(w_up, info_name);
	pr->description = InfoCaption(w_up, info_desc);
	pr->category  = InfoCaption(w_up, info_cat);
	pr->vendor = InfoCaption(w_up, info_vendor);
	pr->version = InfoCaption(w_up, info_version);
	pr->architect = InfoCaption(w_up, info_arch);
	pr->date = InfoCaption(w_up, info_date);
	pr->size = InfoCaption(w_up, info_size);
}

void
GetInfo(PackageRecord *pr, PkgPtr p)
{
	SetPopupMessage(pr->infoPopup, NULL);

	XtSetArg(arg[0], XtNstring, p->pkg_name ? p->pkg_name : "");
	XtSetValues(pr->name, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_desc ? p->pkg_desc : "");
	XtSetValues(pr->description, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_cat ? p->pkg_cat : "");
	XtSetValues(pr->category,  arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_vers ? p->pkg_vers : "");
	XtSetValues(pr->version, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_arch ? p->pkg_arch : "");
	XtSetValues(pr->architect, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_vend ? p->pkg_vend : "");
	XtSetValues(pr->vendor, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_date ? p->pkg_date : "");
	XtSetValues(pr->date, arg, 1);
	XtSetArg(arg[0], XtNstring, p->pkg_size ? p->pkg_size : "");
	XtSetValues(pr->size, arg, 1);
}
