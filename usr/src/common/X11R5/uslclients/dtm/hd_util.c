#pragma ident	"@(#)dtm:hd_util.c	1.43"

/******************************file*header********************************

    Description:
     This file contains the source code to add and remove an application
	to and from the Help Desk.
*/
                              /* #includes go here     */


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private  Procedures
          2. Public Procedures
*/

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * Reads a one-line description from a help file to be displayed when
 * an icon in the Help Desk is SELECTed on.
 */
void
DmGetHDAppInfo(char *help_file, char **label, char **descrp)
{
	DmMapfilePtr mp;
	char *save_curptr;
	char *bp;
	char *ep;
	char *val;

	/* map file for reading */
	if (!(mp = Dm__mapfile(help_file, PROT_READ, MAP_SHARED)))
		return;

	save_curptr = MF_CURPTR(mp);

	if (descrp) {
		/* scan the file for description */
		val = NULL;
		if ((bp = Dm__strstr(mp, "\n^?")) != NULL) {
			MF_NEXTC(mp); MF_NEXTC(mp); MF_NEXTC(mp);
			bp = MF_GETPTR(mp);
			if ((ep = Dm__findchar(mp, '\n')) != NULL) {
				val = (char *)strndup(bp, ep - bp);
			}
		}
		*descrp = val;
	}

	if (label) {
		val = NULL;
		MF_CURPTR(mp) = save_curptr;
		/* scan the file for icon label */
		if ((bp = Dm__strstr(mp, "\n^:")) != NULL) {
			MF_NEXTC(mp); MF_NEXTC(mp); MF_NEXTC(mp);
			bp = MF_GETPTR(mp);
			if ((ep = Dm__findchar(mp, '\n')) != NULL) {
				val = (char *)strndup(bp, ep - bp);
			}
		}
		*label = val;
	}
	Dm__unmapfile(mp);

} /* end of DmGetHDAppInfo */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Adds an application (icon) to the Help Desk when a DT_ADD_TO_HELPDESK
 * client request is received.  Saves icon label, description, help
 * directory, icon file and help file as instance properties.
 * "icon_label" is icon label from DT_ADD_TO_HELPDESK request.
 */
int
DmAddAppToHD(char *app_name, char *icon_label, char *icon_file, char *help_file,
	char *help_dir, char *full_path)
{
	Cardinal n;
	char buf[32];
	char *prop;
	char *descrp;
	char *icon_label_hf; /* icon label from help file */
	DmObjectPtr op;

	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		DmInitHelpDesk(NULL, False, False);

	DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
	/* check if app already exists in Help Desk */
	for (op = DESKTOP_HELP_DESK(Desktop)->views[0].cp->op; op;
	  op = op->next)
	{
		prop = DmGetObjProperty(op, HELP_FILE, NULL);
		if (strcmp(op->name, app_name) == 0 &&
		  strcmp(prop, help_file) == 0)
		{
#ifdef DEBUG
			/* application already exists */
			DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop),
				True, TXT_HDESK_APP_EXISTS, icon_label ?
				icon_label : app_name);
#endif
			/* Fail silently because the Application Installer
			 * doesn't care about failure due to duplicates;
			 * it was probably added by the application.
			 */
			return(-1);	
		}
	}
	op = NULL;
	if (!(op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec)))) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(-1);
	}
	op->name      = STRDUP(app_name);
	op->container = DESKTOP_HELP_DESK(Desktop)->views[0].cp;
	op->ftype     = DM_FTYPE_DATA;
	op->fcp       = hddp->fcp;

	/* Get locale-specific icon label and description from help file
	 * and save icon label as default icon label.
	 */ 
	DmGetHDAppInfo(full_path, &icon_label_hf, &descrp);
	DmSetObjProperty(op, DFLT_ICONLABEL, icon_label, NULL);
	DmSetObjProperty(op, ICON_LABEL, icon_label_hf, NULL);
	DmSetObjProperty(op, DESCRP, descrp, NULL);
	DmSetObjProperty(op, ICONFILE, icon_file, NULL);
	DmSetObjProperty(op, HELP_FILE, help_file, NULL);
	DmSetObjProperty(op, HELP_DIR, help_dir, NULL);
	 
	if ((n = DmAddObjectToIconContainer(
		DESKTOP_HELP_DESK(Desktop)->views[0].box,
		&(DESKTOP_HELP_DESK(Desktop)->views[0].itp),
		&(DESKTOP_HELP_DESK(Desktop)->views[0].nitems),
	        DESKTOP_HELP_DESK(Desktop)->views[0].cp, op, 1, 1, 
	        DM_B_SPECIAL_NAME | DM_B_ALLOW_DUPLICATES)) != ExmNO_ITEM)
	{
		/* set item label */
		MAKE_WRAPPED_LABEL(
			(DESKTOP_HELP_DESK(Desktop)->views[0].itp + n)->label,
			FPART(DESKTOP_HELP_DESK(Desktop)->views[0].box).font,
			icon_label ? icon_label :
			icon_label_hf ? icon_label : app_name);
		 
		DmSizeIcon(DESKTOP_HELP_DESK(Desktop)->views[0].box,
			   DESKTOP_HELP_DESK(Desktop)->views[0].itp + n);

		DmSortItems(DESKTOP_HELP_DESK(Desktop), DM_BY_NAME, 0, 0, 0);

		DmWriteDtInfo(DESKTOP_HELP_DESK(Desktop)->views[0].cp,
			DESKTOP_HELP_DESK(Desktop)->views[0].cp->path, 0);
		return(0);

	} else {
		DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop),
			True, TXT_ADD_TO_HDESK_FAILED, icon_label ?
			icon_label : app_name);
		Dm__FreeObject(op);
		return(-1);
	}

} /* end of DmAddAppToHD */

/****************************procedure*header*****************************
 * Removes an application (icon) from the Help Desk when a DT_DEL_FROM_HELPDESK
 * request.
 */
int
DmDelAppFromHD(char *app_name, char *help_file)
{
	int i;
	char *prop;
	DmItemPtr itp;

	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		DmInitHelpDesk(NULL, False, False);

	DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->views[0].itp;
	     i < DESKTOP_HELP_DESK(Desktop)->views[0].nitems; i++, itp++) {

		if (ITEM_MANAGED(itp)) {
			prop = DmGetObjProperty(ITEM_OBJ(itp), HELP_FILE,NULL);
			if (strcmp(ITEM_OBJ(itp)->name, app_name) == 0 &&
			     strcmp(prop, help_file) == 0)
			{
				itp->managed = (XtArgVal)False;
				DmDelObjectFromContainer(
				  DESKTOP_HELP_DESK(Desktop)->views[0].cp,
					ITEM_OBJ(itp));

				/* sort items by name */
				DmSortItems(DESKTOP_HELP_DESK(Desktop),
					DM_BY_NAME, 0, 0, 0);

				DmWriteDtInfo(
				  DESKTOP_HELP_DESK(Desktop)->views[0].cp,
				  DESKTOP_HELP_DESK(Desktop)->views[0].cp->path, 0);
				return(0);
			}
		}
	}
	/* Fail silently because the Application Installer doesn't care
	 * if the entry is not found; it was probably removed by the
	 * application.
	 */
#ifdef DEBUG
	DmVaDisplayStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop), True,
		TXT_HD_DELETE_FAILED, app_name);
#endif
	return(-1);

} /* end of DmDelAppFromHD */
