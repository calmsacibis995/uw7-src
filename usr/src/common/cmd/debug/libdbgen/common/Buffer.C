#ident	"@(#)debugger:libdbgen/common/Buffer.C	1.8"
#include	"Buffer.h"
#include	"NewHandle.h"
#include	"UIutil.h"
#include	<stdlib.h>
#include	<string.h>

// BSIZ is the minimum growth of the string in bytes.
#define BSIZ	100

void
Buffer::getmemory(size_t howmuch)
{
	size_t	sz;

	sz = howmuch < BSIZ ? BSIZ : howmuch;
	if (total_bytes == 0)
	{
		total_bytes = sz;
		string = (char *)malloc(sz);
	}
	else
	{
		total_bytes = total_bytes + sz;
		string = (char *)realloc(string,total_bytes);
	}
	if (string == 0)
	{
		bytes_used = total_bytes = 0;
		newhandler.invoke_handler();
	}
}

// append a string to the existing string, leaving room for the
// null byte

void
Buffer::add(const char *p)
{
	size_t sz = strlen(p) + 1;

	if (sz > (total_bytes - bytes_used))
		getmemory(sz);

	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	strcpy(string + bytes_used, p);
	bytes_used += sz;
}

void
Buffer::add(const char *p, size_t len)
{
	size_t	sz = len + 1;
	if (sz > (total_bytes - bytes_used))
		getmemory(sz);

	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	strncpy(string + bytes_used, p, len);
	bytes_used += sz;
	*(string + bytes_used - 1) = 0;
}

// append c to the string, making sure the string is
// still null-terminated

void
Buffer::add(char c)
{
	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	if (total_bytes - bytes_used < 2)
		getmemory(2);
	string[bytes_used++] = c;
	string[bytes_used++] = '\0';
}

// implement a simple stack of buffers for use in rest of debugger
Buffer_pool::Buffer_pool(Buffer b[BPOOL_SIZE])
{
	for (int i = 0; i < BPOOL_SIZE; i++)
		pool[i] = &b[i];
	top = 0;
}

Buffer *
Buffer_pool::get()
{
	Buffer	*b;

	if (pool[top] == 0)
	{
		interface_error("Buffer_pool::get", __LINE__, 1);
		return 0;
	}
	b = pool[top];
	pool[top] = 0;
	top++;
	if (top > (BPOOL_SIZE-1))
		top = 0;
	return b;
}

void
Buffer_pool::put(Buffer *buf)
{
	top--;
	if (top < 0)
		top = BPOOL_SIZE - 1;
	if (pool[top] != 0)
	{
		interface_error("Buffer_pool::get", __LINE__, 1);
		return;
	}
	pool[top] = buf;
}
