#pragma ident	"@(#)dtm:wb_util.c	1.65.1.23"

/******************************file*header********************************

    Description:
     This file contains the source code for switching the wastebasket's
	process icon pixmap, suspending, resuming and restarting the timer,
	creating the process icon, assigning version number to a deleted file,
	and labeling icons in the wastebasket.
*/
                              /* #includes go here     */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <X11/Xatom.h>

#include <Dt/Desktop.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>             /* for _XmFindTopMostShell */
#include <Xm/Protocols.h>       /* for XmAddWMProtocolCallback */
#include <Xm/Form.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/BaseWGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Label files in wastebasket using their basename and assigned
 * version numbers. 
 */
void
DmLabelWBFiles()
{
	struct stat wbstat;
	DmItemPtr itp;
	DmObjectPtr op;
	int i;
	int ver_num;
	char *real_path;
	char *bname;

	/* If Wastebasket's .dtinfo file contains entries for files which don't
	 * exist, unmanage the item and remove the object from Wastebasket.
	 */
	for (op = DESKTOP_WB_WIN(Desktop)->views[0].cp->op; op; op = op->next) {
		sprintf(Dm__buffer, "%s/%s", DESKTOP_WB_WIN(Desktop)->views[0].cp->path,
			op->name);
	  if (stat(Dm__buffer, &wbstat) != 0)
		DmDelObjectFromContainer((DmContainerPtr)DESKTOP_WB_WIN(Desktop)->views[0].cp,
			op);
	  else
		DmSetObjProperty(op, VERSION, NULL, NULL);
	}

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	     i < DESKTOP_WB_WIN(Desktop)->views[0].nitems; i++, itp++) {

		if (itp->managed) {
			real_path = DmGetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL);
			bname = basename(real_path);
			ver_num = DmWBGetVersion(itp, bname);
			sprintf(Dm__buffer, "%d", ver_num);
			DmSetObjProperty(ITEM_OBJ(itp), VERSION, Dm__buffer, NULL);
			sprintf(Dm__buffer, "%s:%d", bname, ver_num);
			MAKE_WRAPPED_LABEL(itp->label, 
			    FPART(DESKTOP_WB_WIN(Desktop)->views[0].box).font,
			    Dm__buffer);
			DmSizeIcon(DESKTOP_WB_WIN(Desktop)->views[0].box, itp); 
		}
	}
	DmTouchIconBox((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL, 0);
	DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);
/*
	DmSwitchWBIcon();
*/

} /* end of DmLabelFiles */

/****************************procedure*header*****************************
 * Returns the next version number to be used for a deleted file.
 * Returns 1 if there's no other file of the same name; otherwise,
 * returns the next higher integer.
 */
int
DmWBGetVersion(itp, fname)
DmItemPtr itp;
char *fname;	/* basename of file */
{
#define MAX(A,B)	(((A) > (B)) ? (A) : (B))
	DmObjectPtr op;
	char *real_path;
	char *bname;
	char *version;
	int count;
	int vnum = 0;

	for (op = DESKTOP_WB_WIN(Desktop)->views[0].cp->op; op; op = op->next) {
		real_path = DmGetObjProperty(op, REAL_PATH, NULL);
		bname = basename(real_path);
		if (strcmp(bname, fname) == 0 &&
		    strcmp(ITEM_OBJ(itp)->name, op->name) != 0) {
			/* VERSION may not be set */
			if (version = DmGetObjProperty(op, VERSION, NULL)) {
				count = atoi(version) + 1;
				vnum = MAX(vnum, count);
			}
		}
	}
	if (!vnum)
		vnum++;
	return(vnum);
#undef MAX
} /* end of DmGetVersion */

/****************************procedure*header*****************************
    DmWBCleanUpMethod- Returns Wastebasket's current clean up method.

*/
/* ARGSUSED */
int
DmWBCleanUpMethod(DmDesktopPtr desktop)
{
	return(wbinfo.cleanUpMethod);
}

/****************************procedure*header*****************************
 * Returns folder window ptr to wastebasket if win is window id of
 * Wastebasket window; otherwise, returns NULL. 
 */
DmFolderWinPtr
DmIsWB(win)
Window	win;
{
    if (win == XtWindow(DESKTOP_WB_WIN(Desktop)->views[0].box) ||
	      (win == DESKTOP_WB_ICON(Desktop)))
	return ( DESKTOP_WB_WIN(Desktop) );

    return(NULL);

} /* end of DmIsWB */

/****************************procedure*header*****************************
 * Switches icon pixmap for wastebasket process icon when the wastebasket
 * becomes empty or non-empty.
 */
void
DmSwitchWBIcon()
{
	Widget w = QueryGizmo(BaseWindowGizmoClass,
		DESKTOP_WB_WIN(Desktop)->gizmo_shell, GetGizmoWidget,
		"ActionMenu:2");

	/* update state of Action:Empty button */
	if (WB_IS_EMPTY(Desktop))
		XtSetArg(Dm__arg[0], XmNsensitive, False);
	else
		XtSetArg(Dm__arg[0], XmNsensitive, True);
	XtSetValues(w, Dm__arg, 1);

	SetBaseWindowGizmoPixmap(DESKTOP_WB_WIN(Desktop)->gizmo_shell,
		(WB_IS_EMPTY(Desktop)) ? "emptywb" : "filledwb");

} /* end of DmSwitchWBIcon */

/****************************procedure*header*****************************
 * If wastebasket is empty, don't restart timer; otherwise, restart timer
 * with an adjusted timer interval determined from the lowest time stamp of
 * files in the wastebasket.
 */
void
DmWBRestartTimer()
{
	if (WB_IS_EMPTY(Desktop)) {

		wbinfo.tm_interval =
			(unsigned long)(wbinfo.interval * wbinfo.time_unit);
		wbinfo.tm_remain = wbinfo.tm_interval;
		wbinfo.suspend = True;
		wbinfo.restart = True;

		if (wbinfo.timer_id) {
			XtRemoveTimeOut(wbinfo.timer_id);
			wbinfo.timer_id = NULL;
		}

		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), TXT_WB_SUSPEND);

		/* Set button label to Resume Timer */
		DmWBToggleTimerBtn(False);
		return;

	} else {
		int i;
		time_t ts;
		time_t lowest_ts;
		time_t current_time;
		unsigned long intv1;
		unsigned long intv2;
		DmItemPtr itp;
		Boolean reset = False;

		lowest_ts = LONG_MAX;
		time(&current_time);

		for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
		     i < DESKTOP_WB_WIN(Desktop)->views[0].nitems; i++, itp++) {

			if (ITEM_MANAGED(itp)) {
				ts = strtoul(DmGetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
						NULL), NULL, 0);

				/* If the time stamp of file is greater than the current
				 * time (this *is* possible), then reset its time stamp
				 * to the current_time to avoid an infinite loop.
				 */
				if (ts > current_time) {
					ts = current_time;
					sprintf(Dm__buffer, "%ld", ts);
					DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
						Dm__buffer, NULL);
					reset = True;
				}

				if (ts < lowest_ts) {
					lowest_ts = ts;
				}
			}
		}
		/* update Wastebasket's .dtinfo file if any time stamp was reset */
		if (reset)
			DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

		/* compute timer interval and time elapsed in milliseconds */
		intv1 = (unsigned long)(wbinfo.interval * wbinfo.time_unit);
		intv2 = (unsigned long)((current_time - lowest_ts) * 1000);

		if (intv2 > intv1)
			wbinfo.tm_interval = (unsigned long)0;
		else
			wbinfo.tm_interval = (unsigned long)(intv1 - intv2);

		if (wbinfo.suspend == False || (wbinfo.suspend && wbinfo.restart)) {
			if (wbinfo.timer_id != NULL)
				XtRemoveTimeOut(wbinfo.timer_id);
			 
			wbinfo.timer_id = XtAddTimeOut(wbinfo.tm_interval,
				(XtTimerCallbackProc)DmWBTimerProc, NULL);
			time(&(wbinfo.tm_start));

			DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				TXT_WB_RESUME);

			/* Set button label to Suspend Timer */
			DmWBToggleTimerBtn(True);
			wbinfo.restart = False;

		} else {
			wbinfo.timer_id = NULL;
			wbinfo.tm_remain = wbinfo.tm_interval;

			DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				TXT_WB_SUSPEND);

			/* Set button label to Resume Timer */
			DmWBToggleTimerBtn(True);
		}
	}
} /* end of DmRestartTimer */

/****************************procedure*header*****************************
 * DmGetWBPixmaps
 */
void
DmGetWBPixmaps()
{
	DmGlyphPtr gp1;
	DmGlyphPtr gp2;
		
	gp1 = DmGetPixmap(DESKTOP_SCREEN(Desktop), "emptywb");

	if (!gp1) {
		gp1 = DmGetPixmap(DESKTOP_SCREEN(Desktop), NULL);
	}
	wbinfo.egp = gp1;

	gp2 = DmGetPixmap(DESKTOP_SCREEN(Desktop), "filledwb");

	if (!gp2) {
		gp2 = DmGetPixmap(DESKTOP_SCREEN(Desktop), NULL);
	}
	wbinfo.fgp = gp2;

} /* end of DmGetWBPixmaps */

/****************************procedure*header*****************************
 * Creates a shell for wastebasket icon to handle drops.
 */
void
DmCreateWBIconShell(Widget toplevel, Boolean iconic)
{
	Widget form;
	Arg wb_args[1];
	static Atom targets[5];

	targets[0] = OL_USL_NUM_ITEMS(DESKTOP_DISPLAY(Desktop));
	targets[1] = OL_USL_ITEM(DESKTOP_DISPLAY(Desktop));
	targets[2] = OL_XA_FILE_NAME(DESKTOP_DISPLAY(Desktop));
	targets[3] = XA_STRING;
	targets[4] = OL_XA_HOST_NAME(DESKTOP_DISPLAY(Desktop));

	/* create process icon */
	XtSetArg(wb_args[0], XtNinitialState,
			(iconic ? IconicState : NormalState));

#ifdef NOT_USE
	XtSetArg(Dm__arg[0], XtNiconPixmap, (!WB_IS_EMPTY(Desktop)) ?
		wbinfo.fgp->pix : wbinfo.egp->pix);
	XtSetArg(Dm__arg[1], XtNiconMask, (!WB_IS_EMPTY(Desktop)) ?
		wbinfo.fgp->mask : wbinfo.egp->mask);
#endif

	wbinfo.iconShell = DtCreateProcessIcon(toplevel,
					wb_args, 1,
					NULL, 0,
					NULL, 0);
#ifdef NOT_USE
					Dm__arg, 2);

	DESKTOP_WB_ICON(Desktop) = XtWindow(wbinfo.iconShell);

	XtSetArg(Dm__arg[0], XmNbackgroundPixmap, (!WB_IS_EMPTY(Desktop)) ?
		wbinfo.fgp->pix : wbinfo.egp->pix);
	wbinfo.iconForm = XtCreateManagedWidget ("form", xmFormWidgetClass,
		wbinfo.iconShell, Dm__arg, 1);

	DESKTOP_WB_ICON(Desktop) = XtWindow(wbinfo.iconForm);

	XtSetArg(Dm__arg[0], XmNimportTargets,  targets);
	XtSetArg(Dm__arg[1], XmNimportTargets,  XtNumber(targets));
	XtSetArg(Dm__arg[2], XmNdropSiteOperations,
		XmDROP_MOVE|XmDROP_COPY|XmDROP_LINK);
	XtSetArg(Dm__arg[3], XmNdropProc, DmDropProc);
	XmDropSiteRegister(wbinfo.iconForm, Dm__arg, 4);
#endif

} /* end of DmCreateWBIconShell */

/****************************procedure*header*****************************
 * Toggles button label of first item in Actions menu between Resume Timer
 * and Suspend Timer depending on current state of timer.
 */
void
DmWBToggleTimerBtn(Boolean sensitive)
{
	Widget w;
	static char rm;
	static char sm;
	static XmString resume;
	static XmString suspend = NULL;

	if (suspend == NULL) {
		/* These labels are cached to avoid multiple calls to
		 * Dm__gettxt().
		 */
		suspend = XmStringCreateLocalized(Dm__gettxt(TXT_SUSPEND_LBL));
		resume = XmStringCreateLocalized(Dm__gettxt(TXT_RESUME_LBL));
		sm = *Dm__gettxt(TXT_M_SUSPEND_LBL);
		rm = *Dm__gettxt(TXT_M_RESUME_LBL);
	}
	w = QueryGizmo(BaseWindowGizmoClass,
		DESKTOP_WB_WIN(Desktop)->gizmo_shell, GetGizmoWidget,
		"ActionMenu:0");

	if (wbinfo.suspend) {
		XtSetArg(Dm__arg[0], XmNlabelString, resume);
		XtSetArg(Dm__arg[1], XmNmnemonic, (XtArgVal)rm);
	} else {
		XtSetArg(Dm__arg[0], XmNlabelString, (XtArgVal)suspend);
		XtSetArg(Dm__arg[1], XmNmnemonic, (XtArgVal)sm);
	}
	XtSetArg(Dm__arg[2], XtNsensitive, (XtArgVal)sensitive);
	XtSetValues(w, Dm__arg, 3);

}	/* end of DmToggleTimerBtn */

/****************************procedure*header*****************************
 * Resumes the Wastebasket timer after it has been suspended.
 */
void
DmWBResumeTimer()
{
	/*
	 * wbinfo.tm_remain should have been initialised either in
	 * in DmInitWasteBasket (i.e. in DmWBRestartTimer() ),
	 * when the timer was set via the Wastebasket Properties
	 * window, or when the timer was previously suspended.
	 */
	wbinfo.tm_interval = wbinfo.tm_remain;
	wbinfo.timer_id = XtAddTimeOut(wbinfo.tm_interval,
				(XtTimerCallbackProc)DmWBTimerProc, NULL);
	time(&(wbinfo.tm_start));
	wbinfo.suspend = False;

	/* update status area */
	DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
		TXT_WB_RESUME);

	/* Set button label to Suspend Timer */
	DmWBToggleTimerBtn(True);

} /* end of DmWBResumeTimer */

/****************************procedure*header*****************************
 * Suspend the Wastebasket timer.
 */
void
DmWBSuspendTimer()
{
	time_t current_time;
	unsigned long elapsed;

	if (wbinfo.timer_id) {
		XtRemoveTimeOut(wbinfo.timer_id);
		wbinfo.timer_id = NULL;
	}

	time(&(current_time));
	elapsed =(unsigned long)((current_time - wbinfo.tm_start) * 1000);

	if (elapsed > wbinfo.tm_interval)
		wbinfo.tm_remain = (unsigned long)0;
	else
		wbinfo.tm_remain = (unsigned long)(wbinfo.tm_interval - elapsed);

	wbinfo.suspend = True;
	DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop),
		TXT_WB_SUSPEND);

	/* Set button label to Resume Timer */
	DmWBToggleTimerBtn(WB_IS_EMPTY(Desktop) ? False : True);

} /* end of DmWBSuspendTimer */

/****************************procedure*header*****************************
 * WBGetFileInfo -
 */
DmFileInfoPtr
WBGetFileInfo(path)
char *path;
{
	DmFileInfoPtr f_info = (DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));
	if (f_info != NULL) {
		struct stat buf;

		if ((stat(path, &buf)) == 0) {
			f_info->mode  = buf.st_mode;
			f_info->nlink = buf.st_nlink;
			f_info->uid   = buf.st_uid;
			f_info->gid   = buf.st_gid;
			f_info->size  = buf.st_size;
			f_info->mtime = buf.st_mtime;
			return(f_info);
		}
	}
	return(NULL);
	
} /* end of WBGetFileInfo */ 
