#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/set_local_access.c	1.37"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/List.h>
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
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/MessageB.h>

#include <Dt/Desktop.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#include "route_util.h"

/* miscellaneous macros */
#define GOBBLEWHITE(p)		while (isspace(*p)) ++p
#define TRAVERSETOKEN(p)	while (*p != 0 && !isspace(*p)) ++p

/* function prototypes */
/* external */
extern Widget		createActionArea(Widget, ActionAreaItems *, int, Widget, DmMnemonicInfo, int);
extern Widget		crt_inputlabel(Widget, char *, int);
extern Widget		crt_inputradio(Widget, char *, int, radioList *, Boolean);
extern Widget		crt_inputlabellabel(Widget, char *, int, char *);
extern Widget		crt_radio(Widget, radioList *);
extern Widget		crt_rc_radio(Widget, radioList *);
extern XmString		createString(char *);
extern int		makePosVisible(Widget, int);
extern void		createMsg(Widget, msgType, char *, char *);
extern Boolean		isInDomainList(int, dnsList *);
extern void		helpkeyCB(Widget, XtPointer, XtPointer);
extern void		displayFooterMsgEH(Widget, MiniHelp_t *, XFocusChangeEvent *);
extern Dimension	getMaxWidth(char **, WidgetClass, String, Widget);
extern char *		mygettxt(char *);
extern int		r_decor(Widget);
extern Boolean		removeWhitespace(char *);
extern void		noSpacesCB(Widget, XtPointer, XtPointer);

/* static */
static void	okCB(Widget, XtPointer, XtPointer);
static void	applyCB(Widget, XtPointer, XtPointer);
static void	resetCB(Widget, XtPointer, XtPointer);
static void	cancelCB(Widget, XtPointer, XtPointer);
static void	entireCB(Widget, XtPointer, XtPointer);
static void	setUserLabel(accessRight);
static void	personalCB(Widget, XtPointer, XtPointer);
static void	nooneCB(Widget, XtPointer, XtPointer);
static void	selfCB(Widget, XtPointer, XtPointer);
static void	allCB(Widget, XtPointer, XtPointer);
static void	specificCB(Widget, XtPointer, XtPointer);
static void	selectionCB(Widget, XtPointer, XtPointer);
static void	addCB(Widget, XtPointer, XtPointer);
static void	modCB(Widget, XtPointer, XtPointer);
static void	delCB(Widget, XtPointer, XtPointer);
static void	loginIDValCB(Widget, XtPointer, XtPointer);
static void	systemNameFocusCB(Widget, XtPointer, XtPointer);
static void	parse(accessRight, securityList **);
static void	writeBack(char *, securityList *);
static Boolean	decideScreenLayout(void);
static void	deleteAllIdItems(idList *);
static void	deleteAllSecItems(securityList *);
static void	deleteMatchingItems(securityList *, char *);
static void	bailOut(void);
static void	setUserState(userAccess);
static void	generate(void);

/* globals */
static char *	title;
char		rhostsPath[MEDIUMBUFSIZE];
static Boolean	changesPending = False;
static Boolean	firstTime = True;

char *	topLabels[] = {
	TXT_ruaSysName,
	TXT_ruaRestrict,
	NULL
};

char *	bottomLabels[] = {
	TXT_Login,
	" ",
	NULL
};

enum { ADD, MODIFY, DELETE };

char *	laButtons[] = {
	TXT_add,
	TXT_mod,
	TXT_delete,
	NULL
};

char *	laButtonMnemonics[] = {
	MNE_add,
	MNE_modify,
	MNE_delete,
	NULL
};

HelpText	SLAHelp = { NULL, NULL, "30" };
MiniHelp_t	slaHelp[] = {
	{ MiniHelp_sla1,	NULL },
	{ MiniHelp_sla2,	NULL },
	{ MiniHelp_sla3,	NULL },
	{ MiniHelp_sla4,	NULL },
	{ MiniHelp_sla5,	NULL }
};

static ActionAreaItems actions[] = {
	{ TXT_OK,	MNE_OK,		okCB,		NULL },
	{ TXT_Apply,	MNE_Apply,	applyCB,	NULL },	
	{ TXT_Reset,	MNE_Reset,	resetCB,	NULL },	
	{ TXT_Cancel,	MNE_Cancel,	cancelCB,	NULL },	
	{ TXT_Help,	MNE_Help,	helpkeyCB,	&SLAHelp }
};
 
static radioItem	acc_radio[] = {
	{ TXT_ruaRestrictUser,	entireCB,	NULL	},
	{ TXT_ruaRestrictPers,	personalCB,	NULL	},
};

static radioItem	user_radio[] = {
	{ TXT_NoOne,	nooneCB,	NULL},
	{ TXT_Self,	selfCB,		NULL},
	{ TXT_All,	allCB,		NULL},
	{ TXT_Specific, specificCB,	NULL},
};

radioList	acc_radioList = {
	acc_radio,
	NUM_ACCESS_RIGHT,
	XmVERTICAL,
	NULL
};

radioList	user_radioList = {
	user_radio,
	NUM_USER_ACCESS,
	XmHORIZONTAL,
	NULL
};

typedef struct _localAccess {
	/* gui part */
	Widget			popup;
	Widget			systemName;
	Widget			persLabel;
	Widget			userLabel;
	Widget			accessForm;
	Widget			listFrame;
	Widget			login;
	Widget			button[3];
	Widget			swin;
	Widget			list;
	ActionAreaItems *	actions;
	Widget			status;
	/* non-gui part */
	char *			sysName_C;
	char *			sysName_P;
	accessRight		accessRight_C;
	accessRight		accessRight_P;
	userAccess		userAccess_C;
	userAccess		userAccess_P;
	securityList *		hostsEquiv;
	securityList *		rhosts;
	idList *		hostEqId_C;
	idList *		hostEqId_P;
	idList *		rhostsId_C;
	idList *		rhostsId_P;
	int			curSelectedPos;
	int			preSelectedPos;
} localAccess;

localAccess	la; 

static void
updateSecList(securityList *new, securityList **old)
{
	int	i;

	if (*old) {
		deleteAllSecItems(*old);
	} else {
		*old = (securityList *)malloc(sizeof(securityList));
		memset(*old, 0, sizeof(securityList));
		(*old)->list = (permEntry *)malloc(sizeof(permEntry) * 50);
		memset((*old)->list, 0, sizeof(permEntry)*50);
	}
	for (i = 0; i < new->count; i++) {
		(*old)->list[i].hostName = strdup(new->list[i].hostName);	
		(*old)->list[i].id = strdup(new->list[i].id);
		(*old)->list[i].preserve = strdup(new->list[i].preserve);
	}
	(*old)->count = new->count;	
}

static void
updateIdList(idList *new, idList **old)
{
	int	i;
	
	if (*old) {
		deleteAllIdItems(*old);
	} else {
		*old = (idList *)malloc(sizeof(idList));
		memset(*old, 0, sizeof(idList));
		(*old)->list = (idString *)malloc(sizeof(idString) * 50);
		memset((*old)->list, 0, sizeof(idString)*50);
	}
	for (i=0; i< new->count; i++) {
		(*old)->list[i].id = strdup(new->list[i].id);
	}
	(*old)->count = new->count;	
}

static void
deleteMatchingItems(securityList *list, char * name)
{
	int	i;

	for (i=0; i<list->count; i++) {
		if (strcmp(list->list[i].hostName, name) == 0) {
			free(list->list[i].hostName);
			list->list[i].hostName = NULL;
		}
	}
}

static void
deleteAllIdItems(idList * list)
{
	int	i;

	if (! list) {
		return;
	}
	for (i=0; i<list->count; i++) {
		free(list->list[i].id);
		list->list[i].id = NULL;
	}
	memset(list->list, 0, sizeof(idString) * 50);
	list->count = 0;
}

static void
deleteAllSecItems(securityList * list)
{
	int	i;

	if (! list) {
		return;
	}
	for (i = 0; i < list->count; i++) {
		if (list->list[i].hostName)
			free(list->list[i].hostName);
		if (list->list[i].id)
			free(list->list[i].id);
		if (list->list[i].preserve)
			free(list->list[i].preserve);
	}
	memset(list->list, 0, sizeof(permEntry) * 50);
	list->count = 0;
}

static void
addItem(securityList *list, char * name, char * id)
{
	int	i;
	for (i=0; i < list->count; i++) {
		if ((! list->list[i].preserve) &&
				(strcmp(list->list[i].hostName, "") == 0)) {
			list->list[i].hostName = strdup(name);
			if (list->list[i].id) {
				free(list->list[i].id);
				list->list[i].id = NULL;
			}
			if (id) {
				list->list[i].id = strdup(id);
			}
			return;
		}
	}
	if (((list->count % 50) == 0) && (list->count != 0)) {
		list->list = (permEntry *)realloc(
			(permEntry *)list->list,
			((list->count + 50) * sizeof(permEntry))
		);
		list->list[list->count].hostName = strdup(name);
		if (id) {
			list->list[list->count].id = strdup(id);
		}
		list->count++;
	} else {
		list->list[list->count].hostName = strdup(name);
		if (id) {
			list->list[list->count].id = strdup(id);
		}
		list->count++;
	}
}

static Boolean
search(securityList * list, char * system, idList ** id)
{
	int	i, j=0;
	Boolean	found=False;
	
	if (*id) {
		deleteAllIdItems(*id);
	} else {
		*id = (idList *)malloc(sizeof(idList));
		memset(*id, 0, sizeof(idList));
		(*id)->list = (idString *)malloc(sizeof(idString) * 50);
		memset((*id)->list, 0, sizeof(idString) * 50);
	}

	/* search for the system name */
	for ( i = 0; i < list->count; i++) {
		if (strcmp(list->list[i].hostName, system) == 0) {
			found = True;
			if (strcmp(list->list[i].id, "") != 0) {
				(*id)->list[j].id = strdup(list->list[i].id);
				j++;
			}
		}
	} /* end for */
	(*id)->count = j;

	return(found);
}

static void
setUserState(userAccess access)
{
	if (firstTime) {
		la.userAccess_C = la.userAccess_P = access;
	} else {
		XtVaSetValues(user_radioList.list[la.userAccess_C].widget,
			XmNset, False, NULL);
		la.userAccess_C = access;
	}
	XtVaSetValues(user_radioList.list[access].widget,
		XmNset, True, NULL);
}

static void
loginIDValCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *	login;

	login = XmTextFieldGetString(la.login);
	removeWhitespace(login);
	if (strlen(login) == 0) {
		XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
		if (XtIsSensitive(la.button[MODIFY])) {
			XtVaSetValues(
				la.button[MODIFY],
				XmNsensitive,
				False,
				NULL
			);
		}
	} else {
		XtVaSetValues(la.button[ADD], XmNsensitive, True, NULL);
		if (la.curSelectedPos != 0) {
			XtVaSetValues(
				la.button[MODIFY],
				XmNsensitive,
				True,
				NULL
			);
		}
	}
	XtFree(login);
}

static void
systemNameFocusCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	String	login;

	XtVaGetValues(la.systemName, XmNvalue, &login, NULL);
	removeWhitespace(login);
	if (strlen(login) > 0) {
		if (strcmp(login, la.sysName_C) != 0) {
			if (la.sysName_C) {
				free(la.sysName_C);
				la.sysName_C = NULL;
			}
			la.sysName_C = strdup(login);
			generate();
			if (la.accessRight_P == ENTIRE_SYSTEM) {
				setUserLabel(ENTIRE_SYSTEM);
			}
			else {
				setUserLabel(PERSONAL_ACCOUNT_ONLY);
			}
		}
	} else {
		if (la.sysName_C) {
			free(la.sysName_C);
			la.sysName_C = NULL;
		}
		createMsg(la.popup, INFO, mygettxt(INFO_systemNeed), title);	
	}
}

static void
addCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmString	string;
	String		login;
	int		curpos, i;
	idList *	list;
	Boolean		found;
	
	changesPending = True;

	curpos = la.curSelectedPos;
	XtVaGetValues(la.login, XmNvalue, &login, NULL);
	removeWhitespace(login);

	/* blank the loginId field and grey out all the buttons*/
	/*
	XtVaSetValues(la.login, XmNvalue, "", NULL);
	*/
	XmTextFieldSetString(la.login, "");
	XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[MODIFY], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[DELETE], XmNsensitive, False, NULL);

	if (la.accessRight_C == ENTIRE_SYSTEM) {
		list = la.hostEqId_C;
	} else {
		list = la.rhostsId_C;
	}

	/* check for duplicates */
	found = False;
	for (i = 0; i < list->count; i++) {
		if (! strcmp(login, list->list[i].id)) {
			found = True;
			break;
		}
	}
	if (found) {
		XtFree(login);
		XmListDeselectAllItems(la.list);
		la.curSelectedPos = 0;
		return;
	}

	/* update the scrolled list */
	string = createString(login);
	XmListAddItemUnselected(la.list, string, curpos);
	freeString(string);	

	/* update the internal data structure */
	++list->count;
	if (((list->count % 50) == 0) && (list->count != 0)) {
		list->list = (idString *)realloc(
			(idString *)list->list,
			((list->count + 50) * sizeof(idString))
		);
	}
	if (curpos == 0) {
		list->list[list->count - 1].id = strdup(login);
	} else {
		(void)memmove(
			(void *)&(list->list[curpos]),
			(void *)&(list->list[curpos - 1]),
			(sizeof(idString) * (list->count - curpos + 1))
		);
		list->list[curpos - 1].id = strdup(login);
		++la.curSelectedPos;
	}

	XtFree(login);
}

static void
modCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmString	string;
	String		login;
	int		curpos, i;
	idList *	list;
	Boolean		found;
	
	changesPending = True;

	curpos = la.curSelectedPos;
	XtVaGetValues(la.login, XmNvalue, &login, NULL);
	removeWhitespace(login);

	/* blank the loginId field and grey out all the buttons*/
	/*
	XtVaSetValues(la.login, XmNvalue, "", NULL);
	*/
	XmTextFieldSetString(la.login, "");
	XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[MODIFY], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[DELETE], XmNsensitive, False, NULL);

		
	if (la.accessRight_C == ENTIRE_SYSTEM) {
		list = la.hostEqId_C;
	} else {
		list = la.rhostsId_C;
	}

	/* check for duplicates */
	found = False;
	for (i = 0; i < list->count; i++) {
		if (! strcmp(login, list->list[i].id)) {
			found = True;
			break;
		}
	}
	if (found) {
		XtFree(login);
		XmListDeselectAllItems(la.list);
		la.curSelectedPos = 0;
		return;
	}

	/* update the scrolled list */
	string = createString(login);
	XmListReplaceItemsPos(la.list, &string, 1, curpos);
	freeString(string);	

	/* update the internal data structure */
	free(list->list[curpos - 1].id);
	list->list[curpos - 1].id = strdup(login);

	la.curSelectedPos = 0;

	XtFree(login);
}

static void
delCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		curpos;
	idList *	list;
	
	changesPending = True;

	curpos = la.curSelectedPos;

	/* update the scrolled list */
	XmListDeletePos(la.list, curpos);

	/* blank the loginId field and grey out all the buttons*/
	/*
	XtVaSetValues(la.login, XmNvalue, "", NULL);
	*/
	XmTextFieldSetString(la.login, "");
	XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[MODIFY], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[DELETE], XmNsensitive, False, NULL);

	/* update the internal data structure */
		
	if (la.accessRight_C == ENTIRE_SYSTEM) {
		list = la.hostEqId_C;
	} else {
		list = la.rhostsId_C;
	}
	if (list->count == 1) {
		free(list->list[0].id);
		list->list[0].id = NULL;
	} else {
		free(list->list[curpos - 1].id);
		(void)memmove(
			(void *)&(list->list[curpos - 1]),
			(void *)&(list->list[curpos]),
			(sizeof(idString) * (list->count - curpos))
		);
		(void)memset(
			(void *)&(list->list[list->count - 1]),
			0,
			sizeof(idString)
		);
	}
	--list->count;
	la.curSelectedPos = 0;
}

static void
selectionCB(Widget list_w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *	cbs = (XmListCallbackStruct *)call_data;
	char *			choice;
	
	XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &choice);
	la.curSelectedPos = cbs->item_position;
	/*
	XtVaSetValues(la.login, XmNvalue, choice, NULL);
	*/
	XmTextFieldSetString(la.login, choice);
	XtVaSetValues(la.button[ADD], XmNsensitive, True, NULL);
	XtVaSetValues(la.button[MODIFY], XmNsensitive, True, NULL);
	XtVaSetValues(la.button[DELETE], XmNsensitive, True, NULL);

	XtFree(choice); 
}

void
nooneCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (call_data) {
		if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set)
			return;
	}
	if (la.userAccess_C != NO_ONE) {
		changesPending = True; 
		setUserState(NO_ONE);
	}

	/* unmap the login id list if it's up... */
	if (XtIsManaged(la.listFrame))
		XtUnmanageChild(la.listFrame);		
}

void
selfCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
	if (call_data) {
		if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set)
			return;
	}
	if (la.userAccess_C != SELF) {
		changesPending = True;
		setUserState(SELF);
	}
	if (XtIsManaged(la.listFrame))
		XtUnmanageChild(la.listFrame);
}

void
allCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (call_data) {
		if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set)
			return;
	}
	if (la.userAccess_C != ALL_USERS) {
		changesPending = True;
		setUserState(ALL_USERS);
		createMsg(hi.net.common.toplevel, WARN,
			mygettxt(WARN_otherAcctAllUsers),title);
	}
	if (XtIsManaged(la.listFrame))
		XtUnmanageChild(la.listFrame);		
}

void
specificCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmString	string;
	int		i, count, num = 0;
	idList *	id;
	
	if (call_data) {
		if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set)
			return;
	}
	if (la.userAccess_C != SPECIFIC_USERS) {
		changesPending = True;
		setUserState(SPECIFIC_USERS);
	}

	XtManageChild(la.listFrame);

	if (la.accessRight_C == ENTIRE_SYSTEM) {
		count = la.hostEqId_C->count;
		id = la.hostEqId_C;
		createMsg(hi.net.common.toplevel, WARN,
			mygettxt(WARN_otherAcctSpecificUsers),title);
	} else {
		count = la.rhostsId_C->count;
		id = la.rhostsId_C;
		createMsg(hi.net.common.toplevel, WARN,
			mygettxt(WARN_personalAcctSpecificUsers),title);
	}
	if (! firstTime)
		XmListDeleteAllItems(la.list);

	for (i = 0; i < count; i++) {
		if (strcmp(id->list[i].id, "") != 0) {
			string = createString(id->list[i].id);
			XmListAddItemUnselected(la.list, string, 0);
			freeString(string);
			num++;
		}
	}
	XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[MODIFY], XmNsensitive, False, NULL);
	XtVaSetValues(la.button[DELETE], XmNsensitive, False, NULL);
}

static void
setUserLabel(accessRight kind)
{
	char	buf[BUFSIZ];

	switch (kind) {
		case ENTIRE_SYSTEM:
			sprintf(buf, mygettxt(TXT_ruaEquivMenu), la.sysName_C);
			break;
		case PERSONAL_ACCOUNT_ONLY:
		default:
			sprintf(buf, mygettxt(TXT_ruaRhostsMenu), la.sysName_C);
			break;
	}
	/* update user label */
	setLabel(la.userLabel, buf);
}
	

static void
entireCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set) {
		return;
	}
	if (changesPending == True) {
		changesPending = False;
		createMsg(
			hi.net.common.toplevel,
			WARN,
			mygettxt(WARN_switchAccessRights),
			title
		);
	}
	XtVaGetValues(la.systemName, XmNvalue, &la.sysName_C, NULL);	
	generate();
	XtManageChild(user_radioList.list[ALL_USERS].widget);
	setUserLabel(ENTIRE_SYSTEM);
}

static void
personalCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if ( ! ((XmToggleButtonCallbackStruct *)call_data)->set) {
		return;
	}
	if (changesPending == True) {
		changesPending = False;
		createMsg(
			hi.net.common.toplevel,
			WARN,
			mygettxt(WARN_switchAccessRights),
			title
		);
	}
	XtVaGetValues(la.systemName, XmNvalue, &la.sysName_C, NULL);	
	generate();
	XtUnmanageChild(user_radioList.list[ALL_USERS].widget);
	setUserLabel(PERSONAL_ACCOUNT_ONLY);
}

static void
okCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (! la.sysName_C) {
		createMsg(
			hi.net.common.toplevel,
			INFO,
			mygettxt(INFO_systemNeed),
			title
		);	
		return;
	}
	changesPending = False;
	firstTime = True;
	applyCB(NULL, NULL, NULL);
	XtUnmanageChild(la.popup);
	bailOut();
}

static void
applyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int		i;
	struct passwd *	pass;

	if (! la.sysName_C) {
		createMsg(
			hi.net.common.toplevel,
			INFO,
			mygettxt(INFO_systemNeed),
			title
		);	
		return;
	}
	changesPending = False;
	pass = getpwuid(getuid());
	if (la.accessRight_C == ENTIRE_SYSTEM) { 
		deleteMatchingItems(la.hostsEquiv, la.sysName_C);
	} else {
		deleteMatchingItems(la.rhosts, la.sysName_C);
	}	
	switch (la.userAccess_C) {
		case NO_ONE	: {
			if (la.accessRight_C == ENTIRE_SYSTEM) {
				writeBack(hostEquivPath, la.hostsEquiv);
			} else {
				writeBack(rhostsPath, la.rhosts);
			}
			break;
		}
		case SELF	: {
			if (la.accessRight_C == ENTIRE_SYSTEM) {
				addItem(
					la.hostsEquiv,
					la.sysName_C,
					pass->pw_name
				);
				writeBack(hostEquivPath, la.hostsEquiv);
			} else {
				addItem(
					la.rhosts,
					la.sysName_C,
					pass->pw_name
				);
				writeBack(rhostsPath, la.rhosts);
			}
			break;
		}
		case ALL_USERS	: {
			addItem(la.hostsEquiv, la.sysName_C, "");
			writeBack(hostEquivPath, la.hostsEquiv);
			break;
		}
		case SPECIFIC_USERS	: {
			if (la.accessRight_C == ENTIRE_SYSTEM) {
				for (i = 0; i < la.hostEqId_C->count; i++) {
					addItem(
						la.hostsEquiv,
						la.sysName_C,
						la.hostEqId_C->list[i].id
					);
				}
				writeBack(hostEquivPath, la.hostsEquiv);
			} else {
				for (i = 0; i < la.rhostsId_C->count; i++) {
					addItem(
						la.rhosts,
						la.sysName_C,
						la.rhostsId_C->list[i].id
					);
				}
				writeBack(rhostsPath, la.rhosts);
			}
			break;
		}
	}
	if (la.accessRight_C == ENTIRE_SYSTEM) {
		parse(ENTIRE_SYSTEM, &la.hostsEquiv);
		search(la.hostsEquiv, la.sysName_C, &la.hostEqId_C);
		updateIdList(la.hostEqId_C, &la.hostEqId_P);
	} else {
		parse(PERSONAL_ACCOUNT_ONLY, &la.rhosts);
		search(la.rhosts, la.sysName_C, &la.rhostsId_C);
		updateIdList(la.rhostsId_C, &la.rhostsId_P);
	}
	if (la.sysName_P) 
		free(la.sysName_P);
	la.sysName_P = strdup(la.sysName_C);
	la.accessRight_P = la.accessRight_C;
	la.userAccess_P = la.userAccess_C;
	la.preSelectedPos = la.curSelectedPos;

	/* clear out list gui */
	if (la.userAccess_C != SPECIFIC_USERS) {
		XmListDeleteAllItems(la.list);
		/*
		XtVaSetValues(la.login, XmNvalue, "", NULL);
		*/
		XmTextFieldSetString(la.login, "");
		XtVaSetValues(la.button[ADD], XmNset, False, NULL);
		XtVaSetValues(la.button[MODIFY], XmNset, False, NULL);
		XtVaSetValues(la.button[DELETE], XmNset, False, NULL);
	} else {
		/*
		XtVaSetValues(la.login, XmNvalue, "", NULL);
		*/
		XmTextFieldSetString(la.login, "");
		XtVaSetValues(la.button[ADD], XmNset, False, NULL);
		XtVaSetValues(la.button[MODIFY], XmNset, False, NULL);
		XtVaSetValues(la.button[DELETE], XmNset, False, NULL);
	}
}

static void
resetCB(Widget w, XtPointer client_data, XtPointer call_data)
{

	int		i, count;
	idList		*id;
	XmString	string;	

	changesPending = False;

	/*
	XtVaSetValues(la.systemName, XmNvalue, la.sysName_P, NULL);
	*/
	XmTextFieldSetString(la.systemName, la.sysName_P);
	if (la.sysName_C) free(la.sysName_C);
	la.sysName_C = strdup(la.sysName_P);

	XtVaSetValues(acc_radioList.list[la.accessRight_C].widget,
		XmNset, False, NULL);
	XtVaSetValues(acc_radioList.list[la.accessRight_P].widget,
		XmNset, True, NULL);
	la.accessRight_C = la.accessRight_P;

	if (la.accessRight_P == ENTIRE_SYSTEM) {
		updateIdList(la.hostEqId_P, &la.hostEqId_C);
		count = la.hostEqId_P->count;
		id = la.hostEqId_P;
		setUserLabel(ENTIRE_SYSTEM);
	} else {
		updateIdList(la.rhostsId_P, &la.rhostsId_C);
		count = la.rhostsId_P->count;
		id = la.rhostsId_C;
		setUserLabel(PERSONAL_ACCOUNT_ONLY);
		XtUnmanageChild(
			user_radioList.list[ALL_USERS].widget
		);
	}

	setUserState(la.userAccess_P);
	la.userAccess_C = la.userAccess_P;	

	if (la.userAccess_P == SPECIFIC_USERS) {
		/*
		XtVaSetValues(la.login, XmNvalue, "", NULL);
		*/
		XmTextFieldSetString(la.login, "");
		XmListDeleteAllItems(la.list);	
		for (i = 0; i < count; i++) {
			string = createString(id->list[i].id);
			XmListAddItemUnselected(la.list, string, 0);
			freeString(string);
		}
		la.curSelectedPos = la.preSelectedPos = 0;
		XtVaSetValues(la.button[ADD], XmNsensitive, False, NULL);
		XtVaSetValues(la.button[MODIFY], XmNsensitive, False, NULL);
		XtVaSetValues(la.button[DELETE], XmNsensitive, False, NULL);
		XtManageChild(la.listFrame);	
	} else {
		if (XtIsManaged(la.listFrame))
			XtUnmanageChild(la.listFrame);
	}
}

static void
cancelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	changesPending = False;
	firstTime = True;
	XtUnmanageChild(la.popup);
	bailOut();
}

void
setupGui(Widget w, XtPointer client_data, XtPointer call_data)
{
	static Widget	topForm;
	Dimension	maxWidth;
	Widget		laRC, laForm;
	Widget		listRC;
	Arg		args[10];
	int		i, j;
	int		index;
	char *		name = NULL;
	int		wid_pos;
	dnsList *	cur_pos;
	char		buf[MEDIUMBUFSIZE];
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	unsigned char *		mne;
	char			tmpstr[BUFSIZ];
	KeySym			mneks;
	XmString		mneString;
	int			mm;

	title =  mygettxt(TXT_ruaTitle);

	if (!la.popup) {
		la.popup = XtVaCreatePopupShell("dialogshell",
			xmDialogShellWidgetClass, XtParent(w),
			NULL);
		XtVaSetValues(la.popup,
			XmNtitle, mygettxt(TXT_ruaTitle),
			XmNallowShellResize, True,
			NULL);
		r_decor(la.popup);

		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, la.popup,
			NULL);

		
		laRC = XtVaCreateManagedWidget("rowcolumn",
			xmRowColumnWidgetClass, topForm,
			NULL);
		XtAddCallback(laRC, XmNhelpCallback, helpkeyCB, &SLAHelp);

		maxWidth = getMaxWidth(
			topLabels,
			xmLabelWidgetClass,
			XmNlabelString,
			laRC
		);
		la.systemName = crt_inputlabel(laRC, TXT_ruaSysName, maxWidth);
		XtAddCallback(
			la.systemName,
			XmNlosingFocusCallback,
			systemNameFocusCB,
			NULL
		);
		XtAddCallback(
			la.systemName,
			XmNmodifyVerifyCallback,
			noSpacesCB,
			NULL
		);
		la.accessForm = crt_inputradio(laRC, TXT_ruaRestrict, maxWidth, &acc_radioList, False);
		la.persLabel = crt_inputlabellabel(laRC, TXT_ruaRestrict,
			maxWidth, TXT_ruaRestrictPers);	

		(void)XtVaCreateManagedWidget("separator",
			xmSeparatorWidgetClass, laRC, NULL);
	
		la.userLabel = XtVaCreateManagedWidget("user",
			xmLabelWidgetClass, laRC, NULL);
		setLabel(la.userLabel, TXT_ruaRhostsMenu);

		(void)crt_rc_radio(laRC, &user_radioList);
			
		maxWidth = getMaxWidth(
			bottomLabels,
			xmLabelWidgetClass,
			XmNlabelString,
			laRC
		);
		
		la.listFrame = XtVaCreateManagedWidget(
			"frame", xmFrameWidgetClass, laRC,
			XmNshadowType,	XmSHADOW_ETCHED_IN,
			NULL
		);
		listRC = XtVaCreateManagedWidget("rowcolumn",
			xmRowColumnWidgetClass, la.listFrame,
			NULL);

		la.login = crt_inputlabel(listRC, TXT_Login, maxWidth);			
		XtAddCallback(la.login, XmNvalueChangedCallback, 
			loginIDValCB, NULL);
		XtAddCallback(la.login, XmNmodifyVerifyCallback, 
			noSpacesCB, NULL);

		(void)crt_inputlabellabel(listRC, "", maxWidth, TXT_Allow);

		laForm = XtVaCreateManagedWidget(
			"form", xmFormWidgetClass, listRC,
			NULL
		);
		for (i = 0; i < (XtNumber(laButtons)) - 1; i++) {
			Dimension		d;
			XmString		xms;

			d = getMaxWidth(
				laButtons,
				xmPushButtonWidgetClass,
				XmNlabelString,
				listRC
			);

			strcpy(tmpstr, mygettxt(laButtons[i]));
			la.button[i] = XtVaCreateManagedWidget(
				tmpstr, xmPushButtonWidgetClass, laForm,
				XmNwidth,	d,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNleftOffset,		maxWidth - d,
				XmNsensitive,		False,
				NULL
			);
			XmSTR_N_MNE(
				laButtons[i],
				laButtonMnemonics[i],
				mneInfo[i],
				DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
			);
			XmStringFree(mneString);
			XtVaSetValues(la.button[i], XmNmnemonic, mneks, NULL);
			if (i == 0) {
				XtVaSetValues(
					la.button[i],
					XmNtopAttachment,	XmATTACH_FORM,
					NULL
				);
			} else {
				XtVaSetValues(
					la.button[i],
					XmNtopAttachment,	 XmATTACH_WIDGET,
					XmNtopWidget,		 la.button[i-1],
					NULL
				);
			}
			if (i == 2) {
				XtVaSetValues(
					la.button[i],
					XmNbottomAttachment,	 XmATTACH_WIDGET,
					NULL
				);
			}
		}
		XtAddCallback(la.button[ADD], XmNactivateCallback, addCB, NULL);
		mneInfo[0].w = la.button[ADD];
		mneInfo[0].cb = (XtCallbackProc)addCB;
		XtAddCallback(la.button[MODIFY], XmNactivateCallback, modCB, NULL);
		mneInfo[1].w = la.button[MODIFY];
		mneInfo[1].cb = (XtCallbackProc)modCB;
		XtAddCallback(la.button[DELETE], XmNactivateCallback, delCB, NULL);
		mneInfo[2].w = la.button[DELETE];
		mneInfo[2].cb = (XtCallbackProc)delCB;
		
		createActionArea(
			laRC,
			actions,
			XtNumber(actions),
			la.userLabel,
			mneInfo,
			3
		);

		(void)XtVaCreateManagedWidget("separator",
			xmSeparatorWidgetClass, laRC, NULL);

		la.status = XtVaCreateManagedWidget("sla_status",
			xmLabelWidgetClass, laRC,
			NULL);
		setLabel(la.status, "");

		/* set the default and cancel buttons */
		XtVaSetValues(topForm, 
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[3].widget, 
			NULL);
		j=0;
		XtSetArg(args[j], XmNtopAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNbottomAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNleftAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNrightAttachment, XmATTACH_FORM); j++;
		XtSetArg(args[j], XmNleftOffset, (maxWidth + 2)); j++;
		XtSetArg(args[j], XmNvisibleItemCount, 4); j++;
		la.list = XmCreateScrolledList(laForm, "list", args, j);	

		/* double click */
		XtAddCallback(la.list, XmNdefaultActionCallback, 
			selectionCB, NULL);
		/* single click */
		XtAddCallback(la.list, XmNbrowseSelectionCallback,
			selectionCB, NULL);

		XtManageChild(la.list);

		/* add mini-help */
		for (i = 0; i < 5; i++) {
			slaHelp[i].widget = la.status;
		}
		/* system name */
		XtAddEventHandler(
			la.systemName,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&slaHelp[0]
		);
		/* entire system radio button */
		XtAddEventHandler(
			acc_radioList.list[ENTIRE_SYSTEM].widget,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&slaHelp[1]
		);
		/* personal account radio button */
		XtAddEventHandler(
			acc_radioList.list[PERSONAL_ACCOUNT_ONLY].widget,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&slaHelp[2]
		);
		/* user buttons */
		for (i = 0; i < 4; i++) {
			XtAddEventHandler(
				user_radioList.list[i].widget,
				FocusChangeMask,
				False,
				(void(*)())displayFooterMsgEH,
				(XtPointer)&slaHelp[3]
			);
		}

		/* login */
		XtAddEventHandler(
			la.login,
			FocusChangeMask,
			False,
			(void(*)())displayFooterMsgEH,
			(XtPointer)&slaHelp[4]
		);

		REG_MNE(la.popup, mneInfo, XtNumber(actions) - 1 + 3);

		if (decideScreenLayout() == False)
			return;

	} else {
		if (hi.net.common.cur_view == etcHost) {
			index = ((hi.net.etc.etcHostIndex + (ETCCOLS - 1)) / ETCCOLS) - 2;	
			name = strdup(hi.net.etc.etcHosts->list[index].etcHost.name);
		} else {
			wid_pos = hi.net.dns.cur_wid_pos;
			cur_pos = hi.net.dns.cur_pos;
			for (i = 0; i < wid_pos; i++)
				cur_pos = cur_pos->next;
			if (
				isInDomainList(
					hi.net.dns.dnsIndex[wid_pos],
					cur_pos
				) == True
			) {
				createMsg(hi.net.common.toplevel, ERROR,
					mygettxt(ERR_cantAccDomain), title);
				return;
			} else {
				if (! strchr(hi.net.dns.dnsSelection[wid_pos], '.')) {
					sprintf(
						buf,
						"%s.%s",
						hi.net.dns.dnsSelection[wid_pos],
						cur_pos->domain.name
					);
				} else {
					strcpy(buf,
					hi.net.dns.dnsSelection[wid_pos]);
				}
			}
			name = strdup(buf);
		}
		if (strcmp(la.sysName_C, name) != 0) {
			if (la.sysName_C) {
				free(la.sysName_C);
				la.sysName_C = NULL;
			}
			la.sysName_C = strdup(name);
			/*
			XtVaSetValues(
				la.systemName,
				XmNvalue,
				la.sysName_C,
				NULL
			);
			*/
			XmTextFieldSetString(
				la.systemName,
				la.sysName_C
			);
			free(name);
			generate();
			if (la.accessRight_P == ENTIRE_SYSTEM) {
				setUserLabel(ENTIRE_SYSTEM);
			} else {
				setUserLabel(PERSONAL_ACCOUNT_ONLY);
			}
		}
	}
	XtManageChild(topForm);
	XtManageChild(la.popup);
}

static void
generate(void)
{
	Boolean		set;
	struct passwd *	pass;

	if ((pass = getpwuid(getuid())) == 0) {
		createMsg(hi.net.common.toplevel, ERROR, 
		mygettxt(ERR_cantGetPasswd),title);
		return;
	}

	/* check the acc_radioLIst */
	XtVaGetValues(acc_radioList.list[ENTIRE_SYSTEM].widget,
		XmNset, &set,
		NULL);
	if (set) { /* Entire System */
		la.accessRight_C = ENTIRE_SYSTEM;
		parse(ENTIRE_SYSTEM, &la.hostsEquiv);
		if (search(la.hostsEquiv, la.sysName_C, &la.hostEqId_C)) {
			if (la.hostEqId_C->count == 0) {
				setUserState(ALL_USERS);
				allCB(NULL, NULL, NULL);
			}
			if (la.hostEqId_C->count > 0) {
				if (firstTime == True)
					updateIdList(la.hostEqId_C, &la.hostEqId_P);
				if ((la.hostEqId_C->count) == 1 &&
					(strcmp(la.hostEqId_C->list[0].id, pass->pw_name) == 0)){
					setUserState(SELF);
					selfCB(NULL, NULL, NULL);
				} else {
					setUserState(SPECIFIC_USERS);
					specificCB(NULL, NULL, NULL);
				}
			}
		} else {
			setUserState(NO_ONE);
			nooneCB(NULL, NULL, NULL);
		}
	} else {	/* Personal Account Only */
		/* open and read the $HOME/.rhosts file and fill
		 * up the rhost internal data structure
		 */
		la.accessRight_C = PERSONAL_ACCOUNT_ONLY;
		parse(PERSONAL_ACCOUNT_ONLY, &la.rhosts);
		if (search(la.rhosts, la.sysName_C, &la.rhostsId_C)) {
			if (la.rhostsId_C->count > 0) {
				if (firstTime == True)
					updateIdList(la.rhostsId_C, &la.rhostsId_P);
				if ((la.rhostsId_C->count) == 1 &&
					(strcmp(la.rhostsId_C->list[0].id, pass->pw_name) == 0)) {
					setUserState(SELF);
					selfCB(NULL, NULL, NULL);
				} else {
					setUserState(SPECIFIC_USERS);
					specificCB(NULL, NULL, NULL);
				}
			}
		} else {
			setUserState(NO_ONE);
			nooneCB(NULL, NULL, NULL);
		}
	}
	if (firstTime == True)
		firstTime = False;
}

static Boolean
decideScreenLayout(void)
{
	char		buf[MEDIUMBUFSIZE];
	int		listIndex;
	int		wid_pos, i;
	dnsList *	cur_pos;

	/* Get the current selection */
	if (hi.net.common.cur_view == etcHost) {
		listIndex=(((hi.net.etc.etcHostIndex + (ETCCOLS - 1)) /
			ETCCOLS) - 2);
		la.sysName_C = strdup(
			hi.net.etc.etcHosts->list[listIndex].etcHost.name
		);
		la.sysName_P = strdup(la.sysName_C);
	} else {
		wid_pos = hi.net.dns.cur_wid_pos;
		cur_pos = hi.net.dns.cur_pos;
		for (i=0; i < wid_pos; i++)
			cur_pos = cur_pos->next;
		/* From Dns view, need to check if it is a domain.*/
		if (
			isInDomainList(
				hi.net.dns.dnsIndex[wid_pos],
				cur_pos
			) == TRUE
		) {
			createMsg(hi.net.common.toplevel, ERROR, 
				mygettxt(ERR_cantAccDomain),title);
			return False;
		} else {
			if (! strchr(hi.net.dns.dnsSelection[wid_pos], '.')) {
				sprintf(
					buf,
					"%s.%s",
					hi.net.dns.dnsSelection[wid_pos],
					cur_pos->domain.name
				);
			} else {
				strcpy(buf, hi.net.dns.dnsSelection[wid_pos]);
			}
			la.sysName_C = strdup(buf);
			la.sysName_P = strdup(buf);
		}
	}
	/*
	XtVaSetValues(la.systemName, XmNvalue, la.sysName_C, NULL);
	*/
	XmTextFieldSetString(la.systemName, la.sysName_C);
	/* set the current and privious values */
	la.accessRight_C = PERSONAL_ACCOUNT_ONLY;
	la.accessRight_P = PERSONAL_ACCOUNT_ONLY;
	la.hostsEquiv = la.rhosts = (securityList *)NULL;
	la.hostEqId_C = la.hostEqId_P = (idList *)NULL;
	la.rhostsId_C = la.rhostsId_P = (idList *)NULL;
	la.curSelectedPos = la.preSelectedPos = 0;
	
	setUserLabel(PERSONAL_ACCOUNT_ONLY);
	if (hi.net.common.isOwner) {
		/* unmap the personal label and map the radio buttons */
		XtUnmanageChild(XtParent(la.persLabel));
		XtManageChild(la.accessForm);
		XtVaSetValues(acc_radioList.list[PERSONAL_ACCOUNT_ONLY].widget,
			XmNset, True,
			NULL);
		/* and just in case rua is already up... */
		XtVaSetValues(acc_radioList.list[ENTIRE_SYSTEM].widget,
			XmNset, False,
			NULL);
	}
	else {
		/* unmap the access_radioList and create the label for
		 * personal account only
		 */
		XtUnmanageChild(la.accessForm);
		XtManageChild(XtParent(la.persLabel));
		/* unmap the "All Users" radio button */
	}	
	XtUnmanageChild(user_radioList.list[ALL_USERS].widget);
	generate();
	return True;
}

void
createLocalAccess(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	*home, *getenv();

	home = getenv("HOME");
	sprintf(rhostsPath, "%s/.rhosts", home);

	setupGui(w, client_data, call_data);
}

static void
parse(accessRight access, securityList ** list)
{
	static FILE	*fd;
	char		buf[MEDIUMBUFSIZE];
	char		*p;
	char *		p1;
	int		i = 0;

	if (*list) {
		deleteAllSecItems(*list);
	} else {
		*list = (securityList *)malloc(sizeof(securityList));
		memset(*list, 0, sizeof(securityList));
		(*list)->list = (permEntry *)malloc(sizeof(permEntry) * 50);
		memset((*list)->list, 0, sizeof(permEntry) * 50);
	}
	if ((fd = fopen(access == ENTIRE_SYSTEM ? hostEquivPath : rhostsPath, "r")) == (FILE *)NULL) {
		return;
	} else {
		while (fgets(buf, sizeof(buf), fd)) {
			p = buf;
			/* skip the white spaces until the first char */
			GOBBLEWHITE(p);
			/* check for a comment, newline or null line */
			if (*p == '#') {
				continue;
			} else if ((!(*p)) || (*p == '\n')) {
				continue;
			} else if ((*p == '+') && (*(p+1) == '\n')) {
				continue;
			} else {
				;
			}
			/* check for NIS info */
			if ((strstr(p, "+@")) || (strstr(p, "-@"))) {
				/* got one */
				(*list)->list[i].hostName = NULL;
				(*list)->list[i].id = NULL;
				(*list)->list[i].preserve = strdup(buf);
				i++;
				continue;
			}
			p1 = p;
			TRAVERSETOKEN(p1);
			if (*p1 == '\n') {
				if (access == ENTIRE_SYSTEM) {
					*p1 = '\0';
					(*list)->list[i].hostName = strdup(p);
					(*list)->list[i].id = NULL;
					(*list)->list[i].preserve = NULL;
					i++;
				}
				continue;
			}
			(*list)->list[i].hostName = strndup(p, p1-p);
			p = p1;
			GOBBLEWHITE(p);
			p1 = p;
			TRAVERSETOKEN(p1);
			(*list)->list[i].id = strndup(p, p1-p);
			(*list)->list[i].preserve = NULL;
			i++;
		}	
		(*list)->count = i;
	}
	fclose(fd);
}

static void
writeBack(char *pathname, securityList * list)
{
	static FILE	*fd;
	char		buf[MEDIUMBUFSIZE];
	int		i;
	
	if ((fd = fopen(pathname, "w")) == (FILE *)NULL) {
		sprintf(buf, mygettxt(ERR_cantWriteFile), pathname);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		
		return;
	} else {
		for (i=0; i< list->count; i++) {
			if (list->list[i].preserve) {
				fputs(list->list[i].preserve, fd);
				continue;
			}
			if (strcmp(list->list[i].hostName, "") != 0) {
				sprintf(buf, "%s", list->list[i].hostName);
				if (list->list[i].id) {
					sprintf(buf, "%s\t%s\n", buf,
						list->list[i].id);
				} else {
					strcat(buf, "\n");
				}
				if (fputs(buf, fd) == EOF) {
					sprintf(buf, mygettxt(ERR_cantWriteFile), pathname);
					createMsg(hi.net.common.toplevel,
						ERROR, buf, title);
					return;
				}
			}
		}
	}
	fclose(fd);
}

static void
bailOut(void)
{
	int	i;

	if (la.sysName_C) free(la.sysName_C);
	if (la.sysName_P) free(la.sysName_P);
	la.sysName_C = NULL;
	la.sysName_P = NULL;

	la.accessRight_P = la.accessRight_C = 0;
	la.userAccess_P = la.userAccess_C = 0;

	deleteAllSecItems(la.rhosts);
	if (la.rhosts->list) free(la.rhosts->list);
	if (la.rhosts) free(la.rhosts);
	la.rhosts = NULL;

	deleteAllSecItems(la.hostsEquiv);
	if (la.hostsEquiv->list) free(la.hostsEquiv->list);
	if (la.hostsEquiv) free(la.hostsEquiv);
	la.hostsEquiv = NULL;

	deleteAllIdItems(la.hostEqId_C);
	if (la.hostEqId_C->list) free(la.hostEqId_C->list);
	if (la.hostEqId_C) free(la.hostEqId_C);
	la.hostEqId_C = NULL;

	deleteAllIdItems(la.hostEqId_P);
	if (la.hostEqId_P->list) free(la.hostEqId_P->list);
	if (la.hostEqId_P) free(la.hostEqId_P);
	la.hostEqId_P = NULL;

	deleteAllIdItems(la.rhostsId_C);
	if (la.rhostsId_C->list) free(la.rhostsId_C->list);
	if (la.rhostsId_C) free(la.rhostsId_C);
	la.rhostsId_C = NULL;

	deleteAllIdItems(la.rhostsId_P);
	if (la.rhostsId_P->list) free(la.rhostsId_P->list);
	if (la.rhostsId_P) free(la.rhostsId_P);
	la.rhostsId_P = NULL;

	memset((void *)&la, 0, sizeof(localAccess));
	/*
	for (i = 0; i < NUM_ACCESS_RIGHT; i++) {
		XtVaSetValues(
			acc_radioList.list[i].widget,
			XmNset,
			False,
			NULL
		);
	}
	for (i = 0; i < NUM_USER_ACCESS; i++) {
		XtVaSetValues(
			user_radioList.list[i].widget,
			XmNset,
			False,
			NULL
		);
	}
	*/
}
