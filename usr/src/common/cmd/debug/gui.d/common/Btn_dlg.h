#ifndef _Btn_dlg_h_
#define _Btn_dlg_h_

#ident	"@(#)debugger:gui.d/common/Btn_dlg.h	1.2"

#include "Dialogs.h"

class Base_window;
class Bar_descriptor;
class Button_name_dialog;
class Button_bar;
class Radio_list;
class Table;
struct Table_calldata;
class Toggle_button;
class Window_set;

class Button_dialog : public Dialog_box
{
	Base_window		*window;
	Bar_descriptor		*current_bar;
	Bar_descriptor		*new_bar;
	Table			*pane;
	Button_bar		*btn_bbar;
	Button_name_dialog	*btn_name_box;
	Radio_list		*which_bar;
	Toggle_button		*toggles;
	Text_line		*config_dir;
	short			bar_count;
	short			rows;
	short			current_selection;
	Boolean			bottom;
	Boolean			create_new_bar;
	Boolean			initial;
	int		setup_row(Boolean first_time, int row, 
				Button_core *, const char *);
	int		setup_list(Boolean bottom, Boolean create_new);
public:
			Button_dialog(Base_window *, Window_set *);
			~Button_dialog();
	void		re_init();
			// callbacks;
	void		select_cb(Table *, Table_calldata *);
	void		default_cb(Table *, Table_calldata *);
	void		deselect_cb(Table *, int);
	void		cancel_cb(void *, void *);
	void		apply(void *, void *);
	void		button_name_cb(void *, void *);
	void		del_btn_cb(void *, void *);
	void		del_all_cb(void *, void *);
	void		set_button(int);
	void		toggle_new_cb(Component *, Boolean);
	void		toggle_save_cb(Component *, Boolean);
	void		which_bar_cb(Component *, int);
};

#endif
