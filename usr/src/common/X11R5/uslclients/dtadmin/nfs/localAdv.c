/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/localAdv.c	1.32.1.7"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	localAdv.c    Advertise local folder dialog box
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Xol/ControlAre.h>
#include <Xol/Flat.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/PopupWindo.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/InputGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include "text.h"
#include "nfs.h"
#include "verify.h"
#include "notice.h"
#include "sharetab.h"
#include "local.h"
#include "utilities.h"
#include "client.h"
#include "help.h"

#define READONLY	"ro"
#define READWRITE	"rw"

static Boolean  PropertyPopupMode = False;
static Widget   PopupShell = NULL; /* shell of localProperiesPrompt */
static UserData user;
static Boolean verifyLocalAccess();
static void AccessExceptionCB();
static void ExecuteCB();
static void SelectCB();
static void UnselectCB();
static void localPropCB();
static void defaultAccessCB();
static void getHostCB();
static void parseLocalOptions();
static void insertException();
static void localPathCB();
static void setLabel();
static void findFolderCB();
static void filePopupCB();
static void postVerifyCB();
static void extendedCB();
static void hideExtendedCB();
static void resetFocus();

extern NFSWindow *MainWindow;
extern Boolean DeleteLocalCB();
extern int  GetWMPushpinState();

typedef struct _localSettings
{
    Setting folderLabel;
    Setting localPath;
    Setting defaultAccess;
    Setting bootAdv;
    Setting options;
    Setting host;
    Setting extended;
    Setting customOptions;
} localSettings;

static localSettings localSetting = 
{
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { " ",  " ",  " ",  " "  },
    { NULL, NULL, NULL, NULL }
};

static InputGizmo folderLabelG =
{
    &LocalFolderLabelHelp,
    "folderLabel",
    TXT_Label,
    "",
    &localSetting.folderLabel,
    LABEL_FIELD_SIZE,
    (void (*)())NULL,
};

static InputGizmo localPathG =
{
    &LocalLocalPathHelp,
    "localPathShare",
    NULL,
    "",
    &localSetting.localPath,
    PATH_FIELD_SIZE,
    localPathCB,
};
static MenuItems findFolderItems[] = {{ True, TXT_Find, CHR_Find }, {0}};
static MenuGizmo findFolderMenu = { &LocalFindHelp, "findFolderMenu", "",
					findFolderItems, findFolderCB };
static ChoiceGizmo findFolderChoice = {&LocalFindHelp, "findFolderChoice", "",
					   &findFolderMenu}; 

static GizmoRec localPathGizmos[] =
{
  { InputGizmoClass,  &localPathG },
  { ChoiceGizmoClass, &findFolderChoice },
};

static LabelGizmo localPathLabel =
   { &LocalLocalPathHelp, "localPathLabel", TXT_LocalPathShare, localPathGizmos,
	 XtNumber(localPathGizmos), OL_FIXEDROWS
   };

typedef enum _filePopupItemIndex
{ ffApply, ffCancel, ffHelp } filePopupItemIndex;
static MenuItems	filePopupMenuItem[] =
{
			{True, TXT_OK,  CHR_OK, NULL, NULL, NULL},
			{True, TXT_Cancel, CHR_Cancel, NULL, NULL, NULL},
			{True, TXT_Help,   CHR_Help, NULL, NULL, NULL},
			{NULL}
};
static MenuGizmo	filePopupMenu =
{
    &LocalFindMenuHelp,"filePopup_menu","_X",filePopupMenuItem,filePopupCB,NULL
};
static FileGizmo	fileFolderPopup =
{
    &LocalFindPopupHelp, "mountPoint", TXT_ShareItemTitle, &filePopupMenu, NULL,
    NULL, NULL, FOLDERS_AND_FILES, NULL
};

typedef enum _bootAdvIndex { yes, no } bootAdvIndex;
Setting bootAdv;
static MenuItems bootAdvItems[] = {
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

static MenuGizmo bootAdvMenu = {
        &BootAdvHelp,	/*HELP		*/
        "bootAdv",	/* name		*/
        "_X_",		/* title	*/
        bootAdvItems,	/* Items	*/
        0,		/* Function	*/
        NULL,		/* Client data	*/
        EXC,		/* Button type	*/
        OL_FIXEDROWS,	/* Layout type	*/
        1,		/* Measure	*/
        OL_NO_ITEM	/* Default item	*/
};

static ChoiceGizmo bootAdvChoice = {
        &BootAdvHelp,			/* HELP		*/
        "bootAdvG",			/* NAME		*/
        TXT_bootAdvertise,		/* CAPTION	*/
        &bootAdvMenu,			/* MENU		*/
        &localSetting.bootAdv		/* SETTINGS	*/
};

typedef enum _defaultAccessIndex { ro, rw, none } defaultAccessIndex;
static MenuItems defaultAccessItems[] =
{
   { True, TXT_ReadOnly,  CHR_ReadOnly, NULL, NULL, NULL, True  },
   { True, TXT_ReadWrite, CHR_ReadWrite },
   { True, TXT_NoAccess,  CHR_NoAccess  },
   { NULL }
};
static MenuGizmo defaultAccessMenu = 
{
    &DefaultAccessHelp, "defaultAccessMenu", "_X_", defaultAccessItems,
    defaultAccessCB, NULL, EXC 
};
static ChoiceGizmo defaultAccessChoice = 
{
    &DefaultAccessHelp, "defaultAccess", TXT_DefaultAccess,
    &defaultAccessMenu, &localSetting.defaultAccess 
};

static StaticTextGizmo exceptionsText =
{
    &ExceptionTextHelp,
    "exceptions",
    TXT_AccessExceptions,
    WestGravity,
};
static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};
static Setting ExceptionListSetting =
{
	"ExceptionList",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};


static ListGizmo ExceptionListG = 
{
    &ExceptionListHelp, "ExceptionList", "list Label",
    (Setting *)&ExceptionListSetting,
    "%s  %s", True, 4,
    NULL,
    (XtArgVal) False,		/* Don't copy this list */
    ExecuteCB, SelectCB, UnselectCB,
};

static InputGizmo hostG =
{
    &ExceptionHostHelp,
    "Localhost",
    TXT_Host,
    "",
    &localSetting.host,
    15,
    (void (*)())NULL,
};

static MenuItems getHostItems[] = {{ True, TXT_LookupBtn, CHR_LookupBtn }, {0}};
static MenuGizmo getHostMenu = { &LocalGetHostHelp, "_X_", "_X_", getHostItems,
				    getHostCB };
static ChoiceGizmo getHostChoice = {&LocalGetHostHelp, "_X_", "",  &getHostMenu};

static GizmoRec hostGizmos[] =
{
  { InputGizmoClass,  &hostG },
  { ChoiceGizmoClass, &getHostChoice },
};

static LabelGizmo hostLabel =
   { &ExceptionHostHelp, "hostLabel", "", hostGizmos,
	 XtNumber(hostGizmos), OL_FIXEDROWS
   };

  
typedef enum _ExceptionMenuIndex
{ ExceptionReadOnly, ExceptionReadWrite, ExceptionDelete, ExceptionDeleteAll }
ExceptionMenuIndex; 

static MenuItems ExceptionMenuItems[] =
{
   { False, TXT_InsertReadOnly,  CHR_InsertReadOnly },
   { True,  TXT_InsertReadWrite, CHR_InsertReadWrite },
   { False, TXT_Delete,          CHR_Delete2 },
   { False, TXT_DeleteAll,       CHR_DeleteAll },
   { NULL }
};
static MenuGizmo ExceptionMenu =
   { &ExceptionMenuHelp, "exceptionMenu", "_1_", ExceptionMenuItems,
	 AccessExceptionCB, NULL, CMD, OL_FIXEDCOLS };

static ChoiceGizmo ExceptionChoice = 
{ &ExceptionMenuHelp, "exceptionChoice", "", &ExceptionMenu };

static GizmoRec ExceptionGizmos1[] = 
{
   { ChoiceGizmoClass, &ExceptionChoice },
   { ListGizmoClass, &ExceptionListG },
};

static LabelGizmo exceptionLabel1 =
   { NULL, "exceptionLabel1", "", ExceptionGizmos1,
	 XtNumber(ExceptionGizmos1), OL_FIXEDROWS
   };

static SpaceGizmo vspace = {2, 2};

static GizmoRec ExceptionGizmos2[] = 
{
   { LabelGizmoClass, &hostLabel       },
   { SpaceGizmoClass, &vspace          },
   { LabelGizmoClass, &exceptionLabel1 },
};

static LabelGizmo exceptionLabel2 =
   { NULL, "exceptionLabel2", TXT_AccessExceptions, ExceptionGizmos2,
	 XtNumber(ExceptionGizmos2), OL_FIXEDCOLS
   };

static MenuItems extendedItems[] =
{
   { True, "", CHR_Extend  },
   { 0 }
};
static MenuGizmo extendedMenu = 
{ &LocalExtendedHelp, "extendedMenu", "_X_", extendedItems,
      extendedCB, NULL, CNS };

static ChoiceGizmo extendedChoice =
{ &LocalExtendedHelp, "extendedChoice", TXT_ExtendedOpts, &extendedMenu,
      &localSetting.extended };

static InputGizmo customOptionsG =
{
    &LocalCustomHelp,
    "customOptions",
    TXT_MountOptions,
    "",
    &localSetting.customOptions,
    OPTIONS_FIELD_SIZE,
    (void (*)())NULL,
};

static GizmoRec extendedGizmos[] =
{
   { InputGizmoClass,  &customOptionsG  },
};

static LabelGizmo extendedLabelG =
{ &LocalCustomHelp, "extendedLabel", NULL, extendedGizmos,
	 XtNumber(extendedGizmos), OL_FIXEDCOLS
};

static GizmoRec LocalGizmos[] =
{
   { LabelGizmoClass,  &localPathLabel    },
   { InputGizmoClass,  &folderLabelG  },
   { ChoiceGizmoClass, &bootAdvChoice },
   { ChoiceGizmoClass, &defaultAccessChoice },
/*   { StaticTextGizmoClass, &exceptionsText },
*/
   { LabelGizmoClass,  &exceptionLabel2 },
   { ChoiceGizmoClass, &extendedChoice }, 
   { LabelGizmoClass,  &extendedLabelG   },
};

static MenuItems  AdvMenuItems[] =
   {
      {True, TXT_OK,  CHR_OK },
      {True, TXT_Reset,  CHR_Reset },
      {True, TXT_Cancel, CHR_Cancel },
      {True, TXT_Help,	 CHR_Help },
      { 0 }
   };
extern MenuGizmo  AdvMenu = 
{ &AdvMenuHelp, "_X_", "_X_", AdvMenuItems, localPropCB };

static PopupGizmo LocalPropertiesGizmo =
   { &AdvMenuHelp, "adv", TXT_nfsLocalFolder, &AdvMenu, LocalGizmos,
	 XtNumber(LocalGizmos) }; 



static Boolean
bringDownPopups(Widget parent, Boolean force)
{
    extern NFSWindow *nfsw;
    Widget shell;
    long pushpin_state = WMPushpinIsOut;

    if (force == False)
	GetWMPushpinState(XtDisplay(parent), XtWindow(parent),
			  &pushpin_state); 

    switch (pushpin_state) 
    {
    case WMPushpinIsIn:
	return False;
	break;
    case WMPushpinIsOut:
    default:
	XtPopdown(parent);

	if ((shell = GetFileGizmoShell(&fileFolderPopup)) != NULL)
	{
	    SetWMPushpinState(XtDisplay(shell), XtWindow(shell),
			      WMPushpinIsOut); 
	    XtPopdown(shell);
	}
	/* FIX: hostPopup could belong to Local window */
	if ((shell = (nfsw-> hostPopup ?
	             GetPopupGizmoShell(nfsw-> hostPopup) :
		     NULL)) != NULL)
	{
	    SetWMPushpinState(XtDisplay(shell), XtWindow(shell),
			      WMPushpinIsOut); 
	    XtPopdown(shell);
	}
	return True;
	break;
    }
}



static void
setExceptionSensitivity(defaultAccessIndex index, Boolean doDelete)
{
    /* set the Exception Menu Item sensitivity based on the        */
    /* DefaultAccess that is selected.  */
    
    switch (index)
    {
    case ro:
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadWrite,
			  XtNsensitive, True, NULL);
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadOnly,
			  XtNsensitive, False, NULL);
	break;
    case rw:
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadWrite,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadOnly,
			  XtNsensitive, True, NULL);
	break;
    case none:
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadWrite,
			  XtNsensitive, True, NULL);
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadOnly,
			  XtNsensitive, True, NULL);
	break;
    default:
	DEBUG0("default in defaultAccessCB taken!!!\n");
	break;
    }
    if (doDelete)
    {
	ListHead *headPtr = (ListHead *)ExceptionListG.settings->current_value;

	if (headPtr-> size >0)
	{
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
			      XtNsensitive, True, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
			      XtNsensitive, True, NULL);
	}
	else
	{
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
			      XtNsensitive, False, NULL);
	}
    }
}

static void
createLocalPopup(Widget w)
{
    extern NFSWindow *nfsw;
    Arg               arg[4];

    nfsw->localPropertiesPrompt = &LocalPropertiesGizmo;
    
    PopupShell = CreateGizmo(w, PopupGizmoClass,
			nfsw->localPropertiesPrompt, NULL, 0); 
    XtAddCallback(PopupShell , XtNpopdownCallback, hideExtendedCB, NULL);
    OlVaFlatSetValues(AdvMenu.child, PropApply,
		      XtNsensitive, OwnerLocal, NULL);
    XtSetArg(arg[0], XtNnoneSet, True);
    XtSetValues(ExceptionListG.flatList, arg, 1);
    XtSetArg(arg[0], XtNmaximumSize, LABEL_FIELD_SIZE);
    XtSetValues(folderLabelG.textFieldWidget, arg, 1);
    SetValue(extendedChoice.buttonsWidget, XtNunselectProc, extendedCB,
	      (XtPointer)0);
    return;
}


/*
 * LocalPropertyCB
 *
 */
static char * shareOpts[] =
{
#define OPT_RO	0
		READONLY,
#define OPT_RW	1
		READWRITE,
		NULL
};

extern void
LocalPropertyCB(Widget w, Boolean displayPopup, XtPointer call_data)
{
    extern NFSWindow *nfsw;
    dfstab        * dfsp;
    struct share  * sharep;
    DmObjectPtr     op;
    ObjectListPtr   slp = nfsw-> localSelectList;
    ListHead      * headPtr;
    char          * options, *ptr, *value, *token, *host;
    char 	  *customOptions;
    defaultAccessIndex dAccess = none;

    if (displayPopup == False && PropertyPopupMode == False)
	return; /* don't switch to selected data if in add mode */

    RETURN_IF_NULL(slp, TXT_SelectFolderProp);
    if (displayPopup && slp-> next != NULL)
    {
	SetMessage(MainWindow ,TXT_Select1Folder, Base);
	return;
    }
    RETURN_IF_NULL((op = slp-> op), TXT_BadOp);
    RETURN_IF_NULL((dfsp = (dfstab *)op-> objectdata), TXT_BadVp);
    RETURN_IF_NULL((sharep = dfsp-> sharep), TXT_BadVp);
    RETURN_IF_NULL(sharep-> sh_path, TXT_BadVp);

    PropertyPopupMode = True;

    if (nfsw->localPropertiesPrompt == NULL)
    {
	if (displayPopup == False)
	    return;
	createLocalPopup(w);
        XtUnmanageChild(extendedLabelG.controlWidget);
    }

    SetValue(PopupShell, XtNtitle, GetGizmoText(TXT_PropertyLocalTitle)); 
    OlVaFlatSetValues(AdvMenu.child, PropApply,
		      XtNlabel, GetGizmoText(TXT_OK),
		      XtNmnemonic, *(GetGizmoText(CHR_OK)), NULL);
    
    /* fill in fields */

    /* set up list */

    headPtr = (ListHead *) (ExceptionListG.settings-> previous_value);
    if (headPtr != NULL)
	FreeList(headPtr);
    headPtr-> numFields = 2;
    headPtr-> size = 0;
    headPtr-> list = NULL;

    setPreviousValue(&localSetting.folderLabel, sharep-> sh_res);
    setPreviousValue(&localSetting.localPath, sharep-> sh_path);
    setPreviousValue(&localSetting.host, "");
    setPreviousValue(&localSetting.options, sharep-> sh_opts);
    if (user.prevVal != NULL)
	FREE(user.prevVal);
    user.prevVal = STRDUP(localSetting.host.previous_value);
    ptr = options = STRDUP(sharep-> sh_opts);
    if ((customOptions = MALLOC(strlen(options) + 1)) == NULL)
	NO_MEMORY_EXIT();
    customOptions[0] = customOptions[1] = EOS;
    while (*options)
    {
	switch (getsubopt(&options, shareOpts, &value))
	{
	case OPT_RO:
	    if (value)
	    {
		while ((host = strtok(value, ":")))
		{
		    value = NULL;
		    insertException(headPtr, host, ro);
		}
	    }
	    else
		dAccess = ro;
	    break;
	case OPT_RW:
	    if (value)
	    {
		while ((host = strtok(value, ":")))
		{
		    value = NULL;
		    insertException(headPtr, host, rw);
		}
	    }
	    else
		dAccess = rw;
	    break;
	default:
	    strcat(customOptions, ",");
	    strcat(customOptions, value);
	}
    }

    FREE(ptr);

    customOptions++;		/* skip leading comma */
    DEBUG1("customOptions = \"%s\"\n", customOptions);
    setPreviousValue(&localSetting.customOptions, customOptions);
    FREE(--customOptions);
    setChoicePreviousValue(&localSetting.defaultAccess, dAccess);
    setChoicePreviousValue(&localSetting.bootAdv,
			   (dfsp-> autoShare) ? yes : no);
        
    if (displayPopup == True)
    {
	resetFocus();
	SetMessage(nfsw-> localPropertiesPrompt, " ", Popup);
	MapGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt);
    }
    ManipulateGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt, 
		    ResetGizmoValue); 
    setExceptionSensitivity(localSetting.defaultAccess.current_value, True);

} /* end of LocalPropertyCB */



/*
 * LocalAddCB
 *
 */

extern void
LocalAddCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    NFSWindow *nfsw        = FindNFSWindow(w);
    ListHead  *headPtr;

    PropertyPopupMode = False;

    if (nfsw->localPropertiesPrompt == NULL)
	createLocalPopup(w);

    headPtr = (ListHead *) (ExceptionListG.settings-> initial_value);
    if (headPtr != NULL)
	FreeList(headPtr);
    headPtr-> numFields = 2;
    headPtr-> size = 0;
    headPtr-> list = NULL;

    ManipulateGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt, 
		    ReinitializeGizmoValue); 
    SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
    SetValue(GetPopupGizmoShell(nfsw-> localPropertiesPrompt), XtNtitle,
	     GetGizmoText(TXT_AddLocalTitle)); 

    OlVaFlatSetValues(AdvMenu.child, PropApply,
		      XtNlabel, GetGizmoText(TXT_Add2),
		      XtNmnemonic, *(GetGizmoText(CHR_Add)), NULL);
    
    OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadWrite,
		      XtNsensitive, True, NULL);
    OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadOnly,
		      XtNsensitive, False, NULL);
    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
		      XtNsensitive, False, NULL);
    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
		      XtNsensitive, False, NULL);

    XtUnmanageChild(extendedLabelG.controlWidget);
    resetFocus();
    MapGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt);

} /* end of LocalAddCB */


static void
addException(defaultAccessIndex type)
{
    extern NFSWindow * nfsw;

    ListHead *	headPtr = (ListHead *)ExceptionListG.settings-> current_value;
	    
    DEBUG0("addException Entry\n");
    ManipulateGizmo(InputGizmoClass, &hostG, GetGizmoValue);
    if(user.hostSelected &&
           !strcmp(localSetting.host.current_value, user.prevVal)){
                printf("Valid host no check\n"); /*JC, remove it*/
                ;                        /*valid host do nothing*/
    }else                                /* if verify host is invalid */
        if(verifyHost((char *)localSetting.host.current_value) == INVALID)
                     return ;

    insertException(headPtr, localSetting.host.current_value, type);

    DEBUG0("addException: after manipulate\n");
    return;
}


static void
AccessExceptionCB(Widget wid, XtPointer client_data,
		  OlFlatDropCallData *call_data) 
{
    extern NFSWindow *nfsw;
    Cardinal  item_index = call_data->item_data.item_index;
    ListHead *headPtr = (ListHead *)ExceptionListG.settings-> current_value;
    int item;

    SetMessage(nfsw-> localPropertiesPrompt, "", Popup);

    switch(item_index)
    {
    case ExceptionReadOnly:
	addException(ro);
	break;
    case ExceptionReadWrite:
	addException(rw);
	break;
    case ExceptionDelete:
	for (item = headPtr-> size - 1; item >= 0; item--)
	{
	    if (headPtr->list[item].set == True)
		DeleteListItem(&ExceptionListG, item);
	}
	if (headPtr-> size == 0)
	{
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
			      XtNsensitive, False, NULL);
	}
	break;
    case ExceptionDeleteAll:
	for (item = headPtr-> size - 1; item >= 0; item--)
	    DeleteListItem(&ExceptionListG, item);
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
			  XtNsensitive, False, NULL);
	OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
			  XtNsensitive, False, NULL);
	break;
    default:
	DEBUG0("default in AccessExceptionCB taken!!!\n");
	break;
    }

    /* move input focus back to the host input field */

	MoveFocus(hostG.textFieldWidget);

    return;
}


static void 
ExecuteCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}
static void 
SelectCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}

static void 
UnselectCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
    return;
}


static Boolean
verifyAllFields()
{
    extern  NFSWindow *nfsw;
    Gizmo   popup = nfsw-> localPropertiesPrompt;

    if (verifyLocalAccess((char *) localSetting.defaultAccess.current_value,
			ExceptionListG.settings->current_value) == INVALID)
		return INVALID;

    if (verifyShareOptions((char *)localSetting.customOptions.current_value,
                      popup) == INVALID)
        return INVALID;

    if (verifyLocalPath((char **)&localSetting.localPath.current_value,
			popup, localApplyMode, postVerifyCB) == INVALID) 
	return INVALID;

    setLabel();
    if (verifyLabel((char *)localSetting.folderLabel.current_value,
		    popup) == INVALID)
    	return INVALID;

    return VALID;
}

static Boolean
verifyLocalAccess(defaultAccessIndex access, ListHead *headPtr)
{

	if (access != none) return VALID;
	if (headPtr->size == 0) {
		/* we have no access selected without an exception list */
		SetMessage(nfsw->localPropertiesPrompt,
			TXT_NoAccessError, Notice);
		return INVALID;
	}
return VALID;		
}	
static void
newLocal(Boolean doShare)
{
    extern int ShareIt();
    extern DmObjectPtr UpdateContainer();
    extern NFSWindow  *nfsw;
    extern MenuGizmo   EditMenu;

    dfstab *dfsp;
    struct share *sharep;
    DmObjectPtr op;
    Cardinal index;

    dfsp = (dfstab *)MALLOC(sizeof(dfstab));
    dfsp-> sharep = sharep = (struct share *)CALLOC(1, sizeof(struct share));

    if ((int)localSetting.bootAdv.current_value == yes)
    {	
	dfsp-> autoShare = True;
    }
    else
    {
	dfsp-> autoShare = False;
    }
    sharep-> sh_path   = STRDUP(localSetting.localPath.current_value);
    sharep-> sh_res    = STRDUP(localSetting.folderLabel.current_value);
    sharep-> sh_fstype = STRDUP("nfs");
    parseLocalOptions();
    sharep-> sh_opts   = STRDUP(localSetting.options.current_value);
    writedfs(dfsp);

    /* add to iconbox */

    op = UpdateContainer(sharep-> sh_res, (XtPointer)dfsp, 
			 nfsw-> local_fcp, &index, nfsLocal);
    if ( op != (DmObjectPtr)NULL )
    {
	if (doShare)
	    (void)ShareIt(sharep, op);
	OlVaFlatSetValues(EditMenu.child, EditDelete,
			  XtNsensitive, OwnerLocal, NULL);
	OlVaFlatSetValues(EditMenu.child, EditProperties,
			  XtNsensitive, True, NULL);
    }

    return;
}

static void
updateData()
{
    extern NFSWindow * nfsw, *MainWindow;
    extern Boolean isShared();
     
    DeleteFlags flags = Silent;
    Boolean result;
    Boolean shared = True;
    struct share *sharep;
    dfstab *dfsp;

    RETURN_IF_NULL(nfsw-> localSelectList, TXT_ReselectFolderProp);
    RETURN_IF_NULL(nfsw-> localSelectList-> op, TXT_BadOp);

    dfsp = (dfstab *)nfsw-> localSelectList-> op -> objectdata;
    RETURN_IF_NULL(dfsp, TXT_BadDfs);

    RETURN_IF_NULL(dfsp, TXT_BadDfs);
    sharep = dfsp-> sharep;
    shared = isShared(sharep);
    parseLocalOptions();
    if (localSetting.folderLabel.current_value !=
	localSetting.folderLabel.previous_value ||
	localSetting.localPath.current_value !=
	localSetting.localPath.previous_value ||
	localSetting.options.current_value !=
	localSetting.options.previous_value ||
	localSetting.host.current_value !=
	localSetting.host.previous_value)
	flags = ReDoIt_Silent;

    result = DeleteLocalCB((Widget)NULL, (XtPointer)NULL, (XtPointer)flags);
    if (result != FAILURE) newLocal(shared);
    return;
}

static void 
localPropCB(w, client_data, call_data)
Widget    w;
XtPointer client_data;
XtPointer call_data;
{
    OlFlatCallData * p          = (OlFlatCallData *)call_data;
    NFSWindow *     nfsw        = FindNFSWindow(w);
    PopupGizmo *     popup      = nfsw-> localPropertiesPrompt;

    switch (p-> item_index)
    {
    case PropApply:
        ManipulateGizmo(InputGizmoClass, &hostG, GetGizmoValue);
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	DEBUG1("case PropApply: PropertyPopupMode = %d\n", PropertyPopupMode);
	if (PropertyPopupMode == True) /* property sheet */
	{
            if (nfsw-> viewMode != ViewLocal)
	    {
                SetMessage(nfsw-> localPropertiesPrompt,
			   TXT_ApplyNotLocal, Notice);
                return;
            }
	    ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
	    if (verifyAllFields() == VALID)
	    {			/* nothing wrong */
		updateData();
		MoveFocus(localPathG.textFieldWidget);
                if (bringDownPopups(PopupShell, False) == True)
                    PropertyPopupMode = False;
		return;
	    }
	    break;
	}
	else			/* add new folder mode */
	{
            if (nfsw-> viewMode != ViewLocal)
	    {
                SetMessage(nfsw-> localPropertiesPrompt,
			   TXT_AddNotLocal, Notice);
                return;
            }
	    ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
	    if (verifyAllFields() == VALID)
	    {			/* nothing wrong */
		newLocal(True);
		MoveFocus(localPathG.textFieldWidget);
                (void)bringDownPopups(PopupShell, False);
	    }
	    else
		return;
	}
	/* FALL THRU */ /* should only get here after successful add */
    case PropReset:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	if (PropertyPopupMode == True) /* property sheet mode*/
	{
	    ManipulateGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt, 
			    ResetGizmoValue);
	    setExceptionSensitivity(localSetting.defaultAccess.current_value,
				    True);
	}
	else			/* add new mode */
	{
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadWrite,
			      XtNsensitive, True, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionReadOnly,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
			      XtNsensitive, False, NULL);
	    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
			      XtNsensitive, False, NULL);
	    ManipulateGizmo(PopupGizmoClass, nfsw->localPropertiesPrompt, 
			    ReinitializeGizmoValue); 
	    
	}
	MoveFocus(localPathG.textFieldWidget);
	break;
    case PropCancel:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	SetWMPushpinState(XtDisplay(PopupShell), XtWindow(PopupShell),
			  WMPushpinIsOut); 
	PropertyPopupMode = False;
	(void)bringDownPopups(PopupShell, True);
	break;
    case PropHelp:
	if (PropertyPopupMode == True) /* property sheet mode*/
	    PostGizmoHelp(nfsw-> baseWindow-> shell,
			  &LocalPropWindowHelp);
	else
	    PostGizmoHelp(nfsw-> baseWindow-> shell,
			  &LocalAddWindowHelp);   
	break;
    default:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	DEBUG0("default in localPropCB taken!!!\n");
	break;
    }
    return;
}				/* end of localPropCB */

static void
growBuffer(char *ptr, size_t *allocated, size_t needed )
{
    size_t len;
    DEBUG1("allocated =%d\n", *allocated);

    if ((len = strlen(ptr)) + needed >= *allocated)
	if ((ptr = REALLOC(ptr, (*allocated += BUFSIZ))) == NULL)
	    NO_MEMORY_EXIT();
    DEBUG3("allocated =%d, len=%d, needed=%d\n", *allocated, len, needed);
    return;
}

static void 
parseLocalOptions()
{
    ListHead *headPtr = (ListHead *)ExceptionListG.settings->current_value;
    ListItem *list = headPtr-> list;
    defaultAccessIndex defaccess;
    int	      i, numItems = headPtr-> size;
    size_t    roSize = BUFSIZ, rwSize = BUFSIZ, len;
    char    **fields;
    char     *options, *optr;
    char     *roHosts, *roptr; 
    char     *rwHosts, *rwptr; 
    
    defaccess = localSetting.defaultAccess.current_value;

    if ((roptr = roHosts = MALLOC(BUFSIZ)) == NULL)
	        NO_MEMORY_EXIT();

    if ((rwptr = rwHosts = MALLOC(BUFSIZ)) == NULL)
	        NO_MEMORY_EXIT();

    strcpy(roHosts, ",ro=");
    strcpy(rwHosts, ",rw=");

    /* parse exceptions */

    for (i=0; i < numItems; i++)
    {
	char * ptr;
	defaultAccessIndex type;

	fields = (char **)list[i].fields;
	type   = list[i].clientData;

	if (type == ro && fields[0] != NULL)
	{
	    growBuffer(roHosts, &roSize, strlen(fields[0])+2);
	    ptr = strcat(roptr, fields[0]);
	    strcat(ptr, ":");
	}
	else if (type == rw && fields[0] != NULL)
	{
	    growBuffer(rwHosts, &rwSize, strlen(fields[0])+2);
	    ptr = strcat(rwptr, fields[0]);
	    strcat(ptr, ":");
	}

    }
    if (defaccess == ro  ||          /* read-only exceptions not allowed or */
	roHosts[i = (strlen(roHosts) - 1)] != ':')    /* no read-only hosts */
	i = 0;			
    roHosts[i] = EOS;

    if (defaccess == rw ||          /* read-write exceptions not allowed or */
	rwHosts[i = (strlen(rwHosts) - 1)] != ':')   /* no read-write hosts */
	i = 0;			
    rwHosts[i] = EOS;

    len = strlen(roHosts) + strlen(rwHosts) + MINIBUFSIZE;
    if (localSetting.customOptions.current_value)
	len += strlen(localSetting.customOptions.current_value);
    if ((optr = options = MALLOC(len)) == NULL)
	NO_MEMORY_EXIT();

    switch ((int)defaccess)
    {
    case ro:
	strcpy(options, "ro");
	break;
    case rw:
	strcpy(options, "rw");
	break;
    case none:
	/*FALLTHRU*/
    default:
	options[0] = EOS;
	break;
    }

    strcat(options, rwHosts);
    strcat(options, roHosts);
    if (options[0] == ',')
	optr++;
    if ( localSetting.customOptions.current_value != NULL &&
        *((char *)localSetting.customOptions.current_value) != EOS)
    {
        strcat(optr, ",");
        strcat(optr, localSetting.customOptions.current_value);
    }

    if (localSetting.options.current_value)
	FREE(localSetting.options.current_value);
    localSetting.options.current_value = STRDUP(optr);
    FREE(options);
    FREE(roHosts);
    FREE(rwHosts);
}


static void
insertException(ListHead *headPtr, char * host, defaultAccessIndex type)
{
    static char 	*readOnly = NULL;  
    static char 	*readWrite = NULL; 
    int			i;
    char **		tmp;

    DEBUG0("InsertException Entry\n");
    if (readOnly == NULL)
    {
	readOnly= STRDUP(GetGizmoText(TXT_ReadOnly));
	readWrite = STRDUP(GetGizmoText(TXT_ReadWrite));
    }

    /* FIX: check to see if host is already in list & delete old entry */

    i = headPtr-> size++;
    DEBUG2("numFields=%d; size=%d\n", headPtr->numFields, headPtr-> size);
    headPtr-> list = (ListItem *)REALLOC((char *)headPtr-> list,
					sizeof(ListItem)*headPtr-> size);
    headPtr-> list[i].set = False;
    headPtr-> list[i].fields = (XtArgVal)MALLOC
	(sizeof (XtArgVal *) * headPtr-> numFields);
    headPtr-> list[i].clientData = (XtArgVal)type;
    tmp = (char **)headPtr-> list[i].fields;
    tmp[0] = STRDUP(host);
    tmp[1] = STRDUP((type == ro) ? readOnly : readWrite);

    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDelete,
		      XtNsensitive, True, NULL);
    OlVaFlatSetValues(ExceptionMenu.child, ExceptionDeleteAll,
		      XtNsensitive, True, NULL);

    UpdateList(&ExceptionListG, headPtr);
}
    
static void 
defaultAccessCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData * p    = (OlFlatCallData *)call_data;
    NFSWindow *      nfsw = FindNFSWindow(w);

    SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
    setExceptionSensitivity(p-> item_index, False);
    return;
}


static void 
localPathCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;

    ManipulateGizmo(InputGizmoClass, &folderLabelG, GetGizmoValue);
    ManipulateGizmo(InputGizmoClass, &localPathG,   GetGizmoValue);
    if (verifyLocalPath((char **)&localSetting.localPath.current_value, 
			nfsw-> localPropertiesPrompt, localTabMode,
			postVerifyCB) == VALID)  
	setLabel();
    return;
}


static void 
setLabel()
{
    char * data, *lp = localSetting.localPath.current_value;
    int last;

    if (*((char *)localSetting.folderLabel.current_value) == EOS)
    {

	/* remove trailing / */
	last = strlen(lp) - 1;
	if (last > 0 && lp[last] == '/')
	{
	    lp[last] = EOS;
	    SetValue(localPathG.textFieldWidget, XtNstring, lp);
	}
	data = STRDUP(localSetting.localPath.current_value);

	setInitialValue(&localSetting.folderLabel, basename(data));
	ManipulateGizmo(InputGizmoClass, &folderLabelG,
			ReinitializeGizmoValue);
	setInitialValue(&localSetting.folderLabel, "");
	FREE(data);
    }
}
 
static void
getHostCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;
    static HelpText help;

    help.title = HostWindowHelp.title;
    help.file = HostWindowHelp.filename;
    help.section  = HostWindowHelp.section;

    SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
    user.help = &help;
    user.text = hostG.textFieldWidget;
    HostCB(wid, &user, call_data);
    return;
}

static void
findFolderCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    static FileGizmo *fileG = NULL;
    char *home;

    if (fileG == NULL)
    {
	fileG = &fileFolderPopup;
	if ((home = getenv("HOME")) == NULL)
		home = "";
	fileG-> directory = STRDUP(home);
	CreateGizmo(wid, FileGizmoClass, fileG, NULL, 0);
    }
    SetFileGizmoMessage(&fileFolderPopup, TXT_ShareItemPrompt);
    MapGizmo(FileGizmoClass, fileG);
	
    return;
}

static void
filePopupCB(Widget wid, XtPointer client_data, OlFlatDropCallData *call_data)
{
    extern NFSWindow *nfsw;
    Cardinal  item_index = call_data->item_data.item_index;
    Widget    shell      = GetFileGizmoShell(&fileFolderPopup);
    char     *file;
    int       last;

    switch ((filePopupItemIndex)item_index)
    {
    case ffApply:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	file = GetFilePath(&fileFolderPopup);
	last = strlen(file) - 1;
	if (last > 0 && file[last] == '/')
	    file[last] == EOS;	
	setInitialValue(&localSetting.localPath, file);
			
	ManipulateGizmo(InputGizmoClass, &localPathG,
			ReinitializeGizmoValue);
	setInitialValue(&localSetting.localPath, "");
	setLabel();
	BringDownPopup(shell);
	break;
    case ffCancel:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
	XtPopdown(shell);
	break;
    case ffHelp:
	PostGizmoHelp(nfsw-> baseWindow-> shell, &FindWindowHelp);
	break;
    default:
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	DEBUG0("default taken in filePopupCB\n");
	break;
    }
    return;
}

/* postVerifyCB is called after we create a directory. */
/* Here we do whatever would have been done if verifyLocalPath had */
/* returned VALID */

static void 
postVerifyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;
    Cardinal  item_index =
	((OlFlatDropCallData*)call_data)->item_data.item_index;
    Gizmo	popup = nfsw-> localPropertiesPrompt;
    Widget	nshell = GetModalGizmoShell(nfsw-> noticePopup);
    pathData   *pdata  = (pathData *)client_data;
    char       *path   =  pdata-> path;
    int         result;
    mode_t      st_mode = DIR_CREATE_MODE;
    
    switch ((noticeIndex)item_index)
    {
    case NoticeDoIt:
	SetMessage(popup, "", Popup);
	result = mkdirp(path, st_mode);
	XtPopdown(nshell);
	if (result < 0)
	{
	    SetMessage(popup, TXT_DirNotCreated, Notice);
	    break;
	}
	setLabel();
	if (pdata-> mode == localApplyMode &&
	    verifyLabel((char *)localSetting.folderLabel.current_value,
			popup) == VALID)
	{
	    if (PropertyPopupMode == True)
	    {
		updateData();
		if (bringDownPopups(PopupShell, False) == True)
		    PropertyPopupMode = False;
	    }
	    else
	    {
		newLocal(True);
		(void)bringDownPopups(PopupShell, False);
	    }
	}
	break;
    case NoticeCancel:
	/* FIX: do we want a "%s not created" message here? */
	SetMessage(nfsw-> localPropertiesPrompt, "", Popup);
	XtPopdown(nshell);
	break;
    case NoticeHelp:
        PostGizmoHelp(nfsw-> baseWindow-> shell, &CreateFolderNoticeHelp); 
	break;
    default:
	SetMessage(nfsw, "", Base);
	DEBUG0("default in local postVerifyCB taken!!!\n");
    }
    FREE((char *)pdata);
    return;
}

static void 
extendedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    extern NFSWindow *nfsw;

    SetMessage(nfsw-> localPropertiesPrompt, "", Popup);

    ManipulateGizmo(ChoiceGizmoClass, &extendedChoice, GetGizmoValue);
    if (*(char *)(localSetting.extended.current_value) == 'x')
    {
	XtManageChild(extendedLabelG.controlWidget);    
	MoveFocus(customOptionsG.textFieldWidget);
    }
    else
	XtUnmanageChild(extendedLabelG.controlWidget);    
}


static void 
hideExtendedCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    ManipulateGizmo(ChoiceGizmoClass, &extendedChoice,
		    ReinitializeGizmoValue); 
    XtUnmanageChild(extendedLabelG.controlWidget);
}



static void
resetFocus()
{
    Arg arg[5];

    XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)localPathG.textFieldWidget);
    XtSetValues(PopupShell, arg, 1);
}
