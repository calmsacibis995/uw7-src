#ident	"@(#)debugger:gui.d/common/Queries.C	1.16"

// Debug headers
#include "Message.h"
#include "Msgtypes.h"
#include "Vector.h"
#include "gui_label.h"

// GUI headers
#include "Boxes.h"
#include "config.h"
#include "Base_win.h"
#include "Component.h"
#include "Dialogs.h"
#include "Dispatcher.h"
#include "Sel_list.h"
#include "Stext.h"
#include "UI.h"
#include "Windows.h"
#include "Window_sh.h"

#include <string.h>
#include <stdio.h>

// Query_dialog handles debugger queries about overloaded functions
// It displays a scrolling list of the overloaded function names
// and asks the user to pick one

class Query_dialog : public Dialog_box
{
	Selection_list	*choices;
	DBcontext	dbcontext;
	Simple_text	*text;
	int		nfuncs;
	Boolean		response_sent;

public:
			Query_dialog(Base_window *);
			~Query_dialog() {}

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		select_all(Component *, void *);
	void		default_cb(Component *, int);

			// initialize the scrolling list
	void		set_list(Message *);
};

Query_dialog::Query_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_exec, LAB_select, LAB_select_mne, 
			(Callback_ptr)(&Query_dialog::apply), },
		{ B_exec, LAB_select_all, LAB_select_all_mne, 
			(Callback_ptr)(&Query_dialog::select_all), },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Query_dialog::cancel), },
		{ B_help, LAB_none, LAB_none, 0},
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
	};

	Expansion_box	*box;
	const char	*initial_string = "";

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_overload_func, 
		(Callback_ptr)(&Query_dialog::cancel), 
		this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_none);
	box = new Expansion_box(dialog, "box", OR_vertical);

	text = new Simple_text(box, "", TRUE);
	box->add_component(text);

	choices = new Selection_list(box, "function_list", 10, 1, 
		sel_col, 1, &initial_string, SM_single, this,
		Search_none, 0,
		0, 0,
		(Callback_ptr)(&Query_dialog::default_cb));
	box->add_component(choices, TRUE);
	box->update_complete();
	dialog->add_component(box);
	nfuncs = 0;
	response_sent = FALSE;
}

void
Query_dialog::set_list(Message *m)
{
	char		buf[BUFSIZ];
	char		*name;
	char		*functions;
	char		*fptr;
	char		*p;
	Vector		*v = vec_pool.get();
	Word		n;

	dbcontext = m->get_dbcontext();
	response_sent = FALSE;

	if (m->get_msg_id() == QUERY_which_function)
	{
		m->unbundle(name, functions);
		dialog->set_sensitive(LAB_select_all, FALSE);
	}
	else
	{
		// QUERY_which_func_or_all allows selection of
		// "all of the above functions"
		m->unbundle(name, functions, n);
		dialog->set_sensitive(LAB_select_all, TRUE);
	}

	sprintf(buf, gm_format(GM_overloaded), name);
	text->set_text(buf);

	// Create the list of functions.  This assumes the list is one string in
	// the form: number\tfunction\n ...
	v->clear();
	nfuncs = 0;
	p = functions;
	while (*p)
	{
		fptr = strchr(p, '\t') + 1;
		p = strchr(fptr, '\n');
		*p++ = '\0';
		v->add(&fptr, sizeof(char *));
		++nfuncs;
	}
	choices->set_list(nfuncs, (const char **)v->ptr());
	vec_pool.put(v);
}

void
Query_dialog::apply(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (!choices->get_selections(v))
	{
		dialog->error(E_ERROR, GE_no_function);
		vec_pool.put(v);
		return;
	}

	dispatcher.send_response(dbcontext, *(int *)v->ptr() + 1);
	vec_pool.put(v);
	response_sent = TRUE;
}

void
Query_dialog::default_cb(Component *,int item_index)
{
	dialog->default_start();
	dispatcher.send_response(dbcontext, item_index + 1);
	response_sent = TRUE;
	dialog->default_done();
}

void
Query_dialog::select_all(Component *, void *)
{
	dispatcher.send_response(dbcontext, nfuncs+1);
	response_sent = TRUE;
}

// since this is the dismiss callback, it gets called whenever
// the window pops down, even after a select or select all
// make sure we don't send a response twice
void
Query_dialog::cancel(Component *, void *)
{
	if (response_sent)
	{
		return;
	}
	dispatcher.send_response(dbcontext, 0);
	response_sent = TRUE;
}

// Object used to handle simple yes-or-no queries
class Query_object : public Command_sender
{
	DBcontext	dbcontext;
public:
		Query_object(Window_set *ws, DBcontext ctxt) : Command_sender(ws)
					{ dbcontext = ctxt; }
	void	yes_or_no(Component *, int);
};

void
Query_object::yes_or_no(Component *, int yorn)
{
	dispatcher.send_response(dbcontext, yorn);
}

// called by Transport layer when CLI queries are received
void
query_handler(Message *m)
{
	static Query_dialog *query_box;
	Window_set	*ws;
	Base_window	*bw;
	Command_sender	*sender;

	sender = (Command_sender *)m->get_uicontext();
	if (!sender || ((bw = sender->get_window()) == 0))
	{
		ws = (Window_set *)windows.first();
		bw = ws->get_window();
	}
	switch(m->get_msg_id())
	{
	case QUERY_which_function:
	case QUERY_which_func_or_all:
		if (!query_box)
			query_box = new Query_dialog(bw);
		query_box->set_list(m);
		query_box->display();
		break;

	case QUERY_search_for_header:
	case QUERY_search_for_all_funcs:
	case QUERY_set_break_on_all:
	case QUERY_search_for_all:
	{
		Query_object	qobj(ws, m->get_dbcontext());
		display_msg((Callback_ptr)(&Query_object::yes_or_no), &qobj,
			LAB_yes, LAB_no, m);
	}
		break;

	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}
