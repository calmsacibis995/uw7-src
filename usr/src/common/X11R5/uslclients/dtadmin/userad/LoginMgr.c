/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/LoginMgr.c	1.7.4.61"
#endif
/*
 *	LoginMgr - administer user accounts and groups, including "owner"
 */
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/secsys.h>		
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <limits.h>
#include <errno.h>
#include <deflt.h>
#include "LoginMgr.h"
#include <Xm/Protocols.h>	/* For XmAddWMProtocolCallback */
#include <Xm/MessageB.h>	/* For XmMessageBoxGetChild */
#ifdef DEBUG
#include <X11/Xmu/Editres.h>
#endif /* DEBUG */

#define ApplicationClass "LoginMgr"
#define ProgramName	"LoginMgr"
#define	ICON_NAME	"user48.glyph"
#define EOS		'\0'
#define WHITESPACE	" \t\n"
#define OWNER		"owner"
#define LOCALEHEIGHT	(XtArgVal)3
#define LOWEST_USER_UID	100
#define YPBINDPROC "ypbind"

#include "../dtamlib/dtamlib.h"

extern	char	*getenv();
extern	char	*GetXWINHome();
extern	struct	passwd	*getpwent();
extern	struct	group	*getgrent(), *getgrnam(), *getgrgid();
extern Widget  w_extctls[2];

extern void	CreatePropSheet();
extern void	SetPropMenuItem(Boolean);
extern void     SetNisUserValues();

static void	FileValueChangeCB(Widget, XtPointer, XtPointer);
static void	SetOwnerCB(Widget, XtPointer, XtPointer);
static void	SelNisGroupCB();
static void	GetGroupItems();
static void	GetUserItems();
static void	permCB();
static void	propCB();
static void	addCB();
static void	deleteCB();
void	exitCB();
void	helpCB();
void	applyCB();
void	resetCB();
void	cancelCB();
static void	yesCB();
static void	noCB();
void	OKCB();
void	OKCB2();
static void	applyPermCB();
static void	resetPermCB();
static void	cancelPermCB();
static void	SetViewCB();

static void	DisplayPrompt();
void	ErrorNotice();
void	WarnNotice();
static void	CheckPermList();
static Boolean	FileCheck();
static Boolean	DirCheck();
static Boolean	AddLoginEntry();
static int	DeleteLoginEntry();
static Boolean	InitPasswd();
Boolean removeWhitespace();
static void	ResetIconBox();
static void	SetPopupValues();
static void	SetGroupValues();
static void	Reselect();
void	reinitPopup();
static void	busyCursor();
static void	standardCursor();
int	cmpuid();
int	cmplogin();
int	cmpgroup();
int	cmplocale();
static void    DblClickCB();
static void    SingleClick(Widget, XtPointer, XtPointer);
extern void    CheckXdefaults(char *);
extern void    CheckProfile(char *);
extern void    ChangeXdefaults();
extern void    ChangeProfile();
extern void    ChangeLogin();
void           GetLocaleName(char *);
void           GetLocale(char *);
static void    CreateDayOneFile();
static void    SetDefaultLocale();
static void    GetDefaultName(char *);
static void    CheckIfNisUser(char *);
static void    CheckIfNisGroup(char *);
static void    CheckIfNisOn();
static void    SetNisGroupValues();
static void    SetMenuSensTrue();
static void    SetMenuSensFalse();
extern char  * GetMessage_Fmt(int, int, int);
static void    CheckIfUidLoggedOn(int, char *, char *);
static void    CheckIfNWS();

Gizmo	g_condelgrp = NULL;	/* Group delete confirmation */
Gizmo	g_condelete = NULL;	/* User delete confirmation */
Gizmo	g_warnnote = NULL;	/* Warning notice */
Gizmo	g_errnote = NULL;	/* Error notice */
Gizmo	g_conf = NULL;
Gizmo	g_base;		/* Gizmo handle for base window */
Gizmo	g_gpopup;	/* Group property sheet gizmo handle */
Gizmo	g_popup;	/* User property sheet gizmo handle */
Gizmo	g_perm;		/* Permissions popup */
Gizmo	g_checks;	/* Permissions menu */

extern LabelGizmo  ndsLabel;

Widget	w_swin;		/* Scrolling window in base window */
Widget	w_toplevel, w_iconbox, w_baseshell;
Widget	w_popup, w_gpopup, w_perm;
Widget	w_login, w_desc, w_home, w_group, w_glist = NULL, w_uid, w_shell;
Widget	w_localetxt, w_locale = NULL;
Widget	w_remote, w_gname, w_gid, w_own;
Widget	w_extra, w_nis, w_gnis, w_prop_0, w_nds;

FListPtr2       LocaleItems = (FListPtr2)0;

Screen		*theScreen;
Display		*theDisplay;

Options	options;

typedef enum _backupStatus
{ NoErrs, NoAttempt, FindErrs, CpioErrs } backupStatus;
backupStatus backupFiles();

typedef enum _action_menu_index 
{ action_exit } action_menu_index; 

static MenuItems action_menu_item[] = {
	MENU_ITEM(TRUE, exit,  0, exitCB, 0),
	{ NULL }
};

static MenuItems edit_menu_item[] = {
	MENU_ITEM(TRUE,  new,	0, addCB,	0),
	MENU_ITEM(FALSE, delete,0, deleteCB,	0),
	MENU_ITEM(FALSE, prop,  0, propCB,	0),
	MENU_ITEM(FALSE, perm,  0, permCB,	0),
	{ NULL }
};

static MenuItems view_menu_item[] = {
	MENU_ITEM(TRUE, users,   0, NULL, 0),
	MENU_ITEM(TRUE, groups,  0, NULL, 0),
	MENU_ITEM(TRUE, sysaccts,0, NULL, 0),
	{ NULL }
};

static HelpInfo HelpIntro	= { 0, "", HELP_PATH, help_intro };
static HelpInfo HelpGroups	= { 0, "", HELP_PATH, help_groups};
static HelpInfo HelpPerms	= { 0, "", HELP_PATH, help_perms };
static HelpInfo HelpDesk	= { 0, "", HELP_PATH, "HelpDesk"};
static HelpInfo HelpTOC		= { 0, "", HELP_PATH, "TOC" };

static MenuItems help_menu_item[] = {  
	MENU_ITEM(TRUE, intro, 0, helpCB, (char *)&HelpIntro),
	MENU_ITEM(TRUE, toc,   0, helpCB, (char *)&HelpTOC),
	MENU_ITEM(TRUE, hlpdsk,0, helpCB, (char *)&HelpDesk),
	{ NULL }
};

static MenuGizmo action_menu = {0, "action_menu", NULL, action_menu_item};
static MenuGizmo edit_menu   = {0, "edit_menu",   NULL, edit_menu_item};
static MenuGizmo view_menu   = {0, "view_menu",   NULL, view_menu_item, SetViewCB};
static MenuGizmo help_menu   = {0, "help_menu",   NULL, help_menu_item};

static MenuItems main_menu_item[] = {
	MENU_ITEM(TRUE, action, &action_menu, NULL, 0),
	MENU_ITEM(TRUE, edit,   &edit_menu, NULL, 0),
	MENU_ITEM(TRUE, view,   &view_menu, NULL, 0),
	MENU_ITEM(TRUE, help,   &help_menu, NULL, 0),
	{ NULL }
};
static MenuGizmo menu_bar = {0, "menu_bar", NULL, main_menu_item}; 

static IconBoxGizmo iconBox = {
	0, "iconbox", 0, 0, NULL, 0, SingleClick, DblClickCB, 0, 0
};
static GizmoRec icon [] = {
	{IconBoxGizmoClass,	&iconBox}
};

static MsgGizmo footer = {0, "footer", " ", " "};

static BaseWindowGizmo base = {0, "base", string_userBaseLine,
	(Gizmo)&menu_bar, icon, XtNumber(icon), &footer,
	string_iconName, ICON_NAME };


/********************************************************************
 * Gizmos for the group property popup
 */
static InputGizmo group_name = {0, "group_name", NULL, 8};
static GizmoRec inputArray1[] = {
	{InputGizmoClass,	&group_name}
};
static LabelGizmo group_name_label = {
	0, "group_name_label", label_gname, False, inputArray1,
	XtNumber(inputArray1), G_LEFT_LABEL
};
static InputGizmo group_id = {0, "group_id", NULL, 8};
static GizmoRec inputArray2[] = {
	{InputGizmoClass,	&group_id}
};
static LabelGizmo gid_label = {
	0, "gid_label", label_gid, False, inputArray2,
	XtNumber(inputArray2), G_LEFT_LABEL
};
static MenuItems nis_menu_item[] = {  
	{TRUE, label_nis_group, "", I_TOGGLE_BUTTON, 0, SelNisGroupCB},
	{ NULL }
};
static MenuGizmo nis_menu = {
	0, "nis_menu", NULL, nis_menu_item, NULL, NULL, XmHORIZONTAL, 1
};
static GizmoRec group_array[] = {
	{LabelGizmoClass,	&group_name_label},
	{CommandMenuGizmoClass,	&nis_menu},
	{LabelGizmoClass,       &gid_label}
};
static MenuItems group_menu_item[] = {  
	MENU_ITEM(TRUE, ok,     0, applyCB,	NULL),
	MENU_ITEM(TRUE, reset,  0, resetCB,	NULL),
	MENU_ITEM(TRUE, cancel, 0, cancelCB,	NULL),
	MENU_ITEM(TRUE, help,   0, helpCB,	(XtPointer)&HelpGroups),
	{ NULL }
};
static MenuGizmo group_menu = {0, "group_menu", NULL, group_menu_item };
static PopupGizmo group_popup = {
	0, "popup", string_groupLine, (Gizmo)&group_menu,
	group_array, XtNumber(group_array)
};
/********************************************************************
 * End of gizmos for the group property popup
 */

/********************************************************************
 * Start of permission property sheet
 */

/* Owner Privileges: */
static MenuItems ownerItems[] = {
	{TRUE, label_owner_id, "", I_TOGGLE_BUTTON, 0, 0, 0, True},
	{NULL}
};
static MenuGizmo ownerMenu = {
	0, "ownerMenu", NULL, ownerItems, SetOwnerCB, NULL, XmHORIZONTAL, 1, 1
};
static ChoiceGizmo ownerChoice = {0, "ownerChoice", &ownerMenu, G_TOGGLE_BOX};
/* Account <user> may: */
static LabelGizmo pcapLabel = {
	0, "pcapLabel", string_user_may, False, NULL, 0, G_LEFT_LABEL
};
static MenuGizmo permMenu = {
	0, "checks", " ", NULL, NULL, NULL, XmVERTICAL, 1, 1};
static ChoiceGizmo permChoice = {0, "permChoice", &permMenu, G_TOGGLE_BOX };
static GizmoRec array2[] = {{ChoiceGizmoClass,	&permChoice}};
static ContainerGizmo sc = {
	0, "sc", G_CONTAINER_SW, 0, 150, array2, XtNumber(array2)
};
static Arg     args[] = {
	{XmNscrollingPolicy,		XmAUTOMATIC},
};
static GizmoRec array1[] = {
	{ChoiceGizmoClass,	&ownerChoice},
	{LabelGizmoClass,	&pcapLabel},
	{ContainerGizmoClass,	&sc,		args, XtNumber(args)}
};

static MenuItems perm_menu_item[] = {  
	MENU_ITEM(TRUE, ok,	0, applyPermCB,	NULL),
	MENU_ITEM(TRUE, reset,  0, resetPermCB,	NULL),
	MENU_ITEM(TRUE, cancel, 0, cancelPermCB,NULL),
	MENU_ITEM(TRUE, help,   0, helpCB,	(XtPointer)&HelpPerms),
	{ NULL }
};
static MenuGizmo perm_menu = {0, "privileges", NULL, perm_menu_item };
static MsgGizmo foot = {0, "footer", " ", " "};
static PopupGizmo perm_popup = {
	0, "popup", string_permLine, &perm_menu, array1, XtNumber(array1),
	&foot
};
/********************************************************************
 * End of permission property sheet
 */


static MenuItems confirm_item[] = {
	MENU_ITEM(TRUE, yes, 0, 0, 0),
	MENU_ITEM(TRUE, no,  0, 0, 0),
	MENU_ITEM(TRUE, help,0, helpCB, (XtPointer)&HelpTOC),
	{ NULL }
};
static	MenuGizmo confirm_menu = {
	0, "note", "note", confirm_item, NULL, 0, XmHORIZONTAL, 1, 0, 1
};
static	ModalGizmo confirm = {
	0, "warn", string_confLine, &confirm_menu, "", NULL, 0,
	XmDIALOG_FULL_APPLICATION_MODAL, XmDIALOG_QUESTION
};

static MenuItems fileItems[] = {
	{TRUE, label_remove, mnemonic_remove, I_TOGGLE_BUTTON, 0, 0, 0, True},
	{TRUE, label_bkup, mnemonic_bkup, I_TOGGLE_BUTTON, 0, 0, 0, False},
	{ NULL }
};
static MenuGizmo fileMenu = {
	0, "note", "note", fileItems, FileValueChangeCB, 0, XmVERTICAL, 1, 1, 1
};
static ChoiceGizmo fileChoice = {
	0, "fileChoice", &fileMenu, G_TOGGLE_BOX
};
static GizmoRec array3[] = {
	{ChoiceGizmoClass,	&fileChoice}
};
static MenuItems condelgrp_item[] = {
	MENU_ITEM(TRUE, yes, 0, yesCB, 0),
	MENU_ITEM(TRUE, no,  0, noCB, 0),
	MENU_ITEM(TRUE, help,0, helpCB, (XtPointer)&HelpTOC),
	{ NULL }
};
static	MenuGizmo condelgrp_menu = {
	0, "note", "note", condelgrp_item, NULL, 0, XmHORIZONTAL, 1, 0, 1
};
static	ModalGizmo condelete= {
	0, "warn", string_confLine, &condelgrp_menu, "", array3,
	XtNumber(array3), XmDIALOG_FULL_APPLICATION_MODAL, XmDIALOG_QUESTION
};
static	ModalGizmo condelgrp ={
	0, "warn", string_confLine, &condelgrp_menu, "", NULL, 0,
	XmDIALOG_FULL_APPLICATION_MODAL, XmDIALOG_QUESTION
};

static MenuItems errnote_item[] = {
	MENU_ITEM(TRUE, ok,  0, OKCB, 0),
	{ NULL }
};
static	MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static	ModalGizmo errnote = {
	0, "warn", string_errLine, (Gizmo)&errnote_menu, "", NULL, 0,
	XmDIALOG_FULL_APPLICATION_MODAL, XmDIALOG_ERROR
};

static MenuItems warnnote_item[] = {
        MENU_ITEM(TRUE, ok,  0, OKCB2, 0),
        { NULL }
};
static  MenuGizmo warnnote_menu = {0, "note", "note", warnnote_item };
static  ModalGizmo warnnote = {
	0, "warn", string_warnLine, (Gizmo)&warnnote_menu, "", NULL, 0,
	XmDIALOG_MODELESS, XmDIALOG_WARNING
};


typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  XtArgVal	sensitive;
		  XtArgVal	selCB;
		  XtArgVal      subMenu;
} Items;

#define	N_FIELDS	5

struct	_FListItem {
	char	*label;
	Boolean	set;
       };

typedef struct _FListItem FListItem, *FListPtr;
char **	GroupItems = (char **)0;

char		*userLocale, *userLocaleName;
char		*curLocaleName, *curLocale;
String		defaultLocale,  defaultLocaleName;
char		*displayName = NULL;

typedef	struct	passwd	UserRec, *UserPtr;

UserPtr	u_list = (UserPtr)0;
UserPtr	u_reset;
int	u_cnt = 0;
int	uid_cnt = 0;
int	*uid_list;
int	owner_set=0;
int	group_list_index;	/* Index selected item in group list */
int	locale_list_index;	/* Index selected item in locale list */

typedef	struct	_grec {
		char	*g_name;
		gid_t	g_gid;
} GroupRec, *GroupPtr;

GroupPtr	g_list = (GroupPtr)0;
GroupPtr	g_reset;
int		g_cnt = 0;
int		max_gid = 0;

char            **locale_list;
int                 locale_cnt;

typedef	struct	{
	char	*label;
	char	*cmds;
	char	*help;
	Boolean	granted;
} PermRec, *PermPtr;

PermPtr	p_list = (PermPtr)0;
int	p_cnt = 0;

Dimension       xinch, yinch;

Boolean		I_am_owner, this_is_owner;

#define	MOTIF_DTM	-1
#define	NO_DTM		0
#define	OL_DTM		1

Boolean	dtm_account = True;
int	dtm_style = MOTIF_DTM;
Boolean	nis_user = FALSE;
Boolean	nis_group = FALSE;
Boolean	ypbind_on = FALSE;

int	view_type = USERS;

#define	KSH		"/usr/bin/ksh"
#define	BSH		"/usr/bin/sh"

static	char	*HOME_DIR	= "/home/";

static	char	*PRIV_TABLE	= "desktop/LoginMgr/PrivTable";
static	char	*PERM_FILE	= "desktop/LoginMgr/Users";
static	char	*ADMINUSER	= "/usr/bin/adminuser ";
static	char	*MAKE_OWNER	= "adm/make-owner";
static  char	*SET_LOGIN	= "adm/dtsetlogin";

static	char	*ADD_USER	= "/usr/sbin/useradd";
static	char	*DEL_USER	= "/usr/sbin/userdel";
static	char	*MOD_USER	= "/usr/sbin/usermod";
static	char	*ADD_GROUP	= "/usr/sbin/groupadd";
static	char	*DEL_GROUP	= "/usr/sbin/groupdel";
static	char	*MOD_GROUP	= "/usr/sbin/groupmod";
static	char	*AGE_UID	= "/etc/security/ia/ageduid";
static	char	*PRIV_FILE	= "/etc/security/tfm/users";

static	char	*ADD_DTUSER	= "adm/dtadduser";
static	char	*DEL_DTUSER	= "adm/dtdeluser";
static	char	*CHG_DTVAR	= "adm/olsetvar";
static	char	*UNSETVAR	= "adm/olunsetvar";
static  char    *DAYONE         = "desktop/LoginMgr/DayOne";
static  char    *PKGINFO        = "/usr/bin/pkginfo";

#define	LEAVE_HOME	0
#define	DEL_HOME	1
#define	BKUP_HOME	2
#define	XFER_HOME	4

int	home_flag = DEL_HOME;

#define WIDTH   (6*xinch)
#define HEIGHT  (3*yinch)

#define INIT_X  35
#define INIT_Y  20
#define INC_X   70
#define INC_Y   70
#define	MARGIN	20

char    *context, *operation;
char    *login, *desc, *home, *group, *remote=NULL, *shell, *uid, *gname, *gid;
char    *ndsname, *cur_ndsname;
int 	sethome = 1;
int     operation_num, context_num;
char    * retrieved_Fmt;

int	u_pending = 0;	    /* user/system view operation in progress */
int	g_pending = 0;      /* group view operation in progress */
int	exit_code;

/*
 *	many of the exit codes from useradd/mod/del and groupadd/mod/del are
 *	"impossible" because of prior validation of their inputs; these and
 *	any others that would be more mystifying than helpful to most users
 *	are reported with a "generic" error message.
 */
static	void
DiagnoseExit(char *op, char *type, char *name, int popup_type)
{
	char	buf[BUFSIZ];

	if (exit_code == 0)
		return;
	if (WIFEXITED(exit_code))
	    exit_code = WEXITSTATUS(exit_code);
	*buf = '\0';
	if (popup_type == GROUPS) {
		switch (exit_code) {
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:
                        retrieved_Fmt = 
                              GetMessage_Fmt(operation_num, context_num, 0);
                        sprintf(buf, GetGizmoText(retrieved_Fmt));
			break;
		case 47:
			sprintf(buf,GetGizmoText(string_noNis));
			break;
		case 48:
			sprintf(buf,GetGizmoText(string_groupNotFound),name);
			break;
		case 49:
			sprintf(buf,GetGizmoText(string_unkNisProblem));
			break;
		}
	}
	else {
		switch (exit_code) {
		case 8: if (u_pending != P_ADD) {
				sprintf(buf,GetGizmoText(string_inUse),
								op,type,name);
				break;
			}
			/* in the P_ADD case, this appears to be a symptom
			 * of permissions problems -- try to second guess.
			 */
		case 6:
                        retrieved_Fmt =                                         
                              GetMessage_Fmt(operation_num, context_num, 0);
                        sprintf(buf, GetGizmoText(retrieved_Fmt));
			break;
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:/*
			 *	shouldn't happen -- refers to error in groups
			 *	file, whereas LoginMgr doesn't use the -G flag
			 */
			break;
		case 11:
		case 12:
			sprintf(buf,GetGizmoText(string_badDir),op,type,name);
			break;
		case 15:
			sprintf(buf,GetGizmoText(string_badNDSName2), ndsname);
			break;
		case 16:
			sprintf(buf,GetGizmoText(string_dupNDSName), ndsname);
			break;
		case 47:
			sprintf(buf,GetGizmoText(string_noNis));
			break;
		case 48:
			sprintf(buf,GetGizmoText(string_notFound),name);
			break;
		case 49:
			sprintf(buf,GetGizmoText(string_unkNisProblem));
			break;
		}
	}
	if (*buf == '\0')
		sprintf(buf, GetGizmoText(string_unknown), op, type, name);
	ErrorNotice(buf, popup_type);
}


static void
DblClickCB(w, client_data, call_data)
         Widget          w;
         XtPointer       client_data;
         XtPointer       call_data;
{
         /*   Double clicking on an icon              */
         /*   will bring up the property window.      */

         propCB(w, client_data, call_data);
}


static void
setContext(int popup_type)
{
    switch (popup_type)
    {
    case GROUPS:
	context = GetGizmoText(tag_group);
	context_num = 2;
	break;
    case USERS:
	context = GetGizmoText(tag_login);
	context_num = 0;
	break;
    case RESERVED:
	context = GetGizmoText(tag_sys);
	context_num = 3;
	break;
    default:
	break;
    }	    
    return;
}
void
MoveFocus(Widget wid)
{
	XmProcessTraversal(wid, XmTRAVERSE_CURRENT);
}

void
resetFocus(int popup_type)
{
    Arg arg[5];

    switch (popup_type)
    {
    case GROUPS:
	XmProcessTraversal(w_gname, XmTRAVERSE_CURRENT);
	break;
    case USERS:
	/* FALL THRU */
    case RESERVED:
	XmProcessTraversal(w_login, XmTRAVERSE_CURRENT);
	break;
    }
    return;
}

static	void
FreeUserList(void)
{
	UserPtr	up;
	char	*p;

	free (uid_list);
	while (u_cnt--) {
		up = &u_list[u_cnt];
		if (p = up->pw_name)	free(p);
		if (p = up->pw_comment)	free(p);
		if (p = up->pw_dir)	free(p);
		if (p = up->pw_shell)	free(p);
	}
	free (u_list);
	u_list = (UserPtr)0;
	u_cnt = 0;
}

#define	BUNCH	16
#define	U_QUANT	(BUNCH*sizeof(UserRec))
#define	G_QUANT	(BUNCH*sizeof(GroupRec))

static	Boolean
MakeUserList(void)
{
static  time_t   lastListTime = 0;
struct  stat	 pwStat;
struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	int	n, ret;

        while ((ret = stat("/etc/passwd", &pwStat)) != 0 && errno == EINTR)
	       ;		/* try again */
        if (ret != 0)
	{
	    if (u_list)
		return False;
            else
		exit(1);
	}

        if (lastListTime >= pwStat.st_mtime)
	    return False;
        else
	    lastListTime = pwStat.st_mtime;
	if (u_list)
	        FreeUserList();
	while (pwd = _DtamGetpwent(DONT_STRIP,NIS_EXPAND,NULL)) {
		if (pwd->pw_dir == NULL || strlen(pwd->pw_dir) == 0 )
			continue;
		if (pwd->pw_uid > UID_MAX)
			continue;
		if (u_cnt == 0) {
			u_list = (UserPtr)malloc(U_QUANT);
		}
		else if (u_cnt % BUNCH == 0) {
			u_list = (UserPtr)realloc((void *)u_list,
						(1+(u_cnt/BUNCH))*U_QUANT);
		}
		up = &u_list[u_cnt++];
		up->pw_name = strdup(pwd->pw_name);
		up->pw_uid = pwd->pw_uid;
		up->pw_gid = pwd->pw_gid;
		up->pw_comment = pwd->pw_comment? strdup(pwd->pw_comment): NULL;
		up->pw_dir = pwd->pw_dir? strdup(pwd->pw_dir): NULL;
		up->pw_shell = pwd->pw_shell? strdup(pwd->pw_shell): NULL;
	}
	endpwent();
	if (uid_list = (int *)malloc(u_cnt*sizeof(int))) {
		for (n = 0; n < u_cnt; n++)
			uid_list[n] = u_list[n].pw_uid;
		/*
		 *	attach ageing uids to the list, so they won't
		 *	be chosen by default (this still requires a
		 *	a test in Validate(), as the user my try override
		 */
		if (fp=fopen(AGE_UID,"r")) {
			while (fgets(buf, 40, fp)) {
				uid_list = (int *)realloc(uid_list,
							sizeof(int)*(n+1));
				uid_list[n++] = atoi(buf);
			}
			fclose(fp);
		}
		uid_cnt = n;
		qsort((void *)uid_list, uid_cnt, sizeof(int), cmpuid);
	}
	qsort((void *)u_list, u_cnt, sizeof(UserRec), (int (*)())cmplogin);

        return True;
}

static	int
NextUid()
{
    register  int	n;

    if (MakeUserList())
	ResetIconBox();
    for (n = 1; n < uid_cnt; n++)
	if (uid_list[n]-uid_list[n-1] > 1)
	    if (view_type == RESERVED || uid_list[n-1] >= LOWEST_USER_UID - 1)
		break;
    return uid_list[n-1]+1;
}

static	void
FreeGroupList()
{
	char	*p;

/*	free(GroupItems);			-- this causes core dump; why?
/*	GroupItems = (char *)NULL;		-- I'm going to allow the leak.
*/	while (g_cnt--) {
		if (p = g_list[g_cnt].g_name)	free(p);
	}
	free (g_list);
	g_list = (GroupPtr)0;
	g_cnt = 0;
}

static	void
MakeGroupList(void)
{
struct	group	*gp;
	int	n;
	XmString	strings[100];
	XmString *	str;

	if (g_list)
		FreeGroupList();
	max_gid = 0;
	while (gp = _DtamGetgrent(DONT_STRIP,NIS_EXPAND,NULL)) {
		if (gp->gr_gid > UID_MAX)
			continue;
		if (g_cnt == 0)
			g_list = (GroupPtr)malloc(G_QUANT);
		else if (g_cnt % BUNCH == 0)
			g_list = (GroupPtr)realloc((void *)g_list,
						(1+(g_cnt/BUNCH))*G_QUANT);
		g_list[g_cnt].g_name = strdup(gp->gr_name);
		if ((g_list[g_cnt].g_gid  = gp->gr_gid) > max_gid)
			if (gp->gr_gid < UID_MAX-2)
			/*
			 * special case to filter out nobody,noaccess
			 */
				max_gid = gp->gr_gid;
		g_cnt++;
	}
	endgrent();
	qsort((void *)g_list, g_cnt, sizeof(GroupRec), (int (*)())cmpgroup);
	if (GroupItems = (char **)malloc(g_cnt*sizeof(char *)))
		for (n = 0; n < g_cnt; n++) {
                        if (strncmp(g_list[n].g_name,"+",1) == 0)
                             GroupItems[n] = g_list[n].g_name + 1;
                        else
                             GroupItems[n] = g_list[n].g_name;
		}
	if (w_glist) {
		str = strings;
		if (g_cnt > 100) {
			str = (XmString *)MALLOC(sizeof(XmString)*g_cnt);
		}
		for (n=0; n<g_cnt; n++) {
			str[n] = XmStringCreateLocalized(GroupItems[n]);
		}

		XtVaSetValues(w_glist,  XmNitems,		str,
					XmNitemCount,		g_cnt,
					XmNvisibleItemCount,	GROUPHEIGHT,
				  NULL);
		for (n=0; n<g_cnt; n++) {
			XmStringFree(str[n]);
		}
		if(str != strings) {
			XtFree((char*)str);
		}
	}
}

static	void
MakeLocaleList(void)
{
    int                 n, i, j, k;
    char               *lineptr;
    FILE               *fp;
    static char         localefile[BUFSIZ];
    char                labelbuf[BUFSIZ], line[BUFSIZ];
    Boolean             found = FALSE;
    String              LocaleLabel;

    /* First we find all locales */
    if (locale_list = (char **) FindLocales ())
    {
	for (locale_cnt=0; locale_list[locale_cnt++]; )
	    ; 
        if ( locale_cnt > 0 && (locale_list[locale_cnt-1] == NULL))
            locale_cnt = locale_cnt - 1; 

        if (LocaleItems = (FListPtr2)malloc(locale_cnt*sizeof(FListItem2)))
        {
                /* Put the C locale first */
             LocaleItems[0].label = (XtArgVal)GetGizmoText(string_AmericanEng); 
             LocaleItems[0].user_data = (XtArgVal)"C";
             LocaleItems[0].set = FALSE;

                 /* For every locale in list, get locale label */
                j = 1;
                for (n = 0; n < locale_cnt; n++) 
                {
                    if (strcmp (locale_list[n], "C") !=0 )
                    {

                    /* Open the ol_locale_def file */
                     sprintf( localefile, "%s/%s/ol_locale_def", 
                                              LocalePath,locale_list[n]);
                    if ((fp = fopen (localefile, "r")) != NULL)
                    {

                     while (fgets (line, BUFSIZ, fp))
                     {
                       lineptr = line;
                        if (strncmp (XNLLANGUAGE, lineptr, 13) == 0)
                         {
                            while (*lineptr != '\"' && *lineptr != '\n')
                                   lineptr++;
                            if (*lineptr == '\"')
                            {
                                 lineptr++;
                                 while (*lineptr == ' ')lineptr++;
                                 i=0;
                                 while (*lineptr != '\"' && *lineptr != '\n')
                                 labelbuf[i++]=*lineptr++;
                                 labelbuf[i] = '\0';
                                 LocaleLabel = strdup(GetGizmoText(labelbuf));
                                 found = TRUE;
                                 break;
                            } /* If first quote is found */

                          } /* If XNLLANGUAGE */

                      } /* While fgets */

                        if (found)
                            LocaleItems[j].label = (XtArgVal)LocaleLabel;
                        else
                            LocaleItems[j].label = (XtArgVal)locale_list[n];

                        LocaleItems[j].user_data = (XtArgVal)locale_list[n];
                        LocaleItems[j].set = FALSE;
                        j++;
                        fclose (fp);
                        found = FALSE;
                             
                     } /* If open */

                    } /* If not C locale */

                 } /*  Loop for every Locale */

        } /* If LocaleItems */

    } /* If locale_list */
 
  if ( locale_cnt != j )
               locale_cnt = j;
  qsort((void*)LocaleItems, locale_cnt, sizeof(FListItem2),
                                    (int (*)()) cmplocale);
  SetDefaultLocale();
 
}	


static	int
ChangeDTVars(char * node, Boolean dtm_flag, int style_flag)	/* 0 => no change, 1 => ok, -1 => bad */
{
	char	buf[BUFSIZ];
	Boolean	node_change = FALSE;

	*buf = EOS;

	if (remote==NULL || node==NULL)
		node_change = (remote != node);
	else
		node_change = strcmp(node,remote);

	if (dtm_flag != dtm_account) {
	/*
	 *	add or delete desktop environment
	 */
		if (dtm_flag == False) {
			operation = GetGizmoText(tag_delOp);
                        operation_num = 2;
			sprintf(buf, "%s %s", GetXWINHome(DEL_DTUSER), login);
		}
		else {
			operation = GetGizmoText(tag_addOp);
                        operation_num = 0;
			strcpy(buf, GetXWINHome(ADD_DTUSER));
			if (style_flag == MOTIF_DTM)
				strcat(buf, " -m");
			if (node)
				strcat(strcat(buf," -r "),node);
			strcat(strcat(buf, " "), login);
                        CreateDayOneFile();
		}
	}
	else {
		operation = GetGizmoText(tag_chgOp);
                operation_num = 1;
		*buf = EOS;
		if (dtm_flag && dtm_style != style_flag) {
			sprintf(buf, " %s XGUI %s %s ", GetXWINHome(CHG_DTVAR),
				style_flag==OL_DTM? "OPEN_LOOK": "MOTIF", login);
		}
		if (node_change) {
			if (*buf) {
				buf[0] = '(';
				buf[strlen(buf)-1] = ';';
			}
			if (node)
				sprintf(buf+strlen(buf), "%s REMOTE %s %s",
					GetXWINHome(CHG_DTVAR), node, login);
			else 
				sprintf(buf+strlen(buf), "%s REMOTE %s",
					GetXWINHome(UNSETVAR), login);
			if (*buf == '(')
				strcat(buf,")");
		}
	}
	if (*buf == EOS)	/* nothing to do */
	    return 0;
	exit_code = system(buf);
	return (exit_code == 0? 1: -1);
}


static	UserPtr
SelectedUser()
{
	int		last, count;
	int		i;
	UserPtr		p;

	XtVaGetValues(w_iconbox, XmNlastSelectItem, &last,
				 XmNselectCount,    &count,
				 NULL);
	if (count == 0)
		return (UserPtr)NULL;
	else {
		ExmVaFlatGetValues(w_iconbox, last, XmNuserData, &p, 0);
	}
	return (UserPtr)p;
}


static	void
noCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int pending = (view_type == GROUPS) ? g_pending : u_pending;
    Arg arg[5];

    FooterMsg("");
    if (pending == P_OWN)
	resetPermCB(wid, NULL, NULL);
    else if (pending == P_DEL2)
    {
	XtPopdown(GetModalGizmoShell(g_conf));
	return;
    }

    if (pending != P_DEL)
    {
	if (client_data == w_gpopup)
	    standardCursor(GROUPS);
	else 
	    if (client_data == w_popup) {
		standardCursor(USERS);

		/* if the user didn't enter a home folder, i.e. if we */
		/* filled in the default value for the user when they */
		/* clicked apply, then we need "unfill it in". */
		/* Otherwise if the user changes the login-id the code */
		/* won't fill in the default based on the new login-id */

		if (sethome == 0)
		{
		    sethome = 1;
		    home = HOME_DIR;
		    XmTextFieldSetString(w_home, home);
 		}
 	    }
	XtPopdown(GetModalGizmoShell(g_conf));
    }
    else if (view_type == GROUPS)
	XtPopdown(GetModalGizmoShell(g_condelgrp));
    else
	XtPopdown(GetModalGizmoShell(g_condelete));
}

static void
no2CB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    home_flag ^= DEL_HOME;
    yesCB(wid, client_data, call_data);
}

static	void
yesCB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    Gizmo	this_popup;
    Boolean	op_OK, flag;
    Boolean	update_iconbox = False;
    Boolean	dtm_flag;
    int		style_flag;
    int		result = 1, v_result = 0;
    char       *name, *node;
    char	buf[BUFSIZ];
    int		popup_type;
    int		pending;
    backupStatus status;
    Widget	w_dtm;

    if (client_data == NULL)
	popup_type = view_type;
    else
    {
	if ((Widget)client_data == w_gpopup)
	    popup_type = GROUPS;
	else
	    popup_type = (atoi(uid) < LOWEST_USER_UID) ? RESERVED : USERS;
    }

    setContext(popup_type);
    pending = (popup_type == GROUPS) ? g_pending : u_pending;

    noCB(wid, NULL, call_data); /*to do popdown */
    if (pending == P_OWN) {
	name = login;
	this_popup = g_perm;
    }
    else if (popup_type == GROUPS)
    {
	name = gname;
	this_popup = g_gpopup;
    }
    else {
	name = login;
	this_popup = g_popup;
	node = XmTextFieldGetString(w_remote);
	if (!*node)
	{
	    FREE(node);
	    node = NULL;
	}
	w_dtm = QueryGizmo(PopupGizmoClass, g_popup, GGW, "dtmMenu:0");
	dtm_flag = XmToggleButtonGadgetGetState(w_dtm);
	if (dtm_flag)
	{
	    dtm_flag = True;
            style_flag = MOTIF_DTM;
	}
    }
    switch (pending) {

    case P_OWN:
	sprintf(buf,"%s %s %s",GetXWINHome(MAKE_OWNER),owner_set?"":"-",name);
	op_OK = (system(buf) == 0);
	exit_code = 0;		/* don't diagnose specific failures */
	CheckPermList(SelectedUser());
	resetPermCB(w_own, NULL, NULL);
	MapGizmo(PopupGizmoClass, g_perm);
	break;
    case P_DEL:
	if (popup_type == GROUPS) {
            if (nis_group == True)
	        sprintf(buf, "%s %s%s", DEL_GROUP, "+",gname);
            else
	        sprintf(buf, "%s %s", DEL_GROUP, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	    break;
	}
	else {
	    status = backupFiles(&home_flag);
	    if (WIFEXITED(status))
		status = (backupStatus)WEXITSTATUS(status);
	    else
		status = CpioErrs;
	    if (home_flag & DEL_HOME) /* one "&" correct here */
	    {
		char notebuf[BUFSIZ];

		u_pending = P_DEL2;
		switch (status)
		{
		case NoErrs:
		    sprintf(notebuf, GetGizmoText(string_deleteFiles), home);
		    break;
		case NoAttempt:
		    sprintf(notebuf, GetGizmoText(string_notBackedUp),
			    name, home);
		    break;
		case FindErrs:
		    /* FALL THRU */
		case CpioErrs:
		    /* FALL THRU */
		default:
		    sprintf(notebuf, GetGizmoText(string_backupErrs),
			    name, home);
		    break;
		}
		DisplayPrompt(notebuf, w_popup);
		return;
	    }
	}
	/* FALL THRU */
    case P_DEL2:
	pending = u_pending = P_DEL;
	exit_code = DeleteLoginEntry(home_flag);
	/* just remove the permission file instead of calling
	 * dtdeluser since DeleteLoginEntry() does most of the
	 * work.  Also we don't care about the exit value here.
	 */
        sprintf(buf, "/sbin/rm -f %s/%s %s/%s", GetXWINHome(PERM_FILE), name,
                       GetXWINHome(DAYONE), name);
	system(buf);
	op_OK = (exit_code == 0);
	break;
    case P_ADD:
	if ((Widget)client_data == w_gpopup) {
            if (nis_group == True)
	        sprintf(buf, "%s %s%s", ADD_GROUP, "+", gname);
            else
	        sprintf(buf, "%s -g %s -o %s", ADD_GROUP, gid, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	}
	else {
	    if (op_OK = AddLoginEntry()) {
		update_iconbox = True;
		if (nis_user != True)
                       InitPasswd();
		if (dtm_flag) {
		    operation = GetGizmoText(tag_addOp);
                    operation_num = 0;
		    context = GetGizmoText(tag_desktop);
                    context_num = 1;
		    strcpy(buf, GetXWINHome(ADD_DTUSER));
		    if (style_flag == MOTIF_DTM)
			strcat(buf, " -m");
		    if (node)
			sprintf(buf+strlen(buf),
				" -r %s", node);
                    CreateDayOneFile();
		    sprintf(buf+strlen(buf)," %s", login);
		    op_OK = (system(buf) == 0);
		    exit_code = 0;
                    ChangeXdefaults();
                    ChangeProfile();
                    ChangeLogin();
		}
                if (dtm_flag == 0){
                   ChangeProfile();
                   ChangeLogin();
                }
	    }
	}
	break;
    case P_CHG:
	if (popup_type == GROUPS) {
            if (atoi(gid) != g_reset->g_gid ||
                                strcmp(gname,g_reset->g_name) != 0)
            {
                if (nis_group == True || strcmp(gname,g_reset->g_name) != 0
                        && strncmp(g_reset->g_name,"+",1) == 0 )
                                sprintf(buf, "%s ", MOD_GROUP);
                else
                    sprintf(buf, "%s -g %s -o ", MOD_GROUP, gid);
		if (nis_group == True || strcmp(gname,g_reset->g_name) != 0)
		{
                    if (strncmp(g_reset->g_name,"+",1) == 0)
		        name = g_reset->g_name + 1;
                    else
		        name = g_reset->g_name;

                    if ( nis_group == True)
		         sprintf(buf+strlen(buf),"-n %s%s ","+",gname);
                    else
		         sprintf(buf+strlen(buf),"-n %s ",gname);
		}
		strcat(buf, g_reset->g_name);
                if (nis_group == True && strncmp(g_reset->g_name,"+",1) == 0
                        && strcmp(gname,g_reset->g_name + 1) == 0)
                        exit_code = 0;
                else
                        exit_code = system(buf);
		op_OK = (exit_code == 0);
		result = op_OK? 1: -1;
	    }
	}
	else {
	    update_iconbox = op_OK = ((result=ChangeLoginProps())==1);
	    if (result >= 0) {
		v_result = ChangeDTVars(node, dtm_flag, style_flag);
                if (dtm_flag == 1)
                       ChangeXdefaults();
		if (v_result) {
		    op_OK = (v_result == 1);
		    context = GetGizmoText(tag_desktop);
		    context_num = 1;
		}
	    }
	}
	break;
    }
    if (result == 0 && v_result == 0) {
	sprintf(buf, GetGizmoText(string_noChange), login);
	SetPopupWindowLeftMsg(this_popup, buf);
    }
    else {
        retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, op_OK? 2: 3);
	sprintf(buf, GetGizmoText(retrieved_Fmt), name);
	FooterMsg(buf);
	if (pending == P_CHG && result == -1 && !I_am_owner) {
            retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, 0);
	    sprintf(buf, GetGizmoText(retrieved_Fmt));
	    ErrorNotice(buf, popup_type);
	}
	else if (exit_code)
	    DiagnoseExit(operation, context, name, popup_type);
	if (op_OK || update_iconbox)
	{
	    if (popup_type == GROUPS)
		MakeGroupList();
	    else
		MakeUserList();
	    ResetIconBox();
	    if (pending != P_DEL)
		Reselect((popup_type == GROUPS) ? gname: login);
	    if (pending == P_ADD)
		reinitPopup(popup_type);
            SetMenuSensFalse();
            XtPopdown(GetPopupGizmoShell(this_popup));
	}
    }
    standardCursor(popup_type);
}

static	void
DisplayPrompt (char *buf, Widget wid)
{
    Widget	w;
    static PFV	lastFunc = NULL;
    static XtPointer	lastCD;

    if (!g_conf) {
	g_conf = CreateGizmo(w_baseshell, ModalGizmoClass, &confirm, NULL, 0);
	w = GetModalGizmoDialogBox(g_conf);
    }

    SetModalGizmoMessage(g_conf, buf);
    w = GetModalGizmoDialogBox(g_conf);
    if (lastFunc != NULL) {
	XtRemoveCallback(w, XmNcancelCallback, lastFunc, lastCD);
        XtRemoveCallback(w, XmNokCallback, yesCB, lastCD);
    }
    lastFunc = wid == w_popup && u_pending == P_DEL2 ? no2CB : noCB;
    lastCD = wid;
    XtAddCallback(w, XmNcancelCallback, lastFunc, lastCD);
    XtAddCallback(w, XmNokCallback, yesCB, lastCD);
    MapGizmo(ModalGizmoClass, g_conf);
}

static	void
FileValueChangeCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	Widget	w;
	Boolean	set;
	char	buf[20];
	MenuGizmoCallbackStruct * cbs = (MenuGizmoCallbackStruct*)client_data;

	sprintf(buf, "note:%d", cbs->index);
	w = QueryGizmo(ModalGizmoClass, g_condelete, GGW, buf);
	XtVaGetValues(w, XmNset, &set, 0);

	if (set == True) {
		switch(cbs->index) {
			case 0:	home_flag |= DEL_HOME;
				break;
			case 1:	home_flag |= BKUP_HOME;
				break;
		}
	}
	else {
		switch(cbs->index) {
			case 0:	home_flag ^= DEL_HOME;
				break;
			case 1:	home_flag ^= BKUP_HOME;
				break;
		}
	}
}

static	void
ConfirmDelete(char *buf)
{
	Widget		w;

	if (view_type == GROUPS) {
		if (!g_condelgrp)
			g_condelgrp = CreateGizmo(
				w_baseshell, ModalGizmoClass, &condelgrp,
				NULL, 0
			);
		SetModalGizmoMessage(g_condelgrp, buf);
		MapGizmo(ModalGizmoClass, g_condelgrp);
		return;
	}
	if (!g_condelete) {
		g_condelete = CreateGizmo(
			w_baseshell, ModalGizmoClass, &condelete, NULL, 0
		);
	}
	w = QueryGizmo(ModalGizmoClass, g_condelete, GGW, "note:0");
	XtVaSetValues(w, XmNset, True, 0);
	w = QueryGizmo(ModalGizmoClass, g_condelete, GGW, "note:1");
	XtVaSetValues(w, XmNset, False, 0);
	SetModalGizmoMessage(g_condelete, buf);
	MapGizmo(ModalGizmoClass, g_condelete);
}

void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int popup_type = (int)client_data;
    Arg arg[5];

    if (popup_type != GROUPS && u_pending != P_DEL)
    {

	/* if the user didn't enter a home folder, i.e. if we */
	/* filled in the default value for the user when they */
	/* clicked apply, then we need "unfill it in". */
	/* Otherwise if the user changes the login-id the code */
	/* won't fill in the default based on the new login-id */

	if (sethome == 0)
	{
	    sethome = 1;
	    home = HOME_DIR;
	    XmTextFieldSetString(w_home, home);
	}
    }
    standardCursor(popup_type);
    XtPopdown(GetModalGizmoShell(g_errnote));
}


void
ErrorNotice (char *buf, int popup_type)
{
	Widget	w;

	if (!g_errnote)
		g_errnote = CreateGizmo(
			w_baseshell, ModalGizmoClass, &errnote, NULL, 0
		);

	SetModalGizmoMessage(g_errnote, buf);
	w = QueryGizmo(ModalGizmoClass, g_errnote, GGW, "note:0");
	XtVaSetValues(w, XmNclientData, (XtArgVal)popup_type, 0);
	MapGizmo(ModalGizmoClass, g_errnote);
}

void
WarnNotice (char *buf, int popup_type)
{
	Widget	w;

        if (!g_warnnote)
                g_warnnote = CreateGizmo(
			w_baseshell, ModalGizmoClass, &warnnote, NULL, 0
		);

        SetModalGizmoMessage(g_warnnote, buf);
	w = QueryGizmo(ModalGizmoClass, g_warnnote, GGW, "note:0");
	XtVaSetValues(w, XmNclientData, (XtArgVal)popup_type, 0);
        MapGizmo(ModalGizmoClass, g_warnnote);
}

void
OKCB2(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int popup_type = (int)client_data;

    standardCursor(popup_type);
    XtPopdown(GetModalGizmoShell(g_warnnote));
}



/* This routine is called when a user
 * selects the NIS button for groups. It will set
 * the button and change the sensitivity of 
 * several property fields.
 */
static  void
SelNisGroupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    char   buf[BUFSIZ];
    char   temp_gid[8];

	XtVaGetValues(w_gnis, XmNset, &nis_group, NULL);
	if ( nis_group == True ){
		sprintf(temp_gid, "%s", "");
		XmTextFieldSetString(w_gid, temp_gid);
		sprintf(buf, "%s\n", GetGizmoText(string_noGroupValues));
		FooterMsg(buf);
	}
	else
		FooterMsg("");
 
	SetNisGroupValues();
}


static	void
Reselect(itemname)
	char	*itemname;
{
    int		n = iconBox.numItems;
    char	*label;

    while (n--) {
	ExmVaFlatGetValues(w_iconbox, n, XmNlabelString, &label, NULL);
	ExmVaFlatSetValues(w_iconbox, n, XmNset,
			  strcmp(label,itemname)==0, NULL);
    }
    XtVaSetValues(w_iconbox, XmNitemsTouched, TRUE, NULL);
    if (view_type == GROUPS)
    {
	g_reset = (GroupPtr)SelectedUser();
	SetGroupValues(g_reset);
    }
    else
    {
	u_reset = SelectedUser();
	SetPopupValues(u_reset);
    }
}
static	backupStatus
backupFiles(int *flag)
{
    char buf[BUFSIZ];

    if (*flag & BKUP_HOME)
    {
	if (access(home,R_OK) == -1)
	{
	    *flag = LEAVE_HOME;
	    return NoAttempt;
	}
	/* LoginMgr has more privs than MediaMgr so use tfadmin to */
	/* invoke MediaMgr with only the privs it needs */
	sprintf(buf, "/sbin/tfadmin MediaMgr -B -L -C %s", home);
	return ((backupStatus)system(buf));
    }
    return NoErrs;
}


/*
 *	DeleteLoginEntry invokes /usr/sbin/userdel; flag indicates disposal
 *	of the home directory -- currently it is left alone OR just deleted.
 */
static	int
DeleteLoginEntry(flag)
	int	flag;
{
	char	*ptr, buf[BUFSIZ];

	/* needed so login is removed from mail list for pkgadd */
	sprintf(buf, "if /sbin/tfadmin -t LoginMgr >/dev/null 2>&1 ;then %s - %s;fi", 
		GetXWINHome(MAKE_OWNER), login);
	system(buf);
        if (nis_user == True)
	sprintf(buf, "%s -d %s; %s %s%s%s",  ADMINUSER, login,
		DEL_USER, flag&DEL_HOME? "-r ": "", "+",login); 
        else
	sprintf(buf, "%s -d %s; %s %s%s",  ADMINUSER, login,
		DEL_USER, flag&DEL_HOME? "-r ": "", login); 
	return (system(buf));
}

static	Boolean
AddLoginEntry()
{
	char	buf[BUFSIZ];

        if (nis_user == True){
	    if (*ndsname)
                sprintf(buf, "%s -n \"%s\" -m %s%s", ADD_USER, ndsname,         
                        "+",login);
	    else
	    	sprintf(buf, "%s -m %s%s", ADD_USER, "+",login);
	}
        else{
	    sprintf(buf, "%s -m -c \"%s\" -u %s -o ", ADD_USER, desc, uid);
	    if (*home)
		sprintf(buf+strlen(buf), "-d %s ", home);
	    if (*ndsname)
		sprintf(buf+strlen(buf), "-n \"%s\" ", ndsname);
	    if (strcmp(group,"other"))
		sprintf(buf+strlen(buf), "-g %s ", group);
	    if (*shell)
		sprintf(buf+strlen(buf), "-s %s ", shell);
	    strcat(buf, login);
        }
	exit_code=system(buf);
	return (exit_code==0);
}

/*
 *	the new account should be set up with a password; for now, it is
 *	done through a rather crude invocation of passwd(1) via xterm.
 */

static	Boolean
InitPasswd(void)
{
	char	buf[BUFSIZ];
	char	display_buf[BUFSIZ];

	if (displayName != NULL){
       		sprintf(display_buf,"DISPLAY=%s", displayName);
		putenv(display_buf);
	}
       	sprintf(buf,"exec xterm -geometry 40x6 -T \"%s\" -e /usr/bin/passwd ", 
		GetGizmoText(string_passwdTitle));
	strcat(buf, login);
	return (system(buf)==0);
}

static	int
ChangeLoginProps(void)
{
	Boolean	change = FALSE, change_startup = FALSE, change_login = FALSE;
        Boolean change_locale = FALSE, basic_change = FALSE;
	int	i, n;
	char	buf[BUFSIZ];
	char	buf2[BUFSIZ];

	strcpy(buf, MOD_USER);
        if (nis_user == True)
            sprintf(buf2,"%s%s","+",login);
        else
            sprintf(buf2,"%s",login);
            
	if (strcmp(buf2,u_reset->pw_name)) {
		sprintf(buf+strlen(buf), " -l %s ", buf2);
		change = TRUE;
		basic_change = TRUE;
		change_login = TRUE;
	}
	if (strcmp(ndsname,cur_ndsname)) {
		sprintf(buf+strlen(buf), " -n \"%s\" ", ndsname);
		change = TRUE;
		basic_change = TRUE;
        }
        if (nis_user == False){
	   if (strcmp(desc,u_reset->pw_comment)) {
		   sprintf(buf+strlen(buf), " -c \"%s\" ", desc);
		   change = TRUE;
		   basic_change = TRUE;
	   }
	   if (u_reset->pw_uid != (i=atoi(uid))) {
		   sprintf(buf+strlen(buf), " -u %s -U ", uid);
		   for (n = 0; n < u_cnt; n++) {
			   if (u_list[n].pw_uid == i) {
				   strcat(buf, "-o ");
				   break;
			   }
		   }
		   change = TRUE;
		   basic_change = TRUE;
	   }
	   if (*group) {
		   struct	group	*gp = getgrgid(u_reset->pw_gid);
		   if (gp == NULL || strcmp(group, gp->gr_name)) {
			sprintf(buf+strlen(buf), " -g %s ", group);
			change = TRUE;
		        basic_change = TRUE;
		   }
	   }
	   if (strcmp(home,u_reset->pw_dir)) {
		   sprintf(buf+strlen(buf), " -d %s -m ", home);
		   change = TRUE;
		   basic_change = TRUE;
	   }

	if (strcmp(shell,u_reset->pw_shell)) {
		sprintf(buf+strlen(buf), " -s %s ", shell);
		change = TRUE;
		change_startup = TRUE;
		basic_change = TRUE;
	}

       } /* If not nis_user */
        
        if (userLocaleName != curLocaleName){
                change = TRUE;
                change_locale = TRUE;
        }

	if (change && I_am_owner) {
               if (basic_change == TRUE) {
		  strcat(buf, u_reset->pw_name);
		  exit_code=system(buf);
                }

                if (change_locale == TRUE) {
                   ChangeXdefaults();
                   ChangeProfile();
                   ChangeLogin();
                 }
                if (exit_code == 0 && change_login) {
                    if(strncmp(u_reset->pw_name,"+",1) != 0 &&
                         nis_user == False && dtm_account == True){
                        /* transfer the permission file */
                        sprintf(buf, "/sbin/mv %s/%s %s/%s", 
				GetXWINHome(PERM_FILE), u_reset->pw_name, 
				GetXWINHome(PERM_FILE), login);
                        exit_code = system(buf);

                        if (exit_code == 0) {
                                /* transfer the privileges file */
                            sprintf(buf, "/sbin/mv %s/%s %s/%s", 
					PRIV_FILE, u_reset->pw_name, 
					PRIV_FILE, login);
                                exit_code = system(buf);
                        }

                        if (exit_code == 0) {
                                /* transfer the dayone file */
                            sprintf(buf, "/sbin/mv %s/%s %s/%s", 
					GetXWINHome(DAYONE), u_reset->pw_name, 
					GetXWINHome(DAYONE), login);
                            exit_code = system(buf);
                        }

                    } /* if strncmp */
                }

		/* if shell changed (e.g. Korn shell to C shell) then */
		/* the startup file may be different (e.g. .profile   */
		/* vs. .login) so make sure the startup file for the  */
		/* new shell invokes .olsetup when user logs in.      */

		if (exit_code == 0 && change_startup) {
		    sprintf(buf, "%s %s %s %s %s", GetXWINHome(SET_LOGIN),
			    home, shell, group, login);
		    exit_code = system(buf);
		}
		return (exit_code==0? 1: -1);
	}
	else if (change) {
		return -1;
	}
	else
		return 0;
}

static	void
ResetIconBox(void)
{
	Gizmo		g;

	if (view_type == GROUPS) {
		GetGroupItems();
	}
	else {
		GetUserItems();
	}

	/* Get new object list */
	g = (Gizmo)QueryGizmo(BaseWindowGizmoClass, g_base, GGG, "iconbox");
	/* Put new list into a container */
	ResetIconBoxGizmo(g, &iconBox, NULL, 0);
	w_iconbox = (Widget)QueryGizmo(
		BaseWindowGizmoClass, g_base, GGW, "iconbox"
	);
}

char	vld_msg[BUFSIZ];
char	*vfmt = NULL;
char	*vfmt2 = NULL;

static	Boolean
Validate(void)
{
	struct	group	*gp;
	Boolean	valid = TRUE;
	char	*ptr, buf[BUFSIZ], *start;
	int		gidno, n, bits, chg_id;
	register char	*nds_ptr = ndsname;

	if (vfmt == NULL)
		vfmt = GetGizmoText(format_syntaxFmt);
	if (vfmt2 == NULL)
		vfmt2 = GetGizmoText(format_syntaxFmt2);
	*vld_msg = '\0';

	(void)removeWhitespace(login);
	(void)removeWhitespace(ndsname);
	if (*login == '\0') {
		valid = FALSE;
		sprintf(vld_msg, GetGizmoText(format_noIdFmt),
		    GetGizmoText(label_login));
	}
	else if (ptr = strpbrk(login, " :")) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(login,"+", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '+',
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(login,"-", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '-',
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(u_reset->pw_name,"+", 1) == 0 &&
	    	strcmp(login, u_reset->pw_name + 1) != 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg),"%s\n", 
			GetGizmoText(string_badLogin));
	}
	else if (strncmp(u_reset->pw_name,"+", 1) != 0 &&
		nis_user == True && strcmp(login, u_reset->pw_name ) != 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg),"%s\n", 
			GetGizmoText(string_badLogin));
	}
	else if (u_reset == NULL || strcmp(login, u_reset->pw_name))
	{   /* new or modified login name; dis-allow duplicates. */
		int index;

		for (index = 0; index < u_cnt; index++)
			if (strcmp(login, u_list[index].pw_name) == 0)
			{
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg), 
					GetGizmoText(format_inUse),
				    	GetGizmoText(label_login), login);
				break;
			}
	}
	if (*ndsname) {
		for ( ; *nds_ptr != NULL; nds_ptr++) {
                	if (!isprint(*nds_ptr) || (*nds_ptr == ':')){
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
					GGT(format_syntaxFmt3),
					*nds_ptr, GetGizmoText(label_nds),
						 ndsname);
				break;
			}
        	}
	}
	if (*ndsname) {
		if (strncmp(ndsname,"CN=", 3) == 0) {
			if (strlen(ndsname) > NDSFULNAME_MAX){
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
					GGT(format_maxFullNDSFmt),
				 	NDSFULNAME_MAX);
			}
		}
		else{
			if (strlen(ndsname) > NDSNAME_MAX){
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
					GGT(format_maxNDSFmt),
				 	NDSNAME_MAX);
			}
		}
	}
	if ( nis_user == False ){
		if (strchr(desc, ':')) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
			    GetGizmoText(label_desc), desc);
		}
		if (strchr(home,':')) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
			    GetGizmoText(label_Home), home);
		}
		if ( *uid == '\0') {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_noIdFmt),
			    	GetGizmoText(label_Uid));
		}
		if ((n = strspn(uid,"0123456789")) != strlen(uid)) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, uid[n],
			    GetGizmoText(label_Uid), uid);
		}
		else if (atol(uid) < LOWEST_USER_UID) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_userMinIdFmt));
		}
		else if (atol(uid) > UID_MAX) { /* defined in <limits.h> */
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_maxIdFmt),
			    	GetGizmoText(label_Uid), UID_MAX);
		}
		else {			/* check for uid being aged */
			sprintf(buf,"/usr/bin/grep '^%s:' %s >/dev/null 2>&1",
					uid,AGE_UID);
			if (system(buf) == 0) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_ageIdFmt),
				    GetGizmoText(label_Uid), uid);
			}
		}
		if (*group) {
			if (gp = getgrnam(group))
				gidno = gp->gr_gid;
			else {
				gidno = 1;
				valid = FALSE;
				strcat(vld_msg, GetGizmoText(string_badGroup));
			}
		}

		/* validate the access permission on the HOME value */
		bits = R_OK | W_OK | X_OK;
		if ( u_pending == P_CHG && (int)(u_reset->pw_uid) != \
							(chg_id=atoi(uid)) ){
			if (!DirCheck(home, bits, u_reset->pw_uid, 
							 u_reset->pw_gid) ) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_noaccessFmt),
				    login, GetGizmoText(label_Home), home);
			}
		}
		else {
			if (!DirCheck(home, bits, atoi(uid), gidno)) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_noaccessFmt),
				    login, GetGizmoText(label_Home), home);
			}
		}

		(void)removeWhitespace(shell);
		if (*shell == EOS)		/* shell field is empty */
		{
			valid = FALSE;
			strcat(vld_msg,GetGizmoText(string_shellRequired));
		}
		else if ( FileCheck("", shell, X_OK, atoi(uid), gidno)==FALSE)
		{
			valid = FALSE;
			strcat(strcat(vld_msg,GetGizmoText(string_badShell)), 
                                              shell);
		}
	}
	return valid;
}

static	Boolean
ValidateGroup(char *name, char *gidno)
{
	Boolean	valid = TRUE;
	char	*ptr;
	int	n;

	if (vfmt == NULL)
		vfmt = GetGizmoText(format_syntaxFmt);
	if (vfmt2 == NULL)
		vfmt2 = GetGizmoText(format_syntaxFmt2);
	*vld_msg = '\0';
	if (*name == '\0') {
		valid = FALSE;
		sprintf(vld_msg, GetGizmoText(format_noIdFmt),
				 GetGizmoText(label_gname));
	}
	else if (ptr = strpbrk(name, " :")) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
					GetGizmoText(label_gname), name);
	}
        else if (strncmp(name,"+", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '+',
					GetGizmoText(label_gname), name);
	}
        else if (strncmp(name,"-", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '-',
					GetGizmoText(label_gname), name);
	}
        if (nis_group == False){
	if (*gid == '\0') {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_noIdFmt),
			GetGizmoText(label_gid));
	}
	if ((n = strspn(gid,"0123456789")) != strlen(gid)) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, gid[n],
					GetGizmoText(label_gid), gid);
	}
	/* LOWEST_USER_UID applies to gid too */
	else if (atol(gid) < LOWEST_USER_UID) {	
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_minIdFmt));
	}
	/* UID_MAX defined in <limits.h> applies to gid too */
	else if (atol(gid) > UID_MAX) {	
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_maxIdFmt),
				GetGizmoText(label_gid), gid, UID_MAX);
	}
	else
	{
	    for (n = 0; n < g_cnt; n++)
                if (strcmp(g_list[n].g_name, name) == 0)
		{
		    /* if just changing gid then name will be in list */
		    if (g_reset != NULL && g_list[n].g_gid == g_reset->g_gid) 
			break;

		    valid = FALSE;
		    sprintf(vld_msg+strlen(vld_msg),
			    GetGizmoText(format_inUse),
			    GetGizmoText(label_gname), name);
		    break;
		}
	}
	    
       }
	return valid;
}

static void
CheckDuplHome(char * home_dir, char *buf)
{
	int	n;
	/* Check for existing home folder */
	for (n = 0; n < u_cnt; n++)
		if (strcmp(home_dir, u_list[n].pw_dir) == 0) {
			sprintf(buf+strlen(buf), 
				GetGizmoText(format_sharehomeFmt),
				GetGizmoText(label_Home), home_dir);
			break;
		}
}

static	void
CheckDuplicate(int uidno, char *buf)
{
	int	n;

	for (n = 0; n < uid_cnt; n++)
		if (uidno == uid_list[n]) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_Uid), uidno);
			break;
		}
}

static	void
CheckDuplGroup(int gidno, char *buf)
{
	int	n;

	for (n = 0; n < g_cnt; n++)
		if (gidno == g_list[n].g_gid) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_gid), gidno);
			break;
		}
}

void
applyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int		idno;
    char       *name;
    char	buf[PATH_MAX + BUFSIZ];
    Widget	shell_wid;
    int		pending;
    int		popup_type;

    if ((shell_wid = DtGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;
    if (shell_wid == w_gpopup)
	popup_type = GROUPS;
    else
	popup_type = USERS;	/* for now... see below */
    FooterMsg("");
    busyCursor(popup_type);

    if (popup_type == GROUPS)
    {
	/* pending may be wrong if another operation such as P_DEL was */
	/* invoked after the properties sheet was opened so reset it. */
	if (g_reset != NULL) 
	    g_pending = P_CHG;
	else
	    g_pending = P_ADD;
	pending = g_pending;
	context = GetGizmoText(tag_group);
	context_num = 2;
	SetPopupWindowLeftMsg(g_gpopup, "");
	gname = XmTextFieldGetString(w_gname);
	removeWhitespace(gname);
	name = gname;
	gid = XmTextFieldGetString(w_gid);
	idno = atoi(gid);
	if (!ValidateGroup(name, gid))
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    XtFree(gid);
	    XtFree(gname);
	    return;
	}
    }
    else
    {
	SetPopupWindowLeftMsg(g_popup, "");
	/* pending may be wrong if another operation such as P_DEL was */
	/* invoked after the properties sheet was opened so reset it. */
	if (u_reset != NULL) 
	    u_pending = P_CHG;
	else
	    u_pending = P_ADD;
	pending = u_pending;
	uid = XmTextFieldGetString(w_uid);
	idno = atoi(uid);
        if ( nis_user == True){
	    popup_type = USERS;
	    context = GetGizmoText(tag_login);
	    context_num = 0;
        }
        else{
	    if (idno < LOWEST_USER_UID)
	    {
	        popup_type = RESERVED;
	        context = GetGizmoText(tag_sys);
	        context_num = 3;
	    }
	    else
	    {
	    popup_type = USERS;
	    context = GetGizmoText(tag_login);
	    context_num = 0;
	    }
        }
	login = XmTextFieldGetString(w_login);
	name = login;
	ndsname = XmTextFieldGetString(w_nds);
	desc = XmTextFieldGetString(w_desc);
	home = XmTextFieldGetString(w_home);
	removeWhitespace(home);
	if (*home == EOS)
	    home = HOME_DIR;
	if (strcmp(home,HOME_DIR)==0) {
	    strcat(strcpy(buf,home),login);
	    home = strdup(buf);		/* memory leak */
	    XmTextFieldSetString(w_home, home);
	    sethome = 0;
	}
	shell = XmTextFieldGetString(w_shell);
        
	if (!Validate())
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    return;
	}
    }

    operation = (pending == P_ADD)? GetGizmoText(tag_addOp):
    GetGizmoText(tag_chgOp);
    operation_num = (pending == P_ADD)? 0: 1;

    if (pending == P_CHG)
    {
	if (shell_wid == w_gpopup){
            if (strncmp(g_reset->g_name,"+",1) == 0)
	         name = g_reset->g_name + 1;
            else
                 name = g_reset->g_name;
        }
        else{
            if (strncmp(u_reset->pw_name,"+",1) == 0)
                 name = u_reset->pw_name +1;
            else
                 name = u_reset->pw_name;
       }
    }

    retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, 1);
    sprintf(buf, GetGizmoText(retrieved_Fmt), name);
    if (shell_wid == w_gpopup) {
	if (pending == P_ADD || idno != g_reset->g_gid)
            if (nis_group == False)
	         CheckDuplGroup(idno, buf);
    }
    else {
       if ( nis_user == False ){
           if ( pending == P_ADD || idno != (int)u_reset->pw_uid)
		CheckDuplicate(idno, buf);
           if (pending == P_ADD || (strcmp(home, u_reset->pw_dir) != 0))
		CheckDuplHome(home, buf);
           if ( pending == P_CHG && idno != (int)u_reset->pw_uid ) 
                CheckIfUidLoggedOn(u_reset->pw_uid, buf, name);
       }
    }
    DisplayPrompt(buf, shell_wid);
}


/*
 *	Check if user can access (read/write/execute) the HOME directory. 
 */
static Boolean
DirCheck(char *dir, int bits, int uid, int gid)
{
struct	stat	stbuf;
static  uid_t	priv_uid;
static	Boolean first_time = TRUE;
        ushort  mode = 0;

	if (first_time == TRUE) {
		first_time = FALSE;
		priv_uid = (uid_t)secsys(ES_PRVID, 0);
	}
	/* If the file does not exist, return TRUE since it will be created */
	if (stat(dir, &stbuf) != 0)
		return TRUE;
	else {
                if (uid == priv_uid)
                        return TRUE;
                else if (uid == stbuf.st_uid){
                        mode = stbuf.st_mode & S_IRWXU;
                        return((mode & stbuf.st_mode) == S_IRWXU);
                }
                else if (gid == stbuf.st_gid){
                        mode = stbuf.st_mode & S_IRWXG;
                        return((mode & stbuf.st_mode) == S_IRWXG);
                }
                else{
                        mode = stbuf.st_mode & S_IRWXO;
                        return((mode & stbuf.st_mode) == S_IRWXO);
                }
	}
}

/*
 *	check if user can access file;  Access can't be used here, as
 *      I am checking on someone ELSE's permissions. 
 *	FileCheck will return FALSE if the file does not exist, or if
 *	user does not have requested access.  
 */
static	Boolean
FileCheck(char *dir, char *base, int bits, int uid, int gid)
{
struct	stat	stbuf;
	char	path[PATH_MAX];
static  uid_t	priv_uid;
static  Boolean first_time = TRUE;

        if (first_time == TRUE)
	{
	    first_time = FALSE;
	    /*	If we are using a uid based privilege mechanism
	     *	and then the privileged uid has access to all files.
	     *  otherwise secsys will return -1, an invalid uid.
	     */
	    priv_uid = (uid_t)secsys(ES_PRVID, 0);
	}

	sprintf(path,"%s/%s", dir, base);
	if (stat(path,&stbuf) != 0)
		return FALSE;
	else
	{
	        if (uid == priv_uid) 
			return TRUE;
		if (uid == stbuf.st_uid)
			bits *= S_IXUSR;
		else if (gid == stbuf.st_gid)
			bits *= S_IXGRP;
		return ((bits & stbuf.st_mode) == bits);
	}
}

/*
 *	check for presence of .olsetup, and determine XGUI and REMOTE values
 */
static	void
CheckIfDtm(int uid, int gid, char *homedir)
{
    char	*start, *end, buf[BUFSIZ];
    FILE	*fp;

    if (!FileCheck(homedir, ".olsetup",  R_OK, uid, gid) ||
	!FileCheck(homedir, ".dtfclass", R_OK, uid, gid))
    {
	dtm_account = False;
        CheckProfile(homedir);
	return;
    }
    sprintf(buf, "%s/%s", homedir, ".olsetup");
    if (fp = fopen(buf,"r"))
    {
	dtm_account = True;
	while (fgets(buf, BUFSIZ, fp)) {
	    for (start=buf; isspace(*start); start++)
		;
	    if (strncmp(start,"XGUI=",5)==0) {
		dtm_style = strncmp(start+5,"MOTIF", 5)? OL_DTM: MOTIF_DTM;
		continue;
	    }
	    if (strncmp(start,"REMOTE=", 7) == 0) {
		for (end = start+7; !isspace(*end); end++)
		    ;
		*end = '\0';
		if (start[7])
		    remote = strdup(start+7);
	    }
	}
	fclose(fp);

       /* Now check the user's .Xdefaults file */
      CheckXdefaults(homedir);

    }
    else {
	dtm_account = False;	/* FIX: is this line needed? */
        CheckProfile(homedir);
    }
    return;
}


/* This routine checks if the user
 * is a NIS user and call SetNisUserValues
 */
static  void
CheckIfNisUser(char *ulogin)
{

    if (strncmp(ulogin,"+",1)){
        nis_user = FALSE;
        SetNisUserValues();
    }
    else{
        nis_user = TRUE;
        SetNisUserValues();
    }
}


/* This routine checks if the group
 * is a NIS group and call SetNisGroupValues
 */
static  void
CheckIfNisGroup(char *ugroup)
{

    if (strncmp(ugroup,"+",1)){
        nis_group = FALSE;
        SetNisGroupValues();
    }
    else{
        nis_group = TRUE;
        SetNisGroupValues();
    }
}


/* This routine checks if the ypbind
 * daemon is running (i.e. NIS installed and
 * running)
 */
static  void
CheckIfNisOn()
{

        char    buf[BUFSIZ];

        sprintf(buf, "ps -ef | grep \"%s\" | grep -v grep >/dev/null 2>&1", 
                                                               YPBINDPROC);
        if (system(buf) == 0){
            XtVaSetValues(w_nis, XmNsensitive, (XtArgVal)True, NULL);
            XtVaSetValues(w_gnis, XmNsensitive, (XtArgVal)True, NULL);
            ypbind_on = True;
        }
        else{
            XtVaSetValues(w_nis, XmNsensitive, (XtArgVal)False, NULL);
            XtVaSetValues(w_gnis, XmNsensitive, (XtArgVal)False, NULL);
            ypbind_on = False;
        }
            
}

/* This routine checks if the NWS
 * package is installed. 
 */
static  void
CheckIfNWS()
{

		char	buf[BUFSIZ];
        static	Widget	w_ndsLabel = NULL;

	if (w_ndsLabel == NULL){
		w_ndsLabel = (Widget) QueryGizmo (
                	LabelGizmoClass, g_popup, GetGizmoWidget, "ndsLabel");
	}
	sprintf(buf, "%s -q %s 2>/dev/null", PKGINFO, "nws");
        if (system(buf) == 0){
		XtVaSetValues(w_nds, XmNsensitive, (XtArgVal)True, NULL);
		XtVaSetValues(w_ndsLabel, XmNsensitive, (XtArgVal)True, NULL);
        }
        else{
		XtVaSetValues(w_nds, XmNsensitive, (XtArgVal)False, NULL);
		XtVaSetValues(w_ndsLabel, XmNsensitive, (XtArgVal)False, NULL);
        }
            
}

/* This routine sets the sensitivity 
 * of certain property widgets depending
 * on if group is a NIS group or not.
 */
static  void
SetNisGroupValues()
{
 Widget	nis_item_0;
 Widget	group_item_0;
 
 nis_item_0 = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "nis_menu:0");
 if ( nis_group == True){
     XtVaSetValues(nis_item_0, XmNset, (XtArgVal)nis_group, NULL);
     XtVaSetValues(w_gid, XmNsensitive, (XtArgVal)False, NULL);
     group_item_0 = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "group_menu:0");
     if (ypbind_on == True)
          XtVaSetValues(nis_item_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
     else
          XtVaSetValues(nis_item_0, XmNsensitive, (XtArgVal)False, NULL);

 }
 else{
     XtVaSetValues(w_gnis, XmNset, (XtArgVal)nis_group, NULL);
     XtVaSetValues(w_gid, XmNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(nis_item_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
 }

}

static  void
SetMenuSensTrue()
{

    Widget	b0;
    Widget	b1;
    Widget	b3;
/* Set sensitivity to True when object is selected */

    b0 = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:2");
    XtVaSetValues(b0, XmNsensitive, (XtArgVal)True, NULL);

    b0 = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:0");
    b1 = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:1");
    b3 = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:3");

    if (view_type == GROUPS)
    {
        XtVaSetValues(b0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
        XtVaSetValues(b1, XmNsensitive, (XtArgVal)I_am_owner, NULL);
        XtVaSetValues(b3, XmNsensitive, (XtArgVal)False, NULL);
    }
    else
    {
        if ( view_type == USERS){
            XtVaSetValues(b0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
            XtVaSetValues(b1, XmNsensitive, (XtArgVal)I_am_owner, NULL);
        }
        else {     /* RESERVED */
            XtVaSetValues(b0, XmNsensitive, (XtArgVal)False, NULL);
            XtVaSetValues(b1, XmNsensitive, (XtArgVal)False, NULL);
        }
        XtVaSetValues(b3, XmNsensitive, (XtArgVal)True, NULL);
    }
}

static  void
SetMenuSensFalse()
{
    Widget	w;

/* Set sensitivity to False when no object is selected */
    w = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:1");
    XtVaSetValues(w, XmNsensitive, (XtArgVal)False, NULL);
    w = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:2");
    XtVaSetValues(w, XmNsensitive, (XtArgVal)False, NULL);
    w = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:3");
    XtVaSetValues(w, XmNsensitive, (XtArgVal)False, NULL);

    w = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "edit_menu:0");
    if (view_type == RESERVED)
    {
        XtVaSetValues(w, XmNsensitive, (XtArgVal)False, NULL);
    }
    else{
        XtVaSetValues(w, XmNsensitive, (XtArgVal)I_am_owner, NULL);
    }
}

static  void
CheckIfUidLoggedOn(int uidno, char *buf, char *name )
{
  char        uid_buf[BUFSIZ];

       /* Check if the uid is logged on */
                sprintf(uid_buf,"ps -fu%d |grep %s|grep -v grep \
                          >/dev/null 2>&1", uidno, name);
       /* Add the Uid warning message */
               if (system (uid_buf) == 0)
                      sprintf(buf+strlen(buf), GetGizmoText(format_uidBusyFmt),
                                      name);
               else
                      sprintf(buf+strlen(buf), GetGizmoText(format_chgUidFmt));

}

void
GetLocaleName(char * ulocale)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale, get locale label */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].user_data, ulocale) == 0)
     {
        userLocaleName = (char *)LocaleItems[i].label;
        userLocale = (char *)LocaleItems[i].user_data;
        found = TRUE;
        break;
     }

     i++;
   }
   if (found == 0)
    {
    userLocaleName = defaultLocaleName;
    userLocale = defaultLocale;
    }
}


void
GetLocale(char * locale_name)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale label, get locale */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].label, locale_name) == 0)
     {
        userLocaleName = (char *)LocaleItems[i].label;
        userLocale = (char *)LocaleItems[i].user_data;
        found = TRUE;
        break;
     }

     i++;
   }
   if (found == 0)
    {
    userLocaleName = defaultLocaleName;
    userLocale = defaultLocale;
    }

}

/* This routine creates a Day One file
 * for the user in $XWINHOME/desktop/LoginMgr/DayOne
 * directory. The file will contain the dayone locale.
 */
static  void
CreateDayOneFile()
{

             char       buf1[BUFSIZ];
             char       buf2[BUFSIZ];
             FILE       *fp;
             Boolean    Open_OK = FALSE;

         /* Open the user's DayOne file */
         sprintf(buf1, "%s/%s", GetXWINHome(DAYONE), login);
         if (fp = fopen(buf1,"w")) {
               sprintf(buf2, "%s\n",userLocale);
               fputs(buf2, fp);
               fclose(fp);
               Open_OK = TRUE;
         }

         if (Open_OK == TRUE ) {
               sprintf(buf2, "/usr/bin/chmod 444 %s >/dev/null 2>&1",
                                           buf1);
               system (buf2);
         }

}

/* The routine checks the /etc/default/locale
 * file to see if there a default locale.
 */
static  void
SetDefaultLocale()
{

        char       *def_lang = NULL;
        FILE       *locale_fp;

        if (((locale_fp = defopen("locale")) != NULL) &&
           ((def_lang = defread(locale_fp, "LANG")) != NULL)){
                defaultLocale = strdup(def_lang);
                (void)defclose(locale_fp);
        }
        else
                defaultLocale = "C";

        GetDefaultName(defaultLocale);
        userLocaleName = defaultLocaleName;
        userLocale = defaultLocale;
        curLocaleName = userLocaleName;
        curLocale = userLocale;

}


/* The routine tries to get the name of the
 * default locale from the locale list.
 */
static  void
GetDefaultName(char * ulocale)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale, get locale label */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].user_data, ulocale) == 0)
     {
        defaultLocaleName = (char *)LocaleItems[i].label;
        found = TRUE;
        break;
     }

     i++;
   }

   if (found == FALSE){
        defaultLocale = "C";
        defaultLocaleName = GetGizmoText(string_AmericanEng);
   }
}


static void
SetButtonLabel(Widget w, char *l, char *m)
{
    KeySym		ks;
    XmString		string;
    char *		label;
    unsigned char *	mnemonic;

    label = GGT(l);
    mnemonic = (unsigned char *)GGT(m);
    string = XmStringCreateLocalized(label);
    XtVaSetValues(w, XmNlabelString, string, NULL);
    XmStringFree(string);
    if (mnemonic != NULL &&
        (((ks = XStringToKeysym((char *)mnemonic)) != NoSymbol) ||
        (ks = (KeySym)(mnemonic[0]))) &&
        strchr(label, (char)(ks & 0xff)) != NULL) {
	    XtVaSetValues(w, XmNmnemonic, ks, NULL);
    }
}


static	void
SetGroupValues(GroupPtr gp)
{
    static	char	gid[8];
    char	*gname;
    Arg         arg[8];
    Widget	w;

    w = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "group_menu:0");
    if (gp == NULL){		/* adding a new group */
        if (nis_group){
            gname = g_reset->g_name;
	    sprintf(gid, "%s", "");
        }
        else{
	    gname = "";
	    sprintf(gid, "%d", max_gid+1);
        }
	XtSetArg(arg[0], XmNtitle, (XtArgVal)GGT(string_addGroupLine));
	XtSetValues(w_gpopup, arg, 1);
        SetButtonLabel(w, label_add, mnemonic_add);
    }
    else {			/* displaying properties of existing group */
        CheckIfNisGroup(gp->g_name);
        if (strncmp(gp->g_name,"+",1) == 0)
            group = gname = gp->g_name + 1;
        else
	    group = gname = gp->g_name;
	sprintf(gid, "%d", gp->g_gid);
	XtSetArg(arg[0], XmNtitle, (XtArgVal)GetGizmoText(string_groupLine));
	XtSetValues(w_gpopup, arg, 1);
        SetButtonLabel(w, label_ok, mnemonic_ok);
    }

    XmTextFieldSetString(w_gname, gname);
    XmTextFieldSetString(w_gid, gid);
}

static	void
SetPopupValues(UserPtr	up)
{
    char	uidbuf[8];
    struct	group	*gp;
    int		i, n;
    XmString	string;
    Widget	w_dtm0;
    Widget	w_dtm1;
    char	buf[256];
    struct	hybrid  *hp;

    if (remote) {
	free(remote);
	remote = NULL;
    }
    sethome = 1;
    if (up) {
	CheckIfDtm(up->pw_uid, up->pw_gid, up->pw_dir);
        CheckIfNisUser(up->pw_name);
        if (strncmp(up->pw_name,"+",1) == 0)
	    login = up->pw_name + 1;
        else
	    login = up->pw_name;
	desc  = up->pw_comment;
	home  = up->pw_dir;
	shell = up->pw_shell;
	sprintf(uidbuf, "%d", up->pw_uid);
	uid = strdup(uidbuf);
	gp = getgrgid(up->pw_gid);
	group = strdup(gp ? gp->gr_name : "");
	hp = getndsnam(login);
        if (ndsname) FREE(ndsname);
        if (cur_ndsname) FREE(cur_ndsname);
        ndsname = cur_ndsname = strdup(hp ? hp->nds_namp : "");
    }
    else {
	dtm_account = True;
	dtm_style   = MOTIF_DTM;

        if (nis_user){
	    login = XmTextFieldGetString(w_login);
	    ndsname = XmTextFieldGetString(w_nds);
	    home        = "";
	    desc        = ""; 
	    uid = strdup("");
	    group       = "other";
	    shell       = "";
        }
        else{
            login       = "";
            ndsname     = "";
            home        = HOME_DIR;
            desc        = "";
            sprintf(uidbuf,  "%d", NextUid());
            uid = strdup(uidbuf);
            group       = "other";
            shell       = KSH;
            if (!FileCheck("", shell, X_OK, atoi(uid), 1))
            shell = BSH;
        }
    }
    w_dtm0 = QueryGizmo(PopupGizmoClass, g_popup, GGW, "dtmMenu:0");
    w_dtm1 = QueryGizmo(PopupGizmoClass, g_popup, GGW, "dtmMenu:1");
    if (dtm_account) {
	XmToggleButtonGadgetSetState(w_dtm0, True);
	XmToggleButtonGadgetSetState(w_dtm1, False);
    }
    else {
	XmToggleButtonGadgetSetState(w_dtm1, True);
	XmToggleButtonGadgetSetState(w_dtm0, False);
    }
    XtVaSetValues(w_remote, XmNsensitive, dtm_account, NULL);
    if (login)	XmTextFieldSetString(w_login, login);
    if (ndsname)	XmTextFieldSetString(w_nds, ndsname);
    if (desc)	XmTextFieldSetString(w_desc, desc);
    if (home)	XmTextFieldSetString(w_home, home);
    if (shell)	XmTextFieldSetString(w_shell, shell);
    XmTextFieldSetString(w_remote, remote ? remote : "");
    XmTextFieldSetString(w_uid, uid);
    strcpy(buf, GGT(label_selection));
    strcat(buf, group);
    string = XmStringCreateLocalized(buf);
    XtVaSetValues(w_group, XmNlabelString, string, 0);
    XmStringFree(string);
    if (u_pending == P_ADD)
      {
       userLocaleName = defaultLocaleName;
       userLocale = defaultLocale;
      }
    strcpy(buf, GGT(label_selection));
    strcat(buf, userLocaleName);
    string = XmStringCreateLocalized(buf);
    XtVaSetValues(w_localetxt, XmNlabelString, string, 0);
    XmStringFree(string);

    for (n = 0; n < g_cnt; n++)
	if (strcmp(group,GroupItems[n]) == 0){
	    XmListSelectPos(w_glist, n+1, False);
            XmListSetPos(w_glist, n+1);
        }

    for (n = 0; n < locale_cnt; n++)
	if (strcmp(userLocaleName, (char *)LocaleItems[n].label) == 0){
	    XmListSelectPos(w_locale, n+1, False);
            XmListSetPos(w_locale, n+1);
        }
}

void
resetCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = DtGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    FooterMsg("");
    if (shell_wid == w_gpopup)
    {
	MoveFocus(w_gname);
	SetPopupWindowLeftMsg(g_gpopup, "");
	SetGroupValues(g_reset);
    }
    else
    {
	MoveFocus(w_login);
	SetPopupValues(u_reset);
	SetPopupWindowLeftMsg(g_popup, "");
    }
    return;
}

void
reinitPopup(int popup_type)
{
    FooterMsg("");
    if (popup_type == GROUPS)
    {
	SetPopupWindowLeftMsg(g_gpopup, "");
	SetGroupValues(NULL);
	MoveFocus(w_gname);
    }
    else
    {
	SetPopupValues(NULL);
	SetPopupWindowLeftMsg(g_popup, "");
	MoveFocus(w_login);
    }
    return;
}

void
cancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = DtGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    if (shell_wid == w_gpopup)
    {
	SetPopupWindowLeftMsg(g_gpopup, "");
        g_pending = 0;
    }
    else
    {
	SetPopupWindowLeftMsg(g_popup, "");
        u_pending = 0;
    }

    FooterMsg("");
    XtPopdown(shell_wid);
}

static	void
CreateGroupProp(void)
{
	char	gid_max[40];
        size_t	gid_len;

	g_gpopup = CreateGizmo(
		w_baseshell, PopupGizmoClass, &group_popup, NULL, 0
	);
	w_gpopup = GetPopupGizmoShell(g_gpopup);
	w_gname = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "group_name");
	w_gnis = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "nis_menu:0");
	w_gid = QueryGizmo(PopupGizmoClass, g_gpopup, GGW, "group_id");

	/* Set max group name length */
        XtVaSetValues(w_gname, XmNmaxLength, (XtArgVal)8, NULL);

	/* Set max length for gid */
        sprintf(gid_max, "%ld", (long)UID_MAX);
        gid_len = strlen(gid_max);
        XtVaSetValues(w_gid, XmNmaxLength, (XtArgVal)gid_len, NULL);
}

static	void
MakePermList(void)
{
	FILE	*pfile;
	char	buf[BUFSIZ];
	char    *tmp;

	if (pfile = fopen(GetXWINHome(PRIV_TABLE),"r")) {
		while (fgets(buf, BUFSIZ, pfile)) {
			p_list = p_cnt==0 ?
				(PermPtr)malloc(sizeof(PermRec)) :
				(PermPtr)realloc(p_list,(p_cnt+1)*sizeof(PermRec));
			if (p_list == NULL) {
                            ErrorNotice(GetGizmoText(string_malloc), view_type);
				break;
			}
			if ((tmp = strtok(buf, "\t\n")) == NULL)
			    continue;
			p_list[p_cnt].label = strdup(tmp);
			if ((tmp = strtok(NULL,"\t\n")) == NULL)
			    continue;
			p_list[p_cnt].cmds  = strdup(tmp);
			if ((tmp = strtok(NULL,"\t\n")) == NULL)
			    p_list[p_cnt].help  = NULL;
			else
			    p_list[p_cnt].help  = strdup(tmp);
			p_cnt++;
		}
		pclose(pfile);
	}
	else
                ErrorNotice(GetGizmoText(string_permfile), view_type);
}

static	void
CheckPermList(UserPtr u_select)
{
	XmString	string;
	FILE	*fperm;
	char	buf[BUFSIZ];
	int	n;
	static char mybuf[BUFSIZ], strbuf[256];

	this_is_owner = FALSE;
	mybuf[0] = '\0';
	strbuf[0] = '\0';
	for (n = 0; n < p_cnt; n++)
		p_list[n].granted = FALSE;
        if (strncmp(u_select-> pw_name,"+",1) == 0)
	    sprintf(buf, "%s/%s", PERM_FILE, u_select->pw_name + 1);
        else
	    sprintf(buf, "%s/%s", PERM_FILE, u_select->pw_name);
	if (fperm = fopen(GetXWINHome(buf),"r")) {
		while (fgets(buf, BUFSIZ, fperm)) {
			buf[strlen(buf)-1] = '\0';
			if (strcmp(buf, OWNER) == 0)
				this_is_owner = TRUE;
			else if (*buf == '#' || strncmp(buf,"ICON=",5)==0)
				;
			else for (n = 0; n < p_cnt; n++) {
				if (strcmp(buf, p_list[n].label) == 0) {
					p_list[n].granted = TRUE;
					break;
				}
			}
		}
		fclose (fperm);
	}
        if (strncmp(u_select-> pw_name,"+",1) == 0)
	    sprintf(mybuf, GetGizmoText(label_owner_id), u_select->pw_name + 1);
        else
	    sprintf(mybuf, GetGizmoText(label_owner_id), u_select->pw_name);
        string = XmStringCreateLocalized(mybuf);
	XtVaSetValues(w_own, XmNlabelString, string, NULL);
	XmStringFree(string);
        if (strncmp(u_select-> pw_name,"+",1) == 0)
          sprintf(strbuf, GetGizmoText(string_user_may), u_select->pw_name + 1);
        else
	  sprintf(strbuf, GetGizmoText(string_user_may), u_select->pw_name);
        string = XmStringCreateLocalized(strbuf);
	XtVaSetValues(
		QueryGizmo(PopupGizmoClass, g_perm, GGW, "pcapLabel"),
		XmNlabelString, string, NULL
	);
	XmStringFree(string);
}

static	void
resetPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;
	Boolean	ok;
	Widget	w;
	char	buf[10];

        SetPopupWindowLeftMsg(g_perm, "");
	XtVaSetValues(w_own, XmNset, (XtArgVal)this_is_owner, NULL);
	for (n = 0; n < p_cnt; n++) {
		ok = (/*this_is_owner ||*/ p_list[n].granted);
		sprintf(buf, "checks:%d", n);
		w = QueryGizmo(PopupGizmoClass, g_perm, GGW, buf);
		XtVaSetValues(w, XmNset, (XtArgVal)ok, NULL);
	}
}

static	void
ChangeOwner(char * new_owner)
{
	char	buf[BUFSIZ];

	u_pending = P_OWN;
	login = new_owner;
	operation = GetGizmoText(owner_set? tag_addOwner : tag_delOwner);
	operation_num = owner_set? 4 : 3;
        if (owner_set)
	     sprintf(buf,GetGizmoText(format_addOwner),login); 
        else
	     sprintf(buf,GetGizmoText(format_delOwner),login); 
	DisplayPrompt(buf, w_perm);
}

static	void
applyPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	char	checks[10];
	Widget	w;
	Boolean	set;
	FILE	*fp;
	char	*usr, *ptr, buf[BUFSIZ];
	int	n, change, exit_code;
	UserPtr selected;

        SetPopupWindowLeftMsg(g_perm, "");
	if ((selected = SelectedUser()) != NULL){
            if (strncmp(selected-> pw_name,"+",1) == 0)
                usr = selected->pw_name + 1;
            else
                usr = selected->pw_name;
        }
	else
	{
	    SetPopupWindowLeftMsg(g_perm, "");
	    sprintf(buf, GetGizmoText(format_noSelectFmt),
		    GetGizmoText((view_type == USERS) ? tag_login : tag_sys));
	    ErrorNotice(buf, view_type);
	    return;
	}
	XtVaGetValues(w_own, XmNset, &set, NULL);
	owner_set=set;
	if (!owner_set) {
		if (isLastOwnerAcc(usr)) {
			sprintf(buf, GetGizmoText(format_rmLastOwnerFmt), usr);
			ErrorNotice(buf, view_type);
			return;
		}
	}
	if (this_is_owner != owner_set)
		ChangeOwner(usr);
	strcpy(buf,ADMINUSER);
	ptr = buf+strlen(buf);
	for (change = n = 0; n < p_cnt; n++) {
		sprintf(checks, "checks:%d", n);
		w = QueryGizmo(PopupGizmoClass, g_perm, GGW, checks);
		XtVaGetValues(w, XmNset, &set, NULL);
		if (set == p_list[n].granted)
			continue;
		else if (change == 0) {	/* confirm user in database */
			sprintf(ptr, "%s >/dev/null 2>&1", usr);
			if ((exit_code = system(buf)) != 0) {
				sprintf(ptr, "-n %s", usr);
				if ((exit_code = system(buf)) != 0) {
					ptr = GetGizmoText(string_adm);
                                        ErrorNotice(ptr, view_type);
					return;
				}
			}
		}
		/*
		 *	update the changed permission value
		 */
		if (set)
			sprintf(ptr, "-a %s ",p_list[n].cmds);
		else {
			char	*src, *dst;
 			/*
			 * 	removal just uses command *names*
			 */
			strcpy(ptr,"-r ");
			for (src=p_list[n].cmds,dst=ptr+3;*src;++src){
				if (*src != ':')
					*dst++ = *src;
				else {
					do src++;
					while (*src && *src !=  ',');
					--src;
				}
			}
			*dst++ = ' ';
			*dst = '\0';
		}
		strcat(buf, usr);
		if (system(buf) == 0) {
			change = 1;
			p_list[n].granted = set;
		}
		else {
			change = -1;
			break;
		}
	}
	switch (change) {
		case -1:	ptr = GetGizmoText(tag_bad);  break;
		case  0:	ptr = GetGizmoText(tag_null); break;
		case  1:	ptr = GetGizmoText(tag_good); break;
                default:        break;
	}
	if (change)
	{
	    sprintf(buf, GetGizmoText(format_permFmt), usr, ptr);
	    FooterMsg(buf);
	}
	/*
	 *	update the record in PERM_FILE
	 */
	if (change) {
		sprintf(buf,"%s/%s",PERM_FILE,usr);
		if (fp=fopen(GetXWINHome(buf),"w")) {
		    if (this_is_owner == TRUE)
			fprintf(fp, "%s\n", OWNER);
		    for (n = 0; n < p_cnt; n++)
			if (p_list[n].granted)
			    fprintf(fp,"%s\n",p_list[n].label);
		    fclose(fp);
		}
		/* if adding/removing ownership, leave popup up until */
		/* user confirms change */
		if (this_is_owner == owner_set){
                    MoveFocus(w_own);
    		    BringDownPopup(GetPopupGizmoShell(g_perm));
                }
        }
        else{
                MoveFocus(w_own);
    		BringDownPopup(GetPopupGizmoShell(g_perm));
	}
}

static	void
cancelPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupWindowLeftMsg(g_perm, "");
    MoveFocus(w_own);
    BringDownPopup(GetPopupGizmoShell(g_perm));
}

static	void
SetOwnerCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;
	char	buf[10];
	Boolean	set;
	Widget	w;

	XtVaGetValues(w_own, XmNset, &set, NULL);
        SetPopupWindowLeftMsg(g_perm, "");
	for (n = 0; n < p_cnt; n++) {
		sprintf(buf, "checks:%d", n);
		w = QueryGizmo(PopupGizmoClass, g_perm, GGW, buf);
		if (set) {
			XtVaSetValues(w, XmNset, True, NULL);
		}
		else {
			XtVaSetValues(w, XmNset, p_list[n].granted, NULL);
		}
	}
}

static	void
CreatePermSheet(void)
{
static	MenuItems	*PermItems;	/* actually, nonexclusive checkboxes */
	int		n;

	MakePermList();
	/* Last item in list needs to be NULL, hence +1 */
	PermItems = (MenuItems *)calloc(p_cnt+1, sizeof(MenuItems));
	for (n = 0; n < p_cnt; n++) {
		char	*ptr 		= strdup(p_list[n].label);
		PermItems[n].sensitive	= True;
		PermItems[n].label	= DtamGetTxt(ptr);
		PermItems[n].type	= I_TOGGLE_BUTTON;
		PermItems[n].set	= (XtArgVal)FALSE;
	}
	permMenu.items = PermItems;
	g_perm = CreateGizmo(
		w_baseshell, PopupGizmoClass, &perm_popup, NULL, 0
	);
	w_perm = GetPopupGizmoShell(g_perm);
	w_own = QueryGizmo(PopupGizmoClass, g_perm, GGW, "ownerMenu:0");
	g_checks = QueryGizmo(PopupGizmoClass, g_perm, GGG, "checks");
}

static	void
DoPopup(UserPtr u_select)
{
	resetFocus(view_type);
	if (view_type == GROUPS) {
		g_reset = (GroupPtr)u_select;
		SetGroupValues(g_reset);
		MapGizmo(PopupGizmoClass, g_gpopup);
	}
	else {
		u_reset = u_select;
		SetPopupValues(u_select);
		XtVaSetValues(w_extra, XmNset, (XtArgVal)FALSE, NULL);
		XtUnmanageChild(
			QueryGizmo(PopupGizmoClass, g_popup, GGW, "rc8")
		);
		MapGizmo(PopupGizmoClass, g_popup);
	}
}

static	void
addCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg		arg[4];

    FooterMsg("");

   /* Initialize NIS settings for the ADD */
    nis_user = False;
    nis_group = False;
    if ( view_type == USERS || view_type == RESERVED)
        SetNisUserValues();
    else if (view_type == GROUPS)
        SetNisGroupValues();
    CheckIfNisOn();
    CheckIfNWS();

    switch(view_type)
    {
    case GROUPS:
	SetPopupWindowLeftMsg(g_gpopup, "");
	g_pending = P_ADD;
	break;
    case USERS:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addUserLine));
	XtSetValues(w_popup, arg, 1);
        SetButtonLabel(w_prop_0, label_add, mnemonic_add);
        XtVaSetValues(w_localetxt, XmNsensitive, TRUE, NULL);
        XtVaSetValues(w_locale, XmNsensitive, TRUE, NULL);
	SetPopupWindowLeftMsg(g_popup, "");
	u_pending = P_ADD;
	break;
    case RESERVED:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addSysLine));
	XtSetValues(w_popup, arg, 1);
        SetButtonLabel(w_prop_0, label_add, mnemonic_add);
        XtVaSetValues(w_localetxt, XmNsensitive, TRUE, NULL);
        XtVaSetValues(w_locale, XmNsensitive, TRUE, NULL);
	SetPopupWindowLeftMsg(g_popup, "");
	u_pending = P_ADD;
	break;
    default:
	break;
    }
    DoPopup((UserPtr)NULL);
}

isLastOwnerAcc(char * login)
{
	char	buf[BUFSIZ+PATH_MAX];
	char	user_path[PATH_MAX];
	FILE	*fp;
	int	i;

	sprintf(user_path, "%s/%s", GetXWINHome(PERM_FILE), login);
        sprintf(buf, "grep \"^owner$\" %s >/dev/null 2>&1", user_path);
	if (system(buf) != 0)
		return(FALSE);
	else {
		sprintf(user_path, "%s/*", GetXWINHome(PERM_FILE));
		sprintf(buf, "grep \"^owner$\" %s | wc -l", user_path);
		if (fp=popen(buf, "r")) {
			while(fgets(buf, BUFSIZ, fp))
				;
			if (pclose(fp) == 0) {
				i = atoi(buf);
				if (i <= 1)
					return(TRUE);
				else
					return(FALSE);
			}
		}
	}
}

static	void
deleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char		*name, buf[BUFSIZ];
	UserPtr		u_sel = SelectedUser();

	FooterMsg("");
	operation = GetGizmoText(tag_delOp);
	operation_num = 2;
	setContext(view_type);
	if (!u_sel) {
		sprintf(buf, GetGizmoText(format_noSelectFmt), context);
		ErrorNotice(buf, view_type);
	}
	else {
		if (view_type == GROUPS) {
			GroupPtr	g_sel = (GroupPtr)u_sel;

		        g_pending = P_DEL;
                        if (strncmp(g_sel->g_name,"+",1) == 0)
			    name = gname = strdup(g_sel->g_name + 1);
                        else
			    name = gname = strdup(g_sel->g_name);
		}
		else {
		        u_pending = P_DEL;
                        if (strncmp(u_sel->pw_name,"+",1) == 0)
			    name = login = strdup(u_sel->pw_name + 1);
                        else
			    name = login = strdup(u_sel->pw_name);
			home = u_sel->pw_dir;
			home_flag = DEL_HOME;
			uid = XmTextFieldGetString(w_uid);
		}
		if (isLastOwnerAcc(login)) {
			sprintf(buf, GetGizmoText(format_lastOwnerAccFmt), login);
			ErrorNotice(buf, view_type);
		}
		else {
                        retrieved_Fmt = 
                                 GetMessage_Fmt(operation_num, context_num, 1);
			sprintf(buf, GetGizmoText(retrieved_Fmt), name);
			ConfirmDelete(buf);
		}
	}
}
    
static	void
propCB(Widget w, XtPointer client_data, XtPointer call_data)
{

    UserPtr	 u_select = SelectedUser();
    PopupGizmo	*pop;
    char 	 buf[BUFSIZ], uid_buf[BUFSIZ];
    Arg		 arg[4];
    Boolean      uid_flag = TRUE;

    setContext(view_type);
    if (!u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt), context);
	ErrorNotice(buf, view_type);
	return;
    }
    FooterMsg("");
       CheckIfNisOn();
       CheckIfNWS();

    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	g_reset = (GroupPtr)u_select;
	SetGroupValues(g_reset);
        XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
    }
    else
    {
	u_pending = P_CHG;
	u_reset = u_select;
	XtSetArg(arg[0], XmNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	SetButtonLabel(w_prop_0, label_ok, mnemonic_ok);
	SetPopupValues(u_reset);

        if ( view_type == USERS )
            XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)I_am_owner, NULL);
        else      /* RESERVED */
            XtVaSetValues(w_prop_0, XmNsensitive, (XtArgVal)False, NULL);

        /* Check if the uid is running dtm */
        sprintf(uid_buf, "ps -fu%d |grep dtm|grep -v grep >/dev/null 2>&1",
                                     atoi(uid));
        if (system (uid_buf) == 0)
                uid_flag = FALSE;
        XtVaSetValues(w_localetxt, XmNsensitive, uid_flag, NULL);
        XtVaSetValues(w_locale, XmNsensitive, uid_flag, NULL);

    }
    pop = view_type==GROUPS? g_gpopup: g_popup; 
    SetPopupWindowLeftMsg(pop, "");
    resetFocus(view_type);
    MapGizmo(PopupGizmoClass, pop);
}

static	void
permCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    UserPtr	u_select = SelectedUser();
    char	buf[BUFSIZ];
    char       *errContext;
    

    errContext = (view_type == GROUPS) ? tag_account :
	((view_type == USERS) ? tag_login : tag_sys);
    SetPopupWindowLeftMsg(g_perm, "");
    if (view_type == GROUPS || !u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt),
		GetGizmoText(errContext));
	ErrorNotice(buf, view_type);
    }
    else {
	FooterMsg("");
	CheckPermList(u_select);
	resetPermCB(w, NULL, NULL);
	MapGizmo(PopupGizmoClass, g_perm);
    }
}

static	void
SetViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *	cbs = (MenuGizmoCallbackStruct *)client_data;
    Arg  arg[4];
    Widget	menu;

    menu = (Widget)QueryGizmo(BaseWindowGizmoClass, g_base, GGW, "menu_bar:1");
    FooterMsg("");
    view_type = cbs->index;
    SetMenuSensFalse();

    switch (view_type)
    {
    case USERS:
	XtSetArg(arg[0], XmNtitle, GetGizmoText(string_userBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	SetButtonLabel(menu, label_edit, mnemonic_edit);
	break;
    case GROUPS:
	XtSetArg(arg[0], XmNtitle, GetGizmoText(string_groupBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	SetButtonLabel(menu, label_groupBtn, mnemonic_groupBtn);
	break;
    case RESERVED:
	XtSetArg(arg[0], XmNtitle, GetGizmoText(string_sysBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	SetButtonLabel(menu, label_edit, mnemonic_edit);
	break;
    default:
	break;
    }
    ResetIconBox();
}

static	void
SingleClick(Widget w, XtPointer client_data, XtPointer call_data)
{
    ExmFlatCallData	*d = (ExmFlatCallData *)call_data;
    Arg              arg[4];
    UserPtr		u;

    FooterMsg("");

    ExmVaFlatGetValues(w, d->item_index, XmNuserData, &u, NULL);
    SetMenuSensTrue();

    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(string_groupLine)); 
	XtSetValues(w_gpopup, arg, 1);
	g_reset = (GroupPtr)u;
    }
    else
    {
	u_pending = P_CHG;
	u_reset = u;
	XtVaSetValues(w_glist, XmNvisibleItemCount, GROUPHEIGHT, NULL);
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	CheckPermList(u_reset);
	resetPermCB(w, NULL, NULL);
    }
}

static void
AddUserItem(UserPtr p, int i)
{
    FILE       *fperm;
    char	buf[BUFSIZ+PATH_MAX];
    char       *pw_name;

    if (strncmp(p->pw_name,"+",1) == 0){
        ICON_LABEL(iconBox.items[i].label, p->pw_name + 1)
	pw_name = p->pw_name + 1;
    }
    else{
        ICON_LABEL(iconBox.items[i].label, p->pw_name)
	pw_name = p->pw_name;
    }
    iconBox.items[i].object_ptr = (XtArgVal)DmGetPixmap(
	theScreen, "login.glyph"
    );
    iconBox.items[i].client_data = (XtArgVal)p;
    iconBox.items[i].sensitive = (XtArgVal)True;
    iconBox.items[i].select = (XtArgVal)False;
    iconBox.items[i].managed = (XtArgVal)True;
    sprintf(buf, "%s/%s", PERM_FILE, pw_name);
    if (fperm = fopen(GetXWINHome(buf),"r")) {
	while (fgets(buf, BUFSIZ, fperm)) {
	    if (strncmp(buf,"ICON=",5)==0) { /* note: no whitespace allowed */
		buf[strlen(buf)-1] = '\0';
		if (buf[5] == '\0') {
		    continue;	/* path is missing */
		}
		if ((iconBox.items[i].object_ptr = (XtArgVal)DmGetPixmap(
			theScreen,buf+5
		    )) == NULL) {
		    continue;
		}
		break;
	    }
	}
	fclose(fperm);
    }
}

static void
AddGroupItem(GroupPtr p, int i)
{
    iconBox.items[i].object_ptr = (XtArgVal)DmGetPixmap(
	theScreen, "group.glyph"
    );
    iconBox.items[i].client_data = (XtArgVal)p;
    iconBox.items[i].sensitive = (XtArgVal)True;
    iconBox.items[i].select = (XtArgVal)False;
    iconBox.items[i].managed = (XtArgVal)True;
    if (strncmp(p->g_name,"+",1) == 0) {
	ICON_LABEL(iconBox.items[i].label, p->g_name + 1);
    }
    else  {
	ICON_LABEL(iconBox.items[i].label, p->g_name);
    }
}

static void
GetGroupItems()
{
    int		n;
    Gizmo	g;

    iconBox.items = (DmItemPtr)realloc(
	iconBox.items, g_cnt*sizeof(DmItemRec)
    );
    iconBox.numItems = g_cnt;
    for (n=0; n<g_cnt; n++) {
	AddGroupItem(&g_list[n], n);
    }
}

static void
GetUserItems()
{
    int		n;
    int		cnt = 0;
    int		i;
    Gizmo	g;

    /* First count the actual items to go into the list */
    for (n=0; n<u_cnt; n++) {
	if (view_type == USERS && u_list[n].pw_uid >= LOWEST_USER_UID
	    && u_list[n].pw_uid < UID_MAX-2
	    ||  view_type == RESERVED && u_list[n].pw_uid < LOWEST_USER_UID
	    ||  view_type == RESERVED && u_list[n].pw_uid > UID_MAX-3) {
	    cnt += 1;
	}
    }
    iconBox.items = (DmItemPtr)realloc(
	iconBox.items, cnt*sizeof(DmItemRec)
    );
    iconBox.numItems = cnt;
    for (i=0, n=0; n<u_cnt; n++) {
	if (view_type == USERS && u_list[n].pw_uid >= LOWEST_USER_UID
	    && u_list[n].pw_uid < UID_MAX-2
	    ||  view_type == RESERVED && u_list[n].pw_uid < LOWEST_USER_UID
	    ||  view_type == RESERVED && u_list[n].pw_uid > UID_MAX-3) {
	    AddUserItem(&u_list[n], i);
	    i += 1;
	}
    }
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

	help->appTitle = help_app_title;
	help->title = string_appName;
	help->section = GetGizmoText(help->section);
	PostGizmoHelp(w_baseshell, help);
}

main(int argc, char *argv[])
{   
    char           atom[SYS_NMLN+30]= ApplicationClass;
    struct utsname name;
    Window         another_window;
    int            i;

    I_am_owner = _DtamIsOwner(OWN_LOGIN);

    SetPropMenuItem(I_am_owner);
    edit_menu_item[0].sensitive  = (XtArgVal)I_am_owner;
    perm_menu_item[0].sensitive  = (XtArgVal)I_am_owner;
    group_menu_item[0].sensitive = (XtArgVal)I_am_owner;

    /*
     * scan the arg list for "-display" or "-d" .
     */
    for (i = 1; i < argc; i++) {
	if (strncmp (argv[i], "-d", 2) == 0){
	    if (++i < argc )
	    	displayName = argv[i];
	}
    }

    XtSetLanguageProc(NULL, NULL, NULL);
    w_toplevel = XtInitialize("userad", ApplicationClass, NULL, 0, &argc, argv);
    DtiInitialize(w_toplevel);
    InitializeGizmos(ProgramName, ProgramName);
    xinch = XmConvertUnits(
	w_toplevel, XmHORIZONTAL, Xm100TH_POINTS, 7200, XmPIXELS
    );
    yinch = XmConvertUnits(
	w_toplevel, XmVERTICAL, Xm100TH_POINTS, 7200, XmPIXELS
    );

    base.title = GetGizmoText(base.title);
    base.iconName = GetGizmoText(base.iconName);

    MakeUserList();
    MakeGroupList();
    MakeLocaleList();

    theScreen  = XtScreen(w_toplevel);
    theDisplay = XtDisplay(w_toplevel);
    if (view_type == GROUPS) {
	GetGroupItems();
    }
    else {
	GetUserItems();
    }

    /*
     *	create base window icon box with logins
     */
    g_base =
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, &base, NULL, 0); 
	
    w_baseshell = GetBaseWindowShell(g_base);
#ifdef DEBUG
	XtAddEventHandler(
		w_baseshell, (EventMask) 0, True,
		_XEditResCheckMessages, NULL
	);
#endif /* DEBUG */
    XmAddWMProtocolCallback(
	w_baseshell, XA_WM_DELETE_WINDOW(XtDisplay(w_baseshell)), exitCB, NULL
    );

    /* if another copy of this program running on this system is */
    /* running on this display then pop it to the top and exit  */

    MapGizmo(BaseWindowGizmoClass, g_base);

    if (uname(&name) >0)
    {
	strcat(atom, ":");
	strcat(atom, name.nodename);
    }
    another_window = DtSetAppId(theDisplay, XtWindow(w_baseshell), atom);
    if (another_window != None)
    {    
	XMapWindow(theDisplay, another_window);
	XRaiseWindow(theDisplay, another_window);
	XFlush(theDisplay);
	exit(0);
    }

    CreatePropSheet();
    CreatePermSheet();
    CreateGroupProp();
    w_iconbox = (Widget)QueryGizmo(
	BaseWindowGizmoClass, g_base, GGW, "iconbox"
    );
    w_swin = GetIconBoxSW(
	(Gizmo)QueryGizmo(BaseWindowGizmoClass, g_base, GGG, "iconbox")
    );

    MapGizmo(BaseWindowGizmoClass, g_base);
    XtMainLoop();
}

/* remove leading and trailing whitespace without moving the pointer */
/* so that the pointer may still be free'd later.                    */
/* returns True if the string was modified; False otherwise          */

Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
	return False;

    while (isspace(*ptr))
    {
	ptr++;
	changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
	*string = EOS;
	return changed;
    }

    if (changed)
	(void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
							   /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
	ptr--;
	changed = True;
    }
    *(++ptr) = EOS;
    
    return changed;
}

static void 
newCursor(int popup_type, Cursor newCursor)
{
    XDefineCursor(theDisplay, XtWindow(w_baseshell),  newCursor);
    switch (popup_type)	
    {
    case GROUPS:
	if (!XtIsRealized(w_gpopup))
	    return;
        XDefineCursor(theDisplay, XtWindow(w_gpopup), newCursor);
	break;
    case USERS:
	/* FALL THRU */
    case RESERVED:
	if (!XtIsRealized(w_popup))
	    return;
	XDefineCursor(theDisplay, XtWindow(w_popup),  newCursor);
	break;
    }
}

static void
standardCursor(int popup_type)
{
    newCursor(popup_type, DtGetStandardCursor(w_baseshell));
}

static void
busyCursor(int popup_type)
{
    newCursor(popup_type, DtGetBusyCursor(w_baseshell));
}

int	cmpuid(int *m, int *n)
{
	return *m - *n;
}

int	cmplogin(UserPtr x, UserPtr y)
{
	return strcoll(x->pw_name, y->pw_name);
}

int	cmpgroup(GroupPtr x, GroupPtr y)
{
	return strcoll(x->g_name, y->g_name);
}

int	cmplocale(FListItem2 *x, FListItem2 *y)
{
        return strcoll((char *)x->label, (char*)y->label);
}
