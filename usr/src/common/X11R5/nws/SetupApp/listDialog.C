#ident	"@(#)listDialog.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */

//	listDialog.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/DialogS.h>
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/List.h>
#include	<Xm/PanedW.h>
#include	<Xm/TextF.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database


//  External variables, functions, etc.

extern void   doListOkActionCB    (Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doListCancelActionCB(Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doListHelpActionCB  (Widget, setupObject_t *curObj, XmAnyCallbackStruct *);
extern void   exitListPopupCB     (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);
extern void   singleSelectCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *cbs);

extern Widget createActionArea (Widget parent, ActionAreaItem *actions,
			Cardinal numActions, Widget highLevelWid, void* mPtr);
extern void   setActionAreaHeight (Widget actionArea);



//  Local variables, functions, etc.

void	createListDialog (Widget parent, setupObject_t *object);

static Widget	createListControlArea (Widget parent, setupObject_t *object);
static Widget	createList (Widget parent, setupObject_t *object);
void   populateList (Widget list, setupObject_t *object);


/* old
static ActionAreaItem listActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doListOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doListCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True,doListHelpActionCB,  (XtPointer)0 }
};
*/
static ActionAreaItem listActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doListOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doListCancelActionCB,(XtPointer)0 }
};
static int	numListButtons = XtNumber (listActions);


Widget	listDialog = (Widget)0;



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void createListDialog (Widget parent, setupObject_t *object)
//
//  DESCRIPTION:
//	Create the List (Selection-Only) Dialog for selecting a choice.
//
//  RETURN:
//	Nothing.
//

void
createListDialog (Widget parent, setupObject_t *object)
{
	Widget		dialog, form, pane, actionArea;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (object);
	int		i;


	//  We'll create our own simple dialog.
	log1 (C_FUNC, "createListDialog()");
	listDialog = dialog = XtVaCreatePopupShell (setupObjectLabel(object),
			xmDialogShellWidgetClass,	parent,
			XmNdeleteResponse,		XmDESTROY,
			NULL);

	form = XtVaCreateWidget ("listForm", xmFormWidgetClass, dialog,
			XmNdialogStyle,		XmDIALOG_FULL_APPLICATION_MODAL,
			NULL);

	//  Catch the XtdestroyWidget(us) event, so we can clean things up.
	XtAddCallback (dialog, XmNdestroyCallback,
			(XtCallbackProc)exitListPopupCB, (XtPointer)object);

	//  Catch the "Help" key (i.e., the F1 key), so we can display help.
//	XtAddCallback (form, XmNhelpCallback, (XtCallbackProc)doListHelpActionCB, (XtPointer)object);

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
	(void)createListControlArea (pane, object);

	//  Get a new mnemonic info record pointer. This must be done before
	//  creating anything with mnemonics.
	//  Create the action area (lower button area of the window) of the "dialog".
	for (i = 0 ; i < numListButtons ; i++)
		listActions[i].clientData = object;

	actionArea = createActionArea (pane, &listActions[0], numListButtons,
							form, NULL);
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

	log1 (C_FUNC, "End  createListDialog()");

}	//  End  createListDialog ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createListControlArea (Widget parent, setupObject_t *object)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//

static Widget
createListControlArea (Widget parent, setupObject_t *object)
{
	Widget	controlArea, list;
	XtWidgetGeometry returnGeo;


	//  Create the control area of the dialog.  We use a form as the base.
	controlArea = XmCreateForm (parent, "ctrlform", (ArgList)0, (Cardinal)0);

	list = createList (controlArea, object);
	populateList (list, object);

	//  Get the height of the textfield to make the label the same height.
	XtQueryGeometry (list, (XtWidgetGeometry *)0, &returnGeo);

	XtVaSetValues (XtParent (list),
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

}	//  End  createListControlArea ()



static Widget
createList (Widget parent, setupObject_t *object)
{
	Widget	var;
	Arg	args[5];
	Cardinal argc = 0;


	XtSetArg (args[argc],  XmNvisibleItemCount,	7);		argc++;
	XtSetArg (args[argc],  XmNselectionPolicy,    XmSINGLE_SELECT);	argc++;


	var = XmCreateScrolledList (parent,  "scrolledL",  args,  argc);

	XtManageChild (var);

	XtAddCallback (var, XmNdefaultActionCallback,
			   (XtCallbackProc)doListOkActionCB, (XtPointer)object);

	XtAddCallback (var, XmNsingleSelectionCallback,
			   (XtCallbackProc)singleSelectCB, (XtPointer)object);

	return (var);

}	//  End  createList ()

void
populateList (Widget list, setupObject_t *object)
{
	setupObject_t	*listObj = setupVarList (object);
	setupChoice_t	*choice;
	int		i;


	for (i = 0, choice = setupChoiceListChoice (listObj,i) ;
		choice ; i++, choice=setupChoiceListChoice(listObj,i))
	{
		if (listObj)
		{
			XmListAddItemUnselected (list, XmStringCreateLocalized
				(setupChoiceLabel (choice)), 0);
		}
	}

}	//  End  populateList ()
