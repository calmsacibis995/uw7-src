#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/mount.c	1.25"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	mount.c      Mount or unmount a folder, or see if a
 *                           folder is mounted
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/mnttab.h>
#include <sys/vfstab.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"
#include "proc.h"
#include "utilities.h"

extern NFSWindow *MainWindow, *nfsw;
extern DmObjectPtr	new_object();
extern void 		AddObjectToContainer();
extern void 		UpdateView();
extern int 		popenAddInput();
extern int		p3close();
static void		PostMount2();

#define UNMOUNTCMD "/sbin/umount"
#define MOUNTCMD "/sbin/mount -F nfs"
#define DASHO	 "-o"
static FILE *Mount_fp = NULL;


static FILE *
Open_mnttab()
{
    if (Mount_fp == NULL)
    {
        if ((Mount_fp = fopen(MNTTAB, "r")) == NULL)
	{
	    SetMessage(MainWindow, TXT_mnttabOpenError, Notice);
	    return NULL;
	}
    }
    else
    {
	DEBUG0("rewinding...\n");
	rewind(Mount_fp);
    }
    return Mount_fp;
}

extern Boolean
isMounted (char *mountp, char *special)
{
    struct mnttab entry;
    static struct mnttab lookFor = { NULL, NULL, "nfs", NULL, NULL };
    int status;
    char * errorText;

DEBUG2("isMounted: mountp=%s, special=%s.\n", mountp, special);
    if (Open_mnttab() == NULL)
	return False;
    DEBUG1("	file pos=%d\n", ftell(Mount_fp));
    lookFor.mnt_mountp  = mountp;
    lookFor.mnt_special = special;

    status = getmntany(Mount_fp, &entry, &lookFor);

    DEBUG2("	file pos=%d, status=%d\n", ftell(Mount_fp), status);
    
    if (status == 0)
	return True;
    else if (status > 0)
    {
	errorText = GetGizmoText(TXT_Bad_mnttabEntry);
	(void)fprintf(stderr, "%s %d\n", errorText, status);
    }
    return False;
}

static void
PostMount(XtPointer client_data, int *source, XtInputId *id)
{
    int count;
    char buf[BUFSIZ];
    
    count = read(*source, buf, BUFSIZ);
    buf[count] = EOS;		/* only needed for DEBUG statement */
    DEBUG1("PostMount: buf=\"%s\"\n", buf);

    if (count == 0)
	PostMount2(client_data, source, id);
    return;
}
    

static void
PostMount2(XtPointer client_data, int *source, XtInputId *id)
{
    inputProcData  *data = (inputProcData *)client_data;
    DmObjectPtr op = data->op;
    struct vfstab *vp;
    int status, exitCode;
    char buf[BUFSIZ];
    int  maxErrorTxt;
    static char *errorTxt[] =	/* if you change the data type of */
    {				/* errorTxt then change the       */
	TXT_MountExecFailed,	/* maxErrorTxt calculation below  */
	TXT_MountRetUnknown,
	TXT_MountRetUnknown,	/* really RetOK but only prints if */
				/* IsMounted says its not mounted  */
	TXT_MountRetUsage,
	TXT_MountRetOptions,
	TXT_MountRetArgs,
	TXT_MountRetMacInstall,
	TXT_MountRetFstypeMax,
	TXT_MountRetFstypeOne,
	TXT_MountRetVfsOpen,
	TXT_MountRetVfsGetent,
	TXT_MountRetMntOpen,
	TXT_MountRetMntLock,
	TXT_MountRetMntGetent,
	TXT_MountRetMpStat,
	TXT_MountRetMpUnk,
	TXT_MountRetMpAbspath,
	TXT_MountRetMpExist,
	TXT_MountRetMpUnklvl,
	TXT_MountRetMpRange,
	TXT_MountRetDevUnk,
	TXT_MountRetDdbNoent,
	TXT_MountRetDdbInval,
	TXT_MountRetDdbAccess,
	TXT_MountRetLvlInval,
	TXT_MountRetCeiling,
	TXT_MountRetLvlRange,
	TXT_MountRetLtdbAccess,
	TXT_MountRetLvlPrint,
	TXT_MountRetMallocErr,
	TXT_MountRetWaitErr,
	TXT_MountRetForkErr,
	TXT_MountRetExecErr,
	TXT_MountRetExecAccess,
	TXT_MountRetUsage2,
	TXT_MountRetEbusy,
	TXT_MountRetEperm,
	TXT_MountRetEnxio,
	TXT_MountRetEnotdir,
	TXT_MountRetEnoent,
	TXT_MountRetEinval,
	TXT_MountRetEnotblk,
	TXT_MountRetErofs,
	TXT_MountRetEnospc,
	TXT_MountRetEnoload,
	TXT_MountRetEnodev,
	TXT_MountRetMisc,
	TXT_MountRetTmpOpen,
	TXT_MountRetTmpWrite,
	TXT_MountRetMntOpen2,
	TXT_MountRetMntLock2,
	TXT_MountRetMntToolong,
	TXT_MountRetMntToofew,
	TXT_MountRetMntToomany,
	TXT_MountRetVfsOpen2,
	TXT_MountRetVfsStat,
	TXT_MountRetVfsNoent,
	TXT_MountRetMalloc,
	"", "", "", "", "",	/* 56-60 */  /* exit codes not...   */
	"", "", "", "", "",	/* 61-65 */  /* applicable to...    */
	"", "", "", "",		/* 66-69 */  /* remote filesystems. */
	TXT_MountRetRetry,
	TXT_MountRetGiveUp,
	TXT_MountRetHP,
	TXT_MountRetInvOp,
	TXT_MountRetServ,
	TXT_MountRetAddr,
	TXT_MountRetSec,
	TXT_MountRetAccess,
	TXT_MountRetNoent,
	TXT_MountRetNfsDown,
    };

    maxErrorTxt = (sizeof(errorTxt) / sizeof(char *)) - 1;

    XtRemoveInput(data-> readId);
    XtRemoveInput(data-> exceptId);

    status = p3close(data-> fp, data-> pid);

    DEBUG1("PostMount2: status = %x\n", status);

    if ((vp = (struct vfstab *)op-> objectdata) == NULL ||
	vp-> vfs_special == NULL || vp-> vfs_mountp == NULL)
    {
	DEBUG0("Bad op in PostMount\n");
	FREE((char *)data);
	return;
    }
    if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
    {
	UpdateView(ViewRemote);
	sprintf(buf, GetGizmoText(TXT_MountSucceeded), vp-> vfs_fsckpass);
	SetMessage(MainWindow, buf, Base);
    }
    else
    {
	/* error condition exit codes from NFS mount start at 32 */
	if (!WIFEXITED(status))
	    exitCode = 0;
	else if ((exitCode = WEXITSTATUS(status) + 2)  > maxErrorTxt)
	    exitCode = 1;	/* unknown exitCode */
	sprintf(buf, GetGizmoText(TXT_MountRetPrefix), vp-> vfs_fsckpass,
		GetGizmoText(errorTxt[exitCode]));
	SetMessage(MainWindow, buf, Notice);
    }
    OlVaFlatSetValues(nfsw-> iconbox, data-> index, XtNbusy, False, NULL);

    FREE((char *)data);
    return;
}


extern void
MountIt(struct vfstab *vp, DmObjectPtr op, Cardinal index)
{
    char	mountCommand[BUFSIZ];
    inputProcData *data;

    if ((data = (inputProcData *)MALLOC(sizeof(inputProcData))) == NULL)
	NO_MEMORY_EXIT();

    data-> op = op;
    data-> index = index;

    sprintf(mountCommand, "%s %s %s %s %s", MOUNTCMD,
	    (*vp-> vfs_mntopts == EOS) ? "" : DASHO,
	    vp-> vfs_mntopts,
	    vp-> vfs_special,
	    vp-> vfs_mountp);
    DEBUG1("mountCommand=\"%s\"\n", mountCommand);
    OlVaFlatSetValues(nfsw-> iconbox, index, XtNbusy, True, NULL);
    popenAddInput(mountCommand, PostMount, PostMount2, (XtPointer)data);
    return;
}


extern void
MountCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern MenuGizmo ActionsMenu;
    struct vfstab * vp;
    DmObjectPtr     op;
    ObjectListPtr   slp = nfsw-> remoteSelectList;

    DEBUG0("MountCB entry\n");

    RETURN_IF_NULL(slp, TXT_NothingToMount);

    for ( ; slp; slp = slp-> next)
    {	 
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Notice);
	    continue;
	}
	if ((vp = (struct vfstab *)op-> objectdata) == NULL ||
	    vp-> vfs_special == NULL || vp-> vfs_mountp == NULL)
	{
	    SetMessage(MainWindow, TXT_BadVp, Notice);
	    continue;
	}
	if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
	{
	    op-> fcp = nfsw-> mounted_fcp;
	    OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			      XtNsensitive, OwnerRemote, NULL);
	    SetMessage(MainWindow, TXT_AlreadyMounted, Base);
	    continue;
	}
	else			/* mount it */
	    MountIt(vp, op, slp-> index);
	OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);

    }
    return;
}


extern int
UnMountIt(struct vfstab *vp, DmObjectPtr op)
{
    int 	status;
    char	unmountCommand[BUFSIZ];

    sprintf(unmountCommand, "%s %s", UNMOUNTCMD,
	    vp-> vfs_mountp);
    DEBUG1("unmountCommand=\"%s\"\n", unmountCommand);

    status = system(unmountCommand);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
	op-> fcp = nfsw-> remote_fcp;
	SetMessage(MainWindow, TXT_UnMountSucceeded, Base);
    }
    else
    {
	/* FIX: parse status to report specific error */
	DEBUG2("umount attempt returns %d, errno=%d\n", status, errno);
    }
    return status;
}



extern void
UnMountCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern MenuGizmo ActionsMenu;
    struct vfstab * vp;
    DmObjectPtr     op;
    ObjectListPtr   slp = nfsw-> remoteSelectList;

    DEBUG0("UnMountCB entry\n");

    RETURN_IF_NULL(slp, TXT_NothingToUnMount);

    for ( ; slp; slp = slp-> next)
    {	 
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Notice);
	    continue;
	}
	if ((vp = (struct vfstab *)op-> objectdata) == NULL ||
	    vp-> vfs_special == NULL || vp-> vfs_mountp == NULL)
	{
	    SetMessage(MainWindow, TXT_BadVp, Notice);
	    continue;
	}
	if (!isMounted(vp-> vfs_mountp, vp-> vfs_special))
	{
	    op-> fcp = nfsw-> remote_fcp;
	    OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			      XtNsensitive, OwnerRemote, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			      XtNsensitive, False, NULL);
	    SetMessage(MainWindow, TXT_AlreadyUnMounted, Base);
	    continue;
	}
	else			/* unmount it */
	{
	    if (UnMountIt(vp, op) == 0)
	    {
		OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);
		OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
				  XtNsensitive, OwnerRemote, NULL);
		OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
				  XtNsensitive, False, NULL);
	    }
	else  
	SetMessage(MainWindow, TXT_UnMountFailed, Notice);
	}
    }
    return;
}

extern void
getMountedItems()
{
    struct mnttab entry;
    static struct mnttab lookFor = { NULL, NULL, "nfs", NULL, NULL };
    struct vfstab vfs_entry;
    struct vfstab vfs_lookFor;
    FILE         *fp, *dtfp;
    int		  status;
    char 	  buf[SMALLBUFSIZE] = "*";
    char         *errorText;
    DmObjectPtr op;

    if (Open_mnttab() == NULL)
	return;
    RETURN_IF_NULL((fp = fopen(DTVFSTAB, "r")), TXT_vfstabOpenError);

    vfsnull(&vfs_lookFor);
    vfs_lookFor.vfs_fstype = "nfs";

    while ((status = getmntany(Mount_fp, &entry, &lookFor) >= 0))
    {
	DEBUG1("getmntany status =%d\n", status);
	if ( status > 0)
	{
	    DEBUG4("special=%s, mountp=%s, fstype=%s, opts=%s\n",
		   entry.mnt_special, entry.mnt_mountp,
		   entry.mnt_fstype,  entry.mnt_mntopts);
/**
	    errorText = GetGizmoText(TXT_Bad_mnttabEntry);
	    (void)fprintf(stderr, "%s %d\n", errorText, status);
	    continue;
**/
	}
	vfs_lookFor.vfs_special = entry.mnt_special;
	vfs_lookFor.vfs_mountp = entry.mnt_mountp;
	if ((status = getvfsany(fp, &vfs_entry, &vfs_lookFor)) != 0)
	{
	    /* found mounted nfs folder thats not in vfstab */
	    strncpy(&buf[1], basename(entry.mnt_special), SMALLBUFSIZE - 1);
	    vfsnull(&vfs_entry);
	    vfs_entry.vfs_special  = entry.mnt_special;
	    vfs_entry.vfs_mountp   = entry.mnt_mountp;
	    vfs_entry.vfs_fstype   = "nfs";
	    vfs_entry.vfs_automnt  = "no";
	    vfs_entry.vfs_mntopts  = entry.mnt_mntopts;

	    /* add to DTVFSTAB, VFSTAB */

	    if ((dtfp = fopen(VFSTAB, "a+")) != NULL)
	    {
		putvfsent(dtfp, &vfs_entry);
		fclose(dtfp);
	    }
	    vfs_entry.vfs_fsckpass = buf;
	    if ((dtfp = fopen(DTVFSTAB, "a+")) != NULL)
	    {
		putvfsent(dtfp, &vfs_entry);
		fclose(dtfp);
	    }
	    if ((op = new_object(vfs_entry.vfs_fsckpass, 0, 0,
				 Copy_vfstab(&vfs_entry),
				 nfsRemote)) == (DmObjectPtr)NULL)
		NO_MEMORY_EXIT();
	    op-> fcp = nfsw-> mounted_fcp;
	    AddObjectToContainer(op);
	}
	rewind(fp);
    }
    fclose(fp); 
    return;
}
