#ident	"@(#)debugger:libdbgen/common/Link.C	1.1"
#include "Link.h"

Link *
Link::prepend(Link *elem)
{
	if(_prev = elem->_prev)
		_prev->_next = this;
	elem->_prev = this;
	_next = elem;
	return this;
}

Link *
Link::append(Link *elem)
{
	if(_next = elem->_next)
		_next->_prev = this;
	_prev = elem;
	elem->_next = this;
	return this;
}

Link *
Link::unlink()
{
	if(_next)
		_next->_prev = _prev;
	if(_prev)
		_prev->_next = _next;
	_next = 0;
	_prev = 0;
	return this;
}

Link *
Link::rjoin(Link *elem)
{
	_next = elem;
	if ( elem )
		elem->_prev = this;
	return this;
}

Link *
Link::ljoin(Link *elem)
{
	_prev = elem;
	if ( elem )
		elem->_next = this;
	return this;
}
