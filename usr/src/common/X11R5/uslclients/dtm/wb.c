#pragma ident	"@(#)dtm:wb.c	1.167"

/******************************file*header********************************

    Description:
     This file contains the source code for initializing the wastebasket,
	and functions to relabel the icons in it by version and setting up
	a wastebasket file class.
*/
                              /* #includes go here     */

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>

#include <Dt/Desktop.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>             /* for _XmFindTopMostShell */
#include <Xm/Protocols.h>       /* for XmAddWMProtocolCallback */

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
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
                         /* private procedures         */

static void	InitWBData();
static void	WMCB(Widget, XtPointer, XtPointer);
static void	ExitWBCB(Widget, XtPointer, XtPointer);
static void	WBEventHandler(Widget, XtPointer, XEvent *, Boolean *);
static void	CreateWBFileClass();
static void	UpdateWBObjs();

/* Define the menus and submenus for the wastebasket menu bar.  */

#define XA_WM_STATE(d)          XInternAtom((d), "WM_STATE", False)

#define XA XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE

static MenuItems ActionMenuItems[] = {
 { False, TXT_WB_TIMER_ON, TXT_M_WB_TIMER_ON, I_PUSH_BUTTON, NULL,
	DmWBTimerCB,      B_A, False },
 { True,  TXT_OPTIONS,     TXT_M_OPTIONS,     I_PUSH_BUTTON, NULL,
	DmWBPropCB,       B_A, True },
 { True,  TXT_WB_EMPTY,    TXT_M_WB_EMPTY,    I_PUSH_BUTTON, NULL,
	DmConfirmEmptyCB, B_A, False },
 { True,  TXT_FILE_EXIT,   TXT_M_Exit,        I_PUSH_BUTTON, NULL,
	ExitWBCB,         B_A, False },
 { NULL }
};

MENU("ActionMenu", ActionMenu);

static MenuItems ViewSortMenuItems[] = {
 { True, TXT_SORT_TYPE, TXT_M_SORT_Type, I_PUSH_BUTTON, NULL, DmViewSortCB },
 { True, TXT_SORT_NAME, TXT_M_SORT_NAME, I_PUSH_BUTTON, NULL, DmViewSortCB },
 { True, TXT_SORT_SIZE, TXT_M_SORT_SIZE, I_PUSH_BUTTON, NULL, DmViewSortCB },
 { True, TXT_SORT_AGE, TXT_M_SORT_AGE, I_PUSH_BUTTON, NULL, DmViewSortCB },
 { NULL }
};

MENU("ViewSortMenu", ViewSortMenu);

static MenuItems ViewMenuItems[] = {
 { True, TXT_VIEW_ALIGN, TXT_M_VIEW_ALIGN, I_PUSH_BUTTON, NULL, DmViewAlignCB },
 { True, TXT_VIEW_SORT,  TXT_M_VIEW_SORT,  I_PUSH_BUTTON, (void *)&ViewSortMenu, NULL },
 { NULL }
};
 
MENU("ViewMenu", ViewMenu);

static MenuItems EditMenuItems[] = {
 { True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   I_PUSH_BUTTON, NULL,
	DmEditSelectAllCB,  B_A, False },
 { True, TXT_EDIT_UNSELECT, TXT_M_Unselect, I_PUSH_BUTTON, NULL,
	DmEditUnselectAllCB, B_M, False },
 { True, TXT_WB_FILEPROP,   TXT_M_WB_FILEPROP,   I_PUSH_BUTTON, NULL,
	DmWBEMFilePropCB,   B_M, False },
 { True, TXT_WB_PUTBACK,    TXT_M_WB_PUTBACK,    I_PUSH_BUTTON, NULL,
	DmWBEMPutBackCB,    B_M, False },
 { True, TXT_WB_DELETE,     TXT_M_WB_DELETE,     I_PUSH_BUTTON, NULL,
	DmWBEMDeleteCB,     B_M, False },
 { NULL }
};

MENU("EditMenu", EditMenu);

static MenuItems HelpMenuItems[] = {
 { True,TXT_HELP_WB,       TXT_M_HELP_WB,       I_PUSH_BUTTON, NULL,
	DmHelpSpecificCB, B_A, False },
 { True,TXT_HELP_M_AND_K,  TXT_M_HELP_M_AND_K,  I_PUSH_BUTTON, NULL,
	DmHelpMAndKCB,    B_A, False },
 { True,TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON, NULL,
	DmHelpTOCCB,      B_A, False },
 { True,TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON, NULL,
	DmHelpDeskCB,     B_A, False },
 { NULL }
};

MENU("HelpMenu", HelpMenu);

static MenuItems MenuBarItems[] = {
 { True, TXT_ACTION, TXT_M_ACTION, I_PUSH_BUTTON, (void *)&ActionMenu,
	NULL },
 { True, TXT_EDIT,   TXT_M_EDIT,   I_PUSH_BUTTON, (void *)&EditMenu,
	NULL },
 { True, TXT_VIEW,   TXT_M_VIEW,   I_PUSH_BUTTON, (void *)&ViewMenu,
	NULL },
 { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, (void *)&HelpMenu,
	NULL},
 { NULL }
};

MENU_BAR("menubar", MenuBar, DmMenuSelectCB, 0, 0);

static ContainerGizmo swin_gizmo = {NULL, "swin", G_CONTAINER_SW };

static GizmoRec wb_gizmos[] = {
	{ContainerGizmoClass, &swin_gizmo},
};

static MsgGizmo footer = {NULL, "footer", " ", " "};

static BaseWindowGizmo BaseWindow = {
	NULL,			/* help */
	"Wastebasket",		/* shell widget name */
	TXT_WB_TITLE,		/* title */
	&MenuBar,		/* menu bar */
	wb_gizmos,		/* gizmo array */
	XtNumber(wb_gizmos),	/* # of gizmos in array */
	&footer,		/* MsgGizmo is status & error area */
	TXT_WB_TITLE,		/* icon_name */
	"filledwb",		/* name of pixmap file */
};

#undef XA

DmWbDataRec wbinfo;

static unsigned long timer_unit[]  = { MINUNIT, HOURUNIT, DAYUNIT };

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Initializes the wastebasket.
 */
unsigned int
DmInitWasteBasket(char *geom_str, Boolean iconic, Boolean map_window)
{
	DmFolderWindow wb_win;
	struct stat wbstat;
	Gizmo base;
	int n;
	Arg args[15];
	Widget form;
	Widget menu_btn;

	wbinfo.propGizmo = NULL;

	/* Alloc a wastebasket window and store it in the Desktop struct */
	wb_win = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
	DESKTOP_WB_WIN(Desktop) = wb_win;

	wb_win->attrs = DM_B_WASTEBASKET_WIN;
	wb_win->views[0].view_type = DM_ICONIC;
	wb_win->views[0].sort_type = DM_BY_TYPE;

	DmGetWBPixmaps();

	n = 0;
	XtSetArg(args[n], XmNshadowThickness, 2); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, 4); n++;
	XtSetArg(args[n], XmNrightOffset, 4); n++;
	XtSetArg(args[n], XmNtopOffset, 4); n++;
	XtSetArg(args[n], XmNbottomOffset, 4); n++;
	XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
	XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
	wb_gizmos->args = args;
	wb_gizmos->numArgs = n;

	if (!geom_str &&
		(geom_str = DtGetProperty(&(wb_win->views[0].cp->plist),
			FOLDERSIZE, NULL)))
	{
		/* make sure it is less than the screen size */
		int sw = WidthOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
		int sh = HeightOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
		int w = 0, h = 0;

		if ((sscanf(geom_str, "%dx%d", &w, &h) != 2) || !w || !h ||
		  (w > sw) || (h > sh))
			geom_str = NULL;
	}

	/* Create application shell */
	XtSetArg(Dm__arg[0], XmNiconic, iconic);
	XtSetArg(Dm__arg[1], XmNgeometry, geom_str);
	XtSetArg(Dm__arg[2], XmNmappedWhenManaged, map_window);

	ViewMenu.defaultItem = 1;
	MenuBar.clientData = (XtPointer)wb_win;
	wb_win->gizmo_shell = base = CreateGizmo(DESKTOP_SHELL(Desktop),
		BaseWindowGizmoClass, &BaseWindow, Dm__arg, 3);

	wb_win->shell = GetBaseWindowShell(base);
	wb_win->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
			GetGizmoWidget, "swin");
	DmSetSwinSize(wb_win->swin);
	wbinfo.iconShell = GetBaseWindowIconShell(base);
	DESKTOP_WB_ICON(Desktop) = XtWindow(wbinfo.iconShell);

#ifdef MINIMIZED_DND 
	XtAddEventHandler(wbinfo.iconShell, (EventMask)StructureNotifyMask,
		False, DmDnDRegIconShell, (XtPointer)DESKTOP_WB_WIN(Desktop));
#endif /* MINIMIZED_DND */

	wbinfo.wbdir = (char *)DmDTProp(WBDIR, NULL);
	/* check if wbinfo.wbdir exists */
	if ((stat(wbinfo.wbdir, &wbstat)) != 0) {
		if (errno == ENOENT) {
			Dm__VaPrintMsg(TXT_WB_NOT_EXIST, wbinfo.wbdir);
			/* create wbinfo.wbdir */
			if (mkdir(wbinfo.wbdir, DESKTOP_UMASK(Desktop)) != 0) {
				Dm__VaPrintMsg(TXT_MKDIR, wbinfo.wbdir);
				return(1);
			}
		}
	}
	CreateWBFileClass();
	sprintf(Dm__buffer, "%s/.Wastebasket", wbinfo.wbdir);
	wbinfo.dtinfo = strdup(Dm__buffer);
	wb_win->views[0].cp = Dm__NewContainer(wbinfo.wbdir);
	DmReadDtInfo(wb_win->views[0].cp, wbinfo.dtinfo, 0);
	DtAddCallback(&wb_win->views[0].cp->cb_list, UpdateFolderCB , 
		  (XtPointer) wb_win);

	if (!WB_IS_EMPTY(Desktop))
		UpdateWBObjs();
	else {
		/* make Empty button insensitive */
		Widget w = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
			GetGizmoWidget, "ActionMenu:2");
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(w, Dm__arg, 1);
		/* change iconPixmap */
		SetBaseWindowGizmoPixmap(base, "/usr/X/lib/pixmaps/emptywb");
	}
	wb_win->views[0].nitems = wb_win->views[0].cp->num_objs + 5;

	n = 0;
	/* Create icon container	*/
	XtSetArg(Dm__arg[n], XmNmovableIcons,   True); n++;
	XtSetArg(Dm__arg[n], XmNdrawProc,       DmDrawLinkIcon); n++;
	XtSetArg(Dm__arg[n], XmNmenuProc,       DmIconMenuProc); n++;
	XtSetArg(Dm__arg[n], XmNclientData,     wb_win); n++; /*for MenuProc*/
	XtSetArg(Dm__arg[n], XmNpostSelectProc, DmButtonSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNpostUnselectProc, DmButtonSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNtargets, DESKTOP_DND_TARGETS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNnumTargets,
			XtNumber(DESKTOP_DND_TARGETS(Desktop))); n++;
	XtSetArg(Dm__arg[n], XmNdropProc,       DmDropProc); n++;
    	XtSetArg(Dm__arg[n], XmNtriggerMsgProc, DmFolderTriggerNotify); n++;
    	XtSetArg(Dm__arg[n], XmNconvertProc,    DtmConvertSelectionProc); n++;
    	XtSetArg(Dm__arg[n], XmNdragDropFinishProc, DtmDnDFinishProc); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight, GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth, GRID_WIDTH(Desktop) / 2);n++;
	XtSetArg(Dm__arg[n], XmNgridRows, 2 * FOLDER_ROWS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns, 2 * FOLDER_COLS(Desktop) ); n++;

	wb_win->views[0].box = DmCreateIconContainer(wb_win->swin,
				DM_B_SPECIAL_NAME,
				Dm__arg, n, wb_win->views[0].cp->op,
				wb_win->views[0].cp->num_objs,
				&(wb_win->views[0].itp),
				wb_win->views[0].nitems, NULL);
	InitWBData();
	DmLabelWBFiles();
	/* put something in the status area */
	DmDisplayStatus((DmWinPtr)wb_win);

	/* If wbinfo.suspend is True, reset it to False if map_window is
	 * False (wastebasket window is withdrawn) or iconic is True, and
	 * always set wbinfo.restart to False.
	 *
	 * If wbinfo.suspend is False, set wbinfo.restart to True if
	 * wastebasket window is in normal state.
	 *
	 * If wbinfo.suspend is True but wbinfo.restart is True, restart timer.
	 */
	if (WB_BY_TIMER(wbinfo)) {
		if (wbinfo.suspend == True) {
			if (map_window == False || iconic == True) {
				wbinfo.suspend = False;
				wbinfo.restart = False;
			}
		} else {
			if (iconic == False && map_window == True)
				wbinfo.restart = True;
		}

		if (wbinfo.suspend)
			DmWBRestartTimer();
		else
			DmWBTimerProc(NULL, NULL);

	} else {
		DmWBToggleTimerBtn(False);
		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL);
	}

	/*  We will handle Close requests ourself.  Both the
	 *  Wastebasket folder and the icon menu must be unmapped.
	 */
	XmAddWMProtocolCallback(wb_win->shell,
		XA_WM_DELETE_WINDOW(XtDisplay(wb_win->shell)), WMCB,
		(XtPointer)wb_win);
	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(wb_win->shell, Dm__arg, 1);

	XtRealizeWidget(wb_win->shell);
	XtAddEventHandler(wb_win->shell, PropertyChangeMask, False,
		  WBEventHandler, NULL);

	/* Get a help id for Wastebasket to be used its the Help menu. */
	WB_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(wb_win->shell),
               XtWindow(wb_win->shell), (char *)dtmgr_title,
			GetGizmoText(TXT_WB_TITLE), DESKTOP_NODE_NAME(Desktop),
			NULL, "wb32.icon")->app_id;

	/* Register for context-sensitive help. Since XmNhelpCallback is a
	 * MOTIF resource, register help on the child of the toplevel shell
	 * instead of the shell itself.
	 */
	menu_btn = QueryGizmo(BaseWindowGizmoClass, wb_win->gizmo_shell,
		GetGizmoWidget, "HelpMenu:0");
	form = QueryGizmo(BaseWindowGizmoClass, wb_win->gizmo_shell,
		GetGizmoWidget, NULL);
	XtSetArg(Dm__arg[0], XmNuserData, wb_win);
	XtSetValues(form, Dm__arg, 1);
	XtAddCallback(form, XmNhelpCallback,(XtCallbackProc)DmBaseWinHelpKeyCB,
		(XtPointer)menu_btn);
	return(0);

} /* end of DmInitWasteBasket */

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 * Called when the Exit is selected from the Wastebasket's File menu.
 */
static void
ExitWBCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmUnmapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
} /* end if ExitWBCB */

/****************************procedure*header*****************************
 * Called when the Quit/Close is selected from the Wastebasket window menu.
 */
static void
WMCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	DmUnmapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
} /* end of WMCB */

/****************************procedure*header*****************************
 * Called from DtmExit when the session terminates.
 */
void
DmWBExit()
{
	DmWinPtr window = (DmWinPtr)(DESKTOP_WB_WIN(Desktop));

	if (DESKTOP_WB_WIN(Desktop) == NULL)
		return;

	/* save x's and y's of icons in Wastebasket */
	DmSaveXandY(window->views[0].itp, window->views[0].nitems);

	/* save window dimensions */
	DmWindowSize(window, Dm__buffer);
	DtSetProperty(&(window->views[0].cp->plist), FOLDERSIZE, Dm__buffer, NULL);

	/* make sure wastebasket properties are up-to-date */
	wbinfo.restart = wbinfo.suspend ? False : True;
#ifdef MOTIF_WB_PROPS
	DmSaveWBProps();
#endif /* MOTIF_WB_PROPS */

	/* flush window info to disk */
	DmFlushContainer(window->views[0].cp);

	if (WB_ON_EXIT(wbinfo) && wbinfo.wbdir) {
		sprintf(Dm__buffer, "rm -rf %s/*", wbinfo.wbdir);
		system(Dm__buffer);
	}
	DESKTOP_WB_WIN(Desktop) = NULL;

} /* end of DmWBExit */

/****************************procedure*header*****************************
 * Event handler to track opening and closing the wastebasket window.
 * The state of timer is updated appropriately if wbinfo.cleanUpMethod
 * is WBByTimer. 
 */
static void
WBEventHandler(Widget w, XtPointer client_data, XEvent *xevent,
Boolean *cont_to_dispatch)
{

	if (!WB_BY_TIMER(wbinfo))
		return;

	if ((xevent->type != PropertyNotify) ||
	    (xevent->xproperty.state != PropertyNewValue))
		return;

	if (xevent->xproperty.atom == XA_WM_STATE(xevent->xany.display)) {
		int winstate = GetWMState(XtDisplay(
					DESKTOP_WB_WIN(Desktop)->shell), 
					XtWindow(DESKTOP_WB_WIN(Desktop)->shell)
					);
		if (((winstate == IconicState) || (winstate == WithdrawnState))
		    && wbinfo.suspend) 
			DmWBResumeTimer();
		else if (winstate == NormalState) {

			/* if Wastebasket is not iconized at the start of a session
			 * and the timer was not suspended at the end of the previous
			 * session, don't suspend the timer - but only if wastebasket
			 * is not empty.
			 */
			if (!wbinfo.suspend && wbinfo.restart && !WB_IS_EMPTY(Desktop)) {
				wbinfo.restart = False;
				return;
			}
			wbinfo.restart = False;
			DmWBSuspendTimer();
		}
		/* update wbSuspend resource in WB's .dtinfo file */
		sprintf(Dm__buffer, "%d", wbinfo.suspend);
		DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist), TIMER_STATE,
			Dm__buffer, NULL);
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);
	}

} /* end of WBEventHandler */

/****************************procedure*header*****************************
 * Create a file class to be used for files in the Wastebasket.
 */
static void
CreateWBFileClass()
{
	char	*dflt_icon = "datafile.icon";

	if ((wbinfo.fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec)))
	     == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return;
	}

	wbinfo.fcp->key = (void *)&(wbinfo.key);
	DtSetProperty(&(wbinfo.fcp->plist), ICONFILE, dflt_icon, NULL);
	DtSetProperty(&(wbinfo.fcp->plist), DFLTICONFILE, dflt_icon, NULL);

} /* end of CreateWBFileClass */

/****************************procedure*header*****************************
 * Update file class of files in Wastebasket.
 */
static void
UpdateWBObjs()
{
	register DmObjectPtr op;
	register char        *p;
	struct stat buf;
	struct stat lbuf;

	for (op = DESKTOP_WB_WIN(Desktop)->views[0].cp->op; op; op = op->next) {
		/* This is just to set ftype. */
		p = DmObjPath(op);
		(void)lstat(p, &lbuf);
		if (stat(p, &buf) == -1)
			Dm__SetDfltFileClass(op, NULL, &lbuf);
		else {
			DmInitObj(op, &buf, DM_B_INIT_FILEINFO);
			Dm__SetDfltFileClass(op, &buf, &lbuf);
		}

		/* override it with WB's special fcp */
		op->fcp = wbinfo.fcp;
	}
} /* end of UpdateWBObjs */

/****************************procedure*header*****************************
 * Initializes wbinfo.  Clean up property values are stored as container
 * properties.
 */
static void
InitWBData()
{
	char *p;

	if (p = DtGetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
			CLEANUP_METHOD, NULL))
		wbinfo.cleanUpMethod = atoi(p);
	else
		wbinfo.cleanUpMethod = DEFAULT_WB_CLEANUP;

	if (p = DtGetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
			TIMER_INTERVAL, NULL))
		wbinfo.interval = atoi(p);
	else
		wbinfo.interval = DEFAULT_WB_TIMER_INTERVAL;

	if (p = DtGetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
			UNIT_INDEX, NULL))
		wbinfo.unit_idx = atoi(p);
	else
		wbinfo.unit_idx = DEFAULT_WB_TIMER_UNIT;

	if (p = DtGetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
			TIMER_STATE, NULL))
		wbinfo.suspend = atoi(p);
	else
		wbinfo.suspend = DEFAULT_WB_SUSPEND_TIMER;

	if (p = DtGetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
			RESTART_TIMER, NULL))
		wbinfo.restart = atoi(p);
	else
		wbinfo.restart = DEFAULT_WB_RESTART_TIMER;

	wbinfo.time_unit     = timer_unit[wbinfo.unit_idx];
	wbinfo.timer_id      = NULL;
	wbinfo.tm_start      = (time_t)0;
	wbinfo.tm_remain     = (unsigned long)0;
	wbinfo.tm_interval   = (unsigned long)0;
	wbinfo.key.name	    = "";

} /* end of InitWBData */


void
DmSetWBMenuItem(MenuItems ** ret_menu_items, char * name, int index,
                                                DmFolderWinPtr folder)
{
	switch(index)
	{

	case 0:

		*ret_menu_items = ActionMenuItems;
		strcpy(name, "ActionMenu");
		break;

	case 1:

		*ret_menu_items = EditMenuItems;
		strcpy(name, "EditMenu");
		break;

	case 2:

		*ret_menu_items = ViewMenuItems;
		strcpy(name, "ViewMenu");
		break;

	case 3:

		*ret_menu_items = HelpMenuItems;
		strcpy(name, "HelpMenu");
		break;

	default:
		break;
	}

}
