#ident	"@(#)browseDialog.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */

//	browseDialog.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/DialogS.h>
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/List.h>
#include	<Xm/PanedW.h>
#include	<Xm/TextF.h>

#include	"MultiPList.h"		//  for tree browser
#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database
#include	"treeBrowse.h"		//  for tree browser
#include	"treeBrowseCBs.h"	//  for tree browser callbacks


//  External variables, functions, etc.

extern void   doBrowseOkActionCB    (Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doBrowseCancelActionCB(Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doBrowseHelpActionCB  (Widget, setupObject_t *curObj, XmAnyCallbackStruct *);
extern void   exitBrowsePopupCB     (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);
extern void   secBrowseNodeOpenedCB (setupObject_t *curObj, void *d_node);
extern void   singleSelectCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs);
extern Widget createActionArea (Widget parent, ActionAreaItem *actions,
			Cardinal numActions, Widget highLevelWid, void* mPtr);
extern void   setActionAreaHeight (Widget actionArea);



//  Local variables, functions, etc.

void	createBrowseDialog (Widget parent, setupObject_t *object);

static Widget	createBrowseControlArea (Widget parent, setupObject_t *object);



/* old
static ActionAreaItem browseActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doBrowseOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doBrowseCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True,doBrowseHelpActionCB,  (XtPointer)0 }
};
*/
static ActionAreaItem browseActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doBrowseOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doBrowseCancelActionCB,(XtPointer)0 }
};
static int	numBrowseButtons = XtNumber (browseActions);


Widget	browseDialog = (Widget)0;



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void createBrowseDialog (Widget parent, setupObject_t *object)
//
//  DESCRIPTION:
//	Create the Browse (Selection-Only) Dialog for selecting a choice.
//
//  RETURN:
//	Nothing.
//

void
createBrowseDialog (Widget parent, setupObject_t *object)
{
	Widget		dialog, form, pane, actionArea;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (object);
	int		i;


	//  We'll create our own simple dialog.
	log1 (C_FUNC, "createBrowseDialog()");
	browseDialog = dialog = XtVaCreatePopupShell (setupObjectLabel(object),
			xmDialogShellWidgetClass,	parent,
			XmNdeleteResponse,		XmDESTROY,
			NULL);

	form = XtVaCreateWidget ("browseForm", xmFormWidgetClass, dialog,
			XmNdialogStyle,		XmDIALOG_FULL_APPLICATION_MODAL,
			NULL);

	//  Catch the XtdestroyWidget(us) event, so we can clean things up.
	XtAddCallback (dialog, XmNdestroyCallback,
			(XtCallbackProc)exitBrowsePopupCB, (XtPointer)object);

	//  Catch the "Help" key (i.e., the F1 key), so we can display help.
//	XtAddCallback (form, XmNhelpCallback, (XtCallbackProc)doBrowseHelpActionCB, (XtPointer)object);

	//  The pane does some managing for us (between the control area and
	//  the action area), and also gives us a separator line.
	pane = XtVaCreateWidget ("pane",
				xmPanedWindowWidgetClass,	form,
				XmNsashWidth,		1,	//  0 = invalid so use 1
				XmNsashHeight,		1,	//  so user won't resize
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				NULL);

	//  Create the control area (main area of the window) of the "dialog".
	(void)createBrowseControlArea (pane, object);

	//  Get a new mnemonic info record pointer. This must be done before
	//  creating anything with mnemonics.
	cData->p_mPtr = NULL;

	//  Create the action area (lower button area of the window) of the "dialog".
	for (i = 0 ; i < numBrowseButtons ; i++)
		browseActions[i].clientData = object;

	actionArea = createActionArea (pane, &browseActions[0],
					numBrowseButtons, form, NULL);
	XtVaSetValues (form, XmNdefaultButton, (Widget)0, NULL);

	XtManageChild (pane);
	turnOffSashTraversal (pane);

	XtManageChild (form);
	XtManageChild (dialog);

	XtPopup (dialog, XtGrabNone);

	//  Get and set the height of the action area.  We want this area to
	//  remain the same height while the control area gets larger, when
	//  the user resizes the window larger.  Don't know if this can be
	//  reliably done before XtPopup().
	setActionAreaHeight (actionArea);

	log1 (C_FUNC, "End  createBrowseDialog()");

}	//  End  createBrowseDialog ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createBrowseControlArea (Widget parent, setupObject_t *object)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//

static Widget
createBrowseControlArea (Widget parent, setupObject_t *object)
{
	Widget		controlArea, browse;
	setupObject_t	*listObj;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (object);


	//  Create the control area of the dialog.  We use a form as the base.
	controlArea = XmCreateForm (parent, "ctrlform", (ArgList)0, (Cardinal)0);

	listObj = setupVarList (object);
	cData->win->treeBrowse = new treeBrowse (app.appContext, controlArea,
					listObj, cData->win, False, 2,7);
	browse = cData->win->treeBrowse->getTopWidget ();
	cData->win->treeBrowse->setSelectCallback ((SelectCallback)
						treeNodeSelectedCB, object);
	cData->win->treeBrowse->setDblClickCallback ((DblClickCallback)
						secBrowseNodeOpenedCB, object);
	cData->win->treeBrowse->setUnselectCallback ((UnselectCallback)
						treeNodeUnSelectedCB, object);

	XtVaSetValues (browse,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			3,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		3,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			3,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNrightOffset,			3,
			NULL);

	XtManageChild (controlArea);

	return (controlArea);

}	//  End  createBrowseControlArea ()
