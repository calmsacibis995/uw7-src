#ident	"@(#)debugger:gui.d/common/Ps_pane.C	1.27"

// GUI headers
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Boxes.h"
#include "Ps_pane.h"
#include "UI.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Windows.h"
#include "Menu.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "Window_sh.h"
#include "Sense.h"
#include "config.h"
#include "gui_label.h"

#include "UIutil.h"
#include "Vector.h"

enum Cell { CURRENT, PROGRAM, ID, STATE, FUNCTION, LOCATION, COMMAND };

static const Column ps_spec[] =
{
	{ LAB_none,	1,	Col_glyph },
	{ LAB_program,	8,	Col_text },
	{ LAB_id,	6,	Col_text },
#ifdef DEBUG_THREADS
	{ LAB_state,	12,	Col_text },	// need enough room for "Core off lwp"
#else
	{ LAB_state,	8,	Col_text },
#endif
	{ LAB_function,	12,	Col_text },
#ifdef MOTIF_UI
	{ LAB_location,	18,	Col_right_text },
#else
	{ LAB_location,	18,	Col_text },
#endif
	{ LAB_command,	12,	Col_wrap_text },
};

Ps_pane::Ps_pane(Window_set *ws, Base_window *parent, Box *box, Pane_descriptor *pdesc)
	: PANE(ws, parent, PT_process)
{
	window_set = ws;
	total_selections = 0;

	pane = new Table(box, "programs", SM_multiple, ps_spec,
		sizeof(ps_spec)/sizeof(Column), pdesc->get_rows(), 
		FALSE, Search_none, 0,
		(Callback_ptr)(&Ps_pane::select_cb),
		(Callback_ptr)(&Ps_pane::col_select_cb),
		(Callback_ptr)(&Ps_pane::deselect_cb),
		(Callback_ptr)(&Ps_pane::default_cb),
		(Callback_ptr)(&Ps_pane::drop_proc), 
		this, HELP_ps_pane);
	box->add_component(pane, TRUE);

	Plink	*link;
	int	i;
	for (link = ws->get_obj_list(), i = 0; link; link = link->next(), i++)
		add_obj(i, link->procobj());
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(pane, parent, 
			pdesc->get_menu_descriptor());
}

Ps_pane::~Ps_pane()
{
	delete popup_menu;
}

void
Ps_pane::add_process(int slot, Process *ptr)
{
	if (total_selections)
	{
		pane->deselect_all();
		total_selections = 0;
		parent->set_selection(0);
	}
	if (!ptr->has_threads())
	{
		pane->insert_row(slot,
			(ptr == window_set->current_obj()) ? Gly_hand : Gly_blank,
			ptr->get_program()->get_name(),
			ptr->get_name(),
			ptr->get_state_string(),
			ptr->get_function(),
			ptr->get_location(),
			ptr->get_program()->get_cmd_line());
		return;
	}
#ifdef DEBUG_THREADS
	Boolean	delay_update = FALSE;
	Plink	*link = ptr->get_head();

	if (link->next())
	{
		// >1 threads, try to avoid display churn
		delay_update = TRUE;
		pane->delay_updates();
	}
	// add all children
	do
	{
		Thread *thr = link->thread();
		pane->insert_row(slot,
			(link->procobj() == window_set->current_obj()) ? 
				Gly_hand : Gly_blank,
			thr->get_program()->get_name(),
			thr->get_name(),
			thr->get_state_string(),
			thr->get_function(),
			thr->get_location(),
			thr->get_program()->get_cmd_line());
		++slot;
		link = link->next();
	} while(link);
	if (delay_update)
		pane->finish_updates();
#endif
}

void
Ps_pane::add_obj(int slot, ProcObj *ptr)
{
	pane->insert_row(slot,
		(ptr == window_set->current_obj()) ? Gly_hand : Gly_blank,
		ptr->get_program()->get_name(),
		ptr->get_name(),
		ptr->get_state_string(),
		ptr->get_function(),
		ptr->get_location(),
		ptr->get_program()->get_cmd_line());
}

void
Ps_pane::delete_obj(int slot)
{
	if (total_selections)
	{
		pane->deselect_all();
		total_selections = 0;
		parent->set_selection(0);
	}
	pane->delete_rows(slot, 1);
}

void
Ps_pane::update_obj(Reason_code rc, int slot, ProcObj *ptr)
{
	if (!ptr)
		return;

	if (rc == RC_rename)
	{
		pane->set_cell(slot, PROGRAM, ptr->get_program()->get_name());
		return;
	}
	else if (rc == RC_animate)
	{
		pane->set_row(slot, Gly_hand,
			ptr->get_program()->get_name(),
			ptr->get_name(),
			"Stepping",
			0, 0,
			ptr->get_program()->get_cmd_line());
		parent->set_sensitivity();
		return;
	}
	else if (ptr->is_animated())
	{
		return;
	}

	pane->set_row(slot, (ptr == window_set->current_obj())
		? Gly_hand : Gly_blank,
		ptr->get_program()->get_name(),
		ptr->get_name(),
		ptr->get_state_string(),
		ptr->get_function(),
		ptr->get_location(),
		ptr->get_program()->get_cmd_line());
	parent->set_sensitivity();
}

void
Ps_pane::set_current(ProcObj *proc)
{
	Plink	*plink = window_set->get_obj_list();
	int	i = 0;

	for (; plink; plink = plink->next())
	{
		if (plink->procobj() == proc)
			// set the new one
			pane->set_cell(i, 0, Gly_hand);
		else if (pane->get_cell(i, 0).glyph == Gly_hand)
			// unset the old one
			pane->set_cell(i, 0, Gly_blank);
		i++;
	}
	parent->set_sensitivity();
}

// called for new row selected
void
Ps_pane::select_cb(Table *, Table_calldata *tdata)
{
	total_selections++;
	parent->set_selection(this);
#ifdef MOTIF_UI
	const char *string;
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

// called when new column within already selected row is selected
// does not affect total_selections
void
Ps_pane::col_select_cb(Table *, Table_calldata *tdata)
{
#ifdef MOTIF_UI
	const char *string;
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

void
Ps_pane::deselect_cb(Table *, void *)
{
	if (total_selections > 0)
	{
		total_selections--;
		parent->set_selection(total_selections ? this : 0);
	}
}

void
Ps_pane::default_cb(Table *, const Table_calldata *call)
{
	ProcObj	*current = window_set->current_obj();
	ProcObj	*new_current = 0;
	Plink	*plink = window_set->get_obj_list();
	int	index = 0;

	for(; plink; plink = plink->next(), ++index)
	{
		if (index == call->index)
			new_current = plink->procobj();
	}
	if (current != new_current)
		window_set->set_current(new_current);
}

void
Ps_pane::drop_proc(Table *, const Table_calldata *data)
{
	if (!data || !data->dropped_on || !data->dropped_on->get_creator())
		return;

	Window_set *ws = ((Command_sender *)data->dropped_on->get_creator())->get_window_set();
	Plink	*plink = window_set->get_obj_list();
	int	index = 0;

	for(; plink; plink = plink->next(), ++index)
	{
		if (index == data->index)
			break;
	}
	if (plink)
	{
		ProcObj	*pptr[1];
		pptr[0] = plink->procobj();
		proclist.move_objs(pptr, 1, ws);
	}
}

void
Ps_pane::deselect()
{
	total_selections = 0;
	pane->deselect_all();
}

Selection_type
Ps_pane::selection_type()
{
	return SEL_process;
}

int
Ps_pane::get_selections(Vector *rvec)
{
	Plink	*plink = window_set->get_obj_list();
	int	*sel;
	int	pindex = 0;
	Vector	*v = vec_pool.get();

	if (total_selections <= 0 || pane->get_selections(v) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return 0;
	}

	// the selection array is a sorted array of indices into the process 
	// list, so walk the process list to pick out the indexed ones and 
	// collect them in a vector
	rvec->clear();
	sel = (int *)v->ptr();
	for (int i = 0; i < total_selections; i++, sel++)
	{
		while (pindex < *sel && plink)
		{
			pindex++;
			plink = plink->next();
		}
		if (!plink)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			vec_pool.put(v);
			return 0;
		}
		ProcObj *p = plink->procobj();
		rvec->add(&p, sizeof(ProcObj *));
	}
	vec_pool.put(v);
	return total_selections;
}

int
Ps_pane::check_sensitivity(Sensitivity *sense)
{
	if (sense->sel_required() &&
		!sense->process_sel())
		// wrong selection type
		return 0;
	// applies to selected pobjs
	Vector	*v = vec_pool.get();
	int	total = get_selections(v);
	ProcObj	**plist = (ProcObj **)v->ptr();

	// must have a single process selected?
	if (sense->single_sel() && total > 1)
	{
		vec_pool.put(v);
		return 0;
	}
#ifdef DEBUG_THREADS
	// threads selected are all threads of parent proc
	if (sense->proc_only()  && 
		window_set->get_command_level() == THREAD_LEVEL &&
		get_full_procs(plist, total) == 0)
	{
		vec_pool.put(v);
		return 0;
	}
#endif
	for (int i = 0; i < total; i++, plist++)
	{
		if (!(*plist)->check_sensitivity(sense))
		{
			vec_pool.put(v);
			return 0;
		}
	}
	vec_pool.put(v);
	return 1;
}
