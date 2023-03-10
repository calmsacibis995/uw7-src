#ifndef	_DIALOG_SHP
#define	_DIALOG_SHP
#ident	"@(#)debugger:libmotif/common/Dialog_shP.h	1.7"

// toolkit specific members of the Dialog_shell class
// included by ../../gui.d/common/Dialog_sh.h

class DNDdata;
class MneInfo;

// toolkit specific members:
// popup_window		primary widget - widget in the base class is form or
//				rubber tile containing children
// control_area		widget containing buttons
// msg_widget		static text widget for error messages
// state		current state (see below)
// ok_to_popdown	true if button pushed makes the dialog go away when done
// cmds_sent		dialog can't be popped down until debug cmds are completed
// errors		number of errors generated by last action
// error_string		space for error messages
// buttons		array of button-specific data
// nbuttons		number of buttons in the control area
//
// make_buttons		creates the Button_data array and buttons widget

// Dialogs in state Dialog_popped_up are mapped.  We distinguish
// between dialogs that have simply been unmapped (because parent
// window has been closed) and dialogs that have been closed
// through XtPopdown (explicit close, cancel, dismiss).

enum Dialog_state {
	Dialog_popped_down,
	Dialog_unmapped,	// unmapped, but XtPopdown not called
	Dialog_popped_up,
};

#define DIALOG_SHELL_TOOLKIT_SPECIFICS \
private:					\
	Widget		popup_window;		\
	Widget		control_area;		\
	Widget		focus_widget;		\
	Widget		*buttons;		\
	Widget		mseparator;		\
	Widget		msg_widget;		\
	Widget		escape_button;		\
	char		*error_string;		\
	MneInfo		*mne_info;		\
	Boolean		*busy_buttons;		\
	Dialog_state	state;			\
	unsigned char	nbuttons;		\
	Boolean		ok_to_popdown;		\
	Boolean		cmds_sent;		\
	Boolean		default_is_exec;	\
	Boolean		is_busy;		\
	Boolean		buttons_centered;	\
	void		make_buttons(const DButton *, int num_buttons);\
						\
public:						\
	void		center_buttons();	\
	void		handle_escape(XEvent *);\
										\
	Widget		get_popup_window() { return popup_window; } \
	Boolean		get_cmds_sent()	{ return cmds_sent; } \
	void		set_ok_to_popdown(Boolean b) { ok_to_popdown = b; } \
	void		set_state(Dialog_state s)	{ state = s; } \
	Dialog_state	get_state() { return state; } \
	void		dismiss();

#endif	// _DIALOG_SHP
