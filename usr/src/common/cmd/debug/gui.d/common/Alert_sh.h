#ifndef	_ALERT_SH_H
#define	_ALERT_SH_H
#ident	"@(#)debugger:gui.d/common/Alert_sh.h	1.4"

#include "Component.h"
#include "Alert_shP.h"
#include "UI.h"
#include "gui_label.h"

// An Alert_shell is a notice box used to display a message to the user
// and ask for confirmation.  An alert_shell may have one (action) or two
// (action, no_action) buttons.  The component is destroyed when the
// widget is popped down - the widget is not reused because if the
// message or button strings are replaced the widget may not be resized
// properly

// Callbacks are invoked as
// 	object->function((Alert_shell *)this, int yes_or_no)

class Alert_shell : public Component
{
	Callback_ptr	handler;
	Command_sender	*object;
	Boolean		fatal_error;

	ALERT_SHELL_TOOLKIT_SPECIFICS

public:
			Alert_shell(const char *message_string,
				Alert_type,
				LabelId action,	// first button label
				LabelId no_action = LAB_none, // 2nd button label
				Callback_ptr handler = 0, void *object = 0);
			~Alert_shell();

			// access functions
	Callback_ptr	get_handler()	{ return handler; }
	Command_sender	*get_object()	{ return object; }

	void		popup();
	void		popdown();
	Boolean		is_fatal() { return fatal_error; }
};

#endif	// _ALERT_SH_H
