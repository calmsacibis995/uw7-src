#ident	"@(#)debugger:gui.d/common/Dispatcher.C	1.50"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

// Debug headers
#include "Msgtypes.h"
#include "Message.h"
#include "Path.h"
#include "Transport.h"
#include "Severity.h"
#include "Vector.h"

// GUI headers
#include "UI.h"
#include "Eventlist.h"
#include "Dispatcher.h"
#include "Proclist.h"
#include "Windows.h"
#include "Dialogs.h"
#include "gui_label.h"

Transport transport(fileno(stdin), fileno(stdout), read, write, exit, query_handler);
Dispatcher	dispatcher;
int		in_script;
int		has_assoc_cmd;

// vector is used to keep a list of all the newly created processes between
// one MSG_cmd_complete and the next MSG_new_pty - that will give the set
// of processes that are associated with that pseudo-terminal
static Vector	vector;

// pack the arguments into the message for transfer to debug
static void
pack(Message &msg, Msg_id mid ...)
{
	va_list	ap;

	msg.clear();
	va_start(ap, mid);
	msg.bundle(mid, E_NONE, ap);
	va_end(ap);
}

// The current working directory is a global property, so all 
// change directory dialogs and all source panes must be updated 
// whenever the current directory changes
static void
set_directory(Message *m)
{
	Window_set	*ws;
	char		*working_directory = 0;

	m->unbundle(working_directory);
	if (!working_directory)
		working_directory = "";

	if ((chdir(working_directory) != 0) || (init_cwd() == 0))
	{
		// error: cannot change directory to '%s'
		display_msg(E_ERROR, GE_cant_cd, working_directory);
		return;
	}
	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_cd_dialog(current_dir);
		ws->update_source_wins();
}

static void
set_in_script(Boolean ins)
{
	if (ins)
	{
		in_script++;
		if (in_script > 1)
			return;
	}
	else
	{
		in_script--;
		if (in_script)
			return;

		has_assoc_cmd = 0;
		proclist.update_all();
	}

	Window_set	*ws = (Window_set *)windows.first();
	for ( ; ws; ws = (Window_set *)windows.next())
	{
		ws->set_in_script(ins);
	}
}

Dispatcher::Dispatcher()
{
	create_id = 0;
	first_new_process = TRUE;
	process_killed = FALSE;
	in_create = FALSE;
	notice_raised = FALSE;
	io_flag = TRUE;
	update_needed = FALSE;
	// initialize quit & sync_response messages
	pack(quit_msg, MSG_command, "quit\n");
	pack(sync_msg, MSG_sync_response, 0);

}

// received a sync request from debug, send back a sync response
void
Dispatcher::sync_response()
{
	transport.send_message(&sync_msg, TT_UI_notify, 0, 0);
}

// sprintf the format string and arguments into a command for debug
// The pointer to the object that generated the message (obj) is passed
// as the uicontext in the message, so that on getting the response,
// the dispatcher will send the incoming message back to the same object
// The DBcontext identifies the object in debug that the command is to
// apply to, if the command does not include an explicit process list
void
Dispatcher::send_msg(Command_sender *obj, DBcontext context, const char *fmt ...)
{
	const char	*cmdline;
	va_list		ap;

	va_start(ap, fmt);
	cmdline = do_vsprintf(fmt, ap);
	va_end(ap);

#if DEBUG > 1
	fprintf(stderr, "Dispatcher::send_msg: %s, db %x, ui %x\n",
		cmdline, context, obj);
#endif
	pack(out_msg, MSG_command, cmdline);
	transport.send_message(&out_msg, TT_UI_user_cmd, context, obj);
}

// cleanup is called just prior to aborting, e.g. getting a memory
// fault or some type of internal error. to avoid calls to malloc,
// since the malloc arena could be trashed at this point, we use a
// preinitialized quit message and directly invoke transport's
// send message rather than calling Dispatcher::send_msg()
void
Dispatcher::cleanup()
{
	transport.send_message(&quit_msg, TT_UI_user_cmd, 0, 0); 
}

// send the response to a debugger query
void
Dispatcher::send_response(DBcontext context, int response)
{
	pack(out_msg, MSG_response, response);
	transport.send_message(&out_msg, TT_UI_response, context, 0);
}

// direct a message from debug to the appropriate places:
// first, to the framework object (dialog, pane, etc) that generated the command,
// next, to the window set that owns the process, so that event notifications
//	are always displayed,
// and then,
//	if the message indicates a change in process state,
// 		to the process list
//	or a change in an event,
//		to the event list
void
Dispatcher::process_msg()
{
	Msg_id		mtype;
	Command_sender	*obj;
	ProcObj		*pobj;
	Window_set	*ws;

	current_msg.clear();
	if (!transport.get_next_message(&current_msg))
		return;	// called the query handler from inside Transport

	mtype = current_msg.get_msg_id();
	obj = (Command_sender *)current_msg.get_uicontext();
#if DEBUG > 1
        extern const char *Msg_type_names[];
        fprintf(stderr,"Dispatcher::process_msg: %s, db %x, ui %x\n",
                Msg_type_names[mtype], current_msg.get_dbcontext(), obj);
#endif

	if (mtype == MSG_quit)
	{
		exit_called = 1;
		exit(0);
	}

	if (mtype == MSG_sync_response)	// only gui should generate this
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// Find the window set where the message should be handled
	// A new process will not have a process object, but one
	// resulting from a create or grab will have a uicontext (command_sender)
	// A message for a newly forked process goes to its parent's window
	// Messages generated from a script go to the script's window set,
	// even if the message is about a process in a different window set

	pobj = proclist.find_obj(&current_msg);
	if (in_script && obj && (ws = obj->get_window_set()) != 0)
		;
	else if (pobj && (ws = pobj->get_window_set()) != 0)
		;
	else if (obj && (ws = obj->get_window_set()) != 0)
		;
	else if ((ws = (Window_set *)windows.first()) == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// Messages resulting from commands are handled by the object
	// that generated the command
	// Event notifications will have a process id (the dbcontext)
	// but no object, since they are not the result of specific commands,
	// and are displayed in the command window of the appropriate window set,
	// as is output from commands typed in the command line pane

	ws->init_sense_counts();

	if (obj && !in_script)
	{
		if (mtype == MSG_cmd_complete)
			obj->cmd_complete();
		else if (mtype != MSG_sync_request &&
			mtype != MSG_event_assigned)
			obj->de_message(&current_msg);
	}
	ws->inform(&current_msg, pobj);

	// all messages that indicate changes in process state (or event state)
	// go to the process list (or event list), regardless of what
	// object generated them
	switch(mtype)
	{
		// If this is the first process created in the current create command,
		// the process becomes the current process in its window set
		// The Process pointers are saved and handled later as a group if
		// MSG_new_pty is received
		case MSG_createp:
			if (pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				Process *newp = proclist.new_proc(&current_msg, ws,
					first_new_process, create_id);
				vector.add(&newp, sizeof(Process *));
				first_new_process = FALSE;
			}
			in_create = TRUE;
			break;

#ifdef DEBUG_THREADS
		case MSG_new_thread:
			// there are 2 cases here:
			// 1) main thread creation during process creation in
			//    the presence of libthread, or
			// 2) thread creation as part of live or core process
			//    grab
			proclist.proc_new_thread(&current_msg);
			break;

		case MSG_thread_create:
			// new thread as a result of thr_create
			// pobj is the creator thread
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.new_thread(&current_msg, (Thread *)pobj);
			break;

		case MSG_give_up_lwp:
			if (!pobj || !pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.set_state(pobj, State_off_lwp);
			break;

		case MSG_thr_suspend:
			if (!pobj || !pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.set_state(pobj, State_suspended);
			break;

		case MSG_pick_up_lwp:
		case MSG_thr_continue:
		case MSG_thr_continue_off_lwp:
		case MSG_es_halted_thread_start:
			if (!pobj || !pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.set_thread_state((Thread *)pobj, mtype, ws);
			break;
#endif

		case MSG_new_pty:
			if (vector.size() == 0)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				io_flag = TRUE;
			break;

		case MSG_proc_fork:
			if (!pobj || pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_forked(&current_msg, pobj);
			break;

#ifdef DEBUG_THREADS
		case MSG_thread_fork:
			if (!pobj || !pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_forked(&current_msg, pobj);
			break;
#endif

		case MSG_proc_exec:
			if (!pobj || pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_execed(&current_msg, (Process *)pobj);
			break;

		case MSG_new_core:
			if (pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				proclist.proc_grabbed(&current_msg, ws, 0, create_id);
				first_new_process = FALSE;
			}
			break;

#ifdef DEBUG_THREADS
		case MSG_new_core_thread:
			if (pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_new_thread(&current_msg);
			break;
#endif

		case MSG_grab_proc:
			if (pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				// if this is the first process grabbed in the
				// current grab command, the process becomes
				// the current process in its window set
				proclist.proc_grabbed(&current_msg, ws,
					first_new_process, create_id);
				first_new_process = FALSE;
			}
			break;

		case MSG_proc_exit:
#ifdef DEBUG_THREADS
		case MSG_thread_exit:
#endif
		case MSG_release_run:
		case MSG_release_suspend:
		case MSG_release_core:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			// FALL-THROUGH

		// not an error to not have a process in this case - this message
		// could appear during a create if one process in a pipeline fails
		case MSG_proc_killed:
			if (pobj)
			{
				proclist.remove_obj(pobj);
				// If obj is non-null, this message is the result
				// of a command, like kill.  Resetting the current
				// process is delayed until the command is finished
				// so there won't be a lot of churn if more than
				// one process is killed.  If obj is null, the
				// process exited while running
				if (obj)
					process_killed = 1;
				else if (!ws->current_obj())
					// if current object was deleted, 
					// pick another one
					ws->set_current(0);
			}
			break;

		// These messages are generated when an event triggers, but may
		// also be generated for other reasons.  is_incomplete() says
		// they are needed to complete the event notification.
		// Ditto for the next group and MSG_source_file
		case MSG_line_src:
		case MSG_disassembly:
		case MSG_dis_line:
		case MSG_line_no_src:
		case ERR_get_text:	// bad text address, can't do dis
			if (!pobj)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				display_msg(&current_msg);
			}
			else if (pobj->is_incomplete())
				proclist.finish_update(&current_msg, pobj, in_create);
			break;

		case MSG_loc_sym_file:
		case MSG_loc_sym:
		case MSG_loc_unknown:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else if (pobj->is_incomplete())
				proclist.update_location(&current_msg,	pobj);
			break;

		case MSG_source_file:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else if (pobj->is_incomplete())
				proclist.set_path(&current_msg,	pobj);
			break;

		case MSG_es_core:
			// only get this when core file is first
			// grabbed; use it to set the current
			// process or thread for the core file;
			// for threads, this is the thread that caused
			// the process to dump core
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
#ifdef DEBUG_THREADS
				if ((pobj->get_type() == Type_process)
					&& (((Process *)pobj)->has_threads()))
				{
					// faulting thread not a
					// user thread - set current
					// to first user thread
					ws->set_current((ProcObj *)((Process *)pobj)->get_head()->thread());

				}
				else
#endif
					ws->set_current(pobj);
			}
			break;
			
		// first message in an event notification
		case MSG_es_halted:
		case MSG_es_stepped:
		case MSG_es_signal:
		case MSG_es_sysent:
		case MSG_es_sysxit:
		case MSG_es_stop:
#if EXCEPTION_HANDLING
		case MSG_es_eh_thrown:
		case MSG_es_eh_caught:
#endif
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_stopped(pobj);
			break;
		case MSG_step_not_done:
			// if not animated, we are in middle of
			// a step -c cnt; do not process
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else if (pobj->is_animated())
			{
				proclist.proc_stopped(pobj);
				if (!ws->is_animated())
					pobj->clear_animation();
			}
			break;
#ifdef DEBUG_THREADS
		case MSG_es_halted_off_lwp:
			if (!pobj || !pobj->is_thread())
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_halted_off_lwp((Thread *)pobj);
			break;
#endif

		case ERR_step_watch:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.set_state(pobj, State_stepping);
			break;

		case MSG_proc_start:
			if (pobj)
				proclist.set_state(pobj, State_running);
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;

		// the process was running to evaluate a function call in an expression
		case MSG_proc_stop_fcall:
			if (pobj)
			{
				proclist.set_state(pobj, State_stopped, 1);
				update_needed = TRUE;
			}
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;

		case MSG_set_frame:
			if (!pobj)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				break;
			}
			proclist.set_frame(&current_msg, pobj);
			break;

		case MSG_rename:
			proclist.rename(&current_msg);
			break;

		case MSG_event_assigned:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			event_list.new_event(&current_msg, pobj);
			break;

		case MSG_bkpt_set:
		case MSG_bkpt_set_addr:
			event_list.breakpt_set(&current_msg);
			break;

 		case MSG_event_changed:
			event_list.change_event(&current_msg, pobj);
 			break;
 
		case MSG_event_deleted:
			event_list.delete_event(&current_msg, pobj);
			break;

		case MSG_event_enabled:
			event_list.enable_event(&current_msg);
			break;

		case MSG_event_disabled:
			event_list.disable_event(&current_msg);
			break;

		// debug is waiting for an acknowledgement from the gui,
		// so it won't get too far ahead in spitting out output
		case MSG_sync_request:
			sync_response();
			// if the command was a create command, the processes are
			// updated at the end, after the gui is sure there are no
			// problems in creating processes later in the pipeline
			// Also done for processes changing state during expression
			// evaluation, to avoid problems with multiple updates
			if (vector.size())
				update_new_processes();
			else if (update_needed)
			{
				proclist.update_touched();
				update_needed = FALSE;
			}
			if (!first_new_process)
			{
				first_new_process = TRUE;
				create_id++;
			}
			in_create = FALSE;
			io_flag = FALSE;
			if (process_killed)
			{
				Window_set	*ws = (Window_set *)windows.first();
				for ( ; ws; ws = (Window_set *)windows.next())
				{
					if (!ws->current_obj())
						ws->set_current(0);
				}
				process_killed = 0;
			}
			break;

		case MSG_cmd_complete:
			break;

		// if a create fails, all processes are killed.
		case ERR_create_fail:
			vector.clear();
			break;

		// keep the Create dialog up-to-date
		case MSG_oldargs:
			set_create_args(&current_msg);
			first_new_process = 1;
			break;

		// keep the Language dialog up-to-date
		case MSG_set_language:
			set_lang(&current_msg);
			break;

		// keep the frame dir dialog and stack panes up-to-date
		case MSG_set_frame_dir:
			set_frame_dir(&current_msg);
			break;

		// keep the dis mode dialog and dis panes up-to-date
		case MSG_set_dis_mode:
			set_dis_mode(&current_msg);
			break;


		// update current context after a jump command
		case MSG_jump:
			if (!pobj)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_jumped(&current_msg, pobj);
			break;

		// update current working directory after cd command
		case MSG_cd:
			set_directory(&current_msg);
			break;

		case MSG_script_on:
			set_in_script(TRUE);
			break;

		case MSG_script_off:
			set_in_script(FALSE);
			break;

		case MSG_assoc_cmd:
			send_msg(0, 0, "\n");
			if (!has_assoc_cmd)
			{
				has_assoc_cmd = 1;
				if (ws->get_event_action() != A_raise && !notice_raised)
				{
					notice_raised = TRUE;
					display_msg((Callback_ptr)(&Window_set::send_interrupt_cb),
						(Window_set *)windows.first(),
						LAB_dok, LAB_interrupt,
						AT_message, GM_assoc_cmd);
				}
			}
			break;

#if EXCEPTION_HANDLING
		case MSG_uses_eh:
			if (pobj)
				pobj->get_program()->set_uses_eh();
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
#endif

		default:
			break;
	}
	ws->set_sensitivity();
}


// ask debug for information needed before the gui can continue
// the response is handled by get_response()
void
Dispatcher::query(Command_sender *obj, DBcontext context, const char *fmt ...)
{
	const char	*cmdline;
	va_list		ap;

	va_start(ap, fmt);
	cmdline = do_vsprintf(fmt, ap);
	va_end(ap);

#if DEBUG > 1
	fprintf(stderr, "Dispatcher::query: %s, db %x, ui %x\n",
		cmdline, context, obj);
#endif
	pack(out_msg, MSG_command, cmdline);
	transport.send_message(&out_msg, TT_UI_query, context, obj);
}

Message *
Dispatcher::get_response()
{
	Msg_id	mtype;

	for (;;)
	{
		transport.get_response(&current_msg);
		mtype = current_msg.get_msg_id();
#if DEBUG > 1
        extern const char *Msg_type_names[];
        fprintf(stderr,"Dispatcher::get_response: %s, db %x, ui %x\n",
                Msg_type_names[mtype], current_msg.get_dbcontext(), 
		current_msg.get_uicontext());
#endif

		if (mtype == MSG_sync_request)
			sync_response();
		else if (mtype == MSG_cmd_complete)
		{
			transport.query_done();
			return 0;
		}
		else
			return &current_msg;
	}
}

// update all processes in the most recent create command
void
Dispatcher::update_new_processes()
{
	Process **pptr = (Process **)vector.ptr();
	int	total = vector.size()/sizeof(Process *);

	for (int i = 0; i < total; i++, pptr++)
		(*pptr)->finish_update(io_flag);
	vector.clear();
}
