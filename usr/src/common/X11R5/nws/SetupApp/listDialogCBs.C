#ident	"@(#)listDialogCBs.C	1.3"
//	passwdDialogCBs.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/TextF.h>		//  for XmTextFieldGetString()
#include	<Xm/List.h>		//  for XmListGetSelectedPos()

#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database




//  External variables, functions, etc.

extern Boolean	doOkActionCB  (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void	errorDialog (Widget topLevel, char *errorText, setupObject_t *curObj);
extern void	shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);
extern void	setVariableFocus (setupObject_t *obj);

extern Boolean		pwdChanged;
extern Boolean		pwdActivate;
extern Widget		pwdDialog;
extern Boolean		doingOK;


//  Local variables, functions, etc.

void doListOkActionCB    (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs);
void doListCancelActionCB(Widget w, setupObject_t *curObj, XmAnyCallbackStruct *);
void doListHelpActionCB  (Widget,   setupObject_t *curObj, XmAnyCallbackStruct *);
void singleSelectCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs);
void exitListPopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);





//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doListOkActionCB (Widget w, setupObject_t *curObj,
//						     XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called when the "Ok" button has been pushed in the password
//	dialog popup window.  We compare what the user typed in to this
//	validation popup with what was typed in the first password field.
//	If it doesn't match, we pop up an error dialog, and put the input
//	focus on the first password field, which has been cleared.  If it
//	does match, we set the new password value using the API call.
//
//  RETURN:
//	Nothing.
//

void
doListOkActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	


	log3 (C_FUNC,"doListOkActionCB(): ","selectedItem = ",cData->l_selectedItem);

	if (cData->l_selectedItem)
	{
	      if (cbs->reason == XmCR_DEFAULT_ACTION || cbs->reason == XmCR_ARM)
	      {
		    if (cbs->reason == XmCR_ARM)
			log1 (C_ALL, "\tReason == ARM");
		    else
			log1 (C_ALL, "\tReason == DEFAULT_ACTION");

		    XmTextFieldSetString (cData->var, cData->l_selectedItem);
		    log3 (C_ALL,"\t\"",cData->l_selectedItem,"\" set in text field");
		    XtFree (cData->l_selectedItem);
		    cData->l_selectedItem = (char *)0;
	      }
	      else
		    log2 (C_ALL, "*****Reason = ",cbs->reason);
	}

	shellWidgetDestroy (w,(XtPointer)curObj,(XmAnyCallbackStruct *)0);

	log1 (C_FUNC, "End  doListOkActionCB()");

}	//  End  doListOkActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doListCancelActionCB (Widget w, setupObject_t *curObj,
//						     XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This gets called when the "Cancel" button has been pushed (or the ESC
//	key was pressed) in the password dialog popup window.  We simply call
//	shellWidgetDestroy() which causes the associated XmNdestroyCallback
//	(exitListPopupCB()) for the shell widget of this window to be called.
//	That function is what performs the exit tasks such as clearing the
//	password fields, destroying the mnemInfoRec, freeing memory, etc.
//
//  RETURN:
//	Nothing.
//

void
doListCancelActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
{

	log1 (C_FUNC, "doListCancelActionCB()");

	shellWidgetDestroy (w, (XtPointer)curObj, (XmAnyCallbackStruct *)0);

	log1 (C_FUNC, "End  doListCancelActionCB()");

}	//  End  doListCancelActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void exitListPopupCB (Widget w, XtPointer clientData,
//							XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called (automatically) when XtDestroyWidget() has been called
//	either directly in this setup window (here, using shellWidgetDestroy())
//	or when the XmNdestroyCallback is executed because the window manager
//	executes an XtDestroyWidget() on us (like when the user presses Alt-F4,
//	(or selects the mwm Close button or double-clicks on the window manager
//	button).
//	Here, we destroy the mnemInfoRec, clear the first password text field,
//	free up the memory associated with both password fields, reinitialize
//	those pointers, and clear a couple of flags.
//
//  RETURN:
//	Nothing.
//

void
exitListPopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs)
{
	setupObject_t	*curObj = (setupObject_t *)clientData;
	VarEntry	*cData = (VarEntry *)setupObjectClientData(curObj);


	log1 (C_FUNC, "exitListPopupCB()");
	cData = (VarEntry *)setupObjectClientData (curObj);

//	XtFree (anything??);
	//  Set the input focus back to the variable field.
	setVariableFocus (curObj);
	log1 (C_FUNC, "End  exitListPopupCB()");

}	//  End  exitListPopupCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doListHelpActionCB (Widget, setupObject_t *curObj, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This gets called when the "Help..." button has been pushed in the
//	password dialog popup window, or when the XmNhelpCallback gets
//	activated via the F1 key.  We simply call displayHelp().
//
//  RETURN:
//	Nothing.
//

void
doListHelpActionCB (Widget, setupObject_t *curObj, XmAnyCallbackStruct *)
{
}	//  End  doListHelpActionCB ()

void
singleSelectCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs)
{
	XmListCallbackStruct *lcbs = (XmListCallbackStruct *)cbs;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	char		*selectedItem;



	log1 (C_FUNC, "singleSelectCB()");

	if (cbs->reason == XmCR_SINGLE_SELECT)
	{
		XmStringGetLtoR (lcbs->item, XmFONTLIST_DEFAULT_TAG, &selectedItem);

		if (strcmp (cData->l_selectedItem, selectedItem))
		{
			if (cData->l_selectedItem)
			{
				XtFree (cData->l_selectedItem);
			}

			cData->l_selectedItem = strdup (selectedItem);
			log2 (C_FUNC, "\tselectedItem = ", selectedItem);
		}
		else		//  strings are the same, this is an UNselect.
		{
			log3 (C_FUNC, "\tItem (", selectedItem, ") is UNselected");
			XtFree (cData->l_selectedItem);
			cData->l_selectedItem = (char *)0;
		}

		XtFree (selectedItem);
	}

	log1 (C_FUNC, "End  singleSelectCB()");
}
