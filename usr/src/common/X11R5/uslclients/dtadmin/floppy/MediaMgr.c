#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/MediaMgr.c	1.13.2.53"
#endif
/*
 *	UNIX Desktop Removable Media (Diskette/Tape) Manager
 *
 *	handles mounting/unmounting, formatting and switching to other
 *	dtadmin facilities (backup/restore, package installation, etc.)
 */
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "media.h"

#define FMT_ATOM_PFX		"DtFmt_"
#define RST_ATOM_PFX		"DtRst_"

Boolean	show_full_warn = False;

enum { Backup_Mode, Restore_Mode, UnixFS_Mode, Format_Mode };

static void	SetOwner (char *atomName, char **file_list);
static Boolean	DropNotify (Widget w, Window win, Position x, Position y, 
			    Atom selection, Time timestamp, 
			    OlDnDDropSiteID drop_site_id, 
			    OlDnDTriggerOperation op, 
			    Boolean send_done, Boolean forwarded,
			    XtPointer closure);
static void	SelectionCB (Widget w, XtPointer client_data,
			     XtPointer call_data);
static void	ContCB (Widget w, XtPointer client_data, XtPointer call_data);
static Bool	TrapMapOrVis(Display *, XEvent *, XPointer);
static void	MedDispatchEvent(Widget, Widget);
static void	WaitNotice(void);
static void	CloseWaitNotice(void);
void	McontCB(Widget, XtPointer, XtPointer);
void	MDelCB(Widget, XtPointer, XtPointer);
void	ShowWarn();

char		*cur_file = NULL;
char		*curdev	= NULL;			/* these maintain the current */
char		*curalias = NULL;		/* getdev line and its alias. */
char		*bkup_alias = NULL;		
char		*rst_alias = NULL;	

Widget		w_toplevel;
Widget		w_msg;
Display		*theDisplay;
Screen		*theScreen;
Dimension	x3mm, y3mm;
XFontStruct	*def_font, *bld_font;
Arg		arg[12];
static int	Mode;
static Boolean 	waitModal_up = False;

char	*tfadminPkg = "/sbin/tfadmin";
Boolean	packageOwner;
char	*CpioCmd;		/* set in main() used by backup & restore */
char	*NDSCmd = "/sbin/tfadmin nds"; /* used by backup & restore */


XtAppContext	App_con;
char	*save_command[1];
int	save_count = 1;
int	backup_flag = 0;
int	restricted_flag = 0;
ExitValue Restricted_exit_val  = NoAttempt;
char	*MBtnFields[] = {XtNlabel, XtNmnemonic,
			 XtNsensitive, XtNselectProc, XtNpopupMenu };
char	*ExclFields[] = {XtNlabel, XtNmnemonic, XtNset, XtNsensitive };

void	exitCB();
void	retryCB();
void	helpCB();
void	SelectAction();
char	*GenericMsg();

extern BaseWindowGizmo rbase;
extern BaseWindowGizmo bbase;
extern BaseWindowGizmo dbase;
static void FreezeMinWindowSize(Widget shell);

static	HelpInfo DHelpIntro	= { 0, "", DHELP_PATH, help_intro };
static	HelpInfo FHelpIntro	= { 0, "", DHELP_PATH, help_intro };
static	HelpInfo THelpIntro	= { 0, "", THELP_PATH, help_intro };
static	HelpInfo CDHelpIntro	= { 0, "", CDHELP_PATH, help_intro };

static MenuItems note_menu_item[] = {  
	{ TRUE, label_continue,mnemonic_continue, 0, retryCB, NULL },
	{ TRUE, label_cancel,mnemonic_cancel, 0, exitCB, NULL },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB, (char *)&DHelpIntro },
	{ NULL }
};
static MenuGizmo note_menu = {0, "note_menu", NULL, note_menu_item};
PopupGizmo note = {0, "popup", NULL, (Gizmo)&note_menu};

MountInfo Minfo;

MenuItems mount_item[] = {
	{ TRUE, label_ok, mnemonic_ok, 0, McontCB, (char *)&Minfo},
	{ TRUE, label_delWarn, mnemonic_delWarn, 0, MDelCB, (char *)&Minfo},
	{ NULL }
};

MenuGizmo mount_menu = {0, "Mount_menu", NULL, mount_item };
ModalGizmo mount_warn = {0, "mount_warn", NULL, 
		(Gizmo)&mount_menu };

static  ModalGizmo waitModal = {0, "wait", NULL, NULL };

static MenuItems ShortNote_menu_item[] = {  
	{ TRUE, label_continue,mnemonic_continue, 0, ContCB, NULL },
	{ NULL }
};
static MenuGizmo ShortNote_menu = {0, "ShortNote_menu", NULL,
				       ShortNote_menu_item};
PopupGizmo ShortNote = {0, "popup", NULL, (Gizmo)&ShortNote_menu};


/* resources for the MediaMgr */
struct MMOptions MMOptions;

static XtResource resources[] = {
	{ XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
          MM_FONTLIST_OFFSET, XtRImmediate, NULL}
};


void	helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	help->app_title	= 
	help->title	= DtamDevAlias(curdev);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(note.shell, help);
}

void	exitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	_DtamUnlink();
	if (restricted_flag)
	    exit((int)Restricted_exit_val);
	else
	    exit((int) client_data);
}

void	ContCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(_OlGetShellOfWidget (wid));
}

void	retryCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int	diagn;
	char	*msg;
	char	**drop_list = (char **)client_data;

	diagn = DtamCheckMedia(curalias);
	switch (diagn) {

	case DTAM_NO_DISK:
	case DTAM_NOT_OWNER:
	case DTAM_DEV_BUSY:
	case DTAM_UNREADABLE:	
		msg = GenericMsg(diagn);
		XtSetArg(arg[0],  XtNstring, (XtPointer)msg);
		XtSetValues(w_msg, arg, 1);
		return;
	default:
		BringDownPopup(note.shell);
		SelectAction(drop_list ? DO_COPY: DO_OPEN, diagn, curalias,drop_list);
		return;
	}
}

char	*FmtInsertMsg(button_label)
	char	*button_label;
{
static	char	insert_str[80];
	char	*str;

	if ((str = DtamDevAttrTxt(curdev,"volume")) == NULL)
		str = STRDUP(GetGizmoText(string_genMedia));
	sprintf(insert_str, GetGizmoText(string_ins2Msg), str, button_label);
	return insert_str;
}

char	*GenericMsg(diagnostic)
	int	diagnostic;
{
	char	      *str, *drive;
	static char    buf[128];

	drive = DtamDevAlias(curdev);
	switch (diagnostic) {

	case DTAM_CANT_MOUNT:	sprintf(buf, GetGizmoText(string_mountErr),
							drive);
				str = buf;
				break;
	case DTAM_CANT_OPEN:	sprintf(buf, GetGizmoText(string_cantOpen),
							drive);
				str = buf;
				break;
	case DTAM_NO_ROOM:	sprintf(buf, GetGizmoText(string_noRoom2),
							drive);
				str = buf;
				break;
	case DTAM_NOT_OWNER:	str = GetGizmoText(string_notOwner);
				break;
	case DTAM_UNKNOWN:
	case DTAM_DEV_BUSY:
	case DTAM_UNREADABLE:
	case DTAM_UNFORMATTED:	if ((str=DtamDevAttrTxt(curdev,"volume")) == NULL)
					str = GetGizmoText(string_genMedia);
				if (strncmp(curdev,DISKETTE,strlen(DISKETTE))!=0)
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, "");
				else
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, drive+strlen(drive)-1);
				str = buf;
				break;
		default:	str = FmtInsertMsg(GetGizmoText(label_continue));
				break;
	}
	FREE(drive);
	return str;
}

DevItem		Devs[N_DEVS];

void	SetCaptionCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char		*label;
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*tmpalias;

	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);
	curalias = DtamMapAlias(label=Devs[olcd->item_index].label);
	if (curdev)
		FREE(curdev);
	tmpalias = STRDUP(curalias);
	curdev = DtamGetDev(strcat(tmpalias, ":"),FIRST);
	FREE(tmpalias);
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_ud, arg, 1);
	XtSetArg(arg[0], XtNlabel, label);
	XtSetValues(XtParent(w_ud), arg, 1);

	if ( strstr(curalias, "cdrom") != 0 ) 
	   	DHelpIntro = CDHelpIntro;
	else if ( strstr(curalias, "disk") != 0 ) 
	   	DHelpIntro = FHelpIntro;
	else if ( strstr(curalias, "tape") != 0 ) 
	   	DHelpIntro = THelpIntro;
}

void
WaitNotice()
{
	ModalGizmo	*waitGizmo = &waitModal;
	char	*label = DtamDevAlias(curdev);
	static char    buf[128];

	if (!waitGizmo->shell) {
		waitGizmo->title = STRDUP(label);
		XtSetArg(arg[0], XtNnoticeType, OL_INFORMATION);
		CreateGizmo(w_toplevel, ModalGizmoClass, waitGizmo, arg, 1);
	}

	sprintf(buf, GetGizmoText(please_wait), label);
	SetModalGizmoMessage(waitGizmo, buf);
	MapGizmo(ModalGizmoClass, waitGizmo);
	waitModal_up = True;
}

void CloseWaitNotice()
{
	if (waitModal.shell) {
		if (waitModal_up) {
			XtPopdown(waitModal.shell);
			waitModal_up = False;
		}
	}
}

void
BaseNotice(int diagn, Boolean brief, char **drop_list)
{
static	Widget	w_desc;
static	Widget	w_smsg;
	Widget	w_up, w_devmenu, w_abvbtn;
	PopupGizmo	*noteGizmo;
	char	*label = DtamDevAlias(curdev);
	MenuGizmo *n_menu;

	noteGizmo = (brief) ? &ShortNote : &note;

	if (!noteGizmo->shell) {
		noteGizmo->title = label;
		CreateGizmo(w_toplevel, PopupGizmoClass, noteGizmo, NULL, 0);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(noteGizmo->shell, arg, 1);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(noteGizmo->shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNvPad,		y3mm*3);
		XtSetArg(arg[4], XtNvSpace,		y3mm*2);
		XtSetArg(arg[5], XtNhPad,		x3mm*3);
		XtSetArg(arg[6], XtNhSpace,		x3mm*2);

		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNalignment,	OL_CENTER);
		XtSetArg(arg[1], XtNgravity,	CenterGravity);
		XtSetArg(arg[2], XtNwidth,	32*x3mm);
		XtSetArg(arg[3], XtNfont, 	bld_font);

		if (!brief)
		{
		    if ( strstr(curalias, "cdrom") != 0 ) 
		   	DHelpIntro = CDHelpIntro;
		    else if ( strstr(curalias, "disk") != 0 ) 
		   	DHelpIntro = FHelpIntro;
		    else if ( strstr(curalias, "tape") != 0 ) 
		   	DHelpIntro = THelpIntro;

		    w_msg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);
		    w_desc = DevMenu(Devs, 0, N_DEVS, w_up, curalias,
				(XtPointer)SetCaptionCB,
				 "removable=\"true", &w_devmenu, &w_abvbtn, 0);
		}
		else
		    w_smsg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);
	}

	if (!brief)
	{
		XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
		XtSetValues(w_desc, arg, 1);

		XtSetArg(arg[0], XtNlabel, label);
		XtSetValues(XtParent(w_desc), arg, 1);

		n_menu = QueryGizmo (PopupGizmoClass, noteGizmo,
                            GetGizmoGizmo, "note_menu");
		n_menu->items[0].client_data = (char *)drop_list;
	}

	XtSetArg(arg[0], XtNstring, GenericMsg(diagn));
	XtSetValues((brief) ? w_smsg : w_msg, arg, 1);

	MapGizmo(PopupGizmoClass, noteGizmo);
}

/* SetOwner -- Attempt to become the "owner" of the device.  If the device
 * is already owned by another manifestation of the media manager, then
 * send a message to it to do the work; die after the message is acknowledged.
 */
static void
SetOwner (char *atomName, char **file_list)
{
    static Window	owner;

    /* Check if we are already the owner */
    if (owner)
	return;

    XtSetArg(arg[0], XtNmappedWhenManaged, FALSE);
    XtSetArg(arg[1], XtNheight, y3mm);
    XtSetArg(arg[2], XtNwidth,  x3mm);
    XtSetValues(w_toplevel, arg, 3);
    XtRealizeWidget(w_toplevel);

    OlDnDRegisterDDI(w_toplevel, OlDnDSitePreviewNone, DropNotify,
		     (OlDnDPMNotifyProc) 0, True, (XtPointer) &Mode);

    owner = DtSetAppId (XtDisplay (w_toplevel), XtWindow (w_toplevel),
			atomName);

    if (owner != None)
    {

	if (!file_list){
		file_list = (char **)malloc(2 * sizeof(char *));
		file_list[0] = "";
		file_list[1] = NULL;
	}
	if (DtNewDnDTransaction(w_toplevel, file_list,
				DT_B_SEND_EVENT | DT_B_STATIC_LIST,
				0, 0, CurrentTime, owner, DT_COPY_OP,
				NULL, (XtCallbackProc) exitCB, 0))
	{
	    /* Another MediaMgr is running, and we successfully sent a
	     * message to it.  Wait for death.
	     */
	    XtAppMainLoop(App_con);
	}
    }

    /* Either we are now the owner, or our message failed.  In either event,
     * just continue on doing whatever we would normally do as if we are owner.
     */
    owner = XtWindow (w_toplevel);
}

/* SelectionCB
 *
 * Called when the file list is ready after a drag and drop operation.
 * client_data is a pointer to the operation to perform; call_data is
 * DtDnDInfoPtr drop information.
 */
static void
SelectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    int				*mode  = (int *) client_data;
    DtDnDInfoPtr		dip = (DtDnDInfoPtr) call_data;
    int				i;
    extern BaseWindowGizmo	rbase;
    extern BaseWindowGizmo	fbase;

    if (dip->error)
	return;

    switch (*mode) {
    case Backup_Mode:
	BackupTakeDrop (w, client_data, call_data);
	break;
    case Restore_Mode:
	XMapWindow (theDisplay, XtWindow (rbase.shell));
	XRaiseWindow (theDisplay, XtWindow (rbase.shell));
	break;
    case Format_Mode:
	XMapWindow (theDisplay, XtWindow (fbase.shell));
	XRaiseWindow (theDisplay, XtWindow (fbase.shell));
	break;
    case UnixFS_Mode:
	UnixTakeDrop (w, client_data, call_data);
	break;
    }
}	/* End of SelectionCB () */

/* DropNotify
 *
 * Called by a pseudo-drop event on the toplevel window.  closure indicates
 * what operation we ought to do on the dropped files.
 */
static Boolean
DropNotify (Widget w, Window win, Position x, Position y, Atom selection,
            Time timestamp, OlDnDDropSiteID drop_site_id,
            OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded, 
            XtPointer closure)
{
    DtGetFileNames (w, selection, timestamp, send_done, SelectionCB, closure);

    return(True);
}	/* End of DropNotify () */

void
MDelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[BUFSIZ], *full_path;
	struct	passwd	*getpwuid(), *pwuid;
	uid_t	getuid(), uid;
	int	mfile;

	if ((full_path = getenv("HOME")) == NULL) {
		uid = getuid();
		pwuid = getpwuid(uid);
		full_path = STRDUP(pwuid->pw_dir);
	}
	if (show_full_warn)
		sprintf(buf, "%s/%s", full_path, ".fullsystem");
	else if (strstr(curalias, "disk") != 0)
		sprintf(buf, "%s/%s", full_path, ".diskette");
	else
		sprintf(buf, "%s/%s", full_path, ".cdrom");

	mfile = creat(buf, (mode_t)0644);
	(void) close (mfile);	
	McontCB(wid, client_data, call_data);
}

void
McontCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	MountInfo *mountinfo = (MountInfo *)client_data;

	extern	XtIntervalId	disk_tid;
	FileOpPtr		drop_files;
	int			i;

	if (mount_warn.shell)
		BringDownPopup(mount_warn.shell);
	if (show_full_warn) {
		show_full_warn = False;
		return;
	}
	if (attempt_mount(mountinfo->diagn, curdev, &(mountinfo->fstype)) & DTAM_CANT_MOUNT)
		BaseNotice(DTAM_CANT_MOUNT, False, mountinfo->drop_list);
	else if (mountinfo->opflag != DO_COPY &&
		 attempt_open(curalias, True) & CANT_OPEN)
	{
		if (disk_tid)
			XtRemoveTimeOut(disk_tid);
		(void) DtamUnMount(_dtam_mntpt);
		BaseNotice(CANT_OPEN, False, mountinfo->drop_list);
	}

	if (mountinfo->opflag == DO_COPY) {
		/* attempt_open will be called when
		 * attempt_copy completes
		 */
		drop_files = (FileOpPtr)MALLOC(sizeof(FileOpRec));
		for (i=0; mountinfo->drop_list[i]; i++);  /* NULL-LOOP */
		drop_files->src_files = (char **)MALLOC(i * sizeof(char *));
		for ( i=0; mountinfo->drop_list[i]; i++ )
			drop_files->src_files[i] = STRDUP(mountinfo->drop_list[i]);
		drop_files->num_files = i;
		drop_files->device = STRDUP(curdev);
		drop_files->fstype = mountinfo->fstype;
		attempt_copy(drop_files);
	}

}

void
ShowWarn()
{
	int	n=0;
	char	*label = DtamDevAlias(curdev), *full_path;
	static	char	buf[BUFSIZ];
	struct	passwd	*getpwuid(), *pwuid;
	uid_t	getuid(), uid;

	if ((full_path = getenv("HOME")) == NULL) {
		uid = getuid();
		pwuid = getpwuid(uid);
		full_path = STRDUP(pwuid->pw_dir);
	}

	if (show_full_warn)
		sprintf(buf, "%s/%s", full_path, ".fullsystem");
	else if (strstr(curalias, "disk") != 0)
		sprintf(buf, "%s/%s", full_path, ".diskette");
	else
		sprintf(buf, "%s/%s", full_path, ".cdrom");

	if (access(buf, R_OK) == 0) { 
		/* the file exists, don't display warning */
		McontCB(w_toplevel, &Minfo, NULL);		
		return;
	}

	/* always display warning when the file is not there */

	if (!mount_warn.shell) {
		XtSetArg(arg[n], XtNnoticeType, OL_WARNING); n++;
		mount_warn.title = STRDUP(GetGizmoText(string_WarnTitle));
		CreateGizmo(w_toplevel, ModalGizmoClass, &mount_warn, arg, n);
	} 

	if (show_full_warn)
		sprintf(buf, GetGizmoText(string_fullWarn));
	else if (strstr(curalias, "disk") != 0)
		sprintf(buf, GetGizmoText(string_fmountWarn), label);
	else
		sprintf(buf, GetGizmoText(string_cmountWarn), label);

	SetModalGizmoMessage(&mount_warn, buf);
	XtVaSetValues(mount_warn.stext, XtNalignment, (XtArgVal)OL_LEFT,
		NULL);
	MapGizmo(ModalGizmoClass, &mount_warn);
}

void	SelectAction(int opflag, int diagn, char *atomName, char **drop_list)
{
	char			*newAtom;
	extern XtIntervalId	disk_tid;
	char			*fstype = NULL;
	FileOpPtr		drop_files;
	int			i;

	switch (opflag) {
	case DO_BACKUP:	Mode = Backup_Mode;
			SetOwner (atomName, drop_list);
			CreateBackupWindow(w_toplevel, drop_list, atomName);
			FreezeMinWindowSize(bbase.shell);
			return;
	case DO_FORMAT:	
			Mode = Format_Mode;
			if (!backup_flag)
			{
				newAtom = XtMalloc (strlen (FMT_ATOM_PFX) +
						    strlen (atomName) + 1);
				strcpy (newAtom, FMT_ATOM_PFX);
				strcat (newAtom, atomName);
				SetOwner (newAtom, drop_list);
			}
			CreateFormatWindow(w_toplevel);
			return;
	case DO_RESTOR:	newAtom = XtMalloc (strlen (RST_ATOM_PFX) +
					    strlen (atomName) + 1);
			strcpy (newAtom, RST_ATOM_PFX);
			strcat (newAtom, atomName);
			Mode = Restore_Mode;
			SetOwner (newAtom, drop_list);
			CreateRestoreWindow(w_toplevel, atomName);
			FreezeMinWindowSize(rbase.shell);
			return;
	}
	if (diagn == -1) {
		WaitNotice();
		MedDispatchEvent(waitModal.shell, waitModal.stext);
		diagn = DtamCheckMedia(curalias);
	}
	if (opflag != DO_COPY &&
	    (diagn&DTAM_PACKAGE || diagn&DTAM_INSTALL || diagn==DTAM_CUSTOM)) {

		if (packageOwner)
			execlp (tfadminPkg, tfadminPkg,
				"PackageMgr", "-D", curalias, NULL);
		else {
		        extern char	*GetXWINHome();
			char 		*packager;

			packager = GetXWINHome ("bin/PackageMgr");
			execlp (packager, packager, "-D", curalias);
		}
	}
	else if (diagn & DTAM_FS_TYPE) {
		if (_DtamIsOwner(OWN_FLOPPY)) {
			Mode = UnixFS_Mode;
			SetOwner (atomName, drop_list);

			Minfo.opflag = opflag;
			Minfo.diagn = diagn;
			Minfo.fstype = fstype;
			Minfo.drop_list = drop_list;
	
			ShowWarn();
		}
		else
			BaseNotice(diagn = DTAM_NOT_OWNER, False, drop_list);
	}
	else {
		switch(diagn) {
	case DTAM_TAR:
	case DTAM_BACKUP:
	case DTAM_CPIO:
	case DTAM_CPIO_BINARY:
	case DTAM_CPIO_ODH_C:	if (opflag != DO_COPY) {
					newAtom =
					    XtMalloc (strlen (RST_ATOM_PFX) +
					    strlen (atomName) + 1);
					strcpy (newAtom, RST_ATOM_PFX);
					strcat (newAtom, atomName);
					Mode = Restore_Mode;
					SetOwner (newAtom, drop_list);
					CreateRestoreWindow(w_toplevel, atomName);
					FreezeMinWindowSize(rbase.shell);
					CloseWaitNotice();
					return;
				}
				/* else fall through to backup cases */
	case DTAM_UNFORMATTED:	if (opflag != DO_COPY) {
					newAtom =
					    XtMalloc (strlen (FMT_ATOM_PFX) +
					    strlen (atomName) + 1);
					strcpy (newAtom, FMT_ATOM_PFX);
					strcat (newAtom, atomName);
					Mode = Format_Mode;
					SetOwner (newAtom, drop_list);
					CreateFormatWindow(w_toplevel);
					CloseWaitNotice();
					return;
				}
				/* else fall through to backup cases */
	case DTAM_NO_DISK:	if ( strstr(curalias, "cdrom") != 0 ) {
					BaseNotice(DTAM_NO_DISK, False, drop_list);
					CloseWaitNotice();
					return;
				}
				/* else fall through to backup cases */
	default:
	case DTAM_UNDIAGNOSED:
	case DTAM_UNKNOWN:	if ( strstr(curalias, "cdrom") != 0 ) {
					BaseNotice(DTAM_UNKNOWN, False, drop_list);
					CloseWaitNotice();
					return;
				}

				Mode = Backup_Mode;
				SetOwner (atomName, drop_list);
				CreateBackupWindow(w_toplevel, drop_list, atomName);
				FreezeMinWindowSize(bbase.shell);
				CloseWaitNotice();
				return;
		}
	}
	CloseWaitNotice();
}

void	main(argc, argv)
	int	argc;
	char	*argv[];
{
extern	char	*optarg;

	char	*atomName, *tmpalias;
	int	opflag = 0;
	int	n, m, i;
	char	**drop_list = NULL;

#ifdef MEMUTIL
	InitializeMemutil();
#endif

        (void)setsid();		/* become a session leader (divorce dtm) */

        /* undo some of the stuff we inherit from dtm */
        sigset(SIGCHLD, SIG_DFL);
        sigset(SIGINT,  SIG_DFL);
        sigset(SIGQUIT, SIG_DFL);
        sigset(SIGTERM, SIG_DFL);

	for (m = n = 0; n < argc; n++)
		m += strlen(argv[n])+1;
	*save_command = (char *)CALLOC(m+1,sizeof(char));
	for (n = 0; n < argc; n++)
		strcat(strcat(*save_command,argv[n])," ");
	OlToolkitInitialize(&argc, argv, NULL);

#ifdef UseXtApp
	w_toplevel = XtAppInitialize(
			&App_con,		/* app_context_return	*/
			"MediaMgr",		/* application_class	*/
			(XrmOptionDescList)NULL,/* options		*/
			(Cardinal)0,		/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String)NULL,		/* fallback_resources	*/
			(ArgList)NULL,		/* args			*/
			(Cardinal)0		/* num_args		*/
	);
#else
	w_toplevel = XtInitialize("MediaMgr", "MediaMgr", NULL, 0, &argc, argv);
        App_con = XtWidgetToApplicationContext(w_toplevel);
#endif

	/* This will get a font list */
	XtGetApplicationResources(w_toplevel, (XtPointer) &MMOptions,
				  resources, XtNumber(resources), NULL, 0);

	/*
	 *	note - this will not be realized UNLESS there is some
	 *	honest work to do in a base window (backup, restore, format
	 *	or package management.  Notices (e.g. to insert floppies)
	 *	and the monitoring of mounted devices can go on without an
	 *	irrelevant visible thingy on the screen.
	 */
	theDisplay = XtDisplay(w_toplevel);
	theScreen = XtScreen(w_toplevel);
	x3mm = OlMMToPixel(OL_HORIZONTAL,3);
	y3mm = OlMMToPixel(OL_VERTICAL,3);
	DtInitialize(w_toplevel);
	def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);
	bld_font = _OlGetDefaultFont(w_toplevel, OlDefaultNoticeFont);
	_DtamWMProtocols(w_toplevel);

/*
 *	analyze calling situation; if invoked with a specific flag, go to
 *	the appropriate main window, if by double-clicking on a specific
 *	device, diagnose this for its contents (if any) and again go the
 *	the appropriate window -- or if none is determinable, a notice with
 *	a diagnostic message displayed. 
 */
	while ((n = getopt(argc, argv, "BC:D:FXIO:RWL")) != EOF) {
                switch (n) {
			case 'B':	opflag = DO_BACKUP;
					break;
			case 'C':	/* receipt of dropped objects */
					if (!opflag)
						opflag = DO_COPY;
					drop_list = (char **)MALLOC(argc * sizeof(char *));
                                        drop_list[0] = STRDUP(optarg);
					for ( i=1; optind < argc; optind++) {
						drop_list[i] = 
						  (char *)MALLOC(strlen(argv[optind]) + 1);
						strcpy(drop_list[i++], argv[optind]);
					}

					drop_list[i] = NULL;
                                        break;
                        case 'D':       curalias = STRDUP(optarg);
					break;
			case 'F':	opflag = DO_FORMAT;
					break;
			case 'O':	/* "open" the arg (dropped file) */
					cur_file = STRDUP(optarg);
					break;
			case 'R':	opflag = DO_RESTOR;
					break;
			case 'X':	backup_flag = 1;
					break;
			case 'L':	restricted_flag = 1;
			                break;
			default:	break;
                }
        }
        if (system("/sbin/tfadmin -t cpio >/dev/null 2>&1") == 0)
	    CpioCmd = "/sbin/tfadmin cpio";
        else
	    CpioCmd = "/usr/bin/cpio";

        if (system("/sbin/tfadmin -t PackageMgr >/dev/null 2>&1") == 0)
	    packageOwner = True;
        else
	    packageOwner = False;

	if (!curalias) {
		atomName = "DtMedia";
		curdev = DtamGetDev("tape", FIRST);
		if (!curdev)
			curdev = DtamGetDev("diskette1", FIRST);
	}
	else {
		atomName = STRDUP(curalias);
		tmpalias = STRDUP(curalias);
		curdev = DtamGetDev(strcat(tmpalias, ":"), FIRST);
		FREE(tmpalias);
	}
	if (curdev) {
		char	*cdev = DtamDevAttr(curdev, CDEVICE);
		if (cdev && access(cdev, R_OK) == 0) {
			curalias = DtamDevAttr(curdev, ALIAS);
		}
		else {
			FREE(curdev);
			curdev = NULL;
			if (cdev)
				FREE(cdev);
		}
	}

	if (!curdev)
		curdev = DtamGetDev(curalias,FIRST);

	bkup_alias = STRDUP(curalias);
	rst_alias = STRDUP(curalias);
	
	SelectAction(opflag, -1, atomName, drop_list); 	/* go to appropriate base */

						/* window or give notice. */
	XtAppMainLoop(App_con);
}
/*
 * FreezeMinWindowSize
 *
 */

static void
FreezeMinWindowSize(Widget shell)
{
	Arg arg[4];
	Dimension width;
	Dimension height;

	XtSetArg(arg[0], XtNwidth, &width);
	XtSetArg(arg[1], XtNheight, &height);
	XtGetValues(shell, arg, 2);

#ifdef DEBUG
	(void)fprintf(stderr,"set min size: %dx%d\n", (int)width, (int)height);
#endif

	XtSetArg(arg[0], XtNminWidth, width);
	XtSetArg(arg[1], XtNminHeight, height);
	XtSetValues(shell, arg, 2);

} /* end of FreezeMinWindowSize */

/*
 * Simulate return to main loop - dispatch awating events.  Called
 * when a callback is going to perform a time-expensive task.
 */
static Bool
TrapMapOrVis(Display * dpy, XEvent * xe, XPointer client_data)
{
	if (xe->type == MapNotify || xe->type == VisibilityNotify) {
		Window		window = XtWindow((Widget)client_data);

		XSetInputFocus(
			xe->xany.display, window, RevertToNone, CurrentTime);
		return True;
	} else {
		return False;
	}
}

static void
MedDispatchEvent(Widget shell, Widget stext)
{
	XEvent	xe;

	/* Check the event queue for MapNotify/VisibilityNotify event. 
	 * XIfEvent will block until the expected event show up in the queue.
	 */
	XIfEvent(XtDisplay(stext), &xe, TrapMapOrVis, (XPointer)shell);

	/* If the event is found, then update the display (XSync and dispatch
	 * the Expose event.
	 */
	OlUpdateDisplay(stext);

} /* end of MedDispatchEvent */
