#ifndef Thread_h
#define Thread_h
#ident	"@(#)debugger:inc/common/Thread.h	1.17"

// Thread control.
// Each Thread points to its containing process and is
// a member of the list of Threads for that process.
//
// Some operations happen at the Thread level.  Some are
// simply passed up to the process level.

#include "ProcObj.h"
#include "ProcFollow.h"
#include "Proctypes.h"
#include "Iaddr.h"
#include <sys/types.h>
#include <signal.h>
#include "sys/regset.h"

#ifdef DEBUG_THREADS
#include <thread.h>
#else
typedef	void *	thread_t;
#endif

class Process;

#if defined(OLD_PROC) || defined(PTRACE)
typedef long	lwpid_t;
#endif

// thread control modes
#define TCHANGE_IGNORE		0
#define TCHANGE_ANNOUNCE	1
#define TCHANGE_STOP		2

class Thread : public ProcObj {
	friend class	Process;
#ifdef DEBUG_THREADS
protected:
	Process		*parent;
	Iaddr		map_addr;
	struct thread_map	map;
	Thread		*old_thread; /*thread that gave up its LWP to us */
	sigset_t	cancel_set;

	int		cleanup_et();
	int		copy_et_create();
	int		copy_et_forkall(Process *);

	int		write_state();
	int		read_state();

public:
			Thread(int tnum,  Process *, struct thread_map *);
	void		grab_core(Proccore *, int suspended);
	int		grab_live(Proclive *, Iaddr map, Iaddr start,
				Process *old, int virtual_thread,
				int suspended );
			~Thread();
	thread_t	thread_id() { return map.thr_tid; }
	void		clear_pctl() { pctl = 0; }
	void		set_pctl(Proclive *p) { pctl = p; }
	Iaddr		get_addr() { return map_addr; }

	Process		*process();
	void		add_event(Event *);
	void		remove_event(Event *);
	int		set_consistent(follower_mode);
	void		set_inconsistent(Thread *);
	int		respond_to_threadpt(follower_mode);

	int		release_obj(int run);
	int		destroy(int);
	int		exit();
	void		mark_dead(int mode);

	int		cancel_sigs( sig_ctl *);
	int		pending_sigs( sig_ctl *);

	int		in_stack( Iaddr );
	Iaddr		end_stack();

	gregset_t	*read_greg();
	fpregset_t	*read_fpreg();
	int		write_greg(gregset_t *);
	int		write_fpreg(fpregset_t *);
	int		set_debug();	// set or clear state of 
	int		clear_debug();	// debug registers

	int		pick_up_lwp(Proclive *, Thread *, follower_mode);
	int		give_up_lwp(Thread *, follower_mode);

	void		thread_suspend_pending(int same_thread);
	int		thread_suspend();
	int		thread_continue(Thread *oldthread);

#else
	Thread		();
#endif
public:
	Thread		*next() { return (Thread *)ProcObj::next(); }
	Thread		*prev() { return (Thread *)ProcObj::prev(); }
};

#endif
