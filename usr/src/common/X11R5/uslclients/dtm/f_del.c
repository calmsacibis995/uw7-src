#pragma ident	"@(#)dtm:f_del.c	1.6"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-related callbacks.
*/
						/* #includes go here	*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/MessageB.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************
 * 
 * Forward Procedure definitions.
 */
static void	CancelCB(Widget, XtPointer, XtPointer);
static void	DeleteCB(Widget, XtPointer, XtPointer);
static void	DontDeleteCB(Widget, XtPointer, XtPointer);
static void	HelpCB(Widget, XtPointer, XtPointer);

/*****************************file*variables******************************
 * 
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

typedef struct _DeleteInfo {
    DmFileOpInfoPtr	opr_info;
    DmItemPtr *		items;
    int			indx;
} DeleteInfo;

static MenuItems menubarItems[] = {
    MENU_ITEM( TXT_NO_SKIP,		TXT_M_No,	DontDeleteCB ),
    MENU_ITEM( TXT_YES_DELETE,		TXT_M_Yes,	DeleteCB ),
    MENU_ITEM( TXT_CANCEL,		TXT_M_CANCEL,	CancelCB ),
    MENU_ITEM( TXT_HELP,		TXT_M_HELP,	HelpCB ),
    { NULL }						/* NULL terminated */
};

MENU_BAR("confirmDeleteNoticeMenubar", menubar, NULL, 0, 2);

static ModalGizmo noticeGizmo = {
    NULL,				/* help info */
    "confirmDeleteNotice",		/* shell name */
    TXT_G_PRODUCT_NAME,			/* title */
    &menubar,				/* menu */
    "",					/* message */
    NULL, 0,				/* gizmos, num_gizmos */
    XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
    XmDIALOG_WARNING,			/* type */
};

/***************************private*procedures****************************
 * 
 *		PRIVATE PROCEDURES
 */

/****************************procedure*header*****************************
 * DoIt- initiate the delete op
 */
static void
DoIt(DmFileOpInfoPtr opr_info)
{
    if (DM_WBIsImmediate(Desktop))
    {
	opr_info->type		= DM_DELETE;
	opr_info->target_path	= NULL;
	opr_info->dst_win	= NULL;

    } else
    {
	/* NOTE: If dropping on icon, can't use x & y */

	opr_info->type		= DM_MOVE;
	opr_info->target_path	= strdup( DM_WB_PATH(Desktop) );
	opr_info->dst_win	= DESKTOP_WB_WIN(Desktop);
    }
    opr_info->src_path		= strdup( DM_WIN_PATH(opr_info->src_win) );
    opr_info->options		=
	(opr_info->src_win->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN)) ?
	    REPORT_PROGRESS | OVERWRITE | OPRBEGIN | MULTI_PATH_SRCS :
	    REPORT_PROGRESS | OVERWRITE | OPRBEGIN;

    DmFreeTaskInfo(opr_info->src_win->task_id);
    opr_info->src_win->task_id =
        DmDoFileOp(opr_info, DmFolderFMProc, NULL);
}

/****************************procedure*header*****************************
 * DelLoop-
 */
static void
DelLoop(DeleteInfo * del_info)
{
    DmFileOpInfoPtr	opr_info = del_info->opr_info;

    while(del_info->items[++del_info->indx])
    {
	DmItemPtr	item = del_info->items[del_info->indx];
	String		ret = DmGetObjProperty(ITEM_OBJ(item), SYSTEM, NULL);

	if (ret && ((ret[0] == 'y') || (ret[0] == 'Y')))
	{
	    sprintf(Dm__buffer, Dm__gettxt(TXT_DELETE_SYSTEM_ICON_NOTICE),
		    ITEM_OBJ_NAME(item), ITEM_OBJ_NAME(item),
		    ITEM_OBJ_NAME(item));

	    if (opr_info->src_win->confirmDeleteNotice == NULL)
	    {					/* ie, once per folder */
		Widget shell;

		noticeGizmo.message = Dm__buffer;
		opr_info->src_win->confirmDeleteNotice =
		    CreateGizmo(opr_info->src_win->views[0].box,
				ModalGizmoClass, &noticeGizmo, NULL, 0);
		/*
		shell = GetModalGizmoShell(gizmo);
		XmAddWMProtocolCallback(shell, XA_WM_DELETE_WINDOW(XtDisplay(shell)),
					DestroyMsgBox_Phase2, (XtPointer) gizmo);
		XtAddCallback(GetModalGizmoShell(gizmo), XmNpopdownCallback, 
			      DestroyMsgBox_Phase2, (XtPointer) gizmo);
		*/
	    } else
	    {
		Widget box =
		    GetModalGizmoDialogBox(opr_info->src_win->confirmDeleteNotice);
		XmProcessTraversal(XmMessageBoxGetChild(box, XmDIALOG_DEFAULT_BUTTON),
				   XmTRAVERSE_CURRENT);
		SetModalGizmoMessage(opr_info->src_win->confirmDeleteNotice,
				     Dm__buffer);
	    }
	    MapGizmo(ModalGizmoClass, opr_info->src_win->confirmDeleteNotice);
	    return;

	} else
	{
	    opr_info->src_list[opr_info->src_cnt++] =
		strdup(ITEM_OBJ_NAME(item));
	}
    }

    /* Getting here means we're ready to actually initiate delete op (if any
     * src items were actually generated).
     */
    XtFree((char *)del_info->items);
    XtFree((char *)del_info);
    if (opr_info->src_cnt == 0)
    {
	XtFree((char *)opr_info->src_list);
	return;
    }

    /* Initiate delete op */
    DoIt(opr_info);
}

/****************************procedure*header*****************************
 * DeleteCB-
 */
static void
DeleteCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteInfo *	del_info = (DeleteInfo *)menubarItems[0].clientData;

    XtPopdown((Widget)DtGetShellOfWidget(widget));

    del_info->opr_info->src_list[del_info->opr_info->src_cnt++] =
	strdup(ITEM_OBJ_NAME(del_info->items[del_info->indx]));
    DelLoop(del_info);
}

/****************************procedure*header*****************************
 * CancelCB-
 */
static void
CancelCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown((Widget)DtGetShellOfWidget(widget));
}

/****************************procedure*header*****************************
 * DontDeleteCB-
 */
static void
DontDeleteCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DeleteInfo *	del_info = (DeleteInfo *)menubarItems[0].clientData;

    XtPopdown((Widget)DtGetShellOfWidget(widget));

    DelLoop(del_info);
}

/****************************procedure*header*****************************
 * HelpCB- 
 */
static void
HelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmDisplayHelpSection(DmGetHelpApp(DESKTOP_HELP_ID(Desktop)), NULL,
			 FOLDER_HELPFILE, FOLDER_CONFIRM_SYS_DELETE_SECT);

}					/* end of ExitHelpCB */

/***************************private*procedures****************************
 * 
 *		PUBLIC PROCEDURES
 */
void
DeleteItems(DmFolderWindow src_win, DmItemPtr * items, Position x, Position y)
{
    DmFileOpInfoPtr	opr_info =
	(DmFileOpInfoPtr)XtMalloc(sizeof(DmFileOpInfoRec));

    /* Load into opr_info params that are independent of IGNORE_SYSTEM prop */
    opr_info->x		= x;
    opr_info->y		= y;
    opr_info->src_win	= src_win;

    if (DmGetDTProperty(IGNORE_SYSTEM, NULL))
    {
	opr_info->src_list =
	    DmItemListToSrcList((void **)items, &opr_info->src_cnt);
	XtFree((char *)items);
	DoIt(opr_info);

    } else
    {
	DmItemPtr *	ip;
	int		cnt;
	DeleteInfo *	del_info;

	/* Get count of selected items so 1 malloc can be done rather than
	 * realloc for each selected item.
	 */
	for (cnt = 0, ip = items; *ip; cnt++, ip++)
	    ;

	opr_info->src_list	= (String *)XtMalloc(cnt * sizeof(String));
	opr_info->src_cnt	= 0;

	del_info = (DeleteInfo *)XtMalloc(sizeof(DeleteInfo));
	del_info->opr_info	= opr_info;
	del_info->items		= items;
	del_info->indx		= -1;		/* indx is auto pre-inc'ed */

	menubarItems[0].clientData = del_info;

	DelLoop(del_info);
    }
}
