#pragma ident	"@(#)dtm:icon_setup.c	1.59"

/******************************file*header********************************

    Description:
     This file contains the source code for Icon Setup.
*/
		/* includes go here */

#include "icon_setup.h"

			/* private procedures           */
static void InitNewWinInfo();
static void FreeNewWin(WinInfoPtr wip);
static void ChangeViewPrompt();
static void SwitchViewCB(CB_PROTO);
static void SwitchView();
static void ResizeCB(Widget w, XtPointer client_data, XEvent *xevent,
		Boolean *continue_to_use);
static void HelpCB(CB_PROTO);
static void ViewCB(CB_PROTO);
static void ClassMenuCB(CB_PROTO);
static void NewMenuCB(CB_PROTO);
static void ConfirmDelete();
static void ConfirmDeleteCB(CB_PROTO);

			/* public procedures            */
void DmISHandleFontChanges();

	/* Define the menus and submenus for the menu bar.  */

static MenuItems ActionMenuItems[] = {
    MENU_ITEM( TXT_FILE_EXIT, TXT_M_Exit, DmExitIconSetup ),
    { NULL }
};

MENU("ActionMenu", ActionMenu);

static MenuItems NewMenuItems[] = {
    MENU_ITEM( TXT_FILE, TXT_M_FILE_TYPE, NULL ),
    MENU_ITEM( TXT_G_FOLDER_TITLE, TXT_M_Folder, NULL ),
    MENU_ITEM( TXT_APPLICATION, TXT_M_APP_TYPE, NULL ),
    { NULL }
};

static MenuGizmo NewMenu = {
	NULL, "NewMenu", "", NewMenuItems, NewMenuCB, NULL
};

static MenuItems ClassMenuItems[] = {
 { True, TXT_NEW, TXT_M_NEW, I_PUSH_BUTTON, &NewMenu, NULL, NULL, False },
 { False, TXT_FILE_DELETE, TXT_M_FILE_DELETE, I_PUSH_BUTTON, NULL, NULL, NULL, False },
 { True, "", "", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { False, TXT_FILE_PROP, TXT_M_Properties, I_PUSH_BUTTON, NULL, NULL, NULL, False },
 { NULL }
};

static MenuGizmo ClassMenu = {
	NULL, "ClassMenu", "", ClassMenuItems, ClassMenuCB, NULL
};

static MenuItems ViewMenuItems[] = {
 { False, TXT_PERSONAL_CLASSES, TXT_M_PERSONAL, I_PUSH_BUTTON, NULL, NULL, NULL, False },
 { True, TXT_SYSTEM_CLASSES, TXT_M_SYSTEM_CLASSES, I_PUSH_BUTTON, NULL, NULL, NULL, False },
 { NULL }
};

static MenuGizmo ViewMenu = {
	NULL, "ViewMenu", "", ViewMenuItems, ViewCB, NULL
};

static MenuItems HelpMenuItems[] = {
    MENU_ITEM( TXT_Icon_Setup, TXT_M_IB_TITLE, NULL ),
    MENU_ITEM( TXT_HELP_M_AND_K, TXT_M_HELP_M_AND_K, NULL ),
    MENU_ITEM( TXT_HELP_TOC, TXT_M_HELP_TOC, NULL ),
    MENU_ITEM( TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, NULL ),
    { NULL }
};

static MenuGizmo HelpMenu = {
	NULL, "HelpMenu", "", HelpMenuItems, HelpCB, NULL
};

static MenuItems MenuBarItems[] = {
    MENU_ITEM( TXT_ACTION, TXT_M_ACTION, NULL ),
    MENU_ITEM( TXT_ICONCLASS, TXT_M_CLASS, NULL ),
    MENU_ITEM( TXT_VIEW, TXT_M_VIEW, NULL ),
    MENU_ITEM( TXT_HELP, TXT_M_HELP, NULL ),
    { NULL }
};

MENU_BAR("menubar", MenuBar, NULL, 0, 0);

static ContainerGizmo swin_gizmo = {
	NULL,			/* help */
	"swin",			/* name */
	G_CONTAINER_SW,		/* container type */
};

static GizmoRec gizmos[] = {
     {ContainerGizmoClass, &swin_gizmo},
};

static MsgGizmo msgGizmo = {NULL, "footer", " ", " "};

static BaseWindowGizmo BaseWindow = {
	NULL,			/* help */
	"IconSetup",		/* shell widget name */
	TXT_IB_TITLE,		/* title */
	&MenuBar,		/* menu bar */
	gizmos,			/* gizmo array */
	XtNumber(gizmos),	/* # of gizmos in array */
	&msgGizmo,		/* footer */
	TXT_IB_TITLE, 		/* icon_name */
	"binder48.icon",     	/* name of pixmap file */
};

/* gizmos for confirmation prompts */
static MenuItems viewChangeMenuItems[] = {
 { True, TXT_YES,  TXT_M_YES,  I_PUSH_BUTTON, NULL, SwitchViewCB,(XtPointer)0},
 { True, TXT_NO,   TXT_M_NO,   I_PUSH_BUTTON, NULL, SwitchViewCB,(XtPointer)1},
 { True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, NULL, SwitchViewCB,(XtPointer)2},
 { NULL }
};

MENU_BAR( "viewChangeMenu", viewChangeMenu, NULL, 0, 0 );

static ModalGizmo viewChangeGizmo = {
	NULL,					/* help info */
	"viewChangeGizmo",			/* shell name */
	TXT_ICON_SETUP_SWITCH_VIEW,		/* title */
	&viewChangeMenu,			/* menu */
	TXT_SAVE_CHANGES,			/* message */
	NULL,					/* gizmos */
	0,					/* num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
	XmDIALOG_QUESTION,			/* type */
};

static MenuItems confirmDeleteMenuItems[] = {
 { True, TXT_YES,  TXT_M_YES, I_PUSH_BUTTON,NULL,ConfirmDeleteCB,(XtPointer)0},
 { True, TXT_NO,   TXT_M_NO,  I_PUSH_BUTTON,NULL,ConfirmDeleteCB,(XtPointer)1},
 { True, TXT_HELP, TXT_M_HELP,I_PUSH_BUTTON,NULL,ConfirmDeleteCB,(XtPointer)2},
 { NULL }
};

MENU_BAR( "confirmDeleteMenu", confirmDeleteMenu, NULL, 1, 1 );

static ModalGizmo confirmDeleteGizmo = {
	NULL,					/* help info */
	"confirmDeleteGizmo",			/* shell name */
	TXT_ICON_SETUP_DELETE_CLASS,		/* title */
	&confirmDeleteMenu,			/* menu */
	NULL,					/* message */
	NULL,					/* gizmos */
	0,					/* num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL,	/* style */
	XmDIALOG_QUESTION,			/* type */
};

	/* global variables */

DmFolderWindow base_fwp = NULL;
char *dflt_print_cmd    = NULL;
char *systemLibraryPath = NULL;

Widget NewFileShell   = NULL;
Widget NewFolderShell = NULL;
Widget NewAppShell    = NULL;

char **FileProgTypes = NULL;
char **AppProgTypes  = NULL;

Atom targets[1];

	/* static variables */

static DmHelpInfoRec helpInfo;
static WinInfo FileWinInfo;
static WinInfo FolderWinInfo;
static WinInfo AppWinInfo;

static Gizmo confirmDeletePrompt = NULL;
static Gizmo viewChangePrompt    = NULL;

/***************************public*procedures****************************

    Public Procedures
*/
/****************************procedure*header*****************************
 * DmInitIconSetup - Performs initialization of global variables and
 * creates the Icon Setup base window.  Also registers necessary callbacks
 * and event handlers.
 */
unsigned int
DmInitIconSetup(char *geom_str, Boolean iconic, Boolean map_window)
{
	int n;
	char *p;
	Gizmo action_menu;
	Gizmo class_menu;
	Gizmo view_menu;
	Gizmo help_menu;
	Gizmo menu_bar;
	Arg args[10];
	DmItemPtr ip;
	Widget form;
	Widget iconbox;
	static MenuGizmoCallbackStruct cbs;
	char buf[PATH_MAX];

	/* If Icon Setup is already initialized, just map it and return. */
	if (DESKTOP_ICON_SETUP_WIN(Desktop)) {
		DmMapWindow((DmWinPtr)DESKTOP_ICON_SETUP_WIN(Desktop));
		return(0);
	}
	/* Initialize drag-and-drop targets. */
	targets[0] = OL_XA_FILE_NAME(DESKTOP_DISPLAY(Desktop));

	/* Initialize system icon library path. */
	if (realpath(PlanesOfScreen(DESKTOP_SCREEN(Desktop)) > 1 ?
		DFLT_PIXMAP_LIBRARY_PATH : DFLT_BITMAP_LIBRARY_PATH, buf))
			systemLibraryPath = STRDUP(buf);

	/* Get desktop-wide default print command. */
	dflt_print_cmd = DmGetDTProperty(DFLTPRINTCMD, NULL);

	/* Initialize settings for New windows. */
	InitNewWinInfo();

	/* Determine what program types are available and set up
	 * menu items for Program Types menu.
	 */
	DmISGetProgTypes();

	/* Allocate a folder window to be used for the base window.  */
	base_fwp = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
	DESKTOP_ICON_SETUP_WIN(Desktop) = base_fwp;
	base_fwp->attrs = DM_B_ICON_SETUP_WIN | DM_B_BASE_WINDOW |
				DM_B_PERSONAL_CLASSES;
	base_fwp->views[0].view_type = DM_ICONIC;
	base_fwp->views[0].sort_type = DM_BY_POSITION;

	/* Set up resources for Gizmos. */
	n = 0;
	XtSetArg(args[n], XmNshadowThickness, 2); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, 2); n++;
	XtSetArg(args[n], XmNrightOffset, 2); n++;
	XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
	XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
	gizmos->args = args;
	gizmos->numArgs = n;

	ClassMenu.clientData = (XtPointer)base_fwp;
	NewMenu.clientData   = (XtPointer)base_fwp;
	ViewMenu.clientData  = (XtPointer)base_fwp;

	XtSetArg(Dm__arg[0], XtNiconic, iconic);
	XtSetArg(Dm__arg[1], XtNgeometry, geom_str);
	XtSetArg(Dm__arg[2], XtNmappedWhenManaged, map_window);

	base_fwp->gizmo_shell = CreateGizmo(NULL, BaseWindowGizmoClass,
			&BaseWindow, Dm__arg, 3);

	/* Set up menu bar. */ 
	action_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &ActionMenu, NULL, 0);
	class_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &ClassMenu, NULL, 0);
	view_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &ViewMenu, NULL, 0);
	help_menu = CreateGizmo(DESKTOP_SHELL(Desktop),
		PulldownMenuGizmoClass, &HelpMenu, NULL, 0);

	menu_bar = GetBaseWindowMenuBar(base_fwp->gizmo_shell);
	SetSubMenuValue(menu_bar, action_menu, 0);
	SetSubMenuValue(menu_bar, class_menu, 1);
	SetSubMenuValue(menu_bar, view_menu, 2);
	SetSubMenuValue(menu_bar, help_menu, 3);

	base_fwp->shell = GetBaseWindowShell(base_fwp->gizmo_shell);
	base_fwp->swin = (Widget)QueryGizmo(BaseWindowGizmoClass,
		base_fwp->gizmo_shell, GetGizmoWidget, "swin");
	DmSetSwinSize(base_fwp->swin);

	/* Allocate a container for Icon Setup.  Can't use DmOpenDir()
	 * here because Icon Setup is not a folder for a directory.
	 */ 
	if ((base_fwp->views[0].cp = (DmContainerPtr)CALLOC(1,
		sizeof(DmContainerRec))) == NULL)
	{
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return(-1);
	}
	/* Initialize container and set win->cp->path to a bogus path. */
	base_fwp->views[0].cp->path = NULL;
	base_fwp->views[0].cp->count = 1;

	/* Extract the classes to be displayed. */
	DmISExtractClass(base_fwp->attrs);

	base_fwp->views[0].nitems = base_fwp->views[0].cp->num_objs;

	/* Create an icon container (flat icon box). */
	n = 0;
	XtSetArg(Dm__arg[n], XmNclientData, base_fwp); n++; /* for MenuProc */
	XtSetArg(Dm__arg[n], XmNdrawProc, DmDrawIcon); n++;
	XtSetArg(Dm__arg[n], XmNmovableIcons, False); n++;
	XtSetArg(Dm__arg[n], XmNexclusives, True); n++;
	XtSetArg(Dm__arg[n], XmNnoneSet, True); n++;
	XtSetArg(Dm__arg[n], XmNunselectProc, DmISUnSelectCB); n++;
	XtSetArg(Dm__arg[n], XmNselectProc, DmISSelectCB); n++;
	XtSetArg(Dm__arg[n], XmNdblSelectProc, DmISDblSelectCB); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight, GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth, GRID_WIDTH(Desktop) / 2);n++;
	XtSetArg(Dm__arg[n], XmNgridRows, 2 * FOLDER_ROWS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns, 2 * FOLDER_COLS(Desktop)); n++;

	iconbox = base_fwp->views[0].box = DmCreateIconContainer(base_fwp->swin,
		DM_B_SPECIAL_NAME,
		Dm__arg, n,
		base_fwp->views[0].cp->op,
		base_fwp->views[0].cp->num_objs,
		&(base_fwp->views[0].itp),
		base_fwp->views[0].nitems,
		NULL);

	/* Label each icon using its CLASSNAME property or CLASS value. */
	for (n = 0, ip = base_fwp->views[0].itp; n < base_fwp->views[0].nitems;
		n++,ip++)
	{
		if (ITEM_MANAGED(ip))
		{
			if (p = DtGetProperty(&(ITEM_OBJ(ip)->fcp->plist),
				CLASS_NAME, NULL)) {
				MAKE_WRAPPED_LABEL(ip->label, FPART(iconbox).font, GGT(p));
			} else {
				MAKE_WRAPPED_LABEL(ip->label, FPART(iconbox).font,
					ITEM_CLASS(ip)->name);
			}
		}
	}
	DmTouchIconBox((DmWinPtr)base_fwp, NULL, 0);

	DmISAlignIcons(base_fwp);

	XtSetArg(Dm__arg[0], XtNtitle, GGT(TXT_ICON_SETUP_PERSONAL_CLASSES));
	XtSetValues(base_fwp->shell, Dm__arg, 1);

	XtRealizeWidget(base_fwp->shell);

	/* Register callback to destroy Icon Setup when it is closed. */ 
	XmAddWMProtocolCallback(base_fwp->shell, 
		XA_WM_DELETE_WINDOW(XtDisplay(base_fwp->shell)),
		DmExitIconSetup, (XtPointer)base_fwp) ;

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(base_fwp->shell, Dm__arg, 1);

	/* Register interest in structure changes and event handler to realign
	 * icons when Icon Setup window is resized.  Note that icons in the
	 * Icon Setup window are not movable.
	 */
	XtAddEventHandler(base_fwp->shell, StructureNotifyMask, False,
		(XtEventHandler)ResizeCB, NULL);

	/* register Icon Setup for help */
	ICON_SETUP_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(base_fwp->shell),
		XtWindow(base_fwp->shell), (char *)dtmgr_title,
		Dm__gettxt(TXT_ICON_SETUP), DESKTOP_NODE_NAME(Desktop),
		NULL, "binder.icon")->app_id;

	/* initialize help info for switch view and delete class modals */
	helpInfo.app_id  = ICON_SETUP_HELP_ID(Desktop);
	helpInfo.file    = STRDUP(ICON_SETUP_HELPFILE);
	helpInfo.section = NULL;

	/* Register for context-sensitive help.  Since XmNhelpCallback is a
	 * MOTIF resource, register help on the child of the toplevel shell
	 * instead of the shell itself.  Handle this by simulating the
	 * selection of the first button in the Help menu.
	 */
	cbs.index = 0;
	cbs.clientData = NULL;
	form = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, NULL);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)HelpCB,
		(XtPointer)&cbs);
	return(0);

} /* end of DmInitIconSetup */

/****************************procedure*header*****************************
 * DmExitIconSetup - Called when Actions:Exit is selected, when Close
 * is selected from Icon Setup's window menu, or when the desktop is
 * exited while Icon Setup is up.  Frees resources allocated for all
 * existing windows and destroys those windows.
 */
void
DmExitIconSetup()
{
	int i;
	DmItemPtr ip;
	WinInfoPtr wip;

	if (DESKTOP_ICON_SETUP_WIN(Desktop) == NULL)
		return;

	/* Destroy any Properties window that are currently up.  Note that
	 * DestroyPropWin() is called when a Properties window is popped down.
	 */
	for (i = 0, ip = base_fwp->views[0].itp; i < base_fwp->views[0].nitems;
		i++,ip++)
	{
		if (ITEM_MANAGED(ip) && ITEM_OBJ(ip)->objectdata) {
			/* wip is freed in DestroyPropWin() */
			wip = (WinInfoPtr)(ITEM_OBJ(ip)->objectdata);
			XtPopdown(wip->w[W_Shell]);
		}
	}
	/* Free resources allocated for New windows. */
	if (NewFileShell) {
		FreeNewWin(&FileWinInfo);
		NewFileShell = NULL;
	}
	if (NewFolderShell) {
		FreeNewWin(&FolderWinInfo);
		NewFolderShell = NULL;
	}
	if (NewAppShell) {
		FreeNewWin(&AppWinInfo);
		NewAppShell = NULL;
	}
	if (confirmDeletePrompt) {
		FreeGizmo(ModalGizmoClass, confirmDeletePrompt);
		confirmDeletePrompt = NULL;
	}

	if (viewChangePrompt) {
		FreeGizmo(ModalGizmoClass, viewChangePrompt);
		viewChangePrompt = NULL;
	}

	/* Can't call DmCloseContainer() for Icon Setup base window because
	 * it's not a regular folder window - just free the objects and 
	 * container.
	 */
	DmISFreeContainer(base_fwp->views[0].cp);

	if (helpInfo.file) {
		FREE(helpInfo.file);
		helpInfo.file = NULL;
	}
	if (helpInfo.section) {
		FREE(helpInfo.section);
		helpInfo.section = NULL;
	}

	/* Destroy base window shell and free item list. */
	DmDestroyIconContainer(base_fwp->shell, base_fwp->views[0].box,
		base_fwp->views[0].itp, base_fwp->views[0].nitems);

	/* Free "folder" window and reset base_fwp to NULL. */
	FREE(base_fwp);
	base_fwp = DESKTOP_ICON_SETUP_WIN(Desktop) = NULL;

	if (systemLibraryPath) {
		FREE(systemLibraryPath);
		systemLibraryPath = NULL;
	}

} /* end of DmExitIconSetup */

/***************************private*procedures****************************

    Private Procedures
*/
/****************************procedure*header*****************************
 * InitNewWinInfo - Initializes settings for New windows.
 */
static void
InitNewWinInfo()
{
	FileWinInfo.new_class   = True;
	FolderWinInfo.new_class = True;
	AppWinInfo.new_class    = True;
	
	FileWinInfo.class_type   = FileType;
	FolderWinInfo.class_type = DirType;
	AppWinInfo.class_type    = AppType;

} /* end of InitNewWinInfo */

/****************************procedure*header*****************************
 * ResizeCB - Callback for StructureNotify on Icon Setup base window.
 * Realigns icons if window was resized.
 */
static void
ResizeCB(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use)
{
	if (xevent->type == ConfigureNotify)
		DmISAlignIcons(base_fwp);

} /* end of ResizeCB */

/****************************procedure*header*****************************
 * NewMenuCB - Callback for Icon Class:New.  Creates/re-map New window
 * for the selected type.
 */
static void
NewMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget btn;
	Boolean ignored;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

	switch(cbs->index) {
	case 0: /* File Type */
		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "NewMenu:0");
		NewFileShell = DmISCreateFileWin(NULL, &FileWinInfo, &ignored);
		break;
	case 1: /* Folder Type */
		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "NewMenu:1");
		NewFolderShell = DmISCreateFolderWin(NULL, &FolderWinInfo,
			&ignored);
		break;
	case 2: /* Application Type */
		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "NewMenu:2");
		NewAppShell = DmISCreateAppWin(NULL, &AppWinInfo, &ignored);
		break;
	}
	/* Make menu button for selected type insensitive because there is
	 * only one New window for each type.
	 */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	XtSetValues(btn, Dm__arg, 1);

} /* end of NewMenuCB */

/****************************procedure*header*****************************
 * ConfirmDelete - Displays a confirmation modal to delete a file class.
 */
static void
ConfirmDelete(DmItemPtr ip)
{
	XmString msg;

	/* Set up message with the name of the selected file class. */
	sprintf(Dm__buffer, GGT(TXT_REALLY_DELETE_CLASS), ITEM_OBJ(ip)->name);
	msg = XmStringCreateLocalized(Dm__buffer);

	XtSetArg(Dm__arg[0], XmNmessageString, msg);
	XtSetArg(Dm__arg[1], XmNuserData, (XtPointer)ip);

	if (confirmDeletePrompt == NULL) {
		confirmDeletePrompt = CreateGizmo(base_fwp->shell,
			ModalGizmoClass, &confirmDeleteGizmo, Dm__arg, 2);
		XtAddCallback(GetModalGizmoDialogBox(confirmDeletePrompt),
			XmNhelpCallback, DmPopupWinHelpKeyCB, &helpInfo);
	} else {
		XtSetValues(GetModalGizmoDialogBox(confirmDeletePrompt),
			Dm__arg, 2);
	}
	XmStringFree(msg);
	if (helpInfo.section)
		FREE(helpInfo.section);
	helpInfo.section = STRDUP(ICON_SETUP_DELETE_CLASS_SECT);
	DmBeep();
	MapGizmo(ModalGizmoClass, confirmDeletePrompt);
	XRaiseWindow(DESKTOP_DISPLAY(Desktop),
		XtWindow(GetModalGizmoShell(confirmDeletePrompt)));

} /* end of ConfirmDelete  */

/****************************procedure*header*****************************
 * ConfirmDeleteCB - Callback for menu in delete class confirmation modal.
 * Calls DeleteClass(), just pop down the modal or display help on the modal.
 */
static void
ConfirmDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	index = (int)client_data;

	switch(index) {
	case 0: /* Yes */
	{
		DmItemPtr ip;

		XtPopdown(GetModalGizmoShell(confirmDeletePrompt));
		XtSetArg(Dm__arg[0], XmNuserData, &ip);
		XtGetValues(GetModalGizmoDialogBox(confirmDeletePrompt),
			Dm__arg, 1);
		DmISDeleteClass(ip);
	}
		break;
	case 1: /* No */
		XtPopdown(GetModalGizmoShell(confirmDeletePrompt));
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
		  NULL, ICON_SETUP_HELPFILE, ICON_SETUP_DELETE_CLASS_SECT);
		break;
	}

} /* end of ConfirmDeleteCB */

/****************************procedure*header*****************************
 * DmISShowProperties - Called when a file class is double-clicked on or when
 * Icon Class:Properties is selected to display a Properties window for
 * the selected class.  Can only display classes of types DATA, DIR and
 * EXEC.
 */
void
DmISShowProperties(DmItemPtr ip)
{
	char *p;
	WinInfoPtr wip;
	DmObjectPtr op = ITEM_OBJ(ip);
	Boolean bad_icon_file = False;

	p = DtGetProperty(&(op->fcp->plist), FILETYPE, NULL);

	/* Deactivate Delete and Properties buttons in Class menu. */
	DmISUnSelectCB(NULL, NULL, NULL);

	wip = (WinInfoPtr)CALLOC(1, sizeof(WinInfo));
	if (!strcmp(p, "DIR")) {
		wip->class_type = DirType;
		(void)DmISCreateFolderWin(op, wip, &bad_icon_file);
	} else if (!strcmp(p, "EXEC")) {
		wip->class_type = AppType;
		(void)DmISCreateAppWin(op, wip, &bad_icon_file);
	} else {
		/* Display properties of classes of undefined FILETYPE or of
		 * FILETYPE UNK in a DATA type window.
	 	 */
		if (strcmp(p, "DATA"))
			wip->no_ftype = True;
		wip->class_type = FileType;
		(void)DmISCreateFileWin(op, wip, &bad_icon_file);
	}
	if (bad_icon_file) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			TXT_INVALID_ICON_FILE, OLD_VAL(icon_file));
	}

} /* end of DmISShowProperties */

/****************************procedure*header*****************************
 * ViewCB - Callback for View menu.  Checks if any New or Properties window
 * is currently up.  If so, ask the user if s/he wants to save any unsaved
 * changes; otherwise, just switch the view.
 */
static void
ViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int i;
	DmItemPtr ip;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	Boolean prompt = False;

	/* clear footer */
	SetBaseWindowLeftMsg(base_fwp->gizmo_shell, " ");

	/* check if any New window for current view is up */
	if (NewFileShell &&
		GetWMState(DESKTOP_DISPLAY(Desktop), XtWindow(NewFileShell))
		== NormalState)
	{
		prompt = True;
	} else if (NewFolderShell &&
		GetWMState(DESKTOP_DISPLAY(Desktop), XtWindow(NewFolderShell))
		== NormalState)
	{
		prompt = True;
	} else if (NewAppShell &&
		GetWMState(DESKTOP_DISPLAY(Desktop), XtWindow(NewAppShell))
		== NormalState)
	{
		prompt = True;
	}
	if (!prompt) {
		/* check if any Properties window for current view is up */
		for (i = 0, ip = base_fwp->views[0].itp;
			i < base_fwp->views[0].nitems; i++,ip++)
		{
			/* If object's objectdata is not NULL, a Properties
			 * window currently exists for it -> objectdata is wip.
			 */
			if (ITEM_MANAGED(ip) && ITEM_OBJ(ip)->objectdata) {
				prompt = True;
				break;
			}
		}
	}
	if (prompt)
		ChangeViewPrompt();
	else
		SwitchView();

} /* end of ViewCB */

/****************************procedure*header*****************************
 * ChangeViewPrompt - Creates and displays a confirmation modal to ask user
 * if any unsaved changes should be saved before switching view.  Called
 * from ViewCB().
 */
static void
ChangeViewPrompt()
{
	if (viewChangePrompt == NULL) {
		viewChangePrompt = CreateGizmo(base_fwp->shell,
			ModalGizmoClass, &viewChangeGizmo, NULL, 0);
		XtAddCallback(GetModalGizmoDialogBox(viewChangePrompt),
			XmNhelpCallback, DmPopupWinHelpKeyCB, &helpInfo);
	}
	if (helpInfo.section)
		FREE(helpInfo.section);
	helpInfo.section = STRDUP(ICON_SETUP_SWITCH_VIEW_SECT);
	DmBeep();
	MapGizmo(ModalGizmoClass, viewChangePrompt);
	XRaiseWindow(DESKTOP_DISPLAY(Desktop),
		XtWindow(GetModalGizmoShell(viewChangePrompt)));

} /* end of ChangeViewPrompt  */

/****************************procedure*header*****************************
 * SwitchViewCB - Callback for switch view confirmation modal to change
 * view, display help on the modal or do nothing.
 */
static void
SwitchViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int index = (int)client_data;

	switch(index) {
	case 0: /* Yes - just pop down modal */ 
		XtPopdown(GetModalGizmoShell(viewChangePrompt));
		break;
	case 1: /* No - switch view */
	{
		int i;
		DmItemPtr ip;
		WinInfoPtr wip;

		XtPopdown(GetModalGizmoShell(viewChangePrompt));

		/* Destroy any Properties window and dismiss any New windows
		 * before switching view.
		 */
		for (i = 0, ip = base_fwp->views[0].itp;
			i < base_fwp->views[0].nitems; i++, ip++)
		{
			if (ITEM_MANAGED(ip) && ITEM_OBJ(ip)->objectdata) {
				/* wip is freed in Properties window's 
				 * popdownCallback.
				 */
				wip = (WinInfoPtr)(ITEM_OBJ(ip)->objectdata);
				XtPopdown(wip->w[W_Shell]);
				ITEM_OBJ(ip)->objectdata = NULL;
			}
		}
		if (NewFileShell)
			XtPopdown(NewFileShell);
		if (NewFolderShell)
			XtPopdown(NewFolderShell);
		if (NewAppShell)
			XtPopdown(NewAppShell);
		SwitchView();
	}
		break;
	case 2: /* Help - display help on the modal */
		DmDisplayHelpSection(DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
			NULL,ICON_SETUP_HELPFILE, ICON_SETUP_SWITCH_VIEW_SECT);
		break;
	}

} /* end of SwitchViewCB */

/****************************procedure*header*****************************
 * SwitchView - Called from SwitchViewCB() or ViewCB() to switch view from
 * from Personal Classes to System Classes, or vice-versa.  If switching to
 * System Classes, checks if the user has permission to modify system classes
 * and update sensitivity of Icon Class:New button.
 */
static void
SwitchView()
{
	int i;
	char *p;
	DmItemPtr ip;
	DmObjectPtr op;
	DmObjectPtr save;
	Widget btn;
	Boolean allowed = True;
	Widget iconbox;

	iconbox = base_fwp->views[0].box;

	/* clear footer */
	SetBaseWindowLeftMsg(base_fwp->gizmo_shell, " ");
	SetBaseWindowRightMsg(base_fwp->gizmo_shell, " ");

	BUSY_DESKTOP(base_fwp->shell);

	/* Free current object list and reuse the same container.  The current
	 * item labels will be freed and the item list realloced in
	 * DmResetIconContainer.
	 */
	for (op = base_fwp->views[0].cp->op; op;) {
		save = op->next;
		Dm__FreeObject(op);
		op = save;
	}
	base_fwp->views[0].cp->op = NULL;
	base_fwp->views[0].cp->num_objs = 0;

	/* If current view is Personal Classes, then switch to System Classes,
	 * and vice-versa.
	 */
	DmISExtractClass(base_fwp->attrs & DM_B_PERSONAL_CLASSES ?
		DM_B_SYSTEM_CLASSES : DM_B_PERSONAL_CLASSES);

	/* Item list is realloced in DmResetIconContainer(). */
	DmResetIconContainer(base_fwp->views[0].box, DM_B_SPECIAL_NAME,
		base_fwp->views[0].cp->op, base_fwp->views[0].cp->num_objs,
		&(base_fwp->views[0].itp),
		&(base_fwp->views[0].nitems), 0);

	/* Label items using CLASSNAME property or CLASS value. */
	for (i = 0, ip = base_fwp->views[0].itp; i < base_fwp->views[0].nitems;
		i++, ip++)
	{
		if (ITEM_MANAGED(ip))
		{
			if (p = DtGetProperty(&(ITEM_OBJ(ip)->fcp->plist),
				CLASS_NAME,NULL))
			{
				MAKE_WRAPPED_LABEL(ip->label, FPART(iconbox).font, GGT(p));
			} else {
				MAKE_WRAPPED_LABEL(ip->label, FPART(iconbox).font,
					ITEM_CLASS(ip)->name);
			}
		}
	}
	DmTouchIconBox((DmWinPtr)base_fwp, NULL, 0);
	DmISAlignIcons(base_fwp);

	/* Update window attributes to keep track of which view we're in and
	 * sensitivity of View menu items.
	 */
	if (base_fwp->attrs & DM_B_SYSTEM_CLASSES) {
		base_fwp->attrs &= ~DM_B_SYSTEM_CLASSES;
		base_fwp->attrs |= DM_B_PERSONAL_CLASSES;

		XtSetArg(Dm__arg[0], XtNtitle,
			GGT(TXT_ICON_SETUP_PERSONAL_CLASSES));
		XtSetValues(base_fwp->shell, Dm__arg, 1);

		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "ViewMenu:0");
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn, Dm__arg, 1);
		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "ViewMenu:1");
		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn, Dm__arg, 1);
	} else {
		base_fwp->attrs &= ~DM_B_PERSONAL_CLASSES;
		base_fwp->attrs |= DM_B_SYSTEM_CLASSES;

		XtSetArg(Dm__arg[0], XtNtitle,
			GGT(TXT_ICON_SETUP_SYSTEM_CLASSES));
		XtSetValues(base_fwp->shell, Dm__arg, 1);

		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "ViewMenu:1");
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn, Dm__arg, 1);
		btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, "ViewMenu:0");
		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn, Dm__arg, 1);
	}

	/* Deactivate or activate Icon Class:New button depending on whether
	 * the user has permission to modify system file classes.
	 */
	if (base_fwp->attrs & DM_B_SYSTEM_CLASSES) {
		/* In UW 2.0, assume system owner has permission to modify
		 * the system file class database.
		 */
		allowed = PRIVILEGED_USER();
	}
	SetBaseWindowRightMsg(base_fwp->gizmo_shell, allowed ?
		" " : TXT_READ_ONLY);

	XtSetArg(Dm__arg[0], XmNsensitive, allowed);
	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:0");
	XtSetValues(btn, Dm__arg, 1);

	/* deactivate Delete and Properties button */
	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:1");
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:3");
	XtSetValues(btn, Dm__arg, 1);

} /* end of SwitchView */

/****************************procedure*header*****************************
 * HelpCB - Callback for Help menu in base window.
 */
static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpAppPtr help_app = DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop));

	/* clear footer */
	SetBaseWindowLeftMsg(base_fwp->gizmo_shell, " ");

	switch(cbs->index) {
	case 0: /* Icon Setup */
		DmDisplayHelpSection(help_app, NULL, ICON_SETUP_HELPFILE,
			ICON_SETUP_INTRO_SECT);
		break;
	case 1: /* Mouse and Keyboard */
		DmDisplayHelpSection(help_app, NULL, DESKTOP_HELPFILE,
			MOUSE_AND_KEYBOARD_SECT);
		break;
	case 2: /* Table of Contents */
		DmDisplayHelpTOC(w, &(help_app->hlp_win), ICON_SETUP_HELPFILE,
			help_app->app_id);
		break;
	case 3: /* Help Desk */
		DmHelpDeskCB(NULL, NULL, NULL);
		break;
	}
} /* end of HelpCB */

/****************************procedure*header*****************************
 * ClassMenuCB - Callback for Icon Class menu.  Calls ConfirmDelete()
 * and DmISShowProperties() when Delete and Properties is selected.
 */
static void
ClassMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmFolderWinPtr win = (DmFolderWinPtr)(cbs->clientData);

	/* clear footer */
	SetBaseWindowLeftMsg(base_fwp->gizmo_shell, " ");

	switch(cbs->index) {
	case 0: /* New */
		/* do nothing */
		break;
	case 1: /* Delete */
	{
		int idx;

		XtSetArg(Dm__arg[0], XmNlastSelectItem, &idx);
		XtGetValues(base_fwp->views[0].box, Dm__arg, 1);
		ConfirmDelete(base_fwp->views[0].itp + idx);
	}
		break;
	case 2: /* Separator */
		break;
	case 3: /* Properties */
	{
		int idx;

		XtSetArg(Dm__arg[0], XmNlastSelectItem, &idx);
		XtGetValues(base_fwp->views[0].box, Dm__arg, 1);

		DmISShowProperties(base_fwp->views[0].itp + idx);
	}
		break;
	}
} /* end of ClassMenuCB */

/****************************procedure*header*****************************
 * DisplayTemplates: Parses the value of the TEMPLATE file class property
 * and displays the template(s) in a flat list widget.  The value can
 * comprise one or a list of templates.
 */
void
DisplayTemplates(char **tmpl_list, int nitems, Widget listW)
{
	int i;
	XmString items[128];

	if (tmpl_list == NULL)
		return;

	for (i = 0; i < nitems; i++) {
		items[i] = XmStringCreateLocalized(tmpl_list[i]);
	}

	XtSetArg(Dm__arg[0], XmNitems, items);
	XtSetArg(Dm__arg[1], XmNitemCount, nitems);
	XtSetValues(listW, Dm__arg, 2);

	for (i = 0; i < nitems; i++)
		XmStringFree(items[i]);

} /* end of DisplayTemplates */

/****************************procedure*header*****************************
 * DmISHandleFontChanges - Called when font is changed to resize and relayout
 * items in the main window and icon library of New and Properties windows.
 * Note that view is always DM_ICONIC in all those windows.
 */
void
DmISHandleFontChanges()
{
	int i;
	DmItemPtr itp;

	/* Resize and relayout items in main window based on the new font. */
	DmISAlignIcons(base_fwp);

	/* Assume that the same font is used in the main window and icon
	 * library of New and Properties windows.
 	 */

	/* Do the same for all icon library windows in New windows. */
	if (NewFileShell && FileWinInfo.lib_fwp)
		DmISAlignIcons(FileWinInfo.lib_fwp);

	if (NewFolderShell && FolderWinInfo.lib_fwp)
		DmISAlignIcons(FolderWinInfo.lib_fwp);

	if (NewAppShell && AppWinInfo.lib_fwp)
		DmISAlignIcons(AppWinInfo.lib_fwp);

	/* Do the same for all icon library windows in Properties windows. */
	for (i = 0, itp = base_fwp->views[0].itp;
	  i < base_fwp->views[0].nitems; i++, itp++)
		if (ITEM_MANAGED(itp) && ITEM_OBJ(itp)->objectdata)
		  DmISAlignIcons(
			((WinInfoPtr)(ITEM_OBJ(itp)->objectdata))->lib_fwp);

} /* end of DmISHandleFontChanges */

/****************************procedure*header*****************************
 * FreeNewWin - Free resources dynamically allocated for a New window.
 */
static void
FreeNewWin(WinInfoPtr wip)
{
	BringDownPopup(GetPopupGizmoShell(wip->popupGizmo));

	FREE(wip->hip->file);
	FREE(wip->hip->section);
	FREE(wip->hip);

	if (wip->mip)
		FREE(wip->mip);

	FreeGizmo(PopupGizmoClass, wip->popupGizmo);

} /* end of FreeNewWin */

