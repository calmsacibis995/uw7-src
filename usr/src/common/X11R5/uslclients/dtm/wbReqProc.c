#pragma ident	"@(#)dtm:wbReqProc.c	1.129"

/******************************file*header********************************

    Description:
     This file contains the source code for handling client requests
	to delete and undelete a file.
*/
                              /* #includes go here     */

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"
#include "error.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static int MoveToWBProc(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *);
static int MoveFromWBProc(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *);
static int DisplayWBProc(Screen *scrn, XEvent *xevent, DtRequest *request,
		DtReply *);
static void ProcessWBFile(DmItemPtr itp, char *src_file, char *src_path);
static DmItemPtr GetItemPtr(char *pathname);
static void ReqClientProc(DmProcReason reason, XtPointer client_data,
		XtPointer call_data, char * src_file, char * dst_file);
static void RemoveWBObj(DmItemPtr itp, DmFileOpInfoPtr opr_info);

static int (*(wb_procs[]))() = {
	MoveToWBProc,
	MoveFromWBProc,
	DisplayWBProc
};

/***************************private*procedures****************************

    Private Procedures
*/


/****************************procedure*header*****************************
 * Initializes Wastebasket procedures to handle client requests.
 */
void
DmInitWBReqProcs(Widget w)
{
	static DmPropEventData wb_prop_ev_data = {
				(Atom)0,
				Dt__wb_msgtypes,
				wb_procs,
				XtNumber(wb_procs)
	};

	/*
	 * Since _WB_QUEUE is not constant,
	 * it cannot be initialized at compile time.
	 */
	wb_prop_ev_data.prop_name = _WB_QUEUE(XtDisplay(w));

	DmRegisterReqProcs(w, &wb_prop_ev_data);

} /* end of DmInitWBReqProcs */
	
/****************************procedure*header*****************************
 * Called when a file is dropped onto the Wastebasket icon in the home
 * folder and when a DT_MOVE_TO_WB request is received.
 */
int
DmMoveToWBProc2(char *pathname, Screen *scrn, XEvent *xevent,
	DtRequest *request)
{
	DmWBMoveFileReqPtr reqp;
	char *path;
	struct stat wbstat;
	DmFileOpInfoPtr opr_info;

	if ((stat(pathname, &wbstat)) != 0) {
		if (errno == ENOENT) {
			DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				True, TXT_DELREQ_NOFILE, basename(pathname));
			return(1);
		}
	}
	reqp = (DmWBMoveFileReqPtr)CALLOC(1,sizeof(DmWBMoveFileReqRec));
	if (request) {
		reqp->screen  = scrn;
		reqp->client  = xevent->xselectionrequest.requestor;
		reqp->replyq  = xevent->xselectionrequest.property;
		reqp->serial  = request->move_to_wb.serial;
		reqp->version = request->move_to_wb.version;
	}
	else
		reqp->client = None;

	path = strdup(pathname);

	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->options	= 0;		/* ie. no OVERWRITEs to WB */
	opr_info->src_path	= strdup(dirname(path));
	opr_info->src_list	= (char **)MALLOC(sizeof(char *));
	opr_info->src_cnt	= 1;
	opr_info->src_win	= NULL;
	opr_info->x		= opr_info->y = UNSPECIFIED_POS;

	FREE(path);

	if (WB_IMMEDIATELY(wbinfo)) {
		opr_info->type         = DM_DELETE;
		opr_info->target_path  = NULL;
		opr_info->dst_win      = NULL;
		reqp->op_type          = DM_IMMEDDELETE;
	} else {
		opr_info->type         = DM_MOVE;
		opr_info->target_path  = strdup(DM_WB_PATH(Desktop));
		opr_info->dst_win      = DESKTOP_WB_WIN(Desktop);
		reqp->op_type          = DM_MOVETOWB;
	}
	opr_info->src_list[0] = strdup(basename(pathname));

	(void)DmDoFileOp(opr_info, ReqClientProc, (XtPointer)reqp);
	/*
	 * This request will get a reply, but not now, since the work
	 * is done asynchronously.
	 */
	return(0);

} /* end of DmMoveToWBProc2 */

/****************************procedure*header*****************************
 * Calls DmMoveToWBProc2 when a DT_MOVE_TO_WB request is received to
 * delete a file.
 */
static int
MoveToWBProc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	if (request->move_to_wb.pathname == NULL ||
		DmMoveToWBProc2(request->move_to_wb.pathname,scrn,xevent,
			request))
	{
		reply->header.status = DT_FAILED;
		return(1);
	} else
		return(0);

} /* end of MoveToWBProc */

/****************************procedure*header*****************************
 * Called when file(s) in a folder is/are deleted and when a DT_MOVE_TO_WB
 * request is received.  Removes the file from source folder and adds it
 * to the wastebasket. Calls ProcessWBFile to save its time stamp, etc.
 */
void
DmMoveFileToWB(DmFileOpInfoPtr opr_info, Boolean client_req)
{
	char **src;
	DtAttrs *src_info;
	int save_nfile = NUM_WB_OBJS(Desktop);

	DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	if (opr_info->src_win == NULL)
		opr_info->src_win = DmQueryFolderWindow(opr_info->src_path,
				NULL);

	if (opr_info->dst_win == NULL)
		opr_info->dst_win = DESKTOP_WB_WIN(Desktop);

	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);

	/* Now find the new item(s) and "process" them.  Pass in basename of
	 * src file to ProcessWBFile() - it could be from a Found window in
	 * long view.
	 */
	for (src = opr_info->src_list, src_info = opr_info->src_info;
		src < opr_info->src_list + opr_info->cur_src; src++,src_info++)
	{
		DmItemPtr itp;
		char *path;
		char *dir;

		if (*src_info & SRC_B_IGNORE)
			continue;

		itp = DmObjNameToItem((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				basename(*src));

		if (itp == NULL)	/* shouldn't be */
		    continue;

		path = strdup( DmMakePath(opr_info->src_path, *src) );
		dir = strdup(path);
		ProcessWBFile(itp, basename(path), dirname(dir));
		FREE(path);
		FREE(dir);
	}

	/* Nothing done if cur_src = 0 or all items skipped */
	if (src == opr_info->src_list)
		return;

	/* Update wastebasket's .dtinfo file */
	DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

	if (!WB_IS_EMPTY(Desktop))
		DmTouchIconBox((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL, 0);

	if (save_nfile == 0) {
		/* wb was previously empty */
		DmSwitchWBIcon();
		if (WB_BY_TIMER(wbinfo)) {
			if (wbinfo.restart)
				wbinfo.restart = wbinfo.suspend = False;
			DmWBRestartTimer();
		}
	}
}				/* end of DmMoveFileToWB */

/****************************procedure*header*****************************
 * Saves information about a file that was just deleted as file instance
 * properties. 
 */
static void
ProcessWBFile(DmItemPtr itp, char *src_file, char *src_path)
{
	int vnum;
	time_t time_stamp;
	char *p;
	char *path;

	sprintf(Dm__buffer, "%s/%s", src_path, src_file);
	DmSetObjProperty(ITEM_OBJ(itp), REAL_PATH, Dm__buffer, NULL);

	/* Save the object's _ICONFILE property as an instance property.
	 * This is necessary because only the object's instance properties
	 * are saved when WB exits.  It is needed when WB is restarted to
	 * determine which icon file to use for a particular object, if it's
	 * not of a built-in class.
	 */
	if (DtGetProperty(&(ITEM_OBJ(itp)->plist), "_ICONFILE", NULL) == NULL)
		DmSetObjProperty(ITEM_OBJ(itp), "_ICONFILE",
			DtGetProperty(&(ITEM_OBJ(itp)->fcp->plist),
				"_ICONFILE", NULL), NULL);

	DmSetObjProperty(ITEM_OBJ(itp), CLASS_NAME,
		DmObjClassName(ITEM_OBJ(itp)), NULL);

	path = tempnam(DM_WB_PATH(Desktop), NULL);
	sprintf(Dm__buffer, "%s/%s", DM_WB_PATH(Desktop), ITEM_OBJ_NAME(itp));
	if (rename((const char *)Dm__buffer, (const char *)path) != 0) {
		Dm__VaPrintMsg(TXT_CANT_RENAME, Dm__buffer, path);
	}
	p = basename(path);
	FREE(ITEM_OBJ_NAME(itp));
	ITEM_OBJ_NAME(itp) = strdup(p);

	time(&time_stamp);
	sprintf(Dm__buffer, "%ld", time_stamp);
	DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP, Dm__buffer, NULL);

	vnum = DmWBGetVersion(itp, src_file);
	sprintf(Dm__buffer, "%d", vnum);
	DmSetObjProperty(ITEM_OBJ(itp), VERSION, Dm__buffer, NULL);

	sprintf(Dm__buffer, "%s:%d", src_file, vnum);
	if (ITEM_LABEL(itp))
		FREE(ITEM_LABEL(itp));
	MAKE_WRAPPED_LABEL(itp->label,
		 FPART(DESKTOP_WB_WIN(Desktop)->views[0].box).font, Dm__buffer);

	DmSizeIcon(DESKTOP_WB_WIN(Desktop)->views[0].box, itp);
	FREE(path);

} /* end of ProcessWBFile */

/****************************procedure*header*****************************
 * Undeletes a file when a DT_MOVE_FROM_WB request is received.
 * It first checks if the file exists in the Wastebasket.
 */
static int
MoveFromWBProc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	DmItemPtr itp;
	DmWBMoveFileReqPtr reqp;
	DmFileOpInfoPtr opr_info;

	itp = GetItemPtr(request->move_from_wb.pathname);

	if (itp == NULL) {
		reply->header.status = DT_FAILED;
		return(1);
	}
	reqp = (DmWBMoveFileReqPtr)CALLOC(1, sizeof(DmWBMoveFileReqRec));
	reqp->screen  = scrn;
	reqp->client  = xevent->xselectionrequest.requestor;
	reqp->replyq  = xevent->xselectionrequest.property;
	reqp->serial  = request->move_to_wb.serial;
	reqp->version = request->move_to_wb.version;
	reqp->op_type = DM_MOVEFRWB;
	reqp->itp     = itp;

	/* Initialize file's op->objectdata to its stat info */
	if (ITEM_OBJ(itp)->objectdata == NULL) {
		char *path=DmMakePath(DM_WB_PATH(Desktop),ITEM_OBJ_NAME(itp));
		ITEM_OBJ(itp)->objectdata = (void *)WBGetFileInfo(path);
	}
	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->type        = DM_MOVE;
	opr_info->options     = 0;
	opr_info->target_path = strdup(request->move_from_wb.pathname);
	opr_info->src_path    = strdup(DM_WB_PATH(Desktop));
	opr_info->src_list    = (char **)MALLOC(sizeof(char *));
	opr_info->src_cnt     = 1;
	opr_info->src_win     = DESKTOP_WB_WIN(Desktop);
	opr_info->dst_win     = NULL;
	opr_info->x           = opr_info->y = UNSPECIFIED_POS;
	opr_info->src_list[0] = strdup(ITEM_OBJ_NAME(itp));
	(void)DmDoFileOp(opr_info, ReqClientProc, (XtPointer)reqp);

	/* This request will get a reply, but not now, since the work
	 * is done asynchronously.
	 */
	return(0);

} /* end of MoveFromWBProc */

/****************************procedure*header*****************************
 * Called to open the Wastebasket window when requested by the user via the
 * Wastebasket icon in the home folder.
 */
static int
DisplayWBProc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
	/*
	 * This code assume that the wastebasket window is always created
	 * up front.
	 */
	DmMapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	DmDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	return(0);

} /* end of DisplayWBProc */

/****************************procedure*header*****************************
 * Returns the item ptr of item whose object name matches pathname
 * and is the most recently deleted file, if there is more than one
 * match.
 *
 * It displays an error message if the file to be undeleted already
 * exist in the destination and returns NULL.
 *
 * This routine is called from MoveFromWBProc and can only be used
 * with items in the wastebasket.
 */

static DmItemPtr
GetItemPtr(char *pathname)
{
	int i;
	char *real_path;
	DmItemPtr itp;
	DmItemPtr citp = NULL;	/* current itp */
	time_t cts = 0;      	/* current time stamp */
	time_t nts;          	/* new time stamp */

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	     i < DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++, i++) {

		if (ITEM_MANAGED(itp) && ITEM_SENSITIVE(itp)) {
			real_path = (char *)DmGetObjProperty(ITEM_OBJ(itp),
					REAL_PATH, NULL);
			if (strcmp(real_path, pathname) == 0) {

				/*check if file already exists in destination*/
				struct stat wbstat;

				if ((stat(real_path, &wbstat)) == 0) {
					DmVaDisplayStatus(
					  (DmWinPtr)DESKTOP_WB_WIN(Desktop),
					  True, TXT_WB_CANT_OVERWRITE,
					  pathname);
					return(NULL);
				} else {
					nts = strtoul(
						DmGetObjProperty(ITEM_OBJ(itp),
							TIME_STAMP, NULL),
							NULL, 0);
					if (cts < nts) {
						cts = nts;
						citp = itp;
					}
				}
			}
		}
	}

	if (citp == NULL) {
		/*no file in wastebasket has real name that matches pathname*/
		DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), True,
			TXT_UNDELREQ_NOFILE, basename(pathname));
		return(NULL);
	} else {
		return(citp);
	}

} /* end of GetItemPtr */

/****************************procedure*header*****************************
 * Called when a file is undeleted.
 */
void
DmMoveFileFromWB(DmFileOpInfoPtr opr_info)
{
	char **src;
	DtAttrs *src_info;
	DmItemPtr itp;

	DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	if (opr_info->cur_src == 0)	/* There is nothing to do */
		return;

	for (src = opr_info->src_list, src_info = opr_info->src_info;
		src < opr_info->src_list + opr_info->cur_src; src++,src_info++)
	{
		char *orig_name;
		char *real_path;
		DmFileOpInfoPtr new_opr;
		DmWBCPDataPtr cpdp;
		DmItemPtr citp = NULL;	/* Current pointer */
		time_t	  cts = 0;
		time_t 	  nts;

		if (*src_info & SRC_B_IGNORE)
			continue;

		orig_name = strdup(DmMakePath(opr_info->src_path, *src));
		for (itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
			itp < DESKTOP_WB_WIN(Desktop)->views[0].itp +
			DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++)
		{
			if (!ITEM_MANAGED(itp) || !ITEM_SENSITIVE(itp))
				continue;

			real_path = DmGetObjProperty(ITEM_OBJ(itp), REAL_PATH,
					NULL);

			/* It's possible more than one file of the same name

			 * from the same folder is now in wb. We are not
			 * handling that case.
			 */
			if (strcmp(orig_name, real_path) == 0) {
				nts = strtoul(DmGetObjProperty(ITEM_OBJ(itp),
						TIME_STAMP, NULL), NULL, 0);
				if (cts <= nts) {
					cts = nts;
					citp = itp;
				}
			}
		}
		if (citp) {
			/* Initialize file's op->objectdata to its stat info */
			if (ITEM_OBJ(citp)->objectdata == NULL) {
				char *path = DmMakePath(DM_WB_PATH(Desktop),
							ITEM_OBJ_NAME(citp));
				ITEM_OBJ(citp)->objectdata =
					(void *)WBGetFileInfo(path);
			}

			cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
			cpdp->op_type = DM_PUTBACK;
			cpdp->itp     = citp;

			new_opr = (DmFileOpInfoPtr)CALLOC(1,
				sizeof(DmFileOpInfoRec));
			new_opr->type        = DM_MOVE;
			new_opr->options        = UNDO;
			new_opr->target_path = orig_name;
			new_opr->src_path    = strdup(DM_WB_PATH(Desktop));
			new_opr->src_list    = (char **)MALLOC(sizeof(char *));
			new_opr->src_cnt     = 1;
			new_opr->dst_win     = NULL;
			new_opr->src_win     = DESKTOP_WB_WIN(Desktop);
			new_opr->x           = new_opr->y = UNSPECIFIED_POS;
			new_opr->src_list[0] = strdup(ITEM_OBJ_NAME(citp));

			(void)DmDoFileOp(new_opr, DmWBClientProc,
				(XtPointer)cpdp);
		}
	}
}					/* end of DmMoveFileFromWB */

/****************************procedure*header*****************************
	RemoveWBObj - Called when a DT_MOVE_FROM_WB request is successful to
	"remove" undeleted object from the Wastebasket, and suspend timer,
	if necessary.
 */
static void
RemoveWBObj(DmItemPtr itp, DmFileOpInfoPtr opr_info)
{
	/* opr_info->target_path must always be a full pathname */
	FREE(ITEM_OBJ_NAME(itp));
	ITEM_OBJ_NAME(itp) = strdup(basename(opr_info->target_path));
	FREE(opr_info->src_list[0]);
	opr_info->src_list[0] = strdup(ITEM_OBJ_NAME(itp));

	if (opr_info->dst_win == NULL) {
		char *dup_target = strdup(opr_info->target_path);

		opr_info->dst_win = DmQueryFolderWindow(dirname(dup_target),
					NULL);
		FREE(dup_target);
	}
	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);
	DmSwitchWBIcon();
	DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

	if (WB_IS_EMPTY(Desktop) && WB_BY_TIMER(wbinfo) &&
		wbinfo.suspend == False)
	{
		DmWBRestartTimer();
		wbinfo.restart = True;
	}
}					/* end of RemoveWBObj */

/****************************procedure*header*****************************
 * Client proc called when a delete /undelete operation in response
 * a client request is completed or failed. 
 */
static void
ReqClientProc(DmProcReason reason, XtPointer client_data,
		  XtPointer call_data, char *src, char *dst)
{
	int ret;
	int rptype;
	DmTaskInfoListPtr tip = (DmTaskInfoListPtr)call_data;
	DmWBMoveFileReqPtr reqp = (DmWBMoveFileReqPtr)client_data;
	DmFileOpInfoPtr opr_info = tip->opr_info;

	switch(reason) {
	case DM_DONE:
		if ( !(opr_info->src_info[0] & SRC_B_IGNORE) ) {
			switch(reqp->op_type) {
			case DM_MOVETOWB:
			{
				DmFolderWinPtr src_win =
					DmQueryFolderWindow(opr_info->src_path,
						NULL);
				if (src_win)
					DmVaDisplayStatus((DmWinPtr)src_win,
						False, TXT_DELETE_DONE, NULL);
				DmMoveFileToWB(opr_info, True); 
				rptype = DT_MOVE_TO_WB;
			}
				break;

			case DM_IMMEDDELETE:
				if (opr_info->src_win == NULL)
					opr_info->src_win=DmQueryFolderWindow(
						opr_info->src_path, NULL);
				DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
				rptype = DT_MOVE_TO_WB;
				break;

			case DM_MOVEFRWB:
				RemoveWBObj(reqp->itp, opr_info);
				rptype = DT_MOVE_FROM_WB;
				break;

			default:
				break;
			}
			ret = DT_OK;
		}
		DmFreeTaskInfo(tip);
		break;

	case DM_ERROR:
		switch(reqp->op_type) {
		case DM_MOVETOWB:
		case DM_IMMEDDELETE:
		{
			char buf[PATH_MAX];

			if (ROOT_DIR(opr_info->src_path))
				sprintf(buf, "/%s", opr_info->src_list[0]);
			else
				sprintf(buf, "%s/%s", opr_info->src_path,
					opr_info->src_list[0]);
			DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				True, TXT_WB_DELETE_FAILED, buf);
			rptype = DT_MOVE_TO_WB;
			/* remove any partially deleted files */
			sprintf(buf, "rm -rf %s/%s", DM_WB_PATH(Desktop),
				opr_info->src_list[0]);
			system(buf);
		}
			break;

		case DM_MOVEFRWB:
			/*Overwrite conditions are checked before DoFileOp().*/
			DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				True, TXT_WB_PUTBACK_FAILED,
				basename(opr_info->target_path));
			rptype = DT_MOVE_FROM_WB;
			break;
		default:
			break;
		}
		ret = DT_FAILED;
		break;
	default:
		break;
	}

	if (reqp->client != None) {
		DtReply reply;

		reply.header.rptype = rptype;
		reply.header.status = ret;
		reply.header.version= reqp->version;
		reply.header.serial = reqp->serial;
		DtSendReply(reqp->screen, reqp->replyq, reqp->client, &reply);
		/* set client to None so that it doesn't get a reply for
		 * the DM_DONE case.
		 */
		if (reason == DM_ERROR)
			reqp->client = None;
		else
			free(reqp);
	}
} /* end of ReqClientProc */
