#ident	"@(#)setupWin.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */

/*  setupWin.C
//
//  This file contains all the high-level routines for creating a setup window.
*/


#include	<iostream.h>		//  for cout()
#include	<unistd.h>		//  for sleep()

#include	<Xm/Xm.h>
#include	<Xm/Form.h>
#include	<Xm/MainW.h>
#include	<Xm/PanedW.h>
#include	<Xm/SashP.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"controlArea.h"		//  for controlArea functions
#include	"dtFuncs.h"		//  for HelpText, GUI lib funcs, etc.
#include	"setup.h"		//  for AppStruct
#include	"setupWin.h"		//  for SetupWin
#include	"setupAPIs.h"		//  for setupType_t definition
#include	"setup_txt.h"		//  the localized message database
#include	"action.h"		//  for menubar Action objects
#include	"menubar.h"		//  for the Menubar object
#include	"menubarMenu.h"		//  for the MenubarMenu object
#include	"menubarItem.h"		//  for the MenubarItem object
#include	"treeBrowse.h"		//  for type svt_list (browser)




/*  External variables, functions, etc.					     */

extern void   doOkActionCB    (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void   doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void   doCancelActionCB(Widget w,XtPointer clientData,XmAnyCallbackStruct *);
extern void   doHelpActionCB  (Widget, XtPointer clientData, XmAnyCallbackStruct *);

extern AppStruct	app;




/*  Local variables, functions, etc.					     */

static void	doAppHelpCB (Widget w, XtPointer clientData, XtPointer);
static void	doTOCHelpCB (Widget w, XtPointer clientData, XtPointer);
static void	doHelpDeskHelpCB (Widget w, XtPointer clientData, XtPointer);
static void	 exitCallback (Widget, XtPointer data, XtPointer);
static Boolean	 createMenubar (Widget parent, SetupWin *win);
static MenubarMenu *createFileMenu (Menubar *menuBar);
static MenubarMenu *createHelpMenu (Menubar *menuBar, SetupWin *win);

/* old
ActionAreaItem actionItems[] =
{ //   label         mnemonic          which    sensitive  callback       clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True, doOkActionCB,    (XtPointer)0 },
 {TXT_ResetButton, MNEM_ResetButton, RESET_BUTTON, True, doResetActionCB, (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True, doCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True, doHelpActionCB,  (XtPointer)0 }
};
*/
ActionAreaItem actionItems[] =
{ /*   label         mnemonic          which    sensitive  callback       clientData*/
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True, doOkActionCB,    (XtPointer)0 },
 {TXT_ResetButton, MNEM_ResetButton, RESET_BUTTON, True, doResetActionCB, (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True, doCancelActionCB,(XtPointer)0 }
};
int	numButtons = XtNumber (actionItems);





/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	SetupWin  *getNewSetupWin (void)
//
//  DESCRIPTION:
//	Allocate memory for a new setup window structure.
//
//  RETURN:
//	A pointer to the newly allocated SetupWin structure.
//
////////////////////////////////////////////////////////////////////////////// */

SetupWin *
getNewSetupWin (void)
{
	SetupWin	*setupWin;

	setupWin = (SetupWin *)XtCalloc (1, (Cardinal)sizeof (SetupWin));
	return (setupWin);

}	//  End  getNewSetupWin ()


/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  createSetupWindow (SetupWin *win)
//
//  DESCRIPTION:
//	Create the generic setup window, filling in portions of the SetupWin
//	structure as necessary.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
createSetupWindow (SetupWin *win)
{
	log3 (C_FUNC, "createSetupWindow(setupWin=", win, ")");

	switch (setupObjectType (win->object))
	{
	    case sot_web:		//  List of variables

		switch (setupWebType (win->object))
		{
		    case swt_primary:	//  DON'T SUPPORT THIS!!???
				win->isPrimary = True;
				win->highLevelWid=XmCreateMainWindow(win->topLevel,
							"main", 0, 0);
				XtManageChild (win->highLevelWid);
				createPrimaryWindow (win->highLevelWid, win);
				break;
		    case swt_secondary:
				win->highLevelWid = XmCreateForm (win->topLevel,
						"form", (ArgList)0, (Cardinal)0);
				win->isPrimary = False;
				createSecondaryWindow (win->highLevelWid, win);
				XtManageChild (win->highLevelWid);
				break;
		    default:
				win->highLevelWid = XmCreateForm (win->topLevel,
						"form", (ArgList)0, (Cardinal)0);
				win->isPrimary = False;
				createSecondaryWindow (win->highLevelWid, win);
				XtManageChild (win->highLevelWid);
				break;
		}
		break;

	    case sot_list:	//  Primary with browser
		win->isPrimary = True;
		win->objList = win->object;
		win->highLevelWid = XmCreateMainWindow(win->topLevel, "main", 0, 0);
		XtManageChild (win->highLevelWid);
		createPrimaryWindow (win->highLevelWid, win);
		break;

	    case sot_objectList: //  Some error or unsupported condition
	    case sot_var:	 //  occurred.  Probably no ASCII config file
	    default:		 //  exists for this app.  Put up an empty
				 //  (default (generic) secondary) window).
		win->highLevelWid = XmCreateForm (win->topLevel,
				"form", (ArgList)0, (Cardinal)0);
		win->isPrimary = False;
		createSecondaryWindow (win->highLevelWid, win);
		XtManageChild (win->highLevelWid);
		break;
	}

	/*  Set up the mnemonic info for all focus widgets in this dialog.    */
	/*  Support the "Help" key (i.e. F1).
	//  We must set the XmNhelpCallback on the highest level Motif widget.*/
//	XtAddCallback (win->highLevelWid, XmNhelpCallback,
//				(XtCallbackProc)doHelpActionCB, (XtPointer)win);

}	//  End  createSetupWindow ()


/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  createSecondaryWindow (Widget parent, SetupWin *win)
//
//  DESCRIPTION:
//	Create a secondary General Setup window, filling in portions of the
//	SetupWin structure as necessary.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
createSecondaryWindow (Widget parent, SetupWin *win)
{
	Widget	pane;
	int	i;

	/*  The pane does some managing for us (between the control area and
	//  the action area), and also gives us a separator line.	     */
	pane = XtVaCreateWidget ("pane",
				xmPanedWindowWidgetClass,	parent,
				XmNsashWidth,		1,  //  0 = invalid so use 1
				XmNsashHeight,		1,  //  so user won't resize
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				NULL);

	/*  Create the control area (main area of the window) of the "dialog".*/
	(void)createControlArea (pane, win);

	/*  Initialize the button client data.				     */
	for (i = 0 ; i < numButtons ; i++)
		actionItems[i].clientData = win;

	/*  Create the action area (lower buttons) of the "dialog".	     */
/*
	win->actionArea = createActionArea (pane, actionItems,
				XtNumber (actionItems), parent, NULL);
*/
	win->actionArea = createActionArea (pane, actionItems,
				XtNumber (actionItems), parent, NULL);
	XtManageChild (pane);
	turnOffSashTraversal (pane);

}	/*  End  createSecondaryWindow ()				     */



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	resizeCB (Widget w, SetupWin *win, XEvent *xEvent)
//
//  DESCRIPTION:
//	We capture the scrolled window resize events because the form inside it
//	will not resize.  Since we want the widgets inside the form to grow and
//	shrink width-wise within the scrolled window (in some cases), we set the
//	width of the form to fit the scrolled window.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
resizeCB (Widget w, SetupWin *win, XEvent *)
{
	Dimension	width;


	log1 (C_FUNC, "resizeCB()");

	/*  Get the width of the scrolled window to resize the form in it.   */
	XtVaGetValues (w,		XmNwidth,		&width,
					NULL);
	log2 (C_ALL, "\tNew size of scrolled window = ", width);

	/*  Let the width grow (but not shrink) with the scrolled window when
	//  resizing.  This way, horizontal scrollbars will appear only if the
	//  user makes the window too narrow, and if the user makes the window
	//  wider, the variables (esp. text fields, integers, and passwds) will
	//  stretch out. Don't let the height change, so that the scrollbars
	//  will appear if needed.					     */
	if (width >= win->varListWidth)
	{
		log2(C_ALL,"\tSW width >= form width: Setting form width to ",
						    width-WIDTH_OFFSET);
		XtVaSetValues (win->varList,	XmNwidth,	width-WIDTH_OFFSET,
						NULL);
	}
	else	/*  new width < original size- keep it that way for scrollbars */
	{
		log2(C_ALL,"\tSW w < form width: Setting form w=",win->varListWidth);
		XtVaSetValues (win->varList,	XmNwidth,	win->varListWidth,
						NULL);
	}
}



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  createPrimaryWindow (Widget parent, SetupWin *win)
//
//  DESCRIPTION:
//	Create a primary setup window, filling in portions of the SetupWin
//	structure as necessary.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

class Action;
class Menubar;
Action	*d_exitAction;
Action	*d_helpAppAction, *d_helpTOCAction, *d_helpHelpDeskAction;

void
createPrimaryWindow (Widget parent, SetupWin *win)
{


	/*  Create the menubar Action objects.				     */
	d_exitAction = new Action ((XtCallbackProc)exitCallback,
					(XtPointer)win, 0);

	if (!createMenubar (parent, win))
		log1 (C_ERR, "ERR: createMenubar() failed!");

	/*  Create the control area of the "dialog".			     */
	(void)createMainWindowArea (parent, win);

}	//  End  createPrimaryWindow ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Boolean  createMenubar (Widget parent, SetupWin *win)
//
//  DESCRIPTION:
//	Create the menubar.
//
//  RETURN:
//	True if all went well, False if something couldn't get created..
//
////////////////////////////////////////////////////////////////////////////// */

static Boolean
createMenubar (Widget parent, SetupWin *win)
{
	Menubar		*d_menuBar;
	MenubarMenu	*d_helpMenuPulldown;

	if (!(d_menuBar = new Menubar (parent)))
		return (False);

	if (!(createFileMenu (d_menuBar)))
		return (False);

//	if (!(d_helpMenuPulldown = createHelpMenu (d_menuBar, win)))
//		return (False);

//	d_menuBar->helpMenu (d_helpMenuPulldown);

	return (True);

}	//  End  createMenubar ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static MenubarMenu  *createFileMenu (Menubar *menubar)
//
//  DESCRIPTION:
//	Create the File pulldown menu in the menubar.
//
//  RETURN:
//	A MenubarMenu instance if successfull, False if it wasn't.
//
////////////////////////////////////////////////////////////////////////////// */

static MenubarMenu *
createFileMenu (Menubar *menuBar)
{
	MenubarMenu*	pulldown;
	XmString	str;
	Arg		args[32];
	int		count;

	/*  Create the File pulldown menu				     */
	count = 0;
	XtSetArg (args[count], XmNlabelString,
			XmStringCreateLtoR (getStr (TXT_fileTitle),
					XmSTRING_DEFAULT_CHARSET));
	count++;
	XtSetArg (args[count], XmNmnemonic, *(getStr (MNEM_fileTitle)));
	count++;
	if (!(pulldown = new MenubarMenu (menuBar, "fileWidget", args, count))) {
		return (False);
	}

	//  Add an "Exit" item to the pulldown menu
	count = 0;
	XtSetArg (args[count],
			  XmNlabelString,
			  XmStringCreateLtoR (getStr (TXT_fileExitTitle),
					XmSTRING_DEFAULT_CHARSET));
	count++;
	XtSetArg (args[count], XmNmnemonic, *(getStr (MNEM_fileExitTitle)));
	count++;
	str = XmStringCreateLtoR ("<Alt>F4", XmSTRING_DEFAULT_CHARSET);
	XtSetArg (args[count], XmNacceleratorText, str);
	count++;
	new MenubarItem (pulldown, d_exitAction, "exitWidget", args, count);
	XmStringFree (str);

	return (pulldown);

}	//  end  createFileMenu ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static MenubarMenu  *createHelpMenu (Menubar *menubar, SetupWin *win)
//
//  DESCRIPTION:
//	Create the Help pulldown menu in the menubar.
//
//  RETURN:
//	A MenubarMenu instance if successfull, False if it wasn't.
//
////////////////////////////////////////////////////////////////////////////// */

static MenubarMenu*
createHelpMenu (Menubar *menuBar, SetupWin *win)
{
//	MenubarMenu*	pulldown;
//	MenubarItem	*helpApp, *helpTOC, *helpHelpDesk;
//	XmString	xmLabel;
//
//	//  Create the "Help" pulldown menu
//	if (!(pulldown = new MenubarMenu (menuBar, "helpWidget", NULL, 0)))
//	{
//		return (0);
//	}
//
//	xmLabel  = XmStringCreateLocalized (getStr(TXT_helpTitle));
//	XtVaSetValues (pulldown->button(),
//				XmNlabelString,	    xmLabel,
//				XmNmnemonic,	    *(getStr (MNEM_helpTitle)),
//				NULL);
//	XmStringFree (xmLabel);
//
//	//  Create the "Help App..." menu item.
//	helpApp = new MenubarItem (pulldown, d_helpAppAction,
//						"helpAppWidget", NULL, 0);
//
//	xmLabel  = XmStringCreateLocalized (getStr(win->help->title));
//	XtVaSetValues (helpApp->widget(),
//				XmNlabelString,	    xmLabel,
//				XmNmnemonic,	    'N',
//				NULL);
//	XmStringFree (xmLabel);
//
//	//  Create the "Table of Contents..." menu item.
//	helpTOC = new MenubarItem (pulldown, d_helpTOCAction,
//						"helpTOCWidget", NULL, 0);
//	xmLabel  = XmStringCreateLocalized (getStr(TXT_helpTOC));
//	XtVaSetValues (helpTOC->widget(),
//				XmNlabelString,	   xmLabel,
//				XmNmnemonic,	   *(getStr (MNEM_helpTOC)),
//				NULL);
//	XmStringFree (xmLabel);
//
//	//  Create the "Help Desk..." menu item.
//	helpHelpDesk = new MenubarItem (pulldown, d_helpHelpDeskAction,
//						"helpHelpDesk", NULL, 0);
//	xmLabel  = XmStringCreateLocalized (getStr(TXT_helpHelpDesk));
//	XtVaSetValues (helpHelpDesk->widget(),
//				XmNlabelString,	   xmLabel,
//				XmNmnemonic,	   *(getStr(MNEM_helpHelpDesk)),
//				NULL);
//	XmStringFree (xmLabel);
//
//	return (pulldown);
//
}	//  End  createHelpMenu ()


static void
doAppHelpCB (Widget w, XtPointer clientData, XtPointer)
{
}

static void
doTOCHelpCB (Widget w, XtPointer clientData, XtPointer) {
}

static void
doHelpDeskHelpCB (Widget w, XtPointer clientData, XtPointer) {
}



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	turnOffSashTraversal (Widget pane)
//
//  DESCRIPTION:
//	This function turns off tabbing traversal to the sash in the paned
//	window.  This must be done after the paned window widget is managed.
//	We cannot simply set XmNtraversalOn to False on the paned window
//	widget, since it is a manager widget (so all its children are
//	members of its tab group), and thus would turn off tab traversal
//	for all its children. This code was taken from O'Reilly's Vol 6A-
//	Motif Programming Manual for OSF/Motif Release 1.2.
//
//  RETURN:
//	Nothing.
//

void
turnOffSashTraversal (Widget pane)
{
	Widget	*children;
	int	numChildren;

	XtVaGetValues (pane,	XmNchildren,	&children,
				XmNnumChildren,	&numChildren,
				NULL);

	while (numChildren-- > 0)
	{
		if (XmIsSash (children[numChildren]))
		{
			XtVaSetValues (children[numChildren],
				XmNtraversalOn,		False,
				NULL);
		}
	}
}



static void
exitCallback (Widget, XtPointer data, XtPointer)
{
	exit (0);
}
