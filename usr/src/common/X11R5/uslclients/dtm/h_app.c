#pragma ident	"@(#)dtm:h_app.c	1.58"

/******************************file*header********************************

    Description:
     This file contains the source code for adding and removing an
	application to the list of applications which have registered
	for help.
*/
                              /* #includes go here     */

#include <errno.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/

/**************************private*procedures***************************

	Private Procedures.
*/
static void GetAppName(char *old, char **new);
static void GetAppTitle(char *old, char **new);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/*
 * This is used to generate an application id.
 * The value 0 is reserved.
 */
static int  ticket = 1;

/****************************procedure*header*****************************
 * This routine will allocate a new application structure and
 * returns the associated app_id.
 */
DmHelpAppPtr
DmNewHelpAppID(Screen *scrn, Window win, char *app_name, char *app_title,
	char *node, char *help_dir, char *icon_file)
{
	char buf[PATH_MAX];
	char *lang;
	DmHelpAppPtr hap;
	register DmHelpAppPtr thap;

 	/* check if app is already in list */
	for (thap = DESKTOP_HELP_INFO(Desktop); thap; thap = thap->next) {
		if (strcmp(thap->name, app_name) == 0 &&
		  (app_title && strcmp(thap->title, app_title) == 0))
		    return(thap);
	}
	if ((hap = (DmHelpAppPtr)CALLOC(1, sizeof(DmHelpAppRec))) == NULL)
		return(NULL);

	hap->name       = strdup(app_name);
	hap->title      = app_title ? strdup(app_title) : strdup(app_name);
	hap->hlp_win.sp = -1;

	if (help_dir)
		hap->help_dir = strdup(help_dir);

	/* takes care of wraparound */
	if (ticket < 0)
		ticket = 1;

	hap->app_id = ticket++;

	/* add it to the list */
	hap->next = DESKTOP_HELP_INFO(Desktop);
	DESKTOP_HELP_INFO(Desktop) = hap;
	return(hap);

} /* end of DmNewHelpAppID */

/****************************procedure*header*****************************
 * This routine will return the DmHelpAppPtr given the associated app_id.
 */
DmHelpAppPtr
DmGetHelpApp(int app_id)
{
	register DmHelpAppPtr hap;

	for (hap=DESKTOP_HELP_INFO(Desktop); hap; hap=hap->next) {
		if (app_id == hap->app_id) {
			return(hap);
		}
	}
	return(NULL);

} /* end of DmGetHelpAppID */

/****************************procedure*header*****************************
 * This routine will free all the resources associated with the app_id.
 */
void
DmFreeHelpAppID(int app_id)
{
	DmHelpAppPtr hap;

	if (hap = DmGetHelpApp(app_id)) {
		/* remove it from the list */
		if (hap->name)
			FREE(hap->name);

		if (hap->title)
			FREE(hap->title);

		if (hap->help_dir)
			FREE(hap->help_dir);

		DmRemoveAppFromList(hap);
		DmCloseHelpWindow(&(hap->hlp_win));
		FREE((void *)hap);
	}
} /* end of DmFreeHelpAppID */

/****************************procedure*header*****************************
 * Removes an application from the list of applications which are
 * registered for help.
 */
void
DmRemoveAppFromList(DmHelpAppPtr hap)
{
	register DmHelpAppPtr thap = DESKTOP_HELP_INFO(Desktop);

	if (thap == hap) {
		DESKTOP_HELP_INFO(Desktop) = hap->next;
	} else {
		for (; thap->next; thap = thap->next) {
			if (thap->next == hap) {
				thap->next = hap->next;
				return;
			}
		}
	}
} /* end of DmRemoveAppFromList */

