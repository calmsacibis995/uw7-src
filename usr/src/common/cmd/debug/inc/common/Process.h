#ifndef Process_h
#define Process_h
#ident	"@(#)debugger:inc/common/Process.h	1.24"

#include "ProcObj.h"
#include "Machine.h"
#include "ProcFollow.h"
#include "Thread.h"
#include "Iaddr.h"
#include "Language.h"
#include <sys/types.h>
#include "sys/regset.h"

#ifdef DEBUG_THREADS

#include <thread.h>
#include <ucontext.h>
#else
typedef	long	lwpid_t;
#endif

class	Breakpoint;
class	Event;
class	EventTable;
class	Program;
class	Location;
class	Proclive;
struct	sig_ctl;

// definitions for create I/O redirection
#define	REDIR_IN	1
#define	REDIR_OUT	2
#define	REDIR_PTY	4

// definitions for proto process creation
#define P_KILL		0
#define P_RELEASE	1
#define P_EXEC		2

// Process control.
// Handles operations common to a single address space.
// Operations specific to a thread-of-control within
// the address space are handled by Threads.

class Process : public ProcObj {
	unsigned short	stop_all_cnt;
	unsigned short	next_thread; 
	pid_t		ppid;
	int		textfd;
	Location	*startloc;

	Thread		*head_thread;	// not really needed for non-
	Iaddr		thr_brk_addr;   // threads case, but it makes
					// some other code simpler
#ifdef DEBUG_THREADS
	Thread		*unused_list;
	Thread		*last_thread;
	Iaddr		thr_map_addr;
	Iaddr		thr_dbg_addr;	// address of _thr_debug
#endif

	EventTable 	*etable;
	Program		*pprogram;
	Event		*firstevent;  // first assigned for this object

	Language	lang;

	int		inform_startup(int what, int why);
	int		grab_fork(Process *, int procnum, pid_t, int syscall);
	int		control_new_prog(const char *);
	int		control_new_proc(pid_t, ProcObj *, int syscall);
	int		setup_process(int stop_in_init);
	int		setup_data(int use_obj);
	int		setup_name(const char *, int procnum, 
				int use_obj, time_t &, char *old = 0);

	int		lift_all_bkpt(Breakpoint *, int other_thread);
	int		insert_all_bkpt(Breakpoint *);
	void		remove_all_bkpt(Breakpoint *);

	int		use_et( EventTable * );
	int		re_init_et();
	int		setup_et_copy(Process *);
	int		copy_events(Process *);
	int		cleanup_et(int mode, int delete_events);

#if EXCEPTION_HANDLING
	int		eh_debug_setup(int is_core = 0);
#endif

#ifdef DEBUG_THREADS
	int		control_new_lwp(lwpid_t);
	void		mark_unused(Thread *);	// move to unused list
	int		thread_debug_setup();
	int		thread_setup_core();
	int		thread_setup_live(Process *old);
	int		get_thread_desc(Iaddr, struct thread_map &);
	int		get_lwp_id(struct thread_map *, lwpid_t *);
	Thread 		*thread_create(ProcObj *oldthread, Iaddr map);
	int		thread_make_virtual(Proclive *, int is_new);
	void		remove_all_threads();
	void		print_thread_list(int show, 
				Execstate pstate = es_invalid);
#endif

	int		make_proto(int);

	friend class	ProcObj;
	friend class	Thread;
public:
			Process();
			~Process();
	Thread		*first_thread(int virtual_ok);
					// first live thread
	//		Access functions
	Thread		*thread_list()	{ return(thr_brk_addr ?
				head_thread : 0); }
	Event		*event_list() { return firstevent; }
	pid_t		pid() { return ppid; }
	Program		*program() { return pprogram; }
	EventTable	*events() { return etable; }
	int		is_grabbed()	{ return(flags & L_GRABBED); }
	int		is_child()	{ return(flags & L_IS_CHILD); }
	int		is_stop_all()  	{ return(stop_all_cnt > 0); }
	Process		*next() { return (Process *)ProcObj::next(); }
	Process		*prev() { return (Process *)ProcObj::prev(); }
	Language	get_current_language()	{ return lang; }
	void		set_current_language(Language l) { lang = l; }
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
	void		set_check_follow()  { flags |= L_CHECK_FOLLOW; }
	int		check_follow();
#endif

	Process		*process();
	void		add_thread(Thread *);
	void		remove_thread(Thread *);
#ifdef DEBUG_THREADS
	Thread		*find_thread_addr(Iaddr);
	Thread		*find_thread_id(lwpid_t);
	void		clear_bkpt(Iaddr, Thread *);
#endif

	int		create(char *, int pnum, int in, int out, 
				int redir, int id, int on_exec, 
				int follow, Location *,
				const char *srcpath);
	int		grab(int pnum, char *path, char *loadfile,
				int id, int follow, const char *srcpath);
	int		grab_core(int tfd, int cfd, int pnum, 
				const char *ename, const char *srcpath);
	int		release_obj(int run);
	int		drop_process(int run);
	int		destroy(int sendsig);
	int		update_status();

	void		add_event(Event *);
	void		remove_event(Event *);

	int		cancel_sigs( sig_ctl *);
	int		pending_sigs( sig_ctl *);

	int		in_stack( Iaddr );
	Iaddr		end_stack();

	int		stop_all();
	int		restart_all();

	gregset_t	*read_greg();
	fpregset_t	*read_fpreg();
	int		write_greg(gregset_t *);
	int		write_fpreg(fpregset_t *);
};

#endif

// end of Process.h
