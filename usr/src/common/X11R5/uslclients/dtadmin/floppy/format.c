#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/format.c	1.49.1.2"
#endif

#include "media.h"
#include <stdio.h>
#include <string.h>

extern void	CreateFormatWindow OL_ARGS((Widget));
extern void     ErrorNotice();
extern void     OKCB();

extern	void	FmtInsertMessage();
static char *GetLogicalDevicePointer OL_ARGS((char *, int));
static char *GetLogicalDeviceLabel OL_ARGS((char *));

extern	long	_dtam_flags;
extern	int	backup_flag;

static Widget w_dns;
static Boolean fconfirm_up = False;

void	fkillCB();
void	formatCB();
void	cancelFmtCB();
void	fhelpCB();
void	propCB();
void	applyCB();
void	resetCB();
void	cancelCB();
void	doformat();


static MenuItems ffile_menu_item[] = {
	{ TRUE, label_format,mnemonic_format, 0, formatCB, "format"},
	{ TRUE, label_prop,mnemonic_prop, 0, propCB},
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static HelpInfo FHelpFormat	= { 0, "", DHELP_PATH, help_format };
static HelpInfo FHelpProperties	= { 0, "", DHELP_PATH, help_fproperties };
static HelpInfo FHelpTOC	= { 0, "", DHELP_PATH, NULL        };
static HelpInfo FHelpDesk	= { 0, "", DHELP_PATH, "HelpDesk"  };

static MenuItems fhelp_menu_item[] = {  
	{ TRUE, label_fmtHlp,mnemonic_fmtHlp,0, fhelpCB, (char *)&FHelpFormat },
	{ TRUE, label_toc,   mnemonic_toc,   0, fhelpCB, (char *)&FHelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk,0, fhelpCB, (char *)&FHelpDesk },
	{ NULL }
};

static MenuGizmo ffile_menu = {0, "file_menu", NULL, ffile_menu_item};
static MenuGizmo fhelp_menu = {0, "help_menu", NULL, fhelp_menu_item};

static MenuItems fmain_menu_item[] = {
	{ TRUE, label_file, mnemonic_file, (Gizmo) &ffile_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &fhelp_menu},
	{ NULL }
};
static MenuGizmo fmenu_bar = { 0, "menu_bar", NULL, fmain_menu_item};

BaseWindowGizmo fbase = {0, "base", label_format, (Gizmo)&fmenu_bar,
	NULL, 0, label_format, D1ICON_NAME, " ", " ", 90 };

static MenuItems prop_menu_item[] = {
	{ TRUE, label_ok, mnemonic_ok, 0, applyCB, NULL },
	{ TRUE, label_reset, mnemonic_reset, 0, resetCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB, NULL },
	{ TRUE, label_help, mnemonic_help, 0, fhelpCB, (XtPointer)&FHelpProperties },
	{ NULL }
};

static MenuGizmo prop_menu = {0, "properties", NULL, prop_menu_item};
static PopupGizmo prop_popup = {0, "popup", string_propLine, (Gizmo)&prop_menu};

static MenuItems fwatch_menu_item[] = {  
	{ TRUE, label_cancel, mnemonic_cancel, 0, fkillCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, fhelpCB, (char *)&FHelpTOC },
	{ NULL }
};
static MenuGizmo fwatch_menu = {0, "fwatch_menu", NULL, fwatch_menu_item};
static PopupGizmo fwatch = {0, "popup", title_doingFmt, (Gizmo)&fwatch_menu};

static MenuItems fconfirm_menu_item[] = {  
	{ TRUE, label_format, mnemonic_format, 0, formatCB, "confirm" },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelFmtCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, fhelpCB, (char *)&FHelpFormat },
	{ NULL }
};
static MenuGizmo fconfirm_menu = {0, "fconfirm_menu", NULL, fconfirm_menu_item};
static ModalGizmo fconfirm = {0, "", title_confirmFmt, (Gizmo)&fconfirm_menu};

#define	HIGH_DNS	0
#define	LOW_DNS		1


#define	BKUP_FMT	0	/* i.e., no file system */
#define	S5_FMT		1
#define	DOS_FMT		2

#define FMT_OP		0
#define MKFS_OP		1

int	fmt_type = BKUP_FMT;

FILE		*cmdfp[2];


/* An ExclItem is:
 *
	typedef struct  {	char  *       label;
				XtArgVal      mnem;
				Boolean       setting;
	} ExclItem;

    Lets change it to a DensExclItem - 
	typedef struct  {	char  *       label;
				XtArgVal      mnem;
				char	*logical_device;
				Boolean       setting;
	} ExclItem;
 *
 */
typedef struct  {
	char		*label;
	XtArgVal	mnem;
	char		*logical_device;
	Boolean		setting;
} DensExclItem;

/* We will have to redefice ExclFields to DensExclFields, and add in the
 * extra field - an XtNuserData field.
 */
char	*DensExclFields[] = { XtNlabel, XtNmnemonic, XtNuserData, XtNset };

#define MAX_DENSITIES	10

typedef struct {
	char		*label;
	XtArgVal	mnem;
	char		*cmd;
	Boolean		setting;
} FsExclItem;

char	*FsExclFields[] = { XtNlabel, XtNmnemonic, XtNuserData, XtNset };

static char cur_num_density_items;
static char *FS_TABLE = "desktop/MediaMgr/FsTable";

Dimension	xinch, yinch;

/* UIS - use the new DensExclItem */
DensExclItem	DnsItems[MAX_DENSITIES];

ExclItem	FmtItems[3];

DevItem		DeviceItem[N_DEVS];

XtIntervalId	gauge_id = 0;		/* also used by backup ??? */
int		g_value = 0, filesystem = 0, prev_fs, fs_num = 0, cur_fs=0;
Widget		w_baseshell, w_gauge, w_txt, w_prop, w_popup, w_checks;

Boolean		fmt_done = FALSE;
Widget		w_fdesc, w_dcap;
int		op; 
String		cur_fstype, cur_fscmd;

void	propCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	SetPopupMessage(&prop_popup, NULL);
	FooterMsg(fbase, NULL);
	XtPopup(w_popup, XtGrabNone);
	OlSetInputFocus(w_checks, RevertToNone, CurrentTime);
}


void	applyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Boolean set;
	int	i;
	
	for (i=0; i<fs_num; i++) {
		OlVaFlatGetValues(w_checks, i, XtNset, &set, NULL);
		if (set == TRUE) {
			XtSetArg(arg[0], XtNlabel, (XtArgVal)&cur_fstype);
			XtSetArg(arg[1], XtNuserData, (XtArgVal)&cur_fscmd);
			OlFlatGetValues(w_checks, i, arg, 2);
			cur_fs = i;
			break;
		}
	}
	BringDownPopup(w_popup);
}

void	resetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	i;

	SetPopupMessage(&prop_popup, "");
	OlVaFlatSetValues(w_checks, cur_fs, XtNset, (XtArgVal)TRUE, NULL);	
	
}

void	cancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	SetPopupMessage(&prop_popup, NULL);
	FooterMsg(fbase, NULL);
	SetWMPushpinState(XtDisplay(w_popup), XtWindow(w_popup), 
		WMPushpinIsOut);
	BringDownPopup(w_popup);
}


void	fhelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;

	FooterMsg(fbase, NULL);
	help->app_title	= 
	help->title	= GetGizmoText(label_format);
	help->section = GetGizmoText(STRDUP(help->section));
	PostGizmoHelp(fbase.shell, help);
}

void	cancelFmtCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	FooterMsg(fbase, NULL);
	BringDownPopup(fconfirm.shell);
	fconfirm_up = False;
	if (cmdfp[1]) {
		_Dtam_p3close(cmdfp, 0);
		cmdfp[0] = cmdfp[1] = (FILE *)NULL;
	}
}

void	DropWatch(XtPointer closure, XtIntervalId id)
{
	Cursor	cursor;

	cursor = GetOlStandardCursor(theScreen);
	XDefineCursor(theDisplay, XtWindow(fwatch.shell), cursor);
	XDefineCursor(theDisplay, XtWindow(w_gauge), cursor);
	XDefineCursor(theDisplay, XtWindow(fbase.shell), cursor);
	BringDownPopup(fwatch.shell);
}

void	fkillCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	buf[BUFSIZ];

	FooterMsg(fbase, NULL);
	_Dtam_p3close(cmdfp, SIGTERM);
	sprintf(buf, GetGizmoText(string_fmtKilled));
	XtSetArg(arg[0], XtNstring, (XtArgVal)buf);
	XtSetValues(w_txt, arg, 1);
	if (gauge_id) {
		XtRemoveTimeOut(gauge_id);
		gauge_id = 0;
		XtSetArg(arg[0], XtNsliderValue,	0);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetValues(w_gauge, arg, 2);
	}
	FooterMsg(fbase, buf);
	XtAddTimeOut(1500, (XtTimerCallbackProc)DropWatch, (XtPointer)NULL);
}

SetGauge(int value)
{
	if (value < 0)
		value = 0;
	else if (value > 100)
		value = 100;
	g_value = value;
	XtSetArg(arg[0], XtNsliderValue, (XtArgVal)g_value);
	XtSetValues(w_gauge, arg, 1);
}

void
UpdateGauge(int delta, XtIntervalId id)	/* follows a gauge in 1% increments */
{
	if (cmdfp[1] == NULL || g_value >= 100) {
		g_value = 0;
		gauge_id = NULL;
		XtSetArg(arg[0], XtNsliderValue, 	g_value);
		XtSetArg(arg[1], XtNmappedWhenManaged,	FALSE);
		XtSetValues(w_gauge, arg, 2);
	}
	else {
		++g_value;
		gauge_id = XtAddTimeOut(delta, (XtTimerCallbackProc)UpdateGauge,
							(XtPointer)delta);
		XtSetArg(arg[0], XtNsliderValue, (XtArgVal)g_value);
		XtSetValues(w_gauge, arg, 1);
	}
}


char	*FmtDiagMsg(int diagnostic)
{
extern	char	*FmtInsertMsg();
static	char	buf[256];
	char	*str;
	char	*drive = DtamDevAlias(curdev);

	if (_dtam_flags & DTAM_READ_ONLY) {
		sprintf(buf, GetGizmoText(string_cantWrite), drive);
		str = buf;
	}
	else switch (diagnostic) {

	case NO_DISK:		str = FmtInsertMsg(GetGizmoText(label_format));
				break;
	case UNDIAGNOSED:
	case UNREADABLE:	if ((str=DtamDevAttrTxt(curdev,"volume")) == NULL)
					str = GetGizmoText(string_genMedia);
				if(strncmp(curdev,DISKETTE,strlen(DISKETTE))!=0)
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, "");
				else
					sprintf(buf, 
						GetGizmoText(string_unreadDisk),
						str, drive+strlen(drive)-1);
				str = buf;
				break;
	case UNFORMATTED:	str = " ";
				break;
	case DOS_DISK:		str = GetGizmoText(string_fmtDOStoUNIX);
				break;
	case UNKNOWN:		sprintf(buf, GetGizmoText(string_isFormatted),
						drive+strlen(drive)-1);
				str = buf;
				break;
	default:		str = GetGizmoText(string_hasData);
	}
	FREE(drive);
	return str;
}

/*
 *	look for error messages, or confirmatory output; show gauge if limited
 */
void	CheckStderr(int limit, XtIntervalId id)
{
static	Boolean	in_mkfs = FALSE;
static	Boolean	in_fmt = FALSE;
static	char	*msg = NULL;
	char	buf[BUFSIZ], *str;
	int	n, status;

	if (cmdfp[1] == NULL)
		return;			/* terminated by cancel; end timeouts */
	n = read(CMDOUT, buf, BUFSIZ);
	switch (n) {
	case 0:	/*
		 *	end of file; command has finished
		 */
		fmt_done = TRUE;
		in_fmt = in_mkfs = FALSE;
		status = _Dtam_p3close(cmdfp, 0);

                /*
                 *  More cases can be added to this status
                 *  switch if the status codes become more unique
                 *  in the future.
                 */
                if (WIFEXITED(status))
                      status = WEXITSTATUS(status);

                switch (status) {
                 
                case 0:
                        if (op == FMT_OP)
                                sprintf(buf, GetGizmoText(string_fmtOK));
                        else
                                sprintf(buf, GetGizmoText(string_mkfsOK));
                        break;
                default:
                        if (op == FMT_OP)
                                sprintf(buf, GetGizmoText(string_formatFailed));
                        else
                                sprintf(buf, GetGizmoText(string_mkfsFailed));
                        if (msg) {
                                if (op == FMT_OP)
                                   fprintf(stderr, GGT(string_fmtStderr), msg);
                                else
                                   fprintf(stderr, GGT(string_mkfsStderr), msg);
                        }
                        break;
                }

                if ( status == 0 ){
		    XtSetArg(arg[0], XtNstring, (XtArgVal)buf);
		    XtSetValues(w_txt, arg, 1);
		    XtAddTimeOut(1500, (XtTimerCallbackProc)DropWatch,
							(XtPointer)NULL);
		    FooterMsg(fbase, buf);
                }
                else{
		    XtAddTimeOut(1500, (XtTimerCallbackProc)DropWatch,
							(XtPointer)NULL);
                    ErrorNotice(buf, GetGizmoText(string_formatError));
                }
		if (msg) {
			FREE(msg);
			msg = NULL;
		}
		/*
		 * An unformatted diskette was detected during backup operation.
		 */
		if (backup_flag)
			exit(0);
		break;

	default:/*
		 *	save message for later examination
		 */
		if (msg == NULL) {
			msg = (char *)MALLOC(n+2);
			*msg = '\0';
		}
		else
			msg = (char *)REALLOC(msg, strlen(msg)+n+2);
		if (msg == NULL) {
			XtSetArg(arg[0], XtNstring,
					GetGizmoText(string_badMalloc));
			XtSetValues(w_txt, arg, 1); 
			return;
		}
		buf[n] = '\0';
		if (strstr(buf,"ormatting")) {
			in_fmt = TRUE;
			in_mkfs = FALSE;
			if (limit) {
				SetGauge(0);
				XtSetArg(arg[0], XtNmappedWhenManaged, TRUE);
				XtSetValues(w_gauge, arg, 1);
				gauge_id = XtAddTimeOut(100,
					(XtTimerCallbackProc)UpdateGauge,
					(XtPointer)(limit/100));
			}
		}
		strcat(msg,buf);
		if (strstr(msg,"make s5 file system?") && !in_mkfs) {
			in_mkfs = TRUE;
			op = MKFS_OP;
			XtSetArg(arg[0], XtNstring, 
					GetGizmoText(string_doingMkfs));
			XtSetValues(w_txt,  arg, 1);
		}
		/* fall through to wait for more input */
	case -1:
		XtAddTimeOut(1250, (XtTimerCallbackProc)CheckStderr,
							(XtPointer)limit);
		break;
	}
}

/*
 * density represents the index into the density exclusives list - 0 is
 * the first one.
 */
int	density = 0;

void
SetDensity(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	FooterMsg(fbase, NULL);
	density = olcd->item_index;
}

void
SetFormatType(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;

	FooterMsg(fbase, NULL);
	fmt_type = olcd->item_index;
}

ParseMdenslist(char *str)
{
	char *ptr;
	char name[256];
	int i,len;
	int devicecount = 0;
	char *newalias;
	char *deviceinfo;
	char *tempstr = str; /* the mdenslist attribute */
	char *newdev;
	char *attrstr;
	
	/* Parse the mdenslist attribute */
	while (ptr = strchr(tempstr,',')) {
		len = ptr - tempstr;
		strncpy(name, tempstr, len);
		name[len] = '\0';
		/* Do this for each logical device */
		deviceinfo = GetLogicalDevicePointer(name, len);
		DnsItems[devicecount].logical_device = deviceinfo;

		/* Now get the label */
		attrstr = DtamDevAttr((char *)deviceinfo, "mdenslabel");

		/* Use this label in the gettxt call.  How do you get the
		 * default label?  The filename can not contain the
		 * : character.  Therefore, the algorithm here should
		 * search for the first :, then for the ^.
		 */
		DnsItems[devicecount].label =
				GetLogicalDeviceLabel(attrstr);
		devicecount++;
		tempstr = ++ptr;
	}
	/* One more label */
	if (tempstr && strlen(tempstr)) {
		deviceinfo = GetLogicalDevicePointer(tempstr, strlen(tempstr));
		DnsItems[devicecount].logical_device = deviceinfo;
		/* Now get the label */
		attrstr = DtamDevAttr((char *)deviceinfo, "mdenslabel");
		DnsItems[devicecount].label =
				GetLogicalDeviceLabel(attrstr);
		devicecount++; /* True device count */
		cur_num_density_items = devicecount;
	}

	for (i=0; i < devicecount; i++) {
		DnsItems[i].setting = FALSE;
		DnsItems[i].mnem = (XtArgVal)'\0';
	}

	/* Set the first item to true */
	DnsItems[0].setting = TRUE;
}

/*
 *	The File menu lists formattable devices by alias; when one is
 *	selected, this callback is invoked to update the w_fdesc description
 *	taken from the desc attribute in the device table.
 */
void
SetDevDesc(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	Widget		w_ud;
	OlFlatCallData	*olcd = (OlFlatCallData *) call_data;
	char		*dev, *str;
	char		*nalias = NULL;
	int		i;

	FooterMsg(fbase, NULL);
	if (curalias) {
		nalias = DtamMapAlias(str = DeviceItem[olcd->item_index].label);
		if (nalias && (!strcmp(curalias, nalias)) ) {
			/* No change */
			return;
		}
		curalias = STRDUP(nalias);
	}

	if (curdev)
		FREE(curdev);
	curdev = DtamGetDev(curalias,FIRST);
	XtSetArg(arg[0], XtNuserData, &w_ud);
	XtGetValues(wid,  arg, 1);
	XtSetArg(arg[0], XtNlabel, (XtArgVal)DtamDevDesc(curdev));
	XtSetValues(w_ud, arg, 1);
	XtSetArg(arg[0], XtNlabel, (XtArgVal)str);
	XtSetValues(XtParent(w_ud), arg, 1);

	str = DtamDevAttr(curdev,"mdenslist");
	XtSetArg(arg[0], XtNmappedWhenManaged, (str!=NULL));
	XtSetValues(w_dcap, arg, 1);

	/* Fix up the labels, etc. for the density exclusives list */
	ParseMdenslist(str);

	/* Set density to the first item in the density list */
	density = 0;

	XtSetArg(arg[0], XtNitems,	(XtArgVal)DnsItems);
	XtSetArg(arg[1], XtNnumItems,	(XtArgVal)cur_num_density_items);
	XtSetArg(arg[2], XtNitemsTouched, (XtArgVal)cur_num_density_items);
	XtSetValues(w_dns, arg, 3);

	if (str)
		FREE(str);
}

void
MapWatchNotice()
{
	Widget	w_up;

	if (!fwatch.shell) {
		CreateGizmo(fbase.shell, PopupGizmoClass, &fwatch, NULL, 0);

		XtSetArg(arg[0], XtNupperControlArea, &w_up);
		XtGetValues(fwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNwindowHeader, FALSE);
		XtSetValues(fwatch.shell, arg, 1);

		XtSetArg(arg[0], XtNlayoutType,		OL_FIXEDCOLS);
		XtSetArg(arg[1], XtNalignCaptions,	TRUE);
		XtSetArg(arg[2], XtNcenter,		TRUE);
		XtSetArg(arg[3], XtNhPad,		x3mm);
		XtSetValues(w_up, arg, 4);

		XtSetArg(arg[0], XtNheight, 3*y3mm);
		XtCreateManagedWidget("spacer", rectObjClass, w_up, arg, 1);

		XtSetArg(arg[0], XtNalignment,		OL_CENTER);
		XtSetArg(arg[1], XtNgravity,		CenterGravity);
		XtSetArg(arg[2], XtNwidth,		(32*x3mm));
		XtSetArg(arg[3], XtNfont, 		bld_font);
		w_txt = XtCreateManagedWidget("text",
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
	XtSetArg(arg[0], XtNstring, (XtArgVal)GGT(string_doingFmt));
	XtSetValues(w_txt, arg, 1);
	MapGizmo(PopupGizmoClass, &fwatch);
}

/*
 *	It is ad hoc, but useful, to take some empirical times for formatting
 *	and making file systems on diskettes, to allow a reasonable timing
 *	gauge.  ISV's who wish may add dtfmttime and dtmkfstime attributes to
 *	the device table for other devices with fmtcmd and mkfscmd options.
 */
struct {char	f_factor;
	int	t_fmt;
	int	t_mkfs;
	int	t_dos;
} ftime[] = 	{
			{'3',  108,  27, 85},	/* 3.5" diskettes */
			{'3',  108,  48, 88},
			{'5',  48,  30, 70},	/* 5.25" diskettes */
			{'5',  92,  49, 72},
			{'\0',  0,   0,  0},
		};

void	formatCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	diag;
	static  int try = 1;
	static	int last_diag;

	FooterMsg(fbase, NULL);
	OlUpdateDisplay(wid);
	if (!strcmp((char *)client_data, "format"))
		try = 1;
	if (fconfirm_up){
		BringDownPopup(fconfirm.shell);
		fconfirm_up = False;
	}
	diag = DtamCheckMedia(curalias);
	if ( (diag == UNFORMATTED) || 
		( (try > 1) && (diag != NO_DISK) && (diag == last_diag) ) ) {
		try = 1;
		doformat(wid, client_data, call_data);
	}
	else {
		try++;
		last_diag = diag;
		if (!fconfirm.shell)
			CreateGizmo(fbase.shell, ModalGizmoClass, &fconfirm,
						NULL, 0);
		SetModalGizmoMessage(&fconfirm, FmtDiagMsg(diag));
		MapGizmo(ModalGizmoClass, &fconfirm);
		fconfirm_up = True;
	}
}

extern void
doformat OLARGLIST((wid, client_data, call_data))
OLARG(Widget, wid)
OLARG(XtPointer, client_data)
OLGRA(XtPointer, call_data)
{
static	char	*ltag = "LANG=C ";
	char	*devline, *attr, *cmd1 = NULL, *cmd2 = NULL;
	char	form;
	int	n, time = 0, fstype;
	Cursor	cursor;

	FooterMsg(fbase, NULL);
	fmt_done = FALSE;
	MapWatchNotice();
	cursor = GetOlBusyCursor(theScreen);
	XDefineCursor(theDisplay, XtWindow(w_gauge), cursor);
	XDefineCursor(theDisplay, XtWindow(fwatch.shell), cursor);
	XDefineCursor(theDisplay, XtWindow(fbase.shell), cursor);

	/*
	 *	fiddle around if format may be other than default
	 */
	/* density is the index into the DnsItems array at this point.
	 * Use it to get the logical device being pointer to.
	 */
	if (strstr(curdev, DISKETTE)) {
		form = *strpbrk(strstr(curdev,"fmtcmd"),"35");
	}
		/*
		 *	get this now, as mdenslist may obscure the issue
		 */
	devline = DnsItems[density].logical_device;
	if (fmt_type == DOS_FMT) {
		char	*ptr;
		attr = "/usr/bin/dosformat -f ";
		cmd2 = DtamDevAttr(devline, "fmtcmd");
		ptr = strstr(cmd2, "/dev");
		cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+strlen(ptr)+8);
		sprintf(cmd1, "%s%s%s", ltag, attr, ptr);
		FREE(cmd2);
	}
	else {
		char *temp, *spot;
		temp = DtamDevAttr(devline,"fmtcmd");
		spot = strstr(temp,"-v ");
		attr = (char *) MALLOC(strlen(temp)+10);

		if (spot)
		{ /* "-V" (partial verify) flag after "-v" flag */
			*spot = '\0';
			strcpy(attr,temp);
			strcat(attr,"-v -V");
			strcat(attr,spot+2);
		}
		else
			strcpy (attr,temp);
		FREE(temp);

		if (fmt_type == BKUP_FMT) {
			cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+8);
			strcat(strcpy(cmd1,ltag),attr);
			FREE(attr);
		}
		else {
			/* based on the cur_fscmd, get the
			 * corresponding command from the device.tab file.
			 */
			cmd2 = DtamDevAttr(devline, cur_fscmd);

			cmd1 = (char *)MALLOC(strlen(ltag)+strlen(attr)+
							strlen(cmd2)+12);
			sprintf(cmd1,"(%s%s;%s)", ltag, attr, cmd2);
			FREE(attr);
			FREE(cmd2);
			if (attr=DtamDevAttr(devline,"dtmkfstime")) {
				time = atoi(attr);
				FREE(attr);
			}
		}
		if (attr = DtamDevAttr(devline,"dtfmttime")) {
			time += atoi(attr);
			FREE(attr);
		}
	}
	if (time == 0 && strstr(curalias,DISKETTE)) {
	/*
	 *	 check pre-tabulated values
	 */
		for (n = 0; ftime[n].f_factor; n++) {
			if (form == ftime[n].f_factor) {
				if (density == HIGH_DNS)
					n++;
				switch (fmt_type) {
				case DOS_FMT:	time = ftime[n].t_dos;
						break;
				case S5_FILES:	time = ftime[n].t_mkfs;
						/*
						 * and add t_fmt:
						 */
				case BKUP_FMT:	time += ftime[n].t_fmt;
						break;
				}
				break;
			}
		}
	}
	strcat(cmd1, " 2>&1");
	/* 
	 * If we are creating a vxfs file system, add the -o version=1 option
	 * to make the file system backwards compatible (to UnixWare 1.*).
	 */
	{
		char *temp, *spot;
		temp = STRDUP(cmd1);
		if (spot = strstr(temp,"-F vxfs")) {
			cmd1 = (char*)REALLOC(cmd1, strlen(temp)+20);
			*spot = '\0';
			strcpy(cmd1,temp);
			strcat(cmd1,"-F vxfs -o version=1");
			strcat(cmd1,spot+7);
		}
	}
	_Dtam_p3open(cmd1, cmdfp, TRUE);
	XtAddTimeOut(2500, (XtTimerCallbackProc)CheckStderr,
						(XtPointer)(1000*time));
	op = FMT_OP;
	FREE(cmd1);
}

static void
CreatePropSheet(void)
{
static  FsExclItem	PropItems[50];
Widget	w_up, w_cap, w_sc;
FILE	*pfile, *fopen();
char	buf[BUFSIZ], *tmp;


	xinch = OlPointToPixel(OL_HORIZONTAL,72);
	yinch = OlPointToPixel(OL_VERTICAL,72);

	CreateGizmo(w_baseshell, PopupGizmoClass, &prop_popup, NULL, 0);
	w_popup = GetPopupGizmoShell(&prop_popup);
	XtVaGetValues(w_popup, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up, 
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNcenter,		(XtArgVal)FALSE,
			XtNhPad,		(XtArgVal)xinch/2,
			XtNvPad,		(XtArgVal)yinch/3,
			XtNvSpace,		(XtArgVal)yinch/4,
			NULL);

	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_fs),
			NULL);
	
	/* Get the File System types from the FsTable file */
	if (pfile = fopen(GetXWINHome(FS_TABLE), "r")) {
		while (fgets(buf, BUFSIZ, pfile)) {
			if ((tmp = strtok(buf, "\t\n")) == NULL)
				continue;
			PropItems[fs_num].label = strdup(DtamGetTxt(tmp));
			PropItems[fs_num].mnem = 0;
			PropItems[fs_num].setting = (XtArgVal)FALSE;
			if ((tmp = strtok(NULL, "\t\n")) == NULL)
				continue;
			PropItems[fs_num].cmd = strdup(tmp);
			fs_num++;
		}
	}
	else
                ErrorNotice(GetGizmoText(string_propfile), 
                                    GetGizmoText(string_formatError));
	fclose(pfile);
	PropItems[0].setting = (XtArgVal)TRUE;
	/* Set the default mkfs cmd */
	cur_fscmd = strdup(PropItems[0].cmd);	
	w_sc = fs_num < 10 ? w_up:
		XtVaCreateManagedWidget("scrolled", scrolledWindowWidgetClass,
			w_up,
			XtNwidth,	(XtArgVal)((Dimension)3.0*xinch),
			XtNheight,	(XtArgVal)((Dimension)2.0*yinch),
			NULL);
	w_checks = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_sc,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_RECT_BTN,
			XtNlabelJustify,	(XtArgVal)OL_LEFT,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)FsExclFields,
			XtNnumItemFields,	(XtArgVal)(XtNumber(FsExclFields)),
			XtNitems,		(XtArgVal)PropItems,
			XtNnumItems,		(XtArgVal)fs_num,
			NULL);
}

extern void
CreateFormatWindow OLARGLIST((parent))
OLGRA(Widget, parent)
{
	Widget		w_ctl, w_cap, w_fmt, w_devmenu, w_abvbtn;
	char		*dev, *str;
	char *tempstr;

	if (note.shell)
		XtDestroyWidget(note.shell);
	if (strcmp(curalias,"diskette2")==0)
		fbase.icon_pixmap = D2ICON_NAME;
	else if (strcmp(curalias,"diskette1") != 0)
		fbase.icon_pixmap = GENICON_NAME;
	w_baseshell = CreateGizmo(parent, BaseWindowGizmoClass, &fbase, NULL, 0);

	XtSetArg(arg[0], XtNwidth, 44*x3mm);
	XtSetValues(fbase.scroller,  arg, 1);

	XtSetArg(arg[0], XtNlayoutType,	 OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNalignCaptions,TRUE);
	XtSetArg(arg[2], XtNvSpace,	 (2*y3mm));
	XtSetArg(arg[3], XtNvPad,	 (2*y3mm));
	XtSetArg(arg[4], XtNhPad,	 x3mm);
	XtSetArg(arg[5], XtNshadowThickness, 0);

	w_ctl = XtCreateManagedWidget("control",
			controlAreaWidgetClass, fbase.scroller, arg, 6);

	w_fdesc = DevMenu(DeviceItem, 0, N_DEVS, w_ctl, GGT(label_devCaption),
				(XtPointer)SetDevDesc, "fmtcmd", &w_devmenu, 
				&w_abvbtn, 0);
	if (!curalias)
		curalias = DtamMapAlias(DeviceItem[0].label);
	if (!curdev)
		curdev = DtamGetDev(curalias, FIRST);

	XtSetArg(arg[0], XtNlabel, DtamDevDesc(curdev));
	XtSetValues(w_fdesc, arg, 1);
	XtSetArg(arg[0], XtNlabel, DtamDevAlias(curdev));
	XtSetValues(XtParent(w_fdesc), arg, 1);
/*
 *	Density exclusives
 */
	/* In this segment, just set up the labels for the densities */
	str = DtamDevAttr(curdev,"mdenslist");

	XtSetArg(arg[0], XtNposition,		OL_LEFT);
	XtSetArg(arg[1], XtNspace,		x3mm);
	XtSetArg(arg[2], XtNlabel,		GGT(label_dnsCaption));
	XtSetArg(arg[3], XtNmappedWhenManaged,	(str != NULL));

	/* Don't free the str until later - we need to look at it */
	ParseMdenslist(str);
	
	if (str)
		FREE(str);
	XtSetArg(arg[4], XtNalignment,	OL_TOP);
	w_dcap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 5);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
	XtSetArg(arg[2], XtNexclusives,		TRUE);
	XtSetArg(arg[3], XtNsameWidth,		OL_ALL);

	XtSetArg(arg[4], XtNitemFields,		DensExclFields);
	XtSetArg(arg[5], XtNnumItemFields,	(XtArgVal)(XtNumber(DensExclFields)));
	XtSetArg(arg[6], XtNitems,		DnsItems);
	XtSetArg(arg[7], XtNnumItems,		(XtArgVal)cur_num_density_items);
	XtSetArg(arg[8], XtNselectProc,		SetDensity);
	XtSetArg(arg[9], XtNlayoutType,		OL_FIXEDCOLS);
	w_dns = XtCreateManagedWidget("density",
			flatButtonsWidgetClass, w_dcap, arg, 10);
/*
 *	type of format (raw, file system or DOS)
 */
	SET_EXCL(FmtItems, 0, bkupFmt, TRUE /*set*/);
	SET_EXCL(FmtItems, 1, s5Fmt, FALSE /*unset*/);
	SET_EXCL(FmtItems, 2, dosFmt, FALSE /*unset*/);

	XtSetArg(arg[0], XtNlabel,		GGT(label_fmtCaption));
	XtSetArg(arg[1], XtNposition,		OL_LEFT);
	XtSetArg(arg[2], XtNspace,		x3mm);

	w_cap = XtCreateManagedWidget("caption",
			captionWidgetClass, w_ctl, arg, 3);

	XtSetArg(arg[0], XtNtraversalOn,	TRUE);
	XtSetArg(arg[1], XtNbuttonType,		OL_RECT_BTN);
	XtSetArg(arg[2], XtNexclusives,		TRUE);
	XtSetArg(arg[3], XtNitemFields,		ExclFields);
	XtSetArg(arg[4], XtNnumItemFields,	NUM_ExclFields);
	XtSetArg(arg[5], XtNitems,		FmtItems);
	XtSetArg(arg[6], XtNnumItems,		XtNumber(FmtItems));
	XtSetArg(arg[7], XtNselectProc,		SetFormatType);

	w_fmt = XtCreateManagedWidget("type",
			flatButtonsWidgetClass, w_cap, arg, 8);

	CreatePropSheet();

	MapGizmo(BaseWindowGizmoClass, &fbase);
}
#define MSGFILENAME "dtmedia"
#define MSGLEN 60
static char msgd[MSGLEN];

static char *
GetLogicalDeviceLabel OLARGLIST((labelattr))
OLGRA(char *, labelattr)
{
	char *p;
	char *labelval = NULL;
	
	strncpy(msgd, labelattr, MSGLEN-1);
	msgd[MSGLEN] = '\0';
	p = strchr(msgd, '^');
	if (!p)
		return(NULL);
	*p = '\0';
	/* Now get the default message */
	p = strchr(labelattr, '^');
	p++;
	labelval = gettxt((char *)msgd, p);
	return(labelval);
	
	

} /* GetLogicalDeviceLabel() */



/*
 * GetLogicalDevicePointer() - given a logical device name,
 * find the device entry containing all attributes for it,
 * in the device.tab file.
 */

static char *
GetLogicalDevicePointer OLARGLIST((devicename, len))
OLARG(char *, devicename)
OLGRA(int, len)
{
	char *deviceinfo;

	deviceinfo = DtamGetDev((char *)devicename, FIRST);
	for(; deviceinfo && strncmp(devicename, deviceinfo, len); ) {
		deviceinfo = DtamGetDev((char *)devicename, NEXT);
	}
	return(deviceinfo);
} /* GetLogicalDevicePointer */
