#pragma ident	"@(#)dtm:help.h	1.74.3.24"

#ifndef _help_h
#define _help_h

#include <mapfile.h>

#include <MGizmo/MenuGizmo.h>
#include <MGizmo/PopupGizmo.h>

#include <HyperText.h>
#include <DtI.h>

/* default section name for a section with no name; not seen by the user */
#define DEFAULT_SECTION_NAME "Unknown"

/* section name for Table of Contents */
#define TABLE_OF_CONTENTS "TOC"

/* default number of rows and columns to display in help window text pane */
#define HELP_WINDOW_NUM_ROWS 15
#define HELP_WINDOW_NUM_COLS 70

/* default application icon */
#define DFLT_APP_ICON "exec.icon"

#define DESCRP         "_DESCRP"
#define HELP_FILE      "_HELPFILE"
#define HELP_DIR       "_HELPDIR"
#define DFLT_ICONLABEL "_DFLT_ICONLABEL"

/* help manager's help file name */
#define HELPMGR_HELP_FILE 		"DesktopMgr/help.hlp"

/* section tags for help manager */
#define USING_HELP_SECT			"10"

/* help file and section tag for dtm */
#define DESKTOP_HELPFILE		"DesktopMgr/desktop.hlp"
#define DESKTOP_INTRO_SECT		"10"
#define MOUSE_AND_KEYBOARD_SECT         "30"
#define BAD_DTFCLASS_SECT		"25"
#define EXIT_DESKTOP_SECT		"460"

#define ICON_SETUP_HELPFILE		"DesktopMgr/iconset.hlp"
#define ICON_SETUP_INTRO_SECT		"10"
#define ICON_SETUP_NEW_CLASS_SECT	"50"
#define ICON_SETUP_CHANGE_CLASS_SECT	"160"
#define ICON_SETUP_LIBRARY_FIND_SECT	"90"
#define ICON_SETUP_TEMPLATE_FIND_SECT	"110"
#define ICON_SETUP_ICON_LIBRARY_SECT	"80"
#define ICON_SETUP_SWITCH_VIEW_SECT	"25"
#define ICON_SETUP_DELETE_CLASS_SECT	"180"

#define SHUTDOWN_HELPFILE		"DesktopMgr/shutdown.hlp"
#define SHUTDOWN_INTRO_SECT		"10"
#define SHUTDOWN_EXIT_SECT		"20"

#define FOLDER_HELPFILE			"DesktopMgr/folder.hlp"
#define FOLDER_INTRO_SECT		"10"
#define FOLDER_FILE_NEW_SECT		"60"
#define FOLDER_FILE_COPY_SECT		"80"
#define FOLDER_FILE_RENAME_SECT		"100"
#define FOLDER_FILE_CONVERT_SECT	"240"
#define FOLDER_FILE_MOVE_SECT		"120"
#define FOLDER_FILE_LINK_SECT		"140"
#define FOLDER_FILE_FIND_SECT		"170"
#define FOLDER_FILE_PROP_SECT		"190"
#define FOLDER_VIEW_FILTER_SECT		"212"
#define FOLDER_OVERWRITE_SECT		"153"
#define FOLDER_FOUND_WIN_SECT		"175"
#define FOLDER_FILE_OPEN_OTHER_SECT	"30"
#define FOLDER_CONFIRM_SYS_DELETE_SECT	"157"

#define FMAP_HELPFILE			"DesktopMgr/fmap.hlp"
#define FMAP_INTRO_SECT			"10"
#define FMAP_BEGIN_OTHER_SECT		"20"

#define WB_HELPFILE			"DesktopMgr/wb.hlp"
#define WB_INTRO_SECT			"10"
#define WB_EMPTY_SECT			"30"
#define WB_FILE_PROP_SECT		"70"
#define WB_OPTIONS_SECT			"40"
#define WB_DUE_DELETE_SECT		"50"

#define HELPDESK_HELPFILE		"DesktopMgr/helpdesk.hlp"

#define APPLICATIONS_FOLDER_HELPFILE	"DesktopMgr/appl.hlp"
#define APPLICATIONS_FOLDER_INTRO_SECT	"10"

#define PREFERENCES_FOLDER_HELPFILE	"DesktopMgr/pref.hlp"
#define PREFERENCES_FOLDER_INTRO_SECT	"10"

#define DISKS_ETC_FOLDER_HELPFILE	"dtadmin/disk.hlp"
#define DISKS_ETC_FOLDER_INTRO_SECT	"10"

#define ADMIN_TOOLS_FOLDER_HELPFILE	"DesktopMgr/admintools.hlp"
#define ADMIN_TOOLS_FOLDER_INTRO_SECT	"10"

#define GAMES_FOLDER_HELPFILE		"DesktopMgr/games.hlp"
#define GAMES_FOLDER_INTRO_SECT		"10"

#define MAILBOX_FOLDER_HELPFILE		"DesktopMgr/Mailbox.hlp"
#define MAILBOX_FOLDER_INTRO_SECT	"10"

#define NETWORKING_FOLDER_HELPFILE	"DesktopMgr/Networking.hlp"
#define NETWORKING_FOLDER_INTRO_SECT	"10"

#define NETWARE_FOLDER_HELPFILE		"NetWare/NetWare.hlp"
#define NETWARE_FOLDER_INTRO_SECT	"10"

#define STARTUP_ITEMS_FOLDER_HELPFILE	"DesktopMgr/startup.hlp"
#define STARTUP_ITEMS_FOLDER_INTRO_SECT	"10"

#define WALLPAPER_FOLDER_HELPFILE	"DesktopMgr/wallpaper.hlp"
#define WALLPAPER_FOLDER_INTRO_SECT	"10"

#define UUCP_INBOX_FOLDER_HELPFILE	"DesktopMgr/uucp.hlp"
#define UUCP_INBOX_FOLDER_INTRO_SECT	"10"

#define DISKETTE_FOLDER_HELPFILE	"dtadmin/floppy.hlp"
#define DISKETTE_FOLDER_INTRO_SECT	"70"

#define CDROM_FOLDER_HELPFILE		"dtadmin/cdrom.hlp"
#define CDROM_FOLDER_INTRO_SECT		"30"

/*
 * Since keywords and definitions are stored as properties. Attrs is
 * used to distinguish them.
 */
#define DM_B_KEYWORD	(1 << 0)
#define DM_B_DEF		(1 << 1)

/* macros for accessing HyperText widget's HyperSegment structures */
#define HyperSegmentKey(HS)        ((HS)->key)
#define HyperSegmentText(HS)       ((HS)->text)
#define HyperSegmentScript(HS)     ((HS)->script)
#define HyperSegmentRV(HS)         ((HS)->reverse_video)

/* Use this now */
enum helpSourceType {
	DT_DISK_SOURCE, DT_STRING_SOURCE, DT_DESKTOP_SOURCE
};

/* global variables */
extern const char *toc_title;
extern const char *def_title;
extern const char *notes_title;
extern const char *search_title;
extern const char *bmark_title;
extern const char *gloss_title;
extern const char *help_str;
extern const char *nohelp_str;


typedef struct dmHelpAppRec *DmHelpAppPtr;

/*
 * This flat list expects a pointer to a vector of pointers to
 * be supplied as XtNformatData.  Therefore, instead of making
 * the bookmark label fields part of the DmHelpBmarkRec structure,
 * they are contained in a separate structure.
 */
typedef struct dmHelpBmarkRec *DmHelpBmarkPtr;
typedef struct dmHelpBmarkRec {
	char *app_name;  	/* application name                */
	char *file_name; 	/* file name                       */
	char *sect_name; 	/* section name for display        */
	char *sect_tag;	 	/* section tag to locate a section */
	DmHelpBmarkPtr next;	/* ptr to next bookmark */
} DmHelpBmarkRec;

typedef struct _namelist *namelistp;
typedef struct _namelist {
	namelistp next;
	char name[4];
} namelist;

/*
 * This structure describes a unique location in a help file.
 */
typedef struct {
	char *file;      /* help file name */
	char *sect_tag;  /* section tag    */
	char *sect_name; /* section name   */
	char *string;    /* string source  */
	namelistp filep; /* point to the help file name list */
	unsigned int linecnt; /* line number in the help section */
} DmHelpLocRec, *DmHelpLocPtr;

/*
 * This structure contains the name of a section and the name of its
 * associated notes file.
 */
typedef struct dmHelpNotesRec *DmHelpNotesPtr;
typedef struct dmHelpNotesRec {
	char *sect_tag;		/* section tag     */
	char *sect_name;	/* section name    */
	char *notes_file;	/* notes file name */
	DmHelpNotesPtr next;	/* next notes rec  */
} DmHelpNotesRec;

/*
 * This structure describes each section in a help file. The sections are
 * read in pre-order into an array.
 */
typedef struct dmHelpSectRec *DmHelpSectPtr;
typedef struct dmHelpSectRec {
	unsigned short	level;         /* level in the tree */
	DtAttrs		attrs;         /* section attributes */
	char		*name;         /* section name */
	char		*alias;        /* alias section name */
	char		*tag;          /* section tag */
	char		*raw_data;     /* ptr to the section in file*/
	char		*cooked_data;  /* ptr to processed section */
	unsigned long	raw_size;      /* size of the raw section */
	unsigned long	cooked_size;   /* size of processed section */
	DtPropList	keywords;      /* list of keywords */
	DtPropList	defs;          /* list of definitions */
	Boolean		matched;       /* matched section in search */
	Boolean		notes_chged;   /* was notes changed */
} DmHelpSectRec;

/*
 * This structure describes a help file.
 */
typedef struct {
	int		count;         /* usage count */
	int		version;       /* version */
	char		*name;         /* file name */
	char		*title;        /* title */
	char		*notes;        /* name of notes file */
	DmHelpNotesPtr	notesp;        /* pointer to notes */
	DmMapfilePtr	mp;            /* map file data */
	DmHelpSectPtr	toc;           /* ptr to toc section */
	DmHelpSectPtr	sections;      /* ptr to section array */
	unsigned short	num_sections;  /* size of sections array */
	Dimension	width;         /* width of displayed text */
	DtAttrs		attrs;         /* file attributes */
	DtPropList	keywords;      /* list of keywords */
	DtPropList	defs;          /* list of definitions */
	DmHelpSectPtr	prev_hsp;      /* previous match */
} DmHelpFileRec, *DmHelpFilePtr;

/*
 * This structure describes a help window. The history stack is stored here.
 */
typedef struct {
	/* These fields must be the same as DmWinRec */
	DtAttrs		attrs;         /* window type, etc. */	
	void *		gizmo_shell;   /* "toplevel" gizmo */
	Widget		shell;         /* shell */
	Widget		swin;          /* scrolled window */

	Gizmo		defWin;	/* definition popup gizmo */
	Gizmo		searchWin;	/* search popup gizmo */
	Gizmo		notesWin;	/* notes popup gizmo */
	Gizmo		glossWin;	/* glossary popup gizmo */
	Gizmo		bmarkWin;	/* bookmark popup gizmo */
	Widget		htext;         /* hypertext widget */
	Widget		notes_te;      /* notes textedit widget */
	Widget		search_tf;     /* search textfield widget */
	Widget		bmark_flist;   /* bookmark flat list */
	Widget		menubar;       /* menubar widget */
	DmHelpFilePtr	hfp;           /* ptr to file rec */
	DmHelpSectPtr	hsp;           /* ptr to current section rec*/
	DmHelpLocPtr	stack;         /* ptr to stack */
	int		stack_size;    /* stack size */
	int		sp;            /* stack pointer (idx) */
	int		app_id;        /* application id */
	char		*defn;		/* definition of a term */
	char 		*search_str;	/* string specified for search */
	char		*search_file;	/* file being searched */
	namelist	*help_files;	/* list of all the help files */
	namelist	*filep;		/* point to current search file name */
	unsigned int	linecnt;	/* current line number in a section */
	unsigned int	index;		/* index into the current section */
	Boolean		found;		/* at least one successful search in
					   the current file */
} DmHelpWinRec, *DmHelpWinPtr;

/*
 * This structure stores info about each application that has registered
 * with the help manager. It assumes that there is one window per application.
 * There is a linked list of these structures hanging off of the desktop
 * structure.
 */
typedef struct dmHelpAppRec {
	int		app_id;        /* application id */
	int		num_bmark;     /* number of bookmarks */
	char		*name;         /* application name */
	char		*title;        /* application title */
	char		*help_dir;     /* help directory */
	DmHelpWinRec	hlp_win;       /* help window data */
	DmHelpAppPtr	next;          /* next app */
	DmHelpBmarkPtr	bmp;           /* ptr to list of bookmarks */
} DmHelpAppRec;

/*
 * This structure is used for storing the file class information
 * for icons and the list of applications in the Help Desk.
 */
typedef struct dmHDDataRec *DmHDDataPtr;
typedef struct dmHDDataRec {
	DmFclassPtr	fcp;           /* ptr to file class */
	DmHelpAppPtr	hap;           /* ptr to app structure */
} DmHDDataRec;


/** external declarations **/
extern DmHDDataPtr	hddp;

/* Used in helpCallback for popup windows */
typedef struct dmHelpInfoRec {
	int  app_id;
	char *file;
	char *section;
} DmHelpInfoRec, *DmHelpInfoPtr;

/* general routines */
extern char *DmGetSectTag(DmHelpFilePtr hfp, char *sect_name);
extern char *Dm__GetSectName(DmHelpFilePtr hfp, char *sect_tag);
extern int Dm__GetNameValue(DmMapfilePtr mp, char **name, char **value);
extern int DmGetKeyword(DmMapfilePtr mp, DtPropListPtr plistp);
extern void DmGetDefinition(DmMapfilePtr mp, DtPropListPtr plistp);
extern void DmReadDefFile(DmMapfilePtr mp, DtPropListPtr plistp, char *path);
extern DmHelpLocPtr DmHelpRefToLoc(char *p);
extern char * DmGetFullPath(char *file_name, char *help_dir);

/* application related routines */
extern void DmRemoveAppFromList(DmHelpAppPtr hap);
extern void DmFreeHelpAppID(int app_id);

/* help window related routines */
extern void DmCloseHelpWindow(DmHelpWinPtr hwp);
extern void DmDisplayHelpString(DmHelpAppPtr hap, char *title, char *string);
extern void DmChgHelpWinBtnState(DmHelpWinPtr hwp, Boolean skip_menubar,
	Boolean sensitive);

/* help file related routines */
extern DmHelpFilePtr DmOpenHelpFile(DmHelpAppPtr hap, char *filename);
extern void DmCloseHelpFile(DmHelpFilePtr hfp);

/* section related routines */
extern int DmProcessHelpSection(DmHelpSectPtr hsp);
extern void DmFreeAllHelpSections(DmHelpSectPtr hsp, int count);
extern DmHelpSectPtr DmGetSection(DmHelpFilePtr hfp, char *name);
extern void DmPushHelpStack(DmHelpWinPtr hwp, char *file, char *sect_name,
	char *sect_tag);

/* help window button callbacks */
extern void DmHtextSelectCB(Widget, XtPointer, XtPointer);
extern void DmNextSectionCB(Widget, XtPointer, XtPointer);
extern void DmPrevSectionCB(Widget, XtPointer, XtPointer);
extern void DmBackTrackCB(Widget, XtPointer, XtPointer);
extern void DmTOCCB(Widget, XtPointer, XtPointer);
extern void DmGlossaryCB(Widget, XtPointer, XtPointer);
extern void DmUsingHelpCB(Widget, XtPointer, XtPointer);
extern void DmOpenHelpDeskCB(Widget, XtPointer, XtPointer);
extern void DmHelpDeskCB(Widget, XtPointer, XtPointer);
extern void DmSearchCB(Widget, XtPointer, XtPointer);
extern void DmNotesCB(Widget, XtPointer, XtPointer);
extern void DmDisplayNotes(DmHelpWinPtr hwp, DmHelpNotesPtr np);
extern void DmBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmAddBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmGoToBookmarkCB(Widget, XtPointer, XtPointer);
extern void DmHelpWinPopdownCB(Widget, XtPointer, XtPointer);
extern DmHelpNotesPtr DmGetNotesPtr(DmHelpWinPtr hwp, char *cur_tag);

/* external help desk routines */
extern void DmHDSelectProc(Widget, XtPointer, XtPointer);
extern void DmHDDblSelectProc(Widget, XtPointer, XtPointer);
extern void DmHDMenuProc(Widget, XtPointer, XtPointer);
extern void DmHDGlossaryCB(Widget, XtPointer, XtPointer);
extern void DmHDIMOpenCB(Widget, XtPointer, XtPointer);
extern void DmHDOpenCB(Widget, XtPointer, XtPointer);
extern void DmHDAlignCB(Widget, XtPointer, XtPointer);
extern int  DmDelAppFromHD(char *app_name, char *icon_label);
extern void DmHDHelpCB(Widget, XtPointer, XtPointer);
extern void DmHDHelpTOCCB(Widget, XtPointer, XtPointer);
extern void DmGetHDAppInfo(char *help_file, char **label, char **descrp);
extern int DmAddAppToHD(char *app_name, char *icon_label, char *icon_file,
	char *help_file, char *help_dir, char *fullpath);

extern void GetLocale(char **lang);
extern void SetAppName(DmHelpAppPtr hap, char **app_name);

extern void DmDestroySearchWin(Widget w, XtPointer client_data,
	XtPointer call_data);
extern void DmDestroyBmarkWin(Widget w, XtPointer client_data,
	XtPointer call_data);
extern void DmDestroyNotesWin(Widget w, XtPointer client_data,
	XtPointer call_data);
extern void DmDestroyGlossWin(Widget w, XtPointer client_data,
	XtPointer call_data);
extern void DmDestroyDefWin(Widget w, XtPointer client_data,
	XtPointer call_data);

#endif /* _help_h */
