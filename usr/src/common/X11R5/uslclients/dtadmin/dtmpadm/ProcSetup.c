#ifndef NOIDENT
#pragma ident	"@(#)ProcSetup.c	1.11"
#endif
/*
 *	Processor Setup - Desktop Administration for the MP UNIX.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/unistd.h>
#include <sys/keyctl.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/PanedW.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/TextF.h>
#include <Xm/SeparatoG.h>
#include <Xm/MessageB.h>
#include "ProcSetup.h"
#include "proc_msgs.h"

#include <DesktopP.h>
#include <FIconBoxI.h>
#include "Flat.h"
#include "DtI.h"

#define GRID_WD         60
#define GRID_HI         80
#define NUM_COLS        4
#define NUM_ROWS        2

extern char	*mygettxt();
extern Boolean	_IsOwner();


#define Xm_MNE(M,MNE_INFO,OP)\
	if (M) {\
		mne = (unsigned char *)mygettxt(M);\
		ks = XStringToKeysym((char *)mne);\
		(MNE_INFO).mne = (unsigned char *)strdup((char *)mne);\
		(MNE_INFO).mne_len = strlen((char *)mne);\
		(MNE_INFO).op = OP;\
	} else {\
		mne = (unsigned char *)NULL;\
		ks = NoSymbol;\
	}

#define XmSTR_N_MNE(S,M,M_INFO,OP)\
	string = XmStringCreateSimple(S);\
	Xm_MNE(M,M_INFO,OP)

	/* Assume i */
#define REG_NME(W,MNE_INFO,N_MNE)\
	DmRegisterMnemonic(W,MNE_INFO,N_MNE);\
	for (i = 0; i < N_MNE; i++) {\
		XtFree((char *)MNE_INFO[i].mne);\
	}

#define MI_TOTAL	10

/* Global structure used to store the information for each configured
 * processor in the system.
 */

typedef struct	_pstruct {
	int	proc_id;	/* processor id */
	int	proc_state;	/* 1 = active, 0 = inactive */
	int	is_select;	/* 1 = selected, 0 = not selected */
} Process_t;

Process_t	*process_arr;

/* data structure for flat icon box */
static String fields[] = { XmNx, XmNy, XmNwidth, XmNheight, XmNlabelString,
                               XmNset, XmNsensitive, XmNobjectData };

typedef struct {
    XtArgVal x;
    XtArgVal y;
    XtArgVal width;
    XtArgVal height;
    XtArgVal label;
    XtArgVal set;
    XtArgVal sensitive;
    XtArgVal obj;
} MyItem;

static MyItem	*items;
static DmFclassRec	online_fclass, offline_fclass;
static DmObjectRec	online_obj, offline_obj;	/* = { &fclass }; */

Widget	toplevel, container, main_w, status, sktext, sntext, selected_icon; 
Widget	prop_dialogs=NULL, prop_form=NULL, online_but, offline_but;
XtAppContext	app;
void	online_cb(), offline_cb(), prop_cb(), exit_cb();
void	mphelp_cb(), do_info_ok();
int	selection=0;
char	infostr[BUF_SIZE];
long	nproc_conf=0;

DmGlyphPtr	glyph;
DmItemPtr	itp;
DmContainerPtr	cp;
DmObjectPtr	op;

Boolean		show_offline=FALSE, show_online=FALSE;
Boolean		I_am_owner=FALSE;

void 	InfoMsg();
void	TopMsg();

/* Processor menu items */

MenuItem actions_items[] = {
	{ label_properties, (Widget)NULL, &xmPushButtonGadgetClass, mnemonic_properties, NULL, NULL, prop_cb, 0, NULL},
	{ label_separator, (Widget)NULL, &xmSeparatorGadgetClass, mnemonic_separator, NULL, NULL, NULL, 0, NULL},
	{ label_exit, (Widget)NULL, &xmPushButtonGadgetClass, mnemonic_exit2, NULL, NULL, exit_cb, 0, NULL},
	NULL,
};

typedef enum _actions_items_index
{enum_properties, enum_separator, enum_exit} actions_items_index;

/* ^^Temporay help file */
HelpText AppHelp = {
	"MpHelp","dtadmin/multiproc.hlp","10",
};

HelpText TOCHelp = {
	"MpHelp","dtadmin/multiproc.hlp",0,
};

HelpText PropHelp = {
	"MpHelp","dtadmin/multiproc.hlp","60",
};
/* Help menu items */
MenuItem help_items[] = {
	{ label_mpadmin, (Widget )NULL, &xmPushButtonWidgetClass, mnemonic_mpadmin, NULL, NULL, mphelp_cb, (XtPointer)&AppHelp, NULL},
	{ label_toc, (Widget )NULL, &xmPushButtonWidgetClass, mnemonic_toc, NULL, NULL, mphelp_cb, (XtPointer)&TOCHelp, NULL},
	{ label_separator, (Widget )NULL, &xmSeparatorGadgetClass, mnemonic_separator, NULL, NULL, NULL, 0, NULL},
	{ label_helpdesk, (Widget )NULL, &xmPushButtonWidgetClass, mnemonic_helpdesk, NULL, NULL, mphelp_cb, (XtPointer)0, NULL},
	NULL,
};

void
mphelp_cb(w, client_data, cbs)
Widget	w;
XtPointer	client_data;
XmAnyCallbackStruct	*cbs;
{
	DisplayHelp(w, (HelpText *)client_data);
}


/*
 * DisplayHelp() -- Send a message to dtm to display a help window.  
 *		If help is NULL, then ask dtm to display the help desk.
 */

void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    static DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay (widget);
    Window			win = XtWindow (XtParent (widget));
    char *AppTitle="Processor_Setup";
    char *AppName="Processor_Setup";

    req = (DtRequest *) &displayHelpReq;
    memset(req, 0, sizeof(displayHelpReq));
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
    displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = AppName;
	displayHelpReq.app_title = AppTitle;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */

/*
 * update_status() - update the status line on the base window to 
 *		     reflect the current status of the machine.
 */
void
update_status()
{
	char		tmpstr[BUF_SIZE];
	XmString	status_line;
	long		sysconf(), nproc_onln;
	int		proc_limit = 0;

	if ((nproc_onln = sysconf(_SC_NPROCESSORS_ONLN)) == -1) {
		InfoMsg(toplevel, mygettxt(ERR_cantGetNumProc));
		return;
	}
	sprintf(tmpstr, mygettxt(string_runprocs), nproc_conf, nproc_onln);
	status_line = XmStringCreateSimple(tmpstr);
	XtVaSetValues(status, XmNlabelString, status_line, NULL);
	XmStringFree(status_line);
}

void
do_info_ok(w, info_dialog, cbs)
Widget	w, info_dialog;
XmAnyCallbackStruct	*cbs;
{
	XtPopdown(info_dialog);
}

/*
 * TopMsg() -- Inform the user that error has occurred. Since there
 * 		is not toplevel shell yet, we need to create one.
 */

void
TopMsg(msg)
char	*msg;
{
	static	Widget	top, err_dialog;
	Arg		arg[10];
	int		n;
	XmString	str;
	char		buf[BUF_SIZE];

	Widget		OK;
	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char		*mne;
	KeySym			ks;
	XmString		string;
	int			i;

/*
	top = XtVaAppInitialize(&app, "Proccessor_Admin_error",
		NULL, 0, 0, NULL, NULL, NULL);
	XtVaSetValues(top, XmNtitle, mygettxt(title_toperr), 
		XmNallowShellResize, True,
		NULL);
*/
	n=0;
	str = XmStringCreateSimple(mygettxt(label_ok));
	XtSetArg(arg[n], XmNokLabelString, str);n++;
	XtSetArg(arg[n], XmNdialogType, XmDIALOG_ERROR); n++;
	XtSetArg(arg[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON);n++;
	err_dialog = XmCreateMessageBox(toplevel,
		"error", arg, n);
	XmStringFree(str);
	XtAddCallback(err_dialog, XmNokCallback, 
		(void(*)())exit_cb, NULL);
	strcpy(buf, msg);
	str = XmStringCreateLtoR(buf, "charset");
	XmSTR_N_MNE(label_ok, mnemonic_ok, mne_info[0],
		DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS);
	XtVaSetValues(err_dialog, XmNmessageString, str,
		XmNmnemonic, ks,
		NULL);
	XmStringFree(string);
	XmStringFree(str);

	mne_info[0].w =  XmMessageBoxGetChild(err_dialog, XmDIALOG_OK_BUTTON);
	mne_info[0].cb = (XtCallbackProc)exit_cb;
	XtManageChild(err_dialog);
	REG_NME(err_dialog, mne_info, 1);

	XtRealizeWidget(toplevel);
	DtiInitialize(toplevel);
	XtAppMainLoop(app);
}

/* 
 * InfoMsg() -- Inform the user that something has gone wrong in the
 * 		   process (e.g online, offine).
 */

void
InfoMsg(parent, msg)
Widget	parent;
char	*msg;
{
	static Widget 	info_dialog;
	Widget		OK, HELP;
	char		buf[BUF_SIZE];
	XmString	text, ok_str;
	void		do_info_ok();

	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char		*mne;
	KeySym			ks;
	XmString		string;
	int			i;

	if (!info_dialog) {
		info_dialog = XmCreateInformationDialog(parent, 
				"info", NULL, 0);
		XtVaSetValues(XtParent(info_dialog),
			XmNtitle, mygettxt(title_information),
			NULL);
		XtUnmanageChild(XmMessageBoxGetChild(info_dialog, 
			XmDIALOG_CANCEL_BUTTON));
		OK = XmMessageBoxGetChild(info_dialog, XmDIALOG_OK_BUTTON);
		HELP = XmMessageBoxGetChild(info_dialog,
			XmDIALOG_HELP_BUTTON);
		XmSTR_N_MNE(label_ok, mnemonic_ok, mne_info[0],
			DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
		XtVaSetValues(OK, XmNmnemonic, ks, NULL);
		XmStringFree(string);

		XmSTR_N_MNE(label_help, mnemonic_help, mne_info[1],
			DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
		XtVaSetValues(HELP, XmNmnemonic, ks, NULL);
		XmStringFree(string);

		XtAddCallback(info_dialog, XmNokCallback,
			(void(*)())do_info_ok, XtParent(info_dialog));
		XtAddCallback(info_dialog, XmNhelpCallback,
			(void(*)())mphelp_cb, NULL);

		mne_info[0].w = OK;
		mne_info[0].cb = (XtCallbackProc)do_info_ok;
		mne_info[0].cd = (XtPointer)XtParent(info_dialog);

		mne_info[1].w = HELP;
		mne_info[1].cb = (XtCallbackProc)mphelp_cb;

		REG_NME(info_dialog, mne_info, 2);
	}
	strcpy(buf, msg);
	text = XmStringCreateLtoR(buf, "charset");
	ok_str = XmStringCreateSimple(mygettxt(label_ok));
	XtVaSetValues(info_dialog,
		XmNmessageString, text,
		XmNokLabelString, ok_str,
		NULL);
	XmStringFree(text);
	XmStringFree(ok_str);
	XtManageChild(info_dialog);
	XtPopup(XtParent(info_dialog), XtGrabNone);	
}

/*
 * do_select() -- 
 *		  If the properties sheet is up, call the prop_cb() to
 *		  update the information for the selection.
 */

void
do_select(w, client_data, cbs) 
Widget	w;
XtPointer	client_data;
XtPointer	*cbs;
{
	ExmFIconBoxButtonCD 	*d = (ExmFIconBoxButtonCD *)cbs;
	show_online = show_offline = FALSE;

	selection = d->item_data.item_index;
	process_arr[selection].is_select = 1;
	if (process_arr[selection].proc_state == 1)
		show_offline = TRUE;
	else
		show_online = TRUE;
	
	if (I_am_owner) {
		if (show_online == FALSE)
			XtVaSetValues(online_but, XmNsensitive, False, NULL);
		else
			XtVaSetValues(online_but, XmNsensitive, True, NULL);
		if (show_offline == FALSE)
			XtVaSetValues(offline_but, XmNsensitive, False, NULL);
		else
			XtVaSetValues(offline_but, XmNsensitive, True, NULL);
	}

	if (prop_dialogs != NULL)
		if (XtIsManaged(prop_form) == TRUE)
			prop_cb(container, NULL);

}

/*
 * onlin_cb() -- search the proc_arr[] for the selected item(s). Call
 *		 p_online() to put the processors on line.
 *		 
 *		 If fails, display the information dialog to let the
 *		 user konws which processors fail.
 *		 Otherwise, update the status_line in the base window.
 */

void
online_cb(w, client_data, cbs) 
Widget	w;
int	client_data;
XmAnyCallbackStruct	*cbs;
{
	Pixmap		pixmap;
	char		tmpstr[BUF_SIZE];
	int		i;
	DmObjectPtr	optr;
	MyItem		*item;

	infostr[0] = '\0';

	for (i = 0; i < nproc_conf; i ++) {
		/*if (process_arr[i].is_select  == 1) { */
		if (items[i].set == True) {
			if (process_arr[i].proc_state == P_OFFLINE) {
				if (p_online((processorid_t)i, P_ONLINE) >= 0) {
					/* change the icon */
					online_obj.fcp = &online_fclass;
					/* items->obj = (XtArgVal)&online_obj; */
					ExmVaFlatSetValues(container,
						i, XmNobjectData, (XtArgVal)&online_obj,
						0);
					ExmFlatRefreshItem(container,
						i, True);	
					process_arr[i].proc_state = P_ONLINE;
					update_status();
				}
				else {
					sprintf(tmpstr, mygettxt(string_cannotonline),
						i);
					strcat(infostr, strcat(tmpstr, "\n"));
				}
			}
			/* else just silently skipped those that already
			 * online, since no action is needed for them .
			 */
			process_arr[i].is_select = 0;
		}
	}

	/* reset the show_online and it will be set properly on the 
	 * next call to do_select.
	 */
	show_online = FALSE;

	if (strlen(infostr) > 0)
		InfoMsg(toplevel, infostr);

/* WHY need to call this ??
	if (prop_dialogs != NULL)
		if (XtIsManaged(prop_form) == TRUE)
			prop_cb(container,NULL);
*/

}

void
offline_cb() {
	Pixmap		pixmap;
	char		tmpstr[BUF_SIZE];
	int		i;
	DmObjectPtr	optr;

	infostr[0] = '\0';
	for (i = 0; i < nproc_conf; i++) {
		/* if (process_arr[i].is_select == 1) { */
		if (items[i].set == True) {
			if (process_arr[i].proc_state == P_ONLINE) {
				if (p_online((processorid_t)i, P_OFFLINE) >= 0) {
					offline_obj.fcp = &offline_fclass;
					/* items->obj = (XtArgVal)&offline_obj; */
					ExmVaFlatSetValues(container, i,
						XmNobjectData, (XtArgVal)&offline_obj, 0);
					ExmFlatRefreshItem(container,
						i, True);	
					process_arr[i].proc_state = P_OFFLINE;
					update_status();
				}
				else {
					sprintf(tmpstr, mygettxt(string_cannotoffline), i);
					strcat(infostr, strcat(tmpstr, "\n"));
				}
			}
			/*
			else {
				sprintf(tmpstr, mygettxt(string_alreadyoffline),
					i);
				strcat(infostr, strcat(tmpstr, "\n"));
			}
			*/
			/* reset this so next time it will work */
			process_arr[i].is_select = 0;
		}
	}

	/* reset the show_offline so it can be set correctly in do_select() */
	show_offline = FALSE;

	if (strlen(infostr) > 0)
		InfoMsg(toplevel, infostr);

/* WHY call this

	if (prop_dialogs != NULL)
		if (XtIsManaged(prop_form) == TRUE)
			prop_cb(container,NULL);
*/

}


/*
 * do_prop() -- called when the OK button is pressed.
 *		All it does is popdown the window.		
 */

void
do_prop(w, prop_dialogs, cbs)
Widget	w, prop_dialogs;
XmAnyCallbackStruct	*cbs;
{
	XtUnmanageChild(prop_form);
	XtPopdown(prop_dialogs);
	ExmVaFlatSetValues (selected_icon, selection, XmNsensitive, True, NULL);
}

void
dblclick_cb(w, client_data, cbs)
Widget 	w;
XtPointer	client_data;
XtPointer	cbs;
{
	ExmFIconBoxButtonCD	*d = (ExmFIconBoxButtonCD *)cbs;
	int			i;

	selected_icon = w;
	selection = d->item_data.item_index;
	process_arr[selection].is_select = 1;
	ExmVaFlatSetValues (selected_icon, selection, XmNsensitive, False, NULL);
	prop_cb(w, NULL);
}

/*
 * prop_cb() -- display the properties of the selected processor.
 *
 */

void
prop_cb(w, cbs)
Widget		w;
XtPointer	cbs;
{
	static Widget	prop_rows, prop_rc;
	static Widget	proclabel, procid, typelabel, type, clocklabel, floatl;
	static Widget	clock, floatlabel, iobuslabel, iobus;
	static Widget	procid_rc, clock_rc, iobus_rc, float_rc, type_rc; 
	static Widget	statelabel, state, state_rc;
	extern Widget	CreateActionArea();
	Widget		OK, HELP;
	void		do_prop();
	XmString	string;
	char		tmpstr[BUF_SIZE];
	int		i;

	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char 		* mne;
	KeySym			ks;

	processor_info_t	p_info;

	if (processor_info((processorid_t)selection, &p_info) < 0) {
		sprintf(tmpstr, mygettxt(ERR_cantGetProcInfo), selection);
		InfoMsg(toplevel, tmpstr);
		return;
	}

	if (!prop_dialogs) {
		prop_dialogs = XmCreateDialogShell(XtParent(w), 
			"prop_dialogs", NULL, 0);
		XtVaSetValues(prop_dialogs,
			XmNtitle, mygettxt(title_properties),
			NULL);
		prop_form = XmCreateMessageBox(prop_dialogs, 
			"prop_controlarea", NULL, 0);
		XtUnmanageChild(XmMessageBoxGetChild(prop_form,
			XmDIALOG_CANCEL_BUTTON));
		OK = XmMessageBoxGetChild(prop_form,
			XmDIALOG_OK_BUTTON);
		HELP = XmMessageBoxGetChild(prop_form,
			XmDIALOG_HELP_BUTTON);

		string = XmStringCreateSimple(mygettxt(label_ok));
		XtVaSetValues(prop_form,
			XmNokLabelString, string,
			NULL);

		XmSTR_N_MNE(label_ok, mnemonic_ok, mne_info[0], 
			DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
		XtVaSetValues(OK, XmNmnemonic, ks, NULL);
		XmStringFree(string); 

		/* help */
		XmSTR_N_MNE(label_help, mnemonic_help, mne_info[1],
			DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
		XmStringFree(string);

		XtVaSetValues(HELP, XmNmnemonic, ks, NULL);

		XtAddCallback(prop_form, XmNokCallback,
			(void(*)())do_prop, prop_dialogs);
		XtAddCallback(prop_form, XmNhelpCallback,
			(void(*)())mphelp_cb, (XtPointer)&PropHelp);
		prop_rc = XtVaCreateManagedWidget("prop_controlarea",
			xmRowColumnWidgetClass, prop_form,
			XmNorientation, XmVERTICAL,
			NULL);
		prop_rows = XtVaCreateManagedWidget("prop_rows",
			xmRowColumnWidgetClass, prop_rc,
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNnumColumns, 6,
			XmNisAligned, True,
			XmNentryAlignment, XmALIGNMENT_END,
			NULL);

		string = XmStringCreateSimple(mygettxt(label_pid));
		proclabel = XtVaCreateManagedWidget("proclabel", 
			xmLabelWidgetClass, prop_rows,
			XmNlabelString, string,
			NULL);
		XmStringFree(string);
		procid_rc = XtVaCreateManagedWidget("procid_rc",
			xmRowColumnWidgetClass, prop_rows,
			NULL);		
		sprintf(tmpstr, "%d", selection);
		string = XmStringCreateSimple(tmpstr);
		procid = XtVaCreateManagedWidget("procid",
			xmLabelWidgetClass, procid_rc,
			XmNlabelString, string,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
		XmStringFree(string);
		string = XmStringCreateSimple(mygettxt(label_state));
		statelabel = XtVaCreateManagedWidget("statelabel",
			xmLabelWidgetClass, prop_rows,
			XmNlabelString, string,
			NULL);
		XmStringFree(string);
		state_rc = XtVaCreateManagedWidget("state_rc",
			xmRowColumnWidgetClass, prop_rows,
			NULL);
		if (process_arr[selection].proc_state == 1)
			string = XmStringCreateSimple(mygettxt(label_OnLine));
		else
			string = XmStringCreateSimple(mygettxt(label_OffLine));
		state = XtVaCreateManagedWidget("state",
			xmLabelWidgetClass, state_rc,
			XmNlabelString, string,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
		XmStringFree(string);
				
		string = XmStringCreateSimple(mygettxt(label_proctype));
		typelabel = XtVaCreateManagedWidget("typelabel",
			xmLabelWidgetClass, prop_rows,
			XmNlabelString, string,
			NULL);
		XmStringFree(string);
		type_rc = XtVaCreateManagedWidget("type_rc",
			xmRowColumnWidgetClass, prop_rows,
			NULL);
		strcpy(tmpstr, p_info.pi_processor_type);
		string = XmStringCreateSimple(tmpstr);
		type = XtVaCreateManagedWidget("type",
			xmLabelWidgetClass, type_rc,
			XmNlabelString, string,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
		XmStringFree(string);
/*** Remove in sbird. In the future release when UNIX support this, 
 *** I should put it back.

		string = XmStringCreateSimple(mygettxt(label_clockspeed));
		clocklabel = XtVaCreateManagedWidget("clock label",
			xmLabelWidgetClass, prop_rows,
			XmNlabelString, string,
			NULL);
		XmStringFree(string);
		clock_rc = XtVaCreateManagedWidget("clock_rc",
			xmRowColumnWidgetClass, prop_rows,
			NULL);
		sprintf(tmpstr, "%d", p_info.pi_clock);
		strcat(tmpstr, " MHz");
		string = XmStringCreateSimple(tmpstr);
		clock = XtVaCreateManagedWidget("clock",
			xmLabelWidgetClass, clock_rc,
			XmNlabelString, string,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
		XmStringFree(string);
***/
		string = XmStringCreateSimple(mygettxt(label_floatpttype));
		floatlabel = XtVaCreateManagedWidget("floatlabel",
			xmLabelWidgetClass, prop_rows,
			XmNlabelString, string,
			NULL);
		XmStringFree(string);
		float_rc = XtVaCreateManagedWidget("float_rc",
			xmRowColumnWidgetClass, prop_rows,
			NULL);
		strcpy(tmpstr, p_info.pi_fputypes);
		string = XmStringCreateSimple(tmpstr);
		floatl = XtVaCreateManagedWidget("float",
			xmLabelWidgetClass, float_rc,
			XmNlabelString, string,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
		XmStringFree(string);

		XtManageChild(prop_form);
		mne_info[0].w = OK;
		mne_info[0].cb = (XtCallbackProc)do_prop;
		mne_info[0].cd = (XtPointer)prop_dialogs;

		mne_info[1].w = HELP;
		mne_info[1].cb = (XtCallbackProc)mphelp_cb;
		mne_info[1].cd = (XtPointer)&PropHelp;

		/* register the mnemonics */
		REG_NME(prop_dialogs, mne_info, 2);

	}
	else {
		sprintf(tmpstr, "%d", selection);
		string = XmStringCreateSimple(tmpstr);
		XtVaSetValues(procid, XmNlabelString, string, NULL);
		XmStringFree(string);
		if (process_arr[selection].proc_state == 1)
			string = XmStringCreateSimple(mygettxt(label_OnLine));
		else
			string = XmStringCreateSimple(mygettxt(label_OffLine));
		XtVaSetValues(state, XmNlabelString, string, NULL);	
		XmStringFree(string);
		strcpy(tmpstr, p_info.pi_processor_type);
		string = XmStringCreateSimple(tmpstr);
		XtVaSetValues(type, XmNlabelString, string, NULL);
		XmStringFree(string);
/**** REMOVE
		sprintf(tmpstr, "%d", p_info.pi_clock);
		strcat(tmpstr, " MHz");
		string = XmStringCreateSimple(tmpstr);
		XtVaSetValues(clock, XmNlabelString, string, NULL);
		XmStringFree(string);
****/
		strcpy(tmpstr, p_info.pi_fputypes);
		string = XmStringCreateSimple(tmpstr);
		XtVaSetValues(floatl, XmNlabelString, string, NULL);
		XmStringFree(string);

		/* More to come once the processor_info() is fixed. */	

		XtManageChild(prop_form);
	}
	
	XtPopup(prop_dialogs, XtGrabNone);
}

void
exit_cb() {
	exit(0);
}

DmObjectPtr
add_object(int count, Process_t	*proc)
{
	MyItem	* item;
	int	i, j, k;

	items = (MyItem *)XtMalloc(sizeof(MyItem) * count);
	item = items;

	for (i=0; i < count; i++) {
		char		buf[100];
		XmString	str;
		sprintf(buf, "%d", i);
		str = XmStringCreateLocalized(buf);

		/* Store the label in the internal
		 * representation that Motif uses.
		 */

		if (i < NUM_COLS) {
			item->x = (XtArgVal)(i * GRID_WD)+5;	
			item->y	= (XtArgVal)0 + 5;
		}
		else {
			item->x = (XtArgVal)((i % NUM_COLS) * GRID_WD) + 5;
			item->y = (XtArgVal)GRID_HI + 5;
		}

		/* for centering the item in the grid */
		item->width 	= (XtArgVal)GRID_WD-10;
		item->height 	= (XtArgVal)GRID_HI-10;

		item->set 	= (XtArgVal)False;
		item->sensitive	= (XtArgVal)True;
		if (proc[i].proc_state == P_ONLINE) {
			online_obj.fcp = &online_fclass;
			item->obj 	= (XtArgVal)&online_obj;
		}
		else {
			offline_obj.fcp = &offline_fclass;
			item->obj 	= (XtArgVal)&offline_obj;
		}
		item->label	= (XtArgVal)_XmStringCreate(str);
		XmStringFree(str);
		item++;
	}
}

void
new_fileclass(char *type)
{
        static char *iconpath;
        DmFclassPtr fcp;
        char icon[128];

        if (!strcmp(type, OFFLINE)) {
                sprintf(icon, "%s", OFFLINE_ICON);
		offline_fclass.glyph = DmGetPixmap(XtScreen(toplevel), icon);
		/* DmMaskPixmap(toplevel, offline_fclass.glyph); */
	}
        else {
                sprintf(icon, "%s", ONLINE_ICON);
		online_fclass.glyph = DmGetPixmap(XtScreen(toplevel), icon);
		/* DmMaskPixmap(toplevel, online_fclass.glyph); */
	}
}


/*
 * mkproclist() -- create the processor icons in the base window.
 *
 *		Call sysconf() for the total number of processor 
 *		configured in the system.
 *
 *		Call online() to check for the status of each processor.
 *		Store the information (pid, state, IconGadget handle and
 *		selection flag) in the global structure process_arr[].
 */
Boolean
mkproclist() 
{
	Pixel		fg, bg;
	Pixmap		pixmap;
	char		*bm, tmplabel[8];
	XmString	label;
	void		do_select();
	char		tmpstr[BUF_SIZE];
	int		i, status;
	long		sysconf(), nproc_onln;


	if ((nproc_conf = sysconf(_SC_NPROCESSORS_CONF)) == -1) {
		sprintf(tmpstr, mygettxt(ERR_cantGetNumProc));
		TopMsg(tmpstr);
		return False;
	}

	/* create the flat icon box stuff */

	new_fileclass(ONLINE);
	new_fileclass(OFFLINE);	

	
	process_arr = (Process_t *)XtMalloc(nproc_conf * sizeof(Process_t));
	
	for (i=0; i<(int)nproc_conf; i++) {

		process_arr[i].proc_id = i;
		if ((status = online(i, P_QUERY)) == -1) {
			sprintf(tmpstr, mygettxt(ERR_cantGetOnlineInfo), i);
			TopMsg(tmpstr);
			return False;
		}

		if (status == P_ONLINE) {
			process_arr[i].proc_state = P_ONLINE;
		}
		else {
			process_arr[i].proc_state = P_OFFLINE;
		}
		
		/* clear the is_select field */
		process_arr[i].is_select = 0;
	}
	/* create the objects inside the flat icon box */
	add_object(nproc_conf, process_arr);	
	return True;
}

void
AlignIcons()
{
	
	DmItemPtr	item;
	int		maxlen = 0, i;
	Dimension	width;
	Dimension	margin = XmConvertUnits(container, XmHORIZONTAL,
					Xm100TH_POINTS, ICON_MARGIN * 100, 
					XmPIXELS);
	Dimension	pad = XmConvertUnits(container, XmHORIZONTAL,
					Xm100TH_POINTS, INTER_ICON_PAD * 100,
					XmPIXELS);

	Cardinal	nitems;
	WidePosition	x = margin;
	WidePosition	y = margin;
	WidePosition	center_x;
	WidePosition	next_x;
	Dimension	grid_width;
	Dimension	row_height;
	Dimension	item_width;


	XtVaGetValues(container, XmNnumItems, &nitems, NULL);
	XtVaGetValues(container, XmNwidth, &width, NULL);

	/* Compute row height and grid width outside of loop */
	row_height = GRID_HI;
	grid_width = GRID_WD;
	center_x = x + (grid_width/2);

/*
	for (i = 0, item = itp; i < nitems;i++, item++)
		item-> x = item-> y = 0;
*/

	for (i = 0, item = itp; i < nitems;i++, item++)
		if (ITEM_MANAGED(item)) {
			item_width = ITEM_WIDTH(item);
again:
			/* Horiz: centered */
			center_x = x + (grid_width /2);
			next_x = center_x - (item_width / 2);

			/* Wrap now if item will extend beyond "wrap" width */
			if ((x != margin) &&
				((Dimension)(x + grid_width) > width))
			{
				x = margin;
				y += row_height;
				goto again;
			}
			/* Vert: bottom justified */
			item->y = VertJustifyInGrid(y, ITEM_HEIGHT(item), 
				row_height);
			item->x = next_x;

			x += grid_width;
			center_x += grid_width / 2;	/* next center */
		}

	XtVaSetValues(container, XmNitemsTouched, True, NULL);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	char		atom[BUF_SIZE] = "ProcSetup";
	Window		another_window;
	static Widget	menubar, rc, rcb, main_rc, form2, swin;
	static Widget	help_wid, frame;
	Widget		BuildMenu();
	char		tmpstr[BUF_SIZE];
	long		sysconf(), nproc_onln;
	Arg		arg[10];
	int		i;

	Pixmap		icon, iconmask;

	DmMnemonicInfoRec	mne_info[MI_TOTAL];
	unsigned char		*mne;
	KeySym			ks;
	unsigned long		op;
	XmString		string;

	XtSetLanguageProc(NULL, NULL, NULL);

	/* Initialize toolkit */
	toplevel = XtVaAppInitialize(&app, "Processor_Administration",
		NULL, 0, &argc, argv, NULL, NULL);

	DtiInitialize(toplevel);	
	if (glyph == NULL)
		glyph = DmGetPixmap(XtScreen(toplevel), "proc.stp48");
	
	if (glyph) {
		icon = glyph->pix;
		iconmask = glyph->mask;
	}
	else
		icon = iconmask = (Pixmap)0;

	XtVaSetValues(toplevel, 
		XmNtitle, mygettxt(title_procadmin), 
		XtNiconPixmap, (XtArgVal) icon,
		XtNiconMask, (XtArgVal) iconmask,
		XtNiconName, (XtArgVal) mygettxt(TXT_iconName),
		NULL);

	/* main window contains a MenuBar and a container displaying
	 * different processors on the system.
	 */
	
	main_w = XtVaCreateManagedWidget("form",
		xmFormWidgetClass, toplevel,
		NULL);
	
	/* Create a MenuBar that contains two pulldown menus */
	menubar = XmCreateMenuBar(main_w, "menubar", NULL, 0);
	BuildMenu(menubar, XmMENU_PULLDOWN, label_actions, mnemonic_actions, actions_items);
	help_wid = BuildMenu(menubar, XmMENU_PULLDOWN, label_help, mnemonic_help, help_items);
	XtVaSetValues(menubar, XmNmenuHelpWidget, help_wid, NULL);
	XtVaSetValues(menubar, XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, NULL);

	XtManageChild(menubar);


	/* Add two buttons for online and offline next to the container */

	sprintf(tmpstr, mygettxt(label_online));	
	online_but = XtVaCreateManagedWidget(tmpstr,
		xmPushButtonWidgetClass, main_w,
		NULL);
	XmSTR_N_MNE(label_online, mnemonic_online2, mne_info[0],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(online_but, XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menubar,
		XmNrightAttachment, XmATTACH_FORM,
		XmNmnemonic, ks,
		NULL);
	
	XtAddCallback(online_but, XmNactivateCallback, 
		(void(*)())online_cb, NULL);	
	XmStringFree(string);
	mne_info[0].w = online_but;
	mne_info[0].cb = (XtCallbackProc)online_cb;

	sprintf(tmpstr, mygettxt(label_offline));
	offline_but = XtVaCreateManagedWidget(tmpstr,	
		xmPushButtonWidgetClass, main_w,
		NULL);
	XmSTR_N_MNE(label_offline, mnemonic_offline2, mne_info[1],
		DM_B_MNE_ACTIVATE_CB|DM_B_MNE_GET_FOCUS);
	XtVaSetValues(offline_but, XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, online_but,
		XmNrightAttachment, XmATTACH_FORM,
		XmNmnemonic, ks,
		NULL);
	XtAddCallback(offline_but, XmNactivateCallback, 
		(void(*)())offline_cb, NULL);	
	XmStringFree(string);
	mne_info[1].w = offline_but;
	mne_info[1].cb = (XtCallbackProc)offline_cb;

	status = XtVaCreateManagedWidget("status",
		xmLabelWidgetClass, main_w,
		NULL);
	XtVaSetValues(status, XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	frame = XtVaCreateManagedWidget("frame",
		xmFrameWidgetClass, main_w,
		NULL);


	XtVaSetValues(frame, XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menubar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, online_but,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, status,
		NULL);

	form2 = XtVaCreateManagedWidget("form",
		xmFormWidgetClass, frame,
		NULL);
/*

	rc = XtVaCreateManagedWidget("rc",
		xmRowColumnWidgetClass, frame,
		XmNorientation, XmHORIZONTAL,
		NULL);
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNscrollBarDisplayPolicy, XmAS_NEEDED,

*/
	swin = XtVaCreateManagedWidget("swin",
		xmScrolledWindowWidgetClass, form2,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNshadowThickness, 0,
		NULL);

	XtVaSetValues(swin, XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
/* 
	XtVaSetValues(swin, XmNwidth, 200,
		XmNheight, 100, NULL);
*/

	if (mkproclist() == False)
		return;
		
	if (nproc_conf == -1) {
		/* no object(processor) on the base window */
		TopMsg(mygettxt(ERR_noItems));
		return;
	}

	container = XtVaCreateManagedWidget(
		"container", exmFlatIconBoxWidgetClass, swin,
		XmNitemFields,			fields,
		XmNnumItemFields,		XtNumber(fields),
		XmNitems,			items,
		XmNnumItems,			nproc_conf,
		XmNdrawProc,			ExmFIconDrawProc,
		XmNexclusives,			False,
		XmNdblSelectProc,		dblclick_cb,
		XmNselectProc,			do_select,
		XmNmovableIcons,		False,
		XmNgridColumns,			NUM_COLS,
		XmNgridRows,			NUM_ROWS,
		XmNgridWidth,			GRID_WD,
		XmNgridHeight,			GRID_HI,
		NULL);

	/* numItems=0, items=NULL in XtCreate(). XtStringExtent to get fontlist
	   XtSetValues on numItems and items; get the dynamic fontlist and
	   recaluate the width and height of the item*/

	update_status();

	I_am_owner = _IsOwner("ProcSetup");

	XtVaSetValues(online_but,
		XmNsensitive, (XtArgVal)I_am_owner, NULL);	
	XtVaSetValues(offline_but,
		XmNsensitive, (XtArgVal)I_am_owner, NULL);	

	REG_NME(toplevel, mne_info, 2);

	XtRealizeWidget(toplevel);
	another_window = DtSetAppId(XtDisplay(toplevel), 
		XtWindow(toplevel), atom);
	if (another_window != None) {
		/* We are already running. Bring that window to the top 
		 * and die. 
		 */
		XMapWindow(XtDisplay(toplevel), another_window);
		XRaiseWindow(XtDisplay(toplevel), another_window);
		XFlush(XtDisplay(toplevel));
		exit(0);
	}
	XtAppMainLoop(app);
}
