#ident	"@(#)PasswordField.C	1.2"
#include "PasswordField.h"
#include "misc.h"
#include <iostream.h>

#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>

/******************************************************************************
 * Constructor for the PasswordField class.  
 *****************************************************************************/
PasswordField::PasswordField()
{
#ifdef DEBUG
cout << "ctor for PasswordField" << endl;
#endif
	_password = NULL;
}

/******************************************************************************
 * Destructor for the PasswordField class.
 *****************************************************************************/
PasswordField::~PasswordField()
{
#ifdef DEBUG
cout << "dtor for PasswordField" << endl;
#endif
	if (_password)
		delete _password;
	_password = NULL;
}

/******************************************************************************
 * Create the Password label and text fields. 
 *****************************************************************************/
void
PasswordField::postPasswordField(Widget parent, XmString pwdlabel)
{
	unsigned long	bg;

	/* Create the password label field. 
	 */
	_labelW = XtVaCreateManagedWidget("PwdLabels", xmLabelWidgetClass, parent,
									XmNlabelString,		pwdlabel,
									NULL);

	/* Create the password text field. 
	 */
	_textW = XtVaCreateManagedWidget("PwdText", 	/* Text field */
								xmTextFieldWidgetClass, 	parent, 
								XmNverifyBell,				False,
								XmNcursorPositionVisible,	False,
								NULL);

	/* Add callbacks for changing each character and for focus on text field
	 */
	XtAddCallback (_textW, XmNmodifyVerifyCallback, passwdTextCB, this);
	XtAddCallback (_textW, XmNfocusCallback, passwdFocusCB, this);

	/* Blank out the password field. Set foreground to background color
	 */
	XtVaGetValues (_textW, XtNbackground, &bg, NULL);
	XtVaSetValues (_textW, XtNforeground, bg, NULL);
}

/*******************************************************************************
 * Password text field modify CB. Save each pwd character into a private member 
 * handling backspaces and paste operations.  Maximum  one character at a time.
 ******************************************************************************/
void
PasswordField::passwdTextCB(Widget, XtPointer client_data, XtPointer call_data )
{
    XmTextVerifyCallbackStruct * cbs =(XmTextVerifyCallbackStruct *) call_data;
	PasswordField	*obj = (PasswordField *) client_data;
	
	obj->passwdTextChanged(cbs);
}

/******************************************************************************
 * Password text field has been modified. Member function called from the
 * static void callback function above. 
 *****************************************************************************/
void
PasswordField::passwdTextChanged(XmTextVerifyCallbackStruct *cbs)
{
	char		*newStr;

	/* Handle the backspace character case
	 */
    if (cbs->text->ptr == NULL || cbs->text->length == 0) {
        if (strlen(_password) <= 0) {
        	cbs->endPos = cbs->startPos = 0;
			return; 
		}
        cbs->endPos = strlen(_password); /* delete from here to end */
        _password[cbs->startPos] = 0;    /* backspace--terminate */
        return;
    }

	/* Don't allow "paste" operations make the user *type* the password!
	 */
    if (cbs->text->length > 1) {
        cbs->doit = False; 
        return; 
    }

	/* Save new passwd char into local buffer allocating more space each time.
	 */
    newStr = XtMalloc((unsigned int)cbs->endPos + 2 ); /*new char + NULL end */
    if (_password) {
       	strncpy(newStr, _password,(unsigned int)cbs->endPos);
       	newStr[cbs->endPos] = NULL;
       	XtFree(_password);
    } 
	else
       	newStr[0] = NULL;
    _password = newStr;
    strncat(_password, cbs->text->ptr, 1);
   	_password[cbs->endPos + 1] = 0;

	/* Change text to a '*' incase someone wants to cut password from the field 
	 */
	cbs->text->ptr[0] = '*';
	cout << cbs->text->ptr[0] << endl;

	cout << _password << endl;
}

/*****************************************************************
 * Clear the password field on focus accept
 *****************************************************************/
void 
PasswordField::passwdFocusCB (Widget w, XtPointer client_data, XtPointer ) 
{
	PasswordField	*obj = (PasswordField *) client_data;

	obj->clearPasswordField(w);
}

/******************************************************************************
 * Clear the password field when input focus is received. 
 *****************************************************************************/
void
PasswordField::clearPasswordField(Widget w)
{
	if (_password) 
		_password[0] = NULL;	
	XtVaSetValues (w, XmNvalue, (XtArgVal)NULL, NULL);
}
