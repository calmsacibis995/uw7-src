#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dial.c	1.48.1.1"
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

#define	XTERM	"xterm"
#define	CU	"dtcu"

static Boolean showStatusWindow = 0;
static int action;
static int speedFound;
static String 	devSpeed;
static char	ctype[16];
static void	SpeedChangedCB();
static void	DialCB();
static void	RestartQD();
static void	ReapChild();

extern void	HelpCB();
static Boolean VerifyOtherSpeed();
static void	GetCurrentSpeed();
extern int	ExecProgram();
extern void	ModifyPhoneCB ();
extern void	ModifyNameCB ();
extern void	DeviceNotifyUser ();

typedef struct _QuickDial {
	PopupGizmo *	dialPrompt;
	Widget		popup;
	InputGizmo *os_p;
	Setting	 *sp;
	Boolean speedMapped;
} QuickDial, *QuickDialPtr;

QuickDialPtr	qd;
	
ExecItem program = {
	RestartQD, NULL, NULL, 0, 0, { NULL, "-T", NULL}
};
 
extern void callRegisterHelp(Widget, char *, char *);
static OlDtHelpInfo help_info[] = {NULL, NULL,HELP_FILE, NULL, NULL};

static HelpText AppHelp = {
    title_dial, HELP_FILE, help_dial,
};

typedef enum {
	DialApply, DialReset, DialCancel, DialHelp
} DialMenuItemIndex;

static MenuItems  DialMenuItems[] = {
	{(XA)True, label_dial, mnemonic_dial},
	{(XA)True, label_reset, mnemonic_reset},
	{(XA)True, label_cancel, mnemonic_cancel},
	{(XA)True, label_help, mnemonic_help, NULL, HelpCB, (char *)&AppHelp },
	{ 0 }
};

static MenuGizmo DialMenu = {
	NULL, "dial", "_X_", DialMenuItems, DialCB, NULL, CMD, OL_FIXEDROWS, 1, 0
};

#define MAXPHONE 18

typedef struct _Input {
	Setting number;
} Input;

static Input OtherSpeed = {""};
static Input Number = {""};

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

typedef enum {
	Speed300, Speed1200, Speed2400, Speed4800, Speed9600,
	Speed14400, Speed19200, Speed28800, Speed38400, SpeedOther, SpeedAny
} SpeedMenuIndex;


static char *Speeds[] = {
        "300", "1200", "2400", "4800", "9600",
        "14400", "19200", "28800", "38400", "Other", "Any" 
};


static MenuItems  SpeedItems[] = {
	{(XA)True,	label_b300,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b1200,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b2400,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b4800,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b9600,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b14400,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b19200,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b28800,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_b38400,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_other,	"", NULL, NULL, NULL, (XA) False, NULL, NULL},
	{(XA)True,	label_any,	"", NULL, NULL, NULL, (XA) True, NULL, NULL},
	{ 0 }
};
static MenuGizmo SpeedMenu = {
	NULL, "speed", "_X_", SpeedItems, SpeedChangedCB, NULL, EXC,
        OL_FIXEDROWS,	/* Layout type	*/
        3,		/* Measure	*/
        SpeedAny,	/* Default item	*/
	0

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


/* Show Toggle status window  */
static MenuItems showStatusItems[] = {  
	{TRUE, label_showStatusWindow},
	{ NULL }
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

static GizmoRec SpeedFields[] =  {
	{ ChoiceGizmoClass, &SpeedChoice },
	{ InputGizmoClass, &otherSpeedField },
};

static LabelGizmo speedFields = {
	NULL,
	"speedfields",
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

static PopupGizmo QuickDialPrompt = {
	NULL,
	"dial",
	title_quickDial,
	&DialMenu,
	Dials,
	XtNumber(Dials),
};

/*
 * QuickDialCB
 *
 */

extern void
QuickDialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	String type;
	static InputGizmo *gp;
	int i, speed_cnt;
	Setting *os;
	static Boolean	first_time = True;
	PopupGizmo	* popup;

	/* dial from Actions menu brings you here to setup
	the dial screen */

	ClearFooter(df->footer);
	/* nothing selected, just return */
	if (df->select_op == (DmObjectPtr) NULL) {
        	DeviceNotifyUser(df->toplevel, GGT(string_noSelect));
		return;
	} else {
		type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
		devSpeed = ((DeviceData*)(df->select_op->objectdata))->portSpeed;
	}
	
#ifdef DEBUG
	fprintf(stderr,"devSpeed=%s\n",devSpeed);
#endif
	if (qd == NULL) {
		SET_HELP(AppHelp);
#ifdef POPUP
		sigset(SIGCLD, ReapChild);
#endif
		qd = (QuickDial *)XtMalloc (sizeof(QuickDial));
		qd->dialPrompt = CopyGizmo(PopupGizmoClass, &QuickDialPrompt);
		qd->popup = CreateGizmo(w, PopupGizmoClass, qd->dialPrompt, NULL, 0);
		df->QDPopup = qd->popup;
		qd->speedMapped = True;
	}
	popup = qd->dialPrompt;
	callRegisterHelp(df->QDPopup, title_dial, help_dtcall);
	SetPopupMessage(qd->dialPrompt, "");
	MapGizmo(PopupGizmoClass, qd->dialPrompt);
	if (first_time) {
		first_time = False;
		gp = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				qd->dialPrompt,
				GetGizmoGizmo,
				"phone");
		qd->os_p = (InputGizmo *)QueryGizmo(PopupGizmoClass,
				qd->dialPrompt,
				GetGizmoGizmo,
				"otherSpeed");
		df->w_acu = gp->captionWidget;
	}

	XtRemoveAllCallbacks(
	    qd->os_p->textFieldWidget,
	    XtNmodifyVerification
	);
	XtRemoveAllCallbacks(
	    gp->textFieldWidget,
	    XtNmodifyVerification
	);
	if (qd->speedMapped == True) {
		qd->speedMapped = False;
		if (XtIsRealized(qd->os_p->captionWidget))
			XtUnmapWidget(qd->os_p->captionWidget);
	}
	GetCurrentSpeed();
	if (strcmp(type,"datakit") == 0) {
		XtAddCallback(
		    gp->textFieldWidget,
		    XtNmodifyVerification,
		    (XtCallbackProc)ModifyNameCB,
		    (caddr_t) qd->dialPrompt->message
		);
		strcpy(ctype, "-cDK");
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
	} else 
		if (strcmp(type, "uudirect") == 0) {
		strcpy(ctype, "-cDirect");
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
	} else { /* ACU type */
		XtAddCallback(
		    gp->textFieldWidget,
		    XtNmodifyVerification,
		    (XtCallbackProc)ModifyPhoneCB,
		    (caddr_t) qd->dialPrompt->message
		);
		strcpy(ctype, "-cACU");
		if (XtIsRealized(df->w_acu))
			XtMapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, TRUE, NULL);
	}
} /* QuickDialCB */

static
void DialCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	register i;
  	static char	speed[MAXSIZE];
	static char 	path[PATH_MAX];
	char 	line[BUFSIZ];
	char *x;
	OlFlatCallData * p = (OlFlatCallData *)call_data;
	PopupGizmo	* popup = qd->dialPrompt;
	static char cupath[PATH_MAX];
	Widget status_W;
	Setting *os;
	Setting *fp;
	Setting *pp;
	Setting *cp;
	Setting *dp;
	String  port = ((DeviceData*)(df->select_op->objectdata))->portNumber;
	String  type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
	Boolean acu = False;
	String speed_str;
	Arg args[2];
	int	j,n, portNumber;

	if (!strncmp(port, "com", 3)) {
		sscanf(port, "com%d", &portNumber);
		/* use h version of port number  - hardware handshaking */
		sprintf(line, "-ltty%.2dh", portNumber-1);
	} else	sprintf(line, "-l%s", port);
	action = p->item_index;
	switch (p->item_index) {
	case DialApply:
		ClearFooter(df->footer); /* clear mesaage area */
		SetPopupMessage(qd->dialPrompt, "");

		ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
		fp = (Setting *)QueryGizmo (PopupGizmoClass,
				popup,
				GetGizmoSetting,
				"phone");
		qd->sp = (Setting *)QueryGizmo (PopupGizmoClass, popup,
				GetGizmoSetting,
				"speed");
		os = (Setting *)QueryGizmo (PopupGizmoClass, popup,
				GetGizmoSetting,
				"otherSpeed");

		pp = (Setting *)QueryGizmo (PopupGizmoClass, popup,
				GetGizmoSetting,
				"parity");
		cp = (Setting *)QueryGizmo (PopupGizmoClass, popup,
				GetGizmoSetting,
				"charSize");
		dp = (Setting *)QueryGizmo (PopupGizmoClass, popup,
				GetGizmoSetting,
				"duplex");
		status_W = (Widget )QueryGizmo (PopupGizmoClass, popup,
				GetGizmoWidget,
				"showStatusMenu");
		XtSetArg(args[0], XtNset, &showStatusWindow);
		OlFlatGetValues(status_W, 0, args, 1); 
#ifdef DEBUG
	fprintf(stderr,"current status window value=%d\n",showStatusWindow);
	fprintf(stderr,"current values: phone=%s speed=%d otherspeed=%s parity=%d charSize=%d duplex=%d\n", fp->current_value, qd->sp->current_value, os->current_value, pp->current_value, cp->current_value, dp->current_value);
#endif
	
         /*
          * data check
          */

		n = (int) qd->sp->current_value;
		if ((strcmp(SpeedItems[n].label, label_other)) == 0)  {
				/* do check for other speeds, since blank
				is invalid and 14400 and 28800 get reset to other
				values */
				if ((VerifyOtherSpeed(os->current_value)) == INVALID) return;
		}
		/* if the connection is acu, check the phone number */
		if (strcmp(type, "datakit") != 0 
			&& strcmp(type, "uudirect") != 0) {
			if (strlen(fp->current_value) == 0) {
			    DeviceNotifyUser(df->toplevel, GGT(string_badPhone));
			    return;
			}
			else

			if ( strlen(fp->current_value) !=
			     strspn(fp->current_value, "0123456789=-*#") ) {
			    /* it's not a legimtimate telno */
#ifdef DEBUG
			fprintf(stderr,"bad phone number: fp->current_value=%s\n",fp->current_value);
#endif
			    
			    DeviceNotifyUser(df->toplevel, GGT(string_badPhone));
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
				sprintf(cupath, "%s%s%s", x, "/desktop/rft/", CU);
				sprintf(path, "%s%s%s", x, "/bin/", XTERM);
			} else {
				sprintf(cupath, "%s%s", "/usr/X/desktop/rft/", CU);
				sprintf(path, "%s%s", "/usr/X/bin/", XTERM);
			}
			program.exec_argv[0] = strdup(path);
		}

		for (i = 2; i < 14; i++) {
			if (program.exec_argv[i] != NULL) {
				free (program.exec_argv[i]);
				program.exec_argv[i] = NULL;
			}
		}


		i=2;
		program.exec_argv[i++] =  strdup(GGT(cu_title));
		program.exec_argv[i++] =  strdup("-E ");
		program.exec_argv[i++] = strdup(cupath);
		if (showStatusWindow == True)  {
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
				sprintf(speed, "%s%s", "-s", os->current_value);
				program.exec_argv[i++] = strdup(speed);
			}
		} else 
		/* check if speed is not other or any then set up
			the speed to use */
		if ((strcmp(SpeedItems[n].label, label_any)) != 0)  {
			sprintf(speed, "-s%s", Speeds[n]);
			program.exec_argv[i++] = strdup(speed);
		}
		n = (int)cp->current_value;
		if (n) {
			program.exec_argv[i++] = strdup("-b8");
		} else {
			program.exec_argv[i++] = strdup("-b7");
		}	
		if((int)dp->current_value)
			program.exec_argv[i++] = strdup("-h");
		n = (int)pp->current_value;
		if (n == 2)  {
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
		fprintf(stderr,"Command line = ");
		for (j=0; j < 14; j++) 
			fprintf(stderr, "%s ",program.exec_argv[j]);
#endif
		if(ExecProgram(&program) == -1) {
			DeviceNotifyUser(df->toplevel, GGT(string_forkFail));
		}
		else {
			BringDownPopup(qd->popup);
		}
		ManipulateGizmo(PopupGizmoClass, popup, ApplyGizmoValue);
		break;
	case DialReset:
		ManipulateGizmo(PopupGizmoClass, popup, ResetGizmoValue);
		SpeedChangedCB(w, 0, &call_data);
		break;
	case DialCancel:
		BringDownPopup(qd->popup);
		break;
	default:
		(void)fprintf(stderr,"at %d in %s\n", __LINE__, __FILE__);
		break;
	}

callRegisterHelp(df->toplevel, title_setup, help_device);

} /* DialCB */



static void
RestartQD()
{
#ifdef DEBUG
	fprintf(stderr,"RestartQD program.exit_code=%d\n",program.exit_code);
#endif
	
	ClearFooter(df->footer); /* clear mesaage area */
	/* probably need to sensitize the button */
	switch (program.exit_code) {
		case 0:
			SetPopupMessage(qd->dialPrompt, "");
			return;
		case 29:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_killedCU));
			break;
		case 39:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_badPhone));
			break;
		case 40:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_usageCU));
			break;
		case 41:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_exceed58));
			break;
		case 43:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_connectFail));
			break;
		case 45:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_lostConnect));
			break;
		case 47:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_lostCarrier));
			break;
		case 49:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_dialFail));
			break;
		case 50:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_scriptFail));
			break;
		case 51:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_dialFail));
			break;
		case 52:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_noDevice));
			break;
		case 53:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_noSystem));
			break;
		default:
			XtPopup(df->toplevel, XtGrabNone);
			DeviceNotifyUser(df->toplevel, GGT(string_unknownFail));
			break;
		}
		/*XtPopup(df->toplevel, XtGrabNone);*/
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

#ifdef DEBUG
	fprintf(stderr,"dial: ReapChild\n");
#endif

	if((pid = wait(&status)) == -1)
		return;
	if(program.pid == pid) {
		program.exit_code = WEXITSTATUS(status);
#ifdef DEBUG
	fprintf(stderr,"dial: ReapChild program.exit_code=%d\n",program.exit_code);
#endif
		RestartQD();
	}
#ifdef DEBUG
	fprintf(stderr, "Pid #%d???\n", pid);
#endif
} /* ReapChild */


static
void SpeedChangedCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
	Widget 	dialOtherSpeed;
	Widget speed_widget;
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
#ifdef DEBUG
	fprintf(stderr,"SpeedChangedCB n=%d\n",n);
#endif
	if ((strcmp(SpeedItems[n].label, label_b14400)) == 0) {
		DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
		OlVaFlatSetValues(speed_widget,
			Speed19200,
			XtNset,
			True,
			0);
		OlVaFlatSetValues(speed_widget,
			Speed19200,
			XtNfocusWidget,
			True,
			0);
		n =  Speed19200;

	} else
	if ((strcmp(SpeedItems[n].label, label_b28800)) == 0) {
		DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
		OlVaFlatSetValues(speed_widget,
			Speed38400,
			XtNset,
			True,
			0);

		n =  Speed38400;
	}
	if ((strcmp(SpeedItems[n].label, label_other)) == 0)  {
				/* do other speed stuff */
#ifdef DEBUG
	fprintf(stderr,"SpeedChangedCB other devSpeed=%s\n",devSpeed);
#endif
		
                if (qd->speedMapped == False) {
			XtMapWidget(dialOtherSpeed);
			qd->speedMapped = True;
			}
		if ((action == DialReset)  && (speedFound == False)) {
			XtVaSetValues(qd->os_p->textFieldWidget, XtNstring, devSpeed, 0);
		}
		AcceptFocus(dialOtherSpeed);
  
	}
	else {

	if (qd->speedMapped == True)  {
		XtUnmapWidget(dialOtherSpeed);
		qd->speedMapped = False;
		}
	}
}

static void
GetCurrentSpeed()
{
	int i, speed_cnt;
	Widget dialOtherSpeed;
	
	dialOtherSpeed = qd->os_p->captionWidget;
	devSpeed = ((DeviceData*)(df->select_op->objectdata))->portSpeed;
	qd->sp = (Setting *)QueryGizmo (PopupGizmoClass, qd->dialPrompt,
			GetGizmoSetting,
			"speed");
	speed_cnt = XtNumber(Speeds);
#ifdef DEBUG
	fprintf(stderr,"speed_cnt=%d\n",speed_cnt);
#endif
	for (i=0,speedFound=False; i < speed_cnt; i++) {
		if (strcmp(Speeds[i], devSpeed) == 0) {
			speedFound = True;
		/* need to set initial speed value in radio boxes */
			qd->sp->initial_value = (XtPointer)i;
			qd->sp->previous_value = (XtPointer)i;
			ManipulateGizmo(PopupGizmoClass, qd->dialPrompt, ResetGizmoValue);
			break;
		}
	}

#ifdef DEBUG
	fprintf(stderr,"speedFound=%d\n",speedFound);
#endif
	if (speedFound == False) {
		/* need to set value in textfield and manage the other speed
		text field */
		qd->sp->initial_value = (XtPointer) SpeedOther;
		qd->sp->previous_value = (XtPointer) SpeedOther;
		ManipulateGizmo(PopupGizmoClass, qd->dialPrompt, ResetGizmoValue);
		
#ifdef DEBUG
		fprintf(stderr,"other speed found set textfield widget to %s\n",devSpeed);
#endif
		XtVaSetValues(qd->os_p->textFieldWidget, XtNstring, devSpeed, 0);
		
		if (qd->speedMapped == False) {
			XtMapWidget(dialOtherSpeed);
			qd->speedMapped = True;
		}
	 } else {
		if (qd->speedMapped == True) {
			XtUnmapWidget(dialOtherSpeed);
			qd->speedMapped = False;
		}
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
		DeviceNotifyUser(df->toplevel, GGT(string_blankSpeed));
		return INVALID;
	} else
	if (strcmp(speed, "") == 0) {
		DeviceNotifyUser(df->toplevel, GGT(string_blankSpeed));
		return INVALID;
	} else
	if ((strchr(speed, ' ') != NULL)) {
		/* spaces not allowed in string */
		DeviceNotifyUser(df->toplevel, GGT(string_badExpect));
		return INVALID;
	}
	if ((strcmp(speed, "14400") != 0) &&
		(strcmp(speed, "28800") != 0)) return VALID;



	/*check for 14400 */
	if ((strcmp(speed, "14400") == 0)) {
		DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
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
		DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
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

