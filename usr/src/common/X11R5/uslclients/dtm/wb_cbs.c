#pragma ident	"@(#)dtm:wb_cbs.c	1.180"

/******************************file*header********************************

    Description:
     This file contains the source code for callbacks for buttons
	in the Wastebasket window.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

#include <Xm/MessageB.h> /* for XmMessageBoxGetChild() */

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/ModalGizmo.h>

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
static void	CreateFilePropSheet(DmItemPtr);
static void	FilePropBtnCB(Widget, XtPointer, XtPointer);
static char 	*GetFilePerms(mode_t mode, int type);
static void	VerifyPopdnCB(Widget, XtPointer, XtPointer);
static void	PopdownCB(Widget, XtPointer, XtPointer);
static int	PutBack(DmItemPtr item);
static void	FreeWBProps(DmItemPtr);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* used in file instance property sheet */
#define OWNER	1
#define GROUP	2
#define OTHER	3

#define NameToItem(name) DmObjNameToItem((DmWinPtr) \
					 DESKTOP_WB_WIN(Desktop), name)

#define GET_STR_FROM__XMSTRING(string, _xmstr)	\
    { XmFontList  font = NULL; \
 	 string=_XmStringGetTextConcat(_XmStringCreateExternal(font, _xmstr)); \
    }

static Arg label_args[] = {
	{XmNalignment, XmALIGNMENT_BEGINNING},
};

/* Define gizmo for empty wastebasket notice */
static MenuItems menubarItems[] = {
    MENU_ITEM ( TXT_YES,	TXT_M_YES,	DmEmptyYesWB ),
    MENU_ITEM ( TXT_NO,	TXT_M_NO,	DmEmptyNoWB ),
    MENU_ITEM ( TXT_HELP,	TXT_M_HELP,	DmEmptyHelpWB ),
    { NULL }					/* NULL terminated */
};

MENU_BAR("emptyNoticeMenubar", menubar, NULL, 1, 1);	/* default: Cancel */

static Gizmo emptygizmo = NULL;

static ModalGizmo emptyGizmo = {
	NULL,					/* help info */
	"emptyNotice",				/* shell name */
	TXT_WB_EMPTY_NOTICE,			/* title */
	&menubar,				/* menu */
	TXT_EMPTY_WB,				/* message */
	NULL,					/* gizmos */
	0,					/* num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
	XmDIALOG_QUESTION,			/* type */
};

static LabelGizmo wbprop_name = {
        NULL,                   /* help */
        "wbpname",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_name_rec[] =  {
	{ LabelGizmoClass, &wbprop_name, label_args, 1 },
};

static LabelGizmo wbprop_name_label = {
        NULL,                   /* help */
        "wbpname_label",       	/* widget name */
        TXT_FP_FILE_NAME, 	/* caption label */
        False,                 	/* align caption */
        wbprop_name_rec,        /* gizmo array */
        XtNumber(wbprop_name_rec),/* number of gizmos */
};

static LabelGizmo wbprop_link = {
        NULL,                   /* help */
        "wbplink",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_link_rec[] =  {
	{ LabelGizmoClass, &wbprop_link, label_args, 1 },
};

static LabelGizmo wbprop_link_label = {
        NULL,                   /* help */
        "wbplink_label",       	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        wbprop_link_rec,          /* gizmo array */
        XtNumber(wbprop_link_rec),/* number of gizmos */
};

static LabelGizmo wbprop_loc = {
        NULL,                   /* help */
        "wbploc",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_loc_rec[] =  {
	{ LabelGizmoClass, &wbprop_loc, label_args, 1 },
};

static LabelGizmo wbprop_loc_label = {
        NULL,                   /* help */
        "wbploc_label",        	/* widget name */
        TXT_WB_ORIG_LOC,	/* caption label */
        False,                 	/* align caption */
        wbprop_loc_rec,          /* gizmo array */
        XtNumber(wbprop_loc_rec),/* number of gizmos */
};

static LabelGizmo wbprop_owner = {
        NULL,                   /* help */
        "wbpowner",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_owner_rec[] =  {
	{ LabelGizmoClass, &wbprop_owner, label_args, 1 },
};

static LabelGizmo wbprop_owner_label = {
        NULL,                   /* help */
        "wbpowner_label",         	/* widget name */
        TXT_OWNER,       		/* caption label */
        False,                 	/* align caption */
        wbprop_owner_rec,          /* gizmo array */
        XtNumber(wbprop_owner_rec),/* number of gizmos */
};

static LabelGizmo wbprop_group = {
        NULL,                   /* help */
        "wbpgroup",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_group_rec[] =  {
	{ LabelGizmoClass, &wbprop_group, label_args, 1 },
};

static LabelGizmo wbprop_group_label = {
        NULL,                   /* help */
        "wbpgroup_label",         	/* widget name */
        TXT_GROUP,       		/* caption label */
        False,                 	/* align caption */
        wbprop_group_rec,          /* gizmo array */
        XtNumber(wbprop_group_rec),/* number of gizmos */
};

static LabelGizmo wbprop_modtime = {
        NULL,                   /* help */
        "wbpmodtime",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_modtime_rec[] =  {
	{ LabelGizmoClass, &wbprop_modtime, label_args, 1 },
};

static LabelGizmo wbprop_modtime_label = {
        NULL,                   /* help */
        "wbpmodtime_label",         	/* widget name */
        TXT_MODTIME,       		/* caption label */
        False,                 	/* align caption */
        wbprop_modtime_rec,          /* gizmo array */
        XtNumber(wbprop_modtime_rec),/* number of gizmos */
};

static LabelGizmo wbprop_deltime = {
        NULL,                   /* help */
        "wbpdeltime",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_deltime_rec[] =  {
	{ LabelGizmoClass, &wbprop_deltime, label_args, 1 },
};

static LabelGizmo wbprop_deltime_label = {
        NULL,                   /* help */
        "wbpdeltime_label",         	/* widget name */
        TXT_WB_TIME_DELETED,       		/* caption label */
        False,                 	/* align caption */
        wbprop_deltime_rec,          /* gizmo array */
        XtNumber(wbprop_deltime_rec),/* number of gizmos */
};

static LabelGizmo wbprop_ownaccess = {
        NULL,                   /* help */
        "wbpownaccess",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_ownaccess_rec[] =  {
	{ LabelGizmoClass, &wbprop_ownaccess, label_args, 1 },
};

static LabelGizmo wbprop_ownaccess_label = {
        NULL,                   /* help */
        "wbpownaccess_label",         	/* widget name */
        TXT_OWNER_ACCESS,       		/* caption label */
        False,                 	/* align caption */
        wbprop_ownaccess_rec,          /* gizmo array */
        XtNumber(wbprop_ownaccess_rec),/* number of gizmos */
};

static LabelGizmo wbprop_grpaccess = {
        NULL,                   /* help */
        "wbpgrpaccess",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_grpaccess_rec[] =  {
	{ LabelGizmoClass, &wbprop_grpaccess, label_args, 1 },
};

static LabelGizmo wbprop_grpaccess_label = {
        NULL,                   /* help */
        "wbpgrpaccess_label",  	/* widget name */
        TXT_GROUP_ACCESS, 	/* caption label */
        False,                 	/* align caption */
        wbprop_grpaccess_rec,          /* gizmo array */
        XtNumber(wbprop_grpaccess_rec),/* number of gizmos */
};

static LabelGizmo wbprop_othaccess = {
        NULL,                   /* help */
        "wbpothaccess",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_othaccess_rec[] =  {
	{ LabelGizmoClass, &wbprop_othaccess, label_args, 1 },
};

static LabelGizmo wbprop_othaccess_label = {
        NULL,                   /* help */
        "wbpothaccess_label",  	/* widget name */
        TXT_OTHER_ACCESS, 	/* caption label */
        False,                 	/* align caption */
        wbprop_othaccess_rec,          /* gizmo array */
        XtNumber(wbprop_othaccess_rec),/* number of gizmos */
};

static LabelGizmo wbprop_class = {
        NULL,                   /* help */
        "wbpclass",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_class_rec[] =  {
	{ LabelGizmoClass, &wbprop_class, label_args, 1 },
};

static LabelGizmo wbprop_class_label = {
        NULL,                   /* help */
        "wbpclass_label",         	/* widget name */
        TXT_ICON_CLASS,       		/* caption label */
        False,                 	/* align caption */
        wbprop_class_rec,          /* gizmo array */
        XtNumber(wbprop_class_rec),/* number of gizmos */
};

static LabelGizmo wbprop_comment = {
        NULL,                   /* help */
        "wbpcomment",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec wbprop_comment_rec[] =  {
	{ LabelGizmoClass, &wbprop_comment, label_args, 1 },
};

static LabelGizmo wbprop_comment_label = {
        NULL,                   /* help */
        "wbpcomment_label",         	/* widget name */
        TXT_COMMENTS,       		/* caption label */
        False,                 	/* align caption */
        wbprop_comment_rec,          /* gizmo array */
        XtNumber(wbprop_comment_rec),/* number of gizmos */
};



static GizmoRec WBPropertyPreRec[] = {
	{LabelGizmoClass, &wbprop_name_label},
	{LabelGizmoClass, &wbprop_link_label},
	{LabelGizmoClass, &wbprop_loc_label},
	{LabelGizmoClass, &wbprop_owner_label},
	{LabelGizmoClass, &wbprop_group_label},
	{LabelGizmoClass, &wbprop_modtime_label},
	{LabelGizmoClass, &wbprop_ownaccess_label},
	{LabelGizmoClass, &wbprop_grpaccess_label},
	{LabelGizmoClass, &wbprop_othaccess_label},
	{LabelGizmoClass, &wbprop_class_label},
	{LabelGizmoClass, &wbprop_comment_label},
};

static ContainerGizmo WBPropertyContainer = {
        NULL, "WBPropertyContainer", G_CONTAINER_RC,
        NULL, NULL, WBPropertyPreRec, XtNumber (WBPropertyPreRec),
};

static GizmoRec WBPropertyRec[] = {
	{ContainerGizmoClass, &WBPropertyContainer}
};

/* Define the menu */

static MenuItems WBMenubarItems[] = {
        MENU_ITEM( TXT_OK,   TXT_M_OK,    NULL ),
        MENU_ITEM( TXT_HELP,    TXT_M_HELP,     NULL ),
        { NULL }
};
MENU_BAR("WBPropMenubar", WBMenubar, FilePropBtnCB, 0, 0);  /* default: Apply */

static PopupGizmo WBPropertyWindow = {
        NULL, 					/* help */
        "wbprop_popup",				/* widget name */
        TXT_WB_FILEPROP_TITLE, 			/* title */
        &WBMenubar,           			/* menu */
        WBPropertyRec, 				/* gizmo array */
        XtNumber(WBPropertyRec),    		/* number of gizmos */
};


/***************************private*procedures****************************

    Private Procedures
*/


/****************************procedure*header*****************************
 * Calls DmDoFileOp() to undelete a file in the wastebasket.
 */
static int
PutBack(DmItemPtr item)
{
	DmWBCPDataPtr cpdp;
	DmFileOpInfoPtr opr_info;
	char *real_path;
	char *path;

	/* make item busy so that it can't be used by anyone else
	 * e.g. the timer
	 */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	ExmFlatSetValues(DESKTOP_WB_WIN(Desktop)->views[0].box,
		item - DESKTOP_WB_WIN(Desktop)->views[0].itp, Dm__arg, 1);

	/* make WB window busy while doing file op */
	DmBusyWindow(DESKTOP_WB_WIN(Desktop)->shell, True);

	if (ITEM_OBJ(item)->objectdata == NULL) {
		/* initialize file's op->objectdata to its stat info */
		path = DmMakePath(DM_WB_PATH(Desktop), ITEM_OBJ_NAME(item));
		ITEM_OBJ(item)->objectdata = (void *)WBGetFileInfo(path);
	}
	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_PUTBACK;
	cpdp->itp     = item;

	real_path = DmGetObjProperty(ITEM_OBJ(item), REAL_PATH, NULL);
	cpdp->target = strdup(real_path);

	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->type = DM_RENAME;	/* re-maps to DM_MOVE */
	opr_info->options = DONT_OVERWRITE;

	/* Need to remove the one '/' from real_path if target is root
	 * directory.
	 */
	if (real_path[0] == '/' && real_path[1] == '/')
		opr_info->target_path = strdup(real_path + 1);
	else
		opr_info->target_path = strdup(real_path);

	opr_info->src_path    = strdup(DM_WB_PATH(Desktop));
	opr_info->src_list    = (char **)MALLOC(sizeof(char *));
	opr_info->src_cnt     = 1;
	opr_info->dst_win     = NULL;
	opr_info->src_win     = DESKTOP_WB_WIN(Desktop);
	opr_info->x	       = opr_info->y = UNSPECIFIED_POS;
	opr_info->src_list[0] = strdup(ITEM_OBJ_NAME(item));
	if (DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp) == NULL)
	{
	    XtSetArg(Dm__arg[0], XmNsensitive, True);
	    ExmFlatSetValues(DESKTOP_WB_WIN(Desktop)->views[0].box,
			     item - DESKTOP_WB_WIN(Desktop)->views[0].itp,
			     Dm__arg, 1);
	    return(1);

	} else
	    return(0);
}					/* end of PutBack */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Returns all selected files in wastebasket to where they were
 * deleted from.  Files are put back one at a time because they can
 * be targeted for different folders - can't specify more than one
 * target in call to DmDoFileOp(). 
 *
 * (NOTE: In all callbacks in Edit menu, can't reply on client_data
 * being NULL; client_data is used to set/unset sensitivity.)
 *
 */
void
DmWBEMPutBackCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmItemPtr	itp;

    DmDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
    for (itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	 itp < DESKTOP_WB_WIN(Desktop)->views[0].itp +
	 DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++)
	if (ITEM_MANAGED(itp) && ITEM_SELECT(itp) && ITEM_SENSITIVE(itp))
	    if (PutBack(itp))
		break;
}					/* end of DmWBEMPutBackCB */

/****************************procedure*header*****************************
 * Requests user confirmation to empty the wastebasket.
 */
void
DmConfirmEmptyCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	Widget btn;

	if (WB_IS_EMPTY(Desktop))
		return;

	DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	/* Create the modal gizmo */
	if (emptygizmo  == NULL) {
		emptygizmo =
		CreateGizmo(XtParent(DESKTOP_WB_WIN(Desktop)->views[0].box),
			ModalGizmoClass, &emptyGizmo, NULL, 0);
		/* register for context-sensitive help */
		XtAddCallback(GetModalGizmoDialogBox(emptygizmo),
			XmNhelpCallback, DmEmptyHelpWB, NULL);
	}
	DmBeep();
	MapGizmo(ModalGizmoClass, emptygizmo);

	btn = QueryGizmo(ModalGizmoClass, emptygizmo, GetGizmoWidget,
		"emptyNoticeMenubar:1");
	XmProcessTraversal(btn, XmTRAVERSE_CURRENT);

} /* end of DmConfirmEmptyCB */

/****************************procedure*header*****************************
 * Calls DmDoFileOp() to delete a file in the wastebasket.
 */
void
DmWBDelete(char ** src_list, int src_cnt, DmWBCPDataPtr cpdp)
{
    DmFileOpInfoPtr	opr_info =
	(DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));

    opr_info->type        = DM_DELETE;
    opr_info->options     = 0;
    opr_info->target_path = NULL;
    opr_info->src_path    = strdup(DM_WB_PATH(Desktop));
    opr_info->src_list    = src_list;
    opr_info->src_cnt     = src_cnt;
    opr_info->src_win     = DESKTOP_WB_WIN(Desktop);
    opr_info->dst_win     = NULL;
    opr_info->x           = opr_info->y = UNSPECIFIED_POS;
    (void)DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);

}					/* end of DmWBDelete */

/****************************procedure*header*****************************
 * Permanently removes the contents of the wastebasket.
 */
void
DmEmptyYesWB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmItemPtr itp;
	DmWBCPDataPtr cpdp;
	char **src_list;
	char **src;

	XtPopdown(QueryGizmo(ModalGizmoClass, emptygizmo, GetGizmoWidget,
					"emptyNotice"));

         /* no items */
	if (WB_IS_EMPTY(Desktop))
	    return;

	src_list = src = (char **)
	    MALLOC(DESKTOP_WB_WIN(Desktop)->views[0].nitems * sizeof(char *));

	for (itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	     itp < DESKTOP_WB_WIN(Desktop)->views[0].itp +
	     DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++)
	{
		if (ITEM_MANAGED(itp) && ITEM_SENSITIVE(itp))
			*src++ = strdup(ITEM_OBJ_NAME(itp));
	}
	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_EMPTY;
	DmWBDelete(src_list, src - src_list, cpdp);

}					/* end of DmEmptyWB */

/****************************procedure*header*****************************
 * Cancels the wastebasket's Empty Confirmation Notice.
 */
void
DmEmptyNoWB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtPopdown(QueryGizmo(ModalGizmoClass, emptygizmo, GetGizmoWidget,
					"emptyNotice"));

	return;
}					/* end of DmEmptyNoWB

/****************************procedure*header*****************************
 * Posts Help for the wastebasket's Empty Confirmation Notice.
 */
void
DmEmptyHelpWB(Widget w, XtPointer client_data, XtPointer call_data)
{
     /* Help */
     DmHelpAppPtr help_app = DmGetHelpApp(WB_HELP_ID(Desktop));

     DmDisplayHelpSection(help_app, NULL, WB_HELPFILE, WB_EMPTY_SECT);
     XtAddGrab(help_app->hlp_win.shell, False, False);
     return;
}


/****************************procedure*header*****************************
 * Submits a list of files in the wastebasket to be deleted.
 */
static void
Delete(Cardinal item_index)
{
	DmWBCPDataPtr cpdp;
	void **item_list;
	char **src_list;
	int src_cnt;

        if ((item_list = DmGetItemList((DmWinPtr)DESKTOP_WB_WIN(Desktop),
		item_index)) == NULL)
	{
		return;
	}
        if ((src_list = DmItemListToSrcList(item_list, &src_cnt)) == NULL)
	{
        	FREE((void *)item_list);
		return;
	}
        FREE((void *)item_list);

        if (src_cnt == 0)
            return;

        cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
        cpdp->op_type = DM_WBDELETE;
        DmWBDelete(src_list, src_cnt, cpdp);

}					/* end of Delete */

/****************************procedure*header*****************************
 * Permanently removes selected files in wastebasket.
 */
void
DmWBEMDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	Delete(ExmNO_ITEM);
}					/* end of DmWBEMDeleteCB */

/****************************procedure*header*****************************
    DmWBPutBackByDnD - Called from DmDropProc() to handle undeleting files
    by dragging them out of the Wastebasket.
 */
void
DmWBPutBackByDnD(DmFolderWinPtr dst_win, ExmFlatDropCallData *d, void **list)
{
	DmObjectPtr dst_obj;
	DmFolderWindow src_win;
	Cardinal dst_indx;
	int src_cnt;
	char **src;
	char **src_list;

	DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	/* Get type of dst object.  Can only drop on background of a folder
	 * window or on a folder icon.
	 */
	dst_indx = d->item_data.item_index;

	if ((dst_indx != ExmNO_ITEM) &&
	  !OBJ_IS_DIR((dst_obj = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_indx)))))
	{
		DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), True,
			TXT_WB_DROP_FAILED);
		return;
	}
	/* Get the list of items to be operated on.  "list" will be freed in
	 * DmDropProc().
	 */
	if ((src_list = DmItemListToSrcList(list, &src_cnt)) == NULL)
	{
		return;
	}
	/* Since file op's are generated in a loop but they are executed async
	 * later, we cannot handle overwrites.
	 */
	for (src = src_list; src < src_list + src_cnt; src++)
	{
		char *real_path;
		char *bname;
		char *target;
		DmWBCPDataPtr cpdp;
		DmFileOpInfoPtr opr_info;
		DmItemPtr src_item = NameToItem(*src);

		if (src_item == NULL)
			continue;

		/* Initialize file's op->objectdata to its stat info if
		 * it's NULL
		 */
		if (ITEM_OBJ(src_item)->objectdata == NULL)
			ITEM_OBJ(src_item)->objectdata =
			  (void *)WBGetFileInfo(DmMakePath(DM_WB_PATH(Desktop),
			  *src));

		real_path = DmGetObjProperty(ITEM_OBJ(src_item), REAL_PATH,
				NULL);
		bname = basename(real_path);

		/* If dropping on folder icon, target is that path (and can't
		 * use drop coord's.  Otherwise it's the path of the container.
		 */
		if (dst_indx == ExmNO_ITEM) {
			/* dropping on background */
			target = DmMakePath(dst_win->views[0].cp->path, bname);
			if (src_cnt > 1) {
				d->x = UNSPECIFIED_POS;
				d->y = UNSPECIFIED_POS;
			}
		} else {
			target = DmMakePath(DmObjPath(dst_obj), bname);
			d->x = UNSPECIFIED_POS;
			d->y = UNSPECIFIED_POS;
		}	    
		cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
		cpdp->itp      = src_item;
		cpdp->op_type  = DM_DROP;
		cpdp->target   = strdup(target);

		opr_info = (DmFileOpInfoPtr)CALLOC(1, sizeof(DmFileOpInfoRec));
		opr_info->type        = DM_RENAME; /* re-maps to DM_MOVE */
		opr_info->options     = DONT_OVERWRITE;
		opr_info->target_path = strdup(target);
		opr_info->src_path    = strdup(DM_WB_PATH(Desktop));
		opr_info->src_list    = (char **)MALLOC(sizeof(char *));
		opr_info->src_cnt     = 1;
		opr_info->src_win     = DESKTOP_WB_WIN(Desktop);
		opr_info->dst_win     = NULL;
		opr_info->x           = d->x;
		opr_info->y           = d->y;
		opr_info->src_list[0] = strdup(*src);
		DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);
	}
}					/* end of DmWBPutBackByDnD */

/****************************procedure*header*****************************
 * Called when timer expires.  Checks for files to be deleted based on
 * their time stamps and the current time, then restarts the timer.
 * If the wastebasket is empty, call DmWBRestartTimer() to "do the right
 * thing", although this function should never be called if the wastebasket
 * is empty.
 */
void
DmWBTimerProc(client_data, timer_id)
XtPointer client_data;
XtIntervalId timer_id;
{
	DmItemPtr itp;
	DmWBCPDataPtr cpdp;
	Boolean reset = False;
	time_t time_stamp;
	time_t current_time;
	time_t interval_unit;
	char **nsrc_list;
	char **src_list = NULL;
	int i;
	int src_cnt = 0;
	int alloc = NUM_ALLOC;


	wbinfo.timer_id = NULL;

	/* this should never be used */
	if (WB_IS_EMPTY(Desktop) && WB_BY_TIMER(wbinfo)) {
		if (wbinfo.suspend == False)
			wbinfo.restart = True;
		DmWBRestartTimer();
		return;
	}
	src_list = (char **)MALLOC(NUM_ALLOC * sizeof(char *));

	time(&(current_time));
	/* get interval unit in seconds */
	interval_unit = (time_t)(wbinfo.time_unit / 1000);

	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	     i < DESKTOP_WB_WIN(Desktop)->views[0].nitems; i++, itp++)
	{
		if (ITEM_MANAGED(itp))
		{
			time_stamp = strtoul(DmGetObjProperty(ITEM_OBJ(itp),
						TIME_STAMP, NULL), NULL, 0);

			if (((current_time - time_stamp) / interval_unit)
				>= (time_t)(wbinfo.interval)) {

				/* don't delete busy files and reset their time stamp
				 * to current time to avoid an infinite loop.
				 */
				if (!ITEM_SENSITIVE(itp))
				{
					sprintf(Dm__buffer, "%ld", current_time);
					DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
						Dm__buffer, NULL);
					reset = True;
					continue;
				}
				if (src_cnt == alloc)
				{
					nsrc_list = (char **)REALLOC((void *)src_list,
						(alloc + NUM_ALLOC) * sizeof(char *));
					alloc += NUM_ALLOC;
					src_list = nsrc_list;
				}
				src_list[src_cnt] = strdup(ITEM_OBJ_NAME(itp));
				++src_cnt;
			} 
		}
	}
	if (reset)
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

	if (src_cnt == 0) {
		DmWBRestartTimer();
		if (src_list)
			FREE((void *)src_list);
		return;
	}
	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_TIMER;
	DmWBDelete(src_list, src_cnt, cpdp);

} /* end of DmWBTimerProc */

/****************************procedure*header*****************************
 * Called when the Suspend/Resume Timer button is selected.
 * Suspends the timer if it's currently on; otherwise, turn
 * it on again.  Update wbSuspend resource in .Xdefaults
 * so that it is always up-to-date.  Note that this resource
 * is otherwise only updated when the Wastebasket properties
 * are changed.
 */
void
DmWBTimerCB(w, client_data, call_data)
Widget w;
XtPointer	client_data;
XtPointer	call_data;
{
	if (!WB_BY_TIMER(wbinfo) || WB_IS_EMPTY(Desktop))
		return;

	if (wbinfo.suspend == True)
		DmWBResumeTimer();
	else {
		DmWBSuspendTimer();
		wbinfo.restart = False;
	}

	/* save timer state as a container property and save in WB's .dtinfo */
	sprintf(Dm__buffer, "%d", wbinfo.suspend);
	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist), TIMER_STATE,
		Dm__buffer, NULL);
	sprintf(Dm__buffer, "%d", wbinfo.restart);
	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist), RESTART_TIMER,
		Dm__buffer, NULL);
	DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

} /* end of DmWBTimerCB */

#include "error.h"	/* For overwrite error below */

/****************************procedure*header*****************************
 * Client proc called after file manipulation is completed.
 */
void
DmWBClientProc(DmProcReason reason, XtPointer client_data, XtPointer call_data,
char *src_file, char *dst_file)
{
	DmTaskInfoListPtr tip = (DmTaskInfoListPtr)call_data;
	DmFileOpInfoPtr opr_info = tip->opr_info;
	DmWBCPDataPtr cpdp = (DmWBCPDataPtr)client_data;
	char *name;
	char *tmp;

	/* unbusy WB window */
	DmBusyWindow(DESKTOP_WB_WIN(Desktop)->shell, False);

	switch(reason) {
	case DM_DONE:
		switch(cpdp->op_type) {
		case DM_PUTBACK:
		case DM_DROP:
			if (opr_info->src_info[0] & SRC_B_IGNORE)
				break;

			/* opr_info->target_path_path must always be a full
			 * path.
			 */
			tmp = strdup(opr_info->target_path);
			name = dirname(tmp);

			if (opr_info->error == 0)
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
				0, TXT_WB_PUTBACK_SUCCESS, cpdp->itp->label,
				basename(name));

			FREE(ITEM_OBJ_NAME(cpdp->itp));
			ITEM_OBJ_NAME(cpdp->itp) =
				strdup(basename(opr_info->target_path));

			FREE(opr_info->src_list[0]);
			opr_info->src_list[0]=strdup(ITEM_OBJ_NAME(cpdp->itp));

			FreeWBProps(cpdp->itp);

			if (opr_info->dst_win == NULL)
				opr_info->dst_win = DmQueryFolderWindow(name, NULL);
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN | DM_UPDATE_DSTWIN);
			FREE(tmp);
			break;
		case DM_WBDELETE:
		case DM_TIMERCHG:
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			if (opr_info->error == 0) {
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			}
			break;
		case DM_EMPTY:
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);

			if (opr_info->error == 0)
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			break;
		case DM_TIMER:
			/* Temporarily deal with error condition: if timer faibled
			 * to remove a file, remove the item and obj anyway so
			 * timer doesn't continue to try to delete it.
			 */
			if (opr_info->error != 0) {
			    DmContainerPtr cp;
			    DmObjectPtr obj;
			    char *name;

			    cp = DESKTOP_WB_WIN(Desktop)->views[0].cp;
			    name = opr_info->src_list[(opr_info->cur_src == 0) ? 0 : opr_info->cur_src - 1];
			    obj = DmGetObjectInContainer(cp, name);
			    DmRmObjectFromContainer(cp, obj);
			} else
			    DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			break;
		case DM_IMMEDDELETE:
			if (opr_info->src_win == NULL)
				opr_info->src_win =
					DmQueryFolderWindow(opr_info->src_path, NULL);
			DmUpdateWindow(opr_info, DM_UPDATE_SRCWIN);
			if (opr_info->error == 0)
				DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
			break;
		default:
			break;
		}
		DmSwitchWBIcon();
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

		/* If the wastebasket is now empty but the timer is still
		 * running, suspend it and set wbinfo.restart to True so
		 * that when it is non-empty again, the timer will be
		 * resumed for the user. If By Timer is selected but
		 * wbinfo.suspend is True, make the Resume Timer button
		 * insensitive because the timer is automatically suspended
		 * when the wastebasket is empty. If reason is DM_TIMER,
		 * just restart the timer.
		 */
		if (WB_BY_TIMER(wbinfo)) {
			if (WB_IS_EMPTY(Desktop)) {
				if (wbinfo.suspend == False) {
					wbinfo.restart = True;
					DmWBRestartTimer();
				} else {
					wbinfo.restart = False;
					/* make Suspend/Timer Button insensitive */
					DmWBToggleTimerBtn(False);
				}
			} else if (cpdp->op_type == DM_TIMER
				|| cpdp->op_type == DM_TIMERCHG)
			{
					if (wbinfo.suspend == False)
						wbinfo.restart = True;
					DmWBRestartTimer();
			}
		}
		DmFreeTaskInfo(tip);

		if (cpdp->target)
			FREE(cpdp->target);

		if (cpdp)
			FREE((void *)cpdp);
		break;
	case DM_ERROR:
		switch(cpdp->op_type) {
		case DM_PUTBACK:
		case DM_DROP:
			name = (char *)DmGetObjProperty(ITEM_OBJ(cpdp->itp),
					REAL_PATH, NULL);

			if (opr_info->error == ERR_IsAFile)
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    True, TXT_WB_CANT_OVERWRITE, basename(name));
			else
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    True, TXT_WB_PUTBACK_FAILED, basename(name));
			break;
		case DM_WBDELETE:
		case DM_TIMERCHG:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_WBDELETE_FAILED);
			else
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_PARTIAL_DELETE);
			break;
		case DM_EMPTY:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_EMPTY_FAILED);
			else
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_PARTIAL_EMPTY);
			break;
		case DM_TIMER:
			/*Temporarily ignore any errors during timer deletion*/
			break;
		case DM_IMMEDDELETE:
			if (opr_info->cur_src == 0 || opr_info->cur_src == 1)
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_IMMEDDEL_FAILED);
			else
			  DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop),
			    1, TXT_PARTIAL_DELETE);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}					/* end of DmWBClientProc */

/****************************procedure*header*****************************
 * Frees all wastebasket-specific properties. 
 * This routine is called when a file is undeleted.
 */
static void
FreeWBProps(DmItemPtr itp)
{
	DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), VERSION, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), "f", NULL, NULL);
	DmSetObjProperty(ITEM_OBJ(itp), CLASS_NAME, NULL, NULL);

} /* end of FreeWBProps */

/****************************procedure*header*****************************
 * Callback for Properties button in icon menu to show file properties
 * of item from which icon menu is popped up.
 */
/****************************procedure*header*****************************
 * This routine is called when Delete is selected from the icon menu.
 * It moves an item in the wastebasket back to where it was deleted from.
 * client_data contains pointer to item to be put back.
 */
/****************************procedure*header*****************************
 * Permanently removes item from which the icon menu is obtained.
 */
void
DmWBIconMenuCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmItemPtr itp = (DmItemPtr)(cbs->clientData);

	DmDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));

	switch(cbs->index)
	{
	case 0:

		/* pop up property sheet only if item is not currently busy */
		if (ITEM_SENSITIVE(itp))
			CreateFilePropSheet(itp);
		break;

	case 1:
		(void)PutBack((DmItemPtr)itp);
		break;

	case 2:
		Delete(itp - DESKTOP_WB_WIN(Desktop)->views[0].itp);
		break;

	default:
		break;
	}
	

} /* end of DmWBIconMenuCB */


/****************************procedure*header*****************************
 * Callback for Properties button in Edit menu to show file properties
 * of all selected items in wastebasket window.
 */
void
DmWBEMFilePropCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmItemPtr itp;
	int i;

	DmDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
	for (i = 0, itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
		i < DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++, i++) {

		if (ITEM_MANAGED(itp) && ITEM_SELECT(itp) && ITEM_SENSITIVE(itp))
			CreateFilePropSheet(itp);
	}
} /* end of DmWBEMFilePropCB */

/****************************procedure*header*****************************
 * Creates a property sheet for item pointed to by itp.  The item is set
 * to busy state while the property sheet is up.  The property sheet is
 * destroyed when it is dismissed.
 */
void
CreateFilePropSheet(itp)
DmItemPtr itp;
{
#define FINFO(obj)  ( (DmFileInfoPtr)((obj)->objectdata) )

	struct passwd *pw;
	struct group *gr;
	time_t tstamp;
	Gizmo fp_shell;
	int len;
	char *pval;
	char *path;
	static int first = 0;
	Boolean unmanage_link;
	char buffer[BUFSIZ];
	char * owner_perms;
	char * group_perms;
	char * other_perms;
	DmFPropSheetPtr	fpsptr;
	static int wbprop_num = 1;

	fpsptr		= GetNewSheet(DESKTOP_WB_WIN(Desktop));
	fpsptr->prop_num= wbprop_num++;
	fpsptr->window 	= DESKTOP_WB_WIN(Desktop);
	fpsptr->item	= itp;
	fpsptr->prev	= ITEM_OBJ_NAME(itp);

	/* busy the item; unbusy it when property sheet is dismissed */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	ExmFlatSetValues(DESKTOP_WB_WIN(Desktop)->views[0].box,
		itp - DESKTOP_WB_WIN(Desktop)->views[0].itp, Dm__arg, 1);

	GET_STR_FROM__XMSTRING(wbprop_name.label, (_XmString)itp->label);

	path = DmMakePath(DM_WB_PATH(Desktop), ITEM_OBJ_NAME(itp));
	if (ITEM_OBJ(itp)->objectdata == NULL) {
		/* initialize file's op->objectdata to its stat info */
		ITEM_OBJ(itp)->objectdata = (void *)WBGetFileInfo(path);
	}

	/* display link info */
	if (ITEM_OBJ(itp)->attrs & DM_B_SYMLINK)
	{
		/* display symbolic link info */
		int len = readlink(path, buffer, BUFSIZ);
		buffer[len] = '\0'; /* readlink doesn't do this */
		wbprop_link_label.label = TXT_FPROP_SYMLINK;
                wbprop_link.label = buffer;
	}
	else if (ITEM_OBJ(itp)->ftype != DM_FTYPE_DIR)
	{
		/* display hard link info */
		sprintf(buffer, "%d", FINFO(ITEM_OBJ(itp))->nlink);
		wbprop_link_label.label = TXT_FPROP_HARDLINK;
                wbprop_link.label = buffer;
	}
	else
		unmanage_link = True;

	pval = DmGetObjProperty(ITEM_OBJ(itp), REAL_PATH, NULL);
	path = strdup(pval);
	wbprop_loc.label = dirname(path);


	pw = getpwuid(FINFO(ITEM_OBJ(itp))->uid);
	wbprop_owner.label = pw->pw_name;

	gr = getgrgid(FINFO(ITEM_OBJ(itp))->gid);
	wbprop_group.label = gr->gr_name;

	(void)strftime(Dm__buffer, sizeof(Dm__buffer), TIME_FORMAT,
		localtime(&(FINFO(ITEM_OBJ(itp))->mtime)));
	wbprop_modtime.label = strdup(Dm__buffer);



	tstamp = strtoul(DmGetObjProperty(ITEM_OBJ(itp), TIME_STAMP, NULL),
			NULL, 0);
	(void)strftime(Dm__buffer, sizeof(Dm__buffer), TIME_FORMAT,
		localtime(&tstamp));
	wbprop_deltime.label = strdup(Dm__buffer);


	owner_perms = strdup(GetFilePerms(FINFO(ITEM_OBJ(itp))->mode, OWNER));
	wbprop_ownaccess.label = owner_perms;

	group_perms = strdup(GetFilePerms(FINFO(ITEM_OBJ(itp))->mode, GROUP));
	wbprop_grpaccess.label = group_perms;

	other_perms = strdup(GetFilePerms(FINFO(ITEM_OBJ(itp))->mode, OTHER));
	wbprop_othaccess.label = other_perms;

	wbprop_class.label =DmGetObjProperty(ITEM_OBJ(itp), CLASS_NAME, NULL);

	pval = DmGetObjProperty(ITEM_OBJ(itp), "Comment", NULL);
	wbprop_comment.label =  pval ? pval : "";


	((MenuGizmo *)(WBPropertyWindow.menu))->clientData = (char *) fpsptr;
	fpsptr->shell = CreateGizmo(DESKTOP_WB_WIN(Desktop)->shell, 
						PopupGizmoClass, 
						&WBPropertyWindow, NULL, 0);

	XtAddCallback(GetPopupGizmoShell(fpsptr->shell), XtNpopdownCallback,
		PopdownCB, (XtPointer)fpsptr);

	/* register for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(fpsptr->shell),
		WB_HELP_ID(Desktop), WB_HELPFILE, WB_FILE_PROP_SECT);

	MapGizmo(PopupGizmoClass, fpsptr->shell);

	free(path);
	free(wbprop_modtime.label);
	free(wbprop_deltime.label);
	free(owner_perms);
	free(group_perms);
	free(other_perms);
} /* end of CreateB


/****************************procedure*header*****************************
 * Callback for menu in Properties window.  It pops down the window when
 * OK is selected and displays help when Help is selected.
 */
static void
FilePropBtnCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	MenuGizmoCallbackStruct *cbs  = (MenuGizmoCallbackStruct *)client_data;
	DmFPropSheetPtr   	fpsptr = (DmFPropSheetPtr)cbs->clientData;

	if (cbs->index == 0)
		BringDownPopup(GetPopupGizmoShell(fpsptr->shell));
	else
		/* Help button selected */
		DmDisplayHelpSection(DmGetHelpApp(WB_HELP_ID(Desktop)), NULL, 
			WB_HELPFILE, WB_FILE_PROP_SECT);

} /* end of FilePropBtnCB */

/****************************procedure*header*****************************
 * Destroy file properties sheet and unbusy item.
 */
static void
PopdownCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmFPropSheetPtr fpsptr = (DmFPropSheetPtr)client_data;
	DmItemPtr itp = NULL;

	/* unbusy the item */
	if (fpsptr->prev != NULL) {
		itp = NameToItem(fpsptr->prev);

		if (itp != NULL) {
			ExmVaFlatSetValues(
				DESKTOP_WB_WIN(Desktop)->views[0].box,
				itp - DESKTOP_WB_WIN(Desktop)->views[0].itp,
				XmNsensitive, True, NULL);
		}
	}
	FreeGizmo(PopupGizmoClass, fpsptr->shell);
	XtDestroyWidget(w);
	fpsptr->shell = NULL;
	fpsptr->prop_num = -1;


} /* end of PopdownCB */

/****************************procedure*header*****************************
 * Gets file permissions of a file in the wastebasket to be displayed
 * in its properties sheet.
 */
static char *
GetFilePerms(mode_t mode, int type)
{
	Boolean rp, wp, ep;
	static char *read_perm;
	static char *write_perm;
	static char *exec_perm;
	static int first = 0;

	if (!first) {
		read_perm = (char *)GetGizmoText(TXT_READ_PERM);
		write_perm = (char *)GetGizmoText(TXT_WRITE_PERM);
		exec_perm = (char *)GetGizmoText(TXT_EXEC_PERM);
		first = 1;
	}

	switch(type) {
	case OWNER:
		rp = (mode & S_IRUSR ? True : False);
		wp = (mode & S_IWUSR ? True : False);
		ep = (mode & S_IXUSR ? True : False);
		break;
	case GROUP:
		rp = (mode & S_IRGRP ? True : False);
		wp = (mode & S_IWGRP ? True : False);
		ep = (mode & S_IXGRP ? True : False);
		break;
	case OTHER:
		rp = (mode & S_IROTH ? True : False);
		wp = (mode & S_IWOTH ? True : False);
		ep = (mode & S_IXOTH  ? True : False);
		break;
	}
	sprintf(Dm__buffer, "%s %s %s", (rp ? read_perm : ""),
			(wp ? write_perm : ""), (ep ? exec_perm : "")); 
	return(Dm__buffer);

} /* end of GetFilePerms */

