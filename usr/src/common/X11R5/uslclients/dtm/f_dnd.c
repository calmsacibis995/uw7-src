/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_dnd.c	1.30"

/******************************file*header********************************

    Description:
	This file contains the source code for handling external and
	internal drag-an-drop transactions where dtm is either the
	source or destination.
*/
						/* #includes go here	*/

#include <libgen.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Xm/DragDrop.h>
#include <Dt/DesktopI.h>

#include "DnDUtilP.h" /* in ~uslclients/libMDtI */

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static char *ExpandSrcList(void **list, Boolean fullpath, Boolean quoted,
	int flag);
static void **GetDragItemList(DmWinPtr window, int item_index);

static void GotSrcFiles(Widget w, XtPointer client_data,
	XtPointer call_data);

static char **GetNumFiles(char **list, int *src_cnt);


					/* public procedures		*/

void DmFolderTriggerNotify(Widget w, XtPointer client_data,
	XtPointer call_data);
void DmDropProc(Widget w, XtPointer client_data, XtPointer call_data);

/****************************procedure*header*****************************
 * GotSrcFiles - Called to process information received from the DnD source.
 * Depending on where the drop occurred, this may result in a file operation,
 * invoking the DROP command of an object, or a no-op.
 */
static void
GotSrcFiles(Widget w, XtPointer client_data, /* selection */
	XtPointer call_data /* dip */)
{
	DmWinPtr dst_win;
	WidePosition x, y;
	Window dummy_win;
	Cardinal dst_idx;
	char *target;
	DmFileOpInfoPtr opr_info;
	_DmDnDDstInfoPtr dip = (_DmDnDDstInfoPtr)call_data; /* dip */
	DmItemPtr itp = (DmItemPtr)dip->items;

    if (dip->error || (dip->nitems == 0)) {
	return;
    }
    dst_win = (DmWinPtr)dip->client_data;

    if (IS_WB_WIN(Desktop, (DmFolderWindow)dst_win))
    {
	if (dip->operation != XmDROP_MOVE)
	{
	    DmVaDisplayStatus(dst_win, True, (dip->operation == XmDROP_LINK) ?
		TXT_CANT_WB_LINK : TXT_CANT_WB_COPY);
	    return;
	}
	target = strdup(DM_WB_PATH(Desktop));
	goto drop;
    }
    dst_idx = dip->item_index;

    if (dst_idx == ExmNO_ITEM)		/* dropping in an empty space? */
    {
	/* Dropping onto background of Found Window or Folder Map has no
	 * meaning.
	 */
	if (dst_win->attrs & (DM_B_TREE_WIN & DM_B_FOUND_WIN))
	{
	    DmVaDisplayStatus(dst_win, True, IS_TREE_WIN(Desktop,
		(DmFolderWindow)dst_win) ? TXT_CANT_TREE_DROP :
		TXT_CANT_FOUND_DROP);
	    return;
	}
	target = strdup(dst_win->views[0].cp->path);
	x = dip->x;
	y = dip->y;
    } else				/* dropping onto an icon */
    {
	char *drop_cmd;
	DmObjectPtr dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_idx));

	/* If there is a DROPCMD property, use it and return */
	if ((drop_cmd = DmGetObjProperty(dst_op, DROPCMD, NULL)) != NULL)
	{
	    if (strstr(drop_cmd, "s*") || strstr(drop_cmd, "S*")) {
		    DmDropObject(dst_win, dst_idx, NULL, NULL,
			(void **)dip->files, ExpandSrcList,
			MULTI_PATH_SRCS | FILE_NAMES);
	    } else {
	        char ** p;
	        for (p = dip->files; *p != NULL; p++)
		    DmDropObject(dst_win, dst_idx, NULL, (void *)*p, NULL,
			NULL, 0);
	    }
	    return;

	} else if (!OBJ_IS_DIR(dst_op)) {
	    return;		/* dropping onto non-dir w/no property */
	}
	target = strdup(DmObjPath(dst_op));

	/* Not dropping on bg so can't use dst_win */
	dst_win = NULL;

	/* Make position unspecified so item can be correctly positioned
	   in the dest folder.
	*/
	x = UNSPECIFIED_POS;
	y = UNSPECIFIED_POS;
    }

    /* Falling thru to here means the item(s) will be mv/cp/ln'ed */
 drop:
    dip->attrs |= DT_B_STATIC_LIST;	/* keep src_list around */

    opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
    opr_info->target_path	= target;	/* already dup'ed above */
    opr_info->src_path		= NULL;		/* src's have full paths */
    opr_info->src_list		= dip->files;
    opr_info->src_cnt		= dip->nitems;
    opr_info->src_win		= NULL;
    opr_info->dst_win		= (DmFolderWindow)dst_win;
    opr_info->x			= x;
    opr_info->y			= y;

    /* Establish file operation type based on drop type */
    opr_info->type = (dip->operation == XmDROP_MOVE) ? DM_MOVE :
	(dip->operation == XmDROP_LINK)	? DM_SYMLINK :
	    /* XmDROP_COPY */ DM_COPY;

    /* Note that src list may have multi-paths. */
    opr_info->options = MULTI_PATH_SRCS | EXTERN_DND;

    /* Return now without freeing client_data yet if file op accepted and
       caller needs notification when done.  Task info is freed then, too.
    */
#ifdef NOT_USE
    if ((DmDoFileOp(opr_info, DmFolderFMProc, (XtPointer)NULL) != NULL) &&
	(opr_info->options & EXTERN_SEND_DONE))
#endif
    if (DmDoFileOp(opr_info, DmFolderFMProc, (XtPointer)NULL) != NULL)
	return;

}					/* end of GotSrcFiles */

/****************************procedure*header*****************************
 * DmFolderTriggerNotify - Called when an external client drops onto dtm.
 * Sets up information that will be needed later as client data for
 * transferProc.
 */
void
DmFolderTriggerNotify(Widget w, XtPointer client_data, XtPointer call_data)
{
	int i, j;
	_DmDnDDstInfoPtr dip;
	XmDropTransferEntryRec transferEntries[1];
	Cardinal numTransfers = 1;
	Boolean found = False;
	ExmFIconBoxTriggerMsgCD *cd = (ExmFIconBoxTriggerMsgCD *)call_data;

	if (dip = (_DmDnDDstInfoPtr)malloc(sizeof(_DmDnDDstInfo))) {
		dip->widget      = w; /* needed in transferProc later */
		dip->timestamp   = 0; /* is this needed? */
		dip->proc        = GotSrcFiles; /* called from transferProc */
		dip->error       = False; /* set to True if error later */
		dip->client_data = client_data; /* dst win */
		dip->attrs       = 0;
		dip->targets     = NULL; /* will be set in GetNumItems() */
		dip->cd_list     = NULL; /* will be set in GetNumItems() */
		dip->operation   = cd->operation;
		dip->items       = (XtPointer)(cd->item_data.items);
		dip->item_index  = cd->item_data.item_index;
		dip->x           = cd->x;
		dip->y           = cd->y;
	} else {
		return;
	}
	for (i = 0; i < cd->num_import_targets; i++) {
		for (j = 0; j < cd->num_export_targets; j++) {
			if (cd->import_targets[i] == cd->export_targets[j]) {
				found = True;
				transferEntries[0].target =
					cd->import_targets[i];
				break;
			}
		}
		if (found)
			break;
	}
	i = 0;
	if (!found) {
		/* this is not likely to happen because Xm should have
		 * made sure there is at least one match in the targets.
		 */
		free(dip);
		XtSetArg(Dm__arg[i], XmNtransferStatus,XmTRANSFER_FAILURE);i++;
		XtSetArg(Dm__arg[i], XmNnumDropTransfers, 0); i++;
	} else {
		transferEntries[0].client_data = (XtPointer)dip;

#ifdef NOT_USE
		/* Don't handle XA_DELETE for now.  Need to find out when
		 * this should be done.
		 */
		if (cd->operation == XmDROP_MOVE) {
                	transferEntries[1].client_data = (XtPointer)dip;
                	transferEntries[1].target =
				OL_XA_DELETE(DESKTOP_DISPLAY(Desktop));
                	++numTransfers;
		}
#endif
		XtSetArg(Dm__arg[i], XmNdropTransfers, transferEntries); i++;
		XtSetArg(Dm__arg[i], XmNnumDropTransfers, numTransfers); i++;
		if (transferEntries[0].target==OL_USL_NUM_ITEMS(XtDisplay(w)))
		{
			XtSetArg(Dm__arg[i], XmNtransferProc,DmDnDGetNumItems);
				i++;
		} else {
			XtSetArg(Dm__arg[i], XmNtransferProc,DmDnDGetOneName);
				i++;
		}
	}
	XmDropTransferStart(cd->drag_context, Dm__arg, i);

} /* end of DmFolderTriggerNotify */

/****************************procedure*header*****************************
 * DmDropProc - Called when a drop is initiated from dtm.
 * The destination can be dtm itself (ExmINTERNAL_DROP) or an external
 * client (ExmEXTERNAL_DROP).
 *
 * In the case of ExmINTERNAL_DROP, if the drop is on an icon that has
 * a DROP command, that command is executed; otherwise, the drop is
 * handled as a file operation (move/copy/link).  The destination's
 * dropProc is called in this case.  Note that the flat icon box widget
 * will set dropSiteStatus to XmDROP_NOOP and call XmDropTransferStart().
 *
 * In the case of ExmEXTERNAL_DROP, information pertaining to the
 * drop is cached and is used later in dtm's convertProc.
 */
void
DmDropProc(Widget w, XtPointer client_data, XtPointer call_data)
{
    void **list;
    DmFolderWinPtr src_win;
    ExmFlatDropCallData *d = (ExmFlatDropCallData *)call_data;

    if (d->reason == ExmINTERNAL_DROP)
    {
	DmFolderWinPtr dst_win = (DmFolderWinPtr)client_data;
	DmFileOpInfoPtr opr_info;
	DmObjectPtr src_op;
	Cardinal dst_idx;
	char *target;
	int status;

	XtSetArg(Dm__arg[0], XmNclientData, &src_win);
	XtGetValues(d->source, Dm__arg, 1);

	/* Make this simple check up front: can't drop on a busy item */
	if (d->item_data.item_index != ExmNO_ITEM &&
	    !ITEM_SENSITIVE(DM_WIN_ITEM(dst_win, d->item_data.item_index)))
	{
	    return;
	}

	/* get the list of items to be operated on */
	if ((list = GetDragItemList((DmWinPtr)src_win, d->src_item_index))
	    == NULL)
	{
	    return;
	}

	/* Process a drop on the WasteBasket */
	if (IS_WB_WIN(Desktop, dst_win))
	{
	    if (d->operation == XmDROP_MOVE)
		DeleteItems(src_win, (DmItemPtr *)list, d->x, d->y);

	    else
		DmVaDisplayStatus((DmWinPtr)src_win, True,
			((d->operation == XmDROP_LINK) ?
			TXT_CANT_WB_LINK : TXT_CANT_WB_COPY));
	    return;
	}

	/* Process a drop on a FolderWindow */
	dst_idx = d->item_data.item_index;

	/* Check to see if drop is in same window, and whether the drop is
	 * onto the icon itself or on empty space, as well as which window
	 * we are dealing with.
	 * 
	 * This is a new condition that must be handled because FIconBox can
	 * not reliably provide this convenience.  (FIconBox does not know
	 * the design of an item (ie, glyph + label)).
	 */
	if (dst_idx == ExmNO_ITEM) /* dropped on empty space/on icon itself */
	{
		if (dst_win->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
		{
			DmVaDisplayStatus((DmWinPtr)dst_win, True,
				(dst_win->attrs & DM_B_TREE_WIN) ?
				TXT_CANT_TREE_DROP : TXT_CANT_FOUND_DROP);
			return;
		} else if (dst_win == src_win)
		{
			/* Dropped within the same window.  Reposition if
			 * not in short or long view.
			 */
			if (dst_win->views[0].view_type != DM_NAME &&
				dst_win->views[0].view_type != DM_LONG)
			{
				/* Reposition the icon. */
				XtSetArg(Dm__arg[0], XtNx, d->x -
					(ITEM_WIDTH(DM_WIN_ITEM(src_win,
					d->src_item_index)) / 2));
				XtSetArg(Dm__arg[1], XtNy, d->y -
					(GLYPH_PTR(DM_WIN_ITEM(src_win,
					d->src_item_index))->height / 2));
				ExmFlatSetValues(src_win->views[0].box,
					d->src_item_index, Dm__arg, 2);
			}
			return;
		}
		/* set up file operation */
		target = strdup(dst_win->views[0].cp->path);
	} else					/* Dropped onto another icon */
	{
	    char *dropcmd;
	    DmObjectPtr dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_idx));

	    /* If there is a DROPCMD property, use it and return */
	    if ((dropcmd = DmGetObjProperty(dst_op, DROPCMD, NULL)) != NULL)
	    {
		if (strcmp(dropcmd, "##DELETE()") == 0)
		    DeleteItems(src_win, (DmItemPtr *)list,
				UNSPECIFIED_POS, UNSPECIFIED_POS);
		else if (strstr(dropcmd, "s*") || strstr(dropcmd, "S*"))
		    DmDropObject((DmWinPtr)dst_win, dst_idx,
				 (DmWinPtr)src_win, NULL, (void **)list,
				 ExpandSrcList, MULTI_PATH_SRCS);
		else
		    while (*list != NULL)
			DmDropObject((DmWinPtr)dst_win, dst_idx,
				     (DmWinPtr)src_win,
				     ITEM_OBJ((DmItemPtr)(*list++)),
				     NULL, NULL, 0);
		return;
	    } else if (!OBJ_IS_DIR(dst_op))
	    {
		/* dropping onto non-dir w/no DROP command */
		DmVaDisplayStatus((DmWinPtr)src_win, True,
				  TXT_NO_DROP_ACTION, NULL);
		return;
	    }
	    /* Getting here means dropping onto folder icon */
	    target = strdup(DmObjPath(dst_op));

	    /* Not dropping on bg so can't use dst_win or x & y*/
	    dst_win = NULL;
	    d->x = UNSPECIFIED_POS;
	    d->y = UNSPECIFIED_POS;
	}
	/* Falling thru to here means the item will be
	 * mv/cp/ln'ed
	 */
drop:
	opr_info = (DmFileOpInfoPtr)CALLOC(1, sizeof(DmFileOpInfoRec));

	/* check for "special" source windows: Wastebasket,
	 * Folder Map, Found Window.
	 */
	if (IS_WB_WIN(Desktop, src_win))
	{
	    dst_win = (DmFolderWinPtr)client_data;
	    DmWBPutBackByDnD(dst_win, d, list);
	    FREE((void *)list);
	    return;
	} else if (IS_TREE_WIN(Desktop, src_win) ||
		   IS_FOUND_WIN(Desktop, src_win))
	{
	    /* Source window is Folder Map or Found
	     * Window), tag the list of srcs as special.
	     */
	    opr_info->options |= MULTI_PATH_SRCS;
	}
	/*check for "special" destination window:Wastebasket*/
	if (dst_win == DESKTOP_WB_WIN(Desktop))
	{
	    if (DM_WBIsImmediate(Desktop))
	    {
		/* Destination is Wastebasket and Immediate
		 * Delete mode is set, make this a DM_DELETE
		 * operation.
		 */ 
		opr_info->target_path = NULL;
		opr_info->dst_win     = NULL;
		opr_info->type        = DM_DELETE;
	    } else {
		opr_info->target_path = target;
		opr_info->dst_win     = dst_win;
		opr_info->type        = DM_MOVE;
	    }
	    if (GetWMState(XtDisplay(DESKTOP_WB_WIN(Desktop)->shell),
		   XtWindow(DESKTOP_WB_WIN(Desktop)->shell)) == IconicState)
	    {
		/* If Wastebasket is iconized, set opr_info->x
		 * and opr_info->y to UNSPECIFIED_POS so that
		 * icons won't be stacked upon each other.
		 */
		opr_info->x = opr_info->y = UNSPECIFIED_POS;
	    } else {
		opr_info->x = d->x;
		opr_info->y = d->y;
	    }
	    opr_info->options |= REPORT_PROGRESS | OPRBEGIN;
	} else {
	    opr_info->x           = d->x;
	    opr_info->y           = d->y;
	    opr_info->target_path = target;
	    opr_info->dst_win     = dst_win;
	    opr_info->type        = d->operation == XmDROP_MOVE ?
		DM_MOVE : d->operation == XmDROP_LINK ?
		    DM_SYMLINK :	/* OL_DUPLICATE */ DM_COPY;
	    opr_info->options |= REPORT_PROGRESS|OVERWRITE|OPRBEGIN;
	}
	opr_info->src_list = DmItemListToSrcList(list, &(opr_info->src_cnt));
	FREE((void *)list);
	if (opr_info->src_list == NULL) {
	    FREE(target);
	    FREE(opr_info);
	    return;
	}
	opr_info->src_win  = src_win;
	opr_info->src_path =strdup(src_win->views[0].cp->path);
	DmFreeTaskInfo(src_win->task_id);
	src_win->task_id = DmDoFileOp(opr_info, DmFolderFMProc, NULL);

    } else
    { 
	/* cd->reason == ExmEXTERNAL_DROP */
	src_win = (DmFolderWinPtr)client_data;

	/* get the list of items to be operated on */
	if ((list=GetDragItemList((DmWinPtr)src_win,d->src_item_index))==NULL)
		return;

	(void)DtmDnDNewTransaction((DmWinPtr)src_win,
		(DmItemPtr *)list,
		0, /* attrs */
		0, /* dst window id */
		d->selection,
		d->operation);
    }

} /* end of DmDropProc */

/****************************procedure*header*****************************
 * GetNumFiles - Returns a copy of list and number of files.
 */
static char **
GetNumFiles(char **list, int *src_cnt)
{
	char **p;
	char **src_list = NULL;

	for (*src_cnt=0, p=list; *p; p++,(*src_cnt)++) ;

	if (src_list = (char **)MALLOC(sizeof(char *) * *src_cnt)) {
		char **sp;

		for (sp=src_list,p=list; *p; p++)
			*sp++ = STRDUP(*p);
	}
	return(src_list);

} /* end of GetNumFiles */

/****************************procedure*header*****************************
 * ExpandSrcList - Converts a list of object ptrs into a list of file names.
 * Value returned is freed in the calling function Dm__expand_sh().
 */
static char *
ExpandSrcList(void **list, Boolean fullpath, Boolean quoted, int flag)
{
	int i;
	int len = 0;
	int src_cnt;
	char **src_list;
	char *src_files;
	char src_path[PATH_MAX];

	if (flag & FILE_NAMES)
		src_list = GetNumFiles((char **)list, &(src_cnt));
	else
		src_list = DmItemListToSrcList(list, &(src_cnt));

	if (src_list == NULL || src_cnt == 0)
		return(NULL);

	for (i = 0; i < src_cnt; ++i)
		len += strlen(src_list[i]);

	/* account for quotes, spaces and NULL terminator */
	if (quoted)
		len += (src_cnt == 1) ? 3 : (src_cnt * 3);
	else
		len += (src_cnt == 1) ? 1 : (src_cnt - 1);

	if (fullpath) {
		sprintf(src_path, "%s/", DmGetSrcWinPath());
		len += strlen(src_path) * src_cnt;
	}

	if ((src_files = (char *)CALLOC(len, sizeof(char))) == NULL)
		return(NULL);

	if (quoted) {
		if (src_cnt == 1) {
			if (fullpath)
				sprintf(Dm__buffer, "\"%s%s\"", src_path,
					src_list[0]);
			else
				sprintf(Dm__buffer, "\"%s\"", src_list[0]);
			strcpy(src_files, Dm__buffer);
			return(src_files);
		}
		strcpy(src_files, "\"");
		if (fullpath)
			strcat(src_files, src_path);
		strcat(src_files, src_list[0]);
		strcat(src_files, "\"");
		strcat(src_files, " ");

		for (i = 1; i < src_cnt-1; ++i) {
			strcat(src_files, "\"");
			if (fullpath)
				strcat(src_files, src_path);
			strcat(src_files, src_list[i]);
			strcat(src_files, "\"");
			strcat(src_files, " ");
		}
		strcat(src_files, "\"");
		if (fullpath)
			strcat(src_files, src_path);
		strcat(src_files, src_list[src_cnt-1]);
		strcat(src_files, "\"");
	} else {
		if (src_cnt == 1) {
			if (fullpath) {
				sprintf(Dm__buffer, "%s%s", src_path,
					src_list[0]);
				strcpy(src_files, Dm__buffer);
			} else
				strcpy(src_files, src_list[0]);
			return(src_files);
		}
		if (fullpath) {
			strcpy(src_files, src_path);
			strcat(src_files, src_list[0]);
		} else {
			strcpy(src_files, src_list[0]);
		}
		strcat(src_files, " ");
		for (i = 1; i < src_cnt-1; ++i) {
			if (fullpath)
				strcat(src_files, src_path);
			strcat(src_files, src_list[i]);
			strcat(src_files, " ");
		}
		if (fullpath)
			strcat(src_files, src_path);
		strcat(src_files, src_list[src_cnt-1]);
	}
	return(src_files);

} /* end of ExpandSrcList */

/***************************************************************************
 * GetDragItemList: construct the list of items being dragged
 *	INPUT: folder window (source)
 *	       item_index (from XmNdropProc)
 *	OUTPUT: list of items for DnD operation
 *	
 *	The user can initiate DnD in two ways: 1) drag and drop a single
 *	item 2) select multiple items (via bounding box or ADJUST button)
 *	and then drag and drop one of the items.  In case 1), the
 *	original selection is unaffected by the operation (The item
 *	selected before the operation remains so and the dragged item
 *	is not made selected.)  We use the select status of the
 *	item being passed in to distinguish the cases.
 *
 */
static void **
GetDragItemList(window, item_index)
DmWinPtr window;
int item_index;		/* item being operated on */
{
    void **list = NULL;

    if (item_index == ExmNO_ITEM)
	return(NULL);
    
   if (!ITEM_SELECT(DM_WIN_ITEM(window, item_index))){
       /* Dragging a single item -- the one identified by item_index. 
	*/
       list = DmOneItemList(DM_WIN_ITEM(window, item_index));
   }
   else{
       /* Dragging Multiple items: The one being dragged (it will be
	* selected but not sensitive) and all other items that are
	* selected and sensitive.
	*/
       int i;
       DmItemPtr ip;
       void **lp;

       /* get the # of selected items */
       XtSetArg(Dm__arg[0], XmNselectCount, &i);
       XtGetValues(window->views[0].box, Dm__arg, 1);
       i+= 2; 	/* item dragged plus NULL terminator */

       if (list = (void **)MALLOC(sizeof(void *) * i)) {
	   for (ip=window->views[0].itp,i=window->views[0].nitems,
		lp=list; i; i--,ip++)
	       if (ITEM_MANAGED(ip) && ITEM_SELECT(ip) && ITEM_SENSITIVE(ip))
		   *lp++ = ip;
	   /* add item passed in */	
	   *lp++ = DM_WIN_ITEM(window, item_index);
	   *lp = NULL; /* a NULL terminated list */
       }	
   }

    return(list);
}	/* end of GetDragItemList */


