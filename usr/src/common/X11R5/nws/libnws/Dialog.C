#ident	"@(#)Dialog.C	1.4"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Removal of help buttons
 */
////////////////////////////////////////////////////////////////////
// Dialog.C: A dialog object for invoking motif dialogs 
/////////////////////////////////////////////////////////////////////
#include "Dialog.h"

#include <iostream.h>
#include <assert.h>

#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Protocols.h>

/*****************************************************************************
 *	The Dialog ctor initializes the name and type variable
 *****************************************************************************/
Dialog::Dialog (char *name, int type)
{
	assert (name != 0);
	_name = new char[strlen (name) + 1];
	_type = type;
	_dialogw = NULL;
	_setMnemonic = False;
	_canceluserdata = 0;
	_cancelfunc = 0;
#ifdef DEBUG
	cout << "ctor for dialog" << endl;
#endif
}

/*****************************************************************************
 *	The Dialog dtor de-initializes the name and type variable
 *****************************************************************************/
Dialog::~Dialog () 
{
	delete _name;
	_type = 0;
	_canceluserdata = 0;
	_cancelfunc = 0;
	_setMnemonic = False;
	XtDestroyWidget (_dialogw);
#ifdef DEBUG
	cout << "dtor for dialog" << endl;
#endif
}

/*****************************************************************************
 *	The postDialog functions posts the dialog based on the type 
 *****************************************************************************/
void Dialog::postDialog (Widget parent, char *title, char *msg) 
{
	int			ac;
	Arg			al[20];

	ac = 0;
	XtSetArg (al[ac],XmNtitle, title); ac++;
	XtSetArg (al[ac],XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac],XmNnoResize, True); ac++;

	switch (_type) {
	
		case ERROR: 	
			_dialogw = XmCreateErrorDialog (parent, _name, al, ac);
			break;

		case INFORMATION: 	
			_dialogw = XmCreateInformationDialog (parent, _name, al, ac);
			break;

		case QUESTION: 	
			_dialogw = XmCreateQuestionDialog (parent, _name, al, ac);
			break;

		case WORKING: 	
			_dialogw = XmCreateWorkingDialog (parent, _name, al, ac);
			break;

		case WARNING: 	
			_dialogw = XmCreateWarningDialog (parent, _name, al, ac);
			break;
		case PROMPT: 	
			_dialogw = XmCreatePromptDialog (parent, _name, al, ac);
			break;
	}

	switch (_type) {
	
		case ERROR:
		case INFORMATION: 	
		case QUESTION: 	
		case WORKING: 	
		case WARNING: 	
			_okWidget = XmMessageBoxGetChild (_dialogw,
				XmDIALOG_OK_BUTTON); 
			_cancelWidget = XmMessageBoxGetChild (_dialogw,
				XmDIALOG_CANCEL_BUTTON); 
			_helpWidget = XmMessageBoxGetChild (_dialogw, 
				XmDIALOG_HELP_BUTTON); 
			XtUnmanageChild(_helpWidget);
			break;
		case PROMPT:
			_okWidget = XmSelectionBoxGetChild (_dialogw,
				XmDIALOG_OK_BUTTON); 
			_cancelWidget = XmSelectionBoxGetChild (_dialogw,
				XmDIALOG_CANCEL_BUTTON);
//			_helpWidget = XmSelectionBoxGetChild (_dialogw,
//				XmDIALOG_HELP_BUTTON);
			break;
	}

	if (_type != PROMPT && _type != WORKING)  {
		if (msg)
			XtVaSetValues (_dialogw, XmNmessageString, XmStringCreateLocalized
							(msg), 0);
	}
	else if (_type == PROMPT)  {
		if (msg)
			XtVaSetValues (_dialogw, XmNtextString, XmStringCreateLocalized
							(msg), 0);
	}
}

/*****************************************************************************
 * Register the CANCEL callback method here. 
 *****************************************************************************/
void Dialog::registerCancelCallback (XtCallbackProc func, XtPointer userdata) 
{
	_cancelfunc = func;
	_canceluserdata = userdata; 
	XtAddCallback (_dialogw, XmNcancelCallback, func, userdata);
}

/*****************************************************************************
 * Register the Help callback method here. 
 *****************************************************************************/
void Dialog::registerHelpCallback (XtCallbackProc func, XtPointer userdata) 
{
//	XtRemoveAllCallbacks( _helpWidget, XmNactivateCallback) ;
//	XtAddCallback (_dialogw, XmNhelpCallback, func, userdata);
//	XtAddCallback (_helpWidget, XmNactivateCallback, func, userdata);
}

/*****************************************************************************
 * Register the OK callback method here. 
 *****************************************************************************/
void Dialog::registerOkCallback (XtCallbackProc func, XtPointer userdata) 
{
	XtAddCallback (_dialogw, XmNokCallback, func, userdata);
}

/*****************************************************************************
 * Unmanage the OK widget 
 *****************************************************************************/
void Dialog::unmanageOk ()
{
	XtUnmanageChild (_okWidget); 
}

/*****************************************************************************
 * Unmanage the Cancel widget 
 *****************************************************************************/
void Dialog::unmanageCancel () 
{
	XtUnmanageChild (_cancelWidget); 
}

/*****************************************************************************
 * Unmanage the help widget 
 *****************************************************************************/
void Dialog::unmanageHelp () 
{
//	XtUnmanageChild (_helpWidget); 
}

/*****************************************************************************
 * Set the selection label string here.
 *****************************************************************************/
void Dialog::setSelectionLabelString (char *string) 
{
	if (_type == PROMPT)
		XtVaSetValues (_dialogw, 
					XmNselectionLabelString, XmStringCreateLocalized (string), 
					0); 
}

/*****************************************************************************
 * Set the OK label string here.
 *****************************************************************************/
void Dialog::setOkString (char *okstring, char *mnem) 
{

	XtVaSetValues (_dialogw,
			XmNokLabelString, XmStringCreateLocalized(okstring), 
			0); 
	if ( mnem != NULL )
	{
		_setMnemonic = True;
/* tony
		registerMnemInfo(_okWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
*/
	}
}

/*****************************************************************************
 * Set the Help label string here.
 *****************************************************************************/
void Dialog::setHelpString (char *helpstring, char *mnem) 
{
//	XtVaSetValues (_dialogw,
//			XmNhelpLabelString,XmStringCreateLocalized(helpstring), 
//			0); 
//	if ( mnem != NULL )
//	{
//		_setMnemonic = True;
/* tony
		registerMnemInfo(_helpWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
*/
//	}
}

void Dialog::setCancelString (char *cancelstring, char *mnem) 
{
	XtVaSetValues (_dialogw,
			XmNcancelLabelString,XmStringCreateLocalized(cancelstring), 
			0);
	if ( mnem != NULL )
	{
		_setMnemonic = True;
/* tony
		registerMnemInfo(_cancelWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
*/
	}
}

/*****************************************************************
		manage function  - manage the widget
*****************************************************************/
void Dialog::manage()
{
/* tony
	if (_setMnemonic)
		registerMnemInfoOnShell(_dialogw);
*/
    XtManageChild ( _dialogw );
}

/*****************************************************************
	Set modality	
*****************************************************************/
void Dialog::SetModality(unsigned char modality)
{
	XtVaSetValues (_dialogw, XmNdialogStyle, modality, 0);
}

/**************************************************************
 * Set the sensitivity of the buttons here.
 **************************************************************/
void Dialog::SetSensitive(int button, Boolean flag)
{
	switch (button) {
		case OK:	XtSetSensitive (_okWidget, flag); 
					break;
		case CANCEL:XtSetSensitive (_cancelWidget, flag); 
					break;
//		case HELP:	XtSetSensitive (_helpWidget, flag); 
//					break;
	}
}

/**************************************************************
 * COnnect the cancel callback to the closing of the window.
 **************************************************************/
Boolean Dialog::ConnectCloseWindow ()
{
	Atom		deletewin;

	if (!_cancelfunc)
		return False;
	deletewin = XmInternAtom (XtDisplay (_dialogw), "WM_DELETE_WINDOW", False);	
	XmAddWMProtocols (_dialogw, &deletewin, 1);
	XmAddWMProtocolCallback (_dialogw, deletewin, (XtCallbackProc)
							_cancelfunc, (XtPointer) _canceluserdata);
	return True;
}
