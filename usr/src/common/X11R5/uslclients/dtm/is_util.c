/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:is_util.c	1.31"

/******************************file*header********************************

    Description:
     This file contains the source code for Icon Setup.
*/
		/* includes go here */
#ifdef DEBUG
#include <X11/Xmu/Editres.h>
#endif

#include "icon_setup.h"

static void PopdownNewWin(CB_PROTO);
static void DestroyPropWin(CB_PROTO);
static void IconSelectCB(CB_PROTO);
static void IconLibPopdownCB(CB_PROTO);
static void NewWinPopdownCB(CB_PROTO);
static void LoadMultiFilesCB(CB_PROTO);
static void TmplNameChangedCB(CB_PROTO);
static int CreateObjsFromList(DmContainerPtr cp);
static DmContainerPtr CreateSystemIconLibrary();
static MenuItems *SetupProgTypeMenu(int ftype);
static void RefreshNewWin(WinInfoPtr wip);
static void UpdateDropSites(WinInfoPtr wip, int new_page);
static void ChangePage(WinInfoPtr wip, int page);
static void UpdateIcon(WinInfoPtr wip);
static Boolean ShowIconLibrary(XtPointer client_data);
static Boolean CustomizedOpenCmd(WinInfoPtr wip, int prev_type, int cur_type);
static Boolean CustomizedDropCmd(WinInfoPtr wip, int prev_type, int cur_type);
static Boolean CustomizedPrintCmd(WinInfoPtr wip, int prev_type, int cur_type);
static Boolean CustomizedIconFile(WinInfoPtr wip, int prev_type, int cur_type);
static void UpdateClassMenu(DmFnameKeyPtr fnkp);

/* Gizmos for all new and properties windows */

static MenuItems addMenuItems[] = {
  { True, TXT_ADD,    TXT_M_ADD,    I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_APPLY,  TXT_M_Apply,  I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_Reset,  TXT_M_Reset,  I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo addMenu = {
	NULL, "addMenu", "", addMenuItems, DmISAddMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 3
};

static MenuItems propMenuItems[] = {
  { True, TXT_OK,     TXT_M_OK,     I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_APPLY,  TXT_M_APPLY,  I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_Reset,  TXT_M_Reset,  I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo propMenu = {
	NULL, "propMenu", "", propMenuItems, DmISAddMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 3
};

static MenuItems fileCategoryMenuItems[] = {
  { False, TXT_BASIC_OPTIONS, TXT_M_BASIC_OPTIONS, I_PUSH_BUTTON, NULL,
	NULL, NULL, True},
  { True,  TXT_FILE_TYPING,   TXT_M_FILE_TYPING,   I_PUSH_BUTTON, NULL,
	NULL, NULL, False},
  { True,  TXT_ICON_ACTIONS,  TXT_M_ICON_ACTIONS,  I_PUSH_BUTTON, NULL,
	NULL, NULL, False},
  { True,  TXT_TEMPLATES,     TXT_M_TEMPLATES,     I_PUSH_BUTTON, NULL,
	NULL, NULL, False},
  { NULL }
};

static MenuGizmo fileCategoryMenu = {
	NULL, "fileCategoryMenu", "", fileCategoryMenuItems,
	DmISCategoryMenuCB, NULL
};

static ChoiceGizmo fileCategoryChoice = {
	NULL,			/* Help information */
	"fileCategoryChoice",	/* Name of menu gizmo */
	&fileCategoryMenu,	/* Choice buttons */
	G_OPTION_BOX,		/* Type of menu created */
};

static GizmoRec fileCategoryRec[] = {
	{ ChoiceGizmoClass, &fileCategoryChoice },
};

static LabelGizmo fileCategoryLabel = {
	NULL,				/* help */
	"fileCategoryLabel",		/* widget name */
	TXT_CATEGORY,			/* caption label */
	False,				/* align caption */
	fileCategoryRec,		/* gizmo array */
	XtNumber(fileCategoryRec),	/* number of gizmos */
};

static SeparatorGizmo separatorGizmo = {
	NULL, "separator", XmSHADOW_ETCHED_OUT, XmHORIZONTAL
};

static Arg inputArgs[] = {
	{ XmNcolumns,		15 },
};

static InputGizmo classNameInput = {
	NULL, "classNameInput", "", 0, NULL
};

static GizmoRec classNameRec[] = {
	{ InputGizmoClass, &classNameInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo classNameLabel = {
	NULL,			/* help */
	"classNameLabel",	/* widget name */
	TXT_CLASS_NAME,		/* caption label */
	False,			/* align caption */
	classNameRec,		/* gizmo array */
	XtNumber(classNameRec),	/* number of gizmos */
};

static ContainerGizmo iconPixmapGizmo = {
	NULL,			/* help */
	"iconPixmapGizmo",	/* widget name */
	G_CONTAINER_FRAME,	/* type */
};

static GizmoRec iconPixmapRec[] = {
	{ ContainerGizmoClass, &iconPixmapGizmo },
};

static LabelGizmo iconPixmapLabel = {
	NULL,				/* help */
	"iconPixmapLabel",		/* widget name */
	TXT_ICON,			/* caption label */
	False,				/* align caption */
	iconPixmapRec,			/* gizmo array */
	XtNumber(iconPixmapRec),	/* number of gizmos */
};

static InputGizmo iconFileInput = {
	NULL, "iconFileInput", "", 0, NULL
};

static GizmoRec iconFileRec[] = {
	{ InputGizmoClass, &iconFileInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo iconFileLabel = {
	NULL,			/* help */
	"iconFileLabel",	/* widget name */
	TXT_ICON_FILE,		/* caption label */
	False,			/* align caption */
	iconFileRec,		/* gizmos */
	XtNumber(iconFileRec),	/* numGizmos */
};

static MenuItems iconMenuItems[] = {
  { True, TXT_ICONS_ELLIPSIS, TXT_M_ICON,        I_PUSH_BUTTON, NULL,
	NULL, NULL, True },
  { True, TXT_UPDATE_ICON,    TXT_M_UPDATE_ICON, I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
  { NULL }
};

static MenuGizmo iconMenu = {
	NULL, "iconMenu", "", iconMenuItems, DmISIconMenuCB, NULL,
	XmHORIZONTAL, 1, 0
};

static Arg rcArgs[] = {
	{ XmNpacking,	XmPACK_TIGHT }
};

static GizmoRec iconFileRCRec[] = {
	{ LabelGizmoClass, &iconFileLabel },
	{ CommandMenuGizmoClass, &iconMenu, rcArgs, XtNumber(rcArgs) },
};

static ContainerGizmo iconFileRC = {
	NULL,				/* help */
	"iconFileRC",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	iconFileRCRec,			/* gizmos */
	XtNumber(iconFileRCRec),	/* number of gizmos */
};

static GizmoRec fileFormGizmoRec0[] = {
	{ LabelGizmoClass, &iconPixmapLabel },
	{ ContainerGizmoClass, &iconFileRC },
};

static ContainerGizmo iconForm = {
	NULL,				/* help */
	"iconForm",			/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec0,		/* gizmos */
	XtNumber(fileFormGizmoRec0),	/* numGizmos */
};

static GizmoRec fileFormGizmoRec1[] = {
	{ LabelGizmoClass, &classNameLabel },
	{ ContainerGizmoClass, &iconForm },
};

static ContainerGizmo fileFormGizmo1 = {
	NULL,				/* help */
	"fileFormGizmo1",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec1,		/* gizmos */
	XtNumber(fileFormGizmoRec1),	/* numGizmos */
};

static InputGizmo patternInput = {
	NULL, "patternInput", "", 0, NULL
};

static GizmoRec patternRec[] = {
	{ InputGizmoClass, &patternInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo patternLabel = {
	NULL,			/* help */
	"patternLabel",		/* widget name */
	TXT_FILENAME_EXTENSION,	/* caption label */
	True,			/* align caption */
	patternRec,		/* gizmo array */
	XtNumber(patternRec),	/* number of gizmos */
};

static InputGizmo programInput = {
	NULL, "programInput", "", 0, NULL
};

static GizmoRec programRec[] = {
	{ InputGizmoClass, &programInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo programLabel = {
	NULL,			/* help */
	"programLabel",		/* widget name */
	TXT_PROGRAM_TO_RUN,	/* caption label */
	True,			/* align caption */
	programRec,		/* gizmo array */
	XtNumber(programRec),	/* number of gizmos */
};

static MenuItems progTypeMenuItems[] = {
  { True, TXT_UNIX_GRAPHICAL, TXT_M_UNIX_GRAPHICAL, I_PUSH_BUTTON, NULL,
	NULL, NULL, True },
  { True, TXT_UNIX_CHARACTER, TXT_M_UNIX_CHARACTER, I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
};

static MenuGizmo fileProgTypeMenu = {
	NULL,			/* help info */
	"fileProgTypeMenu",	/* shell name */
	"",			/* menu title */
	NULL,			/* menu items (run time) */
	DmISProgTypeMenuCB,	/* call back (in items) */
	NULL,			/* client data (in items) */
};

static ChoiceGizmo fileProgTypeChoice = {
	NULL,			/* Help information */
	"fileProgTypeChoice",	/* Name of menu gizmo */
	&fileProgTypeMenu,	/* Choice buttons */
	G_OPTION_BOX,		/* Type of menu created */
};

static GizmoRec fileProgTypeRec[] = {
	{ ChoiceGizmoClass, &fileProgTypeChoice },
};

static LabelGizmo fileProgTypeLabel = {
	NULL,				/* help */
	"fileProgTypeLabel",		/* widget name */
	TXT_PROGRAM_TYPE,		/* caption label */
	False,				/* align caption */
	fileProgTypeRec,		/* gizmo array */
	XtNumber(fileProgTypeRec),	/* number of gizmos */
	G_LEFT_LABEL,			/* label position */
};

static GizmoRec fileRCGizmoRec1[] = {
	{ LabelGizmoClass, &patternLabel },
	{ LabelGizmoClass, &programLabel },
};

static ContainerGizmo fileRCGizmo1 = {
	NULL,				/* help */
	"fileRCGizmo1",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	fileRCGizmoRec1,		/* gizmos */
	XtNumber(fileRCGizmoRec1),	/* numGizmos */
};

static GizmoRec fileFormGizmoRec2[] = {
	{ ContainerGizmoClass, &fileRCGizmo1 },
	{ LabelGizmoClass, &fileProgTypeLabel },
};

static ContainerGizmo fileFormGizmo2 = {
	NULL,				/* help */
	"fileFormGizmo2",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec2,		/* gizmos */
	XtNumber(fileFormGizmoRec2),	/* numGizmos */
};

static GizmoRec fileFrameRec1[] = {
	{ ContainerGizmoClass, &fileFormGizmo2 },
};

static ContainerGizmo fileBasicFrame = {
	NULL,				/* help */
	"fileBasicFrame",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	fileFrameRec1,			/* gizmos */
	XtNumber(fileFrameRec1),	/* numGizmos */
};
static MenuItems yesNoMenuItems[] = {
  { True, TXT_YES, TXT_M_YES, I_RADIO_BUTTON, NULL, NULL, NULL, False },
  { True, TXT_NO,  TXT_M_NO,  I_RADIO_BUTTON, NULL, NULL, NULL, False },
  { NULL }
};

static MenuGizmo yesNoMenu = {
	NULL, "yesNoMenu", "", yesNoMenuItems, NULL, NULL,
	XmHORIZONTAL, 1, 0
};

static ChoiceGizmo yesNoChoice = {
	NULL,			/* Help information */
	"yesNoChoice",		/* Name of menu gizmo */
	&yesNoMenu,		/* Choice buttons */
	G_RADIO_BOX,		/* Type of menu created */
};

static GizmoRec inNewWinRec[] = {
	{ ChoiceGizmoClass, &yesNoChoice },
};

static LabelGizmo inNewWinLabel = {
	NULL,				/* help */
	"inNewWinLabel",		/* widget name */
	TXT_DISPLAY_IN_FILE_NEW_WINDOW,	/* caption label */
	False,				/* align caption */
	inNewWinRec,			/* gizmo array */
	XtNumber(inNewWinRec),		/* number of gizmos */
};

static GizmoRec fileFrameRec2[] = {
	{ LabelGizmoClass, &inNewWinLabel },
};

static ContainerGizmo fileFrameGizmo2 = {
	NULL,				/* help */
	"fileFrameGizmo2",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	fileFrameRec2,			/* gizmos */
	XtNumber(fileFrameRec2),	/* numGizmos */
};

static Arg textArgs[] = {
	{ XmNscrollLeftSide,	False },
	{ XmNscrollHorizontal,	True },
	{ XmNscrollTopSide,	False },
	{ XmNscrollVertical,	True },
	{ XmNwordWrap,		True },
};

static TextGizmo openInput = {
	NULL, "openInput", "", NULL, NULL, 2, 30
};

static GizmoRec openSWRec[] = {
	{ TextGizmoClass, &openInput, textArgs, XtNumber(textArgs) },
};

static ContainerGizmo openSWGizmo = {
	NULL,			/* help */
	"openSWGizmo",		/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	openSWRec,		/* gizmos */
	XtNumber(openSWRec),	/* numGizmos */
};

static Arg swArgs[] = {
	{ XmNscrollingPolicy,        XmAPPLICATION_DEFINED },
	{ XmNvisualPolicy,           XmVARIABLE },
	{ XmNscrollBarDisplayPolicy, XmSTATIC },
};

static GizmoRec openLabelRec[] = {
	{ ContainerGizmoClass, &openSWGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo openLabel = {
	NULL,			/* help */
	"openLabel",		/* widget name */
	TXT_TO_OPEN_FILE,	/* caption label */
	False,			/* align caption */
	openLabelRec,		/* gizmo array */
	XtNumber(openLabelRec),	/* number of gizmos */
};

static TextGizmo printInput = {
	NULL, "printInput", "", NULL, NULL, 2, 30
};

static GizmoRec printSWRec[] = {
	{TextGizmoClass, &printInput, textArgs, XtNumber(textArgs) },
};

static ContainerGizmo printSWGizmo = {
	NULL,			/* help */
	"printSWGizmo",		/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	printSWRec,		/* gizmos */
	XtNumber(printSWRec),	/* numGizmos */
};

static GizmoRec printRec[] = {
	{ ContainerGizmoClass, &printSWGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo printLabel = {
	NULL,			/* help */
	"printLabel",		/* widget name */
	TXT_TO_PRINT_FILE,	/* caption label */
	False,			/* align caption */
	printRec,		/* gizmo array */
	XtNumber(printRec),	/* number of gizmos */
};

static GizmoRec fileActionsRec[] = {
	{ LabelGizmoClass, &openLabel },
	{ LabelGizmoClass, &printLabel },
};

static ContainerGizmo fileActionsRC = {
	NULL,				/* help */
	"fileActionsRC",		/* widget name */
	G_CONTAINER_RC,			/* type */
	200,				/* width */
	60,				/* height */
	fileActionsRec,			/* gizmos */
	XtNumber(fileActionsRec),	/* numGizmos */
};

static LabelGizmo iconActionsLabel = {
	NULL,			/* help */
	"iconActionsLabel",	/* widget name */
	TXT_ICON_ACTIONS,	/* caption label */
	False,			/* align caption */
	NULL,			/* gizmo array */
	0,			/* number of gizmos */
	G_TOP_LABEL,		/* label position */
};

static GizmoRec fileFormGizmoRec4[] = {
	{ LabelGizmoClass,     &iconActionsLabel },
	{ ContainerGizmoClass, &fileActionsRC },
};

static ContainerGizmo fileFormGizmo4 = {
	NULL,				/* help */
	"fileFormGizmo4",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec4,		/* gizmos */
	XtNumber(fileFormGizmoRec4),	/* numGizmos */
};

static GizmoRec fileFrameRec3[] = {
	{ ContainerGizmoClass, &fileFormGizmo4 },
};

static ContainerGizmo fileFrameGizmo3 = {
	NULL,				/* help */
	"fileFrameGizmo3",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	fileFrameRec3,			/* gizmos */
	XtNumber(fileFrameRec3),	/* numGizmos */
};

static GizmoRec fileIconActionsRec[] = {
	{ ContainerGizmoClass, &fileFrameGizmo2 },
	{ ContainerGizmoClass, &fileFrameGizmo3 },
};

static ContainerGizmo fileIconActionsRC = {
	NULL,				/* help */
	"fileIconActionsRC",		/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	fileIconActionsRec,		/* gizmos */
	XtNumber(fileIconActionsRec),	/* numGizmos */
};

/********* Gizmos for file typing constraints  ****************/

static InputGizmo filePathInput = {
	NULL, "filePathInput", "", 0, NULL
};

static GizmoRec filePathRec[] = {
	{ InputGizmoClass, &filePathInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo filePathLabel = {
	NULL,				/* help */
	"filePathLabel",		/* widget name */
	TXT_IN_FOLLOWING_FOLDER,	/* caption label */
	False,				/* align caption */
	filePathRec,			/* gizmo array */
	XtNumber(filePathRec),		/* number of gizmos */
};

static InputGizmo lpatternInput = {
	NULL, "lpatternInput", "", 0, NULL
};

static GizmoRec lpatternRec[] = {
	{ InputGizmoClass, &lpatternInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo lpatternLabel = {
	NULL,			/* help */
	"lpatternLabel",	/* widget name */
	NULL,			/* caption label - set at run time */
	False,			/* align caption */
	lpatternRec,		/* gizmo array */
	XtNumber(lpatternRec),	/* number of gizmos */
};

static InputGizmo lfilePathInput = {
	NULL, "lfilePathInput", "", 0, NULL
};

static GizmoRec lfilePathRec[] = {
	{ InputGizmoClass, &lfilePathInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo lfilePathLabel = {
	NULL,			/* help */
	"lfilePathLabel",	/* widget name */
	NULL,			/* caption label - set at run time */
	False,			/* align caption */
	lfilePathRec,		/* gizmo array */
	XtNumber(lfilePathRec),	/* number of gizmos */
};

static GizmoRec fileRCGizmoRec5[] = {
	{ LabelGizmoClass, &filePathLabel },
	{ LabelGizmoClass, &lpatternLabel },
	{ LabelGizmoClass, &lfilePathLabel },
};

static ContainerGizmo fileRCGizmo5 = {
	NULL,				/* help */
	"fileRCGizmo5",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	fileRCGizmoRec5,		/* gizmos */
	XtNumber(fileRCGizmoRec5),	/* numGizmos */
};

static GizmoRec typingLabelRec1[] = {
	{ ContainerGizmoClass, &fileRCGizmo5 },
};

static LabelGizmo typingLabel1 = {
	NULL,				/* help */
	"typingLabel1",			/* widget name */
	TXT_IN_THIS_CLASS_IF,		/* caption label */
	False,				/* align caption */
	typingLabelRec1,		/* gizmo array */
	XtNumber(typingLabelRec1),	/* number of gizmos */
	G_TOP_LABEL,			/* label position */
};

static GizmoRec constraintsLabelRec[] = {
	{ LabelGizmoClass, &typingLabel1 },
};

static LabelGizmo constraintsLabel = {
	NULL,				/* help */
	"constraintsLabel",		/* widget name */
	TXT_ADDTNL_CONSTRAINTS,		/* caption label */
	False,				/* align caption */
	constraintsLabelRec,		/* gizmo array */
	XtNumber(constraintsLabelRec),	/* number of gizmos */
	G_TOP_LABEL,			/* label position */
};

static GizmoRec fileFormGizmoRec6[] = {
	{ LabelGizmoClass, &constraintsLabel },
};

static ContainerGizmo fileTypingForm = {
	NULL,				/* help */
	"fileTypingForm",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec6,		/* gizmos */
	XtNumber(fileFormGizmoRec6),	/* numGizmos */
};

/********* Gizmos for templates ****************/

static Arg listArgs[] = {
	{ XmNscrollingPolicy,		XmAPPLICATION_DEFINED },
	{ XmNvisualPolicy,		XmVARIABLE },
	{ XmNscrollBarDisplayPolicy,	XmSTATIC },
	{ XmNlistSizePolicy,		XmCONSTANT },
	{ XmNselectionPolicy,		XmSINGLE_SELECT },
};

static ListGizmo tmplListGizmo = {
	NULL,			/* help info */
	"tmplListGizmo",	/* name of widget */
	NULL,			/* items */
	0,			/* numItems */
	2,			/* number of items visible */
	NULL,			/* select callback */
	NULL,			/* clientData */
};

static GizmoRec tmplListContRec[] = {
	{ ListGizmoClass, &tmplListGizmo, listArgs, XtNumber(listArgs) },
};

static ContainerGizmo tmplListContGizmo = {
	NULL,				/* help */
	"tmplContGizmo",		/* widget name */
	G_CONTAINER_SW,			/* type */
	250,				/* width */
	0,				/* height */
	tmplListContRec,		/* gizmos */
	XtNumber(tmplListContRec),	/* numGizmos */
};

static GizmoRec tmplListLabelRec[] = {
	{ ContainerGizmoClass, &tmplListContGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo tmplListLabel = {
	NULL,				/* help */
	"tmplListLabel",		/* widget name */
	TXT_EXISTING_TEMPLATES,		/* caption label */
	False,				/* align caption */
	tmplListLabelRec,		/* gizmo array */
	XtNumber(tmplListLabelRec),	/* number of gizmos */
	G_TOP_LABEL,			/* label position */
};

static InputGizmo tmplNameInput = {
	NULL, "tmplNameInput", "", 0, NULL
};

static GizmoRec tmplNameRec[] = {
	{ InputGizmoClass, &tmplNameInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo tmplNameLabel = {
	NULL,				/* help */
	"tmplNameLabel",		/* widget name */
	TXT_FILENAME,			/* caption label */
	False,				/* align caption */
	tmplNameRec,			/* gizmo array */
	XtNumber(tmplNameRec),		/* number of gizmos */
};

static MenuItems tmplListMenuItems[] = {
  { False, TXT_ADD,    TXT_M_ADD,    I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { False, TXT_MODIFY, TXT_M_MODIFY, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { False, TXT_DELETE, TXT_M_DELETE, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo tmplListMenu = {
	NULL, "tmplListMenu", "", tmplListMenuItems, DmISTemplateMenuCB,
	NULL, XmVERTICAL, 1, 0
};

static MenuItems tmplFindBtnMenuItems[] = {
  { True, TXT_FILE_FIND, TXT_M_FILE_FIND, I_PUSH_BUTTON, NULL, NULL,NULL,True},
  { NULL }
};

static MenuGizmo tmplFindBtnMenu = {
	NULL, "tmplFindBtnMenu", "", tmplFindBtnMenuItems, DmISTmplFindCB,
	NULL, XmVERTICAL, 1, 0
};

static GizmoRec tmplRCRec1[] = {
	{ CommandMenuGizmoClass, &tmplFindBtnMenu },
	{ CommandMenuGizmoClass, &tmplListMenu },
	{ LabelGizmoClass,       &tmplListLabel },
};

static ContainerGizmo tmplRCGizmo1 = {
	NULL,				/* help */
	"tmplRCGizmo1",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	tmplRCRec1,			/* gizmos */
	XtNumber(tmplRCRec1),		/* numGizmos */
};

static GizmoRec tmplRCRec2[] = {
	{ LabelGizmoClass,     &tmplNameLabel },
	{ ContainerGizmoClass, &tmplRCGizmo1 },
};

static ContainerGizmo tmplRCGizmo2 = {
	NULL,				/* help */
	"tmplRCGizmo2",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	tmplRCRec2,			/* gizmos */
	XtNumber(tmplRCRec2),		/* numGizmos */
};

static GizmoRec tmplLabelRec[] = {
	{ ContainerGizmoClass, &tmplRCGizmo2 },
};

static LabelGizmo tmplLabelGizmo = {
	NULL,			/* help */
	"tmplLabelGizmo",	/* widget name */
	TXT_TEMPLATES_COLON,	/* caption label */
	False,			/* align caption */
	tmplLabelRec,		/* gizmo array */
	XtNumber(tmplLabelRec),	/* number of gizmos */
	G_TOP_LABEL,		/* label position */
};

static GizmoRec fileFormGizmoRec7[] = {
	{ LabelGizmoClass, &tmplLabelGizmo },
};

static ContainerGizmo fileFormGizmo7 = {
	NULL,				/* help */
	"fileFormGizmo7",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	fileFormGizmoRec7,		/* gizmos */
	XtNumber(fileFormGizmoRec7),	/* numGizmos */
};

static GizmoRec fileFrameRec4[] = {
	{ ContainerGizmoClass, &fileTypingForm },
};

static ContainerGizmo fileTypingFrame = {
	NULL,				/* help */
	"fileTypingFrame",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	fileFrameRec4,			/* gizmos */
	XtNumber(fileFrameRec4),	/* numGizmos */
};

static GizmoRec fileFrameRec5[] = {
	{ ContainerGizmoClass, &fileFormGizmo7 },
};

static ContainerGizmo tmplFrameGizmo = {
	NULL,				/* help */
	"tmplFrameGizmo",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	fileFrameRec5,			/* gizmos */
	XtNumber(fileFrameRec5),	/* numGizmos */
};

static GizmoRec fileWinGizmos[] = {
	{ LabelGizmoClass,     &fileCategoryLabel },
	{ SeparatorGizmoClass, &separatorGizmo },
	{ ContainerGizmoClass, &fileFormGizmo1 },
	{ ContainerGizmoClass, &fileBasicFrame },
	{ ContainerGizmoClass, &fileTypingFrame },
	{ ContainerGizmoClass, &tmplFrameGizmo },
	{ ContainerGizmoClass, &fileIconActionsRC },
};

static MsgGizmo msgGizmo = {NULL, "footer", " ", " "};

static PopupGizmo fileWinPopup = {
	NULL,				/* help */
	"fileWinPopup",			/* name of shell */
	"",				/* window title */
	&addMenu,			/* pointer to menu info */
	fileWinGizmos,			/* gizmo array */
	XtNumber(fileWinGizmos),	/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

static PopupGizmo filePropWinPopup = {
	NULL,				/* help */
	"fileWinPopup",			/* name of shell */
	"",				/* window title */
	&propMenu,			/* pointer to menu info */
	fileWinGizmos,			/* gizmo array */
	XtNumber(fileWinGizmos),	/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

/* Gizmos for Application Type window */

static MenuItems appCategoryMenuItems[] = {
  { False, TXT_BASIC_OPTIONS, TXT_M_BASIC_OPTIONS, I_PUSH_BUTTON, NULL,
	NULL, NULL, True},
  { True,  TXT_FILE_TYPING,   TXT_M_FILE_TYPING,   I_PUSH_BUTTON, NULL,
	NULL, NULL, False},
  { True,  TXT_ICON_ACTIONS,  TXT_M_ICON_ACTIONS,  I_PUSH_BUTTON, NULL,
	NULL, NULL, False},
  { NULL }
};

static MenuGizmo appCategoryMenu = {
	NULL, "appCategoryMenu", "", appCategoryMenuItems,
	DmISCategoryMenuCB, NULL
};

static ChoiceGizmo appCategoryChoice = {
	NULL,			/* Help information */
	"appCategoryChoice",	/* Name of menu gizmo */
	&appCategoryMenu,	/* Choice buttons */
	G_OPTION_BOX,		/* Type of menu created */
};

static GizmoRec appCategoryRec[] = {
	{ ChoiceGizmoClass, &appCategoryChoice },
};

static LabelGizmo appCategoryLabel = {
	NULL,				/* help */
	"appCategoryLabel",		/* widget name */
	TXT_CATEGORY,			/* caption label */
	False,				/* align caption */
	appCategoryRec,			/* gizmo array */
	XtNumber(appCategoryRec),	/* number of gizmos */
};

static InputGizmo programNameInput = {
	NULL, "programNameInput", "", 0, NULL
};

static GizmoRec programNameRec[] = {
	{ InputGizmoClass, &programNameInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo programNameLabel = {
	NULL,				/* help */
	"programNameLabel",		/* widget name */
	TXT_PROGRAM_NAME,		/* caption label */
	False,				/* align caption */
	programNameRec,			/* gizmo array */
	XtNumber(programNameRec),	/* number of gizmos */
};

static MenuGizmo appProgTypeMenu = {
	NULL,			/* help info */
	"appProgTypeMenu",	/* shell name */
	"",			/* menu title */
	NULL,			/* menu items (run time) */
	DmISProgTypeMenuCB,	/* call back (in items) */
	NULL,			/* client data (in items) */
};

static ChoiceGizmo appProgTypeChoice = {
	NULL,			/* Help information */
	"appProgTypeChoice",	/* Name of menu gizmo */
	&appProgTypeMenu,	/* Choice buttons */
	G_OPTION_BOX,		/* Type of menu created */
};

static GizmoRec appProgTypeRec[] = {
	{ ChoiceGizmoClass, &appProgTypeChoice },
};

static LabelGizmo appProgTypeLabel = {
	NULL,				/* help */
	"appProgTypeLabel",		/* widget name */
	TXT_PROGRAM_TYPE,		/* caption label */
	False,				/* align caption */
	appProgTypeRec,			/* gizmo array */
	XtNumber(appProgTypeRec),	/* number of gizmos */
	G_LEFT_LABEL,			/* label position */
};

static GizmoRec appFormRec1[] = {
	{ LabelGizmoClass, &programNameLabel },
	{ LabelGizmoClass, &appProgTypeLabel },
};

static ContainerGizmo appBasicForm = {
	NULL,				/* help */
	"appBasicForm",			/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	appFormRec1,			/* gizmos */
	XtNumber(appFormRec1),		/* numGizmos */
};

static GizmoRec appFrameRec1[] = {
	{ ContainerGizmoClass, &appBasicForm },
};

static ContainerGizmo appBasicFrame = {
	NULL,				/* help */
	"appBasicFrame",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	appFrameRec1,			/* gizmos */
	XtNumber(appFrameRec1),		/* numGizmos */
};

static GizmoRec toWBLabelRec[] = {
	{ ChoiceGizmoClass, &yesNoChoice },
};

static LabelGizmo toWBLabel = {
	NULL,					/* help */
	"toWBLabel",				/* widget name */
	TXT_CAN_BE_MOVED_TO_WASTEBASKET,	/* caption label */
	False,					/* align caption */
	toWBLabelRec,				/* gizmo array */
	XtNumber(toWBLabelRec),			/* number of gizmos */
};

static GizmoRec wbFrameRec[] = {
	{ LabelGizmoClass, &toWBLabel },
};

static ContainerGizmo wbFrameGizmo = {
	NULL,			/* help */
	"wbFrameGizmo",		/* widget name */
	G_CONTAINER_FRAME,	/* type */
	0,			/* width */
	0,			/* height */
	wbFrameRec,		/* gizmos */
	XtNumber(wbFrameRec),	/* numGizmos */
};

static LabelGizmo runLabel = {
	NULL,			/* help */
	"runLabel",		/* widget name */
	TXT_TO_RUN_PROGRAM,	/* caption label */
	False,			/* align caption */
	openLabelRec,		/* gizmo array */
	XtNumber(openLabelRec),	/* number of gizmos */
};

static TextGizmo dropInput = {
	NULL, "dropInput", "", NULL, NULL, 2, 30
};

static GizmoRec dropSWRec[] = {
	{ TextGizmoClass, &dropInput, textArgs, XtNumber(textArgs) },
};

static ContainerGizmo dropSWGizmo = {
	NULL,			/* help */
	"dropSWGizmo",		/* widget name */
	G_CONTAINER_SW,		/* type */
	0,			/* width */
	0,			/* height */
	dropSWRec,		/* gizmos */
	XtNumber(dropSWRec),	/* numGizmos */
};

static GizmoRec dropRec[] = {
	{ ContainerGizmoClass, &dropSWGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo dropLabel = {
	NULL,			/* help */
	"dropLabel",		/* widget name */
	TXT_TO_PROCESS_DROP,	/* caption label */
	False,			/* align caption */
	dropRec,		/* gizmo array */
	XtNumber(dropRec),	/* number of gizmos */
};

static GizmoRec appActionsRec[] = {
	{ LabelGizmoClass, &runLabel },
	{ LabelGizmoClass, &dropLabel },
};

static ContainerGizmo appRCGizmo1 = {
	NULL,				/* help */
	"appRCGizmo1",			/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	appActionsRec,			/* gizmos */
	XtNumber(appActionsRec),	/* numGizmos */
};

static MenuItems loadMultiMenuItems[] = {
  { True, TXT_LOAD_MULTI_FILES, TXT_M_LOAD_MULTI_FILES, I_TOGGLE_BUTTON,
	NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo loadMultiMenu = {
	NULL, "loadMultiMenu", "", loadMultiMenuItems, LoadMultiFilesCB, NULL,
	XmHORIZONTAL, 1
};

static ChoiceGizmo loadMultiChoice = {
	NULL,			/* Help information */
	"loadMultiChoice",	/* Name of menu gizmo */
	&loadMultiMenu,		/* Choice buttons */
	G_TOGGLE_BOX,		/* Type of menu created */
};

static GizmoRec appIconActionsRec[] = {
	{ LabelGizmoClass,     &iconActionsLabel },
	{ ContainerGizmoClass, &appRCGizmo1 },
	{ ChoiceGizmoClass,    &loadMultiChoice },
};

static ContainerGizmo appIconActionsForm = {
	NULL,				/* help */
	"appIconActionsForm",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	appIconActionsRec,		/* gizmos */
	XtNumber(appIconActionsRec),	/* numGizmos */
};

static GizmoRec appFrameRec2[] = {
	{ ContainerGizmoClass, &appIconActionsForm },
};

static ContainerGizmo appIconActionsFrame = {
	NULL,				/* help */
	"appIconActionsFrame",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	appFrameRec2,			/* gizmos */
	XtNumber(appFrameRec2),		/* numGizmos */
};

static GizmoRec appWinGizmos[] = {
	{ LabelGizmoClass,     &appCategoryLabel },
	{ SeparatorGizmoClass, &separatorGizmo },
	{ ContainerGizmoClass, &fileFormGizmo1 },
	{ ContainerGizmoClass, &appBasicFrame },
	{ ContainerGizmoClass, &fileTypingFrame },
	{ ContainerGizmoClass, &wbFrameGizmo },
	{ ContainerGizmoClass, &appIconActionsFrame },
};

static PopupGizmo appWinPopup = {
	NULL,				/* help */
	"appWinPopup",			/* name of shell */
	"",				/* window title */
	&addMenu,			/* pointer to menu info */
	appWinGizmos,			/* gizmo array */
	XtNumber(appWinGizmos),		/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

static PopupGizmo appPropWinPopup = {
	NULL,				/* help */
	"appWinPopup",			/* name of shell */
	"",				/* window title */
	&propMenu,			/* pointer to menu info */
	appWinGizmos,			/* gizmo array */
	XtNumber(appWinGizmos),		/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

/* Gizmos for Folder Type window */

static InputGizmo folderNameInput = {
	NULL, "folderNameInput", "", 0, NULL
};

static GizmoRec folderNameRec[] = {
	{ InputGizmoClass, &folderNameInput, inputArgs, XtNumber(inputArgs) },
};

static LabelGizmo folderNameLabel = {
	NULL,				/* help */
	"folderNameLabel",		/* widget name */
	TXT_ITS_NAME_IS,		/* caption label */
	False,				/* align caption */
	folderNameRec,			/* gizmo array */
	XtNumber(folderNameRec),	/* number of gizmos */
};

static LabelGizmo folderClassLabel = {
	NULL,			/* help */
	"folderClassLabel",	/* widget name */
	TXT_IN_THIS_CLASS_IF,	/* caption label */
};

static GizmoRec folderRCGizmos[] = {
	{ LabelGizmoClass, &folderNameLabel },
	{ LabelGizmoClass, &filePathLabel },
	{ LabelGizmoClass, &lpatternLabel },
	{ LabelGizmoClass, &lfilePathLabel },
};

static ContainerGizmo folderRCGizmo = {
	NULL,				/* help */
	"folderRCGizmo",		/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	folderRCGizmos,			/* gizmos */
	XtNumber(folderRCGizmos),	/* numGizmos */
};

static GizmoRec folderFormGizmos[] = {
	{ LabelGizmoClass,     &folderClassLabel },
	{ ContainerGizmoClass, &folderRCGizmo },
};

static ContainerGizmo folderFormGizmo = {
	NULL,				/* help */
	"folderFormGizmo",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	folderFormGizmos,		/* gizmos */
	XtNumber(folderFormGizmos),	/* numGizmos */
};

static GizmoRec folderFrameRec[] = {
	{ ContainerGizmoClass, &folderFormGizmo },
};

static ContainerGizmo folderFrameGizmo = {
	NULL,				/* help */
	"folderFrameGizmo",		/* widget name */
	G_CONTAINER_FRAME,		/* type */
	0,				/* width */
	0,				/* height */
	folderFrameRec,			/* gizmos */
	XtNumber(folderFrameRec),	/* numGizmos */
};
static GizmoRec folderWinGizmos[] = {
	{ ContainerGizmoClass, &fileFormGizmo1 },
	{ ContainerGizmoClass, &folderFrameGizmo },
};

static PopupGizmo folderWinPopup = {
	NULL,				/* help */
	"folderWinPopup",		/* name of shell */
	"",				/* window title */
	&addMenu,			/* pointer to menu info */
	folderWinGizmos,		/* gizmo array */
	XtNumber(folderWinGizmos),	/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

static PopupGizmo folderPropWinPopup = {
	NULL,				/* help */
	"folderWinPopup",		/* name of shell */
	"",				/* window title */
	&propMenu,			/* pointer to menu info */
	folderWinGizmos,		/* gizmo array */
	XtNumber(folderWinGizmos),	/* number of gizmos */
	&msgGizmo,			/* footer */
	XmSHADOW_ETCHED_OUT,		/* separator type */
};

/* Gizmos for Icon Library */

static MenuItems iconLibraryMenuItems[] = {
  { True, TXT_OK,     TXT_M_OK,     I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo iconLibraryMenu = {
	NULL, "iconLibraryMenu", "_X", iconLibraryMenuItems,
	DmISIconLibraryMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 1
};

static ContainerGizmo iconLibrarySW = {
	NULL,			/* help */
	"iconLibrarySW",	/* widget name */
	G_CONTAINER_SW,		/* type */
};

static GizmoRec iconLibSWLabelRec[] = {
	{ ContainerGizmoClass, &iconLibrarySW, swArgs, XtNumber(swArgs) },
};

static LabelGizmo iconLibrarySWLabel = {
	NULL,				/* help */
	"iconLibrarySWLabel",		/* widget name */
	TXT_ICONS_FOUND_IN,		/* caption label */
	False,				/* align caption */
	iconLibSWLabelRec,		/* gizmo array */
	XtNumber(iconLibSWLabelRec),	/* number of gizmos */
	G_TOP_LABEL,			/* label position */
};

static MenuItems otherIconMenuItems[] = {
  { True, TXT_UPDATE_LISTING, TXT_M_UPDATE_ICON_LISTING, I_PUSH_BUTTON, NULL,
	NULL, NULL, True },
  { True, TXT_FILE_FIND,      TXT_M_FILE_FIND,           I_PUSH_BUTTON, NULL,
	NULL, NULL, False },
  { NULL }
};

static MenuGizmo otherIconMenu = {
	NULL, "otherIconMenu", "", otherIconMenuItems, DmISOtherIconMenuCB,
	NULL, XmHORIZONTAL, 1, 0
};

static InputGizmo otherIconPathInput = {
	NULL, "otherIconPathInput", "", 0, NULL
};

static GizmoRec otherIconPathRec[] = {
	{InputGizmoClass, &otherIconPathInput, inputArgs, XtNumber(inputArgs)},
	{ CommandMenuGizmoClass, &otherIconMenu },
};

static ContainerGizmo otherIconPathRC = {
	NULL,				/* help */
	"otherIconPathRC",		/* widget name */
	G_CONTAINER_RC,			/* type */
	0,				/* width */
	0,				/* height */
	otherIconPathRec,		/* gizmos */
	XtNumber(otherIconPathRec),	/* numGizmos */
};

static MenuItems iconPathMenuItems[] = {
  { True, TXT_SYSTEM_LIBRARY, TXT_M_SYSTEM, I_RADIO_BUTTON, NULL,
	NULL, NULL, True },
  { True, TXT_ALSO_LOOK,      TXT_M_OTHER,  I_RADIO_BUTTON, NULL,
	NULL, NULL, False },
  { NULL }
};

static MenuGizmo iconPathMenu = {
	NULL, "iconPathMenu", "", iconPathMenuItems, DmISIconPathMenuCB, NULL,
	XmHORIZONTAL, 1, 0
};

static ChoiceGizmo iconPathChoice = {
	NULL,			/* Help information */
	"iconPathChoice",	/* Name of menu gizmo */
	&iconPathMenu,		/* Choice buttons */
	G_RADIO_BOX,		/* Type of menu created */
};

static GizmoRec otherIconPathLabelRec[] = {
	{ ChoiceGizmoClass, &iconPathChoice },
};

static LabelGizmo otherIconPathLabel = {
	NULL,					/* help */
	"otherIconPathLabel",			/* widget name */
	TXT_SHOW_ICONS_IN,			/* caption label */
	False,					/* align caption */
	otherIconPathLabelRec,			/* gizmo array */
	XtNumber(otherIconPathLabelRec),	/* number of gizmos */
};

static GizmoRec libraryIconStubRec[] = {
	{ ContainerGizmoClass, &iconPixmapGizmo },
};

static LabelGizmo libraryIconStubLabel = {
	NULL,				/* help */
	"libraryIconStubLabel",		/* widget name */
	TXT_CURRENT_SELECTION,		/* caption label */
	False,				/* align caption */
	libraryIconStubRec,		/* gizmo array */
	XtNumber(libraryIconStubRec),	/* number of gizmos */
};

static GizmoRec otherIconPathFormRec[] = {
	{ LabelGizmoClass,     &otherIconPathLabel },
	{ ContainerGizmoClass, &otherIconPathRC },
	{ LabelGizmoClass,     &libraryIconStubLabel },
};

static ContainerGizmo otherIconPathForm = {
	NULL,				/* help */
	"otherIconPathForm",		/* widget name */
	G_CONTAINER_FORM,		/* type */
	0,				/* width */
	0,				/* height */
	otherIconPathFormRec,		/* gizmos */
	XtNumber(otherIconPathFormRec),	/* numGizmos */
};


static GizmoRec iconLibraryGizmos[] = {
	{ LabelGizmoClass,     &iconLibrarySWLabel },
	{ ContainerGizmoClass, &otherIconPathForm },
};

static PopupGizmo iconLibraryPopup = {
	NULL,				/* help */
	"iconLibraryPopup",		/* name of shell */
	TXT_ICON_LIBRARY,		/* window title */
	&iconLibraryMenu,		/* pointer to menu info */
	iconLibraryGizmos,		/* gizmo array */
	XtNumber(iconLibraryGizmos),	/* number of gizmos */
};

char *TypeNames[] = { "DATA", "DIR", "EXEC" };

extern char *dflt_iconfile[]= { "datafile.icon", "dir.icon",  "%f.icon"};
extern char *dflt_dflticonfile[]= { "datafile.icon", "dir.icon", "exec.icon" };

extern char *dflt_open_cmd[3][2] = {
	{"exec %_PROG_TO_RUN \"%F\" &","exec xterm -E %_PROG_TO_RUN \"%F\"&" },
	{"exec %s \"%%F\" &", "exec xterm -E %s \"%%F\" &" },
	{"exec \"%F\" &", "exec xterm -E \"%F\" &" }
};

extern char *dflt_drop_cmd[2][2] = {
	 {"exec \"%F\" \"%S\" &", "exec xterm -E \"%F\" \"%S\" &"},
	 {"exec \"%F\" %{\"S*\"} &", "exec xterm -E \"%F\" %{\"S*\"} &"}
};

int num_file_ptypes = 0;
int num_app_ptypes = 0;
static DmFnameKeyPtr *file_tmpl_fnkp = NULL;
static DmFnameKeyPtr *app_tmpl_fnkp = NULL;

/****************************procedure*header*****************************
 * DmISAddMenuCB - Callback for menu in New and Properties window to add or
 * apply changes to a class, reset settings, pop down the window or to
 * display help on the window.
 */
void
DmISAddMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);
	Widget shell = GetPopupGizmoShell(wip->popupGizmo);

	switch(cbs->index) {
	case Add:
	case Apply:
		if (DmISValidateInputs(wip) == -1)
			break;

		if (wip->new_class) {
			/* adding a new class */
			if (DmISInsertClass(wip) == -1) {
				/* print error message */
			} else if (cbs->index == Add) {
				XtPopdown(shell);
				SetBaseWindowLeftMsg(base_fwp->gizmo_shell,
					GGT(TXT_CHANGES_SAVED));
			} else { /* Apply */
				/*reinitialize window in case it's used again*/
				DmISFreeSettings(wip, New);
				DmISFreeSettings(wip, Old);
				DmISInitClassWin(NULL, wip, wip->class_type);
				SetPopupWindowLeftMsg(wip->popupGizmo,
					GGT(TXT_CHANGES_SAVED));
				RefreshNewWin(wip);
			}
		} else {
			/* changing an existing class */
			if (DmISModifyClass(wip) == -1) {
#ifdef DEBUG
				fprintf(stderr,"ModifyClass failed\n");
#endif
			} else if (cbs->index == Add) {
				XtPopdown(shell);
				SetBaseWindowLeftMsg(base_fwp->gizmo_shell,
					GGT(TXT_CHANGES_SAVED));
			} else {
				/* update old settings to new settings */
				DmISFreeSettings(wip, Old);
				DmISCopySettings(wip, NewToOld);
				SetPopupWindowLeftMsg(wip->popupGizmo,
					GGT(TXT_CHANGES_SAVED));
			}
		}
		break;
	case Reset:
		DmISFreeSettings(wip, New);
		DmISCopySettings(wip, OldToNew);
		DmISResetValues(wip);
		break;
	case Cancel:
		XtPopdown(shell);
		break;
	case Help:
		if (wip->new_class)
			DmDisplayHelpSection(
				DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
				NULL, ICON_SETUP_HELPFILE,
				ICON_SETUP_NEW_CLASS_SECT);
		else
			DmDisplayHelpSection(
				DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
				NULL, ICON_SETUP_HELPFILE,
				ICON_SETUP_CHANGE_CLASS_SECT);
		break;
	}
} /* end of DmISAddMenuCB */

/****************************procedure*header*****************************
 * DmISIconMenuCB - Callback for Icons... and Update Icon buttons to display
 * an icon library and update icon preview, respectively.
 */
void
DmISIconMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Icons... */
		XtAddWorkProc((XtWorkProc)ShowIconLibrary, (XtPointer)wip);
		break;
	case 1: /* Update Icon */
		UpdateIcon(wip);
		break;
	}
} /* end of DmISIconMenuCB */

/****************************procedure*header*****************************
 * ShowIconLibrary - Called from IconMenuCB() to display an icon library
 * from which an icon can be chosen to be used for a file class.  By default,
 * a predefined subset of the icons in /usr/X/lib/pixmaps (color display)
 * or /usr/X/lib/bitmaps (monochrome display) are displayed.  The file
 * .iconLibrary in those directories contains the set of icon files to
 * display.  If that file is not found, all icons in those directories
 * are displayed.  The .iconLibrary file is used to avoid displaying all
 * icons (which will take a unacceptably long) and to eliminate all icons
 * which are already being used by existing classes and icons of size other
 * than 32x32.
 */
static Boolean
ShowIconLibrary(XtPointer client_data)
{
	int        n;
	XmString   str;
	Widget     w1, w2;
	char       buf[PATH_MAX];
	DmItemPtr  ip;
	WinInfoPtr wip = (WinInfoPtr)client_data;

	if (wip->iconLibraryGizmo) {
		MapGizmo(PopupGizmoClass, wip->iconLibraryGizmo);
		return(True);
	}
	iconLibraryMenu.clientData = (XtPointer)wip;
	iconPathMenu.clientData    = (XtPointer)wip;
	otherIconMenu.clientData   = (XtPointer)wip;

	if ((wip->lib_fwp = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec)))
		== NULL) {
		return;
	}

	if ((wip->lib_fwp->views[0].cp = CreateSystemIconLibrary()) == NULL) {
		FREE(wip->lib_fwp);
		wip->lib_fwp = NULL;
		return;
	}
	wip->lib_fwp->views[0].nitems    = wip->lib_fwp->views[0].cp->num_objs;
	wip->lib_fwp->views[0].view_type = DM_ICONIC;
	wip->lib_fwp->views[0].sort_type = DM_BY_SIZE;

	wip->iconLibraryGizmo = CreateGizmo(wip->w[W_Shell], PopupGizmoClass,
		&iconLibraryPopup, NULL, 0);

	wip->lib_fwp->shell = GetPopupGizmoShell(wip->iconLibraryGizmo);
	wip->lib_fwp->swin  = QueryGizmo(PopupGizmoClass,
		wip->iconLibraryGizmo, GetGizmoWidget, "iconLibrarySW");

	DmSetSwinSize(wip->lib_fwp->swin);

	XtSetArg(Dm__arg[0], XmNshadowThickness, 2);
	XtSetValues(wip->lib_fwp->swin, Dm__arg, 1);

	BUSY_DESKTOP(base_fwp->shell);

	/* create icon container */
	n = 0;
	XtSetArg(Dm__arg[n], XmNclientData,    (XtPointer)wip); n++;
	XtSetArg(Dm__arg[n], XmNdrawProc,      DmDrawIcon); n++;
	XtSetArg(Dm__arg[n], XmNmovableIcons,  False); n++;
	XtSetArg(Dm__arg[n], XmNexclusives,    True); n++;
	XtSetArg(Dm__arg[n], XmNnoneSet,       True); n++;
	XtSetArg(Dm__arg[n], XmNselectProc,    IconSelectCB); n++;
	XtSetArg(Dm__arg[n], XmNdblSelectProc, NULL); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight,    GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth,     GRID_WIDTH(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridRows,      2 * FOLDER_ROWS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns,   2 * FOLDER_COLS(Desktop)); n++;
	wip->lib_fwp->views[0].box = DmCreateIconContainer(wip->lib_fwp->swin,
		DM_B_CALC_SIZE | DM_B_WRAPPED_LABEL,
		Dm__arg, n,
		wip->lib_fwp->views[0].cp->op,
		wip->lib_fwp->views[0].cp->num_objs,
		&(wip->lib_fwp->views[0].itp),
		wip->lib_fwp->views[0].nitems,
		NULL);

	w1 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo, GetGizmoWidget,
		"iconLibrarySWLabel");

	sprintf(buf, "%s %s", GGT(TXT_ICONS_FOUND_IN),
		wip->lib_fwp->views[0].cp->path);
	str = XmStringCreateLocalized(buf);
	XtSetArg(Dm__arg[0], XmNlabelString, str);
	XtSetValues(w1, Dm__arg, 1);
	XmStringFree(str);

	w1 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo, GetGizmoWidget,
		"iconPathMenu");
	XtSetArg(Dm__arg[0], XmNpacking, XmPACK_TIGHT);
	XtSetValues(w1, Dm__arg, 1);

	w1 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo, GetGizmoWidget,
		"otherIconMenu");
	XtSetArg(Dm__arg[0], XmNpacking, XmPACK_TIGHT);
	XtSetValues(w1, Dm__arg, 1);

	w1 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "otherIconPathLabel");
	w1 = XtParent(w1);

	w2 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "otherIconPathRC");

        n = 0;
	XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
	XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(Dm__arg[n], XmNleftOffset,     2); n++;
	XtSetValues(w1, Dm__arg, n);

	n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,     1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,    XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,        XmPACK_COLUMN); n++;
	XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_WIDGET); n++;
	XtSetArg(Dm__arg[n], XmNtopWidget,      w1); n++;
	XtSetArg(Dm__arg[n], XmNtopOffset,      2); n++;
	XtSetArg(Dm__arg[n], XmNrightAttachment,XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(Dm__arg[n], XmNrightWidget,    w1); n++;
	XtSetValues(w2, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "iconPixmapGizmo");

	if (VAR_ICON_FILE(NEW_VAL(icon_file)))
		(void)DmISUpdateIconStub(NEW_VAL(dflt_icon_file), w2,
			wip->lib_fwp, &wip->p[P_IconPixmap], True);
	else
		(void)DmISUpdateIconStub(NEW_VAL(icon_file), w2,
			wip->lib_fwp, &wip->p[P_IconPixmap], True);

	/* Make the item for the current selection selected, if it's found. */ 
	for (n = 0, ip = wip->lib_fwp->views[0].itp;
		n < wip->lib_fwp->views[0].nitems; n++, ip++)
	{
		if (ITEM_MANAGED(ip) &&
			!strcmp(basename(NEW_VAL(icon_file)),
				ITEM_OBJ(ip)->name))
		{
			XtSetArg(Dm__arg[0], XmNset, True);
			ExmFlatSetValues(wip->lib_fwp->views[0].box,
				(ip - wip->lib_fwp->views[0].itp), Dm__arg, 1);
			break;
		}
	}

	w2 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "libraryIconStubLabel");
	w2 = XtParent(w2);

	n = 0;
	XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_WIDGET); n++;
	XtSetArg(Dm__arg[n], XmNtopWidget,      w1); n++;
	XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(w2, Dm__arg, n);

	XtAddCallback(wip->lib_fwp->shell, XtNpopdownCallback,
		DmISDestroyIconLibrary, (XtPointer)wip);

	XmAddWMProtocolCallback(wip->lib_fwp->shell, 
		XA_WM_DELETE_WINDOW(XtDisplay(wip->lib_fwp->shell)),
		IconLibPopdownCB, (XtPointer)wip);

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(wip->lib_fwp->shell, Dm__arg, 1);

	/* unmanage the controls for "Other" library path */
	w2 = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "otherIconPathRC");
	XtUnmanageChild(w2);

	DmSortItems(wip->lib_fwp, DM_BY_NAME, DM_B_CALC_SIZE, 0, 0);

	MapGizmo(PopupGizmoClass, wip->iconLibraryGizmo);
	return(True);

} /* end of ShowIconLibraryCB */

/****************************procedure*header*****************************
 * UpdateIcon - Called from IconMenuCB() to update the icon preview to
 * the icon file specified in the Icon File text field.
 */
static void
UpdateIcon(WinInfoPtr wip)
{
	Gizmo inputGiz = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "iconFileInput");
	char *s = GetInputGizmoText(inputGiz);

	if (VALUE(s) && strcmp(s, NEW_VAL(icon_file))) {
		char *icon_file;

		if (VAR_ICON_FILE(s))
		{
			/* if no default, use default for the file type */
			if (OLD_VAL(dflt_icon_file) == NULL)
				OLD_VAL(dflt_icon_file) = STRDUP(
					dflt_dflticonfile[wip->class_type]);
			icon_file = OLD_VAL(dflt_icon_file);
		} else
			icon_file = s;

		if (DmISUpdateIconStub(icon_file, wip->w[W_IconStub],
			base_fwp, &wip->p[P_IconStub], True) == -1)
		{
#ifdef DEBUG
			fprintf(stderr,"UpdateIcon(): invalid icon file\n");
#endif
		} else {
			XtFree(NEW_VAL(icon_file));
			NEW_VAL(icon_file) = STRDUP(s);
		}
	 } else {
#ifdef DEBUG
		fprintf(stderr, "UpdateIcon: file name is either NULL or \"
			"unchanged or contains %\n");
#endif
	}
	XtFree(s);

} /* end of UpdateIcon */

/****************************procedure*header*****************************
 * DmISCategoryMenuCB - Callback for Category menu to change to the selected
 * page.
 */
void
DmISCategoryMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	ChangePage(wip, cbs->index);

} /* end of DmISCategoryMenuCB */

/****************************procedure*header*****************************
 * UpdateDropSites - Activate drop sites in new page and deactivate those
 * in unmanaged pages.
 */
static void
UpdateDropSites(WinInfoPtr wip, int new_page)
{
	switch(new_page) {
	case BasicOptions:
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_ACTIVE);
		XmDropSiteUpdate(wip->w[W_IconFile], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_IconStub], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Pattern], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_ProgToRun], Dm__arg, 1);
		}
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_INACTIVE);
		XmDropSiteUpdate(wip->w[W_LPattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_FilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LFilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Open], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_Print], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_TmplName], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_List], Dm__arg, 1);
		} else {
			/* AppType */
			XmDropSiteUpdate(wip->w[W_Drop], Dm__arg, 1);
		}
		break;
	case FileTyping:
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_ACTIVE);
		XmDropSiteUpdate(wip->w[W_LPattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_FilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LFilePath], Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_INACTIVE);
		XmDropSiteUpdate(wip->w[W_IconFile], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_IconStub], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Pattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Open], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_ProgToRun], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_Print], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_TmplName], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_List], Dm__arg, 1);
		} else {
			/* AppType */
			XmDropSiteUpdate(wip->w[W_Drop], Dm__arg, 1);
		}
		break;
	case IconActions:
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_ACTIVE);
		XmDropSiteUpdate(wip->w[W_Open], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_Print], Dm__arg, 1);
		} else {
			/* AppType */
			XmDropSiteUpdate(wip->w[W_Drop], Dm__arg, 1);
		}
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_INACTIVE);
		XmDropSiteUpdate(wip->w[W_IconFile], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_IconStub], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Pattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LPattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_FilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LFilePath], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_ProgToRun], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_TmplName], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_List], Dm__arg, 1);
		}
		break;
	case Templates:
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_ACTIVE);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_TmplName], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_List], Dm__arg, 1);
		}
		XtSetArg(Dm__arg[0], XmNdropSiteActivity, XmDROP_SITE_INACTIVE);
		XmDropSiteUpdate(wip->w[W_IconFile], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_IconStub], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Pattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LPattern], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_FilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_LFilePath], Dm__arg, 1);
		XmDropSiteUpdate(wip->w[W_Open], Dm__arg, 1);
		if (wip->class_type == FileType) {
			XmDropSiteUpdate(wip->w[W_ProgToRun], Dm__arg, 1);
			XmDropSiteUpdate(wip->w[W_Print], Dm__arg, 1);
		} else {
			/* AppType */
			XmDropSiteUpdate(wip->w[W_Drop], Dm__arg, 1);
		}
		break;
	}

} /* end of UpdateDropSites */

/****************************procedure*header*****************************
 * ChangePage - Change to the selected page.
 */
static void
ChangePage(WinInfoPtr wip, int page)
{
	Widget w1, w2, w3, w4, w5;
	Widget btn1, btn2, btn3, btn4;

	if (wip->class_type == FileType) {
		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileBasicFrame");
		w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileTypingFrame");
		w3 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileIconActionsRC");
		w4 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "tmplFrameGizmo");

		btn1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu:0");
		btn2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu:1");
		btn3 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu:2");
		btn4 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu:3");
	} else if (wip->class_type == AppType) {
		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appBasicFrame");
		w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileTypingFrame");
		w3 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appIconActionsFrame");
		w4 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "wbFrameGizmo");

		btn1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appCategoryMenu:0");
		btn2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appCategoryMenu:1");
		btn3 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appCategoryMenu:2");
	}
	w5 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
		GetGizmoWidget, "iconForm");

	switch(page) {
	case BasicOptions:
		/* show Basic Options page */
		XtManageChild(w1);
		XtManageChild(w5);
		XtUnmanageChild(w2);
		XtUnmanageChild(w3);
		XtUnmanageChild(w4);

		/* update state of Category menu buttons */
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn1, Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn2, Dm__arg, 1);
		XtSetValues(btn3, Dm__arg, 1);
		if (wip->class_type == FileType)
			XtSetValues(btn4, Dm__arg, 1);

		break;
	case FileTyping:
		/* show File Typing page */
		XtUnmanageChild(w1);
		XtUnmanageChild(w5);
		XtUnmanageChild(w3);
		XtUnmanageChild(w4);
		XtManageChild(w2);

		/* update state of Category menu buttons */
		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn1, Dm__arg, 1);
		XtSetValues(btn3, Dm__arg, 1);
		if (wip->class_type == FileType)
			XtSetValues(btn4, Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn2, Dm__arg, 1);
		break;
	case IconActions:
#ifdef NOT_USED
		if (wip->class_type == FileType && !strcmp(NEW_VAL(open_cmd),
			dflt_open_cmd[FileType][NEW_VAL(prog_type)]))
		{
			char *p = GetInputGizmoText(wip->g[G_ProgToRun]);

			XtFree(NEW_VAL(prog_to_run));
			if (VALUE(p))
			{
				NEW_VAL(prog_to_run) = STRDUP(p1);
				sprintf(Dm__buffer,
					dflt_open_cmd[1][NEW_VAL(prog_type)],
					NEW_VAL(prog_to_run));
				SetTextGizmoText(wip->g[G_Open], Dm__buffer);
			} else {
				/* reset to default */
				NEW_VAL(prog_to_run) = NULL;
				SetTextGizmoText(wip->g[G_Open], 
				 dflt_open_cmd[FileType][NEW_VAL(prog_type)]);
			}
			XtFree(p);
		} else if (wip->class_type == AppType &&
			!strcmp(NEW_VAL(drop_cmd),
			  dflt_drop_cmd[0][NEW_VAL(prog_type)]))
		{
			SetTextGizmoText(wip->g[G_Open], NEW_VAL(open_cmd));
			SetTextGizmoText(wip->g[G_Drop], NEW_VAL(drop_cmd));
		}
#endif
		XtUnmanageChild(w1);
		XtUnmanageChild(w2);
		XtUnmanageChild(w5);
		if (wip->class_type == FileType)
			XtUnmanageChild(w4);
		else
			XtManageChild(w4);
		XtManageChild(w3);

		/* update state of Category menu buttons */
		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn1, Dm__arg, 1);
		XtSetValues(btn2, Dm__arg, 1);
		if (wip->class_type == FileType)
			XtSetValues(btn4, Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn3, Dm__arg, 1);
		break;
	case Templates:
		XtUnmanageChild(w1);
		XtUnmanageChild(w2);
		XtUnmanageChild(w3);
		XtUnmanageChild(w5);
		XtManageChild(w4);

		XtSetArg(Dm__arg[0], XmNsensitive, True);
		XtSetValues(btn1, Dm__arg, 1);
		XtSetValues(btn2, Dm__arg, 1);
		XtSetValues(btn3, Dm__arg, 1);
		XtSetArg(Dm__arg[0], XmNsensitive, False);
		XtSetValues(btn4, Dm__arg, 1);
		break;
	}
	UpdateDropSites(wip, page);

} /* end of ChangePage */

/****************************procedure*header*****************************
 * DmISCreateFileWin - Called from NewMenuCB() and DmISShowProperties() to
 * display a New or Properties window, respectively, for DATA type file class.
 */
Widget
DmISCreateFileWin(DmObjectPtr op, WinInfoPtr wip, Boolean *bad_icon_file)
{
	int    n;
	Widget w1, w2;
	char   buf[PATH_MAX];

	DmISInitClassWin(op, wip, FileType);
	if (!op && NewFileShell) {
		RefreshNewWin(wip);
		return(NewFileShell);
	}
	lpatternLabel.label  = TXT_LINKED_TO_FILE;
	lfilePathLabel.label = TXT_LINKED_TO_FILE_IN;
	fileCategoryMenu.clientData = (XtPointer)wip;
	if (op) {
		DmFnameKeyPtr fnkp = (DmFnameKeyPtr)(op->fcp->key);

		if ((fnkp->attrs & DM_B_DONT_CHANGE) ||
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES &&
				!PRIVILEGED_USER()))
		{
			/* make OK and Apply buttons insensitive */
			propMenuItems[0].sensitive = (XtArgVal)False;
			propMenuItems[1].sensitive = (XtArgVal)False;
			msgGizmo.rightMsgText      = TXT_READ_ONLY;
		} else {
			/* make OK and Apply buttons sensitive */
			propMenuItems[0].sensitive = (XtArgVal)True;
			propMenuItems[1].sensitive = (XtArgVal)True;
			msgGizmo.rightMsgText      = "";
		}
		propMenu.clientData = (XtPointer)wip;
	} else {
		msgGizmo.rightMsgText = "";
		addMenu.clientData    = (XtPointer)wip;
	}
	iconMenu.clientData        = (XtPointer)wip;
	tmplFindBtnMenu.clientData = (XtPointer)wip;
	tmplListMenu.clientData    = (XtPointer)wip;

	fileProgTypeMenu.items[0].clientData = (XtPointer)wip;
	fileProgTypeMenu.items[1].clientData = (XtPointer)wip;

	if (num_file_ptypes > 2) {
		MenuInfoPtr mip = (MenuInfoPtr)MALLOC((num_file_ptypes-2) *
			sizeof(MenuInfo));
		wip->mip = mip;

		/* Note that file_tmpl_fnkp[0] and file_tmpl_fnkp[1] are
		 * not being used.
		 */
		for (n = 2; n < num_file_ptypes; ++n, ++mip) {
			mip->fnkp = file_tmpl_fnkp[n];
			mip->wip  = wip;
			fileProgTypeMenu.items[n].clientData = (XtPointer)mip;
		}
	}

	wip->popupGizmo = CreateGizmo(base_fwp->shell, PopupGizmoClass,
		(op ? &filePropWinPopup : &fileWinPopup), NULL, 0);

	DmISGetIDs(wip, FileType);
	UpdateDropSites(wip, BasicOptions);

	if (DmISShowIcon(wip) == -1)
		*bad_icon_file = True;

	XtAddCallback(wip->w[W_TmplName], XmNmodifyVerifyCallback,
		TmplNameChangedCB, (XtPointer)wip);
	XtAddCallback(wip->w[W_TmplName], XmNvalueChangedCallback,
		TmplNameChangedCB, (XtPointer)wip);

	SET_INPUT_GIZMO_TEXT(wip->g[G_ClassName], OLD_VAL(class_name));
	SET_INPUT_GIZMO_TEXT(wip->g[G_ProgToRun], OLD_VAL(prog_to_run));
	SET_INPUT_GIZMO_TEXT(wip->g[G_Pattern],   OLD_VAL(pattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LPattern],  OLD_VAL(lpattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_FilePath],  OLD_VAL(filepath));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LFilePath], OLD_VAL(lfilepath));
	SetTextGizmoText(wip->g[G_Open],       OLD_VAL(open_cmd));
	SetTextGizmoText(wip->g[G_Print],      OLD_VAL(print_cmd));

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"classNameLabel");
	w1 = XtParent(w1);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconForm");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,      w1); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,      10); n++;
        XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconPixmapLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconFileRC");

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNleftWidget,      w1); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,      10); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNnumColumns,      1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,     XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,         XmPACK_COLUMN); n++;
        XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileBasicFrame");

	n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_FORM); n++;
	XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"inNewWinLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileRCGizmo1");
        n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,     1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,    XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,        XmPACK_COLUMN); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,     2); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileProgTypeLabel");
	w2 = XtParent(w2);

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,        w1); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomOffset,     2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,       2); n++;
        XtSetValues(w2, Dm__arg, n);

	/* file typing page */

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileRCGizmo5");
        n = 0;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,     2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,      2); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"typingLabel1");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,     2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,      2); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"constraintsLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,        2); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,       2); n++;
        XtSetArg(Dm__arg[n], XmNbottomOffset,     2); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplRCGizmo1");

        n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,  3); n++;
        XtSetArg(Dm__arg[n], XmNorientation, XmHORIZONTAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,     XmPACK_TIGHT); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplRCGizmo2");

        n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,  1); n++;
        XtSetArg(Dm__arg[n], XmNorientation, XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,     XmPACK_TIGHT); n++;
        XtSetValues(w2, Dm__arg, n);

	XtAddCallback(wip->w[W_List], XmNsingleSelectionCallback,
		(XtCallbackProc)DmISTmplListSelectCB, (XtPointer)wip);

	if (op) {
		/* display templates, if any */
		char *p = DmGetObjProperty(op, TEMPLATE, NULL);

		if (p)
			DisplayTemplates(OLD_VAL(tmpl_list),
				OLD_VAL(num_tmpl), wip->w[W_List]);
	}
	/* update Program Type menu */
	DmISResetProgTypeMenu(wip, "fileProgTypeMenu", OLD_VAL(prog_type),
		num_file_ptypes);

	/* update Display In New window menu */
	DmISResetYesNoMenu(wip, OLD_VAL(in_new));

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileTypingFrame");

	/* unmanage the File Typing page */
	XtUnmanageChild(w1);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplFrameGizmo");

	/* unmanage the Templates page */
	XtUnmanageChild(w1);

	/* end of file typing page */

	/* start of Icon Actions page */

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileActionsRC");
	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconActionsLabel");

        n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,       1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,      XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,          XmPACK_TIGHT); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,        w2); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
	XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileIconActionsRC");

	/* unmanage Icon Actions page */
	XtUnmanageChild(w1);

	/* end of Icon Actions page */

	/* register icon stub as a drop site */
        XtSetArg(Dm__arg[0], XmNuserData, (XtPointer)wip);
	XtSetValues(wip->w[W_IconStub], Dm__arg, 1);

	n = 0;
        XtSetArg(Dm__arg[n], XmNimportTargets,      targets); n++;
        XtSetArg(Dm__arg[n], XmNnumImportTargets,   XtNumber(targets)); n++;
        XtSetArg(Dm__arg[n], XmNdropSiteOperations, XmDROP_COPY); n++;
        XtSetArg(Dm__arg[n], XmNdropProc,           DmISIconStubDropCB); n++;
	XmDropSiteRegister(wip->w[W_IconStub], Dm__arg, n);

	if (op) {
		sprintf(buf, "%s: %s %s", GGT(TXT_IB_TITLE),
			OLD_VAL(label) ? OLD_VAL(label) : OLD_VAL(class_name),
			GGT(TXT_PROPERTIES));
		op->objectdata = (void *)wip;
	}
	XtSetArg(Dm__arg[0], XtNtitle, (op ? buf :
		(base_fwp->attrs & DM_B_SYSTEM_CLASSES)?
		GGT(TXT_NEW_SYSTEM_FILE) : GGT(TXT_NEW_PERSONAL_FILE)));
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	XtAddCallback(wip->w[W_Shell], XtNpopdownCallback,
		op ? DestroyPropWin : PopdownNewWin, (XtPointer)wip);

	XmAddWMProtocolCallback(wip->w[W_Shell], 
		XA_WM_DELETE_WINDOW(XtDisplay(wip->w[W_Shell])), 
		NewWinPopdownCB, (XtPointer)wip);

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	/* register callback for context-sensitive help */
	wip->hip          = (DmHelpInfoPtr)MALLOC(sizeof(DmHelpInfoRec));
	wip->hip->app_id  = ICON_SETUP_HELP_ID(Desktop);
	wip->hip->file    = STRDUP(ICON_SETUP_HELPFILE);
	wip->hip->section = op ? STRDUP(ICON_SETUP_CHANGE_CLASS_SECT) :
		STRDUP(ICON_SETUP_NEW_CLASS_SECT);
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		NULL);
	XtAddCallback(w1, XmNhelpCallback, DmPopupWinHelpKeyCB,
		(XtPointer)(wip->hip));

	DmISRegisterFocusEH(wip);
	MapGizmo(PopupGizmoClass, wip->popupGizmo);

#ifdef DEBUG
	XtAddEventHandler(wip->w[W_Shell], (EventMask) 0, True,
		_XEditResCheckMessages, NULL);
#endif

	/* move focus to Category menu */
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileCategoryMenu:0");
	XmProcessTraversal(w1, XmTRAVERSE_CURRENT);

	return(wip->w[W_Shell]);

} /* end of DmISCreateFileWin */

/****************************procedure*header*****************************
 * DestroyPropWin - popdownCallback for a Properties window.  Frees all
 * resources allocated for the window.
 */
static void
DestroyPropWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	int i;
	DmItemPtr ip;
	WinInfoPtr wip = (WinInfoPtr)client_data;

	for (i = 0, ip = base_fwp->views[0].itp; i < base_fwp->views[0].nitems;
		i++,ip++)
	{
		if (ITEM_MANAGED(ip)) {
			if (!strcmp(NEW_VAL(class_name), ITEM_OBJ(ip)->name))
			{
				Boolean set;

				ITEM_OBJ(ip)->objectdata = NULL;
				/* unbusy item */
				XtSetArg(Dm__arg[0], XmNsensitive, True);
				ExmFlatSetValues(base_fwp->views[0].box,
					(ip - base_fwp->views[0].itp),
					Dm__arg, 1);
				/* reactivate Delete and Properties buttons
				 * in Class menu if item is selected
				 */
				XtSetArg(Dm__arg[0], XmNset, &set);
				ExmFlatGetValues(base_fwp->views[0].box,
					(ip - base_fwp->views[0].itp),
					Dm__arg, 1);
				if (set)
					UpdateClassMenu(ITEM_CLASS(ip));
				break;
			}
		}

	}
	DmISFreeSettings(wip, New);
	DmISFreeSettings(wip, Old);

	/* destroy property window and all its sub-windows
	 * (Icon Library, Path Finder) if they were created
	 */ 
	if (wip->tmplFileGizmo) {
		/* Template path finder will be destroyed in popdownCallback */
		XtPopdown(GetFileGizmoShell(wip->tmplFileGizmo));
	}
	if (wip->iconLibraryGizmo) {
		/* Icon Library will be destroyed in popdownCallback */
		XtPopdown(wip->lib_fwp->shell);
	}

	FREE(wip->hip->file);
	FREE(wip->hip->section);
	FREE(wip->hip);

	if (wip->mip)
		FREE(wip->mip);

	FreeGizmo(PopupGizmoClass, wip->popupGizmo);
	XtUnmapWidget(wip->w[W_Shell]);
	XtDestroyWidget(wip->w[W_Shell]);
	FREE(wip);

} /* end of DestroyPropWin */

/****************************procedure*header*****************************
 * DmISCreateFolderWin - Creates a New or Properties window for a DIR type
 * class.
 */
Widget
DmISCreateFolderWin(DmObjectPtr op, WinInfoPtr wip, Boolean *bad_icon_file)
{
	int    n;
	Widget w1, w2;
	Gizmo  giz;
	char   buf[PATH_MAX];

	DmISInitClassWin(op, wip, DirType);
	
	if (!op && NewFolderShell) {
		RefreshNewWin(wip);
		return(NewFolderShell);
	}
	if (op) {
		DmFnameKeyPtr fnkp = (DmFnameKeyPtr)(op->fcp->key);

		if ((fnkp->attrs & DM_B_DONT_CHANGE) ||
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES &&
				!PRIVILEGED_USER()))
		{
			/* make OK and Apply buttons insensitive */
			propMenuItems[0].sensitive = (XtArgVal)False;
			propMenuItems[1].sensitive = (XtArgVal)False;
			msgGizmo.rightMsgText      = TXT_READ_ONLY;
		} else {
			/* make OK and Apply buttons sensitive */
			propMenuItems[0].sensitive = (XtArgVal)True;
			propMenuItems[1].sensitive = (XtArgVal)True;
			msgGizmo.rightMsgText      = "";
		}
		propMenu.clientData = (XtPointer)wip;
	} else {
		msgGizmo.rightMsgText = "";
		addMenu.clientData    = (XtPointer)wip;
	}
	lpatternLabel.label  = TXT_LINKED_TO_FOLDER;
	lfilePathLabel.label = TXT_LINKED_TO_FOLDER_IN;
	iconMenu.clientData  = (XtPointer)wip;

	wip->popupGizmo = CreateGizmo(base_fwp->shell, PopupGizmoClass,
		(op ? &folderPropWinPopup : &folderWinPopup), NULL, 0);

	DmISGetIDs(wip, DirType);

	if (DmISShowIcon(wip) == -1)
		*bad_icon_file = True;

	SET_INPUT_GIZMO_TEXT(wip->g[G_ClassName], OLD_VAL(class_name));
	SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile],  OLD_VAL(icon_file));
	SET_INPUT_GIZMO_TEXT(wip->g[G_Pattern],   OLD_VAL(pattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LPattern],  OLD_VAL(lpattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_FilePath],  OLD_VAL(filepath));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LFilePath], OLD_VAL(lfilepath));

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"classNameLabel");
	w1 = XtParent(w1);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
		GetGizmoWidget, "iconForm");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,     w1); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,     10); n++;
        XtSetValues(w2, Dm__arg, n);


	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconPixmapLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconFileRC");

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNleftWidget,      w1); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,      10); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNnumColumns,      1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,     XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,         XmPACK_COLUMN); n++;
        XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"folderClassLabel");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,     2); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"folderRCGizmo");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,        w1); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,       2); n++;
        XtSetArg(Dm__arg[n], XmNbottomOffset,     2); n++;
        XtSetValues(w2, Dm__arg, n);


	/* register icon stub as a drop site */
        XtSetArg(Dm__arg[0], XmNuserData, (XtPointer)wip);
	XtSetValues(wip->w[W_IconStub], Dm__arg, 1);

	n = 0;
        XtSetArg(Dm__arg[n], XmNimportTargets,      targets); n++;
        XtSetArg(Dm__arg[n], XmNnumImportTargets,   XtNumber(targets)); n++;
        XtSetArg(Dm__arg[n], XmNdropSiteOperations, XmDROP_COPY); n++;
        XtSetArg(Dm__arg[n], XmNdropProc,           DmISIconStubDropCB); n++;
	XmDropSiteRegister(wip->w[W_IconStub], Dm__arg, n);


	if (op) {
		sprintf(buf, "%s: %s %s", GGT(TXT_IB_TITLE),
			OLD_VAL(label) ? OLD_VAL(label) :
			OLD_VAL(class_name), GGT(TXT_PROPERTIES));
		op->objectdata = (void *)wip;
	}

	XtSetArg(Dm__arg[0], XtNtitle, (XtArgVal)(op ? buf :
		(base_fwp->attrs & DM_B_SYSTEM_CLASSES)?
			GGT(TXT_NEW_SYSTEM_FOLDER) :
			GGT(TXT_NEW_PERSONAL_FOLDER)));
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	XtAddCallback(wip->w[W_Shell], XtNpopdownCallback,
		op ? DestroyPropWin : PopdownNewWin, (XtPointer)wip);

	XmAddWMProtocolCallback(wip->w[W_Shell], 
		XA_WM_DELETE_WINDOW(XtDisplay(wip->w[W_Shell])), 
		NewWinPopdownCB, (XtPointer)wip);

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	/* register callback for context-sensitive help */
	wip->hip          = (DmHelpInfoPtr)MALLOC(sizeof(DmHelpInfoRec));
	wip->hip->app_id  = ICON_SETUP_HELP_ID(Desktop);
	wip->hip->file    = STRDUP(ICON_SETUP_HELPFILE);
	wip->hip->section = op ? STRDUP(ICON_SETUP_CHANGE_CLASS_SECT) :
		STRDUP(ICON_SETUP_NEW_CLASS_SECT);
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		NULL);
	XtAddCallback(w1, XmNhelpCallback, DmPopupWinHelpKeyCB,
		(XtPointer)(wip->hip));

	DmISRegisterFocusEH(wip);
	MapGizmo(PopupGizmoClass, wip->popupGizmo);

#ifdef DEBUG
	XtAddEventHandler(wip->w[W_Shell], (EventMask) 0, True,
		_XEditResCheckMessages, NULL);
#endif

	XmProcessTraversal(wip->w[W_ClassName], XmTRAVERSE_CURRENT);
	return(wip->w[W_Shell]);

} /* end of DmISCreateFolderWin */

/****************************procedure*header*****************************
 * DmISCreateAppWin - Creates a New or Properties window for an EXEC type
 * class.
 */
Widget
DmISCreateAppWin(DmObjectPtr op, WinInfoPtr wip, Boolean *bad_icon_file)
{
	int    n;
	Widget w1, w2;
	char   buf[PATH_MAX];

	DmISInitClassWin(op, wip, AppType);

	if (!op && NewAppShell) {
		RefreshNewWin(wip);
		return(NewAppShell);
	}
	lpatternLabel.label  = TXT_LINKED_TO_PROGRAM;
	lfilePathLabel.label = TXT_LINKED_TO_PROGRAM_IN;
	appCategoryMenu.clientData = (XtPointer)wip;

	if (op) {
		DmFnameKeyPtr fnkp = (DmFnameKeyPtr)(op->fcp->key);

		if ((fnkp->attrs & DM_B_DONT_CHANGE) ||
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES &&
				!PRIVILEGED_USER()))
		{
			/* make OK and Apply buttons insensitive */
			propMenuItems[0].sensitive = (XtArgVal)False;
			propMenuItems[1].sensitive = (XtArgVal)False;
			msgGizmo.rightMsgText      = TXT_READ_ONLY;
		} else {
			/* make OK and Apply buttons sensitive */
			propMenuItems[0].sensitive = (XtArgVal)True;
			propMenuItems[1].sensitive = (XtArgVal)True;
			msgGizmo.rightMsgText      = "";
		}
		propMenu.clientData = (XtPointer)wip;
	} else {
		msgGizmo.rightMsgText = "";
		addMenu.clientData    = (XtPointer)wip;
	}
	iconMenu.clientData        = (XtPointer)wip;
	loadMultiMenu.clientData   = (XtPointer)wip;
	loadMultiMenu.items[0].set = OLD_VAL(load_multi) ? True : False;

	appProgTypeMenu.items[0].clientData = (XtPointer)wip;
	appProgTypeMenu.items[1].clientData = (XtPointer)wip;

	if (num_app_ptypes > 2) {
		MenuInfoPtr mip = (MenuInfoPtr)MALLOC((num_app_ptypes-2) *
			sizeof(MenuInfo));
		wip->mip = mip;

		/* Note that app_tmpl_fnkp[0] and app_tmpl_fnkp[1] are
		 * not being used.
		 */
		for (n = 2; n < num_app_ptypes; ++n, ++mip) {
			mip->fnkp = app_tmpl_fnkp[n];
			mip->wip  = wip;
			appProgTypeMenu.items[n].clientData = (XtPointer)mip;
		}
	}

	wip->popupGizmo = CreateGizmo(base_fwp->shell, PopupGizmoClass,
		(op ? &appPropWinPopup : &appWinPopup), NULL, 0);

	DmISGetIDs(wip, AppType);
	UpdateDropSites(wip, BasicOptions);

	if (DmISShowIcon(wip) == -1)
		*bad_icon_file = True;

	SET_INPUT_GIZMO_TEXT(wip->g[G_ClassName], OLD_VAL(class_name));
	SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile],  OLD_VAL(icon_file));
	SET_INPUT_GIZMO_TEXT(wip->g[G_Pattern],   OLD_VAL(pattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LPattern],  OLD_VAL(lpattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_FilePath],  OLD_VAL(filepath));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LFilePath], OLD_VAL(lfilepath));
	SetTextGizmoText(wip->g[G_Open],       OLD_VAL(open_cmd));
	SetTextGizmoText(wip->g[G_Drop],       OLD_VAL(drop_cmd));


	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"classNameLabel");
	w1 = XtParent(w1);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
		GetGizmoWidget, "iconForm");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,     w1); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,     10); n++;
        XtSetValues(w2, Dm__arg, n);


	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconPixmapLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconFileRC");

        n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNleftWidget,      w1); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNnumColumns,      1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,     XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,         XmPACK_COLUMN); n++;
        XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
		GetGizmoWidget, "programNameLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,     2); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,    2); n++;
        XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
		GetGizmoWidget, "appProgTypeLabel");
	w2 = XtParent(w2);

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,        w1); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,       2); n++;
        XtSetArg(Dm__arg[n], XmNbottomOffset,     2); n++;
        XtSetValues(w2, Dm__arg, n);

	/* register icon stub as a drop site */
        XtSetArg(Dm__arg[0], XmNuserData, (XtPointer)wip);
	XtSetValues(wip->w[W_IconStub], Dm__arg, 1);

	n = 0;
        XtSetArg(Dm__arg[n], XmNimportTargets,      targets); n++;
        XtSetArg(Dm__arg[n], XmNnumImportTargets,   XtNumber(targets)); n++;
        XtSetArg(Dm__arg[n], XmNdropSiteOperations, XmDROP_COPY); n++;
        XtSetArg(Dm__arg[n], XmNdropProc,           DmISIconStubDropCB); n++;
	XmDropSiteRegister(wip->w[W_IconStub], Dm__arg, n);


	/* file typing page */
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileRCGizmo5");
        n = 0;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"constraintsLabel");
	w1 = XtParent(w1);

        n = 0;
        XtSetArg(Dm__arg[n], XmNrightAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNbottomAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,    XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,   XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopOffset,        2); n++;
        XtSetArg(Dm__arg[n], XmNrightOffset,      2); n++;
        XtSetArg(Dm__arg[n], XmNleftOffset,       2); n++;
        XtSetArg(Dm__arg[n], XmNbottomOffset,     2); n++;
        XtSetValues(w1, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"fileTypingFrame");

	/* unmanage the File Typing page */
	XtUnmanageChild(w1);

	/* end of file typing page */

	/* start of Icon Actions page */
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"iconActionsLabel");

        n = 0;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(w1, Dm__arg, n);

	w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"appRCGizmo1");

        n = 0;
        XtSetArg(Dm__arg[n], XmNnumColumns,      1); n++;
        XtSetArg(Dm__arg[n], XmNorientation,     XmVERTICAL); n++;
        XtSetArg(Dm__arg[n], XmNpacking,         XmPACK_TIGHT); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,   XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,       w1); n++;
        XtSetArg(Dm__arg[n], XmNleftAttachment,  XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetValues(w2, Dm__arg, n);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"loadMultiMenu");

	n = 0;
        XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(Dm__arg[n], XmNtopAttachment,  XmATTACH_WIDGET); n++;
        XtSetArg(Dm__arg[n], XmNtopWidget,      w2); n++;
	XtSetValues(w1, Dm__arg, n);

	/* unmanage the Icon Actions page */
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"appIconActionsFrame");
	XtUnmanageChild(w1);

	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"wbFrameGizmo");
	XtUnmanageChild(w1);

	/* end of Icon Actions page */

	if (op) {
		sprintf(buf, "%s: %s %s", GGT(TXT_IB_TITLE), OLD_VAL(label) ?
			OLD_VAL(label) : OLD_VAL(class_name),
			GGT(TXT_PROPERTIES));
		op->objectdata = (void *)wip;
	}

	/* update Program Type menu */
	DmISResetProgTypeMenu(wip, "appProgTypeMenu", OLD_VAL(prog_type),
		num_app_ptypes);

	/* update Move to Wastebasket menu */
	DmISResetYesNoMenu(wip, OLD_VAL(to_wb));

	XtSetArg(Dm__arg[0], XtNtitle, (op ? buf :
		(base_fwp->attrs & DM_B_SYSTEM_CLASSES)?
		GGT(TXT_NEW_SYSTEM_APP) : GGT(TXT_NEW_PERSONAL_APP)));
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	XtAddCallback(wip->w[W_Shell], XtNpopdownCallback,
		op ? DestroyPropWin : PopdownNewWin, (XtPointer)wip);

	XmAddWMProtocolCallback(wip->w[W_Shell], 
		XA_WM_DELETE_WINDOW(XtDisplay(wip->w[W_Shell])), 
		NewWinPopdownCB, (XtPointer)wip);

	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
	XtSetValues(wip->w[W_Shell], Dm__arg, 1);

	/* register callback for context-sensitive help */
	wip->hip          = (DmHelpInfoPtr)MALLOC(sizeof(DmHelpInfoRec));
	wip->hip->app_id  = ICON_SETUP_HELP_ID(Desktop);
	wip->hip->file    = STRDUP(ICON_SETUP_HELPFILE);
	wip->hip->section = op ? STRDUP(ICON_SETUP_CHANGE_CLASS_SECT) :
		STRDUP(ICON_SETUP_NEW_CLASS_SECT);
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		NULL);
	XtAddCallback(w1, XmNhelpCallback, DmPopupWinHelpKeyCB,
		(XtPointer)(wip->hip));

	DmISRegisterFocusEH(wip);
	MapGizmo(PopupGizmoClass, wip->popupGizmo);

#ifdef DEBUG
	XtAddEventHandler(wip->w[W_Shell], (EventMask) 0, True,
		_XEditResCheckMessages, NULL);
#endif

	/* move focus to Category menu */
	w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"appCategoryMenu:0");
	XmProcessTraversal(w1, XmTRAVERSE_CURRENT);

	return(wip->w[W_Shell]);

} /* end of DmISCreateAppWin */

/****************************procedure*header*****************************
 * RefreshNewWin - Reinitialize settings in a New window to default settings.
 * Called when the user selects Apply in a New window.
 */
static void
RefreshNewWin(WinInfoPtr wip)
{
	Widget w1, w2;

	/* clear footer */
	SetPopupWindowLeftMsg(wip->popupGizmo, " ");
	SetPopupWindowRightMsg(wip->popupGizmo, " ");

	SetInputGizmoText(wip->g[G_ClassName], "");
	SetInputGizmoText(wip->g[G_Pattern],   "");
	SetInputGizmoText(wip->g[G_LPattern],  "");
	SetInputGizmoText(wip->g[G_FilePath],  "");
	SetInputGizmoText(wip->g[G_LFilePath], "");
	SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile], OLD_VAL(icon_file));
	SetTextGizmoText(wip->g[G_Open], OLD_VAL(open_cmd));

	if (wip->class_type == FileType)
	{
		OLD_VAL(in_new)    = 0;
		OLD_VAL(prog_type) = 0;

		/* Get menu widget id */
		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu");
		/* Get button widget id */
		w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "fileCategoryMenu:0");
		XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
		XtSetValues(w1, Dm__arg, 1);

		ChangePage(wip, BasicOptions);

		DmISResetProgTypeMenu(wip, "fileProgTypeMenu", 0,
			num_file_ptypes);

		DmISResetYesNoMenu(wip, 0);

		SetInputGizmoText(wip->g[G_ProgToRun], "");
		SetInputGizmoText(wip->g[G_TmplName], "");
		SetTextGizmoText(wip->g[G_Print], OLD_VAL(print_cmd));
		XmListDeleteAllItems(wip->w[W_List]);

		(void)DmISUpdateIconStub(dflt_dflticonfile[FileType],
			wip->w[W_IconStub], base_fwp, 
			&wip->p[P_IconStub], True);

		XtSetArg(Dm__arg[0], XtNtitle,
			(base_fwp->attrs &DM_B_SYSTEM_CLASSES) ?
			GGT(TXT_NEW_SYSTEM_FILE) : GGT(TXT_NEW_PERSONAL_FILE));
		XtSetValues(NewFileShell, Dm__arg, 1);
	} else if (wip->class_type == AppType) {
		OLD_VAL(to_wb)     = 1;
		OLD_VAL(prog_type) = 0;

		/* Get menu widget id */
		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appCategoryMenu");
		/* Get button widget id */
		w2 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "appCategoryMenu:0");
		XtSetArg(Dm__arg[0], XmNmenuHistory, w2);
		XtSetValues(w1, Dm__arg, 1);

		ChangePage(wip, BasicOptions);

		DmISResetProgTypeMenu(wip, "appProgTypeMenu", 0,
			num_app_ptypes);

		/* update Move to Wastebasket menu */
		DmISResetYesNoMenu(wip, 1);

		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "loadMultiMenu:0");
		XtSetArg(Dm__arg[0], XmNset, False);
		XtSetValues(w1, Dm__arg, 1);

		SetTextGizmoText(wip->g[G_Drop], OLD_VAL(drop_cmd));
		(void)DmISUpdateIconStub(dflt_dflticonfile[AppType],
			wip->w[W_IconStub], base_fwp, 
			&wip->p[P_IconStub], True);

		XtSetArg(Dm__arg[0], XtNtitle,
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES) ?
			GGT(TXT_NEW_SYSTEM_APP) : GGT(TXT_NEW_PERSONAL_APP));
		XtSetValues(NewAppShell, Dm__arg, 1);
	} else { /* DirType */
		(void)DmISUpdateIconStub(dflt_dflticonfile[DirType],
			wip->w[W_IconStub], base_fwp, 
			&wip->p[P_IconStub], True);
		XtSetArg(Dm__arg[0], XtNtitle,
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES) ?
			GGT(TXT_NEW_SYSTEM_FOLDER) :
			GGT(TXT_NEW_PERSONAL_FOLDER));
		XtSetValues(NewFolderShell, Dm__arg, 1);
	}
	XtPopup(wip->w[W_Shell], XtGrabNone);

	if (wip->class_type == FileType || wip->class_type == AppType)
		XmProcessTraversal(w2, XmTRAVERSE_CURRENT);
	else
		XmProcessTraversal(wip->w[W_ClassName], XmTRAVERSE_CURRENT);

} /* end of RefreshNewWin */

/****************************procedure*header*****************************
 * IconSelectCB - selectProc for icon library's flat icon box to set the
 * icon preview in the icon library to the current selection.
 */
static void
IconSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip         = (WinInfoPtr)client_data;
	ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;
	DmObjectPtr op         = ITEM_OBJ(ITEM_CD(d->item_data));
	Widget stub            = QueryGizmo(PopupGizmoClass,
		wip->iconLibraryGizmo, GetGizmoWidget, "iconPixmapGizmo");

	DmISCreatePixmapFromBitmap(stub, &wip->p[P_IconPixmap], op->fcp->glyph);

	/* update current selection */
	XtSetArg(Dm__arg[0], XmNbackgroundPixmap, wip->p[P_IconPixmap]);
	XtSetArg(Dm__arg[1], XmNwidth,            op->fcp->glyph->width+4);
	XtSetArg(Dm__arg[2], XmNheight,           op->fcp->glyph->height+4);
	XtSetValues(stub, Dm__arg, 3);

} /* end of IconSelectCB */

/****************************procedure*header*****************************
 * NewWinPopdownCB - Called when a WM_DELETE_WINDOW message is received to
 * pop down a New window.  Note that PopdownNewWin() will be invoked when
 * the window is popped down.
 */
static void
NewWinPopdownCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip = (WinInfoPtr)client_data;
	
	BringDownPopup(GetPopupGizmoShell(wip->popupGizmo));

} /* end of NewWinPopdownCB */

/****************************procedure*header*****************************
 * PopdownNewWin - popdownCallback for a New window.  Frees settings,
 * destroy icon library window and re-activate menu button in Icon
 * Class:New menu.
 */
static void
PopdownNewWin(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget btn;
	WinInfoPtr wip = (WinInfoPtr)client_data;

	if (wip->iconLibraryGizmo) {
		/* Icon Library will be destroyed in popdownCallback */
		XtPopdown(wip->lib_fwp->shell);
	}

	if (wip->tmplFileGizmo) {
		/* Template path finder will be destroyed in popdownCallback */
		XtPopdown(GetFileGizmoShell(wip->tmplFileGizmo));
	}
	DmISFreeSettings(wip, New);
	DmISFreeSettings(wip, Old);

	/* make menu button sensitive again */
	sprintf(Dm__buffer, "NewMenu:%d", wip->class_type == FileType ?
		0 : wip->class_type == DirType ? 1 : 2);
	XtSetArg(Dm__arg[0], XmNsensitive, True);
	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
			GetGizmoWidget, Dm__buffer);
	XtSetValues(btn, Dm__arg, 1);

} /* end of PopdownNewWin */

/****************************procedure*header*****************************
 * DmISIconPathMenuCB - Callback for System and Other icon library menu.
 * Unmanages Update Listing and Find buttons when System is selected
 * and manages it when Other is selected.
 */
void
DmISIconPathMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget wid;
	Widget btn;
	Boolean set;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);
	Widget w1;
	Widget w2;

	btn = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "iconPathMenu:0");

	XtSetArg(Dm__arg[0], XmNset, &set);
	XtGetValues(btn, Dm__arg, 1);

	w1 = XtParent(QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "otherIconPathLabel"));
	w2 = XtParent(QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "libraryIconStubLabel"));
	wid = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
		GetGizmoWidget, "otherIconPathRC");
	XtSetArg(Dm__arg[0], XmNtopAttachment, XmATTACH_WIDGET);

	if (set) { /* System Library */
		/* unmanage other library path controls */
		XtUnmanageChild(wid);
		XtSetArg(Dm__arg[1], XmNtopWidget, w1);

		DmISSwitchIconLibrary(wip, systemLibraryPath);
	} else { /* other path */
		XtManageChild(wid);
		XtSetArg(Dm__arg[1], XmNtopWidget, wid);
	}
	XtSetValues(w2, Dm__arg, 2);

} /* end of DmISIconPathMenuCB */

/****************************procedure*header*****************************
 * DmISSwitchIconLibrary - Creates icon library window that displays icons in
 * specified path. 
 */
void
DmISSwitchIconLibrary(WinInfoPtr wip, char *path)
{
	Widget    w;
	XmString  str;
	DmItemPtr ip;
	char      buf[PATH_MAX];
	char      *rpath = realpath(path, buf);

	if (rpath == NULL) {
		DmVaDisplayStatus((DmWinPtr)wip->lib_fwp, True,
			TXT_BAD_ICON_LIBRARY_PATH, path);
		return;
	}

	/* Must compare with realpath of user-specified path because cp->path
	 * is a resolved path.
	 */
	if (strcmp(wip->lib_fwp->views[0].cp->path, rpath) == 0)
	{
		return;
	}
	if (wip->lib_fwp->views[0].cp->count != 0)
		DmCloseContainer(wip->lib_fwp->views[0].cp, DM_B_NO_FLUSH);
	else {
		/* Free the objects and container.  The item list be realloced
		 * in DmResetIconContainer().
		 */
		DmISFreeContainer(wip->lib_fwp->views[0].cp);
	}

	if (!strcmp(rpath, systemLibraryPath)) {
		BUSY_DESKTOP(wip->lib_fwp->shell);
		wip->lib_fwp->views[0].cp = CreateSystemIconLibrary();
	} else {
		/* don't want to grab pointer, library may be on
		 * a netware server that requires authentication
		 */
		DmBusyWindow(wip->lib_fwp->shell, True);
		if (!(wip->lib_fwp->views[0].cp = DmOpenDir(rpath,
			DM_B_TIME_STAMP | DM_B_INIT_FILEINFO | DM_B_SET_TYPE)))
		{
#ifdef DEBUG
			fprintf(stderr, "DmISSwitchIconLibrary(): Failed to \"
				"open %s\n", path);
#endif
			DmVaDisplayStatus((DmWinPtr)wip->lib_fwp, True,
				TXT_BAD_ICON_LIBRARY_PATH, path);
			DmBusyWindow(wip->lib_fwp->shell, False);
			return;
		}
	}
	DmResetIconContainer(wip->lib_fwp->views[0].box, DM_B_CALC_SIZE,
		      wip->lib_fwp->views[0].cp->op,
		      wip->lib_fwp->views[0].cp->num_objs,
		      &(wip->lib_fwp->views[0].itp),
		      &(wip->lib_fwp->views[0].nitems),
		      0);

	DmSortItems(wip->lib_fwp, DM_BY_NAME, DM_B_CALC_SIZE, 0, 0);

	/* update label */
	w = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
			GetGizmoWidget, "iconLibrarySWLabel");

	sprintf(Dm__buffer, "%s %s", GGT(TXT_ICONS_FOUND_IN), path);
	str = XmStringCreateLocalized(Dm__buffer);
	XtSetArg(Dm__arg[0], XmNlabelString, str);
	XtSetValues(w, Dm__arg, 1);
	XmStringFree(str);

	if (strcmp(rpath, systemLibraryPath))
		DmBusyWindow(wip->lib_fwp->shell, False);

} /* end of DmISSwitchIconLibrary */

/****************************procedure*header*****************************
 * DmISProgTypeMenuCB - Callback for Program Types menu.  If icon actions
 * are default settings, update them to defaults for new program type.
 */
void
DmISProgTypeMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int        i;
	int        num_btns;
	int        prev_type;
	char       buf[32];
	char       *menu_name;
	Widget     btn;
	WinInfoPtr wip;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

	switch(cbs->index) {
	case 0:
	case 1:
	{
		wip = (WinInfoPtr)(cbs->clientData);
		prev_type = NEW_VAL(prog_type);
		NEW_VAL(prog_type) = cbs->index;

		if (!CustomizedOpenCmd(wip, prev_type, NEW_VAL(prog_type)))
		{
			XtFree(NEW_VAL(open_cmd));
			NEW_VAL(open_cmd) = STRDUP(
			  dflt_open_cmd[wip->class_type][NEW_VAL(prog_type)]);
			SetTextGizmoText(wip->g[G_Open], NEW_VAL(open_cmd));
		}
		if (!CustomizedIconFile(wip, prev_type, NEW_VAL(prog_type)))
		{
			XtFree(NEW_VAL(icon_file));
			NEW_VAL(icon_file)
				= STRDUP(dflt_iconfile[wip->class_type]);
			SetInputGizmoText(wip->g[G_IconFile],
				NEW_VAL(icon_file));

			XtFree(NEW_VAL(dflt_icon_file));
			NEW_VAL(dflt_icon_file) =
				STRDUP(dflt_dflticonfile[wip->class_type]);
			if (VAR_ICON_FILE(NEW_VAL(icon_file)))
			  (void)DmISUpdateIconStub(NEW_VAL(dflt_icon_file),
				wip->w[W_IconStub], base_fwp, 
				&wip->p[P_IconStub], True);
			else
			  (void)DmISUpdateIconStub(NEW_VAL(icon_file),
				wip->w[W_IconStub], base_fwp, 
				&wip->p[P_IconStub], True);
		}
		if (wip->class_type == AppType) {
			if (!CustomizedDropCmd(wip, prev_type,
			  NEW_VAL(prog_type)))
			{
				XtFree(NEW_VAL(drop_cmd));
				NEW_VAL(drop_cmd) = STRDUP(
					dflt_drop_cmd[NEW_VAL(load_multi)]
						[cbs->index]);
				SetTextGizmoText(wip->g[G_Drop],
					NEW_VAL(drop_cmd));
			}
		} else {
			if (!CustomizedPrintCmd(wip, prev_type,
			  NEW_VAL(prog_type)))
			{
				XtFree(NEW_VAL(print_cmd));
				NEW_VAL(print_cmd) = STRDUP(dflt_print_cmd);
				SetTextGizmoText(wip->g[G_Print],
					NEW_VAL(print_cmd));
			}
		}
	}
		break;
	default:
	{
		char          *p;
		MenuInfoPtr   mip  = (MenuInfoPtr)(cbs->clientData);
		DmFnameKeyPtr fnkp = mip->fnkp;

		wip                = mip->wip;
		prev_type          = NEW_VAL(prog_type);
		NEW_VAL(prog_type) = cbs->index;

		if (!CustomizedOpenCmd(wip, prev_type, NEW_VAL(prog_type)))
		{
			p = DtGetProperty(&(fnkp->fcp->plist), "_Open", NULL);
			SetTextGizmoText(wip->g[G_Open], p ? p : "");
			XtFree(NEW_VAL(open_cmd));
			NEW_VAL(open_cmd) = p ? STRDUP(p) : NULL;
		}
		if (wip->class_type == AppType) {
			if (!CustomizedDropCmd(wip, prev_type,
				NEW_VAL(prog_type)))
			{
				p = DtGetProperty(&(fnkp->fcp->plist), "_DROP",
					NULL);
				SetTextGizmoText(wip->g[G_Drop], p ? p : "");
				XtFree(NEW_VAL(drop_cmd));
				NEW_VAL(drop_cmd) = p ? STRDUP(p) : NULL;
			}
		} else if (wip->class_type == FileType) {
			if (!CustomizedPrintCmd(wip, prev_type,
				NEW_VAL(prog_type)))
			{
				p = DtGetProperty(&(fnkp->fcp->plist),
					"_Print", NULL);
				SetTextGizmoText(wip->g[G_Print], p ? p : "");
				XtFree(NEW_VAL(print_cmd));
				NEW_VAL(print_cmd) = p ? STRDUP(p) : NULL;
			}
		}
		if (!CustomizedIconFile(wip, prev_type, NEW_VAL(prog_type)))
		{
			p = DtGetProperty(&(fnkp->fcp->plist), "_ICONFILE",
				NULL);
			SetInputGizmoText(wip->g[G_IconFile], p ? p : "");

			XtFree(NEW_VAL(icon_file));
			NEW_VAL(icon_file) = p ? STRDUP(p) : NULL;

			p = DtGetProperty(&(fnkp->fcp->plist), "_DFLTICONFILE",
				NULL);
			XtFree(NEW_VAL(dflt_icon_file));
			NEW_VAL(dflt_icon_file) = p ? STRDUP(p) : NULL;

			if (!NEW_VAL(icon_file))
				break;

			if (VAR_ICON_FILE(NEW_VAL(icon_file))) {
				char *icon_file = NEW_VAL(dflt_icon_file) ?
					NEW_VAL(dflt_icon_file) :
					dflt_dflticonfile[wip->class_type];

			  (void)DmISUpdateIconStub(icon_file,
				wip->w[W_IconStub], base_fwp, 
				&wip->p[P_IconStub], True);
			} else {
			  (void)DmISUpdateIconStub(NEW_VAL(icon_file),
				wip->w[W_IconStub], base_fwp, 
				&wip->p[P_IconStub], True);
			}
		}
	}
		break;
	}
	/* update menu button sensitivity - deactivate current selection */
	menu_name = (wip->class_type == FileType ? "fileProgTypeMenu" :
		"appProgTypeMenu");
	num_btns = (wip->class_type == FileType ? num_file_ptypes :
		num_app_ptypes);

	for (i = 0; i < num_btns; i++) {
		sprintf(buf, "%s:%d", menu_name, i);
		btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, buf);
		XtSetArg(Dm__arg[0], XmNsensitive, i == cbs->index ?
			False : True);
		XtSetValues(btn, Dm__arg, 1);
	}

} /* end of DmISProgTypeMenuCB */

/****************************procedure*header*****************************
 * IconLibPopdownCB - Called when a WM_DELETE_WINDOW message is received
 * by the icon library window to pop down then window.  Note that 
 * DmISDestroyIconLibrary() is called when it is popped down.
 */
static void
IconLibPopdownCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip = (WinInfoPtr)client_data;
	
	BringDownPopup(wip->lib_fwp->shell);

} /* end of IconLibPopdownCB */

/****************************procedure*header*****************************
 * DmISDestroyIconLibrary - popdownCallback for icon library window.  Frees
 * allocated resources before destroying the window.
 */
void
DmISDestroyIconLibrary(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip = (WinInfoPtr)client_data;

	if (wip->iconLibraryGizmo == NULL)
		return;

	if (wip->iconFileGizmo) {
		/* Icon File path finder will be destroyed in popdownCallback*/
		XtPopdown(GetFileGizmoShell(wip->iconFileGizmo));
	}

	if (wip->lib_fwp->views[0].cp->count != 0)
		DmCloseContainer(wip->lib_fwp->views[0].cp, DM_B_NO_FLUSH);
	else
		DmISFreeContainer(wip->lib_fwp->views[0].cp);

	FreeGizmo(PopupGizmoClass, wip->iconLibraryGizmo);
	wip->iconLibraryGizmo = NULL;
	DmDestroyIconContainer(wip->lib_fwp->shell, wip->lib_fwp->views[0].box,
		wip->lib_fwp->views[0].itp, wip->lib_fwp->views[0].nitems);
	XtDestroyWidget(wip->lib_fwp->shell);

	FREE(wip->lib_fwp);
	wip->lib_fwp = NULL;
	
} /* end of DmISDestroyIconLibrary */

/****************************procedure*header*****************************
 * CreateSystemIconLibrary - Creates objects for a container from a list of
 * predefined list of file names.  If that fails, creates objects from a
 * system icon library directory (/usr/X/lib/pixmaps, by default).
 */
static DmContainerPtr
CreateSystemIconLibrary()
{
	int ret;
	DmContainerPtr cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec));

	if (cp == NULL)
	{
#ifdef DEBUG
		fprintf(stderr, "CreateSystemIconLibrary(): Couldn't \"
			"allocate container for Icon Library\n");
#endif
		return(NULL);
	}
	cp->path = STRDUP(systemLibraryPath);

	ret = CreateObjsFromList(cp);

	if (ret == -1) { /* failure due to reasons other than ENOENT */
		FREE(cp->path);
		FREE(cp);
		return(NULL);
	} else if (ret == 1) {
		/* ICON_LIBRARY_FILE doesn't exist, try systemLibraryPath */
		FREE(cp->path);
		FREE(cp);
		if ((cp = DmOpenDir(systemLibraryPath, DM_B_SET_TYPE |
			DM_B_TIME_STAMP | DM_B_INIT_FILEINFO)) == NULL)
		{
#ifdef DEBUG
			fprintf(stderr,"CreateSystemIconLibrary(): Couldn't \"
				"open %s\n",systemLibraryPath);
#endif
			return(NULL);
		}
	} else
		/* use cp->count as a flag to determine whether to call
		 * DmCloseContainer() on the container in
		 * DmISSwitchIconLibrary().
		 */
		cp->count = 0;

	return(cp);

} /* end of CreateSystemIconLibrary */

/****************************procedure*header*****************************
 * CreateObjsFromList - Creates an object list from a list of file names.
 * Returns 0 on success, -1 or 1 on failure.
 */
static int
CreateObjsFromList(DmContainerPtr cp)
{
#define ICON_LIBRARY_FILE	".iconLibrary"

	FILE *fp;
	char buf[PATH_MAX];

	sprintf(Dm__buffer, "%s/%s", systemLibraryPath, ICON_LIBRARY_FILE);
	if ((fp = fopen(Dm__buffer, "r")) == NULL) {
		if (errno == ENOENT) {
#ifdef DEBUG
			fprintf(stderr, "CreateObjsFromList(): %s not found\n",
				Dm__buffer);
#endif
			return(1);
		} else {
#ifdef DEBUG
			perror("fopen");
#endif
			return(-1);
		}
	}
	while ((fgets(buf, PATH_MAX, fp)) != NULL)
	{
		(void)strtok(buf, "\n");
		if (Dm__CreateObj(cp, basename(buf), DM_B_INIT_FILEINFO
			| DM_B_SET_TYPE) == NULL)
		{
#ifdef DEBUG
			fprintf(stderr, "CreateObjsFromList(): Couldn't \"
				"create object for %s\n", buf);
#endif
			break;
		}
	}
	(void)fclose(fp);
	return(0);

#undef ICON_LIBRARY_FILE

} /* end of CreateObjsFromList */

/****************************procedure*header*****************************
 * DmISFreeContainer - This is called if the Icon Library was created
 * from ICON_LIBRARY_FILE to only free the objects and the container.
 * Also called from DmExitIconSetup().
 */
void
DmISFreeContainer(DmContainerPtr cp)
{
	DmObjectPtr op;
	DmObjectPtr save;

	for (op = cp->op; op;) {
		save = op->next;
		Dm__FreeObject(op);
		op = save;
	}
	if (cp->path)
		FREE(cp->path);
	FREE(cp);

} /* end of DmISFreeContainer */

/****************************procedure*header*****************************
 * DmISSelectCB - selectProc for flat icon box.  Called when a class is
 * selected.  Updates sensitivity of Delete and Properties buttons in Icon
 * Class menu.
 */
void
DmISSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *p;
	ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;
	DmObjectPtr op = ITEM_OBJ(base_fwp->views[0].itp
			+ d->item_data.item_index);
	DmFnameKeyPtr fnkp = (DmFnameKeyPtr)(op->fcp->key);

	UpdateClassMenu(fnkp);

	p = DmGetObjProperty(op, FILETYPE, NULL);

	/* If DATA or EXEC type class, display interesting info. */
	if (!strcmp(p, "EXEC")) {
		int i;
		char buf[PATH_MAX];
		char *p1 = DmGetObjProperty(op, PROG_TYPE, NULL);
		char *p2 = DmGetObjProperty(op, PATTERN, NULL);

		if (p2 == NULL)
			p2 = DmGetObjProperty(op, LPATTERN, NULL);

		if (!p1 || !p2) {
			SetBaseWindowLeftMsg(base_fwp->gizmo_shell, "");
			return;
		}
		for (i = 0; i < num_app_ptypes; i++) {
			if (!strcmp(p1, AppProgTypes[i]))
				break;
		}
		sprintf(buf, "%s %s      %s %s", GGT(TXT_PROGRAM), p2,
			GGT(TXT_TYPE), GGT(appProgTypeMenu.items[i].label));
		SetBaseWindowLeftMsg(base_fwp->gizmo_shell, buf);
	} else if (!strcmp(p, "DATA")) {
		char *p = DmGetObjProperty(op, PROG_TO_RUN, NULL);
		char buf[PATH_MAX];

		if (p == NULL) {
			SetBaseWindowLeftMsg(base_fwp->gizmo_shell, "");
			return;
		}
		sprintf(buf, GGT(TXT_FILE_OPEN_USING), op->name, p);
		SetBaseWindowLeftMsg(base_fwp->gizmo_shell, buf);
	} else
		SetBaseWindowLeftMsg(base_fwp->gizmo_shell, "");

} /* end of DmISSelectCB */

static void
StorePtype(DmFnameKeyPtr **tmpl_fnkp, int num, DmFnameKeyPtr ptype)
{
	*tmpl_fnkp = (DmFnameKeyPtr *)REALLOC(
		*tmpl_fnkp, num * sizeof(DmFnameKeyPtr *)
	);
	(*tmpl_fnkp)[num - 1] = ptype;
}
/****************************procedure*header*****************************
 * SetupProgTypeMenu - Determines what program types are available by scanning
 * the file class database for template classes and sets up the menu items
 * for the Program Types menu.
 */
static MenuItems *
SetupProgTypeMenu(int ftype)
{
	int       i;
	int       nitems;
	Boolean   new_type;
	char      *p1, *p2;
	MenuItems *menu_items;
	MenuItems *menu_item;
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

	nitems = XtNumber(progTypeMenuItems);
	menu_items = (MenuItems *)CALLOC((nitems + 1), sizeof(MenuItems));

	(void)memcpy(menu_items, progTypeMenuItems, sizeof(progTypeMenuItems));

	for (; fnkp; fnkp = fnkp->next)
	{
		if (!(fnkp->attrs & DM_B_TEMPLATE_CLASS))
			continue;

		if ((p1 = DtGetProperty(&(fnkp->fcp->plist), "_FILETYPE",
			NULL)) == NULL || strcmp(p1, TypeNames[ftype]))
			continue;

		if ((p1 = DtGetProperty(&(fnkp->fcp->plist),"_PROG_TYPE_LABEL",
			NULL)) == NULL)
			continue;

		if ((p2 = DtGetProperty(&(fnkp->fcp->plist), "_PROG_TYPE",
			NULL)) == NULL)
			continue;

		/* add program type to the list if it's not already there */
		new_type = True;
		if (ftype == FileType) {
			for (i = 0; i < num_file_ptypes; i++) {
				if (!strcmp(FileProgTypes[i], p2)) {
					new_type = False;
					break;
				}
			}
			if (new_type) {
				++num_file_ptypes;
				FileProgTypes =
					(char **)REALLOC((void *)FileProgTypes,
					num_file_ptypes * sizeof(char *));
				FileProgTypes[num_file_ptypes-1] = STRDUP(p2);
			}
		} else {
			for (i = 0; i < num_app_ptypes; i++) {
				if (!strcmp(AppProgTypes[i], p2)) {
					new_type = False;
					break;
				}
			}
			if (new_type) {
				++num_app_ptypes;
				AppProgTypes =
					(char **)REALLOC((void *)AppProgTypes,
					num_app_ptypes * sizeof(char *));
				AppProgTypes[num_app_ptypes-1] = STRDUP(p2);
			}
		}
		if (new_type) {
			++nitems;
			menu_items = (MenuItems *)REALLOC((void *)menu_items,
				(nitems + 1) * sizeof(MenuItems));

			menu_item = menu_items + (nitems - 1);
			(void)memset((void *)menu_item, 0, sizeof(MenuItems));

			menu_item->sensitive = True;
			menu_item->label     = p1;
			menu_item->mnemonic  = NULL;
			menu_item->type      = I_PUSH_BUTTON;
			menu_item->callback  = NULL;
			menu_item->set       = False;

			if (ftype == FileType)
				StorePtype(
					&file_tmpl_fnkp, num_file_ptypes, fnkp
				);
			else
				StorePtype(
					&app_tmpl_fnkp, num_app_ptypes, fnkp
				);
		}
	}
	menu_items[nitems].label = NULL;
	return(menu_items);

} /* end of SetupProgTypeMenu */

/****************************procedure*header*****************************
 * DmISGetProgTypes - Called at startup to determine what program types are
 * available on the system and set up the Program Types menu items for
 * a FILE and EXEC type New and Properties window.
 */
void
DmISGetProgTypes()
{
	FileProgTypes    = (char **)MALLOC(sizeof(char *) * 2);
	FileProgTypes[0] = strdup("UNIX Graphical");
	FileProgTypes[1] = strdup("UNIX Character");

	AppProgTypes    = (char **)MALLOC(sizeof(char *) * 2);
	AppProgTypes[0] = strdup("UNIX Graphical");
	AppProgTypes[1] = strdup("UNIX Character");

	num_file_ptypes = num_app_ptypes = 2;

	fileProgTypeMenu.items = SetupProgTypeMenu(FileType);
	appProgTypeMenu.items  = SetupProgTypeMenu(AppType);

} /* end of DmISGetProgTypes */


/****************************procedure*header*****************************
 * DmISShowIcon - Calls DmISUpdateIconStub() to display the image of the icon
 * file setting.  If the icon file is invalid or not found, use the default
 * icon file and set error flag to True.  This function is to enable the
 * modal to be mapped after the New/Properties window so that it is not
 * hidden by the latter.  Called from the routines that create the New and
 * Properties windows and ResetValues().
 */
int
DmISShowIcon(WinInfoPtr wip)
{
	Boolean error = False;

	if (VAR_ICON_FILE(OLD_VAL(icon_file)))
	{
		(void)DmISUpdateIconStub(OLD_VAL(dflt_icon_file),
			wip->w[W_IconStub], base_fwp, 
			&wip->p[P_IconStub], False);
		SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile], OLD_VAL(icon_file));
	} else {
		/* If icon is invalid or not found, use default icon */
		if (DmISUpdateIconStub(OLD_VAL(icon_file), wip->w[W_IconStub],
		  base_fwp, &wip->p[P_IconStub], False) == -1)
		{
			error = True;
			(void)DmISUpdateIconStub(OLD_VAL(dflt_icon_file),
				wip->w[W_IconStub], base_fwp,
				&wip->p[P_IconStub], False);

			/* change the new setting to the default icon file */
			XtFree(NEW_VAL(icon_file));
			NEW_VAL(icon_file) = STRDUP(OLD_VAL(dflt_icon_file));
			SetInputGizmoText(wip->g[G_IconFile],
				NEW_VAL(icon_file));
		} else
			SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile],
				OLD_VAL(icon_file));
	}
	return(error ? -1 : 0);

} /* end of DmISShowIcon */

/****************************procedure*header*****************************
 * LoadMultiFilesCB - Called when Load multiple files menu is selected.
 * Updates the Drop command depending on whether the option is selected
 * if the current program type is UNIX Graphical or UNIX Character and
 * the DROP command is not customized by the user.
 */
static void
LoadMultiFilesCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr               wip = (WinInfoPtr)(cbs->clientData);
	Widget                   btn = QueryGizmo(PopupGizmoClass,
					wip->popupGizmo, GetGizmoWidget,
					"loadMultiMenu:0");

	XtSetArg(Dm__arg[0], XmNset, &(NEW_VAL(load_multi)));
	XtGetValues(btn, Dm__arg, 1);

	if (NEW_VAL(prog_type) == 0 || NEW_VAL(prog_type) == 1)
	{
		char *p = GetTextGizmoText(wip->g[G_Drop]);

		if (!VALUE(p)) {
			XtFree(p);
			return;
		}
		if (!strcmp(dflt_drop_cmd[NEW_VAL(load_multi) ?
			0 : 1][NEW_VAL(prog_type)], p))
		{
			XtFree(NEW_VAL(drop_cmd));
			NEW_VAL(drop_cmd) = STRDUP(
				dflt_drop_cmd[NEW_VAL(load_multi)]
					[NEW_VAL(prog_type)]);
			SetTextGizmoText(wip->g[G_Drop], NEW_VAL(drop_cmd));
		}
		XtFree(p);
	}

} /* end of LoadMultiFilesCB */

/****************************procedure*header*****************************
 * TmplNameChangedCB - modifyVerifyCallback for Template Name text field
 * to activate the Add menu button.
 */
static void
TmplNameChangedCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget     btn;
	WinInfoPtr wip  = (WinInfoPtr)client_data;
	char       *val = GetInputGizmoText(wip->g[G_TmplName]);

	/* activate Add button in Template menu */
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListMenu:0");
	XtSetArg(Dm__arg[0], XmNsensitive, VALUE(val) ? True : False);
	XtSetValues(btn, Dm__arg, 1);
	XtFree(val);

} /* end of TmplNameChangedCB */

/****************************procedure*header*****************************
 * CustomizedOpenCmd - Returns True if the Open command has been customized
 * by the user; otherwise, returns False.  Checks current Open command with
 * the default Open command of the previous program type.
 */
static Boolean
CustomizedOpenCmd(WinInfoPtr wip, int prev_type, int cur_type)
{
	DmFnameKeyPtr fnkp;
	char          *p1, *p2;

	p1 = GetTextGizmoText(wip->g[G_Open]);
	if (prev_type == 0 || prev_type == 1) {
		if (VALUE(p1)) {
			if (strcmp(dflt_open_cmd[wip->class_type][prev_type],
				p1))
			{
				XtFree(p1);
				return(True);
			} else {
				XtFree(p1);
				return(False);
			}
		} else {
			XtFree(p1);
			return(False);
		}
	}
	/* prev_type can't be 0 or 1 at this point because file_tmpl_fnkp[0],
	 * file_tmpl_fnkp[1], app_tmpl_fnkp[0] and appl_tmpl_fnkp[1] are
	 * not set.
	 */
	fnkp = (wip->class_type == FileType ? file_tmpl_fnkp[prev_type] :
			app_tmpl_fnkp[prev_type]);

	/* get Open command from class property list */
	if (!(p2 = DtGetProperty(&(fnkp->fcp->plist), OPENCMD, NULL))) {
		if (!VALUE(p1)) {
			XtFree(p1);
			return(False);
		} else {
			XtFree(p1);
			return(True);
		}
	} else if (!VALUE(p1)) {
		XtFree(p1);
		return(True);
	}
	if (strcmp(p1, p2)) {
		XtFree(p1);
		return(True);
	} else {
		XtFree(p1);
		return(False);
	}

} /* end of CustomizedOpenCmd */

/****************************procedure*header*****************************
 * CustomizedDropCmd - Returns True if the DROP command has been customized
 * by the user; otherwise, returns False.  Checks current DROP command with
 * the default DROP command of the previous program type.
 */
static Boolean
CustomizedDropCmd(WinInfoPtr wip, int prev_type, int cur_type)
{
	DmFnameKeyPtr fnkp;
	char          *p1, *p2;

	p1 = GetTextGizmoText(wip->g[G_Drop]);
	if (prev_type == 0 || prev_type == 1) {
		if (VALUE(p1)) {
			if (strcmp(
				dflt_drop_cmd[NEW_VAL(load_multi)][prev_type],
				p1))
			{
				XtFree(p1);
				return(True);
			} else {
				XtFree(p1);
				return(False);
			}
		} else {
			XtFree(p1);
			return(False);
		}
	}
	/* prev_type can't be 0 or 1 at this point because file_tmpl_fnkp[0],
	 * file_tmpl_fnkp[1], app_tmpl_fnkp[0] and appl_tmpl_fnkp[1] are
	 * not set.
	 */
	fnkp = (wip->class_type == FileType ? file_tmpl_fnkp[prev_type] :
			app_tmpl_fnkp[prev_type]);

	/* get DROP command from class property list */
	if (!(p2 = DtGetProperty(&(fnkp->fcp->plist), DROPCMD, NULL))) {
		if (!VALUE(p1)) {
			XtFree(p1);
			return(False);
		} else {
			XtFree(p1);
			return(True);
		}
	} else if (!VALUE(p1)) {
		XtFree(p1);
		return(True);
	}
	if (strcmp(p1, p2)) {
		XtFree(p1);
		return(True);
	} else {
		XtFree(p1);
		return(False);
	}

} /* end of CustomizedDropCmd */

/****************************procedure*header*****************************
 * CustomizedPrintCmd - Returns True if the Print command has been customized
 * by the user; otherwise, returns False.  Checks current Print command with
 * the default Print command of the previous program type.
 */
static Boolean
CustomizedPrintCmd(WinInfoPtr wip, int prev_type, int cur_type)
{
	DmFnameKeyPtr fnkp;
	char          *p1, *p2;

	p1 = GetTextGizmoText(wip->g[G_Print]);
	if (prev_type == 0 || prev_type == 1) {
		if (VALUE(p1)) {
			if (strcmp(dflt_print_cmd, p1)) {
				XtFree(p1);
				return(True);
			} else {
				XtFree(p1);
				return(False);
			}
		} else {
			XtFree(p1);
			return(False);
		}
	}
	/* prev_type can't be 0 or 1 at this point because file_tmpl_fnkp[0],
	 * file_tmpl_fnkp[1], app_tmpl_fnkp[0] and appl_tmpl_fnkp[1] are
	 * not set.
	 */
	fnkp = (wip->class_type == FileType ? file_tmpl_fnkp[prev_type] :
			app_tmpl_fnkp[prev_type]);

	/* get Print command from class property list */
	if (!(p2 = DtGetProperty(&(fnkp->fcp->plist), PRINTCMD, NULL))) {
		if (!VALUE(p1)) {
			XtFree(p1);
			return(False);
		} else {
			XtFree(p1);
			return(True);
		}
	} else if (!VALUE(p1)) {
		XtFree(p1);
		return(True);
	}
	if (strcmp(p1, p2)) {
		XtFree(p1);
		return(True);
	} else {
		XtFree(p1);
		return(False);
	}

} /* end of CustomizedOpenCmd */

/****************************procedure*header*****************************
 * CustomizedIconFile - Returns True if the icon file has been customized
 * by the user; otherwise, returns False.  Checks current ICONFILE property
 * with the default ICONFILE setting of the previous program type.
 */
static Boolean
CustomizedIconFile(WinInfoPtr wip, int prev_type, int cur_type)
{
	DmFnameKeyPtr fnkp;
	char          *p1, *p2;

	p1 = GetInputGizmoText(wip->g[G_IconFile]);
	if (prev_type == 0 || prev_type == 1) {
		if (VALUE(p1)) {
			if (strcmp(dflt_iconfile[wip->class_type], p1)) {
				XtFree(p1);
				return(True);
			} else {
				XtFree(p1);
				return(False);
			}
		} else {
			XtFree(p1);
			return(False);
		}
	}
	/* prev_type can't be 0 or 1 at this point because file_tmpl_fnkp[0],
	 * file_tmpl_fnkp[1], app_tmpl_fnkp[0] and appl_tmpl_fnkp[1] are
	 * not set.
	 */
	fnkp = (wip->class_type == FileType ? file_tmpl_fnkp[prev_type] :
			app_tmpl_fnkp[prev_type]);

	/* get ICONFILE command from class property list */
	if (!(p2 = DtGetProperty(&(fnkp->fcp->plist), ICONFILE, NULL))) {
		if (!VALUE(p1)) {
			XtFree(p1);
			return(False);
		} else {
			XtFree(p1);
			return(True);
		}
	} else if (!VALUE(p1)) {
		XtFree(p1);
		return(True);
	}
	if (strcmp(p1, p2)) {
		XtFree(p1);
		return(True);
	} else {
		XtFree(p1);
		return(False);
	}

} /* end of CustomizedOpenCmd */

/****************************procedure*header*****************************
 * UpdateClassMenu - Updates the sensitivity of the Delete and Properties
 * buttons in the Icon Class menu when a class is selected in the main window.
 * The Delete button is made insensitive if it's not deletable.
 */
static void
UpdateClassMenu(DmFnameKeyPtr fnkp)
{
	Widget  btn;
	Boolean allowed = True;

	if (fnkp->attrs & DM_B_DONT_DELETE)
		allowed = False;
	else if (base_fwp->attrs & DM_B_SYSTEM_CLASSES)
		allowed = PRIVILEGED_USER();

	XtSetArg(Dm__arg[0], XmNsensitive, allowed);

	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:1");
	XtSetValues(btn, Dm__arg, 1);

	XtSetArg(Dm__arg[0], XmNsensitive, True);
	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:3");
	XtSetValues(btn, Dm__arg, 1);

} /* end of UpdateClassMenu */

