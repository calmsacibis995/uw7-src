#ifndef	_RADIO_H
#define	_RADIO_H
#ident	"@(#)debugger:gui.d/common/Radio.h	1.4"

#include "Component.h"
#include "RadioP.h"
#include "gui_label.h"

// A radio list is array of buttons, either horizontal or vertical
// one and only one button may be selected

// Framework callbacks:
// there is one callback function associated with the entire list
// the callback is invoked as
//	creator->function((Radio_list *)this, (int)selection_button)

class Radio_list : public Component
{
	RADIO_TOOLKIT_SPECIFICS

private:
        Callback_ptr    callback;

public:
			Radio_list(Component *parent, const char *name, Orientation,
				const LabelId *buttons, int cnt, int initial_button,
				Callback_ptr fn = 0, Command_sender *creator = 0,
				Help_id help_msg = HELP_none);
			~Radio_list();

	int		which_button();		// 0 to cnt-1
	void		set_button(int button);	// 0 to cnt-1
	void		set_sensitive(int which, Boolean sense);
	Callback_ptr	get_callback()		{ return callback; }
};

#endif	// _RADIO_H
