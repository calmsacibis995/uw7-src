#ident	"@(#)sccs:lib/mpwlib/xcreat.c	6.4.1.1"
# include	"../../hdr/defines.h"
# include	<ccstypes.h>


/*
	"Sensible" creat: write permission in directory is required in
	all cases, and created file is guaranteed to have specified mode
	and be owned by effective user.
	(It does this by first unlinking the file to be created.)
	Returns file descriptor on success,
	fatal() on failure.
*/
int
xcreat(name,mode)
char *name;
mode_t mode;
{
	register int fd;
	char d[FILESIZE];
	int	xmsg(), creat(), unlink(), fatal(), stat();

	copy(name,d);
	if (!exists(dname(d))) {
		fatal(":240:directory `%s' nonexistent (ut1)",d);
	}
	unlink(name);
	if ((fd = creat(name,mode)) >= 0)
		return(fd);
	return(xmsg(name,"xcreat"));
}
