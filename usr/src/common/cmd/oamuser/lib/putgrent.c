#ident	"@(#)putgrent.c	1.2"
#ident  "$Header$"

#include <stdio.h>
#include <grp.h>
#include <unistd.h>

void
putgrent(grpstr, to)
struct group *grpstr;	/* group structure to write */
FILE *to;		/* file to write to */
{
	register char **memptr;		/* member vector pointer */

	(void) fprintf( to, "%s:%s:%ld:", grpstr->gr_name, grpstr->gr_passwd, 
		grpstr->gr_gid);

	memptr = grpstr->gr_mem;

	while( *memptr != NULL ) {
		(void) fprintf( to, "%s", *memptr );
		memptr++;
		if( *memptr != NULL) (void) fprintf( to, "," );
	}

	(void) fprintf( to, "\n" );
}
