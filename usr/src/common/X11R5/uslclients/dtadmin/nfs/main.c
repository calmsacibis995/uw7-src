#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/main.c	1.29"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	main.c  
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/vfstab.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <X11/Shell.h>
#include <Xol/Form.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/InputGizmo.h>

#include "dtadmin/dtamlib/owner.h"

#include "text.h"
#include "nfs.h"
#include "sharetab.h"
#include "local.h"
#include "notice.h"
#include "utilities.h"
#include "helpinfo.h"

extern Boolean	_DtamIsOwner();
extern void	GetRemoteContainerItems();
extern void	Get_dfstabEntries();
extern void  	Get_sharetabEntries();
extern MenuGizmo MenuBar;
extern NFSWindow *FindNFSWindow();
extern void LocalPropertyCB();
extern void RemotePropertyCB();
extern void LocalAddCB();
extern void RemoteAddCB();
extern void StatusCB();
extern void NoticeCB();
extern void unselectAll();
extern void ReselectAll();
extern void InitializeIcon();
extern void 		UpdateView();
extern char   	       *debug_nfs = NULL;
extern dfstabEntryType	Get_dfstab_entry();
extern void		TimerUpdate();
extern void		TimerOnOff();
extern void		createStatus();

typedef enum _resetMode { save, restore } resetMode;

#define ApplicationClass	"File_Sharing"
#define ClientName		"File_Sharing"
#define NFS_PROCESS_ICON	"nfs48.icon"

XtAppContext		AppContext;
XtIntervalId		TimerId = (XtIntervalId)NULL;
Boolean			OwnerRemote, OwnerLocal;
Widget       		root;
unsigned long		UpdateRate;

/* forward declarations */

static NFSWindow       *CreateNFSWindow();
extern void Get_vfstabEntries();
extern void Get_dfstabEntries();

static void check_dtvfstab();
static void resetSensitivities();

extern NFSWindow *MainWindow = NULL;
extern NFSWindow *nfsw = NULL;
static Boolean    Warnings = False;
extern char *IconFilenames[] =
{
    "nfsLocal",
    "nfsShared",
    "nfsRemote",
    "nfsMounted"
};

ApplicationResources FSresources; 
static XtResource resources[]=
{
    { XtNgeometry, XtCGeometry, XtRString, sizeof(String), 
	  (Cardinal)&FSresources.geometry, XtRString,
	  (XtPointer)"400x200" },
    { "updateRate", XtCInterval, XtRCardinal, sizeof(Cardinal),
	  (Cardinal)&FSresources.updateRate, XtRString,
	  (XtPointer)"30" }, 
};

main(argc, argv)
int argc;
char * argv[];
{
    extern Boolean CreateNotice();
    Boolean	isTrusted;

    debug_nfs=getenv("DEBUG_NFS");

    sigset(SIGCHLD, SIG_DFL);

    root = InitializeGizmoClient(ClientName, ApplicationClass,
				 ClientName, NULL, NULL, NULL,
				 0, &argc, argv, NULL, NULL,
				 resources, XtNumber(resources), NULL,
				 0, NULL, NULL, NULL); 

    AppContext = XtWidgetToApplicationContext (root);

    /* buttons such as advertise which require privilege will be       */
    /* greyed out unless the -t option is passed to the program.  This */
    /* option is passed by the desktop when the program is invoked via */
    /* tfadmin. */

    isTrusted = (argc == 2 && strcmp(argv[1], "-t") == 0);
    if (isTrusted)
    {
	DEBUG0("Trusted\n");
	OwnerRemote = _DtamIsOwner(OWN_REMOTE, NULL);
	OwnerLocal  = _DtamIsOwner(OWN_LOCAL, NULL);
    }
    DEBUG2("OwnerRemote=%d, OwnerLocal=%d\n", OwnerRemote, OwnerLocal);
    nfsw = CreateNFSWindow(root);

    UpdateRate = (unsigned long)1000 * (unsigned long)FSresources.updateRate;
    TimerId = XtAppAddTimeOut(AppContext, UpdateRate, TimerUpdate, NULL);
    XtInsertEventHandler(GetBaseWindowShell(nfsw-> baseWindow),
			 PropertyChangeMask, FALSE, TimerOnOff, 
			 NULL, XtListTail);
    XtAppAddWorkProc(AppContext, CreateNotice, NULL); 
    XtAppMainLoop(AppContext);

}



static BaseWindowGizmo baseWin = { &ApplicationHelp, "nfs", TXT_ApplicationTitle, 
				       &MenuBar, NULL,
				       0, TXT_IconLabel,
				       NFS_PROCESS_ICON,
				       " ", " ", 90 };


static NFSWindow * 
CreateNFSWindow(root)
Widget	root;
{
    extern void InitContainer();
    extern void SetValue();
    extern MenuGizmo EditMenu;
    Widget	shell;
    Window	another_window;
    Display    *display;
    Arg    	arg[8];
    struct utsname name;
    Cardinal    nitems;
    char        atom[SYS_NMLN+MINIBUFSIZE]= ApplicationClass;

    NFSWindow * new = (NFSWindow *) MALLOC(sizeof(NFSWindow));
  
    if (MainWindow)
	FREE((char *)MainWindow);
    MainWindow = nfsw = new;

    new-> viewMode		= ViewRemote;
    new-> selectList		= &new-> remoteSelectList;
    new-> baseWindow		= &baseWin;
    new-> iconbox		= NULL;
    new-> statusPopup		= NULL;
    new-> noticePopup   	= NULL;
    new-> hostPopup		= NULL;
    new-> remotePropertiesPrompt= NULL;
    new-> localPropertiesPrompt = NULL;
    new-> itp			= NULL; 
    new-> cp			= NULL;
    new-> remote_cp		= NULL;
    new-> local_cp		= NULL;
    new-> remoteSelectList	= NULL;
    new-> localSelectList	= NULL;

    /* utill BaseWindowGizmo is fixed... */
    baseWin.title = STRDUP(GetGizmoText(TXT_BaseRemoteTitle));

    shell = CreateGizmo(root, BaseWindowGizmoClass, new-> baseWindow, NULL, 0);

    /* set default geometry wide enough for messages */
    XtSetArg(arg[0], XtNgeometry, (XtArgVal)FSresources.geometry);
    XtSetValues(shell, arg, 1);

    XtVaSetValues (shell, XtNmappedWhenManaged, (XtArgVal) False, 0);
    XtRealizeWidget(shell);
    display = XtDisplay(shell);
    if (uname(&name) >0)
    {
	strcat(atom, ":");
	strcat(atom, name.nodename);
    }
    DEBUG1("atom=%s\n",atom);
    another_window = DtSetAppId(display, XtWindow(shell), atom);
    if (another_window != None)
    {    
	XMapWindow(display, another_window);
	XRaiseWindow(display, another_window);
	XFlush(display);
	exit(0);
    }
    XtVaSetValues (shell, XtNmappedWhenManaged, (XtArgVal) True, 0);
    check_dtvfstab();

    GetRemoteContainerItems();
    InitContainer(baseWin.scroller, baseWin.form);

    MapGizmo(BaseWindowGizmoClass, new-> baseWindow);
    OlVaFlatSetValues(EditMenu.child, EditAdd, XtNsensitive, OwnerRemote, NULL);
    SetValue(baseWin.shell, XtNtitle, GetGizmoText(TXT_BaseRemoteTitle)); 
    nitems =  GetValue(nfsw-> iconbox, XtNnumItems);
    if (nitems == 0)
	SetMessage(MainWindow, TXT_FirstTime, Base);

    createStatus(shell);

    return new;

} /* end of CreateNFSWindow */

static void 
ResetIconbox()
{
    extern void InitContainer();

    XtUnmanageChild(nfsw-> iconbox);
    XtDestroyWidget(nfsw-> iconbox);
    InitContainer(baseWin.scroller, baseWin.form);
    ReselectAll();
}


extern void
nfsActionsCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * ptr = (OlFlatCallData *)call_data;

   switch(ptr-> item_index)
   {
   case ActionsConnect:
       SetMessage(MainWindow, "", Base);
       MountCB(w, client_data, call_data);
       break;
   case ActionsUnconnect:
       SetMessage(MainWindow, "", Base);
       UnMountCB(w, client_data, call_data);
       break;
   case ActionsAdvertise:
       SetMessage(MainWindow, "", Base);
       AdvertiseCB(w, client_data, call_data);
       break;
   case ActionsUnadvertise:
       SetMessage(MainWindow, "", Base);
       UnAdvertiseCB(w, client_data, call_data);
       break;
   case ActionsStatus:
       SetMessage(MainWindow, "", Base);
       StatusCB(w, client_data, call_data);
       break;
   case ActionsExit:
       /* FIX: do we need to do any more? */
       exit (0);
       break;
   default:
       SetMessage(MainWindow, "", Base);
       DEBUG0("default in nfsActionsCB taken!!!\n");
   }
}


extern void
nfsEditCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData * ptr = (OlFlatCallData *)call_data;
    static noticeData       ndata = {"", TXT_Delete, CHR_Delete,
					 (XtPointer)DeleteCB, NULL};

    if (nfsw-> viewMode == ViewRemote)
    {
	switch(ptr-> item_index)
	{
	case EditAdd:
	    SetMessage(MainWindow, "", Base);
	    RemoteAddCB(w, client_data, call_data);
	    break;
	case EditDelete:
	    SetMessage(MainWindow, "", Base);
	    ndata.text  = TXT_ConfirmRemoteDelete;
	    NoticeCB(w, &ndata, call_data);
	    break;
	case EditProperties:
	    SetMessage(MainWindow, "", Base);
	    RemotePropertyCB(w, True, call_data);
	    break;
	default:
	    SetMessage(MainWindow, "", Base);
	    DEBUG1("default in remote nfsEditCB taken!!! index=%d\n", ptr-> item_index );
	}
    }
    else			/* local view is current */
    {
	DEBUG0("nfsEditLocalCB entry\n");

	switch(ptr-> item_index)
	{
	case EditAdd:
	    SetMessage(MainWindow, "", Base);
	    LocalAddCB(w, client_data, call_data);
	    break;
	case EditDelete:
	    SetMessage(MainWindow, "", Base);
	    ndata.text  = TXT_ConfirmLocalDelete;
	    NoticeCB(w, &ndata, call_data);
	    break;
	case EditProperties:
	    SetMessage(MainWindow, "", Base);
	    LocalPropertyCB(w, True, call_data);
	    break;
	default:
	    SetMessage(MainWindow, "", Base);
	    DEBUG0("default in local nfsEditCB taken!!!\n");
	}
    }
}

extern void
nfsViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    extern void InitContainer();
    extern MenuGizmo ActionsMenu, ViewMenu;
    extern MenuGizmo AdvMenu, RemoteMenu;
    static Boolean firstTime = True;

    OlFlatCallData * ptr = (OlFlatCallData *)call_data;

    switch(ptr-> item_index)
    {
    case ViewLocal:
	if (nfsw-> viewMode == ViewLocal)
	{
	    /* shouldn't be able to get here, but play it safe */
	    return;
	}
	else
	    SetMessage(MainWindow, "", Base);

	GetStatus(ViewLocal);

	resetSensitivities(save);
	nfsw-> viewMode = ViewLocal;
	resetSensitivities(restore);

	/* switch to local iconbox */

	SetValue(baseWin.shell, XtNtitle, GetGizmoText(TXT_BaseLocalTitle));
	nfsw-> selectList = &nfsw-> localSelectList;
	nfsw-> remote_cp = nfsw-> cp;
	nfsw-> cp = nfsw-> local_cp;
	if (firstTime == True)
	{
	    /* first time local view displayed */
	    firstTime = False;
	    Get_dfstabEntries(); 
	    ResetIconbox();
	    Get_sharetabEntries();
	}
	else
	{
	    ResetIconbox();
	    UpdateView(ViewLocal);
	}
	break;

    case ViewRemote:
	if (nfsw-> viewMode == ViewRemote)
	{
	    /* shouldn't be able to get here, but play it safe */
	    return;
	}
	else
	{
	    SetMessage(MainWindow, "", Base);

	    resetSensitivities(save);
	    nfsw-> viewMode = ViewRemote;
	    resetSensitivities(restore);
	   
	    /* switch to remote iconbox */

	    SetValue(baseWin.shell, XtNtitle,
		     GetGizmoText(TXT_BaseRemoteTitle)); 
	    nfsw-> selectList = &nfsw-> remoteSelectList;
	    nfsw-> local_cp = nfsw-> cp;
	    nfsw-> cp = nfsw-> remote_cp;
	    ResetIconbox();
	    UpdateView(ViewRemote);
	}
	break;
    default:
	SetMessage(MainWindow, "", Base);
	DEBUG0("default in nfsViewCB taken!!!\n");
    }
}
extern void
nfsHelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * ptr   = (OlFlatCallData *)call_data;
   Widget 	    shell = nfsw-> baseWindow-> shell;

   switch(ptr-> item_index)
   {
   case HelpFileSharing:
       PostGizmoHelp(shell, &ApplicationHelp);
       break;
   case HelpTOC:
       PostGizmoHelp(shell, &TOCHelp);
       break;
   case HelpDesk:
       PostGizmoHelp(shell, &HelpDeskHelp);
       break;
   default:
       DEBUG0("default in nfsHelpCB taken!!!\n");
   }
}  /* end of nfsCB */


/*
 * FindToplevel
 *
 */


extern NFSWindow * 
FindNFSWindow(Widget w)
{
   return (MainWindow);

} /* end of FindNFSWindow */


static void
check_dtvfstab()
{
    int fd;

    /* create DTVFSTAB if non-existant */

    if (access(DTVFSTAB, F_OK) == 0 || errno != ENOENT)
	return;
    if ((fd = creat(DTVFSTAB, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1)
	return;
    fchown(fd, (uid_t)0, (gid_t)1);
    close(fd);
}
       
static void
resetSensitivities(resetMode what)
{
    extern MenuGizmo ActionsMenu,          EditMenu,       ViewMenu;
    extern MenuItems ActionsMenuItems[],   EditMenuItems[];
    static XtArgVal  localDelSens = False, localPropSens = False;
    static XtArgVal  remoteDelSens,        remotePropSens;

    /* If what == save then save the sensitivities so we can reset   */
    /* them later.  Otherwise reset the sensitivities to the saved   */
    /* values.  Note that the ActionsMenu sensitivities for          */
    /* [un]connect and [un]advertise are reset in updateRemoteView   */
    /* and updateLocalView.                                          */

    switch (nfsw-> viewMode)
    {
    case ViewLocal:
    
	if (what == save)
	{
	    localDelSens    = EditMenuItems[EditDelete].sensitive;
	    localPropSens   = EditMenuItems[EditProperties].sensitive;
	    return;
	}
	else
	{
	    /* change sensitivity of view menu items */

	    OlVaFlatSetValues(ViewMenu.child, 
			      ViewLocal,  XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ViewMenu.child, 
			      ViewRemote, XtNsensitive, True,  NULL); 

	    /* change sensitivity of Actions and Edit menu items */

	    OlVaFlatSetValues(ActionsMenu.child, 
			      ActionsConnect,   XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, 
			      ActionsUnconnect, XtNsensitive, False, NULL);

	    OlVaFlatSetValues(EditMenu.child, EditAdd, XtNsensitive,
			      OwnerLocal, NULL); 
	    OlVaFlatSetValues(EditMenu.child,
			      EditDelete,     XtNsensitive, localDelSens,
			      NULL);
	    OlVaFlatSetValues(EditMenu.child,
			      EditProperties, XtNsensitive, localPropSens,
			      NULL);
	}
	break;
    case ViewRemote:
	if (what == save)
	{
	    remoteDelSens  = EditMenuItems[EditDelete].sensitive;
	    remotePropSens = EditMenuItems[EditProperties].sensitive;
	    return;
	}
	else
	{

	    /* change sensitivity of view menu items */
	    OlVaFlatSetValues(ViewMenu.child,
			      ViewLocal,  XtNsensitive, True,  NULL);
	    OlVaFlatSetValues(ViewMenu.child,
			      ViewRemote, XtNsensitive, False, NULL);

	    OlVaFlatSetValues(ActionsMenu.child,
			      ActionsAdvertise,   XtNsensitive, False, 
			      NULL);
	    OlVaFlatSetValues(ActionsMenu.child,
			      ActionsUnadvertise, XtNsensitive, False,
			      NULL); 

	    OlVaFlatSetValues(EditMenu.child, EditAdd, XtNsensitive,
			      OwnerRemote, NULL); 
            OlVaFlatSetValues(EditMenu.child,
                              EditDelete,     XtNsensitive, remoteDelSens,
			      NULL);
            OlVaFlatSetValues(EditMenu.child,
                              EditProperties, XtNsensitive, remotePropSens,
                              NULL);
	}
	break;
    default:
	DEBUG1("resetSensitivities called with bad ViewMode (%d)\n",
	       nfsw-> viewMode);
	break;
    }
    return;
}
