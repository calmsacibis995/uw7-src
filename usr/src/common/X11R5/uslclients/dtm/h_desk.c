/*		copyright	"%c%" 	*/

#pragma ident	"@(#)dtm:h_desk.c	1.102"

/******************************file*header********************************

    Description:
     This file contains the source code to initialize the Help Desk.
*/
                              /* #includes go here     */
#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>	/* for XmAddWMProtocolCallback */

#include <Dt/Desktop.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public  Procedures
          2. Private Procedures
*/
                         /* private procedures         */

static void UpdateObjs();
static void SetupIcons(Boolean new_locale);
static void CreateFileClass();
static void WMCB(Widget, XtPointer, XtPointer);
static void ExitCB(Widget, XtPointer, XtPointer);
static void HandleResize(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* global variable */
DmHDDataPtr hddp = NULL;

/* to be used to get value of _locale line in Help Desk's .dtinfo file. */
#define LOCALE         "locale"

static MenuItems FileMenuItems[] = {
 { True, TXT_FILE_OPEN, TXT_M_FILE_OPEN, I_PUSH_BUTTON, NULL, DmHDOpenCB,
	NULL, True },
 { True, TXT_FILE_EXIT, TXT_M_Exit,      I_PUSH_BUTTON, NULL, ExitCB,
	NULL, False },
 { NULL }
};

MENU("FileMenu", FileMenu);

static MenuItems HelpMenuItems[] = {
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON, NULL,
	NULL, NULL, True },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
 { NULL }
};

static MenuGizmo HelpMenu = {
	NULL, "HelpMenu", "_X_", HelpMenuItems, DmHDHelpCB, NULL
};

static MenuItems MenuBarItems[] = {
 { True, TXT_FILE, TXT_M_FILE, I_PUSH_BUTTON, NULL, NULL },
 { True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, NULL, NULL },
 { NULL }
};

MENU_BAR("menubar", MenuBar, DmMenuSelectCB, 0, 0);

static ContainerGizmo swin_gizmo = {
	NULL,			/* help */
	"swin",			/* name */
	G_CONTAINER_SW,		/* container type */
};

static GizmoRec gizmos[] = {
     {ContainerGizmoClass, &swin_gizmo},
};

static MsgGizmo msg_gizmo = {NULL, "footer", " ", " "};

static BaseWindowGizmo HelpDeskWindow = {
	NULL,			/* help */
	"helpdesk",		/* shell widget name */
	TXT_HELPDESK_TITLE,	/* title */
	&MenuBar,		/* menu bar */
	gizmos,			/* gizmo array */
	XtNumber(gizmos),	/* # of gizmos in array */
	&msg_gizmo,		/* footer */
	TXT_HELPDESK_TITLE, 	/* icon_name */
	"hdesk48.icon",     	/* name of pixmap file */
};

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
   DmInitHelpDesk - Initializes the Help Desk. Creates the Help Desk window,
	and reads in the list of applications/objects to be installed in it
	from $XWINHOME/desktop/Help_Desk which is the Help Desk's .dtinfo file.
 */

unsigned int
DmInitHelpDesk(char *geom_str, Boolean iconic, Boolean map_window)
{
#define EXTRA_ICON_SLOT 5
	int n;
	Arg args[10];
	char *hdpath;
	char *q;
	struct stat hstat;
	DmFolderWindow hd_win;
	Gizmo file_menu;
	Gizmo view_menu;
	Gizmo help_menu;
	Gizmo menu_bar;
	Widget form;
	static MenuGizmoCallbackStruct cbs;

	/* get name of Help Desk file */
	hdpath = (char *)(DmDTProp(HDPATH, NULL));

	/* check if hdpath exists. */
	if ((stat(hdpath, &hstat)) != 0) {
		if (errno == ENOENT) {
			/* couldn't locate Help Desk file */
			Dm__VaPrintMsg(TXT_NO_HELPDESK_FILE);
			return(1);
		} /* handle else case */
	}

	/* allocate structure to store help desk info if this hasn't
	 * already been done in DmInitDesktop().
	 */
	if (hddp == NULL) {
		if ((hddp =
		  (DmHDDataPtr)CALLOC(1, sizeof(DmHDDataRec))) == NULL)
		{
			Dm__VaPrintMsg(TXT_MEM_ERR);
			return(1);
		}
	}
	if ((hd_win =
	  (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec))) == NULL)
	{
		Dm__VaPrintMsg(TXT_MEM_ERR);
		FREE(hddp);
		return(1);
	}
	hd_win->views[0].view_type = DM_ICONIC;
	hd_win->attrs              = DM_B_HELPDESK_WIN;

	/* Create application shell	*/
	XtSetArg(Dm__arg[0], XmNiconic, iconic);
	XtSetArg(Dm__arg[1], XmNgeometry, geom_str);
	XtSetArg(Dm__arg[2], XmNmappedWhenManaged, map_window);

	n = 0;
        XtSetArg(args[n], XmNshadowThickness, 2); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
        XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++;
        XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
	gizmos->args = args;
	gizmos->numArgs = n;

	MenuBar.clientData = (XtPointer)hd_win;
	hd_win->gizmo_shell = CreateGizmo(NULL, BaseWindowGizmoClass,
		&HelpDeskWindow, Dm__arg, 3);

	hd_win->shell = GetBaseWindowShell(hd_win->gizmo_shell);
	hd_win->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass,
		hd_win->gizmo_shell, GetGizmoWidget, "swin");
	DmSetSwinSize(hd_win->swin);

	file_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &FileMenu, NULL, 0);

	help_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &HelpMenu, NULL, 0);

	menu_bar = GetBaseWindowMenuBar(hd_win->gizmo_shell);
	SetSubMenuValue(menu_bar, file_menu, 0);
	SetSubMenuValue(menu_bar, help_menu, 1);


	CreateFileClass();
	/* allocate a container struct */
	if ((hd_win->views[0].cp =
	  (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL)
	{
		Dm__VaPrintMsg(TXT_MEM_ERR);
		FREE(hddp);
		FREE(hd_win);
		return(1);
	}
	hd_win->views[0].cp->path = strdup(hdpath);
	hd_win->views[0].cp->count = 1;

	DmReadDtInfo(hd_win->views[0].cp, hdpath, 0);
	DESKTOP_HELP_DESK(Desktop) = hd_win;

	if (hd_win->views[0].cp->num_objs > 0)
		UpdateObjs();

	hd_win->views[0].nitems =
		hd_win->views[0].cp->num_objs + EXTRA_ICON_SLOT;

	/* Create icon container	*/
	n = 0;
	XtSetArg(Dm__arg[n], XmNmovableIcons, False); n++;
	XtSetArg(Dm__arg[n], XmNdrawProc, DmDrawLinkIcon); n++;
	XtSetArg(Dm__arg[n], XmNmenuProc, DmIconMenuProc); n++;
	XtSetArg(Dm__arg[n], XmNpostSelectProc,DmHDSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNdblSelectProc, DmHDDblSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNpostUnselectProc, DmHDSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNclientData, hd_win); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight, GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth, GRID_WIDTH(Desktop) / 2);n++;
	XtSetArg(Dm__arg[n], XmNgridRows, 2 * FOLDER_ROWS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns, 2 * FOLDER_COLS(Desktop)); n++;

	hd_win->views[0].box = DmCreateIconContainer(hd_win->swin,
				DM_B_SPECIAL_NAME,
				Dm__arg, n, hd_win->views[0].cp->op,
				hd_win->views[0].cp->num_objs,
				&(hd_win->views[0].itp),
				hd_win->views[0].nitems,
				NULL);

	/* find out what current locale is */
	q = setlocale(LC_MESSAGES, NULL);
	DtSetProperty(&(hd_win->views[0].cp->plist), LOCALE, q, NULL);

	SetupIcons(True);

	XtRealizeWidget(hd_win->shell);
	XmAddWMProtocolCallback(hd_win->shell, 
		XA_WM_DELETE_WINDOW(XtDisplay(hd_win->shell)), WMCB,
		(XtPointer)hd_win) ;
	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(hd_win->shell, Dm__arg, 1);

	XtAddEventHandler(hd_win->shell, StructureNotifyMask, False,
		HandleResize, NULL);

	/* Get a help id for Help Desk to be used with its Help menu. */
	hddp->hap = (DmHelpAppPtr)DmNewHelpAppID(XtScreen(hd_win->shell),
				XtWindow(hd_win->shell), (char *)dtmgr_title,
				GetGizmoText(TXT_HELPDESK_TITLE),
				DESKTOP_NODE_NAME(Desktop),
				NULL, "hdesk32.icon"); 
	/* Register for context-sensitive help.  Since XmNhelpCallback is a
	 * MOTIF resource, register help on the child of the toplevel shell
	 * instead of the shell itself.
	 */
	cbs.index = 0;
	cbs.clientData = NULL;
	form = QueryGizmo(BaseWindowGizmoClass, hd_win->gizmo_shell,
		GetGizmoWidget, NULL);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)DmHDHelpCB,
		(XtPointer)&cbs);
	return(0);

} /* end of DmInitHelpDesk */

/****************************procedure*header*****************************
    DmHDExit - Called when a session terminates.
*/
void
DmHDExit()
{
	DmWinPtr window = (DmWinPtr)(DESKTOP_HELP_DESK(Desktop));

	if (DESKTOP_HELP_DESK(Desktop) == NULL)
		return;

     /* save x's and y's of icons in Help Desk */
     DmSaveXandY(window->views[0].itp, window->views[0].nitems);

     /* save window dimensions */
     DmWindowSize(window, Dm__buffer);
     DtSetProperty(&(window->views[0].cp->plist), FOLDERSIZE, Dm__buffer, NULL);

	/* flush window info to disk */
	DmFlushContainer(window->views[0].cp);

	DESKTOP_HELP_DESK(Desktop) = NULL;

} /* end of DmHDExit */

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
   CreateFileClass - This is only called once when Help Desk is initialized
   to create a new file class for icons in the Help Desk.
 */   
static void
CreateFileClass()
{
	static DmFnameKeyRec key;

	if ((hddp->fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec))) == NULL)
	{
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return;
	}
	hddp->fcp->key = (void *)&(key);
	DtSetProperty(&(hddp->fcp->plist), ICONFILE, DFLT_APP_ICON, NULL);
	DtSetProperty(&(hddp->fcp->plist), DFLTICONFILE, DFLT_APP_ICON, NULL);

} /* end of CreateFileClass */

/****************************procedure*header*****************************
    ExitCB - Callback for Exit button in File menu.
*/

static void
ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmUnmapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
} /* end of ExitCB */

/****************************procedure*header*****************************
    WMCB - Called when Quit is selected from Help Desk's window menu.
*/
static void
WMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmUnmapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));

} /* end of WMCB */


/****************************procedure*header*****************************
    UpdateObjs - Sets fcp and ftype of Help Desk objects.
*/
static void
UpdateObjs()
{
	DmObjectPtr op;
	
	for (op = DESKTOP_HELP_DESK(Desktop)->views[0].cp->op; op;
	  op = op->next)
	{
		op->fcp   = hddp->fcp;
		op->ftype = DM_FTYPE_DATA;

	}
} /* end of UpdateObjs */

/****************************procedure*header*****************************
    SetupIcons - If switching locale, retrieve icon label and one-line
    description from help file; otherwise, use existing information in
    /usr/X/desktop/Help_Desk.  If the ICON_LABEL property is not set,
    use the value of DFLT_ICONLABEL (i.e. American English label set in 
    DmAddAppToHD).  If DFLT_ICONLABEL is not set, use the application name
    as icon label.
*/
static void
SetupIcons(Boolean new_locale)
{
	int i;
	char *prop;
	char *descrp;
	char *label;
	char *dflt_label;
	char *fullpath;
	DmItemPtr itp;
	Widget iconbox;
	
	iconbox = DESKTOP_HELP_DESK(Desktop)->views[0].box;
	for (i = 0, itp = DESKTOP_HELP_DESK(Desktop)->views[0].itp;
	  i < DESKTOP_HELP_DESK(Desktop)->views[0].nitems; i++, itp++)
	{
	  if (ITEM_MANAGED(itp))
	  {
	    descrp = label = NULL;
	    /* Get default icon label, if set, to be used if no icon
	     * label is found in help file.
 	     */
	    dflt_label = DmGetObjProperty(ITEM_OBJ(itp), DFLT_ICONLABEL, NULL);
	    /* Read icon label and one-line description from its help file.  */
	    if (new_locale || DmGetObjProperty(ITEM_OBJ(itp), DESCRP, NULL)
	      == NULL || (label = DmGetObjProperty(ITEM_OBJ(itp), ICON_LABEL,
	      NULL)) == NULL)
	    {
  		prop = DmGetObjProperty(ITEM_OBJ(itp), HELP_FILE, NULL);
  		/* check if help file name is full path */
		if (prop) {
    			if (*prop == '/') {
				DmGetHDAppInfo(prop, &label, &descrp);
			} else {
				fullpath =
				  XtResolvePathname(DESKTOP_DISPLAY(Desktop),
				  "help", prop, NULL, NULL, NULL, 0, NULL);
				if (fullpath) {
				  DmGetHDAppInfo(fullpath, &label, &descrp);
				  FREE(fullpath);
				}
			}
	        }
	        if (descrp)
		    DmSetObjProperty(ITEM_OBJ(itp), DESCRP, descrp, NULL);
	        if (label) {
		    DmSetObjProperty(ITEM_OBJ(itp), ICON_LABEL, label, NULL);
		    MAKE_WRAPPED_LABEL(itp->label, FPART(iconbox).font, label);
	        } else if (dflt_label) {
		    MAKE_WRAPPED_LABEL(itp->label, FPART(iconbox).font, dflt_label);
	        } else {
		    /* set icon label to application name */
		    MAKE_WRAPPED_LABEL(itp->label, FPART(iconbox).font, ITEM_OBJ(itp)->name);
		}
	    } else {
		MAKE_WRAPPED_LABEL(itp->label, FPART(iconbox).font, label);
	    }
	    prop = DmGetObjProperty(ITEM_OBJ(itp), "_LINK", NULL);
	    if (prop)
		  ITEM_OBJ(itp)->attrs |= DM_B_SYMLINK;
	    DmSizeIcon(DESKTOP_HELP_DESK(Desktop)->views[0].box, itp);
	  }
	}
	XtSetArg(Dm__arg[0], XmNitemsTouched, True);
	XtSetValues(DESKTOP_HELP_DESK(Desktop)->views[0].box, Dm__arg, 1);

	DmSortItems(DESKTOP_HELP_DESK(Desktop), DM_BY_NAME, 0, 0, 0);

	/* Flush Help Desk database to disk, if any changes were made,
	 * to be reused if locale is not changed in the next session.
	 * If locale is changed in the same session or in the next session,
	 * this database will have to be recreated.
	 */ 
	DmWriteDtInfo(DESKTOP_HELP_DESK(Desktop)->views[0].cp,
		DESKTOP_HELP_DESK(Desktop)->views[0].cp->path, 0);

} /* end of SetupIcons */

/****************************procedure*header*****************************
 * HandleResize -
 */
static void
HandleResize(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use)
{
	if (xevent->type == ConfigureNotify)
		DmSortItems(DESKTOP_HELP_DESK(Desktop), DM_BY_NAME, 0, 0, 0);
} /* end of HandleResize */

/****************************procedure*header*****************************
 * DmSetHDMenuItem -
 */
void
DmSetHDMenuItem(MenuItems **ret_menu_items, char *name, int index,
	DmFolderWinPtr folder)
{
	switch(index) {
	case 0:
		*ret_menu_items = FileMenuItems;
		strcpy(name, "FileMenu");
		break;
	case 1:
		*ret_menu_items = HelpMenuItems;
		strcpy(name, "HelpMenu");
		break;
	default:
		break;
	}
} /* end of DmSetHDMenuItem */
