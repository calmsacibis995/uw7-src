#ident	"@(#)dtadmin:internet/nis_server.c	1.14"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>
#include <wait.h>
#include <sys/secsys.h>
#include <errno.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/List.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#include "route_util.h"
#include "nis_server.h"

/* function prototypes */
/* external -- these should be in a header file somewhere */
extern Widget	createActionArea(Widget, ActionAreaItems *, int, Widget, DmMnemonicInfo, int);
extern void	createMsg(Widget, msgType, char *, char *);
extern Widget	crt_inputlabel(Widget, char *, int);
extern int	crt_inet_addr(Widget, char *, int, inetAddr *);
extern char *	mygettxt(char *);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern void	displayFooterMsgEH(Widget, MiniHelp_t *, XFocusChangeEvent *);
extern int	r_decor(Widget);

/* static */
/* callback fcns */
static void	okCB(Widget, XtPointer, XtPointer);
static void	resetCB(Widget, XtPointer, XtPointer);
static void	cancelCB(Widget, XtPointer, XtPointer);
static void	helpCB(Widget, XtPointer, XtPointer);
static void	addCB(Widget, XtPointer, XtPointer);
static void	modifyCB(Widget, XtPointer, XtPointer);
static void	deleteCB(Widget, XtPointer, XtPointer);
static void	editCB(Widget, XtPointer, XtPointer);
static void	selectCB(Widget, XtPointer, XtPointer);

/* other fcns */
static int		getInitialValues(void);
Dimension		getMaxWidth(char **, WidgetClass, String, Widget);
static void		cleanupNIS(void);
static void		cleanupNISList(NisServerList_t *);
static void		modifyButtons(Boolean, Boolean, Boolean);
static void		addCallbacks(void);

/* local definitions */
typedef enum {
	State1,		/* server entry and list item selected */
	State2,		/* server entry and no list item selected */
	State3,		/* no server entry and list item selected */
	State4		/* no server entry and no list item selected */
} NisState_t;

/* globals */
char *	labels[] = {
	TXT_nisServer,
	TXT_nisDomain,
	" ",
	NULL
};

char *	buttons[] = {
	TXT_add,
	TXT_mod,
	TXT_delete,
	NULL
};

char *	mnemonics[] = {
	MNE_add,
	MNE_modify,
	MNE_delete,
	NULL
};

MiniHelp_t	nisHelp[] = {
	{ MiniHelp_nis1,	NULL },
	{ MiniHelp_nis2,	NULL }
};

HelpText	NISHelp = { NULL, NULL, "190" };

static ActionAreaItems	actions[] = {
	{ TXT_OK, MNE_OK, okCB, NULL },
	{ TXT_Reset, MNE_Reset, resetCB, NULL },	
	{ TXT_Cancel, MNE_Cancel, cancelCB, NULL },	
	{ TXT_Help, MNE_Help, helpkeyCB, &NISHelp }
};

NisServer_t	nisWin;
NisValues_t	nisValues;
NisState_t	nisState;

void
createNisAccess(Widget w, XtPointer client_data, XtPointer call_data)
{
	static Widget		topForm;
	Widget			nisRC;
	NisServerList_t *	ptr;
	Dimension		maxWidth;
	Widget			nisForm;
	Arg			args[10];
	Cardinal		n;
	int			i;
	int			ret;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;


	if (!nisWin.nisPopup) {

		nisState = State4;

		ret = getInitialValues();
		switch (ret) {
			case 0	:
				break;
			case 1	:
				createMsg(
					hi.net.common.toplevel,
					ERROR,
					mygettxt(ERR_nisInit),
					mygettxt(TXT_nisView)
				);
				return;
			case 2	:
				return;
			default	:
				break;
		}

		nisWin.nisPopup = XtVaCreatePopupShell(
			"popup", xmDialogShellWidgetClass, XtParent(w),
			NULL
		);
		XtVaSetValues(
			nisWin.nisPopup,
			XmNtitle,		mygettxt(TXT_nisView),
			XmNallowShellResize,	True,
			XmNdeleteResponse,	XmUNMAP,
			NULL
		);

		r_decor(nisWin.nisPopup);
		
		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, nisWin.nisPopup,
			NULL);

		nisRC = XtVaCreateManagedWidget(
			"rowcolumn", xmRowColumnWidgetClass, topForm,
			NULL
		);
		XtVaSetValues(nisRC,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtAddCallback(nisRC, XmNhelpCallback, helpkeyCB, &NISHelp);

		maxWidth = getMaxWidth(
			labels,
			xmLabelWidgetClass,
			XmNlabelString,
			nisRC
		);

		nisWin.nisDomain = crt_inputlabel(
			nisRC, TXT_nisDomain, maxWidth
		);
		if (nisValues.domain) {
			XtVaSetValues(
				nisWin.nisDomain,
				XmNvalue,	nisValues.domain,
				NULL
			);
		}
		nisWin.nisServer = crt_inputlabel(
			nisRC, TXT_nisServer, maxWidth
		);
		(void)crt_inputlabellabel(nisRC, "", maxWidth, TXT_nisServers);

		nisForm = XtVaCreateManagedWidget(
			"form", xmFormWidgetClass, nisRC,
			NULL
		);

		for (i = 0; i < (XtNumber(buttons) - 1); i++) {
			XtWidgetGeometry	g;
			Dimension		d;
			XmString		xms;
	
			g.request_mode = CWWidth;

			d = getMaxWidth(
				buttons,
				xmPushButtonWidgetClass,
				XmNlabelString,
				nisRC
			);

			strcpy(tmpstr, mygettxt(buttons[i]));
			nisWin.nisButton[i] = XtVaCreateManagedWidget(
				tmpstr, xmPushButtonWidgetClass, nisForm,
				XmNwidth,		d,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNleftOffset,		maxWidth - d,
				XmNsensitive,		False,
				NULL
			);
			XmSTR_N_MNE(
				buttons[i],
				mnemonics[i],
				mneInfo[i],
				DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
			);
			XmStringFree(mneString);
			XtVaSetValues(
				nisWin.nisButton[i],
				XmNmnemonic,	mneks,
				NULL
			);
			if (i == 0) {
				XtVaSetValues(
					nisWin.nisButton[i],
					XmNtopAttachment,	XmATTACH_FORM,
					NULL
				);
			} else {
				XtVaSetValues(
					nisWin.nisButton[i],
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		nisWin.nisButton[i-1],
					NULL
				);
			}
			if (i == 2) {
				XtVaSetValues(
					nisWin.nisButton[i],
					XmNbottomAttachment,	XmATTACH_WIDGET,
					NULL
				);
			}
		}

		n = 0;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNleftOffset, (maxWidth + 2)); n++;
		XtSetArg(args[n], XmNvisibleItemCount, 4); n++;
		XtSetArg(args[n], XmNvisibleItemCount, 4); n++;
		nisWin.nisList = XmCreateScrolledList(
			nisForm, "list", args, n
		);
		XtManageChild(nisWin.nisList);

		ptr = nisValues.serverList;
		while (ptr) {
			XmString	xms;

			xms = XmStringCreateLocalized(ptr->server);
			XmListAddItemUnselected(
				nisWin.nisList,
				xms,
				0
			);
			XmStringFree(xms);
			ptr = ptr->next;
		}

		(void)XtVaCreateManagedWidget(
			"separator", xmSeparatorWidgetClass, nisRC,
			NULL
		);
		mneInfo[ADD].w = nisWin.nisButton[ADD],
		mneInfo[ADD].cb = (XtCallbackProc)addCB;
		mneInfo[MODIFY].w = nisWin.nisButton[MODIFY],
		mneInfo[MODIFY].cb = (XtCallbackProc)modifyCB;
		mneInfo[DELETE].w = nisWin.nisButton[DELETE],
		mneInfo[DELETE].cb = (XtCallbackProc)deleteCB;
		createActionArea(
			nisRC,
			actions, 
			XtNumber(actions),
			nisWin.nisDomain,
			mneInfo,
			3
		);
		(void)XtVaCreateManagedWidget(
			"separator", xmSeparatorWidgetClass, nisRC,
			NULL
		);
		nisWin.nisStatus = XtVaCreateManagedWidget(
			"label", xmLabelWidgetClass, nisRC,
			NULL);
		setLabel(nisWin.nisStatus, "");

		/* add mini-help */
		nisHelp[0].widget = nisWin.nisStatus;
		nisHelp[1].widget = nisWin.nisStatus;
		XtAddEventHandler(
			nisWin.nisDomain,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&nisHelp[0]
		);
		XtAddEventHandler(
			nisWin.nisServer,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&nisHelp[1]
		);

		addCallbacks();
		XtVaSetValues(topForm,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[2].widget,
			NULL);

		REG_MNE(nisWin.nisPopup, mneInfo, XtNumber(actions) + 3);
	}	

	XtManageChild(topForm);
	XtManageChild(nisWin.nisPopup);
}

/*
 *	return: 0 if success
 *		1 if failure
 *		2 if nis is not installed
 */
static int
getInitialValues(void)
{
	struct stat		st;
	FILE *			cmdpipe;
	FILE *			fd;
	char			buf[BUFSIZ];
	char *			cret;
	int			ret;
	NisServerList_t *	line;
	NisServerList_t **	nextLine;

	nisValues.firstTime = False;
	/* first check to see if nis package is installed */
	if ((ret = stat("/var/sadm/pkg/nis", &st)) < 0 && errno == ENOENT) {
		createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_nisStartUp1), mygettxt(TXT_nisView));
		return 2;
	}
	/* get NIS domain (if it exists) */
	if ((cmdpipe = popen(YPDOMCMD, "r")) == NULL) {
		return 1;
	}
	if ((cret = fgets(buf, BUFSIZ, cmdpipe)) == NULL) {
		return 1;
	}
	pclose(cmdpipe);
	if (*buf == '\n') {
		/* NIS has not yet been set up */
		nisValues.firstTime = True;
		return 0;
	}

	nisValues.domain = strndup(buf, (strlen(buf) - 1));

	sprintf(buf, YPSERVERS, nisValues.domain);
	if ((ret = stat(buf, &st)) < 0 && errno == ENOENT) {
		return 0;
	}

	if ((fd = fopen(buf, "r")) == NULL) {
		return 1;
	}

	nextLine = &nisValues.serverList;
	while (fgets(buf, BUFSIZ, fd)) {
		line = (NisServerList_t *)calloc(1, sizeof(NisServerList_t));
		if (!line) {
			fclose(fd);
			return 1;
		}
		*nextLine = line;
		nextLine = &line->next;
		line->server = strndup(buf, (strlen(buf) - 1));
		line->next = NULL;
	}

	fclose(fd);
	return 0;
}

static void
okCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	char		buf[BUFSIZ];
	int		ret;
	FILE *		cmdpipe;
	FILE *		fd;
	Boolean		didSetuid;
	uid_t		origUid, privUid;
	char *		newDomain;
	int		newServerCnt;
	XmStringTable	newServerList;
	char *		newServer;

	/* extract information from form */
	newDomain = XmTextFieldGetString(nisWin.nisDomain);
	newServer = XmTextFieldGetString(nisWin.nisServer);
	XtVaGetValues(
		nisWin.nisList,
		XmNitemCount,	&newServerCnt,
		XmNitems,	&newServerList,
		NULL
	);
	if (*newDomain == '\0' || (*newServer == '\0' && newServerCnt == 0)) {
		createMsg(
			hi.net.common.toplevel,
			WARN,
			mygettxt(ERR_nisIncomplete),
			mygettxt(TXT_nisView)
		);
		XtFree(newDomain);

		return;
	}
	/*
	 * get root administrator user and current id,
	 */
	origUid = getuid();
	privUid = secsys(ES_PRVID, 0);
	if ((privUid >= 0) && (origUid == privUid)) {
		didSetuid = False;
	} else {
		setuid(privUid);
		didSetuid = True;
	}

	if ((cmdpipe = popen("/usr/sbin/ypinit -c > /tmp/ypinit.out 2>&1", "w")) == NULL) {
		createMsg(
			hi.net.common.toplevel, 
			ERROR,
			mygettxt(ERR_nisErr1),
			mygettxt(TXT_nisView)
		);
		return;
	}
	sprintf(buf, "%s\n", newDomain);
	if (nisValues.firstTime) {
		/* no domain name */
		fputs(buf, cmdpipe);
		fputs("\n", cmdpipe);
	} else {
		if (strcmp(newDomain, nisValues.domain)) {
			/* different domain name */
			fputs("n\n", cmdpipe);
			fputs(buf, cmdpipe);
			fputs("\n", cmdpipe);
		} else {
			/* same domain name */
			fputs("\n", cmdpipe);
		}
	}
	/* write out servers */
	if (newServerCnt == 0) {
		fputs(newServer, cmdpipe);
	} else {
		for (i = 0; i < newServerCnt; i++) {
			char *	t;
	
			XmStringGetLtoR(newServerList[i], XmFONTLIST_DEFAULT_TAG, &t);
			if (i != (newServerCnt - 1)) {
				sprintf(buf, "%s\n", t);
				fputs(buf, cmdpipe);
			} else {
				fputs(t, cmdpipe);
			}
			XtFree(t);
		}
	}
	fputs("\n", cmdpipe);
	pclose(cmdpipe);

	XtFree(newDomain);
	XtFree(newServer);
	cleanupNIS();
	if (didSetuid) {
		setuid(origUid);
	}
	XtUnmanageChild(nisWin.nisPopup);
	nisWin.nisPopup = NULL;
}

static void
resetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	NisServerList_t *	ptr;

	XtVaSetValues(nisWin.nisDomain, XmNvalue, nisValues.domain, NULL);
	XtVaSetValues(nisWin.nisServer, XmNvalue, "", NULL);
	XmListDeleteAllItems(nisWin.nisList);
	ptr = nisValues.serverList;
	while (ptr) {
		XmString	xms;

		xms = XmStringCreateLocalized(ptr->server);
		XmListAddItemUnselected(nisWin.nisList, xms, 0);
		XmStringFree(xms);
		ptr = ptr->next;
	}
	nisState = State4;
	modifyButtons(False, False, False);
}

static void
cancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	cleanupNIS();
	XtUnmanageChild(nisWin.nisPopup);
	nisWin.nisPopup = NULL;
}

static void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	printf("help\n");
}

/*
 * a more efficient way to calculate the widths of objects. we only
 * have to create and destroy one widget.
 */
Dimension
getMaxWidth(char ** str, WidgetClass class, String resource, Widget w)
{
	Dimension	max;
	Widget		tmp;

	tmp = XtVaCreateWidget("tmp", class, w, NULL);
	max = 0;
	while (*str != NULL) {
		XmString		xms;
		Dimension		w;
		XtWidgetGeometry	g;

		xms = XmStringCreateLocalized(mygettxt(*str));
		XtVaSetValues(tmp, resource, xms, NULL);
		g.request_mode = CWWidth;
		XtQueryGeometry(tmp, NULL, &g);
		if (g.width > max) {
			max = g.width;
		}
		XmStringFree(xms);
		str++;
	}
	XtDestroyWidget(tmp);

	return max;
}

static void
cleanupNIS(void)
{
	if (nisValues.domain)
		free(nisValues.domain);
	if (nisValues.serverList) {
		cleanupNISList(nisValues.serverList);
	}
}

static void
cleanupNISList(NisServerList_t * ptr)
{
	if (ptr) {
		if (ptr->server) {
			free(ptr->server);
		}
		cleanupNISList(ptr->next);
		free(ptr);
	}
}

static void
modifyButtons(Boolean add, Boolean mod, Boolean del)
{
	XtSetSensitive(nisWin.nisButton[ADD], add);
	XtSetSensitive(nisWin.nisButton[MODIFY], mod);
	XtSetSensitive(nisWin.nisButton[DELETE], del);
}

static void
addCallbacks(void)
{
	XtAddCallback(
		nisWin.nisButton[ADD],
		XmNactivateCallback,
		addCB,
		NULL
	);
	XtAddCallback(
		nisWin.nisButton[MODIFY],
		XmNactivateCallback,
		modifyCB,
		NULL
	);
	XtAddCallback(
		nisWin.nisButton[DELETE],
		XmNactivateCallback,
		deleteCB,
		NULL
	);
	XtAddCallback(
		nisWin.nisServer,
		XmNvalueChangedCallback,
		editCB,
		NULL
	);
	XtAddCallback(
		nisWin.nisList,
		XmNbrowseSelectionCallback,
		selectCB,
		NULL
	);
}

static void
addCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmString	xms;
	char *		newServer;

	/* get value and add to list */
	newServer = XmTextFieldGetString(nisWin.nisServer);
	xms = XmStringCreateLocalized(newServer);
	XmListAddItemUnselected(nisWin.nisList, xms, nisValues.selectedItem);

	/* now blank out textfield and reset buttons */
	XtVaSetValues(nisWin.nisServer, XmNvalue, "", NULL);
	modifyButtons(False, False, False);
	nisState = State4;
	nisValues.selectedItem = 0;
	XmListDeselectAllItems(nisWin.nisList);

	XmStringFree(xms);
	XtFree(newServer);

	return;
}

static void
modifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmString	xms;
	int		cnt;
	int *		pos;
	char *		text;

	text = XmTextFieldGetString(nisWin.nisServer);
	XmListGetSelectedPos(nisWin.nisList, &pos, &cnt);
	xms = XmStringCreateLocalized(text);
	XmListReplaceItemsPos(nisWin.nisList, &xms, 1, *pos);

	XtVaSetValues(nisWin.nisServer, XmNvalue, "", NULL);
	modifyButtons(False, False, False);
	nisState = State4;
	nisValues.selectedItem = 0;
	XmListDeselectAllItems(nisWin.nisList);

	XmStringFree(xms);
	XtFree(text);
	XtFree((char *)pos);

	return;
}

static void
deleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	cnt;
	int *	pos;

	XmListGetSelectedPos(nisWin.nisList, &pos, &cnt);
	XmListDeletePos(nisWin.nisList, *pos);
	XtVaSetValues(nisWin.nisServer, XmNvalue, "", NULL);
	modifyButtons(False, False, False);
	nisState = State4;
	nisValues.selectedItem = 0;
	XmListDeselectAllItems(nisWin.nisList);

	XtFree((char *)pos);

	return;
}

static void
editCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *		text;
	int		length;
	XmString	xms;

	text = XmTextFieldGetString(nisWin.nisServer);
	length = strlen(text);
	switch (nisState) {
		case State1: {
			if (length == 0) {
				modifyButtons(False, True, True);
			} else {
				modifyButtons(True, True, True);
			}
			break;
		}
		case State2: {
			if (length == 0) {
				nisState = State4;
				modifyButtons(False, False, False);
			}
			break;
		}
		case State3: {
			if (length > 0) {
				nisState = State1;
				modifyButtons(False, True, True);
			}
			break;
		}
		case State4: {
			if (length > 0) {
				nisState = State2;
				modifyButtons(True, False, False);
			}
			break;
		}
		default:
			break;
	}
	XtFree(text);
	return;
}

static void
selectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *	cbs = (XmListCallbackStruct *)call_data;
	char *			text;

	XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &text);
	nisValues.selectedItem = cbs->item_position;
	XtVaSetValues(
		nisWin.nisServer,
		XmNvalue,	text,
		NULL
	);
	XtFree(text);
	nisState = State1;
	modifyButtons(False, True, True);
}
