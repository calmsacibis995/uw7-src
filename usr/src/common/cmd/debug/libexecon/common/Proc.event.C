#ident	"@(#)debugger:libexecon/common/Proc.event.C	1.18"

#include "Event.h"
#include "Process.h"
#include "ProcObj.h"
#include "Thread.h"
#include "EventTable.h"
#include "Interface.h"
#include "Machine.h"
#include "List.h"
#include "Procctl.h"
#include "Proglist.h"
#include "Watchlist.h"
#include "Proctypes.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

#include <string.h>

// just mark as removed
void
Process::remove_all_bkpt(Breakpoint *b)
{
	if (!b)
		return;
	remove_all_bkpt(b->left());
	remove_all_bkpt(b->right());
	b->set_remove();
}

// no_reset says do not change breakpoint settings;
// this is used for fork: remove all breakpoints in new
// process for which there are entries in old process'
// breaklist, but do not change settings in old process
int
Process::lift_all_bkpt(Breakpoint *b, int no_reset)
{
	if (!b)
		return 1;
	if (!lift_all_bkpt(b->left(), no_reset) ||
		!lift_all_bkpt(b->right(), no_reset))
		return 0;
	if ( b->is_inserted() && (lift_bkpt(b, no_reset) == 0 ) )
	{
		return 0;
	}
	return 1;
}

int 
Process::insert_all_bkpt(Breakpoint *b)
{
	if (!b)
		return 1;
	if (!insert_all_bkpt(b->left()) ||
		!insert_all_bkpt(b->right()))
		return 0;
	if ( !b->is_inserted() && insert_bkpt(b) == 0 )
	{
		return 0;
	}
	return 1;
}


// Use an event table that might have been part
// of an earlier process instance that died.
int
Process::use_et( EventTable * e )
{
	if ((e == 0) ||
		( etable != 0 ) ||
		( state == es_corefile ))
	{
		printe(ERR_internal, E_ERROR, "Process::use_et", 
			__LINE__);
		return 0;
	}
	etable = e;
	return 1;
}

// Re-initializes events
int
Process::re_init_et()
{
	Breakpoint	*b;
	Event		*eptr;

	if (!etable || state == es_corefile)
	{
		printe(ERR_internal, E_ERROR, "Process::re_init_et",
			__LINE__);
		return 0;
	}

	b = etable->breaklist.first();
	if (!insert_all_bkpt(b))
		return 0;

	firstevent = etable->firstevent;

	if (!head_thread)
	{
		eptr = firstevent;
		while(eptr)
		{
			Event	*etmp = eptr->enext();
			if (!eptr->re_init(this))
			{
				printe(ERR_event_reset, E_WARNING,
					eptr->get_id(), pobj_name);
				eptr->remove((ProcObj *)this);
				if (eptr == firstevent)
					firstevent = etmp;
				m_event.drop_event(eptr);
			}
			eptr = etmp;
		}
	}

	// re-init resets system call and signal masks but
	// does not do low-level setting - do it now
	if ( !reset_sigs(etable))
	{
		return 0;
	}
	memcpy((char *)&sigset, (char *)etable->siglist.sigset(),
		sizeof(sigset_t));
#ifdef HAS_SYSCALL_TRACING
	if (flags & L_IGNORE_FORK)
	{
		if (!etable->tsclist.events(SYSCALL_fork, TSC_exit))
			prdelset(etable->tsclist.tracemask(TSC_exit), SYS_fork);
#ifdef DEBUG_THREADS
		if (!etable->tsclist.events(SYSCALL_fork1, TSC_exit))
			prdelset(etable->tsclist.tracemask(TSC_exit), SYS_fork1);
		if (!etable->tsclist.events(SYSCALL_forkall, TSC_exit))
			prdelset(etable->tsclist.tracemask(TSC_exit), SYS_forkall);
#endif
		// always trap vfork so we can sanely release child
	}
	else 
	{
		// might have been deleted by an earlier process
		// that used this event table
		praddset(etable->tsclist.tracemask(TSC_exit), SYSCALL_fork);
		praddset(etable->tsclist.tracemask(TSC_exit), SYSCALL_vfork);
#ifdef DEBUG_THREADS
		praddset(etable->tsclist.tracemask(TSC_exit), SYSCALL_fork1);
		praddset(etable->tsclist.tracemask(TSC_exit), SYSCALL_forkall);
#endif
	}
#ifdef DEBUG_THREADS
	// might have been deleted by an earlier process
	// that used this event table
	praddset(etable->tsclist.tracemask(TSC_exit), SYS_lwpcreate);
#endif
	if (( pctl->sys_entry(etable->tsclist.tracemask(TSC_entry)) == 0) ||
		( pctl->sys_exit(etable->tsclist.tracemask(TSC_exit)) == 0))
	{
		printe(ERR_no_syscalls, E_ERROR, pobj_name);
		return 0;
	}
#endif
	return 1;
}

// Remove all process events and trace flags for released
// processes, dead or killed processes and processes that exec
int
Process::cleanup_et(int mode, int delete_events)
{
	Breakpoint 	*b;

	if (hw_watch)
	{
		if (pctl)
			hw_watch->clear_state(pctl);
		delete(hw_watch);
		hw_watch = 0;
	}

	b = etable->breaklist.first();

	// If we are releasing, replace all breakpoints with
	// the original text.
	// If the process died, just mark them all as removed
	if (mode == P_RELEASE)
	{
		if (!lift_all_bkpt(b, 0))
			return 0;
		if ( state == es_breakpoint )
			state = es_halted;
	}
	else
	{
		remove_all_bkpt(b);
	}

	remove(bk_hoppt);
	remove(bk_destpt);
	remove(bk_dynpt);
#ifdef DEBUG_THREADS
	remove(bk_threadpt);
#endif
#if EXCEPTION_HANDLING
	remove(bk_ehpt);
#endif
#ifndef HAS_SYSCALL_TRACING
	remove(bk_execvept);
	remove(bk_forkpt);
	remove(bk_fork1pt);
	remove(bk_forkallpt);
	remove(bk_vforkpt);
#endif

	if (delete_events)
	{
		Event	*eptr = firstevent;
		while(eptr)
		{
			Event	*etmp = eptr->enext();
			m_event.drop_event(eptr);
			eptr = etmp;
		}
		etable->object = 0;
	}
	else
	{
		// there may be multiple events
		// with the same event id due
		// to multiple threads.
		// If an event's level is < P_PROGRAM,
		// we always remove it.
		// Otherwise, if it is not the first
		// event with the same ID, we remove it.
		// Otherwise, we just clean it up and mark
		// it invalid

		Event	*eptr = firstevent;
		int	id = -1;

		while(eptr)
		{
			Event	*etmp = eptr->enext();
			if (eptr->get_level() < P_PROGRAM)
			{
				id = -1;
				if (eptr == firstevent)
					firstevent = etmp;
				eptr->remove((ProcObj *)this);
				m_event.drop_event(eptr);
			}
			else
			{
				if (eptr->get_id() == id)
				{
					if (eptr == firstevent)
						firstevent = etmp;
					eptr->remove((ProcObj *)this);
					m_event.drop_event(eptr);
				}
				else 
				{
					// first in series
					id = eptr->get_id();
					eptr->cleanup();
				}

			}
			eptr = etmp;
		}
		etable->firstevent = firstevent;
	}
	cleanup_foreign();

	firstevent = 0;
	etable = dispose_et( etable );
	return 1;
}

// copy event table for fork
int
Process::setup_et_copy(Process *oproc)
{
	EventTable	*oe = oproc->etable;

	etable = new EventTable;
	etable->object = oe->object;

	etable->siglist.copy(oe->siglist);
	memcpy((char *)&sigset, (char *)etable->siglist.sigset(),
		sizeof(sigset_t));
	if (!reset_sigs(etable))
		return 0;

	// lift all breakpoints in new process for which
	// there are entries in old process' breaklist;
	// do not change settings in old process

	if (!lift_all_bkpt(oe->breaklist.first(), 1))
		return 0;

	if (oproc->hoppt)
		hoppt = set_bkpt(oproc->hoppt->addr(), 0, 0);
	if (oproc->dynpt)
		dynpt = set_bkpt(oproc->dynpt->addr(), 0, 0);
	if (oproc->destpt)
		destpt = set_bkpt(oproc->destpt->addr(), 0, 0);
#ifdef DEBUG_THREADS
	if (oproc->threadpt)
		threadpt = set_bkpt(oproc->threadpt->addr(), 0, 0);
#endif
#if EXCEPTION_HANDLING
	if (oproc->eh_data)
		eh_data = new Exception_data(oproc->eh_data, this);
#endif
#ifndef HAS_SYSCALL_TRACING
	if (oproc->execvept)
		execvept = set_bkpt(oproc->execvept->addr(), 0, 0);
	if (oproc->forkpt)
		forkpt = set_bkpt(oproc->forkpt->addr(), 0, 0);
	if (oproc->fork1pt)
		fork1pt = set_bkpt(oproc->fork1pt->addr(), 0, 0);
	if (oproc->forkallpt)
		forkallpt = set_bkpt(oproc->forkallpt->addr(), 0, 0);
	if (oproc->vforkpt)
		vforkpt = set_bkpt(oproc->vforkpt->addr(), 0, 0);
#endif
	return 1;
}

// Copy each program level event.
// Process has forked, but there are no threads.
// There may be multiple copies of each event; only copy once.
int
Process::copy_events(Process *oproc)
{
	Event	*eptr = oproc->firstevent;
	int	id = -1;
	int	ret = 1;

	for(; eptr; eptr = eptr->enext())
	{
		if (eptr->get_level() == P_PROGRAM)
		{
			if (eptr->get_id() != id)
			{
				// first in series
				id = eptr->get_id();
				if (!m_event.copy(eptr, this, 1))
					ret = 0;
			}
		}
		else
			id = -1;
	}
	return ret;
}

// events are kept in order, sorted by event id
void
Process::add_event(Event *event)
{
	int	id = event->get_id();
	Event	*e = firstevent;
	Event	*eprev = 0;

	while(e && (e->get_id() < id))
	{
		eprev = e;
		e = e->enext();
	}
	if (!eprev)
	{
		// first event
		if (e)
			event->eprepend(e);
		firstevent = event;
	}
	else
		event->eappend(eprev);

}

void
Process::remove_event(Event *event)
{
	if (event == firstevent)
		firstevent = event->enext();
	event->eunlink();
}

int
Process::cancel_sigs(sig_ctl *inset)
{
	sig_ctl		pend_set;
	int		current;

	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "Process:cancel_sigs",
			__LINE__);
		return 0;
	}
	if (!pctl->pending_sigs(&pend_set, 0) ||
		!pctl->current_sig(current))
	{
		printe(ERR_sig_pending, E_ERROR, pobj_name);
		return 0;
	}
	for(int sig = 1; sig <= NUMBER_OF_SIGS; sig++)
	{
		if (!prismember(&inset->signals, sig))
			continue;
		if (sig == current)
		{
			if (!pctl->cancel_current())
			{
				printe(ERR_sig_cancel,
					E_ERROR, pobj_name);
				return 0;
			}
		}
		else if (prismember(&pend_set.signals, sig))
		{
			if (!pctl->cancel_sig(sig))
			{
				printe(ERR_sig_cancel,
					E_ERROR, pobj_name);
				return 0;
			}
		}
	}
	return 1;
}

int
Process::pending_sigs(sig_ctl *outset)
{
	sig_ctl	pend_set;
	int	current;

	if (!pctl)
	{
		printe(ERR_internal, E_ERROR, "Process:pending_sigs",
			__LINE__);
		return 0;
	}
	if (!pctl->pending_sigs(&pend_set, 0) ||
		!pctl->current_sig(current))
	{
		printe(ERR_sig_pending, E_ERROR, pobj_name);
		return 0;
	}
	memcpy((char *)outset, (char *)&pend_set, sizeof(sig_ctl));
	if (current)
		praddset(&outset->signals, current);
	return 1;
}

#ifdef DEBUG_THREADS

// just removed a bkpt - go through thread list and make
// set state of other threads stopped at this breakpoint
// to es_halted
void
Process::clear_bkpt(Iaddr addr, Thread *t)
{
	for(Thread *thread = head_thread; thread; 
		thread = thread->next())
	{
		if (thread == t)
			continue;
		if ((thread->state == es_breakpoint) &&
			(thread->pc == addr))
			thread->state = es_halted;
	}
	
}

#endif
