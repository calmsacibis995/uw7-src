#ident	"@(#)ArrowButtons.C	1.2"
/******************************************************************************
 *	ArrowButtons class - put up arrow up/down widgets to manipulate text field
 ******************************************************************************/
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "ArrowButtons.h"
#include "misc.h"
#include "Dialog.h"

#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/ArrowB.h>

/*********************************************************************
 * ArrowButtons Constructors/Destructors
 *********************************************************************/
ArrowButtons::ArrowButtons (XtAppContext appc)
{
#ifdef DEBUG
cout << "ctor for ArrowButtons" <<  endl;
#endif
	_maximum = _minimum = _range = _incr = 0;
	_arrow_timer_id = 0;
	_appcontext = appc;
	_arrowup = _arrowdown = _textfieldw = 0;
	/* Set the steptime to be default of 200 milliseconds
	 */
	_steptime = TIME_GAP;
}

/*********************************************************************
 * Dtor for NetWare Server Status object.  Clean up the process object
 *********************************************************************/
ArrowButtons::~ArrowButtons ()
{
#ifdef DEBUG
cout << "dtor for ArrowButtons" <<  endl;
#endif
	XtDestroyWidget (_arrowup);	
	XtDestroyWidget (_arrowdown);	
	XtDestroyWidget (_textfieldw);	
}

/**************************************************************************
 * Post the gui interface for the arrow widgets here.
 **************************************************************************/
void
ArrowButtons::postInterface (Widget parent, XmString textStr, int upval, 
							int downval)
{
	int	i;

	/* Set up arrow up/down fields to manipulate value of textfield here
	 * The user data tells the callback method how to change the field.
	 */
	for (i = 0; i < 2; i++) {

		Widget arrow_w;

		arrow_w  = XtVaCreateManagedWidget ("ArrowW", xmArrowButtonWidgetClass,                                             parent, 
                                            XmNarrowDirection,  i ? XmARROW_DOWN
                                                                : XmARROW_UP, 
                                            XmNuserData,   i ? downval : upval ,
                                            NULL);
    	XtAddCallback (arrow_w, XmNarmCallback, ArrowMoveCB, this);
    	XtAddCallback (arrow_w, XmNdisarmCallback, ArrowMoveCB, this);
	
		/* Set private variables for external usage
	 	 */
		i == 0 ? _arrowup = arrow_w : _arrowdown = arrow_w; 
	}

	/* Set up the text field to hold the value here. Add callback to verify
	 * modified character in the text field.
	 */
    _textfieldw = XtVaCreateManagedWidget("textfieldw", xmTextFieldWidgetClass, 
                                           parent, 	NULL);
    XtAddCallback (_textfieldw, XmNmodifyVerifyCallback, TextModifyVerifyCB,
                   this);

	/* Set up the label, if any,  after the text field here
	 */
	if (textStr) 
  		XtVaCreateManagedWidget ("textStr", xmLabelWidgetClass, parent, 
                                 XmNlabelString,   textStr, NULL);
}

/**************************************************************
 * Set the maximum value if any, for the text field. 
 **************************************************************/
void
ArrowButtons::SetMaxValue (int max)
{
	/* Set the max value for the text field
	 */
	_maximum = max;	
}

/**************************************************************
 Set the minimum value if any, for the text field. 
 **************************************************************/
void
ArrowButtons::SetStepTime (int stepin)
{
	/* Set the min value for the text field
	 */
	_steptime = stepin;	
}

/**************************************************************
 Set the minimum value if any, for the text field. 
 **************************************************************/
void
ArrowButtons::SetMinValue (int min)
{
	/* Set the min value for the text field
	 */
	_minimum = min;	
}

/**************************************************************
 * Set the default value in the text field next to the arrows.
 **************************************************************/
void
ArrowButtons::SetDefaultValue (int defaultvalue)
{
	char			buf[LOCAL_BUF];

	/* Set the default value for the text feild
	 */
    sprintf (buf, "%d", defaultvalue);
	XtVaSetValues (_textfieldw, XmNvalue, buf, NULL);
}

/**************************************************************
 * Text field value has modified and needs to be validated. 
 **************************************************************/
void
ArrowButtons::TextModifyVerifyCB(Widget widget, XtPointer client_data, 
								XtPointer call_data)
{
	XmTextVerifyCallbackStruct *cb = (XmTextVerifyCallbackStruct *) call_data;
	ArrowButtons	*obj = (ArrowButtons *) client_data;

	obj->validate_character(cb);
}

/******************************************************************************
 * Function that sees if only digits are entered in the step up/down field.
 *****************************************************************************/
void
ArrowButtons::validate_character(XmTextVerifyCallbackStruct *cb)
{
	int			len;

	for (len = 0; len < cb->text->length; len++) {
		if (!isdigit (cb->text->ptr[len]))  {
			cb->doit = FALSE;
			break;
		} 	/* If not digit */
	} 	/* Loop thru all characters */
}

/******************************************************************************
 * Static callback function for arrow buttons. 
 *****************************************************************************/
void 
ArrowButtons::ArrowMoveCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 
	ArrowButtons *obj = (ArrowButtons *) client_data;
    XmArrowButtonCallbackStruct *cb = (XmArrowButtonCallbackStruct *)call_data;

    obj->arrow_move(w, cb);
}

/******************************************************************************
 * When the arrow moves up / down call this routine. 
 *****************************************************************************/
void 
ArrowButtons::arrow_move(Widget w, XmArrowButtonCallbackStruct *cb)
{ 
	XtVaGetValues (w, XmNuserData, &_incr, NULL);
    if (cb->reason == XmCR_ARM) 
		change_value ((XtPointer)this, cb->event->type == ButtonPress || 
                      cb->event->type == KeyPress); 
	else if (cb->reason == XmCR_DISARM)
		XtRemoveTimeOut (_arrow_timer_id);
}

/******************************************************************************
 * When the grace period field has been changed up/down then call this routine.
 *****************************************************************************/
void 
ArrowButtons::change_value (XtPointer client_data, XtIntervalId id)
{ 
	ArrowButtons *obj = (ArrowButtons *)client_data; 
	char            buf[LOCAL_BUF]; 
	int				newrange;			
	
	obj->GetTimeValue (); 

	newrange = obj->_range += obj->_incr; 
        
	/* If the max value is set and it was reached you were trying to add to it 
 	 * then you reset to minimum value. If you were trying to go further down 
	 * below the minimum value you reset the value to minimum value.
	 */ 
	if ((obj->_maximum && newrange > obj->_maximum) || 
		(newrange < obj->_minimum))
			obj->_range = obj->_minimum;

	sprintf (buf, "%d", obj->_range); 
	XtVaSetValues (obj->_textfieldw, XmNvalue,  buf, NULL); 
	obj->_arrow_timer_id = XtAppAddTimeOut (obj->_appcontext, obj->_steptime, 
                                            (XtTimerCallbackProc) 
                                            &ArrowButtons::change_value, 
                                            (XtPointer)obj);
}

/******************************************************************************
 * Get the value from the text field. 
 *****************************************************************************/
int 
ArrowButtons::GetTimeValue ()
{
	char 	      *value;

	XtVaGetValues (_textfieldw, XmNvalue, &value, NULL);
   _range = atoi(value);
	return _range;
}
