#ident	"@(#)ArrowButtons.h	1.2"
/******************************************************************************
 *		ArrowButtons class - Object that provides interface for up/down
 * 							arrow widgets with a text field. 
 ******************************************************************************/
#ifndef  ARROWBUTTONS_H
#define  ARROWBUTTONS_H

#include <Xm/Xm.h>

class ArrowButtons {

public:							/* Constructors/Destructors */ 

	ArrowButtons (XtAppContext); 
	~ArrowButtons ();

private:						/* Private Data */

	Widget				_textfieldw, _arrowup, _arrowdown;
	XtIntervalId		_arrow_timer_id;
	XtAppContext		_appcontext;
	int					_range, _incr, _minimum, _maximum, _steptime;

	enum { TIME_GAP = 200};
	enum { LOCAL_BUF = 16};

private:						/* Private Methods */
	
	static void			ArrowMoveCB(Widget, XtPointer, XtPointer);
	static void			TextModifyVerifyCB(Widget,XtPointer, XtPointer);

	void				arrow_move(Widget,XmArrowButtonCallbackStruct*);
	void				validate_character(XmTextVerifyCallbackStruct*);
	static void			change_value(XtPointer, XtIntervalId);

public:							/* Public ArrowButtons Methods */

	void				postInterface(Widget, XmString, int, int);
													/* Set Private data */
	void				SetDefaultValue(int);
	void				SetMaxValue(int);
	void				SetMinValue(int);
	void				SetStepTime(int);
													/* Get Private data */
	int					GetTimeValue ();
	int					GetStepTime () const  { return _steptime; }
	Widget				GetArrowUpWidget () const { return _arrowup; }
	Widget				GetArrowDownWidget() const { return _arrowdown;}
	Widget				GetTextFieldWidget() const { return _textfieldw;}
};

#endif	/* ARROWBUTTONS_H  */
