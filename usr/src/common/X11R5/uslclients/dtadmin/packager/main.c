#ifndef NOIDENT
#pragma ident	"@(#)main.c	15.1"
#endif

#include <locale.h>
#include <sys/utsname.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <pwd.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Vendor.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/BaseWindow.h>

#include "packager.h"
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/STextGizmo.h>
#include "SWGizmo.h"
#include "RTileGizmo.h"
#include <libDtI/DtI.h>

#define PROGRAM_NAME	"PackageMgr"
#define PING		"/usr/sbin/ping"

#define DESC                    "desc"
#define CTAPE1                  "ctape1"
#define CTAPE2                  "ctape2"
#define DISKETTE                "diskette"
#define CDROM                   "cdrom"

extern char *		optarg;
extern int		s_cnt;
extern void		MakeServerList(void);

Atom		class_atom, help_atom;

Widget          w_toplevel;
Screen *	theScreen;
Display *	theDisplay;
XFontStruct *	def_font;	/* Default font returned by _OlGetDefaultFont() */
int		SPOOL_VIEW;	/* # of devices in the uninstalled menu */
int		NETWORK_VIEW;	/* index to network install 		*/

typedef struct {
        char * list;
	char * value;
} devices;

devices alias[MAX_DEVS];

PackageRecord	Pr;		/* Main package record */
PackageRecord *	pr = &Pr;

char		CUSTOM[]  = "/sbin/custom";

Dimension	x3mm;
Dimension	y3mm;

DmFclassRec	unpkg_fcrec;
DmFclassRec	unset_fcrec;
DmFclassRec	pkg_fcrec;
DmFclassRec	set_fcrec;
DmFclassRec	exec_fcrec;

char *	save_command[1];
int	save_count = 1;

static const char *	spooldefault = "/usr/spool/pkg";

static void	PromptCB (BLAH_BLAH_BLAH);
static void	CancelCatalogCB (BLAH_BLAH_BLAH);
static void	CancelNoteCB (BLAH_BLAH_BLAH);
static void	EndExecCB (BLAH_BLAH_BLAH);
static void	SysViewCB (BLAH_BLAH_BLAH);
static void	ShowAppsCB (BLAH_BLAH_BLAH);
static void	SAppsCB (BLAH_BLAH_BLAH);
static void	InstallCB (BLAH_BLAH_BLAH);
static void	DeleteSetCB (BLAH_BLAH_BLAH);
static void	DevViewCB (BLAH_BLAH_BLAH);
static void	ExitCB (BLAH_BLAH_BLAH);
static void	FolderCB (BLAH_BLAH_BLAH);
static void	FindCB (BLAH_BLAH_BLAH);
static void	HideCB (BLAH_BLAH_BLAH);
static void	FindServerCB (BLAH_BLAH_BLAH);
static void	ServerCB (BLAH_BLAH_BLAH);
void		ReadFromServer(PackageRecord *, ListRec *);
void		NwCB (BLAH_BLAH_BLAH);
void		ErrorNotice();
void		OKCB();
int		CheckP3closeStatus();

HelpInfo	HelpIntro	= { 0, "", HELP_PATH, help_newintro };
HelpInfo	HelpTOC		= { 0, "", HELP_PATH, "TOC" };
HelpInfo	HelpDesk	= { 0, "", HELP_PATH, "HelpDesk" };
HelpInfo	HelpProps	= { 0, "", HELP_PATH, help_newprops };
HelpInfo	HelpIcons	= { 0, "", HELP_PATH, help_newicons };
HelpInfo	HelpFolder	= { 0, "", HELP_PATH, help_newfolder };
HelpInfo	HelpPkgwin	= { 0, "", HELP_PATH, help_newpkgwin };
HelpInfo   	HelpCatalog     = { 0, "", HELP_PATH, help_newcatalog };
HelpInfo        HelpPackage     = { 0, "", HELP_PATH, help_newpkg };
HelpInfo        HelpCompat      = { 0, "", HELP_PATH, help_newcompat };

static MenuItems	prompt_menu_item[] = {
	{ TRUE, label_select, mnemonic_select, 0, PromptCB, NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, PromptCB, NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (char *)&HelpIntro },
	{ NULL}
};
static MenuGizmo	prompt_menu = {0, "prompt", NULL, prompt_menu_item };

static FileGizmo	spool_prompt = {
	&HelpIntro, "spooldir", NULL, &prompt_menu, NULL, NULL,
	" ", FOLDERS_ONLY
};

static MenuItems actionItems[] = {
	{True, label_set_option2, mnemonic_option, 0, SetFormatCB},
	{True, label_exit,	mnemonic_exit, 0, ExitCB},
	{NULL}
};

static MenuGizmo actionMenu = {
	NULL, "actionMenu", NULL, actionItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems viewItems[] = {
	{True, " ",	mnemonic_one},
	{True, " ",	mnemonic_two},
	{NULL}
};

static MenuGizmo viewMenu = {
	NULL, "viewMenu", NULL, viewItems,
	HideCB, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems helpItems[] = {
  {True, label_intro2, mnemonic_intro, 0, helpCB, (char *)&HelpIntro},
  {True, label_toc, mnemonic_toc, 0, helpCB, (char *)&HelpTOC},
  {True, label_hlpdsk, mnemonic_hlpdsk, 0, helpCB, (char *)&HelpDesk},
  {NULL}
};

static MenuGizmo helpMenu = {
	NULL, "helpMenu", NULL, helpItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mainItems[] = {
	{True, label_action,	mnemonic_action,	(char *)&actionMenu},
	{True, label_view,	mnemonic_view,		(char *)&viewMenu},
	{True, label_help,	mnemonic_help,		(char *)&helpMenu},
	{NULL}
};

static MenuGizmo mainMenu = {
	NULL, "mainMenu", NULL, mainItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static StaticTextGizmo title1 = {
	NULL, "title1", "New Applications", CenterGravity
};


static MenuItems serverItems[] = {
	{False, label_server, mnemonic_server, NULL, NULL},
	{NULL}
};

static MenuGizmo findServer = {
	NULL, "findServer", NULL, serverItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};
static MenuItems findItems[] = {
	{True, label_find, mnemonic_find, NULL, FindCB},
	{NULL}
};

static MenuGizmo findMenu = {
	NULL, "findMenu", NULL, findItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static MenuItems errnote_item[] = {
        { TRUE, label_ok2,  mnemonic_ok, 0, OKCB },
        { NULL }
};

static  MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static  ModalGizmo errnote = {0, "warn", string_appError,(Gizmo)&errnote_menu };

static InputGizmo input = {
	NULL, "input", string_folder, NULL, 0, 20, SAppsCB
};

static GizmoRec array0[] = {
	{InputGizmoClass,	&input},
	{MenuBarGizmoClass,	&findMenu},
	{MenuBarGizmoClass,	&findServer}
};

static LabelGizmo folder = {
	NULL, "folder", "", array0, XtNumber (array0), OL_FIXEDROWS, 1,
	NULL, 0, True
};

/* allocate MAX_DEVS+2 (Other and Network install) + 1 (NULL, required by 
 * MenuGizmo) entries .
 */
static MenuItems choice1Items[MAX_DEVS+3] = {
	{True, label_network,	mnemonic_network, NULL, NULL},
	{True, label_spooled2,	mnemonic_spooled, NULL, NULL},
	{NULL}
};

static MenuGizmo choice1Menu = {
	NULL, "choice1Menu", NULL, choice1Items,
	DevViewCB, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static Setting firstChoiceSetting;

static ChoiceGizmo firstChoice = {
	NULL, "firstChoice", string_install_from, &choice1Menu,
	&firstChoiceSetting, 0
};

static GizmoRec choiceArray1[] = {
	{AbbrevChoiceGizmoClass,	&firstChoice}
};

static LabelGizmo choice1 = {
	NULL, "choice1", "", choiceArray1, XtNumber (choiceArray1),
	OL_FIXEDCOLS, 1, NULL, 0, True
};

static MenuItems addItems[] = {
	{True, label_update_view,mnemonic_update_view, NULL, ShowAppsCB},
	{True, label_add,	mnemonic_add,		NULL, InstallCB},
	{NULL}
};

static MenuGizmo addMenu = {
	NULL, "addMenu", NULL, addItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static ScrolledWindowGizmo firstSW = {
	"1st sw", 100, 20
};

static GizmoRec sw1array[] = {
	{ScrolledWindowGizmoClass,	&firstSW},
	{MenuBarGizmoClass,		&addMenu}
};

static RubberTileGizmo innerBox1 = {
	"innerBox1", NULL, OL_HORIZONTAL, sw1array, XtNumber (sw1array)
};

static GizmoRec array1[] = {
	{StaticTextGizmoClass,		&title1},
	{LabelGizmoClass,		&choice1},
	{LabelGizmoClass,		&folder},
	{RubberTileGizmoClass,		&innerBox1}
};

static RubberTileGizmo outerBox1 = {
	"outerBox1", "footer1", OL_VERTICAL, array1, XtNumber (array1)
};

static StaticTextGizmo title2 = {
	NULL, "title2", "", CenterGravity
};

static MenuItems choice2Items[] = {
	{True, label_all,	mnemonic_l_all},
	{True, label_apps,	mnemonic_apps},
	{True, label_system,	mnemonic_y_system},
	{NULL}
};

static MenuGizmo choice2Menu = {
	NULL, "choice2Menu", NULL, choice2Items,
	SysViewCB, NULL, EXC, OL_FIXEDROWS, 1, 0
};

static GizmoRec choiceArray2[] = {
	{MenuBarGizmoClass,	&choice2Menu}
};

static LabelGizmo choice2 = {
	NULL, "choice2", string_apps_to_show,
	choiceArray2, XtNumber (choiceArray2),
	OL_FIXEDROWS, 1, NULL, 0, True
};

static ScrolledWindowGizmo secondSW = {
	"2nd sw", 100, 20
};

static MenuItems removeItems[] = {
	{True, label_icons,	mnemonic_contents,	NULL, SetContentsCB},
	{True, label_iinfo,	mnemonic_n_info,	NULL, SetInfoCB},
	{True, label_delete,	mnemonic_delete,	NULL, DeleteSetCB},
	{NULL}
};

static MenuGizmo removeMenu = {
	NULL, "removeMenu", NULL, removeItems,
	0, NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static GizmoRec sw2array[] = {
	{ScrolledWindowGizmoClass,	&secondSW},
	{MenuBarGizmoClass,		&removeMenu}
};

static RubberTileGizmo innerBox2 = {
	"innerBox2", NULL, OL_HORIZONTAL, sw2array, XtNumber (sw2array)
};

static GizmoRec array2[] = {
	{StaticTextGizmoClass,		&title2},
	{LabelGizmoClass,		&choice2},
	{RubberTileGizmoClass,		&innerBox2}
};

static RubberTileGizmo outerBox2 = {
	"outerBox2", "footer2", OL_VERTICAL, array2, XtNumber (array2)
};

static GizmoRec array3[] = {
	{RubberTileGizmoClass,	&outerBox1},
	{RubberTileGizmoClass,	&outerBox2},
};

BaseWindowGizmo	base = {
	NULL, "base", string_newappName, &mainMenu,
	array3, XtNumber (array3), string_newiconName, ICON_NAME, " ", " ", 90
};

MenuItems	msg_item[] = {
	{ TRUE, label_cancel,mnemonic_cancel, 0, CancelCatalogCB },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB, (char *)&HelpCatalog },
	{ NULL }
};
MenuGizmo  msg_menu = {0, "msg", "msg", msg_item };

StaticTextGizmo msg_text[] = {
	0, "msg_text", " ", CenterGravity, 0
};

GizmoRec messages[] = {
	{StaticTextGizmoClass,	&msg_text}
};

PopupGizmo info_msg = {
	0, "catalog", string_newmsgTitle, (Gizmo)&msg_menu,
	messages, XtNumber (messages)
};

MenuItems	note_item[] = {
	{ TRUE, label_go,    mnemonic_go, 0, GetMedia },
	{ TRUE, label_cancel,mnemonic_cancel, 0, CancelNoteCB },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB,(char *)&HelpIntro},
	{ NULL }
};
MenuGizmo  note_menu = {0, "note", "note", note_item };
ModalGizmo insert_note = {0, "insert", string_newmediaTitle, (Gizmo)&note_menu};

MenuItems	cant_item[] = {
	{ TRUE, label_go,    mnemonic_go,   0, EndExecCB },
	{ TRUE, label_help,  mnemonic_help, 0, helpCB,(char *)&HelpIntro},
	{ NULL }
};
MenuGizmo  cant_menu = {0, "cant", "cant", cant_item };
ModalGizmo cant_exec = {
	0, "exec", string_newsvr3Title, (Gizmo)&cant_menu, string_cantExec
};

char		XTERM[PATH_MAX+16];	/* Command line for invoking xterm */
char *		our_node = NULL;	/* Name of this machine */
char *		spooled;		/* "Other..." */
char *		network;		/* "Network Install..." */
uid_t		our_uid;		/* Uid of user */
extern char *		our_login;
Boolean		owner;

static void
ExitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	exit (0);
}

/*
 * Set the labels on the view menus to be either "Hide" or "Show"
 * and manage or unmanage the window as appropriate.
 */
void
SetViewMenuLabels (int index)
{
	ListRec *	lp;
	Widget		w;
	Widget		menu;
	static char	l0[50];
	static char	l1[50];
	char *		str;
	char *		where;

	if (index == 0) {
		lp = pr->mediaList;
		w = (Widget)QueryGizmo (
			BaseWindowGizmoClass, pr->base,
			GetGizmoWidget, "outerBox1"
		);
	}
	else {
		lp = pr->sysList;
		w = (Widget)QueryGizmo (
			BaseWindowGizmoClass, pr->base,
			GetGizmoWidget, "outerBox2"
		);
	}
	menu = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "viewMenu"
	);
	str = l1;
	where = our_node;
	if (index == 0) {
		where = lp->curlabel;
		str = l0;
	}
	if (lp->hide == True) {
		XtUnmanageChild (w);
		XtSetArg (arg[0], XtNsensitive, False);
		sprintf (str, GGT(format_show_apps_on), where);
	}
	else {
		XtManageChild (w);
		XtSetArg (arg[0], XtNsensitive, True);
		sprintf (str, GGT(format_hide_apps_on), where);
	}
	OlFlatSetValues (menu, !index, arg, 1);
	XtSetArg (arg[0], XtNlabel, str);
	OlFlatSetValues (menu, index, arg, 1);
}

static void
HideCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *	olcd = (OlFlatCallData *) call_data;
	int			index = olcd->item_index;

	if (index == 0) {
		pr->mediaList->hide = !pr->mediaList->hide;
	}
	else {
		pr->sysList->hide = !pr->sysList->hide;
	}
	SetViewMenuLabels (index);
}

static void
ReadFromFolder (PackageRecord *pr, ListRec *lp)
{
	pr->have_dir = TRUE;
	lp->viewType = SPOOL_VIEW;
	GetUninstalledList(lp, pr->spooldir, lp->curlabel);
}

void
ReadFromServer (PackageRecord *pr, ListRec *lp)
{
	pr->have_dir = TRUE;
	lp->viewType = NETWORK_VIEW;
	GetListFromServer(lp, pr->servername);
}


/*
 * This is the callback from the "Other..." pathfinder window.
 * This routine handles Select and Cancel.
 */

static void
PromptCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData  *olcd = (OlFlatCallData *) call_data;
	int             n = olcd->item_index;
	ListRec *	lp = pr->mediaList;
        InputGizmo *    ip;

	BringDownPopup(GetFileGizmoShell(pr->prompt));
	if (n == 1) {	/* Cancel */
		if (lp->timeout) {
			XtRemoveTimeOut(lp->timeout);
			lp->timeout = (XtIntervalId)NULL;
		}
	}
	else {		/* Select */
		pr->spooldir = GetFilePath(pr->prompt);
                ip = QueryGizmo (BaseWindowGizmoClass, 
				 pr->base, 
				 GetGizmoGizmo, 
				 "input"
                );
                XtVaSetValues(ip->textFieldWidget,
                              XtNstring, pr->spooldir,
                              (String)0
                );
		ReadFromFolder (pr, lp);
	}
}

/*
 * Look at the time stamps on /var/sadm/pkg and /var/options and /etc/perms.
 * If any of the directories has been touched since the last cataloging was done
 * the mark the list as being out of date (pr->sysList->validList==False);
 */
static void
CheckPkgDirs ()
{
	PkgPtr		p;
	time_t		time;
	ListRec *	lp = pr->mediaList;

	time = (pr->pkgDirDate > pr->optionDate ? pr->pkgDirDate : 
		                                 pr->optionDate);
	time = (time > pr->permsDate ? time : pr->permsDate);

	pr->pkgDirDate = StatFile (PKGDIR);
	pr->optionDate = StatFile (OPTIONS);
	pr->permsDate  = StatFile (PERMS);
	if (time < pr->pkgDirDate || time < pr->optionDate || 
	    time < pr->permsDate) { 
		/*lp->validList = False;*/
		for (p=lp->pkg; p<lp->pkg+lp->count; p++) {
			UpdateMediaListIcon (p, PkgInstalled(p));
		}
		pr->sysList->validList = False;
	}
}

/*
 * Catalogue installed packages and display the according to the view.
 * Invoked from any "View:Installed Appl-ns" button.
 */
static void
SysViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = pr->sysList;
	StaticTextGizmo	*	g;

	FooterMsg2(NULL);
	lp->viewType = d->item_index;
	XtSetArg(arg[0], XtNlabel, &lp->curlabel);
	OlFlatGetValues(w, d->item_index, arg, 1);

	pr->have_dir = FALSE;
	/* Look to see if any packaging directories have been changed. */
	CheckPkgDirs ();
	if (lp->validList == False) {
		FindInstalled();
	}
	else {
		CreateSetIcons (lp, lp->curlabel);
	}
}

/*
 * Invoke removepkg() (no arguments)
 */

static void
CallRemovePkg(char *name)
{
	char		str[128];
	ListRec *	lp = pr->sysList;

	sprintf(str,"%s\"%s%s\" -e %s/dtexec -ZN /usr/bin/removepkg %s", XTERM,
					GGT(string_remTitle), name,
					GetXWINHome("adm"), name);
	setuid(0); 
	if (P3OPEN(str, lp->cmdfp, FALSE) != -1) {
		FooterMsg2 (GGT(string_invokeRemPkg));
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)Wait3_2Pkg, NULL
		);
	}
	else {
		setuid(our_uid);
		FooterMsg2 (GGT(string_badRemPkg));
	}
}

/*
 * Invoked by "Remove..." from main window (sets)
 */

static void
DeleteSetCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	int		n, callpkgrm = 0;
	char		*ptr, names[BUFSIZ];
	Boolean		did_svr3_2 = FALSE;
	Boolean		chosen;
	DmObjectPtr	optr;
	PkgPtr		p_sel;
	ListRec *	lp = pr->sysList;
	int		del_num = 0;

	pr->delete = SET_DELETE;
	DontAllowAddOrDelete ();
	for (*names = '\0', n = 0; n < lp->setCount; n++) {
		optr = (DmObjectPtr)lp->setitp[n].object_ptr;
		p_sel = (PkgPtr)optr->objectdata;
		if (lp->setitp[n].select && lp->setitp[n].managed) {
			/* Call SetPkgDefs() inorder to set the HELP
			 * field in the package record such that the
			 * help files will also be removed.
			 */
			SetPkgDefs (p_sel);

			FindFuture(p_sel, del_num);
			del_num++;

			switch(p_sel->pkg_fmt[0]) {
				case '3': {
					if (!did_svr3_2) {
						did_svr3_2 = TRUE;
						CallRemovePkg(p_sel->pkg_name);
						/*
						 *	removepkg will ask for
						 *	specific packaged via menus
						 */
					}
					break;
				}
				case 'C': {
					CallCustomDel (
						pr, p_sel->pkg_name,
						" ALL", our_uid
					);
					break;
				}
				default: {
					strcat (
						strcat(names," "),
						p_sel->pkg_name
					);
					callpkgrm++;
					break;
				}
			}
			SetBusy(TRUE);
		}
	}
	if (callpkgrm) {
		sprintf (
			buf, "%s\"%s%s\" -e %s/dtexec -N %s %s",
			XTERM, GGT(string_remTitle), names, 
			GetXWINHome("adm"), REMPKG, names
		);
		if (P3OPEN(buf, lp->cmdfp, FALSE) != -1) {
			FooterMsg2 (GGT(string_invokePkgOp));
			lp->timeout = XtAddTimeOut (
				1000, (XtTimerCallbackProc)WaitRemovePkg,
				STRDUP(names)
			);
		}
		else {
			FooterMsg2 (GGT(string_badPkgOp));
			lp->timeout = 0;
		}
	}
}

static void
FolderCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	InputGizmo *	ip;
	ListRec *	lp = pr->mediaList;

	FooterMsg2(NULL);
	ip = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "input"
	);
	XtVaGetValues (
		ip->textFieldWidget, XtNstring, &pr->spooldir, (String)0
	);
	if (access(pr->spooldir,F_OK) != 0){
		ErrorNotice(GGT(string_badFolder), 
			GetGizmoText(string_appError));
		return;
	}
	if (pr->spooldir && *pr->spooldir)
	        ReadFromFolder (pr, lp);
}

static void
ServerCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	InputGizmo *	ip;
	ListRec *	lp = pr->mediaList;
	char    	buf[BUFSIZ];
	char 		*servername, *token;

	ip = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "input"
	);
	XtVaGetValues (
		ip->textFieldWidget, XtNstring, &pr->servername, (String)0
	);
	lp->viewType = NETWORK_VIEW;

	/*
 	* Check for spaces in Server name.
 	*/
	if ( strchr(pr->servername, ' ') != 0 ){
		ErrorNotice(GGT(string_noSpacesAllowed), 
			GetGizmoText(string_appError));
		return;
	}
	/*
 	* Check for ":" (colon) in Server name.
 	* For example: "seuss:/var/spool/dist" . 
 	* If colon is found, we need to separate server name.
 	*/
	if ( strchr(pr->servername, ':') != 0 ){
		token = STRDUP(pr->servername);
		servername = strtok(token,":");
		sprintf(buf,"%s %s > /dev/null 2>&1", PING, servername);
		FREE (token);
	}
	else
		sprintf(buf,"%s %s > /dev/null 2>&1", PING, pr->servername);
	if ( system(buf) != 0 ) {
		ErrorNotice(GGT(string_badServer2), 
			GetGizmoText(string_appError));
		return;
	}
	if (pr->servername && *pr->servername)
	        ReadFromServer (pr, lp);
}

static void
FindCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	pr->have_dir = FALSE;
	MapGizmo(FileGizmoClass, pr->prompt);
}

static void
FindServerCB (Widget w, XtPointer client_data, XtPointer call_data)
{
        if (s_cnt == 0) MakeServerList();
        MapGizmo(PopupGizmoClass, pr->nservers);
}

static char *
LocalAlias(char *i18nalias)
{

        int     n;

        for (n = 0;  n < SPOOL_VIEW; n ++) {
                if (strcmp(alias[n].list, i18nalias) == 0)
                        return alias[n].value;
	      }
        return i18nalias;

}

/*
 * Initiate cataloging of the target device for installations.
 * Invoked from the "View:Uninstalled Appl-ns" button.
 */

static void
DevViewCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	ListRec *		lp = pr->mediaList;
	Widget			w, w3, w4;
	InputGizmo		*w2;
	Time			t = CurrentTime;


	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "folder"
	);
	FooterMsg1(NULL);
	XtSetArg(arg[0], XtNlabel, &lp->curlabel);
	OlFlatGetValues(wid, d->item_index, arg, 1);
	if (d->item_index == SPOOL_VIEW || d->item_index == NETWORK_VIEW) {
		w2 = QueryGizmo (
			BaseWindowGizmoClass, pr->base,
			GetGizmoGizmo, "input"
		);
	        w3 = QueryGizmo (BaseWindowGizmoClass, pr->base, 
			      GetGizmoWidget, "findMenu"
		);
	        w4 = QueryGizmo (BaseWindowGizmoClass, pr->base, 
			      GetGizmoWidget, "findServer"
		);
		if (d->item_index == NETWORK_VIEW) {
		        if (XtIsManaged(w3)==True)
			           XtUnmanageChild (w3);
			if (XtIsManaged(w4)==True)
			           XtUnmanageChild (w4); 
		        XtVaSetValues(w2->captionWidget,
				   XtNlabel, GGT(label_nwname),
				   0
			);
		        XtVaSetValues (w2->textFieldWidget, 
				       XtNstring, "",
				       (String)0
			);			
			pr->curalias = network;
			pr->mediaList->viewType = NETWORK_VIEW;
		}
		else  {
		        if (XtIsManaged(w4)==True)
			           XtUnmanageChild (w4); 
			if (XtIsManaged(w3)==False)
			           XtManageChild (w3);

		        XtVaSetValues(w2->captionWidget,
				      XtNlabel, GGT(string_folder),
				      0
			);
		        XtVaSetValues (w2->textFieldWidget, 
				       XtNstring, spooldefault,
				       (String)0
			);
			pr->mediaList->viewType = SPOOL_VIEW;
			pr->curalias = spooled;
		}
		XtManageChild (w);
		XtCallAcceptFocus (w2->textFieldWidget, &t);
		pr->defaultcb = False;
		SetMediaListTitle(pr);
	}
	else {
		XtUnmanageChild (w);
		BringDownPopup(GetFileGizmoShell(pr->prompt));
		pr->mediaList->viewType = DEV_VIEW;
		pr->curalias = MapAlias(LocalAlias(lp->curlabel));
		SetMediaListTitle(pr);
		GetMedia(NULL, NULL, NULL);
	}
}

static void
EndExecCB(wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	BringDownPopup(pr->cantExec->shell);
}

static	void
CancelCatalogCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	ListRec *	lp = pr->mediaList;

	BringDownPopup (pr->catalog->shell);
	if (lp->cmdfp[1]) {
		_Dtam_p3close(lp->cmdfp, SIGINT);
		lp->cmdfp[0] = lp->cmdfp[1] = (FILE *)NULL;
	}
	SetMediaListTitle (pr);
	SetSetSensitivity (pr->mediaList);
}

static	void
CancelNoteCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	ListRec *	lp = pr->sysList;
	ChoiceGizmo *	g;

	BringDownPopup(pr->insertNote->shell);
	g = QueryGizmo (
		BaseWindowGizmoClass, pr->base,
		GetGizmoGizmo, "firstChoice"
	);
	XtVaGetValues (
		g->previewWidget, XtNstring, 
		&pr->mediaList->curlabel, (String)0
	);
	pr->curalias = MapAlias(pr->mediaList->curlabel);
	if (lp->timeout) {
		XtRemoveTimeOut(lp->timeout);
		lp->timeout = (XtIntervalId)NULL;
	}
}

static void
PropertyEventHandler (
	Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *cont_to_dispatch
)
{
	DtReply		reply;
	int		ret;

	if (xevent->type != SelectionNotify)
		return;
	memset(&reply, 0, sizeof(reply));
	if (xevent->xselection.selection == help_atom) {
		ret = DtAcceptReply(XtScreen(w),
			help_atom, XtWindow(w), &reply);
		OlDnDFreeTransientAtom(pr->base->shell, help_atom);
	}
	if (xevent->xselection.selection == class_atom) {
		ret = DtAcceptReply(XtScreen(w),
			class_atom, XtWindow(w), &reply);
		OlDnDFreeTransientAtom(pr->base->shell, class_atom);
	}
}

/*
 * Create the path finder window that the user will use to access
 * "Other..." packages.
 */

static void
CreateFolderPrompt()
{
	Widget		w;
	FileGizmo *	ipg;

	pr->prompt = CopyGizmo(FileGizmoClass, &spool_prompt);
	pr->prompt->title = GGT(string_spoolPrompt);
	pr->prompt->directory = STRDUP (spooldefault);
	CreateGizmo(pr->base->shell, FileGizmoClass, pr->prompt, NULL, 0);
	/* Set the default directory in the input field.
	 * This could be different from "/var/spool/pkg" because the
	 * file gizmo processes links.
	 */
	ipg = QueryGizmo (
		FileGizmoClass, pr->prompt, GetGizmoGizmo, "spooldir"
	);
	w = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "input"
	);
        XtVaSetValues (w, XtNstring, ipg->directory, (String)0);
	SetFileGizmoMessage(pr->prompt, GGT(string_promptMsg));
	XtUnmanageChild(XtParent(pr->prompt->textFieldWidget));
}

static void
InitListRecord (ListRec **list)
{
	ListRec *	lp;

	lp = (ListRec *) MALLOC (sizeof (ListRec));
	lp->max = QUANTUM;
	lp->pkg = (PkgPtr)MALLOC (
		(lp->max+1)*sizeof(PkgRec)
	);
	lp->count = 0;
	lp->setCount = 0;
	lp->timeout = (XtIntervalId)0;
	lp->setx = INIT_X;
	lp->sety = INIT_Y;
	lp->setBox = (Widget)0;
	lp->validList = False;
	lp->hide = False;
	lp->sw = NULL;
	*list = lp;
}

static void
GetAlias (PackageRecord *pr, ListRec *lp, char *name)
{
	char *		dev;
	static char *	type = "removable=\"true";

	if (name == 0) {
		if ((dev = DtamGetDev(type,FIRST)) == NULL) {
			return;
		}
	}
	else if ((dev = DtamGetDev(name, FIRST)) == NULL) {
		return;
	}
	/*
	 * Given the name of a device such as:
	 *    "diskette1",
	 *    "5.25 inch 1.2 Mbyte" 
	 *    "Packaging Spool Directory"
	 * Return the alias:
	 *    "diskette1"
	 *    "mdens2HIGH"
	 *    "spool"
	 * obtained from /etc/device.tab.
	 */
	pr->curalias = DtamDevAttr(dev, ALIAS);
	lp->curlabel = DtamDevAlias(dev);
	FREE(dev);
}

static void
PostInitPackageRecord ()
{
	RubberTileGizmo *	rt;

	pr->mediaList->title = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "title1"
	);
	pr->sysList->title = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "title2"
	);
	rt = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "outerBox1"
	);
	pr->mediaList->footer = GetRubberTileFooter (rt);
	rt = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "outerBox2"
	);
	pr->sysList->footer = GetRubberTileFooter (rt);

	/* Don't use the base window's footer */
	XtUnmanageChild (pr->base->message);

	pr->mediaList->sw = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "1st sw"
	);
	pr->sysList->sw = QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "2nd sw"
	);
	SetViewMenuLabels (0);
	SetViewMenuLabels (1);
}

static void
PreInitPackageRecord ()
{
	InitListRecord (&pr->sysList);
	InitListRecord (&pr->mediaList);

	pr->optionDate = StatFile (OPTIONS);
	pr->pkgDirDate = StatFile (PKGDIR);
	pr->permsDate  = StatFile (PERMS);
	pr->sysList->viewType = ALL_VIEW;
	pr->mediaList->viewType = DEV_VIEW;
	pr->installInProgress = False;
	pr->have_dir = False;
	pr->pkgBoxUp = False;
	pr->pkgPopup = (PopupGizmo *)0;
	pr->pkgsw = NULL;
	pr->iconBoxUp = False;
	pr->folderBoxUp = False;
	pr->iconPopup = (PopupGizmo *)0;
	pr->noIconPopup = (ModalGizmo *)0;
	pr->iconsw = NULL;
	pr->curalias = NULL;
	pr->pkgBox = (Widget)0;
	pr->pkgCount = 0;
	pr->labelAddOn = GGT (label_apps);
	pr->labelAll = GGT (label_all);
	pr->labelSystem = GGT (label_system);
	pr->sysList->curlabel = pr->labelAll;
	pr->defaultcb = True;
}

static char *
GetDevName(char *devline)
{
        char buf[80];
	int  numCD;

        if (strncmp(devline, CTAPE1, sizeof(CTAPE1)-1)==0) {
                strcpy(buf, GGT(alias_ctape1));
	}
        else if (strncmp(devline, CTAPE2, sizeof(CTAPE2)-1)==0) {
                strcpy(buf, GGT(alias_ctape2));
	}
        /* for CDROMs the first field in devline is of the form cdrom#
         * where # can be any integer.  Parse out "cdrom" from the
         * string and translate then re-append the number.
         */
        else if (strncmp(devline, CDROM, sizeof(CDROM)-1)==0) {
                sscanf(devline+sizeof(CDROM)-1, "%d", &numCD);
                sprintf(buf, "%s_%d", GGT(alias_cdrom), numCD);
        }
        else if (strncmp(devline, DISKETTE, sizeof(DISKETTE)-1)==0) {
                /*
                 *      translate "diskette1" etc. to "Disk_A" etc
                 *      (where the etc. means that one sequence is
                 *      mapped to another, starting with the I18N
                 *      tag_disk character.
                 */
                char    c = devline[sizeof(DISKETTE)-1];
                char    A = *GGT(tag_disk);
                sprintf(buf, GGT(alias_disk), c + A - '1');
	}
        else
                return DtamDevAttr(devline, DESC);
        return STRDUP(buf);
}


/*
 * Get the labels for the items in the "View:Uninstalled Appl'ns" menu.
 * This menu gets filled with the various devices such as Disk_A
 * and Disk_B.
 */

static void
InitDevices()
{
	static char *	type = "removable=\"true";
	char *		dev;
	char *		attr;
	int		n = 0;
	int		index = 0;

	for (dev = DtamGetDev(type,FIRST); dev; dev = DtamGetDev(type,NEXT)) {
		if ((dev[0] == '#') || strstr(dev,"display=\"false")) {
			FREE(dev);
			continue;
		}
		attr = DtamDevAttr(dev, CDEVICE);
		if (access(attr, R_OK) == -1) {
			FREE(attr);
			FREE(dev);
			continue;
		}
		choice1Items[n+2] = choice1Items[n+1];  /* bump up "Other"   */
		choice1Items[n+1] = choice1Items[n];	/* bump up "Network" */
		alias[n].value = DtamDevAlias(dev);
		attr = GetDevName(DtamDevAttr(dev, ALIAS));
		alias[n].list = attr;
		if (strcmp (pr->mediaList->curlabel, attr) == 0) {
			index = n;
		}
		FREE(dev);
		choice1Items[n].sensitive = TRUE;
		choice1Items[n].label = alias[n].value;
		choice1Items[n].mnemonic=alias[n].value+strlen(alias[n].value)-1;
		if (++n >= MAX_DEVS)
			break;
	}
	/* Set the label of the preview widget for this menu */
	firstChoiceSetting.previous_value = (XtPointer)index;
	NETWORK_VIEW = n;
	SPOOL_VIEW = n + 1;
}

static void
SetGravityWeight (PackageRecord *pr)
{
	Widget		w;
	XFontStruct *	boldFont;

	boldFont = _OlGetDefaultFont (
		pr->mediaList->title->widget, OlDefaultBoldFont
	);
	XtVaSetValues (
		pr->mediaList->title->widget,
		XtNweight,	0,
		XtNpaneGravity,	CenterGravity,
		XtNfont,	boldFont,
		(String)0
	);
	XtVaSetValues (
		pr->sysList->title->widget,
		XtNweight,	0,
		XtNpaneGravity,	CenterGravity,
		XtNfont,	boldFont,
		(String)0
	);
	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "folder"
	);
	XtVaSetValues (
		w,
		XtNweight,	0,
		XtNpaneGravity,	WestGravity,
		(String)0
	);
	XtUnmanageChild (w);

	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "choice1"
	);
	XtVaSetValues (
		w,
		XtNweight,	0,
		XtNpaneGravity,	WestGravity,
		(String)0
	);
	XtVaSetValues (pr->mediaList->sw, 
		       XtNweight, 1, 
		       XtNhStepSize, (INC_X/2),
		       XtNvStepSize, (INC_Y/2),
		       (String)0
	);
	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "choice2"
	);
	XtVaSetValues (
		w,
		XtNweight,	0,
		XtNpaneGravity,	WestGravity,
		(String)0
	);
	XtVaSetValues (pr->sysList->sw, 
		       XtNweight, 1, 
		       XtNhStepSize, (INC_X/2),
		       XtNvStepSize, (INC_Y/2),
		       (String)0
	);
	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "addMenu"
	);
	XtVaSetValues (w, XtNweight, 0, (String)0);
	w=QueryGizmo (
		BaseWindowGizmoClass, pr->base, GetGizmoWidget, "removeMenu"
	);
	XtVaSetValues (w, XtNweight, 0, (String)0);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	StaticTextGizmo	*	g;
	struct utsname		sys_name;
	Window			win;
	char *			dev;
	char			buf[128];
	int			n;
	char			tmpalias[BUFSIZ];
	struct passwd		*user_info;

	setlocale(LC_ALL, "");
#ifdef	MEMUTIL
	InitializeMemutil();
#endif
        (void)setsid();		/* become a session leader (divorce dtm) */

        /* undo some of the stuff we inherit from dtm */
        sigset(SIGCHLD, SIG_DFL);
        sigset(SIGINT,  SIG_DFL);
        sigset(SIGQUIT, SIG_DFL);
        sigset(SIGTERM, SIG_DFL);

	sprintf(XTERM, "%s/xterm -T ", GetXWINHome("bin"));
	uname(&sys_name);
	our_node = sys_name.nodename;
	our_uid = getuid();
	user_info = getpwuid(our_uid);
	our_login = strdup(user_info->pw_name);
	/*
	 *	owner will setuid(0) to perform custom/installpkg operations
	 *	and revert to this (for linking files, for example) when done
	 */
	owner = _DtamIsOwner(OWN_PACKAGE);
	chdir("/");

	addItems[1].sensitive  =
        removeItems[2].sensitive  = (XtArgVal) owner;

	PreInitPackageRecord ();

	while ((n = getopt(argc, argv, "D:")) != EOF) {
		switch (n) {
		case 'D':	strcpy (tmpalias, optarg);
				strcat (tmpalias, ":");
				GetAlias (pr, pr->mediaList, tmpalias);
				media_context = FIRST;
				break;
		}
	}
	
	if (media_context != FIRST) {
		/* If the alias hasn't been set by the -D flag then set */
		/* it now */
		GetAlias (pr, pr->mediaList, NULL);
	}

	InitDevices();
	if ((pr->deskTopDir = (char *)getenv("DESKTOPDIR")) == NULL)
		if ((pr->deskTopDir = (char *)getenv("HOME")) == NULL)
			pr->deskTopDir = ".";

	OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
	w_toplevel = XtInitialize("packager","PackageMgr",NULL,0,&argc,argv);
	DtInitialize(w_toplevel);
	InitializeGizmos(PROGRAM_NAME, PROGRAM_NAME);
	_DtamWMProtocols(w_toplevel);

	spooled  = GGT(label_spooled2);
	network  = GGT(label_network);
	theScreen = XtScreen(w_toplevel);
	theDisplay = XtDisplay(w_toplevel);
	def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);
	x3mm = OlMMToPixel(OL_HORIZONTAL,4);
	y3mm = OlMMToPixel(OL_VERTICAL,4);
	base.icon_name = GGT(base.icon_name);
	pr->base = CopyGizmo (BaseWindowGizmoClass, &base);
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, pr->base, NULL, 0);

	PostInitPackageRecord ();
	SetGravityWeight (pr);
	SetMediaListTitle (pr);
	XtSetArg(arg[0], XtNwidth,  WIDTH);
	XtSetArg(arg[1], XtNheight, HEIGHT);
	XtSetValues (pr->mediaList->sw, arg, 2);
	XtSetValues (pr->sysList->sw, arg, 2);
	XtRealizeWidget(pr->base->shell);
	win = DtSetAppId(theDisplay, XtWindow(pr->base->shell), "dtpkg");
	if (win != None) {
		XMapWindow(theDisplay, win);
		XRaiseWindow(theDisplay, win);
		XFlush(XtDisplay(pr->base->shell));
		exit(0);
	}

	/* Create the "Application Installer: Cataloging" popup */
	pr->catalog = CopyGizmo (PopupGizmoClass, &info_msg);
	CreateGizmo(pr->base->shell, PopupGizmoClass, pr->catalog, NULL, 0);
	XtUnmanageChild (pr->catalog->message);

	/* Create the "Application Installer: Package Media" popup */
	pr->insertNote = CopyGizmo (ModalGizmoClass, &insert_note);
	CreateGizmo(pr->base->shell, ModalGizmoClass, pr->insertNote, NULL, 0);

	/* Create popup for displaying No Permission message */
	pr->cantExec = CopyGizmo(ModalGizmoClass, &cant_exec);
	CreateGizmo(pr->base->shell, ModalGizmoClass, pr->cantExec, NULL, 0);

	/* Create property sheet */
	CreateInfoSheet(pr);

	/* Create icon sheet popup */
	CreateIconSheet();

	/* Create no programs notice */
	CreateNoIconSheet();

	/* Create path finder popup for "Other..." packages */
	CreateFolderPrompt();

	/* Create Copy to Folder popup */
	CreateCopyToFolder();

	/* create icon box with logins */
	unpkg_fcrec.glyph  = DmGetPixmap(theScreen, "unpkgmgr.glyph");
	unset_fcrec.glyph  = DmGetPixmap(theScreen, "unpkgset.glyph");
	pkg_fcrec.glyph  = DmGetPixmap(theScreen, "pkgmgr.glyph");
	set_fcrec.glyph  = DmGetPixmap(theScreen, "pkgset.glyph");
	exec_fcrec.glyph = DmGetPixmap(theScreen, "exec.icon");
	exec_fcrec.cursor = DmGetCursor(theScreen, "exec.icon");
	pkg_cntrec.count = 1;

	MapGizmo(BaseWindowGizmoClass, pr->base);
	pr->sysList->curlabel = GGT(label_all);
	FindInstalled ();
	if (media_context == FIRST)
	        GetMedia (NULL, (XtPointer)True, NULL);
	XtAddEventHandler(pr->base->shell, (EventMask)NoEventMask, True,
			PropertyEventHandler, (XtPointer)NULL);
	XtMainLoop();
}

/*
 * Invoked by "Application:Install" button
 */

static void
InstallCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char *		ptr;
	char		names[BUFSIZ];
	Boolean		chosen;
	Boolean		cust_fmt = FALSE;
	int		n;
	ListRec *	lp = pr->mediaList;
	PkgPtr		p_sel = GetSelectedSet (lp);
	char	*	save_var;
	char		env_buf[BUFSIZ];

	if (!p_sel) {
		FooterMsg1(GGT(string_noSelect));
		return;
	}
	for (*names = '\0', n = 0; n < lp->setCount; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		OlFlatGetValues (lp->setBox, n, arg, 1);
		if (chosen) {
			sprintf(
				names+strlen(names),
				" %s",
				lp->pkg[n].pkg_name);
			cust_fmt = (
				lp->pkg[n].pkg_fmt[0] == 'C'
			);
		}
	}
	if (*names == '\0')
		return;
	/*
	 *	note: the following is adapted to adding either a list of
	 *	pkgadd format add-ons, or a *single* custom format add-on.
	 *	If multiple custom packages are found, e.g. in a server
	 *	context, or if custom and pkgadd formats can be mixed, it
	 *	will be necessary to run several commands in succession --
	 *	a single pkgadd will handle all the pkgadd format add-ons,
	 *	at least one subsequent invocation of custom will be needed
	 *	to handle that format (probably, one should do a single call
	 *	to custom with the FIRST name in the list, and assume that
	 *	the user will have go on to handle the others wanted.
	 */
	if (cust_fmt) {
		char		c = pr->curalias[strlen(pr->curalias)-1] - 1;
		char	*	scompat = ScompatVariable ();
		if (c == '0')
			c = ' ';
		if (scompat == NULL) {
			sprintf (
				buf,
				"%s\"%s%s\" -e %s/dtexec -ZN %s -m /dev/install%c",
				XTERM,
				GGT(string_addTitle), names,
				GetXWINHome("adm"), CUSTOM, c
			);
		}
		else {
			sprintf (
				buf,
				"SCOMPAT=%s %s\"%s%s\" -e %s/dtexec -ZN %s -m /dev/install%c",
				scompat, XTERM,
				GGT(string_addTitle), names,
				GetXWINHome("adm"), CUSTOM, c
			);
		}
		setuid(0);
	}
	else {
	        int e_file;
		char nwexec[] = "/tmp/.nwexec";

	        if (pr->curalias == network) {
	                e_file = open(nwexec, O_CREAT | O_RDWR | O_TRUNC);
			sprintf (buf, "for pkg in %s\ndo\n", names);
	                write(e_file, &buf, strlen(buf));

		        sprintf (buf, 
				 "%s -s %s: $pkg 2> /dev/null | %s -p -q -d - $pkg\ndone\n",
			         CATPKG, pr->servername, ADDPKG
			);

	                write(e_file, &buf, strlen(buf));

 	                close(e_file);
	                chmod(nwexec, S_IRWXU|S_IRWXG|S_IRWXO);

		        sprintf (buf,"%s\"%s%s\" -e %s/dtexec -N %s",
			XTERM,GGT(string_addTitle),names,GetXWINHome("adm"),
			nwexec);
		}
		else  {
		        sprintf (
			buf,
			"%s\"%s%s\" -e %s/dtexec -ZN %s -p -q -d %s %s",
			XTERM, GGT(string_addTitle), names, 
			GetXWINHome("adm"), ADDPKG,
			pr->curalias==spooled?pr->spooldir:pr->curalias,names
			);
		}
	}
	DontAllowAddOrDelete ();

        if ((save_var = getenv("INSTALL_ICONS"))==NULL)
	        sprintf(env_buf, "INSTALL_ICONS=");
	else
	        sprintf(env_buf, "INSTALL_ICONS=%s", save_var);

	putenv("INSTALL_ICONS=FALSE");

	if (P3OPEN(buf, lp->cmdfp, FALSE) != -1) {
		FooterMsg1(GGT(cust_fmt? string_invokeCustom:
						 string_invokePkgOp));
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)WaitInstallPkg,
			STRDUP(names+1)
		);
		SetBusy(TRUE);
	}
	else {
		setuid(our_uid);
		FooterMsg1(GGT(string_badPkgOp));
		lp->timeout = 0;
	}
	
	putenv(env_buf);
}

static void
SAppsCB(Widget w, XtPointer client_data, XtPointer call_data)
{
        pr->defaultcb = False;
	ShowAppsCB(w, client_data, call_data);
}

static void
ShowAppsCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	OlUpdateDisplay(w);  
        if (pr->curalias == spooled) {
	        FolderCB (w, client_data, call_data);
	}
	else if (pr->curalias == network) {
	        ServerCB (w, client_data, call_data);
	}
	else {
	        GetMedia(NULL, NULL, NULL);
	}
}

void
ErrorNotice (char *msg, char *title)
{
	if (!errnote.shell)
		CreateGizmo(w_toplevel, ModalGizmoClass, &errnote, NULL, 0);
	SetModalGizmoMessage(&errnote, msg);
	OlVaFlatSetValues(errnote_menu.child, 0,
		XtNclientData, (XtArgVal)0, 0);
	XtVaSetValues(errnote.stext, XtNalignment, 
		(XtArgVal)OL_LEFT, NULL);
	if (title != NULL)
		XtVaSetValues(errnote.shell, XtNtitle, (XtArgVal)title, NULL);
	MapGizmo(ModalGizmoClass, &errnote);
}

void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    BringDownPopup(errnote.shell);
}

int
CheckP3closeStatus(int status)
{
	int		exitCode = 0;

	exitCode = WEXITSTATUS(status);
	switch (exitCode) {
	case 0:	 /* successful */
		break;
	case 1: /* some type failure */
		ErrorNotice(GetGizmoText(format_cantList), 
			GetGizmoText(string_appCatalog));
		break;
	case 2: /* old daemon detected and used (successful) */
		/* S.E said no message for now. */
		break;
	default:
		break;
	}
	return exitCode;
}
