#ifndef _SYMS_PANE_H
#define _SYMS_PANE_H
#ident	"@(#)debugger:gui.d/common/Syms_pane.h	1.16"

#include "Component.h"
#include "Dialogs.h"
#include "Sender.h"
#include "Panes.h"
#include "List.h"

class Base_window;
class Message;
struct Pane_descriptor;
class ProcObj;
class Prog_sym_list;
class Sym_list;
class Symbol;
class Symbol_dialog;
class Symbols_pane;
class Table;
class Table_calldata;
class Toggle_button;
class Vector;
class Window_set;
class Sensitivity;

enum Syms_cell { SY_pin, SY_name, SY_loc, SY_type, SY_value };

#define SYM_local	001
#define	SYM_file	002
#define	SYM_global	004
#define	SYM_debugger	010
#define	SYM_user	020

class Pin_list
{
	Symbols_pane	*syms_pane;
	Sym_list	*du_list;	// debugger & user symbols
	Prog_sym_list	*prog_list;	// program symbols for current pobj
	List		prog_list_list;	// prog symbols for all pobjs
	void		do_update(Sym_list *list, ProcObj *pobj = 0);

public:
			Pin_list(Symbols_pane *sp);
			~Pin_list();
	Boolean		is_empty() { return !du_list && !prog_list; }
	void		delete_current();
	void		set_current(ProcObj *);
	void		update(Boolean delay);
	Symbol		*find_sym(char *name, char *loc);
	void		add_sym(char *name, char *loc, char *type);
	void		remove_sym(char *name, char *loc);
};

class Symbols_pane : public Pane
{
	friend		Pin_list;

	int		symbol_types;
	Pin_list	pin_list;
	Table		*pane;
	int		next_row;
	int		total_selections;
	int		reading_symbols;
	int		update_in_progress;
	int		update_abort;
	Symbol_dialog	*sym_dialog;
	void		clear();

public:
			Symbols_pane(Window_set *cw, Base_window *parent, Box *,
				Pane_descriptor *);
			~Symbols_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_sym_type()	{ return symbol_types; }
	int		get_total()	{ return total_selections; }

	void		set_sym_type(int);

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *server, Reason_code, void *, ProcObj *);
	void		select_symbol(Table *, Table_calldata *);
	void		col_select_symbol(Table *, Table_calldata *);
	void		deselect_symbol(Table *, int);
	void		default_cb(Table *, Table_calldata *);
	void		setup_syms_cb(Component *, void *);
	void		export_syms_cb(Component *, void *);
	void		set_watchpoint_cb(Component *, void *);
	void		pin_sym_cb(Component *, void *);
	void		unpin_sym_cb(Component *, void *);

			// functions inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

			// functions overriding virtuals in Pane base class
	Selection_type	selection_type();
	void		deselect();
	int		get_selections(Vector *);
	void		popup();
	void		popdown();
	int		check_sensitivity(Sensitivity *);
};

class Symbol_dialog : public Dialog_box
{
	Toggle_button	*program_settings;
	Toggle_button	*debug_settings;
	Symbols_pane	*parent;

public:
			Symbol_dialog(Symbols_pane *);
			~Symbol_dialog() {}

	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

#endif	// _SYMS_PANE_H
