#ifndef	_PROCLIST_H
#define _PROCLIST_H
#ident	"@(#)debugger:gui.d/common/Proclist.h	1.45"

#include <sys/types.h>		// for pid_t
#ifdef DEBUG_THREADS
#include <thread.h>		// for thread_t
#endif

// GUI headers
#include "UI.h"
#include "Windows.h"
#include "Reason.h"

// Debug headers
#include "Message.h"
#include "Link.h"
#include "Iaddr.h"

class Elink;
class Event;
class Program;
class Vector;
class Process_list;
class Sensitivity;

#define	PROGRAM_LEVEL	1
#define PROCESS_LEVEL	2
#define	THREAD_LEVEL	3

// the order of state enums matters!
// need to maintain the following predicates for State_is_*():
// 1. state > State_core => is_live
// 2. state >= State_stopped => is_halted
enum ProcObj_state
{
	State_none,
	State_core_off_lwp,
	State_core_suspended,
	State_core,
	State_running,
	State_stepping,
	State_stopped,
	State_off_lwp,
	State_suspended,
};

#define State_is_live(s)	((s) > State_core)
#ifdef DEBUG_THREADS
#define State_is_core(s)	((s) == State_core || \
				 (s) == State_core_off_lwp || \
				 (s) == State_core_suspended)
#else
#define State_is_core(s)	((s) == State_core)
#endif
#define State_is_halted(s)	((s) >= State_stopped)
#define State_is_runnable(s)	((s) == State_stopped)
#define State_is_running(s)	((s) == State_running || (s) == State_stepping)

enum ProcObj_type
{
	Type_unknown,
	Type_thread,
	Type_process,
};

class Process;

class ProcObj
{
protected:
	DBcontext	id;		// magic cookie identifies obj for debug
	ProcObj_type	type;		// obj type (thread or process)
	ProcObj_state	state;		// current state
	Window_set	*window_set;	// owner
	char		*name;		// name (p1, etc.)
	char		*function;	// current function, may be 0
	char		*file;		// current file name, may be 0
	char		*path;		// full path of file
	char		*location;	// character representation of location
	long		line;		// current line, may be 0
	int		frame;		// current frame number
	Boolean		incomplete;	// remainder of event notification (location)
					// has not been received yet
	Boolean		bad_state;	// error message already displayed
	Boolean		touched;	// state changed in script
#ifdef DEBUG_THREADS
	Boolean		used_flag;	// used by make_plist() & is_all_procs()
	unsigned int	animated;	// process/thread stepping under gui's control
					// threaded process may have several
					// child threads animated in different
					// window sets
#else
	Boolean		animated;	// process stepping under gui's control
#endif
	Elink		*ehead;		// list of pointers to event objects
	Elink		*etail;

	friend		Process_list;
	friend		Program;
public:
			ProcObj(DBcontext, const char *name,
				ProcObj_state, ProcObj_type,
				Window_set *, Boolean incomplete);
			~ProcObj();

			// access functions
	DBcontext	get_id()		{ return id; }
	ProcObj_type	get_type()		{ return type; }
	ProcObj_state	get_state()		{ return state; }
	Window_set	*get_window_set()	{ return window_set; }
	const char	*get_name()		{ return name; }
	const char	*get_function()		{ return function; }
	const char	*get_file()		{ return file; }
	const char	*get_path()		{ return path; }
	int		get_line()		{ return line; }
	int		get_frame()		{ return frame; }
	void		set_frame(int f)	{ frame = f; }
	const char	*get_location()		{ return location; }
	const char	*get_state_string();
	virtual Process	*get_process();
	virtual Program	*get_program();
	Boolean		is_live()		{ return State_is_live(state); }
	Boolean		is_core()		{ return State_is_core(state); }
	Boolean		is_halted()		{ return State_is_halted(state); }
	Boolean		is_running()		{ return State_is_running(state); }
	Boolean		is_runnable()		{ return State_is_runnable(state); }
#ifdef DEBUG_THREADS
	Boolean		is_thread()		{ return type == Type_thread; }
#else
	Boolean		is_thread()		{ return FALSE; }
#endif
	Boolean		is_incomplete()		{ return incomplete; }
#ifdef DEBUG_THREADS
	void		inc_animation()		{ ++animated; }
	void		dec_animation()		{ --animated; }
	virtual void	set_animation();
	virtual void	clear_animation();
	Boolean		is_animated()		{ return animated > 0; }
#else
	void		set_animation()		{ animated = TRUE; }
	void		clear_animation()	{ animated = FALSE; }
	Boolean		is_animated()		{ return animated; }
#endif
	void		set_location();
	Elink		*get_events()		{ return ehead; }
	void		touch()			{ touched = TRUE; }
	Boolean		was_touched()		{ return touched; }
	Boolean		in_bad_state()		{ return bad_state; }
#ifdef DEBUG_THREADS
	Boolean		get_used_flag()		{ return used_flag; }
	void		set_used_flag(Boolean v){ used_flag = v; }
#endif
	void		finish_update();


			// This function returns the number of signals.
			// Pointers to the strings are in the first vector.
			// The strings themselves are in the second vector.
	int		get_pending_signals(Order, Vector *vptr, Vector *vstr);

	void		get_frame_info();

			// functions to maintain event list
	void		add_event(Event *e);
	void		delete_event(Event *e);
	void		init_events();

			// get_break_list return the number of
			// breakpoints between the start and end addresses
			// or whose file and line no. match
			// it returns the list of breakpoint addresses in breaks
	int		get_break_list(Iaddr start, Iaddr end, Vector *breaks, Boolean get_ev_ids = FALSE, const char *file = 0, int line = 0);
			// get_event_list returns the number of Stop events
			// with breakpoints on address addr
			// or on file/line
			// it returns the list of event numbers in events
	int		get_event_list(Iaddr addr, Vector *events, const char *file = 0, int line = 0)
			{ return get_break_list(addr, addr, events, TRUE, file, line); }

			// get_objects and get_functions both return the number of
			// strings.  Pointers to the strings are returned in the vectors
	int		get_objects(Vector *);
	int		get_functions(Vector *, const char *file,
				const char *obj, const char *filter, int dashg);

	void		update_location(const char *function, const char *location);

				// all the messages from an event notification
				// have been processed, and the window set is
				// informed of the state change
	void		update_done(Reason_code = RC_change_state);
	void		partial_update(); // state changed while in a script

	int		check_sensitivity(Sensitivity *);
};

class Thread;

// Plink is used for linked lists of threads, processes or programs
class Plink : public Link
{
	void	*data;
public:
		Plink(void *p)	{ data = p; }
		~Plink()	{}

	Plink	*next()		{ return (Plink *)Link::next(); }
	Plink	*prev()		{ return (Plink *)Link::prev(); }
#ifdef DEBUG_THREADS
	Thread	*thread()	{ return (Thread *)data; }
#endif
	ProcObj	*procobj()	{ return (ProcObj *)data; }
	Process	*process()	{ return (Process *)data; }
	Program	*program()	{ return (Program *)data; }
};

#ifdef DEBUG_THREADS
class Thread :  public ProcObj
{
	Process		*proc;		// parent process
	thread_t	thrid;		// thread id
public:
			Thread(Process *, thread_t thrid, const char *name, ProcObj_state, Window_set *);
			Thread(DBcontext, Process *, const char *name, Window_set *);
			~Thread()
			    { if (window_set) window_set->delete_obj(this); }
	Program		*get_program();
	Process		*get_process();
	void		set_animation();
	void		clear_animation();
	thread_t	get_tid()		{ return thrid; }
	void		set_dbcontext(DBcontext nid);
	void		do_ps(Boolean update);
};
#endif

class Process : public ProcObj
{
	Program		*program;	// pointer to parent
	pid_t		pid;		// UNIX system process id
#ifdef DEBUG_THREADS
	Plink		*head;		// thread list head
	Plink		*tail;		// thread list tail
#endif
	Boolean		create_fail;	// flag create failure

	friend		Process_list;
	friend		Program;

public:
			Process(DBcontext id, const char *progname,
				const char *procname, ProcObj_state st,
				Window_set *ws, Boolean incomplete,
				Boolean make_current, int create_id,
				Boolean in_create = FALSE);
			~Process();
	Program		*get_program();
	Process		*get_process();
#ifdef DEBUG_THREADS
	void		add_thread(Thread *thr);
	void		delete_thread(Thread *thr);
	Boolean		has_threads()		{ return head != 0; }
	Boolean		is_single_threaded()	{ return head != 0 && head->next() == 0; }
	Plink		*get_head()		{ return head; }
	void		set_animation();
	void		clear_animation();
#else
	Boolean		has_threads()		{ return FALSE; }
	Plink		*get_head()		{ return 0; }
#endif
	Boolean		create_failed()		{ return create_fail; }
	pid_t		get_pid()		{ return pid; }

	void		finish_update(Boolean io_flag);
			// query debug for command line, etc.
	void		do_ps(char *&cmd, Boolean in_create);

};

class Program
{
	Plink		*head;		// list of processes
	Plink		*tail;
	char		*progname;	// program name
	char		*cmd_line;	// exec command line
	Boolean		io_redirected;
	Boolean		used_flag;	// flag used by make_plist()
#if EXCEPTION_HANDLING
	Boolean		uses_eh_flag;
#endif
	int		create_id;	// which create command produced this program

	friend		Process_list;
	friend		Process;
public:
			Program(const char *name, const char *cmd_line, int create_id);
			~Program() { delete cmd_line; delete progname; }

			// access functions;
	const char	*get_name()		{ return progname; }
	const char	*get_cmd_line()		{ return cmd_line; }
	Boolean		is_io_redirected()	{ return io_redirected; }
	Plink		*get_head()		{ return head; }
	Plink		*get_tail()		{ return tail; }
	Boolean		get_used_flag()		{ return used_flag; }
	void		set_used_flag(Boolean u)	{ used_flag = u; }
	int		get_create_id()		{ return create_id; }
#if EXCEPTION_HANDLING
	int		uses_eh()		{ return uses_eh_flag; }
	void		set_uses_eh()		{ uses_eh_flag = TRUE; }
#endif

			// functions to maintain process list
	void		add_process(Process *);
	void		delete_process(Process *);
};

// The process list maintains the state for all processes under the debugger's
// control.  There is only one master process list, but there are subsidiary
// lists of processes with each window set; those lists are driven by
// changes to the master list.

class Process_list
{
	Plink		*head;		// linked list of programs
	Plink		*tail;
	ProcObj		*last_refd;	// saved to help lookup performance

public:
			Process_list();
			~Process_list() {}

			// interfaces to Dispatcher; these drive process list updates
	Process		*new_proc(Message *, Window_set *, Boolean first_process,
				int create_id);
#ifdef DEBUG_THREADS
	void		proc_new_thread(Message *);
	void		new_thread(Message *, Thread *);
#endif
	void		proc_grabbed(Message *, Window_set *, Boolean first_process,
				int create_id);
	void		proc_forked(Message *, ProcObj *);
	void		proc_execed(Message *, Process *);
	void		remove_obj(ProcObj *);

	void		update_location(Message *, ProcObj *);
	void		set_path(Message *, ProcObj *);
	void		finish_update(Message *, ProcObj *, Boolean delay);
	void		proc_stopped(ProcObj *);
#ifdef DEBUG_THREADS
	void		proc_halted_off_lwp(Thread *);
#endif
	void		set_frame(Message *, ProcObj *);
	void		proc_jumped(Message *, ProcObj *);
	void		rename(Message *);
	void		update_all();
	void		update_touched();
	void		kill_all(int create_id, Window_set *ws);

			// interfaces for GUI objects to change state
	void		set_state(ProcObj *, ProcObj_state, int delay_update = 0);
#ifdef DEBUG_THREADS
	void		set_thread_state(Thread *, Msg_id, Window_set *);
#endif
	void		move_objs(ProcObj **, int, Window_set *);

			// find the given process or program
	ProcObj		*find_obj(DBcontext);
	ProcObj		*find_obj(const char *name);
	ProcObj		*find_obj(Message *);
	Process		*find_process(pid_t);
#ifdef DEBUG_THREADS
	Process		*find_process(char *name);
#endif
	Program		*find_program(const char *name);

	void		add_program(Program *);
	void		delete_program(Program *);

	int		any_live(); // search for any process in any window set
	void		clear_plist(unsigned char level);

#if DEBUG
	void		dump();
#endif
};

extern Process_list proclist;

#ifdef DEBUG_THREADS
extern const char	*make_plist(int total, ProcObj **, int use_blanks = 0,
				unsigned char level = THREAD_LEVEL);
extern int		get_full_procs(ProcObj **, int, Vector *v = 0, 
				Boolean match_single = TRUE);
#else
extern const char	*make_plist(int total, ProcObj **, int use_blanks = 0,
				unsigned char level = PROCESS_LEVEL);
#endif

#endif	// _PROCLIST_H
