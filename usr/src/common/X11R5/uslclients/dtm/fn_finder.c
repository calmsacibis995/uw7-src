#pragma ident	"@(#)dtm:fn_finder.c	1.112.2.1"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Intrinsic.h>
#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/ListGizmo.h>
#include <MGizmo/ModalGizmo.h>
#include <MGizmo/NumericGiz.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/FileGizmo.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>	/* for XmAddWMProtocolCallback */

#include <X11/Xmu/Editres.h>	/* FLH REMOVE Editres */

#include "Dtm.h"
#include "dm_strings.h"
#include "error.h"
#include "extern.h"
#include "fn_find.h"
#include "CListGizmo.h"

/************************static procedure headers***************************/

static void	fn_CloseFoundCB(Widget, XtPointer, XtPointer);
static void	fn_MoreFindingCB(Widget, XtPointer, XEvent *, Boolean *);
static void 	fn_InitializeInfoStruct();
static void 	FoundWinWMCB();
static char *	fn_transform();
static void	fn_FindCB();
static void	fn_AddPathToList();
static void	fn_getfind();
static void	fn_HelpCB();
static void	fn_FoundHelpCB();
static void	DmFindCancel();
static Widget	CreateFindWindow();

/*
 * Each member of the Where to Look: list 
 * will have the following structure.
 */
typedef struct pathrec * pathrecptr;

typedef struct pathrec {
	char * path;
	pathrecptr next;
} PathRec;

#define XA	XtArgVal
#define B_A	(XtPointer)DM_B_ANY
#define B_O	(XtPointer)DM_B_ONE
#define B_M	(XtPointer)DM_B_ONE_OR_MORE
#define B_U     (XtPointer)DM_B_UNDO

/*
 * Need defines for types in the Where to Look Field
 */
#define FN_ALLUSERS 1
#define FN_DISK1    2
#define FN_DISK2    3
#define FN_PATH     4

/* Args for row column */
static Arg rc_args[] = {
        {XmNnumColumns, 2},
        {XmNorientation, XmVERTICAL},
        {XmNpacking, XmPACK_COLUMN},
};

/*
 * Menu for the Base Find Window Menu Bar.
 */
MenuItems MainItems[] = {
  { True, TXT_FILE_FIND,TXT_M_FILE_FIND, I_PUSH_BUTTON, NULL, fn_FindCB },
  { True, TXT_CANCEL,	TXT_M_CANCEL,	 I_PUSH_BUTTON, NULL, fn_FindCancelCB },
  { True, TXT_HELP,	TXT_M_HELP,	 I_PUSH_BUTTON, NULL, fn_HelpCB},
  { NULL }
};

MenuGizmo MainMenu = {
       	NULL,			/* help */
       	"MainMenu",		/* shell widget name */
       	NULL,			/* title */
       	MainItems,		/* items */
       	NULL,			/* default selectProc */
       	NULL,			/* default clientData */
       	XmVERTICAL,		/* layout type */
       	1,
	0,			/* default button */
	1,			/* cancel button */
};


/* 
 * Menu definition for the Menu Bar at the
 * Top of the Found Window. "File" "Edit" "Help"
 */
MenuItems FileMenuItems[] = {
 { True, TXT_FILE_OPEN,   TXT_M_FILE_OPEN,   I_PUSH_BUTTON, NULL, DmFileOpenCB,    B_M, False },
 { True, TXT_FILE_PRINT,  TXT_M_FILE_PRINT,  I_PUSH_BUTTON, NULL, DmFilePrintCB,   B_M, False },
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_EXIT,   TXT_M_Exit,   I_PUSH_BUTTON, NULL, fn_CloseFoundCB, B_A, False },
 { NULL }
};

MenuItems EditMenuItems[] = {
 {True, TXT_EDIT_UNDO,     TXT_M_EDIT_UNDO,     I_PUSH_BUTTON, NULL, DmEditUndoCB, B_U, False},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_COPY,   TXT_M_FILE_COPY,   I_PUSH_BUTTON, NULL, DmFileCopyCB,    B_M, False},
 { True, TXT_FILE_MOVE,   TXT_M_FILE_MOVE,   I_PUSH_BUTTON, NULL, DmFileMoveCB,    B_M, False},
 { True, TXT_FILE_LINK,   TXT_M_FILE_LINK,   I_PUSH_BUTTON, NULL, DmFileLinkCB,    B_M, False},
 { True, TXT_FILE_RENAME,   TXT_M_FILE_RENAME,   I_PUSH_BUTTON, NULL, DmFileRenameCB,    B_O, False},
 { True, TXT_FILE_DELETE, TXT_M_FILE_DELETE, I_PUSH_BUTTON, NULL, DmFileDeleteCB,  B_M, False},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 {True,  TXT_EDIT_SELECT,   TXT_M_EDIT_SELECT,   I_PUSH_BUTTON, NULL, DmEditSelectAllCB,  (char *)B_A, False},
 {True,  TXT_EDIT_UNSELECT, TXT_M_Unselect, I_PUSH_BUTTON, NULL, DmEditUnselectAllCB,B_M, False},
 { True, "", " ", I_SEPARATOR_1_LINE, NULL, NULL, NULL},
 { True, TXT_FILE_PROP,   TXT_M_Properties,   I_PUSH_BUTTON, NULL, DmFilePropCB,    B_M, False},
 { NULL }
};

MenuItems HelpMenuItems[] = {
 { True, TXT_HELP_FOLDER,   TXT_M_HELP_FOLDER,   I_PUSH_BUTTON, NULL, fn_FoundHelpCB, B_A, False},
 { True, TXT_HELP_M_AND_K,  TXT_M_HELP_M_AND_K,  I_PUSH_BUTTON, NULL, DmHelpMAndKCB,    B_A, False},
 { True, TXT_HELP_TOC,      TXT_M_HELP_TOC,      I_PUSH_BUTTON, NULL, DmHelpTOCCB,    B_A, False},
 { True, TXT_HELP_HELPDESK, TXT_M_HELP_HELPDESK, I_PUSH_BUTTON, NULL, DmHelpDeskCB, B_A, False},
 { NULL }
};

MENU("filemenu", FileMenu);
MENU("editmenu", EditMenu);
MENU("helpmenu", HelpMenu);

static MenuItems FoundMenuBarItems[] = {
 { True,  TXT_FILE, TXT_M_FILE, I_PUSH_BUTTON, (void *)&FileMenu, NULL},
 { True,  TXT_EDIT, TXT_M_EDIT, I_PUSH_BUTTON, (void *)&EditMenu, NULL},
 { True,  TXT_HELP, TXT_M_HELP, I_PUSH_BUTTON, (void *)&HelpMenu, NULL},
 { NULL }
};

MENU_BAR("menubar", FoundMenuBar, DmMenuSelectCB, 0, 0);

/*
 * Menu Items and Menu Definition for the checkboxes used 
 * to do "Search Remote Folders".
 */
MenuItems RemoteMenuItems[] =
        {
                {(XA)True, " ", NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, True},
		{0}
        };

MenuGizmo RemoteMenu=
        { NULL, "FRemote", "Remote", RemoteMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo  RemoteField= {NULL, "FRemote", &RemoteMenu, G_TOGGLE_BOX};

static GizmoRec Remote_rec[] =  {
        { ChoiceGizmoClass, &RemoteField },
};
static LabelGizmo Remote_label = {
	NULL,               /* help */
	"remote_label",     /* widget name */
	TXT_REMOTE_SEARCH,  /* caption label */
	False,              /* align caption */
	Remote_rec,         /* gizmo array */
	XtNumber(Remote_rec),  /* number of gizmos */
};

/*
 * Menu Items and Menu Definition for the checkboxes used 
 * to ignore blanks.
 */
MenuItems FIgnoreMenuItems[] =
        {
                {(XA)True, " ", NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
		{0}
        };

MenuItems SIgnoreMenuItems[] =
        {
                {(XA)True, " ", NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
		{0}
        };


MenuGizmo FIgnoreMenu=
        { NULL, "FBlank", "FBlank", FIgnoreMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo  FIgnoreField= {NULL, "FCase", &FIgnoreMenu, G_TOGGLE_BOX};

static GizmoRec FIgnore_rec[] =  {
        { ChoiceGizmoClass, &FIgnoreField },
};
static LabelGizmo FIgnore_label = {
	NULL,                   /* help */
	"fignore_label",     /* widget name */
	TXT_IGNORE_CASE,       /* caption label */
	False,                  /* align caption */
	FIgnore_rec,            /* gizmo array */
	XtNumber(FIgnore_rec),  /* number of gizmos */
};

static MenuGizmo SIgnoreMenu=
        { NULL, "SBlank", "SBlank", SIgnoreMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo  SIgnoreField= {NULL, "CCase", &SIgnoreMenu, G_TOGGLE_BOX};

static GizmoRec SIgnore_rec[] =  {
        { ChoiceGizmoClass, &SIgnoreField },
};
static LabelGizmo SIgnore_label = {
	NULL,                   /* help */
	"signore_label",     /* widget name */
	TXT_IGNORE_CASE,       /* caption label */
	False,                  /* align caption */
	SIgnore_rec,            /* gizmo array */
	XtNumber(SIgnore_rec),  /* number of gizmos */
};

static InputGizmo fn_pattern = {
	NULL, "filename", "", 15, NULL, NULL
};

static GizmoRec fn_pattern_rec[] =  {
        { InputGizmoClass, &fn_pattern },
};

static LabelGizmo fn_pattern_label = {
	NULL,                   /* help */
	"fn_pattern_label",     /* widget name */
	TXT_FILE_NAME,       /* caption label */
	False,                  /* align caption */
	fn_pattern_rec,            /* gizmo array */
	XtNumber(fn_pattern_rec),  /* number of gizmos */
};


static InputGizmo fn_content = {
	NULL, "filecontent", "", 15, NULL, NULL
};

static GizmoRec fn_content_rec[] =  {
        { InputGizmoClass, &fn_content },
};

static LabelGizmo fn_content_label = {
	NULL,                   /* help */
	"fn_content_label",     /* widget name */
	TXT_FILE_CONTENT,       		/* caption label */
	False,                  /* align caption */
	fn_content_rec,            /* gizmo array */
	XtNumber(fn_content_rec),  /* number of gizmos */
};

static GizmoRec controlarray1[] = {
	{LabelGizmoClass,	&fn_pattern_label},
	{LabelGizmoClass,	&fn_content_label},
};

static GizmoRec controlarray2[] = {
	{LabelGizmoClass,	&FIgnore_label},
	{LabelGizmoClass,	&SIgnore_label},
};

static ContainerGizmo label1 = {
	NULL, "Label1", G_CONTAINER_RC,
	NULL, NULL, controlarray1, XtNumber (controlarray1),
};

static ContainerGizmo label2 = {
	NULL, "Label2", G_CONTAINER_RC,
	NULL, NULL, controlarray2, XtNumber (controlarray2),
};

static GizmoRec controlarray3[] = {
	{ContainerGizmoClass,	&label1},
	{ContainerGizmoClass,	&label2},
};

static ContainerGizmo label3 = {
	NULL, "Label3", G_CONTAINER_RC,
	NULL, NULL, controlarray3, XtNumber (controlarray3),
};

/*
 *  Motif List Widget (previously Flat List Gizmo)
 */

static ListGizmo fnList = {
	NULL,				/* help info */
	"fnlist",			/* name of widget */
	NULL,				/* items */
	NULL,				/* number of items */
	2,				/* number of items visible */
	NULL,				/* select callback */
	NULL,				/* clientData */
};

static GizmoRec listArray[] = {
	{ListGizmoClass,	&fnList},
};

static ContainerGizmo ListContainer = {
        NULL, "ListContainer", G_CONTAINER_SW,
        NULL, NULL, listArray, XtNumber (listArray),
};

static GizmoRec ListContainerRec[] = {
        {ContainerGizmoClass, &ListContainer}
};
static LabelGizmo fnlistlabel = {
	NULL, 
	"fnlist_label", 
	TXT_WHERE_LOOK, 
	False,                  /* align caption */
	ListContainerRec, 
	XtNumber (ListContainerRec),
	G_LEFT_LABEL,
};

static InputGizmo fn_other = {
	NULL, "otherpath", "", 20, NULL, NULL
};

static GizmoRec fn_other_rec[] =  {
        { InputGizmoClass, &fn_other },
};

static LabelGizmo fn_other_label = {
	NULL,                   /* help */
	"fn_content_label",     /* widget name */
	TXT_ALSO_LOOK,       /* caption label */
	False,                  /* align caption */
	fn_other_rec,            /* gizmo array */
	XtNumber(fn_other_rec),  /* number of gizmos */
};

static NumericGizmo fn_updated = {
	NULL,			/* help */
	"daysupdated",		/* name */ 
	0, 			/* Initial Value */
	0, 			/* min */
	365, 			/* max */
	1, 			/* inc */
	0, 			/* XmNdecimalPoints */
};

static GizmoRec fn_updated_rec[] =  {
        { NumericGizmoClass, &fn_updated },
};

static LabelGizmo fn_updated_label = {
	NULL,                   /* help */
	"fn_content_label",     /* widget name */
	TXT_LAST_UPDATE,       /* caption label */
	False,                  /* align caption */
	fn_updated_rec,            /* gizmo array */
	XtNumber(fn_updated_rec),  /* number of gizmos */
};

static CListGizmo FN_clist = {
	NULL,			/* help */
	"fnclist",		/* name */
        3,			/* view width */
	NULL,			/* required property */
	False,			/* file */
        True,			/* sys class */
	False,                  /* xenix class */
        False,                   /* usr class */
        True,                   /* overridden class */
        False,                   /* exclusives behavior */
	False,                   /* noneset behaviour */
        NULL,         /* select proc */
};

static GizmoRec controlarray4[] = {
        {CListGizmoClass,       &FN_clist},
};

static LabelGizmo label4 = {
	NULL, 			/* help */
	"Label4",		/* widget name */ 
	TXT_IB_FILE_TYPE,	/* caption label */
	False,			/* align caption */
	controlarray4, 		/* gizmo array */
	XtNumber(controlarray4), /* number of gizmos */
};

static GizmoRec fnwindowarray[] = {
	{ContainerGizmoClass,	&label3, rc_args, 3},
	{LabelGizmoClass,       &Remote_label},
	{LabelGizmoClass,	&fnlistlabel},
	{LabelGizmoClass,	&fn_other_label},
	{LabelGizmoClass,	&fn_updated_label},
	{LabelGizmoClass,	&label4},
};

static ContainerGizmo FindContainer = {
        NULL, "FindContainer", G_CONTAINER_RC,
        NULL, NULL, fnwindowarray, XtNumber (fnwindowarray),
};

static GizmoRec FindRec[] = {
        {ContainerGizmoClass, &FindContainer}
};

static PopupGizmo FindWindow = {
        NULL,         		/* help */
        "findwindow",         	/* widget name */
        TXT_FIND_WINDOW,       	/* title */
        &MainMenu,             	/* menu */
        FindRec,              	/* gizmo array */
        XtNumber(FindRec),    	/* number of gizmos */
};

/*
 * Menu for the Popup containing the Stop Button.
 */
MenuItems StopItems[] = {
 { True,  TXT_STOP,   TXT_M_STOP, I_PUSH_BUTTON, NULL, fn_StopCB},
 { NULL }
};

MenuGizmo StopMenu = {
	NULL,
	"stopMenu",
	NULL,
	StopItems,
	NULL,
	NULL,
       	XmVERTICAL,		/* layout type */
       	1,
	0,
};

static ModalGizmo StopNotice = {
	NULL,				/* help */
	"stopnotice",			/* widget name */
	TXT_STOP_WINDOW,		/* title */
	&StopMenu,			/* menu */
	TXT_STOP_SEARCH_TEXT,		/* message */
	NULL, 0,			/* gizmos, num_gizmos */
	XmDIALOG_PRIMARY_APPLICATION_MODAL,/* style */
	XmDIALOG_WORKING		/* type */
};

static ContainerGizmo swin_gizmo = {
		NULL, "swin", G_CONTAINER_SW,
};

static GizmoRec found_gizmos[] = {
	{ContainerGizmoClass,	&swin_gizmo},
};

static MsgGizmo footer = {NULL, "footer", " ", " "};

static BaseWindowGizmo FoundWindow = {
	NULL,			/* help */
	"foundwindow",		/* shell widget name */
	TXT_FOUND_TITLE,	/* title */
	&FoundMenuBar,		/* menu bar */
	found_gizmos,		/* gizmo array */
	XtNumber(found_gizmos), /* # of gizmos in array */
	&footer,                /* MsgGizmo is status & error area */
        TXT_FOUND_TITLE,        /* icon_name (runtime) */
        NULL,                   /* name of pixmap file */
};

/* Find Information Structure:
 * This structure contains all the information that is
 * relavant to one instance of the finder function. Since it
 * is possible for several folders to invoke a find, each
 * time a user selects the find button from a find window 
 * a FindInfoStruct is allocated. 
 */

typedef struct {
	DmFolderWinPtr window; 		/* Found Window */
	XtIntervalId    timeout_id;	/* id returned from add timeout */
	FILE 		* fptr;		/* file ptr of the pipe from popen */
	Boolean 	Complete_Flag;  /* flag indicating find op cmpleted */
	int		yposition; 	/* stores current Found Win y position*/
	DmFolderWinPtr 	FolderWin;      /* The Folder Window invoking find */
	char 		* lastfile;	/* left-over portion of last filename*/
	int		maxlen;		/* last max filename length */
	ModalGizmo	* stopnotice;	/* A modal gizmo is used for stop notice */
	char		* disk1;	/* Store the name of mount point     */
	char		* disk2; 	/* Store the name of mount point     */
} FindInfoStruct, *FindInfoPtr;



/****************************procedure*header*****************************
DmFindCB	- callback for bringing up a Find Prompt Box.  This callback
		is invoked if a user selects the find... button under the file
		menu.  The callback simply calls the CreateFindWindow routine
		which builds a Find Prompt Box and sticks it into the 
		finderWindow PopupGizmo associated with each folder window 
		structure.
*/
void
DmFindCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{

	DmFolderWindow folder = (DmFolderWindow)DmGetWinPtr(widget);

	/* This could take ahile so entertain user */
	BUSY_FOLDER(folder);
	CreateFindWindow(folder);
}

/****************************procedure*header*****************************
    fn_CloseFoundCB-
*/
static void
fn_CloseFoundCB(Widget w, XtPointer client_data, XtPointer call_data)
{
        FindInfoStruct * info = (FindInfoStruct *)client_data;

	DmBringDownIconMenu(info->window);

	if (!info->Complete_Flag)
	{
		XtRemoveTimeOut(info->timeout_id);
                fn_pkill(info->fptr);
		info->Complete_Flag = True;
	}

	if (info->window->copy_prompt != NULL)
		FreeGizmo(FileGizmoClass, info->window->copy_prompt);

	if (info->window->rename_prompt != NULL)
		FreeGizmo(PopupGizmoClass, info->window->rename_prompt);

	if (info->window->move_prompt != NULL) 
		FreeGizmo(FileGizmoClass, info->window->move_prompt);

	if (info->window->link_prompt != NULL) 
		FreeGizmo(FileGizmoClass, info->window->link_prompt);

	FreeGizmo(ModalGizmoClass, info->stopnotice);
        DmCloseContainer(info->window->views[0].cp, DM_B_NO_FLUSH);
        DmDestroyIconContainer(info->window->shell, info->window->views[0].box, 
				info->window->views[0].itp, info->window->views[0].nitems);
	if (info->disk1){
		DtamUnMount(info->disk1);
		FREE(info->disk1);
	}

	if (info->disk2){
		DtamUnMount(info->disk2);
		FREE(info->disk2);
	}

	FREE((void *)info);
}					/* end of fn_CloseFoundCB */

/****************************procedure*header*****************************
    fn_InitializeInfoStruct - routine is invoked each time the user selects the Find
                  button in the Find Prompt Box.  It is called from fn_FindCB
		  and is used to initialize the variables in the FindInfoStruct.
*/
static void
fn_InitializeInfoStruct(FindInfoPtr * info) 
{
	/*
	 * Allocate the needed memory to carry the information
	 * needed.
	 */
   	*info = (FindInfoPtr) MALLOC (sizeof(FindInfoStruct));
	
	/* 
	 * Not guaranteed variables initialized to 0
	 * so they have to be initialized here.
	 */
	((FindInfoPtr)(*info))->lastfile = NULL;
	((FindInfoPtr)(*info))->Complete_Flag = False;
#ifdef MOTIF_FINDER
	((FindInfoPtr)(*info))->yposition = 
			    Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN);
#endif /* MOTIF_FINDER */
	((FindInfoPtr)(*info))->maxlen = 0;
	((FindInfoPtr)(*info))->disk1  = NULL;
	((FindInfoPtr)(*info))->disk2  = NULL;
}

/****************************procedure*header*****************************
 * CreateFindWindow - function is invoked each time the find button in the
 *                    file menu is selected. It is called from DmFindCB.
 * INPUTS:  folder
 * OUTPUTS: none
 * GLOBALS: none
 *
 * The CreateFindWindow function is used to create 
 * the base find window which is drawn whenever the find
 * button is selected from the file menu in a folder window.
 * The window contains a "File Name(s):" field as well as
 * a " Word / Phrase" field.  The input for each of these fields
 * are passed to find and grep respectively. Below these fields are:
 *
 * "Where To Look:" list of relevant folders to search
 * "Also Look In:" alternate folder(s) to search not currently in the list
 * "Days Since File(s) Last Updated:"
 * "File Type:"  The List of File Types on which to search
 * 
 * Action Menu: "Find" "Cancel" "Help"
 * 
 */

static Widget 
CreateFindWindow(folder)
DmFolderWinPtr  folder;
{

#define FIRST 1

   	char		*alias;
	Widget 		fnlist;
	Widget 		tmp_widget;
	CListGizmo 	* fn_clist;
	struct passwd 	*user_entry;
	entryptr        list, loclist, headlist;

	unsigned short 	user_id;
	int 		cnt, i;
	Boolean 	AddBoth = True;

	DmItemPtr      	item;
	XmString	look_str[10];


	/*
	 * If folder already has a Find Prompt Box associated with it,
	 * map the popup....
	 */
	if (folder->finderWindow) {

		((MenuGizmo *)(FindWindow.menu))->clientData = (char *)folder;

		MapGizmo(PopupGizmoClass, folder->finderWindow);
		return;
	}
	/*
	 * ...otherwise, 
	 * Create the Popup Gizmo for the Find Prompt Box now that every
	 * thing is setup.  BEFORE MOTIF - Needed to get the entries for the 
	 * ListItems * in the ListHead before doing this, otherwise would 
         * have been creating the FlatList would 0 entries.
	 */

	((MenuGizmo *)(FindWindow.menu))->clientData = (char *)folder;
	folder->finderWindow = CreateGizmo((Widget)folder->shell, 
					PopupGizmoClass, &FindWindow, NULL, 0);

	/* Check if the path for this folder is the same as the user's
	 * home path in the /etc/passwd file.  This determines if both
	 * the literal "My Folders" and the folder's path are added to
	 * the "Where To Look:" list.
	 */
	user_id = getuid();
	user_entry = getpwuid(user_id);
	if (strlen(folder->views[0].cp->path) == strlen(user_entry->pw_dir))
		if (!strcmp(folder->views[0].cp->path, user_entry->pw_dir))
			AddBoth = False;

	cnt = 0;
	headlist = loclist = list = (entry *) MALLOC (sizeof(entry));
	if (AddBoth) {
	    loclist->name	= strdup(folder->views[0].cp->path);
	    cnt++;

	    loclist->next	= (entry *) MALLOC (sizeof(entry));
	    loclist		= loclist->next;
	}

	/* Add an entry for "My Folders" to the "Where To Look:" list */
	loclist->name		= strdup(Dm__gettxt(TXT_MY_FOLDERS));
	cnt++;

	/* Add an entry for "All Users" to the "Where To Look:" list */
	loclist->next		= (entry *) MALLOC (sizeof(entry));
	loclist			= loclist->next;
	loclist->name		= strdup(Dm__gettxt(TXT_USER_FOLDERS));
	cnt++;

	/* Add an entry for "diskette1/2" to the "Where To Look:" list */
  	alias = DtamGetAlias("diskette1", FIRST);
	if (alias) {
		loclist->next = (entry *) MALLOC (sizeof(entry));
		loclist = loclist->next;
		loclist->name = strdup(alias);
		FREE(alias);
		cnt++;
	}

	alias = DtamGetAlias("diskette2", FIRST);
	if (alias) {
		loclist->next = (entry *) MALLOC (sizeof(entry));
		loclist = loclist->next;
		loclist->name = strdup(alias);
		FREE(alias);
		cnt++;
	}

	/* Add an entry for "The Whole System" to the "Where To Look:" list */
	loclist->next		= (entry *) MALLOC (sizeof(entry));
	loclist			= loclist->next;
	loclist->name		= strdup(Dm__gettxt(TXT_WHOLE_SYSTEM));
	cnt++;

	loclist->next =NULL;
   	setpwent();

	/*
	 * Need to set loclist to the beginning of the list.  This way
	 * We can loop through the list until loclist->next =NULL and 
	 * use the values in the "entry" structure to set up the XmList.
	 */

	loclist = headlist;

	/*
	 * Get the widget for the "daysupdate" NumericGizmo & "otherpath" 
	 * InputGizmothat has been associated with this folder window.
	 * Set rightAttachment to ATTACH_NONE. 
	 */
	tmp_widget = (Widget )
	    QueryGizmo(PopupGizmoClass,
		       folder->finderWindow, GetGizmoWidget, "daysupdated");
        XtSetArg(Dm__arg[0], XmNrightAttachment, XmATTACH_NONE);
        XtSetValues(tmp_widget, Dm__arg, 1);

	tmp_widget = (Widget )
	    QueryGizmo(PopupGizmoClass,
		       folder->finderWindow, GetGizmoWidget, "otherpath");
        XtSetArg(Dm__arg[0], XmNrightAttachment, XmATTACH_NONE);
        XtSetValues(tmp_widget, Dm__arg, 1);
	/*
	 * Get the widget for the XmList that has been associated
	 * with this folder window 
	 */
	fnlist = (Widget )
	    QueryGizmo(PopupGizmoClass,
		       folder->finderWindow, GetGizmoWidget, "fnlist");

	/*
	 * Loop through each of the allocated ListItems and set the
	 * fields in the ListItems (set, fields, clientData) to the 
	 * values that was obtained in the linked list of loclists.
	 */
    	for(i=0; i< cnt; i++)
    	{
	   look_str[i] = XmStringCreateLocalized(GGT(loclist->name));
	   loclist = loclist->next;
     	}
	XmListAddItems(fnlist, look_str, i, 0);
	XmListSelectPos(fnlist, 1, False);

        XtSetArg(Dm__arg[0], XmNselectionPolicy, XmMULTIPLE_SELECT);
        XtSetArg(Dm__arg[1], XmNscrollBarDisplayPolicy, XmSTATIC);
        XtSetValues(fnlist, Dm__arg, 2);

	/*
	 * By default set the first entry in the list to True. The
	 * 1st entry being either the path or the literal "My Folders"
	 * Set the size in the ListHead Structure.
	 */


	fn_clist = (CListGizmo *)QueryGizmo(PopupGizmoClass,
							folder->finderWindow,
							GetGizmoGizmo, 
							"fnclist");
	
	for (item = fn_clist->itp; item < fn_clist->itp + fn_clist->cp->num_objs; item++)
          	item->select = True;

        XtSetArg(Dm__arg[0], XmNitemsTouched, True);
        XtSetArg(Dm__arg[1], XmNselectCount, fn_clist->cp->num_objs);
        XtSetValues(fn_clist->boxWidget, Dm__arg, 2);

	/* register for context-sensitive help */
	XtAddCallback(GetPopupGizmoRowCol(folder->finderWindow),
		XmNhelpCallback, fn_HelpCB, NULL);

	MapGizmo(PopupGizmoClass, folder->finderWindow);

} /* end of CreateFindWindow */

/* ARGSUSED */
/****************************procedure*header*****************************
  fn_FindCancelCB - routine is invoked each time the cancel button in the
                    Find Prompt Box is selected. This routine is a callback
                    registered with the Cancel Button.
*/
static void
fn_FindCancelCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{

	DmFolderWinPtr folder = (DmFolderWinPtr)client_data;

   	XtPopdown(GetPopupGizmoShell(folder->finderWindow));
}


/* ARGSUSED */
/****************************procedure*header*****************************
  fn_FoundHelpCB -  routine is invoked each time the Found Window button in the
                    Help menu of the Found Window is selected. This routine is 
		    a callback registered with the Found Window Button.
*/
static void
fn_FoundHelpCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{

	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL, 
				FOLDER_HELPFILE, FOLDER_FOUND_WIN_SECT);
}


/* ARGSUSED */
/****************************procedure*header*****************************
  fn_HelpCB -  	    routine is invoked each time the Help button at the
                    bottom of the Find Prompt Box is selected. This routine 
		    is a callback registered with the Help Button.
*/
static void
fn_HelpCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL, 
				FOLDER_HELPFILE, FOLDER_FILE_FIND_SECT);
}

static void
fn_scrolldown(info)
FindInfoStruct * info;
{
#ifdef MOTIF_FINDER

	int	vsbproplen, vsbmaxlen;
	Widget  vsb;
                XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
                XtGetValues(info->window->swin, Dm__arg, 1);

/*
                XtSetArg(Dm__arg[0], XtNproportionLength, &vsbproplen);
                XtSetArg(Dm__arg[1], XtNsliderMax, &vsbmaxlen);
                XtGetValues(vsb, Dm__arg, 2);

                XtSetArg(Dm__arg[0], XtNsliderValue, vsbmaxlen - vsbproplen);
                XtSetValues(vsb, Dm__arg, 1);
*/
		OlActivateWidget(vsb, OL_SCROLLBOTTOM, 0);
#endif /* MOTIF_FINDER */
}

static void
fn_scrollup(info)
FindInfoStruct * info;
{
#ifdef MOTIF_FINDER

	Widget  vsb;
                XtSetArg(Dm__arg[0], XtNvScrollbar, &vsb);
                XtGetValues(info->window->swin, Dm__arg, 1);
		OlActivateWidget(vsb, OL_SCROLLTOP, 0);
#endif /* MOTIF_FINDER */
}

/* ARGSUSED */
/****************************procedure*header*****************************
  fn_StopCB -  The StopCB CallBack is invoked as soon as the Stop Button
               in the Finding Window is selected.  The button can be selected
               at any point in the finding process; as such the Complete_Flag
               in the FindInfo Struct structure is essential in determining if
               find process (ie. popen) had completed (Complete_Flag = True)
               or is being interupted.
*/
static void
fn_StopCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{

	FindInfoStruct * info = (FindInfoStruct *)client_data;

	/*
	 * Only Do the Following if the request to terminate the search is
	 * processed before the complete flag is set to True.
	 */
	if (info->Complete_Flag == False) {
		XtRemoveTimeOut(info->timeout_id);
                fn_pkill(info->fptr);
		info->Complete_Flag = True;
	}
       	XtPopdown(GetModalGizmoShell(info->stopnotice));
#ifdef MOTIF_FINDER
	DmBusyWindow(info->window->shell, False,
                        fn_StopKeyCB, (XtPointer)info);
	fn_scrollup(info);
#endif /* MOTIF_FINDER */
	DmDisplayStatus((DmWinPtr)(info->window));
	DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_STOP_SEARCH);
}

/* ARGSUSED */
/****************************procedure*header*****************************
  CreateFoundWindow -  The Create FW is invoked whenever the user selects the
		       find button in the Find Prompt Box.  The routine is 
		       called from the fn_FindCB after all the find selections
              	       are verified and a pipe is opened to the output of a
	      	       find request.  The routine creates a base window with
		       a flat icon box. The View of the entries in the box
		       is only given in long view.
*/
static void
CreateFoundWindow(info)
FindInfoStruct * info;
{
#define INIT_ICON_SLOTS        50
	Widget form;
	Gizmo *base;
	int		n;

	/*
	 * Allocate space for the FolderWindow Structure
	 */
	info->window = (DmFolderWinPtr)CALLOC(1, sizeof(DmFolderWinRec));
	/*
	 * Allocate space for the DmContainer Structure
	 */
        if ((info->window->views[0].cp = 
	(DmContainerPtr)CALLOC(1, sizeof(DmContainerRec))) == NULL) {
		Dm__VaPrintMsg(TXT_MEM_ERR);
                return;
        }

    /*
     * Register for notification of updates to this container
     */
	DtAddCallback(&info->window->views[0].cp->cb_list, UpdateFolderCB ,
                  (XtPointer) info->window);

	/*
	 * Initialize the path to "", this is how a found window id id'ed
	 */
        info->window->views[0].cp->path 	  = strdup("");
        info->window->views[0].cp->count   = 1;
        info->window->views[0].cp->num_objs= 0;

	/* identify as a Folder window */
	info->window->attrs       = DM_B_FOLDER_WIN | DM_B_FOUND_WIN;
	info->window->views[0].view_type   = DM_LONG;
	info->window->views[0].sort_type   = DM_BY_TYPE;


    /* set up resources for the scrolled window:
       
       1) attachments: to make it fill up its parent Form
       2) application-defined scrolling (grid units are determined by view type.
          see DmInitFolderView.)
     */
    	n=0;
    	XtSetArg(Dm__arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
    	XtSetArg(Dm__arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
    /* FLH MORE: should we make leftOffset, shadowThickness resolution independent ? */
    	XtSetArg(Dm__arg[n], XmNleftOffset, 4); n++;	
    	XtSetArg(Dm__arg[n], XmNrightOffset, 4); n++;	
    	XtSetArg(Dm__arg[n], XmNshadowThickness, 2); n++;	/* FLH REMOVE */
    	XtSetArg(Dm__arg[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
    	XtSetArg(Dm__arg[n], XmNvisualPolicy, XmVARIABLE); n++;
    	XtSetArg(Dm__arg[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
    	found_gizmos->args = Dm__arg;
    	found_gizmos->numArgs = n;

	/* Set the client data for submenus */
	FoundMenuBar.clientData = (XtPointer)(info->window);

	/* Get File Menu and set the client data of Exit Button to info */
	FileMenuItems[3].clientData = (char *) info;

	info->window->gizmo_shell = base = CreateGizmo(DESKTOP_SHELL(Desktop), 
					BaseWindowGizmoClass,&FoundWindow,
					  NULL, 0);
	info->window->shell 	  = GetBaseWindowShell(base);
	info->window->swin  = (Widget)QueryGizmo(BaseWindowGizmoClass, base,
					GetGizmoWidget, "swin");
	DmSetSwinSize(info->window->swin);

    /* FLH REMOVE Editres */
    XtAddEventHandler(info->window->shell, (EventMask) 0, True,
                      _XEditResCheckMessages, NULL);
    /* FLH REMOVE Editres */

	XmAddWMProtocolCallback( info->window->shell,
                            XA_WM_DELETE_WINDOW(XtDisplay(info->window->shell)),
                            FoundWinWMCB, (XtPointer) info ) ;
    	XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
    	XtSetValues(info->window->shell, Dm__arg, 1);


	/*
	 * Initialize the number of items to 50 to avoid MALLOCs of
	 * item structures.
	 */
	info->window->views[0].nitems = INIT_ICON_SLOTS;

        n = 0;
        /* Create icon container        */
        XtSetArg(Dm__arg[n], XmNpostSelectProc, DmButtonSelectProc); n++;
        XtSetArg(Dm__arg[n], XmNpostUnselectProc, DmButtonSelectProc); n++;
	XtSetArg(Dm__arg[n], XmNdblSelectProc,  DmDblSelectProc); n++;
        XtSetArg(Dm__arg[n], XmNmenuProc,       DmIconMenuProc); n++;
        XtSetArg(Dm__arg[n], XmNclientData,     info->window); n++; /*MenuProc*/
        XtSetArg(Dm__arg[n], XmNmovableIcons,   False); n++;
        XtSetArg(Dm__arg[n], XmNdrawProc,       DmDrawLongIcon); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight, 	GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth, 	GRID_WIDTH(Desktop) / 2);n++;
	XtSetArg(Dm__arg[n], XmNgridRows, 	2 * FOLDER_ROWS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns, 	2 * FOLDER_COLS(Desktop) ); n++;
	XtSetArg(Dm__arg[n], XmNtargets,    DESKTOP_DND_TARGETS(Desktop)); n++;
	XtSetArg(Dm__arg[n], XmNnumTargets,
                XtNumber(DESKTOP_DND_TARGETS(Desktop))); n++;
	XtSetArg(Dm__arg[n], XmNdropProc,	DmDropProc); n++;
	XtSetArg(Dm__arg[n], XmNtriggerMsgProc, DmFolderTriggerNotify); n++;
        XtSetArg(Dm__arg[n], XmNconvertProc,    DtmConvertSelectionProc); n++;
        XtSetArg(Dm__arg[n], XmNdragDropFinishProc, DtmDnDFinishProc); n++;

        info->window->views[0].box = DmCreateIconContainer(info->window->swin,
                                0,
                                Dm__arg, n, info->window->views[0].cp->op,
                                info->window->views[0].cp->num_objs,
                                &(info->window->views[0].itp),
                                info->window->views[0].nitems, NULL);

	XtRealizeWidget(info->window->shell);
    	{
        	DmViewFormatType type = (info->window->views[0].cp->attrs & DM_B_NO_INFO) ?
            			(DmViewFormatType)-1 : info->window->views[0].view_type;
        	info->window->views[0].view_type = (DmViewFormatType)-1;
        	DmFormatView(info->window, type);
    	}
	/* register for context-sensitive help */
	form = QueryGizmo(BaseWindowGizmoClass, info->window->gizmo_shell,
		GetGizmoWidget, NULL);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)fn_FoundHelpCB,
		NULL);
}



/* ARGSUSED */
static void
fn_getfind(XtPointer client_data, XtIntervalId * id)
{

	FindInfoStruct * info = (FindInfoStruct *)client_data;
        char buffer[5120];
	char * name;
	char * piecefile;
	int nbytes, i, newnbytes, namelen, startlen;
        DmObjectPtr 	op;
	Boolean		AddToFound = True;
	Dimension	pane_width;
	int             adjustbuffer = 80;
	char *next;

	piecefile = NULL;

	/*
	 * Need to Fix:  For now leave space at from of buffer
         * 		 to account for \n as the first thing in buffer
         */
	buffer[5119] = '\0';
        if (!((nbytes=
	read(fileno(info->fptr), buffer+adjustbuffer, 5039))== NULL))
	{
	    /* Since the read is NONBLOCK, need to check if there is currently
	     * anything on the pipe. If there is no data available, and the
	     * pipe is still open - addtimeout ....
	     */
	    if (nbytes == -1) {
		info->timeout_id = XtAddTimeOut(500, fn_getfind, info);
		return;
	    }
	    if (!(buffer[(nbytes - 1) + adjustbuffer] == '\n')) {
		/*
		 * Loop until we find the first newline
		 */
		for(newnbytes = nbytes - 1; 
		    buffer[newnbytes + adjustbuffer] != '\n'; 
		    newnbytes--);
		buffer[nbytes + adjustbuffer] = '\0';
		piecefile = strdup(buffer+adjustbuffer + newnbytes +1);
		buffer[newnbytes + adjustbuffer] = '\0';
	    }
	    else {
		newnbytes = nbytes;
		buffer[newnbytes + adjustbuffer] = '\0';
	    }
	    startlen = info->maxlen;
	    XtVaGetValues(info->window->views[0].box, XmNwidth, &pane_width, NULL);
	    if (buffer[adjustbuffer] == '\n'){
		adjustbuffer = adjustbuffer - strlen(info->lastfile);
		strncpy(buffer + adjustbuffer, 
			info->lastfile,
			strlen(info->lastfile));
		FREE(info->lastfile);
		info->lastfile = NULL;
	    }
#ifdef NOT_ESMP
	    for(i=0, name = strtok(buffer + adjustbuffer, "\n");
		name != NULL; name = strtok(NULL, " \t\n"))
#else
	    for(i=0, name = strtok_r(buffer + adjustbuffer, "\n", &next);
		name != NULL; name = strtok_r(NULL, " \t\n", &next))
#endif
	    {
		namelen = strlen(name);
		if (info->lastfile)
		{
		    char * newname = (char *)
			MALLOC(strlen(info->lastfile) + namelen + 1);
		    strcpy(newname, info->lastfile);
		    strcat(newname, name);
		    FREE(info->lastfile);
		    info->lastfile = NULL;
		    name = newname;
		    namelen = strlen(name);
		}

		op = Dm__CreateObj(info->window->views[0].cp, name, 
				   DM_B_SET_TYPE | DM_B_INIT_FILEINFO);


		/* Can't continue if failed to create obj */
		if (op == NULL)
		{
		    DmVaDisplayStatus((DmWinPtr)info->window, True,
				      StrError(ERR_CreateEntry));
		    break;
		}

		/* Do not want to pass hidden files to DmInitObjType */
		if (op->attrs & DM_B_HIDDEN)
		    continue;

		/* TAKEN FROM DMAddObjToFolder */
		DmAddObjectToView(info->window, 0, op, UNSPECIFIED_POS, UNSPECIFIED_POS);
	    }
	    XtVaGetValues(info->window->views[0].box, XmNwidth, 
							&pane_width, NULL);
	    DmComputeLayout(info->window->views[0].box, 
				info->window->views[0].itp, 
				info->window->views[0].nitems,
                    		info->window->views[0].view_type, pane_width,
                    		0, 0);
	    DmTouchIconBox((DmWinPtr)info->window, NULL, 0);


		/* Put in Check to see if y dimension has maxed out */
		/* The y coordinate is limited to a signed short 2**15 */
	    if (!AddToFound) {
		fn_pkill(info->fptr);
		info->Complete_Flag = True;
        	XtPopdown(GetModalGizmoShell(info->stopnotice));
#ifdef MOTIF_FINDER
		DmBusyWindow(info->window->shell, False,
				fn_StopKeyCB, (XtPointer)info);
		fn_scrollup(info);
#endif /* MOTIF_FINDER */
		DmDisplayStatus((DmWinPtr)(info->window));
		DmVaDisplayStatus((DmWinPtr)(info->window), True, TXT_SEARCH_TERMINATED);
        	return;
	    }

	    if (piecefile)
		info->lastfile = piecefile;

	    /* put something in the status area */
	    DmDisplayStatus((DmWinPtr)(info->window));

	    info->timeout_id = XtAddTimeOut(500, fn_getfind, info);

	} else
	{
#ifdef MOTIF_FINDER 
	    fn_scrollup(info);
#endif					/* MOTIF_FINDER */
	    /* If got here there is no more to read and the timer has not
	     * been enabled, so, close the pipe and there is no need to
	     * XtRemoveTimeout.
	     */
	    fn_pclose(info->fptr);
	    info->Complete_Flag = True;
	    XtPopdown(GetModalGizmoShell(info->stopnotice));
#ifdef MOTIF_FINDER 
	    DmBusyWindow(info->window->shell, False,
			 fn_StopKeyCB, (XtPointer)info);
#endif					/* MOTIF_FINDER */
	    DmDisplayStatus((DmWinPtr)info->window);
	    DmVaDisplayStatus((DmWinPtr)info->window, False, TXT_COMPLETE_SEARCH);
	    return;
	}
}

/*
 * The FindingCB routine is invoked after an event is received
 * indicating that a mapped event of the notice find in progress popup 
 * has occurred. XtAddEventHandler is the means through which MoreFindingCB 
 * is invoked.
 * MoreFindingCB basically registers the get_find routine with 
 * XtAddTimeOut.  Given that a find can be extremely time consuming,
 * This allows for other events to be processed if no information is
 * available from the fd.
 */ 
static void
fn_MoreFindingCB(Widget popup, XtPointer client_data,
		 XEvent * xevent, Boolean * continue_to_dispatch)
{

	register XMapEvent *event = (XMapEvent *)xevent;
	Arg  args[6];
	FindInfoStruct * info = (FindInfoStruct *)client_data;
	Widget	stopbutton;

        if(event->type == MapNotify) {
		info->timeout_id = XtAddTimeOut(500, fn_getfind, info);
#ifdef MOTIF_FINDER
		DmBusyWindow(info->window->shell, True,
                     	fn_StopKeyCB, (XtPointer)info);
		stopbutton = (Widget) (((MenuGizmo *)(info->stopnotice->menu))->child);
                OlAddCallback(stopbutton, XtNconsumeEvent, fn_StopKeyCB, info);
#endif /* MOTIF_FINDER */
                XtRemoveEventHandler(GetModalGizmoShell(info->stopnotice),
                        StructureNotifyMask, FALSE, fn_MoreFindingCB, info);
	}

}

/*
 * The FindingCB routine is invoked after an event is received
 * indicating that a mapped event of the found window has occurred.
 * XtAddEventHandler is the means through which FindingCB is invoked.
 * An Event Handler is added here to account for the Notice Window 
 * indicating that the find is in progress.
 */ 
static void
fn_FindingCB(Widget popup, XtPointer client_data,
	     XEvent * xevent, Boolean * continue_to_dispatch)
{

	register XMapEvent *event = (XMapEvent *)xevent;
	Arg  args[6];
	FindInfoStruct * info = (FindInfoStruct *)client_data;

        if(event->type == MapNotify) {
		StopItems[0].clientData = (char *) info;
		info->stopnotice = 
		CreateGizmo((Widget)info->window->views[0].box, ModalGizmoClass,
                            &StopNotice, args, 0);
		MapGizmo(ModalGizmoClass, info->stopnotice);

                XtAddEventHandler(GetModalGizmoShell(info->stopnotice),
                        StructureNotifyMask, FALSE, fn_MoreFindingCB, info);

                XtRemoveEventHandler(info->window->shell,
                        StructureNotifyMask, FALSE, fn_FindingCB, info);
	}

}


/* The FindCB is invoked whenever the "Find" button in the find
 * Base Window is selected.  The routines calls CreateFindingWindow;
 * and also constructs a command string eg. "find /home -name "p" -print
 * which is used as an argument to the popen.
 */

/* ARGSUSED */
static void
fn_FindCB(widget,client_data,call_data)
Widget widget;
XtPointer client_data, call_data;
{
	extern FILE *fn_popen();
	FindInfoPtr info;
	DmFolderWinPtr folder = (DmFolderWinPtr)client_data;
	char findcmd[4096];
	char * filename_string;
	char * filecontent_string;
	char * trns_string;
	char * file_string;
	char * othername_string;
	char * other_string;
	char * mntpnt;
	Arg  args[6];
	Boolean typeset[DM_FTYPE_SEM -1];
	Boolean set, allset, alreadyset, special_type;
	struct  stat    st_buf;
	int i, flags, num_path, num_files, num_types;

	Gizmo 		find_popup = folder->finderWindow;
	Widget 		find_shell = GetPopupGizmoShell(find_popup);
	CListGizmo	* fn_clist = (CListGizmo *)QueryGizmo(PopupGizmoClass,
								find_popup,
								GetGizmoGizmo,
								"fnclist");
	int		updated_value;
	Gizmo 		updated_gizmo = (Gizmo) QueryGizmo(PopupGizmoClass,
								find_popup,
								GetGizmoGizmo,
								"daysupdated");
	Gizmo		remote_search_gizmo;
	Gizmo		ign_name_case_gizmo;
	Gizmo		ign_content_case_gizmo;
	char * 		ign_name_case_value;
	char * 		ign_content_case_value;
	char * 		remote_search_value;
	Widget		fnlist_widget;

	XmString	**selected_xmstrings;
	char *		selected_string;
	char *		diskA_alias = DtamGetAlias("diskette1", FIRST);
	char *		diskB_alias = DtamGetAlias("diskette2", FIRST);
	int		selected_count;
	pathrecptr local, scratchlocal, tmplocal;
	struct passwd *user_entry;

#define FIRST 1

#define FN_STRCAT(A,B)	{						\
			if ((int)(strlen(A) + strlen(B)) > 4096) {     	\
			 	DmVaDisplayStatus((DmWinPtr)(info->window), 1, GetGizmoText(TXT_FNEXCEEDED_ERR)); \
				return;					\
			}						\
			else {						\
				strcat(A,B);				\
			}						\
			}

	fn_InitializeInfoStruct(&info);
        info->FolderWin = (DmFolderWinPtr)folder;
	fnlist_widget = (Widget)QueryGizmo(PopupGizmoClass, 
				find_popup, GetGizmoWidget, "fnlist");
	XtSetArg(args[0], XmNselectedItems, &selected_xmstrings);
	XtSetArg(args[1], XmNselectedItemCount, &selected_count);
	XtGetValues(fnlist_widget, args, 2);

   	othername_string = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass, 
				find_popup, GetGizmoGizmo, "otherpath"));
	num_path = 0;

	strcpy(findcmd, "find   ");
	local = NULL;

	for(i = 0; i < selected_count; i++) {
		selected_string = GET_TEXT((XmString)*selected_xmstrings);
		selected_xmstrings++;
		if (!strcmp(Dm__gettxt(TXT_MY_FOLDERS), selected_string))
		{
			fn_AddPathToList(&local, DESKTOP_HOME(Desktop));
			num_path++;
		}
		if (!strcmp(Dm__gettxt(TXT_USER_FOLDERS), selected_string))
		{
        		while(!((user_entry= getpwent()) == NULL)) {
                    		if ((user_entry->pw_uid < 60000) &&
                                       (user_entry->pw_uid > 100)) {
					 fn_AddPathToList(&local, user_entry->pw_dir );
					 num_path++;
				}
			}
			setpwent();
		}
		if ((!strcmp(diskA_alias, selected_string)) || (!strcmp(diskB_alias, selected_string)))
		{
			flags = DtamMountDev(selected_string, &mntpnt);
			if(flags == DTAM_NO_DISK) {
				DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True,
					TXT_NODISK_ERR);
					return;
			}
			else
			if (flags == DTAM_CANT_MOUNT) {
				DmVaDisplayStatus( (DmWinPtr)(info->FolderWin), True,
					TXT_CANTMOUNT_ERR);
				return;
			}
			else
			if (flags == DTAM_NOT_OWNER) {
				DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True,
					TXT_NOTOWNER_ERR);
				return;
			}
			else
			if (flags == 0) {
				 if (!strcmp(diskA_alias, selected_string)) {
					if (info->disk1)
						FREE(info->disk1);
					info->disk1 = strdup(mntpnt);
				  }
				  if (!strcmp(diskB_alias, selected_string)) {
					if (info->disk2)
						FREE(info->disk2);
					info->disk2 = strdup(mntpnt);
				  }
			}
			else{
                               	if (!(flags == DTAM_MOUNTED)) {
					DmVaDisplayStatus( (DmWinPtr)(info->FolderWin), True,
					TXT_CANTMOUNT_ERR);
					return;
				}
			}
			fn_AddPathToList(&local, mntpnt);
			num_path++;
		}
		if (!strcmp(Dm__gettxt(TXT_WHOLE_SYSTEM), selected_string))
		{
			fn_AddPathToList(&local, "/");
			num_path++;
		}
		if(selected_string[0] == '/')
		{
			fn_AddPathToList(&local, selected_string);
			num_path++;
		}
	}
	for(other_string=strtok(othername_string, " \t\n"); other_string; 
					other_string=strtok(NULL, " \t\n")) {
		
                (void)stat(other_string, &st_buf);
		if(!((st_buf.st_mode & S_IFMT) == S_IFDIR)){
			DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True,
						TXT_INVOTHER_ERR);
			return;
		}
		fn_AddPathToList(&local, other_string);
		num_path++;
	}
	if(num_path == 0) {

		DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True, TXT_NOPATH_ERR);
		return;
	}
	scratchlocal = local;
	while(scratchlocal != NULL) {
		FN_STRCAT(findcmd, scratchlocal->path);
		FN_STRCAT(findcmd, " ");
		scratchlocal = scratchlocal->next;
	}
	scratchlocal = local;
	while(scratchlocal != NULL) {
		tmplocal = scratchlocal->next;
		FREE(scratchlocal->path);
		FREE((void *)scratchlocal); 
		scratchlocal = tmplocal;
	}
	
	/*
	 * Check if the "Search Local Folders Only is set
	 */
        remote_search_gizmo = QueryGizmo(PopupGizmoClass, find_popup, 
						GetGizmoGizmo, "FRemote");
	ManipulateGizmo(ChoiceGizmoClass, remote_search_gizmo, 
						GetGizmoValue);
	remote_search_value = (char *) QueryGizmo(ChoiceGizmoClass,
				remote_search_gizmo,GetGizmoCurrentValue, NULL);
	if (remote_search_value[0] == 'x') {
		FN_STRCAT(findcmd, " ! -local -prune -o ");
	}

   	filename_string = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass, 
				(Gizmo)find_popup, GetGizmoGizmo, "filename"));
	num_files = 0;
	if (!(filename_string[0] == '\0')){
	 FN_STRCAT(findcmd, " \\( ");
	 for(file_string=strtok(filename_string, " \t\n"); file_string; 
					file_string=strtok(NULL, " \t\n")) {
		
		num_files++;
		if (num_files > 1)
			FN_STRCAT(findcmd, " -o -name \"")
		else
			FN_STRCAT(findcmd, " -name \"")
        	ign_name_case_gizmo = QueryGizmo(PopupGizmoClass, 
				find_popup, GetGizmoGizmo, "FCase");
		ManipulateGizmo(ChoiceGizmoClass, ign_name_case_gizmo, 
								GetGizmoValue);
		ign_name_case_value = (char *) QueryGizmo(ChoiceGizmoClass,
						ign_name_case_gizmo, 
						GetGizmoCurrentValue, NULL);

		if (ign_name_case_value[0] == 'x') {
			trns_string = fn_transform(file_string, strlen(file_string));
			FN_STRCAT(findcmd, trns_string)
			FN_STRCAT(findcmd, "\"")
			FREE(trns_string);
		}
		else{
			FN_STRCAT(findcmd, file_string)
			FN_STRCAT(findcmd, "\"")
		}
	}
	 FN_STRCAT(findcmd, " \\) ")
	}

	updated_value =  GetNumericGizmoValue(updated_gizmo);
	if (updated_value > 0){
		char updated_string[5];

		sprintf(updated_string, "%d", updated_value);
		FN_STRCAT(findcmd, " -mtime ");
		FN_STRCAT(findcmd, "-");
		FN_STRCAT(findcmd, updated_string);
		FN_STRCAT(findcmd, " ");
	}

	/*
	 * Go through list looking for Set Icon, to populate typeset array
	 */
	allset = True;
	alreadyset = False;
	special_type = False;
	set = False;
	num_types = 0;

	for (i = 0; i < fn_clist->cp->num_objs; i++) {
		if (ITEM_SELECT(fn_clist->itp + i)){
			typeset[i] = True;
			num_types++;
		}
		else {
			typeset[i] = False;
			allset = False;
		}
	}
	
   	filecontent_string =GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass,
				find_popup, GetGizmoGizmo, "filecontent"));

	if (!allset) {

	   if (num_types > 1)
			FN_STRCAT(findcmd, " \\( ");

	
           /*
	    * Check for Directories
            */
	   if (typeset[DM_FTYPE_DIR -1]) {
			FN_STRCAT(findcmd, " \\( -type d \\) ");
			alreadyset = True;
	   }

           /*
	    * Check for regular files (executable or data files)
            */
	   if (typeset[DM_FTYPE_EXEC -1] && typeset[DM_FTYPE_DATA -1]) {
		if (alreadyset)
			FN_STRCAT(findcmd, " -o ");
		FN_STRCAT(findcmd, " \\( -type f \\)");
		if (!alreadyset)
			alreadyset = True;
	   }
	   else
		if (typeset[DM_FTYPE_EXEC -1]){
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, 
			" \\( \\( -perm -010 -o -perm -100 -o -perm 001 \\)  -type f \\) ");
			if (!alreadyset)
				alreadyset = True;
	   	}
		else
	   	if (typeset[DM_FTYPE_DATA -1]) {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ")
			FN_STRCAT(findcmd, 
			" \\( \\( ! \\( -perm -010 -o -perm -100 -o -perm 001 \\) \\)  -type f \\) ")
			if (!alreadyset)
				alreadyset = True;
		}

           /*
	    * Check for FIFO (named pipe) files
            */
	   if (typeset[DM_FTYPE_FIFO -1]) {
		if (!(filecontent_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type p \\)");
			if (!alreadyset)
				alreadyset = True;
		}
	   }

           /*
	    * Check for Character Special files
            */
	   if (typeset[DM_FTYPE_CHR -1]) {
		if (!(filecontent_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type c \\) ");
			if (!alreadyset)
				alreadyset = True;
		}
	   }

           /*
	    * Check for Block Special files
            */
	   if (typeset[DM_FTYPE_BLK -1]) {
		if (!(filecontent_string[0] == '\0')){
			special_type = True;
		}
		else {
			if (alreadyset)
				FN_STRCAT(findcmd, " -o ");
			FN_STRCAT(findcmd, " \\( -type b \\) ");
		}
	   }

	   if (num_types > 1)
			FN_STRCAT(findcmd, " \\) ");

	   if (!(filecontent_string[0] == '\0')){
		if (special_type) {
        		DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True,
        				TXT_SPECIALTYPE_ERR);
        		return;
		}
           }
	}
	else {
	   if (!(filecontent_string[0] == '\0'))
			FN_STRCAT(findcmd, " \\( -type f \\) ");
	}

	FN_STRCAT(findcmd, " -follow ");

	if (!(filecontent_string[0] == '\0')) {
		FN_STRCAT(findcmd, " -print 2> /dev/null |xargs grep -l ");
        	ign_content_case_gizmo = QueryGizmo(PopupGizmoClass, 
				find_popup, GetGizmoGizmo, "CCase");
		ManipulateGizmo(ChoiceGizmoClass, ign_content_case_gizmo, 
								GetGizmoValue);
		ign_content_case_value = (char *) QueryGizmo(ChoiceGizmoClass,
						ign_content_case_gizmo, 
						GetGizmoCurrentValue, NULL);
		if (ign_content_case_value[0] == 'x') {
			FN_STRCAT(findcmd, "-i ");
		}
		FN_STRCAT(findcmd, "\""); 
		FN_STRCAT(findcmd, filecontent_string); 
		FN_STRCAT(findcmd, "\" ");
	}
	else {
		FN_STRCAT(findcmd, " -print");
	}
	FN_STRCAT(findcmd, " 2> /dev/null");

   	BringDownPopup(find_shell);

	CreateFoundWindow(info);

	info->fptr = NULL;
	info->fptr = fn_popen(findcmd, "r");
	if (info->fptr == NULL) {
		DmVaDisplayStatus((DmWinPtr)(info->FolderWin), True, TXT_FIND_FAILED);
		perror("dtmgr find");
		return;
	}
	fcntl(fileno(info->fptr), F_SETFL, O_NONBLOCK);

   	XtAddEventHandler(info->window->shell, StructureNotifyMask, FALSE, fn_FindingCB, info);

}

static void
fn_AddPathToList(pathptr, dir)
pathrecptr * pathptr;
char * dir;
{

	pathrecptr head;
	pathrecptr current;
	pathrecptr local;
	pathrecptr tail;
	pathrecptr previous;
	int dirlen, pathlen;
	Boolean AddToList = True;
	Boolean AlreadyAdded = False;

	if (!(*pathptr)) {
		*pathptr = (pathrecptr) MALLOC (sizeof(PathRec));
		((pathrecptr)(*pathptr))->next= NULL;
		((pathrecptr)(*pathptr))->path= strdup(dir);
		return;
	}

	head = current = previous = local = *pathptr;

	while (!(local == NULL)) {
		tail = local;
		local = local->next;
	}
	
	dirlen = strlen(dir);

	while (!(current == NULL)) {
		pathlen = strlen(current->path);

		if (pathlen > dirlen) {
			/*
			 * BELOW: If the path up to dirlen is equal
			 * then remove current path since it is taken care 
			 * of by dir.
			 */
			if (!strncmp(current->path, dir, dirlen)) {

				/*
				 * remove from list
 				 */
				if (!(previous == current)) {
			    		previous->next = current->next;
					FREE(current->path);
					FREE((void *)current);
					current = previous->next;
				}
				else {
				    	if (current == *pathptr) {
						*pathptr = current->next;
					}
					
					previous = current->next;
					FREE(current->path);
					FREE((void *)current);
					current = previous;
				}

			}
			else {
				/* Add to end of list */
				previous = current;
				current = current->next;
			}
		}
		else
		if (dirlen > pathlen) {
			/*
			 * BELOW: If the dir up to pathlen is equal
			 * then do nothing since it is taken care 
			 * of by path.
			 */
			if (!strncmp(current->path, dir, pathlen)) {
				/* do not add to list becase already taken 
				 * care of
				 */
				AddToList = False;
				previous = current;
				current = current->next;
			}
			else {
				previous = current;
				current = current->next;
			}
		}
		else
		if (dirlen == pathlen) {
			if (!strncmp(current->path, dir, pathlen)) {
				AddToList = False;
				previous = current;
				current = current->next;
			}
			else {
				previous = current;
				current = current->next;
			}
		}
			
	}
	if (AddToList) {
		if (!(previous == NULL)) {
			local = (pathrecptr) MALLOC (sizeof(PathRec));
			local->next= NULL;
			local->path= strdup(dir);
			previous->next = local;
		}
		else {
			*pathptr = (pathrecptr) MALLOC (sizeof(PathRec));
			((pathrecptr)(*pathptr))->next= NULL;
			((pathrecptr)(*pathptr))->path= strdup(dir);
			return;
		}
	}
}

static char *
fn_transform(file_string, len)
char * file_string;
int len;
{
	char * tmp_string;
	char * new_string;
	int incnt, outcnt, tmp_char;

	/*
	 * newstring represents the max size eg. i -> [iI] 
         */
	new_string = (char *) MALLOC (((sizeof(char))*len *4) + (sizeof(char)));

	outcnt = 0;

	for (incnt = 0; incnt < len; incnt++)
	{
		if(islower(file_string[incnt])) {
			new_string[outcnt] = '['; outcnt++;
			new_string[outcnt] = file_string[incnt]; outcnt++;
			new_string[outcnt] = toupper(file_string[incnt]); outcnt++;
			new_string[outcnt] = ']'; outcnt++;
		}
		else 
			if(isupper(file_string[incnt])) {
				new_string[outcnt] = '['; outcnt++;
				new_string[outcnt] = file_string[incnt]; outcnt++;
				new_string[outcnt] = tolower(file_string[incnt]); outcnt++;
				new_string[outcnt] = ']'; outcnt++;
			}
			else {

				new_string[outcnt] = file_string[incnt]; outcnt++;
			}
	}
	incnt++;
	new_string[outcnt]='\0';
	return new_string;
}


static void
FoundWinWMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	FindInfoStruct * info = (FindInfoStruct *)client_data;

	fn_CloseFoundCB(NULL, info, NULL);

}


/****************************procedure*header*****************************
    DmSetFWMenuItems
*/
void
DmSetFWMenuItem(MenuItems ** ret_menu_items, char * name, int index,
						DmFolderWinPtr folder)
{
	switch(index)
	{

	case 0:

		*ret_menu_items = FileMenuItems;
		strcpy(name, "filemenu");
		break;

	case 1:

		*ret_menu_items = EditMenuItems;
		strcpy(name, "editmenu");
		break;

	case 2:

		*ret_menu_items = HelpMenuItems;
		strcpy(name, "helpmenu");
		break;

	default:
		break;
	}
}


/****************************procedure*header*********************************
 *  DmDestroyFindWindow: destroy the Find Window
 *      INPUTS: folder
 *      OUTPUTS: none
 *      GLOBALS:
 *****************************************************************************/
void
DmDestroyFindWindow(DmFolderWindow folder)
{
	Widget          fnlist;

	if (folder->finderWindow != NULL) {
		fnlist = (Widget ) QueryGizmo(PopupGizmoClass,
                       		folder->finderWindow, GetGizmoWidget, "fnlist");
		XmListDeleteAllItems(fnlist);
		FreeGizmo(PopupGizmoClass, folder->finderWindow);
		folder->finderWindow = NULL;
	}
}       /* end of DmDestroyFindWindow */
