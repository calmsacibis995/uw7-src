#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/getfolder.c	1.14"
#endif

/*
 * Module:	dtadmin: nfs  Graphical Administration of Network File Sharing
 * File:	getfolder.c:  add folders to icon box.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/vfstab.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <DtI.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>

#include "nfs.h"
#include "sharetab.h"
#include "local.h"
#include "text.h"

extern Boolean		getline();

static void FreeObject();

extern NFSWindow *MainWindow;
extern NFSWindow *nfsw;
extern struct vfstab *Copy_vfstab();
extern void           Free_vfstab();

void
AddObjectToContainer(op)
DmObjectPtr op;
{
	/* add it to the end of list */
	if (nfsw->cp->op == NULL)
		nfsw->cp->op = op;
	else
	{
		DmObjectPtr endp;

		for (endp=nfsw->cp->op; endp->next; endp=endp->next)
		    ;
		endp->next = op;
	}
	nfsw->cp->num_objs++;
}


void
DelObjectFromContainer(DmObjectPtr target_op, DataType dataType)
{
    register DmObjectPtr op = nfsw->cp->op;

    if (op == target_op)
    {
	nfsw->cp->op = op->next;
	nfsw->cp->num_objs--;
    }
    else
	for (; op->next; op=op->next)
	    if (op->next == target_op)
	    {
		op->next = target_op->next;
		nfsw->cp->num_objs--;
		break;
	    }
    FreeObject(target_op, dataType);
}


DmFclassPtr
new_fileclass(filename)
char * filename;
{
    DmFclassPtr fcp;

    if ((fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec))) == NULL)
	return(NULL);
    fcp->glyph = DmGetPixmap(XtScreen(root), filename);
    return(fcp);
}



extern DmObjectPtr
new_object(name, ix, iy, vp, dataType)
char *name;
int ix, iy;
XtPointer  vp;
DataType dataType;
{
    DmObjectPtr   op, objp;
    dfstab       *dp;
    struct share *sp;

/* FIX: is this right??  if so, add dataType check before uncommenting
   /* check for duplicate names
 
    if (nfsw->cp->op != NULL) 
    {
	for (objp=nfsw->cp->op; objp; objp=objp->next)
	    if (!strcmp(objp->name, name))
	    {
		Free_vfstab(objp->objectdata);
		objp->objectdata = (void *)Copy_vfstab(vp);
		return((DmObjectPtr)OL_NO_ITEM);
	    }
    }
*/

    if (!(op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec))))
	return(NULL);

    op->container = nfsw->cp;
    DEBUG1("nfsw-> cp = %x\n", nfsw-> cp);

    if (name)
	op->name = STRDUP(name);

    switch (dataType)
    {
    case nfsRemote:
	op->objectdata = (void *)vp;
	DEBUG2("objectdata: mountp=%s, label=%s\n",
	       ((struct vfstab *)op->objectdata)->vfs_mountp,
	       ((struct vfstab *)op->objectdata)->vfs_fsckpass);
	break;
    case nfsLocal:
	op-> objectdata = (void *)vp;

	dp = (dfstab *) op-> objectdata;
	sp = dp-> sharep;
	DEBUG2("==path=%s label=%s\n", sp->sh_path, sp->sh_res);
	break;
    default:
	DEBUG1("new_object called with bad dataType (%d)\n", dataType);
	break;
    }
    return(op);
}

void
new_container(path)
char *path;
{
    if ((nfsw->cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL)
	NO_MEMORY_EXIT();

    nfsw->cp->path = STRDUP(path);
    nfsw->cp->count = 1;
    nfsw->cp->num_objs = 0;
}



static void
FreeObject(DmObjectPtr op, DataType dataType)
{
    RETURN_IF_NULL(op, TXT_BadOp);

    if (op->name)
	FREE(op->name);

    switch (dataType)
    {
    case nfsLocal:
	free_dfstab((dfstab *)op-> objectdata);
	break;
    case nfsRemote:
	Free_vfstab((struct vfstab *)op-> objectdata);
	break;
    default:
	DEBUG1("FreeObject called with bad dataType (%d)\n", dataType);
	return;
    }
    FREE((char *)op);
    return;
}

Boolean
getline(buf, len, fd)
char *buf;
int len;
FILE *fd;
{
	while (fgets(buf, len, fd) != NULL) {
		if (buf[0] == ' ' || buf[0] == '\t' ||  buf[0] == '\n'
		    || buf[0] == '\0' || buf[0] == '#')
			continue;
		return(True);
	}
	return(False);
} /* getline */
