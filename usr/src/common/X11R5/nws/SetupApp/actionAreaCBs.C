#ident	"@(#)actionAreaCBs.C	1.3"
/*  actionAreaCBs.C
//
//  This file contains the functions needed to create the action area of a
//  secondary window.  Essentially, this is all the buttons in the lower
//  portion of the window.
*/


#include	<iostream.h>		//  for cout()
#include	<stdlib.h>		//  for strtol ()

#include	<Xm/Xm.h>
#include	<Xm/TextF.h>

#include	"dtFuncs.h"		//  for HelpText, GUI lib funcs, etc.
#include	"controlArea.h"
#include	"setupWin.h"		//  for SetupWin definiton
#include	"setupAPIs.h"		//  for setupType_t definition
#include	"setup_txt.h"		//  the localized message database
#include	"variables.h"		//  for variable supporting functions



/*  External variables, functions, etc.					     */

extern void   shellWidgetDestroy (Widget w,XtPointer clientData,XmAnyCallbackStruct *cbs);
extern void	errorDialog (Widget topLevel, char *errorText,
							setupObject_t *curObj);
extern Boolean	pwdChanged;
extern Boolean	pwdActivate;
extern Boolean	doMsgDialog;
extern Widget	pwdDialog;
extern Widget	errDialog;



/*  Local variables, functions, etc.					     */

void	doOkActionCB    (Widget w, XtPointer clientData, XmAnyCallbackStruct *);
Boolean	doApplyActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
void	doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
void	resetVarLists (setupObject_t *object);
void	resetVarList (setupObject_t *object);
void	doCancelActionCB(Widget w, XtPointer clientData, XmAnyCallbackStruct *);
void	doHelpActionCB  (Widget, XtPointer, XmAnyCallbackStruct *);

Boolean	doingOK = False;





void
doOkActionCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{

	log1 (C_FUNC, "doOkActionCB()");
	doingOK = True;

	if (pwdActivate)	//  Enter was pressed while in pwd field.
	{
		log1 (C_PWD, "\tpwdActivate is True (Enter pressed), returning..");
		return;
	}

	/*  Alt-O (for Ok button) was pressed while either in a changed pwd
	//  field or while in a field that currently has an invalid value in it.
	//  We return from here since we will come back again after the popup
	//  is done.							     */
	if (pwdDialog || errDialog)
	{
		if (pwdDialog)
			log1 (C_PWD, "\tpwdDialog up (Alt-O pressed), returning..");
		else
			log1 (C_ALL, "\terrDialog up (Alt-O pressed), returning..");
		return;
	}

	if (doApplyActionCB ((Widget)0, clientData, (XmAnyCallbackStruct *)0))
	{
		doCancelActionCB (w, ((SetupWin *)clientData)->object,
						    (XmAnyCallbackStruct *)0);
	}

}	//  End  doOkActionCB ()



Boolean
doApplyActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *)
{
	SetupWin	*win = (SetupWin *)clientData;
	char		*err;


	log1 (C_FUNC, "doApplyActionCB()");
	if (setupWebApply (win->object))
	{
		if (!errDialog)
		{
		    if (!(err = setupObjectErrorStringGet (win->object)))
			err = TXT_okFailed;

		    errorDialog (win->topLevel, err, win->object);
		}
		return (False);
	}

	//  If we have a treeBrowse node that is currently selected,
	//  update the Add and Delete button sensitivities.
	if (win->treeBrowse)
		if (win->treeBrowse->getSelectedNode())
			win->treeBrowse->setEditButtonSensitivities ();

	//  Even though the web apply did not fail, we may still have
	//  a message to put up (though not technically an error).
	if (err = setupObjectErrorStringGet (win->object))
	{
		if (!errDialog)
		{
			doMsgDialog = True;
			errorDialog (win->topLevel, err, win->object);
			return (False);
		}
	}
	return (True);

}	//  End  doApplyActionCB ()



void
doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *)
{
	SetupWin	*win = (SetupWin *)clientData;


	log1 (C_FUNC, "doResetActionCB()");

	if (pwdDialog)
	{
		XtDestroyWidget (pwdDialog);
		pwdDialog = (Widget)0;
		log1 (C_ALL, "\tDestroyed the pwd dialog! pwdDialog = 0");
	}

	if (errDialog)
	{
		XtDestroyWidget (errDialog);
		errDialog = (Widget)0;
		log1 (C_ALL, "\tDestroyed the err dialog! errDialog = 0");
	}

	setupWebReset (win->object);
	resetVarLists (win->objList);

}	//  End  doResetActionCB ()



void
resetVarLists (setupObject_t *object)
{
	setupObject_t	*curObj;


	if (setupObjectType (setupObjectListNext (object,
				   (setupObject_t *)0)) == sot_objectList)
	{
		for (curObj = setupObjectListNext (object,
				(setupObject_t *)0) ; curObj ;
				curObj = setupObjectListNext (object,curObj))
		{
			resetVarLists (curObj);		//  recursive
		}
	}
	else
		resetVarList (object);	//  reset variables in this list

}



void
resetVarList (setupObject_t *object)
{
	setupObject_t	*curObj;
	VarEntry	*cData;
	unsigned long	*longValue = (unsigned long *)0;
	char		buff[256], *buffPtr = buff;


	log1 (C_FUNC, "resetVarList()");
	for (curObj = setupObjectListNext (object, (setupObject_t *)0) ;
			curObj ; curObj = setupObjectListNext (object, curObj))
	{
	    if (setupObjectType (curObj) != sot_var)
	    {
		return;
	    }

	    if (!(cData = (VarEntry *)setupObjectClientData (curObj)))
	    {
		log2 (C_FUNC, setupObjectLabel(curObj),": no cData: returning");
		return;
	    }

	    switch (setupVarType (curObj))
	    {
		case svt_string:
		case svt_integer:
			//  ListOnly means we have an option menu - 
			//  no text field to set.
			if (setupVarListOnly (curObj))
			{
				setOptionMenuValue (cData->var, curObj);
				break;
			}

			//  We have a text field that we need to reset.
			if (setupVarType (curObj) == svt_string)
			{
			    buffPtr = "";
			    getStringValue (curObj, &buffPtr);
			    log2 (C_API,"getStringValue returned ", buffPtr);
			}
			else		//  type svt_integer
			{
				getIntValue (curObj, &buffPtr);
			}

			XmTextFieldSetString (cData->var, buffPtr);
			break;

		case svt_flag:
			if (setupObjectGetValue (curObj, &longValue))
			{
				//  Error getting value
			}

			if (!longValue)
			{
				//  Error: no value retrieved.
			}

			if (*longValue)
			{
			    XtVaSetValues (cData->f_onBtn,  XmNset, True, NULL);
			    XtVaSetValues (cData->f_offBtn, XmNset, False,NULL);
			}
			else
			{
			    XtVaSetValues (cData->f_offBtn, XmNset, True, NULL);
			    XtVaSetValues (cData->f_onBtn,  XmNset, False,NULL);
			}
			break;

		case svt_password:
			log1 (C_PWD, "\tClearing field, so passwdTextCB()");
			XmTextFieldSetString (cData->var, "");

			XtFree (cData->p_1stText);
			XtFree (cData->p_2ndText);
			cData->p_1stText = cData->p_2ndText = NULL;

			pwdChanged = pwdActivate = False;
			log1 (C_PWD, "\tXtFree'd & cleared 1st & 2nd fields");
			log1 (C_PWD, "\tpwdChanged = pwdActivate = False.");
			break;

		default:
			break;

	    }	//  End  switch (setupVarType (curObj))

	}	//  End  for loop (loop thru variable list)

}	//  End  resetVarList ()



void
doCancelActionCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{
	log1 (C_FUNC, "doCancelActionCB()");

	//  Destroy the shell widget, which will cause exitSetupWinCB()
	//  to be called, which does a setupWebReset(), etc.

	if (w)
		shellWidgetDestroy (w, clientData, (XmAnyCallbackStruct*)0);

}	//  End  doCancelActionCB ()



void
doHelpActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *)
{
}	//  End  doHelpActionCB ()
