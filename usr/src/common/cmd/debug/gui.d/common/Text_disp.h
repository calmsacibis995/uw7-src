#ifndef	_TEXT_DISP_H
#define	_TEXT_DISP_H
#ident	"@(#)debugger:gui.d/common/Text_disp.h	1.11"

#include "Component.h"
#include "Text_area.h"
#include "Text_dispP.h"
#include "Text_edit.h"
#include "Help.h"

class Base_window;

class Text_display : public Text_area, public Text_editor
{

	TEXT_DISPLAY_TOOLKIT_SPECIFICS

	int		save_current(const char *fname);
	int		set_file(const char *path);

public:
			Text_display(Component *, const char *name,
				Base_window *,
				int rows, int columns,
				Callback_ptr select_cb = 0,
				Callback_ptr toggle_break_cb = 0,
				Command_sender *creator = 0,
				Help_id help_msg = HELP_none);
			~Text_display();

			// initialization routines
	void		setup_source_file();
	void		setup_disassembly();
	void		set_buf(const char *buf);

	void		set_line(int line);
	void		position(int line);
	void		set_stop(int line);
	void		clear_stop(int line);
	Search_return	search(const char *expr, int forwards);
	void		clear();

	int		get_position();
	int		get_last_line();
	void		change_directory(const char *);
	char		*get_selected_text();
	void		copy_selected_text();

	void		*get_client_data();
	void		cut_selected_text();
	void		delete_selected_text();
	void		paste_text();
	void		undo_last_change();
	Boolean		paste_available();
	Boolean		undo_pending();
};

#endif	// _TEXT_DISP_H
