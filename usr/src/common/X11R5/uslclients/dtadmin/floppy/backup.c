#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/backup.c	1.8.5.57"
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <nws/nwbackup.h>
#include "media.h"

extern	void	InsertNotice();
extern	void	SetLocaleTags();
extern	char	*GetXWINHome();
extern	char	*nbfgets (char *buf, int cnt, FILE *pFile);
extern	void	remap_rst();

extern	Boolean	show_full_warn;
extern	long	_dtam_flags;
extern	char	*CpioCmd;
extern	char	*NDSCmd;
extern  ExitValue Restricted_exit_val;
extern  int	restricted_flag;
extern 	BaseWindowGizmo rbase;
extern	char	*bkup_alias;

extern	void    McontCB(Widget, XtPointer, XtPointer);
extern	void    MDelCB(Widget, XtPointer, XtPointer);
extern	void    ShowWarn();
extern MountInfo Minfo;
extern MenuItems mount_item[];
extern MenuGizmo mount_menu;
extern ModalGizmo mount_warn;

static	int	Blocks = 0;

static void	bhelpCB();
static void	bkillCB();
static void	gotoRestoreCB();
static void	openCB();
static void	saveCB();
static void	saveasCB();
static void	dosaveCB();
static void	cancelSaveCB();
static void	ucancelCB();
static void	doopenCB();
static void	cancelOpenCB();
static void	backupCB();
static void	startIndex();
static void	schedCB();
static void	testCB();
static void	excludeCB();
static void	BackupLaunchCB();
static void	ResetIconBox();
static void	MakeUserList();
static char    *BkupLine();
static void	startFormat(int);
static void	postFormat();
static void	postVolume();
static void	startCpio(int);
static void	CheckCpio();
static void	CheckNDS();
static void	CheckVolume();
static Boolean	BkupScript();
static void	BkRegisterHelp();
void		ShowNDSError();
void		ErrorNotice();
void		InfoNotice();
void		OKCB();
void		infoOKCB();
void		NotePidFiles();
static void	bokCB();
static void	bapplyCB();
static void	bwokCB();
static void	callBackup();

static MenuItems bfile_menu_item[] = {
	{ TRUE, label_gotoRes,  mnemonic_gotoRes, 0, gotoRestoreCB},
	{ TRUE, label_open,  mnemonic_open, 0, openCB},
	{ TRUE, label_save,  mnemonic_save, 0, saveCB},
	{ TRUE, label_saveas,mnemonic_saveas, 0, saveasCB},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems bbkup_menu_item[] = {
	{ TRUE, label_exclude,mnemonic_exclude, 0, excludeCB},
	{ NULL }
};


static MenuItems errnote_item[] = {
        { TRUE, label_ok,  mnemonic_ok, 0, OKCB },
        { NULL }
};

static  MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static  ModalGizmo errnote = {0, "warn", string_backupError,(Gizmo)&errnote_menu };

static HelpInfo BHelpBackup	= { 0, "", BHELP_PATH, help_intro };
static HelpInfo BHelpTOC	= { 0, "", BHELP_PATH, NULL };
static HelpInfo BHelpDesk	= { 0, "", BHELP_PATH, "HelpDesk"  };
static HelpInfo BHelpOpen	= { 0, "", BHELP_PATH, help_bkup_open };
static HelpInfo BHelpSave	= { 0, "", BHELP_PATH, help_bkup_save };
static HelpInfo BHelpConfirm	= { 0, "", BHELP_PATH, help_bkup_confirm };
static HelpInfo BHelpUsrLst	= { 0, "", BHELP_PATH, help_user_list };
static HelpInfo BHelpDoingBkup	= { 0, "", BHELP_PATH, help_bkup_doing };
static HelpInfo BHelpOverwrite	= { 0, "", BHELP_PATH, help_overwrite };
/* reuse the string for section 20 in help file */
static HelpInfo BHelpBkupFile	= { 0, "", BHELP_PATH, help_format };

static OlDtHelpInfo help_info[] = {NULL, NULL, BHELP_PATH, NULL, NULL}; 

static MenuItems infonote_item[] = {
	{ TRUE, label_ok, mnemonic_ok, 0, infoOKCB },
	{ TRUE, label_help, mnemonic_help, 0, bhelpCB, (char *)&BHelpBkupFile},
	{ NULL }
}; 

static MenuGizmo infonote_menu = {0, "info", "info", infonote_item };
static ModalGizmo infonote = {0, "info", string_infoTitle, (Gizmo)&infonote_menu };

static MenuItems bhelp_menu_item[] = {  
	{ TRUE, label_bkrst, mnemonic_bkrst, 0, bhelpCB, (char *)&BHelpBackup },
	{ TRUE, label_toc,   mnemonic_toc,   0, bhelpCB, (char *)&BHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, bhelpCB, (char *)&BHelpDesk },
	{ NULL }
};

static MenuGizmo bfile_menu = {0, "file_menu", NULL, bfile_menu_item};
static MenuGizmo bbkup_menu = {0, "bkup_menu", NULL, bbkup_menu_item};
static MenuGizmo bhelp_menu = {0, "help_menu", NULL, bhelp_menu_item};

static MenuItems bmain_menu_item[] = {
	{ TRUE, label_file,   mnemonic_file, (Gizmo) &bfile_menu},
	{ TRUE, label_edit, mnemonic_edit, (Gizmo) &bbkup_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &bhelp_menu},
	{ NULL }
};
static MenuGizmo bmenu_bar = { 0, "menu_bar", NULL, bmain_menu_item,
   				NULL, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM };

BaseWindowGizmo bbase = {0, "base", label_backup, (Gizmo)&bmenu_bar,
	NULL, 0, label_backup, "backup48.icon", " ", " ", 90 };

static MenuItems bwatch_menu_item[] = {  
	{ TRUE, label_cancel,  mnemonic_cancel, 0, bkillCB, NULL },
	{ TRUE, label_help,    mnemonic_help, 0,bhelpCB,(char *)&BHelpDoingBkup },
	{ NULL }
};
static MenuGizmo bwatch_menu = {0, "bwatch_menu", NULL, bwatch_menu_item};
static PopupGizmo bwatch = {0, "popup", title_doingBkup, (Gizmo)&bwatch_menu};

static MenuItems bnote_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, testCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, bkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB,(char *)&BHelpConfirm },
	{ NULL }
};
static MenuGizmo bnote_menu = {0, "bnote_menu", NULL, bnote_menu_item};
static ModalGizmo bnote = {0, "", title_confirmBkup, (Gizmo)&bnote_menu};

static MenuItems buser_menu_item[] = {  
	{ TRUE, label_ok, mnemonic_ok, 0, bokCB, NULL },
	{ TRUE, label_apply, mnemonic_apply, 0, bapplyCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, ucancelCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpUsrLst },
	{ NULL }
};

static MenuGizmo buser_menu = {0, "buser_menu", NULL, buser_menu_item};
static PopupGizmo buser = {0, "", title_bkupUsers, (Gizmo)&buser_menu};

static MenuItems bsave_menu_item[] = {  
	{ TRUE, label_save,   mnemonic_save, 0, dosaveCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelSaveCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpSave },
	{ NULL }
};
static MenuGizmo bsave_menu = {0, "bsave_menu", NULL, bsave_menu_item};
static FileGizmo save_prompt = {0, "", title_bkupSave, (Gizmo)&bsave_menu, NULL,
				"", NULL, FOLDERS_AND_FILES, NULL };

static MenuItems bopen_menu_item[] = {  
	{ TRUE, label_open,   mnemonic_open, 0, doopenCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelOpenCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, bhelpCB, (char *)&BHelpOpen },
	{ NULL }
};
static MenuGizmo bopen_menu = {0, "bopen_menu", NULL, bopen_menu_item};
static FileGizmo open_prompt = {0, "", title_bkupOpen, (Gizmo)&bopen_menu, NULL,
				"", NULL, FOLDERS_AND_FILES, NULL };

static MenuItems mounted_menu_item[] = {
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB, NULL},
	{ NULL }
	};
static MenuGizmo mounted_menu = {0, "mounted_menu", NULL, mounted_menu_item};
static ModalGizmo mounted = {0, "", title_mounted, (Gizmo)&mounted_menu};

static MenuItems in_use_menu_item[] = {
	{ TRUE, label_overwrite, mnemonic_overwrite, 0, BackupLaunchCB, NULL },
	{ TRUE, label_exit, mnemonic_exit, 0, exitCB, NULL},
	{ TRUE, label_help, mnemonic_help, 0, bhelpCB, (char *)&BHelpOverwrite },
	{ NULL }
	};
static MenuGizmo in_use_menu = {0, "in_use_menu", NULL, in_use_menu_item};
static ModalGizmo in_use = {0, "", title_in_use, (Gizmo)&in_use_menu};

extern	char	*HALT_tag;
extern	char	*ERR_tag;
extern	char	*WARN_tag;
extern	char	*NDS_ERR;
extern	char	*ERR_fld;
extern	char	*END_tag;

#define	ROOT		"/"
#define OWN_NDS "ndsbackup"

static char	IGNORE[]	= "Ignore";
static char	BKUP_HIST[]	= ".lastbackup";
static char	INCR_HIST[]	= ".lastpartial";
static char	BKUP_LOG[]	= "backuplog";
static char	XARGS[]		= "xargs -i find {}";

static  FILE	*log;

#define WIDTH	(45*x3mm)
#define	HEIGHT	(32*y3mm)

#define INIT_X  32
#define INIT_Y  24
#define INC_X   72
#define INC_Y   24
#define	MARGIN	24

#define	SHELL_LINE  "#!/bin/sh"
#define	MARKER_LINE "#Backup Script File - DO NOT EDIT"

static Dimension	ibx = INIT_X, iby = INIT_Y;

static  Widget	w_bkmsg, w_class, w_type, w_target, 
		w_icons, w_log, w_bkmenu, w_bdesc, w_devmenu;

static DmItemPtr	b_itp;
static DmContainerRec	b_cntrec;
static DmFclassRec	doc_fcrec, dir_fcrec, nds_fcrec;

static char	*bkup_cmd = NULL;
static char	*bkup_doc = NULL;
static char	*bdoc_alias;
static char	*user_home = NULL;
static char	*system_home = "/etc";
static char	*home;
static char	 flpindex[] = "/tmp/flp_index";
static char	*indexscript;
static pid_t	 child_pgid = 0;
char		*copy_source = NULL;
char		*bkup_source = NULL;
char		*EOM = "EoM %d";
char		 pid[48];

static _Dtam_inputProcData *volumeData;
char		*pidindex;
char		*ptyname;
FILE		*ptyfp;


#define	B_IMMEDIATE	0
#define	B_SCRIPT	1

#define	BKUP_COMPL	0
#define	BKUP_INCR	1
#define HERE_DOC_WORD	"ThIsShOuLdNotCoLiDeWiThAnYfIlEnAmE"

static const char *FindErrorOutput = "FindCommandGeneratedErrors";
static Boolean     FindErrors = FALSE;
static Boolean     CpioWarnings = FALSE;

static Boolean		save_log = FALSE;

static int		bkup_type = BKUP_COMPL;

/* order of Backup Data buttons */
#define	CLS_SELF	0
#define	CLS_FILES	1
#define	CLS_USERS	2
#define	CLS_SYSTEM	3
#define	CLS_NDS		4

static int		bkup_class = CLS_SELF;
static int		bkup_count = 0;
static char		*user = NULL;
static FileList	BkupTarget = NULL;	

static DevItem		BkupDevice[N_DEVS];

typedef struct _buffer {
    size_t  bufsize;	/* number of bytes allocated */
    char   *buf;	/* the buffer                */
    char   *next;	/* where to write next char  */
    char   *end;	/* last byte in buffer       */
    Boolean complete;	/* buffer ends in newline    */
} buffer_t;

typedef struct _bdata {
    int	      blocks;	/* total number of blocks to write */
    char     *indexfile;/* file to save index of files being backed up */
    FILE     *indexfp;	/* FILE ptr for indexfile          */
} bkupClient_t;

typedef	struct	{
	char	*u_name;
	char	*u_home;
} UserItem, *UserList;

static UserList	users = (UserList)NULL;
static int		user_count = 0;

static char		*UserFields[] = { XtNlabel, XtNuserData };

#define	BUNCH	32
#define	QUANTUM	(BUNCH*sizeof(UserItem))

char		*volume=NULL;
int		vol_count=0;


static char	*type_label[3];

void
InfoNotice(char *buf)
{
	int	n=0;
	if (!infonote.shell) {
		XtSetArg(arg[n], XtNnoticeType, OL_INFORMATION); n++;
		CreateGizmo(w_toplevel, ModalGizmoClass, &infonote, arg, n);
	}
	SetModalGizmoMessage(&infonote, buf);
	XtVaSetValues(infonote.stext, XtNalignment, (XtArgVal)OL_LEFT,
		NULL);
	MapGizmo(ModalGizmoClass, &infonote);
}

void
ErrorNotice (char *buf, char *title)
{
        if (!errnote.shell)
         CreateGizmo(w_toplevel, ModalGizmoClass, &errnote, NULL, 0);
         SetModalGizmoMessage(&errnote, buf);
         OlVaFlatSetValues(errnote_menu.child, 0, 
                      XtNclientData, (XtArgVal)0, 0);
         XtVaSetValues(errnote.stext, XtNalignment, (XtArgVal)OL_LEFT, NULL);
         if (title != NULL)
            XtVaSetValues(errnote.shell, XtNtitle, (XtArgVal)title, NULL);
         MapGizmo(ModalGizmoClass, &errnote);
}

void
infoOKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(infonote.shell);
}


void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{

    BringDownPopup(errnote.shell);
}


static void BkRegisterHelp()
{
      help_info->filename =  BHELP_PATH;
      help_info->title    =  GetGizmoText(label_backup);
      help_info->section = GetGizmoText(STRDUP(help_bkup_win));
      OlRegisterHelp(OL_WIDGET_HELP, bbase.shell, "MediaMgr", OL_DESKTOP_SOURCE,
                (XtPointer)&help_info);
}

static	char *
SummaryMsg(void)
{
struct	utsname	sysname;
static	char	*ptr, buf[128];
	char	*contents;

	if (bkup_class == CLS_SYSTEM) {
		uname(&sysname);
		contents = sysname.nodename;
	}
	else
		contents = bkup_source;

	ptr = curdev? DtamDevAlias(curdev): bkup_doc;

	if (bkup_class == CLS_NDS) 
		sprintf(buf, GetGizmoText(string_ndsBkupSumm), 
			ptr ? ptr : GGT(label_file));
	else if (bkup_class == CLS_FILES) 
		sprintf(buf, GetGizmoText(string_selBkupSumm), 
			ptr ? ptr : GGT(label_file));
        else if (bkup_type == BKUP_COMPL)
		sprintf(buf, GetGizmoText(string_complBkupSumm), 
			ptr ? ptr : GGT(label_file), contents);
        else /* (bkup_type == BKUP_INCR) */
		sprintf(buf, GetGizmoText(string_incrBkupSumm), 
			ptr ? ptr : GGT(label_file), contents);
            
        if ((ptr != NULL) && curdev)
	    FREE(ptr);
	return buf;
}

static void
insert_bnote(ModalGizmo *note, int diagn, _Dtam_inputProcData *inputData)
{
    OlVaFlatSetValues(note-> menu-> child, 0, XtNclientData,
		      (XtArgVal)inputData, 0);
    OlVaFlatSetValues(note-> menu-> child, 1, XtNclientData,
		      (XtArgVal)inputData, 0);
    InsertNotice(note, diagn);

}


static void
init_pty(_Dtam_inputProcData *bkupData)
{
    int	 fdmaster;

    if (curdev == NULL)		/* backing up to hard disk file */
	ptyfp = (FILE *)NULL;
    else
    {
	fdmaster = open("/dev/ptmx", O_RDWR);
	grantpt(fdmaster);
	unlockpt(fdmaster);
	ptyname = ptsname(fdmaster);
	fcntl(fdmaster, F_SETFL, O_NONBLOCK);
	ptyfp = fdopen(fdmaster, "r+");
	setbuf(ptyfp, NULL);
	volumeData = (_Dtam_inputProcData *)
	    MALLOC(sizeof(_Dtam_inputProcData));
	volumeData-> appContext = App_con;
	volumeData-> fp[1]	= ptyfp;
	volumeData-> readId  	= XtAppAddInput(App_con, fdmaster,
						(XtPointer)XtInputReadMask,  
						CheckVolume,
						(XtPointer)bkupData); 
	volumeData-> exceptId	= XtAppAddInput(App_con, fdmaster,
						(XtPointer)XtInputExceptMask,  
						CheckVolume,
						(XtPointer)bkupData); 
    }
    return;
}

static void
backupAck()
{
    /* tell cpio/ndsbackup we've inserted the next volume */

    if (vol_count > 1)
    {
	fputs("\n", ptyfp);
	fflush(ptyfp);
    }
}


 /* read from an open file pointer into a dynamicly growing buffer.  */
 /* Designed to read full lines from descriptors that might not have */
 /* the full line available. returns number of chars read or -1 if a */
 /* malloc error occurs. */

int
getbuf(buffer_t *buffer, FILE *fp)
{
    char  *ptr;
    size_t len;

#define MINIMUM_BUF 20		/* 20 is arbitrary */

    if (buffer-> buf == NULL)	/* first-time */
    {
	if ((buffer-> buf  = MALLOC(BUFSIZ)) == NULL)
	    return -1;
	buffer-> next = buffer-> buf;
	buffer-> end  = buffer-> buf + BUFSIZ;
	buffer-> bufsize  = BUFSIZ;
	buffer-> complete = FALSE;
    }
    else if ((buffer-> end - buffer-> next) < MINIMUM_BUF)
    {                         /* buf almost full; allocate more*/
	buffer-> bufsize += BUFSIZ;
	if ((buffer-> buf =
	     REALLOC(buffer-> buf, buffer-> bufsize)) == NULL) 
	{
	    buffer-> bufsize -= BUFSIZ;
	    return -1;
	}
	buffer-> end += BUFSIZ;
    }

    ptr = fgets(buffer-> next, buffer-> end - buffer-> next, fp);
    if (ptr == NULL)
	return 0;
    len = strlen(buffer-> next);
    if (buffer-> next[len-1] == '\n')
    {
	buffer-> next = buffer-> buf;
	buffer-> complete = TRUE;
    }
    else
    {
	buffer-> next += len;
	buffer-> complete = FALSE;
    }
    return len;
}


static	void
bhelpCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(bbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_backup);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(bbase.shell, help);
}

static	void
DropBkupWatch(XtPointer closure, XtIntervalId id)
{

    if (bwatch.shell && !closure ) {
	/* in case there is no error on backup, we just leave the 
	 * message box up, relabel the cancel button to ok button.
	 * So user can press ok to popdown the window.
	 */
	OlVaFlatSetValues(bwatch_menu.child, 0, 
		XtNlabel, GetGizmoText(label_ok),
		XtNmnemonic, *GetGizmoText(mnemonic_ok),
		XtNselectProc, bwokCB,
		NULL);
    }	
    if (bwatch.shell)
    	XDefineCursor(theDisplay, XtWindow(bwatch.shell),
		  GetOlStandardCursor(theScreen));
    if (w_gauge)
    	XDefineCursor(theDisplay, XtWindow(w_gauge),
		  GetOlStandardCursor(theScreen));
    XDefineCursor(theDisplay, XtWindow(bbase.shell),
		  GetOlStandardCursor(theScreen));
    if (bnote.shell)
	BringDownPopup(bnote.shell);
    if (closure)
	if (bwatch.shell)
		BringDownPopup(bwatch.shell);
}

static	void
StartBkupWatch(_Dtam_inputProcData *inputData)
{
	Widget	w_up;

	if (!bwatch.shell) {
		CreateGizmo(bbase.shell, PopupGizmoClass, &bwatch, NULL, 0);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(bwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(bwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNwidth,		36*x3mm);
		XtSetArg(arg[4], XtNhPad,		x3mm);
		XtSetArg(arg[5], XtNvPad,		y3mm);
		XtSetArg(arg[6], XtNvSpace,		y3mm);

		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNalignment,	 OL_CENTER);
		XtSetArg(arg[1], XtNgravity,	 CenterGravity);
		XtSetArg(arg[2], XtNwidth,	 32*x3mm);
		XtSetArg(arg[3], XtNfont, 	 bld_font);

		w_bkmsg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 2*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNspan,		32*x3mm);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetArg(arg[2], XtNorientation,	OL_HORIZONTAL);
		XtSetArg(arg[3], XtNminLabel,		" 0 %");
		XtSetArg(arg[4], XtNmaxLabel,		"100 %  ");
		XtSetArg(arg[5], XtNsliderMax,		100);
		XtSetArg(arg[6], XtNsliderValue,	0);
		XtSetArg(arg[7], XtNshowValue,		TRUE);
		XtSetArg(arg[8], XtNtickUnit,		OL_PERCENT);
		XtSetArg(arg[9], XtNticks,		10);

	 	w_gauge = XtCreateManagedWidget("gauge",
				gaugeWidgetClass, w_up, arg, 10);
	}
	else {
		/* On the second time comes here, we need to restore
		 * the button to cancel since user has pressed ok before.
		 */
		OlVaFlatSetValues(bwatch_menu.child, 0, 
			XtNlabel, GetGizmoText(label_cancel),
			XtNmnemonic, *GetGizmoText(mnemonic_cancel),
			XtNselectProc, bkillCB,
			0);
	}
	XtSetArg(arg[0],  XtNstring, GetGizmoText(string_waitIndex));
	XtSetValues(w_bkmsg, arg, 1);
	OlVaFlatSetValues(bwatch_menu.child, 0, 
		XtNclientData, (XtArgVal)inputData, 
		0);

	MapGizmo(PopupGizmoClass, &bwatch);
	XDefineCursor(theDisplay, XtWindow(bwatch.shell),
					GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(w_gauge),
					GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(bbase.shell),
					GetOlBusyCursor(theScreen));
}

static	void
ucancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(buser.shell);
}

static void
bwokCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(bwatch.shell);
	OlVaFlatSetValues(bwatch_menu.child, 0, 
		XtNlabel, GetGizmoText(label_cancel),
		XtNmnemonic, *GetGizmoText(mnemonic_cancel),
		XtNselectProc, bkillCB,
		NULL);
}

static	void
bkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    char                 buf[BUFSIZ];
    _Dtam_inputProcData *inputData = (_Dtam_inputProcData *)client_data;

    if (inputData)
	_Dtam_p3closeInput(inputData, SIGTERM);
    if (ptyfp) 
	postVolume(NULL, 0, NULL);
    sprintf(buf, GetGizmoText(string_bkupKilled));
    if (log) {
	fprintf(log, "%s\n", buf);
	fclose(log);
	log = (FILE *)NULL;
    }
    if (w_gauge) {
	XtSetArg(arg[0], XtNsliderValue,	0);
	XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
	XtSetValues(w_gauge, arg, 2);
    }
    FooterMsg(bbase, buf);
    XtVaSetValues(w_bkmsg, XtNstring, buf, NULL);
    DropBkupWatch((XtPointer)NULL, (XtIntervalId)NULL);
}

static	void
OpenFile(void)
{
    FILE	 *fp;
    char	 *ptr, *ptr2, *source, buf[BUFSIZ], *end, *target=NULL, *desc;
    int           Cset = 0, Tset = 0, Hset = 0, Sset = 0, i;
    Boolean       allSet = False;
    Boolean	  first;

    if ((ptr2=strstr(cur_file,".bkup")) == NULL || ptr2[5] != '\0')
       {
        cur_file = (char *) REALLOC(cur_file, strlen(cur_file) + 6);
	sprintf(buf, "%s.bkup", cur_file);	
	cur_file = strdup(buf);
       }

    if ((fp=fopen(cur_file,"r")) == NULL) {
	sprintf(buf, GGT(string_openFailed));
        ErrorNotice(buf, GetGizmoText(string_backupError));
	return;
    }

    first = True;
    while (fgets(buf, BUFSIZ-1, fp) != NULL)
    {
	if (first == True)
	{   /* read first few lines to make sure this is script file */
	    first = False;
	    if ((strncmp(buf,SHELL_LINE,strlen(SHELL_LINE)) != 0) ||
		(fgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strlen(buf) != 1) || buf[0] != '\n'  ||
		(fgets (buf, BUFSIZ-1, fp) == NULL) ||
	    	(strncmp(buf,MARKER_LINE,strlen(MARKER_LINE)) != 0) ||
		(fgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strlen(buf) != 1) || buf[0] != '\n'  ||
		(fgets (buf, BUFSIZ-1, fp) == NULL) ||
		(strncmp(buf,"Class=",6) != 0))
	    {
                ErrorNotice(GetGizmoText(string_notScript), 
                                            GetGizmoText(string_backupError));
                MapGizmo(FileGizmoClass, &open_prompt);
    	    	fclose(fp);
		return;
	    }
	}

	if ((ptr = strchr(buf, '=')) == NULL)
	    continue;
	ptr++;

	switch (*buf)
	{
	case 'C':
	    bkup_class = atoi(ptr);
	    Cset++;
	    break;
	case 'T':
	    bkup_type = atoi(ptr);
	    Tset++;
	    break;
	case 'H':
	    if ((end = strpbrk(ptr, " \t\n")) != NULL)
		*end = '\0';
	    home = STRDUP(ptr);	/* used to be ptr to static string */
	    Hset++;
	    break;
	case 'S':
            if ((end = strpbrk(ptr, " \t\n")) != NULL)
		*end = '\0';
	    if (source = DtamGetDev(ptr, FIRST)) /* single = is correct here */
		target = NULL;
	    else
		target = STRDUP(ptr);
	    Sset++;
	    break;
	}
	if (Cset && Tset && Hset && Sset) /* all ok */
	{
	    allSet = True;
	    break;
	}
    }
    if (allSet)			/* then read in filenames */
    {
	allSet = False;		/* need at least one filename */
	/* skip to the first filename */
	while(nbfgets(buf,BUFSIZ-1,fp))
	{
	    if (strstr(buf, HERE_DOC_WORD) == NULL)
		continue;
	    else
		break;
	}
	if (copy_source)
	    FREE(copy_source);
	copy_source = STRDUP("\n");
	while (nbfgets(buf, BUFSIZ-1, fp))
	{
	    if (strstr(buf, HERE_DOC_WORD) != NULL)
		break;
	    allSet = True;
	    if ((end = strpbrk(ptr, "\n")) != NULL)
		*end = '\0';
	    copy_source = (char *)REALLOC(copy_source,
					  strlen(copy_source)+strlen(buf)+2);
	    strcat(strcat(copy_source, buf), "\n");
	}
	bkup_source = copy_source;
    }
    if (allSet)
    {
	BringDownPopup(open_prompt.shell);
	if (source)
	{			/*it's a device */
	    curdev = source;
	    curalias = DtamDevAttr(curdev, ALIAS);
/*          curalias = DtamMapAlias(DtamDevAlias(curdev)); */
	    desc = DtamDevDesc(curdev);
	    XtSetArg(arg[0], XtNstring, "");
	    XtSetValues(w_target, arg, 1);
	    XtSetArg(arg[0], XtNmappedWhenManaged, False);
	    XtSetValues(XtParent(w_target), arg, 1);
	    for (i=1; i< N_DEVS; i++)
	    {
		if (strcmp(BkupDevice[i].label, curalias) == 0)
		{
		    XtSetArg(arg[0], XtNset, True);
		    OlFlatSetValues(w_devmenu, i, arg, 1);
		    break;
		}
	    }
	}
	else
	{
	    curdev = NULL;
	    desc = bdoc_alias;
	    curalias = STRDUP(bdoc_alias);
	    bkup_doc = target;
	    XtSetArg(arg[0], XtNstring, bkup_doc);
	    XtSetValues(w_target, arg, 1);
	    XtSetArg(arg[0], XtNmappedWhenManaged, True);
	    XtSetValues(XtParent(w_target), arg, 1);
	    XtSetArg(arg[0], XtNset, True);
	    OlFlatSetValues(w_devmenu, 0, arg, 1);
		
	}
	bkup_alias = STRDUP(curalias);

        XtSetArg(arg[0], XtNlabel, (XtArgVal)desc);
        XtSetValues(w_bdesc, arg, 1);

	XtSetArg(arg[0], XtNset, TRUE);
	OlFlatSetValues(w_type,  bkup_type,  arg, 1);
	OlFlatSetValues(w_class, bkup_class, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
    }
    else
    {
	if (target != NULL)
	    FREE(target);
        ErrorNotice(GetGizmoText(string_notScript), 
                                          GetGizmoText(string_backupError));
        MapGizmo(FileGizmoClass, &open_prompt);
    }
    fclose(fp);
}

static void
gotoRestoreCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	/* unmap backup window */
	if (bbase.shell)
		XtUnmapWidget(bbase.shell);
	/* map or create restore window */
	if (rbase.shell == NULL)
		SelectAction(DO_RESTOR, -1, client_data, NULL);
	else {
		remap_rst(rbase.shell);
	}
	
}

/*
 * Re-map backup window, after "Go to Backup" is selected.
 */
void
remap_bkup(Widget wid)
{
	char *desc, *tmpalias;

	XtSetArg(arg[0], XtNlabel, (XtArgVal)&desc);
	XtGetValues(w_bdesc, arg, 1);

	if (!strcmp(desc, bdoc_alias)) {
		curalias = STRDUP(bdoc_alias);
		curdev = NULL;
	}
	else {
		curalias = STRDUP(bkup_alias);
		tmpalias = STRDUP(curalias);
                curdev = DtamGetDev(strcat(tmpalias, ":"), FIRST);
                FREE(tmpalias);

	}
	
	XtMapWidget(wid);
}

static	void
doopenCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (cur_file)
	    FREE(cur_file);
	cur_file = GetFilePath(&open_prompt);
	OpenFile();
}

static	void
cancelOpenCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(open_prompt.shell);
	FooterMsg(bbase, NULL);
}

static	void
openCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(bbase, NULL);
	SetFileGizmoMessage(&open_prompt, "");
	SetFileCriteria(&open_prompt, NULL, "");
	MapGizmo(FileGizmoClass, &open_prompt);
}

static	void
saveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FILE	*fp;
	char	*ptr, buf[PATH_MAX + 80];
	Boolean ok;

	NotePidFiles();
	if (!cur_file)
		saveasCB(wid, client_data, call_data);
	else {
		if ((ptr=strstr(cur_file,".bkup")) == NULL
		|| ptr[5] != '\0')
		{
			cur_file = (char *) REALLOC(cur_file,
						strlen(cur_file) + 6);
			sprintf(buf, "%s.bkup", cur_file);
			cur_file = strdup(buf);
		}
		if ((fp = fopen(cur_file, "w")) == NULL)
		{
		    sprintf(buf, GetGizmoText(saveCantWriteScript),cur_file);
                    ErrorNotice(buf, GetGizmoText(string_backupError));
		    return;
		}		    
		ok = BkupScript(fp);
		chmod(cur_file, 00755);
		if (ok == False)
		    return;
		sprintf(buf, GetGizmoText(string_savedAs), cur_file);
		FooterMsg(bbase, buf);
	}
}

static	void
cancelSaveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(save_prompt.shell);
	FooterMsg(bbase, NULL);
}

static	void
dosaveCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (cur_file)
	    FREE(cur_file);
	cur_file = GetFilePath(&save_prompt);
	BringDownPopup(save_prompt.shell);
	saveCB(wid, client_data, call_data);
}

static	void
saveasCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(bbase, NULL);
	if (!save_prompt.shell)
		CreateGizmo(bbase.shell, FileGizmoClass, &save_prompt, NULL, 0);
	SetFileCriteria(&save_prompt, NULL, "");
	MapGizmo(FileGizmoClass, &save_prompt);
}

static	void
excludeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n, max;
	char	*ptr, *name;
	Boolean	set, change = FALSE;
   	char	delimitedName[PATH_MAX + 3] = "\n";
  	char	*dnp = delimitedName, *dnp1 = &delimitedName[1];
	
	FooterMsg(bbase, NULL);
	XtSetArg(arg[0], XtNnumItems, &max);
	XtGetValues(w_icons, arg, 1);
	for (n = 0; n < max; n++) {
		XtSetArg(arg[0], XtNlabel,	&name);
		XtSetArg(arg[1], XtNset,	&set);
		OlFlatGetValues(w_icons, n, arg, 2);
		if (set) {
			change = TRUE;
   			strcat(strcpy(dnp1, name), "\n");  /* "\nname\n" */
   			if (ptr=strstr(copy_source, dnp)) {
   				ptr++;
				while (*ptr && *ptr != '\n')
					*ptr++ = '\n';
			}
		}
	}
	if (change) {
		ResetIconBox();
	}
	else
		FooterMsg(bbase, GetGizmoText(string_noneset));
}

static	Boolean
WriteableMedium(_Dtam_inputProcData *inputData)
{
	char	buf[128];

	if (_dtam_flags & DTAM_READ_ONLY) {
		char	*dev = DtamDevAlias(curdev);
		sprintf(buf, GetGizmoText(string_cantWrite), dev);
		insert_bnote(&bnote, 0, inputData);
		XtVaSetValues(bnote.stext, XtNstring, buf, NULL);
		FREE(dev);
		return FALSE;
	}
	return TRUE;
}

static	void
testCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int	diagn;
    char	*ptr;
    _Dtam_inputProcData *inputData = (_Dtam_inputProcData *)client_data;

    switch (diagn = DtamCheckMedia(curalias)) {
    case DTAM_NO_DISK:	
	_OlBeepDisplay(wid, 1);
	break;			
    case DTAM_UNFORMATTED:
    {
	FooterMsg(bbase, ptr=GetGizmoText(string_doingFmt));
	if (w_bkmsg) {
	    XtSetArg(arg[0], XtNstring, ptr);
	    XtSetValues(w_bkmsg, arg, 1);
	}
	XSync(theDisplay, FALSE);
	startFormat(Blocks);
	BringDownPopup(bnote.shell);
	break;
    }

    case DTAM_UNKNOWN:
    case DTAM_UNREADABLE:
	if (WriteableMedium(inputData))
	    BringDownPopup(bnote.shell);
	callBackup();
	break;
    default:
	if (diagn > 0) {
	    BringDownPopup(bnote.shell);
	    if (!in_use.shell)
		CreateGizmo(bbase.shell,ModalGizmoClass, &in_use,NULL,0);
	    SetModalGizmoMessage(&in_use,GetGizmoText(string_in_use));
	    MapGizmo(ModalGizmoClass,&in_use);
	    return;		/* check this !! */
	}
    }
}

static	void
MakeUserList()
{
	struct	passwd	*pw;

	while (pw = _DtamGetpwent(STRIP, NIS_EXPAND, NULL)) {
		if (pw->pw_uid >= 100 && pw->pw_uid < UID_MAX-2 ) {
 			if (pw->pw_dir == NULL || strlen(pw->pw_dir) == 0 )
				continue;
			if (user_count == 0)
				users = (UserList)MALLOC(QUANTUM);
			else if (user_count % BUNCH == 0)
				users = (UserList)REALLOC(users,
					(1+(user_count/BUNCH))*QUANTUM);
			users[user_count].u_name = STRDUP(pw->pw_name);
			users[user_count++].u_home = STRDUP(pw->pw_dir);
		}
	}
	endpwent();
}

static void
bapplyCB()
{
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
}

static void
bokCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	bapplyCB();
	ucancelCB(wid, client_data, call_data);
}

static void
SelUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData  *olcd = (OlFlatCallData *) call_data;
	char            *name, *home_dir;

	XtSetArg(arg[0], XtNlabel, &name);
	XtSetArg(arg[1], XtNuserData, &home_dir);
	OlFlatGetValues(wid, olcd->item_index, arg, 2);

	user = name;
	home = bkup_source = home_dir;
}

static	void
dblSelUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*name, *home_dir;

	XtSetArg(arg[0], XtNlabel, &name);
	XtSetArg(arg[1], XtNuserData, &home_dir);
	OlFlatGetValues(wid, olcd->item_index, arg, 2);

	user = name;
	home = bkup_source = home_dir;

	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();
	ucancelCB(wid, client_data, call_data);
}

static	void
CreateUserPopup(Widget parent)
{
	Widget	w_up, w_tmp;

	CreateGizmo(parent, PopupGizmoClass, &buser, NULL, 0);

	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(buser.shell, arg, 1);

	w_tmp = XtCreateManagedWidget("scrolledWindow",
			scrolledWindowWidgetClass, w_up, NULL, 0);

	XtSetArg(arg[0], XtNviewHeight,	 	5);
	XtSetArg(arg[1], XtNnoneSet,		TRUE);
	XtSetArg(arg[2], XtNformat,		"%s");
	XtSetArg(arg[3], XtNitemFields,		UserFields);
	XtSetArg(arg[4], XtNnumItemFields,	2);
	XtSetArg(arg[5], XtNitems,		users);
	XtSetArg(arg[6], XtNnumItems,		user_count);
	XtSetArg(arg[7], XtNselectProc,		SelUserCB);
	XtSetArg(arg[8], XtNdblSelectProc,	dblSelUserCB);
	XtSetArg(arg[9], XtNexclusives,		TRUE);

	XtCreateManagedWidget("userList", flatListWidgetClass, 
		w_tmp, arg, 10);
}

static	FileList
MakeObjectList(char *source, int *count)
{
struct	stat		st_buf;
register int		n;
	char		*ptr, *str;
	FileList	bklist = NULL;

	if (source)
	    str = STRDUP(source);
	else {
	    *count = 0;
	    return (FileList)NULL;
	}
	for (n=0, ptr=strtok(str,"\n"); ptr; ptr=strtok(NULL,"\n")) {
		if (bklist)
			bklist=(FileList)REALLOC(bklist,(n+1)*sizeof(FileObj));
		else
			bklist=(FileList)MALLOC(sizeof(FileObj));
		bklist[n].bk_path = STRDUP(ptr);
		if (bkup_class == CLS_NDS) {
			bklist[n].bk_type = 'n';
		} else {
			stat(ptr, &st_buf);
			bklist[n].bk_type = (st_buf.st_mode & S_IFDIR)?'d':'f';
		}
		n++;
	}
	*count = n;
	if (str)
	    FREE(str);
	return bklist;
}

static int	item_count = 0;

static	DmObjectPtr
AddItem(FileList b)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
	optr->container = &b_cntrec;
	switch (b->bk_type) {
	case 'd':
		optr->fcp = &dir_fcrec;
		break;
	case 'n':
		optr->fcp = &nds_fcrec;
		break;
	default:
		optr->fcp = &doc_fcrec;
	}
	optr->name = b->bk_path;	/* maybe just basename? */
        optr->x = UNSPECIFIED_POS;
	optr->y = UNSPECIFIED_POS;
	optr->objectdata = (XtPointer)b;
	if (item_count++ == 0) {
		b_cntrec.op = optr;
	}
	else {
		DmObjectPtr endp = b_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	b_cntrec.num_objs = item_count;
	return optr;
}

void
BackupTakeDrop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	DtDnDInfoPtr    dip = (DtDnDInfoPtr)call_data;
	char		**p, *name, *ptr;
   	char		delimitedName[PATH_MAX + 3] = "\n";
   	char		*dnp = delimitedName, *dnp1 = &delimitedName[1];

   	if (copy_source == NULL)
   		copy_source = STRDUP("\n");

	if (dip->files && *dip->files && **dip->files) {
		for (p=dip->files; *p; p++) {
			name = *p;
			if (name[0] == '/' && name[1] == '/')
				name++;
   			strcat(strcpy(dnp1, name), "\n");  /* "\nname\n" */
   			if (ptr=strstr(copy_source, dnp))
				continue;
			else {
				copy_source = (char *)REALLOC(copy_source,
					strlen(copy_source)+strlen(name)+2);
				strcat(strcat(copy_source, name), "\n");
			}
		}
		bkup_source = copy_source;
		FooterMsg(bbase, SummaryMsg());
		ResetIconBox();
	}
	else {
		XMapWindow (theDisplay, XtWindow (bbase.shell));
		XRaiseWindow (theDisplay, XtWindow (bbase.shell));
	}
}

static Boolean
TriggerNotify(	Widget			w,
		Window			win,
		Position		x,
		Position		y,
		Atom			selection,
		Time			timestamp,
		OlDnDDropSiteID		drop_site_id,
		OlDnDTriggerOperation	op,
		Boolean			send_done,
		XtPointer		closure)
{
	DtGetFileNames(w, selection, timestamp, send_done, BackupTakeDrop,
		       closure);
}

static	Widget
GetIconBox(Widget parent, int count)
{
	int	n;
	Widget	w_box;
	DmItemPtr	item;
	Dimension	width;
	Position icon_x, icon_y;

	item_count = 0; ibx = INIT_X; iby = INIT_Y;
	for (n = 0; n < count; n++)
		AddItem(BkupTarget+n);

	n = 0;	XtSetArg(arg[0], XtNmovableIcons,	TRUE);
	n++;	XtSetArg(arg[1], XtNminWidth,		1);
	n++;	XtSetArg(arg[2], XtNminHeight,		1);
	n++;	XtSetArg(arg[3], XtNdrawProc,		DmDrawIcon);
	if (bkup_class == CLS_FILES) {
		n++;	XtSetArg(arg[4], XtNtriggerMsgProc, TriggerNotify);
	}
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, arg, ++n,
			b_cntrec.op,count,&b_itp,count,NULL,
                        MM_FONTLIST,def_font,1);

	XtVaGetValues(w_box, XtNwidth, &width, 0); 
	width -= MARGIN;	/* FIX: shouldn't be needed:  iconbox bug */

	for (item = b_itp; item < b_itp + count; item++)
	    if (ITEM_MANAGED(item))
	    {
		DmGetAvailIconPos(b_itp, count, ITEM_WIDTH(item),
				  ITEM_HEIGHT(item), width,
			  INC_X, INC_Y,
				  &icon_x, &icon_y );
		item->x = (XtArgVal)icon_x;
		item->y = (XtArgVal)icon_y;
	    }
	XtSetArg(arg[0], XtNitemsTouched, TRUE);
	XtSetValues(w_box, arg, 1);

	return w_box;
}

static	void
ResetIconBox()
{
	XtPointer	items;

	if (BkupTarget)
		FREE(BkupTarget);
	XtUnmanageChild(w_icons);
	XtSetArg(arg[0], XtNitems, &items);
	XtGetValues(w_icons, arg, 1);
	XtDestroyWidget(w_icons);
	BkupTarget = MakeObjectList(bkup_source, &bkup_count);
	w_icons = GetIconBox(bbase.scroller, bkup_count);
}

static char*
GetNDSparts()
{
	FILE	*cmdpipe;
	char	buf[BUFSIZ];
	int	status = -1;
	char 	*parts = NULL;

	sprintf(buf, "%sbackup -l", NDSCmd);

	/* This may take a while so change the cursor to a clock */
	XDefineCursor(theDisplay, XtWindow(bbase.shell),
	    GetOlBusyCursor(theScreen));

	/* Force the busy cursor to be displayed */
	XSync(theDisplay, FALSE);

	if (cmdpipe = popen(buf,"r")) {
		while (fgets(buf, BUFSIZ, cmdpipe)) {
			if (parts) {
				parts = REALLOC(parts, strlen(parts) + strlen(buf) + 1);
				strcat(parts, buf);
			} else
				parts = strdup(buf);
		}
		status = pclose (cmdpipe);
	} 

    	if (WIFEXITED(status))
		if ((status = WEXITSTATUS(status)) != 0) {
			ShowNDSError(status);
			parts = NULL;
		}

	/* Return to regular cursor */
	XDefineCursor(theDisplay, XtWindow(bbase.shell), 
	    GetOlStandardCursor(theScreen));

	return	parts;
}

static	void
SetBkupData(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlDnDDropSiteID drop_site_id;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*ptr;

	XtSetArg(arg[0], XtNlabel, &ptr);
	OlFlatGetValues(wid, olcd->item_index, arg, 1);
	bkup_class = olcd->item_index;

	/* Desensitize incremental backup type if nds or sel files */
	if (bkup_class == CLS_NDS || bkup_class == CLS_FILES) {
		bkup_type = BKUP_COMPL;
		XtSetArg(arg[0], XtNsensitive, FALSE);
		OlFlatSetValues(w_type, BKUP_INCR, arg, 1);
	} else {
		XtSetArg(arg[0], XtNsensitive, TRUE);
		OlFlatSetValues(w_type, BKUP_INCR, arg, 1);
	}

	/* Desensitize Local Files Only option if nds backup */
	if (bkup_class == CLS_NDS) {
		XtSetArg(arg[0], XtNsensitive, FALSE);
                OlFlatSetValues(w_log, 0, arg, 1);
	} else {
		XtSetArg(arg[0], XtNsensitive, TRUE);
                OlFlatSetValues(w_log, 0, arg, 1);
	}

	switch (bkup_class) {
	    case CLS_SYSTEM:	bkup_source = ROOT;
				home = system_home;
				/* 
				 * If backing up complete system,
				 * default should be to backup local files
				 * only. 
				 */
				XtSetArg(arg[0], XtNset, TRUE);
                    		OlFlatSetValues(w_log, 0, arg, 1);
				break;
	    case CLS_USERS:	if (!buser.shell)
					CreateUserPopup(bbase.shell);
				MapGizmo(PopupGizmoClass, &buser);
				/* 
				 * If backing up other users' files,
				 * default should be to backup all files,
				 * including remote files.
				 */
				XtSetArg(arg[0], XtNset, FALSE);
                    		OlFlatSetValues(w_log, 0, arg, 1);
				return;
				/* i.e., leave resetting of icon box to popup */
	    case CLS_SELF:	home = bkup_source = user_home;
				/* 
				 * If backing up personal files,
				 * default should be to backup all files,
				 * including remote files.
				 */
				XtSetArg(arg[0], XtNset, FALSE);
                    		OlFlatSetValues(w_log, 0, arg, 1);
				break;
	    case CLS_FILES:	bkup_source = copy_source;
				/* 
				 * If backing up selected files,
				 * default should be to backup all files,
				 * including remote files.
				 */
				XtSetArg(arg[0], XtNset, FALSE);
                    		OlFlatSetValues(w_log, 0, arg, 1);
				XtSetArg(arg[0], XtNtriggerMsgProc, TriggerNotify);
				XtSetValues(w_icons, arg, 1);
				break;
	    case CLS_NDS:	bkup_source = GetNDSparts();
				/* 
				 * If NDS backup, default should be 
				 * to backup local files only. 
				 */
				XtSetArg(arg[0], XtNset, TRUE);
                    		OlFlatSetValues(w_log, 0, arg, 1);
				break;
	}

	/* iff selected files set Edit:Exclude button to sensitive */
	XtSetArg(arg[0], XtNsensitive, bkup_class == CLS_FILES);
	OlFlatSetValues(w_bkmenu, 0, arg, 1);

	/* iff selected files turn the icon pane into a drop site */
        XtVaGetValues(w_icons, XtNdropSiteID, &drop_site_id, NULL);
        OlDnDSetDropSiteInterest(drop_site_id,
                                (bkup_class == CLS_FILES) ?
                                      True : False);

	bkup_type = BKUP_COMPL;
	XtSetArg(arg[0], XtNset, TRUE);
	OlFlatSetValues(w_type, bkup_type, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();

	/* Display Full System warning/explanation */
	if (bkup_class == CLS_SYSTEM) {
		show_full_warn = True;
		ShowWarn();
	}
}

static	void
SetBkupType(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char		*ptr;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	XtSetArg(arg[0], XtNlabel, &ptr);
	OlFlatGetValues(wid, olcd->item_index, arg, 1);
	FooterMsg(bbase,ptr);
	switch (bkup_type=olcd->item_index) {
	    case BKUP_COMPL:
	    case BKUP_INCR:	bkup_source = (bkup_class == CLS_SYSTEM) ?
				    ROOT : home;
				if (bkup_class == CLS_SYSTEM) {
					/* 
				 	 * If backing up full system,
				 	 * default should be to backup 
				 	 * local files only.
				 	 */
					XtSetArg(arg[0], XtNset, TRUE);
                    			OlFlatSetValues(w_log, 0, arg, 1);
				}
				break;
	}
	XtSetArg(arg[0], XtNtriggerMsgProc, NULL);
	XtSetValues(w_icons, arg, 1);
	OlFlatSetValues(w_bkmenu, 0, arg, 1);
	FooterMsg(bbase, SummaryMsg());
	ResetIconBox();


}

static void 
gaugeIO(Boolean open, bkupClient_t *bdata)
{

    if (open)
    {
	if ((bdata-> indexfp = fopen(bdata-> indexfile, "r")) != NULL)
	{
	    SetGauge(0);
	    XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
	    XtSetValues(w_gauge, arg, 1);
	}
    }	
    else
    {
	fclose(bdata-> indexfp);
	unlink(bdata-> indexfile);
    }
}

FILE *
OpenLogFile()
{
    	time_t     clk;
	char buf[PATH_MAX];
	FILE *fp;

	sprintf(buf, "%s/%s", home, BKUP_LOG);
	log = fopen(buf, "a");
	if (log) 		/* datestamp start of backup */
	{
	    fprintf(log, "%s\n", SummaryMsg());
	    time(&clk);
	    fprintf(log, GetGizmoText(string_startBkup),
		    ctime(&clk));
	}

	return log;
}

static void
startBackup(int blocks)
{
    _Dtam_inputProcData *bkupData;
    time_t     clk;
    bkupClient_t *bdata;
    char 	buf[BUFSIZ];

    /* If new media has been inserted tell cpio/ndsbackup to continue */
    if (vol_count > 1)
	backupAck();

    /* startBackup is called by testCB which is called for each diskette */
    /* but we only need to start the backup once so set the global */
    /* "Blocks" to 0 so the parameter "blocks" will be 0 after the */
    /* backup is started.  */

    if (blocks == 0)
	return;
    Blocks = 0;

    bkupData = (_Dtam_inputProcData *)MALLOC(sizeof(_Dtam_inputProcData));
    bkupData-> appContext  = App_con;
    bkupData-> client_data = (XtPointer)MALLOC(sizeof(bkupClient_t));
    bdata = (bkupClient_t *)bkupData-> client_data;
    bdata-> blocks   = blocks;
    bdata-> indexfile = pidindex;
    bdata-> indexfp  = NULL;

    FooterMsg(bbase, NULL);

    /* start off the actual backup */

    /*
     *	set up for interaction with cpio; clone a pseudo-tty
     *  to capture/respond to cpio's prompts to insert next volume.
     */
    init_pty(bkupData);
    if (bkup_class == CLS_NDS) {
    	bkup_cmd = STRDUP(BkupLine(B_IMMEDIATE, NULL));
	StartBkupWatch(bkupData);
    }
    else
    	bkup_cmd = STRDUP(BkupLine(B_IMMEDIATE, bdata-> indexfile));

    XtSetArg(arg[0], XtNstring, GetGizmoText(string_doingBkup));
    XtSetValues(w_bkmsg, arg, 1);
    OlVaFlatSetValues(bwatch_menu.child, 0, XtNclientData,
		      (XtArgVal)bkupData, 0);
    MapGizmo(PopupGizmoClass, &bwatch);

printf("about to try bkup_cmd = %s\n", bkup_cmd);
    if (bkup_class == CLS_NDS) {
        _Dtam_p3openInput(bkup_cmd, CheckNDS, CheckNDS, bkupData, FALSE);
    }
    else {
    	gaugeIO(TRUE, (bkupClient_t *)bkupData-> client_data);
        _Dtam_p3openInput(bkup_cmd, CheckCpio, CheckCpio, bkupData, FALSE);
    }


    if (save_log)
	log = OpenLogFile();
    else
	log = (FILE *)NULL;
}


static void
startFormat(int blocks)
{
    _Dtam_inputProcData *formatData;
    FILE                *fp;
    char		 buf[BUFSIZ];
    int			 fd;

    formatData = (_Dtam_inputProcData *)
	MALLOC(sizeof(_Dtam_inputProcData));
    sprintf(buf, "%s -F -X -D %s &",
	    GetXWINHome("bin/MediaMgr"), curalias);
    formatData-> fp[0] = fp = popen(buf, "r");
    formatData-> fp[1] = NULL;
    formatData-> appContext = App_con;
    formatData-> client_data = (XtPointer)blocks;
    fcntl((fd = fileno(fp)), F_SETFL, O_NONBLOCK);
    formatData-> exceptId = XtAppAddInput(App_con, fd,
					  (XtPointer)XtInputExceptMask,
					  postFormat, (XtPointer)formatData); 
    formatData-> readId   = XtAppAddInput(App_con, fd,
					  (XtPointer)XtInputReadMask, 
					  postFormat, (XtPointer)formatData); 

}


static void
postFormat(XtPointer client_data, int *source, XtInputId *id)
{
    _Dtam_inputProcData *formatData = (_Dtam_inputProcData *)client_data;
    char buf[BUFSIZ];
    int  cnt, blocks;

    if (*id == formatData-> readId)
    {
	cnt = read(*source, buf, BUFSIZ);
	if (cnt != 0)
	    return;
    }
    blocks = (int) formatData-> client_data;
    XtRemoveInput(formatData-> readId);
    XtRemoveInput(formatData-> exceptId);
    pclose(formatData-> fp[0]);
    FREE(formatData);
    callBackup();
    return;
}


static void
postVolume(XtPointer client_data, int *source, XtInputId *id)
{
    XtRemoveInput(volumeData-> readId);
    XtRemoveInput(volumeData-> exceptId);
    fclose(ptyfp);
    ptyfp = (FILE *)NULL;
    FREE((char *)volumeData);
    volumeData = (_Dtam_inputProcData *)NULL;
}

static void
CheckVolume(XtPointer client_data, int *source, XtInputId *id)
{
    static      buffer_t buffer;
    int                  numRead;
    _Dtam_inputProcData *bkupData = (_Dtam_inputProcData *)client_data;

    numRead = getbuf(&buffer, ptyfp);
    if (numRead > 0 && buffer.complete == TRUE)
    {
	buffer.complete = FALSE;

	if (strncmp(buffer.buf, EOM, 4) == 0)
	{
	    vol_count = atoi(buffer.buf+4);
	    insert_bnote(&bnote, NO_DISK, bkupData);
	    rewind(ptyfp);
	}
    }
}

void
ShowNDSError(int status)
{
	char 	buf[BUFSIZ];

	switch(status) {
	case GENERAL_NWS_NOT_RUNNING:
            sprintf(buf, GetGizmoText(string_NDSnwsDown));
	    break;
	case NWRST_NWS_INTERNAL_ERR:
	case NWRST_UNKNOWN_ERR:
	case NWBKUP_NWS_INTERNAL_ERR:
	case NWBKUP_UNKNOWN_ERR:
            sprintf(buf, GetGizmoText(string_NDSnwsFail));
	    break;
	case GENERAL_INPUT_OPEN_ERR:
	case GENERAL_OUTPUT_OPEN_ERR:
	case GENERAL_READ_ERR:
	case GENERAL_WRITE_ERR:
            sprintf(buf, GetGizmoText(string_NDSmedFail));
	    break;
	default:
            sprintf(buf, GetGizmoText(string_bkupError));
	    break;
	}
        ErrorNotice(buf, GetGizmoText(string_backupError));
        FooterMsg(bbase, GetGizmoText(string_bkupFailed));
}

static void
postNDS(Boolean errors, _Dtam_inputProcData *bkupData, char * errmsg)
{
    bkupClient_t *bdata = (bkupClient_t *)bkupData-> client_data;
    char 	buf[BUFSIZ];
    int		status;
    time_t	clk;

    /* Close pseudo term and check status of ndsbackup */
    status = _Dtam_p3closeInput(bkupData, SIGTERM);
    if (WIFEXITED(status))
	status = WEXITSTATUS(status);

printf("status from NDS backup = <%d>\n", status);
    Restricted_exit_val = status ? NDSErrs : NoErrs;

    /* No errors, put up backup complete window */
    if (errors != TRUE  && status==0) {
        sprintf(buf, GetGizmoText(string_bkupOK));
        XtSetArg(arg[0], XtNstring, buf);
        XtSetValues(w_bkmsg, arg, 1);
        FooterMsg(bbase, buf);
    }
    else { 
	FindErrors = True;
	ShowNDSError(status);
    }

    /* close index file... */
    if (ptyfp) {
	postVolume(NULL, 0, NULL);
    }

    if (log) {
	time(&clk);
	fprintf(log, buf);
	fprintf(log, " -- %s", ctime(&clk));
	fclose(log);
	log = (FILE *)NULL;
    }

    FREE((char *)bdata);
    FREE((char *)bkupData);

    /* done! bring down gauge after 2 seconds */
    XtAddTimeOut(2000, (XtTimerCallbackProc)DropBkupWatch,
		 (XtPointer)FindErrors);
    return;
}
    
static void
postCpio(Boolean errors, _Dtam_inputProcData *bkupData, char * errmsg)
{
    bkupClient_t  *bdata = (bkupClient_t *)bkupData-> client_data;
    char 	buf[BUFSIZ];
    char 	path[BUFSIZ];
    int		status;
    time_t	clk;

    /*
     *	done! output time stamps
     */
    if (errors == TRUE){	/* cpio output contained HALT_tag */
        if (errmsg != NULL)
            ErrorNotice(errmsg, GetGizmoText(string_backupError));
        else{
            sprintf(buf, GetGizmoText(string_bkupFailed));
            ErrorNotice(buf, GetGizmoText(string_backupError));
        }
    }
    else{
        if (FindErrors){
             sprintf(buf, GetGizmoText(string_findErrors), GetGizmoText(label_backup));
             ErrorNotice(buf, GetGizmoText(string_backupError));
        } 
	else if (CpioWarnings) {
	     sprintf(path, "%s/%s", home, BKUP_LOG);
             sprintf(buf, GetGizmoText(string_cpioWarnings), path);
             XtSetArg(arg[0], XtNstring, buf);
             XtSetValues(w_bkmsg, arg, 1);
             FooterMsg(bbase, buf);
	}
        else{
             sprintf(buf, GetGizmoText(string_bkupOK));
             XtSetArg(arg[0], XtNstring, buf);
             XtSetValues(w_bkmsg, arg, 1);
             FooterMsg(bbase, buf);
        }
    }

    XtSetArg(arg[0], XtNmappedWhenManaged, FALSE);
    XtSetValues(w_gauge, arg, 1);
    status = _Dtam_p3closeInput(bkupData, SIGTERM);
    if (WIFEXITED(status))
	status = WEXITSTATUS(status);

    if (status == 0)
	Restricted_exit_val = FindErrors ?
	    FindErrs : NoErrs;
    else
	Restricted_exit_val = CpioErrs;
    child_pgid = 0;
    /* close index file... */
    gaugeIO(FALSE, bdata);
    if (ptyfp) {
	postVolume(NULL, 0, NULL);
    }

    if (bkup_class != CLS_FILES) {
	char buf[PATH_MAX + 20];

	sprintf(buf, "echo `date` > %s/%s",
		home, bkup_type==BKUP_INCR?
	    INCR_HIST: BKUP_HIST);
	system(buf);
	if (bkup_type == BKUP_COMPL) {
	    sprintf(buf, "%s/%s", home,
		    BKUP_INCR);
	    unlink(buf);
	}
    }
    if (log) {
	time(&clk);
	fprintf(log, GetGizmoText(string_bkupOK));
	fprintf(log, " -- %s", ctime(&clk));
	fclose(log);
	log = (FILE *)NULL;
    }

    FREE((char *)bdata);
    FREE((char *)bkupData);

    /* done! bring down gauge after 2 seconds */
    XtAddTimeOut(2000, (XtTimerCallbackProc)DropBkupWatch,
		 (XtPointer)FindErrors);
    return;
}
    
static	void
CheckNDS(XtPointer client_data, int *source, XtInputId *id)
{
	static	Boolean	 ndsErrors = FALSE;
	static	Boolean	 bkup_done  = FALSE;
	static 	buffer_t buffer;
	static 	size_t	 n = 0;
	size_t		 len;
	char        	 tmp_buffer[BUFSIZ];
	char		*ptr, *tmp_ptr1, *tmp_ptr2;
	int		 numRead;
	_Dtam_inputProcData *bkupData = (_Dtam_inputProcData *)client_data;
	bkupClient_t *bdata = (bkupClient_t *)bkupData-> client_data;

	if (*id == bkupData-> readId)
	{
		numRead = getbuf(&buffer, bkupData-> fp[1]);
		if (numRead > 0 && buffer.complete == TRUE)
		{
			buffer.complete = FALSE;
			if (strncmp(buffer.buf,NDS_ERR,
                                              n=strlen(NDS_ERR))==0)
			{
				ndsErrors = TRUE;
				/* error -- report as notice */
	                 	postNDS(ndsErrors, bkupData, 
                                        GetGizmoText(string_bkupError));
				ndsErrors = FALSE;
				BringDownPopup(bwatch.shell);
				FooterMsg(bbase, NULL);
			}
			else { /* just spit out the object name */
				FooterMsg(bbase, buffer.buf);
				if (log)
					fputs(buffer.buf, log);
				if (bkup_done)
					return;
			}
		}    
		else if (numRead == 0) {  /* ndsbackup finished */
			postNDS(ndsErrors, bkupData, NULL);
			ndsErrors = FALSE;
		}
	}
	else	/* nds didn't have anything to backup */
	{
		postNDS(ndsErrors, bkupData, NULL);
		ndsErrors = FALSE;
	}
}


static	void
CheckCpio(XtPointer client_data, int *source, XtInputId *id)
{
	static	int	 fraction = 0;
	static	char	 indexline[9+PATH_MAX];
	static	Boolean	 cpioErrors = FALSE;
	static	Boolean	 bkup_done  = FALSE;
	static 	buffer_t buffer;
	static 	size_t	 n = 0;
	size_t		 len;
	char        tmp_buffer[BUFSIZ];
	char		*ptr, *tmp_ptr1, *tmp_ptr2;
	int			 numRead;
	_Dtam_inputProcData *bkupData = (_Dtam_inputProcData *)client_data;
	bkupClient_t	*bdata    = (bkupClient_t *)bkupData-> client_data;

	if (*id == bkupData-> readId)
	{
		numRead = getbuf(&buffer, bkupData-> fp[1]);
		if (numRead > 0 && buffer.complete == TRUE)
		{
			buffer.complete = FALSE;
			if (strstr(buffer.buf, END_tag))
			{
				SetGauge(100);
				postCpio(cpioErrors, bkupData, NULL);
				cpioErrors = FALSE;
				fraction = 0;
				return;
			}
			else if (strncmp(buffer.buf,HALT_tag,
                                              n=strlen(HALT_tag))==0)
			{
				cpioErrors = TRUE;
				fraction = 0;
				return;
			}
			else if (strncmp(buffer.buf,WARN_tag,
                                              n=strlen(WARN_tag))==0)
			{
				CpioWarnings = TRUE;

				/* make sure log is open for warnings */
				if (log == (FILE *)NULL) {  
					log = OpenLogFile();
					save_log = TRUE;
				}

				/* report the warning */
				FooterMsg(bbase, buffer.buf);
				if (log)
					fputs(buffer.buf, log);
			}
			else if (*buffer.buf == '/') /* file transfer by cpio */
			{
				/*
				 *	bother: cpio holds links to the end,
				 *	so that they are not reported in order;
				 *	I should do the same, saving their size
				 *	as well, and match against this list as
				 *	cpio gets to them and add to the gauge.
				 *	for now, I'm going to fake it.
				 */

				FooterMsg(bbase, buffer.buf);
				if (log)
					fputs(buffer.buf, log);
				if (bkup_done)
					return;

				fgets(indexline, sizeof(indexline), 
                                                          bdata-> indexfp);
				if (strstr(indexline, "BLOCKS="))
				{
					/*
			                 * cpio is getting around to the link
				         * file that we accounted for in order
							     */
					bkup_done = TRUE;
					return;
				}
				fraction += atoi(indexline);
				SetGauge(100 * fraction / bdata-> blocks);
			}
			else
			{
				cpioErrors = TRUE;

				/*
				 *	error condition -- report as notice
				 */
				if (strncmp(buffer.buf,ERR_tag,
                                                   n=strlen(ERR_tag))==0) {
	                 	   postCpio(cpioErrors, bkupData, 
                                            GetGizmoText(string_bkupError));
				   cpioErrors = FALSE;
				   BringDownPopup(bwatch.shell);
				   FooterMsg(bbase, NULL);
				}
				else {
		                   postCpio(cpioErrors, bkupData, 
					    GetGizmoText(string_unknownErr));
				   cpioErrors = FALSE;
				   BringDownPopup(bwatch.shell);
				   FooterMsg(bbase, NULL);
				}

			}
		}
	}
	else	/* cpio exited */
	{
		postCpio(cpioErrors, bkupData, NULL);
		cpioErrors = FALSE;
	}
}

static void
indexRemoveInput(_Dtam_inputProcData *data)
{
    char *indexscript = (char *)data-> client_data;

    unlink(indexscript);
    BringDownPopup(bwatch.shell);
    return;
}


static	void
FetchIndex(XtPointer client_data, int *source, XtInputId *id)
{
    struct	stat	 st;
    int			 media_type, status;
    char		*ptr;
    static buffer_t	 buffer;
    int			 numRead;
    static int		 blocks;
    _Dtam_inputProcData *indexData = (_Dtam_inputProcData *)client_data;
    
    if (*id == indexData-> readId)
    {
	numRead = getbuf(&buffer, indexData-> fp[1]);
	if (buffer.complete == FALSE && numRead >= 0)
	    return;
	buffer.complete = FALSE;

	if (numRead < 0 || strstr(buffer.buf, ERR_fld))
	{
		if (strstr(buffer.buf, FindErrorOutput) != 0) 
			ErrorNotice(GetGizmoText(string_cantIndex), 
                                        GetGizmoText(string_backupError));
		if (strstr(buffer.buf, "ERROR:") != 0)
			InfoNotice(GetGizmoText(string_noUpdate));
		indexRemoveInput(indexData);
		_Dtam_p3closeInput(indexData, SIGTERM);
		FREE((char *)indexData);
		DropBkupWatch(NULL, NULL);
		return;
	}
	if (strncmp(buffer.buf, "BLOCKS=", 7) == 0)  {
	    Blocks = blocks = atoi(buffer.buf+7);
	    goto done;
	}

	if (strstr(buffer.buf, FindErrorOutput)) 
	    FindErrors = True;
	else if (strncmp(buffer.buf, "dtindex:", 8) == 0)
	{
	    /*
	     *	unreadable file; check if symbolic link
	     *	(if so, ignore silently; else warn user)
	     */
	    ptr = strrchr(buffer.buf,' ')+1;
	    ptr[strlen(ptr)-1] = '\0';
	    if (stat(ptr, &st) == 0)
	    {
		sprintf(buffer.buf,GetGizmoText(string_unreadFile),ptr);
		XtSetArg(arg[0], XtNstring, buffer.buf);
		XtSetValues(w_bkmsg, arg, 1);
	    }
	}
	else
	    FooterMsg(bbase, strchr(buffer.buf,'\t')+1);
	return;
    }
 done:
    indexRemoveInput(indexData); 
    status = _Dtam_p3closeInput(indexData, 0);
    if (WIFEXITED(status))
	status = WEXITSTATUS(status);

    FREE((char *)indexData);
    if (bkup_cmd) {
			FREE(bkup_cmd);
			bkup_cmd = NULL;
    }
	startBackup(blocks);
}

static void
callBackup()
{
	/* The first step for a cpio backup is to create the index */
	if (bkup_class != CLS_NDS  &&  vol_count <= 1)
		startIndex();
	else 
		startBackup(Blocks);	
}

static	void
BackupLaunchCB()
{
	
	BringDownPopup(in_use.shell);
	callBackup();
}

static	char
*BkupLine(int flag, char *indexfile)
{
static	char	buf[BUFSIZ];
struct	stat	fbuf;
	char   *target;

	if (curdev)
		target = DtamDevAttr(curdev, "cdevice");
	else {
		XtSetArg(arg[0], XtNstring, &bkup_doc);
		XtGetValues(w_target, arg, 1);
		if (bkup_doc == NULL || *bkup_doc == '\0') {
			FooterMsg(bbase, GetGizmoText(string_bdocTitle));
			return NULL;
		}
		else if (stat(bkup_doc,&fbuf) == 0) {
			sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
			FooterMsg(bbase, buf);
			return NULL;
		}
		target = bkup_doc;
	}
	*buf = '\0';

	if (bkup_class == CLS_NDS) {
		sprintf(buf, "%sbackup -v -o %s", NDSCmd, target);
	} else 
		sprintf(buf+strlen(buf),
		    "/usr/bin/cut -f2 < %s | %s | %s -odlucvB -O %s",
		    indexfile, GetXWINHome ("adm/RemoteFilt"), 
		    CpioCmd, target);
	if (curdev) {
		FREE(target);
		if (flag == B_IMMEDIATE)
			sprintf(buf+strlen(buf), " -M \"%s\n\" -G %s",
					 	EOM, ptyname);
	}
	if (flag == B_IMMEDIATE)
		strcat(buf, " 2>&1");
	else {
		sprintf(buf+strlen(buf)," >/tmp/bkupout.%s 2>/tmp/bkuperr.%s\n",
				pid, pid);
		/*
		 *	need to attach here mail on success/failure
		 *	and updating user logs and datefiles.
		 */
		sprintf(buf+strlen(buf), "/usr/bin/rm -f /tmp/*.%s\n", pid);
	}
	return buf;
}

static char *
getSource()
{
           char  buf[BUFSIZ];
    struct stat  fbuf;
    static char	*target;

    if (curdev)
	target = DtamDevAttr(curdev, "cdevice");
    else {
	XtSetArg(arg[0], XtNstring, &bkup_doc);
	XtGetValues(w_target, arg, 1);
	if (bkup_doc == NULL || *bkup_doc == '\0') {
	    FooterMsg(bbase, GetGizmoText(string_bdocTitle));
	    return NULL;
	}
	else if (stat(bkup_doc,&fbuf) == 0) {
	    sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
	    FooterMsg(bbase, buf);
	    return NULL;
	}
	target = bkup_doc;
    }
    return target;
}

static	void
IndexCmd(FILE *scriptfd, int flag)
{
	char	*ptr, buf[PATH_MAX+8];
	Boolean	do_grep = FALSE;
	Boolean	local_only = FALSE;	/* backup local files only ? */

	if (bkup_class != CLS_FILES) {
		sprintf(buf, "%s/%s", home, IGNORE);
		do_grep = (access(buf,R_OK)==0);
	}
	*buf = '\0';

	XtSetArg(arg[0], XtNset, &local_only);
        OlFlatGetValues(w_log, 0, arg, 1);

	/* buf should either be an empty string or contain the 
	 * find arguments for doing an incremental backup
	 */
	if (bkup_type == BKUP_INCR) {
		sprintf(buf, " -newer %s/%s", home, INCR_HIST);
		if (access(buf+8, R_OK) != 0) {
			sprintf(buf+8, "%s/%s", home, BKUP_HIST);
			if (access(buf+8, R_OK) != 0)
				*buf = '\0';
		}
	}

	/* Make sure that if there is a request to backup a sym link
	 * that the link is followed, do this by trying to cd to the
	 * directory and do a pwd.  We print out something like:
	 * 
	 * rm /tmp/FFILES.nnn /tmp/FIND.ext 2>/dev/null
	 * while read line
	 * do
	 * 	realfile="`(cd "$line" > /dev/null 2>&1  && /bin/pwd) || echo "$line"`"
	 *	find $realfile -print >> /tmp/FFILES.nnn 2>> /tmp/FIND.nnn
	 * done <<-"ThIsShOuLdNotCoLiDeWiThAnYfIlEnAmE"
	 */
	fprintf(scriptfd, 
	    "rm /tmp/?FILES.%s /tmp/FIND.%s 2>/dev/null\nwhile read line\ndo\n",
	     pid, pid);
	fprintf(scriptfd, "\trealfile=\"`(cd \"$line\" > /dev/null 2>&1 && /bin/pwd) || echo \"$line\"`\"\n");
	
	fprintf(scriptfd, "\t/usr/bin/find $realfile ");
	/* local-only, non-incremental backup looks like:
	 *  find <files> ! -local -prune -o -print
	 * local-only, incremental backup looks like:
	 *  find <files> ! -local -prune -o -newer <lastbackup> -print
	 */
	fprintf(scriptfd, "%s ", local_only? "! -local -prune -o":" ");
	fprintf(scriptfd,
		  "%s -print >> /tmp/%cFILES.%s 2>> /tmp/FIND.%s\ndone <<-\"%s\"\n",
		  *buf? buf: " ", do_grep? 'G': 'F', pid, pid, HERE_DOC_WORD);
	/* Print out the files/dirs to be backed up */
	for (ptr = bkup_source; *ptr== '\n'; ptr++)
		;
	for (; *ptr; ptr++)
	{
		if (*ptr == '\n') 
		{
			fputc('\n', scriptfd);
			while (*ptr == '\n')
				++ptr;
		}
		if (*ptr)
			fputc(*ptr, scriptfd);
		else
			ptr--;
	}
	if (*(ptr-1) != '\n') fprintf(scriptfd, "\n");
	fprintf(scriptfd, "%s\n", HERE_DOC_WORD);
	if (do_grep) {
		fprintf(scriptfd,
			"sed 's;\\(.*\\);\\^\\1\\/;g' %s/%s > /tmp/Ignore.%s\n", home, IGNORE, pid);
		fprintf(scriptfd,
			"egrep -v -f /tmp/Ignore.%s < /tmp/GFILES.%s > /tmp/FFILES.%s\n",
			pid, pid, pid);
	}
	fprintf(scriptfd, "if  [ -s /tmp/FIND.%s ]\nthen\n\techo '%s' 1>&2\nfi\n",
		pid, FindErrorOutput);
	fprintf(scriptfd,
		"if [ ! -s /tmp/FFILES.%s ]\nthen\n\techo '%s' 1>&2\nelse\n",
		pid, ERR_fld);
	fprintf(scriptfd, "\t%s -p %s %s\nfi\n",
		GetXWINHome("adm/dtindex"), pid, flag==B_IMMEDIATE? "-v": "");
	fprintf(scriptfd, "/usr/bin/rm -f /tmp/FIND.%s\n", pid);
	fprintf(scriptfd, "/usr/bin/rm -f /tmp/IGNORE.%s\n", pid);
}

static	Boolean
BkupScript(FILE *scriptfp)
{
    char *source, *ptr;
    Boolean	local_only = FALSE;		/* backup local files only ? */
    int		tapenum = 0;
    char	*str_tapenum;

    if ((source = getSource()) == NULL)
    {
	char buf[BUFSIZ];

	fclose(scriptfp);
	sprintf(buf, GetGizmoText(string_saveFailed));
        ErrorNotice(buf, GetGizmoText(string_backupError));
		
	return False;
    }
    
    XtSetArg(arg[0], XtNset, &local_only);
    OlFlatGetValues(w_log, 0, arg, 1);

    if ((str_tapenum = strstr(curalias, "tape")) != 0)
		tapenum = atoi(str_tapenum + 4);
	
    fprintf(scriptfp, "#!/bin/sh\n\n\
#Backup Script File - DO NOT EDIT\n\n\
Class=%d\n\
Type=%d\n\
Home=%s\n\
Source=%s\n\
Local=%d\n\
Extension=%s\n\
Tapenum=%d\n\
\n\
if /sbin/tfadmin -t dtbackup 2>/dev/null\n\
then\n\
\tbackupCmd=\"/sbin/tfadmin dtbackup -s $Source -t $Type -h $Home -l $Local -e $Extension -n $Tapenum\"\n\
else\n\
\tbackupCmd=\"%s -s $Source -t $Type -h $Home -l $Local -e $Extension -n $Tapenum\"\n\
fi\n\
eval $backupCmd << \"%s\"\n",
	    bkup_class, bkup_type, home, source, local_only, pid, tapenum,
	    GetXWINHome("adm/dtbackup.sh"), HERE_DOC_WORD);
    for (ptr = bkup_source; *ptr== ' '; ptr++)
	;
    for (; *ptr; ptr++)
    {
		if (*ptr == ' ')
		{
	    	fputc('\n', scriptfp);
	    	while (*ptr == ' ')
				++ptr;
		}
		if (*ptr)
	    	fputc(*ptr, scriptfp);
		else 
			ptr--;
    }
    fprintf(scriptfp, "\n%s\n", HERE_DOC_WORD);
    if (curdev)
	FREE(source);
    fclose(scriptfp);
    return True;
}

void
NotePidFiles()	/* temporary files common to Backup and Restore */
{
	char	buf[BUFSIZ], tempfile[BUFSIZ], *full_path, *ptr;
	char	temp[]="XXXXXX";
	int	fd;
	struct	passwd *getpwuid(), *pwuid;
	uid_t	getuid(), uid;
	long	lrand48(), num;

	if ((full_path = getenv("HOME")) == NULL) {
		uid = getuid();
		pwuid = getpwuid(uid);
		full_path = STRDUP(pwuid->pw_dir);
	}

	num = lrand48();
	sprintf(buf, "%d", num);
	buf[9] = '\0';
	sprintf(tempfile, "%s/%s.bkup", full_path, buf);

	while (access(tempfile, R_OK) == 0) {
		/* file exists, try to generate another one */
		num = lrand48();
		sprintf(buf, "%d", num);
		buf[9] = '\0';
		sprintf(tempfile, "%s/%s.bkup", full_path, buf);
	}

	/* the file name is unique, save the uniqe number */
	sprintf(pid, "%s", buf);

	sprintf(buf, "%s.%s", flpindex, pid);
	pidindex = STRDUP(buf);
	_DtamNoteTmpFile(pidindex);
}

static	void
backupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
struct	stat	 fbuf;
	FILE	*fp;
	char	 buf[256];
	int	 media_type;

	NotePidFiles();

	XtSetArg(arg[0], XtNset, &save_log);
	OlFlatGetValues(w_log, 1, arg, 1);
	if (curdev == NULL) {
		XtSetArg(arg[0], XtNstring, &bkup_doc);
		XtGetValues(w_target, arg, 1);
		if (bkup_doc == NULL || *bkup_doc == '\0') {
			FooterMsg(bbase, GetGizmoText(string_bdocTitle));
			return;
		}
		else if (stat(bkup_doc,&fbuf) == 0) {
			sprintf(buf, GetGizmoText(string_newFile), bkup_doc);
			FooterMsg(bbase, buf);
			return;
		}
	}
	FooterMsg(bbase, NULL);

	/* Blocks is used in startBackup() as a flag to indicate
	 * when a backup should be kicked off.  For cpio backups
	 * Blocks is set when creating the index, since NDS 
	 * backups don't need to create an index we'll set 
	 * Blocks here.
	 */
	if (bkup_class == CLS_NDS)
		Blocks=1;

	/* Check the device first before creating index */
	if (curdev != NULL) {
		vol_count = 1;
		volume = DtamDevAttrTxt(curdev,"volume");
		media_type = DtamCheckMedia(curalias);
		if (_dtam_flags & DTAM_MOUNTED) {
			if (!mounted.shell)
				CreateGizmo(bbase.shell,ModalGizmoClass,
					&mounted,NULL,0);
			SetModalGizmoMessage(&mounted,
				GetGizmoText(string_mounted));
			MapGizmo(ModalGizmoClass, &mounted);
			return;
		}

		switch (media_type) {

			case NO_DISK:
				insert_bnote(&bnote, NO_DISK, 
					(_Dtam_inputProcData *)NULL);
				break;
			case UNFORMATTED:
				startFormat(0);
				break;

			case DTAM_UNKNOWN:
			case DTAM_UNREADABLE:
				/* DTAM_UNKNOWN: formatted, not "occupied" type (i.e. no data)*/
				/* DTAM_UNREADABLE: most likely empty tape; perhaps bad floppy*/
				if (!WriteableMedium((_Dtam_inputProcData *)NULL))
					break;
				callBackup();
				break;
			default:
				if (media_type > 0) {
					if (!in_use.shell)
						CreateGizmo(bbase.shell,
							ModalGizmoClass, &in_use,NULL,0);
					SetModalGizmoMessage(&in_use,
						GetGizmoText(string_in_use));
					MapGizmo(ModalGizmoClass,&in_use);
					break;
				}
				break;
		}
					
	}
	else {
		callBackup();
	}
}

static void
startIndex() 
{
static	Boolean	 first = TRUE;
		FILE    *fp;
		char     buf[256];
static  _Dtam_inputProcData *indexData;

	/*
	 *	note the temporary files for removal on exit
	 */
	first = FALSE;
	sprintf(buf, "/tmp/%s.bkup", pid);
	indexscript = STRDUP(buf);
	_DtamNoteTmpFile(indexscript);
	sprintf(buf, "/tmp/FFILES.%s", pid);
	_DtamNoteTmpFile(buf);
	buf[5] = 'G';
	_DtamNoteTmpFile(buf);

	if (access(GetXWINHome("adm/dtindex"), X_OK) != 0)
                ErrorNotice(GetGizmoText(string_cantIndex), 
                                         GetGizmoText(string_backupError));
	else {
		if (bkup_source == NULL)
			InfoNotice(GetGizmoText(string_noSelectedFiles));
		else {
			FindErrors = FALSE;
			indexData = (_Dtam_inputProcData *)
			    MALLOC(sizeof(_Dtam_inputProcData));
			indexData-> appContext = App_con;
			indexData-> client_data = (XtPointer)indexscript;
			StartBkupWatch(indexData);
			fp = fopen(indexscript, "w");
			IndexCmd(fp, B_IMMEDIATE);
			fclose(fp);
			chmod(indexscript, 00755);
			_Dtam_p3openInput(indexscript, FetchIndex, FetchIndex,
				  indexData, FALSE); 
		}
	}
}

static	void
schedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FILE	*fp;
	char	*file, buf[PATH_MAX + 80 + 30];
	char	*full_path;
	struct	passwd	*getpwuid(), *pwuid;
	uid_t	getuid(), uid;

	NotePidFiles();

	if ((full_path = getenv("HOME")) == NULL) {
		uid = getuid();
		pwuid = getpwuid(uid);
		full_path = STRDUP(pwuid->pw_dir);
	}
		/* LEFT: add the left `"' here, the right `"' will
		 * be added below, grep for `RIGHT'.
		 * The reason for having these two `"' is because
		 * libGizmo only passes argv[1] to DtDnDNewTransaction() */
	sprintf(buf, "%s \"%s/%s.bkup", GetXWINHome("bin/dtsched"), full_path, pid);
	file = strstr(buf, full_path);
	if ((fp = fopen(file, "w")) == NULL)
        {
	    char error_buf[PATH_MAX + 80];

	    sprintf(error_buf, GetGizmoText(schedCantWriteScript),file);
            ErrorNotice(error_buf, GetGizmoText(string_backupError));
	    return;
	}		    
	if (BkupScript(fp) == False)
	    return;
	chmod(file, 00755);


		/* RIGHT: Tell dtsched that this is a read only entry...
		 * Add right `"' here, grep for `LEFT' */
	strcat(buf, " #!@ Do not edit this line !@\"&");
	system(buf);
	FooterMsg(bbase, GetGizmoText(string_callSched));
}

static	void
BdescCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char		*desc;
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	register  int	n = olcd->item_index;
	char		*tmpalias;

	if (curdev)
		FREE(curdev);
	if (n == 0) {	/* backup to directory */
		desc = bdoc_alias;
		curalias = STRDUP(bdoc_alias);
		curdev = NULL;
	}
	else {
		curalias = DtamMapAlias(BkupDevice[olcd->item_index].label);
		tmpalias = STRDUP(curalias);
		curdev = DtamGetDev(strcat(tmpalias, ":"), FIRST);
		FREE(tmpalias);
		desc = DtamDevDesc(curdev);
	}

	bkup_alias = STRDUP(curalias);

	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);
	
	XtSetArg(arg[0], XtNlabel, (XtArgVal)desc);
	XtSetValues(w_ud, arg, 1);

	XtSetArg(arg[0], XtNmappedWhenManaged, n==0);
	XtSetValues(XtParent(w_target), arg, 1);
	if (n == 0)
		OlSetInputFocus(w_target, RevertToNone, CurrentTime);
	FooterMsg(bbase, SummaryMsg());
}

static	void
SetBkupDoc(Widget wid, XtPointer client_data, XtPointer call_data)
{
	struct	stat		fbuf;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	char			msgbuf[BUFSIZ];

	if (stat(verify->string,&fbuf) == 0) {
		sprintf(msgbuf, GetGizmoText(string_newFile), verify->string);
		FooterMsg(bbase, msgbuf);
	}
	else {
		bkup_doc = STRDUP(verify->string);
		FooterMsg(bbase, SummaryMsg());
	}
}

Boolean
NWSInstalled()
{
	if (access("/usr/sbin/ndsbackup", F_OK) == 0) 
		return True;
	else
		return False;
}

void
CreateBackupWindow(Widget parent, char **drop_list, char *atomName)
{
static	ExclItem	BkupClass[5];
static	ExclItem	BkupType[2];
static	MBtnItem	BCheckBox[2];
static	MBtnItem	BAction[2];
	Widget		w_ctl, w_cap, w_action, w_filemenu, b_abvbtn;
	char		buf[256];
	struct passwd	*getpwuid(), *pwuid;
	uid_t		getuid(), uid;
	int		i, source_size;
	char		*tmpalias;
static  Boolean	 	backup_owner;
static  Boolean	 	nds_owner;
	Boolean	 	nws_installed;
	int		num_rows;
	int		num_buts;

	SetLocaleTags();
	if (note.shell)
		XtDestroyWidget(note.shell);
	if ((home = user_home = getenv("HOME")) == NULL) {
		/* get home directory from the passwd file */
		uid = getuid();
		pwuid = getpwuid(uid);
		home = user_home = STRDUP(pwuid->pw_dir);
	}
	if (drop_list) {
		bkup_class = CLS_FILES;
		source_size = 2;
		for (i=0; drop_list[i]; i++)
			source_size += strlen(drop_list[i]) + 1;
   		bkup_source = copy_source = (char *)MALLOC(source_size);
		strcpy(copy_source,"\n");
		for (i=0; drop_list[i]; i++){
			strcat(copy_source, drop_list[i]);
			strcat(copy_source, "\n");
		}
	}
	else {
		bkup_source = user_home;
		/*
 		 * if a previous complete backup was done,
		 * default becomes incremental instead of complete
		 */
		sprintf(buf, "%s/%s", user_home, BKUP_HIST);
		if (access(buf,R_OK) == 0)
			bkup_type = BKUP_INCR;
	}
	dir_fcrec.glyph = DmGetPixmap(theScreen, "dir.icon");
	doc_fcrec.glyph = DmGetPixmap(theScreen, "datafile.icon");
	nds_fcrec.glyph = DmGetPixmap(theScreen, "Part32.xpm");
	MakeUserList();
/*
 *	create base window
 */
	bbase.icon_name = GetGizmoText(bbase.icon_name);
	w_ctl = CreateMediaWindow(parent, &bbase, NULL, 0, &w_action);
	w_bkmenu = GetMenu(GetSubMenuGizmo(bbase.menu, 1));
	XtSetArg(arg[0], XtNsensitive, bkup_class==CLS_FILES);
	OlFlatSetValues(w_bkmenu, 0, arg, 1);

	w_filemenu = GetMenu(GetSubMenuGizmo(bbase.menu, 0));
	XtSetArg(arg[0], XtNclientData, atomName);
	OlFlatSetValues(w_filemenu, 0, arg, 1);

	XtSetArg(arg[0], XtNhPad,	x3mm);
	XtSetArg(arg[1], XtNhSpace,	x3mm);
	XtSetArg(arg[2], XtNvPad,	y3mm);
	XtSetArg(arg[3], XtNvSpace,	y3mm);

	XtSetValues(w_ctl, arg, 4);
/*
 *	create doc/device abbreviated button menu
 */
	BkupDevice[0].label = bdoc_alias = GetGizmoText(label_doc);
	w_bdesc = DevMenu(BkupDevice, 1, N_DEVS, w_ctl, 
			GetGizmoText(label_bkupToCaption),
			(XtPointer)BdescCB, "removable=\"true", 
			&w_devmenu, &b_abvbtn, 1);

	if (bkup_alias) {
		tmpalias = malloc(strlen(bkup_alias)+2);
		sprintf(tmpalias, "%s:", bkup_alias);
                curdev = DtamGetDev(tmpalias, FIRST);
                FREE(tmpalias);
		curalias = STRDUP(bkup_alias);
	}
		
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_bdesc, arg, 1);
/*
 *	controls specific to Backup
 */
	XtSetArg(arg[0], XtNlabel,	 	GGT(label_targetCaption));
	XtSetArg(arg[1], XtNspace,	 	x3mm);
	XtSetArg(arg[2], XtNmappedWhenManaged,	FALSE);
	XtSetArg(arg[3], XtNposition,	 	OL_LEFT);

	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 4);
	
	XtSetArg(arg[0], XtNcharsVisible, 37);
	w_target = XtCreateManagedWidget("textfield",
			textFieldWidgetClass, w_cap, arg, 1);
	XtAddCallback(w_target, XtNverification, SetBkupDoc, NULL);

        if (restricted_flag)
	{
	    /* only allow immediate backup of file specified  */
	    /*  in -C option to MediaMgr */
	    bkup_type  = BKUP_COMPL;
	    bkup_class = CLS_USERS;
	    OlVaFlatSetValues(bbkup_menu.child, 0, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 1, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 2, XtNsensitive, FALSE, NULL);
	    OlVaFlatSetValues(bfile_menu.child, 3, XtNsensitive, FALSE, NULL);
	}
        else {
	    XtSetArg(arg[0], XtNlabel,	GGT(label_bkupTypeCaption));
	    XtSetArg(arg[1], XtNposition,	OL_LEFT);
	    XtSetArg(arg[2], XtNspace,	x3mm);

	    w_cap = XtCreateManagedWidget("bkuptypecaption",
					  captionWidgetClass, w_ctl, arg, 3);
	    SET_EXCL(BkupType, 0, complType,   bkup_type==0);
	    SET_EXCL(BkupType, 1, incrType,    bkup_type==1);

	    XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	    XtSetArg(arg[1], XtNbuttonType,	OL_RECT_BTN);
	    XtSetArg(arg[2], XtNexclusives,	TRUE);
	    XtSetArg(arg[3], XtNitemFields,	ExclFields);
	    XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
	    XtSetArg(arg[5], XtNitems,		BkupType);
	    XtSetArg(arg[6], XtNnumItems,	2);
	    XtSetArg(arg[7], XtNselectProc,	SetBkupType);
	    XtSetArg(arg[8], XtNsameWidth,	OL_ALL);

	    w_type = XtCreateManagedWidget("typeexcl",
					   flatButtonsWidgetClass, w_cap,
					   arg, 9);

	    XtSetArg(arg[0], XtNlabel,	GGT(label_bkupClassCaption));
	    XtSetArg(arg[1], XtNposition,	OL_LEFT);
	    XtSetArg(arg[2], XtNspace,	x3mm);
	    w_cap = XtCreateManagedWidget("classcaption",
	       			      captionWidgetClass, w_ctl,
	    			      arg, 3);

	    /* Privilege and NWS installation determine number of 
	     * buttons & rows to display under "Backup Data"
	     * on backup window
	     */
	    backup_owner = _DtamIsOwner(OWN_BACKUP);
	    nds_owner = _DtamIsOwner(OWN_NDS);
	    num_rows = 1;
	    num_buts = 2;
	    if (backup_owner || nds_owner) {
	    	num_rows = 2;
	    	num_buts = 4;
	        if (nws_installed = NWSInstalled()) {
	            num_buts = 5;
	        }
	    }
	    	

	    /* Set up Backup Data buttons depending on privilege */
	    SET_EXCL(BkupClass, CLS_SELF, selfClass,   TRUE);
	    SET_EXCL(BkupClass, CLS_FILES, selectFiles, FALSE);
	    if (nds_owner || backup_owner) {
		SET_EXCL(BkupClass, CLS_USERS, userClass, FALSE);
		SET_EXCL(BkupClass, CLS_SYSTEM, systemClass, FALSE);
		if (nws_installed) {
		    SET_EXCL(BkupClass, CLS_NDS, ndsData, FALSE);
		}
	    }

	    XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	    XtSetArg(arg[1], XtNbuttonType,	OL_RECT_BTN);
	    XtSetArg(arg[2], XtNexclusives,	TRUE);
	    XtSetArg(arg[3], XtNitemFields,	ExclFields);
	    XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
	    XtSetArg(arg[5], XtNitems,		BkupClass);
	    XtSetArg(arg[6], XtNnumItems,	num_buts);
	    XtSetArg(arg[7], XtNselectProc,	SetBkupData);
	    XtSetArg(arg[8], XtNlayoutType,	OL_FIXEDROWS);
	    XtSetArg(arg[9], XtNmeasure,	num_rows);

	    w_class = XtCreateManagedWidget("classexcl",
	    				flatButtonsWidgetClass, w_cap,
	    				arg, 10);

	    /* Desensitize CLS_SYSTEM & CLS_USERS buttons if the user
	     * has nds permission but not owner permission
	     */
	    if (nds_owner  &&  !backup_owner) {
		XtSetArg(arg[0], XtNsensitive, FALSE);
		OlFlatSetValues(w_class, CLS_SYSTEM, arg, 1);
		OlFlatSetValues(w_class, CLS_USERS, arg, 1);
	    }
	}

	SET_BTN(BCheckBox, 0, local, NULL);
	SET_BTN(BCheckBox, 1, log, NULL);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNitemFields,		MBtnFields);
	XtSetArg(arg[2], XtNnumItemFields,	NUM_MBtnFields);
	XtSetArg(arg[3], XtNitems,		BCheckBox);
	XtSetArg(arg[4], XtNnumItems,		XtNumber(BCheckBox));
	XtSetArg(arg[5], XtNexclusives,		FALSE);
	XtSetArg(arg[6], XtNbuttonType,		OL_CHECKBOX);
	XtSetArg(arg[7], XtNsameWidth,		OL_NONE);

	w_log = XtCreateManagedWidget("checkbox",
			flatButtonsWidgetClass,w_ctl, arg, 8);
/*
 *	Backup window: icon box for "Select Files" mode
 */
	BkupTarget = MakeObjectList(bkup_source, &bkup_count);
	XtSetArg(arg[0], XtNwidth,	WIDTH);
	XtSetArg(arg[1], XtNheight,	(Dimension)HEIGHT/(Dimension)2);
	XtSetValues(bbase.scroller, arg, 2);

	w_icons = GetIconBox(bbase.scroller, bkup_count);
/*
 *	Backup window: Backup Now / Backup Later...
 */
	SET_BTN(BAction, 0, now, backupCB );
	SET_BTN(BAction, 1, later, schedCB);

	XtSetArg(arg[0], XtNitems,		BAction);
	XtSetArg(arg[1], XtNnumItems,		XtNumber(BAction));
	XtSetArg(arg[2], XtNsameWidth,		OL_NONE);
	XtSetArg(arg[3], XtNnoneSet,		TRUE);
	XtSetValues(w_action, arg, 4);
        if (restricted_flag) /* -L specified */
	{
	    /* only allow immediate backup of file specified  */
	    /*  in -C option to MediaMgr */
	    OlVaFlatSetValues(w_action, 1, XtNsensitive, FALSE, NULL);
	}
	type_label[0] = BkupType[0].label;
	type_label[1] = BkupType[1].label;
	type_label[2] = BkupType[2].label;
	FooterMsg(bbase, SummaryMsg());
	CreateGizmo(bbase.shell, ModalGizmoClass, &bnote, NULL, 0);
	CreateGizmo(bbase.shell, FileGizmoClass, &open_prompt, NULL, 0);
	MapGizmo(BaseWindowGizmoClass, &bbase);
        BkRegisterHelp();

	OlSetInputFocus(b_abvbtn, RevertToNone, CurrentTime);
	OlVaFlatSetValues(w_action, 0, XtNdefault, True, NULL);

	if (cur_file)
		OpenFile();
	{
	OlDnDDropSiteID drop_site_id;
        XtVaGetValues(w_icons, XtNdropSiteID, &drop_site_id, NULL);
        OlDnDSetDropSiteInterest(drop_site_id,
                                (bkup_class == CLS_FILES) ?
                                      True : False);
	}
}

char *
nbfgets (char *buf, int cnt, FILE *pFile)
{
    int		n, max;
    char	*cp;

    /* Get a line from a file that was opened nodelay.  If the string
     * doesn't end with a newline, we had a timing problem.  Pause, and
     * then try again.  This assumes the last line ends with a newline.
     */

    if (!fgets (buf, cnt, pFile))
	return (0);

    n = strlen (buf);
    if (buf [n-1] == '\n')
	return (buf);

    cp = buf + n;
    cnt -= n;
    max = 5;
    while (cnt > 1 && --max >= 0)
    {
	sleep (1);

	if (fgets (cp, cnt, pFile))
	{
	    n = strlen (cp);
	    if (cp [n-1] == '\n')
		return (buf);

	    cp += n;
	    cnt -= n;
	}
    }
    return (buf);
}
