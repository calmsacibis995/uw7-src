#ifndef EventTable_h
#define EventTable_h
#ident	"@(#)debugger:inc/common/EventTable.h	1.4"

// EventTable manages the low-level events associated with a given
// process.  When a process dies or exits, its event table is saved with
// a proto program so its events can be re-instantiated.

#include "Breaklist.h"
#include "Siglist.h"
#include "TSClist.h"
#include "Ev_Notify.h"

class Event;
class Object;
class ProcObj;

class EventTable {
	void		add_to_list(NotifyEvent *&, NotifyEvent *);
	int		remove_from_list(NotifyEvent *&, Notifier, void *, ProcObj *);
public:
	Breaklist	breaklist;
	Siglist		siglist;
#ifdef HAS_SYSCALL_TRACING
	TSClist		tsclist;
#endif
	NotifyEvent	*watchlist;
	NotifyEvent	*onstoplist;
#if EXCEPTION_HANDLING
	NotifyEvent	*exceptionlist;
#endif
	Object 		*object;
	Event		*firstevent; 
			EventTable();
			~EventTable();

	void		set_watchpoint(Notifier, void *, ProcObj *);
	int		remove_watchpoint(Notifier, void *, ProcObj *);
	void		remove_event(Event *);
	void		set_onstop(Notifier, void *, ProcObj *);
	int		remove_onstop(Notifier, void *, ProcObj *);
#if EXCEPTION_HANDLING
	void		set_eh_event(Notifier, void *, ProcObj *);
	int		remove_eh_event(Notifier, void *, ProcObj *);
#endif
};

EventTable		*find_et( int, char *&path );
EventTable		*dispose_et( EventTable * );

#endif

// end of EventTable.h

