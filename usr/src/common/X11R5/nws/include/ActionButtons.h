#ident	"@(#)ActionButtons.h	1.3"
/*****************************************************************************
 * ActionButtons.h:
 *****************************************************************************/
#ifndef ACTIONBUTTONS_H 
#define  ACTIONBUTTONS_H
#include <Xm/Xm.h>

class ActionButtons  {
    
public:						/* CTOR & DTOR for ActionButtons */
   	ActionButtons (Widget parent, int numOfButtons);
   	~ActionButtons ();

private:					/* Private data */
	Widget		_form;
	Widget		_parent;
	int			_numOfButtons;

private:					/* private methods */
	void		ActivateKeyReturn();

protected:					/* protected data for inheritance; */
	virtual enum {MAXBUTTONS = 10};
	Widget		_actionbuttons[MAXBUTTONS];

public:
	/* Manage the widget and register mnemonic info on shell 
	 */
	virtual void	SetAttachments(int fractionBase = 0, int increments = 0);
	virtual void	SetButtonMnemonics(char **mnemonics);
	virtual void	SetButtonLabels(char **labels);

	/* Register callbacks for action buttons. Pass client data.
	 */
	virtual void 	registerButtonCallback (int, XtCallbackProc, XtPointer);

	/* Set Button Resources
	 */
	virtual void 	SetCancelButton(Widget form, int buttonNum);
	virtual void 	SetDefaultButton(Widget form, int buttonNum);
	virtual void 	SetShowAsDefault(int buttonNum);
	virtual void 	SetShadowThickness(int buttonNum, int thickness);
	virtual void 	Sensitize(int buttonNum);
	virtual void 	DeSensitize(int buttonNum);
	Widget			ActionAreaWidget () const { return _form; }
	Widget			GetActionButtonWidget(int i) const {
						return _actionbuttons[i]; 
					}
	virtual Boolean	ConnectCloseToCancel(Widget, XtCallbackProc, XtPointer);
};

#endif /* ACTIONBUTTONS_H */
