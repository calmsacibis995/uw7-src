#ifndef _ProcFollow_h
#define _ProcFollow_h
#ident	"@(#)debugger:inc/i386/ProcFollow.h	1.8"

// parameter to start
enum follower_mode {
	follow_no,
	follow_yes,
	follow_add_nostart,
};

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)

#include <thread.h>
#include <synch.h>
#include <sys/types.h>

struct pollfd;

// data structures that maintain poll list for follower

class ProcFollow {
	int		used;		// number of active entries
	int		total;		// total entries
	int		last;		// highest number allocated
	int		firstfree;	// first free entry on list
	int		ready;
	pid_t		procid;		// process's pid
	int		follower_running;
	int		exit_status;	// set by follower
	struct pollfd	*poll_array;
	struct pollfd	*poll_end;
	thread_t	debugger;
	thread_t	follower;
	mutex_t		mutex;
	cond_t		cond;
	barrier_t	barrier;
	void		grow();
public:
			ProcFollow();
			~ProcFollow();
	int		initialize(pid_t);	// create follower
	int		add(int fd, follower_mode); // adds an entry to watch - 
					// returns its index in table
					// if follower_mode == follow_yes 
					// also starts follower
	int		remove(int index);
					// removes an entry given its index
	int		disable_all();
	int		enable_all();
	int		start_follow();
	//		Access functions
	int		is_empty() { return (used == 0); }
	int		is_ready() { return ready; }
	int		entries() { return last + 1; }
	int		get_procid()  { return procid; }
	struct pollfd	*get_list() { return poll_array; }
	mutex_t		*lock() { return &this->mutex; }
	cond_t		*condition() { return &this->cond; }
	barrier_t	*get_barrier() { return &this->barrier; }
	thread_t	debug_id() { return debugger; }
	thread_t	follow_id() { return follower; }
	void		set_follower(thread_t f) { follower = f; }
	void		clear_running() { follower_running = 0; }
	void		set_running() { follower_running = 1; }
	int		running() { return follower_running; }
	int		*get_status_addr() { return &this->exit_status; }
};

#else
// old /proc version

#include <sys/types.h>

class ProcFollow {
	pid_t	followpid;
public:
		ProcFollow() { followpid = -1; }
		~ProcFollow();
	int	initialize(char *path);
	int	start_follow();
	int	add(int fd, follower_mode);// these are null for old /proc
	int	remove(int index);
	int	enable_all();
	int	disable_all();
	int	running() { return 0; }
};

#endif

#endif
