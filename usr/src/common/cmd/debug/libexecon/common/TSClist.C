#ident	"@(#)debugger:libexecon/common/TSClist.C	1.8"

#include "Machine.h"
#ifdef HAS_SYSCALL_TRACING

#include "TSClist.h"
#include "Ev_Notify.h"
#include "Proctypes.h"
#include <sys/syscall.h>
#include <sys/types.h>

struct TSCevent {
	int		tsc;
	NotifyEvent	*entry_events;
	NotifyEvent	*exit_events;
	TSCevent	*next_tsc;
			TSCevent(int sys) { tsc = sys; entry_events = 0;
				exit_events = 0; next_tsc = 0; }
			~TSCevent();
};

TSCevent::~TSCevent()
{
	NotifyEvent	*ne = entry_events;
	while(ne)
	{
		NotifyEvent	*tmp;
		tmp = ne;
		ne = ne->next();
		delete(tmp);
	}
	ne = exit_events;
	while(ne)
	{
		NotifyEvent	*tmp;
		tmp = ne;
		ne = ne->next();
		delete(tmp);
	}
}

// TSClist maintains a list of TSCevents, ordered by system
// call number.  Each TSCevent has a list of entry events
// and a list of exit events.  TSClist also maintains an
// overall entry mask and exit mask.

TSClist::TSClist()
{
	// start off trapping certain special syscalls
	premptyset(&entrymask.scalls)
	premptyset(&exitmask.scalls)
	praddset(&exitmask.scalls, SYS_exec);
	praddset(&exitmask.scalls, SYS_execve);
	praddset(&exitmask.scalls, SYS_fork);
	praddset(&exitmask.scalls, SYS_vfork);
#ifdef DEBUG_THREADS
	praddset(&exitmask.scalls, SYS_fork1);
	praddset(&exitmask.scalls, SYS_forkall);
	praddset(&exitmask.scalls, SYS_lwpcreate);
#endif

	_events = 0;
}

TSClist::~TSClist()
{

	while(_events)
	{
		TSCevent	*e;

		e = _events;
		_events = _events->next_tsc;
		delete e;
	}
}

sys_ctl *
TSClist::tracemask( int mode )
{
	if ( mode == TSC_exit )
		return &exitmask;
	else
		return &entrymask;
}

TSCevent*
TSClist::lookup(int sys)
{
	register TSCevent	*e = _events;

	while(e)
	{
		if (sys == e->tsc)
			return e;
		if (e->tsc > sys)
			return 0;
		e = e->next_tsc;
	}
	return 0;
}

// find is like lookup, but adds a TSCevent if none exists already
TSCevent*
TSClist::find(int sys)
{
	register TSCevent	*e = _events; 
	TSCevent		*eprev = 0, *enew;

	for(; e; eprev = e, e = e->next_tsc)
	{
		if (sys == e->tsc)
			return e;
		if (sys < e->tsc)
			break;
	}

	// not found - insert in list, keeping list sorted
	enew = new TSCevent(sys);
	enew->next_tsc = e;
	if (!eprev)
	{
		// first event
		_events = enew;
	}
	else
	{
		eprev->next_tsc = enew;
	}
	return enew;
}

NotifyEvent *
TSClist::events( int sys, int mode )
{
	TSCevent	*e = lookup(sys);
	if (e == 0)
		return 0;
	if ( mode == TSC_exit )
		return e->exit_events;
	else
		return e->entry_events;
}

// add an event;
// returns 1 if the sysmask changes; else 0
int
TSClist::add( int sys, int mode, Notifier func, void *thisptr, 
	ProcObj *p )
{
	int		changed = 0;
	TSCevent	*e = find(sys);
	NotifyEvent	*ne, *newevent;

	newevent = new NotifyEvent(func, thisptr, p);

	if (mode == TSC_exit)
	{
		ne = e->exit_events;
		if (!ne)
		{
			praddset(&exitmask.scalls, sys);
			changed = 1;
		}
		else
		{
			newevent->prepend(ne);
		}
		e->exit_events = newevent;
	}
	else 
	{
		ne = e->entry_events;
		if (!ne)
		{
			praddset(&entrymask.scalls, sys);
			changed = 1;
		}
		else
		{
			newevent->prepend(ne);
		}
		e->entry_events = newevent;
	}
	return changed;
}

// delete an event;
// returns 1 if the sigmask changes; else 0
int
TSClist::remove( int sys, int mode, Notifier func, 
	void *thisptr, ProcObj *p )
{
	NotifyEvent	*ne;
	int		changed = 0;
	TSCevent 	*e = lookup(sys);

	if (mode == TSC_exit)
	{
		ne = e->exit_events;
		for(; ne; ne = ne->next())
		{
			if ((ne->func == func) &&
				(ne->object == p) &&
				(ne->thisptr == thisptr))
			break;
		}
		if (!ne)
			return 0;
		if (ne == e->exit_events)
		{
			e->exit_events = ne->next();
		}
		ne->unlink();
		delete(ne);

		if (!e->exit_events && !SPECIAL_EXIT(sys))
		{
			prdelset(&exitmask.scalls, sys);
			changed = 1;
		}
	}
	else
	{
		ne = e->entry_events;
		for(; ne; ne = ne->next())
		{
			if ((ne->func == func) &&
				(ne->object == p) &&
				(ne->thisptr == thisptr))
			break;
		}
		if (!ne)
			return 0;
		if (ne == e->entry_events)
		{
			e->entry_events = ne->next();
		}
		ne->unlink();
		delete(ne);
		if (!e->entry_events)
		{
			prdelset(&entrymask.scalls, sys);
			changed = 1;
		}
	}
	return changed;
}
#endif
