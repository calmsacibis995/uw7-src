#ident	"@(#)debugger:libexecon/common/Breaklist.C	1.3"

#include	"Breaklist.h"
#include	"Ev_Notify.h"
#include	"Iaddr.h"

// A Breaklist is maintained as a binary search tree, ordered
// by address

Breaklist::~Breaklist()
{
	Breakpoint	*b = root;
	
	dispose(b);
}

void
Breaklist::dispose(Breakpoint *b)
{
	if (!b)
		return;
	dispose(b->_left);
	dispose(b->_right);
	delete(b);
}

// add a breakpoint with the given address; if there is
// already a breakpoint at that addr, add event to its 
// eventlist
// breakpoint events are sorted by priority, from high to low

Breakpoint *
Breaklist::add( Iaddr a, Notifier func, void *thisptr, ProcObj *pobj,
	ev_priority ep)
{
	Breakpoint	*b, *nb, *p = 0;

	b = root;
	while(b)
	{
		if (a > b->_addr)
		{
			p = b;
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			p = b;
			b = b->_left;
		}
		else 
		{
			// already a breakpoint at that address
			// just add the event
			b->add_event(func, thisptr, pobj, ep);
			return b;
		}
	}
	nb = new Breakpoint(a, func, thisptr, pobj, ep);
	if (!p && !b)
	{
		root = nb;
	}
	else if (a > p->_addr)
	{
		p->_right = nb;
	}
	else
	{
		p->_left = nb;
	}
	return nb;
}

// remove an event from a breakpoint, but don't delete the
// breakpoint from the tree
Breakpoint *
Breaklist::remove(Iaddr a, Notifier func, void *thisptr, ProcObj *pobj)
{
	Breakpoint	*b = root;
	
	while(b)
	{
		if (a > b->_addr)
		{
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			b = b->_left;
		}
		else 
		{
			// found
			if (!b->remove_event(func, thisptr, pobj))
				return 0;
			return b;
		}
	}
	return 0;
}

// remove a breakpoint from the tree
int
Breaklist::remove( Iaddr a )
{
	Breakpoint	*b, *p = 0;
	int		left = 0;

	b = root;
	while(b)
	{
		if (a > b->_addr)
		{
			p = b;
			b = b->_right;
			left = 0;
		}
		else if (a < b->_addr)
		{
			p = b;
			b = b->_left;
			left = 1;
		}
		else 
		{
			break;
		}
	}
	if (!b)
		return 0;

	if (!p)
	{
		// removing root
		if (!b->_right && !b->_left)
			root = 0;
		else if (!b->_right)
			root = b->_left;
		else
		{
			root = b->_right;
			if (b->_left)
			{
				Breakpoint	*r;
				r = root->_left;
				while(r && r->_left)
				{
					r = r->_left;
				}
				if (r)
					r->_left = b->_left;
				else
					root->_left = b->_left;
			}
		}
	}
	else if (left)
	{
		// removing left child of some subtree
		if (b->_right)
		{
			Breakpoint	*r;
			p->_left = b->_right;
			r = b->_right;
			while(r && r->_left)
			{
				r = r->_left;
			}
			r->_left = b->_left;
		}
		else
			p->_left = b->_left;
	}
	else
	{
		// removing right child of some subtree
		if (b->_right)
		{
			p->_right = b->_right;
			if (b->_left)
			{
				Breakpoint	*r;
				r = b->_right;
				while(r && r->_left)
				{
					r = r->_left;
				}
				r->_left = b->_left;
			}
		}
		else
			p->_right = b->_left;
	}
	delete b;
	return 1;
}


Breakpoint *
Breaklist::lookup( Iaddr a )
{
	Breakpoint	*b = root;

	while(b)
	{
		if (a > b->_addr)
		{
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			b = b->_left;
		}
		else
		{
			return b;
		}
	}
	return 0;
}

Breakpoint::Breakpoint( Iaddr a, Notifier func, 
	void *thisptr, ProcObj *p, ev_priority ep)
{
	_addr = a;
	_left = 0;
	_right = 0;
	_flags = 0;
	_events = new NotifyEvent(func, thisptr, p, ep);
}

Breakpoint::~Breakpoint()
{
	NotifyEvent	*tmp, *ne = _events;

	while(ne)
	{
		tmp = ne;
		ne = ne->next();
		tmp->unlink();
		delete tmp;
	}
}

// Add an event notifier to an existing breakpoint; notifiers
// are sorted by priority, from high to low
// A new notifier is always added as the last one with the
// same priority to preserve order of entry.
void
Breakpoint::add_event( Notifier func, void *thisptr, ProcObj *p, 
	ev_priority ep)
{
	NotifyEvent	*ne;
	ne = new NotifyEvent(func, thisptr, p, ep);
	if (!_events)
	{
		_events = ne;
	}
	else
	{
		NotifyEvent	*cur = _events;
		NotifyEvent	*prev = 0;
		while(cur && cur->priority >= ep)
		{
			prev = cur;
			cur = cur->next();
		}
		if (prev)
			ne->append(prev);
		else 
		{
			// first
			ne->prepend(cur);
			_events = ne;
		}
	}
}

int
Breakpoint::remove_event( Notifier func, void *thisptr, ProcObj *p)
{
	NotifyEvent	*ne = _events;

	for(; ne ; ne = ne->next())
	{
		if ((ne->func == func) &&
			(ne->object == p) &&
			(ne->thisptr == thisptr))
		{
			if (ne == _events)
				_events = ne->next();
			ne->unlink();
			delete(ne);
			return 1;
		}
	}
	return 0;
}
