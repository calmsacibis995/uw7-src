#ident	"@(#)errorDialog.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */

//  errorDialog.C


#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/MessageB.h>		//  for the error dialog
#include	<Xm/TextF.h>		//  for XmTextFieldSetString()

#include	"dtFuncs.h"		//  for HelpText, GUI lib funcs, etc.
#include	"controlArea.h"
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database
#include	"variables.h"		//  for variable supporting functions



//  External functions, variables, etc.

extern void    doCancelActionCB (Widget w, XtPointer clientData,
							XmAnyCallbackStruct *);


//  Local functions, variables, etc.

void		errorDialog (Widget topLevel, char *errorText, setupObject_t *curObj);
static void	doErrOkActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *);

Widget	errDialog = (Widget)0;
Boolean	doMsgDialog = False;





void
errorDialog (Widget topLevel, char *errorText, setupObject_t *curObj)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	Widget		dialog;
	Arg		args[6];
	Cardinal	argc = (Cardinal)0;
	XmString	title, message, ok;


	log1 (C_FUNC, "errorDialog(topLevel, errorText, varInError)");
	title   = XmStringCreateLocalized (cData->win->title);

	if (!title)
		title = XmStringCreateLocalized (getStr (TXT_appNoName));

	message = XmStringCreateLtoR (getStr (errorText), XmSTRING_DEFAULT_CHARSET);
	ok      = XmStringCreateLocalized (getStr (TXT_OkButton));

	if (errorText)
	{
		XtSetArg (args[argc],	XmNautoUnmanage,	False);    argc++;
		XtSetArg (args[argc],	XmNdialogTitle,		title);    argc++;
		XtSetArg (args[argc],	XmNmessageString,	message);  argc++;
		XtSetArg (args[argc],	XmNcancelLabelString,	ok);       argc++;
		XtSetArg (args[argc],	XmNdeleteResponse,	XmDESTROY);argc++;
		XtSetArg (args[argc],	XmNdialogStyle,
					  XmDIALOG_PRIMARY_APPLICATION_MODAL); argc++;

		if (doMsgDialog)
		{
			errDialog = dialog = XmCreateMessageDialog (topLevel,
						"msgDialog", args, argc);
		}
		else
		{
			errDialog = dialog = XmCreateErrorDialog (topLevel,
						"errorDialog", args, argc);
		}

		XtAddCallback (dialog, XmNdestroyCallback,
				(XtCallbackProc)doErrOkActionCB, (XtPointer)curObj);

		XtAddCallback (dialog, XmNcancelCallback, (XtCallbackProc)doErrOkActionCB,
						(XtPointer)curObj);

		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
//		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));

		{
		Widget _helpWidget = XmMessageBoxGetChild (dialog,
			XmDIALOG_HELP_BUTTON);
		XtUnmanageChild(_helpWidget);
		}

		XtManageChild (dialog);

		XtPopup (XtParent (dialog), XtGrabNone);
	}

	XmStringFree (ok);
	XmStringFree (message);
	XmStringFree (title);
	log1 (C_FUNC, "End  errorDialog()");

}	//  End  errorDialog ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void  doErrOkActionCB (Widget w, setupObject_t *curObj,
//							XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	If a curObj was passed in, we had an error while trying to set a
//	variable through the setup API.	In that case, we reset the variable
//	value back to the last correct value that was set and set the focus
//	to the variable that was in error.  In all cases, we then dismiss
//	the error dialog and reset the errDialog flag.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

static void
doErrOkActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
{
	VarEntry	*cData;
	unsigned long	*longValue = (unsigned long *)0;
	char            buff[256], *buffPtr, *format = (char *)0;


	log1 (C_FUNC, "doErrOkActionCB(w, curObj, XmAnyCallbackStruct *)");

	if (curObj)	//  We have an invalid variable value (as opposed to
	{		//  an error like the user doesn't have permission).
	    if (cData = (VarEntry *)setupObjectClientData (curObj))
	    {

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
			else
			{
			    if (!(setupObjectGetValue (curObj, &longValue)))
			    {
				if (longValue)
				{
				    buffPtr = buff;

				    if (format = setupObjectGetFormat (curObj))
					sprintf (buff, format, *longValue);
				    else
					sprintf (buff, "%u", *longValue);
				}
			    }
			}

			XmTextFieldSetString (cData->var, buffPtr);
			break;

		    case svt_flag:
		    case svt_password:
		    default:
			break;
		}

		log1 (C_ALL, "\tSetting focus to variable in error");

		//  Set focus to the variable in error.
		(void)XmProcessTraversal (cData->var, XmTRAVERSE_CURRENT);
	    }
	}

	XtPopdown (XtParent (w));
	log1 (C_ALL, "\tError dialog popped down, errDialog = 0");

	errDialog = (Widget)0;

	if (doMsgDialog)
	{
		doCancelActionCB (cData->win->topLevel, cData->win,
						(XmAnyCallbackStruct *)0);
		doMsgDialog = False;
	}

	log1 (C_FUNC, "End  doErrOkActionCB()");

}	//  End  doErrOkActionCB ()
