/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/remoteProp.c	1.35.2.1"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	remoteProp.c  Remote Folder Property Sheet
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/vfstab.h>

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
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include "nfs.h"
#include "text.h"
#include "remoteProp.h"
#include "verify.h"
#include "notice.h"
#include "client.h"
#include "proc.h"
#include "help.h"

#define EMPTY     True
#define NOT_EMPTY False

Boolean is_multi_byte = False;

static Boolean  PropertyPopupMode = False;
static Widget   PopupShell = NULL; /* shell of the remotePropertiesPrompt */
static UserData user;

static Boolean
bringDownPopups(Widget parent, Boolean force)
{
    extern NFSWindow *nfsw;
    Widget shell;
    long pushpin_state = WMPushpinIsOut;

    if (force == True)
	SetWMPushpinState(XtDisplay(parent), XtWindow(parent), WMPushpinIsOut);
    else
	GetWMPushpinState(XtDisplay(parent), XtWindow(parent),
			  &pushpin_state); 

    switch (pushpin_state) 
    {
    case WMPushpinIsIn:
	return False;
	break;
    case WMPushpinIsOut:
    default:
	XtPopdown(parent);

	if ((shell = GetFileGizmoShell(&fileFolderPopup)) != NULL)
	{
	    SetWMPushpinState(XtDisplay(shell), XtWindow(shell),
			      WMPushpinIsOut); 
	    XtPopdown(shell);
	}
	/* FIX: hostPopup could belong to Local window */
	if (nfsw-> hostPopup != NULL &&
	    (shell = GetPopupGizmoShell(nfsw-> hostPopup)) != NULL)
	{
	    SetWMPushpinState(XtDisplay(shell), XtWindow(shell),
			      WMPushpinIsOut); 
	    XtPopdown(shell);
	}
	return True;
	break;
    }
}


static void 
newRemote(Boolean mount)
{
    extern void MountIt();
    extern DmObjectPtr	UpdateContainer();

    extern NFSWindow *nfsw;
    extern MenuGizmo EditMenu;

    FILE *fp, *dtfp;
    struct vfstab vfs_entry;
    char   buf[BUFSIZ], mountOptions[BUFSIZ], *bootMount;
    DmObjectPtr op;
    Cardinal index;

    if ((int)remoteSetting.bootMount.current_value == yes)
	bootMount = "yes";
    else
	bootMount = "no";

    *mountOptions = EOS;

    if ((int)remoteSetting.writable.current_value == ReadOnly)
    {
	strcat(mountOptions, READONLY);
	DEBUG0("ReadOnly\n");
    }
    else
    {
        strcat(mountOptions, READWRITE);
	DEBUG0("ReadWrite\n");
    }
    if ((int)remoteSetting.mountType.current_value == HardMount)
    {
	strcat(mountOptions, HARDMOUNT);
	DEBUG0("HardMount\n");
    }
    else
    {
	strcat(mountOptions, SOFTMOUNT);
	DEBUG0("SoftMount\n");
    }
    if ( remoteSetting.customOptions.current_value != NULL && 
	*((char *)remoteSetting.customOptions.current_value) != EOS)
    {
	strcat(mountOptions, ",");
	strcat(mountOptions, remoteSetting.customOptions.current_value);
    }
    sprintf(buf, "%s:%s", remoteSetting.host.current_value,
	    remoteSetting.remotePath.current_value );
    vfsnull(&vfs_entry);
    vfs_entry.vfs_special = buf;
    vfs_entry.vfs_fsckdev = NULL;
    vfs_entry.vfs_mountp  = remoteSetting.localPath.current_value;
    vfs_entry.vfs_fstype  = "nfs";
    vfs_entry.vfs_fsckpass = NULL;
    vfs_entry.vfs_automnt  = bootMount;
    vfs_entry.vfs_mntopts  = mountOptions;

DEBUG1("mount opts = \"%s\"\n", vfs_entry.vfs_mntopts);
    if ((fp = fopen(VFSTAB, "a+")) == NULL)
    {
        SetMessage(nfsw-> remotePropertiesPrompt, TXT_vfstabOpenError, Notice);
	return;
    }    
    putvfsent(fp, &vfs_entry);
    fclose(fp);
DEBUG1("mount opts = \"%s\"\n", vfs_entry.vfs_mntopts);

DEBUG4("bootmount=\"%s\", %x, access=\"%s\", %x\n",
       remoteSetting.bootMount.current_value,
       remoteSetting.bootMount.current_value,
       remoteSetting.writable.current_value,
       remoteSetting.writable.current_value);
	    
    vfs_entry.vfs_fsckpass = remoteSetting.folderLabel.current_value;

    if ((dtfp = fopen(DTVFSTAB, "a+")) == NULL)
    {
        SetMessage(nfsw-> remotePropertiesPrompt, TXT_dtvfstabOpenError, Notice);
	return;
    }
    putvfsent(dtfp, &vfs_entry);
    fclose(dtfp);
DEBUG1("mount opts = \"%s\"\n", vfs_entry.vfs_mntopts);

    /* add to iconbox */

    op = UpdateContainer(vfs_entry.vfs_fsckpass,
			 (XtPointer)Copy_vfstab(&vfs_entry), 
			 nfsw-> remote_fcp, &index, nfsRemote);  
    if ( op != (DmObjectPtr)NULL )
    {
	if (mount)
	    MountIt(&vfs_entry, op, index);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerRemote, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
    }
    return;
}


static void
doPropReset(void)
{
    extern NFSWindow *nfsw;

    SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
    if (PropertyPopupMode == True) /* property sheet mode*/
	ManipulateGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt, 
			ResetGizmoValue); 
    else			/* add new mode */
	ManipulateGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt, 
			ReinitializeGizmoValue); 
    MoveFocus(remoteHostG.textFieldWidget);
    return;
}


static void 
checkMountPointCB(Widget wid, XtPointer client_data,
		  OlFlatDropCallData *call_data) 
{
    extern NFSWindow *nfsw;
    Cardinal item_index = call_data->item_data.item_index;
    ModalGizmo       *popup      = nfsw-> noticePopup;
    Widget	      shell      = GetModalGizmoShell(popup);

    switch ((noticeIndex)item_index)	
    {
    case NoticeDoIt:		/* mount over non-empty folder */
	SetMessage(nfsw-> remotePropertiesPrompt, "", Popup);
	newRemote(True);
	XtPopdown(shell);
	bringDownPopups(PopupShell, False);
	doPropReset();
	break;
    case NoticeCancel:
	SetMessage(nfsw-> remotePropertiesPrompt, "", Popup);
	MoveFocus(localPathG.textFieldWidget);
	XtPopdown(shell);
	break;
    case NoticeHelp:
	PostGizmoHelp(PopupShell, &MountPointNotEmptyHelp);
	break;
    default:
	DEBUG0("default in checkMountPointCB taken!!!\n");
    }
    return;
}

#define DM_INFO_FILENAME ".dtinfo" /* also defined in <dtm/Dtm.h> */

static Boolean
checkMountPoint(void)
{
    DIR           *dirp;
    struct dirent *direntp;
    Boolean	   empty = True;
    char	  *buf;
    noticeData	   ndata = { NULL, TXT_Add2, CHR_Add2,
				 (XtPointer)checkMountPointCB, NULL }; 

    if ((dirp = opendir((char *)remoteSetting.localPath.current_value)) != NULL)
    {
	while ((direntp = readdir(dirp)) != NULL)
	{
	    DEBUG1("d_name = \"%s\"\n", direntp-> d_name);
	    if (strcmp(direntp-> d_name, ".")  == 0 ||
		strcmp(direntp-> d_name, "..") == 0 ||
		strcmp(direntp-> d_name, DM_INFO_FILENAME) == 0)
		   continue;
	    else
	    {
		empty = False;
		break;
	    }
        } 
	closedir(dirp);
	if (!empty)
	{
	    if ((buf =
		 MALLOC(strlen(GetGizmoText(TXT_MountPointNotEmpty)) + 
			strlen((char *)remoteSetting.localPath.current_value) +
			strlen((char *)remoteSetting.folderLabel.current_value))
		 ) == NULL)
		    NO_MEMORY_EXIT();

	    sprintf(buf, GetGizmoText(TXT_MountPointNotEmpty),
		    (char *)remoteSetting.localPath.current_value,
		    (char *)remoteSetting.folderLabel.current_value);
	    ndata.text = buf;
	    NoticeCB(PopupShell, (XtPointer)&ndata, NULL);
	    free(buf);
	    return NOT_EMPTY;
	}
    }	   
    return EMPTY;
}


/*
 * remotePropCB
 *
 */

static void 
remotePropCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
    OlFlatCallData * p          = (OlFlatCallData *)call_data;
    NFSWindow *     nfsw        = FindNFSWindow(w);
    PopupGizmo *     popup      = nfsw-> remotePropertiesPrompt;

    switch (p-> item_index)
    {
    case PropApply:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	DEBUG1("case PropApply: PropertyPopupMode = %d\n", PropertyPopupMode);
	if (PropertyPopupMode == True) /* property sheet */
	{
	    if (nfsw-> viewMode != ViewRemote)
	    {
		SetMessage(nfsw-> remotePropertiesPrompt, TXT_ApplyNotRemote,
			   Notice);
		return;
	    }
	    ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
	    if (verifyAllFields() == VALID)
	    {			/* nothing wrong */
		updateData();
		MoveFocus(remoteHostG.textFieldWidget);
		if (bringDownPopups(PopupShell, False) == True)
		    PropertyPopupMode = False;
		return;
	    }
	    return;
	}
	else			/* add new folder mode */
	{
	    if (nfsw-> viewMode != ViewRemote)
	    {
		SetMessage(nfsw-> remotePropertiesPrompt, TXT_AddNotRemote,
			   Notice);
		return;
	    }
	    ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
	    if (verifyAllFields() == VALID)
	    {			/* nothing wrong */
		if (checkMountPoint() == NOT_EMPTY) 
		    return;
		newRemote(True);
		bringDownPopups(PopupShell, False);
	    }
	    else
		return;
	}
	/* FALL THRU */ /* should only get here after successful add */	
    case PropReset:
	doPropReset();
	break;
    case PropCancel:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	PropertyPopupMode = False;
	bringDownPopups(PopupShell, True);
	break;
    case PropHelp:
	if (PropertyPopupMode == True) /* property sheet mode*/
	    PostGizmoHelp(nfsw-> baseWindow-> shell,
			  &RemotePropWindowHelp);
	else
	    PostGizmoHelp(nfsw-> baseWindow-> shell,
			  &RemoteAddWindowHelp);   
	break;
    default:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	DEBUG0("default in remotePropCB taken!!!\n");
    }
}				/* end of remotePropCB */

static void
createRemotePopup(Widget w)
{
    extern NFSWindow *nfsw;
    Arg               arg[5];
    
    nfsw->remotePropertiesPrompt = &RemotePropertiesGizmo;
    PopupShell = CreateGizmo(w, PopupGizmoClass,
			     nfsw->remotePropertiesPrompt, NULL, 0); 
    XtAddCallback(PopupShell , XtNpopdownCallback, hideExtendedCB, NULL);
    OlVaFlatSetValues(RemoteMenu.child, PropApply,
		      XtNsensitive, OwnerRemote, NULL);
    XtSetArg(arg[0], XtNnoneSet, True);
    XtSetValues(AvailableFolders.flatList, arg, 1);
    XtSetArg(arg[0], XtNmaximumSize, LABEL_FIELD_SIZE);
    XtSetValues(folderLabelG.textFieldWidget, arg, 1);
    SetValue(extendedChoice.buttonsWidget, XtNunselectProc, extendedCB,
	      (XtPointer)0);
    return;
}
/*
 * RemoteAddCB
 *
 */

extern void
RemoteAddCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    NFSWindow *     nfsw        = FindNFSWindow(w);
    Arg             arg[10];

    PropertyPopupMode = False;

    if (nfsw->remotePropertiesPrompt == NULL)
	createRemotePopup(w);
    else			
    {
	XtManageChild(Heading.widget);
	XtManageChild(listLabelG.controlWidget);    
    }
    /* ManipulateGizmo must be outside the else or custom options */
    /* won't get initialized */
    ManipulateGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt, 
		    ReinitializeGizmoValue); 
    SetValue(GetPopupGizmoShell(nfsw-> remotePropertiesPrompt), XtNtitle,
	     GetGizmoText(TXT_AddRemoteTitle)); 
    OlVaFlatSetValues(RemoteMenu.child, PropApply,
		      XtNlabel, GetGizmoText(TXT_Add2),
		      XtNmnemonic, *(GetGizmoText(CHR_Add)), NULL);
    XtUnmanageChild(extendedLabelG.controlWidget);    
    resetFocus();
    SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
    MapGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt);

} /* end of RemoteAddCB */

/*
 * RemotePropertyCB
 *
 */

extern void
RemotePropertyCB(Widget w, Boolean displayPopup, XtPointer call_data)
{
    extern NFSWindow   *nfsw;
    Arg             	arg[10];
    struct vfstab      *vp;
    DmObjectPtr     	op;
    ObjectListPtr   	slp = nfsw-> remoteSelectList;
    char               *special, *options, *customOptions;
    char	       *token, *ptr;
    AccessItemIndex     access = ReadWrite; /* matches NFS default */
    mountTypeIndex	type   = HardMount; /* matches NFS default */

    if (displayPopup == False && PropertyPopupMode == False)
	return; /* don't switch to selected data if in add mode */

    RETURN_IF_NULL(slp, TXT_SelectFolderProp);
    if (displayPopup && slp-> next != NULL)
    {
	SetMessage(MainWindow ,TXT_Select1Folder, Base);
	return;
    }
    RETURN_IF_NULL((op = slp-> op), TXT_BadOp);
    RETURN_IF_NULL((vp = (struct vfstab *)op-> objectdata), TXT_BadVp);
    RETURN_IF_NULL(vp-> vfs_special, TXT_BadVp);
    RETURN_IF_NULL(vp-> vfs_mountp, TXT_BadVp);

    PropertyPopupMode = True;

    if (nfsw->remotePropertiesPrompt == NULL)
    {
	if (displayPopup == False)
	    return;
	createRemotePopup(w);
	XtUnmanageChild(extendedLabelG.controlWidget);    
    }
    
    SetValue(PopupShell, XtNtitle, GetGizmoText(TXT_PropertyRemoteTitle)); 
    OlVaFlatSetValues(RemoteMenu.child, PropApply,
		      XtNlabel, GetGizmoText(TXT_OK),
		      XtNmnemonic, *(GetGizmoText(CHR_OK)), NULL);

    XtUnmanageChild(Heading.widget);
    XtUnmanageChild(listLabelG.controlWidget);    

    /* fill in fields */

    special = STRDUP(vp-> vfs_special);
    setPreviousValue(&remoteSetting.host, strtok(special, ":"));
    setPreviousValue(&remoteSetting.remotePath, strtok(NULL, ":"));
    if (user.prevVal != NULL)
	FREE(user.prevVal);
    user.prevVal = strdup(remoteSetting.host.previous_value);
    FREE(special);
    setPreviousValue(&remoteSetting.folderLabel, vp-> vfs_fsckpass);
    setPreviousValue(&remoteSetting.localPath, vp-> vfs_mountp);
    setChoicePreviousValue(&remoteSetting.bootMount,
			  strcmp(vp-> vfs_automnt, "no") == 0 ? no : yes);
    ptr = options = STRDUP(vp-> vfs_mntopts);
    if ((customOptions = MALLOC(strlen(options)+2)) == NULL)
	NO_MEMORY_EXIT();

    /* customOptions[1] = EOS in case there's no comma to skip below */
    customOptions[0] = customOptions[1] = EOS;
    while (token = strtok(ptr, ","))
    {
	ptr = NULL;
	if (strcmp(token, "ro") == 0)
	    access = ReadOnly;
	else if (strcmp(token, "rw") == 0)
	    access = ReadWrite;
	else if (strcmp(token, "hard") == 0)
	    type = HardMount;
	else if (strcmp(token, "soft") == 0)
	    type = SoftMount;
	else
	{   /* 1st time we hit this, we get an unwanted leading comma */
	    strcat(customOptions, ",");
	    strcat(customOptions, token);
	}
    }
    customOptions++;		/* skip leading comma */
    setChoicePreviousValue(&remoteSetting.writable,  access);
    setChoicePreviousValue(&remoteSetting.mountType, type);
    setPreviousValue(&remoteSetting.customOptions, customOptions);
    FREE(--customOptions);	/* un-skip leading comma */
    FREE(options);
    ManipulateGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt, 
		    ResetGizmoValue); 
    if (displayPopup == True)
    {
	resetFocus();
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	MapGizmo(PopupGizmoClass, nfsw->remotePropertiesPrompt);
    }

} /* end of RemotePropertyCB */


static void 
setLabel(char * path, Boolean force)
{
    char * data;
    int last;
    if ( force == True ||
	*((char *)remoteSetting.folderLabel.current_value) == EOS) 
    {
	data = STRDUP(path);

	/* remove trailing / */
	last = strlen(data) - 1;
	if (last > 0 && data[last] == '/')
	    data[last] = EOS;

	setInitialValue(&remoteSetting.folderLabel, basename(data));
	ManipulateGizmo(InputGizmoClass, &folderLabelG,
			ReinitializeGizmoValue);
	setInitialValue(&remoteSetting.folderLabel, "");
	FREE(data);
    }
}

static void 
ExecuteCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}
static void 
SelectCB (Widget wid, XtPointer client_data, OlFlatDropCallData *call_data)
{
    Cardinal  item_index = call_data->item_data.item_index;
    char **       fields = (char **)GetListField(&AvailableFolders, item_index);
    NFSWindow *     nfsw = FindNFSWindow(wid);

    DEBUG2("client_data=%s, %x\n", (char *)client_data, client_data);
    DEBUG1("field1=%s\n", fields[0]);
    DEBUG1("remote path current_value before reset: \"%s\"\n",
	  remoteSetting.remotePath.current_value);
    DEBUG1("remote path previous_value before reset: \"%s\"\n",
	  remoteSetting.remotePath.previous_value);
    DEBUG1("remote path initial_value before reset: \"%s\"\n",
	  remoteSetting.remotePath.initial_value);

    SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);

    ManipulateGizmo(InputGizmoClass, &folderLabelG, GetGizmoValue);
    setLabel(fields[0], True);

    setPreviousValue(&remoteSetting.remotePath, fields[0]);
    setPreviousValue(&remoteSetting.localPath, "");

    ManipulateGizmo(InputGizmoClass, &remotePathG, ResetGizmoValue); 
    ManipulateGizmo(InputGizmoClass, &localPathG,  ResetGizmoValue); 
    MoveFocus(localPathG.textFieldWidget);
    DEBUG1("remote path current_value after reset: \"%s\"\n",
	  remoteSetting.remotePath.current_value);
    DEBUG1("remote path previous_value after reset: \"%s\"\n",
	  remoteSetting.remotePath.previous_value);
    DEBUG1("remote path initial_value after reset: \"%s\"\n",
	  remoteSetting.remotePath.initial_value);
}

static void 
UnselectCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}

static void
GetAvailableList (textField, which, tf_verify)
Widget	textField;
int	which;
OlTextFieldVerify       *tf_verify;
{
    extern NFSWindow *nfsw;
    char 	 buf[MAXPATHLEN+SYS_NMLN+2];
    char 	*host;
    char 	*token, *charPtr, *errorText;
    char       **fields;
    int 	 i, status, num;
    ListHead 	*headPtr;
    FILE	*fp[2];
    int		 exitCode = 2000; /* bigger than any legal exit code */

    if (PropertyPopupMode == True) /* property sheet mode*/
	return;

    busyCursor();
    ManipulateGizmo(InputGizmoClass, &remoteHostG, GetGizmoValue);
    host = remoteSetting.host.current_value;
    if(user.hostSelected &&
           !strcmp(remoteSetting.host.current_value, user.prevVal)){
                ;                        /*valid host do nothing*/
    } else                               /* if verify host is invalid */
        if(verifyHost((char *)remoteSetting.host.current_value) == INVALID) {
	    MoveFocus(remoteHostG.textFieldWidget);
	    ManipulateGizmo(ListGizmoClass, &AvailableFolders, ResetGizmoValue);
	    standardCursor();
	    return;
        }
    SetMessage(nfsw-> remotePropertiesPrompt, TXT_DoingDfshares, Popup);

    headPtr = (ListHead *) (AvailableFolders.settings->previous_value);
    if (headPtr != NULL)
	FreeList(headPtr);
    headPtr->numFields = 1;
    headPtr->size = 0;
    headPtr->list = NULL;
    
    DEBUG2("host = \"%s\", in hex, host = %x\n", host, host);

    /* fork and exec dfshares(1) to get AvailableFolders of available folders */

    sprintf(buf, "%s -F nfs -h -o %s", DFSHARES, host);
    if (p3open (buf, fp) != 0)
    {
	errorText = GetGizmoText(TXT_PopenFailed);
	fprintf (stderr, errorText, buf);
	SetMessage(nfsw-> remotePropertiesPrompt, TXT_dfsharesFailed, Notice);
	standardCursor();
	return;
    }

    /* Parse buf for lines and put each line into a list item */
    for ( i=0;
	 (num = fscanf(fp[1], " %s %*s %*s %*s", buf)) != EOF;
	 i++)
    {
	DEBUG1("buf=\"%s\"\n", buf);
	headPtr->size++;
	headPtr->list = (ListItem *)REALLOC((char *)headPtr->list,
					      sizeof(ListItem)*headPtr->size);
	headPtr->list[i].set = False;
        headPtr->list[i].fields = (XtArgVal)MALLOC
	    (sizeof (XtArgVal *) * headPtr->numFields);
	headPtr->list[i].clientData = (XtArgVal)STRDUP(buf);
	fields = (char **)headPtr->list[i].fields;
	token  = strtok(buf,":");  /* host name */
	token  = strtok(NULL,":"); /* resource */
	fields[0] = STRDUP(token);
    }
    status = p3close (fp, BAD_PID);
    if (WIFEXITED(status) && (exitCode = WEXITSTATUS(status)) == 0) 
    {	/* success */
	ManipulateGizmo(ListGizmoClass, &AvailableFolders, ResetGizmoValue);
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	MoveFocus(AvailableFolders.flatList);
    }
    else
    {
	ManipulateGizmo(ListGizmoClass, &AvailableFolders,
			ReinitializeGizmoValue);
	DEBUG2("dfshares status=%x, exit code=%d\n", status, exitCode);
	switch (exitCode)
	{
	case 34:
	    sprintf(buf, GetGizmoText(TXT_dfsharesRPCerror), host);
	    SetMessage(nfsw-> remotePropertiesPrompt, buf, Notice);
	    break;
	case 35:
	    sprintf(buf, GetGizmoText(TXT_dfsharesNoExports), host);
	    SetMessage(nfsw-> remotePropertiesPrompt, buf, Notice);
	    break;
	default:
	    SetMessage(nfsw-> remotePropertiesPrompt,
		       TXT_dfsharesFailed, Popup);
	    break;
	}
    }
    standardCursor();
    return;
} /* GetAvailableList */


static Boolean
verifyAllFields()
{
    extern  NFSWindow *nfsw;
    Gizmo   popup = nfsw-> remotePropertiesPrompt;
    Arg	    arg[1];
    OlFontList *font_list;

    if (verifyRemotePath((char *)remoteSetting.remotePath.current_value,
			 popup)	== INVALID) 
	return INVALID;

    setLabel(remoteSetting.remotePath.current_value, False);

    XtSetArg(arg[0], XtNfontGroup, &font_list);
    XtGetValues(folderLabelG.textFieldWidget, arg, 1);
    if (font_list == NULL) 
	is_multi_byte = False;
    else 
	is_multi_byte = True;


    if (verifyLabel((char *)remoteSetting.folderLabel.current_value,
		    popup) == INVALID) 
	return INVALID;

    if(user.hostSelected && 
	   !strcmp(remoteSetting.host.current_value, user.prevVal)){
		printf("Valid host no check\n"); /* JC no need */
                ; 			 /*valid host do nothing*/ 
    } else				 /* if verify host is invalid */
        if(verifyHost((char *)remoteSetting.host.current_value) == INVALID)
		     return INVALID;

    setPreviousValue(&remoteSetting.host, remoteSetting.host.current_value);

    if (verifyMountOptions((char *)remoteSetting.customOptions.current_value,
		      popup) == INVALID) 
	return INVALID;

    /* The localPath must be verified LAST because it may put up a     */
    /* notice to allow user to create a missing dir.  This would cause */
    /* us to lose the execution thread so the notice callback          */
    /* finishes the work of the apply callback that got us to here.    */

    if ((PropertyPopupMode == False ||         /* adding new item or.. */
	 strcmp(remoteSetting.localPath.current_value, /* path changed */
		remoteSetting.localPath.previous_value) != 0) &&
	verifyLocalPath((char **)&remoteSetting.localPath.current_value,
			popup, remoteMode, postVerifyCB) == INVALID) 
	return INVALID;

    return VALID;
}



static void 
updateData()
{
    extern NFSWindow * nfsw;
    extern Boolean isMounted();
    DeleteFlags flags = Silent;
    Boolean mounted = True;
    struct vfstab *vp;
    Boolean result;

    RETURN_IF_NULL(nfsw-> remoteSelectList, TXT_ReselectFolderProp);
    RETURN_IF_NULL(nfsw-> remoteSelectList-> op, TXT_BadOp);
    vp = (struct vfstab *)nfsw-> remoteSelectList-> op -> objectdata;
    RETURN_IF_NULL(vp, TXT_BadVp);

    if (vp != NULL && vp-> vfs_special != NULL && vp-> vfs_mountp != NULL)
	mounted = isMounted(vp-> vfs_mountp, vp-> vfs_special);
    if (remoteSetting.remotePath.current_value !=
	remoteSetting.remotePath.previous_value ||
	remoteSetting.localPath.current_value !=
	remoteSetting.localPath.previous_value ||
	remoteSetting.writable.current_value !=
	remoteSetting.writable.previous_value ||
	remoteSetting.mountType.current_value !=
	remoteSetting.mountType.previous_value ||
	remoteSetting.host.current_value !=
	remoteSetting.host.previous_value)
	flags = ReDoIt_Silent;

    result = DeleteRemoteCB((Widget)NULL, (XtPointer)NULL, (XtPointer)flags);
    if (result != FAILURE) newRemote(mounted);
    return;
}
 
static void
getHostCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow * nfsw;
    static HelpText help;

    help.title = HostWindowHelp.title;
    help.file = HostWindowHelp.filename;
    help.section  = HostWindowHelp.section;
    SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
    user.help = &help;
    user.text = remoteHostG.textFieldWidget;
    user.hostSelected = False;
    HostCB(wid, &user, call_data);
    return;
}

static void
findFolderCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    static FileGizmo *fileG = NULL;
    char *home;

    if (fileG == NULL)
    {
	fileG = &fileFolderPopup;
	if ((home = getenv("HOME")) == NULL)
		home = "";
	fileG-> directory = STRDUP(home);
	CreateGizmo(wid, FileGizmoClass, fileG, NULL, 0);
    }
    SetFileGizmoMessage(&fileFolderPopup, TXT_MountPointPrompt);
    MapGizmo(FileGizmoClass, fileG);
	
    return;
}

static void
filePopupCB(Widget wid, XtPointer client_data, OlFlatDropCallData *call_data)
{
    extern NFSWindow *nfsw;
    Cardinal  item_index = call_data->item_data.item_index;
    Widget shell       = GetFileGizmoShell(&fileFolderPopup);

    switch ((filePopupItemIndex)item_index)
    {
    case ffApply:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	setInitialValue(&remoteSetting.localPath,
			GetFilePath(&fileFolderPopup));
	ManipulateGizmo(InputGizmoClass, &localPathG,
			ReinitializeGizmoValue);
	setInitialValue(&remoteSetting.localPath, "");
	BringDownPopup(shell);
	break;
    case ffCancel:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
	XtPopdown(shell);
	break;
    case ffHelp:
        PostGizmoHelp(nfsw-> baseWindow-> shell, &FindFolderWindowHelp); 
	break;
    default:
	DEBUG0("default taken in filePopupCB\n");
	break;
    }
    return;
}

/* postVerifyCB is called after we create a directory for the mointpoint. */
/* Here we do whatever would have been done if verifyLocalPath had */
/* returned VALID */

static void 
postVerifyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;
    Cardinal  item_index =
	((OlFlatDropCallData*)call_data)->item_data.item_index;   
    Widget	nshell = GetModalGizmoShell(nfsw-> noticePopup);
    pathData   *pdata  = (pathData *)client_data;
    char       *path   =  pdata-> path;
    int         result;
    mode_t      st_mode = DIR_CREATE_MODE;
    
    switch ((noticeIndex)item_index)
    {
    case NoticeDoIt:
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	result = mkdirp(path, st_mode);
	XtPopdown(nshell);
	if (result < 0)
	{
	    SetMessage(nfsw-> remotePropertiesPrompt, TXT_DirNotCreated, Notice);
	}
	else
	{
	    if (PropertyPopupMode == True)
	    {
		updateData();
		if (bringDownPopups(PopupShell, False) == True)
		    PropertyPopupMode = False;
	    }
	    else
	    {
		newRemote(True);
		(void)bringDownPopups(PopupShell, False);
	    }
	}
	break;
    case NoticeCancel:
	/* FIX: do we want a "%s not created" message here? */
	SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);
	XtPopdown(nshell);
	break;
    case NoticeHelp:
        PostGizmoHelp(nfsw-> baseWindow-> shell, &CreateMountPointNoticeHelp); 
	break;
    default:
	SetMessage(nfsw, " ", Base);
	DEBUG0("default in remote postVerifyCB taken!!!\n");
    }
    FREE((char *)pdata);
    return;
}

static void 
extendedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;

    SetMessage(nfsw-> remotePropertiesPrompt, " ", Popup);

    ManipulateGizmo(ChoiceGizmoClass, &extendedChoice, GetGizmoValue);
    if (*(char *)(remoteSetting.extended.current_value) == 'x')
    {
	XtManageChild(extendedLabelG.controlWidget);    
	MoveFocus(customOptionsG.textFieldWidget);
    }
    else
	XtUnmanageChild(extendedLabelG.controlWidget);    
}

static void 
hideExtendedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    ManipulateGizmo(ChoiceGizmoClass, &extendedChoice,
		    ReinitializeGizmoValue); 
    XtUnmanageChild(extendedLabelG.controlWidget);
}

static void
resetFocus()
{
    Arg arg[5];

    XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)remoteHostG.textFieldWidget);
    XtSetValues(PopupShell, arg, 1);
}

static void 
busyCursor()
{
    Widget w = PopupShell;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlBusyCursor(XtScreen(w)));
    XSync(XtDisplay(w), False);
    return;
}	


static void 
standardCursor()
{
    Widget w = PopupShell;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlStandardCursor(XtScreen(w)));
    return;
}
