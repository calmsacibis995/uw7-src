#ident  "@(#)movedir.c	1.3"
#ident  "$Header$"

#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include "messages.h"

extern	int	access(),
		rename();

extern	void	errmsg();

/*
	Move directory contents from one place to another
*/
int
move_dir(from, to)
	char *from;			/* directory to move files from */
	char *to;			/* dirctory to move files to */
{
	register rc = EX_SUCCESS;
	
	if (access(from, 0) == 0) {	/* home dir exists */
		if (access(to, 0) == 0) {	/* "new" directory exists */
			errmsg(M_NOSPACE, from, to);
			return EX_NOSPACE;
		}
		/*
		 * rename the "from" directory to the "to" directory
		*/
		(void) rename(from, to);
	}
	return rc;
}
