#ident	"@(#)ksh93:src/lib/libast/vec/vecfree.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * file to string vector support
 */

#include <ast.h>
#include <vecargs.h>

/*
 * free a string vector generated by vecload()
 *
 * retain!=0 frees the string pointers but retains the string data
 * in this case the data is permanently allocated
 */

void
vecfree(register char** vec, int retain)
{
	if (vec)
	{
		if (*(vec -= 2) && !retain) free(*vec);
		free(vec);
	}
}