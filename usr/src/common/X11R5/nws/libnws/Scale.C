#ident	"@(#)Scale.C	1.2"
/*---------------------------------------------------------------------------- *
 *	Scale Object - 	provide an interface for a scale widget (acts like a Motif
 *	 				wrapper).
 *--------------------------------------------------------------------------*/
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>

#include "Scale.h"
#include "misc.h"
#include "Dialog.h"

#include <Xm/Scale.h>

/*********************************************************************
 * Scale Constructors/Destructors
 *********************************************************************/
Scale::Scale (char *scalelabel, int orientation, int min, int max)
{
#ifdef DEBUG
cout << "ctor for Scale" <<  endl;
#endif
	_maximum = max == 0 ? 100 : max;
	_minimum = min;
	_orientation = orientation;
	_titlestring = XmStringCreateLocalized(scalelabel);
}

/*********************************************************************
 * Dtor for Scale object.  Clean up the title string for which space 
 * was created.
 *********************************************************************/
Scale::~Scale ()
{
#ifdef DEBUG
cout << "dtor for Scale" <<  endl;
#endif
	delete _titlestring;
}

/**************************************************************************
 * Post the gui interface for the scale widget here.
 **************************************************************************/
void
Scale::postInterface (Widget parent)
{
	/* Set up the row column to hold the arrow fields here
	 */
	_scale = XtVaCreateManagedWidget ("scale", xmScaleWidgetClass, parent,
                    					XmNorientation,  	_orientation,
                    					XmNtitleString,  	_titlestring,
                    					XmNminimum,  		_minimum,
                    					XmNmaximum,  		_maximum,
                    					0);

	/* Add callback methods for the scale widget
	 * else use our default CB methods
	 */
	XtAddCallback (_scale, XmNvalueChangedCallback, ValueChangedCB, this);
	XtAddCallback (_scale, XmNdragCallback, DragScaleCB, this);
}

/**************************************************************
 * Set the scale to be input-output or output only 
 **************************************************************/
void
Scale::SetSensitive(Boolean sensitive)
{ 
	XtVaSetValues (_scale, XmNsensitive, sensitive, 0);
}

/**************************************************************
 * Set the current position of the scale. Make sure the current
 * position is within the min and max values for the scale.
 **************************************************************/
Boolean
Scale::SetValue(int value, Boolean showvalue)
{ 
	if (value > _maximum || value < _minimum)
		return False;

	XtVaSetValues (_scale,
					XmNvalue,  			value, 
					XmNshowValue,  		showvalue,
					0);
	return True;
}

/**************************************************************
 * Set the maximum value if any, for the scale. 
 **************************************************************/
void
Scale::SetMaxValue(int max)
{
	/* Set the max value for the text field
	 */
	XtVaSetValues (_scale, XmNmaximum, max, 0);
	_maximum = max;	
}

/**************************************************************
 Set the minimum value if any, for the scale. 
 **************************************************************/
void
Scale::SetMinValue (int min)
{
	/* Set the min value for the text field
	 */
	XtVaSetValues (_scale, XmNminimum, min, 0);
	_minimum = min;	
}

/**************************************************************
 * Set the value to be displayed in the scale.
 **************************************************************/
void
Scale::SetDefaultValue (int defaultvalue)
{

	/* Set the value for the scale to be displayed 
	 */
	XtVaSetValues (_scale, XmNvalue, defaultvalue, 0);
}

/******************************************************************************
 * Set the current value on the scale. 
 *****************************************************************************/
void 
Scale::SetCurrentScaleValue (int value)
{
	XtVaSetValues (_scale, XmNvalue, value, XmNshowValue,  True, NULL);
}

/*******************************************************************************
 * Static callback function when scale value has been modified by moving scale. 
 * Calls a pure virtual function that is defined in the inherited class.
 ******************************************************************************/
void
Scale::ValueChangedCB(Widget widget,XtPointer client_data, XtPointer call_data)
{
	Scale	*obj = (Scale *) client_data;
	XmScaleCallbackStruct *cb = (XmScaleCallbackStruct *) call_data;

	obj->value_changed(cb);
}

/******************************************************************************
 * Static callback function when scale is dragged. 
 * Calls a pure virtual function that is defined in the inherited class.
 *****************************************************************************/
void 
Scale::DragScaleCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 
	Scale *obj = (Scale *) client_data;
	XmScaleCallbackStruct *cb = (XmScaleCallbackStruct *) call_data;

    obj->drag_scale(cb);
}
