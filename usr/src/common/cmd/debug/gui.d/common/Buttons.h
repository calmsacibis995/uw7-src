#ifndef _BUTTONS_H
#define _BUTTONS_H
#ident	"@(#)debugger:gui.d/common/Buttons.h	1.3"

// Common button structure used for menu and button bar
// buttons - there is a core part, which is intended
// to be read-only, and a client-modifiable part.

#include "Link.h"
#include "Sense.h"
#include "Panes.h"
#include "str.h"
#include "gui_label.h"

#ifdef MULTIBYTE
#include <stdlib.h>  // for wchar_t
typedef	wchar_t		mnemonic_t;
#else
typedef unsigned char	mnemonic_t;
#endif

extern int windows_per_set;

enum CButtons {
	B_windows,
	B_sources,
	B_window_sets,
	B_dismiss,
	B_exit,
	B_create_dlg,
	B_grab_core_dlg,
	B_grab_proc_dlg,
	B_cd_dlg,
	B_rel_running,
	B_rel_susp,
	B_release,
	B_new_window_set,
	B_script_dlg,
	B_open_dlg,
	B_new_source,
	B_save,
	B_save_as_dlg,
	B_copy,
	B_move_dlg,
	B_set_current,
	B_input_dlg,
	B_interrupt,
	B_disable,
	B_enable,
	B_delete,
	B_change_dlg,
	B_cut,
	B_delete_text,
	B_paste,
	B_undo,
	B_map_dlg,
	B_export,
	B_pin,
	B_unpin,
	B_show_value_dlg,
	B_set_value_dlg,
	B_show_type_dlg,
	B_dump_dlg,
	B_show_line_dlg,
	B_show_func_source_dlg,
	B_search_dlg,
	B_show_loc_dlg,
	B_show_func_dis_dlg,
	B_run,
	B_return,
	B_run_until_dlg,
	B_step_statement,
	B_step_instruction,
	B_next_statement,
	B_next_instruction,
	B_step_dlg,
	B_jump_dlg,
	B_halt,
	B_animate_source,
	B_animate_dis,
	B_stop_on_func_dlg,
	B_stop_dlg,
	B_signal_dlg,
	B_syscall_dlg,
	B_on_stop_dlg,
#if EXCEPTION_HANDLING
	B_exception_dlg,
	B_ignore_exceptions_dlg,
#endif
	B_cancel_dlg,
	B_destroy,
	B_kill_dlg,
	B_ignore_signals_dlg,
	B_set_watch,
	B_set_break,
	B_delete_break,
	B_language_dlg,
	B_granularity_dlg,
	B_output_dlg,
	B_path_dlg,
	B_symbols_dlg,
	B_animation_dlg,
	B_dis_mode_dlg,
	B_frame_dir_dlg,
	B_button_dlg,
	B_table_of_cont_hlp,
#if OLD_HELP
	B_help_desk_hlp,
#endif
	B_version,
	B_process_pane_hlp,
	B_stack_pane_hlp,
	B_syms_pane_hlp,
	B_command_pane_hlp,
	B_source_pane_hlp,
	B_dis_pane_hlp,
	B_regs_pane_hlp,
	B_event_pane_hlp,
	B_status_pane_hlp,
	B_popup,
	B_save_layout,
	B_debug_cmd,
	B_exec_cmd,
// all buttons that can't go in a window button bar go after here;
// those that can, go before.
	B_separator,	// just a separator - not a real button
	B_menu,			// not a real button - sub-menu
	B_window_popup,
	B_set_popup,
	B_btn_dlg_add,	// button dialog
	B_btn_dlg_change,
	B_btn_dlg_del,
	B_btn_dlg_del_all,
	B_last
};

// last button valid in button bar
#define B_last_button_bar	(int)B_separator
// total possible buttons in button bar (one popup button for each
//			window except secondary source window
#define BUTTON_BAR_TOTAL	(B_last_button_bar+windows_per_set-2)

// The flags field in Button_cre describes what type of framework 
// object // the button's callback references, 
// or if the button brings up another menu.
// Callbacks are invoked as
// Pane_cb:	pane->function((Obj *)this, button->cdata)
// Window_cb:	base_win->function((Obj*)this, button->cdata)
// Set_cb:	window_set->function((Obj*)this, window)
// Set_data_cb:	window_set->function((Obj*)this, button->cdata)
// Creator_cb:	creator->function((Obj*)this, button->cdata)
// Here Obj refers to a Menu or Button_bar, or Command_sender.

#define	Pane_cb		1	// individual pane
#define Window_cb	2	// base window
#define	Set_cb		4	// window set, with window where button was pushed
#define	Menu_button	8	// cascade, brings up another menu
#define Set_data_cb	16	// window set, with call data
#define Menu_separator	32	// separator line (for grouping)
#define Creator_cb	64	// creator (dialog)

class Menu_descriptor;

struct Button_core {
	CButtons	button_type;
	LabelId		label;
	unsigned char	flags;		// callback type
	unsigned short 	pane_type;	// if pane-specific, which pane
	unsigned short 	panes_req;	// panes required for this button
	Sensitivity	sensitivity;
	Callback_ptr	callback;	// function called when this
					// button is selected
	Help_id		help_msg;	// context sensitive help
					// may be 0
	int		cdata;		// client data used in 
					// callbacks, or
					// number of buttons for
					// cascading menus
};

class Button : public Link {
	const char	*name;
	Button_core	*button_core;
	Menu_descriptor	*sub_table;	// secondary menu, only when 
					// flags == Menu_button
	void		*user_data;
	mnemonic_t	mnemonic; 
public:
			Button(const char *n, mnemonic_t mne,
				Button_core *bc) {
				name = n; mnemonic = mne;
				button_core = bc; sub_table = 0;
				user_data = 0; }
			Button(Button *);
			Button() {}
			~Button();
	void		set_label(const char *new_lab) 
				{ delete (char *)name; 
					name = makestr(new_lab); }
	Button		*next()	{ return (Button *)Link::next(); }
	Button		*prev() { return (Button *)Link::prev(); }
	Button_core	*core_info() { return button_core; }
	const char	*get_name() { return name; }
	mnemonic_t 	get_mne() { return mnemonic; }
	void		*get_udata() { return user_data; }
	void		set_udata(void *d) { user_data = d; }
	Menu_descriptor	*get_sub_table() { return sub_table; }
	void		set_sub_table(Menu_descriptor *md) 
				{ sub_table = md; }
	Callback_ptr	get_callback() { return button_core->callback; }
	int		get_flags() { return button_core->flags; }
	int		get_pane_type() { return button_core->pane_type; }
	int		get_panes_req() { return button_core->panes_req; }
	Sensitivity	*get_sensitivity() { return &button_core->sensitivity; }
	int		get_cdata() { return button_core->cdata; }
	Help_id		get_help_msg() { return button_core->help_msg; }
	CButtons	get_type() { return button_core->button_type; }
};

extern Button_core	*find_button(const char *);
extern Button_core	*find_button(CButtons);
extern const char	*button_action(CButtons);
// list of button core pointers, sorted alphabetically
// by label description - used by dynamic button bar
// configuration
extern Button_core	**button_list;
extern void		build_button_list();

#endif
