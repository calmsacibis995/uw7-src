/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dtcall.c	1.29.1.1"
#endif

#include <stdio.h>
#include <wait.h>
#include <signal.h>
#include <limits.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <ControlAre.h>
#include <PopupWindo.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <InputGizmo.h>
#include <LabelGizmo.h>
#include <ChoiceGizm.h>
#include "uucp.h"
#include "error.h"


/*
 * DO NOT REMOVE.  It is used to overwrite the default help
 * file set in the error.h file.
 */

#define	XTERM	"xterm"
#define	CU	"dtcu"

static Boolean	showStatusWindow = 0;
extern void	HelpCB();
extern int	ExecProgram();
extern char *	GetAttrValue();
extern	int	AttrParse();
extern	void	NotifyUser ();
extern	void	rexit();
extern void	ModifyPhoneCB ();
extern void	ModifyNameCB ();

static	void	CreatePopup();
static	void	DialCB();
static void	DialCB();
static	void	ExitMain();
static	void	WindowManagerEventHandler();
static void	RestartQD();
static void	ReapChild();
static void	DtcallRegisterHelp();

static void 	SpeedChangedCB();
static String devSpeed=0;
Widget	root;
char 	*port;
char	*type;
char	ctype[16];

#define TXT_TITLE		"dtcall:1" FS "Call System: Properties"
#define FormalClientName   	TXT_TITLE

#define ClientName		"dtcall:2" FS "Call System"
#define ClientClass		"dtcall"

#define TXT_DEV_NO		"dtcall:10" FS "device not specified"
#define TXT_LOGIN_NO		"dtcall:11" FS "unable to determine user login home"
#define TXT_NODE_NO		"dtcall:12" FS "unable to load properties"

char *		ApplicationName;
char *		Program;

/* Link list of name=value structures */

typedef	struct	stringll {
	char	*name;
	char	*value;
	struct	stringll  *next;
} stringll_t;

typedef struct _QuickDial {
	PopupGizmo *	dialPrompt;
	Widget		popup;
	InputGizmo	*os_p;
	Setting		*sp;
	Boolean		speedMapped;
} QuickDial, *QuickDialPtr;

QuickDialPtr	qd;

static Boolean VerifyOtherSpeed();
	
ExecItem program = {
	RestartQD, NULL, NULL, 0, 0, { NULL, "-T", NULL}
};

static OlDtHelpInfo help_info[] = {NULL, NULL,HELP_FILE, NULL, NULL};

static HelpText AppHelp = {
    title_dial, HELP_FILE, help_dtcall,
};

typedef enum {
	DialApply, DialReset, DialCancel, DialHelp
} DialMenuItemIndex;

static MenuItems  DialMenuItems[] = {
	{(XA)True, label_dial, mnemonic_dial},
	{(XA)True, label_reset, mnemonic_reset},
	{(XA)True, label_cancel, mnemonic_cancel},
	{(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char*)&AppHelp },
	{ 0 }
};

static MenuGizmo DialMenu = {
	NULL, "dial", "_X_", DialMenuItems, DialCB, NULL, CMD, OL_FIXEDROWS, 1, 0
};

#define MAXPHONE 18

typedef struct _Input {
	Setting number;
} Input;

static Input Number = {""};
static Input OtherSpeed = {""};

static InputGizmo otherSpeedField = {
        NULL,
	"otherSpeed",
	"",
	"",
	&OtherSpeed.number,
	10,
	(void (*)())0,
};

static InputGizmo phoneField = {
        NULL,
	"phone",
	label_phoneOrName,
	"",
	&Number.number,
	MAXPHONE,
	(void (*)())0,
};


static char *Speeds[] = {
	"300", "1200", "2400", "4800", "9600", 
	"14400", "19200", "28800", "38400", 
};

typedef enum {
	Speed300, Speed1200, Speed2400, Speed4800, Speed9600,
	Speed14400, Speed19200, Speed28800, Speed38400, SpeedOther, SpeedAny
} SpeedMenuIndex;

static MenuItems  SpeedItems[] = {
	{(XA)True,	label_b300,	""},
	{(XA)True,	label_b1200,	""},
	{(XA)True,	label_b2400,	""},
	{(XA)True,	label_b4800,	""},
	{(XA)True,	label_b9600,	""},
	{(XA)True,	label_b14400,	""},
	{(XA)True,	label_b19200,	""},
	{(XA)True,	label_b28800,	""},
	{(XA)True,	label_b38400,	""},
	{(XA)True,	label_other,	""},
	{(XA)True,	label_any,	""},
	{ 0 }
};

static MenuGizmo SpeedMenu = {
	NULL, "speed", "_X_", SpeedItems, SpeedChangedCB, NULL, EXC,
        OL_FIXEDROWS,	/* Layout type	*/
        3,		/* Measure	*/
        SpeedAny	/* Default item	*/
};

typedef enum {
	ParityEven, ParityOdd, ParityNone
} ParityMenuIndex;

static MenuItems  ParityItems[] = {
	{(XA)True,	label_even,	mnemonic_even},
	{(XA)True,	label_odd,	mnemonic_odd},
	{(XA)True,	label_none,	mnemonic_none},
	{ 0 }
};
static MenuGizmo ParityMenu = {
	NULL, "parity", "_X_", ParityItems, NULL, NULL, EXC
};

typedef enum {
	CharSize7, CharSize8
} CharSizeMenuIndex;

static MenuItems  CharSizeItems[] = {
	{(XA)True,	label_seven,	mnemonic_seven},
	{(XA)True,	label_eight,	mnemonic_eight},
	{ 0 }
};

static MenuGizmo CharSizeMenu = {
	NULL, "charSize", "_X_", CharSizeItems, NULL, NULL, EXC
};

typedef enum {
	DuplexFull, DuplexHalf
} DuplexMenuIndex;

static MenuItems  DuplexItems[] = {
	{(XA)True,	label_full,	mnemonic_full},
	{(XA)True,	label_half,	mnemonic_half},
	{ 0 }
};

static MenuGizmo DuplexMenu = {
	NULL, "duplex", "_X_", DuplexItems, NULL, NULL, EXC
};

/* show Toggle for status window */
static MenuItems showStatusItems[] = {
	{TRUE, label_showStatusWindow},
	{NULL}
};

static MenuGizmo showStatusMenu = {
	0, "showStatusMenu", NULL, showStatusItems, NULL, NULL, CHK,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

typedef struct _MySettings {
	Setting speed;
	Setting parity;
	Setting charSize;
	Setting duplex;
} MySettings;

static MySettings MySetting;

static ChoiceGizmo SpeedChoice = {
	NULL,
	"speed",
	label_speed,
	&SpeedMenu,
	&MySetting.speed,
};

static ChoiceGizmo ParityChoice = {
	NULL,
	"parity",
	label_parity,
	&ParityMenu,
	&MySetting.parity,
};

static ChoiceGizmo CharSizeChoice = {
	NULL,
	"charSize",
	label_charSize,
	&CharSizeMenu,
	&MySetting.charSize,
};

static ChoiceGizmo DuplexChoice = {
	NULL,
	"duplex",
	label_duplex,
	&DuplexMenu,
	&MySetting.duplex,
};

static GizmoRec SpeedFields[] = {
	{ ChoiceGizmoClass, &SpeedChoice },
	{ InputGizmoClass, &otherSpeedField },
};


static LabelGizmo speedFields = {
	NULL,
	"speedFields",
	NULL,
	SpeedFields,
	XtNumber(SpeedFields),
	OL_FIXEDCOLS,	
	2,
};

	
static GizmoRec Dials[] = {
	{ InputGizmoClass, &phoneField },
	{ LabelGizmoClass, &speedFields },
	{ ChoiceGizmoClass, &ParityChoice },
	{ ChoiceGizmoClass, &CharSizeChoice },
	{ ChoiceGizmoClass, &DuplexChoice },
	{ MenuBarGizmoClass, &showStatusMenu},
};

static	char	title[40];
static PopupGizmo QuickDialPrompt = {
	NULL,
	"dial",
	title,
	&DialMenu,
	Dials,
	XtNumber(Dials),
	NULL,
	NULL,
	NULL,
	NULL,
	100,
};

static void
DtcallRegisterHelp()
{
  help_info->filename =  HELP_FILE;
  help_info->app_title    =  GetGizmoText(title_dial);
  help_info->section = GetGizmoText(STRDUP(help_dtcall));
  OlRegisterHelp(OL_WIDGET_HELP,qd->popup, "dtcall", OL_DESKTOP_SOURCE,
   (XtPointer)&help_info);
}

static void
ExitMain()
{
	exit(0);
}

/*
 * CreatePopup
 *
 */

static void
CreatePopup(w)
Widget    w;
{

	static InputGizmo *gp;
	Widget w_acu;
	Widget dialOtherSpeed;
	Window		another_window;
	if (qd == NULL) {
		ApplicationName = GGT (ClientName);
		SET_HELP(AppHelp);
		sigset(SIGCHLD, ReapChild);
		qd = (QuickDial *)XtMalloc (sizeof(QuickDial));
		qd->dialPrompt = CopyGizmo(PopupGizmoClass, &QuickDialPrompt);
		qd->popup = CreateGizmo(w, PopupGizmoClass, qd->dialPrompt, NULL, 0);
		sf->toplevel = qd->popup;
		qd->speedMapped = True;
	}
	XtVaSetValues (
		qd->popup,
		XtNmappedWhenManaged,           (XtArgVal) False,
		XtNwmProtocolInterested,        (XtArgVal) OL_WM_DELETE_WINDOW,
		0
	);

        DtcallRegisterHelp();
	XtRealizeWidget(qd->popup);
	another_window = DtSetAppId (
				XtDisplay(qd->popup),
				XtWindow(qd->popup),
				port);
	if (another_window != None) {
		XMapWindow(XtDisplay(qd->popup), another_window);
		XRaiseWindow(XtDisplay(qd->popup), another_window);
		XFlush(XtDisplay(qd->popup));
		ExitMain();
	}

	OlAddCallback (
		qd->popup,
		XtNwmProtocol, WindowManagerEventHandler,
		(XtPointer) 0
	);

	MapGizmo(PopupGizmoClass, qd->dialPrompt);
	gp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
			qd->dialPrompt,
			GetGizmoGizmo,
			"phone");
	qd->os_p = (InputGizmo *)QueryGizmo(PopupGizmoClass,
			qd->dialPrompt,
			GetGizmoGizmo,
			"otherSpeed");
	w_acu = gp->captionWidget;
	dialOtherSpeed = qd->os_p->captionWidget;
	/* start off with autoselect on speed so other
		text field is unmapped */
	if (qd->speedMapped == True) {
		if (XtIsRealized(dialOtherSpeed)) XtUnmapWidget(dialOtherSpeed);
		qd->speedMapped = False;
	}
	if (strcmp(type,"datakit") == 0) {
		XtAddCallback(
		    gp->textFieldWidget,
		    XtNmodifyVerification,
		    (XtCallbackProc)ModifyNameCB,
		    (caddr_t) qd->dialPrompt->message
		);
		strcpy(ctype, "-cDK");
		if (XtIsRealized(w_acu))
			XtUnmapWidget(w_acu);
		else
			SetValue(w_acu, XtNmappedWhenManaged, False, NULL);
	} else
		if  (strcmp(type,"uudirect") == 0) {
			strcpy(ctype, "-cDirect");
			if (XtIsRealized(w_acu))
				XtUnmapWidget(w_acu);
			else
			SetValue(w_acu, XtNmappedWhenManaged, False, NULL);
	} else {
		XtAddCallback(
		    gp->textFieldWidget,
		    XtNmodifyVerification,
		    (XtCallbackProc)ModifyPhoneCB,
		    (caddr_t) qd->dialPrompt->message
		);
		strcpy(ctype, "-cACU");
		if (XtIsRealized(w_acu))
			XtMapWidget(w_acu);
		else
			SetValue(w_acu, XtNmappedWhenManaged, TRUE, NULL);
	}
} /* CreatePopup */

static void
DialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	register i;
	char 	line[BUFSIZ], speed[MAXSIZE];
	char *x;
	static char path[PATH_MAX];
	static char cupath[PATH_MAX];
	OlFlatCallData * p = (OlFlatCallData *)call_data;
	PopupGizmo	* popup = qd->dialPrompt;
	Setting *os;
	Setting *fp;
	Setting *pp;
	Setting *cp;
	Setting *dp;
	Widget status_W;
	Boolean acu = False;
	Arg args[2];
	int	n, portNumber;

	if (!strncmp(port, "com", 3)) {
		sscanf(port, "com%d", &portNumber);
			/* use h version of port number for com ports */
		sprintf(line, "-ltty%.2dh", portNumber-1);
	} else	sprintf(line, "-l%s", port);
	switch (p->item_index) {
	case DialApply:
		SetPopupMessage(qd->dialPrompt, "");
		ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
		fp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"phone");
		qd->sp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"speed");
		os = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"otherSpeed");
		pp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"parity");
		cp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"charSize");
		dp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"duplex");
		status_W = (Widget) QueryGizmo(PopupGizmoClass,
				popup,
				GetGizmoWidget,
				"showStatusMenu");
		XtSetArg(args[0], XtNset, &showStatusWindow);
		OlFlatGetValues(status_W, 0, args, 1);

#ifdef DEBUG
	fprintf(stderr,"current values:sp=%s os=%s pp=%s cp=%s dp=%s\n",
		qd->sp->current_value,	
		os->current_value,	
		pp->current_value,	
		cp->current_value,	
		dp->current_value);
#endif
         /*
          * data check
          */

		n = (int) qd->sp->current_value;
		if ((strcmp(SpeedItems[n].label, label_other)) == 0) {
			/* do check for other speeds, since blank
			is invalid and 14400 and 28800 need to get reset
			to the next higher value (19200 and 38400 respectively)*/
			if ((VerifyOtherSpeed(os->current_value)) == INVALID)
				return;
		}
		/* if the connection is acu, check the phone number */
		if ((strcmp(type, "datakit") !=0) 
		&& (strcmp(type, "uudirect") != 0)) {
			if (strlen(fp->current_value) == 0) {
				SetPopupMessage(qd->dialPrompt, GGT(string_badPhone));
				return;
			} else
			if ( strlen(fp->current_value) !=
			     strspn(fp->current_value, "0123456789=-*#") ) {
			    /* it's not a legimtimate telno */
			    
			    SetPopupMessage(qd->dialPrompt, GGT(string_badPhone));
			    return;
			}
			acu = True;
		}
		/*
		* if OK, pass it to the cu command and store it for the future
		*/

                if (program.exec_argv[0] == NULL) {
                        x = getenv("XWINHOME");
                        if (x) {
                                sprintf(cupath, "%s%s%s", x, "/desktop/rft/",  CU);
                                sprintf(path, "%s%s%s", x, "/bin/",  XTERM);
                        } else {
                                sprintf(cupath, "/usr/X/desktop/rft/", CU);
                                sprintf(path, "/usr/X/bin/", XTERM);
                        }
                        program.exec_argv[0] = strdup(path);
                }
		for (i = 2; i < 14; i++) {
			if (program.exec_argv[i] != NULL) {
				free (program.exec_argv[i]);
				program.exec_argv[i] = NULL;
			}
		}
		i = 2;
		program.exec_argv[i++] = strdup(GGT(cu_title));
		program.exec_argv[i++] = strdup("-E ");
		program.exec_argv[i++] = strdup(cupath);
			/* we need to show status with other than a
			-d, since xterm interprets the -d as a
			display variable and does not pass it on
			to cu properly. We run an intermediate command
			dtcu in an xterm. dtcu will parse this value
			and run cu with -d if it has a 1 set, or no
			argument if 0 is set. */
		if (showStatusWindow == True) {
			program.exec_argv[i++] = strdup("1");
		} else {
			program.exec_argv[i++] = strdup("0");
		}
		program.exec_argv[i++] = strdup(line);
		program.exec_argv[i++] = strdup(ctype);
		/* check if speed is "Any".  If not, get the speed string */ 
		n = (int) qd->sp->current_value;
		if ((strcmp(SpeedItems[n].label, label_other)) == 0)  {
				/* do other speed stuff */
			if((int) os->current_value) {
				sprintf(speed, "%s%s", "-s" , os->current_value);
				program.exec_argv[i++] = strdup(speed);
			}
		} else 
		/* check if speed is not other or any then set up
			the speed to use */
		if ((strcmp(SpeedItems[n].label, label_any)) != 0)  {
			sprintf(speed, "-s%s", Speeds[n]);
			program.exec_argv[i++] = strdup(speed);
		}
		n=(int)cp->current_value; 
		if (n) {
			program.exec_argv[i++] = strdup("-b8");
		} else {
			program.exec_argv[i++] = strdup("-b7");
		}
		
		if((int)dp->current_value) 
			program.exec_argv[i++] = strdup("-h");
		n = (int)pp->current_value;
		if (n == 2) {
			program.exec_argv[i++] = strdup("-p");
		} else
		if (n == 1) {
			program.exec_argv[i++] = strdup("-o");
		} else {
			program.exec_argv[i++] = strdup("-e");
		}	
		if (acu == True)
			program.exec_argv[i++] = strdup(fp->current_value);
#ifdef DEBUG
	fprintf(stderr,"dtcall cmdline=");
	for (i=0; i < 14; i++) {
		fprintf(stderr,"%s ", program.exec_argv[i]);
	}
	fprintf(stderr,"\n");
#endif
		if(ExecProgram(&program) == -1) {
			SetPopupMessage(qd->dialPrompt,GGT(string_badExec));
		} else {
			SetPopupMessage(qd->dialPrompt, GGT(string_startCU));
			BringDownPopup(qd->popup);
		}
		ManipulateGizmo(PopupGizmoClass, popup, ApplyGizmoValue);
		break;
	case DialReset:
		ManipulateGizmo(PopupGizmoClass, popup, ReinitializeGizmoValue);
		SpeedChangedCB(w, 0, 0);
		break;
	case DialCancel:
		BringDownPopup(qd->popup);
		ExitMain();
		break;
	default:
#ifdef debug
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
#endif
		break;
	}

} /* DialCB */

main( argc, argv )
int argc;
char *argv[];
{

	extern char *basename(char *);
	char	stderr_file[PATH_MAX];
	char	*home;
	char  	*system;
	char	target[UNAMESIZE];
	char	*device;
	int  	c, x, i;
	FILE	*stderr_p;
	stringll_t	*attr_list;

	Program = basename(argv[0]);
	root = InitializeGizmoClient(ClientName, ClientClass,
		Program,
		NULL, NULL,
		NULL, 0,
		&argc, argv,
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		0,
		NULL,
		NULL,
		NULL);

	/* parse command line */

	port = 0;
	type = 0;
	system = 0;
	device = 0;

	if (argc != 2)
		rexit(2, TXT_DEV_NO, "");
	else
		device = argv[1];

	sprintf(title, GGT(ClientName));

	/* get login home */
	home = getenv("HOME");
	if (!home)
		home = "";

	(void)umask(022);

	/* create path name to the property file */

	(void)sprintf(target, "%s/.port/%s", home, device);

#ifdef debug
	(void)fprintf(stderr, "reading the attribute file: \"%s\"\n", target);

#endif
	/* parse the attr.node file */

	sf = (SystemFile *) XtMalloc(sizeof(SystemFile));
	sf->category = NULL;
	for  (i=0; i < MAXPAGES; i++)
		sf->pages[i]= NULL;
	

	if (AttrParse(target, &attr_list) == -1) {
		rexit(4, TXT_NODE_NO, target);
	}

	system = (char *)GetAttrValue(attr_list, "SYSTEM_NAME");
	port = (char *)GetAttrValue(attr_list, "PORT");
	type = (char *)GetAttrValue(attr_list, "TYPE");

#ifdef debug
	fprintf( stderr, "PORT=%s\nTYPE=%s\n",
		port, type);
#endif
	if (port == (char *)NULL || type == (char*)NULL)
		rexit(5, TXT_NODE_NO, target);

	CreatePopup(root);

	XtMainLoop();

}/* main */

static void
RestartQD()
{
	
	/* probably need to sensitize the button */
	switch (program.exit_code) {
		case 0:
			SetPopupMessage(qd->dialPrompt, "");
			ExitMain();
		case 29:
			SetPopupMessage(qd->dialPrompt, GGT(string_killedCU));
			break;
		case 39:
			SetPopupMessage(qd->dialPrompt, GGT(string_badPhone));
			break;
		case 40:
			SetPopupMessage(qd->dialPrompt, GGT(string_usageCU));
			break;
		case 41:
			SetPopupMessage(qd->dialPrompt, GGT(string_exceed58));
			break;
		case 43:
			SetPopupMessage(qd->dialPrompt, GGT(string_connectFail));
			break;
		case 45:
			SetPopupMessage(qd->dialPrompt, GGT(string_lostConnect));
			break;
		case 47:
			SetPopupMessage(qd->dialPrompt, GGT(string_lostCarrier));
			break;
		case 49:
			SetPopupMessage(qd->dialPrompt, GGT(string_dialFail));
			break;
		case 50:
			SetPopupMessage(qd->dialPrompt, GGT(string_scriptFail));
			break;
		case 51:
			SetPopupMessage(qd->dialPrompt, GGT(string_deviceFail));
			break;
		case 52:
			SetPopupMessage(qd->dialPrompt, GGT(string_noDevice));
			break;
		case 53:
			SetPopupMessage(qd->dialPrompt, GGT(string_noSystem));
			break;
		default:
			SetPopupMessage(qd->dialPrompt, GGT(string_unknownFail));
			break;
		}
		XtPopup(qd->popup, XtGrabNone);
} /* RestartQD */

/*
 * ReapChild - the child terminated, so find out why and set the
 * timmer to call the retry procedure.
 */
static void
ReapChild()
{
	int	pid;
	int	status;

	if((pid = wait(&status)) == -1)
		return;
	if(program.pid == pid) {
		program.exit_code = WEXITSTATUS(status);
		XtAddTimeOut(0L, (XtTimerCallbackProc)RestartQD, NULL);
		return;
	}
} /* ReapChild */

static void
WindowManagerEventHandler(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlWMProtocolVerify *	p = (OlWMProtocolVerify *)call_data;

	switch (p->msgtype) {
	case OL_WM_DELETE_WINDOW:
#ifdef debug
		fprintf (stdout, "Delete yourself\n");
#endif
		ExitMain ();
		break;

	case OL_WM_SAVE_YOURSELF:
		/*
		 *	Do nothing for now; just respond.
		 */
#ifdef debug
		fprintf (stdout, "Save yourself\n");
#endif
		ExitMain ();
		break;

	default:
#ifdef debug
		fprintf (stdout, "Default action\n");
#endif
		OlWMProtocolAction(wid, p, OL_DEFAULTACTION);
		break;
	}
} /* WindowManagerEventHandler */



static
void SpeedChangedCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	Widget speed_widget;
	Widget dialOtherSpeed;
	int	n;

	ManipulateGizmo(PopupGizmoClass, qd->dialPrompt, GetGizmoValue);
	speed_widget = (Widget)QueryGizmo (PopupGizmoClass,
				qd->dialPrompt,
				GetGizmoWidget,
				"speed");
	qd->sp = (Setting *)QueryGizmo (PopupGizmoClass,
				qd->dialPrompt,
				GetGizmoSetting,
				"speed");

	dialOtherSpeed = qd->os_p->captionWidget;
		/* check if speed is "Any".  If not, get the speed string */ 
	n = (int) qd->sp->current_value;
	
	if ((strcmp(SpeedItems[n].label, label_b14400)) == 0) {
		NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		OlVaFlatSetValues(speed_widget,
			Speed19200,
			XtNset,
			True,
			0);
		n =  Speed19200;
	} else
	if ((strcmp(SpeedItems[n].label, label_b28800)) == 0) {
		NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		OlVaFlatSetValues(speed_widget,
			Speed38400,
			XtNset,
			True,
			0);
		n =  Speed38400;
	} else
	if ((strcmp(SpeedItems[n].label, label_other)) == 0)  {
		/* do other speed stuff */
		if (qd->speedMapped == False) {
                	XtMapWidget(dialOtherSpeed);
			qd->speedMapped = True;
		}
		XtVaGetValues(qd->os_p->textFieldWidget, XtNstring, devSpeed, 0);
		AcceptFocus(dialOtherSpeed);
	} else 

		if (qd->speedMapped == True) {
			XtUnmapWidget(dialOtherSpeed);
			qd->speedMapped = False;
		}

}


/* VerifyOtherSpeed
 *
 */
static Boolean
VerifyOtherSpeed (speed)
char  *speed;
{
	Widget sp_widget;
	Widget dialOtherSpeed;
#ifdef DEBUG
	fprintf(stderr, "speed=%s\n",speed);
#endif
	dialOtherSpeed = qd->os_p->captionWidget;
	sp_widget = (Widget)QueryGizmo (PopupGizmoClass, 
			qd->dialPrompt,
			GetGizmoWidget,
			"speed");
	if (speed == NULL) {
		NotifyUser(sf->toplevel, GGT(string_blankSpeed));
		return INVALID;
	} else
	if (strcmp(speed, "") == 0) {
		NotifyUser(sf->toplevel, GGT(string_blankSpeed));
		return INVALID;
	} else
	if ((strchr(speed, ' ') != NULL)) {
		/* spaces not allowed in string */
		NotifyUser(sf->toplevel, GGT(string_badExpect));
		return INVALID;
	}
	if ((strcmp(speed, "14400") != 0) &&
		(strcmp(speed, "28800") != 0)) return VALID;



	/*check for 14400 */
	if ((strcmp(speed, "14400") == 0)) {
		NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		qd->sp->current_value = (XtPointer) Speed19200;
		XtVaSetValues(qd->os_p->textFieldWidget, XtNstring, "", 0);
		OlVaFlatSetValues(sp_widget,
			Speed19200,
			XtNset, 
			True, 
			0);
		ManipulateGizmo(PopupGizmoClass, qd->dialPrompt, ApplyGizmoValue);
	}
	if ((strcmp(speed, "28800") == 0)) {
		NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		qd->sp->current_value = (XtPointer) Speed38400;
		XtVaSetValues(qd->os_p->textFieldWidget, XtNstring, "", 0);
		OlVaFlatSetValues(sp_widget,
			Speed38400,
			XtNset, 
			True, 
			0);
		ManipulateGizmo(PopupGizmoClass, qd->dialPrompt, ApplyGizmoValue);
	}
	if (qd->speedMapped == True) {
		XtUnmapWidget(dialOtherSpeed);
		qd->speedMapped = False;
	}
	return INVALID;
} /* VerifyOtherSpeedCB() */

