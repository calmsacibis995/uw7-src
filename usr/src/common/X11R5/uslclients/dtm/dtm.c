/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dtm.c	1.178"

/******************************file*header********************************

    Description:
	This file contains the source code for dtm "main".
*/
						/* #includes go here	*/
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>

#include <Xm/Xm.h>
#include <Xm/FontObj.h>

#include <MGizmo/Gizmo.h>		/* for Exit Notice */
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"
#include "olwsm/dtprop.h"		/* for DEFAULT's */
#include "dm_exit.h"

static int	(*DefaultErrorHdr)();

/**************************forward*declarations***************************
 *
 *	Forward Procedure definitions.
 */
static void	AtExit(void);
static void	CancelExitCB(Widget, XtPointer, XtPointer);
static void	DmInitDynamicFonts(void);
static void	DtmExit(void);
static void	Exit(int error_code, char * message);
static void	ExitCB(Widget, Boolean);
static void	ExitAndSaveCB(Widget, XtPointer, XtPointer);
static void	ExitOnlyCB(Widget, XtPointer, XtPointer);
static void	ExitHelpCB(Widget, XtPointer, XtPointer);
static void	InitFsTbl(void);
static void	SignalHandler(int signum);
static void     ShutdownHelpCB(Widget, XtPointer, XtPointer);
static char *	SetHOME();
static void	ProcessEvent(void);
static void	cdbNoticeMenuCB(Widget w, XtPointer client_data,
			XtPointer call_data);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#ifndef NOWSM
#define RC			"/.olinitrc"
#define TIME_TO_WAIT_FOR_WM	10

extern Widget	InitShell;
extern Widget	handleRoot;

extern void	BusyPeriod(Widget w, Boolean busy);
extern Widget	CreateHandleRoot(void);
extern void	DestroyPropertyPopup(void);
extern int	DmInitWSM(int, String *);
extern void	ExecRC(char * path);
extern char *	GetPath(char * name);
extern void     ServiceEvent OL_ARGS((XEvent *event));
extern void	SetWSMBang(Display *, Window, unsigned long);
extern void	WSMExit(void);
#endif

/* Define exit codes */
#define START_UP_ERROR		41
#define SHUTDOWN_EXIT_CODE	42	/* also in xinit.c */
#define OPEN_DT_ERROR		43
#define INIT_WB_ERROR		44
#define INIT_HM_ERROR		45
#define INIT_HD_ERROR		46
#define INIT_WSM_ERROR		47

static Gizmo cdbNotice;

static const char * const	DmAppName  = "DesktopMgr";
static const char * const	DmAppClass = "DesktopMgr";

/* Define global desktop structure (should be 1-per display) */
DmDesktopPtr		Desktop = NULL;


/* title of dtm modules used in on-line help */
const char *dtmgr_title;
const char *product_title;
const char *folder_title;

/* Define variable for exit code.  Used by olwsm/wsm.c:WSMExit */
int DtmExitCode;
#ifdef FDEBUG
int Debug = FDEBUG;
#endif


/****************************widget*resources*****************************
    Define resource list for dtm options
*/
/* Define resource names */
static char XtNkillAllWindows[]		= "killAllWindows";
static char XtNshowHiddenFiles[]	= "showHiddenFiles";
static char XtNlabelSeparators[]	= "labelSeparators";
static char XtCCols[]			= "Cols";
static char XtCLaunchFromCWD[]		= "LaunchFromCWD";
static char XtCOpenInPlace[]		= "OpenInPlace";
static char XtCKillAllWindows[]		= "KillAllWindows";
static char XtCRows[]			= "Rows";
static char XtCShowFullPaths[]		= "ShowFullPaths";
static char XtCShowHiddenFiles[]	= "ShowHiddenFiles";
static char XtCSyncInterval[]		= "SyncInterval";
static char XtCSyncRemoteFolders[]	= "SyncRemoteFolders";
static char XtCTreeDepth[]		= "TreeDepth";
static char XtCWbSuspend[]		= "WbSuspend";
static char XtCWbCleanUpMethod[]	= "WbCleanUpMethod";
static char XtCLabelSeparators[]	= "LabelSeparators";
static char XtCWbTimerInterval[]	= "WbTimerInterval";
static char XtCWbTimerUnit[]		= "WbTimerUnit";
static char XtCHelpKeyColor[]		= "HelpKeyColor";

#define OFFSET(field)	XtOffsetOf(DmOptions, field)

static XtResource resources[] = {
  { XtNfolderCols, XtCCols, XtRUnsignedChar, sizeof(u_char),
    OFFSET(folder_cols), XtRImmediate, (XtPointer)DEFAULT_FOLDER_COLS },

  { XtNfolderRows, XtCRows, XtRUnsignedChar, sizeof(u_char),
    OFFSET(folder_rows), XtRImmediate, (XtPointer)DEFAULT_FOLDER_ROWS },

  { XtNgridWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    OFFSET(grid_width), XtRImmediate, (XtPointer)DEFAULT_GRID_WIDTH },

  { XtNgridHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    OFFSET(grid_height), XtRImmediate, (XtPointer)DEFAULT_GRID_HEIGHT},

  { XtNlaunchFromCWD, XtCLaunchFromCWD, XtRBoolean, sizeof(Boolean),
    OFFSET(launch_from_cwd), XtRImmediate, (XtPointer)DEFAULT_LAUNCH_FROM_CWD},

  { XtNkillAllWindows, XtCKillAllWindows, XtRBoolean, sizeof(Boolean),
    OFFSET(kill_all_windows), XtRImmediate, (XtPointer)True },

  { XtNopenInPlace, XtCOpenInPlace, XtRBoolean, sizeof(Boolean),
    OFFSET(open_in_place), XtRImmediate, (XtPointer)DEFAULT_OPEN_IN_PLACE },

  { XtNshowFullPaths, XtCShowFullPaths, XtRBoolean, sizeof(Boolean),
    OFFSET(show_full_path), XtRImmediate, (XtPointer)DEFAULT_SHOW_PATHS },

  { XtNshowHiddenFiles, XtCShowHiddenFiles, XtRBoolean, sizeof(Boolean),
    OFFSET(show_hidden_files), XtRImmediate, False },

  { XtNlabelSeparators, XtCLabelSeparators, XtRString, sizeof(String),
    OFFSET(label_separators), XtRImmediate, NULL },

  /* NOTE: should be unsigned long.  Need converter */
  { XtNsyncInterval, XtCSyncInterval, XtRInt, sizeof(int),
    OFFSET(sync_interval), XtRImmediate, (XtPointer)DEFAULT_SYNC_INTERVAL },

  { XtNsyncRemoteFolders, XtCSyncRemoteFolders, XtRBoolean, sizeof(Boolean),
    OFFSET(sync_remote_folders), XtRImmediate, False },

  { XtNtreeDepth, XtCTreeDepth, XtRUnsignedChar, sizeof(u_char),
    OFFSET(tree_depth), XtRImmediate, (XtPointer)DEFAULT_TREE_DEPTH },

};

#undef OFFSET

#include <X11/Xproto.h>
int
DtmErrorHdr(Display * dpy, XErrorEvent * error)
{
		/* These errors can happen when doing drag-and-drop with
		 * Moolit apps (e.g., dialup and backup). We shall find
		 * the real cause(s). Enable the code here is just for
		 * protection because this piece shall never die unless
		 * we say so... */
	if ((error->error_code == BadWindow ||
	     error->error_code == BadDrawable) &&
	    (error->request_code == X_ChangeWindowAttributes ||
	     error->request_code == X_PolyFillRectangle)) {

		fprintf(stderr, "%s error on %s\n",
			error->error_code == BadWindow ?
				"BadWindow" : "BadDrawable",
			error->request_code == X_ChangeWindowAttributes ?
				"X_ChangeWindowAttributes" :
				"X_PolyFillRectangle");
		return 0;
	} else {

		fprintf(stderr, "dtm: ");
		(*DefaultErrorHdr)(dpy, error);
	}
}

/****************************procedure*header*****************************
    main-
*/
void
main(int argc, char * argv[])
{
    struct utsname	unames;
    Display *		dpy;
    Window		win;
    CharArray *		dummy_array;
    Boolean		new_dtfclass = False;
    char *		home;
    char *		separators;
    int			labelwidth;

#ifdef MEMUTIL
    InitializeMemutil();
#endif

    /* don't buffer stdout output */
    setbuf(stdout, NULL);

    /* Allocate global Desktop structure */
    if ((Desktop = (DmDesktopPtr)CALLOC(1, sizeof(DmDesktopRec))) == NULL)
    {
	/* Don't use "Exit()" since there's no InitShell, yet */
	Dm__VaPrintMsg(TXT_MEM_ERR);
	exit(START_UP_ERROR);
	/* NOTREACHED */
    }

    /****	Stuff needed before exec'ing .olinitrc file	****/

    /* make sure XWINHOME is set in environment */
    if (!getenv("XWINHOME"))
	putenv("XWINHOME=/usr/X");

    /* Make sure HOME is set in environment */
    if ((home = getenv("HOME")) == NULL)
	home = SetHOME();
    DESKTOP_HOME(Desktop)= strdup(home); /* While we're at it, save HOME */

    /* Register the default language proc */
    XtSetLanguageProc(NULL, NULL, NULL);

    /* Begin new process group */
    (void)setpgrp();

    /* ExecRC will wait for child death (script finishes).
     * Child death of processes created AFTER ExecRC are ignored (below).
     */
#ifndef NOWSM
    ExecRC(GetPath(RC));
#endif

    /* dtm doesn't wait for forked child processes so ignore SIGCLD to
     * prevent defunct processes (zombies)
     */
    (void)signal(SIGCLD, SIG_IGN);

    /* Notes from SAMC:
     *
     * You may want to use XtAppInitialize later on because of other
     * functionalities. When you do this, you also need to do changes
     * on your MainLoop, e.g., replace XtNextEvent with XtAppNextNext.
     */
    DESKTOP_SHELL(Desktop) =
	XtInitialize((char *)DmAppName, (char *)DmAppClass,
		     NULL, 0, &argc, argv);

    DefaultErrorHdr = XSetErrorHandler(DtmErrorHdr);

    XtSetArg(Dm__arg[0], XtNmappedWhenManaged, False);
    XtSetArg(Dm__arg[1], XtNwidth, 1);
    XtSetArg(Dm__arg[2], XtNheight, 1);
    XtSetValues(DESKTOP_SHELL(Desktop), Dm__arg, 3);

    XtRealizeWidget(DESKTOP_SHELL(Desktop));

    dpy = DESKTOP_DISPLAY(Desktop);
    win = XtWindow(DESKTOP_SHELL(Desktop));

    if (DtSetAppId(dpy, win, "_DT_QUEUE") ||
        DtSetAppId(dpy, win, "_WB_QUEUE") ||
	DtSetAppId(dpy, win, "_HELP_QUEUE"))
    {
    	Dm__VaPrintMsg(TXT_DTM_IS_RUNNING);
    	exit(START_UP_ERROR);
	/* NOTREACHED */
    }


#ifndef NOWSM
    /* Set these for olwsm */
    InitShell	= DESKTOP_SHELL(Desktop);
    handleRoot	= CreateHandleRoot();

    BusyPeriod(handleRoot, True);

    if (DmInitWSM(argc, argv))
    {
	Exit(INIT_WSM_ERROR, TXT_WSM_INIT_FAILED);
	/* NOTREACHED */
    }
#endif /* NOWSM */

    XtGetApplicationResources(DESKTOP_SHELL(Desktop),
			      (XtPointer)&DESKTOP_OPTIONS(Desktop),
			      resources, XtNumber(resources), NULL, 0);
    InitializeGizmos(DmAppName);
    DtiInitialize(DESKTOP_SHELL(Desktop));
    ExmInitDnDIcons(DESKTOP_SHELL(Desktop));

    /* get node name */
    (void)uname(&unames);
    DESKTOP_NODE_NAME(Desktop) = strdup(unames.nodename);

    /* get umask */
    DESKTOP_UMASK(Desktop) = umask(0);
    umask(DESKTOP_UMASK(Desktop));
    DESKTOP_UMASK(Desktop) =
	~DESKTOP_UMASK(Desktop) & (S_IRWXU | S_IRWXG | S_IRWXO);

    /* initialize bg flag */
    Desktop->bg_not_regd	= True;
    DESKTOP_CUR_TASK(Desktop)	= NULL;

    DESKTOP_CONTAINERS(Desktop).esize = sizeof(DmContainerPtr);
    SYNC_TIMER(Desktop) = NULL;

    /* Initialize number of visited folders */
    NUM_VISITED(Desktop) = 0;

    DESKTOP_CWD(Desktop) = getcwd(NULL, PATH_MAX + 1);/* remember initial CWD */

    if (cuserid(Dm__buffer)) {
    	DESKTOP_LOGIN(Desktop) = strdup(Dm__buffer);
    } else {
	fprintf(stderr,"dtm: failed to get user login name.\n");
	exit(START_UP_ERROR);
    }
	
    Desktop->C_locale = !strcmp(setlocale(LC_MESSAGES, NULL), "C");

    /* announce gui mode for MoOLIT applications */
    putenv("XGUI=MOTIF");

    DmInitDTMReqProcs(DESKTOP_SHELL(Desktop));
    DmInitWBReqProcs(DESKTOP_SHELL(Desktop));
    DmInitHMReqProcs(DESKTOP_SHELL(Desktop));

#ifndef NOWSM
    /* Need to wait for window manager to come up, at least for a while,
     * before mapping dtm windows.
     */
    {
	XWindowAttributes	win_attr;
	int			i;

	dpy = DESKTOP_DISPLAY(Desktop);
	win  = RootWindowOfScreen(DESKTOP_SCREEN(Desktop));

	for (i=TIME_TO_WAIT_FOR_WM; i; i--)
	{
	    XGetWindowAttributes(dpy, win, &win_attr);
	    if (win_attr.all_event_masks & SubstructureRedirectMask)
		break;
	    sleep(1);
	}
    }
#endif

    DmInitDynamicFonts();
    InitFsTbl();

    /* Initialize wrapped labe info before opening any windows */
    if (LABEL_SEPS(Desktop))
	separators = LABEL_SEPS(Desktop);
    else
	separators = Dm__gettxt(TXT_LABEL_SEPARATORS);

    /* Should take into account of shadow thickness */
    labelwidth = GRID_WIDTH(Desktop) - 3 *
		     WidthOfScreen(DESKTOP_SCREEN(Desktop)) /
		     WidthMMOfScreen(DESKTOP_SCREEN(Desktop));
    if (labelwidth < 2)
	labelwidth = 2;
    DmSetWrappedLabelInfo(separators, labelwidth);

    switch(DmOpenDesktop())
    {
    case -1:
	Exit(OPEN_DT_ERROR, TXT_OPEN_DESKTOP);
	/* NOTREACHED */

    case 2:
	new_dtfclass = True;
	/* FALLTHROUGH */

    case 0:
	/* FALLTHROUGH */

    case 1:
	/* Start Wastebasket and Help Desk here only if they weren't saved in
	 * the previous session.  They should always be *initialized* 	
	 * regardless of whether they're saved in a previous session so that
	 * file deletion and adding/removing apps to/from the Help Desk will
	 * work.
	 */
	if (!DESKTOP_WB_WIN(Desktop) && DmInitWasteBasket(NULL, False, False))
	{
	    Exit(INIT_WB_ERROR, TXT_WB_INIT_FAILED);
	    /* NOTREACHED */
	}

	if (DmInitHelpManager())
	{
	    Exit(INIT_HM_ERROR, TXT_HM_INIT_FAILED);
	    /* NOTREACHED */
	}
	break;

    }

    /* Get Help ID for folders */
    FOLDER_HELP_ID(Desktop) = DmNewHelpAppID(DESKTOP_SCREEN(Desktop),
		XtWindow(DESKTOP_SHELL(Desktop)), (char *)dtmgr_title,
		(char *)folder_title, DESKTOP_NODE_NAME(Desktop),
		NULL, "dir.icon")->app_id;

    /*catch signals */
    sigset(SIGHUP, SignalHandler);
    sigset(SIGTERM, SignalHandler);
    sigset(SIGINT, SignalHandler);

	/* inform user $HOME/.dtfclass was replaced */
	if (new_dtfclass) {
		static MenuItems cdbNoticeMenuItems[] = {
  			{ True, TXT_OK,   TXT_M_OK,   I_PUSH_BUTTON, NULL,
				cdbNoticeMenuCB, (XtPointer)0, True},
  			{ True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, NULL,
				cdbNoticeMenuCB, (XtPointer)1, False},
			{ NULL }
    		};

		static MenuGizmo cdbNoticeMenu = {
    			NULL, "cdbNoticeMenu", "_X_", cdbNoticeMenuItems,
			NULL, NULL
		};

		static ModalGizmo cdbNoticeGizmo = {
			NULL,				/* help info */
			"cdbNotice",			/* shell name */
			TXT_G_PRODUCT_NAME,		/* title */
			&cdbNoticeMenu,			/* menu */
			"",				/* message */
			NULL, 0,			/* gizmos, num_gizmos*/
			XmDIALOG_MODELESS,		/* style */
			XmDIALOG_INFORMATION		/* type */
		};


		char *p = DmDTProp(FILEDB_PATH, NULL);

		sprintf(Dm__buffer, TXT_BAD_FILE_DATABASE, p, p);
		cdbNoticeGizmo.message = Dm__buffer;
		cdbNotice = CreateGizmo(DESKTOP_SHELL(Desktop),
			ModalGizmoClass, &cdbNoticeGizmo, NULL, 0);

		/* register for context-sensitive help */
		DmRegContextSensitiveHelp(GetModalGizmoDialogBox(cdbNotice),
			DESKTOP_HELP_ID(Desktop), DESKTOP_HELPFILE,
			BAD_DTFCLASS_SECT);
		MapGizmo(ModalGizmoClass, cdbNotice);
	}

#if !defined(BEEP_WHEN_OLFM_READY) && !defined(NOWSM)
    DmBeep();
#endif
    
#ifndef NOWSM
    BusyPeriod(handleRoot, False);
#endif /* NOWSM */

    atexit(AtExit);

    /* XtMainLoop equivalent:  This code was moved from wsm.c (olwsm) */

    /* FLH MORE: should we use GizmoMainLoop here?  
       Problem: we have two event handlers (ServiceEvent
       and HandleWindowDeaths, which must occur on opposite
       sides of XtDispatchEvent.  Can HandleWindowDeaths
       be done *before* the event is dispatched?
     */
    for (;;) {
	XEvent event;

	XtNextEvent (&event);
#ifndef NOWSM
	ServiceEvent(&event);
#else
	XtDispatchEvent(&event);
#endif /* NOWSM */
        HandleWindowDeaths(&event);

    }
    /*NOTREACHED*/
}


/***************************private*procedures****************************
 *
 *		PRIVATE PROCEDURES
 */

static void
AtExit(void)
{
    DtmExit();
#ifndef NOWSM
    WSMExit();
#endif /* NOWSM */
}

static void
cdbNoticeMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int index = (int)client_data;

	switch(index) {
	case 0:
	{
		Widget shell = GetModalGizmoShell(cdbNotice);

		FreeGizmo(ModalGizmoClass, cdbNotice);
		XtUnmapWidget(shell);
		XtDestroyWidget(shell);
	}
		break;
	case 1:
		DmDisplayHelpSection(DmGetHelpApp(DESKTOP_HELP_ID(Desktop)),
			NULL, DESKTOP_HELPFILE, BAD_DTFCLASS_SECT);
		break;
	}
} /* end of cdbNoticeMenuCB */

/****************************procedure*header*****************************
 * CancelExitCB
 *
 * This callback procedure is executed when the user SELECTs the
 * "Cancel" button in the Exit Notice.  The callback unposts the
 * notice and resets the state of the Session data structure.
 */
static void
CancelExitCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean          shutdown = (Boolean)client_data;

    /* FLH MORE: we are popping down a modal gizmo here.
       Do we need to do anything else?  (unmanage the child
       of the shell?  If we make changes here, we need to do
       it everywhere we pop down the modal.
    */
    XtPopdown((Widget)DtGetShellOfWidget(widget));

    if (shutdown)
	DmVaDisplayStatus((DmWinPtr)DESKTOP_TOP_FOLDER(Desktop),
                          False, TXT_SHUTDOWN_ABORTED);

    Session.ProcessTerminating = 0;
    Session.WindowKillCount    = 0;

    return;
}					/* end of CancelExitCB */

/****************************procedure*header*****************************
 * DestoryCB-
 */
static void
DestroyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    Gizmo gizmo = (Gizmo)clientData;

    FreeGizmo(ModalGizmoClass, gizmo);
}

/****************************procedure*header*****************************
 * ExitCB
 *
 * This callback procedure is executed when the user SELECTs the 
 * "Exit and Save Session" or "Exit" buttons of the Exit Notice.
 * If the session manager was already supposed to be terminating
 * the session (i.e., Session.ProcessTerminating is set) then the
 * callback calls RealExit to force an exit.  This is done to give
 * the user control in the event that, for some reason, the normal
 * session exit is not proceeding.  If the session was not yet in
 * a termination state, the callback unposts the notice, saves the
 * Desktop properties, sets the termination flag, and attempts the
 * session termination (by calling QueryAndKillWindowTree).  If, on
 * this first attempt there are no windows left to be handled, RealExit
 * is called to terminate the desktop manager.
 * 
 * The routine also, conditionally - based on which button was SELECTed,
 * saves the current state of the desktop windows by calling DmSaveSession.
 *
 * The \fIshutdown\fP flag passed in as \fIclient_data\fP, is used to save
 * the appropriate return code in the ProcessTerminating member of the
 * Session struct.  The value of this variable is maintained to be one
 * of: 0 (indicating that the session is not in the termination state,
 * 1 (indicating that the session is in a \fInormal\fP termination state,
 * or 43 (indicating that the session is terminating and effecting a
 * system shutdown).
 */
static void
ExitCB(Widget widget, Boolean shutdown)
{
    if (!Session.ProcessTerminating)
    {
	XtPopdown((Widget)DtGetShellOfWidget(widget));

	if (Desktop->dpalette)
	    MergeResources(Desktop->dpalette);

	DmSaveDesktopProperties(Desktop);

	WSMExitProperty(DESKTOP_DISPLAY(Desktop));

	DtmExitCode = shutdown ? SHUTDOWN_EXIT_CODE : 0;

	Session.ProcessTerminating = ((shutdown) ? SHUTDOWN_EXIT_CODE : 0) + 1;

	/* Kill window tree.  If this fails, return now */
	if (KILL_ALL_WINDOWS(Desktop) &&
	    QueryAndKillWindowTree(XtDisplay(widget)))
	    return;
    }
    RealExit(Session.ProcessTerminating);

}					/* end of ExitCB */

/* ExitOnlyCB & ExitAndSaveCB are really just frontends to ExitCB */

static void
ExitOnlyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean          shutdown = (Boolean)client_data;

    ExitCB(widget, shutdown);
}

static void
ExitAndSaveCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean          shutdown = (Boolean)client_data;

    DmSaveSession(DmMakePath(DmDTProp(DESKTOPDIR, NULL),
			     ".lastsession"));
    ExitCB(widget, shutdown);
}

/****************************procedure*header*****************************
 * DtmExit- this performs final shut-down of dtm.
 *	This is external so that it can be called from olwsm/wsm.c.  When
 *	exiting normally (by the user), this is called as the result of a
 *	WSM_EXIT message from the olwm.
 */
static void
DtmExit()
{
    static int first = 1;

    if (!first)
	return;
    first = 0;

    if (Desktop->dpalette)
        MergeResources(Desktop->dpalette);

    DmWBExit();			/* close waste basket */
    DmHDExit();			/* close help desk */
    DmHMExit();			/* close all help windows */
    DmExitIconSetup();		/* close Icon Setup */
    DmCloseFolderWindows();	/* close all opened folder windows */
}

/****************************procedure*header*****************************
 * Exit-
 */
static void
Exit(int exit_code, char * message)
{
#ifndef NOWSM
    BusyPeriod(handleRoot, False);
#endif /* NOWSM */
    Dm__VaPrintMsg(message);
    exit(exit_code);
}

/****************************procedure*header*****************************
 * ExitHelpCB- 
 */
static void
ExitHelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
	DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

	DmDisplayHelpSection(help_app, NULL, DESKTOP_HELPFILE,
		EXIT_DESKTOP_SECT);
}					/* end of ExitHelpCB */

/****************************procedure*header*****************************
 * InitFsTbl- Populate fstbl with supported file system in the system.
 */
static void
InitFsTbl(void)
{
    struct fsinfo *	tfstbl;
    int			nfstype;
    int			i;

    DOSFS_ID(Desktop)	= -1;
    NUCFS_ID(Desktop)	= -1;
    NUCAM_ID(Desktop)	= -1;
    NFS_ID(Desktop)	= -1;

    nfstype = NUMFSTYPES(Desktop) = sysfs(GETNFSTYP);
    Desktop->fstbl = (struct fsinfo *)
	CALLOC(1, sizeof(struct fsinfo) * (nfstype + 1));

    for (i = 1, tfstbl = Desktop->fstbl; i <= nfstype; i++, tfstbl++)
    {
	char buf[FSTYPSZ];

	tfstbl->f_maxnm = -1;

	sysfs(GETFSTYP, i, buf);
	strcpy(tfstbl->f_fstype, buf);
	if (!strcmp(buf, DOSFS))
	    DOSFS_ID(Desktop) = i;
	else if (!strcmp(buf, NUCFS))
	    NUCFS_ID(Desktop) = i;
	else if (!strcmp(buf, NUCAM))
	    NUCAM_ID(Desktop) = i;
	else if (!strcmp(buf, _NFS))
	    NFS_ID(Desktop) = i;
    }
    Desktop->fstbl[NFS_ID(Desktop)].f_maxnm = -2;
}

static void
ShutdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown((Widget)DtGetShellOfWidget(widget));
    DmPromptExit(True);
}

/****************************procedure*header*****************************
 * Displays help on Shutdown.
 */
static void
ShutdownHelpCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
	DmHelpAppPtr help_app = DmGetHelpApp(DESKTOP_HELP_ID(Desktop));

	DmDisplayHelpSection(help_app, NULL, SHUTDOWN_HELPFILE,
		SHUTDOWN_INTRO_SECT);

} /* end of ShutdownHelpCB */

/****************************procedure*header*****************************
 *    SignalHandler-
 */
static void
SignalHandler(int signum)
{
    Dm__VaPrintMsg(TXT_CAUGHT_SIGNAL, signum);
    DtmExit();
#ifndef NOWSM
    WSMExit();
#endif /* NOWSM */
    /* NOTREACHED */
}

/****************************procedure*header*****************************
 * SetHOME - Sets the HOME environment variable to home directory set
 *	in the effective user's password entry.  This is called at
 *	initialization if HOME is not set.
 */
static char *
SetHOME()
{	
    char * home;

    struct passwd *pw = getpwuid(geteuid());
    sprintf(Dm__buffer, "HOME=%s", pw->pw_dir);
    home = malloc(strlen(Dm__buffer)+1);
    strcpy(home, Dm__buffer);
    putenv(home);
    return(pw->pw_dir);
}					/* end of SetHOME */


/****		Dynamic Fonts		****/

/****************************procedure*header*********************************
 * ResizeFolderItems-
 */
static void
ResizeFolderItems(DmFolderWindow folder)
{
    DmItemPtr 			item;
    int				i;
    Dimension 			width, height;
    Widget			icon_box;

    icon_box = folder->views[0].box;

    for (i=0, item = folder->views[0].itp; i < folder->views[0].nitems; item++, i++){
	if(!ITEM_MANAGED(item))
	    continue;

	FREE_LABEL(item->label);
	MAKE_WRAPPED_LABEL(item->label, FPART(icon_box).font,
			   DmGetObjectName(ITEM_OBJ(item)));
	DmComputeItemSize(icon_box, item, 
			  folder->views[0].view_type, 
			  &width, &height);
	item->icon_width = (XtArgVal)width;
	item->icon_height = (XtArgVal)height;
    }
    DmTouchIconBox((DmWinPtr) folder, NULL, 0);

} /* end of ResizeFolderItems */

/****************************procedure*header*********************************
 * HandleFontUpdate:  cache the fontlist and geometry info for each
 * 	family fontlist.  Go through all folders and update the size 
 *	of each item.
 *
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 */
static void
HandleFontUpdate(Widget fontObj, XtPointer client_data, XtPointer call_data)
{
    XmFontObjectCallbackStruct *data = (XmFontObjectCallbackStruct *)call_data;
    Dimension 			width, height;
    XmFontList 			font;
    DmFolderWinPtr 		folder;
    Arg				args[2];

    _DPRINT1(stderr, "HandleFontUpdate: \n");

    switch (data->reason){
    
    case XmCR_SANS_SERIF_FAMILY_CHANGED:
	DM_FontExtent(data->new_font, &width, &height);
	DESKTOP_SANS_SERIF(Desktop).font = data->new_font;
	DESKTOP_SANS_SERIF(Desktop).max_width = width;
	DESKTOP_SANS_SERIF(Desktop).max_height = height;
	break;

    case XmCR_SERIF_FAMILY_CHANGED:	
	DM_FontExtent(data->new_font, &width, &height);
	DESKTOP_SERIF(Desktop).font = data->new_font;
	DESKTOP_SERIF(Desktop).max_width = width;
	DESKTOP_SERIF(Desktop).max_height = height;
	break;

    case XmCR_MONOSPACED_FAMILY_CHANGED:
	DM_FontExtent(data->new_font, &width, &height);
	DESKTOP_MONOSPACED(Desktop).font = data->new_font;
	DESKTOP_MONOSPACED(Desktop).max_width = width;
	DESKTOP_MONOSPACED(Desktop).max_height = height;
	break;
    default:
	/* bad reason, don't update anything */
	return;
	break;
    }

    /*	Update item sizes for all folders that are using the changed font.
     *  We know the dynamic font callback is triggered *after*
     *	the FontObject has done SetValues on the widgets.
     *	If we recalculate item sizes for all folders using
     *	the new font, we will get all of those that changed
     *  (and perhaps a few extra too, but that's better than missing
     * 	some)
     * 
     */
    XtSetArg(args[0], XmNfontList, &font);
    for (folder = DESKTOP_FOLDERS(Desktop); folder != NULL; 
	 folder = folder->next){
	XtGetValues(folder->views[0].box, args, 1);
	if (font == data->new_font)
	    ResizeFolderItems(folder);
    }
    if ((folder = (DESKTOP_WB_WIN(Desktop))) != NULL){
	XtGetValues(folder->views[0].box, args, 1);	
	if (font == data->new_font)
	    ResizeFolderItems(folder);
    }
    if ((folder = (DESKTOP_HELP_DESK(Desktop))) != NULL){
	XtGetValues(folder->views[0].box, args, 1);	
	if (font == data->new_font)
	ResizeFolderItems(folder);
    }
    if ((folder = (TREE_WIN(Desktop))) != NULL){
	XtGetValues(folder->views[0].box, args, 1);	
	if (font == data->new_font)
	    ResizeFolderItems(folder);
    }
    if ((folder = (DESKTOP_ICON_SETUP_WIN(Desktop))) != NULL){
	XtGetValues(folder->views[0].box, args, 1);	
	if (font == data->new_font)
	    DmISHandleFontChanges();
    }
}	/* end of HandleFontUpdate */

/****************************procedure*header*********************************
 * DmInitDynamicFonts: Determine if the toolkit supports the font 
 *	object extension.  If so:
 *			init the cache of font geometries
 *			register a dynamic handler to update the cache
 *			when the fonts are changed dynamically
 *	Otherwise: set a flag to indicate that the font extension is
 *	       not being used.
 * INPUTS:
 * OUTPUTS:
 * GLOBALS: Desktop structure
 */
static void
DmInitDynamicFonts(void)
{
    Widget	fontObj =
	XtNameToWidget(XmGetXmDisplay(XtDisplay(DESKTOP_SHELL(Desktop))),
		       "fontObject");

    if (USE_FONT_OBJ(Desktop) = (fontObj != NULL))
    {
	XtSetArg(Dm__arg[0], XmNserifFamilyFontList, &DESKTOP_SERIF(Desktop));
	XtSetArg(Dm__arg[1], XmNsansSerifFamilyFontList,
		 &DESKTOP_SANS_SERIF(Desktop));
	XtSetArg(Dm__arg[2], XmNmonospacedFamilyFontList,
		 &DESKTOP_MONOSPACED(Desktop));
	XtGetValues(fontObj, Dm__arg, 3);

	DM_FontExtent(DESKTOP_SERIF(Desktop).font,
		      &DESKTOP_SERIF(Desktop).max_width,
		      &DESKTOP_SERIF(Desktop).max_height);

	DM_FontExtent(DESKTOP_SANS_SERIF(Desktop).font,
		      &DESKTOP_SANS_SERIF(Desktop).max_width,
		      &DESKTOP_SANS_SERIF(Desktop).max_height);

	DM_FontExtent(DESKTOP_MONOSPACED(Desktop).font,
		      &DESKTOP_MONOSPACED(Desktop).max_width,
		      &DESKTOP_MONOSPACED(Desktop).max_height);

	XtAddCallback(fontObj, XmNdynamicFontCallback, HandleFontUpdate, NULL);
    }

} /* end of DmInitDynamicFonts */

/***************************public*procedures*****************************
 *
 *		PUBLIC PROCEDURES
 */

/****************************procedure*header*****************************
 * DmPromptExit-
 */
void
DmPromptExit(Boolean shutdown)
{
    static Gizmo gizmo = NULL;

    static MenuItems	menubarItems[] = {
	MENU_ITEM( TXT_SAVELAYOUT_N_EXIT, TXT_M_SAVE_N_EXIT, ExitAndSaveCB ),
	MENU_ITEM( TXT_FILE_EXIT,   TXT_M_Exit,	ExitOnlyCB ),
	MENU_ITEM( TXT_CANCEL,      TXT_M_CANCEL,	CancelExitCB ),
	MENU_ITEM( TXT_HELP,      TXT_M_HELP,		ExitHelpCB ),
	{ NULL }				/* NULL terminated */
    };

    MENU_BAR("exitNoticeMenubar", menubar, NULL, 0, 2);

    static ModalGizmo notice_gizmo = {
	NULL,				/* help info */
	"exitNotice",			/* shell name */
	TXT_G_PRODUCT_NAME,		/* title */
	&menubar,			/* menu */
	TXT_END_SESSION,		/* message */
	NULL, 0,			/* gizmos, num_gizmos */
	XmDIALOG_FULL_APPLICATION_MODAL,/* style */
	XmDIALOG_WARNING		/* type */
    };

    DmBeep();

    /* Set client data each time for callbacks to work */
    menubarItems[0].clientData = menubarItems[1].clientData =
	menubarItems[2].clientData = (char *)shutdown;

    if (gizmo){
	XtAddCallback(GetModalGizmoShell(gizmo),
		      XtNdestroyCallback, DestroyCB, gizmo);
	XtDestroyWidget(GetModalGizmoShell(gizmo));
    }
    XtSetArg(Dm__arg[0], XmNmessageAlignment, XmALIGNMENT_CENTER);
    gizmo = CreateGizmo(DESKTOP_SHELL(Desktop), ModalGizmoClass,
			&notice_gizmo, Dm__arg, 1);

    /* register for context-sensitive help */
    XtAddCallback(GetModalGizmoDialogBox(gizmo), XmNhelpCallback,
		  ExitHelpCB, NULL);

    /* map the notice */
    MapGizmo(ModalGizmoClass, gizmo);
}					/* end of DmPromptExit */

/****************************procedure*header*****************************
 * DmSaveDesktopProperties-
 */
void
DmSaveDesktopProperties(DmDesktopPtr desktop)
{
	register int i;
	register DtPropPtr pp = DESKTOP_PROPS(desktop).ptr;
	char *path;
	FILE *f;

	path = DmMakePath(DmDTProp(DESKTOPDIR, NULL), ".dtprops");
	if ((f = fopen(path, "w")) == NULL) {
		Dm__VaPrintMsg(TXT_DTPROP_SAVE);
		return;
	}

	/*
	 * Save all desktop properties except DESKTOPDIR.
	 * DESKTOPDIR cannot be saved into this file, because it is used
	 * to find the fullpath of this file. A chicken & egg situation.
	 */
	for (i=DESKTOP_PROPS(desktop).count; i; i--, pp++)
		if (strcmp(pp->name, DESKTOPDIR))
			fprintf(f, "%s=%s\n", pp->name, pp->value);

	fclose(f);
}

/****************************procedure*header*****************************
 * DmShutdownPrompt-
 */
void
DmShutdownPrompt(void)
{
    static Gizmo 	gizmo = NULL;
    static char		tst[] = "/sbin/tfadmin -t /sbin/shutdown 2>/dev/null";
    ModalGizmo 		*notice;
    Widget		msg_box;
    static XtPopdownIDRec	popdown_rec;

    static MenuItems	privItems[] = {
	MENU_ITEM ( TXT_OK, TXT_M_OK, XtCallbackPopdown ),
	{ NULL }				/* NULL terminated */
    };
    MENU_BAR("privMenubar", priv, NULL, 0, 0);	/* default: "Ok" */

    static ModalGizmo priv_gizmo = {
	NULL,					/* help info */
	"privNotice",				/* shell name */
	TXT_G_PRODUCT_NAME,			/* title */
	&priv,					/* menu */
	TXT_CANT_SHUTDOWN,			/* message */
	NULL, 0,				/* gizmos, num_gizmos */
	XmDIALOG_MODELESS,			/* style */
	XmDIALOG_INFORMATION			/* type */
    };

    static MenuItems	shutdownItems[] = {
	MENU_ITEM(TXT_SHUTDOWN,	TXT_M_SHUTDOWN,	ShutdownCB ),
	MENU_ITEM(TXT_CANCEL,	TXT_M_CANCEL,	XtCallbackPopdown ),
	MENU_ITEM(TXT_HELP,	TXT_M_HELP,	ShutdownHelpCB ),
	{ NULL }				/* NULL terminated */
    };
    MENU_BAR("shutdownMenubar", shutdown, NULL, 1, 1);	/* default: Cancel */

    static ModalGizmo shutdown_gizmo = {
	NULL,					/* help info */
	"shutdownNotice",			/* shell name */
	TXT_G_PRODUCT_NAME,			/* title */
	&shutdown,				/* menu */
	TXT_CONFIRM_SHUTDOWN,			/* message */
	NULL, 0,				/* gizmos, num_gizmos */
	XmDIALOG_FULL_APPLICATION_MODAL,	/* style */
	XmDIALOG_WARNING			/* type */
    };

    privItems[0].clientData =
	shutdownItems[1].clientData = (char *)&popdown_rec;

    if (!gizmo)
    {
	/* Create the appropriate notice gizmo (once) and map it */
	notice = ((getuid() == 0) || (system(tst) == 0)) ?
	    &shutdown_gizmo : &priv_gizmo;

	XtSetArg(Dm__arg[0], XmNmessageAlignment, XmALIGNMENT_CENTER); 
	gizmo = CreateGizmo(DESKTOP_SHELL(Desktop), ModalGizmoClass,
			    notice, Dm__arg, 1);
	msg_box = GetModalGizmoDialogBox(gizmo);
	
	/* for "cancel" callbacks */
	popdown_rec.shell_widget = DtGetShellOfWidget(msg_box);	

	/* register for context-sensitive help */
	XtAddCallback(msg_box, XmNhelpCallback, ShutdownHelpCB, NULL);

    }
    DmBeep();

    /* FLH MORE: are these needed ?
    XtSetArg(Dm__arg[0], XtNgravity, WestGravity);
    XtSetArg(Dm__arg[1], XtNalignment, OL_LEFT);
    XtSetValues(notice->stext, Dm__arg, 2);
    */
    MapGizmo(ModalGizmoClass, gizmo);
}					/* DmShutdownPrompt */

