#ident	"@(#)debugger:libdbgen/common/List.C	1.3"

#include "List.h"

// singly linked queue package

// delete everything in the list
void
List::clear()
{
	ListItem	*ep = head, *ni;
	while(ep)
	{
		ni = ep;
		ep = ep->lnext;
		delete ni;
	}
	head = tail = current = 0;
}

// append to the end of the list
void
List::add(void *e)
{
	ListItem	*ep = new ListItem;

	ep->ldata = e;
	ep->lnext = 0;
	if (!head)
	{
		head = current = tail = ep;
	}
	else
	{
		tail->lnext = ep;
		tail = ep;
	}
}

void		
List::insert(void *e) 
{
	ListItem * old_current = current; // insert before old_current
	ListItem * old_tail = tail;
	add(e);
	// tail/current is the new element.
	if (old_current)
	{
		ListItem * insert_after = head;
		if (insert_after == old_current)
		{
			// Insert at beginning.  Move last element to first.
			head = tail;
			head->lnext = insert_after;

		}
		else
		{
			while(insert_after->lnext != old_current) 
				insert_after = insert_after->lnext;
			// Insert after insert_after.  Move last element
			// to after insert_after.
			tail->lnext = old_current;
			insert_after->lnext = tail;
		}
		old_tail->lnext = 0;
		tail = old_tail;
	}
}

// delete just one item from the list

int
List::remove(void * item)
{
	ListItem	*ep, *ep2;

	if (!head)
		return 0;
	ep = ep2 = head;

	while(ep)
	{
		if (ep->ldata == item)
			break;
		else
		{
			ep2 = ep;
			ep = ep->lnext;
		}
	}
	if (!ep)
		return 0;

	if (ep == head)
	{
		head = head->lnext;
		if (current == ep)
			current = head;
		delete ep;
	}
	else
	{
		if (current == ep)
			current = ep->lnext;
		ep2->lnext = ep->lnext;
		if (!ep2->lnext)
			tail = ep2;
		delete ep;
	}
	return 1;
}
