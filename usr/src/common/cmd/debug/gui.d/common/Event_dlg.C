#ident	"@(#)debugger:gui.d/common/Event_dlg.C	1.54"

// GUI headers
#include "Base_win.h"
#include "Dialogs.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Sel_list.h"
#include "Toggle.h"
#include "Text_line.h"
#include "Text_area.h"
#include "Stext.h"
#include "Radio.h"
#include "Dispatcher.h"
#include "Events.h"
#include "Windows.h"
#include "Ps_pane.h"
#include "Proclist.h"
#include "UI.h"
#include "Eventlist.h"
#include "Window_sh.h"

// debug headers
#include "str.h"
#include "Buffer.h"
#include "Machine.h"
#include "Vector.h"
#include "gui_label.h"
#include "Label.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

static const LabelId	order_buttons[] = { LAB_name, LAB_number };

// strip the open ('{') and close ('}') curly braces off an event's command list
// before displaying the list in a Change event dialog
// The vector v provides space to store the changed string
static char *
strip_curlies(const char *s, Vector *v)
{
	char	*ptr;
	char	*first = 0;
	char	*last = 0;

	if (!s)
		return 0;

	v->clear();
	v->add((char *)s, strlen(s)+1);
	ptr = (char *)v->ptr();

	first = strchr(ptr, '{');
	last = strrchr(ptr, '}');

	if (first != 0 && last != 0)
	{
		*last = '\0';
		return first+1;
	}
	else	
		return ptr;
}

// Set the Signal or Syscall selection list from the event condition
// This function is called to initialize the Change dialogs
static void
set_selections(const char *condition, Selection_list *list, int nentries)
{
	// deselect the old selections
	Vector	*v = vec_pool.get();
	int	nsels = list->get_selections(v);
	int	*sels = (int *)v->ptr();
	for (; nsels; sels++, nsels--)
		list->deselect(*sels);

	// use the condition string to select the syscall list
	v->clear();
	v->add((char *)condition, strlen(condition)+1);
	char *ptr = strtok((char *)v->ptr(), " ,");
	
	while (ptr != NULL)
	{
		if (strlen(ptr) != 0)
		{
			for (int i= 0; i < nentries; i++)
			{
				if (strcmp(ptr, list->get_item(i, 0)) == 0)
				{
					list->select(i);
					break;
				}
			}
		}
		ptr = strtok(NULL, " ,");
	}
	vec_pool.put(v);
}

Stop_dialog::Stop_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_stop,  LAB_stop_mne,
			(Callback_ptr)&Stop_dialog::apply},
		{ B_apply, LAB_none, LAB_none, 
			(Callback_ptr)&Stop_dialog::apply},
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Stop_dialog::reset },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Packed_box	*box;
	Caption		*caption;

	save_expr = save_cmd = save_count = 0;
	doChange = 0;
	eventId = 0;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_stop, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_stop_dialog);
	box = new Packed_box(dialog, "stop box", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_expression_line, CAP_LEFT);
	expr = new Text_line(caption, "expression", "", 25, 1);
	caption->add_component(expr);
	box->add_component(caption);

	caption = new Caption(box, LAB_commands_line, CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 25, 1);
	caption->add_component(commands);
	box->add_component(caption);

	caption = new Caption(box, LAB_stop_count_line, CAP_LEFT);
	count = new Text_line(caption, "count", "1", 8, 1);
	caption->add_component(count, FALSE);
	box->add_component(caption);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(expr);
}

// Create the stop event
void
Stop_dialog::apply(Component *, int)
{
	char		*cmd;
	char		*stop_expr;
	char		*cnt;

	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	cmd = commands->get_text();
	stop_expr = expr->get_text();
	cnt = count->get_text();

	if (!stop_expr || !*stop_expr)
	{
		dialog->error(E_ERROR, GE_no_stop_expr);
		return;
	}

	// change cmd always preserves the process list of the event
	if (doChange)
	{
		if (!cnt || !*cnt)
			cnt = "1";
		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d -c%s %s {%s}\n", eventId, cnt,
				stop_expr, cmd);
		else
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d -c%s %s\n", eventId, cnt, stop_expr);
	}
	else
	{
		// save values for reset
		delete save_cmd;
		delete save_expr;
		delete save_count;
		save_count = 0;
		save_cmd = 0;
		save_expr = makestr(stop_expr);
		if (cmd && *cmd)
			save_cmd = makestr(cmd);

		// a blank count is equivalent to count of 1
		if (cnt && *cnt)
			save_count = makestr(cnt);
		else
			cnt = "1";

		//create event, rather than change event
		//	In context window, it uses all the selected processes
		//	In other windows, it uses the current process
		const char *	plist = make_plist(total, pobjs, 0, level);
	
		if (save_cmd)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"stop -p %s -c%s %s {%s}\n", plist, cnt,
				stop_expr, cmd);
		else
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"stop -p %s -c%s %s\n", plist, cnt, stop_expr);
	}

	dialog->wait_for_response();
}

// fillContents is used by the Change dialog to initialize the components
void
Stop_dialog::fillContents(Event *event)
{
	char	buf[MAX_INT_DIGITS+1];

	expr->set_text(event->get_condition());

	if (((Stop_event *)event)->get_count())
	{
		sprintf(buf, "%d", ((Stop_event *)event)->get_count());
		count->set_text(buf);
	}
	else
		count->set_text("1");

	Vector	*v = vec_pool.get();
	char	*p = strip_curlies(event->get_commands(), v);

	commands->set_text(p);
	vec_pool.put(v);
}

void
Stop_dialog::reset(Component *, int)
{
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		expr->set_text(save_expr);
		commands->set_text(save_cmd);
		count->set_text(save_count);
	}
}

// set_expression is called from the menu button callback to initialize
// the expression with a location if a line is selected in the Source or
// Disassembly windows
void
Stop_dialog::set_expression(const char *s)
{
	delete save_expr;
	save_expr = s ? makestr(s) : 0;
	expr->set_text(save_expr);
}

void
Stop_dialog::setChange(int state)
{
	if (doChange == state)
		return;

	doChange = state;
	if (doChange)
	{
		dialog->set_label(LAB_change, LAB_change_mne);
		dialog->set_help(HELP_change_dialog);
	}
	else
	{
		dialog->set_label(LAB_stop, LAB_stop_mne);
		dialog->set_help(HELP_stop_dialog);
		reset(0,0);
	}
}

Stop_on_function_dialog::Stop_on_function_dialog(Base_window *bw)
	: OBJECT_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_stop_on_func, LAB_stop_on_func_mne,
			(Callback_ptr)(&Stop_on_function_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Stop_on_function_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Stop_on_function_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
	};

	Expansion_box	*box;
	Caption		*fcaption;
	const char	*initial_string = "";

	f_selection = -1;

	dialog = new Dialog_shell(bw->get_window_shell(), LAB_stop_on_func,
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton),
		HELP_stop_on_function_dialog);
	box = new Expansion_box(dialog, "Stop on function", OR_vertical);
	component_init(box);

	box->add_component(make_obj_list(box));

	fcaption = new Caption(box, LAB_filter, CAP_LEFT);
	filter = new Text_line(fcaption, "filter", "", 25, TRUE,
		0, 0, HELP_none, TRUE);
	fcaption->add_component(filter);
	box->add_component(fcaption);

	caption = new Caption(box, LAB_functions_line, CAP_TOP_LEFT);
	functions = new Selection_list(caption, "functions", 8, 1, sel_col, 1,
		&initial_string, SM_single, this, 
		Search_alphabetic, 0,
		(Callback_ptr)0, (Callback_ptr)0,
		(Callback_ptr)(&Stop_on_function_dialog::default_cb));
	caption->add_component(functions);
	box->add_component(caption, TRUE);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(filter);
}

void
Stop_on_function_dialog::set_obj(Boolean reset)
{
	caption->set_label(labeltab.get_label(LAB_functions_line));
	functions->set_list(0, 0);
	Object_dialog::set_obj(reset);
}

void
Stop_on_function_dialog::apply(Component *, void *)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	if (functions->get_selections(v) == 0)
	{
		dialog->error(E_ERROR, GE_no_function);
		vec_pool.put(v);
		return;
	}
	f_selection = *(int *)v->ptr();
	vec_pool.put(v);
	do_it();
}

void
Stop_on_function_dialog::do_it()
{
	Vector	*v = vec_pool.get();

	objects->get_selections(v);
	saved_object = *(int *)v->ptr();
	vec_pool.put(v);

	// -p is needed to ensure process-specific stops
	dispatcher.send_msg(this, pobjs[0]->get_id(), "stop -p %s %s\n",
		make_plist(1, pobjs, 0, level),
		functions->get_item(f_selection, 0));
	dialog->wait_for_response();
}

void
Stop_on_function_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	f_selection = item_index;
	do_it();
	// always wait for cmd to execute

}

void
Stop_on_function_dialog::cancel(Component *, void *)
{
	if (f_selection > -1)
		functions->select(f_selection);
	else
		functions->deselect(f_selection);

	if (!dialog->is_pinned())
		return;
	cancel_change();
}

void
Stop_on_function_dialog::object_changed(const char *obj)
{
	if (!obj)
	{
		caption->set_label(labeltab.get_label(LAB_functions_line));
		functions->set_list(0, 0);
		f_selection = -1;
		return;
	}

	Vector	*v = vec_pool.get();
	char	*filt = filter->get_text();
	if (!filt)
		filt = "";
	int	nfuncs = pobjs[0]->get_functions(v, 0, obj, filt, 0);
	const char **funclist = (const char **)v->ptr();

	if (nfuncs == 0)
	{
		dialog->error(E_ERROR, GE_stripped_file, obj);
		vec_pool.put(v);
		return;
	}

	const char	*fmt = gm_format(GM_functions_from);
	char		*buf = makesf(strlen(fmt) + strlen(obj), fmt, obj);
	caption->set_label(buf);
	functions->set_list(nfuncs, funclist);

	delete buf;
	free_strings(funclist, nfuncs);
	f_selection = -1;
	vec_pool.put(v);
}

// shared by several event dialogs
static const Sel_column	event_sel_col[] =
{
	{ SCol_text,	0 },
	{ SCol_numeric,	0 },
};

Signal_dialog::Signal_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_signal, LAB_signal_mne, 
			(Callback_ptr)&Signal_dialog::apply },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)&Signal_dialog::apply },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Signal_dialog::reset },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Expansion_box	*ebox;
	Expansion_box	*ebox2;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_signal, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_signal_dialog);
	ebox = new Expansion_box(dialog, "signal box", OR_vertical);
	component_init(ebox);

	ebox2 = new Expansion_box(ebox, "signal box", OR_horizontal);
	ebox->add_component(ebox2, TRUE);

	order = Numeric;
	siglist = new Selection_list(ebox2, "signal list", 5, 2, 
		event_sel_col, Nsignals,
		get_signals(order), SM_multiple, this,
		Search_random, 0,
		0, 0,
		(Callback_ptr)(&Signal_dialog::default_cb));
	ebox2->add_component(siglist);

	caption = new Caption(ebox2, LAB_order_list_by, CAP_TOP_LEFT);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(LabelId), 1,
		(Callback_ptr)&Signal_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	ebox2->add_component(caption, TRUE);

	caption = new Caption(ebox, LAB_commands_line, CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 20, 1);
	caption->add_component(commands);
	ebox->add_component(caption);
	ebox2->update_complete();
	ebox->update_complete();
	dialog->add_component(ebox);
	dialog->set_popup_focus(commands);

	doChange = 0;
	eventId  = 0;
	save_cmd = 0;
	nselections = 0;
	selections = 0;
}

// Create the Signal event
void
Signal_dialog::apply(Component *, int)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	int	nsigs = siglist->get_selections(v);

	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		vec_pool.put(v);
		return;
	}

	(void)do_it(nsigs, (int *)v->ptr());
	vec_pool.put(v);
}

int
Signal_dialog::do_it(int nsigs, int *signals)
{
	char	*cmd = commands->get_text();
	if (!cmd || !*cmd)
	{
		dialog->error(E_ERROR, GE_no_cmd_list);
		return 0;
	}

	Buffer	*buf = buf_pool.get();
	buf->clear();
	for (int i = 0; i < nsigs; i++)
	{
		if (i > 0)
			buf->add(' ');
		buf->add(siglist->get_item(signals[i], 0));
	}

	if (doChange)
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"change %d %s {%s}\n", eventId, (char *)*buf, cmd);
	else
	{
		// save values for reset
		delete save_cmd;
		save_cmd = makestr(cmd);
		order = (Order)ordering->which_button();
		delete selections;
		nselections = nsigs;
		selections = new int[nsigs];
		memcpy(selections, signals, sizeof(int) * nsigs);

		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"signal -p %s %s {%s}\n", make_plist(total, pobjs, 0, level),
			(char *)*buf, cmd);
	}
	dialog->wait_for_response();
	buf_pool.put(buf);
	return 1;
}

void
Signal_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}
	if (!do_it(1, &item_index))
		dialog->default_done();
	// no need to call default_done if do_it sent the request
	// since we're always waiting for cli response in that case
}

void
Signal_dialog::set_order(Component *, Order o)
{
	siglist->set_list(Nsignals, get_signals(o));
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			set_selections(event->get_condition(), siglist, Nsignals);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

void
Signal_dialog::reset(Component *, int)
{
	int	i;
	if (ordering->which_button() != order)
	{
		ordering->set_button(order);
		siglist->set_list(Nsignals, get_signals(order));
	}

	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		if (ordering->which_button() == order)
		{
			Vector	*v = vec_pool.get();
			int	nsig = siglist->get_selections(v);
			int	*sig = (int *)v->ptr();

			for (i = 0; i < nsig; i++, sig++)
				siglist->deselect(*sig);
			vec_pool.put(v);
		}

		for (i = 0; i < nselections; i++)
			siglist->select(selections[i]);
		commands->set_text(save_cmd);
	}
}

void
Signal_dialog::fillContents(Event *event)
{
	Vector	*v = vec_pool.get();
	char	*p = strip_curlies(event->get_commands(), v);

	// use the condition string to select the signal list
	set_selections(event->get_condition(), siglist, Nsignals);
	commands->set_text(p);
	vec_pool.put(v);
}

void
Signal_dialog::setChange(int state)
{
	if (doChange == state)
		return;

	doChange = state;
	if (doChange)
	{
		dialog->set_label(LAB_change, LAB_change_mne);
		dialog->set_help(HELP_change_dialog);
	}
	else
	{
		dialog->set_label(LAB_signal, LAB_signal_mne);
		dialog->set_help(HELP_signal_dialog);
		reset(0,0);
	}
}

// Combinations of Entry and Exit checkboxes
#define	NONE_SET	0
#define	ENTRY_SET	1
#define EXIT_SET	2
#define BOTH_SET	3

Syscall_dialog::Syscall_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_syscall, LAB_syscall_mne,
			(Callback_ptr)(&Syscall_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Syscall_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none, 
			(Callback_ptr)(&Syscall_dialog::reset) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Toggle_data toggles[] =
	{
		{ LAB_entry,	TRUE,	0 },
		{ LAB_exit,	FALSE,	0 },
	};

	Expansion_box	*ebox;
	Expansion_box	*ebox2;
	Caption		*caption;
	const char	**list;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_syscall, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_syscall_dialog);
	ebox = new Expansion_box(dialog, "syscall box", OR_vertical);
	component_init(ebox);

	ebox2 = new Expansion_box(ebox, "syscall box", OR_horizontal);
	ebox->add_component(ebox2, TRUE);

	order = Alpha;
	list = get_syslist(order);
	syslist = new Selection_list(ebox2, "syscall list", 7, 2, 
		event_sel_col, Nsyscalls, list, SM_multiple, this,
		Search_random, 0,
		0, 0,
		(Callback_ptr)&Syscall_dialog::default_cb);
	ebox2->add_component(syslist);

	caption = new Caption(ebox2, LAB_order_list_by, CAP_TOP_LEFT);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(LabelId), 0,
		(Callback_ptr)&Syscall_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	ebox2->add_component(caption, TRUE);

	caption = new Caption(ebox, LAB_stop_on_line, CAP_LEFT);
	entry_exit = new Toggle_button(caption, "entry or exit", toggles,
		sizeof(toggles)/sizeof(Toggle_data), OR_horizontal, this);
	caption->add_component(entry_exit, FALSE);
	ebox->add_component(caption);

	caption = new Caption(ebox, LAB_commands_line, CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 20, TRUE);
	caption->add_component(commands);
	ebox->add_component(caption);

	caption = new Caption(ebox, LAB_sys_count_line, CAP_LEFT);
	count = new Text_line(caption, "count", "1", 4, TRUE);
	caption->add_component(count, FALSE);
	ebox->add_component(caption);

	ebox2->update_complete();
	ebox->update_complete();
	dialog->add_component(ebox);
	dialog->set_popup_focus(commands);

	doChange = 0;
	eventId  = 0;
	save_count = save_cmd = 0;
	nselections = 0;
	selections = 0;
	entry_exit_state = NONE_SET;
}

void
Syscall_dialog::apply(Component *, int)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	int	nsys = syslist->get_selections(v);

	if (!nsys)
	{
		dialog->error(E_ERROR, GE_no_sys_calls);
		vec_pool.put(v);
		return;
	}
	do_it(nsys, (int *)v->ptr());
	vec_pool.put(v);
}

int
Syscall_dialog::do_it(int nsys, int *syscalls)
{
	const char	*ex;
	int		toggle_state;

	if (entry_exit->is_set(0) && entry_exit->is_set(1))
	{
		toggle_state = BOTH_SET;
		ex = "-ex";
	}
	else if (entry_exit->is_set(0))
	{
		toggle_state = ENTRY_SET;
		ex = "-e";
	}
	else if (entry_exit->is_set(1))
	{
		toggle_state = EXIT_SET;
		ex = "-x";
	}
	else
	{
		dialog->error(E_ERROR, GE_entry_exit);
		return 0;
	}

	char		*cmd = commands->get_text();
	char		*cnt = count->get_text();
	Buffer		*buf = buf_pool.get();

	buf->clear();
	for (int i = 0; i < nsys; i++)
	{
		if (i > 0)
			buf->add(' ');
		buf->add(syslist->get_item(syscalls[i], 0));
	}

	if (doChange)
	{
		if (!cnt || !*cnt)
			cnt = "1";
		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d -c%s %s %s {%s}\n", eventId, cnt, ex,
				(char *)*buf, cmd);
		else
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d -c%s %s %s\n", eventId, cnt, ex,
				(char *)*buf);
	}
	else
	{
		// save state for reset
		nselections = nsys;
		delete selections;
		selections = new int[nsys];
		memcpy(selections, syscalls, sizeof(int) * nsys);
	
		delete save_cmd;
		if (cmd && *cmd)
			save_cmd = makestr(cmd);
		else
			save_cmd = 0;

		delete save_count;
		if (cnt && *cnt)
			save_count = makestr(cnt);
		else
		{
			save_count = 0;
			cnt = "1";
		}
		entry_exit_state = toggle_state;

		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"syscall -p %s -c%s %s %s {%s}\n",
				make_plist(total, pobjs, 0, level),
				cnt, ex, (char *)*buf, cmd);
		else
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"syscall -p %s -c%s %s %s\n",
				make_plist(total, pobjs, 0, level),
				cnt, ex, (char *)*buf);
		order = (Order)ordering->which_button();
	}

	dialog->wait_for_response();
	buf_pool.put(buf);
	return 1;
}

void
Syscall_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}
	if (!do_it(1, &item_index))
		dialog->default_done();
	// no need to call default_done if do_it sent the request
	// since we're always waiting for cli response in that case
}

void
Syscall_dialog::set_order(Component *, Order o)
{
	syslist->set_list(Nsyscalls, get_syslist(o));
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			set_selections(event->get_condition(), syslist, Nsyscalls);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

void
Syscall_dialog::reset(Component *, int)
{
	int i;

	if (ordering->which_button() != order)
	{
		ordering->set_button(order);
		syslist->set_list(Nsyscalls, get_syslist(order));
	}

	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		if (ordering->which_button() == order)
		{
			Vector	*v = vec_pool.get();
			int	nsys = syslist->get_selections(v);
			int	*sys = (int *)v->ptr();

			for (i = 0; i < nsys; i++, sys++)
				syslist->deselect(*sys);
			vec_pool.put(v);
		}

		for (i = 0; i < nselections; i++)
			syslist->select(selections[i]);
		commands->set_text(save_cmd);
		count->set_text(save_count);
		if (entry_exit_state == BOTH_SET)
		{
			entry_exit->set(0, 1);
			entry_exit->set(1, 1);
		}
		else if (entry_exit_state == ENTRY_SET)
		{
			entry_exit->set(0, 1);
			entry_exit->set(1, 0);
		}
		else
		{
			entry_exit->set(0, 0);
			entry_exit->set(1, 1);
		}
	}
}

void
Syscall_dialog::fillContents(Event *event)
{
	char	s[MAX_INT_DIGITS+1];
	char	*ptr;
	Vector	*v = vec_pool.get();
	char	*p = strip_curlies(event->get_commands(), v);

	set_selections(event->get_condition(), syslist, Nsyscalls);
	commands->set_text(p);
	vec_pool.put(v);

	if (((Syscall_event *)event)->get_count())
	{
		sprintf(s, "%d", ((Syscall_event *)event)->get_count());
		count->set_text(s);
	}
	else
		count->set_text("1");
	
	// exit or enter	
	ptr = strchr(event->get_type_string(), ' ');
	if (ptr && *++ptr)
	{
		if (*ptr == 'E')
		{
			entry_exit->set(0, 1);
			ptr++;
		}
		else
			entry_exit->set(0, 0);

		if (*ptr == 'X')
			entry_exit->set(1, 1);
		else
			entry_exit->set(1, 0);
	}
}

void
Syscall_dialog::setChange(int state)
{
	if (doChange == state)
		return;

	doChange = state;
	if (doChange)
	{
		dialog->set_label(LAB_change, LAB_change_mne);
		dialog->set_help(HELP_change_dialog);
	}
	else
	{
		dialog->set_label(LAB_syscall, LAB_syscall_mne);
		dialog->set_help(HELP_syscall_dialog);
		reset(0,0);
	}
}

Onstop_dialog::Onstop_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_on_stop, LAB_on_stop_mne,
			(Callback_ptr)&Onstop_dialog::apply },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)&Onstop_dialog::apply },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Onstop_dialog::reset },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_on_stop, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_on_stop_dialog);
	box = new Expansion_box(dialog, "onstop_dialog", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_commands_line, CAP_TOP_LEFT);
	commands = new Text_area(caption, "commands", 4, 40, TRUE);
	caption->add_component(commands);
	box->add_component(caption, TRUE);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(commands);
	
	doChange = 0;
	eventId  = 0;
	save_cmds = 0;
}

void
Onstop_dialog::apply(Component *, int)
{
	char	*cmd = commands->get_text();

	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if (!cmd || !*cmd)
	{
		dialog->error(E_ERROR, GE_no_cmd_list);
		return;
	}

	if (doChange)
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"change %d {%s}\n", eventId, cmd);
	else
	{
		// save state for reset
		delete save_cmds;
		save_cmds = makestr(cmd);

		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"onstop -p %s {%s}\n", make_plist(total, pobjs, 0, level),
			cmd);
	}
	dialog->wait_for_response();
}

void
Onstop_dialog::reset(Component *, int)
{
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		commands->clear();
		commands->add_text(save_cmds);
	}
}

void
Onstop_dialog::fillContents(Event *event)
{
	Vector	*v = vec_pool.get();
	char	*p = strip_curlies(event->get_commands(), v);

	commands->clear();
	commands->add_text(p);
	vec_pool.put(v);
}

void
Onstop_dialog::setChange(int state)
{
	if (doChange == state)
		return;

	doChange = state;
	if (doChange)
	{
		dialog->set_label(LAB_change, LAB_change_mne);
		dialog->set_help(HELP_change_dialog);
	}
	else
	{
		dialog->set_label(LAB_on_stop, LAB_on_stop_mne);
		dialog->set_help(HELP_on_stop_dialog);
		reset(0,0);
	}
}

Cancel_dialog::Cancel_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_cancel_signal,  LAB_cancel_signal_mne,
			(Callback_ptr)&Cancel_dialog::apply },
		{ B_apply, LAB_none,  LAB_none,
			(Callback_ptr)&Cancel_dialog::apply },
		{ B_cancel, LAB_none, LAB_none, 
			(Callback_ptr)&Cancel_dialog::cancel },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;
	const char	*signals[2];

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_cancel, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_cancel_dialog);
	box1 = new Expansion_box(dialog, "cancel box", OR_vertical);
	component_init(box1);

	box2 = new Expansion_box(box1, "cancel box", OR_horizontal);
	box1->add_component(box2, TRUE);

	// Temporarily initialize the list with a blank entry.
	// The list will be set correctly later in set_process
	order = Alpha;
	signals[0] = "       ";
	signals[1] = "  ";
	siglist = new Selection_list(box2, "signal list", 5, 2, 
		event_sel_col, 1, signals, SM_multiple, this,
		Search_random, 0,
		0, 0,
		(Callback_ptr)&Cancel_dialog::default_cb);
	box2->add_component(siglist);

	caption = new Caption(box2, LAB_order_list_by, CAP_TOP_LEFT);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(LabelId), 1,
		(Callback_ptr)&Cancel_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box2->add_component(caption, TRUE);
	box2->update_complete();
	box1->update_complete();
	dialog->add_component(box1);
}

void
Cancel_dialog::set_order(Component *, Order o)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	set_list(o);
}

void
Cancel_dialog::apply(Component *, int)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	int	nsigs = siglist->get_selections(v); // # of signals to cancel

	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		vec_pool.put(v);
		return;
	}

	do_it(nsigs, (int *)v->ptr());
	vec_pool.put(v);
}

void
Cancel_dialog::do_it(int nsigs, int *signos)
{
	nselections = nsigs;
	delete selections;
	selections = new int[nsigs];
	memcpy(selections, signos, sizeof(int) *nsigs);

	Buffer	*buf = buf_pool.get();
	buf->clear();
	for (int i = 0; i < nsigs; i++, signos++)
	{
		buf->add(siglist->get_item(*signos, 0));
		buf->add(' ');
	}

	dispatcher.send_msg(this, pobjs[0]->get_id(), "cancel %s\n", (char *)*buf);
	dialog->wait_for_response();
	order = (Order)ordering->which_button();
	buf_pool.put(buf);
}

void
Cancel_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}

	do_it(1, &item_index);
	// always wait for the cmd to execute
}

void
Cancel_dialog::cancel(Component *, int)
{
	if (!dialog->is_pinned())
		return;

	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	ordering->set_button(order);
	set_list(order);
}

void
Cancel_dialog::cmd_complete()
{
	if (dialog->is_pinned())
		set_list(order);
	dialog->cmd_complete();
}

void
Cancel_dialog::set_obj(Boolean reset)
{
	if (reset)
		dialog->set_busy(TRUE);
	set_list(order);
	if (reset)
		dialog->set_busy(FALSE);
}

void
Cancel_dialog::set_list(Order o)
{
	if (!pobjs)
		siglist->set_list(0, 0);
	else
	{
		Vector	*vptr = vec_pool.get();
		Vector	*vstr = vec_pool.get();
		int	num_sigs = pobjs[0]->get_pending_signals(o, vptr, vstr);

		siglist->set_list(num_sigs, (const char **)vptr->ptr());
		vec_pool.put(vptr);
		vec_pool.put(vstr);
	}
}

Kill_dialog::Kill_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_kill, LAB_kill_mne,
			(Callback_ptr)&Kill_dialog::apply },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)&Kill_dialog::apply },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Kill_dialog::cancel },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;
	const char	**signals;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_kill, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_kill_dialog);
	box1 = new Expansion_box(dialog, "kill box", OR_vertical);
	component_init(box1);

	box2 = new Expansion_box(box1, "kill box", OR_horizontal);
	box1->add_component(box2, TRUE);

	order = Numeric;
	signals = get_signals(order);
	siglist = new Selection_list(box2, "signal list", 9, 2, 
		event_sel_col, Nsignals, signals, SM_single, this,
		Search_random, 0, 0, 0,
		(Callback_ptr)&Kill_dialog::default_cb);
	box2->add_component(siglist);

	selection = 0;

	caption = new Caption(box2, LAB_order_list_by, CAP_TOP_LEFT);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(LabelId), 1,
		(Callback_ptr)&Kill_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box2->add_component(caption, TRUE);
	box2->update_complete();
	box1->update_complete();
	dialog->add_component(box1);
}

void
Kill_dialog::set_order(Component *, Order o)
{
	const char	**signals;

	signals = get_signals(o);
	siglist->set_list(Nsignals, signals);
	for (int i = 0; i < Nsignals; i++)
	{
		if (selection && (strcmp(siglist->get_item(i, 0), 
			selection) == 0))
		{
			siglist->select(i);
			break;
		}
	}
}

void
Kill_dialog::apply(Component *, int)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();

	//for SM_single case, total selections will always be 0 or 1
	if (siglist->get_selections(v) == 0)
	{
		dialog->error(E_ERROR, GE_no_signals);
		vec_pool.put(v);
		return;
	}
	int sel = *(int *)v->ptr();
	vec_pool.put(v);
	do_it(sel);
}

void
Kill_dialog::do_it(int sel_index)
{
	delete selection;
	selection = makestr(siglist->get_item(sel_index, 0));
	
#ifdef DEBUG_THREADS
	// if the selected threads from the same process are all the
	// threads in that process, and the process has more than one threads
	if (level == THREAD_LEVEL)
	{
		Vector	*v = vec_pool.get();
		int	ptotal = get_full_procs(pobjs, total, v, FALSE);

		if (ptotal > 0)
		{
			display_msg((Callback_ptr)(&Kill_dialog::thread_kill_cb), 
				this, LAB_each_thread, LAB_parent_proc,
				AT_question, GM_thread_kill, 
				make_plist(ptotal, (ProcObj **)v->ptr(), 0, 
					PROCESS_LEVEL));
			vec_pool.put(v);
			order = (Order)ordering->which_button();
			dialog->wait_for_response();
			return;
		}
		vec_pool.put(v);
	}
#endif

	dispatcher.send_msg(this, pobjs[0]->get_id(), "kill -p %s %s\n",
		make_plist(total, pobjs, 0, level), selection);
	order = (Order)ordering->which_button();
	dialog->wait_for_response();
}

void
Kill_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}
	do_it(item_index);
	// always wait for the cmd to execute
}

#ifdef DEBUG_THREADS
void
Kill_dialog::thread_kill_cb(Component *, int send_to_threads)
{
	dispatcher.send_msg(this, pobjs[0]->get_id(), "kill -p %s %s\n",
		make_plist(total, pobjs, 0, 
			send_to_threads ? THREAD_LEVEL : PROCESS_LEVEL), 
		selection);
}
#endif

void
Kill_dialog::cancel(Component *c, int)
{
	ordering->set_button(order);
       	set_order(c, order);
}

#define SETUP_PROCESS	0
#define SETUP_DEFAULTS	1

static const LabelId	list_buttons[] =
{ 
	LAB_listed_process, 
	LAB_debugger_defaults
};

Setup_signals_dialog::Setup_signals_dialog(Base_window *bw) :
				PROCESS_DIALOG(bw, true)
{
	static const DButton	buttons[] =
	{
		{ B_exec, LAB_ignore, LAB_ignore_mne, 
			(Callback_ptr)&Setup_signals_dialog::ignore_sigs },
		{ B_exec, LAB_catch, LAB_catch_mne,
			(Callback_ptr)&Setup_signals_dialog::catch_sigs},
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Setup_signals_dialog::cancel},
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
		{ SCol_numeric,	3 },
		{ SCol_text,	0 },
		{ SCol_text,	0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	// Temporarily initialize list with a blank entry.
	// The list will be set correctly later in set_process
	const char	*initial_string[4];
	for(int i = 0; i < 4; i++)
		initial_string[i] = "";

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_ignore_signals, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_ignore_signals_dialog);
	box = new Expansion_box(dialog, "signals box", OR_vertical);
	component_init(box);

	order = Numeric;
	list_type = SETUP_PROCESS;
	caption = new Caption(box, LAB_set_signals_for, CAP_LEFT);
	which_list = new Radio_list(caption, "which_list", 
		OR_horizontal, list_buttons,
		sizeof(list_buttons)/sizeof(LabelId), list_type,
		(Callback_ptr)&Setup_signals_dialog::set_list_type, this);
	caption->add_component(which_list);
	box->add_component(caption);
	hcaption = new Caption(box, LAB_signal_hdr, CAP_TOP_LEFT);
	siglist = new Selection_list(hcaption, "signal list", 5, 4, 
		sel_col, 1, initial_string, 
		SM_multiple, this, 
		Search_random, 0,
		0, 0,
		(Callback_ptr)&Setup_signals_dialog::default_cb);
	hcaption->add_component(siglist);
	box->add_component(hcaption);

	caption = new Caption(box, LAB_order_list_by, CAP_TOP_LEFT);
	ordering = new Radio_list(caption, "order", OR_horizontal, order_buttons,
		sizeof(order_buttons)/sizeof(LabelId), 1,
		(Callback_ptr)&Setup_signals_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box->add_component(caption, TRUE);
	box->update_complete();
	dialog->add_component(box);
}

void
Setup_signals_dialog::set_list_type(Component *, int which)
{
	Order	o = (Order)ordering->which_button();
	set_list(o, which);
}

void
Setup_signals_dialog::set_order(Component *, Order o)
{
	set_list(o, which_list->which_button());
}

void
Setup_signals_dialog::ignore_sigs(Component *, void *)
{
	common_apply(TRUE);
}

void
Setup_signals_dialog::catch_sigs(Component *, void *)
{
	common_apply(FALSE);
}

void
Setup_signals_dialog::common_apply(Boolean ignore)
{
	dialog->clear_msg();
	if (!pobjs && (which_list->which_button() == SETUP_PROCESS))
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	int	nsigs = siglist->get_selections(v);

	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		vec_pool.put(v);
		return;
	}

	do_it(nsigs, (int *)v->ptr(), ignore);
	vec_pool.put(v);
}

void
Setup_signals_dialog::do_it(int nsigs, int *signos, Boolean ignore)
{
	Buffer	*buf = buf_pool.get();
	buf->clear();
	for (int i = 0; i < nsigs; i++, signos++)
	{
		if (i > 0)
			buf->add(' ');
		buf->add(siglist->get_item(*signos, 0));
	}

	if (which_list->which_button() == SETUP_PROCESS)
		dispatcher.send_msg(this, pobjs[0]->get_id(), 
			"signal %s %s\n",
			(ignore == TRUE) ? "-i" : "", (char *)*buf);
	else
		dispatcher.send_msg(this, 0, "signal -d %s %s\n",
			(ignore == TRUE) ? "-i" : "", (char *)*buf);
	dialog->wait_for_response();
	order = (Order)ordering->which_button();
	list_type = which_list->which_button();
	buf_pool.put(buf);
}

void
Setup_signals_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs && (which_list->which_button() == SETUP_PROCESS))
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}
	do_it(1, &item_index, TRUE); 	// default button is Ignore
	// no need to call default_done since we're always
	// waiting for cli response
}

void
Setup_signals_dialog::set_obj(Boolean)
{
	int	which;
	dialog->set_busy(TRUE);
	if (!pobjs)
	{
		which = SETUP_DEFAULTS;
		which_list->set_button(SETUP_DEFAULTS);
		// set insensitive by Process_dialog::set_plist
		dialog->set_sensitive(TRUE);
	}
	else
		which = which_list->which_button();
	set_list((Order)ordering->which_button(), which);
	dialog->set_busy(FALSE);
}

// If the dialog is pinned, the signal list should be updated to reflect the
// current state.  If the dialog is not pinned, this isn't necessary, since
// the list will be updated anyway the next time the dialog is popped up
void
Setup_signals_dialog::cmd_complete()
{
	if (dialog->is_pinned())
		set_list(order, list_type);
	dialog->cmd_complete();
}

void
Setup_signals_dialog::cancel(Component *, int)
{
	ordering->set_button(order);
	which_list->set_button(list_type);
	if (dialog->is_pinned())
		set_list(order, list_type);
}

int
Setup_signals_dialog::get_signals_with_status(ProcObj *p, Order order, 
	Vector *vptrs, Vector *vsig)
{
	Message		*msg;
	int		nsig = 0;
	DBcontext	id = p ? p->get_id() : 0;
	char		*flag = p ? "" : "-d";

	dispatcher.query(0, id, "signal %s\n", flag);
	vsig->clear();
	while ((msg = dispatcher.get_response()) != 0)
	{
		char	*name;
		char	*func;
		char	buf[MAX_INT_DIGITS];
		Word	sig;
		Word	addr;

		Msg_id mid = msg->get_msg_id();
		if (mid == MSG_sig_caught || mid == MSG_sig_ignored)
		{
			msg->unbundle(sig, name, func);
			vsig->add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vsig->add(buf, strlen(buf)+1);
			if (mid == MSG_sig_caught)
				vsig->add("caught", sizeof("caught"));
			else
				vsig->add("ignored", sizeof("ignored"));
			vsig->add(func, strlen(func)+1);
			nsig++;
		}
		else if (mid == MSG_sig_caught_addr || mid == MSG_sig_ignored_addr)
		{
			msg->unbundle(sig, name, addr);
			vsig->add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vsig->add(buf, strlen(buf)+1);
			if (mid == MSG_sig_caught_addr)
				vsig->add("caught", sizeof("caught"));
			else
				vsig->add("ignored", sizeof("ignored"));
			(void) sprintf(buf, "%#x", addr);
			vsig->add(buf, strlen(buf)+1);
			nsig++;
		}
		else if (mid == MSG_sig_caught_default 
			|| mid == MSG_sig_ignored_default)
		{
			msg->unbundle(sig, name);
			vsig->add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vsig->add(buf, strlen(buf)+1);
			if (mid == MSG_sig_caught_default)
				vsig->add("caught", sizeof("caught"));
			else
				vsig->add("ignored", sizeof("ignored"));
			vsig->add("", sizeof(""));
			nsig++;
		}
		else // ignore all the event messages
			continue;
	}
	Nsignals = nsig;

	// get pointers
	char *ptr = (char *)vsig->ptr();
	vptrs->clear();
	for (int i = 0; i < nsig; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			vptrs->add(&ptr, sizeof(char *));
			ptr += strlen(ptr)+1;
		}
	}

	// assumes signals come in numeric order
	if (order == Alpha)
		qsort(vptrs->ptr(), nsig, sizeof(char *[4]), alpha_comp);
	return nsig;
}

void
Setup_signals_dialog::set_list(Order o, int which)
{
	if (which == SETUP_PROCESS)
		hcaption->set_label(labeltab.get_label(LAB_signal_hdr));
	else
		hcaption->set_label(labeltab.get_label(LAB_signal_default_hdr));
	if (which == SETUP_DEFAULTS || pobjs)
	{
		Vector	*vptr = vec_pool.get();
		Vector	*vstr = vec_pool.get();
		ProcObj	*p = (which == SETUP_PROCESS) ? pobjs[0]: 0;
		int	nsig = get_signals_with_status(p, o, vptr, vstr);

		siglist->set_list(nsig, (const char **)vptr->ptr());
		vec_pool.put(vptr);
		vec_pool.put(vstr);
	}
	else
		siglist->set_list(0, 0);
}

#if EXCEPTION_HANDLING
Exception_dialog::Exception_dialog(Base_window *bw)
	: PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_exception, LAB_exception_mne, 
			(Callback_ptr)&Exception_dialog::apply },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)&Exception_dialog::apply },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Exception_dialog::reset },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Toggle_data toggles[] =
	{
		{ LAB_throw,	TRUE,	0 },
		{ LAB_catch,	TRUE,	0 },
	};

	Expansion_box	*ebox;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_exception, (Callback_ptr)&Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_exception_dialog);
	ebox = new Expansion_box(dialog, "exception box", OR_vertical);
	component_init(ebox);

	caption = new Caption(ebox, LAB_stop_on_line, CAP_LEFT);
	throw_catch = new Toggle_button(caption, "throw or catch", toggles,
		sizeof(toggles)/sizeof(Toggle_data), OR_horizontal, this);
	caption->add_component(throw_catch, FALSE);
	ebox->add_component(caption);

	caption = new Caption(ebox, LAB_type_line, CAP_LEFT);
	type = new Text_line(caption, "type", "", 25, 1);
	caption->add_component(type);
	ebox->add_component(caption);

	caption = new Caption(ebox, LAB_commands_line, CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 20, 1);
	caption->add_component(commands);
	ebox->add_component(caption);
	ebox->update_complete();
	dialog->add_component(ebox);

	dialog->set_popup_focus(type);

	doChange = 0;
	eventId  = 0;
	save_cmd = 0;
	save_type = 0;
	throw_catch_state = tc_none;
}

void
Exception_dialog::reset_toggles()
{
	switch (throw_catch_state)
	{
	case tc_both:
		throw_catch->set(0, 1);
		throw_catch->set(1, 1);
		break;

	case tc_throw:
		throw_catch->set(0, 1);
		throw_catch->set(1, 0);
		break;

	case tc_catch:
		throw_catch->set(0, 0);
		throw_catch->set(1, 1);
		break;

	case tc_none:
	default:
		throw_catch->set(0, 0);
		throw_catch->set(1, 0);
		break;
	}
}

Exception_dialog::TC_state
Exception_dialog::get_state(const char *&opts)
{
	if (throw_catch->is_set(0) && throw_catch->is_set(1))
	{
		opts = "throw catch";
		return tc_both;
	}
	else if (throw_catch->is_set(0))
	{
		opts = "throw";
		return tc_throw;
	}
	else if (throw_catch->is_set(1))
	{
		opts = "catch";
		return tc_catch;
	}
	opts = "";
	return tc_none;
}

// Create the Exception event
void
Exception_dialog::apply(Component *, int)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char		*cmd = commands->get_text();
	char		*type_string = type->get_text();
	const char	*tc = "";
	TC_state	toggle_state = get_state(tc);

	if (!type_string || !*type_string)
	{
		dialog->error(E_ERROR, GE_no_type);
		return;
	}

	if (doChange)
	{
		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d %s %s {%s}\n", eventId, tc,
					type_string, cmd);
		else
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"change %d %s %s\n", eventId, tc, type_string);
	}
	else
	{
		if (toggle_state == tc_none)
		{
			dialog->error(E_ERROR, GE_throw_or_catch);
			return;
		}

		// save values for reset
		delete save_cmd;
		save_cmd = makestr(cmd);
		delete save_type;
		save_type = makestr(type_string);
		throw_catch_state = toggle_state;

		if (cmd && *cmd)
		{
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"exception -p %s %s %s {%s}\n",
				make_plist(total, pobjs, 0, level),
				tc, type_string, cmd);
		}
		else
		{
			dispatcher.send_msg(this, window_set->current_obj()->get_id(),
				"exception -p %s %s %s\n",
				make_plist(total, pobjs, 0, level),
				tc, type_string);
		}
	}
	dialog->wait_for_response();
}

void
Exception_dialog::reset(Component *, int)
{
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		commands->set_text(save_cmd);
		type->set_text(save_type);
		reset_toggles();
	}
}

void
Exception_dialog::fillContents(Event *event)
{
	Vector	*v = vec_pool.get();
	char	*p = strip_curlies(event->get_commands(), v);

	commands->set_text(p);
	vec_pool.put(v);

	type->set_text(event->get_condition());

	// throw or catch	
	const char *ptr = strchr(event->get_type_string(), ' ');
	if (ptr && *++ptr)
	{
		if (*ptr == 'T')
		{
			throw_catch->set(0, 1);
			ptr++;
		}
		else
			throw_catch->set(0, 0);

		if (*ptr == 'C')
			throw_catch->set(1, 1);
		else
			throw_catch->set(1, 0);
	}
}

void
Exception_dialog::setChange(int state)
{
	if (doChange == state)
		return;

	doChange = state;
	if (doChange)
	{
		dialog->set_label(LAB_change, LAB_change_mne);
		dialog->set_help(HELP_change_dialog);
	}
	else
	{
		dialog->set_label(LAB_exception, LAB_exception_mne);
		dialog->set_help(HELP_exception_dialog);
		reset(0,0);
	}
}

Ignore_exceptions_dialog::Ignore_exceptions_dialog(Base_window *bw)
	: PROCESS_DIALOG(bw, true)
{
	static const DButton	buttons[] =
	{
		{ B_exec, LAB_ignore, LAB_ignore_mne, 
			(Callback_ptr)&Ignore_exceptions_dialog::ignore_cb },
		{ B_exec, LAB_stop, LAB_stop_mne,
			(Callback_ptr)&Ignore_exceptions_dialog::stop_cb },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Ignore_exceptions_dialog::cancel},
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
	};

	Packed_box	*box;
	Caption		*caption;

	// Temporarily initialize list with a blank entry.
	// The list will be set correctly later in set_process
	const char	*initial_string[1] = { "" };

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_ignore_exceptions, 0, this,
		buttons, sizeof(buttons)/sizeof(DButton),
		HELP_ignore_exceptions_dialog);
	box = new Packed_box(dialog, "exceptions box", OR_vertical);
	component_init(box);

	list_type = SETUP_PROCESS;
	caption = new Caption(box, LAB_set_eh_disp_for, CAP_LEFT);
	which_list = new Radio_list(caption, "which_list", 
		OR_horizontal, list_buttons,
		sizeof(list_buttons)/sizeof(LabelId), list_type,
		(Callback_ptr)&Ignore_exceptions_dialog::set_list_type, this);
	caption->add_component(which_list);
	box->add_component(caption);

	caption = new Caption(box, LAB_eh_disposition, CAP_TOP_LEFT);
	ehlist = new Selection_list(caption, "exception list", 2, 1, 
		sel_col, 1, initial_string, 
		SM_multiple, this, 
		Search_random, 0,
		0, 0,
		(Callback_ptr)&Ignore_exceptions_dialog::default_cb);
	caption->add_component(ehlist);
	box->add_component(caption);

	box->update_complete();
	dialog->add_component(box);
}

void
Ignore_exceptions_dialog::set_list_type(Component *, int which)
{
	dialog->clear_msg();
	set_list(which);
}

void
Ignore_exceptions_dialog::ignore_cb(Component *, int)
{
	common_apply(TRUE);
}

void
Ignore_exceptions_dialog::stop_cb(Component *, int)
{
	common_apply(FALSE);
}

void
Ignore_exceptions_dialog::common_apply(Boolean ignore)
{
	dialog->clear_msg();
	if (!pobjs && (which_list->which_button() == SETUP_PROCESS))
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	int	num = ehlist->get_selections(v);

	if (!num)
	{
		dialog->error(E_ERROR, GE_no_selection);
		vec_pool.put(v);
		return;
	}

	do_it(num, (int *)v->ptr(), ignore);
	vec_pool.put(v);
}

void
Ignore_exceptions_dialog::do_it(int num, int *selections, Boolean ignore)
{
	const char *tc;
	if (num == 2)
		tc = "throw catch";
	else if (*selections == 0)
		tc = "throw";
	else
		tc = "catch";

	if (which_list->which_button() == SETUP_PROCESS)
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"exception -p %s %s %s\n",
			make_plist(total, pobjs, 0, level),
			(ignore == TRUE) ? "-i" : "", tc);
	else
		dispatcher.send_msg(this, 0, "exception -d %s %s\n",
			(ignore == TRUE) ? "-i" : "", tc);
	dialog->wait_for_response();
}

void
Ignore_exceptions_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs && (which_list->which_button() == SETUP_PROCESS))
	{
		dialog->error(E_ERROR, GE_selection_gone);
		dialog->default_done();
		return;
	}
	do_it(1, &item_index, TRUE); 	// default button is Ignore
	// no need to call default_done since we're always
	// waiting for cli response
}

// If the dialog is pinned, the list should be updated to reflect the
// current state.  If the dialog is not pinned, this isn't necessary, since
// the list will be updated anyway the next time the dialog is popped up
void
Ignore_exceptions_dialog::cmd_complete()
{
	if (dialog->is_pinned())
		set_list(list_type);
	dialog->cmd_complete();
}

void
Ignore_exceptions_dialog::set_obj(Boolean)
{
	int	which;
	dialog->set_busy(TRUE);
	dialog->clear_msg();
	if (!pobjs)
	{
		which = SETUP_DEFAULTS;
		which_list->set_button(SETUP_DEFAULTS);
		// set insensitive by Process_dialog::set_plist
		dialog->set_sensitive(TRUE);
	}
	else
		which = which_list->which_button();
	set_list(which);
	dialog->set_busy(FALSE);
}

void
Ignore_exceptions_dialog::cancel(Component *, int)
{
	which_list->set_button(list_type);
	if (dialog->is_pinned())
		set_list(list_type);
}

void
Ignore_exceptions_dialog::set_list(int which)
{
	Vector		*veh = vec_pool.get();
	const char	*ptrs[2];
	Message		*msg;
	int		nstrs = 0;

	if (which != SETUP_DEFAULTS && !pobjs)
		ehlist->set_list(0, 0);

	if (which_list->which_button() == SETUP_PROCESS)
	{
		if (!pobjs)
		{
			ehlist->set_list(0, 0);
			return;
		}
		dispatcher.query(0, 0, "exception\n");
	}
	else
		dispatcher.query(0, 0, "exception -d\n");
	veh->clear();
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch (msg->get_msg_id())
		{
		case MSG_eh_stopped_default:
		case MSG_eh_ignored_default:
		{
			char	*str = msg->format();
			size_t	len = strlen(str);
			str[len - 1] = '\0';	// get rid of newline
			char	*cp = strchr(str, '\t');
			if (cp)
				*cp = ' ';	// get rid of tab
			veh->add(str, len);
			nstrs++;
			break;
		}
		case ERR_no_exceptions:
			dialog->error(msg);
			break;

		default: // ignore everything else
			continue;
		}
	}

	// get pointers
	if (nstrs)
	{
		ptrs[0] = (char *)veh->ptr();
		ptrs[1] = ptrs[0] + strlen(ptrs[0]) + 1;
		ehlist->set_list(nstrs, ptrs);
	}
	else
		ehlist->set_list(0, 0);
	vec_pool.put(veh);
}

#endif // EXCEPTION_HANDLING
