#ident	"@(#)debugger:libexecon/common/Thr.event.C	1.16"

#ifdef DEBUG_THREADS

#include "ProcObj.h"
#include "Process.h"
#include "Thread.h"
#include "EventTable.h"
#include "Event.h"
#include "Siglist.h"
#include "Watchlist.h"
#include "Procctl.h"
#include "Proglist.h" 
#include "Interface.h"
#include "Machine.h"
#include "Proctypes.h"
#include <string.h>
#include <signal.h>
#include <sys/types.h>

void
Thread::add_event(Event *event)
{
	parent->add_event(event);
}

void
Thread::remove_event(Event *event)
{
	parent->remove_event(event);
}

int
Thread::set_debug()
{
	if (!hw_watch)
		return 1;
	return(hw_watch->set_state(pctl));
}

int
Thread::clear_debug()
{
	if (!hw_watch)
		return 1;
	return(hw_watch->clear_state(pctl));
}

// Cancel signals in a thread.
// If signals are pending at process, use /proc.
// If signals are pending at thread, post cancel request
// to thread.  If thread is busy,
// save request until we get a notifier op that says
// cancel complete
// Otherwise, if signals are pending at lwp,
// use /proc.
//
int
Thread::cancel_sigs(sig_ctl *inset)
{
	sigset_t	set;
	sig_ctl		proc_set;
	sig_ctl		lwp_set;
	int		lwp_current;
	int		update = 0;
	Msg_id		msg;
	int		sig;

	if (inset)
	{
		mysigsetcombine(&this->cancel_set, &inset->signals,
			&set);
	}
	else
	{
		if (mysigsetisempty(&this->cancel_set))
			return 1;
		memcpy((char *)&set, (char *)&this->cancel_set,
			sizeof(sigset_t));
	}

	premptyset(&this->cancel_set);

	if (!parent->stop_all())
		return 0;
	
	if (!read_state() ||
		!parent->proc_ctl()->pending_sigs(&proc_set, 1))
	{
		msg = ERR_sig_pending;
		goto pending_out;
	}

	if (state != es_off_lwp)
	{
		if (!pctl || !pctl->pending_sigs(&lwp_set, 0) ||
			!pctl->current_sig(lwp_current))
		{
			msg = ERR_sig_pending;
			goto pending_out;
		}
	}
	else
	{
		premptyset(&lwp_set.signals);
		lwp_current = 0;
	}

	for(sig = 1; sig <= NUMBER_OF_SIGS; sig++)
	{

		if (!prismember(&set, sig))
			continue;

		if (prismember(&proc_set.signals, sig))
		{
			DPRINT(DBG_THREAD, ("cancel_sig: %s sig %d pending at process\n", pobj_name, sig));
			if (!parent->proc_ctl()->cancel_sig(sig))
				goto cancel_out;
		}
		else if (prismember(&this->map.thr_psig, sig))
		{
			DPRINT(DBG_THREAD, ("cancel_sig: %s sig %d in thread psig\n", pobj_name, sig));
			if (map.thr_dbg_busy && map.thr_dbg_cancel)
			{
				DPRINT(DBG_THREAD, ("cancel_sig: %s sig %d thread busy\n", pobj_name, sig));
				praddset(&this->cancel_set, sig);
			}
			else
			{
				update = 1;
				praddset(&this->map.thr_dbg_set, sig);
				map.thr_dbg_cancel = 1;
			}
		}
		else
		{
			if (sig == lwp_current)
			{
				DPRINT(DBG_THREAD, ("cancel_sig: %s sig %d current sig for lwp\n", pobj_name, sig));
				if (!pctl->cancel_current())
					goto cancel_out;
			}
			else if (prismember(&lwp_set.signals, sig))
			{
				DPRINT(DBG_THREAD, ("cancel_sig: %s sig %d pending at lwp\n", pobj_name, sig));
				if (!pctl->cancel_sig(sig))
					goto cancel_out;
			}
		}
	}
	if (update)
	{
		if (!write_state())
			goto cancel_out;
	}
	return parent->restart_all();
cancel_out:
	msg = ERR_sig_cancel;
pending_out:
	printe(msg, E_ERROR, pobj_name);
	parent->restart_all();
	return 0;
}

int
Thread::pending_sigs(sig_ctl *outset)
{
	sig_ctl		proc_set;
	sig_ctl		lwp_set;
	int		lwp_current;

	if (!parent->stop_all())
		return 0;
	
	if (!read_state() ||
		!parent->proc_ctl()->pending_sigs(&proc_set, 1))
	{
		printe(ERR_sig_pending, E_ERROR, pobj_name);
		parent->restart_all();
		return 0;
	}

	if (state != es_off_lwp)
	{
		if (!pctl || !pctl->pending_sigs(&lwp_set, 0) ||
			!pctl->current_sig(lwp_current))
		{
			printe(ERR_sig_pending, E_ERROR, pobj_name);
			parent->restart_all();
			return 0;
		}
		mysigsetcombine(&proc_set.signals, &lwp_set.signals, 
			&outset->signals);
		if (lwp_current)
			praddset(&outset->signals, lwp_current);
	}
	else
		memcpy((char *)&outset->signals, (char *)&proc_set.signals,
			sizeof(sigset_t));
	mysigsetcombine(&outset->signals, &this->map.thr_psig,
		&outset->signals);
	return parent->restart_all();
}

// Copy process and program level events for a given thread
// from our parent process.
// This version of copy_et is used for creating a new thread
// in an existing process, not for forkall.
// If event is currently invalid,
// parent process, we just re-initialize 
// with new thread id instead of copying.
// Be careful not to copy an event with the same id twice.
// Event list is sorted by number to make this easier.
// We must be careful since we may be copying from the same
// process we are adding to:  the event list may change
// as we are going.

int
Thread::copy_et_create()
{
	Event	*eptr = parent->event_list();
	int	id = -1;
	int	ret = 1;

	while(eptr)
	{
		Event	*tmp;

		// save next in case we add to list here
		tmp = eptr->enext();
		if (eptr->get_level() >= P_PROCESS)
		{
			if (id != eptr->get_id())
			{
				// not yet seen
				id = eptr->get_id();
				DPRINT(DBG_EVENT, ("Thread::copy_et_create, %s event %d\n", pobj_name, id));

				if (((eptr->get_state() == E_INVALID) ||
					(eptr->get_state() == E_DISABLED_INV))
					&& (eptr->get_obj() == 0))
				{
					if (!eptr->re_init((ProcObj *)this))
					{
						eptr->remove((ProcObj *)this);
						m_event.drop_event(eptr);
						ret = 0;
					}
				}
				else
					if (!m_event.copy(eptr,(ProcObj *)this, 0))
						ret = 0;
			}
		}
		else
		{
			id = -1;
		}
		eptr = tmp;
	}
	// copy global signal state
	memcpy((char *)&this->sigset, 
		(char *)parent->events()->siglist.sigset(),
		sizeof(sigset_t));
	return ret;
}

// This version of copy_et is used for forkall; we copy all events
// for the thread that corresponds to this thread in the
// parent process (and whose level is > P_THREAD).  
// This depends on forkall preserving thread ids.
int
Thread::copy_et_forkall(Process *proc)
{
	Event	*eptr = proc->event_list();
	int	ret = 1;

	for(; eptr; eptr = eptr->enext())
	{
		if (eptr->get_level() >= P_PROCESS)
		{
			Thread	*thread;
			thread = (Thread *)eptr->get_obj();
			if (thread && 
				(thread->thread_id() == map.thr_tid))
			{
				DPRINT(DBG_EVENT, ("Thread::copy_et_forkall, %s event %d\n", pobj_name, eptr->get_id()));

				if (!m_event.copy(eptr,(ProcObj *)this, 1))
						ret = 0;
			}
		}
	}
	// copy global signal state
	memcpy((char *)&this->sigset, 
		(char *)proc->events()->siglist.sigset(),
		sizeof(sigset_t));
	return ret;
}

// get rid of events pertaining to this thread
// if the thread is the last thread for a process or program
// level event, just clean it up; do not delete altogether
// Be careful, since we may be deleting from middle of list
// as we are walking it.
int
Thread::cleanup_et()
{
	Event	*eptr = parent->event_list();
	
	premptyset(&sigset);

	while(eptr)
	{
		Event	*n, *p;

		n = eptr->enext();
		p = eptr->eprev();

		if (eptr->get_obj() == (ProcObj *)this)
		{
			if (eptr->get_level() < P_PROCESS)
			{
				eptr->remove((ProcObj *)this);
			}
			else
			{
				if ((n && (n->get_id() == eptr->get_id())) ||
					(p && (p->get_id() == eptr->get_id())))
				{
					// not the last with this id
					eptr->remove((ProcObj *)this);
				}
				else
				{
					// last in series
					eptr->cleanup();
				}
			}
		}
		eptr = n;
	}

	cleanup_foreign();

	if (hw_watch)
	{
		if (pctl)
			hw_watch->clear_state(pctl);
		delete(hw_watch);
		hw_watch = 0;
	}
	return 1;
}
#endif
