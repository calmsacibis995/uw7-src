#ident	"@(#)debugger:libexecon/common/EventTable.C	1.4"

#include "EventTable.h"
#include "Object.h"
#include "Proglist.h"
#include "Program.h"
#include "Event.h"

EventTable::EventTable()
{
	onstoplist = 0; 
	object = 0;
	watchlist = 0;
	firstevent = 0;
#if EXCEPTION_HANDLING
	exceptionlist = 0;
#endif
}

static void
delete_list(NotifyEvent *list)
{
	NotifyEvent	*ne = list;
	while(ne)
	{
		NotifyEvent	*tmp;
		tmp = ne;
		ne = ne->next();
		tmp->unlink();
		delete tmp;
	}
}

EventTable::~EventTable()
{
	delete_list(onstoplist);
	delete_list(watchlist);
#if EXCEPTION_HANDLING
	delete_list(exceptionlist);
#endif
}

// Find an event table associated with this object
EventTable *
find_et( int fdobj, char *&path )
{
	EventTable	*et;
	Object 		*s;
	Program		*p;

	path = 0;

	if (( fdobj == -1 ) ||
		( (s = find_object(fdobj, 0 )) == 0 ))
	{
		return 0;
	}
	for (p = proglist.prog_list(); p ; p = p->next())
	{
		if (((et = p->events()) != 0) && (et->object == s))
		{
			// Get rid of proto program.
			// Only proto programs have event tables.
			// For a live program, the event table(s) is
			// stored with particular ProcObjs.

			path = (char *)(p->src_path());
			proglist.remove_program(p);
			return et;
		}
	}
	// Not found
	et = new EventTable;
	et->object = s;
	return et;
}

EventTable *
dispose_et( EventTable * e )
{
	if (e && (e->object == 0))
	{
		delete e;
	}
	return 0;
}

void
EventTable::add_to_list(NotifyEvent *&list, NotifyEvent *ne)
{
	// add to end of list to preserve order
	if (!list)
	{
		list = ne;
		return;
	}
	NotifyEvent *prev = list;
	for (; prev->next(); prev = prev->next())
		;
	ne->append(prev);
}

int
EventTable::remove_from_list(NotifyEvent *&list, Notifier func, void *thisptr, ProcObj *p)
{
	NotifyEvent	*ne = list;
	for(; ne; ne = ne->next())
	{
		if ((ne->func == func) &&
			(ne->object == p) &&
			(ne->thisptr == thisptr))
		break;
	}
	if (!ne)
		return 0;
	if (ne == list)
	{
		list = ne->next();
	}
	ne->unlink();
	delete(ne);
	return 1;
}

void
EventTable::set_onstop(Notifier func, void *thisptr, ProcObj *p)
{
	add_to_list(onstoplist, new NotifyEvent(func, thisptr, p));
}

int
EventTable::remove_onstop(Notifier func, void *thisptr, ProcObj *p)
{
	return remove_from_list(onstoplist, func, thisptr, p);
}

void
EventTable::set_watchpoint(Notifier func, void *thisptr, ProcObj *p)
{
	NotifyEvent	*ne = new NotifyEvent(func, thisptr, p);
	if (watchlist)
		ne->prepend(watchlist);
	watchlist = ne;
}

int
EventTable::remove_watchpoint(Notifier func, void *thisptr, ProcObj *p)
{
	return remove_from_list(watchlist, func, thisptr, p);
}

#if EXCEPTION_HANDLING

void
EventTable::set_eh_event(Notifier func, void *thisptr, ProcObj *p)
{
	add_to_list(exceptionlist, new NotifyEvent(func, thisptr, p));
}

int
EventTable::remove_eh_event(Notifier func, void *thisptr, ProcObj *p)
{
	return remove_from_list(exceptionlist, func, thisptr, p);
}

#endif
