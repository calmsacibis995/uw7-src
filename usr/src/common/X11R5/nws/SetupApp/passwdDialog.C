#ident	"@(#)passwdDialog.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons 
 */
//	passwdDialog.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/DialogS.h>
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/PanedW.h>
#include	<Xm/TextF.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"setupWin.h"		//  for SetupWin
#include	"setup_txt.h"		//  the localized message database



//  External variables, functions, etc.

extern void   doPwdOkActionCB    (Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doPwdCancelActionCB(Widget w, setupObject_t *curObj,XmAnyCallbackStruct *);
extern void   doPwdHelpActionCB  (Widget, setupObject_t *curObj, XmAnyCallbackStruct *);
extern void   exitPwdPopupCB     (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);

extern Widget createActionArea (Widget parent, ActionAreaItem *actions,
			Cardinal numActions, Widget highLevelWid, void* mPtr);
extern void   setActionAreaHeight (Widget actionArea);
extern void   passwdTextCB (Widget, char **password, XmTextVerifyCallbackStruct *cbs);



//  Local variables, functions, etc.

void	createPasswdDialog (Widget parent, setupObject_t *pwdObj);

static Widget	createPwdControlArea (Widget parent, setupObject_t *pwdObj);
static Widget	createPwdTextField (Widget parent, setupObject_t *pwdObj);
static Widget	createPwdLabel (Widget parent, Dimension height, setupObject_t *pwdObj);


/* old
static ActionAreaItem pwdActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doPwdOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doPwdCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True,doPwdHelpActionCB,  (XtPointer)0 }
};
*/
static ActionAreaItem pwdActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doPwdOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doPwdCancelActionCB,(XtPointer)0 }
};
static int	numPwdButtons = XtNumber (pwdActions);


Widget	pwdDialog = (Widget)0;



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void createPasswdDialog (Widget parent, setupObject_t *pwdObj)
//
//  DESCRIPTION:
//	Create the Password Dialog for entering the password again.
//
//  RETURN:
//	Nothing.
//

void
createPasswdDialog (Widget parent, setupObject_t *pwdObj)
{
	Widget		dialog, form, pane, actionArea;
	VarEntry	*cData = (VarEntry *)setupObjectClientData (pwdObj);
	int		i;


	//  We'll create our own simple dialog.
	log1 (C_FUNC, "createPasswdDialog()");
	pwdDialog = dialog = XtVaCreatePopupShell ("MHS Mail Setup Password Validation",
				xmDialogShellWidgetClass, parent,
				XmNdeleteResponse,	XmDESTROY,
				NULL);

	form = XtVaCreateWidget ("pwdForm", xmFormWidgetClass, dialog,
				XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
				NULL);

	//  Catch the XtdestroyWidget(us) event, so we can clean things up.
	XtAddCallback (dialog, XmNdestroyCallback,
			(XtCallbackProc)exitPwdPopupCB, (XtPointer)pwdObj);

	//  Catch the "Help" key (i.e., the F1 key), so we can display help.
//	XtAddCallback (form, XmNhelpCallback, (XtCallbackProc)doPwdHelpActionCB,
//							(XtPointer)pwdObj);

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
	(void)createPwdControlArea (pane, pwdObj);

	//  Create the action area (lower button area of the window) of the "dialog".
	for (i = 0 ; i < numPwdButtons ; i++)
		pwdActions[i].clientData = pwdObj;

	actionArea = createActionArea (pane, &pwdActions[0], numPwdButtons,
							form, NULL);

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

	log1 (C_FUNC, "End  createPasswdDialog()");

}	//  End  createPasswdDialog ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createPwdControlArea (Widget parent, setupObject_t *pwdObj)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//

static Widget
createPwdControlArea (Widget parent, setupObject_t *pwdObj)
{
	Widget	controlArea, label, pwdTxt;
	XtWidgetGeometry returnGeo;


	//  Create the control area of the dialog.  We use a form as the base.
	controlArea = XmCreateForm (parent, "ctrlform", (ArgList)0, (Cardinal)0);

	pwdTxt = createPwdTextField (controlArea, pwdObj);

	//  Get the height of the textfield to make the label the same height.
	XtQueryGeometry (pwdTxt, (XtWidgetGeometry *)0, &returnGeo);

	label = createPwdLabel (controlArea, returnGeo.height, pwdObj);

	XtVaSetValues (label,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			3,	// : 6,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			5,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		6,
			NULL);

	XtVaSetValues (pwdTxt,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			3,	// : 6,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		6,
			XmNleftAttachment,		XmATTACH_WIDGET,
			XmNleftWidget,			label,
			XmNleftOffset,			3,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNrightOffset,			5,
			NULL);

	XtManageChild (controlArea);

	return (controlArea);

}	//  End  createPwdControlArea ()



static Widget
createPwdLabel (Widget parent, Dimension height, setupObject_t *pwdObj)
{
	Widget		label;


	label = XtVaCreateManagedWidget ("label",
			xmLabelWidgetClass,		parent,
			XmNheight,			height,
			XmNalignment,			XmALIGNMENT_END,
			XmNrecomputeSize,		False,
			XmNlabelString,
			    XmStringCreateLocalized (setupObjectLabel(pwdObj)),
			NULL);

	return (label);

}	//  End  createPwdLabel ()


static Widget
createPwdTextField (Widget parent, setupObject_t *pwdObj)
{
	Widget	var;
	Pixel	bg;
	VarEntry	*cData;


	var = XtVaCreateManagedWidget ("textF",
			xmTextFieldWidgetClass,		parent,
			NULL);

	XtVaGetValues (var, XtNbackground, &bg, NULL);
	XtVaSetValues (var, XtNforeground,  bg, NULL);

	cData = (VarEntry *)setupObjectClientData (pwdObj);

	XtAddCallback (var, XmNactivateCallback, (XtCallbackProc)doPwdOkActionCB,
						(XtPointer)pwdObj);

	XtAddCallback (var, XmNmodifyVerifyCallback, (XtCallbackProc)passwdTextCB,
						(XtPointer)(&(cData->p_2ndText)));

	return (var);

}	//  End  createPwdTextField ()
