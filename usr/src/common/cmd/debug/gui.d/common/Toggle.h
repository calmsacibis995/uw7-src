#ifndef	_TOGGLE_H
#define	_TOGGLE_H
#ident	"@(#)debugger:gui.d/common/Toggle.h	1.4"

#include "Component.h"
#include "ToggleP.h"
#include "gui_label.h"

// A Toggle_button is series of on/off indicators (check boxes or push buttons)
// with text (the name) to the right
// The Toggle constructor is passed an array of descriptors,
// one for each toggle.  The information includes a name,
// callback function, and initial state.

struct Toggle_data
{
	LabelId		label;
	Boolean		state;
	Callback_ptr	callback;	// framework callback, may be null
};

// Framework callbacks:
// each individual toggle has an associated callback function
// callbacks are invoked as creator->function((Toggle_button *)this, (Boolean)state)

class Toggle_button : public Component
{
	TOGGLE_BUTTON_TOOLKIT_SPECIFICS

public:
			Toggle_button(Component *parent, const char *name,
				const Toggle_data *buttons, int nbuttons,
				Orientation, Command_sender *creator = 0,
				Help_id help_msg = HELP_none);
			~Toggle_button();

	Boolean		is_set(int button);	// 0 to nbuttons-1
	void		set(int button, Boolean);
	void		set_sensitive(int button, Boolean);
};

#endif	// _TOGGLE_H
