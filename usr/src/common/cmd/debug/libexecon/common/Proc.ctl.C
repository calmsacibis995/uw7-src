#ident	"@(#)debugger:libexecon/common/Proc.ctl.C	1.19"

#include "ProcObj.h"
#include "Process.h"
#include "ProcFollow.h"
#include "Interface.h"
#include "Machine.h"
#include "Procctl.h"
#include "Instr.h"
#include "global.h"
#include "List.h"
#include <errno.h>
#include <string.h>

// stop all threads and mark running threads as es_procstop
// we allow for nested calls to stop_all/restart_all - the inner
// calls do nothing
int
Process::stop_all()
{
	if ((stop_all_cnt++ > 0) ||
		state == es_corefile || state == es_dead)
		return 1;

	if (!thr_brk_addr)
	{
		// process level - if already stopped, do nothing
		if (!is_running() || stop_all_cnt > 0)
			return 1;
		flags &= ~L_CHECK;
		save_state = state;
		state = es_procstop;
		if (!pctl->stop())
		{
			printe(ERR_sys_cant_stop, E_ERROR, pobj_name);
			return 0;
		}
		update_state();
		return 1;
	}
	if (!pctl->stop())
	{
		// might have died already - if so, don't print
		// error message
		int	what, why;
		if (pctl->status(what, why) != p_dead)
			printe(ERR_sys_cant_stop, E_ERROR, pobj_name);
		return 0;
	}

#ifdef DEBUG_THREADS
	Thread	*thread;

	for(thread = head_thread; thread; thread = thread->next())
	{
		if (thread->state == es_stepping || 
			thread->state == es_running)
		{
			thread->flags &= ~L_CHECK;
			thread->save_state = thread->state;
			thread->state = es_procstop;
			thread->update_state();
			DPRINT(DBG_CTL, ("stop_all: %#x (%s), pc = %#x\n", thread, thread->obj_name(), thread->pc));

		}
	}
	return pctl->follower_disable();
#else
	return 1;
#endif
}

// restart all threads marked as es_procstop
int
Process::restart_all()
{
	if (stop_all_cnt == 0)
	{
		printe(ERR_internal, E_ERROR, "Process:restart_all",
			__LINE__);
		return 0;
	}
	stop_all_cnt--;
	if (stop_all_cnt > 0 || state == es_corefile)
		return 1;
	

	if (!thr_brk_addr)
	{
		// process level - if not stopped by a stop_all(), do nothing
		if (state != es_procstop)
			return 1;
		latestsig = 0;
		return restart(follow_yes);
	}

#ifdef DEBUG_THREADS
	Thread	*thread;
	int	found = 0;
	for(thread = head_thread; thread; thread = thread->next())
	{
		if (stop_all_cnt > 0)
			// a previous thread's restart resulted
			// in a stop all
			break;
		if (thread->state == es_procstop)
		{
			int what, why;
			int started = 1;
			thread->proc_ctl()->status(what, why);
			DPRINT(DBG_CTL, ("restart_all: %#x (%s) what = %d, why = %d\n", this, thread->obj_name(), what, why));
			if ((((thread->save_state == es_running) ||
				(thread->save_state == es_stepping)) &&
				why != STOP_REQUESTED) ||
				((started = thread->restart(follow_no)) != 0))
			{
				// If thread was running and
				// stop type is not STOP_REQUESTED,
				// do not restart.
				// The thread might have stopped on a 
				// breakpoint or system call before
				// the stop_all request.
				// Just add to follower - it should
				// return immediately and process
				// the stop normally.

				if (thread->state == es_procstop)
					// not restarted
					thread->state = thread->save_state;
				found = 1;
				thread->set_check();
				if (!thread->proc_ctl()->added_to_follower())
					thread->proc_ctl()->add_to_follower(follow_add_nostart);
			}
			else if (started == 0 && process()->get_state()
				== es_dead)
				// process died while in loop
				return 0;
		}
	}
	if (found)
	{
		if (!pctl->follower_enable())
			return 0;
#if !defined(FOLLOWER_PROC) && !defined(PTRACE)
		set_check_follow();
#endif
	}
#endif
	return 1;
}

int
Process::release_obj(int run)
{
	if (state == es_dead)
	{
		printe(ERR_invalid_op_dead, E_ERROR, pobj_name);
		return 0;
	}
#if PTRACE
	else if (flags & L_IS_CHILD)
	{
		printe(ERR_no_release_child, E_ERROR);
		return 0;
	}
#endif
	else if (state != es_corefile && pctl->stop() == 0 )
		return 0;

#ifdef DEBUG_THREADS
	for(Thread *thread = head_thread; thread;
		thread = thread->next())
		thread->mark_dead(P_RELEASE);
#endif

	if (state == es_corefile)
	{
		state = es_dead;
		printm(MSG_release_core, pobj_name);
		return 1;
	}

	if (make_proto(P_RELEASE) == 0 )
	{
		return 0;
	}

	if (!drop_process(run))
		return 0;

	if (run)
		printm(MSG_release_run, pobj_name);
	else
		printm(MSG_release_suspend, pobj_name);
	return 1;
}

// release process
int
Process::drop_process(int run)
{
	int	ret = 1;

	state = es_dead;

	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "Process::drop_run",
			__LINE__);
		return 0;
	}
#if PTRACE
	if (!run)
	{
		printe(ERR_no_release_suspend, E_WARNING);
		run = 1;
	}
#endif

	if ((pctl->release(run, ((flags & L_IS_CHILD)!= 0)) == 0) ||
		(pctl->cancel_fault() == 0))
		ret = 0;

	pctl->close();
	delete pctl;
	pctl = 0;

	return ret;
}

int
Process::destroy(int send_sig)
{
	flags &= ~L_CHECK;
	if (flags & L_WAITING)
	{
		flags &= ~L_WAITING;
		waitlist.remove(this);
	}

	if (state == es_dead)
		return 1;

	if (!core && send_sig && !pctl->stop())
		return 0;

	state = es_dead;

#ifdef DEBUG_THREADS
	for(Thread *thread = head_thread; thread; 
		thread = thread->next())
		thread->mark_dead(P_KILL);
#endif

	if (core)
		return 1;

	if (!make_proto(P_KILL))
		return 0;

	if (send_sig && !pctl->destroy((flags & L_IS_CHILD) != 0))
	{
		printe(ERR_sys_kill, E_ERROR, progname, strerror(errno));
		return 0;
	}

	pctl->close();
	delete pctl;
	pctl = 0;

	return 1;
}

#if !defined(FOLLOWER_PROC) && !defined(PTRACE)

int
Process::check_follow()
{
	if (!(flags & L_CHECK_FOLLOW))
		return 1;
	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "Process::check_follow",
			__LINE__);
		return 0;
	}
	flags &= ~L_CHECK_FOLLOW;
	if (!pctl->follower_running())
	{
		DPRINT(DBG_FOLLOW, ("Process:check_follow: %s start follower\n", pobj_name));
		return pctl->start_follower();
	}
	return 1;
}

#endif

