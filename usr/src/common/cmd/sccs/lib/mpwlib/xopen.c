#ident	"@(#)sccs:lib/mpwlib/xopen.c	6.4.1.1"
/*
	Interface to open(II) which differentiates among the various
	open errors.
	Returns file descriptor on success,
	fatal() on failure.
*/

# include "errno.h"
# include <ccstypes.h>

xopen(name,mode)
char name[];
mode_t mode;
{
	register int fd;
	extern int errno;
	int	open(), sprintf(), fatal(), xmsg();

	if ((fd = open(name,mode)) < 0) {
		if(errno == EACCES) {
			if(mode == 0)
				fd=fatal(":248:`%s' unreadable (ut5)",name);

			else if(mode == 1)
				fd=fatal(":249:`%s' unwritable (ut6)",name);

			else
				fd=fatal(":250:`%s' unreadable or unwritable (ut7)",name);

		}
		else
			fd = xmsg(name,"xopen");
	}
	return(fd);
}
