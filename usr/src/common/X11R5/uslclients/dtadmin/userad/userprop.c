/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/userprop.c	1.1.1.1"
#endif

#include "LoginMgr.h"
#include <MGizmo/ListGizmo.h>

extern void	applyCB(Widget, XtPointer, XtPointer);
extern void	resetCB(Widget, XtPointer, XtPointer);
extern void	cancelCB(Widget, XtPointer, XtPointer);
extern void	helpCB(Widget, XtPointer, XtPointer);
extern void	reinitPopup(int);
extern void	GetLocaleName(char *);
extern void	GetLocale(char *);

extern Gizmo	g_popup;
extern Gizmo	g_gpopup;
extern Gizmo	g_base;
extern Widget	w_popup;
extern Widget	w_prop_0;
extern Widget	w_login;
extern Widget	w_nds;
extern Widget	w_nis;
extern Widget	w_desc;
extern Widget	w_extra;
extern Widget	w_home;
extern Widget	w_uid;
extern Widget	w_shell;
extern Widget	w_remote;
extern Widget	w_glist;
extern Widget	w_group;
extern Widget	w_locale;
extern Widget	w_localetxt;
extern Widget	w_baseshell;

extern Boolean		ypbind_on;
extern Boolean		I_am_owner;
extern Boolean		nis_user;
extern char **		GroupItems;
extern FListPtr2	LocaleItems;
extern int		locale_cnt;
extern int		g_cnt;
extern int		u_pending;
extern int		locale_list_index;
extern int		group_list_index;

extern char             *login, *desc, *home, *group, *remote;
extern char		*shell, *uid, *gname, *gid;
extern char		*ndsname;
extern char		*userLocale, *userLocaleName;
extern int		view_type;

static HelpInfo HelpProps       = { 0, "", HELP_PATH, help_props };


static void
SetGroup(Widget wid, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *	cbs = (XmListCallbackStruct *)call_data;
	XmString		string;
	char			buf[256];

	group_list_index = cbs->item_position-1;
	strcpy(buf, GGT(label_selection));
	strcat(buf, GroupItems[cbs->item_position-1]);
	string = XmStringCreateLocalized(buf);
	XtVaSetValues(w_group, XmNlabelString, string, 0);
	XmStringFree(string);
	group = GroupItems[cbs->item_position-1];
}

static	void
SetLocale(Widget wid, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	XmString		string;
	char			buf[256];

	locale_list_index = cbs->item_position-1;
	strcpy(buf, GGT(label_selection));
	strcat(buf, (char *)LocaleItems[cbs->item_position-1].label);
	string = XmStringCreateLocalized(buf);
	XtVaSetValues(w_localetxt, XmNlabelString, string, 0);
	XmStringFree(string);
	userLocaleName = (char *)LocaleItems[cbs->item_position-1].label;
	GetLocale(userLocaleName);
}

static void
SetDesktop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	XtArgVal dtm_flag;

	dtm_flag = (XtArgVal)(cbs->index == 0);
	XtVaSetValues(w_remote, XmNsensitive, dtm_flag, NULL);
}

static	void
SelExtraCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Boolean	set;
    Widget	w;

    XtVaGetValues(w_extra, XmNset, &set, NULL);
    SetPopupWindowLeftMsg(g_popup, "");
    w = QueryGizmo(PopupGizmoClass, g_popup, GGW, "rc8");
    if (set ==  True) {
    	XtManageChild(w);
    }
    else {
    	XtUnmanageChild(w);
    }
}


/* This routine sets the sensitivity 
 * of certain property widgets depending
 * on if user is a NIS user or not.
 */
void
SetNisUserValues()
{

 XtVaSetValues(w_nis, XmNset, (XtArgVal)nis_user, NULL);

 if ( nis_user == True){
     XtVaSetValues(w_desc, XmNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_group, XmNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_glist, XmNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_shell, XmNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_uid, XmNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_home, XmNsensitive, (XtArgVal)False, NULL);
     if (ypbind_on == True && view_type != RESERVED)
              XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
     else
              XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)False, NULL);
 }
 else{
     XtVaSetValues(w_desc, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_group, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_glist, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_shell, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_uid, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_home, XmNsensitive, (XtArgVal)True, NULL);
     if (view_type != RESERVED)
          XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
     else
          XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)False, NULL);
 }

}

/* This routine is called when a non-NIS user
 * selects the NIS button. It will set
 * the button and change the sensitivity of 
 * several property fields.
 */
static void
SelNisUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char  buf[BUFSIZ];

	XtVaGetValues(w_nis, XmNset, &nis_user, NULL);
	if ( nis_user == True ){
		if ( u_pending == P_ADD || u_pending == P_CHG ){
  			reinitPopup(0);
	  		sprintf(buf, "%s\n", GetGizmoText(string_noUserValues));
			FooterMsg(buf);
		}
	}
	else {
	    if ( u_pending == P_ADD )
		  reinitPopup(0);
	    FooterMsg("");
	}

	SetNisUserValues();
}

static void
hideExtraCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtVaSetValues(w_extra, XmNset, (XtArgVal)FALSE, NULL);
    XtUnmanageChild(QueryGizmo(PopupGizmoClass, g_popup, GGW, "rc8"));
}


/********************************************************************
 * Gizmos for user property sheet
 */
/* Login ID: */
static InputGizmo _login = {0, "login", NULL, 8};
static GizmoRec loginArray[] = {
	{InputGizmoClass,	&_login}
};
static LabelGizmo loginLabel = {
	0, "loginLabel", label_login, False, loginArray,
	XtNumber(loginArray), G_LEFT_LABEL
};
/* Type: */
static MenuItems dtmItems[] = {
	{TRUE, label_desktop, NULL, I_RADIO_BUTTON, 0, 0, 0, True},
	{TRUE, label_nondesk, NULL, I_RADIO_BUTTON, 0, 0, 0, False},
	{NULL}
};
static MenuGizmo dtmMenu = {
	0, "dtmMenu", NULL, dtmItems, SetDesktop, NULL, XmHORIZONTAL, 1, 1
};
static ChoiceGizmo dtmChoice = {
	0, "dtmChoice", &dtmMenu, G_RADIO_BOX
};
static GizmoRec dtmArray[] = {
	{ChoiceGizmoClass,	&dtmChoice}
};
static LabelGizmo dtmLabel = {
	0, "dtmLabel", label_type, False, dtmArray,
	XtNumber(dtmArray), G_LEFT_LABEL
};

static GizmoRec array1[] = {
	{LabelGizmoClass,	&loginLabel},
	{LabelGizmoClass,	&dtmLabel}
};
static ContainerGizmo rc1 = {
	0, "rc1", G_CONTAINER_RC, 10, 10, array1, XtNumber(array1)
};
Arg     arg1[] = {
	{XmNorientation,	XmHORIZONTAL}
};
Arg     arg2[] = {
	{XmNorientation,	XmVERTICAL},
};

/* NDS name: */
static InputGizmo _nds = {0, "nds", NULL, 8};
static GizmoRec ndsArray[] = {{InputGizmoClass, &_nds}};
static LabelGizmo ndsLabel = {
	0, "ndsLabel", label_nds, False, ndsArray, XtNumber(ndsArray),
	G_LEFT_LABEL
};

/* Manage NIS */
static MenuItems nisMenuItem[] = {  
	{TRUE, label_nis_user, "", I_TOGGLE_BUTTON, 0, SelNisUserCB},
	{ NULL }
};
static MenuGizmo nisMenu = {
	0, "nisMenu", NULL, nisMenuItem, NULL, NULL, XmHORIZONTAL, 1
};
static ChoiceGizmo nisChoice = {
	0, "nisChoice", &nisMenu, G_TOGGLE_BOX
};
/* Comment: */
static InputGizmo _desc = {0, "desc", NULL, 8};
static GizmoRec descArray[] = {{InputGizmoClass, &_desc}};
static LabelGizmo descLabel = {
	0, "descLabel", label_desc, False, descArray, XtNumber(descArray),
	G_LEFT_LABEL
};
/* Show Other Options */
static MenuItems extraMenuItem[] = {  
	{TRUE, label_extra, NULL, I_TOGGLE_BUTTON, 0, SelExtraCB},
	{ NULL }
};
static MenuGizmo extraMenu = {
	0, "extraMenu", NULL, extraMenuItem, NULL, NULL, XmHORIZONTAL, 1
};
static ChoiceGizmo extraChoice = {
	0, "extraChoice", &extraMenu, G_TOGGLE_BOX
};
/* Extra Options */
/* Home Folder: */
static InputGizmo _home = {0, "home", NULL, 8};
static GizmoRec homeArray[] = {{InputGizmoClass, &_home}};
static LabelGizmo homeLabel = {
	0, "homeLabel", label_Home, False, homeArray, XtNumber(homeArray),
	G_LEFT_LABEL
};
/* User ID: */
static InputGizmo _uid = {0, "uid", NULL, 8};
static GizmoRec uidArray[] = {{InputGizmoClass, &_uid}};
static LabelGizmo uidLabel = {
	0, "uidLabel", label_Uid, False, uidArray, XtNumber(uidArray),
	G_LEFT_LABEL
};
/* Shell: */
static InputGizmo _shell = {0, "shell", NULL, 8};
static GizmoRec shellArray[] = {{InputGizmoClass, &_shell}};
static LabelGizmo shellLabel = {
	0, "shellLabel", label_Shell, False, shellArray, XtNumber(shellArray),
	G_LEFT_LABEL
};
/* X-Terminal: */
static InputGizmo _remote = {0, "remote", NULL, 8};
static GizmoRec remoteArray[] = {{InputGizmoClass, &_remote}};
static LabelGizmo remoteLabel = {
	0, "remoteLabel", label_remote, False, remoteArray,
	XtNumber(remoteArray), G_LEFT_LABEL
};
static GizmoRec array2[] = {
	{LabelGizmoClass,	&homeLabel},
	{LabelGizmoClass,	&shellLabel}
};
static GizmoRec array3[] = {
	{LabelGizmoClass,	&uidLabel},
	{LabelGizmoClass,	&remoteLabel}
};
static ContainerGizmo rc2 = {
	0, "rc2", G_CONTAINER_RC, 1, 1, array2, XtNumber(array2)
};
static ContainerGizmo rc3 = {
	0, "rc3", G_CONTAINER_RC, 1, 1, array3, XtNumber(array3)
};
Arg     arg3[] = {
	{XmNfractionBase,       6},
};
Arg     arg4[] = {
	{XmNorientation,	XmVERTICAL},
	{XmNleftAttachment,	XmATTACH_FORM},
	{XmNrightAttachment,	XmATTACH_POSITION},
	{XmNrightPosition,	3}
};
Arg     arg5[] = {
	{XmNorientation,	XmVERTICAL},
	{XmNrightAttachment,	XmATTACH_FORM},
	{XmNleftAttachment,	XmATTACH_POSITION},
	{XmNleftPosition,	3}
};
static GizmoRec array4[] = {
	{ContainerGizmoClass,	&rc2,		arg4, XtNumber(arg4)},
	{ContainerGizmoClass,	&rc3,		arg5, XtNumber(arg5)}
};
static ContainerGizmo rc4 = {
	0, "rc4", G_CONTAINER_FORM, 1, 1, array4, XtNumber(array4)
};
/* Groups: */
static ListGizmo glist = {
	0, "glist", NULL, 0, GROUPHEIGHT, SetGroup
};
static GizmoRec glistArray1[] = {
	{ListGizmoClass,	&glist}
};
static ContainerGizmo sw1 = {
	NULL, "sw1", G_CONTAINER_SW, 50, 0,
	glistArray1, XtNumber(glistArray1)
};
static GizmoRec glistArray2[] = {
	{ContainerGizmoClass,	&sw1}
};
static LabelGizmo glistLabel = {
	0, "glistLabel", label_glist, False, glistArray2,
	XtNumber(glistArray2), G_TOP_LABEL
};
/* Locales: */
static ListGizmo locale = {
	0, "locale", NULL, 0, GROUPHEIGHT, SetLocale
};
static GizmoRec localeArray1[] = {
	{ListGizmoClass,	&locale}
};
static ContainerGizmo sw2 = {
	NULL, "sw2", G_CONTAINER_SW, 50, 0,
	localeArray1, XtNumber(localeArray1)
};
static GizmoRec localeArray2[] = {
	{ContainerGizmoClass,	&sw2}
};
static LabelGizmo localeLabel = {
	0, "localeLabel", label_locales, False, localeArray2,
	XtNumber(localeArray2), G_TOP_LABEL
};
/* Selection */
static LabelGizmo _group = {
	0, "group", "", False, NULL, 0, G_LEFT_LABEL
};
/* Selection */
static LabelGizmo localetxt = {
	0, "localetxt", "", False, NULL, 0, G_LEFT_LABEL
};
static GizmoRec array5[] = {
	{LabelGizmoClass,	&glistLabel},
	{LabelGizmoClass,	&_group}
};
static GizmoRec array6[] = {
	{LabelGizmoClass,	&localeLabel},
	{LabelGizmoClass,	&localetxt}
};
static ContainerGizmo rc5 = {
	0, "rc5", G_CONTAINER_RC, 10, 10, array5, XtNumber(array5)
};
static ContainerGizmo rc6 = {
	0, "rc6", G_CONTAINER_RC, 10, 10, array6, XtNumber(array6)
};
static GizmoRec array7[] = {
	{ContainerGizmoClass,	&rc5,		arg4, XtNumber(arg4)},
	{ContainerGizmoClass,	&rc6,		arg5, XtNumber(arg5)}
};
static ContainerGizmo rc7 = {
	0, "rc7", G_CONTAINER_FORM, 10, 10, array7, XtNumber(array7)
};
static GizmoRec array8[] = {
	{ContainerGizmoClass,	&rc4,		arg3, XtNumber(arg3)},
	{ContainerGizmoClass,	&rc7,		arg3, XtNumber(arg3)}
};
static ContainerGizmo rc8 = {
	0, "rc8", G_CONTAINER_RC, 10, 10, array8, XtNumber(array8)
};
static GizmoRec propArray[] = {
	{ContainerGizmoClass,	&rc1,		arg1, XtNumber(arg1)},
	{LabelGizmoClass,	&ndsLabel},
	{ChoiceGizmoClass,	&nisChoice},
	{LabelGizmoClass,	&descLabel},
	{ChoiceGizmoClass,	&extraChoice},
	{ContainerGizmoClass,	&rc8,		arg2, XtNumber(arg2)}
};

/* Apply Reset Cancel Help */
static MenuItems prop_menu_item[] = {  
	MENU_ITEM(TRUE, ok,     0, applyCB,	NULL),
	MENU_ITEM(TRUE, reset,  0, resetCB,	NULL),
	MENU_ITEM(TRUE, cancel, 0, cancelCB,	NULL),
	MENU_ITEM(TRUE, help,   0, helpCB,	(XtPointer)&HelpProps),
	{ NULL }
};
static MenuGizmo prop_menu = {0, "prop_menu", NULL, prop_menu_item };
static PopupGizmo prop_popup = {
	0, "popup", string_propLine, (Gizmo)&prop_menu, propArray,
	XtNumber(propArray)
};
/********************************************************************
 * End of User property popup sheet
 */

void
CreatePropSheet(void)
{
	char **	list;
	int	i;
	char    uid_max[40];
	size_t  uid_len;

	list = (char **)malloc(locale_cnt*sizeof(char *));
	for (i=0; i<locale_cnt; i++) {
		list[i] = (char *)LocaleItems[i].label;
	}
	locale.items = list;
	locale.numItems = locale_cnt;
	glist.items = GroupItems;
	glist.numItems = g_cnt;
	g_popup = CreateGizmo(w_baseshell, PopupGizmoClass, &prop_popup, NULL, 0);
	w_popup = GetPopupGizmoShell(g_popup);
	XtAddCallback(w_popup, XmNpopdownCallback, hideExtraCB, NULL);
	w_login = QueryGizmo(PopupGizmoClass, g_popup, GGW, "login");
	w_nds = QueryGizmo(PopupGizmoClass, g_popup, GGW, "nds");
	w_nis = QueryGizmo(PopupGizmoClass, g_popup, GGW, "nisMenu:0");
	w_desc = QueryGizmo(PopupGizmoClass, g_popup, GGW, "desc");
	w_extra = QueryGizmo(PopupGizmoClass, g_popup, GGW, "extraMenu:0");
	w_home = QueryGizmo(PopupGizmoClass, g_popup, GGW, "home");
	w_uid = QueryGizmo(PopupGizmoClass, g_popup, GGW, "uid");
	w_shell = QueryGizmo(PopupGizmoClass, g_popup, GGW, "shell");
	w_remote = QueryGizmo(PopupGizmoClass, g_popup, GGW, "remote");
	w_glist = QueryGizmo(PopupGizmoClass, g_popup, GGW, "glist");
	w_group = QueryGizmo(PopupGizmoClass, g_popup, GGW, "group");
	w_locale = QueryGizmo(PopupGizmoClass, g_popup, GGW, "locale");
	w_localetxt = QueryGizmo(PopupGizmoClass, g_popup, GGW, "localetxt");
	w_prop_0 = QueryGizmo(PopupGizmoClass, g_popup, GGW, "prop_menu:0");

	/* Set lists such that only one item can be selected */
	XtVaSetValues(w_glist, XmNselectionPolicy, XmSINGLE_SELECT, 0);
	XtAddCallback(w_glist, XmNsingleSelectionCallback, SetGroup, 0);
	XtVaSetValues(w_locale, XmNselectionPolicy, XmSINGLE_SELECT, 0);
	XtAddCallback(w_locale, XmNsingleSelectionCallback, SetLocale, 0);
	
	/* Set max length for uid */
	sprintf(uid_max, "%ld", (long)UID_MAX);
        uid_len = strlen(uid_max);
	XtVaSetValues(w_uid, XmNmaxLength, (XtArgVal)uid_len, NULL);

	XtUnmanageChild(QueryGizmo(PopupGizmoClass, g_popup, GGW, "rc8"));
}

void
SetPropMenuItem(Boolean owner)
{
	prop_menu_item[0].sensitive  = (XtArgVal)owner;
}
