#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/status.c	1.24"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	status.c      start/stop NFS + 
 *                            show who's got our resources mounted. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Flat.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/PopupWindo.h>
#include <Xol/TextField.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/STextGizmo.h>

#include "nfs.h"
#include "text.h"
#include "notice.h"
#include "proc.h"

extern void SetStaticTextGizmoText();
extern void UpdateView();
extern void popenAddInput();
static void PostStart();
static void PostStop();
extern void NoticeCB();
extern void StatusCB();
static void ExecuteCB();
static void SelectCB();
static void UnselectCB();
static void statusMenuCB();
static void dfmounts();
static void startnfs();
static void stopnfs();
static void busyCursor();
static void standardCursor();
static Boolean startrpc();


typedef enum _startStopExits
{
    success, failure
} startStopExits;

static StaticTextGizmo nfs_serverStatus =
{
    NULL,
    "nfs_serverStatus",
    TXT_NFScheckingServer,
    CenterGravity,
    "OlDefaultBoldFont",
};

static StaticTextGizmo nfs_clientStatus =
{
    NULL,
    "nfs_clientStatus",
    TXT_NFScheckingClient,
    CenterGravity,
    "OlDefaultBoldFont",
};

static SpaceGizmo vspace = {5, 150};

static StaticTextGizmo dfmountsDesc =
{
    NULL,
    "dfmountsDesc",
    TXT_dfmountsDesc,
    WestGravity,
};

static StaticTextGizmo dfmountsHeading =
{
    NULL,
    "dfmountsHeading",
    TXT_dfmountsHeading,
    WestGravity,
};

static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};
static Setting dfmountsSetting =
{
	"dfmountsSetting",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};


static ListGizmo dfmountsList = {
	NULL, "dfmountsList", "list Label", (Setting *)&dfmountsSetting,
	"%s  %s", True, 4,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};
	
typedef enum _statusMenuItemIndex
   { startNFS, stopNFS, updateStatus, cancelStatus, statusHelp } 
   statusMenuItemIndex;

static MenuItems  statusMenuItems[] =
   {
      {False, TXT_StartNFS, CHR_StartNFS },
      {False, TXT_StopNFS,  CHR_StopNFS },
      {True,  TXT_Update,   CHR_Update },
      {True,  TXT_Cancel,   CHR_Cancel },
      {True,  TXT_Help,     CHR_Help },
      { 0 }
   };
static MenuGizmo  statusMenu = { &StatusWindowHelp, "statusMenu", "statusMenu",
				     statusMenuItems, statusMenuCB};

static GizmoRec statusGizmos[] =
{ 
   {StaticTextGizmoClass, &nfs_serverStatus},
   {StaticTextGizmoClass, &nfs_clientStatus},
   {SpaceGizmoClass,      &vspace         },
   {StaticTextGizmoClass, &dfmountsDesc   },
   {StaticTextGizmoClass, &dfmountsHeading},
   {ListGizmoClass,       &dfmountsList   },
};

static PopupGizmo statusGizmo =
{&StatusWindowHelp, "statusPopup", TXT_StatusTitle, &statusMenu, statusGizmos,
      XtNumber(statusGizmos) };

typedef enum _runState
{
    ClientServerBootPC	= 0,
    AllStopped		= 32,
    NFSstopped		= 33,
    BootServer		= 34,
    PCserver		= 35,
    Server	 	= 36,
    Client		= 37,
    ClientServerBoot	= 38,
    ClientServerPC	= 39,
    ClientServer	= 40,
    RPCstopped		= 41, 
    NFScorrupt		= 42,
    StateUnknown	= 43
} runState;

question NFSclient = q_Idunno;
question NFSserver = q_Idunno;
Widget   popupShell = NULL;

#define PING_RPC_CMD          "/usr/sbin/nfsping -o rpcbind > /dev/null 2>&1"
#define NFS_STATUS_CMD        "/usr/sbin/nfsping -a > /dev/null 2>&1"
#define NFS_CLIENT_STATUS_CMD "/usr/sbin/nfsping -c > /dev/null 2>&1"
#define NFS_SERVER_STATUS_CMD "/usr/sbin/nfsping -s > /dev/null 2>&1"
#define NFS_START_CMD  "/bin/sh /etc/init.d/nfs start"
#define NFS_STOP_CMD   "/bin/sh /etc/init.d/nfs stop"
#define NFS_STOPSTART_CMD  "/bin/sh /etc/init.d/nfs stop;/bin/sh /etc/init.d/nfs start"
#define RPC_START_CMD  "/bin/sh /etc/init.d/rpc rpcstart"

static runState
isNFSRunning()
{
    extern NFSWindow *nfsw;
    runState nfs_state;
    int retval;

    retval = system(NFS_STATUS_CMD);
    if (! WIFEXITED(retval))
    {
	SetMessage(nfsw-> statusPopup, TXT_NFSstatusFailed, Notice);
	SetStaticTextGizmoText(&nfs_serverStatus, GetGizmoText(TXT_NFSunknown));
	SetStaticTextGizmoText(&nfs_clientStatus, " ");
	NFSserver = NFSclient = q_Idunno;
	
	/* FIX: what else do we want to do here? */
	return StateUnknown;
    }

    nfs_state = (runState)WEXITSTATUS(retval);

    if (nfs_state == NFSstopped || nfs_state == AllStopped)
	OlVaFlatSetValues(statusMenu.child, stopNFS,
			  XtNsensitive, False, NULL);
    else
	OlVaFlatSetValues(statusMenu.child, stopNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
    switch (nfs_state)
    {
    case ClientServerBootPC:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientOK));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = NFSclient = q_Yes;
	break;
    case AllStopped:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverDown));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
	NFSserver = NFSclient = q_No;
	break;
    case NFSstopped:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverDown));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
	NFSserver = NFSclient = q_No;
	break;
    case BootServer:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_No;
	break;
    case PCserver:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_No;
	break;
    case Server:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_No;
	break;
    case Client:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverDown));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientOK));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_No;
	NFSclient = q_Yes;
	break;
    case ClientServerBoot:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientOK));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_Yes;
	break;
    case ClientServerPC:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientOK));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_Yes;
	break;
    case ClientServer:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverOK));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientOK));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, False, NULL);
	NFSserver = q_Yes;
	NFSclient = q_Yes;
	break;
    case RPCstopped:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverDown));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
	NFSserver = NFSclient = q_No;
	break;
    case NFScorrupt:
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFSserverDown));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFSclientDown));
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
	NFSserver = NFSclient = q_No;
	break;
    default:
	SetStaticTextGizmoText(&nfs_serverStatus, GetGizmoText(TXT_NFSunknown));
	SetStaticTextGizmoText(&nfs_clientStatus, "");
	OlVaFlatSetValues(statusMenu.child, startNFS,
			  XtNsensitive, OwnerRemote|OwnerLocal, NULL);
	NFSserver = NFSclient = q_Idunno;
	break;
    }
    return nfs_state;
}

/* sizeList adds and deletes a dummy item to the dfmounts list so that */
/* it appears as large as the header.  gag.  */

static void
sizeList()
{
    ListHead    *headPtr;
    char       **tmp;

    headPtr = (ListHead *) (dfmountsList.settings->current_value);
    headPtr->numFields = 2;
    headPtr->size = 1;
    headPtr->list = (ListItem *)MALLOC(sizeof(ListItem));
    headPtr->list[0].fields = (XtArgVal)MALLOC
            (sizeof (XtArgVal *) * headPtr->numFields);
    tmp = (char **)headPtr->list[0].fields;
    tmp[0] = STRDUP(GetGizmoText(TXT_dfmountsHeading));
    tmp[1] = STRDUP(" ");
    UpdateList(&dfmountsList, headPtr);
    ManipulateGizmo(ListGizmoClass, &dfmountsList, ResetGizmoValue); 
}


static Boolean
doStatus(XtPointer client_data)
{
    int numEvents;
    static int callCount = 0;

    /* make sure the popup has finished drawing */

    if (callCount++ == 0)
	return False;

    busyCursor();
    isNFSRunning();
    dfmounts();
    standardCursor();

    return True;

}

extern void
createStatus(Widget wid)
{
    extern NFSWindow   *nfsw;

    nfsw->statusPopup = &statusGizmo;
    popupShell = CreateGizmo(wid, PopupGizmoClass,
			     nfsw->statusPopup, NULL, 0); 
}

extern void
StatusCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow   *nfsw;
    extern XtAppContext AppContext;
    static Boolean	firstTime = True;
    Arg                 arg[10];

    if (firstTime == True)
    {
	firstTime = False;
        XtSetArg(arg[0], XtNnoneSet, True);
	XtSetValues(dfmountsList.flatList, arg, 1);
	sizeList();
    }
    XtAppAddWorkProc(AppContext, doStatus, NULL); 
    MapGizmo(PopupGizmoClass, nfsw->statusPopup);
    return;
}

static void 
SelectCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}


static void 
UnselectCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}


static void 
ExecuteCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}


static void 
StopNoticeCB(Widget w, XtPointer client_data,  OlFlatDropCallData *call_data) 
{
    extern NFSWindow *nfsw;
    Cardinal          item_index = call_data->item_data.item_index;
    ModalGizmo       *popup      = nfsw-> noticePopup;
    Widget	      shell      = GetModalGizmoShell(popup);

    DEBUG1("StopNoticeCB entry, index=%d\n", item_index);
    switch ((noticeIndex)item_index)
    {
    case NoticeDoIt:
	SetMessage(nfsw-> statusPopup, "", Popup);
	OlVaFlatSetValues(NoticeMenu.child, NoticeDoIt, XtNbusy, True, NULL);
	stopnfs();
	break;
    case NoticeCancel:
	SetMessage(nfsw-> statusPopup, TXT_NfsNotStopped, Popup);
	XtPopdown(shell);
	/* FIX: do we want to update the status popup here??  */
	standardCursor();
	break;
    case NoticeHelp:
        PostGizmoHelp(nfsw-> baseWindow-> shell, &StopNFSNoticeHelp);
	break;
    default:
	SetMessage(nfsw, "", Base);
	DEBUG0("default in StopNoticeCB taken!!!\n");
    }
}				/* end of StopNoticeCB */


static void 
statusMenuCB(Widget w, XtPointer client_data,  OlFlatDropCallData * call_data)
{
    Cardinal  item_index = call_data->item_data.item_index;
    NFSWindow *     nfsw        = FindNFSWindow(w);
    PopupGizmo *     popup      = nfsw-> statusPopup;
    Widget	    shell	= GetPopupGizmoShell(popup);
    static noticeData ndata = { TXT_ConfirmStop, TXT_StopNFS, CHR_StopNFS,
				    (XtPointer)StopNoticeCB, NULL };

    switch (item_index)
    {
    case startNFS:
	SetMessage(nfsw-> statusPopup, "", Popup);
	DEBUG0("case startNFS\n");
	busyCursor();
	OlVaFlatSetValues(statusMenu.child, startNFS, XtNbusy, True, NULL);
	startnfs();
	break;
    case stopNFS:
	SetMessage(nfsw-> statusPopup, "", Popup);
	busyCursor();
	DEBUG0("case stopNFS\n");
	if (NoticeMenu.child)
	    OlVaFlatSetValues(NoticeMenu.child, NoticeDoIt, XtNbusy,
			      True, NULL); 
	NoticeCB(w, &ndata, call_data);
	break;
    case updateStatus:
	SetMessage(nfsw-> statusPopup, "", Popup);
	busyCursor();
	SetStaticTextGizmoText(&nfs_serverStatus,
			       GetGizmoText(TXT_NFScheckingServer));
	SetStaticTextGizmoText(&nfs_clientStatus,
			       GetGizmoText(TXT_NFScheckingClient));
	isNFSRunning();
	dfmounts();
	standardCursor();
	break;
    case cancelStatus:
	SetMessage(nfsw-> statusPopup, "", Popup);
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
	XtPopdown(shell);
	break;
    case statusHelp:
	PostGizmoHelp(nfsw-> baseWindow-> shell, &StatusWindowHelp);
	break;
    default:
	SetMessage(nfsw-> statusPopup, "", Popup);
	DEBUG0("default in statusMenuCB taken!!!\n");
    }
}				/* end of statusMenuCB */


#define DFMOUNTSCMD "/usr/sbin/dfmounts -F nfs -h"

static void
dfmounts()
{
    extern NFSWindow *nfsw;
    extern	int p3open(), p3close();
    FILE 	*fp[2];
    char  	myhost[BUFSIZ], path[PATH_MAX], connectedHosts[BUFSIZ];
    char *	errorText;
    int   	i, status, num;
    char 	**tmp;
    ListHead	*headPtr;

    headPtr = (ListHead *) (dfmountsList.settings->previous_value);
    if (headPtr != NULL)
	FreeList(headPtr);
    headPtr->numFields = 2;
    headPtr->size = 0;
    headPtr->list = NULL;

    /* fork and exec dfmounts(1) to get list of hosts connected to our */
    /* resources  */

    if (p3open (DFMOUNTSCMD, fp) != 0) {
	p3close (fp, BAD_PID);
	errorText = GetGizmoText(TXT_PopenFailed);
	fprintf (stderr, errorText, DFMOUNTSCMD);
	ManipulateGizmo(ListGizmoClass, &dfmountsList, ResetGizmoValue); 
	return;
    }

    /* Parse buf for lines and put each line into a list item */
    for ( i=0; (num = fscanf(fp[1], " %*s %s %s %s", myhost, path,
			     connectedHosts)) != EOF;
	 i++)
    {
	headPtr->size++;
	headPtr->list = (ListItem *)REALLOC((char *)headPtr->list,
					      sizeof(ListItem)*headPtr->size);
	headPtr->list[i].set = False;
        headPtr->list[i].fields = (XtArgVal)MALLOC
	    (sizeof (XtArgVal *) * headPtr->numFields);
	headPtr->list[i].clientData = NULL;
	tmp = (char **)headPtr->list[i].fields;
	tmp[0] = STRDUP(path);
	tmp[1] = STRDUP(connectedHosts);
	DEBUG2("path=\"%s\", hosts = \"%s\"\n", path, connectedHosts);
    }
    status = p3close(fp, BAD_PID);
    
    DEBUG1("dfmounts status = %x\n", status);
    if ((WIFEXITED(status)) && ((WEXITSTATUS(status)) == 0)) /* success */
	ManipulateGizmo(ListGizmoClass, &dfmountsList, ResetGizmoValue);
    else
    {
	/* FIX: choose message based on exit code */
	if (NFSserver == q_Yes)
	    SetMessage(nfsw-> statusPopup, TXT_dfmountsFailed, Notice);
	ManipulateGizmo(ListGizmoClass, &dfmountsList, ReinitializeGizmoValue);
	DEBUG2("dfmounts status = %d (%xX)\n", status, status);

    }	
    return;
}


static void 
startnfs()
{
    extern NFSWindow *nfsw;
    runState state;
    inputProcData *data;
    int exit_code;
    uid_t uid;

    busyCursor();
    if ((data = (inputProcData *)MALLOC(sizeof(inputProcData))) == NULL)
	NO_MEMORY_EXIT();

    SetStaticTextGizmoText(&nfs_serverStatus,
			   GetGizmoText(TXT_NFScheckingServer));
    SetStaticTextGizmoText(&nfs_clientStatus,
			   GetGizmoText(TXT_NFScheckingClient));

/*
    if (NFSclient == q_Yes || NFSserver  == q_Yes)
        stopnfs(False);
*/
    /* start nfs as root to avoid problems like rpc services */
    /* registered to the last user who started nfs preventing the */
    /* current user from starting nfs */
    
    uid = getuid();
    while (setuid((uid_t)0) < 0 && errno == EINTR)
	; /* try again */
    startrpc();
/**    popenAddInput(NFS_START_CMD, PostStart, PostStart,
**/
    popenAddInput(NFS_STOPSTART_CMD, PostStart, PostStart,
		  (XtPointer)data);
    while (setuid(uid) < 0 && errno == EINTR)
	; /* try again */
    return;
}

static void
PostStart(XtPointer client_data, int *source, XtInputId *id)
{
    extern NFSWindow *nfsw;
    inputProcData  *data = (inputProcData *)client_data;
    startStopExits exitCode;
    int status, count;
    runState nfs_state;
    char buf[BUFSIZ];

    DEBUG0("PostStart Entry\n");
    
    if (*id == data-> readId)
    {
	count = read(*source, buf, BUFSIZ);
	DEBUG2(">> count=%d, errno=%d\n", count,errno);
	buf[count] = EOS;
	DEBUG1(">> buf = %s\n", buf);

	if (count > 0)
	    return;
    }
    DEBUG0("removing inputs\n");
    XtRemoveInput(data-> readId);
    XtRemoveInput(data-> exceptId);

    status = p3close(data-> fp, data-> pid);

    DEBUG1("PostStart: status = %x\n", status);

    sleep(2);
    nfs_state = isNFSRunning();
    if (! WIFEXITED(status))
    {
	SetMessage(nfsw-> statusPopup, TXT_NFSstartFailed, Notice);
	DEBUG1("nfs start command fork failed, status=%x\n", status);
	standardCursor();
	return; 
    }

    OlVaFlatSetValues(statusMenu.child, startNFS, XtNbusy, False, NULL);

    exitCode = (startStopExits)WEXITSTATUS(status);
    
    switch (nfs_state)
    {
    case ClientServerBootPC:
	/* Fall Thru */
    case ClientServerBoot:
	/* Fall Thru */
    case ClientServerPC:
	/* Fall Thru */
    case ClientServer:
	SetMessage(nfsw-> statusPopup, TXT_NFSstartSucceeded, Popup);
	dfmounts();
	UpdateView(ViewCurrent);
	standardCursor();
	return;
    case BootServer:
	/* Fall Thru */
    case PCserver:
	/* Fall Thru */
    case Server:
	dfmounts();
	/* Fall Thru */
    case Client:
	/* Fall Thru */
    case NFScorrupt:
	/* Fall Thru */
    case RPCstopped:
	/* Fall Thru */
    case NFSstopped:
	/* Fall Thru */
    case StateUnknown:
	/* Fall Thru */
    default:
	SetMessage(nfsw-> statusPopup, TXT_NFSstartFailed, Notice);
	DEBUG1("nfs start failed, exit code = %d\n", exitCode);
	UpdateView(ViewCurrent);
	standardCursor();
	return;
    }
    
}


static void
stopnfs()
{
    extern NFSWindow *nfsw;
    inputProcData *data;

    if ((data = (inputProcData *)MALLOC(sizeof(inputProcData))) == NULL)
	NO_MEMORY_EXIT();

    SetStaticTextGizmoText(&nfs_serverStatus,
			   GetGizmoText(TXT_NFScheckingServer));
    SetStaticTextGizmoText(&nfs_clientStatus,
			   GetGizmoText(TXT_NFScheckingClient));
    XtPopdown(GetModalGizmoShell(nfsw-> noticePopup));
    
    popenAddInput(NFS_STOP_CMD, PostStop, PostStop, (XtPointer)data);
    return;
}


static void
PostStop(XtPointer client_data, int *source, XtInputId *id)
{
    extern NFSWindow *nfsw;
    inputProcData  *data = (inputProcData *)client_data;
    startStopExits exitCode;
    runState nfs_state;
    int status, count;
    char buf[BUFSIZ];

    DEBUG0("PostStop Entry\n");
    
    if (*id == data-> readId)
    {
	count = read(*source, buf, BUFSIZ);
	DEBUG2(">> count=%d, errno=%d\n", count,errno);
	buf[count] = EOS;
	DEBUG1(">> buf = %s\n", buf);

	if (count > 0)
	    return;
    }
    DEBUG0("removing inputs\n");
    XtRemoveInput(data-> readId);
    XtRemoveInput(data-> exceptId);

    status = p3close(data-> fp, data-> pid);

    DEBUG1("PostStop: status = %x\n", status);

    if (! WIFEXITED(status))
    {
	SetMessage(nfsw-> statusPopup, TXT_NFSstopFailed, Notice);
	DEBUG1("nfs stop command fork failed, status=%x\n", status);
	isNFSRunning();
	XtPopdown(GetModalGizmoShell(nfsw-> noticePopup));
	standardCursor();
	return;
    }

    nfs_state = isNFSRunning();

    OlVaFlatSetValues(NoticeMenu.child, NoticeDoIt, XtNbusy, False, NULL);
    OlVaFlatSetValues(NoticeMenu.child, NoticeCancel, XtNsensitive, True, NULL);

    exitCode = (startStopExits)WEXITSTATUS(status);
    
    switch (nfs_state)
    {
    case AllStopped:
	/* Fall Thru */
    case NFSstopped:
	SetMessage(nfsw-> statusPopup, TXT_NFSstopSucceeded, Popup);
	ManipulateGizmo(ListGizmoClass, &dfmountsList, ReinitializeGizmoValue);
	XtPopdown(GetModalGizmoShell(nfsw-> noticePopup));
	UpdateView(ViewCurrent);
	standardCursor();
	return;
    default:
	DEBUG1("nfs stop failed, exit code = %d\n", exitCode);
	SetMessage(nfsw-> statusPopup, TXT_NFSstopFailed, Notice);
	XtPopdown(GetModalGizmoShell(nfsw-> noticePopup));
	standardCursor();
	return;
    }
}

static Boolean
startrpc()
{
    extern NFSWindow *nfsw;
    int status;
    startStopExits exitCode;

    status = system(PING_RPC_CMD);
    if (! WIFEXITED(status))
    {
	DEBUG0("rpc ping command fork failed\n");
	if (NFSserver == q_Yes || NFSclient == q_Yes)
	    return SUCCESS;	 /* assume rpc is up */
    }
    else
    {
	if ((exitCode = (startStopExits)WEXITSTATUS(status)) == success)
	    return SUCCESS;
    }

    status = system(RPC_START_CMD);
    if (! WIFEXITED(status))
    {
	SetMessage(nfsw-> statusPopup, TXT_RPCstartFailed, Notice);
	DEBUG0("rpc start command fork failed\n");
	isNFSRunning();
	return FAILURE;
    }

    exitCode = (startStopExits)WEXITSTATUS(status);
    
    switch (exitCode)
    {
    case success:
	SetMessage(nfsw-> statusPopup, TXT_RPCstartSucceeded, Popup);
	return SUCCESS;
    default:
	DEBUG1("rpc start failed, exit code = %d\n", exitCode);
	SetMessage(nfsw-> statusPopup, TXT_RPCstartFailed, Notice);
	return FAILURE;
    }

}


extern void
GetStatus(ViewIndex view)
{
    extern NFSWindow *nfsw;
    int    exit_code;
    question previousServerValue = NFSserver;

    if ((view == ViewLocal || view == ViewCurrent) && 
	nfsw-> viewMode == ViewLocal) 
    {
	if ((exit_code = system(NFS_SERVER_STATUS_CMD)) == 0)	
	{
	    SetStaticTextGizmoText(&nfs_serverStatus,
				   GetGizmoText(TXT_NFSserverOK));
	    NFSserver = q_Yes;
	}
	else
	{
	    NFSserver = q_No;
	    SetStaticTextGizmoText(&nfs_serverStatus,
				   GetGizmoText(TXT_NFSserverDown));
	}
	
	if (previousServerValue != NFSserver)
	{
	    if ((exit_code = system(NFS_CLIENT_STATUS_CMD)) == 0)	
	    {
		SetStaticTextGizmoText(&nfs_clientStatus,
				       GetGizmoText(TXT_NFSclientOK));
		NFSclient = q_Yes;
	    }
	    else
	    {
		NFSclient = q_No;
		SetStaticTextGizmoText(&nfs_clientStatus,
				       GetGizmoText(TXT_NFSclientDown));
	    }
	}
    }
}

static void 
busyCursor()
{
    Widget w = statusGizmo.shell;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlBusyCursor(XtScreen(w)));
    XSync(XtDisplay(w), False);
    return;
}	


static void 
standardCursor()
{
    Widget w = statusGizmo.shell;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlStandardCursor(XtScreen(w)));
    return;
}

