#ident	"@(#)browseDialogCBs.C	1.3"
//	browseDialogCBs.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/Text.h>		//  for XmTextFieldSetString()
#include	<Xm/TextF.h>		//  for XmTextFieldGetString()
#include	<Xm/List.h>		//  for XmListGetSelectedPos()

#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database




//  External variables, functions, etc.

extern void errorDialog (Widget topLevel, char *errorText, setupObject_t *curObj);
extern void shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);
extern void setVariableFocus (setupObject_t *obj);

extern setupObject_t	*lastFocused;



//  Local variables, functions, etc.

void doBrowseOkActionCB    (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs);
void doBrowseCancelActionCB(Widget w, setupObject_t *curObj, XmAnyCallbackStruct *);
void doBrowseHelpActionCB  (Widget,   setupObject_t *curObj, XmAnyCallbackStruct *);
void exitBrowsePopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);
void secBrowseNodeOpenedCB (setupObject_t *curObj, void *d_node);





//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doBrowseOkActionCB (Widget w, setupObject_t *curObj,
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
doBrowseOkActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	char		*name;
	void		*node, *nodePtr, *nodeStructPtr;
	


	log3 (C_FUNC, "doBrowseOkActionCB(): ", "selectedItem = ",
							cData->l_selectedItem);
	if (node = cData->win->treeBrowse->getSelectedNode ())
	{
		if (!(nodeIsInternal (node)))
		{
		    if (nodePtr = nodeData (node))
		    {
			if (nodeStructPtr = nodeDataStruct((nodeData_t *)nodePtr))
			{
			    if (name = hostName (nodeStructPtr))
			    {
				XmTextFieldSetString (cData->var, name);
			    }
			}
		    }
		}
	}

	delete (cData->win->treeBrowse);
	shellWidgetDestroy (w, (XtPointer)curObj, (XmAnyCallbackStruct *)0);

	log1 (C_FUNC, "End  doBrowseOkActionCB()");

}	//  End  doBrowseOkActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doBrowseCancelActionCB (Widget w, setupObject_t *curObj,
//						     XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This gets called when the "Cancel" button has been pushed (or the ESC
//	key was pressed) in the password dialog popup window.  We simply call
//	shellWidgetDestroy() which causes the associated XmNdestroyCallback
//	(exitBrowsePopupCB()) for the shell widget of this window to be called.
//	That function is what performs the exit tasks such as clearing the
//	password fields, destroying the mnemInfoRec, freeing memory, etc.
//
//  RETURN:
//	Nothing.
//

void
doBrowseCancelActionCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);

	log1 (C_FUNC, "doBrowseCancelActionCB()");

	delete (cData->win->treeBrowse);
	shellWidgetDestroy (w, (XtPointer)curObj, (XmAnyCallbackStruct *)0);

	log1 (C_FUNC, "End  doBrowseCancelActionCB()");

}	//  End  doBrowseCancelActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void exitBrowsePopupCB (Widget w, XtPointer clientData,
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
exitBrowsePopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs)
{
	setupObject_t	*curObj = (setupObject_t *)clientData;
	VarEntry	*cData = (VarEntry *)setupObjectClientData(curObj);


	log1 (C_FUNC, "exitBrowsePopupCB()");

//	XtFree (anything??);
	lastFocused = (setupObject_t *)0;
	//  Set the input focus back to the variable field.
	setVariableFocus (curObj);
	log1 (C_FUNC, "End  exitBrowsePopupCB()");

}	//  End  exitBrowsePopupCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doBrowseHelpActionCB (Widget, setupObject_t *curObj, XmAnyCallbackStruct *)
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
doBrowseHelpActionCB (Widget, setupObject_t *curObj, XmAnyCallbackStruct *)
{
}	//  End  doBrowseHelpActionCB ()



/* //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	secBrowseNodeOpenedCB (setupObject_t *curObj, void *d_node)
//
//  DESCRIPTION:
//	This is the callback that gets executed when a tree (browser) node
//	gets "opened" (double-clicked) that is in a secondary window popup
//	browser.
//
//  RETURN:
//	Nothing.
//
////////////////////////////////////////////////////////////////////////////// */

void
secBrowseNodeOpenedCB (setupObject_t *curObj, void *node)
{
	VarEntry	*cData = (VarEntry *)setupObjectClientData (curObj);
	char		*description;


	log4 (C_FUNC, "treeNodeOpenedCB(obj=", curObj, ", node=",
							nodeName(node));
	if (cData && cData->win && cData->win->descArea)
	{
	    if (description = nodeDescription (node))
		XmTextSetString (cData->win->descArea, description);
	}

	cData->win->treeBrowse->setEditButtonSensitivities ();

	if (!nodeIsInternal (node))
	{
		doBrowseOkActionCB (cData->win->treeBrowse->getTopWidget(),
					     curObj, (XmAnyCallbackStruct *)0);
	}

}	//  End  secBrowseNodeOpenedCB ()
