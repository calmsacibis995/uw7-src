#ifndef	_DIALOG_SH
#define	_DIALOG_SH
#ident	"@(#)debugger:gui.d/common/Dialog_sh.h	1.18"

#include "Component.h"
#include "gui_msg.h"
#include "gui_label.h"
#include "Severity.h"
#include "Dialog_shP.h"

class Message;
class Dialog_box;

// Each dialog constructor is passed an array of buttons.
// Each button includes a name, callback function, and type.
// The type indicates the action the Dialog_shell is to take
// after the callback is invoked.
// Except for B_exec and B_non_exec, the button type also
// indicates the default label; if no label is supplied
// the default label is used.  Whether or not the default
// label is used if a name is supplied is toolkit dependent.
// There can be no more than one button of each type
// (except for B_exec and B_non_exec) in any dialog.

enum Button_type
{
	B_ok,		// Motif-specific apply + close, same as apply for OpenLook
	B_apply,	// primary execution button
	B_reset,	// restore settings to state at popup-time
	B_close,	// dismiss the dialog
	B_cancel,	// reset plus close
	B_exec,		// secondary execution button (in OpenLook,
			// the dialog is dismissed)
	B_non_exec,	// non-execution button (dialog is not dismissed)
	B_help,		// help button
};

struct DButton
{
	Button_type	type;
	LabelId		label;
	LabelId		mnemonic;
	Callback_ptr	func;
};

// Dialog_shell is the interface to the popup window widget for command
// and property dialogs.  A Dialog_shell accepts one and only one child
// component (usually a box of some kind)

// Framework callbacks:
// dismiss is the function called when the dialog is dismissed (popped down);
// each button (except Close) also has an associated callback function
// all callbacks are invoked as creator->function(this, NULL)

// action taken when a file is dropped onto the popup window
enum Drop_cb_action {
	Drop_cb_popdown,
	Drop_cb_stayup
};

class Dialog_shell : public Component
{
	DIALOG_SHELL_TOOLKIT_SPECIFICS

private:
	Component	*child;
	Callback_ptr	dismiss_cb;

public:
			Dialog_shell(Component *parent, LabelId label,
				Callback_ptr dismiss, Dialog_box *creator,
				const DButton *buttons, int num_buttons,
				Help_id help_msg, Callback_ptr dcb = NULL,
				Drop_cb_action dcb_act = Drop_cb_stayup);
			~Dialog_shell();

	void		add_component(Component *);	// accepts only one child
	void		set_popup_focus(Component *);
	void		popup();		// bring up the popup window
	void		popdown();		// dismiss the dialog
	void		default_start();
	void		default_done();
	void		cmd_complete();		// dialog action is finished
	void		set_busy(Boolean);	// show busy cursor until action complete
	void		wait_for_response();	// cmd sent to debug, action not completed
	void		error(Message *);	// display an error message from debug
	void		error(Severity, Gui_msg_id, ...); // error caught by the gui
	void		error(Severity, const char *);
	void		clear_msg();
	Boolean		is_pinned();
	Boolean		is_open();
	void		set_label(LabelId label, LabelId mnemonic);
	void		set_help(Help_id help_msg);
	void		set_focus(Component *);	// change focus widget
	void		set_sensitive(LabelId, Boolean); // mne says which button
	void		set_sensitive(Boolean); // set state of all action buttons
	void		remove_default_button(); // this dialog has
						 // no default

			// desktop drop interface:
	char		*get_drop_item();
};

#endif	// _DIALOG_SH
