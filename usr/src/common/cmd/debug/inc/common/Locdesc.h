#ifndef Locdesc_h
#define Locdesc_h
#ident	"@(#)debugger:inc/common/Locdesc.h	1.5"

#include	"Vector.h"
#include	"Wordstack.h"
#include	"Place.h"

typedef void *	Addrexp;	// just a pointer to LocOp

class ProcObj;
class Frame;

class Locdesc {
	Vector		vector;
	Wordstack	stack;
	int		calculate_expr( Place &, ProcObj *, Frame *, int deref = 1 );
public:
			Locdesc()		{}
			Locdesc(Locdesc &l)	{ *this = l; }
			~Locdesc()		{}

	Locdesc &	clear();
	Locdesc &	add();
	Locdesc &	deref4();
	Locdesc &	reg( int );
	Locdesc &	reg_pair( int, int );
	Locdesc &	basereg( int );
	Locdesc &	offset( long );
	Locdesc &	addr( Iaddr );

	Locdesc &	adjust( Iaddr );

	Addrexp		addrexp();
	int		size()	{ return vector.size();	}
	Locdesc &	operator=( Locdesc& );
	Locdesc &	operator=( Addrexp );

	Place		place( ProcObj *, Frame * );
	Place		place( ProcObj *, Frame *, Iaddr base );
	int		place( Place &, ProcObj *, Frame *, Iaddr base, int deref = 1 );
};

#endif /* Locdesc_h */
