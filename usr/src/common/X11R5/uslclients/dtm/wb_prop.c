#pragma ident	"@(#)dtm:wb_prop.c	1.124"

/******************************file*header********************************

    Description:
     This file contains the source code for the wastebasket's Properties
	window.
*/
                              /* #includes go here     */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include <Xm/Protocols.h>       /* for XmAddWMProtocolCallback */
#include <Xm/RowColumn.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/NumericGiz.h>
#include <MGizmo/PopupGizmP.h>
#include <MGizmo/ModalGizmo.h>

#include <DtWidget/SpinBox.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void MethodCB(Widget w, XtPointer client_data, XtPointer call_data);
static void UnitCB(Widget, XtPointer, XtPointer); 
static void DelCurFiles(Widget parent, int ninterval,unsigned long ntime_unit);
static void DoDelCurFiles(Widget, XtPointer, XtPointer);
static void VerifyPopdnCB(Widget, XtPointer, XtPointer);
static void PopdownCB(Widget, XtPointer, XtPointer);
static void ApplyChanges();
static void PropMenuCB(Widget, XtPointer, XtPointer); 
static void DeleteFiles(char **src_list, int cnt);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

static int MaxInterval[3] = { 44640, 744, 31 };

typedef struct {
	unsigned int   cleanUpMethod;
	unsigned int   interval;
	unsigned int   unit_idx;
	unsigned long  tm_interval;
	unsigned long  time_unit;
	char           *time_str;
} CurValRec;

static unsigned long timer_unit[]  = { MINUNIT, HOURUNIT, DAYUNIT };
static char *timer_strs[6];

static Widget unit_excl;
static Widget cleanup_method;
static Widget tf;
static Widget tf_caption;
static Widget footer;
static CurValRec newval;

/* Gizmo declarations */

static MenuItems methodMenuItems[] = {
  { True, TXT_BY_TIMER,    TXT_M_BY_TIMER,    I_PUSH_BUTTON, NULL, NULL, NULL,
	True },
  { True, TXT_ON_EXIT,     TXT_M_ON_EXIT,     I_PUSH_BUTTON, NULL, NULL, NULL,
	False },
  { True, TXT_IMMEDIATELY, TXT_M_IMMEDIATELY, I_PUSH_BUTTON, NULL, NULL, NULL,
	False },
  { True, TXT_NEVER,       TXT_M_NEVER,       I_PUSH_BUTTON, NULL, NULL, NULL,
	False },
  { NULL }
};

static MenuGizmo methodMenu = {
  NULL, "methodMenu", "", methodMenuItems, MethodCB, NULL
};

static ChoiceGizmo methodChoice = {
	NULL,		/* Help information */
	"method",	/* Name of menu gizmo */
	&methodMenu,	/* Choice buttons */
	G_OPTION_BOX,	/* Type of menu created */
};

static GizmoRec methodRec[] = {
	{ ChoiceGizmoClass, &methodChoice },
};

static LabelGizmo methodLabel = {
	NULL,			/* help */
	"methodLabel",		/* widget name */
	TXT_CLEAN_UP_METHOD,	/* caption label */
	False,			/* align caption */
	methodRec,		/* gizmo array */
	XtNumber(methodRec),	/* number of gizmos */
};

static NumericGizmo intervalGizmo = {
	NULL,
	"interval",
	7,
	0,
	31,
	1,
	0,
};

static GizmoRec intervalRec[] = {
	{ NumericGizmoClass, &intervalGizmo },
};

static LabelGizmo intervalLabel = {
	NULL,			/* help */
	"intervalLabel",	/* widget name */
	TXT_REMOVE_AFTER,	/* caption label */
	False,			/* align caption */
	intervalRec,		/* gizmo array */
	XtNumber(intervalRec),	/* number of gizmos */
};

static MenuItems timeMenuItems[] = {
  { True, TXT_WB_U_MINUTE, TXT_M_MINUTES, I_PUSH_BUTTON, NULL, NULL, NULL, True },
  { True, TXT_WB_U_HOUR,   TXT_M_HOURS,   I_PUSH_BUTTON, NULL, NULL, NULL, False },
  { True, TXT_WB_U_DAY,    TXT_M_DAYS,    I_PUSH_BUTTON, NULL, NULL, NULL, False },
  { NULL }
};

static MenuGizmo timeMenu = {
  NULL, "timeMenu", "", timeMenuItems, UnitCB, NULL
};

static ChoiceGizmo timeChoice = {
	NULL,		/* Help information */
	"time",		/* Name of menu gizmo */
	&timeMenu,	/* Choice buttons */
	G_OPTION_BOX,	/* Type of menu created */
};

static GizmoRec rcGizmos[] =  {
	{ LabelGizmoClass, &intervalLabel },
	{ ChoiceGizmoClass, &timeChoice },
};

static ContainerGizmo contGizmo = {
	NULL,			/* help */
	"rowCol",		/* name */
	G_CONTAINER_RC,		/* container type */
	0,			/* width */
	0,			/* height */
	rcGizmos,		/* gizmo array */
	XtNumber(rcGizmos),	/* num gizmos */
};

static GizmoRec propGizmos[] = {
	{ LabelGizmoClass,	&methodLabel },
	{ ContainerGizmoClass,	&contGizmo },
};

static MenuItems propMenuItems[] = {
  { True, TXT_OK,                 TXT_M_OK,               I_PUSH_BUTTON, NULL,
	NULL, NULL, True },
  { True, TXT_Reset,              TXT_M_Reset,            I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
  { True, TXT_CANCEL,             TXT_M_CANCEL,           I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
  { True, TXT_HELP,               TXT_M_HELP,             I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
  { NULL }
};

static MenuGizmo propMenu = {
  NULL, "propMenu", "", propMenuItems, PropMenuCB, NULL, XmHORIZONTAL,
  1, 0, 2
};

static PopupGizmo propPopup = {
	NULL,			/* help */
	"Properties",		/* name of shell */
	TXT_WB_OPTIONS,		/* window title */
	&propMenu,		/* pointer to menu info */
	propGizmos,		/* gizmo array */
	XtNumber(propGizmos),	/* number of gizmos */
};

typedef enum {
	Ok,
	Reset,
	Cancel,
	Help
} PropMenuItemIndex;

/* Define gizmo for delete files notice */

typedef struct _dummy {
	int	itemNum;
	int	cnt;
} _Dummy;

_Dummy	d0 = {0, 0};
_Dummy	d1 = {1, 0};
_Dummy	d2 = {2, 0};

static MenuItems delFileMenuItems[] = {
    {True, TXT_YES, TXT_M_YES, I_PUSH_BUTTON, NULL, DoDelCurFiles, (XtPointer)&d0},
    {True, TXT_NO,  TXT_M_NO,  I_PUSH_BUTTON, NULL, DoDelCurFiles, (XtPointer)&d1},
    {True, TXT_HELP,TXT_M_HELP,I_PUSH_BUTTON, NULL, DoDelCurFiles, (XtPointer)&d2},
    { NULL }			/* NULL terminated */
};

static MenuGizmo delFileMenu = {
	NULL, "delFileMenu", "", delFileMenuItems, NULL, NULL,
	XmHORIZONTAL, 1, 1
};

static ModalGizmo delFilePrompt = {
	NULL,				/* help info */
	"delFilePrompt",		/* shell name */
	TXT_WB_DELETE_NOTICE,		/* title */
	&delFileMenu,			/* menu */
	TXT_DELETE_NOW,			/* message */
	NULL,				/* gizmos */
	0,				/* num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL, /* style */
	XmDIALOG_WARNING		/* type */
};

/***************************public*procedures****************************

    Public Procedures
*/


/****************************procedure*header*****************************
 * Creates and displays the Wastebasket properties window.  Invoked when
 * Set Properties button in Actions menu is selected.
 */
void
DmWBPropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget w1;
	Widget w2;
	Boolean first = True;

	DmVaDisplayStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop), False, NULL);

	if (wbinfo.propGizmo != NULL) {
		MapGizmo(PopupGizmoClass, wbinfo.propGizmo);
		return;
	}
	if (first) {
		/* get strings for timer units and save them */
		timer_strs[0] = Dm__gettxt(TXT_MINUTE_STR);
		timer_strs[1] = Dm__gettxt(TXT_HOUR_STR);
		timer_strs[2] = Dm__gettxt(TXT_DAY_STR);
		timer_strs[3] = Dm__gettxt(TXT_MINUTES_STR);
		timer_strs[4] = Dm__gettxt(TXT_HOURS_STR);
		timer_strs[5] = Dm__gettxt(TXT_DAYS_STR);
		first = False;
	}
	newval.cleanUpMethod = wbinfo.cleanUpMethod;
	newval.interval      = wbinfo.interval;
	newval.unit_idx      = wbinfo.unit_idx;
	newval.tm_interval   = wbinfo.tm_interval;
	newval.time_unit     = wbinfo.time_unit;

	if (wbinfo.interval == 1)
		newval.time_str = wbinfo.time_str =
			timer_strs[wbinfo.unit_idx];
	else
		newval.time_str = wbinfo.time_str =
			timer_strs[wbinfo.unit_idx + 3];

	((MenuGizmo *)(propPopup.menu))->clientData = (XtPointer)NULL;

	wbinfo.propGizmo = CreateGizmo(DESKTOP_WB_WIN(Desktop)->shell,
				PopupGizmoClass, &propPopup, NULL, 0);

	w1 = GetPopupGizmoShell(wbinfo.propGizmo);

	XmAddWMProtocolCallback(w1, XA_WM_DELETE_WINDOW(XtDisplay(w1)),
		PopdownCB, (XtPointer)NULL);

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(w1, Dm__arg, 1);

	wbinfo.numericGizmo = (NumericGizmo *)QueryGizmo(PopupGizmoClass,
		wbinfo.propGizmo, GetGizmoGizmo, "interval");
	SetNumericGizmoValue(wbinfo.numericGizmo, wbinfo.interval);

	/* update max interval depending on current selection of time unit */
	w1 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
		"interval");
	XtSetArg(Dm__arg[0], XmNmaximumValue, MaxInterval[wbinfo.unit_idx]);
	XtSetValues(w1, Dm__arg, 1);

	wbinfo.rowCol = (Widget)QueryGizmo(PopupGizmoClass, wbinfo.propGizmo,
			GetGizmoWidget, "rowCol");
	XtSetArg(Dm__arg[0], XmNnumColumns, 2);
	XtSetArg(Dm__arg[1], XmNorientation, XmVERTICAL);
	XtSetArg(Dm__arg[2], XmNpacking, XmPACK_COLUMN);
	XtSetValues(wbinfo.rowCol, Dm__arg, 3);
	XtSetSensitive(wbinfo.rowCol, WB_BY_TIMER(wbinfo));


	/* update Clean Up Method menu to current setting */
	sprintf(Dm__buffer, "methodMenu:%d", wbinfo.cleanUpMethod);
	/* Get menu widget id */
	w1 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
		"methodMenu");
	/* Get button widget id */
	w2 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
		Dm__buffer);

	XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
	XtSetValues(w1, Dm__arg, 1);

	/* update time unit menu to current setting */
	sprintf(Dm__buffer, "timeMenu:%d", wbinfo.unit_idx);
	/* Get menu widget id */
	w1 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
		"timeMenu");
	/* Get button widget id */
	w2 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
		Dm__buffer);

	XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
	XtSetValues(w1, Dm__arg, 1);

	/* register for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(wbinfo.propGizmo),
		WB_HELP_ID(Desktop), WB_HELPFILE, WB_OPTIONS_SECT);

	MapGizmo(PopupGizmoClass, wbinfo.propGizmo);

} /* end of DmWBPropCB */

/****************************procedure*header*****************************
 * PropMenuCB -
 */
static void
PropMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

	switch(cbs->index) {
	case Ok:
		ApplyChanges();
		break;
	case Reset:
	{
		Widget w1;
		Widget w2;

		SetNumericGizmoValue(wbinfo.numericGizmo, wbinfo.interval);
		XtSetSensitive(wbinfo.rowCol, WB_BY_TIMER(wbinfo));

		sprintf(Dm__buffer, "methodMenu:%d", wbinfo.cleanUpMethod);
		/* Get menu widget id */
		w1 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo,
			GetGizmoWidget, "methodMenu");
		/* Get button widget id */
		w2 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo,
			GetGizmoWidget, Dm__buffer);
		XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
		XtSetValues(w1, Dm__arg, 1);

		sprintf(Dm__buffer, "timeMenu:%d", wbinfo.unit_idx);
		/* Get menu widget id */
		w1 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo,
			GetGizmoWidget, "timeMenu");
		/* Get button widget id */
		w2 = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo,
			GetGizmoWidget, Dm__buffer);
		XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
		XtSetValues(w1, Dm__arg, 1);

		newval.time_str      = wbinfo.time_str;
		newval.interval      = wbinfo.interval;
		newval.time_unit     = wbinfo.time_unit;
		newval.unit_idx      = wbinfo.unit_idx;
		newval.cleanUpMethod = wbinfo.cleanUpMethod;
	}
		break;
	case Cancel:
		BringDownPopup(GetPopupGizmoShell(wbinfo.propGizmo));
		break;
	case Help:
		DmDisplayHelpSection(DmGetHelpApp(WB_HELP_ID(Desktop)),
			NULL, WB_HELPFILE, WB_OPTIONS_SECT);
		break;
	}
} /* end of PropMenuCB */

/****************************procedure*header*****************************
 * Applies new properties and update appropriate resources in .Xdefaults.
 */
static void
ApplyChanges()
{
	/* unregister timeout procedure, if set */
	if (wbinfo.timer_id != NULL) {
		XtRemoveTimeOut(wbinfo.timer_id);
		wbinfo.timer_id = NULL;
	}
	switch(newval.cleanUpMethod) {
	case WBByTimer:
	{
		int value = GetNumericGizmoValue(wbinfo.numericGizmo);

		newval.interval = value;

		if (newval.interval < 2)
			newval.time_str = timer_strs[newval.unit_idx];
		else
			newval.time_str = timer_strs[newval.unit_idx + 3];

		wbinfo.cleanUpMethod = WBByTimer;
		wbinfo.interval  = newval.interval;
		wbinfo.time_str  = newval.time_str;
		wbinfo.time_unit = newval.time_unit;
		wbinfo.unit_idx  = newval.unit_idx;

		/*
		 * Prompt user to delete files which are now "eligible" for
		 * deletion only if timer is not suspended and the wastebasket
		 * is not empty.
		 */
		if (!wbinfo.suspend && !WB_IS_EMPTY(Desktop) &&
		    (newval.tm_interval = (unsigned long)(newval.interval *
			newval.time_unit)) != wbinfo.tm_interval)
		{
			wbinfo.restart = (newval.interval > 0) ? True : False;
			DelCurFiles(DESKTOP_WB_WIN(Desktop)->shell,
				newval.interval, newval.time_unit);
		} else
			DmWBRestartTimer();

		sprintf(Dm__buffer,"%s %d %s.",Dm__gettxt(TXT_WB_BY_TIMER_MSG),
			newval.interval, newval.time_str);
		SetPopupWindowLeftMsg(wbinfo.propGizmo, Dm__buffer);
	}
		break;

	case WBOnExit:
		wbinfo.cleanUpMethod = WBOnExit;
		SetPopupWindowLeftMsg(wbinfo.propGizmo,
			Dm__gettxt(TXT_WB_ON_EXIT_MSG));
		break;

	case WBImmediately:
		wbinfo.cleanUpMethod  = WBImmediately;
		SetPopupWindowLeftMsg(wbinfo.propGizmo,
			Dm__gettxt(TXT_WB_IMMEDIATE_MSG));

		if (!WB_IS_EMPTY(Desktop))
			DelCurFiles(DESKTOP_WB_WIN(Desktop)->shell, 0,
				newval.time_unit);
		break;

	case WBNever:
		wbinfo.cleanUpMethod  = WBNever;
		SetPopupWindowLeftMsg(wbinfo.propGizmo,
			Dm__gettxt(TXT_WB_NEVER_MSG));
		break;
	}

	if (wbinfo.cleanUpMethod != 0) {
		/* By Timer not selected, turn off timer. */
		if (wbinfo.timer_id != NULL) {
			XtRemoveTimeOut(wbinfo.timer_id);
			wbinfo.timer_id = NULL;
		}
		wbinfo.restart  = False;
		DmVaDisplayState((DmWinPtr)DESKTOP_WB_WIN(Desktop), NULL);

		/* make Resume/Suspend Timer in Actions menu insensitive */
		DmWBToggleTimerBtn(False);
	}
	/* save values of properties in WB's .dtinfo file */
	DmSaveWBProps();

	BringDownPopup(GetPopupGizmoShell(wbinfo.propGizmo));

} /* end of ApplyChanges */

/****************************procedure*header*****************************
 * Callback for unit exclusives. 
 */
static void
UnitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget spinBox;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

	newval.time_unit = timer_unit[cbs->index];
	newval.unit_idx  = cbs->index;

	/* update max for NumericGizmo */
	spinBox = QueryGizmo(PopupGizmoClass, wbinfo.propGizmo, GetGizmoWidget,
			"interval");
	XtSetArg(Dm__arg[0], XmNmaximumValue, MaxInterval[cbs->index]);
	XtSetValues(spinBox, Dm__arg, 1);

} /* end of UnitCB *

/****************************procedure*header*****************************
 * Called when a change in timer settings results in files due for deletion.
 * Asks the user if he/she wants to delete those files now.
 */
static void
DelCurFiles(Widget parent, int ninterval, unsigned long ntime_unit)
{
	DmItemPtr itp;
	char **src_list;
	char **src;
	int cnt;
	time_t current_time;
	int interval_unit;
	Boolean reset = False;

	src = src_list = (char **)MALLOC(sizeof(char *)*NUM_WB_OBJS(Desktop));

	/* find out which files meet the deletion criterion, if any */
	if (ninterval > 0) {
		time(&current_time);

		/* convert from milliseconds to seconds */
		interval_unit = (ntime_unit) / 1000;
	}

	for (itp = DESKTOP_WB_WIN(Desktop)->views[0].itp;
	  itp < DESKTOP_WB_WIN(Desktop)->views[0].itp +
	  DESKTOP_WB_WIN(Desktop)->views[0].nitems; itp++)
	{
		if (ITEM_MANAGED(itp) && (WB_IMMEDIATELY(wbinfo) ||
			(((current_time -
				strtoul(DmGetObjProperty(ITEM_OBJ(itp),
				TIME_STAMP, NULL), NULL, 0)) / interval_unit)
				>= ninterval)))
		{
			/* Skip busy items and reset time stamp to current time
			 * if this function was called due to a change in timer
			 * settings.
			 */
			if (!ITEM_SENSITIVE(itp))
			{
				if (!WB_IMMEDIATELY(wbinfo))
				{
					sprintf(Dm__buffer,"%ld",current_time);
					DmSetObjProperty(ITEM_OBJ(itp),
						TIME_STAMP, Dm__buffer,NULL);
					reset = True;
				}
				continue;
			}
			*src++ = strdup(ITEM_OBJ_NAME(itp));
		}
	}
	if (reset)
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp,
			wbinfo.dtinfo, 0);

	if ( (cnt = src - src_list) == 0 )
	{
		/* No files to delete after all */
		FREE((void *)src_list);
		DmWBRestartTimer();
		return;
	}
	if (WB_IMMEDIATELY(wbinfo) ||
	  (WB_BY_TIMER(wbinfo) && !wbinfo.suspend && wbinfo.interval == 0))
	{
		DeleteFiles(src_list, cnt);
		return;
	}
	/* Create the modal gizmo */
	d0.cnt = cnt;
	d1.cnt = cnt;
	d2.cnt = cnt;
	wbinfo.delFileGizmo = CreateGizmo(
		XtParent(DESKTOP_WB_WIN(Desktop)->views[0].box),
		ModalGizmoClass, &delFilePrompt, NULL, 0);

	XtSetArg(Dm__arg[0], XmNuserData, src_list);
	XtSetValues(GetModalGizmoDialogBox(wbinfo.delFileGizmo), Dm__arg, 1);

	DmRegContextSensitiveHelp(GetModalGizmoDialogBox(wbinfo.delFileGizmo),
		WB_HELP_ID(Desktop), WB_HELPFILE, WB_DUE_DELETE_SECT);

	MapGizmo(ModalGizmoClass, wbinfo.delFileGizmo);

}				/* end of DelCurFiles */

/****************************procedure*header*****************************
 * Deletes files which are due for deletion due to a change in timer settings.
 */
static void
DoDelCurFiles(Widget w, XtPointer client_data, XtPointer call_data)
{
	int index = ((_Dummy *)client_data)->itemNum;
	int cnt = ((_Dummy *)client_data)->cnt;
	DmWBCPDataPtr cpdp;
	char **src_list;
	DmFileOpInfoPtr opr_info;
	Widget shell;

	if (index == 2) { /* Help was selected */
		DmDisplayHelpSection(DmGetHelpApp(WB_HELP_ID(Desktop)), NULL,
			WB_HELPFILE, WB_DUE_DELETE_SECT);
		return;
	}
	XtSetArg(Dm__arg[0], XmNuserData, &src_list);
	XtGetValues(GetModalGizmoDialogBox(wbinfo.delFileGizmo), Dm__arg, 1);

	shell = GetModalGizmoShell(wbinfo.delFileGizmo);
	FreeGizmo(ModalGizmoClass, wbinfo.delFileGizmo);
	XtDestroyWidget(shell);

	if (index == 1)
	{
		/* "No" was selected */
		int i;
		DmItemPtr itp;
		time_t current_time;

		time(&current_time);
		for (i = 0; i < cnt; i++) {
			/* reset time stamp s.t. the file(s) will not be
			 * deleted immediately when the timer is restarted.
			 */
			itp=DmObjNameToItem((DmWinPtr)DESKTOP_WB_WIN(Desktop),
					src_list[i]);
			sprintf(Dm__buffer, "%ld", current_time);
			DmSetObjProperty(ITEM_OBJ(itp), TIME_STAMP,
				Dm__buffer, NULL);
			FREE(src_list[i]);
		}
		FREE((void *)src_list);
		DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp,
			wbinfo.dtinfo, 0);
		DmWBRestartTimer();
		return;
	}
	/* "Yes" was selected - delete files */
	DeleteFiles(src_list, cnt);

} /* end of DoDelCurFiles */

/****************************procedure*header*****************************
 * DeleteFiles -
 */
static void
DeleteFiles(char **src_list, int cnt)
{
	DmWBCPDataPtr cpdp;
	DmFileOpInfoPtr opr_info;

	cpdp = (DmWBCPDataPtr)CALLOC(1, sizeof(DmWBCPDataRec));
	cpdp->op_type = DM_TIMERCHG;

	opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
	opr_info->type		= DM_DELETE;
	opr_info->options	= 0;
	opr_info->target_path	= NULL;
	opr_info->src_path	= strdup(DM_WB_PATH(Desktop));
	opr_info->src_list	= src_list;
	opr_info->src_cnt	= cnt;
	opr_info->src_win	= DESKTOP_WB_WIN(Desktop);
	opr_info->dst_win	= NULL;
	opr_info->x		= UNSPECIFIED_POS;
	opr_info->y		= UNSPECIFIED_POS;
	DmDoFileOp(opr_info, DmWBClientProc, (XtPointer)cpdp);

} /* end of DeleteFiles */

/****************************procedure*header*****************************
 * Displays help on the Wastebasket properties window.
 */
static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmDisplayHelpSection(DmGetHelpApp(WB_HELP_ID(Desktop)), NULL,
		WB_HELPFILE, WB_OPTIONS_SECT);
}	/* end of HelpCB */

/****************************procedure*header*****************************
 * Pops down the Wastebasket properties window. 
 */
static void
PopdownCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(GetPopupGizmoShell(wbinfo.propGizmo));
}	/* end of PopdownCB */

/****************************procedure*header*****************************
 * Called when the Clean Up Method exclusives is selected.
 */
static void
MethodCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	Boolean sensitive = (cbs->index == 0);

	newval.cleanUpMethod = cbs->index;
	XtSetSensitive(wbinfo.rowCol, sensitive);

}	/* end of MethodCB */

/****************************procedure*header*****************************
 * Save current wastebasket property settings as container properties
 * in its .dtinfo file.
 */
void
DmSaveWBProps()
{
     /* save WB properties as container properties and in WB's .dtinfo */
     sprintf(Dm__buffer, "%d", wbinfo.cleanUpMethod);
     DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
	CLEANUP_METHOD, Dm__buffer, NULL);

	if (wbinfo.cleanUpMethod == WBByTimer)
	{
     	sprintf(Dm__buffer, "%d", wbinfo.suspend);
     	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
		TIMER_STATE, Dm__buffer, NULL);

     	sprintf(Dm__buffer, "%d", wbinfo.restart);
     	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
		RESTART_TIMER, Dm__buffer, NULL);

     	sprintf(Dm__buffer, "%d", wbinfo.interval);
     	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
		TIMER_INTERVAL, Dm__buffer, NULL);

     	sprintf(Dm__buffer, "%d", wbinfo.unit_idx);
     	DtSetProperty(&(DESKTOP_WB_WIN(Desktop)->views[0].cp->plist),
		UNIT_INDEX, Dm__buffer, NULL);
	}
     DmWriteDtInfo(DESKTOP_WB_WIN(Desktop)->views[0].cp, wbinfo.dtinfo, 0);

}					/* end of DmSaveWBProps */

