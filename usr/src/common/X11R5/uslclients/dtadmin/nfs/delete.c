#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/delete.c	1.15"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	delete.c  delete table entries
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/mnttab.h>
#include <sys/vfstab.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"

static Boolean vfstabDelete();

extern NFSWindow *MainWindow, *nfsw;
extern void DelObjectFromContainer();
extern void FreeObjectList();
extern int  UnMountIt();
extern Boolean isMounted();
extern void    alignIcons();

extern Boolean
DeleteRemoteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DeleteFlags flags = (DeleteFlags) call_data;
    struct vfstab * vp;
    Dimension       width;
    DmObjectPtr     op;
    DmItemPtr	    ip;
    int		    nitems;
    int		    i;
    Boolean	    result = SUCCESS;
    ObjectListPtr   prev, slp = nfsw-> remoteSelectList;

    DEBUG0(">>DeleteRemoteCB entry\n");

    RETURN_IF_NULL(slp, TXT_SelectFolderDel);
    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);

    for (prev = NULL; slp; prev = slp, slp = slp-> next)
    {	 
	DEBUG0(">>slp loop\n");
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Notice);
	    return FAILURE;
	}
	if ((vp = (struct vfstab *)op-> objectdata) == NULL ||
	    vp-> vfs_special == NULL || vp-> vfs_mountp == NULL)
	{
	    SetMessage(MainWindow, TXT_BadVp, Notice);
	    return FAILURE;
	}

	if ((flags == ReDoIt_Confirm || flags == ReDoIt_Silent) &&
	    isMounted(vp-> vfs_mountp, vp-> vfs_special)) 
	{
	    if (UnMountIt(vp, op) != 0)	{
		/* unmount failed */
		SetMessage(MainWindow, TXT_chgPropertyFail, Notice);
		return FAILURE;
		}
	}
	/* FIX: save a copy of VFSTAB to restore if DTVFSTAB update */
	/* fails */

	if (vfstabDelete(VFSTAB, vp) == FAILURE)
	    continue;
	if (vfstabDelete(DTVFSTAB,   vp) == FAILURE)
	    continue;

	for (i=0, ip = nfsw-> itp; i < nitems; i++, ip++)
	{
	    DEBUG0(">>itp loop\n");
	    if(ITEM_MANAGED(ip) == False)
		continue;
	    if((DmObjectPtr)OBJECT_PTR(ip) == op)
	    {			/* found it */
		DEBUG1(">>found item number %d\n", i);
		OlVaFlatSetValues(nfsw-> iconbox, (int)(ip - nfsw-> itp),
				  XtNmanaged, False,
				  XtNselect, False,
				  0
				  );
		ip-> select = False;
		DelObjectFromContainer(op, nfsRemote);
		DeleteFromObjectList(slp, prev);
		if (flags == ReDoIt_Confirm)
		    SetMessage(MainWindow, TXT_DeleteSucceeded, Base);
		break;
	    }
	}
    }
    unselectAll();
    XtVaGetValues(GetBaseWindowScroller(nfsw-> baseWindow), XtNwidth,
                  &width, 0);
    alignIcons(width);
    DEBUG0(">>DeleteRemoteCB exit\n");
    return SUCCESS;
}

/* TMPDIR must be in the same filesystem as VFSTAB and DTVFSTAB */
#define TMPDIR "/etc/dfs"
#define PREFIX ".dtr"

static Boolean 
vfstabDelete(char * vfstable, struct vfstab *vp)
{
    struct vfstab next;
    struct stat statbuf; /* to hold the original file status */ 
    FILE 	*vfp, *tfp;
    char *      tname, *errorText;
    int         status;

    DEBUG1("--vfstabDelete %s\n", vfstable);
    /* FIX: do we need to lock the file? */
    while (stat(vfstable, &statbuf) < 0)
    {
	DEBUG1("stat failed; errno =%d\n", errno);
	switch (errno)
	{
	case EINTR:
	    break;		/* try again */
	default:
	    SetMessage(MainWindow, TXT_RemoteDeleteFailed, Notice);
	    return FAILURE;
	    break;
	}
    }
    RETURN_VALUE_IF_NULL((vfp = fopen(vfstable, "r")),
			 TXT_vfstabOpenError, FAILURE);
    if ((tname = tempnam(TMPDIR, PREFIX)) == NULL ||
	(tfp   = fopen(tname, "w"))       == NULL)
    {
	fclose(vfp);
	if (tname) free(tname);
	SetMessage(MainWindow, TXT_RemoteDeleteFailed, Notice);
	errorText = GetGizmoText(TXT_tmpfileOpenError);
	fprintf(stderr, "%s\n", errorText);
	return FAILURE;
    }
    DEBUG1("--tempname = %s\n", tname);

    /* copy all entries except the one we're deleting into the tmpfile */
    while ((status = getvfsent(vfp, &next)) != -1)
    {
	if ( status != 0)
	{
	    errorText = GetGizmoText(TXT_Bad_vfstabEntry);
	    (void)fprintf(stderr, "%s %d\n", errorText, status);
	    continue;
	}
	/* FIX: compare label if dtvfstab */
	if ((strcmp(next.vfs_fstype,  vp->vfs_fstype)  != 0)	||
	    (strcmp(next.vfs_special, vp->vfs_special) != 0)	||
	    (strcmp(next.vfs_mountp,  vp->vfs_mountp)  != 0)	||
	    (strcmp(next.vfs_automnt, vp->vfs_automnt) != 0)	||
	    (strcmp(next.vfs_mntopts, vp->vfs_mntopts) != 0))
	{
	    putvfsent(tfp, &next);
	}
	else
	    DEBUG1("++Matched %s\n", next.vfs_special);  
    }
    /* replace the old table with the new. */
    fclose(vfp);
    fclose(tfp);
    if (chmod(tname, statbuf.st_mode) < 0) {
	DEBUG1("chmod failed; errno =%d\n", errno);
    }
    if (chown(tname, statbuf.st_uid, statbuf.st_gid) < 0) {
	DEBUG1("chgrp failed; errno =%d\n", errno);
    }
    if (rename(tname, vfstable) != 0)
    {
	SetMessage(MainWindow, TXT_RemoteDeleteFailed, Notice);
	errorText = GetGizmoText(TXT_RenameError);
	fprintf(stderr, errorText, tname, vfstable, errno);
	return FAILURE;
    }
    DEBUG0("--Renamed file\n");
    if (tname)
	free(tname);
    return SUCCESS;
}
