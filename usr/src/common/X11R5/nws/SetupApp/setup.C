#ident	"@(#)setup.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */
/*  setup.C
//
//  This file contains main(), and all the other high-level routines for the
//  generic setup application.
*/

#include	<errno.h>		//  for perror()
#include	<iostream.h>		//  for cout()
#include	<stdlib.h>		//  for exit()
#include	<sys/param.h>		//  for MAXPATHLEN
#include	<unistd.h>		//  for sleep()

/*  On production apps, do we allow editres commuication???		     */
#include	<X11/Xmu/Editres.h>	//  so editres can communicate w/ us
#include	<X11/cursorfont.h>	//  to get the watch and pointer cursors

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"cDebug.h"
#include	"controlArea.h"		//  for setVariableFocus()
#include	"dtFuncs.h"		//  for HelpText, GUI lib funcs, etc.
#include	"setup.h"		//  for the AppStruct definition
#include	"setupWin.h"		//  for the SetupWin definition
#include	"setupAPIs.h"		//  for setup API definitions
#include	"setup_txt.h"		//  the localized message database


/*  External functions, variables, etc.					     */

extern void	errorDialog(Widget topLevel,char *errorText,setupObject_t *curObj);
extern ActionAreaItem	actionItems[];


/*  Local functions, variables, etc.					     */

void	      main (int argc, char **argv);

static char  *getRealBasename (char *execPath);
static Widget createTopLevelShell (XtAppContext *appContext, Display **display);
static void   setupAppInit (Widget topLevel);
static void   getWebInitValues (SetupWin *win, Display *display);
static int    numVarLists (setupObject_t *object);
static void   setupWinInit (SetupWin *win);
static void   createWatchCursor (void);
static void   exitSetupWinCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *);

AppStruct	app = {0};
static char	*localizedErrorMsg = (char *)0;

XtResource resources[] =
{
 { /* Debug level for SetupApp (0=OFF, 1=ERR, 2=API, 3=FUNC, 4=PWD, 5=ALL)   */
   (String)"cDebugLevel",  "CDebugLevel",    XtRInt,           sizeof(int),
   XtOffset (RDataPtr, cDebugLevel),         XtRString,        "0"            },
 { // Debug file name for SetupApp client debugging stmts (used if cDebugLevel)
   (String)"cLogFile",     "CLogFile",       XtRString,        sizeof(String),
   XtOffset (RDataPtr, cLogFile),            XtRString,        (String)0      },
 { // Debug level for setup APIs (0=OFF, 1-10 are different levels (min to max))
   (String)"sDebugLevel",  "SDebugLevel",    XtRInt,           sizeof(int),
   XtOffset (RDataPtr, sDebugLevel),         XtRString,        "0"            },
};


XrmOptionDescRec cmdLineOptions[] =
{
  {   "-cDebugLevel",     "cDebugLevel",    XrmoptionSepArg,    (caddr_t)0   },
  {   "-cLogFile",        "cLogFile",       XrmoptionSepArg,    (caddr_t)0   },
  {   "-sDebugLevel",     "sDebugLevel",    XrmoptionSepArg,    (caddr_t)0   },
};






/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void main (int argc, char **argv)
//
//  DESCRIPTION:
//	Start the generic Setup application.  Let the setup API call us when
//	it's time to create one or more windows.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
main (int argc, char **argv)
{
	char	*execPath;



	XtSetLanguageProc (0, 0, 0);	/*  Must always be first.            */
	execPath = strdup (argv[0]);	/*  Save before Xt destroys it.	     */

	/*  The following 4 items are done individually, rather than all at
	//  once with XtAppInitialize, because we need much of this info
	//  before we initialize the setup library, and also because we
	//  create multiple application shells.				     */
	XtToolkitInitialize ();
	app.appContext = XtCreateApplicationContext ();
	XtAppSetFallbackResources (app.appContext, (String *)0);
	app.display = XtOpenDisplay (app.appContext, (String)0,
				(String)0, "setupApp",
				cmdLineOptions, XtNumber (cmdLineOptions),
				&argc, argv);
	if (!app.display)
	{
		fprintf (stderr, "Error: Can't open display!\n");
		exit (1);
	}

	/*  Initialize the setup library, then tell it our executable name,
	//  and the functions to call for creating and exiting setup windows. */
	app.realName = getRealBasename (execPath);
	free (execPath);
	setupObjectInit(app.rData.sDebugLevel, app.appContext);

	//  setupDefExecute() errors if no ASCII config file was found, if 
	//  there is no FIRST object declared in the ASCII config file, or
	//  if we pass no execute function (newSetupWin()).
	if (setupDefExecute (app.realName, newSetupWin, destroySetupWin))
	{
		//  Create generic no-name window with error dialog popup.
		newSetupWin ((setupObject_t *)0);
	}

	XtAppMainLoop (app.appContext);

}	//  End  main ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static char *getRealBasename (char *execPath)
//
//  DESCRIPTION:
//	Using the given execution path, which can be relative or absolute,
//	and resolve all symbolic links, and '.' and ".." in execPath.
//	Then find the basename (the portion after the last '/').
//	It is the responsibilty of the caller to free the returned basename.
//	Note:  This function *does* handle multibyte languages.
//	       Slash characters are always single byte.
//
//	Purpose for SetupApp:
//	What is being executed could be a localized_pgmName, but is always
//	symbolically linked to a non-localized /usr/X/bin/pgmName.  It is
//	this pgmName (from /usr/X/bin/pgmName) that we need, so that the
//	ASCII config file for this program, which must bear the same name,
//	can be read in and processed.  This is why XtGetApplicationName()
//	is not useful in this case.
//
//  RETURN:
//	The basename of the resolved execPath.
//
///////////////////////////////////////////////////////////////////////////// */


static char *
getRealBasename (char *execPath)
{
	char		resolved[(MAXPATHLEN+1)*sizeof(wchar_t)], *path;
	char		*slash;



	//  Resolve the command's path.  Don't go thru a hard link.

	if ((path = realpath (execPath, resolved)) == NULL)
	{
		perror ("realpath");
		return (NULL);
	}


	//  The basename of the command starts just past the last '/'.

	slash = strrchr (path, '/');
	return (strdup (slash+1));

}	//  End  getRealBasename ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void newSetupWin (setupObject_t *object)
//
//  DESCRIPTION:
//	Create a setup window if we don't already have one of this particular
//	kind up.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
newSetupWin (setupObject_t *object)
{
	static Boolean	firstTime = True;
	SetupWin	*win;
	ObjData		*objData;


	log4 (C_FUNC, "newSetupWin(obj=", object, ") objType=",
						setupObjectType (object));
	if (!object)
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_noModes;

	/*  If client data already exists for this object, it means that we
	//  already created a setup window for it.  We don't allow the same
	//  setup window to have multiple instances, so we'll open the existing
	//  one (if it's iconified), raise it to the top, and set focus to it.*/
	if (setupObjectClientData (object))
	{
		objData = (ObjData *)setupObjectClientData (object);
		XMapWindow (app.display, objData->win->window);
		XRaiseWindow (app.display, objData->win->window);
		//  Have to sync before setting input focus to something
		//  that may not be realized yet.
		XSync (app.display, False);
		XSetInputFocus (app.display, objData->win->window,
					RevertToParent, CurrentTime);
		return;
	}

	win = getNewSetupWin ();
	win->object = object;

	objData = (ObjData *)XtCalloc (1, (Cardinal)sizeof (ObjData));
	objData->win = win;
	setupObjectClientDataSet (object, objData);

	win->topLevel = createTopLevelShell (&app.appContext, &(app.display));
	if (firstTime)
	{
		setupAppInit (win->topLevel);
		firstTime = False;
	}

	if (setupObjectType (object) == sot_web)
	{
		win->objList = setupWebVariableList (object);
	}

	getWebInitValues (win, app.display);
	setupWinInit (win);

	createSetupWindow (win);		//  the real work is done here

	XtRealizeWidget (win->topLevel);	//  make the window visible

	/*  Get and set the height of the action area.  We want this area to
	//  remain the same height, but the control area can get larger, when
	//  the user resizes the window.				     */
	setActionAreaHeight (win->actionArea);

	XtSetMappedWhenManaged (win->topLevel, True);
	XtMapWidget (win->topLevel);

	/*  Get the window id now (we can't get it before realization).	     */
	win->window = XtWindow (win->topLevel);

	/*  Set the cursor for this app as the pointer (arrow) cursor	     */
	setCursor (C_POINTER, win->window, C_FLUSH);

	/*  Set the input focus to the first variable (which also ensures that
	//  the description text gets displayed.			     */
	if (win->numOpts >= 1)
	{
		setVariableFocus (win->firstObj);
	}

	if (!localizedErrorMsg)
	{
		setButtonSensitivity (actionItems, OK_BUTTON, True);
		setButtonSensitivity (actionItems, RESET_BUTTON, True);
	}
	else
	{
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
		errorDialog (win->topLevel, localizedErrorMsg, object);
		localizedErrorMsg = NULL;
	}

}	//  End  newSetupWin ()


/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createTopLevelShell (XtAppContext *appContext,
//							      Display **display)
//
//  DESCRIPTION:
//	Create a topLevel shell for a setup window. Setting XtNmappedWhenManaged
//	to False is needed so that we can later set the paned window minimum
//	and maximum (for the action area height).
//
//  RETURN:
//	The widget id of the the top level shell.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createTopLevelShell (XtAppContext *appContext, Display **display)
{
	Widget		topLevel;


	topLevel = XtVaAppCreateShell ("setup", (String)"SetupWin",
				topLevelShellWidgetClass,	*display,
				XtNmappedWhenManaged,		False,
				NULL);

	return (topLevel);

}	//  End  createTopLevelShell ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void setupAppInit (Widget topLevel)
//
//  DESCRIPTION:
//	Perform the setup application initialization functions - things that
//	only need to be performed once.  We get the application resources,
//	set the debug level information, and create the watch cursor.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static void
setupAppInit (Widget topLevel)
{

	XtGetApplicationResources (topLevel, &(app.rData), resources,
						XtNumber (resources), NULL, 0);
	/*  We've got the resources, now initialize debugging		     */
	if (!(cDebugInit (app.rData.cDebugLevel, (char *)app.rData.cLogFile, C_ALL)))
	{
		if (app.rData.cDebugLevel < 0 || app.rData.cDebugLevel > C_ALL)
			log2(C_ERR,"ERR: Invalid cDebugLevel ",app.rData.cDebugLevel);
		else
			log2(C_ERR,"ERR: Can't open cLogFile ", app.rData.cLogFile);

		app.rData.cDebugLevel = 0;
	}

	log6 (C_ALL, "\ncDebug=", app.rData.cDebugLevel, ", logFile=",
			    app.rData.cLogFile, ", sDebug=", app.rData.sDebugLevel);
	createWatchCursor ();

}	//  End  setupAppInit ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void getWebInitValues (SetupWin *win, Display *display)
//
//  DESCRIPTION:
//	Get all the high-level info. we need from the setup API to get started.
//	Retrieve the number of variable lists (if any), so we can determine if
//	we'll need a "Category" option menu, and also to see if we have anything
//	at all to display - if we don't, put up an error message.  Also get the
//	things needed for any window: its title, icon filename, icon label, and
//	help-related info.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static void
getWebInitValues (SetupWin *win, Display *display)
{
	setupObjectType_t	objType = setupObjectType (win->object);
	setupObject_t		*object;
	setupObject_t		*iconObj;


	log3 (C_FUNC, "getWebInitValues(setupWin=", win, ")");

	win->numOpts = numVarLists (win->object);

	/*  If we have a web, we've got to either have variables, or 1 or
	//  more lists of variables, or we really have nothing to display.   */
	if (objType == sot_web && win->numOpts < 1)
	{
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_noModes;
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
	}

	/*  Get the win title, the icon file and icon title for our win.
	//  If the object is a list, get this info from the definition object.
	//  Otherwise, get it from the object itself (win->object)	     */

	if (objType == sot_list)
		object = setupObjectDefinition (win->object);
	else
		object = win->object;

	win->title = setupObjectLabel (object);

	if (!(win->title))
		win->title = getStr (TXT_appNoName);

	if (iconObj = setupObjectIcon (object))
	{
		win->iconTitle = setupObjectLabel(iconObj);
		win->iconFile = setupIconFilename(iconObj);
	}

	if (!(win->iconTitle))
		win->iconTitle = getStr (TXT_iconNoName);

	if (!(win->iconFile))
		win->iconFile = getStr (TXT_defaultIconFile);

	/*  Get the Help file, section, and title from the web if it's there. */
//	win->help = (HelpText *)XtCalloc (1, (Cardinal)sizeof (HelpText));
//	win->help->file = setupDefHelpFile (setupObjectDefinition(win->object));
//	if (!win->help->file)
//	{
//		win->help->file = HELP_FILE;
//		win->help->title = TXT_appHelpTitle;
//		win->help->section = TXT_appHelpSection;
////	}
//	else
//	{
//		win->help->title = setupObjectHelpTitle (win->object);
//		if (!win->help->title)
//			win->help->title = TXT_appHelpTitle;
//
//		win->help->section = setupObjectHelpSection (win->object);
//		if (!win->help->section)
//			win->help->section = TXT_appHelpSection;
//	}

}	//  End  getWebInitValues ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static int  numVarLists (setupObject_t *object)
//
//  DESCRIPTION:
//	Calculate the number of variable lists (i.e number of "Categories").
//
//  RETURN:
//	The number of variable lists..
//
///////////////////////////////////////////////////////////////////////////// */

static int
numVarLists (setupObject_t *object)
{
	setupObject_t	*objList, *curObj;
	int		i = 0, numVarLists = 0;


	if (setupObjectType (object) == sot_web)
	{
	    if (objList = setupWebVariableList (object))
	    {
		switch (setupObjectType (setupObjectListNext (objList, NULL)))
		{
		    case	sot_objectList:	//  List has other list[s]

			for (curObj = setupObjectListNext (objList,
			              (setupObject_t *)0) ;
			     curObj ;
			     curObj = setupObjectListNext(objList,curObj))
			{
				i++;
			}

			numVarLists = i;
			break;

		    case	sot_var:	//  No lists, just variables
			numVarLists = 1;
			break;

		    default:
			break;
		}
	    }
	}
	return (numVarLists);

}	//  End  numVarLists ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void setupWinInit (SetupWin *win)
//
//  DESCRIPTION:
//	Perform window initialization: don't let this window be larger than the
//	user's resolution, set the title, exit callback, set the icon info., and
//	check user permissions.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static void
setupWinInit (SetupWin *win)
{

	log3 (C_FUNC, "setupWinInit(setupWin=", win, ")");

	/*  Don't allow this window to be larger than the user's screen
	//  resolution.  Also set the window title.			     */
	XtVaSetValues (win->topLevel,
	      XmNmaxHeight, DisplayHeight(app.display, DefaultScreen (app.display)),
	      XmNmaxWidth,  DisplayWidth (app.display, DefaultScreen (app.display)),
	      XmNtitle,     win->title,
	      XmNdeleteResponse, XmDESTROY,
	      NULL);

	XtAddCallback (win->topLevel, XmNdestroyCallback,
			    (XtCallbackProc)exitSetupWinCB, (XtPointer)win->object);

	/*  Allow EditRes to speak with this appplication.		     */
        XtAddEventHandler (win->topLevel, (EventMask)0, True,
				(XtEventHandler)_XEditResCheckMessages, NULL);

	if (!setupWebPerm (win->object))
	{
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_noPerms;
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
	}
	else
	{
		setButtonSensitivity (actionItems, OK_BUTTON, True);
		setButtonSensitivity (actionItems, RESET_BUTTON, True);
	}

}	//  End  setupWinInit ()


/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void createWatchCursor (void)
//
//  DESCRIPTION:
//	Create (and save for future use) the watch cursor.  We use the 
//	"default" cursor as the pointer cursor so as not to be "different".
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static void
createWatchCursor (void)
{
	app.cursorWait = XCreateFontCursor (app.display, XC_watch);

	if (app.cursorWait == BadAlloc || app.cursorWait == BadFont
						|| app.cursorWait == BadValue)
	{
		app.cursorWait = 0;
	}

}	//  End  createWatchCursor ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void setCursor (int cursorType, Window window, Boolean flush)
//
//  DESCRIPTION:
//	Set the cursor to the cursorType, and make the X library send it to the
//	Server NOW if flush is True.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
setCursor (int cursorType, Window window, Boolean flush)
{

	if (cursorType == C_WAIT)	//  cursor "wait"
	{
		XDefineCursor (app.display, window, app.cursorWait);
	}
	else				//  cursor "pointer"
	{
		/*  "None" as the cursor means use the parent's cursor, or,
		//  if the parent is the root window, use the default cursor. */
		XDefineCursor (app.display, window, None);
	}

	if (flush)
		XFlush (app.display);
}



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void exitSetupWin (setupObject_t *web)
//
//  DESCRIPTION:
//	This is called by the setup library code when it determines that a web
//	(setup window) should be destroyed.  This function address is given to 
//	setupDefExecute() as the 3rd arg.  We simply call exitSetupWinCB(),
//	which is where we do all our clean-up.
//
//  RETURN:
//	Nothing.
//	Note:  There is no return to this window, and if it is the last
//	remaining window, we will exit altogether.
//
///////////////////////////////////////////////////////////////////////////// */

void
exitSetupWin (setupObject_t *web)
{
	log3 (C_FUNC, "exitSetupWin(web=", web, ")");
	exitSetupWinCB ((Widget)0, (XtPointer)web, (XmAnyCallbackStruct *)0);

}	//  End  exitSetupWin ()



void
destroySetupWin (setupObject_t *web)
{
	ObjData		*objData;

	objData = (ObjData *)setupObjectClientData (web);
	log3 (C_FUNC, "destroySetupWin(web=", web,")");

	if (objData)
	{
		shellWidgetDestroy (objData->win->topLevel, web,
						(XmAnyCallbackStruct*)0);
	}
}	//  End  destroySetupWin ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void exitSetupWinCB (Widget w, XtPointer clientData,
//							XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This is the XmNdestroyCallback, and it gets called when the application
//	exits via an mwm destroy or when the app calls XtDestroyWidget().
//	Since this function performs all necessary application exiting functions
//	(frees all widget-associated resources, calls XDestroyWindow, etc.),
//	it SHOULD be called (via XtDestroyWidget (&app)) each time the app
//	wishes to exit.
//
//  RETURN:
//	Nothing.
//	Note:  There is no return from here.
//
///////////////////////////////////////////////////////////////////////////// */

static void
exitSetupWinCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{
	setupObject_t	*object = (setupObject_t *)clientData;
	setupObject_t	*objList, *curObj, *curVar;


	log1 (C_FUNC, "exitSetupWinCB()");

	switch (setupObjectType (object))
	{
	    case  sot_web:		//  Secondary w/ or w/o Category menu
		objList = setupWebVariableList (object);

		for (curObj = setupObjectListNext(objList,(setupObject_t *)0) ;
		     curObj ;
		     curObj = setupObjectListNext (objList, curObj))
		{
		    /*  Free the client data for each variable object we had. */
		    for (curVar=setupObjectListNext(curObj,(setupObject_t *)0) ;
			 curVar ;
			 curVar = setupObjectListNext (curObj, curVar))
		    {
			if (setupObjectClientData (curVar))
			{
			    XtFree ((char *)setupObjectClientData (curVar));
			    setupObjectClientDataSet (curVar, NULL);
			}
		    }

		    /*  Free the client data for each variable list object    */
		    if (setupObjectClientData (curObj))
		    {
			XtFree ((char *)setupObjectClientData (curObj));
			setupObjectClientDataSet (curObj, NULL);
		    }
		}
		break;

	    /*  The following have their objet client data free'd below	      */
	    case  sot_list:		//  Primary w/ browser
	    case  sot_objectList:
	    case  sot_var:		//  A variable? Not supported.
	    default:			//  Unrecognized type
		break;

	}	/*  End  switch (setupObjectType (object))		     */

	/*  Free the client data associated with this web (window).	     */
	if (setupObjectClientData (object))
	{
		XtFree ((char *)(setupObjectClientData (object)));
		setupObjectClientDataSet (object, NULL);
	}

	setupWebReset (object);

	/*  Return of non-zero is web (window) created by setup lib
	//  (the first one), so we can exit.				     */
	if (setupObjectExit (object))
	{
		log1 (C_ALL, "\tLast setupWin (first)! Exiting program...");
		setupObjectFree (object);
		exit (0);
	}

	if (!object)
		exit (0);

}	//  End  exitSetupWinCB ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void shellWidgetDestroy (Widget w, XtPointer clientData,
//						XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called when it is desired to destroy a shell widget and
//	all its descendants.  It finds the shell widget using the widget
//	received, then destroys it.  But, the callback associated with the
//	XmNdestroyCallback resource gets called automatically before the
//	widget is actually destroyed.  The destroy callback itself then
//	performs all the necessary tasks (such as freeing up memory, etc.).
//
//	This function should be called when an XtDestroyWidget() call has
//	NOT already been made, like in a button callback (such as for the
//	Cancel button, or in a menubar exit button callback function).
//	It should not be used when an XtDestroyWidget() call has been made
//	(like in a window manager exit such as when the user presses Alt-F4,
//	(or selects the window manager Close button or double-clicks on the
//	window manager button).
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs)
{
	log1 (C_FUNC, "shellWidgetDestroy()");

	while (w && !XtIsWMShell (w))
		w = XtParent (w);

	XtDestroyWidget (w);

	log1 (C_ALL, "\tJust destroyed shell widget");
	log1 (C_FUNC, "End  shellWidgetDestroy()");

}	//  End  shellWidgetDestroy ()
