/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_new.c	1.79"


/******************************file*header********************************

    Description:
	This file contains the source code for the callback for the New
	button in a folder's File menu used to create new files.
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/ListGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/ContGizmo.h>

#include "Dtm.h"
#include "CListGizmo.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures
*/
					/* private procedures		*/
static void TmplCB(Widget, XtPointer, XtPointer);
static void CListCB(Widget, XtPointer, XtPointer);
static void CreateCB(Widget, XtPointer, XtPointer);
static int  CreateFile(CreateInfo *cip, Boolean open);
static void NewFileFMProc(DmProcReason reason, XtPointer client_data,
	XtPointer call_data, char * src, char * dst);
static int  VerifyTmpl(CreateInfo *cip);
static void UpdateTemplates(CreateInfo *cip, char *template);
static int  VerifyInputs(CreateInfo *cip);
static int  CreateDataFile(CreateInfo *cip, Boolean *success);
static int  CreateDir(CreateInfo *cip, Boolean *success);

					/* public procedures		*/

void DmPromptCreateFile(DmFolderWindow folder);

/*****************************file*variables******************************

	Define global/static variables and #defines, and
	Declare externally referenced variables
*/


/* defines for New popup */
#define DM_DONT_USE_TEMPLATE 1 /* either no template or none selected */
#define DM_USE_TEMPLATE      2 /* valid template selected             */
#define DM_CREATE_N_OPEN     3 /* open file after creating it         */

/* Gizmo declarations for New popup window */

static MenuItems createMenuItems[] = {
  { True, TXT_NEW_CREATE_AND_OPEN, TXT_M_NEW_CREATE_AND_OPEN,  I_PUSH_BUTTON, NULL,
	NULL, NULL, True
  },
  { True, TXT_NEW_CREATE,          TXT_M_CREATE, I_PUSH_BUTTON, NULL,
	NULL, NULL, False
  },
  { True, TXT_CANCEL,              TXT_M_CANCEL,     I_PUSH_BUTTON, NULL,
	NULL, NULL, False
  },
  { True, TXT_HELP,                TXT_M_HELP,       I_PUSH_BUTTON, NULL,
	NULL, NULL, False
  },
  { NULL }
};

static MenuGizmo createMenu = {
	NULL, "create_menu", "_X_", createMenuItems, CreateCB, NULL,
	XmHORIZONTAL, 1, 0, 2
};

static CListGizmo clist_gizmo = {
	NULL,		/* help */
	"clist",	/* widget name */
	2,		/* view width */
	DISPLAY_IN_NEW,	/* required property */
	False,		/* file */
	True,		/* sys class */
	False,		/* xenix class */
	True,		/* usr class */
	False,		/* overridden class */
	True,		/* exclusive behavior */
	False,		/* noneset behavior */
	CListCB,	/* select proc */
};

static GizmoRec label0_rec[] = {
	{ CListGizmoClass, &clist_gizmo },
};

static LabelGizmo label0_gizmo = {
	NULL,			/* help */
	"class",		/* widget name */
	TXT_NEW_FILE_TYPE,	/* caption label */
	False,			/* align caption */
	label0_rec,		/* gizmo array */
	XtNumber(label0_rec),	/* number of gizmos */
};

static ListGizmo template_gizmo = {
	NULL,		/* help info */
	"template",	/* name of widget */
	NULL,		/* items */
	0,		/* numItems */
	3,		/* number of items visible */
	NULL,		/* select callback */
	NULL,		/* clientData */
};

static Arg listArgs[] = {
	{ XmNscrollBarDisplayPolicy,    XmSTATIC },
	{ XmNlistSizePolicy,            XmCONSTANT },
};

static GizmoRec tmplContGizmoRec[] = {
	{ ListGizmoClass, &template_gizmo, listArgs, XtNumber(listArgs) },
};

static ContainerGizmo tmplContGizmo = {
	NULL,                       /* help */
	"tmplContGizmo",            /* widget name */
	G_CONTAINER_SW,             /* type */
	0,                          /* width */
	0,                          /* height */
	tmplContGizmoRec,           /* gizmos */
	XtNumber(tmplContGizmoRec), /* numGizmos */
};

static Arg swArgs[] = {
	{ XmNscrollingPolicy,        XmAPPLICATION_DEFINED },
	{ XmNvisualPolicy,           XmVARIABLE },
	{ XmNscrollBarDisplayPolicy, XmSTATIC },
};

static GizmoRec tmplLabelRec[] = {
	{ ContainerGizmoClass, &tmplContGizmo, swArgs, XtNumber(swArgs) },
};

static LabelGizmo label1_gizmo = {
	NULL,			/* help */
	"template_label",	/* widget name */
	TXT_NEW_TEMPLATE,	/* caption label */
	False,			/* align caption */
	tmplLabelRec,		/* gizmo array */
	XtNumber(tmplLabelRec),	/* number of gizmos */
};

static LabelGizmo pattern_gizmo_1 = {
	NULL,				/* help */
	"pattern_label",		/* widget name */
	TXT_FILE_EXT_OR_PATTERN,	/* caption label */
	False,				/* align caption */
	NULL,				/* gizmo array */
	0,				/* number of gizmos */
};

static LabelGizmo pattern_gizmo_2 = {
	NULL,			/* help */
	"pattern",		/* widget name */
	TXT_NONE,		/* caption label */
	False,			/* align caption */
	NULL,			/* gizmo array */
	0,			/* number of gizmos */
};

static GizmoRec rc_gizmos[] =  {
	{ LabelGizmoClass, &pattern_gizmo_1 },
	{ LabelGizmoClass, &pattern_gizmo_2 },
};
static ContainerGizmo rc_gizmo = {
	NULL,			/* help */
	"rowCol",		/* name */
	G_CONTAINER_RC,		/* container type */
	200,			/* width */
	30,			/* height */
	rc_gizmos,		/* gizmo array */
	XtNumber(rc_gizmos),	/* num gizmos */
};

static InputGizmo input_gizmo = {
	NULL, "name", "", 20, NULL, NULL
};

static GizmoRec fname_gizmos[] =  {
	{ InputGizmoClass, &input_gizmo },
};

static LabelGizmo fname_label = {
	NULL,			/* help */
	"fname_label",		/* widget name */
	TXT_FILENAME,		/* caption label */
	False,			/* align caption */
	fname_gizmos,		/* gizmo array */
	XtNumber(fname_gizmos),	/* number of gizmos */
};

static GizmoRec create_gizmo[] = {
	{ LabelGizmoClass,	&label0_gizmo },
	{ LabelGizmoClass,	&label1_gizmo },
	{ ContainerGizmoClass,	&rc_gizmo },
	{ LabelGizmoClass,	&fname_label },
};

static PopupGizmo create_popup = {
	NULL,			/* help */
	"create",		/* name of shell */
	TXT_NEW_TITLE,		/* window title */
	&createMenu,		/* pointer to menu info */
	create_gizmo,		/* gizmo array */
	XtNumber(create_gizmo),	/* number of gizmos */
};

typedef enum {
	CreateAndOpen,
	CreateOnly,
	CreateCancel,
	CreateHelp
} CreateFileMenuItemIndex;

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
	Sets default file name to name of selected template.
*/
static void
TmplCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	CreateInfo *cip = (CreateInfo *)client_data;
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;

	if (cip->template)
		FREE(cip->template);

	if (cbs->selected_item_count == 0) {
		cip->template = NULL;
#ifdef NOT_USE
		SetInputGizmoText(cip->inputG, "");
#endif
	} else {
		cip->template = GET_TEXT(cbs->item);
#ifdef NOT_USE
		SetInputGizmoText(cip->inputG, basename(cip->template));
#endif
	}

}				/* End of TmplCB */

/****************************procedure*header*****************************
	Updates the list of templates when a file type is selected.
*/
static void
CListCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget form;
	char *p;
	CreateInfo *cip = (CreateInfo *)client_data;
	ExmFIconBoxButtonCD *fcd = (ExmFIconBoxButtonCD *)call_data;
	DmItemPtr itp = ITEM_CD(fcd->item_data);
	DmFclassPtr fcp = FCLASS_PTR(itp);

	cip->itp = itp;

	/* free previous list items */
	XmListDeleteAllItems(cip->listW);

	form = QueryGizmo(PopupGizmoClass, cip->fwp->createWindow,
			GetGizmoWidget, "template_label");

	/* get value of TEMPLATE property of item */
	if (p = DtGetProperty(&(fcp->plist), TEMPLATE, NULL)) {
		XtSetSensitive(form, True);
		UpdateTemplates(cip, p);
	} else { /* no template */
#ifdef MOTIF_FOCUS
		Widget tf = QueryGizmo(PopupGizmoClass,
			cip->fwp->createWindow, GetGizmoWidget, "name");
		OlSetInputFocus(tf, RevertToNone, CurrentTime);
#endif
		if (cip->template) {
			FREE(cip->template);
			cip->template = NULL;
		}

		/* Make list widget empty and insensitive */
		XtSetSensitive(form, False);
	}
	/* Update file name extension/pattern */
	if (p = DtGetProperty(&(fcp->plist), PATTERN, NULL))
		XtSetArg(Dm__arg[0], XmNlabelString,
			XmStringCreateLocalized(p));
	else
		XtSetArg(Dm__arg[0], XmNlabelString,
			XmStringCreateLocalized(GetGizmoText(TXT_NONE)));
	XtSetValues(cip->patternW, Dm__arg, 1);

}				/* End of CListCB */

/****************************procedure*header*****************************
    CreateCB - callback for buttons in New popup window.
*/
static void
CreateCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget tf;
	MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
	CreateInfo *cip = (CreateInfo *)cbs->clientData;
	DmFolderWindow fwp = cip->fwp;

	tf = QueryGizmo(PopupGizmoClass, cip->fwp->createWindow,
			GetGizmoWidget, "name");

	switch(cbs->index) {
	case CreateAndOpen:
	case CreateOnly:
		/* False for not to open file after creation */
		if (CreateFile(cip, (cbs->index == CreateAndOpen))) {
			/* Must check if the window has already been destroyed
			 * because if Open-In-Place is in effect, it would have
			 * been destroyed in DmOpenInPlace() at this point.
			 */
			if (fwp->createWindow)
				DmCloseNewWindow(cip);
		} else {
#ifdef MOTIF_FOCUS
			OlSetInputFocus(tf, RevertToNone, CurrentTime);
#endif
		}
		break;

	case CreateCancel:
		DmCloseNewWindow(cip);
		break;

	case CreateHelp:
		{

		DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)),
			NULL, FOLDER_HELPFILE, FOLDER_FILE_NEW_SECT);
#ifdef NOT_USE
		OlSetInputFocus(tf, RevertToNone, CurrentTime);
#endif
		}
		break;
	}
}					/* End of CreateCB */


/****************************procedure*header*****************************
	CreateFile - callback for New operation to create a new file.
	open is used as a flag to open or not open the file after
	it's created.  cip->flag is used to indicate
	whether a template should be used.
*/
static int
CreateFile(CreateInfo *cip, Boolean open)
{
	char *fname;
	DmFileType ftype;
	DmFclassPtr fcp;
	Boolean success = False;
	int type = -1;

	/* Clear message area */
	DmClearStatus((DmWinPtr)cip->fwp);

	/* get file name */
	fname = GetInputGizmoText(cip->inputG);

	if ((fname == NULL) || (fname[0] == '\0')) {
		DmVaDisplayStatus((DmWinPtr)cip->fwp, True, TXT_NO_FILE_NAME);
		return(0);
	}
	if (cip->fname)
		FREE(cip->fname);
	cip->fname = fname; /* fname is malloc'ed in GetInputGizmoText() */

	if (VerifyInputs(cip) == -1) {
		return(0);
	}

	fcp = FCLASS_PTR(cip->itp);

	if (((DmFmodeKeyPtr)(fcp->key))->attrs & DM_B_SYS) {
		if (!strcmp(((DmFmodeKeyPtr)(fcp->key))->name, "DIR")) {
			type = 0;
		}else if (!strcmp(((DmFmodeKeyPtr)(fcp->key))->name, "DATA")) {
			type = 1;
		}
	} else if (!strcmp(((DmFnameKeyPtr)(fcp->key))->name, "DIR")) {
		type = 0;
	} else if (!strcmp(((DmFnameKeyPtr)(fcp->key))->name, "DATA") &&
	  cip->template == NULL) {
		type = 1;
	}

	if (cip->flag != DM_USE_TEMPLATE) {
		char *p;
		DmFnameKeyPtr fnkp = (DmFnameKeyPtr)(fcp->key);

		if (type == 0) { /* DIR */
			if (CreateDir(cip, &success) == -1)
				return(0);
		} else if (type == 1) { /* DATA */
			if (CreateDataFile(cip, &success) == -1)
				return(0);
		}
		else {
			/* find out file type and create file */
			p = DtGetProperty(&(fcp->plist), FILETYPE, NULL);
			if (!strcmp(p, "DATA")) {
				if (CreateDataFile(cip, &success) == -1)
					return(0);
			} else if (!strcmp(p, "DIR")) {
				if (CreateDir(cip, &success) == -1)
					return(0);
			} else {
				/* invalid file type */
				return(0);
			}
		}
	} else {
		DmFileOpInfoPtr opr_info;

		/* if open is True, set flag to DM_CREATE_N_OPEN */
		if (open)
			cip->flag = DM_CREATE_N_OPEN;

		/* load parameters into opr_info struct */
		opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
		opr_info->type        = DM_COPY;
		opr_info->options     = 0;
		opr_info->target_path = strdup(cip->fname);
		opr_info->src_path    = dirname(strdup(cip->template));
		opr_info->src_list    = (char **)MALLOC(sizeof(char *));
		opr_info->src_cnt     = 1;
		opr_info->src_win     = NULL;
		opr_info->dst_win     = (DmFolderWinPtr)cip->fwp;
		opr_info->x = opr_info->y = UNSPECIFIED_POS;

		opr_info->src_list[0] = strdup(basename(cip->template));

		(void)DmDoFileOp(opr_info, NewFileFMProc, (XtPointer)cip);
		/* return 0, not 1, because DmCloseNewWindow() is called in
		   NewFileFMProc() for this case.
		 */
		return(0);
	}
	/* Successfully created file, now open it. (Note that item is added
	   to folder but may not be displayed due to filter).
	*/
	if (success) {
		DmObjectPtr obj;
		/*
		 * Add the object to the container.  All views,
		 * will be notified and create items.
		 */
/*
 * Will re-enable this feature once timestamping is done.
 */
#ifdef NOT_USE
		obj = DmAddObjectToContainer(cip->fwp->views[0].cp,
			  	(DmObjectPtr)(((DmFnameKeyPtr)(fcp->key))->name),
			  	basename(cip->fname),
			  	DM_B_SET_TYPE | DM_B_INIT_FILEINFO | DM_B_SET_CLASS_PROP);
#endif
		obj = DmAddObjectToContainer(cip->fwp->views[0].cp,
			  	NULL,
			  	basename(cip->fname),
			  	DM_B_SET_TYPE | DM_B_INIT_FILEINFO);
		if (obj && open)
			DmOpenObject((DmWinPtr)cip->fwp, obj, 0);

		return(1);
	}
}					/* End of CreateFile */


/****************************procedure*header*****************************
	NewFileFMProc- This is called from DmDoFileOp() with DM_DONE or
	DM_ERROR from an operation to create a new file using a template;
	i.e. DmDoFileOp() was called with with DM_COPY request.
*/

static void
NewFileFMProc(DmProcReason reason, XtPointer client_data,
	XtPointer call_data, char *src, char *dst)
{
	CreateInfo *cip = (CreateInfo *)client_data;
	DmTaskInfoListPtr tip = (DmTaskInfoListPtr)call_data;
	DmFileOpInfoPtr opr_info = (DmFileOpInfoPtr)tip->opr_info;
	DmFolderWindow win = opr_info->dst_win;
	char *name = basename(opr_info->target_path);

	switch(reason) {
	case DM_DONE:
		if (!(opr_info->src_info[0] & SRC_B_IGNORE))
		{
	    		DmUpdateWindow(opr_info, DM_UPDATE_DSTWIN);

	    		if (cip->flag == DM_CREATE_N_OPEN)
	    		{
				DmItemPtr itp =
					DmObjNameToItem((DmWinPtr)win, name);

				if (itp != NULL)
		    			DmOpenObject((DmWinPtr)win,
						ITEM_OBJ(itp), 0);
	    		}
	    		DmVaDisplayStatus((DmWinPtr)win, False,
				TXT_CREATE_SUCCESS, name);
			DmCloseNewWindow(cip);
		}
		DmFreeTaskInfo(tip);
		break;

	case DM_ERROR:
		/* display reason for failure */
		DmVaDisplayStatus((DmWinPtr)win, True, TXT_CREATE_FAILED,name);
		break;
    }
}					/* end of NewFileFMProc() */

/****************************procedure*header*****************************
 * This function returns a 0 if the selected file type has one or more
 * and none is selected or a selected template is valid (i.e. it exists)
 * otherwise, it returns a -1. Also set cip->flag to
 * DM_USE_TEMPLATE if selected template is valid.
 */
static int
VerifyTmpl(CreateInfo *cip)
{
	struct stat mystat;
	char *full_path = NULL;
	char *tmpl_dir = NULL;
	Boolean set = False;

	/* check if selected template exists */
	if (cip->template[0] != '/') {
		if ((tmpl_dir = (char *)DmDTProp(TEMPLATEDIR, NULL)))
			full_path = DmMakePath(tmpl_dir, cip->template);
		else
			full_path = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
				"template", cip->template, NULL, NULL, NULL,
				0, NULL);

		if (full_path == NULL) {
			/* give up, don't know where else to look */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_BAD_TEMPLATE, cip->template);
			return(-1);
		}
	} else
		full_path = cip->template;

	if (stat(full_path, &mystat) == 0) {
		FREE(cip->template);
		cip->template = strdup(full_path);
		cip->flag = DM_USE_TEMPLATE;
		return(0);
	} else {
		cip->flag = DM_DONT_USE_TEMPLATE;
		DmVaDisplayStatus((DmWinPtr)cip->fwp, True, TXT_BAD_TEMPLATE,
			full_path);
		return(-1);
	}
}				/* end of VerifyTmpl() */

/****************************procedure*header*****************************
 * This function parses the value of the TEMPLATE file class property
 * and displays the template(s) in a flat list widget.  The value
 * can comprise one or a list of templates.
 */
static void
UpdateTemplates(CreateInfo *cip, char *template)
{
	char *p;
	char *s;
	char *tmp;
	XmString items[128];
	int cnt = 0;

	if ((p = strchr(template, ',')) == NULL) {  /* only one template */
		items[cnt++] = XmStringCreateLocalized(template);
	} else { /* multiple templates */
		p = NULL;
		s = template;
		while (*s != '\n') {
			if ((p = strchr(s, ',')) != NULL) {
				tmp = (char *)strndup(s, p - s);
				items[cnt++] = XmStringCreateLocalized(tmp);
				free(tmp);
				/* skip ',' */
				s = ++p;
			} else {
				int len = strlen(s);
				s[len] = '\0';
				items[cnt++] = XmStringCreateLocalized(s);
				break;
			}
		}
	}
	XtSetArg(Dm__arg[0], XmNitems, items);
	XtSetArg(Dm__arg[1], XmNitemCount, cnt);
	XtSetValues(cip->listW, Dm__arg, 2);

	/* select the first template */
	XmListSelectItem(cip->listW, items[0], False);

	/* set template to use to the first template */
	if (cip->template)
		FREE(cip->template);
	cip->template = GET_TEXT(items[0]);

#ifdef NOT_USE
	SetInputGizmoText(cip->inputG, cip->template);
#endif

	for (; cnt; cnt--)
		XmStringFree(items[cnt-1]);



}				/* end of UpdateTemplates() */

/****************************procedure*header*****************************
	VerifyInputs- This function verifies the input in the create new
	file prompt window.  Returns a 0 if all inputs are valid;
	otherwise, returns a -1.
*/

static int
VerifyInputs(CreateInfo *cip)
{
	struct stat mystat;
	char *p;
	char *str;
	DmFclassPtr fcp;
	DmFnameKeyPtr fnkp;
	dmvpret_t ret;

	str = strdup(cip->fname);
	if (str[0] != '/') { /* file name is not a full path */
		/* make sure there's no intermediate directories. */
		if (strchr(str, '/') != NULL) {
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_IN_THIS_FOLDER);
			FREE(str);
			return(-1);
		}
		p = DmMakePath(cip->fwp->views[0].cp->path,str);
		ret = DmValidateFname((DmWinPtr)cip->fwp, p, &mystat);
		if (ret) {
			if (ret  != NOFILE) {
				FREE(str);
				return(-1);
			}
		} else {
			/* file already exist */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_ALREADY_EXISTS, basename(cip->fname));
			FREE(str);
			return(-1);
		}
		FREE(cip->fname);
		/* construct a full path name */
		cip->fname=strdup(p);
		FREE(str);
	} else {
		/* A full path name is entered. Make sure new file is
		 * within current directory.
		 */
		char *s;
		char *ss;
		char *p;
		char *pp;
		char *t;

		/* cip->fwp->views[0].cp->path may have redundant '/' in front
		 * of it. Skip any preceeding '/' in it as well as in str.
		 */
		s = ss = strdup(str);
		while (*s == '/')
			++s;

		p = pp = strdup(cip->fwp->views[0].cp->path);
		while (*p == '/')
			++p;

		/* make sure file is to reside in current folder */
		if (strncmp(p, s, strlen(p)) != 0) {
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_IN_THIS_FOLDER);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}
		/* skip the '/' after the folder path */
		t = s + strlen(p) + 1;

		/* make sure there are no subdirectories */
		if (strchr(t, '/')) {
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_IN_THIS_FOLDER);
			FREE(ss);
			FREE(pp);
			FREE(str);
			return(-1);
		}
		FREE(ss);
		FREE(pp);
		ret = DmValidateFname((DmWinPtr)cip->fwp, cip->fwp->views[0].cp, str, &mystat);
		if (ret != NOFILE) {
			FREE(str);
			return(-1);
		} else {
			FREE(str);
			/* file already exist */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_ALREADY_EXISTS, basename(cip->fname));
			return(-1);
		}
	}
	/* make sure the file name specified matches the value of the */
	/* selected file class's PATTERN property, if it's defined.   */
	/* This check may be taken out, since we have PFA now */
	fcp = FCLASS_PTR(cip->itp);
	fnkp = (DmFnameKeyPtr)(fcp->key);
	if (fnkp->attrs & DM_B_REGEXP) {
		char *p2;
		char *free_this;

		for (p2=fnkp->re; *p2; p2=p2+strlen(p2)+1)
		{
			free_this = DmToUpper(p2);
			if (gmatch(basename(cip->fname), p2) || (free_this &&
			  gmatch(basename(cip->fname), free_this)))
			{
				/* found a match */
				if (free_this)
					free(free_this);
					break;
				}
				if (free_this)
					free(free_this);
			}
			if (*p2 == '\0') {
				p = DtGetProperty(&(fcp->plist), PATTERN,NULL);
				DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
					TXT_NOT_MATCH_PATTERN, p);
				return(-1);
			}
	}
	/* Validate selected template, if any */
	if (cip->template) {
		return(VerifyTmpl(cip));
	}
	cip->flag = DM_DONT_USE_TEMPLATE;
	/* inputs are valid */
	return(0);
}					/* End of VerifyInputs */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmPromptCreateFile - create popup to prompt for info to create new file.
*/
void
DmPromptCreateFile(DmFolderWindow fwp)
{
	DmClearStatus((DmWinPtr)fwp);
	if (fwp->createWindow == NULL) {
		Widget shell;
		Widget w;
		CreateInfo *cip;
		DmItemPtr itp;
		DmFclassPtr fcp;
		int nitems;
		int i;
		char *template;
		char *pattern;

		cip = (CreateInfo *)CALLOC(1, sizeof(CreateInfo));
		cip->fwp = fwp;
		create_popup.menu->clientData =(XtPointer)cip;

		fwp->createWindow = CreateGizmo(fwp->shell, PopupGizmoClass,
			&create_popup, NULL, 0);

		shell = GetPopupGizmoShell(fwp->createWindow);

#ifdef NOT_USE
		w = QueryGizmo(PopupGizmoClass, fwp->createWindow,
				GetGizmoWidget, "name");

		XtSetArg(Dm__arg[0], XtNfocusWidget, w);
		XtSetValues(shell, Dm__arg, 1);
#endif

		cip->inputG = QueryGizmo(PopupGizmoClass,
			fwp->createWindow, GetGizmoGizmo, "name");

		cip->clistG = (CListGizmo *)QueryGizmo(PopupGizmoClass,
			fwp->createWindow, GetGizmoGizmo, "clist");

		w = QueryGizmo(PopupGizmoClass, fwp->createWindow,
			GetGizmoWidget, "rowCol");

		XtSetArg(Dm__arg[0], XmNnumColumns, 2);
		XtSetArg(Dm__arg[1], XmNorientation, XmVERTICAL);
		XtSetArg(Dm__arg[2], XmNpacking, XmPACK_COLUMN);
		XtSetValues(w, Dm__arg, 3);

		cip->patternW = QueryGizmo(PopupGizmoClass,
			fwp->createWindow, GetGizmoWidget, "pattern");

		/* make Datafile the default */
		XtVaGetValues(cip->clistG->boxWidget, XmNnumItems, &nitems,
			NULL);

		for (i = 0, itp = cip->clistG->itp; i < nitems; i++, itp++) {
			if (itp->managed) {
				fcp = FCLASS_PTR(itp);
				/* check for both built-in and user-defined
				 * DATA file class.
				 */
				if (strcmp(((DmFmodeKeyPtr)(fcp->key))->name,
					"DATA") == 0)
				{
				  ExmVaFlatSetValues(cip->clistG->boxWidget,
					(itp - cip->clistG->itp), XmNset,
					True, NULL);
				  cip->itp = itp;
				  break;
				}
			}
		}
		XtSetArg(Dm__arg[0], XmNclientData, (XtPointer)cip);
		XtSetValues(cip->clistG->boxWidget, Dm__arg, 1);

		cip->listW = QueryGizmo(PopupGizmoClass, fwp->createWindow,
				GetGizmoWidget, "template");

		XtSetArg(Dm__arg[0], XmNselectionPolicy, XmSINGLE_SELECT);
		XtSetValues(cip->listW, Dm__arg, 1);

		XtAddCallback(cip->listW, XmNdefaultActionCallback,
			(XtCallbackProc)TmplCB, (XtPointer)cip);
		XtAddCallback(cip->listW, XmNsingleSelectionCallback,
			(XtCallbackProc)TmplCB, (XtPointer)cip);

		pattern = DtGetProperty(&(ITEM_OBJ(itp)->fcp->plist),
			PATTERN, NULL);
		if (pattern) {
			XtSetArg(Dm__arg[0], XmNlabelString,
				XmStringCreateLocalized(pattern));
			XtSetValues(cip->patternW, Dm__arg, 1);
		}

		w = QueryGizmo(PopupGizmoClass, fwp->createWindow,
			GetGizmoWidget, "template_label");

		/* get value of TEMPLATE property of Datafile file class */
		template = DtGetProperty(&(ITEM_OBJ(itp)->fcp->plist),
			TEMPLATE, NULL);
		if (template) {
			XtSetSensitive(w, True);
			UpdateTemplates(cip, template);
		} else {
			/* no template, make list widget insensitive */
			XtSetSensitive(w, False);
		}
		DmRegContextSensitiveHelp(
			GetPopupGizmoRowCol(fwp->createWindow),
			FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
			FOLDER_FILE_NEW_SECT);

	}
	MapGizmo(PopupGizmoClass, fwp->createWindow);

}				/* End of DmPromptCreateFile */

/****************************procedure*header*****************************
	DmCloseNewWindow - Frees resources previously allocated and if
	destroy is set, destroys the New window.  destroy is False if
	caller is DmCloseFolderWindow(); it's True otherwise.
*/
void
DmCloseNewWindow(CreateInfo *cip)
{
    XmString items;
    DmFolderWindow fwp = cip->fwp;
    Widget shell = GetPopupGizmoShell(cip->fwp->createWindow);

    XtVaGetValues(cip->listW, XmNitems, &items, NULL);
    if (items)
	XmListDeleteAllItems(cip->listW);

    if (cip->fname)
	FREE(cip->fname);
    if (cip->template)
	FREE(cip->template);
    FREE(cip);
    FreeGizmo(PopupGizmoClass, fwp->createWindow);
    fwp->createWindow = NULL;
    XtDestroyWidget(shell);
} /* end of DmCloseNewWindow */

/****************************procedure*header*********************************
 *  DmDestroyNewWindow: destroy the New Window
 *	INPUTS:	folder
 *	OUTPUTS: none
 *	GLOBALS:
 *****************************************************************************/
void
DmDestroyNewWindow(DmFolderWindow folder)
{
    if (folder->createWindow != NULL) {
	CreateInfo *cip;
	CListGizmo *clistG = (CListGizmo *)QueryGizmo(PopupGizmoClass,
			folder->createWindow, GetGizmoGizmo, "clist");

	XtSetArg(Dm__arg[0], XmNclientData, &cip);
	XtGetValues(clistG->boxWidget, Dm__arg, 1);
	DmCloseNewWindow(cip);
    }
}	/* end of DmDestroyNewWindow */

/****************************procedure*header*****************************
	CreateDataFile - Creates a file of DATA file class without using a
	template.
 */
static int
CreateDataFile(CreateInfo *cip, Boolean *success)
{
	int fd;

	/* turn off execute bit */
	if (((fd = creat(cip->fname,
		DESKTOP_UMASK(Desktop) & ~(S_IXUSR | S_IXGRP | S_IXOTH))))
		!= -1)
	{
		close(fd);
		DmVaDisplayStatus((DmWinPtr)cip->fwp, False,TXT_CREATE_SUCCESS,
			basename(cip->fname));
		*success = True;
		return(0);
	} else {
		/* check errno & display reason for failure */
		if (errno == EACCES) {
			/* no permission */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_CREATE_NOPERM);
		} else {
			/* failure due to other reasons */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_CREATE_FAILED, basename(cip->fname));
		}
		return(-1);
	}
} /* end of CreateDataFile */

/****************************procedure*header*****************************
	CreateDir -
 */
static int
CreateDir(CreateInfo *cip, Boolean *success)
{
	if ((mkdir(cip->fname, DESKTOP_UMASK(Desktop))) != -1)
	{
		DmVaDisplayStatus((DmWinPtr)cip->fwp, False,TXT_CREATE_SUCCESS,
			basename(cip->fname));
		*success = True;
		return(0);
	} else {
		/* check errno & display reason for failure */
		if (errno == EACCES)
			/* no permission */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_CREATE_NOPERM);
		else
			/* failure due to other reasons */
			DmVaDisplayStatus((DmWinPtr)cip->fwp, True,
				TXT_CREATE_FAILED, basename(cip->fname));
		return(-1);
	}
} /* end of CreateDir */
