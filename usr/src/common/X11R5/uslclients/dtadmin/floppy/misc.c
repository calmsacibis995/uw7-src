#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/misc.c	1.30.1.1"
#endif

#include "media.h"

#include <archives.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define	CORRUPT		0x2900
#define	BUSY_MNT	0xf00


char *dos_chars = "abcdefghijklmnopqurstuvwxyz0123456789_^$~!#%&-{}'`()@";
#define DM_DOS_TRUNCATE 	( 1 << 0 ) /* filename truncated */
#define DM_DOS_CASE_CONV 	( 1 << 1 ) /* case-conversion done */
#define	DM_DOS_BAD_CHAR 	( 1 << 2 ) /* illegal char converted to $*/
static int Unix2DosName(char *unix_name, char *dos_name, int unix_len,
			unsigned short *conversions);

static void OverWritePrompt(FileOpPtr file_op);
static void PostErrorMsg(FileOpPtr file_op);
static Boolean CheckFileOp(FileOpPtr file_op);
static Boolean DoFileOp(FileOpPtr file_op);
static void ContinueFileOp(FileOpPtr file_op);
static void SyncFolder(XtPointer, XtIntervalId *);



static void	CancelFileOpCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	DestroyCB(Widget widget, XtPointer client_data, 
			  XtPointer call_data);
static void	OverwriteCB (Widget widget, XtPointer client_data,
			     XtPointer call_data);
static void	DontOverwriteCB (Widget widget, XtPointer client_data,
				 XtPointer call_data);
static void	OverwriteHelpCB (Widget widget, XtPointer client_data,
				 XtPointer call_data);
static void	PopdownCB(Widget widget, XtPointer client_data, 
			  XtPointer call_data);
static void	ReopenCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	RecloseCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	MntCloseCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	StartFileOpCB (Widget widget, XtPointer client_data,
		     XtPointer call_data);
static void	attempt_umount (void);

static MenuItems Insert_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, RecloseCB, 0 },
	{ NULL }
};
static MenuGizmo Insert_menu = {0, "Insert_menu", NULL, Insert_menu_item};
static ModalGizmo Insert = {0, "", title_confirmIns, (Gizmo)&Insert_menu};

static MenuItems Busy_menu_item[] = {  
	{ TRUE, label_close, mnemonic_close, 0, MntCloseCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, ReopenCB, NULL },
	{ NULL }
};
static MenuGizmo Busy_menu = {0, "Busy_menu", NULL, Busy_menu_item};
static ModalGizmo Busy = {0, "", title_confirmUmt, (Gizmo)&Busy_menu};

static MenuItems Name_prompt_menu_items[] = {  
	{ TRUE, label_continueOp, mnemonic_continueOp, 0, StartFileOpCB, 0 },
	{ TRUE, label_dontContinue, mnemonic_dontContinue, 0, CancelFileOpCB, 0 },
	{ NULL }
};
static MenuGizmo Name_prompt_menu = {0, "Name_prompt_menu", NULL, 
					Name_prompt_menu_items};
static ModalGizmo Name_prompt = {0, "", title_fileop, (Gizmo)&Name_prompt_menu, title_namePrompt};

static MenuItems Error_menu_items[] = {  
	{ TRUE, label_ok, mnemonic_ok, 0, DontOverwriteCB, 0 },
	{ NULL }
};

static MenuGizmo Error_menu = {0, "Error_menu", NULL, 
					Error_menu_items};
static ModalGizmo Error_prompt = {0, "", title_fileop, (Gizmo)&Error_menu, NULL};

static MenuItems overwrite_menu_items[] = {  
	{ TRUE, label_overwrite, mnemonic_overwrite, 0, OverwriteCB, 0 },
	{ TRUE, label_dontOverwrite, mnemonic_dontOverwrite, 0, DontOverwriteCB, 0 },
	{ TRUE, label_help, mnemonic_help, 0, OverwriteHelpCB, 0 },
	{ NULL }
};
static MenuGizmo overwrite_menu = {0, "overwrite_menu", NULL, 
				overwrite_menu_items};
static ModalGizmo Overwrite_prompt = {0, "", title_fileop, (Gizmo)&overwrite_menu, NULL};

extern	char	*_dtam_mntpt;
extern	long	_dtam_flags;

XtIntervalId	disk_tid = NULL;

void	MonitorMount(XtPointer closure, XtIntervalId id)
{
	sync();
	/*
	 *	should probably do a CheckMedia() and issue a popup warning
	 *	in case the disk has been removed, doing the sync only if 
	 *	the disk remains in the drive.
	 */
	disk_tid = XtAddTimeOut(3000,(XtTimerCallbackProc)MonitorMount, NULL);
}

int	attempt_mount(diagn, device, fstype)
	int	diagn;
	char	*device;
	char	**fstype;
{
	extern	char	*_dtam_fstyp;
	char	*ptr, cmdbuf[BUFSIZ];
	char	cd_mntpt[BUFSIZ];
	char	*label;
	char	*rdonly;
	FILE	*cmdfp;
	int	result;
	Boolean	retry = TRUE;

	*fstype = NULL;

	if (_dtam_flags & DTAM_MOUNTED){
		*fstype = strdup(_dtam_fstyp);
		goto success;
	}
	switch(diagn&DTAM_FS_TYPE) {
		case DTAM_S5_FILES:	*fstype=strdup("s5");	break;
		case DTAM_UFS_FILES:	*fstype=strdup("ufs");	break;
		default:		*fstype=strdup(_dtam_fstyp);
	}

        /* We should NOT internationalize the mount point */

        if (strcmp(curalias,"diskette1")==0)
                label = D1_MNTPT;
        else if (strcmp(curalias,"diskette2")==0)
                label = D2_MNTPT;
        else if (strncmp(curalias,"cdrom", 5)==0) {
		sprintf(cd_mntpt, "%s_%s", CD_MNTPT, curalias+5);
                label = cd_mntpt;
	}
        else label = DtamDevAlias(device);
        
	if (!_dtam_mntpt)
		if ((_dtam_mntpt = (char *)MALLOC(strlen(label)+2)) == NULL)
			return DTAM_CANT_MOUNT;
	*_dtam_mntpt = '/';
	strcpy(_dtam_mntpt+1,label);
	ptr = DtamDevAttr(device,BDEVICE);
	if (strstr(curalias, DISKETTE) && ((_dtam_flags & DTAM_TFLOPPY) == 0))
		/*
		 *	a file system was recognized on /dev/dsk/f?, NOT f?t
		 */
		ptr[strlen(ptr)-1] = '\0';
	rdonly = (_dtam_flags & DTAM_READ_ONLY)? " -r " : " ";

retry:	sprintf(cmdbuf, "/sbin/tfadmin fmount %s-F %s %s %s",
				rdonly, *fstype, ptr, _dtam_mntpt);
	result = system(cmdbuf);
	switch (result) {
	
	case 0:		goto success;
	case CORRUPT:
		if (retry) {
		    switch(diagn&DTAM_FS_TYPE) {
			case DTAM_S5_FILES:	
				sprintf(cmdbuf, "/sbin/fsck -y -q %s",ptr);
				break;
			case DTAM_UFS_FILES: 	
				sprintf(cmdbuf, "/sbin/fsck -F ufs -y %s",ptr);
				break;
			default:		
				sprintf(cmdbuf, "/sbin/fsck -F %s -y %s",
					*fstype, ptr);
				break;
		    }
		    system(cmdbuf);
		    retry = FALSE;
		    goto retry;
		}
		/* Fall through!!! */
	default:	FREE(ptr);
			return CANT_MOUNT;
	}
success:
	/* Change the working directory to the mount point to prevent the
	 * disk from being unmounted by someone else behind our back.
	 */
	chdir (_dtam_mntpt);
	disk_tid = XtAddTimeOut(3000, (XtTimerCallbackProc)MonitorMount, NULL);
	return MOUNTED;
}


long	request_no;

void	CloseDisketteHandler(w, client_data, xevent, cont_to_dispatch)
	Widget		w;
	XtPointer	client_data;
	XEvent		*xevent;
	Boolean		*cont_to_dispatch;
{
	DtReply		reply;
	DtPropPtr	pp;

	if ((xevent->type != SelectionNotify)
	||  (xevent->xselection.selection != _DT_QUEUE(theDisplay)))
		return;
	if (disk_tid)
		XtRemoveTimeOut(disk_tid);
        memset(&reply, 0, sizeof(reply));
	DtAcceptReply(theScreen, _DT_QUEUE(theDisplay),
				XRootWindowOfScreen(theScreen), &reply);
	/* Always unmount the floppy, even if we didn't mount it. */
	attempt_umount ();
}

static void
attempt_umount (void)
{
	char	msg [BUFSIZ];
	char	*drive;

	chdir ("/");
	switch (DtamUnMount(_dtam_mntpt)) {
	case 0:
	    exit (0);
	case -1:
	    /* Insert floppy */
	    if (!Insert.shell)
		CreateGizmo(w_toplevel, ModalGizmoClass, &Insert, NULL, 0);

	    if (strncmp (curdev, DISKETTE, strlen (DISKETTE)) != 0)
		sprintf (msg, GetGizmoText (string_reInsMsg), "");
	    else
	    {
		drive = DtamDevAlias(curdev);
		sprintf (msg, GetGizmoText (string_reInsMsg),
			 drive + strlen (drive) - 1);
		FREE (drive);
	    }
	    SetModalGizmoMessage(&Insert, msg);
	    MapGizmo(ModalGizmoClass, &Insert);
	    break;
	default:
	    /* Busy or misc. failure */
	    if (!Busy.shell) 
		CreateGizmo(w_toplevel, ModalGizmoClass, &Busy, NULL, 0);

	    
	    if (strstr(curalias, DISKETTE))
	    	sprintf (msg, GetGizmoText (busyMntDisk), _dtam_mntpt,
		     _dtam_mntpt, _dtam_mntpt);
	    else if (strstr(curalias, CDROM))    
	    	sprintf (msg, GetGizmoText (busyMntCD), _dtam_mntpt,
		     _dtam_mntpt, _dtam_mntpt);
	    else
		sprintf (msg, GetGizmoText (string_busyMsg), _dtam_mntpt,
                     _dtam_mntpt);
	    XtVaSetValues(Busy.stext, XtNalignment, (XtArgVal)OL_LEFT, NULL);
	    SetModalGizmoMessage(&Busy, msg);
	    MapGizmo(ModalGizmoClass, &Busy);
	    break;
	}
	chdir (_dtam_mntpt);
}

static void
ReopenCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	attempt_open (curalias, False);
	BringDownPopup(_OlGetShellOfWidget (w));
}

static void
RecloseCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(_OlGetShellOfWidget (w));
	attempt_umount ();
}

int	attempt_open(alias, addEvent)
	char	*alias;
	Boolean	addEvent;
{
	DtRequest       request;

        memset(&request, 0, sizeof(request));
        request.open_folder.rqtype= DT_OPEN_FOLDER;
        request.open_folder.path  = _dtam_mntpt;
        request.open_folder.title = alias;
	request.open_folder.options = DT_NOTIFY_ON_CLOSE;
        request_no = DtEnqueueRequest(theScreen,
			_DT_QUEUE(theDisplay),
			_DT_QUEUE(theDisplay),
			 XtWindow(w_toplevel), &request);
	if (addEvent)
	    XtAddEventHandler(w_toplevel, (EventMask)NoEventMask, True,
			CloseDisketteHandler, (XtPointer)NULL);
	return 0;
}

void
attempt_copy (FileOpPtr file_op)
{
    ModalGizmo		*prompt;
    Arg			args[2];

    file_op->destination = strdup(_dtam_mntpt);
    file_op->cur_file = 0;
    file_op->dest_files = (char **) XtCalloc(file_op->num_files,sizeof(char *));
    errno = 0;
    
    
    if (file_op->fstype && !strcmp(file_op->fstype, "dosfs")){
	
	/* Copying to DOSFS; warn user about file name truncation
	 * Create the Notice gizmo once per file.
	 * We may get multiple file operations at once via UnixTakeDrop
	 */
	prompt = CopyGizmo(ModalGizmoClass, &Name_prompt);
	XtSetArg(args[0], XtNnoticeType, OL_WARNING);
	CreateGizmo(w_toplevel, ModalGizmoClass, prompt, args, 1);
	prompt->menu->items[0].client_data = (char *) file_op;
	prompt->menu->items[1].client_data = (char *) file_op;
	
	MapGizmo(ModalGizmoClass, prompt);
	XtAddCallback(prompt->shell, XtNpopdownCallback, PopdownCB, prompt);
	XtAddCallback(prompt->shell, XtNdestroyCallback, DestroyCB, prompt);
    }
    else{
	ContinueFileOp(file_op);
    }
}	/* end of attempt_copy */


int	FindSize(list)
	char	*list;
{
	FILE	*cmdpipe;
	int	total = 0;
	char	buf[BUFSIZ];

	sprintf(buf, "du -s %s",list);
	if (cmdpipe = popen(buf,"r")) {
		while (fgets(buf,BUFSIZ,cmdpipe)) {
			total += atoi(buf);
		}
		(void) pclose (cmdpipe);
	}
	return	total;
}

void
UnixTakeDrop(Widget wid, XtPointer client_data, XtPointer call_data)
{
    DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
    extern char		*dropstr;
    FileOpPtr 		file_op;
    int			i;
    extern char		*_dtam_fstyp;
	
    
    if (dip->files && *dip->files && **dip->files) {
		file_op = (FileOpPtr) XtCalloc(1, sizeof(FileOpRec));
		file_op->num_files = dip->nitems;
		file_op->src_files = malloc(sizeof(char *) * dip->nitems);
		file_op->device = strdup(curdev);
	
		for (i=0; i<file_op->num_files; i++)
			file_op->src_files[i] = strdup(dip->files[i]);

		/*
		 * The file system type should be the same as last 
		 * device diagnosis done by this instance of MediaMgr.
		 */
		file_op->fstype = strdup(_dtam_fstyp);
		attempt_copy(file_op);
	
    }
    else
	(void) attempt_open (curalias, False);
}


/*****************************************************************************
 *  Unix2DosName: Convert a UNIX filename to a DOS filename
 *
 *  INPUT: unix_name
 *	   dos name buffer (must be at least 13 bytes)
 *  OUTPUT: 0 if name could be converted
 *	    1 if name could not be converted (name began with '.')
 *
 *	    mask of the following 'conversion' flags:
 *		DM_DOS_TRUNCATE: 	name had to be truncated
 *		DM_DOS_CASE_CONV:	case conversion was required
 *		DM_DOS_BAD_CHAR:	non-dos chars found, replaced with '$'
 *
 *  This function does not ensure that valid
 *  characters for a dos filename are supplied.
 ****************************************************************************/
static int
Unix2DosName(char *unix_name, char *dos_name, int unix_len,
unsigned short *conversions)
{
    int i, j;
    unsigned char c;
    
    *conversions = 0;

    /*
     * We do not want to copy . files.
     */
    if (*unix_name == '.')
	return 1;
    if (unix_len == 0){
	*dos_name = NULL;
	return 0;
    }
    
    /*
     *  Copy the unix filename into the dos filename string
     *  upto the end of string, a '.', or 8 characters.
     *  Whichever happens first stops us.
     *  This forms the name portion of the dos filename.
     *  Fold to upper case.
     */
    for (i = 0; i <= 7  &&  unix_len  &&  (c = *unix_name)  &&  c != '.'; i++){
	if (strchr(dos_chars, (int)c) == NULL) {
	    if (isupper((int)c)){
		*conversions |= DM_DOS_CASE_CONV;
		dos_name[i] = tolower((int)c);
	    }
	    else {
		dos_name[i] = '$';
		*conversions |= DM_DOS_BAD_CHAR;
	    }
	}
	else
	    dos_name[i] = c;
	unix_name++;
	unix_len--;
    }
    
    /*
     *  Strip any further characters up to a '.' or the
     *  end of the string.
     */
    while (unix_len  &&  (c = *unix_name)  &&  c != '.') {
	unix_name++;
	unix_len--;
	*conversions |= DM_DOS_TRUNCATE;
    }
    
    /*
     *  If we stopped on a '.', then get past it.
     */
    if (c == '.') {
	dos_name[i++] = c; 
	unix_name++;
	unix_len--;
    }
    
    /*
     *  Copy in the extension part of the name, if any.
     *  Force to upper case.
     *  Note that the extension is allowed to contain '.'s.
     *  Filenames in this form are probably inaccessable
     *  under dos.
     */
    
    j = i;
    for (;i <= j+2  &&  unix_len  &&  (c = *unix_name) && c != '.'; i++) {
	if (strchr(dos_chars, (int)c) == NULL) {
	    if (isupper((int)c)){
		*conversions |= DM_DOS_CASE_CONV;
		dos_name[i] = tolower((int)c);
	    }
	    else {
		dos_name[i] = '$';
		*conversions |= DM_DOS_BAD_CHAR;
	    }
	}
	else
	    dos_name[i] = c;
	unix_name++;
	unix_len--;
    }
    dos_name[i] = NULL;

    if (unix_len)
	return 1;

    return 0;
}



/**************************************************************************
 * CancelFileOpCB: callback to discontinue file op. Free file_op and
 *	popdown the modal message.  The ModalGizmo and widget are
 *	freed/destroyed in PopdownCB.
 *
 **************************************************************************/
static void
CancelFileOpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    FileOpPtr	file_op = (FileOpPtr) client_data;


    FreeFileOp(file_op);
    BringDownPopup(_OlGetShellOfWidget (widget));

} /* end of CancelFileOpCB */

/**************************************************************************
 * StartFileOpCB: kick off a file operation
 *
 * called from "truncation" popup
 *
 **************************************************************************/
static void
StartFileOpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    FileOpPtr	file_op = (FileOpPtr) client_data;


    /* dismiss the popup.  See PopdownCB for free/destroy of gizmo/widget */
    BringDownPopup(_OlGetShellOfWidget (widget));	

    /*
     * Now kick off the file operation, one file at a time, we
     * may need to pause to present an overwrite
     */
    ContinueFileOp(file_op);
} /* end of StartFileOpCB */

/**************************************************************************
 * ContinueFileOp: continue a file-op at the current file until we complete
 *		all files or hit an overwrite message or an error.
 *
 **************************************************************************/
static void
ContinueFileOp (FileOpPtr file_op)
{
    Boolean	stopped = False;

    /*
     * Now kick off the file operation, one file at a time, we
     * may need to pause to present an overwrite.  If we are
     * entering this file-op for the first time, open the folder
     * for the device.
     */

    if (file_op->cur_file == 0)
	attempt_open(curalias, True);

    while ( !stopped && file_op->cur_file < file_op->num_files){
	if ((stopped = CheckFileOp(file_op)) == False)
	    /* DoFileOp will increment file_op->cur_file */
	    stopped = DoFileOp(file_op);
    }
    /* If file-op not stopped, it should be complete, else it 
     * has been aborted or will be completed/aborted in callback.
     */
    if (!stopped){
	FreeFileOp(file_op);
    }
} /* end of ContinueFileOp */

/**************************************************************************
 * CheckFileOp: for current operation in FileOpRec:
 *		* calculate name under DOSFS (if filetype is dosfs)
 *		* check for overwrite condition
 *		* check for sufficient space
 *
 *	INPUT: FileOpPtr
 *	OUTPUT: 
 *		True: error detected, popup has been posted or
 *			file-op aborted
 *		False: no problem, continue with file-op
 **************************************************************************/
static Boolean
CheckFileOp(FileOpPtr file_op)
{
    Boolean 	found_error = False;
    int		current	= file_op->cur_file;
    char	*dev, *attr;
    int		capacity = 0;
    char 	*unix_name;
    char 	dest_name[256];
    char 	dest_path[PATH_MAX];
    unsigned short char_conversions;
    struct stat stat_buf;


    if (dev = DtamGetDev(file_op->device, FIRST)) {
	if (attr = DtamDevAttr(dev,"capacity")) {
	    capacity = atoi(attr);
	    FREE(attr);
	}
	FREE(dev);
	capacity -= FindSize(file_op->destination);
    }
    if (capacity < FindSize(file_op->src_files[current])){
	/* no room.  Notify user and abort operation.
	 */
	BaseNotice(NO_ROOM, True, NULL);
	FreeFileOp(file_op);	
	found_error = True;
    }
    else{

	/* Determine the destination path and check for overwrite */
	
	strcpy(dest_path, file_op->destination);
	strcat(dest_path,"/");
	unix_name = basename(file_op->src_files[current]);

	if (!strcmp(file_op->fstype, "dosfs")){
		if (Unix2DosName(unix_name, dest_name, strlen(unix_name), 
			 	&char_conversions ) == 0){
	    
	    		strcat(dest_path, dest_name);
		}
		else{
			/* We couldn't translate the name.
			 * Just use unix_name for now.
			 */
			strcat(dest_path, unix_name);
			
		}
	}
	else{
		strcat(dest_path, unix_name);
	}

	file_op->dest_files[current] = strdup(dest_path);

	    		/* check for overwrite */
	errno = 0;
	if (stat(dest_path, &stat_buf) == 0){
		/* overwrite, post warning message */
		found_error = True;
		OverWritePrompt(file_op);
	}
    }

    return found_error;

}  /* end of CheckFileOp */


/**************************************************************************
 * DoFileOp: perform current operation in file_op,
 *		INCREMENT cur_file counter in file_op
 *	INPUT: FileOpPtr
 *	OUTPUT: 
 *		True: error occurred
 *		False: no error occurred
 **************************************************************************/
static Boolean
DoFileOp(FileOpPtr file_op)
{
    Boolean 	found_error = False;
    struct stat stat_buf;
    char 	command[BUFSIZ];
    int		current = file_op->cur_file;
    char 	*unix_path = file_op->src_files[current];
    char 	*dest_path = file_op->dest_files[current];

    
    errno = 0;
    if ((stat(unix_path, &stat_buf) == 0) && 
	((stat_buf.st_mode & S_IFMT) == S_IFDIR))
	    /* cpio is not invoked by CpioCmd here, because this is
	     * supposed to mimic a folder copy as done by dtm, which
	     * does not run with privilege.  This might be revisited
	     * later.
	     */
	    /* BOSI MORE: need to rename subdirectories here */
	sprintf(command, "mkdir \'%s\';"
		" cd \"%s\"; "
		"find . -print | "
		"/usr/bin/cpio -pd \'%s\' 2>/dev/null",
		dest_path, unix_path, dest_path);
    else
	sprintf(command, "cp \"%s\" \'%s\'", unix_path, dest_path);

#ifdef DEBUG    
    fprintf(stderr,"DoFileOp: doing op %d of %d\n", file_op->cur_file,
	    file_op->num_files);
#endif
    if (system(command) != 0){
	found_error = True;
	PostErrorMsg(file_op);
    }

    if (!found_error)
	/* Mark file-op as complete if no error occurred */
	file_op->cur_file++;

    return found_error;
}  /* end of DoFileOp */




/**************************************************************************
 * OverwritePrompt: Warn user of overwrite condition for file-op
 *
 *
 **************************************************************************/
static void
OverWritePrompt(FileOpPtr file_op)
{
    ModalGizmo		*prompt;
    Arg			args[2];

    int			current = file_op->cur_file;
    char 		*dest_path = file_op->dest_files[current];
    char 		message[BUFSIZ];
    char 		*template;

    /* Create the Notice gizmo (once per folder) */
    prompt = CopyGizmo(ModalGizmoClass, &Overwrite_prompt);
    XtSetArg(args[0], XtNnoticeType, OL_WARNING);
    CreateGizmo(w_toplevel, ModalGizmoClass, prompt, args, 1);
    prompt->menu->items[0].client_data = (char *) file_op;
    prompt->menu->items[1].client_data = (char *) file_op;

    template = GetGizmoText(label_overwriteNotice);
    sprintf(message, template, dest_path);
    SetModalGizmoMessage(prompt, message);

    MapGizmo(ModalGizmoClass, prompt);
    XtAddCallback(prompt->shell, XtNpopdownCallback, PopdownCB, prompt);
    XtAddCallback(prompt->shell, XtNdestroyCallback, DestroyCB, prompt);

} /* end of OverwritePrompt */

/**************************************************************************
 * PostErrorMsg: Warn user of failed copy operation
 *
 *
 **************************************************************************/
static void
PostErrorMsg(FileOpPtr file_op)
{
    ModalGizmo		*prompt;
    Arg			args[2];

    int			current = file_op->cur_file;
    char 		*dest_path = file_op->dest_files[current];
    char 		*unix_path = file_op->src_files[current];
    char 		message[BUFSIZ];
    char 		*template;

    /* Create the Notice gizmo (once per folder) */
    prompt = CopyGizmo(ModalGizmoClass, &Error_prompt);
    XtSetArg(args[0], XtNnoticeType, OL_ERROR);
    CreateGizmo(w_toplevel, ModalGizmoClass, prompt, args, 1);
    prompt->menu->items[0].client_data = (char *) file_op;

    template = GetGizmoText(msg_failedFileOp);
    sprintf(message, template, unix_path, dest_path);
    SetModalGizmoMessage(prompt, message);

    MapGizmo(ModalGizmoClass, prompt);
    XtAddCallback(prompt->shell, XtNpopdownCallback, PopdownCB, prompt);
    XtAddCallback(prompt->shell, XtNdestroyCallback, DestroyCB, prompt);

} /* end of PostErrorMsg */

/**************************************************************************
 * OverWriteCB: continue file operation at current file
 **************************************************************************/
static void	
OverwriteCB (Widget widget, XtPointer client_data, XtPointer call_data)
{		
    FileOpPtr	file_op = (FileOpPtr) client_data;
    Boolean	stopped;
    char 	command[BUFSIZ];
    char	*dest_path = file_op->dest_files[file_op->cur_file];

    BringDownPopup(_OlGetShellOfWidget (widget));	
    
    /* Do Current File Operation */
    sprintf(command, "rm -rf \"%s\" 2>/dev/null", dest_path);
    if (system(command) != 0)
	PostErrorMsg(file_op);
    else {
    	if ((stopped = DoFileOp(file_op)) == False)
		/* Now continue file operation */
		ContinueFileOp(file_op);
    }
}	/* end of OverwriteCB */


/**************************************************************************
 * DontOverWriteCB: continue file operation at next file
 *	This is called both by the "Don't Overwrite" button in the
 *	"Overwrite" warning popup and by the "Ok" button in the
 *	"Error" popup.
 **************************************************************************/
static void	
DontOverwriteCB (Widget widget, XtPointer client_data, XtPointer call_data)
{		
    FileOpPtr	file_op = (FileOpPtr) client_data;

    BringDownPopup(_OlGetShellOfWidget (widget));	

    /* Skip Current File Operation */
    file_op->cur_file++;
    /* Now continue file Operation */
    ContinueFileOp(file_op);
}	/* end of DontOverwriteCB */

/**************************************************************************
 * OverWriteHelpCB: 
 **************************************************************************/
static void	
OverwriteHelpCB (Widget w, XtPointer client_data, XtPointer call_data)
{		
    FileOpPtr	file_op = (FileOpPtr) client_data;
    static DtDisplayHelpRequest req;

    req.rqtype         = DT_DISPLAY_HELP;
    req.serial         = 0;
    req.version        = 1;
    req.client         = XtWindow(w);
    req.nodename       = NULL;
    req.source_type    = DT_SECTION_HELP;
    req.app_name       = "MediaMgr";
    req.app_title      = NULL;
    req.title          = NULL;
    req.help_dir       = NULL;
    req.file_name      = "DesktopMgr/folder.hlp";
    req.sect_tag       = "Overwrite Notice Window";
    
   (void)DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(XtDisplay(w)),
      _HELP_QUEUE(XtDisplay(w)), XtWindow(w), (DtRequest *)&req);

}	/* end of DontOverwriteCB */

/**************************************************************************
 * FreeFileOp: free file operation structure
 *
 *
 *
 **************************************************************************/
void
FreeFileOp(FileOpPtr file_op)
{
    int i;

    XtAddTimeOut((ulong)1000, (XtTimerCallbackProc)SyncFolder, (XtPointer)file_op->destination);
    
    for (i=0; i<file_op->num_files; i++){
	XtFree(file_op->src_files[i]);
	XtFree(file_op->dest_files[i]);
    }
    XtFree((char *) file_op->src_files);
    XtFree((char *) file_op->dest_files);
    XtFree(file_op->device);
    XtFree(file_op->fstype);
    XtFree((char *) file_op);
}	/* end of FreeFileOp */

/**************************************************************************
 * PopdownCB: Destroy widget and free gizmo used for modal message
 *
 **************************************************************************/
static void
PopdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Gizmo modal = (Gizmo) client_data;
    
    XtDestroyWidget(widget);

}	/* end of PopdownCB */

/**************************************************************************
 * DestroyCB: Destroy widget and free gizmo used for modal message
 *
 **************************************************************************/
static void
DestroyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Gizmo modal = (Gizmo) client_data;

    FreeGizmo(ModalGizmoClass, modal);

}	/* end of DestroyCB */

/**************************************************************************
 * SyncFolder:   Ask DTM to sync mount point directory in case sync timer ran
 * as we created a file.
 *
 **************************************************************************/
static void
SyncFolder(XtPointer client_data, XtIntervalId *id) 
{

    DtSyncFolderRequest	req;

    req.path = (char *)client_data;
    req.rqtype = DT_SYNC_FOLDER;

    (void)DtEnqueueRequest(XtScreen(w_toplevel), _DT_QUEUE(XtDisplay(w_toplevel)),
      _DT_QUEUE(XtDisplay(w_toplevel)), XtWindow(w_toplevel), (DtRequest *)&req);

   FREE(client_data);
}

/* 
 * MntCloseCB:  Called from the Close button of the  Busy-mount-point popup.
 */
void	MntCloseCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	/* attempt once more to unmount */
	chdir ("/");
	if (DtamUnMount(_dtam_mntpt) != 0) {
		/* 
		 * if the unmount was unsuccessful,
		 * sync one last time before exiting
		 */
		sync();
		_DtamUnlink();
		exit(1);
	} else {
		_DtamUnlink();
		exit(0);
	}
}
