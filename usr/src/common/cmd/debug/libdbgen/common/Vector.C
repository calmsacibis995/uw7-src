#ident	"@(#)debugger:libdbgen/common/Vector.C	1.8"
#include	"Vector.h"
#include	"UIutil.h"
#include	"NewHandle.h"
#include	<string.h>

// BSIZ is the minimum growth of the vector in bytes.
#define BSIZ	100

Vector::Vector()
{
	bytes_used = 0;
	total_bytes = 0;
	vector = 0;
}

void
Vector::getmemory(size_t howmuch)	// clients assume word alignment!
{
	size_t	sz;

	sz = howmuch < BSIZ ? BSIZ : howmuch;
	if (total_bytes == 0)
	{
		vector = (char *)malloc(sz);
		total_bytes = sz;
	}
	else
	{
		total_bytes = total_bytes + sz;
		vector = (char *)realloc(vector,total_bytes);
	}
	check();
}

Vector::Vector(const Vector & v)
{
	vector = (char *)malloc(v.total_bytes);
	check();
	memcpy(vector,v.vector,v.bytes_used);
	total_bytes = v.total_bytes;
	bytes_used = v.bytes_used;
}

Vector &
Vector::add(void * p, size_t sz)
{
	if (sz > (total_bytes - bytes_used))
	{
		getmemory(sz);
	}
	memcpy((char*)vector + bytes_used, (char *)p, sz);
	bytes_used += sz;
	return *this;
}

Vector &
Vector::operator=(const Vector &v)
{
	if (this != &v)
	{
		if (vector) free (vector);
		if (v.total_bytes > 0) // check fails for 0
		{
			vector = (char *)malloc(v.total_bytes);
			check();
			memcpy(vector,v.vector,v.bytes_used);
		}
		total_bytes = v.total_bytes;
		bytes_used = v.bytes_used;
	}
	return *this;
}

#if DEBUG
#include <stdio.h>

Vector &
Vector::report(char * msg)
{
	if (msg)
		printf("%s\n",msg);
	printf("\ttotal bytes : %d (%#x)\n",total_bytes,total_bytes);
	printf("\tbytes used : %d (%#x)\n",bytes_used,bytes_used);
	printf("\tvector : (%#x) >%s<\n",vector,vector);
	if (bytes_used != 0)
		printf("\tvector[%d] : %c (%#x)\n",bytes_used-1,
			((char *)vector)[bytes_used-1],
			((long *)vector)[bytes_used-1]);
	printf("\tvector[%d] : %c (%#x)\n",bytes_used,
		((char *)vector)[bytes_used],((long *)vector)[bytes_used]);
	return *this;
}
#endif

void
Vector::check()
{
	if (!vector)
	{
		total_bytes = 0;
		bytes_used = 0;
		newhandler.invoke_handler();
	}
}

// implement a simple stack of vectors for use in rest of debugger
Vector_pool::Vector_pool(Vector v[VPOOL_SIZE])
{
	for (int i = 0; i < VPOOL_SIZE; i++)
		pool[i] = &v[i];
	top = 0;
}

Vector *
Vector_pool::get()
{
	Vector	*v;

	if (pool[top] == 0)
	{
		interface_error("Vector_pool::get", __LINE__, 1);
		return 0;
	}
	v = pool[top];
	pool[top] = 0;
	top++;
	if (top > (VPOOL_SIZE-1))
		top = 0;
	return v;
}

void
Vector_pool::put(Vector *v)
{
	top--;
	if (top < 0)
		top = VPOOL_SIZE - 1;
	if (pool[top] != 0)
	{
		interface_error("Vector_pool::get", __LINE__, 1);
		return;
	}
	pool[top] = v;
}
