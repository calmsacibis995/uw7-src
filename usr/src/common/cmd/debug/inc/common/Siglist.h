#ifndef Siglist_h
#define Siglist_h
#ident	"@(#)debugger:inc/common/Siglist.h	1.3"

#include "Ev_Notify.h"
#include "Proctypes.h"
#include "Machine.h"
#include <signal.h>

class ProcObj;

// _sigctl keeps track of the global mask actually set for the process.
// _sigset keeps track of the process level settings to be used
// when a new thread is created; this set is changed on signal or signal -i;
// it is not effected by a signal event.

class Siglist {
	sig_ctl		_sigctl;
	sigset_t	_sigset;
	NotifyEvent	*_events[ NUMBER_OF_SIGS ];
public:
			Siglist();
			~Siglist();
	void		copy(Siglist &);
	int		add( sigset_t *, Notifier, void *, ProcObj *);
	int		remove( sigset_t *, Notifier, void *, ProcObj *);
	int		catch_sigs(sigset_t *, int level);
	int		ignore_sigs(sigset_t *);
	NotifyEvent 	*events( int sig );
	sig_ctl		*sigctl() { return &_sigctl; }
	sigset_t	*sigset() { return &_sigset; }
};

#endif

// end of Siglist.h

