#pragma ident	"@(#)dtm:hd_cbs.c	1.54"

/******************************file*header********************************

    Description:
     This file contains the source code to display a description of
	and help on an application in the Help Desk.
*/
                              /* #includes go here     */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "HyperText.h"

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/***************************private*procedures****************************

    Private Procedures
*/
static void GetAppInfo(DmObjectPtr op, char **help_file, char **help_dir,
	char **icon_file, char **icon_label);
static void GetHelp(DmObjectPtr op);

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Displays a one-line description of an application in the footer when
 * an icon is SELECTed.
 */
void
DmHDSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *descrp;
	Boolean set;
	ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;

	XtSetArg(Dm__arg[0], XmNset, &set);
	ExmFlatGetValues(w, d->item_data.item_index, Dm__arg, 1);
 
#ifdef NOT_USE
	if (set) {
		return;
	}
#endif

	descrp = DmGetObjProperty(OBJECT_CD(d->item_data), DESCRP, NULL);
	if (descrp)
		SetBaseWindowLeftMsg(
			(DmWinPtr)DESKTOP_HELP_DESK(Desktop)->gizmo_shell,
			descrp);
	else
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
			 TXT_NO_DESCRIP);

}				/* end of DmHDSelectProc */

/****************************procedure*header*****************************
 * Displays help on an application in a help window when its icon is
 * double-clicked on.
 */
void
DmHDDblSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
{
	ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;

	GetHelp(OBJECT_CD(d->item_data));

} /* end of DmHDDblSelectProc */

/****************************procedure*header*****************************
 * Callback for Open button in File menu to display help a selected
 * application.
 */
void
DmHDOpenCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int  i;
	char *help_file;
	char *help_dir;
	char *icon_file;
	char *icon_label;
	DmItemPtr itp;
	Boolean found = False;

	DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->views[0].itp;
		i < DESKTOP_HELP_DESK(Desktop)->views[0].nitems; i++, itp++)
	{
		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp)) {
			found = True;
			help_file = help_dir = icon_file = icon_label = NULL;
			GetHelp(ITEM_OBJ(itp));
			break;
		}
	}
	if (!found)
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
			TXT_HD_NONE_SELECTED);

} /* end of DmHDOpenCB */

/****************************procedure*header*****************************
 * Callback for Open in icon menu to display help for selected application.
 */
void
DmHDIMOpenCB( Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmItemPtr    itp = (DmItemPtr)(cbs->clientData);
	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);
	GetHelp(ITEM_OBJ(itp));

} /* end of DmHDIMOpenCB */

/****************************procedure*header*****************************
 * Callback for Align button in View menu to sort icons by name.
 */
void
DmHDAlignCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmSortItems(DESKTOP_HELP_DESK(Desktop), DM_BY_NAME, 0, 0, 0);

} /* end of DmHDAlignCB */

/****************************procedure*header*****************************
 * Callback for Help Desk's Help menu.
 */
void
DmHDHelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), False, NULL);

	/* reset help_dir to NULL if it was previously used for another app */
	if (hddp->hap->help_dir) {
		free(hddp->hap->help_dir);
		hddp->hap->help_dir = NULL;
	}
	switch(cbs->index) {
	case 0: /* Help Desk */
		DmDisplayHelpSection(hddp->hap, NULL, HELPDESK_HELPFILE, NULL);
		break;
	case 1: /* Table of Contents */
		DmDisplayHelpTOC(w, &(hddp->hap->hlp_win), HELPDESK_HELPFILE,
			hddp->hap->app_id);
		break;
	}

} /* end of DmHDHelpCB */
		
/****************************procedure*header*****************************
 * GetAppInfo - Gets help info on an application.  Returns help file name,
 * help directory, icon label and icon pixmap.
 */
static void
GetAppInfo(DmObjectPtr op, char **help_file, char **help_dir, char **icon_file,
	char **icon_label)
{
	*help_file  = DmGetObjProperty(op, HELP_FILE, NULL);
	*help_dir   = DmGetObjProperty(op, HELP_DIR, NULL);
	*icon_file  = DmGetObjProperty(op, ICONFILE, NULL);
	*icon_label = DmGetObjProperty(op, ICON_LABEL, NULL);

	if (hddp->hap->help_dir) {
		free(hddp->hap->help_dir);
		hddp->hap->help_dir = NULL;
	}

	if (*help_dir)
		hddp->hap->help_dir = strdup(*help_dir);

	if (*icon_file == NULL)
		*icon_file = DFLT_APP_ICON;

} /* end of GetAppInfo */

/****************************procedure*header*****************************
 * GetHelp - Gets help info and displays help window for an item in
 * the Help Desk.
 */
static void
GetHelp(DmObjectPtr op)
{
	char *help_file;
	char *help_dir;
	char *icon_file;
	char *icon_label;
	DmHelpAppPtr hap;

	/* display application's help */
	GetAppInfo(op, &help_file, &help_dir, &icon_file, &icon_label);

	/* Get hap of selected application and use it to get a help window */
	if (hap = DmNewHelpAppID(DESKTOP_SCREEN(Desktop), NULL,
			op->name, icon_label, NULL, help_dir, icon_file))
	{
		DmDisplayHelpSection(hap, icon_label ? icon_label : op->name,
			help_file, NULL);
	} else {
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop),
			True, TXT_HD_CANT_SHOW_HELP, icon_label);
	}
} /* end of GetHelp */
