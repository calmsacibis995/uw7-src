#ident	"@(#)debugger:gui.d/common/Stack_pane.C	1.37"

#include "Dispatcher.h"
#include "Windows.h"
#include "Window_sh.h"
#include "UI.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Stack_pane.h"
#include "Menu.h"
#include "config.h"
#include "Sense.h"
#include "gui_label.h"

#include "Message.h"
#include "Msgtab.h"
#include "str.h"
#include "Buffer.h"
#include "Machine.h"
#include "Vector.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

static const Column stack_spec[] =
{
	{ LAB_none,	1,	Col_glyph },
	{ LAB_frame,	5,	Col_numeric },
	{ LAB_function,	20,	Col_text },
	{ LAB_parameters, 27,	Col_text },
	{ LAB_location,	12,	Col_right_text },
};

Stack_pane::Stack_pane(Window_set *ws, Base_window *parent, Box *box,
	Pane_descriptor *pdesc) : PANE(ws, parent, PT_stack)
{
	top_frame = cur_frame = -1;
	next_row = 0;
	has_selection = 0;
	update_in_progress = 0;
	update_abort = FALSE;
	stack_count_down = FALSE;
	
	pane = new Table(box, "stack", SM_single, stack_spec,
		sizeof(stack_spec)/sizeof(Column), pdesc->get_rows(), 
			FALSE, Search_none, 0,
		(Callback_ptr)(&Stack_pane::select_frame), 0,
		(Callback_ptr)(&Stack_pane::deselect_frame), 
		(Callback_ptr)(&Stack_pane::default_action),
		0, 
		this, HELP_stack_pane);
	box->add_component(pane, TRUE);
	if (pdesc->get_menu_descriptor())
		popup_menu = create_popup_menu(pane, parent, 
			pdesc->get_menu_descriptor());
}

Stack_pane::~Stack_pane()
{
	delete popup_menu;
	window_set->change_current.remove(this,
		(Notify_func)(&Stack_pane::update_cb), 0);
}

void
Stack_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this, (Notify_func)(&Stack_pane::update_cb), 0);
	window_set->change_state.add(this, (Notify_func)(&Stack_pane::update_state_cb), 0);
}

void
Stack_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)(&Stack_pane::update_cb), 0);
	window_set->change_state.remove(this,
		(Notify_func)(&Stack_pane::update_state_cb), 0);
}

void
Stack_pane::set_current()
{
	ProcObj	*proc = window_set->current_obj();
	int	row;
	Vector	*v = vec_pool.get();

	if (!proc || pane->get_selections(v) != 1)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}

	row = *(int *)v->ptr();
	vec_pool.put(v);

	// top_frame should have been set by now. this is usually
	// done as part of the initial update
	int new_frame = stack_count_down ? top_frame - row : row;
	dispatcher.send_msg(0, proc->get_id(), "set %%frame = %d\n", new_frame);
}

// default action is to set_current on the dblselected frame
void
Stack_pane::default_action(Table *, Table_calldata *tdp)
{
	int	old_row, new_row;

	if (stack_count_down)
	{
		old_row = top_frame - cur_frame;
		new_row = top_frame - tdp->index;
	}
	else
	{
		old_row = cur_frame;
		new_row = tdp->index;
	}
	if (old_row != tdp->index)
	{
		dispatcher.send_msg(0, window_set->current_obj()->get_id(), 
			"set %%frame = %d\n", new_row);
	}
}

void
Stack_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
	{
		pane->set_sensitive(FALSE);
	}
	parent->set_sensitivity();
}

void
Stack_pane::update_cb(void *, Reason_code rc, void *, ProcObj *call_data)
{
	if (rc == RC_delete)
		// delete is followed by set_current of new pobj or 0
		return;

	if (!call_data)
	{
		// no more processes
		reset();
		return;
	}

	if (call_data->is_animated() || in_script)
	{
		if (rc == RC_animate)
			pane->set_sensitive(FALSE);
		return;
	}

	if (call_data->is_incomplete())
	{
		// a process state update is in progress, meanwhile a
		// user action might have resulted in this update routine 
		// being called. we ignore it here, knowing that when the
		// process state update is finished, the user action result
		// will be handled again.
		pane->set_sensitive(FALSE);
		cur_frame = -1;
		return;
	}

	if (rc == RC_set_frame)
	{
		int	old_row, new_row;

		if (stack_count_down)
		{
			old_row = top_frame - cur_frame;
			new_row = top_frame - call_data->get_frame();
		}
		else
		{
			old_row = cur_frame;
			new_row = call_data->get_frame();
		}
		if (cur_frame > -1)
			pane->set_cell(old_row, ST_current, Gly_blank);
		pane->set_cell(new_row, ST_current, Gly_hand);
		cur_frame = call_data->get_frame();
		return;
	}

	if (call_data->is_halted() || call_data->is_core())
	{
		if (!update_in_progress)
		{
			parent->inc_busy();
			next_row = 0;
			top_frame = -1;
			dispatcher.send_msg(this, call_data->get_id(), "stack\n");
			++update_in_progress;
		}
		if (update_abort)
			update_abort = FALSE;
	}
	else if (rc == RC_set_current)
		reset();
	else
	{
		if (update_in_progress)
			// when a process or thread is set running while
			// a stack pane update is in progress (this should
			// only happen currently when a thread is restarted
			// automatically when its state changes), we need to
			// "cancel" the update, otherwise the rest of the 
			// update will again sensitize the pane
			update_abort = TRUE;
		pane->set_sensitive(FALSE);
	}
}

void
Stack_pane::reset()
{
	pane->clear();
	cur_frame = top_frame = -1;
	next_row = 0;
	if (has_selection)
	{
		has_selection = 0;
		parent->set_selection(0);
	}
}

void
Stack_pane::cmd_complete()
{
	if (!update_abort)
	{
		int	cur_rows = pane->get_rows();

		if (next_row < cur_rows)
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
	cur_frame = window_set->current_obj()->get_frame();
}

static Buffer buffer;

void
Stack_pane::de_message(Message *m)
{
	char		loc[PATH_MAX+1+MAX_INT_DIGITS+1];
	char		*file = 0;
	Word		wtmp;
	char		buf[MAX_INT_DIGITS+1];
	char		*cur_indicator;


	// the messages from the dispatcher is received in the 
	// following sequence:
	//	stack_header
	//	stack_frame
	//	stack_arg (or stack_arg2 or stack_arg3)
	//	...
	//	stack_frame_end_1 (or stack_frame_end_2)
	//	stack_frame
	//	stack_arg (or stack_arg2 or stack_arg3)
	//	...
	//	stack_frame_end_1 (or stack_frame_end_2)
	//	...
	//	cmd_complete

	if (update_abort)
		return;
	switch(m->get_msg_id())
	{
	case MSG_stack_header:
		top_frame = -1;
		next_row = 0;
		if (has_selection)
		{
			has_selection = 0;
			pane->deselect_all();
			parent->set_selection(0);
		}
		break;

	case MSG_stack_frame:
		m->unbundle(cur_indicator, wtmp, func);
		cur_frame = (int)wtmp;
		func = makestr(func);

		if (top_frame == -1)
		{
			top_frame = cur_frame;
			if (top_frame > 0)
				stack_count_down = TRUE;
			else
				stack_count_down = FALSE;
			pane->delay_updates();
		}
		if (strcmp(cur_indicator, "*") == 0)
			window_set->current_obj()->set_frame(cur_frame);
		buffer.clear();
		buffer.add('(');
		break;

	case MSG_stack_frame_end_1:
		m->unbundle(file, wtmp);
		buffer.add(')');
		sprintf(loc, "%s@%d", file, wtmp);
set_row:
		sprintf(buf, "%d", cur_frame);
		if (next_row >= pane->get_rows())
			pane->insert_row(next_row,
				window_set->current_obj()->get_frame()==cur_frame ? 
				Gly_hand : Gly_blank, 
				buf, func, (char *)buffer, loc);
		else
			pane->set_row(next_row,
				window_set->current_obj()->get_frame()==cur_frame ? 
				Gly_hand : Gly_blank, 
				buf, func, (char *)buffer, loc);
		delete func;
		next_row++;
		break;

	case MSG_stack_frame_end_2:
		m->unbundle(wtmp);
		buffer.add(')');
		sprintf(loc, "%#x", wtmp);
		goto set_row;

	case MSG_stack_arg:
	case MSG_stack_arg2:
	case MSG_stack_arg3:
		buffer.add(m->format());
		break;

	case ERR_short_read:
		// to avoid this annoying message on every stack frame
		// update when an argument to a function is a LONG string, 
		// the gui ignores it since it's obvious that
		// the data has been truncated.
		break;

	case ERR_get_text:
		if (!window_set->current_obj()->in_bad_state())
			display_msg(m);
		break;

	case ERR_invalid_op_dead:
		// gui should never be doing a stack on a dead process.
		// however, a process or thread may have
		// died after the stack was issued but before gui
		// received the notification.
		break;

	default:
		if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
			display_msg(m);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void
Stack_pane::select_frame(Table *, Table_calldata *tdata)
{
	has_selection = 1;
	parent->set_selection(this);
	const char *string;
	string = pane->get_cell(tdata->index, tdata->col).string;
	parent->get_window_shell()->display_msg(E_NONE, string);
}

void
Stack_pane::deselect_frame(Table *, int)
{
	if (has_selection)
	{
		// due to a Table bug for updating glyphs, further calls to
		// pane->deselect() or pane->deselect_all() must be avoided
		has_selection = 0;
		parent->set_selection(0);
	}
}

Selection_type
Stack_pane::selection_type()
{
	return SEL_frame;
}

void
Stack_pane::deselect()
{
	if (has_selection)
	{
		has_selection = 0;
		pane->deselect_all();
	}
}

int
Stack_pane::get_frame(int frame, const char *&func, const char *&loc)
{
	if (frame < 0 || frame >= next_row)
		return 0;
	int want_row = stack_count_down ? top_frame - frame : frame;
	func = pane->get_cell(want_row, ST_func).string;
	loc = pane->get_cell(want_row, ST_loc).string;
	return 1;
}

int
Stack_pane::check_sensitivity(Sensitivity *sense)
{
	return(sense->frame_sel() && has_selection);
}

void
Stack_pane::redisplay()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
}
