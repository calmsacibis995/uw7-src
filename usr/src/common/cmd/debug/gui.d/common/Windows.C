#ident	"@(#)debugger:gui.d/common/Windows.C	1.128"

// GUI headers
#include "Base_win.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Show_value.h"
#include "Radio.h"
#include "Sense.h"
#include "Resources.h"
#include "Toggle.h"
#include "Text_line.h"
#include "Text_area.h"
#include "Text_disp.h"
#include "Transcript.h"
#include "Stext.h"
#include "Slider.h"
#include "Command.h"
#include "Events.h"
#include "Eventlist.h"
#include "Src_pane.h"
#include "Dis.h"
#include "Sch_dlg.h"
#include "Mem_dlg.h"
#include "Buttons.h"
#include "Menu.h"
#include "Windows.h"
#include "Dispatcher.h"
#include "Proclist.h"
#include "Ps_pane.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "Regs_pane.h"
#include "UI.h"
#include "Timer.h"
#include "Window_sh.h"
#include "gui_label.h"
#include "Label.h"
#include "config.h"
#include "Text_edit.h"
#include "Sel_list.h"
#include "FSdialog.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"
#include "Buffer.h"
#include "str.h"
#include "Vector.h"

#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

List		windows;

static int	next_window;

// make sure selected text does not extend past a newline
char *
truncate_selection(char *s)
{
	if (s)
	{
		char *p = strchr(s, '\n');
		if (p)
			*p = '\0';
	}
	return s;
}

Window_set::Window_set() : COMMAND_SENDER(this), change_current(this),
	change_any(this), change_state(this)
{
	event_action = resources.get_event_action();
	output_action = resources.get_output_action();
#ifdef DEBUG_THREADS
	thread_action = resources.get_thread_action();
#endif

	first_window = 0;
	open_windows = 0;
	animated_pane = 0;

	id = ++next_window;

	head = tail = 0;
	cur_obj = 0;

	create_box = 0;
	grab_process = 0;
	grab_core = 0;
	set_language = 0;
	set_value = 0;
	show_value = 0;
	show_type = 0;
	run_box = 0;
	step_box = 0;
	jump_box = 0;
	stop_box = 0;
	signal_box = 0;
	syscall_box = 0;
	onstop_box = 0;
#if EXCEPTION_HANDLING
	exception_box = 0;
	ignore_exceptions_box = 0;
#endif
	cancel_box = 0;
	kill_box = 0;
	setup_signals = 0;
	path_box = 0;
	granularity_box = 0;
	cd_box = 0;
	action_box = 0;
	animation_box = 0;
	dump_box = 0;
	map_box = 0;
	stop_on_function_dialog = 0;
	move_dlg = 0;
	dis_mode_box = 0;
	frame_dir_box = 0;
	save_open_source_box = 0;

	transcript = 0;

	event_level = resources.get_event_level();
	command_level = resources.get_command_level();

	animated = FALSE;
	step_interval = 5;	// middle position
	timer = 0;

	windows.add(this);
	make_windows();

	// update window menus in all other window_sets
	Window_set	*ws = (Window_set *)windows.first();
	for ( ; ws; ws = (Window_set *)windows.next())
	{
		if (ws == this)
			continue;
		ws->add_ws(id);
		// Base_window constructor will add existing
		// window sets to its list
	}
}

Window_set::~Window_set()
{
	if (head)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	
	windows.remove(this);

	delete create_box;
	delete grab_process;
	delete grab_core;
	delete set_language;
	delete set_value;
	delete show_value;
	delete show_type;
	delete run_box;
	delete step_box;
	delete jump_box;
	delete stop_box;
	delete signal_box;
	delete syscall_box;
	delete onstop_box;
#if EXCEPTION_HANDLING
	delete exception_box;
	delete ignore_exceptions_box;
#endif
	delete cancel_box;
	delete kill_box;
	delete setup_signals;
	delete path_box;
	delete granularity_box;
	delete cd_box;
	delete action_box;
	delete animation_box;
	delete dump_box;
	delete map_box;
	delete stop_on_function_dialog;
	delete transcript;
	delete dis_mode_box;
	delete frame_dir_box;
	delete save_open_source_box;

	for (int i = 0; i < windows_per_set; i++)
		delete base_windows[i];
	delete base_windows;
	delete timer;

	/* update window menus in all other window_sets */
	Window_set	*ws = (Window_set *)windows.first();
	for ( ; ws; ws = (Window_set *)windows.next())
	{
		ws->delete_ws(id);
	}
}

void
Window_set::make_windows()
{
	Window_descriptor	*wdesc = window_descriptor;
	Window_configuration	*wconf, *wtmp;
	int			i;

	// saved window positions
	wconf = window_configuration;
	for(; wconf; wconf = wconf->next())
	{
		if (wconf->get_wid() == id)
			break;
	}
	first_window = 0;
	base_windows = new Base_window *[windows_per_set];
	for (i = 0; wdesc; i++, wdesc = wdesc->next())
	{
		
		for(wtmp = wconf; wtmp; wtmp = wtmp->next())
		{
			if (wtmp->get_wid() != id)
			{
				wtmp = 0;
				break;
			}
			if (wtmp->get_wdesc() == wdesc)
				break;
		}

		base_windows[i] = 0;
		// create window if specified in the saved configuration
		// or if there was no saved information for this
		// window set and the window is marked as auto-popup
		if (wtmp || (!wconf &&
			(wdesc->get_flags() & W_AUTO_POPUP)))
		{
			popup_window(i, TRUE, wtmp);
			if (!first_window)
			{
				first_window = base_windows[i]->get_window_shell();
			}
		}
	}
}

void
Window_set::popup_window_cb(Component *, int index)
{
	popup_window(index);
}

void
Window_set::popup_window(int index, Boolean grab_focus, 
	Window_configuration *wconf)
{
	Base_window		*window = base_windows[index];
	Window_descriptor	*wdesc = window_descriptor;
	Command_pane		*cp;

	for(int i = 0; i < index; i++)
		wdesc = wdesc->next();

	if (!window)
	{
		base_windows[index] = window = new Base_window(this, wdesc, wconf);
	}
	if (((cp = (Command_pane *)window->get_pane(PT_command)) != 0)
		&& transcript && transcript->size())
		cp->get_transcript()->add_text(transcript->get_string());
	if (!window->is_open())
		open_windows++;
	window->popup(grab_focus);
}

// overrides Command_sender virtual
Base_window *
Window_set::get_window()
{
	Base_window	**bw = base_windows;

	for(int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw && ((*bw)->get_window_shell() == first_window))
			break;
	}
	return *bw;
}

// Inform lets the user know when an event occurs or a procobj generates output,
// but inform is called for all messages, because it was easier to decide here
// which ones to handle than to clutter up the Dispatcher with that knowledge.
void
Window_set::inform(Message *m, ProcObj *proc)
{
	Msg_id		mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error && !in_script)
	{
		// if the message has a uicontext - that is, the error is a result
		// of a command from a Command_sender object - the object will have
		// already handled the error.  The Dispatcher calls the object's
		// de_message before calling inform
		if (!m->get_uicontext())
			display_msg(m);
		return;
	}

	// gui-only messages are used to update state information and
	// are not displayed directly to the user
	if (gui_only_message(m))
		return;

	switch (mtype)
	{
	case ERR_cmd_pointer:	// pertinent only to cmd line
		break;

	case MSG_version:	// the result of pressing Version in the Help menu
		display_msg(m);
		break;

	case MSG_es_stepped:
		if (animated || proc->is_animated())
		{
			animate_state = AS_step_done;
			if (!animated)
				proc->clear_animation();
			break;
		}
		else
			goto display_msg;

	case MSG_step_not_done:
		if (animated || proc->is_animated())
		{
			animate_state = AS_step_done;
			// don't clear proc's animation so as to
			// ensure its state change to Stopped
			// (this is how we distinguish between
			// animation steps and "step -c cnt" steps)
			// changing its state to Stopped is important
			// for Panes to update themselves
			break;
		}
		else
			goto display_msg;

#ifdef DEBUG_THREADS
	case MSG_thread_exit:
	case MSG_give_up_lwp:
	case MSG_thr_suspend:
		if (proc->is_animated())
		{
			clear_animation();
			proc->clear_animation();
		}
		// FALLTHRU
	case MSG_thread_create:
	case MSG_thr_continue:
	case MSG_pick_up_lwp:
	case MSG_es_halted_thread_start:
		// check thread_action
		if (!in_script)
		{
			if (thread_action & TCA_beep)
			{
				beep();
			}
			// TCA_stop action is done in 
			// Process_list::set_thread_state()
		}
		goto display_msg;
#endif
	// event notifications or other ProcObj state changes
	case MSG_proc_killed:
	case MSG_proc_fork:
	case MSG_proc_exec:
	case MSG_proc_exit:
	case MSG_es_halted:
	case MSG_es_signal:
	case MSG_es_sysent:
	case MSG_es_sysxit:
	case MSG_es_stop:
#ifdef DEBUG_THREADS
	case MSG_thread_fork:
	case MSG_es_halted_off_lwp:
#endif
#if EXCEPTION_HANDLING
	case MSG_es_eh_thrown:
	case MSG_es_eh_caught:
#endif
		if (!in_script)
		{
			switch (event_action)
			{
			case A_message:	display_msg(m);		break;
			case A_beep:	beep();			break;
			case A_raise:	popup_command();	break;

			case A_none:
			default:
				break;
			}
		}
		if (proc->is_animated())
		{
			clear_animation();
			proc->clear_animation();
		}
		// in addition to the beep, etc., event notifications are
		// always displayed in the bottom of the window, and in
		// the command window
		//FALL-THROUGH

	case MSG_createp:
	case MSG_event_assigned:
	case MSG_new_core:
	case MSG_grab_proc:
	case MSG_release_run:
	case MSG_release_suspend:
display_msg:
		if (!in_script)
		{
			for (int i = 0; i < windows_per_set; i++)
			{
				Base_window	*win = base_windows[i];
				if (win && win->is_open())
				{
					win->get_window_shell()->clear_msg();
					win->get_window_shell()->display_msg(m);
				}
			}
		}
		//FALL-THROUGH

	case MSG_disassembly:
	case MSG_dis_line:
	case MSG_dis_src:
		if (!animated)
		{
			if (in_script)
				add_to_transcript(m, 0, 0);
			else
				add_to_transcript(m, 1, 1);
		}
		break;

	case MSG_loc_sym_file:
	case MSG_loc_sym:
	case MSG_loc_unknown:
	case MSG_line_src:
	case MSG_line_no_src:
	case MSG_es_core:
	case MSG_core_state:
	case MSG_core_state_addr:
		if (!animated)
		{
			if (in_script)
				add_to_transcript(m, 0, 0);
			else
				add_to_transcript(m, 1, 0);
		}
		break;

	case MSG_proc_output:
		if (in_script || animated)
			add_to_transcript(m, 0, 0);
		else
		{
			add_to_transcript(m, 1, 0);
			switch (output_action)
			{
			case A_message:	display_msg(E_NONE, GM_output);	break;
			case A_beep:	beep();				break;
			case A_raise:	popup_command();		break;

			case A_none:
			default:
				break;
			}
		}
		break;

	default:
		if (in_script || !m->get_uicontext())
			add_to_transcript(m, 0, 0);
		break;
	}
}

void
Window_set::add_to_transcript(Message *m, int check_cmd, int check_dis)
{
	Base_window	**win;
	Command_pane	*cp;
	UIcontext	uic;

	if (m->get_msg_flags() & MSG_REDIRECTED)
		// output of command redirected
		return;

	uic = m->get_uicontext();

	if (uic)
	{
		if (check_cmd)
		{
			// don't add to transcript if command pane
			// was the initiator of the command
			cp = (Command_pane *)command_panes.first();
			for(; cp; cp = (Command_pane *)command_panes.next())
			{
				if (cp == uic)
					return;
			}
		}
		if (check_dis)
		{
			// if dis pane was initiator of command,
			// we just display disassembly in the dis
			// pane, not in transcript
			win = base_windows;
			for(int i = 0; i < windows_per_set; i++, win++)
			{
				if (*win && ((*win)->get_pane(PT_disassembler) == uic))
					return;
			}
		}
	}

	char	*text;

	if (m->get_msg_id() == MSG_proc_output)
	{
		char *s1;

		// don't display the pty id
		m->unbundle(s1, text);
	}
	else
		text = m->format();

	command_add_text(text);
}

// set_current may be called with ptr == 0 if the
// current ProcObj is being deleted.  In that case, set_current picks
// an arbitrary ProcObj off the list to be the new current ProcObj
void
Window_set::set_current(ProcObj *ptr)
{
	Ps_pane	*ps_pane;

	if (!ptr)
	{
		for (Plink *link = head; link; link = link->next())
		{
			if (link->procobj()->is_live() || 
			    link->procobj()->is_core())
			{
				ptr = link->procobj();
				break;
			}
		}
	}

	cur_obj = ptr;
	ps_pane = (Ps_pane *)ps_panes.first();
	for(; ps_pane; ps_pane = (Ps_pane *)ps_panes.next())
		ps_pane->set_current(ptr);
	if (in_script || has_assoc_cmd || animated)
	{
		if (ptr)
			ptr->touch();
		else
			change_current_notify(RC_set_current, 0);
	}
	else
	{
		change_current_notify(RC_set_current, ptr);
		change_any.notify(RC_set_current, ptr);
	}
}

#ifdef DEBUG_THREADS
// delete_process removes a process from the window set.
// for threaded process, all of the threads are removed
void
Window_set::delete_process(Process *proc)
{
	if (!proc)
		return;

	if (proc->has_threads())
	{
		for (Plink *link = proc->get_head(); link; link = link->next())
			delete_obj(link->procobj());
		return;
	}
	else 
		delete_obj(proc);
}
#endif

// delete_obj removes a procobj from the window set.
void
Window_set::delete_obj(ProcObj *proc)
{
	Plink	*ptr;
	int	index;
	Ps_pane	*ps_pane;

	if (!proc || !(ptr = find_obj(proc, index)))
		return;

	ps_pane = (Ps_pane *)ps_panes.first();
	for(; ps_pane; ps_pane = (Ps_pane *)ps_panes.next())
		ps_pane->delete_obj(index);
	if (ptr == head)
		head = head->next();
	if (ptr == tail)
		tail = tail->prev();
	ptr->unlink();
	delete ptr;

	if (proc == cur_obj)
	{
		cur_obj = 0;
		change_current_notify(RC_delete, proc);
	}
	change_any.notify(RC_delete, proc);
}

void
Window_set::add_obj(ProcObj *p, Boolean make_current)
{
	int index;
	Ps_pane	*ps_pane;

	(void)add_to_list(find_add_location(index, p), p);
	ps_pane = (Ps_pane *)ps_panes.first();
	for(; ps_pane; ps_pane = (Ps_pane *)ps_panes.next())
		ps_pane->add_obj(index, p);
	if (make_current)
		set_current(p);
}

#ifdef DEBUG_THREADS
void
Window_set::add_process(Process *proc, Boolean make_current)
{
	int	index;
	Plink	*ptr;
	Ps_pane	*ps_pane;

	if (!proc)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	ptr = find_add_location(index, proc);
	if (proc->has_threads())
		for (Plink *link = proc->get_head(); link; link = link->next())
		{
			ptr = add_to_list(ptr, link->thread());
		}
	else
		add_to_list(ptr, proc);

	ps_pane = (Ps_pane *)ps_panes.first();
	for(; ps_pane; ps_pane = (Ps_pane *)ps_panes.next())
		ps_pane->add_process(index, proc);

	if (make_current || !cur_obj)
		set_current(proc->has_threads() ? 
			(ProcObj *)(proc->get_head()->thread()) : 
			(ProcObj *)proc);
}
#endif

// find a place in the window-set's procobj list to insert p
// return both a pointer to and an index of the list item where the 
// insertion should take place. the pointer is 0 if the list is
// empty
Plink *
Window_set::find_add_location(int &index, ProcObj *p)
{
	index = 0;

	// assume p does not already exist in list
	long	pid1;
	int	tid1;
	get_name_components(p->get_name(), pid1, tid1);

	if (head)
	{	
		for (Plink *link = head; link; link = link->next(), index++)
		{
			ProcObj	*p2 = link->procobj();
			int	val = strcmp(p->get_program()->get_name(),
						p2->get_program()->get_name());

			// programs names are ordered alphabetically,
			// processes and threads are numeric
			if (val < 0)
				return link->prev();
			else if (val == 0)
			{
				long	pid2;
				int	tid2;
				get_name_components(p2->get_name(), pid2, tid2);

				if (pid1 < pid2 ||
				    (pid1 == pid2 && tid1 < tid2))
				{
					return link->prev();
				}
			}
		}
		return tail;
	}
	return 0;
}

void
Window_set::get_name_components(const char *name, long &pid, int &tid)
{
	char	*dotp;

	// assumes procobj name always looks like "p<num>" or "p<num>.<num>"
	pid = strtol(name+1, &dotp, 10);
	if (dotp && *dotp == '.')
		tid = atoi(dotp+1);
	else
		tid = 0;
}

Plink *
Window_set::add_to_list(Plink *ptr, ProcObj *p)
{
	Plink *new_link = new Plink(p);
	if (ptr)
	{
		new_link->append(ptr);
		if (ptr == tail)
			tail = new_link;
	}
	else
	{
		if (head)
			new_link->prepend(head);
		else
			tail = new_link;
		head = new_link;
	}
	return new_link;
}

void
Window_set::update_obj(ProcObj *pobj, Reason_code rc)
{
	int	index = 0;

	if (!pobj || !find_obj(pobj, index))
		return;

	if (cur_obj == pobj)
		change_current_notify(rc, pobj);
	change_any.notify(rc, pobj);

	if (animated) 
	{
		if ((cur_obj == pobj) && pobj->is_runnable())
		{
			if (step_interval == 0)
				// no delay set, restart
				timer_cb(0,0);
			else
			{
				// restart after specified delay
				// step_interval is multiple of 1/10-th of a sec
				// convert to milliseconds
				int	msecs = step_interval * 100;
				if (!timer)
					timer = new Timer(this);
				timer->set(msecs, (Callback_ptr)&Window_set::timer_cb);
			}
		}
	}
	else
		update_ps_panes(rc, index, pobj);
}

void
Window_set::timer_cb(Component *, void *)
{
	if (animated_pane) 
	{
		if (animated_pane->get_type() == PT_disassembler)
		{
			dispatcher.send_msg(this, cur_obj->get_id(),
				"step -i -b\n");
			animate_state = AS_step_start;
			return;
		}
		else if (animated_pane->get_type() == PT_source)
		{
			dispatcher.send_msg(this, cur_obj->get_id(),
				"step -b\n");
			animate_state = AS_step_start;
			return;
		}
	}

	display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
}

// rename_process is called if renaming doesn't affect the process's ordering
// in the list, otherwise delete_process is called followed by add_process
void
Window_set::rename_process(Process *proc)
{
	int	index = 0;

#ifdef DEBUG_THREADS
	if (!proc)
		return;
	// update all objects whose process is proc
	for (Plink *ptr = head; ptr; ptr = ptr->next(), ++index)
	{
		ProcObj *obj = ptr->procobj();
		if (proc != obj->get_process())
			continue;
		update_ps_panes(RC_rename, index, obj);
		if (cur_obj == obj)
			change_current_notify(RC_rename, obj);
		change_any.notify(RC_rename, obj);
	}
#else
	if (!proc || !find_obj(proc, index))
		return;

	update_ps_panes(RC_rename, index, proc);
	if (cur_obj == proc)
		change_current_notify(RC_rename, proc);
	change_any.notify(RC_rename, proc);
#endif
}

void
Window_set::set_frame(ProcObj *p)
{
	if (!in_script && !has_assoc_cmd && !animated)
	{
		if (p == cur_obj)
			change_current_notify(RC_set_frame, p);
		change_any.notify(RC_set_frame, p);
	}
}

int
Window_set::get_frame(int frame, const char *&function, const char *&location)
{
	int	index = 0;
	Base_window	**win = base_windows;
	for(; index < windows_per_set; index++, win++)
	{
		Stack_pane	*stack_pane;
		if (*win && ((stack_pane = (Stack_pane *)(*win)->get_pane(PT_stack)) != 0))
			return stack_pane->get_frame(frame, 
				function, location);
	}
	return 0;
}

Plink *
Window_set::find_obj(ProcObj *proc, int &index)
{
	Plink	*ptr;

	for (ptr = head, index = 0; ptr; ptr = ptr->next(), index++)
	{
		if (ptr->procobj() == proc)
			break;
	}
	return ptr;
}

void
Window_set::popup_command()
{
	Command_pane	*cp = (Command_pane *)command_panes.first();
	if (!cp)
	{
		// find first window that has command pane
		int	index = 0;
		Window_descriptor	*wdesc = window_descriptor;
		for(; index < windows_per_set; index++, wdesc = wdesc->next())
		{
			if (wdesc->get_flags() & PT_command)
				break;
		}
		popup_window(index, FALSE);
	}
	else
	{
		if (!cp->get_parent()->is_open())
			open_windows++;
		cp->get_parent()->popup(FALSE);
	}
}

// Call the change current notifier.  If any base windows are
// on the set_line_exclusive list, first set the set_line counts
// for those windows.  After notification, clear the counts.
void
Window_set::change_current_notify(Reason_code rc, ProcObj *pobj)
{
	Base_window	*bw = 
		(Base_window *)set_line_exclusive_wins.first();
	for(; bw; bw = (Base_window *)set_line_exclusive_wins.next())
		bw->set_set_line_count(1);

	change_current.notify(rc, pobj);

	bw = (Base_window *)set_line_exclusive_wins.first();
	for(; bw; bw = (Base_window *)set_line_exclusive_wins.next())
		bw->set_set_line_count(0);
}

static OpenSourceRecord *
add_to_open_list(OpenSourceRecord *list, char *fname,
	Text_editor *edit, StoredFiles *sf)
{
	OpenSourceRecord	*orp = new OpenSourceRecord();
	orp->file = fname;
	orp->editor = edit;
	orp->stored = sf;
	orp->next = list;
	return orp;
}

OpenSourceRecord *
Window_set::build_open_source_list()
{
	Window_set	*ws = (Window_set *)windows.first();
	Base_window	*win;
	Source_pane	*sp;
	Text_editor	*edit;
	StoredFiles	*sf;
	OpenSourceRecord	*open_list = 0;

	for ( ; ws; ws = (Window_set *)windows.next())
	{
		Base_window	**bw = base_windows;
		for(int i = 0; i < windows_per_set; i++, bw++)
		{
			if (!*bw ||
				((sp = (Source_pane *)((*bw)->get_pane(PT_source))) == 0))
				continue;
			edit = sp->get_pane();
			if (sp->current_changed())
				open_list = add_to_open_list(
					open_list,
					sp->get_current_path(), edit, 0);
			sf = edit->get_stored_files();
			for(; sf; sf = sf->get_next())
				open_list = add_to_open_list(
					open_list, 0, edit, sf);
		}
		win = (Base_window *)source_windows.first();
		for(; win; win = (Base_window *)source_windows.next())
		{
			if ((sp = (Source_pane *)win->get_pane(PT_second_src)) == 0)
				continue;
			edit = sp->get_pane();
			if (sp->current_changed())
				open_list = add_to_open_list(
					open_list,
					sp->get_current_path(), edit, 0);
			sf = edit->get_stored_files();
			for(; sf; sf = sf->get_next())
				open_list = add_to_open_list(
					open_list, 0, edit, sf);
		}
	}
	return open_list;
}

// ok_to_quit is called if there are any live processes left.  It puts
// up a notice asking for confirmation
void
Window_set::ok_to_quit(Component *, Base_window *bw)
{
	OpenSourceRecord	*open_list;

	if ((open_list = build_open_source_list()) != 0)
	{
		if (!save_open_source_box)
			save_open_source_box = new Save_open_source_dialog(bw,
				(Callback_ptr)Window_set::ok_to_quit);
		save_open_source_box->setup(open_list);
		save_open_source_box->display();
		return;
	}
	if (proclist.any_live())
	{
		display_msg((Callback_ptr)(&Window_set::quit_cb), this,
			LAB_exit_debugger, LAB_cancel, 
			AT_urgent, GE_ok_to_exit);
	}
	else
	{
		quit_cb(0, 1);
	}
}

// quit_cb is the callback from the confirmation notice.  quit_flag gives the
// user's response - 1 for "Quit", 0 for "Cancel".  If the user presses
// "Quit", send a message to debug.  The gui will exit when it gets the
// acknowledgement back
void
Window_set::quit_cb(Component *, int quit_flag)
{
	if (!quit_flag)
		return;
	exit_called = 1;
	dispatcher.send_msg(this, 0, "quit\n");
	exit(1);
}

void
Window_set::send_interrupt_cb(Component *, void *ok_flag)
{
	if (!(int)ok_flag)
		kill(getppid(), SIGINT);
	dispatcher.set_notice_raised(FALSE);
}

// called when help button is pressed for a table of contents
// for a specific section
void
Window_set::help_toc_cb(Component *, Base_window *w)
{
	Window_shell *ws = w->get_window_shell();
	Help_id help = ws->get_help_msg();

	if(!help)
		return;
	display_help(ws->get_widget(), HM_toc, help);
}

#if OLD_HELP
// called when help button is pressed for Help Desk (Open)
void
Window_set::help_helpdesk_cb(Component *, Base_window *w)
{
	helpdesk_help(w->get_window_shell()->get_widget());
}
#endif

// the dismiss callback is called from the File menu of any base window
// there are several possibilities here:
// 1) other windows are still open in this window-set, just pop it down
// 2) only window in this window-set, and there's only one window_set,
//    call ok_to_quit() to see if there are still live processes
//    note: core processes are ok to quit here.
// 3) only window in this window-set and there are other window-sets and
//    there are active processes (live or core) still, then display 
//    "can't close" error message.
// 4) only window in this window-set, and there are other window-sets and
//    there are no active processes, then destroy everything.
void
Window_set::dismiss(Component *, Base_window *w)
{
	if (open_windows > 1)
	{
		--open_windows;
		w->popdown();
		return;
	}
	if (windows.first() == windows.last())
		ok_to_quit(0, w);
	else if (head)
		display_msg(E_ERROR, GE_cant_close);
	else	// last window in set with no active processes, destroy window set
	{
		w->popdown();
		delete this;
	}
}

void
Window_set::apply_to_obj(Control_type ct, Base_window *win)
{
	const char	*cmd;
	const char	*plist;

	// gui processes are all run in the background, unless started
	// from a script or the command line
	switch (ct)
	{
	case CT_run:			cmd = "run -b";		break;
	case CT_return:			cmd = "run -b -r";	break;
	case CT_step_statement:		cmd = "step -b";	break;
	case CT_step_instruction:	cmd = "step -b -i";	break;
	case CT_next_statement:		cmd = "step -b -o";	break;
	case CT_next_instruction:	cmd = "step -b -o -i";	break;
	case CT_halt:			cmd = "halt";		break;
	case CT_release_running:	cmd = "release";	break;
	case CT_release_suspended:	cmd = "release -s";	break;
	case CT_destroy:		cmd = "kill SIGKILL";	break;
	}

	unsigned char cmd_level = command_level;
#ifdef DEBUG_THREADS
	// release/suspended button can only be sensitive when:
	// 1) command_level == THREAD_LEVEL and all threads of a process
	//    are selected
	// 2) command_level == THREAD_LEVEL and an unthreaded process is
	//    selected
	// 3) command_level != THREAD_LEVEL
	// in the first 2 cases, we promote command level to parent process
	// temporarily for this operation
	if (ct == CT_destroy || (ct == CT_release_suspended && 
		command_level == THREAD_LEVEL))
		cmd_level = PROCESS_LEVEL;
#endif
	if (win->selection_type() == SEL_process)
	{
		Vector	*v = vec_pool.get();
		int	total = win->get_selections(v);

		plist = make_plist(total, (ProcObj **)v->ptr(), 0, cmd_level);
		vec_pool.put(v);
	}
	else
		plist = make_plist(1, &cur_obj, 0, cmd_level);

	dispatcher.send_msg(this, cur_obj->get_id(), "%s -p %s\n", cmd, plist);
}

// Callbacks for the command buttons in the Control menu
void
Window_set::run_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_run, win);
}

void
Window_set::run_r_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_return, win);
}

void
Window_set::step_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_step_statement, win);
}

void
Window_set::step_i_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_step_instruction, win);
}

void
Window_set::step_o_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_next_statement, win);
}

void
Window_set::step_oi_button_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_next_instruction, win);
}

void
Window_set::halt_button_cb(Component *, Base_window *win)
{
	if (animated)
	{
		timer->unset();
		if (animate_state == AS_step_start)
			apply_to_obj(CT_halt, win);
		animate_cleanup();
	}
	else
		apply_to_obj(CT_halt, win);
}

void
Window_set::destroy_process_cb(Component *, Base_window *win)
{
	if (animated)
	{
		timer->unset();
		clear_animation();
	}
	apply_to_obj(CT_destroy, win);
}

void
Window_set::release_running_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_release_running, win);
}

void
Window_set::release_suspended_cb(Component *, Base_window *win)
{
	apply_to_obj(CT_release_suspended, win);
}

void
Window_set::save_current_layout_cb(Component *, Base_window *)
{
	save_current_layout();
}

// callbacks to bring up dialogs.  dialog objects are created as needed
void
Window_set::create_dialog_cb(Component *, Base_window *win)
{
	if (!create_box)
		create_box = new Create_dialog(win);
	create_box->display();
}

// The GUI received MSG_oldargs.  Create dialogs in all window sets
// are updated.
void
Window_set::update_create_dialog(const char *create_args)
{
	if (create_box)
		create_box->set_create_args(create_args);
}

void
Window_set::grab_process_dialog_cb(Component *, Base_window *win)
{
	if (grab_process)
		grab_process->setup();	// get current ps list
	else
		grab_process = new Grab_process_dialog(win);
	grab_process->display();
}

void
Window_set::grab_core_dialog_cb(Component *, Base_window *win)
{
	if (!grab_core)
		grab_core = new Grab_core_dialog(win);
	grab_core->display();
}

void
Window_set::set_language_dialog_cb(Component *, Base_window *bw)
{
	if (!set_language)
		set_language = new Set_language_dialog(bw);
	set_language->set_plist(bw, PROGRAM_LEVEL);
	set_language->display();
}

// The GUI received MSG_set_language.  Set Language dialogs in all window sets
// are updated.
void
Window_set::update_language_dialog()
{
	if (set_language)
		set_language->reset(0, 0);
}


// callback for the Change Directory... button in the File menu
void
Window_set::cd_dialog_cb(Component *, Base_window *win)
{
	if (!cd_box)
		cd_box = new Cd_dialog(win);
	cd_box->display();
}

// The GUI received MSG_cd.  Change Directory dialogs in all window sets
// are updated.
void
Window_set::update_cd_dialog(const char *s)
{
	if (cd_box)
		cd_box->update_directory(s);

	Base_window	**bw = base_windows;
	for (int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw)
		{
			Source_pane	*sp;
			if ((sp = (Source_pane *)(*bw)->get_pane(PT_source)) != 0)
			{
				sp->get_pane()->change_directory(s);
			}
		}
	}
	
}

// The expression field in the Set Value dialog is initialized with selected
// text from the Source or Disassembly windows or with a symbol selected in
// the Symbol Pane
void
Window_set::set_value_dialog_cb(Component *, Base_window *win)
{
	if (!set_value)
		set_value = new Set_value_dialog(win);
	set_value->set_plist(win, command_level);

	switch (win->selection_type())
	{
	case SEL_symbol:
		{
			Vector	*v = vec_pool.get();

			// The sensitivity handling ensures that only one symbol is selected
			win->get_selections(v);
			set_value->set_expression(*(char **)v->ptr());
			vec_pool.put(v);
			break;
		}

	case SEL_src_line:
	case SEL_regs:
	case SEL_instruction:
		set_value->set_expression(truncate_selection(win->get_selection()));
		break;

	default:
		break;
	}

	set_value->display();
}

// The expression field in the Show Value dialog is initialized with selected
// text from the Source or Disassembly windows or with multiple symbols from
// the Symbol Pane
void
Window_set::show_value_dialog_cb(Component *, Base_window *win)
{
	char		*text = 0;
	Buffer		*buf = 0;

	if (!show_value)
		show_value = new Show_value_dialog(win, 0);

	switch (win->selection_type())
	{
	case SEL_symbol:
		{
			Vector	*v = vec_pool.get();
			int	total = win->get_selections(v);
			char	**syms = (char **)v->ptr();

			buf = buf_pool.get();
			buf->clear();
			for (int i = 0; i < total; i++, syms++)
			{
				if (i)
					buf->add(", ");
				buf->add(*syms);
			}
			text = (char *)*buf;
			vec_pool.put(v);
			break;
		}

	case SEL_src_line:
	case SEL_regs:
	case SEL_instruction:
		text = win->get_selection();
		break;

	default:
		break;
	}

	show_value->set_plist(win, command_level);
	if (text)
	{
		show_value->set_expression(truncate_selection(text));
		show_value->show_value(0, 0);
	}
	show_value->display();
	if (buf)
		buf_pool.put(buf);
}

Show_value_dialog *
Window_set::get_show_value_dialog(Base_window *win)
{
	if (!show_value)
		show_value = new Show_value_dialog(win, 0);
	return show_value;
}



// The expression field in the Show Type dialog is initialized with selected
// text from the Source window or with a symbol from the Symbol Pane
// The selection handler will ensure that only one symbol is selected
void
Window_set::show_type_dialog_cb(Component *, Base_window *win)
{
	if (!show_type)
		show_type = new Show_type_dialog(win);

	show_type->set_plist(win, command_level);

	Selection_type stype = win->selection_type();
	if (stype == SEL_symbol)
	{
		Vector	*v = vec_pool.get();

		win->get_selections(v);
		show_type->set_expression(*(char **)v->ptr());
		show_type->apply(0, 0);
		vec_pool.put(v);
	}
	else if (stype == SEL_src_line)
	{
		show_type->set_expression(truncate_selection(win->get_selection()));
		show_type->apply(0, 0);
	}
	else
		show_type->clear_result();

	show_type->display();
}

// The Run dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::run_dialog_cb(Component *, Base_window *win)
{
	if (!run_box)
		run_box = new Run_dialog(win);

	if (win->selection_type() == SEL_src_line)
	{
		char		buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_pane	*sw = 
			(Source_pane *)win->get_pane(PT_source);

		sprintf(buf, "%s@%d", sw->get_current_file(), sw->get_selected_line());
		run_box->set_location(buf);
	}
	else if (win->selection_type() == SEL_instruction)
	{
		Disassembly_pane *dw = 
			(Disassembly_pane *)win->get_pane(PT_disassembler);

		run_box->set_location(dw->get_selected_addr());
	}

	run_box->set_plist(win, command_level);
	run_box->display();
}

void
Window_set::step_dialog_cb(Component *, Base_window *win)
{
	if (!step_box)
		step_box = new Step_dialog(win);
	step_box->set_plist(win, command_level);
	step_box->display();
}

// The Jump dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::jump_dialog_cb(Component *, Base_window *win)
{
	if (!jump_box)
		jump_box = new Jump_dialog(win);

	if (win->selection_type() == SEL_src_line)
	{
		char		buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_pane	*sw = 
			(Source_pane *)win->get_pane(PT_source);
		sprintf(buf, "%s@%d", sw->get_current_file(),
			sw->get_selected_line());
		jump_box->set_location(buf);
	}
	else if (win->selection_type() == SEL_instruction)
	{
		Disassembly_pane *dw = 
			(Disassembly_pane *)win->get_pane(PT_disassembler);

		jump_box->set_location(dw->get_selected_addr());
	}

	jump_box->set_plist(win, command_level);
	jump_box->display();
}

// The Stop dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::stop_dialog_cb(Component *, Base_window *bw)
{
	// stop_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (stop_box)
		stop_box->setChange(0);
	else
		stop_box = new Stop_dialog(bw);

	if (bw->selection_type() == SEL_src_line)
	{
		char	buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_pane	*sw = 
			(Source_pane *)bw->get_pane(PT_source);

		sprintf(buf, "%s@%d", sw->get_current_file(), sw->get_selected_line());
		stop_box->set_expression(buf);
	}
	else if (bw->selection_type() == SEL_instruction)
	{
		Disassembly_pane *dw = 
			(Disassembly_pane *)bw->get_pane(PT_disassembler);
		stop_box->set_expression(dw->get_selected_addr());
	}

	stop_box->set_plist(bw, event_level);
	stop_box->display();
}

void
Window_set::stop_on_function_cb(Component *, Base_window *bw)
{
	if (!stop_on_function_dialog)
		stop_on_function_dialog = new Stop_on_function_dialog(bw);
	stop_on_function_dialog->set_plist(bw, event_level);
	stop_on_function_dialog->display();
}

void
Window_set::signal_dialog_cb(Component *, Base_window *bw)
{
	// signal_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (signal_box)
		signal_box->setChange( 0 );
	else
		signal_box = new Signal_dialog(bw);

	signal_box->set_plist(bw, event_level);
	signal_box->display();
}

void
Window_set::syscall_dialog_cb(Component *, Base_window *bw)
{
	// syscall_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (syscall_box)
		syscall_box->setChange( 0 );
	else
		syscall_box = new Syscall_dialog(bw);

	syscall_box->set_plist(bw, event_level);
	syscall_box->display();
}

void
Window_set::onstop_dialog_cb(Component *, Base_window *bw)
{
	// onstop_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (onstop_box)
		onstop_box->setChange( 0 );
	else
		onstop_box = new Onstop_dialog(bw);

	onstop_box->set_plist(bw, event_level);
	onstop_box->display();
}

#if EXCEPTION_HANDLING
void
Window_set::exception_dialog_cb(Component *, Base_window *bw)
{
	// exception_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (exception_box)
		exception_box->setChange( 0 );
	else
		exception_box = new Exception_dialog(bw);

	exception_box->set_plist(bw, event_level);
	exception_box->display();
}

void
Window_set::ignore_exceptions_dialog_cb(Component *, Base_window *bw)
{
	if (!ignore_exceptions_box)
		ignore_exceptions_box = new Ignore_exceptions_dialog(bw);
	ignore_exceptions_box->set_plist(bw, command_level);
	ignore_exceptions_box->display();
}
#endif

void
Window_set::cancel_dialog_cb(Component *, Base_window *bw)
{
	if (!cancel_box)
		cancel_box = new Cancel_dialog(bw);
	cancel_box->set_plist(bw, command_level);
	cancel_box->display();
}

void
Window_set::kill_dialog_cb(Component *, Base_window *bw)
{
	if (!kill_box)
		kill_box = new Kill_dialog(bw);
	kill_box->set_plist(bw, command_level);
	kill_box->display();
}

void
Window_set::setup_signals_dialog_cb(Component *, Base_window *bw)
{
	if (!setup_signals)
		setup_signals = new Setup_signals_dialog(bw);
	setup_signals->display();
	setup_signals->set_plist(bw, command_level);
}

void
Window_set::path_dialog_cb(Component *, Base_window *bw)
{
	if (!path_box)
		path_box = new Path_dialog(bw);
	path_box->set_plist(bw, PROGRAM_LEVEL);
	path_box->display();
}

void
Window_set::set_granularity_cb(Component *, Base_window *bw)
{
	if (!granularity_box)
		granularity_box = new Granularity_dialog(bw);
	granularity_box->display();
}

// callback for the Output Action... button
void
Window_set::action_dialog_cb(Component *, Base_window *bw)
{
	if (!action_box)
		action_box = new Action_dialog(bw);
	action_box->display();
}

void
Window_set::version_cb(Component *, Base_window *)
{
	dispatcher.send_msg(this, 0, "version\n");
}

void
Window_set::new_window_set_cb(Component *, Base_window *bw)
{
	Window_set	*ws = new Window_set();

	if (bw->selection_type() != SEL_process)
		return;

	Vector	*v = vec_pool.get();
	int	total = bw->get_selections(v);
	proclist.move_objs((ProcObj **)v->ptr(), total, ws);
	vec_pool.put(v);
}

void
Window_set::set_current_cb(Component *, Base_window *bw)
{
	if (bw->selection_type() == SEL_frame)
	{
		Stack_pane	*stack_pane;
		stack_pane = (Stack_pane *)bw->get_pane(PT_stack);
		stack_pane->set_current();
	}
	else if (bw->selection_type() == SEL_process)
	{
		Vector	*v = vec_pool.get();
		(void) bw->get_selections(v);
		ProcObj	*p = *(ProcObj **)v->ptr();
		vec_pool.put(v);

		set_current(p);
	}
}

// The dump dialog may be initialized with the location of the
// selected symbol in the symbols pane
void
Window_set::dump_dialog_cb(Component *, Base_window *bw)
{
	int	set_loc = 0;
	if (!dump_box)
		dump_box = new Dump_dialog(bw);

	switch (bw->selection_type())
	{
	case SEL_symbol:
	{
		Vector	*v = vec_pool.get();

		bw->get_selections(v);
		dump_box->set_location(*(char **)v->ptr());
		vec_pool.put(v);
		set_loc = 1;
		break;
	}
	case SEL_src_line:
	case SEL_regs:
	case SEL_instruction:
		dump_box->set_location(bw->get_selection());
		set_loc = 1;
		break;

	default:
		break;
	}
	dump_box->set_plist(bw, command_level);
	dump_box->clear();
	if (set_loc)
		dump_box->do_dump(0,0);
	dump_box->display();
}

void
Window_set::map_dialog_cb(Component *, Base_window *bw)
{
	if (!map_box)
		map_box = new Map_dialog(bw);

	map_box->set_plist(bw, command_level);
	map_box->do_map();
	map_box->display();
}

// callback for the Animation... popup window (under Properties)
void
Window_set::animation_dialog_cb(Component *, Base_window *bw)
{
	if (!animation_box)
		animation_box = new Animation_dialog(bw);
	animation_box->display();
}

// callback for the Animate command button (under Control)
void
Window_set::animate_src_cb(Component *, Base_window *win)
{
	Source_pane	*source_pane;
	if ((source_pane = (Source_pane *)win->get_pane(PT_source)) == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	animated_pane = source_pane;
	source_pane->animate(TRUE);
	dispatcher.send_msg(this, cur_obj->get_id(), "step -b\n");
	common_animate(win, GM_source_animation);
}

void
Window_set::animate_dis_cb(Component *, Base_window *win)
{
	Disassembly_pane	*dis_pane;
	if ((dis_pane = (Disassembly_pane *)win->get_pane(PT_disassembler)) == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	animated_pane = dis_pane;
	dis_pane->animate(TRUE);
	dispatcher.send_msg(this, cur_obj->get_id(), "step -i -b\n");
	common_animate(win, GM_dis_animation);
}

void
Window_set::common_animate(Base_window *win, Gui_msg_id msg_id)
{
	Window_shell	*wsh = win->get_window_shell();

	animate_state = AS_step_start;

	if (win->is_open())
		wsh->raise();
	wsh->clear_msg();
	wsh->display_msg(E_NONE, msg_id);

	int index;

	animated = TRUE;
	cur_obj->set_animation();
	find_obj(cur_obj, index);
	update_ps_panes(RC_animate, index, cur_obj);
	change_current_notify(RC_animate, cur_obj);
	change_any.notify(RC_animate, cur_obj);
}

// called when user hits halt to stop animation
void
Window_set::animate_cleanup()
{
	if (!animated_pane)
		return;

	Window_shell	*wsh;
	wsh = animated_pane->get_parent()->get_window_shell();
	clear_animation();

	// delay the clearing of pobj animation if a halt
	// command has been issued while a step is in progress
	// until the pobj stops and update_obj is
	// called. this ensures the correct state of the pobj
	// so that the dispatcher won't get confused with a
	// "step -c n" command
	if (animate_state == AS_step_done)
	{
		cur_obj->clear_animation();
		update_obj(cur_obj);
	}
	wsh->clear_msg();
	wsh->display_msg(E_NONE, GM_animation_end);
}

// called by debugger to stop animation when process stops or dies
void
Window_set::clear_animation()
{
	animated = FALSE;
	if (!animated_pane)
		return;
	if (animated_pane->get_type() == PT_source)
	{
		((Source_pane *)animated_pane)->animate(FALSE);
	}
	else if (animated_pane->get_type() == PT_disassembler)
	{
		((Disassembly_pane *)animated_pane)->animate(FALSE);
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	animated_pane = 0;
}

void
Window_set::set_dis_mode_cb(Component *, Base_window *bw)
{
	if (!dis_mode_box)
		dis_mode_box = new Dis_mode_dialog(bw);
	dis_mode_box->display();
}

void
Window_set::update_dis_mode_dialog()
{
	if (dis_mode_box)
		dis_mode_box->reset(0, 0);

	Base_window	**win = base_windows;
	for(int i = 0; i < windows_per_set; i++, win++)
	{
		Disassembly_pane	*dis_pane;
		if (*win && ((dis_pane = (Disassembly_pane *)(*win)->get_pane(PT_disassembler)) != 0))
			dis_pane->redisplay();
	}
}

void
Window_set::frame_dir_dlg_cb(Component *, Base_window *bw)
{
	if (!frame_dir_box)
		frame_dir_box = new Frame_dir_dialog(bw);
	frame_dir_box->display();
}

void
Window_set::update_frame_dir_dialog()
{
	if (frame_dir_box)
		frame_dir_box->reset(0, 0);

	Base_window	**win = base_windows;
	for(int i = 0; i < windows_per_set; i++, win++)
	{
		Stack_pane	*stack_pane;
		if (*win && ((stack_pane = (Stack_pane *)(*win)->get_pane(PT_stack)) != 0))
			stack_pane->redisplay();
	}
}

void
Window_set::clear_msgs()
{
	for (int i = 0; i <windows_per_set; i++)
	{
		Base_window	*win = base_windows[i];
		if (win && win->is_open())
			win->get_window_shell()->clear_msg();
	}

	Base_window	*w =  (Base_window *)source_windows.first();
	for ( ; w; w = (Base_window *)source_windows.next())
		w->get_window_shell()->clear_msg();
}

// handle error messages that are result of commands issued
// in the window set itself, e.g. run, step, etc.
void
Window_set::de_message(Message *m)
{
	if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
	{
		display_msg(m);
		return;
	}
}

void
Window_set::set_in_script(Boolean ins)
{
	for (int i = 0; i < windows_per_set; i++)
	{
		Base_window	*win = base_windows[i];
		if (win && win->is_open())
		{
			win->get_window_shell()->clear_msg();
		}
	}
	change_state.notify(ins ? RC_start_script : RC_end_script, cur_obj);
}

static void
add_window_set_entry(Base_window *win, char *tile, Button *btn)
{
	Menu *mp = win->get_menu_bar()->find_item(B_window_sets);
	if (!mp)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	mp->add_item(btn);
	win->set_sensitivity();
}

void
Window_set::add_ws(int ws_id)
{
	Button		*btn;
	Button_core	*bcore;
	mnemonic_t	mne;
	Base_window	*win;

	const char	*lab = labeltab.get_label(LAB_window_set);
	char		*title = new char[strlen(lab) + MAX_INT_DIGITS + 2];

	sprintf(title, "%s %d", lab, ws_id);
	if (ws_id < 10)
	{
		mne = '0' + ws_id;
	}
	else
		mne = 0;
	bcore = find_button(B_set_popup);

	// add an item to Windows menu
	for (int i = 0; i < windows_per_set; i++)
	{
		win = base_windows[i];

		if (!win)
			continue;

		btn = new Button(title, mne, bcore);
		btn->set_udata((void *)ws_id);
		add_window_set_entry(win, title, btn);

	}
	win = (Base_window *)source_windows.first();
	for(; win; win = (Base_window *)source_windows.next())
	{
		btn = new Button(title, mne, bcore);
		btn->set_udata((void *)ws_id);
		add_window_set_entry(win, title, btn);
	}
}

void
Window_set::popup_ws(Component *, void *data)
{
	int ws_id = (int)data;

	// popup window set with id 'ws_id'
	for (Window_set *ws = (Window_set *)windows.first(); 
		ws != NULL; ws = (Window_set *)windows.next())
	{
		if (ws->get_id() == ws_id)
		{
			Window_descriptor	*wdesc = window_descriptor;
			for (int i = 0; wdesc; i++, wdesc = wdesc->next())
			{
				if (wdesc->get_flags() & W_AUTO_POPUP)
					ws->popup_window(i);
			}
			return;
		}
	}
}

static void
delete_window_set_entry(Base_window *win, char *title)
{
	Menu *mp = win->get_menu_bar()->find_item(B_window_sets);
	if (!mp)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	mp->delete_item(title);
	win->set_sensitivity();
}

void
Window_set::delete_ws(int ws_id)
{
	const char	*lab = labeltab.get_label(LAB_window_set);
	char		*title = new char[strlen(lab) + MAX_INT_DIGITS + 2];
	Base_window	*win;

	sprintf(title, "%s %d", lab, ws_id);

	// remove an item from Windows menu
	for (int i = 0; i < windows_per_set; i++)
	{
		win = base_windows[i];

		if (!win)
			continue;
		delete_window_set_entry(win, title);
	}
	win = (Base_window *)source_windows.first();
	for(; win; win = (Base_window *)source_windows.next())
	{
		delete_window_set_entry(win, title);
	}
	delete title;
}

void
Window_set::move_to_ws_cb(Component *, Base_window *bw)
{
	if (!move_dlg)
		move_dlg = new Move_dialog(bw);
	else
		move_dlg->setup();
	move_dlg->set_plist(bw, command_level);
	move_dlg->display();
}

// initalize sensitivity counters for all Base windows
void
Window_set::init_sense_counts()
{
	Base_window	**bw = base_windows;
	for (int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw)
			(*bw)->set_sense_count(1);
	}
}

// call set_sensitivity for any base window whose sense_count
// is greater than 1 - set all counts to 0
void
Window_set::set_sensitivity()
{
	Base_window	**bw = base_windows;
	for (int i = 0; i < windows_per_set; i++, bw++)
	{
		if (!*bw)
			continue;
		if ((*bw)->get_sense_count() > 1)
		{
			(*bw)->set_sense_count(0);
			(*bw)->set_sensitivity();
		}
		else
			(*bw)->set_sense_count(0);
	}
}

void
Window_set::update_ps_panes(Reason_code rc, int slot, ProcObj *obj)
{
	Ps_pane	*ps_pane;
	ps_pane = (Ps_pane *)ps_panes.first();
	for(; ps_pane; ps_pane = (Ps_pane *)ps_panes.next())
		ps_pane->update_obj(rc, slot, obj);
}

void
Window_set::command_add_text(char *text)
{
	Command_pane	*cp;
	int		count = 0;

	cp = (Command_pane *)command_panes.first();
	for (; cp; cp = (Command_pane *)command_panes.next())
	{
		cp->get_transcript()->add_text(text);
		count++;
	}
	if (count < cmd_panes_per_set)
	{
		if (!transcript)
			transcript = new Transcript;
		transcript->add(text);
	}
	else if (transcript)
		transcript->clear();
}

void
Window_set::add_src_menus(const char *file, Base_window *b)
{
	Base_window	**bw = base_windows;
	for(int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw)
			(*bw)->add_src_menu(file, b);
		
	}
	Base_window	*sw;
	sw = (Base_window *)source_windows.first();
	for(; sw; sw = (Base_window *)source_windows.next())
	{
		if (sw != b)
			sw->add_src_menu(file, b);
	}
}

void
Window_set::update_src_menus(const char *file, Base_window *b)
{
	Base_window	**bw = base_windows;
	for(int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw)
			(*bw)->update_src_menu(file, b);
		
	}
	Base_window	*sw;
	sw = (Base_window *)source_windows.first();
	for(; sw; sw = (Base_window *)source_windows.next())
	{
		if (sw != b)
			sw->update_src_menu(file, b);
	}
}

void
Window_set::delete_src_menus(const char *file, Base_window *b)
{
	Base_window	**bw = base_windows;
	for(int i = 0; i < windows_per_set; i++, bw++)
	{
		if (*bw)
			(*bw)->delete_src_menu(file, b);
		
	}
	Base_window	*sw;
	sw = (Base_window *)source_windows.first();
	for(; sw; sw = (Base_window *)source_windows.next())
	{
		if (sw != b)
			sw->delete_src_menu(file, b);
	}
}

// add or delete button panel
void
Window_set::update_button_bar(Bar_info *bi)
{
	Window_descriptor	*wdesc = bi->window_desc;

	if (bi->old_bdesc)
	{
		if (bi->remove)
			wdesc->remove_button_bar(bi);
		else
			wdesc->replace_button_bar(bi);
	}
	else
	{
		wdesc->add_button_bar(bi->new_bdesc);
	}
	wdesc->set_flag(W_DESC_CHANGED);
	if (bi->bottom)
		bi->new_wbar = wdesc->get_bottom_button_bars();
	else
		bi->new_wbar = wdesc->get_top_button_bars();

	int		index = -1;
	Window_set	*ws = (Window_set *)windows.first();
	Base_window	*bw;
	if (!(wdesc->get_flags() & PT_second_src))
	{
		index = 0;
		Window_descriptor	*wd = window_descriptor;
		for(; wd; index++, wd = wd->next())
		{
			if (wd == wdesc)
				break;
		}
	}
	for ( ; ws; ws = (Window_set *)windows.next())
	{
		if (index == -1)
		{
			bw =  (Base_window *)ws->source_windows.first();
			for ( ; bw; bw = (Base_window *)source_windows.next())
				bw->update_button_bar(bi);
		}
		else
		{
			bw = ws->base_windows[index];
			if (bw)
				bw->update_button_bar(bi);
				
		}
	}
	Button	*btn = bi->old_bdesc->get_buttons();
	while(btn)
	{
		Button	*buttons;
		buttons = btn->next();
		delete btn;
		btn = buttons;
	}
	delete bi->old_bdesc;
}

void
Window_set::update_source_wins()
{
	Source_pane	*source;
	Base_window	**win = base_windows;

	for (int i = 0; i < windows_per_set; i++, win++)
	{
		if (*win && ((source = (Source_pane *)(*win)->get_pane(PT_source)) != 0))
			source->update_cb(0, RC_change_state, 0, cur_obj);
	}
}

Set_value_dialog::Set_value_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_set_value,  LAB_set_value_mne, 
			(Callback_ptr)(&Set_value_dialog::apply) },
		{ B_apply, LAB_none,  LAB_none, 
			(Callback_ptr)(&Set_value_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Set_value_dialog::cancel) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Packed_box	*box;
	Caption		*caption;

	static const Toggle_data set_toggles[] = {
		{ LAB_export, FALSE, 0 },
	};

	save_expr = 0;
	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_set_value, (Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_set_value_dialog);

	box = new Packed_box(dialog, "set value box", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_expression_line, CAP_LEFT);
	expression = new Text_line(caption, "expression", "", 40, 1);
	caption->add_component(expression);
	box->add_component(caption);

	export = new Toggle_button(box, "toggle", set_toggles,
		sizeof(set_toggles)/sizeof(Toggle_data), OR_horizontal);
	box->add_component(export);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(expression);
}

void
Set_value_dialog::set_expression(const char *s)
{
	expression->set_text(s);
}

// Unlike most other Process_dialogs, the Set Value dialog does not
// require a current process or process selection (so the user may set
// debugger or user variables in an empty window set).
void
Set_value_dialog::apply(Component *, void *)
{
	char	*expr = expression->get_text();
	if (!expr || !*expr)
	{
		dialog->error(E_ERROR, GE_no_expression);
		return;
	}

	if (export->is_set(0))
	{
		if (*expr != '$')
		{
			dialog->error(E_ERROR, GE_user_sym_required);
			return;
		}
		export_state = TRUE;
	}
	else
		export_state = FALSE;

	delete save_expr;
	save_expr = makestr(expr);

	if (pobjs)
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"set -p %s %s\n", make_plist(total, pobjs, 0, level), expr);
	else
		dispatcher.send_msg(this, 0, "set %s\n", expr);
	dialog->wait_for_response();
}

void
Set_value_dialog::cancel(Component *, void *)
{
	expression->set_text(save_expr);
	export->set(0, export_state);
}

void
Set_value_dialog::cmd_complete()
{
	if (export_state == TRUE)
	{
		char	*p;
		char	saved;
		if (((p = strchr(save_expr, ' ')) != 0) ||
			((p = strchr(save_expr, '=')) != 0))
		{
			saved = *p;
			*p = 0;
		}
		dispatcher.send_msg(0, 0, "export %s\n", save_expr);
		if (p)
			*p = saved;
			
	}

	Window_set	*ws = get_window_set();
	Base_window	**win = ws->get_base_windows();

	for(int i = 0; i < windows_per_set; i++, win++)
	{
		Register_pane	*regs_pane;
		Symbols_pane	*syms_pane;

		if (!*win)
			continue;
		regs_pane = (Register_pane *)(*win)->get_pane(PT_registers);
		syms_pane = (Symbols_pane *)(*win)->get_pane(PT_symbols);

		if (syms_pane && syms_pane->get_parent()->is_open())
			syms_pane->update_cb(0, RC_set_current, 0, ws->current_obj());
		if (regs_pane && regs_pane->get_parent()->is_open())
			regs_pane->update_cb(0, RC_set_current, 0, ws->current_obj());
	}
	dialog->cmd_complete();
}

void
Set_value_dialog::set_obj(Boolean reset)
{
	// make sure this one is always sensitive
	if (reset)
		dialog->set_sensitive(TRUE);
}

Show_type_dialog::Show_type_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Show_type_dialog::apply) },
		{ B_close, LAB_none, LAB_none, 0 },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_show_type, (Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_show_type_dialog);
	box = new Expansion_box(dialog, "show type", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_expression_line, CAP_LEFT);
	expression = new Text_line(caption, "expression", "", 25, 1);
	caption->add_component(expression);
	box->add_component(caption);
	expr = 0;

	caption = new Caption(box, LAB_type_line, CAP_TOP_LEFT);
	result = new Text_area(caption, "type", 8, 30);
	caption->add_component(result);
	box->add_component(caption, TRUE);

	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(expression);
}

void
Show_type_dialog::clear_result()
{
	result->clear();
}

void
Show_type_dialog::set_expression(const char *s)
{
	expr = makestr(s);
	expression->set_text(expr);
}

// Unlike most other Process_dialogs, the Show Type dialog does not
// require a current process or process selection
void
Show_type_dialog::apply(Component *, void *)
{
	Buffer		*buffer;

	delete expr;
	buffer = buf_pool.get();
	expr = makestr(protect(expression->get_text(), buffer));
	buf_pool.put(buffer);

	dialog->clear_msg();
	if (!expr || !*expr)
	{
		dialog->error(E_ERROR, GE_no_expression);
		return;
	}

	result->clear();
	if (pobjs)
	{
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"whatis -p %s %s\n", make_plist(total, pobjs, 0, level),
				expr);
	}
	else
	{
		dispatcher.send_msg(this, 0, "whatis %s\n", expr);
	}
	dialog->wait_for_response();
}

void
Show_type_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
		show_error(m, TRUE);
	else if (!gui_only_message(m))
		result->add_text(m->format());
}

// scroll back to the top of the text area, in case the result is
// bigger than the available space
void
Show_type_dialog::cmd_complete()
{
	result->position(1);
	dialog->cmd_complete();
}

void
Show_type_dialog::set_obj(Boolean reset)
{
	// make sure this dialog is ALWAYS sensitive!
	if (reset)
		dialog->set_sensitive(TRUE);
}

// Whenever the current object stops, update the result pane
// (since type may be context sensitive - especially type of %eh_object)
void
Show_type_dialog::update_obj(ProcObj *p, Reason_code rc)
{
	if (p->is_running() || p->is_animated() || rc == RC_fcall_in_expr)
		return;
	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}
	if (!expr || !*expr)
	{
		return;
	}

	result->clear();
	dispatcher.send_msg(this, pobjs[0]->get_id(),
		"whatis -p %s %s\n",
		make_plist(total, pobjs, 0, level),
		expr);
	dialog->wait_for_response();
}

// values correspond to radio button number
#define GLOBAL_PATH	0
#define PROGRAM_PATH	1

Path_dialog::Path_dialog(Base_window *bw) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Path_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Path_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Path_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Path_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	// this array must be kept in sync with *_PATH values
	static const LabelId radio_buttons[] =
	{
		LAB_global_path,
		LAB_program_path,
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_source_path, (Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_path_dialog);
	box = new Expansion_box(dialog, "path_dialog", OR_vertical);
	component_init(box);

	state = GLOBAL_PATH;
	caption = new Caption(box, LAB_set_line, CAP_LEFT);
	choice = new Radio_list(caption, "path type", OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(LabelId), 0,
		(Callback_ptr)(&Path_dialog::set_path_type), this);
	caption->add_component(choice, FALSE);
	box->add_component(caption);

	path_area = new Text_area(box, "path", 10, 20, TRUE);
	box->add_component(path_area, TRUE);
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(path_area);
}

void
Path_dialog::apply(Component *, void *)
{
	state = choice->which_button();
	if ((state == PROGRAM_PATH) && !pobjs)
	{
		dialog->error(E_ERROR, GE_no_process);
		return;
	}

	char	*buf = path_area->get_text();

	if (!buf)
		buf = "";

	for (char *ptr = buf; *ptr; ptr++)
	{
		if (isspace(*ptr))
			*ptr = ':';
	}

	DBcontext	pobj_id;
	char		*path;
	if (state == GLOBAL_PATH)
	{
		pobj_id = 0;
		path = "global_path";
	}
	else
	{
		pobj_id = pobjs[0]->get_id();
		path = "path";
	}
	dispatcher.send_msg(this, pobj_id, "set %%%s = \"%s\"\n", path, buf);
	dialog->wait_for_response();
}

void
Path_dialog::cmd_complete()
{
	// reinitialize source panes
	if (state == PROGRAM_PATH)
		window_set->update_source_wins();
	else
	{
		Window_set *ws;
		for (ws = (Window_set *)windows.first();
			ws; ws = (Window_set *)windows.next())
		{
			ws->update_source_wins();
		}
	}
	dialog->cmd_complete();
}

void
Path_dialog::set_path_type(Radio_list *radio, int new_state)
{
	Message	*msg;
	char	*buf = 0;

	if (new_state == PROGRAM_PATH && !pobjs)
	{
		choice->set_button(GLOBAL_PATH);
		dialog->error(E_ERROR, GE_no_process);
		return;
	}

	if (!radio) // was called from reset(), rather than radio button callback
		choice->set_button(new_state);

	DBcontext	pobj_id;
	char		*path;
	if (new_state == GLOBAL_PATH)
	{
		pobj_id = 0;
		path = "global_path";
		// force sensitivity for global path setting
		dialog->set_sensitive(TRUE);
	}
	else
	{
		pobj_id = pobjs[0]->get_id();
		path = "path";
	}
	dispatcher.query(this, pobj_id, "print %%%s\n", path);
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
			msg->unbundle(buf);
		else
			display_msg(msg);
	}

	path_area->clear();
	if (!buf || *buf == '0')
		return;

	if (*buf == '"')
		buf++;
	for (char *ptr = buf; *ptr; ptr++)
	{
		if (*ptr == ':')
			*ptr = '\n';
		else if (*ptr == '"')
			*ptr = '\0';
	}
	path_area->add_text(buf);
}

void
Path_dialog::reset(Component *, void *)
{
	set_path_type(0, state);
}

void
Path_dialog::set_obj(Boolean)
{
	if (pobjs)
	{
		state = choice->which_button();
	}
	else
	{
		state = GLOBAL_PATH;
	}
	set_path_type(0, state);
}

// values correspond to entries in radio button list
#ifdef DEBUG_THREADS
#define	THREAD_GRANULARITY	0
#define	PROCESS_GRANULARITY	1
#define PROGRAM_GRANULARITY	2
#else
#define	PROCESS_GRANULARITY	0
#define PROGRAM_GRANULARITY	1
#endif

static int
level_to_state(int level)
{
	switch(level)
	{
#ifdef DEBUG_THREADS
	case THREAD_LEVEL:	return THREAD_GRANULARITY;
#endif
	case PROCESS_LEVEL:	return PROCESS_GRANULARITY;
	case PROGRAM_LEVEL:	return PROGRAM_GRANULARITY;
	}
	return 0;
}

Granularity_dialog::Granularity_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Granularity_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Granularity_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Granularity_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Granularity_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	// this array must be kept in sync with *_GRANULARITY values
	static const LabelId radio_buttons[] =
	{
#ifdef DEBUG_THREADS
		LAB_thread_only,
		LAB_parent_proc,
#else
		LAB_proc_only,
#endif
		LAB_parent_program,
	};

	Packed_box	*box;
	Caption		*caption;

	event_state = level_to_state(window_set->get_event_level());
	command_state = level_to_state(window_set->get_command_level());

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_granularity, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_granularity_dialog);
	box = new Packed_box(dialog, "granularity box", OR_vertical);

	caption = new Caption(box, LAB_events_apply_line, CAP_LEFT);
	event_choice = new Radio_list(caption, "event buttons", OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(LabelId), event_state, 0, this);
	caption->add_component(event_choice);
	box->add_component(caption);

	caption = new Caption(box, LAB_other_apply_line, CAP_LEFT);
	command_choice = new Radio_list(caption, "command buttons", OR_vertical,
		radio_buttons, sizeof(radio_buttons)/sizeof(LabelId), command_state, 0, this);
	caption->add_component(command_choice);
	box->add_component(caption);

	box->update_complete();
	dialog->add_component(box);
}

static unsigned char
state_to_level(int state)
{
	switch(state)
	{
#ifdef DEBUG_THREADS
	case THREAD_GRANULARITY:	return THREAD_LEVEL;
#endif
	case PROCESS_GRANULARITY:	return PROCESS_LEVEL;
	case PROGRAM_GRANULARITY:	return PROGRAM_LEVEL;
	}
	return 0;
}

void
Granularity_dialog::apply(Component *, void *)
{
	event_state = event_choice->which_button();
	command_state = command_choice->which_button();

	window_set->set_event_level(state_to_level(event_state));
#ifdef DEBUG_THREADS
	unsigned char new_command_level = state_to_level(command_state);
	if (window_set->get_command_level() != new_command_level)
	{
		// command level changed, set it and reset sensitivity
		// of any window containing a process pane,
		// since the File/Release/Suspended
		// button depends on it
		window_set->set_command_level(new_command_level);
		Base_window	**win = window_set->get_base_windows();
		for(int i = 0; i < windows_per_set; i++, win++)
		{
			if (*win && (*win)->is_open() && 
				(*win)->get_pane(PT_process))
				(*win)->set_sensitivity();
		}
	}
#else
	window_set->set_command_level(state_to_level(command_state));
#endif
}

void
Granularity_dialog::reset(Component *, void *)
{
	event_choice->set_button(event_state);
	command_choice->set_button(command_state);
}

#ifdef DEBUG_THREADS
#define THREAD_BEEP_TOGGLE	0
#define	THREAD_STOP_TOGGLE	1
#endif

Action_dialog::Action_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Action_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Action_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Action_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Action_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const LabelId choices[] =
	{
		LAB_open_raise,
		LAB_beep,
		LAB_alert_box,
		LAB_no_action,
	};

#ifdef DEBUG_THREADS
	static Toggle_data thread_toggles[] =
	{
		{ LAB_beep, TRUE, 0 },
		{ LAB_stop_action, TRUE, 0 },
	};
#endif

	Packed_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(window_set->get_window_shell(),
		LAB_output_action, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_action_dialog);
	box = new Packed_box(dialog, "box", OR_vertical);

	caption = new Caption(box, LAB_event_action_line, CAP_LEFT);
	event_action = new Radio_list(caption, "event action", OR_vertical,
		choices, sizeof(choices)/sizeof(LabelId),
		(int)window_set->get_event_action());
	caption->add_component(event_action);
	box->add_component(caption);

	caption = new Caption(box, LAB_output_action_line, CAP_LEFT);
	output_action = new Radio_list(caption, "output action", OR_vertical,
		choices, sizeof(choices)/sizeof(LabelId),
		(int)window_set->get_output_action());
	caption->add_component(output_action);
	box->add_component(caption);

#ifdef DEBUG_THREADS
	unsigned char state = window_set->get_thread_action();
	caption = new Caption(box, LAB_thread_action_line, CAP_LEFT);
	// initialized to TRUE
	if (!(state & TCA_beep))
		thread_toggles[THREAD_BEEP_TOGGLE].state = FALSE;
	if (!(state & TCA_stop))
		thread_toggles[THREAD_STOP_TOGGLE].state = FALSE;
	thread_action = new Toggle_button(caption, "thread action", 
		thread_toggles, sizeof(thread_toggles)/sizeof(Toggle_data),
		OR_vertical, this);
	caption->add_component(thread_action);
	box->add_component(caption);
#endif
	box->update_complete();
	dialog->add_component(box);
}

void
Action_dialog::apply(Component *, void *)
{
	window_set->set_event_action((Action)event_action->which_button());
	window_set->set_output_action((Action)output_action->which_button());
#ifdef DEBUG_THREADS
	unsigned char state = 0;
	if (thread_action->is_set(THREAD_BEEP_TOGGLE))
		state |= TCA_beep;
	if (thread_action->is_set(THREAD_STOP_TOGGLE))
		state |= TCA_stop;
	window_set->set_thread_action(state);
#endif
}

void
Action_dialog::reset(Component *, void *)
{
	event_action->set_button(window_set->get_event_action());
	output_action->set_button(window_set->get_output_action());
#ifdef DEBUG_THREADS
	unsigned char state = window_set->get_thread_action();
	thread_action->set(THREAD_BEEP_TOGGLE, (state&TCA_beep) != 0);
	thread_action->set(THREAD_STOP_TOGGLE, (state&TCA_stop) != 0);
#endif
}

Animation_dialog::Animation_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Animation_dialog::apply) },
		{ B_apply, LAB_none, LAB_none,
			(Callback_ptr)(&Animation_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Animation_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Animation_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	dialog = new Dialog_shell(window_set->get_window_shell(),
		LAB_animation, 0, this, buttons, sizeof(buttons)/sizeof(DButton),
		HELP_animation_dialog);

#ifdef TOOLKIT_BUG_FIX
	// the following code can cause bad visuals on certain
	// displays (probably high enough resolutions e.g. 800x1024)
	// that result in chopped slider "current value" label or
	// even the slider itself.
	Packed_box	*box = new Packed_box(dialog, "granularity box", OR_vertical);
	Caption		*caption = new Caption(box, LAB_delay,
				CAP_TOP_CENTER);

	slider = new Slider(caption, "slider", OR_horizontal, 0, 10, window_set->get_interval(), 1);
	caption->add_component(slider);
	box->add_component(caption);

	// an alternative which also causes bad visuals that result
	// in chopped dialog buttons:
	//
	// Expansion_box	*box = new Expansion_box(dialog, "granularity box", OR_vertical);
	// Simple_text	*caption = new Simple_text(box, labeltab.get_label(LAB_delay), TRUE);
	// slider = new Slider(box, "slider", OR_horizontal, 0, 10, window_set->get_interval(), 1);
	// box->add_component(caption);
	// box->add_component(slider, TRUE);
#else
	Divided_box	*box = new Divided_box(dialog, "granularity box", FALSE);
	Simple_text	*caption = new Simple_text(box, labeltab.get_label(LAB_delay), TRUE);
	slider = new Slider(box, "slider", OR_horizontal, 0, 10, window_set->get_interval(), 1);
	box->add_component(caption);
	box->add_component(slider);
#endif
	box->update_complete();
	dialog->add_component(box);
}

void
Animation_dialog::apply(Component *, void *)
{
	window_set->set_interval(slider->get_value());
}

void
Animation_dialog::reset(Component *, void *)
{
	slider->set_value(window_set->get_interval());
}

// disassembly mode is a global property, so the Dis_mode
// dialog in each window
// set must be updated whenever the direction changes

int	dis_mode;

void
set_dis_mode(Message *m)
{
	Window_set	*ws;
	char		*l;

	m->unbundle(l);
	if (strcmp(l, "nosource") == 0)
		dis_mode = DISMODE_DIS_ONLY;
	else
		dis_mode = DISMODE_DIS_SOURCE;

	for (ws = (Window_set *)windows.first(); ws;
		ws = (Window_set *)windows.next())
		ws->update_dis_mode_dialog();
}

Dis_mode_dialog::Dis_mode_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Dis_mode_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Dis_mode_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Dis_mode_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	// this array must be kept in sync with *_DIS_TYPE values
	static const LabelId radio_buttons[] =
	{
		LAB_dis_only,
		LAB_dis_and_source,
	};

	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_dis_mode, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_dis_mode_dialog);

	caption = new Caption(dialog, LAB_dis_mode_line, CAP_LEFT);
	dis_choice = new Radio_list(caption, "dis_mode buttons",
		OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(LabelId), dis_mode, 0,
		this);
	caption->add_component(dis_choice);
	dialog->add_component(caption);
}

void
Dis_mode_dialog::apply(Component *, void *)
{
	int		dis = dis_choice->which_button();

	(void)set_debug_dis_mode(this, dis);

	dialog->wait_for_response();
}

void
Dis_mode_dialog::reset(Component *, void *)
{
	dis_choice->set_button(dis_mode);
}

static int frame_number_dir;

#define FRAME_DIR_UP	0
#define FRAME_DIR_DOWN	1


// frame direction is a global property, so the Set_frame_dir
// dialog in each window
// set must be updated whenever the direction changes
void
set_frame_dir(Message *m)
{
	Window_set	*ws;
	char		*l;

	m->unbundle(l);
	if (strcmp(l, "down") == 0)
		frame_number_dir = FRAME_DIR_DOWN;
	else
		frame_number_dir = FRAME_DIR_UP;

	for (ws = (Window_set *)windows.first(); ws;
		ws = (Window_set *)windows.next())
		ws->update_frame_dir_dialog();
}

Frame_dir_dialog::Frame_dir_dialog(Base_window *bw) : DIALOG_BOX(bw)
{
	static const DButton	buttons[] =
	{
		{ B_ok, LAB_none, LAB_none,
			(Callback_ptr)(&Frame_dir_dialog::apply) },
		{ B_reset, LAB_none, LAB_none,
			(Callback_ptr)(&Frame_dir_dialog::reset) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Frame_dir_dialog::reset) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	// this array must be kept in sync with FRAM_DIR* values
	static const LabelId radio_buttons[] =
	{
		LAB_frame_dir_up,
		LAB_frame_dir_down,
	};

	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_frame_dir, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton), HELP_frame_dir_dlg);

	caption = new Caption(dialog, LAB_frame_dir_line, CAP_LEFT);
	frame_dir_choice = new Radio_list(caption, "frame_dir buttons",
		OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(LabelId),
		frame_number_dir, 0,
		this);
	caption->add_component(frame_dir_choice);
	dialog->add_component(caption);
}

void
Frame_dir_dialog::apply(Component *, void *)
{
	const char	*fdir;
	int		dir = frame_dir_choice->which_button();
	switch(dir)
	{
	case FRAME_DIR_UP:
		fdir = "up";
		break;
	case FRAME_DIR_DOWN:
		fdir = "down";
		break;
	default:	
		dialog->error(E_ERROR, GE_internal, __FILE__, 
			__LINE__);
		return;
	}
	dispatcher.send_msg(this, 0, "set %%frame_numbers = %s\n", 
		fdir);
	dialog->wait_for_response();
}

void
Frame_dir_dialog::reset(Component *, void *)
{
	frame_dir_choice->set_button(frame_number_dir);
}

Save_open_source_dialog::Save_open_source_dialog(Base_window *bw,
	Callback_ptr cb) : Dialog_box(bw)
{
	static const DButton	buttons[] =
	{
		{ B_apply, LAB_save, LAB_save_mne,
			(Callback_ptr)(&Save_open_source_dialog::save_cb) },
		{ B_apply, LAB_save_as, LAB_save_as_mne,
			(Callback_ptr)(&Save_open_source_dialog::save_as_dlg_cb) },
		{ B_apply, LAB_discard, LAB_discard_mne,
			(Callback_ptr)(&Save_open_source_dialog::discard_cb) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Save_open_source_dialog::cancel_cb) },
		{ B_help, LAB_none, LAB_none, 0 },
	};

	static const Sel_column	sel_col[] =
	{
		{ SCol_text,	0 },
	};

	Expansion_box	*box;
	Simple_text	*stext;
	const char	*initial_string = "";

	okcb = cb;
	open_list = 0;
	window = bw;
	current_selection = -1;
	file_selection_box = 0;
	dialog = new Dialog_shell(window->get_window_shell(),
		LAB_save_open_source, 0, this, buttons,
		sizeof(buttons)/sizeof(DButton),
		HELP_save_open_source_dlg);
	box = new Expansion_box(dialog, "box", OR_vertical);

	stext = new Simple_text(box, gm_format(GM_save_open_files),
		FALSE, HELP_none);
	box->add_component(stext);

	files = new Selection_list(box, "open_file_list", 10, 1, 
		sel_col, 1, &initial_string, SM_single, this,
		Search_none, 0,
		(Callback_ptr)Save_open_source_dialog::select_cb,
		(Callback_ptr)Save_open_source_dialog::deselect_cb);
	box->add_component(files, TRUE);

	box->update_complete();
	dialog->add_component(box);
}

Save_open_source_dialog::~Save_open_source_dialog()
{
	delete file_selection_box;
}

void
Save_open_source_dialog::select_cb(Component *, int sel)
{
	current_selection = sel;
	dialog->set_sensitive(TRUE);
}

void
Save_open_source_dialog::deselect_cb(Component *, int sel)
{
	if (sel == current_selection)
	{
		current_selection = -1;
		dialog->set_sensitive(FALSE);
	}
}

void
Save_open_source_dialog::cancel_cb(Component *, void *)
{
}

void
Save_open_source_dialog::remove_from_open_list(OpenSourceRecord *orp)
{
	OpenSourceRecord	*rptr = open_list;
	OpenSourceRecord	*prev = 0;
	for(; rptr; prev = rptr, rptr = rptr->next)
	{
		if (rptr == orp)
			break;
	}
	if (prev)
		prev->next = rptr->next;
	else
		open_list = rptr->next;
	delete rptr;
}

void
Save_open_source_dialog::save_as_dlg_cb(Component *p, void *)
{
	if (!file_selection_box)
	{
		file_selection_box = new FileSelectionDialog(p,
			labeltab.get_label(LAB_save_as_dlg_title), 0, 
			(Callback_ptr)&Save_open_source_dialog::save_as_cb, 
			this, HELP_save_as_dlg);
	}
	file_selection_box->popup();
}

void
Save_open_source_dialog::save_as_cb(Component *, char *path)
{
	OpenSourceRecord *orp = open_list;
	for(int i = 0; i < current_selection; i++)
		orp = orp->next;
	if (orp->file)
	{
		if (!orp->editor->save_current_as(path))
		{
			dialog->error(E_ERROR, GE_save_as_failed,
				orp->file, path);
			return;
		}
	}
	else
	{
		if (!orp->editor->save_stored_file(orp->stored, path))
		{
			dialog->error(E_ERROR, GE_save_as_failed,
				orp->stored->get_file(), path);
			return;
		}
	}
	remove_from_open_list(orp);
	build_list();
}

void
Save_open_source_dialog::save_cb(Component *, void *)
{
	OpenSourceRecord *orp = open_list;
	for(int i = 0; i < current_selection; i++)
		orp = orp->next;
	if (orp->file)
	{
		if (!orp->editor->save_current_as(orp->file))
		{
			dialog->error(E_ERROR, GE_save_source_failed,
				orp->file);
			return;
		}
	}
	else
	{
		if (!orp->editor->save_stored_file(orp->stored, 
			orp->stored->get_file()))
		{
			dialog->error(E_ERROR, GE_save_source_failed,
				orp->stored->get_file());
			return;
		}
	}
	remove_from_open_list(orp);
	build_list();
}

void
Save_open_source_dialog::discard_cb(Component *, void *)
{
	OpenSourceRecord *orp = open_list;
	for(int i = 0; i < current_selection; i++)
		orp = orp->next;
	if (orp->file)
	{
		orp->editor->discard_changes();
	}
	else
	{
		orp->editor->discard_changes(orp->stored);
	}
	remove_from_open_list(orp);
	build_list();
}

void
Save_open_source_dialog::build_list()
{
	OpenSourceRecord	*orp = open_list;
	int	rows = 0;

	if (!orp)
	{
		files->set_list(0, 0);
		dialog->set_sensitive(FALSE);
		current_selection = -1;
		if (okcb)
			(window_set->*okcb)(0, window);
		dialog->popdown();
		return;
	}
	Vector	*v = vec_pool.get();
	v->clear();
	for(; orp; orp = orp->next)
	{
		char	*file;
		if (orp->file)
			file = orp->file;
		else
			file = orp->stored->get_file();
			
		v->add(&file, sizeof(char *));
		rows++;
	}
	files->set_list(rows, (const char **)v->ptr());
	current_selection = -1;
	dialog->set_sensitive(FALSE);
	vec_pool.put(v);
}

void
Save_open_source_dialog::setup(OpenSourceRecord *list)
{
	if (open_list)
	{
		while(open_list)
		{
			OpenSourceRecord *orp = open_list->next;
			delete open_list;
			open_list = orp;
		}
	}
	open_list = list;
	build_list();
}
