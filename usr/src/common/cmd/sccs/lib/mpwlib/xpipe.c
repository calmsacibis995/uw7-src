#ident	"@(#)sccs:lib/mpwlib/xpipe.c	6.3"
/*
	Interface to pipe(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

int
xpipe(t)
int *t;
{
	static char p[]="pipe";
	int	pipe(), xmsg();

	if (pipe(t) == 0)
		return(0);
	return(xmsg(p,p));
}
