/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/remoteProp.h	1.28"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	remoteProp.h  Remote Folder Property Sheet header file
 */

extern NFSWindow *MainWindow;
extern int  GetWMPushpinState();
extern Boolean DeleteRemoteCB();
static void ExecuteCB();
static void SelectCB();
static void UnselectCB();
static void GetAvailableList();
static void remotePropCB();
static void getHostCB();
static void findFolderCB();
static void filePopupCB();
static void postVerifyCB();
static void extendedCB();
static void hideExtendedCB();
static void updateData();
static void resetFocus();
static void busyCursor();
static void standardCursor();
static Boolean verifyAllFields();

extern void setPreviousValue();
extern void setChoicePreviousValue();
extern void MoveFocus();
extern void setInitialValue();
extern struct vfstab *Copy_vfstab();
extern int  p3open();
extern int  p3close();

#define DEFAULT_CUSTOM_OPTIONS "intr,retry=3,rsize=1024,wsize=1024"
/* READONLY and READWRITE are defined differently in localAdv.c  */
#define READONLY	"ro,"
#define READWRITE	"rw,"
/* last option pair doesn't end with a comma;  */
#define HARDMOUNT	"hard"
#define SOFTMOUNT	"soft"        

#define COMMA ','
#define SPACE ' '
#define DFSHARES "/usr/sbin/dfshares"
#define DIR_ICON	"sdir.icon"
#define FILE_ICON	"sdatafile.icon"
#define EXECUTABLE_ICON	"sexec.icon"
#define UNKNOWN_ICON	"sblank.icon"

typedef struct _remoteSetting
{
    Setting folderLabel;
    Setting remotePath;
    Setting localPath;
    Setting writable;
    Setting mountType;
    Setting bootMount;
    Setting host;
    Setting extended;
    Setting customOptions;
    char *  selectedHost;
    int     currentItem;
    char *  mountArgs;
} remoteSettings;

static remoteSettings remoteSetting =
{
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { " ",  " ",  " ",  " " },
    { NULL, DEFAULT_CUSTOM_OPTIONS, NULL, NULL },
    NULL, 0, NULL 
};

/* menu button */

static InputGizmo remoteHostG =
{
    NULL,
    "Remoethost",  
    TXT_RemoteHost,
    "",
    &remoteSetting.host,
    HOST_FIELD_SIZE,
    (void (*)())GetAvailableList,
};


static MenuItems getHostItems[] = {{ True, TXT_LookupBtn, CHR_LookupBtn}, {0}};
static MenuGizmo getHostMenu = { &HostWindowHelp, "_X_", "_X_", getHostItems,
				    getHostCB };
static ChoiceGizmo getHostChoice = {&HostWindowHelp, "_X_", "",  &getHostMenu};

static GizmoRec hostGizmos[] =
{
   { InputGizmoClass, &remoteHostG },
   { ChoiceGizmoClass, &getHostChoice },
};

static LabelGizmo hostLabelG =
{ NULL, "_X_", NULL, hostGizmos, XtNumber(hostGizmos), OL_FIXEDROWS };

static StaticTextGizmo Heading =
{
    NULL,
    "heading",
    TXT_dfsharesHeading,
    WestGravity,
};

static MenuItems getListItems[] = {{ True, TXT_GetList, CHR_GetList }, {0}};
static MenuGizmo getListMenu = { &HostWindowHelp, "_X_", "_X_", getListItems,
				    GetAvailableList };
static ChoiceGizmo getListChoice = {&HostWindowHelp, "_X_", "",  &getListMenu};

static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};
static Setting AvailableListSetting =
{
	"AvailableList",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};


static ListGizmo AvailableFolders = {
	NULL, "dfsharesList", "", (Setting *)&AvailableListSetting,
	/*"%p %s  %s"*/ "%s", True, 4,
	"OlDefaultBoldFont",
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};

static GizmoRec ListGizmos[] =
{
   { ListGizmoClass, &AvailableFolders },
   { ChoiceGizmoClass, &getListChoice },
};

static LabelGizmo listLabelG =
{ NULL, "_X_", NULL, ListGizmos, XtNumber(ListGizmos), OL_FIXEDCOLS };

static InputGizmo folderLabelG =
{
    NULL,
    "folderLabel",
    TXT_Label,
    "",
    &remoteSetting.folderLabel,
    LABEL_FIELD_SIZE,
    (void (*)())NULL,
};

static InputGizmo remotePathG =
{
    NULL,
    "remotePath",
    TXT_RemotePath,
    "",
    &remoteSetting.remotePath,
    PATH_FIELD_SIZE,
    (void (*)())NULL,
};

static InputGizmo localPathG =
{
    NULL,
    "localPath",
    NULL,
    "",
    &remoteSetting.localPath,
    PATH_FIELD_SIZE,
    (void (*)())NULL,
};

static MenuItems findFolderItems[] = {{ True, TXT_FindFolder, CHR_FindFolder }, {0}};
static MenuGizmo findFolderMenu = { &FindFolderWindowHelp, "_X_", "_X_", findFolderItems,
				    findFolderCB };
static ChoiceGizmo findFolderChoice = {&FindFolderWindowHelp, "_X_", "",  &findFolderMenu};

static GizmoRec localPathGizmos[] =
{
  { InputGizmoClass,  &localPathG },
  { ChoiceGizmoClass, &findFolderChoice },
};

static LabelGizmo localPathLabel =
   { NULL, "localPathLabel", TXT_LocalPath, localPathGizmos,
	 XtNumber(localPathGizmos), OL_FIXEDROWS
   };

typedef enum _filePopupItemIndex
{ ffApply, ffCancel, ffHelp } filePopupItemIndex;
static MenuItems	filePopupMenuItem[] =
{
			{True, TXT_OK,     CHR_OK,     NULL, NULL, NULL},
			{True, TXT_Cancel, CHR_Cancel, NULL, NULL, NULL},
			{True, TXT_Help,   CHR_Help,   NULL, NULL, NULL},
			{NULL}
};
static MenuGizmo	filePopupMenu =
{
    NULL,"filePopup_menu","_X",filePopupMenuItem,filePopupCB,NULL
};
static FileGizmo	fileFolderPopup =
{
    NULL, "mountPoint", TXT_MountPointTitle, &filePopupMenu, NULL,
    NULL, NULL, FOLDERS_AND_FOLDERS, NULL
};

typedef enum _AccessItemIndex { ReadOnly, ReadWrite } AccessItemIndex;
static MenuItems AccessItems[] =
{
   { True, TXT_ReadOnly, CHR_ReadOnly, NULL, NULL, NULL, 1 },
   { True, TXT_ReadWrite, CHR_ReadWrite },
   { NULL }
};

static MenuGizmo AccessMenu = { NULL, "_X_", "_X_",
					  AccessItems, NULL, NULL, EXC };

static ChoiceGizmo AccessChoice = { NULL, "Access",
					  TXT_Writable,
					   &AccessMenu,
					   &remoteSetting.writable };

typedef enum _bootMountIndex { yes, no } bootMountIndex;
static MenuItems bootMountItems[] = {
        {True,		/* sensitive	*/
	TXT_Yes,	/* LABEL	*/
	CHR_Yes,	/* MNEMONIC	*/
	NULL,		/* NEXTTIER	*/
	NULL,		/* SELCB	*/
	0,		/* CLIENT_DATA	*/
	True},		/* SET	*/

        {True,		/* sensitive	*/
	TXT_No,		/* LABEL	*/
	CHR_No,		/* MNEMONIC	*/
	NULL,		/* NEXTTIER	*/
	NULL,		/* SELCB	*/
	0,		/* CLIENT_DATA	*/
	False},		/* SET	*/
        { 0 }
};

static MenuGizmo bootMountMenu = {
        NULL,		/*HELP		*/
        "bootMount",	/* name		*/
        "_X_",		/* title	*/
        bootMountItems,	/* Items	*/
        0,		/* Function	*/
        NULL,		/* Client data	*/
        EXC,		/* Button type	*/
        OL_FIXEDROWS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo bootMountChoice = {
        NULL,				/* HELP		*/
        "bootMountG",			/* NAME		*/
        TXT_bootMount,			/* CAPTION	*/
        &bootMountMenu,			/* MENU		*/
        &remoteSetting.bootMount	/* SETTINGS	*/
};
static MenuItems extendedItems[] =
{
   { True, "", CHR_Extend  },
   { 0 }
};
static MenuGizmo extendedMenu = { NULL, "extendedMenu", "_X_", extendedItems,
				      extendedCB, NULL, CNS,
				      OL_FIXEDROWS, 1, 0,
/*				      extendedCB
*/				      };
static ChoiceGizmo extendedChoice =
{ NULL, "extendedChoice", TXT_ExtendedOpts, &extendedMenu,
      &remoteSetting.extended };

typedef enum _mountTypeIndex {SoftMount, HardMount } mountTypeIndex;
static MenuItems  mountTypeItems[] =
   {
      {True, TXT_Soft,         CHR_Soft, NULL, NULL, NULL, 1 },
      {True, TXT_Hard,	       CHR_Hard, NULL, NULL, NULL, 0 },
      { 0 }
   };
static MenuGizmo       mountTypeMenu = { NULL, "_X_", "_X_",
					     mountTypeItems, NULL, NULL, EXC };

static ChoiceGizmo mountTypeChoice = { NULL, "mountType", TXT_MountType,
					   &mountTypeMenu,
					   &remoteSetting.mountType };
static InputGizmo customOptionsG =
{
    NULL,
    "customOptions",
    TXT_MountOptions,
    "",
    &remoteSetting.customOptions,
    OPTIONS_FIELD_SIZE,
    (void (*)())NULL,
};

static GizmoRec extendedGizmos[] =
{
   { ChoiceGizmoClass, &mountTypeChoice },
   { InputGizmoClass,  &customOptionsG  },
};

static LabelGizmo extendedLabelG =
   { NULL, "extendedLabel", NULL, extendedGizmos,
	 XtNumber(extendedGizmos), OL_FIXEDCOLS
   };

static GizmoRec Props[] =
{
/*   { InputGizmoClass,  &remoteHostG },
*/
   { LabelGizmoClass,  &hostLabelG   },
   { StaticTextGizmoClass,  &Heading }, 
   { LabelGizmoClass,  &listLabelG   },     
   { InputGizmoClass,  &folderLabelG },
   { InputGizmoClass,  &remotePathG },
   { LabelGizmoClass,  &localPathLabel },
   { ChoiceGizmoClass, &AccessChoice }, 
   { ChoiceGizmoClass, &bootMountChoice }, 
   { ChoiceGizmoClass, &extendedChoice }, 
   { LabelGizmoClass,  &extendedLabelG   },
/*   { ChoiceGizmoClass, &mountTypeChoice }, 
*/
};

static MenuItems  RemoteMenuItems[] =
   {
      {True, TXT_Add2,             CHR_Add2 },
      {True, TXT_Reset,		   CHR_Reset },
      {True, TXT_Cancel,           CHR_Cancel },
      {True, TXT_Help,             CHR_Help },
      { 0 }
   };
extern MenuGizmo RemoteMenu =
   { NULL, "_X_", "_X_", RemoteMenuItems, remotePropCB };

static PopupGizmo RemotePropertiesGizmo =
   { NULL, "prop", TXT_AddRemoteTitle, &RemoteMenu, Props, XtNumber(Props) };
