#pragma ident	"@(#)dtm:f_cbs.c	1.193"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-related callbacks.
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
#ifdef DEBUG
#include <X11/Xmu/Editres.h>
#endif

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/PopupGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static int	CheckPath(char *path, struct stat * stat_buf);
static void	CopyCB(Widget, XtPointer, XtPointer);
static void	ConvertCB(Widget, XtPointer, XtPointer, Boolean);
static void	ConvertD2UCB(Widget, XtPointer, XtPointer);
static void	ConvertU2DCB(Widget, XtPointer, XtPointer);
static Gizmo 	CreateFilePrompt(DmFolderWindow folder, FileGizmo * gizmo);
static char     *GetTarget(DmFolderWindow win, Gizmo prompt, char *cwd);
static void	LinkCB(Widget, XtPointer, XtPointer);
static void	MoveCB(Widget, XtPointer, XtPointer);
static void	OpenOtherCB(Widget, XtPointer, XtPointer);
static void	RenameCB(Widget, XtPointer, XtPointer);
static int	SetupFileOp(DmFolderWindow, DmFileOpType, char * target);
static void	Undo(DmFolderWinPtr window);

					/* public procedures		*/
void		DmEditUndoCB(Widget, XtPointer, XtPointer);
void		DmFileConverD2UCB(Widget, XtPointer, XtPointer);
void		DmFileConverU2DCB(Widget, XtPointer, XtPointer);
void		DmFileCopyCB(Widget, XtPointer, XtPointer);
void		DmFileDeleteCB(Widget, XtPointer, XtPointer);
void		DmFileLinkCB(Widget, XtPointer, XtPointer);
void		DmFileMoveCB(Widget, XtPointer, XtPointer);
void		DmFileNewCB(Widget, XtPointer, XtPointer);
void		DmFileOpenCB(Widget, XtPointer, XtPointer);
void		DmFileOpenNewCB(Widget, XtPointer, XtPointer);
void		DmFilePrintCB(Widget, XtPointer, XtPointer);
void		DmFilePropCB(Widget, XtPointer, XtPointer);
void		DmFileRenameCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenDirCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenOtherCB(Widget, XtPointer, XtPointer);
void		DmFolderOpenParentCB(Widget, XtPointer, XtPointer);
void		DmGotoDesktopCB(Widget, XtPointer, XtPointer);
void 		DmGetPrompt(Gizmo, char **);
int		DmSetupFileOp(DmFolderWindow, DmFileOpType, char *, void **);
int		DmUpdatePrompts(DmFolderWindow folder);
dmvpret_t DmValidateFname(DmWinPtr, char *, struct stat *);
/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/


/* Menu Items for Convert DOS To Unix & for Convert Unix To DOS */
static MenuItems  FileConvertD2UMenuItems[] = {
    MENU_ITEM( TXT_CONVERT,	TXT_M_CONVERT,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

static MenuItems  FileConvertU2DMenuItems[] = {
    MENU_ITEM( TXT_CONVERT,	TXT_M_CONVERT,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

/* Menu Items for "Copy", "Rename" "Move", "Link" & "Open" prompts */
static MenuItems  FileCopyMenuItems[] = {
    MENU_ITEM( TXT_COPY,	TXT_M_COPY,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

static MenuItems  FileRenameMenuItems[] = {
    MENU_ITEM( TXT_RENAME,	TXT_M_RENAME,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

static MenuItems  FileMoveMenuItems[] = {
    MENU_ITEM( TXT_MOVE,	TXT_M_MOVE,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

static MenuItems  FileLinkMenuItems[] = {
    MENU_ITEM( TXT_LINK,	TXT_M_LINK,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

static MenuItems  FileOpenOtherMenuItems[] = {
    MENU_ITEM( TXT_OPEN,	TXT_M_OPEN,	NULL ),
    MENU_ITEM( TXT_CANCEL,	TXT_M_CANCEL,	NULL ),
    MENU_ITEM( TXT_HELP,	TXT_M_HELP,	NULL ),
    { NULL }
};

/* Menus for "Copy", "Move", "Link" & "Open", etc */
MENU_BAR("ConvertD2UMenuBar",	FileConvertD2UMenu,	ConvertD2UCB, 0, 1 );
MENU_BAR("ConvertU2DMenuBar",	FileConvertU2DMenu,	ConvertU2DCB, 0, 1 );
MENU_BAR("CopyMenuBar",		FileCopyMenu,		CopyCB, 0, 1 );
MENU_BAR("RenameMenuBar",	FileRenameMenu,		RenameCB, 0, 1 );
MENU_BAR("MoveMenuBar",		FileMoveMenu,		MoveCB, 0, 1 );
MENU_BAR("LinkMenuBar",		FileLinkMenu,		LinkCB, 0, 1 );
MENU_BAR("OpenOtherMenuBar",	FileOpenOtherMenu,	OpenOtherCB, 0, 1 );


/* Radio Button definition for the Link Type: "Hard" "Symbolic" */
static MenuItems LinkTypeItems[] = {
  { True, TXT_LINK_SOFT, TXT_M_LINK_SOFT, I_RADIO_BUTTON, NULL, NULL, (char *)DM_SYMLINK, True },
  { True, TXT_LINK_HARD, TXT_M_LINK_HARD, I_RADIO_BUTTON, NULL, NULL, (char *)DM_HARDLINK, False },
  { NULL }
};

static MenuGizmo LinkTypeMenu =
  { NULL, "TypeMenu", NULL, LinkTypeItems, NULL, NULL, XmHORIZONTAL, 1, 0 };

/* Define LinkType ChoiceGizmo ie. a caption with a RADIO_BOX attached. */
static ChoiceGizmo LinkTypeChoice =
  { NULL, "TypeChoice", &LinkTypeMenu, G_RADIO_BOX };

static GizmoRec LinkTypeGiz[] = {
        {ChoiceGizmoClass,       &LinkTypeChoice},
};

static LabelGizmo LinkLowerGizmos = {
    NULL, 			/* help */
    "link_type", 		/* name */
    TXT_LINK_LABEL,		/* label */
    True,			/* dontAlignLabel */	/* FLH REMOVE */
    LinkTypeGiz, 		/* gizmos */
    XtNumber(LinkTypeGiz)	/* numGizmos */
};
static GizmoRec link_lower_gizmos[] = {
  { LabelGizmoClass, &LinkLowerGizmos, /* args, numArgs */ }
};

static InputGizmo RenameText = {
    NULL,		/* help */
    "renameText",	/* name */
    NULL,		/* XmNvalue */
    0,			/* XmNmaxLength */
    NULL,		/* XmNxactivateCallback */  /* FLH MORE: what to set */
    NULL		/* client_data */
};
static GizmoRec rename_text_gizmos[] = {
  { InputGizmoClass, &RenameText, /* args, numArgs */ }
};

static LabelGizmo RenameCaption =  { 
    NULL, 			/* help */
    "renameCaption", 		/* name */
    TXT_RENAME_CAPTION, 	/* XmNlabelString */
    False,			/* dontAlignLabel */
    rename_text_gizmos,		/* gizmos */
    XtNumber(rename_text_gizmos)/* numGizmos */
};

static GizmoRec RenameGizmos[] = {
  { LabelGizmoClass, &RenameCaption, },
};

/* Inputs for Convert */

static InputGizmo ConvertD2UText = {
    NULL,		/* help */
    "ConvertD2UText",	/* name */
    NULL,		/* XmNvalue */
    0,			/* XmNmaxLength */
    NULL,		/* XmNxactivateCallback */  /* FLH MORE: what to set */
    NULL		/* client_data */
};
static GizmoRec convertd2u_text_gizmos[] = {
  { InputGizmoClass, &ConvertD2UText, /* args, numArgs */ }
};

static LabelGizmo ConvertD2UCaption =  { 
    NULL, 			/* help */
    "convertD2UCaption", 		/* name */
    TXT_CONVERT_CAPTION, 	/* XmNlabelString */
    False,			/* dontAlignLabel */
    convertd2u_text_gizmos,		/* gizmos */
    XtNumber(convertd2u_text_gizmos)/* numGizmos */
};

static GizmoRec ConvertD2UGizmos[] = {
  { LabelGizmoClass, &ConvertD2UCaption, },
};

static InputGizmo ConvertU2DText = {
    NULL,		/* help */
    "ConvertU2DText",	/* name */
    NULL,		/* XmNvalue */
    0,			/* XmNmaxLength */
    NULL,		/* XmNxactivateCallback */  /* FLH MORE: what to set */
    NULL		/* client_data */
};
static GizmoRec convertu2d_text_gizmos[] = {
  { InputGizmoClass, &ConvertU2DText, /* args, numArgs */ }
};

static LabelGizmo ConvertU2DCaption =  { 
    NULL, 			/* help */
    "convertU2DCaption", 		/* name */
    TXT_CONVERT_CAPTION, 	/* XmNlabelString */
    False,			/* dontAlignLabel */
    convertu2d_text_gizmos,		/* gizmos */
    XtNumber(convertu2d_text_gizmos)/* numGizmos */
};

static GizmoRec ConvertU2DGizmos[] = {
  { LabelGizmoClass, &ConvertU2DCaption, },
};

/* Prompts for "Copy", "Rename", "Move", "Link" & "Open" */

static LabelGizmo FileUpperItems = {
    NULL, 		/* help */
    "file_items", 	/* name */
    NULL,		/* label */
    True,		/* dontAlignLabel */
    NULL, 0		/* gizmos, numGizmos */
};
static GizmoRec file_upper_items[] = {
  { LabelGizmoClass, &FileUpperItems, /* args, numArgs */ }
};

static LabelGizmo CopyUpperOperation = {
    NULL, 		/* help */
    "file_operation", 	/* name */
    TXT_COPY_OPER, 	/* label */
    False,		/* dontAlignLabel */
    file_upper_items, 	/* gizmos */
    XtNumber(file_upper_items),	/* numGizmos */
};

static GizmoRec copy_upper_gizmos[] = {
  { LabelGizmoClass, &CopyUpperOperation, /* args, numArgs */ }
};

static FileGizmo FileCopyPrompt = {
	NULL, 				/* help */
	"filecopy", 			/* name */
	TXT_EDIT_COPY_TITLE, 		/* title */
	&FileCopyMenu, 			/* menu */
	copy_upper_gizmos, 		/* upperGizmos */
	XtNumber(copy_upper_gizmos), 	/* numUpper */
	NULL, 0,			/* lowerGizmos, numLower */
	TXT_TO,				/* path Label */
	TXT_AS,				/* input Label */
	FOLDERS_ONLY, 			/* dialogType */
	".",				/* directory */
	ABOVE_LIST			/* lower gizmo position */
};

static PopupGizmo FileRenamePrompt = {
	NULL, 			/* help */
	"filerename", 		/* name */
	TXT_EDIT_RENAME_TITLE, 	/* title for wm */
	&FileRenameMenu,	/* menu */
	RenameGizmos, 		/* gizmos */
	XtNumber(RenameGizmos)	/* numGizmos */
};

static LabelGizmo MoveUpperOperation = {
    NULL, 			/* help */
    "file_operation", 		/* name */
    TXT_MOVE_OPER, 		/* label */
    False,			/* dontAlignLabel */
    file_upper_items,		/* gizmos, numGizmos */
    XtNumber(file_upper_items)	/* gizmos, numGizmos */
};

static GizmoRec move_upper_gizmos[] = {
  { LabelGizmoClass, &MoveUpperOperation, /* args, numArgs */ }
};

static FileGizmo FileMovePrompt = {
    NULL, 			/* help */
    "filemove", 		/* name */
    TXT_EDIT_MOVE_TITLE,	/* title */
    &FileMoveMenu, 		/* menu */
    move_upper_gizmos, 		/* upperGizmos */
    XtNumber(move_upper_gizmos),/* numUpper */
    NULL, 0,			/* lowerGizmos, numLower */
    TXT_TO,			/* path label */
    TXT_AS,			/* input label */
    FOLDERS_ONLY,		/* dialogType */
    ".",			/* directory */
    ABOVE_LIST			/* lower gizmo position */
};

static LabelGizmo LinkUpperOperation = {
    NULL, 			/* help */
    "file_operation", 		/* name */
    TXT_LINK_OPER, 		/* label */
    False,			/* dontAlignLabel */
    file_upper_items, 		/* gizmos */
    XtNumber(file_upper_items)	/* numGizmos */
};

static GizmoRec link_upper_gizmos[] = {
  { LabelGizmoClass, &LinkUpperOperation, /* args, numArgs */ }
};

static FileGizmo FileLinkPrompt = {
    NULL, 			/* help */
    "filelink", 		/* name */
    TXT_EDIT_LINK_TITLE,	/* title */
    &FileLinkMenu, 		/* menu */
    link_upper_gizmos, 		/* upperGizmos */
    XtNumber(link_upper_gizmos),/* numUpper */
    link_lower_gizmos,		/* lowerGizmos */
    XtNumber(link_lower_gizmos),/* numLower */
    TXT_TO,			/* path label */
    TXT_AS,			/* input label */
    FOLDERS_ONLY,		/* dialogType */
    ".",			/* directory */
    BELOW_LIST			/* lower gizmo position */
};


static FileGizmo FileOpenPrompt = {
	NULL,  			/* help */
	"fileopen", 		/* name */
	TXT_GOTO_OPEN_OTHER, 	/* title */
	&FileOpenOtherMenu, 	/* menu */
	NULL, 0,		/* upperGizmos, numUpper */
	NULL, 0, 		/* lowerGizmos, numLower */
	TXT_OPEN_OPER,		/* path Label (Open:)*/
	TXT_QUICK_OPEN,		/* input Label (Quick Open:) */
	FOLDERS_ONLY,		/* dialogType */
	".",			/* directory */
	ABOVE_LIST		/* lower gizmo position */
};

/* Prompts for "Convert" */

static PopupGizmo FileConvertD2UPrompt = {
	NULL, 	/* help */
	"filed2uconvert", 		/* name */
	TXT_CONVERTD2U_TITLE, 	/* title for wm */
	&FileConvertD2UMenu,	/* menu */
	ConvertD2UGizmos, 		/* gizmos */
	XtNumber(ConvertD2UGizmos)	/* numGizmos */
};

static PopupGizmo FileConvertU2DPrompt = {
	NULL, 			/* help */
	"fileu2dconvert", 		/* name */
	TXT_CONVERTU2D_TITLE, 	/* title for wm */
	&FileConvertU2DMenu,	/* menu */
	ConvertU2DGizmos, 		/* gizmos */
	XtNumber(ConvertU2DGizmos)	/* numGizmos */
};

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    CopyCB- callback for Copy operation.
*/
static void
CopyCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    char *		target;


    _DPRINT1(stderr,"CopyCB: item %d selected for %s\n", gizmo_cd->index, 
	    DM_WIN_PATH(folder));

    switch(gizmo_cd->index)
    {
    case FilePromptTask:
	target = GetTarget(folder, folder->copy_prompt, NULL);
	if ((target != NULL) && (SetupFileOp(folder, DM_COPY, target) == 0))
	    BringDownPopup( (Widget)DtGetShellOfWidget(widget) );
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(widget));
	break;

    case FilePromptHelp:
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_COPY_SECT);
	break;
    }
}				/* End of CopyCB */


/****************************procedure*header*****************************
    CreateFilePrompt-
*/
static Gizmo
CreateFilePrompt(DmFolderWindow folder, FileGizmo * gizmo)
{
    Gizmo prompt;

    gizmo->menu->clientData = 	(XtPointer)folder;
    gizmo->directory =
	IS_FOUND_WIN(Desktop, folder) ? 
	    DESKTOP_DIR(Desktop) : IS_TREE_WIN(Desktop, folder) ?
		DmObjPath(ITEM_OBJ(folder->views[0].itp)) : 
		    DM_WIN_USERPATH(folder);

    prompt = CreateGizmo(folder->shell, FileGizmoClass, gizmo, NULL, 0);

#ifdef DEBUG
    {
	Widget shell = QueryGizmo(FileGizmoClass, prompt, GetGizmoWidget,
				  NULL);
	XtAddEventHandler(shell, (EventMask) 0, True, _XEditResCheckMessages, 
			  NULL);

    }
#endif
    return(prompt);
}					/* end of CreateFilePrompt */


/*
 * DmValidateFname
 * 	Validate the user input to prompt boxes that are presented
 *	when user initiates file operations -- copy, move, link, etc.
 */
dmvpret_t 
DmValidateFname(DmWinPtr f, char *pathp, struct stat *sb)
{
	struct stat lsb;
	char	    *pnmp;
	char	    *fnmp;
	dmvpret_t   ret = VALID;
	int 	    len;
	int 	    fstype;
	char	    dosnm[DOS_MAXPATH];
	extern	    int legalfcreate(unsigned char *);


	pnmp = strdup(pathp);

	/* Get the directory */
	if (dirname(pnmp) != NULL) { 
		if (!sb)
			sb = &lsb;
		errno = 0;
		if (stat(pnmp, sb)) {
			ret = DSTATFAIL;
			goto err;
		}
		fstype = convnmtonum(sb->st_fstype);
	}

	if (Desktop->fstbl[fstype].f_maxnm == -1) {
		struct statvfs v;
		statvfs(pnmp, &v);
		Desktop->fstbl[fstype].f_maxnm = v.f_namemax;
	}

	free (pnmp);
	pnmp = strdup(pathp);
	fnmp = basename(pnmp);

	/* MS_DOS file system */
	if (fstype == DOSFS_ID(Desktop)) {
		if (unix2dosfn(fnmp, dosnm, legalfcreate, strlen(fnmp)))
			ret = INVALCHAR;
		else if (strlen(fnmp) != strlen(dosnm))
			ret = INVALCHAR;
	} else if (((len = strlen(fnmp)) > Desktop->fstbl[fstype].f_maxnm) &&
	           (Desktop->fstbl[fstype].f_maxnm != -2)) {
		ret =  MAXNM;
		goto err;
	}

	if (ret == VALID) {
		if (!sb)
			sb = &lsb;
		errno = 0;
		if (stat(pathp, sb)) {
    			free(pnmp);
			if (errno = ENOENT)
				return NOFILE;
			return STATFAIL;
		} else {
    			free(pnmp);
			return VALID;
		}
	}
err:
    switch (ret) {
    case MAXNM:
	if (fstype == DOSFS_ID(Desktop))
		DmVaDisplayStatus(f, True, TXT_DOSNAME_TOO_LONG);
	else
		DmVaDisplayStatus(f, True, TXT_NAME_TOO_LONG, 
				  Desktop->fstbl[fstype].f_maxnm);
	break;

    case INVALCHAR:
	DmVaDisplayStatus(f, True, TXT_VALID_CHAR, doschar); 
	break;

    case DSTATFAIL:
	DmVaDisplayStatus(f, True, TXT_STAT, errno, pnmp);
	break;
    }
    free(pnmp);
    return ret;
}


/****************************procedure*header*****************************
 *   GetTarget- get target from "prompt", validate it and display error msg if
 *	error. 
 *	INPUT:	folder
 *		prompt
 *		current working directory (only used for Rename)
 *	OUTPUT: full pathname of target if valid, NULL otherwise
 *************************************************************************/
static char *
GetTarget(DmFolderWindow folder, Gizmo prompt, char *cwd)
{
    char 	*target = NULL;
    char 	*save = NULL;
    int		ret;

    if (prompt != folder->rename_prompt && prompt!= folder->convertd2u_prompt
	&& prompt != folder->convertu2d_prompt)
	DmGetPrompt(prompt, &target);
    else {
	/* 
	 * The RenamePrompt is not a FileGizmo.  It requires
	 * special handling to get the full path.
	 */
	Gizmo	rename_text;

	if (prompt == folder->convertd2u_prompt)
		rename_text = QueryGizmo(PopupGizmoClass, folder->convertd2u_prompt,
				 GetGizmoGizmo, "ConvertD2UText");
	else if (prompt == folder->convertu2d_prompt)
		rename_text = QueryGizmo(PopupGizmoClass, folder->convertu2d_prompt,
				 GetGizmoGizmo, "ConvertU2DText");
	else if (prompt == folder->rename_prompt)
		rename_text = QueryGizmo(PopupGizmoClass, folder->rename_prompt,
				 GetGizmoGizmo, "renameText");
	target = GetInputGizmoText(rename_text);


	if (!target || target[0] =='\0') {
 		DmVaDisplayStatus((DmWinPtr)folder, True, TXT_NO_FILE_NAME);
		return NULL;
	}
	if (strchr(target, '/')) {
		DmVaDisplayStatus((DmWinPtr)folder, True,
                                TXT_IN_THIS_FOLDER);
		FREE(target);
		return NULL;
	}
        save = target;
	target = strdup(DmMakePath(cwd, target));
	FREE(save);
    }
 
    switch (DmValidateFname((DmWinPtr)folder, target, NULL)) { 
    case STATFAIL:
	DmVaDisplayStatus((DmWinPtr)folder, True,
			 TXT_INVALID_FILE_OR_FOLDER, target);
	if (target != NULL) {
		FREE(target);
		target = NULL;
	}
	break;

    case VALID:
    case NOFILE:
	break;

    default:
	if (target != NULL) {
		FREE(target);
		target = NULL;
	}
	break;
    }

    return(target);
}					/* end of GetTarget */


/****************************procedure*header*****************************
    LinkCB- callback for Link operation.
*/
static void
LinkCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    int			link_index;
    DmFileOpType	link_type;
    Gizmo 		choice;
    char *		target;

   switch(gizmo_cd->index)
   {
   case FilePromptTask:
       choice = (Gizmo) QueryGizmo(FileGizmoClass, folder->link_prompt,
				   GetGizmoGizmo, "TypeChoice");
       
       ManipulateGizmo(ChoiceGizmoClass, choice, GetGizmoValue);
       link_index = (int) QueryGizmo(ChoiceGizmoClass, choice, 
				     GetGizmoCurrentValue, NULL);
       link_type = (DmFileOpType) LinkTypeItems[link_index].clientData;

       _DPRINT3(stderr, "LinkCB: %s\n", link_type == DM_SYMLINK ? "DM_SYMLINK":
	       link_type == DM_HARDLINK ? "DM_HARDLINK" : "BAD_LINK_TYPE");


       target =  GetTarget(folder, folder->link_prompt, NULL);
	if ((target != NULL) && (SetupFileOp(folder, link_type, target) == 0))
	    BringDownPopup( (Widget)DtGetShellOfWidget(widget) );
       break;

   case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(widget) );
	break;
   case FilePromptHelp:
       DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_LINK_SECT);
       break;
   }
}				/* end of LinkCB */

/****************************procedure*header*****************************
    MoveCB- callback for Move operation.
*/
static void
MoveCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    char *		target;


    _DPRINT1(stderr,"MoveCB: item %d selected for %s\n",
	    gizmo_cd->index, DM_WIN_PATH(folder));

    switch(gizmo_cd->index)
    {
    case FilePromptTask:
	target = GetTarget(folder, folder->move_prompt, NULL);
	if ((target != NULL) && (SetupFileOp(folder, DM_MOVE, target) == 0))
	    BringDownPopup( (Widget)DtGetShellOfWidget(w) );
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(w));
	break;

    case FilePromptHelp:
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_MOVE_SECT);
	break;
    }
}				/* end of MoveCB */

/****************************procedure*header*****************************
    OpenOtherCB- this is called from the Open Folder popup which occurs
	when the user asks to open an "other" folder.  The user has typed in
	a folder name.

	It can be static since it is referenced only by the Prompt struct
	for the popup in this file.
*/
static void
OpenOtherCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    DmFolderWinPtr	new_folder;
    char * 		foldername;
    int			status;
    struct stat		stat_buf;
    
    
    _DPRINT1(stderr,"OpenOtherCB: item %d selected for %s\n",
	     gizmo_cd->index, DM_WIN_PATH(folder));
    
    
    switch(gizmo_cd->index) {
    case FilePromptTask:
	DmGetPrompt(folder->folder_prompt, &foldername);
	status = stat(foldername, &stat_buf); 
	
	/* Even when no error, check for 'dir' type */
	if ((status == 0) && S_ISDIR(stat_buf.st_mode)) {
	    if (IS_WB_PATH(Desktop, foldername)) {
		/* From wbReqProc.c:DisplayWBProc
		 * This code assume that the wastebasket 
		 * window is always created up front.
		 */
		DmMapWindow((DmWinPtr)DESKTOP_WB_WIN(Desktop));
		DmClearStatus((DmWinPtr)DESKTOP_WB_WIN(Desktop));
		
	    } else {
		/* 
		 * This could take a while so entertain 
		 * the user 
		 */
		BUSY_FOLDER(folder);
		
		/* Raise the folder if it is already opened 
		 * We don't want a rooted folder or the desktop
		 * folder
		 */
		if (new_folder = DmQueryFolderWindow(foldername, DM_B_NOT_ROOTED | DM_B_NOT_DESKTOP)) {
		    
		    DmMapWindow((DmWinPtr)new_folder);
		    /* move it to top of menu */
		    Dm__UpdateVisitedFolders(NULL, DM_WIN_PATH(new_folder));
		} else
		    new_folder = DmOpenInPlace((DmFolderWinPtr)
					       folder, foldername);
		
		if (new_folder != NULL)
		    BringDownPopup( (Widget)DtGetShellOfWidget(w) );
		else
		    DmVaDisplayStatus((DmWinPtr)folder, True,
				      TXT_OpenErr, foldername); 
	    }
	} else 
	    DmVaDisplayStatus((DmWinPtr)folder, True, TXT_NOT_DIR, 
			      foldername);
	
	if (foldername != NULL)
	    FREE(foldername);
	break;
	
    case FilePromptCancel:
	BringDownPopup( (Widget)DtGetShellOfWidget(w) );
	break;
	
    case FilePromptHelp:
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
			     FOLDER_HELPFILE, FOLDER_FILE_OPEN_OTHER_SECT);
	break;
    }
}				/* End of OpenOtherCB */


/****************************procedure*header*****************************
    ConvertD2UCB- callback for Convert operation.
*/

static void
ConvertD2UCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	ConvertCB(w, client_data, call_data, True);

}

/****************************procedure*header*****************************
    ConvertU2DCB- callback for Convert operation.
*/

static void
ConvertU2DCB(Widget w, XtPointer client_data, XtPointer call_data)
{

	ConvertCB(w, client_data, call_data, False);
}

/****************************procedure*header*****************************
    ConvertCB- callback for Convert operation.
*/

static void
ConvertCB(Widget w, XtPointer client_data, XtPointer call_data, Boolean d2u)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    Gizmo 		prompt;
    char *		target;
    char *		cwd;

    switch(gizmo_cd->index)
    {
    case FilePromptTask:
	if (d2u){
		prompt = folder->convertd2u_prompt;
	}
	else
		prompt = folder->convertu2d_prompt;

	/* 
	 * Must generate correct dir-name: container path for folders,
	 * obj-path for other windows (must get src obj).
	 */
	if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	{
	    DmItemPtr	item;

	    for (item = folder->views[0].itp;
		 item < folder->views[0].itp + folder->views[0].nitems; item++)
		if (ITEM_MANAGED(item) && ITEM_SELECT(item) && ITEM_SENSITIVE(item))
		    break;

	    /* *Must* find an item since button would be insens otherwise */
	    cwd = strdup( DmObjDir(ITEM_OBJ(item)) );

	} else
	    cwd = folder->views[0].cp->path;

	target = GetTarget(folder, prompt, cwd);
	if (target != NULL)
	{
	    char * name = target + strlen(cwd);

	    if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
		FREE(cwd);

	    /* Check for NULL input and don't allow paths (no slash) */
	    if ((name[0] == '\0') || (strchr(name + 1, '/') != NULL))
	    {
		DmVaDisplayStatus((DmWinPtr)folder, True, (name[0] == '\0') ?
				  TXT_NO_PATH_ALLOWED : TXT_NO_PATH_ALLOWED);
		FREE(target);
		return;
	    }
	    /* Will free target as necessary: */
	    if (SetupFileOp(folder, d2u ? DM_CONV_D2U : DM_CONV_U2D, target) == 0)
		BringDownPopup( (Widget)DtGetShellOfWidget(w) );
	}
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(w));
	break;


    case FilePromptHelp:
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_CONVERT_SECT);
	break;
    }
}				/* end of ConvertCB */

/****************************procedure*header*****************************
    RenameCB- callback for Rename operation.
*/

static void
RenameCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *gizmo_cd = (MenuGizmoCallbackStruct *)client_data;
    DmFolderWindow	folder = (DmFolderWindow) gizmo_cd->clientData;
    Gizmo 		prompt;
    char *		target;
    char *		cwd;

    switch(gizmo_cd->index)
    {
    case FilePromptTask:
	prompt = folder->rename_prompt;

	/* 
	 * Must generate correct dir-name: container path for folders,
	 * obj-path for other windows (must get src obj).
	 */
	if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	{
	    DmItemPtr	item;

	    for (item = folder->views[0].itp;
		 item < folder->views[0].itp + folder->views[0].nitems; item++)
		if (ITEM_MANAGED(item) && ITEM_SELECT(item) && ITEM_SENSITIVE(item))
		    break;

	    /* *Must* find an item since button would be insens otherwise */
	    cwd = strdup( DmObjDir(ITEM_OBJ(item)) );

	} else
	    cwd = folder->views[0].cp->path;

	target = GetTarget(folder, prompt, cwd);
	if (target != NULL)
	{
	    char * name = target + strlen(cwd);

	    if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
		FREE(cwd);

	    /* Check for NULL input and don't allow paths (no slash) */
	    if ((name[0] == '\0') || (strchr(name + 1, '/') != NULL))
	    {
		DmVaDisplayStatus((DmWinPtr)folder, True, (name[0] == '\0') ?
				  TXT_NO_PATH_ALLOWED : TXT_NO_PATH_ALLOWED);
		FREE(target);
		return;
	    }
	    /* Will free target as necessary: */
	    if (SetupFileOp(folder, DM_RENAME, target) == 0)
		BringDownPopup( (Widget)DtGetShellOfWidget(w) );
	}
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(w));
	break;


    case FilePromptHelp:
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_RENAME_SECT);
	break;
    }
}				/* end of RenameCB */


static int
SetupFileOp(DmFolderWindow folder, DmFileOpType type, char * target)
{
    void **item_list;
    int retval;

    /* get list of selected items to be operated on */
    item_list = DmGetItemList((DmWinPtr)folder, ExmNO_ITEM);
    retval = DmSetupFileOp(folder, type, target, item_list);
    FREE(item_list);
    return(retval);
}
/****************************procedure*header*****************************
    SetupFileOp- this function performs common functionality of callback
	routines (CopyCB, MoveCB, etc) to set up parameters appropriately and
	call DmDoFileOp().
*/
int
DmSetupFileOp(DmFolderWindow folder, DmFileOpType type, char * target, void **item_list)
{
    char **		src_list;
    int			src_cnt;
    DmFileOpInfoPtr	opr_info;

    /* get list of selected items to be operated on */
    src_list = DmItemListToSrcList(item_list, &src_cnt);

    if (src_cnt == 0)
    {
	DmVaDisplayStatus((DmWinPtr)folder, True, TXT_NONE_SELECTED);
	XtFree(target);
	return(1);
    }

    /* load parameters into opr_info struct */
    opr_info = (DmFileOpInfoPtr)MALLOC(sizeof(DmFileOpInfoRec));
    opr_info->type		= type;
    opr_info->target_path	= target;
    opr_info->src_path		= strdup(folder->views[0].cp->path);
    opr_info->src_list		= src_list;
    opr_info->src_cnt		= src_cnt;
    opr_info->src_win		= folder;
    opr_info->dst_win		=
	((type == DM_RENAME) &&
	 !(folder->attrs & (DM_B_FOUND_WIN | DM_B_TREE_WIN))) ? folder : NULL;
    opr_info->x			= opr_info->y = UNSPECIFIED_POS;

    /* Determine options.  For Tree view and Found window, srcs can
       have multi-paths.
    */
    opr_info->options = REPORT_PROGRESS | OVERWRITE | OPRBEGIN;
    if (folder->attrs & (DM_B_TREE_WIN | DM_B_FOUND_WIN))
	opr_info->options |= MULTI_PATH_SRCS;

    /* free previous task info structure so that now we are about to do 
       new operation. The reason to keep info. until next opr. is started,
       is; to facilitate undo. We need the info. in TaskInfo structure
       for being able to do undo
    */
    DmFreeTaskInfo(folder->task_id);
    folder->task_id = DmDoFileOp(opr_info, DmFolderFMProc, NULL);

    return(folder->task_id == NULL);
}					/* End of SetupFileOp */

/****************************procedure*header*****************************
    Undo - implements undo of last operation. The undo functionality is
	also implemented with asynchronous processing mechanism.  If the
	previous operation was a DELETE operation then the wastebasket
	function DmMoveFileFromWB()  is called which in turn calls
	DmDoFileOp() to 'undo' asynchrounously.
*/
static void
Undo(DmFolderWinPtr window)
{
    DmTaskInfoListPtr	tip;
    DmFileOpInfoPtr	opr_info;
    char *		tmp_path;
    DmFolderWindow	tmp_win;

    /* See if anything to undo */
    if ( (tip = window->task_id) == NULL)
	return;

    opr_info = tip->opr_info;

    if ((opr_info->cur_src == 0) || (opr_info->task_cnt == 0))
	goto done;

    /* WB will look at options so adjust them now */
    opr_info->options |= UNDO;
    opr_info->options &= ~OVERWRITE;	/* ignore overwrites */

    /* Undo'ing from WB */
    if (IS_WB_PATH(Desktop, opr_info->target_path))
    {
	DmMoveFileFromWB(opr_info);
	goto done;
    }

    if (opr_info->x != UNSPECIFIED_POS)
    {
	opr_info->x = UNSPECIFIED_POS;		/* UNDO can't specify pos. */
	opr_info->y = UNSPECIFIED_POS;
    }
    
    /* Establish new 'src_path' and 'target_path' */
    if (opr_info->attrs & (DM_B_DIR_EXISTS | DM_B_DIR_NEEDED_4FILES))
    {
	tmp_path = opr_info->src_path;
	opr_info->src_path = opr_info->target_path;

	if (opr_info->attrs & DM_B_DIR_NEEDED_4FILES)
	{
	    opr_info->options |= RMNEWDIR;

	    /* Attempt to establish new dst_win when the original target
	       was created by the operation.
	    */
	    opr_info->dst_win = DmQueryFolderWindow(opr_info->target_path, NULL);
	}
	else
	    /* The dst win may not exist anymore */
	    opr_info->dst_win = NULL;

	/* since 'target_path' will be existing dir: */
	opr_info->attrs = DM_B_DIR_EXISTS | DM_B_ADJUST_TARGET;

    } else
    {
	tmp_path =strdup(DmMakePath(opr_info->src_path,opr_info->src_list[0]));

	FREE(opr_info->src_list[0]);
	FREE(opr_info->src_path);
	opr_info->src_list[0] = strdup(basename(opr_info->target_path));
	opr_info->src_path = strdup(dirname(opr_info->target_path));
	FREE(opr_info->target_path);

	/* since 'target_path' will NOT be existing dir */
	opr_info->attrs = 0;
    }
    opr_info->target_path = tmp_path;

    /* Switch src & dst win's. */
    tmp_win = opr_info->src_win;
    opr_info->src_win = opr_info->dst_win;
    opr_info->dst_win = (tmp_win->attrs & DM_B_FOUND_WIN) ? NULL : tmp_win;
    if (tmp_win->attrs & DM_B_FOUND_WIN)
	tmp_win->task_id = NULL;

    (void)DmUndoFileOp(tip);
    return;			/* can't free task info until done */

 done:
    DmFreeTaskInfo(window->task_id);
    window->task_id = NULL;
}					/* end of Undo */

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmEditUndoCB-
*/
void
DmEditUndoCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Undo((DmFolderWindow)DmGetWinPtr(w));

}				/* End of DmEditUndoCB */


/****************************procedure*header*****************************
    DmFileCopyCB-
*/
void
DmFileCopyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    Arg	args[5];

    if (folder->copy_prompt == NULL) {
	/* Align the label containing the list of selected items */
	XtSetArg(args[0], XmNalignment, XmALIGNMENT_BEGINNING);
	file_upper_items[0].args = args;
	file_upper_items[0].numArgs = 1;
	folder->copy_prompt = CreateFilePrompt(folder, &FileCopyPrompt);
	/* register for context-sensitive help */
	DmRegContextSensitiveHelp(GetFileGizmoRowCol(folder->copy_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_COPY_SECT);
    }
    else{
	/* prompt already exists.  
	 *	1) Clear input field
	 *	2) update directory contents
	 * FLH MORE: we really need a way to reset the directory
	 * to the initial value.
	 */
	SetFileGizmoInputField(folder->copy_prompt, "");
	ExpandFileGizmoFilename(folder->copy_prompt);
    }
    UpdateFileGizmoPrompt(folder, folder->copy_prompt);
    MapGizmo(FileGizmoClass, folder->copy_prompt);
}

/****************************procedure*header*****************************
    DmFileDeleteCB-
*/
void
DmFileDeleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow folder = (DmFolderWindow)DmGetWinPtr(w);
	
    DeleteItems(folder,
		(DmItemPtr *)DmGetItemList((DmWinPtr)folder, ExmNO_ITEM),
		UNSPECIFIED_POS, UNSPECIFIED_POS);

}					/* end of DmFileDeleteCB */


/****************************procedure*header*****************************
    DmFileLinkCB-
*/
void
DmFileLinkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    Gizmo		choice;
    Arg			upper_args[5];
    Arg			lower_args[5];

    if (folder->link_prompt == NULL) {
	/* Align the label containing the list of selected items */
	XtSetArg(upper_args[0], XmNalignment, XmALIGNMENT_BEGINNING);
	file_upper_items[0].args = upper_args;
	file_upper_items[0].numArgs = 1;
	/* set menu margin to 0 to avoid squashing radio buttons ul94-10449*/
	XtSetArg(lower_args[0], XmNmarginHeight, 0);
	LinkTypeGiz[0].args = lower_args;
	LinkTypeGiz[0].numArgs = 1;


	folder->link_prompt = CreateFilePrompt(folder, &FileLinkPrompt);
	DmRegContextSensitiveHelp(GetFileGizmoRowCol(folder->link_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_LINK_SECT);
    } else{
	/* prompt already exists.  
	 *	1) Clear input field
	 *	2) update directory contents
	 * FLH MORE: we really need a way to reset the directory
	 * to the initial value.
	 */
	SetFileGizmoInputField(folder->link_prompt, "");
	ExpandFileGizmoFilename(folder->link_prompt);
	/* Reset the link type to the default (move into above else) */
	choice = (Gizmo) QueryGizmo(FileGizmoClass, folder->link_prompt,
				    GetGizmoGizmo, "TypeChoice");
	ManipulateGizmo(ChoiceGizmoClass, choice, ReinitializeGizmoValue);
    }

    UpdateFileGizmoPrompt(folder, folder->link_prompt);
    MapGizmo(FileGizmoClass, folder->link_prompt);
}	/* end of DmFileLinkCB */

/****************************procedure*header*****************************
    DmFileMoveCB-
*/
void
DmFileMoveCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    Arg			args[5];

    if (folder->move_prompt == NULL) {
	/* Align the label containing the list of selected items */
	XtSetArg(args[0], XmNalignment, XmALIGNMENT_BEGINNING);
	file_upper_items[0].args = args;
	file_upper_items[0].numArgs = 1;
	folder->move_prompt = CreateFilePrompt(folder, &FileMovePrompt);
	DmRegContextSensitiveHelp(GetFileGizmoRowCol(folder->move_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_MOVE_SECT);
    }
    else{
	/* prompt already exists.  
	 *	1) Clear input field
	 *	2) update directory contents
	 * FLH MORE: we really need a way to reset the directory
	 * to the initial value.
	 */
	SetFileGizmoInputField(folder->move_prompt, "");
	ExpandFileGizmoFilename(folder->move_prompt);
    }
    UpdateFileGizmoPrompt(folder, folder->move_prompt);
    MapGizmo(FileGizmoClass, folder->move_prompt);
}


/****************************procedure*header*****************************
    DmFileNewCB-
*/
void
DmFileNewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    DmPromptCreateFile(folder);
}

/****************************procedure*header*****************************
    DmFileOpenCB-
*/
void
DmFileOpenCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    register DmItemPtr	item;
    register Boolean	busy_cursor = False;
    int 		num_selected = 0;
    DtAttrs		open_attrs;

    /* 
     * If multiple items are selected, we need to open
     * them in new folders.  If only a single item is selected
     * we do open in place.  (Assuming we are opening directories)
     *
     * As soon as we do OpenInPlace, the item list has been
     * realloc'd, making further iterations invalid.
     */
    XtSetArg(Dm__arg[0], XmNselectCount, &num_selected);
    XtGetValues(folder->views[0].box, Dm__arg, 1);
    open_attrs = (num_selected > 1 ? 0 : DM_B_OPEN_IN_PLACE);

    if (num_selected > 0){
	BUSY_FOLDER(folder);
	for (item = folder->views[0].itp; num_selected > 0; item++)
	    if (ITEM_MANAGED(item) && ITEM_SELECT(item) && 
		ITEM_SENSITIVE(item))
	    {
		num_selected--;
		DmOpenObject((DmWinPtr)folder, ITEM_OBJ(item), open_attrs);
	    }
    }
}					/* end of DmFileOpenCB */

/****************************procedure*header*****************************
    DmFileOpenNewCB- Open in Another window
*/
void
DmFileOpenNewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    register DmItemPtr	item;
    register int	i;
    register Boolean	busy_cursor = False;

    for (i = folder->views[0].nitems, item = folder->views[0].itp; i > 0; i--, item++)
	if (ITEM_MANAGED(item) && ITEM_SELECT(item) && ITEM_SENSITIVE(item))
	{
	    if (!busy_cursor)
	    {
		/* Opens can take awhile so entertain user */
		BUSY_FOLDER(folder);
		busy_cursor = True;
	    }
	    DmOpenObject((DmWinPtr)folder, ITEM_OBJ(item), DM_B_OPEN_NEW);
	}
}					/* end of DmFileOpenNewCB */

/****************************procedure*header*****************************
    DmFilePrintCB-
*/
void
DmFilePrintCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmPrintCB(w, DmGetWinPtr(w), call_data);
}

/****************************procedure*header*****************************
    DmFilePropCB-
*/
void
DmFilePropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Dm__PopupFilePropSheets( (DmFolderWindow)DmGetWinPtr(w) );
}

/****************************procedure*header*****************************
    DmFileRenameCB-
*/

void
DmFileRenameCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(widget);

    /* FLH MORE: MOTIF_FONT  we need to get the bold font and
     * set it on the label */
    if ( (folder->rename_prompt) == NULL )
    {
	FileRenamePrompt.menu->clientData = (XtPointer)folder;
	folder->rename_prompt = CreateGizmo(folder->shell, PopupGizmoClass, 
					    &FileRenamePrompt, NULL, 0);
	DmRegContextSensitiveHelp(GetPopupGizmoRowCol(folder->rename_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_RENAME_SECT);
    } else{
	Gizmo	rename_text;
	rename_text = QueryGizmo(PopupGizmoClass, folder->rename_prompt,
				 GetGizmoGizmo, "renameText");
	SetInputGizmoText(rename_text, "");
    }
    MapGizmo(PopupGizmoClass, folder->rename_prompt);
}

/*
 * DmFileConvertD2UCB-
 *	Convert file from dos to unix .
 */

void
DmFileConvertD2UCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(widget);

    /* FLH MORE: MOTIF_FONT  we need to get the bold font and
     * set it on the label */
    if ( (folder->convertd2u_prompt) == NULL )
    {
	FileConvertD2UPrompt.menu->clientData = (XtPointer)folder;
	folder->convertd2u_prompt = CreateGizmo(folder->shell, PopupGizmoClass, 
					    &FileConvertD2UPrompt, NULL, 0);
	DmRegContextSensitiveHelp(
		GetPopupGizmoRowCol(folder->convertd2u_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_CONVERT_SECT);
    } else{
	Gizmo	convert_text;
	convert_text = QueryGizmo(PopupGizmoClass, folder->convertd2u_prompt,
				 GetGizmoGizmo, "ConvertD2UText");
	SetInputGizmoText(convert_text, "");
    }
    MapGizmo(PopupGizmoClass, folder->convertd2u_prompt);
}

/*
 * DmFileConvertU2DCB-
 *	Convert file from unix to dos .
 */

void
DmFileConvertU2DCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(widget);

    /* FLH MORE: MOTIF_FONT  we need to get the bold font and
     * set it on the label */
    if ( (folder->convertu2d_prompt) == NULL )
    {
	FileConvertU2DPrompt.menu->clientData = (XtPointer)folder;
	folder->convertu2d_prompt = CreateGizmo(folder->shell, PopupGizmoClass, 
					    &FileConvertU2DPrompt, NULL, 0);
	DmRegContextSensitiveHelp(
		GetPopupGizmoRowCol(folder->convertu2d_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_CONVERT_SECT);
    } else{
	Gizmo	convert_text;
	convert_text = QueryGizmo(PopupGizmoClass, folder->convertu2d_prompt,
				 GetGizmoGizmo, "ConvertU2DText");
	SetInputGizmoText(convert_text, "");
    }
    MapGizmo(PopupGizmoClass, folder->convertu2d_prompt);
}


/****************************procedure*header*****************************
    DmFolderOpenDirCB- callback when user presses button on item in
	Folder menu.
*/
void
DmFolderOpenDirCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    char **		path = (char **)client_data;
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    DmFolderWinPtr	window;
    

    /* FLH MORE: what about rooted folders? Let them be returned here*/
    if (window = DmQueryFolderWindow(*path, DM_B_NOT_DESKTOP))
	DmMapWindow((DmWinPtr)window);
    else {
	BUSY_FOLDER(folder);
        if (DmOpenInPlace(folder, *path) == NULL)
            DmVaDisplayStatus((DmWinPtr)folder, True, TXT_OpenErr, *path);
    }
}				/* End of DmFolderOpenDirCB */


/****************************procedure*header*****************************
    DmFolderOpenOtherCB-
*/
void
DmFolderOpenOtherCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);

    if (folder->folder_prompt== NULL)
    {
	folder->folder_prompt = CreateFilePrompt(folder, &FileOpenPrompt);
	DmRegContextSensitiveHelp(GetFileGizmoRowCol(folder->folder_prompt),
		FOLDER_HELP_ID(Desktop), FOLDER_HELPFILE,
		FOLDER_FILE_OPEN_OTHER_SECT);
    }
    else{
	/* prompt already exists.  
	 *	1) Clear input field
	 *	2) update directory contents
	 * FLH MORE: we really need a way to reset the directory
	 * to the initial value.
	 */
	SetFileGizmoInputField(folder->folder_prompt, "");
	ExpandFileGizmoFilename(folder->folder_prompt);
    }

    MapGizmo(FileGizmoClass, folder->folder_prompt);

}				/* End of DmFolderOpenOtherCB */


/****************************procedure*header*****************************
 *   DmFolderOpenParentCB- If the parent folder is open on the desktop,
 *   raise it; otherwise Open it In place
 ***********************************************************************/
void
DmFolderOpenParentCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	folder = (DmFolderWindow)DmGetWinPtr(w);
    char *		path;
    char *		parent;

    if (ROOT_DIR(DM_WIN_PATH(folder)))
	return;

    /* Preserve record of symbolic links in user-traversed path by
     * using window->user_path
     */
    path = strdup(folder->user_path);
    parent = dirname(path);

    /* Open Parent *always* does OpenInPlace, even if Parent is already 
     * open elsewhere. This makes the Parent button an easy way to
     * navigate the filesystem.
     */
    
    (void) DmOpenInPlace((DmFolderWinPtr) folder, parent);
    FREE(path);
}	/* end of DmFolderOpenParentCB */


/****************************procedure*header*****************************
 *   DmGotoDesktopCB- Raise the desktop folder
 *   
 ***********************************************************************/
void
DmGotoDesktopCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmMapWindow((DmWinPtr)DESKTOP_TOP_FOLDER(Desktop));
}	/* end of DmGotoDesktopCB */

/****************************procedure*header*****************************
 * DmGetPrompt-  
 * 	This function retunr user input to prompt
 * 	boxes.  Prompt boxes are presented when the user initiates file
 * 	manipulation (Copy, Move, Link) via the Folder Window menu bar
 */
void
DmGetPrompt(Gizmo gizmo, char **filename_ret)
{

    /* Make sure the file gizmo has expanded the path */
    ExpandFileGizmoFilename(gizmo);
    if (filename_ret)
    	*filename_ret = GetFilePath(gizmo);
}				/* end of DmGetPrompt */


/****************************procedure*header*****************************
    DmUpdatePrompts- update Copy, Move, Link prompts to be in
    sync with selected items in folder
*/
int
DmUpdatePrompts(DmFolderWindow folder)
{

    if (folder->copy_prompt)
       	UpdateFileGizmoPrompt(folder, folder->copy_prompt);
    if (folder->move_prompt)
       	UpdateFileGizmoPrompt(folder, folder->move_prompt);
    if (folder->link_prompt)
       	UpdateFileGizmoPrompt(folder, folder->link_prompt);
}
/****************************procedure*header*****************************
    UpdateFileGizmoPrompt - update Copy, Move, or Link prompt to be
    in sync with the selected items in folder
*/
static int
UpdateFileGizmoPrompt(DmFolderWindow folder, Gizmo gizmo) 
{
    char **             src_list = NULL;
    void **             item_list;
    int                 src_cnt;
    char                formatlist[80];
    Widget		file_items;
    XmString		item_string;
    static char *	multi_copy_label = NULL;
    static char *	multi_move_label = NULL;
    static char *	multi_link_label = NULL;
    static char *	single_file_label = NULL;

    /* get list of selected items to be operated on */
    item_list = DmGetItemList((DmWinPtr)folder, ExmNO_ITEM);
    src_list = DmItemListToSrcList(item_list, &src_cnt);

    FREE((void *)item_list);

    if (!multi_copy_label){
	multi_copy_label = GetGizmoText(TXT_MULTICOPYLABEL);
	multi_move_label = GetGizmoText(TXT_MULTIMOVELABEL);
	multi_link_label = GetGizmoText(TXT_MULTILINKLABEL);
	single_file_label = GetGizmoText(TXT_AS);
    }

    /* Multiple items selected in the folder window */
    if (src_cnt > 1) {
	char *label = (gizmo == folder->copy_prompt ? multi_copy_label :
		       gizmo == folder->move_prompt ? multi_move_label :
		       multi_link_label);
	SetFileGizmoInputLabel(gizmo, label);
	SetFileGizmoInputField(gizmo, "");
	SelectFileGizmoInputField(gizmo);
        sprintf(formatlist, "%s,%s, ...", *src_list, *(src_list++));
    }
    /* Else only single (or No) item selected in the folder window */
    else{
	char *item_name = (src_cnt > 0 ? *src_list : "");
	char *base_item_name = strdup(item_name);  

	SetFileGizmoInputLabel(gizmo, single_file_label);
	SetFileGizmoInputField(gizmo, basename(base_item_name));
	SelectFileGizmoInputField(gizmo);
        sprintf(formatlist, "%s", item_name);

	free(base_item_name);
    }


    _DPRINT3(stderr, "UpdateFileGizmoPrompt: filelist = %s\n", formatlist);


    /* We need to set alignment here because the Label appears to
     * reset its alignment each time the label changes.
     */
    item_string = XmStringCreateLocalized (formatlist);
    file_items = QueryGizmo(FileGizmoClass, gizmo, GetGizmoWidget,
			    "file_items");
    XtSetArg(Dm__arg[0], XmNlabelString, item_string);
    XtSetArg(Dm__arg[1], XmNalignment, XmALIGNMENT_BEGINNING);
    XtSetValues(file_items, Dm__arg, 2);
    XmStringFree(item_string);
}

/**************************************************************************
 *	CheckPath: do a stat on a path.  
 *	
 *	INPUT:  path
 *		stat buffer (or NULL)
 *	OUTPUT: 0 if stat succeeded (returned 0)
 *		errno if stat failed
 **************************************************************************/
static int
CheckPath(char *path, struct stat * stat_buf)
{
    struct stat		my_stat_buf;
    int status;

    if (stat_buf == NULL)
	stat_buf = &my_stat_buf;
    errno = 0;
    status = (stat(path, stat_buf) == 0) ? 0 : errno;
    return(status);
} 	/* end of CheckPath */
