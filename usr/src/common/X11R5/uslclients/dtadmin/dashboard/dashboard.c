#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dashboard/dashboard.c	1.43.1.13"
#endif
/*
 *	UNIX Desktop Dashboard -- configurable system display
 *
 *	default components include user name, clock and calendar
 *	node name, and a list of machine resources (memory, peripherals)
 */

#include <stdio.h>

#include <locale.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <sys/utsname.h>
#include <time.h>
#include <pwd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/systeminfo.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/PopupMenu.h>
#include <Xol/RubberTile.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/StepField.h>
#include <Xol/IntegerFie.h>
#include <Xol/Form.h>
#include <Xol/Caption.h>
#include <Xol/Gauge.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrolledWi.h>
#include <Xol/Footer.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/NumericGiz.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/ChoiceGizm.h>

#include <Gizmo/ModalGizmo.h>

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>

#include "../dtamlib/dtamlib.h"
#include "dashmsgs.h"
#include "tz.h"

#define	ICON_NAME	"dash48.icon"

void	exitCB();
void	helpCB();
void	applyCB();
void	defaultCB();
void	resetCB();
void	cancelCB();
void	PropPopup();
extern void     ErrorNotice (Widget, char *);

Boolean         owner;
Boolean         validation_ok;
static  char    tst[] = "/sbin/tfadmin -t date 2>/dev/null";
void    Create_tz (); 
char *  gettz (); 
void    SetTime (); 
void    Reset_Time (); 
void    MoveFocus();
extern Widget current_selection;

static MenuItems action_menu_item[] = {
	{ TRUE, label_prop,  mnemonic_prop, 0, PropPopup},
	{ TRUE, label_exit,  mnemonic_exit2, 0, exitCB},
	{ NULL }
};

static HelpInfo HelpIntro	= { 0, "", HELP_PATH, help_intro };
static HelpInfo HelpProps	= { 0, "", HELP_PATH, help_props };
static HelpInfo HelpTOC		= { 0, "", HELP_PATH, "TOC" };
static HelpInfo HelpDesk       = { 0, "", HELP_PATH, "HelpDesk"  };

static MenuItems help_menu_item[] = {  
	{ TRUE, label_intro, mnemonic_intro, 0, helpCB, (char *)&HelpIntro },
	{ TRUE, label_toc,   mnemonic_toc, 0, helpCB, (char *)&HelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk, 0, helpCB, (char *)&HelpDesk },
	{ NULL }
};

static MenuGizmo action_menu = {0, "action_menu", NULL, action_menu_item};
static MenuGizmo help_menu = {0, "help_menu", NULL, help_menu_item};

static MenuItems main_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &action_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &help_menu},
	{ NULL }
};
static MenuGizmo menu_bar = { 0, "menu_bar", NULL, main_menu_item, NULL, NULL,
	CMD, OL_FIXEDROWS, 1, OL_NO_ITEM };

BaseWindowGizmo base = {0, "base", string_appName, (Gizmo)&menu_bar,
	NULL, 0, string_iconName, ICON_NAME, " ", " ", 90 };

static MenuItems prop_menu_item[] = {  
	{ TRUE, label_ok,  mnemonic_ok, 0, applyCB,    NULL },
	{ TRUE, label_default,mnemonic_default, 0, defaultCB,  NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpProps },
	{ NULL }
};
static MenuGizmo prop_menu = {0, "properties", NULL, prop_menu_item };

static PopupGizmo prop_popup = {0,"popup",string_propTitle,(Gizmo)&prop_menu };

/* Warning dialog for date changes */
static Boolean	ClockReset(void);
static void	DateChgsCB(Widget, XtPointer, XtPointer);
static MenuItems	date_chgs_menu_items[] = {
	{ True, label_ok, mnemonic_ok },
	{ NULL }
};
static MenuGizmo	date_chgs_menu = {
	NULL, "date_chgs_bar", "date_chgs_bar", date_chgs_menu_items, DateChgsCB
};
static ModalGizmo	date_chgs_notice = {
	NULL, "date_chgs_notice", string_warningTitle, &date_chgs_menu,
	string_date_chgs
};

/* Application resources... */
typedef struct {
	int	date_chg_threshold;	/* dateChgThreshold, UOM=minute.
					 * This application resource controls
					 * whether the date_chgs_notice should
					 * be popped or not when people make
					 * changes to year, month, date, hour,
					 * and/or minute, default is 30 minutes.
					 *
					 * It's recommended that people shall
					 * restart a session (i.e., kill dtm)
					 * whenever s/he made changes in those
					 * fields, otherwise, Xt timers will
					 * not behave properly...
					 */
} RscTable;

/* Currently, only this file references this variable, if this is
 * no longer True in the future, then RscTable shall be moved into
 * a header, and rsc_table shall be defined as an extern... */
static RscTable	rsc_table;
static XtResource	my_resources[] = {
		/* UOM = minute */
	{ "dateChgThreshold", "DateChgThreshold", XtRInt, sizeof(int),
	  XtOffsetOf(RscTable, date_chg_threshold),
	  XtRImmediate, (XtPointer)30 },
};
#define DateChgThreshold	rsc_table.date_chg_threshold

#define	BI_LENGTH	7
#define PRTCONF		"/usr/sbin/prtconf"
#define	MOUNT		"/sbin/mount"
#define DATECMD0        "/usr/bin/date"
#define	DATECMD1	"/sbin/tfadmin date"

extern	char	*GetXWINHome();
extern	char	*getenv();

char		*ProgramName;
Widget		w_toplevel, w_dash, w_message, w_lefty, w_righty;
Widget		w_tick, w_dskup;
Widget		w_leftx = (Widget)NULL;
Widget		w_clock = (Widget)NULL;
Widget		w_cal   = (Widget)NULL;
Widget		w_popup = (Widget)NULL;
Widget          w_tz = (Widget)NULL;
Widget		w_numf[5];
Display		*theDisplay;
Arg		arg[16];

#define	FooterMsg(txt)	SetBaseWindowMessage(&base,txt)

char		*dashdir = "desktop/dashboard";
char		*dashfile = "StatusFile";
int		clock_tick = 60;
int		new_tick;
int		old_tick;
int		disk_update = 0;
int             tz_update = 0;
int		new_update;
int		old_update;
Boolean		clock_update = FALSE;
Dimension	inch;

char		**comp_list;
char		*std_comp[] = {	"OS version\t+\tbuiltin\n",
				"node\t+\tbuiltin\n",
				"login\t+\tbuiltin\n",
				"date\t+\tbuiltin\n",
				"clock\t+60\tbuiltin\n",
				"memory\t+r\tbuiltin\n",
				"math\t+r\tbuiltin\n",
				"diskette\t+r\tbuiltin\n",
				"ctape\t+r\tbuiltin\n",
				"disk\t+60r\tbuiltin\n",
				"tz\t+\tbuiltin\n",
				"" };

typedef	struct	{ char	*  label;
		  XtArgVal mnem;
		  XtArgVal selCB;
		} Items; /* for flat buttons */

char	*Fields[]   = { XtNlabel, XtNmnemonic, XtNselectProc };
#define	NUM_Fields	3

#define	BUILT_IN	"builtin"
#define	DASH_WIDTH	(7.85*(int)inch)

char	date_buf[40] = "01/01/70";
time_t	current_time;
struct	tm	*now;
struct	tm	*old_time;

/*
	*	these strings are actually internationalized, by being replaced
	*	using the first 12 lines of /usr/lib/locale/<locale>/LC_TIME
	*/
char	*months[12] = {	"Jan","Feb","Mar","Apr","May","Jun",
			"Jul","Aug","Sep","Oct","Nov","Dec"};

void
MoveFocus(Widget w)
{
    Time time;

    time = XtLastTimestampProcessed(XtDisplay(w));
    if (OlCanAcceptFocus(w, time))
        XtCallAcceptFocus(w, &time);
}

void
exitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	exit(0);
}


void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;
	static String help_app_title;

	if (help_app_title == NULL)
		help_app_title = GetGizmoText(string_appName);

	help->app_title = help_app_title;
	help->title = string_appName;
	help->section = GetGizmoText(help->section);
	PostGizmoHelp(base.shell, help);
}

char	*SetTFitoa(Widget wid, int n)
{
static	char	txtbuf[16];

	sprintf(txtbuf, "%02d", n);
	XtSetArg(arg[0], XtNstring, txtbuf);
	XtSetValues(wid, arg, 1);
	return txtbuf;
}

int	SetCycle(Widget wid, int delta, int range)
{
	char	*val, buf[8];

	XtSetArg(arg[0], XtNstring, &val);
	XtGetValues(wid, arg, 1);
	delta += atoi(val);
	if (delta == range)
		delta = 0;
	else if (delta == -1)
		delta = range - 1;
	SetTFitoa(wid, delta);
	return delta;
}

void
DateChanged(Widget wid, XtPointer client_data, XtPointer call_data)
{
	clock_update = TRUE;
}

void
StepIt(Widget wid, int cycle, OlTextFieldStepped *stp)
{
	int	i, n;
	char	*val;

	switch(stp->reason) {
		default:			return;
		case OlSteppedIncrement:	n = 1;	break;
		case OlSteppedDecrement:	n = -1;	break;
	}
	clock_update = TRUE;
	switch (cycle) {
		default:	SetCycle(wid, n, cycle);
				break;
		case 32:	if (SetCycle(wid, n, cycle) == 0)
					SetCycle(wid, n, cycle);
				break;
				/* probably should special case months */
		case 12:	XtSetArg(arg[0], XtNstring, &val);
				XtGetValues(wid, arg, 1);
				for (i = 0; i < 12; i++)
					if (strcmp(months[i],val)==0)
						break;
				if (i == 12) {
					i = atoi(val);
					if (i < 0) 	 i = 0;
					else if (i > 11) i = 11;
				}
				i += n;
				if (i < 0)	 i = 11;
				else if (i > 11) i = 0;
				XtSetArg(arg[0], XtNstring, months[i]);
				XtSetValues(wid, arg, 1);
				break;
	}
}

void	QueryProperties()
{
	char	*str;
	int	n;

        validation_ok = TRUE;
	XtSetArg(arg[0], XtNstring, &str);
	XtGetValues(w_tick, arg, 1);
	for (n = 0; str[n]; n++)
		if (!isdigit(str[n])  && !isspace(str[n])) {
                        validation_ok = FALSE;
                        ErrorNotice (base.shell, GetGizmoText(string_badTick));
			str = SetTFitoa(w_tick, clock_tick);
			break;
		}
	new_tick = atoi(str);
	XtSetArg(arg[0], XtNstring, &str);
	XtGetValues(w_dskup, arg, 1);
	for (n = 0; str[n]; n++)
		if (!isdigit(str[n])  && !isspace(str[n])) {
                        validation_ok = FALSE;
                        ErrorNotice (base.shell, GetGizmoText(string_badTick));
			str = SetTFitoa(w_dskup, disk_update);
			break;
		}
	new_update = atoi(str);
}

void	ResetDate()
{
	char	cur_date[40];
        char    buf[BUFSIZ];
       
	time(&current_time);
	cftime(cur_date, "%x", &current_time);
	if (strcmp(cur_date, date_buf)) {
		strcpy(date_buf, cur_date);
            sprintf(buf, "%s %s", GetGizmoText(tag_date), date_buf);
		XtSetArg(arg[0], XtNstring, buf);
		XtSetValues(w_cal, arg, 1);
	}
}

void	CheckDate(wid, interval)
	Widget		wid;
	XtIntervalId	interval;
{
	ResetDate();
	XtAddTimeOut(1000*(60 - current_time % 60), 
			(XtTimerCallbackProc)CheckDate, (XtPointer)wid);
}

XtIntervalId	time_check;

void
CheckTime(Widget wid, XtIntervalId interval)
{
	char	time_buf[40];
        char    buf[BUFSIZ];

	time(&current_time);
	cftime(time_buf, "%X", &current_time);
        sprintf(buf, "%s %s", GetGizmoText(tag_clock), time_buf);
	XtSetArg(arg[0], XtNstring, buf);
	XtSetValues(wid, arg, 1);
	if (clock_tick) {
		time_check = XtAddTimeOut(
			1000 * (clock_tick - current_time % clock_tick),
			(XtTimerCallbackProc)CheckTime, (XtPointer)w_clock);
	}
}

void	UpdateComponent(char *name, int value)
{
	int	n;
	char	*ptr, *ptr2, *new;

	for (n = 0, ptr = comp_list[0]; *ptr;  ptr = comp_list[++n])
		if (strncmp(ptr, name, strlen(name))==0)
			break;
	if (*ptr == '\0')
		return;
	if ((ptr2 = new=(char *)MALLOC(strlen(ptr)+4))== NULL) {
		return;
	}
	while (!isdigit(*ptr))
		*ptr2++ = *ptr++;
	while (isdigit(*ptr++))
		;
	sprintf(ptr2, "%d%s", value, ptr-1);
	if (comp_list != std_comp)
		FREE(comp_list[n]);
	comp_list[n] = new;
}

static void
DateChgsCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(date_chgs_notice.shell);
	XtPopdown(w_popup);
}

void
applyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
static	char	*msg = NULL;
uid_t           myid;
Boolean		additional_warning = False;

	QueryProperties();
	if (clock_update)
		additional_warning = ClockReset();
	if (clock_tick != new_tick && w_clock) {
		UpdateComponent("clock\t", new_tick);
		clock_tick = new_tick;
		if (w_clock) {
			XtRemoveTimeOut(time_check);
			time_check = XtAddTimeOut(100,
				(XtTimerCallbackProc)CheckTime,
							(XtPointer)w_clock);
		}
	}
	if (disk_update != new_update)
		UpdateComponent("disk\t", new_update);
	disk_update = new_update;

        /* Added to Popdown Property window after apply */

        if (tz_update) {
                SetTime (w_toplevel);
                tz_update = 0;
        }

        if (validation_ok == TRUE){
             if (!msg)
                      msg = GetGizmoText(string_propOK);
             SetPopupMessage(&prop_popup, msg);

             if (w_numf[0])
                 MoveFocus(w_numf[0]);
             else if (w_tick != NULL)
                 MoveFocus(w_tick);

#if 0
             XtPopdown((Widget) _OlGetShellOfWidget(wid));
#else
	     if (!additional_warning)
		     XtPopdown(w_popup);
	     else
		     MapGizmo(ModalGizmoClass, (Gizmo)&date_chgs_notice);
#endif
        }

}

void
defaultCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	QueryProperties();
	WriteDashrc();
}

void
resetCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
static	char	*msg = NULL; 

	if (clock_update) {
		*now = *old_time;
		UpdateTimeFields(w_numf);
		clock_update = FALSE;
	}
	SetTFitoa(w_tick, new_tick=clock_tick=old_tick);
	SetTFitoa(w_dskup, disk_update = new_update = old_update);
	if (!msg)
		msg = GetGizmoText(string_resetOK);
        if (owner && system(tst) == 0)
               Reset_Time ();
	SetPopupMessage(&prop_popup, msg);
}

void
cancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (w_numf[0])
             MoveFocus(w_numf[0]);
        else if (w_tick != NULL)
             MoveFocus(w_tick);

	BringDownPopup(w_popup);
	SetPopupMessage(&prop_popup, NULL);
}

static Boolean
ClockReset(void)
{
	int		i, n[5];	/* yr, mon, day, hr, min */
	FILE		*cmdfp[2];
	char		*val, *msg, buf[80];
	Boolean		ret_val;
	int		delta_chgs = 0;

	XtSetArg(arg[0], XtNvalue, &n[0]);
	XtGetValues(w_numf[0], arg, 1);
	for (i = 1; i < 5; i++) {
		XtSetArg(arg[0], XtNstring, &val);
		XtGetValues(w_numf[i],arg, 1);
		if (i > 1)
			n[i] = atoi(val);
		else {
			int	j;

			for (j = 0; j < 12; j++)
				if (strcmp(val, months[j])==0)
					break;
			n[1] = j+1;
		}
	}

		/* Figure out delta_chgs in minutes, also assume that
		 * 1 year  == 365 days,
		 * 1 month == 30 days
		 *
		 * Also note that yr -> 1900 + now->tm_year,
		 *		  month -> now->tm_mon + 1.
		 */
	if (i = n[0] - now->tm_year - 1900)		/* year */
		delta_chgs = i * 365 * 24 * 60;

	if (i = n[1] - now->tm_mon - 1)			/* month */
		delta_chgs += (i * 30 * 24 * 60);

	if (i = n[2] - now->tm_mday)			/* day */
		delta_chgs += (i * 24 * 60);

	if (i = n[3] - now->tm_hour)			/* hour */
		delta_chgs += (i * 60);

	if (i = n[4] - now->tm_min)			/* minute */
		delta_chgs += i;

	if (delta_chgs == 0)
		return(False);
	else if (delta_chgs < 0)
		delta_chgs = -delta_chgs;

	ret_val = delta_chgs >= DateChgThreshold;

        setuid (0);
        sprintf(buf, "%s %02d%02d%02d%02d%02d", (system(tst) == 0)? 
                        DATECMD1: DATECMD0, n[1], n[2], n[3], n[4], n[0]);
	if (_Dtam_p3open(buf, cmdfp, FALSE) != -1) {
		fgets(buf, sizeof(buf), cmdfp[1]);
		_Dtam_p3close(cmdfp, 0);
		XtRemoveTimeOut(time_check);
		time_check = XtAddTimeOut(100,
					(XtTimerCallbackProc)CheckTime,
							(XtPointer)w_clock);
		SetPopupMessage(&prop_popup, buf);
	}
	else	
		SetPopupMessage(&prop_popup,GetGizmoText(string_cantReset));
	ResetDate();
	clock_update = FALSE;

	return ret_val;
}

UpdateTimeFields(Widget	*w)
{
	XtSetArg(arg[0], XtNvalue, 1900+now->tm_year);
	XtSetValues(w[0], arg, 1);
	XtSetArg(arg[0], XtNstring, months[now->tm_mon]);
	XtSetValues(w[1], arg, 1);
	SetTFitoa(w[2], now->tm_mday);
	SetTFitoa(w[3], now->tm_hour);
	SetTFitoa(w[4], now->tm_min);
}

char	*ExclFields[] = {XtNlabel, XtNmnemonic, XtNset };

typedef	struct	{ char		*label;
		  XtArgVal	mnem;
		  Boolean	setting;
} ExclItem;

ExclItem	Citems[2];

CreateStandardOptions(parent)
	Widget	parent;
{
	Widget		w_ctl, w_cap, w_tmp;
	char		*suffix, txtbuf[24];
        char            ptr[256];

	XtSetArg(arg[0], XtNlayoutType, 	OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNmeasure, 		2);
	XtSetArg(arg[2], XtNalignCaptions,	TRUE);
	XtSetArg(arg[3], XtNshadowThickness,	0);
	w_ctl = XtCreateManagedWidget("setclock", controlAreaWidgetClass,
			parent, arg, 4);

	XtSetArg(arg[0], XtNposition,	OL_LEFT);
	XtSetArg(arg[1], XtNalignment,	OL_TOP);
	XtSetArg(arg[2], XtNlabel,	GetGizmoText(label_tick));
	w_cap = XtCreateManagedWidget("clocktick", captionWidgetClass,
			w_ctl, arg,  3);

	suffix = GetGizmoText(label_second);
	sprintf(txtbuf, "%d", clock_tick); 

	XtSetArg(arg[0], XtNcharsVisible, 6);
	XtSetArg(arg[1], XtNstring,	  txtbuf);
	w_tick = XtCreateManagedWidget("tick", textFieldWidgetClass,
			w_cap, arg, 2);

	XtSetArg(arg[0], XtNstring, suffix);
	XtCreateManagedWidget("text", staticTextWidgetClass,
			w_ctl, arg, 1);
	/*
	 *	disk gauge update interval
	 */
	XtSetArg(arg[0], XtNposition,	OL_LEFT);
	XtSetArg(arg[1], XtNalignment,	OL_TOP);
	XtSetArg(arg[2], XtNlabel,	GetGizmoText(label_gauge));
	w_cap = XtCreateManagedWidget("diskgauge", captionWidgetClass,
			w_ctl, arg, 3);

	sprintf(txtbuf, "%d", disk_update);
	XtSetArg(arg[0], XtNstring, 	  txtbuf);
	XtSetArg(arg[1], XtNcharsVisible, 6);
	w_dskup = XtCreateManagedWidget("gauge",textFieldWidgetClass,
			w_cap, arg, 2);

	XtSetArg(arg[0], XtNstring, suffix);
	XtCreateManagedWidget("text", staticTextWidgetClass,
			w_ctl, arg, 1);

        if (owner && system(tst) == 0)
                if (gettz(ptr) != NULL)
                        Create_tz (parent);
}

Widget	w_time = (Widget)NULL;

void
PropPopup(Widget wid, XtPointer client_data, XtPointer call_data)
{
        char    ptr[256];

	if (w_time) {
		time(&current_time);
		old_time = now = localtime(&current_time);
		UpdateTimeFields(w_numf);
	}

        if (w_tz) {
               if (owner && system(tst) == 0)
                if (gettz (ptr) != NULL)
                        XtVaSetValues (current_selection, XtNstring,
                                        (XtArgVal) ptr, 0);
        }

	SetTFitoa(w_dskup, new_update = old_update = disk_update);
	clock_update = FALSE;
	SetTFitoa(w_tick, new_tick = old_tick = clock_tick);
	FooterMsg(NULL);
	SetPopupMessage(&prop_popup, NULL);
	MapGizmo(PopupGizmoClass, &prop_popup);
}

void	CreatePropSheet()
{
static	Items	items[4];
	Widget	w_up, w_low, w_cap;

	CreateGizmo(w_toplevel, PopupGizmoClass, &prop_popup, NULL, 0);
	w_popup = GetPopupGizmoShell(&prop_popup);
	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(w_popup, arg, 1);

	XtSetArg(arg[0], XtNlayoutType,	OL_FIXEDCOLS);
	XtSetArg(arg[1], XtNvSpace,	(inch/6));
	XtSetArg(arg[2], XtNvPad,	(inch/6));
	XtSetArg(arg[3], XtNhSpace,	(inch/6));
	XtSetArg(arg[4], XtNhPad,	(inch/6));
	XtSetValues(w_up, arg, 5);

        if (owner && system(tst) == 0){
		time(&current_time);
		now = localtime(&current_time);

		XtSetArg(arg[0], XtNalignCaptions,	TRUE);
		XtSetArg(arg[1], XtNlayoutType, 	OL_FIXEDCOLS);
		XtSetArg(arg[2], XtNmeasure, 		3);
		XtSetArg(arg[3], XtNvSpace,		4);
		XtSetArg(arg[4], XtNvPad,		4);
		XtSetArg(arg[5], XtNhSpace,		(inch/6));
		XtSetArg(arg[6], XtNhPad,		(inch/6));
		XtSetArg(arg[7], XtNshadowThickness,	0);

		w_time = XtCreateManagedWidget("setclock",
			controlAreaWidgetClass, w_up, arg, 8);

		XtSetArg(arg[0], XtNlabel, GetGizmoText(label_year));
		w_cap =  XtCreateManagedWidget("yearcap", captionWidgetClass,
			w_time, arg, 1);

		XtSetArg(arg[0], XtNvalueMin,	1970);
		XtSetArg(arg[1], XtNvalueMax,	2220);
		XtSetArg(arg[2], XtNcharsVisible, 5);
		XtSetArg(arg[3], XtNvalue,	(1900+now->tm_year));
		w_numf[0] = XtCreateManagedWidget("year",
			integerFieldWidgetClass, w_cap, arg, 4);

		XtAddCallback(w_numf[0], XtNpostModifyNotification,
				(XtCallbackProc)DateChanged, (XtPointer)NULL);

		XtSetArg(arg[0], XtNlabel, GetGizmoText(label_month));
		w_cap = XtCreateManagedWidget("monthcap", captionWidgetClass,
			w_time, arg, 1);

		XtSetArg(arg[0], XtNstring,	months[now->tm_mon]);
		XtSetArg(arg[1], XtNcharsVisible, 5);
		w_numf[1] = XtCreateManagedWidget("month",
			stepFieldWidgetClass, w_cap, arg, 2);

		XtAddCallback(w_numf[1], XtNpostModifyNotification,
				(XtCallbackProc)DateChanged, (XtPointer)NULL);

		XtAddCallback(w_numf[1], XtNstepped, (XtCallbackProc)StepIt,
								(XtPointer)12);

		XtSetArg(arg[0], XtNlabel, GetGizmoText(label_day));
		w_cap = XtCreateManagedWidget("daycap", captionWidgetClass,
			w_time, arg, 1);

		XtSetArg(arg[0], XtNcharsVisible, 5);
		w_numf[2] = XtCreateManagedWidget("day",
			stepFieldWidgetClass, w_cap, arg, 1);
		SetTFitoa(w_numf[2], now->tm_mday);

		XtAddCallback(w_numf[2], XtNpostModifyNotification,
				(XtCallbackProc)DateChanged, (XtPointer)NULL);

		XtAddCallback(w_numf[2], XtNstepped, (XtCallbackProc)StepIt,
								(XtPointer)32);

		XtSetArg(arg[0], XtNlabel, GetGizmoText(label_hour));
		w_cap = XtCreateManagedWidget("hourcap", captionWidgetClass,
			w_time, arg, 1);

		XtSetArg(arg[0], XtNcharsVisible, 5);
		w_numf[3] = XtCreateManagedWidget("hour",
			stepFieldWidgetClass, w_cap, arg, 1);
		SetTFitoa(w_numf[3], now->tm_hour);

		XtAddCallback(w_numf[3], XtNpostModifyNotification,
				(XtCallbackProc)DateChanged, (XtPointer)NULL);

		XtAddCallback(w_numf[3], XtNstepped, (XtCallbackProc)StepIt,
								(XtPointer)24);

		XtSetArg(arg[0], XtNlabel, GetGizmoText(label_minute));
		w_cap = XtCreateManagedWidget("mincap", captionWidgetClass,
			w_time, arg, 1);

		XtSetArg(arg[0], XtNcharsVisible, 5);
		w_numf[4] = XtCreateManagedWidget("minute",
			stepFieldWidgetClass, w_cap, arg, 1);
		SetTFitoa(w_numf[4], now->tm_min);

		XtAddCallback(w_numf[4], XtNpostModifyNotification,
				(XtCallbackProc)DateChanged, (XtPointer)NULL);
		XtAddCallback(w_numf[4], XtNstepped, (XtCallbackProc)StepIt,
								(XtPointer)60);

		XtSetArg(arg[0], XtNnoticeType, OL_WARNING);
		CreateGizmo(w_popup, ModalGizmoClass,
				(Gizmo)&date_chgs_notice, arg, 1);
		XtSetArg(arg[0], XtNwrap, False);
		XtSetValues(date_chgs_notice.stext, arg, 1);
	}
	CreateStandardOptions(w_up);
}

void	NewComponent(line, flag)
	char	*line;
	Boolean	flag;
{
static	int	count = 0;
	int	n;
	char	*ptr;

	/*
	 *	flag (set if the system Dashfile is being read after a user's)
	 *	indicates that components already named in the list are skipped
	 */
	if (flag) {
		if ((ptr=strchr(line,'\t')) ==NULL)
			return;
		for (n = 0; n < count; n++) {
			if (strncmp(comp_list[n], line, 1+ptr-line)==0) {
				return;
			}
		}
	}
	if (count++)
		comp_list=(char **)REALLOC(comp_list,(1+ count)*sizeof(char *));
	else
		comp_list=(char **)MALLOC(2*sizeof(char *));
	comp_list[count]   = "";
	comp_list[count-1] = STRDUP(line);
}

void	CreateCompList()
{
	char	*ptr, buf[BUFSIZ];
	FILE	*dashrc = NULL;
	Boolean	user = FALSE;

	ptr = getenv("HOME");
	sprintf(buf, "%s/%s", ptr, dashfile);
	if (dashrc=fopen(realpath(buf,buf+strlen(buf)+1),"r"))
		user = TRUE;
	else {
		sprintf(buf, "%s/%s", dashdir, dashfile);
		dashrc = fopen(GetXWINHome(buf),"r");
	}
	if (dashrc) {
		while (fgets(buf, BUFSIZ, dashrc))
			NewComponent(buf, FALSE);
		fclose(dashrc);
	}
	if (user) {
		sprintf(buf, "%s/%s", dashdir, dashfile);
		if (dashrc = fopen(GetXWINHome(buf,ptr),"r")) {
			while (fgets(buf, BUFSIZ, dashrc))
				NewComponent(buf, TRUE);
			fclose(dashrc);
		}
	}
	if (!comp_list)
		comp_list = std_comp;
}

WriteDashrc()
{
	FILE	*dashrc, *newdash;
	char	*ptr, *line, *home, fname[PATH_MAX], buf[80];
	int	n;

	home = getenv("HOME");
	sprintf(fname, "%s/%s", strcmp(home,"/")? home: "", dashfile);
	if ((newdash=fopen(fname,"w"))==NULL) {
		sprintf(buf, "%s %s", GetGizmoText(string_cantWrite), fname);
                ErrorNotice (base.shell, buf);
	}
	else {
		/*
		 *	this needs a more systematic, table-driven form
		 *	for the extended properties output on all components
		 */
		for (n=0, line=comp_list[0]; *line;  line=comp_list[++n]) {
			if (strncmp(line,"clock\t",6) == 0 &&
						new_tick != clock_tick) {
				for (ptr = line; ptr < line+7; ptr++)
					fputc(*ptr, newdash);
				while (isdigit(*ptr))
					ptr++;
				fprintf(newdash,"%d%s",new_tick,ptr);
			}
			else if (strncmp(line,"disk\t",5) == 0 &&
				 		new_update != disk_update) {
				for (ptr = line; ptr < line+6; ptr++)
					fputc(*ptr, newdash);
				while (isdigit(*ptr))
					ptr++;
				fprintf(newdash,"%d%s",new_update,ptr);
			}
			else
				fputs(line, newdash);
		}
		fclose(newdash);

                if (validation_ok == TRUE){
		    sprintf(buf,"%s %s", GetGizmoText(string_dfltOK), fname);
		    SetPopupMessage(&prop_popup, buf);
                }
	}
}

Boolean	ShowClock();
Boolean	ShowDate();
Boolean	ShowLogin();
Boolean	ShowUsersLic();
Boolean	ShowProcsLic();
Boolean	ShowOS();
Boolean	ShowNode();
Boolean	ShowDiskette();
Boolean	ShowTape();
Boolean	ShowCdrom();
Boolean	ShowDisk();
Boolean	ShowMemory();
Boolean	ShowMath();
Boolean	ShowTZ();

typedef	Boolean	(*BooleanProc)();

struct	defs	{	char *		bi_label;
			BooleanProc	bi_display;
		} built_in[] = {
			{ "clock",	ShowClock },
			{ "date",	ShowDate },
			{ "login",	ShowLogin },
			{ "UsersLic",	ShowUsersLic },
			{ "ProcsLic",	ShowProcsLic },
			{ "node",	ShowNode },
			{ "OS version",	ShowOS },
			{ "memory",	ShowMemory },
			{ "math",	ShowMath },
			{ "diskette",	ShowDiskette },
			{ "ctape",	ShowTape },
			{ "cdrom",	ShowCdrom },
			{ "disk",	ShowDisk },
                        { "tz",         ShowTZ },
			{ (char *)NULL,	NULL }
		};

XtTimerCallbackProc
SystemTO(wid, interval)
	Widget		wid;
	XtIntervalId	interval;
{
	FILE	*cmdpipe;
	char	line[BUFSIZ];
	char	*cmdbuf, *ptr;
	int	time_out;

	XtSetArg(arg[0], XtNuserData, &cmdbuf);
	XtGetValues(wid, arg, 1);
	time_out = 1000*atoi(cmdbuf);
	for (ptr = cmdbuf; *ptr != '\t'; ptr++)
		;
	if ((cmdpipe = popen(ptr+1,"r")) == NULL)
                ErrorNotice (base.shell, GetGizmoText(string_cantUpdate));
	else {
		fgets(line, BUFSIZ, cmdpipe);
		ptr = line+strlen(line)-1;
		if (*ptr = '\n') *ptr = 0;
		XtSetArg(arg[0], XtNstring,  line);
		XtSetValues(wid, arg, 1);
		pclose(cmdpipe);
	}
	if (time_out)
		XtAddTimeOut(time_out, (XtTimerCallbackProc)SystemTO,
								(XtPointer)wid);
}

CreateComponent(char *line)
{
	Widget	CreateText();

	static	int	time_out = 100;
	Widget	w_txt;
	char	*dupline, *ptr, *name, *flag, *display, *gauge, *setup;
	char	msg[80];
	int	n, r, t;

/*	if ((ptr = strchr(line,'#')) != NULL && ptr[-1] != '\\')
/*		*ptr = 0;
*/	dupline = STRDUP(line);
	if ((name = strtok(dupline, "#\t\n")) == NULL) { /* comment line */
		FREE(dupline);
		return;
	}
	/*
	*	check first whether the component is turned off; note that
	*	this will ignore the syntax error of a missing flag field.
	*/
	if ((flag=strtok(NULL, "\t\n")) == NULL || *flag == '-') {
		FREE(dupline);
		return;
	}
	/*
		*	handle built in components, this time marking any errors.
		*/
	if ((display=strtok(NULL,"\t\n")) == NULL 
	||  strncmp(display, BUILT_IN, BI_LENGTH) == 0) {
		/*
	 	*	should be a builtin component.
	 	*/
		for (n = 0; built_in[n].bi_label != NULL; n++)
			if (strcmp(name, built_in[n].bi_label) == 0)
				break;
		if (built_in[n].bi_label == NULL
		||  built_in[n].bi_display == NULL
		|| !built_in[n].bi_display(name, flag)) {
			sprintf(msg, "%s%s", GetGizmoText(string_noInfo), name);
                        ErrorNotice (base.shell, msg);
		}
		FREE(dupline);
		return;
	}
	/*
	 *	handle external table entries.  For these, a display command
	 *	is supposed to yield a string (preferably a single line) on
	 *	stdout which is displayed in a simple static text widget.
	 *	Or, prefixed by GAUGE there may be two commands expected to
	 *	yield digit strings to be used to make a percent gauge.
	 *	[I'm not handling this elaboration yet].
	 *
	 *	This is packaged into a SystemTO timeout to update the dash
	 *	(done once at the start unless the flag argument gives an
	 *	update time as a digit string after the plus sign.)
	 */	
	t = atoi(flag+1);			/* some number of seconds. */
	if (ptr = (char *)MALLOC(strlen(flag)+strlen(display))) {
		sprintf(ptr, "%d\t%s", t, display);
		w_txt = CreateText(name, flag, ptr);
		XtAddTimeOut(time_out, (XtTimerCallbackProc)SystemTO,
							(XtPointer)w_txt);
		time_out += 100;
	}
	else {
		sprintf(msg,"%s%s", GetGizmoText(string_cantBuild), name);
                ErrorNotice (base.shell, msg);
	}
	FREE(dupline);
}

/*
	*	For non-builtin components, the string is actually a shell
	*	command to be executed on startup, and possibly periodically
	*	thereafter.  This is placed in XtNuserData for later access.
	*/
Widget	CreateText(name, flag, cmdstr)
	char	*name;
	char	*flag;
	char	*cmdstr;
{
	Widget		w_xref, w_yref, w_tmp;
	Boolean		right;

	if (right = (tolower(flag[strlen(flag)-1]) == 'r')) {
		w_yref = w_righty;
		if ((w_xref=w_leftx) == NULL)
			w_leftx = w_xref = w_lefty;
	}
	else {
		w_yref = w_lefty;
		w_xref = w_dash;
	}
	XtSetArg(arg[0], XtNxRefWidget,		w_xref);
	XtSetArg(arg[1], XtNxAddWidth,		right);
	XtSetArg(arg[2], XtNxOffset,		6);
	XtSetArg(arg[3], XtNxAttachOffset,	6);
	XtSetArg(arg[4], XtNxAttachRight,	right);
	XtSetArg(arg[5], XtNxVaryOffset,	right);
	XtSetArg(arg[6], XtNyRefWidget,		w_yref);
	XtSetArg(arg[7], XtNyAddHeight,		TRUE);
	XtSetArg(arg[8], XtNyOffset,		6);
	XtSetArg(arg[9], XtNyAttachOffset,	6);
	XtSetArg(arg[10],XtNalignment,		OL_LEFT);
	XtSetArg(arg[11],XtNgravity,		WestGravity);
	XtSetArg(arg[12],XtNstring,		cmdstr);
	XtSetArg(arg[13],XtNuserData,		cmdstr);
	w_tmp = XtCreateManagedWidget(name, staticTextWidgetClass, w_dash,
		arg, 14);
	if (right)
		w_righty = w_tmp;
	else {
		w_lefty = w_tmp;
		if (!w_leftx)
			w_leftx = w_tmp;
	}
	return w_tmp;
}

CreateGauge(name, flag, mntpt)
	char	*name, *flag, *mntpt;
{
	void	CheckGauge();
	Widget	w_tmp, w_cap, w_g, w_xref, w_yref;
	Boolean	right;
	int	n;
	char	*ptr, 	*label;

	if (right = (tolower(flag[strlen(flag)-1]) == 'r')) {
		w_yref = w_righty;
		if ((w_xref=w_leftx) == NULL)
			w_xref = w_leftx = w_lefty;
	}
	else {
		w_yref = w_lefty;
		w_xref = w_dash;
	}
	if (strcmp(mntpt,"/")==0)
		label = "/ (root)";
	else
		label = mntpt;

	XtSetArg(arg[0], XtNxRefWidget,		w_xref);
	XtSetArg(arg[1], XtNxAddWidth,		right);
	XtSetArg(arg[2], XtNxOffset,		6);
	XtSetArg(arg[3], XtNxAttachOffset,	6);
	XtSetArg(arg[4], XtNxAttachRight,	right);
	XtSetArg(arg[5], XtNxVaryOffset,		right);
	XtSetArg(arg[6], XtNyRefWidget,		w_yref);
	XtSetArg(arg[7], XtNyAddHeight,		TRUE);
	XtSetArg(arg[8], XtNyOffset,		6);
	XtSetArg(arg[9], XtNalignment,		OL_LEFT);
	XtSetArg(arg[10], XtNgravity,		WestGravity);
	XtSetArg(arg[11], XtNstring,		label);
	w_tmp = XtCreateManagedWidget(name, staticTextWidgetClass, w_dash,
					arg, 12);

	XtSetArg(arg[0], XtNxRefWidget,		w_tmp);
	XtSetArg(arg[1], XtNxAddWidth,		TRUE);
	XtSetArg(arg[2], XtNxOffset,		0);
	XtSetArg(arg[3], XtNxAttachOffset,	0);
	XtSetArg(arg[4], XtNyRefWidget,		w_tmp);
	XtSetArg(arg[5], XtNposition,		OL_TOP);
	XtSetArg(arg[6], XtNalignment,		OL_RIGHT);
	XtSetArg(arg[7], XtNlabel,		"??? Mbyte   ");
	w_cap = XtCreateManagedWidget("caption", captionWidgetClass, w_dash,
					arg, 8);

	XtSetArg(arg[0], XtNshowValue,		TRUE);	/* for Motif mode */
	XtSetArg(arg[1], XtNorientation,	OL_HORIZONTAL);
	XtSetArg(arg[2], XtNminLabel,		"0 %");
	XtSetArg(arg[3], XtNmaxLabel,		"100 %      ");
	XtSetArg(arg[4], XtNsliderMax,		100);
	XtSetArg(arg[5], XtNticks,		20);
	XtSetArg(arg[6], XtNtickUnit,		OL_PERCENT);
	XtSetArg(arg[7], XtNsliderValue,	0);
	w_g = XtCreateManagedWidget("gauge", gaugeWidgetClass, w_cap, arg, 8);

	XtSetArg(arg[0], XtNxRefWidget,		w_xref);
	XtSetArg(arg[1], XtNxAddWidth,		right);
	XtSetArg(arg[2], XtNxOffset,		6);
	XtSetArg(arg[3], XtNxAttachOffset,	6);
	XtSetArg(arg[4], XtNyRefWidget,		w_yref);
	XtSetArg(arg[5], XtNyAddHeight,		TRUE);
	XtSetArg(arg[6], XtNyOffset,		6);
	XtSetArg(arg[7], XtNheight,		(XtArgVal)(0.75*(int)inch));
	w_tmp = XtCreateManagedWidget("space", rectObjClass, w_dash, arg, 8);

	if (right)
		w_righty = w_tmp;
	else {
		w_lefty = w_tmp;
		if (!w_leftx)
			w_leftx = w_g;
	}
	new_update = disk_update = atoi(flag+1);
	XtSetArg(arg[0], XtNuserData, STRDUP(mntpt));
	XtSetValues(w_g, arg, 1);
	XtAddTimeOut(500, (XtTimerCallbackProc)CheckGauge, (XtPointer)w_g);
}

void
CheckGauge(wid, interval)
	Widget		wid;
	XtIntervalId	interval;
{
	char	*ptr, *lbl;
	int	Kbytes;
	double	Max;

	XtSetArg(arg[0], XtNuserData, &ptr);
	XtGetValues(wid, arg, 1);
	Kbytes = DiskUtil(wid, ptr);

	XtSetArg(arg[0], XtNlabel, (XtArgVal)&lbl);
	XtGetValues(XtParent(wid), arg, 1);

	Max = atoi(lbl);
	XtSetArg(arg[0], XtNsliderMax,	 Max);
	XtSetArg(arg[1], XtNsliderValue, Kbytes/1000);
	XtSetValues(wid, arg, 2);
	if (disk_update > 0)
		XtAddTimeOut((1000*disk_update),
			(XtTimerCallbackProc)CheckGauge, (XtPointer)wid);
}

int	DiskUtil(wid, device)	/* returns Kbytes used on device */
	Widget	wid;
	char	*device;
{
	FILE	*cmdpipe;
	char	cmdbuf[BUFSIZ];
	char	*ptr;
	int	n;

	sprintf(cmdbuf, "LC_ALL=C /sbin/dfspace %s", device);
	if ((cmdpipe = popen(cmdbuf,"r")) == NULL) {
                ErrorNotice (base.shell, GetGizmoText(string_cantGauge));
		return 0;
	}
	else {
		fgets(cmdbuf, BUFSIZ, cmdpipe);
		pclose(cmdpipe);
	/*
	 *	set parent caption to capacity in megabytes,
	 */
		ptr = strchr(cmdbuf,'(');
		if (!ptr)
			return 0;
		n = atoi(ptr+1);		/* this is the percent FREE */
		if (ptr = strstr(cmdbuf," of ")) {
			*(strchr(ptr,'M')) = 0;
			sprintf(cmdbuf," %s%s    ", ptr+4,
					GetGizmoText(tag_mbyte));
			XtSetArg(arg[0], XtNlabel, cmdbuf);
			XtSetValues(XtParent(wid), arg, 1);
		}
		return (100-n)*10*atof(cmdbuf);
	}
}

Boolean	ShowClock(name, flag)
	char	*name, *flag;
{
	char	time_buf[40];
        char    buf[BUFSIZ];

	time(&current_time);
	cftime(time_buf, "%X", &current_time);
        sprintf(buf, "%s %s", GetGizmoText(tag_clock), time_buf);
	w_clock = CreateText(name, flag, buf);
	clock_tick = atoi(flag+1);
	time_check = XtAddTimeOut(100, (XtTimerCallbackProc)CheckTime,
			(XtPointer)w_clock);
	return TRUE;
}

Boolean	ShowDate(name, flag)
	char	*name, *flag;
{
	w_cal = CreateText(name, flag, date_buf);
	ResetDate();
	XtAddTimeOut(6000, (XtTimerCallbackProc)CheckDate, (XtPointer)w_cal);
	return TRUE;
}

static	struct	utsname	sysname = {"", "", "", "", ""};

Boolean	ShowOS(name, flag)
	char	*name, *flag;
{
	Widget	w_txt;
	char	buf[BUFSIZ];

	if (*sysname.release == (char)0 )
		if (uname(&sysname) == -1)
			return FALSE;

	sprintf(buf, GetGizmoText(string_OSfmt), sysname.release,
			sysname.version, sysname.machine);
	w_txt = CreateText(name, flag, buf);
	return TRUE;
}

FILE	*cmdpipe = NULL;
char	memline[80]  = "";
char	mathline[80] = "";

Boolean	DoPrtconf()
{
	char	*ptr, buf[80];
	int	 memsize, n;

	if ((cmdpipe = popen(PRTCONF,"r")) == NULL)
		return FALSE;
	while (fgets(buf, 80, cmdpipe) != NULL) {
		if (strstr(buf,"Memory") != NULL) {
			for (ptr=buf; *ptr && !isdigit(*ptr); ptr++)
				;
			memsize = atoi(ptr);
			memline[0] = 0;
			sprintf(memline, GetGizmoText(string_memfmt),
				memsize);
		}
		else if (strstr(buf,"Math") != NULL) {
			for (ptr=buf; *ptr == ' '; ptr++)
				;
                        if (*ptr == '\t') ptr = ptr + 1;
			n = strstr(ptr, "Math") - ptr;
			strncpy(mathline, ptr, n); mathline[n] = '\0';
			strcat(mathline, GetGizmoText(string_math));
		}
	}
	pclose(cmdpipe);
	cmdpipe = (FILE *)-1;
	return TRUE;
}

Boolean	ShowMemory(name, flag)
	char	*name, *flag;
{
	Widget	w_txt;

	if (cmdpipe == NULL && DoPrtconf() == FALSE)
		return FALSE;
	if (*memline == 0)
		return FALSE;
	w_txt = CreateText(name, flag, memline);
	return TRUE;
}

Boolean	ShowMath(name, flag)
	char	*name, *flag;
{
	Widget	w_txt;

	if (cmdpipe == NULL && DoPrtconf() == FALSE)
		return FALSE;
	if (*mathline == 0)
		return FALSE;
	w_txt = CreateText(name, flag, mathline);
	return TRUE;
}

Boolean	ShowLogin(name, flag)
	char	*name, *flag;
{
	char	*ptr;
	char	buf[BUFSIZ];
	Widget	w_txt;

	if ((ptr = getenv("LOGNAME")) == NULL)
		return FALSE;

	sprintf(buf, "%s %s", GetGizmoText(tag_login), ptr);
	w_txt = CreateText(name, flag, buf);
	return TRUE;
}

Boolean	ShowNode(name, flag)
	char	*name, *flag;
{
	char	buf[BUFSIZ];
	Widget	w_txt;

	if (*sysname.nodename == (char)0 )
		if (uname(&sysname) == -1)
			return FALSE;

	sprintf(buf,"%s %s", GetGizmoText(tag_netnode), sysname.nodename);
	w_txt = CreateText(name, flag, buf);
	return TRUE;
}

Boolean ShowTZ(name, flag)
        char    *name, *flag;
{
        char    buf[BUFSIZ];
        static char   ptr[256];
        Widget  w_txt;

        if ((gettz(ptr)) == NULL)
                return FALSE;

        sprintf(buf,"%s %s", GetGizmoText(tag_tz), ptr);
        w_tz = CreateText(name, flag, buf);
        return TRUE;
}

Boolean	ShowDiskette(name, flag)
	char	*name, *flag;
{
	char	c, *ptr, *alias, *desc;
	char	buf[BUFSIZ];
	Widget	w_txt;

	for (c = '1'; ; c++) {
		sprintf(buf, "diskette%c", c);
		if ((ptr = DtamGetDev(buf, FIRST)) == NULL)
			break;
		alias = DtamDevAlias(ptr);
		strcpy(buf, alias);
		FREE(alias);
		desc = DtamDevDesc(ptr);
		FREE(ptr);
		strcat(strcat(buf,":  "),desc);
		FREE(desc);
		w_txt = CreateText(name, flag, buf);
	}
	return TRUE;
}

Boolean	ShowTape(name, flag)
	char	*name, *flag;
{
	char	*ptr, *attr, *attr2;
	char	*norewind = NULL;
	char	buf[BUFSIZ];
	int	n;
	Widget	w_txt;

	        sprintf(buf, "tape");
        for (ptr = DtamGetDev(buf, FIRST); ptr; ptr = DtamGetDev(buf, NEXT)) {
		attr = DtamDevAttr(ptr, CDEVICE);
		if (access(attr,F_OK) != 0) {
			FREE(attr);
			FREE(ptr);
			continue;
		}
		FREE(attr);
		strcpy(buf, attr = DtamDevAlias(ptr));
		FREE(attr);
		strcat(strcat(buf,":  "), attr = DtamDevDesc(ptr));
		FREE(attr);
		w_txt = CreateText(name, flag, buf);
		FREE(ptr);
	}
	return TRUE;
}

Boolean	ShowCdrom(name, flag)
	char	*name, *flag;
{
	char	*ptr, *attr; 
	char	buf[BUFSIZ];
	int	n;
	Widget	w_txt;

	        sprintf(buf, "cdrom");
        for (ptr = DtamGetDev(buf, FIRST); ptr; ptr = DtamGetDev(buf, NEXT)) {
		attr = DtamDevAttr(ptr, CDEVICE);
		if (access(attr,R_OK) != 0) {
			FREE(attr);
			FREE(ptr);
			continue;
		}
		FREE(attr);
		strcpy(buf, attr = DtamDevAlias(ptr));
		FREE(attr);
		strcat(strcat(buf,":  "), attr = DtamDevDesc(ptr));
		FREE(attr);
		w_txt = CreateText(name, flag, buf);
		FREE(ptr);
	}
	return TRUE;
}

Boolean	ShowUsersLic(name, flag)
	char	*name, *flag;
{
	Widget	w_txt;

	FILE    *cmdpipe;
        char    cmdbuf[BUFSIZ];
        char    labelbuf[BUFSIZ], users_licensed[BUFSIZ];
        char    *ptr;
        int     i;

        sprintf(cmdbuf, "/sbin/keyadm -g %s", "USERS");
        if ((cmdpipe = popen(cmdbuf,"r")) == NULL) {
                ErrorNotice (base.shell, GetGizmoText(string_noUsersLicensed));
                return 0;
        }
        else {
                fgets(cmdbuf, BUFSIZ, cmdpipe);
                pclose(cmdpipe);
	}
	ptr = cmdbuf;
	i=0;
	while (*ptr != '\t' && *ptr != '\n')
		users_licensed[i++]=*ptr++;
	users_licensed[i] = '\0';
	sprintf(labelbuf,"%s %s", GetGizmoText(tag_usersLicensed), 
		users_licensed);
	w_txt = CreateText(name, flag, labelbuf);
	return TRUE;
}

Boolean	ShowProcsLic(name, flag)
	char	*name, *flag;
{
	Widget	w_txt;

	FILE    *cmdpipe;
        char    cmdbuf[BUFSIZ];
        char    labelbuf[BUFSIZ], procs_licensed[BUFSIZ];
        char    *ptr;
        int     i;

        sprintf(cmdbuf, "/sbin/keyadm -g %s", "PROCESSORS");
        if ((cmdpipe = popen(cmdbuf,"r")) == NULL) {
                ErrorNotice (base.shell, GetGizmoText(string_noProcsLicensed));
                return 0;
        }
        else {
                fgets(cmdbuf, BUFSIZ, cmdpipe);
                pclose(cmdpipe);
	}
	ptr = cmdbuf;
	i=0;
	while (*ptr != '\t' && *ptr != '\n')
		procs_licensed[i++]=*ptr++;
	procs_licensed[i] = '\0';
	sprintf(labelbuf,"%s %s", GetGizmoText(tag_procsLicensed), 
		procs_licensed);
	w_txt = CreateText(name, flag, labelbuf);
	return TRUE;
}


#define	PROC	"/proc "
#define	PLEN	6
#define	DEVFD	"/dev/fd "
#define	FLEN	8
#define	SPROC	"/system/processor"
#define	SLEN	17
#define	NETWARE	"/.NetWare"
#define	NLEN	9

Boolean	ShowDisk(name, flag)
	char	*name, *flag;
{
	char	*ptr, *attr;
	char	buf[BUFSIZ];
	int	n;

	if ((cmdpipe = popen(MOUNT,"r")) == NULL)
		return FALSE;
	while (fgets(buf, BUFSIZ, cmdpipe)) {	/* filter out remote mounts */
		if (strstr(buf,"remote"))	/* and other non-disk items */
			continue;
		if (strncmp(buf,PROC,PLEN)==0 || strncmp(buf,DEVFD,FLEN)==0)
			continue;
		if (strncmp(buf,SPROC,SLEN)==0 )
			continue;
		if (strncmp(buf,NETWARE,NLEN)==0 )
			continue;
		if (strncmp(buf,"/stand ",7)==0)
			continue;
		if (strstr(buf," /dev/dsk/f"))
			continue;
		/*
		 *	note: it might make sense to show gauges for
		 *	for mounted diskettes, but not while the gauge
		 *	value is displayed in megabytes (as in MOTIF mode)
		 *	-- if some way to indicate that the MOTIF gauge is
		 *	percentage is implemented, this can be revisited.)
		 */
		ptr = strchr(buf,' ');
		*ptr = 0;
		CreateGauge(name, flag, buf);
	}
	return TRUE;
}

/****************************************************************************/
main(int argc, char *argv[])
{   
	Window		win;
	FILE		*dashrc;
	char		buf[BUFSIZ];
	char		*ptr;
	int		n;
        Boolean     isTrusted;

        setlocale (LC_ALL,"");
        isTrusted = (argc == 2 && strcmp(argv[1], "-t") == 0);
        if (isTrusted)
           owner = _DtamIsOwner("dashboard");

        setuid (0);

#ifdef	MEMUTIL
	InitializeMemutil();
#endif
	if ((ptr = setlocale(LC_TIME, NULL)) != NULL) {
		FILE	*monfile;
		int	n;
		sprintf(buf, "/usr/lib/locale/%s/LC_TIME", ptr);
		if (monfile = fopen(buf,"r")) {
			for (n = 0; n < 12; n++) {
				if (fgets(buf, BUFSIZ, monfile))
					buf[strlen(buf)-1] = '\0';
				months[n] = STRDUP(buf);
			}
			fclose(monfile);
		}
	}
	OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
	w_toplevel = XtInitialize("dashboard","System_Status",NULL,0,&argc,argv);

	XtGetApplicationResources(w_toplevel, (XtPointer)&rsc_table,
		my_resources, XtNumber(my_resources), NULL, 0);

	DtInitialize(w_toplevel);
	inch = OlPointToPixel(OL_HORIZONTAL,72);

	base.title = GetGizmoText(base.title);
	base.icon_name = GetGizmoText(base.icon_name);
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, &base, NULL, 0);
	XtSetArg(arg[0], XtNwidth, (DASH_WIDTH));
	XtSetArg(arg[1], XtNheight, DASH_WIDTH/2);
	XtSetValues(base.scroller, arg, 2); 
	XtSetArg(arg[0], XtNheight, DASH_WIDTH);
	XtSetArg(arg[1], XtNwidth, DASH_WIDTH);
	w_lefty = w_righty = w_dash = XtCreateManagedWidget("dash",
			formWidgetClass, base.scroller, arg, 2);
        XtSetArg(arg[0], XtNmappedWhenManaged, False);
        XtSetValues(base.shell, arg, 1);
	XtRealizeWidget(base.shell);
	uname(&sysname);
	sprintf(buf, "dtstatus:%s", sysname.nodename);
	win = DtSetAppId(XtDisplay(base.shell), XtWindow(base.shell), buf);
	if (win != None) {
		XRaiseWindow(XtDisplay(base.shell), win);
		XFlush(XtDisplay(base.shell));
		exit(0);
	}

	CreatePropSheet();
	CreateCompList();
	for (n = 0, ptr = comp_list[0]; *ptr;  ptr = comp_list[++n])
		CreateComponent(ptr);
	MapGizmo(BaseWindowGizmoClass, &base);
	XtMainLoop();
}
