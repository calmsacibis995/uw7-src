/*ident	"@(#)ccsdemos:sc_demos/Array_set.C	1.1" */

#include "Array_set.h"
#include <iostream.h>

template <class T>
static void compare(ptrdiff_t n, T* p)
{
	assert(n==0 || *p >= *(p-1));
}

template <class T>
void Blockset<T>::check()
{
	//  Check the representation invariant:
	//      n<=b.size()
	//      b is sorted
	//      b contains no repetitions
	T* t1 = &b[0];
	T* t2 = &b[n];
	assert(n<=b.size());
	generate(compare,t1,t2);
	assert(unique(t1,t2)==t2);
}

