#ifndef _Text_editor_
#define _Text_editor_

#ident	"@(#)debugger:gui.d/common/Text_edit.h	1.3"

#include "Component.h"
#include "Text_editP.h"
#include "Help.h"
#include "Link.h"
#include "str.h"
#include <string.h>

class Base_window;
class Command_sender;

class Breaklist : public Link {
	int		line;
public:
			Breaklist(int l) { line = l; }
	int		get_line() { return line; }
	Breaklist	*next() { return (Breaklist *)Link::next(); }
};

enum Search_return { SR_bad_expression, SR_found, SR_notfound };

// keep track of stored temporary files
// the current file does not have a StoredFile record
class StoredFiles {
	char		*file;
	char		*savedto;
	StoredFiles	*next;
public:
			StoredFiles(const char *orig, const char *save,
				StoredFiles *n)
				{
					file = makestr(orig);
					savedto = (char *)save;
					next = n;
				}
			~StoredFiles()	{ delete file; delete savedto; }
	int		is_file(const char *fname)
				{ return(strcmp(fname, file) == 0); }
	char		*get_saved() { return savedto; }
	char		*get_file() { return file; }
	StoredFiles	*get_next() { return next; }
	void		set_next(StoredFiles *n) { next = n; }
};


class Text_editor : public virtual Component {

	TEXT_EDIT_TOOLKIT_SPECIFICS

protected:
	const char		*curr_file;
	const char		*curr_temp;
	Base_window		*bw;
	const char		*search_expr;
	Breaklist		*breaks;
	int			rows;
	int			columns;
	int			last_line;
	StoredFiles		*stored_files;
	Callback_ptr		toggle_break_cb;
	Boolean			empty;
	Boolean			sfile_changed;

	void			clear_breaks();
	virtual int		save_current(const char *fname) = 0;
	virtual int		set_file(const char *path) = 0;
public:
				Text_editor(Component *p, const char *l,
					Command_sender *c,
					Base_window *, int rows, 
					int columns,
					Callback_ptr tbcb = 0,
					Help_id h = HELP_none);
	virtual			~Text_editor();
	void			set_breaklist(int *breaklist);
	Boolean			has_breakpoint(int line);
	Boolean			file_changed() { return sfile_changed; }
	int			set_new_file(const char *path);
	int			save_current_as(const char *fname);
	int			discard_changes();
	void			discard_changes(StoredFiles *);
	int			save_current_to_temp();
	int			save_stored_file(StoredFiles *, char *);
	StoredFiles		*get_stored_files() { return stored_files; }

	virtual void		setup_source_file() = 0;

	virtual void		set_line(int line) = 0;
	virtual void		position(int line) = 0;
	virtual void		set_stop(int line) = 0;
	virtual void		clear_stop(int line) = 0;
	virtual Search_return	search(const char *expr, int forwards) = 0;
	virtual void		clear() = 0;
	virtual void		change_directory(const char *) = 0;

	virtual int		get_position() = 0;
	virtual int		get_last_line() = 0;
	virtual char		*get_selected_text() = 0;
	virtual void		copy_selected_text() = 0;
	virtual void		cut_selected_text() = 0;
	virtual void		delete_selected_text() = 0;
	virtual void		paste_text() = 0;
	virtual void		undo_last_change() = 0;
	virtual Boolean		paste_available() = 0;
	virtual Boolean		undo_pending() = 0;
};

#endif
