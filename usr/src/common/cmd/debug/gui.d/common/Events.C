#ident	"@(#)debugger:gui.d/common/Events.C	1.50"

// GUI headers
#include "Dispatcher.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Events.h"
#include "Eventlist.h"
#include "Window_sh.h"
#include "Windows.h"
#include "Menu.h"
#include "Sense.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Status.h"
#include "Caption.h"
#include "Radio.h"
#include "gui_label.h"
#include "config.h"

// debug headers
#include "Buffer.h"
#include "Machine.h"
#include "Message.h"
#include "str.h"
#include "Vector.h"

#include <stdio.h>

enum TableCol { COL_ID, COL_STATE, COL_TYPE, COL_PROC, COL_COND, COL_COUNT,
		COL_COMMAND };

static const Column	event_spec[] =
{
	{ LAB_sp_id,		4,	Col_numeric },
	{ LAB_none,		1,	Col_text },
	{ LAB_type,		12,	Col_text },
	{ LAB_processes,	9,	Col_text },
	{ LAB_condition,	21,	Col_text },
	{ LAB_count,		5,	Col_numeric },
	{ LAB_command_list,	12,	Col_wrap_text },
};

static const Column	onstop_spec[] =
{ 
	{ LAB_sp_id,		4,	Col_numeric },
	{ LAB_none,		1,	Col_text },
	{ LAB_processes,	9,	Col_text },
	{ LAB_command_list,	39,	Col_text },
};

Event_pane::Event_pane(Window_set *ws, Base_window *parent, Box *box,
	Pane_descriptor *pdesc) : PANE(ws, parent, PT_event)
{
	max_events = max_onstop = 0;
	ableEventSel = 0;
	disableEventSel = 0;

	events = new Table(box, "events", SM_multiple, event_spec,
		sizeof(event_spec)/sizeof(Column), pdesc->get_rows(), 
			FALSE, Search_none, 0,
		(Callback_ptr)(&Event_pane::selectEventCb),
		(Callback_ptr)(&Event_pane::col_selectEventCb),
		(Callback_ptr)(&Event_pane::deselectEventCb), 
		0,
		0, 
		this, HELP_event_pane);
	box->add_component(events, TRUE);
	onstop = new Table(box, "onstop", SM_multiple, onstop_spec,
		sizeof(onstop_spec)/sizeof(Column), pdesc->get_rows()/2, 
		FALSE, Search_none, 0,
		(Callback_ptr)(&Event_pane::selectEventCb),
		(Callback_ptr)(&Event_pane::col_selectEventCb),
		(Callback_ptr)(&Event_pane::deselectEventCb), 
		0,
		0, 
		this, HELP_event_pane);
	box->add_component(onstop, TRUE);
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(box, parent, 
			pdesc->get_menu_descriptor());

}

Event_pane::~Event_pane()
{
	delete popup_menu;
	window_set->change_current.remove(this, (Notify_func)(&Event_pane::update_cb), 0);
	window_set->change_state.remove(this, (Notify_func)(&Event_pane::update_state_cb), 0);
}

void
Event_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this, (Notify_func)(&Event_pane::update_cb), 0);
	window_set->change_state.add(this, (Notify_func)(&Event_pane::update_state_cb), 0);
}

void
Event_pane::popdown()
{
	window_set->change_current.remove(this, (Notify_func)(&Event_pane::update_cb), 0);
	window_set->change_state.remove(this, (Notify_func)(&Event_pane::update_state_cb), 0);
}

int
Event_pane::contains(int e)
{
	int i;

	for (i = 0; i < max_events; i++)
	{
		const char	*p = events->get_cell(i, COL_ID).string;

		if (atoi(p) == e) return 1; 
	}

	for (i = 0; i < max_onstop; i++)
	{
		const char	*p = onstop->get_cell(i, COL_ID).string;

		if (atoi(p) == e) return 1; 
	}

	return 0;
}

// used for new row selected
void
Event_pane::selectEventCb(Table *table, Table_calldata *tdata)
{
	const char	*string;
	const char *state = table->get_cell(tdata->index, COL_STATE).string;

	if (state && *state == 'D')
		disableEventSel++;
	else
		ableEventSel++;

	parent->set_selection(this);

#ifdef MOTIF_UI
	// display full string in window footer
	string = table->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

// used when new column within already selected row is selected
void
Event_pane::col_selectEventCb(Table *table, Table_calldata *row)
{
#ifdef MOTIF_UI
	const char *string;
	string = table->get_cell(row->index, row->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
#endif
}

void
Event_pane::deselectEventCb(Table *table, int row)
{
	const char *state = table->get_cell(row, COL_STATE).string;

	if (state && *state == 'D')
	{
		if (disableEventSel)
		{
			disableEventSel--;
			parent->set_selection((ableEventSel + disableEventSel) ? this : 0);
		}
	}
	else
	{
		if (ableEventSel)
		{
			ableEventSel--;
			parent->set_selection((ableEventSel + disableEventSel) ? this : 0);
		}
	}
}

void
Event_pane::deselect()
{
	disableEventSel = ableEventSel = 0;
	events->deselect_all();
	onstop->deselect_all();
}

Selection_type
Event_pane::selection_type()
{
	return SEL_event;
}

int
Event_pane::check_sensitivity(Sensitivity *sense)
{
	if (!sense->event_sel())
		// wrong selection type
		return 0;
	if (sense->single_sel())
		 return ((ableEventSel + disableEventSel) == 1);
	if (sense->event_enable())
		return((ableEventSel > 0) && (disableEventSel == 0)); 
	if (sense->event_disable())
		return((disableEventSel > 0) && (ableEventSel == 0)); 
	return ((disableEventSel > 0) || ( ableEventSel > 0)); 
}

void
Event_pane::add_event(Event *event)
{
	Buffer	*plist = buf_pool.get();
	char	id[MAX_INT_DIGITS + 1];
	char	buf[MAX_INT_DIGITS + 1];
	char	state[2];
	int	i;

	sprintf(id, "%d", event->get_id());
	state[0] = state[1] = 0;
	buf[0] = 0;

	event->get_plist(plist);
	if (event->get_state() == ES_disabled)
		state[0] = 'D';

	if (event->get_type() == ET_onstop)
	{
		for (i = 0; i < max_onstop; i++)
		{
			int eid = atoi(onstop->get_cell(i, COL_ID).string);
			if (event->get_id() < eid)
				break;
		}
		onstop->deselect_all();
		onstop->insert_row(i, id, state, (char *)*plist,
			event->get_commands());
		max_onstop++;
	}
	else
	{
		switch(event->get_type())
		{
			case ET_stop:
				if (((Stop_event *)event)->get_count() > 1)
					sprintf(buf,"%d",((Stop_event *)event)->get_count());
				break;
			case ET_syscall:
				if (((Syscall_event *)event)->get_count() > 1)
					sprintf(buf,"%d",((Syscall_event *)event)->get_count());
				break;
		}

		for (i = 0; i < max_events; i++)
		{
			int eid = atoi(events->get_cell(i, COL_ID).string);
			if (event->get_id() < eid)
				break;
		}
		events->deselect_all();
		events->insert_row(i, id, state, 
			event->get_type_string(),
			(char *)*plist,
 			event->get_condition(), 
			buf,
			event->get_commands());
		max_events++;
	}
	buf_pool.put(plist);

	// recalculate the disabled and enabled event counts
	disableEventSel = ableEventSel = 0;
	count_selections(events);
	count_selections(onstop);
	parent->set_sensitivity();
}

void
Event_pane::change_event(Event *p)
{
	Table	*t	= 0;
	int	max	= 0;
	char	buf[MAX_INT_DIGITS + 1];
	char	state[2];

	state[0] = state[1] = 0;
	buf[0] = 0;

	if (p->get_state() == ES_disabled)
		state[0] = 'D';

	switch(p->get_type())
	{
		case ET_stop:
		case ET_signal:
		case ET_syscall:
#if EXCEPTION_HANDLING
		case ET_exception:
#endif
			t = events;
			max = max_events;
			break;

		case ET_onstop:
			t = onstop;
			max = max_onstop;
			break;

		case ET_none:
		default:
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}

	Buffer	*plist = buf_pool.get();
	for (int i = 0; i < max; i++)
	{
		Cell_value item = t->get_cell(i, COL_ID);
		int idInt = atoi(item.string);
	
		if (p->get_id() == idInt)
		{
			const char	*id = item.string;

			p->get_plist(plist);
			if (p->get_type() == ET_onstop)
			{
				t->set_row(i, id, state, (char *)*plist,
						p->get_commands());
			}
			else
			{
				switch(p->get_type())
				{
					case ET_stop:
						if (((Stop_event *)p)->get_count() > 1)
							sprintf(buf, "%d",((Stop_event *)p)->get_count());
						break;
					case ET_syscall:
						if (((Syscall_event *)p)->get_count() > 1)
							sprintf(buf, "%d",((Syscall_event *)p)->get_count());
						break;
				}

				t->set_row(i, id, state, p->get_type_string(),
					(char *)*plist,
		 			p->get_condition(), buf,
					p->get_commands());
			}
			break;
		}
	}
	buf_pool.put(plist);

	// recalculate the disabled and enabled event counts
	disableEventSel = ableEventSel = 0;
	count_selections(events);
	count_selections(onstop);
	parent->set_sensitivity();
}

void
Event_pane::count_selections(Table *t)
{
	Vector	*v = vec_pool.get();
	int	total = t->get_selections(v);
	int	*selections = (int *)v->ptr();

	for (int i = 0; i < total; i++, selections++)
	{
		const char *state = t->get_cell(*selections, COL_STATE).string;

		if (state && *state == 'D')
			disableEventSel++;
		else
			ableEventSel++;
	}

	vec_pool.put(v);
}

// find the table 
// find the row
// delete it 
void
Event_pane::delete_event(Event *p)
{
	Table	*t	= 0;
	int	*max	= 0;

	switch(p->get_type())
	{
		case ET_stop:
		case ET_signal:
		case ET_syscall:
#if EXCEPTION_HANDLING
		case ET_exception:
#endif
			t = events;
			max = &max_events;
			break;

		case ET_onstop:
			t = onstop;
			max = &max_onstop;
			break;

		case ET_none:
		default:
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}

	for (int i = 0; i < *max; i++)
	{
		Cell_value item = t->get_cell(i, 0);
		int id = atoi(item.string);
	
		if (p->get_id() == id)
		{
			t->deselect(i);
			t->delete_rows(i, 1);
			*max = (*max) - 1;
			break;
		}
	}
}

void
Event_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
	{
		events->set_sensitive(FALSE);
		onstop->set_sensitive(FALSE);
	}
	else if (rc == RC_end_script)
	{
		events->set_sensitive(TRUE);
		onstop->set_sensitive(TRUE);
	}
	parent->set_sensitivity();
}

void
Event_pane::update_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	int	next_event = 0;
	int	next_onstop = 0;

	if (in_script || rc != RC_set_current)
		return;

	if (!proc)
	{
		events->clear();
		onstop->clear();
		max_onstop = max_events = 0;
		ableEventSel = disableEventSel = 0;
		parent->set_sensitivity();
		return;
	}

	Buffer	*plist = buf_pool.get();

	onstop->deselect_all();
	onstop->delay_updates();
	events->deselect_all();
	events->delay_updates();
	ableEventSel = disableEventSel = 0;
	for (Elink *link = proc->get_events(); link; link = link->next())
	{
		Event	*e = link->event();
		char	id[MAX_INT_DIGITS + 1];
		char	buf[MAX_INT_DIGITS + 1];
		char	state[2];

		sprintf(id, "%-d", e->get_id());
		state[0] = state[1] = 0;
		buf[0] = 0;

		e->get_plist(plist);
		if (e->get_state() == ES_disabled)
			state[0] = 'D';

		if (e->get_type() == ET_onstop)
		{
			if (next_onstop >= max_onstop)
				onstop->insert_row(next_onstop, id, state, (char *)*plist,
					e->get_commands());
			else
				onstop->set_row(next_onstop, id, state, (char *)*plist,
					e->get_commands());
			next_onstop++;
		}
		else
		{
			switch(e->get_type())
			{
				case ET_stop:
					if (((Stop_event *)e)->get_count() > 1)
						sprintf(buf, "%d",((Stop_event *)e)->get_count());
					break;
				case ET_syscall:
					if (((Syscall_event *)e)->get_count() > 1)
						sprintf(buf, "%d",((Syscall_event *)e)->get_count());
					break;
			}

			if (next_event >= max_events)
				events->insert_row(next_event, id, state,
					e->get_type_string(), (char *)*plist,
		 			e->get_condition(), buf, e->get_commands());
			else
				events->set_row(next_event, id, state,
					e->get_type_string(), (char *)*plist,
		 			e->get_condition(), buf, e->get_commands());

			next_event++;
		}
	}
	buf_pool.put(plist);

	if (max_events > next_event)
		events->delete_rows(next_event, max_events - next_event);
	events->finish_updates();
	max_events = next_event;

	if (max_onstop > next_onstop)
		onstop->delete_rows(next_onstop, max_onstop - next_onstop);
	onstop->finish_updates();
	max_onstop = next_onstop;
}

void
Event_pane::get_selected_ids(Buffer *buffer)
{
	Vector	*v = vec_pool.get();
	int	selected;
	int	*sel;
	int	i;

	Cell_value item;

	buffer->clear();

	// get events from main event pane
	selected = events->get_selections(v);
	sel = (int *)v->ptr();
	for (i = 0; i < selected; i++, ++sel)
	{
		item = events->get_cell(*sel, COL_ID);

		buffer->add(item.string);
		buffer->add(' ');
	}

	// get events from onstop pane
	selected = onstop->get_selections(v);
	sel = (int *)v->ptr();
	for (i = 0; i < selected; i++, sel++)
	{
		item = onstop->get_cell(*sel, COL_ID);
		buffer->add(item.string);
		buffer->add(' ');
	}

	vec_pool.put(v);
}

void
Event_pane::disableEventCb(Component *, void *)
{
	Buffer	*buf = buf_pool.get();

	get_selected_ids(buf);
	dispatcher.send_msg(this, get_window_set()->current_obj()->get_id(),
		"disable %s\n", (char *)*buf);
	buf_pool.put(buf);
}

void
Event_pane::enableEventCb(Component *, void *)
{
	Buffer	*buf = buf_pool.get();

	get_selected_ids(buf);
	dispatcher.send_msg(this, get_window_set()->current_obj()->get_id(),
		"enable %s\n", (char *)*buf);
	buf_pool.put(buf);
}

void
Event_pane::deleteEventCb(Component *, void *)
{
	Buffer	*buf = buf_pool.get();

	get_selected_ids(buf);
	dispatcher.send_msg(this, get_window_set()->current_obj()->get_id(),
		"delete %s\n", (char *)*buf);
	buf_pool.put(buf);
}

// Find the event id of the selected event 
// Bring up the appropriate dialog according to the type of event
void
Event_pane::changeEventCb(Component *, void *)
{
	Vector	*v = vec_pool.get();
	int	eventId;

	// check main event pane first, then onstop pane
	// should be one and only one event selected in both panes
	if (events->get_selections(v))
	{
		eventId = atoi(events->get_cell(*(int *)v->ptr(), 0).string);
		vec_pool.put(v);
	}
	else if (onstop->get_selections(v))
	{
		eventId = atoi(onstop->get_cell(*(int *)v->ptr(), 0).string);
		vec_pool.put(v);
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}

	Event		*p 	= event_list.findEvent(eventId);
	Event_type	type = p->get_type();
	Process_dialog	*pToShareCode = 0;

	switch(type)
	{
	case ET_stop:
	{
		Stop_dialog	*stop_box = window_set->get_stop_box();

		// fill data in each interface object
		if (!stop_box)
		{
			stop_box = new Stop_dialog(parent);
			window_set->set_stop_box(stop_box);
		}

		pToShareCode = stop_box;

		stop_box->setEventId(eventId);
		stop_box->fillContents(p);
		stop_box->setChange(1);
		break;
	}
	case ET_signal:
	{
		Signal_dialog	*signal_box = window_set->get_signal_box();

		if (!signal_box)
		{
			signal_box = new Signal_dialog(parent);
			window_set->set_signal_box(signal_box);
		}

		pToShareCode = signal_box;
		signal_box->setEventId(eventId);
		signal_box->fillContents(p);
		signal_box->setChange(1);
		break;
	}
	case ET_syscall:
	{
		Syscall_dialog	*syscall_box = window_set->get_syscall_box();
		if (!syscall_box)
		{
			syscall_box = new Syscall_dialog(parent);
			window_set->set_syscall_box(syscall_box);
		}

		pToShareCode = syscall_box;
		syscall_box->setEventId(eventId);
		syscall_box->fillContents(p);
		syscall_box->setChange(1);
		break;
	}
	case ET_onstop:
	{
		Onstop_dialog	*onstop_box = window_set->get_onstop_box();

		if (!onstop_box)
		{
			onstop_box = new Onstop_dialog(parent);
			window_set->set_onstop_box(onstop_box);
		}

		pToShareCode = onstop_box;
		onstop_box->setEventId(eventId);
		onstop_box->fillContents(p);
		onstop_box->setChange(1);
		break;
	}
#if EXCEPTION_HANDLING
	case ET_exception:
	{
		Exception_dialog *exception_box = window_set->get_exception_box();
		if (!exception_box)
		{
			exception_box = new Exception_dialog(parent);
			window_set->set_exception_box(exception_box);
		}

		pToShareCode = exception_box;
		exception_box->setEventId(eventId);
		exception_box->fillContents(p);
		exception_box->setChange(1);
		break;
	}
#endif
	case ET_none:
	default:
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	}

	pToShareCode->set_plist(parent, window_set->get_event_level());
	pToShareCode->display();
}
