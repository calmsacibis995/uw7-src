#ifndef EV_Notify_h
#define EV_Notify_h

#ident	"@(#)debugger:inc/common/Ev_Notify.h	1.4"

#include "Link.h"

class ProcObj;

// Process level primitive events are connected to 
// the higher level EventManager events through Event Notifiers.
// An NotifyEvent object contains a pointer to an interface function,
// a pointer to a ProcObj, and a "this" pointer.
// The interface function calls a class member function, that in turn 
// provides notification actions.
//
// typedef used to register callback functions with event handlers

typedef int (*Notifier)(void *, ...);

// priority of notifiers 
enum ev_priority {
	ev_none,
	ev_low,
	ev_high,
};

struct NotifyEvent : public Link {
	Notifier	func;
	void		*thisptr;
	ProcObj		*object;
	ev_priority	priority;
			NotifyEvent(Notifier f, void *t, ProcObj *o,
				ev_priority ep = ev_none)
				{ func = f; thisptr = t; object = o;
					priority = ep; }
			~NotifyEvent() {}
	NotifyEvent	*next() { return (NotifyEvent *)Link::next(); }
};

extern int notify_sig_e_trigger(void *thisptr);
extern int notify_sys_e_trigger(void *thisptr);
extern int notify_onstop_e_trigger(void *thisptr);
extern int notify_stoploc_trigger(void *thisptr);
extern int notify_stop_e_clean_foreign(void *thisptr);
extern int notify_returnpt_trigger(void *thisptr);
extern int notify_watchframe_start(void *thisptr);
extern int notify_watchframe_watch(void *thisptr);
extern int notify_watchframe_end(void *thisptr);
#if EXCEPTION_HANDLING
extern int notify_eh_trigger(void *thisptr);
#endif

#endif
