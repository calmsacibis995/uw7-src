#ident	"@(#)debugger:gui.d/common/Syms_pane.C	1.53"

#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "UI.h"
#include "Table.h"
#include "Toggle.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Resources.h"
#include "Sense.h"
#include "Syms_pane.h"
#include "Show_value.h"
#include "Menu.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "Label.h"

#include "Message.h"
#include "Msgtab.h"
#include "Machine.h"
#include "str.h"
#include "Buffer.h"
#include "Vector.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>

// we make the following assumptions about symbols:
// 1) for debugger or user symbols, it is uniquely identified by its
//    name, the location and type info are fixed globally
// 2) for a program symbol, it is uniquely identified by the triple
//    [name, file, line] and its execution context (pobj)
//    NOTE: we can't handle the following case currently:
//  	{ int x; ... { int x; ... } } /* all in one line */

class Symbol
{
	friend		Sym_list;
	friend		Pin_list;

	char		*name;
	char		*loc;
	char		*type;	// carried for display purposes
public:
			Symbol(char *n, char *l, char *t = 0);
			~Symbol();
};

// Sym_list is a list of symbols in sorted order
// Debugger & user symbols have no file,line & type info
class Sym_list
{
protected:
	List		sym_list;	// list of symbols
public:
			Sym_list() { }
			~Sym_list();
	Symbol		*first_sym() { return (Symbol *)sym_list.first(); }
	Symbol		*next_sym() { return (Symbol *)sym_list.next(); }
	void		add(char *name, char *loc = 0, char *type = 0);
	void		remove(char *name, char *loc = 0);
	Symbol		*find(char *name, char *loc = 0);
};

// for program symbols, these exist for the duration of the process or thread
class Prog_sym_list : public Sym_list
{
	ProcObj		*pobj;		// execution context of the symbols
public:
			Prog_sym_list(ProcObj *p) { pobj = p; }
			~Prog_sym_list() { }
	ProcObj		*get_pobj() { return pobj; }
};

Symbol::Symbol(char *n, char *l, char *t)
{
	name = makestr(n);
	loc = makestr(l);
	type = makestr(t);
}

Symbol::~Symbol()
{
	delete name;
	delete loc;
	delete type;
}

Sym_list::~Sym_list()
{
	for(Symbol *s = (Symbol *)sym_list.first(); 
			s; 
			s = (Symbol *)sym_list.next())
	{
		sym_list.remove(s);
		delete s;
	}
}

void
Sym_list::add(char *name, char *loc, char *type)
{
	if (!find(name, loc))
	{
		// assume sym_list's current pointer is either NULL
		// or pointing to one past the insertion point
		Symbol *sym = new Symbol(name, loc, type);
		sym_list.insert(sym);
	}
}

void
Sym_list::remove(char *name, char *loc)
{
	Symbol *s;

	if ((s = find(name, loc)) != NULL)
	{
		sym_list.remove(s);
		delete s;
	}
}

Symbol *
Sym_list::find(char *name, char *loc)
{
	// the list is sorted by name
	// for each name that matches, the location must also match
	for(Symbol *s = (Symbol *)sym_list.first(); 
			s; 
			s = (Symbol *)sym_list.next())
	{
		int val1 = strcmp(name, s->name);
		if (val1 < 0)
			break;
		else if (val1 > 0)
			continue;
		else if (!loc)
			// name match only
			return s;
		else
		{
			if (strcmp(loc, s->loc) == 0)
				return s;
		}
	}
	return NULL;
}

Pin_list::Pin_list(Symbols_pane *sp)
{
	syms_pane = sp;
	du_list = 0;
	prog_list = 0;
}

Pin_list::~Pin_list()
{
	delete du_list;
	Prog_sym_list *list;
	for (list = (Prog_sym_list *)prog_list_list.first(); list; 
			list = (Prog_sym_list *)prog_list_list.next())
		delete list;
}

void
Pin_list::update(Boolean delay_finish)
{
	if (!du_list && !prog_list)
		return;
	syms_pane->reading_symbols = 1;
	if (du_list)
		do_update(du_list);
	if (prog_list)
		do_update(prog_list, prog_list->get_pobj());
	if (!delay_finish)
		syms_pane->cmd_complete();
}

// create a fully qualified name
// there are 3 cases:
// 1) globals - <obj>@<name>
// 2) file statics - <file>@<name>
// 3) locals (autos & statics) - <func>@<line>@<name>
// for total disambiguation, we prepend a '@' for
// cases 1 & 2, and "@@" for case 3
// this ensures that we display the correct value of
// the variable even if hidden from the current scope

void
fully_qualified_name(const char *loc, const char *name, Buffer *buf)
{
	char	*at = strchr(loc, '@');
	buf->clear();
	buf->add('@');
	// need to enclose either <file> or <obj> in quotes
	// since these may contain '/', '-', ... which can be
	// confused with operators
	if (at)
	{
		// <func>@<line> for block scope
		// add another '@' to disambiguate it
		// from <thread_id>@<file>@... cases
		buf->add('@');
		buf->add('"');
		buf->add(loc, at - loc);
		buf->add('"');
		buf->add(at);
	}
	else
	{
		buf->add('"');
		buf->add(loc);
		buf->add('"');
	}
	buf->add('@');
	buf->add(name);
}

void
Pin_list::do_update(Sym_list *list, ProcObj *pobj)
{
	Table		*pane = syms_pane->pane;
	DBcontext	pobj_id = pobj ? pobj->get_id() : 0;
	int		&next_row = syms_pane->next_row;
	Buffer		*buf = buf_pool.get();
	char		*loc;
	char		*type;

	// issue "print -b fully-qualified-name"
	// for each symbol in list
	if (!pane->is_delayed())
        	pane->delay_updates();
	for (Symbol *s = (Symbol *)list->first_sym(); s;
		s = (Symbol *)list->next_sym())
	{
		Message	*m;
		char	*value = 0;
		Boolean	visible = TRUE;

		buf->clear();
		if (s->name[0] == '%' || s->name[0] == '$')
		{
			buf->add(s->name);
			loc = "debugger";
			type = "";
		}
		else if (s->loc)
		{
			loc = s->loc;
			type = s->type;
			fully_qualified_name(s->loc, s->name, buf);
		}
		else
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		dispatcher.query(syms_pane, pobj_id, 
			"print -b %s\n", (char *)*buf);
		while( (m = dispatcher.get_response()) != 0)
		{
			switch(m->get_msg_id())
			{
			case MSG_print_val:
                		m->unbundle(value);
				if (value && *value)
					// strip new-line
					value[strlen(value)-1] = '\0';
				break;

			case ERR_type_assumed:
				// ignore
				break;

			case ERR_id_qualifiers:
			case ERR_func_qualifier:
			case ERR_block_not_in_scope:
				// program variable went out of scope
				visible = FALSE;
				break;

        		default:
                		if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
                        		display_msg(m);
				else if (!gui_only_message(m))
					syms_pane->parent->get_window_shell()->display_msg(m);
				visible = FALSE;
                		break;
			}
		} // while

		if (visible == FALSE)
			value = "??";

                if (next_row >= pane->get_rows())
                {
                       	pane->insert_row(next_row, Gly_pin, s->name,
				loc, type, value);
                }
                else
                	pane->set_row(next_row, Gly_pin, s->name, loc, type, value);
                next_row++;
	}
	buf_pool.put(buf);
}

void
Pin_list::delete_current()
{
	if (prog_list)
	{
		// remove the entire current prog_list
		prog_list_list.remove(prog_list);
		delete prog_list;
		prog_list = NULL;
	}
}

void
Pin_list::set_current(ProcObj *newp)
{
	Prog_sym_list *list;

	// look for the new one
	for (list = (Prog_sym_list *)prog_list_list.first(); list;
			list = (Prog_sym_list *)prog_list_list.next())
	{
		if (list->get_pobj() == newp)
			break;
	}
	// could be NULL
	prog_list = list;
}

Symbol *
Pin_list::find_sym(char *name, char *loc)
{
	if (name[0] == '%' || name[0] == '$')
		return du_list ? du_list->find(name) : 0;
	return prog_list ? prog_list->find(name, loc) : 0;
}

void
Pin_list::add_sym(char *name, char *loc, char *type)
{
	if (name[0] == '%' || name[0] == '$')
	{
		if (!du_list)
			du_list = new Sym_list();
		du_list->add(name);
	}
	else
	{
		if (!prog_list)
		{
			prog_list = new Prog_sym_list(syms_pane->window_set->current_obj());
			prog_list_list.add(prog_list);
		}
		prog_list->add(name, loc, type);
	}
}

void
Pin_list::remove_sym(char *name, char *loc)
{
	if (name[0] == '%' || name[0] == '$')
		// assert(du_list != NULL)
		du_list->remove(name);
	else
	{
		// assert(prog_list != NULL)
		prog_list->remove(name, loc);
	}
}

static const Column syms_spec[] =
{
	{ LAB_none,	1,	Col_glyph },
	{ LAB_name,	18,	Col_wrap_text },
#ifdef MOTIF_UI
	{ LAB_location,	14,	Col_right_text },
#else
	{ LAB_location,	14,	Col_text },
#endif
	{ LAB_type,	19,	Col_wrap_text },
	{ LAB_value,	19,	Col_wrap_text },
};

Symbols_pane::Symbols_pane(Window_set *ws, Base_window *parent, Box *box,
	Pane_descriptor *pdesc) : PANE(ws, parent, PT_symbols),
					pin_list(this)
{
	sym_dialog = 0;
	symbol_types = resources.get_symbol_types();
	next_row = 0;
	total_selections = 0;
	reading_symbols = 0;
	update_in_progress = 0;
	update_abort = FALSE;
	pane = new Table(box, "symbols", SM_multiple, syms_spec,
		sizeof(syms_spec)/sizeof(Column), pdesc->get_rows(), 
		FALSE, Search_alphabetic, 1,
		(Callback_ptr)(&Symbols_pane::select_symbol),
		(Callback_ptr)(&Symbols_pane::col_select_symbol),
		(Callback_ptr)(&Symbols_pane::deselect_symbol), 
		(Callback_ptr)(&Symbols_pane::default_cb), 
		0, 
		this, HELP_syms_pane);
	box->add_component(pane, TRUE);
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(pane, parent, 
			pdesc->get_menu_descriptor());
}

Symbols_pane::~Symbols_pane()
{
	delete popup_menu;
	delete sym_dialog;
}

void
Symbols_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this,
		(Notify_func)(&Symbols_pane::update_cb), 0);
	window_set->change_state.add(this,
		(Notify_func)(&Symbols_pane::update_state_cb), 0);
}

void
Symbols_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)(&Symbols_pane::update_cb), 0);
	window_set->change_state.remove(this,
		(Notify_func)(&Symbols_pane::update_state_cb), 0);
}

void
Symbols_pane::setup_syms_cb(Component *, void *)
{
	if (!sym_dialog)
		sym_dialog = new Symbol_dialog(this);
	sym_dialog->display();
}

void
Symbols_pane::set_sym_type(int s)
{
	symbol_types = s;
	update_cb(0, RC_change_state, 0, window_set->current_obj());
}

// used for newly selected row
void
Symbols_pane::select_symbol(Table *, Table_calldata *tdata)
{
	total_selections++;
	parent->set_selection(this);
#ifdef MOTIF_UI
	const char *string;
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

// default action is to bring up show values on the symbol
void
Symbols_pane::default_cb(Table *, Table_calldata *tdata)
{
	char			*expr;
	Show_value_dialog	*show;

	show = window_set->get_show_value_dialog(parent);
	expr = makestr(pane->get_cell(tdata->index, 1).string);
	show->set_plist(parent, window_set->get_command_level());
	show->set_expression(truncate_selection(expr));
	show->show_value(0, 0);
	show->display();
	delete expr;
}

// used when new column within currently selected row is selected
void
Symbols_pane::col_select_symbol(Table *, Table_calldata *tdata)
{
#ifdef MOTIF_UI
	const char *string;
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

void
Symbols_pane::deselect_symbol(Table *, int)
{
	if (total_selections)
	{
		total_selections--;
		parent->set_selection(total_selections ? this : 0);
	}
}

Selection_type
Symbols_pane::selection_type()
{
	return SEL_symbol;
}

void
Symbols_pane::deselect()
{
	total_selections = 0;
	pane->deselect_all();
}

void
Symbols_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
	{
		pane->set_sensitive(FALSE);
	}
	parent->set_sensitivity();
}

void
Symbols_pane::clear()
{
	next_row = 0;
	pane->clear();
	if (total_selections)
	{
		// don't clear Base_window selection unless
		// it came from this pane (total selection != 0)
		total_selections = 0;
		parent->set_selection(0);
	}
}

void
Symbols_pane::update_cb(void *, Reason_code rc, void *, ProcObj *call_data)
{
	char		buf[sizeof("-tv -l -g -f -d -u")];

	if (rc == RC_rename)
		return;

	if (rc == RC_delete)
	{
		// delete is followed by either set_current of new pobj
		// or set_current of 0
		pin_list.delete_current();
		return;
	}

	if (!call_data)
	{
		// no more procobjs
		// assert (pin_list does not have any prog syms)
		if (!(symbol_types&(SYM_debugger|SYM_user)) && 
		    pin_list.is_empty())
		{
			clear();
			return;
		}

		next_row = 0;
		if (!pin_list.is_empty())
			// assert (pin_list has only debugger & user symbols)
			// don't delay update finish since we're not going to 
			// issue any symbols command
			pin_list.update(FALSE);
		else
		{
			parent->inc_busy();
			strcpy(buf, "-tv");
			if (symbol_types&SYM_debugger)
				strcat(buf, " -d");
			if (symbol_types&SYM_user)
				strcat(buf, " -u");
			pane->set_sensitive(FALSE);
			dispatcher.send_msg(this, 0, "symbols %s\n", buf);
			++update_in_progress;
		}
		return;
	}

	if (call_data->is_animated() || in_script)
	{
		if (rc == RC_animate)
			pane->set_sensitive(FALSE);
		return;
	}

	if (rc == RC_set_current)
	{
		clear();
		pin_list.set_current(call_data);
	}

	if (!symbol_types && pin_list.is_empty())
	{
		if (rc != RC_set_current)
		{
			clear();
		}
		return;
	}

	if (call_data->is_incomplete())
	{
		pane->set_sensitive(FALSE);
		return;
	}

	if (call_data->is_running())
	{
		if (update_in_progress)
			// when a process or thread is set running while
			// a symbols pane update is in progress (this should
			// only happen currently when a thread is restarted
			// automatically when its state changes), we need to
			// "cancel" the update, otherwise the rest of the 
			// update will again sensitize the pane
			update_abort = TRUE;
		pane->set_sensitive(FALSE);
		return;
	}

	// when a process aborts with corrupted text space,
	// debug will complain on "symbols" queries
	if (call_data->in_bad_state())
	{
		clear();
		pin_list.delete_current();
		return;
	}

	if (!update_in_progress)
	{
		next_row = 0;
		parent->inc_busy();
		pin_list.update(TRUE);	// delay update finish until the 
					// end of the "syms" command
		strcpy(buf, "-tv");

		if (symbol_types&SYM_local)
			strcat(buf, " -l");
		if (symbol_types&SYM_global)
			strcat(buf, " -g");
		if (symbol_types&SYM_file)
			strcat(buf, " -f");
		if (symbol_types&SYM_debugger)
			strcat(buf, " -d");
		if (symbol_types&SYM_user)
			strcat(buf, " -u");

		dispatcher.send_msg(this, call_data->get_id(), "symbols %s\n", buf);
		if (total_selections)
		{
			total_selections = 0;
			parent->set_selection(0);
			pane->deselect_all();
		}
		++update_in_progress;
	}
	update_abort = FALSE;
}

void
Symbols_pane::cmd_complete()
{
	if (!reading_symbols)
	{
		if (update_in_progress)
		{
			--update_in_progress;
			if (!update_in_progress && update_abort)
				// updates done
				update_abort = FALSE;
		}
		return;
	}

	if (!update_abort)
	{
		int	cur_rows = pane->get_rows();

		if (!next_row)
		{
			total_selections = 0;
			pane->clear();
		}
		else if (next_row < cur_rows)
			pane->delete_rows(next_row, cur_rows - next_row);
		if (pane->is_delayed())
			pane->finish_updates();
	}
	if (update_in_progress)
	{
		--update_in_progress;
		if (!update_in_progress && update_abort)
			// updates done
			update_abort = FALSE;
	}
	parent->dec_busy();
	reading_symbols = 0;
}

void
Symbols_pane::de_message(Message *m)
{
	static	const char	*assumed;
	static	char		*assumed_buf;

	char	loc[MAXPATHLEN+1+MAX_LONG_DIGITS+1];
	char	*type = 0;
	char	*name = 0;
	char	*file = 0;
	char	*line = 0;
	char	*value = 0;

	if (!assumed)
	{
		assumed = labeltab.get_label(LAB_assumed);
		assumed_buf = new char[sizeof("int (...) ") + strlen(assumed)];
	}

	switch(m->get_msg_id())
	{
	case MSG_sym_header:
	case MSG_sym_type_header:
	case MSG_sym_val_header:
	case MSG_sym_type_val_header:
		reading_symbols = 1;
		if (total_selections)
		{
			total_selections = 0;
			parent->set_selection(0);
		}
		break;

	case MSG_symbol_type_val:
	case MSG_symbol_type_val_assume:
		if (update_abort)
			break;
		if (!pane->is_delayed())
        		pane->delay_updates();
		m->unbundle(name, file, line, type, value);
		if (m->get_msg_id() == MSG_symbol_type_val_assume)
		{
			// type may be either "int" (for object) or "int (...)"
			// for function.  The word "assumed" may be translated,
			// the type isn't
			sprintf(assumed_buf, "%s %s", type, assumed);
			type = assumed_buf;
		}
		// check to see if it's already handled by pin list
		if (line && *line)
			sprintf(loc, "%s@%s", file, line);
		else
			strcpy(loc, file);
		if (pin_list.find_sym(name, loc))
			break; 

		if (next_row >= pane->get_rows())
		{
			pane->insert_row(next_row, Gly_blank, name, loc, type, value);
		}
		else
			pane->set_row(next_row, Gly_blank, name, loc, type, value);
		next_row++;
		break;

	case ERR_short_read:
	case ERR_get_addr:
	case ERR_proc_read:
		break;

	default:
		if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
			display_msg(m);
		else if (!gui_only_message(m))
			parent->get_window_shell()->display_msg(m);
		break;
	}
}

int
Symbols_pane::get_selections(Vector *rvec)
{
	int	*sel;
	Vector	*v = vec_pool.get();

	if (total_selections < 0 || pane->get_selections(v) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return 0;
	}

	Buffer	*buf = buf_pool.get();
	rvec->clear();
	sel = (int *)v->ptr();
	for (int i = 0; i < total_selections; i++, sel++)
	{
		fully_qualified_name(pane->get_cell(*sel, SY_loc).string,
					pane->get_cell(*sel,SY_name).string,
					buf);
		char	*s = makestr((char *)*buf);
		rvec->add(&s, sizeof(char *));
	}
	vec_pool.put(v);
	return total_selections;
}

void
Symbols_pane::export_syms_cb(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (total_selections < 0 || pane->get_selections(v) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}

	int	*sel = (int *)v->ptr();
	for (int i = 0; i < total_selections; i++, sel++)
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"export %s\n", pane->get_cell(*sel,SY_name).string);
	vec_pool.put(v);
}

int
Symbols_pane::check_sensitivity(Sensitivity *sense)
{
	if (sense->sel_required() &&
		!sense->symbol_sel())
		// wrong selection type
		return 0;

	if (sense->pin_sym() && pin_list.is_empty())
		return 0;

	Vector	*v = vec_pool.get();
	int	total = pane->get_selections(v);
	int	*sel_list = (int *)v->ptr();
	Boolean	sym_ok = TRUE;
	int	pins = 0;

	if (sense->single_sel() && total > 1)
	{
		vec_pool.put(v);
		return 0;
	}

	// if the selected symbols are inappropriate
	// then the button is insensitive
	for (int i = 0; i < total; i++, sel_list++)
	{
		int row = *sel_list;
		char c = pane->get_cell(row, SY_name).string[0];

		if (sense->user_sym())
		{ 
			if (c != '$')
			{
				sym_ok = FALSE;
				break;
			}
		}
		else if (sense->program_sym())
		{
			if (c == '$' || c == '%')
			{
				sym_ok = FALSE;
				break;
			}
		}
		if ((sense->pin_sym() || sense->unpin_sym()) &&
		    pane->get_cell(row, SY_pin).glyph == Gly_pin)
			++pins;
	}
	vec_pool.put(v);
	if (!sym_ok)
		return 0;
	if (sense->pin_sym() && !pins)
		return 0;
	if (sense->unpin_sym() && pins == total)
		return 0;
	return 1;
}

void
Symbols_pane::set_watchpoint_cb(Component *, void *)
{
	Vector	*v = vec_pool.get();
	int	total = get_selections(v);
	char	**syms = (char **)v->ptr();

	for (int i = 0; i < total; i++)
	{
		if (*syms[i] == '$' || *syms[i] == '%')
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		ProcObj *proc = window_set->current_obj();
		int	lev = window_set->get_event_level();
		dispatcher.send_msg(this, proc->get_id(),
			"stop -p %s *%s\n", 
			lev == PROGRAM_LEVEL ? 
				proc->get_program()->get_name() : 
				(lev == PROCESS_LEVEL ?
					proc->get_process()->get_name() :
					proc->get_name()), 
			syms[i]);
	}
	vec_pool.put(v);
}

void
Symbols_pane::pin_sym_cb(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (total_selections < 0 || pane->get_selections(v) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}
	int *ip = (int *)v->ptr();
	for (int nsel = 0; nsel < total_selections; ++nsel, ++ip)
	{
		int row = *ip;
		if (pane->get_cell(row, SY_pin).glyph == Gly_pin)
			continue;
		pane->set_cell(row, SY_pin, Gly_pin);
		char	*name = (char *)pane->get_cell(row, SY_name).string;
		char	*loc = (char *)pane->get_cell(row, SY_loc).string;
		char	*type = (char *)pane->get_cell(row, SY_type).string;
		pin_list.add_sym(name, loc, type);
	}
	vec_pool.put(v);
	parent->set_sensitivity();
}

void
Symbols_pane::unpin_sym_cb(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (total_selections < 0 || pane->get_selections(v) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}
	int *ip = (int *)v->ptr();
	for (int nsel = 0; nsel < total_selections; ++nsel, ++ip)
	{
		int row = *ip;
		if (pane->get_cell(row, SY_pin).glyph == Gly_blank)
			continue;
		pane->set_cell(row, SY_pin, Gly_blank);
		char	*name = (char *)pane->get_cell(row, SY_name).string;
		char	*loc = (char *)pane->get_cell(row, SY_loc).string;
		pin_list.remove_sym(name, loc);
	}
	vec_pool.put(v);
	parent->set_sensitivity();
}

#define	GLOBAL_TOGGLE	0
#define FILE_TOGGLE	1
#define	LOCAL_TOGGLE	2

#define	DEBUGGER_TOGGLE	0
#define	USER_TOGGLE	1

Symbol_dialog::Symbol_dialog(Symbols_pane *p)
	: DIALOG_BOX(p->get_parent())
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none,  LAB_none, 
			(Callback_ptr)(&Symbol_dialog::apply) },
		{ B_apply, LAB_none,  LAB_none, 
			(Callback_ptr)(&Symbol_dialog::apply) },
		{ B_reset, LAB_none,  LAB_none, 
			(Callback_ptr)(&Symbol_dialog::reset) },
		{ B_cancel, LAB_none,  LAB_none, 
			(Callback_ptr)(&Symbol_dialog::reset) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static Toggle_data toggle1[] =
	{
		{ LAB_global,	FALSE,	0 },
		{ LAB_file,	FALSE,	0 },
		{ LAB_local,	FALSE,	0 },
	};

	static Toggle_data toggle2[] =
	{
		{ LAB_debugger,	FALSE,	0 },
		{ LAB_user,	FALSE,	0 },
	};

	int		stypes;
	Packed_box	*box;

	parent = p;

	// setup initial types from resource
	stypes = parent->get_sym_type();
	if (stypes & SYM_global)
		toggle1[GLOBAL_TOGGLE].state = TRUE;
	if (stypes & SYM_file)
		toggle1[FILE_TOGGLE].state = TRUE;
	if (stypes & SYM_local)
		toggle1[LOCAL_TOGGLE].state = TRUE;
	if (stypes & SYM_debugger)
		toggle2[DEBUGGER_TOGGLE].state = TRUE;
	if (stypes & SYM_user)
		toggle2[USER_TOGGLE].state = TRUE;

	dialog = new Dialog_shell(p->get_parent()->get_window_shell(),
		LAB_symbols, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_symbols_dialog);
	box = new Packed_box(dialog, "symbols box", OR_horizontal);

	program_settings = new Toggle_button(box, "program symbols", toggle1,
		sizeof(toggle1)/sizeof(Toggle_data), OR_vertical, this);
	box->add_component(program_settings, TRUE);

	debug_settings = new Toggle_button(box, "debug symbols", toggle2,
		sizeof(toggle2)/sizeof(Toggle_data), OR_vertical, this);
	box->add_component(debug_settings, FALSE);
	box->update_complete();
	dialog->add_component(box);
}

void
Symbol_dialog::apply(Component *, void *)
{
	int	state;

	state = 0;
	if (program_settings->is_set(GLOBAL_TOGGLE))
		state |= SYM_global;
	if (program_settings->is_set(FILE_TOGGLE))
		state |= SYM_file;
	if (program_settings->is_set(LOCAL_TOGGLE))
		state |= SYM_local;
	if (debug_settings->is_set(DEBUGGER_TOGGLE))
		state |= SYM_debugger;
	if (debug_settings->is_set(USER_TOGGLE))
		state |= SYM_user;
	parent->set_sym_type(state);
}

void
Symbol_dialog::reset(Component *, void *)
{
	int	state = parent->get_sym_type();

	program_settings->set(GLOBAL_TOGGLE, (state&SYM_global) != 0);
	program_settings->set(FILE_TOGGLE, (state&SYM_file) != 0);
	program_settings->set(LOCAL_TOGGLE, (state&SYM_local) != 0);
	debug_settings->set(DEBUGGER_TOGGLE, (state&SYM_debugger) != 0);
	debug_settings->set(USER_TOGGLE, (state&SYM_user) != 0);
}
