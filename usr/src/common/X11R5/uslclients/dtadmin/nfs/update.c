#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/update.c	1.11"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	update.c  update the icon box.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/vfstab.h>

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

#include "text.h"
#include "nfs.h"
#include "sharetab.h"
#include "local.h"
#include "notice.h"
#include "utilities.h"

extern Boolean		isMounted();
extern Boolean		isShared();
extern DmObjectPtr	new_object();
extern void		new_container();
extern dfstabEntryType	Get_dfstab_entry();
extern void		getMountedItems();
extern void		AddObjectToContainer();
static void		getVfstabItems();
extern struct vfstab 	*Copy_vfstab();
extern DmFclassPtr	new_fileclass();
extern NFSWindow 	*nfsw;
extern NFSWindow 	*MainWindow;

static void
restoreVfstabItem(FILE *fp, struct vfstab *vp)
{
    struct vfstab entry, *lookFor;
    int           status;

    if (fp == NULL)
	return;

    vp-> vfs_fsckpass = NULL;	/* discard label */
    lookFor = Copy_vfstab(vp);	/* *vp is static memory used by getvfsany */
    if ((status = getvfsany(fp, &entry, lookFor)) != 0)
    {
        DEBUG1("getvfsany status =%d\n", status);
	/* found dtvfstab entry thats not in vfstab */
	putvfsent(fp, lookFor);
    }
    Free_vfstab(lookFor);
    rewind(fp);
    return;
}


extern void
GetRemoteContainerItems()
{
    struct vfstab vfs_entry, *vp = &vfs_entry;
    FILE *        dtfp, *fp;
    int		  status;
    int		  remoteCount = 0, mountedCount = 0;
    char *	  errorText;
    IconType      type;
    DmObjectPtr op;

    nfsw-> remote_fcp = new_fileclass(IconFilenames[type=RemoteIcon]);
    nfsw-> mounted_fcp = new_fileclass(IconFilenames[type=MountedIcon]);
    new_container(DTVFSTAB);

    RETURN_IF_NULL((dtfp = fopen(DTVFSTAB, "r")), TXT_vfstabOpenError);
    if ((fp = fopen(VFSTAB, "a+")) == NULL)
    {
	(void)fprintf(stderr, GetGizmoText(TXT_vfstabOpenError));
    }

    while ((status = getvfsent(dtfp, vp)) != -1)
    {
	if ( status != 0)
	{
	    errorText = GetGizmoText(TXT_Bad_vfstabEntry);
	    (void)fprintf(stderr, "%s %d\n", errorText, status);
	    continue;
	}
	if ((op = new_object(vp-> vfs_fsckpass, 0, 0, Copy_vfstab(vp),
			     nfsRemote)) == (DmObjectPtr)OL_NO_ITEM)
	{
/* FIX: change to footer message */
	    DEBUG2("Duplicate dtvfstab entry: name=%s mount point=%s\n",
		   vp-> vfs_fsckpass, vp-> vfs_mountp);
	    continue;
	}
	if (op == (DmObjectPtr)NULL)
	    NO_MEMORY_EXIT();
	    
	if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
	{
	    mountedCount++;
	    op-> fcp = nfsw-> mounted_fcp;
	}
	else
	{
	    remoteCount++;
	    op-> fcp = nfsw-> remote_fcp;
	}
	AddObjectToContainer(op);
	restoreVfstabItem(fp, vp);
    }
    fclose(dtfp);
    if (fp)
	fclose(fp);
    getVfstabItems();
    getMountedItems();

    return;
}


static void
getVfstabItems()
{
    struct vfstab entry, *vp;
    static struct vfstab lookFor;
    FILE         *fp, *dtfp;
    int		  status;
    char 	 *errorText;
    char 	  buf[LABEL_FIELD_SIZE+1] = "*";
    DmObjectPtr op;


    RETURN_IF_NULL((fp   = fopen(VFSTAB, "r")), TXT_vfstabOpenError);
    RETURN_IF_NULL((dtfp = fopen(DTVFSTAB, "a+")), TXT_vfstabOpenError);

    vfsnull(&lookFor);
    lookFor.vfs_fstype = "nfs";
    while ((status = getvfsany(fp, &entry, &lookFor) >= 0))
    {
	DEBUG1("getvfsany status =%d\n", status);
	if ( status > 0)
	{
            DEBUG4("special=%s, mountp=%s, fstype=%s, opts=%s\n",
                   entry.vfs_special, entry.vfs_mountp,
                   entry.vfs_fstype,  entry.vfs_mntopts);
/**
	    errorText = GetGizmoText(TXT_Bad_vfstabEntry);
	    (void)fprintf(stderr, "%s %d\n", errorText, status);
	    continue;
**/
	}
	vp = Copy_vfstab(&entry);
	rewind(dtfp);
	if ((status = getvfsany(dtfp, &entry, vp)) != 0)
	{
	    /* found vfstab entry thats not in dtvfstab */
	    strncpy(&buf[1], basename(vp-> vfs_special), LABEL_FIELD_SIZE);
	    if (vp-> vfs_fsckpass) FREE(vp-> vfs_fsckpass);
	    vp-> vfs_fsckpass = STRDUP(buf);
	    putvfsent(dtfp, vp);
	    if ((op = new_object(vp-> vfs_fsckpass, 0, 0,
				 vp, nfsRemote)) == (DmObjectPtr)NULL)
		NO_MEMORY_EXIT();
	    if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
		op-> fcp = nfsw-> mounted_fcp;
	    else
		op-> fcp = nfsw-> remote_fcp;
	    AddObjectToContainer(op);
	}
	else
	    Free_vfstab(vp);
    }
    fclose(fp); 
    fclose(dtfp); 
    return;
}


extern void 
Get_dfstabEntries()
{
    FILE *        fp;
    char          buf[BUFSIZ];
    IconType	  type;
    dfstab	 *dfsp = NULL;
    struct share *sharep;
    DmObjectPtr   op;  
    dfstabEntryType etype;

    RETURN_IF_NULL((fp = fopen(DFSTAB, "r")), TXT_dfstabOpenError);

    nfsw-> local_fcp = new_fileclass(IconFilenames[type=LocalIcon]);
    nfsw-> shared_fcp = new_fileclass(IconFilenames[type=SharedIcon]);
    new_container(DFSTAB);

    while ((etype = Get_dfstab_entry(fp, &dfsp, buf)) != NoMore)
    {
	if (etype != NFS)
	    continue;
	sharep = dfsp-> sharep;
	if ((op = new_object(sharep-> sh_res, 0, 0, dfsp, nfsLocal)) ==
	    (DmObjectPtr)OL_NO_ITEM)
	{
/* FIX: change to footer message */
	    DEBUG2("Duplicate dfstab entry: name=%s path=%s\n",
		   sharep-> sh_res, sharep-> sh_path);
	    continue;
	}
	if (op == (DmObjectPtr)NULL)
	    NO_MEMORY_EXIT();
	    
	if (isShared(sharep))
	{
	    op-> fcp = nfsw-> shared_fcp;
	}
	else
	{
	    op-> fcp = nfsw-> local_fcp;
	}
	AddObjectToContainer(op);
    }
    fclose(fp);
    return;
}


static void
updateLocalView(int nitems)
{
    extern MenuGizmo ActionsMenu;
    DmItemPtr	  itp = nfsw-> itp;
    DmObjectPtr   op;  
    dfstab       *dfsp;
    struct share *sharep;
    int           index;
    int		  shared_selected = 0, local_selected = 0;

    for (index=0, itp = nfsw-> itp; index < nitems; index++, itp++)
    {
	if(ITEM_MANAGED(itp) == False)
	    continue;
	if ((op = (DmObjectPtr)OBJECT_PTR(itp)) == NULL ||
	    (dfsp = op-> objectdata) == NULL ||
	    (sharep = dfsp-> sharep) == NULL)
	       continue;
	if (isShared(sharep))
	{
	    op-> fcp = nfsw-> shared_fcp;
	    if (ITEM_SELECT(itp)) shared_selected++;
	}
	else
	{
	    op-> fcp = nfsw-> local_fcp;
	    if (ITEM_SELECT(itp)) local_selected++;
	}
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    if (local_selected == 0 && shared_selected > 0)
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			  XtNsensitive, OwnerLocal, NULL);
    }
    else
    {
	if (local_selected > 0 && shared_selected == 0)
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			      XtNsensitive, OwnerLocal, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			      XtNsensitive, False, NULL);
	}
	else
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			      XtNsensitive, False, NULL);
	}
    /* FIX: look for shared resources that aren't we don't know about */
    }
    return;
}


static void
updateRemoteView(int nitems)
{
    extern MenuGizmo ActionsMenu;
    DmItemPtr	  itp = nfsw-> itp;
    DmObjectPtr   op;  
    struct vfstab *vp;
    int           index;
    int		  remote_selected = 0, mounted_selected = 0;

    for (index=0, itp = nfsw-> itp; index < nitems; index++, itp++)
    {
	if(ITEM_MANAGED(itp) == False)
	    continue;
	if ((op = (DmObjectPtr)OBJECT_PTR(itp)) == NULL ||
	    (vp = op-> objectdata) == NULL)
	    continue;
	if (isMounted(vp-> vfs_mountp, vp-> vfs_special))
	{
	    op-> fcp = nfsw-> mounted_fcp;
	    if (ITEM_SELECT(itp)) mounted_selected++;
	}
	else
	{
	    op-> fcp = nfsw-> remote_fcp;
	    if (ITEM_SELECT(itp)) remote_selected++;
	}
	OlFlatRefreshItem(nfsw-> iconbox, index , True);
    }
    if (remote_selected == 0 && mounted_selected > 0)
    {
	OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			  XtNsensitive, OwnerRemote, NULL);
    }
    else
    {
	if (remote_selected > 0 && mounted_selected == 0)
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			      XtNsensitive, OwnerRemote, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			      XtNsensitive, False, NULL);
	}
	else
	{
	    OlVaFlatSetValues(ActionsMenu.child, ActionsConnect,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnconnect,
			      XtNsensitive, False, NULL);
	}
    }
    /* FIX: look for mounted resources that  we don't know about */
    return;
}



extern void
UpdateView(ViewIndex view)
{
    int nitems;

    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);

    if ((view == ViewLocal || view == ViewCurrent) && 
	nfsw-> viewMode == ViewLocal) 
            updateLocalView(nitems);
    else
	if ((view == ViewRemote || view == ViewCurrent) &&
	    nfsw-> viewMode == ViewRemote)
	        updateRemoteView(nitems);

    return;
}

extern void
Get_sharetabEntries()
{

    FILE         *fp;
    int           status;
    Cardinal 	  index, nitems;
    dfstab	  *dfsp;
    struct share  *sharep, *tabsharep;
    DmObjectPtr    op;
    DmItemPtr      ip;

    RETURN_IF_NULL((fp = fopen(SHARETAB, "r")), TXT_sharetabOpenError);
    if (nfsw-> viewMode != ViewLocal) /* code below assumes local view */
    {
	DEBUG0("****Get_sharetabEntries called when not in local view\n");
	return;
    }

    XtVaGetValues(nfsw->iconbox, XtNnumItems, &nitems, 0);

    while ((status = getshare(fp, &tabsharep)) != 0)
    {
	if (status < 0)
	{
	    DEBUG0("bad sharetab entry\n");
	    continue;
	}
	if (strcmp(tabsharep-> sh_fstype, "nfs") != 0)
	    continue;		/* not an NFS resource */

	/* check to see if we know about this resource */
	for (index=0, ip = nfsw-> itp; index < nitems; index++, ip++)
	{
	    op = (DmObjectPtr)OBJECT_PTR(ip);
	    if ((dfsp = (dfstab *)op-> objectdata) == NULL ||
		(sharep = dfsp-> sharep) == NULL)
	    {
		DEBUG1("****bad op in local item list, index = %d\n", index);
		/* FIX: delete bad item from list */
		continue;
	    }
	    if (sharecmp(sharep, tabsharep) == True) /* matched */
		break;
	}
	if (index == nitems)	/* finished loop without breaking, ie */
	{			/* no match */
	    if ((dfsp = (dfstab *)MALLOC(sizeof(dfstab))) == NULL)
		NO_MEMORY_EXIT();	    
	    dfsp-> sharep = sharep = sharedup(tabsharep);
	    dfsp-> autoShare   = False;
	    if ((op = new_object(sharep-> sh_res, 0, 0, dfsp, nfsLocal)) ==
		(DmObjectPtr)OL_NO_ITEM)
	    {
		/* FIX: change to footer message */
		DEBUG2("Duplicate dfstab entry: name=%s path=%s\n",
		       sharep-> sh_res, sharep-> sh_path);
	    }
	    if (op == (DmObjectPtr)NULL)
		NO_MEMORY_EXIT();
	    
	    if (isShared(sharep))
	    {
		op-> fcp = nfsw-> shared_fcp;
	    }
	    else
	    {
		op-> fcp = nfsw-> local_fcp;
	    }
	    AddObjectToContainer(op);
	    writedfs(dfsp);
	}
    }
    fclose(fp);
    return;
}


extern void
TimerUpdate(XtPointer client_data, XtIntervalId *id)
{
    extern void GetStatus();
    extern XtAppContext	 AppContext;
    extern XtIntervalId	 TimerId;
    extern unsigned long UpdateRate;

    GetStatus(ViewCurrent);
    UpdateView(ViewCurrent);
    TimerId = XtAppAddTimeOut(AppContext, UpdateRate, TimerUpdate, NULL);

}

/* turn off Timer Updates when iconized; restart when opened */

extern void
TimerOnOff(Widget wid, XtPointer client_data, XEvent *event, 
	   Boolean *continue_to_dispatch)
{
    extern XtIntervalId	 TimerId;
    Widget toplevel = GetBaseWindowShell(nfsw-> baseWindow);
    int state;

    DEBUG0("TimerOnOff Entry\n");
    state = GetWMState(XtDisplay(toplevel), XtWindow(toplevel));

    if (state == IconicState && TimerId != (XtIntervalId)NULL) 
    {
	XtRemoveTimeOut(TimerId);
	TimerId = (XtIntervalId)NULL;
    }
    else if (state == NormalState && TimerId == (XtIntervalId)NULL)
	TimerUpdate(client_data, (XtIntervalId *)NULL);

    return;
}


