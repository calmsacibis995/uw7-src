#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/restore.c	1.73.2.1"
#endif

#include <fcntl.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include "media.h"

extern	void		NotePidFiles();
extern	void		remap_bkup();
extern	void		ErrorNotice();
extern	void		OKCB();
extern  void		ShowNDSErrors();
extern  Boolean		NWSInstalled();

static Boolean 	watch = False;

extern	FILE		*ptyfp;
extern	char		*ptyname;
extern	char		*pidindex;
extern	char		*EOM;
extern	char		*volume;
extern	int		vol_count;
extern	char            *CpioCmd;
extern	char            *NDSCmd;
extern	char            *rst_alias;
extern	BaseWindowGizmo bbase;

static	Boolean		wait;
static	XtIntervalId	TID = 0;

void	restoreCB();
void	showFilesCB();
void	selectCB();
void	unselectCB();
void	insertCB();
void	rhelpCB();
void	rkillCB();
Boolean	RestorePrep();
void	InsertNotice();
char	* StoreIndexNumber(char *);
void	prevGroupCB();
void	WatchRestore(XtPointer closure, XtIntervalId intid);
void	nextGroupCB();
void	tooManyCB();
void	fileSelectCB();
void	fileUnselectCB();
static Boolean SetFilesInDir(char *dirname);
static void RstRegisterHelp();
static void ShowNotice();
static void showOKCB();
static void gotoBackupCB();
extern	char	*nbfgets (char *buf, int cnt, FILE *pFile);
static void 	errCB(Widget wid, XtPointer client_data, XtPointer call_data);

static int action = 0;

#define	SHOWFILES 1
#define RESTORE 2
#define STOP 3

static MenuItems raction_menu_item[] = {
	{ TRUE, label_gotoBack,mnemonic_gotoBack, 0, gotoBackupCB},
	{ TRUE, label_restore,mnemonic_restore, 0, restoreCB},
	{ TRUE, label_show,   mnemonic_show, 0, showFilesCB},
	{ TRUE, label_exit,   mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems redit_menu_item[] = {
	{ TRUE, label_select,  mnemonic_select, 0, selectCB},
	{ TRUE, label_unselect,mnemonic_unselect, 0, unselectCB},
	{ NULL }
};

static HelpInfo RHelpIntro	= { 0, "", BHELP_PATH, help_rst_intro };
static HelpInfo RHelpConfirm	= { 0, "", BHELP_PATH, help_rst_intro };
static HelpInfo RHelpDoingRst	= { 0, "", BHELP_PATH, help_rst_doing };
static HelpInfo RHelpTOC	= { 0, "", BHELP_PATH, NULL };
static HelpInfo RHelpDesk       = { 0, "", BHELP_PATH, "HelpDesk"  };

static OlDtHelpInfo help_info[] = {NULL, NULL, BHELP_PATH, NULL, NULL};

static MenuItems rhelp_menu_item[] = {  
	{ TRUE, label_bkrst, mnemonic_bkrst, 0, rhelpCB, (char *)&RHelpIntro },
	{ TRUE, label_toc,   mnemonic_toc,   0, rhelpCB, (char *)&RHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, rhelpCB, (char *)&RHelpDesk },
	{ NULL }
};

static MenuGizmo raction_menu = {0, "action_menu", NULL, raction_menu_item};
static MenuGizmo redit_menu   = {0, "edit_menu", NULL, redit_menu_item};
static MenuGizmo rhelp_menu   = {0, "help_menu", NULL, rhelp_menu_item};

static MenuItems rmain_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &raction_menu},
	{ TRUE, label_edit,   mnemonic_edit, (Gizmo) &redit_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &rhelp_menu},
	{ NULL }
};
static MenuGizmo rmenu_bar = { 0, "menu_bar", NULL, rmain_menu_item};

BaseWindowGizmo rbase = {0, "base", label_restore, (Gizmo)&rmenu_bar,
	NULL, 0, label_restore, "restore48.icon", " ", " ", 90 };

static MenuItems rwatch_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, rkillCB, (XtPointer) 1 , NULL},
	{ TRUE, label_help,   mnemonic_help, 0, rhelpCB, (char *)&RHelpDoingRst },
	{ NULL }
};
static MenuGizmo rwatch_menu = {0, "rwatch_menu", NULL, rwatch_menu_item};
static PopupGizmo rwatch = {0, "popup", title_doingRst, (Gizmo)&rwatch_menu};

static MenuItems rnote_menu_item[] = {  
	{ TRUE, label_continue, mnemonic_continue, 0, insertCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, rkillCB, (XtPointer) 2, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, rhelpCB,(char *)&RHelpConfirm },
	{ NULL }
};
static MenuGizmo rnote_menu = {0, "rnote_menu", NULL, rnote_menu_item};
static ModalGizmo rnote = {0, "", title_confirmRst, (Gizmo)&rnote_menu};

static MenuItems errnote_item[] = {
        { TRUE, label_ok,  mnemonic_ok, 0, errCB },
        { NULL }
};

static  MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static  ModalGizmo errnote = {0, "warn", string_restoreError, (Gizmo)&errnote_menu };

static  MenuItems shownote_item[] = {
	{ TRUE, label_ok, mnemonic_ok, 0, showOKCB },
	{ NULL }
};

static MenuGizmo shownote_menu = {0, "info", "info", shownote_item };
static ModalGizmo shownote = {0, "info", string_infoTitle, (Gizmo)&shownote_menu};

DevItem		RstDevice[N_DEVS];

char	*rdoc_alias;
char	*rst_doc;
int	last_restore;
int	restore_count;
boolean_t	restore_privs_flag = B_FALSE;
boolean_t	privileged_user = B_FALSE;

typedef	struct	{
	XtArgVal	f_name;
	XtArgVal	f_set;	
} ListItem, *ListPtr;

static char	tfadminpath[50] = "/sbin/tfadmin";

static char * flatMenuFields[] =
   {
      XtNsensitive,  /* sensitive                      */
      XtNlabel,      /* label                          */
      XtNuserData,   /* mnemonic string                */
      XtNuserData,   /* nextTier | resource_value      */
      XtNselectProc, /* function                       */
      XtNclientData, /* client_data                    */
      XtNset,        /* set                            */
      XtNpopupMenu,  /* button                         */
      XtNuserData,   /* mnemonic                       */
   };

#define	INIT_FILELIST_SIZE	50	
#define	INIT_GROUP_SIZE  	5000
static int filelist_size =	0;
static Cardinal group_size = INIT_GROUP_SIZE; /* max items that fit in a list */
static int show_num;
static int group_show;		/* which one to show */
static int group_max;		/* total number to show */
static int select_count;	/* number currently selected */


static MenuItems change_group_menu_items[] = {  
	{ FALSE, NULL, NULL, 0, prevGroupCB, NULL },
	{ FALSE, NULL, NULL, 0, nextGroupCB, NULL }
};
			
#define XtNitemsLimitExceeded "itemsLimitExceeded"

ListPtr	filelist = (ListPtr)NULL;
int	file_count = 0;
int	rst_format;
Boolean	init_cpio;
char	*bkup_index = NULL;
char	rst_buf[256];
char	*ListFields[] = { XtNlabel, XtNset };

static	Widget	w_rstmsg, w_file, w_list, w_opt, w_opt2, w_group, 
		w_groupcaption, w_rdesc; 

char	*ERR_fld  = " ERROR:";
char	*ERR_tag  = "UX:cpio: ERROR:";
char	*WARN_tag  = "UX:cpio: WARNING:";
char	*NDS_ERR  = "UX:ndsbackup: ERROR:";
char	*HALT_tag = "UX:cpio: HALT:";
char	*END_tag  = " blocks";
char	*SKP1_tag = "Existing \"";
char	*SKP2_tag = "\" same age or newer";

#define	NONE_SET	0
#define	SOME_SET	1
#define	ALL_SET		2

static void RstRegisterHelp()
{
#ifdef TRACE
	fprintf(stderr,"restore.c: RstRegisterHelp\n");
#endif
      help_info->filename =  BHELP_PATH;
      help_info->title    =  GetGizmoText(label_restore);
      help_info->section = GetGizmoText(STRDUP(help_rst_intro));
      OlRegisterHelp(OL_WIDGET_HELP, rbase.shell, "MediaMgr", OL_DESKTOP_SOURCE,                (XtPointer)&help_info);
}

static void
ShowNotice(char *buf)
{
        int     n=0;
        if (!shownote.shell) {
                XtSetArg(arg[n], XtNnoticeType, OL_INFORMATION); n++;
                CreateGizmo(w_toplevel, ModalGizmoClass, &shownote, arg, n);
        }
        SetModalGizmoMessage(&shownote, buf);
        XtVaSetValues(shownote.stext, XtNalignment, (XtArgVal)OL_LEFT,
                NULL);
        MapGizmo(ModalGizmoClass, &shownote);
}

static void
showOKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        BringDownPopup(shownote.shell);
}

static void
ErrNotice (char *buf)
{
        if (!errnote.shell)
         CreateGizmo(rbase.shell, ModalGizmoClass, &errnote, NULL, 0);
         SetModalGizmoMessage(&errnote, buf);
         OlVaFlatSetValues(errnote_menu.child, 0, XtNclientData,
                      (XtArgVal)0, 0);
         MapGizmo(ModalGizmoClass, &errnote);
}

static void
errCB(Widget wid, XtPointer client_data, XtPointer call_data)
{

    BringDownPopup(errnote.shell);
}


void	SetLocaleTags(void)
{
	FILE	*tmpfp;
	char	fname[16];
	char	buf[BUFSIZ];
	char	*ptr;

#ifdef TRACE
	fprintf(stderr,"restore.c: SetLocaleTags\n");
#endif
	sprintf(fname,"/tmp/%d", getpid());
	if (tmpfp = fopen(fname,"w")) {
		setlocale(LC_ALL, "");
		pfmt(tmpfp, MM_ERROR, NULL);
		fputc('\n',tmpfp);
		setlabel("UX:cpio");
		pfmt(tmpfp, MM_ERROR, NULL);
		fputc('\n',tmpfp);
		pfmt(tmpfp, MM_HALT, NULL);
		fputc('\n',tmpfp);
		fclose(tmpfp);
		if (tmpfp = fopen(fname,"r")) {
			if (fgets(buf, BUFSIZ, tmpfp)) {
				ERR_fld = STRDUP(buf);
				ERR_fld[strlen(ERR_fld)-1] = 0;
				if (fgets(buf, BUFSIZ, tmpfp)) {
					ERR_tag = STRDUP(buf);
					ERR_tag[strlen(ERR_tag)-1] = 0;
		 			if (fgets(buf, BUFSIZ, tmpfp)) {
						HALT_tag = STRDUP(buf);
						HALT_tag[strlen(HALT_tag)-1]=0;
					}
				}
			}
			fclose(tmpfp);
		}
		setlabel(NULL);
		unlink(fname);
	}
	setcat("uxcore.abi");
	sprintf(buf, gettxt(":46","Existing \"%s\" same age or newer"), "QQQ");
	if (ptr = strstr(buf, "QQQ")) {
		*ptr = '\0';
		SKP1_tag = STRDUP(buf);
		SKP2_tag = STRDUP(ptr+3);
	}
	sprintf(buf, gettxt(":57","%ld blocks"), 999);
	if (ptr = strstr(buf,"999"))
		END_tag = STRDUP(ptr+3);
	setcat("");
}

void	RstSummary()
{
	char	buf[256];

#ifdef TRACE
	fprintf(stderr,"restore.c: RstSummary\n");
#endif
	sprintf(buf,GetGizmoText(string_rstSummary),
		curdev? DtamDevDesc(curdev):
		(rst_doc ? rst_doc : GGT(label_file)));
	FooterMsg(rbase, buf);
}

void	rhelpCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(rbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_restore);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(rbase.shell, help);
}

void	DropRstWatch(XtPointer closure, XtIntervalId id)
{
#ifdef TRACE
	fprintf(stderr,"restore.c: DropRstWatch\n");
#endif
	XDefineCursor(theDisplay, XtWindow(rwatch.shell),
						GetOlStandardCursor(theScreen));
/*	XDefineCursor(theDisplay, XtWindow(w_gauge),
/*						GetOlStandardCursor(theScreen));
*/	XDefineCursor(theDisplay, XtWindow(rbase.shell),
						GetOlStandardCursor(theScreen));
	if (rwatch.shell) {
		watch = False;
		BringDownPopup(rwatch.shell);
	}
	if (bkup_index) {
		unlink(bkup_index);
		FREE(bkup_index);
		bkup_index = NULL;
	}
}

void	StartRstWatch(char *msg, char *title)
{
	Widget	w_up;

#ifdef TRACE
	fprintf(stderr,"restore.c: StartRstWatch\n");
#endif
	if (!rwatch.shell) {
		CreateGizmo(rbase.shell, PopupGizmoClass, &rwatch, NULL, 0);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetArg(arg[1], XtNwidth,	  36*x3mm);
		XtSetArg(arg[2], XtNresize,	  FALSE);
		XtSetValues(rwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(rwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNwidth,		36*x3mm);
		XtSetArg(arg[4], XtNhPad,		x3mm);
		XtSetArg(arg[5], XtNvPad,		y3mm);
		XtSetArg(arg[6], XtNvSpace,		y3mm);
		XtSetValues(w_up, arg, 7);

		XtSetArg(arg[0], XtNheight, 3*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNalignment,		OL_CENTER);
		XtSetArg(arg[1], XtNgravity,		CenterGravity);
		XtSetArg(arg[2], XtNwidth,		36*x3mm);
		XtSetArg(arg[3], XtNfont, 		bld_font);
		w_rstmsg = XtCreateManagedWidget("text",
				staticTextWidgetClass, w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 2*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNspan,        	32*x3mm);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetArg(arg[2], XtNorientation, 	OL_HORIZONTAL);
		XtSetArg(arg[3], XtNminLabel,    	" 0 %");
		XtSetArg(arg[4], XtNmaxLabel,    	"100 %  ");
		XtSetArg(arg[5], XtNsliderMax,   	100);
		XtSetArg(arg[6], XtNsliderValue, 	0);
		XtSetArg(arg[7], XtNshowValue,   	TRUE);
		XtSetArg(arg[8], XtNtickUnit,		OL_PERCENT);
		XtSetArg(arg[9], XtNticks,		10);
	 	w_gauge = XtCreateManagedWidget("gauge",
				gaugeWidgetClass, w_up, arg, 10);
	}
	XtSetArg(arg[0], XtNstring, (XtArgVal)msg);
	XtSetValues(w_rstmsg, arg, 1);
	XtVaSetValues(rwatch.shell, XtNtitle, title, 0);
	MapGizmo(PopupGizmoClass, &rwatch);
	XDefineCursor(theDisplay, XtWindow(rwatch.shell),
						GetOlBusyCursor(theScreen));
	XDefineCursor(theDisplay, XtWindow(rbase.shell),
						GetOlBusyCursor(theScreen));
	watch = True;
}

void	rkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[256];
	int	type;
#ifdef TRACE
	fprintf(stderr,"restore.c: rkillCB\n");
#endif

	type = (int) client_data;
	FooterMsg(rbase, NULL);
	if (cmdfp[1] && cmdfp[0]) {
		_Dtam_p3close(cmdfp, SIGTERM);
		cmdfp[1] = cmdfp[0] = (FILE *)NULL;
	}
	else if (cmdfp[1]) {	/* in midst of reading the index file */
		fclose(cmdfp[1]);
		cmdfp[1] = (FILE *)NULL;
	}
	if (ptyfp) {
		fclose(ptyfp);
		ptyfp = (FILE *)NULL;
	}
	if (TID)
		XtRemoveTimeOut(TID);
	sprintf(buf,GetGizmoText(string_restKilled));
	FooterMsg(rbase, buf);
			/* type 2 the popup is a notice warning */
	if ((type == 2)&& (rnote.shell)) BringDownPopup(rnote.shell);
	if (watch == True) {
			/* popup is restore or find files in progress */
		XtAddTimeOut(100, (XtTimerCallbackProc)DropRstWatch, (XtPointer)NULL);
	}

}

static	int
SetOrUnsetFiles(Boolean flag, Boolean touch)
{
	register int	n;
#ifdef TRACE
	fprintf(stderr,"restore.c: SetOrUnsetFiles\n");
#endif

	if (filelist)
		for (n = 0; n < file_count; n++)
			filelist[n].f_set = (XtArgVal)flag;
	if (touch)
	{
		XtSetArg(arg[0], XtNitemsTouched, TRUE);
		XtSetValues(w_list, arg, 1);
	}
}

static	int
CheckTargetFile()
{
	int	n;
	char	buf[256];

#ifdef TRACE
	fprintf(stderr,"restore.c: CheckTargetFile\n");
#endif
	XtSetArg(arg[0], XtNstring, &rst_doc);
	XtGetValues(w_file, arg, 1);
	switch (n = diagnose(rst_doc, curalias)) {
		case BAD_DEVICE:
		case UNREADABLE:
			sprintf(buf, GetGizmoText(string_unreadFile), rst_doc);
                        ErrNotice(buf);
			break;
		case DTAM_NO_DISK:
			InsertNotice(&rnote, NO_DISK);
			return False;
		case TAR:
		case BACKUP:
		case CPIO:
		case DTAM_NDS:
			rst_format = n;
			break;
		default:
			sprintf(buf, GetGizmoText(string_notaBkup), rst_doc);
                        ErrNotice(buf);
			break;
	}
	return n;
}

void	AddToFileList(char *name)
{
#ifdef TRACE
	fprintf(stderr,"restore.c: AddToFileList\n");
#endif
	if (name[strlen(name)-1] == '\n')
		name[strlen(name)-1] = '\0';
	if (file_count >= filelist_size) {
		if (filelist_size == 0) filelist_size = INIT_FILELIST_SIZE;
		else filelist_size *= 2;
		filelist = (ListPtr)REALLOC((void *)filelist,
			filelist_size*sizeof(ListItem));
	}
	filelist[file_count].f_name = (XtArgVal)STRDUP(name);
	filelist[file_count++].f_set = (XtArgVal)FALSE;
}

static	char	buf[BUFSIZ];
static	char	BLKS[] = "BLOCKS=";

void	FetchFiles(XtPointer closure, XtIntervalId id)
{
	static		Boolean	restart;
	char		msgbuf[128], errbuf[BUFSIZ];
	static char	index_file[50] = { NULL };
	char		index_buf[BUFSIZ];
	FILE		*index_fp;
	char		*dev, *ptr;
	int		timer;
	int		fdmaster;

#ifdef TRACE
	fprintf(stderr,"restore.c: FetchFiles\n");
#endif
#ifdef DEBUG 
	fprintf(stderr, "FetchFiles action=%d\n",action);
	fprintf(stderr,"init_cpio=%d\n",init_cpio);
#endif
	if (action == STOP) return;
	if (wait)
		timer = 2000;
	else if (init_cpio) {
		init_cpio = restart = FALSE;
		file_count = 0;	/* should free existing filelist! */
		timer = 2000;

		switch (rst_format) {
		case DTAM_TAR:
			ptr = strstr(rst_buf, "Bct         ");
			strncpy(ptr, "t -H tar ", 9);
			break;
		case DTAM_BACKUP:
			sprintf (index_file,"/tmp/flp_index.%s", 
						StoreIndexNumber(NULL));
			if (bkup_index) {
				unlink(bkup_index);
				FREE(bkup_index);
				bkup_index = NULL;
			}
			unlink(index_file);
			restart = TRUE;
			ptr = strstr(rst_buf,"Bct   ");
			strncpy (ptr, "Bcv   ", 6);
			if (curdev)
				*strstr(rst_buf, "-M") = '\0';
			strcat (rst_buf," ");
			strcat (rst_buf,index_file);/* just read in one file */
			strcat(rst_buf, " 2>&1");
			break;

		}
		if (curdev == NULL || rst_format == BACKUP)
			ptyfp = (FILE *)NULL;
		else {
		/*
		 *	prepare pseudo-tty to use for volume changes
		 */
			fdmaster = open("/dev/ptmx", O_RDWR);
			grantpt(fdmaster);
			unlockpt(fdmaster);
			ptyname = ptsname(fdmaster);
			fcntl(fdmaster, F_SETFL, O_NONBLOCK);
			ptyfp = fdopen(fdmaster, "r+");
			setbuf(ptyfp, NULL);
			ptr = strstr(rst_buf, "-G ");
			strcpy(ptr+3, ptyname);
		}
		_Dtam_p3open(rst_buf, cmdfp, TRUE);
	}
	else {
		timer = 100;
		buf[0] = '\0';
		if (ptyfp && nbfgets(buf, BUFSIZ, ptyfp)) {
			if (strncmp(buf, EOM, 4)==0) {
				wait = TRUE;
				vol_count = atoi(buf+4);
				InsertNotice(&rnote, NO_DISK);
				timer = 5000;
				rewind(ptyfp);
			}
		}
		buf[0] = '\0';
		while (nbfgets(buf, BUFSIZ, cmdfp[1])) {
			int	n;
			if (*buf == '\0')
				break;
			if (strstr(buf,"BLOCKS=")!=NULL
			||  strstr(buf,END_tag)!=NULL
			|| (*index_file != 0 && 
				strncmp(buf,index_file,strlen(index_file)) ==0))
			{
			/*
			 *	done!
			 */
				if (*index_file == 0)
					_Dtam_p3close(cmdfp, 0);
				else {	/* we have the index file; process it */
					_Dtam_p3close(cmdfp, SIGTERM);
					if ((index_fp = fopen (index_file, "r")) == NULL) {
						/* do something better here */
						fprintf (stderr, "MediaMgr, FetchFiles: unable to read index file\n");
						AddToFileList (index_file);
					}
					else {
						nbfgets(index_buf,BUFSIZ,index_fp); /* skip index file */
						while (nbfgets (index_buf, BUFSIZ, index_fp))
						{
							if (strncmp(index_buf,BLKS,strlen(BLKS)) != 0)
							{ /* find tab then skip it */
								char *index_ptr = index_buf;
								while (*index_ptr != '\t')
									index_ptr++;
								AddToFileList(++index_ptr);
							}
						}
						fclose (index_fp);
						unlink(index_file);
						*index_file = 0;
					}
				}
				if (nbfgets(buf, BUFSIZ, cmdfp[1]))
					;
				cmdfp[1] = cmdfp[0] = (FILE *)NULL;
				if (bkup_index)
				{
					unlink(bkup_index);
					FREE(bkup_index);
					bkup_index = NULL;
				}
				if (ptyfp) {
					fclose(ptyfp);
					ptyfp = (FILE *)NULL;
				}
				XtAddTimeOut(100,
					(XtTimerCallbackProc)DropRstWatch,NULL);
				TID = 0;
				SetOrUnsetFiles(TRUE,FALSE);	/* set true; don't update X */

				/* now display the first group of files */
				show_num = file_count>group_size ? 
							group_size:file_count;
				XtSetArg(arg[0], XtNitems,        filelist);
				XtSetArg(arg[1], XtNnumItems,     show_num);
				XtSetArg(arg[2], XtNviewHeight,   7);
				XtSetArg(arg[3], XtNviewItemIndex,show_num-1);
				XtSetValues(w_list, arg, 4);
				if (file_count > group_size)
				{	/* more than one group of files; display first group */
					show_num = group_size;
					XtSetArg(arg[0],XtNitems,filelist);
					XtSetArg(arg[1],XtNnumItems,show_num);
					XtSetArg(arg[2],XtNviewHeight, 7);
					XtSetArg(arg[3],XtNviewItemIndex,
					    show_num - 1);
					XtSetArg(arg[4],XtNitemsTouched, TRUE);
					XtSetValues(w_list, arg, 5);
					group_show = 0;
					group_max  = file_count/show_num;
					if (file_count % show_num == 0)
						group_max--;
					/* buttons visible; next sensitive */
					XtSetArg(arg[0], 
					    XtNmappedWhenManaged,	TRUE);
					XtSetValues(w_groupcaption, arg, 1);
					XtSetValues(w_group, arg, 1);
					change_group_menu_items[0].sensitive = 
					    (XtArgVal)FALSE;
					change_group_menu_items[1].sensitive = 
					    (XtArgVal)TRUE;
					XtSetArg(arg[0], XtNitems,
					    change_group_menu_items);
					XtSetArg(arg[1], XtNnumItems,
					    XtNumber(change_group_menu_items));
					XtSetValues(w_group, arg, 1);
				}
				else
				{
					/* make sure buttons invisible */
					XtSetArg(arg[0], 
					    XtNmappedWhenManaged, FALSE);
					XtSetValues(w_groupcaption, arg, 1);
					XtSetValues(w_group, arg, 1);
				}
				/* set footer message with number of files */
				select_count = file_count;
				sprintf(msgbuf,GetGizmoText(string_selected),
					select_count,file_count);
				FooterMsg(rbase, msgbuf);

				return;
			}
			else if (rst_format == BACKUP && !restart) {
				AddToFileList(strchr(buf,'\t')+1);
			}
			else if ((strncmp(buf,ERR_tag,n=strlen(ERR_tag))==0) 
				|| (strncmp(buf, HALT_tag,n=strlen(HALT_tag))==0)) {
				_Dtam_p3close(cmdfp, 0);
				cmdfp[1] = cmdfp[0] = (FILE *)NULL;
				if (bkup_index) {
					unlink(bkup_index);
					FREE(bkup_index);
					bkup_index = NULL;
				}
				if (ptyfp) {
					fclose(ptyfp);
					ptyfp = (FILE *)NULL;
				}
				XtAddTimeOut(100,
					(XtTimerCallbackProc)DropRstWatch, NULL);
				TID = 0;	
				sprintf(errbuf, GetGizmoText(string_showFail));
				ErrorNotice(errbuf, GetGizmoText(string_restoreError));	
				return;
			}
			else {
				buf[strlen(buf)-1] = '\0';
				if (restart) {
					_Dtam_p3close(cmdfp, 0);
					cmdfp[0] = (FILE *)NULL;
					cmdfp[1] = fopen(buf,"r");
					fcntl(fileno(cmdfp[1]), F_SETFL,
								O_NONBLOCK);
					restart = FALSE;
					bkup_index = STRDUP(buf);
				}
				else 
					AddToFileList(buf);
			}
		}
	}
	TID = XtAddTimeOut(timer, (XtTimerCallbackProc)FetchFiles, NULL);
}

void	insertCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	buf[128];
	int	diagn = DtamCheckMedia(curalias);
#ifdef TRACE
	fprintf(stderr,"restore.c: insertCB\n");
#endif

	FooterMsg(rbase, NULL);
	if (vol_count == 1)
		switch(diagn) {

	case DTAM_BACKUP:
	case DTAM_CPIO:
	case DTAM_NDS:
	case DTAM_TAR:	BringDownPopup(rnote.shell);
			wait = FALSE;
			rst_format = diagn;
			/* put up restore in progrss poup and start timeout */
			if (action == SHOWFILES) {
				showFilesCB(wid, 0, 0);
			}
			else 
			if (action == RESTORE) {
				restoreCB(wid, 0, 0);
			}
			break; 

		case DTAM_NO_DISK:
			InsertNotice(&rnote, NO_DISK);
			break;
	default:	sprintf(buf, GetGizmoText(string_notaBkup), volume);
			SetModalGizmoMessage(&rnote, buf);
			break;
		}
	else
		switch(diagn) {
	/* others? */
	case DTAM_UNKNOWN:	
			BringDownPopup(rnote.shell);
			wait = FALSE;
			if (ptyfp) {
				fputs("\n", ptyfp);
				fflush(ptyfp);
			}
			/* put up restore in progrss poup and start timeout */
			if (action == SHOWFILES) {
				StartRstWatch(GetGizmoText(string_readIndex),
						GetGizmoText(string_readTitle));
				XtAddTimeOut(wait? 2500: 100, 
						(XtTimerCallbackProc)FetchFiles, NULL);
				/*showFilesCB(wid, 0, 0);*/
			}
			else 
			if (action == RESTORE) {
				StartRstWatch(GetGizmoText(string_doingRst), 
						GetGizmoText(title_doingRst));
				XtAddTimeOut(wait? 2500: 100, (XtTimerCallbackProc)WatchRestore, NULL);
				/*restoreCB(wid, 0, 0);*/
			}
			break;

		case DTAM_NO_DISK:
			InsertNotice(&rnote, NO_DISK);
			break;
	default:	sprintf(buf, GetGizmoText(string_notaBkup), volume);
			SetModalGizmoMessage(&rnote, buf);
			break;
		}
}

void	InsertNotice(ModalGizmo *note, int diagn)
{
	char	*button, volstr[64], msgbuf[64];

#ifdef TRACE
	fprintf(stderr,"restore.c: InsertNotice\n");
#endif
	sprintf(volstr, GetGizmoText(string_fmtVolume), volume, vol_count);
	button = note->menu->items->label;
	sprintf(msgbuf, GetGizmoText(string_ins2Msg), volstr, button);
	SetModalGizmoMessage(note, msgbuf);
	MapGizmo(ModalGizmoClass, note);
}

void	selectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	msgbuf[128];

#ifdef TRACE
	fprintf(stderr,"restore.c: selectCB\n");
#endif
	select_count = file_count;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
	SetOrUnsetFiles(TRUE,TRUE);	/* select ALL: set true; update X */
}

void	unselectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	char	msgbuf[128];

#ifdef TRACE
	fprintf(stderr,"restore.c: unselectCB\n");
#endif
	select_count = 0;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
	SetOrUnsetFiles(FALSE,TRUE);	/* unselect ALL: set false; update X */
}

static void
gotoBackupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
#ifdef TRACE
	fprintf(stderr,"restore.c: gotoBackupCB\n");
#endif
	/* unmap the restore window */
	if(rbase.shell)
		XtUnmapWidget(rbase.shell);
	/* map or create backup window */
	if(bbase.shell == NULL)
		SelectAction(DO_BACKUP, -1, client_data, NULL);
	else {
		remap_bkup(bbase.shell);
	}
}

/*
 * Re-map backup window, after "Go to Backup" is selected.
 */
void
remap_rst(Widget wid)
{
	char *desc, *tmpalias;

	XtSetArg(arg[0], XtNlabel, (XtArgVal)&desc);
	XtGetValues(w_rdesc, arg, 1);

	if (!strcmp(desc, rdoc_alias)) {
		curalias = STRDUP(rdoc_alias);
		curdev = NULL;
	}
	else {
		curalias = STRDUP(rst_alias);
		tmpalias = STRDUP(curalias);
		curdev = DtamGetDev(strcat(tmpalias, ":"), FIRST);
		FREE(tmpalias);
	}
	
	XtMapWidget(wid);
}

void	showFilesCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	int	diagn;
	char	*dev;

	Boolean good_result = True;
#ifdef TRACE
	fprintf(stderr,"restore.c: showFilesCB\n");
#endif
	action = SHOWFILES;
	FooterMsg(rbase, NULL);
	if ((good_result = RestorePrep()) !=True) return;
	dev = curdev? DtamDevAttr(curdev, "cdevice"): rst_doc;
#ifdef DEBUG
fprintf(stderr,"dev=%s rst_doc=%s curdev=%s\n",dev,rst_doc,curdev);
#endif
	sprintf(rst_buf, "%s -iBct         -I %s", CpioCmd, dev);
	if (curdev) {
		FREE(dev);
		sprintf(rst_buf+strlen(rst_buf), " -M \"%s\n\" -G %s",
						EOM, "/dev/ptsdummyname");
	}
	if (curdev == NULL) {
		 switch (diagn = CheckTargetFile()) {
			default:	
				return;
			case DTAM_TAR:
			case DTAM_BACKUP:
			case DTAM_CPIO:	rst_format = diagn;
					break;	/* ok */
			/* warn that can't show files for NDS archive */
			case DTAM_NDS:	rst_format = diagn;
					action = 0;
					ShowNotice(GetGizmoText(string_NDSshowfile));
					return;	/* ok */
		}
	}
	else {
		switch (diagn=DtamCheckMedia(curalias)) {
			case DTAM_NO_DISK:
				InsertNotice(&rnote, diagn);
				return;
			/* warn that can't show files for NDS archive */
			case DTAM_NDS:	
				action = 0;
				rst_format = diagn;
				ShowNotice(GetGizmoText(string_NDSshowfile));
				return;	
			default:	
				break;	
		}
	}
	XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
	XtSetValues(w_list, arg, 1);
	if (action != STOP) {
		StartRstWatch(GetGizmoText(string_readIndex),
				GetGizmoText(string_readTitle));
		XtAddTimeOut(wait? 2500: 100, 
				(XtTimerCallbackProc)FetchFiles, NULL);
	}
}

static	void
CountSelFile(char *name)
{
	int	n;

#ifdef TRACE
	fprintf(stderr,"restore.c: CountSelFile\n");
#endif
	for (n = last_restore; n; n--) {
		if (filelist[n].f_set && strcmp((char *)filelist[n].f_name,name)==0) {
#ifdef DEBUG
	fprintf(stderr,"filelist[n]=%s\n", filelist[n].f_name);
#endif
			--restore_count;
			return;
		}
	}
}

static	char *
SkipMsg(char *line)
{
#define	TRUNC	64
	char	*ptr, fname[TRUNC], buf[256];
	int	n;

	ptr = strchr(line,'"')+1;
	*strchr(ptr,'"') = '\0';
	if (last_restore)
		CountSelFile(ptr);
	if ((n = strlen(ptr)) >= TRUNC) {
		strcpy(fname,"...");
		strcat(fname,ptr+4+n-TRUNC);
	}
	else {
		strcpy(fname, ptr);
		while (n < TRUNC-1)
			fname[n++] = ' ';
		fname[n] = '\0';
	}
	sprintf(buf, GetGizmoText(string_skipFile), fname);
	return buf;
}

void	StopRestore(char *msg)
{
	char	bbuf[256];
	int	n, status;

#ifdef TRACE
	fprintf(stderr,"restore.c: StopRestore\n");
#endif
	action = STOP;	
	sprintf(bbuf, GetGizmoText(string_restOK));
	if (msg) {
		sprintf(bbuf, GetGizmoText(string_restFail));
		ErrorNotice(bbuf, GetGizmoText(string_restoreError));
	}
	else {
		sprintf(bbuf, GetGizmoText(string_restOK));
		XtSetArg(arg[0], XtNstring, bbuf);
		XtSetValues(w_rstmsg, arg, 1);
		FooterMsg(rbase, bbuf);
	}
	if (nbfgets(buf, BUFSIZ, cmdfp[1]))	/* discard pending msgs */
		;
/*	XtSetArg(arg[0],  XtNmappedWhenManaged, FALSE);
/*	XtSetValues(w_gauge, arg, 1);
*/	
	status = _Dtam_p3close(cmdfp, msg ? 0: SIGTERM);
	status = WEXITSTATUS(status);
printf("status from NDS restore = <%d>\n", status);
	cmdfp[1] = cmdfp[0] = (FILE *)NULL;
	if (ptyfp) {
		fclose(ptyfp);
		ptyfp = (FILE *)NULL;
	}

	/* Check for errors from NDS restore */
	if (rst_format == DTAM_NDS && status != 0)
		ShowNDSError(status);

	XtAddTimeOut(1000, (XtTimerCallbackProc)DropRstWatch,NULL);
	TID = 0;
}

void	WatchRestore(XtPointer closure, XtIntervalId intid)
{
static	int	count = 0;
	int	timer;
	int	fdmaster;
	char	buf[BUFSIZ], msg[BUFSIZ];

#ifdef TRACE
	fprintf(stderr,"restore.c: WatchRestore action=%d\n", action);
#endif
	if (action == STOP) return;
	if (wait)
		timer = 2000;
	else if (init_cpio) {
		init_cpio = FALSE;
		count = 0;
		timer = 2000;
		if (rst_format == TAR) {
			char	*ptr;
			ptr = strstr(rst_buf, "Bdlcv        ");
			if (ptr) 
				strncpy(ptr, "dv -H tar ", 10);
			else {
				ptr = strstr(rst_buf, "Bdlcvu       ");
				if (ptr) strncpy(ptr, "dvu -H tar ", 11);
				else { /* shouldn't happen, but ... */
					fprintf (stderr, "MediaMgr, WatchRestore: invalid cpio command\n");
					return;	/* is this good enough? */
				}
			}
		}
		if (curdev == NULL)
			ptyfp = (FILE *)NULL;
		else {
			fdmaster = open("/dev/ptmx", O_RDWR);
			grantpt(fdmaster);
			unlockpt(fdmaster);
			ptyname = ptsname(fdmaster);
			fcntl(fdmaster, F_SETFL, O_NONBLOCK);
			ptyfp = fdopen(fdmaster, "r+");
			setbuf(ptyfp, NULL);
			strcpy(strstr(rst_buf, "-G ")+3, ptyname);
#ifdef DEBUG
	fprintf(stderr,"rst_buf=%s\n",rst_buf);
#endif
		}
		strcat(rst_buf, " 2>&1");
		_Dtam_p3open(rst_buf, cmdfp, TRUE);
	}
	else {
		timer = 100;
		buf[0] = '\0';
		if (ptyfp && nbfgets(buf, BUFSIZ, ptyfp)) {
			if (strncmp(buf, EOM, 4)==0) {
				wait = TRUE;
				vol_count = atoi(buf+4);;
				InsertNotice(&rnote, NO_DISK);
				timer = 5000;
				rewind(ptyfp);
			}
		}
		buf[0] = '\0';
		if (nbfgets(buf, BUFSIZ, cmdfp[1])) {
			int	n;
			if (*buf == '\0')
				;
			else if (strstr(buf, END_tag)) {
				StopRestore(NULL);
				return;
			}
			else if (strncmp(buf,HALT_tag,n=strlen(HALT_tag))==0) {
				StopRestore(buf+n);
				return;
			}
			else if (strncmp(buf,ERR_tag,n=strlen(ERR_tag))==0) {
				sprintf(msg, GetGizmoText(string_restFail));
				ErrorNotice(msg, 
                                      GetGizmoText(string_restoreError));
			}
			else if (strncmp(buf,SKP1_tag,n=strlen(SKP1_tag))==0
			     &&  strstr(buf+n, SKP2_tag)!=0) {
				XtSetArg(arg[0],XtNstring,SkipMsg(buf));
				XtSetValues(w_rstmsg, arg, 1);
			}
			else {	/* one file was read in; process it */
				buf[strlen(buf)-1] = '\0';
				FooterMsg(rbase, buf);
				if (count++ == 0 && rst_format == BACKUP) {
					if (bkup_index)
					{
						unlink(bkup_index);
						FREE(bkup_index);
						bkup_index = NULL;
					}
				}
				if (last_restore)
					CountSelFile(buf);
			}
			if (! restore_privs_flag &&
				last_restore && restore_count == 0) {
				StopRestore(NULL);
				return;
			}
		}
		else {	/* NDS archive restore complete */
			if (feof(cmdfp[1]))
				StopRestore(NULL);
		}
	}
	TID = XtAddTimeOut(timer, (XtTimerCallbackProc)WatchRestore, NULL);
}

PartialRestore()
{
	Boolean	anyset = FALSE, anyunset = FALSE;
#ifdef TRACE
	fprintf(stderr,"restore.c: PartialRestore\n");
#endif

	if (filelist) {
		register int n;
		for (n= 0; n < file_count; n++) {
			if (filelist[n].f_set)
				anyset = TRUE;
			else
				anyunset = TRUE;
			if (anyset && anyunset)
				return SOME_SET;
		}
	} 
	if (anyunset && !anyset)
		return NONE_SET;
	else
		return ALL_SET;
}

Boolean	RestorePrep()
{
	int	n;
	char	buf[256];

#ifdef TRACE
	fprintf(stderr,"restore.c: RestorePrep\n");
#endif
	unlink(pidindex);
	if (bkup_index)
	{
	        FREE(bkup_index);
	        bkup_index = NULL;
	}

	init_cpio = TRUE;
	if (curdev) {
		vol_count = 1;
		volume = DtamDevAttrTxt(curdev, "volume");
		wait = TRUE;
		switch (n = DtamCheckMedia(curalias)) {
		case DTAM_BACKUP:	
		case DTAM_TAR:
		case DTAM_CPIO:	
		case DTAM_NDS:	
				wait = FALSE;	/* ok */
				rst_format = n;
				break;
		case DTAM_NO_DISK:
				InsertNotice(&rnote, n);
				return False;
				break;
		case DTAM_UNREADABLE:
				InsertNotice(&rnote, n);
				sprintf(buf, GetGizmoText(string_unreadFile),
								volume);
				SetModalGizmoMessage(&rnote, buf);
				return False;
		default:	InsertNotice(&rnote, n);
				sprintf(buf, GetGizmoText(string_notaBkup),
								volume);
				SetModalGizmoMessage(&rnote, buf);
				return False;
		}
	}
	else  {
		switch (CheckTargetFile()) {
		case DTAM_BACKUP:	
		case DTAM_TAR:
		case DTAM_CPIO:	
		case DTAM_NDS:	
				wait = FALSE;	/* ok */
				break;
		case BAD_DEVICE:
		case UNREADABLE:
				return False;
		default:	
				return True;
		}
	}
return True;
}

void	restoreCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	FILE	*fp;
	char	*dev, *ptr;
	Boolean	good_result;
	Boolean	chk_flag;
	int	n, rst_flag;
#ifdef TRACE
	fprintf(stderr,"restore.c: restoreCB\n");
#endif

	action = RESTORE;
	FooterMsg(rbase, NULL);
	if ((good_result = RestorePrep()) != True) return;
	if (PartialRestore() == NONE_SET) {
		FooterMsg(rbase, GetGizmoText(string_noneset));
		return;
	}
	restore_count = last_restore = 0;

	if (privileged_user) {
		XtSetArg(arg[0], XtNset, &restore_privs_flag);
		OlFlatGetValues(w_opt2, 0, arg, 1);
	}

	switch (rst_flag = PartialRestore()) {
	case NONE_SET:
		FooterMsg(rbase, GetGizmoText(string_noneset));
		return;
	case SOME_SET:
		fp = fopen(pidindex, "w");
		for (n = 0; n < file_count; n++)
			if (filelist[n].f_set) {
				last_restore = n;
				++restore_count;
				fprintf(fp, "%s\n", filelist[n].f_name);
			}
		fclose(fp);
	}
	XtSetArg(arg[0], XtNset, &chk_flag);
	OlFlatGetValues(w_opt, 0, arg, 1);
	dev = curdev? DtamDevAttr(curdev, "cdevice"): rst_doc;
	/* Check if restoring NDS archive */
	if (rst_format == DTAM_NDS) {
		if (NWSInstalled() != True) {
			ShowNotice(GetGizmoText(string_NDSInstall));
			return;
		}
		sprintf(rst_buf, "%srestore -v -i %s", NDSCmd, dev);
	}
	else {
		sprintf(rst_buf, "%s -iBdlcvm%c %s -I %s", CpioCmd,
			(chk_flag? 'u': ' '), 
			(privileged_user && restore_privs_flag == B_FALSE)?
			"-e all=warn,priv=ignore": "", dev);
	}
	if (rst_flag == SOME_SET)
		strcat(strcat(rst_buf, " -E "), pidindex);
	if (curdev) {
		FREE(dev);
		sprintf(rst_buf+strlen(rst_buf), " -M \"%s\n\" -G %s",
						EOM, "/dev/ptsdummyname");
	}
printf("About to restore with: %s\n", rst_buf);
	if (action != STOP) {
		StartRstWatch(GetGizmoText(string_doingRst), 
			GetGizmoText(title_doingRst));
		XtAddTimeOut(wait? 2500: 100, (XtTimerCallbackProc)WatchRestore, NULL);
	}
}

void	RdescCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*desc, *name;
	register  int	n = olcd->item_index;
	char	*tmpalias;

#ifdef TRACE
	fprintf(stderr,"restore.c: RdescCB \n");
#endif
	FooterMsg(rbase, NULL);
	name = RstDevice[olcd->item_index].label;
	if (curdev)
		FREE(curdev);
	if (n == 0) {		/* restore from directory */
		desc = name;
		curalias = STRDUP(name);
		curdev = NULL;
	}
	else {
		curalias = DtamMapAlias(name);
		tmpalias = STRDUP(curalias);
		curdev = DtamGetDev(strcat(tmpalias, ":"),FIRST);
		FREE(tmpalias);
		desc = DtamDevDesc(curdev);
	}

	rst_alias = STRDUP(curalias);

	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid, arg, 1);

	XtSetArg(arg[0], XtNlabel, desc);
	XtSetValues(w_ud, arg, 1);

	XtSetArg(arg[0], XtNmappedWhenManaged, n==0);
	XtSetValues(XtParent(w_file), arg, 1);
	if (n == 0)
		OlSetInputFocus(w_file, RevertToNone, CurrentTime);
	RstSummary();
}


void	prevGroupCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
ListPtr	show_list;

#ifdef TRACE
	fprintf(stderr,"restore.c: prevGroupCB\n");
#endif
	group_show--;
	show_num = group_size;

	show_list = filelist + group_show * group_size;

	XtSetArg(arg[0],XtNitems,show_list);
	XtSetArg(arg[1],XtNnumItems,show_num);
	XtSetArg(arg[2],XtNviewHeight, 7);
	XtSetArg(arg[3],XtNviewItemIndex, show_num - 1);
	XtSetArg(arg[4],XtNitemsTouched, TRUE);
	XtSetValues(w_list, arg, 5);

	/* buttons visible; next sensitive */
	if (group_show > 0)
		change_group_menu_items[0].sensitive = (XtArgVal)TRUE;
	else
		change_group_menu_items[0].sensitive = (XtArgVal)FALSE;
	change_group_menu_items[1].sensitive = (XtArgVal)TRUE;
	XtSetArg(arg[0], XtNitems, change_group_menu_items);
	XtSetArg(arg[1], XtNitemsTouched, TRUE);
	XtSetValues(w_group, arg, 2);
}


void	nextGroupCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
ListPtr	show_list;

#ifdef TRACE
	fprintf(stderr,"restore.c: nextGroupCB\n");
#endif
	group_show++;
	if (group_show == group_max)
	{
		show_num = file_count % group_size;
		if (show_num == 0) show_num = group_size;
	}
	else
		show_num = group_size;

	show_list = filelist + group_show * group_size;

	XtSetArg(arg[0],XtNitems,show_list);
	XtSetArg(arg[1],XtNnumItems,show_num);
	XtSetArg(arg[2],XtNviewHeight, 7);
	XtSetArg(arg[3],XtNviewItemIndex, show_num - 1);
	XtSetArg(arg[4],XtNitemsTouched, TRUE);
	XtSetValues(w_list, arg, 5);

	/* buttons visible; next sensitive */
	if (group_show < group_max)
		change_group_menu_items[1].sensitive = (XtArgVal)TRUE;
	else
		change_group_menu_items[1].sensitive = (XtArgVal)FALSE;
	change_group_menu_items[0].sensitive = (XtArgVal)TRUE;
	XtSetArg(arg[0], XtNitems, change_group_menu_items);
	XtSetArg(arg[1], XtNitemsTouched, TRUE);
	XtSetValues(w_group, arg, 2);
}


static	void
SetRstDoc(Widget wid, XtPointer client_data, XtPointer call_data)
{
	struct	stat		fbuf;
	OlTextFieldVerify	*verify = (OlTextFieldVerify *)call_data;
	char			msgbuf[BUFSIZ];

	if (stat(verify->string,&fbuf) == 0) {
		sprintf(msgbuf, GetGizmoText(string_newFile), rst_doc);
		FooterMsg(rbase, msgbuf);
	}
	else {
		rst_doc = STRDUP(verify->string);
		RstSummary();
	}
}

CreateRestoreWindow(parent, atomName)
	Widget	parent;
	char	*atomName;
{
	static	ExclItem	RCheckBox[2];
	static	ExclItem	R2CheckBox[2];
	Widget		w_ctl, w_cap, w_devmenu, w_filemenu, r_abvbtn;
	char		*str, buf[64];
	char		tfadminstring[256];
	int		rval, i;
	char		*tmpalias;

#ifdef TRACE
	fprintf(stderr,"restore.c: CreateRestoreWindow\n");
#endif
	if (note.shell)
		XtDestroyWidget(note.shell);
	NotePidFiles();
	SetLocaleTags();
/*
 *	create base window
 */
	rbase.icon_name = GetGizmoText(rbase.icon_name);
	w_ctl = CreateMediaWindow(parent, &rbase, NULL, 0, NULL);
	w_filemenu = GetMenu(GetSubMenuGizmo(rbase.menu, 0));
	XtSetArg(arg[0], XtNclientData, atomName);
	OlFlatSetValues(w_filemenu, 0, arg, 1);
/*
 *	create doc/device abbreviated button menu
 */
	RstDevice[0].label = rdoc_alias = GetGizmoText(label_doc);
	w_rdesc = DevMenu(RstDevice, 1, N_DEVS, w_ctl,
			GetGizmoText(label_rstFromCaption),
			(XtPointer)RdescCB, "removable=\"true", 
			&w_devmenu,  &r_abvbtn, 0);
	if (rst_alias) {
		tmpalias = STRDUP(rst_alias);
                curdev = DtamGetDev(strcat(tmpalias, ":"), FIRST);
                FREE(tmpalias);
		curalias = STRDUP(rst_alias);
	}
	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_rdesc, arg, 1);

/*
 *	controls specific to Restore
 */

	XtSetArg(arg[0], XtNlabel,		GGT(label_targetCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);
	XtSetArg(arg[3], XtNmappedWhenManaged,	FALSE);
	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 4);

	XtSetArg(arg[0], XtNcharsVisible, 40);
	w_file = XtCreateManagedWidget("textfield",
			textFieldWidgetClass, w_cap, arg, 1);
	XtAddCallback(w_file, XtNverification, SetRstDoc, NULL);

	RCheckBox[0].setting = (XtArgVal)FALSE;
	RCheckBox[0].label = GetGizmoText(string_overwrite);
	RCheckBox[0].sensitive = (XtArgVal)TRUE;

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNitemFields,		ExclFields);
	XtSetArg(arg[2], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[3], XtNitems,		RCheckBox);
	XtSetArg(arg[4], XtNnumItems,		1);
	XtSetArg(arg[5], XtNbuttonType,		OL_CHECKBOX);
	w_opt = XtCreateManagedWidget("button",
			flatButtonsWidgetClass, w_ctl, arg, 6);

        R2CheckBox[0].setting = (XtArgVal)TRUE;
        R2CheckBox[0].label = GetGizmoText(string_privRstButton);
	R2CheckBox[0].sensitive = (XtArgVal)TRUE;

        XtSetArg(arg[0], XtNtraversalOn,        TRUE);
        XtSetArg(arg[1], XtNitemFields,         ExclFields);
        XtSetArg(arg[2], XtNnumItemFields,      NUM_ExclFields);
        XtSetArg(arg[3], XtNitems,              R2CheckBox);
        XtSetArg(arg[4], XtNnumItems,           1);
        XtSetArg(arg[5], XtNbuttonType,         OL_CHECKBOX);
        w_opt2 = XtCreateWidget("button2",
                        flatButtonsWidgetClass, w_ctl, arg, 6);
	/* Check if the user should be allowed to restore file 
	 * privileges.  If so, manage the w_opt2 widget so that the
	 * menu button for restoring privilege is displayed.
	 */
	if (privileged_user = _DtamIsOwner(OWN_BACKUP)) {
		XtManageChild(w_opt2);
	}
/*
 *	Restore window: scrolling list of files in "Select Files" mode
 */
	CreateFileList(rbase.scroller);
	
	XtSetArg(arg[0], XtNlabel,		GGT(label_groupCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);
	XtSetArg(arg[3], XtNmappedWhenManaged,	FALSE);
	w_groupcaption = XtCreateManagedWidget("groupcaption",
			captionWidgetClass, w_ctl, arg, 4);

	change_group_menu_items[0].label = GGT(label_prev_group);
	change_group_menu_items[1].label = GGT(label_next_group);

	XtSetArg(arg[0], XtNitems, 		change_group_menu_items);
	XtSetArg(arg[1], XtNnumItems, 		XtNumber(change_group_menu_items));
	XtSetArg(arg[2], XtNitemFields, 	flatMenuFields);
	XtSetArg(arg[3], XtNnumItemFields, 	XtNumber(flatMenuFields));
	XtSetArg(arg[4], XtNlayoutType, 	OL_FIXEDROWS);
	XtSetArg(arg[5], XtNexclusives, 	TRUE);
	XtSetArg(arg[6], XtNnoneSet, 		TRUE);
	XtSetArg(arg[7], XtNbuttonType, 	CMD);
	XtSetArg(arg[8], XtNmappedWhenManaged,	FALSE);
	w_group = XtCreateManagedWidget("groupbuttons",
			flatButtonsWidgetClass, w_groupcaption, arg, 9);
	
	RstSummary();
	CreateGizmo(rbase.shell, ModalGizmoClass, &rnote, NULL, 0);
	MapGizmo(BaseWindowGizmoClass, &rbase);
        RstRegisterHelp();
	group_size = INIT_GROUP_SIZE;
	OlSetInputFocus(r_abvbtn, RevertToNone, CurrentTime);
}

CreateFileList(parent)
	Widget	parent;
{
#ifdef TRACE
	fprintf(stderr,"restore.c: CreateFileList\n");
#endif
	XtSetArg(arg[0], XtNwidth,	32*x3mm);
	XtSetArg(arg[1], XtNheight,	12*y3mm);
	XtSetArg(arg[2], XtNminWidth,	1);
	XtSetArg(arg[3], XtNminHeight,	1);
	XtSetValues(parent, arg, 4);

	XtSetArg(arg[0], XtNmappedWhenManaged,	FALSE);
	XtSetArg(arg[1], XtNviewHeight,		7);
	XtSetArg(arg[2], XtNformat,		"%24s");
	XtSetArg(arg[3], XtNexclusives,		FALSE);
	XtSetArg(arg[4], XtNitemFields,		ListFields);
	XtSetArg(arg[5], XtNnumItemFields,	XtNumber(ListFields));
	XtSetArg(arg[6], XtNitems,		filelist);
	XtSetArg(arg[7], XtNnumItems,		file_count);
	XtSetArg(arg[8], XtNselectProc,		(XtCallbackProc)fileSelectCB);
	XtSetArg(arg[9], XtNunselectProc,	(XtCallbackProc)fileUnselectCB);

	w_list = XtCreateManagedWidget("fileList",
			flatListWidgetClass, parent, arg, 10);

	/* check for maximum number of items allowed in list */
	XtAddCallback(w_list,XtNitemsLimitExceeded,(XtCallbackProc)tooManyCB,NULL);

}

#define NO_ODD_LOT 100

void	tooManyCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{ /* too many items in list widget; check for (and set new) maximum */
OlFListItemsLimitExceededCD *tooMany = call_data;

#ifdef TRACE
	fprintf(stderr,"restore.c: tooManyCB\n");
#endif
	group_size = tooMany -> preferred;
	if (group_size > NO_ODD_LOT) 
		group_size -= group_size%NO_ODD_LOT;
	tooMany -> preferred = group_size;
	tooMany -> ok = TRUE;
}

void	fileSelectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	OlFlatCallData	*call_data;
{ /* user selected a file; change count and display */
	int itemIndex;
	DIR *dir;
	char	msgbuf[128];

#ifdef TRACE
	fprintf(stderr,"restore.c: fileSelectCB\n");
#endif
	/* get selected */
	itemIndex = call_data->item_index;
#ifdef DEBUG
	fprintf(stderr,"fileSelectCB file=%s\n", filelist[itemIndex].f_name);
#endif
	/* check if a directory is selected, and if so select all files
		in directory as well */
		if ((SetFilesInDir((char *) filelist[itemIndex].f_name)) == True)  {
				XtVaSetValues(w_list, XtNviewItemIndex,
					itemIndex, 0);
		}
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
		

}

void	fileUnselectCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{ /* user deselected a file; change count and display */

	char	msgbuf[128];

	select_count--;
	sprintf(msgbuf,GetGizmoText(string_selected),select_count,file_count);
	FooterMsg(rbase, msgbuf);
}

static Boolean
SetFilesInDir(dirname)
char *dirname;
{
	int i;
	Boolean result = False;
	unsigned char	*s1, *s2;

#ifdef TRACE
	fprintf(stderr,"restore.c: SetFilesInDir\n");
#endif
	if (!dirname) return;
	for (i=0; i < file_count; i++)  {
		/* part of same directory */
#ifdef DEBUG
	fprintf(stderr,"filelist[%d].f_name=%s dirname=%s\n",i,filelist[i].f_name,dirname);
#endif
		s1	= (unsigned char *) dirname;
		s2	= (unsigned char *) filelist[i].f_name;
		while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2)
		{
      			++s1;
      			++s2;
   		}

		/* must be itself or s1 is a dir & s1 is below */
		if ( (*s1 == *s2) || (*s1 == '\0' && *s2 == '/') )
		{
#ifdef DEBUG
			fprintf(stderr,"setting %s to set\n",filelist[i].f_name);
#endif

			XtVaSetValues(w_list, 
					XtNset, 
					i, 
					XtNitemsTouched,
					True,
					XtNviewItemIndex,
					i,
					0);
			filelist[i].f_set = TRUE;
			select_count++;
			result = True;
		}
	}


return result;
}
