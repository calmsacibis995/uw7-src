#ifndef Wordstack_h
#define Wordstack_h
#ident	"@(#)debugger:inc/common/Wordstack.h	1.2"

#include	"Vector.h"

class Wordstack {
	Vector		vector;
	int		count;
public:
			Wordstack();
			~Wordstack()	{}
	void		push( unsigned long );
	unsigned long	pop();
	int		not_empty()	{	return count>0;	}
	void		clear()		{	vector.clear(); count = 0; }
};

#endif	/* Wordstack_h */
