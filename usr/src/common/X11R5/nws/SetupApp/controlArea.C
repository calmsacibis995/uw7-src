#ident	"@(#)controlArea.C	1.2"
/*  controlArea.C
//
//  This file contains the functions needed to create all items in the control
//  area of a window.  This includes things like the "Category" option menu,
//  the "Variables" list, and the Description area.
*/


#include	<iostream.h>		//  for cout
#include	<unistd.h>		//  for sleep

#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/PushB.h>
#include	<Xm/PushBG.h>
#include	<Xm/RowColumn.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/Separator.h>
#include	<Xm/Text.h>
#include	<Xm/TextF.h>
#include	<Xm/ToggleB.h>

#include	"MultiPList.h"		//  for type svt_list (browser)
#include	"treeBrowse.h"		//  for tree browser
#include	"treeBrowseCBs.h"	//  for tree browser callbacks
#include	"controlArea.h"		//  for ButtonItem definition
#include	"controlAreaCBs.h"	//  for controlArea callbacks
#include	"dtFuncs.h"		//  for HelpText, GUI lib funcs, etc.
#include	"setup.h"		//  for AppStruct
#include	"setupWin.h"		//  for SetupWin
#include	"setupAPIs.h"		//  for setup API definitions
#include	"setup_txt.h"		//  the localized message database
#include	"variables.h"		//  for variables functions

#define		LABEL_LEFT_OFFSET	5
#define		VAR_OFFSET		3


/*  External functions, variables, etc.					     */

extern AppStruct	app;




/*  Local static functions, variables, etc.				     */

static Widget	createControlAreaTitle (Widget parent, XmString text);
static Widget	createLabel (Widget parent, char *widestLabel,
							setupObject_t *curObj);
static Widget	createCategoryMenu (Widget parent, char *label, SetupWin *win);
static Widget	createOptionMenu (Widget parent, char *label, Widget *pulldown);
static void	createOptionButton (Widget parent, char *label, char *mnem,
				      XtCallbackProc callback, ButtonItem *button);
static int	calcMaxLabelWidth (setupObject_t *object, XmFontList fontList,
						setupObject_t **widestObj);
static int	calcListMaxLabelWidth (setupObject_t *object, XmFontList fontList,
						setupObject_t **widestObj);
static Widget	createVariable (Widget parent, setupObject_t *curObj, SetupWin *win);
static Widget	createMenu (Widget parent, setupObject_t *curObj, SetupWin *win);
static Widget	createDescriptionArea (Widget parent);

setupObject_t	*lastFocused = (setupObject_t *)0;





/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createControlArea (Widget parent, SetupWin *win)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//
////////////////////////////////////////////////////////////////////////////// */

Widget
createControlArea (Widget parent, SetupWin *win)
{
	Widget		controlArea, option, sep1, sep2, varLabel, descLabel;
	Boolean		descRight = True;
	XmString	xmStr;
	Dimension	maxLabelWidth;
	Dimension	totLineWidth;
	Dimension	height = (Dimension)0, width = (Dimension)0;
	XmFontList	fontList;     //  for the high-level form labelFontList
	setupObject_t	*widestObj = (setupObject_t *)0;


	log5 (C_FUNC,"createControlArea(parent=", parent, ",setupWin=",win,")");
	/*  Create the control area of the dialog.  We use a form as the base.*/
	controlArea = XtVaCreateManagedWidget ("ctrlform",
			xmFormWidgetClass,		parent,
			NULL);

	/*  Create the "Category" option menu, if we have > 1 variable list.  */
	if (win->numOpts > 1)
	{
		option = createCategoryMenu (controlArea, TXT_category, win);

		XtVaSetValues (option,	XmNtopAttachment,	XmATTACH_FORM,
				XmNtopOffset,			5,
				XmNleftAttachment,		XmATTACH_POSITION,
				XmNleftPosition,		35,
				NULL);

		sep1 = XtVaCreateManagedWidget ("horizSeparator",
				xmSeparatorWidgetClass,		controlArea,
				XmNorientation,			XmHORIZONTAL,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,			option,
				XmNtopOffset,			5,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				NULL);
	}

	/*  Get the labelFontList from the high level form, and calculate
	//  the longest label width using that font so that we can determine
	//  how wide the scrolled window variable list should be.  We also
	//  use this info for creating the variable label buttons, as well
	//  as for determining if the description area should be to the right
	//  or under the variable list.  If the widest label is very wide,
	//  put the desc area underneath, so that the window does not appear
	//  so wide and short.  If the widestLabel is not very wide, we'll
	//  put the desc are on the right, since that is actually the ideal
	//  place in terms of maximizing what is shown in the desc area
	//  (the user is least likely to have to scroll).	     */

	XtVaGetValues (win->highLevelWid, XmNlabelFontList, &fontList, NULL);
	maxLabelWidth = calcMaxLabelWidth (win->objList, fontList, &widestObj);
	win->widestLabel = strdup (setupObjectLabel (widestObj));
	log2 (C_ALL, "\tmaxLabelWidth (in pixels) = ", maxLabelWidth);

	if (maxLabelWidth > 140)
	{
		descRight = False;
		log1(C_ALL,"\tDescription area on bottom (widestLabel > 140)");
	}
	else
		log1(C_ALL,"\tDescription area on right (widestLabel <= 140)");

	/*  Create a vertical separator. Attach it to the control area form.*/
	if (descRight)
	{
		sep2 = XtVaCreateManagedWidget ("verticalSeparator",
			xmSeparatorWidgetClass,		controlArea,
			XmNorientation,			XmVERTICAL,
			XmNseparatorType,		XmNO_LINE,
			XmNtopAttachment,		XmATTACH_WIDGET,
			XmNtopWidget,    win->numOpts>1?  sep1 : controlArea,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNleftAttachment,		XmATTACH_POSITION,
			XmNleftPosition,		60,
			NULL);
	}

	/*  Create the title of the "Variables" list.  Only if the object
	//  doesn't give us a localized string, get the "Variables" label
	//  from the generic i18n file.					     */
	xmStr = XmStringCreateLocalized (setupObjectLabel (win->objList));
	if (!xmStr)
	{
		xmStr = XmStringCreateLocalized (getStr (TXT_variables));
	}

	varLabel = createControlAreaTitle (controlArea, xmStr);
	XmStringFree (xmStr);

	XtVaSetValues (varLabel,XmNtopAttachment,		XmATTACH_WIDGET,
			XmNtopWidget, win->numOpts>1? sep1: controlArea,
			XmNtopOffset,			6,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNrightAttachment,		XmATTACH_WIDGET,
			XmNrightWidget,	    descRight?  sep2 : controlArea,
			NULL);

	//  This is an attempt to calculate how wide a text field will be.
	//  By default, they are 20 chars wide (but 20 of which chars?),
	//  so this is just an assortment of characters to hopefully 
	//  accomdate the possibility of variable-width chars that could
	//  be filled-in.  (We can't easily get the contents of all the
	//  text fields now).
	XmStringExtent(fontList,XmStringCreateLocalized("GgPXwy9MhLQOFsepZAui"),
							  &width, &height);
	totLineWidth = maxLabelWidth+width+LABEL_LEFT_OFFSET + 2*VAR_OFFSET;
	log3 (C_ALL, "\ttotLineWidth=", totLineWidth,
		    " (maxLabelWidth+width+LABEL_LEFT_OFFSET+2*VAR_OFFSET)");
	log6 (C_ALL,"\t\twidth=",width,", LABEL_LEFT_OFFSET=",LABEL_LEFT_OFFSET,
						", VAR_OFFSET=", VAR_OFFSET);
	log2 (C_ALL, "\tWIDTH_OFFSET=", WIDTH_OFFSET);
	log3 (C_ALL, "\tSetting scrolled window width to win->varListWidth = ",
	      45+totLineWidth+WIDTH_OFFSET, " (45+totLineWidth+WIDTH_OFFSET)");
	win->varListWidth = 45+totLineWidth+WIDTH_OFFSET;

	/*  Create the scrolled window for the variables, & attach to form.  */
	win->varWin = XtVaCreateManagedWidget ("scrolled_w",
			xmScrolledWindowWidgetClass,	controlArea,
			XmNscrollingPolicy,		XmAUTOMATIC,
			XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
			XmNvisualPolicy,		XmVARIABLE,
			XmNtopAttachment,		XmATTACH_WIDGET,
			XmNtopWidget,			varLabel,
			XmNtopOffset,			1,
			XmNbottomAttachment, descRight? XmATTACH_FORM :
							    XmATTACH_NONE,
			XmNbottomOffset,		6,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			6,
			XmNrightAttachment,		XmATTACH_WIDGET,
			XmNrightWidget,	     descRight? sep2 : controlArea,
			XmNrightOffset,			2,
			XmNwidth,			win->varListWidth,
			XmNheight,	     descRight? (Dimension)230 :
						        (Dimension)180,
			NULL);

	/*  Create the managed "Variable" list (containing textfields, etc.)
	//  that go in the scrolled window. This first time, we should display
	//  the contents of the first "Category" menu item.		     */
	win->varList = createVariableList (win->varWin, win->numOpts > 1 ? 
			 setupObjectListNext (win->objList,
				(setupObject_t *)0) : win->objList, win);

	/*  Create the title of the description area in the control area.    */
	descLabel = createControlAreaTitle (controlArea,
			   XmStringCreateLocalized (getStr (TXT_rightTitle)));

	/*  Create the Description area window.  Since XmCreateScrolledText()
	//  is used, the widget returned is that of the Text widget, not of the
	//  scrolled window.  When the scrolledwin widget id is required (for
	//  attachments, for instance) you'll see XtParent(win.descArea) used
	//  below, but just win.descArea when Text resources are being set.  */
	win->descArea = createDescriptionArea (controlArea);

	if (descRight)
	{
		XtVaSetValues (descLabel,
			XmNtopAttachment,		XmATTACH_WIDGET,
			XmNtopWidget,      win->numOpts>1? sep1 : controlArea,
			XmNtopOffset,			6,
			XmNleftAttachment,		XmATTACH_WIDGET,
			XmNleftWidget,			sep2,
			XmNrightAttachment,		XmATTACH_FORM,
			NULL);
	}
	else
	{
		XtVaSetValues (descLabel,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNbottomAttachment,		XmATTACH_WIDGET,
			XmNbottomWidget,		XtParent(win->descArea),
			XmNbottomOffset,		1,
			NULL);

		XtVaSetValues (win->descArea,
			XmNrows,			5,
			NULL);

		XtVaSetValues (win->varWin,
			XmNbottomAttachment,		XmATTACH_WIDGET,
			XmNbottomWidget,		descLabel,
			XmNbottomOffset,			6,
			NULL);
	}

	/*  Attach the description area  to the form.			     */
	XtVaSetValues (XtParent (win->descArea),
		       XmNtopAttachment,descRight?XmATTACH_WIDGET:XmATTACH_NONE,
			XmNtopWidget,			descLabel,
			XmNtopOffset,			1,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		6,
			XmNleftAttachment,		XmATTACH_WIDGET,
			XmNleftWidget,	    descRight? sep2 : controlArea,
			XmNleftOffset,			6,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNrightOffset,			6,
			NULL);

	return (controlArea);

}	//  End  createControlArea ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createMainWindowArea (Widget parent, SetupWin *win)
//
//  DESCRIPTION:
//	Create the Main window area of the primary window.
//
//  RETURN:
//	Return the widget id of the Main Window area.
//
////////////////////////////////////////////////////////////////////////////// */

Widget
createMainWindowArea (Widget parent, SetupWin *win)
{
	Widget		controlArea, descLabel;


	log5(C_FUNC,"createMainWindowArea(parent=",parent,",setupWin=",win,")");
	/*  Create the control area of the dialog.  Use a form as the base. */
	controlArea = XtVaCreateManagedWidget ("ctrlform",
			xmFormWidgetClass,		parent,
			NULL);

	/*  Create the managed "Variable" list (containing textfields, etc.)
	//  that go in the scrolled window.				     */
	win->varList = createVariableList (controlArea, win->numOpts>1? 
			   setupObjectListNext(setupObjectListNext(win->objList,
				  (setupObject_t *)0),(setupObject_t *)0) :
				  win->objList, win);

	XtVaSetValues (win->varList, XmNtopAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNleftOffset,		3,
				XmNrightAttachment,	XmATTACH_FORM,
				XmNrightOffset,		6,
				NULL);

	/*  Create the title of the description area.			     */
	descLabel = createControlAreaTitle (controlArea,
			     XmStringCreateLocalized (getStr (TXT_rightTitle)));

	/*  Create the Description area window.  Since XmCreateScrolledText()
	//  is used, the widget returned is that of the Text widget, not of the
	//  scrolled window.  When the scrolledW widget id is required (for
	//  attachments, for instance) you'll see XtParent(win.descArea) used
	//  below, but just win.descArea when Text resources are being set.  */
	win->descArea = createDescriptionArea (controlArea);

	XtVaSetValues (descLabel,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNrightAttachment,	XmATTACH_FORM,
				XmNbottomAttachment,	XmATTACH_WIDGET,
				XmNbottomWidget,	XtParent(win->descArea),
				XmNbottomOffset,	1,
				NULL);

	XtVaSetValues (win->descArea,
				XmNrows,		5,
				NULL);

	XtVaSetValues (win->varList,
				XmNbottomAttachment,	XmATTACH_WIDGET,
				XmNbottomWidget,	descLabel,
				XmNbottomOffset,		6,
				NULL);

	//  Attach the description area  to the form.
	XtVaSetValues (XtParent (win->descArea),
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNbottomOffset,		6,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNleftOffset,			6,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNrightOffset,			6,
				NULL);

	return (controlArea);

}	//  End  createMainWindowArea ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createCategoryMenu (Widget parent, char *label,
//								 SetupWin *win)
//
//  DESCRIPTION:
//	Create the Category option menu. This is displayed at the top of the
//	setup window.  The ASCII config file for this window (hence, the setup
//	APIs) tell us which and how many option menu items there are.
//	Note that this function is never called if the setup window has only
//	variables, or one list of variables.
//
//  RETURN:
//	Return the widget id of the option menu widget.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createCategoryMenu (Widget parent, char *label, SetupWin *win)
{
	Widget		pulldown, option;
	setupObject_t	*curObj;
	ButtonItem	*bData;
	int		i = 0;


	log1 (C_FUNC, "createCategoryMenu()");
	option = createOptionMenu (parent, label, &pulldown);

	for (curObj = setupObjectListNext (win->objList, (setupObject_t *)0) ;
		     curObj ; curObj = setupObjectListNext(win->objList,curObj))
	{
		bData = (ButtonItem *)XtCalloc (1,(Cardinal)sizeof(ButtonItem));
		bData->win   = win;
		bData->curObj= curObj;

		createOptionButton (pulldown, setupObjectLabel (curObj) ?
				setupObjectLabel (curObj): TXT_noLabel,
				setupObjectMnemonic (curObj),
				(XtCallbackProc)categoryCB, bData);

		setupObjectClientDataSet (curObj, bData);
		i++;
	}

	return (option);

}	//  End  createCategoryMenu ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createOptionMenu (Widget parent, char *label,
//							    Widget *pulldown)
//
//  DESCRIPTION:
//	Create an option menu.  This is either the "Category" option menu at
//	the top of the setup window (i.e. with Basic or Extended options), or,
//	it is a type of variable, (that shows up in the Variables list).
//
//  RETURN:
//	Return the widget id of the option menu widget.
//
////////////////////////////////////////////////////////////////////////////// */

static Widget
createOptionMenu (Widget parent, char *label, Widget *pulldown)
{
	Widget	option;
	Arg	args[2];
	Cardinal argc = 0;


	*pulldown = XmCreatePulldownMenu (parent, "optionPulldown", NULL, 0);

	XtSetArg (args[argc],	XmNsubMenuId,	*pulldown);	argc++;
	XtSetArg (args[argc],	XmNlabelString,	XmStringCreateLocalized
					(getStr (label))); argc++;

	option = XmCreateOptionMenu (parent, (getStr (label)), args, argc);

	/*  Manage option menu, not pulldown menu.			     */
	XtManageChild (option);

	return (option);

}	//  End  createOptionMenu ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void createOptionButton (Widget parent, char *label, char *mnem,
//				XtCallbackProc callback, ButtonItem *button)
//
//  DESCRIPTION:
//	Create the option menu item button.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static void
createOptionButton (Widget parent, char *label, char *mnem,
				    XtCallbackProc callback, ButtonItem *button)
{

	button->widget = XtVaCreateManagedWidget ("button",
		xmPushButtonGadgetClass,	parent,
		XmNlabelString,	XmStringCreateLocalized (getStr (label)),
		XmNmnemonic,	*(getStr (mnem)),
		NULL);

	XtAddCallback (button->widget, XmNactivateCallback, callback,
							     (XtPointer)button);
}	//  End  createOptionButton ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createControlAreaTitle (Widget parent, XmString text)
//
//  DESCRIPTION:
//	Create a label widget containing the text designated by text.
//
//  RETURN:
//	Return the widget id of the managed label widget.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createControlAreaTitle (Widget parent, XmString text)
{
	Widget	title;

	title = XtVaCreateManagedWidget ("title",
				xmLabelWidgetClass,		parent,
				XmNlabelString,			text,
				NULL);
	return (title);

}	//  End  createControlAreaTitle ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createVariableList (Widget parent, setupObject_t *varObjs,
//								SetupWin *win)
//
//  DESCRIPTION:
//	Create the subpart of the main part of the control area which is the 
//	list of "Variable" "lines" (comprised of textfields, etc.).
//
//  RETURN:
//	Return the widget id of the variable list (the form, in this case).
//
///////////////////////////////////////////////////////////////////////////// */

Widget
createVariableList (Widget parent, setupObject_t *varObjs, SetupWin *win)
{
	Widget		varList, lineForm = (Widget)0;
	setupObject_t	*curObj;
	VarEntry	*cData;
	int		i;
	static Boolean	firstTime = True;


	log1 (C_FUNC, "createVariableList()");

	lastFocused =  (setupObject_t *)0;

	varList = XtVaCreateManagedWidget ("varform",
			xmFormWidgetClass,		parent,
			XmNresizePolicy,		firstTime?
						XmRESIZE_NONE : XmRESIZE_GROW,
			NULL);

	if (setupObjectType (varObjs) == sot_list)
	{
		cData = (VarEntry *)XtCalloc (1, (Cardinal)sizeof (VarEntry));
		cData->win = win;
		setupObjectClientDataSet (varObjs, cData);

		lineForm = XtVaCreateManagedWidget ("lineForm",
			xmFormWidgetClass,		varList,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNnavigationType,		XmNONE,
			NULL);

		XtAddCallback (lineForm, XmNfocusCallback,
					    (XtCallbackProc)focusCB, varObjs);

		cData->win->treeBrowse = new treeBrowse (app.appContext,
						lineForm, varObjs,
						(XtPointer)win, True, 2,6);

		cData->var = cData->win->treeBrowse->getTopWidget ();
		cData->win->treeBrowse->setSelectCallback ((SelectCallback)
						treeNodeSelectedCB, varObjs);
		cData->win->treeBrowse->setDblClickCallback ((DblClickCallback)
						treeNodeOpenedCB, varObjs);
		cData->win->treeBrowse->setUnselectCallback ((UnselectCallback)
						treeNodeUnSelectedCB,varObjs);

		XtVaSetValues (cData->var,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			1,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			VAR_OFFSET,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNrightOffset,			VAR_OFFSET,
			XmNnavigationType,		XmNONE,
			NULL);
	}
	else
	{
		/*  Save the 1st variable, so we can set focus to it.	     */
		win->firstObj = curObj = setupObjectListNext (varObjs,
							(setupObject_t *)0);
		for (i = 0 ; curObj ; 
			   curObj = setupObjectListNext (varObjs, curObj), i++)
		{
			cData = (VarEntry *)XtCalloc (1,
						(Cardinal)sizeof (VarEntry));
			cData->win = win;

			setupObjectClientDataSet (curObj, cData);

			lineForm = XtVaCreateManagedWidget ("lineForm",
				xmFormWidgetClass,		varList,
				XmNtopAttachment,   i?
						XmATTACH_WIDGET : XmATTACH_FORM,
				XmNtopWidget,			lineForm,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNnavigationType,		XmNONE,
				NULL);

			XtAddCallback (lineForm, XmNfocusCallback,
						(XtCallbackProc)focusCB,curObj);

			if (!win->isPrimary)
			{
			    cData->label = createLabel (lineForm,
						    win->widestLabel, curObj);
			}

			cData->var = createVariable (lineForm, curObj, win);

			if (!win->isPrimary)
			{
				XtVaSetValues (cData->label,
					XmNtopAttachment,	XmATTACH_FORM,
					XmNtopOffset,		i? 2 : 6,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNleftOffset,	      LABEL_LEFT_OFFSET,
					XmNbottomAttachment,	XmATTACH_FORM,
					NULL);
			}

			XtVaSetValues (cData->popupButton?
					     XtParent (cData->var) : cData->var,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNtopOffset,			i? 2 : 6,
				XmNleftAttachment,	win->isPrimary?
					     XmATTACH_FORM : XmATTACH_WIDGET,
				XmNleftWidget,			cData->label,
				XmNleftOffset,			VAR_OFFSET,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNrightOffset,			VAR_OFFSET,
				XmNnavigationType,		XmNONE,
				NULL);

			if (cData->popupButton)
			{
				XtVaSetValues (cData->var,
					XmNnavigationType,	XmNONE,
					NULL);
			}

		}	//  End  looping thru variables
	}

	XtVaSetValues (lineForm,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	if (win->varWin)
	{
		XtAddEventHandler (win->varWin, StructureNotifyMask, False,
				  (XtEventHandler)resizeCB, (XtPointer)win);
		XtAddEventHandler (varList, ExposureMask, False,
				  (XtEventHandler)varListChgCB, (XtPointer)win);
	}

	firstTime = False;

	return (varList);

}	//  End  createVariableList ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createLabel (Widget parent, char *widestLabel,
//							setupObject_t *curObj)
//
//  DESCRIPTION:
//	Create a label but put it on a button that's invisible to the user
//	(shadowThickness = 0, etc.). When the user "touches the label", the
//	description area gets filled in with the appropriate text.  The button
//	is created as wide as widestLabel, then the appropriate label for the
//	curObj is used, which is right-justified on the button.
//
//  RETURN:
//	Return the widget id of the label widget.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createLabel (Widget parent, char *widestLabel, setupObject_t *curObj)
{
	Widget		labelButton;
	char		*label;
	XmString	dummyLabel, xmStr;


	if (!(label = setupObjectLabel (curObj)))
		label = getStr (TXT_noLabel);

	xmStr = XmStringCreateLocalized (label);

	dummyLabel = XmStringCreateLocalized (widestLabel);

	switch (setupVarType (curObj))
	{
		case svt_string  :
		case svt_integer :
		case svt_flag    :
		case svt_password:
		case svt_list    :
		case svt_none	 :  //  Create default label even if type none.
			//  Create a button w/ a label instead of just a label.
			//  The user doesn't see it as a button, but we want to
			//  catch its "arm" event so we can display the
			//  description when the label (button) is touched.
			labelButton = XtVaCreateManagedWidget ("labelButton",
				xmPushButtonWidgetClass,	parent,
				XmNshadowThickness,		0,
				XmNhighlightThickness,		0,
				XmNtraversalOn,			False,
				XmNalignment,			XmALIGNMENT_END,
				XmNrecomputeSize,		False,
				XmNfillOnArm,			False,
				XmNlabelString,			dummyLabel,
				NULL);

			XtVaSetValues (labelButton, XmNlabelString, xmStr,NULL);

			//  Set the "arm" CB so we can set the description
			//  field when the user pushes (as opposed to
			//  releasing) the label.
			XtAddCallback (labelButton, XmNarmCallback,
					      (XtCallbackProc)labelCB, curObj);
			break;

		default:
			break;			//  Major error here...
	}

	XmStringFree (dummyLabel);
	XmStringFree (xmStr);

	return (labelButton);

}	//  End  createLabel ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static int calcMaxLabelWidth (setupObject_t *object, XmFontList fontList)
//
//  DESCRIPTION:
//	Calculate the longest width (in pixels) of the labels in the SetupVar
//	list using the fontList provided.
//
//  RETURN:
//	Return what was just calculated.
//
///////////////////////////////////////////////////////////////////////////// */

static int
calcMaxLabelWidth (setupObject_t *object, XmFontList fontList,
						    setupObject_t **widestObj)
{
	setupObject_t	*curObj;
	setupObject_t	*wide = (setupObject_t *)0, *widest=(setupObject_t *)0;
	Dimension	width = (Dimension)0, maxWidth = (Dimension)0;


	log1 (C_FUNC, "calcMaxLabelWidth(object, fontList)");

	if (!object)
	{
		*widestObj = (setupObject_t *)0;
		return (0);
	}

	if (setupObjectType (setupObjectListNext (object,
					(setupObject_t *)0)) == sot_objectList)
	{
		for (curObj = setupObjectListNext (object,
			       (setupObject_t *)0) ; curObj ;
			       curObj = setupObjectListNext (object,curObj))
		{
			width = calcMaxLabelWidth (curObj, fontList, &wide);

			if (width > maxWidth)
			{
				maxWidth = width; 
				widest = wide;
			}
		}
	}
	else
	{
		maxWidth = calcListMaxLabelWidth (object, fontList, &widest);
	}

	*widestObj = widest;
	return (maxWidth);

}	//  End  calcMaxLabelWidth ()



static int
calcListMaxLabelWidth (setupObject_t *object, XmFontList fontList,
						   setupObject_t **widestObj)
{
	setupObject_t	*curObj, *wideObj;
	Dimension	maxHeight = (Dimension)0, maxWidth = (Dimension)0;
	Dimension	height = (Dimension)0, width = (Dimension)0;
	char		*label;
	XmString	xmStr;


	for (curObj = setupObjectListNext (object, (setupObject_t *)0) ;
			curObj ; curObj = setupObjectListNext (object, curObj))
	{

		if (!(label = setupObjectLabel (curObj)))
		{
			//  Use a default label.
			label = getStr (TXT_noLabel);
		}

		xmStr = XmStringCreateLocalized (label);
		XmStringExtent (fontList, xmStr, &width, &height);

		if (height > maxHeight)
			maxHeight = height;

		if (width > maxWidth)
		{
			maxWidth = width;
			wideObj = curObj;
		}

		XmStringFree (xmStr);
	}

	*widestObj = wideObj;
	return (maxWidth);

}	//  End  calcListMaxLabelWidth ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createVariable (Widget parent, setupObject_t *curObj,
//								SetupWin *win)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createVariable (Widget parent, setupObject_t *curObj, SetupWin *win)
{
	Widget		var, varForm;
	Pixel		bg;
	unsigned long	l, *longValue = &l;
	char		*charValue, buff[256], *buffPtr = buff;;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	setupObject_t	*listObject;
	int		choiceValue;
	setupChoice_t	*choice;



	log1 (C_FUNC, "createVariable()");
	/*  If setupVarList() returns some sort of setupObject_t type,
	//  (we assume sot_choiceList or sot_list) we need a popup button.   */

	switch (setupVarType (curObj))
	{
	    default:		//  There is no variable type.  Use a text
				//  field as the default.  Fall thru...
	    case svt_none:	//     "
				//     "
	    case svt_string:		//  Text Field
		cData->popupButton = setupVarList (curObj);

		if (setupVarListOnly (curObj))
		{
			cData->popupButton = False;
			var = createMenu (parent, curObj, win);
			break;
		}
					//  fall through ...
	    case svt_password:		//  Text-Field, not displaying chars
		if (setupObjectGetValue (curObj, &charValue))
		{
			charValue = "";
		}

		if (!charValue)
			charValue = "";

		if (cData->popupButton && setupVarType (curObj) == svt_string)
		{
			Widget	button;

			varForm = XtVaCreateManagedWidget ("varForm",
				xmFormWidgetClass,		parent,
				NULL);

			//  Create "..." button for the popup.
			var = XtVaCreateManagedWidget ("textF",
				xmTextFieldWidgetClass,		varForm,
				XmNcolumns,			17,
				XmNresizeWidth,			True,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_FORM,
				NULL);

			button = XtVaCreateManagedWidget ("...",
				xmToggleButtonWidgetClass, varForm,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_WIDGET,
				XmNleftWidget,			var,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNlabelString,  XmStringCreateLocalized("..."),
				NULL);

			XtAddCallback (button, XmNvalueChangedCallback,
				(XtCallbackProc)popupDialogCB, curObj);
		}
		else
		{
			var = XtVaCreateManagedWidget ("textF",
				xmTextFieldWidgetClass,		parent,
				XmNresizeWidth,			True,
				NULL);
		}

		/*  Catch Return (or Enter) key so we can retrieve the
		//  current text and set it before we exit the setup window. */
		XtAddCallback (var, XmNactivateCallback,
			     (XtCallbackProc)losingFocusCB, (XtPointer)curObj);

		/*  Catch the losing focus event so we can retrieve the
		//  current text, set it, and thus validate it.		     */
		XtAddCallback (var, XmNlosingFocusCallback,
					 (XtCallbackProc)losingFocusCB, curObj);

		if (setupVarType (curObj) == svt_password)
		{
			XtVaGetValues (var, XtNbackground, &bg, NULL);
			XtVaSetValues (var, XtNforeground,  bg, NULL);

			XtAddCallback (var, XmNmodifyVerifyCallback,
					     (XtCallbackProc)passwdTextCB,
					     (XtPointer)(&(cData->p_1stText)));
			break;
		}

		XmTextFieldSetString (var, charValue);
		break;		//  End  type svt_string or type svt_password

	    case svt_integer:	//  Integer (text) field
		cData->popupButton = setupVarList (curObj);

		if (setupVarListOnly (curObj))
		{
			cData->popupButton = False;
			var = createMenu (parent, curObj, win);
			break;
		}

		getIntValue (curObj, &buffPtr);

		if (cData->popupButton)
		{
			Widget	button;

			varForm = XtVaCreateManagedWidget ("varForm",
				xmFormWidgetClass,		parent,
				NULL);

			//  Create "..." button for the popup.
			var = XtVaCreateManagedWidget ("textF",
				xmTextFieldWidgetClass,		varForm,
				XmNcolumns,			17,
				XmNresizeWidth,			True,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_FORM,
				NULL);

			button = XtVaCreateManagedWidget ("...",
				xmToggleButtonWidgetClass, varForm,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_WIDGET,
				XmNleftWidget,			var,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNlabelString,  XmStringCreateLocalized("..."),
				NULL);

			XtAddCallback (button, XmNvalueChangedCallback,
				(XtCallbackProc)popupDialogCB, curObj);
		}
		else
		{
			var = XtVaCreateManagedWidget ("textF",
					xmTextFieldWidgetClass,	parent,
					XmNresizeWidth,		True,
					NULL);
		}

		XtAddCallback (var, XmNactivateCallback,
			      (XtCallbackProc)losingFocusCB, (XtPointer)curObj);

		XtAddCallback (var, XmNlosingFocusCallback,
					 (XtCallbackProc)losingFocusCB, curObj);
		XmTextFieldSetString (var, buffPtr);

		break;		//  End  type svt_integer

	    case svt_flag:		//  Toggle (2 radio buttons)

		if (setupObjectGetValue (curObj, &longValue))
		{
			log4 (C_ERR, "createVariable(): svt_flag: ",
				setupObjectGetValue(curObj,&longValue),
				" = setupObjectGetValue() ERROR! longValue = ",
				*longValue);
		}

		if (*longValue != 0 && *longValue != 1)
		{
			log4 (C_ERR, "createVariable(): svt_flag: ",
				setupObjectGetValue(curObj,&longValue),
				" = setupObjectGetValue() ERROR! longValue = ",
				*longValue);
			*longValue = 1;		//  No by default
		}

		listObject = setupVarList (curObj);

		if (setupObjectType (listObject) != sot_choiceList)
		{
			exit (1);
		}

		var = XtVaCreateManagedWidget ("rowCol",
				xmRowColumnWidgetClass,	parent,
				XmNorientation,		XmHORIZONTAL,
				XmNpacking,		XmPACK_COLUMN,
				XmNradioBehavior,	True,
				XmNentryClass,	      xmToggleButtonWidgetClass,
				XmNisHomogeneous,	True,
				XmNuserData,		curObj,
				NULL);

		choice = setupChoiceListChoice (listObject, 0);
		(void) setupChoiceValue (choice, &choiceValue);

		cData->f_onBtn = XtVaCreateManagedWidget ("toggleButton2",
			xmToggleButtonWidgetClass,	var,
			XmNlabelString,			listObject ?
			   XmStringCreateLocalized (setupChoiceLabel (choice)) :
			   XmStringCreateLocalized (getStr (TXT_toggleOn)),
			XmNset,	(*longValue==choiceValue) ? True : False,
			XmNuserData,			choiceValue,
			NULL);

		XtAddCallback (cData->f_onBtn, XmNvalueChangedCallback,
				   (XtCallbackProc)toggleCB, (XtPointer)curObj);

		choice = setupChoiceListChoice(listObject, 1);
		(void) setupChoiceValue(choice, &choiceValue);

		cData->f_offBtn = XtVaCreateManagedWidget ("toggleButton1",
			xmToggleButtonWidgetClass,	var,
			XmNlabelString,			listObject ?
			   XmStringCreateLocalized (setupChoiceLabel (choice)) :
			   XmStringCreateLocalized (getStr (TXT_toggleOff)),
			XmNset,	(*longValue==choiceValue) ? True : False,
			XmNuserData,			choiceValue,
			NULL );
			

		XtAddCallback (cData->f_offBtn,	XmNvalueChangedCallback,
				  (XtCallbackProc)toggleCB, (XtPointer)curObj);
		break;		//  End  type svt_flag


	    case svt_list:		//  Multi-PList (tree browser)
		cData->win->treeBrowse = new treeBrowse (app.appContext,
						parent, curObj,
						(XtPointer) win, True, 2,7);

		var = cData->win->treeBrowse->getTopWidget ();
		cData->win->treeBrowse->setSelectCallback ((SelectCallback)
						treeNodeSelectedCB, curObj);
		cData->win->treeBrowse->setDblClickCallback ((DblClickCallback)
						treeNodeOpenedCB, curObj);
		cData->win->treeBrowse->setUnselectCallback ((UnselectCallback)
						treeNodeUnSelectedCB,curObj);
		break;
	}

	return (var);

}	//  End  createVariable ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createDescriptionArea (Widget parent)
//
//  DESCRIPTION:
//	Create the scrolled text widget.
//
//  RETURN:
//	Return the widget id of the ScrolledWindow..
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createDescriptionArea (Widget parent)
{
	Widget	stext;
	Arg	args[5];
	Cardinal argc = 0;


	XtSetArg (args[argc],	XmNeditMode,	     XmMULTI_LINE_EDIT);argc++;
	XtSetArg (args[argc],	XmNeditable,	     False);		argc++;
	XtSetArg (args[argc],	XmNwordWrap,	     True);		argc++;
	XtSetArg (args[argc],	XmNscrollHorizontal, False);		argc++;

	stext = XmCreateScrolledText (parent, "stext", args, argc);
	XtManageChild (stext);

	return (stext);

}	//  End  createDescriptionArea ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	setVariableFocus (setupObject_t *obj)
//
//  DESCRIPTION:
//	Set the focus to the variable object (which is passed in).
//	We also make sure that this forces the description for this
//	variable to be shown in the description area.  Normally,
//	this is done for the first variable in the list after the
//	variable list has just been created, but it can be used anytime,
//	for any variable.  We do assume that all widgets are realized.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
setVariableFocus (setupObject_t *obj)
{
	VarEntry		*cData;
	int			*intValue;
	Widget			option;
	char			*desc;


	log1 (C_FUNC, "setVariableFocus()");

	if (obj == lastFocused)
	{
		return;
	}

	lastFocused = obj;

	if (!(cData = (VarEntry *)setupObjectClientData (obj)))
		return;

	switch (setupVarType (obj))
	{
	    case svt_string:	//  Text field.
	    case svt_integer:	//  Text field that takes integers.
		if (setupVarListOnly (obj))	//  option menu only
		{
			//  Need to get the Cascade button widget to set focus
			option = XmOptionButtonGadget (cData->var);
			(void)XmProcessTraversal (option, XmTRAVERSE_CURRENT);
			break;
		}
				//  fall thru since all text fields equal here
	    case svt_password:	//  Text field- invisible w/ validation popup.

		(void)XmProcessTraversal (cData->var, XmTRAVERSE_CURRENT);
		break;

	    case svt_flag:	//  2 radio buttons.
		setupObjectGetValue (obj, &intValue);

		if (*intValue != 0 && *intValue != 1)
			*intValue = 0;	//  Put up error message here.

		(void)XmProcessTraversal (*intValue?
			  cData->f_onBtn : cData->f_offBtn, XmTRAVERSE_CURRENT);
		break;

	    default:
		break;
	}

	if (!(desc = setupObjectDescription (obj)))
		desc = getStr (TXT_noDescription);

	XmTextSetString (cData->win->descArea, desc);

}	//  End  setVariableFocus ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createMenu (Widget parent, setupObject_t *curObj,
//								SetupWin *win)
//
//  DESCRIPTION:
//	Create the option menu.
//
//  RETURN:
//	Return the widget id of the option menu.
//
///////////////////////////////////////////////////////////////////////////// */

static Widget
createMenu (Widget parent, setupObject_t *curObj, SetupWin *win)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	Widget		var, pulldown, *widList = 0;
	ButtonItem	*buttonData;
	setupObject_t	*listObj;
	setupChoice_t	*choice;
	char		*charValue;
	char		*curCharValue;
	int		*intValue;
	int		curIntValue;
	setupVariableType_t listType;
	int		i;


	listObj = setupVarList (curObj);
	listType = setupChoiceListType (listObj);

	switch (listType)
	{
		case	svt_integer:
			setupObjectGetValue (curObj, &intValue);
			break;

		case	svt_string:
			setupObjectGetValue (curObj, &charValue);
			break;

		default:
			break;
	}

	/*  Create the option menu with no associated label,
	//  since we already created the label earlier.			     */
	var = createOptionMenu (parent, "", &pulldown);

	for ( i = 0 ; choice = setupChoiceListChoice (listObj,i); i++)
	{
		buttonData = (ButtonItem *)XtCalloc (1,
						 (Cardinal)sizeof (ButtonItem));
		buttonData->label = setupChoiceLabel (choice);
		buttonData->choice = choice;
		buttonData->curObj = curObj;
		buttonData->win    = win;
		//  SCOTT: WE NEED CHOICE MNEMONIC SOMETIME
		//  Note:  createOptionButton() sets buttonData->widget
		createOptionButton (pulldown, buttonData->label, (char *)0,
				(XtCallbackProc)optionCB, buttonData);
		widList = (Widget *)XtRealloc ((char *)widList,
				((i+2)*sizeof(Widget)));
		widList[i] = buttonData->widget;
		widList[i+1] = (Widget)0;

		/*  Default: first menu option is the current		     */
		if (i == 0)
			cData->m_origChoice = buttonData->widget;

		/*  We could have a different button than
		//  the first specified as the current one.		     */
		switch (listType)
		{
			case	svt_integer:
			{
				setupChoiceValue (choice, &curIntValue);

				if (curIntValue == *intValue)
				{
				      cData->m_origChoice = buttonData->widget;
				}
				break;
			}
		  
			case	svt_string:
			{
				setupChoiceValue (choice, &curCharValue);

				if (!strToWideCaseCmp (curCharValue, charValue))
				{
				      cData->m_origChoice = buttonData->widget;
				}
				break;
			}

			default:
				break;
		}
	}

	XtVaSetValues (var,	XmNmenuHistory,	cData->m_origChoice,
				XmNuserData,	widList,
				NULL);

	return (var);

}	//  End  createMenu ()



/* ///////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	int strToWideCaseCmp (const char *s1, const char *s2)
//
//  DESCRIPTION:
//	The arguments s1 and s2 point to strings (arrays of characters
//	terminated by a null character).  strToWideCaseCmp converts its
//	arguments to wide character uppercase strings in order to
//	perform the case-insensitive string compare of the two strings.
//
//  RETURN:
//	An integer less than, equal to, or greater than 0, based upon
//	whether the wide string created from s1 is less than, equal to,
//	or greater than the wide string created from s2.
//
/////////////////////////////////////////////////////////////////////////// */
#include	<stdlib.h>		//  for malloc()
#include	<wchar.h>		//  for wcslen() & wcscmp()
#include	<wctype.h>		//  for towupper()

int
strToWideCaseCmp (const char *s1, const char *s2)
{
	wchar_t	*ws1, *ws2;
	int	i;


	//  Get enough space for the wide strings to be.
	ws1 = (wchar_t *)malloc ((strlen (s1) + 1) * sizeof (wchar_t));
	ws2 = (wchar_t *)malloc ((strlen (s2) + 1) * sizeof (wchar_t));

	//  Convert both strings to wide strings.
	(void)mbstowcs (ws1, s1, strlen (s1) + 1);
	(void)mbstowcs (ws2, s2, strlen (s2) + 1);

	//  For the case-insensitive aspect, we convert both
	//  strings to uppercase before doing a compare.
	for (i=0 ; i < wcslen (ws1) ; i++)
	{
		ws1[i] = towupper (ws1[i]);
	}

	for (i=0 ; i < wcslen (ws2) ; i++)
	{
		ws2[i] = towupper (ws2[i]);
	}

	//  Now simply do the compare.
	i = wcscmp (ws1, ws2);

	free (ws1);
	free (ws2);

	return (i);

}	//  End  strToWideCaseCmp ()
