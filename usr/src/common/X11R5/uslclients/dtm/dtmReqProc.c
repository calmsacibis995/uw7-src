/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:dtmReqProc.c	1.39.2.2" */

/******************************file*header********************************

    Description:
	This file contains the source code for callback functions
	which are shared among components of the desktop manager.
*/
						/* #includes go here	*/
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Dt/DesktopI.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

#define MSG_PROTO	Screen *scrn, XEvent *xevent, \
			DtRequest *request,  DtReply *reply

static int	open_folder_proc ( MSG_PROTO );
static int	sync_folder_proc ( MSG_PROTO );
static int	create_class_proc ( MSG_PROTO );
static int	delete_class_proc ( MSG_PROTO );
static int	query_class_proc ( MSG_PROTO );
static int	get_class_proc ( MSG_PROTO );
static int	get_property_proc ( MSG_PROTO );
static int	set_property_proc ( MSG_PROTO );
static int	display_prop_sheet_proc ( MSG_PROTO );
static int	display_binder_proc ( MSG_PROTO );
static int	open_fmap_proc ( MSG_PROTO );
static int	dt_shutdown_proc ( MSG_PROTO );
static int	get_filename_proc ( MSG_PROTO );
static int	merge_resource_proc ( MSG_PROTO );
static int	close_folder_proc ( MSG_PROTO );
static int	set_file_prop_proc ( MSG_PROTO );
static int	sync_proc ( MSG_PROTO );

static char *ExpandProps(char *str, DtPropListPtr obj_pp, DtPropListPtr req_pp);
static DtPropList ExpandPropList(DtPropList plist, DtPropListPtr obj_pp,
	DtPropListPtr req_pp);
static DmFnameKeyPtr DmClassFile(char *fname, unsigned short ftype,
		DtAttrs options, char *node, DtPropListPtr obj_pp);

	/* external function declarations */
extern char *dirname(char *);
extern char *basename(char *);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/*
 * This array must match with information in DtDTMMsg.h.
 */
static int (*(dtm_procs[]))() = {
	open_folder_proc,
	sync_folder_proc,
	create_class_proc,
	delete_class_proc,
	query_class_proc,
	get_property_proc,
	set_property_proc,
	display_prop_sheet_proc,
	display_binder_proc,
	open_fmap_proc,
	dt_shutdown_proc,
	get_class_proc,
	get_filename_proc,
	merge_resource_proc,
	close_folder_proc,
	set_file_prop_proc,
	sync_proc,
};

static DtPropList _plist;


/* Private Procedures */


static int
merge_resource_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	/* need to send reply */
	if (request->merge_res.resp) {
		if (request->merge_res.flag) { 
			if (Desktop->dpalette)
				XtFree(Desktop->dpalette);
			Desktop->dpalette= request->merge_res.resp;
		} else {
			MergeResources(request->merge_res.resp);
			XtFree(request->merge_res.resp);
		}
	}
	reply->merge_res.status = DT_OK;
	return 1;
}

/****************************procedure*header*****************************
    open_folder_proc-
*/
static int
open_folder_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	int ret = 0;

#define REQ	request->open_folder
#define REP	reply->open_folder

	if (request->open_folder.path) {
		DmFolderWinPtr fwp;

		fwp = DmOpenFolderWindow(REQ.path, DM_B_ROOTED_FOLDER, NULL, 
					 False);

		if (REQ.options & DT_NOTIFY_ON_CLOSE) {
			if (fwp) {
				DmCloseNotifyPtr np;

				if (np = (DmCloseNotifyPtr)realloc(fwp->notify,
						  sizeof(DmCloseNotifyRec) *
						  (fwp->num_notify + 1))) {
					fwp->notify = np;
					np += fwp->num_notify;
					np->serial = REQ.serial;
					np->version= REQ.version;
					np->client = xevent->xselectionrequest.
						     requestor;
					np->replyq = xevent->xselectionrequest.
						     property;
					fwp->num_notify++;
				}
				else {
					/* realloc failed */
					REP.status = DT_FAILED;
					ret = 1;
				}
			}
			else {
				/* open failed */
				REP.status = DT_FAILED;
				ret = 1;
			}
		}
	}

	/* free data in request structure */
	XtFree(REQ.class_name);
	XtFree(REQ.pattern);
	XtFree(REQ.path);
	XtFree(REQ.title);

	return(ret);

#undef REP
#undef REQ
}

/****************************procedure*header*****************************
    close_folder_proc-
*/
static int
close_folder_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{

#define REQ	request->close_folder

	if (REQ.path) {
	    DmContainerPtr	cp;
   
	    if ((cp = DmQueryContainer(REQ.path)) != NULL){
		DmDestroyContainer(cp);
	    }
	    XtFree(REQ.path);
	}

	/* free data in request structure */

	return(0);
#undef REQ

}


/****************************procedure*header*****************************
	sync_folder_proc -
 */
static int
sync_folder_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
	DmContainerPtr cp;

#ifdef DEBUG
	fprintf(stderr, "sync_folder_proc: %s\n", request->sync_folder.path ?
		request->sync_folder.path : "NULL");
#endif

#define REQ	request->sync_folder

	if ( (cp = DmQueryContainer(REQ.path)) != NULL )
	{
	    Dm__RmFromStaleList(cp);
	    /* force sync */
	    Dm__SyncContainer(cp, True);
	}

	/* free data in request structure */
	XtFree(REQ.path);

	return(0);

#undef REQ
}

/****************************procedure*header*****************************
	create_class_proc -
 */
static int
create_class_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	DmFnameKeyPtr new_fnkp;
	DmFnameKeyPtr last_new_fnkp; /* end of the new class list */
	DmFnameKeyPtr last_new_fcfp; /* last fcfp in the new class list */
	DmFnameKeyPtr fnkp;
	DmFnameKeyPtr last_fnkp;     /* end of the current class list */
	DmFnameKeyPtr save_fnkp;
	char *sysfile;

#define REQ	request->create_fclass
#define REP	reply->create_fclass
#define FCFP(P)	((DmFclassFilePtr)(P))

	/* check if file is already in the database */
	if (DmCheckFileClassDB(REQ.file_name)) {
		REP.status = DT_DUP;
		goto bye;
	}

	/* read the database */
	new_fnkp = fnkp = DmReadFileClassDB(REQ.file_name);

	if (new_fnkp) {
		last_new_fcfp = new_fnkp;

		/*
		 * Turn on the REPLACED bit, so that new classes are recognized.
		 */
		while (fnkp) {
			fnkp->attrs |= DM_B_REPLACED;
			/* This is useful only if DT_PRIVATE is set.
			 * If DT_PRIVATE is not set, the levels are
			 * reset below.
			 */
			fnkp->level++; /* included in toplevel file */

			if (fnkp->attrs & DM_B_CLASSFILE)
				last_new_fcfp = fnkp;
			else
				DmInitFileClass(fnkp);

			if (fnkp->next)
				fnkp = fnkp->next;
			else {
				last_new_fnkp = fnkp;
				break;
			}
		}

		/* Determine which file to insert new entry in.  If it's
		 * a private file class, turn the write bit on for
		 * $HOME/.dtfclass; otherwise, do that for
		 * $XWINHOME/lib/classdb/file_classes.
		 */
		if (REQ.options & DT_PRIVATE) {

			/* The first entry in fnkp is the file header.  It's
			 * level is one greater than that of the file it's
			 * INCLUDE'd in, in this case its $HOME/.dtfclass,
			 * which is level 0.
			 *
			 * Other entries in fnkp (which are the actual file
			 * class entries) should have level one higher than
			 * its file header.
			 *
			 * Note that level was incremented above and in
			 * DmReadFileClassDB().
			 */
			new_fnkp->level--;

			/* assume DESKTOP_FNKP can never be NULL */
			fnkp = DESKTOP_FNKP(Desktop); /* get the current list */
			fnkp->attrs |= DM_B_WRITE_FILE;

		} else {
			/* In SVR4.2 V1, DT_APPEND meant add the new file class
			 * to the end of the file class database.  Since new file
			 * classes were only included in the user's $HOME/.dtfclass
			 * file, the file class file was added to the end of .dtfclass.
			 *
			 * Starting with SVR4.2 V2, if DT_PRIVATE and DT_APPEND are
			 * both set, then it's meaning is unchanged.  If DT_PRIVATE is
			 * not set and DT_APPEND is, the new file class file is
			 * included at the beginning of "system.post".  It is not
			 * appended to the end of "system.post" to avoid attributes
			 * of the new file class being overridden by potentially more
			 * generic attributes of file classes in "system.post".
			 *
			 * If DT_PRIVATE and DT_APPEND are both not set, the new
			 * file class is added to the front of the "system" file.
			 *
			 * "system" and "system.post" reside in /usr/X/lib/classdb.
			 */

			if (REQ.options & DT_APPEND) {
				sysfile = "system.post";
				/* turn of DT_APPEND option */
				REQ.options &= ~DT_APPEND;
			} else
				sysfile = "system";

			for (fnkp = DESKTOP_FNKP(Desktop); fnkp; fnkp = fnkp->next) {
				if (fnkp->attrs & DM_B_CLASSFILE &&
					!strcmp(fnkp->name, sysfile))
				{
					fnkp->attrs |= DM_B_WRITE_FILE;
					/* level of file class header should always
					 * be one higher than that of the file it's
					 * INCLUDE'd in.
					 */
					new_fnkp->level = fnkp->level + 1;
					save_fnkp = fnkp;
					break;
				}
			}
			
			/* Reset level of file class entries in REQ.file_name.
			 * Note that their level was set to one above.
			 * There should be at least one file class entry.
			 * The level of file class entries is one higher than
			 * its file header.
			 */
			fnkp = new_fnkp->next;
			while (fnkp) {
				fnkp->level = new_fnkp->level + 1;
				fnkp = fnkp->next;
			}
			/* restore fnkp */
			fnkp = save_fnkp;
     	}

		/* At this point, fnkp should be set to DESKTOP_FNKP if
		 * DT_PRIVATE is set; otherwise, it should be pointing to
		 * fnkp of $XWINHOME/lib/classdb/file_classes.
		 */
		if (REQ.options & DT_APPEND) {
			/* insert the file at the end */
			DmFnameKeyPtr last_fcfp = fnkp;

			for (; fnkp->next; fnkp=fnkp->next) {
				if (fnkp->attrs & DM_B_CLASSFILE)
					last_fcfp = fnkp;
			}

			fnkp->next = new_fnkp;
			new_fnkp->prev = fnkp;
			FCFP(last_fcfp)->next_file = FCFP(new_fnkp);
		}
		else { /* default case - insert the new file to the front */

			/* The first entry in fnkp is always preserved
			 * because it is the file header.
			 */
			last_new_fnkp->next = fnkp->next;
			if (fnkp->next)
				fnkp->next->prev = last_new_fnkp;
			fnkp->next = new_fnkp;
			new_fnkp->prev = fnkp;
			FCFP(last_new_fcfp)->next_file = FCFP(fnkp)->next_file;
			FCFP(fnkp)->next_file = FCFP(new_fnkp);
		}

		/* sync all windows */
		DmSyncWindows(DESKTOP_FNKP(Desktop), NULL);

		/* turn off the replaced bit */
		for (fnkp=new_fnkp; fnkp; fnkp=fnkp->next)
			fnkp->attrs &= ~DM_B_REPLACED;
		/* flush the change to disk */
		DmWriteFileClassDBList(DESKTOP_FNKP(Desktop));
		REP.status = DT_OK;
	} else
		REP.status = DT_FAILED;

bye:
	/* free data in request structure */
	XtFree(REQ.file_name);

	return(1);
#undef REP
#undef REQ
}

/****************************procedure*header*****************************
	delete_class_proc -
 */
static int
delete_class_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	DmFnameKeyPtr fcfp;
	DmFnameKeyPtr prev_fcfp;

#define REQ	request->delete_fclass
#define REP	reply->delete_fclass

	/* find the file in the database */
	prev_fcfp = NULL;
	fcfp = DESKTOP_FNKP(Desktop);
	while (fcfp) {
		if (!strcmp(fcfp->name, REQ.file_name))
			break;
		prev_fcfp = fcfp;
		fcfp = (DmFnameKeyPtr)(FCFP(fcfp)->next_file);
	}

	/* check if file is already in the database */
	if (fcfp) {
		DmFnameKeyPtr fnkp;
		DmFnameKeyPtr last_fcfp;
		unsigned short ilevel; /* initial file level */
		/* must not remove the toplevel file */
		if (fcfp == DESKTOP_FNKP(Desktop)) {
			REP.status = DT_NOENT;
			goto bye;
		}

		/* find the end of the list to be deleted */
		for (fnkp=fcfp->next, ilevel=fnkp->level, last_fcfp=fcfp;
		     fnkp; fnkp=fnkp->next){
			if (fnkp->level < ilevel) {
				fnkp = fnkp->prev; /* go back one */
				break;
			}

			if (fnkp->attrs & DM_B_CLASSFILE)
				last_fcfp = fnkp;
		}

		/* remove the deleted classes from the standard list */
		FCFP(prev_fcfp)->next_file = FCFP(last_fcfp)->next_file;
		fcfp->prev->next = fnkp->next;
		if (fnkp->next)
			fnkp->next->prev = fcfp->prev;

		/* sync all windows */
		DmSyncWindows(DESKTOP_FNKP(Desktop), fcfp);

		/* free classes */
		/*
		 * There are problems freeing the class info here, because
		 * fnkp or fcp may still be used by property sheets or
		 * wastebasket. Without maintaining a usage count, the deleted
		 * cannot be freed. Sigh.
		 */

		/* flush the change to disk */
		DESKTOP_FNKP(Desktop)->attrs |= DM_B_WRITE_FILE;
		DmWriteFileClassDBList(DESKTOP_FNKP(Desktop));
		REP.status = DT_OK;
	}
	else
		REP.status = DT_NOENT;

bye:
	/* free data in request structure */
	XtFree(REQ.file_name);

	return(1);
#undef REP
#undef REQ
}

/****************************procedure*header*****************************
	query_class_proc -
 */
static int
query_class_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
#define REQ	request->query_fclass
#define REP	reply->query_fclass

	if (REQ.class_name) {
		register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

		REP.class_name  = NULL;
		REP.plist.count = 0;
		REP.plist.ptr   = NULL;

		for (; fnkp; fnkp = fnkp->next) {
			if (!(fnkp->attrs & DM_B_CLASSFILE) &&
			    !strcmp(fnkp->name, REQ.class_name)) {
				REP.status = DT_OK;
				if (REQ.options & DT_GET_PROPERTIES) {
					REP.class_name = strdup(fnkp->name);
					DtCopyPropertyList(&(REP.plist), &(fnkp->fcp->plist));
				}
				break;
			}
		}

		if (!fnkp)
			REP.status = DT_FAILED;

		/* free data in request structure */
		free(REQ.class_name);
	}
	else
		REP.status = DT_BAD_INPUT;

	return(1);

#undef REP
#undef REQ
}

/****************************procedure*header*****************************
	get_class_proc - Given a file name and other optional information,
	returns the name of its file class and, optionally, a pointer to its
	file class.
 */
static int
get_class_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
#define REQ	request->get_fclass
#define REP	reply->get_fclass

	if (REQ.file_name) {
		DmFnameKeyPtr fnkp;
		DtPropList obj_plist;

		REP.file_name   = strdup(REQ.file_name);
		REP.plist.count = 0;
		REP.plist.ptr   = NULL;
		obj_plist.count = 0;
		obj_plist.ptr   = NULL;
		if (fnkp = DmClassFile(REQ.file_name, REQ.file_type, REQ.options,
			REQ.nodename, &obj_plist))
		{
			REP.status     = DT_OK;
			REP.class_name = strdup(fnkp->name);
			if ((REQ.options & DT_GET_PROPERTIES) && fnkp->fcp->plist.count)
			{
				if (REQ.options & DT_EXPAND_PROPERTIES)
				{
					REP.plist = ExpandPropList(fnkp->fcp->plist, &obj_plist,
						REQ.plist.ptr ? &(REQ.plist) : NULL);
				} else {
					DtCopyPropertyList(&(REP.plist), &(fnkp->fcp->plist));
				}
			}
		} else {
			REP.status = DT_FAILED;
			REP.class_name = strdup("UNK");
		}

		/* free data in request structure */
		free(REQ.file_name);

		/* free object's property list */
		DtFreePropertyList(&obj_plist);
	}
	else
		REP.status = DT_BAD_INPUT;

	return(1);

#undef REP
#undef REQ
} /* end of get_class_proc */

/****************************procedure*header*****************************
	get_property_proc -
 */
static int
get_property_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
#define REQ	request->get_property
#define REP	reply->get_property

	if (REQ.name) {
		REP.value  = strdup(DmGetDTProperty(REQ.name, &(REP.attrs)));
		REP.status = DT_OK;
		XtFree(REQ.name);
	} else
		REP.status = DT_BAD_INPUT;
		
	/* free data in request structure */

	return(1);

#undef REP
#undef REQ
}

/****************************procedure*header*****************************
	get_property_proc -
 */
static int
set_property_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
#define REQ	request->set_property

	if (REQ.name) {
		DtSetProperty(&DESKTOP_PROPS(Desktop), REQ.name, REQ.value,
			 	REQ.attrs);
		DmSaveDesktopProperties(Desktop);

		/* free data in request structure */
		XtFree(REQ.name);
	}
	XtFree(REQ.value);

	return(0);
#undef REQ
}

/****************************procedure*header*****************************
    display_prop_sheet_proc- popup Desktop property sheet.
	This must mimic a button press on the Properties button of olwsm.
	The request contains the name of the sheet (currently unused).
*/
static int
display_prop_sheet_proc( MSG_PROTO )
{
#define REQ	request->display_prop_sheet

#ifndef NOWSM
    extern void PropertySheetByName();
    PropertySheetByName( (REQ.prop_name == NULL) ? "Desktop" : REQ.prop_name );
#endif /* NOWSM */

    XtFree(REQ.prop_name);	/* free data in request structure */
    return(0);

#undef REQ
}				/* end of display_prop_sheet_proc */

/****************************procedure*header*****************************
	display_binder_proc -
 */
static int
display_binder_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{

	DmInitIconSetup(NULL, False, True);
	return(0);
}

/****************************procedure*header*****************************
	open_fmap_proc -
 */
static int
open_fmap_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	DmFolderOpenTreeCB(NULL, NULL, NULL);
	return(0);
}

/****************************procedure*header*****************************
	dt_shutdown_proc -
 */
static int
dt_shutdown_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{

	DmShutdownPrompt();
	return(0);
}

/***************************public*procedures*****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmInitDTMReqProcs-
*/
void
DmInitDTMReqProcs(Widget w)
{
	static DmPropEventData dtm_prop_ev_data = {
				(Atom)0,
				Dt__dtm_msgtypes,
				dtm_procs,
				XtNumber(dtm_procs)
	};

	/*
	 * Since _DT_QUEUE is not a constant,
	 * it cannot be initialized at compile time.
	 */
	dtm_prop_ev_data.prop_name = _DT_QUEUE(XtDisplay(w));

	DmRegisterReqProcs(w, &dtm_prop_ev_data);
}

/****************************procedure*header*****************************
	DmClassFile - Classes a file and returns a pointer to its file class
	and instance property list.  It classes a file using the same mechanism
	for icons in a folder if the file is on a local file system and the
	file name is a full path.  Otherwise, it classes the file by trying
	to find a match between the information specified and the file clases.
 */
static DmFnameKeyPtr
DmClassFile(char *fname, unsigned short ftype, DtAttrs options, char *node,
	DtPropListPtr obj_pp)
{
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);
	DmFnameKeyPtr fp;
	char *p;
	char *free_this;
	char *s = strdup(fname);
	Boolean found;
	Boolean is_link = False;
	Boolean is_full_path = False;
	Boolean is_local = False;

	if (*fname == '/')
		is_full_path = True;

	if (!strcmp(node, DESKTOP_NODE_NAME(Desktop)))
		is_local = True;

	if (is_full_path && is_local && (fp = DmGetFileClass(s, obj_pp))) {
		free(s);
		return(fp);
	}
	free(s);

	if (is_full_path && !(options & DT_UPPER_CASE) && is_local)
	{
		struct stat lbuf;

		if (ftype == DT_NO_FILETYPE) {
			struct stat buf;
			if (stat(fname, &buf) != -1) {
				register DmFmodeKeyPtr fmkp =
					DESKTOP_FMKP(Desktop);

				mode_t fmt  = buf.st_mode & S_IFMT;
				mode_t perm = buf.st_mode &
					(S_IRWXU | S_IRWXG | S_IRWXO);
				dev_t  rdev = buf.st_rdev;

				for (; fmkp->name; fmkp++) {
					if ((!(fmkp->fmt)  ||
					  (fmkp->fmt == fmt)) &&
					  (!(fmkp->perm) ||
					  (fmkp->perm & perm)) &&
					  (!(fmkp->rdev) ||
					  (fmkp->rdev == rdev)))
					{
						ftype = fmkp->ftype;
						break;
					}
				}
			}
		}
		if (lstat(fname, &lbuf) != -1 &&
		  ((lbuf.st_mode & S_IFMT)==S_IFLNK))
			is_link = True;
	}

	for (; fnkp; fnkp = fnkp->next) {
		found = False;
		if (fnkp->attrs & (DM_B_CLASSFILE | DM_B_CLASSFILE|
		  DM_B_OVERRIDDEN))
			continue;

		if (!is_full_path && (fnkp->attrs & DT_UPPER_CASE) &&
			((!is_link && (!(fnkp->attrs & DM_B_REGEXP) ||
				(fnkp->attrs & DM_B_FILEPATH))) ||
			(is_link && (!(fnkp->attrs & DM_B_LREGEXP) ||
				(fnkp->attrs & DM_B_LFILEPATH)))))
			continue;

		if (fnkp->attrs & DM_B_REGEXP) {
			char *file_name = basename(fname);

			for (p = fnkp->re; *p; p = p + strlen(p)+1) {
				if (options & DT_UPPER_CASE)
					free_this = DmToUpper(p);
				else
					free_this = strdup(p);

				if (gmatch(file_name, free_this)) {
					free(free_this);
					found = True;
					break;
				}
				free(free_this);
			}
			if (!found)
				continue;
		}

		if (fnkp->attrs & DM_B_FILEPATH && is_full_path) {
			char *path;
			char *s = NULL;
			char *rpath =DmResolveSymLink(fname,&path,&free_this);

			if (!rpath) {
				s = strdup(fname);
				path = dirname(s);
			} else {
				free(rpath);
			}
			free_this = NULL;
			p = DtGetProperty(&(fnkp->fcp->plist), FILEPATH, NULL);
			/* expand any desktop properties */
			p = Dm__expand_sh(p, NULL, NULL);

			if (options & DT_UPPER_CASE)
				free_this = DmToUpper(p);

			found = DmMatchFilePath(free_this?free_this : p, path);
			free(p);
			if (s)
				free(s);
			if (free_this)
				free(free_this);
			if (!found)
				continue;
		}

		if (fnkp->attrs & DM_B_LFILEPATH && is_full_path)
		{
			char *real_path;
			char *real_name;
			char *p;

			if (!is_link)
				continue;

			if (!(free_this = DmResolveSymLink(fname, &real_path,
					&real_name)))
			    continue;
			free(free_this);

			if (!(p = DmResolveLFILEPATH(fnkp, NULL)) ||
				strcmp((p[0] == '/' && p[1] == '/') ?
					p + 1 : p, real_path))
				continue;
			else
				found = True;

			if (!found)
				continue;
		}

		if (fnkp->attrs & DM_B_LREGEXP && is_full_path)
		{
			char *real_name;
			char *real_path;
			char *p;

			if (!is_link)
				continue;

			if (!(free_this = DmResolveSymLink(fname, &real_path,
			  &real_name)))
				continue;
			free(free_this);

			for (p=fnkp->lre; *p; p = p + strlen(p)+1) {
				if (gmatch(real_name, p)) {
					found = True;
					break;
				} else
					found = False;
			}
			if (!found)
				continue;
		}

		if (fnkp->attrs & DM_B_FILETYPE) {
			if (ftype == DT_IGNORE_FILETYPE)
				if (found)
					return(fnkp);
			else if (ftype != fnkp->ftype)
				continue;
		}

		if (found)
			return(fnkp);
	}
	return(NULL);

} /* end of DmClassFile */

/****************************procedure*header*****************************
	ExpandPropList - Expands desktop properties in a property list and
	returns an expanded property list.  Environment variables and other
	properties such as %s, %f are not expanded.  Accepts a list of instance
	properties and the list of properties passed in with a client request
	to be used in the expansion.
 */
static DtPropList
ExpandPropList(DtPropList plist, DtPropListPtr obj_pp, DtPropListPtr req_pp)
{
	DtPropPtr newp;
	DtPropPtr p;
	int i;

	if (plist.count == 0) {
		_plist.count = 0;
		_plist.ptr = NULL;
		return(_plist);
	}
	newp = (DtPropPtr)calloc(plist.count, sizeof(DtPropRec));
	_plist.ptr = newp;
	for (i = 0, p = plist.ptr; i < plist.count; p++, newp++, i++) {
		newp->name = strdup(p->name);
          newp->value = ExpandProps(p->value, obj_pp, req_pp);
		newp->attrs = p->attrs;
	}
	_plist.count = plist.count;
	return(_plist);

} /* end of ExpandPropList */

/****************************procedure*header*****************************
	ExpandDesktopProps - Searches and expands desktop properties.
	Returns expanded string.  Accepts two property lists: instance
	properties and a listed created by the caller.  The latter has
	highest precedence over the desktop manager's desktop properties
	and the instance properties, and the instance properties have a
	higher precedence over the desktop properties.  The caller should
	free the returned string when done with it.
 */
static char *
ExpandProps(char *str, DtPropListPtr obj_pp, DtPropListPtr req_pp)
{
     register char *p = str;
     char *start;
     char *name;
     char *end;
     char *value;
     char *ret;
     char *free_this = NULL;
     int size;
     int notfirst = 0;
     int enclosed;

	while (1) {
		while (*p && *p != '%')
			p++;
		if (!*p) {
			/* nothing to expand */
			if (notfirst)
				return(str);
			else
				/* return a copy of the original string */
				return(strdup(str));
		}
		start = p;
		if (*++p == '{') {
			/* %{name} : look for matching '}' */
			name = ++p;
			while (*p && (*p != '}'))
				p++;
			enclosed = 1;
		} else {
			/* %name */
			name = p;
			while (*p && (isalpha(*p) || isdigit(*p) || (*p == '_')))
				p++;
			enclosed = 0;
		}
		/* now, p points to the end of env name */
		end = p;
		name = strndup(name, p - name);
		switch(*start) {
		case '%':
			value = NULL;
			if (req_pp)
				value = DtGetProperty(req_pp, name, NULL);
			if (!value && obj_pp)
				value = DtGetProperty(obj_pp, name, NULL);
			/* name may be an object property so value can be NULL */
			if (!value)
				value = DmGetDTProperty(name, NULL);
			if (!value) {
				sprintf(Dm__buffer, "%%%s", name);
				value = Dm__buffer;
			}
			break;
		}
		free(name);
		size = strlen(str) + 1 + (value ? strlen(value) : strlen(name));
		if (!(ret = (char *)malloc(size)))
			return(NULL);
		memcpy(ret, str, start - str);
		p = ret + (start - str);
		if (value) {
			strcpy(p, value);
			p += strlen(value);
		}

		if (free_this) {
			free(free_this);
			free_this = NULL;
		}
		if (enclosed)
			++end;
		strcpy(p, end);
		if (notfirst)
			free(str);
		str = ret;
		notfirst = 1;
	} /* while */

} /* ExpandDesktopProps */

/****************************procedure*header*****************************
	get_filename_proc - Returns the file name of an icon at specified
	position (x, y) in window of a specified id.  Returns the path name
	of a folder window if there is a match between the window id and
	a flat icon box but no icon was found at (x, y).  If there is no
	match for the window id, file name returned is NULL.
 */
static int
get_filename_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
#define REQ	request->get_fname
#define REP	reply->get_fname

	Cardinal idx;
	DmFolderWindow window = DESKTOP_FOLDERS(Desktop);

	REP.file_name = NULL;

	for (window; window; window = window->next)
	{
		if (XtWindow(window->views[0].box) == REQ.win_id)
		{
			if ((idx = ExmFlatGetItemIndex(window->views[0].box, REQ.icon_x,
				REQ.icon_y)) == ExmNO_ITEM)
			{
				REP.file_name = strdup(window->views[0].cp->path);
			} else {
				REP.file_name =
					strdup(DmObjPath(ITEM_OBJ((window->views[0].itp)+idx)));
			}
			REP.status = DT_OK;
			break;
		}
	}
	if (!window)
		REP.status = DT_FAILED;

	return(1);

#undef REP
#undef REQ

} /* end of get_filename_proc */


/****************************procedure*header*****************************
    set_file_prop_proc-
*/
static int
set_file_prop_proc(Screen *scrn, XEvent *xevent, DtRequest *request,
	DtReply *reply)
{
	int ret = 0;

#define REQ	request->set_file_prop

	if (REQ.path && (REQ.path[0] == '/')) {
	    DmContainerPtr	cp;
	    char *name;
	    char *path;

	    path = REQ.path;

	    if (REQ.name)
	    	name = REQ.name;
	    else {
	    	char *p;

	    	/* take the last part of path as the name */
	    	p = strrchr(REQ.path, '/');

	    	/* p cannot be NULL. We already checked the first char */
	    	if (p == path)
	    		path = "/";
	    	*p++ = '\0';
	    	name = p;
	    }

	    if (cp = DmOpenDir(path, DM_B_READ_DTINFO)) {
			DmObjectPtr op;

			if (op = DmGetObjectInContainer(cp, name)) {

				if (REQ.plist.count) {
			    	DtPropPtr pp;

			    	pp = DtFindProperty(&(REQ.plist), 0);
			    	while (pp) {
			        	DmSetObjProperty(op, pp->name, *(pp->value) ?
							pp->value : NULL, pp->attrs);
			        	pp = DtFindProperty(NULL, 0);
			    	}
				}
				else {
			    	/* remove all properties */
			    	DtFreePropertyList(&(op->plist));
				}

				DmFlushContainer(cp); /* force a flush */

				DmRetypeObj(op, True);
				DmCloseContainer(cp);
			}
	    }

	    /* free data in request structure */
	    XtFree(REQ.path);
	    XtFree(REQ.name);
	}

	return(0);
#undef REQ

}

/****************************procedure*header*****************************
	sync_proc - This request is used by clients to ensure dtm has
		    started processing of all previous requests from that
		    client.
 */
static int
sync_proc(Screen *scrn, XEvent *xevent, DtRequest *request, DtReply *reply)
{
#define REP	reply->sync

	REP.status = DT_OK;
	return(1);

#undef REP
}

