#ident	"@(#)debugger:libexecon/common/Thr.ctl.C	1.27"

#ifdef DEBUG_THREADS

#include "ProcObj.h"
#include "Breaklist.h"
#include "Thread.h"
#include "Process.h"
#include "Proccore.h"
#include "ProcFollow.h"
#include "Proglist.h"
#include "Interface.h"
#include "Manager.h"
#include "Procctl.h"
#include "Frame.h"
#include "global.h"
#include "List.h"

// Cannot just ignore thread since it will still be
// hitting the threadpt, and we must process.
int
Thread::release_obj(int run)
{
	if (state == es_dead)
	{
		printe(ERR_invalid_op_dead, E_ERROR, pobj_name);
		return 0;
	}
	if (is_core())
	{
		state = es_dead;
		printm(MSG_release_core, pobj_name);
	}
	else
	{
		if (pctl && stop() == 0 )
			return 0;
		if ( cleanup_et() == 0)
		{
			return 0;
		}
		remove(bk_hoppt);
		remove(bk_destpt);
		remove(bk_startpt);
		if (pctl && (restart(follow_yes) == 0))
			return 0;

		flags |= L_RELEASED;

		if (!run)
			printe(ERR_thread_release_suspended, 
				E_WARNING, pobj_name);
		printm(MSG_release_run, pobj_name);
	}
	if (!parent->first_thread(0))
		return parent->release_obj(run);
	return 1;
}

int
Thread::destroy(int)
{
	flags &= ~L_CHECK;
	if (flags & L_WAITING)
	{
		flags &= ~L_WAITING;
		waitlist.remove(this);
	}

	remove(bk_hoppt);
	remove(bk_destpt);
	remove(bk_startpt);

	if (is_user())
	{
		cleanup_et();
	}
	else
	{
		// idle thread whose LWP has just died
		if (pctl)
		{
			((Lwplive *)pctl)->close();
			delete (Lwplive *)pctl;
			pctl = 0;
		}
		DPRINT(DBG_THREAD, ("Thread::destroy : %#x idle thread has exited\n", this));
	}
	if (!parent->update_status())
	{
		// entire process has exited
		DPRINT(DBG_THREAD, ("Thread_destroy::entire process has exited\n"));
		return 1;
	}
	state = es_dead;
	return 1;
}

// user thread is about to exit
int 
Thread::exit()
{
	Proclive	*llive = pctl;
	int		ret = 1;

	pctl = 0;
	destroy(0);
	if (!(flags & L_RELEASED))
	{
		if (thr_change >= TCHANGE_ANNOUNCE)
			printm(MSG_thread_exit, pobj_name);
		if ((ProcObj *)this == proglist.current_object())
			proglist.reset_current(1);
	}
	if (parent->get_state() != es_dead)
	{
		ret = parent->thread_make_virtual(llive, 0);
		parent->remove_thread(this);
	}
	return ret;
}

void
Thread::mark_dead(int mode)
{
	
	// The entire process is being released or has exited.
	// If released, remove bkpts from address space and
	// table.  If exited, just from table.
	// set_remove marks bkpts as not inserted in address space,
	// so remove will not try to remove them from address space.

	if (flags & L_WAITING)
	{
		flags &= ~L_WAITING;
		waitlist.remove(this);
	}
	if (flags & L_CHECK)
	{
		if ((mode & P_EXEC) && pctl)
			clear_check();
		else
			flags &= ~L_CHECK;
	}
	if (state == es_dead)
	{
		return;
	}

	state = es_dead;

	if (core)
		return;
	
	if ((mode == P_RELEASE) && pctl)
	{
		pctl->cancel_fault();
	}

	if (mode != P_EXEC)
	{
		if (hoppt)
		{
			if (mode == P_KILL)
				hoppt->set_remove();
			remove(bk_hoppt);
		}
		if (destpt)
		{
			if (mode == P_KILL)
				destpt->set_remove();
			remove(bk_destpt);
		}
		if (startpt)
		{
			if (mode == P_KILL)
				startpt->set_remove();
			remove(bk_startpt);
		}
	}
	cleanup_foreign();
	if (pctl)
	{
		((Lwplive *)pctl)->close();
		delete((Lwplive *)pctl);
		pctl = 0;
	}
}

// user thread picks up an LWP, either from another
// user thead or from a virtual thread
int
Thread::pick_up_lwp(Proclive *llive, Thread *old_thread, 
	follower_mode mode)
{
	pctl = llive;
	state = es_breakpoint;  // stopped at threadpt
	latestbkpt = threadpt;
	update_state();
	set_inconsistent(old_thread);
	if (!set_debug())
		return 0;
	return(restart(mode));
}

// Thread gives up its lwp to another thread.
// There are 3 cases:
// 1. virtual thread gives up its lwp to user thread and becomes 
//       unused.
// 2. user thread gives up its lwp to virtual thread and becomes
//       es_off_lwp.
// 3. user thread gives up its lwp to another user thread and becomes
//       es_off_lwp.

int
Thread::give_up_lwp(Thread *target, follower_mode mode)
{
	Proclive	*llive;
	
	llive = pctl;
	pctl = 0;

	if (!(flags & L_VIRTUAL))
	{
		int	announce;
		announce = ((thr_change >= TCHANGE_ANNOUNCE) &&
			!(flags & L_RELEASED));
		state = es_off_lwp;
		if (flags & L_SUSP_PENDING)
		{
			flags &= ~L_SUSP_PENDING;
			flags |= L_SUSPENDED;
			if (announce)
				printm(MSG_thr_suspend, pobj_name);
		}
		else
		{
			if (announce)
				printm(MSG_give_up_lwp, pobj_name);
		}
		if (target == 0)
		{
			// we are giving up our lwp to a 
			// virtual thread (case 2)
			set_inconsistent(0);
			return parent->thread_make_virtual(llive, 0);
		}
		// otherwise - case 3 - we simply become off_lwp
	}
	else
	{
		// case 1 - virtual thread becoming unused
		// target could be 0 if this is a thread that
		// is exiting and is switching off; the debugger
		// has already switched the lwp to an idle thread,
		// so this could look like an idle thread giving
		// up its lwp to another idle thread.
		// In that case, just keep going.
		if (target == 0)
		{
			pctl = llive;
			return 1;
		}
		parent->mark_unused(this);
	}
	return target->pick_up_lwp(llive, this, mode);
}

// handle calls to thr_continue()
// as a special case, we get the continue message for
// newly created bound threads once the oldthread has
// executed _lwp_continue
int
Thread::thread_continue(Thread *oldthread)
{
	if (pctl)
	{
		if (thr_change >= TCHANGE_STOP)
		{
			if (!pctl->stop())
				return 0;
			state = es_halted;
			update_state();
			find_cur_src();
		}
	}
	if (flags & L_SUSP_PENDING) 
	{
		// pending suspension cancelled
		flags &= ~L_SUSP_PENDING;
	}
	else
	{
		if (thr_change >= TCHANGE_ANNOUNCE)
		{
			if ((flags & L_NEW_THREAD) &&
				!(flags & L_SUSPENDED))
			{
				// new bound thread
				message_manager->reset_context((ProcObj *)this);
				printm(MSG_thread_create, 
					oldthread->obj_name(), pobj_name);
				message_manager->reset_context(0);
			}
			else
			{
				// normal continue
				if (state == es_off_lwp)
					printm(MSG_thr_continue_off_lwp,
						pobj_name);
				else
					printm(MSG_thr_continue, pobj_name);
			}
		}
	}
	flags &= ~L_NEW_THREAD;
	flags &= ~L_SUSPENDED;
	return 1;
}

// if same is non-zero, thread is suspending itself
void
Thread::thread_suspend_pending(int same_thread)
{
	// thread suspending itself or being suspended
	// in either case, we will let the thread being suspended
	// run; either it switches off its lwp,
	// or waits on a condition variable (bound threads).
	if (same_thread)
		state = es_breakpoint;  // stopped at notifier
	flags |= L_SUSP_PENDING;
}

int
Thread::thread_suspend()
{
	// bound thread completing suspension of itself
	if (!(flags & L_SUSP_PENDING))
	{
		printe(ERR_libthreads, E_ERROR, pobj_name);
		return 0;
	}
	flags &= ~L_SUSP_PENDING;
	flags |= L_SUSPENDED;
	state = es_breakpoint; 	// stopped at notifier
	if ((thr_change >= TCHANGE_ANNOUNCE) && !(flags & L_RELEASED))
		printm(MSG_thr_suspend, pobj_name);
	// remove from wait list, even though we will be restarting,
	// since we are "virtually stopped," i.e. suspended

	if (waiting())
	{
		waitlist.remove(this);
		clear_wait();
	}
	return 1;
}

int
Thread::set_consistent(follower_mode mode)
{
	flags &= ~L_INCONSISTENT;
	if (old_thread)
	{
		old_thread->flags &= ~L_INCONSISTENT;
		old_thread->update_state();
		old_thread->find_cur_src();
		old_thread = 0;
	}
	if (is_user() && startpt == 0)
	{
		if (thr_change >= TCHANGE_ANNOUNCE)
		{
			message_manager->reset_context(this);
			printm(MSG_pick_up_lwp, pobj_name);
			message_manager->reset_context(0);
		}
		if (thr_change >= TCHANGE_STOP)
		{
			update_state();
			find_cur_src();
			return 1;
		}
	}
	return restart(mode);
}

void
Thread::set_inconsistent(Thread *thread)
{
	flags |= L_INCONSISTENT;
	regaccess.update();
	if (thread)
	{
		old_thread = thread;
		old_thread->set_inconsistent(0);
	}
	else
		old_thread = 0;
}

// monitor thread state changes
int
Thread::respond_to_threadpt(follower_mode mode)
{
	Iaddr			map_ptr;
	enum thread_change	tchange;
	Frame			*frame = topframe();
	Thread			*target;
	Thread			*nthread;
	int			ret = 1;


	// Stopped at threads library notifier function;
	// read arguments to function.
	// Arguments are pointer to affected thread_map
	// and type of change.

	map_ptr = (Iaddr)frame->quick_argword(0);
	tchange = (enum thread_change)frame->quick_argword(1);

	DPRINT(DBG_THREAD, ("Notifier: thread %#x (%s), map_ptr %#x, thread_change %d\n", this, pobj_name, map_ptr, (int)tchange));

	// if map_ptr is non-0, it is the address of a thread
	// descriptor; if the action is thr_create, this will
	// be a thread we don't yet know about, otherwise, it
	// should already be on our thread list.
	if (map_ptr != 0)
	{
		map_ptr = (Iaddr)THR_MAP(map_ptr);
		if (tchange != tc_thread_create)
		{
			if ((target = parent->find_thread_addr(map_ptr))
				== 0)
			{
				printe(ERR_libthreads, E_ERROR, 
					pobj_name);
				return 0;
			}
		}
	}
	else if (tchange != tc_switch_complete && tchange !=
		tc_switch_begin)
	{
		printe(ERR_libthreads, E_ERROR, pobj_name);
		return 0;
	}
	else
	{
		target = 0;
	}
	switch(tchange)
	{
	case tc_thread_create:
		if ((nthread = parent->thread_create(this, map_ptr))
			== 0)
			return 0;
		if (thr_change < TCHANGE_STOP || 
			((nthread->flags & L_NEW_THREAD) &&
			!(nthread->flags & L_SUSPENDED)))
			// restart for new bound threads
			break;
		check_watchpoints();
		find_cur_src();
		return 1;
	case tc_thread_exit:
		// current thread is exiting
		return(exit());
	case tc_switch_begin:
		// thread drops lwp - if map_ptr is NULL,
		// we are simply being switched out.
		// If it points to a different thread, we
		// are giving up our lwp to that thread.
		if (!give_up_lwp(target, mode))
			return 0;
		if (is_virtual() && !target)
			break;
		find_cur_src();
		return 1;
	case tc_switch_complete:
		return set_consistent(mode);
	case tc_cancel_complete:
		ret = target->cancel_sigs(0);
		break;
	case tc_thread_continue:
	{
		int new_bound = (target->flags & L_NEW_THREAD) != 0;
		ret = target->thread_continue(this);
		if (thr_change < TCHANGE_STOP)
		{
			if (new_bound)
				target->restart(mode);
			break;
		}
		if (!target->pctl || !target->startpt)
			break;
		// don't restart original thread
		// for bound threads, this is where we
		// inform user about new threads
		check_watchpoints();
		find_cur_src();
		return ret;
	}
	case tc_thread_suspend_pending:
		target->thread_suspend_pending(target == this);
		break;
	case tc_thread_suspend:
		target->thread_suspend();
		break;
	case tc_invalid:
	default:
		printe(ERR_libthreads, E_ERROR, pobj_name);
		return 0;
	}

	if (!ret)
		return 0;
	return restart(mode);
}

#endif
