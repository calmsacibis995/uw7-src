#pragma ident	"@(#)dtm:f_tree.c	1.131.2.1"

/******************************file*header********************************
 *
 *    Description: This file contains the source code for tree view.
 */
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <FlatP.h>		/* to get normal_gc from Flat FLH REMOVE*/
#include <buffutil.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/FileGizmo.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"


/*****************************file*variables******************************
 *
 * Declare these struct here before forward declarations below since some
 * functions use these types.
 */

typedef struct _Node {
    DmObjectRec	obj;
    u_short	level;
    Boolean	subdirs_shown;
    Boolean	has_subdirs;
} Node;

typedef Bufferof(Node)		Tree;


/**************************forward*declarations***************************
 *
 * Forward Procedure declarations
 */

static void	AddNode(char * path, Node * parent, Node * old_node);
static void	AddToTree(Tree * branch, Cardinal indx);
static void	BeginHere(DmFolderWindow, DmItemRec *);
static void     BeginOtherCB(Widget, XtPointer, XtPointer);
static void	BeginParent(DmFolderWindow tree_win);
static void	BeginTree(char * path);
static void	CloseTreeViewCB(Widget, XtPointer, XtPointer);
static int	Compare(const void *, const void *);
static char *	GetObjectLabel(char *name, DmContainerPtr cp, Node *node);
static void	GetIconLabels(Tree *root, DmItemPtr items);
static int	I18NCompare(const void *, const void *);
static void	CreateIcons(Tree *, DmItemRec **);
static Tree *	CreateTree(char * path, u_short level, u_short depth);
static void	DeleteBranch(Tree * root, Cardinal indx, Boolean inclusive);
static void	DeleteFromTree(Cardinal, Boolean inclusive);
static void	DeleteIcons(DmItemRec **, Cardinal *, Cardinal, int);
static void	ExposureEH(Widget, XtPointer, XEvent *, Boolean *);
static void	Find(Node *, Cardinal, char *, Node **, Node **);
static void	FixLinks(Tree * root, DmItemRec * items);
static void	FreeTree(Node * root_node, DmItemPtr items, int cnt);
static char *	HideFolders(DmFolderWindow tree_win, Cardinal item_index);
static void	InsertIcons(DmItemRec **, Cardinal *, Cardinal, DmItemRec *, int);
static Node *	ParentNode(Node *);
static char *	ShowAllFolders(DmFolderWindow tree_win, Cardinal item_index);
static char *	ShowFolders(DmFolderWindow, Cardinal, u_short depth);
static void	ViewMenuProc(Widget, DmTreeOption);
static void	ViewMenuShowCB(Widget, XtPointer, XtPointer);
static void	ViewMenuHideCB(Widget, XtPointer, XtPointer);
static void	ViewMenuShowAllCB(Widget, XtPointer, XtPointer);
static void	ViewMenuStartHereCB(Widget, XtPointer, XtPointer);
static void	ViewMenuStartMainCB(Widget, XtPointer, XtPointer);
static void	ViewMenuStartOtherCB(Widget, XtPointer, XtPointer);
static void	ViewMenuUpOneLevelCB(Widget, XtPointer, XtPointer);
static void	WalkTree(Tree *, char *, u_short, u_short);
static void	WMCB(Widget w, XtPointer client_data, XtPointer call_data);
static void	InitObj(DmObjectPtr op);
static int	path_strcmp(char * p1, char * p2);


/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */
#define TreeWin			TREE_WIN(Desktop)
#define TreePath(win)		DM_WIN_PATH(win)
#define IBOX()			TreeWin->views[0].box

#define INIT_NODE(node, node_name, node_level, stat_buf, init_obj) \
    (node)->obj.container	= TREE_WIN(Desktop)->views[0].cp; \
    (node)->obj.name		= strdup(node_name); \
    (node)->obj.ftype		= DM_FTYPE_DIR; \
    (node)->obj.fcp		= fmodeKey->fcp; \
    (node)->obj.attrs		= 0; \
    (node)->obj.plist.count	= 0; \
    (node)->obj.plist.ptr	= NULL; \
    (node)->level		= node_level; \
    (node)->subdirs_shown	= False; \
    (node)->has_subdirs		= False; \
    if (init_obj) DmInitObj(&(node)->obj, stat_buf, DM_B_INIT_FILEINFO);\
	else InitObj(&(node)->obj)

#define NODE(item)		( (Node *)ITEM_OBJ(item) )
#define ITEM_NODE(items, i)	NODE(items + i)
#define ROOT_NODE(root_item)	NODE(TreeWin->views[0].itp)  /* Note: 'item' unused */
#define BUF_NODE(buf, indx)	( (buf).p + indx )
#define IS_SYMLINK(node)	( (node)->obj.attrs & DM_B_SYMLINK )
#define NAME(node)		( (node)->obj.name )
#define PATH(node)		( ROOT_DIR(NAME(node)) ? \
				 NAME(node) : DmObjPath(&(node)->obj) )

/* NOTE: A ptr to a FmodeKey struct for a tree_win (dir type) is saved
 *   statically while in tree view.  Perhaps this can remain.  The small icon
 *   is used for tree view.
 *
 * NOTE: ICON_WIDTH,ICON_HEIGHT only valid after DmInitSmallIcons called.
 */
static DmFmodeKeyPtr fmodeKey;
#define ICON_WIDTH	fmodeKey->small_icon->width
#define ICON_HEIGHT	fmodeKey->small_icon->height
#define HORIZ_PAD	XmConvertUnits(DESKTOP_SHELL(Desktop), XmHORIZONTAL, \
				       Xm100TH_MILLIMETERS, 400, XmPIXELS)
#define VERT_PAD	XmConvertUnits(DESKTOP_SHELL(Desktop), XmVERTICAL, \
				       Xm100TH_MILLIMETERS, 400, XmPIXELS)

/* WARNING: SHADOW, HIGHLIGHT, BORDER, and Y_DELTA macros cannot be
 * used before creation of the FIconBox (DmCreateIconContainer).  They depend
 * upon FIconBox resource values
 */
#define SHADOW		PPART(IBOX()).shadow_thickness
#define HIGHLIGHT	PPART(IBOX()).highlight_thickness
#define BORDER		( SHADOW + HIGHLIGHT )
#define Y_DELTA		(Dimension)((int)ICON_HEIGHT + (int)VERT_PAD \
                        + (int) (2 * BORDER))


/* Items are displayed in "virtual" window within scrolled window widget.
 * FPART.[xy]_offset is used to adjust positions within that window.
 */
#define ADJ_X(X)		( (X) - FPART(IBOX()).x_offset )
#define ADJ_Y(Y)		( (Y) - FPART(IBOX()).y_offset )

/* Make node name by removing "root" path from fullpath. */
#define MakeNodeName(fullpath, root_len) \
    strcpy(Dm__buffer, (fullpath) + (root_len) + 1)

/* Note: src->used must be incremented just before calling InsertIntoBuffer
 * (then decremented) to work around func's string-dependency.
 */
#define InsertBuffer(target, src, indx) \
	(src)->used++; \
	InsertIntoBuffer((Buffer *)(target), (Buffer *)(src), indx); \
	(src)->used--

#define GrowBuf(buf, inc)	GrowBuffer(buf, inc); (buf)->used += inc

/* Put these extern function prototypes here instead of in extern.h to avoid
 * compiler warning, "dubious tag declaration", about use of 'stat *'.
 */
extern void	DmInitObj(DmObjectPtr, struct stat *, DtAttrs);

static Boolean fmap_busy = False;

/******************************gizmo*structs*******************************
    Gizmo struct's for Tree view window
*/

#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U     (XtPointer)DM_B_UNDO

static MenuItems FMapFileMenuItems[] = {
  { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   I_PUSH_BUTTON,  	NULL, 
	DmFileOpenCB,    B_M },
  { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
  { True, TXT_FILE_EXIT,   TXT_M_Exit,   I_PUSH_BUTTON,	NULL, 
	CloseTreeViewCB, B_A },
  { NULL }
};

MENU("fmapfilemenu", FMapFileMenu);

static MenuItems FMapViewMenuItems[] = {
  { True, TXT_TREE_SHOW,    TXT_M_TREE_SHOW, 	I_PUSH_BUTTON, 	NULL, 
	ViewMenuShowCB,  B_M },
  { True, TXT_TREE_HIDE,    TXT_M_TREE_HIDE,     I_PUSH_BUTTON,	NULL, 
	ViewMenuHideCB,  B_M },
  { True, TXT_TREE_SHOW_ALL,TXT_M_TREE_SHOW_ALL, I_PUSH_BUTTON,	NULL, 
	ViewMenuShowAllCB,  B_M },
  { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
  { True, TXT_TREE_HERE,    TXT_M_TREE_HERE,     I_PUSH_BUTTON,	NULL, 
	ViewMenuStartHereCB,  B_O },
  { True, TXT_TREE_MAIN,    TXT_M_TREE_Main,     I_PUSH_BUTTON,	NULL, 
	ViewMenuStartMainCB,  B_A },
  { True, TXT_TREE_OTHER,   TXT_M_TREE_OTHER,    I_PUSH_BUTTON,	NULL, 
	ViewMenuStartOtherCB,  B_A },
  { True, "", " ", I_SEPARATOR_1_LINE},	/* FLH REMOVE */
  { True, TXT_TREE_UP,      TXT_M_TREE_UP,       I_PUSH_BUTTON,	NULL, 
	ViewMenuUpOneLevelCB,  B_A },
  { NULL }
};

MENU("fmapviewmenu", FMapViewMenu);

static MenuItems FMapEditMenuItems[] = {
 {False,TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     I_PUSH_BUTTON,	NULL, 
      DmEditUndoCB,       B_U},
  { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
  { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   I_PUSH_BUTTON,	NULL, 
	DmFileCopyCB,    B_M },
  { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   I_PUSH_BUTTON,	NULL, 
	DmFileMoveCB,    B_M },
  { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   I_PUSH_BUTTON,	NULL, 
	DmFileLinkCB,    B_M },
  { True, TXT_FILE_RENAME, TXT_M_FILE_RENAME, I_PUSH_BUTTON,	NULL, 
	DmFileRenameCB,  B_O },
  { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, I_PUSH_BUTTON,	NULL, 
	DmFileDeleteCB,  B_M },
  { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 {True, TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   I_PUSH_BUTTON,	NULL, 
      DmEditSelectAllCB,  B_A},
 {True, TXT_EDIT_UNSELECT, TXT_M_Unselect, I_PUSH_BUTTON,	NULL, 
      DmEditUnselectAllCB,B_M},
  { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
  { True, TXT_FILE_PROP,   TXT_M_Properties,   I_PUSH_BUTTON,	NULL, 
	DmFilePropCB,    B_M },
 {NULL}
};

MENU("fmapeditmenu", FMapEditMenu);

static MenuItems FMapHelpMenuItems[] = {
 { True, TXT_HELP_FMAP,     TXT_M_HELP_FMAP,     I_PUSH_BUTTON,	NULL, 
       DmHelpSpecificCB, B_A },
 { True, TXT_HELP_M_AND_K,  TXT_M_HELP_M_AND_K,  I_PUSH_BUTTON,	NULL, 
       DmHelpMAndKCB,    B_A },
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON,	NULL, 
       DmHelpTOCCB,      B_A },
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON,	NULL, 
       DmHelpDeskCB,     B_A },
 { NULL }
};

MENU("fmaphelpmenu", FMapHelpMenu);

static MenuItems FMapMenuBarItems[] = {
  { True, TXT_FILE, TXT_M_FILE, I_PUSH_BUTTON, &FMapFileMenu,  
	NULL}, 
  { True, TXT_EDIT, TXT_M_EDIT, I_PUSH_BUTTON, &FMapEditMenu,  
	NULL},
  { True, TXT_VIEW, TXT_M_VIEW, I_PUSH_BUTTON, &FMapViewMenu,  
	NULL},
  { True, TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, &FMapHelpMenu,  
	NULL},
  { NULL }
};

MENU_BAR("menubar", FMapMenuBar, DmMenuSelectCB, 0, 0);

#undef B_A
#undef B_O
#undef B_M
#undef B_U

static ContainerGizmo swin_gizmo = { NULL, "swin", G_CONTAINER_SW };
static GizmoRec fmap_gizmos[] = {
  { ContainerGizmoClass,	&swin_gizmo }
};
static MsgGizmo footer = {NULL, "footer", " ", " "};

static BaseWindowGizmo FMapWindow = {
    NULL,			/* help */
    "foldermap",		/* shell widget name */
    TXT_FOLDER_MAP,		/* title */
    &FMapMenuBar,		/* menu bar */
    fmap_gizmos,		/* gizmo array */
    XtNumber(fmap_gizmos),	/* # of gizmos in array */
    &footer,			/* footer */
    "",				/* icon_name */
    "fmap48.icon",		/* name of pixmap file */
};

/* Define Gizmo structs for "BeginOther" prompt */

static MenuItems  BeginOtherMenubarItems[] = {
  MENU_ITEM(TXT_OK,	TXT_M_OK,	NULL),
  MENU_ITEM(TXT_CANCEL,	TXT_M_CANCEL,	NULL),
  MENU_ITEM(TXT_HELP,	TXT_M_HELP,	NULL),
  { NULL }
};
MENU_BAR("filemapmenu", BeginOtherMenubar, BeginOtherCB, 0, 1);

static FileGizmo BeginOtherPrompt = {
      NULL,			/* help */
      "filemapprompt", 		/* name */
      TXT_FMAP_OPEN_TITLE,	/* title */
      &BeginOtherMenubar, 	/* menu */
      NULL, 0, 			/* upperGizmos, numUpper */
      NULL, 0, 			/* lowerGizmos, numLower */
      TXT_FMAP_DIR_CAPTION, 	/* pathLabel (Start At:) */	
      TXT_FMAP_FOLDER_LABEL, 	/* inputLabel (Quick Start At:)*/
      FOLDERS_ONLY,		/* dialogType */
      ".",			/* directory */
      ABOVE_LIST		/* lower gizmo position */
      };


/***************************private*procedures****************************
 *
 *    Private Procedures
 */

/****************************procedure*header*****************************
    path_strcmp- compare path strings
*/
static int
path_strcmp(char * p1, char * p2)
{
    int		result;

    if (strcmp(setlocale(LC_COLLATE, NULL), "C") != 0) /* not in C locale */
	result = strcoll(p1, p2);
    else
    {
	char *	fullpath1 = strdup( p1 );
	char *      fullpath2 = strdup( p2 );
	char *      p;

	for (p = fullpath1; *p; p++)
	   if (*p == '/')
	      *p = '\1';

	for (p = fullpath2; *p; p++)
	   if (*p == '/')
	      *p = '\1';

	result = strcmp(fullpath1, fullpath2);

	FREE(fullpath1);
	FREE(fullpath2);
    }

    return(result);

} /* end of path_strcmp */

/****************************procedure*header*****************************
    AddNode- given non-NULL parent node and unique path, add node for 'path'.
*/
static void
AddNode(char * path, Node * parent, Node * old_node)
{
    Node *	node;
    Node	new;
    Tree	branch;
    int		root_len;

    if (!parent->has_subdirs)		/* parent now has subdirs */
	parent->has_subdirs = True;

    if (!parent->subdirs_shown)		/* and subdirs are shown */
	parent->subdirs_shown = True;

    /* Find insertion point.  We know there is no duplicate */
    for (node = parent + 1;
	 node < ROOT_NODE(TreeWin) + TreeWin->views[0].nitems; node++)
	if (path_strcmp(PATH(node), path) > 0)
	    break;

    /* Add new node before 'node' */
    if ( (root_len = strlen(TreePath(TreeWin))) == 1 )
	root_len = 0;
    if (old_node == NULL)
    {
	char *dup_path = STRDUP(path);

	/* Initialize new node */
	if (IS_NETWARE_PATH(dirname(dup_path))) {
		INIT_NODE(&new, MakeNodeName(path, root_len),
			parent->level + 1, NULL, False);
	} else {
		INIT_NODE(&new, MakeNodeName(path, root_len),
			parent->level + 1, NULL, True);
	}
	FREE(dup_path);
    } else
    {
	new = *old_node;
	new.obj.objectdata = NULL;
	new.obj.name = strdup(MakeNodeName(path, root_len));
	new.level = parent->level + 1;
	new.subdirs_shown = False;	/* though it might HAVE subdirs */
    }

    /* Insertion requires using buffers so use pseudo one here. */
    branch.p	= &new;
    branch.size	= branch.used = 1;
    branch.esize= sizeof(Node);
    AddToTree(&branch, (--node) - ROOT_NODE(TreeWin));

}					/* end of AddNode */

/****************************procedure*header*****************************
    AddToTree- add 'branch' to tree AFTER index 'indx'.  Insert the
	node, make and insert icons, and fix links.
*/
static void
AddToTree(Tree * branch, Cardinal indx)
{
    Tree	root;
    DmItemPtr	items = NULL;

    /* Insertion requires using buffers so make use pseudo buffer here that
       points to the "root" of all the nodes.
    */
    root.p	= ROOT_NODE(TreeWin);
    root.size	= root.used = TreeWin->views[0].nitems;
    root.esize	= sizeof(Node);

    InsertBuffer(&root, branch, indx + 1);
    CreateIcons(branch, &items);
    InsertIcons(&TreeWin->views[0].itp, &TreeWin->views[0].nitems, indx, items, branch->used);
    FixLinks(&root, TreeWin->views[0].itp);
    FREE((void *)items);
}					/* end of AddToTree */

/****************************procedure*header*****************************
    BeginHere- called to re-root tree after user selects "Begin Here" from
	View menu or Icon menu.
*/
static void
BeginHere(DmFolderWindow tree_win, DmItemRec * item)
{
    char * path;

    /* If item is already root of the tree, return now */
    if (item - tree_win->views[0].itp == 0)
    {
	DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_ALREADY_HERE);
	return;
    }

    path = strdup( PATH(NODE(item)) );	/* since Dm__buffer used */
    BeginTree(path);
    FREE(path);
}					/* end of BeginHere */


/****************************procedure*header*****************************
    BeginOtherCB- called when user selects "Open" on "Start At Other" prompt.
*/
static void
BeginOtherCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *menu_data = (MenuGizmoCallbackStruct *) client_data;
    DmFolderWindow	tree_win;
    char * 		foldername;
    struct stat		stat_buf;
    int			status;
    Boolean		skip_stat = False;

    switch(menu_data->index)
    {
    case FilePromptTask:
	tree_win = TREE_WIN(Desktop);	
	DmGetPrompt(tree_win->folder_prompt, &foldername);

	/* Check if new path is same as current path */
	if ((ROOT_DIR(NAME(ROOT_NODE(tree_win))) &&
		IS_SAME_PATH(foldername, "/")) ||
		IS_SAME_PATH(DmMakePath(TreePath(tree_win),
		NAME(ROOT_NODE(tree_win))), foldername))
	{
		DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_ALREADY_HERE);
		break;
	}
	if (IS_NETWARE_PATH(foldername))
		skip_stat = True;
	else
		status = stat(foldername, &stat_buf); 
	/* Be sure input is a directory */
	if (skip_stat || (status == 0) && S_ISDIR(stat_buf.st_mode)) {
	    BeginTree(foldername);
	    /* FLH REMOVE this ?? */
	    BringDownPopup( (Widget)DtGetShellOfWidget(widget) );

	} else
	    DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_NOT_DIR, 
					foldername);
	if (foldername != NULL)
	    FREE(foldername);
	break;

    case FilePromptCancel:
	XtPopdown( (Widget)DtGetShellOfWidget(widget) );
	break;

    case FilePromptHelp:
     DmDisplayHelpSection(DmGetHelpApp(FMAP_HELP_ID(Desktop)), NULL,
		FMAP_HELPFILE, FMAP_BEGIN_OTHER_SECT);
	break;
    }
}					/* End of BeginOtherCB */


/****************************procedure*header*****************************
    BeginParent- called when user selects "Next Level Up" from View menu.
*/
static void
BeginParent(DmFolderWindow tree_win)
{
    Node *	node = ROOT_NODE(tree_win);
    char *	path;

    /* If this is already "/", return now */
    if (ROOT_DIR( NAME(node) ))
    {
	DmVaDisplayStatus((DmWinPtr)tree_win, True, TXT_NO_PARENT);
	return;
    }
    path = strdup( PATH(node) );
    BeginTree(dirname(path));
    FREE(path);
}					/* end of BeginParent */

/****************************procedure*header*****************************
 *  BeginTree- root the tree view at 'path'
 *  
 *  Note: be careful about freeing tree_win path since this may be
 *  the same as 'path' passed in (so save it in 'old_path').
 */
static void
BeginTree(char * path)
{
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    char *		old_path = TreePath(tree_win);
    char *		bname;
    Tree		root;
    Boolean		netware_dir = False;
    
    if (DM_WIN_USERPATH(tree_win) != NULL)
	FREE(DM_WIN_USERPATH(tree_win));

    /* Change the path of the (pseudo) container. "/" is special*/
    if (ROOT_DIR(path))
    {
	TreePath(tree_win) = strdup(path);
	bname = path;
	
    } else
    {
	char * dir = strdup(path);

	if (IS_NETWARE_PATH(path)) {
		tree_win->attrs |= DM_B_NETWARE_WIN;
		netware_dir = True;
	}
	TreePath(tree_win) = strdup(dirname(dir));
	FREE(dir);
	bname = basename(path);
    }
    DM_WIN_USERPATH(tree_win) = strdup(TreePath(tree_win));
    
    /* Now create "root" node and create tree and icons */
    root.size	= root.used = 0;
    root.esize	= sizeof(Node);
    root.p	= NULL;
    
    /* Make "starter" node and init with path and level '0' */
    GrowBuf((Buffer *)&root, 1);
	INIT_NODE(BUF_NODE(root, 0), bname, 0, NULL, netware_dir ? False : True);
    
    /* Only create branch if "root" is not a symbolic link */
    if ( !IS_SYMLINK(BUF_NODE(root, 0)) )
    {
	Tree * branch = CreateTree(path, 1, 1 + TREE_DEPTH(Desktop) - 1);
	
	if (!BufferEmpty(branch))
	{
	    BUF_NODE(root, 0)->has_subdirs = True;
	    BUF_NODE(root, 0)->subdirs_shown = True;
	    InsertBuffer(&root, branch, 1);
	}
	FreeBuffer((Buffer *)branch);
    }
    
    /* Free previous tree nodes (if any) */
    if (ROOT_NODE(tree_win) != NULL)
	FreeTree(ROOT_NODE(tree_win), tree_win->views[0].itp, tree_win->views[0].nitems);
    
    CreateIcons(&root, &tree_win->views[0].itp);
    tree_win->views[0].nitems = root.used;
    DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
    DmDisplayStatus((DmWinPtr)tree_win);
    
    if (old_path != NULL)
	FREE(old_path);
}					/* end of BeginTree */

/****************************procedure*header*****************************
 * BusyWindow-
 */
static void
BusyWindow(Boolean state)
{
    Widget shell = TreeWin->shell;
    
    fmap_busy = state;

    XDefineCursor(XtDisplay(shell), XtWindow(shell),
		  state ? DtGetBusyCursor(shell) : 
		  DtGetStandardCursor(shell));
    DmBusyWindow(shell, state);
}

/****************************procedure*header*****************************
 * CloseTreeView-
 */
static void
CloseTreeViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmUnmapWindow((DmWinPtr)TREE_WIN(Desktop));
    
}					/* end of CloseTreeViewCB */

/****************************procedure*header*****************************
 * Compare-
 */
static int
Compare(const void * arg1, const void * arg2)
{
    Node *	node1 = (Node *)arg1;
    Node *	node2 = (Node *)arg2;
    char *	fullpath1 = strdup( NAME(node1) );
    char *      fullpath2 = strdup( NAME(node2) );
    int		result;
    
    result = path_strcmp(fullpath1, fullpath2);
    
    FREE(fullpath1);
    FREE(fullpath2);
    
    return(result);
}



/****************************procedure*header*****************************
 * I18NCompare-
 */
static int
I18NCompare(const void * arg1, const void * arg2)
{
    Node *	node1 = (Node *)arg1;
    Node *	node2 = (Node *)arg2;
    char *	fullpath1 = strdup( NAME(node1) );
    char *	fullpath2 = strdup( NAME(node2) );
    int		result;
    
#ifdef USE_MBLEN_BASED_ALGORITHM
    char *      p;
    int		len;
    
    p = fullpath1;
    
    while (*p) {
	if ((len = mblen(p, MB_LEN_MAX)) > 1)
	    p += len;
	else {
	    if (*p == '/')
		*p = '\1';
	    p++;
	}
    }
    
    p = fullpath2;
    
    while (*p) {
	if ((len = mblen(p, MB_LEN_MAX)) > 1)
	    p += len;
	else {
	    if (*p == '/')
		*p = '\1';
	    p++;
	}
    }
#endif /* USE_MBLEN_BASED_ALGORITHM */
    
    result = strcoll(fullpath1, fullpath2);
    
    FREE(fullpath1);
    FREE(fullpath2);
    
    return(result);
}

/****************************procedure*header*********************************
 * CreateIcons- create corresponding item icons given 'tree' 
 * 
 * Layout Method: The folder map presents folders in short view.  
 *	Items are presented as (small) folder icons with a label to the 
 *	right of the icon. Items are laid out one-per-row, with the 
 *	indentation of each item indicating its depth in the tree.  Each 
 *	item is positioned by determining the (x,y) position of the folder
 *	glyph. It's index in the item list determines Y position; it's
 *	depth in the tree determines X position.  In addition to the folder 
 *	glyph width and height and horizontal and vertical pads, the position 
 *	is affected by 1) the margin which surrounds the whole folder map and 
 *	2) the thickness of the shadow and highlight which surround each item.
 *
 *	X = h_pad + depth * (glyph_w + h_pad + shadow + highlight);
 *	Y = v_pad + item_num *(glyph_h + v_pad + 2*shadow + 2*highlight);
 *
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 */
static void
CreateIcons(Tree * tree, DmItemRec ** items)
{
    DmItemRec *		item;
    Node *		node;
    WidePosition	y;
    Widget		icon_box = IBOX();
    Dimension 		pad = IPART(icon_box).hpad;
    Dimension		margin = HORIZ_PAD;
    Dimension		x_inc = ICON_WIDTH + margin + BORDER;
    Dimension		height = ICON_HEIGHT + 2 * BORDER;
    Dimension		y_inc = Y_DELTA;

#define TXT_WIDTH(I) DmTextWidth(icon_box, (I))
    
    *items = (DmItemRec *)
	REALLOC((void *)*items, tree->used * sizeof(DmItemRec));
    item	= *items;
    y		= VERT_PAD;

    GetIconLabels(tree, *items);
    for (node = BUF_NODE(*tree, 0); node < BUF_NODE(*tree, tree->used); node++)
    {
	item->x			= (XtArgVal)margin + x_inc * node->level;
	item->y			= (XtArgVal)y;
	item->icon_width	= (XtArgVal)ICON_WIDTH + pad + TXT_WIDTH(item)
	                           + 2 * BORDER;
	item->icon_height	= (XtArgVal)height;
	item->managed		= (XtArgVal)True;
	item->select		= (XtArgVal)False;
	item->sensitive		= (XtArgVal)True;
	item->client_data	= (XtArgVal)NULL;
	item->object_ptr	= (XtArgVal)node;	
	item++;
	y += y_inc;
    }
#undef TXT_WIDTH
}					/* end of CreateIcons */

/****************************procedure*header*****************************
  CreateTree- generate a list of directory tree nodes starting at 'path'.
  Consider the level of this branch to be 'root_level'.  Extend tree
  to up to BUT EXCLUDING 'terminate_level'.  (WalkTree will set the
  level of each branch node according to the level of the "root").
  
  Note: caller is responsible for freeing tree.
  */
static Tree *
CreateTree(char * path, u_short root_level, u_short terminate_level)
{
    Tree * root = (Tree *)AllocateBuffer(sizeof(Node), 0);
    
    WalkTree(root, path, root_level, terminate_level);
    
    if (!BufferEmpty(root)) {
	qsort((void *)root->p, (size_t)root->used,
	      (size_t)sizeof(Node),
	      strcmp(setlocale(LC_COLLATE, NULL), "C") ? I18NCompare : Compare);
    }
    
    return (root);
}					/* end of CreateTree */

/****************************procedure*header*****************************
  DeleteBranch- delete nodes which represent branch "below" 'indx'.
  */
static void
DeleteBranch(Tree * root, Cardinal indx, Boolean inclusive)
{
    Node *	node;
    u_short	root_level = BUF_NODE(*root, indx)->level;
    Cardinal	num_nodes = root->used;
    Cardinal	start;
    Cardinal	end;
    
    start = inclusive ? indx : indx + 1;	/* preserve 'indx' */
    
    for (end = start, node = BUF_NODE(*root, start);
	 end < num_nodes; end++, node++)
    {
	if ((end > indx) && (node->level <= root_level))
	    break;
	
	FREE( NAME(node) );
	DtFreePropertyList(&node->obj.plist);
	XtFree( node->obj.objectdata );		/* FileInfo */
    }
    
    /* Ripple down any remaining nodes */
    if (end < num_nodes)
    {
	Node *	src;
	Node *	dst;
	
	for (src = BUF_NODE(*root, end), dst = BUF_NODE(*root, start);
	     src < BUF_NODE(*root, num_nodes); src++, dst++)
	    *dst = *src;
    }
    
    end -= start;		/* Compute number of nodes in branch */
    /* Shrink buffer by number of nodes in branch */
    GrowBuf((Buffer *)root, -end);
    
}					/* end of DeleteBranch */

/****************************procedure*header*****************************s
  DeleteFromTree- common code to remove nodes from tree (from HideFolders
  and UpdateTreeView, for instance).  'inclusive' means delete the
  'indx' node and its branch (if any), otherwise, just delete the
  branch.
  
  NOTE: if last dir of parent is deleted, must update 'has_subdirs'
  */
static void
DeleteFromTree(Cardinal indx, Boolean inclusive)
{
    Tree	root;
    
    root.p	= ROOT_NODE(TreeWin);
    root.size	= root.used = TreeWin->views[0].nitems;
    root.esize	= sizeof(Node);		/* to be safe but probably not used */

    DeleteBranch(&root, indx, inclusive);
    DeleteIcons(&TreeWin->views[0].itp, &TreeWin->views[0].nitems,
		inclusive ? indx : indx + 1, TreeWin->views[0].nitems - root.used);
    FixLinks(&root, TreeWin->views[0].itp);
}					/* end of DeleteFromTree */

/****************************procedure*header*****************************
  DeleteIcons- delete 'count' number of icons starting at 'item_index'.
  Shrink buffer of items and (as a convenience) set the new number of
  items into 'num_items'.  Free the icon label.
  */
static void
DeleteIcons(DmItemRec ** items, Cardinal * num_items, Cardinal item_index,
		int count)
{
    int move_cnt = *num_items - count - item_index;
    DmItemRec *	item;
    
    for (item = *items + item_index; item < *items + count; item++) {
	FREE_LABEL(item->label);
    }
    
    if (move_cnt > 0)		/* are there any to the "right" ? */
    {
	Dimension	y_dec = count * Y_DELTA;
	
	/* First, adjust their 'y' values */
	for (item = *items + item_index + count;
	     item < *items + *num_items; item++)
	    item->y -= y_dec;
	
	
	/* Now move them 'left' */
	(void)memmove(*items + item_index,
		      *items + item_index + count,
		      move_cnt * sizeof(DmItemRec));
    }
    *num_items -= count;		/* dec number of items */
    
    /* Realloc items array smaller */
    *items = (DmItemRec *)REALLOC((void *)*items, *num_items*sizeof(DmItemRec));
    
}					/* end of DeleteIcons */

#define ABOVE			( 1 << 0 )
#define BELOW			( 1 << 1 )
#define RIGHT			( 1 << 2 )
#define LEFT			( 1 << 3 )
#define X_IN_VIEW(X)		( !((X) & (LEFT | RIGHT)) )
#define Y_IN_VIEW(Y)		( !((Y) & (ABOVE | BELOW)) )

#define VIEW_TOP(VIEW)		( (VIEW)->y )
#define VIEW_BOTTOM(VIEW)	( (VIEW)->y + (VIEW)->height )
#define VIEW_LEFT(VIEW)		( (VIEW)->x )
#define VIEW_RIGHT(VIEW)	( (VIEW)->x + (VIEW)->width )

/****************************procedure*header*********************************
 * HorizIntersect-
 */
static Boolean
HorizIntersect(WidePosition x1, WidePosition x2,	/* x2 < x1 (left of) */
	       WidePosition y, XRectangle * view)
{
    int y_code =
	(y < VIEW_TOP(view)) ? ABOVE : (y > VIEW_BOTTOM(view)) ? BELOW : 0;
    int x1_code =
	(x1 < VIEW_LEFT(view)) ? LEFT : (x1 > VIEW_RIGHT(view)) ? RIGHT : 0;
    int x2_code =
	(x2 < VIEW_LEFT(view)) ? LEFT : (x2 > VIEW_RIGHT(view)) ? RIGHT : 0;

    return(Y_IN_VIEW(y_code) &&
	   (X_IN_VIEW(x1_code) || X_IN_VIEW(x2_code) ||
	    ((x1_code & RIGHT) && (x2_code & LEFT))));
}

/****************************procedure*header*********************************
 * VertIntersect-
 */
static Boolean
VertIntersect(WidePosition y1, WidePosition y2,		/* y2 < y1 (above) */
	      WidePosition x, XRectangle * view)
{
    int x_code =
	(x < VIEW_LEFT(view)) ? LEFT : (x > VIEW_RIGHT(view)) ? RIGHT : 0;
    int y1_code =
	(y1 < VIEW_TOP(view)) ? ABOVE : (y1 > VIEW_BOTTOM(view)) ? BELOW : 0;
    int y2_code =
	(y2 < VIEW_TOP(view)) ? ABOVE : (y2 > VIEW_BOTTOM(view)) ? BELOW : 0;

    return(X_IN_VIEW(x_code) &&
	   (Y_IN_VIEW(y1_code) || Y_IN_VIEW(y2_code) ||
	    ((y1_code & BELOW) && (y2_code & ABOVE))));
}

/****************************procedure*header*********************************
 * GetPoints-
 */
static int
GetPoints(DmItemPtr item, DmItemPtr parent, XRectangle * view, XPoint * points)
{
    WidePosition	x1, x2, y1, y2;
    WidePosition	origin_x, origin_y;
    int			line_cnt = 0;

    origin_x = ADJ_X(ITEM_X(item));		/* upper left relative to */
    origin_y = ADJ_Y(ITEM_Y(item));		/*  view and can be negative */

    /* Compute points for horiz line (draw thru shadow & highlight) */
    x1 = origin_x + BORDER;
    x2 = origin_x - HORIZ_PAD - ICON_WIDTH/2;
    y1 = origin_y + ICON_HEIGHT/2 + BORDER;
    if (HorizIntersect(x1, x2, y1, view))
	line_cnt++;

    /* Compute final 'y' for vert line to parent.
     * 
     * NOTE: Folder icons are not square and there's no way of knowing where
     * in the pixmap the bottom of the folder is so the number below is
     * 'empirical'
     */
    y2 = ADJ_Y(ITEM_Y(parent)) + BORDER + ICON_HEIGHT - 2;
    if (VertIntersect(y1, y2, x2, view))
    {
	if (y1 > VIEW_BOTTOM(view))
	    y1 = VIEW_BOTTOM(view);
	if (y2 < VIEW_TOP(view))
	    y2 = VIEW_TOP(view);

	if (line_cnt)			/* first return horiz line coord's */
	{
	    points[0].x = (short)x1;
	    points[0].y = (short)y1;
	    points++;
	}
	line_cnt++;
	points[0].x = points[1].x = (short)x2;	/* return vert line coord's */
	points[0].y = (short)y1;
	points[1].y = (short)y2;

    } else if (line_cnt)			/* return horiz line coord's */
    {
	points[0].x = (short)x1;
	points[1].x = (short)x2;
	points[0].y = points[1].y = (short)y1;
    }
    
    return (line_cnt ? ++line_cnt : 0);		/* return point count */
}

/****************************procedure*header*********************************
 * 	DrawLines- Draw horizontal and vertical lines to connect parents 
 *	and children. Each parent has a vertical line descending from the 
 *	horizontal center of its folder glyph.  For each child, there is 
 *	a horizontal line extending from the parent's vertical line to 
 *	the vertical center of the child's folder glyph.  The x,y values
 *	for the lines are determined by the grid used to lay out the 
 *	items, the folder glyph dimensions, the horizontal and vertical
 *	pads between items and the thickness of the shadow and highlight
 *	surrounding each item.  (See CreateIcons for layout calculations.)
 *	Note that the lines extend through the item highlights and shadows
 *	to the very edge of the folder glyphs.
 *	
 *	Horizontal center of parent (x): child_x - (glyph_w/2 + h_pad)
 *	Vertical center of child (y): child_y + (glyph_h/2 + shadow +highlight)
 *
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 */
static void
DrawLines(DmItemPtr * item, DmItemPtr parent, DmItemPtr end, XRectangle * view)
{
    u_short	child_level = NODE(parent)->level + 1;

    while (((*item) < end) && (NODE(*item)->level >= child_level))
    {
	if (NODE(*item)->level > child_level)
	    DrawLines(item, (*item) - 1, end, view);
	
	else		/* consider drawing line from child to parent */
	{
	    XPoint	points[3];	/* at most 2 lines */
	    int		num_points = GetPoints(*item, parent, view, points);
	    if (num_points)
	    {
		Widget	ibox = IBOX();
		GC	gc = FPART(ibox).normal_gc;

		XDrawLines(XtDisplay(ibox), XtWindow(ibox),
			   gc, points, num_points, CoordModeOrigin);
	    }
	    (*item)++;
	}
    }
}					/* end of DrawLines */

#ifdef NOT_USED
    /* Now, if node has subdirs, indicate this with a "dangling" branch
     */
    if (node->has_subdirs && !node->subdirs_shown)
    {
	int i;
	points[0].x = points[1].x = x + ICON_WIDTH/2 + BORDER;
	points[0].y = y + ICON_HEIGHT + BORDER;
	points[1].y = points[0].y + VERT_PAD/2;

	/* Adjust positions for offset of FIconBox virtual window
	 * within scrolled window 
	 */
	for (i=0; i<2; i++){
	    points[i].x -= FPART(ibox).x_offset;
	    points[i].y -= FPART(ibox).y_offset;
	}
	       
	XDrawLines(XtDisplay(ibox), XtWindow(ibox),
		   gc, points, 2, CoordModeOrigin);
    }
#endif
	    
/****************************procedure*header*****************************
  ExposureEH- event handler for exposes on FIconBox.  If there are
  sub-nodes,  draw the connecting lines.
  */
static void
ExposureEH(Widget ibox, XtPointer client_data, XEvent* xevent, Boolean* cont)
{
    DmFolderWindow	tree_win = (DmFolderWindow)client_data;
    XExposeEvent *	ev = (XExposeEvent *)xevent;
    DmItemPtr		item;
    XRectangle		view;
    
    if (tree_win->views[0].nitems < 2)
	return;
    
    view.x	= ev->x;	/* put expose region in convenient form */
    view.y	= ev->y;
    view.width	= ev->width;
    view.height	= ev->height;
    item	= DM_WIN_ITEM(tree_win, 1);	/* 1st descendant */
    DrawLines(&item, tree_win->views[0].itp,
	      DM_WIN_ITEM(tree_win, tree_win->views[0].nitems), &view);
    
}					/* end of ExposureEH */

/****************************procedure*header*****************************
  Find- find 'path' in tree.  Since tree is sorted, search can be
  optimized: at each node, 'path' must compare >= node path.
  */
static void
Find(Node * start, Cardinal cnt, char * path, Node ** node, Node ** parent)
{
    char *	dup_path = strdup(path);
    char *	parent_path = dirname(dup_path); 
    Boolean	root_found = False;
    
    for (*parent = start; *parent < (start + cnt); (*parent)++)
    {
	char *	node_path = PATH(*parent);
	int	result = path_strcmp(parent_path, node_path);
	
	if (result == 0)
	{
	    /* Tree no longer in true sorted order so search not OPTIMIZED */
	    root_found = True;
	    break;
	}
    }
    if (!root_found)
	*parent = NULL;
    
    FREE(dup_path);

    /* Assuming valid dir path as input, loop would not fall thru; comparison
     * would fail first.
     */
    if ((*parent != NULL) && (*parent)->subdirs_shown)
    {
	/* 'parent' is set.  Now get 'node' */
	for (*node = (*parent) + 1; *node < (start + cnt); (*node)++)
	{
	    int result = path_strcmp(PATH(*node), path);
	    if (result > 0)
		break;
	    if (result == 0)
		return;
	}
    }

    /* Assuming valid dir path as input, loop would not fall thru; comparison
     * would fail first.
     */
    *node = NULL;
}					/* end of Find */

/****************************procedure*header*****************************
    FixLinks- FIconBox items must point to "object's".  The "object" data is
	stored in the tree nodes.  The items point to the tree nodes
	initially when the tree (and items) are built from scratch.  When the
	tree nodes change (as from Hide/Show Folders), however, the
	pointers in the items must be fixed up.

*/
static void
FixLinks(Tree * root, DmItemRec * items)
{
    DmItemRec *	item = items;
    Node *	node = BUF_NODE(*root, 0);
    Cardinal	i;

    for (i = root->used; i != 0; i--)
	item++->object_ptr = (XtArgVal)node++;

}					/* end of FixLinks */

/****************************procedure*header*****************************
    FreeTree-
*/
static void
FreeTree(Node * root_node, DmItemPtr items, int cnt)
{
    Node * node;
    int i;

    for (node = root_node; cnt != 0; cnt--, node++)
    {
	if (NAME(node) != NULL)
	    FREE(NAME(node));
	DtFreePropertyList(&node->obj.plist);
	FREE_LABEL(items[cnt-1].label);
    }

    if (root_node != NULL)
	FREE((void *)root_node);
}					/* end of FreeTree */

/*****************************************************************************
 *  	GetObjectLabel: get the (localized) label for an object
 *			store the object class in the node for the object
 *			store the IconLabel property in the object proplist
 *			(this will make the File Property sheet work)
 *	INPUTS:	object name (basename)
 *		container (real container from DmOpenDir)
 *		node
 * 	OUTPUT: label for object
 *	CAUTION: Memory is allocated for the returned label; the
 *	caller must free this memory.
 */
static char *
GetObjectLabel(char *name, DmContainerPtr cp, Node *node)
{
    DmFnameKeyPtr fnkp;
    DtPropList plist;
    char *label;

    if (!cp)
	return(strdup(name));

    plist.ptr = (DtPropPtr) NULL;
    plist.count = 0;

    if (IS_NETWARE_PATH(cp->path)) {
	fnkp = DmGetNetWareServerClass();
    } else {
    	fnkp = DmLookUpObjClass(cp, name, &plist);
    }
    /*
     * This folder belongs to a file class other than folder, store
     * it in the object so the File Property Sheet can use it if
     * needed.
     */
    if (fnkp)
	node->obj.fcp = fnkp->fcp;

    /* 
     * First, look for an instance property
     */
    if (label = DtGetProperty(&plist, ICON_LABEL, NULL)){
	/* Make a copy of the label before freeing the Property List,
	 * and copy the ICONLABEL property into the folder window object 
	 * for later use by the File Property Sheet.  
	 */
	label = strdup(label);
	DtSetProperty(&node->obj.plist, ICON_LABEL, label, (DtAttrs) NULL);
	DtFreePropertyList(&plist);
	return(label);
    }
    DtFreePropertyList(&plist);

    /*
     * Next, look for a class property.
     */
    if (fnkp){
	if (label = DtGetProperty(&(fnkp->fcp->plist), ICON_LABEL, NULL))
	    return(strdup(label));
    }

    /*
     * Just use the object name
     */
    return(strdup(name));
}

/****************************procedure*header*****************************
 * GetIconLabels - Find the labels for the tree nodes 
 * 	For each directory in the tree, open the directory and get
 *	labels for all of its directory children at once.  Loop through
 *	the tree until all directories have been labelled.
 *	
 * Input: Tree, items
 * Output: none
 */
static void
GetIconLabels(Tree *tree, DmItemPtr item_list)
{
    char *path;
    char *base;
    char *parent_dir;
    Node *root;
    Node *node;
    Node *sibling;
    DmContainerPtr cp;
    int	  count;
    char *label_string;

    /* The root of the tree is a special case.  Get its
     * label first.
     */
    root = BUF_NODE(*tree, 0);
    path = strdup(PATH(root));
    if (ROOT_DIR(path)){
	MAKE_LABEL(item_list[0].label, path);
    }
    else{
	base = strdup(basename(path));
	parent_dir = dirname(path);
	cp = DmOpenDir(parent_dir, DM_B_READ_DTINFO);
	label_string = GetObjectLabel(base, cp, root);
	MAKE_LABEL(item_list[0].label, label_string);
	FREE(base);
	FREE(label_string);
	if (cp)
	    DmCloseContainer(cp, DM_B_NO_FLUSH);
    }
    FREE(path);

    /* 
     * Initialize labels 
     */
    for (count=1; count < tree->used; count++)
	item_list[count].label = (XtArgVal) NULL;

    for (count = 1; count < tree->used;){
	/*
	 * Open the container for this node
	 */
	node = BUF_NODE(*tree,count); 
	path = strdup(PATH(node));
	base = strdup(basename(path));
	parent_dir = dirname(path);
	cp = DmOpenDir(parent_dir, DM_B_READ_DTINFO);
	label_string = GetObjectLabel(base, cp, node);
	MAKE_LABEL(item_list[count].label, label_string);
	FREE(base);
	FREE(path);
	FREE(label_string);
	/*
	 * Now, while we have the container open for this
	 * node, use it to get the labels for any siblings.
	 * Rely upon the fact that the labels are sorted to
	 * optimize this search - we have found the last sibling
	 * when the 
	 */
	for (sibling = node+1; 
	     sibling < BUF_NODE(*tree, tree->used) && 
	     sibling->level >= node->level; 
	     sibling++){
	    if (sibling->level == node->level){
		/*
		 * This must be a sibling
		 */
		path = strdup(PATH(sibling));
		base = basename(path);
		label_string = GetObjectLabel(base, cp, sibling);
		MAKE_LABEL(item_list[sibling-root].label, label_string);
		FREE(path);
		FREE(label_string);
	    }
	}
	if (cp)
		DmCloseContainer(cp, DM_B_NO_FLUSH);
	/*
	 * move up to next unlabelled item
	 */
	while (item_list[count].label)
	    count++;
    }
} /* end of GetIconLabels */


/****************************procedure*header*****************************
    HideFolders- called to hide (sub)folders when user selects "Hide" from
	View menu or Icon menu.
*/
static char *
HideFolders(DmFolderWindow tree_win, Cardinal item_index)
{
    Node *	node = ITEM_NODE(tree_win->views[0].itp, item_index);

    /* Point to node and see if no subdirs or subdirs already not shown. */
    if (!node->has_subdirs || !node->subdirs_shown)
	return( !node->has_subdirs ? TXT_NO_SUBDIRS : TXT_ALREADY_HIDDEN );

    node->subdirs_shown = False;
    DeleteFromTree(item_index, False);
    return(NULL);
}					/* end of HideFolders */

/****************************procedure*header*****************************
    InsertIcons-
*/
static void
InsertIcons(DmItemRec ** base, Cardinal * num_items, Cardinal item_index,
	    DmItemRec * items, int count)
{
    Buffer	src;
    Buffer	dst;
    DmItemRec *	item;
    Cardinal	i;
    WidePosition y;
    Dimension	y_inc = Y_DELTA;

    src.size	= src.used = count;
    src.esize	= sizeof(DmItemRec);
    src.p	= (BufferElement *)items;

    dst.size	= dst.used = *num_items;
    dst.esize	= sizeof(DmItemRec);
    dst.p	= (BufferElement *)*base;

    InsertBuffer(&dst, &src, item_index + 1);

    *base	= (DmItemRec *)dst.p;	/* Return new icon array */
    *num_items	= dst.used;		/* A convenience */

    /* Now adjust 'y' in new items and those below */
    for (i = 0, item = *base + item_index + 1,
	 y = (*base)[item_index].y + y_inc;
	 i < dst.used - (item_index + 1);
	 i++, item++, y += y_inc)
	item->y = y;

}					/* end of InsertIcons */

/****************************procedure*header*****************************
    ParentNode-
*/
static Node *
ParentNode(Node * node)
{
    Node * parent = node;

    while ((parent->level > 0) && (parent->level >= node->level))
	   parent--;

    return(parent);
}					/* end of ParentNode */

/****************************procedure*header*****************************
    ShowAllFolders- called to show all (sub)folders when user selects "Show
	All Levels" from View menu or Icon menu.
*/
static char * 
ShowAllFolders(DmFolderWindow tree_win, Cardinal item_index)
{
    Node *	node;
    DmItemPtr	item;
    Cardinal	bottom_index;
    u_short	level;
    Boolean	found;

    node = ITEM_NODE(tree_win->views[0].itp, item_index);	/* Point to this node */

    /* Nothing to do if no subdirs to show or node is a symbolic link */
    if (!node->has_subdirs || IS_SYMLINK(node))
	return( !node->has_subdirs ? TXT_NO_SUBDIRS : TXT_DIR_IS_SYMLINK );

    /* Show subdirs starting at this node and return */
    if (!node->subdirs_shown)
	return( ShowFolders(tree_win, item_index, 0) );

    /* Getting here means node is already showing some subdirs.  Must find
       "leaf" nodes and begin showing from there.  Note: we know it has
       subdirs.  Start at the "bottom" and work up so we don't iterate over
       new branches added (branches are inserted AFTER index).
    */
    level = node->level;
    item = DM_WIN_ITEM((DmFolderWindow)tree_win, item_index + 1);

    /* Break when non-descendant found or end of items */
    while ((item < DM_WIN_ITEM((DmFolderWindow)tree_win, tree_win->views[0].nitems)) &&
	   (((Node *)ITEM_OBJ(item))->level > level))
    {
	item++;
    }

    /* We now have bracketed the descendants of item_index.
       CAUTION: must use index 'bottom_index' here rather than item ptr
       since 'itp' & 'nitems' may change with each call.
    */
    found = False;
    for (bottom_index = item - tree_win->views[0].itp - 1;	/* ie, pre-dec */
	 bottom_index != item_index; bottom_index--)
     {
	node = ITEM_NODE(tree_win->views[0].itp, bottom_index);

	/* Expand non-symlink nodes with subdirs not shown */
	if (node->has_subdirs && !node->subdirs_shown && !IS_SYMLINK(node))
	{
	    (void)ShowFolders(tree_win, bottom_index, 0);
	    found = True;
	}
    }
    return( found ? NULL : TXT_ALL_SHOWN );

}					/* end of ShowAllFolders */

/****************************procedure*header*****************************
    ShowFolders- called to show (sub)folders when user selects "Show" from
	View menu or Icon menu.
*/
static char * 
ShowFolders(DmFolderWindow tree_win, Cardinal item_index, u_short depth)
{
    Tree *	branch;
    Node *	node;
    char *	path;

    /* First, point to node and see if sub folders are already shown. */
    node = ITEM_NODE(tree_win->views[0].itp, item_index);

    /* Nothing to do if no subdirs to show or they're already shown
       or node is a symbolic link
    */
    if (!node->has_subdirs || node->subdirs_shown || IS_SYMLINK(node))
	return(!node->has_subdirs ? TXT_NO_SUBDIRS :
	       node->subdirs_shown ? TXT_ALREADY_SHOWN : TXT_DIR_IS_SYMLINK );

    node->subdirs_shown = True;

    /* Create (sub)tree and insert it */
    path = strdup( PATH(node) );		/* since Dm__buffer used */
    branch = CreateTree(path, node->level + 1,
			(depth == 0) ? 0 : node->level + 1 + depth - 1);
    FREE((void *)path);

    /* Now add branch, make and insert icons, etc. */
    AddToTree(branch, item_index);
    FreeBuffer((Buffer *)branch);

    return(NULL);
}				/* end of ShowFolders */

/****************************procedure*header*****************************
    UpdateDisplay-
*/
static void 
UpdateDisplay(void)
{
    Display *	display = XtDisplay(TreeWin->shell);
    XEvent	xevent;

    XSync(display, False);		/* flush the event queue */

		/* peel off Expose events manually and dispatch them */
    while (XCheckTypedEvent(display, Expose, &xevent) == True)
	XtDispatchEvent(&xevent);

}				/* END OF UpdateDisplay */

/*****************************************************************************
 *  	Callbacks for View Menu: 
 *		ViewMenuShowCB
 *		ViewMenuHideCB
 *		ViewMenuShowAllCB
 *		ViewMenuStartHereCB
 *		ViewMenuStartMainCB
 *		ViewMenuStartOtherCB
 *		ViewMenuUpOneLevelCB
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 */
static void
ViewMenuShowCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_SHOW_SUBS);
}	/* end of ViewMenuShowCB */

static void
ViewMenuHideCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_HIDE_SUBS);
}	/* end of ViewMenuHideCB */

static void
ViewMenuShowAllCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_SHOW_ALL_SUBS);
}	/* end of ViewMenuShowAllCB */

static void
ViewMenuStartHereCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_BEGIN_HERE);
}	/* end of ViewMenuStartHereCB */

static void
ViewMenuStartMainCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_BEGIN_MAIN);
}	/* end of ViewMenuStartMainCB */

static void
ViewMenuStartOtherCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_BEGIN_OTHER);
}	/* end of ViewMenuStartOtherCB */

static void
ViewMenuUpOneLevelCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    ViewMenuProc(w, DM_BEGIN_PARENT);
}	/* end of ViewMenuUpOneLevelCB */

/****************************procedure*header*****************************
    ViewMenuProc- handles actions for all buttons in View menu.
*/
static void 
ViewMenuProc(Widget widget, DmTreeOption option)
{
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    Cardinal		i;
    Boolean		touch_items;

    DmClearStatus((DmWinPtr)tree_win);		/* Clear footer first */

#ifdef FLH_REMOVE
    How do we unpost the menu? Do we need to?

    OlUnpostPopupMenu(XtParent(widget));
#endif
    BusyWindow(True);

    switch (option)
    {
    case DM_SHOW_SUBS:
    case DM_HIDE_SUBS:
    case DM_SHOW_ALL_SUBS:
	touch_items = False;

	/* CAUTION: must use index 'i' here rather than item ptr since
	   'itp' & 'nitems' may change with each call.
	*/ 
	for (i = 0; i < tree_win->views[0].nitems; i++)
	    if (ITEM_MANAGED(tree_win->views[0].itp + i) && ITEM_SELECT(tree_win->views[0].itp + i))
		touch_items |=
		    ((option == DM_SHOW_SUBS) ? ShowFolders(tree_win, i, 1) :
		     (option == DM_HIDE_SUBS) ? HideFolders(tree_win, i) :
		     ShowAllFolders(tree_win, i)) == NULL;

	if (touch_items)
	{
	    DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
	    DmDisplayStatus((DmWinPtr)tree_win);
	}
	break;

    case DM_BEGIN_MAIN:
	/* If already rooted at Main, issue warning and break now */
	if (path_strcmp(DmMakePath(TreePath(tree_win), NAME(ROOT_NODE(tree_win))),
		   DESKTOP_HOME(Desktop)) == 0)
	{
	    DmVaDisplayStatus((DmWinPtr)tree_win, False, TXT_ALREADY_MAIN);
	    break;
	}

	BeginTree(DESKTOP_HOME(Desktop));
	break;
    case DM_BEGIN_OTHER:

	/* Create prompt if necessary and map it.
	   Note: We use the tree_win_prompt field of the tree_win structure
	   given that the Folder Map does not have a Folder Menu.
	*/
	if ( tree_win->folder_prompt == NULL )
	{
	    BeginOtherPrompt.directory = strdup(PATH(ROOT_NODE(tree_win)));
	    tree_win->folder_prompt = 
		CreateGizmo(tree_win->shell, FileGizmoClass,&BeginOtherPrompt,
			    NULL, 0);
	    
	    /* register for context-sensitive help */
	    DmRegContextSensitiveHelp(
		GetFileGizmoRowCol(tree_win->folder_prompt),
		FMAP_HELP_ID(Desktop), FMAP_HELPFILE,
		FMAP_BEGIN_OTHER_SECT);
	}
	MapGizmo(FileGizmoClass, tree_win->folder_prompt);
	break;

    case DM_BEGIN_HERE:
	/* Get index of selected item and begin tree there (or from parent) */
	XtVaGetValues(tree_win->views[0].box, XmNlastSelectItem, &i, NULL);

	/* Safety check for now.  Later, button will be insensitive */
	if (i == ExmNO_ITEM)
	    break;

	BeginHere(tree_win, tree_win->views[0].itp + i);
	break;

    case DM_BEGIN_PARENT:
	BeginParent(tree_win);
	break;

    default:
	break;
    }
    BusyWindow(False);
}					/* end of ViewMenuCB */

/****************************procedure*header*****************************
    WalkTree- this routine is called recursively to traverse a directory
	tree rooted at 'path'.  The "root" level is 'root_level' and
	recursion should continue until a level of 'terminate_level' is
	reached (except when 'terminate_level' is '0' which means recurse
	until all leaf nodes are found; ie., "infinite depth").  The head of
	the tree list, 'head', is realloc'ed for each new node added to the
	list at each subdir entry.
*/
static void
WalkTree(Tree * root, char * path, u_short root_level, u_short terminate_level)
{
    DIR *		dirp;
    struct dirent *	dir_ent;
    Node *		node;
    int			root_len;
    u_short		current_level;
    Cardinal		parent_indx;
    char		*dup_path;
    Boolean		skip_stat;

    if ( (dirp = opendir(path)) == NULL)
    {
	Dm__VaPrintMsg(TXT_OPENDIR, errno, path);
	return;
    }

    UpdateDisplay();

    /* An empty buffer indicates the initial call */
    if (BufferEmpty(root))
    {
	parent_indx	= ExmNO_ITEM;	/* FLH check this */
	current_level	= root_level;

    } else
    {
	parent_indx	= root->used - 1;
	current_level	= BUF_NODE(*root, parent_indx)->level + 1;
    }

    /* Compute length of container path.  This is constant for the duration
       of building a tree so it's unfortunate that it's being re-computed.
       (If tree rooted at "/", ignore it)
    */
    if ( (root_len = strlen(TreePath(TreeWin))) == 1 )
	root_len = 0;

    dup_path = STRDUP(path);
    if (IS_NETWARE_PATH(path) || IS_NETWARE_PATH(dirname(dup_path)))
    {
	skip_stat = True;
    } else {
	skip_stat = False;
    }
    FREE(dup_path);

    while ( (dir_ent = readdir(dirp)) != NULL )
    {
	struct stat	stat_buf;
	char *		fullpath;

	if (dir_ent->d_name[0] == '.')
	    continue;

	fullpath = (char *)MALLOC((size_t)strlen(path) +
				  strlen(dir_ent->d_name) +
				  1 +			/* '/' */
				  1);			/* '\0' */

	(void)strcpy(fullpath, path);
	if (!ROOT_DIR(path))
	    (void)strcat(fullpath, "/");
	(void)strcat(fullpath, dir_ent->d_name);


	if (!skip_stat) {
		if (stat(fullpath, &stat_buf) == -1)
		{
			Dm__VaPrintMsg(TXT_STAT, errno, fullpath);
			FREE((void *)fullpath);
			continue;
		}

		/* Ignore non-directories */
		if ( !S_ISDIR(stat_buf.st_mode) )
		{
			FREE((void *)fullpath);
			continue;
		}
	}

	/* A subdir was found... indicate this to the parent (if any) */
	if (parent_indx != ExmNO_ITEM)
	{
	    /* CAUTION: parent_node is only used locally here.  It is made
	       invalid when buffer is realloced, for instance.
	    */
	    Node * parent_node = BUF_NODE(*root, parent_indx);

	    if ((parent_node != NULL) && !parent_node->has_subdirs)
	    {
		parent_node->has_subdirs = True;

		/*
		 * We've reached the 'terminate_level' (except when
		 * 'terminate_level is '0') and have gone one level of
		 * recursion farther to indicate whether the "leaf"
		 * parent has any subdirs (and we've now found one).
		 */
		if ((current_level > terminate_level) &&
		     (terminate_level != 0))
		{
		    FREE((void *)fullpath);
		    break;
		}
		/* else */
		parent_node->subdirs_shown = True;
	    }
	}

	GrowBuf((Buffer *)root, 1);	/* Alloc node for this dir entry */

	/* CAUTION: parent_node is made invalid after GrowBuf */

	node = BUF_NODE(*root, root->used - 1);	/* point to new node */

	/* Make node name, set path and level. */
	if (skip_stat) {
		INIT_NODE(node, MakeNodeName(fullpath, root_len),
		  current_level, NULL, False);
	} else {
		INIT_NODE(node, MakeNodeName(fullpath, root_len),
		  current_level, NULL, True);
	}
		  
	if (!IS_SYMLINK(node))
		WalkTree(root, fullpath, root_level, terminate_level);
	FREE((void *)fullpath);
    }

    if (closedir(dirp) == -1)
	Dm__VaPrintMsg(TXT_CLOSEDIR, errno);

    return;
}					/* end of WalkTree */

/****************************procedure*header*****************************
    WMCB-  Handle WM_DELETE_WINDOW message from window manager.
    Unmap the Folder Map (for later use) and Icon Menu.  We
    don't need to check for pending folder operations because
    we are not destroying the folder map FolderWindow.
*/
static void
WMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    /* FLH MORE: we only handle WM_DELETE_WINDOW now (see call
     * to XmAddWMProtocolCallback.  Do we need to handle more
     * protocols?  WM_SAVE_YOURSELF?
     */
#ifdef MOTIF_ICON_MENU
    DmBringDownIconMenu((DmWinPtr)TREE_WIN(Desktop));
#endif
    DmUnmapWindow((DmWinPtr)TREE_WIN(Desktop));

}					/* end of WMCB */

/***************************private*procedures****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
    DmDrawTreeIcon- currently just draws same icon as used in "Name" view.
*/
void
DmDrawTreeIcon(Widget widget, XtPointer client_data, XtPointer call_data)
{
    if (!fmap_busy)
	DmDrawNameIcon(widget, client_data, call_data);
}

/****************************procedure*header*****************************
    DmFolderOpenTreeCB- callback when user presses button to open tree view.
*/
void
DmFolderOpenTreeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFolderWindow	window;
    Gizmo    menu;
    Widget   menu_w;
    Widget   form;

    if ( (window = TREE_WIN(Desktop)) != NULL )
    {
	DmMapWindow((DmWinPtr)window);
	return;
    }

    /* Initialize the Folder Window Structure for the  Folder Map.  First
       allocate the Folder structure and next allocate a Container Pointer
       structure.
    */
    window = TREE_WIN(Desktop) = 
	(DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));

    window->views[0].cp = (DmContainerPtr)CALLOC(1, sizeof(DmContainerRec));
    if (window->views[0].cp == NULL)
    {
	Dm__VaPrintMsg(TXT_MEM_ERR);
	return;
    }

    /* Initialize Folder Structure. */
    window->views[0].cp->path	= strdup(DESKTOP_HOME(Desktop));
    window->user_path		= strdup(DESKTOP_HOME(Desktop));
    window->views[0].cp->count	= 1;
    window->views[0].cp->num_objs= 0;
    window->attrs		= DM_B_FOLDER_WIN | DM_B_TREE_WIN;
    window->views[0].view_type	= DM_NAME;		/* sort of */
    window->views[0].sort_type	= DM_BY_NAME;

    /* Set this window as the client_data for sub-menus */
    FMapMenuBar.clientData = (XtPointer)window;

    /* Format the "Main" button with user's home login */
    /* FLH MORE: check this */
    sprintf(Dm__buffer, Dm__gettxt(TXT_TREE_MAIN), DESKTOP_HOME(Desktop));
    FMapViewMenuItems[DM_BEGIN_MAIN].label = strdup(Dm__buffer);

    /* Create The BaseWindow Gizmo that Houses the Folder Map
     * There is only one tree view - hence no need to copy gizmo
     */
    XtSetArg(Dm__arg[0], XmNscrollingPolicy, XmAPPLICATION_DEFINED);
    XtSetArg(Dm__arg[1], XmNvisualPolicy, XmVARIABLE);
    XtSetArg(Dm__arg[2], XmNscrollBarDisplayPolicy, XmSTATIC);
    /* FLH MORE: set the attachments: 
       Should we standardize the shadow width
       and offsets ? Resolution independent?
     */
    XtSetArg(Dm__arg[3], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(Dm__arg[4], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(Dm__arg[6], XmNleftOffset, 4); 
    XtSetArg(Dm__arg[7], XmNrightOffset, 4);
    XtSetArg(Dm__arg[8], XmNshadowThickness, 2);	/* FLH REMOVE */
    XtSetArg(Dm__arg[9], XmNbottomAttachment, XmATTACH_FORM);

    fmap_gizmos->args = Dm__arg;
    fmap_gizmos->numArgs = 10;
    window->gizmo_shell = CreateGizmo(NULL, BaseWindowGizmoClass, 
				&FMapWindow, NULL, 0);
    window->shell = GetBaseWindowShell(window->gizmo_shell);
    /* Get rid of shadow on menu 
       FLH MORE: standardize this.
     */
    menu = GetBaseWindowMenuBar(window->gizmo_shell);
    menu_w = QueryGizmo(MenuBarGizmoClass, menu, GetGizmoWidget, NULL);
    XtSetArg(Dm__arg[0], XmNshadowThickness, 0);
    XtSetValues(menu_w, Dm__arg, 1);

    window->swin  = (Widget)
	QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
		   GetGizmoWidget, "swin");

    /* Size the scrolled window.  Reverse the notion of rows and cols here
     * since Tree view is vertically oriented.
     */
    XtSetArg(Dm__arg[0], XmNwidth, GRID_WIDTH(Desktop)*FOLDER_ROWS(Desktop) +
	     GRID_WIDTH(Desktop) / 2);
    XtSetArg(Dm__arg[1], XmNheight, GRID_HEIGHT(Desktop)*FOLDER_COLS(Desktop));
    XtSetValues(window->swin, Dm__arg, 2);

    XtSetArg(Dm__arg[0], XmNdrawProc,		DmDrawTreeIcon);
    XtSetArg(Dm__arg[1], XmNmovableIcons,	False);
    XtSetArg(Dm__arg[2], XmNdblSelectProc,	DmDblSelectProc);
    XtSetArg(Dm__arg[3], XmNclientData,		window);
    XtSetArg(Dm__arg[4], XmNmenuProc,		DmIconMenuProc);
    XtSetArg(Dm__arg[5], XmNpostSelectProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[6], XmNpostUnselectProc,	DmButtonSelectProc);
    XtSetArg(Dm__arg[7], XmNtargets,		DESKTOP_DND_TARGETS(Desktop)); 
    XtSetArg(Dm__arg[8], XmNnumTargets,XtNumber(DESKTOP_DND_TARGETS(Desktop))); 
    XtSetArg(Dm__arg[9], XmNdropProc,		DmDropProc);
    XtSetArg(Dm__arg[10], XmNtriggerMsgProc,	DmFolderTriggerNotify);
    XtSetArg(Dm__arg[11], XmNconvertProc,	DtmConvertSelectionProc);
    XtSetArg(Dm__arg[12], XmNdragDropFinishProc,DtmDnDFinishProc);


/* FLH REMOVE: Why is this here?  It will cause us to
	do a FreeTree the first time we come up because the
	items are not initialized by CreateIconContainer.  
	(only managed is set to false)  For now, CreateIconContainer
	is initializing the object_data and label fields to NULL.
*/
    window->views[0].nitems = 5;
    window->views[0].box = DmCreateIconContainer(window->swin, 0,
						 Dm__arg, 13,
						 window->views[0].cp->op, 
						 window->views[0].cp->num_objs,
						 &(window->views[0].itp), 
						 window->views[0].nitems,
						 NULL);

    XmProcessTraversal(window->views[0].box, XmTRAVERSE_CURRENT);
    
    /*  We will handle Close requests ourself.  Both the
     *  folder map and the icon menu must be unmapped
     */
    XmAddWMProtocolCallback( window->shell, 
			    XA_WM_DELETE_WINDOW(XtDisplay(window->shell)), 
			    WMCB, (XtPointer) NULL ) ;
    XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
    XtSetValues(window->shell, Dm__arg, 1);

    /* Bring up initial view of tree */
    fmodeKey = DmFtypeToFmodeKey(DM_FTYPE_DIR);	/* set (fixed) FmodeKey */
    DmInitSmallIcons(DESKTOP_SHELL(Desktop));

    /* Set the grid for scrolling of FIconBox: See CreateIcons */
    /* NOTE: similar to DmFormatView.  Should be shared */
    XtSetArg(Dm__arg[0], XmNgridWidth, (ICON_WIDTH + HORIZ_PAD + BORDER)/2);
    XtSetArg(Dm__arg[1], XmNgridHeight,
	     DM_NameRowHeight(window->views[0].box));
    XtSetValues(window->views[0].box, Dm__arg, 2);

    XtAddEventHandler(window->views[0].box,		/* for drawing lines */
		      ExposureMask, False, ExposureEH, (XtPointer)window);

    BeginTree(TreePath(window));

    XtRealizeWidget(window->shell);	/* Realize and Map the window */
    DmMapWindow((DmWinPtr)window);

    /* put something in the status area */
    DmDisplayStatus((DmWinPtr)window);

    /* get a help id for Folder Map to be used with its Help menu. */
    FMAP_HELP_ID(Desktop) = DmNewHelpAppID(XtScreen(window->shell),
	XtWindow(window->shell),
	(char *)dtmgr_title,
	Dm__gettxt(TXT_FOLDERMAP),
	DESKTOP_NODE_NAME(Desktop),
	NULL, "fmap.icon")->app_id;

    /* Register for context-sensitive help.  Since XmNhelpCallback is a MOTIF
     * resource, register help on the child of the toplevel shell instead of
     * the shell itself.
     */
    form = QueryGizmo(BaseWindowGizmoClass, window->gizmo_shell,
	GetGizmoWidget, NULL);
    XtSetArg(Dm__arg[0], XmNuserData, window);
    XtSetValues(form, Dm__arg, 1);
    XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)DmBaseWinHelpKeyCB,
	(XtPointer)window);
}					/* end of DmFolderOpenTreeCB */

/****************************procedure*header*****************************
    TreeIconMenuCB- called for all buttons in Icon menu.
*/
void
TreeIconMenuCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MenuGizmoCallbackStruct *cbs = (MenuGizmoCallbackStruct *)client_data;
    DmItemPtr		item = (DmItemPtr)(cbs->clientData);
    DmFolderWindow	tree_win = TREE_WIN(Desktop);
    char *		err_msg;
    Cardinal		i;

    DmClearStatus((DmWinPtr)tree_win);		/* Clear footer first */
/*
    OlUnpostPopupMenu(XtParent(widget));
*/
    BusyWindow(True);

    switch(cbs->index)
    {
    case DM_IM_OPEN:
    case DM_IM_PROPERTIES:
    case DM_IM_DELETE:
	cbs->clientData = NULL;
	DmIconMenuCB(widget, client_data, call_data);
	break;

    case DM_IM_SHOW_SUBS:
    case DM_IM_HIDE_SUBS:
    case DM_IM_SHOW_ALL_SUBS:
	i = (Cardinal)(item - tree_win->views[0].itp);
	err_msg =
	    ((DmTreeOption)(cbs->index) == DM_IM_SHOW_SUBS) ? 
					ShowFolders(tree_win, i, 1) :
	    ((DmTreeOption)(cbs->index) == DM_IM_HIDE_SUBS) ? 
					HideFolders(tree_win, i) :
		ShowAllFolders(tree_win, i);

	if (err_msg == NULL ) 
	{
	    DmTouchIconBox((DmWinPtr)tree_win, NULL, 0);
    	    DmDisplayStatus((DmWinPtr)tree_win);
	}
	else
	    DmVaDisplayStatus((DmWinPtr)tree_win, True, err_msg);
	break;

    case DM_IM_BEGIN_HERE:
	BeginHere(tree_win, item);
	break;

    case DM_IM_BEGIN_MAIN:
    case DM_IM_BEGIN_OTHER:
    case DM_IM_BEGIN_PARENT:
    default:
	break;
    }
    BusyWindow(False);
}					/* end of TreeIconMenuCB */

/****************************procedure*header*****************************
 *    DmSetFMMenuItems
 */
void
DmSetFMMenuItem(MenuItems ** ret_menu_items, char *name, int index,
						DmFolderWinPtr folder)
{
    static const struct {
	MenuItems * menu_items;
	String		name;
    } info[] = {
    { FMapFileMenuItems, "fmapfilemenu" },
    { FMapEditMenuItems, "fmapeditmenu" },
    { FMapViewMenuItems, "fmapviewmenu" },
    { FMapHelpMenuItems, "fmaphelpmenu" },
    };

    if (index < XtNumber(info))
    {
	*ret_menu_items = info[index].menu_items;
	strcpy(name, info[index].name);
    }
}


/****************************procedure*header*****************************
    Dm__UpdateTreeView- add, delete or modify node in tree view based on
	non-NULL old_path & new_path:

	old_path == NULL  :	add node for 'new_path'
	new_path == NULL  :	delete node with 'old_path'
	else			modify node with 'old_path' to 'new_path'

	For deletions and modifications, 'old_path' may not be found in tree
	since caller may not know if they currently appear in the tree or if
	the path represents a dir.  If found, descendant paths must also be
	processed.
*/
void
Dm__UpdateTreeView(char * old_path, char * new_path)
{
    Node *	node;
    Node *	parent_node;

    if (TreeWin == NULL)	/* return now if tree is not up */
	return;

    if (old_path == NULL)			/* ie. ADDITION */
    {
	/* Get ptr to node and parent node */
	Find(ROOT_NODE(TreeWin), TreeWin->views[0].nitems, new_path, &node, &parent_node);

	if ((parent_node == NULL) ||		/* does not contain parent */
						/* branch not shown: */
	    (parent_node->has_subdirs && !parent_node->subdirs_shown) ||
	    (node != NULL))			/* ignore duplicates */
	    return;

	AddNode(new_path, parent_node, NULL);

    } else if (new_path == NULL)		/* ie. DELETION */
    {
	int	old_len = strlen(old_path);

	/* For deletes, check first to see if the "root" path of tree is
	   same or descendant of path being deleted ('old_path').

	   If 'old_path' matches tree root or the root node, tree must be
	   re-rooted.  The best guess is to re-root it to Main (rather than
	   re-rooting it to the parent of the dir being deleted, for example).
	   If we must re-root but Main is also same or descendant of
	   'old_path', what then??  Do nothing: system is probably in bad
	   state and everything will come down soon anyway.
	*/
	if (DmSameOrDescendant(old_path, PATH(ROOT_NODE(TreeWin)), old_len))
	{
	    if (!DmSameOrDescendant(old_path, DESKTOP_HOME(Desktop), old_len))
		BeginTree(DESKTOP_HOME(Desktop));

	    return;
	}

	/* Get ptr to node and parent node */
	Find(ROOT_NODE(TreeWin), TreeWin->views[0].nitems, old_path, &node, &parent_node);

	if ((parent_node == NULL) ||		/* does not contain parent */
	    (node == NULL))			/* no node for this path */
	    return;

	DeleteFromTree(node - ROOT_NODE(TreeWin), True);

    } else					/* ie. MODIFICATION */
    {
	int	old_len = strlen(old_path);
	Node *	new_node;
	Node *	parent_new_node;

	/* For mods, check first to see if the "root" path of tree is same
	   or descendant of path being modified ('old_path').

	   Must check for affects of by both 'old_path' & 'new_path':
				parent of
		'old_path'	'new_path'
		in tree		not in tree	delete 'old_path' and branch
		not in tree	in tree		add new node for 'new_path'
		in tree		in tree		delete old, add new
		not in tree	not in tree	do nothing
	*/
	if (DmSameOrDescendant(old_path, TreePath(TreeWin), old_len))
	{
	    char * save = TreePath(TreeWin);

	    TreePath(TreeWin) =
		strdup((save[old_len] == '\0') ? new_path :  /* exact match */
		       DmMakePath(new_path, save + old_len + 1));
	    FREE((void *)save);
	    return;
	}

	/* Get ptr to nodes and parent nodes */
	Find(ROOT_NODE(TreeWin), TreeWin->views[0].nitems, old_path, &node, &parent_node);
	Find(ROOT_NODE(TreeWin), TreeWin->views[0].nitems,
	     new_path, &new_node, &parent_new_node);

	if ((node == NULL) && (parent_new_node == NULL))
	    return;

	if ((parent_new_node != NULL) && parent_new_node->subdirs_shown &&
	    (new_node == NULL))		/* Should be */
	{				/* 'new_path' will be in tree */
	    /* Insert new node using data from 'node' (if any) */
	    AddNode(new_path, parent_new_node, node);

	    /* NOTE: for now, must re-find 'old_path' in tree since
	       AddNode may have just realloc'ed the tree.
	    */
	    if (node != NULL)
		Find(ROOT_NODE(TreeWin), TreeWin->views[0].nitems,
		     old_path, &node, &parent_node);
	}

	if (node != NULL)		/* 'old_path' in tree */
	    DeleteFromTree(node - ROOT_NODE(TreeWin), True);
    }

    /* Getting here means tree was 'touched' */
    DmTouchIconBox((DmWinPtr)TreeWin, NULL, 0);
    DmDisplayStatus((DmWinPtr)TreeWin);
}					/* end of Dm__UpdateTreeView */

static void
InitObj(DmObjectPtr op)
{
	op->objectdata = NULL;
	if (IS_DOT_DOT_FILE(op->name))
		op->attrs |= DM_B_HIDDEN;

} /* end of InitObj */
