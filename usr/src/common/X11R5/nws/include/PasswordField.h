#ident	"@(#)PasswordField.h	1.2"
/////////////////////////////////////////////////////////
// PasswordField.h:
/////////////////////////////////////////////////////////
#ifndef PASSWORDFIELD_H
#define PASSWORDFIELD_H

#include <Xm/Xm.h>

class PasswordField  {
    
private:
	Widget			_labelW, _textW;
	char			*_password;

	static void		passwdTextCB (Widget, XtPointer, XtPointer);
	static void		passwdFocusCB (Widget, XtPointer, XtPointer);

	void			passwdTextChanged(XmTextVerifyCallbackStruct *);
	void			clearPasswordField(Widget);

public:											/* PUBLIC member functions */
    
   	PasswordField(); 								/* CTOR for PasswordField */
   	~PasswordField(); 								/* DTOR for PasswordField */
	void		postPasswordField(Widget, XmString);/* Post pwd text field */
	Widget		GetPasswordTextFieldWidget() const { return _textW; }	
	Widget		GetPasswordLabelWidget() const { return _labelW; }	
	char		*GetPassword() const { return _password; };
};

#endif
