#ident	"@(#)debugger:gui.d/common/Dialogs.C	1.62"

#include "gui_msg.h"
#include "Dialogs.h"
#include "Ps_pane.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Sel_list.h"
#include "Stext.h"
#include "Toggle.h"
#include "Text_line.h"
#include "Radio.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Proclist.h"
#include "UI.h"
#include "FileInfo.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "Label.h"

#include "Language.h"
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"
#include "str.h"
#include "Buffer.h"
#include "Vector.h"
#include "Path.h"

#include <string.h>
#include <stdio.h>
#include <sys/param.h>	// for MAXPATHLEN

static const char	*create_args;
Language	cur_language;

void
Dialog_box::show_error(Message *m, Boolean stay_up)
{
	Msg_id	mtype = m->get_msg_id();

	// command-line specific messages
	if (mtype == ERR_cmd_pointer || mtype == ERR_asis
		|| mtype == ERR_syntax)
		return;

	// errors are displayed in the dialog to indicate the command failed.
	// for warnings, since the command completes and the dialog pops down,
	// the message is displayed in a separate notice box
	if (mtype == ERR_syntax_loc)
		dialog->error(E_ERROR, GE_syntax_error);
	else if (stay_up || m->get_severity() == E_ERROR)
		dialog->error(m);
	else
		display_msg(m);
}

void
Dialog_box::de_message(Message *m)
{
	if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
		show_error(m, FALSE);
}

void
Dialog_box::display()
{
	if (!dialog->is_open())
		window_set->change_state.add(this, (Notify_func)(&Dialog_box::state_change_cb), 0);
	dialog->popup();
}

void
Dialog_box::dismiss()
{
	window_set->change_state.remove(this,  (Notify_func)(&Dialog_box::state_change_cb), 0);
}

void
Dialog_box::cmd_complete()
{
	dialog->cmd_complete();
}

void
Dialog_box::state_change_cb(void *, int rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
		dialog->set_sensitive(FALSE);
	else if (rc == RC_end_script)
		dialog->set_sensitive(TRUE);
}

// overrides Command_sender virtual
Base_window *
Dialog_box::get_window()
{
	return window;
}

// sp (single process) defines wether only one process is required
// by the process dialog even if there are many processes. This is
// specifically used by the exception and ignore event dialogs.
Process_dialog::Process_dialog(Base_window *bw, Boolean sp) : DIALOG_BOX(bw)
{
#ifdef DEBUG_THREADS
	level = THREAD_LEVEL;
#else
	level = PROCESS_LEVEL;
#endif
	total = 0;
	pobjs = 0;
	obj_list = 0;
	obj_caption = 0;
	track_current = FALSE;
        single_process = sp;
}

void
Process_dialog::component_init(Packed_box *box)
{
	obj_caption = new Caption(box, LAB_process_line, CAP_LEFT);
	obj_list = new Simple_text(obj_caption, "", TRUE);
	obj_caption->add_component(obj_list);
	box->add_component(obj_caption);
}

void
Process_dialog::component_init(Expansion_box *box)
{
	obj_caption = new Caption(box, LAB_process_line, CAP_LEFT);
	obj_list = new Simple_text(obj_caption, "", TRUE);
	obj_caption->add_component(obj_list);
	box->add_component(obj_caption);
}

// set_plist is called when the dialog is first popped up to create the list
// of procobjs it will operate on.  That list contains either the selections
// from the process pane, or, if there are no selections, just the current procobj.
// It also registers with the window set to be notified if the state of one of
// those procobj changes.
void
Process_dialog::set_plist(Base_window *win, unsigned char lev)
{
	if (!obj_list || !obj_caption)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (dialog->is_open())
	{
		// clean up from previous invocation
		if (track_current)
		{
			window_set->change_current.remove(this,
				(Notify_func)(&Process_dialog::update_current_cb), 0);
		}
		else
		{
			window_set->change_any.remove(this,
				(Notify_func)(&Process_dialog::update_list_cb), 0);
		}
	}

	delete pobjs;
	level = lev;
        
	// Only create a list of processes if multiple selection and
	// single_process is false.
	if (win->selection_type() == SEL_process && !single_process)
	{
		Vector	*v = vec_pool.get();

	 	total = win->get_selections(v);

		pobjs = new ProcObj *[total];
		memcpy(pobjs, v->ptr(), total * sizeof(ProcObj *));
		vec_pool.put(v);

		track_current = FALSE;
		window_set->change_any.add(this,
			(Notify_func)(&Process_dialog::update_list_cb), 0);
	}
	else if (window_set->current_obj())
	{
		total = 1;
		pobjs = new ProcObj *[1];
		pobjs[0] = window_set->current_obj();

		// Display warning if a selection exists
		if (win->selection_type() == SEL_process)
		{
			Vector	*v = vec_pool.get();
	 		int total_tmp = win->get_selections(v);

			vec_pool.put(v);

			// Display appropriate message
	 		if (total_tmp == 1)
			{
				display_msg (E_WARNING, GE_invalid_proc_select);
			}
			else
			{
				display_msg (E_WARNING,
						GE_invalid_procs_select);
			}
		}

		track_current = TRUE;
		window_set->change_current.add(this,
			(Notify_func)(&Process_dialog::update_current_cb), 0);
	}
	else
	{
		total = 0;
		track_current = FALSE;
		window_set->change_any.add(this,
			(Notify_func)(&Process_dialog::update_list_cb), 0);
		pobjs = 0;
		obj_list->set_text("");
		dialog->set_sensitive(FALSE);
		set_obj(TRUE);
		return;
	}

	// set caption label

	LabelId	label;
	if (level == PROGRAM_LEVEL)
		label = total > 1 ? LAB_programs_line : LAB_program_line;
	else if (level == PROCESS_LEVEL)
		label = total > 1 ? LAB_processes_line : LAB_process_line;
#ifdef DEBUG_THREADS
	else
	{
		// if all processes then "Process(es)"
		// if all threads then "Thread(s)"
		// else "Selection(s)"
		int process_cnt = 0;
		for (int i=0; i < total; ++i)
			if (!pobjs[i]->is_thread())
				++process_cnt;
		if (process_cnt == total)
			label = total > 1 ? LAB_processes_line : LAB_process_line;
		else if (process_cnt == 0)
			label = total > 1 ? LAB_threads_line : LAB_thread_line;
		else
			label = total > 1 ? LAB_selections_line : LAB_selection_line;
	}
#endif
	obj_caption->set_label(labeltab.get_label(label));
	obj_list->set_text(make_plist(total, pobjs, 1, level));
	dialog->set_sensitive(TRUE);
	set_obj(FALSE);
}

void
Process_dialog::dismiss_cb(Component *, void *)
{
	if (track_current)
		window_set->change_current.remove(this,
			(Notify_func)(&Process_dialog::update_current_cb), 0);
	else
		window_set->change_any.remove(this,
			(Notify_func)(&Process_dialog::update_list_cb), 0);
	delete pobjs;
	pobjs = 0;
}

// update_list_cb is called whenever any process changes state.
// The dialog is only affected if a process is in its list.

void
Process_dialog::update_list_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (!total && (rc == RC_set_current))
	{
		// no current object - only for dialogs that
		// are sensitive without a process
		update_current_cb(0, rc, 0, proc);
		return;
	}
	for (int i = 0; i < total; i++)
	{
		if (pobjs[i] != proc)
			continue;

		update_obj(proc, rc);
		if (rc == RC_delete)
		{
			--total;
			if (total)
			{
				for (int j = i; j < total; j++)
					pobjs[j] = pobjs[j+1];
				obj_list->set_text(make_plist(total, pobjs, 1, 
					level));
			}
			else
			{
				delete pobjs;
				pobjs = 0;
				dialog->set_sensitive(FALSE);
			}
			break;
		}
	}
}

void
Process_dialog::update_current_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (rc == RC_delete)
	{
		delete pobjs;
		dialog->set_sensitive(FALSE);
		pobjs = 0;
		total = 0;
		obj_list->set_text("");
		set_obj(TRUE);	// let the derived class know the current
					// proc died
		return;
	}
	else if (rc == RC_set_current)
	{
		if (!proc)
			// this should always be preceeded by RC_delete
			return;

		if (!pobjs)
		{
			pobjs = new ProcObj *[1];
			dialog->set_sensitive(TRUE);
		}
		
		pobjs[0] = proc;
		total = 1;
		const char	*label;
		if (level == PROGRAM_LEVEL)
			label = proc->get_program()->get_name();
		else if (level == PROCESS_LEVEL)
			label = proc->get_process()->get_name();
		else
			label = proc->get_name();
		obj_list->set_text(label);
		set_obj(TRUE);
	}

	if (rc == RC_start_script || rc == RC_end_script || has_assoc_cmd)
		return;

	if (!proc->is_incomplete())
		update_obj(proc, rc);
}

void
Process_dialog::set_obj(Boolean)
{
}

void
Process_dialog::update_obj(ProcObj *, Reason_code)
{
}

Object_dialog::Object_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	current_object = saved_object = -1;
	save_proc = 0;
}

Component *
Object_dialog::make_obj_list(Component *parent)
{
	Caption		*caption;
	const char	*initial_string = "";

	static const Sel_column	sel_col[] =
	{
		{ SCol_text, 0 },
	};

	caption = new Caption(parent, LAB_objects_line, CAP_TOP_LEFT);
	objects = new Selection_list(caption, "objects", 4, 1, sel_col, 
		1, &initial_string, SM_single, this, Search_none, 0,
		(Callback_ptr)(&Object_dialog::select_cb));
	caption->add_component(objects);
	return caption;
}

void
Object_dialog::select_cb(Selection_list *, int selection)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	dialog->set_busy(TRUE);
	dialog->clear_msg();
	current_object = selection;
	object_changed(objects->get_item(selection, 0));
	dialog->set_busy(FALSE);
}

void
Object_dialog::cancel_change()
{
	if (!pobjs)
	{
		saved_object = current_object = -1;
		objects->set_list(0,0);
		object_changed(0);
		return;
	}

	if (saved_object == -1 || saved_object == current_object)
		return;
 
	objects->select(saved_object);
	object_changed(objects->get_item(saved_object, 0));
}

void
Object_dialog::set_obj(Boolean reset)
{
	if (!pobjs)
	{
		objects->set_list(0, 0);
		saved_object = -1;
		current_object = -1;
		delete save_proc;
		save_proc = 0;
		object_changed(0);
		return;
	}

	if (reset)
		dialog->set_busy(TRUE);

	ProcObj	*proc = pobjs[0];
	Vector	*v = vec_pool.get();
	int	total = proc->get_objects(v);
	const char **strings = (const char **)v->ptr();
	int	i;

	if (total == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		dialog->set_busy(FALSE);
		return;
	}

	// If the name of one of the objects matches
	// the program name, use that as the initial selection,
	// otherwise use the first object
	const char	*obj_name = proc->get_program()->get_name();

	// initialize the object list, even if the process is the same
	// as the last time the window was popped up, since the shared
	// objects in use may be different.  The selected object can,
	// however, stay the same.
	if (save_proc && strcmp(save_proc, proc->get_name()) == 0)
	{
		if (saved_object > -1)
		{
			obj_name = objects->get_item(saved_object, 0);
		}
	}
	else
	{
		delete save_proc;
		save_proc = makestr(proc->get_name());
	}

	for (i = 0; i < total; i++)
	{
		if (strcmp(strings[i], obj_name) == 0)
		{
			break;
		}
	}
	objects->set_list(total, strings);
	vec_pool.put(v);

	if (reset)
		dialog->set_busy(FALSE);
}

// pure virtual function
void
Object_dialog::object_changed(const char *)
{
}

#define IO_TOGGLE	0
#define FOLLOW_PROC_TOGGLE	1
#define KILL_TOGGLE	2
#define NEW_SET_TOGGLE	3

static const Toggle_data create_toggles[] =
{
	{ LAB_capture_io, TRUE,	0 },
	{ LAB_follow, TRUE, 0 },
	{ LAB_kill_previous, FALSE, 0 },
	{ LAB_new_window_set, FALSE, 0 },
};

Create_dialog::Create_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_create, LAB_create_mne, 
			(Callback_ptr)(&Create_dialog::do_create)},
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Create_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Caption		*caption;
	Packed_box	*box;

	follow_state = TRUE;
	io_state = TRUE;
	new_window_state = FALSE;
	kill_state = FALSE;
	if (create_args)
		save_cmd = makestr(create_args);
	else
		save_cmd = 0;
	save_location = makestr("main");
	save_ws = window_set;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_create, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_create_dialog, (Callback_ptr)&Create_dialog::drop_cb,
		Drop_cb_stayup);
	box = new Packed_box(dialog, "create box", OR_vertical);

	caption = new Caption(box, LAB_command_line, CAP_LEFT);
	cmd_line = new Text_line(caption, "command line", 
		create_args ? create_args : "", 25, 1);
	caption->add_component(cmd_line);
	box->add_component(caption);

	caption = new Caption(box, LAB_starting_loc_line, CAP_LEFT);
	start_loc = new Text_line(caption, "start location", "main", 25, 1);
	caption->add_component(start_loc);
	box->add_component(caption);

	toggles = new Toggle_button(box, "toggles", create_toggles,
		sizeof(create_toggles)/sizeof(Toggle_data), OR_vertical);
	box->add_component(toggles);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(cmd_line);
}

void
Create_dialog::set_create_args(const char *s)
{
	delete save_cmd;
	save_cmd = makestr(s);
	cmd_line->set_text(save_cmd);
}

void
Create_dialog::drop_cb(Component *, void *)
{
	char *s;

	if(s = dialog->get_drop_item())
	{
		cmd_line->set_text(s);
		// in anticipation of cmd args
		cmd_line->set_cursor(strlen(s));
	}
}

void
Create_dialog::do_create(Component *, void *)
{
	char		*s = cmd_line->get_text();
	const char	*r, *f;
	const char	*dashl;
	char		*loc = start_loc->get_text();

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_cmd_line);
		return;
	}
	delete save_cmd;
	save_cmd = makestr(s);

	if (toggles->is_set(KILL_TOGGLE))
	{
		// kill processes resulting from previous create command
		proclist.kill_all(dispatcher.get_create_id() - 1, save_ws);
		kill_state = TRUE;
	}
	else
		kill_state = FALSE;

	if (loc && *loc)
		dashl = "-l";
	else
		dashl = "";
	delete save_location;
	save_location = makestr(loc);

	if (toggles->is_set(IO_TOGGLE))
	{
		io_state = TRUE;
		r = "-r";
	}
	else
	{
		io_state = FALSE;
		r = "-d";
	}

	if (toggles->is_set(FOLLOW_PROC_TOGGLE))
	{
		follow_state = TRUE;
		f = "procs";
	}
	else
	{
		follow_state = FALSE;
		f = "none";
	}

	dispatcher.send_msg(this, 0, "create %s %s %s -f %s %s\n", r, dashl,
		save_location, f, s);
	dialog->wait_for_response();

	// window_set is used by the Dispatcher to determine where to send messages,
	// so the window set must be created before the messages come back
	if (toggles->is_set(NEW_SET_TOGGLE))
	{
		new_window_state = TRUE;
		window_set = new Window_set();
	}
	else
	{
		new_window_state = FALSE;
		window_set = save_ws;
	}
}

void
Create_dialog::cancel(Component *, void *)
{
	cmd_line->set_text(save_cmd);
	start_loc->set_text(save_location);
	toggles->set(IO_TOGGLE, io_state);
	toggles->set(FOLLOW_PROC_TOGGLE, follow_state);
	toggles->set(NEW_SET_TOGGLE, new_window_state);
	toggles->set(KILL_TOGGLE, kill_state);
}

void
Create_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
	{
		if (mtype == ERR_create_fail)
			display_msg(m);
		else
			show_error(m, FALSE);
	}
}

// set_create_args is called whenever MSG_createp is received, no matter where
// the create command originated.  The Create windows in all the window sets
// must be updated.

void
set_create_args(Message *m)
{
	Window_set	*ws;
	char		*s;

	m->unbundle(s);
	delete (char *)create_args;
	create_args = makestr(s);

	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_create_dialog(create_args);
}

#define GP_FOLLOW_PROC_TOGGLE	0
#define GP_NEW_SET_TOGGLE	1

Grab_process_dialog::Grab_process_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_grab_proc, LAB_grab_proc_mne,
			(Callback_ptr)&Grab_process_dialog::apply},
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Grab_process_dialog::cancel },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Toggle_data grab_toggles[] =
	{
		{  LAB_follow, TRUE, 0 },
		{  LAB_new_window_set, FALSE, 0 },
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_numeric,	0 },
		{ SCol_text,	0 },
	};

	Caption		*caption;
	Expansion_box	*box;
	int		num_proc;

	follow_state = TRUE;
	new_window_state = FALSE;
	save_obj = 0;
	save_ws = window_set;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_grab_proc, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_grab_process_dialog);

	box = new Expansion_box(dialog, "grab box", OR_vertical);
	caption = new Caption(box, LAB_grab_proc_list, CAP_TOP_LEFT);

	Vector	*v1 = vec_pool.get();
	Vector	*v2 = vec_pool.get();
	num_proc = do_ps(v1, v2);
	ps_list = new Selection_list(caption, "Processes", 5, 2, sel_col, 
			num_proc, (const char **)v2->ptr(), SM_multiple, this,
			Search_random, 1, 0, 0,
			(Callback_ptr)(&Grab_process_dialog::default_cb),
			(Callback_ptr)(&Grab_process_dialog::drop_cb));
	vec_pool.put(v1);
	vec_pool.put(v2);
	caption->add_component(ps_list);
	box->add_component(caption, TRUE);

	caption = new Caption(box, LAB_object_file_line, CAP_LEFT);
	object_file = new Text_line(caption, "object file", "", 25, 1);
	caption->add_component(object_file, FALSE);
	box->add_component(caption);

	toggles = new Toggle_button(box, "toggles", grab_toggles, 
		sizeof(grab_toggles)/sizeof(Toggle_data), OR_vertical);
	box->add_component(toggles);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(object_file);
}

// reinitialize the process list in an existing dialog
void
Grab_process_dialog::setup()
{
	Vector	*v1 = vec_pool.get();
	Vector	*v2 = vec_pool.get();
	int	num_proc;

	window_set = save_ws;
	num_proc = do_ps(v1, v2);
	ps_list->set_list(num_proc, (const char **)v2->ptr());
	vec_pool.put(v1);
	vec_pool.put(v2);
}

void
Grab_process_dialog::apply(Component *, void *)
{
	if (toggles->is_set(GP_NEW_SET_TOGGLE))
	{
		new_window_state = TRUE;
		do_it(0);
	}
	else
	{
		new_window_state = FALSE;
		do_it(save_ws);
	}
}

void
Grab_process_dialog::do_it(Window_set *ws)
{
	int		npids;
	Vector		*v = vec_pool.get();

	npids = ps_list->get_selections(v);
	if (!npids)
	{
		dialog->error(E_ERROR, GE_no_process_selection);
		vec_pool.put(v);
		return;
	}
	if (!do_grab(npids, (int *)v->ptr()))
	{
		vec_pool.put(v);
		return;
	}
	vec_pool.put(v);
	if (ws)
		window_set = ws;
	else
		window_set = new Window_set();
}

Boolean
Grab_process_dialog::do_grab(int npids, int *pids)
{
	const char	*f;
	char		*obj_filename;

	delete save_obj;
	save_obj = 0;
	obj_filename = object_file->get_text();
	if (obj_filename && *obj_filename)
		save_obj = makestr(obj_filename);

	if (save_obj && npids > 1)
	{
		dialog->error(E_ERROR, GE_only_one);
		return FALSE;
	}

	if (toggles->is_set(GP_FOLLOW_PROC_TOGGLE))
	{
		follow_state = TRUE;
		f = "procs";
	}
	else
	{
		follow_state = FALSE;
		f = "none";
	}

	Buffer	*buffer	= buf_pool.get();
	buffer->clear();
	for (int i = 0; i < npids; i++, pids++)
	{
		buffer->add(ps_list->get_item(*pids, 0));
		buffer->add(' ');
	}

	if (save_obj)
		dispatcher.send_msg(this, 0, "grab -f %s -l %s %s\n", f, save_obj, 
			(char *)*buffer);
	else
		dispatcher.send_msg(this, 0, "grab -f %s %s\n", f, (char *)*buffer);
	dialog->wait_for_response();
	buf_pool.put(buffer);
	return TRUE;
}

void
Grab_process_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (toggles->is_set(GP_NEW_SET_TOGGLE))
		new_window_state = TRUE;
	else
		new_window_state = FALSE;
	(void)do_grab(1, &item_index);
	if (new_window_state)
		window_set = new Window_set;
	else
		window_set = save_ws;
	dialog->default_done();
}

void
Grab_process_dialog::cancel(Component *, void *)
{
	if (dialog->is_pinned())
	{
		Vector	*v = vec_pool.get();
		int	npids = ps_list->get_selections(v);
		int	*pids = (int *)v->ptr();

		for (int i = 0; i < npids; i++, pids++)
			ps_list->deselect(*pids);
		vec_pool.put(v);
	}

	if (save_obj)
		object_file->set_text(save_obj);
	else
		object_file->set_text("");

	toggles->set(GP_FOLLOW_PROC_TOGGLE, follow_state);
	toggles->set(GP_NEW_SET_TOGGLE, new_window_state);
}

void
Grab_process_dialog::drop_cb(Selection_list *, Component *dropped_on)
{
	if (dropped_on)
	{
		Base_window	*window = dropped_on->get_base();
		do_it(window->get_window_set());
	}
	else
		do_it(0);
	dialog->popdown();
}

Grab_core_dialog::Grab_core_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_grab_core, LAB_grab_core_mne,
			(Callback_ptr)&Grab_core_dialog::apply},
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Grab_core_dialog::cancel },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Toggle_data new_set_data[] = { LAB_new_window_set, 
		FALSE, 0 };

	Caption		*caption;
	Packed_box	*box;

	save_core = 0;
	save_obj = 0;
	save_toggle = FALSE;
	save_ws = window_set;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_grab_core, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_grab_core_dialog, (Callback_ptr)&Grab_core_dialog::drop_cb,
		Drop_cb_stayup);
	box = new Packed_box(dialog, "grab box", OR_vertical);

	caption = new Caption(box, LAB_core_file_line, CAP_LEFT);
	core_file = new Text_line(caption, "core file", "", 25, 1);
	caption->add_component(core_file);
	box->add_component(caption);

	caption = new Caption(box, LAB_object_file_line, CAP_LEFT);
	object_file = new Text_line(caption, "object file", "", 25, 1);
	caption->add_component(object_file);
	box->add_component(caption);

	new_set = new Toggle_button(box, "new set", new_set_data, 1, OR_vertical);
	box->add_component(new_set);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(core_file);
}

void
Grab_core_dialog::drop_cb(Component *, void *)
{
	char *s = dialog->get_drop_item();

	if(s)
	{
		FileInfo file(s);

		switch(file.type())
		{
		case FT_EXEC:
			object_file->set_text(s);
			break;
		case FT_CORE:
			core_file->set_text(s);
			object_file->set_text(file.get_obj_name());
			break;
		default:
			dialog->error(E_ERROR, GE_bad_drop);
			break;
		}
	}
}

void
Grab_core_dialog::apply(Component *, void *)
{
	char	*s1 = object_file->get_text();
	char	*s2 = core_file->get_text();
	if (!s1 || !*s1)
	{
		dialog->error(E_ERROR, GE_no_file);
		return;
	}

	if (!s2 || !*s2)
	{
		dialog->error(E_ERROR, GE_no_core_file);
		return;
	}

	delete save_obj;
	delete save_core;
	save_obj = makestr(s1);
	save_core = makestr(s2);

	dispatcher.send_msg(this, 0, "grab -c %s %s\n", save_core, save_obj);
	dialog->wait_for_response();

	if (new_set->is_set(0))
		window_set = new Window_set();
	else
		window_set = save_ws;
	save_toggle = new_set->is_set(0);
}

void
Grab_core_dialog::cancel(Component *, void *)
{
	object_file->set_text(save_obj);
	core_file->set_text(save_core);
	new_set->set(0, save_toggle);
}

#define DERIVED_button	0
#define C_button	1
#define CPLUS_button	2

static int
lang_to_button(Language lang)
{
	switch (lang)
	{
	case UnSpec:		return DERIVED_button;
	case C:			return C_button;
	case CPLUS_ASSUMED:
	case CPLUS_ASSUMED_V2:
	case CPLUS:		return CPLUS_button;

	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return DERIVED_button;
	}
}

Set_language_dialog::Set_language_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none, 
			(Callback_ptr)&Set_language_dialog::apply },
		{ B_reset, LAB_none, LAB_none, 
			(Callback_ptr)&Set_language_dialog::reset },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Set_language_dialog::reset },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const LabelId language_buttons[] =
	{
		LAB_lang_none,
		LAB_lang_c,
		LAB_lang_c_plus,
	};

	Packed_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_language, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_language_dialog);
	box = new Packed_box(dialog, "language box", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_current_lang_line, CAP_LEFT);
	current_lang = new Simple_text(caption, 
		labeltab.get_label(LAB_lang_none), TRUE);
	caption->add_component(current_lang);
	box->add_component(caption);
	
	caption = new Caption(box, LAB_override_line, CAP_LEFT);
	lang_choices = new Radio_list(caption, "language", OR_horizontal,
		language_buttons, sizeof(language_buttons)/sizeof(LabelId),
		lang_to_button(cur_language));
	caption->add_component(lang_choices, FALSE);
	box->add_component(caption);
	box->update_complete();
	dialog->add_component(box);
}

void
Set_language_dialog::apply(Component *, void *)
{
	const char	*language;
	int		lang = lang_choices->which_button();

	switch(lang)
	{
	case 0:		language = "\"\"";	cur_language = UnSpec;	break;
	case 1:		language = "C";		cur_language = C;	break;
	case 2:		language = "C++";	cur_language = CPLUS;	break;

	default:	
		dialog->error(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	dispatcher.send_msg(this, 0, "set %%lang = %s\n", language);
	dialog->wait_for_response();
}

void
Set_language_dialog::reset(Component *, void *)
{
	lang_choices->set_button(lang_to_button(cur_language));
}

void
Set_language_dialog::update_obj(ProcObj *pobj, Reason_code rc)
{
	if (rc == RC_fcall_in_expr)
		return;

	if (!pobj)
	{
		current_lang->set_text("None");
		return;	
	}

	Message	*msg;
	dispatcher.query(this, pobj->get_id(), "print %%db_lang\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
		{
			char *buf = 0;
			msg->unbundle(buf);
			if (!buf || !*buf || *buf == '0')
				current_lang->set_text("None");
			else
			{
				// get rid of quotes and newline
				buf[strlen(buf)-2] = '\0';
				current_lang->set_text(buf+1);
			}
		}
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

void
Set_language_dialog::set_obj(Boolean reset)
{
	if (!reset)
		update_obj(pobjs ? pobjs[0] : 0);
	else
		// language dialog is always sensitive
		dialog->set_sensitive(TRUE);
}

// Language is a global property, so the Set_language dialog in each window
// set must be updated whenever the language changes
void
set_lang(Message *m)
{
	Window_set	*ws;
	char		*l;

	m->unbundle(l);
	if (strcmp(l, "C") == 0)
		cur_language = C;
	else if (strcmp(l, "C++") == 0)
		cur_language = CPLUS;
	else
		cur_language = UnSpec;

	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_language_dialog();
}

Cd_dialog::Cd_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_cd, LAB_cd_mne, 
			(Callback_ptr)&Cd_dialog::apply },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Cd_dialog::cancel },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Packed_box	*box;
	Caption		*caption;

	if (!current_dir && init_cwd() == 0)
	{
		// error: no current working directory
		display_msg(E_ERROR, GE_no_cwd);
		return;
	}
	save_text = 0;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_cd, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_cd_dialog);
	box = new Packed_box(dialog, "cd dialog", OR_vertical);

	caption = new Caption(box, LAB_current_dir_line, CAP_LEFT);
	current_directory = new Simple_text(caption, current_dir, FALSE);
	caption->add_component(current_directory);
	box->add_component(caption);

	caption = new Caption(box, LAB_new_dir_line, CAP_LEFT);
	new_directory = new Text_line(caption, "directory", "", 25, 1);
	caption->add_component(new_directory);
	box->add_component(caption);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(new_directory);
}

void
Cd_dialog::apply(Component *, void *)
{
	char		*s;

	delete save_text;
	s = new_directory->get_text();
	save_text = makestr(s ? s : "");

	dispatcher.send_msg(this, 0, "cd %s\n", s);
	dialog->wait_for_response();
}

void
Cd_dialog::cancel(Component *, void *)
{
	new_directory->set_text(save_text);
}

void
Cd_dialog::update_directory(const char *s)
{
	current_directory->set_text(s);
}

#define MOVE_NEW_WS	0
#define	MOVE_OLD_WS	1

Move_dialog::Move_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_move, LAB_move_mne, 
			(Callback_ptr)&Move_dialog::apply },
		{ B_reset, LAB_none, LAB_none, 
			(Callback_ptr)&Move_dialog::reset },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)&Move_dialog::reset },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const LabelId choice_buttons[] =
	{
		LAB_new_ws,
		LAB_old_ws,
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_move, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_move_dialog);
	box = new Expansion_box(dialog, "move box", OR_vertical);
	component_init(box);

	// Move to:
	//	<> a new window set
	//	<> an existing window set
	caption = new Caption(box, LAB_move_line, CAP_LEFT);
	choices = new Radio_list(caption, "choices", OR_vertical,
		choice_buttons, sizeof(choice_buttons)/sizeof(LabelId),
		MOVE_NEW_WS, (Callback_ptr)&Move_dialog::choices_cb, this);
	caption->add_component(choices, FALSE);

	// Existing window sets:
	//	|------|
	//	| ws1  |
	//	|------|
	//	| ws2  |
	//	|------|
	//	| ...  |
	//	|------|
	box->add_component(caption);
	caption = new Caption(box, LAB_old_ws_line, CAP_TOP_LEFT);
	Vector	*v = vec_pool.get();
	int	nws = get_ws_list(v);

	ws_sel_list = new Selection_list(caption, "window sets", 4, 1, sel_col,
		nws, (const char **)v->ptr(), SM_single, this, 
		Search_none, 0, 0, 0, 
		(Callback_ptr)&Move_dialog::default_cb); 
	free_ws_list((char **)v->ptr(), nws);
	vec_pool.put(v);
	ws_sel_list->set_sensitive(FALSE);
	caption->add_component(ws_sel_list);
	box->add_component(caption, TRUE);
	box->update_complete();
	dialog->add_component(box);
}

void
Move_dialog::setup()
{
	// get ws list and make up ws array
	Vector	*v = vec_pool.get();
	int	nws = get_ws_list(v);
	ws_sel_list->set_list(nws, (const char **)v->ptr());
	free_ws_list((char **)v->ptr(), nws);
	vec_pool.put(v);
}

int
Move_dialog::get_ws_list(Vector *v)
{
	int	nws = 0;

	v->clear();
	for (Window_set *ws = (Window_set *)windows.first();
		ws != NULL; ws = (Window_set *)windows.next())
	{
		int	id = ws->get_id();

		if (id != window_set->get_id())
		{
			const char	*lab = 
				labeltab.get_label(LAB_window_set);
			char	*wsnp = new char[strlen(lab) + 
				MAX_INT_DIGITS + 2];
			sprintf(wsnp, "%s %d", lab, id);
			v->add(&wsnp, sizeof(wsnp));
			++nws;
		}
	}
	return nws;
}

void
Move_dialog::free_ws_list(char **list, int nitems)
{
	char **p = list;

	for (int i = 0; i < nitems; ++i, ++p)
		delete *p;
}

void
Move_dialog::choices_cb(Component *, void *)
{
	ws_sel_list->set_sensitive( choices->which_button() == MOVE_OLD_WS ? 
		TRUE : FALSE);
}

void
Move_dialog::apply(Component *, void *)
{
	Vector		*v;
	int		selected_item;

	switch(choices->which_button())
	{
	case MOVE_OLD_WS:
		// get selection in ws_sel_list
		v = vec_pool.get();
		if (ws_sel_list->get_selections(v) == 0)
		{
			dialog->error(E_ERROR, GE_no_window_set);
			vec_pool.put(v);
			return;
		}
		selected_item = *(int *)v->ptr();
		vec_pool.put(v);
		do_it(selected_item);
		break;
	case MOVE_NEW_WS:
		move_objs(new Window_set());
		break;
	}
}

void
Move_dialog::do_it(int index)
{
	Window_set	*target;
	const char	*selection;
	int		id;

	selection = ws_sel_list->get_item(index, 0);
	// assume the string in the form 
	// "LAB_window_set <id>"
	const char *ptr = selection + strlen(selection);
	while(*ptr != ' ')
		ptr--;
	id = atoi(ptr + 1);
	// find ws with that id
	for (target = (Window_set *)windows.first();
		target != NULL; target = (Window_set *)windows.next())
	{
		if (target->get_id() == id)
			break;
	}
	move_objs(target);
}

void
Move_dialog::move_objs(Window_set *dest)
{
	if (dest == NULL)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// make copy of pobj list since we can't be sure
	// how delete_obj (called during move) might affect it
	ProcObj	**plist = new ProcObj *[total];
	memcpy((void *)plist, (const void *)pobjs, 
		(size_t)total*sizeof(ProcObj *));
	proclist.move_objs(plist, total, dest);
	delete plist;
}

void
Move_dialog::default_cb(Component *, int item)
{
	dialog->default_start();
	do_it(item);
	dialog->default_done();
}

void
Move_dialog::reset(Component *, void *)
{
	// set choices to new
	choices->set_button(MOVE_NEW_WS);
	ws_sel_list->set_sensitive(FALSE);
}
