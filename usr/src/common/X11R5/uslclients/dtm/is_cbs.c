/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:is_cbs.c	1.31"

/******************************file*header********************************

    Description:
     This file contains the source code for Icon Setup.
*/
		/* includes go here */

#include "icon_setup.h"

static void IconFileGizmoPopdnCB(CB_PROTO);
static void TmplFileGizmoPopdnCB(CB_PROTO);
static void IconStubTransferCB(Widget w, XtPointer closure, Atom *seltype,
		Atom *type, XtPointer value, unsigned long *length,int format);
static void AddTemplate(WinInfoPtr wip);
static void ModifyTemplate(WinInfoPtr wip);
static void DeleteTemplate(WinInfoPtr wip);
static void AddClass(Widget parent, DmContainerPtr cp, char *name,
		DmFclassPtr fcp, Boolean viewable);
static int ValidateClassName(char *name, Boolean new_class);
static int is_empty_str(char *str);
static void SetClassProperties(DtPropListPtr plistp, WinInfoPtr wip,
		DtAttrs change);
static void SyncClassList();
static void ChangeItemGlyph(DmItemPtr ip);
static int CheckPatternAndPath(WinInfoPtr wip);
static int ValidateIconFile(char *icon_file);
static void FreeTemplates(WinInfoPtr wip, int which);
static void FocusEventHandler(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use);
static void RemoveObject(DmObjectPtr obj);

	/* File Gizmos */

static MenuItems findMenuItems[] = {
  { True, TXT_OK,     TXT_M_OK,     I_PUSH_BUTTON, NULL, NULL, NULL, True},
  { True, TXT_CANCEL, TXT_M_CANCEL, I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { True, TXT_HELP,   TXT_M_HELP,   I_PUSH_BUTTON, NULL, NULL, NULL, False},
  { NULL }
};

static MenuGizmo tmplFindMenu = {
	NULL, "tmplFindMenu", "", findMenuItems, DmISTmplFindMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 1
};

static FileGizmo tmplFileGizmo = {
	NULL, 				/* help */
	"tmplFileGizmo",		/* name */
	TXT_ICON_SETUP_FIND_TMPL,	/* title */
	&tmplFindMenu,			/* menu */
	NULL, 				/* upperGizmos */
	0, 				/* numUpper */
	NULL, 0,			/* lowerGizmos, numLower */
	TXT_FROM,			/* path Label */
	TXT_USE,			/* input Label */
	FOLDERS_AND_FILES, 		/* dialogType */
	"",				/* directory */
	ABOVE_LIST,			/* lower gizmo position */
};

static MenuGizmo iconFindMenu = {
	NULL, "iconFindMenu", "", findMenuItems, DmISIconFindMenuCB, NULL,
	XmHORIZONTAL, 1, 0, 1
};

static FileGizmo iconFileGizmo = {
	NULL, 				/* help */
	"iconFileGizmo",		/* name */
	TXT_ICON_SETUP_FIND_ICON,	/* title */
	&iconFindMenu, 			/* menu */
	NULL, 				/* upperGizmos */
	0, 				/* numUpper */
	NULL, 0,			/* lowerGizmos, numLower */
	TXT_SHOW_ICONS_IN,		/* path Label */
	TXT_QUICK_SHOW,			/* input Label */
	FOLDERS_ONLY, 			/* dialogType */
	"",				/* directory */
	ABOVE_LIST,			/* lower gizmo position */
};

/****************************procedure*header*****************************
 * DmISInitClassWin - Initializes settings for a New (op is NULL in this
 * case) or Properties window.  The settings for a New window are the default
 * settings; the settings for a Properties window are obtained from the
 * property values of the specified class.
 */
void
DmISInitClassWin(DmObjectPtr op, WinInfoPtr wip, int class_type)
{
	char *p;
	Widget w1, w2;

	if (op) {
		wip->new_class = False;

		OLD_VAL(class_name) = STRDUP(op->name);

		/* In releases prior to UW 2.0, a file class of any type can
		 * have the _CLASSNAME and SYSTEM property set, so don't check
		 * for file type here.
		 */
		if (p = DmGetObjProperty(op, CLASS_NAME, NULL))
			OLD_VAL(label) = STRDUP(GGT(p));

		if ((p = DmGetObjProperty(op, SYSTEM, NULL)) &&
		  (*p == 'y' || *p == 'Y'))
			OLD_VAL(to_wb) = 1;
		else
			OLD_VAL(to_wb) = 0;

		p = DmGetObjProperty(op, FILETYPE, NULL);
		if (!strcmp(p, "DATA"))
			wip->class_type = FileType;
		else if (!strcmp(p, "DIR"))
			wip->class_type = DirType;
		else if (!strcmp(p, "EXEC"))
			wip->class_type = AppType;

		if (p = DmGetObjProperty(op, PATTERN, NULL))
			OLD_VAL(pattern) = STRDUP(p);

		if (p = DmGetObjProperty(op, ICONFILE, NULL))
			OLD_VAL(icon_file) = STRDUP(p);

		if (p = DmGetObjProperty(op, FILEPATH, NULL))
			OLD_VAL(filepath) = STRDUP(p);

		if (p = DmGetObjProperty(op, LFILEPATH, NULL))
			OLD_VAL(lfilepath) = STRDUP(p);

		if (p = DmGetObjProperty(op, LPATTERN, NULL))
			OLD_VAL(lpattern) = STRDUP(p);

		if (p = DmGetObjProperty(op, DFLTICONFILE, NULL))
			OLD_VAL(dflt_icon_file) = STRDUP(p);
		else
			OLD_VAL(dflt_icon_file) =
				STRDUP(dflt_dflticonfile[wip->class_type]);

		if (wip->class_type == FileType)
		{
			if (p = DmGetObjProperty(op, OPENCMD, 0))
				OLD_VAL(open_cmd) = STRDUP(p);

			if (p = DmGetObjProperty(op, PROG_TO_RUN, NULL))
				OLD_VAL(prog_to_run) = STRDUP(p);

			if (p = DmGetObjProperty(op, PRINTCMD, 0))
				OLD_VAL(print_cmd) = STRDUP(p);

			if ((p = DmGetObjProperty(op, DISPLAY_IN_NEW, NULL)) &&
			  (*p == 'y' || *p == 'Y'))
				OLD_VAL(in_new) = 0;
			else
				OLD_VAL(in_new) = 1;

			if (p = DmGetObjProperty(op, TEMPLATE, NULL)) {
				OLD_VAL(tmpl_list) = DmParseList(p,
					&(OLD_VAL(num_tmpl)));
			}
			if (p = DmGetObjProperty(op, PROG_TYPE, NULL)) {
				int i;

				for (i = 0; i < num_file_ptypes; i++) {
					if (!strcmp(p, FileProgTypes[i])) {
						OLD_VAL(prog_type) = i;
						break;
					}
				}
			}
		} else if (wip->class_type == AppType)
		{
			if (p = DmGetObjProperty(op, OPENCMD, 0))
				OLD_VAL(open_cmd) = STRDUP(p);

			if (p = DmGetObjProperty(op, PROG_TYPE, NULL)) {
				int i;

				for (i = 0; i < num_app_ptypes; i++) {
					if (!strcmp(p, AppProgTypes[i])) {
						OLD_VAL(prog_type) = i;
						break;
					}
				}
			}
			if (p = DmGetObjProperty(op, DROPCMD, NULL)) {
				OLD_VAL(drop_cmd) = STRDUP(p);
				if (strstr(p, "s*") || strstr(p, "S*"))
					OLD_VAL(load_multi) = 1;
			}

		}
	} else {
		OLD_VAL(class_name) = NULL;
		OLD_VAL(prog_type)  = 0;
		OLD_VAL(label)      = NULL;
		OLD_VAL(pattern)    = NULL;
		OLD_VAL(lpattern)   = NULL;
		OLD_VAL(filepath)   = NULL;
		OLD_VAL(lfilepath)  = NULL;

		wip->class_type = class_type;

		OLD_VAL(icon_file) = STRDUP(dflt_iconfile[class_type]);
		OLD_VAL(dflt_icon_file) =STRDUP(dflt_dflticonfile[class_type]);

		if (class_type == FileType) {
			OLD_VAL(open_cmd)    = STRDUP(dflt_open_cmd[0][0]);
			OLD_VAL(print_cmd)   = STRDUP(dflt_print_cmd);
			OLD_VAL(in_new)      = 0;
			OLD_VAL(prog_to_run) = NULL;
			OLD_VAL(tmpl_name)   = NULL;
			OLD_VAL(tmpl_list)   = NULL;
			OLD_VAL(num_tmpl)    = 0;
		} else if (class_type == AppType) {
			OLD_VAL(to_wb)      = 1;
			OLD_VAL(load_multi) = 0;
			OLD_VAL(open_cmd)   = STRDUP(dflt_open_cmd[2][0]);
			OLD_VAL(drop_cmd)   = STRDUP(dflt_drop_cmd[0][0]);
		}
	}
	DmISCopySettings(wip, OldToNew);

} /* end of DmISInitClassWin */

/****************************procedure*header*****************************
 * DmISIconStubDropCB - dropProc for icon stub in a New or Properties window.
 * Sets up transfer list and starts the transfer.
 */
void
DmISIconStubDropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int n;
	Arg args[5];
	XmDropProcCallback DropData = (XmDropProcCallback)call_data;

	/* set the transfer resources */
	n = 0;

	/* If the action is not Drop or the operation is not Copy,
	 * cancel the drop.
	 */
	if ((DropData->dropAction != XmDROP) ||
			(DropData->operation != XmDROP_COPY))
	{
		XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
		XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
	} else {
		WinInfoPtr wip;
		XmDropTransferEntryRec transferEntries[1];

		XtSetArg(Dm__arg[0], XmNuserData, &wip);
		XtGetValues(w, Dm__arg, 1);

		/* The drop can continue.  Establish the transfer list and
		 * start the transfer.
		 */ 
		transferEntries[0].target = 
	 		OL_XA_FILE_NAME(DESKTOP_DISPLAY(Desktop));
		transferEntries[0].client_data = (XtPointer)wip;

		XtSetArg(args[n], XmNdropTransfers, transferEntries); n++;
		XtSetArg(args[n], XmNnumDropTransfers, 1); n++;
		XtSetArg(args[n], XmNtransferProc, IconStubTransferCB); n++;
	}
	XmDropTransferStart(DropData->dragContext, args, n);

} /* end of DmISIconStubDropCB */

/****************************procedure*header*****************************
 * IconStubTransferCB - transferProc for icon stub in a New or Properties
 * window to get the name of the icon file and pixmap to use.
 */
static void
IconStubTransferCB(Widget w, XtPointer closure, Atom *seltype, Atom *type,
	XtPointer value, unsigned long *length, int format)
{
	extern char *dirname(char *);
	extern char *basename(char *);
	DmObjectPtr op;
	DmContainerPtr cp;
	DmGlyphPtr gp;
	char *path;
	char *p;
	WinInfoPtr wip = (WinInfoPtr)closure;

	if ((*type != OL_XA_FILE_NAME(DESKTOP_DISPLAY(Desktop)) &&
	*type != XA_STRING) || *length == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "IconStubTransferCB(): unexpected target \"
			"received\n");
#endif
		return;
	}
	path = STRDUP(value);
	p = dirname(path);

	/* get icon file of file */
	if ((cp = DtGetData(NULL, DM_CACHE_FOLDER, (void *)p, strlen(p)+1))) {
		op = DmGetObjectInContainer(cp, basename(value));
		FREE(path);
		FREE(value);
	} else {
		FREE(path);
		FREE(value);
		return;
	}

	/* set icon stub to the pixmap of dnd source */
	DmISCreatePixmapFromBitmap(wip->w[W_IconStub], &wip->p[P_IconStub],
				   op->fcp->glyph);
	XtSetArg(Dm__arg[0], XmNbackgroundPixmap, wip->p[P_IconStub]);
	XtSetArg(Dm__arg[1], XmNwidth,            op->fcp->glyph->width+4);
	XtSetArg(Dm__arg[2], XmNheight,           op->fcp->glyph->height+4);
	XtSetValues(wip->w[W_IconStub], Dm__arg, 3);

	/* update Icon File textfield and icon_file setting */
	XtFree(NEW_VAL(icon_file));
	NEW_VAL(icon_file) = STRDUP(op->fcp->glyph->path);

	SetInputGizmoText(wip->g[G_IconFile], NEW_VAL(icon_file));

} /* end of IconStubTransferCB */

/****************************procedure*header*****************************
 * IconFileGizmoPopdnCB -  Popdown callback for Template path finder.
 */
static void
IconFileGizmoPopdnCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip = (WinInfoPtr)client_data;

	FreeGizmo(FileGizmoClass, wip->iconFileGizmo);
	wip->iconFileGizmo = NULL;
	XtDestroyWidget(w);

} /* end of IconFileGizmoPopdnCB */

/****************************procedure*header*****************************
 * TmplFileGizmoPopdnCB -  Popdown callback for Template path finder.
 */
static void
TmplFileGizmoPopdnCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	WinInfoPtr wip = (WinInfoPtr)client_data;

	FreeGizmo(FileGizmoClass, wip->tmplFileGizmo);
	wip->tmplFileGizmo = NULL;
	XtDestroyWidget(w);

} /* end of TmplFileGizmoPopdnCB */

/****************************procedure*header*****************************
 * DmISTmplFindCB - Callback for Find button in Templates page to display
 * a path finder for template files.
 */
void
DmISTmplFindCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	if (wip->tmplFileGizmo == NULL) {
		char *tmpldir = DmDTProp("TEMPLATEDIR", NULL);

		tmplFindMenu.clientData = (XtPointer)wip;
		tmplFileGizmo.directory =
			(base_fwp->attrs & DM_B_SYSTEM_CLASSES ?
			(tmpldir ? tmpldir : DESKTOP_HOME(Desktop)) :
			DESKTOP_HOME(Desktop));
		wip->tmplFileGizmo = CreateGizmo(base_fwp->shell,
			FileGizmoClass, &tmplFileGizmo, NULL, 0);

		XtAddCallback(GetFileGizmoShell(wip->tmplFileGizmo),
			XtNpopdownCallback, TmplFileGizmoPopdnCB,
			(XtPointer)wip);

		/* register for context-sensitive help */
		DmRegContextSensitiveHelp(
			GetFileGizmoRowCol(wip->tmplFileGizmo),
			ICON_SETUP_HELP_ID(Desktop), ICON_SETUP_HELPFILE,
			ICON_SETUP_TEMPLATE_FIND_SECT);
	}
	MapGizmo(FileGizmoClass, wip->tmplFileGizmo);

} /* end of DmISTmplFindCB */

/****************************procedure*header*****************************
 * DmISIconFindMenuCB - Callback for menu in icon library's path finder for
 * icon files to set the icon library path, pop down the path finder and
 * to display help on it.
 */
void
DmISIconFindMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)cbs->clientData;
	Widget shell = GetFileGizmoShell(wip->iconFileGizmo);

	switch(cbs->index) {
	case 0: /* OK */
	{
		Gizmo giz;
		char *path;
		
		ExpandFileGizmoFilename(wip->iconFileGizmo);
		path = GetFilePath(wip->iconFileGizmo);

		if (path == NULL) {
			return;
		}
		giz = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
				GetGizmoGizmo, "otherIconPathInput");
		SetInputGizmoText(giz, path);
		DmISSwitchIconLibrary(wip, path);
		FREE(path);
		XtPopdown(shell);
	}
		break;
	case 1: /* Cancel */
		DmBusyWindow(wip->lib_fwp->shell, False);
		XtPopdown(shell);
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
		  NULL, ICON_SETUP_HELPFILE, ICON_SETUP_LIBRARY_FIND_SECT);
		break;
	}

} /* end of DmISIconFindMenuCB */

/****************************procedure*header*****************************
 * DmISOtherIconMenuCB - Callback for Update Listing and Find buttons in 
 * an icon library window.
 */
void
DmISOtherIconMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* Update Listing */
	{
		Gizmo giz = QueryGizmo(PopupGizmoClass, wip->iconLibraryGizmo,
				GetGizmoGizmo, "otherIconPathInput");
		char *path = GetInputGizmoText(giz);

		if (!VALUE(path)) {
#ifdef DEBUG
			fprintf(stderr,"DmISOtherIconMenuCB(): no path \"
				"specified\n");
#endif
			XtFree(path);
			return;
		} else if (path[0] != '/') {
#ifdef DEBUG
			fprintf(stderr, "DmISOtherIconMenuCB(): a full path \"
				"is required\n");
#endif
			DmVaDisplayStatus((DmWinPtr)wip->lib_fwp, True,
				TXT_BAD_ICON_LIBRARY_PATH, path);
			XtFree(path);
			return;
		}
		DmISSwitchIconLibrary(wip, path);
		XtFree(path);
	}
		break;
	case 1: /* Find */
		if (wip->iconFileGizmo == NULL) {
			iconFindMenu.clientData = (XtPointer)wip;
			iconFileGizmo.directory = DESKTOP_HOME(Desktop);
			wip->iconFileGizmo = CreateGizmo(base_fwp->shell,
				FileGizmoClass, &iconFileGizmo, NULL, 0);

			XtAddCallback(GetFileGizmoShell(wip->iconFileGizmo),
				XtNpopdownCallback, IconFileGizmoPopdnCB,
				(XtPointer)wip);

			/* register for context-sensitive help */
			DmRegContextSensitiveHelp(
				GetFileGizmoRowCol(wip->iconFileGizmo),
				ICON_SETUP_HELP_ID(Desktop),
				ICON_SETUP_HELPFILE,
				ICON_SETUP_LIBRARY_FIND_SECT);
		}
		MapGizmo(FileGizmoClass, wip->iconFileGizmo);
		break;
	}

} /* end of DmISOtherIconMenuCB */

/****************************procedure*header*****************************
 * DmISTemplateMenuCB - Callback for menu in Template page to add, modify
 * or delete a template.
 */
void
DmISTemplateMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	switch(cbs->index) {
	case TemplateAdd:
		AddTemplate(wip);
		break;
	case TemplateModify:
		ModifyTemplate(wip);
		break;
	case TemplateDelete:
		DeleteTemplate(wip);
		break;
	}

} /* end of DmISTemplateMenuCB */

/****************************procedure*header*****************************
 * AddTemplate - Checks if specified template name is valid.  If so, check
 * if it is already in the list.  If not, add it to the template list.
 */
static void
AddTemplate(WinInfoPtr wip)
{
	int nitems;
	XmString item;
	XmString *items;
	Widget w;
	char *name = GetInputGizmoText(wip->g[G_TmplName]);

	if (!VALUE(name)) {
#ifdef DEBUG
		fprintf(stderr,"AddTemplate(): No template name specified\n");
#endif
		XtFree(name);
		return;
	} else if (strchr(name, ',')) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			TXT_COMMA_IN_TEMPLATE_NAME);
		XtFree(name);
		return;
	}

	w = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListGizmo");

	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtSetArg(Dm__arg[1], XmNitemCount, &nitems);
	XtGetValues(w, Dm__arg, 2);

	item = XmStringCreateLocalized(name);

	if (nitems) {
		int i;

		/* check for duplicate */
		for (i = 0; i < nitems; i++) {
			if (XmStringCompare(items[i], item)) {
				DmVaDisplayStatus((DmWinPtr)base_fwp, True,
					TXT_TEMPLATE_ALREADY_EXISTS, name);
				XmStringFree(item);
				XtFree(name);
				return;
			}
		}
	}
	XmListAddItemUnselected(w, item, nitems+1);
	XmStringFree(item);
	XtFree(name);

	/* clear textfield */
	SetInputGizmoText(wip->g[G_TmplName], "");

} /* end of AddTemplate */

/****************************procedure*header*****************************
 * ModifyTemplate - Checks if specified template name is valid.  If so,
 * find out which list item is selected and replace it with the new name. 
 */
static void
ModifyTemplate(WinInfoPtr wip)
{
	int i;
	int *pos;
	int cnt;
	Widget w;
	XmString items[1];
	char *name = GetInputGizmoText(wip->g[G_TmplName]);

	if (!VALUE(name)) {
#ifdef DEBUG
		fprintf(stderr,"ModifyTemplate(): No new template name \"
			"specified\n");
#endif
		XtFree(name);
		return;
	}
	w = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListGizmo");

	/* find out which template is selected */
	if (!XmListGetSelectedPos(w, &pos, &cnt)) {
#ifdef DEBUG
		fprintf(stderr,"ModifyTemplate(): no template selected\n");
#endif
		return;
	}
	items[0] = XmStringCreateLocalized(name);
	XtFree(name);

	XmListReplaceItemsPos(w, items, 1, pos[0]);
	XmStringFree(items[0]);

	/* deactivate Add, Modify and Delete buttons */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	for (i = 0; i < 3; i++) {
		char buf[PATH_MAX];

		sprintf(buf, "tmplListMenu:%d", i);
		w = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, buf);
		XtSetValues(w, Dm__arg, 1);
	}
	/* clear text field */
	SetInputGizmoText(wip->g[G_TmplName], "");

} /* end of ModifyTemplate */

/****************************procedure*header*****************************
 * DeleteTemplate - Remove selected item in template list.
 */
static void
DeleteTemplate(WinInfoPtr wip)
{
	int i;
	int *pos;
	int cnt;
	Widget w;

	w = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListGizmo");

	/* find out which template is selected */
	if (!XmListGetSelectedPos(w, &pos, &cnt)) {
#ifdef DEBUG
		fprintf(stderr,"DeleteTemplate(): no template selected\n");
#endif
		return;
	}
	XmListDeletePos(w, pos[0]);

	/* deactivate Add, Modify and Delete buttons */
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	for (i = 0; i < 3; i++) {
		char buf[PATH_MAX];

		sprintf(buf, "tmplListMenu:%d", i);
		w = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, buf);
		XtSetValues(w, Dm__arg, 1);
	}
	/* clear text field */
	SetInputGizmoText(wip->g[G_TmplName], "");

} /* end of DeleteTemplate */

/****************************procedure*header*****************************
 * DmISTmplFindMenuCB - Callback for menu in template path finder to set the
 * template file name textfield to the selected template, dismiss the
 * path finder and to display help on the path finder.
 */
void
DmISTmplFindMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)cbs->clientData;
	Widget shell = GetFileGizmoShell(wip->tmplFileGizmo);

	switch(cbs->index) {
	case 0: /* OK */
	{
		char *file;
		
		ExpandFileGizmoFilename(wip->tmplFileGizmo);
		file = GetFilePath(wip->tmplFileGizmo);

		if (file == NULL)
			return;

		SetInputGizmoText(wip->g[G_TmplName], file);
		FREE(file);
		XtPopdown(shell);
	}
		break;
	case 1: /* Cancel */
		XtPopdown(shell);
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
		  NULL, ICON_SETUP_HELPFILE, ICON_SETUP_TEMPLATE_FIND_SECT);
		break;
	}

} /* end of DmISTmplFindMenuCB */
/****************************procedure*header*****************************
 * DmISTmplListSelectCB - defaultActionProc for template list.  Sets template
 * file name textfield to selected template.
 */
void
DmISTmplListSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget btn;
	WinInfoPtr wip = (WinInfoPtr)client_data;
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;

	/* Display name of selected template in textfield. */
	SET_INPUT_GIZMO_TEXT(wip->g[G_TmplName], GET_TEXT(cbs->item));

	/* deactivate Add button - it would have been activated as a result
	 * of the above call.
	 */
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListMenu:0");
	XtSetArg(Dm__arg[0], XmNsensitive, False);
	XtSetValues(btn, Dm__arg, 1);

	/* activate Delete and Modify buttons */
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListMenu:1");
	XtSetArg(Dm__arg[0], XmNsensitive, True);
	XtSetValues(btn, Dm__arg, 1);
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		"tmplListMenu:2");
	XtSetValues(btn, Dm__arg, 1);

} /* end of DmISTmplListSelectCB */

/****************************procedure*header*****************************
 * DmISExtractClass - Extracts all classes that are either personal or system,
 * depending on attrs.
 */
void
DmISExtractClass(DtAttrs attrs)
{
	DmFmodeKeyPtr fmkp;
	DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

	if (attrs & DM_B_SYSTEM_CLASSES) {
		for (; fnkp; fnkp = fnkp->next)
		{
			if (fnkp->attrs & (DM_B_CLASSFILE | DM_B_OVERRIDDEN
			  | DM_B_NEW | DM_B_DELETED | DM_B_PERSONAL_CLASS))
				continue;

			fmkp = (DmFmodeKeyPtr)fnkp;
			AddClass(base_fwp->shell, base_fwp->views[0].cp,
				(char *)fmkp->name, fmkp->fcp, True);
		}
	} else if (attrs & DM_B_PERSONAL_CLASSES) {
		for (; fnkp; fnkp = fnkp->next)
		{
			if (fnkp->attrs & (DM_B_CLASSFILE | DM_B_OVERRIDDEN
			  | DM_B_NEW | DM_B_DELETED | DM_B_SYSTEM_CLASS))
				continue;

			fmkp = (DmFmodeKeyPtr)fnkp;
			AddClass(base_fwp->shell, base_fwp->views[0].cp,
				(char *)fmkp->name, fmkp->fcp, True);
		}
	}

} /* end of DmISExtractClass */

/****************************procedure*header*****************************
 * SyncClassList - Called after a class has been added, deleted or changed
 * to update the in-core file class database and on disk, and to re-type
 * any objects that are affected by the change.
 */
void
SyncClassList()
{
#define CHANGED_CLASS	(DM_B_NEW | DM_B_REPLACED | DM_B_DELETED)

	int i;
	int idx;
	char *p;
	DmItemPtr ip;
	DmFnameKeyPtr save_fnkp;
	DmFnameKeyPtr fnkp      = DESKTOP_FNKP(Desktop);
	DmFnameKeyPtr fnkp_list = fnkp;
	DmFnameKeyPtr del_fnkp  = NULL;

	/*
	 * Before processing changes in the class list, make sure the
	 * affected class file header entries are flagged with DM_B_WRITE_FILE.
	 */
	for (; fnkp; fnkp = fnkp->next)
		if ((fnkp->attrs & DM_B_CLASSFILE) && fnkp->next) {
			unsigned short ilevel; /* initial file level */
			Boolean changed = False;

			/* check if any entries in this file have changed */
			for (save_fnkp=fnkp->next, ilevel=save_fnkp->level;
			     save_fnkp; save_fnkp=save_fnkp->next) {
				if (save_fnkp->level > ilevel)
					continue;
				if (save_fnkp->level < ilevel)
					break;
				if (save_fnkp->attrs & CHANGED_CLASS) {
					changed = True;
					break;
				}
			}
			if (changed == True)
				fnkp->attrs |= DM_B_WRITE_FILE;
		}

	/*
	 * Process the new list:
	 * NEW entries - turn off attributes.
	 * REPLACED entries - turn off attributes, set all the borrowed
	 *		      resources from the next deleted entries to NULL.
	 * DELETED entries - free resources and remove it from the list.
	 */
	fnkp = fnkp_list;
	while (fnkp) {
		if (fnkp->attrs & DM_B_REPLACED) {
			/* The next fnkp MUST be the one being replaced. */
			save_fnkp = fnkp->next;
/*
 * Can't change the original list, another window (like "New" or "Find") may
 * be using it.
 */
#ifdef NOT_USE
			if (!(fnkp->attrs & DM_B_NEW_NAME))
				save_fnkp->name = NULL;

			if (!(fnkp->attrs & DM_B_VISUAL))
				save_fnkp->fcp->glyph = NULL;
#endif

			/* turn off bits so that it can used */
			fnkp->attrs &= ~(DM_B_NEW | DM_B_NEW_NAME |
					 DM_B_VISUAL | DM_B_TYPING);

			fnkp = fnkp->next;
		}
		else if (fnkp->attrs & DM_B_NEW) {
			/* turn off bits so that it can used */
			fnkp->attrs &= ~DM_B_NEW;

			/* turn on replaced bit, will be used later */
			fnkp->attrs |= DM_B_REPLACED;

			fnkp = fnkp->next;
		}
		else if (fnkp->attrs & DM_B_DELETED) {
			save_fnkp = fnkp->next;

			/* remove from the linked list and then free it */
			if (fnkp->prev)
				fnkp->prev->next = fnkp->next;
			if (fnkp->next)
				fnkp->next->prev = fnkp->prev;

			/* if it is the head of the list, update the head */
			if (fnkp == fnkp_list)
				fnkp_list = fnkp->next;

			/* put it in the deleted list */
			fnkp->next = del_fnkp;
			del_fnkp = fnkp;

			fnkp = save_fnkp;
		}
		else
			fnkp = fnkp->next;
	}

	/* install the new class list */
	DESKTOP_FNKP(Desktop) = fnkp_list;

	/* now do the dynamic update */
	DmSyncWindows(fnkp_list, del_fnkp);

	/* turn off the replaced bit */
	for (fnkp=fnkp_list; fnkp; fnkp=fnkp->next)
		fnkp->attrs &= ~DM_B_REPLACED;

#ifdef NOT_USE
	/* remove the deleted entries from the class list and display it */
	for (ip=win->itp, i=0; i < win->cp->num_objs; i++, ip++) {
		if (ITEM_MANAGED(ip) == False)
			continue;

		if (ITEM_CLASS(ip)->attrs & DM_B_DELETED)
			ip->managed = (XtArgVal)False;
	}
#endif

	/* free the deleted list */
	for (fnkp=del_fnkp; fnkp;) {
		save_fnkp = fnkp->next;
		DmFreeFileClass(fnkp);
		fnkp = save_fnkp;
	}
	DmWriteFileClassDBList(fnkp_list);

} /* end of SyncClassList */

/****************************procedure*header*****************************
 * DmISInsertClass - Inserts a new class either before a selected class or
 * to the beginning of the class list.
 */
int
DmISInsertClass(WinInfoPtr wip)
{
	int i;
	int idx;
	int ret;
	DmItemPtr ip;
	DmItemPtr itp;
	DmObjectPtr op;
	DmFclassPtr  new_fcp;
	DmFnameKeyPtr fnkp;
	DmFnameKeyPtr new_fnkp;
	XmString str;
	Boolean selected = False;

	if (((new_fnkp = (DmFnameKeyPtr)CALLOC(1, sizeof(DmFnameKeyRec)))
		== NULL) || ((new_fcp = DmNewFileClass(new_fnkp)) == NULL)) {
		/* print error message */
		return(-1);
	}
	/* set up the new fnkp structure */
	new_fnkp->name  = STRDUP(NEW_VAL(class_name));
	new_fnkp->attrs = DM_B_NEW | DM_B_NEW_NAME | DM_B_VISUAL | DM_B_TYPING;

	if (base_fwp->attrs & DM_B_PERSONAL_CLASSES)
		new_fnkp->attrs |= DM_B_PERSONAL_CLASS;
	else
		new_fnkp->attrs |= DM_B_SYSTEM_CLASS;

	if (wip->class_type == FileType) {
		new_fnkp->attrs |= DM_B_PROG_TO_RUN | DM_B_PROG_TYPE
				| DM_B_IN_NEW | DM_B_TEMPLATE
				| DM_B_ICON_ACTIONS;
	} else if (wip->class_type == AppType) {
		new_fnkp->attrs |= DM_B_PROG_TYPE |
				DM_B_TO_WB | DM_B_ICON_ACTIONS;
	}

	new_fnkp->fcp    = new_fcp;
	new_fcp->key     = (void *)new_fnkp;

	SetClassProperties(&(new_fcp->plist), wip, new_fnkp->attrs);
	DmInitFileClass(new_fnkp);

	XtSetArg(Dm__arg[0], XmNlastSelectItem, &idx);
	XtGetValues(base_fwp->views[0].box, Dm__arg, 1);

	/* The new class should have the same level as the selected item.
	   If no item selected, set level to that of the 1st item.
	 */
	if (idx == ExmNO_ITEM) {
		/* find first managed item in the list */
		for (i = 0, ip = base_fwp->views[0].itp;
			i < base_fwp->views[0].nitems; i++, ip++)
		{
			if (ITEM_MANAGED(ip)) {
				idx = i;
				break;
			}
		}
		if (idx == ExmNO_ITEM) {
			/* no managed item at all */
			idx = 0;
		}
	} else {
		selected = True;
	}

	ip = base_fwp->views[0].itp + idx;

	fnkp = ITEM_CLASS(ip);
	new_fnkp->level = fnkp->level;
	
	/* insert the new class to the fnkp list */
	if (fnkp->prev)
		fnkp->prev->next = new_fnkp;
	new_fnkp->next = fnkp;
	new_fnkp->prev = fnkp->prev;
	fnkp->prev     = new_fnkp;

	/* create an object for the new class */
	if ((op = Dm__NewObject(base_fwp->views[0].cp,new_fnkp->name)) == NULL)
	{
		/* print error message */
		return(-1);
	}
	op->fcp = new_fcp;

	/* unselect the selected class, if any, before the class list is
	   realloced
	 */
	ip->select = (XtArgVal)False;

	/* expand items array */
	itp = (DmItemPtr)REALLOC(base_fwp->views[0].itp,
		sizeof(DmItemRec) * (base_fwp->views[0].nitems + 1));

	if (itp == NULL) {
		/* attempt to recover from the error */
		base_fwp->views[0].cp->num_objs--;
		if (selected) {
			XtSetArg(Dm__arg[0], XmNset, True);
			ExmFlatSetValues(base_fwp->views[0].box,
				(ip - base_fwp->views[0].itp), Dm__arg, 1);
		}
		new_fnkp->attrs = DM_B_DELETED;
		/* print error message */
		return(-1);
	}
	base_fwp->views[0].itp = itp;
	base_fwp->views[0].nitems++;

	/* make a hole in the array */
	memmove((void *)(itp + idx + 1), (void *)(itp + idx),
		sizeof(DmItemRec) * (base_fwp->views[0].nitems-idx-1));

	/* make ip point to new class */
	ip = base_fwp->views[0].itp + idx;

	MAKE_WRAPPED_LABEL(ip->label, FPART(base_fwp->views[0].box).font, op->name);

	ip->x           = (XtArgVal)op->x;
	ip->y           = (XtArgVal)op->y;
	ip->managed     = (XtArgVal)True;
	ip->select      = (XtArgVal)False;
	ip->sensitive   = (XtArgVal)True;
	ip->client_data = (XtArgVal)NULL;
	ip->object_ptr  = (XtArgVal)op;

	DmInitObjType(base_fwp->views[0].box, op);
	op->ftype = DM_FTYPE_DATA;

	DmSizeIcon(base_fwp->views[0].box, ip);

	if (selected) {
		/* make item selected again */
		(ip+1)->select = (XtArgVal)True;
	}

	DmTouchIconBox((DmWinPtr)base_fwp, NULL, 0);
	DmISAlignIcons(base_fwp);
	SyncClassList();
	return(0);

} /* end of DmISInsertClass */

/****************************procedure*header*****************************
 * DmISModifyClass - Apply changes to an existing class, if all settings are
 * valid.  The in-core file class database is updated and written out to
 * disk.
 */
int
DmISModifyClass(WinInfoPtr wip)
{
	int i;
	char *p;
	DtAttrs change = DM_B_NEW;
	DmFclassPtr fcp;
	DmFclassPtr new_fcp;
	DmFnameKeyPtr fnkp;
	DmFnameKeyPtr new_fnkp;
	DmItemPtr ip;

	/* CAUTION: can't assume last selected item is the item to operate on
	 * as the item list may have been touch since the property sheet was
	 * popped up due to additions and/or deletions.
	 */ 
	for (i = 0, ip = base_fwp->views[0].itp; i < base_fwp->views[0].nitems;
		i++,ip++)
	{
		if (ITEM_MANAGED(ip)) {
			if (!strcmp(ITEM_OBJ(ip)->name, OLD_VAL(class_name)))
				break;
		}

	}
	if (i == base_fwp->views[0].nitems) {
#ifdef DEBUG
		fprintf(stderr,"DmISModifyClass(): Couldn't find item to \"
			"apply changes to\n");
#endif
		return(-1);
	}
	fcp = ITEM_OBJ(ip)->fcp;
	fnkp = (DmFnameKeyPtr)(fcp->key);

	/* check for class name change */
	if (strcmp(NEW_VAL(class_name), fnkp->name)) {
		FREE(fnkp->name);
		fnkp->name = STRDUP(NEW_VAL(class_name));
		change |= DM_B_NEW_NAME;
	}

	if (CheckPatternAndPath(wip) == -1)
		return(-1);

	/* check for changes that affect icon visuals */
	if (NEW_VAL(icon_file))
		if (OLD_VAL(icon_file) == NULL ||
			strcmp(NEW_VAL(icon_file), OLD_VAL(icon_file)))
			change |= DM_B_VISUAL;
	else if (OLD_VAL(icon_file))
		change |= DM_B_VISUAL;


	/* check for changes that affect file typing */
	if (NEW_VAL(pattern))
		if (OLD_VAL(pattern) == NULL || strcmp(NEW_VAL(pattern),
		OLD_VAL(pattern)))
			change |= DM_B_TYPING;
	else if (OLD_VAL(pattern))
		change |= DM_B_TYPING;

	if (NEW_VAL(lpattern))
		if (OLD_VAL(lpattern) == NULL || strcmp(NEW_VAL(lpattern),
		OLD_VAL(lpattern)))
			change |= DM_B_TYPING;
	else if (OLD_VAL(lpattern))
		change |= DM_B_TYPING;

	if (NEW_VAL(filepath))
		if (OLD_VAL(filepath) == NULL || strcmp(NEW_VAL(filepath),
		OLD_VAL(filepath)))
			change |= DM_B_TYPING;
	else if (OLD_VAL(filepath))
		change |= DM_B_TYPING;

	if (NEW_VAL(lfilepath))
		if (OLD_VAL(lfilepath) == NULL ||
			strcmp(NEW_VAL(lfilepath), OLD_VAL(lfilepath)))
			change |= DM_B_TYPING;
	else if (OLD_VAL(lfilepath))
		change |= DM_B_TYPING;

	if (NEW_VAL(open_cmd))
		if (OLD_VAL(open_cmd) == NULL || strcmp(NEW_VAL(open_cmd),
		OLD_VAL(open_cmd)))
			change |= DM_B_ICON_ACTIONS;
	else if (OLD_VAL(open_cmd))
		change |= DM_B_ICON_ACTIONS;

	if (OLD_VAL(to_wb) != NEW_VAL(to_wb))
		change |= DM_B_TO_WB;

	if (wip->class_type == FileType || wip->class_type == AppType)
	{
		if (OLD_VAL(prog_type) != NEW_VAL(prog_type))
			change |= DM_B_PROG_TYPE;
	}

	if (wip->class_type == FileType)
	{
		change |= DM_B_TEMPLATE;
		DmISUpdateTemplates(wip);

		if (NEW_VAL(prog_to_run))
			if (OLD_VAL(prog_to_run) == NULL ||
			  strcmp(NEW_VAL(prog_to_run), OLD_VAL(prog_to_run)))
				change |= DM_B_PROG_TO_RUN;
		else if (OLD_VAL(prog_to_run))
			change |= DM_B_PROG_TO_RUN;

		if (OLD_VAL(in_new) != NEW_VAL(in_new))
			change |= DM_B_IN_NEW;

		if (NEW_VAL(print_cmd))
			if (OLD_VAL(print_cmd) == NULL ||
			  strcmp(NEW_VAL(print_cmd), OLD_VAL(print_cmd)))
				change |= DM_B_ICON_ACTIONS;
		else if (OLD_VAL(print_cmd))
			change |= DM_B_ICON_ACTIONS;

	} else { /* AppType */
		if (NEW_VAL(drop_cmd))
			if (OLD_VAL(drop_cmd) == NULL ||
			  strcmp(NEW_VAL(drop_cmd), OLD_VAL(drop_cmd)))
				change |= DM_B_ICON_ACTIONS;
		else if (OLD_VAL(drop_cmd))
			change |= DM_B_ICON_ACTIONS;
	}

	if (fnkp->attrs & DM_B_NEW) {
		/* entry has already been modified */
		new_fnkp = fnkp;
		new_fcp = fnkp->fcp;
	} else {
		/* create a copy of the original entry */
		change |= DM_B_REPLACED;

		/* mark old entry for deletion */
		fnkp->attrs |= DM_B_DELETED;

		/* create a new entry */
		if (((new_fnkp = (DmFnameKeyPtr)CALLOC(1,
			sizeof(DmFnameKeyRec))) == NULL) ||
			((new_fcp = DmNewFileClass(new_fnkp)) == NULL))
		{
			return(-1);
		}
	}
	new_fcp->attrs |= (base_fwp->attrs &
		DM_B_PERSONAL_CLASS ? DM_B_PERSONAL_CLASS : DM_B_SYSTEM_CLASS);

	if (!(new_fnkp->attrs & DM_B_NEW)) {
		/* copy from original entry and insert the new entry in front
		 * of the old one.
	 	 */
		*new_fnkp = *fnkp;
		if (change & DM_B_NEW_NAME)
			new_fnkp->name = STRDUP(NEW_VAL(class_name));
		else
			new_fnkp->name = STRDUP(fnkp->name);
		new_fnkp->next = fnkp;

		if (fnkp->prev)
			fnkp->prev->next = new_fnkp;
		fnkp->prev = new_fnkp;

		new_fnkp->fcp  = new_fcp;
		new_fcp->key   = (void *)new_fnkp;
		new_fcp->glyph = fcp->glyph;
		new_fcp->glyph->count++;
	
		(void)DtCopyPropertyList(&(new_fcp->plist), &(fcp->plist));
		ITEM_OBJ(ip)->fcp = new_fcp;
	} else {
		if (new_fnkp->re) {
			FREE(new_fnkp->re);
			new_fnkp->re = NULL;
		}
		if (new_fnkp->lre) {
			FREE(new_fnkp->lre);
			new_fnkp->lre = NULL;
		}
		if (new_fnkp->lfpath) {
			FREE(new_fnkp->lfpath);
			new_fnkp->lfpath = NULL;
		}
	}
	SetClassProperties(&(new_fcp->plist), wip, change);
	new_fnkp->attrs &= ~(DM_B_REGEXP | DM_B_FILEPATH | DM_B_LFILEPATH |
				DM_B_LREGEXP | DM_B_DELETED);

	DmInitFileClass(new_fnkp);

	if (change & DM_B_NEW_NAME &&
		strcmp(ITEM_OBJ(ip)->name, NEW_VAL(class_name)))
	{
		FREE(ITEM_OBJ(ip)->name);
		ITEM_OBJ(ip)->name = STRDUP(NEW_VAL(class_name));
		FREE_LABEL((_XmString)ip->label);
		MAKE_WRAPPED_LABEL(ip->label,
			FPART(base_fwp->views[0].box).font,
			NEW_VAL(class_name));
		DmSizeIcon(base_fwp->views[0].box, ip);
		DmTouchIconBox((DmWinPtr)base_fwp, NULL, 0);
	}
	if (change & DM_B_VISUAL) {
		ChangeItemGlyph(ip);
	}
	new_fnkp->attrs |= change;
	SyncClassList();
	DmISAlignIcons(base_fwp);
	return(0);

} /* end of DmISModifyClass */

/****************************procedure*header*****************************
 * ValidateClassName - Checks if class name specified by the user is NULL
 * or is already being used by an existing class.
 */
static int
ValidateClassName(char *name, Boolean new_class)
{
	char *p;
	DmFnameKeyPtr fnkp;

	if (name == NULL) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True, TXT_NO_CLASS_NAME);
		return(-1);
	}

	/* check if class already exists */
	for (fnkp = DESKTOP_FNKP(Desktop); fnkp; fnkp = fnkp->next) {
		if (fnkp->attrs & DM_B_CLASSFILE ||
			fnkp->attrs & DM_B_DELETED ||
			fnkp->attrs & DM_B_IGNORE_CLASS)
			continue;
		if (!strcmp(fnkp->name, name) && new_class)
		{
			DmVaDisplayStatus((DmWinPtr)base_fwp, True,
				TXT_CLASS_ALREADY_EXISTS, name);
			return(-1);
		}
	}
	return(0);

} /* end of ValidateClassName */

/****************************procedure*header*****************************
 * real_string - Checks if the input string is NULL or is an empty string.
 * Returns the string if it is not; otherwise, returns NULL.
 */
char *
real_string(char *str)
{
	if (!str || (*str == '\0') || is_empty_str(str))
		return(NULL);
	else
		return(str);

} /* end of real_string */

/****************************procedure*header*****************************
 * is_empty_string - Skips white space in input string and returns True
 * if the string is empty; False, otherwise.
 */
static int
is_empty_str(char *str)
{
	while (*str && ((*str == ' ' || *str == '\t'))) ++str;
	return(!((int)*str));

} /* end of is_empty_str */

/****************************procedure*header*****************************
 * SetClassProperties - Sets the properties of a class.  Called when a class
 * is added or changed.  Only properties which are changed are updated.
 */
static void
SetClassProperties(DtPropListPtr plistp, WinInfoPtr wip, DtAttrs change)
{
	if (change & DM_B_TO_WB)
		DtSetProperty(plistp, SYSTEM, NEW_VAL(to_wb) ? "Y" : NULL,0);

	if (change & DM_B_VISUAL) {
		DtSetProperty(plistp, ICONFILE, NEW_VAL(icon_file), 0);

		/* No need for DFLTICONFILE if not a variable icon file */
		if (VAR_ICON_FILE(NEW_VAL(icon_file)) &&
		  NEW_VAL(dflt_icon_file))
			DtSetProperty(plistp, DFLTICONFILE,
				NEW_VAL(dflt_icon_file), 0);
	}
	if (change & DM_B_TYPING) {
		DtSetProperty(plistp, PATTERN, NEW_VAL(pattern), 0);
		DtSetProperty(plistp, LPATTERN, NEW_VAL(lpattern), 0);
		DtSetProperty(plistp, FILEPATH, NEW_VAL(filepath), 0);
		DtSetProperty(plistp, LFILEPATH, NEW_VAL(lfilepath),0);
		/* Don't set FILETYPE if it was never set. */
		if (!(wip->no_ftype))
			DtSetProperty(plistp, FILETYPE,
				TypeNames[wip->class_type], 0);
	}

	if (change & DM_B_PROG_TYPE) {
		DtSetProperty(plistp, PROG_TYPE, (wip->class_type == FileType ?
			FileProgTypes[NEW_VAL(prog_type)] :
			AppProgTypes[NEW_VAL(prog_type)]), 0);
	}

	if (change & DM_B_ICON_ACTIONS) {
		/* update action properties */
		DtSetProperty(plistp, OPENCMD, NEW_VAL(open_cmd),
			DT_PROP_ATTR_MENU);
		if (wip->class_type == AppType) {
			DtSetProperty(plistp, DROPCMD, NEW_VAL(drop_cmd), 0);
		} else if (wip->class_type == FileType) {
			DtSetProperty(plistp, PRINTCMD, NEW_VAL(print_cmd),
				DT_PROP_ATTR_MENU);
		}
	}

	if (wip->class_type == FileType) {
		if (change & DM_B_PROG_TO_RUN) {
			DtSetProperty(plistp, PROG_TO_RUN,
				NEW_VAL(prog_to_run), 0);
		}

		if (change & DM_B_TEMPLATE) {
		int nitems;
		XmString *items;

		XtSetArg(Dm__arg[0], XmNitems, &items);
		XtSetArg(Dm__arg[1], XmNitemCount, &nitems);
		XtGetValues(wip->w[W_List], Dm__arg, 2);

		if (nitems) {
			int i;
			int len = 0;
			char *s;
			char **tp;

			tp = (char **)MALLOC(sizeof(char *)*nitems);

			for (i = 0; i < nitems; i++) {
				tp[i] = GET_TEXT(items[i]);
				len += strlen(tp[i]);
			}
			/* include ','s and NULL terminator */
			s = (char *)CALLOC((len+nitems), sizeof(char));
			for (i = 0; i < nitems - 1; i++) {
				strcat(s, tp[i]);
				strcat(s, ",");
			}
			strcat(s, tp[nitems-1]);
			DtSetProperty(plistp, TEMPLATE, s, 0);
			for (i = 0; i < nitems; i++)
				FREE(tp[i]);
			FREE(tp);
			FREE(s);
			} else
				DtSetProperty(plistp, TEMPLATE, NULL, 0);
		}
		if (NEW_VAL(lpattern) || NEW_VAL(lfilepath))
			DtSetProperty(plistp, DISPLAY_IN_NEW, NULL, 0);
		else if (change & DM_B_IN_NEW)
			DtSetProperty(plistp, DISPLAY_IN_NEW, NEW_VAL(in_new) ?
				NULL : "Y", 0);
	}

} /* end of SetClassProperties */

/****************************procedure*header*****************************
 * ChangeItemGlyph - Changes the glyph of an item.  Called from ModifyClass()
 * when the icon file setting is changed.
 */
static void
ChangeItemGlyph(DmItemPtr ip)
{
	DmObjectPtr op   = ITEM_OBJ(ip);
	Dimension width  = ITEM_WIDTH(ip);
	Dimension height = ITEM_HEIGHT(ip);

	if (op->fcp->glyph) {
		/* set glyph for this op to NULL in case the pixmap is still
		 * in use.
		 */
		op->fcp->glyph = NULL;
	}

	/* reset attributes */
	op->fcp->attrs &= ~(DM_B_VAR | DM_B_FREE);
	DmInitObjType(base_fwp->views[0].box, op);

	DmSizeIcon(base_fwp->views[0].box, ip);
	if ((width != ITEM_WIDTH(ip)) || (height != ITEM_HEIGHT(ip)))
	{
		DmISAlignIcons(base_fwp);
	} else {
		ExmFlatRefreshItem(base_fwp->views[0].box,
			ip - base_fwp->views[0].itp, True);
	}

} /* end of ChangeItemGlyph */

/****************************procedure*header*****************************
 * CheckPatternAndPath - Validates settings for PATTERN, LPATTERN, FILEPATH
 * and LFILEPATH.
 */
static int
CheckPatternAndPath(WinInfoPtr wip)
{
	/* FILEPATH and LFILEPATH are mutually exclusive */
	if (NEW_VAL(filepath) && NEW_VAL(lfilepath)) {
			DmVaDisplayStatus((DmWinPtr)base_fwp, True,
				TXT_FILEPATH_N_LFILEPATH);
		return(-1);
	}
	/* PATTERN and LPATTERN are mutually exclusive */
	if (NEW_VAL(pattern) && NEW_VAL(lpattern)) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			(wip->class_type == FileType ? TXT_PATTERN_N_LPATTERN
			: wip->class_type == AppType ?
			TXT_APP_PATTERN_N_LPATTERN :
			TXT_FDLR_PATTERN_N_LPATTERN));
		return(-1);
	}
	/* PATTERN and LFILEPATH are mutually exclusive */
	if (NEW_VAL(pattern)  && NEW_VAL(lfilepath)) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			(wip->class_type == FileType ? TXT_PATTERN_N_LFILEPATH
			: wip->class_type == AppType ?
			TXT_APP_PATTERN_N_LFILEPATH :
			TXT_FDLR_PATTERN_N_LFILEPATH));
		return(-1);
	}
	/* LPATTERN and FILEPATH are mutually exclusive */
	if (NEW_VAL(lpattern) && NEW_VAL(filepath)) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			TXT_LPATTERN_N_FILEPATH);
		return(-1);
	}
	if (wip->class_type == DirType) {
		if (!NEW_VAL(pattern) && !NEW_VAL(lpattern)
			&& !NEW_VAL(filepath) && !NEW_VAL(lfilepath))
		{
			DmVaDisplayStatus((DmWinPtr)base_fwp, True,
				TXT_NO_PATTERN_OR_FILEPATH);
			return(-1);
		}
	} else if (wip->class_type == AppType) {
		if (!NEW_VAL(pattern) && !NEW_VAL(lpattern))
		{
			if (!NEW_VAL(lfilepath)) {
				DmVaDisplayStatus((DmWinPtr)base_fwp, True,
					TXT_NO_PROGRAM_NAME);
				return(-1);
			}
		}
	}
	return(0);

} /* end of CheckPatternAndPath */

/****************************procedure*header*****************************
 * GetNewValue -
 */
static void
GetNewValue(char **value, Gizmo gizmo, Boolean inputGizmo)
{
	char *input;

	if (*value)
		FREE(*value);

	input = inputGizmo ? GetInputGizmoText(gizmo):GetTextGizmoText(gizmo);
	*value = (VALUE(input) ? STRDUP(input) : NULL);
	XtFree(input);

} /* end of GetNewValue */

/****************************procedure*header*****************************
 * DmISValidateInputs - Gets current settings and calls validation routines.
 */
int
DmISValidateInputs(WinInfoPtr wip)
{
	char *tmp;

	GetNewValue(&NEW_VAL(class_name), wip->g[G_ClassName], True);

	if (ValidateClassName(NEW_VAL(class_name), wip->new_class) == -1)
		return(-1);

	GetNewValue(&NEW_VAL(pattern), wip->g[G_Pattern], True);
	GetNewValue(&NEW_VAL(lpattern), wip->g[G_LPattern], True);
	GetNewValue(&NEW_VAL(filepath), wip->g[G_FilePath], True);
	GetNewValue(&NEW_VAL(lfilepath), wip->g[G_LFilePath], True);

	if (CheckPatternAndPath(wip))
		return(-1);

	tmp = GetInputGizmoText(wip->g[G_IconFile]);

	if (!VALUE(tmp)) {
		DmVaDisplayStatus((DmWinPtr)base_fwp, True, TXT_NO_ICON_FILE);
		XtFree(tmp);
		return(-1);
	} else if (!VAR_ICON_FILE(tmp) && ValidateIconFile(tmp))
	{
		DmVaDisplayStatus((DmWinPtr)base_fwp, True,
			TXT_INVALID_ICON_FILE, tmp);
		XtFree(tmp);
		return(-1);
	}
	if (NEW_VAL(icon_file))
		FREE(NEW_VAL(icon_file));
	NEW_VAL(icon_file) = STRDUP(tmp);
	XtFree(tmp);

	if (wip->class_type == FileType) {
		GetNewValue(&NEW_VAL(prog_to_run), wip->g[G_ProgToRun], True);
		if (!NEW_VAL(prog_to_run) && !(wip->no_ftype))
		{
			DmVaDisplayStatus((DmWinPtr)base_fwp, True,
				TXT_NO_PROGRAM_TO_RUN);
			return(-1);
		}
		GetNewValue(&NEW_VAL(print_cmd), wip->g[G_Print], False);

		ManipulateGizmo(ChoiceGizmoClass, (Gizmo)wip->g[G_WbNew],
			GetGizmoValue);

		NEW_VAL(in_new) = (int)QueryGizmo(ChoiceGizmoClass,
			wip->g[G_WbNew], GetGizmoCurrentValue, NULL);
	} else if (wip->class_type == AppType) {
		ManipulateGizmo(ChoiceGizmoClass, (Gizmo)wip->g[G_WbNew],
			GetGizmoValue);
		NEW_VAL(to_wb) = (int)QueryGizmo(ChoiceGizmoClass,
			wip->g[G_WbNew], GetGizmoCurrentValue, NULL);

		GetNewValue(&NEW_VAL(drop_cmd), wip->g[G_Drop], False);
	}
	if (wip->class_type == FileType || wip->class_type == AppType)
	{
		GetNewValue(&NEW_VAL(open_cmd), wip->g[G_Open], False);

		ManipulateGizmo(ChoiceGizmoClass, (Gizmo)wip->g[G_ProgType],
			GetGizmoValue);
		NEW_VAL(prog_type) = (int)QueryGizmo(ChoiceGizmoClass,
			wip->g[G_ProgType], GetGizmoCurrentValue, NULL);
	}
	return(0);

} /* end of DmISValidateInputs */

/****************************procedure*header*****************************
 * ValidateIconFile - Validates icon file setting to make sure it exists
 * and is a valid icon file. 
 */
static int
ValidateIconFile(char *icon_file)
{
	DmGlyphPtr gp = DmGetPixmap(DESKTOP_SCREEN(Desktop), icon_file);

	return((gp == NULL || gp->path == NULL) ? -1 : 0);

} /* end of ValidateIconFile */

/****************************procedure*header*****************************
 * AddClass - Called from DmISExtractClass() to add an object for a class
 * to the container cp.
 */
static void
AddClass(Widget parent, DmContainerPtr cp, char *name, DmFclassPtr fcp,
	Boolean viewable)
{
	DmObjectPtr op;

	if (!name)
		name = DmClassName(fcp);

	op = Dm__NewObject(cp, name);
	op->fcp = fcp;
	if (!viewable) {
		op->attrs |= DM_B_HIDDEN;
	} else {
		DmInitObjType(parent, op);
		/* Set file type to DATA so that we can take advantage of
		 * certain routines.
		 */
		op->ftype = DM_FTYPE_DATA;
	}

} /* end of AddClass */

/****************************procedure*header*****************************
 * DmISAlignIcons - Called when the Icon Setup window is resized or when the
 * an item is added or unmanaged, and after items are labeled.
 */
void
DmISAlignIcons(DmFolderWinPtr fwp)
{
	Dimension wrap_width;

	XtSetArg(Dm__arg[0], XmNwidth, &wrap_width);
	XtGetValues(fwp->views[0].box, Dm__arg, 1);

	DmComputeLayout(fwp->views[0].box, fwp->views[0].itp,
		fwp->views[0].nitems, DM_ICONIC, wrap_width,
		DM_B_CALC_SIZE, 0);
	DmTouchIconBox((DmWinPtr)fwp, NULL, 0);

} /* end of DmISAlignIcons */

/****************************procedure*header*****************************
 * DmISDeleteClass - Delete class referenced by item ptr ip by marking it
 * as deleted and unmanaging the item.
 */
void
DmISDeleteClass(DmItemPtr ip)
{
	char *p;
	DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

	/* Find and delete selected class. */
	for (; fnkp; fnkp = fnkp->next) {
		if (fnkp->attrs & DM_B_CLASSFILE ||
			fnkp->attrs & DM_B_DELETED ||
			fnkp->attrs & DM_B_IGNORE_CLASS)
			continue;

		p = DtGetProperty(&(fnkp->fcp->plist),CLASS_NAME, NULL);
		/* CLASSNAME could be a catalogue reference. */
		if (!strcmp(fnkp->name, ITEM_OBJ(ip)->name) || (p &&
			!strcmp(GGT(p),ITEM_OBJ(ip)->name)))
		{
			fnkp->attrs |= DM_B_DELETED | DM_B_IGNORE_CLASS;
			XtSetArg(Dm__arg[0], XmNmanaged, False);
			XtSetArg(Dm__arg[1], XmNset, False);
			ExmFlatSetValues(base_fwp->views[0].box,
				(ip - base_fwp->views[0].itp), Dm__arg, 2);

			/* don't decrement base_fwp->views[0].nitems since
			 * items are unmanaged, not removed.
			 */
			DmTouchIconBox((DmWinPtr)base_fwp, NULL, 0);

			/* remove object from container */
			RemoveObject(ITEM_OBJ(ip));

			DmISAlignIcons(base_fwp);
			SyncClassList();
			/* Deactivate Delete and Properties buttons in Class
			 * menu.
			 */
			DmISUnSelectCB(NULL, NULL, NULL);
			return;
		}
	}
} /* end of DmISDeleteClass  */

/****************************procedure*header*****************************
 * DmISUnSelectCB - unSelectProc for flat icon box.  Make Delete and Properties
 * buttons in Icon Class menu insensitive.
 */
void
DmISUnSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget btn;

	/* clear footer */
	SetBaseWindowLeftMsg(base_fwp->gizmo_shell, " ");

	XtSetArg(Dm__arg[0], XmNsensitive, False);

	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:1");
	XtSetValues(btn, Dm__arg, 1);

	btn = QueryGizmo(BaseWindowGizmoClass, base_fwp->gizmo_shell,
		GetGizmoWidget, "ClassMenu:3");
	XtSetValues(btn, Dm__arg, 1);

} /* end of DmISUnSelectCB */

/****************************procedure*header*****************************
 * DmISDblSelectCB - dblSelectProc for flat icon box.  Calls
 * DmISShowProperties() for class icon that was double-clicked on.
 */
void
DmISDblSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	ExmFIconBoxButtonCD *d = (ExmFIconBoxButtonCD *)call_data;

	DmISShowProperties(base_fwp->views[0].itp + d->item_data.item_index);

	/* set d->ok to False so that the item remains busy */
	d->ok = False;

} /* end of DmISDblSelectCB */

/****************************procedure*header*****************************
 * DmISGetIDs - Gets and stores widget and gizmo ids of controls in wip
 * to avoid lots of calls to QueryGizmo().
 */
void
DmISGetIDs(WinInfoPtr wip, int type)
{
	/* gizmos and widgets for all three types */

	wip->w[W_Shell] = GetPopupGizmoShell(wip->popupGizmo);

	wip->p[P_IconStub] = NULL;
	wip->p[P_IconPixmap] = NULL;

	wip->g[G_ClassName] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "classNameInput");

	wip->w[W_ClassName] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "classNameInput");

	wip->g[G_LPattern] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "lpatternInput");

	wip->w[W_LPattern] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "lpatternInput");

	wip->g[G_FilePath] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "filePathInput");

	wip->w[W_FilePath] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "filePathInput");

	wip->g[G_LFilePath] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "lfilePathInput");

	wip->w[W_LFilePath] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "lfilePathInput");

	wip->g[G_IconFile] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "iconFileInput");

	wip->w[W_IconFile] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "iconFileInput");

	wip->g[G_IconStub] = (Gizmo)QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoGizmo, "iconPixmapGizmo");

	wip->w[W_IconStub] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "iconPixmapGizmo");

	wip->w[W_IconMenu] = QueryGizmo(PopupGizmoClass,
		wip->popupGizmo, GetGizmoWidget, "iconMenu");

	if (wip->new_class)
		wip->w[W_AddMenu] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "addMenu");
	else
		wip->w[W_PropMenu] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "propMenu");

	if (type == FileType || type == AppType) {
		/* widgets for File and Application type only  */
		wip->g[G_WbNew] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "yesNoChoice");

		wip->g[G_Open] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "openInput");

		wip->w[W_Open] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "openInput");

	}
	if (type == FileType) {
		/* widgets for File type only  */
		wip->g[G_ProgType] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "fileProgTypeChoice");

		wip->w[W_ProgType] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "fileProgTypeMenu");

		wip->w[W_Category] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "fileCategoryMenu");

		wip->g[G_Pattern] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "patternInput");

		wip->w[W_Pattern] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "patternInput");

		wip->g[G_ProgToRun] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "programInput");

		wip->w[W_ProgToRun] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "programInput");

		wip->g[G_Print] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "printInput");

		wip->w[W_Print] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "printInput");

		wip->w[W_List] = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "tmplListGizmo");

		wip->g[G_List] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "tmplListGizmo");

		wip->g[G_TmplName] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "tmplNameInput");

		wip->w[W_TmplName] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "tmplNameInput");

		wip->w[W_TmplMenu] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "tmplListMenu");

		wip->w[W_TmplFind] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "tmplFindBtnMenu");

		wip->w[W_InNew] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "yesNoMenu");

	} else if (type == AppType) {
		/* widgets for Application type only  */
		wip->g[G_ProgType] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "appProgTypeChoice");

		wip->w[W_ProgType] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "appProgTypeMenu");

		wip->w[W_Category] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "appCategoryMenu");

		wip->w[W_ToWB] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "yesNoMenu");

		wip->g[G_Pattern] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "programNameInput");

		wip->w[W_Pattern] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "programNameInput");

		wip->g[G_Drop] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "dropInput");

		wip->w[W_Drop] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "dropInput");

		wip->w[W_LoadMulti] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "loadMultiMenu");
	} else if (type == DirType) {
		wip->g[G_Pattern] = (Gizmo)QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoGizmo, "folderNameInput");

		wip->w[W_Pattern] = QueryGizmo(PopupGizmoClass,
			wip->popupGizmo, GetGizmoWidget, "folderNameInput");
	} 

} /* end of DmISGetIDs */

/****************************procedure*header*****************************
 * DmISFreeSettings - Frees settings in stored in wip.
 */
void
DmISFreeSettings(WinInfoPtr wip, Boolean flag)
{
	switch(flag) {
	case Old:
		XtFree(OLD_VAL(class_name));
		XtFree(OLD_VAL(pattern));
		XtFree(OLD_VAL(lpattern));
		XtFree(OLD_VAL(filepath));
		XtFree(OLD_VAL(lfilepath));
		XtFree(OLD_VAL(icon_file));
		XtFree(OLD_VAL(label));
		XtFree(OLD_VAL(dflt_icon_file));

		if (wip->class_type == FileType)
		{
			XtFree(OLD_VAL(open_cmd));
			XtFree(OLD_VAL(prog_to_run));
			XtFree(OLD_VAL(print_cmd));
			FreeTemplates(wip, Old);
		} else if (wip->class_type == AppType)
		{
			XtFree(OLD_VAL(open_cmd));
			XtFree(OLD_VAL(drop_cmd));
		}
		break;
	case New:
		XtFree(NEW_VAL(class_name));
		XtFree(NEW_VAL(pattern));
		XtFree(NEW_VAL(lpattern));
		XtFree(NEW_VAL(filepath));
		XtFree(NEW_VAL(lfilepath));
		XtFree(NEW_VAL(icon_file));
		XtFree(NEW_VAL(label));
		XtFree(NEW_VAL(dflt_icon_file));

		if (wip->class_type == FileType)
		{
			XtFree(NEW_VAL(open_cmd));
			XtFree(NEW_VAL(prog_to_run));
			XtFree(NEW_VAL(print_cmd));
			FreeTemplates(wip, New);
		} else if (wip->class_type == AppType)
		{
			XtFree(NEW_VAL(open_cmd));
			XtFree(NEW_VAL(drop_cmd));
		}
		break;
	}

} /* end of DmISFreeSettings */

/****************************procedure*header*****************************
 * DmISCopySettings - Copies old settings to new settings, or vice-versa.
 */
void
DmISCopySettings(WinInfoPtr wip, int flag)
{
#define MYSTRDUP(s) (s ? STRDUP(s) : NULL)

	if (flag == NewToOld) {
		OLD_VAL(class_name)     = MYSTRDUP(NEW_VAL(class_name));
		OLD_VAL(pattern)        = MYSTRDUP(NEW_VAL(pattern));
		OLD_VAL(prog_to_run)    = MYSTRDUP(NEW_VAL(prog_to_run));
		OLD_VAL(label)          = MYSTRDUP(NEW_VAL(label));
		OLD_VAL(open_cmd)       = MYSTRDUP(NEW_VAL(open_cmd));
		OLD_VAL(drop_cmd)       = MYSTRDUP(NEW_VAL(drop_cmd));
		OLD_VAL(print_cmd)      = MYSTRDUP(NEW_VAL(print_cmd));
		OLD_VAL(icon_file)      = MYSTRDUP(NEW_VAL(icon_file));
		OLD_VAL(dflt_icon_file) = MYSTRDUP(NEW_VAL(dflt_icon_file));
		OLD_VAL(filepath)       = MYSTRDUP(NEW_VAL(filepath));
		OLD_VAL(lpattern)       = MYSTRDUP(NEW_VAL(lpattern));
		OLD_VAL(lfilepath)      = MYSTRDUP(NEW_VAL(lfilepath));
		OLD_VAL(num_tmpl)       = NEW_VAL(num_tmpl);
		OLD_VAL(prog_type)      = NEW_VAL(prog_type);
		OLD_VAL(in_new)         = NEW_VAL(in_new);
		OLD_VAL(to_wb)          = NEW_VAL(to_wb);
		OLD_VAL(load_multi)     = NEW_VAL(load_multi);

		if (wip->class_type == FileType && NEW_VAL(num_tmpl))
		{
			int i;

			OLD_VAL(tmpl_list) = (char **)CALLOC(NEW_VAL(num_tmpl),
				sizeof(char *));

			for (i = 0; i < NEW_VAL(num_tmpl); i++)
				OLD_VAL(tmpl_list)[i] =
					STRDUP(NEW_VAL(tmpl_list)[i]);
		} else {
			OLD_VAL(tmpl_list) = NULL;
		}
	} else {
		NEW_VAL(class_name)     = MYSTRDUP(OLD_VAL(class_name));
		NEW_VAL(pattern)        = MYSTRDUP(OLD_VAL(pattern));
		NEW_VAL(prog_to_run)    = MYSTRDUP(OLD_VAL(prog_to_run));
		NEW_VAL(label)          = MYSTRDUP(OLD_VAL(label));
		NEW_VAL(open_cmd)       = MYSTRDUP(OLD_VAL(open_cmd));
		NEW_VAL(drop_cmd)       = MYSTRDUP(OLD_VAL(drop_cmd));
		NEW_VAL(print_cmd)      = MYSTRDUP(OLD_VAL(print_cmd));
		NEW_VAL(icon_file)      = MYSTRDUP(OLD_VAL(icon_file));
		NEW_VAL(dflt_icon_file) = MYSTRDUP(OLD_VAL(dflt_icon_file));
		NEW_VAL(filepath)       = MYSTRDUP(OLD_VAL(filepath));
		NEW_VAL(lpattern)       = MYSTRDUP(OLD_VAL(lpattern));
		NEW_VAL(lfilepath)      = MYSTRDUP(OLD_VAL(lfilepath));
		NEW_VAL(num_tmpl)       = OLD_VAL(num_tmpl);
		NEW_VAL(prog_type)      = OLD_VAL(prog_type);
		NEW_VAL(in_new)         = OLD_VAL(in_new);
		NEW_VAL(to_wb)          = OLD_VAL(to_wb);
		NEW_VAL(load_multi)     = OLD_VAL(load_multi);

		if (wip->class_type == FileType && OLD_VAL(num_tmpl))
		{
			int i;

			NEW_VAL(tmpl_list) = (char **)CALLOC(OLD_VAL(num_tmpl),
						sizeof(char *));

			for (i = 0; i < OLD_VAL(num_tmpl); i++)
				NEW_VAL(tmpl_list)[i] =
					STRDUP(OLD_VAL(tmpl_list)[i]);
		} else {
			NEW_VAL(tmpl_list) = NULL;
		}
	}
#undef MYSTRDUP

} /* end of DmISCopySettings */

/****************************procedure*header*****************************
 * FreeTemplates - Frees templates in wip.
 */
static void
FreeTemplates(WinInfoPtr wip, int which)
{
	switch(which) {
	case Old:
		if (OLD_VAL(tmpl_list)) {
			for (; OLD_VAL(num_tmpl); OLD_VAL(num_tmpl)--)
				FREE(OLD_VAL(tmpl_list)[OLD_VAL(num_tmpl)-1]);
			FREE(OLD_VAL(tmpl_list));
			OLD_VAL(tmpl_list) = NULL;
			OLD_VAL(num_tmpl)  = 0;
		}
		break;
	case New:
		if (NEW_VAL(tmpl_list)) {
			for (; NEW_VAL(num_tmpl); NEW_VAL(num_tmpl)--)
				FREE(NEW_VAL(tmpl_list)[NEW_VAL(num_tmpl)-1]);
			FREE(NEW_VAL(tmpl_list));
			NEW_VAL(tmpl_list) = NULL;
			NEW_VAL(num_tmpl)  = 0;
		}
		break;
	}

} /* end of FreeTemplates */

/****************************procedure*header*****************************
 * DmISUpdateTemplates - Saves count and name of items in template list
 * in wip.
 */
void
DmISUpdateTemplates(WinInfoPtr wip)
{
	int i;
	int nitems;
	XmString *items;
	char **tmpl_list;

	FreeTemplates(wip, New);

	XtSetArg(Dm__arg[0], XmNitems, &items);
	XtSetArg(Dm__arg[1], XmNitemCount, &nitems);
	XtGetValues(wip->w[W_List], Dm__arg, 2);

	if (nitems == 0) {
		NEW_VAL(tmpl_list) = NULL;
		NEW_VAL(num_tmpl) = 0;
		return;
	}
	tmpl_list = (char **)MALLOC(sizeof(char *) * nitems);
	for (i = 0; i < nitems; i++)
		tmpl_list[i] = STRDUP(GET_TEXT(items[i]));

	NEW_VAL(tmpl_list) = tmpl_list;
	NEW_VAL(num_tmpl)  = nitems;

} /* end of DmISUpdateTemplates */

/****************************procedure*header*****************************
 * DmISResetTemplateList - Resets template list to currently saved values.
 */
void
DmISResetTemplateList(WinInfoPtr wip)
{
	FreeTemplates(wip, New);
	if (OLD_VAL(tmpl_list)) {
		int i;
		XmString *items;

		items =
		  (XmString *)MALLOC(sizeof(XmString)*OLD_VAL(num_tmpl));
		NEW_VAL(tmpl_list) = (char **)MALLOC(sizeof(char *) *
				OLD_VAL(num_tmpl) + 1);
		for (i = 0; i < OLD_VAL(num_tmpl); i++) {
			NEW_VAL(tmpl_list)[i] = STRDUP(OLD_VAL(tmpl_list)[i]);
			items[i] =
				XmStringCreateLocalized(OLD_VAL(tmpl_list)[i]);
		}
		NEW_VAL(num_tmpl) = OLD_VAL(num_tmpl);

		XtSetArg(Dm__arg[0], XmNitems, items);
		XtSetArg(Dm__arg[1], XmNitemCount, NEW_VAL(num_tmpl));
		XtSetValues(wip->w[W_List], Dm__arg, 2);

		for (i = 0; i < NEW_VAL(num_tmpl); i++)
			XmStringFree(items[i]);
	}

} /* end of DmISResetTemplateList */

/****************************procedure*header*****************************
 * DmISResetValues - Reset settings to previously saved values.
 */
void
DmISResetValues(WinInfoPtr wip)
{
	int val;
	Widget w1, w2;

	/* clear left footer */
	SetPopupWindowLeftMsg(wip->popupGizmo, " ");

	SET_INPUT_GIZMO_TEXT(wip->g[G_ClassName], OLD_VAL(class_name));
	SET_INPUT_GIZMO_TEXT(wip->g[G_IconFile], OLD_VAL(icon_file));
	SET_INPUT_GIZMO_TEXT(wip->g[G_Pattern], OLD_VAL(pattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LPattern], OLD_VAL(lpattern));
	SET_INPUT_GIZMO_TEXT(wip->g[G_FilePath], OLD_VAL(filepath));
	SET_INPUT_GIZMO_TEXT(wip->g[G_LFilePath], OLD_VAL(lfilepath));

	(void)DmISShowIcon(wip);

	if (wip->class_type == DirType)
		return;

	if (wip->class_type == FileType) {
		DmISResetYesNoMenu(wip, OLD_VAL(in_new));
		DmISResetProgTypeMenu(wip, "fileProgTypeMenu",
			OLD_VAL(prog_type), num_file_ptypes);

		SET_INPUT_GIZMO_TEXT(wip->g[G_ProgToRun],OLD_VAL(prog_to_run));
		SET_INPUT_GIZMO_TEXT(wip->g[G_TmplName], OLD_VAL(tmpl_name));
		SetTextGizmoText(wip->g[G_Open], OLD_VAL(open_cmd));
		SetTextGizmoText(wip->g[G_Print], OLD_VAL(print_cmd));

		XmListDeleteAllItems(wip->w[W_List]);
		if (!(wip->new_class))
			DmISResetTemplateList(wip);
	} else { /* AppType */
		w1 = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, "loadMultiMenu:0");
		XtSetArg(Dm__arg[0], XmNset, OLD_VAL(load_multi));
		XtSetValues(w1, Dm__arg, 1);

		DmISResetYesNoMenu(wip, OLD_VAL(to_wb));
		DmISResetProgTypeMenu(wip, "appProgTypeMenu",
			OLD_VAL(prog_type), num_app_ptypes);

		SetTextGizmoText(wip->g[G_Open], OLD_VAL(open_cmd));
		SetTextGizmoText(wip->g[G_Drop], OLD_VAL(drop_cmd));
	}

} /* end of DmISResetValues */

/****************************procedure*header*****************************
 * FocusEventHandler - Focus event handler to display a one-liner help on
 * the widget with focus.   This is used for the New and Properties windows.
 */
static void
FocusEventHandler(Widget w, XtPointer client_data, XEvent *xevent,
	Boolean *continue_to_use)
{
	char *msg = NULL;
	WinInfoPtr wip = (WinInfoPtr)client_data;

	if (xevent->type != FocusIn)
		return;

	if (w == wip->w[W_ClassName]) {
		if (wip->class_type == FileType)
			msg = TXT_CLASS_NAME_FIELD;
		else if (wip->class_type == AppType)
			msg = TXT_APP_CLASS_NAME_FIELD;
		else
			msg = TXT_DIR_CLASS_NAME_FIELD;
	} else if (w == wip->w[W_Pattern] || w == wip->w[W_LPattern]) {
		if (wip->class_type == AppType)
			msg = TXT_APP_PATTERN_FIELD;
		else
			msg = TXT_PATTERN_FIELD;
	} else if (w == wip->w[W_FilePath] || w == wip->w[W_LFilePath]) {
		if (wip->class_type == FileType)
			msg = TXT_FILE_FILEPATH_FIELD;
		else if (wip->class_type == AppType)
			msg = TXT_APP_FILEPATH_FIELD;
		else
			msg = TXT_DIR_FILEPATH_FIELD;
	} else if (w == wip->w[W_IconFile])
		msg = TXT_ICON_FILE_FIELD;
	else if (w == wip->w[W_IconStub])
		msg = TXT_ICON_FILE_FIELD;
	else if (w == wip->w[W_IconMenu])
		msg = TXT_CLASS_ICON_MENU;

	if (msg) {
		SetPopupWindowLeftMsg(wip->popupGizmo, GetGizmoText(msg));
		return;
	}
	if (wip->new_class) {
		if (w == wip->w[W_AddMenu]) {
			SetPopupWindowLeftMsg(wip->popupGizmo,
				GetGizmoText(TXT_CLASS_ADD_MENU));
			return;
		}
	} else {
		if (w == wip->w[W_PropMenu]) {
			SetPopupWindowLeftMsg(wip->popupGizmo,
				GetGizmoText(TXT_CLASS_PROP_MENU));
			return;
		}
	}

	if (wip->class_type == FileType || wip->class_type == AppType) {
		if (w == wip->w[W_Category]) {
			msg = TXT_CLASS_CATEGORY_MENU;
		} else if (w == wip->w[W_ProgType]) {
			msg = TXT_PROG_TYPE_MENU;
		}
	}
	if (msg) {
		SetPopupWindowLeftMsg(wip->popupGizmo, GetGizmoText(msg));
		return;
	}
	if (wip->class_type == AppType) {
		if (w == wip->w[W_ToWB])
			msg = TXT_MOVE_TO_WB_MENU;
		else if (w == wip->w[W_Drop])
			msg = TXT_DROP_CMD_FIELD;
		else if (w == wip->w[W_Open])
			msg = TXT_APP_OPEN_CMD_FIELD;
		else if (w == wip->w[W_LoadMulti])
			msg = TXT_APP_LOAD_MULTI;
	} else if (wip->class_type == FileType) {
		if (w == wip->w[W_Open])
			msg = TXT_FILE_OPEN_CMD_FIELD;
		else if (w == wip->w[W_ProgToRun])
			msg = TXT_PROG_TO_RUN_FIELD;
		else if (w == wip->w[W_InNew])
			msg = TXT_DISPLAY_IN_NEW_MENU;
		else if (w == wip->w[W_Print])
			msg = TXT_PRINT_CMD_FIELD;
		else if (w == wip->w[W_List]) {
			SetPopupWindowLeftMsg(wip->popupGizmo, " ");
			return;
		} else if (w == wip->w[W_TmplName])
			msg = TXT_TEMPLATE_NAME_FIELD;
		else if (w == wip->w[W_TmplMenu])
			msg = TXT_TEMPLATE_MENU;
		else if (w == wip->w[W_TmplFind])
			msg = TXT_TEMPLATE_FIND;
	}
	if (msg)
		SetPopupWindowLeftMsg(wip->popupGizmo, GetGizmoText(msg));

} /* end of FocusEventHandler */

/****************************procedure*header*****************************
 * DmISRegisterFocusEH - Registers an event handler for focus events.
 */
void
DmISRegisterFocusEH(WinInfoPtr wip)
{
	int i;

	XtAddEventHandler(wip->w[W_ClassName], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_Pattern], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_LPattern], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_FilePath], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_LFilePath], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_IconFile], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_IconStub], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);
	XtAddEventHandler(wip->w[W_IconMenu], (EventMask)FocusChangeMask,
		False, FocusEventHandler, (XtPointer)wip);

	if (wip->new_class)
		XtAddEventHandler(wip->w[W_AddMenu],(EventMask)FocusChangeMask,
			False, FocusEventHandler, (XtPointer)wip);
	else {
		XtAddEventHandler(wip->w[W_PropMenu],
			(EventMask)FocusChangeMask, False, FocusEventHandler,
			(XtPointer)wip);
	}

	if (wip->class_type == FileType || wip->class_type == AppType) {
		XtAddEventHandler(wip->w[W_Category],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_ProgType],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_Open],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
	}
	if (wip->class_type == FileType) {
		XtAddEventHandler(wip->w[W_ProgToRun],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_InNew],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_Print],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_List],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_TmplName],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_TmplMenu],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_TmplFind],
			(EventMask)FocusChangeMask, False,
			FocusEventHandler, (XtPointer)wip);
	} else if (wip->class_type == AppType) {
		XtAddEventHandler(wip->w[W_Drop], (EventMask)FocusChangeMask,
			False, FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_ToWB], (EventMask)FocusChangeMask,
			False, FocusEventHandler, (XtPointer)wip);
		XtAddEventHandler(wip->w[W_LoadMulti],
			(EventMask)FocusChangeMask, False, FocusEventHandler,
			(XtPointer)wip);
	}

} /* end of DmISRegisterFocusEH */

/****************************procedure*header*****************************
 * DmISCreatePixmapFromBitmap - copy supplied bitmap into a pixmap.
 */
void
DmISCreatePixmapFromBitmap(Widget w, Pixmap *pixmap, DmGlyphPtr gp)
{
	Display *		dpy = XtDisplay(w);
	static GC		gc = NULL;
	Pixel			p;
	XGCValues		values;

	if (*pixmap != NULL) {
		XFreePixmap(dpy, *pixmap);
	}
	*pixmap = XCreatePixmap(
		dpy, (Drawable)RootWindowOfScreen(XtScreen(w)),
		gp->width+4, gp->height+4, DefaultDepthOfScreen(XtScreen(w))
	);
	if (gc == NULL) {
		XtVaGetValues(w, XmNbackground, &p, NULL);
		values.background = p;
		gc = XCreateGC(dpy, *pixmap, GCBackground, &values);
	}
	XFillRectangle(dpy, *pixmap, gc, 0, 0, gp->width+4, gp->height+4);
	if (gp->depth == 1) {
		XCopyPlane(
			dpy, gp->pix, *pixmap, gc, 0, 0, gp->width, gp->height,
			2, 2, (unsigned long)1
		);
	}
	else {
		XCopyArea(
			dpy, gp->pix, *pixmap, gc, 0, 0, gp->width, gp->height,
			2, 2
		);
	}
}

/****************************procedure*header*****************************
 * DmISUpdateIconStub - Used in the New and Properties windows as well as the
 * Icon Library to update the icon stub when icon file is changed.  Only
 * display error if display_error is True.
 */
int
DmISUpdateIconStub(char *icon_file, Widget stub, DmFolderWindow fwp,
	Pixmap *pixmap, Boolean display_error)
{
	DmGlyphPtr gp = DmGetPixmap(DESKTOP_SCREEN(Desktop), icon_file);

	if (!gp) {
		if (display_error) {
			DmVaDisplayStatus((DmWinPtr)fwp, True,
				TXT_INVALID_ICON_FILE, icon_file);
		}
		return(-1);
	}
	DmISCreatePixmapFromBitmap(stub, pixmap, gp);

	XtSetArg(Dm__arg[0], XmNbackgroundPixmap, *pixmap);
	XtSetArg(Dm__arg[1], XmNwidth,            gp->width+4);
	XtSetArg(Dm__arg[2], XmNheight,           gp->height+4);
	XtSetValues(stub, Dm__arg, 3);
	return(0);

} /* end of DmISUpdateIconStub */

/****************************procedure*header*****************************
 * DmISIconLibraryMenuCB - Callback for icon library menu to set icon file
 * to the current selection in the icon library, dismiss the icon library
 * window and to display help on the window.
 */
void
DmISIconLibraryMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	WinInfoPtr wip = (WinInfoPtr)(cbs->clientData);

	switch(cbs->index) {
	case 0: /* OK */
	{
		int idx;
		DmObjectPtr op;

		XtSetArg(Dm__arg[0], XmNlastSelectItem, &idx);
		XtGetValues(wip->lib_fwp->views[0].box, Dm__arg, 1);

		if (idx == ExmNO_ITEM) {
#ifdef DEBUG
		  fprintf(stderr,"DmISIconLibraryMenuCB():no selected icon\n");
#endif
			return;
		}
		op = ITEM_OBJ(wip->lib_fwp->views[0].itp + idx);

		/* Update icon stub in New/Property window and new
		 * icon file setting.
		 */
		if (op->fcp->glyph->path == NULL ||
			DmISUpdateIconStub(op->fcp->glyph->path,
				wip->w[W_IconStub], base_fwp,
				&wip->p[P_IconStub], True) == -1)
		{
			return;
		} else {
			XtFree(NEW_VAL(icon_file));
			NEW_VAL(icon_file) = STRDUP(op->fcp->glyph->path);

			/* update Icon File Name textfield */
			SetInputGizmoText(wip->g[G_IconFile],
				NEW_VAL(icon_file));
			XtPopdown(wip->lib_fwp->shell);
		}
	}
		break;
	case 1: /* Cancel */
		XtPopdown(wip->lib_fwp->shell);
		break;
	case 2: /* Help */
		DmDisplayHelpSection(DmGetHelpApp(ICON_SETUP_HELP_ID(Desktop)),
		  NULL, ICON_SETUP_HELPFILE, ICON_SETUP_ICON_LIBRARY_SECT);
		break;
	}

} /* end of DmISIconLibraryMenuCB */

/****************************procedure*header*****************************
 * RemoveObject - Removes an object from the base window container.
 */
static void
RemoveObject(DmObjectPtr obj)
{
	DmObjectPtr op = base_fwp->views[0].cp->op;

	if (obj == op) {
		base_fwp->views[0].cp->op = obj->next;
		base_fwp->views[0].cp->num_objs--;
	} else {
		for (; op->next; op = op->next)
			if (obj == op->next) {
				op->next = obj->next;
				base_fwp->views[0].cp->num_objs--;
				break;
			}
	}
	Dm__FreeObject(obj);

} /* end of RemoveObject */

/****************************procedure*header*****************************
 * DmISResetYesNoMenu - Resets the Display In File:New or Can be moved to
 * the Wastebasket menu to specified selection.
 */
void
DmISResetYesNoMenu(WinInfoPtr wip, int new_idx)
{
	Widget btn;

	sprintf(Dm__buffer, "yesNoMenu:%d", new_idx);
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		Dm__buffer);
	XtSetArg(Dm__arg[0], XmNmenuHistory, btn);
	XtSetValues(wip->class_type == FileType ? wip->w[W_InNew] :
		wip->w[W_ToWB], Dm__arg, 1);

	XtSetArg(Dm__arg[0], XmNset, True);
	XtSetValues(btn, Dm__arg, 1);

	sprintf(Dm__buffer, "yesNoMenu:%d", new_idx == 0 ? 1 : 0);
	btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo, GetGizmoWidget,
		Dm__buffer);
	XtSetArg(Dm__arg[0], XmNset, False);
	XtSetValues(btn, Dm__arg, 1);

} /* end of DmISResetYesNoMenu */

/****************************procedure*header*****************************
 * DmISResetProgTypeMenu - Resets the Program Types menu to the specified
 * selection and updates the sensitivity of the menu buttons.
 */
void
DmISResetProgTypeMenu(WinInfoPtr wip, char *menu_name, int new_index,
	int num_btns)
{
	int i;
	Widget btn;
	char buf[64];

	for (i = 0; i < num_btns; i++) {
		/* Get button widget id */
		sprintf(buf, "%s:%d", menu_name, i);
		btn = QueryGizmo(PopupGizmoClass, wip->popupGizmo,
			GetGizmoWidget, buf);
		if (i == new_index) {
			Widget menu = QueryGizmo(PopupGizmoClass,
				wip->popupGizmo, GetGizmoWidget, menu_name);
			XtSetArg(Dm__arg[0], XmNmenuHistory, btn);
			XtSetValues(menu, Dm__arg, 1);
			XtSetArg(Dm__arg[0], XmNsensitive, False);
			XtSetValues(btn, Dm__arg, 1);
		} else {
			XtSetArg(Dm__arg[0], XmNsensitive, True);
			XtSetValues(btn, Dm__arg, 1);
		}
	}

} /* end of DmISResetProgTypeMenu */

