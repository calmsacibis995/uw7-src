#ident	"@(#)debugger:gui.d/common/Transcript.C	1.5"

#include "Transcript.h"
#include "config.h"
#include "Command.h"
#include "UI.h"
#include "NewHandle.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

Transcript::Transcript()
{
	total_bytes = bytes_used = 0;
	// 0th line unused
	lines = new size_t[LINES_TO_KEEP];
	last = 0;
	lines[0] = 0;
}

Transcript::~Transcript()
{
	if (string)
		free(string);
	delete lines;
}

void
Transcript::clear()
{
	bytes_used = 0;
	last = 0;
}

// BSIZ is the minimum growth of the string in bytes.
#define BSIZ	100

void
Transcript::getmemory(size_t howmuch)
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
		total_bytes = bytes_used = 0;
		newhandler.invoke_handler();
	}
}

// Add text to transcript.
// Update line count pointers.
// If number of lines is greater than LINES_TO_KEEP,
// clear out beginning lines.
void
Transcript::add(char *ptr)
{

	size_t	sz = strlen(ptr) + 1;
	char	*start;
	size_t	off;
	int	newline;

	if (sz > (total_bytes - bytes_used))
		getmemory(sz);

	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	start = string + bytes_used;
	strcpy(start, ptr);
	off = bytes_used;
	bytes_used += sz;
	newline = ((last == 0) || (*(start - 1) == '\n'));
	while(*start)
	{
		char	*next;

		if (newline)
		{
			// first string or
			// previous string ended in newline
			last++;
			if (last >= LINES_TO_KEEP)
			{
				off  -= discard_old();
				start = string + off;
			}
			lines[last] = off;
		}
		if ((next = strchr(start, '\n')) == 0)
			break;
		newline = 1;
		next++;
		off += (next - start);
		start = next;
	}
}

// Get rid of max_rows old lines, readjust offsets
// of existing lines.
// Returns the delta of offset of lines.
size_t
Transcript::discard_old()
{
	char	*from = string + lines[max_rows + 1];
	size_t	delta = from - string;
	int	i, j;

	bytes_used -= delta;
	(void)memmove(string, from, bytes_used);

	i = max_rows + 1;
	j = 1;
	for(; i < last; i++, j++)
	{
		lines[j] = lines[i] - delta;
	}
	last -= max_rows;
	return delta;
}
