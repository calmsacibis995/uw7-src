#pragma ident	"@(#)dtm:f_update.c	1.157.2.4"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-releated callbacks.
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#ifdef DEBUG
#include <X11/Xmu/Editres.h>	
#endif

#include <Xm/Protocols.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/ModalGizmo.h>
#include <MGizmo/PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations
 */
static void	AddNameToParent(char *, char *);
static void	FileOpDone(DmFileOpInfoPtr opr_info);
static DmWinPtr	GetParentWin(char * target_path);
static void	OverwriteCB(Widget, XtPointer, XtPointer);
static void	DontOverwriteCB(Widget, XtPointer, XtPointer);
static void	OverwriteHelpCB(Widget, XtPointer, XtPointer);
static void	NameChangeContinueCB(Widget, XtPointer, XtPointer);
static void	NameChangeDiscontinueCB(Widget, XtPointer, XtPointer);
static void	StopKeyEH(Widget, XtPointer, XEvent *, Boolean *);
static void	RmNameFromParent(char * dir, char * name);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Define gizmo for Overwrite Notice */

static MenuItems overwrite_menubarItems[] = {
    MENU_ITEM( TXT_OVERWRITE,	  TXT_M_OVERWRITE,	OverwriteCB ),
    MENU_ITEM( TXT_DONT_OVERWRITE,TXT_M_DONT_OVERWRITE,	DontOverwriteCB ),
    MENU_ITEM( TXT_HELP,	  TXT_M_HELP,		OverwriteHelpCB ),
    { NULL }					/* NULL terminated */
};
MENU_BAR("overwriteNoticeMenubar", overwrite_menubar, NULL, 1, 1);

static ModalGizmo overwrite_notice_gizmo = {
    NULL,				/* help info */
    "overwriteNotice",			/* shell name */
    TXT_G_FOLDER_TITLE,			/* title */
    &overwrite_menubar,			/* menu */
    NULL,				/* message (run-time) */
    NULL, 0,				/* gizmos, num_gizmos */
    XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
    XmDIALOG_WARNING			/* type */
};

/* Define gizmo for Name Change Notice */

static MenuItems name_change_menubarItems[] = {
    MENU_ITEM(TXT_CONTINUE,      TXT_M_CONTINUE,      NameChangeContinueCB),
    MENU_ITEM(TXT_CANCEL,	 TXT_M_Cancel,        NameChangeDiscontinueCB),
    { NULL }				/* NULL terminated */
};
MENU_BAR("nameChangeNoticeMenubar", name_change_menubar, NULL, 1, 1);

static ModalGizmo name_change_notice_gizmo = {
    NULL,				/* help info */
    "nameChangeNotice",			/* shell name */
    TXT_G_FOLDER_TITLE,			/* title */
    &name_change_menubar,		/* menu */
    TXT_NMCHG_NOTICE_1,			/* message */
    NULL, 0,				/* gizmos, num_gizmos */
    XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
    XmDIALOG_WARNING			/* type */
};


/***************************private*procedures****************************

    Private Procedures
*/
/*****************************************************************************
 *  	AddNameToParent: add an object to parent container of path
 *		NOTE: we can't specify an X and Y position because
 *		we only have a container name, not a folder window.
 *		There may be many folder windows viewing the same
 *		container.
 *	INPUTS:	
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/

static void
AddNameToParent(char * dir, char * name)
{
    DmContainerPtr parent;
    char *dup_path;
    
    dup_path = strdup(dir);

    if ((parent = DmQueryContainer(dirname(dup_path))) != NULL){
	(void)DmAddObjectToContainer(parent, (DmObjectPtr)NULL, name,
				     DM_B_SET_TYPE | DM_B_INIT_FILEINFO);
    }
    FREE(dup_path);
}					/* end of AddNameToParent */
/****************************procedure*header*****************************
 * BusyWindow-
 */
static void
BusyWindow(Widget shell, Boolean busy, XtPointer client_data)
{
    Widget focus_w;

    DmBusyWindow(shell, busy);

    if (busy && (focus_w = XmGetFocusWidget(shell)))
    {
	if (!XtIsWidget(focus_w))
	    focus_w = XtParent(focus_w);
	XtInsertEventHandler(focus_w, KeyPressMask, False,
			     StopKeyEH, client_data, XtListHead);
    }
}

/****************************procedure*header*****************************
 * BusyAllWindows- [un]busy folder window at start or end of file op.
 *	Also busy any popups in an attempt to "serialize" file op requests
 *	from user.  Only single file op can occur at a time for a given src
 *	window.
 */
static void
BusyAllWindows(DmFolderWindow folder, Boolean busy, XtPointer client_data)
{
    FolderPromptInfoRec const * info;

    for (info = FolderPromptInfo;
	 info < FolderPromptInfo + NumFolderPromptInfo; info++)
    {
	Gizmo * gizmo = (Gizmo *)(((char *)folder) + info->gizmo_offset);
	Widget	shell;

	if (*gizmo && (shell = (*info->get_gizmo_shell)(*gizmo)))
	    BusyWindow(shell, busy, client_data);
    }

    /* Busy the file prop sheets to prevent kicking off a rename file-op */
    DmBusyPropSheets(folder, busy);

    /* Busy the folder window itself */
    BusyWindow(folder->shell, busy, client_data);

    if (busy)
	folder->attrs |= DM_B_FILE_OP_BUSY;
    else
	folder->attrs &= ~DM_B_FILE_OP_BUSY;

}					/* end of BusyAllWindows */

static void
DestroyFileOpNotices(DmFolderWindow folder)
{
    Widget shell;

    if (folder->overwriteNotice)
    {
	if (shell = GetModalGizmoShell(folder->overwriteNotice))
	    XtDestroyWidget(shell);
	FreeGizmo(ModalGizmoClass, folder->overwriteNotice);
	folder->overwriteNotice = NULL;
    }
    if (folder->nmchgNotice)
    {
	if (shell = GetModalGizmoShell(folder->nmchgNotice))
	    XtDestroyWidget(shell);
	FreeGizmo(ModalGizmoClass, folder->nmchgNotice);
	folder->nmchgNotice = NULL;
    }
}

/****************************procedure*header*****************************
    FileOpDone- do whatever's necessary following a completed file operation
*/
static void
FileOpDone(DmFileOpInfoPtr opr_info)
{
    switch(opr_info->type)
    {
    case DM_DELETE:			/* for undo-ing move, copy or links */
	/* FLH MORE: just call Dm__SyncContainer here?  File ops
	 * cause a sync to occur anyway.  Why waste our time updating
	 * the window
	 */
	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
	break;

    case DM_COPY:
    case DM_HARDLINK:
    case DM_SYMLINK:
	/* If the target is an existing dir, update it if it's open.
	   Otherwise, if a directory was created, display the glyph in the
	   parent folder if open.
	*/
	if ((opr_info->attrs & DM_B_DIR_EXISTS) ||
	    ( !(opr_info->attrs & DM_B_DIR_NEEDED_4FILES) ))

	    DmUpdateWindow(opr_info, DM_UPDATE_DSTWIN);

	else

	    AddNameToParent(opr_info->target_path, 
			    basename(opr_info->target_path));
	break;
	
    case DM_MOVE:
	if (IS_WB_PATH(Desktop, opr_info->target_path))
	{
	    DmMoveFileToWB(opr_info, False);
	    break;
	}

	/* Update src and dst windows */
	DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);

	/* If new dir was created, display it in parent (if any) */
	if (opr_info->attrs & DM_B_DIR_NEEDED_4FILES)
	    AddNameToParent(opr_info->target_path,
			    basename(opr_info->target_path));
	break;

    default:
	break;
    }

    /* For UNDO, remove the dir icon from the parent dir if one was created
       originally on behalf of the user (the mutli-src case).
    */
    if (opr_info->options & RMNEWDIR)
    {
	DmContainerPtr cp;

	/* Close folder if one is open on this dir (i.e. original target) */
	if ((cp = DmQueryContainer(opr_info->src_path)) != NULL)
	    DmDestroyContainer(cp);

	/* Remove folder glyph from parent if open */
	RmNameFromParent(opr_info->src_path, basename(opr_info->src_path));
    }
}					/* end of FileOpDone */

/****************************procedure*header*****************************
    GetParentWin-
*/
static DmWinPtr
GetParentWin(char * path)
{
    DmFolderWindow	folder;
    char *		dup_path = strdup(path);

    /* Any folder that matches this path will do */
    folder = DmQueryFolderWindow(dirname(dup_path), NULL);
    FREE((void *)dup_path);

    return((DmWinPtr)folder);
}

static void
NameChangeContinue(DmFolderWindow folder, Boolean contin)
{
    XtPopdown(GetModalGizmoShell(folder->nmchgNotice));
    Dm__NameChange(folder->task_id, contin);
}

static void
NameChangeContinueCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    NameChangeContinue((DmFolderWindow)client_data, True);
} 

static void
NameChangeDiscontinueCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    NameChangeContinue((DmFolderWindow)client_data, False);
} 

static void
Overwrite(DmFolderWindow folder, Boolean overwrite)
{
    XtPopdown(GetModalGizmoShell(folder->overwriteNotice));
    Dm__Overwrite(folder->task_id, overwrite);

}

static void
DontOverwriteCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Overwrite((DmFolderWindow)client_data, False);
}

/****************************procedure*header*****************************
  OverwriteCB-
*/
static void
OverwriteCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Overwrite((DmFolderWindow)client_data, True);
}

/****************************procedure*header*****************************
  OverwriteHelpCB - Displays help on the overwrite prompt.
*/
static void
OverwriteHelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)),
			 NULL, FOLDER_HELPFILE, FOLDER_OVERWRITE_SECT);
}

/****************************procedure*header*****************************
 * PositionItem-
 */
static void
PositionItem(DmFileOpInfoPtr opr_info, Cardinal *prev_indx, DmItemPtr new_item)
{
    if ((opr_info->x != UNSPECIFIED_POS) &&
	(opr_info->dst_win->views[0].view_type == DM_ICONIC))
    {
	if (*prev_indx != ExmNO_ITEM)	/* ie, not the 1st item */
	{
	    /* For 2nd & subsequent items, cascade from 1st item */
	    opr_info->x += GLYPH_PTR(new_item)->width / 2;
	    opr_info->y += GLYPH_PTR(new_item)->height / 2;
	}

	/* New item becomes previous item (index) */
	*prev_indx = new_item - opr_info->dst_win->views[0].itp;

	/* Adjust x/y: x/y is hot spot, not upper-left corner of item. */
	XtSetArg(Dm__arg[0], XtNx, opr_info->x - (ITEM_WIDTH(new_item) / 2));
	XtSetArg(Dm__arg[1], XtNy, opr_info->y -
		 (GLYPH_PTR(new_item)->height / 2));
	ExmFlatSetValues(opr_info->dst_win->views[0].box,
			 *prev_indx, Dm__arg, 2);
    }
}

static void
PopupFileOpNoticeGizmo(Gizmo notice, XtPointer client_data)
{
    Widget shell = GetModalGizmoShell(notice);

    /* 'shell' and 'client_data' would be used for adding grab and event
     * handler for STOP key.  This was problematic since couldn't get focus
     * widget of notice (before it is popped up?).  Maybe next release...
     */

    DmBeep();
    MapGizmo(ModalGizmoClass, notice);

#ifdef DEBUG
    XtAddEventHandler(shell, (EventMask) 0, True,
		      _XEditResCheckMessages, NULL);
#endif
}

static void
PromptNameChange(char *msg, DmFolderWindow win, char *file)
{
    Gizmo notice;

    /* Create the Notice gizmo (once per file op) */
    if ((notice = win->nmchgNotice) == NULL)
    {
	Widget shell;

	name_change_menubarItems[0].clientData =
	    name_change_menubarItems[1].clientData = (char *)win;
	XtSetArg(Dm__arg[0], XmNmessageAlignment, XmALIGNMENT_CENTER); 
	notice = win->nmchgNotice =
	    CreateGizmo(win->shell, ModalGizmoClass,
			&name_change_notice_gizmo, Dm__arg, 1);

	shell = GetModalGizmoShell(notice);
	XmAddWMProtocolCallback(shell, XA_WM_DELETE_WINDOW(XtDisplay(shell)),
				NameChangeDiscontinueCB, (XtPointer)win);
    }
    SetModalGizmoMessage(notice, GGT(msg));
    PopupFileOpNoticeGizmo(notice, win);
}					/* End of PromptNameChange */

/****************************procedure*header*****************************
    PromptOverwrite- popup overwrite notice.
	The round-about way of passing the current task ptr to the callback
	is to first register src_win as the client_data before CreateGizmo.
	This will be passed to the widget when created.  Then client_data is
	set to DESKTOP_CUR_TASK afterwards which is not set on the widget but
	can be accessed from the notice callbacks.
*/
static void
PromptOverwrite(DmFolderWindow win, char * file)
{
    Gizmo notice;

    /* Create the Notice gizmo (once per file op) */
    if ((notice = win->overwriteNotice) == NULL)
    {
	Widget shell;

	sprintf(Dm__buffer, Dm__gettxt(TXT_OVERWRITE_NOTICE), "xxxx");
	overwrite_notice_gizmo.message = Dm__buffer;
	overwrite_menubarItems[0].clientData = 
	overwrite_menubarItems[1].clientData =
	overwrite_menubarItems[2].clientData = (char *)win;
	XtSetArg(Dm__arg[0], XmNmessageAlignment, XmALIGNMENT_CENTER); 
	notice = win->overwriteNotice =
	    CreateGizmo(win->shell, ModalGizmoClass,
			&overwrite_notice_gizmo, Dm__arg, 1);

	shell = GetModalGizmoShell(notice);
	XmAddWMProtocolCallback(shell, XA_WM_DELETE_WINDOW(XtDisplay(shell)),
				DontOverwriteCB, (XtPointer)win);

	/* register callback for context-sensitive help */
	XtAddCallback(GetModalGizmoDialogBox(notice), XmNhelpCallback,
		OverwriteHelpCB, NULL);
    }

    /* Construct I18N Notice text with file name, etc */
    sprintf(Dm__buffer, Dm__gettxt(TXT_OVERWRITE_NOTICE), file);
    SetModalGizmoMessage(notice, Dm__buffer);
    PopupFileOpNoticeGizmo(notice, win);
}					/* end of PromptOverwrite */

/****************************procedure*header*****************************
 *  	RmNameFromParent: remove an object from its parent container
 * 	Dependent views will be notified via callbacks
 *	INPUTS:	name of directory 
 *		name of object to be removed
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
static void
RmNameFromParent(char * dir, char * name)
{
    DmContainerPtr 	cp;
    DmObjectPtr 	op;
    char *		dup_path;
    
#ifdef DEBUG
    fprintf(stderr, "RmNameFromParent: %s: %s\n", dir, name);
#endif

    dup_path = strdup(dir);
    if ((cp = DmQueryContainer(dirname(dup_path))) &&
	(op = DmGetObjectInContainer(cp, name))) {
	DmRmObjectFromContainer(cp, op);
    }
    FREE((void *)dup_path);
}				/* end of RmNameFromParent */

/****************************procedure*header*****************************
 * SkippedAllItems-
 */
static Boolean
SkippedAllItems(DmFileOpInfoPtr opr_info)
{
    DtAttrs  * attrs;

    for (attrs = opr_info->src_info;
	 attrs < opr_info->src_info + opr_info->src_cnt; attrs++)
	if (!(*attrs & SRC_B_SKIP_OVERWRITE))
	    break;

    return(attrs == opr_info->src_info + opr_info->src_cnt);
}


/****************************procedure*header*****************************
 * StopKeyEH- callback called when user presses STOP key during file operation.
 *	Popdown any overwrite notice, close any open file descriptors, and
 *	removes all un-executed tasks from the list, effectively marking the
 *	end of the operation. Note, Undo operation will undo only what was
 *	done before the STOP key was entered. There may be some
 *	synchronization problem here, if an undo is invoked right after STOP.
 */

static void
StopKeyEH(Widget widget, XtPointer client_data,
	  XEvent *event, Boolean * continue_to_dispatch)
{
    static KeyCode	s_keycode = (KeyCode)0;
    XKeyPressedEvent *	keyEvent = (XKeyPressedEvent *)event;
    DmFolderWindow	folder;
    DmTaskInfoListPtr	tip;

    if (keyEvent->type != KeyPress)
	return;

    if (((folder = (DmFolderWindow)client_data) == NULL) ||
	((tip = folder->task_id) == NULL))
	return;

    if (s_keycode == (KeyCode)0)	/* ie, 1st time */
    {
	KeySym	keysym = XStringToKeysym("s");
	KeyCode	keycode;

	if (keysym == NoSymbol ||
            !(keycode = XKeysymToKeycode(XtDisplay(folder->shell), keysym)))
	    return;
	s_keycode = keycode;
    }

    if ((keyEvent->state != ControlMask) || (keyEvent->keycode != s_keycode))
	return;

    /* STOP key (<Ctrl>s) has been pressed */

    *continue_to_dispatch = False;
    tip->opr_info->attrs |= DM_B_FILE_OP_STOPPED;

    /* Popdown overwrite notice if one is up */
    if (folder->overwriteNotice)
	XtPopdown(GetModalGizmoShell(folder->overwriteNotice));

    /* Popdown name-change notice if one is up */
    if (folder->nmchgNotice)
	XtPopdown(GetModalGizmoShell(folder->nmchgNotice));

    DmStopFileOp(tip);		/* clean up tasks after "stopping" */

}					/* end of StopKeyEH */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmFolderFMProc- A client_proc for the folder window file manipulation.
	It is called when an operation begins, error occurs, successful
	completion, to report progress of the operation.
*/
void 
DmFolderFMProc(DmProcReason reason, XtPointer client_data, XtPointer call_data,
	       char * src_file, char * dst_file)
{
    DmTaskInfoListPtr	tip = (DmTaskInfoListPtr)call_data;
    DmFileOpInfoPtr	opr_info = tip->opr_info;
    DmFolderWindow 	src_win;
    DmFolderWindow 	dst_win;
    DmFolderWindow 	tmp_win;

    src_win = opr_info->src_win;
    dst_win = opr_info->dst_win;

    switch(reason)
    {
    case DM_OPRBEGIN:
	/* Operation is about to begin, busy out src_win & dst_win as
	 * appropriate.  Always pass src_win for client_data.  This is used
	 * by StopKeyEH which needs access to tip.
	 */
	/* FLH MORE:  It is no longer appropriate to simply use
	   DmQueryFolderWindow to determine what folder to busy.
	   There may be multiple views into the same container,
	   so the result will be arbitrary.  We need to busy
	   the folder that is involved in the file operation,
	   so that another file operation does not destroy
	   the first.
	 */
	if (opr_info->options & UNDO)
	{
		register DmFolderWindow window = DESKTOP_FOLDERS(Desktop);

		/* make sure src_win still exists */
		if (src_win && src_win != dst_win) {
			for (; window; window = window->next)
			{
				/* folders are reused in open-in-place so make sure the paths
				 * match as well.
				 */
				if (src_win == window &&
				!strcmp(src_win->views[0].cp->path,window->views[0].cp->path))
				{
					/* src_win exists, break out of loop */
					break;
				}
			}
			/* no match found, set src_win to NULL */
			src_win = NULL;
		}

	    if (src_win == NULL)
	    {
		src_win = (opr_info->attrs & DM_B_ADJUST_TARGET) ?
		    DmQueryFolderWindow(opr_info->src_path, NULL) :
		(DmFolderWindow)GetParentWin(opr_info->src_path);
	    }
	    if (dst_win ||
		(!(opr_info->options & MULTI_PATH_SRCS) &&
		 (dst_win = DmQueryFolderWindow(opr_info->target_path, NULL))))
		BusyAllWindows(dst_win, True, src_win);

	    if (src_win && (src_win != dst_win))
		BusyAllWindows(src_win, True, src_win);

	} else
	{
	    if (src_win ||
		(!(opr_info->options & MULTI_PATH_SRCS) &&
		 (src_win = DmQueryFolderWindow(opr_info->src_path, NULL))))
		BusyAllWindows(src_win, True, src_win);

	    /* Find dst_win: if the target is a dir being created by this op,
	     * there will be no dst_win.  Otherwise, use simply the target path
	     * when the target needs adjusting or the parent when the target
	     * is "fully qualified".
	     */
	    if (!(opr_info->attrs & DM_B_DIR_NEEDED_4FILES) &&
		(dst_win == NULL) && (opr_info->type != DM_DELETE))
	    {
		dst_win = (opr_info->attrs & DM_B_ADJUST_TARGET) ?
		    DmQueryFolderWindow(opr_info->target_path, NULL) :
		(DmFolderWindow)GetParentWin(opr_info->target_path);
	    }

	    if (dst_win && (dst_win != src_win))
		BusyAllWindows(dst_win, True, src_win);
	}
	opr_info->src_win = src_win;
    	opr_info->dst_win = dst_win;
	break;

    case DM_DONE:
	/* operation successfully completed.
	 * Note: DM_DONE is always called (no option is needed) so must be
	 * careful to test for NULL src_win & dst_win.
	 */

	if (src_win)		/* Unbusy src_win if any */
	{
	    BusyAllWindows(src_win, False, NULL);
	    DestroyFileOpNotices(src_win); /* Destroy any file op notices */
	}

	/* Unbusy dst_win if any and if !src_win */
	if (dst_win && (dst_win != src_win))
	    BusyAllWindows(dst_win, False, NULL);

	FileOpDone(opr_info);		/* Update folder(s) */

	/* Report that file op is done.  If file op was stopped or if all
	 * items were "skipped", report that file op was stopped.  Otherwise,
	 * report success.
	 */

	tmp_win = (opr_info->options & UNDO) ? dst_win : src_win;
	if (tmp_win)
	{
	    if ((opr_info->attrs & DM_B_FILE_OP_STOPPED) ||
		(!(opr_info->options & UNDO) && SkippedAllItems(opr_info)))
	    {
		DmVaDisplayStatus((DmWinPtr)tmp_win, True,
				  TXT_FILE_OP_STOPPED);

	    } else if (!opr_info->error)
	    {
		DmVaDisplayStatus((DmWinPtr)tmp_win, False,
			(opr_info->type == DM_DELETE	? TXT_DELETE_DONE :
			 opr_info->type == DM_COPY	? TXT_COPY_DONE :
			 opr_info->type == DM_HARDLINK	? TXT_LINK_DONE :
			 opr_info->type == DM_SYMLINK	? TXT_LINK_DONE :
			 opr_info->type == DM_MKDIR	? TXT_MKDIR_DONE :

			 /* This is ugly.  If this is a move and the target
			  * is the WB, it's really a delete.
			  */
			 opr_info->type == DM_MOVE	?
			 (IS_WB_PATH(Desktop, opr_info->target_path)
							? TXT_DELETE_DONE :
							  TXT_MOVE_DONE) :
			/* else */			  NULL),

			src_file);
	    }
	}
	/* Free task info now for UNDO and external DnD's or when moving to
	 * the WB and its clean-up method is immediate or if no (sub)tasks
	 * were actually executed.
	 */
	if ((opr_info->options & (EXTERN_DND | UNDO)) ||
	    ((opr_info->type == DM_MOVE) && DM_WBIsImmediate(Desktop) &&
	     IS_WB_PATH(Desktop, opr_info->target_path)) ||
	    ((tmp_win != DESKTOP_WB_WIN(Desktop)) &&
	     (opr_info->type == DM_DELETE) && DM_WBIsImmediate(Desktop)) ||
	    (opr_info->task_cnt == 0))

	{
	    if (tmp_win)
		tmp_win->task_id = 0;
	    DmFreeTaskInfo(tip);
	}
	break;

    case DM_REPORT_PROGRESS:
	/* report progress info. about the operation.
	 * Note: using the REPORT_PROGRESS option assumes src_win != NULL
	 */
	tmp_win = (opr_info->options & UNDO) ? dst_win : src_win;
	if (tmp_win)
	{
#define MSG_(type_, msg_)	(opr_info->type == type_) ? msg_
	    DmVaDisplayStatus((DmWinPtr)tmp_win, False,
			      MSG_(DM_DELETE,	TXT_DELETE_IN_PROGRESS) :
			      MSG_(DM_COPY,	TXT_COPY_IN_PROGRESS) :
			      MSG_(DM_HARDLINK,	TXT_LINK_IN_PROGRESS) :
			      MSG_(DM_SYMLINK,	TXT_LINK_IN_PROGRESS) :
			      MSG_(DM_MKDIR,	TXT_MKDIR_IN_PROGRESS) :
			      MSG_(DM_MOVE,	TXT_MOVE_IN_PROGRESS) :
			      /* else */	NULL,
			      basename(src_file));
#undef MSG_
	}
	break;

    case DM_OVRWRITE:
	/* An overwrite condition occurred.
	 * Note: using the OVERWRITE option assumes src_win != NULL
	 */
	PromptOverwrite(src_win, dst_file);
	break;

    case DM_NMCHG: {
	int         src_cnt = opr_info->src_cnt;
	DtAttrs *   src_info = opr_info->src_info;
	DtAttrs *   info;
	char *      msg;

	msg = TXT_NMCHG_NOTICE_0;
	for(info=src_info; info<src_info+src_cnt; info++) {
		if (*info == DM_IS_DIR) {
			if (opr_info->type == DM_MOVE) {
				msg = TXT_NMCHG_NOTICE_1;
			}
			else {
				msg = TXT_NMCHG_NOTICE_2;
			}
			break;
		}
	}
	/* An name change condition occurred.  */
	PromptNameChange(msg, src_win, dst_file);
	break;
    }

    case DM_ERROR:
	/* Error occurred during file op: display approp message.
	 * Note: DM_ERROR is always called (no option is needed) so must be
	 * careful to test for NULL src_win & dst_win.
	 */
	if (opr_info->error)
	{
	    tmp_win = (opr_info->options & UNDO) ? dst_win : src_win;
	    if (tmp_win)
		DmVaDisplayStatus((DmWinPtr)tmp_win, True,
				  StrError(opr_info->error), src_file,
				  IS_WB_PATH(Desktop, opr_info->target_path) ?
				  GetGizmoText(TXT_WB_TITLE) : dst_file);
	}
	break;

    default: 
	break;
    }
}				/* end of DmFolderFMProc */

/****************************procedure*header*****************************
    DmFolderPathChanged- called when a window's path is changed or removed:
	Update visited folders array (if path is found there).
	Update path of "root" folder (if open) and any descendants.
*/
void
DmFolderPathChanged(char * old_path, char * new_path)
{
    int				old_len;
    DmContainerBuf		containers;
    DmContainerPtr		cp;
    int				i;

#ifdef DEBUG
    fprintf(stderr,"DmFolderPathChanged: %s --> %s\n",
	    old_path ? old_path : "NULL", new_path ? new_path : "NULL");
#endif

    Dm__UpdateVisitedFolders(old_path, new_path);
    Dm__UpdateTreeView(old_path, new_path);
	
    old_len = strlen(old_path);		/* do it once outside of loop */
    
    /*  We want the container list to stay fixed while we notify
     *  dependents of container destruction.  We'll clean up the
     *  list after notifying dependents.
     */
    DmLockContainerList(True);
    containers = DESKTOP_CONTAINERS(Desktop);

    for (i=0; i<containers.used; i++){
	int status;
	
	cp = containers.p[i];
	status = DmSameOrDescendant(old_path, cp->path, old_len);

	if (status != 0){
	    char *dir_path;

	    /* container matches path or is a descendant */
	    if (new_path == NULL){
		/* ancestor or self deleted, notify dependents */
		DmDestroyContainer(cp);
		continue;
	    }
	    if (status < 0)
		dir_path = strdup(new_path); 	/* exact match */
	    else		
		/* descendant, replace old_path in descendant name */
		dir_path = strdup(DmMakePath(new_path, cp->path + old_len + 1));
	    DmMoveContainer(cp, dir_path);
	    FREE(dir_path);
	}
    }
    /* clean up the container list and closed destroyed containers */
    DmLockContainerList(False);
}					/* end of DmFolderPathChanged */


/****************************procedure*header*****************************
    DmRmObjectFromContainer - Controller routine to remove an object
    from a container.   The container will take care of notifying
    its dependents (folder windows) of the change.  We must explicitly
    notify the Visited Folders Menu, Folder Map, and dependents of
    all descendant containers because they are not registered as 
    dependents on the container.
*/
void
DmRmObjectFromContainer(DmContainerPtr cp, DmObjectPtr op)
{
#ifdef DEBUG
    fprintf(stderr, "DmRmObjectFromContainer: %s %s\n", cp->path, op->name);
#endif

    DmDelObjectFromContainer(cp, op);

    if (OBJ_IS_DIR(op))
    {
	/* Notify Folders Menu, Folder Map, and all descendants */
	char buf[PATH_MAX];

	DmFolderPathChanged(Dm_ObjPath(op,buf), NULL);
    }
} /* end of DmRmObjectFromContainer */

/****************************procedure*header*****************************
    DmUpdateWindow()- core function to update the containers after a File
	manipulation operation is done. 'options' allows caller to indicate
	what kind of updates they are interested in. e.g. a delete operation
	will only be interested in updating src. window, a copy operation
	will only be interested in updating a destination window and move
	opr. will be interested in both. Also, after the files (items) are
	moved we need to keep objectlist in sync., unmange item from the
	itemlist etc. For new files that gets added via operation, we may
	need to get new object structure or in some case (e.g. move), we may
	be able to use the old one (i.e. src obejct).
	
	Besides updating the source and destination *containers*, we need
	to make sure that the position of any new files in the destination
	*folder* are set correctly if specified as part of the operation
	(i.e. DnD).
	
				DM_UPDATE_SRCWIN
				True	False
	DM_UPDATE_DSTWIN
			True	move	copy/create
			False	delete	no-op
*/
void
DmUpdateWindow(DmFileOpInfoPtr opr_info, DtAttrs options)
{
    char **		src;
    DtAttrs *		src_info;
    Cardinal		prev_indx;
    Boolean		use_dst_win;

    if (opr_info->cur_src == 0)		/* There is nothing to do */
	return;


    /* Do some work outside of the loop below for the dst_win: Can only 
     * use dst_win if it's defined and NOT undo'ing multi-path op.  
     */
    if ((options & DM_UPDATE_DSTWIN) && (opr_info->dst_win != NULL) &&
	((opr_info->options &
	  (MULTI_PATH_SRCS | UNDO)) != (MULTI_PATH_SRCS | UNDO))) {
	use_dst_win = True;
	prev_indx = ExmNO_ITEM;
    } else
	use_dst_win = False;

    for (src = opr_info->src_list, src_info = opr_info->src_info;
	 src < opr_info->src_list + opr_info->cur_src; src++, src_info++) {
	char 		*src_bname;
	char		*src_name;
	char		*dos_name;
	char 		*dst_name;
	char 		dosnm[DOS_MAXPATH];
	DmItemPtr	src_item = NULL;
	DmObjectPtr     src_obj = NULL;
	DmItemPtr	dst_item  = NULL;
	DmObjectPtr	dst_obj = NULL;
	extern		int legalfmove(char *);

	if (*src_info & SRC_B_IGNORE)
	    continue;

	src_bname = basename(*src);
	src_name = *src;

	if (*src_info & SRC_B_NMCHG)
	{
	    unix2dosfn(src_bname, dosnm, legalfmove, strlen(src_bname));
	    if (opr_info->options & UNDO)
	    {
		src_bname = dosnm;
		src_name = dosnm;
	    }
	}

	/*	Update src_win (DELETE or MOVE)		*/

	if (options & DM_UPDATE_SRCWIN) {
	    Boolean undo_multi =
		((opr_info->options &
		  (UNDO | MULTI_PATH_SRCS)) == (UNDO | MULTI_PATH_SRCS));

	    /* 
	     * Deleteing or moving a dir can affect the visited folders array
	     * and other folder descendants.  For example, if /foo/bar is
	     * moved to /tmp/bar, the paths of the folder displaying /foo/bar
	     * (if any) and its descendants (if any) must be updated.  If
	     * /foo/bar is deleted, those folder must be closed.  This is
	     * only necessary if the src item is a directory.
	     *
	     * (src_win can be NULL for an UNDO (where the dtm folder window
	     * may not be open) or if a client asks the wastebasket to move a
	     * file to the wastebasket (there is no dtm window)).
	     */
	    if ((*src_info & SRC_TYPE_MASK) == DM_IS_DIR) {
		char *old_path;
		char *new_path;

		old_path = strdup(DmMakePath(opr_info->src_path, undo_multi? 
				  src_bname : src_name));

		/* 
		 * If this is a DELETE (no dst_win or dst_win is wastebasket),
		 * don't supply a new path since there is none.  Otherwise,
		 * construct a new path depending on whether the target was
		 * created by this operation or not.
		 */
		if (!(options & DM_UPDATE_DSTWIN) || (opr_info->dst_win &&
		    IS_WB_WIN(Desktop, opr_info->dst_win))) {
		    new_path = NULL;
		} else if (opr_info->attrs & DM_B_ADJUST_TARGET) {
		    char *tmp_bname = src_bname;
		    char *tmp_name = *src;
		    int free = 0;
		    if (*src_info & SRC_B_NMCHG) {  
			if (opr_info->options & UNDO) {
				char *tmp;
				tmp_bname  = basename(*src); 
				tmp = strdup(*src); 
				tmp_name = strdup(DmMakePath(dirname(tmp), 
						tmp_bname));
				XtFree(tmp);
				free = 1;
			} else 
				tmp_bname = dosnm;
		    }
		    new_path = strdup(DmMakePath(opr_info->target_path,
					  undo_multi ? tmp_name : tmp_bname));
		    if (free)
			XtFree(tmp_name);
		
		} else
		    new_path = strdup(opr_info->target_path);

		DmFolderPathChanged(old_path, new_path);

		XtFree(old_path);
		XtFree(new_path);
	    }

	    /* 
	     * Set 'item' so it can be used during dst_win update below.
	     * 'item' should not be NULL except that the sync timer may have
	     * already removed the item (ie, while an overwrite notice is up).
	     * Note: 'item' != NULL means src_win != NULL.
	     */
	    
	    if (opr_info->src_win != NULL) {
		src_item = DmObjNameToItem((DmWinPtr)opr_info->src_win,
				       undo_multi ? src_bname : src_name);
		if (src_item != NULL)
			src_obj = ITEM_OBJ(src_item);
	    }

	    /* 
	     * Only remove the item if this is a delete or it's a move and
	     * the src and dst windows are different.  Moving within the
	     * same window (a rename) is handled while updating dst_win.
	     * Note: DmFolderPathChanged (above) updates tree view.
	     */
	    if (src_obj && !IS_TREE_WIN(Desktop, opr_info->src_win) &&
		(!(options & DM_UPDATE_DSTWIN) ||
		 (opr_info->src_win != opr_info->dst_win)))
		DmDelObjectFromContainer(opr_info->src_win->views[0].cp, 
					 src_obj);
	    /* Don't FREE object yet.  It may be reused to update dst_win */

	    /* Even if src_win is NULL, if the src_list contains
	     * "multi-paths", there may be a folder opened which contains the
	     * item.  opr->src_win (dst_win on UNDO) is probably the found 
	     * window or the tree view.  
	     * Note: UNDO becomes multi-path "target".
	     */
	    if (opr_info->options & MULTI_PATH_SRCS) {	
		DmContainerPtr parent;
		char *parent_path;
		
		if (opr_info->attrs & (DM_B_DIR_EXISTS | DM_B_ADJUST_TARGET)){
		    /*
		     * multiple items in operation. 
		     */
		    parent_path = DmMakeDir((opr_info->options & UNDO) ?
					    opr_info->target_path :
					    opr_info->src_path, src_name);
		}
		else{
		    /* 
		     * Single item operation 
		     */
		    parent_path = ((opr_info->options & UNDO) ?
				   dirname(opr_info->target_path) :
				   DmMakeDir(opr_info->src_path, src_name));
		}
		parent = DmQueryContainer(parent_path);
		if (parent)
		    /* MORE: should we sync it if parent == src_win ?? */
		    Dm__SyncContainer(parent, False);
	    }
	}

	/* 	Update dst_win (MOVE, COPY, CREATE or undo MOVE)  
	 *
	 * See if item already exists in dst_win.  This can occur when an
	 * item is overwritten or the sync timer may have already added the
	 * item while the overwrite notice is up (for a later item).
	 *
	 * if UPDATE_SRCWIN, then opr is a MOVE so make use of the object
	 * from src_win (can't re-use objs from tree view, though). Otherwise
	 * (for COPY, etc), just add a new item to dst_win.  *
	 *
	 * Note that (for MOVE) 'src_obj' may have already be removed from
	 * src_win as by the sync timer example mentioned above.  In all
	 * cases, items in the dst_win must be cascaded.
	 *
	 * For MOVE, if 'src_obj' != NULL, re-use the obj (but only if it's the
	 * same file-type): change its name and (must) re-class it.  Finally,
	 * add the obj to the dst_win if it is different from src_win.
	 * Otherwise (a rename), just update the label.
	 */
	if (!use_dst_win)
	   continue;

	if (*src_info & SRC_B_NMCHG)
	{
	    if (opr_info->options & UNDO)
		src_bname  = basename(*src); 
	    else
		src_bname = dosnm;
	}
	dst_name = (opr_info->attrs & DM_B_ADJUST_TARGET) ?
			src_bname : basename(opr_info->target_path);
	dst_item = DmObjNameToItem((DmWinPtr)opr_info->dst_win, dst_name);

	/* If item already in dst_win, use it instead of item found in
	 * src_win (if any) as long as it's the same type.
	 */
	if (dst_item != NULL) {
	    dst_obj = ITEM_OBJ(dst_item);
	    FREE(dst_obj->objectdata); 
	    dst_obj->attrs = 0;
	    DmInitObj(dst_obj, NULL, DM_B_INIT_FILEINFO);

	    DmRetypeObj(dst_obj, True);

	    /* Use dst_item (just re-position it if necessary) */
	    PositionItem(opr_info, &prev_indx, dst_item);	


	    /* Now deal with the un-needed src-item:
	     * If "rename", item hasn't been removed from container yet
	     */
	    if (src_item && !IS_TREE_WIN(Desktop, opr_info->src_win))
	    {
		if (opr_info->src_win == opr_info->dst_win)
		    DmDelObjectFromContainer(opr_info->src_win->views[0].cp,
					     src_obj);	
		Dm__FreeObject(src_obj);
	    }
	    continue;
	}

	if ((options & DM_UPDATE_SRCWIN) && (src_item != NULL) && /* MOVE */
	    !IS_TREE_WIN(Desktop, opr_info->src_win))
	{
	    if (opr_info->src_win == opr_info->dst_win)	/* ie. RENAME */
	    {
		DmRenameObject(src_obj, dst_name);

	    } else				/* MOVE but not RENAME */
	    {
		DtAttrs attrs =
		    (opr_info->dst_win->views[0].cp->fstype ==
		     DOSFS_ID(Desktop)) ?
			 (DM_B_INIT_FILEINFO | DM_B_SET_TYPE) :
			     IS_WB_PATH(Desktop, opr_info->src_path) ? 
				 DM_B_SET_TYPE : 0;

		/* If this is a move from WB to folder, type must
		 * be set, because WB has special file type.
		 * 
		 * DmAddObjectToContainer will check if this is a directory
		 * and update tree view (old code did not do this.)
		 */
		FREE(src_obj->name);
		src_obj->name = strdup(dst_name);

		/* src_obj may change if DmAddObjToContainer finds a newborn
		 * object
		 */
		dst_obj = 
		    DmAddObjectToContainer(opr_info->dst_win->views[0].cp,
					   src_obj, dst_name, attrs);
		if (dst_obj != src_obj)
		    Dm__FreeObject(src_obj);
	    }
	} else
	{					/* COPY, CREATE or Undo MOVE */
	    dst_obj =
		DmAddObjectToContainer(opr_info->dst_win->views[0].cp,
				       NULL, dst_name,
				       DM_B_INIT_FILEINFO | DM_B_SET_TYPE);
	}

	dst_item = DmObjectToItem((DmWinPtr)opr_info->dst_win, dst_obj);

	/* Position this item and cascade next items in dst window */
	PositionItem(opr_info, &prev_indx, dst_item);

    }					/* for all src_list items */

#ifdef NOT_USE
    /* Time stamp src and dst windows.  Only time-stamp src_win 
       if not already stamped (if src_list does not have "multi-paths").
    */

    if ((options & DM_UPDATE_SRCWIN) &&	(opr_info->src_win != NULL)) {
	if ((opr_info->src_win->attrs & DM_B_FOLDER_WIN) &&
	    !(opr_info->options & MULTI_PATH_SRCS))
	    Dm__StampContainer(opr_info->src_win->views[0].cp);
    }
    if ((use_dst_win) && !((options & DM_UPDATE_SRCWIN) &&
			   (opr_info->dst_win == opr_info->src_win))) {
	if (opr_info->dst_win->attrs & DM_B_FOLDER_WIN)
	    Dm__StampContainer(opr_info->dst_win->views[0].cp);
    }
#endif
}				/* end of DmUpdateWindow() */
