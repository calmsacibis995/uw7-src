#pragma ident	"@(#)dtm:h_util.c	1.32"

/******************************file*header********************************

    Description:
     This file contains the source code for parsing a help file,
	reading a definition file and retrieving notes previously saved.
*/
                              /* #includes go here     */
#include <locale.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/TextGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public Procedures
          2. Private  Procedures
*/
                         /* public procedures         */

void SetAppName(DmHelpAppPtr hap, char **app_name);

                         /* private procedures         */
static void GetNotesInfo(DmHelpWinPtr hwp);
static char *ReadNotes(char *file);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* strings used by the help manager */
const char *toc_title;
const char *def_title;
const char *notes_title;
const char *search_title;
const char *bmark_title;
const char *gloss_title;
const char *help_str;
const char *nohelp_str;

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Called at the beginning of a session to initialize the help manager. 
 */
int
DmInitHelpManager()
{
	/* cache strings used by the help manager to avoid multiple calls
	 * to gettxt() for the same string
	 */
	toc_title    = Dm__gettxt(TXT_TOC);
	def_title    = Dm__gettxt(TXT_DEF_OF);
	notes_title  = Dm__gettxt(TXT_NOTES);
	search_title = Dm__gettxt(TXT_SEARCH);
	bmark_title  = Dm__gettxt(TXT_BOOKMARK);
	gloss_title  = Dm__gettxt(TXT_GLOSSARY);
	help_str     = Dm__gettxt(TXT_HELP_STR);
	nohelp_str   = Dm__gettxt(TXT_NO_HELP_AVAIL);
	return(0);

} /* end of DmInitHelpManager */

/****************************procedure*header*****************************
 *  Call at the end of a session to close all help windows.
 */
void
DmHMExit()
{
	/* close all help windows */
	DmCloseAllHelpWindows();

} /* end of DmHMExit */

/****************************procedure*header*****************************
 * This function parses lines like "^*name^value". If successful, the
 * pointer is placed at the beginning of the next line.
 */
int
Dm__GetNameValue(DmMapfilePtr mp, char **name, char **value)
{
	char *start = MF_GETPTR(mp);
	char *p;

	if (p = Dm__findchar(mp, '^')) {
		*name = (char *)strndup(start, p - start);
		start = ++p;
		MF_NEXTC(mp); /* skip '^' */
		if (p = Dm__findchar(mp, '\n')) {
			*value = (char *)strndup(start, p - start);
			MF_NEXTC(mp); /* skip '\n' */
			return(0);
		} else {
			if (name)
				free(name);
			*name = *value = NULL;
			return(1); /* can't find '\n' */
		}
	}
	*name = *value = NULL;
	return(1); /* can't find '^' between name and value */
} /* end of Dm__GetNameValue() */

/****************************procedure*header*****************************
 * This function parses lines like "^%keyword^value". If successful,
 * the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
int
DmGetKeyword(DmMapfilePtr mp, DtPropListPtr plistp)
{
	char *name;
	char *value;

	if (Dm__GetNameValue(mp, &name, &value))
		return(1);

	DtSetProperty(plistp, name, value, DM_B_KEYWORD);
	free(value);
	free(name);
	return(0);
} /* end of DmGetKeyword() */

/****************************procedure*header*****************************
 * This function parses definition lines like "^=keyword" until the end of
 * the definition "^=".
 * If successful, the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
void
DmGetDefinition(DmMapfilePtr mp, DtPropListPtr plistp)
{
	char *name = MF_GETPTR(mp);
	char *value;
	char *p;

	if (p = Dm__findchar(mp, '\n')) {
		name = (char *)strndup(name, p - name);
		MF_NEXTC(mp); /* skip '\n' */

		/* find end of definition "\n^=\n" */
		value = MF_GETPTR(mp);
		if (p = Dm__strstr(mp, "\n^=\n")) {
			value = (char *)strndup(value, p - value);
			DtSetProperty(plistp, name, value, DM_B_DEF);
			free(value);
			free(name);
			MF_NEXTC(mp); /* skip '\n' */
			MF_NEXTC(mp); /* skip '^'  */
			MF_NEXTC(mp); /* skip '='  */
			MF_NEXTC(mp); /* skip '\n' */
			return;
		}
		free(name);
		return;
	}
} /* end of DmGetDefinition() */

/****************************procedure*header*****************************
 * This function parses definition file line like "^+deffile" until the end of
 * the definition "^=".
 * If successful, the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
void
DmReadDefFile(DmMapfilePtr mp, DtPropListPtr plistp, char *help_dir)
{
	DmMapfilePtr def_mp;
	char *name = MF_GETPTR(mp);
	char *def_file;
	char *p;
	int c;

	if (p = Dm__findchar(mp, '\n')) {
		name = (char *)strndup(name, p - name);
		MF_NEXTC(mp); /* skip '\n' */

		if ((def_file = DmGetFullPath(name, help_dir)) == NULL)
			return;

		/* map file for reading */
		if (!(def_mp = Dm__mapfile(def_file, PROT_READ, MAP_SHARED))) {
			free(def_file);
  			return;
		}
		free(def_file);

		/* scan the file for definitions */
		while (MF_NOT_EOF(def_mp)) {

			if (MF_PEEKC(def_mp) == '#') {
				Dm__findchar(def_mp, '\n');
				MF_NEXTC(def_mp); /* skip #ident string */

			} else if (MF_PEEKC(def_mp) == '\n') {
				MF_NEXTC(def_mp); /* skip blank line */

			} else if (MF_PEEKC(def_mp) == '^') {
				MF_NEXTC(def_mp); /* skip '^' */

				switch(c = MF_PEEKC(def_mp)) {
				case '=':
					/* definition */
					MF_NEXTC(def_mp);
					DmGetDefinition(def_mp, plistp);
					break;

				default:
					/* silently ignore other options and find next line */
					Dm__findchar(def_mp, '\n');
					MF_NEXTC(def_mp); /* skip '\n' */
					break;
				}
			}
		}
		free(name);
		Dm__unmapfile(def_mp);
		return;
	}
} /* end of DmReadDefFile() */

/****************************procedure*header*****************************
 * This routine will convert a reference string ("file^sect_tag")
 * to a DmHelpLoc structure and return a pointer to this static structure.
 * The original string is not modified in anyway.
 */
DmHelpLocPtr
DmHelpRefToLoc(char *str)
{
	char *save = strdup(str);
	register char *p = save;
	static DmHelpLocRec loc = { NULL };
	char *array[2] = { NULL };
	char **pp = array;

	/* free previous strings */
	if (loc.file) {
		free(loc.file);
		loc.file = NULL;
	}
	if (loc.sect_tag) {
		free(loc.sect_tag);
		loc.sect_tag = NULL;
	}

	if (*p != '^') {
		/* file_name is specified */ 
		*pp = p;

		if (p = strchr(p, '^')) {
			*p++ = '\0';

			/* assign the rest of the string to sect_tag */
			/* it could be NULL */
			*(++pp) = p;
		} else {
			/* assume str only contains file_name */
			*(++pp) = NULL;
		}
	} else {
		*pp = NULL;
		*p++;
		*(++pp) = p;
	}

	if (array[0])
		loc.file = strdup(array[0]);
	if (array[1])
		loc.sect_tag = strdup(array[1]);

	free(save);
	return(&loc);
} /* end of DmHelpRefToLoc() */

/****************************procedure*header*****************************
 */
void
DmDisplayNotes(DmHelpWinPtr hwp, DmHelpNotesPtr np)
{
	Widget button = QueryGizmo(PopupGizmoClass, hwp->notesWin,
			GetGizmoWidget, "notesMenu:1");
	TextGizmo *textGiz = (TextGizmo *)QueryGizmo(PopupGizmoClass,
			hwp->notesWin, GetGizmoGizmo, "notesTextGizmo");

	if (np) {
		char *notes = ReadNotes(np->notes_file);
		if (notes) {
			SetTextGizmoText(textGiz, notes);

			/* make Delete button sensitive */
			XtSetArg(Dm__arg[0], XtNsensitive, True);
			XtSetValues(button, Dm__arg, 1);
			free(notes);
			return;
		}
	}
	SetTextGizmoText(textGiz, "");

	/* make Delete button insensitive */
	XtSetArg(Dm__arg[0], XtNsensitive, False);
	XtSetValues(button, Dm__arg, 1);

} /* end of DmDisplayNotes */

/****************************procedure*header*****************************
 * ReadNotes - Reads and returns the contents of a notes file.
 * The caller should free returned value when done with it.
 */
static char *
ReadNotes(char *file)
{
	char *buf;
	struct stat hstat;
	FILE *fp;

	if (stat(file, &hstat) != 0) {
		return(NULL);
	}
	if ((buf = (char *)MALLOC(sizeof(char)*hstat.st_size)) == NULL) {
		return(NULL);
	}
	if ((fp = fopen(file, "r")) == NULL) {
		perror("fopen");
		return(NULL);
	}
	if (fread(buf, sizeof(char), hstat.st_size, fp) == 0) {
		perror("fread");
		(void)fclose(fp);
		return(NULL);
	}
	(void)fclose(fp);
	return(buf);
	
} /* end of ReadNotes */

/****************************procedure*header*****************************
 * Called when a new section is displayed to activate/deactivate the
 * Previous Topic, Next Topic, Backtrack, Bookmark and Notes buttons
 * depending on whether a "regular" section or a Table of Contents is
 * being displayed.  Also deactivates/activates buttons in Bookmark
 * and Notes window if they're up when a Table of Contents and a
 * "regular" section is displayed, respectively.
 */
void
DmChgHelpWinBtnState(DmHelpWinPtr hwp, Boolean skip_menubar, Boolean sensitive)
{
	XtSetArg(Dm__arg[0], XtNsensitive, sensitive);

	if (skip_menubar == False) {
		Widget btn;

		/* Backtrack button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:0");
		XtSetValues(btn, Dm__arg, 1);

		/* Next Topic button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:1");
		XtSetValues(btn, Dm__arg, 1);

		/* Previous Topic button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:2");
		XtSetValues(btn, Dm__arg, 1);

		/* Search button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:3");
		XtSetValues(btn, Dm__arg, 1);

		/* Bookmark button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:4");
		XtSetValues(btn, Dm__arg, 1);

		/* Notes button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:5");
		XtSetValues(btn, Dm__arg, 1);

		/* Table of Contents button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:6");
		XtSetValues(btn, Dm__arg, 1);

		/* Glossary button */
		btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:7");
		XtSetValues(btn, Dm__arg, 1);
	}
#ifdef NOT_USE
	/* if Search window is up, deactivate/activate its menu */
	if (hwp->searchWin) {
		Widget menu = QueryGizmo(PopupGizmoClass, hwp->searchWin,
			GetGizmoWidget, "searchMenu");
		XtSetValues(menu, Dm__arg, 1);
	}
#endif
	/* if Bookmark window is up, deactivate/activate its menus */
	if (hwp->bmarkWin) {
		Widget menu;

		menu = QueryGizmo(PopupGizmoClass, hwp->bmarkWin,
			GetGizmoWidget, "bmarkMenu");
		XtSetValues(menu, Dm__arg, 1);
		menu = QueryGizmo(PopupGizmoClass, hwp->bmarkWin,
			GetGizmoWidget, "bmarkListMenu");
		XtSetValues(menu, Dm__arg, 1);
	}
	/* if Notes window is up, deactivate/activate its menu */
	if (hwp->notesWin) {
		Widget menu = QueryGizmo(PopupGizmoClass, hwp->notesWin,
			GetGizmoWidget, "notesMenu");
		XtSetValues(menu, Dm__arg, 1);
	}

} /* end of DmChgHelpWinBtnState */

/****************************procedure*header*****************************
 * This routine reads in notes information stored in
 * $DESKTOPDIR/.dthelp/.<locale>/.<app_name>/.notes/<hfp->name>
 * file saved from a previous session.  The notes file contains the
 * section name and the name of its notes file in the
 * format "sect_tag^sect_name^notes_file". 
 */
static void
GetNotesInfo(DmHelpWinPtr hwp)
{
     char *begp;
     char *endp;
     char *app_name;
     DmMapfilePtr mp;
     DmHelpNotesPtr notesp;
     DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);

	SetAppName(hap, &app_name);
	sprintf(Dm__buffer,"%s/.dthelp/.%s/.%s/.notes/%s",DESKTOP_DIR(Desktop),
		setlocale(LC_MESSAGES,NULL),app_name,basename(hwp->hfp->name));

	hwp->hfp->notes = strdup(Dm__buffer);
	/* map the notes file for reading */
	if ((mp = Dm__mapfile(Dm__buffer, PROT_READ, MAP_SHARED)) == NULL)
		return;

	while (MF_NOT_EOF(mp)) {
		notesp = (DmHelpNotesPtr)CALLOC(1, sizeof(DmHelpNotesRec));
		begp = MF_GETPTR(mp);
		if (!(endp = Dm__findchar(mp, '^'))) {
			free(notesp);
			goto bye;
		}
		if ((endp - begp) > 0)
			notesp->sect_tag =(XtPointer)(strndup(begp,endp-begp));
		MF_NEXTC(mp);
		begp = MF_GETPTR(mp);
		if (!(endp = Dm__findchar(mp, '^'))) {
			if (notesp->sect_tag)
				free(notesp->sect_tag);
			free(notesp);
			goto bye;
		}
		notesp->sect_name = (XtPointer)(strndup(begp, endp - begp));
		MF_NEXTC(mp);
		begp = MF_GETPTR(mp);
		if (!(endp = Dm__findchar(mp, '^'))) {
			if (notesp->sect_tag)
				free(notesp->sect_tag);
			free(notesp->sect_name);
			free(notesp);
			goto bye;
		}
		notesp->notes_file = (XtPointer)(strndup(begp, endp - begp));
		notesp->next = hwp->hfp->notesp;
		hwp->hfp->notesp = notesp;
		MF_NEXTC(mp);	/* skip '^' */
		MF_NEXTC(mp);	/* skip '\n' */
	}
bye:
	Dm__unmapfile(mp);
	return;

} /* end of GetNotesInfo */

/****************************procedure*header*****************************
	GetFullHelpFileName - Returns full path given a help file name
	and an optional help directory.
 */
char *
DmGetFullPath(char *file_name, char *help_dir)
{
	char *fullpath;

	if (!file_name)
		fullpath = NULL;
	else if (*file_name == '/') { /* full path name is specified */
		fullpath = strdup(file_name);
	} else if (help_dir) { /* help directory is specified */
		fullpath = strdup(DmMakePath(help_dir, file_name));
	} else { /* look for file in standard paths */
		fullpath = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
			"help", file_name, NULL, NULL, NULL, 0, NULL);
	}
	return(fullpath);

} /* end of DmGetFullPath */

/****************************procedure*header*****************************
 * DmGetNotesPtr - Returns ptr to notes for a given section tag.
 */
DmHelpNotesPtr
DmGetNotesPtr(DmHelpWinPtr hwp, char *cur_tag)
{
	DmHelpNotesPtr np;

	if (hwp->hfp->notesp == NULL)
		GetNotesInfo(hwp);

	for (np = hwp->hfp->notesp; np; np = np->next) {
		if (np->sect_name && !strcmp(cur_tag, np->sect_name) ||
			(np->sect_tag && !strcmp(cur_tag, np->sect_tag))) {
				return(np);
		}
	}
	return(NULL);

} /* end of DmGetNotesPtr */
