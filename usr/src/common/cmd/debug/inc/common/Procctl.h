#ifndef Procctl_h
#define Procctl_h
#ident	"@(#)debugger:inc/common/Procctl.h	1.21"

// Operating-system level control of processes and lwps
// Common interface handles old /proc, new /proc, ptrace.
// The mechanism for following multiple processes
// is hidden in the interface-specific pieces.
//
// The same interface is provided for live processes, core files
// and static object files.
// 
// The "read" function is virtual - reads from an object file, live process
// address space and core file data region all use the same interface.

#include <sys/types.h>
#include "Iaddr.h"
#include "Machine.h"
#include "Proctypes.h"
#include "ProcFollow.h"

enum	Procstat {
	p_unknown = 0,
	p_dead,
	p_core,
	p_stopped,
	p_running,
};

#ifdef DEBUG_THREADS
#define VIRTUAL	virtual
#else
#define VIRTUAL
#endif

class Lwplive;
class PtyInfo;

class Proclive {
protected:
	int		fd;
	void		*data;	     // used for both Procdata and Lwpdata
	ProcFollow	*follower;
	int		check_stat;  // only reread status if 
				     // state changes
	int		poll_index;
#ifdef DEBUG_THREADS
	Proclive	*parent;	// null for process, 
#endif
	int		default_traps();
public:
			Proclive();
			~Proclive();
	pid_t		create(const char *cmd, const char *ename,
				int redir, int input, int output,
				PtyInfo *);
	int		open(pid_t, int is_child);
	int		open(const char *path, pid_t, int is_child);
				// path might not be in local /proc
				// directory
	void		close();
	int		follow_children();
	const char	*psargs();
	int		stop();
	pid_t		proc_id();
#ifdef DEBUG_THREADS
	void		set_check() { check_stat = 1;
				if (parent) parent->check_stat = 1; }
	int		set_async();
	Lwplive		*open_lwp(lwpid_t);
	Lwplive		*open_all_lwp(const char *);
#else
	void		set_check() { check_stat = 1; }
#endif
	VIRTUAL Procstat status(int &why, int &what);
	VIRTUAL int	pending_sigs(sig_ctl *, int process_only);
	VIRTUAL int	current_sig(int &sig);
	VIRTUAL gregset_t *read_greg();
	VIRTUAL fpregset_t *read_fpreg();
	VIRTUAL dbregset_t *read_dbreg();
	VIRTUAL int	err_handle(int);
	Procstat	wait_for(int &why, int &what, 
				int allow_interrupt = 0);
	int		cancel_sig(int sig);
	int		cancel_current();
	int		kill(int sig);
	int		run(int sig, follower_mode);
	int		step(int sig, follower_mode);
#ifdef HAS_SYSCALL_TRACING
	int		sys_entry(sys_ctl *);
	int		sys_exit(sys_ctl *);
#endif
	int		trace_sigs(sig_ctl *);
	int		trace_traps(flt_ctl *);
	int		cancel_fault();
	int		update_stack(Iaddr &low, Iaddr &hi);
	int		open_object(Iaddr, const char *);
	int		write_greg(gregset_t *);
	int		write_fpreg(fpregset_t *);
	int		write_dbreg(dbregset_t *);
	int		read(Iaddr from, void *to, int len);
	int		write(Iaddr to, const void *from, int len);
	map_ctl		*seg_map(int &num);
	int		get_sig_disp(struct sigaction *&, int &number);
	int		release(int run, int is_child);
	int		destroy(int is_child);
	int		start_follower()
				{ return follower->start_follow(); }
#if defined(FOLLOWER_PROC) || defined(PTRACE)
	int		remove_from_follower() { return 0; }
#else
	int		remove_from_follower();
#endif
	int		added_to_follower() { return(poll_index >= 0); }
	void		add_to_follower(follower_mode mode)
				{ poll_index = follower->add(fd, mode); }
	int		follower_disable() { return follower->disable_all(); }
	int		follower_enable() { return follower->enable_all(); }
	int		follower_running() { return follower->running(); }
	ProcFollow	*get_follower() { return follower; }
};

#ifdef DEBUG_THREADS
class Lwplive : public Proclive {
	Lwplive		*_next;
	int		err_handle(int);
public:
			Lwplive();
			~Lwplive();
	Lwplive		*next() { return _next; }
	void		set_next(Lwplive *n) { _next = n; }
	lwpid_t		lwp_id();
	int		open(const char *path, lwpid_t, Proclive *);
	void		close();
	Procstat	status(int &why, int &what);
	int		current_sig(int &sig);
	int		pending_sigs(sig_ctl *, int process_only);
	gregset_t	*read_greg();
	fpregset_t	*read_fpreg();
	dbregset_t	*read_dbreg();
};
#endif

extern int		stop_self_on_exec();
extern int		release_child(pid_t);

extern int		live_proc(const char *path);

#endif
