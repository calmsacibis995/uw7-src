#ifndef	BASE_WIN_H
#define	BASE_WIN_H
#ident	"@(#)debugger:gui.d/common/Base_win.h	1.9"

#include "Component.h"
#include "Sender.h"
#include "Reason.h"
#include "Panes.h"
#include "Link.h"
#include "UI.h"

class Expansion_box;
class Bar_descriptor;
class Button_bar;
class Button_dialog;
class Menu;
class Pane;
class Vector;
class ProcObj;
class Panes_dialog;
class Search_dialog;
class Sensitivity;
class Status_pane;
class Event_pane;
class Ps_pane;
class Symbols_pane;
class Source_pane;
class Command_pane;
class Register_pane;
class Disassembly_pane;
class Stack_pane;
class Window_configuration;
class Window_descriptor;
class Command_dialog;
struct Bar_info;

extern int	cmd_panes_per_set;

// Base_window flags
#define	BW_IS_OPEN		1
#define BW_SET_LINE_EXCLUSIVE	2
class Base_window : public Command_sender
{
protected:
	Window_shell		*window;
	Expansion_box		*box1;
	Menu_bar		*menu_bar;
	Button_bar		*top_button_bar;
	Button_bar		*bottom_button_bar;
	Component		*top_insertion;
	Component		*bottom_insertion;
	unsigned char		flags;
	unsigned char		busy_count;
	unsigned char		sense_count; // for set_sensitivity
	unsigned char		set_line_count; // for Text_disp::set_line

	Pane 			*selection_pane;

	Status_pane		*status_pane;
	Event_pane		*event_pane;
	Ps_pane			*ps_pane;
	Disassembly_pane	*dis_pane;
	Register_pane		*regs_pane;
	Source_pane		*source_pane;
	Command_pane		*cmd_pane;
	Symbols_pane		*syms_pane;
	Stack_pane		*stack_pane;

	Window_descriptor	*win_desc;
#ifndef MOTIF_UI
	Panes_dialog		*panes_dialog;
#endif
	Search_dialog		*search_dialog;
	Command_dialog		*cmd_dialog;
	Button_dialog		*btn_config_box;

	int			check_sensitivity(Sensitivity *);
	char			*parse_cmd(const char *cmd);
public:
				Base_window(Window_set *, 
					Window_descriptor *,
					Window_configuration *wc = 0);
				~Base_window();

				// access functions
	Window_shell		*get_window_shell()	{ return window; }
	Menu_bar		*get_menu_bar()		{ return menu_bar; }
	Boolean			is_open()		{ return (flags & BW_IS_OPEN); }
	Pane			*get_selection_pane()	{ return selection_pane; }
	void			set_sense_count(int c)  { sense_count = (unsigned char)c; }
	int			get_sense_count()  	{ return sense_count; }
	int			get_set_line_count()  	{ return set_line_count; }
	void			set_set_line_count(int c) { set_line_count = c; }
	Pane			*get_pane(int type);
				// functions inherited from Command_sender
	void			de_message(Message *);
	Base_window		*get_window();

				// display functions
	void			popup(Boolean focus = TRUE);
	void			popup_window(Component *, void *);
	void			popdown();
	void			add_src_menu(const char *, Base_window *);
	void			update_src_menu(const char *, Base_window *);
	void			delete_src_menu(const char *, Base_window *);

				//callbacks
	void			update_state_cb(void *, Reason_code, void *, ProcObj *);

	void			inc_busy();
	void			dec_busy();
	void			set_sensitivity();
	void			set_sensitivity(Menu *);
	int			is_sensitive(Sensitivity *);
#ifndef MOTIF_UI
	void			setup_panes_cb(Component *, void *);
#endif
	void			copy_cb(Component *, void *);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			search_dialog_cb(Component *, void *);
	void 			help_sect_cb(Component *, void * );
	void 			button_dialog_cb(Component *, void *);
	Window_descriptor	*get_wdesc() { return win_desc; }
	Button_bar		*get_top_bar() { return top_button_bar; }
	Button_bar		*get_bottom_bar() { return bottom_button_bar; }
	void			update_button_bar(Bar_info *);
	void			update_button_config_cb();

				// selection handling routines
	Selection_type		selection_type();
	void			set_selection(Pane *);
	char			*get_selection();
	int			get_selections(Vector *);
	void			debug_cmd_cb(Component *, void *cmd);
	void			exec_cb(Component *, void *cmd);
};

#endif // BASE_WIN_H
