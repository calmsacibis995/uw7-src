/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_create.c	1.96.9.4"

/******************************file*header********************************

    Description:
	This file contains the source code for creating folder-related
	UI objects.
*/
						/* #includes go here	*/
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>		/* for _XmFindTopMostShell */
#include <Xm/Protocols.h>	/* for XmAddWMProtocolCallback */
#ifdef DEBUG
#include <X11/Xmu/Editres.h>	
#endif

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/ModalGizmo.h>
#include <MGizmo/PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "dm_exit.h"

/**************************forward*declarations***************************
 *
 * Forward Procedure definitions
 */
static void	ToolbarSelectCB(Widget, XtPointer, XtPointer);
static void	CloseFolderCB(Widget, XtPointer, XtPointer);
static void	ExitCB(Widget, XtPointer, XtPointer);
static void	DetermineLayoutAttributes(DmFolderWinPtr window,
					  int view_index,
					  DmViewFormatType *view_type,
					  DtAttrs * loptions,
					  DtAttrs * coptions);
static void	MakeFolderMenu(Boolean);
static void	FolderWinWMCB(Widget, XtPointer, XtPointer);
static void	DmGetFolderAttributes(DmFolderWindow folder, 
				      char **title, 
				      char **icon_name, 
				      char **icon_pixmap, 
				      unsigned char desktop);
static void	DmInitFolderView(DmFolderWinPtr window, int view_index);
static void	DmCheckRoot(DmFolderWindow window);

extern  void 	UpdateFolderCB(Widget w, 
			       XtPointer client_data, 
			       XtPointer call_data);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define	VIEWTYPE		"viewType"

/* container property for minimized folder icon */ 
#define	FOLDERICON		"icon"

#define EXTRA_ICON_SLOTS	5

static WidgetList FolderWidgets;
static int save_offset = 0;
static char *FolderPathList[MAX_VISITED_FOLDERS];
static XmString FolderLabelList[MAX_VISITED_FOLDERS];

#define	VISITED_PATH(index)	(FolderPathList[(index)])
#define TOP_INDEX		(MAX_VISITED_FOLDERS - NUM_VISITED(Desktop))
#define NUM_TOP_ITEMS		3
#define MENU_INDEX(index)	((index) + NUM_TOP_ITEMS)
#define SHUFFLE_FOLDERS(start, end) \
	{ \
		int ind; \
		for (ind = start; ind > end; ind--) { \
			FolderPathList[ind] = FolderPathList[ind - 1]; \
			FolderLabelList[ind] = FolderLabelList[ind - 1]; \
		} \
	}

/* Define a mapping for view names and view types */
static const DmMapping view_mapping[] = {
    "DM_ICONIC",	DM_ICONIC,
    "DM_LONG",		DM_LONG,
    "DM_NAME",		DM_NAME,
    NULL,		-1
};

static const DmMapping sort_mapping[] = {
    "BYNAME",		DM_BY_NAME,
    "BYSIZE",		DM_BY_SIZE,
    "BYTYPE",		DM_BY_TYPE,
    "BYTIME",		DM_BY_TIME,
    NULL,		-1
};

/*	MENU DEFINITIONS

	CAUTION: the order of the buttons for some menus below is also
	reflected in enum types in the source file associated with the
	callback or Dtm.h.
*/

#define XA	XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U	(XtPointer)DM_B_UNDO
#define B_L	(XtPointer)DM_B_LINK

#define TOOLBAR_ITEM(name, type, cb) { \
	(XtArgVal)True, name, NULL, type, \
	NULL, ToolbarSelectCB, (XtPointer)cb, False \
}
static MenuItems toolbarItems[] = {
 TOOLBAR_ITEM("dtm.parnt.24",	I_PIXMAP_BUTTON,	DmFolderOpenParentCB),
 TOOLBAR_ITEM(" ",		I_SEPARATOR_0_LINE,	NULL),
 TOOLBAR_ITEM("dtm.align.24",	I_PIXMAP_BUTTON,	DmViewAlignCB),
 TOOLBAR_ITEM("dtm.sort.24",	I_PIXMAP_BUTTON,	DmViewSortCB),
 TOOLBAR_ITEM(" ",		I_SEPARATOR_0_LINE,	NULL),
 TOOLBAR_ITEM("dtm.copy.24",	I_PIXMAP_BUTTON,	DmFileCopyCB),
 TOOLBAR_ITEM("dtm.move.24",	I_PIXMAP_BUTTON,	DmFileMoveCB),
 TOOLBAR_ITEM("dtm.link.24",	I_PIXMAP_BUTTON,	DmFileLinkCB),
 TOOLBAR_ITEM("dtm.print.24",	I_PIXMAP_BUTTON,	DmFilePrintCB),
 TOOLBAR_ITEM(" ",		I_SEPARATOR_0_LINE,	NULL),
 TOOLBAR_ITEM("dtm.del.24",	I_PIXMAP_BUTTON,	DmFileDeleteCB),
 {NULL}
};

static MenuGizmo toolbarMenu = {
 NULL, "toolbarMenu", "Toolbar", toolbarItems, NULL, NULL,
 XmHORIZONTAL, 1
};

static MenuItems ExitMenuItems[] = {
 { True, TXT_FILE_NEW,	  TXT_M_FILE_NEW,    I_PUSH_BUTTON, NULL, DmFileNewCB,	B_A, False},
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   I_PUSH_BUTTON, NULL, DmFileOpenCB,	B_M, False},
 { True, TXT_FILE_OPEN_NEW,  TXT_M_FILE_OPEN_NEW,   I_PUSH_BUTTON, NULL, DmFileOpenNewCB,	B_M, False },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  I_PUSH_BUTTON, NULL, DmFilePrintCB,B_M, False },
 { True, TXT_FILE_FIND,	  TXT_M_FILE_FIND,   I_PUSH_BUTTON, NULL, DmFindCB,		B_A, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_DESKTOP_EXIT,	TXT_M_Desktop_Exit, I_PUSH_BUTTON, NULL, ExitCB,		B_A, False },
 { NULL }
};

static MenuItems FileMenuItems[] = {
 { True, TXT_FILE_NEW,	  TXT_M_FILE_NEW,    I_PUSH_BUTTON, NULL, DmFileNewCB,		B_A, False },
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   I_PUSH_BUTTON, NULL, DmFileOpenCB,	B_M, False },
 { True, TXT_FILE_OPEN_NEW,  TXT_M_FILE_OPEN_NEW,   I_PUSH_BUTTON, NULL, DmFileOpenNewCB,	B_M, False },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  I_PUSH_BUTTON, NULL, DmFilePrintCB,	B_M, False },
 { True, TXT_FILE_FIND,	  TXT_M_FILE_FIND,   I_PUSH_BUTTON, NULL, DmFindCB,		B_A, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_EXIT,   TXT_M_Exit,   I_PUSH_BUTTON, NULL, CloseFolderCB,	B_A, False },
 { NULL }
};

static MenuItems ConvertMenuItems[] = {
 { True, TXT_CONVERT_D2U, TXT_M_CONVERT_D2U, I_PUSH_BUTTON, NULL, DmFileConvertD2UCB},
 { True, TXT_CONVERT_U2D, TXT_M_CONVERT_U2D, I_PUSH_BUTTON, NULL, DmFileConvertU2DCB},
 { NULL }
};

MENU("convertmenu",	ConvertMenu);

static MenuItems EditMenuItems[] = {
 { False,TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     I_PUSH_BUTTON, NULL, DmEditUndoCB,       B_U},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   I_PUSH_BUTTON, NULL, DmFileCopyCB,	B_M, False },
 { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   I_PUSH_BUTTON, NULL, DmFileMoveCB,	B_M, False },
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   I_PUSH_BUTTON, NULL, DmFileLinkCB,	B_L, False },
 { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, I_PUSH_BUTTON, NULL, DmFileRenameCB,	B_O, False },
{ True, TXT_EDIT_CONVERT,   TXT_M_EDIT_CONVERT,	I_PUSH_BUTTON, (void *)&ConvertMenu, NULL, B_O, False },
 { True, TXT_DELETE, TXT_M_DELETE, I_PUSH_BUTTON, NULL, DmFileDeleteCB,	B_M, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   I_PUSH_BUTTON, NULL, DmEditSelectAllCB,  B_A},
 { True, TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT, I_PUSH_BUTTON, NULL, DmEditUnselectAllCB,B_M},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_PROP,   TXT_M_Properties,   I_PUSH_BUTTON, NULL, DmFilePropCB,	B_M, False },
 { NULL }
};

static MenuItems ViewFormatMenuItems[] = {
 { True, TXT_VIEW_ICON,  TXT_M_VIEW_ICON,  I_PUSH_BUTTON, NULL, DmViewFormatCB,
   (char *)DM_ICONIC},
 { True, TXT_VIEW_SHORT, TXT_M_VIEW_SHORT, I_PUSH_BUTTON, NULL, DmViewFormatCB,
   (char *)DM_NAME},
 { True, TXT_VIEW_LONG,  TXT_M_VIEW_LONG,  I_PUSH_BUTTON, NULL, DmViewFormatCB,
   (char *)DM_LONG},
 { NULL }
};

static MenuItems ViewSortMenuItems[] = {
 { True, TXT_SORT_TYPE, TXT_M_SORT_Type, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_TYPE},
 { True, TXT_SORT_NAME, TXT_M_SORT_NAME, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_NAME},
 { True, TXT_SORT_SIZE, TXT_M_SORT_SIZE, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_SIZE},
 { True, TXT_SORT_AGE, TXT_M_SORT_AGE, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_TIME},
 { NULL }
};

/* Folders Menu: consists of "visited" folder entries and "fixed" entries.
   Initialize the "fixed" buttons here so that the text is i18n'ized only
   once (also, the MENU macro can then be used below).
*/
#define FOLDER_MENU_SIZE MAX_VISITED_FOLDERS + 7

static MenuItems FolderMenuItems[FOLDER_MENU_SIZE] = {
 { True, TXT_HOME_WINDOW,   TXT_M_HOME_WINDOW,   I_PUSH_BUTTON, NULL, DmGotoDesktopCB },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_PARENT_FOLDER, TXT_M_PARENT_FOLDER, I_PUSH_BUTTON, NULL, DmFolderOpenParentCB },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[0], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[1], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[2], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[3], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[4], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[5], False },
 { True, "",   "",   I_PUSH_BUTTON, NULL, DmFolderOpenDirCB, &FolderPathList[6], False },
 { True, TXT_OTHER_FOLDER,  TXT_M_OTHER_FOLDER,  I_PUSH_BUTTON, NULL, DmFolderOpenOtherCB },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FOLDER_MAP,    TXT_M_FOLDER_MAP,    I_PUSH_BUTTON, NULL, DmFolderOpenTreeCB },
 { NULL }
};

static MenuItems HelpMenuItems[] = {
 { True, TXT_HELP_FOLDER,   TXT_M_HELP_FOLDER,   I_PUSH_BUTTON, NULL, DmHelpSpecificCB, B_A, False },
 { True, TXT_HELP_M_AND_K,  TXT_M_HELP_M_AND_K,  I_PUSH_BUTTON, NULL, DmHelpMAndKCB,      B_A, False },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON, NULL, DmHelpTOCCB,      B_A, False },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON, NULL, DmHelpDeskCB,     B_A, False },
 { NULL }
};

/* Special help menu for "Main" window */
static MenuItems DesktopHelpMenuItems[] = {
 { True, TXT_HELP_DESKTOP,  TXT_M_HELP_DESKTOP,  I_PUSH_BUTTON, NULL, DmHelpSpecificCB, B_A, False },
 { True, TXT_HELP_M_AND_K,  TXT_M_HELP_M_AND_K,  I_PUSH_BUTTON, NULL, DmHelpMAndKCB,      B_A, False },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON, NULL, DmHelpTOCCB,      B_A, False },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON, NULL, DmHelpDeskCB,     B_A, False },
 { NULL }
};

MENU("editmenu",	EditMenu);
MENU("exitmenu",	ExitMenu);
MENU("filemenu",	FileMenu);
MENU("foldermenu",	FolderMenu);
MENU("helpmenu",	HelpMenu);
MENU("dthelpmenu",	DesktopHelpMenu);
MENU("viewformatmenu",	ViewFormatMenu);
MENU("viewsortmenu",	ViewSortMenu);

static Gizmo		filemenu;
static Gizmo		exitmenu;
static Gizmo		editmenu;
static Gizmo		viewmenu;
static Gizmo		foldermenu;
static Gizmo		helpmenu;
static Gizmo		desktophelpmenu;
static Gizmo		viewformatmenu;
static Gizmo		viewsortmenu;

/* Menu items for the View folder menu */
static MenuItems ViewMenuItems[] = {
  { True, TXT_VIEW_ALIGN,  TXT_M_VIEW_ALIGN, I_PUSH_BUTTON, NULL, DmViewAlignCB, B_A, False },
  { True, TXT_VIEW_SORT,   TXT_M_VIEW_SORT,  I_PUSH_BUTTON, (void *)&ViewSortMenu, NULL, B_A, False },
  { True, TXT_VIEW_FORMAT, TXT_M_VIEW_FORMAT,I_PUSH_BUTTON, (void *)&ViewFormatMenu, NULL, B_A, False },
  { True, TXT_VIEW_FILTER_LABEL, TXT_M_VIEW_FILTER_LABEL, I_PUSH_BUTTON, NULL, DmViewCustomizedCB, B_A, False },
  { NULL }
};

MENU("viewmenu", ViewMenu);

/* Define the Folder menu bar.  The associated pop up menus are
   assigned in DmOpenFolderWindow, because they are shared menus and only
   need to be created once.
   WARNING: DmCheckRoot depends on the indeces of these menu items.
   Update it if you add/delete/reorganize this menu.
*/
static MenuItems FolderMenuBarItems[] = {
 { True, TXT_FILE,   TXT_M_FILE,   I_PUSH_BUTTON, NULL, NULL },
 { True, TXT_EDIT,   TXT_M_EDIT,   I_PUSH_BUTTON, NULL, NULL },
 { True, TXT_VIEW,   TXT_M_VIEW,   I_PUSH_BUTTON, NULL, NULL },
 { True, TXT_GOTO_FOLDER, TXT_M_GOTO_FOLDER, I_PUSH_BUTTON, NULL, NULL },
 { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL },
 { NULL }
};

MENU_BAR("menubar", FolderMenuBar, DmMenuSelectCB, 0, 0);

    /* PWF - NEED TO FIGURE OUT WHAT TO DO ABOUT SWIN GIZMO */
static ContainerGizmo swin_gizmo = { NULL, "swin", G_CONTAINER_SW };
static GizmoRec folder_gizmos[] = {
  { CommandMenuGizmoClass, 	&toolbarMenu},
  { ContainerGizmoClass,	&swin_gizmo},
};

static MsgGizmo footer = {NULL, "footer", " ", " "};

static BaseWindowGizmo FolderWindow = {
	NULL,			/* help */
	"folder",		/* shell widget name */
	NULL,			/* title (runtime) */
	&FolderMenuBar,		/* menu bar (shared) */
	folder_gizmos,		/* gizmo array */
	XtNumber(folder_gizmos),/* # of gizmos in array */
	&footer,		/* MsgGizmo is status & error area */
	NULL,			/* icon_name (runtime) */
	NULL,			/* name of pixmap file (runtime) */
};

static MenuItems NWFileMenuItems[] = {
 { False, TXT_FILE_NEW,	  TXT_M_FILE_NEW,    I_PUSH_BUTTON, NULL, DmFileNewCB,		B_A, False },
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   I_PUSH_BUTTON, NULL, DmFileOpenCB,	B_M, False },
 { True, TXT_FILE_OPEN_NEW,  TXT_M_FILE_Open_New,   I_PUSH_BUTTON, NULL, DmFileOpenNewCB,	B_M, False },
 { False, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  I_PUSH_BUTTON, NULL, DmFilePrintCB,	B_A, False },
 { True, TXT_FILE_FIND,	  TXT_M_FILE_FIND,   I_PUSH_BUTTON, NULL, DmFindCB,		B_A, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_EXIT,   TXT_M_Exit,   I_PUSH_BUTTON, NULL, CloseFolderCB,	B_A, False },
 { NULL }
};

MENU("nwfilemenu",	NWFileMenu);

static MenuItems NWEditMenuItems[] = {
 { False,TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     I_PUSH_BUTTON, NULL, DmEditUndoCB,       B_U},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { False, TXT_FILE_COPY,   TXT_M_FILE_COPY,   I_PUSH_BUTTON, NULL, DmFileCopyCB,	B_A, False },
 { False, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   I_PUSH_BUTTON, NULL, DmFileMoveCB,	B_A, False },
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   I_PUSH_BUTTON, NULL, DmFileLinkCB,	B_L, False },
 { False, TXT_FILE_RENAME, TXT_M_FILE_RENAME, I_PUSH_BUTTON, NULL, DmFileRenameCB,	B_A, False },
{ False, TXT_EDIT_CONVERT,   TXT_M_EDIT_CONVERT,	I_PUSH_BUTTON, (void *)&ConvertMenu, NULL, B_A, False },
 { False, TXT_DELETE, TXT_M_DELETE, I_PUSH_BUTTON, NULL, DmFileDeleteCB,	B_A, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   I_PUSH_BUTTON, NULL, DmEditSelectAllCB,  B_A, False},
 { True, TXT_EDIT_UNSELECT, TXT_M_EDIT_UNSELECT_ALL, I_PUSH_BUTTON, NULL, DmEditUnselectAllCB,B_M, False},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_PROP,   TXT_M_Properties,   I_PUSH_BUTTON, NULL, DmFilePropCB,	B_M, False },
 { NULL }
};

MENU("nweditmenu",	NWEditMenu);

static MenuItems NWViewSortMenuItems[] = {
 { False, TXT_SORT_TYPE, TXT_M_SORT_Type, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_TYPE},
 { True, TXT_SORT_NAME, TXT_M_SORT_NAME, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_NAME},
 { False, TXT_SORT_SIZE, TXT_M_SORT_SIZE, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_SIZE},
 { False, TXT_SORT_AGE, TXT_M_SORT_AGE, I_PUSH_BUTTON, NULL, DmViewSortCB,
   (char *)DM_BY_TIME},
 { NULL }
};

MENU("nwviewsortmenu",	NWViewSortMenu);

static MenuItems NWViewMenuItems[] = {
  { True, TXT_VIEW_ALIGN,  TXT_M_VIEW_ALIGN, I_PUSH_BUTTON, NULL, DmViewAlignCB, B_A, False },
  { True, TXT_VIEW_SORT,   TXT_M_VIEW_SORT,  I_PUSH_BUTTON, (void *)&NWViewSortMenu, NULL, B_A, False },
  { True, TXT_VIEW_FORMAT, TXT_M_VIEW_FORMAT,I_PUSH_BUTTON, (void *)&ViewFormatMenu, NULL, B_A, False },
  { True, TXT_VIEW_FILTER_LABEL, TXT_M_VIEW_FILTER_LABEL, I_PUSH_BUTTON, NULL, DmViewCustomizedCB, B_A, False },
  { NULL }
};

MENU("nwviewmenu", NWViewMenu);

static Gizmo		NWfilemenu;
static Gizmo		NWeditmenu;
static Gizmo		NWviewsortmenu;
static Gizmo		NWviewmenu;

#undef XA
#undef B_A
#undef B_O
#undef B_M
#undef B_U

/* This is a global array of folder prompt info.  This is handy when
 * operating on ALL prompts in a folder such as when closing the folder or
 * making it busy.
 */
#define OFFSET(prompt)	XtOffsetOf(DmFolderWinRec, prompt)
const FolderPromptInfoRec FolderPromptInfo[] = {
  { FileGizmoClass,	OFFSET(copy_prompt),		GetFileGizmoShell },
  { FileGizmoClass,	OFFSET(move_prompt),		GetFileGizmoShell },
  { FileGizmoClass,	OFFSET(link_prompt),		GetFileGizmoShell },
  { PopupGizmoClass,	OFFSET(rename_prompt),		GetPopupGizmoShell },
  { PopupGizmoClass,	OFFSET(convertu2d_prompt),	GetPopupGizmoShell },
  { PopupGizmoClass,	OFFSET(convertd2u_prompt),	GetPopupGizmoShell },
  { FileGizmoClass,	OFFSET(folder_prompt),		GetFileGizmoShell },
  { PopupGizmoClass,	OFFSET(customWindow),		GetPopupGizmoShell },
  { PopupGizmoClass,	OFFSET(finderWindow),		GetPopupGizmoShell },
  { PopupGizmoClass,	OFFSET(createWindow),		GetPopupGizmoShell },
  { ModalGizmoClass,	OFFSET(overwriteNotice),	GetModalGizmoShell },
  { ModalGizmoClass,	OFFSET(confirmDeleteNotice),	GetModalGizmoShell },
  { ModalGizmoClass,	OFFSET(nmchgNotice),		GetModalGizmoShell },
};
#undef OFFSET
const int NumFolderPromptInfo = XtNumber(FolderPromptInfo);
static unsigned char netware_menus_created = 0;

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    ToolbarSelectCB-
*/
static void
ToolbarSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmSetWinPtr((DmWinPtr)DmFindFolderWindow(w));
    if ((void (*)())client_data == DmViewSortCB) {
	((void (*)())client_data)(w, (XtPointer)DM_BY_TYPE, call_data);
    }
    else {
        ((void (*)())client_data)(w, (XtPointer)DM_B_ONE_OR_MORE, call_data);
    }
}

/****************************procedure*header*****************************
    CloseFolderCB-
*/
static void
CloseFolderCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmCloseFolderWindow((DmFolderWinPtr)DmGetWinPtr(w));

}					/* end of CloseFolderCB */

/****************************procedure*header*****************************
    ExitCB-
*/
static void
ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmPromptExit(False);
}					/* end of ExitCB */

/****************************procedure*header*****************************
 *   DetermineLayoutAttributes- set geometry options according      
 *	to the view type.  Bit flags:
 *		DM_B_SPECIAL_NAME: tells DmCreateIconContainer not
 *				   to create a lable for this item 
 *		DM_B_CALC_SIZE: tells DmCreateIconContainer to calculate
 *			       	the size of this item
 *   
 *   INPUT: folder
 *	    view_index
 *	    geom_options address
 *	    layout_options address  (always set to 0)
 *
 ****************************procedure*header*****************************/
static void
DetermineLayoutAttributes(DmFolderWindow window, int view_index, 
			  DmViewFormatType * view_type,
			  DtAttrs * geom_options, DtAttrs * layout_options)
{	
    /* FLH MORE: check this: the old default was DM_B_CALC_SIZE, but
       we always call DmFormatView *after* we call DmCreateIconContainer
       or DmResetIconContainer.  DmFormatView *always* calculates
       the icon sizes anyway, so there is no need to do it here.
       Leave this, or restructure the FormatView stuff to allow
       optional size calculation
     */
    *geom_options = 0;
    *layout_options = 0;
	
    /* if .dtinfo file exists */
    if ( !(window->views[view_index].cp->attrs & DM_B_NO_INFO) )
    {
	/* Note: for ICONIC view, the VIEWTYPE is not specified
	   in .dtinfo, so the ICONIC case below will never occur.
	 */
	char * view_name = DtGetProperty(&(window->views[view_index].cp->plist), VIEWTYPE, NULL);

	if (view_name)
	{
	    *view_type = DmValueToType(view_name, (DmMapping *)view_mapping);

	    switch(*view_type)
	    {
	    case DM_ICONIC:	
		*geom_options	= 0;
		break;

	    case DM_LONG:
		/* Don't create item labels or calculate item sizes */
		*geom_options	= DM_B_SPECIAL_NAME;
		break;
	    case DM_NAME:
		/* create item labels, but don't calculate sizes */
		*geom_options   = 0;
		break;
	    default:
		break;
	    }
	}
    }
}				/* end of DetermineLayoutAttributes() */

/****************************procedure*header*****************************
    MakeFolderMenu- make the Folders menu from the paths of any "visited"
	folders and the "fixed" buttons.
*/
static void
MakeFolderMenu(Boolean change_all)
{
    int top_index = TOP_INDEX;
    int offset = 0;
    char buf[PATH_MAX];
    int i;
    
#ifdef DEBUG
    time_t start, end;
    fprintf(stderr,"MakeFolderMenu:ENTER\n");
    start = time(NULL);
#endif
    
	if (NUM_VISITED(Desktop) <= 0)
		return;

	if (!SHOW_FULL_PATH(Desktop)) {

	    /*
	     * figure the offset into all the path names to strip out
	     * the common part.
	     */

	    int		fewest = SHRT_MAX;
	    char *	shortest;
	    int		num_components;
	    char *	sub_path;
	    char *	path;
	    int i;
	    
	    /*
	     * Find the item with the fewest "components" in its path.  This
	     * will be used below to strip off any common dir components.
	     * Display at least 2 components in each button label.
	     */
	    for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
		sub_path = path = VISITED_PATH(i);
		
		for (num_components = 0; num_components < fewest;
			num_components++) {

		    if ( (sub_path = strchr(sub_path, '/')) == NULL )
			break;
		    
		    sub_path++;
		}
		
		if (num_components < fewest) {
		    fewest = num_components;
		    shortest = path;
		}
	    }
	    
	    if (fewest > 2) {
	        /*
	         * Look for common directory "components".  In this way, the
	         * button label can display just the unique part of the path.
	         */
		int	len;
		char *	dup_shortest;
		
		/* If there's only 1 entry, try to make it relative to 'home' */
		if (NUM_VISITED(Desktop) == 1)
		    shortest = dup_shortest = strdup(DESKTOP_HOME(Desktop));
		else {
		    shortest = dup_shortest = strdup(shortest);
		    shortest = dirname(shortest);
		}
		shortest = dirname(shortest);
		
		while (!ROOT_DIR(shortest)) {
		    len = strlen(shortest);
		    
		    for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
			path = VISITED_PATH(i);
			
			if ((strncmp(path, shortest, len) != 0) ||
			    (path[len] != '/'))
			    break;
		    }

		    if (++i == top_index) {
			offset = len + 1;
			break;
		    }
		    shortest = dirname(shortest);
		}
		FREE(dup_shortest);
	    }
    	}
	
	if (offset != save_offset) {
		change_all = True;
		save_offset = offset;
	}

	/* Add entries for "visited" folder paths. */
	for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
	    
	    if (FolderLabelList[i]) {
		if (change_all)
	            XmStringFree(FolderLabelList[i]);
		else
		    continue;
	    }
	    
    	    FolderLabelList[i] = XmStringCreateLocalized(VISITED_PATH(i) + offset);
	}
    
	/* set value on all the labels of the visited folder buttons */
	for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
	    Arg args[1];
   	    XtSetArg(args[0], XmNlabelString, FolderLabelList[i]);
	    XtSetValues(FolderWidgets[i], args, 1);
	}

#ifdef DEBUG
    end = time(NULL);
    fprintf(stderr,"MakeFolderMenu:EXIT [%d sec]\n", end-start);
#endif
    
}				/* end of MakeFolderMenu */

/****************************procedure*header*****************************
    FolderWinWMCB-
*/
static void
FolderWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow folder = (DmFolderWindow)client_data;

    /* Don't allow deleting the window while file op in progress */
    if (folder->attrs & DM_B_FILE_OP_BUSY)
	return;

    if (folder == DESKTOP_TOP_FOLDER(Desktop))
	DmPromptExit(False);

    else {
	Widget shell;

	/*
	 * If one of the shared menu is currently associated with this
	 * base window, pop it down too.
	 */
		/* PWF - NEED TO FIGURE OUT HOW MENUS ARE SHARED */
/*
	if (folder == DmGetLastMenuShell(&shell)) {
		OlActivateWidget(shell, OL_CANCEL, NULL);
	}

	DmBringDownIconMenu(folder);
*/

	DmCloseFolderWindow(folder);
    }
}					/* end of FolderWinWMCB */

/*****************************************************************************
 *  	UpdateFolderCB: update view in response to container/object mods
 *			used for wastebasket folder and folders created
 *	INPUTS: widget is NULL (always)
 *		client_data is DmFolderWindow (regist'd by DmOpenFolderWindow)
 *		call_data is DmContainerCallDataPtr (see Dt/DesktopP.h)
 *	OUTPUTS: none
 *	GLOBALS:
 *****************************************************************************/
void
UpdateFolderCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWinPtr	    folder = (DmFolderWinPtr) client_data;
    DmContainerCallDataPtr cp_data = (DmContainerCallDataPtr) call_data;
    DmContainerCBReason	    reason = cp_data->reason;
    int			view_index = 0;
    DmFolderView	      view = &folder->views[view_index];

    _DPRINT3(stderr,"UpdateFolderCB: view = %s, container = %s\n",
	    view->cp->path, cp_data->cp->path);

    
    switch(reason){
    case DM_CONTAINER_CONTENTS_CHANGED:
    {
	Boolean items_touched = False;
	DmItemPtr item;
	DmObjectPtr *obj;
	DmContainerChangedRec *changed = &cp_data->detail.changed;
	int i;
	
	/* FLH MORE: currently only a single object will be added/deleted
	   at a time (there is no container routine to add/remove multiple
	   objects), so it is better to do a FlatSetValues on the individual
	   item than to set itemsTouched to True (which would force the
	   *whole* icon box to be laid out again.  In future we may
	   want to reconsider this if multiple-object addition/removal	
	   routines are provided.
	 */
	/* Deal with Deletions first 
	 * Reset status line if items are touched.
	 * Sort items if touched and folder is not in TREE_WIN
	 */
	if (changed->num_removed > 0 ){
	    /* FLH MORE: use view index here */
	    for (obj = changed->objs_removed, i=0; i<changed->num_removed; 
		 i++, obj++){
		if ((item = DmObjectToItem((DmWinPtr)folder, (*obj))) != NULL){
		    
		    /* Unmanage the item.  Free label *after* notifying
		     * IconBox.  
		     * MORE: encapsulate in DmRmObjectFromView ?
		     */
		    items_touched = True;	/* FLH MORE: remove */
		    XtSetArg(Dm__arg[0], XmNmanaged, False);
		    ExmFlatSetValues(view->box, item - view->itp, Dm__arg, 1);
		    FREE_LABEL(ITEM_LABEL(item));
		    item->label = NULL;
		}
	    }
	}
	if (changed->num_added > 0){
	    /*
	     * Add items that match filter.  DmAddObjectToView
	     * checks for HIDDEN objects.
	     */
	    for (obj = changed->objs_added, i=0; i<changed->num_added; 
		 i++, obj++){

		if (!folder->filter_state || DmObjMatchFilter(folder, *obj))
		    if (DmAddObjectToView(folder, 0, *obj, UNSPECIFIED_POS,
					  UNSPECIFIED_POS)) 
			items_touched = True;
	    }
	}
	if (items_touched){
	    /*
	     * Sort the items if not in ICONIC view because removal
	     * may leave gaps in items.
	     * 
	     * DmSortItems will do DmTouchIconBox
	     * so we only need to touch it ourselves if DmSortItems
	     * is not called.  Is it OK to call DmSortItems
	     * without doing DmTouchIconBox first?
	     *
	     * We don't need to do TouchIconBox if we only removed
	     * items, since we do ExmFlatSetValues above.  
	     * (AddObjToView does not do TouchIconBox)
	     */
	    if ((view->view_type != DM_ICONIC))
		/* && !IS_TREE_WIN(Desktop, folder)) cb not used for fmap*/
		DmSortItems(folder, view->sort_type, 0, 0, 0);
	    else if (changed->num_added > 0)
		DmTouchIconBox((DmWinPtr)folder, NULL, 0);
	    DmDisplayStatus((DmWinPtr)folder);
	}
    }
	break;
    case DM_CONTAINER_DESTROYED:
	/*
	 *  Folder operations in progress.  Let them finish up.
	 *  We'll be called again later by sync timer when they finish.
	 */
	if (!(folder->attrs & DM_B_FILE_OP_BUSY) &&
	    (folder != DESKTOP_TOP_FOLDER(Desktop)))
	    DmCloseFolderWindow(folder);
	break;
    case DM_CONTAINER_MOVED:
    {
	DmContainerMovedRec *moved = &cp_data->detail.moved;
	

	_DPRINT3(stderr, "UpdateFolder: %s moved to %s\n", 
		 moved->old_path, moved->new_path);

	/* The user-traversed path to this container may not match
	 * the real container. We want to display the user_path in the title.
	 * If user_path matches the container path, it contains no
	 * symbolic links and we should update it to match the new
	 * container path.

	 * If the user_path differs from the container path, the
	 * user traversed a link to get here and the link *may* be
	 * broken.  
	 * FLH_MORE: what should we do in this case?  (Close Window,
	 * revert to real-path)
	 */

	if (!strcmp(moved->old_path, DM_WIN_USERPATH(folder))){
	    FREE(DM_WIN_USERPATH(folder));
	    DM_WIN_USERPATH(folder) = strdup(moved->new_path);
	}


	DmUpdateFolderTitle(folder);
    }
	break;
    case DM_CONTAINER_OBJ_CHANGED:
    {
	/* obj_info = DM_B_OBJ_NAME  or DM_B_OBJ_TYPE.  DM_B_OBJ_TYPE
	   is a subset of DM_B_OBJ_NAME
	 */

	DmContainerObjChangedRec *obj_changed = &cp_data->detail.obj_changed;
	DtAttrs obj_info = obj_changed->attrs;
	DmObjectPtr obj = obj_changed->obj;
	DmItemPtr item = DmObjectToItem((DmWinPtr)folder, (obj));
	DmViewFormatType view_type = folder->views[0].view_type;
	int       save_w;
	int       save_h;
	Dimension icon_width;
	Dimension icon_height;
	XtArgVal  new_label;	/* FLH MORE: Hide this */
	XtArgVal  save_label;	/* FLH MORE: Hide this */
	int	  argcnt = 0;

	save_label = (XtArgVal) ITEM_LABEL(item);

	if (obj_info & DM_B_OBJ_NAME){

	    /* object switched name.  we need to resort the
	     *  items if not in iconic view because the rename
	     *  may change the order.
	     */
	    if (view_type == DM_ICONIC)
		MAKE_WRAPPED_LABEL(new_label, FPART(view->box).font,
			DmGetObjectName(ITEM_OBJ(item)));
	    else
	    	MAKE_LABEL(new_label,Dm__MakeItemLabel(item, view_type, 0));
	    item->label = new_label;
	    XtSetArg(Dm__arg[argcnt], XmNlabelString, item->label);argcnt++;
	}

	save_w = ITEM_WIDTH(item);
	save_h = ITEM_HEIGHT(item);
	    
	/* Now recompute its size */
	DmComputeItemSize(folder->views[0].box, item, view_type, 
			  &icon_width, &icon_height);
	item->icon_width = (XtArgVal)icon_width;
	item->icon_height = (XtArgVal)icon_height;
	    
	if (view_type == DM_ICONIC) {
	    WidePosition  new_x;
	    
	    new_x = ITEM_X(item) + (save_w - (int)ITEM_WIDTH(item)) / 2;
	    XtSetArg(Dm__arg[argcnt], XmNwidth, item->icon_width);argcnt++;
	    XtSetArg(Dm__arg[argcnt], XmNheight, item->icon_height);argcnt++;
	    XtSetArg(Dm__arg[argcnt], XmNx, new_x);argcnt++;

	    if (item->icon_width != save_w || item->icon_height != save_h ||
		item->x != new_x || item->label != save_label){
		
		/* restore dimension */
		item->icon_width  = (XtArgVal)save_w;
		item->icon_height = (XtArgVal)save_h;
		item->label       = (XtArgVal)save_label;
		
		ExmFlatSetValues(folder->views[0].box, (Cardinal)
				 (item - folder->views[0].itp),
				 Dm__arg, argcnt);
	    }
	    else{
		/* just refresh the item.  They glyph may have changed */
		ExmFlatRefreshItem(folder->views[0].box, 
				   (Cardinal)(item - folder->views[0].itp),
				   True);
	    }
	}	
	else if (obj_info & DM_B_OBJ_NAME){
	    /* Resort items for long and short views
	     */
	    DmSortItems(folder, folder->views[0].sort_type, 0,0,0);
	}
	    
	/*
	 * Don't free label earlier, because item setvalue needs to
	 * make a comparison of the new and the current.
	 */
	if (obj_info & DM_B_OBJ_NAME)
	    FREE_LABEL(save_label);

	DmDisplayStatus((DmWinPtr)folder);

    }	/* DM_CONTAINER_OBJ_CHANGED */
	break;
    case DM_CONTAINER_SYNC:	/* 1 or more objects reclassed, or
				 * classes modified.
				 * FLH MORE: revisit after PFA
				 */
	DmTouchIconBox((DmWinPtr)folder, NULL, 0);
	break;
    default:
	break;
    }
    
} /* end of UpdateFolderCB */

void
DmWindowSize(DmWinPtr window, char *buf)
{
	Dimension w, h;

	/* PWF - HOW DO WE REACH INTO THE SWIN AND SET RESOURCES */
	XtSetArg(Dm__arg[0], XmNwidth, &w);
	XtSetArg(Dm__arg[1], XmNheight, &h);
	XtGetValues(window->shell, Dm__arg, 2);
	sprintf(buf, "%dx%d", w, h);
}

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmApplyShowFullPath- process "Apply" from Desktop property sheet for
	Show Full Path setting.
	This routine is here so it can access the FolderMenu gizmo.

*/
void
DmApplyShowFullPath(Boolean show)
{
    DmFolderWindow folder;

    if (SHOW_FULL_PATH(Desktop) == show)	/* If no change, do nothing. */
	return;

    SHOW_FULL_PATH(Desktop) = show;

    /* Skip the first folder assuming it's the Destiny window */
    for (folder = DESKTOP_FOLDERS(Desktop)->next;
	 folder != NULL; folder = folder->next)
	DmUpdateFolderTitle(folder);

    MakeFolderMenu(True);
}					/* end of DmApplyShowFullPath */



/****************************procedure*header*****************************
    DmCloseFolderWindow-
*/
void
DmCloseFolderWindow(DmFolderWinPtr folder)
{
    FolderPromptInfoRec const * info;

    /* Can't close folder if it is busy with file op */
    if (folder->attrs & DM_B_FILE_OP_BUSY)
	return;

    /* Free the last task performed (kept around for UNDO) */
    if (folder->task_id != NULL){
	DmFreeTaskInfo(folder->task_id);
	folder->task_id = NULL;	
    }
    
    /*
     *	unregister timeout to restore cursor
     */
    if (folder->busy_id)
	XtRemoveTimeOut(folder->busy_id);
    /*
     * Unregister for notification of updates to the container
     */
    DtRemoveCallback(&folder->views[0].cp->cb_list, UpdateFolderCB , 
		     (XtPointer) folder);

    /* send notifications if any */
    if (folder->notify)
    {
	DmCloseNotifyPtr np;
	DtReply reply;

	reply.header.rptype = DT_OPEN_FOLDER;
	reply.header.status = DT_OK;
	for (np = folder->notify;
	     np < folder->notify + folder->num_notify; np++)
	{
	    reply.header.serial = np->serial;
	    reply.header.version= np->version;
	    DtSendReply(XtScreen(folder->views[0].box), np->replyq,
			np->client, &reply);
	}

	FREE((void *)folder->notify);
    }
    /* Free the root_dir string */
    if (folder->root_dir)
	FREE(folder->root_dir);

    /*
     * free the user-traversed path (not processed by realpath())
     */
    if (folder->user_path)
	FREE(folder->user_path);

    /* If this is the head of the list, point to next folder.  Otherwise,
       find folder in list of folders and remove it.
    */
    if (folder == DESKTOP_FOLDERS(Desktop))
    {
	DESKTOP_FOLDERS(Desktop) = folder->next;

    } else
    {
	DmFolderWinPtr tmp = DESKTOP_FOLDERS(Desktop);

	while (tmp->next != folder)
	    tmp = tmp->next;

	tmp->next = folder->next;
    }


    /* Destroy the Find Window and free associated memory */
    /* PWF - try using the same model as the New window below */
    DmDestroyFindWindow(folder);

    /* Destroy the New Window and free associated memory */
    DmDestroyNewWindow(folder);

    /* Dismiss all of the File property sheets */
    PopdownAllPropSheets(folder);

    /* Free prompt gizmos, popup gizmos, etc. The shells will be
     * destroyed automatically for us when we destroy the folder
     * window shell
     */
    for (info = FolderPromptInfo;
	 info < FolderPromptInfo + NumFolderPromptInfo; info++)
    {
	Gizmo * gizmo = (Gizmo *)(((char *)folder) + info->gizmo_offset);

	if (*gizmo)
	    FreeGizmo(info->class, *gizmo);
    }

    FreeGizmo(BaseWindowGizmoClass, folder->gizmo_shell);

    DmCloseView(folder, True);
    FREE((void *) folder);
}					/* end of CloseFolderWindow */

/****************************procedure*header*****************************
    DmCloseFolderWindows-
*/
void
DmCloseFolderWindows()
{
    DmFolderWindow folder;

    for (folder = DESKTOP_FOLDERS(Desktop);
	 folder != NULL; folder = folder->next)
	DmCloseFolderWindow(folder);

    /* Indicate to sync timer that we're exiting */
    DESKTOP_FOLDERS(Desktop) = NULL;
}

/****************************procedure*header*****************************
 *   DmCloseView
 *		If the window is not an OpenInPlace Window, copy 
 *			view information to container (for .dtinfo file)
 *		Close the container
 *		Destroy the iconbox (optionally)
 *************************************************************************/
void
DmCloseView(DmFolderWindow window, Boolean destroy)
{
    DtAttrs attrs;

    if (window->saveFormat){
	/* FLH MORE: should we be setting/removing some attributes here, so 
	   we don't save the container to disk? Probably not, because
	   other views may have the container open and may want to
	   have object coordinates saved.
	 */
	attrs = 0;	/* write out .dtinfo file */
	if (window->views[0].view_type == DM_ICONIC) {
	    /* save X and Y */
	    DmSaveXandY(window->views[0].itp, window->views[0].nitems);
	    DtSetProperty(&(window->views[0].cp->plist), VIEWTYPE, NULL, 0);
	}
	else
	    /* save current view information (possibly sort info. later ) */
	    DtSetProperty(&(window->views[0].cp->plist), VIEWTYPE,
			  DmTypeToValue(window->views[0].view_type, 
					(DmMapping *)view_mapping), NULL);

	/* save window dimensions */
	DmWindowSize((DmWinPtr) window, Dm__buffer);
	DtSetProperty(&(window->views[0].cp->plist), FOLDERSIZE, Dm__buffer, 
		      NULL);
    }
    else{
	attrs = DM_B_NO_FLUSH;	/* don't write out .dtinfo file */
    }

    _DPRINT1(stderr, "DmCloseView: %s: %s write .dtinfo file\n",
	     DM_WIN_PATH(window), window->saveFormat ? "" : "DON'T");

    /* Free the ContainerRec, free icon labels and destroy widget tree */
    DmCloseContainer(window->views[0].cp, attrs);
    if (destroy)
	DmDestroyIconContainer(window->shell, window->views[0].box,
			       window->views[0].itp, window->views[0].nitems);

}					/* end of DmCloseView */

/****************************procedure*header*****************************
    DmFindFolderWindow-
*/
DmFolderWinPtr
DmFindFolderWindow(Widget w)
{
    Widget		shell = DtGetShellOfWidget(w);
    DmFolderWinPtr	window;

    if (shell == NULL)
	window = NULL;

    else
	for (window = DESKTOP_FOLDERS(Desktop);
	     (window != NULL) && (window->shell != shell);
	     window = window->next)
	    ;

    return (window);
}				/* end of DmFindFolderWindow */



/****************************procedure*header*****************************
    MultiClickOff-
*/
#define QUERYGIZMO(but)	(Widget)QueryGizmo(BaseWindowGizmoClass, base, \
					GetGizmoWidget, "toolbarMenu:"but)
static void
MultiClickOff(Gizmo base, unsigned char first)
{
    Widget	w;

    w = QUERYGIZMO("0");
    if (first) {
	XtUnmapWidget(w);	/* Don't show parent in desktop folder */
	XtSetSensitive(w, False);
    }
    else {
	XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    }
    w = QUERYGIZMO("2");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("3");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("5");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("6");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("7");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("8");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
    w = QUERYGIZMO("10");
    XtVaSetValues(w, XmNmultiClick, XmMULTICLICK_DISCARD, NULL);
}
#undef QUERYGIZMO

static char *	ToolbarHelp[] = {
	TXT_PARENT_HELP,
	"",
	TXT_ALIGN_HELP,
	TXT_SORT_HELP,
	"",
	TXT_COPY_HELP,
	TXT_MOVE_HELP,
	TXT_LINK_HELP,
	TXT_PRINT_HELP,
	"",
	TXT_DELETE_HELP
};

static void
ToolbarEventCB(Widget w, XtPointer client_data, XEvent *xevent, Boolean *c)
{
	int		i = (int)client_data;
	DmFolderWinPtr	window;

	window = DmFindFolderWindow(w);
	if(xevent->type == EnterNotify) {
		SetBaseWindowLeftMsg(window->gizmo_shell, GGT(ToolbarHelp[i]));
	}
	else if(xevent->type == LeaveNotify) {
		DmDisplayStatus((DmWinPtr)window);
	}
}

/****************************procedure*header*****************************
    DmOpenFolderWindow-
*/

/* Insensitive pixmaps for the toolbar: */
static Pixmap	*ip = NULL;

DmFolderWinPtr
DmOpenFolderWindow(char * path, DtAttrs attrs, char * geom_str, Boolean iconic)
{
    DmGlyphRec		tp;
    DmGlyphRec *	gp;
    char		buf[256];
    static unsigned char first = 1;
    DmFolderWinPtr	window;
    Dimension		view_width;
    Gizmo 		base; /* PWF - ARE THESE THE CORRECT STRUCTURES?? */
    Gizmo 		menu;
    Widget		menu_w;
    Widget		form;
    Widget		icon_shell;
    int			n;
    Arg			args[10];
    Boolean		rooted = ((attrs & DM_B_ROOTED_FOLDER) != 0);
    Pixmap		stipple;
    Pixmap		pm;
    Pixel		p;
    Display *		dpy;
    Screen *		scrn;
    static GC		gc = NULL;
    XGCValues		v;
	char		*real_path;


    /* If $HOME is '/', then DESKTOPDIR will be "//desktop" which will
       confuse things like the Folder menu.  Fix this here.
    */
    if ((path[0] == '/') && (path[1] == '/'))
	path++;

    if (access(path, F_OK) != 0)
	return (NULL);
    
    if (!(attrs & DM_B_OPEN_NEW)){
	/* DT_OPEN_FOLDER requests always bring up rooted folders.
	   If this is not a request for a rooted folder, make sure user doesn't
	   get stuck in an existing rooted folder.
	   */
	window = DmQueryFolderWindow(path, rooted ?  DM_B_NOT_DESKTOP : 
				     DM_B_NOT_DESKTOP | DM_B_NOT_ROOTED);
	if (window)
	{
	    DmMapWindow((DmWinPtr)window);
	    
	    /* move it to top of menu */
	    Dm__UpdateVisitedFolders(NULL, DM_WIN_PATH(window));
	    return(window);
	}
    }


    window = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));

    real_path = realpath(path, Dm__buffer);
    if (real_path == NULL)
	return(NULL);

    if (IS_NETWARE_PATH(real_path)) {
	/* Since we know that everything in /.NetWare are servers, we can
	 * class them ourselves rather than doing stat on each of them, which
	 * can potentially tie up dtm for a significantly long time.
 	 */
    	window->views[0].cp = DmOpenDir(path, attrs | DM_B_TIME_STAMP);
		if (window->views[0].cp)
			DmClassNetWareServers(window->views[0].cp);
    } else {
    	window->views[0].cp = DmOpenDir(path, attrs | DM_B_TIME_STAMP
		| DM_B_SET_TYPE | DM_B_INIT_FILEINFO);
    }

    if (window->views[0].cp == NULL)
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, path);
	FREE((void *)window);
	return (NULL);
    }

    /* Store root dir.  Use realpath() version, not user_path.
     * The user_path may contain symbolic links, which will confuse
     * DmCheckRoot below.
     */
    if (rooted)
	window->root_dir = strdup(DM_WIN_PATH(window));

    /*
     * store user-traversed path (not processed by realpath())
     */
    window->user_path = strdup(path);

    /*
     * Register for notification of updates to this container
     */
    DtAddCallback(&window->views[0].cp->cb_list, UpdateFolderCB , 
		  (XtPointer) window);

    /* Since there is only one baseWindowGizmo structure (FolderWindow)
       set the values for title, iconName, iconPixmap directly in the 
       structure. These values are NOT strdup values
     */
    DmGetFolderAttributes(window, &FolderWindow.title, &FolderWindow.iconName,
			  &FolderWindow.iconPixmap, first);

    window->saveFormat = True;	/* save size/format in .dtinfo on close*/
    window->views[0].view_type	= DM_ICONIC;
    window->views[0].sort_type	= DM_BY_TYPE;

    /* insert new window into the list */
    DmAddWindow((DmWinPtr *)&DESKTOP_FOLDERS(Desktop), (DmWinPtr)window);

    if (first)
    {
	/* initialize dnd targets */
    	DESKTOP_DND_TARGETS(Desktop)[0] =
		OL_USL_NUM_ITEMS(DESKTOP_DISPLAY(Desktop));
    	DESKTOP_DND_TARGETS(Desktop)[1] =
		OL_USL_ITEM(DESKTOP_DISPLAY(Desktop));
    	DESKTOP_DND_TARGETS(Desktop)[2] =
		OL_XA_FILE_NAME(DESKTOP_DISPLAY(Desktop));
    	DESKTOP_DND_TARGETS(Desktop)[3] = XA_STRING;
    	DESKTOP_DND_TARGETS(Desktop)[4] =
		OL_XA_HOST_NAME(DESKTOP_DISPLAY(Desktop));
    	DESKTOP_DND_TARGETS(Desktop)[5] =
		OL_XA_COMPOUND_TEXT(DESKTOP_DISPLAY(Desktop));

	/* Activate "sync" timer when 1st folder is created */
	if (SYNC_INTERVAL(Desktop) > 0)
	    Dm__AddSyncTimer(Desktop);
	/* The Desktop Window is always the first window opened */
	DESKTOP_TOP_FOLDER(Desktop) = window;	
	window->attrs |= DM_B_TOP_FOLDER;

    } else
    {
	/* Add this folder path to the list of visited folders. */
	Dm__UpdateVisitedFolders(NULL, DM_WIN_PATH(window));
    }

    if (!geom_str && (geom_str = DtGetProperty(&(window->views[0].cp->plist),
			 FOLDERSIZE, NULL))) {
	/* make sure it is less than the screen size */
	int sw = WidthOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
	int sh = HeightOfScreen(XtScreen(DESKTOP_SHELL(Desktop)));
	int w = 0, h = 0;

	if ((sscanf(geom_str, "%dx%d", &w, &h) != 2) || !w || !h ||
	    (w > sw) || (h > sh))
		geom_str = NULL;
    }


    /* set up resources for the scrolled window:
       
       1) attachments: to make it fill up its parent Form
       2) application-defined scrolling (grid units are determined by view type.
          see DmInitFolderView.)
     */
    n=0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    /* FLH MORE: should we make leftOffset, shadowThickness resolution independent ? */
    XtSetArg(args[n], XmNleftOffset, 4); n++;	
    XtSetArg(args[n], XmNrightOffset, 4); n++;	
    XtSetArg(args[n], XmNshadowThickness, 2); n++;	/* FLH REMOVE */
    XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
    XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++;
    XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
    folder_gizmos[0].args = args;
    folder_gizmos[0].numArgs = n-4;
    folder_gizmos[1].args = args;
    folder_gizmos[1].numArgs = n;

    /* Set the client data for the menubar items 
       FLH MORE: we are writing directly into the template.
       As long as we do this every time, we should be ok.  Right?
       
       PWF - ARE THESE THE CORRECT RESOURCES TO PASS ???
     */
    FolderMenuBar.clientData = (XtPointer)window;

    XtSetArg(Dm__arg[0], XmNiconic, iconic );
    XtSetArg(Dm__arg[1], XmNgeometry, geom_str);

    _DPRINT1(stderr,"DmOpenFolderWindow: geom_str = %s\n", 
	    geom_str ? geom_str: "NULL");


    window->gizmo_shell = base = CreateGizmo(DESKTOP_SHELL(Desktop), 
			BaseWindowGizmoClass, &FolderWindow, Dm__arg, 2);
    window->shell =   GetBaseWindowShell(base);
    icon_shell    = GetBaseWindowIconShell(base);
#ifdef MINIMIZED_DND
    XtAddEventHandler(icon_shell, (EventMask)StructureNotifyMask, False,
        DmDnDRegIconShell, (XtPointer)window);
#endif /* MINIMIZED_DND */
    
    /* Set the insensitive pixmaps in the toolbar */
    if (ip == NULL) {
	ip = (Pixmap *)CALLOC(15, sizeof(Pixmap));
	stipple = XmGetPixmapByDepth(
	    XtScreen(window->shell), "50_foreground", 1, 0, 1
	);
    }
    dpy = XtDisplay(window->shell);
    scrn = XtScreen(window->shell);
    for (n=0; toolbarItems[n].label!=NULL; n++) {
	if (toolbarItems[n].label[0] != ' ') {
	    sprintf(buf, "toolbarMenu:%d", n);
	    menu_w  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
					 GetGizmoWidget, buf);
    	    XtAddEventHandler(menu_w,
			      (EventMask)EnterWindowMask|LeaveWindowMask,
			      False, ToolbarEventCB, (XtPointer)n);
	    if ((gp = DmGetPixmap(scrn, toolbarItems[n].label)) != NULL) {
		if (gc == NULL) {
		    gc = XCreateGC(dpy, gp->pix, 0, &v);
		}
		if (ip[n] == NULL) {
		    /* Create the pixmap that will contain the icon */
	            ip[n] = XCreatePixmap(
		        dpy, RootWindowOfScreen(scrn),
		        (unsigned int)gp->width, (unsigned int)gp->height,
		        gp->depth
	            );
	            pm = XCreatePixmap(
		        dpy, RootWindowOfScreen(scrn),
		        (unsigned int)gp->width, (unsigned int)gp->height,
		        gp->depth
	            );
		    DmMaskPixmap(menu_w, gp);
		    /* Draw the insensitive icon in the pixmap */
		    ExmFIconDrawIcon(
			menu_w, gp, ip[n], gc, False, stipple, 0, 0
		    );
		    tp = *gp;
		    tp.pix = ip[n];
		    DmMaskPixmap(menu_w, &tp);
		}
	        XtVaSetValues(menu_w, XmNlabelInsensitivePixmap, ip[n], NULL);
	        XtVaSetValues(menu_w, XmNlabelPixmap, gp->pix, NULL);
	    }
	}
    }


#ifdef DEBUG
    XtAddEventHandler(window->shell, (EventMask) 0, True, 
		      _XEditResCheckMessages, NULL);
#endif

    menu_w  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
				 GetGizmoWidget, "toolbarMenu");
    window->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
				 GetGizmoWidget, "swin");
    XtSetArg(Dm__arg[0], XmNtopAttachment, XmATTACH_WIDGET);
    XtSetArg(Dm__arg[1], XmNtopWidget, menu_w);
    XtSetValues(window->swin, Dm__arg, 2);

    if (first)
    {
		/* Build sub menus that are shared by all folder windows. */

		filemenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&FileMenu, NULL, 0);
		exitmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&ExitMenu, NULL, 0);
		viewmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&ViewMenu, NULL, 0);
		editmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&EditMenu, NULL, 0);

		foldermenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass,
							&FolderMenu,NULL,0);
		FolderWidgets = QueryGizmo(PulldownMenuGizmoClass, foldermenu,
                                GetItemWidgets, (char *)NULL);
        /* skip the fixed widgets on top */
        FolderWidgets += NUM_TOP_ITEMS;
        XtUnmanageChildren(FolderWidgets, MAX_VISITED_FOLDERS);

		helpmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&HelpMenu, NULL, 0);
		desktophelpmenu = CreateGizmo(DESKTOP_SHELL(Desktop),
			PulldownMenuGizmoClass, &DesktopHelpMenu, NULL, 0);
    } else if (window->attrs & DM_B_NETWARE_WIN && netware_menus_created == 0)
	{
		NWfilemenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWFileMenu, NULL, 0);
		NWeditmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWEditMenu, NULL, 0);
		NWviewmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWViewMenu, NULL, 0);
		netware_menus_created = 1;
	}
    menu = GetBaseWindowMenuBar(base);
    /* Get rid of shadow on menu */
    menu_w = QueryGizmo(MenuBarGizmoClass, menu, GetGizmoWidget, NULL);
    XtSetArg(Dm__arg[0], XmNshadowThickness, 2);
    XtSetValues(menu_w, Dm__arg, 1);

	/* Set the shared menu gizmos pointers into the new gizmo struct.
	 * Separate File, Edit and View menus are used for the NetWare folder
	 * because file operations are not allowed on NetWare servers.
	 */

    if (first) /* First window gets the Exit button */
		SetSubMenuValue (menu, exitmenu, 0);
    else if (window->attrs & DM_B_NETWARE_WIN)
		SetSubMenuValue (menu, NWfilemenu, 0);
	else
		SetSubMenuValue (menu, filemenu, 0);

    if (window->attrs & DM_B_NETWARE_WIN) {
		SetSubMenuValue (menu, NWeditmenu, 1);
		SetSubMenuValue (menu, NWviewmenu, 2);
	} else {
		SetSubMenuValue (menu, editmenu, 1);
		SetSubMenuValue (menu, viewmenu, 2);
	}
	SetSubMenuValue (menu, foldermenu, 3);

    if (first) 
		SetSubMenuValue (menu, desktophelpmenu, 4);
    else 
		SetSubMenuValue (menu, helpmenu, 4);

    DmInitFolderView(window, 0);

    /*  We will handle Close requests ourself.  Both the
     *  folder and the icon menu must be unmapped.  Also,
     *  if a folder operation is still pending, we will want to
     *  postpone destruction of the folder until the file operation
     *  is complete.
     */
    XmAddWMProtocolCallback( window->shell, 
			    XA_WM_DELETE_WINDOW(XtDisplay(window->shell)), 
			    FolderWinWMCB, (XtPointer) window ) ;
    XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
    XtSetValues(window->shell, Dm__arg, 1);


    /* put something in the status area */
    DmDisplayStatus((DmWinPtr)window);

    if (window->root_dir)
	DmCheckRoot(window);

    XtRealizeWidget(window->shell);

    /* Turn off multiClick for all buttons in the toolbar */
    MultiClickOff(base, first);
    if (first)
    {
	DmSetToolbarSensitivity((DmWinPtr)window, 0);
        Session.SessionLeader = XtWindow(GetBaseWindowShell(base));
        Session.SessionLeaderIcon = XtWindow(GetBaseWindowIconShell(base));
    }

    /* FORMAT VIEW after window has been realized so geometries have been
       negotiated.  Pass the view_type in 'type' and override the window
       view_type with 'no value'.  SPECIAL: if ICONIC and no ".dtinfo",
       don't pass any view_type; DmFormatView understands.
    */
    {
	DmViewFormatType type =
	    ((window->views[0].view_type == DM_ICONIC) &&
	     (window->views[0].cp->attrs & DM_B_NO_INFO)) ?
		 (DmViewFormatType)-1 : window->views[0].view_type;
	window->views[0].view_type = (DmViewFormatType)-1;
	DmFormatView(window, type);
    }

    /* Register for context-sensitive help for all folders.  Since
     * XmNhelpCallback is a MOTIF resource, register help on the child
     * of the toplevel shell instead of the shell itself.
     */
    form = QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
	GetGizmoWidget, NULL);
    XtSetArg(Dm__arg[0], XmNuserData, window);
    XtSetValues(form, Dm__arg, 1);

    if (first)
    {
	first = 0;			/* no longer the first time */

	/*
	 * Register Destiny window for help separately from other
	 * folders because it has a different app_title. 
	 */
	DESKTOP_HELP_ID(Desktop) =
	    DmNewHelpAppID(DESKTOP_SCREEN(Desktop),
			   XtWindow(window->shell),
			   (char *)dtmgr_title,
			   (char *)product_title,
			   DESKTOP_NODE_NAME(Desktop),
			   NULL, "dtprop.icon")->app_id;

	menu_w = QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
		GetGizmoWidget, "dthelpmenu:0");
    } else
	menu_w = QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
		GetGizmoWidget, "helpmenu:0");
    XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)DmBaseWinHelpKeyCB,
        (XtPointer)menu_w);
    
    /* Set focus to FlatIconBox */
    XmProcessTraversal(window->views[0].box, XmTRAVERSE_CURRENT);    

    return(window);
}				/* end of DmOpenFolderWindow */

/****************************procedure*header*****************************
 * DmOpenInPlace
 *
 *	INPUT:
 *	OUTPUT: 
 *	FLH MORE: what params do we need ?
 *************************************************************************/
DmFolderWinPtr
DmOpenInPlace(DmFolderWinPtr folder, char *path)
{
    DmContainerPtr	cp;
    char 		*title;
    char 		*icon_name;
    char 		*icon_pixmap;
    char 		*real_path;
    DtAttrs		geom_options;
    DtAttrs		dummy_layout_options;		/* not used */
    DmViewFormatType	ignore_type;			/* not used */
    FolderPromptInfoRec const * info;
	Gizmo		menu;
    
    _DPRINT1(stderr, "DmOpenInPlace: %s OPEN_IN_PLACE is %s\n", path,
	    OPEN_IN_PLACE(Desktop) ? "ON" : "OFF");

    /* 
     * Open a new folder if OpenInPlace is turned off, or if the
     * open is being initiated from a "special" window.  Force
     * OpenInPlace if we are in a rooted folder.
     *
     * FLH MORE: do we need all of these checks?
     */
     if (((OPEN_IN_PLACE(Desktop) == False) && (folder->root_dir == NULL))
	 || IS_TREE_WIN(Desktop, folder) || IS_FOUND_WIN(Desktop, folder) 
	 || IS_WB_WIN(Desktop, folder) 
	 || (folder == DESKTOP_TOP_FOLDER(Desktop))){

	 _DPRINT1(stderr,"Can't do OpenInPlace from this window\n");
	 return (DmOpenFolderWindow(path, 0, NULL, False));

     }

    /* Can't close folder if it is busy with file op */
    if (folder->attrs & DM_B_FILE_OP_BUSY)
	return (folder);

    /*
     * We're ready to do an Open-In-Place.  Let's make sure we can
     * read the new directory.  If we can't bail out before we
     * destroy the current directory info.
     */

    /* If $HOME is '/', then DESKTOPDIR will be "//desktop" which will
       confuse things like the Folder menu.  Fix this here.
       FLH MORE: is this necessary? Try to remove it.
    */
    if ((path[0] == '/') && (path[1] == '/'))
	path++;

    real_path = realpath(path, Dm__buffer);
    if (real_path == NULL)
	return(NULL);

    if (IS_NETWARE_PATH(real_path)) {
	/* Since we know that everything in /.NetWare are servers, we can
	 * class them ourselves rather than doing stat on each of them, which
	 * can potentially tie up dtm for a significantly long time.
 	 */
    	if (cp = DmOpenDir(path, DM_B_TIME_STAMP)) {
			DmClassNetWareServers(cp);
		}
    } else {
    	/* FLH MORE: do we really need to read the .dtinfo file here? */
    	cp = DmOpenDir(path, DM_B_TIME_STAMP | DM_B_SET_TYPE |
			DM_B_INIT_FILEINFO);
    }
    if (cp == NULL)
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, path);
	return (NULL);
    }

    /* 
     * We're ready to go for it. Start removing the information
     * on the current directory.
     */

#ifdef NOT_USED
    /*
     *	Set flag to indicate that we don't want to save .dtinfo
     *	FLH MORE: can we put this in the folder->attrs field ?
     */
    remove this as per uu94-11953
     folder->saveFormat = False;
#endif


    if (folder->task_id != NULL){
	/* FLH MORE: DmMenuSelectCB should automatically desensitize 
	 * the Undo button when it notices folder->task_id is NULL
	 */
	DmFreeTaskInfo(folder->task_id);
	folder->task_id = NULL;
    }
    /* Unregister for notification of updates to the container */
    DtRemoveCallback(&folder->views[0].cp->cb_list, UpdateFolderCB , 
		     (XtPointer) folder);

    /* 
     * FLH MORE: free the finder information as in DmCloseFolderWindow
     */
    _DPRINT1(stderr,"DmOpenInPlace: Need to Free Finder\n");

    /* Dismiss all File Property Sheets */
    PopdownAllPropSheets(folder);


    /* Get rid of the New Window as in DmCloseFolderWindow */
    DmDestroyNewWindow(folder);

    /* Free prompt gizmos, popup gizmos, etc., and destroy
	 * any associated shells.
	 */
    for (info = FolderPromptInfo;
	 info < FolderPromptInfo + NumFolderPromptInfo; info++)
    {
	Gizmo * gizmo = (Gizmo *)(((char *)folder) + info->gizmo_offset);

	if (*gizmo)
	{
	    Widget shell = (*info->get_gizmo_shell)(*gizmo);
	    if (shell)
		XtDestroyWidget(shell);
	    FreeGizmo(info->class, *gizmo);
	    *gizmo = NULL;
	}
    }

    /* close the view and save layout info */
    DmCloseView(folder, False);

    /*
     * Now construct the information for the new container
     *
     *	Things to leave untouched: view_type, size, root_dir,
     *	filter, sort_type, notification list.
     *
     * FLH MORE: what attrs should we pass?  If we add attrs
     * to our own arglist, perhaps they should be passed here too.
     */
    folder->views[0].cp = cp;

    /*
     * free the old user-traversed path (not processed by realpath())
     * and store the new one
     */
    if (folder->user_path)
	FREE(folder->user_path);
    folder->user_path = strdup(path);

    /*
     * Register for notification of updates to this container
     */
    DtAddCallback(&folder->views[0].cp->cb_list, UpdateFolderCB, 
		  (XtPointer) folder);

    Dm__UpdateVisitedFolders(NULL, DM_WIN_PATH(folder));

    /* FLH MORE: do we need to notify the scrolled window or
     * FIconBox about a change in virtual window size?  It
     * needs to update the scrollbar sizes
     */
    /*
     *	FLH MORE: set the title, icon name, and folder label
     */
    DmGetFolderAttributes(folder, &title, &icon_name, &icon_pixmap, 0);
    SetBaseWindowTitle(folder->gizmo_shell, title);
    SetBaseWindowGizmoPixmap(folder->gizmo_shell, icon_pixmap);
    SetBaseWindowIconName(folder->gizmo_shell, icon_name);

    /* geom_options: let DmFormatView do size calculations 
     *  		(i.e., don't set DM_B_CALC_SIZE).
     *		let DmFormatView calculate the label in DM_LONG view
     */
    geom_options = (folder->views[0].view_type == DM_LONG) ? DM_B_SPECIAL_NAME
	: (folder->views[0].view_type == DM_ICONIC) ? DM_B_WRAPPED_LABEL : 0;

    DmResetIconContainer(folder->views[0].box, 
			 geom_options, 
			 folder->views[0].cp->op,
			 folder->views[0].cp->num_objs,
			 &(folder->views[0].itp),
			 &(folder->views[0].nitems),
			 EXTRA_ICON_SLOTS);

    /* 
       Pass the view_type in 'type' and override the window
       view_type with 'no value'.  SPECIAL: if ICONIC and no ".dtinfo",
       don't pass any view_type; DmFormatView understands.
    */
    {
	DmViewFormatType type =
	    ((folder->views[0].view_type == DM_ICONIC) &&
	     (folder->views[0].cp->attrs & DM_B_NO_INFO)) ?
		 (DmViewFormatType)-1 : folder->views[0].view_type;
	folder->views[0].view_type = (DmViewFormatType)-1;
	DmFormatView(folder, type);
    }

    if (folder->root_dir)
	DmCheckRoot(folder);

    DmDisplayStatus((DmWinPtr)folder);

    /* Set focus to FlatIconBox */
    XmProcessTraversal(folder->views[0].box, XmTRAVERSE_CURRENT);    


    /* FLH MORE: ResetIconContainer should not do an itemsTouched
       if we are not in iconic view
       */
    /* Handle long and short views */
    /* Layout the items if they were not laid out before */
    /* Now Filter the view */
    /* Set button sensitivities appropriately */


	/* Set the shared menu gizmos pointers into the new gizmo struct.
	 * Separate File, Edit and View menus are used for the NetWare folder
	 * because file operations are not allowed on NetWare servers.
	 */
    menu = GetBaseWindowMenuBar(folder->gizmo_shell);
    if (folder->attrs & DM_B_NETWARE_WIN) {
    	if (netware_menus_created == 0)
		{
			NWfilemenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWFileMenu, NULL, 0);
			NWeditmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWEditMenu, NULL, 0);
			NWviewmenu = CreateGizmo(DESKTOP_SHELL(Desktop),PulldownMenuGizmoClass, 
							&NWViewMenu, NULL, 0);
			netware_menus_created = 1;
		}
		SetSubMenuValue (menu, NWfilemenu, 0);
		SetSubMenuValue (menu, NWeditmenu, 1);
		SetSubMenuValue (menu, NWviewmenu, 2);
	} else {
		SetSubMenuValue (menu, filemenu, 0);
		SetSubMenuValue (menu, editmenu, 1);
		SetSubMenuValue (menu, viewmenu, 2);
	}

    /* No need to register for context-sensitive help here because we are
     * reusing the same folder window.
     */

    return(folder);
#ifdef NOT_USED
    /*
     * 1) check if a file operation is busy.  If so, ignore open
     *
     * 2) "close" the old folder  (see DmCloseFolderWindow)
     *
     *		unregister container callback
     *		destroy all of the gizmo popups (or reset directory)
     *		destroy all file prop sheets
     *		send notification  (not needed because we can't be last ref)
     *		free finder window stuff (move into subroutine from DmClose...)
     *		free new window list (CloseView)
     *		free gizmos
     *		close the container (can this be done earlier?)
     *		what about edit->undo stuff ??	DmFreeTaskInfo 
     *		(check DmCloseFolderWindow on this stuff)
     *		clear all old items (??)
     *		remove all references to the old container (??)

     *
     *	3) reset the "view" 
     *		what is the user_data field used for?
     *		open a new container DmOpenDir (what attrs to pass)
     *		register for container callbacks
     *		update the visited folders list
     *		set saveFormat to False (this must be set True in OpenFolder)
     *		build item list from cp (like DmCreateIconContainer how
     *			do we make sure the X,Y positions are set correctly??)
     *		leave view type from old folder
     *		reset title, iconName, and iconPixmap (BaseWindowGizmo)
     *		re-register for help with the new folder title
     *		Format the view according to the current view (??)
     *			(should we just call DmSortItems??)
     *		if (filter_state == True) filter the view (can this
     *			replace the sort above?)
     *	
     *		set the foooter message (num_items, num_selected)
     *
     *	4) (de)sensitize buttons
     *		Goto button (as per root_dir)
     *		Edit Undo button
     *		others?
    */
#endif
}	/* end of DmOpenInPlace */

/*****************************************************************************
 *  	DmInitFolderView
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *	MORE: belongs in f_view.c
 *****************************************************************************/
static void
DmInitFolderView(DmFolderWinPtr window, int view_index)
{
    DtAttrs		geom_options;
    DtAttrs		dummy_layout_options;	/* not used */
    Widget              parent;
    int n;

#ifdef BROWSE_VIEW
    /* In Browse-View, each view needs its own scrolled window.
     * Create it here.  (In single view we only need a single
     * scrolled window - we can get it from the basewindow
     * as is done below.
     */
    window->views[view_index].rubber = XtCreateManagedWidget("_X_", 
					rubberTileWidgetClass, window->swin, 
					Dm__arg, 2);
    window->views[view_index].swin = XtCreateManagedWidget("_X_", 
				scrolledWindowWidgetClass, 
 				window->views[view_index].rubber, Dm__arg, 0);
#else
    /*
     * 	Use the scrolled window from the SWinGizmo
     */
    window->views[view_index].swin = window->swin;
    DmSetSwinSize(window->swin);
#endif


   DetermineLayoutAttributes(window, 0, &window->views[0].view_type,
			     &geom_options, &dummy_layout_options);

   if (window->views[view_index].view_type == DM_ICONIC)
   	geom_options |= DM_B_WRAPPED_LABEL;

    n = 0;
    XtSetArg(Dm__arg[n], XmNpostSelectProc,	DmButtonSelectProc); n++;
    XtSetArg(Dm__arg[n], XmNpostUnselectProc,	DmButtonSelectProc); n++;
    XtSetArg(Dm__arg[n], XmNdblSelectProc,	DmDblSelectProc); n++;
    XtSetArg(Dm__arg[n], XmNmenuProc,		DmIconMenuProc); n++;
    XtSetArg(Dm__arg[n], XmNclientData,		window); n++;
    XtSetArg(Dm__arg[n], XmNmovableIcons, window->views[view_index].view_type
	== DM_ICONIC);

    XtSetArg(Dm__arg[n], XmNdrawProc, (window->views[view_index].view_type
	== DM_LONG) ?	DmDrawLongIcon :
        (window->views[view_index].view_type == DM_NAME) ? DmDrawNameIcon :
	     /* else */ DmDrawLinkIcon); n++;
    /*
     *	FlatIconBox will use gridWidth and gridHeight
     *  as the scrolling units for the scrolledWindow scrollbars.
     *	We must update these when the view changes.
     *  Set gridRows and gridCols so IconBox can determine its desired
     *  size in case the size does not come from .dtinfo.
     *
     *  FLH_MORE: problem: we cant know the LONG and NAME grid sizes
     *  until the iconbox is created because we need the icon box
     *  font.  For the purpose of initial size, we can just use
     *  the ICONIC view params.  Then, after creating the widget,
     *  we can reset the gridHeight and gridWidth by formatting the view.
     *
     *  FLH MORE: Clean this up to handle odd grid widths correctly.
     */
    XtSetArg(Dm__arg[n], XmNgridHeight, GRID_HEIGHT(Desktop) / 2); n++;
    XtSetArg(Dm__arg[n], XmNgridWidth, GRID_WIDTH(Desktop) / 2);n++;
    XtSetArg(Dm__arg[n], XmNgridRows, 2 * FOLDER_ROWS(Desktop)); n++;
    XtSetArg(Dm__arg[n], XmNgridColumns, 2 * FOLDER_COLS(Desktop) ); n++;
    XtSetArg(Dm__arg[n], XmNtargets,	DESKTOP_DND_TARGETS(Desktop)); n++;
    XtSetArg(Dm__arg[n], XmNnumTargets,
		XtNumber(DESKTOP_DND_TARGETS(Desktop))); n++;
    XtSetArg(Dm__arg[n], XmNdropProc,		DmDropProc); n++;
    XtSetArg(Dm__arg[n], XmNtriggerMsgProc,	DmFolderTriggerNotify); n++;
    XtSetArg(Dm__arg[n], XmNconvertProc,	DtmConvertSelectionProc); n++;
    XtSetArg(Dm__arg[n], XmNdragDropFinishProc,	DtmDnDFinishProc); n++;

    /* FLH MORE: set fixed font here if we are in long view.  Also 
       reset font to default in View CB (f_view.c)
     */
    window->views[view_index].nitems = window->views[view_index].cp->num_objs 
							+ EXTRA_ICON_SLOTS;
    parent = window->views[view_index].swin;
    window->views[view_index].box = DmCreateIconContainer(
					parent,
					geom_options, Dm__arg, n,
			  		window->views[view_index].cp->op, 
					window->views[view_index].cp->num_objs,
			  		&(window->views[view_index].itp), 
					window->views[view_index].nitems,
			  		NULL);
#ifdef DEBUG
{
    /* FLH REMOVE */
    Dimension gridHeight, gridWidth, rows, cols, width, height;
    gridHeight = GRID_HEIGHT(Desktop) / 2;
    gridWidth = GRID_WIDTH(Desktop) / 2;
    rows = 2 * FOLDER_ROWS(Desktop);
    cols = 2 * FOLDER_COLS(Desktop);
    fprintf(stderr,"GRID WxH = %d x %d\n", gridHeight ,gridWidth);
    fprintf(stderr,"rows,cols  %d , %d\n", rows ,cols);
    fprintf(stderr, "width , height = %d , %d\n", rows * gridHeight,
	   cols * gridWidth);
    XtVaGetValues( window->views[view_index].box,
		  XmNwidth, &width,
		  XmNheight, &height,
		  NULL);
    fprintf(stderr, "ICONBOX width , height = %d , %d\n", width, height);
}
#endif
}	/* end of DmInitFolderView */

/****************************procedure*header*****************************
    Dm__UpdateVisitedFolders- update the list of visited folders.
	If the list changes, rebuild the Folder Menu.  (Optimally, the menu
	would only be (re)made when the menu is popped up.)
	This routine is here so it can access the FolderMenu gizmo.

	old_path == NULL  :	add 'new_path'
	new_path == NULL  :	delete 'old_path'
	else			modify 'old_path' to 'new_path'

	For deletions and modifications, descendant paths must also be
	processed.

	Note that deletions do not occur each time a folder is closed but
	rather only when a directory becomes invalid (ie. is deleted).
*/
void
Dm__UpdateVisitedFolders(char * old_path, char * new_path)
{
    int		old_len;
    Boolean	changed = False;
    char *path;
    int i;
    int top_index  = TOP_INDEX;

    if (old_path == NULL) {

	/* ie. ADDITION */

	/* Ensure unique entries */
	for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
	    if (strcmp(FolderPathList[i], new_path) == 0) {
		if (i != top_index) {
		    char *tmp_path;
		    XmString tmp_path_string;

		    tmp_path = FolderPathList[i];
		    tmp_path_string = FolderLabelList[i];
		    SHUFFLE_FOLDERS(i, top_index);
		    FolderPathList[top_index] = tmp_path;
		    FolderLabelList[top_index] = tmp_path_string;
		    changed = True;
		    break;
		} else {
		    /* ignore duplicates */
		     return;			
		}
	    }
	}

	if (!changed) {
	    if (NUM_VISITED(Desktop) == MAX_VISITED_FOLDERS) {
	        /* Free any path that will "fall off the bottom" */
	        FREE(FolderPathList[MAX_VISITED_FOLDERS - 1]);
	        XmStringFree(FolderLabelList[MAX_VISITED_FOLDERS - 1]);
	        SHUFFLE_FOLDERS(MAX_VISITED_FOLDERS - 1, 0);
	    } else {
	        NUM_VISITED(Desktop)++;
	        XtManageChild(FolderWidgets[i]);
	    }

	    i = TOP_INDEX;
	    FolderPathList[i] = strdup(new_path);
	    FolderLabelList[i] = NULL;
	    changed = True;
	}

    } else if (new_path == NULL) {

	/* ie. DELETION */

	old_len = strlen(old_path);

	for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
	    if (DmSameOrDescendant(old_path, FolderPathList[i], old_len)) {
	        FREE(FolderPathList[i]);
	        XmStringFree(FolderLabelList[i]);
		SHUFFLE_FOLDERS(i, top_index);
	        NUM_VISITED(Desktop)--;
		XtUnmanageChild(FolderWidgets[top_index]);
		top_index++;
		changed = True;
	    }
	}

    } else {

	/* ie. MODIFICATION */

	int	result;
	char	*save_path;

	old_len = strlen(old_path);
	for (i = MAX_VISITED_FOLDERS - 1; i >= top_index; i--) {
	    result = DmSameOrDescendant(old_path, FolderPathList[i], old_len);

	    if (result != 0) {
		save_path = FolderPathList[i];

		FolderPathList[i] = strdup((result < 0) ? new_path :
			   DmMakePath(new_path, save_path + old_len + 1));
	        XmStringFree(FolderLabelList[i]);
		FolderLabelList[i] = NULL;

		FREE(save_path);
		changed = True;
	    }
	}
    }

    if (changed)
	MakeFolderMenu(False);
}					/* end of Dm__UpdateVisitedFolders */

/****************************procedure*header*********************************
 * DmGetFolderAttributes: retrieve title, icon name, and icon pixmap name
 *	INPUTS: folder, 
 *		char **'s to return strings
 *		boolean indicating whether this is the Desktop Dolder
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/

static void
DmGetFolderAttributes(DmFolderWindow folder, char **title, char **icon_name,
		      char **icon_pixmap, unsigned char desktop)
{
    char *name;
    int i;
    Boolean found;
    char *dayone_names[] = {
	"Preferences",
	"Disks-etc",
	"Applications",
	"Admin_Tools",
	"Mailbox",
	"Networking",
	"Games",
	"Wallpaper",
	"Startup_Items",
    };
    DtAttrs dayone_attrs[] = {
	DM_B_PREFERENCES_WIN,
	DM_B_DISKS_ETC_WIN,
	DM_B_APPLICATIONS_WIN,
	DM_B_ADMIN_TOOLS_WIN,
	DM_B_MAILBOX_WIN,
	DM_B_NETWORKING_WIN,
	DM_B_GAMES_WIN,
	DM_B_WALLPAPER_WIN,
	DM_B_STARTUP_ITEMS_WIN,
    };

    _DPRINT1(stderr, "DmGetFolderAttributes: Desktop = %d\n", desktop);

    /* Identify window as folder window */
      
    folder->attrs = DM_B_FOLDER_WIN;

    /* Set base window title, icon_name and icon_pixmap. */
    *title = DmMakeFolderTitle(folder);
    if (desktop)
    {
	/* UNIX Desktop folder - its icon name and icon pixmap are fixed */
	*icon_name = *title;
	*icon_pixmap = "dtprop48.icon";
    } else if (!strcmp(folder->views[0].cp->path, NETWARE_PATH)) {
	*icon_name = Dm_DayOneName("NetWare", DESKTOP_LOGIN(Desktop));
	*icon_pixmap = "netware.xpm";
	folder->attrs |= DM_B_NETWARE_WIN;
    } else if (strstr(folder->views[0].cp->path, UUCP_RECEIVE_PATH)) {
	*icon_name = Dm_DayOneName("UUCP_Inbox", DESKTOP_LOGIN(Desktop));
	*icon_pixmap = "uucp.in48";
	folder->attrs |= DM_B_UUCP_INBOX_WIN;
    } else if (strstr(folder->views[0].cp->path, DISK_A)) {
	*icon_name = Dm_DayOneName("Disk_A", DESKTOP_LOGIN(Desktop));
	*icon_pixmap = "dir48.icon";
	folder->attrs |= DM_B_DISKETTE_WIN;
    } else if (strstr(folder->views[0].cp->path, DISK_B)) {
	*icon_name = Dm_DayOneName("Disk_B", DESKTOP_LOGIN(Desktop));
	*icon_pixmap = "dir48.icon";
	folder->attrs |= DM_B_DISKETTE_WIN;
    } else if (strstr(folder->views[0].cp->path, CD_ROM)) {
	*icon_name = Dm_DayOneName(basename(folder->views[0].cp->path),
		     DESKTOP_LOGIN(Desktop));
	*icon_pixmap = "dir48.icon";
	folder->attrs |= DM_B_CDROM_WIN;
    } else {
	/* set name for minimized folder icon */
	*icon_name = ROOT_DIR(DM_WIN_PATH(folder)) ? DM_WIN_PATH(folder) :
		basename(DM_WIN_USERPATH(folder));
	
	/* set icon pixmap file to use for minimized folder icon */
	if ((*icon_pixmap = DmGetFolderIconFile(folder->views[0].cp->path))
  	      == NULL)
	    *icon_pixmap = "dir48.icon";

	for (i=0, found = False; i < XtNumber(dayone_names) && !found; i++){
	    name = Dm_DayOneName(dayone_names[i], DESKTOP_LOGIN(Desktop));
	    if (name && !strcmp(*icon_name, name)){
		folder->attrs |= dayone_attrs[i];
		found = True;
	    }
	    if (name != dayone_names[i])
		FREE(name);
	}
    }
}	/* end of DmGetFolderAttributes */

/****************************procedure*header*********************************
	DmSetFolderMenuItems - Sets menu information depending in type of folder
	window.  Called from DmMenuSelectCB() when something in a folder window's
	menubar is selected to handle shared menus.
 */
void
DmSetFolderMenuItem(MenuItems **ret_menu_items, char *name, int index,
						DmFolderWinPtr folder)
{
	switch(index) {
	case 0:
		if (folder == DESKTOP_TOP_FOLDER(Desktop)) {
			*ret_menu_items = ExitMenuItems;
			strcpy(name, "exitmenu");
		} else if (folder->attrs & DM_B_NETWARE_WIN) {
			*ret_menu_items = NWFileMenuItems;
			strcpy(name, "nwfilemenu");
		} else {
			*ret_menu_items = FileMenuItems;
			strcpy(name, "filemenu");
		}
		break;
	case 1:
		if (folder->attrs & DM_B_NETWARE_WIN) {
			*ret_menu_items = NWEditMenuItems;
			strcpy(name, "nweditmenu");
		} else {
			*ret_menu_items = EditMenuItems;
			strcpy(name, "editmenu");
		}
		break;
	case 2:
		if (folder->attrs & DM_B_NETWARE_WIN) {
			*ret_menu_items = NWViewMenuItems;
			strcpy(name, "nwviewmenu");
		} else {
			*ret_menu_items = ViewMenuItems;
			strcpy(name, "viewmenu");
		}
		break;
	case 3:
		*ret_menu_items = FolderMenuItems;
		strcpy(name, "foldermenu");
		break;
	case 4:
		if (folder == DESKTOP_TOP_FOLDER(Desktop)) {
			*ret_menu_items = DesktopHelpMenuItems;
			strcpy(name, "dthelpmenu");
		}
		else{
			*ret_menu_items = HelpMenuItems;
			strcpy(name, "helpmenu");
		}
		break;
	default:
		break;

	}
} /* end of DmSetFolderMenuItem */

/****************************procedure*header*********************************
 *  DmCheckRoot: check if a folder is rooted and sensitize/desensitize
 *	buttons/menus that allow the user to navigate above the root directory.
 *	These include:
 *			the parent button in the toolbar (only if at root)
 *			The entire Goto menu (always, to prevent Goto Other)
 *			File-->Open-New (always)
 *		NOTE: Open-New is sensitized/desensitized by DmMenuSelectCB
 *		based on the currently selected items.  It must also be
 *		checked against the root_dir there.
 *	FLH MORE: ultimately, we should allow a limited amount of navigation
 *		outside of this folder.  "Descendant" folders should inherit
 *		the same root.  We should allow Goto->Parent when not at root,
 *		Goto->Desktop always, and other Gotos on a limited basis	*	       (only if they are still open).  We should never allow
 *		Goto other.
 *
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
static void
DmCheckRoot(DmFolderWindow window)
{     
    Boolean at_root = False;
    Arg true_arg[2];
    Arg false_arg[2];
    Arg	*arg;
    Widget w;

    if (window->root_dir == NULL)
	return;
    
    if (!strncmp(DM_WIN_PATH(window), window->root_dir, 
		 strlen(DM_WIN_PATH(window))))
	at_root = True;

    
    XtSetArg(true_arg[0], XmNsensitive, True);
    XtSetArg(false_arg[0], XmNsensitive, False);
    arg = at_root ? false_arg : true_arg;

    /* Handle the Parent button in the toolbar */
    w = QueryGizmo (BaseWindowGizmoClass, window->gizmo_shell, GetGizmoWidget,
		    "toolbarMenu:0");
    if (w)
	XtSetValues(w, arg, 1);

    /* Desensitize Goto Menu */
    w = QueryGizmo (BaseWindowGizmoClass, window->gizmo_shell, GetGizmoWidget,
		    "menubar:3");
    if (w)
	XtSetValues(w, false_arg, 1);

}	/* end of DmCheckRoot */
