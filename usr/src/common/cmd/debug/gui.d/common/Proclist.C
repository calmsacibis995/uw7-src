#ident	"@(#)debugger:gui.d/common/Proclist.C	1.73"

// GUI headers
#include "Eventlist.h"
#include "Proclist.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Events.h"
#include "UI.h"
#include "Menu.h"
#include "Sense.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "Buffer.h"
#include "Message.h"
#include "Msgtypes.h"
#include "Machine.h"
#include "str.h"
#include "Vector.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#ifdef __cplusplus
#define	PROCOBJ	ProcObj
#else
#define	PROCOBJ
#endif

#ifdef DEBUG_THREADS
#define UNBUNDLE(m,c,pg,p,pid,tid,f,l,cmd) \
	m->unbundle(c, pg, p, pid, tid, f, l, cmd)
#define UNBUNDLE_R(m,c,pg,p,pid,tid,cmd) \
	m->unbundle(c, pg, p, pid, tid, cmd)
#else
#define UNBUNDLE(m,c,pg,p,pid,tid,f,l,cmd) \
	m->unbundle(c, pg, p, pid, f, l, cmd)
#define UNBUNDLE_R(m,c,pg,p,pid,tid,cmd) \
	m->unbundle(c, pg, p, pid, cmd)
#endif

Process_list proclist;

static int
numeric_signal_comp(const void *s1, const void *s2)
{
	// s1 and s2 are both pointers to name and number string 
	// pairs this routine fetches the number string from each 
	// and compares their numeric value.

	char	*ptr1 = ((char **)s1)[1];
	char	*ptr2 = ((char **)s2)[1];
	long	aval = strtol(ptr1, NULL, 0);
	long	bval = strtol(ptr2, NULL, 0);

	if (aval<bval) return -1;
	if (aval>bval) return 1;
	return 0;

}

ProcObj::ProcObj(DBcontext i, const char *n, ProcObj_state s, ProcObj_type t,
	Window_set *ws, Boolean inc)
{
	id = i;
	state = s;
	type = t;
	bad_state = FALSE;
#ifdef DEBUG_THREADS
	used_flag = FALSE;
#endif
	name = makestr(n);
	window_set = ws;
	location = 0;
	function = 0;
	file = 0;
	line = 0;
	path = 0;
	frame = -1;	// 0 is a valid frame number
	incomplete = inc;
	ehead = etail = 0;
	animated = FALSE;

	// while executing inside a script, to avoid constant updates
	// to the window set when process states are changing, all 
	// updated processes are marked as 'touched'. at the end of the 
	// script, all touched processes are updated in one swoop.
	// touched may also be set at other times, as when a process
	// is set running to evaluate a function call in an expression.
	if (in_script || has_assoc_cmd)
		touched = TRUE;
	else
		touched = FALSE;
}

ProcObj::~ProcObj()
{
	Elink *item, *next;

	// delete this obj from each event's proc list
	for (item = ehead; item; item = next)
	{
		item->event()->delete_obj( this );
		next = item->next();
		item->unlink();
		delete item;
	}

	// update all event windows
	Window_set	*ws = (Window_set *)windows.first();

	for(; ws; ws = (Window_set *)windows.next()) 
	{
		Base_window	**win = ws->get_base_windows();
		for(int i = 0; i < windows_per_set; i++, win++)
		{
			Event_pane	*eWin;

			if (!*win)
				continue;
			eWin = (Event_pane *)(*win)->get_pane(PT_event);
			if( eWin )
			{
				eWin->update_cb( 0, RC_set_current, 0,
					ws->current_obj());
			}
		}
	}
	
	delete name;
	delete file;
	delete path;
	delete function;
	delete location;
}

void
ProcObj::set_location()
{
	Message	*m;
	char	*buf;	// throwaways
	Word	tmp;	// throwaways
	char	*func = 0;
	char	*loc = 0;

	dispatcher.query(0, 0, "ps -p %s\n", name);

	while ((m = dispatcher.get_response()) != 0)
	{
		// current indicator, progname, procname, pid, thrid, func,
		//	 location, cmdline
		// should be doing ps on stopped processes and core files only

		switch(m->get_msg_id())
		{
#ifdef DEBUG_THREADS
		case MSG_threads_ps_stopped:
		case MSG_threads_ps_core:
		case MSG_threads_ps_off_lwp:
		case MSG_threads_ps_core_off_lwp:
		case MSG_threads_ps_suspend:
		case MSG_threads_ps_core_suspend:
			m->unbundle(buf, buf, buf, tmp, tmp, func, loc, buf);
			update_location(func, loc);
#else
		case MSG_ps_stopped:
		case MSG_ps_core:
			m->unbundle(buf, buf, buf, tmp, func, loc, buf);
			update_location(func, loc);
#endif
			break;

		default:
			continue;
		}
	}
}

Program *
ProcObj::get_program()
{
	return 0;	// overridden by derived class functions
}

Process *
ProcObj::get_process()
{
	return 0;	// overridden by derived class functions
}

#ifdef DEBUG_THREADS
void
ProcObj::set_animation()
{
	// overridden by derived class functions
}

void
ProcObj::clear_animation()
{
	// overridden by derived class functions
}
#endif

//query for the events applicable to this process
void
ProcObj::init_events()
{
	Word	event_id;
	Message	*m;
	Event	*e = 0;
	Word	count = 0;
	char	*qflag = 0;
	char	*state = 0;
	char	*condition = 0;
	char	*systype = 0;
	char	*cmd = 0;

	dispatcher.query(0, id, "events -p %s\n", name);
	while ((m = dispatcher.get_response()) != 0)
	{
		switch(m->get_msg_id())
		{
		case MSG_stop_event:
			m->unbundle(event_id, qflag, state, count,
				condition, cmd); 
			break;

		case MSG_syscall_event:
			m->unbundle(event_id, qflag, state, systype, count,
				condition, cmd);
			break;

		case MSG_signal_event:
			m->unbundle(event_id, qflag, state, condition, cmd);
			break;

		case MSG_onstop_event:
			m->unbundle(event_id, state, cmd);
			break;

#if EXCEPTION_HANDLING
		case MSG_eh_event:
		{
			char *ehtype;
			m->unbundle(event_id, qflag, state, ehtype,
					condition, cmd);
			break;
		}
#endif

		default:
			continue;
		}

		e = event_list.findEvent( (int)event_id ); 

		if (!e)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		add_event( e );
		e->add_obj( this );
	}
}

void
ProcObj::update_done(Reason_code rc)
{
	incomplete = FALSE;
	touched = FALSE;
	window_set->update_obj(this, rc);
}

void
ProcObj::finish_update()
{
	incomplete = FALSE;
	touched = FALSE;
}

void
ProcObj::partial_update()
{
	touched = TRUE;
	window_set->update_obj(this);
}

void
ProcObj::update_location(const char *func, const char *loc)
{
	delete function;
	delete location;
	delete file;

	function = (func && *func) ? makestr(func) : 0;
	location = (loc && *loc) ? makestr(loc) : 0;

	file = 0;
	line = 0;
	if (location)
	{
		// location is in the form of
		//	file@line
		// or
		//	hex-address
		char *p = strchr(loc, '@');
		if (p)
		{
			size_t len = p - (char *)loc;
			file = new char[len+1];
			strncpy(file, loc, len);
			file[len] = '\0';
			line = atoi(p+1);
		}
	}
}

// convert the state into a string for display in the ps and status panes
const char *
ProcObj::get_state_string()
{
	LabelId	lid;
	switch(state)
	{
	default:
	case State_none:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		lid = LAB_dead;
		break;

	case State_core:
		lid = LAB_core;
		break;
	case State_running:
		lid = LAB_running;
		break;
	case State_stepping:
		lid = LAB_stepping;
		break;
	case State_stopped:
		lid = LAB_stopped;
		break;
#ifdef DEBUG_THREADS
	case State_off_lwp:
		lid = LAB_off_lwp;
		break;
	case State_core_off_lwp:
		lid = LAB_core_off_lwp;
		break;
	case State_suspended:
		lid = LAB_suspended;
		break;
	case State_core_suspended:
		lid = LAB_core_suspended;
		break;
#endif
	}
	return labeltab.get_label(lid);
}

// get the breakpoints for this process or thread that meet
// given constraints: address is between begin_addr and end_address  
// or file and line match breakpoint file and line
// Return the list of addresses in the vector

int
ProcObj::get_break_list(Iaddr begin_addr, Iaddr end_addr, Vector *breaks,
	Boolean get_event_ids, const char *file, int line)
{
	Elink	*list;
	Event	*e;
	int	end = 0;
	int	num = 0;
	
	breaks->clear();

	for (list = ehead; list; list = list->next())
	{
		e = list->event();

		if (e->get_type() != ET_stop)
			continue;

		Breakpoint	*b = ((Stop_event *)e)->get_breakpts();
		int		n = ((Stop_event *)e)->get_nbreakpts();

		for (int count = 0; count < n; count++, b++)
		{
			if (b->addr < begin_addr || b->addr > end_addr)
			{
				if (!file)
					continue;
				if (!b->file 
					|| (strcmp(file, b->file) != 0) 
					|| (b->line != line))
					continue;
			}

			if (get_event_ids)
			{
				int e_id = e->get_id();
				breaks->add(&e_id, sizeof(int));
			}
			else
				breaks->add(&b->addr, sizeof(int));
			num++;
		}
	
	}
	
	breaks->add(&end, sizeof(int));
	return num;
}

void
ProcObj::add_event(Event *e)
{
	Elink	*item = new Elink(e);
	Elink	*list;

	if (!ehead)
	{
		ehead = etail = item;
		return;
	}

	int eid = e->get_id();
	for (list = ehead; list; list = list->next())
	{
		int leid = list->event()->get_id();

		if (eid < leid)
			break;
		else if (eid == leid)
			// already added, can happen if called from
			// Process_list::update_all() at end of script
			return;
	}

	if (list)
	{
		item->prepend(list);
		if (list == ehead)
			ehead = item;
	}
	else
	{
		item->append(etail);
		etail = item;
	}
}

void
ProcObj::delete_event(Event *e)
{
	Elink	*item;

	for (item = ehead; item && e != item->event(); item = item->next())
		;

	if (!item)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	if (item == ehead)
		ehead = ehead->next();
	if (item == etail)
		etail = etail->prev();
	item->unlink();
	delete item;
}

int
ProcObj::check_sensitivity(Sensitivity *sense)
{
	if (sense->proc_io_redir() && !get_program()->is_io_redirected())
		return 0;
	if (sense->proc_live() && !is_live())
		return 0;
	if (sense->proc_stopped() && !is_halted())
		return 0;
	if (sense->proc_runnable() && !is_runnable())
		return 0;
	if (sense->proc_stop_core() && !(is_halted() || is_core()))
		return 0;
	if (sense->proc_running() && !is_running())
		return 0;
#if EXCEPTION_HANDLING
	if (sense->proc_uses_eh() && !get_program()->uses_eh())
		return 0;
#endif
	return 1;
}

int
ProcObj::get_objects(Vector *v)
{
	Message	*msg;
	char	*obj_name = "";
	int	total = 0;

	v->clear();
	dispatcher.query(0, id, "map\n");
	while((msg = dispatcher.get_response()) != 0)
	{
		Word	tmp1, tmp2, tmp3;
		char	*tmp4;
		char	*new_name;

		if (msg->get_msg_id() == MSG_map)
		{
			msg->unbundle(tmp1, tmp2, tmp3, tmp4, new_name);
			if (!new_name || !*new_name)
				continue;

			if( strcmp( new_name, "[STACK]" ) == 0 )
				continue;

			new_name = basename(new_name);
			const char	**sptr = (const char **)v->ptr();
			int		count = v->size()/sizeof(char **);
			int		foundit = 0;
			for (; count; --count, ++sptr)
			{
				if (strcmp(new_name, *sptr) == 0)
					foundit = 1;
			}

			if (!foundit)
			{
				total++;
				obj_name = str(new_name);
				v->add(&obj_name, sizeof(char **));
			}
		}
		else if (msg->get_msg_id() != MSG_map_header)
			display_msg(msg);
	}

	qsort(v->ptr(), total, sizeof(char *), alpha_comp);
	return total;
}

// This routine uses the vector package to handle the list of outstanding
// signals scheduled to be applied to a process. This is sort of like
// killing a flea with a warhead, as it's doubtful there'll ever be
// more than, oh, about 2 signals outstanding, but it handles varying
// amounts of data coming back
//
// What is done is as follows:
//	Send a 'pending' message to the debugger
//	If there are no signals pending, return an array of pointers to
//	char that holds two elements that point at blanks
//
//	If there are signals pending, then
//		stash all the responses from the debugger in the vsig vector
//		construct a vector that points to the data
//		sort the second vector
int
ProcObj::get_pending_signals(Order order, Vector *vptrs, Vector *vsig)
{
	Message	*msg;
	int	num_sigs = 0;

	dispatcher.query(0, id, "pending\n");
	vsig->clear();
	while ((msg = dispatcher.get_response()) != 0)
	{
		char	*name;
		char	buf[MAX_INT_DIGITS];
		Word	sig;

		Msg_id mid = msg->get_msg_id();
		if (mid == MSG_signame)
		{
			msg->unbundle(sig, name);
			vsig->add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vsig->add(buf, strlen(buf)+1);
			num_sigs++;
		}
		else if (mid == MSG_help_hdr_sigs
			|| mid == MSG_newline) // no action needed
			continue;
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	if (num_sigs > 0)
	{
		// get pointers
		char *ptr = (char *)vsig->ptr();
		vptrs->clear();
		for (int i = 0; i < num_sigs; i++)
		{
			// add name
			vptrs->add(&ptr, sizeof(char *));
			ptr += strlen(ptr)+1;
			// add number
			vptrs->add(&ptr, sizeof(char *));
			ptr += strlen(ptr)+1;
		}

		if (order == Alpha)
			qsort(vptrs->ptr(), num_sigs, sizeof(char *[2]), alpha_comp);
		else
			qsort(vptrs->ptr(), num_sigs, sizeof(char *[2]), numeric_signal_comp);
	}
	else
	{	// no data!
		char	*ptr = "";
		num_sigs = 1;
		vptrs->clear();
		vptrs->add(&ptr, sizeof(char *));
		vptrs->add(&ptr, sizeof(char *));
	}
	return num_sigs;
}

int
ProcObj::get_functions(Vector *v, const char *comp_unit,
	const char *obj, const char *filter, int dashg)
{
	Message		*msg;
	const char	*sflag = dashg ? "-s" : "";
	int		total = 0;

	if (comp_unit)
		dispatcher.query(0, id, "functions %s -f %s %s\n", 
			sflag, comp_unit, filter);
	else if (obj)
		dispatcher.query(0, id, "functions %s -o %s %s\n",
			sflag, obj, filter);
	else
		dispatcher.query(0, id, "functions %s %s\n", sflag, 
			filter);

	v->clear();
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_function)
		{
			char *name;
			msg->unbundle(name);
			name = makestr(name);	
			v->add(&name, sizeof(char *));
			total++;
		}
		else if (msg->get_msg_id() != MSG_function_header)
			display_msg(msg);
	}
	return total;
}

void
ProcObj::get_frame_info()
{
	Message		*msg;
	char		*s;
	char		*p;
	size_t		len;

	delete function;
	delete file;
	delete location;
	function = file = location = 0;

	dispatcher.query(0, id, "print %%frame, %%func, %%file, %%line, %%loc\n");

	// the response is in the form:
	//	frame # (decimal)
	//	function name in quotes (char string) or 0
	//	file name in quotes (char string) or 0
	//	line number (decimal)
	//	location (char string or hex)
	// separated by spaces. all fields are assumed to be non-empty.

	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_print_val)
		{
			display_msg(msg);
			continue;
		}

		msg->unbundle(s);
		frame = (int)strtol(s, &p, 10);
		if (!p)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		s = p;
		while (*++s == ' ');
		if (*s == '0')
		{
			p = s + 1;
			len = 0;
		}
		else if (*s == '"' && (p = strchr(++s, '"')) != NULL)
			len = p++ - s;
		else
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		if (len)
		{
			function = new char[len+1];
			strncpy(function, s, len);
			function[len] = '\0';
		}
		s = p;
		while (*s && *s != '"' && *s != '0')
			++s;

		if (*s == '0')
		{
			len = 0;
			p = s + 1;
		}
		else if (*s == '"' && (p = strchr(++s, '"')) != NULL)
			len = p++ - s;
		else
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		if (len)
		{
			file = new char[len+1];
			strncpy(file, s, len);
			file[len] = '\0';
		}
		s = p;

		while (*s == ' ') ++s;
		if (*s == '\0')
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		line = (int)strtol(s, &s, 10);

		if (file && line)
		{
			location = makesf(strlen(file) + MAX_INT_DIGITS + 1,
					"%s@%d", file, line);
		}
		else if (s)
		{
			// hex number starts with "0x"
			while(*s && *s != '0') ++s;
			p = s;
			// last field followed by '\n'
			while (*p && *p != '\n') ++p;
			if (*s == '\0' || *p == '\0')
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				continue;
			}
			len = p - s;
			location = new char[len + 1];
			strncpy(location, s, len);
			location[len] = '\0';
		}
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}


#ifdef DEBUG_THREADS
Thread::Thread(Process *p, thread_t i, const char *n, ProcObj_state s,
		Window_set *ws) :
	PROCOBJ(0, n, s, Type_thread, ws, TRUE)
{
	proc = p;
	thrid = i;
	proc->add_thread(this);
}

Thread::Thread(DBcontext id, Process *p, const char *n, Window_set *ws) :
	PROCOBJ(id, n, State_none, Type_thread, ws, TRUE)
{
	proc = p;
	if (touched)
		do_ps(FALSE);
	else
	{
		init_events();
		do_ps(TRUE);
		incomplete = FALSE;
	}
	proc->add_thread(this);
	window_set->add_obj(this, FALSE);
}

void
Thread::set_dbcontext(DBcontext nid)
{
	id = nid;
	if (!touched)
	{
		incomplete = FALSE;
		if (!proc->create_failed())
		{
			init_events();
			window_set->update_obj(this);
		}
	}
}

void
Thread::do_ps(Boolean update)
{
	Message	*m;
	char	*buf;	// throwaways
	Word	tmp;	// throwaways
	Word	tid;
	char	*func;
	char	*loc;
	ProcObj_state	st;

	if (proc->create_failed())
		return;

	dispatcher.query(0, 0, "ps -p %s\n", name);

	while ((m = dispatcher.get_response()) != 0)
	{
		tid = 0;
		func = 0;
		loc = 0;
		st = State_none;

		// current indicator, progname, procname, pid, thrid, func,
		//	 location, cmdline
		// a thread at creation time can be in off-lwp or
		// suspended states depending on how the thread is
		// created, e.g. multiplexed, bound, or suspended

		switch(m->get_msg_id())
		{
		case MSG_threads_ps_stopped:
			st = State_stopped;
			break;
		case MSG_threads_ps_off_lwp:
			st = State_off_lwp;
			break;
		case MSG_threads_ps_suspend:
			st = State_suspended;
			break;
		default:
			continue;
		}
		m->unbundle(buf, buf, buf, tmp, tid, func, loc, buf);
	}
	if (tid > 0)
		thrid = (int)tid;
	if (st != State_none)
		state = st;
	if (update)
		update_location(func, loc);
}

Program *
Thread::get_program()
{
	return proc->get_program();
}

Process *
Thread::get_process()
{
	return proc;
}

// thread animation is tracked in the parent process,
// so that if the process exits (exit() called explicitly or
// killed as a result of signal), we can clear the animation
// flags for all threads that were in animation at the time of
// of the process exit.
void
Thread::set_animation()
{
	// set animation in parent process
	proc->inc_animation();
	animated = 1;
}

void
Thread::clear_animation()
{
	// clear animation in parent process
	proc->dec_animation();
	animated = 0;
}
#endif

// create a new process, and if necessary, create a new program
// and link it in
Process::Process(DBcontext new_id, const char *progname, const char *name,
	ProcObj_state s, Window_set *ws, Boolean inc, Boolean make_current,
	int create_id, Boolean in_create) : 
	PROCOBJ(new_id, name, s, Type_process, ws, inc)
{
	char *cmd_line = 0;

	pid = -1;
#ifdef DEBUG_THREADS
	head = tail = 0;
#endif
	create_fail = FALSE;

	do_ps(cmd_line, in_create);
	if (!touched && !has_threads() && !create_fail)
	{
		init_events();
	}

	if ((program = proclist.find_program(progname)) == 0)
	{
		program = new Program(progname, cmd_line, create_id);
		proclist.add_program(program);
	}
	program->add_process(this);
	if (!create_fail)
		window_set->add_process(this, make_current);
}

Process::~Process()
{
#ifdef DEBUG_THREADS
	if (head)
	{
		Plink *p, *next;
		// delete threads
		for (p = head; p; p = next)
		{
			next = p->next();
			// dtor for Thread will delete itself from window_set
			delete p->thread();
			p->unlink();
			delete p;
		}
		return;
	}
#endif
	if (window_set)
		window_set->delete_process(this);
}

Program *
Process::get_program()
{
	return program;
}

Process *
Process::get_process()
{
	return this;
}

#ifdef DEBUG_THREADS
// we assume that animation is turned on at the lowest level, i.e.
// on a thread or unthreaded process, and turned off either at the
// lowest level or the parent process of an animated thread.
// the latter can happen when exit() is called while animating
// a thread.
void
Process::set_animation()
{
	animated = 1;
}

void
Process::clear_animation()
{
	// clearing animation on a process clears
	// all of its child threads' animation
	// Note: we assume the process' window set
	//       has already been cleared, and that there
	//       can only be one animation per window set
	if (head)
	{
		for (Plink *p = head; p; p = p->next())
		{
			Thread		*thr = p->thread();
			if (!thr->is_animated())
				continue;
			thr->get_window_set()->clear_animation();
			thr->clear_animation();
		}
		// assert (animated == 0);
	}
	else
		animated = 0;
}

void
Process::add_thread(Thread *thr)
{
	Plink *p = new Plink(thr);
	if (tail)
		p->append(tail);
	else
		head = p;
	tail = p;
}

void
Process::delete_thread(Thread *thr)
{
	Plink	*item;

	if (!thr || thr->get_process() != this)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (item = head; item && thr != item->thread(); item = item->next())
		;
	if (!item)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (item == head)
		head = head->next();
	if (item == tail)
		tail = tail->prev();
	item->unlink();
	delete item;
	delete thr;
}
#endif

// Only need to do a ps on the process when it is first created, grabbed,
// or exec'd, to get command line, after that all information needed should
// be available from event notification messages
// Threads are created if they exist.

void
Process::do_ps(char *&cmd_line, Boolean in_create)
{
	Message	*m;
	char	*buf;	// throwaways
	struct	Ps_info
	{
		char	*name;
		Word	pid;
		Word	tid;
		char	*func;
		char	*loc;
		char	*cmd;
		ProcObj_state	state;
	} ps;
	int	nthreads = 0;
	Vector	*v;

	dispatcher.query(0, 0, "ps -p %s\n", name);

	cmd_line = 0;
	line = 0;

	// there are 2 cases here:
	// 1) create or grab of an un-threaded process - old 4.2 types, 
	//    not linked with libthread
	// 2) create or grab of a multi-threaded process, live or core 
	// Note: create can fail when one of the processes in the pipeline 
	//       fail. in this case, we set a flag so that later thread
	//       processing can handle it.
	v = vec_pool.get();
	v->clear();
	while ((m = dispatcher.get_response()) != 0)
	{
		ps.name = 0;
		ps.pid = 0;
		ps.tid = (Word)-1;
		ps.func = 0;
		ps.loc = 0;
		ps.cmd = 0;
		ps.state = State_none;

		// current indicator, progname, procname, pid, thrid, func,
		//	 location, cmdline
		// assume we're doing ps on stopped processes and core files,
		// and thrid for unthreaded process is -1 and > 0 for threads

		switch(m->get_msg_id())
		{
#ifdef DEBUG_THREADS
		case MSG_threads_ps_stopped:
			ps.state = State_stopped;
			break;
		case MSG_threads_ps_core:
			ps.state = State_core;
			break;
		case MSG_threads_ps_off_lwp:
			ps.state = State_off_lwp;
			break;
		case MSG_threads_ps_core_off_lwp:
			ps.state = State_core_off_lwp;
			break;
		case MSG_threads_ps_suspend:
			ps.state = State_suspended;
			break;
		case MSG_threads_ps_core_suspend:
			ps.state = State_core_suspended;
			break;
#else
		case MSG_ps_stopped:
			ps.state = State_stopped;
			break;
		case MSG_ps_core:
			ps.state = State_core;
			break;
#endif

		default:
			continue;
		}
		UNBUNDLE(m, buf, buf, ps.name, ps.pid, ps.tid, ps.func, ps.loc, ps.cmd);
		v->add(&ps, sizeof(ps));
		if ((int)ps.tid > 0)
			nthreads++;
	}
	if (v->size() <= 0)
	{
		if (in_create)
			// create failure
			create_fail = TRUE;
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		vec_pool.put(v);
		return;
	}
	Ps_info	*p = (Ps_info *)v->ptr();
	// assume all threads have the same command-line
	if (p->cmd && *p->cmd)
		cmd_line = p->cmd;
	if (nthreads == 0)
	{
		// an un-threaded process
		if (!touched)
			update_location(p->func, p->loc);
		pid = (pid_t)p->pid;
	}
#ifdef DEBUG_THREADS
	else
	{
		for (; nthreads > 0; --nthreads, ++p)
		{
			Thread *thr = new Thread(this, (thread_t)p->tid, 
					p->name, p->state, window_set);
			if (!touched)
				thr->update_location(p->func, p->loc);
			if (pid < 0)
				pid = (pid_t)p->pid;
		}
	}
#endif
	vec_pool.put(v);
}

void
Process::finish_update(Boolean io_flag)
{
	ProcObj::finish_update();
	program->io_redirected = io_flag;
#ifdef DEBUG_THREADS
	if (has_threads())
	{
		for (Plink *link = head; link; link = link->next())
		{
			ProcObj *throbj = link->procobj();
			throbj->finish_update();
			window_set->update_obj(throbj);
		}
		return;
	}
#endif
	window_set->update_obj(this);
}

Program::Program(const char *name, const char *cmd, int cid)
{
	progname = makestr(name);
	cmd_line = cmd ? makestr(cmd) : 0;
	head = tail = 0;
	io_redirected = FALSE;
	used_flag = FALSE;
	create_id = cid;
#if EXCEPTION_HANDLING
	uses_eh_flag = FALSE;
#endif
}

// add a new process to the program's list.
// It's ok to just add to the end since debugger's ids are
// assigned in increasing order
void
Program::add_process(Process *ptr)
{
	Plink *item = new Plink(ptr);
	if (tail)
		item->append(tail);
	else
		head = item;
	tail = item;
}

// remove a process that died or was released
void
Program::delete_process(Process *ptr)
{
	Plink *item;

	if (!ptr || ptr->program != this)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (item = head; item && ptr != item->process(); item = item->next())
		;
	if (!item)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (item == head)
		head = head->next();
	if (item == tail)
		tail = tail->prev();
	item->unlink();
	delete item;
	delete ptr;
}

Process_list::Process_list()
{
	last_refd = 0;
	head = tail = 0;
}

// add a new program, keeping the list in alphabetical order
void
Process_list::add_program(Program *ptr)
{
	Plink	*item = new Plink(ptr);
	Plink	*list;

	if (!ptr || !ptr->progname)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!head)
	{
		head = tail = item;
		return;
	}

	for (list = head; list; list = list->next())
	{
		if (strcmp(ptr->progname, list->program()->progname) < 1)
			break;
	}

	if (list)
	{
		item->prepend(list);
		if (list == head)
			head = item;
	}
	else
	{
		item->append(tail);
		tail = item;
	} 
}

// remove a program whose children have all died
void
Process_list::delete_program(Program *ptr)
{
	Plink *item;

	if (!ptr || ptr->head)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (item = head; item && ptr != item->program(); item = item->next())
		;
	if (!item)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (item == head)
		head = head->next();
	if (item == tail)
		tail = tail->prev();
	item->unlink();
	delete item;
	delete ptr;
}

ProcObj *
Process_list::find_obj(DBcontext id)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && last_refd->get_id() == id)
		return last_refd;

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			Process *proc = p2->process();
#ifdef DEBUG_THREADS
			for (Plink *p3 = proc->get_head(); p3; p3 = p3->next())
			{
				if (p3->thread()->get_id() == id)
				{
					last_refd = p3->thread();
					return last_refd;
				}
			}
#endif
			if (proc->get_id() == id)
			{
				last_refd = proc;
				return last_refd;
			}
		}
	}
	return 0;
}

Process *
Process_list::find_process(pid_t pid)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && last_refd->get_process()->get_pid() == pid)
		return last_refd->get_process();

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			if (p2->process()->get_pid() == pid)
			{
				last_refd = p2->process();
				return p2->process();
			}
		}
	}
	return 0;
}

ProcObj *
Process_list::find_obj(const char *name)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && strcmp(last_refd->get_name(), name) == 0)
		return last_refd;

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
#ifdef DEBUG_THREADS
			for (Plink *p3 = p2->process()->get_head(); p3;
				p3 = p3->next())
			{
				if (strcmp(p3->thread()->get_name(), name) == 0)
				{
					last_refd = p3->thread();
					return last_refd;
				}
			}
#endif
			if (strcmp(p2->process()->get_name(), name) == 0)
			{
				last_refd = p2->process();
				return p2->process();
			}
		}
	}
	return 0;
}

ProcObj *
Process_list::find_obj(Message *m)
{
	char	*procname;
	char	*oldname;
	char	*file;
	Word	proc;
	Word	tmp;

	switch(m->get_msg_id())
	{
	case MSG_proc_fork:
#ifdef DEBUG_THREADS
	// "Thread %s has created new thread %s"
	case MSG_thread_create:
#endif
		m->unbundle(oldname, procname);
		return find_obj(oldname);

#ifdef DEBUG_THREADS
	// "Thread %s has been continued"
	// these are handled here specially because the message
	// has the context of the source thread (not the target)
	case MSG_thr_continue:
	case MSG_thr_continue_off_lwp:
		m->unbundle(oldname);
		return find_obj(oldname);

	// "Process %s (thread %s) forked; new process %s now controlled"
	case MSG_thread_fork:
		{
			char *threadname;
			m->unbundle(oldname, threadname, procname);
			return find_obj(threadname);
		}
#endif

	case MSG_proc_start:
	case MSG_proc_stop_fcall:
		m->unbundle(proc);
		return find_obj((DBcontext)proc);

	case MSG_set_frame:
		m->unbundle(proc, tmp);
		return find_obj((DBcontext)proc);

	case MSG_event_enabled:
	case MSG_event_disabled:
	case MSG_event_deleted:
		m->unbundle(tmp, proc);
		return find_obj((DBcontext)proc);

	case MSG_proc_killed:
	case MSG_release_run:
	case MSG_release_suspend:
	case MSG_release_core:
		m->unbundle(procname);
		return find_obj(procname);

	case MSG_jump:
		m->unbundle(proc, tmp, file, tmp);
		return find_obj((DBcontext)proc);

	default:
		return find_obj(m->get_dbcontext());
	}
}

Program *
Process_list::find_program(const char *name)
{
	Plink	*p1;

	if (last_refd && strcmp(last_refd->get_program()->progname, name) == 0)
		return last_refd->get_program();

	for (p1 = head; p1; p1 = p1->next())
	{
		int val = strcmp(p1->program()->progname, name);

		if (val == 0)
			return p1->program();
		else if (val > 0)
			// list is assumed in alphabetic order
			break;
	}
	return 0;
}

// Handle MSG_grab_proc and MSG_new_core from debug
void
Process_list::proc_grabbed(Message *m, Window_set *ws, Boolean first_process,
	int create_id)
{
	char	*procname = 0;
	char	*progname = 0;

	m->unbundle(progname, procname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		(m->get_msg_id()==MSG_new_core) ? State_core : State_stopped, ws, TRUE,
		first_process, create_id);
	// assuming MSG_grab_proc always followed by halted message

#if DEBUG
	dump();
#endif
}

#ifdef DEBUG_THREADS
// new thread from thr_create()
void
Process_list::new_thread(Message *m, Thread *creator)
{
	char		*tmp;
	char		*name;
	Process		*proc = creator->get_process();
	Window_set	*ws = creator->get_window_set();
	Thread		*thr;

	// create the new thread and add it to creator thread's
	// parent's thread list and its window set
	m->unbundle(tmp, name);
	thr = new Thread(m->get_dbcontext(), proc, name, ws);

	if (thr->get_state() == State_stopped)
	{
		// new bound thread - if action is not stop,
		// must restart
		unsigned char	ta = ws->get_thread_action();
		if (!(ta & TCA_stop))
		{
			// restart thread
			set_state(thr, State_running);
			dispatcher.send_msg(ws, thr->get_id(), 
				"run -b\n");
		}
	}
	// update creator which is stopped at the point of creation
	set_thread_state(creator, MSG_thread_create, ws);

#if DEBUG
	dump();
#endif
}

// live and core process creation is followed by 0 or more thread creations
// the thread is a result of the initial process creation
// since the process creation preceeds this, an incomplete
// thread should already exist. so, find the thread, and update its dbcontext
void
Process_list::proc_new_thread(Message *m)
{
	Word	id;
	char	*name;

	m->unbundle(id, name);
	Process *proc = find_process(name);
	if (!proc)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	Thread	*thr;
	if (!proc->has_threads())
	{
		if (proc->create_failed())
		{
			// fake one to catch event notification messages
			// prior to the kill message
			// Note: there can only be one of these
			thr = new Thread(proc, (thread_t)id, name, State_none, 0);
		}
		else
		{
			Window_set *ws = proc->get_window_set();

			thr = new Thread(m->get_dbcontext(), proc, name, ws);
			if (proc == ws->current_obj()->get_process())
				ws->set_current(thr);
			ws->delete_obj(proc);
			if (thr->get_state() == State_stopped)
			{
				// new bound thread - if action is not stop,
				// must restart
				unsigned char	ta = ws->get_thread_action();
				if (!(ta & TCA_stop))
				{
					// restart thread
					set_state(thr, State_running);
					dispatcher.send_msg(ws, thr->get_id(), 
						"run -b\n");
				}
			}
		}
	}
	else
	{
		// find thread
		Plink *p;
		for (p = proc->get_head(); p; p = p->next())
		{
			thr = p->thread();
			if (id == thr->get_tid())
				break;
		}
		if (!p)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}
	// set db_context
	thr->set_dbcontext(m->get_dbcontext());
}

Process *
Process_list::find_process(char *name)
{
	char *s = strchr(name, '.');
	if (!s)
		return 0;
	*s = '\0';
	Process *proc = (Process *)find_obj(name);
	*s = '.';
	return proc;
}
#endif

// Handle MSG_createp from debug
Process *
Process_list::new_proc(Message *m, Window_set *ws, Boolean first_process, int create_id)
{
	char	*procname = 0;
	char	*progname = 0;

	m->unbundle(progname, procname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		State_stopped, ws, TRUE, first_process, create_id, TRUE);
#if DEBUG
	dump();
#endif
	return (Process *)last_refd;
}

// Handle MSG_proc_fork or MSG_thread_fork from debug, create new process and
// update parent process (and its thread if thread called fork())
void
Process_list::proc_forked(Message *m, ProcObj *old)
{
	char		*procname = 0;
	char		*tmp;

	switch(m->get_msg_id())
	{
	case MSG_proc_fork:
		m->unbundle(tmp, procname);
		break;
#ifdef DEBUG_THREADS
	case MSG_thread_fork:
		m->unbundle(tmp, tmp, procname);
		break;
#endif
	}
	last_refd = new Process(m->get_dbcontext(),
		old->get_program()->get_name(), procname,
		State_stopped, old->window_set, FALSE, 0,
		old->get_program()->get_create_id());

	old->state = State_stopped;
	old->frame = -1;
	if (in_script || has_assoc_cmd)
		old->partial_update();
	else
	{
		old->set_location();
		old->update_done();
	}	
#if DEBUG
	dump();
#endif
}

// Handle MSG_proc_exec from debug:
// delete process from old program and add to new one
void
Process_list::proc_execed(Message *m, Process *old_proc)
{
	char		*progname = 0;
	char		*procname = 0;
	char		*tmp;
	int		is_current;

	is_current = (old_proc == old_proc->window_set->current_obj());

	m->unbundle(procname, tmp, progname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		State_stopped, old_proc->window_set, FALSE, is_current,
		old_proc->program->create_id);
	remove_obj(old_proc);
#if DEBUG
	dump();
#endif
}

// Handle message from debug that a process died or was released
// if there are other processes for this program, get rid of the Process
// if this is the last process for this program, get rid of both
void
Process_list::remove_obj(ProcObj *p)
{
#ifdef DEBUG_THREADS
	// if thread is the last one in process, then there
	// will be another proc_exit message following this
	// so, just delete this thread and let the proc_exit
	// handling take care of removing the process.
	if (p->is_thread())
		p->get_process()->delete_thread((Thread *)p);
	else
#endif
	{
		Program	*prog = p->get_program();

		prog->delete_process((Process *)p);
		if (!prog->head)
			delete_program(prog);
	}

	if (last_refd == p)
		last_refd = 0;
#if DEBUG
	dump();
#endif
}

#ifdef DEBUG_THREADS
// received an es_halted_off_lwp event notification
void
Process_list::proc_halted_off_lwp(Thread *p)
{
	// the message was MSG_es_halted_off_lwp
	// this assumes those messages are always followed
	// by location and source line (or disassembly)
	p->frame = -1;
	p->incomplete = TRUE;
	p->state = State_off_lwp;
	// a stopped thread can be in off lwp state, e.g. at the
	// time of a grab, a process can have some of the threads in 
	// off lwp state, and as a result of the grab, the es_halted_off_lwp
	// message is issued for one of these arbitrarily chosen
	// off lwp thread. 
}
#endif

// received an event notification
void
Process_list::proc_stopped(ProcObj *p)
{
	// the message was MSG_es_something
	// this assumes those messages are always followed
	// by location and source line (or disassembly)
	p->frame = -1;
	p->incomplete = TRUE;
	p->state = State_stopped;
}

// Handle the second message in an event notification, giving location
void
Process_list::update_location(Message *m, ProcObj *p)
{
	Msg_id		mtype = m->get_msg_id();
	char		*f1 = 0;
	char		*f2 = 0;

	delete p->file;
	delete p->function;
	delete p->location;
	p->file = p->function = p->location = 0;

	if (mtype == MSG_loc_sym_file)
	{
		m->unbundle(f1, f2);
		p->function = makestr(f1);
		p->file = makestr(f2);
	}
	else if (mtype == MSG_loc_sym)
	{
		m->unbundle(f1);
		p->function = makestr(f1);
	}
}

// Handle the third message in an event notification, giving full path of source file
void
Process_list::set_path(Message *m, ProcObj *p)
{
	char	*tmp = 0;

	m->unbundle(tmp);
	delete p->path;
	p->path = makestr(tmp);
}

// handle the fourth, and last, message in an event notification,
// and notify window sets of change in process state
// delay is set if the message is generated as a result of a create
// command - newly created processes are all handled at cmd_complete
void
Process_list::finish_update(Message *m, ProcObj *p, Boolean delay)
{
	Msg_id		mtype = m->get_msg_id();
	Word		line = 0;
	Word		addr = 0;
	char		*tmp;

	if (mtype == MSG_line_src)
		m->unbundle(line, tmp);
	else if (mtype == MSG_line_no_src)
		m->unbundle(line);
	else if (mtype == MSG_disassembly)
		m->unbundle(addr, tmp);
	else if (mtype == MSG_dis_line)
		m->unbundle(line, tmp, addr, tmp);
	else if (mtype == ERR_get_text)
		p->bad_state = TRUE;
	else
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);

	p->line = (long)line;
	if (p->file && line)
		p->location = makesf(strlen(p->file) + MAX_INT_DIGITS + 1,
					"%s@%d", p->file, p->line);
	else
		p->location = makesf(MAX_INT_DIGITS+2, "%#x", addr);

	if (in_script || has_assoc_cmd || delay)
		p->partial_update();
	else
		p->update_done();
}

void
Process_list::set_frame(Message *m, ProcObj *proc)
{
	Word		tmp;
	Word		frame;

	if (in_script || has_assoc_cmd)
	{
		proc->touched = TRUE;
		return;
	}

	m->unbundle(tmp, frame);
	proc->frame = (int)frame;
	delete proc->path;
	proc->path = 0;
	proc->get_frame_info();
	proc->window_set->set_frame(proc);
}

void
Process_list::proc_jumped(Message *m, ProcObj *proc)
{
	char	*file = 0;
	Word	tmp = 0;
	Word	addr = 0;
	Word	line = 0;

	m->unbundle(tmp, addr, file, line);
	proc->line = (long)line;
	delete proc->file;
	delete proc->location;
	delete proc->path;
	proc->file = proc->path = proc->location = 0;

	if (file && *file)
		proc->file = makestr(file);
	if (proc->file && line)
		proc->location = makesf(strlen(file) + MAX_INT_DIGITS + 1,
					"%s@%d", file, line);
	else
		proc->location = makesf(MAX_INT_DIGITS+2, "%#x", addr);
	if (in_script || has_assoc_cmd)
		proc->partial_update();
	else
		proc->update_done();
}

// this is called when a process is set in motion or when a process
// stops after being run to evaulate a function call in an expression
void
Process_list::set_state(ProcObj *ptr, ProcObj_state s, int delay_update)
{
	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// debug sends stepping message first - don't override with running message
	if (s == ptr->state || 
			(ptr->state == State_stepping && s == State_running))
		return;

	if (State_is_running(s))
	{
		ptr->state = s;
		ptr->bad_state = FALSE;
		delete ptr->function;
		delete ptr->file;
		delete ptr->location;
		delete ptr->path;
		ptr->frame = -1;
		ptr->function = ptr->file = ptr->path = 0;
		ptr->line = 0;
		ptr->location = 0;
		if (in_script || has_assoc_cmd)
			ptr->partial_update();
		else
			ptr->update_done();
	}
	else if (State_is_halted(s))
	{
		ptr->state = s;
		if (delay_update)
			ptr->touch();
		else if (in_script || has_assoc_cmd)
			ptr->partial_update();
		else
		{
			ptr->set_location();
			ptr->update_done();
		}
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

}

#ifdef DEBUG_THREADS
void
Process_list::set_thread_state(Thread *thr, Msg_id m, Window_set *ws)
{
	if (m == MSG_thr_continue_off_lwp)
	{
		// suspended thread being continued,
		// the thread is multiplexed and is off lwp
		set_state(thr, State_off_lwp);
		return;
	}

	// thread picks up LWP, new thread created,
	// thread continued on LWP or new thread reaches
	// its  "main" - if stop not indicated, restart

	unsigned char	ta = ws->get_thread_action();

	if (!(ta & TCA_stop))
	{
		// restart thread
		set_state(thr, State_running);
		dispatcher.send_msg(ws, thr->get_id(), "run -b\n");
		return;
	}

	if (m == MSG_es_halted_thread_start)
	{
		// thread reached its "main" routine;
		// since we are not restarting, treat this
		// like an ordinary breakpoint
		thr->frame = -1;
		thr->incomplete = TRUE;
		thr->state = State_stopped;
	}
	else
	{
		set_state(thr, State_stopped);
	}
}
#endif

void
Process_list::rename(Message *m)
{
	Program	*prog;
	Plink	*p;
	char	*old_name = 0;
	char	*new_name = 0;
	int	reordered = 0;
	int	val = -1;

	m->unbundle(old_name, new_name);
	for (p = head; p; p = p->next())
	{
		val = strcmp(p->program()->get_name(), old_name);
		if (val >= 0)
			break;
	}
	if (val != 0)
	{
		// not found?
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	prog = p->program();
	delete prog->progname;
	prog->progname = makestr(new_name);

	// if the renaming changes the order of items in the list,
	// re-insert into list
	if ((p->prev() && strcmp(p->prev()->program()->get_name(), new_name) > 0)
		|| (p->next() && strcmp(p->next()->program()->get_name(), new_name) < 0))
	{
		if (p == head)
			head = head->next();
		else if (p == tail)
			tail = tail->prev();
		p->unlink();
		delete p;
		add_program(prog);
		reordered = 1;
	}

	// update display since each process must now be relabelled with 
	// the new program name
	for (p = prog->head; p; p = p->next())
	{
		Process		*proc = p->process();
		Window_set	*ws = proc->window_set;
		int		is_current = (proc == ws->current_obj()->get_process());

		if (reordered)
		{
			ws->delete_process(proc);
			ws->add_process(proc, is_current);
		}
		else
			ws->rename_process(proc);
	}
#if DEBUG
	dump();
#endif
}

// move a list of procobjs from a window set to another 'ws'
void
Process_list::move_objs(ProcObj **plist, int pcnt, Window_set *ws)
{
	if (pcnt <= 0)
		return;

	Window_set *old_ws = (*plist)->window_set;
	for (int i = 0; i < pcnt-1; ++i, ++plist)
	{
		old_ws->delete_obj(*plist);
		(*plist)->window_set = ws;
		ws->add_obj(*plist, FALSE);
	}
	old_ws->delete_obj(*plist);
	(*plist)->window_set = ws;
	ws->add_obj(*plist, TRUE);
	if (!old_ws->current_obj())
		old_ws->set_current(0);
}

// returns 1 if there are any live processes in any window set
int
Process_list::any_live()
{
	for (Plink *prog = head; prog; prog = prog->next())
	{
		for (Plink *plink = prog->program()->head; plink; plink = plink->next())
		{	
			Process *proc = plink->process();
#ifdef DEBUG_THREADS
			if (proc->has_threads())
			{
				// a threaded process is live if
				// it contains a live thread
				for (Plink *tlink = proc->get_head(); 
						tlink; tlink = tlink->next())
					if (tlink->thread()->is_live())
						return 1;
				continue;
			}
#endif
			if (proc->is_live())
				return 1;
		}
	}
	return 0;
}

void
Process_list::clear_plist(unsigned char level)
{
	Plink	*p1;

	// reset for the next make_plist()
	switch(level)
	{
	case PROGRAM_LEVEL:
		for (p1 = head; p1; p1 = p1->next())
			p1->program()->set_used_flag(FALSE);
		break;
#ifdef DEBUG_THREADS
	case PROCESS_LEVEL:
		for (p1 = head; p1; p1 = p1->next())
		{
			for (Plink *p2 = p1->program()->get_head(); p2; p2 = p2->next())
				p2->process()->set_used_flag(FALSE);
		}
		break;
#endif
	}
}

#if DEBUG
// to avoid nullptr dereferences
static const char *
s_normalize(const char *s)
{
	static const char	*nulls = "";

	return s ? s : nulls;
}

void
Process_list::dump()
{
	Plink	*p1;
	Plink	*p2;
	Process	*proc;

	fprintf(stderr,"\nProcess list:\n");
	for (p1 = head; p1; p1 = p1->next())
	{
		fprintf(stderr,"%s\n", s_normalize(p1->program()->get_name()));
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			proc = p2->process();
			fprintf(stderr,"\t%s id=%#x, state=%d, function=%s, file=%s line=%d ws=%d\n",
				s_normalize(proc->get_name()), 
				proc->get_id(), proc->get_state(),
				s_normalize(proc->get_function()), 
				s_normalize(proc->get_file()),
				proc->get_line(),
				proc->get_window_set()->get_id());
#ifdef DEBUG_THREADS
			for (Plink *p3 = proc->get_head(); p3; p3 = p3->next())
			{
				Thread *thr = p3->thread();
				fprintf(stderr,"\t\t%s id=%#x, state=%d, function=%s, file=%s line=%d ws=%d\n",
					s_normalize(thr->get_name()), 
					thr->get_id(), thr->get_state(),
					s_normalize(thr->get_function()), 
					s_normalize(thr->get_file()),
					thr->get_line(),
					thr->get_window_set()->get_id());
			}
#endif
		}
	}
}
#endif

static Buffer buf;

// make_plist is called to make up a string of comma-separated PROCOBJ
// names for 2 purposes:
// 1) to be supplied as arg to some debug command, typically as parameter 
//    to -p (e.g. "stop -p ...")
// 2) to be displayed (e.g in the Process dialog)
// in the first case, there cannot be blanks separating process names,
// and in the latter case, the blanks makes for nicer display
// 'use_blanks' is used to distinguish the two cases.
const char *
make_plist(int total, ProcObj **p, int use_blanks, unsigned char level)
{
	proclist.clear_plist(level);

	// assume the first item in p always adds to buf
	buf.clear();
	for (int i = 0; i < total; i++)
	{
		if (level == PROGRAM_LEVEL)
		{
			if (!p[i]->get_program()->get_used_flag())
			{
				// "used" flag is to avoid duplicates
				p[i]->get_program()->set_used_flag(TRUE);
				if (i)
				{
					if (use_blanks)
						buf.add(", ");
					else
						buf.add(',');
				}
				buf.add(p[i]->get_program()->get_name());
			}
		}
#ifdef DEBUG_THREADS
		else if (level == PROCESS_LEVEL)
		{
			if (!p[i]->get_process()->get_used_flag())
			{
				// "used" flag is to avoid duplicates
				p[i]->get_process()->set_used_flag(TRUE);
				if (i)
				{
					if (use_blanks)
						buf.add(", ");
					else
						buf.add(',');
				}
				buf.add(p[i]->get_process()->get_name());
			}
		}
#endif
		else
		{
			if (i)
			{
				if (use_blanks)
					buf.add(", ");
				else
					buf.add(',');
			}
			buf.add(p[i]->get_name());
		}
	}
	return (char *)buf;
}

// update_all is called after running a script.
void
Process_list::update_all()
{
	Message	*m;
	Plink	*prog_link = head;
	Plink	*proc_link = 0;
	Process	*process;
	Plink	*thr_link = 0;

	if (head)
		proc_link = head->program()->get_head();
#ifdef DEBUG_THREADS
	if (proc_link)
		thr_link = proc_link->process()->get_head();
#endif
	dispatcher.query(0, 0, "ps\n");
	while ((m = dispatcher.get_response()) != 0)
	{
		ProcObj_state	state;
		char		*current = 0;
		char		*progname = 0;
		char		*procname = 0;
		char		*function = 0;
		char		*location = 0;
		char		*cmdline = 0;
		Word		pid = 0;
		Word		tid = 0;

		// current indicator, progname, procname, pid, func,
		//	 location, cmdline

		switch(m->get_msg_id())
		{
#ifdef DEBUG_THREADS
		case MSG_threads_ps_stopped:
			state = State_stopped;
			break;
		case MSG_threads_ps_stepping:
			state = State_stepping;
			break;
		case MSG_threads_ps_running:
			state = State_running;
			break;
		case MSG_threads_ps_core:
			state = State_core;
			break;
		case MSG_threads_ps_off_lwp:
			state = State_off_lwp;
			break;
		case MSG_threads_ps_core_off_lwp:
			state = State_core_off_lwp;
			break;
		case MSG_threads_ps_suspend:
			state = State_suspended;
			break;
		case MSG_threads_ps_core_suspend:
			state = State_core_suspended;
			break;
#else
		case MSG_ps_running:
			state = State_running;
			break;

		case MSG_ps_stepping:
			state = State_stepping;
			break;

		case MSG_ps_stopped:
			state = State_stopped;
			break;

		case MSG_ps_core:
			state = State_core;
			break;
#endif

		case MSG_ps_dead:
		case MSG_ps_release:
#ifdef DEBUG_THREADS
		case MSG_threads_ps_header:
		case ERR_reg_read_off_lwp:
#else
		case MSG_ps_header:
#endif
		case ERR_no_proc:
			continue;

#ifdef DEBUG_THREADS
		case MSG_threads_ps_unknown:
#else
		case MSG_ps_unknown:
#endif
		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		if (State_is_running(state))
			UNBUNDLE_R(m, current, progname, procname, pid, tid,
				cmdline);
		else
			UNBUNDLE(m, current, progname, procname, pid, tid,
				function, location, cmdline);

		// There may be processes and threads in the ps that are not yet in the
		// gui's proc list or vice versa - since debug is ahead 
		// of the gui, processes and threads may have died, forked, etc, and the
		// messages are still in the gui's message queue.  Ignore
		// those processes or threads for now, they will be brought up to date
		// as the messages are read.
		// Note that debug's proc list is assumed to be in alphabetic
		// order.

		int val;

		if ((int)tid > 0)
		{
#ifdef DEBUG_THREADS
			Thread	*thread;

			if (!thr_link || (thread = thr_link->thread()) == 0)
				continue;
			val = strcmp(thread->get_name(), procname);
			if (val > 0)
				continue;
			if (val == 0 && thread->touched)
				thread->update_location(function, location);
#endif
		}
		else
		{
			if (!proc_link || (process = proc_link->process()) == 0)
				continue;

			val = strcmp(process->get_name(), procname);
			if (val > 0)
				continue;

			if (val == 0 && process->touched)
			{
				process->update_location(function, location);
				Program *prog = process->get_program();
				if (!prog->cmd_line)
					prog->cmd_line = makestr(cmdline);
			}
		}

		if (thr_link && thr_link->next())
			thr_link = thr_link->next();
		else if (proc_link && proc_link->next())
		{
			proc_link = proc_link->next();
			thr_link = proc_link->process()->get_head();
		}
		else if (prog_link && prog_link->next())
		{
			prog_link = prog_link->next();
			proc_link = prog_link->program()->get_head();
			thr_link = proc_link->process()->get_head();
		}
		else
			prog_link = 0;
	}

	// wait until the query is done to tell the window set the process is
	// updated, since the window set may also need to do a query to get more
	// information
	for (prog_link = head; prog_link; prog_link = prog_link->next())
	{
		for (proc_link = prog_link->program()->head; proc_link;
			proc_link = proc_link->next())
		{
			process = proc_link->process();
			if (process->has_threads())
			{
#ifdef DEBUG_THREADS
				for (Plink *thr_link = process->get_head(); thr_link;
					thr_link = thr_link->next())
				{
					Thread	*thread = thr_link->thread();
					if (thread->touched)
					{
						thread->init_events();
						if (!thread->is_running())
							thread->get_frame_info();
						thread->update_done();
					}
				}
				if (process->touched)
					process->update_done();
#endif
			}
			else if (process->touched)
			{
				process->init_events();
				if (!process->is_running())
					process->get_frame_info();
				process->update_done();
			}
		}
	}
}

// kill all the programs/processes/threads resulting from the specified create command
void
Process_list::kill_all(int create_id, Window_set *ws)
{
	Plink	*p1;
	Program	*prog;
	Buffer	*buf = buf_pool.get();

	buf->clear();
	for (p1 = head; p1; p1 = p1->next())
	{
		prog = p1->program();
		if (prog->create_id != create_id)
			continue;

		if (buf->size() != 0)
			buf->add(',');
		buf->add(prog->get_name());
	}
	if (buf->size() != 0)
		dispatcher.send_msg(ws, 0, "kill -p%s\n", (char *)*buf);
	buf_pool.put(buf);
}

#ifdef DEBUG_THREADS
// given a list of ProcObjs, return a list of processes such that all of
// each process' child threads are listed in the original list.
// if 'vout' is null, then just return the number of such processes.
// 'match_single' says to match single-threaded processes as well as
// multi-threaded ones.
// this function is called, for example, to determine if the selected threads
// constitute all of the threads of the parent process.
int
get_full_procs(ProcObj **list, int nlist, Vector *vout, Boolean match_single)
{
	Vector		*v = vec_pool.get();
	int		nprocs = 0;
	Boolean		nmatched = 0;

	if (vout)
		vout->clear();
	v->clear();
	proclist.clear_plist(PROCESS_LEVEL);
	// make list of processes, only those that are parents of threads
	for (int i = 0; i < nlist; ++i)
	{
		if (list[i]->is_thread())
		{
			Process *proc = list[i]->get_process();
			if (!proc->get_used_flag() && 
			    (match_single || !proc->is_single_threaded()))
			{
				v->add(&proc, sizeof(Process *));
				proc->set_used_flag(TRUE);
				++nprocs;
			}
			list[i]->set_used_flag(TRUE);
		}
	}
	Process **procs = (Process **)v->ptr();
	// see if all threads of each process are marked
	// need to traverse the whole list in order to clear all
	// marks for threads.
	for (int j = 0; j < nprocs; ++j)
	{
		Boolean is_full = TRUE;

		for (Plink *plink = procs[j]->get_head(); 
				plink; plink = plink->next())
		{
			if (!plink->procobj()->get_used_flag())
				is_full = FALSE;
			else
				plink->procobj()->set_used_flag(FALSE);
		}
		if (is_full)
		{
			++nmatched;
			if (vout)
				vout->add(&procs[j], sizeof(Process *));
		}
	}
	vec_pool.put(v);
	return nmatched;
}
#endif

// called to update any processes that ran as a result of a function call
// in expression evaluation.  The update is delayed until the GUI receives
// a sync_request to avoid multiple updates of the same process, or
// unnecessary update of a dead process
void
Process_list::update_touched()
{
	for (Plink *prog = head; prog; prog = prog->next())
	{
		for (Plink *plink = prog->program()->head; plink; plink = plink->next())
		{	
			Process *proc = plink->process();
#ifdef DEBUG_THREADS
			if (proc->has_threads())
			{
				for (Plink *tlink = proc->get_head(); 
						tlink; tlink = tlink->next())
				{
					Thread *thr = tlink->thread();
					if (thr->was_touched())
					{
						if (thr->is_halted())
							thr->set_location();
						thr->update_done(RC_fcall_in_expr);
					}
				}
				continue;
			}
#endif
			if (proc->was_touched())
			{
				if (proc->is_halted())
					proc->set_location();
				proc->update_done(RC_fcall_in_expr);
			}
		}
	}
}
