#pragma ident	"@(#)dtm:hReqProc.c	1.61"

/******************************file*header********************************

    Description:
     This file contains the source code to handle the following requests:
	- DT_DISPLAY_HELP
	- DT_OL_DISPLAY_HELP
	- DT_ADD_TO_HELPDESK
	- DT_DEL_FROM_HELPDESK.
*/
                              /* #includes go here     */

#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Dt/DesktopI.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static int DisplayHelp(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int AddToHelpDesk(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int DelFromHelpDesk(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

static int DisplayOLHelp(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *reply);

/*
 * This array must match with information in DtHMMsg.h.
 */
static int (*(help_procs[]))() = {
	DisplayHelp,
	AddToHelpDesk,
	DelFromHelpDesk,
	DisplayOLHelp,
};

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
	Sets up functions for handling client requests for on-line help.
 */

void
DmInitHMReqProcs(Widget w)
{
	static DmPropEventData help_prop_ev_data = {
				(Atom)0,
				Dt__help_msgtypes,
				help_procs,
				XtNumber(help_procs)
	};

	/*
	 * Since _HELP_QUEUE is not constant,
	 * it cannot be initialized at compile time.
	 */
	help_prop_ev_data.prop_name = _HELP_QUEUE(XtDisplay(w));

	DmRegisterReqProcs(w, &help_prop_ev_data);
}


/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
	Handles a DT_DISPLAY_HELP client request to either display help
	or to open the Help Desk.
 */
static int
DisplayHelp(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
	DmHelpAppPtr hap;
	Boolean success = True;
	char *fullpath = NULL;

#define REQ request->display_help
	/* Simply call DmHelpDeskCB() if source_type is DT_OPEN_HELPDESK */ 
	if (REQ.source_type == DT_OPEN_HELPDESK) {
		DmHelpDeskCB(NULL, NULL, NULL);
		return(0);
	}
	/* app_name must be specified for all other source_types */
	if (!REQ.app_name || REQ.app_name[0] == '\0')
		return(0);

	if (hap = DmNewHelpAppID(scrn, request->header.client, REQ.app_name,
		REQ.app_title, request->header.nodename, REQ.help_dir,
		REQ.icon_file)) {

		if (REQ.source_type == DT_SECTION_HELP ||
			REQ.source_type == DT_TOC_HELP)

			if (!REQ.file_name)
				success = False;
			else {
				struct stat hstat;

				fullpath = DmGetFullPath(REQ.file_name,
					REQ.help_dir);
				if (fullpath == NULL ||
				  stat(fullpath, &hstat) != 0)
					success = False;
			}
		else if (REQ.source_type == DT_STRING_HELP && ! REQ.string)
				success = False;

		if (!success) {
			DmDisplayHelpString(hap, NULL, (char *)nohelp_str);
			goto bye;
		}
		switch(REQ.source_type) {
		case DT_STRING_HELP:
			DmDisplayHelpString(hap, REQ.title, REQ.string);
			break;

		case DT_SECTION_HELP:
			DmDisplayHelpSection(hap, NULL, fullpath,
				REQ.sect_tag);
			break;

		case DT_TOC_HELP:
			DmDisplayHelpTOC(NULL, &(hap->hlp_win), fullpath,
				hap->app_id);
			break;

		default:
			Dm__VaPrintMsg(TXT_UNKNOWN_HELP_TYPE, REQ.source_type);
			break;
		}
	} else
		Dm__VaPrintMsg(TXT_CANT_ALLOC_HAP);

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.app_title);
	XtFree(REQ.string);
	XtFree(REQ.file_name);
	XtFree(REQ.sect_tag);
	XtFree(REQ.icon_file);
	XtFree(fullpath);

	/*0 is return to indicate no reply is needed; 1 is returned otherwise.*/
	return(0);

#undef REQ

} /* end of DisplayHelp */

/****************************procedure*header*****************************
	Handles a DT_ADD_TO_HELPDESK client request.  Adds an application
	to the Help Desk.
 */
static int
AddToHelpDesk(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
#define REQ request->add_to_helpdesk
#define REP reply

	struct stat hstat;
	char *fullpath = NULL;

	/* REQ.app_name and REQ.help_file must both be specified. */
	if (!REQ.app_name || REQ.app_name[0] == '\0' ||
		!REQ.help_file || REQ.help_file[0] == '\0')
	{
		REP->header.status = DT_FAILED;
		goto bye;
	}

	/*
	 * Check whether REQ.help_file exists. It can be a full or
	 * relative path name. If it is the latter and a help directory
	 * is not specified, then it is expected to be in the standard
	 * search path.
	 */

	fullpath = DmGetFullPath(REQ.help_file, REQ.help_dir);
	if (fullpath == NULL || stat(fullpath, &hstat) != 0) {
		if (fullpath)
			free(fullpath);
		REP->header.status = DT_FAILED;
		goto bye;
	}

	if (DmAddAppToHD(REQ.app_name, REQ.icon_label, REQ.icon_file,
		REQ.help_file, REQ.help_dir, fullpath) == 0)

		REP->header.status = DT_OK;
	else
		REP->header.status = DT_FAILED;

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.icon_label);
	XtFree(REQ.icon_file);
	XtFree(REQ.help_file);
	XtFree(REQ.help_dir);
	XtFree(fullpath);

#undef REQ
#undef REP
	return(1);

} /* end of AddToHelpDesk */

/****************************procedure*header*****************************
	Handles a DT_DEL_FROM_HELPDESK client request.  Removes an application
	from the Help Desk.
 */
static int
DelFromHelpDesk(Screen *scrn, XEvent *xevent, DtRequest	*request,
	DtReply *reply)
{
#define REQ	request->del_from_helpdesk
#define REP	reply

	if (REQ.app_name && REQ.help_file &&
		DmDelAppFromHD(REQ.app_name, REQ.help_file) == 0)

		REP->header.status = DT_OK;
	else
		REP->header.status = DT_FAILED;

	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.help_file);
	return(1);

#undef REQ
#undef REP

} /* end of DelFromHelpDesk */

/****************************procedure*header*****************************
 * Called when a DT_OL_DISPLAY_HELP request is received from the
 * OPEN LOOK toolkit and OPEN LOOK window manager.  Actually, this
 * is a re-routed request for context-sensitive help. Displays default
 * help if help file is not found and source_type is OL_DISK_SOURCE or
 * OL_DESKTOP_SOURCE. app_name must be specified for all source_types.
 */
static int
DisplayOLHelp(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
#define REQ	request->ol_display_help
#define OL_DISK_SOURCE		15
#define OL_STRING_SOURCE	64
#define OL_DESKTOP_SOURCE	190

	char *fullpath = NULL;
	DmHelpAppPtr hap;
	Boolean success = True;

	if (! REQ.app_name)
		goto bye;

	if (hap = DmNewHelpAppID(scrn, request->header.client, REQ.app_name,
	    REQ.app_title, request->header.nodename, REQ.help_dir, NULL)) {

		if (REQ.source_type == OL_DISK_SOURCE ||
			REQ.source_type == OL_DESKTOP_SOURCE)

			if (!REQ.file_name)
				success = False;
			else {
				struct stat hstat;

				fullpath = DmGetFullPath(REQ.file_name,
						REQ.help_dir);
				if (fullpath == NULL ||
				  stat(fullpath, &hstat) != 0)
					success = False;
			}
		else if (REQ.source_type == OL_STRING_SOURCE && ! REQ.string)
			success = False;

		if (!success) {
			DmDisplayHelpString(hap, NULL, (char *)nohelp_str);
			goto bye;
		}

		switch(REQ.source_type) {
		case OL_STRING_SOURCE:
			DmDisplayHelpString(hap, REQ.title, REQ.string);
			XtFree(REQ.string);
			break;

		case OL_DISK_SOURCE:
			DmDisplayHelpSection(hap, NULL, fullpath, NULL);
			XtFree(REQ.file_name);
			break;

		case OL_DESKTOP_SOURCE:
			DmDisplayHelpSection(hap, NULL, fullpath,
				REQ.sect_tag);
			XtFree(REQ.file_name);
			XtFree(REQ.sect_tag);
			break;

		default:
			Dm__VaPrintMsg(TXT_UNKNOWN_HELP_TYPE, REQ.source_type);
			break;
		}
	} else
		Dm__VaPrintMsg(TXT_CANT_ALLOC_HAP);

bye:
	/* free data in request structure */
	XtFree(REQ.app_name);
	XtFree(REQ.app_title);
	XtFree(REQ.title);
	XtFree(fullpath);
	return(0);

#undef REQ
#undef OL_DISK_SOURCE
#undef OL_STRING_SOURCE
#undef OL_DESKTOP_SOURCE
} /* end of DisplayOLHelp  */
