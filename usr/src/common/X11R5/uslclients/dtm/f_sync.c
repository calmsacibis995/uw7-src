#pragma ident	"@(#)dtm:f_sync.c	1.42.1.2"

/******************************file*header********************************

    Description:
	This file contains the source code for the "sync" timer
*/
						/* #includes go here	*/
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

typedef struct _FileInfo {
    Boolean	found;
    DmObjectPtr	obj;
} FileInfo;

typedef _OlArrayStruct(FileInfo, _FileArray) FileArray;
typedef struct stalecontainer {
	struct stalecontainer	*next;
	DmContainerPtr 		cp;
} StaleList;

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static int	CmpNames(const void *, const void *);
static FileArray * GetFiles(DmContainerPtr container);
static void 	SyncContainer(DmContainerPtr cp);
static Boolean	SyncProc(XtPointer client_data);
static Boolean  getstalecp(DmContainerPtr *);
static void 	addstalecp(DmContainerPtr);
					/* public procedures		*/
void		Dm__RmFromStaleList(DmContainerPtr);
void		Dm__SyncContainer(DmContainerPtr cp, Boolean force);
void		Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id);

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CmpNames-
*/
static int
CmpNames(const void * f1, const void * f2)
{
    FileInfo * file1 = (FileInfo *)f1;
    FileInfo * file2 = (FileInfo *)f2;

    return(strcmp((const char *)file1->obj->name,
		  (const char *)file2->obj->name));

}					/* End of CmpNames */

/****************************procedure*header*****************************
    GetFiles- this is equivalent to DmOpenDir except that a "dummy" container
	is used.
*/
static FileArray *
GetFiles(DmContainerPtr container)
{
    static FileArray *	files;		/* Freed when dtm exits */
    DmObjectPtr		obj;
    FileInfo		new_file;

    /* Use Dm__ReadDir to get a list of objs */
    if (IS_NETWARE_PATH(container->path)) {
    	(void)Dm__ReadDir(container, 0);
	DmClassNetWareServers(container);
    } else {
    	(void)Dm__ReadDir(container, DM_B_SET_TYPE | DM_B_INIT_FILEINFO);
    }

    /* Initialize (re-use) files buffer.  Freed when dtm exits */
    if (files == NULL)
	_OlArrayAllocate(_FileArray, files,
			 container->num_objs, _OlArrayDefaultStep);
    else
	_OlArraySize(files) = 0;

    /* Now run thru all objs and create FileInfo element for each */
    new_file.found = False;
    for (obj = container->op; obj != NULL; obj = obj->next)
    {
	new_file.obj = obj;
	_OlArrayAppend(files, new_file);
    }

    return(files);
}					/* End of GetFiles */
/*****************************************************************************
 *  	SyncContainer: synchronize all views of a container
 *	INPUTS: container pointer
 *	OUTPUTS: none
 *	GLOBALS: none
 *****************************************************************************/
static void
SyncContainer(DmContainerPtr cp)
{
    FileArray *		files;
    FileInfo *		file;
    DmContainerRec	container;
    DmObjectPtr		obj, next_obj;

#define FINFO(obj)	( (DmFileInfoPtr)((obj)->objectdata) )

    _DPRINT1(stderr,"SyncContainer: %s\n", cp->path);

    /* Stamp container now (as early as possible) */
    Dm__StampContainer(cp);

    /* Get list of objs from files in folder (on disk) */
    container.next		= NULL;
    container.path		= cp->path;
    container.count		= 0;
    container.op		= NULL;
    container.num_objs		= 0;
    container.attrs		= 0;
    container.plist.ptr		= NULL;
    container.plist.count	= 0;
    container.data		= NULL;
    container.cb_list.used	= 0;
    container.cb_list.alloced	= 0;

    files = GetFiles(&container);

    /* Sort files by name */
    qsort(files->array, (size_t)_OlArraySize(files),
	  (size_t)_OlArrayElementSize(files), CmpNames);

    /* First, look for deletions.  If there's an object in the folder which
       is not in the file list, delete it.  Otherwise, mark the file in the
       list as found.
    */

    for (obj = cp->op; obj != NULL; obj = next_obj)
    {
	Boolean		found = False;
	DmItemPtr	item;
	int		result;

	next_obj = obj->next;		/* since obj may be freed below */

        for (file = files->array;
             file < files->array + _OlArraySize(files); file++)
        {
            if (file->found ||          /* already accounted for */
                ((result = strcmp(file->obj->name, obj->name)) < 0))
                continue;

            /* File found or file won't be found (too far down the list) */
            if (result == 0)
                file->found = found = True;

            break;
        }

        if (found)	/* If found, make sure item has latest "stats" */
	{
	    /* Found a NewBorn item, need to class it preserving its
	       property list
	     */
	    if (obj->attrs & DM_B_NEWBORN)
		file->found = False;

	    if ((FINFO(obj) != NULL) && (FINFO(file->obj) != NULL))
	    {
	    	if (FINFO(obj)->mtime < FINFO(file->obj)->mtime)
	    	{
		    /* File modification */
		    *FINFO(obj) = *FINFO(file->obj);
		    /* FLH MORE: we need to do something here to
		     * notify the views that the object should be retyped
		     * and redisplayed.  
		     */
	    	}

	    } else if ((FINFO(obj) == NULL) && (FINFO(file->obj) != NULL))
	    {
		/* Probably a "hidden" file has become valid.  Remove "old"
		   obj from container and change file->found so it will be
		   added below.	 Use DmDelObject instead of DmRmObject
		   because there is no need to update the visited
		   folders menu of folder map.
		*/
                DmDelObjectFromContainer(cp, obj);
		Dm__FreeObject(obj);
		file->found = False;
	    }
	} else		/* else, remove item or object */
	{
	    /* Delete, object is freed in  DmRmObjectFromContainer() */
	    DmRmObjectFromContainer(cp, obj);
	}
    }

    /* Now look for files in file array that were not found in the items list
       and add them to the folder.
    */

    for (file = files->array; file < files->array + _OlArraySize(files); file++)
	if (file->found)
	{
	    Dm__FreeObject(file->obj);
	} else
	{
	    /*	
	     * Addition.
	     * 	 Object has already been inited and typed. Just add it.
	     */
	    if (file->obj->attrs & DM_B_HIDDEN)
		(void)Dm__AddToObjList(cp, file->obj, NULL); 
	    else{
		obj = DmAddObjectToContainer(cp, file->obj, file->obj->name, 
					     NULL);
		if (obj != file->obj)
		    Dm__FreeObject(file->obj);
	    }
	}
}	/* end of SyncContainer */


/*
 * get container from the list of stale container.
 * 	Return True if no more container in the list. Otherwise,
 *	return False.
 */
static Boolean
getstalecp(DmContainerPtr *cp)
{
	StaleList *sp = Desktop->stale_containers;
	if (sp == NULL) {
		*cp = NULL;
		return(True);		/* Nothing to do */
	}
	Desktop->stale_containers = sp->next;
	*cp = sp->cp;
	free(sp);
	if (Desktop->stale_containers)
		return(False);
	return(True);
}


/*
 * Add the specified container to the list of stale containers 
 */
static void
addstalecp(DmContainerPtr cp)
{
	StaleList **sp = &Desktop->stale_containers;
	StaleList *nsp;
	if (!(nsp = CALLOC(1, sizeof(StaleList)))) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
		return;
	}
	nsp->cp = cp;
	while (*sp != NULL)
		sp = &(*sp)->next;
	*sp = nsp;
}
	

/****************************procedure*header*****************************
    SyncProc- called by Dm__SyncTimer to update a "stale" folder.  The sync
    timer builds a list of indexes to stale folders and calls registers this
    work proc.  This work proc updates the "current" stale folders and, if
    there are more stale folders to update, returns "False" so that we're
    called again for the next stale folder.  If there are no more stale
    folders to update, the sync timer is re-activated and this work proc is
    not re-registered (return True).
*/
static Boolean
SyncProc(XtPointer client_data)
{
    DmDesktopPtr	desktop = (DmDesktopPtr)client_data;
    DmContainerPtr 	cp; 
    Boolean		isempty;

    isempty  = getstalecp(&cp);
    if (cp)
    	SyncContainer(cp);

    if (isempty)
	Dm__AddSyncTimer(desktop);

    /* Return True or false */
    return(isempty);
}				/* End of SyncProc */

/***************************public*procedures****************************

    Public Procedures
*/


/****************************procedure*header*****************************
    Dm__RmFromStaleList-
*/
void
Dm__RmFromStaleList(DmContainerPtr cp)
{
	StaleList *sp = Desktop->stale_containers;
	StaleList *tsp;
	if (sp == NULL)
		return;		/* Nothing to remove */
	while (sp) {
		if (sp->cp == cp) {
			if (sp ==  Desktop->stale_containers)
				Desktop->stale_containers = sp->next;
			else
				tsp->next = sp->next;
			free(sp);
			break;
		}
		tsp = sp;
		sp = sp->next;
	}
}					/* end of Dm__RmFromStaleList */


/****************************procedure*header*****************************
 *  Dm__SyncContainer- Public interface to request update of "stale" 
 *	container pointed to by 'cp'.  SyncContainer does the real work.
 *
 *   INPUT: 	container
 *	        flag indicating whether we should force the sync
 *		(if force is True, only sync if container is really stale)
 *   OUTPUT:	none
 */
void
Dm__SyncContainer(DmContainerPtr cp, Boolean force)
{
    struct stat		buf;


    if (stat((const char *)cp->path, &buf) != 0)
    {
	Dm__VaPrintMsg(TXT_STAT, errno, cp->path);
	return;
    }
	
    if (force || cp->time_stamp != buf.st_mtime)
	SyncContainer(cp);

}	/* end of Dm__SyncContainer */

/****************************procedure*header*****************************
    Dm__SyncTimer- this timer is called to keep "Desktop" in sync with
	changes on disk which occur outside of the desktop.

	This timer merely builds a list of open folder which need updating.
	If any "stale" folders are found, it registers a "background" Xt work
	proc to do the actual updates.  If none are found, the timer resets
	itself to try again later.

	The timer interval is user-settable.

	If there is a "file-op" work proc registered (ie., there is Desktop
	file activity in progress), we return right away (after resetting the
	timer).
*/
void
Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id)
{
    DmDesktopPtr	desktop = (DmDesktopPtr)client_data;
    DmContainerBuf	containers;
    DmContainerPtr	cp;
    StaleList *		stale_containers;
    Boolean		found;
    struct stat		buf;
    int 		i;
 


    /* Return now if no containers (exiting) */
    containers = DESKTOP_CONTAINERS(desktop);
    if (containers.used == 0)
	return;
    found			= False;

     /*  We want the list to stay stable while we traverse it. */
    DmLockContainerList(True);

#ifdef FDEBUG
    if (Debug > 3){
	fprintf(stderr,"SyncTimer: open containers:\n");
	for (i=0; i<containers.used; i++){
	    fprintf(stderr,"\t%s\n", containers.p[i]->path);
	}
    }
#endif
    for (i = 0; i < containers.used ; i++)
    {
	/*
	 * FLH_MORE: folder callback will need to keep track of
	 * ongoing folder operations.  See f_create.c: UpdateFolderCB
	if (folder->attrs & DM_B_FILE_OP_BUSY)
	    continue;		* busy with file operation *

	 */

	cp = containers.p[i];
	if (!SYNC_REMOTE_FOLDERS(Desktop) && IS_REMOTE(Desktop, cp->fstype))
	    /* folder is remote, skip it */
	    continue;

	if (!cp->cb_list.used) {
		/* skip containers with no views */
		continue;
	}

	if (stat((const char *)cp->path, &buf) != 0)
	{
	    Dm__VaPrintMsg(TXT_STAT, errno, cp->path);
	    /* Notify dependents that container has been destroyed */
	    DmDestroyContainer(cp);
	    Dm__UpdateVisitedFolders(cp->path, NULL);
	    Dm__UpdateTreeView(cp->path, NULL);

	} else if (cp->time_stamp != buf.st_mtime)
	{
	    addstalecp(cp);
	    found = True;
	}
    }

	
    /* If there are stale containers, register work proc to update them.
       Otherwise, re-activate this timer to check again later.
    */
    if (found)
	XtAddWorkProc(SyncProc, (XtPointer)desktop);

    else
	Dm__AddSyncTimer(desktop);

    /* Now clean up container list and close containers */
    DmLockContainerList(False);

} /* End of Dm__SyncTimer */

