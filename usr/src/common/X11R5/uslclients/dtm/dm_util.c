/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dm_util.c	1.146.1.4"

/******************************file*header********************************

    Description:
	This file contains the source code for utility-type functions which are
	shared between the different components of dtm (Folders, Wastebasket,
	etc).
*/
						/* #includes go here	*/
#include <fmtmsg.h>
#include <limits.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <Flat.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Display.h>
#include <Xm/FontObj.h>
#include <Xm/Protocols.h>

#include <MGizmo/Gizmo.h>		/* for GetGizmoText */
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/PopupGizmo.h>		/* for SetPopupMessage */
#include <MGizmo/MsgGizmo.h>		/* for SetBaseWindowStatus */
#include <MGizmo/BaseWGizmo.h>		/* for SetBaseWindowStatus */
#include <MGizmo/ModalGizmo.h>
#include <memutil.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************
 *
 * Forward Procedure definitions
 */

static void VDisplayMsg(DmWinPtr, char *, va_list, int type);
static void DmWorkingFeedbackOff(XtPointer client_data, XtIntervalId * id);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* message types */
#define DT_STATE_MSG	0
#define DT_ERROR_MSG	1
#define DT_INFO_MSG	2

static const char DFLT_OPEN_CMD[] = "##DROP(dtedit) || exec dtedit %F &";
static int  (*prev_xerror_handler) (Display *, XErrorEvent *);

/***************************private*procedures****************************

    Private Procedures
*/

static int
CatchXError(Display * dpy, XErrorEvent * xevent)
{
    switch (xevent->error_code)
    {
    case BadWindow:			/* destroyed? */
    case BadMatch:			/* not viewable? */
    case BadAccess:
	/* Ignore these errors because there are chances, a window is
	 * unmapped by a window manager or is destroyed by an application,
	 * while assigning the input focus...
	 */
	break;
    default:
	/* Report other error as usual... */
	return((*prev_xerror_handler)(dpy, xevent));
	break;
    }
    return(1);
}					/* end of CatchXError */

static int
IgnoreGrabError(Display * dpy, XErrorEvent * xevent)
{
    switch (xevent->error_code)
    {
    case BadAccess:	
	/* already grabbed by another client */
	break;
    default:
	/* Report other error as usual... */
	return((*prev_xerror_handler)(dpy, xevent));
	break;
    }
    return(1);
}					/* end of IgnoreGrabError */

/****************************procedure*header*********************************
 * DiscardKeys: discard keyboard input to busy window
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 */
static void 
DiscardKeys(Widget w, XtPointer client_data, XEvent *event, 
	    Boolean *continue_to_dispatch)
{
    if (event->type == KeyPress || event->type == KeyRelease)
	*continue_to_dispatch = False;

}					/* end of DiscardKeys */

static void
ErrorNoticeDestroyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Gizmo gizmo = (Gizmo)client_data;
    XtDestroyWidget(GetModalGizmoShell(gizmo));
    FreeGizmo(ModalGizmoClass, gizmo);
}

static void
ErrorNoticePopdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(DtGetShellOfWidget(widget));
}

static void
DisplayErrorNotice(Widget widget, char *msg)
{
    static MenuItems menubarItems[] = {
	MENU_ITEM(TXT_OK, TXT_M_OK, ErrorNoticePopdownCB ),
	{ NULL }				/* NULL terminated */
    };
    MENU_BAR("errorNoticeMenubar", menubar, NULL, 0, 0);

    static ModalGizmo errorNoticeGizmo = {
	NULL,   			/* help info */
	"errorNotice",			/* shell name */
	TXT_G_PRODUCT_NAME,		/* title */
	&menubar,			/* menu */
	"",				/* message (run-time) */
	NULL, 0,			/* gizmos, num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL, /* style */
	XmDIALOG_ERROR,			/* type */
    };
    Gizmo	gizmo;
    Widget	shell;

    /* Create the modal gizmo */
    errorNoticeGizmo.message = msg;
    gizmo = CreateGizmo(widget, ModalGizmoClass,
			      &errorNoticeGizmo, NULL, 0);

    shell = GetModalGizmoShell(gizmo);
    XmAddWMProtocolCallback(shell, XA_WM_DELETE_WINDOW(XtDisplay(shell)),
			    ErrorNoticeDestroyCB, (XtPointer)gizmo);
    XtAddCallback(shell, XmNpopdownCallback,
		  ErrorNoticeDestroyCB, (XtPointer)gizmo);

    MapGizmo(ModalGizmoClass, gizmo);

}					/* end of DisplayErrorNotice */

/****************************procedure*header*****************************
 * VDisplayMsg- display message 'msg' in either the footer of 'window'
 *	or in a message box.  NULL msgs are interpreted to mean "redisplay
 *	the status line".
 */
static void
VDisplayMsg(DmWinPtr window, char *msg, va_list ap, int type)
{
    char buffer[1024];

    if (msg == NULL)			/* assume NULL msg means clear */
    {
	if (window->attrs | DM_B_SHOWN_MSG)
	{
	    window->attrs &= ~DM_B_SHOWN_MSG;

	    if (window->attrs & (DM_B_FOLDER_WIN | DM_B_WASTEBASKET_WIN))
		DmDisplayStatus(window);
	}
	return;
    }

    window->attrs |= DM_B_SHOWN_MSG;
    vsprintf(buffer, Dm__gettxt(msg), ap);

    if (window->attrs & DM_B_BASE_WINDOW)
    {
	switch(type)
	{
	case DT_STATE_MSG:
	    SetBaseWindowRightMsg(window->gizmo_shell, buffer);
	    break;
	case DT_ERROR_MSG:
	    DisplayErrorNotice(window->swin, buffer);
	    break;
	case DT_INFO_MSG:
	    SetBaseWindowLeftMsg(window->gizmo_shell, buffer);
	    SetBaseWindowRightMsg(window->gizmo_shell, "");
	    break;
	}
    } else
    {
	/* Display messages for non-base windows in a message box */
	DisplayErrorNotice(window->swin, buffer);
    }
}					/* end of VDisplayMsg */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*********************************
 *  DmWorkingFeedback: show clock cursor over a folder window to indicate
 *		we are working on something.  Optionally busy the window
 *		as well.
 *	INPUTS: folder window - window to set cursor on
 *		interval - duration of cursor
 *		busy - whether to actually busy the window
 *	OUTPUTS: none
 *	GLOBALS:
 *****************************************************************************/
void
DmWorkingFeedback(DmFolderWindow folder, unsigned long interval, Boolean busy)
{
   Widget 	       shell;
   Display *             dpy;
   Window            window;
   XtAppContext      context;


   if (folder == NULL || folder->busy_id)
       /* Folder already showing working cursor */
       return;

   shell = folder->shell;
   dpy = XtDisplayOfObject(shell);
   window = XtWindowOfObject(shell);
   context = XtDisplayToApplicationContext(dpy);

   if (busy)
   {
       XtSetSensitive(folder->shell, False);
       XmUpdateDisplay(folder->shell);
   }
   XDefineCursor(dpy, window, DtGetBusyCursor(shell));
   folder->busy_id = XtAppAddTimeOut(context, interval, DmWorkingFeedbackOff, folder);
}	/* end of DmWorkingFeedback */

static void
DmWorkingFeedbackOff(XtPointer client_data, XtIntervalId * id)
{
    DmFolderWindow folder = (DmFolderWindow) client_data;
    Display          *dpy = XtDisplay(folder->shell);
    Window            win = XtWindow(folder->shell);

    folder->busy_id = 0;
    XUndefineCursor(dpy, win);
    XtSetSensitive(folder->shell, True);

}					/* end of DmWorkingFeedbackOff */

/*****************************************************************************
 *      DmBusyWindow- busy/unbusy a window
 *	INPUTS: window to busy/unbusy
 *		boolean flag  (True --> make window busy, False --> unbusy)
 *	OUTPUTS:
 *	GLOBALS:
 */
void
DmBusyWindow(Widget shell, Boolean busy)
{
    Display *	dpy = XtDisplay(shell);
    Window	win = XtWindow(shell);
    Widget	focus_w = XmGetFocusWidget(shell);

    if (focus_w && !XtIsWidget(focus_w))
	focus_w = XtParent(focus_w);

#ifdef NOT_USED
    /* Insenstive window has annoying visual (flashing) */
    XtSetSensitive(shell, busy == False ? True : False);
#endif

    if (busy == True)
    {
	Cursor cursor = DtGetBusyCursor(shell);

	XDefineCursor(dpy, win, cursor);
	XSync(dpy, False);
	prev_xerror_handler = XSetErrorHandler(IgnoreGrabError);
	XGrabButton(dpy, AnyButton, AnyModifier, win, False,
		    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		    PointerMotionMask | EnterWindowMask | LeaveWindowMask,
		    GrabModeAsync, GrabModeAsync, (Window)None, cursor);
	XGrabKey(dpy, AnyKey, AnyModifier, win, False,
		 GrabModeAsync, GrabModeAsync);
	XSync(dpy, False);
	(void)XSetErrorHandler(prev_xerror_handler);
	if (focus_w)
	    XtInsertEventHandler(focus_w, KeyPressMask | KeyReleaseMask, 
				 False, DiscardKeys, NULL, XtListHead);
    } else
    {
	XUndefineCursor(dpy, win);
	XUngrabButton(dpy, AnyButton, AnyModifier, win);
	XUngrabKey(dpy, AnyKey, AnyModifier, win);
	if (focus_w)
	    XtRemoveEventHandler(focus_w, KeyPressMask | KeyReleaseMask,  
				 False, DiscardKeys, NULL);
    }
}				/* End of DmBusyWindow */

/****************************procedure*header*****************************
    DmDropCommand-
*/
int
DmDropCommand(DmWinPtr wp, DmObjectPtr op, char * name)
{
#ifdef MOTIF_DND
	Display *dpy = XtDisplay(wp->views[0].box);
	Atom app_id = XInternAtom(dpy, name, False);
	Window app_win;

	if ((app_id != None) &&
	    ((app_win = DtGetAppId(dpy, app_id)) != None)) {
		XWindowAttributes win_attrs;
		OlDnDDragDropInfo root_info;

		/*
		 * We have to get the root_window id. Sigh.
		 */
		XGetWindowAttributes(dpy, app_win, &win_attrs);

		/* DM_B_SEND_EVENT indicates a DnD trigger message is
		 * to be simulated. Not supported yet.
		 */
	    	if (DmDnDNewTransaction((DmWinPtr)wp,
			(DmItemPtr *)DmOneItemList(DmObjectToItem(wp, op)),
			DM_B_SEND_EVENT,
			app_win,
			XmDROP_COPY,
			DmConvertSelectionProc,
			DmDnDFinishProc))
			return(1);
	}
#endif /* MOTIF_DND */
	return(0);
}				/* end of DmDropCommand */

/****************************procedure*header*****************************
    DmDropObject-
*/
void
DmDropObject(DmWinPtr dst_win, Cardinal dst_indx, DmWinPtr src_wp, void *src,
	void **src_list, char *(*expand_proc)(), int flag)
{
    char *	p;
    DmObjectPtr	dst_op;

    if (!(ITEM_SENSITIVE(DM_WIN_ITEM(dst_win, dst_indx))))
	return;				/* Return now if dst item is busy */

    /* busy the dst item */
    XtSetArg(Dm__arg[0], XmNsensitive, False);
    ExmFlatSetValues(dst_win->views[0].box, dst_indx, Dm__arg, 1);
#ifdef MOTIF_OLUPDATEDISPLAY
    OlUpdateDisplay(dst_win->views[0].box);	/* make it look busy immediately */
#endif /* MOTIF_OLUPDATEDISPLAY */

    /* execute drop command (if any) */
    dst_op = ITEM_OBJ(DM_WIN_ITEM(dst_win, dst_indx));
    if ( (p = DmGetObjProperty(dst_op, DROPCMD, NULL)) != NULL )
    {
	DmSetSrcWindow(src_wp);
	if (flag & MULTI_PATH_SRCS)
		DmSetExpandFunc(expand_proc, src_list, flag);
	else
		DmSetSrcObject((DmObjectPtr)src);

	p = Dm__expand_sh(p, DmDropObjProp, (XtPointer)dst_op);
#ifdef DEBUG
		printf("dropcmd=%s\n", p);
#endif

	/*
	 * set force_chdir to True only if the source window is a
	 * folder window.  CAUTION: src_wp can be NULL!
	 */
	if (src_wp && src_wp->attrs & DM_B_FOLDER_WIN)
		(void)DmExecuteShellCmd(src_wp, (DmObjectPtr)src, p, True);
	else
		(void)DmExecuteShellCmd(src_wp, (DmObjectPtr)src, p, False);

	FREE(p);

    } else
    {
	/* put the default behavior here */
    }

    /* un-busy the dst item */
    XtSetArg(Dm__arg[0], XmNsensitive, True);
    ExmFlatSetValues(dst_win->views[0].box, dst_indx, Dm__arg, 1);

}				/* end of DmDropObject */

/****************************procedure*header*****************************
    DmExecCommand-
*/
int
DmExecCommand(DmWinPtr wp, DmObjectPtr op, char * name, char * str)
{
#ifdef DEBUG
printf("DmExecCommand: name=:%s: str=:%s:\n", name, str);
#endif
    return(DmDispatchRequest(wp->views[0].box,
			     XInternAtom(XtDisplay(wp->views[0].box), name, False),
			     str));

}				/* end of DmExecuteCommand */

void
DmApplyLaunchFromCWD(Boolean force_chdir)
{
	LAUNCH_FROM_CWD(Desktop) = force_chdir;
}

/****************************procedure*header*****************************
 * DmExecuteShellCmd- return 1 for success, 0 otherwise.
 */
int
DmExecuteShellCmd(DmWinPtr wp, DmObjectPtr op, char * cmdstr,
		  Boolean force_chdir)
{

#define FINDCHAR(P,C)	while (*(P) != C) (P)++
#define EATSPACE(P,C)	while (*(P) == C) (P)++

#define CMD_PREFIX	"##"
#define CMD_PREFIX_LEN	2

    char *	p2;
    char *	name;
    int		namelen;
    char *	cmd;
    char *	cmd_end;
    char *	selection;
    int		found;
    int		done;
    int		ret = 0;

 loop:
    done = 0;
    EATSPACE(cmdstr, ' ');
    if (*cmdstr == '\0')
	return(ret);

    if (!strncmp(cmdstr, CMD_PREFIX, CMD_PREFIX_LEN)) {
	/* special command */
	name = cmdstr + CMD_PREFIX_LEN;

	/* get name */
	for (p2=name; *p2 && (*p2 != '('); p2++);
	namelen = p2 - name;
	p2++;				/* skip '(' */

	/* get end of special command */
	for (cmd=p2; *p2 && (*p2 != ')'); p2++) {
	    /* handle matching quotes inside parenthesis */
	    if (*p2 == '"') {
		FINDCHAR(p2, '"');
	    }
	    if (*p2 == '\'') {
		FINDCHAR(p2, '\'');
	    }
	}
	cmd_end = p2;

	name = strndup(name, namelen);
	if (!strcmp(name, "DROP")) {
	    if (wp == NULL)
	    {
#ifdef DEBUG
		printf("##DROP and wp==NULL\n");
#endif
		FREE(name);
		return(0);
	    }

	    selection = strndup(cmd, cmd_end - cmd);
	    ret = DmDropCommand(wp, op, selection);
	    FREE(selection);
	}
	else if (!strcmp(name, "DELETE")) {
		DmMoveToWBProc2(wp ? DmObjPath(op) : (char *)op,
				NULL, NULL, NULL);
	    ret = 1;			/* ie, success */
	}
	else if (!strcmp(name, "COMMAND")) {
	    char *comma;
	    char *str;

	    if (wp == NULL)
	    {
#ifdef DEBUG
		printf("##COMMAND and wp==NULL\n");
#endif
		FREE(name);
		return(00);
	    }
	    /* find the comma */
	    comma = cmd;
	    while ((comma < cmd_end) && (*comma != ','))
		comma++;
	    if (*comma != ',') {
		Dm__VaPrintMsg(TXT_MISSING_COMMA, cmd);
		return(0);
	    }
	    selection = strndup(cmd, comma - cmd);
	    str = strndup(comma+1, cmd_end-comma-1);
	    ret = DmExecCommand(wp, op, selection, str);
	    FREE(str);
	    FREE(selection);
	}
	else {
	    FREE(name);
	    Dm__VaPrintMsg(TXT_BAD_NAME, name);
	    return(-1);
	}
	FREE(name);

	cmdstr = cmd_end;
	if (cmdstr)
	    cmdstr++;			/* skip ')' */
	EATSPACE(cmdstr, ' ');
	done = 1;
    }			/* !strncmp(cmdstr, CMD_PREFIX, CMD_PREFIX_LEN))  */

    /* find end of command ";##", "|| ##", or "&& ##" */
    for (p2=cmdstr, found=0; *p2 && !found; p2++) {
	switch(*p2) {
	case '"':
	    FINDCHAR(p2, '"');
	    break;
	case '\'':
	    FINDCHAR(p2, '\'');
	    break;
	case ';':
	    p2++;
	    cmd_end = p2;
	    EATSPACE(p2, ' ');
	    if ((*p2 == '#') && (*(p2 + 1) == '#')) {
		found++;
		break;
	    }
	    break;
	case '|':
	case '&':
	    if (*(p2 + 1) == *p2) {
		cmd_end = p2;
		p2++;
		p2++;
		EATSPACE(p2, ' ');

		if (done) {
		    found++;
		    break;
		}

		if ((*p2 == '#') && (*(p2 + 1) == '#')){
		    found++;
		    break;
		}
	    }
	}
    }

    if (!found)
	cmd_end = p2;

    if (!done) {
	char *	dir;

	cmd = strndup(cmdstr, cmd_end - cmdstr);
#ifdef DEBUG
	printf("system(%s)\n", cmd);
#endif
	switch (fork())
	{
	case 0:
	    /* Reset signals that we're catching until we exec() below.
	     * (SIGCLD is still ignored for the moment).
	     */
	    signal(SIGHUP,SIG_DFL);
	    signal(SIGTERM,SIG_DFL);
	    signal(SIGINT,SIG_DFL);

	    /* Change child to appropriate working directory */
	    if (op)
	    {
		if ((dir = DmGetObjProperty(op, OBJCWD, NULL)) == NULL)
		    dir = (force_chdir || (op->ftype == DM_FTYPE_DATA)) ?
			op->container->path : DESKTOP_HOME(Desktop);
	    } else
		dir = DESKTOP_HOME(Desktop);
	    chdir(dir);

	    /* Reset signals that are ignored so child doesn't ignore them,
	     * too.  (Exec will not carry over signal catchers so only need
	     * to restore signals that are ignored.)
	     */
	    signal(SIGCLD, SIG_DFL);
	    (void) execl ("/sbin/sh", "sh","-c", cmd, (String)0);
	    exit(errno);

	case -1:
	    ret = 0;			/* ie, failure */
	    break;

	default:
	    /* For now, always assume success */
	    /* (void) wait(&ret); */
	    ret = 1;
	}
	FREE(cmd);
    }

    if (found) {
	cmdstr = cmd_end;
	switch(*cmd_end) {
	case ';':
	    cmdstr++;
	    break;
	case '|':
	    if (ret)
		return(ret);
	    else {
		/* skip "||" */
		cmdstr++;
		cmdstr++;
	    }
	    break;
	case '&':
	    if (ret) {
		/* skip "&&" */
		cmdstr++;
		cmdstr++;
	    }
	    else
		return(ret);
	    break;
	case '\0':
	    /* done */
	    return(ret);
	default:
	    Dm__VaPrintMsg(TXT_SHELL_SYNTAX, cmdstr);
	    return(0);			/* ie, failure */
	}

	goto loop;
    }
    return(ret);
}					/* end of DmExecuteShellCmd */

/****************************procedure*header*****************************
    Dm__gettxt-
*/
char *
Dm__gettxt(char * msg)
{
	static char msgid[6 + 10] = "dtmgr:";

	strcpy(msgid + 6, msg);
	return(gettxt(msgid, msg + strlen(msg) + 1));
}

/*****************************************************************************
 *  	DmLockContainerList: lock/unlock container list
 *	At times we must loop through the list of containers
 *	and notify their dependents of container changes.
 *	Dependents may choose to close one or more containers
 *	in reponse to those changes, but we don't want the
 *	list to become shuffled or to contain dangling
 *	references to recently destroyed containers.  This means
 *	we must be the last person to close any container.
 *	So, we increment the reference count for all containers
 *	before the operations and decrement the reference count
 *	(and close, if necessary) after the operations.
 *	Calls to this routine should wrap calls to DmDestroyContainer.
 *	
 *	FLH MORE: This is awkward and error prone.  It needs more thought.
 *	INPUTS:  Boolean (True == Lock ; False == Unlock)
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmLockContainerList(Boolean lock)
{
    int 		i;
    DmContainerBuf 	*containers;

    if (lock){
	/* increment reference counts */
	containers = &(DESKTOP_CONTAINERS(Desktop));
	for (i=0; i<containers->used; i++)
	    containers->p[i]->count++;
    }
    else{
	containers = (DmContainerBuf *) 
	    CopyBuffer((Buffer *) &(DESKTOP_CONTAINERS(Desktop)));
	for (i = 0; i < containers->used; i++){

	    _DPRINT3(stderr, 
		     "DmLockContainer: Decrementing refcount for %s to %d\n",
		     containers->p[i]->path, containers->p[i]->count-1);

	    if (containers->p[i]->count == 1)
		DmCloseContainer(containers->p[i], 0);
	    else
		containers->p[i]->count--;
	}
	FreeBuffer((Buffer *) containers);
    }
}	/* end of DmLockContainerList */


/****************************procedure*header*****************************
    DmMakeFolderTitle- makes folder title given 'path'.
	If "first" folder, give it special product name title.
	Otherwise, title is full path or basename depending on current option.
	Note: result is "constant" string or returned in buf.
*/
char *
DmMakeFolderTitle(DmFolderWindow window)
{
    static char *prefix = NULL;
    static char *remote_prefix = NULL;
    char *name;
    char *path = DM_WIN_USERPATH(window);
    static char buf[PATH_MAX];

    if (!prefix)
    {
        char *logname;
        char *prodname;
        char *buffer;

        /* First Time: also assume this is the main window. */
        prefix = Dm__gettxt(TXT_FOLDER_PREFIX);
        remote_prefix = Dm__gettxt(TXT_REMOTE_FOLDER_PREFIX);
        prodname = Dm__gettxt(TXT_PRODUCT_NAME);
        if ((logname = cuserid(NULL)) == NULL)
            logname = "???";

        /* 4 is " - " + NULL char */
        buffer = malloc(strlen(logname) + strlen(prodname) + 4);
        sprintf(buffer, "%s - %s", prodname, logname);

        /* NOTE: buffer is never freed */
        return(buffer);
    }
    /* visually distinguish remote folders in title bar */
    if (IS_REMOTE(Desktop, window->views[0].cp->fstype))
	strcpy(buf, remote_prefix); /* Copy in prefix */
    else
	strcpy(buf, prefix); /* Copy in prefix */

    /* Always use the icon label for UUCP_Inbox, Netware, Disk_A, Disk_B,
     * and CD-ROM folders in their window title and disregard
     * SHOW_FULL_PATH. See comments in f_create.c:DmOpenFolderWindow() for
     * more details.
     */
    if (strstr(path, UUCP_RECEIVE_PATH)) {
        strcat(buf, GetGizmoText(TXT_UUCP_INBOX_TITLE));
        return(buf);
    } else if (!strcmp(DM_WIN_PATH(window), NETWARE_PATH)) {
        strcat(buf, GetGizmoText(TXT_NETWARE_TITLE));
        return(buf);
    } else if (!strcmp(DM_WIN_PATH(window), DISK_A)) {
        strcat(buf, Dm_DayOneName("Disk_A", DESKTOP_LOGIN(Desktop)));
        return(buf);
    } else if (!strcmp(DM_WIN_PATH(window), DISK_B)) {
        strcat(buf, Dm_DayOneName("Disk_B", DESKTOP_LOGIN(Desktop)));
        return(buf);
    } else if (!strncmp(DM_WIN_PATH(window), CD_ROM, strlen(CD_ROM))) {
        strcat(buf, Dm_DayOneName(basename(DM_WIN_PATH(window)), 
	       DESKTOP_LOGIN(Desktop)));
        return(buf);
    }
    /* Now add path part. Note: "/" is special since basename("/") "fails". */
    name = (SHOW_FULL_PATH(Desktop) || ROOT_DIR(path)) ?
        path : basename(path);
    strcat(buf, name);
    return(buf);

}					/* end of DmMakeFolderTitle */

/****************************procedure*header*****************************
    DmObjectToIndex-
*/
int
DmObjectToIndex(DmWinPtr wp, DmObjectPtr op)
{
    int			num_items = wp->views[0].nitems;
    register DmItemPtr	ip;
    register int	i;

    for (i = num_items, ip = wp->views[0].itp; i ; i--, ip++) {
	if ((ITEM_MANAGED(ip) != False) && (ITEM_OBJ(ip) == op))
	    return(i);
    }
    return(ExmNO_ITEM);
}				/* End of DmObjectToIndex */

/****************************procedure*header*****************************
    DmObjectToItem-
*/
DmItemPtr
DmObjectToItem(DmWinPtr wp, DmObjectPtr op)
{
    int			num_items = wp->views[0].nitems;
    register DmItemPtr	ip;
    register int	i;

    for (i = num_items, ip = wp->views[0].itp; i ; i--, ip++) {
	if ((ITEM_MANAGED(ip) != False) && (ITEM_OBJ(ip) == op))
	    return(ip);
    }
    return(NULL);
}				/* End of DmObjectToItem */

/****************************procedure*header*****************************
    DmObjNameToItem-
*/
DmItemPtr
DmObjNameToItem(DmWinPtr win, register char * name)
{
    register DmItemPtr item;

    for (item = win->views[0].itp; item < win->views[0].itp + win->views[0].nitems; item++)
	if (ITEM_MANAGED(item) && (strcmp(ITEM_OBJ_NAME(item), name) == 0) )
	    return(item);

    return(NULL);
}				/* End of DmObjectToIndex */

/****************************procedure*header*****************************
 *   DmOpenObject-
 *	
 *     INPUT: folder from which operation initiated
 *	      object to be opened
 *	      attrs: flags for folders only
 *		DM_B_OPEN_NEW	 force a new window (don't raise existing)
 *		DM_B_OPEN_PLACE  open in current folder window
 ****************************procedure*header*****************************/
void
DmOpenObject(DmWinPtr wp, DmObjectPtr op, DtAttrs attrs)
{
    char *p = NULL;

    if ((p = DmGetObjProperty(op, OPENCMD, NULL)) == NULL) {
	/* kai, try not to make this an exception */
	/* Use the default Open command depending on file type */
	if (OBJ_IS_DIR(op))
	{
	    DmFolderWinPtr fwp;

	    /* Preserve record of symbolic links in user-traversed path by
	     * using window->user_path
	     */
	    p = strdup(DM_OBJ_USERPATH(((DmFolderWindow) wp), op));
	    if (attrs & DM_B_OPEN_IN_PLACE)
		fwp = DmOpenInPlace((DmFolderWinPtr)wp, p);	/* will destroy object */
	    else
		fwp = DmOpenFolderWindow(p, attrs, NULL, False);
	    if (fwp == NULL)
		DmVaDisplayStatus(wp, True, TXT_OpenErr, p);
	    FREE(p);
	    return;
	}

	/* ultimate OPEN command default for data files */
	if (op->ftype == DM_FTYPE_DATA) {
		p = (char *)DFLT_OPEN_CMD;
	}
	else
		/* No default for others */
		return;
    }

    p = Dm__expand_sh(p, DmObjProp, (XtPointer)op);
    DmExecuteShellCmd(wp, op, p, (Boolean)LAUNCH_FROM_CWD(Desktop));
    FREE(p);
} /* end of DmOpenObject */

/****************************procedure*header*****************************
 *  DmQueryFolderWindow- search the list of folders for a folder that
 *  matches a given path.
 *
 *  INPUT: path
 *         options: a bitmask from the following
 *		DM_B_NOT_ROOTED		 don't return rooted dirs 
 *		DM_B_NOT_DESKTOP	 don't return desktop window 
 *	ASSUMPTIONS: the desktop folder is the first folder, we don't 
 *		call DmQueryFolderWindow before the desktop folder has been 
 *		inited
 *
 * FLH MORE: path should be "real" path, not symbolic link path.
 ****************************procedure*header*****************************/
DmFolderWinPtr
DmQueryFolderWindow(register char * path, DtAttrs options)
{
    register DmFolderWinPtr folder;
    char *real_path = realpath(path, Dm__buffer);
    DmFolderWinPtr	desktop_folder = DESKTOP_TOP_FOLDER(Desktop);

	/* This if is added to prevent possible core dump. */
    if (!real_path) 
	return NULL;

    for (folder = DESKTOP_FOLDERS(Desktop);
	 folder != NULL; folder = folder->next)
	if ((strcmp(real_path, folder->views[0].cp->path) == 0) &&
	    (!(options & DM_B_NOT_ROOTED) || folder->root_dir == NULL) &&
	    (!(options & DM_B_NOT_DESKTOP) || (folder != desktop_folder)))
	    break;

    return(folder);
}				/* end of DmQueryFolderWindow */

/****************************procedure*header*****************************
    DmSameOrDescendant-
		< 0 if path2 is the same as path1,
		> 0 if path2 is a descendant of path1
		= 0 otherwise.

	The caller can pass 'len' to optimize (for loop processing, for
	instance), otherwise, 'len' is calculated.
*/
int
DmSameOrDescendant(char * path1, char * path2, int path1_len)
{
    _DPRINT3(stderr, "DmSameOrDescendant: %s %s\n", path1, path2);

    if (path1_len <= 0)
	path1_len = strlen(path1);

    return((strncmp(path1, path2, path1_len) != 0) ? 0 :
	   (path2[path1_len] == '\0') ? -1 :
	   (path2[path1_len] == '/') ? 1 : 0);

}					/* end of DmSameOrDescendant */

/****************************procedure*header*****************************
    DmTouchIconBox- append "items touched" args to 'args_in' and do
	XtSetValues.  (Assumes args_in is large enough.  Typically, Dm__arg
	is used.)
*/
void
DmTouchIconBox(DmWinPtr window, ArgList args_in, Cardinal num_args)
{
    ArgList args;

    _DPRINT1(stderr,"DmTouchIconBox: %s\n", DM_WIN_PATH(window));
    if (args_in == NULL)
    {
	args = Dm__arg;
	num_args = 0;

    } else
	args = args_in;

    XtSetArg(args[num_args], XmNitems, window->views[0].itp); num_args++;
    XtSetArg(args[num_args], XmNnumItems, window->views[0].nitems); num_args++;
    XtSetArg(args[num_args], XmNitemsTouched,	True); num_args++;
    XtSetValues(window->views[0].box, args, num_args);

}					/* End of DmTouchIconBox */

/****************************procedure*header*****************************
    DmUpdateFolderTitle- generate new folder title and display it.
	Note: at the time of this writing, it is not necessary to update
	the gizmo ('base->title').
*/
void
DmUpdateFolderTitle(DmFolderWindow folder)
{
    XTextProperty	text_prop;
    char *		list[2];

    list[0] = strdup(DmMakeFolderTitle(folder));
    XStringListToTextProperty(list, 1, &text_prop);
    XSetWMName(XtDisplay(folder->shell), XtWindow(folder->shell), &text_prop);
    XtFree(list[0]);
}

/****************************procedure*header*****************************
    DmVaDisplayState- display message 'msg' in "state/mode" part of
	'window' footer.
*/
void
DmVaDisplayState(DmWinPtr window, char * msg, ... )
{
    va_list ap;

    va_start (ap, msg);
    VDisplayMsg(window, msg, ap, DT_STATE_MSG);
    va_end(ap);
}

/****************************procedure*header*****************************
    DmVaDisplayStatus-
*/
void
DmVaDisplayStatus(DmWinPtr window, int type, char * msg, ... )
{
    va_list ap;

    if (type)
	DmBeep();

    va_start (ap, msg);
    VDisplayMsg(window, msg, ap, type ? DT_ERROR_MSG : DT_INFO_MSG);
    va_end(ap);
}

/****************************procedure*header*****************************
    Dm__VaPrintMsg-
*/
void
Dm__VaPrintMsg(char *format, ... )
{
	va_list	ap;
	char buffer[2048];

	/* format message using var args */
	va_start(ap, format);
	vsprintf(buffer, Dm__gettxt(format), ap);
	va_end(ap);

	/* should use fmtmsg() */
	fprintf(stderr, "UX: dtm: %s\n", buffer);
}

void
DmAddWindow(list, newp)
DmWinPtr *list;
DmWinPtr newp;
{
	newp->next = NULL;

	if (*list == NULL)
		*list = newp;
	else {
		register DmWinPtr wp;

		for (wp=*list; wp->next; wp=wp->next);

		wp->next = newp;
	}
}


void
DmSetToolbarSensitivity(DmWinPtr window, int numselected)
{
	Widget menu_w;

	if (numselected == 0) {
		XtSetArg(Dm__arg[0], XmNsensitive, False);
	}
	else {
		XtSetArg(Dm__arg[0], XmNsensitive, True);
	}
	menu_w = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				    GetGizmoWidget, "toolbarMenu:5");
	XtSetValues(menu_w, Dm__arg, 1);
	menu_w = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				    GetGizmoWidget, "toolbarMenu:6");
	XtSetValues(menu_w, Dm__arg, 1);
	menu_w = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				    GetGizmoWidget, "toolbarMenu:8");
	XtSetValues(menu_w, Dm__arg, 1);
	menu_w = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				    GetGizmoWidget, "toolbarMenu:10");
	XtSetValues(menu_w, Dm__arg, 1);
	if (numselected  && (window->views[0].cp->attrs & DM_B_NO_LINK)) 
		XtSetArg(Dm__arg[0], XmNsensitive, False);
	menu_w = (Widget)QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
				    GetGizmoWidget, "toolbarMenu:7");
	XtSetValues(menu_w, Dm__arg, 1);
}


void
DmDisplayStatus(window)
DmWinPtr window;
{
	static Boolean first = True;
	int numselected = 0;
	int nitems = 0;
	int i;
	Widget status;
	DmItemPtr ip;
	char lbuff[64];
	char rbuff[64];

	/* figure out the # of items and # of selected items */
	for (ip=window->views[0].itp, i=window->views[0].nitems; i; i--, ip++)
		if (ITEM_MANAGED(ip)) {
			nitems++;
			if (ITEM_SELECT(ip))
				numselected++;
		}

	/*
	 * Set the sensitivity on the toolbar buttons.
	 * Don't do this the first time - it will be done in
	 * DmOpenFolderWindow.  This is because the label must
	 * be sensitive inorder for the label to have size.  If
	 * the label were set to insensitive here then the labels
	 * would be very small.
	 */
	if (first == False) {
		DmSetToolbarSensitivity(window, numselected);
	}
	first = False;

	/*
	 * Update selectCount at this point.
	 * Because some d&d operations didn't update this correctly.
	 */
	XtSetArg(Dm__arg[0], XmNselectCount, numselected);
	XtSetValues(window->views[0].box, Dm__arg, 1);

	/*
	 * Depending on window type, do something different here.
	 */
	sprintf(lbuff, Dm__gettxt(TXT_SELECTED_ITEMS), numselected);
	sprintf(rbuff, Dm__gettxt(TXT_TOTAL_ITEMS), nitems);

	SetBaseWindowLeftMsg(window->gizmo_shell, lbuff);
	SetBaseWindowRightMsg(window->gizmo_shell, rbuff);
}

/*****************************************************************************
 *  	DmRetypeObj:	reset the file class for an object
 *			FLH MORE: should we notify all views
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmRetypeObj(DmObjectPtr op, Boolean notify)
{
    DmContainerCallDataRec call_data;
    DmContainerPtr cp = op->container;
	

	DmSetFileClass(op);
	DmInitObjType(DESKTOP_SHELL(Desktop), op);
	
	if (notify){
	    call_data.reason = DM_CONTAINER_OBJ_CHANGED;
	    call_data.cp = cp;
	    call_data.detail.obj_changed.obj = op;
	    call_data.detail.obj_changed.attrs = DM_B_OBJ_TYPE;
	    DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);
	}

}

/*****************************************************************************
 *  	DmSyncWindows: 
 *	NOTE: we don't need to handle the wastebasket, and helpdesk,
 *	      because they use their own special file classes.
 *
 *	      FLH MORE: objects in the folder map will not be
 *	      reclassed because the folder map container is not
 *	      in the containers list.
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/

void
DmSyncWindows(new_fnkp, del_fnkp)
DmFnameKeyPtr new_fnkp;
DmFnameKeyPtr del_fnkp;
{
	register DmObjectPtr op;
	DmFnameKeyPtr fnkp;
	int refresh;
	int i;
	int done;
	DmContainerBuf containers;
	DmContainerPtr cp;
	DmContainerCallDataRec call_data;
	DtAttrs attrs;

	/* containers */
	containers = DESKTOP_CONTAINERS(Desktop);

	/* loop through all containers */
	for (i=0; i<containers.used; i++){
	    cp = containers.p[i];
	    refresh = 0;
	    for (op=cp->op; op; op=op->next) {
		done = 0;

		/* skip hidden files */
		if (op->attrs & DM_B_HIDDEN)
		    continue;

		/* skip files with locked or dontchg _CLASS property */
		if (DmGetObjProperty(op, OBJCLASS, &attrs) &&
			attrs & (DT_PROP_ATTR_LOCKED | DT_PROP_ATTR_DONTCHANGE))
			continue;

		/* check for deleted classes */
		for (fnkp=del_fnkp; fnkp; fnkp=fnkp->next) {
		    if (op->fcp->key == fnkp) {
			DmFclassPtr save_fcp = op->fcp;
			
			DmRetypeObj(op, False);
			if (op->fcp != save_fcp) {
			    refresh++;
			    done++;
			    break;
			}
		    }
		}

		if (done)
		    continue;
		
		/* check for new or changed classes */
		for (fnkp=new_fnkp; fnkp; fnkp=fnkp->next) {
		    if (fnkp->attrs & DM_B_REPLACED) {
			DmFclassPtr save_fcp = op->fcp;
			DmRetypeObj(op, False);
			if (op->fcp != save_fcp) {
			    refresh++;
			    done++;
			    break;
			}
		    }
		    else if (op->fcp->key == fnkp)
			break;
		}
	    }

	    if (refresh){
		/* Notify dependents that objects in this container
		 * have changed.  Notification done here
		 * instead of in DmRetypeObject because
		 * the file class may have changed even though
		 * the object has not changed classes.
		 */
		/* FLH MORE: We should pass info about the specific
		 * files changed?
		 */
		call_data.reason = DM_CONTAINER_SYNC;
		call_data.cp = cp;
		DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);
	    }
	} /* for each container */
} /* end of DmSyncWindows */

/*
 * This function searches for the specified file name in the class database.
 * If found, a ptr to the file class header is returned.
 */
DmFclassFilePtr
DmCheckFileClassDB(filename)
char *filename;
{
	register DmFclassFilePtr fcfp=(DmFclassFilePtr)DESKTOP_FNKP(Desktop);

	for (; fcfp; fcfp=fcfp->next_file)
		if (!strcmp(fcfp->name, filename))
			break;

	return(fcfp);
}

/*
 * This function looks at the item. If it is currently selected, then it
 * returns an array of selected items. If it is not selected, then it returns
 * an array with only the item in it.
 */
void **
DmGetItemList(window, item_index)
DmWinPtr window;
int item_index;		/* item being operated on */
{
	void **list = NULL;

	if ((item_index != ExmNO_ITEM) &&
	    !ITEM_SELECT(DM_WIN_ITEM(window, item_index)) && 
	    ITEM_SENSITIVE(DM_WIN_ITEM(window, item_index))) {
	    /* return one entry */
	    return(DmOneItemList(DM_WIN_ITEM(window, item_index)));
	}
	else {
		/* return an array of selected items */
		int i;
		DmItemPtr ip;
		void **lp;

		/* get the # of selected items */
		XtSetArg(Dm__arg[0], XmNselectCount, &i);
		XtGetValues(window->views[0].box, Dm__arg, 1);

		if (i > 0 && (list = (void **)MALLOC(sizeof(void *) * ++i))) {
			for (ip=window->views[0].itp,i=window->views[0].nitems,
			  lp=list; i; i--,ip++)

			    if (ITEM_MANAGED(ip) && ITEM_SELECT(ip) &&
				ITEM_SENSITIVE(ip))
				*lp++ = ip;
			*lp = NULL; /* a NULL terminated list */
		}
	}

	return(list);
}

/*
 * This function builds a one entry item list.
 */
void **
DmOneItemList(DmItemPtr ip)
{
	void **ilist = (void **)MALLOC(sizeof(void *) * 2);

	if (ilist) {
		ilist[0] = ip;
		ilist[1] = NULL;
	}

	return(ilist);
}

/*
 * This function converts a list of item ptrs to a list of filenames & a count.
 * The purpose of this convertion is to accommodate current interface of the
 * file operation code during a transition period.
 */
char **
DmItemListToSrcList(ilist, count)
void **ilist;		/* NULL terminated item list */
int *count;
{
	DmItemPtr *ipp;
	char **src_list = NULL;


	if (ilist == NULL){
	    *count = 0;
	    return(NULL);
	}
	for (*count=0, ipp=(DmItemPtr *)ilist; *ipp; ipp++, (*count)++) ;

	if (src_list = (char **)MALLOC(sizeof(char *) * *count)) {
		char **sp;

		for (sp=src_list, ipp=(DmItemPtr *)ilist; *ipp; ipp++)
    			*sp++ = strdup(ITEM_OBJ(*ipp)->name);
	}

	return(src_list);
}

char *
DmClassName(fcp)
DmFclassPtr fcp;
{
	char *classname;

	if (classname = DtGetProperty(&(fcp->plist), CLASS_NAME, NULL))
		return(GetGizmoText(classname));
	else
		return(((DmFnameKeyPtr)(fcp->key))->name);
}

char *
DmObjClassName(op)
DmObjectPtr op;
{
	return(DmClassName(op->fcp));
}

/*
 * DmMapWindow() maps and raises the window, and then set focus to the icon
 * box widget.
 */
void
DmMapWindow(window)
DmWinPtr window;
{
	XWMHints *wmh;
	Display *dpy = XtDisplay(window->shell);
	Window   win = XtWindow(window->shell);

	wmh = XGetWMHints(dpy, win);
	if (wmh) {
		if (wmh->initial_state != NormalState) {
			wmh->initial_state = NormalState;
			XSetWMHints(dpy, win, wmh);
		}
		FREE((void *)wmh);
	}

	XtMapWidget(window->shell);
	XRaiseWindow(dpy, win);

	/* Stolen from Xol:Traversal.c: allow for the fact that
	 * the window manager may have unmapped the window or the
	 * window may have been destroyed.
	 */
	/* Make sure the 2nd XSync() is for XSetInputFocus... */
	XSync(dpy, False);
	prev_xerror_handler = XSetErrorHandler(CatchXError);
	XSetInputFocus(dpy, XtWindow(window->views[0].box), RevertToNone, 
		       CurrentTime);
	XSync(dpy, False);
	(void)XSetErrorHandler(prev_xerror_handler);
}	/* end of DmMapWindow */

void
DmUnmapWindow(window)
DmWinPtr window;
{
	Widget w = window->shell;
	XUnmapEvent xunmap;

	XtUnmapWidget(w);

	/* Send a synthetic unmap notify event as described in ICCCM */
	xunmap.type		= UnmapNotify;
	xunmap.serial		= (unsigned long)0;
	xunmap.send_event	= True;
	xunmap.display 		= XtDisplay(w);
	xunmap.event 		= RootWindowOfScreen(XtScreen(w));
	xunmap.window		= XtWindow(w);
	xunmap.from_configure	= False;
	XSendEvent(XtDisplay(w), RootWindowOfScreen(XtScreen(w)), False,
		   (SubstructureRedirectMask | SubstructureNotifyMask),
		   (XEvent *)&xunmap);
}

/*
 * DmSetSwinSize - Sets view width and height of scrolled window
 * in a folder window.
 */
void
DmSetSwinSize(swin)
Widget swin;
{
	Dimension	width;
	Dimension	height;

	height = (
		GRID_HEIGHT(Desktop) * FOLDER_ROWS(Desktop) +
		GRID_HEIGHT(Desktop) / 2
	);
	width = (
		GRID_WIDTH(Desktop) * FOLDER_COLS(Desktop) +
		GRID_WIDTH(Desktop) / 2
	);
	XtSetArg(Dm__arg[0], XmNwidth,	width);
	XtSetArg(Dm__arg[1], XmNheight,	height);
	XtSetValues(swin, Dm__arg, 2);
} /* end of DmSetWinDimensions */

/****************************procedure*header*****************************
	DmGetFileClass - Returns file class ptr given a full path name
	along with a copy of its object properties which should be freed
	by the caller.
 */
DmFnameKeyPtr
DmGetFileClass(char *path, DtPropListPtr pp)
{
	DmContainerPtr cp;
	char *name;
	char *s;
	char *d;

	if (ROOT_DIR(s))
		return(NULL);

	name = basename(path);
	s = strdup(path);
	d = dirname(s);

	if (!(cp = DmQueryContainer(d))) {
	    if (!(cp = DmOpenDir(d, DM_B_READ_DTINFO))) {
		free(s);
		return(NULL);
	    } else {
		DmFnameKeyPtr fnkp = DmLookUpObjClass(cp, name, pp);
		DmCloseContainer(cp, DM_B_NO_FLUSH);
		free(s);
		return(fnkp);
	    }
	}
	free(s);
	return(DmLookUpObjClass(cp, name, pp));

} /* end of DmGetFileClass */

/****************************procedure*header*****************************
	DmLookUpObjClass - Given a basename of a file and ptr to its container,
	returns a ptr to its file class along with a copy of its object
	properties which should be freed by the caller.
 */
DmFnameKeyPtr
DmLookUpObjClass(DmContainerPtr cp, char *obj_name, DtPropListPtr pp)
{
	DmObjectPtr op;

	for (op = cp->op; op; op = op->next) {
		if (!strcmp(op->name, obj_name)) {
			if (!(cp->attrs & DM_B_INITED))
				DmSetFileClass(op);
			/* CAUTION: A copy must be made here because the
			  property list will be freed when the container
			  is closed!
			*/
			if (pp && op->plist.ptr)
				(void)DtCopyPropertyList(pp, &(op->plist));
			return((DmFnameKeyPtr)(op->fcp->key));
		}
	}
	return(NULL);

} /* end of DmLookUpObjClass */

/****************************procedure*header*****************************
	DmGetFolderIconFile - Returns a copy of the value of the
	MINIMIZED_ICONFILE instance property or file class property.
	The caller should free a non-NULL return value.
 */
char *
DmGetFolderIconFile(char *path)
{
	char *p = NULL;
	DmFnameKeyPtr fnkp;
	DtPropList plist;

	if (ROOT_DIR(path))
		return(NULL);

	plist.ptr = NULL;
	plist.count = 0;
	fnkp = DmGetFileClass(path, &plist);

	if (plist.ptr) {
		/* check if instance property is set */
		if ((p = DtGetProperty(&plist, "_MINIMIZED_ICONFILE", NULL)))
		{
			/* make a copy of property value before freeing the
			 * property list
			 */
			p = strdup(p);
		}
		DtFreePropertyList(&plist);
	}
	if (p == NULL &&
		/* check if file class property is set */
		(p = DtGetProperty(&(fnkp->fcp->plist), "_MINIMIZED_ICONFILE",
			NULL)))
	{
		p = strdup(p);
	}
	return(p);

} /* end of DmGetFolderIconFile */

/*****************************************************************************
 *  	DmAddContainer - add a container to a list of containers
 * 	INPUTS:  container buffer, container pointer
 *	OUTPUTS: none
 *	GLOBALS: none
 *	DESCRIPTION: if the container is already in the list, do nothing
 *
 *****************************************************************************/
void
DmAddContainer(DmContainerBuf *cp_buf, DmContainerPtr cp)
{
    int i;

    _DPRINT3(stderr, "DmAddContainer: %s\n", cp->path);

    /*
     *	Check if the container is already in the list
     */
    for (i=0; i<cp_buf->used; i++)
	if (cp_buf->p[i] == cp) {
	    return;
	}
    /*
     * The container must be added to the list.
     * Make sure we have room.
     */
    if (BufferFilled(cp_buf)) {
	GrowBuffer((Buffer *)cp_buf, 5);	/* FLH MORE: use #define */
    }
    cp_buf->p[cp_buf->used++] = cp;
}	/* end of DmAddContainer */

/*****************************************************************************
 *  	DmRemoveContainer - remove a container from a list of containers
 * 	INPUTS:  container buffer, container pointer     
 *	OUTPUTS: none
 *	GLOBALS: none
 *	DESCRIPTION: if the container is not in the list, do nothing
 *
 *****************************************************************************/
void
DmRemoveContainer(DmContainerBuf *cp_buf, DmContainerPtr cp)
{
    int i;


    _DPRINT3(stderr, "DmRemoveContainer: %s\n", cp->path);

    /*
     *	Check if the container is already in the list.
     *  If it is, and is not the last item swap with the (old) last item.
     *  Otherwise, we just decrement the used count.
     */
    for (i=0; i<cp_buf->used; i++)
	if (cp_buf->p[i] == cp){
	    cp_buf->p[i] = cp_buf->p[cp_buf->used-1];
	    cp_buf->used--;
	    return;
	}
}	/* end of DmRemoveContainer */

/****************************procedure*header*****************************
	DmIsFolderWin
 */
DmFolderWinPtr
DmIsFolderWin(Window win)
{
	register DmFolderWinPtr fwp;

	for (fwp = DESKTOP_FOLDERS(Desktop); fwp != NULL; fwp = fwp->next)
		if (XtWindow(fwp->views[0].box) == win)
			return(fwp);

	return(NULL);

} /* end of DmIsFolderWin */

/****************************procedure*header*****************************
	DmParseList -
 */
char **
DmParseList(char *list, int *nitems)
{
	int i;
	int n = 0;
	char *p, *s;
	char **items;
	char *str[128];

	*nitems = 0;
	if (list == NULL)
		return(NULL);

	if ((p = strchr(list, ',')) == NULL) {  /* only one item */
		items = (char **)MALLOC(sizeof(char *));
		items[0] =STRDUP(list);
		*nitems = 1;
		return(items);
	} else { /* multiple items */
		p = NULL;
		s = list;
		while (*s != '\n') {
			if ((p = strchr(s, ',')) != NULL) {
				str[n++] = (char *)strndup(s, p - s);
				/* skip ',' */
				s = ++p;
			} else {
				int len = strlen(s);
				s[len] = '\0';
				str[n++] = STRDUP(s);
				break;
			}
		}
		items = (char **)MALLOC(sizeof(char *) * n);
		for (i = 0; i < n; i++) {
			items[i] = str[i];
		}
		*nitems = n;
		return(items);
	}
} /* end of DmParseList */

/****************************procedure*header*********************************
 * DmFontWidth: Get the max width (over all chars) of the font used by
 *		a FlatIconBox
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DmFontWidth(Widget iconbox)
{
    XmFontList font;
    Dimension width;

    XtVaGetValues(iconbox, XmNfontList, &font, NULL);

    /* check the cache of values for the familyFontLists */

    if (font == DESKTOP_MONOSPACED(Desktop).font)
	 return (DESKTOP_MONOSPACED(Desktop).max_width);

    if (font == DESKTOP_SANS_SERIF(Desktop).font)
	 return (DESKTOP_SANS_SERIF(Desktop).max_width);    

    if (font == DESKTOP_SERIF(Desktop).font)
	 return (DESKTOP_SERIF(Desktop).max_width);    

    /* It's not in the cache.  Calculate the value */
    return(DM_FontWidth(font));

}	/* end of DmFontWidth */
/****************************procedure*header*********************************
 * DmFontHeight: Get the max height (over all chars) of the font used by
 *		a FlatIconBox
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DmFontHeight(Widget iconbox)
{
    XmFontList font;
    Dimension width;

    XtVaGetValues(iconbox, XmNfontList, &font, NULL);

    /* check the cache of values for the familyFontLists */

    if (font == DESKTOP_MONOSPACED(Desktop).font)
	 return (DESKTOP_MONOSPACED(Desktop).max_height);

    if (font == DESKTOP_SANS_SERIF(Desktop).font)
	 return (DESKTOP_SANS_SERIF(Desktop).max_height);    

    if (font == DESKTOP_SERIF(Desktop).font)
	 return (DESKTOP_SERIF(Desktop).max_height);    

    /* It's not in the cache.  Calculate the value */
    return(DM_FontHeight(font));

}	/* end of DmFontHeight */

/****************************procedure*header*****************************
 DmGetNetWareServerClass - Returns pointer to "NetWare Server" file class.
 */
DmFnameKeyPtr
DmGetNetWareServerClass()
{
	static DmFnameKeyPtr ns_fnkp = NULL;
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

	if (ns_fnkp)
		return(ns_fnkp);

	for (; fnkp; fnkp = fnkp->next) {
		if (fnkp->attrs & (DtAttrs)(DM_B_NEW | DM_B_CLASSFILE |
			DM_B_OVERRIDDEN | DM_B_INACTIVE_CLASS |
			DM_B_MANUAL_CLASSING_ONLY))
			continue;

		if (!strcmp(fnkp->name, NETWARE_SERVER)) {
			ns_fnkp = fnkp;
			return(ns_fnkp);
		}
	}
	/* Return pointer to DIR class if NetWare Server class not found */
	return((DmFnameKeyPtr)(DESKTOP_FMKP(Desktop)));

} /* end of DmGetNetWareServerClass */

/****************************procedure*header*****************************
 DmClassNetWareServers - Sets file class of objects in a container to the
 "NetWare Server" class.  Use the same stat info. on NetWare directory for
 NetWare servers to be used in long view.
 */
void
DmClassNetWareServers(DmContainerPtr cp)
{
	int status;
	struct stat stat_buf;
	DmFileInfoPtr f_info;
	register DmObjectPtr op;
	register DmFnameKeyPtr fnkp = DmGetNetWareServerClass();

	status = stat(NETWARE_PATH, &stat_buf);
	for (op = cp->op; op; op=op->next) {
		op->fcp   = fnkp->fcp;
		op->ftype = DM_FTYPE_DIR;
		if (status == 0) {
			f_info = (DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));
			if (f_info != NULL) {
	    			f_info->mode   = stat_buf.st_mode;
	    			f_info->nlink  = stat_buf.st_nlink;
	    			f_info->uid    = stat_buf.st_uid;
	    			f_info->gid    = stat_buf.st_gid;
	    			f_info->size   = stat_buf.st_size;
	    			f_info->mtime  = stat_buf.st_mtime;
	    			f_info->fstype = convnmtonum(stat_buf.st_fstype);
			}
		}
	    	op->objectdata = (void *)f_info;
		if (IS_DOT_DOT(op->name))
			op->attrs |= DM_B_HIDDEN;
	}

} /* end of DmClassNetWareServers */

