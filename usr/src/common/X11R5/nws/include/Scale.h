#ident	"@(#)Scale.h	1.2"
/*----------------------------------------------------------------------------
 *		Scale class - Object that provides interface for the Motif scale widget
 *----------------------------------------------------------------------------*/
#ifndef  SCALEWIDGET_H
#define  SCALEWIDGET_H

#include <Xm/Xm.h>

class Scale {

public:							// Constructors/Destructors
								Scale (char *, int, int, int);
	virtual						~Scale ();

private:						// Private Data
	int							_value, _orientation;
	XmString					_titlestring;

private:						// Private Methods
	static void					ValueChangedCB(Widget, XtPointer, XtPointer);
	static void					DragScaleCB(Widget, XtPointer, XtPointer);

	virtual void				value_changed(XmScaleCallbackStruct *) = 0;
	virtual void				drag_scale(XmScaleCallbackStruct *) = 0;

protected:						// Protected data
	Widget						_scale;
	int							_minimum, _maximum;

protected:						// Protected Scale Methods
	virtual						void postInterface(Widget);
								// Set Private data
	virtual void				SetDefaultValue(int);
	virtual void				SetMaxValue(int);
	virtual void				SetMinValue(int);
	virtual void				SetSensitive(Boolean);
	virtual Boolean				SetValue(int, Boolean);
	virtual void				SetCurrentScaleValue (int);

};

#endif	// SCALEWIDGET_H 
