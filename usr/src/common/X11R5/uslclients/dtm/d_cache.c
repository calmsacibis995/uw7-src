#pragma ident	"@(#)dtm:d_cache.c	1.37.1.18"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <X11/Intrinsic.h>
#include <memutil.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************
 *
 *  Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures 
 *************************forward*declarations***************************/

/*  
 * private procedures  
 */
static void DmInitFinfoContainer(DmContainerRec *, DtAttrs);

/*  
 * public procedures  
 */
DmObjectPtr	DmAddObjectToContainer(DmContainerPtr cp, DmObjectPtr obj,
				       char *name, DtAttrs options);
int		DmCloseDir(char *path, DtAttrs options);
int		DmCloseContainer(DmContainerPtr cp, DtAttrs options);
DmObjectPtr	Dm__CreateObj(DmContainerPtr cp, char * name, DtAttrs options);
void		DmDelObjectFromContainer(DmContainerPtr cp, 
					 DmObjectPtr target_op);
#ifdef NOT_USED
DmObjectPtr	DmDupObject(DmObjectPtr op);
#endif
int		DmFlushContainer(DmContainerPtr cp);
int		DmFlushDir(char *path);
void		Dm__FreeContainer(DmContainerPtr cp);
void		Dm__FreeObject(DmObjectPtr op);
DmObjectPtr	DmGetObjectInContainer(DmContainerPtr cp, char *name);
void 		DmInitObj(DmObjectPtr obj, struct stat *caller_buff, 
			  DtAttrs options);
void		DmMoveContainer(DmContainerPtr cp, char *new_name);
DmContainerPtr 	Dm__NewContainer(char *path);
DmObjectPtr	Dm__NewObject(DmContainerPtr cp, char * name);
DmContainerPtr	DmOpenDir(char * path, DtAttrs options);
DmContainerPtr	DmQueryContainer(char *path);
int		Dm__ReadDir(DmContainerPtr cp, DtAttrs options);
void		DmRenameObject(DmObjectPtr obj, char *new_name);


extern void 	Dm__SetDfltFileClass(DmObjectPtr op, struct stat *buf, 
				     struct stat *lbuf);
void		DmSetFileClass(DmObjectPtr op);
void		Dm__StampContainer(DmContainerRec * container);


/*****************************************************************************
 *  	DmAddObjectToContainer: common routine for adding object to container
 *				*create an object if needed
 *				*attach object to the container
 *				*initialize object as specified by options mask
 *				*initialize the object type
 *				*notify all dependents of the newly added object
 *	INPUTS:	container ptr
 *		object Ptr (should point to an object for the associated
 *			    name or be NULL)
 *		object name
 *		options: DM_B_INIT_FILEINFO	Dtm.h: do a stat
 *			 DM_B_SET_TYPE		Dtm.h: determine file class
 *			 DM_B_ADD_TO_END	DtI.h: place object at end
 *			 DM_B_SET_CLASS_PROP	   set the _CLASS property
 *									   obj is used to pass in the value
 *		
 *	OUTPUTS: pointer to object in container or NULL
 *		 NOTE: if the object was already in the container,
 *		       the existing object is returned.  (caller's
 *		       object is ignored and should probably be freed).
 *	GLOBALS: Desktop (for DESKTOP_SHELL)
 *****************************************************************************/
DmObjectPtr	
DmAddObjectToContainer(DmContainerPtr cp, DmObjectPtr obj, char *name, 
DtAttrs options)
{
    DmObjectPtr op;
    DmContainerCallDataRec call_data;

    _DPRINT3(stderr,"AddObjectToContainer: adding %s to %s\n", name, cp->path);
    
    if (name && (op = DmGetObjectInContainer(cp, name))){
		if (op->attrs & DM_B_NEWBORN) {
			/*
			 * This is a new object created by the file op code to store
			 * instance properties. So skip the init code, but make sure
			 * you do the callback at the bottom of this function.
			 */
			op->attrs &= ~DM_B_NEWBORN;

			if (options & DM_B_SET_CLASS_PROP)
				DtSetProperty(&(op->plist), OBJCLASS, (char *)obj, 0);

			DmSetFileClass(op);

			/* free obj??? */
			goto skip_init;
		}

	/* caller should free object passed in */
	return(op);
    }

	if (!(options & DM_B_SET_CLASS_PROP))
    	op = obj;
    if (op == ((DmObjectPtr)NULL)){
	if ( (op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec))) == NULL)
	    return(NULL);
    }

	/*  Set the name if it's not already set.  If it is set,
	 *  assume that the name has been made current by caller.
	 *	FLH MORE: clean this up.  What about reuse of old objects?
	 */
    if (op->name == NULL){
	op->name = STRDUP(name);
    }
    
    if (Dm__AddToObjList(cp, op, options & DM_B_ADD_TO_END) == ExmNO_ITEM) {
	if (obj == NULL) {
		FREE(op->name);
		FREE(op);
	}
	return(NULL);
    }

	/* set the _CLASS property before typing the object below */
	if (options & DM_B_SET_CLASS_PROP)
		DtSetProperty(&(op->plist), OBJCLASS, (char *)obj, 0);

    /* 	
     * Initialize the object, if necessary.  The object must
     *  be initialized before we notify dependents; the dependents
     *  will want to create view items.  Init the object type
     *  here too, for efficiency.
     *  FLH MORE: we should be able to type the file 
     *  without initing the file info again
     */
    if (options & (DM_B_INIT_FILEINFO | DM_B_SET_TYPE)) {
	DmInitObj(op, NULL, options & (DM_B_INIT_FILEINFO | DM_B_SET_TYPE));
    }

skip_init:
    if(!(op->attrs & DM_B_HIDDEN))
    	DmInitObjType(DESKTOP_SHELL(Desktop), op);


    /*
     * Notify dependents of change in container 
     */
    call_data.reason = DM_CONTAINER_CONTENTS_CHANGED;
    call_data.cp = cp;
    call_data.detail.changed.num_added = 1;
    call_data.detail.changed.num_removed = 0;
    call_data.detail.changed.objs_added = &op;
    call_data.detail.changed.objs_removed = NULL;
    DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);

    /* 
     * Notify the folder map
     * FLH MORE: break apart like (Dm)AddObjToFolder
     * Identify cases where we know we don't have to check.
     * DmUpdateWindow used to call AddObjToFolder when
     * it knew the Folder Map did not need to be updated.
     */
    if (OBJ_IS_DIR(op))
    {
	char buf[PATH_MAX];
	Dm__UpdateTreeView(NULL, Dm_ObjPath(op, buf));
    }
    return(op);
} 	/* end of DmAddObjectToContainer */


/****************************procedure*header*****************************
    Dm__StampContainer-
*/
void
Dm__StampContainer(DmContainerRec * container)
{
    struct stat buf;

    container->time_stamp =
	(stat((const char *)container->path, &buf) == 0) ? buf.st_mtime : 0;
}


/*
 * DmInitFinfoContainer-
 *	Initialize time stamp as well as maximum length of 
 *	a path component.
 */
static void
DmInitFinfoContainer(DmContainerRec * container, DtAttrs options)
{
	struct stat buf;

	if (stat(container->path, &buf) == 0) { 
		if (options & DM_B_TIME_STAMP)
			container->time_stamp = buf.st_mtime;
		container->fstype = convnmtonum(buf.st_fstype);
		if (Desktop->fstbl[container->fstype].f_maxnm == -1) {
			struct statvfs v;
			statvfs(container->path, &v);
			Desktop->fstbl[container->fstype].f_maxnm = v.f_namemax;
		}
	} else if (options & DM_B_TIME_STAMP)
		container->time_stamp = 0;
}


void
DmSetFileClass(DmObjectPtr op)
{
    char *p;
    struct stat buf;
    struct stat lbuf;
    
    p = DmObjPath(op);
    if (lstat(p, &lbuf) != -1) { 
	/* Set default file class based on stat(2) info */
	if ((lbuf.st_mode & S_IFMT) == S_IFLNK) {
	    if (stat(p, &buf) == -1)  
		Dm__SetDfltFileClass(op, NULL, &lbuf);
	    else {
		Dm__SetDfltFileClass(op, &buf, &lbuf);
		/* 
		 * Set file class based on user's file 
		 * class database 
		 */
		Dm__SetFileClass(op);
	    }
	} else {
	    Dm__SetDfltFileClass(op, &lbuf, NULL);
	    /* 
	     * Set file class based on user's file 
	     * class database 
	     */
	    Dm__SetFileClass(op);
	}
    } else
	Dm__SetDfltFileClass(op, NULL, NULL);

}	/* end of DmSetFileClass */


DmContainerPtr
Dm__NewContainer(char *path)
{
	DmContainerPtr cp;

	if (!path) 
		return(NULL);

	if ((cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
	}else{
	    cp->path = strdup(path);
	    cp->count = 1;
	}
	return(cp);
}	/* end of Dm__NewContainer */


/*
 * Free container structure.
 * Note: the object list is not freed!
 */
void
Dm__FreeContainer(DmContainerPtr cp)
{
	if (cp->path)
		free(cp->path);

	if (cp->plist.count)
		DtFreePropertyList(&(cp->plist));

	if (cp->data)
		free(cp->data);

	free(cp);
}

void
Dm__FreeObject(DmObjectPtr op)
{

    _DPRINT3(stderr, "Dm__FreeObject: %s\n",  op->name);

	XtFree(op->name);
	XtFree(op->objectdata);
	if (op->fcp && (op->fcp->attrs & DM_B_FREE)) {
		DmReleasePixmap(DESKTOP_SCREEN(Desktop), op->fcp->glyph);
		free(op->fcp);
    	}

	DtFreePropertyList(&(op->plist));
	free(op);
} /* end of Dm__FreeObject */

/*****************************************************************************
 *  	DmMoveContainer: move a container and notify all dependants of
 *			the change.  Dependants of descendent directories
 *			will not be notified unless they are explicitly
 *			registered on the callback list of this container.
 *			See FolderPathChanged for notification of descendants.
 * 	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void		
DmMoveContainer(DmContainerPtr cp, char *new_name)
{
    DmContainerCallDataRec call_data;
    char *old_path = cp->path;
    char *new_path = strdup(new_name);

    _DPRINT1(stderr, "DmMoveContainer: %s --> %s\n", old_path, new_path);

    /* Update the path in the container cache */
    DtDelData(NULL, DM_CACHE_FOLDER, old_path, strlen(old_path) + 1);
    cp->path = new_path;
    DtPutData(NULL, DM_CACHE_FOLDER, new_path, strlen(new_path), cp);

    /* 
     * FLH MORE: is it ok to update the container before notifying
     * the dependents?  Can order of notification be significant?
     */
    call_data.reason = DM_CONTAINER_MOVED;
    call_data.cp = cp;
    call_data.detail.moved.old_path = old_path;
    call_data.detail.moved.new_path = new_path;
    DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);

    /* free old path */
    FREE(old_path);    
} /* end of DmMoveContainer */

/****************************procedure*header*****************************
    Dm__NewObject-
*/
DmObjectPtr
Dm__NewObject(DmContainerPtr cp, char * name)
{
    DmObjectPtr op;
    
    if ( (op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec))) != NULL){
	if (name)
	    op->name = strdup(name);
    
	/* ADD_TO_END on behalf of DmOpenDir... sorry */
	if (Dm__AddToObjList(cp, op, DM_B_ADD_TO_END) == ExmNO_ITEM)
		return(NULL);
    }

    return(op);
}					/* end of Dm__NewObject */


/*
 * convnmtonum()
 *	This function convert filesytem type name to number.
 */
int
convnmtonum(char *fstypep)
{
	int i;

	for (i = 0; i <= NUMFSTYPES(Desktop); i++) 
		if (strcmp(Desktop->fstbl[i].f_fstype, fstypep) == 0)
			return i+1; 
	return 0;
}

 
/****************************procedure*header*****************************
    DmInitObj- for an object, initialize the file info and type it.
	If caller has stat_buf, use it; otherwise, stat it here.

	FLH MORE: do we need to free old file info in some cases?
*/
void
DmInitObj(DmObjectPtr obj, struct stat *stat_bufp, DtAttrs options)
{
    char 		*path = DmObjPath(obj);
    struct stat		stat_buf;
    struct stat		lstat_buf;
    struct stat		*lstat_bufp = NULL;


    /* 
     * Stat the file if the caller has not.  Since each file is classified
     * BEFORE the ".dtinfo" file is read, no instance properties should
     * affect the file class that a file is in.
     *
     * If stat returns an error consider it a "hidden" file.  May be
     * link which points to file that doesn't exist.
     */
    if (stat_bufp == NULL) {
        if (lstat(path, &lstat_buf)) {  
	    obj->attrs |= DM_B_HIDDEN;
	    goto out;
        }
	lstat_bufp = &lstat_buf;
	if ((lstat_bufp->st_mode & S_IFMT) == S_IFLNK) {
    		if (options & DM_B_DIRECTORY_ONLY) {
			obj->attrs |= DM_B_HIDDEN;
			return;
		}
		if (stat(path, &stat_buf)) {
			 obj->attrs |= DM_B_HIDDEN;
		    	 goto out;
		} 
		obj->attrs |= DM_B_SYMLINK;
		stat_bufp = &stat_buf;
	} else
		stat_bufp = &lstat_buf;
    } else if (!(options & DM_B_DIRECTORY_ONLY)) {
		/* Should not fail */
        	if (lstat(path, &lstat_buf)) {  
			lstat_bufp = NULL;
        	} else
			lstat_bufp = &lstat_buf;
   }


    if (options & DM_B_DIRECTORY_ONLY) {
	 if ((stat_bufp->st_mode & S_IFMT) != S_IFDIR) {
		obj->attrs |= DM_B_HIDDEN;
		return;
	}
    }
	
    if (options & DM_B_INIT_FILEINFO) {
	DmFileInfoPtr f_info = (DmFileInfoPtr)MALLOC(sizeof(DmFileInfo));

	if (f_info != NULL) {
	    f_info->mode	= stat_bufp->st_mode;
	    f_info->nlink	= stat_bufp->st_nlink;
	    f_info->uid		= stat_bufp->st_uid;
	    f_info->gid		= stat_bufp->st_gid;
	    f_info->size	= stat_bufp->st_size;
	    f_info->mtime	= stat_bufp->st_mtime;
	    f_info->fstype	= convnmtonum(stat_bufp->st_fstype);
	    obj->objectdata	= (void *)f_info;
	}
    }


out:
    if (options & DM_B_SET_TYPE) {
	/* set default file class based on stat(2) info */
	Dm__SetDfltFileClass(obj, stat_bufp, lstat_bufp);

	/* Set file class based on user's file class database */
	Dm__SetFileClass(obj);
    }
    if (IS_DOT_DOT_FILE(obj->name))
        obj->attrs |= DM_B_HIDDEN;
}					/* end of DmInitObj */


/****************************procedure*header*****************************
/*****************************************************************************
 *      Dm__CreateObj- allocate an object
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *
 *****************************************************************************/
DmObjectPtr
Dm__CreateObj(DmContainerPtr cp, char * name, DtAttrs options)
{
    DmObjectPtr obj;

    if (( (obj = Dm__NewObject(cp, name)) != NULL ) &&
	(options & (DM_B_INIT_FILEINFO | DM_B_SET_TYPE))) {
	DmInitObj(obj, NULL, options);
    }

    return (obj);
}					/* end of Dm__CreateObj */

/****************************procedure*header*****************************
    Dm__ReadDir-
*/
int
Dm__ReadDir(DmContainerPtr cp, DtAttrs options)
{
    DIR *		dirptr;
    struct dirent *	direntptr;
    Boolean		dtinfo_found = False;
    int			status = 0;		/* success */
	DmObjectPtr op;

    errno = 0;

    if ((dirptr = opendir(cp->path)) == NULL ) {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, cp->path);
	return(-1);
    }
	
    if (options & DM_B_SET_TYPE)
	cp->attrs |= DM_B_INITED;

    while ((direntptr = readdir(dirptr)) != NULL ) {
	char * name = direntptr->d_name;

	/* Check for "dot" files: if ".dtinfo" file found, remember this and
	 *  continue.  Never show "." or "..".  Show other dot files 
	 *  conditionally.
	 */
	if (name[0] == '.') {
	    if (!strcmp(name, DM_INFO_FILENAME)) {
		dtinfo_found = True;
		continue;
	    }

	    if (IS_DOT_DOT(name) || !SHOW_HIDDEN_FILES(Desktop)){
		if (!(IS_DOT_DOT_FILE(name)))
		    continue;
	    }
	}

	/*
	 * Don't call Dm__CreateObj() here, because that will type the object.
	 * In order to implement the _CLASS property, objects must be typed
	 * after the dtinfo file is read in.
	 */
		if (Dm__NewObject(cp, name) == NULL) {
			status = -1;
			break;
		}
    }

    /*
     * Must ignore return code from closedir(), because some file system,
     * like /dev/fd, may not support the close() system call, even though
     * everything else works.
     */
    (void)closedir(dirptr);

    if ((status == -1) || !(options & DM_B_READ_DTINFO) || !dtinfo_found ||
	(DmReadDtInfo(cp, DmMakePath(cp->path, DM_INFO_FILENAME),
		      INTERSECT) == -1))
	cp->attrs |= DM_B_NO_INFO;

	if (options & (DM_B_INIT_FILEINFO | DM_B_SET_TYPE))
		for (op = cp->op; op; op=op->next)
			DmInitObj(op, NULL, options);

    return(status);
}				/* end of Dm__ReadDir */

DmContainerPtr
DmOpenDir(char * path, DtAttrs options)
{
    DmContainerPtr	cp = NULL;
    char *		real_path = realpath(path, Dm__buffer);
    int			len;

    errno = 0;
    if (real_path == NULL) {
	_DPRINT1(stderr,
		"realpath(3C) error on '%s' (offending file=%s, errno=%d)\n",
		path, Dm__buffer, errno);
	return(NULL);
    }

    len = strlen(real_path) + 1;

    if (!(options & DM_B_PRIVATE_COPY) && (cp = DtGetData(NULL, 
	DM_CACHE_FOLDER, (void *)real_path, len)) != NULL) {

	if (!(cp->attrs & DM_B_INITED) && (options & DM_B_SET_TYPE)) {
	    DmObjectPtr op;

	    for (op = cp->op; op; op = op->next)
		DmSetFileClass(op);
	}

	cp->count++;		/* bump usage count */

	/* Do a quick check to see if search permission on path is allowed. */
    } else if (!access(real_path,  R_OK|F_OK) &&
	       ((cp = Dm__NewContainer(real_path)) != NULL)) {
	DmInitFinfoContainer(cp, options);
	/* There is no .dtinfo for MS-DOS file system. */
	if (cp->fstype != DOSFS_ID(Desktop))
		options |= DM_B_READ_DTINFO;
	else {
		options &= ~DM_B_READ_DTINFO;
		cp->attrs |= DM_B_NO_LINK;
	}
	Dm__ReadDir(cp, options);
	if (!(options & DM_B_PRIVATE_COPY))
		DtPutData(NULL, DM_CACHE_FOLDER, cp->path, len, cp);


    }

    if (cp) {
    	_DPRINT1(stderr, "DmOpenDir: adding %s to container list\n", cp->path);
    	DmAddContainer(&DESKTOP_CONTAINERS(Desktop), cp);
    }

    return(cp);
}				/* End of DmOpenDir */

int
DmCloseDir(char *path, DtAttrs options)
{
	DmContainerPtr cp;

	if (cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)path, strlen(path)+1))
		return(DmCloseContainer(cp, options));
	return(1);
}

/*****************************************************************************
 *  	DmCloseContainer: Remove a reference to a container.
 *		decrement reference count
 *		If refrence count becomes 0
 *			*remove container from the Desktop container list
 *			*remove container form the Desktop stale list
 *			*remove container from cache
 *			*(optionally) FLUSH .dtinfo to disk
 *			*free the container and its objects
 *			
 *	INPUTS: container
 *		options:
 * 			DM_B_NO_FLUSH: do not flush .dtinfo file
 *	OUTPUTS: 0 (always)
 *	GLOBALS: Desktop struct
 *****************************************************************************/
int
DmCloseContainer(DmContainerPtr cp, DtAttrs options)
{

    _DPRINT1(stderr,"DmCloseContainer: %s\n", cp->path);

	if (!(options & DM_B_NO_FLUSH) && (cp->fstype != DOSFS_ID(Desktop)))  
		/* remember the fact that someone want to flush the data */
		cp->attrs |= DM_B_FLUSH_DATA;

	if (--(cp->count) == 0) {
		register DmObjectPtr op;
		DmObjectPtr save;

		_DPRINT1(stderr,
			"DmCloseContainer: removing %s from container list\n",
			cp->path);

		DmRemoveContainer(&DESKTOP_CONTAINERS(Desktop), cp);
		Dm__RmFromStaleList(cp);

		if (cp->path && !(options & DM_B_PRIVATE_COPY))

			DtDelData(NULL, DM_CACHE_FOLDER, (void *)(cp->path),
				strlen(cp->path)+1);
		if (cp->attrs & DM_B_FLUSH_DATA)
			DmFlushContainer(cp);
		for (op=cp->op; op;) {
			save = op->next;
			Dm__FreeObject(op);
			op = save;
		}
		Dm__FreeContainer(cp);
	}
	return(0);
}

/*****************************************************************************
 *  	DmDelObjectFromContainer: remove an object from a container
 *				  and notify all dependents of change
 *	INPUTS:	container pointer
 *		object to be removed
 *	OUTPUTS: none
 *	GLOBALS: none
 *****************************************************************************/

void
DmDelObjectFromContainer(DmContainerPtr cp, DmObjectPtr target_op)
{
        register DmObjectPtr op = cp->op;
	DmContainerCallDataRec call_data;


	_DPRINT3(stderr,"DmDelObjectFromContainer: removing %s from %s\n",
		target_op->name, cp->path);


	if (op == target_op) {
		cp->op = op->next;
		cp->num_objs--;
	}
	else
	    for (; op->next; op=op->next)
		if (op->next == target_op) {
				op->next = target_op->next;
				cp->num_objs--;
				break;
			}
	/*
	 * Notify dependents of change in container
	 */
	call_data.reason = DM_CONTAINER_CONTENTS_CHANGED;
	call_data.cp = cp;
	call_data.detail.changed.num_added = 0;
	call_data.detail.changed.num_removed = 1;
	call_data.detail.changed.objs_removed = NULL;
	call_data.detail.changed.objs_removed = &target_op;
	DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);
}	/* end of DmDelObjectFromContainer */


/*****************************************************************************
 *  	DmDestroyContainer: Remove all references to a container.
 *		Notify all dependents of container destruction
 *		Notification is advisory only, dependents may choose to 
 *		keep the container around (to clean up file operations)
 *		Dependents should respond by closing container when possible
 *		WARNING: Container may be destroyed if all dependents close it
 *
 *		FLH MORE: should we return an indicator of whether
 *		there are still dependents?
 *	INPUTS:	container
 *	OUTPUTS: none
 *	GLOBALS:
 *****************************************************************************/
void
DmDestroyContainer(DmContainerPtr cp)
{
        
    DmContainerCallDataRec call_data;


    _DPRINT1(stderr,"DmDestroyContainer %s\n", cp->path);
    
    call_data.cp = cp;
    call_data.reason = DM_CONTAINER_DESTROYED;

    DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);
    /* FLH MORE: should we also update he folder map and
       visited folders here?  Right now, DmFolderPathChanged does that.
     */
}	/* end of DmDestroyContainer */

/*****************************************************************************
 *  	DmFlushContainer: write .dtinfo file for a container
 *	INPUTS: container pointer
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
int
DmFlushContainer(DmContainerPtr cp)
{

       static char *wb_dtinfo = NULL;

	if (DESKTOP_HELP_DESK(Desktop)	/* make sure it's not NULL. */
	    && (cp == DESKTOP_HELP_DESK(Desktop)->views[0].cp))
		DmWriteDtInfo(cp, DESKTOP_HELP_DESK(Desktop)->views[0].cp->path, 0);
	else if (DESKTOP_WB_WIN(Desktop) 	/* make sure it's not NULL. */
		&& cp == DESKTOP_WB_WIN(Desktop)->views[0].cp) {
		if (wb_dtinfo == NULL) {
			char buf[PATH_MAX];

			sprintf(buf, "%s/.Wastebasket",
				(char *)DmDTProp(WBDIR, NULL));
			wb_dtinfo = strdup(buf);
		}
		DmWriteDtInfo(cp, wb_dtinfo, 0);
	} else
		DmWriteDtInfo(cp, DmMakePath(cp->path, DM_INFO_FILENAME), 0);
	return(0);
} /* end of DmFlushContainer */

int
DmFlushDir(char *path)
{
	DmContainerPtr cp;

	if (cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)path, strlen(path)+1))
		return(DmFlushContainer(cp));
	return(1);
} /* end of DmFlushDir */


/*****************************************************************************
 *  	DmRenameObject: rename an object and notify all dependents of change
 *		NOTE: we only notify explicitly registered dependents.  
 *		Other dependents must be handled elsewhere (see 
 *		DmMoveContainer in this file and f_update.c:FolderPathChanged.
 *	INPUTS:	object pointer
 *		new name
 *	OUTPUTS: none
 *	GLOBALS: 
 *****************************************************************************/
void
DmRenameObject(DmObjectPtr obj, char *new_name)
{
    DmContainerPtr cp = obj->container;
    DmContainerCallDataRec call_data;
    char *old_name;

    _DPRINT3(stderr,"DmRenameObject: moving %s%s to %s%s\n",
	    obj->container->path, obj->name, obj->container->path, new_name);

    old_name = obj->name;
    obj->name = strdup(new_name);

    DmRetypeObj(obj, False);

    /*
     * Notify dependents of change in container 
     */
    call_data.reason = DM_CONTAINER_OBJ_CHANGED;
    call_data.cp = cp;
    call_data.detail.obj_changed.attrs = DM_B_OBJ_NAME;
    call_data.detail.obj_changed.obj = obj;
    call_data.detail.obj_changed.old_name = old_name;
    call_data.detail.obj_changed.new_name = obj->name;
    DtCallCallbacks(&cp->cb_list, (XtPointer) &call_data);

    FREE(old_name);

}	/* end of DmRenameObject */

DmObjectPtr
DmGetObjectInContainer(DmContainerPtr cp, char *name)
{
	register DmObjectPtr op;

	for (op=cp->op; op; op=op->next)
		if (!strcmp(op->name, name))
			return(op);
	return(NULL);
} /* end of DmGetObjectInContainer */

#ifdef NOT_USED
DmObjectPtr
DmDupObject(DmObjectPtr op)
{
	DmObjectPtr new_op;

	if (new_op = (DmObjectPtr)MALLOC(sizeof(DmObjectRec))) {
		*new_op = *op;
		new_op->next		= NULL;
		new_op->objectdata	= NULL; /* not copied */
		if (op->name)
			new_op->name = strdup(op->name);
		(void)DtCopyPropertyList(&(new_op->plist), &(op->plist));
	}

	return(new_op);
}	/* end of DmDupObject */
#endif

/*****************************************************************************
 *  	DmQueryContainer: check if we have a container for a path
 *	INPUTS: 	path name
 *	OUTPUTS:	container pointer (or NULL)
 *	GLOBALS:	
 *****************************************************************************/
DmContainerPtr
DmQueryContainer(char *path)
{
    DmContainerPtr cp;
    char *real_path = realpath(path, Dm__buffer);
    int path_len;


    _DPRINT3(stderr,"DmQueryContainer: %s\n", path ? path : "NULL");

    if (!real_path)
	return(NULL);

    path_len = strlen(real_path) +1;
    cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)real_path, path_len);
    return(cp);
}	/* end of DmQueryContainer */

