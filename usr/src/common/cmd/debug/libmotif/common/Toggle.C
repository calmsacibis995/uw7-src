#ident	"@(#)debugger:libmotif/common/Toggle.C	1.3"

#include "UI.h"
#include "Component.h"
#include "Toggle.h"
#include "gui_label.h"
#include "Label.h"
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>
#include <stdio.h>

// Invoke the framework callback if there is one
static void
toggleCB(Widget w, Toggle_data *tdata, XmToggleButtonCallbackStruct *data)
{
	Boolean			state;
	Callback_ptr		cb;
	Command_sender		*creator;
	Toggle_button		*ptr = 0;

	XtVaGetValues(w, XmNuserData, &ptr, 0);
	if (!ptr || !tdata)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	state = (Boolean) (data->set);
//fprintf(stderr, "toggleCB: data %x, set %d\n", tdata, state);
	cb = tdata->callback;
	creator = ptr->get_creator();
	if (cb && creator)
		(creator->*cb)(ptr, (void *)state);
}

Toggle_button::Toggle_button(Component *p, const char *s, const Toggle_data *b,
	int n, Orientation o, Command_sender *c, Help_id h) 
	: Component(p, s, c, h)
{
	Widget	*toggle;

	nbuttons = n;
	buttons = b;
	toggle = new Widget[nbuttons];
	toggles = toggle;
	
	widget = XtVaCreateWidget(label,
			xmRowColumnWidgetClass,
			parent->get_widget(),
			XmNisHomogeneous, TRUE,
			XmNentryClass, xmToggleButtonGadgetClass,
			XmNorientation, 
				o == OR_vertical ? XmVERTICAL : XmHORIZONTAL,
			XmNspacing, 0,
			XmNuserData, this,
			0);
	for (int i = 0; i < nbuttons; i++, b++, toggle++)
	{
		const char	*blabel;
		XmString	label_string;

		blabel = labeltab.get_label(b->label);
		label_string = XmStringCreateLocalized((String)blabel);
		*toggle = XtVaCreateManagedWidget(blabel,
				xmToggleButtonGadgetClass,
				widget,
				XmNlabelString, label_string,
				XmNset, b->state,
				XmNindicatorType, XmN_OF_MANY,
				XmNuserData, this,
				0);
		if (b->callback)
			XtAddCallback(*toggle, XmNvalueChangedCallback,
				(XtCallbackProc)toggleCB, (XtPointer)b);
		XmStringFree(label_string);
	}

	XtManageChild(widget);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Toggle_button::~Toggle_button()
{
	delete toggles;
}

void
Toggle_button::set(int button, Boolean setting)
{
	XtVaSetValues(toggles[button], XmNset, setting, 0);
}

void
Toggle_button::set_sensitive(int button, Boolean setting)
{
	XtVaSetValues(toggles[button], XmNsensitive, setting, 0);
}

Boolean
Toggle_button::is_set(int button)
{
	Boolean setting = FALSE;
	XtVaGetValues(toggles[button], XmNset, &setting, 0);
	return setting;
}
