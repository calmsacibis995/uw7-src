#ident	"@(#)debugger:libmotif/common/Radio.C	1.2"

#include "UI.h"
#include "Component.h"
#include "Radio.h"
#include "gui_label.h"
#include "Label.h"
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>
#include <stdio.h>

//#define DEBUG_R(x)	Debug_t(DBG_R, x)
#define DEBUG_R(x)

// radioCB is called by the toolkit when the user pushes a button
// the toolkit sets the state field in the Button_data array
// radioCB sets current to the number of the button pushed, and invokes
// the framework callback, if there is one
static void
radioCB(Widget w, int item, XmToggleButtonCallbackStruct *)
{
	Radio_list	*ptr = 0;

	DEBUG_R((stderr,"radioCB: item %d\n", item));

	XtVaGetValues(w, XmNuserData, &ptr, 0);
	if (!ptr  || item < 0 || item >= ptr->get_nbuttons())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	ptr->set_current(item);

	Command_sender	*creator = ptr->get_creator();
	Callback_ptr	cb = ptr->get_callback();
	if (cb && creator)
		(creator->*cb)(ptr, (void *)item);
}

Radio_list::Radio_list(Component *p, const char *s, Orientation orient,
	const LabelId *names, int cnt, int initial_button, Callback_ptr f, 
	Command_sender *c, Help_id h) : Component(p, s, c, h)
{
	Widget	*button;

	callback = f;
	if ((initial_button >= cnt) || (callback && !creator))
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	current = initial_button;
	nbuttons = cnt;
	widget = XtVaCreateWidget((char *)label,
		xmRowColumnWidgetClass,
		parent->get_widget(),
		XmNuserData, this,
		XmNpacking, XmPACK_TIGHT,
		XmNisHomogeneous, TRUE,
		XmNentryClass, xmToggleButtonGadgetClass,
		XmNradioAlwaysOne, TRUE,
		XmNradioBehavior, TRUE,
		XmNorientation, (orient == OR_vertical) ? 
			XmVERTICAL : XmHORIZONTAL,
		0);
	buttons = button = new Widget[cnt];

	for (int i = 0; i < cnt; i++, button++, names++)
	{
		XmString label_string = XmStringCreateLocalized((String)labeltab.get_label(*names));
		*button = XtVaCreateManagedWidget("radio_button",
			xmToggleButtonGadgetClass,
			widget,
			XmNset, (i == current) ? TRUE : FALSE,
			XmNindicatorType, XmONE_OF_MANY,
			XmNlabelString, label_string,
			XmNuserData, this,
			0);
		XtAddCallback(*button, XmNarmCallback,
			(XtCallbackProc)radioCB, (XtPointer)i);
		XmStringFree(label_string);
	}
	XtManageChild(widget);


	if (help_msg)
		register_help(widget, label, help_msg);
}

Radio_list::~Radio_list()
{
	delete buttons;
}

// set_button is called from the framework, when it needs to update the
// displayed value, for example if the user sets %lang in a script,
// the Set_language dialog will call set_button to display the new
// current language
void
Radio_list::set_button(int which)
{
	if (which < 0 || which >= nbuttons)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	XtVaSetValues(buttons[current], XmNset, FALSE, 0);
	XtVaSetValues(buttons[which], XmNset, TRUE, 0);
	current = which;
}

// this can't be inlined because it is a toolkit-specific implementation of
// a framework-independent interface
int
Radio_list::which_button()
{
	return current;
}

void
Radio_list::set_sensitive(int which, Boolean sense)
{
	if (which < 0 || which >= nbuttons)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	XtVaSetValues(buttons[which], XmNsensitive, sense, 0);
}
