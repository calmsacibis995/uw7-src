#ident	"@(#)debugger:libexecon/i386/ProcFollow.C	1.19"

#include "ProcFollow.h"
#include "Interface.h"
#include "utility.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <unistd.h>
#include <signal.h>

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)

#include "global.h"
#include "NewHandle.h"
#include <thread.h>
#include <synch.h>
#include <stropts.h>
#include <poll.h>

#define GROW_SIZE	20

static void	*follow(void *);
extern "C"	thread_key_t	thrkey;

ProcFollow::ProcFollow()
{
	ready = used = total = last = firstfree = 0;
	poll_array = poll_end = 0;
	follower_running = 0;
	follower = (thread_t)-1;
	debugger = (thread_t)-1;
}

ProcFollow::~ProcFollow()
{

	DPRINT(DBG_FOLLOW, ("ProcFollow::~ProcFollow, follower == %d\n", follower));
	if (follower != (thread_t)-1)
	{
		thread_t	save_id = follower;
		// wake up follower and tell it to exit
		ready = 1;
		used = 1;
		follower = (thread_t)-1;
		thr_kill(save_id, SIGUSR2);
	}
	cond_destroy(&this->cond);
	mutex_destroy(&this->mutex);
	barrier_destroy(&this->barrier);
	if (poll_array)
		free(poll_array);
}

int
ProcFollow::initialize(pid_t pid)
{
	procid = pid;
	debugger = thr_self();
	if ((mutex_init(&this->mutex, USYNC_THREAD, 0) != 0) ||
		(cond_init(&this->cond, USYNC_THREAD, 0) != 0) ||
		(barrier_init(&this->barrier, 2, USYNC_THREAD, 0) != 0))
	{
		printe(ERR_follower_thread, E_ERROR);
		return 0;
	}
	// start follower as a bound thread
	if (thr_create(0, 0, follow, this, THR_BOUND|THR_DETACHED,
		(thread_t *)&this->follower) != 0)
	{
		printe(ERR_follower_thread, E_ERROR);
		return 0;
	}
	grow();
	DPRINT(DBG_FOLLOW, ("new follower thread %d\n", this->follower));
	// wait until follower is initiallized and has blocked
	// all signals
	barrier_wait(&this->barrier);
	return 1;
}

// Add a file descriptor to the poll list; if mode is follow_yes
// also start follower thread.  If mode is follow_add_nostart
// we assume follower is already stopped before adding and we
// do not restart.
// Returns index of poll entry for
// this file descriptor; -1 for failure.
//
// We keep track of highest number entry used in last
//
int
ProcFollow::add(int nfd, follower_mode mode)
{
	int		save_ndx;
	struct pollfd	*ptr;

	DPRINT(DBG_FOLLOW, ("ProcFollow::add follower %d, file %d mode %d\n", follower, nfd, mode));
	if (follower == (thread_t)-1)
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::add", 
			__LINE__);
		return -1;
	}
	ready = 0;
	if (mode != follow_add_nostart)
		// wake up follower so it releases the lock, 
		// then grab it
		thr_kill(follower, SIGUSR2);
	if (mutex_lock(&this->mutex) != 0)
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::add", 
			__LINE__);
		return -1;
	}
	
	used++;
	if (used >= total)
		// always keep 1 more than we need
		grow();

	save_ndx = firstfree;
	poll_array[firstfree].fd = nfd;
	poll_array[firstfree].events = POLLWRNORM;
	poll_array[firstfree].revents = 0;

	// find next free entry
	// we always look from firstfree to make sure
	// list is as compact as possible (to avoid copying
	// in and out of kernel)
	for(ptr = &poll_array[firstfree+1]; ptr <= poll_end; ptr++)
	{
		if (ptr->fd == -1)
		{
			firstfree = (ptr - poll_array);
			break;
		}
	}
	if (ptr > poll_end)
	{
		mutex_unlock(&this->mutex);
		printe(ERR_internal, E_ERROR, "ProcFollow::add", 
			__LINE__);
		return -1;
	}
	if (save_ndx > last)
		last = save_ndx;
	if (mode == follow_yes)
	{
		// tell follower to keep going
		ready = 1;
		DPRINT(DBG_FOLLOW, ("ProcFollow::add follower %d, calling cond_signal\n", follower));
		cond_signal(&this->cond);
	}
	mutex_unlock(&this->mutex);
	return save_ndx;
}

// invalidate all poll entries by setting the pollfd events field
// to 0 - assumes follower is not actively polling
int 
ProcFollow::disable_all()
{

	struct pollfd	*ptr;

	DPRINT(DBG_FOLLOW, ("ProcFollow::disable_all follower %d\n", follower));
	if ((follower == (thread_t)-1) ||
		(mutex_lock(&this->mutex) != 0))
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::disable_all", 
			__LINE__);
		return 0;
	}
	ptr = &poll_array[0];
	for (int i = 0; i <= last; ptr++, i++)
	{
		ptr->events = 0;
	}
	mutex_unlock(&this->mutex);
	return 1;
}

// resinstate all poll entries by setting the pollfd events field
// to POLLWRNORM, but do not start follower
// assumes follower is not either polling with events disabled
// or sigwaiting or cond_wating
int 
ProcFollow::enable_all()
{
	struct pollfd	*ptr;

	DPRINT(DBG_FOLLOW, ("ProcFollow::enable_all follower %d, used = %d \n", follower, used));
	ready = 0;
	if (follower == (thread_t)-1)
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::disable_all", 
			__LINE__);
		return 0;
	}
	thr_kill(follower, SIGUSR2);
	if (mutex_lock(&this->mutex) != 0)
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::disable_all", 
			__LINE__);
		return 0;
	}
	ptr = &poll_array[0];
	for (int i = 0; i <= last; ptr++, i++)
	{
		if (ptr->fd >= 0)
			ptr->events = POLLWRNORM;
	}
	mutex_unlock(&this->mutex);
	return 1;
}

// remove a file descriptor from list; we assume follower has
// relinquished lock and is waiting for notification to restart
int
ProcFollow::remove(int index)
{
	DPRINT(DBG_FOLLOW, ("ProcFollow::remove follower %d, file %d\n",follower, poll_array[index].fd));
	if (index >= total || index < 0)
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::remove", 
			__LINE__);
		return 0;
	}
	if ((follower == (thread_t)-1) ||
		(mutex_lock(&this->mutex) != 0))
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::remove", 
			__LINE__);
		return 0;
	}
	poll_array[index].fd = -1;
	used--;
	if (used == 0)
	{
		firstfree = last = 0;
	}
	else
	{
		if (index == last)
			last--;
		if (index < firstfree)
			firstfree = index;
	}
	mutex_unlock(&this->mutex);
	return 1;
}

void
ProcFollow::grow()
{
	struct pollfd	*ptr;

	total += GROW_SIZE;
	if (!poll_array)
	{
		poll_array = (struct pollfd *)malloc(total * 
			sizeof(struct pollfd));
	}
	else
	{
		poll_array = (struct pollfd *)realloc(poll_array,
			total * sizeof(struct pollfd));
	}
	if (!poll_array)
		newhandler.invoke_handler();

	poll_end = &poll_array[total-1];
	for(ptr = &poll_array[used]; ptr <= poll_end; ptr++)
		ptr->fd = -1;
}

// assume follower is condition waiting or sigwaiting
int
ProcFollow::start_follow()
{
	DPRINT(DBG_FOLLOW, ("ProcFollow::start_follow follower %d\n", follower));
	ready = 0;
	if ((follower == (thread_t)-1) ||
		(mutex_lock(&this->mutex) != 0))
	{
		printe(ERR_internal, E_ERROR, "ProcFollow::start_follow",
			__LINE__);
		return 0;
	}
	thr_kill(follower, SIGUSR2);
	ready = 1;
	cond_signal(&this->cond);
	mutex_unlock(&this->mutex);
	return 1;
}

// check for live process; if dead, inform debugger and exit
void
check_status(pid_t pid, thread_t debugger, void *status)
{
	if (kill(pid, 0) == -1)
	{
		// process has died - exit
		DPRINT(DBG_FOLLOW, ("follower - thread %d, exiting\n", thr_self()));
		thr_kill(debugger, SIGUSR1);
		thr_exit(status);
	}
}

// follower:
// We synchronize with the debugger around the poll file 
// descriptor structure.
// We block all signals but SIGUSR1 and then wait until 
// the debugger signals us.  While waiting, we do not hold the mutex.
// When we are awakened, we grab the mutex and do a condition wait,
// until the debugger has set up the poll structure correctly.
// We then call poll, holding the lock acquired by the condition wait.
// If poll returns successfully, we release the lock,
// signal the debugger
// and suspend again, waiting for SIGUSR2.  If poll is interrupted by
// the debugger sending us a signal, we still hold the lock, and we
// go into our condition wait loop.

static void *
follow(void *pf)
{
	int			*status;
	pid_t			procid;
	thread_t		debugger;
	sigset_t		smask;
	sigset_t		wmask;
	ProcFollow		*procfollow = (ProcFollow *)pf;
	int			sig_received = 0;

	// block all signals
	prfillset(&smask);
	thr_sigsetmask(SIG_SETMASK, &smask, 0);

	status = procfollow->get_status_addr();
	*status = 0;
	procid = procfollow->get_procid();
	debugger = procfollow->debug_id();

	premptyset(&wmask);
	praddset(&wmask, SIGUSR2);
	thr_setspecific(thrkey, &sig_received);

	barrier_wait(procfollow->get_barrier());

	while(1)
	{
		int	sig;

		// wait until we receive sigusr2 from the debugger

		DPRINT(DBG_FOLLOW, ("follower - thread %d, about to sigwait\n", thr_self()));
		do {
			sig = sigwait(&wmask);
		} while(sig == SIGWAITING);

		// on return from sigwait, all signals are blocked

		DPRINT(DBG_FOLLOW, ("follower - thread %d, out of sigwait\n", thr_self()));
		if (procfollow->follow_id() == (thread_t)-1)
		{
			// debugger wants us to exit
			DPRINT(DBG_FOLLOW, ("follower - thread %d, exiting\n", thr_self()));
			return(status);
		}
		check_status(procid, debugger, status);
		if (mutex_lock(procfollow->lock()) != 0)
		{
			// assume proces has died
			*status = 1;
			return(status);
		}
		// we hold the lock until a successful poll completes
		while(1)
		{
			// cond_wait releases the lock until the condition
			// is signalled, then grabs it
			int ret;
			while(!procfollow->is_ready() ||
				procfollow->is_empty())
			{
				DPRINT(DBG_FOLLOW, ("follower - thread %d, about to cond_wait\n", thr_self()));
				cond_wait(procfollow->condition(),
					procfollow->lock());
			}
			DPRINT(DBG_FOLLOW, ("follower - thread %d, about to poll\n", thr_self()));
			// allow SIGUSR2 to interrupt the poll
			errno = 0;
			procfollow->set_running();
			sig_received = 0;
			thr_sigsetmask(SIG_UNBLOCK, &wmask, 0);
			if (sig_received != 0)
			{
				// already signalled by main thread
				thr_sigsetmask(SIG_SETMASK, &smask, 0);
				sig_received = 0;
				procfollow->clear_running();
				DPRINT(DBG_FOLLOW, ("follower - thread %d, poll interrupted\n", thr_self()));
				continue;
			}
			ret = poll(procfollow->get_list(), 
				procfollow->entries(), INFTIM);
			// block all signals again
			thr_sigsetmask(SIG_SETMASK, &smask, 0);
			procfollow->clear_running();
			if ((ret >= 0) || (errno != EINTR))
			{
				// on interrupt just return to cond
				// wait loop
				DPRINT(DBG_FOLLOW, ("follower - thread %d, poll completes, errno = %d\n", thr_self(), errno));
				mutex_unlock(procfollow->lock());
				if (errno != 0)
					// error check for live proc
					check_status(procid, 
						debugger, status);
				thr_kill(debugger, SIGUSR1);
				break;  // return to sigwait loop
			}
			DPRINT(DBG_FOLLOW, ("follower - thread %d, poll interrupted\n", thr_self()));
		}
	}
}

#else
#include <stdio.h>
#include "Machine.h"
#include "global.h"

// Non-poll version - minimal per-process follower
			
ProcFollow::~ProcFollow()
{
	if (followpid > 0)
		kill(followpid, SIGUSR2);

}

int
ProcFollow::start_follow()
{
	if (followpid > 0)
	{
		return (kill(followpid, SIGUSR1) == 0);
	}
	else
		return 0;
}

// create follower process
// usage is: follow /proc/subject_pid debugger_pid
int
ProcFollow::initialize(char *filename)
{
	char debugger[MAX_INT_DIGITS+1];

	sprintf(debugger, "%d", getpid());

	if ((followpid = fork()) == 0) 
	{	// child
		execl(follow_path, "follow", filename, debugger, 0);
		printe(ERR_no_follower, E_ERROR, follow_path, strerror(errno));
		_exit(1);	// use _exit to avoid flushing buffers
	}
	else if (followpid == -1)
	{
		// fork failed
		printe(ERR_no_follower, E_ERROR, follow_path, strerror(errno));
		return 0;
	}
	// test for exec failed
	if (kill(followpid, 0) == -1)
		return 0;
	return 1;
}

// null versions for non-Poll follower
int ProcFollow::add(int, follower_mode)
{
	return 0;
}

int ProcFollow::remove(int)
{
	return 0;
}

int ProcFollow::enable_all()
{
	return 0;
}

int ProcFollow::disable_all()
{
	return 0;
}

#endif // old /proc
