#ident	"@(#)ActionButtons.C	1.2"
/*****************************************************************************
 * ActionButtons.C: A actions buttons object for creating push buttons 
 *					in a popup dialog in the action area.
 *****************************************************************************/
#include "ActionButtons.h"

#include <assert.h>
#include <iostream.h>

#include <Xm/Form.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>

/*****************************************************************************
 * The ActionButtons ctor initializes the form that holds buttons 
 *****************************************************************************/
ActionButtons::ActionButtons (Widget parent, int numOfButtons)
{
#ifdef DEBUG
cout << "ctor for ActionButtons" << endl;
#endif
	assert (numOfButtons > 0);
	_numOfButtons = numOfButtons;
	_parent = parent;
	_form = NULL;
}

/*****************************************************************************
 * Dtor for ActionButtons.Destroy created widgets.
 *****************************************************************************/
ActionButtons::~ActionButtons()
{
#ifdef DEBUG
cout << "dtor for ActionButtons" << endl;
#endif
}

/*****************************************************************************
 * Set the attachments based on # of buttons. If you do not want to worry 
 * about how the attachments are set then pass NULLS as arguments, else pass
 * in the fraction base and the increment in terms of size of unit. 
 *****************************************************************************/
void
ActionButtons::SetAttachments(int fb, int increment)
{
	int				i, position;
	int				fractionBase;

	/* Make sure _numOfButtons is never zero
	 */
	assert (_numOfButtons > 0);

	/* If they did not pass it in use the default as unit for size between
	 * buttons.  If there was no fraction base then ignore any increment.
	 * Use default increment. If there no increment use 1 as the unit of size.
	 */
	if (!fb) {
		fractionBase = (_numOfButtons * 2) + 1;
		increment = 1;
	}
	else 
		fractionBase = fb;

	if (!increment)
		increment = 1;

	/* Determine the postion to start the first of the buttons
	 */
	position = (fractionBase  - ((_numOfButtons + 1) * increment)) /
				_numOfButtons;

	/* Create the form for the lower area to hold the action buttons
	 */
	_form = XtVaCreateWidget ("actionform", xmFormWidgetClass, _parent,
								XmNfractionBase, fractionBase,
								NULL);

	/* Create the action push buttons in a loop here.
	 */
	for (i = 0; i < _numOfButtons; i++)  {
    	_actionbuttons[i] = XtVaCreateManagedWidget ("actionbuttons", 
                            xmPushButtonWidgetClass, _form, 
                            XmNbottomAttachment, 		XmATTACH_FORM,
                            XmNtopAttachment, 			XmATTACH_FORM,
                            XmNleftAttachment, 			XmATTACH_POSITION,
                            XmNleftPosition, 			position,
                            XmNrightAttachment, 		XmATTACH_POSITION,
                            XmNrightPosition, 			position + increment,
                            XmNshowAsDefault, 			False,
                            XmNdefaultButtonShadowThickness,1,
                            NULL);
		position += (increment * 2);
	}

	XtManageChild (_form);

	/* Translate return key to activate callback, since space is key activate
	 * by default in Motif.
	 */
	ActivateKeyReturn();
}

/*****************************************************************************
 * Activate the key return button. Space is the key for activating the push
 * button in Motif.
 *****************************************************************************/
void
ActionButtons::ActivateKeyReturn ()
{
	int		i;
	XtTranslations	trans_table;

	/* Space is otherwise the key for activation
	 */
	trans_table = XtParseTranslationTable ("<Key>Return: ArmAndActivate()");

	for (i = 0; i < _numOfButtons; i++)  
		XtOverrideTranslations (_actionbuttons[i], trans_table);
}

/******************************************************************************
 * Set the cancel button of the form that is passed in, so that
 * escape works like cancel. 
 ******************************************************************************/
void ActionButtons::SetCancelButton (Widget widget, int cancelButton)
{
	XtVaSetValues (widget, XmNcancelButton, _actionbuttons[cancelButton], NULL);
}

/******************************************************************************
 * Set the default button so that when activate occurs, the
 * button action is invoked.
 ******************************************************************************/
void ActionButtons::SetDefaultButton (Widget widget, int defaultButton)
{
	XtVaSetValues (widget, XmNdefaultButton,_actionbuttons[defaultButton],NULL);
}

/******************************************************************************
 * Register callbacks and client data for the buttons.
 ******************************************************************************/
void ActionButtons::registerButtonCallback (int button, XtCallbackProc func,
											XtPointer client_data)
{
		XtAddCallback (_actionbuttons[button], XmNactivateCallback,	func, 
                       client_data);		
}

/******************************************************************************
 * Set the button labels, by creating compound strings.
 ******************************************************************************/
void ActionButtons::SetButtonLabels (char **labels)
{
	XmString		xmstr;
	int				i;

	for (i = 0; i < _numOfButtons; i++)  {
		xmstr = XmStringCreateLocalized(labels[i]);
		XtVaSetValues (_actionbuttons[i], XmNlabelString, xmstr, NULL);
		XmStringFree (xmstr);
	}
}

/******************************************************************************
 * Set the mnemonics, register them using registerMnemInfo and
 * then register them on the shell widget as well.
 ******************************************************************************/
void ActionButtons::SetButtonMnemonics (char **mnem)
{
	int			i;

	for (i = 0; i < _numOfButtons; i++)  {
/* tony
		registerMnemInfo (_actionbuttons[i], (char *)mnem[i], MNE_ACTIVATE_BTN 
                         | MNE_GET_FOCUS);
*/
	}

	/* Register mnemonics information on the shell
	 */
	while (!XtIsShell(_parent))
		_parent = XtParent(_parent);
/* tony
	registerMnemInfoOnShell(_parent);
*/
}

/******************************************************************************
 * Show the button passed as a default buttons(with highlight)
 ******************************************************************************/
void ActionButtons::SetShowAsDefault(int defaultButton)
{
	XtVaSetValues(_actionbuttons[defaultButton], XmNshowAsDefault, True, NULL);
}

/******************************************************************************
 * Set the button to be insensitive.
 ******************************************************************************/
void
ActionButtons::DeSensitize(int button)
{
	XtSetSensitive(_actionbuttons[button], False);
}
 
/******************************************************************************
 * Set the button to be sensitive.
 ******************************************************************************/
void
ActionButtons::Sensitize(int button)
{
	XtSetSensitive(_actionbuttons[button], True);
}

/******************************************************************************
 * Connect close of window manager to the callback function that is passed.
 ******************************************************************************/
Boolean
ActionButtons::ConnectCloseToCancel(Widget dialogw, XtCallbackProc func, 
									XtPointer userdata)
{
	Atom		deletewin;

	if (!func)
		return False;
	deletewin = XmInternAtom (XtDisplay(dialogw), "WM_DELETE_WINDOW", False);	
	XmAddWMProtocols (dialogw, &deletewin, 1);
	XmAddWMProtocolCallback (dialogw, deletewin, (XtCallbackProc) func, 
                             (XtPointer) userdata);
	return (True);
}

/******************************************************************************
 * Set the default button thickness 
 ******************************************************************************/
void ActionButtons::SetShadowThickness(int defaultButton, int shadowthickness)
{
		XtVaSetValues (_actionbuttons[defaultButton], 
                       XmNdefaultButtonShadowThickness, 	shadowthickness, 
                       NULL);
}
