#ifndef NOIDENT
#pragma	ident	"@(#)packager.h	15.1"
#endif
/*
 *	packager.h
 */

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include "../dtamlib/dtamlib.h"
#include <unistd.h>
#include <sys/fcntl.h>

#include "pkg.h"
#include "pkg_msgs.h"

#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/STextGizmo.h>

typedef enum { ALL_VIEW, APP_VIEW, SYS_VIEW, DEV_VIEW } ViewTypes;
typedef enum { PKG_DELETE, SET_DELETE } DeleteOrigin;

#define STRCMP_CHK_ZERO(s1, s2)    (s1 && s2 && (strcmp(s1, s2) == 0))
#define STRCMP_CHK_NONZERO(s1, s2) (s1 && s2 && (strcmp(s1, s2) != 0))

#define GGT		GetGizmoText
#define INIT_X		16
#define INIT_Y		16
#define INC_X		96
#define INC_Y		64
#define MARGIN		16

#define MAX_DEVS	10
#define WIDTH		(int)(40*x3mm)
#define HEIGHT		(int)(10*y3mm)

#define	HELP_PATH	"dtadmin/App_Installer.hlp"
#define	ICON_NAME	"pkgmgr48.icon"
#define ICONDIR		"desktop/PackageMgr"
#define PKGDIR		"/var/sadm/pkg"
#define LOGFILE		"/var/sadm/install/contents"
#define IS_ICON		0
#define IS_PERM		1
#define IS_LOG		2
#define IS_NONE		3

#define		FooterMsg(lp, txt)	XtVaSetValues(\
			lp->footer,XtNleftFoot,txt,(String)0\
		)
#define		FooterMsg1(txt)	XtVaSetValues(\
			pr->mediaList->footer,XtNleftFoot,txt,(String)0\
		)
#define		FooterMsg2(txt)	XtVaSetValues(\
			pr->sysList->footer,XtNleftFoot,txt,(String)0\
		)

#define BLAH_BLAH_BLAH	Widget wid, XtPointer client_data, XtPointer call_data

#ifdef DEBUG
#define FPRINTF(x)	fprintf x;
#define P3OPEN(a, b, c)	fprintf (stderr, "exec %s\n", a), \
			_Dtam_p3open(a, b, c)
#else
#define FPRINTF(x)
#define P3OPEN(a, b, c)	_Dtam_p3open(a, b, c)
#endif

typedef struct {
	PkgPtr		pkg;		/* List of packages */
	int		max;		/* Max # of packages */
	int		count;		/* Number of packages in list */
	XtIntervalId	timeout;
	FILE *		cmdfp[2];
	ViewTypes	viewType;	/* Specify what icons get displayed */
	Boolean		validList;	/* Indicates a valid sys/media list */
	char *		curlabel;
	StaticTextGizmo *title;
	Widget		footer;
	Widget		sw;		/* Scrolled window containing icons */
	char		input[BUFSIZ+1];	/* Input buffer within timeout */
	int		setCount;	/* # of sets displayed */
	DmItemPtr	setitp;
	Widget		setBox;		/* Icon box within sw */
	DmContainerRec	set_cntrec;	/* Container for sets */
	Dimension	setx;
	Dimension	sety;
	Boolean		hide;		/* Hide/Show option */
} ListRec;

typedef struct {
	ListRec *	sysList;
	ListRec *	mediaList;
	Boolean		pkgBoxUp;	/* Indicates pkg window mapped */
	Boolean		iconBoxUp;	/* Indicates icon window mapped */
	Boolean		folderBoxUp;	/* Indicates folder window mapped */
	char *		deskTopDir;
	Boolean		installInProgress;/* pkgrm or pkgadd in progress */
	DeleteOrigin	delete;		/* Origin of delete command */
	Boolean		defaultcb;	

	/* Items used only by the network list */
	char *		servername;
	Widget		nwlist;	       /* Scrolling window of servers */

	/* Items used only by the media list */
	Boolean		have_dir;	/* Filename from pathfinder entered */
	char *		curalias;	/* "Disk_A", "Disk_B", ..., "Other" */
	char *		spooldir;	/* Path from Other... pathfinder */

	/* Items used only by the system list */
	char *		labelAddOn;	/* "Add-on" */
	char *		labelAll;	/* "All" */
	char *		labelSystem;	/* "System" */
	time_t		optionDate;	/* Last modification /var/options */
	time_t		pkgDirDate;	/* Last modification /var/sadm/pkg */
	time_t		permsDate;	/* Last modification /etc/perms */
		/* Package popup things */
	DmItemPtr	pkgitp;
	Dimension	pkgx;
	Dimension	pkgy;
	int		pkgCount;	/* # of packages displayed */
	Widget		pkgBox;
	char *		pkgSetName;	/* Set name for these packages */
	PopupGizmo *	pkgPopup;	/* Package popup */
	Widget		pkgsw;		/* sw parenting icon sheet */
		/* Icon view popup things */
	DmItemPtr	iconitp;
	DmItemPtr	folderitp;
	Dimension	iconx;
	Dimension	icony;
	Widget		iconBox;
	Widget		folderBox;
	char *		iconSetName;	/* Set name for these icons */
	PopupGizmo *	iconPopup;	/* Application Installer: Icons */
	ModalGizmo *	noIconPopup;	/* No programs */
	Widget		iconsw;	       /* Scrolling window in the icon sheet */
		/* Folder view popup things */
	Widget		foldersw;      /* Scrolling window in copy to folder */
	Widget		usersw;	       /* Scrolling window of DT users */

	/* Textfields in property sheet */
	Widget	name;
	Widget	description;
	Widget	category;
	Widget	version;
	Widget	architect;
	Widget	vendor;
	Widget	date;
	Widget	size;

	/* Icon fields within the main window. */
	Widget	type;	/* Upper control area of icon sheet */

	/* Popup shells */
	Widget	icon;	/* Icon popup shell */
	Widget	info;	/* Property sheet popup shell */
	Widget  copy;	/* Folder sheet popup shell */

	/* Gizmos */

	PopupGizmo *	infoPopup;	/* Property sheet popup */
	BaseWindowGizmo *	base;	/* The base window */

	/* File Gizmos */
	FileGizmo *	prompt;		/* Other... pathfinder */

	PopupGizmo *	nservers;	/* Network... pathfinder */

	/* Modal Gizmos */
	PopupGizmo *	catalog;	/* Cataloging window */
	PopupGizmo *	scompatPopup;
	PopupGizmo *	copyFolder;	/* Copy to Folder */
	ModalGizmo *	insertNote;	/* Insert into device notice */
	ModalGizmo *	cantExec;	/* No permission notice */
} PackageRecord;

extern char		ADDPKG[];
extern char		CATPKG[];
extern char		PKGLIST[];
extern char		APPL[];
extern char		CUSTM[];
extern char		CUSTOM[];
extern HelpInfo		HelpIntro;
extern HelpInfo		HelpFolder;
extern HelpInfo		HelpIcons;
extern HelpInfo		HelpPkgwin;
extern HelpInfo		HelpProps;
extern HelpInfo		HelpCatalog;
extern HelpInfo		HelpPackage;
extern HelpInfo		HelpCompat;
extern char		LANG_C[];
extern char		OPTIONS[];
extern char		PERMS[];
extern char		PKGINFO[];
extern char		REMPKG[];
extern char		SET[];
extern char		AS[];
extern char		PE[];
extern char		UNIXWARE[];
extern char		XTERM[];
extern Arg		arg[];
extern char		buf[];
extern XFontStruct *	def_font;
extern DmFclassRec	exec_fcrec;
extern Boolean		media_context;
extern char	*	our_node;	/* Name of this machine */
extern uid_t		our_uid;	/* Uid of user */
extern Boolean		owner;		/* True if we own the desktop */
extern DmContainerRec	pkg_cntrec;
extern DmFclassRec	pkg_fcrec;
extern DmFclassRec	set_fcrec;
extern char *		spooled;
extern char *		network;
extern Display *	theDisplay;
extern Screen *		theScreen;
extern DmFclassRec	unpkg_fcrec;
extern DmFclassRec	unset_fcrec;
extern Dimension	x3mm;
extern Dimension	y3mm;
extern PackageRecord *	pr;

extern void	helpCB (BLAH_BLAH_BLAH);
extern PkgPtr	InitPkg (ListRec *lp, char *name);
extern void	CreateSetIcons (ListRec *lp, char *title);
extern void	FindInstalled (void);
extern void	PkgSize (char *str, PkgPtr set, PkgPtr pkg);
extern void	WaitRemovePkg (char *names, XtIntervalId intid);
extern void	WaitInstallPkg (char *names, XtIntervalId intid);
extern void	ReadCustom (PackageRecord *pr, XtIntervalId tid);
extern char *	FilterExecutable (char *name, char *line, char *format);
extern char *	FilterIcon (char *line);
extern void	GetMedia (BLAH_BLAH_BLAH);
extern void	Wait3_2Pkg (XtPointer dummy, XtIntervalId intid);
extern PkgPtr	GetSelectedPkg ();
extern PkgPtr	GetSelectedSet (ListRec *lp);
extern void	SetSensitivity (void);
extern PkgPtr	AddUninstalledPackage (ListRec *lp, char *str, Boolean set);
extern PkgPtr	AddInstalledPackage (ListRec *lp, char *str);
extern void	SetContentsCB (BLAH_BLAH_BLAH);
extern void	SetInfoCB (BLAH_BLAH_BLAH);
extern void	SetSysListTitle (PackageRecord *pr, char *str);
extern void	SetMediaListTitle (PackageRecord *pr);
extern void	SetViewMenuLabels (int index);
extern void	CreatePkgIcons (PkgPtr pp, ListRec *lp);
extern void	ReadUninstalledList (XtPointer client_data, XtIntervalId tid);
extern void	CancelPkgCB (BLAH_BLAH_BLAH);
extern PkgPtr	AddPkgDesc (ListRec *lp, char *name);
extern void	SetPkgDefs (PkgPtr p);
extern void	CreatePkgIcons (PkgPtr pp, ListRec *lp);
extern void	CheckAdd (char *names);
extern void	PopupIconBox (PkgPtr p_select);
extern void	PopupPropSheet (PkgPtr p_select);
extern void	CheckDelete (ListRec *lp, char *names);
extern void	SetFormatCB (BLAH_BLAH_BLAH);
extern char *	ScompatVariable ();
extern void	UpdateMediaListIcon (PkgPtr p, Boolean installed);
extern void	CreateIconSheet (void);
extern void	CreateNoIconSheet (void);
extern void	AllowAddAndDelete (void);
extern void	DontAllowAddOrDelete (void);
extern void	SetSetSensitivity (ListRec *lp);
extern void	SetPkgSensitivity ();
extern int	CheckP3closeStatus ();
extern void	ErrorNotice ();
