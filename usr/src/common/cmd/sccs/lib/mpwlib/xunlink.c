#ident	"@(#)sccs:lib/mpwlib/xunlink.c	6.3"
/*
	Interface to unlink(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

int
xunlink(f)
char	*f;
{
	int	unlink(), xmsg();
	if (unlink(f))
		return(xmsg(f,"xunlink"));
	return(0);
}
