#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/share.c	1.12"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	share.c      Share or unshare a folder, or see if a
 *                           folder is shareded
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/wait.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"
#include "sharetab.h"
#include "local.h"

extern NFSWindow *MainWindow, *nfsw;
extern DmObjectPtr	new_object();

extern Boolean 
isShared(struct share * sharep)
{
    extern Boolean sharecmp();
    extern question NFSserver;
    FILE * fp;
    struct share *entry;

    if (NFSserver == q_No)
	return False;

    fp = fopen(SHARETAB, "r");
    if (fp == NULL)
    {
	if (errno != ENOENT)
	    SetMessage(MainWindow, TXT_sharetabOpenError, Notice);
	return False;
    }

    while (getshare(fp, &entry) > 0)
    {
	if (sharecmp(sharep, entry))
	{
	    fclose(fp);
	    return True;
	}
    }
    fclose(fp);
    return False;
}


extern int
ShareIt(struct share *sharep, DmObjectPtr op)
{
    int		status;
    char	shareCommand[BUFSIZ], *dasho;

    dasho = (*sharep-> sh_opts) ? "-o" : "";
    sprintf(shareCommand, "%s %s %s %s %s", SHARECMD, dasho, 
	    sharep-> sh_opts, sharep-> sh_path, sharep-> sh_res); 

    DEBUG1("shareCommand=\"%s\"\n", shareCommand);

    status = system(shareCommand);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
	op-> fcp = nfsw-> shared_fcp;
	SetMessage(MainWindow, TXT_ShareSucceeded, Base);
    }
    else
    {
	/* FIX: parse status to report specific error */
	DEBUG2("Mount attempt returns %d, errno=%d\n", status, errno);
	SetMessage(MainWindow, TXT_ShareFailed, Notice);
    }
    return status;
}



extern int
UnShareIt(struct share *sharep, DmObjectPtr op)
{
    int 	status;
    char	unshareCommand[BUFSIZ];

    sprintf(unshareCommand, "%s %s", UNSHARECMD, sharep-> sh_path);
    DEBUG1("unshareCommand=\"%s\"\n", unshareCommand);

    status = system(unshareCommand);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
	op-> fcp = nfsw-> local_fcp;
	SetMessage(MainWindow, TXT_UnShareSucceeded, Base);
    }
    else
    {
	/* FIX: parse status to report specific error */
	DEBUG2("unshare attempt returns %d, errno=%d\n", status, errno);
	SetMessage(MainWindow, TXT_UnShareFailed, Notice);
    }
    return status;
}



extern void
UnAdvertiseCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern MenuGizmo ActionsMenu;
    dfstab	    *dfsp;
    struct share    *sharep;
    DmObjectPtr     op;
    ObjectListPtr   slp = nfsw-> localSelectList;

    DEBUG0("UnAdvertiseCB entry\n");

    RETURN_IF_NULL(slp, TXT_NothingToUnShare);

    for ( ; slp; slp = slp-> next)
    {	 
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Notice);
	    continue;
	}
	if ((dfsp = (dfstab *)op-> objectdata) == NULL ||
	    (sharep = dfsp-> sharep) == NULL || sharep-> sh_path == NULL)
	{
	    SetMessage(MainWindow, TXT_BadDfs, Notice);
	    continue;
	}

	if (!isShared(sharep))
	{
	    op-> fcp = nfsw-> local_fcp;
	    OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			      XtNsensitive, OwnerLocal, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			      XtNsensitive, False, NULL);
	    SetMessage(MainWindow, TXT_AlreadyUnShared, Base);
	    continue;
	}
	else			/* unshare it */
	{
	    if (UnShareIt(sharep, op) == 0)
	    {
		OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);
		OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
				  XtNsensitive, OwnerLocal, NULL);
		OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
				  XtNsensitive, False, NULL);
	    }
	}
    }
    return;
}



extern void
AdvertiseCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern MenuGizmo ActionsMenu;
    dfstab        * dfsp;
    struct share  * sharep;
    DmObjectPtr     op;
    ObjectListPtr   slp = nfsw-> localSelectList;

    DEBUG0("AdvertiseCB entry\n");

    RETURN_IF_NULL(slp, TXT_NothingToShare);

    for ( ; slp; slp = slp-> next)
    {	 
	if ((op = slp-> op) == NULL)
	{
	    SetMessage(MainWindow, TXT_BadOp, Notice);
	    continue;
	}
	if ((dfsp = (dfstab *)op-> objectdata) == NULL ||
	    (sharep = dfsp-> sharep) == NULL || sharep-> sh_path == NULL)
	{
	    SetMessage(MainWindow, TXT_BadDfs, Notice);
	    continue;
	}
	if (isShared(sharep))
	{
	    op-> fcp = nfsw-> shared_fcp;
	    OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
			      XtNsensitive, OwnerLocal, NULL);
	    SetMessage(MainWindow, TXT_AlreadyShared, Base);
	    continue;
	}
	else			/* share  it */
	    if (ShareIt(sharep, op) == SUCCESS)
	    {
		OlVaFlatSetValues(ActionsMenu.child, ActionsAdvertise,
				  XtNsensitive, False, NULL);
		OlVaFlatSetValues(ActionsMenu.child, ActionsUnadvertise,
				  XtNsensitive, OwnerLocal, NULL);
	    }

	OlFlatRefreshItem(nfsw-> iconbox, slp-> index, True);

    }
    return;
}

