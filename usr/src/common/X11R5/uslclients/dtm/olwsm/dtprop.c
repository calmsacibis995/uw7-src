#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/dtprop.c	1.48"
#endif

/******************************file*header********************************

    Description:
	This file contains the source code for desktop property sheet.
*/
						/* #includes go here	*/
#include <signal.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/NumericGiz.h>
#include <DtWidget/SpinBox.h>

#include "error.h"
#include <exclusive.h>
#include <dtprop.h>
#include <list.h>
#include <misc.h>
#include <node.h>
#include <property.h>
#include <resource.h>
#include <wsm.h>
#include <xtarg.h>
#include "changebar.h"

#include <Dtm.h>		/* for Desktop */

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void		Import (XtPointer closure);
static void		Export (XtPointer closure);
static void		Create (Widget work, XtPointer closure);
static ApplyReturn *	ApplyCB (Widget w, XtPointer closure);
static void		ResetCB (Widget w, XtPointer closure);
static void		FactoryCB (Widget w, XtPointer closure);
static void		PingChangeBarCB (Widget, XtPointer, XtPointer);
static void		Report(Exclusive *);
static void		TouchChangeBarDB ();


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define OLFACTORY(x)		*(ADDR *)&(ol_factory[x].value)
#define SETVAR			GetXWINHome("adm/olsetvar")
#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#ifdef GLOBAL
#undef GLOBAL
#endif
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

#define SETVAR			GetXWINHome("adm/olsetvar")
#define DT_AT_LOGIN		"DT"

static Arg desktop_args[] = {
/* Note: if you change the location of this element you must also */
/* change the index value in CreatePropertyPopup(). */
        {XmNorientation,	XmVERTICAL},
	{XmNnumColumns,		1},
};

static HelpInfo DesktopHelp = {
	NULL, NULL, "DesktopMgr/dskpref.hlp", NULL
};

Property desktopProperty = {
	"Desktop",
	desktop_args,
	XtNumber (desktop_args),
	&DesktopHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	0,
	0,
	0,
	0,
	0,
	NULL,
	NULL,
	False,
	NULL,
	420,
	265,
};

/* These define the offsets for the various control on this property sheet.
   That is, the controls appear in this order.
*/
static enum { 
	GRID_WIDTH, GRID_HEIGHT, FOLDER_ROWS, FOLDER_COLS,
	SHOW_PATHS, OPEN_FOLDERS, LAUNCH, SYNC_FOLDERS
};

/* Define strings used as resource values - eliminate these */

static char _yes[]	= "Yes";
static char _no[]	= "No";

static char _true[]	= "True";
static char _false[]	= "False";
static char _home[]	= "False";
static char _cwd[]	= "True";

static char		_same	 [] = "true";
static char		_new	 [] = "false";

/*
   Note that _factory entries with NULL values are those with integer
   defaults and are initialized during "Import".
*/
static Resource _factory[] = {
	{ "*" XtNgridWidth,		NULL },		/* GRID_WIDTH */
	{ "*" XtNgridHeight,		NULL },		/* GRID_HEIGHT */
	{ "*" XtNfolderRows,		NULL },		/* FOLDER_ROWS */
	{ "*" XtNfolderCols,		NULL },		/* FOLDER_COLS */
	{ "*" XtNshowFullPaths,		_false },	/* SHOW_PATHS */
	{ "*" XtNopenInPlace,		_same },	/* OPEN_FOLDERS */
	{ "*" XtNlaunchFromCWD,		_home },	/* LAUNCH */
	{ "*" XtNsyncRemoteFolders,	_false },	/* SYNC_FOLDERS */
};
static List factory = LIST(Resource, _factory);

static Resource _current[] = {
	{ "*" XtNgridWidth,		NULL },
	{ "*" XtNgridHeight,		NULL },
	{ "*" XtNfolderRows,		NULL },
	{ "*" XtNfolderCols,		NULL },
	{ "*" XtNshowFullPaths,		NULL },
	{ "*" XtNopenInPlace,		NULL },
	{ "*" XtNlaunchFromCWD,		NULL },
	{ "*" XtNsyncRemoteFolders,	NULL },
};
static List current = LIST(Resource, _current);

/* Since the _current resource values are ASCII and the numeric fields are
   integer, their converted values must be stored somewhere.  Create buffers
   for the resources which are inputted via numeric fields.
*/
static char current_gwidth_buf[5];
static char current_gheight_buf[5];
static char current_frow_buf[5];
static char current_fcol_buf[5];

static char factory_gwidth_buf[5];
static char factory_gheight_buf[5];
static char factory_frow_buf[5];
static char factory_fcol_buf[5];

/* Need to change the names under ExclusiveItems to come from resource file */

/* Define the exclusive setting for starting desktop at login */
static ExclusiveItem	_login[] = {
	{ (XtArgVal)"Yes", (XtArgVal)_yes, False, False },
	{ (XtArgVal)"No",  (XtArgVal)_no, False, False  },
};
static List		login = LIST(ExclusiveItem, _login);
static Exclusive	Login =
    EXCLUSIVE("startAtLogin", "Start Desktop at login:", &login, TouchChangeBarDB);

static String		factory_login = _yes;
static String		current_login;

/* Define the exclusive setting for showing full path names */
static ExclusiveItem _path[] = {
    { (XtArgVal)"Yes",	(XtArgVal)_true,	False, False },
    { (XtArgVal)"No",	(XtArgVal)_false,	False, False },
};
static List		path = LIST(ExclusiveItem, _path);
static Exclusive	Path =
    EXCLUSIVE("show_path", "Show Full Path Names:", &path, TouchChangeBarDB);

/* Define the exclusive setting for open folders area */
static ExclusiveItem _folders[] = {
    { (XtArgVal)"",	(XtArgVal)_same,	False, False },
    { (XtArgVal)"",	(XtArgVal)_new,		False, False },
};
static List		folders = LIST(ExclusiveItem, _folders);
static Exclusive	Folders =
    EXCLUSIVE("open_folders", "Open Folders in:", &folders, TouchChangeBarDB);

/* Define the exclusive setting for Launch Applications area */
static ExclusiveItem _launch[] = {
    { (XtArgVal)"Home",	(XtArgVal)_home,	False, False },
    { (XtArgVal)"CWD",	(XtArgVal)_cwd,		False, False },
};
static List		launch = LIST(ExclusiveItem, _launch);
static Exclusive	Launch =
    EXCLUSIVE("launch_apps", "Launch Apps:", &launch, TouchChangeBarDB);

/* Define the exclusive setting for sync remote folders area */
static ExclusiveItem _sync[] = {
    { (XtArgVal)"",	(XtArgVal)_true,	False, False },
    { (XtArgVal)"",	(XtArgVal)_false,	False, False },
};
static List		sync = LIST(ExclusiveItem, _sync);
static Exclusive	Sync =
    EXCLUSIVE("sync_folders", "Sync", &sync, TouchChangeBarDB);

/* Define [non]exclusive gizmos */

#define NUMERIC_GIZ(name, min, max) {	\
    NULL,			/* help info */			\
    name,			/* name */			\
    min,			/* value */			\
    min,			/* minimum value */		\
    max,			/* maximum value */		\
    1,				/* increment */			\
    0				/* number of digits */		\
}

/* Define numeric field gizmos */
static NumericGizmo
    grid_width_giz  = NUMERIC_GIZ("gridWidth",  32, 499),
    grid_height_giz = NUMERIC_GIZ("gridHeight", 32, 499),
    folder_rows_giz = NUMERIC_GIZ("folderRows",  1,  99),
    folder_cols_giz = NUMERIC_GIZ("folderCols",  1,  99);

#undef NUMERIC_GIZ

/* Put gizmos together */

typedef struct _NumericArray {
	NumericGizmo *	ng;
	Gizmo		g;
} NumericArray;

static NumericArray gizmos[] = {  
    { &grid_width_giz,	NULL },
    { &grid_height_giz,	NULL },
    { &folder_rows_giz,	NULL },
    { &folder_cols_giz,	NULL }
};


static Widget GridSize;
static Widget FolderSize;
static ChangeBar*	ChangeBarDB;

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    Import- 

	Numeric field controls pose a problem: the value for a numeric field
	(in the gizmo) is an int whereas the resource value must be ASCII.
	What's done here is the "current" buffers for the numeric field
	resources is used here temporarily to store the ASCII representation
	of the default value.  Note this is only done so that default
	resource values can be merged with the gloabl resources.

	See also Apply and Factory.  When "Apply" is called, these buffers
	are re-used to store the current values so they no longer contain
	"factory" values.  When "Factory" is called, the numeric fields are
	re-loaded with the integer values found in the DEFAULT #define's.
*/
static void
Import (XtPointer closure)
{
#define _SET_FACTORY(buf, default, index) \
    sprintf(buf, "%d", default); FACTORY(index) = buf;

    _SET_FACTORY ( factory_gwidth_buf,	DEFAULT_GRID_WIDTH,	GRID_WIDTH );
    _SET_FACTORY ( factory_gheight_buf,	DEFAULT_GRID_HEIGHT,	GRID_HEIGHT );
    _SET_FACTORY ( factory_frow_buf,	DEFAULT_FOLDER_ROWS,	FOLDER_ROWS );
    _SET_FACTORY ( factory_fcol_buf,	DEFAULT_FOLDER_COLS,	FOLDER_COLS );

    merge_resources(&global_resources, &factory);
    return;

#undef	_SET_FACTORY

}				/* Import */

/****************************procedure*header*****************************
    Export-
*/
static void
Export (XtPointer closure)
{
    ExclusiveItem *	p;
    char *		value;

    p = ResourceItem(&Login, getenv(DT_AT_LOGIN));
    current_login = p ? (ADDR)p->addr : factory_login;

#define _EXPORT(exclusive, index) \
    p = ResourceItem(exclusive, GLOBAL(index)); \
	CURRENT(index) = (p ? (ADDR)p->addr : FACTORY(index));

    _EXPORT ( &Folders,		OPEN_FOLDERS );
    _EXPORT ( &Path,		SHOW_PATHS );
    _EXPORT ( &Launch,		LAUNCH );
    _EXPORT ( &Sync,		SYNC_FOLDERS );

#undef	_EXPORT
#define _EXPORT(index) CURRENT(index) = \
    ((value = GLOBAL(index)) == NULL) ? FACTORY(index) : value;

    _EXPORT ( GRID_WIDTH );
    _EXPORT ( GRID_HEIGHT );
    _EXPORT ( FOLDER_ROWS );
    _EXPORT ( FOLDER_COLS );

#undef	_EXPORT

    /* Enforce our notion of valid resource values ("re-Import"). */

    merge_resources(&global_resources, &current);

}				/* Export */

Widget
CreateNumerics(Widget parent, Widget *labels, int *cnt, Widget topWidget)
{
    XmString	string;
    int		i = 0;
    Arg		arg[20];
    Gizmo	g;
    Widget	leftWidget;
    Widget	w;
    char *	name;
    int		width;
    int		maxWidth = 0;

    /* Pick up the initial values for each numeric widget */
    gizmos[GRID_WIDTH].ng->value  = atoi(_current[GRID_WIDTH].value);
    gizmos[GRID_HEIGHT].ng->value = atoi(_current[GRID_HEIGHT].value);
    gizmos[FOLDER_ROWS].ng->value = atoi(_current[FOLDER_ROWS].value);
    gizmos[FOLDER_COLS].ng->value = atoi(_current[FOLDER_COLS].value);

    ChangeBarDB = (ChangeBar *) calloc(2, sizeof(ChangeBar));
    
    string = XmStringCreateLocalized((char *) pGGT(TXT_GRID_SIZE2));
    labels[(*cnt)++] = GridSize = XtVaCreateManagedWidget(
	"gridSize", 
	xmLabelWidgetClass, 
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_END,
	NULL
    );
    XmStringFree(string);
                                        
    i = 0;
    XtSetArg(arg[i], XmNtopAttachment, XmATTACH_WIDGET); i++;
    XtSetArg(arg[i], XmNleftAttachment, XmATTACH_WIDGET); i++;
    XtSetArg(arg[i], XmNleftWidget, GridSize); i++;
    XtSetArg(arg[i], XmNarrowLayout, XmARROWS_FLAT_END); i++;
    XtSetArg(arg[i], XmNtopWidget, topWidget); i++;
    gizmos[0].g = CreateGizmo(
	parent, NumericGizmoClass, &grid_width_giz, arg, i
    );
			  
    leftWidget = (Widget)QueryGizmo(
	NumericGizmoClass, gizmos[0].g, GetGizmoWidget, "gridWidth"
    );

    string = XmStringCreateLocalized((char *) pGGT(TXT_GRID_WIDTH));
    XtVaCreateManagedWidget(
	"gridWidthLabel", 
	xmLabelWidgetClass, 
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_WIDGET,
	XmNleftWidget,		leftWidget,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL
    );
    XmStringFree(string);

    topWidget = leftWidget;
    i -= 1;
    XtSetArg(arg[i], XmNtopWidget, topWidget); i++;
    gizmos[1].g = CreateGizmo(
	parent, NumericGizmoClass, &grid_height_giz, arg, i
    );
    
    leftWidget = (Widget)QueryGizmo(
	NumericGizmoClass, gizmos[1].g, GetGizmoWidget, "gridHeight"
    );
    string = XmStringCreateLocalized((char *) pGGT(TXT_GRID_HEIGHT));
    XtVaCreateManagedWidget(
	"gridHeightLabel", 
	xmLabelWidgetClass, 
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_WIDGET,
	XmNleftWidget,		leftWidget,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL
    );
    XmStringFree(string);
    topWidget = leftWidget;

    string = XmStringCreateLocalized((char *) pGGT(TXT_WINDOW_SIZE));
    labels[(*cnt)++] = FolderSize = XtVaCreateManagedWidget(
	"folderSize",
	xmLabelWidgetClass,
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_END,
	NULL
    );
    XmStringFree(string);
    
    i -= 1;
    XtSetArg(arg[i], XmNtopWidget, topWidget); i++;
    gizmos[2].g = CreateGizmo(
	parent, NumericGizmoClass, &folder_rows_giz, arg, i
    );

    leftWidget = (Widget)QueryGizmo(
	NumericGizmoClass, gizmos[2].g, GetGizmoWidget, "folderRows"
    );
    string = XmStringCreateLocalized((char *) pGGT(TXT_WINDOW_ROWS));
    XtVaCreateManagedWidget(
	"folderRowsLabel", 
	xmLabelWidgetClass, 
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_WIDGET,
	XmNleftWidget,		leftWidget,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL
    );
    XmStringFree(string);
    topWidget = leftWidget;

    i -= 1;
    XtSetArg(arg[i], XmNtopWidget, topWidget); i++;
    gizmos[3].g = CreateGizmo(
	parent, NumericGizmoClass, &folder_cols_giz, arg, i
    );
    
    leftWidget = (Widget)QueryGizmo(
	NumericGizmoClass, gizmos[3].g, GetGizmoWidget, "folderCols"
    );
    string = XmStringCreateLocalized((char *) pGGT(TXT_WINDOW_COLS));
    XtVaCreateManagedWidget(
	"folderColsLabel", 
	xmLabelWidgetClass, 
	parent,
	XmNlabelString, 	string,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		topWidget,
	XmNleftAttachment, 	XmATTACH_WIDGET,
	XmNleftWidget,		leftWidget,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL
    );
    XmStringFree(string);

    CreateChangeBar(GridSize, &ChangeBarDB[0]);
    CreateChangeBar(FolderSize, &ChangeBarDB[1]);

    /* Set up callback so that ChangeBar can be set when any value changes */
    for (i = 0; i < 4; i++) {
        g = gizmos[i].g;
	SetNumericInitialValue(g, atoi(FACTORY(i)));	/* Set init value */
	name = gizmos[i].ng->name;
        w = (Widget)QueryGizmo(NumericGizmoClass, g, GetGizmoWidget, name);
	XtVaGetValues(w, XmNwidth, &width, NULL);
	if (width > maxWidth) {
		maxWidth = width;
	}
    
        XtAddCallback(
		w, XmNvalueChangedCallback, PingChangeBarCB, (XtPointer) i
	);
    }
    for (i = 0; i < 4; i++) {
	name = gizmos[i].ng->name;
        g = gizmos[i].g;
        w = (Widget)QueryGizmo(NumericGizmoClass, g, GetGizmoWidget, name);
	XtVaSetValues(w, XmNwidth, maxWidth, NULL);
    }

    return leftWidget;
}

/****************************procedure*header*****************************
    Create-
*/
static void
Create (Widget parent, XtPointer closure)
{
    Widget	topWidget;
    Widget	labels[11];
    int		cnt = 0;

    _login[0].name	= (XtArgVal) pGGT(TXT_fixedString_yes);
    _login[1].name	= (XtArgVal) pGGT(TXT_fixedString_no);
    Login.current_item	= ResourceItem(&Login, current_login);
    Login.string	= pGGT(TXT_fixedString_login); 

    _path[0].name	= (XtArgVal) pGGT(TXT_fixedString_yes);
    _path[1].name	= (XtArgVal) pGGT(TXT_fixedString_no);
    Path.current_item	= ResourceItem(&Path, CURRENT(SHOW_PATHS));
    Path.string		= pGGT(TXT_fixedString_PATH);


    _launch[0].name	= (XtArgVal) pGGT(TXT_fixedString_HOME_FOLDER);
    _launch[1].name	= (XtArgVal) pGGT(TXT_fixedString_CURRENT_FOLDER1);
    Launch.current_item	= ResourceItem(&Launch, CURRENT(LAUNCH));
    Launch.string	= pGGT(TXT_fixedString_LAUNCH1);

    _sync[0].name	= (XtArgVal) pGGT(TXT_fixedString_yes);
    _sync[1].name	= (XtArgVal) pGGT(TXT_fixedString_no);
    Sync.current_item	= ResourceItem(&Sync, CURRENT(SYNC_FOLDERS));
    Sync.string		= pGGT(TXT_fixedString_SYNC);

    _folders[0].name	= (XtArgVal) pGGT(TXT_fixedString_SAME);
    _folders[1].name	= (XtArgVal) pGGT(TXT_fixedString_NEW);
    Folders.current_item= ResourceItem(&Folders,CURRENT(OPEN_FOLDERS));
    Folders.string	= pGGT(TXT_fixedString_FOLDERS);

    XtVaSetValues(
	parent,
	XmNfractionBase, 10,
	NULL
    );

    labels[cnt++] = CreateExclusive(parent, &Login, True);
    XtVaSetValues(
	XtParent(XtParent(Login.w[0])),
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM, 
	XmNrightAttachment, XmATTACH_FORM, 
	NULL
    );
    
    labels[cnt++] = CreateToggle(parent, &Path, &Login);
    
    /* Create the "Grid spacing" and "Window size" spin boxes */
    topWidget = CreateNumerics(
	parent, labels, &cnt, XtParent(XtParent(Path.w[0]))
    );

    labels[cnt++] = CreateToggle(parent, &Launch, NULL);
    XtVaSetValues(
	XtParent(XtParent(Launch.w[0])),
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, topWidget,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	NULL
    );
    labels[cnt++] = CreateToggle(parent, &Folders, &Launch);
    labels[cnt++] = CreateToggle(parent, &Sync, &Folders);

    AlignLabels(labels, cnt);
    
#undef SET_INIT
}				/* Create */

/****************************procedure*header*****************************
    PingChangeBarCB-
*/
static void
PingChangeBarCB (Widget w, XtPointer client_data, XtPointer call_data)
{
    int i = (int) client_data;
    
    i /= 2;
    SetChangeBarState(
	&ChangeBarDB[i], i, WSM_NORMAL, True, TouchChangeBarDB
    );
}

/****************************procedure*header*****************************
    Report-
*/
static void
Report(Exclusive * exc)
{
    if (exc->current_item)		/* ie, SELECT */
      {
	FooterMessage(exc->w[list_index(exc->items, exc->current_item)], 
	    pGGT(TXT_footerMsg_changeGUI), True);
      }
}

/****************************procedure*header*****************************
    SetVar-
*/
static int
SetVar(String var, String value)
{
    char	buf[80];
    int		status;
    void	(*f)( int );

    sprintf(buf, "%s %s %s", SETVAR, var, value);

    /* Normally we are ignoring/catching child-process death,
       but here we want system() to catch the clean-up.
    */
    f = signal(SIGCLD, SIG_DFL);
    status = system(buf);
    signal (SIGCLD, f);
    debug((stderr, "system(%s) returned %d\n", buf, status));

    return(status);
}

/****************************procedure*header*****************************
    ApplyCB()
*/
static ApplyReturn *
ApplyCB (Widget w, XtPointer closure)
{
    static ApplyReturn	ret = { APPLY_OK };
    extern void		DmApplyLaunchFromCWD(Boolean);
    extern void		DmApplyShowFullPath(Boolean);

    /* Note: The "DT at Login" capability requires the
       presence of two external shell scripts: oladduser, to set up an
       Desktop user's environment, and olsetvar, to make the appropriate
       changes to that environment for starting the system at login.  Both of
       these scripts need to be available if this feature is to function.
    */

    if (current_login != (ADDR)Login.current_item->addr)
    {
	int status = SetVar(DT_AT_LOGIN, (String)Login.current_item->addr);

	if (status && (String)Login.current_item->addr == _yes)
	{
	    ret.bad_sheet = &desktopProperty;  /* Was miscProperty */
	    ret.reason    = APPLY_ERROR;
	    ret.u.message =
		"Can't start Desktop at login--must run oladduser first";
	    return (&ret);
	}

	current_login = (ADDR)Login.current_item->addr;
    }
    SetChangeBarState (Login.ChangeBarDB, 0, WSM_NONE, True,  Login.change);
    
#define _APPLY(exclusive, index) \
	CURRENT(index) = (ADDR)(exclusive).current_item->addr; \
	SetChangeBarState( \
	    exclusive.ChangeBarDB, 0, WSM_NONE, True,  exclusive.change \
	)

    _APPLY ( Folders,	OPEN_FOLDERS );
    _APPLY ( Path,	SHOW_PATHS );
    _APPLY ( Launch,	LAUNCH );
    _APPLY ( Sync,	SYNC_FOLDERS );

    /* Apply "Launch from current working dir to Desktop field */
    DmApplyLaunchFromCWD(CURRENT(LAUNCH) == _cwd);

    /* Apply "Show path" to Desktop field */
    DmApplyShowFullPath(CURRENT(SHOW_PATHS) == _true);

    /* Now deal with Numeric Field Gizmos.  Normally, ManipulateGizmo with
       ApplyGizmoValue would be enough to save the current value in the
       resource database (using a converter).  Until that is implemented, we
       convert the current value to ASCII and store it into "current" which
       gets merged with the resource database.
    */
#undef	_APPLY
#define _APPLY(buf, i) { \
    ManipulateGizmo(NumericGizmoClass, gizmos[i].g, GetGizmoValue); \
    ManipulateGizmo(NumericGizmoClass, gizmos[i].g, ApplyGizmoValue); \
    sprintf(buf, "%d", GetNumericGizmoValue(gizmos[i].g)); \
    CURRENT(i) = buf; \
    SetChangeBarState( \
	&ChangeBarDB[i/2], i/2, WSM_NONE, True, TouchChangeBarDB \
    ); }

    _APPLY(current_gwidth_buf,	GRID_WIDTH);
    _APPLY(current_gheight_buf,	GRID_HEIGHT);
    _APPLY(current_frow_buf,	FOLDER_ROWS);
    _APPLY(current_fcol_buf,	FOLDER_COLS);

#undef	_APPLY

    merge_resources(&global_resources, &current);

    /* Now apply settings to Desktop fields */

#define CUR(i) GetNumericGizmoValue(gizmos[i].g)

    GRID_WIDTH(Desktop)		= (Dimension) CUR ( GRID_WIDTH );
    GRID_HEIGHT(Desktop)	= (Dimension) CUR ( GRID_HEIGHT );
    FOLDER_ROWS(Desktop)	= (u_char) CUR ( FOLDER_ROWS );
    FOLDER_COLS(Desktop)	= (u_char) CUR ( FOLDER_COLS );

#undef CUR
    OPEN_IN_PLACE(Desktop)	= (CURRENT(OPEN_FOLDERS) == _same);
    SYNC_REMOTE_FOLDERS(Desktop)= (CURRENT(SYNC_FOLDERS)==_true);

    return (&ret);
}				/* ApplyCB */

/****************************procedure*header*****************************
    ResetCB()
*/
static void
ResetCB (Widget w, XtPointer closure)
{
    SetExclusive(&Login, ResourceItem(&Login, current_login), WSM_NONE);

#define _RESET(exclusive, index) \
    SetExclusive(exclusive, ResourceItem(exclusive, CURRENT(index)), WSM_NONE) 

    _RESET ( &Path,	SHOW_PATHS );
    _RESET ( &Folders,	OPEN_FOLDERS );
    _RESET ( &Launch,	LAUNCH );
    _RESET ( &Sync,	SYNC_FOLDERS );

#undef	_RESET
#define _RESET(i)  ManipulateGizmo(NumericGizmoClass, gizmos[i].g, ResetGizmoValue); \
    SetChangeBarState(&ChangeBarDB[i/2], i/2, WSM_NONE, True, TouchChangeBarDB);

    _RESET ( GRID_WIDTH );
    _RESET ( GRID_HEIGHT );
    _RESET ( FOLDER_ROWS );
    _RESET ( FOLDER_COLS );

#undef	_RESET
}				/* ResetCB */

/****************************procedure*header*****************************
    FactoryCB-
*/
static void
FactoryCB (Widget w, XtPointer closure)
{
    int		value;

    NumericGizmo * numeric = NULL;
    SetExclusive(&Login, ResourceItem(&Login, factory_login), WSM_NORMAL);

#define _FACTORY(exclusive, index) \
	SetExclusive( \
	    exclusive, \
	    ResourceItem(exclusive, FACTORY(index)), WSM_NORMAL \
	)

    _FACTORY ( &Path,		SHOW_PATHS );
    _FACTORY ( &Folders,	OPEN_FOLDERS );
    _FACTORY ( &Launch,		LAUNCH );
    _FACTORY ( &Sync,		SYNC_FOLDERS );

#undef	_FACTORY
#define _FACTORY(i) \
      numeric = (NumericGizmo *)gizmos[i].g; \
      ManipulateGizmo(NumericGizmoClass, numeric, ReinitializeGizmoValue);  \
      SetChangeBarState(&ChangeBarDB[i/2], i/2, WSM_NORMAL, True, TouchChangeBarDB);
    
    _FACTORY ( GRID_WIDTH );
    _FACTORY ( GRID_HEIGHT );
    _FACTORY ( FOLDER_ROWS );
    _FACTORY ( FOLDER_COLS );

#undef	_FACTORY
}				/* FactoryCB */

/****************************procedure*header*****************************
    TouchChangeBarDB-
*/

static void TouchChangeBarDB()
{
	int i;
	int j = 0;

	for(i = 0; i < 2; i++)  /* ChangeBars for Gizmos */
	  {
	    if(ChangeBarDB[i].state)
	      {
	        ++j;
	      }
	  }
	
	/* Check exclusives */
	if( (CheckExclusiveChangeBar(&Path)) ||
	    (CheckExclusiveChangeBar(&Login)) ||
	    (CheckExclusiveChangeBar(&Launch)) ||
	    (CheckExclusiveChangeBar(&Sync)) ||
	    (CheckExclusiveChangeBar(&Folders)))
	  {
	  	++j;
	  }
	
	if(j > 0)
	  {
	    desktopProperty.changed = True;
	  }
	else
	  {
	    desktopProperty.changed = False;
	  }
	
	TouchPropertySheets();
	
	return;
}				/* TouchChangeBarDB */
