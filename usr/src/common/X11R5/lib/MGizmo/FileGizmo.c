#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:FileGizmo.c	1.24.2.2"
#endif

/* Edit this file with ts=4 */

#include <stdio.h> 

	/* Replace libgen version with POSIX version (libc) to
	 * keep libMundo alive. Unfortunately, using POSIX version
	 * has performance implications because `pattern' will be
	 * compiled when calling fnmatch() each time. However,
	 * there is a faster version in 486/386 although they are
	 * private API (i.e., starts with _). Note that internally,
	 * fnmatch () is using this set too. So...
	 *
	 * Have necessary #define(s) for all 3 types so that people
	 * can switch them easily!
	 *
	 *	1. #define USE_GMATCH  - use gmatch
	 *	2. #define USE_FNMATCH - use fnmatch
	 *	3. no #define	       - use _fnmcomp/_fnmexec/_fnmfree
	 *
	 * `3' is the default for sbird!!!
	 */
#ifdef USE_GMATCH

#include <libgen.h>

#define FLAGS			0
#define FNM_T			int
#define COMPILE(FNP,PAT,FLAGS)	1
#define MATCH(FNP,N,PAT,FLAGS)	(gmatch(N, PAT) != 0)
#define FREE_FNP(FNP)

#else /* USE_GMATCH */

#include <fnmatch.h>

	/* Use _fnmcomp/_fnmexec/_fnmfree to gain more performance,
	 * if this is not OK, simply include #define USE_FNMATCH */
#ifdef USE_FNMATCH

#define FLAGS			0
#define FNM_T			int
#define COMPILE(FNP,PAT,FLAGS)	1
#define MATCH(FNP,N,PAT,FLAGS)	(fnmatch(PAT,N,FLAGS) == 0)
#define FREE_FNP(FNP)

#else /* USE_FNMATCH */
			/* behave more like gmatch but the flags are private */
#define FLAGS			FNM_BADRANGE | FNM_BKTESCAPE
#define FNM_T			fnm_t
#define COMPILE(FNP,PAT,FLAGS)	(_fnmcomp(FNP,(const unsigned char*)PAT,FLAGS))\
							? 0 : 1
#define MATCH(FNP,N,PAT,FLAGS)	(_fnmexec(FNP,(const unsigned char *)N) == 0)
#define FREE_FNP(FNP)		_fnmfree(FNP)

#endif /* USE_FNMATCH */
#endif /* USE_GMATCH */

#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/MwmUtil.h>
#include <Xm/DialogS.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleBG.h>
#include <Xm/List.h>

#include <DesktopP.h>
#include <DtI.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"
#include "FileGizmoP.h"
#include "LabelGizmP.h"

#ifdef DEBUG
#define FILE(a)	"file"a
#else
#define FILE(a) "_X_"
#endif /* DEBUG */

#define VIEW_HEIGHT			6

#define TXT_ITEMS_IN		"mgizmo:1" FS "Items In: %s"
#define TXT_FOLDERS_IN		"mgizmo:2" FS "Folders in: %s"
#define TXT_FOLDERS			"mgizmo:3" FS "Folders"
#define TXT_FILES			"mgizmo:4" FS "Files"
#define TXT_FILES_FILTERED	"mgizmo:5" FS "Files (%d filtered out)"
#define TXT_SHOW_HIDDEN		"mgizmo:6" FS "Show Hidden (dot) Files"
#define TXT_PARENT			"mgizmo:7" FS "Parent Folder"

static char *	slash = "/";
static char *	dot = ".";
static char *	dotdot = "..";

static void		FreeFileGizmo(FileGizmoP *);
static Gizmo		CreateFileGizmo(Widget, FileGizmo *, Arg *, int);
static void		MapFileGizmo(FileGizmoP *);
static void		ManipulateFileGizmo(FileGizmoP *, ManipulateOption);
static XtPointer	QueryFileGizmo(FileGizmoP *, int, char *);
static Boolean		ReadDirectory(FileGizmoP *, XmString *);

GizmoClassRec FileGizmoClass[] = {
	"FileGizmo",
	CreateFileGizmo,	/* Create */
	FreeFileGizmo,		/* Free	*/
	MapFileGizmo,		/* Map */
	NULL,				/* Get */
	NULL,				/* Get Menu */
	NULL,				/* Dump */
	ManipulateFileGizmo,/* Manipulate */
	QueryFileGizmo,		/* Query */
	NULL,				/* SetByName */
	NULL				/* Attachment */
};

static void
FreeDir(ListRec *lp, int start)
{
	int		i;

	if (lp->cnt == 0) {
		return;
	}
	XmListDeleteAllItems(lp->cw);
	for (i=start; i<lp->cnt; i++) {
		XmStringFree(lp->xmsnames[i]);
		FREE(lp->data[i].names);
	}
	lp->cnt = 0;
}

static void 
FreeFileGizmo(FileGizmoP *gp)
{
	FreeDir(&gp->files, 0);
	FreeDir(&gp->dir, 0);
	FREE(gp->files.data);
	FREE(gp->dir.data);
	FREE(gp->directory);
	FreeGizmo(CommandMenuGizmoClass, gp->menu);
	FreeGizmoArray(gp->upperGizmos, gp->numUpper);
	FreeGizmoArray(gp->lowerGizmos, gp->numLower);
	FREE(gp);
}

/*
 * Reconstruct the path in gp->directory and make sure it contains no
 * ".." or "." and make it an absolute path.
 */
static void
FixDirectory(FileGizmoP *gp)
{
	char *	cp;
	char *	name;
	char *	names[1024];
	char	newPath[BUFSIZ];
	char	buf[BUFSIZ];
	char *	np;
	int		i = 0;
	int		j;
	char *	path = gp->directory;

	/* If this is a relative path then fix it to the current directory */
	if (path[0] != slash[0]) {
		(void)getcwd(buf, BUFSIZ);
		strcat(buf, slash);
		strcat(buf, path);
		path = buf;
	}
	cp = path;
	/* Only fix path if path contains at least one "." */
	if (strchr(path, dot[0]) != NULL) {
		/* Loop thru the path an break it apart into the directory names */
		while((name = strtok(cp, slash)) != NULL) {
			cp = NULL;
			if (strcmp(name, dot) == 0) {
				continue;
			}
			if (strcmp(name, dotdot) == 0) {
				if (i != 0) {
					i -= 1;
				}
			}
			else {
				names[i++] = name;
			}
		}
		cp = newPath;
		cp[0] = slash[0];	/* Set the newPath to "/" incase i == 0 */
		cp[1] = '\0';
		for (j=0; j<i; j++) {
			cp[0] = slash[0];
			strcpy(++cp, names[j]);
			cp += strlen(names[j]);
		}
		cp = newPath;
	}
	if (cp != gp->directory) {
		FREE(gp->directory);
		gp->directory = STRDUP(cp);
	}
}

/*
 * Test to see if filter is a regular expression.
 */
static Boolean
RegularExpression(char *filter)
{
	char *	cp;

	if ((cp = strpbrk(filter, "*?[")) != NULL) {
		if (cp == filter) {
			return True;
		}
		cp -= 1;
		if (*cp != '\\') {
			return True;
		}
	}
	return False;
}

/*
 * Return something like XmStringCreateLocalized("<path>/<path>")
 */
static XmString
FormatPath(char *path)
{
	char *	cp;

	/* Look for last '/' in the path */
#ifdef davef
	cp = strrchr(path, slash[0]);
	if (cp != NULL && cp != path) {
		/* Look for the next to last '/' in the path */
		for (cp-=1; cp!=path; cp--) {
			if (*cp == slash[0]) {
				cp += 1;
				break;
			}
		}
	}
	if (cp == NULL) {
		cp = path;
	}
#endif /* davef */
	return XmStringCreateLocalized(path);
}

/*
 * Set the value of the path field and "...In:" field.
 */
static void
SetPath(FileGizmoP *gp)
{
	XmString		string;
	Arg				arg[10];
	char *			cp;
	char *			name;
	char			buf[BUFSIZ];
	static char *	foldersIn = NULL;
	static char *	itemsIn = NULL;

	/* Set path field */
	string = FormatPath(gp->directory);
	XtSetArg(arg[0], XmNlabelString, string);
	XtSetValues(gp->pathAreaWidget, arg, 1);
	XmStringFree(string);

	/* Set "...In:" field */
	if (itemsIn == NULL) {
		itemsIn = GGT(TXT_ITEMS_IN);
		foldersIn = GGT(TXT_FOLDERS_IN);
	}
	if (strcmp(gp->directory, slash) == 0) {
		name = gp->directory;
	}
	else {
		name = strrchr(gp->directory, slash[0])+1;
	}
	if (gp->dialogType == FOLDERS_ONLY) {
		sprintf(buf, foldersIn, name);
	}
	else {
		sprintf(buf, itemsIn, name);
	}
	string = XmStringCreateLocalized(buf);
	XtSetArg(arg[0], XmNlabelString, string);
	XtSetValues(gp->inFieldWidget, arg, 1);
	XmStringFree(string);
}

/*
 * CompareNames
 */
static int
CompareNames(const void *s1, const void *s2)
{
	ListData *	d1 = (ListData *)s1;
	ListData *	d2 = (ListData *)s2;

	return strcmp(d1->names, d2->names);
}

static void
SortAndLocalize(ListRec *lp, int start)
{
	int	i;

	if (lp->cnt > 1+start) {
		qsort(
			(void *)&lp->data[start], (size_t)lp->cnt-start,
			sizeof(ListData), CompareNames
		);
	}
	for (i=start; i<lp->cnt; i++) {
		lp->xmsnames[i] = NULL;
	}
}

static void
MoreSpace(ListRec *lp)
{
	if (lp->cnt == lp->size) {
		lp->size *= 2;
		lp->data = (ListData *)REALLOC(lp->data, sizeof(ListRec)*lp->size);
		lp->xmsnames = (XmString *)REALLOC(
			lp->xmsnames, sizeof(XmString)*lp->size
		);
	}
}

static Boolean
Blank(char *filter)
{
	char *	cp;

	for (cp=filter; *cp!='\0'; cp++) {
		/* If the name consists entirely of blanks, or tabs or cr's */
		/* then use the gizmo's filter. */
		if (*cp != ' ' && *cp != '\t' && *cp != '\n') {
			return False;
		}
	}
	return True;
}

/*
 * ReadDirectory
 */
static Boolean
ReadDirectory(FileGizmoP *g, XmString * thisString)
{
		/* Define MATCH specific stuff here */
	FNM_T		top;
	int		valid_pattern;
	int		flags = FLAGS;

	static char *	filteredLabel = NULL;
	FileGizmoP *	gp = (FileGizmoP *)g;
	ListRec *		lp;
	DIR *			dp;
	struct dirent *	dep;
	struct stat		statBuf;
	Arg				arg[10];
	Boolean			showHidden = False;
	char *			oldDir;
	char			filterBuf[1024];
	char *			filter;
	char *			input;
	int				filteredOut = 0;		/* # files not matching filter */
	XmString		string;
	char			buf[BUFSIZ];
	Boolean			skip_stat = False;

	/* Get the name in the input field.  This could be a filter. */
	input = XmTextGetString(gp->inputFieldWidget);
	strcpy(filterBuf, input);
	filter = filterBuf;
	/* If the value in the input field is a filter then */
	/* set the gizmo filter to that value. */
	if (RegularExpression(filter) == True) {
		FREE(gp->filter);
		gp->filter = STRDUP(filter);
	}
	else {
		/* Input field contains a non regular expression so */
		/* don't use a filter. */
		filter = "";
	}
	/* Redisplay gizmo's filter if input field is blank. */
	if (Blank(input) == True) {
		XmTextFieldSetString(gp->inputFieldWidget, filter);
	}

	oldDir = (char *)getcwd(NULL, FILENAME_MAX);
	chdir(gp->directory);	/* chdir to allow stat on relative path name */

	if ((dp=opendir(gp->directory)) == NULL) {
		perror("opendir:");
		chdir(oldDir);
		FREE(oldDir);
		FREE(gp->directory);
		gp->directory = gp->prevDir;
		gp->prevDir = STRDUP(gp->directory);
		return False;
	}

	if (gp->inFieldWidget) {
		SetPath(gp);
	}

	if (gp->checkBoxWidget != NULL) {
		XtSetArg(arg[0], XmNset, &showHidden);
		XtGetValues(gp->checkBoxWidget, arg, 1);
	}

	/* Free up data for old directory */
	FreeDir(&gp->dir, 1);
	FreeDir(&gp->files, 0);

	gp->files.cnt = 0;
	gp->dir.cnt = 1;

	/* Use a flag to skip stats if directory is /.NetWare since we know only
	 * directories can exist in it.  This is to avoid dtm being tied up while
	 * doing stats on NetWare servers, which can take a significantly long
	 * time, proportional to the number of servers.
	 */
	if (strcmp(_abi_realpath(gp->directory, Dm__buffer), "/.NetWare") == 0)
		skip_stat = True;

	valid_pattern = (filter[0] != '\0') ? COMPILE(&top, filter, flags) : 0;
	while ((dep = readdir(dp)) != NULL) {
		if (strcmp(dep->d_name, dot)==0 || strcmp(dep->d_name, dotdot)==0) {
			continue;
		}
		if (showHidden == False && dep->d_name[0] == dot[0]) {
			continue;  /* Skip current and parent directories */
		}
		/* Put the file names and directory names into lists. */
		/* Assume files that can't be stat'ed are symbolically linked files */
		/* with no valid link. */
		if (skip_stat ||
			((stat(dep->d_name, &statBuf) >= 0) &&
			((statBuf.st_mode & S_IFMT) == S_IFDIR))
		) {
			if(dep->d_name[0] == dot[0]) {
				continue;
			}
			/* Alloc more space only if needed */
			MoreSpace(&gp->dir);
			gp->dir.data[gp->dir.cnt].names = STRDUP(dep->d_name);
			gp->dir.data[gp->dir.cnt].ftype = DM_FTYPE_DIR;
			gp->dir.cnt += 1;
		}
		else {
			/* Only add files if file container exists */
			if (gp->dialogType == FOLDERS_ONLY) {
				continue;
			}
			if (filter[0] != '\0') {
				if (!valid_pattern ||
					!MATCH(&top, dep->d_name, filter, flags)) {
					/* Skip files that don't match the filter */
					filteredOut += 1;
					continue;
				}
			}
			/* Alloc more space only if needed */
			MoreSpace(&gp->files);
			gp->files.data[gp->files.cnt].names = STRDUP(dep->d_name);
			switch(statBuf.st_mode & S_IFMT) {
				case S_IFIFO: {
					gp->files.data[gp->files.cnt].ftype = DM_FTYPE_FIFO;
					break;
				}
				case S_IFCHR: {
					gp->files.data[gp->files.cnt].ftype = DM_FTYPE_CHR;
					break;
				}
				case S_IFBLK: {
					gp->files.data[gp->files.cnt].ftype = DM_FTYPE_BLK;
					break;
				}
				case S_IFREG: {
					gp->files.data[gp->files.cnt].ftype = DM_FTYPE_DATA;
					if (
						(statBuf.st_mode & S_IXUSR) ||
						(statBuf.st_mode & S_IXGRP) ||
						(statBuf.st_mode & S_IXOTH)
					) {
						gp->files.data[gp->files.cnt].ftype = DM_FTYPE_EXEC;
					}
					break;
				}
				case S_IFNAM: {
					gp->files.data[gp->files.cnt].ftype = DM_FTYPE_SHD;
					if (statBuf.st_rdev == S_INSEM) {
						gp->files.data[gp->files.cnt].ftype = DM_FTYPE_SEM;
					}
				}
			}
			gp->files.cnt += 1;
		}
	}
	closedir(dp);
	if (valid_pattern)
		FREE_FNP(&top);
	if (oldDir) {
		chdir(oldDir);
		FREE(oldDir);
	}

	/* Sort the names and make localize strings of the names */
	SortAndLocalize(&gp->dir, 1);
	SortAndLocalize(&gp->files, 0);

	/* Set the items in the files list */
	if (gp->files.cnt > 0 && gp->files.cw) {
		XtSetArg(arg[0], XmNitems, gp->files.xmsnames);
		XtSetArg(arg[1], XmNitemCount, gp->files.cnt);
		XtSetArg(arg[2], XmNvisibleItemCount, VIEW_HEIGHT);
		XtSetValues(gp->files.cw, arg, 3);
	}

	/* Set the items in the folder list */
	if (gp->dir.cnt > 0 && gp->dir.cw) {
		XtSetArg(arg[0], XmNitems, gp->dir.xmsnames);
		XtSetArg(arg[1], XmNitemCount, gp->dir.cnt);
		XtSetArg(arg[2], XmNvisibleItemCount, VIEW_HEIGHT);
		XtSetValues(gp->dir.cw, arg, 3);
	}

	/* Set the Files (n filtered out) label */
	if (filteredLabel == NULL) {
		filteredLabel = GGT(TXT_FILES_FILTERED);
	}

	sprintf(buf, filteredLabel, filteredOut);
	string = XmStringCreateLocalized(buf);

	/* is invoked from CreateFileGizmo() */
	if (thisString) {
		*thisString = string;
	}
	else {
		if (gp->filesFilteredLabel) {
			XtSetArg(arg[0], XmNlabelString, string);
			XtSetValues(gp->filesFilteredLabel, arg, 1);
		}
		XmStringFree(string);
	}
	FREE(input);

	return True;
}

/*
 * Callback when file selected in file list.
 */
static void
DirSelectCB(Widget w, XtPointer clientData, XtPointer callData)
{
	char					buf[1024];
	char *					name;
	FileGizmoP *			gp = (FileGizmoP *)clientData;
	XmListCallbackStruct *	list = (XmListCallbackStruct *)callData;
	char *					filter;
	
	if (gp->dialogType == FOLDERS_AND_FILES) {
		filter = XmTextGetString(gp->inputFieldWidget);
		if (RegularExpression(filter) != True) {
			/* If the input field doesn't have a re then put in gizmo filter */
			/* but only if the input field isn't blank. */
			if (Blank(filter) == True) {
				gp->filter[0] = '\0';
			}
			XmTextFieldSetString(gp->inputFieldWidget, gp->filter);
		}
		FREE(filter);
	}

	name = gp->dir.data[list->item_position-1].names;
	strcpy(buf, gp->directory);
	if (strcmp(gp->directory, slash) != 0) {
		strcat(buf, slash);
	}
	strcat(buf, name);
	FREE(gp->prevDir);
	gp->prevDir = gp->directory;
	gp->directory = STRDUP(buf);
	FixDirectory(gp);
	ReadDirectory(gp, NULL);
}

void
ResetFileGizmoPath(Gizmo g)
{
	FileGizmoP *	gp = (FileGizmoP *)g;
	char			buf[BUFSIZ];

	FREE(gp->directory);
	(void)getcwd(buf, BUFSIZ);
	gp->directory = STRDUP(buf);
	FixDirectory(gp);
	ReadDirectory(gp, NULL);
}

/*
 * Callback when file selected in file list.
 */
static void
FileSelectCB(Widget w, XtPointer clientData, XtPointer callData)
{
	FileGizmoP *			gp = (FileGizmoP *)clientData;
	XmListCallbackStruct *	list = (XmListCallbackStruct *)callData;
	
	XmTextFieldSetString(
		gp->inputFieldWidget, gp->files.data[list->item_position-1].names
	);
}

static void
CheckBoxChangeCB(Widget w, XtPointer clientData, XtPointer callData)
{
	FileGizmoP *		gp = (FileGizmoP *)clientData;

	ReadDirectory(gp, NULL);
}

static DmFclassPtr	    Icons[NUM_ICONS] = {	/* Pointers to icons */
	NULL
};

static void
ItemInitProc(Widget w, XtPointer clientData, XtPointer callData)
{
	ListRec *						lp = (ListRec *)clientData;
	XmListItemInitCallbackStruct *	cd;
	int								i;
	DmGlyphPtr						glyph;

	cd = (XmListItemInitCallbackStruct *)callData;
	i = cd->position-1;

	if (cd->reason != XmCR_ASK_FOR_ITEM_DATA) {
		return;
	}
	glyph = Icons[lp->data[i].ftype]->glyph;
	cd->pixmap = glyph->pix;
	cd->mask = glyph->mask;
	cd->depth = glyph->depth;
	cd->width = glyph->width;
	cd->height = glyph->height;
	cd->h_pad = 5;
	cd->v_pad = 0;
	cd->glyph_pos = XmGLYPH_ON_LEFT;
	cd->static_data = True;

	lp->xmsnames[i] = XmStringCreateLocalized(lp->data[i].names);
	cd->label = lp->xmsnames[i];
}

static void
CreateList(FileGizmoP* gp, ListRec *lp, Widget parent, Widget attach)
{
	Arg		arg[20];
	int		i;
	XtCallbackRec	cb[2];

	if (parent == NULL) {
		return;
	}
	i = 0;
	XtSetArg(arg[i], XmNvisibleItemCount, VIEW_HEIGHT); i++;
	XtSetArg(arg[i], XmNtopAttachment, XmATTACH_WIDGET); i++;
	XtSetArg(arg[i], XmNtopWidget, attach); i++;
	if (gp->dialogType == FOLDERS_AND_FILES) {
		if (lp == &gp->files) {
			/* Attach the file list to the directory list and the form */
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_POSITION); i++;
			XtSetArg(arg[i], XmNleftPosition, 3); i++;
			XtSetArg(arg[i], XmNleftOffset, 3); i++;
		}
		else {
			/* Attach the directory list to the form */
			XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
			XtSetArg(arg[i], XmNrightAttachment, XmATTACH_POSITION); i++;
			XtSetArg(arg[i], XmNrightPosition, 3); i++;
			XtSetArg(arg[i], XmNrightOffset, 3); i++;
		}
	}
	else {
		/* If this is dealing with folders only then attach the list */
		/* to positions in order to achieve centering */
		XtSetArg(arg[i], XmNrightAttachment, XmATTACH_POSITION); i++;
		XtSetArg(arg[i], XmNrightPosition, 5); i++;
		XtSetArg(arg[i], XmNleftAttachment, XmATTACH_POSITION); i++;
		XtSetArg(arg[i], XmNleftPosition, 1); i++;
	}
	cb[0].closure = (XtPointer)lp;
	cb[0].callback = ItemInitProc;
	cb[1].closure = NULL;
	cb[1].callback = NULL;
	XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNselectionPolicy, XmSINGLE_SELECT); i++;
	XtSetArg(arg[i], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); i++;
	XtSetArg(arg[i], XmNitemInitCallback, (XtArgVal)cb); i++;
	XtSetArg(arg[i], XmNitems, (XtArgVal)lp->xmsnames); i++;
	XtSetArg(arg[i], XmNitemCount, (XtArgVal)lp->cnt); i++;

	lp->cw = (Widget)XmCreateScrolledList(parent, "scrolledList", arg, i);
	if (lp == &gp->dir) {
		/* Callback only good for directories */
		XtAddCallback(lp->cw, XmNsingleSelectionCallback, DirSelectCB, gp);
	}
	else {
		XtAddCallback(lp->cw, XmNsingleSelectionCallback, FileSelectCB, gp);
	}
	XtManageChild(lp->cw);
}

static void
CreatePathArea(FileGizmo *g, FileGizmoP *gp, Widget rc)
{
	Widget		form;
	XmString	string;
	int			i;
	Arg			arg[10];

	form = XtCreateManagedWidget(
		"pathAreaForm", xmFormWidgetClass, rc, NULL, 0
	);

	/* Create the label on the path field */
	string = XmStringCreateLocalized(GGT(g->pathLabel));
	i = 0;
	XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	gp->pathAreaLabel = XtCreateManagedWidget(
		"pathLabel", xmLabelGadgetClass, form, arg, i
	);
	XmStringFree(string);

	/* Create the path field */
	string = FormatPath(gp->directory);
	i = 0;
	XtSetArg(arg[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
	XtSetArg(arg[i], XmNleftWidget, gp->pathAreaLabel); i++;
	XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	gp->pathAreaWidget = XtCreateManagedWidget(
		"pathText", xmLabelGadgetClass, form, arg, i
	);
	XmStringFree(string);
}

static void
ActivateCB(Widget w, XtPointer clientData, XtPointer callData)
{
	FileGizmoP *	gp = (FileGizmoP *)clientData;
	char *			filter;

	filter = XmTextGetString(gp->inputFieldWidget);
	if (filter[0] == '\0') {
		FREE(gp->filter);
		gp->filter = STRDUP("");
	}
	FREE(filter);

	ReadDirectory(gp, NULL);
}

static void
CreateInputField(FileGizmo *g, FileGizmoP *gp, Widget rc)
{
	Widget		form;
	XmString	string;
	int			i;
	Arg			arg[10];

	form = XtCreateManagedWidget(
		"inputAreaForm", xmFormWidgetClass, rc, NULL, 0
	);

	/* Create the label on the input field */
	string = XmStringCreateLocalized(GGT(g->inputLabel));
	i = 0;
	XtSetArg(arg[i], XmNleftAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNalignment, XmALIGNMENT_END); i++;
	XtSetArg(arg[i], XmNlabelString, string); i++;
	gp->inputFieldLabel = XtCreateManagedWidget(
		"inputLabel", xmLabelGadgetClass, form, arg, i
	);
	XmStringFree(string);

	/* Create the input field */
	i = 0;
	XtSetArg(arg[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
	XtSetArg(arg[i], XmNleftWidget, gp->inputFieldLabel); i++;
	XtSetArg(arg[i], XmNbottomAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
	XtSetArg(arg[i], XmNeditable, True); i++;
	gp->inputFieldWidget = XtCreateManagedWidget(
		"inputField", xmTextFieldWidgetClass, form, arg, i
	);
	XtAddCallback(
		gp->inputFieldWidget, XmNactivateCallback, ActivateCB,
		(XtPointer)gp
	);
}

/* Create the two scrolling icon containers */
static void
CreateIconSW(FileGizmoP *gp, Widget form)
{
	char		buf[1024];
	Widget		attach;
	XmString	string;
	int		i = 0;
	Arg		arg[10];

	if (gp->dialogType == FOLDERS_AND_FILES) {
		/* Create "Folders" label */
		string = XmStringCreateLocalized(GGT(TXT_FOLDERS));
		i = 0;
		XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNlabelString, string); i++;
		attach = XtCreateManagedWidget(
			"pathLabel", xmLabelGadgetClass, form, arg, i
		);
		XmStringFree(string);

		CreateList(gp, &gp->dir, form, attach);

		/* Create "Files (n filtered out)" label */
		string = XmStringCreateLocalized(GGT(TXT_FILES));
		i = 0;
		XtSetArg(arg[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
		XtSetArg(arg[i], XmNleftWidget, gp->dir.cw); i++;
		XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNlabelString, string); i++;
		gp->filesFilteredLabel = XtCreateManagedWidget(
			"Files", xmLabelGadgetClass, form, arg, i
		);
		XmStringFree(string);

		CreateList(gp, &gp->files, form, gp->filesFilteredLabel);
	}
	else {
		/* Create the "Folders In:" field */
		i = 0;
		XtSetArg(arg[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		XtSetArg(arg[i], XmNtopAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNrightAttachment, XmATTACH_FORM); i++;
		XtSetArg(arg[i], XmNleftAttachment, XmATTACH_POSITION); i++;
		XtSetArg(arg[i], XmNleftPosition, 1); i++;
		gp->inFieldWidget = XtCreateManagedWidget(
			"pathLabel", xmLabelGadgetClass, form, arg, i
		);

		CreateList(gp, &gp->dir, form, gp->inFieldWidget);
	}

}

/* Create the show hidden menu */
static void
CreateShowHidden(FileGizmoP *gp, Widget rc)
{
	Widget		button;

	if (gp->dialogType == FOLDERS_ONLY) {
		gp->checkBoxWidget = NULL;
		return;
	}
	gp->checkBoxWidget = XtCreateManagedWidget(
		GGT(TXT_SHOW_HIDDEN), xmToggleButtonGadgetClass, rc, NULL, 0
	);
	XtAddCallback(
		gp->checkBoxWidget, XmNvalueChangedCallback, CheckBoxChangeCB,
		(XtPointer)gp
	);
}

static void
AlignCaptions(FileGizmoP *gp)
{
	int				i;
	LabelGizmoP *	lg;
	Dimension		maxWidth = 0;
	Dimension		w;
	Arg				arg[5];

	/* Get the maximum width of all the labels in the upper portion */
	/* of the file Gizmo. */
	XtSetArg(arg[0], XmNwidth, &w);
	for (i=0; i<gp->numUpper; i++) {
		if (gp->upperGizmos[i].gizmoClass == LabelGizmoClass) {
			lg = (LabelGizmoP *)gp->upperGizmos[i].gizmo;
			XtGetValues(lg->label, arg, 1);
			if (w > maxWidth) {
				maxWidth = w;
			}
		}
	}
	XtGetValues(gp->pathAreaLabel, arg, 1);
	if (w > maxWidth) {
		maxWidth = w;
	}
	for (i=0; i<gp->numLower; i++) {
		if (gp->lowerGizmos[i].gizmoClass == LabelGizmoClass) {
			lg = (LabelGizmoP *)gp->lowerGizmos[i].gizmo;
			XtGetValues(lg->label, arg, 1);
			if (w > maxWidth) {
				maxWidth = w;
			}
		}
	}
	XtGetValues(gp->inputFieldLabel, arg, 1);
	if (w > maxWidth) {
		maxWidth = w;
	}

	/* Now, set all the widths of the labels to this maximum width */
	XtSetArg(arg[0], XmNwidth, maxWidth);
	for (i=0; i<gp->numUpper; i++) {
		if (gp->upperGizmos[i].gizmoClass == LabelGizmoClass) {
			lg = (LabelGizmoP *)gp->upperGizmos[i].gizmo;
			XtSetValues(lg->label, arg, 1);
		}
	}
	XtSetValues(gp->pathAreaLabel, arg, 1);
	for (i=0; i<gp->numLower; i++) {
		if (gp->lowerGizmos[i].gizmoClass == LabelGizmoClass) {
			lg = (LabelGizmoP *)gp->lowerGizmos[i].gizmo;
			XtSetValues(lg->label, arg, 1);
		}
	}
	XtSetValues(gp->inputFieldLabel, arg, 1);
}

static char *	iconFilenames[] = {
	"sunk.icon",
	"sdir.icon",
	"sexec.icon",
	"sdatafile.icon",
	"spipe.icon",
	"schrdev.icon",
	"sblkdev.icon",
	"ssem.icon",
	"sshmem.icon",
};

static Gizmo
CreateFileGizmo (Widget parent, FileGizmo *g, Arg *args, int numArgs)
{
	Arg				arg[100];

	ListRec *		list;
	FileGizmoP *	gp = (FileGizmoP *)CALLOC(1, sizeof(FileGizmoP));
	int			i;
	long			dec;
	long			func;
	Widget			rc;
	Widget			form, form1;
	DmMnemonicInfo		mneInfo;
	Cardinal		numMne;
	Boolean			flag;
	XmString		thisString;

	gp->name = g->name;
	gp->dialogType = g->dialogType;
	gp->filter = STRDUP("");
	if (g->directory == NULL || g->directory[0] == '\0') {
		gp->directory = STRDUP(dot);
	}
	else {
		gp->directory = STRDUP(g->directory);
	}
	(char *)getcwd(NULL, FILENAME_MAX);
	gp->prevDir = (char *)getcwd(NULL, FILENAME_MAX);
	FixDirectory(gp);

	i = 0;
	dec = MWM_DECOR_TITLE|MWM_DECOR_BORDER|MWM_DECOR_MENU;
	func = MWM_FUNC_MOVE|MWM_FUNC_CLOSE;
	XtSetArg(arg[i], XmNtitle, GGT(g->title)); i++;
	XtSetArg(arg[i], XmNmwmDecorations, dec); i++;
	XtSetArg(arg[i], XmNmwmFunctions, func); i++;
	XtSetArg(arg[i], XmNallowShellResize, True); i++;
	i = AppendArgsToList(arg, i, args, numArgs);
	gp->shell = XtCreatePopupShell(
		gp->name, xmDialogShellWidgetClass, parent, arg, i
	);

	form = XtVaCreateWidget(
		"form", xmFormWidgetClass, gp->shell,
		XmNresizePolicy, XmRESIZE_ANY,
		NULL
	);
	gp->row_column = rc = XtCreateManagedWidget(
		"top rc", xmRowColumnWidgetClass, form, NULL, 0
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(rc, g->help);
	}

	/* Create the first array of Gizmos */
	if (g->numUpper > 0) {
		gp->numUpper = g->numUpper;
		gp->upperGizmos = CreateGizmoArray(rc, g->upperGizmos, g->numUpper);
	}

	/* Create the Path label and field */
	CreatePathArea(g, gp, rc);

	/* Create the second array of Gizmos */
	if (g->numLower > 0 && g->lower_gizmo_pos == ABOVE_LIST) {
		gp->numLower = g->numLower;
		gp->lowerGizmos = CreateGizmoArray(rc, g->lowerGizmos, g->numLower);
	}

	/* Create the Input label and field */
	CreateInputField(g, gp, rc);

	AlignCaptions(gp);

	/* Create the "Items In:" field */
	if (gp->dialogType == FOLDERS_AND_FILES) {
		XtSetArg(arg[0], XmNalignment, XmALIGNMENT_BEGINNING); i++;
		gp->inFieldWidget = XtCreateManagedWidget(
			"pathLabel", xmLabelGadgetClass, rc, arg, 1
		);
	}

	/* Create the parent of the two scrolling icon containers here,
	 * but defering the child(ren) creation later... */
	XtSetArg(arg[0], XmNfractionBase, 6);
	form1 = XtCreateManagedWidget(
		"form", xmFormWidgetClass, rc, arg, 1
	);

	/* Create the second array of Gizmos */
	if (g->numLower > 0 && g->lower_gizmo_pos == BELOW_LIST) {
	    /* FLH MORE: should we align the captions after this ?*/
	    gp->numLower = g->numLower;
	    gp->lowerGizmos = CreateGizmoArray(rc, g->lowerGizmos, g->numLower);	
	}

	/* Create the show hidden menu */
	CreateShowHidden(gp, rc);

	if (g->menu != NULL) {
		gp->menu = _CreateActionMenu(
			rc, g->menu, NULL, 0, &mneInfo, &numMne
		);
	}

	/* Initialize the file type icons */
	if (Icons[0] == NULL) {
		for (i=0; i<NUM_ICONS; i++) {
			Icons[i] = (DmFclassPtr)CALLOC(1,sizeof(DmFclassRec));
			Icons[i]->glyph = DmGetPixmap(
				XtScreen(parent), iconFilenames[i]
			);
			Icons[i]->cursor = DmGetCursor(
				XtScreen(parent), iconFilenames[i]
			);
		}
	}

		/* Was defined in CreateList() */
	gp->files.size = gp->dir.size = 50;

	gp->files.xmsnames =(XmString*)CALLOC(gp->files.size, sizeof(XmString));
	gp->dir.xmsnames = (XmString *)CALLOC(gp->dir.size, sizeof(XmString));
	gp->files.data = (ListData *)CALLOC(gp->files.size, sizeof(ListData));
	gp->dir.data = (ListData *)CALLOC(gp->dir.size, sizeof(ListData));

	/* Populate first item in folder list with "Parent" */
	gp->dir.data[0].names = STRDUP(dotdot);
	gp->dir.data[0].ftype = DM_FTYPE_DIR;
	gp->dir.xmsnames[0] = XmStringCreateLocalized(GGT(TXT_PARENT));

	flag = ReadDirectory(gp, &thisString);
	CreateIconSW(gp, form1);

		/* Complete the work should be done in ReadDirectory() but
		 * couldn't because the CreateIconSW() is called after
		 * ReadDirectory() now */
	if (flag) {	/* Do work only if no error from ReadDirectory() */

		SetPath(gp);

		if (gp->filesFilteredLabel) {
			XtSetArg(arg[0], XmNlabelString, thisString);
			XtSetValues(gp->filesFilteredLabel, arg, 1);
		}
		XmStringFree(thisString);
	}

	XtManageChild(rc);
	if (g->menu != NULL) {
		DmRegisterMnemonic(gp->shell, mneInfo, numMne);
		FREE(mneInfo);
	}
	return gp;
}

/*
 * MapFileGizmo
 */
static void
MapFileGizmo(FileGizmoP *gp)
{
	Widget	shell = gp->shell;


	/* manage the child because the dialogShell unmanages its
	 * child when it is popped down via the window menu
	 */
	XtManageChild(XtParent(gp->row_column));
	_SetMenuDefault(gp->menu, XtParent(gp->row_column));
	XmProcessTraversal(gp->inputFieldWidget, XmTRAVERSE_CURRENT);
	XtPopup(shell, XtGrabNone);
	XRaiseWindow(XtDisplay(shell), XtWindow(shell));
}

/*
 * ManipulateFileGizmo
 */
static void
ManipulateFileGizmo(FileGizmoP *gp, ManipulateOption option)
{
	GizmoArray	gu = gp->upperGizmos;
	GizmoArray	gl = gp->lowerGizmos;
	int		i;

	for (i=0; i < gp->numUpper; i++) {
		ManipulateGizmo(gu[i].gizmoClass, gu[i].gizmo, option);
	}
	for (i=0; i < gp->numLower; i++) {
		ManipulateGizmo(gl[i].gizmoClass, gl[i].gizmo, option);
	}
}

/*
 * QueryFileGizmo
 */
static XtPointer
QueryFileGizmo(FileGizmoP *gp, int option, char *name)
{
	XtPointer	value;

	if (!name || strcmp(name, gp->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(gp->shell);
			}
			case GetGizmoGizmo: {
				return (XtPointer)gp;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	}
	else {
		if (gp->menu != NULL) {
			value = QueryGizmo(CommandMenuGizmoClass, gp->menu, option, name);
			if (value != NULL) {
				return value;
			}
		}
		value = (XtPointer)QueryGizmoArray(
			gp->upperGizmos, gp->numUpper, option, name
		);
		if (value != NULL) {
			return value;
		}
		value = (XtPointer)QueryGizmoArray(
			gp->lowerGizmos, gp->numLower, option, name
		);
		return value;
	}
}

/*
 * GetFileGizmoShell
 */
Widget
GetFileGizmoShell(Gizmo g)
{
	return ((FileGizmoP *)g)->shell;
}

/*
 * GetFileGizmoRowCol
 */
Widget
GetFileGizmoRowCol(Gizmo g)
{
	return ((FileGizmoP *)g)->row_column;
}

/*
 * GetFilePath - Return the path including the file.  
 *
 *	PATH CONSTRUCTION:  If the user has typed an absolute
 * 	pathname, it is returned as the path.  Otherwise, the path
 *	is constructed by concatenating the current directory
 *	(stored in the gizmo) and the user input.
 *		
 * NOTE: returned string must be freed by caller
 */
char *
GetFilePath(Gizmo g)
{
	char *			file;
	char *			path;
	FileGizmoP *	gp = (FileGizmoP *)g;

	/*
	if (gp == NULL) {
		return NULL;
	}
	*/
	file = XmTextGetString(gp->inputFieldWidget);
	if (RegularExpression(file) == True) {
		FREE(file);
		return NULL;
	}

	path = MALLOC(strlen(gp->directory) + strlen(file) + 2);
	/* if there is no input in textfield, just use current dir */
	if ((file == NULL) || (file[0] == '\0')) {
	    strcpy(path, gp->directory);
	} 
	else if (file[0] != '/') {
	    /* Input field contains relative pathname, append to current dir */
	    strcpy(path, gp->directory);
	    strcat(path, slash);
	    strcat(path, file);
        } 
	else{
	    /* Input field contains absolute pathname */
	    strcpy(path, file);
	}

	FREE(file);
	
	return path;	/* Must be freed */
}

void
ExpandFileGizmoFilename(Gizmo g)
{
	FileGizmoP *	gp = (FileGizmoP *)g;

	/*
	if (gp != NULL) {
	*/
		ReadDirectory(gp, NULL);
	/*
	}
	*/
}

extern void
SelectFileGizmoInputField(Gizmo g)
{
    FileGizmoP *	gp = (FileGizmoP *)g;

	XmTextPosition first = (XmTextPosition) 0;
	XmTextPosition last = XmTextFieldGetLastPosition(gp->inputFieldWidget);


	XmTextFieldSetInsertionPosition(gp->inputFieldWidget, last);
	XmTextFieldSetSelection(gp->inputFieldWidget, first, last, CurrentTime);

} /* end of SelectFileGizmoInputField */

void
SetFileGizmoInputField(Gizmo g, String value)
{	
    FileGizmoP *	gp = (FileGizmoP *)g;
	Arg				arg[5];
	
	XtSetArg(arg[0], XmNvalue, value);
	XtSetValues(gp->inputFieldWidget, arg, 1);

} /* end of SetFileGizmoInputField */


/*
 * SetFileGizmoInputLabel: Set the label for the input field
 *  
 *		NOTE: caller should call gettxt and pass i18n'd string
 *		This string doesn't call gettxt to avoid too many calls
 *		for the same string. (The caller may want to frequently 
 *		swap between two different labels)
 */
void
SetFileGizmoInputLabel(Gizmo g, String value)
{	
    FileGizmoP *	gp = (FileGizmoP *)g;
	Arg				arg[5];
	XmString		string;
	

	string = XmStringCreateLocalized(value);
	XtSetArg(arg[0], XmNlabelString, string);
	XtSetValues(gp->inputFieldLabel, arg, 1);
	XmStringFree(string);
    AlignCaptions(gp);
} /* end of SetFileGizmoInputLabel*/
