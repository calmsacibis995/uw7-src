#ifndef Vector_h
#define Vector_h
#ident	"@(#)debugger:inc/common/Vector.h	1.3"

#include <stdlib.h>

class Vector {
	size_t		total_bytes;
	size_t		bytes_used;
	void *		vector;
	void		getmemory(size_t);
	void		check();
public:
			Vector();
			Vector(const Vector &);
			~Vector()		{ if ( vector ) free(vector); }
	Vector &	add(void *, size_t);
	Vector &	drop(size_t i)		{ if ( i <= bytes_used )
							bytes_used -= i;
						  return *this; }
	void *		ptr()			{ return vector;	}
	size_t		size()			{ return bytes_used;	}
	Vector &	operator= (const Vector&);
	Vector &	clear()			{ bytes_used = 0; return *this;	}
#if DEBUG
	Vector &	report(char * = 0);
#endif
};

// pool of global scratch Vectors

#define VPOOL_SIZE	2
class Vector_pool {
	Vector	*pool[VPOOL_SIZE];
	int	top;
public:
		Vector_pool(Vector v[VPOOL_SIZE]);
		~Vector_pool() {}
	Vector	*get();
	void	put(Vector *);
};

extern Vector_pool	vec_pool;

#endif /* Vector_h */
