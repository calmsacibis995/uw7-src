#pragma ident	"@(#)dtm:h_cbs.c	1.155"

/******************************file*header********************************

    Description:
     This file contains the source code for the callbacks for buttons
	in the help window.  It also contains code for the Bookmark, Notes,
	Search, Definition and Glossary windows.
*/
                              /* #includes go here     */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <libgen.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/TextGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/ListGizmo.h>
#include <MGizmo/ModalGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public Procedures
          2. Private  Procedures
*/
extern void DmShowScrolledSection(DmHelpAppPtr, char *, char *,
		char *, unsigned int, Boolean);

                         /* private procedures         */

static void GlossaryMenuCB(Widget w, XtPointer client_data,
	XtPointer call_data);
static void DisplayDef(DmHelpWinPtr, char *, char *);
static void DefMenuCB(Widget w, XtPointer client_data, XtPointer call_data);
static void NotesMenuCB(Widget w, XtPointer client_data, XtPointer call_data);
static void NotesChgCB(Widget w, XtPointer client_data, XtPointer call_data);
static void SaveNotes(DmHelpWinPtr hwp);
static void DelNotes(DmHelpWinPtr hwp);
static void FreeNotes(DmHelpNotesPtr np);
static void SearchMenuCB(Widget, XtPointer, XtPointer);
static void DoSearchAll(DmHelpWinPtr, Widget);
static void BmarkMenuCB(Widget w, XtPointer client_data, XtPointer call_data);
static void BmarkListMenuCB(Widget w, XtPointer client_data,
		XtPointer call_data);
static void AddBmark(DmHelpWinPtr hwp);
static void DelBmark(DmHelpWinPtr hwp);
static void DelAllBmark(DmHelpWinPtr hwp);
static void DoDelBmark(DmHelpAppPtr hap, DmHelpBmarkPtr bmp);
static void UpdateBmarkFile(DmHelpAppPtr hap);
static void DmReadBookmarks(DmHelpAppPtr hap);
static void FreeBmark(DmHelpBmarkPtr bmp);
static void FreeAllBmarks(DmHelpAppPtr hap);
static char *SearchStr(char *, char *, unsigned int *, unsigned int *);
static char *getnextfile(DmHelpWinPtr);
static void search_dirs(DmHelpWinPtr, char *, char *, namelistp *);
static void VaStatusPopup(Widget, int, char *, ... );

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* section tags for help manager */
#define DEFWINHELP	"50"
#define GLOSSWINHELP	"120"
#define BMARKWINHELP	"130"
#define SEARCHWINHELP	"170"
#define NOTESWINHELP	"180"

/* Gizmos for Search window */

static MenuItems searchMenuItems[] = {
  { True, TXT_Search, TXT_M_Search,    I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_CANCEL, TXT_M_CANCEL,    I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,      I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo searchMenu = {
	NULL, "searchMenu", "_X_", searchMenuItems, SearchMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 1
};

static InputGizmo searchInputGizmo = {
	NULL, "searchInputGizmo", "", 40, NULL
};

static GizmoRec labelGizmoRec[] = {
	{ InputGizmoClass, &searchInputGizmo },
};

static LabelGizmo searchLabelGizmo = {
	NULL,			/* help */
	"searchLabel",		/* widget name */
	TXT_HW_SEARCH_FOR,		/* caption label */
	False,			/* align caption */
	labelGizmoRec,		/* gizmo array */
	XtNumber(labelGizmoRec),/* number of gizmos */
};

static GizmoRec searchGizmos[] = {
	{ LabelGizmoClass, &searchLabelGizmo },
};

static PopupGizmo searchPopup = {
	NULL,			/* help */
	"searchPopup",		/* name of shell */
	"",			/* window title */
	&searchMenu,		/* pointer to menu info */
	searchGizmos,		/* gizmo array */
	XtNumber(searchGizmos),	/* number of gizmos */
};

/* Gizmos for Notes window */

static MenuItems notesMenuItems[] = {
  { True, TXT_SAVE,   TXT_M_SAVE,   I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_DELETE, TXT_M_DELETE, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo notesMenu = {
	NULL, "notesMenu", "_X_", notesMenuItems, NotesMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 2
};

static TextGizmo notesTextGizmo = {
	NULL, "notesTextGizmo", "", NULL, NULL, 10, 50
};

static GizmoRec contGizmoRec[] = {
	{ TextGizmoClass, &notesTextGizmo },
};

static Arg swArgs[] = {
	{ XmNscrollingPolicy,        XmAPPLICATION_DEFINED },
	{ XmNvisualPolicy,           XmVARIABLE },
	{ XmNscrollBarDisplayPolicy, XmSTATIC },
};

static ContainerGizmo notesContGizmo = {
	NULL,			/* help */
	"notesContGizmo",	/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	contGizmoRec,		/* gizmos */
	XtNumber(contGizmoRec),/* numGizmos */
};

static GizmoRec notesGizmos[] = {
	{ ContainerGizmoClass, &notesContGizmo, swArgs, XtNumber(swArgs) },
};

static PopupGizmo notesPopup = {
	NULL,			/* help */
	"notesPopup",		/* name of shell */
	"",			/* window title */
	&notesMenu,		/* pointer to menu info */
	notesGizmos,		/* gizmo array */
	XtNumber(notesGizmos),	/* number of gizmos */
};

/* Gizmos for Glossary window */

static MenuItems glossMenuItems[] = {
  { True, TXT_OK,   TXT_M_OK,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo glossMenu = {
	NULL, "glossMenu", "_X_", glossMenuItems, GlossaryMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 0
};

static TextGizmo glossTextGizmo = {
	NULL, "glossTextGizmo", "", NULL, NULL, 10, 70, G_RDONLY
};

static GizmoRec glossContGizmoRec[] = {
	{ TextGizmoClass, &glossTextGizmo },
};

static ContainerGizmo glossContGizmo = {
	NULL,			/* help */
	"glossContGizmo",	/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	glossContGizmoRec,		/* gizmos */
	XtNumber(glossContGizmoRec),/* numGizmos */
};

static GizmoRec glossGizmos[] = {
	{ ContainerGizmoClass, &glossContGizmo, swArgs, XtNumber(swArgs) },
};

static PopupGizmo glossPopup = {
	NULL,			/* help */
	"glossPopup",		/* name of shell */
	"",			/* window title */
	&glossMenu,		/* pointer to menu info */
	glossGizmos,		/* gizmo array */
	XtNumber(glossGizmos),	/* number of gizmos */
};

/* Gizmos for Bookmark window */

static MenuItems bmarkMenuItems[] = {
  { True, TXT_GO_TO, TXT_M_GO_TO, I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,  TXT_M_HELP,  I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo bmarkMenu = {
	NULL, "bmarkMenu", "_X_", bmarkMenuItems, BmarkMenuCB, NULL,
	XmVERTICAL, 1, 0, 1
};

static MenuItems bmarkListMenuItems[] = {
  { True, TXT_ADD,        TXT_M_ADD,    I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_DELETE,     TXT_M_DELETE, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_DELETE_ALL, TXT_M_ALL,    I_PUSH_BUTTON, NULL, NULL, NULL,
	False},
  { NULL }
};

static MenuGizmo bmarkListMenu = {
	NULL, "bmarkListMenu", "_X_", bmarkListMenuItems, BmarkListMenuCB,
	NULL, XmVERTICAL, 1, 0
};

static ListGizmo bmarkListGizmo = {
	NULL,		/* help info */
	"bmarkList",	/* name of widget */
	NULL,		/* items */
	0,		/* numItems */
	5,		/* number of items visible */
	NULL,		/* select callback */
	NULL,		/* clientData */
};

static Arg listArgs[] = {
	{ XmNscrollBarDisplayPolicy,	XmSTATIC },
	{ XmNlistSizePolicy,		XmCONSTANT },
};

static GizmoRec bmarkContGizmoRec[] = {
	{ ListGizmoClass, &bmarkListGizmo, listArgs, XtNumber(listArgs) },
};

static ContainerGizmo bmarkContGizmo = {
	NULL,			/* help */
	"bmarkContGizmo",	/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	bmarkContGizmoRec,		/* gizmos */
	XtNumber(bmarkContGizmoRec),/* numGizmos */
};

static GizmoRec bmarkLabelRec[] = {
	{ ContainerGizmoClass, &bmarkContGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo bmarkLabelGizmo = {
	NULL,			/* help */
	"bmarkLabel",		/* widget name */
	TXT_CUR_BMARK,		/* caption label */
	False,			/* align caption */
	bmarkLabelRec,		/* gizmo array */
	XtNumber(bmarkLabelRec),/* number of gizmos */
	G_TOP_LABEL,		/* label position */
};

static GizmoRec bmarkFormGizmoRec[] = {
	{ CommandMenuGizmoClass, &bmarkListMenu },
	{ LabelGizmoClass, &bmarkLabelGizmo },
};

static ContainerGizmo bmarkFormGizmo = {
	NULL,			/* help */
	"bmarkFormGizmo",	/* widget name */
	G_CONTAINER_FORM,	/* type */
	0,			/* width */
	0,			/* height */
	bmarkFormGizmoRec,	/* gizmos */
	XtNumber(bmarkFormGizmoRec),/* numGizmos */
};

static GizmoRec bmarkGizmos[] = {
	{ ContainerGizmoClass, &bmarkFormGizmo },
};

static PopupGizmo bmarkPopup = {
	NULL,			/* help */
	"bmarkPopup",		/* name of shell */
	"",			/* window title */
	&bmarkMenu,		/* pointer to menu info */
	bmarkGizmos,		/* gizmo array */
	XtNumber(bmarkGizmos),	/* number of gizmos */
};

/* Gizmos for Definition window */

static MenuItems defMenuItems[] = {
  { True, TXT_OK,   TXT_M_OK,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo defMenu = {
	NULL, "defMenu", "_X_", defMenuItems, DefMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 0
};

static TextGizmo defTextGizmo = {
	NULL, "defTextGizmo", "", NULL, NULL, 4, 60, G_RDONLY
};

static GizmoRec defContGizmoRec[] = {
	{ TextGizmoClass, &defTextGizmo },
};

static ContainerGizmo defContGizmo = {
	NULL,			/* help */
	"defContGizmo",		/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	defContGizmoRec,		/* gizmos */
	XtNumber(defContGizmoRec),/* numGizmos */
};

static GizmoRec defGizmos[] = {
	{ ContainerGizmoClass, &defContGizmo, swArgs, XtNumber(swArgs) },
};

static PopupGizmo defPopup = {
	NULL,			/* help */
	"defPopup",		/* name of shell */
	"",			/* window title */
	&defMenu,		/* pointer to menu info */
	defGizmos,		/* gizmo array */
	XtNumber(defGizmos),	/* number of gizmos */
};

static char *locale = NULL;


/***************************private*procedures****************************
 *
 *	Private Procedures
 */

/***************************public*procedures****************************

    Public Procedures
*/


/****************************procedure*header*****************************
 * Callback to jump to a section when a link is selected, and display
 * a definition window when a term is selected.
 *
 * If SCRIPT is defined, use it to look up a link or a definition.
 *
 * If a link is selected and SCRIPT is defined, then use SCRIPT as the
 * reference to the link.  In this case, SCRIPT must follow the format
 * defined for a reference to a link (i.e. filename^section_tag/name).
 *
 * If a link is selected and SCRIPT is not defined, then construct
 * a reference using KEY which assumed to be a section name.  In this
 * case, the section has to be in the current file; otherwise, SCRIPT
 * must be defined to specify a help file name.
 */
void
DmHtextSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
#define KEY	ExmHyperSegmentKey(hs)
#define TEXT	ExmHyperSegmentText(hs)
#define SCRIPT	ExmHyperSegmentScript(hs)
#define DEF	ExmHyperSegmentRV(hs)
#define SHCMD	ExmHyperSegmentShellCmd(hs)

	ExmHyperSegment *hs = (ExmHyperSegment *)call_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);

	if (SHCMD)
	{
	    /* hlp->file is overloaded to be used for help file name or a
	     * a shell command.
	     */
	    DmHelpLocPtr hlp;

	    if (SCRIPT)
	    {
		hlp = DmHelpRefToLoc(SCRIPT);

	    } else
	    {
		char buf[256];
		sprintf(buf, "^%s", KEY); 
		hlp = DmHelpRefToLoc(buf);
	    }
	    if (hlp && (hlp->file || hlp->sect_tag))
	    {
		if (DmExecuteShellCmd((DmWinPtr)hwp, NULL, hlp->file, 0) != 1)
		    DmVaDisplayStatus((DmWinPtr)hwp, True,
				      TXT_CANT_EXEC_COMMAND, KEY, hlp->file);

	    } else
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_SHELL_CMD);

	} else if (DEF != False) { /* term selected */
		char *ref = NULL;

		if (SCRIPT) {
			if (!(ref = DtGetProperty(&(hwp->hsp->defs), SCRIPT,
			  NULL)) && !(ref = DtGetProperty(&(hwp->hfp->defs),
			  SCRIPT, NULL)))
			{
					DmVaDisplayStatus((DmWinPtr)hwp, True,
						TXT_NO_HELP_DEF, KEY);
					return;
			}
		} else {
		    if (!(ref = DtGetProperty(&(hwp->hsp->defs), KEY, NULL)) &&
			!(ref = DtGetProperty(&(hwp->hfp->defs), KEY, NULL)))
			{
					DmVaDisplayStatus((DmWinPtr)hwp, True,
						TXT_NO_HELP_DEF, KEY);
					return;
			}
		}
		DisplayDef(hwp, (char *)KEY, ref);
	} else { /* link selected */
		DmHelpAppPtr hap;
		DmHelpLocPtr hlp;

		if (SCRIPT) {
			hlp = DmHelpRefToLoc(SCRIPT);
		} else {
			char buf[256];
			sprintf(buf, "^%s", KEY); 
			hlp = DmHelpRefToLoc(buf);
		}
		if (hlp == NULL ||(hlp->file == NULL && hlp->sect_tag == NULL))
		{
			DmVaDisplayStatus((DmWinPtr)hwp,True,TXT_NO_HELP_LINK);
			return;
		}
		hap = DmGetHelpApp(hwp->app_id);
		DmDisplayHelpSection(hap, NULL, hlp->file, hlp->sect_tag);

		/* NOTE: hlp->file and hlp->sect_tag are freed in
		 * DmHelpRefToLoc().
		 */
	}
#undef KEY
#undef TEXT
#undef SCRIPT
}	/* end of DmHtextSelectCB */


/****************************procedure*header*****************************
 * This callback displays the next section in a help file
 */
void
DmNextSectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->hsp && ((int)(hwp->hsp - hwp->hfp->sections) <
		(int)(hwp->hfp->num_sections - 1)))
	{
		DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

		/* Set this to False in case some text was entered but
		 * not saved.
		 */
		hwp->hsp->notes_chged = False;

		/* display the next section */
		DmDisplayHelpSection(hap, hwp->hfp->title, NULL,
			((hwp->hsp + 1)->tag ? (hwp->hsp + 1)->tag :
			(hwp->hsp + 1)->name));
	}
}	/* end of DmNextSectionCB */


/****************************procedure*header*****************************
 * This callback displays the previous section in a help file
 */
void
DmPrevSectionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->hsp && (hwp->hsp != hwp->hfp->sections))
	{
		DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

		/* Set this to False in case some text was entered but
		 * not saved.
		 */
		hwp->hsp->notes_chged = False;

		/* display the previous section */
		DmDisplayHelpSection(hap, hwp->hfp->title, NULL,
			((hwp->hsp - 1)->tag ? (hwp->hsp - 1)->tag :
			(hwp->hsp - 1)->name));
	}
}	/* end of DmPrevSectionCB */


/****************************procedure*header*****************************
 * This callback switches the view to the previous view at the top of
 * the history stack.
 */
void
DmBackTrackCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	/* Set this to False in case some text was entered but not saved. */
	if (hwp->hsp && hwp->hsp->notes_chged == True)
		hwp->hsp->notes_chged = False;

	DmClearStatus((DmWinPtr)hwp);
	DmPopHelpStack(w, hwp);

}	/* end of DmBackTrackCB */

/****************************procedure*header*****************************
 * Displays the table of contents for a given help file. A dollar sign ($)
 * is used as a delimiter in a link.
 */
void
DmTOCCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget btn;
	DmHelpAppPtr hap;
	char title[256];
	char *file = NULL;
	char *sect_name;
	char *sect_tag;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	/*
	 * If called from Go To menu, simply return if TOC is already
	 * being displayed.  The same check is made in DmDisplayHelpTOC()
	 * for when this routine is not called from the Go To menu.
	 */
	if (hwp->hsp && call_data) {
		/* called from Go To menu */ 
		if (strcmp(hwp->hsp->name, TABLE_OF_CONTENTS) == 0)
			return;
		else {
			file = strdup(hwp->hfp->name);
			if (hwp->hsp->name)
				sect_name = strdup(hwp->hsp->name);
			else
				sect_name = NULL;

			if (hwp->hsp->tag)
				sect_tag = strdup(hwp->hsp->tag);
			else
				sect_tag = NULL;
		}
	}
	hap = DmGetHelpApp(hwp->app_id);

	/* deactivate Bookmark, Notes windows */
	DmChgHelpWinBtnState(hwp, True, False);

	/* Deactivate Next Topic, Previous Topic, Bookmark, Notes & TOC */
	XtSetArg(Dm__arg[0], XtNsensitive, False);
	btn = QueryGizmo(PopupGizmoClass, hwp->gizmo_shell, GetGizmoWidget,
		"BtnBarMenu:1");
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(PopupGizmoClass, hwp->gizmo_shell, GetGizmoWidget,
		"BtnBarMenu:2");
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(PopupGizmoClass, hwp->gizmo_shell, GetGizmoWidget,
		"BtnBarMenu:4");
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(PopupGizmoClass, hwp->gizmo_shell, GetGizmoWidget,
		"BtnBarMenu:5");
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(PopupGizmoClass, hwp->gizmo_shell, GetGizmoWidget,
		"BtnBarMenu:6");
	XtSetValues(btn, Dm__arg, 1);

	sprintf(title, "%s %s: %s", hap->title, help_str, toc_title);

	XtSetArg(Dm__arg[0], XtNtitle, title);
	XtSetValues(hwp->shell, Dm__arg, 1);
	DmClearStatus((DmWinPtr)hwp);

	if (hwp->hfp->toc == NULL) {
		/* create table of contents section */
		DmHelpSectPtr hsp;
		int level1, level2, level3, level4, level5, level6;
		int sect_cnt;
		int i;
		int bufsz = 0;
		char *buf;
		char *savebuf;

		sect_cnt = hwp->hfp->num_sections;

		/* get size of buffer */
		hsp = hwp->hfp->sections;

		for (i = 0; i < sect_cnt; i++, hsp++) { 
			bufsz += strlen(hsp->name) + (hsp->alias ?
				strlen(hsp->alias) : strlen(hsp->name)) + 17;
		}
		bufsz += strlen(toc_title);

		hsp = hwp->hfp->sections;
		level1 = 0;
		for (; sect_cnt; sect_cnt--, hsp++) {
			if (hsp->level == 0)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 22;

			else if (hsp->level == 1)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 27;

			else if (hsp->level == 2)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 30;

			else if (hsp->level == 3)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 40;

			else if (hsp->level == 4)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 50;

			else if (hsp->level == 5)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 70;

			else if (hsp->level == 6)
				bufsz += strlen(hsp->name) + (hsp->alias ?
					strlen(hsp->alias)  : 0) + 80;
		}  
		hsp = hwp->hfp->sections;
		sect_cnt = hwp->hfp->num_sections;

		buf = (char *)MALLOC((bufsz+1) * sizeof(char));
		savebuf = buf;

		/* set up global links using section names and aliases */
		for (i = 0; i < sect_cnt; i++, hsp++) { 
			buf += sprintf(buf, "^""%%""%s^^%s\n", hsp->name,
					hsp->alias ? hsp->alias : hsp->name);
		}
		buf += sprintf(buf, "%s", toc_title);

		hsp = hwp->hfp->sections;
		level1 = 0;
		for (; sect_cnt; sect_cnt--, hsp++) {
			if (hsp->level == 0) {
				/* do nothing */
				if (hsp->alias)
					buf += sprintf(buf, "\n ""\\k""$%s^^%s$\n",
							hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "\n ""\\k""$%s$\n", hsp->name);

			} else if (hsp->level == 1) {
				/* reset all sublevels to 0 */
				level2 = level3 = level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf, "\n %d. ""\\k""$%s^^%s$\n",
							++level1, hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "\n %d. ""\\k""$%s$\n",
							++level1, hsp->name);

			} else if (hsp->level == 2) {
				level3 = level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf, "     %d.%d. \\k$%s^^%s$\n",
							level1, ++level2, hsp->name, hsp->alias);
				else
					buf += sprintf(buf, "     %d.%d. \\k$%s$\n",
							level1, ++level2, hsp->name);

			} else if (hsp->level == 3) {
				level4 = level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
							"          %d.%d.%d. \\k$%s^^%s$\n",
							level1, level2, ++level3, hsp->name,
							hsp->alias);
				else
					buf += sprintf(buf, "          %d.%d.%d. \\k$%s$\n",
							level1, level2, ++level3, hsp->name);

			} else if (hsp->level == 4) {
				level5 = level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
					"                 %d.%d.%d.%d. \\k$%s^^%s$\n",
							level1, level2, level3,
							++level4, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
							"                 %d.%d.%d.%d. \\k$%s$\n",
							level1, level2, level3,
							++level4, hsp->name);

			} else if (hsp->level == 5) {
				level6 = 0;
				if (hsp->alias)
					buf += sprintf(buf,
						"                          %d.%d.%d.%d.%d. "
						"\\k$%s^^%s$\n", level1, level2, level3, level4,
						++level5, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
						"                          %d.%d.%d.%d.%d. "
						"\\k$%s$\n", level1, level2, level3, level4,
						++level5, hsp->name);

			} else if (hsp->level == 6) {
				if (hsp->alias)
					buf += sprintf(buf,
						"                            %d.%d.%d.%d.%d.%d. "
						"\\k$%s^^%s$\n", level1, level2, level3, level4,
						level5, ++level6, hsp->name, hsp->alias);
				else
					buf += sprintf(buf,
						"                            %d.%d.%d.%d.%d.%d. "
						"\\k$%s$\n", level1, level2, level3, level4,
						level5, ++level6, hsp->name);
			}
		}

		hsp = (DmHelpSectPtr)CALLOC(1, sizeof(DmHelpSectRec));
		/* initialize the table of contents section structure */
		hsp->name           = strdup(TABLE_OF_CONTENTS);
		/* comment from code review - this should be cooked_data??? */ 
		hsp->raw_data       = savebuf;
		hsp->raw_size       = strlen(savebuf);
		hwp->hfp->toc       = hsp;
	}
	/* comment from code review - this is not needed if do away
		with loop in line 413?
	*/
	if (hwp->hfp->toc->cooked_data == NULL)
		/* cook section first */
		DmProcessHelpSection(hwp->hfp->toc);

	XtSetArg(Dm__arg[0], XtNstring, hwp->hfp->toc->cooked_data);
	XtSetValues(hwp->htext, Dm__arg, 1);

	/*
	 * Push current section onto stack here if there is a current
	 * section and this was called from the Go To menu.
	 */ 
	if (file) {
		DmPushHelpStack(hwp, file, sect_name, sect_tag);
		FREE((void *)file);
		if (sect_name)
			FREE((void *)sect_name);
		if (sect_tag)
			FREE((void *)sect_tag);
	}
	/* set current section */
	hwp->hsp = hwp->hfp->toc;

} /* end of DmTOCCB */

/****************************procedure*header*****************************
 * Displays the definition of all terms in a help file in a popup window.
 */
void
DmGlossaryCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Gizmo textGiz;
	int cnt = 0;
	int bufsz = 0;
	DtPropPtr pp;
	char *gbuf;
	char *savegbuf;
	Widget shell;
	DmHelpAppPtr hap;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->glossWin) {
		MapGizmo(PopupGizmoClass, hwp->glossWin);
		return;
	}
	cnt = hwp->hfp->defs.count;
	if (cnt == 0) {
		/* no definitions */
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_GLOSSARY);
		return;
	}
	glossPopup.menu->clientData = (XtPointer)hwp;
	hwp->glossWin = CreateGizmo(hwp->shell, PopupGizmoClass, &glossPopup,
			NULL, 0);
	shell = GetPopupGizmoShell(hwp->glossWin);

	hap = DmGetHelpApp(hwp->app_id);
	sprintf(Dm__buffer, "%s: %s", hap->title, Dm__gettxt(TXT_GLOSSARY));
	XtSetArg(Dm__arg[0], XmNtitle, Dm__buffer);
	XtSetValues(shell, Dm__arg, 1);

	XtAddCallback(shell, XtNpopdownCallback, DmDestroyGlossWin,
		(XtPointer)hwp);

	pp = hwp->hfp->defs.ptr;
	for (cnt = hwp->hfp->defs.count; cnt; cnt--, pp++)
		/* account for newlines */
		bufsz += strlen(pp->name) + strlen(pp->value) + 6;

	gbuf = (char *)MALLOC((bufsz+1) * sizeof(char));
	savegbuf = gbuf;
	pp = hwp->hfp->defs.ptr;
	for (cnt = hwp->hfp->defs.count; cnt; cnt--, pp++)
		gbuf += sprintf(gbuf, "\"%s\"\n%s\n\n", pp->name, pp->value);

	textGiz = QueryGizmo(PopupGizmoClass, hwp->glossWin,
			GetGizmoGizmo, "glossTextGizmo");
	SetTextGizmoText(textGiz, savegbuf);
	
	/* register callback for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(hwp->glossWin),
		hwp->app_id, HELPMGR_HELP_FILE, GLOSSWINHELP);

	MapGizmo(PopupGizmoClass, hwp->glossWin);

}	/* end of DmGlossaryCB */

/****************************procedure*header*****************************
 * Popup a prompt window for for string to search for and calls DoSearch()
 * to find the string in the current help file.
 */
void
DmSearchCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget shell;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->searchWin) {
		MapGizmo(PopupGizmoClass, hwp->searchWin);
		return;
	}
	searchPopup.menu->clientData = (XtPointer)hwp;
	hwp->searchWin = CreateGizmo(hwp->shell, PopupGizmoClass, &searchPopup,
			NULL, 0);
	shell = GetPopupGizmoShell(hwp->searchWin);

	sprintf(Dm__buffer, "%s: %s", help_str, Dm__gettxt(TXT_SEARCH));

	XtSetArg(Dm__arg[0], XmNtitle, Dm__buffer);
	XtSetValues(shell, Dm__arg, 1);

	XtAddCallback(shell, XtNpopdownCallback, DmDestroySearchWin,
		(XtPointer)hwp);

	/* register for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(hwp->searchWin),
		hwp->app_id, HELPMGR_HELP_FILE, SEARCHWINHELP);

	MapGizmo(PopupGizmoClass, hwp->searchWin);

}	/* end of DmSearchCB */

/****************************procedure*header*****************************
 * SearchMenuCB -
 */
static void
SearchMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget shell;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Search */
		shell = GetPopupGizmoShell(hwp->searchWin);
		DmBusyWindow(shell, True);
		DoSearchAll(hwp, shell);
		DmBusyWindow(shell, False);
		break;
	case 1: /* Close */
		XtPopdown(GetPopupGizmoShell(hwp->searchWin));
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
			HELPMGR_HELP_FILE, SEARCHWINHELP);
		break;
	}
} /* end of SearchMenuCB */

/****************************procedure*header*****************************
 * Searches for a user-specified string in the current help file and
 * displays the section it is contained in in the help window.
 * A search always begins at the beginning of a file and a message is
 * displayed when the search reaches the end of a file.
 * "hsp->matched" is set to True for a section which has been "matched";
 * and is reset to False if the search is repeated for the same file.
 * Sections for which hsp->matched is True are skipped in a search.
 */
static void
DoSearchAll(DmHelpWinPtr hwp, Widget searchWin)
{
	DmHelpFilePtr hfp;
	DmHelpSectPtr hsp;
	DmHelpAppPtr hap;
	int  cnt;
	int  str_len;
	char *newfile;
	char *p, *q, *savep;
	char *str;
	char *str_save = NULL;
	Boolean new_sect = False, openfile = False;
	Gizmo inputGiz = QueryGizmo(PopupGizmoClass, hwp->searchWin,
				    GetGizmoGizmo, "searchInputGizmo");

	str = GetInputGizmoText(inputGiz);
	/* remove all the leading space */
	if (str) {
		str_save = str;
		while (*str == ' ') str++;
	}

	/*
	 * A check is added here for hwp->hfp to workaround not
	 * deactivating the Search window if it's up when a string
	 * is displayed in the help window. This check should be removed
	 * in the next release and the buttons in the Search window
	 * should be made insensitive when a string is displayed.
	 */
	if (hwp->hfp == NULL || str == NULL || *str == '\0') {
	    if (str_save)
		FREE(str_save);
	    VaStatusPopup(searchWin, XmDIALOG_ERROR, TXT_NO_SEARCH);
	    return;
	}

	/* convert search string to lower case */
	for (q = str; *q != NULL; ++q)
		*q = (char)tolower((int)((unsigned char)*q));
	str_len = strlen(str);

	hfp = hwp->hfp;
	hsp = hwp->hsp;

	hap = DmGetHelpApp(hwp->app_id);

	if (hwp->search_str && strcmp(hwp->search_str, str) == 0) {

		/* Repeat search with the same search string. */

		/* After backtrack, continue search in the next file. */
		if (hwp->help_files != NULL &&
		    strcmp(hfp->name, hwp->filep->name) != 0) {
			newfile = hwp->filep->name;
			new_sect = True;
			goto process_file;
		}

	} else {
		DmHelpSectPtr tmphsp;

		/* save the new search string */
		if (hwp->search_str)
			FREE(hwp->search_str);
		hwp->search_str = strdup(str);

		hwp->found = False;

		/* change search string in other help file context,
		 * go back to the beginning of the file list to restart search
		 */
		if (hwp->help_files != NULL) {
			hwp->filep = hwp->help_files;
			newfile = hwp->filep->name;
			new_sect = True;
			goto process_file;
		}

new_file:
		if (hwp->search_file)
			FREE(hwp->search_file);
		hwp->search_file = strdup(hfp->name);
		hwp->linecnt = hwp->index = 0;
		for (cnt = hfp->num_sections,
		     tmphsp = hfp->sections; cnt--;)
			tmphsp++->matched = False;
	}

	/* searching through the whole file */
	while (!hsp->matched) {

		DmProcessHelpSection(hsp);

		/* convert section to lower case */
		for (p = savep = strdup(hsp->cooked_data); *p != NULL; ++p)
			*p = (char)tolower((int)*p);

		if (SearchStr(savep, str, &hwp->linecnt, &hwp->index) != NULL) {
			/* found string in section */
			hwp->found = True;

			if (new_sect) {
				DmShowScrolledSection(hap, hfp->title,
			     	 hfp->name, hsp->name, hwp->linecnt + 1, False);

				/*
				 * Eliminate the extra help file open count
				 * if the help file is opened for search before
				 * it is displayed in Help window.
				 */
				if (openfile)
					DmCloseHelpFile(hfp);
			} else {
				XtSetArg(Dm__arg[0], XmNtopRow,
					 hwp->linecnt + 1);
				XtSetValues(hwp->htext, Dm__arg, 1);
			}

			hwp->index += str_len;
			FREE((void *)savep);
			FREE((void *)str_save);
			return;
		}

		hsp->matched = True;
		hwp->index = hwp->linecnt = 0;
		FREE((void *)savep);

		/* if last section, reset hsp to the top of the file */ 
		if ((int)(hsp - hfp->sections) == (int)(hfp->num_sections - 1))
			hsp = hfp->sections;
		else
			++hsp;
			new_sect = True;
	}

	/* If no match in the current search file, release it. */
	if (strcmp(hwp->hfp->name, hfp->name) != 0)
		DmCloseHelpFile(hfp);

	/* Get the next help file to search */
	while ((newfile = getnextfile(hwp)) != NULL) {
process_file:
		if ((hfp = DmOpenHelpFile(hap, newfile)) != NULL) {
			openfile = True;
			hsp = hfp->sections;
			goto new_file;
		}
	}

	if (hwp->found) {
		/* reach the end of the help file list */
		VaStatusPopup(searchWin, XmDIALOG_INFORMATION,
			      TXT_SEARCH_THROUGH, hwp->search_str);
	} else {
		/* no match found */
		VaStatusPopup(searchWin, XmDIALOG_WARNING, TXT_NO_MATCH);
	}
	hwp->filep = NULL;
	if (str_save)
		FREE((void *)str_save);
	if (hwp->search_str) {
		FREE(hwp->search_str);
		hwp->search_str = NULL;
	}
}	/* end of DoSearchAll */

static char *
getnextfile(DmHelpWinPtr hwp)
{
	Boolean	first = True;
	char *curdir, *curfile, *dir_name, *path;
	DIR *dirp;
	struct dirent *entp;
	struct stat buf;
	namelistp tmpname, lastone;
	int len;

	if (hwp->help_files == NULL) {
		path = strdup(hwp->search_file);
		curfile = basename(path);
		curdir = dirname(path);
		len = strlen(curdir) + sizeof(namelistp) + 6;
		dirp = opendir(curdir);
		while ((entp = readdir(dirp)) != NULL) {
			char *suffix;

			if (entp->d_name[0] == '.')
				continue;
		        tmpname = (namelistp)malloc(strlen(entp->d_name) + len);
		        strcpy(tmpname->name, curdir);
		        strcat(tmpname->name, "/");
		        strcat(tmpname->name, entp->d_name);
			if (stat(tmpname->name, &buf) != 0) {
				FREE(tmpname);
				continue;
			}
			if (buf.st_mode & S_IFDIR) {
				search_dirs(hwp, tmpname->name, NULL, &lastone);
				FREE(tmpname);
				continue;
			}
			if (!(buf.st_mode & S_IFREG)) {
				FREE(tmpname);
				continue;
			}
			suffix = strrchr(entp->d_name, '.');
			if (strcmp("hlp", ++suffix) != 0) {
				FREE(tmpname);
				continue;
			}

			if (first) {
				tmpname->next = NULL;
		    		hwp->help_files = tmpname;
				lastone = tmpname;
				first = False;
			} else {
				if (strcmp(tmpname->name, hwp->search_file) == 0) {
					tmpname->next = hwp->help_files;
					hwp->help_files = tmpname;
				} else {
					tmpname->next = NULL;
					lastone->next = tmpname;
					lastone = tmpname;
				}
			}
		}
		closedir(dirp);

		/* search for files in all the other dirs */
		dir_name = basename(curdir);
		while (strcmp(dir_name, "help") != 0) {
			char *parent;

			parent = dirname(curdir);
			search_dirs(hwp, parent, dir_name, &lastone);
			curdir = parent;
			dir_name = basename(curdir);
		}

		FREE(path);
		hwp->filep = hwp->help_files;

	}
	/* Search or backtrack to the beginning. */
	if (hwp->filep == NULL) {
		hwp->filep = hwp->help_files;
		return (hwp->filep->name);
	}
	if (hwp->filep->next != NULL) {
		hwp->filep = hwp->filep->next;
		return (hwp->filep->name);
	} else
		return (NULL);
}

static void
search_dirs(DmHelpWinPtr hwp, char *dir, char *current, namelistp *last)
{
	int	len;
	DIR	*dirp;
	struct	dirent *entp;
	struct	stat buf;
	namelistp tmpname;

	len = strlen(dir) + sizeof(namelistp) + 6;
	dirp = opendir(dir);
	while ((entp = readdir(dirp)) != NULL) {
		char *suffix;

		if (entp->d_name[0] == '.' ||
		    (current != NULL && strcmp(current, entp->d_name) == 0))
			continue;
	
	        tmpname = (namelist *)malloc(strlen(entp->d_name) + len);
	        strcpy(tmpname->name, dir);
	        strcat(tmpname->name, "/");
	        strcat(tmpname->name, entp->d_name);
		if (stat(tmpname->name, &buf) != 0) {
			FREE(tmpname);
			continue;
		}
		if (buf.st_mode & S_IFDIR) {
			search_dirs(hwp, tmpname->name, NULL, last);
			FREE(tmpname);
			continue;
		}
		if (!(buf.st_mode & S_IFREG)) {
			FREE(tmpname);
			continue;
		}
		suffix = strrchr(entp->d_name, '.');
		if (strcmp("hlp", ++suffix) != 0) {
			FREE(tmpname);
			continue;
		}

		tmpname->next = NULL;
		(*last)->next = tmpname;
		(*last) = tmpname;
	}
	closedir(dirp);
}

/****************************procedure*header*****************************
 * Displays the bookmark popup window to allow adding, deleting, and
 * jumping to a section associated to a bookmark.
 */
void
DmBookmarkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int n;
	Widget shell;
	Widget w1, w2;
	char *items[50];
	DmHelpAppPtr hap;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->bmarkWin) {
		MapGizmo(PopupGizmoClass, hwp->bmarkWin);
		return;
	}
	hap = DmGetHelpApp(hwp->app_id);
	if (hap->bmp == NULL) {
		DmReadBookmarks(hap);
	}

	if (hap->num_bmark > 0) {
		DmHelpBmarkPtr bmp;

		n = 0;
		for (bmp = hap->bmp; bmp; bmp = bmp->next) {
			items[n++] = STRDUP(bmp->sect_name);
		}
	}
	bmarkListGizmo.items = items;
	bmarkListGizmo.numItems = hap->num_bmark;
	bmarkListMenu.clientData = (XtPointer)hwp;
	bmarkPopup.menu->clientData = (XtPointer)hwp;

	hwp->bmarkWin = CreateGizmo(hwp->shell, PopupGizmoClass, &bmarkPopup,
			NULL, 0);
	shell = GetPopupGizmoShell(hwp->bmarkWin);

	sprintf(Dm__buffer, "%s: %s", hap->title, Dm__gettxt(TXT_BOOKMARK));
	XtSetArg(Dm__arg[0], XmNtitle, Dm__buffer);
	XtSetValues(shell, Dm__arg, 1);

	XtAddCallback(shell, XtNpopdownCallback, DmDestroyBmarkWin,
		(XtPointer)hwp);

	for (n = 0; n < hap->num_bmark; n++)
		FREE(items[n]);

	w1 = QueryGizmo(PopupGizmoClass, hwp->bmarkWin, GetGizmoWidget,
		"bmarkListMenu");
	w2 = QueryGizmo(PopupGizmoClass, hwp->bmarkWin, GetGizmoWidget,
		"bmarkLabel");

	n = 0;
	XtSetArg(Dm__arg[n], XmNalignment, XmALIGNMENT_CENTER); n++;
	XtSetValues(w2, Dm__arg, n);

	n = 0;
	XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(Dm__arg[n], XmNleftWidget, w1); n++;
	XtSetValues(XtParent(w2), Dm__arg, n);

	/* register callback for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(hwp->bmarkWin),
		hwp->app_id, HELPMGR_HELP_FILE, BMARKWINHELP);

	MapGizmo(PopupGizmoClass, hwp->bmarkWin);

}	/* end of DmBookmarkCB */

/****************************procedure*header*****************************
 * Adds a bookmark for the current section, if one does not already exist.
 */
static void
AddBmark(DmHelpWinPtr hwp)
{
	XmString item;
	Widget listW;
	char *app_name;
	DmHelpBmarkPtr bmp;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	SetAppName(hap, &app_name);

	/* Check if bookmark already exists for the current section.
	 * Must check if sect_tag is set.
	 */
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		if (strcmp(bmp->app_name, app_name) == 0 &&
		    strcmp(bmp->file_name, hwp->hfp->name) == 0 &&
		    strcmp(bmp->sect_name, hwp->hsp->name) == 0 &&
			(bmp->sect_tag && hwp->hsp->tag &&
		    strcmp(bmp->sect_tag, hwp->hsp->tag) == 0)) {
				DmVaDisplayStatus((DmWinPtr)hwp, True,
					TXT_BMARK_EXISTS);
				return;
			}
	}
	bmp = (DmHelpBmarkPtr)CALLOC(1, sizeof(DmHelpBmarkRec));
	bmp->app_name  = (XtPointer)strdup(app_name);
	bmp->file_name = (XtPointer)strdup(hwp->hfp->name);

	if (hwp->hsp->name)
		bmp->sect_name = (XtPointer)strdup(hwp->hsp->name);

	if (hwp->hsp->tag)
		bmp->sect_tag = (XtPointer)strdup(hwp->hsp->tag);

	listW = QueryGizmo(PopupGizmoClass, hwp->bmarkWin, GetGizmoWidget,
			"bmarkList");

	/* bmp->sect_name should not be NULL */
	item = XmStringCreateLocalized(bmp->sect_name);

	XmListAddItemUnselected(listW, item, hap->num_bmark);
	XmStringFree(item);

	/* add bmp to the list */
	bmp->next = hap->bmp;
	hap->bmp = bmp;
	hap->num_bmark++;

	/* update bookmark file */
	UpdateBmarkFile(hap);

}	/* end of AddBmark */

/****************************procedure*header*****************************
 * Updates the bookmark file.
 */
static void
UpdateBmarkFile(DmHelpAppPtr hap)
{
	int fd;
	FILE *file;
	DmHelpBmarkPtr bmp;
	struct stat hstat;

	/* create bookmark directory if it doesn't already exist */ 
	if (locale == NULL)
		locale = setlocale(LC_MESSAGES, NULL);
	/* Save bookmarks separately for the different modules of dtm. */
	sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s", DESKTOP_DIR(Desktop), locale,
		(strcmp(hap->name, dtmgr_title) == 0) ? hap->title : hap->name);

	if (stat(Dm__buffer, &hstat) != 0) {
		/* create app directory */
		if (mkdirp(Dm__buffer, DESKTOP_UMASK(Desktop)) == -1) {
			Dm__VaPrintMsg(TXT_MKDIR, Dm__buffer);
			return;
		}

		/* create bookmark file */
		strcat(Dm__buffer, "/.bookmark");
		if (creat(Dm__buffer, DESKTOP_UMASK(Desktop) &
		    ~(S_IXUSR | S_IXGRP | S_IXOTH)) == -1) {
			Dm__VaPrintMsg(TXT_TOUCH, Dm__buffer);
			return;
		}
	} else {
		/* create bookmark file if it does not exist */
			sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s/.bookmark",
				DESKTOP_DIR(Desktop), locale,
				(strcmp(hap->name, dtmgr_title) == 0) ?
				hap->title : hap->name);

		if (stat(Dm__buffer, &hstat) != 0) {
			if (creat(Dm__buffer, DESKTOP_UMASK(Desktop) &
			    ~(S_IXUSR | S_IXGRP | S_IXOTH)) == -1) {
				Dm__VaPrintMsg(TXT_TOUCH, Dm__buffer);
				return;
			}
		}
	}
	if ((fd = open(Dm__buffer, O_WRONLY | O_TRUNC | O_CREAT,
			 (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
			  S_IROTH | S_IWOTH | S_ISGID))) == -1)
	{
		Dm__VaPrintMsg(TXT_CANT_ACCESS_BMARK_FILE, Dm__buffer);
		return;
	}
	if (lockf(fd, F_LOCK, 0L) == -1) {
err:
		Dm__VaPrintMsg(TXT_LOCK_FILE, Dm__buffer, errno);
		close(fd);
		return;
	} 
	if ((file = fdopen(fd, "w")) == NULL) {
		goto err;
	}
	/* write bookmarks to disk */
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		fprintf(file, "%s^%s^%s^%s^\n", bmp->app_name,
			bmp->file_name, (bmp->sect_tag ?
			bmp->sect_tag : ""), bmp->sect_name);
	}
	(void)fclose(file);

}	/* end of UpdateBmarkFile */

/****************************procedure*header*****************************
 * Jump to a selected bookmark.
 */
static void
BtnGoToBmark(DmHelpWinPtr hwp)
{
	int *pos;
	int cnt;
	Widget listW;
	char *sect;
	XmString *items;
	DmHelpBmarkPtr bmp;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);

	/* FIX - Go To button should be insensitive if no bookmark or
	 * none selected
	 */
	if (hap->num_bmark == 0)
		return;

	/* find out which bookmark is selected */
	listW = QueryGizmo(PopupGizmoClass, hwp->bmarkWin, GetGizmoWidget,
			"bmarkList");

	if (!XmListGetSelectedPos(listW, &pos, &cnt)) {
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_BMARK_TO_GOTO);
		return;
	}
	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtGetValues(listW, Dm__arg, 1);

	/* the first item is at position 1, not 0 */
	sect = GET_TEXT(items[pos[0]-1]);
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		if (!strcmp(sect, bmp->sect_name)) {
			DmDisplayHelpSection(hap, NULL,
				bmp->file_name, (bmp->sect_tag ?
				bmp->sect_tag : bmp->sect_name));
			BringDownPopup(GetPopupGizmoShell(hwp->bmarkWin));
			return;
		}
	}

}	/* end of BtnGoToBmarkCB */

/****************************procedure*header*****************************
 * DelBmark - Deletes the first selected bookmark in the list.
 */
static void
DelBmark(DmHelpWinPtr hwp)
{
	int *pos;
	int cnt;
	XmString *items;
	char *sect;
	Widget listW;
	DmHelpBmarkPtr bmp;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);

	/* FIX: Delete button should be insensitive if no bookmark or if
	 * no item is selected
	 */
	if (hap->num_bmark == 0)
		return;

	/* find out which bookmark is selected */
	listW = QueryGizmo(PopupGizmoClass, hwp->bmarkWin, GetGizmoWidget,
			"bmarkList");

	if (!XmListGetSelectedPos(listW, &pos, &cnt)) {
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_BMARK_TO_DELETE);
		return;
	}
	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtGetValues(listW, Dm__arg, 1);

	/* the first item is at position 1, not 0 */
	sect = GET_TEXT(items[pos[0]-1]);
	for (bmp = hap->bmp; bmp; bmp = bmp->next) {
		if (!strcmp(sect, bmp->sect_name)) {
			DoDelBmark(hap, bmp);
			XmListDeletePos(listW, pos[0]);
			break;
		}
	}
	free(pos);

}	/* end of DelBmark */

/****************************procedure*header*****************************
 * Deletes all bookmarks if bmp is NULL; otherwise, delete bmp.
 */
static void
DoDelBmark(DmHelpAppPtr hap, DmHelpBmarkPtr bmp)
{
	if (!bmp) {
		/* free bookmark resources */
		FreeAllBmarks(hap);
		UpdateBmarkFile(hap);
	} else {
		register DmHelpBmarkPtr	tbmp = hap->bmp;

		if (tbmp == bmp) {
			hap->bmp = bmp->next;
			FreeBmark(bmp);
			UpdateBmarkFile(hap);
			hap->num_bmark--;
		} else {
			for (; tbmp->next; tbmp = tbmp->next) {
				if (tbmp->next == bmp) {
					tbmp->next = bmp->next;
					/* free bookmark resources */
					FreeBmark(bmp);

					/* update bookmark file */
					UpdateBmarkFile(hap);
					hap->num_bmark--;
					return;
				}
			}
		}
	}
}	/* end of DoDelBmark */
			
/****************************procedure*header*****************************
 * Deletes all existing bookmarks for the current help file.
 */
static void
DelAllBmark(DmHelpWinPtr hwp)
{
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);
	Widget listW = QueryGizmo(PopupGizmoClass, hwp->bmarkWin,
			GetGizmoWidget, "bmarkList");

	DmClearStatus((DmWinPtr)hwp);
	DoDelBmark(hap, NULL);
	XmListDeleteAllItems(listW);

}	/* end of DelAllBmark */

/****************************procedure*header*****************************
 * Free all bookmark resources.
 */
static void
FreeAllBmarks(DmHelpAppPtr hap)
{
	register DmHelpBmarkPtr save;
	register DmHelpBmarkPtr bmp = hap->bmp;

	while (bmp) {
		save = bmp->next;
		FreeBmark(bmp);
		bmp = save;
	}
	hap->bmp = NULL;
	hap->num_bmark = 0;

}	/* end of FreeAllBmarks */

/****************************procedure*header*****************************
 * Free bookmark resources associated with bmp.
 */
static void
FreeBmark(DmHelpBmarkPtr bmp)
{
	FREE((void *)(bmp->app_name));
	FREE((void *)(bmp->file_name));
	FREE((void *)(bmp->sect_name));
	FREE((void *)(bmp->sect_tag));
	FREE((void *)bmp);

}	/* end of FreeBmark */

/****************************procedure*header*****************************
 * Pops up the Notes window.
 */
void
DmNotesCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget shell;
	Widget text;
	DmHelpAppPtr hap;
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->notesWin) {
		/* if current section has notes, display it */
		DmDisplayNotes(hwp,  DmGetNotesPtr(hwp, hwp->hsp->tag ?
			hwp->hsp->tag : hwp->hsp->name));
		MapGizmo(PopupGizmoClass, hwp->notesWin);
		return;
	}
	notesPopup.menu->clientData = (XtPointer)hwp;
	hwp->notesWin = CreateGizmo(hwp->shell, PopupGizmoClass, &notesPopup,
			NULL, 0);
	shell = GetPopupGizmoShell(hwp->notesWin);

	hap = DmGetHelpApp(hwp->app_id);
	sprintf(Dm__buffer, "%s: %s", hap->title, Dm__gettxt(TXT_NOTES));
	XtSetArg(Dm__arg[0], XmNtitle, Dm__buffer);
	XtSetValues(shell, Dm__arg, 1);

	XtAddCallback(shell, XtNpopdownCallback, DmDestroyNotesWin,
		(XtPointer)hwp);

	text = QueryGizmo(PopupGizmoClass, hwp->notesWin,
		GetGizmoWidget, "notesTextGizmo");

	XtAddCallback(text, XmNvalueChangedCallback, NotesChgCB,
		(XtPointer)hwp);

	/* check if notes exists for current section */
	DmDisplayNotes(hwp,  DmGetNotesPtr(hwp, hwp->hsp->tag ?
		hwp->hsp->tag : hwp->hsp->name));

	/* register callback for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(hwp->notesWin),
		hwp->app_id, HELPMGR_HELP_FILE, NOTESWINHELP);

	MapGizmo(PopupGizmoClass, hwp->notesWin);

}	/* end of DmNotesCB */

/****************************procedure*header*****************************
 * Updates master note file for help file for the application.
 */
static int
UpdateNotesFile(DmHelpFilePtr hfp)
{
	int fd;
	FILE *file;
	DmHelpNotesPtr	np;

	/* create notes file if it does not exist */
	if ((fd = open(hfp->notes, O_WRONLY | O_CREAT | O_TRUNC,
	  DESKTOP_UMASK(Desktop) & ~(S_IXUSR | S_IXGRP | S_IXOTH))) == -1)
	{
		Dm__VaPrintMsg(TXT_CANT_OPEN_NOTES_FILE, hfp->notes);
		return(-1);
	}
	if (lockf(fd, F_LOCK, 0L) == -1) {
err:
		Dm__VaPrintMsg(TXT_LOCK_FILE, hfp->notes, errno);
		close(fd);
		return(-1);
	} 
	if ((file = fdopen(fd, "w")) == NULL)
		goto err;

	/* start writing to notes file */
	for (np = hfp->notesp; np; np = np->next) {
		fprintf(file, "%s^%s^%s^\n", (np->sect_tag ? np->sect_tag:""),
			np->sect_name ? np->sect_name : "", np->notes_file);
	}
	fclose(file);
	return(0);

}	/* end of UpdateNotesFile */

/****************************procedure*header*****************************
 * Saves notes in Notes window in notes file.
 */
static void
SaveNotes(DmHelpWinPtr hwp)
{
	FILE *fp;
	int len;
	char *notes;
	char *app_name;
	struct stat hstat;
	DmHelpNotesPtr np;
	DmHelpAppPtr hap;
	Boolean new = True;
	char *tfile = NULL;
	Gizmo textGiz = QueryGizmo(PopupGizmoClass, hwp->notesWin,
			GetGizmoGizmo, "notesTextGizmo");

	DmClearStatus((DmWinPtr)hwp);
	if (!hwp->hsp->notes_chged ||
		(notes = GetTextGizmoText(textGiz)) == NULL)
	{
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_SAVE_CHANGES);
	    return;
	}
	hap = DmGetHelpApp(hwp->app_id);
	if (locale == NULL)
		locale = setlocale(LC_MESSAGES, NULL);
	SetAppName(hap, &app_name);
	sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s/.notes", DESKTOP_DIR(Desktop),
		locale, app_name);

	/* create the notes directory if it doesn't exist */
	if (stat(Dm__buffer, &hstat) != 0) {
		if (mkdirp(Dm__buffer, DESKTOP_UMASK(Desktop)) == -1) {
			DmVaDisplayStatus((DmWinPtr)hwp, True,
				TXT_NOTES_SAVED_FAIL);
			return;
		}
	}
	/*
	 * Check if notes already exists for the section.
	 * Note that np->sect_tag can be NULL.
	 */
	for (np = hwp->hfp->notesp; np; np = np->next) {
		if ((np->sect_tag && hwp->hsp->tag &&
			strcmp(np->sect_tag, hwp->hsp->tag) == 0) ||
			(np->sect_name && hwp->hsp->name &&
			strcmp(np->sect_name, hwp->hsp->name) == 0))
		{
				tfile = strdup(np->notes_file);
				new = False;
				break;
		}
	}
	if (!tfile) {
		sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s/.notes/",
			DESKTOP_DIR(Desktop), locale, app_name);
		tfile = tempnam(Dm__buffer, NULL);
	}
	if (new) {
		np = (DmHelpNotesPtr)CALLOC(1, sizeof(DmHelpNotesRec));
		np->notes_file = strdup(tfile);
		if (hwp->hsp->name)
			np->sect_name = strdup(hwp->hsp->name);
		if (hwp->hsp->tag)
			np->sect_tag = strdup(hwp->hsp->tag);
		/* add notes to the list */
		np->next = hwp->hfp->notesp;
		hwp->hfp->notesp = np;
	}
	if (hwp->hfp->notes == NULL) {
		sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s/.notes/%s",
			DESKTOP_DIR(Desktop), locale, app_name,
			basename(hwp->hfp->name));
		hwp->hfp->notes = strdup(Dm__buffer);
	}
	if (new && UpdateNotesFile(hwp->hfp) == -1) {
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_SAVED_FAIL);
		FREE((void *)tfile);
		return;
	}
	if ((fp = fopen(tfile, "w")) == NULL) {
		perror("fopen");
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_SAVED_FAIL);
		FREE((void *)tfile);
		return;
	}
	len = strlen(notes) + 1; /* include NULL terminator */
	if (fwrite(notes, sizeof(char), len, fp) != len) {
		perror("fwrite");
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_SAVED_FAIL);
	} else {
		Widget button = QueryGizmo(PopupGizmoClass, hwp->notesWin,
			GetGizmoWidget, "notesMenu:1");

		DmVaDisplayStatus((DmWinPtr)hwp, False, TXT_NOTES_SAVED);
		BringDownPopup(GetPopupGizmoShell(hwp->notesWin));

		/* activate Delete button */
		XtSetArg(Dm__arg[0], XtNsensitive, True);
		XtSetValues(button, Dm__arg, 1);

		/* set change flag to False */
		hwp->hsp->notes_chged = False;
	}
	(void)fclose(fp);
	FREE((void *)tfile);

}	/* end of SaveNotes */

/****************************procedure*header*****************************
 * Delete notes file previously saved.
 */
static void
DelNotes(DmHelpWinPtr hwp)
{
	struct stat hstat;
	DmHelpNotesPtr np = NULL;

	if (hwp->hfp->notesp == NULL) {
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_NOTES_TO_DELETE);
	    return;
	}
	for (np = hwp->hfp->notesp; np; np = np->next)
		if (strcmp(hwp->hsp->tag, np->sect_tag) == 0)
			break;

	if (np == NULL) {
		DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NO_NOTES_TO_DELETE);
		return;
	}
	if (stat(np->notes_file, &hstat) == 0 && remove(np->notes_file) == 0)
	{
		register DmHelpNotesPtr	tnp = hwp->hfp->notesp;
		Widget button = QueryGizmo(PopupGizmoClass,
			hwp->notesWin, GetGizmoWidget, "notesMenu:1");
		Gizmo textGiz = QueryGizmo(PopupGizmoClass, hwp->notesWin,
			GetGizmoGizmo,"notesTextGizmo");

		SetTextGizmoText(textGiz, "");

		/*deactivate Delete button */
		XtSetArg(Dm__arg[0], XtNsensitive, False);
		XtSetValues(button, Dm__arg, 1);

		if (tnp == np) {
			hwp->hfp->notesp = np->next;
		} else {
			for (; tnp->next; tnp = tnp->next) {
				if (tnp->next == np) {
					tnp->next = np->next;
					break;
				}
			}
		}
		(void)UpdateNotesFile(hwp->hfp);
		FreeNotes(np);
		DmVaDisplayStatus((DmWinPtr)hwp, False, TXT_NOTES_DELETED);
		BringDownPopup(GetPopupGizmoShell(hwp->notesWin));
	} else
	    DmVaDisplayStatus((DmWinPtr)hwp, True, TXT_NOTES_DELETE_FAIL);

} /* end of DelNotes */

/****************************procedure*header*****************************
 * Frees a notes instance.
 */
static void
FreeNotes(DmHelpNotesPtr np)
{
	if (!np)
		return;
	if (np->sect_tag)
		free(np->sect_tag);
	if (np->sect_name)
		free(np->sect_name);
	if (np->notes_file)
		free(np->notes_file);
	free(np);

} /* end of FreeNotes */

/****************************procedure*header*****************************
 * Pops up a window to display definition of a selected term.
 */
static void
DisplayDef(DmHelpWinPtr hwp, char *term, char *def)
{
	Gizmo textGiz;
	Widget shell;
	DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	DmClearStatus((DmWinPtr)hwp);
	if (hwp->defWin) {
		/* free the previous definition */
		if (hwp->defn) {
			if (!strcmp(hwp->defn, def)) {
				/* already displaying the definition */
				return;
			}
			FREE(hwp->defn);
		}
		hwp->defn = STRDUP(def);

		textGiz = QueryGizmo(PopupGizmoClass, hwp->defWin,
			GetGizmoGizmo, "defTextGizmo");
		SetTextGizmoText(textGiz, hwp->defn);

		sprintf(Dm__buffer, "%s: %s%s", hap->title, def_title, term);
		XtSetArg(Dm__arg[0], XtNtitle, Dm__buffer);
		XtSetValues(GetPopupGizmoShell(hwp->defWin), Dm__arg, 1);

		MapGizmo(PopupGizmoClass, hwp->defWin);
		return;
	}
	defPopup.menu->clientData = (XtPointer)hwp;
	hwp->defWin = CreateGizmo(hwp->shell, PopupGizmoClass, &defPopup,
			NULL, 0);
	shell = GetPopupGizmoShell(hwp->defWin);

	sprintf(Dm__buffer, "%s: %s%s", hap->title, def_title, term);
	XtSetArg(Dm__arg[0], XtNtitle, Dm__buffer);
	XtSetValues(shell, Dm__arg, 1);

	XtAddCallback(shell, XtNpopdownCallback, DmDestroyDefWin,
		(XtPointer)hwp);

	hwp->defn = STRDUP(def);

	textGiz = QueryGizmo(PopupGizmoClass, hwp->defWin, GetGizmoGizmo,
			"defTextGizmo");
	SetTextGizmoText(textGiz, hwp->defn);
	
	/* register callback for context-sensitive help */
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(hwp->defWin),
		hwp->app_id, HELPMGR_HELP_FILE, DEFWINHELP);

	MapGizmo(PopupGizmoClass, hwp->defWin);

}	/* end of DisplayDef */

/****************************procedure*header*****************************
 * Displays help on help window.
 */
void
DmUsingHelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
		HELPMGR_HELP_FILE, USING_HELP_SECT);

}	/* end of DmUsingHelpCB */

/****************************procedure*header*****************************
 * Clears help window's message area and calls DmHelpDeskCB.
 */
void
DmOpenHelpDeskCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmClearStatus((DmWinPtr)client_data);
	DmHelpDeskCB(NULL, NULL, NULL);

} /* end of DmOpenHelpDeskCB */

/****************************procedure*header*****************************
 * Callback for Cancel button for Search window.
 */
void
DmDestroySearchWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	FreeGizmo(PopupGizmoClass, hwp->searchWin);
	XtDestroyWidget(GetPopupGizmoShell(hwp->searchWin));
	hwp->searchWin = NULL;

} /* end of DestroySearchWin */

/****************************procedure*header*****************************
 * Callback for Cancel button for Bookmark window.
 */
void
DmDestroyBmarkWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	FreeGizmo(PopupGizmoClass, hwp->bmarkWin);
	XtDestroyWidget(GetPopupGizmoShell(hwp->bmarkWin));
	hwp->bmarkWin = NULL;

} /* end of DmDestroyBmarkWin */

/****************************procedure*header*****************************
 * Callback for Cancel button for Notes window.
 */
void
DmDestroyNotesWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	FreeGizmo(PopupGizmoClass, hwp->notesWin);
	XtDestroyWidget(GetPopupGizmoShell(hwp->notesWin));
	hwp->notesWin = NULL;

} /* end of DmDestroyNotesWin */

/****************************procedure*header*****************************
 * Callback for Cancel button for Glossary window.
 */
void
DmDestroyGlossWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	/* FIX: free glossary */

	FreeGizmo(PopupGizmoClass, hwp->glossWin);
	XtDestroyWidget(GetPopupGizmoShell(hwp->glossWin));
	hwp->glossWin = NULL;

} /* end of DmDestroyGlossWin */

/****************************procedure*header*****************************
 * Callback for Cancel button for Definition window.
 */
void
DmDestroyDefWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	if (hwp->defn)
		free(hwp->defn);
	FreeGizmo(PopupGizmoClass, hwp->defWin);
	XtDestroyWidget(GetPopupGizmoShell(hwp->defWin));
	hwp->defWin = NULL;

} /* end of DmDestroyDefWin */

/****************************procedure*header*****************************
 * DmReadBookmarks -
 */
void
DmReadBookmarks(DmHelpAppPtr hap)
{
	char *begp;
	char *endp;
	DmMapfilePtr mp;
	DmHelpBmarkPtr bmp;

	sprintf(Dm__buffer, "%s/.dthelp/.%s/.%s/.bookmark",
		DESKTOP_DIR(Desktop), locale ?
		locale : setlocale(LC_MESSAGES, NULL),
		strcmp(hap->name, dtmgr_title) == 0 ? hap->title : hap->name);

	/* create list of bookmarks if exist */
	if (!(mp = Dm__mapfile(Dm__buffer, PROT_READ, MAP_SHARED))) {
		/* no bookmarks */
		return;
	} else {
		/* start reading in bookmarks */
		while (MF_NOT_EOF(mp)) {
			bmp = (DmHelpBmarkPtr)CALLOC(1,sizeof(DmHelpBmarkRec));

			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^')))
				goto bye;
			bmp->app_name = (XtPointer)(strndup(begp, endp-begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->app_name);
				FREE((void *)bmp);
				goto bye;
			}
			bmp->file_name = (XtPointer)(strndup(begp, endp-begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->app_name);
				FREE(bmp->file_name);
				FREE((void *)bmp);
				goto bye;
			}
			bmp->sect_tag = (XtPointer)(strndup(begp, endp-begp));

			MF_NEXTC(mp);
			begp = MF_GETPTR(mp);
			if (!(endp = Dm__findchar(mp, '^'))) {
				FREE(bmp->app_name);
				FREE(bmp->file_name);
				FREE(bmp->sect_tag);
				FREE((void *)bmp);
				goto bye;
			}
			bmp->sect_name = (XtPointer)(strndup(begp, endp-begp));

			MF_NEXTC(mp); /* skip '^' */
			MF_NEXTC(mp); /* skip '\n' */

			bmp->next = hap->bmp;
			hap->bmp = bmp;
			hap->num_bmark++;
		}
	}
bye:
	Dm__unmapfile(mp);

} /* end of DmReadBookmarks */

/****************************procedure*header*****************************
 * SetAppName -
 */
void
SetAppName(DmHelpAppPtr hap, char **app_name)
{
	if (strcmp(hap->name, dtmgr_title) == 0)
		*app_name = hap->title;
	else
		*app_name = hap->name;

} /* end of SetAppName */

/****************************procedure*header*****************************
 * NotesChgCB - Callback for textedit widget used in Notes window to set
 * flag which indicates whether the text was modified.
 */
static void
NotesChgCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;
	XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)call_data;

	if (cbs->reason == XmCR_VALUE_CHANGED)
		hwp->hsp->notes_chged = True;

} /* end of NotesChgCB */

/****************************procedure*header*****************************
 * NotesMenuCB -
 */
static void
NotesMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Save */
		SaveNotes(hwp);
		break;
	case 1: /* Delete */
		DelNotes(hwp);
		break;
	case 2: /* Cancel */
		XtPopdown(GetPopupGizmoShell(hwp->notesWin));
		break;
	case 3: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
			HELPMGR_HELP_FILE, NOTESWINHELP);
		break;
	}
} /* end of NotesMenuCB */

/****************************procedure*header*****************************
 * DefMenuCB -
 */
static void
DefMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* OK */
		XtPopdown(GetPopupGizmoShell(hwp->defWin));
		break;
	case 1: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
			HELPMGR_HELP_FILE, DEFWINHELP);
		break;
	}
} /* end of DefMenuCB */

/****************************procedure*header*****************************
 * GlossaryMenuCB -
 */
static void
GlossaryMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* OK */
		XtPopdown(GetPopupGizmoShell(hwp->glossWin));
		break;
	case 1: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
			HELPMGR_HELP_FILE, GLOSSWINHELP);
		break;
	}
} /* end of GlossaryMenuCB */

/****************************procedure*header*****************************
 * BmarkListMenuCB -
 */
static void
BmarkListMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Add */
		AddBmark(hwp);
		break;
	case 1: /* Delete */
		DelBmark(hwp);
		break;
	case 2: /* Delete All */
		DelAllBmark(hwp);
		break;
	}
} /* end of BmarkListMenuCB */

/****************************procedure*header*****************************
 * BmarkMenuCB -
 */
static void
BmarkMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	DmHelpWinPtr hwp = (DmHelpWinPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Go To */
		BtnGoToBmark(hwp);
		break;
	case 1: /* Close */
		XtPopdown(GetPopupGizmoShell(hwp->bmarkWin));
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(hwp->app_id), NULL,
			HELPMGR_HELP_FILE, BMARKWINHELP);
		break;
	}

} /* end of BmarkMenuCB */

/*
 * SearchStr() locates the first occurrence in the string as1 of
 * the sequence of characters (excluding the terminating null
 * character) in the string as2. SearchStr() returns a pointer 
 * to the located string, or a null pointer if the string is
 * not found. If as2 is "", the function returns as1.
 * The line number and the index in as1 is returned
 * in line and index.
 * Assumes the leading spaces of as2 are already removed.
 */

static char *
SearchStr(char *as1, char *as2, unsigned int *line, unsigned int *index)
{
	register const char *s1,*s2;
	register char c;
	register const char *tptr;
	Boolean link = False;

	s1 = as1 + *index;
	s2 = as2;

	if (s2 == NULL || *s2 == '\0')
		return((char *)s1);
	c = *s2;

	while (*s1) {
		if (link)
			if (*s1 == ')')
				link = False;
			else {
				s1++;
				(*index)++;
				continue;
			}
		
		switch (*s1) {
		case '\\':
			(*index)++;
			switch (*++s1) {
			case 'n':
				(*line)++;
				/* FALLTHROUGH */
			case 'k':
			case 's':
			case 't':
			case '\n':
				(*index)++;
				s1++;
				continue;
			}
		case '^':
			link = True;
			(*index)++;
			s1++;
			continue;
		case '\n':
			(*line)++;
			/* FALLTHROUGH */
		case '(':
		case ')':
		case '\t':
		case ' ':
			(*index)++;
			s1++;
			continue;
		}
		if (*s1++ == c) {
			tptr = s1;
			while ((c = *++s2) == *s1++ && c) ;
			if (c == 0)
				return((char *)tptr - 1);
			s1 = tptr;
			s2 = as2;
			c = *s2;
		}
		(*index)++;
	}
	return(NULL);
}

/* Routines and data structures to display status message in a popup. */

static void
StatusNoticeDestroyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Gizmo gizmo = (Gizmo)client_data;
    XtDestroyWidget(GetModalGizmoShell(gizmo));
    FreeGizmo(ModalGizmoClass, gizmo);
}

static void
StatusNoticePopdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(DtGetShellOfWidget(widget));
}

static MenuItems menubarItems[] = {
	MENU_ITEM(TXT_OK, TXT_M_OK, StatusNoticePopdownCB ),
	{ NULL }                                /* NULL terminated */
};
MENU_BAR("statusNoticeMenubar", menubar, NULL, 0, 0);

static ModalGizmo statusNoticeGizmo = {
	NULL,                           /* help info */
	"statusNotice",                  /* shell name */
	TXT_G_PRODUCT_NAME,             /* title */
	&menubar,                       /* menu */
	"",                             /* message (run-time) */
	NULL, 0,                        /* gizmos, num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL, /* style */
	0                 		/* type */
};

static void
VDisplay(Widget window, char *msg, va_list ap, int type)
{
	char buffer[1024];
	Gizmo	gizmo;
	Widget	shell;

	if (msg == NULL)
		return;

	vsprintf(buffer, Dm__gettxt(msg), ap);
	statusNoticeGizmo.message = buffer;
	statusNoticeGizmo.type = type;
	gizmo = CreateGizmo(window, ModalGizmoClass, &statusNoticeGizmo,
			    NULL, 0);
	shell = GetModalGizmoShell(gizmo);

	XmAddWMProtocolCallback(shell, XA_WM_DELETE_WINDOW(XtDisplay(shell)),
				StatusNoticeDestroyCB, (XtPointer)gizmo);
	XtAddCallback(shell, XmNpopdownCallback,
			StatusNoticeDestroyCB, (XtPointer)gizmo);
	MapGizmo(ModalGizmoClass, gizmo);
}

static void
VaStatusPopup(Widget window, int type, char *msg, ... )
{
	va_list ap;

	if (type == XmDIALOG_ERROR || type == XmDIALOG_WARNING)
		DmBeep();

	va_start(ap, msg);
	VDisplay(window, msg, ap, type);
	va_end(ap);
}
