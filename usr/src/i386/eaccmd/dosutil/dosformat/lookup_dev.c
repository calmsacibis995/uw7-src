/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/lookup_dev.c	1.1.1.2"
#ident  "$Header$"

/*
			lookup_device(handle)

	Search for an entry in our device table using
	the passed handle. Return its index - if found.
	Return -1 on failure.
*/
#include	"MS-DOS.h"

int
lookup_device(handle)
int	handle;
{
	int	index;

	/*
		Look for our specified handle.
	*/
	for (index = 0; index < MAX_DEVICES; index++)
		if (TABLE.handle == handle)
			break;

	/*
		If we reached end of our table, no match
	*/
	if (index == MAX_DEVICES)
		index = -1;

	return(index);
}
