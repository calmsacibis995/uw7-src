#ifndef _SRC_PANE_H
#define _SRC_PANE_H
#ident	"@(#)debugger:gui.d/common/Src_pane.h	1.2"

#include "Component.h"
#include "Eventlist.h"
#include "Sender.h"
#include "Panes.h"
#include "Reason.h"
#include "UI.h"
#include "Link.h"
#include "Menu.h"

class Menu_bar_table;
class ProcObj;
class Status_pane;
class Show_function_dialog;
class Show_line_dialog;
class Open_dialog;
struct Pane_descriptor;
class Text_editor;
class Sensitivity;
class FileSelectionDialog;
class FileSaveQueryDialog;
class StoredFiles;

struct NewSourceInfo {
	const char	*file;
	char		*path;
	int		line;
	int		position;
	Boolean		update;
	Boolean		set_current;
};

class Source_pane : public Pane
{
	Caption			*caption;
	Text_editor		*pane;
	File_list		*file_ptr;	// ptr to breakpoint info for current file
	Flink			*flink;
	const char		*current_file;
	char			*current_path;
	const char		*compilation_unit; // may be same as current file
	int			current_line;
	int			selected_line;
	Pane_descriptor		*pdesc;

	Show_function_dialog	*show_function;
	Show_line_dialog	*show_line;
	Open_dialog		*open_box;

	FileSelectionDialog	*file_selection_box;
	FileSaveQueryDialog	*file_save_query_box;
	NewSourceInfo		new_info;
	Boolean			registered;
	Boolean			animated;
	Boolean			primary;	// primary source window for
						// the window set

	Boolean			do_set;
	Boolean			set_path(Boolean must_set, const char *,
					char *&);
	void			save_as_dlg_init();
	void			clear_info();
public:
				Source_pane(Window_set *, Base_window *, Box *,
					Pane_descriptor *, Boolean is_primary);
				~Source_pane();

				// access functions
	char			*get_current_path()	{ return current_path; }
	const char		*get_current_file()	{ return current_file; }
	const char		*get_compilation_unit()	{ return compilation_unit; }
	int			get_selected_line()	{ return selected_line; }
	int			get_current_line()	{ return current_line; }
	Text_editor		*get_pane()		{ return pane; }
	void			set_current_line(int l)	{ current_line = l; }
	void			animate(Boolean a) 	{ animated = a; }
	Boolean			is_animated()		{ return animated; }

				// framework callbacks
	void			clear();
	void			update_cb(void *, Reason_code, void *, ProcObj *);
	void			update_state_cb(void *, Reason_code, void *, ProcObj *);
	void			break_list_cb(void *, Reason_code_break, void *, Stop_event *);

				// component callbacks
	void			discard_and_set_cb(Component *, void *);
	void			toggle_break_cb(Component *, void *);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			ok_to_delete(Component *, int);
	void			show_function_cb(Component *, void *);
	void			show_line_cb(Component *, void *);
	void			open_dialog_cb(Component *, void *);
	void			new_source_cb(Component *, void *);
	void			select_cb(Text_display *, int line);
	void			cut_cb(Component *, void *);
	void			delete_cb(Component *, void *);
	void			paste_cb(Component *, void *);
	void			undo_cb(Component *, void *);
	void 			save_file_cb(Component *, void *);
	int 			save_and_set();
	void 			save_as_dlg_cb(Component *, void *);
	void 			save_as_and_set_cb(Component *, void *);
	void 			save_as_cb(Component *, const char *);

	void			set_file(const char *file, const char *path,
					const char *comp_unit, int pos, int line);
	void			set_new_file_cb(Component *, void *);
	void			de_message(Message *m);

				// functions overriding virtuals in Pane base class
	void			popup();
	void			popdown();
	char			*get_selection();
	void			copy_selection();
	Selection_type		selection_type();
	int			check_sensitivity(Sensitivity *);
	Boolean			current_changed();
};

#endif	// _SOURCE_H
