/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:dm_cbs.c	1.151.2.3"

/******************************file*header********************************

    Description:
	This file contains the source code for callback functions
	which are shared among components of the desktop manager.
*/
						/* #includes go here	*/

#include <stdio.h>
#include <stdlib.h>
#include <memutil.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "wb.h"

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ModalGizmo.h>

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static Boolean	BuildIconMenu(DmWinPtr, DmItemPtr, MenuItems **, Cardinal *);
static void I18NMenuItems(MenuItems * items);
static void Print(DmWinPtr wp, DmObjectPtr obj);
static void SelectUnselectAll(DmWinPtr, Boolean);
static void SetIconMenuClientData(MenuItems * items, XtPointer data);
static void FreeHelpInfoCB(Widget w, XtPointer client_data,
		XtPointer call_data);

					/* public procedures		*/

void DmIconMenuCB(Widget, XtPointer, XtPointer);
void DmEditSelectAllCB(Widget, XtPointer, XtPointer);
void DmEditUnselectAllCB(Widget, XtPointer, XtPointer);
XtPointer DmGetWinPtr(Widget w);
void DmHelpDeskCB(Widget, XtPointer, XtPointer);
void DmRegContextSensitiveHelp(Widget widget, int app_id, char *file,
	char *section);
void DmBaseWinHelpKeyCB(Widget, XtPointer, XtPointer);
void DmPopupWinHelpKeyCB(Widget, XtPointer, XtPointer);
void DmHelpSpecificCB(Widget, XtPointer, XtPointer);
void DmHelpTOCCB(Widget, XtPointer, XtPointer);
void DmHelpMAndKCB(Widget, XtPointer, XtPointer);
void DmIconMenuProc (Widget, XtPointer, XtPointer);
void DmMenuSelectCB(Widget, XtPointer, XtPointer);
void DmPrintCB(Widget, XtPointer, XtPointer);
void DmDblSelectProc(Widget w, XtPointer client_data, XtPointer call_data);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* Define Folder icon menu items */
static MenuItems FolderMenuItems[] = {
  { True, TXT_FILE_OPEN,    TXT_M_FILE_OPEN,  I_PUSH_BUTTON, NULL, NULL, },
  { True, TXT_FILE_PROP,    TXT_M_Properties,  I_PUSH_BUTTON, NULL, NULL, },
  { True, TXT_DELETE,	    TXT_M_DELETE,     I_PUSH_BUTTON, NULL, NULL, },
  { True, TXT_FILE_PRINT,   TXT_M_FILE_PRINT, I_PUSH_BUTTON, NULL, NULL, },
  { NULL },
};

static char *FolderLabels[] = { "Open", "Properties...", "Delete", "Print" };

/* Define Help Desk icon menu items */

static MenuItems HDMenuItems[] = {
  { True, TXT_FILE_OPEN, TXT_M_FILE_OPEN, I_PUSH_BUTTON,  NULL, NULL, },
  { NULL },
};

/* Define Tree view icon menu items */
static MenuItems TreeMenuItems[] = {
  { True, TXT_FILE_OPEN,     TXT_M_FILE_OPEN, I_PUSH_BUTTON,	NULL, NULL, },
  { True, TXT_FILE_PROP,     TXT_M_FILE_PROP, I_PUSH_BUTTON,	NULL, NULL, },
  { True, TXT_DELETE,	     TXT_M_DELETE, I_PUSH_BUTTON,	NULL, NULL, },
  { True, TXT_TREE_SHOW,     TXT_M_TREE_SHOW, I_PUSH_BUTTON,	NULL, NULL, },
  { True, TXT_TREE_HIDE,     TXT_M_TREE_HIDE, I_PUSH_BUTTON,	NULL, NULL, },
  { True, TXT_TREE_SHOW_ALL, TXT_M_TREE_SHOW_ALL, I_PUSH_BUTTON,NULL,NULL, },
  { True, TXT_TREE_HERE,     TXT_M_TREE_HERE, I_PUSH_BUTTON,	NULL, NULL, },
  { NULL },
};

/* Define Wastebasket icon menu items */
static MenuItems WBMenuItems[] = {
  { True, TXT_IM_WB_FILEPROP, TXT_M_IM_WB_FILEPROP, I_PUSH_BUTTON, NULL, NULL, },
  { True, TXT_IM_WB_PUTBACK,  TXT_M_IM_WB_PUTBACK, I_PUSH_BUTTON, 	NULL, NULL, },
  { True, TXT_IM_WB_DELETE,   TXT_M_IM_WB_DELETE, I_PUSH_BUTTON, 	NULL, NULL, },
  { NULL },
};

/* PWF MORE - Need to investigate if there is a cleaner way to do this
 *            (assumption is that Found Window will use icon_menu)
 */
/* Define gizmo for Folder & Found Icon Menu */
static MenuGizmo icon_menu = {
    NULL,				/* help info */
    "icon_menu",			/* shell name */
    NULL,				/* menu title */
    NULL,				/* menu items (run time) */
    NULL,				/* call back (in items) */
    NULL,				/* client data (in items) */
    XmVERTICAL,				/* layout type */
    1,					/* num rows */
    0					/* index of default item */
};

/* Save these in statics.  They cannot be passed conveniently to the File
   Icon menu callback since the menu is shared.

    'icon_item'	points to the item overwhich MENU was pressed.
    'icon_win'	points to the folder/toolbox window which contains the item.
*/
static DmItemPtr	icon_item;
static DmWinPtr		icon_win;
static DmWinPtr		iconmenu_for;
static Gizmo		iconmenu_gizmo;


/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    BuildIconMenu-
*/
static Boolean
BuildIconMenu(DmWinPtr window, DmItemRec * item,
	      MenuItems ** ret_menu_items, Cardinal * ret_num_items)
{
    Boolean	items_touched = False;

	static MenuItems *	menu_items = NULL;
	static Cardinal		num_alloced = 0;
	MenuItems *		menu_item;
	DtPropPtr		prop;
	int			num_props;
	DmObjectPtr		obj;
	int			num_dflt_items;

	/* Get the object data. */
	obj = ITEM_OBJ(item);

	/* The number of properties computed here is a conservative number
	   which may count duplicate entries: class and instance properties
	   (if specified) may be double-counted.  ('1' added for delimiter)
	*/
	num_props = XtNumber(FolderMenuItems) +
	    obj->plist.count + obj->fcp->plist.count;

	if (num_alloced < num_props)
	{
	    menu_items = (MenuItems *)REALLOC((void *)menu_items,
					      num_props * sizeof(MenuItems));

	    /* First time thru, copy in "fixed" buttons */
	    if (num_alloced == 0)
		(void)memcpy(menu_items,
			     FolderMenuItems, sizeof(FolderMenuItems));

	    num_alloced = num_props;

	} else
	{
	    menu_items[0].clientData = NULL;
	    menu_items[1].clientData = NULL;
	    menu_items[2].clientData = NULL;
	    menu_items[3].clientData = NULL;
	}

	/* Put 'end' after "fixed" buttons.  The "Print" button only
	   appears as a default button for files that type as data-files.
	*/
	menu_item = menu_items + XtNumber(FolderMenuItems) - 2;
	if (obj->ftype == DM_FTYPE_DATA)
	    *menu_item++ = FolderMenuItems[XtNumber(FolderMenuItems) - 2];
	num_dflt_items = (int)(menu_item - menu_items) + 1;

	for (prop = DmFindObjProperty(obj, DT_PROP_ATTR_MENU);
	     prop != NULL; prop = DmFindObjProperty(NULL, DT_PROP_ATTR_MENU))
	{
	    char *	name;
	    char *	value;
	    MenuItems *	ip;

	    /* Get name and value of property.  Skip any leading '_' in
	       property name.  Pass value of property as client_data (though
	       NULL for Print).
	    */
	    name = (prop->name[0] == '_') ? prop->name + 1 : prop->name;
	    value = (strcmp(name, "Print") == 0) ? NULL : prop->value;

	    for (ip = menu_items; ip < menu_item; ip++) {
		/* compare label with localized string */
		if (!strcmp(ip->label, name))
			break;

		/* For system names, compare label with "English" string */
		if ((prop->name != name) &&
		    ((ip - menu_items) < num_dflt_items) &&
		    (!strcmp(FolderLabels[ip - menu_items], name)))
			break;
	    }

	    if (ip < menu_item)		/* ie, duplicate name */
	    {
		if (value != NULL)
		    ip->clientData = value;
		continue;
	    }

	    /* Create new button using property name: button label is
	       property name; pass value as client_data.  (Make sure
	       client_data for "Print" button is NULL.)
	    */

	    menu_item->sensitive	= True;
	    menu_item->label		= name;
	    menu_item->type		= I_PUSH_BUTTON;
	    menu_item->mnemonic		= NULL;
	    menu_item->callback		= NULL;
	    menu_item->clientData	= (char *)value;
	    menu_item->set		= False;
	    menu_item->subMenu		= NULL;

	    menu_item++;
	    items_touched = True;
	}

	/* MenuGizmo needs terminated list of items */
	menu_item->label = NULL;

	*ret_menu_items	= menu_items;
	*ret_num_items	= menu_item - menu_items;

    return(items_touched);
}					/* end of BuildIconMenu */

static DmWinPtr
DmGetIconMenuFor(shell)
Widget *shell;
{
	*shell = QueryGizmo(PopupMenuGizmoClass, iconmenu_gizmo,
                                        GetGizmoWidget, NULL);
	return(iconmenu_for);
}

void
DmBringDownIconMenu(folder)
DmWinPtr folder;
{
	Widget shell;

	/*
	 * If the icon menu is currently associated with this
	 * base window, pop it down too.
	 */
	if (folder == DmGetIconMenuFor(&shell)) {
    		XtUnmanageChild(shell);
	}
}

/****************************procedure*header*****************************
    DmIconMenuCB- this callback services the buttons in the Icon menu
	used in Folder and Toolbox windows.  Callbacks for the icon
	menu in the wastebasket window are handled elsewhere.
*/
void
DmIconMenuCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;

    DmClearStatus(icon_win);			/* Clear footer first */

    switch(cbs->index)
    {
    case 0:					/* "Open" button */
	/* This could take awhile so entertain the user */
	BUSY_FOLDER((DmFolderWindow)icon_win);
	DmOpenObject(icon_win, ITEM_OBJ(icon_item), DM_B_OPEN_IN_PLACE);
	break;

    case 2:					/* "Delete" button */
    {
	DmItemPtr * items = (DmItemPtr *)XtMalloc(2 * sizeof(DmItemPtr));
	items[0] = icon_item;
	items[1] = NULL;
	DeleteItems((DmFolderWindow)icon_win, items,
		    UNSPECIFIED_POS, UNSPECIFIED_POS);
	break;
    }

    case 1:					/* "Property" button */
	if ((icon_win->attrs & DM_B_FOLDER_WIN) && (cbs->clientData == NULL))
	{
	    Dm__PopupFilePropSheet((DmFolderWinPtr)icon_win, icon_item);
	    break;
	}
	/* otherwise, FALLTHROUGH */
    default:
	if (cbs->clientData == NULL)		/* Must be "Print" button */
	{
	    Print(icon_win, ITEM_OBJ(icon_item));
	} else
	{
	    DmObjectPtr	obj = ITEM_OBJ(icon_item);
	    char *	cmd;

	    cmd =
		Dm__expand_sh((char *)cbs->clientData, DmObjProp, (XtPointer)obj);
	    DmExecuteShellCmd(icon_win, obj, cmd, LAUNCH_FROM_CWD(Desktop));
	    FREE(cmd);
	}
	break;
    }
}	/* end of DmIconMenuCB */


/****************************procedure*header*****************************
    I18NMenuItems-
*/
static void
I18NMenuItems(MenuItems * items)
{
    for ( ; items->label != NULL; items++)
    {
	items->label = GetGizmoText(items->label);

#ifdef REAL_MNEMONIC
	if (items->mnemonic == NULL)
	    items->real_mnemonic = 0;
	else
	{
	    char * mnemonic = GetGizmoText(items->mnemonic);
	    items->real_mnemonic = (XtArgVal)mnemonic[0];
	}
#endif /*REAL_MNEMONIC */
    }
}
/****************************procedure*header*****************************
    Print-
*/
static void
Print(DmWinPtr wp, DmObjectPtr obj)
{
    char *	default_printer;
    char *	prt_cmd;

    /* Get print command from property (if any) */
    prt_cmd = DmGetObjProperty(obj, PRINTCMD, NULL);

    if (prt_cmd == NULL)
    {
	if (obj->ftype == DM_FTYPE_DIR)
	{
	    DmVaDisplayStatus(wp, 1, TXT_NO_FOLDER_METHOD); 
	    return;
	}
	if (obj->ftype != DM_FTYPE_DATA)
	{
	    DmVaDisplayStatus(wp, 1, TXT_NO_PRINT_METHOD, obj->name, DmObjClassName(obj), DmObjClassName(obj));
	    return;
	}

	/* This is the default for all data files */
	prt_cmd = DmGetDTProperty(DFLTPRINTCMD, NULL);
    }

    /* If print command uses default printer but property is not
     * defined, alert user.
     */
    default_printer = "%" _DEFAULT_PRINTER;
    if((strstr(prt_cmd, default_printer) != 0) &&
       (DmGetDTProperty(_DEFAULT_PRINTER, NULL) == NULL))
    {
	DmVaDisplayStatus(wp, 1, TXT_NO_DEFAULT_PRT, obj->name);
	return;
    }

    /* Expand command (returns malloc'ed string) */
    prt_cmd = Dm__expand_sh(prt_cmd, DmObjProp, (XtPointer)obj);
    DmExecuteShellCmd(wp, obj, prt_cmd, LAUNCH_FROM_CWD(Desktop));
    FREE(prt_cmd);
}
	
/****************************procedure*header*****************************
    SelectUnselectAll- this routine selects or unselects all the items in
	the folder.  'select' determines setting.
*/
static void
SelectUnselectAll(DmWinPtr window, Boolean select)
{
    int			i;
    DmItemPtr		ip;
    Boolean		touched = False;
    int			cnt;	/* # of items affected */

    for (i = 0, cnt = 0, ip = window->views[0].itp; i < window->views[0].nitems; i++, ip++)
	if (ITEM_MANAGED(ip))
	{
	    cnt++;
	    if (ITEM_SELECT(ip) != select)
	    {
		ip->select = select;
		touched	= True;
	    }
	}

    if (touched)
    {
	XtSetArg(Dm__arg[0], XmNselectCount, (select ? cnt : 0));
	DmTouchIconBox(window, Dm__arg, 1);
	DmDisplayStatus(window);
	/* Update any file operation prompts that are posted */
	DmUpdatePrompts(window);
    }
}				/* End of SelectUnselectAll */

/****************************procedure*header*****************************
    SetIconMenuClientData-
*/
static void
SetIconMenuClientData(MenuItems * items, XtPointer data)
{
    while (items->label != NULL)
	items++->clientData = (char *)data;
}

/***************************private*procedures****************************

    Public Procedures
*/

/*************************************************************************
	Edit menu item callbacks

    These callbacks handle (common) menu items from the Edit menu.
*/

/****************************procedure*header*****************************
    DmEditSelectAllCB-
*/
void
DmEditSelectAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    SelectUnselectAll((DmWinPtr)DmGetWinPtr(w), True);

}				/* End of DmEditSelectAllCB */

/****************************procedure*header*****************************
    DmEditUnselectAllCB-
*/
void
DmEditUnselectAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    SelectUnselectAll((DmWinPtr)DmGetWinPtr(w), False);

}				/* End of DmEditUnselectAllCB */

/*************************************************************************
	Help menu item callbacks

    Each Desktop window (Toolbox, Folder, Wastebasket) has a help menu.  Menu
    items on that menu call these callbacks.  There is one Help menu shared
    between all Folder windows and one Help menu shared between all Toolbox
    windows.
*/
/****************************procedure*header*****************************
    DmHelpDeskCB- called to bring up the Help Desk.
*/

void
DmHelpDeskCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (DESKTOP_HELP_DESK(Desktop) == NULL) {
		DmInitHelpDesk(NULL, False, True);
	} else {
		DmMapWindow((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
		DmClearStatus((DmWinPtr)DESKTOP_HELP_DESK(Desktop));
		XRaiseWindow(XtDisplay(DESKTOP_HELP_DESK(Desktop)->shell),
			XtWindow(DESKTOP_HELP_DESK(Desktop)->shell));
	}
}

/****************************procedure*header*****************************
 * DmRegContextSensitiveHelp - Registers context-sensitive help on a widget
 * using DmPopupWinHelpKeyCB for the XmNhelpCallback resource.  It also
 * registers a destroyCallback to free hip when the widget is destroyed.
 */
void
DmRegContextSensitiveHelp(Widget widget, int app_id, char *file, char *section)
{
	DmHelpInfoPtr hip;

	hip = (DmHelpInfoPtr)MALLOC(sizeof(DmHelpInfoRec));
	hip->app_id  = app_id;
	hip->file    = STRDUP(file);
	hip->section = STRDUP(section);

	XtAddCallback(widget, XmNhelpCallback, DmPopupWinHelpKeyCB,
		(XtPointer)hip);
	XtAddCallback(widget, XmNdestroyCallback, FreeHelpInfoCB,
		(XtPointer)hip);

} /* end of DmRegContextSensitiveHelp */

/****************************procedure*header*****************************
 * FreeHelpInfoCB - Frees help info allocated in DmRegContextSensitiveHelp().
 */
static void
FreeHelpInfoCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpInfoPtr hip = (DmHelpInfoPtr)client_data;

	FREE(hip->file);
	FREE(hip->section);
	FREE(hip);

} /* end of FreeHelpInfo */

/****************************procedure*header*****************************
 * DmPopupWinHelpKeyCB - helpCallback for popup windows to display context-
 * sensitive help.  Help info is passed in via client_data.
 */
void
DmPopupWinHelpKeyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmHelpInfoPtr hip = (DmHelpInfoPtr)client_data;

	DmDisplayHelpSection(DmGetHelpApp(hip->app_id), NULL, hip->file,
		hip->section);

} /* end of DmPopupWinHelpKeyCB */

/****************************procedure*header*****************************
 * DmBaseWinHelpKeyCB - helpCallback for folders, Folder Map, Wastebasket
 * and Help Desk to display context-sensitive help.  This function is to
 * work around not being able to use DmHelpSpecificCB() directly with the
 * helpCallback resource for those windows by simulating selection of the
 * first menu button in those window's Help menu. client_data is the widget
 * id of the first button in the Help menu.
 */
void
DmBaseWinHelpKeyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFolderWinPtr fwp;

	XtSetArg(Dm__arg[0], XmNuserData, &fwp);
	XtGetValues(w, Dm__arg, 1);

	DmSetWinPtr((DmWinPtr)fwp);
	DmHelpSpecificCB((Widget)client_data, NULL, NULL); 

} /* end of DmBaseWinHelpKeyCB */

/************************************************************************
 * help_info table- used for Help CBs below.
 */
#define OFFSET(field)	XtOffsetOf(DmDesktopRec, field)

    /* Table is order dependent.  For instance, want to test for TOP_FOLDER
     * and TREE_WIN before FOLDER_WIN.
     */
static const struct foobar {
	DtAttrs		attrs;
	Cardinal	id_offset;
	String		file;
	String		sect;
    } *hip, help_info[] = {
    { DM_B_TOP_FOLDER, OFFSET(desktop_help_id),
	  DESKTOP_HELPFILE, DESKTOP_INTRO_SECT },
    { DM_B_TREE_WIN, OFFSET(fmap_help_id),
	  FMAP_HELPFILE, FMAP_INTRO_SECT },
    { DM_B_APPLICATIONS_WIN, OFFSET(folder_help_id),
	  APPLICATIONS_FOLDER_HELPFILE, APPLICATIONS_FOLDER_INTRO_SECT },
    { DM_B_PREFERENCES_WIN, OFFSET(folder_help_id),
	  PREFERENCES_FOLDER_HELPFILE, PREFERENCES_FOLDER_INTRO_SECT},
    { DM_B_DISKS_ETC_WIN, OFFSET(folder_help_id),
	  DISKS_ETC_FOLDER_HELPFILE, DISKS_ETC_FOLDER_INTRO_SECT },
    { DM_B_NETWORKING_WIN, OFFSET(folder_help_id), 
	  NETWORKING_FOLDER_HELPFILE, NETWORKING_FOLDER_INTRO_SECT },
    { DM_B_ADMIN_TOOLS_WIN, OFFSET(folder_help_id), 
	  ADMIN_TOOLS_FOLDER_HELPFILE, ADMIN_TOOLS_FOLDER_INTRO_SECT },
    { DM_B_MAILBOX_WIN, OFFSET(folder_help_id), 
	  MAILBOX_FOLDER_HELPFILE, MAILBOX_FOLDER_INTRO_SECT },
    { DM_B_GAMES_WIN, OFFSET(folder_help_id), 
	  GAMES_FOLDER_HELPFILE, GAMES_FOLDER_INTRO_SECT },
    { DM_B_NETWARE_WIN, OFFSET(folder_help_id), 
	  NETWARE_FOLDER_HELPFILE, NETWARE_FOLDER_INTRO_SECT },
    { DM_B_STARTUP_ITEMS_WIN, OFFSET(folder_help_id), 
	  STARTUP_ITEMS_FOLDER_HELPFILE, STARTUP_ITEMS_FOLDER_INTRO_SECT },
    { DM_B_WALLPAPER_WIN, OFFSET(folder_help_id),
	  WALLPAPER_FOLDER_HELPFILE, WALLPAPER_FOLDER_INTRO_SECT },
    { DM_B_UUCP_INBOX_WIN, OFFSET(folder_help_id), 
	  UUCP_INBOX_FOLDER_HELPFILE, UUCP_INBOX_FOLDER_INTRO_SECT },
    { DM_B_DISKETTE_WIN, OFFSET(folder_help_id), 
	  DISKETTE_FOLDER_HELPFILE, DISKETTE_FOLDER_INTRO_SECT },
    { DM_B_CDROM_WIN, OFFSET(folder_help_id), 
	  CDROM_FOLDER_HELPFILE, CDROM_FOLDER_INTRO_SECT },
    { DM_B_FOLDER_WIN, OFFSET(folder_help_id),
	  FOLDER_HELPFILE, FOLDER_INTRO_SECT },
    { DM_B_WASTEBASKET_WIN, OFFSET(wb_help_id),
	  WB_HELPFILE, WB_INTRO_SECT },
    };
#undef OFFSET

/****************************procedure*header*****************************
 * DmHelpSpecificCB- called for specfic (window-dependent) help (usually the
 *	1st menu item).
 *
 *	This callback uses the 'attrs' field in the Window struct to
 *	identifiy which window invoked the callback.  Normally this would be
 *	done by passing client data that would identify the Window type but
 *	this is not possible when the menus are shared.
 */
void
DmHelpSpecificCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr window = (DmWinPtr)DmGetWinPtr(w);

    for (hip = help_info; hip < help_info + XtNumber(help_info); hip++)
    {
	if (window->attrs & hip->attrs)
	{
	    int * help_id = (int *)(((char *)Desktop) + hip->id_offset);

	    DmDisplayHelpSection(DmGetHelpApp(*help_id), NULL,
				 hip->file, hip->sect);
	    break;
	}
    }
}

/****************************procedure*header*****************************
    DmHelpMAndKCB- called to bring up Mouse and Keyboard Help
*/
void
DmHelpMAndKCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmDisplayHelpSection(DmGetHelpApp(DESKTOP_HELP_ID(Desktop)),
	NULL, DESKTOP_HELPFILE, MOUSE_AND_KEYBOARD_SECT);
}					/* end of DmHelpMAndKCB */

/****************************procedure*header*****************************
    DmHelpTOCCB- called to bring up Help Table of Contents
*/
void
DmHelpTOCCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr window = (DmWinPtr)DmGetWinPtr(w);

    for (hip = help_info; hip < help_info + XtNumber(help_info); hip++)
    {
	if (window->attrs & hip->attrs)
	{
	    int * help_id = (int *)(((char *)Desktop) + hip->id_offset);
	    DmHelpAppPtr help_app = DmGetHelpApp(*help_id);

	    DmDisplayHelpTOC(w, &(help_app->hlp_win),
			     hip->file, help_app->app_id);
	    break;
	}
    }
}					/* end of DmHelpTOCCB */

/****************************procedure*header*****************************
    DmIconMenuProc-
*/
void
DmIconMenuProc(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr			window = (DmWinPtr)client_data;
    DmFolderWinPtr              folder = (DmFolderWinPtr) client_data;
    ExmFIconBoxButtonCD *	data = (ExmFIconBoxButtonCD *)call_data;
    DmItemPtr			item = ITEM_CD(data->item_data);
    Boolean			items_touched;
    MenuItems *			menu_items;
    Cardinal			count;

    Widget			row_col;

    /*
     * Remember for which base window the icon menu pop up, so that if
     * the base window goes away, say alt-f4 was pressed, before the icon
     * menu was pop down, we need to pop down the menu ourselves.
     */
    if (icon_win == NULL)
    {
	I18NMenuItems(FolderMenuItems);
	I18NMenuItems(HDMenuItems);
	I18NMenuItems(TreeMenuItems);
	I18NMenuItems(WBMenuItems);
    }
    icon_item	 = item;
    icon_win	 = window;
    iconmenu_for = window;

    if (iconmenu_gizmo) {
    	row_col = QueryGizmo(PopupMenuGizmoClass, iconmenu_gizmo, 
					GetGizmoWidget, NULL);
	FreeGizmo(PopupMenuGizmoClass, iconmenu_gizmo);
	XtDestroyWidget(row_col);
    }

    if (folder->attrs & DM_B_HELPDESK_WIN) {
	icon_menu.callback = DmHDIMOpenCB;
	SetIconMenuClientData(HDMenuItems, icon_item);
    	icon_menu.items = HDMenuItems;
    }
    else
    if (folder->attrs & DM_B_WASTEBASKET_WIN) {
	icon_menu.callback = DmWBIconMenuCB;
   	SetIconMenuClientData(WBMenuItems, icon_item);
    	icon_menu.items = WBMenuItems;
    }
    else
    if (IS_TREE_WIN(Desktop, folder)) {
	icon_menu.callback = TreeIconMenuCB;
	SetIconMenuClientData(TreeMenuItems, icon_item);
	if (!strcmp(OBJ_CLASS_NAME(ITEM_OBJ(item)), NETWARE_SERVER))
		TreeMenuItems[2].sensitive = False;
	else
		TreeMenuItems[2].sensitive = True;
    	icon_menu.items = TreeMenuItems;
    }
    else {

    	/* Build menu */
    	items_touched = BuildIconMenu(window, item, &menu_items, &count);

    	icon_menu.callback = DmIconMenuCB;
    	icon_menu.items = menu_items;

	/* Make Delete button insensitive for NetWare server's icon menu */
	if (window->attrs & DM_B_NETWARE_WIN ||
		!strcmp(OBJ_CLASS_NAME(ITEM_OBJ(item)),NETWARE_SERVER) ||
		!strcmp(OBJ_CLASS_NAME(ITEM_OBJ(item)),NETWARE_SERVER_LINK))
	{
		icon_menu.items[2].sensitive = False;
	} else {
		icon_menu.items[2].sensitive = True;
	}

    }

    /* Use Desktop Shell since it won't go away */
    iconmenu_gizmo = (Gizmo)CreateGizmo(DESKTOP_SHELL(Desktop), 
				PopupMenuGizmoClass, &icon_menu, NULL, 0);

    row_col = QueryGizmo(PopupMenuGizmoClass, iconmenu_gizmo, 
					GetGizmoWidget, NULL);

	/* Setup `menu_widget' (that contains XmMenuShellWidget id)
	 * field and let FIconBox handle posting... */
    data->menu_widget = XtParent(row_col);
}				/* End of DmIconMenuProc */

void
DmButtonSelectProc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DmWinPtr window = (DmWinPtr)client_data;

    /* Update any file operation prompts that are posted */
    DmUpdatePrompts(window);
    DmDisplayStatus(window);
}

/****************************procedure*header*****************************
   The functions DmMenuSelectCB and DmGetWinPtr are used to
   facilitate shared menus among base windows. Before a button
   in a menu is referenced by a menu button, the menu button will
   invoke the Select callback, which is DmMenuButtonSelectCB. This
   remembers which base window is invoked from. Then when any of
   the select callback for buttons in the menu is invoked, it will
   fetch the information by calling DmGetWinPtr.
*/

static void * win_ptr;
static Widget menu_shell;

void *
DmGetLastMenuShell(shell)
Widget *shell;
{
	*shell = menu_shell;
	return(win_ptr);
}

void
DmMenuSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWinPtr win = (DmFolderWinPtr)(cbs->clientData);
    int menu_bar_index 	= cbs->index;
    int menu_index 	= 0;

    MenuItems *	item;
    Arg		args[1];
    int		nselected;

    char  	buff[20];
    char  	menu_name[20];

    Gizmo	menu_bar = GetBaseWindowMenuBar(win->gizmo_shell);
    Gizmo 	active_menu; 
    Widget 	active_widget; 
    Boolean	change_sensitivity;
	Widget button;
	Arg	arg[1];

    /* PWF- THIS VALUE FOR MENU_SHELL IS NOT CORRECT (just a place holder) */
    menu_shell = DtGetShellOfWidget(win->shell);
    win_ptr = (void *)(cbs->clientData);

    sprintf(buff, "%s:%d", "menubar", menu_bar_index);
  
    active_menu = QueryGizmo(MenuBarGizmoClass, menu_bar, GetGizmoGizmo, buff);

    if (win->attrs & DM_B_WASTEBASKET_WIN)
    	DmSetWBMenuItem(&item, menu_name, menu_bar_index, win);
    else
    if (win->attrs & DM_B_TREE_WIN) 
	DmSetFMMenuItem(&item, menu_name, menu_bar_index, win);
    else
    if (win->attrs & DM_B_HELPDESK_WIN)
	DmSetHDMenuItem(&item, menu_name, menu_bar_index, win);
    else
    if (win->attrs & DM_B_FOUND_WIN)
        DmSetFWMenuItem(&item, menu_name, menu_bar_index, win);
    else
    	DmSetFolderMenuItem(&item, menu_name, menu_bar_index, win);

    /*
     * Remember the popup menu shell, so that the menu can be popdown if
     * for any reason the base window is unmapped later before menu was
     * pop down.
     */
#ifdef SHARED_MENU
    menu_shell = DtGetShellOfWidget(item[item_idx].mod.nextTier->parent);

    /* Get to the submenu's item list */
    item = item[item_idx].mod.nextTier->items;
#endif /* SHARED_MENU */

    XtSetArg(args[0], XmNselectCount, &nselected);
    XtGetValues(((DmWinPtr)win_ptr)->views[0].box, args, 1);

    /* convert # of selected items to DM_B_ANY, DM_B_ONE, or DM_B_MORE */
    switch(nselected)
    {
    case 0:
	nselected = DM_B_ANY;
	break;

    case 1:
	nselected = DM_B_ONE;
	break;

    default:
	nselected = DM_B_ONE_OR_MORE;
	break;
    }
	
    while (item->label != NULL) {

	DmMenuItemCDType type = (DmMenuItemCDType)item->clientData;


	sprintf(buff, "%s:%d", menu_name, menu_index);
    	active_widget = QueryGizmo(PulldownMenuGizmoClass, active_menu,
					GetGizmoWidget, buff);


	/* 
	 * This comparison will only work if the table is true:
	 *
	 *	ANY	> 1	ONE
	 *	-------------------
	 * ANY | OK	-	 -
	 * > 1 | OK	OK	 -
	 * ONE | OK	OK	OK
	 *
	 * Also, if client_data is ANY, don't change item's sensitivity.
	 * Because it may be forced to True or False.
	 */
	switch (type)
	{
	case DM_B_ANY:
	    change_sensitivity = False;
	    break;

	case DM_B_LINK:
	    item->sensitive = (((((DmFolderWinPtr)win_ptr)->views[0].cp->attrs & DM_B_NO_LINK) == 0)  && (nselected != DM_B_ANY));
	    change_sensitivity = True;
	    break;

	case DM_B_UNDO:
	    item->sensitive = (((DmFolderWinPtr)win_ptr)->task_id != NULL);
	    change_sensitivity = True;
	    break;

	case DM_B_ONE:
	case DM_B_ONE_OR_MORE:
	    item->sensitive = (nselected >= (int)(item->clientData));
	    change_sensitivity = True;
	    break;

	default:
	    change_sensitivity = False;
	    break;
	}
	if (change_sensitivity) {
            XtSetArg(Dm__arg[0], XmNsensitive, item->sensitive);
            XtSetValues(active_widget, Dm__arg, 1);
	}
        menu_index++;
        item++;
    }
    /* 
     * If we are in a rooted folder desensitize the File->Open_New button 
     * See f_create.c: DmCheckRoot.
     */
    if (win->root_dir) {
		button = QueryGizmo (BaseWindowGizmoClass, win->gizmo_shell, 
			     GetGizmoWidget, "filemenu:2");
		XtSetArg(arg[0], XmNsensitive, False);
		if (button)
	    	XtSetValues(button, arg, 1);
	} else if (menu_bar_index == 1 && (win->attrs & DM_B_FOUND_WIN ||
		(win->attrs & DM_B_NETWARE_WIN && win->attrs & DM_B_TREE_WIN)))
	{
		/* file ops are not allowed in /.NetWare folder so make file ops
		 * menu items insensitive
		 */
		void *items = (DmItemPtr *)DmGetItemList((DmWinPtr)win, ExmNO_ITEM);
		DmItemPtr *itp = (DmItemPtr *)items;
		Boolean found = False;

		/* only update menu sensivitiy if NetWare server is selected */
		for (; *itp; *(++itp)) {
			if (!strcmp(OBJ_CLASS_NAME(ITEM_OBJ(*itp)),NETWARE_SERVER) ||
			!strcmp(OBJ_CLASS_NAME(ITEM_OBJ(*itp)),NETWARE_SERVER_LINK))
			{
				found = True;
				break;
			}
		}
		if (found) {
			XtSetArg(arg[0], XmNsensitive, False);
			for (menu_index = 1; menu_index < 7; menu_index++) {
				/* skip Link button */
				if (menu_index == 4)
					continue;
				sprintf(buff, "%s:%d", menu_name, menu_index);
				button = QueryGizmo(PulldownMenuGizmoClass, active_menu,
					GetGizmoWidget, buff);
				XtSetValues(button, arg, 1);
			}
		}
	}
}

void
DmSetWinPtr(DmWinPtr wp)
{
	win_ptr = wp;
}

XtPointer
DmGetWinPtr(Widget w)
{
    if (win_ptr)
	DmClearStatus((DmWinPtr)win_ptr);
    return(win_ptr);
}

/****************************procedure*header*****************************
    DmPrintCB- called by pressing "Print" on File menu.
*/
void
DmPrintCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmWinPtr	window = (DmWinPtr)client_data;
    DmItemPtr	item;
    int		i;

    for (i = 0, item = window->views[0].itp; i < window->views[0].nitems; i++, item++)
	if (ITEM_MANAGED(item) && ITEM_SELECT(item))
	    Print(window, ITEM_OBJ(item));
} /* End of DmPrintCB */

/****************************procedure*header*****************************
	DmDblSelectProc - Called when an icon is double-clicked on.
 */
void
DmDblSelectProc(Widget w, XtPointer client_data, XtPointer call_data)
{
    ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;

    DmWinPtr	window = (DmWinPtr) client_data;
    DmContainerPtr cp = window->views[0].cp;
    

    BUSY_FOLDER((DmFolderWindow) window);
    DmOpenObject(window, OBJECT_CD(d->item_data), DM_B_OPEN_IN_PLACE);

    /* After we do a DmOpenObject, the original item list,
     * and objects it references may be invalid, so we cannot 
     * reference it again.
     * FLH MORE: we need a better way to do this. The same block
     * of memory could be freed and realloced.
     */
    if (window->views[0].cp != cp ){
	_DPRINT3(stderr,"DmDblSelectProc OpenInPlace: don't sensitize item\n");
	d->ok = False;
    }
} /* end of DmDblSelectProc */
