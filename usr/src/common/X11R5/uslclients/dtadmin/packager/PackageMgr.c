#ifndef NOIDENT
#pragma ident   "@(#)PackageMgr.c	15.1"
#endif
/*
 *	PackageMgr - browse, install and delete software packages
 */
#include <signal.h>
#include <stdio.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>

#include "packager.h"
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/ListGizmo.h>
#include "SWGizmo.h"
#include "RTileGizmo.h"
#include <errno.h>
#include <pwd.h>

#define POPUP_INFO	0
#define POPUP_ICON	1
#define BUNCH   	16
#define U_QUANT 	(BUNCH*sizeof(UserRec))
#define MAX_SERVERS	BUFSIZ
#define USERHEIGHT     (XtArgVal)5

extern 	    int errno;
extern 	    long _dtam_flags;
Boolean	    isfolder = 0;
int	    nwinstall = 0;

/*
 * Forward declarations */
static void	InstallIconCB (BLAH_BLAH_BLAH);
static void	CancelIconCB (BLAH_BLAH_BLAH);
static void	CancelNoIconCB (BLAH_BLAH_BLAH);
static void	ApplyFolderCB (BLAH_BLAH_BLAH);
static void	ResetFolderCB (BLAH_BLAH_BLAH);
static void	CancelFolderCB (BLAH_BLAH_BLAH);
static void	SpecifyCB (BLAH_BLAH_BLAH);
static void	MoreFoldersCB (BLAH_BLAH_BLAH);

static Boolean	MakeFolderLists(void);
static Boolean	read_users = False;
static void	PopupFolderBox(PkgPtr);
void		MakeServerList(void);
void		PopupInstalledFolders(ListRec *);
void		FindFuture(PkgPtr, int);
extern void	ReadFromServer(PackageRecord *, ListRec *);
extern void	NwCB (BLAH_BLAH_BLAH);

typedef struct  passwd  UserRec, *UserPtr;
UserPtr 	u_list = (UserPtr)0;
UserPtr 	u_reset;
int     	u_cnt = 0;

char		*s_list[MAX_SERVERS];
int		s_cnt = 0;

int     	*uid_list;
char		*our_login;

struct  _FListItem {
        char    *label;
        Boolean set;
};

typedef struct 	_FListItem FListItem, *FListPtr;
FListPtr 	 UserItems = (FListPtr)0, ServerItems = (FListPtr)0;
char    	 *ListFields[] = { XtNlabel, XtNset };

Boolean	media_context = NEXT;	/* If this flag == FIRST then PackageMgr */
				/* was started with the -D option */

MenuItems	icon_menu_item[] = {  
	{FALSE, label_copy,mnemonic_copy, 0, InstallIconCB,  NULL },
	{TRUE,  label_cancel,mnemonic_cancel, 0, CancelIconCB,   NULL },
	{TRUE,  label_help,mnemonic_help, 0, helpCB, (char *)&HelpPackage},
	{NULL }
};
MenuGizmo	icon_menu = {0, "iconMenu", NULL, icon_menu_item };
PopupGizmo	icon_popup ={0,"popup",string_newiconTitle,(Gizmo)&icon_menu };

MenuItems	no_icon_menu_item[] = {  
	{ TRUE, label_ok2,	mnemonic_ok, 0, CancelNoIconCB,  NULL },
	{ NULL }
};
MenuGizmo	no_icon_menu = {0, "noIconMenu", NULL, no_icon_menu_item };
ModalGizmo	no_icon_popup = {0,"modal","",(Gizmo)&no_icon_menu };

static MenuItems choice3Items[] = {
        {True, label_self,mnemonic_self,0, SpecifyCB, (char *)False,NULL},
	{True, label_current,mnemonic_current,0,SpecifyCB,(char*)False,NULL},
	{True, label_future,mnemonic_future,0, SpecifyCB, (char*)False,NULL},
	{True, label_specific,mnemonic_specific,0,SpecifyCB,(char *)True,NULL},
	{ NULL }
};

static MenuGizmo choice3Menu = {
        0, "choice3Menu", NULL, choice3Items,
        NULL, NULL, EXC, OL_FIXEDCOLS, 1, 0
};

static Setting install;
static ChoiceGizmo choice3 = {
        0, "choice3", "", &choice3Menu, &install,
};

static StaticTextGizmo title3 = {
        NULL, "title3", string_copy_choices, NorthWestGravity
};

static GizmoRec copyArray[] = {
        {StaticTextGizmoClass,		&title3},
	{ChoiceGizmoClass,		&choice3}
};

static LabelGizmo copyTypeChoice = {
        0, "copyTypeChoice", NULL,
	copyArray, XtNumber(copyArray), OL_FIXEDCOLS,
	1, 0, 0, True
};

static ScrolledWindowGizmo innerBox3 = {
        "innerBox3", 40, 40
};

static Setting userSetting;
static ListGizmo	u_file = {
        NULL, "u_file", "", &userSetting,
	"%13s", True, 4, "fixed", True
};

static GizmoRec u_array[] = {
        {ListGizmoClass,	&u_file}
};

static LabelGizmo dtusers = {
        NULL, "dtusers", string_dtuser, u_array, XtNumber (u_array), 
	OL_FIXEDROWS, 1, NULL, 0, True
};

static GizmoRec c_array[] = {
        {LabelGizmoClass,      &dtusers},
};

static RubberTileGizmo specificChoice = {
        "specific_choices", NULL, OL_HORIZONTAL, c_array, XtNumber (c_array)
};

static GizmoRec copyfolderArray[] = {
	{ScrolledWindowGizmoClass,		&innerBox3},
	{LabelGizmoClass,			&copyTypeChoice},
	{RubberTileGizmoClass,			&specificChoice},
};

static MenuItems copyfolderItems[] = {
        {False, label_ok2, mnemonic_ok, NULL, ApplyFolderCB, (char*)True},
	{False, label_apply, mnemonic_apply, NULL, ApplyFolderCB,(char*)False},
	{True, label_reset, mnemonic_reset, NULL, ResetFolderCB},
	{True, label_cancel, mnemonic_cancel, NULL, CancelFolderCB},
	{True, label_help, mnemonic_help, NULL, helpCB, (char *)&HelpFolder},
	{NULL}
};

static MenuGizmo copyfolderMenu = {
        0, "copyfolderMenu", NULL, copyfolderItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

PopupGizmo copy_folder = {
        0, "copyfolder", string_copy_folder,
	&copyfolderMenu, copyfolderArray, XtNumber (copyfolderArray)
};

static Setting nwSetting;
static ListGizmo	nwservers = {
	NULL, "nwservers", "", &nwSetting, 
	"%21s", True, 4, "fixed", True
};

static GizmoRec nwArray[] = {
  {ListGizmoClass,	&nwservers},
};

MenuItems       nw_menu_item[] = {
  {TRUE,  label_select, mnemonic_select, 0, NwCB, NULL },
  {TRUE,  label_cancel, mnemonic_cancel, 0, NwCB, NULL },
  {TRUE,  label_help, mnemonic_help, 0, helpCB, (char *)&HelpPackage},
  {NULL }
};

MenuGizmo       nw_menu = {0, "nwMenu", NULL, nw_menu_item };
PopupGizmo	nw_servers = {0,"nw",string_server, &nw_menu,
			      nwArray, XtNumber (nwArray)
};

int		icon_count     = 0;
int		folder_count   = 0;
extern  int	folder_end     = 0;
extern  int	folder_start   = 0;
extern  Boolean	set_install     = False;

DmContainerRec	pkg_cntrec,	icon_cntrec,	folder_cntrec;

Arg	arg[12];

char  LANG_C[]  = "LANG=C LC_MESSAGES=C ";
char	buf[BUFSIZ+1] = "";
char	PKGINFO[] = "/usr/bin/pkginfo";
char	ADDPKG[]  = "/usr/sbin/pkgadd";
char	CATPKG[]  = "/usr/sbin/pkgcat";
char	PKGLIST[] = "/usr/bin/pkglist";
char	REMPKG[]  = "/usr/sbin/pkgrm";
char	INSTALL[] = "/usr/bin/installpkg";
char	CUSTM[]   = "Custom";
char	SET[]     = "set";
char 	AS[]	  = "as";
char 	PE[]	  = "pe";
char 	UNIXWARE[]  = "UnixWare";
char	PERMS[]   = "/etc/perms";	/* records custom packages */
char	APPL[]	  = "application";
char	OPTIONS[] = "/var/options";	/* possible preSVR4 packages */
char	cmdtail[32];

static void
CreateIconCursor (Widget w, XtPointer client_data, XtPointer call_data)
{
	OlFlatDragCursorCallData *	cursorData =
		(OlFlatDragCursorCallData *) call_data;
	DmGlyphPtr			glyph;
	DmObjectPtr			op;
	XColor				white;
	XColor				black;
	XColor				junk;
	static unsigned int		xHot;
	static unsigned int		yHot;
	static Cursor			cursor;

	XAllocNamedColor (
		theDisplay, DefaultColormapOfScreen (theScreen),
		"white", &white, &junk
	);
	XAllocNamedColor (
		theDisplay, DefaultColormapOfScreen (theScreen),
		"black", &black, &junk
	);

	XtSetArg (arg[0], XtNobjectData, &op);
	OlFlatGetValues (w, cursorData->item_data.item_index, arg, 1);

	glyph =  op->fcp->cursor;
	if (glyph) {
		xHot = glyph->width / 2;
		yHot = glyph->height / 2;
		cursor = XCreatePixmapCursor (
			theDisplay, glyph->pix, glyph->mask,
			&black, &white, xHot, yHot
		);
	}
	else {
		cursor = None;
		xHot = yHot = 0;
	}

	cursorData->yes_cursor = cursor;
	cursorData->x_hot = xHot;
	cursorData->y_hot = yHot;
}

/*
 * This timeout routine consumes output of pkgadd() or pkginfo().
 */

void
ReadUninstalledList(XtPointer client_data, XtIntervalId tid)
{
	static Boolean	pkgadd_flag = FALSE;
	int		n = 0, p3close_status = 0;
	int		exitCode = 0;
	char *		ptr;
	char *		line;
	ListRec *	lp = pr->mediaList;
	Boolean		set = FALSE;

	if (lp->cmdfp[1] == NULL) {		/* canceled -- restore sanity */
		pkgadd_flag = FALSE;
		*(lp->input) = '\0';
		lp->timeout = (XtIntervalId)0;
		return;
	}
	ptr = lp->input+strlen(lp->input);
	n = read(fileno(lp->cmdfp[1]), ptr, BUFSIZ-(ptr-lp->input)-nwinstall);
	switch (n) {
	case -1:/*
		 *	no current input; keep going
		 */
		break;
	case 0:	/*
		 *	end of file
		 */
		nwinstall = 0;
		p3close_status = _Dtam_p3close(lp->cmdfp, 0);
		exitCode = CheckP3closeStatus(p3close_status);
		lp->cmdfp[0] = lp->cmdfp[1] = (FILE *)NULL;
		BringDownPopup(pr->catalog->shell);
		if (exitCode != 1)
			CreateSetIcons (lp, NULL);
		*(lp->input) = '\0';
		lp->timeout = (XtIntervalId)0;
		lp->validList = (exitCode == 1) ? False : True;
		pkgadd_flag = FALSE;
		return;
	default:/*
		 *	examine each (whole) line
		 */
		ptr[n] = '\0';
		if (strstr(lp->input, "HELO") &&
		    strstr(lp->input, our_node)) {
			*lp->input = '\0';
			nwinstall = BUFSIZ;
			break;
		}

		if (strstr(lp->input, "Type [go]")) {
			fputs("go\n",lp->cmdfp[0]);
			*(lp->input) = '\0';
			break;
		}
		/*
		 * Look for the lines:
		 *     The following packages are available:
		 *       1  edebug     Enhanced Debugger
		 */
		else if (line = strstr(lp->input, "available:")) {
			set = True;
			if (strstr (lp->input, "packages are available")) {
				set = False;
			}
			pkgadd_flag = TRUE;
			strcpy(lp->input, line+11);
		}
		for (line = lp->input; *line == '\n'; line++)
			;
		for (ptr=strchr(line,'\n'); ptr; line=ptr,ptr=strchr(ptr,'\n')){
			char	*ptr2;
			while (*ptr == '\n')
				*ptr++ = '\0';
			if (*line == '\0')
				continue;
			if (pkgadd_flag && atoi(line) != 0) {
				while (isspace(*line) || isdigit(*line))
					line++;
				(void)AddUninstalledPackage (lp, line, set);
			}
			else if (strncmp(line,"Select package",14)==0) {
				fputs("q\n", lp->cmdfp[0]);
			}
			else {
				if (strncmp(line,"set",3) == 0)
					set = True;
				else
					set = False;
				(void)AddPackageFromServer (lp, line, set);
			}
			  
		}
		if (*line && line != lp->input)	/* keep partial line */
			strcpy(lp->input, line);
		else
			*lp->input = '\0';
		break;
	}
	lp->timeout = XtAddTimeOut (
		150, (XtTimerCallbackProc)ReadUninstalledList, NULL
	);
}


/*
 * Check to see if a package is installed.  For noncustom packages it
 * is a simple matter of checking the return from pkginfo().  For custom
 * packages it means making sure the file /etc/perms/<pkg_name|pkg_set>
 * exists and that all the files specified in that file for this package
 * exist (any file reported to be in /tmp is ignored).  If any file doesn't
 * exist then it is assumed the package was removed.
 */

Boolean
PkgInstalled(PkgPtr p)
{
	char	str[BUFSIZ];
	int	n;

	if (p->pkg_fmt[0] == 'C') {
		if (STRCMP_CHK_ZERO(p->pkg_cat,SET))
			sprintf(str, "/etc/perms/%s", p->pkg_name);
		else
			sprintf(str, "/etc/perms/%s", p->pkg_set);
		if (access(str,F_OK) != 0)
			return FALSE;
		else if (STRCMP_CHK_ZERO(p->pkg_cat,SET))
			return TRUE;
		else {
			FILE	*pfp;
			char	*ptr;
			Boolean	deleted = FALSE;

			sprintf(str,"grep \"^%s\" /etc/perms/%s",
					p->pkg_name, p->pkg_set);
			if ((pfp=popen(str,"r")) == NULL)
				return FALSE;
			while (fgets(str, BUFSIZ, pfp)) {
				if (!isspace(str[strlen(p->pkg_name)]))
					continue;
				ptr = strtok(str, " \t");	/* "pkg" */
				ptr = strtok(NULL," \t");	/* mode  */
				ptr = strtok(NULL," \t");	/* owner */
				ptr = strtok(NULL," \t");	/* ????  */
				ptr = strtok(NULL," \t");	/* path  */
				/*
				 * Ignore the file if it is in /tmp.
				 * These files will be removed after a reboot.
				 */
				if (ptr != NULL &&
				    strncmp(ptr, "/tmp", sizeof("/tmp")-1) == 0 ||
				    strncmp(ptr, "./tmp", sizeof("./tmp")-1) == 0) {
					continue;
				}
				if (access(ptr,F_OK) != 0) {
					deleted = TRUE;
					break;
				}
			}
			pclose(pfp);
			return !deleted;
		}
	}
	else {
		sprintf(str, "%s -q %s 2>/dev/null", PKGINFO, p->pkg_name);
		FPRINTF ((stderr, "%s\n", str));
		n = system(str);
		return n==0;
	}
}

PkgPtr
GetSelectedSet (ListRec *lp)
{
	int		count;
	DmObjectPtr	optr = (DmObjectPtr)NULL;

	if (lp == (ListRec *)0 || lp->count == 0) {
		return (PkgPtr)NULL;
	}
	for (count = 0; count < lp->setCount; count++) {
		if (lp->setitp[count].managed && lp->setitp[count].select) {
			optr = (DmObjectPtr)lp->setitp[count].object_ptr;
			break;
		}
	}
	return (optr == NULL? (PkgPtr)NULL: (PkgPtr)(optr->objectdata));
}

/*
 *	this needs to be replaced by a multi-select
 */
PkgPtr
GetSelectedPkg ()
{
	int		count;
	DmObjectPtr	optr = (DmObjectPtr)NULL;

	for (count = 0; count < pr->pkgCount; count++) {
		if (pr->pkgitp[count].managed && pr->pkgitp[count].select) {
			optr = (DmObjectPtr)pr->pkgitp[count].object_ptr;
			break;
		}
	}
	return (optr == NULL? (PkgPtr)NULL: (PkgPtr)(optr->objectdata));
}

static char *
ToolboxName(char *icondef)
{
	static char	str[PATH_MAX];
	static char *	apptoolbox = NULL;
	char *		ptr;
	char *		ptr2;

      	if (apptoolbox == NULL) {
      		apptoolbox = folder_apps;
      		ptr = apptoolbox;
      		while (*ptr && *ptr != '\001') ptr++;
      		if (*ptr == '\001') apptoolbox = ptr+1;
      	}
	ptr = STRDUP(icondef);
	if (ptr2 = strchr(ptr,'\t')) {
		*ptr2 = '\0';
		if (strchr(ptr,'/'))
			strcpy(str, ptr);
		else if (ptr[0]=='.' && ptr[1]=='/')
			strcpy(str, ptr+2);
		else
			sprintf(str, "%s/%s", apptoolbox, ptr);
	}
	else 
	        sprintf(str, "%s/%s", apptoolbox, basename(ptr));

	FREE(ptr);
	return str;
}

/*
 *  Get path of executable to be used to create a link into the desktop
 *  user's home directory.
 */
static char *
ExpFileName(icon_obj *iobj)
{
	static char *	ptr;

	if (ptr=strstr(iobj->def,"$XWINHOME"))
		ptr = GetXWINHome(ptr+10);
	else if (ptr = strrchr(iobj->def, '='))
		ptr += 1;
	else
		ptr = iobj->def;
	return STRDUP(ptr);
}

/*
 * Read icon definition file.
 */

int
DTMInstall(icon_obj *iobj)
{
	Boolean		is_dir = FALSE;

	if (iobj->pkg) {
		is_dir=(strcmp(iobj->def+strlen(iobj->def)-4, "\tDIR") == 0);
	}
	return is_dir;

}

/*  
 *  Link executable to a specific user's $HOME/Applications directory or
 *  to a directory specified by the package.
 *  i.e  /home/build/Applications/Debug -->  /usr/ccs/bin/debug
 */
static void 
CopyOne(icon_obj *iobj, char *home_dir, char *login, Boolean is_dir)
{

        char		*name;
	char		str[PATH_MAX];
	int		result;

	name = ToolboxName(iobj->def);
	sprintf(str, "%s/%s", home_dir, Dm_DayOneName(name, login));

	if (access(str,F_OK) != 0) {

		result = is_dir? mkdir(str, 0777):
				 symlink(ExpFileName(iobj), str);

		if (result == 0)
		        sprintf(buf, GGT(format_instgood), name);
		else
		        sprintf(buf, GGT(format_instbad), name);
		XBell(theDisplay, 100);
		SetPopupMessage(pr->iconPopup, buf);
	}
}

/*
 *  Copy icon to the systems' $(XWINHOME)/desktop/Applications directory.
 *  This means new desktop users added in the future will get this
 *  icon installed in their directory automatically.
 */
static void
CopyFuture(icon_obj *iobj, Boolean is_dir)
{
	char		str[PATH_MAX];
	int		result;
	char		*name;

	name = ToolboxName(iobj->def);
	if (owner) {
		sprintf(str, "%s/%s", GetXWINHome("desktop"), name);
		if (access(str,F_OK)) {
			result = is_dir? mkdir(str, 077):
					 symlink(ExpFileName(iobj), str);
		}
	}
}

static void
SetFolderSensitivity (Boolean selected)
{
	Widget	menu;

        menu = (Widget)QueryGizmo (PopupGizmoClass, 
		       pr->copyFolder, GetGizmoWidget, "copyfolderMenu"
	);

	XtSetArg (arg[0], XtNsensitive, selected);
	OlFlatSetValues(menu, 0, arg, 1);
	OlFlatSetValues(menu, 1, arg, 1);

}

/*
 *  InstallIconCB is used to popup the "Copy to Folders" window.  Since
 *  this routine will only be used to install icons for packages that have 
 *  already been installed on the system, the scrolling window of icons
 *  (pr->foldersw) will be unmanaged.
 */
static void
InstallIconCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	MenuGizmo * f_menu;

	if (XtIsManaged(pr->foldersw)==True)
	        XtUnmanageChild(pr->foldersw);
	if (read_users == True || 
	   ((read_users = MakeFolderLists()) == True)) {
	        ResetFolderCB(wid, client_data, call_data);
		sprintf(buf, "%s", GGT(string_copy_folder));
		XtVaSetValues (pr->copyFolder->shell, 
			       XtNtitle, buf,
			       (String) 0
		);
		isfolder = False;
		SetFolderSensitivity (True);
	        MapGizmo(PopupGizmoClass, pr->copyFolder);
		f_menu = QueryGizmo (PopupGizmoClass, pr->copyFolder,
				     GetGizmoGizmo, "copyfolderMenu");
		OlSetInputFocus(f_menu->parent, RevertToNone, CurrentTime);
	}
}

static void
EndDropCB(Widget w, XtPointer files, XtPointer call_data)
{
	char **	path;
	char	str[BUFSIZ];
	Boolean	done;

	for (path = (char **)files; *path; path++) {
		if (access(*path, F_OK)==0 && strncmp(*path,"/tmp/",5)==0) {
			unlink(*path);
			done = FALSE;
		}
		else
			done = TRUE;

		if (done)
		        sprintf(str, GGT(format_instgood), *path+5);
		else
		        sprintf(str, GGT(format_instbad), *path+5);
		FREE(*path);
	}
	FREE(files);
}

static Boolean
Remix (icon_obj *iobj, char ***files, char *name, int ct)
{
	char *		src;
	char		str[PATH_MAX];
	Boolean		is_dir = FALSE;
	static Boolean	se = False;

	is_dir = (strcmp(iobj->def+strlen(iobj->def)-4, "\tDIR") == 0);
	if (is_dir) {
		src = STRDUP(iobj->def);
		src[strlen(iobj->def)-4] = '\0'; /* Remove \tDIR */
		sprintf (str, "desktop/%s", src);
		FREE(src);
		src = STRDUP(GetXWINHome(str));
	}
	else {
		src = ExpFileName(iobj);
	}
	if (iobj->pkg) {
		sprintf(str, "/tmp/%s", name);
		(*files)[ct-1] = STRDUP(str);
		symlink(src, (*files)[ct-1]);
		RegisterPkg(pr, iobj->pkg, TRUE);
	}
	else {
		se = TRUE;
		(*files)[ct-1] = STRDUP(iobj->def);
	}
	FREE(src);
	(*files)[ct] = NULL;

	return se;
}

static void
DropIconCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	char **			files = (char **)NULL;
	OlFlatDropCallData *	drop = (OlFlatDropCallData *)call_data;
	DmObjectPtr		op;
	icon_obj *		iobj;
	Boolean			chosen;
	Boolean			simple_exec = FALSE;
	int			ct;
	int			n;

	n = drop->item_data.item_index;
	XtSetArg(arg[0], XtNset, &chosen);
	OlFlatGetValues(pr->iconBox, n, arg, 1);
	if (!chosen) {
		ct = 1;
		XtSetArg(arg[0], XtNobjectData, &op);
		OlFlatGetValues(pr->iconBox, n, arg, 1);
		iobj = (icon_obj *)op->objectdata;
		files = (char **)MALLOC(2*sizeof(char *));
		simple_exec = Remix (iobj, &files, op->name, 1);
	}
	else for (ct = n = 0; n < icon_count; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
		OlFlatGetValues(pr->iconBox, n, arg, 1);
		if (chosen) {
			++ct;
			XtSetArg(arg[0], XtNobjectData, &op);
			OlFlatGetValues(pr->iconBox, n, arg, 1);
			iobj = (icon_obj *)op->objectdata;
			if (!files)
				files = (char **)MALLOC(2*sizeof(char *));
			else
				files = (char **)REALLOC(files,
							(ct+1)*sizeof(char *));
			simple_exec = Remix (iobj, &files, op->name, ct);
		}
	}
	if (ct)
		DtNewDnDTransaction(w, files, DT_B_STATIC_LIST,
					drop->root_info->root_x,
					drop->root_info->root_y,
					drop->ve->xevent->xbutton.time,
					drop->dst_info->window,
					simple_exec? DT_LINK_OP: DT_MOVE_OP,
					NULL,
					EndDropCB,
					(XtPointer)files);
}

/*
 * Create the application name window or icon sheet.  This is the window
 * that contains all of the icons, from a package, that can be install
 * to the desktop.
 */

void
CreateIconSheet ()
{
	Widget		w_up;

	pr->iconPopup = CopyGizmo (PopupGizmoClass, &icon_popup);
	CreateGizmo(pr->base->shell, PopupGizmoClass, pr->iconPopup, NULL, 0);

	pr->icon = GetPopupGizmoShell(pr->iconPopup);
	XtSetArg(arg[0], XtNupperControlArea, &w_up);
	XtGetValues(pr->icon, arg, 1);

	XtSetArg(arg[0], XtNalignCaptions,	(XtArgVal)FALSE);
	XtSetArg(arg[1], XtNcenter,		(XtArgVal)FALSE);
	XtSetArg(arg[2], XtNvPad,		(XtArgVal)(y3mm));
	XtSetArg(arg[3], XtNvSpace,		(XtArgVal)(y3mm));
	XtSetArg(arg[4], XtNhPad,		(XtArgVal)(x3mm));
	XtSetArg(arg[5], XtNhSpace,		(XtArgVal)(x3mm));
	XtSetValues(w_up, arg, 6);

	XtSetArg(arg[0], XtNposition,	(XtArgVal)OL_TOP);
	XtSetArg(arg[1], XtNalignment,	(XtArgVal)OL_LEFT);
	XtSetArg(arg[2], XtNspace,	(XtArgVal)6);
	XtSetArg(arg[3], XtNlabel,	(XtArgVal)GGT(info_icons));
	pr->type = XtCreateManagedWidget("caption", captionWidgetClass,
			w_up, arg, 4);

	XtSetArg(arg[0], XtNheight,	(XtArgVal)(4*HEIGHT/5));
	XtSetArg(arg[1], XtNwidth,	(XtArgVal)WIDTH);
	XtSetArg(arg[2], XtNhStepSize,  (XtArgVal)(INC_X/2));
	XtSetArg(arg[3], XtNvStepSize,  (XtArgVal)(INC_Y/2));
	pr->iconsw = XtCreateManagedWidget("package_window",
				scrolledWindowWidgetClass, pr->type, arg, 4);
}

/*
 * Create notice telling user that there are no programs for this package.
 */
void
CreateNoIconSheet ()
{
	pr->noIconPopup = CopyGizmo (ModalGizmoClass, &no_icon_popup);
	CreateGizmo (pr->base->shell, ModalGizmoClass, pr->noIconPopup, 0, 0);
}

static DmObjectPtr
AddInstallable (char *path, char *iname, DmFclassPtr fcp)
{
	DmObjectPtr	op;
	DmObjectPtr	endp;
	DmObjectPtr	prevp;
	icon_obj *	iobj;
	char *		ptr;
	char *		ptr2;

	if (icon_count < icon_cntrec.num_objs) {
		FREE((char *)pr->iconitp[icon_count].label);
		pr->iconitp[icon_count].label = (XtArgVal)STRDUP(iname);
		pr->iconitp[icon_count].select = FALSE;
		pr->iconitp[icon_count].managed = TRUE;
		op = (DmObjectPtr)pr->iconitp[icon_count++].object_ptr;
		if (op->name)
			FREE(op->name);
		op->name = iname;
		op->fcp = fcp;
		iobj = (icon_obj *)op->objectdata;
		if (iobj->pkg) {
			FREE(iobj->pkg);
			iobj->pkg = NULL;
		}
		if (iobj->def)
			FREE(iobj->def);
		if (icon_count == 1)
			icon_cntrec.op = op;
	}
	else {
		iobj = (icon_obj *)CALLOC(1, sizeof(icon_obj));
		op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
		op->name = iname;
		op->container = &icon_cntrec;
		op->objectdata = iobj;
		op->fcp = fcp;
		op->x = pr->iconx;
		op->y = pr->icony;
		if ((int)(pr->iconx += 4*INC_X/3) > WIDTH - MARGIN) {
			pr->iconx = INIT_X;
			pr->icony += INC_Y;
		}
		if (pr->iconBox) {
		/*
		 *	add new object to existing iconbox
		 *	(this use of Dm__AddToIcontainer is borrowed
		 *	from a similar context in nfs/container.c)
		*/
			int	nitems;
			XtSetArg(arg[0], XtNnumItems, &nitems);
			XtGetValues(pr->iconBox, arg, 1);
			icon_count++;
			Dm__AddObjToIcontainer (
				pr->iconBox, &pr->iconitp, (Cardinal *)&nitems,
				&icon_cntrec, op,
				pr->iconx, pr->icony,
				DM_B_CALC_SIZE | DM_B_NO_INIT,
				NULL, def_font, (Dimension)WIDTH, 0, 0
			);
		}
		else if (icon_count++ == 0)
			icon_cntrec.op = op;
		else {
		        prevp = NULL;
			endp = icon_cntrec.op;
		        while (strcmp(endp->name, op->name) < 0)  {
			        prevp = endp;
				if (endp->next == NULL)
				        break;
				endp = endp->next;
			}

			if (prevp == NULL)  {
			        icon_cntrec.op = op;
				op->next = endp;
			}
			else  {
			        op->next = prevp->next;
				prevp->next = op;
			}
		}
	}
	iobj->def = STRDUP(path);
	return op;
}

static DmObjectPtr
AddFolderIcon (char *path, char *iname, DmFclassPtr fcp)
{
	DmObjectPtr	op;
	DmObjectPtr	endp;
	DmObjectPtr	prevp;
	icon_obj *	iobj;
	char *		ptr;
	char *		ptr2;

	if (folder_count < folder_cntrec.num_objs) {
		FREE((char *)pr->folderitp[folder_count].label);
		pr->folderitp[folder_count].label = (XtArgVal)STRDUP(iname);
		pr->folderitp[folder_count].select = FALSE;
		pr->folderitp[folder_count].managed = TRUE;
		op = (DmObjectPtr)pr->folderitp[folder_count++].object_ptr;
		if (op->name)
			FREE(op->name);
		op->name = iname;
		op->fcp = fcp;
		iobj = (icon_obj *)op->objectdata;
		if (iobj->pkg) {
			FREE(iobj->pkg);
			iobj->pkg = NULL;
		}
		if (iobj->def)
			FREE(iobj->def);
		if (folder_count == 1)
			folder_cntrec.op = op;
	}
	else {
		iobj = (icon_obj *)CALLOC(1, sizeof(icon_obj));
		op = (DmObjectPtr)CALLOC(1, sizeof(DmObjectRec));
		op->name = iname;
		op->container = &folder_cntrec;
		op->objectdata = iobj;
		op->fcp = fcp;
		op->x = pr->iconx;
		op->y = pr->icony;
		if ((int)(pr->iconx += 4*INC_X/3) > WIDTH - MARGIN) {
			pr->iconx = INIT_X;
			pr->icony += INC_Y;
		}
		if (pr->folderBox) {
		/*
		 *	add new object to existing iconbox
		 *	(this use of Dm__AddToIcontainer is borrowed
		 *	from a similar context in nfs/container.c)
		*/
			int	nitems;
			XtSetArg(arg[0], XtNnumItems, &nitems);
			XtGetValues(pr->folderBox, arg, 1);
			folder_count++;
			Dm__AddObjToIcontainer (
				pr->folderBox, &pr->folderitp, 
				(Cardinal *)&nitems, &folder_cntrec, op,
				pr->iconx, pr->icony,
				DM_B_CALC_SIZE | DM_B_NO_INIT,
				NULL, def_font, (Dimension)WIDTH, 0, 0
			);
		}
		else if (folder_count++ == 0)
			folder_cntrec.op = op;
		else {
		        prevp = NULL;
			endp = folder_cntrec.op;
		        while (strcmp(endp->name, op->name) < 0)  {
			        prevp = endp;
				if (endp->next == NULL)
				        break;
				endp = endp->next;
			}
			if (prevp == NULL)  {
			        folder_cntrec.op = op;
				op->next = endp;
			}
			else  {
			        op->next = prevp->next;
				prevp->next = op;
			}
		}
	}
	iobj->def = STRDUP(path);
	return op;
}

static void
AddToExecs(char *name, FILE *dfp, char *format, Boolean folder)
{
	DmObjectPtr	op;
	char *		ptr;

	while (fgets(buf, BUFSIZ, dfp)) {
		buf[strlen(buf)-1] = '\0';
		ptr = FilterExecutable(name, buf, format);
		if (ptr) {
		        if (folder == True)
			        op = AddFolderIcon(ptr,
					    STRDUP(1+strrchr(ptr,'/')),
					    &exec_fcrec);
			else
			        op = AddInstallable(ptr,
					    STRDUP(1+strrchr(ptr,'/')),
					    &exec_fcrec);
		}
	}
	fclose(dfp);
}

static void
SetIconSensitivity (Boolean selected)
{
	Widget	menu;

        menu = (Widget)QueryGizmo (PopupGizmoClass, 
		       pr->iconPopup, GetGizmoWidget, "iconMenu"
	);

	if (pr->folderBoxUp == True) {
	        XtVaSetValues (pr->iconBox, 
			       XtNdblSelectProc, NULL,
			       0
		);
	        XtSetArg (arg[0], XtNsensitive, False);	  
		sprintf(buf, "%s", GGT(string_modal_msg));
		SetPopupMessage(pr->iconPopup, buf);
	}
	else {
	        XtVaSetValues (pr->iconBox, 
			       XtNdblSelectProc,InstallIconCB,
			       0
		);
	        XtSetArg (arg[0], XtNsensitive, selected);
	}
	OlFlatSetValues(menu, 0, arg, 1);
}

static void
SelectIconCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	icon_obj *		iobj;
	char *			name;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	iobj = (icon_obj *)op->objectdata;
	name = iobj->pkg? ToolboxName(iobj->def): iobj->def;
	SetPopupMessage(pr->iconPopup, Dm_DayOneName(name, our_login));
	SetIconSensitivity (True);
}

static void
SelectFolderCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	icon_obj *		iobj;
	char *			name;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	iobj = (icon_obj *)op->objectdata;
	name = iobj->pkg? ToolboxName(iobj->def): iobj->def;
	SetPopupMessage(pr->iconPopup, name);
	SetFolderSensitivity (True);
}

static void
AddToIcons (char *name, FILE *dfp, Boolean folder)
{
	DmObjectPtr	op;
	DmFclassPtr	fcp;
	icon_obj *	iobj;
	char *		ptr;
	char *		ptr2;
	char *		iname;
	char *		iconFile;
	char *		ptr3;

	while (fgets(buf, BUFSIZ, dfp)) {
		buf[strlen(buf)-1] = '\0';
		ptr = FilterIcon(buf);
		if (ptr) {
			*ptr++ = '\0';		/* isolate the ICON= field */
		/*
		 *	get the glyph and cursor for it:
	 	 */
			iconFile = STRDUP(buf+5);
			if (ptr3 = strrchr(iconFile, '/')) {
				iconFile = ptr3+1;
			}
			fcp = (DmFclassPtr)CALLOC(1, sizeof(DmFclassRec));
			if (fcp) {
				fcp->glyph = DmGetPixmap(theScreen, buf+5);
				fcp->cursor = DmGetCursor(theScreen, iconFile);
			}
			else
				fcp = &exec_fcrec;
		/*
	 	*	get the label for the glyph
	 	*/
			if (ptr2 = strchr(ptr, '\t')) {
				*ptr2 = '\0';
				iname = STRDUP(basename(DtamGetTxt(ptr)));
				*ptr2 = '\t';
			}
			else	/* shouldn't happen! */
				iname = STRDUP(GGT (string_huh));
			if (folder == True)
			        op = AddFolderIcon(ptr, iname, fcp);
			else
			        op = AddInstallable(ptr, iname, fcp);
			iobj = (icon_obj *)op->objectdata;
			iobj->pkg = STRDUP(name);
			RegisterPkg(pr, iobj->pkg, TRUE);
		}
	}
	fclose(dfp);
}

void
SetPkgDefs (PkgPtr p)
{
	FILE *	icondef;
	char *	ptr;

	if (p->pkg_class)
		FREE(p->pkg_class);
	p->pkg_class = NULL;
	if (p->pkg_help)
		FREE(p->pkg_help);
	p->pkg_help  = NULL;
	sprintf(buf, "%s/%s", GetXWINHome(ICONDIR), p->pkg_name);
	if ((icondef=fopen(buf,"r")) == NULL)
		return;
	while(fgets(buf, BUFSIZ, icondef)) {	
		if (strncmp(buf,"HELP=",5)==0) {
			if (p->pkg_help == NULL)
				p->pkg_help = STRDUP(buf+5);
			else {
				p->pkg_help = (char *)REALLOC(p->pkg_help,
							strlen(p->pkg_help)+
								strlen(buf)-4);
				strcat(p->pkg_help,buf+5);
			}
		}
		else if (strncmp(buf,"CLASS=",6)==0) {
			buf[strlen(buf)-1] = '\0';
			if (p->pkg_class == NULL)
				p->pkg_class = STRDUP(buf+6);
			else {
				p->pkg_class = (char *)REALLOC(p->pkg_class,
							strlen(p->pkg_class)+
								strlen(buf)-4);
				strcat(strcat(p->pkg_class," "),buf+6);
			}
		}
	}
	fclose(icondef);
}

static void
alignIcons()		/* borrowed with minimal adaption from nfs code */
{
	DmItemPtr	item;
	Dimension	width;
	Cardinal	nitems;
	Position	icon_x;
	Position	icon_y;

	XtSetArg(arg[0], XtNnumItems, &nitems);
	XtSetArg(arg[1], XtNwidth, &width);
	XtGetValues(pr->iconBox, arg, 2);
	width -= 20;		/* FIX: shouldn't be needed:  iconbox bug */
    
	for (item = pr->iconitp; item < pr->iconitp + nitems; item++)
		item->x = item->y = UNSPECIFIED_POS;
    
	for (item = pr->iconitp; item < pr->iconitp + nitems; item++) {
        	if (ITEM_MANAGED(item)) {
			DmGetAvailIconPos(pr->iconitp, nitems,
				ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
				4*(Position)INC_X/3, INC_Y,
				&icon_x, &icon_y );
			item->x = (XtArgVal)icon_x;
			item->y = (XtArgVal)icon_y;
		}
	}
}

static void
alignFolder()		/* borrowed with minimal adaption from nfs code */
{
	DmItemPtr	item;
	Dimension	width;
	Cardinal	nitems;
	Position	icon_x;
	Position	icon_y;

	XtSetArg(arg[0], XtNnumItems, &nitems);
	XtSetArg(arg[1], XtNwidth, &width);
	XtGetValues(pr->folderBox, arg, 2);
	width -= 20;		/* FIX: shouldn't be needed:  iconbox bug */
    
	for (item = pr->folderitp; item < pr->folderitp + nitems; item++)
		item->x = item->y = UNSPECIFIED_POS;
    
	for (item = pr->folderitp; item < pr->folderitp + nitems; item++) {
        	if (ITEM_MANAGED(item)) {
			DmGetAvailIconPos(pr->folderitp, nitems,
				ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
				4*(Position)INC_X/3, INC_Y,
				&icon_x, &icon_y );
			item->x = (XtArgVal)icon_x;
			item->y = (XtArgVal)icon_y;
		}
	}
}

void
SetBusy(Boolean waiting)
{
	Widget		w_menu = NULL;

	FPRINTF ((stderr, "SetBusy\n"));
	if (waiting) {
		XtSetArg(arg[0], XtNsensitive, !waiting);
		if (pr->pkgPopup->shell) {
			w_menu = (Widget)QueryGizmo (
				PopupGizmoClass, pr->pkgPopup,
				GetGizmoWidget, "pkgMenu"
			);
			OlFlatSetValues(w_menu, 2, arg, 1);
		}
	}
	else {
		SetPkgSensitivity ();
	}
}

static void
AdjustIconCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	icon_obj *		iobj;
	int			item = d->item_index;
	char *			name;
	int			count;
	Boolean			selected;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	iobj = (icon_obj *)op->objectdata;
	if (iobj->pkg) {
		name = ToolboxName(iobj->def);
		FPRINTF ((stderr, "Name= %s\n", name));
	}
	else {
		name = iobj->def;
	}

	if (pr->iconitp[item].managed && pr->iconitp[item].select) {
	        SetPopupMessage(pr->iconPopup, Dm_DayOneName(name, our_login));
	}
	else {
		SetPopupMessage(pr->iconPopup, NULL);
	}

	selected = False;
	if (pr->folderBoxUp == True)
	        SetIconSensitivity (selected);
	else  {
	        for (count = 0; count < icon_count; count++) {
		        if (pr->iconitp[count].managed && 
			    pr->iconitp[count].select) {
			        selected = True;
				break;
		        }
		}
		SetIconSensitivity (selected);
        }
}

static void
AdjustFolderCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	DmObjectPtr		op;
	OlFlatCallData *	d = (OlFlatCallData *)call_data;
	icon_obj *		iobj;
	int			item = d->item_index;
	char *			name;
	int			count;
	Boolean			selected;

	XtSetArg(arg[0], XtNobjectData, &op);
	OlFlatGetValues(w, d->item_index, arg, 1);
	iobj = (icon_obj *)op->objectdata;
	if (iobj->pkg) {
		name = ToolboxName(iobj->def);
		FPRINTF ((stderr, "Name= %s\n", name));
	}
	else {
		name = iobj->def;
	}

	if (pr->folderitp[item].managed && pr->folderitp[item].select) {
		SetPopupMessage(pr->copyFolder, name);
	}
	else {
		SetPopupMessage(pr->copyFolder, NULL);
	}
	selected = False;
        for (count = 0; count < folder_count; count++) {
                if (pr->folderitp[count].managed && 
		    pr->folderitp[count].select) {
		        selected = True;
                        break;
		      }
        }
	SetFolderSensitivity (selected);
}

int
cmpdmicon(DmItemPtr x, DmItemPtr y)
{
        return strcoll((char*)x->label, (char*)y->label);

}

/*
 * Create the window of user icons (defined or implicit)
 */
static int
GetIconView(PkgPtr p)
{
	int	i = 0;
	int	n;
	int	nitems;

	FILE *	idata;

	sprintf(buf, GGT(string_newpkgTitle), p->pkg_name);
	XtSetArg(arg[0], XtNtitle, buf);
        XtSetValues(pr->iconPopup->shell, arg, 1);
	icon_count = 0;
	pr->iconx = INIT_X;
	pr->icony = INIT_Y;

	if (p->pkg_class == NULL && p->pkg_help == NULL)
		SetPkgDefs(p);

	sprintf(buf, "%s/%s", ICONDIR, p->pkg_name);
	if (idata=fopen(GetXWINHome(buf),"r"))
		AddToIcons(p->pkg_name, idata, False);

	if (icon_count > 0) {
		XtSetArg(arg[0], XtNlabel, GGT(info_icons));
		XtSetValues(pr->type, arg, 1);
	}
	else {
		XtSetArg(arg[0], XtNlabel, "" /* GGT(info_execs) */);
		XtSetValues(pr->type, arg, 1);
		if (p->pkg_fmt[0] == 'C') {
			sprintf(buf, "%s/%s", PERMS,
					p->pkg_set? p->pkg_set: p->pkg_name);
			if (idata=fopen(buf,"r"))
				AddToExecs(p->pkg_name, idata, "C", False);
		}
		else if (idata=fopen(LOGFILE,"r")) {
			AddToExecs(p->pkg_name, idata, "4.0", False);
		}
		if (idata == NULL)
			FooterMsg2 (GGT(string_badLog));
	}
	if (icon_count == 0) {
		return 0;
	}

	if (pr->iconitp) {
	        qsort((void *)pr->iconitp, icon_count, sizeof(DmItemRec), 
		      (int (*)())cmpdmicon);
	}

	SetBusy(TRUE);
	if (pr->iconBox) {
	        XtSetArg(arg[0], XtNnumItems, &nitems);
		XtGetValues(pr->iconBox, arg, 1);
		for (n = icon_count; n < nitems; n++) {
		       pr->iconitp[n].managed = FALSE;
		}
	}
	else {
		icon_cntrec.num_objs = icon_count;
		XtSetArg(arg[i],XtNpostSelectProc,(XtArgVal)SelectIconCB); i++;
		XtSetArg(arg[i],XtNpostAdjustProc,(XtArgVal)AdjustIconCB); i++;
		XtSetArg(arg[i],XtNmovableIcons,(XtArgVal)FALSE); i++;
		XtSetArg(arg[i],XtNminWidth,	(XtArgVal)1); i++;
		XtSetArg(arg[i],XtNminHeight,	(XtArgVal)1); i++;
		XtSetArg(arg[i],XtNdrawProc,	(XtArgVal)DmDrawIcon); i++;
		XtSetArg(arg[i],XtNdropProc,	(XtArgVal)DropIconCB); i++;
		XtSetArg(arg[i],XtNdragCursorProc,(XtArgVal)CreateIconCursor); i++;
	        XtSetArg(arg[i],XtNdblSelectProc,(XtArgVal)InstallIconCB); i++;

	        pr->iconBox = DmCreateIconContainer (
		              pr->iconsw, DM_B_CALC_SIZE, arg, i,
			      icon_cntrec.op, icon_count, &pr->iconitp,
		              icon_count, NULL, NULL, def_font, 1
		);
		nitems = icon_count;

	}
	/* Select all icons by default */
	XtSetArg(arg[0], XtNset, True);
	for (n=0; n < icon_count; n++) 
	        OlFlatSetValues(pr->iconBox, n, arg, 1);

	alignIcons();
	XtSetArg(arg[0], XtNitemsTouched, TRUE);
	XtSetValues(pr->iconBox, arg, 1);
	SetIconSensitivity (True);
	SetBusy(FALSE);
	return icon_count;
}

/*
 * Create the window of user icons (defined or implicit)
 */
static int
GetFolderView(PkgPtr p)
{
	int	i = 0;
	int	n;
	FILE *	idata;
	int	nitems;

	sprintf(buf, GGT(string_newpkgTitle), p->pkg_name);
	XtSetArg(arg[0], XtNtitle, buf);
        XtSetValues(pr->copyFolder->shell, arg, 1);
	folder_count = 0;
	pr->iconx = INIT_X;
	pr->icony = INIT_Y;

	if (p->pkg_class == NULL && p->pkg_help == NULL)
		SetPkgDefs(p);

	sprintf(buf, "%s/%s", ICONDIR, p->pkg_name);
	if (idata=fopen(GetXWINHome(buf),"r"))
		AddToIcons(p->pkg_name, idata, True);

	if (folder_count > 0) {
		XtSetArg(arg[0], XtNlabel, GGT(info_icons));
		XtSetValues(pr->type, arg, 1);
	}
	else {
		XtSetArg(arg[0], XtNlabel, "" /* GGT(info_execs) */);
		XtSetValues(pr->type, arg, 1);
		if (p->pkg_fmt[0] == 'C') {
			sprintf(buf, "%s/%s", PERMS,
					p->pkg_set? p->pkg_set: p->pkg_name);
			if (idata=fopen(buf,"r"))
				AddToExecs(p->pkg_name, idata, "C", True);
		}
	}
	if (folder_count == 0) {
		return 0;
	}

	if (pr->folderitp) {
	        qsort((void *)pr->folderitp, folder_count, sizeof(DmItemRec), 
		      (int (*)())cmpdmicon);
	}

	SetBusy(TRUE);

	if (pr->folderBox) {
	        XtSetArg(arg[0], XtNnumItems, &nitems);
		XtGetValues(pr->folderBox, arg, 1);
		for (n = folder_count; n < nitems; n++) {
		        pr->folderitp[n].managed = FALSE;
		}
	}
	else {
		folder_cntrec.num_objs = folder_count;
		XtSetArg(arg[i],XtNpostSelectProc,(XtArgVal)SelectFolderCB); i++;
		XtSetArg(arg[i],XtNpostAdjustProc,(XtArgVal)AdjustFolderCB); i++;
		XtSetArg(arg[i],XtNmovableIcons,(XtArgVal)FALSE); i++;
		XtSetArg(arg[i],XtNminWidth,	(XtArgVal)1); i++;
		XtSetArg(arg[i],XtNminHeight,	(XtArgVal)1); i++;
		XtSetArg(arg[i],XtNdrawProc,	(XtArgVal)DmDrawIcon); i++;
		XtSetArg(arg[i],XtNdragCursorProc,(XtArgVal)CreateIconCursor); i++;
	        pr->folderBox = DmCreateIconContainer (
		                pr->foldersw, DM_B_CALC_SIZE, arg, i,
			        folder_cntrec.op, folder_count,&pr->folderitp,
		                folder_count, NULL, NULL, def_font, 1
		);
		nitems = folder_count;
	}

	/* Select all icons by default */
	XtSetArg(arg[0], XtNset, True);
	for (n=0; n < folder_count; n++)
	        OlFlatSetValues(pr->folderBox, n, arg, 1);

	alignFolder();
	XtSetArg(arg[0], XtNitemsTouched, TRUE);
	XtSetValues(pr->folderBox, arg, 1);
	SetFolderSensitivity (True);
	SetBusy(FALSE);
	return folder_count;
}

static void
CancelNoIconCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup (GetModalGizmoShell (pr->noIconPopup));
}

static void
CancelIconCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	BringDownPopup(pr->icon);
	pr->iconBoxUp = False;
}

/*
 *  This routine is called when a user chooses to install the icon(s) for
 *  a package to the system.  The routine determines which user directories
 *  the icon is being installed in by using a combination of the choices 
 *  gizmo and the scrolling list of the system's desktop users. 
 */
static void
ApplyFolderCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        int i;
	DmObjectPtr	op;
	Boolean		chosen;
	int		n;
        int 		cnt;
	int		is_dir;
	Setting 	*setting;
	Boolean 	erase = (Boolean) client_data;
	LabelGizmo	*tmpg;
	int		counter = isfolder ? folder_count : icon_count;
	Widget		box = isfolder ? pr->folderBox : pr->iconBox;
	
	ManipulateGizmo(PopupGizmoClass,
		        pr->copyFolder, GetGizmoValue);
        tmpg = (LabelGizmo*)QueryGizmo (PopupGizmoClass, pr->copyFolder,
		GetGizmoGizmo, "copyTypeChoice");

	setting = (Setting *) QueryGizmo (tmpg->gizmos[1].gizmo_class,
		tmpg->gizmos[1].gizmo, GetGizmoSetting, "choice3");

        for (cnt = n = 0; n < counter; n++) {
		XtSetArg(arg[0], XtNset, &chosen);
	        OlFlatGetValues(box, n, arg, 1);
		if (chosen) {
			cnt++;
			XtSetArg(arg[0], XtNobjectData, &op);
			OlFlatGetValues(box, n, arg, 1);
			is_dir=DTMInstall((icon_obj *)op->objectdata);

		        switch ((int)setting->current_value) {

			case 0:  /* Self */
		             CopyOne((icon_obj*)op->objectdata,
				     pr->deskTopDir,
				     our_login, 
				     is_dir
			     );

			     break;
			case 2:   /* Future */
			     CopyFuture((icon_obj *)op->objectdata, is_dir);

			case 1:  /* Current */
			     for (i=0; i<u_cnt; i++) {
			             CopyOne((icon_obj*)op->objectdata,
					     u_list[i].pw_dir, 
					     u_list[i].pw_name,
					     is_dir
				     );
			     }
			     break;

			case 3:  /* Specific */
			     for (i=0; i<u_cnt; i++) {
			             if (UserItems[i].set == True) {
			                  CopyOne((icon_obj*)op->objectdata,
					          u_list[i].pw_dir,
					          u_list[i].pw_name,
					          is_dir
				          );
			             }
			     }
			     break;
			}

	        }
	}
	if (cnt == 0) {
	        XBell(theDisplay, 100);
	        SetPopupMessage(pr->copyFolder,GGT(string_select)); 
	}

	if (erase == True && cnt != 0) {
	        CancelFolderCB(wid, client_data, call_data);
	}
}

static void
ResetFolderCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        int i;

        for (i=0; i<u_cnt; i++) {
	       UserItems[i].set = False;
	}

        ManipulateGizmo (PopupGizmoClass, pr->copyFolder,
			 ResetGizmoValue
	);
	SpecifyCB(wid, False, call_data);
}

static void
CancelFolderCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
       pr->folderBoxUp = False;
       BringDownPopup(pr->copy);
}

/* 
 *  This routine is called whenever the "Copy to Folders" window is popped
 *  down.  It is used to "chain" the "Copy to Folders" popup windows together
 *  so that when a "Copy to Folders" window for one package is popped down, 
 *  the next "Copy to Folders" window for the next package which was installed
 *  is popped up.  This continues until there are no more packages to
 *  process.  The folder_end global is used to save the number of packages
 *  which could have been installed, and the folder_start global is used to
 *  index into the list of possible packages.
 */
static void
MoreFoldersCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
        if (folder_end != folder_start) {
	        folder_start++;
		PopupInstalledFolders(set_install ? pr->sysList : pr->mediaList);
	}
}

/*  
 *  Callback used for "Copy to Folder" window.  The user is allowed to
 *  choose which users will get the selected package's icons installed.
 *  This callback is used to manage the scrolling list of desktop users if
 *  the "Specific Users" option, an unmanage the scrolling list if any
 *  other option is chosen.
 */
static void
SpecifyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
        Widget list;
	Boolean show = (Boolean) client_data;

	list = (Widget)QueryGizmo (PopupGizmoClass, pr->copyFolder,
			   GetGizmoWidget, "specific_choices"
	);
		   
	if (show)
	        XtManageChild(list);
	else
	        XtUnmanageChild(list);

}

void
PopupIconBox (PkgPtr p_select)
{
	int	count;
	MenuGizmo * i_menu;

	count = GetIconView (p_select);
	if (count != 0) {
		MapGizmo(PopupGizmoClass, pr->iconPopup);
		pr->iconSetName = p_select->pkg_name;
		pr->iconBoxUp = True;
		i_menu = QueryGizmo (PopupGizmoClass, pr->iconPopup, 
				     GetGizmoGizmo, "iconMenu");
		OlSetInputFocus(i_menu->parent, RevertToNone, CurrentTime);
	}
	else {
		sprintf (buf, "%s: %s",
			 GGT (string_newappName), p_select->pkg_name
		);
		XtVaSetValues (
			GetModalGizmoShell (pr->noIconPopup),
			XtNtitle, buf,
			(String)0
		);
		sprintf (
			buf, GGT (format_icons_none), p_select->pkg_name
		);
		SetModalGizmoMessage (pr->noIconPopup, buf);
		MapGizmo(ModalGizmoClass, pr->noIconPopup);
	}
}

/*
 *  Call PopupFolderBox (which popups up the "Copy to Folder" window) for
 *  each icon installed.  This will popup a "Copy to Folder" window, 
 *  displaying the icons to be installed, for each package.
 */
void
PopupInstalledFolders(ListRec *list)
{
        PkgPtr	p;
	int	i;		
  
        for (i = folder_start; i < folder_end; i++, folder_start++){
                 p = list->pkg + i;
		 if (p->pkg_opflag == 'T') {
		          p->pkg_opflag = '\0';
                          PopupFolderBox(p);
			  break;
		 }
	}
}

/*
 *  Popup "Copy to Folder" window.  This routine will be called after
 *  a package has been installed to the system.  Therefore, this version
 *  of the "Copy to Folder" window needs to have the scrolling window
 *  of package icons managed.
 */
static void
PopupFolderBox (PkgPtr p_select)
{
	int	count;
	MenuGizmo * f_menu;

	count = GetFolderView (p_select);
	if (count != 0) {
       		if (XtIsManaged(pr->foldersw)==False)
	                XtManageChild(pr->foldersw);
		if (read_users == True || 
		   ((read_users = MakeFolderLists()) == True)) {
		        ResetFolderCB(NULL, NULL, NULL);
			pr->folderBoxUp = True;
			isfolder = True;
			MapGizmo(PopupGizmoClass, pr->copyFolder);
			f_menu = QueryGizmo (PopupGizmoClass, pr->copyFolder,
					     GetGizmoGizmo, "copyfolderMenu");
			OlSetInputFocus(f_menu->parent, RevertToNone, 
					CurrentTime);
		}
	}
	else {
	        folder_start++;
		PopupInstalledFolders(set_install ? pr->sysList : pr->mediaList);
	}
}

void
PopupPropSheet (PkgPtr p_select)
{
	GetInfo (pr, p_select);
	MapGizmo(PopupGizmoClass, pr->infoPopup);
}

/*
 * Invoked from the "Info" button in set window.
 */

void
SetInfoCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	PkgPtr	p_select = GetSelectedSet (pr->sysList);

	FooterMsg2(NULL);
	PopupPropSheet (p_select);
}

/* 
 * Invoked by "Show Contents" button for sets
 */
void
SetContentsCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	ListRec *	lp = pr->sysList;
	PkgPtr		p_select = GetSelectedSet (lp);

	if (pr->defaultcb == True) {
	        if (STRCMP_CHK_ZERO(p_select->pkg_cat,SET)) {
		        CreatePkgIcons(p_select, lp);
		}
		else {
		        SetPopupMessage(pr->iconPopup, NULL);
			PopupIconBox (p_select);
		}
	}
	else
	        pr->defaultcb = True;
}

/*
 * This timeout routine contains the termination processing for pkgadd(),
 * or custom().
 */

void
WaitInstallPkg (char *names, XtIntervalId intid)
{
	char		str[BUFSIZ];
	ListRec *	lp = pr->mediaList;

	if (lp->cmdfp[1] == NULL || read(fileno(lp->cmdfp[1]), str, BUFSIZ) == 0) {
		setuid(our_uid);
		SetBusy(FALSE);
		lp->timeout = (XtIntervalId)NULL;	/* no more calls! */
		if (lp->cmdfp[1]) {
			_Dtam_p3close(lp->cmdfp, 0);
			lp->cmdfp[0] = lp->cmdfp[1] = (FILE *)0;
		}
		CheckAdd(names);
		FREE(names);
		AllowAddAndDelete ();
	}
	else {
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)WaitInstallPkg, names
		);
	}
}

/*
 * This timeout routine is used for termination processing of installpkg()
 * or removepkg().
 */

void
Wait3_2Pkg (XtPointer dummy, XtIntervalId intid)
{
	char		str[BUFSIZ];
	ListRec *	lp = pr->mediaList;

	if (lp->cmdfp[1] == NULL || read(fileno(lp->cmdfp[1]), str, BUFSIZ) == 0) {
		setuid(our_uid);
		SetBusy(FALSE);
		pr->sysList->timeout = (XtIntervalId)NULL;	/* no more calls! */
		pr->sysList->curlabel = "";
		pr->sysList->viewType = ALL_VIEW;
		SetSysListTitle (pr, pr->sysList->curlabel);
		FooterMsg2 (NULL);
		FindInstalled();
		AllowAddAndDelete ();
	}
	else {
		lp->timeout = XtAddTimeOut (
			1000, (XtTimerCallbackProc)Wait3_2Pkg, NULL
		);
	}
}

static void
CallInstallPkg (char *str)
{
	ListRec *	lp = pr->mediaList;

	sprintf (
		str,
		"%s\"%s\" -e %s/dtexec -ZN %s",
		XTERM, GGT(string_newsvr3Title),
		GetXWINHome("adm"), INSTALL
	);
	setuid(0);
	if (access (INSTALL, X_OK) != 0) {
		MapGizmo(ModalGizmoClass, pr->cantExec);
		setuid(our_uid);
		return;
	}
	if (P3OPEN(str, lp->cmdfp, FALSE) != -1) {
		FooterMsg1 (GGT(string_invokeInstPkg));
		lp->timeout = XtAddTimeOut(1000,
			(XtTimerCallbackProc)Wait3_2Pkg, NULL);
		BringDownPopup(pr->insertNote->shell);
		SetBusy(TRUE);
	}
	else {
		setuid(our_uid);
		SetModalGizmoMessage(pr->insertNote,
					GGT(string_badInstPkg));
	}
}

static void
PrintCheckMedia (int diagn)
{
#ifdef DEBUG
	FPRINTF ((stderr, "CheckMedia(%s) = 0x%x = ", pr->curalias, diagn));
	if (((diagn >> 8) & 1) == 1) {
		FPRINTF ((stderr, "DTAM_UNKNOWN|"));
	}
	if (((diagn >> 9) & 1) == 1) {
		FPRINTF ((stderr, "DTAM_PACKAGE|"));
	}
	if (((diagn >> 10) & 1) == 1) {
		FPRINTF ((stderr, "DTAM_INSTALL|"));
	}
	switch (diagn & ~DTAM_FS_TYPE) {
		case DTAM_BACKUP: {
			FPRINTF ((stderr, "DTAM_BACKUP"));
			break;
		}
		case DTAM_CPIO: {
			FPRINTF ((stderr, "DTAM_CPIO"));
			break;
		}
		case DTAM_CPIO_BINARY: {
			FPRINTF ((stderr, "DTAM_CPIO_BINARY"));
			break;
		}
		case DTAM_CPIO_ODH_C: {
			FPRINTF ((stderr, "DTAM_CPIO_ODH_C"));
			break;
		}
		case DTAM_TAR: {
			FPRINTF ((stderr, "DTAM_TAR"));
			break;
		}
		case DTAM_CUSTOM: {
			FPRINTF ((stderr, "DTAM_CUSTOM"));
			break;
		}
		case DTAM_DOS_DISK: {
			FPRINTF ((stderr, "DTAM_DOS_DISK"));
			break;
		}
		case DTAM_UNFORMATTED: {
			FPRINTF ((stderr, "DTAM_UNFORMATTED"));
			break;
		}
		case DTAM_NO_DISK: {
			FPRINTF ((stderr, "DTAM_NO_DISK"));
			break;
		}
		case DTAM_UNREADABLE: {
			FPRINTF ((stderr, "DTAM_UNREADABLE"));
			break;
		}
		case DTAM_BAD_ARGUMENT: {
			FPRINTF ((stderr, "DTAM_BAD_ARGUMENT"));
			break;
		}
		case DTAM_BAD_DEVICE: {
			FPRINTF ((stderr, "DTAM_BAD_DEVICE"));
			break;
		}
		case DTAM_DEV_BUSY: {
			FPRINTF ((stderr, "DTAM_DEV_BUSY"));
			break;
		}
		default: {
			FPRINTF ((stderr, "0x%x", diagn&~(15<<3|DTAM_FS_TYPE)));
			break;
		}
	}
	FPRINTF ((stderr, "\n"));
#endif /* DEBUG */
}

void
GetMedia (Widget wid, XtPointer client_data, XtPointer call_data)
{
	Boolean		firstTime = (Boolean)client_data;
	char *		ptr;
	char		cmdbuf[BUFSIZ];
	char    	mnt_pt[] = "/install";
	char    	target[] = "/install/install/INSTALL";
	int     	result;
	int		diagn;
	char *		vol;
	char *		dev;
	char		str[256];
	char		tmpalias[BUFSIZ];
	char		buf[BUFSIZ];
	ListRec *	lp = pr->mediaList;

	diagn = DtamCheckMedia (pr->curalias);
	PrintCheckMedia (diagn);

	if (_dtam_flags & DTAM_MOUNTED) {
		/*  Notify user that device is already mounted	*/
		sprintf(buf, GGT(string_mounted), lp->curlabel);
		ErrorNotice(buf, GetGizmoText(string_appError));
		return;
	}
	if (diagn == (DTAM_CPIO|DTAM_PACKAGE)) {
		BringDownPopup(pr->insertNote->shell);
      		media_context = NEXT;
		GetUninstalledList (
			lp, pr->curalias, lp->curlabel
		);
      		return;
	}
	if (diagn == DTAM_CUSTOM) {
		BringDownPopup(pr->insertNote->shell);
      		media_context = NEXT;
		GetCustomList (lp);
		return;
	}
	if (diagn == (DTAM_CPIO|DTAM_INSTALL) || diagn == DTAM_CPIO) {
		CallInstallPkg(str);
		return;
	}
	if (diagn & DTAM_FS_TYPE) {
		if (diagn == (DTAM_FS_TYPE|DTAM_INSTALL)) {
                        /*
                         *	guess it is installpkg format
                         */
                        CallInstallPkg(str);
                        return;
		}
                if (diagn == (DTAM_FS_TYPE|DTAM_PACKAGE)) {
                        BringDownPopup(pr->insertNote->shell);
                        media_context = NEXT;
			GetUninstalledList (
				lp, pr->curalias, lp->curlabel
			);
                        return;
		}
	}
	vol = dev = NULL;
	if (dev = DtamGetDev(pr->curalias,FIRST))
		if ((vol=DtamDevAttr(dev,"volume")) == NULL)
		    	vol = STRDUP(GGT(label_medium));
	if (diagn == DTAM_NO_DISK)
		*str = '\0';
	else
		sprintf(str, GGT(format_notPkg), vol);
	sprintf(str+strlen(str), GGT(format_insFmt),
			vol, lp->curlabel, GGT(label_go));
	/* User doesn't have permission to run fmount */
	if (diagn == DTAM_FS_TYPE) {
		sprintf(str, GGT(string_noPerm));
	}
	if (dev) FREE(dev);
	if (vol) FREE(vol);
	if (media_context != FIRST && firstTime == True) {
		/* Don't display any message if we were invoked */
		/* from the packager icon (no -D in argv[]) and */
		/* GetMedia was called from main(). */
	        FooterMsg1(GGT(string_update_view));
		return;
	}
	SetModalGizmoMessage(pr->insertNote, str);
	MapGizmo(ModalGizmoClass, pr->insertNote);
}

void
helpCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	system (
		"/usr/X/bin/scohelp http://localhost:457/INS_install/appinstT.using_appinstaller.html&"
	);
}

/*
 * Record the size of the package and increment the size of the set
 * that it is contained in.
 */

void
PkgSize (char *str, PkgPtr set, PkgPtr pkg)
{
	char *		ptr;
	int		n = atoi(str);
	ListRec *	lp = pr->sysList;

	pkg->pkg_size = STRDUP(str);
	if (set == NULL) {
		for (set=lp->pkg; set<lp->pkg + lp->count; set++)
			if (STRCMP_CHK_ZERO(set->pkg_name, pkg->pkg_set))
				break;
		if (set == lp->pkg + lp->count)
			return;
	}
	if (set->pkg_size == NULL)
		set->pkg_size = (char *)CALLOC(1,16);
	n += atoi(set->pkg_size);
	sprintf(set->pkg_size, "%d", n);
}

/*
 * Mark those packages in installed->pkg as being part of a set if the name
 * of the package appears in a <pkg_name>/setinfo file.
 * Start by opening the setinfo file and for every noncommented line
 * in the file examine the name of each sub package.  Find this package
 * in installed->pkg and mark it as part of a set by setting the pkg_set
 * member of the installed->pkg record.  The category and description are
 * also taken from this file and set in the record.
 */

static void
ExpandSet (PkgPtr p, ListRec *lp)
{
	PkgPtr	q;
	FILE *	fp;
	char *	ptr;

	sprintf(buf, "%s/%s/setinfo", PKGDIR, p->pkg_name);
	fp = fopen(buf, "r");
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf)-1] = '\0';
		if (*buf && *buf != '#') {
			ptr = strtok(buf, " \t"); /* Get the name of the package */
			for (q=lp->pkg; q<lp->pkg + lp->count; q++) {
				if (strcmp(ptr, q->pkg_name) == 0) {
					q->pkg_set = STRDUP(p->pkg_name);
					ptr = strtok(NULL, " \t");
					ptr = strtok(NULL, " \t");
					ptr = strtok(NULL, " \t");
					q->pkg_cat = STRDUP(ptr);
					ptr += strlen(ptr)+1;
					q->pkg_desc = STRDUP(ptr);
					break;
				}
			}
		}
	}
	fclose(fp);
	/*
	 *	We should go through and get set description and other
	 *	property fields by reading /var/sadm/pkg/<set>/pkginfo
	 */
}

/*
 * Catalogue existing packages and create icons for them.
 */
void
FindInstalled (void)
{
	FILE *		fp;
	struct dirent *	dent;
	DIR *		dirp;
	PkgPtr		p;
	ListRec *	lp = pr->sysList;
	char *		cp;
	char *		name;
	Boolean		found;

	if (lp->count) {
		FreePkgList (lp->pkg, &lp->count);
	}
	/* Loop thru the package directory and for every readable file */
	/* create a record in installed->pkg. */
	if (dirp = opendir(PKGDIR)) {
		while (dent = readdir(dirp)) {
			if (strcmp(dent->d_name,".") == 0
			||  strcmp(dent->d_name,"..") == 0) {
				continue;
			}
			if ((p=AddPkgDesc(lp, dent->d_name)) == NULL) {
				continue;
			}
			SetPkgDefs(p);
		}
		closedir(dirp);
		/*
		 *	for sets, locate the component packages
		 */
		for (p=lp->pkg; p<lp->pkg+lp->count; p++)
			if (STRCMP_CHK_ZERO(p->pkg_cat,SET))
				ExpandSet(p, lp);
	}
	/*
	 * Look for any preSVR4 packages.  These are packages found
	 * in /var/options that aren't already in the package list.
	 */
	if (dirp = opendir (OPTIONS)) {
		while (dent = readdir(dirp)) {
			if ((cp = strstr (dent->d_name, ".name")) == NULL) {
				continue;
			}
			name = STRDUP (dent->d_name);
			*cp = '\0';
			found = False;
			for (p=lp->pkg; p<lp->pkg+lp->count; p++) {
				if (strcmp(p->pkg_name, dent->d_name) == 0) {
					found = True;
					break;
				}
			}
			if (found == False) {
				p = InitPkg (lp, name);
				sprintf (buf, "%s/%s", OPTIONS, name);
				p->pkg_name = STRDUP(dent->d_name);
				p->pkg_cat = STRDUP("preSVR4");
				p->pkg_fmt = STRDUP("3.2");
				/* Get the package description */
				fp = fopen (buf, "r");
				if (fp != NULL) {
					fgets (buf, BUFSIZ, fp);
					p->pkg_desc = STRDUP (buf);
					fclose (fp);
				}
			}
			FREE (name);
		}
		closedir(dirp);
	}
	/*
	 * Look for Xenix (custom) packages in /etc/perms and
	 * add them to the list of packages.
	 */
	if (dirp = opendir(PERMS)) {
		while (dent = readdir(dirp)) {
			/*
			 * Ignore the bundle directory and inst file -
			 * these are not packages.
			 */
			if (strcmp(dent->d_name,".") == 0
			||  strcmp(dent->d_name,"..") == 0
			||  strcmp(dent->d_name,"inst") == 0
			||  strcmp(dent->d_name,"bundle") == 0)
				continue;
			p = InitPkg (lp, dent->d_name);
			p->pkg_name = STRDUP(dent->d_name);
			p->pkg_fmt  = STRDUP(CUSTM);
			p->pkg_cat  = STRDUP(SET);
			ExpandCustomSet (lp, p);
		}
		closedir(dirp);
	}
	CreateSetIcons (lp, lp->curlabel);
	lp->validList = True;
}

/*
 * Create the Copy to Folder popup.
 */

void
CreateCopyToFolder() 
{
	ScrolledWindowGizmo *scroll;
	RubberTileGizmo *choice;
	ListGizmo *users;

	pr->copyFolder = CopyGizmo (PopupGizmoClass, &copy_folder);
	pr->copy = CreateGizmo(pr->base->shell, PopupGizmoClass, 
				   pr->copyFolder, NULL, 0);
	XtAddCallback (pr->copy,
		      XtNpopdownCallback,	MoreFoldersCB,
		      (XtPointer) 0
	);

	scroll = QueryGizmo(PopupGizmoClass, pr->copyFolder,
                            GetGizmoGizmo, "innerBox3");

	XtVaSetValues (scroll->sw,
		       XtNheight,     4*HEIGHT/5,
		       XtNwidth,      WIDTH,
		       XtNhStepSize,  INC_X/2,
	               XtNvStepSize,  INC_Y/2,
		       NULL
	);

	pr->foldersw = scroll->sw;

	choice = QueryGizmo(PopupGizmoClass, pr->copyFolder,
                            GetGizmoGizmo, "specific_choices");

	XtVaSetValues (choice->rubberTile, 
		       XtNshadowThickness,	0,
		       NULL
	);

	users = QueryGizmo(PopupGizmoClass, pr->copyFolder,
                            GetGizmoGizmo, "u_file");

	pr->usersw = users->flatList;

	XtVaSetValues(pr->usersw,
                      XtNviewHeight,          USERHEIGHT,
                      XtNexclusives,          (XtArgVal)TRUE,
                      XtNitemFields,          (XtArgVal)ListFields,
		      XtNnumItemFields,       (XtArgVal)XtNumber (ListFields),
                      XtNitems,               (XtArgVal)UserItems,
                      XtNnumItems,            (XtArgVal)u_cnt,
                      NULL
	);

	SpecifyCB(NULL, False , NULL);
}

/*
 *  Sort list of login names.  Used to have users appear in alphabetical
 *  order in scrolling window.
 */
int
cmplogin(UserPtr x, UserPtr y)
{
        return strcoll(x->pw_name, y->pw_name);
}

/*
 *  Free data structure containing the system's user information (e.g.
 *  login, home directory, etc...
 */
static  void
FreeUserList(void)
{
        UserPtr up;
        char    *p;

        free (uid_list);
        while (u_cnt--) {
                up = &u_list[u_cnt];
                if (p = up->pw_name)    free(p);
                if (p = up->pw_comment) free(p);
                if (p = up->pw_dir)     free(p);
                if (p = up->pw_shell)   free(p);
	      }
        free (u_list);
        u_list = (UserPtr)0;
        u_cnt = 0;
}

/* 
 *  Create scrolling list of desktop users for the "Copy to Folder"
 *  popup window.
 */
static Boolean
MakeFolderLists(void)
{

        static  time_t   lastListTime = 0;
	struct  stat	 pwStat;
	struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	char   	dtfile[40];
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
	while (pwd = _DtamGetpwent(STRIP, NIS_EXPAND, NULL)) {
		if (pwd->pw_uid > UID_MAX)
			continue;
		if (pwd->pw_dir == NULL || strlen(pwd->pw_dir) == 0)
		        continue;
		sprintf(dtfile, "%s/%s",GetXWINHome("desktop/LoginMgr/Users"),
				        pwd->pw_name);
		if (stat (dtfile, &pwStat) == -1)
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
		up->pw_comment = pwd->pw_comment? strdup(pwd->pw_comment):NULL;
		up->pw_dir = pwd->pw_dir? strdup(pwd->pw_dir): NULL;
		up->pw_shell = pwd->pw_shell? strdup(pwd->pw_shell): NULL;
	}
	endpwent();

	qsort((void *)u_list, u_cnt, sizeof(UserRec), (int (*)())cmplogin);

	if (UserItems = (FListPtr)malloc(u_cnt*sizeof(FListItem)))
		for (n = 0; n < u_cnt; n++) {
			UserItems[n].label = u_list[n].pw_name;
			UserItems[n].set = FALSE;
		}
	XtVaSetValues(pr->usersw,
		      XtNitems,		UserItems,
		      XtNnumItems,	u_cnt,
		      XtNviewHeight,	USERHEIGHT,
		      XtNexclusives,	FALSE,
		      NULL
        );
	return True;
}

int
cmpserver(char *x, char *y)
{
        return strcoll(x, y);
}

/* 
 *  Create scrolling list of available servers.
 */
void
MakeServerList(void)
{
        char *server, *getservername(), ts[BUFSIZ];
	int n;
	ListGizmo	*w;

	pr->nservers = CopyGizmo(PopupGizmoClass, &nw_servers);
	CreateGizmo(pr->base->shell, PopupGizmoClass, pr->nservers, NULL, 0);
        w = QueryGizmo(PopupGizmoClass, pr->nservers,
                            GetGizmoGizmo, "nwservers");
	pr->nwlist = w->flatList;

/*
        getseverlist();
	qsort((void *)s_list, s_cnt, sizeof(char), (int (*)())cmpserver);
*/
	s_cnt = 1;
	if (ServerItems = (FListPtr)malloc(s_cnt*sizeof(FListItem)))
		for (n = 0; n < s_cnt; n++) {
			ServerItems[n].label = strdup("snowtown");
			ServerItems[n].set = FALSE;
		};
	XtVaSetValues(pr->nwlist,
                      XtNviewHeight,          	USERHEIGHT,
                      XtNexclusives,          	(XtArgVal)TRUE,
                      XtNitemFields,          	(XtArgVal)ListFields,
                      XtNnumItemFields,       	(XtArgVal)XtNumber(ListFields),
		      XtNitems,			(XtArgVal)ServerItems,
		      XtNnumItems,		(XtArgVal)s_cnt,
		      NULL
        );
}

/*  
 *  Locate all installables (i.e. executables or icons) from the package
 *  being removed.  If a link was created in /usr/X/desktop, it should be
 *  removed when the package is removed.
 */
void
FindFuture(PkgPtr p, int file_num)
{
        FILE * idata;
	char   str[PATH_MAX];

        sprintf(buf, "%s/%s", ICONDIR, p->pkg_name);
	sprintf(str, "%s/.%d", GetXWINHome(ICONDIR), file_num);  

        if (idata=fopen(GetXWINHome(buf),"r")) {
	        link(buf, str);
		p->pkg_file = IS_ICON;
	}
	else  {
                if (p->pkg_fmt[0] == 'C') {
                        sprintf(buf, "%s/%s", PERMS,
                                        p->pkg_set? p->pkg_set: p->pkg_name);
                        if (idata=fopen(buf,"r"))  {
			        link(buf, str);
				p->pkg_file = IS_PERM;
			}
		}
                else if (idata=fopen(LOGFILE,"r")) {
                        link(LOGFILE, str);
			p->pkg_file = IS_LOG;
		}
		else  {
		        p->pkg_install = NULL;
			p->pkg_file = IS_NONE;
		}
	}
	p->pkg_install = strdup(str);	
}

int
RemoveLinks(PkgPtr p)
{
        FILE * ifp;
	char * ptr;
	char * ptr2;
	char * name;
	char   str[PATH_MAX];
	char * file_format;

	if ((ifp = fopen(p->pkg_install, "r")) == NULL)
	        return -1;
        switch (p->pkg_file) {

	case IS_ICON:
	        while (fgets(buf, BUFSIZ, ifp)) {
		        buf[strlen(buf) - 1] = '\0';
			ptr = FilterIcon(buf);
			if (ptr)  {
			        *ptr++ = '\0';    
				ptr2 = strchr(ptr, '\t');
				*ptr2 = '\0';
				name = ToolboxName(ptr);
				if (owner)  {
				        sprintf(buf, "%s/%s",
						GetXWINHome("desktop"), name);
				        remove(buf);
				}
			}
		}

		break;

	case IS_PERM:
	case IS_LOG:

		file_format = (p->pkg_file == IS_PERM) ? "C" : "4.0" ;
		while (fgets(buf, BUFSIZ, ifp)) {
		        buf[strlen(buf)-1] = '\0';
			ptr = FilterExecutable(p->pkg_name, buf, file_format);
			if (ptr) {
			        ptr2 = strrchr(ptr, '/') + 1;
				name = ToolboxName(ptr2);
				if (owner)  {
				        sprintf(str, "%s/%s",
						GetXWINHome("desktop"), name);
				        remove(str);
				}
			}
		}

		break;

	case IS_NONE:
	default:
		return -1;

	}

        fclose(ifp);
	remove(p->pkg_install);
	return 0;
}

/*
 * This is the callback from the "Network..." window.
 * This routine handles Select and Cancel.
 */

void
NwCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData  *olcd = (OlFlatCallData *) call_data;
	int             n = olcd->item_index, i;
	ListRec *	lp = pr->mediaList;
	InputGizmo *	ip;

	BringDownPopup(pr->nservers->shell);
	if (n == 1) {	/* Cancel */
		if (lp->timeout) {
			XtRemoveTimeOut(lp->timeout);
			lp->timeout = (XtIntervalId)NULL;
		}
	}
	else {		/* Select */
	        pr->servername = 0;
	        for (i=0; i < s_cnt; i++) 
		        if (ServerItems[i].set == True) 
			        pr->servername = ServerItems[i].label;
		ip = QueryGizmo (
		     BaseWindowGizmoClass, pr->base, GetGizmoGizmo, "input"
		);
		XtVaSetValues(ip->textFieldWidget, 
			      XtNstring, pr->servername,
			      (String)0
		);

		ReadFromServer (pr, lp);
	}
}
