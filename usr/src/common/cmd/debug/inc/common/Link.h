#ifndef LINK_H
#define LINK_H
#ident	"@(#)debugger:inc/common/Link.h	1.2"

// Link -- a doubly-linked list
//	
//
// OPERATIONS
//	prev(), next()	return pointers to the previous and next elements
//			in the list.
//	prepend(e)	given another element e in a list somewhere,
//			link this entry in before it. i.e. item->prepend(head)
//	append(e)	given another element e in a list somewhere,
//			link this entry in after it. i.e. item->append(tail)
//	unlink()	remove this element from its list.
//
//	rjoin(e)	join the "right" edge (next) of this entry to the
//			"left" edge (prev) of e (for list coalescing).
//	ljoin(e)	join the "left" edge of this entry to the
//			"right" edge of e.

class Link
{
	Link 	*_next;
	Link	*_prev;
public:
		Link()	{ _next = 0; _prev = 0; }
		~Link()	{}

	Link	*next()	{ return _next; }
	Link	*prev()	{ return _prev; }

	Link	*append(Link *);
	Link	*prepend(Link *);
	Link	*unlink();	// OK to pass a NULL this

	Link	*rjoin(Link *);
	Link	*ljoin(Link *);
};

// Stack -- a stack implemented by Links

class Stack : private Link 	// not a public derivation;
				//Link operations are not avail
{
public:
		Stack()	{}
		~Stack()	{}
	int	is_empty()	{ return !next(); }
	void	push( Link *p )	{ p->append( this ); }
	Link   *pop()		{ return Link::next()->unlink(); }
};


#endif /* LINK_H */
