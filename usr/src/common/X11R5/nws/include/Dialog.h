#ident	"@(#)Dialog.h	1.3"
/////////////////////////////////////////////////////////
// Dialog.h:
/////////////////////////////////////////////////////////
#ifndef DIALOG_H 
#define  DIALOG_H
#include <Xm/Xm.h>

enum 	{ INFORMATION, PROMPT , WARNING , QUESTION, WORKING , ERROR  };

class Dialog  {
    
private:
    
	Widget				_okWidget;
	Widget				_cancelWidget;
	Widget				_helpWidget;
	char				*_name;
	Boolean				_setMnemonic;
	XtCallbackProc		_cancelfunc;
	XtPointer			_canceluserdata;

	/* the type determines which type of dialog to post
	 * Set it before calling postDialog in the derived
	 * class
	 */
	int		_type;
	enum	{OK,CANCEL, HELP};

protected:

	Widget				_dialogw;

public:
    
	/* CTOR & DTOR for Dialog
	 */
   	Dialog (char *, int);
   	~Dialog ();

	/* 
	 * Manage the widget and register mnemonic info on shell 
	 */
	void	manage ();

	/* Register callbacks for OK, HELP AND CANCEL
	 * if needed in your derived class. Pass any
	 * parameters with the function. 
	 */
	void 	registerOkCallback (XtCallbackProc, XtPointer);
	void 	registerCancelCallback (XtCallbackProc, XtPointer);
	void 	registerHelpCallback (XtCallbackProc, XtPointer);

	/* If some of the buttons in the message dialog are not
	 * needed then unmanage them
	 */
	void 	unmanageOk ();
	void 	unmanageCancel ();
	void 	unmanageHelp ();

	/* set the label strings for the OK, CANCEL AND HELP
	 * if you want them to be anything other than ok
	 * cancel help
	 */
	void 	setOkString (char *,char * mnem = NULL);
	void 	setHelpString (char *,char * mnem = NULL);
	void 	setCancelString (char *,char * mnem = NULL);
	void 	setSelectionLabelString (char *);
	
	/* Set the modality  and sensitivity
	 */
    void 		SetModality(unsigned char);
	void 		SetSensitive(int, Boolean);
	Boolean 	ConnectCloseWindow();

	/* Post the dialog to the screen. In this derived
	 * function, determine the type so the appropriate
	 * message dialog can be posted and also unmanage
	 * the requisite buttons and register your callbacks
	 */
    void 		postDialog ( Widget, char *, char *);
    Widget 		GetOkWidget() const { return _okWidget; }
    Widget		GetHelpWidget() const {return _helpWidget; }
    Widget 		GetDialogWidget() const { return _dialogw; }
};

#endif
