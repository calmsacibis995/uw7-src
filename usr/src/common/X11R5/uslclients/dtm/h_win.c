#pragma ident	"@(#)dtm:h_win.c	1.128"

/******************************file*header********************************

    Description:
     This file contains the source code for creating a help window
	and displaying a section or a string in a help file, popping and
	pushing a "visited" section from/onto a stack, popping up the help
	window, and closing a help window.
*/
                              /* #includes go here     */
#ifdef DEBUG
#include <X11/Xmu/Editres.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>       /* for XmAddWMProtocolCallback */
#include <Xm/ScrolledW.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Private Procedures
          2. Public  Procedures
*/
                         /* private procedures         */

static void DmHelpWinMenuCB(Widget w, XtPointer client_data,
	XtPointer call_data);
static int CreateHelpWindow(DmHelpAppPtr hap);
static void WMCB(Widget w, XtPointer client_data, XtPointer call_data);
static void PopupHelpWindow(DmHelpWinPtr hwp);
static void DisplayHelpString(DmHelpWinPtr hwp, char *title, char *string,
			Boolean realized);
static void EventHandler(Widget w, XtPointer client_data, XEvent *xevent,
			Boolean *continue_to_use);
static void SaveHelpInfo(DmHelpWinPtr hwp, char **file, char **sect_name,
	char **sect_tag);
static void FreeHelpInfo(char *file, char *sect_name, char *sect_tag);
static void SwitchHelpFile(DmHelpWinPtr hwp, char *file, char *sect_name,
	char *sect_tag);
static void SetSwinSize(DmHelpWinPtr hwp);
void DmPushHelpLoc(DmHelpWinPtr, char *, char *, char *, unsigned int);
void DmShowScrolledSection(DmHelpAppPtr, char *, char *, char *, unsigned int,
				Boolean);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define STACK_STEP	16

static MenuItems BtnBarMenuItems[] = {
 { False, TXT_HW_BACKTRACK,  TXT_M_BACKTRACK,      I_PUSH_BUTTON, NULL,
	DmBackTrackCB,    NULL, False, },
 { False, TXT_HW_NEXT,       TXT_M_HW_NEXT,        I_PUSH_BUTTON, NULL,
	DmNextSectionCB,  NULL, False, },
 { False, TXT_HW_PREV,       TXT_M_HW_PREV,       I_PUSH_BUTTON, NULL,
	DmPrevSectionCB,  NULL, True, },
 { True, TXT_HW_SEARCH,      TXT_M_HW_SEARCH,     I_PUSH_BUTTON, NULL,
	DmSearchCB,       NULL, False, },
 { True, TXT_HW_BOOKMARK,    TXT_M_HW_BOOKMARK,   I_PUSH_BUTTON, NULL,
	DmBookmarkCB,     NULL, False, },
 { True, TXT_HW_NOTES,       TXT_M_NOTES,         I_PUSH_BUTTON, NULL,
	DmNotesCB,        NULL, False, },
 { True, TXT_CONTENTS,       TXT_M_CONTENTS,      I_PUSH_BUTTON, NULL,
	DmTOCCB,          NULL, False, },
 { True, TXT_HW_GLOSSARY,    TXT_M_HW_GLOSSARY,   I_PUSH_BUTTON, NULL,
	DmGlossaryCB,     NULL, False, },
 { True, TXT_HELPDESK_TITLE, TXT_M_HELP_HELPDESK2, I_PUSH_BUTTON, NULL,
	DmOpenHelpDeskCB, NULL, False, },
 { True, TXT_HW_USING_HELP,  TXT_M_HW_USING_HELP, I_PUSH_BUTTON, NULL,
	DmUsingHelpCB,    NULL, False, },
 { NULL }
};

static MenuGizmo BtnBarMenu = {
	NULL, "BtnBarMenu", "_X_", BtnBarMenuItems, NULL, NULL,
	XmHORIZONTAL, 2, 1
};

static ContainerGizmo swin_gizmo = {
	NULL,			/* help */
	"swin",			/* name */
	G_CONTAINER_SW,		/* container type */
};

static GizmoRec hwin_gizmos[] = {
	{ CommandMenuGizmoClass, &BtnBarMenu },
	{ ContainerGizmoClass, &swin_gizmo },
};

static MsgGizmo msg_gizmo = {NULL, "footer", " ", " "};

static BaseWindowGizmo HelpWindow = {
	NULL,			/* help */
	"helpwindow",		/* shell widget name */
	"",			/* title */
	NULL, 			/* menu bar */
	hwin_gizmos,		/* gizmo array */
	XtNumber(hwin_gizmos),	/* # of gizmos in array */
	&msg_gizmo,		/* footer */
	"Help Window",		/* icon_name */
	"help48",		/* name of pixmap file */
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
 */
static void
DmHelpWinMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
} /* end of DmHelpWinMenuCB */

/****************************procedure*header*****************************
 * This function creates a help window, open the help file, and display
 * the specified section in the help window. If file_name or sect_name
 * are NULL, it means the current file_name or sect_name.
 */
static int
CreateHelpWindow(DmHelpAppPtr hap)
{
	int n;
	Arg args[10];
	DmHelpWinPtr hwp;
	BaseWindowGizmo *base;

	hwp = &(hap->hlp_win);
	BtnBarMenu.clientData = (XtPointer)hwp;

	n = 0;
        XtSetArg(args[n], XmNshadowThickness, 2); n++;
        XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
        XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++;
        XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
	hwin_gizmos->args = args;
	hwin_gizmos->numArgs = n;

	XtSetArg(Dm__arg[0], XmNiconic, False);
	XtSetArg(Dm__arg[1], XmNmappedWhenManaged, False);
	base = CreateGizmo(NULL, BaseWindowGizmoClass, &HelpWindow,
		Dm__arg, 2);

	hwp->gizmo_shell = base;
	hwp->shell       = GetBaseWindowShell(base);
	hwp->attrs       = DM_B_HELP_WIN;

	hwp->menubar = QueryGizmo(BaseWindowGizmoClass, base, GetGizmoWidget,
			"BtnBarMenu");

	hwp->swin = QueryGizmo(BaseWindowGizmoClass, base, GetGizmoWidget,
			"swin");
	n = 0;
        XtSetArg(Dm__arg[n], XmNshadowThickness, 2); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget, hwp->menubar); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset, 4); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset, 4); n++;
	XtSetValues(hwp->swin, Dm__arg, n);

	n = 0;
	XtSetArg(Dm__arg[n], XmNrows, HELP_WINDOW_NUM_ROWS); n++;
	XtSetArg(Dm__arg[n], XmNcolumns, HELP_WINDOW_NUM_COLS); n++;
	hwp->htext = XtCreateManagedWidget("hyperText",exmHyperTextWidgetClass,
			hwp->swin, Dm__arg, n);
	XtAddCallback(hwp->htext, XmNselect, DmHtextSelectCB, (XtPointer)hwp);
	SetSwinSize(hwp);

	hwp->defWin = NULL;
	hwp->glossWin = NULL;
	hwp->notesWin = NULL;
	hwp->searchWin = NULL;
	hwp->bmarkWin = NULL;
	hwp->search_str = NULL;
	hwp->search_file = NULL;
	hwp->help_files = NULL;
	hwp->filep = NULL;
	hwp->linecnt = 0;
	hwp->index = 0;
	hwp->found = False;

#ifdef DEBUG
	XtAddEventHandler(hwp->shell,  (EventMask) 0, True,
		_XEditResCheckMessages, NULL);
#endif

	return(0);

} /* end of CreateHelpWindow */

/****************************procedure*header*****************************
 * Event handler to track UnmapNotify events.  This is needed to destroy
 * a help window when a help window is closed using the keyboard.
 */
static void
EventHandler(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use)
{
	if (xevent->type == UnmapNotify) {
		DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

		if (hwp->shell) {
          		DmCloseHelpWindow(hwp);
		}
	}
} /* end of EventHandler */

/****************************procedure*header*****************************
 * Makes a copy of help file name, section name and section tag.
 * Called when help file and/or section is/are changed.
 */
static void
SaveHelpInfo(DmHelpWinPtr hwp, char **file, char **sect_name, char **sect_tag)
{
	/* save these for DmPushHelpSection() */
	if (hwp->hfp->name)
		*file = strdup(hwp->hfp->name);
	if (hwp->hsp->name)
		*sect_name = strdup(hwp->hsp->name);
	if (hwp->hsp->tag)
		*sect_tag  = strdup(hwp->hsp->tag);

} /* end of SaveHelpInfo */

/****************************procedure*header*****************************
 * Frees help file name, section name and section tag.
 */
static void
FreeHelpInfo(file, sect_name, sect_tag)
char *file;
char *sect_name;
char *sect_tag;
{
	if (file)
		FREE(file);
	if (sect_name)
		FREE(sect_name);
	if (sect_tag)
		FREE(sect_tag);

} /* end of FreeHelpInfo */

/****************************procedure*header*****************************
 * Pushes current section onto a stack, closes current help file
 * and resets current help file and section pointers in hwp. 
 */
static void
SwitchHelpFile(DmHelpWinPtr hwp, char *file, char *sect_name, char *sect_tag)
{
	DmPushHelpStack(hwp, file, sect_name, sect_tag);
	DmCloseHelpFile(hwp->hfp);
	hwp->hfp = NULL;
	hwp->hsp = NULL;

} /* end of SwitchHelpFile */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * This function displays the specified section in the help window.
 * A history stack of previous displayed locations are kept in the
 * window structure, so that users can backtrack.
 */
void
DmDisplayHelpSection(DmHelpAppPtr hap, char *title, char *file_name,
	char *sect_name)
{
	DmShowScrolledSection(hap, title, file_name, sect_name, 0, True);
}

void
DmShowScrolledSection(DmHelpAppPtr hap, char *title, char *file_name,
    char *sect_name, unsigned int line_num, Boolean raise)
{
	Widget notes_shell;
	char *file_sv = NULL;
	char *sectname_sv = NULL;
	char *secttag_sv = NULL;
	Boolean realized = True;
	DmHelpSectPtr hsp;
	DmHelpWinPtr hwp = &(hap->hlp_win);

	if (file_name == NULL && sect_name == NULL)
		return;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = hap->app_id;
		if (CreateHelpWindow(hap))
			return;
	} else {
		/* if requested section is being displayed, simply return */
		if (hwp->hfp && hwp->hfp->name &&
		    (!file_name || strcmp(hwp->hfp->name, file_name) == 0)) {
			if (hwp->hsp &&
			   ((sect_name && hwp->hsp->name &&
			     strcmp(hwp->hsp->name, sect_name) == 0) ||
		    	    (sect_name && hwp->hsp->tag &&
			     strcmp(hwp->hsp->tag, sect_name) == 0) ||
		            (sect_name == NULL && hwp->hsp->name &&
			     !strcmp(hwp->hsp->name, DEFAULT_SECTION_NAME)))) {
				/* Just map and raise the help window and
				 * return
				 */
				XtMapWidget(hwp->shell);
				if (raise)
					XRaiseWindow(XtDisplay(hwp->shell),
						XtWindow(hwp->shell));
				return;
			}
		}
		if (hwp->hsp && hwp->hfp)
			SaveHelpInfo(hwp, &file_sv, &sectname_sv, &secttag_sv);
		DmClearStatus((DmWinPtr)hwp);
	}

	if (file_name) {

		if (hwp->hfp == NULL || strcmp(hwp->hfp->name, file_name)) {

			DmHelpFilePtr hfp;

			if ((hfp = DmOpenHelpFile(hap, file_name)) == NULL) {
				if (hwp->hfp && hwp->hsp)
					SwitchHelpFile(hwp, file_sv,
						sectname_sv, secttag_sv);
				sprintf(Dm__buffer, "%s %s", hap->title,
					help_str);
				DisplayHelpString(hwp, Dm__buffer,
					(char *)nohelp_str, realized);
				FreeHelpInfo(file_sv, sectname_sv, secttag_sv);
				return;
			}
			/* release all previous resources */
			if (hwp->hfp)
				DmCloseHelpFile(hwp->hfp);
			hwp->hfp = hfp;
		}
	}
	if (sect_name)
	{
		if ((hsp = DmGetSection(hwp->hfp, sect_name)) == NULL)
		{
			if (hwp->hfp && hwp->hsp)
				SwitchHelpFile(hwp, file_sv, sectname_sv,
					secttag_sv);
			sprintf(Dm__buffer, "%s %s", hap->title, help_str);
			DisplayHelpString(hwp, Dm__buffer, (char *)nohelp_str,
				realized);
			DmVaDisplayStatus((DmWinPtr)hwp, True,
				TXT_SECT_NOT_FOUND, sect_name);
			FreeHelpInfo(file_sv, sectname_sv, secttag_sv);
			return;
		}
			
	} else {
		/*
		 * Defaults to the first section only if a new file
		 * is specified.
		 */
		if (file_name)
			hsp = hwp->hfp->sections;
	}
	if (hsp != hwp->hsp) {
		/* set up hypertext widget */
		DmHelpNotesPtr np;
		Widget vsb;
		WidgetList BtnBarWidgets;
		unsigned int save_line;

		/* save the current line number, so it can be saved
		 * for backtracking.
		 */
		XtSetArg(Dm__arg[0], XmNtopRow, &save_line);
		XtGetValues(hwp->htext, Dm__arg, 1);

		/* cook it first */
		DmProcessHelpSection(hsp);

		/*
		 * Help window title should be "<title> Help: <sect_name>".
		 */
		if (title)
			strcpy(Dm__buffer, title);
		else if (hwp->hfp->title)
			strcpy(Dm__buffer, hwp->hfp->title);
		else if (hap->title)
			strcpy(Dm__buffer, hap->title);
		strcat(Dm__buffer, " ");
		strcat(Dm__buffer, help_str);
		strcat(Dm__buffer, ": ");
		if (hsp->name)
			strcat(Dm__buffer, hsp->name);
		else
			strcat(Dm__buffer, DEFAULT_SECTION_NAME);

		XtSetArg(Dm__arg[0], XtNtitle, Dm__buffer);
		XtSetValues(hwp->shell, Dm__arg, 1);

		/* Display the text first, so we can get the right
		 * scroll bar size when try to do the scrolling by
		 * setting the XmNtopRow.
		 */
		XtSetArg(Dm__arg[0], XmNstring, hsp->cooked_data);
		XtSetValues(hwp->htext, Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNtopRow, line_num);
		XtSetValues(hwp->htext, Dm__arg, 1);
		SetSwinSize(hwp);
	
		BtnBarWidgets = QueryGizmo(BaseWindowGizmoClass,
			hwp->gizmo_shell, GetItemWidgets, "BtnBarMenu");
		
		if (strcmp(hsp->name, DEFAULT_SECTION_NAME) == 0) {
			XtSetArg(Dm__arg[0], XmNsensitive, False);
			XtSetValues(hwp->menubar, Dm__arg, 1);
		} else {
			/*a flag to indicate whether any button is turned off*/
			Boolean set = False;

			/* Activate Search, Bookmark, Notes, TOC and Glossary
			 * buttons
			 */
			XtSetArg(Dm__arg[0], XmNsensitive, True);

			XtSetValues(BtnBarWidgets[3], Dm__arg, 1);
			XtSetValues(BtnBarWidgets[4], Dm__arg, 1);
			XtSetValues(BtnBarWidgets[5], Dm__arg, 1);
			XtSetValues(BtnBarWidgets[6], Dm__arg, 1);
			XtSetValues(BtnBarWidgets[7], Dm__arg, 1);

			/*
			 * The following checks are not mutually exclusive
			 * because a help file can contain only one section
			 * and there may or may not be a section to backtrack
			 * to regardless of whether this new section is the
			 * first or last section in the file.
			 */
			if (hsp == hwp->hfp->sections) {
				/*is first section, deactivate Previous Topic*/
				XtSetArg(Dm__arg[0], XmNsensitive, False);
				XtSetValues(BtnBarWidgets[2], Dm__arg, 1);
				set = True;
			} else {
				/*
				 * need to do this in case DmChgHelpWinBtnState
				 * is not called with skip_menubar set to True.
				 */
				XtSetArg(Dm__arg[0], XmNsensitive, True);
				XtSetValues(BtnBarWidgets[2], Dm__arg, 1);
			}
			if (((int)(hsp - hwp->hfp->sections) ==
				(int)(hwp->hfp->num_sections - 1)) ||
				hwp->hfp->num_sections == 1)
			{

				/* is last section or there is only one section
				 * - deactivate Next Topic
				 */
				XtSetArg(Dm__arg[0], XmNsensitive, False);
				XtSetValues(BtnBarWidgets[1], Dm__arg, 1);
				set = True;
			} else {
				/*
				 * need to do this in case DmChgHelpWinBtnState
				 * is not called with skip_menubar set to True.
				 */
				XtSetArg(Dm__arg[0], XmNsensitive, True);
				XtSetValues(BtnBarWidgets[1], Dm__arg, 1);
			}
			DmChgHelpWinBtnState(hwp, set, True);
		}
	 	np = DmGetNotesPtr(hwp, hsp->tag ? hsp->tag : hsp->name);
		/* update notes window if it's up */
		if (hwp->notesWin) {
			notes_shell = GetPopupGizmoShell(hwp->notesWin);
			if (notes_shell != NULL) {
				if ((GetWMState(XtDisplay(notes_shell),
				  XtWindow(notes_shell))) == NormalState)
					DmDisplayNotes(hwp, np);
			}
		}
		/*
		 * It is safe to push section onto stack at this point but
		 * only do so if not displaying 1st section.
		 */
		if (hwp->hsp != NULL) {
			DmPushHelpLoc(hwp, file_sv, sectname_sv,
					secttag_sv, save_line);
			FreeHelpInfo(file_sv, sectname_sv, secttag_sv);
		} else {
			/* There's nothing on the stack to backtrack
			 * to so deactivate Backtrack button.
			 */
			XtSetArg(Dm__arg[0], XmNsensitive, False);
			XtSetValues(BtnBarWidgets[0], Dm__arg, 1);
		}
		if (np) {
			/* inform the user this topic has notes */
			DmVaDisplayStatus((DmWinPtr)hwp, False,
				TXT_TOPIC_HAS_NOTES, NULL);
		}
		hwp->hsp = hsp; /* set current segment */
	}
	if (realized) {
		XtMapWidget(hwp->shell);
		if (raise)
			XRaiseWindow(XtDisplay(hwp->shell),
				XtWindow(hwp->shell));
	} else
		PopupHelpWindow(hwp);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (DmModalCascadeActive(hwp->shell)) {
		XtAddGrab(hwp->shell, False, False);
	}
	return;

} /* end of DmShowScrolledSection */

/****************************procedure*header*****************************
 * This function displays the specified string in the help window.
 */
void
DmDisplayHelpString(DmHelpAppPtr hap, char *title, char *string)
{
	Widget vsb;
	Boolean realized = True;
	DmHelpWinPtr hwp = &(hap->hlp_win);

	if (string == NULL)
		return;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = hap->app_id;
		if (CreateHelpWindow(hap))
			return;
	}
	/* update help window title */
	if (!title)
		sprintf(Dm__buffer, "%s %s", hap->title, help_str);
	else
		sprintf(Dm__buffer, "%s %s: %s", hap->title, help_str, title);

	/* save current section, if any, on visited section stack */
	if (hwp->hsp && hwp->hfp)
		SwitchHelpFile(hwp, hwp->hfp->name, hwp->hsp->name,
			hwp->hsp->tag);
	DisplayHelpString(hwp, Dm__buffer, string, realized);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (DmModalCascadeActive(hwp->shell))
		XtAddGrab(hwp->shell, False, False);

} /* end of DmDisplayHelpString */

/*************************************************************************
 * This variable is only used by DmPushHelpStack() and DmPopHelpStack() to
 * prevent a push operation when a pop operation is in progress. When a
 * location is being pop off of the stack, the view needs to be updated by
 * calling DmDisplayHelpSection(). The problem is that this function will
 * push the current location onto the stack.
 */
static int pop_in_progress = 0;

/****************************procedure*header*****************************
 * This routine pushes the current view location onto a stack.
 */
void
DmPushHelpStack(DmHelpWinPtr hwp, char *file, char *sect_name,
	char *sect_tag)
{
	DmPushHelpLoc(hwp, file, sect_name, sect_tag, 0);
}

void
DmPushHelpLoc(DmHelpWinPtr hwp, char *file, char *sect_name,
	char *sect_tag, unsigned int line_num)
{
	DmHelpLocPtr hlp;
	Widget btn;

	if (pop_in_progress)
		return;

	DmClearStatus((DmWinPtr)hwp);
	if ((hwp->stack == NULL) || (hwp->sp == (hwp->stack_size - 1))) {
		/* expand stack allocation */
		hwp->stack_size += STACK_STEP;
		hlp = (DmHelpLocPtr)realloc(hwp->stack,
				 hwp->stack_size * sizeof(DmHelpLocRec));

		if (hlp == NULL) {
			hwp->stack_size -= STACK_STEP;
			return;
		}
		hwp->stack = hlp;
	}
	/* hwp->sp is -1 when the stack is empty */
	hlp = hwp->stack + ++(hwp->sp); /* bump stack ptr */
	hlp->file = strdup(file);

	if (sect_tag)
		hlp->sect_tag  = strdup(sect_tag);
	else
		hlp->sect_tag = NULL;

	if (sect_name)
		hlp->sect_name = strdup(sect_name);
	else
		hlp->sect_name = NULL;

	hlp->filep = hwp->filep;;
	hlp->linecnt = line_num;

	/*
	 * Activate Backtrack button.
	 */
	btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
		GetGizmoWidget, "BtnBarMenu:0");
	XtSetArg(Dm__arg[0], XmNsensitive, True);
	XtSetValues(btn, Dm__arg, 1);
	return;

} /* end of DmPushHelpStack */

/****************************procedure*header*****************************
 * Pops a section off the stack of "visited" sections.
 */
void
DmPopHelpStack(Widget w, DmHelpWinPtr hwp)
{
	DmHelpLocPtr hlp;
	char *file = NULL;
	char *sect_name = NULL;
	char *sect_tag = NULL;

	SaveHelpInfo(hwp, &file, &sect_name, &sect_tag);
	DmClearStatus((DmWinPtr)hwp);
	hlp = hwp->stack + (hwp->sp)--;

	/*
	 * To prevent DmDisplayHelpSection from pushing the same thing
	 * that was just poppped here, set the pop_in_progress flag.
	 */
	pop_in_progress = 1;

	/* check if the section is "TOC" */
	if (hlp->sect_name && !strcmp(hlp->sect_name, TABLE_OF_CONTENTS)) {
		if (strcmp(hlp->file, hwp->hfp->name) == 0 &&
		  hwp->hfp->toc != NULL)
		{
			DmProcessHelpSection(hwp->hfp->toc);

			XtSetArg(Dm__arg[0], XtNstring,
				hwp->hfp->toc->cooked_data);
			XtSetValues(hwp->htext, Dm__arg, 1);
			SetSwinSize(hwp);

			DmPushHelpStack(hwp, file, sect_name, sect_tag);
			hwp->hsp = hwp->hfp->toc;

			/* deactivate the "irrelevant" buttons */
			DmChgHelpWinBtnState(hwp, False, False);
		} else {
			DmDisplayHelpTOC(w, hwp, hlp->file, hwp->app_id);
		}
		/* Activate Backtrack if section stack is not empty */
		if (hwp->stack != NULL && hwp->sp != -1) {
			Widget btn = QueryGizmo(BaseWindowGizmoClass,
			  hwp->gizmo_shell, GetGizmoWidget, "BtnBarMenu:0");
			XtSetArg(Dm__arg[0], XtNsensitive, True);
			XtSetValues(btn, Dm__arg, 1);
		}
	} else {
		DmHelpAppPtr hap = DmGetHelpApp(hwp->app_id);
		if (hlp->filep != NULL)
			hwp->filep = hlp->filep;
		/*
		 * Use hlp->sect_name if set; otherwise, use hlp->sect_tag
		 * since the former is required and the latter isn't.
		 */ 
		DmShowScrolledSection(hap, NULL, hlp->file,
			hlp->sect_tag ? hlp->sect_tag : hlp->sect_name,
			hlp->linecnt, False);
		hwp->filep = hlp->filep;
	}
	pop_in_progress = 0;
	FreeHelpInfo(file, sect_name, sect_tag);
	FreeHelpInfo(hlp->file, hlp->sect_name, hlp->sect_tag);

	/* if stack is now empty, deactivate Backtrack button */
	if (hwp->sp == -1) {
		Widget btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
			GetGizmoWidget, "BtnBarMenu:0");
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn, Dm__arg, 1);
	}

} /* end of DmPopHelpStack */

/****************************procedure*header*****************************
 * Close all application's help windows.
 */
void
DmCloseAllHelpWindows()
{
	DmHelpAppPtr hap;

	for (hap = DESKTOP_HELP_INFO(Desktop); hap; hap = hap->next) {
		if (hap->hlp_win.shell != NULL)
			DmCloseHelpWindow(&(hap->hlp_win));
	}
} /* end of DmCloseAllHelpWindows */

/****************************procedure*header*****************************
 * DmCloseHelpWindow
 */
void
DmCloseHelpWindow(DmHelpWinPtr hwp)
{
	if (hwp == NULL || hwp->shell == NULL)
		return;

	if (hwp->hfp)
		DmCloseHelpFile(hwp->hfp);

	if (hwp->defWin) {
		DmDestroyDefWin(hwp->shell, (XtPointer)hwp, NULL);
	}
	if (hwp->glossWin) {
		DmDestroyGlossWin(hwp->shell, (XtPointer)hwp, NULL);
	}
	if (hwp->notesWin) {
		DmDestroyNotesWin(hwp->shell, (XtPointer)hwp, NULL);
	}
	if (hwp->searchWin) {
		DmDestroySearchWin(hwp->shell, (XtPointer)hwp, NULL);
	}
	if (hwp->bmarkWin) {
		DmDestroyBmarkWin(hwp->shell, (XtPointer)hwp, NULL);
	}
	/* free section stack */
	if (hwp->stack) {
		DmHelpLocPtr hlp;

		for (hlp = hwp->stack; hwp->sp != -1;
			hlp = hwp->stack + (hwp->sp)--)
		{
			FreeHelpInfo(hlp->file, hlp->sect_name, hlp->sect_tag);
		}
		FREE(hwp->stack);
		hwp->stack = NULL;
		hwp->sp = -1;
	}

	if (hwp->search_str)
		FREE(hwp->search_str);
	if (hwp->search_file)
		FREE(hwp->search_file);
	hwp->search_str = hwp->search_file = NULL;
	hwp->found = False;
	
	/* set ptr to current section to NULL */
	if (hwp->hsp)
		hwp->hsp = NULL;

	/* set ptr to current help file to NULL */
	if (hwp->hfp)
		hwp->hfp = NULL;

	/* free the list of help file names */
	if (hwp->help_files) {
		namelistp savep, freep;

		freep = hwp->help_files; 
		while (freep != NULL) {
			savep = freep->next;
			FREE(freep);
			freep = savep;
		}
		hwp->help_files = NULL;
	}

	FreeGizmo(BaseWindowGizmoClass, hwp->gizmo_shell);
	XtUnmapWidget(hwp->shell);
	XtDestroyWidget(hwp->shell);
	hwp->shell = NULL;

} /* end of DmCloseHelpWindow */

/****************************procedure*header*****************************
 * Called when Dismiss/Close is selected from window menu.  Calls
 * DmCloseWHelpWindow() to destroy help window.
 */
static void
WMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpWinPtr hwp = (DmHelpWinPtr)client_data;

	DmCloseHelpWindow(hwp);

} /* end of WMCB */

/****************************procedure*header*****************************
	Realizes the help window and sets its maximum and minimum width
	if the window isn't already mapped; otherwise, maps the help window
	shell.  Also sets viewHeight, viewWidth, etc. for hwp->swin.
 */
static void
PopupHelpWindow(DmHelpWinPtr hwp)
{
	Dimension width;

	XtRealizeWidget(hwp->shell);
	XmAddWMProtocolCallback(hwp->shell, 
		XA_WM_DELETE_WINDOW(XtDisplay(hwp->shell)), WMCB,
		(XtPointer)hwp) ;
	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(hwp->shell, Dm__arg, 1);

#ifdef NOT_USE
	XtAddEventHandler(hwp->shell, StructureNotifyMask, False, EventHandler,
		(XtPointer)hwp);
#endif

	XtSetArg(Dm__arg[0], XtNwidth, &width);
	XtGetValues(hwp->shell, Dm__arg, 1);

	/* Set minWidth of shell so that the help window
	 * cannot be resized smaller horizontally.
	 */
	XtSetArg(Dm__arg[0], XtNminWidth, width);
	XtSetArg(Dm__arg[1], XtNwidth,    width);
	XtSetValues(hwp->shell, Dm__arg, 2);

	/* finally, map the shell */
	XtMapWidget(hwp->shell);

} /* end of PopupHelpWindow */

/****************************procedure*header*****************************
 * Displays the table of contents of the current help file.
 */
void
DmDisplayHelpTOC(Widget w, DmHelpWinPtr hwp, char *file_name, int app_id)
{
	char *sect_name = NULL;
	char *sect_tag = NULL;
	char *file = NULL;
	DmHelpAppPtr hap = DmGetHelpApp(app_id);
	Boolean realized = True;

	if (hwp->shell == NULL) {
		realized = False;
		hwp->app_id = app_id;
		if (CreateHelpWindow(hap))
			return;
	}

	/* Simply raise help window and return if TOC for file_name is
	 * being displayed.
	 */
	if (hwp->hfp && strcmp(hwp->hfp->name, file_name) == 0 &&
	    strcmp(hwp->hsp->name, TABLE_OF_CONTENTS) == 0) {
		XtMapWidget(hwp->shell);
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
		return;
	}

	if (hwp->hsp && hwp->hfp)
		SaveHelpInfo(hwp, &file, &sect_name, &sect_tag);
	DmClearStatus((DmWinPtr)hwp);

	/* check whether file exists and is valid */
	if (file_name) {
		if (hwp->hfp == NULL ||
		   (hwp->hfp && strcmp(hwp->hfp->name, file_name))) {

			DmHelpFilePtr hfp;

			if ((hfp = (DmHelpFilePtr)
			     DmOpenHelpFile(hap, file_name)) == NULL) {

				if (hwp->hfp && hwp->hsp)
					SwitchHelpFile(hwp, file, sect_name,
						sect_tag);
				sprintf(Dm__buffer, "%s %s", hap->title,
					help_str);
				DisplayHelpString(hwp, Dm__buffer,
					(char *)nohelp_str, realized);
				FreeHelpInfo(file, sect_name, sect_tag);
				return;
			}
			/* release all previous resources if new file */
			if (hwp->hfp)
				DmCloseHelpFile(hwp->hfp);
			hwp->hfp = hfp;
		}
	}
	if (file) {
		DmPushHelpStack(hwp, file, sect_name, sect_tag);
		FreeHelpInfo(file, sect_name, sect_tag);
	}
	DmTOCCB(w, (XtPointer)hwp, NULL);
	if (realized) {
		XtMapWidget(hwp->shell);
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
	} else
		PopupHelpWindow(hwp);

	/* If help is on a modal shell, add help window to the modal cascade */
	if (DmModalCascadeActive(hwp->shell))
		XtAddGrab(hwp->shell, False, False);

	return;

} /* end of DmDisplayHelpTOC */

/****************************procedure*header*****************************
 * Displays a string in a help window.
 */
static void
DisplayHelpString(DmHelpWinPtr hwp, char *title, char *string,
	Boolean realized)
{
	Widget btn;

	XtSetArg(Dm__arg[0], XtNstring, string);
	XtSetValues(hwp->htext, Dm__arg, 1);
	SetSwinSize(hwp);

	XtSetArg(Dm__arg[0], XtNtitle, title);
	XtSetValues(hwp->shell, Dm__arg, 1);

	/* deactivate buttons in Bookmark and Notes windows */
	DmChgHelpWinBtnState(hwp, True, False);

	/* Deactivate Next Topic, Previous Topic, Search, Bookmark,
	 * Notes, TOC and Glossary
	 */
	/* Next Topic button */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
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

	/* TOC button */
	btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
		GetGizmoWidget, "BtnBarMenu:6");
	XtSetValues(btn, Dm__arg, 1);

	/* Glossary button */
	btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
		GetGizmoWidget, "BtnBarMenu:7");
	XtSetValues(btn, Dm__arg, 1);

	/* Backtrack button */
	btn = QueryGizmo(BaseWindowGizmoClass, hwp->gizmo_shell,
		GetGizmoWidget, "BtnBarMenu:0");

	if (hwp->stack != NULL && hwp->sp != -1) {
		/* Activate Backtrack button */
		XtSetArg(Dm__arg[0], XmNsensitive, True);
	} else {
		/* Deactivate Backtrack button */
		XtSetArg(Dm__arg[0], XmNsensitive, False);
	}
	XtSetValues(btn, Dm__arg, 1);

	if (realized) {
		XtMapWidget(hwp->shell);
		XRaiseWindow(XtDisplay(hwp->shell), XtWindow(hwp->shell));
	} else
		PopupHelpWindow(hwp);

} /* end of DisplayHelpString */

static void
SetSwinSize(DmHelpWinPtr hwp)
{
	Dimension width, height;

	XtSetArg(Dm__arg[0], XmNwidth, &width);
	XtSetArg(Dm__arg[1], XmNheight, &height);
	XtGetValues(hwp->htext, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XmNwidth, width);
	XtSetArg(Dm__arg[1], XmNheight, height);
	XtSetValues(hwp->swin, Dm__arg, 2);

} /* end of SetSwinSize */
