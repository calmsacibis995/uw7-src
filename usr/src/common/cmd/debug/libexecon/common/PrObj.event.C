#ident	"@(#)debugger:libexecon/common/PrObj.event.C	1.13"

#include "Event.h"
#include "Breaklist.h"
#include "ProcObj.h"
#include "Process.h"
#include "EventTable.h"
#include "Interface.h"
#include "Machine.h"
#include "List.h"
#include "Procctl.h"
#include "Proglist.h"
#include "Watchlist.h"
#include "Proctypes.h"
#include "Ev_Notify.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

// plant breakpoint - first save original text
int
ProcObj::insert_bkpt( Breakpoint * b )
{
	if (( b == 0 ) ||
		( b->is_inserted() ))
	{
		printe(ERR_internal, E_ERROR, "ProcObj::insert_bkpt", __LINE__);
		return 0;
	}
	Process	*proc = process();

	if ((proc->pctl->read( b->addr(), b->oldtext(), BKPTSIZE) != BKPTSIZE) ||
		(proc->pctl->write( b->addr(), BKPTTEXT, BKPTSIZE) != BKPTSIZE))
	{
		printe(ERR_sys_no_breakpt, E_ERROR, b->addr(), pobj_name);
		return 0;
	}
	b->set_insert();
	return 1;
}

// rewrite original text
int
ProcObj::lift_bkpt( Breakpoint * b, int prevent_reset)
{
	if (( b == 0 ) ||
		( !b->is_inserted() ))
	{
		printe(ERR_internal, E_ERROR, "ProcObj::lift_bkpt", __LINE__);
		return 0;
	}

	if ( process()->pctl->write( b->addr(), b->oldtext(), 
			BKPTSIZE) != BKPTSIZE )
	{
		printe(ERR_sys_breakpt, E_ERROR, b->addr(), pobj_name);
		return 0;
	}
	// Instr class keeps a buffer of instruction text.
	// At time buffer was read, it may have contained
	// breakpoints. Once breakpoint is deleted it will
	// not find out about real oldtext, so we update it
	// explicitly.
	instr.set_text(b->addr(), b->oldtext());

	// prevent_reset says do not change breakpoint settings;
	// this is used for fork: remove breakpoint in new
	// process for which there was an entry in old process'
	// breaklist, but do not change settings in old process

	if (!prevent_reset)
		 b->set_remove();
	return 1;
}

// remove internal breakpt
// if other events or threads are using this breakpoint,
// return the breakpoint pointer, else return 0
Breakpoint *
ProcObj::remove( Bkpt_type btype )
{
	Breakpoint	*b;
	EventTable	*tab = process()->etable;

	switch(btype)
	{
	case bk_hoppt:	
		if (!hoppt)
			return 0;
		b = hoppt;
		hoppt = 0;
		break;
	case bk_destpt:	
		if (!destpt)
			return 0;
		b = destpt;
		destpt = 0;
		break;
	case bk_dynpt:
		if (!dynpt)
			return 0;
		b = dynpt;
		dynpt = 0;
		break;
#ifdef DEBUG_THREADS
	case bk_threadpt:	
		if (!threadpt)
			return 0;
		b = threadpt;
		threadpt = 0;
		break;
	case bk_startpt:	
		if (!startpt)
			return 0;
		b = startpt;
		startpt = 0;
		break;
#endif
#if EXCEPTION_HANDLING
	case bk_ehpt:
		if (!eh_data || !eh_data->eh_bkpt)
			return 0;
		b = eh_data->eh_bkpt;
		eh_data->eh_bkpt = 0;
		break;
#endif // EXCEPTION_HANDLING
#ifndef HAS_SYSCALL_TRACING
	case bk_execvept:	
		if (!execvept)
			return 0;
		b = execvept;
		execvept = 0;
		break;
	case bk_forkpt:	
		if (!forkpt)
			return 0;
		b = forkpt;
		forkpt = 0;
		break;
	case bk_vforkpt:	
		if (!vforkpt)
			return 0;
		b = vforkpt;
		vforkpt = 0;
		break;
	case bk_fork1pt:	
		if (!fork1pt)
			return 0;
		b = fork1pt;
		fork1pt = 0;
		break;
	case bk_forkallpt:	
		if (!forkallpt)
			return 0;
		b = forkallpt;
		forkallpt = 0;
		break;
#endif
	default:
		printe(ERR_internal, E_ERROR, "ProcObj::remove",
			__LINE__);
		return 0;
	}

	if (!tab)
	{
		// check here - we can return safely before this
		printe(ERR_internal, E_ERROR, "ProcObj::remove",
			__LINE__);
		return 0;
	}

	if (!b->remove_event(0, 0, this))
	{
		printe(ERR_internal, E_ERROR, "ProcObj::remove", 
			__LINE__);
		return 0;
	}

	if ( b->events())
		return b;

	if ( b->is_inserted() && (lift_bkpt(b) == 0) )
	{
		return 0;
	}

#ifdef DEBUG_THREADS
	// if other threads are stopped at this bkpt,
	// change their state to es_halted
	if (flags & L_THREAD)
		process()->clear_bkpt(b->addr(), (Thread *)this);
#endif

	if ( tab->breaklist.remove( b->addr() ) == 0 )
	{
		printe(ERR_internal, E_ERROR, "ProcObj::remov_bkpt", 
			__LINE__);
		return 0;
	}
	return 0;
}

char *
ProcObj::text_nobkpt( Iaddr addr )
{

	Breakpoint	*b;
	EventTable	*tab = process()->etable;

	// may not be an event table, if a corefile
	if (!tab || ((b = tab->breaklist.lookup(addr)) == 0))
		return 0;
	else
		return(b->oldtext());
}

int
ProcObj::set_watchpoint(Iaddr pl, Rvalue *rv, Notifier func, 
	void *thisptr)
{
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::set_watchpoint", __LINE__);
		return WATCH_FAIL;
	}
	tab->set_watchpoint(func, thisptr, this);
	if (pl != (Iaddr)-1)
	{
		if (!hw_watch)
			hw_watch = new HW_Watch;
		if (hw_watch->add(pl, rv, this, thisptr))
			return WATCH_HARD;
	}
	sw_watch++;
	return WATCH_SOFT;
}

int
ProcObj::remove_watchpoint(int hw, Iaddr pl, Notifier func,
	void *thisptr)
{
	EventTable	*tab = process()->etable;
	int		ret = 1;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_watchpoint", __LINE__);
		return 0;
	}
	if (hw)
	{
		if ((state != es_dead) && hw_watch &&
			!hw_watch->remove(pl, this, thisptr))
			ret = 0;
	}
	else
	{
		sw_watch--;
		if (!sw_watch && (goal == pg_stmt_step) &&
			(ecount ==  STEP_INF_QUIET))
			// we were single stepping for software
			// watchpoints, but we really want to run.
			goal = pg_run;
	}
	if (!tab->remove_watchpoint(func, thisptr, this))
		ret = 0;
	return ret;
}

// low level events are not set in certain circumstance,
// like fork, where the parent's system call trace mask is copied
#ifdef HAS_SYSCALL_TRACING
int
ProcObj::set_sys_trace(int i, Systype systype, 
	Notifier func, void *thisptr, int sset)
{
	sys_ctl		*nsys;
	int		ret = 1;
	EventTable	*tab = process()->etable;

	// tsclist.add returns 1 if the addition changed the current
	// mask

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::set_sys_trace",
			__LINE__);
		return 0;
	}
	if (systype == Entry || systype == Entry_exit)
	{
		if (tab->tsclist.add(i, TSC_entry, func, thisptr, this)
			&& (sset == E_SET_YES))
		{
			nsys = tab->tsclist.tracemask(TSC_entry);
			if ( process()->pctl->sys_entry( nsys ) == 0)
				ret = 0;
		}
	}
	if (systype == Exit || systype == Entry_exit)
	{
		if (tab->tsclist.add(i, TSC_exit, func, thisptr, this)
			&& (sset == E_SET_YES))
		{
			nsys = tab->tsclist.tracemask(TSC_exit);
			if ( process()->pctl->sys_exit( nsys ) == 0)
				ret = 0;
		}
	}
	return ret;
}

// low level events are not deleted when the process is exiting;
// we just keep the event table in synch for future use
int
ProcObj::remove_sys_trace(int i, Systype systype,
	Notifier func, void *thisptr, int sdelete)
{
	sys_ctl		*nsys;
	int		ret = 1;
	EventTable	*tab = process()->etable;

	// tsclist.remove returns 1 if the deletion changed the current
	// mask

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::remove_sys_trace", __LINE__);
		return 0;
	}

	if (systype == Entry || systype == Entry_exit)
	{
		if (tab->tsclist.remove(i, TSC_entry, func, thisptr,
			this) && (sdelete == E_DELETE_YES))
		{
			nsys = tab->tsclist.tracemask(TSC_entry);
			if ( process()->pctl->sys_entry( nsys ) == 0)
				ret = 0;
		}
	}
	if (systype == Exit || systype == Entry_exit)
	{
		if (tab->tsclist.remove(i, TSC_exit, func, thisptr,
			this) && (sdelete == E_DELETE_YES))
		{
			nsys = tab->tsclist.tracemask(TSC_exit);
			if ( process()->pctl->sys_exit( nsys ) == 0)
				ret = 0;
		}
	}
	return ret;
}
#endif

// low level events are not set in certain circumstance,
// like fork, where the parent's address space is copied
Breakpoint *
ProcObj::set_bkpt( Iaddr addr, Notifier func, void *thisptr, 
	ev_priority ep)
{
	Breakpoint	*b;
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, "ProcObj::set_bkpt", __LINE__);
		return 0;
	}
	b = tab->breaklist.add( addr, func, thisptr, this, ep );
	if ( !b->is_inserted() && (insert_bkpt(b) == 0) )
	{
		return 0;
	}
	// if set bkpt on current location; don't want to 
	// hit it immediately - by setting state to es_breakpoint
	// we will step over it.
	if (addr == pc)
	{
		state = es_breakpoint;
		latestbkpt = b;
	}

	return b;
}

// low level events are not deleted when the process is exiting;
// we just keep the event table in synch for future use
int
ProcObj::remove_bkpt( Iaddr addr, Notifier func, void *thisptr, 
	int bdelete)
{
	Breakpoint	*b;
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, "ProcObj::remove_bkpt", 
			__LINE__);
		return 0;
	}

	// first just remove event
	if ((b = tab->breaklist.remove( addr, func, thisptr, this )) 
		== 0)
		return 0;

	// if still events, just return
	if ((bdelete == E_DELETE_NO) || b->events())
		return 1;

	if ( b->is_inserted() && (lift_bkpt(b) == 0) )
	{
		return 0;
	}

	if ( (state == es_breakpoint) && (pc == addr))
	{
		state = es_halted;
	}

#ifdef DEBUG_THREADS
	// if other threads are stopped at this bkpt,
	// change their state to es_halted
	if (flags & L_THREAD)
		process()->clear_bkpt(addr, (Thread *)this);
#endif

	// remove from table
	if ( tab->breaklist.remove( addr ) == 0 )
	{
		printe(ERR_internal, E_ERROR, "ProcObj::remov_bkpt", 
			__LINE__);
		return 0;
	}
	return 1;
}

// low level events are not set in certain circumstance,
// like fork, where the parent's signal trace mask is copied
int
ProcObj::set_sig_event( sigset_t *sigs, Notifier func, void *thisptr,
	int sset )
{
	EventTable	*tab = process()->etable;

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::set_sig_event", __LINE__);
		return 0;
	}

	// siglist.add returns 1 if the addition changed the current
	// mask
	if (tab->siglist.add(sigs, func, thisptr, this) &&
		(sset == E_SET_YES))
	{
		return reset_sigs(tab);
	}
	return 1;
}

// low level events are not deleted when the process is exiting;
// we just keep the event table in synch for future use
int
ProcObj::remove_sig_event( sigset_t *sigs, Notifier func, void *thisptr,
	int sdelete)
{
	EventTable	*tab = process()->etable;


	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::remove_sig_event", __LINE__);
		return 0;
	}
	// siglist.remove returns 1 if the addition changed the current
	// mask
	if (tab->siglist.remove(sigs, func, thisptr, this) &&
		(sdelete == E_DELETE_YES))
	{
		return reset_sigs(tab);
	}
	return 1;
}

int 
ProcObj::catch_sigs(sigset_t *sigs, int level)
{
	EventTable	*tab = process()->etable;

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::catch_sigs", 
			__LINE__);
		return 0;
	}
	for (int i = 1; i <= NUMBER_OF_SIGS; i++)
	{
		if (prismember(sigs, i))
			praddset(&sigset, i);
	}
	// siglist.catch returns 1 if the addition changed 
	// the current mask
	// we may need to add the signal to the global mask,
	// even if it is a thread level request.
	if (tab->siglist.catch_sigs(sigs, level))
	{
		return reset_sigs(tab);
	}
	
	return 1;
}

int 
ProcObj::ignore_sigs(sigset_t *sigs, int level)
{
	
	EventTable	*tab = process()->etable;

	if (tab == 0)
	{
		printe(ERR_internal, E_ERROR, "ProcObj::ignore_sigs",
			__LINE__);
		return 0;
	}
	for (int i = 1; i <= NUMBER_OF_SIGS; i++)
	{
		if (prismember(sigs, i))
			prdelset(&sigset, i);
	}
	// never delete signals for a thread level request
	if (level > P_THREAD)
	{
		// siglist.catch returns 1 if the addition changed 
		// the current mask
		if (tab->siglist.ignore_sigs(sigs))
		{
			return reset_sigs(tab);
		}
	}
	return 1;
}

int
ProcObj::reset_sigs(EventTable *tab)
{
	
	sig_ctl		*nsigs;

	nsigs = tab->siglist.sigctl();
	if (!process()->pctl->trace_sigs( nsigs ))
	{
		printe(ERR_no_signals, E_ERROR, pobj_name );
		return 0;
	}
	return 1;
}

int
ProcObj::set_onstop(Notifier func, void *thisptr)
{
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::set_onstop", __LINE__);
		return 0;
	}
	tab->set_onstop(func, thisptr, this);
	return 1;
}

int
ProcObj::remove_onstop(Notifier func, void *thisptr)
{
	EventTable	*tab = process()->etable;

	if ( tab == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_onstop",
			__LINE__);
		return 0;
	}
	return(tab->remove_onstop(func, thisptr, this));
}

int
ProcObj::get_sig_disp(struct sigaction *&act, int &number)
{
	Process		*proc = process();
	Proclive	*ppctl;

	if (!proc || ((ppctl = proc->proc_ctl()) == 0))
	{
		printe(ERR_internal, E_ERROR, "ProcObj::get_sig_disp",
			__LINE__);
		return 0;
	}
	return ppctl->get_sig_disp(act, number);
}

void
ProcObj::add_foreign(Notifier func, void *thisptr )
{
	NotifyEvent	*ne = new NotifyEvent(func, thisptr, this);
	if (foreignlist)
		ne->prepend(foreignlist);
	foreignlist = ne;
}

int
ProcObj::remove_foreign(Notifier func, void *thisptr )
{
	NotifyEvent	*ne = foreignlist;

	for(; ne; ne = ne->next())
	{
		if ((ne->func == func) &&
			(ne->object == this) &&
			(ne->thisptr == thisptr))
		break;
	}
	if (!ne)
		return 0;
	if (ne == foreignlist)
	{
		foreignlist = ne->next();
	}
	else
		ne->unlink();
	delete(ne);
	return 1;
}

void
ProcObj::cleanup_foreign()
{
	if (foreignlist)
	{
		NotifyEvent	*ne = foreignlist;
		// invalidate and remove foreign events
		// careful! remove NotifyEvent before calling notifier
		// notifier might try to delete from this list
		while(ne)
		{
			Notifier	func;
			void		*thisptr;
			NotifyEvent	*tmp;
			func = ne->func;
			thisptr = ne->thisptr;
			tmp = ne;
			ne = ne->next();
			tmp->unlink();
			delete(tmp);
			(*func)(thisptr);
		}
		foreignlist = 0;
	}
}

// set destpt to destaddr
int
ProcObj::set_destination(Iaddr destaddr)
{
	if ((destpt = set_bkpt(destaddr, 0, 0)) == 0)
		return 0;
	return 1;
}

// Null base class versions
void
ProcObj::add_event(Event *)
{
	return;
}

void
ProcObj::remove_event(Event *)
{
	return;
}

int
ProcObj::pending_sigs(sig_ctl *)
{
	return 0;
}

int
ProcObj::cancel_sigs( sig_ctl * )
{
	return 0;
}

