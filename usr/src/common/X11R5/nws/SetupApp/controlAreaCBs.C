#ident	"@(#)controlAreaCBs.C	1.4"
/*  controlAreaCBs.C
//
//  This file contains all of the callbacks associated with events we need
//  to capture for the various controls in the control area of a window.
//  Some of the events we much process include: when the "Category" option menu
//  gets switch by the user, when the "Yes/No" radio button gets toggled,
//  and when the focus leaves a variable.
*/


#include	<iostream.h>		//  for cout()
#include	<limits.h>		//  for MB_LEN_MAX
#include	<stdlib.h>		//  for strtol ()
#include	<errno.h>		//  to get errno

#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/Text.h>		//  for XmTextSetString ()
#include	<Xm/TextF.h>		//  for XmTextFieldGetString ()
#include	<Xm/ToggleB.h>		//  for XmToggleButtonSetState ()

#include	"dtFuncs.h"		//  for getStr()
#include	"treeBrowse.h"		//  for type svt_list (browser)
#include	"controlArea.h"		//  for ButtonItem
#include	"controlAreaCBs.h"	//  for functions in this file
#include	"setup.h"		//  for AppStruct
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database



/*  External functions, variables, etc.					     */

extern void	errorDialog (Widget topLevel, char *errorText,
							setupObject_t *curObj);
extern void	createPasswdDialog (Widget parent, setupObject_t *pwdObj);
extern void	createListDialog   (Widget topLevel, setupObject_t *object);
extern void	createBrowseDialog (Widget parent, setupObject_t *object);

extern AppStruct	app;
extern Widget		pwdDialog;
extern Widget		errDialog;
extern setupObject_t	*lastFocused;


/*  Local variables, etc.						     */

Boolean	pwdChanged = False;
Boolean pwdActivate = False;





/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	categoryCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	A new option menu item was selected from the "Category" menu.  Get
//	rid of the list of variables that were being displayed (memory and all),
//	and get the new list of variables.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
categoryCB (Widget, XtPointer callData, XmAnyCallbackStruct *)
{
	ButtonItem	*button = (ButtonItem *)callData;
	setupObject_t	*curObj;


	log1 (C_FUNC, "categoryCB()");

	XtRemoveEventHandler (button->win->varWin, StructureNotifyMask,
			False, (XtEventHandler)resizeCB, (XtPointer)button->win);

	/*  Set the wait cursor 'cuz this may take awhile....		     */
	setCursor (C_WAIT, button->win->window, C_FLUSH);

	/*  Free the client data memory for each variable we had
	//  in the previous variable list ("Category") displayed.	     */
	for (curObj = setupObjectListNext(button->win->objList,(setupObject_t *)0);
		curObj ; curObj = setupObjectListNext(button->win->objList,curObj))
	{
		//  Don't free the list we're going to, just the list we're
		//  coming from.
		if (button->curObj == curObj)
			continue;

		setupObject_t *curVar= (setupObject_t *)0;

		for (curVar = setupObjectListNext (curObj, (setupObject_t *)0) ;
		     curVar ; 
		     curVar = setupObjectListNext (curObj, curVar))
		{
			if (setupObjectClientData (curVar))
			{
				XtFree ((char *)setupObjectClientData (curVar));
				setupObjectClientDataSet (curVar, NULL);
			}
		}
	}

	XtDestroyWidget (button->win->varList);

	/*  It looks better when the whole scrolled window is unmapped.
	//  Otherwise the user sees garbage in it.			     */
	XtUnmapWidget (button->win->varWin);

	/*  Create the managed "Variable" list (containing textfields, etc.)
	//  for this "Category" in the option menu.			     */
	button->win->varList = createVariableList (button->win->varWin,
						button->curObj, button->win);
	/*  Set the cursor back to the arrow.				     */
	setCursor (C_POINTER, button->win->window, C_FLUSH);

	XtRealizeWidget (button->win->varList);
	XtMapWidget (button->win->varWin);

	/*  Set the input focus to the first variable in the list, and
	//  make sure the description for that variable is displayed.	     */
	lastFocused = (setupObject_t *)0;
	setVariableFocus (button->win->firstObj);


}	//  End  categoryCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This is called when an option menu pulldown item button is selected.
//	All we do is set the current value of the setup variable by telling it
//	which value of the option menu item was selected, and make sure we are
//	showing the current description if it is not already being displayed.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
{
	ButtonItem	*button = (ButtonItem *)callData;
	char		*charValue, *err;
	long		longValue, *longPtr = &longValue;


	log1 (C_FUNC, "optionCB()");

	switch (setupVarType (button->curObj))
	{
	    default:			//  fall through to type svt_string
	    case  svt_string:
			setupChoiceValue (button->choice, &charValue);
			err = setupVarSetValue (button->curObj, &charValue);
			break;

	    case  svt_integer:
			setupChoiceValue (button->choice, &longValue);
			err = setupVarSetValue (button->curObj, &longPtr);
			break;
	}

	if (err)
	{
		if (!errDialog)
		      errorDialog (button->win->topLevel, err, button->curObj);
	}

	log3 (C_API, "\tjust did setupVarSetValue(", button->choice, ")");

	//  Make sure we show the user this variable description.
	setVariableFocus (button->curObj);

}	//  End  optionCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	setOptionMenuValue (Widget optionMenu, setupObject_t *curObj)
//
//  DESCRIPTION:
//	This function sets the current object (an option menu) to the value
//	retrieved from setup library.
//
//  RETURN:
//	Nothing
//
////////////////////////////////////////////////////////////////////////////// */

void	setOptionMenuValue (Widget optionMenu, setupObject_t *curObj)
{
	Widget		*widList, widToSet = NULL;
	setupObject_t	*listObj;
	setupVariableType_t listType;
	setupChoice_t	*choice;
	char		*charValue, *curCharValue;
	int		*intValue, curIntValue, i;


	if (!(listObj = setupVarList (curObj)))
	{
		log1 (C_ERR,"setOptionMenuValue(): No listObject! Returning.");
		return;
	}

	switch (listType = setupChoiceListType(listObj))
	{
		case svt_string:
			setupObjectGetValue (curObj, &charValue);
			break;
		case svt_integer:
			setupObjectGetValue (curObj, &intValue);
			break;
		default:
			return;		//  don't support anything else
	}

	XtVaGetValues (optionMenu, XmNuserData, &widList, NULL);

	for (i = 0 ; widList[i] && (choice = setupChoiceListChoice(listObj,i));
			 i++)
	{
		switch (listType)
		{
			case svt_string:
				setupChoiceValue (choice, &curCharValue);

				if (!strToWideCaseCmp (curCharValue, charValue))
					widToSet = widList[i];

				break;

			case svt_integer:
				setupChoiceValue (choice, &curIntValue);

				if (curIntValue == *intValue)
					widToSet = widList[i];

				break;

			default:
				return;		//  don't support anything else
		}
	}

	if (widToSet)
		XtVaSetValues (optionMenu, XmNmenuHistory, widToSet, NULL);

}	//  End  setOptionMenuValue ()



void
varListChgCB (Widget, SetupWin *win, XConfigureEvent *event)
{

	log1 (C_FUNC, "varListChgCB()");

	if (event->type != Expose)
		return;

	XtRemoveEventHandler (win->varList, ExposureMask, False,
			  (XtEventHandler)varListChgCB, (XtPointer)win->varWin);

}	//  End  varListChgCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  labelCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	The user clicked on a label (which is really a button) associated with
//	a variable.  Simply call setVariableFocus() which shows the user the
//	description for the variable, as well as sets focus to that variable.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
labelCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
{
	
	log1 (C_FUNC, "labelCB()");
	setVariableFocus (curObj);

}	//  End  labelCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	focusCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This variable has received focus, either by clicking on it with the
//	mouse, or by tabbing or using the arrow keys.  Show the user the
//	descriptive text for this variable.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
focusCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
{

	log1 (C_FUNC, "focusCB()");
	setVariableFocus (curObj);

}	//  End  focusCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  losingFocusCB (Widget, setupObject_t *curObj,
//						XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	The variable we did have focus on is now losing focus.  At this point,
//	we get the value of the variable, and set the variable through the
//	setupAPI, which in turn calls the validation function for that variable.
//	An error message is popped up if the user entered something invalid,
//	and the focus is returned to the variable in error.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
losingFocusCB (Widget, setupObject_t *curObj, XmTextVerifyCallbackStruct *cbs)
{
	VarEntry *cData = (VarEntry *)setupObjectClientData (curObj);
	char	 *charValue,	   *origCharValue,
		 *err = (char *)0, *endp = (char *)0;
	unsigned long  tmp = 0ul, *uLong = &tmp, *origULong=(unsigned long *)0;


	log1 (C_FUNC, "losingFocusCB()");

	/*  Find out if the variable is losing focus because of internal
	//  tabbing, CR, etc., rather than setting focus to a diff app.	     */
	if (!(inputIsSetToThisApp (cData->win->topLevel, cData->win->widList)))
	{
		log1 (C_ALL, "\tInput focus is OUTSIDE this app, returning...");
		return;
	}
	log1 (C_ALL, "\tInput focus is INSIDE this app");

	switch (setupVarType (curObj))
	{
	    case svt_string:
		//  listOnly means we have an option menu, and the value was
		//  set when it was changed, so we don't need to set it now.
		if (setupVarListOnly (curObj))
		    break;

		charValue = XmTextFieldGetString (cData->var);
		setupObjectGetValue (curObj, &origCharValue);

		//  If field has not changed, don't bother setting it.
		if (!(strcmp (charValue, origCharValue)))
			break;

		if (err = setupVarSetValue (curObj, *charValue ? &charValue :
								    (char **)0))
		{
		    if (*charValue)
		    {
			log6 (C_API,"\tERR (string): ",setupObjectLabel(curObj),
				  	   "\n\t\tsetupVarSetValue (",charValue,
				  	   ") failed:\n\t\t", err);
		    }
		    else
		    {
			log5 (C_API,"\tERR (string): ",setupObjectLabel(curObj),
				  	   "\n\t\tsetupVarSetValue ((char *0)",
				  	   ") failed:\n\t\t", err);
		    }
		    break;
		}

		log3(C_API,"\tsvt_string: did setupVarSetValue(",charValue,")");
		break;

	    case svt_integer:
		//  If an option menu, the value was set when it was changed,
		//  so we don't need to set it now because of losing focus.
		if (setupVarListOnly(curObj))	//  listOnly means option menu
		    break;

		charValue = XmTextFieldGetString (cData->var);

		if (*charValue)
		{
			*uLong = strtoul (charValue, &endp, 0);

			if (*endp)
			{
			    log3(C_ALL,"*endp has unconverted val (",*endp,")");
			    err = TXT_invalIntChars;
			    break;
			}

			if (errno == ERANGE || *charValue == '-')
			{
			    log1 (C_ALL, "errno==ERANGE (out of range #)");
			    err = TXT_invalIntRange;
			    break;
			}
		}		//  End  if *(charValue)

		if (err)	//  already have an error to popup
		    break;

		setupObjectGetValue (curObj, &origULong);

		//  If field has not changed, don't bother setting it.
		if (*uLong == *origULong)
		    break;

		if (err = setupVarSetValue (curObj, *charValue ?  &uLong :
							(unsigned long **)0))
		{
		    if (*charValue)
		    {
			log6 (C_API, "\tERR (int): ", setupObjectLabel (curObj),
					"\n\t\tsetupVarSetValue (", &uLong,
					") failed:\n\t\t", err);
		    }
		    else
		    {
			log5 (C_API, "\tERR (int): ", setupObjectLabel (curObj),
				"\n\t\tsetupVarSetValue ((unsigned long **)0",
					"(int)) failed:\n\t\t", err);
		    }
		    break;
		}

		if (*charValue)
			log3 (C_API, "\tsvt_integer: did setupVarSetValue(",
								*uLong, ")");
		else
			log2 (C_API, "\tsvt_integer: did setupVarSetValue(",
						     "(unsigned long **)0)");
		break;

	    case svt_password:
		/*  Iff the field changed, pop up 2nd passwd dialog.  */
		if (pwdChanged)
		{
		     if (cbs->reason == XmCR_ACTIVATE)
		     {
			   pwdActivate = True;
			   log1 (C_PWD, "\tpwdActivate = True");
		     }

		     if (!pwdDialog)
			   createPasswdDialog (cData->win->topLevel, curObj);
		}
		break;

	    case svt_flag:	//  Gets set at time of pushing toggle button.
	    default:
		break;

	}	//  End  switch (setupVarType (curObj))

	//  If we had an error setting this variable, popup the error dialog.
	if (err)
	{
		if (!errDialog)
		      errorDialog (cData->win->topLevel, err, curObj);
	}

}	//  End  losingFocusCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  passwdTextCB (Widget w, char **password,
//					    XmTextVerifyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	We catch every character entered into the password field, so we can
//	"display" an invisible asterisk (just in case some other user tries to
//	cut and paste from the field.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
passwdTextCB (Widget, char **password, XmTextVerifyCallbackStruct *cbs)
{
	int	len = 0, x;

	log5 (C_FUNC, "passwdTextCB(Widget, password=", password,
					"(*password=", *password, "), cbs)");
	log4 (C_PWD, "\tcbs->text->ptr=", cbs->text->ptr, ",cbs->text->length=",
							cbs->text->length);

	if (*password)
		len = strlen (*password);

	/*  cbs->text->ptr == NULL happens with a backspace as well as when we
	//  set the textfield with a NULL (as in XmTextSetString(widget, "") */
	if (cbs->text->ptr == NULL || cbs->text->length == 0)
	{
		int i;
		for (i = 0 ; i < len ; i += x)  //  find begin of last char
		{
			if ((x = mblen (&(*password)[i], MB_LEN_MAX)) <= 0)
			{
				cbs->startPos = 0;
				return;		    //  catch null password
			}
		}

		/*  Backspace - terminate.				     */
		(*password)[i-x] = '\0';
		return;
	}

	pwdChanged = True;
	log1 (C_PWD, "\tpwdChanged = True,... got a char..");

	/*  cbs->text->length is always 1 or 0.

	//  Get enough space for the old text + the new char + a '\0' char.
	//  If password is NULL (the 1st time), XtRealloc calls XtMalloc for the
	//  memory. It copies the old contents of password to the new place. */
	if (*password)
	{
		*password = XtRealloc (*password,
				(Cardinal)(len + cbs->text->length + 8));
		log1 (C_PWD, "\t*password XtRealloc'd");
	}
	else
	{
		*password = XtCalloc ((Cardinal)1,
				(Cardinal)(len + cbs->text->length + 8));
		log1 (C_PWD, "\t*password XtCalloc'd");
	}

	strncat (*password, cbs->text->ptr, cbs->text->length);
	(*password)[len + cbs->text->length] = '\0';
	log2 (C_PWD, "\t*password now = ", *password);

	/*  Change the text to a '*' (even though it is "invisible"), so
	//  no one can cut the password from the text field.		     */
	cbs->text->ptr[0] = '*';

}	//  End  passwdTextCB ()



void
popupDialogCB (Widget w, setupObject_t *obj, XmToggleButtonCallbackStruct *cbs)
{
	setupObject_t	*listObj;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (obj);


	log3 (C_FUNC, "popupDialogCB(Widget, object=", obj, ", cbs)");
	//  Turn "off" button right away - don't leave it depressed.
	XmToggleButtonSetState (w, False, False);

	listObj = setupVarList (obj);

	switch (setupObjectType (listObj))
	{
	    case    sot_choiceList:	//  Single-column list
		createListDialog (cData->win->topLevel, obj);
		break;

	    case    sot_list:	//  Multi-Plist
		createBrowseDialog (cData->win->topLevel, obj);
		break;

	    case    sot_none:	//  Fall thru, do nothing special
		break;
	    default:		//  Error: setup.def could be bad.
		break;
	}

	//  Make sure that the text field will get focus again afterwards
	lastFocused = (setupObject_t *)0;

}	//  End  popupDialogCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	toggleCB (Widget w, setupObject_t *curObj
//					     XmToggleButtonCallbackStruct *cbs)
//
//  DESCRIPTION:
//	If the button is being set, set the new value thru the API.
//	While we're at it, we set the description area iff this is a different
//	variable than was last focused.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
toggleCB (Widget w, setupObject_t *curObj, XmToggleButtonCallbackStruct *cbs)
{
	int	    state, *statePtr = &state;
	VarEntry    *cData = (VarEntry *)setupObjectClientData (curObj);
	char	    *err;
	

	log1 (C_FUNC, "toggleCB()");

	setVariableFocus (curObj);

	if (cbs->set)	//  This button is being SET (pushed in).
	{
		//  userData: 0 = Off button, 1 = On button
		XtVaGetValues (w, XmNuserData, &state, NULL);

		if (err = setupVarSetValue (curObj, &statePtr))
		{
		    log1 (C_API, "\tERR: setupVarSetValue(flag) failed");

		    if (!errDialog)
			 errorDialog (cData->win->topLevel, err, curObj);

		     return;
		}

		log3 (C_API, "\tsvt_flag: did setupVarSetValue(", state, ")");
	}

	/*  Don't change state if this button is being released.	     */

}	//  End  toggleCB ()



/*  This function is not used by any of our current apps using SetupApp,
//  but it should be implemented.					     */

void
cbFunc (setupObject_t *curObj)
{
//	int		*intValue;

//	setupObjectGetValue (curObj, &intValue);
}



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Boolean	inputIsSetToThisApp (Widget widget, Widget *widList)
//
//  DESCRIPTION:
//	This function first finds the topLevel shell widget id of the widget,
//	and the window id for that shell, then asks X for the current input
//	focus window (and the window that would get focus next (which we don't
//	use).
//
//  RETURN:
//	True if the window id retrieved from X matches our own, False otherwise.
//
////////////////////////////////////////////////////////////////////////////// */

Boolean
inputIsSetToThisApp (Widget widget, Widget *widList)
{
	Window	inputFocusWindow;
	Window	revertTo;
	Window	thisShellWindow;
	Widget	shell;
	Widget	wid;


	for (shell = widget ; !XtIsShell (shell) ; shell = XtParent (shell));

	thisShellWindow = XtWindow (shell);
	XGetInputFocus (XtDisplay(widget), &inputFocusWindow, (int *)&revertTo);

	if (inputFocusWindow == thisShellWindow)
		return (True);

	//  Special case for option menus (they re-parent themselves)
	wid = XtWindowToWidget (app.display, inputFocusWindow);

	if (wid)
	{
  		if (app.appContext == XtWidgetToApplicationContext (wid))
    			return (True);
	}

	return (False);

}	// End  inputIsSetToThisApp ()
