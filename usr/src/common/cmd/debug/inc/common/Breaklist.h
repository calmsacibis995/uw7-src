#ifndef Breaklist_h
#define Breaklist_h
#ident	"@(#)debugger:inc/common/Breaklist.h	1.2"

#include "Iaddr.h"
#include "Ev_Notify.h"
#include "Machine.h"

class Breakpoint;
class ProcObj;

// macros for manipulating flags field
#define REMOVE(x)	( x = (x) & ~0x1 )
#define IS_INSERTED(x)	( (x) & 0x1 )
#define INSERT(x)	( x = (x) | 0x1 )

// A Breaklist is maintained as a binary search tree, sorted by
// sorted by address.

class Breaklist {
	Breakpoint	*root;
	void		dispose(Breakpoint *);
public:
			Breaklist() { root = 0; }
			~Breaklist();
	Breakpoint	*add( Iaddr, Notifier , void *, ProcObj *, 
				ev_priority);
	Breakpoint 	*remove( Iaddr, Notifier, void *, ProcObj *);
	int 		remove( Iaddr );
	Breakpoint	*lookup( Iaddr );
	Breakpoint	*first() { return root; }
};


class Breakpoint {
	char		_flags;
	char		_oldtext[BKPTSIZE];
	Breakpoint	*_left;
	Breakpoint	*_right;
	Iaddr		_addr;
	NotifyEvent 	*_events;
	friend class	Breaklist;
public:
			Breakpoint( Iaddr , Notifier, void *, ProcObj *,
				ev_priority);
			~Breakpoint();
			// Access functions
	Iaddr		addr()    { return _addr; }
	char		*oldtext() { return _oldtext; }
	NotifyEvent	*events() { return _events; }
	Breakpoint	*left()   { return _left; }
	Breakpoint	*right()  { return _right; }
	void		set_remove() { REMOVE(_flags); }
	void		set_insert() { INSERT(_flags); }
	int		is_inserted() { return IS_INSERTED(_flags); }
	void		add_event( Notifier, void *, ProcObj *, 
				ev_priority);
	int		remove_event( Notifier, void *, ProcObj *);
};

#endif


// end of Breaklist.h

